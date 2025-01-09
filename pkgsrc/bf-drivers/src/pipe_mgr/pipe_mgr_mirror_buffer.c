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
//#include <lld/lld_reg_if.h>
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

static mirror_session_cache_t mirror_session_cache[PIPE_MGR_NUM_DEVICES];

static unsigned long make_key(bf_mirror_id_t sid, bf_dev_pipe_t pipe) {
  unsigned long r = sid;
  r <<= 16;
  r |= pipe & 0xFFFF;
  return r;
}
static void decode_key(unsigned long key,
                       bf_mirror_id_t *sid,
                       bf_dev_pipe_t *pipe_id) {
  *sid = key >> 16;
  *pipe_id = key & 0xFFFF;
}
static void save_session_info(rmt_dev_info_t *dev_info,
                              mirror_info_node_t *new_node) {
  bf_dev_id_t dev_id = dev_info->dev_id;
  PIPE_MGR_LOCK(&mirror_session_cache[dev_id].mirror_lock);

  unsigned long key = make_key(new_node->sid, new_node->pipe_id);
  void *data = NULL;
  bf_map_t *map = &mirror_session_cache[dev_id].mir_sess;

  /* First look up the session to see if it exists.  If it does we are going to
   * replace it so take it out of the map and delete it. */
  bf_map_sts_t s = bf_map_get_rmv(map, key, &data);
  if (BF_MAP_OK == s) {
    pipe_mgr_free_mirr_info_node((mirror_info_node_t *)data);
  }

  /* Save the new session. */
  data = new_node;
  bf_map_add(map, key, data);

  PIPE_MGR_UNLOCK(&mirror_session_cache[dev_id].mirror_lock);
  return;
}

mirror_info_node_t *pipe_mgr_mirror_buf_get_session_by_pipe(
    bf_dev_id_t dev_id, bf_mirror_id_t sid, bf_dev_pipe_t pipe_id) {
  unsigned long key = make_key(sid, pipe_id);
  void *data = NULL;
  PIPE_MGR_LOCK(&mirror_session_cache[dev_id].mirror_lock);
  bf_map_t *map = &mirror_session_cache[dev_id].mir_sess;
  bf_map_sts_t s = bf_map_get(map, key, &data);
  PIPE_MGR_UNLOCK(&mirror_session_cache[dev_id].mirror_lock);

  if (s != BF_MAP_OK) return NULL;
  return data;
}
static void remove_session_by_pipe(bf_dev_id_t dev_id,
                                   bf_mirror_id_t sid,
                                   bf_dev_pipe_t pipe_id) {
  unsigned long key = make_key(sid, pipe_id);
  void *data;
  PIPE_MGR_LOCK(&mirror_session_cache[dev_id].mirror_lock);
  bf_map_t *map = &mirror_session_cache[dev_id].mir_sess;
  bf_map_sts_t s = bf_map_get_rmv(map, key, &data);
  PIPE_MGR_UNLOCK(&mirror_session_cache[dev_id].mirror_lock);

  if (s == BF_MAP_OK) pipe_mgr_free_mirr_info_node((mirror_info_node_t *)data);
}
pipe_status_t pipe_mgr_mirror_buf_get_first_session(bf_dev_id_t dev_id,
                                                    bf_mirror_id_t *sid,
                                                    mirror_info_node_t *info) {
  unsigned long key;
  void *data;
  bf_dev_pipe_t pipe_id;
  PIPE_MGR_LOCK(&mirror_session_cache[dev_id].mirror_lock);
  bf_map_sts_t s =
      bf_map_get_first(&mirror_session_cache[dev_id].mir_sess, &key, &data);
  if (s == BF_MAP_OK) {
    decode_key(key, sid, &pipe_id);
    *info = *(mirror_info_node_t *)data;
    PIPE_MGR_UNLOCK(&mirror_session_cache[dev_id].mirror_lock);
    return PIPE_SUCCESS;
  } else {
    PIPE_MGR_UNLOCK(&mirror_session_cache[dev_id].mirror_lock);
    return PIPE_OBJ_NOT_FOUND;
  }
}
pipe_status_t pipe_mgr_mirror_buf_get_next_session(bf_dev_id_t dev_id,
                                                   bf_mirror_id_t cur_sid,
                                                   bf_dev_pipe_t cur_pipe_id,
                                                   bf_mirror_id_t *next_sid,
                                                   mirror_info_node_t *info) {
  unsigned long key = make_key(cur_sid, cur_pipe_id);
  void *data;
  bf_dev_pipe_t pipe_id;
  PIPE_MGR_LOCK(&mirror_session_cache[dev_id].mirror_lock);
  bf_map_sts_t s =
      bf_map_get_next(&mirror_session_cache[dev_id].mir_sess, &key, &data);
  if (s == BF_MAP_OK) {
    decode_key(key, next_sid, &pipe_id);
    *info = *(mirror_info_node_t *)data;
    PIPE_MGR_UNLOCK(&mirror_session_cache[dev_id].mirror_lock);
    return PIPE_SUCCESS;
  } else {
    PIPE_MGR_UNLOCK(&mirror_session_cache[dev_id].mirror_lock);
    return PIPE_OBJ_NOT_FOUND;
  }
}

static pipe_status_t mirror_buf_set_pbm(rmt_dev_info_t *dev_info,
                                        bf_dev_pipe_t pipe_id,
                                        pipe_bitmap_t *pipes) {
  PIPE_BITMAP_INIT(pipes, PIPE_BMP_SIZE);
  if (pipe_id == BF_DEV_PIPE_ALL) {
    for (unsigned int i = 0; i < dev_info->num_active_pipes; ++i) {
      PIPE_BITMAP_SET(pipes, i);
    }
  } else {
    if (pipe_id >= dev_info->num_active_pipes) {
      return PIPE_INVALID_ARG;
    }
    PIPE_BITMAP_SET(pipes, pipe_id);
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_mirror_buf_init(pipe_sess_hdl_t sess_hdl,
                                       bf_dev_id_t dev_id) {
  pipe_status_t sts = PIPE_SUCCESS;

  LOG_TRACE("%s: Start", __func__);

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return PIPE_INVALID_ARG;

  if (!pipe_mgr_init_mode_fast_recfg_quick(dev_id)) {
    bf_map_init(&mirror_session_cache[dev_id].mir_sess);
  }
  PIPE_MGR_LOCK_INIT(mirror_session_cache[dev_id].mirror_lock);
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      sts = pipe_mgr_tof_mirror_buf_init(sess_hdl, dev_id);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      sts = pipe_mgr_tof2_mirror_buf_init(sess_hdl, dev_id);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      sts = pipe_mgr_tof3_mirror_buf_init(sess_hdl, dev_id);
      break;





    case BF_DEV_FAMILY_UNKNOWN:
      LOG_TRACE("%s: Unknown dev_family", __func__);
      sts = PIPE_INVALID_ARG;
      break;
  }

  if (!pipe_mgr_init_mode_fast_recfg_quick(dev_id)) {
    if (sts != PIPE_SUCCESS) {
      PIPE_MGR_LOCK_DESTROY(&mirror_session_cache[dev_id].mirror_lock);
      bf_map_destroy(&mirror_session_cache[dev_id].mir_sess);
      mirror_session_cache[dev_id].ready = false;
    } else {
      mirror_session_cache[dev_id].ready = true;
    }
  }

  if (pipe_mgr_init_mode_hitless(dev_id)) {
    /* Handle the HA stuff */
    sts = pipe_mgr_mirror_hitless_ha_init(sess_hdl, dev_info);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s: failed to initialize mirror hitless HA for device %u, status %u",
          __func__,
          dev_id,
          sts);
      return sts;
    }
    LOG_TRACE("%s: Finished pipe_mgr_mirror_hitless_ha_init", __func__);
  }

  LOG_TRACE("%s: Done", __func__);
  return sts;
}

pipe_status_t pipe_mgr_mirror_buf_init_one_mirror_session(
    pipe_sess_hdl_t shdl,
    rmt_dev_info_t *dev_info,
    bf_mirror_id_t sid,
    bf_dev_pipe_t pipe_tgt) {
  pipe_bitmap_t pbm;
  pipe_status_t sts = mirror_buf_set_pbm(dev_info, pipe_tgt, &pbm);
  if (PIPE_SUCCESS != sts) return sts;

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return pipe_mgr_tof_mirror_buf_init_session(
          shdl, sid, dev_info->dev_id, pbm);
    case BF_DEV_FAMILY_TOFINO2:
      return pipe_mgr_tof2_mirror_buf_init_session(
          shdl, sid, dev_info->dev_id, pbm);
    case BF_DEV_FAMILY_TOFINO3:
      return pipe_mgr_tof3_mirror_buf_init_session(
          shdl, sid, dev_info->dev_id, pbm);

    case BF_DEV_FAMILY_UNKNOWN:
      break;
  }
  PIPE_MGR_DBGCHK(0);
  return PIPE_UNEXPECTED;
}

