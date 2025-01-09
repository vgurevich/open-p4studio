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
 * @file pipe_mgr_tbl.c
 * @date
 *
 * Implementation of pipeline management driver interface
 */

/* Standard header files */
#include <netinet/in.h>

/* Module header files */
#include <dvm/bf_drv_intf.h>
#include <lld/bf_dma_if.h>
#include <tofino_regs/tofino.h>

/* Local header files */
#include "pipe_mgr_int.h"
#include "pipe_mgr_sm.h"
#include "pipe_mgr_tcam.h"
#include "pipe_mgr_tcam_hw.h"
#include "pipe_mgr_alpm.h"
#include "pipe_mgr_exm_tbl_init.h"
#include "pipe_mgr_exm_tbl_mgr.h"
#include "pipe_mgr_adt_init.h"
#include "pipe_mgr_adt_mgr_int.h"
#include "pipe_mgr_stat_tbl_init.h"
#include "pipe_mgr_meter_tbl_init.h"
#include "pipe_mgr_select_tbl.h"
#include "pipe_mgr_phase0_tbl_mgr.h"
#include "pipe_mgr_tbl.h"
#include "pipe_mgr_stful_tbl_mgr.h"
#include "pipe_mgr_idle.h"
#include "pipe_mgr_ctx_json.h"

pipe_status_t pipe_mgr_mat_tbl_get_first_stage_table(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t h,
    profile_id_t *profile_id_p,
    uint32_t *stage_id_p,
    rmt_tbl_hdl_t *stage_table_handle_p) {
  pipe_mat_tbl_info_t *mat_tbl_info;

  mat_tbl_info = pipe_mgr_get_tbl_info(dev_id, h, __func__, __LINE__);
  if (!mat_tbl_info) {
    return PIPE_OBJ_NOT_FOUND;
  }

  uint32_t min_stage_id = 0;
  rmt_tbl_hdl_t min_stage_table_handle = 0;

  if (mat_tbl_info->num_rmt_info) {
    for (uint32_t i = 0; i < mat_tbl_info->num_rmt_info; i++) {
      rmt_tbl_info_t *rmt_info = &mat_tbl_info->rmt_info[i];

      if (i == 0) {
        min_stage_id = rmt_info->stage_id;
        min_stage_table_handle = rmt_info->handle;
      } else {
        if (rmt_info->stage_id < min_stage_id) {
          min_stage_id = rmt_info->stage_id;
          if (rmt_info->handle < min_stage_table_handle) {
            min_stage_table_handle = rmt_info->handle;
          }
        }
      }
    }
  } else if (mat_tbl_info->keyless_info) {
    min_stage_id = mat_tbl_info->keyless_info->stage_id;
    min_stage_table_handle = mat_tbl_info->keyless_info->log_tbl_id;
  }
  if (profile_id_p) {
    *profile_id_p = mat_tbl_info->profile_id;
  }
  if (stage_id_p) {
    *stage_id_p = min_stage_id;
  }
  if (stage_table_handle_p) {
    *stage_table_handle_p = min_stage_table_handle;
  }
  return PIPE_SUCCESS;
}

extern pipe_mgr_ctx_t *pipe_mgr_ctx;

static pipe_status_t get_placed_entry_count(dev_target_t dev_tgt,
                                            pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                            uint32_t *count_p) {
  enum pipe_mgr_table_owner_t owner;
  owner = pipe_mgr_sm_tbl_owner(dev_tgt.device_id, mat_tbl_hdl);
  if (PIPE_MGR_TBL_OWNER_EXM == owner) {
    uint32_t x = 0;
    pipe_status_t rc = PIPE_SUCCESS;
    rc = pipe_mgr_exm_tbl_get_placed_entry_count(dev_tgt, mat_tbl_hdl, &x);
    if (PIPE_SUCCESS == rc) {
      *count_p = x;
    } else {
      return rc;
    }
  } else if (PIPE_MGR_TBL_OWNER_TRN == owner) {
    return pipe_mgr_tcam_get_placed_entry_count(dev_tgt, mat_tbl_hdl, count_p);
  } else if (PIPE_MGR_TBL_OWNER_ALPM == owner) {
    return pipe_mgr_alpm_get_programmed_entry_count(
        dev_tgt, mat_tbl_hdl, count_p);
  } else if (PIPE_MGR_TBL_OWNER_PHASE0 == owner) {
    return pipe_mgr_phase0_get_placed_entry_count(
        dev_tgt, mat_tbl_hdl, count_p);
  } else if (PIPE_MGR_TBL_OWNER_NO_KEY == owner) {
    *count_p = 0;
  } else {
    PIPE_MGR_DBGCHK(0);
  }
  return PIPE_SUCCESS;
}

static pipe_status_t get_programmed_entry_count(dev_target_t dev_tgt,
                                                pipe_mat_tbl_hdl_t tbl_hdl,
                                                uint32_t *count) {
  if (pipe_mgr_is_device_virtual(dev_tgt.device_id)) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }
  pipe_status_t rc = PIPE_SUCCESS;
  int owner = pipe_mgr_sm_tbl_owner(dev_tgt.device_id, tbl_hdl);

  uint32_t x = 0;
  if (owner == PIPE_MGR_TBL_OWNER_TRN) {
    rc = pipe_mgr_tcam_get_programmed_entry_count(dev_tgt, tbl_hdl, &x);
  } else if (owner == PIPE_MGR_TBL_OWNER_ALPM) {
    rc = pipe_mgr_alpm_get_programmed_entry_count(dev_tgt, tbl_hdl, &x);
  } else if (owner == PIPE_MGR_TBL_OWNER_EXM) {
    rc = pipe_mgr_exm_tbl_get_programmed_entry_count(dev_tgt, tbl_hdl, &x);
  } else if (owner == PIPE_MGR_TBL_OWNER_PHASE0) {
    pipe_mgr_phase0_get_programmed_entry_count(dev_tgt, tbl_hdl, &x);
  } else if (owner == PIPE_MGR_TBL_OWNER_NO_KEY) {
    *count = 0;
  } else {
    rc = PIPE_INVALID_ARG;
  }
  if (PIPE_SUCCESS == rc) *count = x;

  return rc;
}

pipe_status_t pipe_mgr_tbl_set_scope(pipe_sess_hdl_t sess_hdl,
                                     bf_dev_id_t dev_id,
                                     pipe_mat_tbl_hdl_t tbl_hdl,
                                     bool symmetric,
                                     scope_num_t num_scopes,
                                     scope_pipes_t *scope_pipe_bmp,
                                     bool update_dflt) {
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_mat_tbl_info_t *info = NULL;
  char buffer[500];
  int i = 0, c_len = 0;

  info = pipe_mgr_get_tbl_info(dev_id, tbl_hdl, __func__, __LINE__);
  if (!info) {
    LOG_ERROR(
        "Invalid table_hdl %d (0x%x) on dev %u ", tbl_hdl, tbl_hdl, dev_id);
    return PIPE_INVALID_ARG;
  }
  PIPE_MGR_MEMSET(buffer, 0, sizeof(buffer));
  for (i = 0; i < num_scopes; i++) {
    c_len += snprintf(buffer + c_len,
                      (500 - c_len - 1),
                      "Scope[%d] 0x%x, ",
                      i,
                      scope_pipe_bmp[i]);
  }

  LOG_TRACE("%s: Scope set: sess %d, dev %d, tbl 0x%x, num_scopes %d, %s",
            __func__,
            sess_hdl,
            dev_id,
            tbl_hdl,
            num_scopes,
            buffer);

  /* Set the symmetric mode */
  rc = pipe_mgr_tbl_set_symmetric_mode(sess_hdl,
                                       dev_id,
                                       tbl_hdl,
                                       symmetric,
                                       num_scopes,
                                       scope_pipe_bmp,
                                       update_dflt);

  return rc;
}

pipe_status_t pipe_mgr_tbl_get_symmetric_mode(bf_dev_id_t dev_id,
                                              pipe_mat_tbl_hdl_t tbl_hdl,
                                              bool *symmetric,
                                              scope_num_t *num_scopes,
                                              scope_pipes_t *scope_pipe_bmp) {
  pipe_status_t rc;
  int owner = pipe_mgr_sm_tbl_owner(dev_id, tbl_hdl);

  /* Get symmetric mode */
  if (owner == PIPE_MGR_TBL_OWNER_TRN) {
    rc = pipe_mgr_tcam_tbl_get_symmetric_mode(
        dev_id, tbl_hdl, symmetric, num_scopes, scope_pipe_bmp);
  } else if (owner == PIPE_MGR_TBL_OWNER_EXM) {
    rc = pipe_mgr_exm_tbl_get_symmetric_mode(
        dev_id, tbl_hdl, symmetric, num_scopes, scope_pipe_bmp);
  } else if (owner == PIPE_MGR_TBL_OWNER_ALPM) {
    rc = pipe_mgr_alpm_tbl_get_symmetric_mode(
        dev_id, tbl_hdl, symmetric, num_scopes, scope_pipe_bmp);
  } else if (owner == PIPE_MGR_TBL_OWNER_PHASE0) {
    rc = pipe_mgr_phase0_get_symmetric_mode(
        dev_id, tbl_hdl, symmetric, num_scopes, scope_pipe_bmp);
  } else if (owner == PIPE_MGR_TBL_OWNER_NO_KEY) {
    rc = PIPE_SUCCESS;
  } else if (owner == PIPE_MGR_TBL_OWNER_ADT) {
    rc = pipe_mgr_adt_tbl_get_symmetric_mode(
        dev_id, tbl_hdl, symmetric, num_scopes, scope_pipe_bmp);
  } else if (owner == PIPE_MGR_TBL_OWNER_SELECT) {
    rc = pipe_mgr_sel_tbl_get_symmetric_mode(
        dev_id, tbl_hdl, symmetric, num_scopes, scope_pipe_bmp);
  } else if (owner == PIPE_MGR_TBL_OWNER_STAT) {
    rc = pipe_mgr_stat_tbl_get_symmetric_mode(
        dev_id, tbl_hdl, symmetric, num_scopes, scope_pipe_bmp);
  } else if (owner == PIPE_MGR_TBL_OWNER_METER) {
    rc = pipe_mgr_meter_tbl_get_symmetric_mode(
        dev_id, tbl_hdl, symmetric, num_scopes, scope_pipe_bmp);
  } else if (owner == PIPE_MGR_TBL_OWNER_STFUL) {
    rc = pipe_mgr_stful_tbl_get_symmetric_mode(
        dev_id, tbl_hdl, symmetric, num_scopes, scope_pipe_bmp);
  } else {
    LOG_ERROR("%s: Symmetric mode get is not supported on dev %d tbl 0x%x",
              __func__,
              dev_id,
              tbl_hdl);
    rc = PIPE_INVALID_ARG;
  }

  return rc;
}

/* Get scope */
pipe_status_t pipe_mgr_tbl_get_scope(bf_dev_id_t dev_id,
                                     pipe_mat_tbl_hdl_t tbl_hdl,
                                     bool *symmetric,
                                     scope_num_t *num_scopes,
                                     scope_pipes_t *scope_pipe_bmp) {
  pipe_status_t status = PIPE_SUCCESS;

  status = pipe_mgr_tbl_get_symmetric_mode(
      dev_id, tbl_hdl, symmetric, num_scopes, scope_pipe_bmp);
  if (status != PIPE_SUCCESS) {
    LOG_TRACE(
        "%s: Scope get failed : dev %d, tbl 0x%x ", __func__, dev_id, tbl_hdl);
  } else {
    int i = 0, c_len = 0;
    char buffer[500];
    PIPE_MGR_MEMSET(buffer, 0, sizeof(buffer));
    for (i = 0; i < (*num_scopes); i++) {
      c_len += snprintf(buffer + c_len,
                        (500 - c_len - 1),
                        "Scope[%d] 0x%x, ",
                        i,
                        scope_pipe_bmp[i]);
    }
    LOG_TRACE(
        "%s: Scope get success: dev %d, tbl 0x%x, sym %d "
        "num-scopes %d, %s ",
        __func__,
        dev_id,
        tbl_hdl,
        *symmetric,
        *num_scopes,
        buffer);
  }
  return status;
}

/* Check if scope is different */
bool pipe_mgr_tbl_is_scope_different(bf_dev_id_t dev_id,
                                     pipe_mat_tbl_hdl_t tbl_hdl,
                                     bool symmetric_a,
                                     scope_num_t num_scopes_a,
                                     scope_pipes_t *scope_pipe_bmp_a,
                                     bool symmetric_b,
                                     scope_num_t num_scopes_b,
                                     scope_pipes_t *scope_pipe_bmp_b) {
  int i = 0;
  (void)dev_id;
  (void)tbl_hdl;

  if (symmetric_a != symmetric_b) {
    return true;
  }
  if (num_scopes_a != num_scopes_b) {
    return true;
  }

  for (i = 0; (i < num_scopes_a) && (i < num_scopes_b); i++) {
    if (scope_pipe_bmp_a[i] != scope_pipe_bmp_b[i]) {
      return true;
    }
  }
  return false;
}
static inline bool pipe_mgr_tbl_is_scope_same(bf_dev_id_t dev_id,
                                              pipe_mat_tbl_hdl_t tbl_hdl,
                                              bool symmetric_a,
                                              scope_num_t num_scopes_a,
                                              scope_pipes_t *scope_pipe_bmp_a,
                                              bool symmetric_b,
                                              scope_num_t num_scopes_b,
                                              scope_pipes_t *scope_pipe_bmp_b) {
  return !pipe_mgr_tbl_is_scope_different(dev_id,
                                          tbl_hdl,
                                          symmetric_a,
                                          num_scopes_a,
                                          scope_pipe_bmp_a,
                                          symmetric_b,
                                          num_scopes_b,
                                          scope_pipe_bmp_b);
}

/* Get lowest pipe in the scope bitmap */
bf_dev_pipe_t pipe_mgr_get_lowest_pipe_in_scope(scope_pipes_t scope_pipe_bmp) {
  uint32_t j = 0;
  for (j = 0; j < (sizeof(scope_pipe_bmp) * 8); j++) {
    if (scope_pipe_bmp & (1u << j)) {
      return j;
    }
  }

  PIPE_MGR_DBGCHK(0);
  return BF_INVALID_PIPE;
}

/* Given a pipe, get the set of all pipes in the scope */
pipe_status_t pipe_mgr_get_all_pipes_in_scope(bf_dev_pipe_t pipe,
                                              scope_num_t num_scopes,
                                              scope_pipes_t *scope_pipe_bmp,
                                              pipe_bitmap_t *pipe_bmp) {
  uint32_t i = 0, j = 0;
  bool found = false;

  if (pipe == BF_DEV_PIPE_ALL) {
    for (j = 0; j < (sizeof(scope_pipes_t) * 8); j++) {
      if (scope_pipe_bmp[i] & (1u << j)) {
        PIPE_BITMAP_SET(pipe_bmp, j);
      }
    }
  } else {
    for (i = 0; i < num_scopes; i++) {
      /* Check if pipe belongs to this scope */
      for (j = 0; j < (sizeof(scope_pipes_t) * 8); j++) {
        if (scope_pipe_bmp[i] & (1u << j)) {
          if (j == pipe) {
            found = true;
            break;
          }
        }
      }
      /* Get all the pipes from this scope */
      if (found) {
        for (j = 0; j < (sizeof(scope_pipes_t) * 8); j++) {
          if (scope_pipe_bmp[i] & (1u << j)) {
            PIPE_BITMAP_SET(pipe_bmp, j);
          }
        }
        break;
      }  // if found
    }    // num_scopes
  }      // else

  return PIPE_SUCCESS;
}

/* Convert scope pipe bitmap from int format to pipe_bitmap_t format */
pipe_status_t pipe_mgr_convert_scope_pipe_bmp(scope_pipes_t scope_pipe_bmp,
                                              pipe_bitmap_t *pipe_bmp) {
  uint32_t j = 0;

  for (j = 0; j < (sizeof(scope_pipes_t) * 8); j++) {
    if (scope_pipe_bmp & (1u << j)) {
      PIPE_BITMAP_SET(pipe_bmp, j);
    }
  }

  return PIPE_SUCCESS;
}

/* Check if the given pipe is in the given pipe scope bit mask. */
pipe_status_t pipe_mgr_is_pipe_in_bmp(pipe_bitmap_t *pipe_bmp,
                                      bf_dev_pipe_t pipe_id) {
  if (pipe_id == BF_DEV_PIPE_ALL || PIPE_BITMAP_GET(pipe_bmp, pipe_id)) {
    return PIPE_SUCCESS;
  } else {
    return PIPE_INVALID_ARG;
  }
}

static pipe_status_t pipe_mgr_tcam_tbl_set_symmetric_mode_all(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    bool symmetric,
    scope_num_t num_scopes,
    scope_pipes_t *scope_pipe_bmp) {
  pipe_status_t rc = PIPE_SUCCESS;

  rc = pipe_mgr_tcam_tbl_set_symmetric_mode(
      dev_id, tbl_hdl, symmetric, num_scopes, scope_pipe_bmp, false);
  rc |= pipe_mgr_tcam_tbl_set_symmetric_mode(
      dev_id, tbl_hdl, symmetric, num_scopes, scope_pipe_bmp, true);

  return rc;
}

static pipe_status_t pipe_mgr_sel_tbl_set_symmetric_mode_all(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    bool symmetric,
    scope_num_t num_scopes,
    scope_pipes_t *scope_pipe_bmp) {
  pipe_status_t rc = PIPE_SUCCESS;

  rc = pipe_mgr_sel_tbl_set_symmetric_mode(
      sess_hdl, dev_id, tbl_hdl, symmetric, num_scopes, scope_pipe_bmp, false);
  rc |= pipe_mgr_sel_tbl_set_symmetric_mode(
      sess_hdl, dev_id, tbl_hdl, symmetric, num_scopes, scope_pipe_bmp, true);
  if (PIPE_SUCCESS != rc) return rc;

  /* If the selection table is using stateful memory, change the symmetric
   * mode on it as well. */
  pipe_stful_tbl_hdl_t stful_hdl;
  stful_hdl = pipe_mgr_sel_tbl_get_stful_ref(dev_id, tbl_hdl);
  if (stful_hdl)
    rc = pipe_mgr_stful_tbl_set_symmetric_mode(
        sess_hdl, dev_id, stful_hdl, symmetric, num_scopes, scope_pipe_bmp);

  return rc;
}

