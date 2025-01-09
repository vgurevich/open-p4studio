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
 * @file pipe_mgr_hitless_ha.c
 * @date
 *
 * Implementation of pipeline management driver interface
 */

/* Module header files */
#include <dvm/bf_drv_intf.h>
#include <lld/bf_dma_if.h>
#include <lld/lld_inst_list_fmt.h>
#include <tofino_regs/tofino.h>

/* Local header files */
#include "pipe_mgr_int.h"
#include "pipe_mgr_sm.h"
#include "pipe_mgr_tcam.h"
#include "pipe_mgr_tcam_hw.h"
#include "pipe_mgr_tcam_ha.h"
#include "pipe_mgr_alpm.h"
#include "pipe_mgr_alpm_ha.h"
#include "pipe_mgr_exm_tbl_init.h"
#include "pipe_mgr_exm_ha.h"
#include "pipe_mgr_adt_mgr_ha_int.h"
#include "pipe_mgr_adt_init.h"
#include "pipe_mgr_stat_tbl_init.h"
#include "pipe_mgr_meter_tbl_init.h"
#include "pipe_mgr_select_tbl.h"
#include "pipe_mgr_select_ha.h"
#include "pipe_mgr_phase0_tbl_mgr.h"
#include "pipe_mgr_tbl.h"
#include "pipe_mgr_stful_tbl_mgr.h"
#include "pipe_mgr_idle.h"
#include "pipe_mgr_phy_mem_map.h"
#include "pipe_mgr_mirror_buffer_ha.h"

/* HA Sequence
 * - Add all tables
 * - pipe_mgr_ha_update_symmetricity Update symmetricity of tables
 * - pipe_mgr_ha_initiate_hw_read - Read the HW state and update shadow mem
 * - Each table manager reads data from shadow and create move list
 * - Process move list in HLP
 */

/* HLP Functions:
 * device_add()
 * - Either use move list from A (below) or move list replay from the
 *   application on the virtual device
 *
 * replay of config
 * - Match the replay to an existing entry or keep track of new entry
 *
 * compute_delta_changes
 * - Reconcile and compute the delta changes
 * - Create move lists for the delta changes ------ B
 * - If virtual device, call the applications move list callbacks
 *
 * push_delta_changes
 * - Nothing
 */

/* LLP Functions:
 * device_add()/device_add_with_virtual()
 * - Read HW
 * - If there is no virtual device, create a move-list ----- A
 *
 * replay of config -- in case a virtual device is tied to this physical device
 * - Compare the move-list with existing and issue ilists instructions
 *
 * compute_delta_changes
 *  - If virtual device does not exist
 *     - The session mgr will push the move lists created for delta changes (B)
 *     - Behave similar to replay of config above
 *
 * push_delta_changes
 * - Push the populated ilists
 */

pipe_mgr_hitless_ha_ctx_t hitless_ha_ctx[BF_MAX_DEV_COUNT];

typedef struct pipe_mgr_ha_read_cb_cookie_s {
  bf_dev_id_t dev_id;
  uint64_t mem_addr;
  bool compare_with_pipe;
  bf_dev_pipe_t compare_pipe_id;
} pipe_mgr_ha_read_cb_cookie_t;

static rmt_mem_type_t pipe_mgr_get_rmt_mem_type_for_pipe_mem_type(
    pipe_mem_type_t pipe_mem_type) {
  rmt_mem_type_t rmt_mem_type = RMT_MEM_SRAM;
  switch (pipe_mem_type) {
    case pipe_mem_type_unit_ram:
      rmt_mem_type = RMT_MEM_SRAM;
      break;
    case pipe_mem_type_map_ram:
      rmt_mem_type = RMT_MEM_MAP_RAM;
      break;
    case pipe_mem_type_stats_deferred_access_ram:
      rmt_mem_type = RMT_MEM_SRAM;
      break;
    case pipe_mem_type_meter_deferred_access_ram:
      rmt_mem_type = RMT_MEM_SRAM;
      break;
    case pipe_mem_type_tcam:
      rmt_mem_type = RMT_MEM_TCAM;
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      break;
  }
  return rmt_mem_type;
}

typedef pipe_status_t (*mat_tbl_iter_func)(rmt_dev_info_t *dev_info,
                                           rmt_dev_profile_info_t *profile_info,
                                           pipe_mat_tbl_info_t *mat_tbl_info,
                                           void *arg);

static pipe_status_t iterate_all_mat_tbls(rmt_dev_info_t *dev_info,
                                          mat_tbl_iter_func iter_func,
                                          mat_tbl_iter_func cleanup_func,
                                          void *iter_arg,
                                          const char *where,
                                          const int line) {
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t p = 0;
  uint32_t completed_tbls = 0;

  for (p = 0; p < dev_info->num_pipeline_profiles; p++) {
    rmt_dev_profile_info_t *dev_profile_info = dev_info->profile_info[p];
    unsigned i;
    pipe_mat_tbl_info_t *mat_tbl_info = NULL;
    for (mat_tbl_info = dev_profile_info->tbl_info_list.mat_tbl_list, i = 0;
         i < dev_profile_info->tbl_info_list.num_mat_tbls;
         ++mat_tbl_info, ++i) {
      rc = iter_func(dev_info, dev_profile_info, mat_tbl_info, iter_arg);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR("Mat tbl 0x%x returned error 0x%x(%s) on device %d at %s:%d",
                  mat_tbl_info->handle,
                  rc,
                  pipe_str_err(rc),
                  dev_info->dev_id,
                  where,
                  line);
        break;
      }
      completed_tbls++;
    }
    if (rc != PIPE_SUCCESS) {
      break;
    }
  }

  /* Cleanup after error */
  if ((rc != PIPE_SUCCESS) && cleanup_func) {
    uint32_t tbls = 0;
    for (p = 0; p < dev_info->num_pipeline_profiles; p++) {
      rmt_dev_profile_info_t *dev_profile_info = dev_info->profile_info[p];
      unsigned i;
      pipe_mat_tbl_info_t *mat_tbl_info = NULL;
      for (mat_tbl_info = dev_profile_info->tbl_info_list.mat_tbl_list, i = 0;
           (tbls < completed_tbls) &&
           (i < dev_profile_info->tbl_info_list.num_mat_tbls);
           ++mat_tbl_info, ++i, ++tbls) {
        rc = cleanup_func(dev_info, dev_profile_info, mat_tbl_info, iter_arg);
        if (rc != PIPE_SUCCESS) {
          LOG_ERROR(
              "Mat tbl 0x%x returned error 0x%x(%s) on device %d at %s:%d",
              mat_tbl_info->handle,
              rc,
              pipe_str_err(rc),
              dev_info->dev_id,
              where,
              line);
          break;
        }
        completed_tbls++;
      }
      if (rc != PIPE_SUCCESS) {
        break;
      }
    }
  }
  return PIPE_SUCCESS;
}

typedef pipe_status_t (*adt_tbl_iter_func)(rmt_dev_info_t *dev_info,
                                           rmt_dev_profile_info_t *profile_info,
                                           pipe_adt_tbl_info_t *adt_tbl_info,
                                           void *arg);

static pipe_status_t iterate_all_adt_tbls(rmt_dev_info_t *dev_info,
                                          adt_tbl_iter_func iter_func,
                                          adt_tbl_iter_func cleanup_func,
                                          void *iter_arg,
                                          const char *where,
                                          const int line) {
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t p = 0;
  uint32_t completed_tbls = 0;

  for (p = 0; p < dev_info->num_pipeline_profiles; p++) {
    rmt_dev_profile_info_t *dev_profile_info = dev_info->profile_info[p];
    unsigned i;
    pipe_adt_tbl_info_t *adt_tbl_info = NULL;
    for (adt_tbl_info = dev_profile_info->tbl_info_list.adt_tbl_list, i = 0;
         i < dev_profile_info->tbl_info_list.num_adt_tbls;
         ++adt_tbl_info, ++i) {
      rc = iter_func(dev_info, dev_profile_info, adt_tbl_info, iter_arg);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR("ADT tbl 0x%x returned error 0x%x(%s) on device %d at %s:%d",
                  adt_tbl_info->handle,
                  rc,
                  pipe_str_err(rc),
                  dev_info->dev_id,
                  where,
                  line);
        break;
      }
      completed_tbls++;
    }
    if (rc != PIPE_SUCCESS) {
      break;
    }
  }

  /* Cleanup after error */
  if ((rc != PIPE_SUCCESS) && cleanup_func) {
    uint32_t tbls = 0;
    for (p = 0; p < dev_info->num_pipeline_profiles; p++) {
      rmt_dev_profile_info_t *dev_profile_info = dev_info->profile_info[p];
      unsigned i;
      pipe_adt_tbl_info_t *adt_tbl_info = NULL;
      for (adt_tbl_info = dev_profile_info->tbl_info_list.adt_tbl_list, i = 0;
           (tbls < completed_tbls) &&
           (i < dev_profile_info->tbl_info_list.num_adt_tbls);
           ++adt_tbl_info, ++i, ++tbls) {
        rc = cleanup_func(dev_info, dev_profile_info, adt_tbl_info, iter_arg);
        if (rc != PIPE_SUCCESS) {
          LOG_ERROR(
              "ADT tbl 0x%x returned error 0x%x(%s) on device %d at %s:%d",
              adt_tbl_info->handle,
              rc,
              pipe_str_err(rc),
              dev_info->dev_id,
              where,
              line);
          break;
        }
        completed_tbls++;
      }
      if (rc != PIPE_SUCCESS) {
        break;
      }
    }
  }
  return PIPE_SUCCESS;
}

typedef pipe_status_t (*sel_tbl_iter_func)(rmt_dev_info_t *dev_info,
                                           rmt_dev_profile_info_t *profile_info,
                                           pipe_select_tbl_info_t *sel_tbl_info,
                                           void *arg);

static pipe_status_t iterate_all_sel_tbls(rmt_dev_info_t *dev_info,
                                          sel_tbl_iter_func iter_func,
                                          sel_tbl_iter_func cleanup_func,
                                          void *iter_arg,
                                          const char *where,
                                          const int line) {
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t p = 0;
  uint32_t completed_tbls = 0;

  for (p = 0; p < dev_info->num_pipeline_profiles; p++) {
    rmt_dev_profile_info_t *dev_profile_info = dev_info->profile_info[p];
    unsigned i;
    pipe_select_tbl_info_t *sel_tbl_info = NULL;
    for (sel_tbl_info = dev_profile_info->tbl_info_list.select_tbl_list, i = 0;
         i < dev_profile_info->tbl_info_list.num_select_tbls;
         ++sel_tbl_info, ++i) {
      rc = iter_func(dev_info, dev_profile_info, sel_tbl_info, iter_arg);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR("sel tbl 0x%x returned error 0x%x(%s) on device %d at %s:%d",
                  sel_tbl_info->handle,
                  rc,
                  pipe_str_err(rc),
                  dev_info->dev_id,
                  where,
                  line);
        break;
      }
      completed_tbls++;
    }
    if (rc != PIPE_SUCCESS) {
      break;
    }
  }

  /* Cleanup after error */
  if ((rc != PIPE_SUCCESS) && cleanup_func) {
    uint32_t tbls = 0;
    for (p = 0; p < dev_info->num_pipeline_profiles; p++) {
      rmt_dev_profile_info_t *dev_profile_info = dev_info->profile_info[p];
      unsigned i;
      pipe_select_tbl_info_t *sel_tbl_info = NULL;
      for (sel_tbl_info = dev_profile_info->tbl_info_list.select_tbl_list,
          i = 0;
           (tbls < completed_tbls) &&
           (i < dev_profile_info->tbl_info_list.num_select_tbls);
           ++sel_tbl_info, ++i, ++tbls) {
        rc = cleanup_func(dev_info, dev_profile_info, sel_tbl_info, iter_arg);
        if (rc != PIPE_SUCCESS) {
          LOG_ERROR(
              "sel tbl 0x%x returned error 0x%x(%s) on device %d at %s:%d",
              sel_tbl_info->handle,
              rc,
              pipe_str_err(rc),
              dev_info->dev_id,
              where,
              line);
          break;
        }
      }
      if (rc != PIPE_SUCCESS) {
        break;
      }
    }
  }
  return PIPE_SUCCESS;
}

typedef pipe_status_t (*meter_tbl_iter_func)(
    rmt_dev_info_t *dev_info,
    rmt_dev_profile_info_t *profile_info,
    pipe_meter_tbl_info_t *meter_tbl_info,
    void *arg);

static pipe_status_t iterate_all_meter_tbls(rmt_dev_info_t *dev_info,
                                            meter_tbl_iter_func iter_func,
                                            void *iter_arg,
                                            const char *where,
                                            const int line) {
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t p = 0;

  for (p = 0; p < dev_info->num_pipeline_profiles; p++) {
    rmt_dev_profile_info_t *dev_profile_info = dev_info->profile_info[p];
    unsigned i;
    pipe_meter_tbl_info_t *meter_tbl_info = NULL;
    for (meter_tbl_info = dev_profile_info->tbl_info_list.meter_tbl_list, i = 0;
         i < dev_profile_info->tbl_info_list.num_meter_tbls;
         ++meter_tbl_info, ++i) {
      rc = iter_func(dev_info, dev_profile_info, meter_tbl_info, iter_arg);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "Meter tbl 0x%x returned error 0x%x(%s) on device %d at %s:%d",
            meter_tbl_info->handle,
            rc,
            pipe_str_err(rc),
            dev_info->dev_id,
            where,
            line);
        break;
      }
    }
    if (rc != PIPE_SUCCESS) {
      break;
    }
  }
  return PIPE_SUCCESS;
}

typedef pipe_status_t (*stful_tbl_iter_func)(
    rmt_dev_info_t *dev_info,
    rmt_dev_profile_info_t *profile_info,
    pipe_stful_tbl_info_t *stful_tbl_info,
    void *arg);

static pipe_status_t iterate_all_stful_tbls(rmt_dev_info_t *dev_info,
                                            stful_tbl_iter_func iter_func,
                                            void *iter_arg,
                                            const char *where,
                                            const int line) {
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t p = 0;
  uint32_t completed_tbls = 0;

  for (p = 0; p < dev_info->num_pipeline_profiles; p++) {
    rmt_dev_profile_info_t *dev_profile_info = dev_info->profile_info[p];
    unsigned i;
    pipe_stful_tbl_info_t *stful_tbl_info = NULL;
    for (stful_tbl_info = dev_profile_info->tbl_info_list.stful_tbl_list, i = 0;
         i < dev_profile_info->tbl_info_list.num_sful_tbls;
         ++stful_tbl_info, ++i) {
      rc = iter_func(dev_info, dev_profile_info, stful_tbl_info, iter_arg);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "Stful tbl 0x%x returned error 0x%x(%s) on device %d at %s:%d",
            stful_tbl_info->handle,
            rc,
            pipe_str_err(rc),
            dev_info->dev_id,
            where,
            line);
        break;
      }
      completed_tbls++;
    }
    if (rc != PIPE_SUCCESS) {
      break;
    }
  }
  return PIPE_SUCCESS;
}