pipe_status_t pipe_mgr_mirror_buf_cfg_sessions(pipe_sess_hdl_t shdl,
                                               rmt_dev_info_t *dev_info) {
  bf_dev_id_t dev_id = dev_info->dev_id;
  int num_pipes = dev_info->num_active_pipes;
  int start = 0, end = 0;
  pipe_status_t sts = PIPE_SUCCESS;
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
      break;




    case BF_DEV_FAMILY_UNKNOWN:
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }

  /* Go through all sessions that were configured for all pipes AND sessions
   * not configured at all. */
  for (int i = start; i < end; ++i) {
    mirror_info_node_t *n =
        pipe_mgr_mirror_buf_get_session_by_pipe(dev_id, i, BF_DEV_PIPE_ALL);
    if (n) {
      sts = pipe_mgr_mirror_buf_session_set_hw(shdl, dev_info, n);
      if (PIPE_SUCCESS != sts) return sts;
    } else {
      bool found_it = false;
      for (int log_pipe = 0; log_pipe < num_pipes && !found_it; ++log_pipe) {
        n = pipe_mgr_mirror_buf_get_session_by_pipe(dev_id, i, log_pipe);
        found_it = !!n;
      }
      if (!found_it) {
        sts = pipe_mgr_mirror_buf_init_one_mirror_session(
            shdl, dev_info, i, BF_DEV_PIPE_ALL);
        if (PIPE_SUCCESS != sts) return sts;
      }
    }
  }

  /* Find all sessions configured per pipe.  Plus, find sessions not configured
   * for the pipe but configured for another pipe. */
  for (int log_pipe = 0; log_pipe < num_pipes; ++log_pipe) {
    for (int i = start; i < end; ++i) {
      mirror_info_node_t *n =
          pipe_mgr_mirror_buf_get_session_by_pipe(dev_id, i, log_pipe);
      if (n) {
        sts = pipe_mgr_mirror_buf_session_set_hw(shdl, dev_info, n);
        if (PIPE_SUCCESS != sts) return sts;
      } else {
        /* Session was not found on this pipe.  If it is there on any other
         * pipe go ahead and initialize it on this pipe. */
        bool found_it = false;
        for (int other_pipe = log_pipe + 1; other_pipe < num_pipes;
             ++other_pipe) {
          n = pipe_mgr_mirror_buf_get_session_by_pipe(dev_id, i, other_pipe);
          if (n) {
            found_it = true;
            break;
          }
        }
        if (found_it) {
          sts = pipe_mgr_mirror_buf_init_one_mirror_session(
              shdl, dev_info, i, log_pipe);
          if (PIPE_SUCCESS != sts) return sts;
        }
      }
    }
  }
  return PIPE_SUCCESS;
}

void pipe_mgr_mirror_buf_cleanup(bf_dev_id_t dev_id) {
  if (mirror_session_cache[dev_id].ready) {
    PIPE_MGR_LOCK_DESTROY(&mirror_session_cache[dev_id].mirror_lock);
    bf_map_t *map = &mirror_session_cache[dev_id].mir_sess;
    unsigned long key;
    void *data;
    while (BF_MAP_OK == bf_map_get_first_rmv(map, &key, &data)) {
      pipe_mgr_free_mirr_info_node((mirror_info_node_t *)data);
    }
    bf_map_destroy(&mirror_session_cache[dev_id].mir_sess);
    mirror_session_cache[dev_id].ready = false;
  }
}

static void pipe_mgr_session_info_translate(
    bf_dev_id_t dev_id,
    pipe_mgr_mirror_session_info_t *internal_info,
    bf_mirror_session_info_t *bf_info) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return;
  if ((internal_info == NULL) || (bf_info == NULL)) return;
  int i;
  internal_info->mirror_type = bf_info->mirror_type;
  internal_info->dir = bf_info->dir;
  internal_info->max_pkt_len = bf_info->max_pkt_len;
  /* Do not change first 4 bytes of user specified hdr on Tofino1 */
  if (dev_info->dev_family != BF_DEV_FAMILY_TOFINO) {
    internal_info->header[0] =
        ((1 << 3) | ((internal_info->dir == BF_DIR_EGRESS ? 1 : 0) << 4) |
         ((internal_info->mirror_type == BF_MIRROR_TYPE_COAL ? 1 : 0) << 5));
  } else {
    internal_info->header[0] = bf_info->header[0];
  }
  for (i = 1; i < 4; i++) {
    internal_info->header[i] = bf_info->header[i];
  }
  internal_info->header_len = bf_info->header_len;
  internal_info->timeout_usec = bf_info->timeout_usec;
  internal_info->extract_len = bf_info->extract_len;
  internal_info->extract_len_from_p4 = bf_info->extract_len_from_p4;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      internal_info->ver = bf_info->ver;
      internal_info->u.mirror_session_meta.ingress_cos = bf_info->ingress_cos;
      internal_info->u.mirror_session_meta.ucast_egress_port_v =
          bf_info->ucast_egress_port_v;
      internal_info->u.mirror_session_meta.ucast_egress_port =
          bf_info->ucast_egress_port;
      internal_info->u.mirror_session_meta.egress_port_queue =
          bf_info->egress_port_queue;
      internal_info->u.mirror_session_meta.packet_color = bf_info->packet_color;
      internal_info->u.mirror_session_meta.pipe_mask = bf_info->pipe_mask;
      internal_info->u.mirror_session_meta.level1_mcast_hash =
          bf_info->level1_mcast_hash;
      internal_info->u.mirror_session_meta.level2_mcast_hash =
          bf_info->level2_mcast_hash;
      internal_info->u.mirror_session_meta.mcast_grp_a = bf_info->mcast_grp_a;
      internal_info->u.mirror_session_meta.mcast_grp_a_v =
          bf_info->mcast_grp_a_v;
      internal_info->u.mirror_session_meta.mcast_grp_b = bf_info->mcast_grp_b;
      internal_info->u.mirror_session_meta.mcast_grp_b_v =
          bf_info->mcast_grp_b_v;
      internal_info->u.mirror_session_meta.mcast_l1_xid = bf_info->mcast_l1_xid;
      internal_info->u.mirror_session_meta.mcast_l2_xid = bf_info->mcast_l2_xid;
      internal_info->u.mirror_session_meta.mcast_rid = bf_info->mcast_rid;
      internal_info->u.mirror_session_meta.engress_bypass =
          0;  // set to default value
      internal_info->u.mirror_session_meta.icos_for_copy_to_cpu =
          bf_info->icos_for_copy_to_cpu;
      internal_info->u.mirror_session_meta.copy_to_cpu = bf_info->copy_to_cpu;
      internal_info->u.mirror_session_meta.deflect_on_drop =
          bf_info->deflect_on_drop;
      break;
    case BF_DEV_FAMILY_TOFINO2:
      internal_info->coal_mode =
          0;                   // only for tof2, set to tofino coalescing mode
      internal_info->pri = 1;  // only for tof2, set to low pri
      internal_info->u.mirror_session_entry.hash_cfg_flag = 0;
      internal_info->u.mirror_session_entry.hash_cfg_flag_p = 0;
      internal_info->u.mirror_session_entry.icos_cfg_flag = 0;
      internal_info->u.mirror_session_entry.dod_cfg_flag = 0;
      internal_info->u.mirror_session_entry.c2c_cfg_flag = 0;
      internal_info->u.mirror_session_entry.mc_cfg_flag = 0;
      internal_info->u.mirror_session_entry.epipe_cfg_flag = 0;
      internal_info->u.mirror_session_entry.mcast_l1_xid =
          bf_info->mcast_l1_xid;
      internal_info->u.mirror_session_entry.mcast_l2_xid =
          bf_info->mcast_l2_xid;
      internal_info->u.mirror_session_entry.mcid1 = bf_info->mcast_grp_a;
      internal_info->u.mirror_session_entry.mcid2 = bf_info->mcast_grp_b;
      internal_info->u.mirror_session_entry.epipe_port =
          bf_info->ucast_egress_port;
      internal_info->u.mirror_session_entry.epipe_port_v =
          bf_info->ucast_egress_port_v;
      internal_info->u.mirror_session_entry.deflect_on_drop =
          bf_info->deflect_on_drop;
      internal_info->u.mirror_session_entry.mcast_rid = bf_info->mcast_rid;
      internal_info->u.mirror_session_entry.hash1 = bf_info->level1_mcast_hash;
      internal_info->u.mirror_session_entry.hash2 = bf_info->level2_mcast_hash;
      internal_info->u.mirror_session_entry.pipe_vec = bf_info->pipe_mask;
      internal_info->u.mirror_session_entry.egr_bypass_flag = 0;
      internal_info->u.mirror_session_entry.icos = bf_info->ingress_cos;
      internal_info->u.mirror_session_entry.color = bf_info->packet_color;
      internal_info->u.mirror_session_entry.mcid1_vld = bf_info->mcast_grp_a_v;
      internal_info->u.mirror_session_entry.mcid2_vld = bf_info->mcast_grp_b_v;
      internal_info->u.mirror_session_entry.c2c_cos =
          bf_info->icos_for_copy_to_cpu;
      internal_info->u.mirror_session_entry.c2c_vld = bf_info->copy_to_cpu;
      internal_info->u.mirror_session_entry.yid_tbl_sel = 0;
      internal_info->u.mirror_session_entry.eport_qid =
          bf_info->egress_port_queue;

      break;
    case BF_DEV_FAMILY_TOFINO3:
      internal_info->coal_mode =
          0;                   // only for tof2, set to tofino coalescing mode
      internal_info->pri = 1;  // only for tof2, set to low pri
      internal_info->u.mirror_session_tof3_entry.hash_cfg_flag = 0;
      internal_info->u.mirror_session_tof3_entry.hash_cfg_flag_p = 0;
      internal_info->u.mirror_session_tof3_entry.icos_cfg_flag = 0;
      internal_info->u.mirror_session_tof3_entry.dod_cfg_flag = 0;
      internal_info->u.mirror_session_tof3_entry.c2c_cfg_flag = 0;
      internal_info->u.mirror_session_tof3_entry.mc_cfg_flag = 0;
      internal_info->u.mirror_session_tof3_entry.epipe_cfg_flag = 0;
      internal_info->u.mirror_session_tof3_entry.mcast_l1_xid =
          bf_info->mcast_l1_xid;
      internal_info->u.mirror_session_tof3_entry.mcid1 = bf_info->mcast_grp_a;
      internal_info->u.mirror_session_tof3_entry.mcid2 = bf_info->mcast_grp_b;
      internal_info->u.mirror_session_tof3_entry.epipe_port =
          bf_info->ucast_egress_port;
      internal_info->u.mirror_session_tof3_entry.epipe_port_v =
          bf_info->ucast_egress_port_v;
      internal_info->u.mirror_session_tof3_entry.deflect_on_drop =
          bf_info->deflect_on_drop;
      internal_info->u.mirror_session_tof3_entry.mcast_rid = bf_info->mcast_rid;
      internal_info->u.mirror_session_tof3_entry.hash1 =
          bf_info->level1_mcast_hash;
      internal_info->u.mirror_session_tof3_entry.hash2 =
          bf_info->level2_mcast_hash;
      internal_info->u.mirror_session_tof3_entry.tm_vec |= 0x1;
      if (bf_info->pipe_mask & 0xF0)
        internal_info->u.mirror_session_tof3_entry.tm_vec |= 0x2;
      internal_info->u.mirror_session_tof3_entry.pipe_vec = bf_info->pipe_mask;
      internal_info->u.mirror_session_tof3_entry.egr_bypass_flag = 0;
      internal_info->u.mirror_session_tof3_entry.icos = bf_info->ingress_cos;
      internal_info->u.mirror_session_tof3_entry.color = bf_info->packet_color;
      internal_info->u.mirror_session_tof3_entry.mcid1_vld =
          bf_info->mcast_grp_a_v;
      internal_info->u.mirror_session_tof3_entry.mcid2_vld =
          bf_info->mcast_grp_b_v;
      internal_info->u.mirror_session_tof3_entry.c2c_cos =
          bf_info->icos_for_copy_to_cpu;
      internal_info->u.mirror_session_tof3_entry.c2c_vld = bf_info->copy_to_cpu;
      internal_info->u.mirror_session_tof3_entry.yid_tbl_sel = 0;
      internal_info->u.mirror_session_tof3_entry.eport_qid =
          bf_info->egress_port_queue;
      internal_info->u.mirror_session_tof3_entry.mcast_l2_xid =
          bf_info->mcast_l2_xid;
      break;

    case BF_DEV_FAMILY_UNKNOWN:
      PIPE_MGR_DBGCHK(0);
      break;
  }
  return;
}
static mirror_info_node_t *pipe_mgr_create_mirr_info_node(
    bf_dev_id_t device_id,
    bf_dev_pipe_t pipe_id,
    uint16_t sid,
    bf_mirror_session_info_t *session_info,
    bool enable_ing,
    bool enable_egr) {
  mirror_info_node_t *p = PIPE_MGR_CALLOC(1, sizeof *p);
  if (!p) return NULL;
  p->pipe_id = pipe_id;
  p->sid = sid;
  p->enable_ing = enable_ing;
  p->enable_egr = enable_egr;
  if (session_info != NULL) {
    pipe_mgr_session_info_translate(
        device_id, &(p->session_info), session_info);
  } else {
    PIPE_MGR_DBGCHK(0);
  }
  return p;
}
void pipe_mgr_free_mirr_info_node(mirror_info_node_t *node) {
  PIPE_MGR_FREE(node);
}

