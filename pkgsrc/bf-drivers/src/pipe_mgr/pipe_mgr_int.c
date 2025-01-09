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
 * @file pipe_mgr_int.c
 * @date
 *
 * Implementation of some internal pipe manager functions
 */

/* Standard header includes */
#include <unistd.h>

/* Module header includes */
#include <pipe_mgr/pipe_mgr_intf.h>
#include <dvm/bf_drv_intf.h>
#include <lld/bf_dev_if.h>
#include <lld/bf_dma_if.h>
#include <lld/lld_inst_list_fmt.h>
#include <lld/lld_err.h>
#include <lld/lld_sku.h>

/* Local header includes */
#include "pipe_mgr_exm_tbl_mgr.h"
#include "pipe_mgr_tcam.h"
#include "pipe_mgr_tcam_hw.h"
#include "pipe_mgr_alpm.h"
#include "pipe_mgr_idle.h"
#include "pipe_mgr_phy_mem_map.h"
#include "pipe_mgr_drv.h"
#include "pipe_mgr_interrupt.h"
#include "pipe_mgr_meter_tbl_init.h"
#include "pipe_mgr_stful_tbl_mgr.h"
#include "pipe_mgr_mirror_buffer.h"
#include "pipe_mgr_phase0_tbl_mgr.h"
#include <tofino_regs/tofino.h>
#include "pipe_mgr_parde.h"
#include "pipe_mgr_db.h"

static bool pipe_mgr_dev_locked[PIPE_MGR_NUM_DEVICES] = {0};

extern pipe_mgr_ctx_t *pipe_mgr_ctx;

bf_subdev_id_t pipe_mgr_subdev_id_from_pipe(bf_dev_pipe_t phy_pipe) {
  return phy_pipe / BF_SUBDEV_PIPE_COUNT;
}

bool pipe_mgr_valid_deviceId(bf_dev_id_t dev,
                             const char *where,
                             const int line) {
  if (dev >= PIPE_MGR_NUM_DEVICES || dev < 0) {
    LOG_ERROR("Invalid device Id %d at %s:%d", dev, where, line);
    return false;
  }
  return true;
}

bool pipe_mgr_valid_dev_tgt(bf_dev_target_t dt,
                            const char *where,
                            const int line) {
  bool ret;
  if (pipe_mgr_valid_deviceId(dt.device_id, where, line) &&
      (BF_DEV_PIPE_ALL == dt.dev_pipe_id ||
       pipe_mgr_get_num_active_pipes(dt.device_id) > dt.dev_pipe_id)) {
    ret = true;
  } else {
    LOG_ERROR("Invalid dev target %d.%d at %s:%d",
              dt.device_id,
              dt.dev_pipe_id,
              where,
              line);
    ret = false;
  }
  return ret;
}

