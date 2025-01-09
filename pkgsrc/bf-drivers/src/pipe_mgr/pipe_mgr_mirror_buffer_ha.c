/*******************************************************************************
 *  Copyright (C) 2024 Intel Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions
 *  and limitations under the License.
 *
 *
 *  SPDX-License-Identifier: Apache-2.0
 ******************************************************************************/


/*!
 * @file pipe_exm_drv_workflows.c
 * @date
 *
 * Provides driver workflow implementations to be used by the action data table
 * manager for performing various operations on the action data tables.
 *
 */
#include <dvm/bf_drv_intf.h>
#include <lld/bf_dma_if.h>
#include <pipe_mgr/pipe_mgr_intf.h>
#include <pipe_mgr/pipe_mgr_mirror_intf.h>

#include "pipe_mgr_int.h"
#include "pipe_mgr_drv.h"
#include "pipe_mgr_mirror_buffer_comm.h"
#include "pipe_mgr_tof_mirror_buffer.h"
#include "pipe_mgr_tof2_mirror_buffer.h"
#include "pipe_mgr_tof3_mirror_buffer.h"
#include "pipe_mgr_mirror_buffer.h"
#include "pipe_mgr_mirror_buffer_ha.h"

static mirror_session_cache_t mirror_session_cache_ha[PIPE_MGR_NUM_DEVICES];

static unsigned long make_ha_key(bf_mirror_id_t sid, bf_dev_pipe_t pipe) {
  unsigned long r = sid;
  r <<= 16;
  r |= pipe & 0xFFFF;
  return r;
}

static void decode_ha_key(unsigned long key,
                          bf_mirror_id_t *sid,
                          bf_dev_pipe_t *pipe_id) {
  *sid = key >> 16;
  *pipe_id = key & 0xFFFF;
}

static void save_ha_session_info(rmt_dev_info_t *dev_info,
                                 mirror_info_node_t *new_node) {
  bf_dev_id_t dev_id = dev_info->dev_id;
  PIPE_MGR_LOCK(&mirror_session_cache_ha[dev_id].mirror_lock);

  unsigned long key = make_ha_key(new_node->sid, new_node->pipe_id);
  void *data = NULL;
  bf_map_t *map = &mirror_session_cache_ha[dev_id].mir_sess;

  /* First look up the session to see if it exists.  If it does we are going to
   * replace it so take it out of the map and delete it. */
  bf_map_sts_t s = bf_map_get_rmv(map, key, &data);
  if (BF_MAP_OK == s) {
    pipe_mgr_free_mirr_info_node((mirror_info_node_t *)data);
  }

  /* Save the new session. */
  data = new_node;
  bf_map_add(map, key, data);

  PIPE_MGR_UNLOCK(&mirror_session_cache_ha[dev_id].mirror_lock);
  return;
}

static mirror_info_node_t *get_ha_session_by_pipe(bf_dev_id_t dev_id,
                                                  bf_mirror_id_t sid,
                                                  bf_dev_pipe_t pipe_id) {
  unsigned long key = make_ha_key(sid, pipe_id);
  void *data = NULL;
  PIPE_MGR_LOCK(&mirror_session_cache_ha[dev_id].mirror_lock);
  bf_map_t *map = &mirror_session_cache_ha[dev_id].mir_sess;
  bf_map_sts_t s = bf_map_get(map, key, &data);
  PIPE_MGR_UNLOCK(&mirror_session_cache_ha[dev_id].mirror_lock);

  if (s != BF_MAP_OK) return NULL;
  return data;
}

static void remove_ha_session_by_pipe(bf_dev_id_t dev_id,
                                      bf_mirror_id_t sid,
                                      bf_dev_pipe_t pipe_id) {
  unsigned long key = make_ha_key(sid, pipe_id);
  void *data;
  PIPE_MGR_LOCK(&mirror_session_cache_ha[dev_id].mirror_lock);
  bf_map_t *map = &mirror_session_cache_ha[dev_id].mir_sess;
  bf_map_sts_t s = bf_map_get_rmv(map, key, &data);
  PIPE_MGR_UNLOCK(&mirror_session_cache_ha[dev_id].mirror_lock);

  if (s == BF_MAP_OK) pipe_mgr_free_mirr_info_node((mirror_info_node_t *)data);
}