typedef pipe_status_t (*stat_tbl_iter_func)(
    rmt_dev_info_t *dev_info,
    rmt_dev_profile_info_t *profile_info,
    pipe_stat_tbl_info_t *stat_tbl_info,
    void *arg);

#if 0
static pipe_status_t iterate_all_stat_tbls(rmt_dev_info_t *dev_info,
                                           stat_tbl_iter_func iter_func,
                                           void *iter_arg,
                                           const char *where,
                                           const int line) {
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t p = 0;
  uint32_t completed_tbls = 0;

  for (p = 0; p < dev_info->num_pipeline_profiles; p++) {
    rmt_dev_profile_info_t *dev_profile_info = dev_info->profile_info[p];
    unsigned i;
    pipe_stat_tbl_info_t *stat_tbl_info = NULL;
    for (stat_tbl_info = dev_profile_info->tbl_info_list.stat_tbl_list, i = 0;
         i < dev_profile_info->tbl_info_list.num_stat_tbls;
         ++stat_tbl_info, ++i) {
      rc = iter_func(dev_info, dev_profile_info, stat_tbl_info, iter_arg);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR("Stat tbl 0x%x returned error 0x%x(%s) on device %d at %s:%d",
                  stat_tbl_info->handle,
                  rc,
                  pipe_str_err(rc),
                  dev_info->dev_id,
                  where,
                  line);
        break;
      }
      completed_tbls++;
    }
    if (rc != PIPE_SUCCESS) {
      break;
    }
  }
  return PIPE_SUCCESS;
}
#endif

static void phy_tbl_read_cb(pipe_mgr_drv_buf_t *b,
                            uint32_t offset,
                            uint32_t count,
                            bool had_err,
                            void *arg) {
  (void)offset;
  (void)count;
  pipe_mgr_ha_read_cb_cookie_t *cookie = (pipe_mgr_ha_read_cb_cookie_t *)arg;
  pipe_status_t rc = PIPE_SUCCESS;

  /* Read the data from the buffer and populate the shadow memory */

  if (!cookie) {
    LOG_ERROR("%s:%d Error, cookie is NULL", __func__, __LINE__);
    PIPE_MGR_DBGCHK(0);
    return;
  }
  bf_dev_id_t dev_id = cookie->dev_id;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s:%d Error, dev_info is NULL", __func__, __LINE__);
    PIPE_MGR_DBGCHK(0);
    goto cleanup;
  }
  bf_dev_pipe_t phy_pipe_id =
      dev_info->dev_cfg.pipe_id_from_addr(cookie->mem_addr);
  uint8_t stage_id = dev_info->dev_cfg.stage_id_from_addr(cookie->mem_addr);
  pipe_mem_type_t mem_type =
      dev_info->dev_cfg.mem_type_from_addr(cookie->mem_addr);
  mem_id_t mem_id = dev_info->dev_cfg.mem_id_from_addr(cookie->mem_addr);

  bool compare_with_pipe = cookie->compare_with_pipe;
  bf_dev_pipe_t compare_pipe_id = cookie->compare_pipe_id;
  UNUSED(compare_pipe_id);

  uint32_t mem_width = (TOF_SRAM_UNIT_WIDTH + 7) / 8;

  if (had_err) {
    LOG_CRIT(
        "%s:%d Error while reading state from hardware for "
        "dev %d phy_pipe %d stage %d mem_type %s mem_id %d",
        __func__,
        __LINE__,
        dev_id,
        phy_pipe_id,
        stage_id,
        mem_type_to_str(mem_type),
        mem_id);
    goto cleanup;
  }

  bf_dev_pipe_t pipe_id;
  rc = pipe_mgr_map_phy_pipe_id_to_log_pipe_id(dev_id, phy_pipe_id, &pipe_id);
  if (rc != PIPE_SUCCESS) {
    PIPE_MGR_ASSERT(0);
    LOG_ERROR("%s:%d Unable to get logical pipe for physical pipe %d on dev %d",
              __func__,
              __LINE__,
              phy_pipe_id,
              dev_id);
    goto cleanup;
  }

  uint32_t entry_count;
  entry_count = pipe_mgr_get_mem_unit_depth(
      dev_id, pipe_mgr_get_rmt_mem_type_for_pipe_mem_type(mem_type));

  uint8_t *data = b->addr;
  uint32_t line_num;
  for (line_num = 0; line_num < entry_count; line_num++, data += mem_width) {
    if (!compare_with_pipe) {

      rc = pipe_mgr_phy_mem_map_write(
          dev_id, 0, pipe_id, stage_id, mem_type, mem_id, line_num, data, NULL);
      if (rc != PIPE_SUCCESS) {
        LOG_CRIT(
            "%s:%d Error updating shadow memory for dev %d pipe %d stage %d "
            "mem_type %s mem_id %d line_num %d rc %s",
            __func__,
            __LINE__,
            dev_id,
            pipe_id,
            stage_id,
            mem_type_to_str(mem_type),
            mem_id,
            line_num,
            pipe_str_err(rc));
        goto cleanup;
      }
      if (!line_num) {
        LOG_TRACE("Created mem shadow: dev %d pipe %d stage %d type %s unit %d",
                  dev_id,
                  pipe_id,
                  stage_id,
                  mem_type_to_str(mem_type),
                  mem_id);
      }
    } else {
/* Removing the byte-by-byte compare of memory across pipes.  We do not
 * require the SRAMs/TCAMs to be identical, we only require the set of
 * active enties to be identical.  We can have a case where a table was
 * asymmetric, all entries were deleted, and the table was made symmetric
 * again.  In this case the SRAM contents may not be identical because
 * the asymmetric entries which were deleted simply had their valid bits
 * cleared so the SRAM contents are different even though the set of
 * entries in the SRAMs is now the same.  This check should be performed
 * when entries are decoded from the shadows. */
#if 0
      /* Compare the contents of the shadow memory with what is in buffer and
       * mark dirty if they are not same */
      uint8_t *exp_data = NULL;
      rc = pipe_mgr_phy_mem_map_get_ref(dev_id,
                                        mem_type,
                                        compare_pipe_id,
                                        stage_id,
                                        mem_id,
                                        line_num,
                                        &exp_data,
                                        true);
      if (rc != PIPE_SUCCESS) {
        LOG_CRIT(
            "%s:%d Error comparing shadow memory for dev %d pipe %d stage %d "
            "mem_type %s mem_id %d line_num %d rc %s",
            __func__,
            __LINE__,
            dev_id,
            compare_pipe_id,
            stage_id,
            mem_type_to_str(mem_type),
            mem_id,
            line_num,
            pipe_str_err(rc));
        goto cleanup;
      }
      if (memcmp(exp_data, data, mem_width)) {
        LOG_CRIT(
            "%s:%d Error symmetric data mismatch for dev %d pipe %d and %d "
            "stage %d mem_type %s mem_id %d line_num %d rc %s",
            __func__,
            __LINE__,
            dev_id,
            pipe_id,
            compare_pipe_id,
            stage_id,
            mem_type_to_str(mem_type),
            mem_id,
            line_num,
            pipe_str_err(rc));
        PIPE_MGR_DBGCHK(0);
      }
#endif
    }
  }
cleanup:
  if (cookie) {
    PIPE_MGR_FREE(cookie);
  }
}

static bool rmt_tbl_is_physically_addressed(rmt_tbl_type_t tbl_type) {
  switch (tbl_type) {
    case RMT_TBL_TYPE_DIRECT_MATCH:
    case RMT_TBL_TYPE_HASH_MATCH:
    case RMT_TBL_TYPE_TERN_MATCH:
    case RMT_TBL_TYPE_TERN_INDIR:
    case RMT_TBL_TYPE_ACTION_DATA:
    case RMT_TBL_TYPE_HASH_ACTION:
    case RMT_TBL_TYPE_ATCAM_MATCH:
      return true;
    default:
      return false;
  }
}

static pipe_status_t pipe_mgr_ha_read_phy_mems(pipe_sess_hdl_t sess_hdl,
                                               rmt_dev_info_t *dev_info,
                                               bf_dev_pipe_t pipe_id,
                                               rmt_tbl_info_t *rmt_tbls,
                                               uint32_t num_rmt_info,
                                               bool compare_with_pipe,
                                               bf_dev_pipe_t compare_pipe_id) {
  pipe_status_t rc;
  bf_dev_pipe_t phy_pipe_id;
  bf_dev_id_t dev_id = dev_info->dev_id;
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe_id, &phy_pipe_id);
  uint32_t i;
  for (i = 0; i < num_rmt_info; i++) {
    rmt_tbl_info_t *rmt_info = &rmt_tbls[i];

    if (!rmt_tbl_is_physically_addressed(rmt_info->type)) {
      continue;
    }

    uint8_t stage_id = rmt_info->stage_id;
    uint32_t bank = 0;

    uint32_t mem_word_width = 0;
    uint32_t mem_word_depth =
        pipe_mgr_get_mem_unit_depth(dev_info->dev_id, rmt_info->mem_type);

    pipe_mem_type_t pipe_mem_type = pipe_mem_type_unit_ram;

    switch (rmt_info->mem_type) {
      case RMT_MEM_SRAM:
        pipe_mem_type = pipe_mem_type_unit_ram;
        mem_word_width = (TOF_SRAM_UNIT_WIDTH + 7) / 8;
        break;
      case RMT_MEM_TCAM:

        pipe_mem_type = pipe_mem_type_tcam;
        /* Even TCAM is 128 bits long */
        mem_word_width = (TOF_SRAM_UNIT_WIDTH + 7) / 8;
        break;
      case RMT_MEM_MAP_RAM:
        /* Map RAM physical read is not needed/supported */
        PIPE_MGR_ASSERT(0);
        break;
      default:
        PIPE_MGR_ASSERT(0);
    }

    for (bank = 0; bank < rmt_info->num_tbl_banks; bank++) {
      uint32_t j;
      for (j = 0; j < rmt_info->bank_map[bank].num_tbl_word_blks; j++) {
        rmt_tbl_word_blk_t *tbl_word_blk =
            &rmt_info->bank_map[bank].tbl_word_blk[j];
        uint32_t k;
        for (k = 0; k < rmt_info->pack_format.mem_units_per_tbl_word; k++) {
          mem_id_t mem_id = tbl_word_blk->mem_id[k];

          uint64_t addr;
          addr = dev_info->dev_cfg.get_full_phy_addr(rmt_info->direction,
                                                     phy_pipe_id,
                                                     stage_id,
                                                     mem_id,
                                                     0,
                                                     pipe_mem_type);

          pipe_mgr_ha_read_cb_cookie_t *cookie;
          cookie = (pipe_mgr_ha_read_cb_cookie_t *)PIPE_MGR_MALLOC(
              sizeof(pipe_mgr_ha_read_cb_cookie_t));
          if (!cookie) {
            LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
            return PIPE_NO_SYS_RESOURCES;
          }

          cookie->dev_id = dev_id;
          cookie->mem_addr = addr;
          cookie->compare_with_pipe = compare_with_pipe;
          cookie->compare_pipe_id = compare_pipe_id;

          rc = pipe_mgr_drv_blk_rd(&sess_hdl,
                                   dev_id,
                                   mem_word_width,
                                   mem_word_depth,
                                   1,
                                   addr,
                                   phy_tbl_read_cb,
                                   cookie);
          if (rc != PIPE_SUCCESS) {
            LOG_ERROR(
                "%s:%d Error issuing read block instruction on "
                "dev %d physical pipe %d stage %d mem_type %s mem_id %d rc "
                "0x%x",
                __func__,
                __LINE__,
                dev_id,
                phy_pipe_id,
                stage_id,
                mem_type_to_str(pipe_mem_type),
                mem_id,
                rc);
            goto cleanup;
          }
        }
      }
    }
  }
  return PIPE_SUCCESS;
cleanup:
  return rc;
}

struct initiate_hw_read_t {
  bool first_pipe_only;
  pipe_sess_hdl_t sess_hdl;
};

