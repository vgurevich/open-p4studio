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
 * @file pipe_mgr_tof2_mau_snapshot.c
 * @date
 *
 * Implementation of MAU snapshot
 */

#include "pipe_mgr_int.h"
#include "pipe_mgr_sm.h"
#include "pipe_mgr_tbl.h"
#include "pipe_mgr_tof2_mau_snapshot.h"
#include "pipe_mgr_drv_intf.h"
#include <lld/bf_dma_if.h>
#include <lld/lld_reg_if.h>
#include <lld/lld_dr_if.h>
#include <tofino_regs/tofino.h>
#include <tofino/pdfixed/pd_common.h>

/* Translate the phv idx of match phv numbering(0-223) to the phv idx of capture
 * phv number(0-279) */
#define PIPE_MGR_TOF2_MATCH2CAP_IDX_TRANSLATOR(idx) \
  ((idx / 16) * 20 + (idx % 16))
/* Wrapper API for reg write */
static inline pipe_status_t pipe_mgr_snapshot_write_register(
    bf_dev_id_t dev,
    bf_subdev_id_t subdev,
    uint32_t reg_addr,
    uint32_t reg_data) {
  return pipe_mgr_write_register(dev, subdev, reg_addr, reg_data);
}

/* Wrapper API for reg read */
static inline int pipe_mgr_snapshot_read_register(bf_dev_id_t dev,
                                                  uint32_t reg_addr,
                                                  uint32_t *reg_data) {
  *reg_data = 0;
  return lld_subdev_read_register(dev, 0, reg_addr, reg_data);
}