pipe_status_t pipe_mgr_mirror_buf_session_set_hw(
    pipe_sess_hdl_t sess_hdl,
    rmt_dev_info_t *dev_info,
    mirror_info_node_t *mirr_node) {
  bf_dev_id_t dev_id = dev_info->dev_id;
  pipe_bitmap_t pbm;
  pipe_status_t sts = mirror_buf_set_pbm(dev_info, mirr_node->pipe_id, &pbm);
  if (PIPE_SUCCESS != sts) {
    LOG_ERROR("Device %d, mirror session id 0x%x has invalid logical pipe %x",
              dev_id,
              mirr_node->sid,
              mirr_node->pipe_id);
    return PIPE_INVALID_ARG;
  }
  pipe_mgr_mirror_session_info_t *session_info = &mirr_node->session_info;
  /* First program the coalescing parameters if this is a coalescing session. */
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      if (session_info->mirror_type == BF_MIRROR_TYPE_COAL) {
        if (!pipe_mgr_tof_mirror_buf_sid_is_coalescing(mirr_node->sid)) {
          LOG_ERROR(
              "Device %d, mirror session id 0x%x does not support coalescing",
              dev_id,
              mirr_node->sid);
          return PIPE_INVALID_ARG;
        }
        sts =
            pipe_mgr_tof_mirror_buf_coal_session_update(sess_hdl,
                                                        dev_id,
                                                        pbm,
                                                        mirr_node->sid,
                                                        session_info,
                                                        mirr_node->enable_egr);
        if (sts != PIPE_SUCCESS) {
          return sts;
        }
      }
      sts = pipe_mgr_tof_mirror_buf_norm_session_update(sess_hdl,
                                                        dev_id,
                                                        pbm,
                                                        mirr_node->sid,
                                                        session_info,
                                                        mirr_node->enable_ing,
                                                        mirr_node->enable_egr);
      if (sts != PIPE_SUCCESS) {
        return sts;
      }
      break;
    case BF_DEV_FAMILY_TOFINO2:
      if (session_info->mirror_type == BF_MIRROR_TYPE_COAL) {
        sts = pipe_mgr_tof2_mirror_buf_coal_session_update(
            sess_hdl,
            dev_id,
            mirr_node->pipe_id,
            pbm,
            mirr_node->sid,
            session_info,
            (mirr_node->enable_egr || mirr_node->enable_ing));
      } else {
        sts =
            pipe_mgr_tof2_mirror_buf_norm_session_update(sess_hdl,
                                                         dev_id,
                                                         mirr_node->pipe_id,
                                                         pbm,
                                                         mirr_node->sid,
                                                         session_info,
                                                         mirr_node->enable_ing,
                                                         mirr_node->enable_egr);
      }
      break;
    case BF_DEV_FAMILY_TOFINO3:
      if (session_info->mirror_type == BF_MIRROR_TYPE_COAL) {
        sts = pipe_mgr_tof3_mirror_buf_coal_session_update(
            sess_hdl,
            dev_id,
            mirr_node->pipe_id,
            pbm,
            mirr_node->sid,
            session_info,
            (mirr_node->enable_egr || mirr_node->enable_ing));
      } else {
        sts =
            pipe_mgr_tof3_mirror_buf_norm_session_update(sess_hdl,
                                                         dev_id,
                                                         mirr_node->pipe_id,
                                                         pbm,
                                                         mirr_node->sid,
                                                         session_info,
                                                         mirr_node->enable_ing,
                                                         mirr_node->enable_egr);
      }
      break;

    case BF_DEV_FAMILY_UNKNOWN:
      PIPE_MGR_DBGCHK(0);
      sts = PIPE_UNEXPECTED;
      break;
  }
  return sts;
}
static pipe_status_t pipe_mgr_mirror_buf_mirror_session_set_hw(
    pipe_sess_hdl_t sess_hdl,
    rmt_dev_info_t *dev_info,
    mirror_info_node_t *mirr_node) {
  bf_dev_id_t dev_id = dev_info->dev_id;
  if (pipe_mgr_is_device_locked(dev_id)) return PIPE_SUCCESS;
  if (pipe_mgr_hitless_warm_init_in_progress(dev_id)) return PIPE_SUCCESS;
  return pipe_mgr_mirror_buf_session_set_hw(sess_hdl, dev_info, mirr_node);
}