static pipe_status_t pipe_mgr_mat_tbl_initiate_hw_read(
    rmt_dev_info_t *dev_info,
    rmt_dev_profile_info_t *profile_info,
    pipe_mat_tbl_info_t *mat_tbl_info,
    void *arg) {
  struct initiate_hw_read_t *argp = (struct initiate_hw_read_t *)arg;
  bool first_pipe_only = argp->first_pipe_only;
  pipe_sess_hdl_t sess_hdl = argp->sess_hdl;
  pipe_status_t rc = PIPE_SUCCESS;
  int pipe_id;

  if (mat_tbl_info->match_type == ALPM_MATCH) {
    // HW reads for ALPM will be taken care of by the lower-level tables
    return PIPE_SUCCESS;
  }

  if (!mat_tbl_info->rmt_info) {
    // No match ram to read for keyless tables
    return PIPE_SUCCESS;
  }

  if (mat_tbl_info->rmt_info->type == RMT_TBL_TYPE_PHASE0_MATCH) {
    /* Read phase0 registers for all the ports into the shadow memory */
    return pipe_mgr_phase0_shadow_mem_populate(
        sess_hdl, dev_info->dev_id, mat_tbl_info->handle);
  }

  bf_dev_pipe_t first_pipe_id =
      PIPE_BITMAP_GET_FIRST_SET(&profile_info->pipe_bmp);

  bool is_tbl_symmetric = mat_tbl_info->symmetric;
  if (first_pipe_only) {
    pipe_id = first_pipe_id;
  } else {
    pipe_id = PIPE_BITMAP_GET_NEXT_BIT(&profile_info->pipe_bmp, first_pipe_id);
  }

  for (; pipe_id != -1;
       pipe_id = PIPE_BITMAP_GET_NEXT_BIT(&profile_info->pipe_bmp, pipe_id)) {
    rc = pipe_mgr_ha_read_phy_mems(sess_hdl,
                                   dev_info,
                                   pipe_id,
                                   mat_tbl_info->rmt_info,
                                   mat_tbl_info->num_rmt_info,
                                   is_tbl_symmetric && !first_pipe_only,
                                   first_pipe_id);
    if (rc != PIPE_SUCCESS) {
      LOG_CRIT(
          "%s:%d Error in reading physical memory for dev %d pipe %d tbl %s "
          "0x%x, rc 0x%x(%s)",
          __func__,
          __LINE__,
          dev_info->dev_id,
          pipe_id,
          mat_tbl_info->name,
          mat_tbl_info->handle,
          rc,
          pipe_str_err(rc));
      return rc;
    }
    if (first_pipe_only) {
      break;
    }
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_adt_tbl_initiate_hw_read(
    rmt_dev_info_t *dev_info,
    rmt_dev_profile_info_t *profile_info,
    pipe_adt_tbl_info_t *adt_tbl_info,
    void *arg) {
  struct initiate_hw_read_t *argp = (struct initiate_hw_read_t *)arg;
  bool first_pipe_only = argp->first_pipe_only;
  pipe_sess_hdl_t sess_hdl = argp->sess_hdl;
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_mgr_adt_t *adt_tbl =
      pipe_mgr_adt_get(dev_info->dev_id, adt_tbl_info->handle);
  if (!adt_tbl) return PIPE_TABLE_NOT_FOUND;
  int pipe_id;

  bf_dev_pipe_t first_pipe_id =
      PIPE_BITMAP_GET_FIRST_SET(&profile_info->pipe_bmp);

  bool is_tbl_symmetric = adt_tbl->symmetric;
  if (first_pipe_only) {
    pipe_id = first_pipe_id;
  } else {
    pipe_id = PIPE_BITMAP_GET_NEXT_BIT(&profile_info->pipe_bmp, first_pipe_id);
  }

  for (; pipe_id != -1;
       pipe_id = PIPE_BITMAP_GET_NEXT_BIT(&profile_info->pipe_bmp, pipe_id)) {
    LOG_TRACE("%s: Dev %d pipe %d first-pipe %d %s 0x%x",
              __func__,
              dev_info->dev_id,
              pipe_id,
              first_pipe_id,
              adt_tbl_info->name,
              adt_tbl_info->handle);
    rc = pipe_mgr_ha_read_phy_mems(sess_hdl,
                                   dev_info,
                                   pipe_id,
                                   adt_tbl_info->rmt_info,
                                   adt_tbl_info->num_rmt_info,
                                   is_tbl_symmetric && !first_pipe_only,
                                   first_pipe_id);
    if (rc != PIPE_SUCCESS) {
      LOG_CRIT(
          "%s:%d Error in reading physical memory for dev %d pipe %d tbl %s "
          "0x%x, rc 0x%x(%s)",
          __func__,
          __LINE__,
          dev_info->dev_id,
          pipe_id,
          adt_tbl_info->name,
          adt_tbl_info->handle,
          rc,
          pipe_str_err(rc));
      return rc;
    }
    if (first_pipe_only) {
      break;
    }
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_ha_initiate_hw_read(pipe_sess_hdl_t sess_hdl,
                                                  bf_dev_id_t dev_id) {
  pipe_status_t rc;
  rmt_dev_info_t *dev_info = NULL;

  if (!(dev_info = pipe_mgr_get_dev_info(dev_id))) {
    return PIPE_OBJ_NOT_FOUND;
  }

  if (pipe_mgr_is_device_virtual(dev_id)) {
    return PIPE_SUCCESS;
  }

  /* Walk through all the mat-tbls and action-data tables and issue a block-read
   */
  struct initiate_hw_read_t arg;
  arg.first_pipe_only = true;
  arg.sess_hdl = sess_hdl;

  rc = iterate_all_mat_tbls(dev_info,
                            pipe_mgr_mat_tbl_initiate_hw_read,
                            NULL,
                            &arg,
                            __func__,
                            __LINE__);
  if (rc != PIPE_SUCCESS) {
    LOG_CRIT("%s:%d Error in initiating hw read for dev %d rc 0x%x(%s)",
             __func__,
             __LINE__,
             dev_id,
             rc,
             pipe_str_err(rc));
    return rc;
  }

  rc = iterate_all_adt_tbls(dev_info,
                            pipe_mgr_adt_tbl_initiate_hw_read,
                            NULL,
                            &arg,
                            __func__,
                            __LINE__);
  if (rc != PIPE_SUCCESS) {
    LOG_CRIT("%s:%d Error in initiating hw read for dev %d rc 0x%x(%s)",
             __func__,
             __LINE__,
             dev_id,
             rc,
             pipe_str_err(rc));
    return rc;
  }

  /* Wait until all the reads have completed */
  rc = pipe_mgr_drv_rd_blk_cmplt_all(sess_hdl, dev_id);
  if (rc != PIPE_SUCCESS) {
    LOG_CRIT(
        "%s:%d Couldn't complete read block operations on dev %d rc 0x%x(%s)",
        __func__,
        __LINE__,
        dev_id,
        rc,
        pipe_str_err(rc));
    return rc;
  }

  /* Now read the rest of the tables and compare */
  arg.first_pipe_only = false;
  arg.sess_hdl = sess_hdl;

  rc = iterate_all_mat_tbls(dev_info,
                            pipe_mgr_mat_tbl_initiate_hw_read,
                            NULL,
                            &arg,
                            __func__,
                            __LINE__);
  if (rc != PIPE_SUCCESS) {
    LOG_CRIT("%s:%d Error in initiating hw read for dev %d rc 0x%x(%s)",
             __func__,
             __LINE__,
             dev_id,
             rc,
             pipe_str_err(rc));
    return rc;
  }

  rc = iterate_all_adt_tbls(dev_info,
                            pipe_mgr_adt_tbl_initiate_hw_read,
                            NULL,
                            &arg,
                            __func__,
                            __LINE__);
  if (rc != PIPE_SUCCESS) {
    LOG_CRIT("%s:%d Error in initiating hw read for dev %d rc 0x%x(%s)",
             __func__,
             __LINE__,
             dev_id,
             rc,
             pipe_str_err(rc));
    return rc;
  }

  return PIPE_SUCCESS;
}

struct update_symmetricity_s {
  pipe_sess_hdl_t sess_hdl;
};

static pipe_status_t mat_tbl_update_symmetricity(
    rmt_dev_info_t *dev_info,
    rmt_dev_profile_info_t *profile_info,
    pipe_mat_tbl_info_t *mat_tbl_info,
    void *_arg) {
  struct update_symmetricity_s *arg = _arg;

  /* Don't restore phase0 symmetric mode by reading hardware, instead rely on
   * the API replay. */
  if (pipe_mgr_mat_tbl_is_phase0(dev_info->dev_id, mat_tbl_info->handle))
    return PIPE_SUCCESS;

  pipe_sess_hdl_t sess_hdl = arg->sess_hdl;
  scope_pipes_t *scope_pipe_bmp = NULL;

  bf_dev_id_t dev_id = dev_info->dev_id;
  bf_dev_pipe_t pipe = PIPE_BITMAP_GET_FIRST_SET(&profile_info->pipe_bmp);

  pipe_status_t rc;
  profile_id_t profile_id;
  uint32_t stage_id = 0;
  rmt_tbl_hdl_t stage_table_handle = 0;
  rc = pipe_mgr_mat_tbl_get_first_stage_table(dev_id,
                                              mat_tbl_info->handle,
                                              &profile_id,
                                              &stage_id,
                                              &stage_table_handle);
  PIPE_MGR_ASSERT(rc == PIPE_SUCCESS);
  if (rc != PIPE_SUCCESS) {
    return rc;
  }
  PIPE_MGR_ASSERT(profile_id == profile_info->profile_id);

  uint32_t scratch_val = pipe_mgr_get_mau_scratch_val(dev_id, pipe, stage_id);
  LOG_TRACE(
      "Update Symmetricity: Dev %d prof %d pipe %d Tbl %s 0x%x restore from "
      "%d.%d cached val 0x%08x",
      dev_id,
      profile_id,
      pipe,
      mat_tbl_info->name,
      mat_tbl_info->handle,
      stage_id,
      stage_table_handle,
      scratch_val);

  if (scratch_val & (1 << stage_table_handle)) {
    /* The table is asymmetric */
    /* Set it to having a single pipe-line scope. If the application wants
     * a different scope, we will reconcile during API replay.
     */
    scope_num_t num_scopes = 0;
    uint32_t i = 0;
    scope_pipe_bmp =
        PIPE_MGR_CALLOC(dev_info->dev_cfg.num_pipelines, sizeof(scope_pipes_t));
    PIPE_BITMAP_ITER(&profile_info->pipe_bmp, i) {
      scope_pipe_bmp[num_scopes] = (1 << i);
      num_scopes++;
    }
    rc = pipe_mgr_tbl_set_symmetric_mode_wrapper(sess_hdl,
                                                 dev_id,
                                                 mat_tbl_info->handle,
                                                 false,
                                                 false,
                                                 num_scopes,
                                                 scope_pipe_bmp,
                                                 false);
    PIPE_MGR_ASSERT(rc == PIPE_SUCCESS);
    if (rc != PIPE_SUCCESS) {
      PIPE_MGR_FREE(scope_pipe_bmp);
      return rc;
    }
  }
  PIPE_MGR_FREE(scope_pipe_bmp);
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_ha_update_symmetricity(pipe_sess_hdl_t sess_hdl,
                                                     bf_dev_id_t dev_id) {
  rmt_dev_info_t *dev_info = NULL;
  pipe_status_t rc;

  if (!(dev_info = pipe_mgr_get_dev_info(dev_id))) {
    return PIPE_OBJ_NOT_FOUND;
  }

  if (pipe_mgr_is_device_virtual(dev_id)) {
    return PIPE_SUCCESS;
  }

  pipe_mgr_lock_mau_scratch(dev_id);
  bf_dev_pipe_t pipe, phy_pipe;
  for (pipe = 0; pipe < dev_info->num_active_pipes; pipe++) {
    rc = pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);
    if (rc != PIPE_SUCCESS) {
      LOG_CRIT(
          "%s:%d Error in converting log pipe %d to phy-pipe dev %d rc "
          "0x%x(%s)",
          __func__,
          __LINE__,
          pipe,
          dev_id,
          rc,
          pipe_str_err(rc));
      pipe_mgr_unlock_mau_scratch(dev_id);
      return rc;
    }
    int stage_id;
    for (stage_id = 0; stage_id < dev_info->num_active_mau; stage_id++) {
      uint32_t reg_addr = 0, reg_val = 0;
      switch (dev_info->dev_family) {
        case BF_DEV_FAMILY_TOFINO:
          reg_addr =
              offsetof(Tofino, pipes[phy_pipe].mau[stage_id].dp.mau_scratch);
          break;
        case BF_DEV_FAMILY_TOFINO2:
          reg_addr =
              offsetof(tof2_reg, pipes[phy_pipe].mau[stage_id].dp.mau_scratch);
          break;
        case BF_DEV_FAMILY_TOFINO3:
          reg_addr =
              offsetof(tof3_reg, pipes[phy_pipe].mau[stage_id].dp.mau_scratch);
          break;

        case BF_DEV_FAMILY_UNKNOWN:
          PIPE_MGR_DBGCHK(0);
          pipe_mgr_unlock_mau_scratch(dev_id);
          return PIPE_UNEXPECTED;
      }
      rc = pipe_mgr_drv_reg_rd(&sess_hdl, dev_id, reg_addr, &reg_val);
      if (rc != PIPE_SUCCESS) {
        LOG_CRIT(
            "%s:%d Error in reading symmetricity of tables from hw dev %d pipe "
            "%d stage %d, rc 0x%x(%s)",
            __func__,
            __LINE__,
            dev_id,
            phy_pipe,
            stage_id,
            rc,
            pipe_str_err(rc));
        pipe_mgr_unlock_mau_scratch(dev_id);
        return rc;
      }

      pipe_mgr_set_mau_scratch_val(
          dev_id, pipe, stage_id, getp_mau_scratch_mau_scratch(&reg_val));
    }
  }

  /* Now walk over all the match tables in this device and set the symmetricity
   */
  struct update_symmetricity_s arg;
  arg.sess_hdl = sess_hdl;
  rc = iterate_all_mat_tbls(
      dev_info, mat_tbl_update_symmetricity, NULL, &arg, __func__, __LINE__);
  if (rc != PIPE_SUCCESS) {
    return rc;
  }

  pipe_mgr_unlock_mau_scratch(dev_id);
  return PIPE_SUCCESS;
}

static pipe_mgr_hitless_ha_ctx_t *get_ha_ctx(bf_dev_id_t device_id) {
  PIPE_MGR_ASSERT(device_id <= BF_MAX_DEV_COUNT);
  return &hitless_ha_ctx[device_id];
}

pipe_status_t pipe_mgr_hitless_ha_init(pipe_sess_hdl_t sess_hdl,
                                       bf_dev_id_t dev_id) {
  pipe_status_t rc = PIPE_SUCCESS;
  rc = pipe_mgr_ha_update_symmetricity(sess_hdl, dev_id);
  if (rc != PIPE_SUCCESS) {
    LOG_CRIT(
        "%s:%d Error in updating symmetricity of tables dev %d rc 0x%x(%s)",
        __func__,
        __LINE__,
        dev_id,
        rc,
        pipe_str_err(rc));
    return rc;
  }

  rc = pipe_mgr_ha_initiate_hw_read(sess_hdl, dev_id);
  if (rc != PIPE_SUCCESS) {
    LOG_CRIT("%s:%d Error in initiating hardware read dev %d rc 0x%x(%s)",
             __func__,
             __LINE__,
             dev_id,
             rc,
             pipe_str_err(rc));
    return rc;
  }

  return PIPE_SUCCESS;
}

bool pipe_mgr_hitless_ha_issue_callbacks_from_llp;

static void pipe_mgr_hitless_ha_save_ml(bf_dev_id_t dev_id,
                                        pipe_tbl_hdl_t tbl_hdl,
                                        void *ml) {
  pipe_mgr_hitless_ha_ctx_t *ha_ctx = get_ha_ctx(dev_id);
  unsigned long key = tbl_hdl;
  bf_map_sts_t map_sts = bf_map_add(&ha_ctx->saved_ml, key, ml);
  PIPE_MGR_ASSERT(map_sts == BF_MAP_OK);
}

static void *pipe_mgr_hitless_ha_get_ml(bf_dev_id_t dev_id,
                                        pipe_tbl_hdl_t tbl_hdl) {
  void *ml = NULL;
  pipe_mgr_hitless_ha_ctx_t *ha_ctx = get_ha_ctx(dev_id);
  unsigned long key = tbl_hdl;
  bf_map_sts_t map_sts = bf_map_get(&ha_ctx->saved_ml, key, &ml);
  if (map_sts == BF_MAP_OK) {
    return ml;
  }
  return NULL;
}

static pipe_status_t mat_tbl_llp_restore_state(
    rmt_dev_info_t *dev_info,
    rmt_dev_profile_info_t *profile_info,
    pipe_mat_tbl_info_t *mat_tbl_info,
    void *arg) {
  UNUSED(profile_info);
  UNUSED(arg);
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_mgr_move_list_t *ml = NULL;
  pipe_mgr_move_list_t **ml_pp = NULL;
  bool phy_device_only =
      !pipe_mgr_is_device_virtual_dev_slave(dev_info->dev_id);

  if (phy_device_only || pipe_mgr_hitless_ha_issue_callbacks_from_llp) {
    ml_pp = &ml;
  }

  bf_dev_id_t dev_id = dev_info->dev_id;
  pipe_mat_tbl_hdl_t mat_tbl_handle = mat_tbl_info->handle;

  /* Restore the LLP state, if a move-list pointer is passed create move-lists*/
  switch (pipe_mgr_sm_tbl_owner(dev_id, mat_tbl_handle)) {
    case PIPE_MGR_TBL_OWNER_EXM:
      rc = pipe_mgr_exm_llp_restore_state(dev_id, mat_tbl_handle, ml_pp);
      break;
    case PIPE_MGR_TBL_OWNER_TRN:
      rc = pipe_mgr_tcam_llp_restore_state(dev_id, mat_tbl_handle, ml_pp);
      break;
    case PIPE_MGR_TBL_OWNER_ALPM:
      // ALPM has no llp state. Do nothing here.
      break;
    case PIPE_MGR_TBL_OWNER_PHASE0:
      rc = pipe_mgr_phase0_llp_restore_state(dev_id, mat_tbl_handle, ml_pp);
      break;
    case PIPE_MGR_TBL_OWNER_NO_KEY:
      // Nothing to be done
      break;
    default:
      PIPE_MGR_ASSERT(0);
      break;
  }
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in restoring LLP state for mat tbl 0x%x, device id %d, "
        "err %s",
        __func__,
        __LINE__,
        mat_tbl_handle,
        dev_id,
        pipe_str_err(rc));
    return rc;
  }

  if (!phy_device_only && pipe_mgr_hitless_ha_issue_callbacks_from_llp) {
    pipe_mgr_hitless_ha_save_ml(dev_id, mat_tbl_handle, (void *)ml);
  }
  mat_tbl_info->ha_move_list = (void *)ml;
  return rc;
}

static pipe_status_t mat_tbl_hlp_restore_state(
    rmt_dev_info_t *dev_info,
    rmt_dev_profile_info_t *profile_info,
    pipe_mat_tbl_info_t *mat_tbl_info,
    void *arg) {
  UNUSED(profile_info);
  UNUSED(arg);
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_mgr_move_list_t *ml = NULL;
  pipe_mat_tbl_info_t *ll_mat_tbl_info = NULL;
  pipe_mat_tbl_hdl_t ll_mat_tbl_handle;

  bf_dev_id_t device_id = dev_info->dev_id;
  pipe_mat_tbl_hdl_t mat_tbl_handle = mat_tbl_info->handle;
  ml = (pipe_mgr_move_list_t *)mat_tbl_info->ha_move_list;

  /* If there is no virtual device with this context, restore the state in HLP
   * Otherwise expect a move-list processing to come to reconcile
   */
  uint32_t success_count = 0;
  /* Restore the HLP state */
  switch (pipe_mgr_sm_tbl_owner(device_id, mat_tbl_handle)) {
    case PIPE_MGR_TBL_OWNER_EXM:
      rc = pipe_mgr_exm_hlp_restore_state(
          device_id, mat_tbl_handle, ml, &success_count);
      break;
    case PIPE_MGR_TBL_OWNER_TRN:
      if (!mat_tbl_info->alpm_info) {
        // Lower-level tcam tables for ALPM will be taken care of by the
        // ALPM manager
        rc = pipe_mgr_tcam_hlp_restore_state(
            device_id, mat_tbl_handle, ml, &success_count);
      }
      break;
    case PIPE_MGR_TBL_OWNER_ALPM:
      // First restore the preclassifier table
      ll_mat_tbl_handle = mat_tbl_info->alpm_info->preclass_handle;
      ll_mat_tbl_info = pipe_mgr_get_tbl_info(
          device_id, ll_mat_tbl_handle, __func__, __LINE__);
      if (ll_mat_tbl_info == NULL) {
        LOG_ERROR(
            "%s:%d Preclass table 0x%x for ALPM table %s 0x%x not found for "
            "device %d",
            __func__,
            __LINE__,
            ll_mat_tbl_handle,
            mat_tbl_info->name,
            mat_tbl_handle,
            device_id);
        return PIPE_OBJ_NOT_FOUND;
      }
      rc = pipe_mgr_tcam_hlp_restore_state(device_id,
                                           ll_mat_tbl_handle,
                                           ll_mat_tbl_info->ha_move_list,
                                           &success_count);
      if (rc != PIPE_SUCCESS) {
        return rc;
      }

      // Next restore the ATCAM table
      success_count = 0;
      ll_mat_tbl_handle = mat_tbl_info->alpm_info->atcam_handle;
      ll_mat_tbl_info = pipe_mgr_get_tbl_info(
          device_id, ll_mat_tbl_handle, __func__, __LINE__);
      if (ll_mat_tbl_info == NULL) {
        LOG_ERROR(
            "%s:%d ATCAM table 0x%x for ALPM table %s 0x%x not found for "
            "device %d",
            __func__,
            __LINE__,
            ll_mat_tbl_handle,
            mat_tbl_info->name,
            mat_tbl_handle,
            device_id);
        return PIPE_OBJ_NOT_FOUND;
      }
      rc = pipe_mgr_tcam_hlp_restore_state(device_id,
                                           ll_mat_tbl_handle,
                                           ll_mat_tbl_info->ha_move_list,
                                           &success_count);
      if (rc != PIPE_SUCCESS) {
        return rc;
      }

      // Finally restore the ALPM logical table
      rc = pipe_mgr_alpm_hlp_restore_state(device_id, mat_tbl_handle);
      break;
    case PIPE_MGR_TBL_OWNER_PHASE0:
      rc = pipe_mgr_phase0_hlp_restore_state(pipe_mgr_get_int_sess_hdl(),
                                             device_id,
                                             mat_tbl_handle,
                                             ml,
                                             &success_count);
      break;
    case PIPE_MGR_TBL_OWNER_NO_KEY:
      // Nothing to be done
      break;
    default:
      PIPE_MGR_ASSERT(0);
      break;
  }
  /* Do not free the move list as the move list nodes are stored in the ha
   * spec
   */
  return rc;
}

static pipe_status_t adt_tbl_llp_restore_state(
    rmt_dev_info_t *dev_info,
    rmt_dev_profile_info_t *profile_info,
    pipe_adt_tbl_info_t *adt_tbl_info,
    void *arg) {
  if (adt_tbl_info->ref_type == PIPE_TBL_REF_TYPE_DIRECT) {
    /* No state stored for directly referenced action data tables */
    return PIPE_SUCCESS;
  }
  UNUSED(profile_info);
  UNUSED(arg);
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_mgr_adt_move_list_t *ml = NULL;
  pipe_mgr_adt_move_list_t **mlp = NULL;

  bool phy_device_only =
      !pipe_mgr_is_device_virtual_dev_slave(dev_info->dev_id);
  if (phy_device_only || pipe_mgr_hitless_ha_issue_callbacks_from_llp) {
    mlp = &ml;
  }
  bf_dev_id_t dev_id = dev_info->dev_id;
  pipe_adt_tbl_hdl_t adt_tbl_hdl = adt_tbl_info->handle;
  rc = pipe_mgr_adt_llp_restore_state(dev_id, adt_tbl_hdl, mlp);

  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in restoring LLP state for ADT %s 0x%x dev %d err %s",
        __func__,
        __LINE__,
        adt_tbl_info->name,
        adt_tbl_hdl,
        dev_id,
        pipe_str_err(rc));
    return rc;
  }

  if (!phy_device_only && pipe_mgr_hitless_ha_issue_callbacks_from_llp) {
    pipe_mgr_hitless_ha_save_ml(dev_id, adt_tbl_hdl, (void *)ml);
  }
  adt_tbl_info->ha_move_list = (void *)ml;
  return rc;
}