pipe_status_t pipe_mgr_mat_ent_get_dir_ent_location(
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    uint32_t subindex,
    bf_dev_pipe_t *pipe_id_p,
    dev_stage_t *stage_id_p,
    rmt_tbl_hdl_t *stage_table_hdl_p,
    uint32_t *index_p) {
  enum pipe_mgr_table_owner_t owner;
  owner = pipe_mgr_sm_tbl_owner(device_id, mat_tbl_hdl);
  if (PIPE_MGR_TBL_OWNER_EXM == owner) {
    if (subindex != 0) {
      return PIPE_OBJ_NOT_FOUND;
    }
    return pipe_mgr_exm_get_dir_ent_idx(device_id,
                                        mat_tbl_hdl,
                                        mat_ent_hdl,
                                        pipe_id_p,
                                        stage_id_p,
                                        stage_table_hdl_p,
                                        index_p);
  } else if (PIPE_MGR_TBL_OWNER_TRN == owner) {
    return pipe_mgr_tcam_entry_get_programmed_location(device_id,
                                                       mat_tbl_hdl,
                                                       mat_ent_hdl,
                                                       subindex,
                                                       pipe_id_p,
                                                       stage_id_p,
                                                       stage_table_hdl_p,
                                                       index_p);
  } else if (PIPE_MGR_TBL_OWNER_ALPM == owner) {
    return pipe_mgr_alpm_entry_get_location(device_id,
                                            mat_tbl_hdl,
                                            mat_ent_hdl,
                                            subindex,
                                            pipe_id_p,
                                            stage_id_p,
                                            stage_table_hdl_p,
                                            index_p);
  } else {
    PIPE_MGR_DBGCHK(0);
    return PIPE_OBJ_NOT_FOUND;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_mat_tbl_update_lock_type(bf_dev_id_t device_id,
                                                pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                                bool idle,
                                                bool stat,
                                                bool add_lock) {
  enum pipe_mgr_table_owner_t owner;
  owner = pipe_mgr_sm_tbl_owner(device_id, mat_tbl_hdl);
  if (PIPE_MGR_TBL_OWNER_EXM == owner) {
    return pipe_mgr_exm_update_lock_type(
        device_id, mat_tbl_hdl, idle, stat, add_lock);
  } else if (PIPE_MGR_TBL_OWNER_TRN == owner) {
    return pipe_mgr_tcam_update_lock_type(
        device_id, mat_tbl_hdl, idle, stat, add_lock);
  } else {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_mat_tbl_update_idle_init_val(
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    uint32_t idle_init_val_for_ttl_0) {
  enum pipe_mgr_table_owner_t owner;
  owner = pipe_mgr_sm_tbl_owner(device_id, mat_tbl_hdl);
  if (PIPE_MGR_TBL_OWNER_EXM == owner) {
    return pipe_mgr_exm_update_idle_init_val(
        device_id, mat_tbl_hdl, idle_init_val_for_ttl_0);
  } else if (PIPE_MGR_TBL_OWNER_TRN == owner) {
    return pipe_mgr_tcam_update_idle_init_val(
        device_id, mat_tbl_hdl, idle_init_val_for_ttl_0);
  } else {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_mat_tbl_reset_idle(pipe_sess_hdl_t sess_hdl,
                                          bf_dev_id_t device_id,
                                          pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                          bf_dev_pipe_t pipe_id,
                                          uint8_t stage_id,
                                          mem_id_t mem_id,
                                          uint32_t mem_offset) {
  enum pipe_mgr_table_owner_t owner;
  owner = pipe_mgr_sm_tbl_owner(device_id, mat_tbl_hdl);
  if (PIPE_MGR_TBL_OWNER_EXM == owner) {
    return pipe_mgr_exm_reset_idle(sess_hdl,
                                   device_id,
                                   mat_tbl_hdl,
                                   pipe_id,
                                   stage_id,
                                   mem_id,
                                   mem_offset);
  } else if (PIPE_MGR_TBL_OWNER_TRN == owner) {
    return pipe_mgr_tcam_reset_idle(sess_hdl,
                                    device_id,
                                    mat_tbl_hdl,
                                    pipe_id,
                                    stage_id,
                                    mem_id,
                                    mem_offset);
  } else {
    LOG_ERROR("%s:%d Idletime reset not supported for table 0x%x device %d",
              __func__,
              __LINE__,
              mat_tbl_hdl,
              device_id);
    return PIPE_INVALID_ARG;
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_mat_tbl_gen_lock_id(bf_dev_id_t device_id,
                                           pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                           pipe_mgr_lock_id_type_e lock_id_type,
                                           lock_id_t *lock_id_p) {
  enum pipe_mgr_table_owner_t owner;
  owner = pipe_mgr_sm_tbl_owner(device_id, mat_tbl_hdl);
  if (PIPE_MGR_TBL_OWNER_EXM == owner) {
    return pipe_mgr_exm_gen_lock_id(
        device_id, mat_tbl_hdl, lock_id_type, lock_id_p);
  } else if (PIPE_MGR_TBL_OWNER_TRN == owner) {
    return pipe_mgr_tcam_gen_lock_id(
        device_id, mat_tbl_hdl, lock_id_type, lock_id_p);
  } else {
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_mat_tbl_get_dir_stat_tbl_hdl(
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_stat_tbl_hdl_t *stat_tbl_hdl_p) {
  pipe_mat_tbl_info_t *mat_tbl_info = NULL;

  if (stat_tbl_hdl_p == NULL) {
    return PIPE_INVALID_ARG;
  }

  mat_tbl_info =
      pipe_mgr_get_tbl_info(device_id, mat_tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    return PIPE_OBJ_NOT_FOUND;
  }
  if (mat_tbl_info->num_stat_tbl_refs == 0) {
    return PIPE_OBJ_NOT_FOUND;
  }
  if (mat_tbl_info->stat_tbl_ref[0].ref_type == PIPE_TBL_REF_TYPE_DIRECT) {
    *stat_tbl_hdl_p = mat_tbl_info->stat_tbl_ref[0].tbl_hdl;
    return PIPE_SUCCESS;
  }

  return PIPE_OBJ_NOT_FOUND;
}

pipe_status_t pipe_mgr_mat_tbl_get_dir_meter_tbl_hdl(
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_meter_tbl_hdl_t *meter_tbl_hdl_p) {
  pipe_mat_tbl_info_t *mat_tbl_info = NULL;

  if (meter_tbl_hdl_p == NULL) {
    return PIPE_INVALID_ARG;
  }

  mat_tbl_info =
      pipe_mgr_get_tbl_info(device_id, mat_tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    return PIPE_OBJ_NOT_FOUND;
  }
  if (mat_tbl_info->num_meter_tbl_refs == 0) {
    return PIPE_OBJ_NOT_FOUND;
  }
  if (mat_tbl_info->meter_tbl_ref[0].ref_type == PIPE_TBL_REF_TYPE_DIRECT) {
    *meter_tbl_hdl_p = mat_tbl_info->meter_tbl_ref[0].tbl_hdl;
    return PIPE_SUCCESS;
  }

  return PIPE_OBJ_NOT_FOUND;
}

bf_status_t pipe_mgr_lock_device(bf_dev_id_t dev_id) {
  bf_status_t status = BF_SUCCESS;
  LOG_TRACE("Entering %s, dev %d ", __func__, dev_id);

  if (dev_id >= PIPE_MGR_NUM_DEVICES) {
    return BF_INVALID_ARG;
  }

  pipe_mgr_dev_locked[dev_id] = true;

  return status;
}

bf_status_t pipe_mgr_unlock_device_cleanup(bf_dev_id_t dev_id) {
  bf_status_t status = BF_SUCCESS;
  LOG_TRACE("Entering %s, dev %d ", __func__, dev_id);

  if (dev_id >= PIPE_MGR_NUM_DEVICES) {
    return BF_INVALID_ARG;
  }

  pipe_mgr_dev_locked[dev_id] = false;

  return status;
}

bf_status_t pipe_mgr_reconfig_error_cleanup(bf_dev_id_t dev_id) {
  bf_status_t status = BF_SUCCESS;
  LOG_TRACE("Entering %s, dev %d ", __func__, dev_id);

  if (dev_id >= PIPE_MGR_NUM_DEVICES) {
    return BF_INVALID_ARG;
  }

  /* Release the api lock taken by create_dma */
  pipe_mgr_exclusive_api_exit(dev_id);

  pipe_mgr_unlock_device_cleanup(dev_id);

  return status;
}

/* write the scratch registers from the cache to update the symmetric mode in
 * the HW for HA */
static pipe_status_t write_scratch_reg(rmt_dev_info_t *dev_info,
                                       bf_dev_id_t dev_id,
                                       pipe_sess_hdl_t sess_hdl) {
  pipe_status_t rc = PIPE_SUCCESS;
  bf_dev_pipe_t phy_pipe = 0;
  uint32_t stage_id;
  uint32_t profile_id;
  uint32_t reg_addr = 0, reg_val = 0;
  // Update the symmetricity
  for (profile_id = 0; profile_id < dev_info->num_pipeline_profiles;
       profile_id++) {
    for (stage_id = 0; stage_id < dev_info->num_active_mau; stage_id++) {
      bf_dev_pipe_t pipe = dev_info->profile_info[profile_id]->lowest_pipe;
      reg_val = dev_info->mau_scratch_val[pipe][stage_id];
      pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);
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
          reg_addr = offsetof(tof3_reg,
                              pipes[phy_pipe % BF_SUBDEV_PIPE_COUNT]
                                  .mau[stage_id]
                                  .dp.mau_scratch);
          break;




        default:
          return PIPE_INVALID_ARG;
      }
      // Add the instructions to the session ilist to be written
      // after core reset.
      rc = pipe_mgr_sess_ilist_add_register_write(
          sess_hdl, dev_id, phy_pipe / BF_SUBDEV_PIPE_COUNT, reg_addr, reg_val);

      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s: Setting asym/symmetric mode failed on resource tables: "
            "sess %d, dev %d, tbl 0x%x, sts %s",
            __func__,
            sess_hdl,
            dev_id,
            0,
            pipe_str_err(rc));
      }
    }
  }
  return rc;
}
bf_status_t pipe_mgr_reconfig_create_dma(bf_dev_id_t dev_id) {
  bf_status_t status = BF_SUCCESS;
  LOG_TRACE("Entering %s, dev %d ", __func__, dev_id);
  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);

  if (!dev_info || dev_id >= PIPE_MGR_NUM_DEVICES) {
    return BF_INVALID_ARG;
  }

  // prepare to write scratch regs of all profiles & stages to hw
  pipe_status_t rc = write_scratch_reg(dev_info, dev_id, shdl);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s: failed to write scratch register", __func__);
  }

  if (pipe_mgr_init_mode_fast_recfg(dev_id) ||
      pipe_mgr_init_mode_fast_recfg_quick(dev_id)) {
    pipe_mgr_exclusive_api_enter(dev_id);
  }

  status = phy_mem_map_load_srams_tcams(dev_info, false);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s: failed to load srams,tcams", __func__);
    return status;
  }
  LOG_TRACE("%s: dev %d sram and tcam init done ", __func__, dev_id);

  status = pipe_mgr_parser_config_create_dma(shdl, dev_info);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR("Failed to push parser configuration for reconfig (%s)",
              pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }

  status = pipe_mgr_idle_fast_reconfig_push_state(shdl, dev_id);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR("Failed to push idletime state for reconfig (%s)",
              pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  LOG_TRACE("%s: dev %d idle state dnld posted", __func__, dev_id);

#ifndef EMU_SKIP_BLOCKS_OPT
  status = pipe_mgr_mirror_buf_cfg_sessions(shdl, dev_info);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR("Failed to push mirror state for reconfig (%s)",
              pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  LOG_TRACE("%s: dev %d mirror state dnld posted", __func__, dev_id);

  status = pipe_mgr_mirrtbl_write(shdl, dev_info);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR("Failed to push mirror table entries for reconfig (%s)",
              pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  LOG_TRACE("%s: dev %d mirror table entries dnld posted", __func__, dev_id);
#endif

  status = pipe_mgr_phase0_create_dma(shdl, dev_info);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR("Failed to push P0 entries for reconfig (%s)",
              pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  LOG_TRACE("%s: dev %d p0 entries dnld posted", __func__, dev_id);

  status = pipe_mgr_pktgen_create_dma(shdl, dev_info);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR("Failed to load pgen config for reconfig (%s)",
              pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  LOG_TRACE("%s: dev %d pgen config dnld posted", __func__, dev_id);

  status = pipe_mgr_imem_write(shdl, dev_info, BF_DEV_PIPE_ALL, -1, true);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR("Failed to load imem config for reconfig (%s)",
              pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  LOG_TRACE("%s: dev %d imem config dnld posted", __func__, dev_id);

  status = pipe_mgr_register_interrupt_notifs(dev_info);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR("Failed to register interrupt callbacks (%s)",
              pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  LOG_TRACE("%s: dev %d interrupt callbacks registered", __func__, dev_id);

  // TODO - Move a few things from unlock up here...
  // load_fm_buffers
  // register_interrupt_notifs
  // pushing wr-blk and ilist buffers into the DR
  LOG_TRACE("Exiting %s, dev %d, with status %s",
            __func__,
            dev_id,
            pipe_str_err(status));
  return status;
}

bf_status_t pipe_mgr_disable_traffic(bf_dev_id_t dev_id) {
  LOG_TRACE("Entering %s, dev %d ", __func__, dev_id);
  bf_status_t sts = BF_SUCCESS;
  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s:%d Device info for device id %d not found",
              __func__,
              __LINE__,
              dev_id);
    return BF_OBJECT_NOT_FOUND;
  }
  if (dev_info->dev_family != BF_DEV_FAMILY_TOFINO) {
    /* PipeMgr will not disable ParDe channels in the disable traffic step for
     * non-Tofino chips.  Traffic will be disabled outside of IPB and EBUF. */
    return PIPE_SUCCESS;
  }

  /* Disable input traffic traffic. */
  sts = pipe_mgr_parde_traffic_disable(shdl, dev_info);

  return sts;
}

bf_status_t pipe_mgr_wait_for_traffic_flush(bf_dev_id_t dev_id) {
  LOG_TRACE("Entering %s, dev %d ", __func__, dev_id);
  bf_status_t sts = BF_SUCCESS;
  bool is_sw_model;

  /* For model, this step needs to be skipped */
  sts = bf_drv_device_type_get(dev_id, &is_sw_model);
  if (sts != BF_SUCCESS) {
    LOG_ERROR("%s:%d: Device type get failed for device %d, status %s (%d)",
              __func__,
              __LINE__,
              dev_id,
              bf_err_str(sts),
              sts);

    return (sts);
  }

  if (is_sw_model) {
    LOG_TRACE("Skipping wait_for_traffic_flush step on model");
    return BF_SUCCESS;
  }

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s:%d Device info for device id %d not found",
              __func__,
              __LINE__,
              dev_id);
    return BF_OBJECT_NOT_FOUND;
  }

  /* Wait for all egress traffic flush in EBUF */
  sts = pipe_mgr_parde_wait_for_traffic_flush(shdl, dev_info);

  return sts;
}

bf_status_t pipe_mgr_config_complete(bf_dev_id_t dev_id) {
  LOG_TRACE("Entering %s, dev %d ", __func__, dev_id);
  bf_status_t sts = BF_SUCCESS;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s:%d Device info for device id %d not found",
              __func__,
              __LINE__,
              dev_id);
    return BF_INVALID_ARG;
  }

  pipe_sess_hdl_t int_shdl = pipe_mgr_ctx->int_ses_hndl;

  /* For Tofino-2 and later traffic will be enabled by port_mgr through MAC
   * register programming.  Therefore we must complete all of our config now
   * before the enable_input_pkts step.
   * For Tofino-1 we only need to wait for the ilists to complete so that the
   * PGR register programming for CPU ports is in place and credits can be given
   * to PGR for the CPU port(s). */
  if (dev_info->dev_family != BF_DEV_FAMILY_TOFINO) {
    pipe_mgr_drv_wr_blk_cmplt_all(int_shdl, dev_id);
    /* Set the slow-mode configuration so that it is applied before any other
     * configuration pushed later. */
    sts = pipe_mgr_set_mem_slow_mode(dev_info, true);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR("Failed to set mem slow mode for dev %d sts %s",
                dev_id,
                pipe_str_err(sts));
      return sts;
    }
    for (pipe_sess_hdl_t shdl = 0; shdl < PIPE_MGR_MAX_SESSIONS; ++shdl) {
      if (!pipe_mgr_session_exists(shdl)) continue;
      pipe_mgr_drv_i_list_cmplt_all(&shdl);
    }
  } else {
    sts = pipe_mgr_drv_i_list_cmplt_all(&int_shdl);
  }

  LOG_TRACE("Exiting %s, dev %d ", __func__, dev_id);

  return sts;
}

pipe_status_t pipe_mgr_hw_notify_cfg(rmt_dev_info_t *dev_info) {
  pipe_status_t sts = PIPE_SUCCESS;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return PIPE_SUCCESS;
    case BF_DEV_FAMILY_TOFINO2: {
      for (unsigned l_pipe = 0; l_pipe < dev_info->num_active_pipes; ++l_pipe) {
        bf_dev_pipe_t p_pipe;
        pipe_mgr_map_pipe_id_log_to_phy(dev_info, l_pipe, &p_pipe);
        for (unsigned stg = 0; stg < dev_info->num_active_mau; ++stg) {
          uint32_t addr = offsetof(
              tof2_reg, pipes[p_pipe].mau[stg].cfg_regs.stage_dump_ctl);
          uint32_t data = 0;
          setp_tof2_stage_dump_ctl_stage_dump_ctl_stage(&data, stg);
          setp_tof2_stage_dump_ctl_stage_dump_ctl_pipe(&data, p_pipe);
          sts = pipe_mgr_write_register(dev_info->dev_id, 0, addr, data);
          if (sts != PIPE_SUCCESS) {
            LOG_ERROR(
                "Dev %d failed to set stage dump ctl, phy pipe %d stage %d sts "
                "%s",
                dev_info->dev_id,
                p_pipe,
                stg,
                pipe_str_err(sts));
            PIPE_MGR_DBGCHK(sts == PIPE_SUCCESS);
            return sts;
          }
        }
      }
      return PIPE_SUCCESS;
    }
    case BF_DEV_FAMILY_TOFINO3: {
      for (unsigned l_pipe = 0; l_pipe < dev_info->num_active_pipes; ++l_pipe) {
        bf_dev_pipe_t p_pipe;
        pipe_mgr_map_pipe_id_log_to_phy(dev_info, l_pipe, &p_pipe);
        bf_subdev_id_t subdev = pipe_mgr_subdev_id_from_pipe(p_pipe);
        p_pipe = p_pipe % BF_SUBDEV_PIPE_COUNT;

        for (unsigned stg = 0; stg < dev_info->num_active_mau; ++stg) {
          uint32_t addr = offsetof(
              tof3_reg, pipes[p_pipe].mau[stg].cfg_regs.stage_dump_ctl);
          uint32_t data = 0;
          setp_tof3_stage_dump_ctl_stage_dump_ctl_stage(&data, stg);
          setp_tof3_stage_dump_ctl_stage_dump_ctl_pipe(&data, p_pipe);
          sts = pipe_mgr_write_register(dev_info->dev_id, subdev, addr, data);
          if (sts != PIPE_SUCCESS) {
            LOG_ERROR(
                "Dev %d failed to set stage dump ctl, phy pipe %d stage %d sts "
                "%s",
                dev_info->dev_id,
                p_pipe,
                stg,
                pipe_str_err(sts));
            PIPE_MGR_DBGCHK(sts == PIPE_SUCCESS);
            return sts;
          }
        }
      }
      return PIPE_SUCCESS;
    }

      // Currently there are no stage_dump_ctl regs in spec

    case BF_DEV_FAMILY_UNKNOWN:
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }
  PIPE_MGR_DBGCHK(0);
  return PIPE_UNEXPECTED;
}

/* Internal Sesson push
   dev_id - Device-id
   reconfig - After fast-reconfig
*/
pipe_status_t pipe_mgr_internal_session_push(bf_dev_id_t dev_id,
                                             bool reconfig) {
  pipe_status_t status = PIPE_SUCCESS;

  /* Push wr blks to dr */
  status = pipe_mgr_push_wr_blks_to_dr(&pipe_mgr_ctx->int_ses_hndl, dev_id);
  if (PIPE_SUCCESS != status) {
    LOG_ERROR("Failed to push wr blk to drs (%d)", status);
    PIPE_MGR_DBGCHK(PIPE_SUCCESS == status);
    return status;
  }

  /* Push default session */
  if (reconfig) {
    status =
        pipe_mgr_drv_reconfig_ilist_push(&pipe_mgr_ctx->int_ses_hndl, dev_id);
    if (PIPE_SUCCESS != status) {
      LOG_ERROR("Failed to push instruction list (%d)", status);
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == status);
      return status;
    }
  } else {
    status = pipe_mgr_drv_ilist_push(&pipe_mgr_ctx->int_ses_hndl, NULL, NULL);
    if (PIPE_SUCCESS != status) {
      LOG_ERROR("Failed to push instruction list (%d)", status);
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == status);
      return status;
    }
  }

  if (!reconfig) {
    status = pipe_mgr_complete_operations(pipe_mgr_ctx->int_ses_hndl);
    if (PIPE_SUCCESS != status) {
      LOG_ERROR("Failed to complete instruction list (%d)", status);
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == status);
      return status;
    }
  }

  return status;
}