// API to correct ECC
pipe_status_t pipe_mgr_mirror_ecc_correct(pipe_sess_hdl_t sess_hdl,
                                          bf_dev_id_t device_id,
                                          bf_dev_pipe_t phy_pipe_id,
                                          uint16_t sid) {
  mirror_info_node_t *p;
  pipe_status_t sts = PIPE_SUCCESS;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(device_id);
  if (!dev_info) return PIPE_INVALID_ARG;
  bool dummy = false;

  /* Lookup the session which had the error.  First check if the session is in
   * our SW DB for all-pipes.  If not, convert the physical pipe to a logical
   * pipe and check if it is in the DB for the logical pipe.  If not, use a
   * dummy empty session to correct the error. */
  p = pipe_mgr_mirror_buf_get_session_by_pipe(device_id, sid, BF_DEV_PIPE_ALL);
  if (!p) {
    bf_dev_pipe_t log_pipe = phy_pipe_id;
    pipe_mgr_map_phy_pipe_id_to_log_pipe_id_optimized(
        dev_info, phy_pipe_id, &log_pipe);
    p = pipe_mgr_mirror_buf_get_session_by_pipe(device_id, sid, log_pipe);

    if (!p) {
      bf_mirror_session_info_t info;
      PIPE_MGR_MEMSET(&info, 0, sizeof info);
      p = pipe_mgr_create_mirr_info_node(
          device_id, log_pipe, sid, &info, false, false);
      dummy = true;
    }
  }
  sts = pipe_mgr_mirror_buf_mirror_session_set_hw(sess_hdl, dev_info, p);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR(
        "Failed to correct ECC error on dev %d phy-pipe %d mirror session "
        "0x%x, "
        "status %s",
        device_id,
        phy_pipe_id,
        sid,
        pipe_str_err(sts));
  }
  if (dummy) pipe_mgr_free_mirr_info_node(p);
  return sts;
}

/* Validate the params passed */
pipe_status_t pipe_mgr_mirror_buf_mirror_session_validate(
    dev_target_t dev_target,
    uint16_t sid,
    bf_mirror_session_info_t *session_info) {
  char *errstr = NULL;
  uint32_t n_pipes = pipe_mgr_get_num_active_pipes(dev_target.device_id);
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_target.device_id);
  if (!dev_info) return PIPE_INVALID_ARG;

  /* Validate the session_info. */
  if (!session_info) return PIPE_INVALID_ARG;

  /* Validate the YID. */
  if (session_info->mcast_l2_xid >= BF_PORT_COUNT) {
    errstr = "XID";
    goto ERROR;
  }

  /* Validate the port number. */
  if (session_info->ucast_egress_port_v) {
    if (!dev_info->dev_cfg.dev_port_validate(session_info->ucast_egress_port)) {
      errstr = "egress port";
      goto ERROR;
    }
    bf_dev_pipe_t p =
        dev_info->dev_cfg.dev_port_to_pipe(session_info->ucast_egress_port);
    if (p >= n_pipes) {
      errstr = "egress port Pipe";
      goto ERROR;
    }
  }

  if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO) {
    /* Validate COS. COS value for all TOFINOs 3 bits.*/
    if (session_info->ingress_cos > TOF_MIRROR_ICOS) {
      errstr = "ingress COS";
      goto ERROR;
    }

    /* Validate C2C COS.*/
    if (session_info->icos_for_copy_to_cpu > TOF_MIRROR_ICOS_C2C) {
      errstr = "Ingress C2C COS";
      goto ERROR;
    }

    /* Validate Queues, TF1-5bits */
    if (session_info->egress_port_queue > TOF_MIRROR_EGR_PORT_Q) {
      errstr = "Egress Port Queue";
      goto ERROR;
    }
  } else if ((dev_info->dev_family == BF_DEV_FAMILY_TOFINO2) ||
             (dev_info->dev_family == BF_DEV_FAMILY_TOFINO3)) {
    /* Validate COS. COS value for all TOFINOs 3 bits.*/
    if (session_info->ingress_cos > TOF2_MIRROR_ICOS) {
      errstr = "ingress COS";
      goto ERROR;
    }

    /* Validate C2C COS. TF2 and TF3 icos-c2c values are same */
    if (session_info->icos_for_copy_to_cpu > TOF2_MIRROR_ICOS_C2C) {
      errstr = "ingress C2C COS";
      goto ERROR;
    }

    /* Validate Queues, TF2, TF3-7 bits  */
    if (session_info->egress_port_queue > TOF2_MIRROR_EGR_PORT_Q) {
      errstr = "Egress Port Queue";
      goto ERROR;
    }
  }
  return PIPE_SUCCESS;

ERROR:
  LOG_ERROR("Dev: %d Mirror session:%d failed, invalid %s",
            dev_target.device_id,
            sid,
            errstr);
  return PIPE_INVALID_ARG;
}

pipe_status_t pipe_mgr_mirror_buf_mirror_session_set(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_target,
    uint16_t sid,
    bf_mirror_session_info_t *session_info,
    bool enable) {
  pipe_status_t sts = PIPE_SUCCESS;
  bool enable_ing = false, enable_egr = false;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_target.device_id);
  if (!dev_info) return PIPE_INVALID_ARG;

  /* Validate the session_info. */
  if (!session_info) return PIPE_INVALID_ARG;

  /* validate the passed paramaters */
  sts = pipe_mgr_mirror_buf_mirror_session_validate(
      dev_target, sid, session_info);
  if (sts != PIPE_SUCCESS) {
    return sts;
  }

  /* Create a pipe bitmap based on the dev_target information. */
  pipe_bitmap_t pipes;
  sts = mirror_buf_set_pbm(dev_info, dev_target.dev_pipe_id, &pipes);
  if (PIPE_SUCCESS != sts) return PIPE_INVALID_ARG;

  /* Determine the per-direction enable flags based on the direction of the
   * session and the enable flag passed in. */
  if (enable) {
    if (session_info->dir == BF_DIR_INGRESS) {
      enable_ing = true;
      enable_egr = false;
    } else if (session_info->dir == BF_DIR_EGRESS) {
      enable_ing = false;
      enable_egr = true;
    } else if (session_info->dir == BF_DIR_BOTH) {
      enable_ing = true;
      enable_egr = true;
    } else {
      LOG_ERROR("Dev: %d Mirror session %d enabled w/o any direction",
                dev_target.device_id,
                sid);
      return PIPE_INVALID_ARG;
    }
  }

  /* If this session is for all-pipes then any existing session must also be for
   * all-pipes.  If this session is for a specific pipe then there cannot be a
   * session for all-pipes. */
  if (dev_target.dev_pipe_id == BF_DEV_PIPE_ALL) {
    /* The session is for all-pipes, ensure there are no existing sessions for a
     * specific pipe. */
    for (unsigned int log_pipe = 0; log_pipe < dev_info->num_active_pipes;
         ++log_pipe) {
      if (pipe_mgr_mirror_buf_get_session_by_pipe(
              dev_target.device_id, sid, log_pipe)) {
        LOG_ERROR(
            "Cannot create dev %d mirror session %d on pipe %x, it already "
            "exists on pipe %x",
            dev_target.device_id,
            sid,
            dev_target.dev_pipe_id,
            log_pipe);
        return PIPE_INVALID_ARG;
      }
    }
  } else {
    if (pipe_mgr_mirror_buf_get_session_by_pipe(
            dev_target.device_id, sid, BF_DEV_PIPE_ALL)) {
      LOG_ERROR(
          "Cannot create dev %d mirror session %d on pipe %x, it already "
          "exists on pipe %x",
          dev_target.device_id,
          sid,
          dev_target.dev_pipe_id,
          BF_DEV_PIPE_ALL);
      return PIPE_INVALID_ARG;
    }
  }

  mirror_info_node_t *new_node =
      pipe_mgr_create_mirr_info_node(dev_target.device_id,
                                     dev_target.dev_pipe_id,
                                     sid,
                                     session_info,
                                     enable_ing,
                                     enable_egr);
  if (!new_node) {
    sts = PIPE_NO_SYS_RESOURCES;
    goto failure_cleanup;
  }

  /* Post instructions to an ilist to program the node in hardware. */
  sts = pipe_mgr_mirror_buf_mirror_session_set_hw(sess_hdl, dev_info, new_node);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR(
        "Failed to write mirror session %d config to HW, pipe %x, dev %d, "
        "status %s",
        new_node->sid,
        new_node->pipe_id,
        dev_target.device_id,
        pipe_str_err(sts));
    goto failure_cleanup;
  }

  /* Save the session-info nodes. */
  save_session_info(dev_info, new_node);
  return sts;

failure_cleanup:
  if (new_node) pipe_mgr_free_mirr_info_node(new_node);
  return sts;
}