static pipe_status_t adt_tbl_hlp_restore_state(
    rmt_dev_info_t *dev_info,
    rmt_dev_profile_info_t *profile_info,
    pipe_adt_tbl_info_t *adt_tbl_info,
    void *arg) {
  if (adt_tbl_info->ref_type == PIPE_TBL_REF_TYPE_DIRECT) {
    /* No state stored for directly referenced action data tables */
    return PIPE_SUCCESS;
  }
  UNUSED(profile_info);
  UNUSED(arg);
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_mgr_adt_move_list_t *ml = NULL;

  bf_dev_id_t dev_id = dev_info->dev_id;
  pipe_adt_tbl_hdl_t adt_tbl_hdl = adt_tbl_info->handle;
  ml = (pipe_mgr_adt_move_list_t *)adt_tbl_info->ha_move_list;

  if (ml) {
    uint32_t success_count = 0;
    rc = pipe_mgr_adt_hlp_restore_state(
        0, dev_id, adt_tbl_hdl, ml, &success_count, NULL);

    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in restoring ADT HLP state for tbl %s 0x%x dev %d err "
          "%s",
          __func__,
          __LINE__,
          adt_tbl_info->name,
          adt_tbl_hdl,
          dev_id,
          pipe_str_err(rc));
      return rc;
    }
  }
  return rc;
}

static pipe_status_t sel_tbl_llp_restore_state(
    rmt_dev_info_t *dev_info,
    rmt_dev_profile_info_t *profile_info,
    pipe_select_tbl_info_t *sel_tbl_info,
    void *arg) {
  UNUSED(profile_info);
  UNUSED(arg);
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_mgr_sel_move_list_t *ml = NULL;
  pipe_mgr_sel_move_list_t **mlp = NULL;

  bool phy_device_only =
      !pipe_mgr_is_device_virtual_dev_slave(dev_info->dev_id);
  if (phy_device_only || pipe_mgr_hitless_ha_issue_callbacks_from_llp) {
    mlp = &ml;
  }
  bf_dev_id_t dev_id = dev_info->dev_id;
  pipe_sel_tbl_hdl_t sel_tbl_hdl = sel_tbl_info->handle;
  rc = pipe_mgr_sel_llp_restore_state(dev_id, sel_tbl_hdl, mlp);

  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in restoring LLP state for sel tbl 0x%x, device id %d, "
        "err %s",
        __func__,
        __LINE__,
        sel_tbl_hdl,
        dev_id,
        pipe_str_err(rc));
    return rc;
  }

  if (!phy_device_only && pipe_mgr_hitless_ha_issue_callbacks_from_llp) {
    pipe_mgr_hitless_ha_save_ml(dev_id, sel_tbl_hdl, (void *)ml);
  }
  sel_tbl_info->ha_move_list = (void *)ml;
  return rc;
}

static pipe_status_t sel_tbl_hlp_restore_state(
    rmt_dev_info_t *dev_info,
    rmt_dev_profile_info_t *profile_info,
    pipe_select_tbl_info_t *sel_tbl_info,
    void *arg) {
  UNUSED(profile_info);
  UNUSED(arg);
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_mgr_sel_move_list_t *ml = NULL;

  bf_dev_id_t dev_id = dev_info->dev_id;
  pipe_sel_tbl_hdl_t sel_tbl_hdl = sel_tbl_info->handle;
  ml = (pipe_mgr_sel_move_list_t *)sel_tbl_info->ha_move_list;

  if (ml) {
    uint32_t success_count = 0;
    rc = pipe_mgr_sel_hlp_restore_state(
        0, dev_id, sel_tbl_hdl, ml, &success_count, NULL);

    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in restoring HLP state for sel tbl 0x%x, device id %d, "
          "err %s",
          __func__,
          __LINE__,
          sel_tbl_hdl,
          dev_id,
          pipe_str_err(rc));
      return rc;
    }
  }
  return rc;
}

pipe_status_t pipe_mgr_hitless_ha_complete_hw_read(pipe_sess_hdl_t sess_hdl,
                                                   bf_dev_id_t dev_id) {
  rmt_dev_info_t *dev_info = NULL;
  pipe_status_t rc;

  if (!(dev_info = pipe_mgr_get_dev_info(dev_id))) {
    return PIPE_OBJ_NOT_FOUND;
  }

  /*
   * - Complete all the Read block operations going on currently
   * - For each table,
   *   - Go through the LLP state restoration and create move
   *     lists if necessary
   *   - Process the move lists in the HLP
   */
  rc = pipe_mgr_drv_rd_blk_cmplt_all(sess_hdl, dev_id);
  if (rc != PIPE_SUCCESS) {
    LOG_CRIT(
        "%s:%d Couldn't complete read block operations on dev %d rc 0x%x(%s)",
        __func__,
        __LINE__,
        dev_id,
        rc,
        pipe_str_err(rc));
    return rc;
  }

  /*
   * LLP state restore from hardware must be done in the order
   * match --> selector --> action. This is because match entries include
   * action and selector virtual addresses, which are needed to find the
   * location of the action entry or selector in hardware. Similarly,
   * the selector itself holds the location of its action members.
   */
  rc = iterate_all_mat_tbls(
      dev_info, mat_tbl_llp_restore_state, NULL, NULL, __func__, __LINE__);
  if (rc != PIPE_SUCCESS) {
    LOG_CRIT(
        "%s:%d Error restoring match table llp state for dev %d rc 0x%x(%s)",
        __func__,
        __LINE__,
        dev_id,
        rc,
        pipe_str_err(rc));
    return rc;
  }

  rc = iterate_all_sel_tbls(
      dev_info, sel_tbl_llp_restore_state, NULL, NULL, __func__, __LINE__);
  if (rc != PIPE_SUCCESS) {
    LOG_CRIT(
        "%s:%d Error restoring selector table llp state for dev %d rc 0x%x(%s)",
        __func__,
        __LINE__,
        dev_id,
        rc,
        pipe_str_err(rc));
    return rc;
  }

  rc = iterate_all_adt_tbls(
      dev_info, adt_tbl_llp_restore_state, NULL, NULL, __func__, __LINE__);
  if (rc != PIPE_SUCCESS) {
    LOG_CRIT(
        "%s:%d Error restoring action table llp state for dev %d rc 0x%x(%s)",
        __func__,
        __LINE__,
        dev_id,
        rc,
        pipe_str_err(rc));
    return rc;
  }

  /*
   * HLP state restore should only happen in single-process mode.
   * Additionally, it should be done in the order
   * action --> selector --> match, due to the dependencies that exist
   * between these tables.
   */
  bool hlp_with_llp = !pipe_mgr_is_device_virtual_dev_slave(dev_info->dev_id);
  if (hlp_with_llp) {
    rc = iterate_all_adt_tbls(
        dev_info, adt_tbl_hlp_restore_state, NULL, NULL, __func__, __LINE__);
    if (rc != PIPE_SUCCESS) {
      LOG_CRIT(
          "%s:%d Error restoring action table hlp state for dev %d rc 0x%x(%s)",
          __func__,
          __LINE__,
          dev_id,
          rc,
          pipe_str_err(rc));
      return rc;
    }

    rc = iterate_all_sel_tbls(
        dev_info, sel_tbl_hlp_restore_state, NULL, NULL, __func__, __LINE__);
    if (rc != PIPE_SUCCESS) {
      LOG_CRIT(
          "%s:%d Error restoring selector table hlp state for dev %d rc "
          "0x%x(%s)",
          __func__,
          __LINE__,
          dev_id,
          rc,
          pipe_str_err(rc));
      return rc;
    }

    rc = iterate_all_mat_tbls(
        dev_info, mat_tbl_hlp_restore_state, NULL, NULL, __func__, __LINE__);
    if (rc != PIPE_SUCCESS) {
      LOG_CRIT(
          "%s:%d Error restoring match table hlp state for dev %d rc 0x%x(%s)",
          __func__,
          __LINE__,
          dev_id,
          rc,
          pipe_str_err(rc));
      return rc;
    }
  }

  return PIPE_SUCCESS;
}