/* pipe_mgr_enable_all_dr
 *
 * Enables all the DRs used by the pipe manager
 */
void pipe_mgr_enable_all_dr(rmt_dev_info_t *dev_info) {
  bf_dev_id_t dev_id = dev_info->dev_id;
  bf_dma_dr_id_t dr_id, tx_end = 0;

  /* Tofino1 uses one ilist DR per pipe, while Tofino2 use multicast to program
     mupltiple pipes with single ilist message, second ilist is used by mcast
     mgr updates. */
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      tx_end = lld_dr_tx_pipe_inst_list_3;
      break;
    case BF_DEV_FAMILY_TOFINO2:
    case BF_DEV_FAMILY_TOFINO3:
      tx_end = lld_dr_tx_pipe_inst_list_1;
      break;






    case BF_DEV_FAMILY_UNKNOWN:
      PIPE_MGR_DBGCHK(0);
      return;
  }

  // Enable lld_dr_fm_lrt, lld_dr_fm_idle, lld_dr_fm_learn
  for (dr_id = lld_dr_fm_lrt; dr_id <= lld_dr_fm_learn; dr_id++) {
    lld_dr_enable_set(dev_id, dr_id, true);
  }

  // Enable lld_dr_tx_pipe_inst_list_0,1,2,3
  for (dr_id = lld_dr_tx_pipe_inst_list_0; dr_id <= tx_end; dr_id++) {
    lld_dr_enable_set(dev_id, dr_id, true);
  }

  /* Enable lld_dr_tx_pipe_write_block and lld_dr_tx_pipe_read_block */
  for (dr_id = lld_dr_tx_pipe_write_block; dr_id <= lld_dr_tx_pipe_read_block;
       dr_id++) {
    lld_dr_enable_set(dev_id, dr_id, true);
  }

  // Enable lld_dr_rx_lrt, lld_dr_rx_idle and lld_dr_rx_learn DRs
  for (dr_id = lld_dr_rx_lrt; dr_id <= lld_dr_rx_learn; dr_id++) {
    lld_dr_enable_set(dev_id, dr_id, true);
  }

  /* Enable lld_dr_cmp_pipe_inst_list_0,1,2,3 */
  for (dr_id = lld_dr_cmp_pipe_inst_list_0;
       dr_id <= lld_dr_cmp_pipe_inst_list_3;
       dr_id++) {
    lld_dr_enable_set(dev_id, dr_id, true);
  }

  /* Enable lld_dr_cmp_pipe_write_blk and lld_dr_cmp_pipe_read_blk */
  for (dr_id = lld_dr_cmp_pipe_write_blk; dr_id <= lld_dr_cmp_pipe_read_blk;
       dr_id++) {
    lld_dr_enable_set(dev_id, dr_id, true);
  }
}

/* pipe_mgr_disable_all_dr
 *
 * Disables all the DRs used by the pipe manager
 */
