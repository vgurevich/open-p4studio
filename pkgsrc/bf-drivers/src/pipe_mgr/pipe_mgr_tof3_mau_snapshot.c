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
 * @file pipe_mgr_tof3_mau_snapshot.c
 * @date
 *
 * Implementation of MAU snapshot
 */

#include "pipe_mgr_int.h"
#include "pipe_mgr_sm.h"
#include "pipe_mgr_tbl.h"
#include "pipe_mgr_tof3_mau_snapshot.h"
#include "pipe_mgr_drv_intf.h"
#include <lld/bf_dma_if.h>
#include <lld/lld_reg_if.h>
#include <lld/lld_dr_if.h>
#include <tofino_regs/tofino.h>
#include <tofino/pdfixed/pd_common.h>

/* Translate the phv idx of match phv numbering(0-223) to the phv idx of capture
 * phv number(0-279) */
#define PIPE_MGR_TOF3_MATCH2CAP_IDX_TRANSLATOR(idx) \
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
                                                  bf_subdev_id_t subdev,
                                                  uint32_t reg_addr,
                                                  uint32_t *reg_data) {
  *reg_data = 0;
  return lld_subdev_read_register(dev, subdev, reg_addr, reg_data);
}

pipe_status_t pipe_mgr_snapshot_cfg_set_tof3(rmt_dev_info_t *dev_info,
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

    bf_subdev_id_t subdev = phy_pipe / BF_SUBDEV_PIPE_COUNT;
    phy_pipe = phy_pipe % BF_SUBDEV_PIPE_COUNT;

    address = offsetof(
        tof3_reg,
        pipes[phy_pipe].mau[stage].dp.snapshot_ctl.mau_snapshot_config);

    pipe_status_t status =
        pipe_mgr_snapshot_read_register(dev, subdev, address, &data);
    if (status != PIPE_SUCCESS) {
      return status;
    }

    setp_tof3_mau_snapshot_config_timebased_snapshot_ingress_enable(
        &data, timer_ig_enable);
    setp_tof3_mau_snapshot_config_timebased_snapshot_egress_enable(
        &data, timer_eg_enable);

    /* Zero out ingress mode bits */
    data &= 0x7;
    switch (mode) {
      case BF_SNAPSHOT_IGM_INGRESS:
        setp_tof3_mau_snapshot_config_snapshot_match_ghost_disable(&data, 1);
        break;
      case BF_SNAPSHOT_IGM_GHOST:
        setp_tof3_mau_snapshot_config_snapshot_match_ingress_disable(&data, 1);
        break;
      case BF_SNAPSHOT_IGM_ANY:
        /* All bits 0, nothing to do. */
        break;
      case BF_SNAPSHOT_IGM_BOTH:
        setp_tof3_mau_snapshot_config_snapshot_match_require_ingress_ghost(
            &data, 1);
        break;
      default:
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
    }

    status = pipe_mgr_snapshot_write_register(dev, subdev, address, data);
    if (status != PIPE_SUCCESS) {
      return status;
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_snapshot_cfg_get_tof3(rmt_dev_info_t *dev_info,
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
      tof3_reg, pipes[phy_pipe].mau[stage].dp.snapshot_ctl.mau_snapshot_config);

  pipe_mgr_snapshot_read_register(
      dev, pipe / BF_SUBDEV_PIPE_COUNT, address, &data);
  *timer_ig_enable =
      getp_tof3_mau_snapshot_config_timebased_snapshot_ingress_enable(&data);
  *timer_eg_enable =
      getp_tof3_mau_snapshot_config_timebased_snapshot_egress_enable(&data);

  if (getp_tof3_mau_snapshot_config_snapshot_match_ghost_disable(&data)) {
    *mode = BF_SNAPSHOT_IGM_INGRESS;
  } else if (getp_tof3_mau_snapshot_config_snapshot_match_ingress_disable(
                 &data)) {
    *mode = BF_SNAPSHOT_IGM_GHOST;
  } else if (getp_tof3_mau_snapshot_config_snapshot_match_require_ingress_ghost(
                 &data)) {
    *mode = BF_SNAPSHOT_IGM_BOTH;
  } else {
    /* All zero - any thread will trigger snapshot */
    *mode = BF_SNAPSHOT_IGM_ANY;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_snapshot_timer_enable_tof3(rmt_dev_info_t *dev_info,
                                                  bf_dev_pipe_t pipe,
                                                  dev_stage_t stage,
                                                  bool ing_enable,
                                                  bool egr_enable) {
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

    address = offsetof(tof3_reg,
                       pipes[phy_pipe % BF_SUBDEV_PIPE_COUNT]
                           .mau[stage]
                           .dp.snapshot_ctl.mau_snapshot_config);

    if (ing_enable) data |= 1;
    if (egr_enable) data |= 2;

    pipe_mgr_snapshot_write_register(
        dev, phy_pipe / BF_SUBDEV_PIPE_COUNT, address, data);
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_snapshot_timer_get_enable_tof3(rmt_dev_info_t *dev_info,
                                                      bf_dev_pipe_t pipe,
                                                      dev_stage_t stage,
                                                      bool *ing_enable,
                                                      bool *egr_enable) {
  uint32_t address = 0, data = 0;
  bf_dev_pipe_t phy_pipe = 0;
  bf_dev_id_t dev = dev_info->dev_id;

  if (pipe == BF_DEV_PIPE_ALL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  /* Get physical pipe from logical pipe */
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  address = offsetof(tof3_reg,
                     pipes[phy_pipe % BF_SUBDEV_PIPE_COUNT]
                         .mau[stage]
                         .dp.snapshot_ctl.mau_snapshot_config);

  pipe_mgr_snapshot_read_register(
      dev, phy_pipe / BF_SUBDEV_PIPE_COUNT, address, &data);
  *ing_enable = data & 1;
  *egr_enable = data & 2;

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_snapshot_timer_set_tof3(rmt_dev_info_t *dev_info,
                                               bf_dev_pipe_t pipe,
                                               dev_stage_t stage,
                                               uint64_t clocks) {
  bf_dev_id_t dev = dev_info->dev_id;
  bf_subdev_id_t subdev = 0;
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
    subdev = phy_pipe / BF_SUBDEV_PIPE_COUNT;
    phy_pipe = phy_pipe % BF_SUBDEV_PIPE_COUNT;
    /* Reset the time counter first */
    address = offsetof(
        tof3_reg,
        pipes[phy_pipe].mau[stage].dp.snapshot_ctl.mau_snapshot_timestamp_lo);
    pipe_mgr_snapshot_write_register(dev, subdev, address, 0);

    address = offsetof(
        tof3_reg,
        pipes[phy_pipe].mau[stage].dp.snapshot_ctl.mau_snapshot_timestamp_hi);
    pipe_mgr_snapshot_write_register(dev, subdev, address, 0);

    /* Write to lo */
    address = offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .mau[stage]
                           .dp.snapshot_ctl.mau_snapshot_timestamp_trigger_lo);
    data = clocks & 0xffffffff;
    pipe_mgr_snapshot_write_register(dev, subdev, address, data);

    /* Write to hi */
    address = offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .mau[stage]
                           .dp.snapshot_ctl.mau_snapshot_timestamp_trigger_hi);
    data = clocks >> 32;
    pipe_mgr_snapshot_write_register(dev, subdev, address, data);
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_snapshot_timer_get_tof3(rmt_dev_info_t *dev_info,
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

  bf_subdev_id_t subdev = phy_pipe / BF_SUBDEV_PIPE_COUNT;
  phy_pipe = phy_pipe % BF_SUBDEV_PIPE_COUNT;
  /* Read the time counter first */
  address = offsetof(
      tof3_reg,
      pipes[phy_pipe].mau[stage].dp.snapshot_ctl.mau_snapshot_timestamp_lo);
  pipe_mgr_snapshot_read_register(dev, subdev, address, &data_lo);

  address = offsetof(
      tof3_reg,
      pipes[phy_pipe].mau[stage].dp.snapshot_ctl.mau_snapshot_timestamp_hi);
  pipe_mgr_snapshot_read_register(dev, subdev, address, &data_hi);
  *clocks_now = data_hi;
  *clocks_now <<= 32;
  *clocks_now |= data_lo;

  /* Read the trigger next */
  address = offsetof(tof3_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .dp.snapshot_ctl.mau_snapshot_timestamp_trigger_lo);
  pipe_mgr_snapshot_read_register(dev, subdev, address, &data_lo);

  address = offsetof(tof3_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .dp.snapshot_ctl.mau_snapshot_timestamp_trigger_hi);
  pipe_mgr_snapshot_read_register(dev, subdev, address, &data_hi);
  *clocks_trig = data_hi;
  *clocks_trig <<= 32;
  *clocks_trig |= data_lo;

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_snapshot_capture_trigger_set_tof3(
    rmt_dev_info_t *dev_info,
    bf_dev_pipe_t pipe_idx,
    dev_stage_t stage,
    pipe_mgr_phv_spec_t *phv_spec) {
  bf_dev_id_t dev = dev_info->dev_id;
  bf_subdev_id_t subdev = 0;
  bf_dev_pipe_t phy_pipe = 0;
  uint32_t address = 0;
  int idx = 0, mask = 0;

  /* Get physical pipe from logical pipe */
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe_idx, &phy_pipe);
  subdev = phy_pipe / BF_SUBDEV_PIPE_COUNT;
  phy_pipe = phy_pipe % BF_SUBDEV_PIPE_COUNT;

  /* Write key and mask */
  for (mask = 0; mask < 2; mask++) {
    /* write 8 bit phvs */
    for (idx = 0; idx < PIPE_MGR_TOF3_NUM_8BIT_MATCH_PHV; idx++) {
      /* Write all phvs that are part of this trigger. Unused phvs will
         be populated with don't-care. Note that input and
         output phvs are different
      */
      address = offsetof(tof3_reg,
                         pipes[phy_pipe]
                             .mau[stage]
                             .dp.snapshot_dp.snapshot_match
                             .mau_snapshot_match_subword8b[idx][mask]);
      pipe_mgr_snapshot_write_register(
          dev,
          subdev,
          address,
          phv_spec[mask].phvs8bit[PIPE_MGR_TOF3_MATCH2CAP_IDX_TRANSLATOR(idx)]);
      /*LOG_TRACE(
          "8bit: Dev %d: pipe %d, mask %d, idx %d,"
          " Writing address 0x%x, data 0x%x",
          dev,
          pipe_idx,
          mask,
          idx,
          address,
          phv_spec[mask].phvs8bit[PIPE_MGR_TOF3_MATCH2CAP_IDX_TRANSLATOR(idx)]);*/
    }
    /* write 16 bit phvs */
    for (idx = 0; idx < PIPE_MGR_TOF3_NUM_16BIT_MATCH_PHV; idx++) {
      address = offsetof(tof3_reg,
                         pipes[phy_pipe]
                             .mau[stage]
                             .dp.snapshot_dp.snapshot_match
                             .mau_snapshot_match_subword16b[idx][mask]);
      pipe_mgr_snapshot_write_register(
          dev,
          subdev,
          address,
          phv_spec[mask]
              .phvs16bit[PIPE_MGR_TOF3_MATCH2CAP_IDX_TRANSLATOR(idx)]);
      /*LOG_TRACE(
          "16bit: Dev %d: pipe %d, mask %d, idx %d,"
          " Writing address 0x%x, data 0x%x",
          dev,
          pipe_idx,
          mask,
          idx,
          address,
          phv_spec[mask].phvs16bit[PIPE_MGR_TOF3_MATCH2CAP_IDX_TRANSLATOR(idx)]);*/
    }
    /* write 32 bit phvs */
    for (idx = 0; idx < PIPE_MGR_TOF3_NUM_32BIT_MATCH_PHV; idx++) {
      address = offsetof(tof3_reg,
                         pipes[phy_pipe]
                             .mau[stage]
                             .dp.snapshot_dp.snapshot_match
                             .mau_snapshot_match_subword32b_lo[idx][mask]);
      pipe_mgr_snapshot_write_register(
          dev,
          subdev,
          address,
          phv_spec[mask]
              .phvs32bit_lo[PIPE_MGR_TOF3_MATCH2CAP_IDX_TRANSLATOR(idx)]);
      /*LOG_TRACE(
          "32bit_lo: Dev %d: pipe %d, mask %d, idx %d,"
          " Writing address 0x%x, data 0x%x",
          dev,
          pipe_idx,
          mask,
          idx,
          address,
          phv_spec[mask].phvs32bit_lo[PIPE_MGR_TOF3_MATCH2CAP_IDX_TRANSLATOR(idx)]);*/

      address = offsetof(tof3_reg,
                         pipes[phy_pipe]
                             .mau[stage]
                             .dp.snapshot_dp.snapshot_match
                             .mau_snapshot_match_subword32b_hi[idx][mask]);
      pipe_mgr_snapshot_write_register(
          dev,
          subdev,
          address,
          phv_spec[mask]
              .phvs32bit_hi[PIPE_MGR_TOF3_MATCH2CAP_IDX_TRANSLATOR(idx)]);
      /*LOG_TRACE(
          "32bit_hi: Dev %d: pipe %d, mask %d, idx %d,"
          " Writing address 0x%x, data 0x%x",
          dev,
          pipe_idx,
          mask,
          idx,
          address,
          phv_spec[mask].phvs32bit_hi[PIPE_MGR_TOF3_MATCH2CAP_IDX_TRANSLATOR(idx)]);*/
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_snapshot_fsm_state_set_tof3(
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
  bf_subdev_id_t subdev = phy_pipe / BF_SUBDEV_PIPE_COUNT;
  phy_pipe = phy_pipe % BF_SUBDEV_PIPE_COUNT;

  address = offsetof(tof3_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .dp.snapshot_ctl.mau_fsm_snapshot_cur_stateq[dir]);
  status = pipe_mgr_snapshot_write_register(dev, subdev, address, data);
  return status;
}

pipe_status_t pipe_mgr_snapshot_fsm_state_get_tof3(
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

  bf_subdev_id_t subdev = phy_pipe / BF_SUBDEV_PIPE_COUNT;
  phy_pipe = phy_pipe % BF_SUBDEV_PIPE_COUNT;
  address = offsetof(tof3_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .dp.snapshot_ctl.mau_fsm_snapshot_cur_stateq[dir]);
  status = pipe_mgr_snapshot_read_register(dev, subdev, address, &data);
  if (status != PIPE_SUCCESS) {
    return status;
  }

  *fsm_state = data & 0x3;
  return status;
}

pipe_status_t pipe_mgr_get_snapshot_captured_data_tof3(
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

  /* Get physical pipe from logical */
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);
  bf_subdev_id_t subdev = phy_pipe / BF_SUBDEV_PIPE_COUNT;
  phy_pipe = phy_pipe % BF_SUBDEV_PIPE_COUNT;

  /* read 8 bit phvs */
  half_size = PIPE_MGR_TOF3_NUM_8BIT_CAP_PHV / 2;
  group_size = PIPE_MGR_TOF3_NUM_8BIT_CAP_PHV / 4;
  for (idx = 0; idx < PIPE_MGR_TOF3_NUM_8BIT_CAP_PHV; idx++) {
    int which_half = idx / half_size;
    int which_group = (idx % half_size) / group_size;
    int grp_idx = idx % group_size;
    /* 8 bit regs are split into two regions, that is the array index on
     * snapshot_capture.  Each region is split into two groups of twenty. */
    address =
        offsetof(tof3_reg,
                 pipes[phy_pipe]
                     .mau[stage]
                     .dp.snapshot_dp.snapshot_capture[which_half]
                     .mau_snapshot_capture_subword8b[grp_idx][which_group]);
    pipe_mgr_snapshot_read_register(
        dev, subdev, address, &(phv_spec[key].phvs8bit[idx]));
  }
  /* read 16 bit phvs */
  half_size = PIPE_MGR_TOF3_NUM_16BIT_CAP_PHV / 2;
  group_size = PIPE_MGR_TOF3_NUM_16BIT_CAP_PHV / 6;
  for (idx = 0; idx < PIPE_MGR_TOF3_NUM_16BIT_CAP_PHV; idx++) {
    int which_half = idx / half_size;
    int which_group = (idx % half_size) / group_size;
    int grp_idx = idx % group_size;
    address =
        offsetof(tof3_reg,
                 pipes[phy_pipe]
                     .mau[stage]
                     .dp.snapshot_dp.snapshot_capture[which_half]
                     .mau_snapshot_capture_subword16b[grp_idx][which_group]);
    pipe_mgr_snapshot_read_register(
        dev, subdev, address, &(phv_spec[key].phvs16bit[idx]));
  }
  /* read 32 bit phvs */
  half_size = PIPE_MGR_TOF3_NUM_32BIT_CAP_PHV / 2;
  group_size = PIPE_MGR_TOF3_NUM_32BIT_CAP_PHV / 4;
  for (idx = 0; idx < PIPE_MGR_TOF3_NUM_32BIT_CAP_PHV; idx++) {
    int which_half = idx / half_size;
    int which_group = (idx % half_size) / group_size;
    int grp_idx = idx % group_size;
    /* 32 bit regs are split into two regions */
    address =
        offsetof(tof3_reg,
                 pipes[phy_pipe]
                     .mau[stage]
                     .dp.snapshot_dp.snapshot_capture[which_half]
                     .mau_snapshot_capture_subword32b_lo[grp_idx][which_group]);
    pipe_mgr_snapshot_read_register(
        dev, subdev, address, &(phv_spec[key].phvs32bit_lo[idx]));

    address =
        offsetof(tof3_reg,
                 pipes[phy_pipe]
                     .mau[stage]
                     .dp.snapshot_dp.snapshot_capture[which_half]
                     .mau_snapshot_capture_subword32b_hi[grp_idx][which_group]);
    pipe_mgr_snapshot_read_register(
        dev, subdev, address, &(phv_spec[key].phvs32bit_hi[idx]));
  }

  /* Read snapshot registers */
  address = offsetof(tof3_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .dp.snapshot_ctl.mau_snapshot_datapath_capture[dir]);
  pipe_mgr_snapshot_read_register(
      dev, subdev, address, &(capture->datapath_capture));

  address = offsetof(tof3_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .rams.match.merge.mau_snapshot_logical_table_hit);
  pipe_mgr_snapshot_read_register(
      dev, subdev, address, &(capture->logical_table_hit));

  address = offsetof(
      tof3_reg,
      pipes[phy_pipe]
          .mau[stage]
          .rams.match.merge.mau_snapshot_gateway_table_inhibit_logical);
  pipe_mgr_snapshot_read_register(
      dev, subdev, address, &(capture->gateway_table_inhibit_logical));

  for (idx = 0; idx < PIPE_MGR_MAX_PHY_BUS; idx++) {
    address = offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .mau[stage]
                           .rams.match.merge
                           .mau_snapshot_physical_exact_match_hit_address[idx]);
    pipe_mgr_snapshot_read_register(
        dev,
        subdev,
        address,
        &(capture->physical_exact_match_hit_address[idx]));

    address = offsetof(
        tof3_reg,
        pipes[phy_pipe]
            .mau[stage]
            .rams.match.merge.mau_snapshot_physical_tcam_hit_address[idx]);
    pipe_mgr_snapshot_read_register(
        dev, subdev, address, &(capture->physical_tcam_hit_address[idx]));
  }

  address = offsetof(tof3_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .rams.match.merge.mau_snapshot_table_active[dir]);
  pipe_mgr_snapshot_read_register(
      dev, subdev, address, &(capture->table_active));

  address = offsetof(tof3_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .rams.match.merge.mau_snapshot_next_table_out[dir]);
  pipe_mgr_snapshot_read_register(
      dev, subdev, address, &(capture->next_table_out));

  /* the regs that only tof3 have */
  address =
      offsetof(tof3_reg,
               pipes[phy_pipe]
                   .mau[stage]
                   .rams.match.merge.mau_snapshot_mpr_next_table_out[dir]);
  pipe_mgr_snapshot_read_register(
      dev, subdev, address, &(capture->mpr_next_table_out));

  address = offsetof(tof3_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .rams.match.merge.mau_snapshot_global_exec_out[dir]);
  pipe_mgr_snapshot_read_register(
      dev, subdev, address, &(capture->global_exec_out));

  address = offsetof(tof3_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .rams.match.merge.mau_snapshot_long_branch_out[dir]);
  pipe_mgr_snapshot_read_register(
      dev, subdev, address, &(capture->long_branch_out));

  address =
      offsetof(tof3_reg,
               pipes[phy_pipe]
                   .mau[stage]
                   .rams.match.merge.mau_snapshot_mpr_global_exec_out[dir]);
  pipe_mgr_snapshot_read_register(
      dev, subdev, address, &(capture->mpr_global_exec_out));

  address =
      offsetof(tof3_reg,
               pipes[phy_pipe]
                   .mau[stage]
                   .rams.match.merge.mau_snapshot_mpr_long_branch_out[dir]);
  pipe_mgr_snapshot_read_register(
      dev, subdev, address, &(capture->mpr_long_branch_out));

  for (idx = 0; idx < PIPE_MGR_MAX_METER_NUMB; idx++) {
    address = offsetof(tof3_reg,
                       pipes[phy_pipe]
                           .mau[stage]
                           .rams.match.adrdist.mau_snapshot_meter_adr[idx]);
    pipe_mgr_snapshot_read_register(
        dev, subdev, address, &(capture->meter_adr[idx]));
  }
  address =
      offsetof(tof3_reg,
               pipes[phy_pipe]
                   .mau[stage]
                   .rams.match.merge.mau_snapshot_capture_datapath_error[dir]);
  pipe_mgr_snapshot_read_register(
      dev, subdev, address, &(capture->datapath_error));

  for (int i = 0; i < PIPE_MGR_TOF3_NUM_32BIT_CAP_PHV; ++i) {
    LOG_DBG("Dev %d Pipe %d Stage %d Dir %d PHV[%3d] = 0x%x",
            dev,
            pipe,
            stage,
            dir,
            i,
            (phv_spec[key].phvs32bit_hi[i] << 16) |
                (phv_spec[key].phvs32bit_lo[i] & 0xFFFF));
  }
  for (int i = 0; i < PIPE_MGR_TOF3_NUM_8BIT_CAP_PHV; ++i) {
    LOG_DBG("Dev %d Pipe %d Stage %d Dir %d PHV[%3d] = 0x%x",
            dev,
            pipe,
            stage,
            dir,
            i + PIPE_MGR_TOF3_NUM_32BIT_CAP_PHV,
            phv_spec[key].phvs8bit[i] & 0xFF);
  }
  for (int i = 0; i < PIPE_MGR_TOF3_NUM_16BIT_CAP_PHV; ++i) {
    LOG_DBG(
        "Dev %d Pipe %d Stage %d Dir %d PHV[%3d] = 0x%x",
        dev,
        pipe,
        stage,
        dir,
        i + PIPE_MGR_TOF3_NUM_32BIT_CAP_PHV + PIPE_MGR_TOF3_NUM_8BIT_CAP_PHV,
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

pipe_status_t pipe_mgr_snapshot_interrupt_clear_tof3(rmt_dev_info_t *dev_info,
                                                     bf_dev_pipe_t pipe,
                                                     dev_stage_t stage,
                                                     bf_snapshot_dir_t dir) {
  bf_dev_id_t dev = dev_info->dev_id;
  uint32_t address = 0, data = 0;
  bf_dev_pipe_t phy_pipe = 0;

  /* Get physical pipe from logical */
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);
  bf_subdev_id_t subdev = phy_pipe / BF_SUBDEV_PIPE_COUNT;
  phy_pipe = phy_pipe % BF_SUBDEV_PIPE_COUNT;

  address = offsetof(tof3_reg,
                     pipes[phy_pipe].mau[stage].dp.intr_status_mau_snapshot);
  /* Set the bit to clear the interrupt (0-ingress,  1-egress) */
  data = 0x1 << dir;

  LOG_TRACE("%s: Clearing interrupt for pipe %d, stage %d, dir %d",
            __func__,
            pipe,
            stage,
            dir);
  pipe_mgr_snapshot_write_register(dev, subdev, address, data);

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_snapshot_interrupt_get_tof3(rmt_dev_info_t *dev_info,
                                                   bf_dev_pipe_t pipe,
                                                   dev_stage_t stage,
                                                   bf_snapshot_dir_t dir,
                                                   bool *is_set) {
  bf_dev_id_t dev = dev_info->dev_id;
  uint32_t address = 0, data = 0;
  bf_dev_pipe_t phy_pipe = 0;

  /* Get physical pipe from logical */
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);
  bf_subdev_id_t subdev = phy_pipe / BF_SUBDEV_PIPE_COUNT;
  phy_pipe = phy_pipe % BF_SUBDEV_PIPE_COUNT;

  address = offsetof(tof3_reg,
                     pipes[phy_pipe].mau[stage].dp.intr_status_mau_snapshot);

  pipe_mgr_snapshot_read_register(dev, subdev, address, &data);
  *is_set = (data >> dir) & 1;

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_snapshot_captured_trigger_type_get_tof3(
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
  bf_subdev_id_t subdev = phy_pipe / BF_SUBDEV_PIPE_COUNT;
  phy_pipe = phy_pipe % BF_SUBDEV_PIPE_COUNT;

  address = offsetof(tof3_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .dp.snapshot_ctl.mau_snapshot_datapath_capture[dir]);
  pipe_mgr_snapshot_read_register(dev, subdev, address, &data);
  *prev_stage_trig = (data >> dir) & 0x1;
  *timer_trig = (data >> 2) & 0x1;
  *local_stage_trig = (data >> (3 + dir)) & 0x1;

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_snapshot_dp_reset_tof3(rmt_dev_info_t *dev_info,
                                              bf_dev_pipe_t pipe,
                                              dev_stage_t stage,
                                              int dir) {
  bf_dev_id_t dev = dev_info->dev_id;
  bf_dev_pipe_t phy_pipe = 0;
  uint32_t address = 0, data = 0;

  /* Get physical pipe from logical pipe */
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);
  bf_subdev_id_t subdev = phy_pipe / BF_SUBDEV_PIPE_COUNT;
  phy_pipe = phy_pipe % BF_SUBDEV_PIPE_COUNT;

  address = offsetof(tof3_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .dp.snapshot_ctl.mau_snapshot_datapath_capture[dir]);
  pipe_status_t status =
      pipe_mgr_snapshot_write_register(dev, subdev, address, data);

  return status;
}

pipe_status_t pipe_mgr_snapshot_captured_thread_get_tof3(
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
  bf_subdev_id_t subdev = phy_pipe / BF_SUBDEV_PIPE_COUNT;
  phy_pipe = phy_pipe % BF_SUBDEV_PIPE_COUNT;

  address = offsetof(tof3_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .dp.snapshot_ctl.mau_snapshot_datapath_capture[dir]);
  pipe_status_t status =
      pipe_mgr_snapshot_read_register(dev, subdev, address, &data);
  if (status) return status;

  *ingress = (data >> 11) & 0x1;
  *egress = (data >> 12) & 0x1;
  *ghost = (data >> 15) & 0x1;

  return PIPE_SUCCESS;
}