static pipe_status_t set_symmetric_mode_on_resource_tbls(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    bool symmetric,
    scope_num_t num_scopes,
    scope_pipes_t *scope_pipe_bmp) {
  pipe_mat_tbl_info_t *info =
      pipe_mgr_get_tbl_info(dev_id, tbl_hdl, __func__, __LINE__);
  if (info == NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }
  pipe_status_t x = PIPE_SUCCESS;
  if (x == PIPE_SUCCESS && info->adt_tbl_ref) {
    x = pipe_mgr_adt_tbl_set_symmetric_mode(dev_id,
                                            info->adt_tbl_ref->tbl_hdl,
                                            symmetric,
                                            num_scopes,
                                            scope_pipe_bmp);
    if (x != PIPE_SUCCESS) {
      LOG_ERROR(
          "Failed to set %ssymmetric mode on dev %u, tbl %d (0x%x), adt %d "
          "(0x%x)",
          symmetric ? "" : "non-",
          dev_id,
          tbl_hdl,
          tbl_hdl,
          info->adt_tbl_ref->tbl_hdl,
          info->adt_tbl_ref->tbl_hdl);
    }
  }
  if (x == PIPE_SUCCESS && info->sel_tbl_ref) {
    x = pipe_mgr_sel_tbl_set_symmetric_mode_all(sess_hdl,
                                                dev_id,
                                                info->sel_tbl_ref->tbl_hdl,
                                                symmetric,
                                                num_scopes,
                                                scope_pipe_bmp);
    if (x != PIPE_SUCCESS) {
      LOG_ERROR(
          "Failed to set %ssymmetric mode on dev %u, tbl %d (0x%x), sel %d "
          "(0x%x)",
          symmetric ? "" : "non-",
          dev_id,
          tbl_hdl,
          tbl_hdl,
          info->sel_tbl_ref->tbl_hdl,
          info->sel_tbl_ref->tbl_hdl);
    }
  }
  if (x == PIPE_SUCCESS && info->stful_tbl_ref) {
    x = pipe_mgr_stful_tbl_set_symmetric_mode(sess_hdl,
                                              dev_id,
                                              info->stful_tbl_ref->tbl_hdl,
                                              symmetric,
                                              num_scopes,
                                              scope_pipe_bmp);
    if (x != PIPE_SUCCESS) {
      LOG_ERROR(
          "Failed to set %ssymmetric mode on dev %u, tbl %d (0x%x), stful %d "
          "(0x%x)",
          symmetric ? "" : "non-",
          dev_id,
          tbl_hdl,
          tbl_hdl,
          info->stful_tbl_ref->tbl_hdl,
          info->stful_tbl_ref->tbl_hdl);
    }
  }

  if (x == PIPE_SUCCESS && pipe_mgr_mat_tbl_has_idle(dev_id, tbl_hdl)) {
    x = pipe_mgr_idle_tbl_set_symmetric_mode(
        sess_hdl, dev_id, tbl_hdl, symmetric, num_scopes, scope_pipe_bmp);
  }

  if (x != PIPE_SUCCESS) {
    LOG_ERROR("Failed to set %ssymmetric mode on dev %u, tbl %d (0x%x), idle ",
              symmetric ? "" : "non-",
              dev_id,
              tbl_hdl,
              tbl_hdl);
  }

  if (x == PIPE_SUCCESS && info->stat_tbl_ref) {
    x = pipe_mgr_stat_tbl_set_symmetric_mode(dev_id,
                                             info->stat_tbl_ref->tbl_hdl,
                                             symmetric,
                                             num_scopes,
                                             scope_pipe_bmp);
    if (x != PIPE_SUCCESS) {
      LOG_ERROR(
          "Failed to set %ssymmetric mode on dev %u, tbl %d (0x%x), stat %d "
          "(0x%x)",
          symmetric ? "" : "non-",
          dev_id,
          tbl_hdl,
          tbl_hdl,
          info->stat_tbl_ref->tbl_hdl,
          info->stat_tbl_ref->tbl_hdl);
    }
  }

  if (x == PIPE_SUCCESS && info->meter_tbl_ref) {
    x = pipe_mgr_meter_tbl_set_symmetric_mode(sess_hdl,
                                              dev_id,
                                              info->meter_tbl_ref->tbl_hdl,
                                              symmetric,
                                              num_scopes,
                                              scope_pipe_bmp);
    if (x != PIPE_SUCCESS) {
      LOG_ERROR(
          "Failed to set %ssymmetric mode on dev %u, tbl %d (0x%x), meter %d "
          "(0x%x)",
          symmetric ? "" : "non-",
          dev_id,
          tbl_hdl,
          tbl_hdl,
          info->meter_tbl_ref->tbl_hdl,
          info->meter_tbl_ref->tbl_hdl);
    }
  }
  return x;
}

/* A helper to clear the default entry of all MATs using the specified ADT.
 * This is helpful when the ADT is changing symmetric mode and the MATs can no
 * longer use an ADT entry (since the MAT has not yet changed symmetric mode).
 */
static pipe_status_t pipe_mgr_tbl_clr_dflts_using_adt(
    pipe_sess_hdl_t shdl,
    rmt_dev_info_t *dev_info,
    pipe_adt_tbl_hdl_t adt_hdl) {
  bf_dev_id_t dev_id = dev_info->dev_id;
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_adt_tbl_info_t *adt_info =
      pipe_mgr_get_adt_tbl_info(dev_id, adt_hdl, __func__, __LINE__);
  if (!adt_info) {
    PIPE_MGR_DBGCHK(adt_info);
    return PIPE_UNEXPECTED;
  }

  /* Get the maximum size of the action data used by the ADT so we can allocate
   * an action spec. */
  uint32_t max_data_sz = 0;
  for (uint32_t i = 0; i < adt_info->num_actions; ++i) {
    if (adt_info->act_fn_hdl_info[i].num_bytes > max_data_sz)
      max_data_sz = adt_info->act_fn_hdl_info[i].num_bytes;
  }
  uint8_t *act_data_bits = PIPE_MGR_MALLOC(max_data_sz);
  if (!act_data_bits) return PIPE_NO_SYS_RESOURCES;

  /* Loop over all profiles. */
  for (uint32_t prof = 0; prof < dev_info->num_pipeline_profiles; ++prof) {
    /* For each profile, loop over all MATs. */
    rmt_dev_profile_info_t *prof_info = dev_info->profile_info[prof];
    rmt_dev_tbl_info_t *tbl_info = &prof_info->tbl_info_list;
    for (uint32_t i = 0; i < tbl_info->num_mat_tbls; ++i) {
      pipe_mat_tbl_hdl_t mat_hdl = tbl_info->mat_tbl_list[i].handle;
      pipe_mat_tbl_info_t *mat_info =
          pipe_mgr_get_tbl_info(dev_id, mat_hdl, __func__, __LINE__);
      if (!mat_info) {
        rc = PIPE_UNEXPECTED;
        goto done;
      }
      if (!mat_info->num_adt_tbl_refs ||
          mat_info->adt_tbl_ref[0].ref_type != PIPE_TBL_REF_TYPE_INDIRECT ||
          mat_info->adt_tbl_ref[0].tbl_hdl != adt_hdl)
        continue;

      enum pipe_mgr_table_owner_t owner =
          pipe_mgr_sm_tbl_owner(dev_id, mat_hdl);

      /* We've found a MAT using the ADT.  Check if the dflt entry has a valid
       * member in the ADT.  If so the dflt entry must be reset. */
      bool sym = false;
      scope_num_t num_scopes = 0;
      scope_pipes_t scopes[dev_info->num_active_pipes];
      pipe_mgr_tbl_get_symmetric_mode(
          dev_id, mat_hdl, &sym, &num_scopes, &scopes[0]);
      for (scope_num_t scope_id = 0; scope_id < num_scopes; ++scope_id) {
        dev_target_t dt;
        dt.device_id = dev_id;
        dt.dev_pipe_id =
            sym ? BF_DEV_PIPE_ALL
                : pipe_mgr_get_lowest_pipe_in_scope(scopes[scope_id]);
        pipe_action_spec_t aspec = {0};
        aspec.act_data.action_data_bits = act_data_bits;
        pipe_act_fn_hdl_t act_hdl = 0;
        rc = pipe_mgr_get_default_entry(
            shdl, dt, mat_info, &aspec, &act_hdl, false);
        if (rc != PIPE_SUCCESS) continue;
        if (!IS_ACTION_SPEC_ACT_DATA_HDL(&aspec) &&
            !IS_ACTION_SPEC_SEL_GRP(&aspec)) {
          continue;
        }
        /* Clear it. */
        pipe_mgr_move_list_t *ml = NULL;
        rc = pipe_mgr_cleanup_default_entry(shdl, dt, mat_info, &ml);
        if (rc != PIPE_SUCCESS) {
          LOG_ERROR(
              "Error (%s) clearing dflt entry for dev %d pipe %x table %s "
              "(0x%x)",
              pipe_str_err(rc),
              dev_id,
              dt.dev_pipe_id,
              mat_info->name,
              mat_info->handle);
          goto done;
        }
        if (ml) {
          if (pipe_mgr_is_device_virtual(dev_id)) {
            pipe_mgr_tbl_free_move_list(owner, ml, false);
          } else {
            uint32_t processed = 0;
            rc = pipe_mgr_tbl_process_move_list(
                shdl, dev_id, mat_hdl, owner, ml, &processed, true);
            if (rc != PIPE_SUCCESS) {
              LOG_ERROR(
                  "Error (%s) processing move list while clearing dflt entry "
                  "for dev %d pipe %x table %s (0x%x)",
                  pipe_str_err(rc),
                  dev_id,
                  dt.dev_pipe_id,
                  mat_info->name,
                  mat_info->handle);
              goto done;
            }
          }
        }
      }
    }
  }
done:
  if (act_data_bits) PIPE_MGR_FREE(act_data_bits);
  return rc;
}

pipe_status_t pipe_mgr_tbl_set_symmetric_mode_wrapper(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    bool symmetric,
    bool update_hw,
    scope_num_t num_scopes,
    scope_pipes_t *scope_pipe_bmp,
    bool update_dflt) {
  pipe_status_t rc = PIPE_SUCCESS;
  int owner = PIPE_MGR_TBL_OWNER_NONE;
  uint32_t num_pipes = 0;
  rmt_dev_info_t *dev_info = NULL;
  bool o_symmetric = false;
  scope_num_t o_num_scopes = 0;
  pipe_bitmap_t pipe_bmp;
  scope_pipes_t *o_scope_pipe_bmp = NULL;

  owner = pipe_mgr_sm_tbl_owner(dev_id, tbl_hdl);
  LOG_TRACE(
      "%s: Symmetric mode: sess %d, dev %d, tbl 0x%x, symmetric %d, owner %d ",
      __func__,
      sess_hdl,
      dev_id,
      tbl_hdl,
      symmetric,
      owner);

  pipe_mat_tbl_info_t *info =
      pipe_mgr_get_tbl_info(dev_id, tbl_hdl, __func__, __LINE__);
  if (info == NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("Invalid device %u ", dev_id);
    return PIPE_INVALID_ARG;
  }

  PIPE_BITMAP_INIT(&pipe_bmp, PIPE_BMP_SIZE);
  rc = pipe_mgr_get_pipe_bmp_for_profile(
      dev_info, info->profile_id, &pipe_bmp, __func__, __LINE__);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error getting the pipe-bmp for profile-id %d, dev %d rc 0x%x",
        __func__,
        __LINE__,
        info->profile_id,
        dev_id,
        rc);
    return rc;
  }
  num_pipes = PIPE_BITMAP_COUNT(&pipe_bmp);

  /* Check request against current scope and return if there is no change */
  o_scope_pipe_bmp = PIPE_MGR_CALLOC(num_pipes, sizeof(scope_pipes_t));
  if (!o_scope_pipe_bmp) {
    LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  pipe_mgr_tbl_get_symmetric_mode(
      dev_id, tbl_hdl, &o_symmetric, &o_num_scopes, &o_scope_pipe_bmp[0]);
  if (!pipe_mgr_tbl_is_scope_different(dev_id,
                                       tbl_hdl,
                                       symmetric,
                                       num_scopes,
                                       scope_pipe_bmp,
                                       o_symmetric,
                                       o_num_scopes,
                                       o_scope_pipe_bmp)) {
    rc = PIPE_SUCCESS;
    goto cleanup;
  }

  /* Check the occupancy in the match table */
  uint32_t usage = 0;
  dev_target_t dev_tgt = {.device_id = dev_id, .dev_pipe_id = BF_DEV_PIPE_ALL};
  rc = pipe_mgr_tbl_get_entry_count(dev_tgt, tbl_hdl, true, &usage);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Dev %d tbl %s 0x%x set-sym %d error %s getting entry count",
        __func__,
        __LINE__,
        dev_id,
        info->name,
        tbl_hdl,
        symmetric,
        pipe_str_err(rc));
    goto cleanup;
  }
  if (usage > 0) {
    LOG_ERROR(
        "%s:%d Dev %d tbl %s 0x%x set-sym %d cannot change symmetric mode "
        "while usage (%u) is non-zero",
        __func__,
        __LINE__,
        dev_id,
        info->name,
        tbl_hdl,
        symmetric,
        usage);
    rc = PIPE_NOT_SUPPORTED;
    goto cleanup;
  }

  /* Check the count in each of the associated resource tables */
  pipe_tbl_ref_t *refs[] = {info->adt_tbl_ref,
                            info->sel_tbl_ref,
                            info->stat_tbl_ref,
                            info->meter_tbl_ref,
                            info->stful_tbl_ref};
  const char *ref_names[] = {
      "Action", "Selector", "Stats", "Meter", "Stateful"};
  for (size_t i = 0; i < (sizeof refs / sizeof refs[0]); ++i) {
    if (!refs[i]) continue;
    /* Only check indirectly referenced resource tables.  Direct tables would
     * have a usage equal to the usage of the MAT which has already been
     * verified above. */
    if (refs[i]->ref_type != PIPE_TBL_REF_TYPE_INDIRECT) continue;

    /* Only check the usage of the resource table if its symmetric mode is the
     * same as the MAT's mode.  When multiple MATs share a resource table the
     * first MAT which changes symmetric mode will also change the mode of the
     * resource table.  When the other MATs change mode we do not verify usage
     * again because the resource tables will not change their symmetric mode
     * (it was already changed). */
    bool res_sym;
    scope_num_t res_num_scopes;
    scope_pipes_t res_scopes[dev_info->num_active_pipes];
    pipe_mgr_tbl_get_symmetric_mode(
        dev_id, refs[i]->tbl_hdl, &res_sym, &res_num_scopes, &res_scopes[0]);
    if (pipe_mgr_tbl_is_scope_same(dev_id,
                                   tbl_hdl,
                                   res_sym,
                                   res_num_scopes,
                                   res_scopes,
                                   symmetric,
                                   num_scopes,
                                   scope_pipe_bmp)) {
      continue;
    }

    rc = pipe_mgr_tbl_get_entry_count(dev_tgt, refs[i]->tbl_hdl, true, &usage);
    if (rc == PIPE_SUCCESS && usage > 0) {
      LOG_ERROR(
          "%s Dev %d tbl %s 0x%x set-sym %d cannot change symmetric mode while "
          "%s 0x%x usage (%u) is non-zero",
          __func__,
          dev_id,
          info->name,
          tbl_hdl,
          symmetric,
          ref_names[i],
          refs[i]->tbl_hdl,
          usage);
      rc = PIPE_NOT_SUPPORTED;
      goto cleanup;
    } else if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "Error %s checking usage on resource table 0x%x while changing "
          "symmetric mode on %s (0x%x)",
          pipe_str_err(rc),
          refs[i]->tbl_hdl,
          info->name,
          info->handle);
      PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);
    }
  }

  if (update_dflt) {
    /* If there are default entries in the table they must be cleared, not
     * reset, before changing the symmetric mode.  Additionally, since the
     * symmetric mode change will propigate to any indirect resource tables as
     * well, other MATs sharing those resource tables may also need their
     * default entry cleared.  This is true for ADTs where the default entries
     * may refer to a default ADT member.  However, this only needs to be done
     * when the first MAT changes symmetric mode.  When subsequent MATs change
     * symmetric mode (to be the same as the resource table) there is no need
     * to clear entries in the resource table or in other MATs. */
    bool has_indirect_adt =
        info->num_adt_tbl_refs &&
        info->adt_tbl_ref[0].ref_type == PIPE_TBL_REF_TYPE_INDIRECT;

    if (has_indirect_adt) {
      pipe_adt_tbl_hdl_t adt_hdl = info->adt_tbl_ref[0].tbl_hdl;
      bool adt_sym;
      scope_num_t adt_num_scopes;
      scope_pipes_t adt_scopes[dev_info->num_active_pipes];
      rc = pipe_mgr_adt_tbl_get_symmetric_mode(
          dev_id, adt_hdl, &adt_sym, &adt_num_scopes, adt_scopes);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR("%s: Error %s getting scope info of ADT 0x%x",
                  __func__,
                  pipe_str_err(rc),
                  adt_hdl);
        goto cleanup;
      }

      if (pipe_mgr_tbl_is_scope_different(dev_id,
                                          adt_hdl,
                                          adt_sym,
                                          adt_num_scopes,
                                          adt_scopes,
                                          symmetric,
                                          num_scopes,
                                          scope_pipe_bmp)) {
        /* Since the requested MAT scope is different than the ADT scope we
         * need to clear the default entry on any MATs using the ADT. */
        rc = pipe_mgr_tbl_clr_dflts_using_adt(sess_hdl, dev_info, adt_hdl);
        if (rc != PIPE_SUCCESS) {
          LOG_ERROR(
              "Dev %d, error %s while clearing default entries for symmetric "
              "mode change on %s (0x%x)",
              dev_id,
              pipe_str_err(rc),
              info->name,
              info->handle);
          goto cleanup;
        }
      }
    }

    /* We still need to explicitly clear the default entry on this MAT since it
     * may not have used an indirect ADT or its default may not have used a
     * member in the ADT. */

    pipe_mat_ent_hdl_t def_hdls[dev_info->num_active_pipes];
    rc = pipe_mgr_tbl_get_default_entry_handles(
        tbl_hdl, dev_id, def_hdls, &usage);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Failed to get default handles for match table 0x%x",
                __func__,
                __LINE__,
                tbl_hdl);
      goto cleanup;
    } else if (usage > 0) {
      /* Default entries exist.  Clear them before changing symmetric mode. Note
       * that we need to pass the correct dev target(s) to the clear API. */
      for (scope_num_t i = 0; i < o_num_scopes; ++i) {
        pipe_mgr_move_list_t *ml = NULL;
        uint32_t processed = 0;
        dev_target_t dt;
        dt.device_id = dev_id;
        if (o_symmetric) {
          dt.dev_pipe_id = BF_DEV_PIPE_ALL;
        } else {
          dt.dev_pipe_id =
              pipe_mgr_get_lowest_pipe_in_scope(o_scope_pipe_bmp[i]);
        }
        rc = pipe_mgr_cleanup_default_entry(sess_hdl, dt, info, &ml);
        if (rc != PIPE_SUCCESS) {
          LOG_ERROR(
              "Dev %d tbl %s 0x%x symmetric change: failed to reset dflt entry "
              "%s",
              dev_id,
              info->name,
              tbl_hdl,
              pipe_str_err(rc));
          goto cleanup;
        }
        if (ml) {
          if (pipe_mgr_is_device_virtual(dev_id)) {
            pipe_mgr_tbl_free_move_list(owner, ml, false);
          } else {
            rc = pipe_mgr_tbl_process_move_list(
                sess_hdl, dev_id, info->handle, owner, ml, &processed, true);
            if (rc != PIPE_SUCCESS) {
              LOG_ERROR(
                  "Dev %d tbl %s 0x%x symmetric change: error %s processing "
                  "reset dflt",
                  dev_id,
                  info->name,
                  tbl_hdl,
                  pipe_str_err(rc));
              goto cleanup;
            }
          }
        }
      }

      /* Push the ilist now.  The resource table clean up may block until all
       * pending notifications (such as barrier/lock-acks) are received and the
       * default entry cleanup may have posted instructions which would cause
       * such notifications. */
      rc = pipe_mgr_drv_ilist_push(&sess_hdl, NULL, NULL);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "Dev %d tbl %s 0x%x %s set: failed push dflt entry cleanup %s",
            dev_id,
            info->name,
            tbl_hdl,
            symmetric ? "symmetric" : "asymmetric",
            pipe_str_err(rc));
        goto cleanup;
      }
    }
  }

  if (owner == PIPE_MGR_TBL_OWNER_TRN) {
    rc = pipe_mgr_tcam_tbl_set_symmetric_mode_all(
        dev_id, tbl_hdl, symmetric, num_scopes, scope_pipe_bmp);
  } else if (owner == PIPE_MGR_TBL_OWNER_EXM) {
    rc = pipe_mgr_exm_tbl_set_symmetric_mode(
        dev_id, tbl_hdl, symmetric, num_scopes, scope_pipe_bmp);
  } else if (owner == PIPE_MGR_TBL_OWNER_ALPM) {
    rc = pipe_mgr_alpm_tbl_set_symmetric_mode(
        sess_hdl, dev_id, tbl_hdl, symmetric, num_scopes, scope_pipe_bmp);
  } else if (owner == PIPE_MGR_TBL_OWNER_PHASE0) {
    update_hw = false; /* Don't cache symmetric mode in HW for phase0. */
    rc = pipe_mgr_phase0_set_symmetric_mode(
        dev_id, tbl_hdl, symmetric, num_scopes, scope_pipe_bmp);
  } else if (owner == PIPE_MGR_TBL_OWNER_NO_KEY) {
    rc = PIPE_SUCCESS;
  } else {
    LOG_ERROR(
        "%s: Symmetric mode change is not supported on sess %d dev %d tbl 0x%x",
        __func__,
        sess_hdl,
        dev_id,
        tbl_hdl);
    rc = PIPE_INVALID_ARG;
  }
  if (rc != PIPE_SUCCESS) {
    goto cleanup;
  }

  rc = set_symmetric_mode_on_resource_tbls(
      sess_hdl, dev_id, tbl_hdl, symmetric, num_scopes, scope_pipe_bmp);
  if (rc != PIPE_SUCCESS) {
    /* Failed to update the symmetric mode on the resource tables, roll back the
     * update on the match table. */
    LOG_ERROR(
        "%s: Symmetric mode change failed on resource tables: sess %d dev %d "
        "tbl %s 0x%x sts %s",
        __func__,
        sess_hdl,
        dev_id,
        info->name,
        tbl_hdl,
        pipe_str_err(rc));
    if (owner == PIPE_MGR_TBL_OWNER_TRN) {
      pipe_mgr_tcam_tbl_set_symmetric_mode_all(
          dev_id, tbl_hdl, o_symmetric, o_num_scopes, &o_scope_pipe_bmp[0]);
    } else if (owner == PIPE_MGR_TBL_OWNER_EXM) {
      pipe_mgr_exm_tbl_set_symmetric_mode(
          dev_id, tbl_hdl, o_symmetric, o_num_scopes, &o_scope_pipe_bmp[0]);
    } else if (owner == PIPE_MGR_TBL_OWNER_ALPM) {
      pipe_mgr_alpm_tbl_set_symmetric_mode(sess_hdl,
                                           dev_id,
                                           tbl_hdl,
                                           o_symmetric,
                                           o_num_scopes,
                                           &o_scope_pipe_bmp[0]);
    }
  } else {
    info->symmetric = symmetric;
  }

  /* Reset the default entry in the table if it was specified by the P4. */
  if (update_dflt) {
    rc = pipe_mgr_tbl_add_init_default_entry(sess_hdl, dev_info, info);
  }

  if (update_hw && (rc == PIPE_SUCCESS)) {
    /* Update the symmetric mode in the HW scratch register for HA */
    profile_id_t profile_id;
    uint32_t stage_id = 0;
    rmt_tbl_hdl_t stage_table_handle = 0;
    rc = pipe_mgr_mat_tbl_get_first_stage_table(
        dev_id, tbl_hdl, &profile_id, &stage_id, &stage_table_handle);
    if (rc != PIPE_SUCCESS) {
      goto cleanup;
    }

    rmt_dev_profile_info_t *dev_profile_info;
    dev_profile_info =
        pipe_mgr_get_profile_info(dev_id, profile_id, __func__, __LINE__);

    bf_dev_pipe_t pipe = PIPE_BITMAP_GET_FIRST_SET(&dev_profile_info->pipe_bmp);
    bf_dev_pipe_t phy_pipe = 0;

    /* Update the symmetricity */
    pipe_mgr_lock_mau_scratch(dev_id);
    uint32_t val;
    val = pipe_mgr_get_mau_scratch_val(dev_id, pipe, stage_id);

    if (symmetric) {
      /* Reset the bit to 0 */
      val &= ~(1 << stage_table_handle);
    } else {
      /* Set the bit to 1 */
      val |= 1 << stage_table_handle;
    }

    pipe_mgr_set_mau_scratch_val(dev_id, pipe, stage_id, val);

    /* Write the value */
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);
    uint32_t reg_addr = 0, reg_val = 0;
    setp_mau_scratch_mau_scratch(&reg_val, val);
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
      default:
        PIPE_MGR_DBGCHK(0);
        rc = PIPE_UNEXPECTED;
        goto cleanup;
    }
    rc = pipe_mgr_drv_reg_wr(
        &sess_hdl, dev_id, phy_pipe / BF_SUBDEV_PIPE_COUNT, reg_addr, reg_val);
    pipe_mgr_unlock_mau_scratch(dev_id);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s: Symmetric mode change failed on resource tables: "
          "sess %d, dev %d, tbl 0x%x, sts %s",
          __func__,
          sess_hdl,
          dev_id,
          tbl_hdl,
          pipe_str_err(rc));
    }
  }