pipe_status_t pipe_mgr_mirr_sess_en_or_dis(pipe_sess_hdl_t sess_hdl,
                                           dev_target_t dev_target,
                                           bf_mirror_direction_e dir,
                                           uint16_t sid,
                                           bool enable) {
  LOG_TRACE("%s dev %d pipe %x sid %d en %d dir %s",
            __func__,
            dev_target.device_id,
            dev_target.dev_pipe_id,
            sid,
            enable,
            dir == BF_DIR_INGRESS
                ? "I"
                : dir == BF_DIR_EGRESS ? "E" : dir == BF_DIR_BOTH ? "IE" : "?");
  bf_dev_id_t dev_id = dev_target.device_id;
  pipe_status_t sts = PIPE_SUCCESS;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return PIPE_INVALID_ARG;

  /* Validate the direction. */
  if (dir != BF_DIR_INGRESS && dir != BF_DIR_EGRESS && dir != BF_DIR_BOTH) {
    return PIPE_INVALID_ARG;
  }

  /* Look up the configuration for the session. */
  mirror_info_node_t *n = pipe_mgr_mirror_buf_get_session_by_pipe(
      dev_id, sid, dev_target.dev_pipe_id);
  if (!n) {
    LOG_ERROR("Mirror session %d on dev %d pipe %x not found",
              sid,
              dev_id,
              dev_target.dev_pipe_id);
    return PIPE_INVALID_ARG;
  }

  /* Save the original enable values incase of a failure. */
  bool orig_ing = n->enable_ing;
  bool orig_egr = n->enable_egr;
  bf_mirror_direction_e orig_dir = n->session_info.dir;

  /* Update the session state with the new enable/disable config. */
  if (dir == BF_DIR_INGRESS) {
    n->enable_ing = enable;
  } else if (dir == BF_DIR_EGRESS) {
    n->enable_egr = enable;
  } else {
    n->enable_ing = enable;
    n->enable_egr = enable;
  }
  if (enable && (n->session_info.dir != dir)) {
    n->session_info.dir = dir;
  }
  /* Apply the config to hardware. */
  sts = pipe_mgr_mirror_buf_mirror_session_set_hw(sess_hdl, dev_info, n);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR(
        "Failed to write mirror session %d config to HW pipe %x dev %d status "
        "%s",
        n->sid,
        n->pipe_id,
        dev_id,
        pipe_str_err(sts));
    n->enable_ing = orig_ing;
    n->enable_egr = orig_egr;
    n->session_info.dir = orig_dir;
    return sts;
  }

  return sts;
}

pipe_status_t pipe_mgr_mirror_buf_mirror_session_reset(pipe_sess_hdl_t sess_hdl,
                                                       dev_target_t dev_target,
                                                       uint16_t sid) {
  bf_dev_id_t dev_id = dev_target.device_id;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return PIPE_INVALID_ARG;
  /* Disable the session. */
  pipe_status_t sts = pipe_mgr_mirr_sess_en_or_dis(
      sess_hdl, dev_target, BF_DIR_BOTH, sid, false);
  if (PIPE_SUCCESS != sts) return sts;

  /* Remove the SW state for the session. */
  remove_session_by_pipe(dev_id, sid, dev_target.dev_pipe_id);

  /* Check whether tof2 coal, if yes, remove from coal mirror id list */
  if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO2) {
    pipe_mgr_tof2_session_reset(
        sess_hdl, dev_info, sid, dev_target.dev_pipe_id);
  } else if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO3) {
    pipe_mgr_tof3_session_reset(
        sess_hdl, dev_info, sid, dev_target.dev_pipe_id);
  }
  return PIPE_SUCCESS;
}

bf_mirror_type_e pipe_mgr_mirror_buf_mirror_type_get(dev_target_t dev_target,
                                                     bf_mirror_id_t sid) {
  bf_dev_id_t dev_id = dev_target.device_id;
  mirror_info_node_t *p = pipe_mgr_mirror_buf_get_session_by_pipe(
      dev_id, sid, dev_target.dev_pipe_id);
  return p ? p->session_info.mirror_type : BF_MIRROR_TYPE_MAX;
}
pipe_status_t pipe_mgr_mirr_sess_pipe_vector_set(pipe_sess_hdl_t sess_hdl,
                                                 dev_target_t dev_target,
                                                 bf_mirror_id_t sid,
                                                 int logical_pipe_vector) {
  pipe_status_t sts = PIPE_SUCCESS;
  bf_dev_id_t dev_id = dev_target.device_id;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return PIPE_INVALID_ARG;

  /* Look up the configuration for the session. */
  mirror_info_node_t *p = pipe_mgr_mirror_buf_get_session_by_pipe(
      dev_id, sid, dev_target.dev_pipe_id);
  if (!p) {
    LOG_ERROR("Cannot find dev %d pipe %x mirror session %d",
              dev_target.device_id,
              dev_target.dev_pipe_id,
              sid);
    return PIPE_INVALID_ARG;
  }

  /* Apply the config to hardware. */
  int old_value;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      /* Update the session state with the new pipe vector config. */
      old_value = p->session_info.u.mirror_session_meta.pipe_mask;
      p->session_info.u.mirror_session_meta.pipe_mask = logical_pipe_vector;
      sts = pipe_mgr_mirror_buf_mirror_session_set_hw(sess_hdl, dev_info, p);
      if (PIPE_SUCCESS != sts)
        p->session_info.u.mirror_session_meta.pipe_mask = old_value;
      break;
    case BF_DEV_FAMILY_TOFINO2:
      /* Update the session state with the new pipe vector config. */
      old_value = p->session_info.u.mirror_session_entry.pipe_vec;
      p->session_info.u.mirror_session_entry.pipe_vec =
          logical_pipe_vector | 0x10;
      sts = pipe_mgr_mirror_buf_mirror_session_set_hw(sess_hdl, dev_info, p);
      if (PIPE_SUCCESS != sts)
        p->session_info.u.mirror_session_entry.pipe_vec = old_value;
      break;
    case BF_DEV_FAMILY_TOFINO3:
      /* Update the session state with the new pipe vector config. */
      old_value = p->session_info.u.mirror_session_tof3_entry.pipe_vec;
      int old_tm_vec = p->session_info.u.mirror_session_tof3_entry.tm_vec;
      p->session_info.u.mirror_session_tof3_entry.tm_vec |= 0x1;
      if (logical_pipe_vector & 0xF0)
        p->session_info.u.mirror_session_tof3_entry.tm_vec |= 0x2;
      p->session_info.u.mirror_session_tof3_entry.pipe_vec =
          logical_pipe_vector;
      sts = pipe_mgr_mirror_buf_mirror_session_set_hw(sess_hdl, dev_info, p);
      if (PIPE_SUCCESS != sts) {
        p->session_info.u.mirror_session_tof3_entry.pipe_vec = old_value;
        p->session_info.u.mirror_session_tof3_entry.tm_vec = old_tm_vec;
      }
      break;

    case BF_DEV_FAMILY_UNKNOWN:
      PIPE_MGR_DBGCHK(0);
      sts = PIPE_UNEXPECTED;
      break;
  }
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR(
        "Failed to write mirror session %d config to HW, pipe %x, dev %d, "
        "status %s",
        p->sid,
        p->pipe_id,
        dev_id,
        pipe_str_err(sts));
    return sts;
  }

  return sts;
}