void pipe_mgr_disable_all_dr(bf_dev_id_t dev_id) {
  bf_dma_dr_id_t dr_id;

  // Disable lld_dr_fm_lrt, lld_dr_fm_idle, lld_dr_fm_learn
  for (dr_id = lld_dr_fm_lrt; dr_id <= lld_dr_fm_learn; dr_id++) {
    lld_dr_enable_set(dev_id, dr_id, false);
  }

  // Disable lld_dr_tx_pipe_inst_list_0,1,2,3
  for (dr_id = lld_dr_tx_pipe_inst_list_0; dr_id <= lld_dr_tx_pipe_inst_list_3;
       dr_id++) {
    lld_dr_enable_set(dev_id, dr_id, false);
  }

  /* Disable lld_dr_tx_pipe_write_block and lld_dr_tx_pipe_read_block */
  for (dr_id = lld_dr_tx_pipe_write_block; dr_id <= lld_dr_tx_pipe_read_block;
       dr_id++) {
    lld_dr_enable_set(dev_id, dr_id, false);
  }

  // Disable lld_dr_rx_lrt, lld_dr_rx_idle and lld_dr_rx_learn DRs
  for (dr_id = lld_dr_rx_lrt; dr_id <= lld_dr_rx_learn; dr_id++) {
    lld_dr_enable_set(dev_id, dr_id, false);
  }

  /* Disable lld_dr_cmp_pipe_inst_list_0,1,2,3 */
  for (dr_id = lld_dr_cmp_pipe_inst_list_0;
       dr_id <= lld_dr_cmp_pipe_inst_list_3;
       dr_id++) {
    lld_dr_enable_set(dev_id, dr_id, false);
  }

  /* Disable lld_dr_cmp_pipe_write_blk and lld_dr_cmp_pipe_read_blk */
  for (dr_id = lld_dr_cmp_pipe_write_blk; dr_id <= lld_dr_cmp_pipe_read_blk;
       dr_id++) {
    lld_dr_enable_set(dev_id, dr_id, false);
  }
}

void pipe_mgr_set_pbus_weights(rmt_dev_info_t *dev_info, int weight) {
  pipe_status_t rc;
  bf_dev_pipe_t phy_pipe, log_pipe;
  uint32_t addr, data = 0;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      for (phy_pipe = 0; phy_pipe < dev_info->dev_cfg.num_pipelines;
           ++phy_pipe) {
        rc = pipe_mgr_map_phy_pipe_id_to_log_pipe_id_optimized(
            dev_info, phy_pipe, &log_pipe);
        if (rc == PIPE_SUCCESS) {
          setp_pbus_arb_ctrl1_host_master_resp_weight(&data, weight);
          setp_pbus_arb_ctrl1_interrupt_req_weight(&data, weight);
          setp_pbus_arb_ctrl1_stat_req_weight(&data, weight);
          setp_pbus_arb_ctrl1_idle_req_weight(&data, weight);
        } else {
          data = 0;
        }
        addr = offsetof(Tofino, device_select.pbc.pbc_pbus.arb_ctrl1[phy_pipe]);
        lld_write_register(dev_info->dev_id, addr, data);
      }
      break;
    case BF_DEV_FAMILY_TOFINO2:
      for (phy_pipe = 0; phy_pipe < dev_info->dev_cfg.num_pipelines;
           ++phy_pipe) {
        rc = pipe_mgr_map_phy_pipe_id_to_log_pipe_id_optimized(
            dev_info, phy_pipe, &log_pipe);
        if (rc == PIPE_SUCCESS) {
          setp_tof2_pbus_arb_ctrl1_host_master_resp_weight(&data, weight);
          setp_tof2_pbus_arb_ctrl1_interrupt_req_weight(&data, weight);
          setp_tof2_pbus_arb_ctrl1_stat_req_weight(&data, weight);
          setp_tof2_pbus_arb_ctrl1_idle_req_weight(&data, weight);
        } else {
          data = 0;
        }
        addr =
            offsetof(tof2_reg, device_select.pbc.pbc_pbus.arb_ctrl1[phy_pipe]);
        lld_write_register(dev_info->dev_id, addr, data);
      }
      break;
    case BF_DEV_FAMILY_TOFINO3:
      for (phy_pipe = 0; phy_pipe < dev_info->dev_cfg.num_pipelines;
           ++phy_pipe) {
        rc = pipe_mgr_map_phy_pipe_id_to_log_pipe_id_optimized(
            dev_info, phy_pipe, &log_pipe);
        if (rc == PIPE_SUCCESS) {
          setp_tof3_pbus_arb_ctrl1_host_master_resp_weight(&data, weight);
          setp_tof3_pbus_arb_ctrl1_interrupt_req_weight(&data, weight);
          setp_tof3_pbus_arb_ctrl1_stat_req_weight(&data, weight);
          setp_tof3_pbus_arb_ctrl1_idle_req_weight(&data, weight);
          setp_tof3_pbus_arb_ctrl1_lq_req_weight(&data, weight);
        } else {
          data = 0;
        }
        addr = offsetof(tof3_reg,
                        device_select.pbc.pbc_pbus
                            .arb_ctrl1[phy_pipe % BF_SUBDEV_PIPE_COUNT]);
        lld_subdev_write_register(dev_info->dev_id,
                                  pipe_mgr_subdev_id_from_pipe(phy_pipe),
                                  addr,
                                  data);
      }
      break;





    case BF_DEV_FAMILY_UNKNOWN:
      PIPE_MGR_DBGCHK(0);
      return;
  }
}

bf_status_t pipe_mgr_unlock_device_internal(bf_dev_id_t dev_id,
                                            bf_dev_init_mode_t warm_init_mode) {
  bf_status_t status = BF_SUCCESS;
  uint32_t hdl = 0;

  LOG_TRACE("Entering %s, dev %d", __func__, dev_id);
  if (dev_id >= PIPE_MGR_NUM_DEVICES) {
    return BF_INVALID_ARG;
  }
  pipe_mgr_dev_locked[dev_id] = false;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s:%d Device info for device id %d not found",
              __func__,
              __LINE__,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (!pipe_mgr_is_device_virtual(dev_id)) {
    // Enable the DRs
    pipe_mgr_enable_all_dr(dev_info);

    status = pipe_mgr_drv_load_fm_buffers(dev_id, warm_init_mode);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR("%s: failed to setup async dma buffers", __func__);
      goto done;
    }

    /* Push data from internal session */
    pipe_mgr_internal_session_push(dev_id, true);

    /* non-default sessons also need to be pushed */
    for (hdl = 0; hdl < PIPE_MGR_MAX_SESSIONS; hdl++) {
      pipe_mgr_sess_ctx_t *ctx;
      if (!pipe_mgr_session_exists(hdl)) continue;
      ctx = pipe_mgr_get_sess_ctx(hdl, __func__, __LINE__);
      if (!ctx) continue;

      status = pipe_mgr_drv_reconfig_ilist_push(&ctx->hdl, dev_id);
      if (PIPE_SUCCESS != status) {
        LOG_ERROR("Failed to push instruction list (%d)", status);
        PIPE_MGR_DBGCHK(PIPE_SUCCESS == status);
        goto done;
      }
    }

    /* After fast reconfig, do timestamp init via register writes. */
    pipe_mgr_tstamp_init(dev_info);
  }

  pipe_mgr_init_mode_reset(dev_id);

done:
  LOG_TRACE("Exiting %s, dev %d, with status %s",
            __func__,
            dev_id,
            pipe_str_err(status));
  return status;
}

bf_status_t pipe_mgr_unlock_device_cb(bf_dev_id_t dev_id) {
  bf_subdev_id_t subdev_id = 0;  // TBD tf3-fix
  /* This is the callback registered for when the device is unlocked. This is
   * called only during fast-recfg at this point. Pipe-mgr needs to know the
   * device init mode during unlock in order to things differently b/w a
   * cold-boot/fast-recfg and hitless warm-init. But the callback only takes the
   * device ID. For hitless warm init scenarios, the unlock of the device is
   * driven by pipe-mgr itself and not DVM and for other scenarios where the
   * unlock is driven by pipe-mgr itself, the pipe_mgr_unlock_device_internal
   * function is called with the right device init mode. Possibly the callback
   * signature itself needs to take the device init mode, since other modules
   * may also be needing this?
   */
  LOG_TRACE("Entering %s, dev %d", __func__, dev_id);
  if (dev_id >= PIPE_MGR_NUM_DEVICES) {
    return BF_INVALID_ARG;
  }
  pipe_mgr_dev_locked[dev_id] = false;

  // Enable pipe manager interrupts
  bf_status_t status = bf_int_ena_pbus(dev_id, subdev_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("interrupt enable with LLD failed for device %d, sts %s (%d)",
              dev_id,
              bf_err_str(status),
              status);
    return status;
  }

  return pipe_mgr_unlock_device_internal(dev_id, BF_DEV_WARM_INIT_FAST_RECFG);
}

/* Set mem slow mode or fast mode */
static pipe_status_t set_mem_slow_mode(rmt_dev_info_t *dev_info,
                                       bool enable,
                                       bool push_ilist);