#if 0
static pipe_status_t update_ha_session_by_pipe(bf_dev_id_t dev_id,
                                               bf_mirror_id_t sid,
                                               bf_dev_pipe_t pipe_id,
                                               mirror_ha_result_e ha_result) {
  unsigned long key = make_ha_key(sid, pipe_id);
  void *data = NULL;
  PIPE_MGR_LOCK(&mirror_session_cache_ha[dev_id].mirror_lock);
  bf_map_t *map = &mirror_session_cache_ha[dev_id].mir_sess;
  bf_map_sts_t s = bf_map_get(map, key, &data);
  PIPE_MGR_UNLOCK(&mirror_session_cache_ha[dev_id].mirror_lock);

  if (s != BF_MAP_OK) return PIPE_INVALID_ARG;
  mirror_info_node_t *node = (mirror_info_node_t *)data;
  node->ha_result = ha_result;
  return PIPE_SUCCESS;
}
#endif

static pipe_status_t first_ha_session(bf_dev_id_t dev_id,
                                      bf_mirror_id_t *sid,
                                      bf_dev_pipe_t *pipe_id,
                                      mirror_info_node_t *info) {
  unsigned long key;
  void *data;
  PIPE_MGR_LOCK(&mirror_session_cache_ha[dev_id].mirror_lock);
  bf_map_sts_t s =
      bf_map_get_first(&mirror_session_cache_ha[dev_id].mir_sess, &key, &data);
  if (s == BF_MAP_OK) {
    decode_ha_key(key, sid, pipe_id);
    *info = *(mirror_info_node_t *)data;
    PIPE_MGR_UNLOCK(&mirror_session_cache_ha[dev_id].mirror_lock);
    return PIPE_SUCCESS;
  } else {
    PIPE_MGR_UNLOCK(&mirror_session_cache_ha[dev_id].mirror_lock);
    return PIPE_OBJ_NOT_FOUND;
  }
}

static void pipe_mgr_mirror_buf_ha_cleanup(bf_dev_id_t dev_id) {
  if (mirror_session_cache_ha[dev_id].ready) {
    PIPE_MGR_LOCK_DESTROY(&mirror_session_cache_ha[dev_id].mirror_lock);
    bf_map_t *map = &mirror_session_cache_ha[dev_id].mir_sess;
    unsigned long key;
    void *data;
    while (BF_MAP_OK == bf_map_get_first_rmv(map, &key, &data)) {
      pipe_mgr_free_mirr_info_node((mirror_info_node_t *)data);
    }
    bf_map_destroy(&mirror_session_cache_ha[dev_id].mir_sess);
    mirror_session_cache_ha[dev_id].ready = false;
  }
}