pipe_status_t pipe_mgr_mirr_sess_pipe_vector_get(pipe_sess_hdl_t sess_hdl,
                                                 dev_target_t dev_target,
                                                 bf_mirror_id_t sid,
                                                 int *logical_pipe_vector) {
  (void)sess_hdl;
  if (!logical_pipe_vector) return PIPE_INVALID_ARG;
  bf_dev_id_t dev_id = dev_target.device_id;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return PIPE_INVALID_ARG;

  /* Look up the configuration for the session. */
  mirror_info_node_t *p = pipe_mgr_mirror_buf_get_session_by_pipe(
      dev_id, sid, dev_target.dev_pipe_id);
  if (!p) {
    LOG_ERROR("Cannot find dev %d pipe %x mirror session %d",
              dev_target.device_id,
              dev_target.dev_pipe_id,
              sid);
    return PIPE_INVALID_ARG;
  }

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      *logical_pipe_vector |= p->session_info.u.mirror_session_meta.pipe_mask;
      break;
    case BF_DEV_FAMILY_TOFINO2:
      *logical_pipe_vector |=
          p->session_info.u.mirror_session_entry.pipe_vec & 0xF;
      break;
    case BF_DEV_FAMILY_TOFINO3:
      *logical_pipe_vector |=
          p->session_info.u.mirror_session_tof3_entry.pipe_vec & 0xFF;
      break;

    case BF_DEV_FAMILY_UNKNOWN:
      PIPE_MGR_DBGCHK(0);
      return PIPE_INVALID_ARG;
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_get_max_mirror_sessions(bf_dev_id_t device_id,
                                               bf_mirror_type_e mirror_type,
                                               bf_mirror_id_t *sid) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(device_id);
  if (!dev_info) return PIPE_INVALID_ARG;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      if (mirror_type == BF_MIRROR_TYPE_NORM)
        *sid = PIPE_MGR_TOF_MIRROR_SESSION_MAX -
               PIPE_MGR_TOF_MIRROR_COAL_SESSION_MAX - 1;
      else if (mirror_type == BF_MIRROR_TYPE_COAL)
        *sid = PIPE_MGR_TOF_MIRROR_COAL_SESSION_MAX +
               PIPE_MGR_TOF_MIRROR_COAL_BASE_SID - 1;
      else
        return PIPE_INVALID_ARG;
      break;
    case BF_DEV_FAMILY_TOFINO2:
      if (mirror_type == BF_MIRROR_TYPE_NORM)
        *sid = PIPE_MGR_TOF2_MIRROR_SESSION_MAX - 1;
      else if (mirror_type == BF_MIRROR_TYPE_COAL)
        *sid = PIPE_MGR_TOF2_MIRROR_SESSION_MAX - 1;
      else
        return PIPE_INVALID_ARG;
      break;
    case BF_DEV_FAMILY_TOFINO3:
      if (mirror_type == BF_MIRROR_TYPE_NORM)
        *sid = PIPE_MGR_TOF3_MIRROR_SESSION_MAX - 1;
      else if (mirror_type == BF_MIRROR_TYPE_COAL)
        *sid = PIPE_MGR_TOF3_MIRROR_SESSION_MAX - 1;
      else
        return PIPE_INVALID_ARG;
      break;






    case BF_DEV_FAMILY_UNKNOWN:
      return PIPE_INVALID_ARG;
  }
  return PIPE_SUCCESS;
}
pipe_status_t pipe_mgr_get_base_mirror_session_id(bf_dev_id_t device_id,
                                                  bf_mirror_type_e mirror_type,
                                                  bf_mirror_id_t *sid) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(device_id);
  if (!dev_info) return PIPE_INVALID_ARG;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      if (mirror_type == BF_MIRROR_TYPE_NORM)
        *sid = 1;
      else if (mirror_type == BF_MIRROR_TYPE_COAL)
        *sid = PIPE_MGR_TOF_MIRROR_COAL_BASE_SID;
      else
        return PIPE_INVALID_ARG;
      break;
    case BF_DEV_FAMILY_TOFINO2:
    case BF_DEV_FAMILY_TOFINO3:
      if (mirror_type == BF_MIRROR_TYPE_NORM)
        *sid = 1;
      else if (mirror_type == BF_MIRROR_TYPE_COAL)
        *sid = 1;
      else
        return PIPE_INVALID_ARG;
      break;

    case BF_DEV_FAMILY_UNKNOWN:
      return PIPE_INVALID_ARG;
  }
  return PIPE_SUCCESS;
}
static pipe_status_t pipe_mgr_mirror_session_info_flags_update(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_target,
    bf_mirror_id_t sid,
    bf_mirror_meta_flag_e mirror_flag,
    bool value) {
  bool error = false;
  pipe_status_t sts = PIPE_SUCCESS;
  bf_dev_id_t dev_id = dev_target.device_id;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return PIPE_INVALID_ARG;

  /* Look up the configuration for the session. */
  mirror_info_node_t *p = pipe_mgr_mirror_buf_get_session_by_pipe(
      dev_id, sid, dev_target.dev_pipe_id);
  if (!p) {
    LOG_ERROR("Cannot find dev %d pipe %x mirror session %d",
              dev_target.device_id,
              dev_target.dev_pipe_id,
              sid);
    return PIPE_INVALID_ARG;
  }

  pipe_mgr_mirror_tof2_session_entry_t *tf2_entry;
  pipe_mgr_mirror_tof3_session_entry_t *tf3_entry;
  pipe_mgr_mirror_tof2_session_entry_t tf2_old_vals;
  pipe_mgr_mirror_tof3_session_entry_t tf3_old_vals;

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO2:
      tf2_entry = &p->session_info.u.mirror_session_entry;
      tf2_old_vals = *tf2_entry;
      if (mirror_flag == BF_HASH_CFG) {
        tf2_entry->hash_cfg_flag = value;
      } else if (mirror_flag == BF_HASH_CFG_P) {
        tf2_entry->hash_cfg_flag_p = value;
      } else if (mirror_flag == BF_ICOS_CFG) {
        tf2_entry->icos_cfg_flag = value;
      } else if (mirror_flag == BF_DOD_CFG) {
        tf2_entry->dod_cfg_flag = value;
      } else if (mirror_flag == BF_C2C_CFG) {
        tf2_entry->c2c_cfg_flag = value;
      } else if (mirror_flag == BF_MC_CFG) {
        tf2_entry->mc_cfg_flag = value;
      } else if (mirror_flag == BF_EPIPE_CFG) {
        tf2_entry->epipe_cfg_flag = value;
      } else {
        error = true;
      }
      break;
    case BF_DEV_FAMILY_TOFINO3:
      tf3_entry = &p->session_info.u.mirror_session_tof3_entry;
      tf3_old_vals = *tf3_entry;
      if (mirror_flag == BF_HASH_CFG) {
        tf3_entry->hash_cfg_flag = value;
      } else if (mirror_flag == BF_HASH_CFG_P) {
        tf3_entry->hash_cfg_flag_p = value;
      } else if (mirror_flag == BF_ICOS_CFG) {
        tf3_entry->icos_cfg_flag = value;
      } else if (mirror_flag == BF_DOD_CFG) {
        tf3_entry->dod_cfg_flag = value;
      } else if (mirror_flag == BF_C2C_CFG) {
        tf3_entry->c2c_cfg_flag = value;
      } else if (mirror_flag == BF_MC_CFG) {
        tf3_entry->mc_cfg_flag = value;
      } else if (mirror_flag == BF_EPIPE_CFG) {
        tf3_entry->epipe_cfg_flag = value;
      } else {
        error = true;
      }
      break;
    default:
      return PIPE_INVALID_ARG;
  }

  if (error) {
    LOG_ERROR("%s: dev %d pipe %x sess %d unexpected flag %d value %d",
              __func__,
              dev_target.device_id,
              dev_target.dev_pipe_id,
              sid,
              mirror_flag,
              value);
    return PIPE_INVALID_ARG;
  }

  sts = pipe_mgr_mirror_buf_mirror_session_set_hw(sess_hdl, dev_info, p);
  if (PIPE_SUCCESS != sts) {
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO2:
        p->session_info.u.mirror_session_entry = tf2_old_vals;
        break;
      case BF_DEV_FAMILY_TOFINO3:
        p->session_info.u.mirror_session_tof3_entry = tf3_old_vals;
        break;
      default:
        break;
    }
  }
  return sts;
}
static pipe_status_t pipe_mgr_mirror_session_info_flags_get(
    dev_target_t dev_target,
    bf_mirror_id_t sid,
    bf_mirror_meta_flag_e mirror_flag,
    bool *value) {
  bf_dev_id_t dev_id = dev_target.device_id;
  bool error = false;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return PIPE_INVALID_ARG;
  if (!value) return PIPE_INVALID_ARG;

  /* Look up the configuration for the session. */
  mirror_info_node_t *p = pipe_mgr_mirror_buf_get_session_by_pipe(
      dev_id, sid, dev_target.dev_pipe_id);
  if (!p) {
    LOG_ERROR("Cannot find dev %d pipe %x mirror session %d",
              dev_target.device_id,
              dev_target.dev_pipe_id,
              sid);
    return PIPE_INVALID_ARG;
  }

  pipe_mgr_mirror_tof2_session_entry_t *tf2_entry;
  pipe_mgr_mirror_tof3_session_entry_t *tf3_entry;

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO2:
      tf2_entry = &p->session_info.u.mirror_session_entry;
      if (mirror_flag == BF_HASH_CFG) {
        *value = tf2_entry->hash_cfg_flag;
      } else if (mirror_flag == BF_HASH_CFG_P) {
        *value = tf2_entry->hash_cfg_flag_p;
      } else if (mirror_flag == BF_ICOS_CFG) {
        *value = tf2_entry->icos_cfg_flag;
      } else if (mirror_flag == BF_DOD_CFG) {
        *value = tf2_entry->dod_cfg_flag;
      } else if (mirror_flag == BF_C2C_CFG) {
        *value = tf2_entry->c2c_cfg_flag;
      } else if (mirror_flag == BF_MC_CFG) {
        *value = tf2_entry->mc_cfg_flag;
      } else if (mirror_flag == BF_EPIPE_CFG) {
        *value = tf2_entry->epipe_cfg_flag;
      } else {
        error = true;
      }
      break;
    case BF_DEV_FAMILY_TOFINO3:
      tf3_entry = &p->session_info.u.mirror_session_tof3_entry;
      if (mirror_flag == BF_HASH_CFG) {
        *value = tf3_entry->hash_cfg_flag;
      } else if (mirror_flag == BF_HASH_CFG_P) {
        *value = tf3_entry->hash_cfg_flag_p;
      } else if (mirror_flag == BF_ICOS_CFG) {
        *value = tf3_entry->icos_cfg_flag;
      } else if (mirror_flag == BF_DOD_CFG) {
        *value = tf3_entry->dod_cfg_flag;
      } else if (mirror_flag == BF_C2C_CFG) {
        *value = tf3_entry->c2c_cfg_flag;
      } else if (mirror_flag == BF_MC_CFG) {
        *value = tf3_entry->mc_cfg_flag;
      } else if (mirror_flag == BF_EPIPE_CFG) {
        *value = tf3_entry->epipe_cfg_flag;
      } else {
        error = true;
      }
      break;
    default:
      return PIPE_INVALID_ARG;
  }

  if (error) {
    LOG_ERROR("%s: dev %d pipe %x sess %d unexpected flag %d",
              __func__,
              dev_target.device_id,
              dev_target.dev_pipe_id,
              sid,
              mirror_flag);
    return PIPE_INVALID_ARG;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_mirror_session_meta_flag_update(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_target,
    bf_mirror_id_t sid,
    bf_mirror_meta_flag_e mirror_flag,
    bool value) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_target.device_id);
  if (!dev_info) return PIPE_INVALID_ARG;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO2:
    case BF_DEV_FAMILY_TOFINO3:
      return pipe_mgr_mirror_session_info_flags_update(
          sess_hdl, dev_target, sid, mirror_flag, value);
    default:
      return PIPE_INVALID_ARG;
  }
}
pipe_status_t pipe_mgr_mirror_session_meta_flag_get(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_target,
    bf_mirror_id_t sid,
    bf_mirror_meta_flag_e mirror_flag,
    bool *value) {
  (void)sess_hdl;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_target.device_id);
  if (!dev_info) return PIPE_INVALID_ARG;
  if (!value) return PIPE_INVALID_ARG;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO2:
    case BF_DEV_FAMILY_TOFINO3:
      return pipe_mgr_mirror_session_info_flags_get(
          dev_target, sid, mirror_flag, value);
    default:
      return PIPE_INVALID_ARG;
  }
}