bf_status_t pipe_mgr_enable_traffic(bf_dev_id_t dev_id) {
  LOG_TRACE("Entering %s, dev %d", __func__, dev_id);
  pipe_status_t sts = PIPE_SUCCESS;
  rmt_dev_info_t *dev_info;
  pipe_sess_hdl_t shdl = pipe_mgr_get_int_sess_hdl();

  if (!(dev_info = pipe_mgr_get_dev_info(dev_id))) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_INVALID_ARG;
  }

  if (!dev_info->virtual_device) {
    /* Slow-mode configuration was already set for other Tofinos in
     * config_complete since traffic was already enabled there in port manager.
     */
    if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO) {
      /* Prepend the slow-mode configuration onto the instruction list so that
       * it is applied before any other configuration.
       * It will be pushed to hardware when the write blocks complete below. */
      sts = set_mem_slow_mode(dev_info, true, false);
      if (sts != PIPE_SUCCESS) {
        LOG_ERROR("Failed to set mem slow mode for dev %d sts %s",
                  dev_id,
                  pipe_str_err(sts));
        return sts;
      }
    }
    /* Enable meter sweeps as well. */
    pipe_mgr_meter_sweep_control(shdl, dev_info, true);

    /* Only Tofino needs to have the IPB channels enabled here.  Tofino2 has
     * already enabled them during the port configuration replay. */
    if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO) {
      sts = pipe_mgr_parde_traffic_enable(shdl, dev_info);
      if (sts != PIPE_SUCCESS) {
        LOG_ERROR("Failed to enable traffic for dev %d sts %s",
                  dev_id,
                  pipe_str_err(sts));
        pipe_mgr_drv_ilist_abort(&shdl);
        return sts;
      }
    }

    /* Append the idle time configuration onto the same instruction list.  This
     * will setup the idle time move reg configuration and also configure the
     * sweep control. */
    sts = pipe_mgr_idle_write_all_idle_ctrl_regs(shdl, dev_info);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR("Failed to setup idle config for dev %d sts %s",
                dev_id,
                pipe_str_err(sts));
      pipe_mgr_drv_ilist_abort(&shdl);
      return sts;
    }

    /* Tofino-1 will enable traffic using an ilist to turn on IBUF channels so
     * traffic will not be started until we push the ilist created just above.
     * Tofino-2 and later enable traffic by MAC programming and traffic will
     * already be flowing at this point.
     * For Tofino-1 we delay the write-block completions until now since traffic
     * is not started, but we must complete all the write-block operations here
     * before starting the ilist which enables traffic.  For Tofino-2 and later
     * the write blocks will be completed in the config_complete step. */
    if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO) {
      sts = pipe_mgr_drv_wr_blk_cmplt_all(shdl, dev_id);
    }
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Failed to complete wr blk operations (%s)", pipe_str_err(sts));
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
    } else {
      pipe_mgr_enable_interrupt_notifs(dev_info);
      pipe_mgr_set_pbus_weights(dev_info, 1 /* weight */);
      sts = pipe_mgr_drv_ilist_push(&shdl, NULL, NULL);
      if (PIPE_SUCCESS != sts) {
        LOG_ERROR(
            "%s: Failed to push channel enables and idle config for dev %d sts "
            "%s",
            __func__,
            dev_id,
            pipe_str_err(sts));
      }
    }
    if (PIPE_SUCCESS != sts) {
      pipe_mgr_drv_ilist_abort(&shdl);
    }

    if (PIPE_SUCCESS == sts) {
#ifndef DEVICE_IS_EMULATOR
      /* This is the end of fast reconfig HA or cold boot.  Start the TCAM scrub
       * timer now that we are going back to normal operation. */
      if (!pipe_mgr_is_p4_skipped(dev_info)) {
        bool is_model = false;
        bf_drv_device_type_get(dev_id, &is_model);
        if (!is_model) {
          sts = pipe_mgr_intr_start_scrub_timer(dev_id);
        }
      }

      /* Start the timer for port stuck detection */
      const int PIPE_MGR_PORT_STUCK_TIMER_DEF_VAL = 1000;
      if (!pipe_mgr_is_p4_skipped(dev_info)) {
        sts = pipe_mgr_port_stuck_detect_timer_init(
            dev_id, PIPE_MGR_PORT_STUCK_TIMER_DEF_VAL);
        if ((sts != PIPE_SUCCESS) && (sts != PIPE_NOT_SUPPORTED)) {
          LOG_ERROR(
              "%s: failed to setup timer for port stuck condition detection!",
              __func__);
        }
      }
#endif
    }
  }

  if (pipe_mgr_init_mode_fast_recfg(dev_id) ||
      pipe_mgr_init_mode_fast_recfg_quick(dev_id)) {
    pipe_mgr_exclusive_api_exit(dev_id);
  }
  LOG_TRACE("Exiting %s, dev %d, with status %s",
            __func__,
            dev_id,
            pipe_str_err(sts));
  return sts;
}

bool pipe_mgr_is_device_locked(bf_dev_id_t dev_id) {
  if (dev_id >= PIPE_MGR_NUM_DEVICES) {
    return false;
  }
  return pipe_mgr_dev_locked[dev_id];
}

bool pipe_mgr_is_any_device_locked() {
  bf_dev_id_t dev_id = 0;

  for (dev_id = 0; dev_id < PIPE_MGR_NUM_DEVICES; dev_id++) {
    if (pipe_mgr_dev_locked[dev_id]) {
      return true;
    }
  }
  return false;
}

void pipe_mgr_init_mode_set(bf_dev_id_t dev_id,
                            bf_dev_init_mode_t dev_init_mode) {
  struct pipe_mgr_dev_ctx *ctx = pipe_mgr_dev_ctx(dev_id);
  if (!ctx) {
    PIPE_MGR_ASSERT(0);
    return;
  }
  ctx->dev_init_mode = dev_init_mode;
  ctx->warm_init_in_progress =
      (dev_init_mode != BF_DEV_INIT_COLD) ? true : false;
}

void pipe_mgr_init_mode_reset(bf_dev_id_t dev_id) {
  struct pipe_mgr_dev_ctx *ctx = pipe_mgr_dev_ctx(dev_id);
  if (!ctx) {
    PIPE_MGR_ASSERT(0);
    return;
  }
  ctx->warm_init_in_progress = false;
}

bool pipe_mgr_warm_init_in_progress(bf_dev_id_t dev_id) {
  struct pipe_mgr_dev_ctx *ctx = pipe_mgr_dev_ctx(dev_id);
  if (!ctx) {
    PIPE_MGR_ASSERT(0);
    return false;
  }
  return (ctx->dev_init_mode != BF_DEV_INIT_COLD) ? ctx->warm_init_in_progress
                                                  : false;
}

bool pipe_mgr_hitless_warm_init_in_progress(bf_dev_id_t dev_id) {
  struct pipe_mgr_dev_ctx *ctx = pipe_mgr_dev_ctx(dev_id);
  if (!ctx) {
    PIPE_MGR_ASSERT(0);
    return false;
  }
  return (ctx->dev_init_mode == BF_DEV_WARM_INIT_HITLESS)
             ? ctx->warm_init_in_progress
             : false;
}

bool pipe_mgr_fast_recfg_warm_init_in_progress(bf_dev_id_t dev_id) {
  struct pipe_mgr_dev_ctx *ctx = pipe_mgr_dev_ctx(dev_id);
  if (!ctx) {
    PIPE_MGR_ASSERT(0);
    return false;
  }
  return ((ctx->dev_init_mode == BF_DEV_WARM_INIT_FAST_RECFG) ||
          (ctx->dev_init_mode == BF_DEV_WARM_INIT_FAST_RECFG_QUICK))
             ? ctx->warm_init_in_progress
             : false;
}

bool pipe_mgr_init_mode_cold_boot(bf_dev_id_t dev_id) {
  struct pipe_mgr_dev_ctx *ctx = pipe_mgr_dev_ctx(dev_id);
  if (!ctx) {
    PIPE_MGR_ASSERT(0);
    return false;
  }
  return (ctx->dev_init_mode == BF_DEV_INIT_COLD);
}

bool pipe_mgr_init_mode_hitless(bf_dev_id_t dev_id) {
  struct pipe_mgr_dev_ctx *ctx = pipe_mgr_dev_ctx(dev_id);
  if (!ctx) {
    PIPE_MGR_ASSERT(0);
    return false;
  }
  return (ctx->dev_init_mode == BF_DEV_WARM_INIT_HITLESS);
}

bool pipe_mgr_init_mode_fast_recfg(bf_dev_id_t dev_id) {
  struct pipe_mgr_dev_ctx *ctx = pipe_mgr_dev_ctx(dev_id);
  if (!ctx) {
    PIPE_MGR_ASSERT(0);
    return false;
  }
  return (ctx->dev_init_mode == BF_DEV_WARM_INIT_FAST_RECFG);
}

bool pipe_mgr_init_mode_fast_recfg_quick(bf_dev_id_t dev_id) {
  struct pipe_mgr_dev_ctx *ctx = pipe_mgr_dev_ctx(dev_id);
  if (!ctx) {
    PIPE_MGR_ASSERT(0);
    return false;
  }
  return (ctx->dev_init_mode == BF_DEV_WARM_INIT_FAST_RECFG_QUICK);
}