pipe_status_t pipe_mgr_mirror_hitless_ha_init(pipe_sess_hdl_t sess_hdl,
                                              rmt_dev_info_t *dev_info) {
  bf_dev_id_t dev_id = dev_info->dev_id;
  int start = 0, end = 0;
  int num_pipes = dev_info->num_active_pipes;
  pipe_status_t sts = PIPE_SUCCESS;
  mirror_info_node_t node;
  pipe_mgr_mirror_session_info_t *session_info = &node.session_info;
  bool session_valid = false;
  bf_dev_pipe_t phy_pipe = 0;

  PIPE_MGR_LOCK_INIT(mirror_session_cache_ha[dev_id].mirror_lock);

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      start = 0;
      end = PIPE_MGR_TOF_MIRROR_SESSION_MAX;
      break;
    case BF_DEV_FAMILY_TOFINO2:
      start = 0;
      end = PIPE_MGR_TOF2_MIRROR_SESSION_MAX;
      break;
    case BF_DEV_FAMILY_TOFINO3:
      start = 0;
      end = PIPE_MGR_TOF3_MIRROR_SESSION_MAX;
      /* Not supported */
      PIPE_MGR_DBGCHK(0);
      sts = PIPE_UNEXPECTED;
      goto cleanup;
      break;

    case BF_DEV_FAMILY_UNKNOWN:
      PIPE_MGR_DBGCHK(0);
      sts = PIPE_UNEXPECTED;
      goto cleanup;
  }

  for (int log_pipe = 0; log_pipe < num_pipes; ++log_pipe) {
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe, &phy_pipe);
    for (int sid = start; sid < end; ++sid) {
      PIPE_MGR_MEMSET(&node, 0, sizeof node);
      session_valid = false;
      node.sid = sid;
      node.pipe_id = log_pipe;
      if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO) {
        if (pipe_mgr_tof_mirror_buf_sid_is_coalescing(sid)) {
          sts = pipe_mgr_tof_mirror_buf_coal_session_read(sess_hdl,
                                                          dev_id,
                                                          phy_pipe,
                                                          sid,
                                                          session_info,
                                                          &node.enable_ing,
                                                          &session_valid);
          if (PIPE_SUCCESS != sts) goto cleanup;
        } else {
          sts = pipe_mgr_tof_mirror_buf_norm_session_read(sess_hdl,
                                                          dev_id,
                                                          phy_pipe,
                                                          sid,
                                                          session_info,
                                                          &node.enable_ing,
                                                          &node.enable_egr,
                                                          &session_valid);
          if (PIPE_SUCCESS != sts) goto cleanup;
        }
      } else if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO2) {
        sts = pipe_mgr_tof2_mirror_buf_session_read(sess_hdl,
                                                    dev_id,
                                                    phy_pipe,
                                                    sid,
                                                    session_info,
                                                    &node.enable_ing,
                                                    &node.enable_egr,
                                                    &session_valid);
        if (PIPE_SUCCESS != sts) goto cleanup;
      }
      if (session_valid) {
        mirror_info_node_t *node_p =
            PIPE_MGR_CALLOC(1, sizeof(mirror_info_node_t));
        if (!node_p) {
          goto cleanup;
        }
        memcpy(node_p, &node, sizeof(node));
        save_ha_session_info(dev_info, node_p);
      }
    }
  }
  mirror_session_cache_ha[dev_id].ready = true;
  return PIPE_SUCCESS;

cleanup:
  PIPE_MGR_LOCK_DESTROY(&mirror_session_cache_ha[dev_id].mirror_lock);
  bf_map_destroy(&mirror_session_cache_ha[dev_id].mir_sess);
  mirror_session_cache_ha[dev_id].ready = false;

  return sts;
}

/* Compare mirror config */
static bool pipe_mgr_ha_mirror_buf_cfg_compare(bf_dev_id_t dev_id,
                                               bf_dev_pipe_t log_pipe,
                                               bf_mirror_id_t sid,
                                               mirror_info_node_t *replay_cfg) {
  mirror_info_node_t *hw_cfg = NULL;
  rmt_dev_info_t *dev_info = NULL;

  dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    return false;
  }

  hw_cfg = get_ha_session_by_pipe(dev_id, sid, log_pipe);
  if (!hw_cfg) {
    LOG_ERROR("Mirror session %d not there in ASIC on pipe %x, dev %d ",
              sid,
              log_pipe,
              dev_id);
    return false;
  }

  if ((hw_cfg->enable_ing != replay_cfg->enable_ing) ||
      (hw_cfg->enable_egr != replay_cfg->enable_egr)) {
    LOG_TRACE(
        "Mirror[sid=%d]. log-Pipe %d, Enable user cfg (ing %d, egr %d) does "
        "not match with hw-cfg (ing %d, egr %d)",
        sid,
        log_pipe,
        replay_cfg->enable_ing,
        replay_cfg->enable_egr,
        hw_cfg->enable_ing,
        hw_cfg->enable_egr);
    return false;
  }

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return pipe_mgr_tof_ha_mirror_buf_cfg_compare(dev_info,
                                                    log_pipe,
                                                    sid,
                                                    &replay_cfg->session_info,
                                                    &hw_cfg->session_info);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      return pipe_mgr_tof2_ha_mirror_buf_cfg_compare(dev_info,
                                                     log_pipe,
                                                     sid,
                                                     &replay_cfg->session_info,
                                                     &hw_cfg->session_info);
    case BF_DEV_FAMILY_TOFINO3:
      /* Need to implement */
      PIPE_MGR_DBGCHK(0);
      return false;

    case BF_DEV_FAMILY_UNKNOWN:
      PIPE_MGR_DBGCHK(0);
      return false;
  }

  return true;
}