static pipe_status_t pipe_mgr_mirror_session_info_pri_update(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_target,
    bf_mirror_id_t sid,
    bool value) {
  pipe_status_t sts = PIPE_SUCCESS;
  bf_dev_id_t dev_id = dev_target.device_id;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return PIPE_INVALID_ARG;

  /* Look up the configuration for the session. */
  mirror_info_node_t *p = pipe_mgr_mirror_buf_get_session_by_pipe(
      dev_id, sid, dev_target.dev_pipe_id);
  if (!p) {
    LOG_ERROR("Cannot find dev %d pipe %x mirror session %d",
              dev_target.device_id,
              dev_target.dev_pipe_id,
              sid);
    return PIPE_INVALID_ARG;
  }

  if (p->session_info.pri != value) {
    bool old_value = p->session_info.pri;
    p->session_info.pri = value;
    sts = pipe_mgr_mirror_buf_mirror_session_set_hw(sess_hdl, dev_info, p);
    if (PIPE_SUCCESS != sts) p->session_info.pri = old_value;
  }

  return sts;
}
pipe_status_t pipe_mgr_mirror_session_pri_update(pipe_sess_hdl_t sess_hdl,
                                                 dev_target_t dev_target,
                                                 bf_mirror_id_t sid,
                                                 bool value) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_target.device_id);
  if (!dev_info) return PIPE_INVALID_ARG;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO2:
    case BF_DEV_FAMILY_TOFINO3:
      return pipe_mgr_mirror_session_info_pri_update(
          sess_hdl, dev_target, sid, value);
    default:
      return PIPE_INVALID_ARG;
  }
}

static pipe_status_t pipe_mgr_mirror_session_info_pri_get(
    dev_target_t dev_target, bf_mirror_id_t sid, bool *value) {
  bf_dev_id_t dev_id = dev_target.device_id;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return PIPE_INVALID_ARG;
  if (!value) return PIPE_INVALID_ARG;

  /* Look up the configuration for the session. */
  mirror_info_node_t *p = pipe_mgr_mirror_buf_get_session_by_pipe(
      dev_id, sid, dev_target.dev_pipe_id);
  if (!p) {
    LOG_ERROR("Cannot find dev %d pipe %x mirror session %d",
              dev_target.device_id,
              dev_target.dev_pipe_id,
              sid);
    return PIPE_INVALID_ARG;
  }

  *value = p->session_info.pri;
  return PIPE_SUCCESS;
}
pipe_status_t pipe_mgr_mirror_session_pri_get(pipe_sess_hdl_t sess_hdl,
                                              dev_target_t dev_target,
                                              bf_mirror_id_t sid,
                                              bool *value) {
  (void)sess_hdl;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_target.device_id);
  if (!dev_info) return PIPE_INVALID_ARG;
  if (!value) return PIPE_INVALID_ARG;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO2:
    case BF_DEV_FAMILY_TOFINO3:
      return pipe_mgr_mirror_session_info_pri_get(dev_target, sid, value);
    default:
      return PIPE_INVALID_ARG;
  }
}

static pipe_status_t pipe_mgr_mirror_session_info_coal_mode_update(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_target,
    bf_mirror_id_t sid,
    bool value) {
  pipe_status_t sts = PIPE_SUCCESS;
  bf_dev_id_t dev_id = dev_target.device_id;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return PIPE_INVALID_ARG;

  /* Look up the configuration for the session. */
  mirror_info_node_t *p = pipe_mgr_mirror_buf_get_session_by_pipe(
      dev_id, sid, dev_target.dev_pipe_id);
  if (!p) {
    LOG_ERROR("Cannot find dev %d pipe %x mirror session %d",
              dev_target.device_id,
              dev_target.dev_pipe_id,
              sid);
    return PIPE_INVALID_ARG;
  }

  if (p->session_info.coal_mode != value) {
    bool old_value = p->session_info.coal_mode;
    p->session_info.coal_mode = value;
    sts = pipe_mgr_mirror_buf_mirror_session_set_hw(sess_hdl, dev_info, p);
    if (PIPE_SUCCESS != sts) p->session_info.coal_mode = old_value;
  }

  return sts;
}
static pipe_status_t pipe_mgr_mirror_session_info_coal_mode_get(
    dev_target_t dev_target, bf_mirror_id_t sid, bool *value) {
  bf_dev_id_t dev_id = dev_target.device_id;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return PIPE_INVALID_ARG;
  if (!value) return PIPE_INVALID_ARG;

  /* Look up the configuration for the session. */
  mirror_info_node_t *p = pipe_mgr_mirror_buf_get_session_by_pipe(
      dev_id, sid, dev_target.dev_pipe_id);
  if (!p) {
    LOG_ERROR("Cannot find dev %d pipe %x mirror session %d",
              dev_target.device_id,
              dev_target.dev_pipe_id,
              sid);
    return PIPE_INVALID_ARG;
  }

  *value = p->session_info.coal_mode;
  return PIPE_SUCCESS;
}
pipe_status_t pipe_mgr_mirror_session_coal_mode_get(pipe_sess_hdl_t sess_hdl,
                                                    dev_target_t dev_target,
                                                    bf_mirror_id_t sid,
                                                    bool *value) {
  (void)sess_hdl;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_target.device_id);
  if (!dev_info) return PIPE_INVALID_ARG;
  if (!value) return PIPE_INVALID_ARG;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO2:
    case BF_DEV_FAMILY_TOFINO3:
      return pipe_mgr_mirror_session_info_coal_mode_get(dev_target, sid, value);
    default:
      return PIPE_INVALID_ARG;
  }
}

pipe_status_t pipe_mgr_mirror_session_coal_mode_update(pipe_sess_hdl_t sess_hdl,
                                                       dev_target_t dev_target,
                                                       bf_mirror_id_t sid,
                                                       bool value) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_target.device_id);
  if (!dev_info) return PIPE_INVALID_ARG;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO2:
    case BF_DEV_FAMILY_TOFINO3:
      return pipe_mgr_mirror_session_info_coal_mode_update(
          sess_hdl, dev_target, sid, value);
    default:
      return PIPE_INVALID_ARG;
  }
}