static pipe_status_t tof_set_mem_slow_mode(pipe_sess_hdl_t shdl,
                                           rmt_dev_info_t *dev_info,
                                           pipe_bitmap_t *pipe_bmp,
                                           bool enable) {
  bf_dev_id_t dev_id = dev_info->dev_id;
  pipe_status_t status = PIPE_SUCCESS;
  /* Set up the data to be written to either enable or disable slow-mode. */
  uint32_t adr_dist_data = 0, adrmux_data = 0;
  uint32_t mau_cfg_data = 0, imem_data = 0;
  if (enable) {
    setp_adr_dist_mem_slow_mode_adr_dist_mem_slow_mode(&adr_dist_data, 1);
    setp_adrmux_row_mem_slow_mode_adrmux_row_mem_slow_mode(&adrmux_data, 1);
    setp_mau_cfg_mem_slow_mode_mau_cfg_mem_slow_mode(&mau_cfg_data, 1);
    setp_imem_parity_ctl_imem_slow_mode(&imem_data, 1);
    setp_imem_parity_ctl_imem_parity_check_enable(&imem_data, 1);
  } else {
    setp_adr_dist_mem_slow_mode_adr_dist_mem_slow_mode(&adr_dist_data, 0);
    setp_adrmux_row_mem_slow_mode_adrmux_row_mem_slow_mode(&adrmux_data, 0);
    setp_mau_cfg_mem_slow_mode_mau_cfg_mem_slow_mode(&mau_cfg_data, 0);
    setp_imem_parity_ctl_imem_slow_mode(&imem_data, 0);
  }

  /* Prepare a noop instruction for later use in the loop below. */
  pipe_noop_instr_t noop_instr;
  construct_instr_noop(dev_id, &noop_instr);

  /* Loop over all stages setting the mode in each. */
  pipe_instr_write_reg_t instr;
  dev_stage_t stage = 0;
  for (stage = 0; stage < dev_info->num_active_mau; stage++) {
    /* Need to make sure no operations are outstanding in an MAU before changing
     * the mode.  This can be done by issuing 32 NOPs to each stage before the
     * mode is changed.  However, we already issue 32 NOPs whenever we change
     * the stage so this may not be required but it is safer to leave here
     * incase we change the logic that issues 32 NOPs when changing stages. */
    int i;
    for (i = 0; i < 32; ++i) {
      status = pipe_mgr_drv_ilist_add(&shdl,
                                      dev_info,
                                      pipe_bmp,
                                      stage,
                                      (uint8_t *)&noop_instr,
                                      sizeof noop_instr);
      if (PIPE_SUCCESS != status) {
        LOG_ERROR(
            "Failed to post pre-%s-slow-mode instruction (%s) dev %d stage %d",
            enable ? "en" : "dis",
            pipe_str_err(status),
            dev_info->dev_id,
            stage);
        return status;
      }
    }

    /* Set adr_dist_mem_slow_mode */
    uint32_t address = 0;
    address = offsetof(
        Tofino, pipes[0].mau[stage].rams.match.adrdist.adr_dist_mem_slow_mode);
    construct_instr_reg_write(dev_id, &instr, address, adr_dist_data);
    status = pipe_mgr_drv_ilist_add(
        &shdl, dev_info, pipe_bmp, stage, (uint8_t *)&instr, sizeof instr);
    if (PIPE_SUCCESS != status) {
      LOG_ERROR(
          "Failed to post adr-dist-%s-slow-mode instruction (%s) dev %d stage "
          "%d",
          enable ? "en" : "dis",
          pipe_str_err(status),
          dev_info->dev_id,
          stage);
      return status;
    }

    /* Set adrmux_row_mem_slow_mode */
    int row = 0;
    for (row = 0; row < dev_info->dev_cfg.stage_cfg.num_sram_rows; row++) {
      address = offsetof(Tofino,
                         pipes[0]
                             .mau[stage]
                             .rams.map_alu.row[row]
                             .adrmux.adrmux_row_mem_slow_mode);
      construct_instr_reg_write(dev_id, &instr, address, adrmux_data);
      status = pipe_mgr_drv_ilist_add(
          &shdl, dev_info, pipe_bmp, stage, (uint8_t *)&instr, sizeof instr);
      if (PIPE_SUCCESS != status) {
        LOG_ERROR(
            "Failed to post row-%d-%s-slow-mode instruction (%s) dev %d stage "
            "%d",
            row,
            enable ? "en" : "dis",
            pipe_str_err(status),
            dev_info->dev_id,
            stage);
        return status;
      }
    }
    /* Set mau_cfg_mem_slow_mode */
    address =
        offsetof(Tofino, pipes[0].mau[stage].cfg_regs.mau_cfg_mem_slow_mode);
    construct_instr_reg_write(dev_id, &instr, address, mau_cfg_data);
    status = pipe_mgr_drv_ilist_add(
        &shdl, dev_info, pipe_bmp, stage, (uint8_t *)&instr, sizeof instr);
    if (PIPE_SUCCESS != status) {
      LOG_ERROR(
          "Failed to post mau-cfg-%s-slow-mode instruction (%s) dev %d stage "
          "%d",
          enable ? "en" : "dis",
          pipe_str_err(status),
          dev_info->dev_id,
          stage);
      return status;
    }

    /* Set imem_slow_mode */
    address = offsetof(Tofino, pipes[0].mau[stage].dp.imem_parity_ctl);
    construct_instr_reg_write(dev_id, &instr, address, imem_data);
    status = pipe_mgr_drv_ilist_add(
        &shdl, dev_info, pipe_bmp, stage, (uint8_t *)&instr, sizeof instr);
    if (PIPE_SUCCESS != status) {
      LOG_ERROR(
          "Failed to post imem-%s-slow-mode instruction (%s) dev %d stage %d",
          enable ? "en" : "dis",
          pipe_str_err(status),
          dev_info->dev_id,
          stage);
      return status;
    }

    /* Add another 32 NOPs after changing the mode in this stage to prevent any
     * future accesses to the MAU from seeing the old mode. */
    for (i = 0; i < 32; ++i) {
      status = pipe_mgr_drv_ilist_add(&shdl,
                                      dev_info,
                                      pipe_bmp,
                                      stage,
                                      (uint8_t *)&noop_instr,
                                      sizeof noop_instr);
      if (PIPE_SUCCESS != status) {
        LOG_ERROR(
            "Failed to post post-%s-slow-mode instruction (%s) dev %d stage %d",
            enable ? "en" : "dis",
            pipe_str_err(status),
            dev_info->dev_id,
            stage);
        return status;
      }
    }
  }
  return PIPE_SUCCESS;
}