pipe_status_t pipe_mgr_snapshot_cfg_set_tof2(rmt_dev_info_t *dev_info,
                                             bf_dev_pipe_t pipe,
                                             dev_stage_t stage,
                                             bool timer_ig_enable,
                                             bool timer_eg_enable,
                                             bf_snapshot_ig_mode_t mode) {
  uint32_t address = 0, data = 0;
  bf_dev_pipe_t start_pipe = 0, pipe_count = 0, pipe_idx = 0;
  bf_dev_pipe_t phy_pipe = 0;
  bf_dev_id_t dev = dev_info->dev_id;

  if (pipe == BF_DEV_PIPE_ALL) {
    start_pipe = 0;
    pipe_count = dev_info->num_active_pipes;
  } else {
    start_pipe = pipe;
    pipe_count = 1;
  }

  for (pipe_idx = start_pipe; pipe_idx < (start_pipe + pipe_count);
       pipe_idx++) {
    /* Get physical pipe from logical pipe */
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe_idx, &phy_pipe);

    address = offsetof(
        tof2_reg,
        pipes[phy_pipe].mau[stage].dp.snapshot_ctl.mau_snapshot_config);

    pipe_status_t status = pipe_mgr_snapshot_read_register(dev, address, &data);
    if (status != PIPE_SUCCESS) {
      return status;
    }

    setp_tof2_mau_snapshot_config_timebased_snapshot_ingress_enable(
        &data, timer_ig_enable);
    setp_tof2_mau_snapshot_config_timebased_snapshot_egress_enable(
        &data, timer_eg_enable);

    /* Zero out ingress mode bits */
    data &= 0x7;
    switch (mode) {
      case BF_SNAPSHOT_IGM_INGRESS:
        setp_tof2_mau_snapshot_config_snapshot_match_ghost_disable(&data, 1);
        break;
      case BF_SNAPSHOT_IGM_GHOST:
        setp_tof2_mau_snapshot_config_snapshot_match_ingress_disable(&data, 1);
        break;
      case BF_SNAPSHOT_IGM_ANY:
        /* All bits 0, nothing to do. */
        break;
      case BF_SNAPSHOT_IGM_BOTH:
        setp_tof2_mau_snapshot_config_snapshot_match_require_ingress_ghost(
            &data, 1);
        break;
      default:
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
    }

    status = pipe_mgr_snapshot_write_register(dev, 0, address, data);
    if (status != PIPE_SUCCESS) {
      return status;
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_snapshot_cfg_get_tof2(rmt_dev_info_t *dev_info,
                                             bf_dev_pipe_t pipe,
                                             dev_stage_t stage,
                                             bool *timer_ig_enable,
                                             bool *timer_eg_enable,
                                             bf_snapshot_ig_mode_t *mode) {
  uint32_t address = 0, data = 0;
  bf_dev_pipe_t phy_pipe = 0;
  bf_dev_id_t dev = dev_info->dev_id;

  if (pipe == BF_DEV_PIPE_ALL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  /* Get physical pipe from logical pipe */
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  address = offsetof(
      tof2_reg, pipes[phy_pipe].mau[stage].dp.snapshot_ctl.mau_snapshot_config);

  pipe_mgr_snapshot_read_register(dev, address, &data);
  *timer_ig_enable =
      getp_tof2_mau_snapshot_config_timebased_snapshot_ingress_enable(&data);
  *timer_eg_enable =
      getp_tof2_mau_snapshot_config_timebased_snapshot_egress_enable(&data);

  if (getp_tof2_mau_snapshot_config_snapshot_match_ghost_disable(&data)) {
    *mode = BF_SNAPSHOT_IGM_INGRESS;
  } else if (getp_tof2_mau_snapshot_config_snapshot_match_ingress_disable(
                 &data)) {
    *mode = BF_SNAPSHOT_IGM_GHOST;
  } else if (getp_tof2_mau_snapshot_config_snapshot_match_require_ingress_ghost(
                 &data)) {
    *mode = BF_SNAPSHOT_IGM_BOTH;
  } else {
    /* All zero - any thread will trigger snapshot */
    *mode = BF_SNAPSHOT_IGM_ANY;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_snapshot_timer_set_tof2(rmt_dev_info_t *dev_info,
                                               bf_dev_pipe_t pipe,
                                               dev_stage_t stage,
                                               uint64_t clocks) {
  bf_dev_id_t dev = dev_info->dev_id;
  uint32_t address = 0, data = 0;
  bf_dev_pipe_t start_pipe = 0, pipe_count = 0, pipe_idx = 0;
  bf_dev_pipe_t phy_pipe = 0;

  if (pipe == BF_DEV_PIPE_ALL) {
    start_pipe = 0;
    pipe_count = dev_info->num_active_pipes;
  } else {
    start_pipe = pipe;
    pipe_count = 1;
  }

  for (pipe_idx = start_pipe; pipe_idx < (start_pipe + pipe_count);
       pipe_idx++) {
    /* Get physical pipe from logical pipe */
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe_idx, &phy_pipe);

    /* Reset the time counter first */
    address = offsetof(
        tof2_reg,
        pipes[phy_pipe].mau[stage].dp.snapshot_ctl.mau_snapshot_timestamp_lo);
    pipe_mgr_snapshot_write_register(dev, 0, address, 0);

    address = offsetof(
        tof2_reg,
        pipes[phy_pipe].mau[stage].dp.snapshot_ctl.mau_snapshot_timestamp_hi);
    pipe_mgr_snapshot_write_register(dev, 0, address, 0);

    /* Write to lo */
    address = offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .mau[stage]
                           .dp.snapshot_ctl.mau_snapshot_timestamp_trigger_lo);
    data = clocks & 0xffffffff;
    pipe_mgr_snapshot_write_register(dev, 0, address, data);

    /* Write to hi */
    address = offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .mau[stage]
                           .dp.snapshot_ctl.mau_snapshot_timestamp_trigger_hi);
    data = clocks >> 32;
    pipe_mgr_snapshot_write_register(dev, 0, address, data);
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_snapshot_timer_get_tof2(rmt_dev_info_t *dev_info,
                                               bf_dev_pipe_t pipe,
                                               dev_stage_t stage,
                                               uint64_t *clocks_now,
                                               uint64_t *clocks_trig) {
  bf_dev_id_t dev = dev_info->dev_id;
  uint32_t address = 0, data_lo = 0, data_hi = 0;
  bf_dev_pipe_t phy_pipe = 0;

  if (pipe == BF_DEV_PIPE_ALL) {
    return PIPE_INVALID_ARG;
  }

  /* Get physical pipe from logical pipe */
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  /* Read the time counter first */
  address = offsetof(
      tof2_reg,
      pipes[phy_pipe].mau[stage].dp.snapshot_ctl.mau_snapshot_timestamp_lo);
  pipe_mgr_snapshot_read_register(dev, address, &data_lo);

  address = offsetof(
      tof2_reg,
      pipes[phy_pipe].mau[stage].dp.snapshot_ctl.mau_snapshot_timestamp_hi);
  pipe_mgr_snapshot_read_register(dev, address, &data_hi);
  *clocks_now = data_hi;
  *clocks_now <<= 32;
  *clocks_now |= data_lo;

  /* Read the trigger next */
  address = offsetof(tof2_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .dp.snapshot_ctl.mau_snapshot_timestamp_trigger_lo);
  pipe_mgr_snapshot_read_register(dev, address, &data_lo);

  address = offsetof(tof2_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .dp.snapshot_ctl.mau_snapshot_timestamp_trigger_hi);
  pipe_mgr_snapshot_read_register(dev, address, &data_hi);
  *clocks_trig = data_hi;
  *clocks_trig <<= 32;
  *clocks_trig |= data_lo;

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_snapshot_capture_trigger_set_tof2(
    rmt_dev_info_t *dev_info,
    bf_dev_pipe_t pipe_idx,
    dev_stage_t stage,
    pipe_mgr_phv_spec_t *phv_spec) {
  bf_dev_id_t dev = dev_info->dev_id;
  bf_dev_pipe_t phy_pipe = 0;
  uint32_t address = 0;
  int idx = 0, mask = 0;

  /* Get physical pipe from logical pipe */
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe_idx, &phy_pipe);

  /* Write key and mask */
  for (mask = 0; mask < 2; mask++) {
    /* write 8 bit phvs */
    for (idx = 0; idx < PIPE_MGR_TOF2_NUM_8BIT_MATCH_PHV; idx++) {
      /* Write all phvs that are part of this trigger. Unused phvs will
         be populated with don't-care. Note that input and
         output phvs are different
      */
      address = offsetof(tof2_reg,
                         pipes[phy_pipe]
                             .mau[stage]
                             .dp.snapshot_dp.snapshot_match
                             .mau_snapshot_match_subword8b[idx][mask]);
      pipe_mgr_snapshot_write_register(
          dev,
          0,
          address,
          phv_spec[mask].phvs8bit[PIPE_MGR_TOF2_MATCH2CAP_IDX_TRANSLATOR(idx)]);
      /*LOG_TRACE(
          "8bit: Dev %d: pipe %d, mask %d, idx %d,"
          " Writing address 0x%x, data 0x%x",
          dev,
          pipe_idx,
          mask,
          idx,
          address,
          phv_spec[mask].phvs8bit[PIPE_MGR_TOF2_MATCH2CAP_IDX_TRANSLATOR(idx)]);*/
    }
    /* write 16 bit phvs */
    for (idx = 0; idx < PIPE_MGR_TOF2_NUM_16BIT_MATCH_PHV; idx++) {
      address = offsetof(tof2_reg,
                         pipes[phy_pipe]
                             .mau[stage]
                             .dp.snapshot_dp.snapshot_match
                             .mau_snapshot_match_subword16b[idx][mask]);
      pipe_mgr_snapshot_write_register(
          dev,
          0,
          address,
          phv_spec[mask]
              .phvs16bit[PIPE_MGR_TOF2_MATCH2CAP_IDX_TRANSLATOR(idx)]);
      /*LOG_TRACE(
          "16bit: Dev %d: pipe %d, mask %d, idx %d,"
          " Writing address 0x%x, data 0x%x",
          dev,
          pipe_idx,
          mask,
          idx,
          address,
          phv_spec[mask].phvs16bit[PIPE_MGR_TOF2_MATCH2CAP_IDX_TRANSLATOR(idx)]);*/
    }
    /* write 32 bit phvs */
    for (idx = 0; idx < PIPE_MGR_TOF2_NUM_32BIT_MATCH_PHV; idx++) {
      address = offsetof(tof2_reg,
                         pipes[phy_pipe]
                             .mau[stage]
                             .dp.snapshot_dp.snapshot_match
                             .mau_snapshot_match_subword32b_lo[idx][mask]);
      pipe_mgr_snapshot_write_register(
          dev,
          0,
          address,
          phv_spec[mask]
              .phvs32bit_lo[PIPE_MGR_TOF2_MATCH2CAP_IDX_TRANSLATOR(idx)]);
      /*LOG_TRACE(
          "32bit_lo: Dev %d: pipe %d, mask %d, idx %d,"
          " Writing address 0x%x, data 0x%x",
          dev,
          pipe_idx,
          mask,
          idx,
          address,
          phv_spec[mask].phvs32bit_lo[PIPE_MGR_TOF2_MATCH2CAP_IDX_TRANSLATOR(idx)]);*/

      address = offsetof(tof2_reg,
                         pipes[phy_pipe]
                             .mau[stage]
                             .dp.snapshot_dp.snapshot_match
                             .mau_snapshot_match_subword32b_hi[idx][mask]);
      pipe_mgr_snapshot_write_register(
          dev,
          0,
          address,
          phv_spec[mask]
              .phvs32bit_hi[PIPE_MGR_TOF2_MATCH2CAP_IDX_TRANSLATOR(idx)]);
      /*LOG_TRACE(
          "32bit_hi: Dev %d: pipe %d, mask %d, idx %d,"
          " Writing address 0x%x, data 0x%x",
          dev,
          pipe_idx,
          mask,
          idx,
          address,
          phv_spec[mask].phvs32bit_hi[PIPE_MGR_TOF2_MATCH2CAP_IDX_TRANSLATOR(idx)]);*/
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_snapshot_fsm_state_set_tof2(
    rmt_dev_info_t *dev_info,
    bf_dev_pipe_t pipe_idx,
    dev_stage_t stage,
    bf_snapshot_dir_t dir,
    pipe_snapshot_fsm_state_t fsm_state) {
  pipe_status_t status = PIPE_SUCCESS;
  bf_dev_id_t dev = dev_info->dev_id;
  uint32_t data = fsm_state;
  uint32_t address = 0;
  bf_dev_pipe_t phy_pipe = 0;

  /* Get physical pipe from logical */
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe_idx, &phy_pipe);

  address = offsetof(tof2_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .dp.snapshot_ctl.mau_fsm_snapshot_cur_stateq[dir]);
  status = pipe_mgr_snapshot_write_register(dev, 0, address, data);
  return status;
}

pipe_status_t pipe_mgr_snapshot_fsm_state_get_tof2(
    rmt_dev_info_t *dev_info,
    bf_dev_pipe_t pipe,
    dev_stage_t stage,
    bf_snapshot_dir_t dir,
    pipe_snapshot_fsm_state_t *fsm_state) {
  pipe_status_t status = PIPE_SUCCESS;
  bf_dev_id_t dev = dev_info->dev_id;
  uint32_t address = 0, data = 0;
  bf_dev_pipe_t phy_pipe = 0;

  /* Get physical pipe from logical pipe */
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  address = offsetof(tof2_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .dp.snapshot_ctl.mau_fsm_snapshot_cur_stateq[dir]);
  status = pipe_mgr_snapshot_read_register(dev, address, &data);
  if (status != PIPE_SUCCESS) {
    return status;
  }

  *fsm_state = data & 0x3;
  return status;
}

static pipe_status_t pipe_mgr_get_enabled_next_tables(
    rmt_dev_info_t *dev_info,
    profile_id_t prof_id,
    bf_dev_pipe_t phy_pipe,
    int stage_id,
    int dir,
    uint16_t mpr_next_table_out,
    uint16_t *enabled_next_tables) {
  int next_tbl_stage =
      mpr_next_table_out / dev_info->dev_cfg.stage_cfg.num_logical_tables;
  uint8_t next_tbl_idx =
      mpr_next_table_out % dev_info->dev_cfg.stage_cfg.num_logical_tables;
  *enabled_next_tables = 0;
  if ((next_tbl_stage <= stage_id) ||
      (next_tbl_stage >= dev_info->profile_info[prof_id]->num_stages))
    return PIPE_SUCCESS;

  uint32_t address =
      offsetof(tof2_reg,
               pipes[phy_pipe]
                   .mau[next_tbl_stage]
                   .rams.match.merge.mpr_next_table_lut[dir][next_tbl_idx]);
  uint32_t data = 0;
  pipe_mgr_snapshot_read_register(dev_info->dev_id, address, &data);
  *enabled_next_tables = getp_tof2_mpr_next_table_lut_mpr_next_table_lut(&data);

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_get_pred_gbl_exec_out(
    rmt_dev_info_t *dev_info,
    profile_id_t prof_id,
    bf_dev_pipe_t phy_pipe,
    uint8_t stage_id,
    int dir,
    uint16_t gbl_exec_out,
    uint16_t *pred_gbl_exec_out) {
  int next_stage = stage_id + 1;

  *pred_gbl_exec_out = 0;
  if ((next_stage >= dev_info->profile_info[prof_id]->num_stages) ||
      (gbl_exec_out == 0))
    return PIPE_SUCCESS;

  uint32_t address = offsetof(tof2_reg,
                              pipes[phy_pipe]
                                  .mau[next_stage]
                                  .rams.match.merge.pred_glob_exec_thread[dir]);
  uint32_t data = 0;
  pipe_mgr_snapshot_read_register(dev_info->dev_id, address, &data);
  *pred_gbl_exec_out =
      gbl_exec_out &
      getp_tof2_pred_glob_exec_thread_pred_glob_exec_thread(&data);
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_get_enabled_gbl_exec_out(
    rmt_dev_info_t *dev_info,
    profile_id_t prof_id,
    bf_dev_pipe_t phy_pipe,
    uint8_t stage_id,
    int dir,
    uint16_t mpr_gbl_exec_out,
    uint16_t *enabled_gbl_exec_out) {
  int idx;
  int next_stage = stage_id + 1;
  uint16_t thread;

  *enabled_gbl_exec_out = 0;
  if ((next_stage >= dev_info->profile_info[prof_id]->num_stages) ||
      (mpr_gbl_exec_out == 0))
    return PIPE_SUCCESS;

  uint32_t address = offsetof(
      tof2_reg,
      pipes[phy_pipe].mau[next_stage].rams.match.merge.mpr_glob_exec_thread);
  uint32_t data = 0;
  pipe_mgr_snapshot_read_register(dev_info->dev_id, address, &data);
  thread = getp_tof2_mpr_glob_exec_thread_mpr_glob_exec_thread(&data);
  if (dir == 0) thread = ~thread;
  mpr_gbl_exec_out &= thread;

  for (idx = 0; idx < BF_MAX_LOG_TBLS; idx++) {
    if ((0x1 << idx) & mpr_gbl_exec_out) {
      address = offsetof(tof2_reg,
                         pipes[phy_pipe]
                             .mau[next_stage]
                             .rams.match.merge.mpr_glob_exec_lut[idx]);
      data = 0;
      pipe_mgr_snapshot_read_register(dev_info->dev_id, address, &data);
      *enabled_gbl_exec_out |=
          getp_tof2_mpr_glob_exec_lut_mpr_glob_exec_lut(&data);
    }
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_snapshot_long_branch_init(
    rmt_dev_info_t *dev_info,
    profile_id_t prof_id,
    bf_dev_pipe_t phy_pipe,
    dev_stage_t s_stage,
    int dir,
    pipe_mgr_snapshot_long_branch_t *long_branch) {
  bf_dev_id_t dev = dev_info->dev_id;
  uint32_t address = 0, data = 0;
  dev_stage_t stage;
  bool enabled;
  uint8_t thread;
  int i;
  dev_stage_t next_stage = s_stage + 1;

  long_branch->data = PIPE_MGR_MALLOC(
      (dev_info->profile_info[prof_id]->num_stages - next_stage) *
      sizeof(pipe_mgr_snapshot_stage_long_branch_t));
  if (!long_branch->data) {
    LOG_ERROR("Unable to allocate memory ");
    return PIPE_NO_SYS_RESOURCES;
  }
  pipe_mgr_snapshot_stage_long_branch_t *long_branch_stage = long_branch->data;
  long_branch->s_stage = next_stage;

  for (stage = next_stage; stage < dev_info->profile_info[prof_id]->num_stages;
       stage++) {
    address = offsetof(
        tof2_reg,
        pipes[phy_pipe].mau[stage].rams.match.merge.pred_long_brch_terminate);
    pipe_mgr_snapshot_read_register(dev, address, &data);
    long_branch_stage->terminate =
        getp_tof2_pred_long_brch_terminate_pred_long_brch_terminate(&data);
    /* Predication */
    address = offsetof(
        tof2_reg,
        pipes[phy_pipe].mau[stage].rams.match.merge.pred_long_brch_thread[dir]);
    pipe_mgr_snapshot_read_register(dev, address, &data);
    thread = getp_tof2_pred_long_brch_thread_pred_long_brch_thread(&data);

    for (i = 0; i < BF_MAX_LOG_TBLS; i++) {
      address = offsetof(
          tof2_reg,
          pipes[phy_pipe].mau[stage].rams.match.merge.pred_long_brch_lt_src[i]);
      pipe_mgr_snapshot_read_register(dev, address, &data);
      enabled =
          getp_tof2_pred_long_brch_lt_src_enabled_3bit_muxctl_enable(&data);
      if (enabled) {
        long_branch_stage->pred_lt_src[i] =
            getp_tof2_pred_long_brch_lt_src_enabled_3bit_muxctl_select(&data);
        if (!(thread & (1 << long_branch_stage->pred_lt_src[i])))
          enabled = false;
      }
      if (!enabled) long_branch_stage->pred_lt_src[i] = PIPE_MGR_INVALID_LT_SRC;
    }
    /* MPR */
    address = offsetof(
        tof2_reg,
        pipes[phy_pipe].mau[stage].rams.match.merge.mpr_long_brch_thread);
    pipe_mgr_snapshot_read_register(dev, address, &data);
    thread = getp_tof2_mpr_long_brch_thread_mpr_long_brch_thread(&data);
    if (dir == 0) thread = ~thread;

    for (i = 0; i < PIPE_MGR_NUM_LONG_BRCH; i++) {
      if (!(thread & (1 << i))) {
        long_branch_stage->mpr_lut[i] = 0;
        continue;
      }
      address = offsetof(
          tof2_reg,
          pipes[phy_pipe].mau[stage].rams.match.merge.mpr_long_brch_lut[i]);
      pipe_mgr_snapshot_read_register(dev, address, &data);
      long_branch_stage->mpr_lut[i] =
          getp_tof2_mpr_long_brch_lut_mpr_long_brch_lut(&data);
    }
    long_branch_stage++;
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_get_long_branch(
    rmt_dev_info_t *dev_info,
    profile_id_t prof_id,
    bf_dev_pipe_t phy_pipe,
    uint8_t stage_id,
    int dir,
    uint32_t long_branch_out,
    pipe_mgr_snapshot_long_branch_t *long_branch) {
  int next_stage = stage_id + 1;
  pipe_status_t rc;

  if ((next_stage >= dev_info->profile_info[prof_id]->num_stages) ||
      (long_branch_out == 0))
    return PIPE_SUCCESS;

  /* Initialize long branch data if needed. */
  if (long_branch->data == NULL) {
    rc = pipe_mgr_snapshot_long_branch_init(
        dev_info, prof_id, phy_pipe, stage_id, dir, long_branch);
    if (rc != PIPE_SUCCESS) return rc;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_get_snapshot_captured_data_tof2(
    rmt_dev_info_t *dev_info,
    bf_dev_pipe_t pipe,
    dev_stage_t stage,
    bf_snapshot_dir_t dir,
    pipe_mgr_phv_spec_t *phv_spec,
    pipe_mgr_snapshot_capture_data_t *capture) {
  bf_dev_id_t dev = dev_info->dev_id;
  int half_size = 0, group_size = 0, idx = 0, key = 0;
  bf_dev_pipe_t phy_pipe = 0;
  uint32_t address = 0;
  profile_id_t prof_id = 0;

  if (pipe_mgr_pipe_to_profile(dev_info, pipe, &prof_id, __func__, __LINE__) !=
      PIPE_SUCCESS) {
    return PIPE_INVALID_ARG;
  }

  /* Get physical pipe from logical */
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  /* read 8 bit phvs */
  half_size = PIPE_MGR_TOF2_NUM_8BIT_CAP_PHV / 2;
  group_size = PIPE_MGR_TOF2_NUM_8BIT_CAP_PHV / 4;
  for (idx = 0; idx < PIPE_MGR_TOF2_NUM_8BIT_CAP_PHV; idx++) {
    int which_half = idx / half_size;
    int which_group = (idx % half_size) / group_size;
    int grp_idx = idx % group_size;
    /* 8 bit regs are split into two regions, that is the array index on
     * snapshot_capture.  Each region is split into two groups of twenty. */
    address =
        offsetof(tof2_reg,
                 pipes[phy_pipe]
                     .mau[stage]
                     .dp.snapshot_dp.snapshot_capture[which_half]
                     .mau_snapshot_capture_subword8b[grp_idx][which_group]);
    pipe_mgr_snapshot_read_register(
        dev, address, &(phv_spec[key].phvs8bit[idx]));
  }
  /* read 16 bit phvs */
  half_size = PIPE_MGR_TOF2_NUM_16BIT_CAP_PHV / 2;
  group_size = PIPE_MGR_TOF2_NUM_16BIT_CAP_PHV / 6;
  for (idx = 0; idx < PIPE_MGR_TOF2_NUM_16BIT_CAP_PHV; idx++) {
    int which_half = idx / half_size;
    int which_group = (idx % half_size) / group_size;
    int grp_idx = idx % group_size;
    address =
        offsetof(tof2_reg,
                 pipes[phy_pipe]
                     .mau[stage]
                     .dp.snapshot_dp.snapshot_capture[which_half]
                     .mau_snapshot_capture_subword16b[grp_idx][which_group]);
    pipe_mgr_snapshot_read_register(
        dev, address, &(phv_spec[key].phvs16bit[idx]));
  }
  /* read 32 bit phvs */
  half_size = PIPE_MGR_TOF2_NUM_32BIT_CAP_PHV / 2;
  group_size = PIPE_MGR_TOF2_NUM_32BIT_CAP_PHV / 4;
  for (idx = 0; idx < PIPE_MGR_TOF2_NUM_32BIT_CAP_PHV; idx++) {
    int which_half = idx / half_size;
    int which_group = (idx % half_size) / group_size;
    int grp_idx = idx % group_size;
    /* 32 bit regs are split into two regions */
    address =
        offsetof(tof2_reg,
                 pipes[phy_pipe]
                     .mau[stage]
                     .dp.snapshot_dp.snapshot_capture[which_half]
                     .mau_snapshot_capture_subword32b_lo[grp_idx][which_group]);
    pipe_mgr_snapshot_read_register(
        dev, address, &(phv_spec[key].phvs32bit_lo[idx]));

    address =
        offsetof(tof2_reg,
                 pipes[phy_pipe]
                     .mau[stage]
                     .dp.snapshot_dp.snapshot_capture[which_half]
                     .mau_snapshot_capture_subword32b_hi[grp_idx][which_group]);
    pipe_mgr_snapshot_read_register(
        dev, address, &(phv_spec[key].phvs32bit_hi[idx]));
  }

  /* Read snapshot registers */
  address = offsetof(tof2_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .dp.snapshot_ctl.mau_snapshot_datapath_capture[dir]);
  pipe_mgr_snapshot_read_register(dev, address, &(capture->datapath_capture));

  address = offsetof(tof2_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .rams.match.merge.mau_snapshot_logical_table_hit);
  pipe_mgr_snapshot_read_register(dev, address, &(capture->logical_table_hit));

  address = offsetof(
      tof2_reg,
      pipes[phy_pipe]
          .mau[stage]
          .rams.match.merge.mau_snapshot_gateway_table_inhibit_logical);
  pipe_mgr_snapshot_read_register(
      dev, address, &(capture->gateway_table_inhibit_logical));

  for (idx = 0; idx < PIPE_MGR_MAX_PHY_BUS; idx++) {
    address = offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .mau[stage]
                           .rams.match.merge
                           .mau_snapshot_physical_exact_match_hit_address[idx]);
    pipe_mgr_snapshot_read_register(
        dev, address, &(capture->physical_exact_match_hit_address[idx]));

    address = offsetof(
        tof2_reg,
        pipes[phy_pipe]
            .mau[stage]
            .rams.match.merge.mau_snapshot_physical_tcam_hit_address[idx]);
    pipe_mgr_snapshot_read_register(
        dev, address, &(capture->physical_tcam_hit_address[idx]));
  }

  address = offsetof(tof2_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .rams.match.merge.mau_snapshot_table_active[dir]);
  pipe_mgr_snapshot_read_register(dev, address, &(capture->table_active));

  address = offsetof(tof2_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .rams.match.merge.mau_snapshot_next_table_out[dir]);
  pipe_mgr_snapshot_read_register(dev, address, &(capture->next_table_out));

  /* the regs that only tof2 have */
  address =
      offsetof(tof2_reg,
               pipes[phy_pipe]
                   .mau[stage]
                   .rams.match.merge.mau_snapshot_mpr_next_table_out[dir]);

  pipe_mgr_snapshot_read_register(dev, address, &(capture->mpr_next_table_out));
  pipe_status_t status =
      pipe_mgr_get_enabled_next_tables(dev_info,
                                       prof_id,
                                       phy_pipe,
                                       stage,
                                       dir,
                                       capture->mpr_next_table_out,
                                       &capture->enabled_next_tables);
  if (status != PIPE_SUCCESS) {
    LOG_TRACE(
        "Failed to get dev %d pipe %d stage %d snapshot enabled next tables %s",
        dev,
        pipe,
        stage,
        pipe_str_err(status));
    return status;
  }

  address = offsetof(tof2_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .rams.match.merge.mau_snapshot_global_exec_out[dir]);
  pipe_mgr_snapshot_read_register(dev, address, &(capture->global_exec_out));
  status = pipe_mgr_get_pred_gbl_exec_out(dev_info,
                                          prof_id,
                                          phy_pipe,
                                          stage,
                                          dir,
                                          capture->global_exec_out,
                                          &capture->pred_global_exec_out);
  if (status != PIPE_SUCCESS) {
    LOG_TRACE(
        "Failed to get dev %d pipe %d stage %d snapshot pred global exec "
        "out %s",
        dev,
        pipe,
        stage,
        pipe_str_err(status));
    return status;
  }

  address = offsetof(tof2_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .rams.match.merge.mau_snapshot_long_branch_out[dir]);
  pipe_mgr_snapshot_read_register(dev, address, &(capture->long_branch_out));
  status = pipe_mgr_get_long_branch(dev_info,
                                    prof_id,
                                    phy_pipe,
                                    stage,
                                    dir,
                                    capture->long_branch_out,
                                    &capture->long_branch);
  if (status != PIPE_SUCCESS) {
    LOG_TRACE(
        "Failed to get dev %d pipe %d stage %d snapshot long branch data %s",
        dev,
        pipe,
        stage,
        pipe_str_err(status));
    return status;
  }

  address =
      offsetof(tof2_reg,
               pipes[phy_pipe]
                   .mau[stage]
                   .rams.match.merge.mau_snapshot_mpr_global_exec_out[dir]);
  pipe_mgr_snapshot_read_register(
      dev, address, &(capture->mpr_global_exec_out));

  status = pipe_mgr_get_enabled_gbl_exec_out(dev_info,
                                             prof_id,
                                             phy_pipe,
                                             stage,
                                             dir,
                                             capture->mpr_global_exec_out,
                                             &capture->enabled_global_exec_out);
  if (status != PIPE_SUCCESS) {
    LOG_TRACE(
        "Failed to get dev %d pipe %d stage %d snapshot enabled global exec "
        "out %s",
        dev,
        pipe,
        stage,
        pipe_str_err(status));
    return status;
  }

  address =
      offsetof(tof2_reg,
               pipes[phy_pipe]
                   .mau[stage]
                   .rams.match.merge.mau_snapshot_mpr_long_branch_out[dir]);
  pipe_mgr_snapshot_read_register(
      dev, address, &(capture->mpr_long_branch_out));
  status = pipe_mgr_get_long_branch(dev_info,
                                    prof_id,
                                    phy_pipe,
                                    stage,
                                    dir,
                                    capture->mpr_long_branch_out,
                                    &capture->long_branch);
  if (status != PIPE_SUCCESS) {
    LOG_TRACE(
        "Failed to get dev %d pipe %d stage %d snapshot long branch data %s",
        dev,
        pipe,
        stage,
        pipe_str_err(status));
    return status;
  }

  for (idx = 0; idx < PIPE_MGR_MAX_METER_NUMB; idx++) {
    address = offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .mau[stage]
                           .rams.match.adrdist.mau_snapshot_meter_adr[idx]);
    pipe_mgr_snapshot_read_register(dev, address, &(capture->meter_adr[idx]));
  }
  address =
      offsetof(tof2_reg,
               pipes[phy_pipe]
                   .mau[stage]
                   .rams.match.merge.mau_snapshot_capture_datapath_error[dir]);
  pipe_mgr_snapshot_read_register(dev, address, &(capture->datapath_error));

  for (int i = 0; i < PIPE_MGR_TOF2_NUM_32BIT_CAP_PHV; ++i) {
    LOG_DBG("Dev %d Pipe %d Stage %d Dir %d PHV[%3d] = 0x%x",
            dev,
            pipe,
            stage,
            dir,
            i,
            (phv_spec[key].phvs32bit_hi[i] << 16) |
                (phv_spec[key].phvs32bit_lo[i] & 0xFFFF));
  }
  for (int i = 0; i < PIPE_MGR_TOF2_NUM_8BIT_CAP_PHV; ++i) {
    LOG_DBG("Dev %d Pipe %d Stage %d Dir %d PHV[%3d] = 0x%x",
            dev,
            pipe,
            stage,
            dir,
            i + PIPE_MGR_TOF2_NUM_32BIT_CAP_PHV,
            phv_spec[key].phvs8bit[i] & 0xFF);
  }
  for (int i = 0; i < PIPE_MGR_TOF2_NUM_16BIT_CAP_PHV; ++i) {
    LOG_DBG(
        "Dev %d Pipe %d Stage %d Dir %d PHV[%3d] = 0x%x",
        dev,
        pipe,
        stage,
        dir,
        i + PIPE_MGR_TOF2_NUM_32BIT_CAP_PHV + PIPE_MGR_TOF2_NUM_8BIT_CAP_PHV,
        phv_spec[key].phvs16bit[i] & 0xFFFF);
  }
  LOG_DBG("Dev %d Pipe %d Stage %d Dir %d datapath_capture = 0x%x",
          dev,
          pipe,
          stage,
          dir,
          capture->datapath_capture);
  LOG_DBG("Dev %d Pipe %d Stage %d Dir %d logical_table_hit = 0x%x",
          dev,
          pipe,
          stage,
          dir,
          capture->logical_table_hit);
  LOG_DBG("Dev %d Pipe %d Stage %d Dir %d gw_table_inhibit = 0x%x",
          dev,
          pipe,
          stage,
          dir,
          capture->gateway_table_inhibit_logical);
  for (int i = 0; i < PIPE_MGR_MAX_PHY_BUS; ++i) {
    LOG_DBG("Dev %d Pipe %d Stage %d Dir %d phy_exm_hit_addr[%2d] = 0x%x",
            dev,
            pipe,
            stage,
            dir,
            i,
            capture->physical_exact_match_hit_address[i]);
  }
  for (int i = 0; i < PIPE_MGR_MAX_PHY_BUS; ++i) {
    LOG_DBG("Dev %d Pipe %d Stage %d Dir %d phy_tcam_hit_addr[%2d] = 0x%x",
            dev,
            pipe,
            stage,
            dir,
            i,
            capture->physical_tcam_hit_address[i]);
  }
  LOG_DBG("Dev %d Pipe %d Stage %d Dir %d table_active = 0x%x",
          dev,
          pipe,
          stage,
          dir,
          capture->table_active);
  LOG_DBG("Dev %d Pipe %d Stage %d Dir %d next_tbl_out = 0x%x",
          dev,
          pipe,
          stage,
          dir,
          capture->next_table_out);
  LOG_DBG("Dev %d Pipe %d Stage %d Dir %d mpr_next_tbl_out = 0x%x",
          dev,
          pipe,
          stage,
          dir,
          capture->mpr_next_table_out);
  LOG_DBG("Dev %d Pipe %d Stage %d Dir %d global_exec_out = 0x%x",
          dev,
          pipe,
          stage,
          dir,
          capture->global_exec_out);
  LOG_DBG("Dev %d Pipe %d Stage %d Dir %d long_branch_out = 0x%x",
          dev,
          pipe,
          stage,
          dir,
          capture->long_branch_out);
  LOG_DBG("Dev %d Pipe %d Stage %d Dir %d mpr_global_exec_out = 0x%x",
          dev,
          pipe,
          stage,
          dir,
          capture->mpr_global_exec_out);
  LOG_DBG("Dev %d Pipe %d Stage %d Dir %d mpr_long_branch_out = 0x%x",
          dev,
          pipe,
          stage,
          dir,
          capture->mpr_long_branch_out);
  for (int i = 0; i < PIPE_MGR_MAX_METER_NUMB; ++i) {
    LOG_DBG("Dev %d Pipe %d Stage %d Dir %d meter_addr[%d] = 0x%x",
            dev,
            pipe,
            stage,
            dir,
            i,
            capture->meter_adr[i]);
  }
  LOG_DBG("Dev %d Pipe %d Stage %d Dir %d datapath_error = 0x%x",
          dev,
          pipe,
          stage,
          dir,
          capture->datapath_error);

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_snapshot_interrupt_clear_tof2(rmt_dev_info_t *dev_info,
                                                     bf_dev_pipe_t pipe,
                                                     dev_stage_t stage,
                                                     bf_snapshot_dir_t dir) {
  bf_dev_id_t dev = dev_info->dev_id;
  uint32_t address = 0, data = 0;
  bf_dev_pipe_t phy_pipe = 0;

  /* Get physical pipe from logical */
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  address = offsetof(tof2_reg,
                     pipes[phy_pipe].mau[stage].dp.intr_status_mau_snapshot);
  /* Set the bit to clear the interrupt (0-ingress,  1-egress) */
  data = 0x1 << dir;

  LOG_TRACE("%s: Clearing interrupt for pipe %d, stage %d, dir %d",
            __func__,
            pipe,
            stage,
            dir);
  pipe_mgr_snapshot_write_register(dev, 0, address, data);

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_snapshot_interrupt_get_tof2(rmt_dev_info_t *dev_info,
                                                   bf_dev_pipe_t pipe,
                                                   dev_stage_t stage,
                                                   bf_snapshot_dir_t dir,
                                                   bool *is_set) {
  bf_dev_id_t dev = dev_info->dev_id;
  uint32_t address = 0, data = 0;
  bf_dev_pipe_t phy_pipe = 0;

  /* Get physical pipe from logical */
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  address = offsetof(tof2_reg,
                     pipes[phy_pipe].mau[stage].dp.intr_status_mau_snapshot);

  pipe_mgr_snapshot_read_register(dev, address, &data);
  *is_set = (data >> dir) & 1;

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_snapshot_captured_trigger_type_get_tof2(
    rmt_dev_info_t *dev_info,
    bf_dev_pipe_t pipe,
    dev_stage_t stage,
    int dir,
    bool *prev_stage_trig,
    bool *local_stage_trig,
    bool *timer_trig) {
  bf_dev_id_t dev = dev_info->dev_id;
  bf_dev_pipe_t phy_pipe = 0;
  uint32_t address = 0, data = 0;

  /* Get physical pipe from logical pipe */
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  address = offsetof(tof2_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .dp.snapshot_ctl.mau_snapshot_datapath_capture[dir]);
  pipe_mgr_snapshot_read_register(dev, address, &data);
  *prev_stage_trig = (data >> dir) & 0x1;
  *timer_trig = (data >> 2) & 0x1;
  *local_stage_trig = (data >> (3 + dir)) & 0x1;

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_snapshot_captured_thread_get_tof2(
    rmt_dev_info_t *dev_info,
    bf_dev_pipe_t pipe,
    dev_stage_t stage,
    int dir,
    bool *ingress,
    bool *egress,
    bool *ghost) {
  bf_dev_id_t dev = dev_info->dev_id;
  bf_dev_pipe_t phy_pipe = 0;
  uint32_t address = 0, data = 0;

  /* Get physical pipe from logical pipe */
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  address = offsetof(tof2_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .dp.snapshot_ctl.mau_snapshot_datapath_capture[dir]);
  pipe_status_t status = pipe_mgr_snapshot_read_register(dev, address, &data);
  if (status) return status;

  *ingress = (data >> 11) & 0x1;
  *egress = (data >> 12) & 0x1;
  *ghost = (data >> 15) & 0x1;

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_snapshot_dp_reset_tof2(rmt_dev_info_t *dev_info,
                                              bf_dev_pipe_t pipe,
                                              dev_stage_t stage,
                                              int dir) {
  bf_dev_id_t dev = dev_info->dev_id;
  bf_dev_pipe_t phy_pipe = 0;
  uint32_t address = 0, data = 0;

  /* Get physical pipe from logical pipe */
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  address = offsetof(tof2_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .dp.snapshot_ctl.mau_snapshot_datapath_capture[dir]);
  pipe_status_t status =
      pipe_mgr_snapshot_write_register(dev, 0, address, data);

  return status;
}