pipe_status_t pipe_mgr_mirror_ha_compute_delta_changes(
    pipe_sess_hdl_t shdl, rmt_dev_info_t *dev_info) {
  bf_dev_id_t dev_id = dev_info->dev_id;
  pipe_status_t sts = PIPE_SUCCESS;
  bf_mirror_id_t sid = 0;
  bf_dev_pipe_t pipe_id = 0;
  mirror_info_node_t node, *replay_cfg = NULL, *hw_cfg = NULL;
  int log_pipe = 0, log_pipe_start = 0, log_pipe_end = 0;

  /*
    Go over the SW DB entries. Find the HW DB entry corresponding to it.
    If entry exists in ASIC, compare to make sure data is same. If data is
    different overwrite with replayed config.
    If entry does not exist in ASIC, then write the replayed config in ASIC.
  */
  sts = pipe_mgr_mirror_buf_get_first_session(dev_id, &sid, &node);

  while (sts == PIPE_SUCCESS) {
    pipe_id = node.pipe_id;
    replay_cfg = pipe_mgr_mirror_buf_get_session_by_pipe(dev_id, sid, pipe_id);
    PIPE_MGR_ASSERT(replay_cfg != NULL);

    if (pipe_id == BF_DEV_PIPE_ALL) {
      log_pipe_start = 0;
      log_pipe_end = dev_info->num_active_pipes - 1;
    } else {
      log_pipe_start = pipe_id;
      log_pipe_end = pipe_id;
    }

    for (log_pipe = log_pipe_start; log_pipe <= log_pipe_end; log_pipe++) {
      hw_cfg = get_ha_session_by_pipe(dev_id, sid, log_pipe);
      if (!hw_cfg) {
        LOG_TRACE(
            "Mirror session %d config does not exist in ASIC on pipe %x,"
            " dev %d, will program it",
            sid,
            log_pipe,
            dev_id);
        /* Set the config in ASIC */
        pipe_mgr_mirror_buf_session_set_hw(shdl, dev_info, replay_cfg);
      } else {
        if (!pipe_mgr_ha_mirror_buf_cfg_compare(
                dev_id, log_pipe, sid, replay_cfg)) {
          LOG_TRACE(
              "Mirror session %d config mismatch on pipe %x, dev %d,"
              " overwriting with replayed config",
              sid,
              log_pipe,
              dev_id);

          pipe_mgr_mirror_buf_session_set_hw(shdl, dev_info, replay_cfg);
        }
        remove_ha_session_by_pipe(dev_id, sid, log_pipe);
      }
    }

    sts =
        pipe_mgr_mirror_buf_get_next_session(dev_id, sid, pipe_id, &sid, &node);
  }

  /*
     HW HA DB now has entries that were not replayed by user,
     Clear these entries from ASIC
  */
  sts = first_ha_session(dev_id, &sid, &pipe_id, &node);
  while (sts == PIPE_SUCCESS) {
    LOG_TRACE(
        "Mirror session %d config not replayed for pipe %x, dev %d,"
        " clearing config from ASIC",
        sid,
        pipe_id,
        dev_id);
    /* Clear this session info from ASIC */
    pipe_mgr_mirror_buf_init_one_mirror_session(shdl, dev_info, sid, pipe_id);

    remove_ha_session_by_pipe(dev_id, sid, pipe_id);
    sts = first_ha_session(dev_id, &sid, &pipe_id, &node);
  }

  pipe_mgr_mirror_buf_ha_cleanup(dev_id);

  return PIPE_SUCCESS;
}