struct mat_tbl_compute_delta_arg_t {
  pipe_sess_hdl_t sess_hdl;
};

static void pipe_mgr_tbl_cleanup_hlp_ha_state(bf_dev_id_t dev_id,
                                              pipe_mat_tbl_hdl_t mat_tbl_hdl) {
  switch (pipe_mgr_sm_tbl_owner(dev_id, mat_tbl_hdl)) {
    case PIPE_MGR_TBL_OWNER_EXM:
      pipe_mgr_exm_cleanup_hlp_ha_state(dev_id, mat_tbl_hdl);
      break;
    case PIPE_MGR_TBL_OWNER_TRN:
      pipe_mgr_tcam_cleanup_hlp_ha_state(dev_id, mat_tbl_hdl);
      break;
    case PIPE_MGR_TBL_OWNER_ALPM:
      pipe_mgr_alpm_cleanup_hlp_ha_state(dev_id, mat_tbl_hdl);
      break;
    case PIPE_MGR_TBL_OWNER_PHASE0:
      /* Not supported */
      break;
    case PIPE_MGR_TBL_OWNER_NO_KEY:
      /* Not supported */
      break;
    default:
      PIPE_MGR_ASSERT(0);
      break;
  }
  return;
}

static void pipe_mgr_tbl_cleanup_llp_ha_state(bf_dev_id_t dev_id,
                                              pipe_mat_tbl_hdl_t mat_tbl_hdl) {
  switch (pipe_mgr_sm_tbl_owner(dev_id, mat_tbl_hdl)) {
    case PIPE_MGR_TBL_OWNER_EXM:
      pipe_mgr_exm_cleanup_llp_ha_state(dev_id, mat_tbl_hdl);
      break;
    case PIPE_MGR_TBL_OWNER_TRN:
      pipe_mgr_tcam_cleanup_llp_ha_state(dev_id, mat_tbl_hdl);
      break;
    case PIPE_MGR_TBL_OWNER_ALPM:
      /* Not supported */
      break;
    case PIPE_MGR_TBL_OWNER_PHASE0:
      /* Not supported */
      break;
    case PIPE_MGR_TBL_OWNER_NO_KEY:
      /* Not supported */
      break;
    default:
      PIPE_MGR_ASSERT(0);
      break;
  }
  return;
}

static pipe_status_t mat_tbl_compute_delta_changes(
    rmt_dev_info_t *dev_info,
    rmt_dev_profile_info_t *profile_info,
    pipe_mat_tbl_info_t *mat_tbl_info,
    void *arg) {
  UNUSED(profile_info);
  /*
   * For each table
   * - process the delta changes with del/mod/add and create move-list entries
   * If virtual device, process the move-list and add ilists
   */

  pipe_status_t rc = PIPE_SUCCESS;
  struct mat_tbl_compute_delta_arg_t *argp =
      (struct mat_tbl_compute_delta_arg_t *)arg;
  pipe_sess_hdl_t sess_hdl = argp->sess_hdl;
  bf_dev_id_t dev_id = dev_info->dev_id;
  pipe_mat_tbl_hdl_t mat_tbl_handle = mat_tbl_info->handle;
  pipe_mgr_move_list_t *ml = NULL;

  /* If the device is a virtual device slave, nothing to be done here */
  if (pipe_mgr_is_device_virtual_dev_slave(dev_id)) {
    if (pipe_mgr_hitless_ha_issue_callbacks_from_llp) {
      ml = (pipe_mgr_move_list_t *)pipe_mgr_hitless_ha_get_ml(dev_id,
                                                              mat_tbl_handle);
      PIPE_MGR_ASSERT(ml);
      pipe_mgr_sm_issue_mat_cb(dev_id, mat_tbl_handle, ml);
    }
    return PIPE_SUCCESS;
  }

  bool virtual_dev = pipe_mgr_is_device_virtual(dev_id);

  /* If the table uses static entries, nothing to be done here */
  if (mat_tbl_info->static_entries) {
    rc = PIPE_SUCCESS;
    goto err_cleanup;
  }

  switch (pipe_mgr_sm_tbl_owner(dev_id, mat_tbl_handle)) {
    case PIPE_MGR_TBL_OWNER_EXM:
      rc = pipe_mgr_exm_hlp_compute_delta_changes(dev_id, mat_tbl_handle, &ml);
      break;
    case PIPE_MGR_TBL_OWNER_TRN:
      rc = pipe_mgr_tcam_hlp_compute_delta_changes(dev_id, mat_tbl_handle, &ml);
      break;
    case PIPE_MGR_TBL_OWNER_ALPM:
      rc = pipe_mgr_alpm_hlp_compute_delta_changes(dev_id, mat_tbl_handle, &ml);
      break;
    case PIPE_MGR_TBL_OWNER_PHASE0:
      rc = pipe_mgr_phase0_hlp_compute_delta_changes(
          dev_id, mat_tbl_handle, &ml);
      break;
    case PIPE_MGR_TBL_OWNER_NO_KEY:
      return PIPE_SUCCESS;
    default:
      PIPE_MGR_ASSERT(0);
      break;
  }

  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error computing delta changes for table %s 0x%x dev %d err %s",
        __func__,
        __LINE__,
        mat_tbl_info->name,
        mat_tbl_handle,
        dev_id,
        pipe_str_err(rc));
    goto err_cleanup;
  }

  /* call the applications callback */
  pipe_mgr_sm_issue_mat_cb(dev_id, mat_tbl_handle, ml);

  if (!virtual_dev) {
    uint32_t processed = 0;
    switch (pipe_mgr_sm_tbl_owner(dev_id, mat_tbl_handle)) {
      case PIPE_MGR_TBL_OWNER_EXM:
        rc = pipe_mgr_exm_tbl_process_move_list(
            sess_hdl, dev_id, mat_tbl_handle, ml, &processed);
        break;
      case PIPE_MGR_TBL_OWNER_TRN:
        rc = pipe_mgr_tcam_process_move_list(
            sess_hdl, dev_id, mat_tbl_handle, ml, &processed);
        break;
      case PIPE_MGR_TBL_OWNER_ALPM:
        rc = pipe_mgr_alpm_process_move_list(
            sess_hdl, dev_id, mat_tbl_handle, ml, &processed);
        break;
      case PIPE_MGR_TBL_OWNER_PHASE0:
        rc = pipe_mgr_phase0_ent_program(
            sess_hdl, dev_id, mat_tbl_handle, ml, &processed);
        break;
      case PIPE_MGR_TBL_OWNER_NO_KEY:
        return PIPE_SUCCESS;
      default:
        PIPE_MGR_ASSERT(0);
        break;
    }
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in processing move-list for tbl 0x%x, device id %d, err "
          "%s",
          __func__,
          __LINE__,
          mat_tbl_handle,
          dev_id,
          pipe_str_err(rc));
      goto err_cleanup;
    }
    free_move_list(&ml, true);
  }

err_cleanup:
  /* Cleanup HLP HA state for this table */
  pipe_mgr_tbl_cleanup_hlp_ha_state(dev_id, mat_tbl_handle);
  /* Cleanup LLP HA state for this table */
  if (!virtual_dev) {
    pipe_mgr_tbl_cleanup_llp_ha_state(dev_id, mat_tbl_handle);
  }
  free_move_list(&ml, false);
  return rc;
}

static pipe_status_t adt_tbl_compute_delta_changes(
    rmt_dev_info_t *dev_info,
    rmt_dev_profile_info_t *profile_info,
    pipe_adt_tbl_info_t *adt_tbl_info,
    void *arg) {
  UNUSED(profile_info);

  pipe_status_t rc = PIPE_SUCCESS;
  bf_dev_id_t dev_id = dev_info->dev_id;
  pipe_adt_tbl_hdl_t tbl_hdl = adt_tbl_info->handle;
  pipe_mgr_adt_move_list_t *ml = NULL;
  struct mat_tbl_compute_delta_arg_t *argp =
      (struct mat_tbl_compute_delta_arg_t *)arg;
  pipe_sess_hdl_t sess_hdl = argp->sess_hdl;

  if (pipe_mgr_is_device_virtual_dev_slave(dev_id)) {
    if (pipe_mgr_hitless_ha_issue_callbacks_from_llp) {
      ml = (pipe_mgr_adt_move_list_t *)pipe_mgr_hitless_ha_get_ml(dev_id,
                                                                  tbl_hdl);
      if (ml) {
        pipe_mgr_sm_issue_adt_cb(dev_id, tbl_hdl, ml);
      }
    }
    return PIPE_SUCCESS;
  }

  bool virtual_dev = pipe_mgr_is_device_virtual(dev_id);

  rc = pipe_mgr_adt_hlp_compute_delta_changes(dev_id, tbl_hdl, &ml);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in computing delta changes for action table 0x%x, device "
        "id %d, err %s",
        __func__,
        __LINE__,
        tbl_hdl,
        dev_id,
        pipe_str_err(rc));
    goto err_cleanup;
  }

  /* Call the applications callback */
  pipe_mgr_sm_issue_adt_cb(dev_id, tbl_hdl, ml);

  if (!virtual_dev) {
    uint32_t processed = 0;
    rc = pipe_adt_process_move_list(sess_hdl, dev_id, tbl_hdl, ml, &processed);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in processing Action data move list for tbl 0x%x, "
          "device id %d, err %s",
          __func__,
          __LINE__,
          tbl_hdl,
          dev_id,
          pipe_str_err(rc));
      return rc;
    }
  }

err_cleanup:
  /* Cleanup the ADT HLP HA state */
  pipe_mgr_adt_cleanup_hlp_ha_state(dev_id, tbl_hdl);
  if (!virtual_dev) {
    /* Cleanup ADT LLP HA state */
    pipe_mgr_adt_cleanup_llp_ha_state(dev_id, tbl_hdl);
  }
  free_adt_move_list(&ml);
  return rc;
}

static pipe_status_t sel_tbl_compute_delta_changes(
    rmt_dev_info_t *dev_info,
    rmt_dev_profile_info_t *profile_info,
    pipe_select_tbl_info_t *sel_tbl_info,
    void *arg) {
  UNUSED(profile_info);
  /*
   * For each table
   * - process the delta changes with del/mod/add and create move-list entries
   * If virtual device, process the move-list and add ilists
   */

  pipe_status_t rc = PIPE_SUCCESS;
  struct mat_tbl_compute_delta_arg_t *argp =
      (struct mat_tbl_compute_delta_arg_t *)arg;
  pipe_sess_hdl_t sess_hdl = argp->sess_hdl;
  bf_dev_id_t dev_id = dev_info->dev_id;
  pipe_sel_tbl_hdl_t tbl_hdl = sel_tbl_info->handle;
  pipe_mgr_sel_move_list_t *ml = NULL;

  if (pipe_mgr_is_device_virtual_dev_slave(dev_id)) {
    if (pipe_mgr_hitless_ha_issue_callbacks_from_llp) {
      ml = (pipe_mgr_sel_move_list_t *)pipe_mgr_hitless_ha_get_ml(dev_id,
                                                                  tbl_hdl);
      PIPE_MGR_ASSERT(ml);
      pipe_mgr_sm_issue_sel_cb(dev_id, tbl_hdl, ml);
    }
    return PIPE_SUCCESS;
  }

  bool virtual_dev = pipe_mgr_is_device_virtual(dev_id);

  rc = pipe_mgr_sel_hlp_compute_delta_changes(dev_id, tbl_hdl, &ml);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in computing selector delta changes for tbl_hdl 0x%x, "
        "device id %d, err %s",
        __func__,
        __LINE__,
        tbl_hdl,
        dev_id,
        pipe_str_err(rc));
    goto err_cleanup;
  }

  /* call the applications callback */
  pipe_mgr_sm_issue_sel_cb(dev_id, tbl_hdl, ml);

  if (!virtual_dev) {
    uint32_t processed = 0;
    rc = pipe_sel_process_move_list(sess_hdl, dev_id, tbl_hdl, ml, &processed);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in processing selector tbl move list for tbl 0x%x, "
          "device id %d, err %s",
          __func__,
          __LINE__,
          tbl_hdl,
          dev_id,
          pipe_str_err(rc));
      goto err_cleanup;
    }
  }

err_cleanup:
  /* Cleanup selector table HLP HA state */
  pipe_mgr_selector_cleanup_hlp_ha_state(dev_id, tbl_hdl, false);
  if (!virtual_dev) {
    /* Cleanup selector table LLP HA state */
    pipe_mgr_selector_cleanup_llp_ha_state(dev_id, tbl_hdl, false);
  }

  free_sel_move_list(&ml);
  return rc;
}