static pipe_status_t pipe_mgr_session_info_translate_back(
    rmt_dev_info_t *dev_info,
    bf_mirror_session_info_t *s_info,
    pipe_mgr_mirror_session_info_t *session_info) {
  int i;
  s_info->mirror_type = session_info->mirror_type;
  s_info->dir = session_info->dir;
  s_info->max_pkt_len = session_info->max_pkt_len;
  if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO) {
    /* For tofino1, the bytes programmed in asic are in the lower bytes of
       array. This fixes issues seen with header lengths 1, 2 and 3
     */
    int hdr_idx = 4 - session_info->header_len;
    for (i = 0; (i < 4) && (hdr_idx < 4); i++, hdr_idx++)
      s_info->header[i] = session_info->header[hdr_idx];
  } else {
    for (i = 0; i < 4; i++) s_info->header[i] = session_info->header[i];
  }
  s_info->header_len = session_info->header_len;
  s_info->timeout_usec = session_info->timeout_usec;
  s_info->extract_len = session_info->extract_len;
  s_info->extract_len_from_p4 = session_info->extract_len_from_p4;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      s_info->ver = session_info->ver;
      s_info->ingress_cos = session_info->u.mirror_session_meta.ingress_cos;
      s_info->ucast_egress_port_v =
          session_info->u.mirror_session_meta.ucast_egress_port_v;
      s_info->ucast_egress_port =
          session_info->u.mirror_session_meta.ucast_egress_port;
      s_info->egress_port_queue =
          session_info->u.mirror_session_meta.egress_port_queue;
      s_info->packet_color = session_info->u.mirror_session_meta.packet_color;
      s_info->pipe_mask = session_info->u.mirror_session_meta.pipe_mask;
      s_info->level1_mcast_hash =
          session_info->u.mirror_session_meta.level1_mcast_hash;
      s_info->level2_mcast_hash =
          session_info->u.mirror_session_meta.level2_mcast_hash;
      s_info->mcast_grp_a = session_info->u.mirror_session_meta.mcast_grp_a;
      s_info->mcast_grp_a_v = session_info->u.mirror_session_meta.mcast_grp_a_v;
      s_info->mcast_grp_b = session_info->u.mirror_session_meta.mcast_grp_b;
      s_info->mcast_grp_b_v = session_info->u.mirror_session_meta.mcast_grp_b_v;
      s_info->mcast_l1_xid = session_info->u.mirror_session_meta.mcast_l1_xid;
      s_info->mcast_l2_xid = session_info->u.mirror_session_meta.mcast_l2_xid;
      s_info->mcast_rid = session_info->u.mirror_session_meta.mcast_rid;
      s_info->icos_for_copy_to_cpu =
          session_info->u.mirror_session_meta.icos_for_copy_to_cpu;
      s_info->copy_to_cpu = session_info->u.mirror_session_meta.copy_to_cpu;
      s_info->deflect_on_drop =
          session_info->u.mirror_session_meta.deflect_on_drop;
      break;
    case BF_DEV_FAMILY_TOFINO2:
      s_info->mcast_grp_a = session_info->u.mirror_session_entry.mcid1;
      s_info->mcast_grp_b = session_info->u.mirror_session_entry.mcid2;
      s_info->ucast_egress_port =
          session_info->u.mirror_session_entry.epipe_port;
      s_info->ucast_egress_port_v =
          session_info->u.mirror_session_entry.epipe_port_v;
      s_info->deflect_on_drop =
          session_info->u.mirror_session_entry.deflect_on_drop;
      s_info->level1_mcast_hash = session_info->u.mirror_session_entry.hash1;
      s_info->level2_mcast_hash = session_info->u.mirror_session_entry.hash2;
      s_info->pipe_mask = session_info->u.mirror_session_entry.pipe_vec;
      s_info->ingress_cos = session_info->u.mirror_session_entry.icos;
      s_info->packet_color = session_info->u.mirror_session_entry.color;
      s_info->mcast_grp_a_v = session_info->u.mirror_session_entry.mcid1_vld;
      s_info->mcast_grp_b_v = session_info->u.mirror_session_entry.mcid2_vld;
      s_info->mcast_l1_xid = session_info->u.mirror_session_entry.mcast_l1_xid;
      s_info->mcast_l2_xid = session_info->u.mirror_session_entry.mcast_l2_xid;
      s_info->mcast_rid = session_info->u.mirror_session_entry.mcast_rid;
      s_info->icos_for_copy_to_cpu =
          session_info->u.mirror_session_entry.c2c_cos;
      s_info->copy_to_cpu = session_info->u.mirror_session_entry.c2c_vld;
      s_info->egress_port_queue =
          session_info->u.mirror_session_entry.eport_qid;
      break;
    case BF_DEV_FAMILY_TOFINO3:
      s_info->mcast_grp_a = session_info->u.mirror_session_tof3_entry.mcid1;
      s_info->mcast_grp_b = session_info->u.mirror_session_tof3_entry.mcid2;
      s_info->ucast_egress_port =
          session_info->u.mirror_session_tof3_entry.epipe_port;
      s_info->ucast_egress_port_v =
          session_info->u.mirror_session_tof3_entry.epipe_port_v;
      s_info->deflect_on_drop =
          session_info->u.mirror_session_tof3_entry.deflect_on_drop;
      s_info->level1_mcast_hash =
          session_info->u.mirror_session_tof3_entry.hash1;
      s_info->level2_mcast_hash =
          session_info->u.mirror_session_tof3_entry.hash2;
      s_info->pipe_mask = session_info->u.mirror_session_tof3_entry.pipe_vec;
      s_info->ingress_cos = session_info->u.mirror_session_tof3_entry.icos;
      s_info->packet_color = session_info->u.mirror_session_tof3_entry.color;
      s_info->mcast_grp_a_v =
          session_info->u.mirror_session_tof3_entry.mcid1_vld;
      s_info->mcast_grp_b_v =
          session_info->u.mirror_session_tof3_entry.mcid2_vld;
      s_info->mcast_l1_xid =
          session_info->u.mirror_session_tof3_entry.mcast_l1_xid;
      s_info->mcast_l2_xid =
          session_info->u.mirror_session_tof3_entry.mcast_l2_xid;
      s_info->mcast_rid = session_info->u.mirror_session_tof3_entry.mcast_rid;
      s_info->icos_for_copy_to_cpu =
          session_info->u.mirror_session_tof3_entry.c2c_cos;
      s_info->copy_to_cpu = session_info->u.mirror_session_tof3_entry.c2c_vld;
      s_info->egress_port_queue =
          session_info->u.mirror_session_tof3_entry.eport_qid;
      break;

    case BF_DEV_FAMILY_UNKNOWN:
      return PIPE_INVALID_ARG;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_mirror_session_enable_get(dev_target_t dev_target,
                                                 bf_mirror_id_t sid,
                                                 bool *session_enable) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_target.device_id);
  if (!dev_info) return PIPE_INVALID_ARG;

  mirror_info_node_t *p;
  p = pipe_mgr_mirror_buf_get_session_by_pipe(
      dev_target.device_id, sid, dev_target.dev_pipe_id);
  if (p == NULL) return PIPE_OBJ_NOT_FOUND;
  if (p->enable_ing || p->enable_egr) {
    *session_enable = true;
  } else {
    *session_enable = false;
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_mirror_session_get(dev_target_t dev_target,
                                          bf_mirror_id_t sid,
                                          bf_mirror_session_info_t *s_info) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_target.device_id);
  if (!dev_info) return PIPE_INVALID_ARG;

  mirror_info_node_t *p;
  p = pipe_mgr_mirror_buf_get_session_by_pipe(
      dev_target.device_id, sid, dev_target.dev_pipe_id);
  if (p == NULL) return PIPE_OBJ_NOT_FOUND;

  pipe_mgr_session_info_translate_back(dev_info, s_info, &p->session_info);

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_mirror_session_get_first(
    dev_target_t dev_target,
    bf_mirror_session_info_t *s_info,
    bf_mirror_id_t *sid,
    bf_dev_pipe_t *pipe_id) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_target.device_id);
  if (!dev_info) return PIPE_INVALID_ARG;
  /* If the pipe in the dev-target is all-pipes then simply return the first
   * configured mirror session.  If the pipe is not all-pipes then we may need
   * to iterate through the map until we find the first configured session for
   * the requested pipe. */
  mirror_info_node_t p;
  pipe_status_t sts =
      pipe_mgr_mirror_buf_get_first_session(dev_target.device_id, sid, &p);
  if (sts != PIPE_SUCCESS) return PIPE_OBJ_NOT_FOUND;
  if (dev_target.dev_pipe_id != BF_DEV_PIPE_ALL) {
    while (sts == PIPE_SUCCESS && dev_target.dev_pipe_id != p.pipe_id) {
      sts = pipe_mgr_mirror_buf_get_next_session(
          dev_target.device_id, p.sid, p.pipe_id, sid, &p);
    }
  }
  if (sts != PIPE_SUCCESS) return PIPE_OBJ_NOT_FOUND;

  pipe_mgr_session_info_translate_back(dev_info, s_info, &p.session_info);
  *pipe_id = p.pipe_id;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_mirror_session_get_next(
    dev_target_t dev_target,
    bf_mirror_id_t cur_sid,
    bf_dev_pipe_t cur_pipe_id,
    bf_mirror_session_info_t *next_info,
    bf_mirror_id_t *next_sid,
    bf_dev_pipe_t *next_pipe_id) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_target.device_id);
  if (!dev_info) return PIPE_INVALID_ARG;
  mirror_info_node_t p;
  pipe_status_t sts = pipe_mgr_mirror_buf_get_next_session(
      dev_target.device_id, cur_sid, cur_pipe_id, next_sid, &p);
  if (sts != PIPE_SUCCESS) return PIPE_OBJ_NOT_FOUND;
  if (dev_target.dev_pipe_id != BF_DEV_PIPE_ALL) {
    while (sts == PIPE_SUCCESS && dev_target.dev_pipe_id != p.pipe_id) {
      sts = pipe_mgr_mirror_buf_get_next_session(
          dev_target.device_id, p.sid, p.pipe_id, next_sid, &p);
    }
  }
  if (sts != PIPE_SUCCESS) return PIPE_OBJ_NOT_FOUND;

  pipe_mgr_session_info_translate_back(dev_info, next_info, &p.session_info);
  *next_pipe_id = p.pipe_id;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_mirror_session_get_count(dev_target_t dev_target,
                                                uint32_t *count) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_target.device_id);
  if (!dev_info) return PIPE_INVALID_ARG;
  bf_dev_id_t dev_id = dev_target.device_id;
  PIPE_MGR_LOCK(&mirror_session_cache[dev_id].mirror_lock);
  *count = bf_map_count(&mirror_session_cache[dev_id].mir_sess);
  PIPE_MGR_UNLOCK(&mirror_session_cache[dev_id].mirror_lock);
  return PIPE_SUCCESS;
}