static pipe_status_t tof3_set_mem_slow_mode(pipe_sess_hdl_t shdl,
                                            rmt_dev_info_t *dev_info,
                                            pipe_bitmap_t *pipe_bmp,
                                            bool enable) {
  bf_dev_id_t dev_id = dev_info->dev_id;
  pipe_status_t status = PIPE_SUCCESS;
  /* Set up the data to be written to either enable or disable slow-mode. */
  uint32_t adr_dist_data = 0, adrmux_data = 0;
  uint32_t mau_cfg_data = 0, imem_data = 0;
  bf_dev_pipe_t pipe_id;
  if (enable) {
    setp_tof3_adr_dist_mem_slow_mode_adr_dist_mem_slow_mode(&adr_dist_data, 1);
    setp_tof3_adrmux_row_mem_slow_mode_adrmux_row_mem_slow_mode(&adrmux_data,
                                                                1);
    setp_tof3_mau_cfg_mem_slow_mode_mau_cfg_mem_slow_mode(&mau_cfg_data, 1);
    setp_tof3_imem_parity_ctl_imem_slow_mode(&imem_data, 1);
  } else {
    setp_tof3_adr_dist_mem_slow_mode_adr_dist_mem_slow_mode(&adr_dist_data, 0);
    setp_tof3_adrmux_row_mem_slow_mode_adrmux_row_mem_slow_mode(&adrmux_data,
                                                                0);
    setp_tof3_mau_cfg_mem_slow_mode_mau_cfg_mem_slow_mode(&mau_cfg_data, 0);
    setp_tof3_imem_parity_ctl_imem_slow_mode(&imem_data, 0);
  }

  /* Prepare a noop instruction for later use in the loop below. */
  pipe_noop_instr_t noop_instr;
  construct_instr_noop(dev_id, &noop_instr);

  /* Loop over all stages setting the mode in each. */
  pipe_instr_write_reg_t instr;
  dev_stage_t stage = 0;
  pipe_bitmap_t pipes_tmp;
  PIPE_BITMAP_ITER(pipe_bmp, pipe_id) {
    PIPE_BITMAP_INIT(&pipes_tmp, dev_info->num_active_pipes);
    PIPE_BITMAP_SET(&pipes_tmp, pipe_id);
    for (stage = 0; stage < dev_info->num_active_mau; stage++) {
      /* Need to make sure no operations are outstanding in an MAU before
       * changing
       * the mode.  This can be done by issuing 32 NOPs to each stage before the
       * mode is changed.  However, we already issue 32 NOPs whenever we change
       * the stage so this may not be required but it is safer to leave here
       * incase we change the logic that issues 32 NOPs when changing stages. */
      int i;
      for (i = 0; i < 32; ++i) {
        status = pipe_mgr_drv_ilist_add(&shdl,
                                        dev_info,
                                        &pipes_tmp,
                                        stage,
                                        (uint8_t *)&noop_instr,
                                        sizeof noop_instr);
        if (PIPE_SUCCESS != status) {
          LOG_ERROR(
              "Failed to post pre-%s-slow-mode instruction (%s) dev %d stage "
              "%d",
              enable ? "en" : "dis",
              pipe_str_err(status),
              dev_info->dev_id,
              stage);
          return status;
        }
      }

      /* Set adr_dist_mem_slow_mode */
      uint32_t address = 0;
      address = offsetof(
          tof3_reg,
          pipes[0].mau[stage].rams.match.adrdist.adr_dist_mem_slow_mode);
      construct_instr_reg_write(dev_id, &instr, address, adr_dist_data);
      status = pipe_mgr_drv_ilist_add(
          &shdl, dev_info, &pipes_tmp, stage, (uint8_t *)&instr, sizeof instr);
      if (PIPE_SUCCESS != status) {
        LOG_ERROR(
            "Failed to post adr-dist-%s-slow-mode instruction (%s) dev %d "
            "stage "
            "%d",
            enable ? "en" : "dis",
            pipe_str_err(status),
            dev_info->dev_id,
            stage);
        return status;
      }

      /* Set adrmux_row_mem_slow_mode */
      int row = 0;
      for (row = 0; row < dev_info->dev_cfg.stage_cfg.num_sram_rows; row++) {
        address = offsetof(tof3_reg,
                           pipes[0]
                               .mau[stage]
                               .rams.map_alu.row[row]
                               .adrmux.adrmux_row_mem_slow_mode);
        construct_instr_reg_write(dev_id, &instr, address, adrmux_data);
        status = pipe_mgr_drv_ilist_add(&shdl,
                                        dev_info,
                                        &pipes_tmp,
                                        stage,
                                        (uint8_t *)&instr,
                                        sizeof instr);
        if (PIPE_SUCCESS != status) {
          LOG_ERROR(
              "Failed to post row-%d-%s-slow-mode instruction (%s) dev %d "
              "stage "
              "%d",
              row,
              enable ? "en" : "dis",
              pipe_str_err(status),
              dev_info->dev_id,
              stage);
          return status;
        }
      }
      /* Set mau_cfg_mem_slow_mode */
      address = offsetof(tof3_reg,
                         pipes[0].mau[stage].cfg_regs.mau_cfg_mem_slow_mode);
      construct_instr_reg_write(dev_id, &instr, address, mau_cfg_data);
      status = pipe_mgr_drv_ilist_add(
          &shdl, dev_info, &pipes_tmp, stage, (uint8_t *)&instr, sizeof instr);
      if (PIPE_SUCCESS != status) {
        LOG_ERROR(
            "Failed to post mau-cfg-%s-slow-mode instruction (%s) dev %d stage "
            "%d",
            enable ? "en" : "dis",
            pipe_str_err(status),
            dev_info->dev_id,
            stage);
        return status;
      }

      /* Set imem_slow_mode */
      address = offsetof(tof3_reg, pipes[0].mau[stage].dp.imem_parity_ctl);
      construct_instr_reg_write(dev_id, &instr, address, imem_data);
      status = pipe_mgr_drv_ilist_add(
          &shdl, dev_info, &pipes_tmp, stage, (uint8_t *)&instr, sizeof instr);
      if (PIPE_SUCCESS != status) {
        LOG_ERROR(
            "Failed to post imem-%s-slow-mode instruction (%s) dev %d stage %d",
            enable ? "en" : "dis",
            pipe_str_err(status),
            dev_info->dev_id,
            stage);
        return status;
      }

      /* Add another 32 NOPs after changing the mode in this stage to prevent
       * any
       * future accesses to the MAU from seeing the old mode. */
      for (i = 0; i < 32; ++i) {
        status = pipe_mgr_drv_ilist_add(&shdl,
                                        dev_info,
                                        &pipes_tmp,
                                        stage,
                                        (uint8_t *)&noop_instr,
                                        sizeof noop_instr);
        if (PIPE_SUCCESS != status) {
          LOG_ERROR(
              "Failed to post post-%s-slow-mode instruction (%s) dev %d stage "
              "%d",
              enable ? "en" : "dis",
              pipe_str_err(status),
              dev_info->dev_id,
              stage);
          return status;
        }
      }
    }
  }
  return PIPE_SUCCESS;
}

static pipe_status_t tof2_set_mem_slow_mode(pipe_sess_hdl_t shdl,
                                            rmt_dev_info_t *dev_info,
                                            pipe_bitmap_t *pipe_bmp,
                                            bool enable) {
  bf_dev_id_t dev_id = dev_info->dev_id;
  pipe_status_t status = PIPE_SUCCESS;
  /* Set up the data to be written to either enable or disable slow-mode. */
  uint32_t adr_dist_data = 0, adrmux_data = 0;
  uint32_t mau_cfg_data = 0, imem_data = 0;
  bf_dev_pipe_t pipe_id;
  if (enable) {
    setp_tof2_adr_dist_mem_slow_mode_adr_dist_mem_slow_mode(&adr_dist_data, 1);
    setp_tof2_adrmux_row_mem_slow_mode_adrmux_row_mem_slow_mode(&adrmux_data,
                                                                1);
    setp_tof2_mau_cfg_mem_slow_mode_mau_cfg_mem_slow_mode(&mau_cfg_data, 1);
    setp_tof2_imem_parity_ctl_imem_slow_mode(&imem_data, 1);
  } else {
    setp_tof2_adr_dist_mem_slow_mode_adr_dist_mem_slow_mode(&adr_dist_data, 0);
    setp_tof2_adrmux_row_mem_slow_mode_adrmux_row_mem_slow_mode(&adrmux_data,
                                                                0);
    setp_tof2_mau_cfg_mem_slow_mode_mau_cfg_mem_slow_mode(&mau_cfg_data, 0);
    setp_tof2_imem_parity_ctl_imem_slow_mode(&imem_data, 0);
  }

  /* Prepare a noop instruction for later use in the loop below. */
  pipe_noop_instr_t noop_instr;
  construct_instr_noop(dev_id, &noop_instr);

  /* Loop over all stages setting the mode in each. */
  pipe_instr_write_reg_t instr;
  dev_stage_t stage = 0;
  pipe_bitmap_t pipes_tmp;
  PIPE_BITMAP_ITER(pipe_bmp, pipe_id) {
    PIPE_BITMAP_INIT(&pipes_tmp, dev_info->num_active_pipes);
    PIPE_BITMAP_SET(&pipes_tmp, pipe_id);
    for (stage = 0; stage < dev_info->num_active_mau; stage++) {
      /* Need to make sure no operations are outstanding in an MAU before
       * changing the mode.  This can be done by issuing 32 NOPs to each stage
       * before the mode is changed.  However, we already issue 32 NOPs whenever
       * we change the stage so this may not be required but it is safer to
       * leave here incase we change the logic that issues 32 NOPs when changing
       * stages. */
      int i;
      for (i = 0; i < 32; ++i) {
        status = pipe_mgr_drv_ilist_add(&shdl,
                                        dev_info,
                                        &pipes_tmp,
                                        stage,
                                        (uint8_t *)&noop_instr,
                                        sizeof noop_instr);
        if (PIPE_SUCCESS != status) {
          LOG_ERROR(
              "Failed to post pre-%s-slow-mode instruction (%s) dev %d stage "
              "%d",
              enable ? "en" : "dis",
              pipe_str_err(status),
              dev_info->dev_id,
              stage);
          return status;
        }
      }

      /* Set adr_dist_mem_slow_mode */
      uint32_t address = 0;
      address = offsetof(
          tof2_reg,
          pipes[0].mau[stage].rams.match.adrdist.adr_dist_mem_slow_mode);
      construct_instr_reg_write(dev_id, &instr, address, adr_dist_data);
      status = pipe_mgr_drv_ilist_add(
          &shdl, dev_info, &pipes_tmp, stage, (uint8_t *)&instr, sizeof instr);
      if (PIPE_SUCCESS != status) {
        LOG_ERROR(
            "Failed to post adr-dist-%s-slow-mode instruction (%s) dev %d "
            "stage "
            "%d",
            enable ? "en" : "dis",
            pipe_str_err(status),
            dev_info->dev_id,
            stage);
        return status;
      }

      /* Set adrmux_row_mem_slow_mode */
      int row = 0;
      for (row = 0; row < dev_info->dev_cfg.stage_cfg.num_sram_rows; row++) {
        address = offsetof(tof2_reg,
                           pipes[0]
                               .mau[stage]
                               .rams.map_alu.row[row]
                               .adrmux.adrmux_row_mem_slow_mode);
        construct_instr_reg_write(dev_id, &instr, address, adrmux_data);
        status = pipe_mgr_drv_ilist_add(&shdl,
                                        dev_info,
                                        &pipes_tmp,
                                        stage,
                                        (uint8_t *)&instr,
                                        sizeof instr);
        if (PIPE_SUCCESS != status) {
          LOG_ERROR(
              "Failed to post row-%d-%s-slow-mode instruction (%s) dev %d "
              "stage "
              "%d",
              row,
              enable ? "en" : "dis",
              pipe_str_err(status),
              dev_info->dev_id,
              stage);
          return status;
        }
      }
      /* Set mau_cfg_mem_slow_mode */
      address = offsetof(tof2_reg,
                         pipes[0].mau[stage].cfg_regs.mau_cfg_mem_slow_mode);
      construct_instr_reg_write(dev_id, &instr, address, mau_cfg_data);
      status = pipe_mgr_drv_ilist_add(
          &shdl, dev_info, &pipes_tmp, stage, (uint8_t *)&instr, sizeof instr);
      if (PIPE_SUCCESS != status) {
        LOG_ERROR(
            "Failed to post mau-cfg-%s-slow-mode instruction (%s) dev %d stage "
            "%d",
            enable ? "en" : "dis",
            pipe_str_err(status),
            dev_info->dev_id,
            stage);
        return status;
      }

      /* Set imem_slow_mode */
      address = offsetof(tof2_reg, pipes[0].mau[stage].dp.imem_parity_ctl);
      construct_instr_reg_write(dev_id, &instr, address, imem_data);
      status = pipe_mgr_drv_ilist_add(
          &shdl, dev_info, &pipes_tmp, stage, (uint8_t *)&instr, sizeof instr);
      if (PIPE_SUCCESS != status) {
        LOG_ERROR(
            "Failed to post imem-%s-slow-mode instruction (%s) dev %d stage %d",
            enable ? "en" : "dis",
            pipe_str_err(status),
            dev_info->dev_id,
            stage);
        return status;
      }

      /* Add another 32 NOPs after changing the mode in this stage to prevent
       * any future accesses to the MAU from seeing the old mode. */
      for (i = 0; i < 32; ++i) {
        status = pipe_mgr_drv_ilist_add(&shdl,
                                        dev_info,
                                        &pipes_tmp,
                                        stage,
                                        (uint8_t *)&noop_instr,
                                        sizeof noop_instr);
        if (PIPE_SUCCESS != status) {
          LOG_ERROR(
              "Failed to post post-%s-slow-mode instruction (%s) dev %d stage "
              "%d",
              enable ? "en" : "dis",
              pipe_str_err(status),
              dev_info->dev_id,
              stage);
          return status;
        }
      }
    }
  }
  return PIPE_SUCCESS;
}