cleanup:
  PIPE_MGR_FREE(o_scope_pipe_bmp);
  return rc;
}

pipe_status_t pipe_mgr_tbl_set_symmetric_mode(pipe_sess_hdl_t sess_hdl,
                                              bf_dev_id_t dev_id,
                                              pipe_mat_tbl_hdl_t tbl_hdl,
                                              bool symmetric,
                                              scope_num_t num_scopes,
                                              scope_pipes_t *scope_pipe_bmp,
                                              bool update_dflt) {
  /* Locking mat tbl locks all the associated tables */
  pipe_hdl_type_t tbl_type = PIPE_GET_HDL_TYPE(tbl_hdl);
  if (tbl_type != PIPE_HDL_TYPE_MAT_TBL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  RMT_API(sess_hdl,
          0,
          pipe_mgr_verify_tbl_access(sess_hdl, dev_id, tbl_hdl, true),
          pipe_mgr_tbl_set_symmetric_mode_wrapper(sess_hdl,
                                                  dev_id,
                                                  tbl_hdl,
                                                  symmetric,
                                                  true,
                                                  num_scopes,
                                                  scope_pipe_bmp,
                                                  update_dflt));
}

pipe_status_t pipe_mgr_tbl_get_property_scope_wrapper(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    bool *symmetric,
    uint32_t *args) {
  pipe_mat_tbl_info_t *mat_tbl_info = NULL;
  int owner = PIPE_MGR_TBL_OWNER_NONE;

  owner = pipe_mgr_sm_tbl_owner(dev_id, tbl_hdl);
  LOG_TRACE("%s: Symmetric mode: dev %d, tbl 0x%x, owner %d ",
            __func__,
            dev_id,
            tbl_hdl,
            owner);

  mat_tbl_info = pipe_mgr_get_tbl_info(dev_id, tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    LOG_ERROR(
        "%s:%d Mat table info for device id %d, table handle 0x%x"
        " not found",
        __func__,
        __LINE__,
        dev_id,
        tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  *symmetric = mat_tbl_info->symmetric;
  *args = mat_tbl_info->scope_value;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tbl_get_property_scope(pipe_sess_hdl_t sess_hdl,
                                              bf_dev_id_t dev_id,
                                              pipe_mat_tbl_hdl_t tbl_hdl,
                                              bool *symmetric,
                                              uint32_t *args) {
  /* Locking mat tbl locks all the associated tables */
  pipe_hdl_type_t tbl_type = PIPE_GET_HDL_TYPE(tbl_hdl);
  if (tbl_type != PIPE_HDL_TYPE_MAT_TBL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  RMT_API(sess_hdl,
          0,
          pipe_mgr_verify_tbl_access(sess_hdl, dev_id, tbl_hdl, true),
          pipe_mgr_tbl_get_property_scope_wrapper(
              dev_id, tbl_hdl, symmetric, args));
}

pipe_status_t pipe_mgr_tbl_set_repeated_notify(pipe_sess_hdl_t sess_hdl,
                                               bf_dev_id_t dev_id,
                                               pipe_mat_tbl_hdl_t tbl_hdl,
                                               bool repeated_notify) {
  /* Locking mat tbl locks all the associated tables */
  pipe_hdl_type_t tbl_type = PIPE_GET_HDL_TYPE(tbl_hdl);
  if (tbl_type != PIPE_HDL_TYPE_MAT_TBL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  RMT_API(
      sess_hdl,
      0,
      pipe_mgr_verify_tbl_access(sess_hdl, dev_id, tbl_hdl, true),
      pipe_mgr_idle_tbl_set_repeated_notify(dev_id, tbl_hdl, repeated_notify));
}

pipe_status_t pipe_mgr_tbl_get_repeated_notify(pipe_sess_hdl_t sess_hdl,
                                               bf_dev_id_t dev_id,
                                               pipe_mat_tbl_hdl_t tbl_hdl,
                                               bool *repeated_notify) {
  /* Locking mat tbl locks all the associated tables */
  pipe_hdl_type_t tbl_type = PIPE_GET_HDL_TYPE(tbl_hdl);
  if (tbl_type != PIPE_HDL_TYPE_MAT_TBL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  RMT_API(
      sess_hdl,
      0,
      pipe_mgr_verify_tbl_access(sess_hdl, dev_id, tbl_hdl, true),
      pipe_mgr_idle_tbl_get_repeated_notify(dev_id, tbl_hdl, repeated_notify));
}

pipe_status_t pipe_mgr_tbl_set_placement_mode(pipe_sess_hdl_t sess_hdl,
                                              bf_dev_id_t dev_id,
                                              pipe_mat_tbl_hdl_t tbl_hdl,
                                              bool placement_app) {
  pipe_status_t rc = PIPE_SUCCESS;
  LOG_TRACE("%s: Placement mode: sess %d, dev %d, tbl 0x%x, app_placement %d ",
            __func__,
            sess_hdl,
            dev_id,
            tbl_hdl,
            placement_app);

  // Need to implement
  return rc;
}

pipe_status_t pipe_mgr_tbl_get_placement_mode(pipe_sess_hdl_t sess_hdl,
                                              bf_dev_id_t dev_id,
                                              pipe_mat_tbl_hdl_t tbl_hdl,
                                              bool *placement_app) {
  pipe_status_t rc = PIPE_SUCCESS;
  LOG_TRACE("%s: Get placement mode: sess %d, dev %d, tbl 0x%x",
            __func__,
            sess_hdl,
            dev_id,
            tbl_hdl);

  // Need to implement
  *placement_app = false;
  return rc;
}

static pipe_status_t pipe_mgr_tbl_set_duplicate_entry_check_wrapper(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    bool duplicate_entry_check) {
  dev_target_t dev_tgt = {.device_id = dev_id, .dev_pipe_id = BF_DEV_PIPE_ALL};
  uint32_t entry_count;
  pipe_status_t ret = PIPE_SUCCESS;
  pipe_mat_tbl_info_t *mat_tbl_info = NULL;
  LOG_TRACE(
      "%s: Duplicate entry check : sess %d, dev %d, tbl 0x%x"
      "check enable %d",
      __func__,
      sess_hdl,
      dev_id,
      tbl_hdl,
      duplicate_entry_check);
  mat_tbl_info = pipe_mgr_get_tbl_info(dev_id, tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    return PIPE_OBJ_NOT_FOUND;
  }

  if (mat_tbl_info->duplicate_entry_check == duplicate_entry_check)
    return PIPE_SUCCESS;

  /* Check if the table is empty. Athough it can have the defult entry
   * present*/
  ret = pipe_mgr_tbl_get_entry_count(dev_tgt, tbl_hdl, true, &entry_count);
  if (ret != PIPE_SUCCESS) {
    return ret;
  }
  if (entry_count > 0 && !mat_tbl_info->static_entries) {
    LOG_ERROR(
        "%s:%d Cannot set duplicate check mode for table 0x%x which is "
        "populated by %d entries.",
        __func__,
        __LINE__,
        mat_tbl_info->handle,
        entry_count);
    return PIPE_NOT_SUPPORTED;
  }
  mat_tbl_info->duplicate_entry_check = duplicate_entry_check;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tbl_set_duplicate_entry_check(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    bool duplicate_check_enable) {
  /* Locking mat tbl locks all the associated tables */
  pipe_hdl_type_t tbl_type = PIPE_GET_HDL_TYPE(tbl_hdl);
  if (tbl_type != PIPE_HDL_TYPE_MAT_TBL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  pipe_mat_tbl_info_t *mat_tbl_info = NULL;
  mat_tbl_info = pipe_mgr_get_tbl_info(dev_id, tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    LOG_ERROR(
        "%s:%d Mat table info for device id %d, table handle 0x%x"
        " not found",
        __func__,
        __LINE__,
        dev_id,
        tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }
  RMT_API(sess_hdl,
          0,
          pipe_mgr_verify_tbl_access(sess_hdl, dev_id, tbl_hdl, true),
          pipe_mgr_tbl_set_duplicate_entry_check_wrapper(
              sess_hdl, dev_id, tbl_hdl, duplicate_check_enable));
}

static pipe_status_t pipe_mgr_tbl_get_duplicate_entry_check_wrapper(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    bool *duplicate_entry_check) {
  pipe_mat_tbl_info_t *mat_tbl_info = NULL;
  LOG_TRACE("%s: Get duplicate entry check : dev %d, tbl 0x%x",
            __func__,
            dev_id,
            tbl_hdl);
  mat_tbl_info = pipe_mgr_get_tbl_info(dev_id, tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    LOG_ERROR(
        "%s:%d Mat table info for device id %d, table handle 0x%x"
        " not found",
        __func__,
        __LINE__,
        dev_id,
        tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  *duplicate_entry_check = mat_tbl_info->duplicate_entry_check;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tbl_get_duplicate_entry_check(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    bool *duplicate_check_enable) {
  /* Locking mat tbl locks all the associated tables */
  pipe_hdl_type_t tbl_type = PIPE_GET_HDL_TYPE(tbl_hdl);
  if (tbl_type != PIPE_HDL_TYPE_MAT_TBL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  RMT_API(sess_hdl,
          0,
          pipe_mgr_verify_tbl_access(sess_hdl, dev_id, tbl_hdl, true),
          pipe_mgr_tbl_get_duplicate_entry_check_wrapper(
              dev_id, tbl_hdl, duplicate_check_enable));
}

static pipe_status_t get_first_placed_entry_handle(pipe_mat_tbl_hdl_t tbl_hdl,
                                                   dev_target_t dev_tgt,
                                                   int *entry_hdl) {
  pipe_status_t rc = PIPE_SUCCESS;
  int owner = PIPE_MGR_TBL_OWNER_NONE;

  owner = pipe_mgr_sm_tbl_owner(dev_tgt.device_id, tbl_hdl);

  if (owner == PIPE_MGR_TBL_OWNER_TRN) {
    rc = pipe_mgr_tcam_get_first_placed_entry_handle(
        tbl_hdl, dev_tgt, entry_hdl);
  } else if (owner == PIPE_MGR_TBL_OWNER_EXM) {
    rc =
        pipe_mgr_exm_get_first_placed_entry_handle(tbl_hdl, dev_tgt, entry_hdl);
  } else if (owner == PIPE_MGR_TBL_OWNER_ALPM) {
    if (!pipe_mgr_is_device_virtual(dev_tgt.device_id)) {
      rc = pipe_mgr_alpm_get_first_entry_handle(tbl_hdl, dev_tgt, entry_hdl);
    } else {
      /* ALPM is not supported on virtual devices. */
      rc = PIPE_INVALID_ARG;
    }
  } else if (owner == PIPE_MGR_TBL_OWNER_ADT) {
    rc =
        pipe_mgr_adt_get_first_placed_entry_handle(tbl_hdl, dev_tgt, entry_hdl);
  } else if (owner == PIPE_MGR_TBL_OWNER_SELECT) {
    rc = pipe_mgr_sel_get_first_entry_handle(tbl_hdl, dev_tgt, entry_hdl);
  } else if (owner == PIPE_MGR_TBL_OWNER_PHASE0) {
    rc = pipe_mgr_phase0_get_first_entry_handle(tbl_hdl, dev_tgt, entry_hdl);
  } else if (owner == PIPE_MGR_TBL_OWNER_NO_KEY) {
    /* For these tables, entry handles are not applicable, return a NOT_FOUND
     * error.
     */
    rc = PIPE_OBJ_NOT_FOUND;
  } else {
    rc = PIPE_INVALID_ARG;
  }

  if (rc == PIPE_SUCCESS) {
    LOG_TRACE("%s: dev %d pipe %x table 0x%x, first handle %d",
              __func__,
              dev_tgt.device_id,
              dev_tgt.dev_pipe_id,
              tbl_hdl,
              *entry_hdl);
  }

  return rc;
}

static pipe_status_t get_next_placed_entry_handles(pipe_mat_tbl_hdl_t tbl_hdl,
                                                   dev_target_t dev_tgt,
                                                   pipe_mat_ent_hdl_t entry_hdl,
                                                   int n,
                                                   int *next_entry_handles) {
  pipe_status_t rc = PIPE_SUCCESS;
  int owner = PIPE_MGR_TBL_OWNER_NONE;

  owner = pipe_mgr_sm_tbl_owner(dev_tgt.device_id, tbl_hdl);
  LOG_TRACE("%s: dev %d pipe %x tbl 0x%x entry_hdl %d count %d",
            __func__,
            dev_tgt.device_id,
            dev_tgt.dev_pipe_id,
            tbl_hdl,
            entry_hdl,
            n);

  if (owner == PIPE_MGR_TBL_OWNER_TRN) {
    rc = pipe_mgr_tcam_get_next_placed_entry_handles(
        tbl_hdl, dev_tgt, entry_hdl, n, next_entry_handles);
  } else if (owner == PIPE_MGR_TBL_OWNER_EXM) {
    rc = pipe_mgr_exm_get_next_placed_entry_handles(
        tbl_hdl, dev_tgt, entry_hdl, n, next_entry_handles);
  } else if (owner == PIPE_MGR_TBL_OWNER_ALPM) {
    if (!pipe_mgr_is_device_virtual(dev_tgt.device_id)) {
      rc = pipe_mgr_alpm_get_next_entry_handles(
          tbl_hdl, dev_tgt, entry_hdl, n, next_entry_handles);
    } else {
      /* ALPM is not supported on virtual devices. */
      rc = PIPE_INVALID_ARG;
    }
  } else if (owner == PIPE_MGR_TBL_OWNER_ADT) {
    rc = pipe_mgr_adt_get_next_placed_entry_handles(
        tbl_hdl, dev_tgt, entry_hdl, n, next_entry_handles);
  } else if (owner == PIPE_MGR_TBL_OWNER_SELECT) {
    rc = pipe_mgr_sel_get_next_entry_handles(
        tbl_hdl, dev_tgt, entry_hdl, n, next_entry_handles);
  } else if (owner == PIPE_MGR_TBL_OWNER_PHASE0) {
    rc = pipe_mgr_phase0_get_next_entry_handles(
        tbl_hdl, dev_tgt, entry_hdl, n, next_entry_handles);
  } else if (owner == PIPE_MGR_TBL_OWNER_NO_KEY) {
    /* For these tables, entry handles are not applicable, return a NOT_FOUND
     * error.
     */
    rc = PIPE_OBJ_NOT_FOUND;
  } else {
    rc = PIPE_INVALID_ARG;
  }

  return rc;
}

static pipe_status_t get_first_programmed_entry_handle(
    pipe_mat_tbl_hdl_t tbl_hdl, dev_target_t dev_tgt, int *entry_hdl) {
  pipe_status_t rc = PIPE_SUCCESS;
  int owner = PIPE_MGR_TBL_OWNER_NONE;

  owner = pipe_mgr_sm_tbl_owner(dev_tgt.device_id, tbl_hdl);

  if (owner == PIPE_MGR_TBL_OWNER_TRN) {
    rc = pipe_mgr_tcam_get_first_programmed_entry_handle(
        tbl_hdl, dev_tgt, entry_hdl);
  } else if (owner == PIPE_MGR_TBL_OWNER_EXM) {
    rc = pipe_mgr_exm_get_first_programmed_entry_handle(
        tbl_hdl, dev_tgt, entry_hdl);
  } else if (owner == PIPE_MGR_TBL_OWNER_ADT) {
    rc = pipe_mgr_adt_get_first_programmed_entry_handle(
        tbl_hdl, dev_tgt, entry_hdl);
  } else if (owner == PIPE_MGR_TBL_OWNER_SELECT) {
    rc = pipe_mgr_sel_get_first_entry_handle(tbl_hdl, dev_tgt, entry_hdl);
  } else if (owner == PIPE_MGR_TBL_OWNER_ALPM) {
    rc = pipe_mgr_alpm_get_first_entry_handle(tbl_hdl, dev_tgt, entry_hdl);
  } else if (owner == PIPE_MGR_TBL_OWNER_PHASE0) {
    rc = pipe_mgr_phase0_get_first_entry_handle(tbl_hdl, dev_tgt, entry_hdl);
  } else if (owner == PIPE_MGR_TBL_OWNER_NO_KEY) {
    /* For these tables, entry handles are not applicable, return a NOT_FOUND
     * error.
     */
    rc = PIPE_OBJ_NOT_FOUND;
  } else {
    rc = PIPE_INVALID_ARG;
  }

  if (rc == PIPE_SUCCESS) {
    LOG_TRACE("%s: dev %d pipe %x table 0x%x, first handle %d",
              __func__,
              dev_tgt.device_id,
              dev_tgt.dev_pipe_id,
              tbl_hdl,
              *entry_hdl);
  }

  return rc;
}

static pipe_status_t get_next_programmed_entry_handles(
    pipe_mat_tbl_hdl_t tbl_hdl,
    dev_target_t dev_tgt,
    pipe_mat_ent_hdl_t entry_hdl,
    int n,
    int *next_entry_handles) {
  pipe_status_t rc = PIPE_SUCCESS;
  int owner = PIPE_MGR_TBL_OWNER_NONE;

  owner = pipe_mgr_sm_tbl_owner(dev_tgt.device_id, tbl_hdl);
  LOG_TRACE("%s: dev %d pipe %x tbl 0x%x entry_hdl %d count %d",
            __func__,
            dev_tgt.device_id,
            dev_tgt.dev_pipe_id,
            tbl_hdl,
            entry_hdl,
            n);

  if (owner == PIPE_MGR_TBL_OWNER_TRN) {
    rc = pipe_mgr_tcam_get_next_programmed_entry_handles(
        tbl_hdl, dev_tgt, entry_hdl, n, next_entry_handles);
  } else if (owner == PIPE_MGR_TBL_OWNER_EXM) {
    rc = pipe_mgr_exm_get_next_programmed_entry_handles(
        tbl_hdl, dev_tgt, entry_hdl, n, next_entry_handles);
  } else if (owner == PIPE_MGR_TBL_OWNER_ADT) {
    rc = pipe_mgr_adt_get_next_programmed_entry_handles(
        tbl_hdl, dev_tgt, entry_hdl, n, next_entry_handles);
  } else if (owner == PIPE_MGR_TBL_OWNER_SELECT) {
    rc = pipe_mgr_sel_get_next_entry_handles(
        tbl_hdl, dev_tgt, entry_hdl, n, next_entry_handles);
  } else if (owner == PIPE_MGR_TBL_OWNER_ALPM) {
    rc = pipe_mgr_alpm_get_next_entry_handles(
        tbl_hdl, dev_tgt, entry_hdl, n, next_entry_handles);
  } else if (owner == PIPE_MGR_TBL_OWNER_PHASE0) {
    rc = pipe_mgr_phase0_get_next_entry_handles(
        tbl_hdl, dev_tgt, entry_hdl, n, next_entry_handles);
  } else if (owner == PIPE_MGR_TBL_OWNER_NO_KEY) {
    /* For these tables, entry handles are not applicable, return a NOT_FOUND
     * error.
     */
    rc = PIPE_OBJ_NOT_FOUND;
  } else {
    rc = PIPE_INVALID_ARG;
  }

  return rc;
}

pipe_status_t pipe_mgr_tbl_get_first_entry_handle(pipe_sess_hdl_t shdl,
                                                  pipe_mat_tbl_hdl_t tbl_hdl,
                                                  dev_target_t dev_tgt,
                                                  int *entry_hdl) {
  if (!entry_hdl) return PIPE_INVALID_ARG;
  if (pipe_mgr_is_device_virtual(dev_tgt.device_id) ||
      pipe_mgr_sess_in_txn(shdl)) {
    return get_first_placed_entry_handle(tbl_hdl, dev_tgt, entry_hdl);
  } else {
    return get_first_programmed_entry_handle(tbl_hdl, dev_tgt, entry_hdl);
  }
}

pipe_status_t pipe_mgr_tbl_get_next_entry_handles(pipe_sess_hdl_t shdl,
                                                  pipe_mat_tbl_hdl_t tbl_hdl,
                                                  dev_target_t dev_tgt,
                                                  pipe_mat_ent_hdl_t entry_hdl,
                                                  int n,
                                                  int *next_entry_handles) {
  if (n < 0) return PIPE_INVALID_ARG;
  if (n && !next_entry_handles) return PIPE_INVALID_ARG;

  if (pipe_mgr_is_device_virtual(dev_tgt.device_id) ||
      pipe_mgr_sess_in_txn(shdl)) {
    return get_next_placed_entry_handles(
        tbl_hdl, dev_tgt, entry_hdl, n, next_entry_handles);
  } else {
    return get_next_programmed_entry_handles(
        tbl_hdl, dev_tgt, entry_hdl, n, next_entry_handles);
  }
}

pipe_status_t pipe_mgr_tbl_get_default_entry_handles(
    pipe_mat_tbl_hdl_t tbl_hdl,
    bf_dev_id_t dev_id,
    pipe_mat_ent_hdl_t *default_hdls,
    uint32_t *num_def_hdls) {
  pipe_status_t rc = PIPE_SUCCESS;
  int owner = PIPE_MGR_TBL_OWNER_NONE;

  owner = pipe_mgr_sm_tbl_owner(dev_id, tbl_hdl);

  if (owner == PIPE_MGR_TBL_OWNER_TRN) {
    rc = pipe_mgr_tcam_get_default_entry_handles(
        tbl_hdl, dev_id, default_hdls, num_def_hdls);
  } else if (owner == PIPE_MGR_TBL_OWNER_EXM) {
    rc = pipe_mgr_exm_get_default_entry_handles(
        tbl_hdl, dev_id, default_hdls, num_def_hdls);
  } else if (owner == PIPE_MGR_TBL_OWNER_ALPM) {
    rc = pipe_mgr_alpm_get_default_entry_handles(
        tbl_hdl, dev_id, default_hdls, num_def_hdls);
  } else if (owner == PIPE_MGR_TBL_OWNER_PHASE0) {
    *num_def_hdls = 0;
    rc = PIPE_SUCCESS;
  } else if (owner == PIPE_MGR_TBL_OWNER_NO_KEY) {
    *num_def_hdls = 0;
    rc = PIPE_SUCCESS;
  } else {
    rc = PIPE_INVALID_ARG;
  }
  return rc;
}

pipe_status_t pipe_mgr_tbl_get_init_default_entry(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    pipe_action_spec_t *action_spec,
    pipe_act_fn_hdl_t *act_fn_hdl) {
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_mgr_action_entry_t *action_entry = NULL;
  pipe_action_data_spec_t act_data_spec = {0};
  uint32_t i = 0;

  pipe_mat_tbl_info_t *mat_tbl_info =
      pipe_mgr_get_tbl_info(dev_id, tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    LOG_ERROR("%s:%d Cannot find match table info for hdl 0x%x device %d",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (!mat_tbl_info->default_info ||
      (mat_tbl_info->adt_tbl_ref &&
       mat_tbl_info->adt_tbl_ref->ref_type == PIPE_TBL_REF_TYPE_INDIRECT)) {
    // For match tables with indirect references to action table, we do not
    // support P4-defined default entries yet. So return NOT_FOUND.
    return PIPE_OBJ_NOT_FOUND;
  }

  action_entry = &mat_tbl_info->default_info->action_entry;
  if (act_fn_hdl) *act_fn_hdl = action_entry->act_fn_hdl;
  if (action_spec == NULL) return PIPE_SUCCESS;
  action_spec->pipe_action_datatype_bmap = PIPE_ACTION_DATA_TYPE;
  if (action_entry->num_act_data) {
    rc = pipe_mgr_create_action_data_spec(
        action_entry->act_data, action_entry->num_act_data, &act_data_spec);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Failed to create action data spec for default entry for tbl "
          "0x%x ",
          __func__,
          __LINE__,
          tbl_hdl);
      return rc;
    }
    if (act_data_spec.num_action_data_bytes >
        action_spec->act_data.num_action_data_bytes) {
      LOG_ERROR(
          "%s:%d %d bytes allocated, but %d bytes needed for default action "
          "spec in table 0x%x device %d",
          __func__,
          __LINE__,
          action_spec->act_data.num_action_data_bytes,
          act_data_spec.num_action_data_bytes,
          tbl_hdl,
          dev_id);
      PIPE_MGR_FREE(act_data_spec.action_data_bits);
      return PIPE_INVALID_ARG;
    }
    PIPE_MGR_MEMCPY(action_spec->act_data.action_data_bits,
                    act_data_spec.action_data_bits,
                    act_data_spec.num_action_data_bytes);
    PIPE_MGR_FREE(act_data_spec.action_data_bits);
  }

  /* Populate indirect resources */
  action_spec->resource_count = 0;
  for (i = 0; i < mat_tbl_info->default_info->action_entry.num_ind_res; i++) {
    pipe_mgr_ind_res_info_t *ind_res = NULL;
    ind_res = &mat_tbl_info->default_info->action_entry.ind_res[i];
    action_spec->resources[action_spec->resource_count].tbl_hdl =
        ind_res->handle;
    action_spec->resources[action_spec->resource_count].tbl_idx = ind_res->idx;
    action_spec->resources[action_spec->resource_count].tag =
        PIPE_RES_ACTION_TAG_ATTACHED;
    action_spec->resource_count++;
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tbl_get_entry_hdlr(
    pipe_mat_tbl_hdl_t tbl_hdl,
    dev_target_t dev_tgt,
    pipe_mat_ent_hdl_t entry_hdl,
    pipe_tbl_match_spec_t *pipe_match_spec,
    pipe_action_spec_t *pipe_action_spec,
    pipe_act_fn_hdl_t *act_fn_hdl,
    bool from_hw) {
  pipe_status_t rc = PIPE_SUCCESS;
  int owner = PIPE_MGR_TBL_OWNER_NONE;
  pipe_mat_tbl_info_t *mat_tbl_info = NULL;

  owner = pipe_mgr_sm_tbl_owner(dev_tgt.device_id, tbl_hdl);
  LOG_TRACE("%s: dev %d, tbl 0x%x, owner %d,  entry_hdl %d ",
            __func__,
            dev_tgt.device_id,
            tbl_hdl,
            owner,
            entry_hdl);
  /* The from_hw flag can only be set on a physical device */
  if (pipe_mgr_is_device_virtual(dev_tgt.device_id) && from_hw) {
    LOG_ERROR(
        "%s:%d Get entry API from hardware not supported on a virtual device, "
        "device id %d, tbl hdl 0x%x, entry hdl %d",
        __func__,
        __LINE__,
        dev_tgt.device_id,
        tbl_hdl,
        entry_hdl);
    return PIPE_NOT_SUPPORTED;
  }

  if (owner == PIPE_MGR_TBL_OWNER_TRN) {
    if (from_hw) {
      rc = pipe_mgr_tcam_get_entry_llp_from_hw(tbl_hdl,
                                               dev_tgt,
                                               entry_hdl,
                                               pipe_match_spec,
                                               pipe_action_spec,
                                               act_fn_hdl);

    } else {
      rc = pipe_mgr_tcam_get_entry(tbl_hdl,
                                   dev_tgt,
                                   entry_hdl,
                                   pipe_match_spec,
                                   pipe_action_spec,
                                   act_fn_hdl);
    }
  } else if (owner == PIPE_MGR_TBL_OWNER_EXM) {
    if (from_hw) {
      rc = pipe_mgr_exm_get_entry_llp_from_hw(tbl_hdl,
                                              dev_tgt,
                                              entry_hdl,
                                              pipe_match_spec,
                                              pipe_action_spec,
                                              act_fn_hdl);

    } else {
      rc = pipe_mgr_exm_get_entry(tbl_hdl,
                                  dev_tgt,
                                  entry_hdl,
                                  pipe_match_spec,
                                  pipe_action_spec,
                                  act_fn_hdl);
    }
  } else if (owner == PIPE_MGR_TBL_OWNER_ALPM) {
    rc = pipe_mgr_alpm_get_entry(tbl_hdl,
                                 dev_tgt,
                                 entry_hdl,
                                 pipe_match_spec,
                                 pipe_action_spec,
                                 act_fn_hdl,
                                 from_hw);
  } else if (owner == PIPE_MGR_TBL_OWNER_PHASE0) {
    rc = pipe_mgr_phase0_get_entry(tbl_hdl,
                                   dev_tgt.device_id,
                                   entry_hdl,
                                   pipe_match_spec,
                                   pipe_action_spec,
                                   act_fn_hdl,
                                   from_hw);
  } else if (owner == PIPE_MGR_TBL_OWNER_NO_KEY) {
    if (entry_hdl != PIPE_MGR_TBL_NO_KEY_DEFAULT_ENTRY_HDL) {
      return PIPE_OBJ_NOT_FOUND;
    }
    mat_tbl_info =
        pipe_mgr_get_tbl_info(dev_tgt.device_id, tbl_hdl, __func__, __LINE__);
    if (mat_tbl_info == NULL) {
      LOG_ERROR(
          "%s:%d Mat table info for device id %d, table handle 0x%x"
          " not found",
          __func__,
          __LINE__,
          dev_tgt.device_id,
          tbl_hdl);
      return PIPE_OBJ_NOT_FOUND;
    }
    if (!mat_tbl_info->tbl_no_key_is_default_entry_valid) {
      /* Indicates that the default action was either never set or reset
         after being set */
      return PIPE_OBJ_NOT_FOUND;
    }
    *act_fn_hdl = mat_tbl_info->act_fn_hdl_info[0].act_fn_hdl;
    if (!mat_tbl_info->adt_tbl_ref) {
      pipe_action_spec->pipe_action_datatype_bmap = PIPE_ACTION_DATA_TYPE;
    } else {
      pipe_action_spec->pipe_action_datatype_bmap = PIPE_ACTION_DATA_HDL_TYPE;
    }
    rc = PIPE_SUCCESS;
  } else {
    rc = PIPE_INVALID_ARG;
  }

  return rc;
}

pipe_status_t pipe_mgr_tbl_get_action_data_hdlr(
    pipe_adt_tbl_hdl_t tbl_hdl,
    dev_target_t dev_tgt,
    pipe_adt_ent_hdl_t entry_hdl,
    pipe_action_data_spec_t *pipe_action_data_spec,
    pipe_act_fn_hdl_t *act_fn_hdl,
    bool from_hw) {
  pipe_status_t rc = PIPE_SUCCESS;
  int owner = PIPE_MGR_TBL_OWNER_NONE;

  owner = pipe_mgr_sm_tbl_owner(dev_tgt.device_id, tbl_hdl);
  LOG_TRACE("%s: dev %d, tbl 0x%x, owner %d,  entry_hdl %d",
            __func__,
            dev_tgt.device_id,
            tbl_hdl,
            owner,
            entry_hdl);

  if (owner == PIPE_MGR_TBL_OWNER_ADT) {
    rc = pipe_mgr_adt_get_entry(tbl_hdl,
                                dev_tgt,
                                entry_hdl,
                                pipe_action_data_spec,
                                act_fn_hdl,
                                from_hw);
  } else {
    rc = PIPE_INVALID_ARG;
  }

  return rc;
}

pipe_status_t pipe_mgr_tbl_get_entry_count(dev_target_t dev_tgt,
                                           pipe_tbl_hdl_t tbl_hdl,
                                           bool read_from_hw,
                                           uint32_t *count_p) {
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_hdl_type_t tbl_type = PIPE_GET_HDL_TYPE(tbl_hdl);

  *count_p = 0;
  /*
   * If read_from_hw is enabled but the device is actually virtual,
   * then we should just return the sw value
   */
  switch (tbl_type) {
    case PIPE_HDL_TYPE_MAT_TBL:
      if (pipe_mgr_is_device_virtual(dev_tgt.device_id) || (!read_from_hw)) {
        rc = get_placed_entry_count(dev_tgt, tbl_hdl, count_p);
      } else {
        rc = get_programmed_entry_count(dev_tgt, tbl_hdl, count_p);
      }
      break;
    case PIPE_HDL_TYPE_ADT_TBL:
      if (pipe_mgr_is_device_virtual(dev_tgt.device_id) || (!read_from_hw)) {
        rc = pipe_mgr_adt_tbl_get_num_entries_placed(dev_tgt, tbl_hdl, count_p);
      } else {
        rc = pipe_mgr_adt_tbl_get_num_entries_programmed(
            dev_tgt, tbl_hdl, count_p);
      }
      break;
    case PIPE_HDL_TYPE_SEL_TBL:
      if (pipe_mgr_is_device_virtual(dev_tgt.device_id) || (!read_from_hw)) {
        rc = pipe_mgr_sel_tbl_get_placed_entry_count(dev_tgt, tbl_hdl, count_p);
      } else {
        rc = pipe_mgr_sel_tbl_get_programmed_entry_count(
            dev_tgt, tbl_hdl, count_p);
      }
      break;
    case PIPE_HDL_TYPE_STAT_TBL:
      break;
    case PIPE_HDL_TYPE_METER_TBL:
      break;
    case PIPE_HDL_TYPE_STFUL_TBL:
      break;
    default:
      rc = PIPE_INVALID_ARG;
      break;
  }
  return rc;
}

pipe_status_t pipe_mgr_tbl_get_total_hw_entry_count(dev_target_t dev_tgt,
                                                    pipe_tbl_hdl_t tbl_hdl,
                                                    size_t *count_p) {
  pipe_status_t rc = PIPE_SUCCESS;
  *count_p = 0;

  pipe_hdl_type_t tbl_type = PIPE_GET_HDL_TYPE(tbl_hdl);
  if (tbl_type != PIPE_HDL_TYPE_MAT_TBL) {
    return rc;
  }

  enum pipe_mgr_table_owner_t owner =
      pipe_mgr_sm_tbl_owner(dev_tgt.device_id, tbl_hdl);

  switch (owner) {
    case PIPE_MGR_TBL_OWNER_TRN:
      rc = pipe_mgr_tcam_get_total_entry_count(dev_tgt, tbl_hdl, count_p);
      break;
    case PIPE_MGR_TBL_OWNER_ALPM:
    case PIPE_MGR_TBL_OWNER_EXM:
    case PIPE_MGR_TBL_OWNER_PHASE0:
    case PIPE_MGR_TBL_OWNER_NO_KEY:
      /* Only TCAM range tables use multiple physical entries to represent one
       * logical entry, other table types have a one-to-one mapping from the
       * logical entry to physical entry so the SW and HW usage are the same,
       * return not supported here with the expectation that the caller will
       * use the SW count instead.*/
      rc = PIPE_NOT_SUPPORTED;
      break;
    default:
      rc = PIPE_INVALID_ARG;
  }

  return rc;
}

pipe_status_t pipe_mgr_tbl_get_reserved_entry_count(dev_target_t dev_tgt,
                                                    pipe_tbl_hdl_t tbl_hdl,
                                                    size_t *count_p) {
  *count_p = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_hdl_type_t tbl_type = PIPE_GET_HDL_TYPE(tbl_hdl);
  if (tbl_type != PIPE_HDL_TYPE_MAT_TBL) {
    return rc;
  }

  enum pipe_mgr_table_owner_t owner =
      pipe_mgr_sm_tbl_owner(dev_tgt.device_id, tbl_hdl);
  if (owner != PIPE_MGR_TBL_OWNER_TRN) {
    return rc;
  }

  return pipe_mgr_tcam_get_reserved_entry_count(dev_tgt, tbl_hdl, count_p);
}

pipe_status_t pipe_mgr_tbl_default_entry_needs_reserve(
    rmt_dev_info_t *dev_info,
    pipe_mat_tbl_info_t *tbl_info,
    bool idle_tbl_present,
    bool *reservation_required) {
  /* Idletime is a direct resource and is present even for the default entry. */
  if (idle_tbl_present) {
    *reservation_required = true;
    return PIPE_SUCCESS;
  }

  /* Go over each action the table has and check if it is a default action and
   * whether or not it requires direct resources. */
  for (uint32_t j, i = 0; i < tbl_info->num_actions; ++i) {
    pipe_act_fn_info_t *info = &tbl_info->act_fn_hdl_info[i];
    /* If the action cannot be a default action then skip it.  Note the for loop
     * with no body, if the action is in the exclude list the loop exits early
     * otherwise it exits once j is equal to the list length. */
    for (j = 0; j < tbl_info->def_act_blacklist_size &&
                info->act_fn_hdl != tbl_info->def_act_blacklist[j];
         ++j)
      ;
    if (j != tbl_info->def_act_blacklist_size) continue;

    /* The action could be a default action so check if it requires a direct
     * resource.  If so, a table entry must be reserved.  The possible direct
     * resources are: action data, counter, meter/lpf/wred, stateful.  The
     * action info already says if it uses any of these except for direct
     * action. */
    if (info->dir_stat_hdl || info->dir_meter_hdl || info->dir_stful_hdl) {
      *reservation_required = true;
      return PIPE_SUCCESS;
    }
    /* If there is a direct action table query entry-format to check if the
     * entry encoding uses a direct action table. */
    for (j = 0; j < tbl_info->num_adt_tbl_refs; ++j) {
      if (tbl_info->adt_tbl_ref[j].ref_type != PIPE_TBL_REF_TYPE_DIRECT)
        continue;
      bool dir_adt = false;
      dev_stage_t stage_id;
      if (tbl_info->keyless_info) {
        stage_id = tbl_info->keyless_info->stage_id;
      } else if (tbl_info->num_rmt_info) {
        stage_id = tbl_info->rmt_info[tbl_info->num_rmt_info - 1].stage_id;
      } else {
        LOG_ERROR("Dev %d cannot find stage for table %s (0x%x)",
                  dev_info->dev_id,
                  tbl_info->name,
                  tbl_info->handle);
        return PIPE_UNEXPECTED;
      }
      pipe_status_t rc =
          pipe_mgr_entry_format_adt_tbl_used(dev_info,
                                             tbl_info->profile_id,
                                             stage_id,
                                             tbl_info->adt_tbl_ref[j].tbl_hdl,
                                             info->act_fn_hdl,
                                             &dir_adt);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "Dev %d cannot find action info for action function handle 0x%x "
            "stage %d table %s (ADT 0x%x), %s",
            dev_info->dev_id,
            info->act_fn_hdl,
            tbl_info->rmt_info[tbl_info->num_rmt_info - 1].stage_id,
            tbl_info->name,
            tbl_info->adt_tbl_ref[j].tbl_hdl,
            pipe_str_err(rc));
        return rc;
      }
      if (dir_adt) {
        *reservation_required = true;
        return PIPE_SUCCESS;
      }
    }
  }
  *reservation_required = false;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tbl_stage_idx_to_hdl(bf_dev_id_t dev_id,
                                            pipe_mat_tbl_hdl_t tbl_hdl,
                                            bf_dev_pipe_t pipe,
                                            uint8_t stage,
                                            uint8_t logical_tbl_id,
                                            uint32_t hit_addr,
                                            pipe_mat_ent_hdl_t *entry_hdl) {
  pipe_hdl_type_t tbl_type = PIPE_GET_HDL_TYPE(tbl_hdl);
  if (PIPE_HDL_TYPE_MAT_TBL != tbl_type) {
    return PIPE_INVALID_ARG;
  }
  pipe_sess_hdl_t sess_hdl = pipe_mgr_ctx->int_ses_hndl;
  dev_target_t dev_tgt;
  pipe_status_t rc;

  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = pipe;
  rc = pipe_mgr_verify_pipe_tbl_access(sess_hdl, dev_tgt, tbl_hdl, true);
  if (rc != PIPE_SUCCESS) return rc;

  pipe_mat_ent_hdl_t x;
  int owner = pipe_mgr_sm_tbl_owner(dev_id, tbl_hdl);
  if (owner == PIPE_MGR_TBL_OWNER_TRN) {
    rc = pipe_mgr_tcam_entry_hdl_from_stage_idx(
        dev_id, pipe, tbl_hdl, stage, logical_tbl_id, hit_addr, &x);
  } else if (owner == PIPE_MGR_TBL_OWNER_ALPM) {
    rc = pipe_mgr_alpm_entry_hdl_from_stage_idx(
        dev_id, pipe, tbl_hdl, stage, logical_tbl_id, hit_addr, &x);
  } else if (owner == PIPE_MGR_TBL_OWNER_EXM) {
    rc = pipe_mgr_exm_entry_hdl_from_stage_idx(
        dev_id, pipe, tbl_hdl, stage, logical_tbl_id, hit_addr, &x);
  } else if (owner == PIPE_MGR_TBL_OWNER_PHASE0) {
    PIPE_MGR_DBGCHK(0);
    rc = PIPE_INVALID_ARG;
  } else {
    rc = PIPE_INVALID_ARG;
  }

  if (!pipe_mgr_sess_in_batch(sess_hdl) && !pipe_mgr_sess_in_txn(sess_hdl)) {
    pipe_mgr_sm_release(sess_hdl);
  }

  if (PIPE_SUCCESS == rc) *entry_hdl = x;
  return rc;
}

static pipe_status_t pipe_mgr_tbl_is_atcam(pipe_mat_tbl_info_t *match_table,
                                           bool *is_atcam) {
  if (!match_table) {
    return PIPE_OBJ_NOT_FOUND;
  }

  switch (match_table->match_type) {
    case ATCAM_MATCH:
      *is_atcam = true;
      break;
    case TERNARY_MATCH:
    case LONGEST_PREFIX_MATCH:
    case EXACT_MATCH:
    case ALPM_MATCH:
      *is_atcam = false;
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_form_htbl_key(
    uint32_t key_sz,
    const pipe_tbl_match_spec_t *match_spec,
    uint8_t **key_p,
    bool is_atcam) {
  uint32_t computed_key_sz = 0;
  (*key_p) = (uint8_t *)PIPE_MGR_CALLOC(key_sz, sizeof(uint8_t));
  if (NULL == (*key_p)) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  uint32_t spec_size = match_spec->num_match_bytes;
  if (match_spec->num_match_bytes) {
    PIPE_MGR_MEMCPY(
        *key_p, match_spec->match_value_bits, match_spec->num_match_bytes);
  }

  computed_key_sz = (2 * spec_size) + sizeof(match_spec->priority);
  if (is_atcam) {
    computed_key_sz += sizeof(match_spec->partition_index);
  }

  /* Addition of partition index in the key for hash table lookup. */
  if (key_sz == computed_key_sz) {
    if (match_spec->num_match_bytes) {
      PIPE_MGR_MEMCPY(&((*key_p)[spec_size]),
                      match_spec->match_mask_bits,
                      match_spec->num_match_bytes);
    }
    if (is_atcam) {
      PIPE_MGR_MEMCPY(&((*key_p)[2 * spec_size]),
                      &(match_spec->partition_index),
                      sizeof(match_spec->partition_index));
      PIPE_MGR_MEMCPY(
          &((*key_p)[(2 * spec_size) + sizeof(match_spec->partition_index)]),
          &(match_spec->priority),
          sizeof(match_spec->priority));
    } else {
      PIPE_MGR_MEMCPY(&((*key_p)[2 * spec_size]),
                      &(match_spec->priority),
                      sizeof(match_spec->priority));
    }
  }

  return PIPE_SUCCESS;
}

static int pipe_mgr_cmp_match_value_bits(
    const void *key1,
    const pipe_tbl_match_spec_t *match_spec2,
    bool is_tern,
    bool is_atcam) {
  uint8_t *key2 = NULL;
  uint32_t key_sz;
  int ret;

  if (!key1) {
    return -1;
  }

  key_sz = match_spec2->num_match_bytes;
  if (is_tern || is_atcam) {
    key_sz *= 2;
    if (is_atcam) {
      key_sz += sizeof(match_spec2->partition_index);
    }
    key_sz += sizeof(match_spec2->priority);
  }

  if ((pipe_mgr_form_htbl_key(key_sz, match_spec2, &key2, is_atcam) ==
       PIPE_NO_SYS_RESOURCES) ||
      !key2) {
    return -1;
  }

  ret = PIPE_MGR_MEMCMP((void *)key1, (void *)key2, key_sz);
  if (ret != 0) {
    if (key2) {
      PIPE_MGR_FREE(key2);
    }
    return -1;
  }
  if (key2) {
    PIPE_MGR_FREE(key2);
  }
  return 0;
}

/* This is the compare function which is invoked by the hashtbl library to
 * compare the key found at a certain hash location. The first argument to the
 * function is an argument that is passed in when initiating the search and
 * the
 * the second argument is the hash table node.
 */
static int pipe_mgr_mat_exm_key_cmp_fn(const void *arg, const void *key1) {
  if (key1 == NULL || arg == NULL) {
    PIPE_MGR_DBGCHK(0);
    return -1;
  }

  void *key2 = bf_hashtbl_get_cmp_data(key1);
  if (key1 == NULL || key2 == NULL) {
    return -1;
  }
  pipe_tbl_match_spec_t *match_spec2 =
      (((pipe_mgr_mat_key_htbl_node_t *)key2)->match_spec);

  return pipe_mgr_cmp_match_value_bits(arg, match_spec2, false, false);
}

static int pipe_mgr_mat_tern_key_cmp_fn(const void *arg, const void *key1) {
  if (key1 == NULL || arg == NULL) {
    PIPE_MGR_DBGCHK(0);
    return -1;
  }

  void *key2 = bf_hashtbl_get_cmp_data(key1);
  if (key1 == NULL || key2 == NULL) {
    return -1;
  }
  pipe_tbl_match_spec_t *match_spec2 =
      (((pipe_mgr_mat_key_htbl_node_t *)key2)->match_spec);

  return pipe_mgr_cmp_match_value_bits(arg, match_spec2, true, false);
}

static int pipe_mgr_mat_atcam_tern_key_cmp_fn(const void *arg,
                                              const void *key1) {
  if (key1 == NULL || arg == NULL) {
    PIPE_MGR_DBGCHK(0);
    return -1;
  }

  void *key2 = bf_hashtbl_get_cmp_data(key1);
  if (key1 == NULL || key2 == NULL) {
    return -1;
  }
  pipe_tbl_match_spec_t *match_spec2 =
      (((pipe_mgr_mat_key_htbl_node_t *)key2)->match_spec);

  return pipe_mgr_cmp_match_value_bits(arg, match_spec2, true, true);
}

static int pipe_mgr_txn_mat_exm_key_cmp_fn(const void *arg, const void *key1) {
  if (key1 == NULL || arg == NULL) {
    PIPE_MGR_DBGCHK(0);
    return -1;
  }

  void *key2 = bf_hashtbl_get_cmp_data(key1);
  if (key1 == NULL || key2 == NULL) {
    return -1;
  }
  pipe_tbl_match_spec_t *match_spec2 =
      (((pipe_mgr_mat_txn_log_htbl_node_t *)key2)->htbl_node->match_spec);

  return pipe_mgr_cmp_match_value_bits(arg, match_spec2, false, false);
}

static int pipe_mgr_txn_mat_tern_key_cmp_fn(const void *arg, const void *key1) {
  if (key1 == NULL || arg == NULL) {
    PIPE_MGR_DBGCHK(0);
    return -1;
  }

  void *key2 = bf_hashtbl_get_cmp_data(key1);
  if (key1 == NULL || key2 == NULL) {
    return -1;
  }
  pipe_tbl_match_spec_t *match_spec2 =
      (((pipe_mgr_mat_txn_log_htbl_node_t *)key2)->htbl_node->match_spec);

  return pipe_mgr_cmp_match_value_bits(arg, match_spec2, true, false);
}

static int pipe_mgr_txn_mat_atcam_tern_key_cmp_fn(const void *arg,
                                                  const void *key1) {
  if (key1 == NULL || arg == NULL) {
    PIPE_MGR_DBGCHK(0);
    return -1;
  }

  void *key2 = bf_hashtbl_get_cmp_data(key1);
  if (key1 == NULL || key2 == NULL) {
    return -1;
  }
  pipe_tbl_match_spec_t *match_spec2 =
      (((pipe_mgr_mat_txn_log_htbl_node_t *)key2)->htbl_node->match_spec);

  return pipe_mgr_cmp_match_value_bits(arg, match_spec2, true, true);
}

pipe_status_t pipe_mgr_mat_tbl_key_exists(pipe_mat_tbl_info_t *mat_tbl_info,
                                          pipe_tbl_match_spec_t *ms,
                                          bf_dev_pipe_t pipe_id,
                                          bool *exists,
                                          pipe_mat_ent_hdl_t *mat_ent_hdl) {
  pipe_mgr_mat_key_htbl_node_t *htbl_node = NULL;
  uint8_t pipe_idx = 0;
  uint8_t *key_p;
  uint32_t key_sz;
  pipe_tbl_match_spec_t *match_spec, *dkm_match_spec;
  pipe_tbl_match_spec_t exm_matchspec;
  uint8_t match_value_bits[TOF_MAX_RAM_WORDS_IN_EXM_TBL_WORD *
                           TOF_BYTES_IN_RAM_WORD];
  bool is_atcam = false;

  if (pipe_id == BF_DEV_PIPE_ALL) {
    pipe_idx = 0;
  } else {
    pipe_idx = pipe_id;
  }

  if (mat_tbl_info->duplicate_entry_check == false) {
    *exists = false;
    return PIPE_SUCCESS;
  }

  if (mat_tbl_info->key_htbl == NULL) {
    /* The very first entry getting added into this table */
    *exists = false;
    return PIPE_SUCCESS;
  }

  match_spec = ms;
  exm_matchspec.match_value_bits = match_value_bits;
  dkm_match_spec = &exm_matchspec;
  if ((pipe_mgr_get_exm_key_with_dkm_mask(mat_tbl_info, ms, dkm_match_spec)) ==
      PIPE_SUCCESS) {
    // EXM table has dynamic key mask property. Change key to include bits as
    // per key-mask.
    match_spec = dkm_match_spec;
  }
  if (pipe_mgr_tbl_is_atcam(mat_tbl_info, &is_atcam) != PIPE_SUCCESS) {
    return PIPE_UNEXPECTED;
  }

  key_sz = match_spec->num_match_bytes;
  if (mat_tbl_info->match_type != EXACT_MATCH) {
    key_sz *= 2;
    if (is_atcam) {
      key_sz += sizeof(match_spec->partition_index);
    }
    key_sz += sizeof(match_spec->priority);
  }
  if (pipe_mgr_form_htbl_key(key_sz, match_spec, &key_p, is_atcam) ==
      PIPE_NO_SYS_RESOURCES) {
    return PIPE_NO_SYS_RESOURCES;
  }
  htbl_node = bf_hashtbl_search(mat_tbl_info->key_htbl[pipe_idx], key_p);

  if (htbl_node == NULL) {
    *exists = false;
  } else {
    *mat_ent_hdl = htbl_node->mat_ent_hdl;
    *exists = true;
  }
  if (key_p) {
    PIPE_MGR_FREE(key_p);
  }
  return PIPE_SUCCESS;
}

static void pipe_mgr_free_mat_key_txn_node(void *obj) {
  pipe_mgr_mat_txn_log_htbl_node_t *txn_log_node = obj;
  if (txn_log_node->htbl_node) {
    pipe_mgr_free_key_htbl_node(txn_log_node->htbl_node);
  }
  PIPE_MGR_FREE(txn_log_node);
  return;
}

static pipe_status_t pipe_mgr_mat_tbl_key_insert_internal(
    pipe_mat_tbl_info_t *mat_tbl_info,
    pipe_tbl_match_spec_t *ms,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    bf_dev_pipe_t pipe_id,
    bool isTxn) {
  bf_hashtbl_sts_t htbl_sts = BF_HASHTBL_OK;
  pipe_mgr_mat_key_htbl_node_t *htbl_node = NULL;
  pipe_mgr_mat_txn_log_htbl_node_t *txn_log_node = NULL;
  uint32_t key_sz = 0;
  uint8_t *key_p = NULL;
  uint8_t pipe_idx = 0;
  pipe_tbl_match_spec_t *match_spec, *dkm_match_spec;
  pipe_tbl_match_spec_t exm_matchspec;
  uint8_t match_value_bits[TOF_MAX_RAM_WORDS_IN_EXM_TBL_WORD *
                           TOF_BYTES_IN_RAM_WORD];
  bool is_atcam = false;
  int (*cmp_fn)(const void *, const void *) = NULL;

  if (pipe_id == BF_DEV_PIPE_ALL) {
    pipe_idx = 0;
  } else {
    pipe_idx = pipe_id;
  }

  match_spec = ms;
  exm_matchspec.match_value_bits = match_value_bits;
  dkm_match_spec = &exm_matchspec;
  if ((pipe_mgr_get_exm_key_with_dkm_mask(mat_tbl_info, ms, dkm_match_spec)) ==
      PIPE_SUCCESS) {
    // EXM table has dynamic key mask property. Change key to include bits as
    // per key-mask.
    match_spec = dkm_match_spec;
  }

  if (pipe_mgr_tbl_is_atcam(mat_tbl_info, &is_atcam) != PIPE_SUCCESS) {
    return PIPE_UNEXPECTED;
  }

  if (mat_tbl_info->key_htbl[pipe_idx] == NULL) {
    /* Hash table is not initialized, may be the first entry that is getting
     * added.
     */
    mat_tbl_info->key_htbl[pipe_idx] =
        (bf_hashtable_t *)PIPE_MGR_CALLOC(1, sizeof(bf_hashtable_t));
    key_sz = match_spec->num_match_bytes;
    if (mat_tbl_info->match_type == EXACT_MATCH) {
      cmp_fn = pipe_mgr_mat_exm_key_cmp_fn;
    } else {
      key_sz *= 2;
      key_sz += sizeof(match_spec->priority);
      if (is_atcam) {
        key_sz += sizeof(match_spec->partition_index);
        cmp_fn = pipe_mgr_mat_atcam_tern_key_cmp_fn;
      } else {
        cmp_fn = pipe_mgr_mat_tern_key_cmp_fn;
      }
    }
    htbl_sts = bf_hashtbl_init(mat_tbl_info->key_htbl[pipe_idx],
                               cmp_fn,
                               pipe_mgr_free_key_htbl_node,
                               key_sz,
                               sizeof(pipe_mgr_mat_key_htbl_node_t),
                               0x98733423);

    if (htbl_sts != BF_HASHTBL_OK) {
      LOG_ERROR(
          "%s:%d Error in initializing hashtable for match table key"
          " table 0x%x",
          __func__,
          __LINE__,
          mat_tbl_info->handle);
      return PIPE_UNEXPECTED;
    }
  }

  htbl_node = (pipe_mgr_mat_key_htbl_node_t *)PIPE_MGR_CALLOC(
      1, sizeof(pipe_mgr_mat_key_htbl_node_t));
  if (htbl_node == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  htbl_node->match_spec = pipe_mgr_tbl_copy_match_spec(NULL, match_spec);
  if (htbl_node->match_spec == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  htbl_node->mat_ent_hdl = mat_ent_hdl;

  key_sz = match_spec->num_match_bytes;
  if (mat_tbl_info->match_type != EXACT_MATCH) {
    key_sz *= 2;
    if (is_atcam) {
      key_sz += sizeof(match_spec->partition_index);
    }
    key_sz += sizeof(match_spec->priority);
  }

  if (pipe_mgr_form_htbl_key(key_sz, match_spec, &key_p, is_atcam) ==
      PIPE_NO_SYS_RESOURCES) {
    return PIPE_NO_SYS_RESOURCES;
  }
  htbl_sts =
      bf_hashtbl_insert(mat_tbl_info->key_htbl[pipe_idx], htbl_node, key_p);

  if (htbl_sts != BF_HASHTBL_OK) {
    LOG_ERROR(
        "%s:%d Error in inserting match spec into the key hash tbl"
        " for tbl 0x%x",
        __func__,
        __LINE__,
        mat_tbl_info->handle);
    if (key_p) {
      PIPE_MGR_FREE(key_p);
    }
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }
  /* If this operation was done as part of a transaction, log this operation
   * This will be used to clean up the entries if the transaction aborts.
   */
  if (isTxn) {
    if (mat_tbl_info->txn_log_htbl[pipe_idx] == NULL) {
      mat_tbl_info->txn_log_htbl[pipe_idx] =
          (bf_hashtable_t *)PIPE_MGR_CALLOC(1, sizeof(bf_hashtable_t));
      if (mat_tbl_info->txn_log_htbl[pipe_idx] == NULL) {
        LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
        if (key_p) {
          PIPE_MGR_FREE(key_p);
        }
        return PIPE_NO_SYS_RESOURCES;
      }
      if (mat_tbl_info->match_type == EXACT_MATCH) {
        cmp_fn = pipe_mgr_txn_mat_exm_key_cmp_fn;
      } else {
        if (is_atcam) {
          cmp_fn = pipe_mgr_txn_mat_atcam_tern_key_cmp_fn;
        } else {
          cmp_fn = pipe_mgr_txn_mat_tern_key_cmp_fn;
        }
      }
      htbl_sts = bf_hashtbl_init(mat_tbl_info->txn_log_htbl[pipe_idx],
                                 cmp_fn,
                                 pipe_mgr_free_mat_key_txn_node,
                                 key_sz,
                                 sizeof(pipe_mgr_mat_txn_log_htbl_node_t),
                                 0x82342341);
      if (htbl_sts != BF_HASHTBL_OK) {
        LOG_ERROR(
            "%s:%d Error in initializing transaction log hash table for"
            " table 0x%x",
            __func__,
            __LINE__,
            mat_tbl_info->handle);
        if (key_p) {
          PIPE_MGR_FREE(key_p);
        }
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
      }
    }
    /* Check if this match spec was already operated upon in the transaction
     */
    txn_log_node =
        bf_hashtbl_get_remove(mat_tbl_info->txn_log_htbl[pipe_idx], key_p);

    if (txn_log_node != NULL) {
      PIPE_MGR_DBGCHK(txn_log_node->operation == PIPE_MGR_MAT_OPERATION_DELETE);
      pipe_mgr_free_mat_key_txn_node(txn_log_node);
    } else {
      txn_log_node = (pipe_mgr_mat_txn_log_htbl_node_t *)PIPE_MGR_CALLOC(
          1, sizeof(pipe_mgr_mat_txn_log_htbl_node_t));
      if (txn_log_node == NULL) {
        LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
        if (key_p) {
          PIPE_MGR_FREE(key_p);
        }
        return PIPE_NO_SYS_RESOURCES;
      }
      txn_log_node->htbl_node =
          PIPE_MGR_CALLOC(1, sizeof(pipe_mgr_mat_key_htbl_node_t));
      if (txn_log_node->htbl_node == NULL) {
        LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
        if (txn_log_node) PIPE_MGR_FREE(txn_log_node);
        return PIPE_NO_SYS_RESOURCES;
      }
      txn_log_node->htbl_node->mat_ent_hdl = mat_ent_hdl;
      txn_log_node->htbl_node->match_spec =
          pipe_mgr_tbl_copy_match_spec(NULL, match_spec);
      txn_log_node->operation = PIPE_MGR_MAT_OPERATION_ADD;
      txn_log_node->pipe_idx = pipe_id;
      /* Insert the node into the txn log hash table */
      htbl_sts = bf_hashtbl_insert(
          mat_tbl_info->txn_log_htbl[pipe_idx], txn_log_node, key_p);
      if (htbl_sts != BF_HASHTBL_OK) {
        LOG_ERROR(
            "%s:%d Error in inserting txn log node into hash table"
            " for table 0x%x",
            __func__,
            __LINE__,
            mat_tbl_info->handle);
        if (key_p) {
          PIPE_MGR_FREE(key_p);
        }
        return PIPE_UNEXPECTED;
      }
    }
  }
  if (key_p) {
    PIPE_MGR_FREE(key_p);
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_mat_tbl_key_insert(bf_dev_id_t device_id,
                                          pipe_mat_tbl_info_t *mat_tbl_info,
                                          pipe_tbl_match_spec_t *match_spec,
                                          pipe_mat_ent_hdl_t mat_ent_hdl,
                                          bf_dev_pipe_t pipe_id,
                                          bool isTxn) {
  uint32_t num_pipelines = 0;

  if (mat_tbl_info->duplicate_entry_check == false) {
    return PIPE_SUCCESS;
  }
  /* If the hash tables are not allocated, allocate it */
  if (mat_tbl_info->key_htbl == NULL) {
    num_pipelines = pipe_mgr_get_num_active_pipes(device_id);
    mat_tbl_info->key_htbl = (bf_hashtable_t **)PIPE_MGR_CALLOC(
        num_pipelines, sizeof(bf_hashtable_t *));
    if (mat_tbl_info->key_htbl == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    PIPE_MGR_DBGCHK(mat_tbl_info->txn_log_htbl == NULL);
    if (mat_tbl_info->txn_log_htbl == NULL) {
      mat_tbl_info->txn_log_htbl = (bf_hashtable_t **)PIPE_MGR_CALLOC(
          num_pipelines, sizeof(bf_hashtable_t *));
      if (mat_tbl_info->txn_log_htbl == NULL) {
        LOG_ERROR("%s;%d Malloc failure", __func__, __LINE__);
        return PIPE_NO_SYS_RESOURCES;
      }
    }
  }
  return pipe_mgr_mat_tbl_key_insert_internal(
      mat_tbl_info, match_spec, mat_ent_hdl, pipe_id, isTxn);
}

void pipe_mgr_free_key_htbl_node(void *node) {
  pipe_mgr_mat_key_htbl_node_t *htbl_node = node;
  if (htbl_node == NULL) {
    return;
  }
  if (htbl_node->match_spec) {
    pipe_tbl_match_spec_t *match_spec = htbl_node->match_spec;
    if (match_spec->num_match_bytes > 0) {
      if (match_spec->match_value_bits) {
        PIPE_MGR_FREE(match_spec->match_value_bits);
      }
      if (match_spec->match_mask_bits) {
        PIPE_MGR_FREE(match_spec->match_mask_bits);
      }
    }
    PIPE_MGR_FREE(htbl_node->match_spec);
  }
  PIPE_MGR_FREE(htbl_node);
  return;
}

static pipe_status_t pipe_mgr_mat_tbl_key_delete_internal(
    bf_dev_id_t device_id,
    pipe_mat_tbl_info_t *mat_tbl_info,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    bool isTxn,
    bf_dev_pipe_t pipe_id,
    pipe_tbl_match_spec_t *ms) {
  pipe_mgr_mat_txn_log_htbl_node_t *txn_log_node = NULL;
  pipe_mgr_mat_key_htbl_node_t *htbl_node = NULL;
  bf_hashtbl_sts_t htbl_sts = BF_HASHTBL_OK;
  bool is_atcam = false;
  uint8_t *key_p = NULL;
  uint32_t key_sz = 0;
  pipe_tbl_match_spec_t *match_spec, *dkm_match_spec;
  pipe_tbl_match_spec_t exm_matchspec;
  uint8_t match_value_bits[TOF_MAX_RAM_WORDS_IN_EXM_TBL_WORD *
                           TOF_BYTES_IN_RAM_WORD];
  int (*cmp_fn)(const void *, const void *) = NULL;

  match_spec = ms;
  exm_matchspec.match_value_bits = match_value_bits;
  dkm_match_spec = &exm_matchspec;
  if ((pipe_mgr_get_exm_key_with_dkm_mask(mat_tbl_info, ms, dkm_match_spec)) ==
      PIPE_SUCCESS) {
    // EXM table has dynamic key mask property. Change key to include bits as
    // per key-mask.
    match_spec = dkm_match_spec;
  }
  if (pipe_mgr_tbl_is_atcam(mat_tbl_info, &is_atcam) != PIPE_SUCCESS) {
    return PIPE_UNEXPECTED;
  }
  key_sz = match_spec->num_match_bytes;
  if (mat_tbl_info->match_type != EXACT_MATCH) {
    key_sz *= 2;
    if (is_atcam) {
      key_sz += sizeof(match_spec->partition_index);
    }
    key_sz += sizeof(match_spec->priority);
  }
  if (pipe_mgr_form_htbl_key(key_sz, match_spec, &key_p, is_atcam) ==
      PIPE_NO_SYS_RESOURCES) {
    return PIPE_NO_SYS_RESOURCES;
  }
  if (pipe_id == BF_DEV_PIPE_ALL) {
    pipe_id = 0;
  }

  htbl_node = bf_hashtbl_get_remove(mat_tbl_info->key_htbl[pipe_id], key_p);
  if (htbl_node == NULL) {
    LOG_ERROR(
        "%s:%d Error : Match spec found in entry handle hash map maintained by "
        "the pipe manager"
        " but not found in match spec based hash table, device id %d"
        " tbl 0x%x",
        __func__,
        __LINE__,
        device_id,
        mat_tbl_info->handle);
    if (key_p) {
      PIPE_MGR_FREE(key_p);
    }
    PIPE_MGR_DBGCHK(0);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (isTxn) {
    /* Log the operation */
    /* Check if an operation was already done on this match spec.
     * If it was done, remove it from the txn log hash table.
     */
    /* If the txn log hash table does not exist, create it */
    if (mat_tbl_info->txn_log_htbl[pipe_id] == NULL) {
      mat_tbl_info->txn_log_htbl[pipe_id] =
          (bf_hashtable_t *)PIPE_MGR_CALLOC(1, sizeof(bf_hashtable_t));
      if (mat_tbl_info->txn_log_htbl[pipe_id] == NULL) {
        LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
        if (key_p) {
          PIPE_MGR_FREE(key_p);
        }
        return PIPE_NO_SYS_RESOURCES;
      }
      if (mat_tbl_info->match_type == EXACT_MATCH) {
        cmp_fn = pipe_mgr_txn_mat_exm_key_cmp_fn;
      } else {
        if (is_atcam) {
          cmp_fn = pipe_mgr_txn_mat_atcam_tern_key_cmp_fn;
        } else {
          cmp_fn = pipe_mgr_txn_mat_tern_key_cmp_fn;
        }
      }
      htbl_sts = bf_hashtbl_init(mat_tbl_info->txn_log_htbl[pipe_id],
                                 cmp_fn,
                                 pipe_mgr_free_mat_key_txn_node,
                                 key_sz,
                                 sizeof(pipe_mgr_mat_txn_log_htbl_node_t),
                                 0x82342341);
      if (htbl_sts != BF_HASHTBL_OK) {
        LOG_ERROR(
            "%s:%d Error in initializing transaction log hash table for"
            " table 0x%x",
            __func__,
            __LINE__,
            mat_tbl_info->handle);
        if (key_p) {
          PIPE_MGR_FREE(key_p);
        }
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
      }
    }
    /* Check if this match spec was already operated upon in the transaction
     */
    txn_log_node =
        bf_hashtbl_get_remove(mat_tbl_info->txn_log_htbl[pipe_id], key_p);
    if (txn_log_node == NULL) {
      txn_log_node = (pipe_mgr_mat_txn_log_htbl_node_t *)PIPE_MGR_CALLOC(
          1, sizeof(pipe_mgr_mat_txn_log_htbl_node_t));
      if (txn_log_node == NULL) {
        LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
        if (key_p) {
          PIPE_MGR_FREE(key_p);
        }
        return PIPE_NO_SYS_RESOURCES;
      }
      txn_log_node->htbl_node =
          PIPE_MGR_CALLOC(1, sizeof(pipe_mgr_mat_key_htbl_node_t));
      if (txn_log_node->htbl_node == NULL) {
        if (key_p) {
          PIPE_MGR_FREE(key_p);
        }
        PIPE_MGR_FREE(txn_log_node);
        LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
        return PIPE_NO_SYS_RESOURCES;
      }
      txn_log_node->htbl_node->mat_ent_hdl = mat_ent_hdl;
      txn_log_node->htbl_node->match_spec =
          pipe_mgr_tbl_copy_match_spec(NULL, match_spec);
      txn_log_node->operation = PIPE_MGR_MAT_OPERATION_DELETE;
      txn_log_node->pipe_idx = pipe_id;
      /* Insert the node into the txn log hash table */
      htbl_sts = bf_hashtbl_insert(
          mat_tbl_info->txn_log_htbl[pipe_id], txn_log_node, key_p);
      if (htbl_sts != BF_HASHTBL_OK) {
        LOG_ERROR(
            "%s:%d Error in inserting txn log node into hash table"
            " for table 0x%x",
            __func__,
            __LINE__,
            mat_tbl_info->handle);
        if (key_p) {
          PIPE_MGR_FREE(key_p);
        }
        return PIPE_UNEXPECTED;
      }
    } else {
      /* An operation was already done on this match spec, check the operation
       * type. It has to be the reverse of what is being done currently, which
       * implies that the operations cancel out each other during the
       * course of the transaction. Otherwise node is already saved.
       */
      if (txn_log_node->operation == PIPE_MGR_MAT_OPERATION_ADD) {
        pipe_mgr_free_mat_key_txn_node(txn_log_node);
      }
    }
  }
  /* If the entry_hdl is not found in any of the entry hdl maps, then the delete
     call might be for a match table which does not have match specs.
     In this case we return a success*/
  if (key_p) {
    PIPE_MGR_FREE(key_p);
  }
  pipe_mgr_free_key_htbl_node(htbl_node);
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_mat_tbl_key_delete(bf_dev_id_t device_id,
                                          pipe_mat_ent_hdl_t mat_ent_hdl,
                                          bool isTxn,
                                          pipe_mat_tbl_info_t *mat_tbl_info,
                                          bf_dev_pipe_t pipe_id,
                                          pipe_tbl_match_spec_t *match_spec) {
  return pipe_mgr_mat_tbl_key_delete_internal(
      device_id, mat_tbl_info, mat_ent_hdl, isTxn, pipe_id, match_spec);
}

void pipe_mgr_mat_tbl_txn_commit(bf_dev_id_t device_id,
                                 pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                 bf_dev_pipe_t *pipes_list,
                                 unsigned nb_pipes) {
  pipe_mat_tbl_info_t *mat_tbl_info = NULL;
  uint32_t num_pipelines = 0;

  mat_tbl_info =
      pipe_mgr_get_tbl_info(device_id, mat_tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    return;
  }

  /* If there is no transaction log hash table, just return */
  if (mat_tbl_info->txn_log_htbl == NULL) {
    return;
  }

  /* Only index 0 has stored txn info if table op was called with
   * BF_DEV_PIPE_ALL as argument. This would cause iteration code below
   * to skip deletion on pipelines that have lowest pipe not 0.
   * Make sure index 0 is handled, itration will skip it. */
  if (mat_tbl_info->symmetric && mat_tbl_info->txn_log_htbl[0]) {
    bf_hashtbl_delete(mat_tbl_info->txn_log_htbl[0]);
    PIPE_MGR_FREE(mat_tbl_info->txn_log_htbl[0]);
    mat_tbl_info->txn_log_htbl[0] = NULL;
  }

  num_pipelines = pipe_mgr_get_num_active_pipes(device_id);
  /* Just Delete the txn log hash table for the related pipes */
  unsigned i, pipe = 0;
  while (pipe < nb_pipes && pipe < num_pipelines) {
    i = pipes_list[pipe++];
    if (mat_tbl_info->txn_log_htbl[i]) {
      bf_hashtbl_delete(mat_tbl_info->txn_log_htbl[i]);
      PIPE_MGR_FREE(mat_tbl_info->txn_log_htbl[i]);
      mat_tbl_info->txn_log_htbl[i] = NULL;
    }
  }
  return;
}

static void pipe_mgr_mat_txn_abort_fn(void *arg, void *obj) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_mat_txn_log_htbl_node_t *txn_log_node = NULL;
  pipe_mgr_mat_tbl_foreach_arg_t *foreach_arg =
      (pipe_mgr_mat_tbl_foreach_arg_t *)arg;
  pipe_mat_tbl_info_t *mat_tbl_info = NULL;
  pipe_tbl_match_spec_t *match_spec = NULL;
  bf_dev_id_t device_id = foreach_arg->device_id;
  txn_log_node = bf_hashtbl_get_cmp_data(obj);

  mat_tbl_info = foreach_arg->mat_tbl_info;
  if (mat_tbl_info == NULL) {
    return;
  }
  match_spec = txn_log_node->htbl_node->match_spec;
  if (txn_log_node->operation == PIPE_MGR_MAT_OPERATION_ADD) {
    /* Undo the add which involves the following
     *    1. Remove the match-spec based entry in the hash table.
     */
    status = pipe_mgr_mat_tbl_key_delete_internal(
        device_id,
        mat_tbl_info,
        txn_log_node->htbl_node->mat_ent_hdl,
        false,
        txn_log_node->pipe_idx,
        match_spec);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in deleting entry from match spec based hash tbl "
          " for table 0x%x, device id %d entry handle 0x%x err %s",
          __func__,
          __LINE__,
          foreach_arg->mat_tbl_hdl,
          foreach_arg->device_id,
          txn_log_node->htbl_node->mat_ent_hdl,
          pipe_str_err(status));
      return;
    }
  } else if (txn_log_node->operation == PIPE_MGR_MAT_OPERATION_DELETE) {
    /* Undo the delete which involves the following
     *    1. Add the match-spec based entry in the hash table.
     */

    /* Even though the below function takes in the pipe-id, whats stored
     * in the transaction node is the pipe-"index", which is the index into
     * the array of hashtables which are maintained one per pipe, and this
     * is what is passed into this function. This function checks for
     * PIPE_ID_ALL
     * and if it is, index 0 will be used if not the pipe_idx is what is passed
     * as the pipe-id. If it was a symmetric table, pipe_idx stored will be
     * ZERO,
     * hence we will access the right index.
     */
    status = pipe_mgr_mat_tbl_key_insert_internal(
        mat_tbl_info,
        match_spec,
        txn_log_node->htbl_node->mat_ent_hdl,
        txn_log_node->pipe_idx,
        false);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in inserting match spec for tbl 0x%x, device id %d, "
          "error %s",
          __func__,
          __LINE__,
          foreach_arg->device_id,
          foreach_arg->mat_tbl_hdl,
          pipe_str_err(status));
      return;
    }
  }
  return;
}

void pipe_mgr_mat_tbl_txn_abort(bf_dev_id_t device_id,
                                pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                bf_dev_pipe_t *pipes_list,
                                unsigned nb_pipes) {
  pipe_mat_tbl_info_t *mat_tbl_info = NULL;
  pipe_mgr_mat_tbl_foreach_arg_t foreach_arg;
  uint32_t num_pipelines = 0;

  mat_tbl_info =
      pipe_mgr_get_tbl_info(device_id, mat_tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    return;
  }

  if (mat_tbl_info->txn_log_htbl == NULL) {
    return;
  }
  foreach_arg.device_id = device_id;
  foreach_arg.mat_tbl_hdl = mat_tbl_hdl;
  foreach_arg.mat_tbl_info = mat_tbl_info;

  /* Only index 0 has stored txn info if table op was called with
   * BF_DEV_PIPE_ALL as argument. This would cause iteration code below
   * to skip rollback on pipelines that have lowest pipe not 0.
   * Make sure index 0 is handled, itration will skip it. */
  if (mat_tbl_info->symmetric && mat_tbl_info->txn_log_htbl[0]) {
    bf_hashtbl_foreach_fn(
        mat_tbl_info->txn_log_htbl[0], pipe_mgr_mat_txn_abort_fn, &foreach_arg);
    /* Delete the txn log hash table */
    bf_hashtbl_delete(mat_tbl_info->txn_log_htbl[0]);
    PIPE_MGR_FREE(mat_tbl_info->txn_log_htbl[0]);
    mat_tbl_info->txn_log_htbl[0] = NULL;
  }

  /* Just Delete the txn log hash table for the related pipes */
  num_pipelines = pipe_mgr_get_num_active_pipes(device_id);
  unsigned i, pipe = 0;
  while (pipe < nb_pipes && pipe < num_pipelines) {
    i = pipes_list[pipe++];
    if (mat_tbl_info->txn_log_htbl[i]) {
      bf_hashtbl_foreach_fn(mat_tbl_info->txn_log_htbl[i],
                            pipe_mgr_mat_txn_abort_fn,
                            &foreach_arg);
      /* Delete the txn log hash table */
      bf_hashtbl_delete(mat_tbl_info->txn_log_htbl[i]);
      PIPE_MGR_FREE(mat_tbl_info->txn_log_htbl[i]);
      mat_tbl_info->txn_log_htbl[i] = NULL;
    }
  }
}

void pipe_mgr_match_spec_htbl_cleanup(bf_dev_id_t device_id,
                                      pipe_mat_tbl_info_t *mat_tbl_info) {
  uint32_t num_pipelines = 0;
  if (mat_tbl_info == NULL) {
    return;
  }
  num_pipelines = pipe_mgr_get_num_active_pipes(device_id);
  unsigned i = 0;
  if (mat_tbl_info->key_htbl == NULL) {
    PIPE_MGR_DBGCHK(mat_tbl_info->txn_log_htbl == NULL);
    return;
  }
  for (i = 0; i < num_pipelines; i++) {
    if (mat_tbl_info->key_htbl[i]) {
      bf_hashtbl_delete(mat_tbl_info->key_htbl[i]);
      PIPE_MGR_FREE(mat_tbl_info->key_htbl[i]);
      mat_tbl_info->key_htbl[i] = NULL;
    }
    if (mat_tbl_info->txn_log_htbl[i]) {
      bf_hashtbl_delete(mat_tbl_info->txn_log_htbl[i]);
      PIPE_MGR_FREE(mat_tbl_info->txn_log_htbl[i]);
      mat_tbl_info->txn_log_htbl[i] = NULL;
    }
  }

  if (mat_tbl_info->key_htbl) {
    PIPE_MGR_FREE(mat_tbl_info->key_htbl);
    mat_tbl_info->key_htbl = NULL;
  }

  if (mat_tbl_info->txn_log_htbl) {
    PIPE_MGR_FREE(mat_tbl_info->txn_log_htbl);
    mat_tbl_info->txn_log_htbl = NULL;
  }

  return;
}

pipe_status_t pipe_mgr_restore_mat_tbl_key_state(
    bf_dev_id_t device_id,
    pipe_tbl_hdl_t tbl_hdl,
    pipe_mgr_move_list_t *move_list) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mat_tbl_info_t *mat_tbl_info =
      pipe_mgr_get_tbl_info(device_id, tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    LOG_ERROR("%s:%d Mat tbl info for tbl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              tbl_hdl,
              device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  /* If the duplicate entry detection on this table is turned off, just return
   * right away
   */
  if (!mat_tbl_info->duplicate_entry_check) {
    return PIPE_SUCCESS;
  }
  /* Iterate through each node in the move-list, and we expect only to find
   * entry add operations.
   */
  pipe_mgr_move_list_t *traverser = NULL;
  for (traverser = move_list; traverser; traverser = traverser->next) {
    if (traverser->op != PIPE_MAT_UPDATE_ADD &&
        traverser->op != PIPE_MAT_UPDATE_SET_DFLT) {
      LOG_ERROR(
          "%s:%d Invalid operation in the move-list during restore of match "
          "table key state for tbl 0x%x, device id %d",
          __func__,
          __LINE__,
          tbl_hdl,
          device_id);
      PIPE_MGR_DBGCHK(0);
      return PIPE_NOT_SUPPORTED;
    }
    if (traverser->op == PIPE_MAT_UPDATE_SET_DFLT) {
      /* Nothing to do for default entry add */
      continue;
    }
    pipe_mat_ent_hdl_t entry_hdl = traverser->entry_hdl;
    pipe_tbl_match_spec_t *match_spec = unpack_mat_ent_data_ms(traverser->data);
    bf_dev_pipe_t pipe_id = get_move_list_pipe(traverser);
    if (match_spec->num_match_bytes) {
      status = pipe_mgr_mat_tbl_key_insert(
          device_id, mat_tbl_info, match_spec, entry_hdl, pipe_id, false);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in match table key insertion for entry hdl %d, tbl "
            "0x%x, device id %d, err %s",
            __func__,
            __LINE__,
            entry_hdl,
            tbl_hdl,
            device_id,
            pipe_str_err(status));
        PIPE_MGR_DBGCHK(0);
        return status;
      }
    } else {
      PIPE_MGR_DBGCHK(0);
    }
  }
  return PIPE_SUCCESS;
}

/*
 * Usage: pipe_mgr_tbl_copy_match_spec(new_match_spec, orig_match_spec)
 * --------------------------------------------------------------------
 * Copies the match_spec data from src to dst.
 * Returns NULL upon calloc failure.
 */
pipe_tbl_match_spec_t *pipe_mgr_tbl_copy_match_spec(
    pipe_tbl_match_spec_t *dst, pipe_tbl_match_spec_t *src) {
  uint8_t *dst_match_value_bits = NULL;
  uint8_t *dst_match_mask_bits = NULL;
  uint8_t need_free_dst = 0;

  if (!src) {
    return NULL;
  }

  if (!dst) {
    dst = (pipe_tbl_match_spec_t *)PIPE_MGR_CALLOC(
        sizeof(pipe_tbl_match_spec_t), 1);
    if (!dst) {
      LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
      return NULL;
    }
    need_free_dst = 1;  // dst is allocated within this function
  } else {
    // Save for later restoration.
    dst_match_value_bits = dst->match_value_bits;
    dst_match_mask_bits = dst->match_mask_bits;
  }

  PIPE_MGR_MEMCPY(dst, src, sizeof(pipe_tbl_match_spec_t));

  /* In case dst had already allocated match/mask bits, restore the pointers.
     Otherwise alloc new mempory accordingly. */
  if (!dst_match_value_bits) {
    dst->match_value_bits =
        (uint8_t *)PIPE_MGR_CALLOC(sizeof(uint8_t), dst->num_match_bytes);
    if (!dst->match_value_bits) {
      LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
      if (need_free_dst) {
        PIPE_MGR_FREE(dst);
      }
      return NULL;
    }
    dst->match_mask_bits =
        (uint8_t *)PIPE_MGR_CALLOC(sizeof(uint8_t), dst->num_match_bytes);
    if (!dst->match_mask_bits) {
      LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
      PIPE_MGR_FREE(dst->match_value_bits);  // dst->match_value_bits will not
                                             // NULL if reach here
      if (need_free_dst) {
        PIPE_MGR_FREE(dst);
      }
      return NULL;
    }
  } else {
    dst->match_value_bits = dst_match_value_bits;
    dst->match_mask_bits = dst_match_mask_bits;
  }

  /* For default entries match value/mask bits can be absent. Return and keep
     whatever was set there by the caller. */
  if (!src->match_value_bits) return dst;

  PIPE_MGR_MEMCPY(
      dst->match_value_bits, src->match_value_bits, dst->num_match_bytes);
  PIPE_MGR_MEMCPY(
      dst->match_mask_bits, src->match_mask_bits, dst->num_match_bytes);

  return dst;
}

void pipe_mgr_tbl_destroy_match_spec(pipe_tbl_match_spec_t **match_spec_p) {
  if (!match_spec_p || !*match_spec_p) {
    return;
  }

  pipe_tbl_match_spec_t *match_spec = *match_spec_p;

  if (match_spec->match_value_bits) {
    PIPE_MGR_FREE(match_spec->match_value_bits);
  }
  if (match_spec->match_mask_bits) {
    PIPE_MGR_FREE(match_spec->match_mask_bits);
  }
  PIPE_MGR_FREE(match_spec);

  *match_spec_p = NULL;
}

pipe_tbl_match_spec_t *pipe_mgr_tbl_alloc_match_spec(size_t num_match_bytes) {
  pipe_tbl_match_spec_t *ms;

  ms = (pipe_tbl_match_spec_t *)PIPE_MGR_CALLOC(1,
                                                sizeof(pipe_tbl_match_spec_t));
  if (!ms) {
    LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
    return NULL;
  }
  ms->match_value_bits =
      (uint8_t *)PIPE_MGR_CALLOC(num_match_bytes, sizeof(uint8_t));
  if (!ms->match_value_bits) {
    LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
    pipe_mgr_tbl_destroy_match_spec(&ms);
    return NULL;
  }
  ms->match_mask_bits =
      (uint8_t *)PIPE_MGR_CALLOC(num_match_bytes, sizeof(uint8_t));
  if (!ms->match_mask_bits) {
    LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
    pipe_mgr_tbl_destroy_match_spec(&ms);
    return NULL;
  }
  ms->num_match_bytes = num_match_bytes;
  return ms;
}

int pipe_mgr_tbl_compare_match_specs(pipe_tbl_match_spec_t *m1,
                                     pipe_tbl_match_spec_t *m2) {
  int d;

  /* Assume no overflow */
  d = m1->num_match_bytes - m2->num_match_bytes;
  if (d) {
    return d;
  }

  d |= memcmp(m1->match_mask_bits, m2->match_mask_bits, m1->num_match_bytes);
  d |= memcmp(m1->match_value_bits, m2->match_value_bits, m1->num_match_bytes);
  // memcmp is not reliable here to compare pipe_tbl_match_spec_t directly
  // because due to struct packing, the padded bytes
  // may not be set by the callers (specifically pd) so need to compare one by
  // one
  d |= m1->partition_index - m2->partition_index;
  d |= m1->num_valid_match_bits - m2->num_valid_match_bits;
  d |= m1->num_match_bytes - m2->num_match_bytes;
  d |= m1->priority - m2->priority;

  return d;
}

pipe_status_t pipe_mgr_create_match_spec(
    uint8_t *key,
    uint8_t *msk,
    int len_bytes,
    int len_bits,
    uint32_t priority,
    pipe_tbl_match_spec_t *pipe_match_spec) {
  if (!key || !msk || !len_bytes) {
    return PIPE_INVALID_ARG;
  }

  uint8_t *pipe_match_value_bits = PIPE_MGR_CALLOC(len_bytes, sizeof(uint8_t));
  uint8_t *pipe_match_mask_bits = PIPE_MGR_CALLOC(len_bytes, sizeof(uint8_t));
  if (!pipe_match_value_bits || !pipe_match_mask_bits) {
    return PIPE_NO_SYS_RESOURCES;
  }

  PIPE_MGR_MEMSET(pipe_match_spec, 0, sizeof(pipe_tbl_match_spec_t));

  PIPE_MGR_MEMCPY(pipe_match_value_bits, key, len_bytes);
  PIPE_MGR_MEMCPY(pipe_match_mask_bits, msk, len_bytes);

  pipe_match_spec->priority = priority;
  pipe_match_spec->match_value_bits = pipe_match_value_bits;
  pipe_match_spec->match_mask_bits = pipe_match_mask_bits;
  pipe_match_spec->num_match_bytes = len_bytes;
  pipe_match_spec->num_valid_match_bits = len_bits;

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_create_action_data_spec(
    pipe_mgr_field_info_t *field_info,
    uint32_t num_fields,
    pipe_action_data_spec_t *act_data_spec) {
  uint32_t cntr = 0;
  uint32_t byte_width = 0;
  uint32_t num_action_bits = 0, num_action_bytes = 0;
  uint8_t *pipe_action_data_bits = NULL;

  if (!field_info) {
    return PIPE_INVALID_ARG;
  }

  /* Calculate num of bytes in action spec - used to allocate memory */
  num_action_bits = 0;
  num_action_bytes = 0;
  for (cntr = 0; cntr < num_fields; cntr++) {
    num_action_bits += field_info[cntr].bit_width;
    num_action_bytes += (field_info[cntr].bit_width + 7) / 8;
  }

  if (num_action_bytes) {
    pipe_action_data_bits = PIPE_MGR_CALLOC(num_action_bytes, sizeof(uint8_t));
    if (!pipe_action_data_bits) {
      return PIPE_NO_SYS_RESOURCES;
    }

    /* Copy the static data values */
    uint8_t *action_data_bits = pipe_action_data_bits;
    for (cntr = 0; cntr < num_fields; cntr++) {
      byte_width = (field_info[cntr].bit_width + 7) / 8;
      PIPE_MGR_MEMCPY(action_data_bits, field_info[cntr].value, byte_width);
      action_data_bits += byte_width;
    }
  }

  act_data_spec->action_data_bits = pipe_action_data_bits;
  act_data_spec->num_valid_action_data_bits = num_action_bits;
  act_data_spec->num_action_data_bytes = num_action_bytes;

  return PIPE_SUCCESS;
}

/* Create action spec from ctx json info */
pipe_status_t pipe_mgr_create_action_spec(bf_dev_id_t dev_id,
                                          pipe_mgr_action_entry_t *ae,
                                          pipe_action_spec_t *act_spec) {
  pipe_status_t rc = PIPE_SUCCESS;
  if (!ae) {
    PIPE_MGR_DBGCHK(ae);
    return PIPE_INVALID_ARG;
  }
  if (!act_spec) {
    PIPE_MGR_DBGCHK(act_spec);
    return PIPE_INVALID_ARG;
  }
  PIPE_MGR_MEMSET(act_spec, 0, sizeof(pipe_action_spec_t));

  /* Assume this is an action-data case (not action-profile/action-selector). */
  act_spec->pipe_action_datatype_bmap = PIPE_ACTION_DATA_TYPE;
  if (ae->num_act_data) {
    rc = pipe_mgr_create_action_data_spec(
        ae->act_data, ae->num_act_data, &act_spec->act_data);
    if (PIPE_SUCCESS != rc) {
      return rc;
    }
  }

  /* Populate the indirect resources in the resource list of the action spec. */
  for (uint8_t i = 0; i < ae->num_ind_res; ++i) {
    act_spec->resources[act_spec->resource_count].tag =
        PIPE_RES_ACTION_TAG_ATTACHED;
    act_spec->resources[act_spec->resource_count].tbl_hdl =
        ae->ind_res[i].handle;
    act_spec->resources[act_spec->resource_count].tbl_idx = ae->ind_res[i].idx;
    ++act_spec->resource_count;
    if (act_spec->resource_count > PIPE_NUM_TBL_RESOURCES) {
      LOG_ERROR("%s:%d Too many (%d) resources specified for static entry",
                __func__,
                __LINE__,
                act_spec->resource_count);
      return PIPE_INVALID_ARG;
    }
  }

  /* Populate the direct resources in the resource list of the action spec. */
  for (uint8_t i = 0; i < ae->num_dir_res; ++i) {
    act_spec->resources[act_spec->resource_count].tag =
        PIPE_RES_ACTION_TAG_ATTACHED;
    act_spec->resources[act_spec->resource_count].tbl_hdl =
        ae->dir_res[i].handle;
    /* Fill in a default spec for the resource. */
    switch (PIPE_GET_HDL_TYPE(ae->dir_res[i].handle)) {
      case PIPE_HDL_TYPE_STAT_TBL:
        /* Use a starting counter of zero for stats. */
        act_spec->resources[act_spec->resource_count].data.counter.bytes = 0;
        act_spec->resources[act_spec->resource_count].data.counter.packets = 0;
        break;
      case PIPE_HDL_TYPE_METER_TBL: {
        /* Use a max-rate spec for meters to color everything green. */
        pipe_meter_tbl_info_t *m = pipe_mgr_get_meter_tbl_info(
            dev_id, ae->dir_res[i].handle, __func__, __LINE__);
        if (!m) return PIPE_INVALID_ARG;
        if (m->meter_type == PIPE_METER_TYPE_STANDARD) {
          act_spec->resources[act_spec->resource_count].data.meter.meter_type =
              m->enable_color_aware ? METER_TYPE_COLOR_AWARE
                                    : METER_TYPE_COLOR_UNAWARE;
          act_spec->resources[act_spec->resource_count].data.meter.cburst =
              m->max_burst_size;
          act_spec->resources[act_spec->resource_count].data.meter.pburst =
              m->max_burst_size;
          if (m->meter_granularity == PIPE_METER_GRANULARITY_BYTES) {
            act_spec->resources[act_spec->resource_count]
                .data.meter.cir.value.kbps = m->max_rate;
            act_spec->resources[act_spec->resource_count]
                .data.meter.pir.value.kbps = m->max_rate;
          } else {
            act_spec->resources[act_spec->resource_count]
                .data.meter.cir.value.pps = m->max_rate;
            act_spec->resources[act_spec->resource_count]
                .data.meter.pir.value.pps = m->max_rate;
          }
        }
        break;
      }
      case PIPE_HDL_TYPE_STFUL_TBL: {
        /* Use the register's initial value for the spec. */
        pipe_stful_tbl_info_t *s = pipe_mgr_get_stful_tbl_info(
            dev_id, ae->dir_res[i].handle, __func__, __LINE__);
        if (!s) return PIPE_INVALID_ARG;
        switch (s->width) {
          case 1:
            act_spec->resources[act_spec->resource_count].data.stful.bit =
                s->initial_val_lo;
            break;
          case 8:
            if (s->dbl_width) {
              act_spec->resources[act_spec->resource_count]
                  .data.stful.dbl_byte.lo = s->initial_val_lo;
              act_spec->resources[act_spec->resource_count]
                  .data.stful.dbl_byte.hi = s->initial_val_hi;
            } else {
              act_spec->resources[act_spec->resource_count].data.stful.byte =
                  s->initial_val_lo;
            }
            break;
          case 16:
            if (s->dbl_width) {
              act_spec->resources[act_spec->resource_count]
                  .data.stful.dbl_half.lo = s->initial_val_lo;
              act_spec->resources[act_spec->resource_count]
                  .data.stful.dbl_half.hi = s->initial_val_hi;
            } else {
              act_spec->resources[act_spec->resource_count].data.stful.half =
                  s->initial_val_lo;
            }
            break;
          case 32:
            if (s->dbl_width) {
              act_spec->resources[act_spec->resource_count]
                  .data.stful.dbl_word.lo = s->initial_val_lo;
              act_spec->resources[act_spec->resource_count]
                  .data.stful.dbl_word.hi = s->initial_val_hi;
            } else {
              act_spec->resources[act_spec->resource_count].data.stful.word =
                  s->initial_val_lo;
            }
            break;
          case 64:
            if (s->dbl_width) {
              act_spec->resources[act_spec->resource_count]
                  .data.stful.dbl_dbl.lo = s->initial_val_lo;
              act_spec->resources[act_spec->resource_count]
                  .data.stful.dbl_dbl.hi = s->initial_val_hi;
            } else {
              act_spec->resources[act_spec->resource_count].data.stful.dbl =
                  s->initial_val_hi;
              act_spec->resources[act_spec->resource_count].data.stful.dbl =
                  (act_spec->resources[act_spec->resource_count].data.stful.dbl
                   << 32) |
                  s->initial_val_lo;
            }
            break;
          default:
            LOG_ERROR(
                "%s:%d Unsupported register width %d double %d on table 0x%x",
                __func__,
                __LINE__,
                s->width,
                s->dbl_width,
                ae->dir_res[i].handle);
            return PIPE_INVALID_ARG;
        }
        break;
      }
      default:
        LOG_ERROR("%s:%d Unsupported resource table type, handle 0x%x",
                  __func__,
                  __LINE__,
                  ae->dir_res[i].handle);
        return PIPE_INVALID_ARG;
    }
    ++act_spec->resource_count;
    if (act_spec->resource_count > PIPE_NUM_TBL_RESOURCES) {
      LOG_ERROR("%s:%d too many (%d) resources specified for static entry",
                __func__,
                __LINE__,
                act_spec->resource_count);
      return PIPE_INVALID_ARG;
    }
  }

  return rc;
}

/*
 * Usage: pipe_mgr_tbl_copy_action_spec(new_action_spec, orig_action_spec)
 * -----------------------------------------------------------------------
 * Copies the action spec data from src to dst.
 * Returns NULL upon calloc failure.
 */
pipe_action_spec_t *pipe_mgr_tbl_copy_action_spec(pipe_action_spec_t *dst,
                                                  pipe_action_spec_t *src) {
  uint8_t *dst_action_data_bits = NULL;

  if (!src) {
    return NULL;
  }

  if (!dst) {
    dst = (pipe_action_spec_t *)PIPE_MGR_CALLOC(1, sizeof(pipe_action_spec_t));
    if (!dst) {
      LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
      return NULL;
    }
  } else {
    dst_action_data_bits = dst->act_data.action_data_bits;
  }

  PIPE_MGR_MEMCPY(dst, src, sizeof(pipe_action_spec_t));

  if (!dst_action_data_bits && dst->act_data.num_action_data_bytes) {
    dst->act_data.action_data_bits = (uint8_t *)PIPE_MGR_CALLOC(
        dst->act_data.num_action_data_bytes, sizeof(uint8_t));
    if (!dst->act_data.action_data_bits) {
      LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
      return NULL;
    }
  } else {
    dst->act_data.action_data_bits = dst_action_data_bits;
  }

  if (src->act_data.num_action_data_bytes) {
    if (!src->act_data.action_data_bits) {
      PIPE_MGR_DBGCHK(src->act_data.action_data_bits);
    } else {
      PIPE_MGR_MEMCPY(dst->act_data.action_data_bits,
                      src->act_data.action_data_bits,
                      dst->act_data.num_action_data_bytes);
    }
  }

  return dst;
}

void pipe_mgr_tbl_destroy_action_spec(pipe_action_spec_t **action_spec_p) {
  if (!action_spec_p || !*action_spec_p) {
    return;
  }
  pipe_action_spec_t *action_spec = *action_spec_p;

  if (action_spec->act_data.action_data_bits) {
    PIPE_MGR_FREE(action_spec->act_data.action_data_bits);
  }
  PIPE_MGR_FREE(action_spec);
  *action_spec_p = NULL;
}

pipe_action_spec_t *pipe_mgr_tbl_alloc_action_spec(
    size_t num_action_data_bytes) {
  pipe_action_spec_t *as;
  as = (pipe_action_spec_t *)PIPE_MGR_CALLOC(1, sizeof(pipe_action_spec_t));
  if (!as) {
    LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
    return NULL;
  }
  as->act_data.action_data_bits =
      (uint8_t *)PIPE_MGR_CALLOC(num_action_data_bytes, sizeof(uint8_t));
  if (!as->act_data.action_data_bits) {
    LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
    pipe_mgr_tbl_destroy_action_spec(&as);
    return NULL;
  }
  as->act_data.num_action_data_bytes = num_action_data_bytes;
  return as;
}

int pipe_mgr_tbl_compare_action_data_specs(pipe_action_data_spec_t *ad1,
                                           pipe_action_data_spec_t *ad2) {
  int d;

  d = ad1->num_action_data_bytes - ad2->num_action_data_bytes;
  if (d) {
    return d;
  }

  d = ad1->num_valid_action_data_bits - ad2->num_valid_action_data_bits;
  if (d) {
    return d;
  }

  if (ad1->action_data_bits && ad2->action_data_bits) {
    d = memcmp(ad1->action_data_bits,
               ad2->action_data_bits,
               ad1->num_action_data_bytes);
  }
  return d;
}

int pipe_mgr_tbl_compare_action_specs(pipe_action_spec_t *a1,
                                      pipe_action_spec_t *a2) {
  int d;

  d = a1->pipe_action_datatype_bmap - a2->pipe_action_datatype_bmap;
  if (d) {
    return d;
  }

  if (IS_ACTION_SPEC_ACT_DATA(a1)) {
    d = pipe_mgr_tbl_compare_action_data_specs(&a1->act_data, &a2->act_data);
  } else if (IS_ACTION_SPEC_ACT_DATA_HDL(a1)) {
    d = a1->adt_ent_hdl - a2->adt_ent_hdl;
  } else if (IS_ACTION_SPEC_SEL_GRP(a1)) {
    d = a1->sel_grp_hdl - a2->sel_grp_hdl;
  }

  if (d) {
    return d;
  }

  d = a1->resource_count - a2->resource_count;
  if (d) {
    return d;
  }

  // VK CAUTION while comparing resources. PD does not initialize everything
  // PD also sets DETACHED/ATTACHED Tag which the entry decoder may not be doing
  if (a1->resource_count) {
    d = memcmp(a1->resources,
               a2->resources,
               sizeof(pipe_res_spec_t) * a1->resource_count);
  }
  return d;
}

void pipe_mgr_tbl_log_specs(bf_dev_id_t dev_id,
                            profile_id_t prof_id,
                            pipe_mat_tbl_hdl_t tbl_hdl,
                            struct pipe_mgr_mat_data *data,
                            cJSON *mat_ent,
                            bool is_default) {
  pipe_tbl_match_spec_t *ms;
  pipe_action_spec_t *as;
  pipe_act_fn_hdl_t act_fn_hdl;
  int res_idx;
  cJSON *ms_node, *as_node, *resources, *resource;

  if (!is_default) {
    ms = unpack_mat_ent_data_ms(data);
    if (ms) {
      cJSON_AddItemToObject(
          mat_ent, "match_spec", ms_node = cJSON_CreateObject());
      cJSON_AddNumberToObject(ms_node, "ptn_idx", ms->partition_index);
      cJSON_AddNumberToObject(ms_node, "priority", ms->priority);
      pipe_mgr_entry_format_jsonify_match_spec(
          dev_id, prof_id, tbl_hdl, ms, ms_node);
    }
  }

  as = unpack_mat_ent_data_as(data);
  act_fn_hdl = unpack_mat_ent_data_afun_hdl(data);
  if (as) {
    cJSON_AddItemToObject(mat_ent, "act_spec", as_node = cJSON_CreateObject());
    cJSON_AddNumberToObject(as_node, "act_fn_hdl", act_fn_hdl);
    cJSON_AddNumberToObject(as_node, "datatype", as->pipe_action_datatype_bmap);
    switch (as->pipe_action_datatype_bmap) {
      case PIPE_ACTION_DATA_TYPE:
        pipe_mgr_entry_format_jsonify_action_spec(
            dev_id, prof_id, &(as->act_data), act_fn_hdl, as_node);
        break;
      case PIPE_ACTION_DATA_HDL_TYPE:
        cJSON_AddNumberToObject(as_node, "act_ent_hdl", as->adt_ent_hdl);
        break;
      case PIPE_SEL_GRP_HDL_TYPE:
        cJSON_AddNumberToObject(as_node, "sel_hdl", as->sel_grp_hdl);
        break;
    }

    cJSON_AddItemToObject(
        mat_ent, "resources", resources = cJSON_CreateArray());
    for (res_idx = 0; res_idx < as->resource_count; res_idx++) {
      cJSON_AddItemToArray(resources, resource = cJSON_CreateObject());
      cJSON_AddNumberToObject(
          resource, "tbl_hdl", as->resources[res_idx].tbl_hdl);
      cJSON_AddNumberToObject(
          resource, "tbl_idx", as->resources[res_idx].tbl_idx);
      cJSON_AddBoolToObject(
          resource,
          "attached",
          (as->resources[res_idx].tag == PIPE_RES_ACTION_TAG_ATTACHED));
    }
  }
}

void pipe_mgr_tbl_restore_specs(bf_dev_id_t dev_id,
                                profile_id_t prof_id,
                                pipe_mat_tbl_hdl_t tbl_hdl,
                                cJSON *mat_ent,
                                pipe_tbl_match_spec_t *ms,
                                pipe_action_spec_t *as,
                                pipe_act_fn_hdl_t *act_fn_hdl) {
  int res_idx;
  cJSON *ms_node, *as_node, *resources, *resource;

  if (!ms || !as) {
    LOG_ERROR("%s:%d Null pointer argument passed", __func__, __LINE__);
    return;
  }

  PIPE_MGR_MEMSET(ms, 0, sizeof(pipe_tbl_match_spec_t));
  PIPE_MGR_MEMSET(as, 0, sizeof(pipe_action_spec_t));
  *act_fn_hdl = 0;

  ms_node = cJSON_GetObjectItem(mat_ent, "match_spec");
  if (ms_node) {
    ms->partition_index = cJSON_GetObjectItem(ms_node, "ptn_idx")->valuedouble;
    ms->priority = cJSON_GetObjectItem(ms_node, "priority")->valuedouble;
    pipe_mgr_entry_format_unjsonify_match_spec(
        dev_id, prof_id, tbl_hdl, ms, ms_node);
  }

  as_node = cJSON_GetObjectItem(mat_ent, "act_spec");
  if (as_node) {
    *act_fn_hdl = cJSON_GetObjectItem(as_node, "act_fn_hdl")->valuedouble;
    as->pipe_action_datatype_bmap =
        cJSON_GetObjectItem(as_node, "datatype")->valueint;
    switch (as->pipe_action_datatype_bmap) {
      case PIPE_ACTION_DATA_TYPE:
        pipe_mgr_entry_format_unjsonify_action_spec(
            dev_id, prof_id, &as->act_data, *act_fn_hdl, as_node);
        break;
      case PIPE_ACTION_DATA_HDL_TYPE:
        as->adt_ent_hdl =
            cJSON_GetObjectItem(as_node, "act_ent_hdl")->valuedouble;
        break;
      case PIPE_SEL_GRP_HDL_TYPE:
        as->sel_grp_hdl = cJSON_GetObjectItem(as_node, "sel_hdl")->valuedouble;
        break;
    }

    resources = cJSON_GetObjectItem(mat_ent, "resources");
    for (resource = resources->child, res_idx = 0; resource;
         resource = resource->next, res_idx++) {
      as->resources[res_idx].tbl_hdl =
          cJSON_GetObjectItem(resource, "tbl_hdl")->valuedouble;
      as->resources[res_idx].tbl_idx =
          cJSON_GetObjectItem(resource, "tbl_idx")->valuedouble;
      as->resources[res_idx].tag =
          cJSON_GetObjectItem(resource, "attached")->type == cJSON_True
              ? PIPE_RES_ACTION_TAG_ATTACHED
              : PIPE_RES_ACTION_TAG_DETACHED;
    }
    as->resource_count = res_idx;
  }
}

pipe_status_t pipe_mgr_cleanup_default_entry(pipe_sess_hdl_t sess_hdl,
                                             bf_dev_target_t dev_tgt,
                                             pipe_mat_tbl_info_t *mat_tbl_info,
                                             pipe_mgr_move_list_t **move_list) {
  pipe_status_t ret = PIPE_SUCCESS;
  pipe_mat_tbl_hdl_t mat_tbl_hdl = mat_tbl_info->handle;
  enum pipe_mgr_table_owner_t owner;
  owner = pipe_mgr_sm_tbl_owner(dev_tgt.device_id, mat_tbl_hdl);

  /* Prepare flags for the table managers. */
  uint32_t flags = pipe_mgr_sess_in_txn(sess_hdl) ? PIPE_MGR_TBL_API_TXN : 0;
  flags |= pipe_mgr_sess_in_atomic_txn(sess_hdl) ? PIPE_MGR_TBL_API_ATOM : 0;

  /* Call the reset function for the appropriate table manager. */
  if (PIPE_MGR_TBL_OWNER_EXM == owner) {
    ret = pipe_mgr_exm_cleanup_default_entry(
        dev_tgt, mat_tbl_hdl, flags, move_list);
  } else if (PIPE_MGR_TBL_OWNER_TRN == owner) {
    ret = pipe_mgr_tcam_cleanup_default_entry(
        dev_tgt, mat_tbl_hdl, flags, move_list);
  } else if (PIPE_MGR_TBL_OWNER_ALPM == owner) {
    ret = pipe_mgr_alpm_cleanup_default_entry(
        dev_tgt, mat_tbl_hdl, flags, move_list);
  } else if (PIPE_MGR_TBL_OWNER_PHASE0 == owner) {
    ret = pipe_mgr_phase0_cleanup_default_entry(
        dev_tgt, mat_tbl_hdl, flags, move_list);
  } else if (PIPE_MGR_TBL_OWNER_NO_KEY == owner) {
    mat_tbl_info->tbl_no_key_is_default_entry_valid = false;
    ret = PIPE_SUCCESS;
  } else {
    LOG_ERROR("No handler for table owner %u, session %u, table 0x%x at %s:%d",
              owner,
              sess_hdl,
              mat_tbl_hdl,
              __func__,
              __LINE__);
    PIPE_MGR_DBGCHK(0);
    ret = PIPE_UNEXPECTED;
  }
  if (PIPE_SUCCESS != ret) {
    LOG_ERROR("%s: Dev %d tbl %s 0x%x pipe %x error %s resetting default entry",
              __func__,
              dev_tgt.device_id,
              mat_tbl_info->name,
              mat_tbl_info->handle,
              dev_tgt.dev_pipe_id,
              pipe_str_err(ret));
  }
  return ret;
}

pipe_status_t pipe_mgr_place_default_entry(pipe_sess_hdl_t sess_hdl,
                                           bf_dev_target_t dev_tgt,
                                           pipe_mat_tbl_info_t *mat_tbl_info,
                                           pipe_act_fn_hdl_t act_fn_hdl,
                                           pipe_action_spec_t *act_spec,
                                           pipe_mat_ent_hdl_t *ent_hdl_p,
                                           pipe_mgr_move_list_t **move_list) {
  pipe_status_t ret = PIPE_SUCCESS;
  pipe_mat_tbl_hdl_t mat_tbl_hdl = mat_tbl_info->handle;
  /* Determine table owner. */
  enum pipe_mgr_table_owner_t owner;
  owner = pipe_mgr_sm_tbl_owner(dev_tgt.device_id, mat_tbl_hdl);

  /* Prepare flags for the table managers. */
  uint32_t flags = pipe_mgr_sess_in_txn(sess_hdl) ? PIPE_MGR_TBL_API_TXN : 0;
  flags |= pipe_mgr_sess_in_atomic_txn(sess_hdl) ? PIPE_MGR_TBL_API_ATOM : 0;

  /* Call the add function for the appropriate table manager. */
  if (PIPE_MGR_TBL_OWNER_EXM == owner) {
    ret = pipe_mgr_exm_default_ent_place(dev_tgt,
                                         mat_tbl_hdl,
                                         act_fn_hdl,
                                         act_spec,
                                         flags,
                                         ent_hdl_p,
                                         move_list);
  } else if (PIPE_MGR_TBL_OWNER_TRN == owner) {
    ret = pipe_mgr_tcam_default_ent_place(dev_tgt,
                                          mat_tbl_hdl,
                                          act_fn_hdl,
                                          act_spec,
                                          flags,
                                          ent_hdl_p,
                                          move_list);
  } else if (PIPE_MGR_TBL_OWNER_ALPM == owner) {
    ret = pipe_mgr_alpm_default_ent_place(dev_tgt,
                                          mat_tbl_hdl,
                                          act_fn_hdl,
                                          act_spec,
                                          flags,
                                          ent_hdl_p,
                                          move_list);
  } else if (PIPE_MGR_TBL_OWNER_PHASE0 == owner) {
    ret = pipe_mgr_phase0_default_ent_place(dev_tgt,
                                            mat_tbl_hdl,
                                            act_fn_hdl,
                                            act_spec,
                                            flags & PIPE_MGR_TBL_API_TXN,
                                            ent_hdl_p,
                                            move_list);
  } else if (PIPE_MGR_TBL_OWNER_NO_KEY == owner) {
    /* Nothing to do except set the ent hdl */
    *ent_hdl_p = PIPE_MGR_TBL_NO_KEY_DEFAULT_ENTRY_HDL;
    mat_tbl_info->tbl_no_key_is_default_entry_valid = true;
    ret = PIPE_SUCCESS;
  } else {
    LOG_ERROR("No handler for table owner %u, session %u, table 0x%x at %s:%d",
              owner,
              sess_hdl,
              mat_tbl_hdl,
              __func__,
              __LINE__);
    PIPE_MGR_DBGCHK(0);
    ret = PIPE_UNEXPECTED;
  }
  return ret;
}

pipe_status_t pipe_mgr_get_default_entry(pipe_sess_hdl_t sess_hdl,
                                         dev_target_t dev_tgt,
                                         pipe_mat_tbl_info_t *mat_tbl_info,
                                         pipe_action_spec_t *pipe_action_spec,
                                         pipe_act_fn_hdl_t *act_fn_hdl,
                                         bool from_hw) {
  pipe_status_t ret = PIPE_SUCCESS;
  pipe_mat_tbl_hdl_t mat_tbl_hdl = mat_tbl_info->handle;
  enum pipe_mgr_table_owner_t owner;
  owner = pipe_mgr_sm_tbl_owner(dev_tgt.device_id, mat_tbl_hdl);

  if (PIPE_MGR_TBL_OWNER_EXM == owner) {
    ret = pipe_mgr_exm_default_ent_get(
        sess_hdl, dev_tgt, mat_tbl_hdl, pipe_action_spec, act_fn_hdl, from_hw);
  } else if (PIPE_MGR_TBL_OWNER_TRN == owner) {
    ret = pipe_mgr_tcam_default_ent_get(
        sess_hdl, dev_tgt, mat_tbl_hdl, pipe_action_spec, act_fn_hdl, from_hw);
  } else if (PIPE_MGR_TBL_OWNER_ALPM == owner) {
    ret = pipe_mgr_alpm_default_ent_get(
        sess_hdl, dev_tgt, mat_tbl_hdl, pipe_action_spec, act_fn_hdl, from_hw);
  } else if (PIPE_MGR_TBL_OWNER_PHASE0 == owner) {
    ret = pipe_mgr_phase0_default_ent_get(
        dev_tgt, mat_tbl_hdl, pipe_action_spec, act_fn_hdl, from_hw);
  } else if (PIPE_MGR_TBL_OWNER_NO_KEY == owner) {
    *act_fn_hdl = mat_tbl_info->act_fn_hdl_info[0].act_fn_hdl;
    pipe_action_spec->pipe_action_datatype_bmap = PIPE_ACTION_DATA_TYPE;
    ret = PIPE_SUCCESS;
  } else {
    LOG_ERROR("No handler for table owner %u, session %u, table %u at %s:%d",
              owner,
              sess_hdl,
              mat_tbl_hdl,
              __func__,
              __LINE__);
    ret = PIPE_OBJ_NOT_FOUND;
  }
  return ret;
}

pipe_status_t pipe_mgr_tbl_process_move_list(pipe_sess_hdl_t sess_hdl,
                                             bf_dev_id_t dev_id,
                                             pipe_mat_tbl_hdl_t tbl_hdl,
                                             enum pipe_mgr_table_owner_t owner,
                                             pipe_mgr_move_list_t *move_list,
                                             uint32_t *processed,
                                             bool free_when_done) {
  pipe_status_t ret = PIPE_SUCCESS;
  switch (owner) {
    case PIPE_MGR_TBL_OWNER_EXM:
      ret = pipe_mgr_exm_tbl_process_move_list(
          sess_hdl, dev_id, tbl_hdl, move_list, processed);
      break;
    case PIPE_MGR_TBL_OWNER_TRN:
      ret = pipe_mgr_tcam_process_move_list(
          sess_hdl, dev_id, tbl_hdl, move_list, processed);
      break;
    case PIPE_MGR_TBL_OWNER_ALPM:
      ret = pipe_mgr_alpm_process_move_list(
          sess_hdl, dev_id, tbl_hdl, move_list, processed);
      break;
    case PIPE_MGR_TBL_OWNER_PHASE0:
      ret = pipe_mgr_phase0_ent_program(
          sess_hdl, dev_id, tbl_hdl, move_list, processed);
      break;
    case PIPE_MGR_TBL_OWNER_ADT:
      ret = pipe_adt_process_move_list(sess_hdl,
                                       dev_id,
                                       tbl_hdl,
                                       (pipe_mgr_adt_move_list_t *)move_list,
                                       processed);
      break;
    case PIPE_MGR_TBL_OWNER_SELECT:
      ret = pipe_sel_process_move_list(sess_hdl,
                                       dev_id,
                                       tbl_hdl,
                                       (pipe_mgr_sel_move_list_t *)move_list,
                                       processed);
      break;
    case PIPE_MGR_TBL_OWNER_NO_KEY:
      break;
    case PIPE_MGR_TBL_OWNER_STFUL:
      ret = pipe_mgr_stful_process_op_list(
          sess_hdl,
          dev_id,
          tbl_hdl,
          (struct pipe_mgr_stful_op_list_t *)move_list,
          processed);
      break;
    case PIPE_MGR_TBL_OWNER_METER:
      ret = pipe_mgr_meter_process_op_list(
          sess_hdl,
          dev_id,
          tbl_hdl,
          (struct pipe_mgr_meter_op_list_t *)move_list,
          processed);
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      ret = PIPE_INVALID_ARG;
  }

  if (free_when_done) pipe_mgr_tbl_free_move_list(owner, move_list, true);
  return ret;
}

void pipe_mgr_tbl_free_move_list(enum pipe_mgr_table_owner_t owner,
                                 pipe_mgr_move_list_t *move_list,
                                 bool free_data) {
  switch (owner) {
    case PIPE_MGR_TBL_OWNER_EXM:
    case PIPE_MGR_TBL_OWNER_TRN:
    case PIPE_MGR_TBL_OWNER_ALPM:
    case PIPE_MGR_TBL_OWNER_PHASE0:
      free_move_list(&move_list, true);
      break;
    case PIPE_MGR_TBL_OWNER_ADT:
      free_adt_move_list((pipe_mgr_adt_move_list_t **)&move_list);
      break;
    case PIPE_MGR_TBL_OWNER_SELECT:
      if (free_data)
        free_sel_move_list_and_data((pipe_mgr_sel_move_list_t **)&move_list);
      else
        free_sel_move_list((pipe_mgr_sel_move_list_t **)&move_list);
      break;
    case PIPE_MGR_TBL_OWNER_NO_KEY:
      break;
    case PIPE_MGR_TBL_OWNER_STFUL:
      pipe_mgr_stful_free_ops((struct pipe_mgr_stful_op_list_t **)&move_list);
      break;
    case PIPE_MGR_TBL_OWNER_METER:
      pipe_mgr_meter_free_ops((struct pipe_mgr_meter_op_list_t **)&move_list);
    default:
      break;
  }
}