#if 0
static pipe_status_t stat_tbl_compute_delta_changes(
    rmt_dev_info_t *dev_info,
    rmt_dev_profile_info_t *profile_info,
    pipe_stat_tbl_info_t *stat_tbl_info,
    void *arg) {
  UNUSED(profile_info);
  UNUSED(arg);
  bf_dev_id_t dev_id = dev_info->dev_id;
  pipe_stat_tbl_hdl_t stat_tbl_hdl = stat_tbl_info->handle;
  pipe_status_t rc = PIPE_SUCCESS;

  rc = pipe_mgr_stat_mgr_reset_table(
      pipe_mgr_get_int_sess_hdl(), dev_id, stat_tbl_hdl);
  return rc;
}
#endif
pipe_status_t pipe_mgr_hitless_ha_get_reconc_report(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_handle,
    pipe_tbl_ha_reconc_report_t *ha_report) {
  UNUSED(sess_hdl);
  rmt_dev_info_t *dev_info = NULL;
  pipe_status_t rc = PIPE_SUCCESS;

  if (!(dev_info = pipe_mgr_get_dev_info(dev_tgt.device_id))) {
    return PIPE_OBJ_NOT_FOUND;
  }

  switch (pipe_mgr_sm_tbl_owner(dev_tgt.device_id, mat_tbl_handle)) {
    case PIPE_MGR_TBL_OWNER_EXM:
      rc =
          pipe_mgr_exm_get_ha_reconc_report(dev_tgt, mat_tbl_handle, ha_report);
      break;
    case PIPE_MGR_TBL_OWNER_TRN:
      rc = pipe_mgr_tcam_get_ha_reconc_report(
          dev_tgt, mat_tbl_handle, ha_report);
      break;
    case PIPE_MGR_TBL_OWNER_ALPM:
      rc = pipe_mgr_alpm_get_ha_reconc_report(
          dev_tgt, mat_tbl_handle, ha_report);
      break;
    case PIPE_MGR_TBL_OWNER_PHASE0:
      rc = pipe_mgr_phase0_get_ha_reconc_report(
          dev_tgt, mat_tbl_handle, ha_report);
      break;
    case PIPE_MGR_TBL_OWNER_NO_KEY:
      rc = PIPE_SUCCESS;
      break;
    default:
      PIPE_MGR_ASSERT(0);
      break;
  }

  return rc;
}