/* Set mem slow mode or fast mode */
static pipe_status_t set_mem_slow_mode(rmt_dev_info_t *dev_info,
                                       bool enable,
                                       bool push_ilist) {
  pipe_status_t status = PIPE_SUCCESS;

  /* Get a bitmap of all pipes to write to. */
  pipe_bitmap_t pipe_bmp = {{0}};
  PIPE_BITMAP_INIT(&pipe_bmp, PIPE_BMP_SIZE);
  uint32_t pipe;
  for (pipe = 0; pipe < dev_info->num_active_pipes; ++pipe) {
    PIPE_BITMAP_SET(&pipe_bmp, pipe);
  }

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      status = tof_set_mem_slow_mode(shdl, dev_info, &pipe_bmp, enable);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      status = tof2_set_mem_slow_mode(shdl, dev_info, &pipe_bmp, enable);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      status = tof3_set_mem_slow_mode(shdl, dev_info, &pipe_bmp, enable);
      break;






    default:
      status = PIPE_UNEXPECTED;
      PIPE_MGR_DBGCHK(0);
  }

  if (PIPE_SUCCESS != status) {
    LOG_ERROR("Failed to prepare mem-slow-mode %s on dev %d, status %s",
              enable ? "enable" : "disable",
              dev_info->dev_id,
              pipe_str_err(status));
    pipe_mgr_drv_ilist_abort(&shdl);
    PIPE_MGR_DBGCHK(PIPE_SUCCESS == status);
  } else if (push_ilist) {
    /* Push the ilist to HW. */
    status = pipe_mgr_drv_ilist_push(&shdl, NULL, NULL);
    if (PIPE_SUCCESS != status) {
      LOG_ERROR("Failed to %s mem-slow-mode on dev %d, status %s",
                enable ? "enable" : "disable",
                dev_info->dev_id,
                pipe_str_err(status));
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == status);
    }
  }

  return status;
}

/* Set mem slow mode or fast mode */
pipe_status_t pipe_mgr_set_mem_slow_mode(rmt_dev_info_t *dev_info,
                                         bool enable) {
  return set_mem_slow_mode(dev_info, enable, true);
}

void resource_trace(bf_dev_id_t dev,
                    pipe_res_spec_t *rspecs,
                    int rsc_count,
                    char *buf,
                    size_t buf_size) {
  char *ptr, *end;
  int rsc;
  ptr = buf;
  end = buf + buf_size;
  if (!buf || buf_size == 0) return;
  buf[0] = '\0';

  for (rsc = 0; rsc < rsc_count; ++rsc) {
    pipe_res_spec_t *rspec = &rspecs[rsc];
    if (rspec->tag != PIPE_RES_ACTION_TAG_ATTACHED) {
      continue;
    }
    switch (PIPE_GET_HDL_TYPE(rspec->tbl_hdl)) {
      case PIPE_HDL_TYPE_STAT_TBL:
        if (rspec->tag == PIPE_RES_ACTION_TAG_ATTACHED) {
          if (pipe_mgr_stat_tbl_is_indirect(dev, rspec->tbl_hdl)) {
            ptr += snprintf(ptr,
                            end > ptr ? (end - ptr) : 0,
                            "Entry references stat tbl 0x%x at idx %d",
                            rspec->tbl_hdl,
                            rspec->tbl_idx);
          } else {
            ptr += snprintf(ptr,
                            end > ptr ? (end - ptr) : 0,
                            "Entry references direct stat tbl 0x%x",
                            rspec->tbl_hdl);
          }
        } else if (rspec->tag == PIPE_RES_ACTION_TAG_DETACHED) {
          ptr += snprintf(ptr,
                          end > ptr ? (end - ptr) : 0,
                          "DETACH stat tbl 0x%x",
                          rspec->tbl_hdl);
        } else if (rspec->tag == PIPE_RES_ACTION_TAG_NO_CHANGE) {
          ptr += snprintf(ptr,
                          end > ptr ? (end - ptr) : 0,
                          "NO-CHANGE stat tbl 0x%x",
                          rspec->tbl_hdl);
        }
        break;
      case PIPE_HDL_TYPE_METER_TBL:
        if (rspec->tag == PIPE_RES_ACTION_TAG_ATTACHED) {
          if (pipe_mgr_meter_tbl_is_indirect(dev, rspec->tbl_hdl)) {
            ptr += snprintf(ptr,
                            end > ptr ? (end - ptr) : 0,
                            "Entry references meter tbl 0x%x at idx %d",
                            rspec->tbl_hdl,
                            rspec->tbl_idx);
          } else {
            ptr += snprintf(ptr,
                            end > ptr ? (end - ptr) : 0,
                            "Entry references direct meter tbl 0x%x",
                            rspec->tbl_hdl);
          }
        } else if (rspec->tag == PIPE_RES_ACTION_TAG_DETACHED) {
          ptr += snprintf(ptr,
                          end > ptr ? (end - ptr) : 0,
                          "DETACH meter tbl 0x%x",
                          rspec->tbl_hdl);
        } else if (rspec->tag == PIPE_RES_ACTION_TAG_NO_CHANGE) {
          ptr += snprintf(ptr,
                          end > ptr ? (end - ptr) : 0,
                          "NO-CHANGE meter tbl 0x%x",
                          rspec->tbl_hdl);
        }
        break;
      case PIPE_HDL_TYPE_STFUL_TBL:
        if (rspec->tag == PIPE_RES_ACTION_TAG_ATTACHED) {
          if (pipe_mgr_stful_tbl_is_direct(dev, rspec->tbl_hdl)) {
            bool dual = false;
            uint32_t hi = 0, lo = 0;
            pipe_mgr_stful_spec_decode(
                dev, rspec->tbl_hdl, rspec->data.stful, &dual, &hi, &lo);
            if (dual) {
              ptr += snprintf(
                  ptr,
                  end > ptr ? (end - ptr) : 0,
                  "Entry references stful tbl 0x%x with spec 0x%x:0x%x",
                  rspec->tbl_hdl,
                  hi,
                  lo);
            } else {
              ptr += snprintf(ptr,
                              end > ptr ? (end - ptr) : 0,
                              "Entry references stful tbl 0x%x with spec 0x%x",
                              rspec->tbl_hdl,
                              lo);
            }
          } else {
            ptr += snprintf(ptr,
                            end > ptr ? (end - ptr) : 0,
                            "Entry references stful tbl 0x%x with index 0x%x",
                            rspec->tbl_hdl,
                            rspec->tbl_idx);
          }
        } else if (rspec->tag == PIPE_RES_ACTION_TAG_DETACHED) {
          ptr += snprintf(ptr,
                          end > ptr ? (end - ptr) : 0,
                          "DETACH stful tbl 0x%x",
                          rspec->tbl_hdl);
        } else if (rspec->tag == PIPE_RES_ACTION_TAG_NO_CHANGE) {
          ptr += snprintf(ptr,
                          end > ptr ? (end - ptr) : 0,
                          "NO-CHANGE stful tbl 0x%x",
                          rspec->tbl_hdl);
        }
        break;
      default:
        break;
    }
  }
}
/* Retrieve pipe_id to be used for access from hardware. */
pipe_status_t pipe_mgr_get_pipe_id(pipe_bitmap_t *pipe_bmp,
                                   bf_dev_pipe_t dev_pipe_id,
                                   bf_dev_pipe_t default_pipe_id,
                                   bf_dev_pipe_t *pipe_id) {
  if (dev_pipe_id == BF_DEV_PIPE_ALL) {
    *pipe_id = default_pipe_id;
    return PIPE_SUCCESS;
  }
  if (!PIPE_BITMAP_GET(pipe_bmp, dev_pipe_id)) {
    LOG_TRACE("%s:%d Invalid request to access pipe %x",
              __func__,
              __LINE__,
              dev_pipe_id);
    return PIPE_INVALID_ARG;
  }
  *pipe_id = dev_pipe_id;
  return PIPE_SUCCESS;
}