pipe_status_t pipe_mgr_hitless_ha_compute_delta_changes(
    pipe_sess_hdl_t sess_hdl, bf_dev_id_t dev_id) {
  UNUSED(sess_hdl);
  UNUSED(dev_id);
  rmt_dev_info_t *dev_info = NULL;
  pipe_status_t rc;

  if (!(dev_info = pipe_mgr_get_dev_info(dev_id))) {
    return PIPE_OBJ_NOT_FOUND;
  }

  struct mat_tbl_compute_delta_arg_t arg;
  arg.sess_hdl = sess_hdl;

  rc = iterate_all_adt_tbls(
      dev_info, adt_tbl_compute_delta_changes, NULL, &arg, __func__, __LINE__);
  if (rc != PIPE_SUCCESS) {
    LOG_CRIT(
        "%s:%d Error in initiating hw read for dev %d "
        "rc 0x%x(%s)",
        __func__,
        __LINE__,
        dev_id,
        rc,
        pipe_str_err(rc));
    return rc;
  }

  rc = iterate_all_sel_tbls(
      dev_info, sel_tbl_compute_delta_changes, NULL, &arg, __func__, __LINE__);
  if (rc != PIPE_SUCCESS) {
    LOG_CRIT(
        "%s:%d Error in computing delta changes for dev %d "
        "rc 0x%x(%s)",
        __func__,
        __LINE__,
        dev_id,
        rc,
        pipe_str_err(rc));
    return rc;
  }

  rc = iterate_all_mat_tbls(
      dev_info, mat_tbl_compute_delta_changes, NULL, &arg, __func__, __LINE__);
  if (rc != PIPE_SUCCESS) {
    LOG_CRIT(
        "%s:%d Error in initiating hw read for dev %d "
        "rc 0x%x(%s)",
        __func__,
        __LINE__,
        dev_id,
        rc,
        pipe_str_err(rc));
    return rc;
  }

#if 0
  /* not resetting counter tables in hitless HA,
     preserve the stats in  hw  for counter restoration
  */

  if (!pipe_mgr_is_device_virtual(dev_id)) {
    rc = iterate_all_stat_tbls(
        dev_info, stat_tbl_compute_delta_changes, &arg, __func__, __LINE__);

    if (rc != PIPE_SUCCESS) {
      LOG_CRIT(
          "%s:%d Error in computing stat tbl delta changes for dev %d "
          "rc 0x%x(%s)",
          __func__,
          __LINE__,
          dev_id,
          rc,
          pipe_str_err(rc));
      return rc;
    }
  }
#endif
  rc = pipe_mgr_mirror_ha_compute_delta_changes(sess_hdl, dev_info);
  if (rc != PIPE_SUCCESS) {
    LOG_CRIT(
        "%s:%d Error in computing mirror delta changes for dev %d "
        "rc 0x%x(%s)",
        __func__,
        __LINE__,
        dev_id,
        rc,
        pipe_str_err(rc));
    return rc;
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_hitless_ha_push_delta_changes(pipe_sess_hdl_t sess_hdl,
                                                     bf_dev_id_t dev_id) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return PIPE_INVALID_ARG;

  /* Get out of HA so we can write config. */
  pipe_mgr_init_mode_reset(dev_id);

  /* Physical devices will start DMA, enable all operations on PBUS now. */
  if (!pipe_mgr_is_device_virtual(dev_id)) {
    pipe_mgr_set_pbus_weights(dev_info, 1 /* weight */);
  }

  /* Unlock of the device pushes ilist of the internal session handle and
   * all other application session handles.  */
  pipe_mgr_unlock_device_internal(dev_id, BF_DEV_WARM_INIT_HITLESS);

  if (!pipe_mgr_is_device_virtual(dev_id)) {
    /* Iterate over all stful tables and form ilist to push stful spec.
     * For hitless HA we "start-over" for stateful stuff, meaning we
     * re-initialize the stateful memory with provided specs during config
     * replay or the initial value as specified in the P4.  */
    pipe_status_t rc = PIPE_SUCCESS;
    rc = iterate_all_stful_tbls(dev_info,
                                pipe_mgr_stful_download_specs_from_shadow,
                                (void *)&sess_hdl,
                                __func__,
                                __LINE__);
    if (rc != PIPE_SUCCESS) {
      LOG_CRIT("%s:%d Error in pushing stateful specs to for dev %d rc %s",
               __func__,
               __LINE__,
               dev_id,
               pipe_str_err(rc));
      return rc;
    }

    rc = iterate_all_meter_tbls(dev_info,
                                pipe_mgr_meter_download_specs_from_shadow,
                                (void *)&sess_hdl,
                                __func__,
                                __LINE__);
    if (rc != PIPE_SUCCESS) {
      LOG_CRIT("%s:%d Error in pushing meter specs to for dev %d rc %s",
               __func__,
               __LINE__,
               dev_id,
               pipe_str_err(rc));
      return rc;
    }
  }

  pipe_mgr_complete_operations(sess_hdl);

  return PIPE_SUCCESS;
}

static inline size_t key_sz_f(pipe_tbl_match_spec_t *match_spec) {
  size_t hash_key_sz =
      sizeof(pipe_tbl_match_spec_t) + (match_spec->num_match_bytes << 1);
  return hash_key_sz ? hash_key_sz : 1;
}

static uint8_t *form_hash_key(uint8_t *hash_key,
                              pipe_tbl_match_spec_t *match_spec) {
  pipe_tbl_match_spec_t *ms_hash = (pipe_tbl_match_spec_t *)hash_key;

  if (!ms_hash) {
    size_t hash_key_sz = key_sz_f(match_spec);
    ms_hash = (pipe_tbl_match_spec_t *)PIPE_MGR_CALLOC(1, hash_key_sz);
    if (!ms_hash) return NULL;
  }

  ms_hash->match_value_bits = (uint8_t *)(ms_hash + 1);
  ms_hash->match_mask_bits =
      ms_hash->match_value_bits + match_spec->num_match_bytes;
  ms_hash = pipe_mgr_tbl_copy_match_spec(ms_hash, match_spec);
  if (!ms_hash) return NULL;

  // Zero out the priority for the hash key
  ms_hash->priority = 0;
  // Internal - zero out
  ms_hash->version_bits = 0;
  /* Set the pointers to zero so they do not affect the hash value.  The
   * match_value_bits and match_mask_bits arrays are still in memory immediately
   * after the match_spec struct. */
  ms_hash->match_value_bits = NULL;
  ms_hash->match_mask_bits = NULL;

  return (uint8_t *)ms_hash;
}

static void form_adt_hash_key(uint8_t *hash_key,
                              pipe_action_data_spec_t *act_data_spec,
                              pipe_act_fn_hdl_t act_fn_hdl,
                              bool sharable) {
  PIPE_MGR_MEMCPY(hash_key,
                  act_data_spec->action_data_bits,
                  sizeof(uint8_t) * act_data_spec->num_action_data_bytes);
  PIPE_MGR_MEMCPY(hash_key + act_data_spec->num_action_data_bytes,
                  &act_fn_hdl,
                  sizeof(pipe_act_fn_hdl_t));
  hash_key += sizeof(uint8_t) * act_data_spec->num_action_data_bytes;
  hash_key += sizeof(pipe_act_fn_hdl_t);
  *hash_key = sharable ? 1 : 0;
  return;
}

void destroy_hash_key(uint8_t *hash_key) { PIPE_MGR_FREE(hash_key); }

static pipe_mgr_ha_entry_t *form_entry(pipe_ent_hdl_t entry_hdl,
                                       pipe_tbl_match_spec_t *match_spec,
                                       pipe_act_fn_hdl_t act_fn_hdl,
                                       pipe_action_spec_t *action_spec,
                                       pipe_mgr_move_list_t *mn,
                                       uint32_t ttl) {
  pipe_mgr_ha_entry_t *entry = PIPE_MGR_CALLOC(1, sizeof *entry);
  if (!entry) {
    return entry;
  }

  entry->ha_state = INVALID_MATCH;
  if (mn) {
    entry->mn = mn;
  } else {
    entry->match_spec = pipe_mgr_tbl_copy_match_spec(NULL, match_spec);
    entry->entry_hdl = entry_hdl;
    entry->act_fn_hdl = act_fn_hdl;
    entry->action_spec = pipe_mgr_tbl_copy_action_spec(NULL, action_spec);
    entry->ttl = ttl;
  }

  return entry;
}

static void destroy_entry(pipe_mgr_ha_entry_t **entry_p) {
  if (!entry_p || !*entry_p) {
    return;
  }

  pipe_mgr_ha_entry_t *entry = *entry_p;

  pipe_mgr_tbl_destroy_match_spec(&entry->match_spec);
  pipe_mgr_tbl_destroy_action_spec(&entry->action_spec);
  PIPE_MGR_FREE(entry);
  *entry_p = NULL;
}

static int spec_cmp_fn(const void *k1, const void *k2) {
  pipe_mgr_ha_entry_t *matching_entries_head =
      (pipe_mgr_ha_entry_t *)bf_hashtbl_get_cmp_data(k2);

  /* Always skip the dummy entry at the head. */
  pipe_mgr_ha_entry_t *entry = matching_entries_head->n;
  /* After entry gets replayed once it gets removed from the list.
   * It is possible there are no entries in the list, hence need to check */
  if (!entry) return 1;
  pipe_tbl_match_spec_t *compare_spec = (pipe_tbl_match_spec_t *)k1;

  compare_spec->match_value_bits = (uint8_t *)(compare_spec + 1);
  compare_spec->match_mask_bits =
      compare_spec->match_value_bits + compare_spec->num_match_bytes;

  /* Compare the match specs k1 and entry */
  pipe_tbl_match_spec_t *ms =
      entry->mn ? unpack_mat_ent_data_ms(entry->mn->data) : entry->match_spec;

  int d = pipe_mgr_tbl_compare_match_specs(compare_spec, ms);

  compare_spec->match_value_bits = NULL;
  compare_spec->match_mask_bits = NULL;

  return d;
}

int adt_spec_cmp_fn(const void *k1, const void *k2) {
  int ret = 0;
  pipe_mgr_ha_entry_t *ha_entry =
      (pipe_mgr_ha_entry_t *)bf_hashtbl_get_cmp_data(k2);
  uint16_t num_action_data_bytes =
      ha_entry->action_spec->act_data.num_action_data_bytes;
  uint8_t *compare_spec = (uint8_t *)k1;
  pipe_act_fn_hdl_t compare_act_fn_hdl =
      *(pipe_act_fn_hdl_t *)((uint8_t *)k1 + num_action_data_bytes);
  ret = memcmp(compare_spec,
               ha_entry->action_spec->act_data.action_data_bits,
               num_action_data_bytes);
  if (ret) {
    return ret;
  }
  if (compare_act_fn_hdl != ha_entry->act_fn_hdl) {
    return -1;
  }
  return 0;
}

static pipe_status_t spec_map_init(pipe_mgr_spec_map_t *spec_map,
                                   size_t key_sz) {
  uint32_t seed = 0xba2ef007;

  spec_map->spec_htbl =
      (bf_hashtable_t *)PIPE_MGR_CALLOC(1, sizeof(bf_hashtable_t));

  bf_hashtbl_sts_t sts;
  sts = bf_hashtbl_init(spec_map->spec_htbl,
                        spec_cmp_fn,
                        NULL,
                        key_sz,
                        sizeof(pipe_mgr_ha_entry_t),
                        seed);
  if (sts != BF_HASHTBL_OK) {
    LOG_ERROR("%s:%d Error initializing spec map hash table sts=0x%x",
              __func__,
              __LINE__,
              sts);
    return PIPE_UNEXPECTED;
  }
  return PIPE_SUCCESS;
}

static pipe_status_t adt_spec_map_init(pipe_mgr_spec_map_t *spec_map,
                                       size_t key_sz) {
  uint32_t seed = 0xba2ef007;

  spec_map->spec_htbl =
      (bf_hashtable_t *)PIPE_MGR_CALLOC(1, sizeof(bf_hashtable_t));

  bf_hashtbl_sts_t sts;
  sts = bf_hashtbl_init(spec_map->spec_htbl,
                        adt_spec_cmp_fn,
                        NULL,
                        key_sz,
                        sizeof(pipe_mgr_ha_entry_t),
                        seed);
  if (sts != BF_HASHTBL_OK) {
    LOG_ERROR("%s:%d Error initializing spec map hash table sts=0x%x",
              __func__,
              __LINE__,
              sts);
    return PIPE_UNEXPECTED;
  }
  return PIPE_SUCCESS;
}

void pipe_mgr_hitless_ha_delete_spec_map(pipe_mgr_spec_map_t *spec_map) {
  bf_hashtbl_delete(spec_map->spec_htbl);
  PIPE_MGR_FREE(spec_map->spec_htbl);
  pipe_mgr_ha_entry_t *entry;
  while (spec_map->full_match_list) {
    entry = spec_map->full_match_list;
    BF_LIST_DLL_REM(spec_map->full_match_list, entry, np, pp);
    if (entry->mn) {
      free_one_move_list_node_and_data(&entry->mn, true);
    }
    destroy_entry(&entry->p);
    destroy_entry(&entry);
  }
  while (spec_map->to_delete_list) {
    entry = spec_map->to_delete_list;
    BF_LIST_DLL_REM(spec_map->to_delete_list, entry, np, pp);
    if (entry->mn) {
      free_one_move_list_node_and_data(&entry->mn, true);
    }
    destroy_entry(&entry->p);
    destroy_entry(&entry);
  }
  while (spec_map->to_modify_list) {
    entry = spec_map->to_modify_list;
    BF_LIST_DLL_REM(spec_map->to_modify_list, entry, np, pp);
    if (entry->mn) {
      free_one_move_list_node_and_data(&entry->mn, true);
    }
    destroy_entry(&entry->p);
    destroy_entry(&entry);
  }
  while (spec_map->to_add_list) {
    entry = spec_map->to_add_list;
    BF_LIST_DLL_REM(spec_map->to_add_list, entry, np, pp);
    if (entry->mn) {
      free_one_move_list_node_and_data(&entry->mn, true);
    }
    destroy_entry(&entry->p);
    destroy_entry(&entry);
  }
}

/* Add a new spec to entry-hdl mapping.
 */
pipe_status_t pipe_mgr_hitless_ha_new_spec(pipe_mgr_spec_map_t *spec_map,
                                           pipe_mgr_move_list_t *move_node) {
  /* Compute a hash and check if it is already present in the spec_map.
   * If present, append the entry hdl
   */
  pipe_tbl_match_spec_t *match_spec;
  pipe_mat_ent_hdl_t entry_hdl;
  pipe_status_t rc = PIPE_SUCCESS;

  if (!spec_map || !move_node) {
    LOG_ERROR("%s:%d Bad args spec_map=%p move_node=%p",
              __func__,
              __LINE__,
              (void *)spec_map,
              (void *)move_node);
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  match_spec = unpack_mat_ent_data_ms(move_node->data);
  entry_hdl = move_node->entry_hdl;

  if (!spec_map->spec_htbl) {
    rc = spec_map_init(spec_map, key_sz_f(match_spec));
    if (rc != PIPE_SUCCESS) {
      return rc;
    }
  }

  uint8_t hash_key[key_sz_f(match_spec)];
  PIPE_MGR_MEMSET(hash_key, 0, key_sz_f(match_spec));
  form_hash_key(hash_key, match_spec);

  pipe_mgr_ha_entry_t *matching_entries_head;

  /* hash_key derived out of match-spec */
  /* entry_state derived out of act_fn_hdl and action_spec */
  matching_entries_head = bf_hashtbl_search(spec_map->spec_htbl, hash_key);
  if (!matching_entries_head) {
    pipe_mgr_ha_entry_t *dummy_entry = form_entry(0, NULL, 0, NULL, NULL, 0);
    if (!dummy_entry) {
      LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
    BF_LIST_DLL_AP(matching_entries_head, dummy_entry, n, p);

    bf_hashtbl_sts_t sts;
    sts =
        bf_hashtbl_insert(spec_map->spec_htbl, matching_entries_head, hash_key);
    if (sts != BF_HASHTBL_OK) {
      LOG_ERROR("%s:%d Error adding entry 0x%x to spec-map sts 0x%x",
                __func__,
                __LINE__,
                entry_hdl,
                sts);
      rc = PIPE_UNEXPECTED;
      goto cleanup;
    }
  }

  pipe_mgr_ha_entry_t *entry;
  entry = form_entry(entry_hdl, NULL, 0, NULL, move_node, 0);
  BF_LIST_DLL_AP(matching_entries_head, entry, n, p);
  BF_LIST_DLL_AP(spec_map->to_delete_list, entry, np, pp);

cleanup:
  return rc;
}

pipe_status_t pipe_mgr_hitless_ha_new_adt_spec(
    pipe_mgr_spec_map_t *spec_map,
    pipe_mgr_adt_move_list_t *move_node,
    uint8_t key_sz) {
  pipe_status_t rc = PIPE_SUCCESS;
  bf_hashtbl_sts_t sts = BF_HASHTBL_OK;
  pipe_action_data_spec_t *act_data_spec =
      unpack_adt_ent_data_ad(move_node->data);
  pipe_act_fn_hdl_t act_fn_hdl = unpack_adt_ent_data_afun_hdl(move_node->data);

  if (!spec_map->spec_htbl) {
    /* Last byte is to store the information if the entry was sharable.
     * Sharable entries are first to be replayed.
     * Non-sharable entries can be reconciled later even if there is an adt
     * handle mismatch.
     */
    rc = adt_spec_map_init(spec_map, key_sz + sizeof(pipe_act_fn_hdl_t) + 1);
    if (rc != PIPE_SUCCESS) {
      return rc;
    }
  }
  uint8_t hash_key[key_sz + sizeof(pipe_act_fn_hdl_t) + 1];
  PIPE_MGR_MEMSET(hash_key, 0, key_sz + sizeof(pipe_act_fn_hdl_t) + 1);
  form_adt_hash_key(hash_key, act_data_spec, act_fn_hdl, move_node->sharable);
  pipe_action_spec_t action_spec = {0};
  action_spec.act_data = *act_data_spec;
  pipe_mgr_ha_entry_t *ha_entry =
      form_entry(move_node->entry_hdl, NULL, act_fn_hdl, &action_spec, NULL, 0);
  sts = bf_hashtbl_insert(spec_map->spec_htbl, ha_entry, hash_key);
  if (sts != BF_HASHTBL_OK) {
    LOG_ERROR("%s:%d Error adding adt entry hdl 0x%x to spec map sts 0x%x",
              __func__,
              __LINE__,
              move_node->entry_hdl,
              sts);
    rc = PIPE_UNEXPECTED;
    goto cleanup;
  }

cleanup:
  return rc;
}

static bool compare_resources(pipe_action_spec_t *action_spec1,
                              pipe_action_spec_t *action_spec2) {
  pipe_res_spec_t *resources1 = action_spec1->resources;
  pipe_res_spec_t *resources2 = action_spec2->resources;
  bool stats_present[2] = {0};
  pipe_res_idx_t stats_idx[2] = {0};
  pipe_res_idx_t stful_idx[2] = {0};
  pipe_res_idx_t meters_idx[2] = {0};
  bool meters_present[2] = {0};
  bool stful_present[2] = {0};

  int i = 0;
  for (i = 0; i < action_spec1->resource_count; i++) {
    switch (PIPE_GET_HDL_TYPE(resources1[i].tbl_hdl)) {
      case PIPE_HDL_TYPE_STAT_TBL:
        if (resources1[i].tag == PIPE_RES_ACTION_TAG_NO_CHANGE ||
            resources1[i].tag == PIPE_RES_ACTION_TAG_ATTACHED) {
          stats_present[0] = true;
          stats_idx[0] = resources1[i].tbl_idx;
        }
        break;
      case PIPE_HDL_TYPE_STFUL_TBL:
        if (resources1[i].tag == PIPE_RES_ACTION_TAG_NO_CHANGE ||
            resources1[i].tag == PIPE_RES_ACTION_TAG_ATTACHED) {
          stful_present[0] = true;
          stful_idx[0] = resources1[i].tbl_idx;
        }
        break;
      case PIPE_HDL_TYPE_METER_TBL:
        if (resources1[i].tag == PIPE_RES_ACTION_TAG_NO_CHANGE ||
            resources1[i].tag == PIPE_RES_ACTION_TAG_ATTACHED) {
          meters_present[0] = true;
          meters_idx[0] = resources1[i].tbl_idx;
        }
        break;
      default:
        PIPE_MGR_ASSERT(0);
        break;
    }
  }
  for (i = 0; i < action_spec2->resource_count; i++) {
    switch (PIPE_GET_HDL_TYPE(resources2[i].tbl_hdl)) {
      case PIPE_HDL_TYPE_STAT_TBL:
        if (resources2[i].tag == PIPE_RES_ACTION_TAG_NO_CHANGE ||
            resources2[i].tag == PIPE_RES_ACTION_TAG_ATTACHED) {
          stats_present[1] = true;
          stats_idx[1] = resources2[i].tbl_idx;
        }
        break;
      case PIPE_HDL_TYPE_STFUL_TBL:
        if (resources2[i].tag == PIPE_RES_ACTION_TAG_NO_CHANGE ||
            resources2[i].tag == PIPE_RES_ACTION_TAG_ATTACHED) {
          stful_present[1] = true;
          stful_idx[1] = resources2[i].tbl_idx;
        }
        break;
      case PIPE_HDL_TYPE_METER_TBL:
        if (resources2[i].tag == PIPE_RES_ACTION_TAG_NO_CHANGE ||
            resources2[i].tag == PIPE_RES_ACTION_TAG_ATTACHED) {
          meters_present[1] = true;
          meters_idx[1] = resources2[i].tbl_idx;
        }
        break;
      default:
        PIPE_MGR_ASSERT(0);
        break;
    }
  }
  if (stats_present[0] && stats_present[1]) {
    if (stats_idx[0] != stats_idx[1]) {
      return false;
    }
  } else if (stats_present[0] != stats_present[1]) {
    return false;
  }

  if (stful_present[0] && stful_present[1]) {
    if (stful_idx[0] != stful_idx[1]) {
      return false;
    }
  } else if (stful_present[0] != stful_present[1]) {
    return false;
  }

/* Do not compare meters, we lose the PFE bit when meter is
   programmed in the asic
*/
#if 0
  if (meters_present[0] && meters_present[1]) {
    if (meters_idx[0] != meters_idx[1]) {
      return false;
    }
  } else if (meters_present[0] != meters_present[1]) {
    return false;
  }
#else
  (void)meters_present;
  (void)meters_idx;
#endif

  return true;
}

bool handle_direct_resources(pipe_mat_tbl_info_t *mat_tbl_info) {
  unsigned i = 0;
  for (i = 0; i < mat_tbl_info->num_stful_tbl_refs; i++) {
    if (mat_tbl_info->stful_tbl_ref[i].ref_type == PIPE_TBL_REF_TYPE_DIRECT) {
      return false;
    }
  }
  return true;
}

#ifdef BF_HITLESS_HA_DEBUG
struct dump_spec_map_arg {
  bf_dev_id_t dev_id;
  profile_id_t prof_id;
  pipe_mat_tbl_hdl_t tbl_hdl;
  pipe_mgr_spec_map_t *spec_map;
};
void dump_spec_map(void *arg, void *obj) {
  if (!arg || !obj) return;
  pipe_mgr_ha_entry_t *entry = bf_hashtbl_get_cmp_data(obj);
  struct dump_spec_map_arg *a = arg;
  if (!entry) return;
  LOG_ERROR("Spec Map Node Start");
  for (; entry; entry = entry->n) {
    if (!entry->mn) {
      continue;
    }
    pipe_tbl_match_spec_t *match_spec = unpack_mat_ent_data_ms(entry->mn->data);
    pipe_action_spec_t *action_spec = unpack_mat_ent_data_as(entry->mn->data);
    pipe_act_fn_hdl_t act_fn_hdl =
        unpack_mat_ent_data_afun_hdl(entry->mn->data);
    if (match_spec) {
      pipe_mgr_entry_format_log_match_spec(
          a->dev_id, BF_LOG_ERR, a->prof_id, a->tbl_hdl, match_spec);
    } else {
      LOG_ERROR(" No MS");
    }
    if (action_spec) {
      pipe_mgr_entry_format_log_action_spec(
          a->dev_id, BF_LOG_ERR, a->prof_id, action_spec, act_fn_hdl);
    } else {
      LOG_ERROR(" No AS");
    }
  }
  LOG_ERROR("Spec Map Node End");
}
static void log_spec_map(pipe_mgr_spec_map_t *spec_map) {
  profile_id_t prof_id = pipe_mgr_get_tbl_profile(
      spec_map->dev_tgt.device_id, spec_map->mat_tbl_hdl, __func__, __LINE__);
  struct dump_spec_map_arg arg;
  arg.dev_id = spec_map->dev_tgt.device_id;
  arg.prof_id = prof_id;
  arg.tbl_hdl = spec_map->mat_tbl_hdl;
  arg.spec_map = spec_map;
  LOG_ERROR("SpecMap Dev %d Pipe %x Tbl 0x%x",
            spec_map->dev_tgt.device_id,
            spec_map->dev_tgt.dev_pipe_id,
            spec_map->mat_tbl_hdl);
  bf_hashtbl_foreach_fn(spec_map->spec_htbl, dump_spec_map, &arg);
}
#endif
/* Lookup a spec and return matching entry handle - caller has to compare the
 * new_entry_hdl and *entry_hdl_p and free if new_entry_hdl if not same
 */
pipe_status_t pipe_mgr_hitless_ha_lookup_spec(pipe_mgr_spec_map_t *spec_map,
                                              pipe_tbl_match_spec_t *match_spec,
                                              pipe_action_spec_t *action_spec,
                                              pipe_act_fn_hdl_t act_fn_hdl,
                                              pipe_ent_hdl_t new_entry_hdl,
                                              pipe_ent_hdl_t *entry_hdl_p,
                                              uint32_t ttl) {
  pipe_mgr_ha_entry_t *matching_entries_head, *matching_entries = NULL;
  pipe_mgr_hitless_ent_state_e ha_state = INVALID_MATCH;

  if (!match_spec) return PIPE_INVALID_ARG;
  {
    uint8_t hash_key[key_sz_f(match_spec)];

    PIPE_MGR_MEMSET(hash_key, 0, key_sz_f(match_spec));
    form_hash_key(hash_key, match_spec);

    matching_entries_head = bf_hashtbl_search(spec_map->spec_htbl, hash_key);
  }
  /* There is always a dummy node at the head of the list, skip it if we found a
   * match. */
  if (matching_entries_head) {
    matching_entries = matching_entries_head->n;
  }

  /* We have a match based on match-spec but now check the rest of the
   * params to ensure consistency - like action-fn-hdl, action-spec etc.
   * - In case of TCAM, also verify that the entries priority matches with the
   *   existing placed entries
   */
  pipe_mgr_ha_entry_t *matching_entry = NULL;
  pipe_mgr_ha_entry_t *entry = NULL;
  pipe_mgr_ha_entry_t *prev_entry = NULL;
  pipe_mgr_hitless_ent_state_e l_ha_state = INVALID_MATCH;
  for (entry = matching_entries; entry; entry = entry->n) {
    if ((ha_state == INVALID_MATCH) ||
        ((l_ha_state < ha_state) && (l_ha_state != INVALID_MATCH))) {
      ha_state = l_ha_state;
      matching_entry = prev_entry;
    }
    prev_entry = entry;
    l_ha_state = INVALID_MATCH;

    if (entry->ha_state != INVALID_MATCH) {
      continue;
    }

    /* At least the action function handle should match */
    if (unpack_mat_ent_data_afun_hdl(entry->mn->data) != act_fn_hdl) {
      l_ha_state = ACTION_DATA_MISS;
      continue;
    }

    if (unpack_mat_ent_data_as(entry->mn->data)->pipe_action_datatype_bmap !=
        action_spec->pipe_action_datatype_bmap) {
      l_ha_state = ACTION_DATA_MISS;
      continue;
    }

    if (IS_ACTION_SPEC_ACT_DATA(action_spec)) {
      /* Only in case of action data spec, the specs can be compared here.
       * The adt and sel-grp handles will be different in replay vs read
       */
      if (pipe_mgr_tbl_compare_action_data_specs(
              &unpack_mat_ent_data_as(entry->mn->data)->act_data,
              &action_spec->act_data)) {
        l_ha_state = ACTION_DATA_DIRTY;
        continue;
      }
    } else if (IS_ACTION_SPEC_ACT_DATA_HDL(action_spec)) {
      /* Check if the ADT entry handle in the replayed action_spec
       * is present in the logical_action_idx pointed to by the entry's
       * action_spec/move-node
       */

      if (pipe_mgr_adt_is_loc_occupied()) {
        l_ha_state = ACTION_DATA_MISS;
        continue;
      }

      if (action_spec->adt_ent_hdl !=
          unpack_mat_ent_data_as(entry->mn->data)->adt_ent_hdl) {
        l_ha_state = ACTION_HDL_DIRTY;
        continue;
      }

      if (!pipe_mgr_adt_is_hdl_present_at_loc()) {
        l_ha_state = ACTION_DATA_DIRTY;
        continue;
      }
    }
    /* Compare the resources: Any mismatch will lead to a modify */
    if (!compare_resources(action_spec,
                           unpack_mat_ent_data_as(entry->mn->data))) {
      /* Lets call this ACTION_DATA_DIRTY */
      l_ha_state = ACTION_DATA_DIRTY;
      continue;
    }
    /* Compare direct addressed resources such as STFUL and METERs which need
     * to be written fresh during hitless HA.
     */
    if (!handle_direct_resources(spec_map->tbl_info)) {
      l_ha_state = RESOURCE_MISMATCH;
      continue;
    }

    /* We have a full match. No need to check further */
    l_ha_state = FULL_MATCH;
    break;
  }

  if ((ha_state == INVALID_MATCH) || (l_ha_state < ha_state)) {
    ha_state = l_ha_state;
    matching_entry = prev_entry;
  }

  if (matching_entry) {
    BF_LIST_DLL_REM(spec_map->to_delete_list, matching_entry, np, pp);

    if (ha_state != FULL_MATCH) {
      if ((ha_state != RESOURCE_MISMATCH) && (ha_state != ACTION_DATA_DIRTY)) {
        LOG_ERROR(
            "%s:%d A Match entry for tbl 0x%x, device %d pipe %x did not match "
            "any existing entries before hitless HA. Currently not supported",
            __func__,
            __LINE__,
            spec_map->mat_tbl_hdl,
            spec_map->dev_tgt.device_id,
            spec_map->dev_tgt.dev_pipe_id);
        goto dump_spec_on_error;
      }
      /* Save the new Actions */
      matching_entry->action_spec =
          pipe_mgr_tbl_copy_action_spec(NULL, action_spec);
      matching_entry->act_fn_hdl = act_fn_hdl;

      BF_LIST_DLL_AP(spec_map->to_modify_list, matching_entry, np, pp);
      if (IS_ACTION_SPEC_ACT_DATA_HDL(action_spec)) {
        /* VK Activate the ADT entry handle at the given logical action index */
      }
    } else {
      BF_LIST_DLL_AP(spec_map->full_match_list, matching_entry, np, pp);
    }

    LOG_DBG("HA match state %s", pipe_mgr_hitless_ent_state_name(ha_state));
    /* Update the priority and TTL values */
    set_mat_ent_data_priority(matching_entry->mn->data, match_spec->priority);
    set_mat_ent_data_ttl(matching_entry->mn->data, ttl);
    matching_entry->ha_state = ha_state;
    *entry_hdl_p = matching_entry->mn->entry_hdl;

    /* Since we have modifed the matching entry (we set entry priority above) we
     * will remove it and free it, from the list of candidate entries.  We do
     * this so that we do not match another entry against it but also because
     * the entire chain of entries on this hash key could become unmatchable.
     * The hash table's compare-key function does a match spec comparison which
     * includes the entry priority which is assumed to be zero for lookups but
     * may no longer be zero since we modified it. */
    BF_LIST_DLL_REM(matching_entries_head, matching_entry, n, p);
  } else {
    /* If an entry does not exist with the given spec, add the entry into a
     * to_add_list and return a new handle
     */
    if (!pipe_mgr_mat_tbl_is_hash_action(spec_map->dev_tgt.device_id,
                                         spec_map->mat_tbl_hdl)) {
      /* As of now, this delta is only supported for hash action tables */
      LOG_ERROR(
          "%s:%d A Match entry for tbl 0x%x dev %d pipe %x did not match any "
          "entries existing before hitless HA. Currently not supported",
          __func__,
          __LINE__,
          spec_map->mat_tbl_hdl,
          spec_map->dev_tgt.device_id,
          spec_map->dev_tgt.dev_pipe_id);
      goto dump_spec_on_error;
    }
    LOG_DBG("New Entry");

    entry = form_entry(
        new_entry_hdl, match_spec, act_fn_hdl, action_spec, NULL, ttl);

    BF_LIST_DLL_AP(spec_map->to_add_list, entry, np, pp);
    /* Return the new entry handle - indicates to the caller that the new handle
     * is in use
     */
    *entry_hdl_p = new_entry_hdl;
  }

  return PIPE_SUCCESS;
dump_spec_on_error : {
  profile_id_t prof_id = pipe_mgr_get_tbl_profile(
      spec_map->dev_tgt.device_id, spec_map->mat_tbl_hdl, __func__, __LINE__);
  if (prof_id != -1) {
    pipe_mgr_entry_format_log_match_spec(spec_map->dev_tgt.device_id,
                                         BF_LOG_ERR,
                                         prof_id,
                                         spec_map->mat_tbl_hdl,
                                         match_spec);
    pipe_mgr_entry_format_log_action_spec(spec_map->dev_tgt.device_id,
                                          BF_LOG_ERR,
                                          prof_id,
                                          action_spec,
                                          act_fn_hdl);
  }
#ifdef BF_HITLESS_HA_DEBUG
  log_spec_map(spec_map);
#endif
  PIPE_MGR_DBGCHK(0);
  return PIPE_NOT_SUPPORTED;
}
}

pipe_status_t pipe_mgr_hitless_ha_lookup_adt_spec(
    pipe_mgr_spec_map_t *spec_map,
    pipe_action_data_spec_t *act_data_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_ent_hdl_t new_entry_hdl,
    pipe_ent_hdl_t *entry_hdl_p,
    uint32_t key_sz) {
  pipe_mgr_ha_entry_t *ha_entry = NULL;
  uint8_t hash_key[key_sz + sizeof(pipe_act_fn_hdl_t) + 1];
  PIPE_MGR_MEMSET(
      hash_key, 0, sizeof(uint8_t) * (key_sz + sizeof(pipe_act_fn_hdl_t)));

  /* First try to remove sharable entry. */
  form_adt_hash_key(hash_key, act_data_spec, act_fn_hdl, true);
  ha_entry = bf_hashtbl_get_remove(spec_map->spec_htbl, hash_key);
  if (ha_entry) goto cleanup;
  form_adt_hash_key(hash_key, act_data_spec, act_fn_hdl, false);
  ha_entry = bf_hashtbl_get_remove(spec_map->spec_htbl, hash_key);
  if (!ha_entry) {
    pipe_action_spec_t action_spec = {0};
    action_spec.act_data = *act_data_spec;
    /* Add it to the to_add_list */
    ha_entry =
        form_entry(new_entry_hdl, NULL, act_fn_hdl, &action_spec, NULL, 0);
    BF_LIST_DLL_AP(spec_map->to_add_list, ha_entry, np, pp);
    /* Return the new entry hdl */
    *entry_hdl_p = new_entry_hdl;
    return PIPE_OBJ_NOT_FOUND;
  }
cleanup:
  *entry_hdl_p = ha_entry->entry_hdl;
  /* This entry should not be matched again. */
  destroy_entry(&ha_entry);
  return PIPE_SUCCESS;
}

static bool updates_needed_for_full_match_entries(pipe_mat_tbl_info_t *tbl_info,
                                                  bf_dev_id_t device_id) {
  if (tbl_info->match_type == TERNARY_MATCH ||
      tbl_info->match_type == ATCAM_MATCH ||
      tbl_info->match_type == ALPM_MATCH) {
    return true;
  }
  if (pipe_mgr_mat_tbl_has_idle(device_id, tbl_info->handle)) {
    return true;
  }
  return false;
}

static void generate_ha_reconc_report(pipe_mgr_spec_map_t *spec_map,
                                      pipe_tbl_ha_reconc_report_t *ha_report) {
  uint32_t count = 0;
  pipe_mgr_ha_entry_t *entry = NULL;

  /* Initialize all the counters to zeros */
  ha_report->num_entries_added = 0;
  ha_report->num_entries_deleted = 0;
  ha_report->num_entries_modified = 0;

  count = 0;
  for (entry = spec_map->to_add_list; entry; entry = entry->np) {
    count++;
  }
  ha_report->num_entries_added = count;

  count = 0;
  for (entry = spec_map->to_delete_list; entry; entry = entry->np) {
    count++;
  }
  ha_report->num_entries_deleted = count;

  count = 0;
  for (entry = spec_map->to_modify_list; entry; entry = entry->np) {
    count++;
  }
  ha_report->num_entries_modified = count;
}

/* VK Need routines to iterate over the remaining entries */
pipe_status_t pipe_mgr_hitless_ha_reconcile(
    pipe_mgr_spec_map_t *spec_map,
    pipe_mgr_move_list_t **move_head_p,
    pipe_mat_tbl_info_t *tbl_info,
    pipe_tbl_ha_reconc_report_t *ha_report) {
  pipe_mgr_ha_entry_t *entry;
  pipe_status_t rc = PIPE_SUCCESS;

#if 0
  uint32_t count = 0;
  for (entry = spec_map->full_match_list; entry && (rc == PIPE_SUCCESS);
       entry = entry->np) {
    count++;
  }
  LOG_ERROR("FULL Match count %d", count);
  count = 0;
  for (entry = spec_map->to_delete_list; entry && (rc == PIPE_SUCCESS);
       entry = entry->np) {
    count++;
  }
  LOG_ERROR("To delete count %d", count);
  count = 0;
  for (entry = spec_map->to_add_list; entry && (rc == PIPE_SUCCESS);
       entry = entry->np) {
    count++;
  }
  LOG_ERROR("To add count %d", count);
  count = 0;
  for (entry = spec_map->to_modify_list; entry && (rc == PIPE_SUCCESS);
       entry = entry->np) {
    count++;
  }
  LOG_ERROR("To modify count %d", count);
  count = 0;
#endif

  struct pipe_mgr_move_list_t move_head;
  move_head.next = NULL;
  struct pipe_mgr_move_list_t *move_tail = &move_head;

  struct pipe_mgr_move_list_t *mh = NULL;
  /* For fully reconciled entries, we ideally do not need to generate any
   * updates or do any work, since HLP and LLP state are in sync. EXCEPT for
   * cases where a piece of info is not availabe via the hardware restore such
   * as entry priority in the case of TCAM based entries and TTL. We check
   * if any of the cases here are applicable and only then walk over the list
   * of fully reconciled entries and call the registered function to get up
   * to speed for such changes.
   */
  if (updates_needed_for_full_match_entries(tbl_info,
                                            spec_map->dev_tgt.device_id)) {
    for (entry = spec_map->full_match_list; entry && (rc == PIPE_SUCCESS);
         entry = entry->np) {
      /* All the entries in here need to update the state in HLP */
      rc = spec_map->entry_update_fn(
          spec_map->dev_tgt.device_id, spec_map->mat_tbl_hdl, entry->mn, &mh);
      if (mh) {
        move_tail->next = mh;
        mh = NULL;
        move_tail = move_tail->next;
      }
    }
  }

  /* Cache the number of elements in the add, delete and modify lists of the
   * spec map in the ha_report structure which can be queried later by the
   * application
   */
  generate_ha_reconc_report(spec_map, ha_report);

  for (entry = spec_map->to_delete_list; entry && (rc == PIPE_SUCCESS);
       entry = entry->np) {
    /* All the entries in here need to be deleted */
    rc = spec_map->entry_delete_fn(spec_map->dev_tgt.device_id,
                                   spec_map->mat_tbl_hdl,
                                   entry->mn->entry_hdl,
                                   0,
                                   &mh);
    if (mh) {
      move_tail->next = mh;
      mh = NULL;
      move_tail = move_tail->next;
    }
  }

  for (entry = spec_map->to_add_list; entry && (rc == PIPE_SUCCESS);
       entry = entry->np) {
    /* All the entries in here need to be freshly added */
    rc = spec_map->entry_place_with_hdl_fn(spec_map->dev_tgt,
                                           spec_map->mat_tbl_hdl,
                                           entry->match_spec,
                                           entry->act_fn_hdl,
                                           entry->action_spec,
                                           entry->ttl,
                                           0,
                                           entry->entry_hdl,
                                           &mh);
    if (mh) {
      move_tail->next = mh;
      mh = NULL;
      move_tail = move_tail->next;
    }
  }

  for (entry = spec_map->to_modify_list; entry && (rc == PIPE_SUCCESS);
       entry = entry->np) {
    /* VK Based on the ha_state of the entry, the modifications might have to
     * follow different processing
     */
    switch (entry->ha_state) {
      case ACTION_HDL_DIRTY:
        break;
      case ACTION_DATA_DIRTY:
        break;
      case ACTION_DATA_MISS:
        break;
      case RESOURCE_MISMATCH:
        break;
      default:
        PIPE_MGR_ASSERT(0);
        break;
    }
    if (updates_needed_for_full_match_entries(tbl_info,
                                              spec_map->dev_tgt.device_id)) {
      /* All the entries in here need to update the state in HLP */
      rc = spec_map->entry_update_fn(
          spec_map->dev_tgt.device_id, spec_map->mat_tbl_hdl, entry->mn, &mh);
      if (mh) {
        move_tail->next = mh;
        mh = NULL;
        move_tail = move_tail->next;
      }
    }
    /* All the entries in here need to be modified */
    rc |= spec_map->entry_modify_fn(spec_map->dev_tgt.device_id,
                                    spec_map->mat_tbl_hdl,
                                    entry->mn->entry_hdl,
                                    entry->act_fn_hdl,
                                    entry->action_spec,
                                    0,
                                    &mh);
    if (mh) {
      move_tail->next = mh;
      mh = NULL;
      move_tail = move_tail->next;
    }
  }

  *move_head_p = move_head.next;

  /* Now cleanup all the state for this spec-map */
  pipe_mgr_hitless_ha_delete_spec_map(spec_map);

  return rc;
}
