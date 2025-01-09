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
 * @file pipe_mgr_tof_mau_snapshot.c
 * @date
 *
 * Implementation of MAU snapshot
 */

#include "pipe_mgr_int.h"
#include "pipe_mgr_sm.h"
#include "pipe_mgr_tbl.h"
#include "pipe_mgr_mau_snapshot.h"
#include "pipe_mgr_tof_mau_snapshot.h"
#include "pipe_mgr_drv_intf.h"
#include <lld/bf_dma_if.h>
#include <lld/lld_reg_if.h>
#include <lld/lld_dr_if.h>
#include <tofino_regs/tofino.h>
#include <tofino/pdfixed/pd_common.h>

/* Wrapper API for reg write */
static inline pipe_status_t pipe_mgr_snapshot_write_register(
    bf_dev_id_t dev, uint32_t reg_addr, uint32_t reg_data) {
  return pipe_mgr_write_register(dev, 0, reg_addr, reg_data);
}

/* Wrapper API for reg read */
static inline int pipe_mgr_snapshot_read_register(bf_dev_id_t dev,
                                                  uint32_t reg_addr,
                                                  uint32_t *reg_data) {
  *reg_data = 0;
  return lld_subdev_read_register(dev, 0, reg_addr, reg_data);
}

pipe_status_t pipe_mgr_snapshot_timer_enable_tof(rmt_dev_info_t *dev_info,
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

    address = offsetof(
        Tofino, pipes[phy_pipe].mau[stage].dp.snapshot_ctl.mau_snapshot_config);

    if (ing_enable) data |= 1;
    if (egr_enable) data |= 2;

    pipe_mgr_snapshot_write_register(dev, address, data);
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_snapshot_timer_get_enable_tof(rmt_dev_info_t *dev_info,
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

  address = offsetof(
      Tofino, pipes[phy_pipe].mau[stage].dp.snapshot_ctl.mau_snapshot_config);

  pipe_mgr_snapshot_read_register(dev, address, &data);
  *ing_enable = data & 1;
  *egr_enable = data & 2;

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_snapshot_timer_set_tof(rmt_dev_info_t *dev_info,
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
        Tofino,
        pipes[phy_pipe].mau[stage].dp.snapshot_ctl.mau_snapshot_timestamp_lo);
    pipe_mgr_snapshot_write_register(dev, address, 0);

    address = offsetof(
        Tofino,
        pipes[phy_pipe].mau[stage].dp.snapshot_ctl.mau_snapshot_timestamp_hi);
    pipe_mgr_snapshot_write_register(dev, address, 0);

    /* Write to lo */
    address = offsetof(Tofino,
                       pipes[phy_pipe]
                           .mau[stage]
                           .dp.snapshot_ctl.mau_snapshot_timestamp_trigger_lo);
    data = clocks & 0xffffffff;
    pipe_mgr_snapshot_write_register(dev, address, data);

    /* Write to hi */
    address = offsetof(Tofino,
                       pipes[phy_pipe]
                           .mau[stage]
                           .dp.snapshot_ctl.mau_snapshot_timestamp_trigger_hi);
    data = clocks >> 32;
    pipe_mgr_snapshot_write_register(dev, address, data);
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_snapshot_timer_get_tof(rmt_dev_info_t *dev_info,
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
      Tofino,
      pipes[phy_pipe].mau[stage].dp.snapshot_ctl.mau_snapshot_timestamp_lo);
  pipe_mgr_snapshot_read_register(dev, address, &data_lo);

  address = offsetof(
      Tofino,
      pipes[phy_pipe].mau[stage].dp.snapshot_ctl.mau_snapshot_timestamp_hi);
  pipe_mgr_snapshot_read_register(dev, address, &data_hi);
  *clocks_now = data_hi;
  *clocks_now <<= 32;
  *clocks_now |= data_lo;

  /* Read the trigger next */
  address = offsetof(Tofino,
                     pipes[phy_pipe]
                         .mau[stage]
                         .dp.snapshot_ctl.mau_snapshot_timestamp_trigger_lo);
  pipe_mgr_snapshot_read_register(dev, address, &data_lo);

  address = offsetof(Tofino,
                     pipes[phy_pipe]
                         .mau[stage]
                         .dp.snapshot_ctl.mau_snapshot_timestamp_trigger_hi);
  pipe_mgr_snapshot_read_register(dev, address, &data_hi);
  *clocks_trig = data_hi;
  *clocks_trig <<= 32;
  *clocks_trig |= data_lo;

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_snapshot_capture_trigger_set_tof(
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
    for (idx = 0; idx < PIPE_MGR_TOF_NUM_8BIT_PHV; idx++) {
      /* Write all phvs that are part of this trigger. Unused phvs will
         be populated with don't-care. Note that input and
         output phvs are different
      */
      address = offsetof(Tofino,
                         pipes[phy_pipe]
                             .mau[stage]
                             .dp.snapshot_dp.snapshot_match
                             .mau_snapshot_match_subword8b[idx][mask]);
      pipe_mgr_snapshot_write_register(
          dev, address, phv_spec[mask].phvs8bit[idx]);
      /*LOG_TRACE(
          "8bit: Dev %d: pipe %d, mask %d, idx %d,"
          " Writing address 0x%x, data 0x%x",
          dev,
          pipe_idx,
          mask,
          idx,
          address,
          phv_spec[mask].phvs8bit[idx]);*/
    }
    /* write 16 bit phvs */
    for (idx = 0; idx < PIPE_MGR_TOF_NUM_16BIT_PHV; idx++) {
      address = offsetof(Tofino,
                         pipes[phy_pipe]
                             .mau[stage]
                             .dp.snapshot_dp.snapshot_match
                             .mau_snapshot_match_subword16b[idx][mask]);
      pipe_mgr_snapshot_write_register(
          dev, address, phv_spec[mask].phvs16bit[idx]);
      /*LOG_TRACE(
          "16bit: Dev %d: pipe %d, mask %d, idx %d,"
          " Writing address 0x%x, data 0x%x",
          dev,
          pipe_idx,
          mask,
          idx,
          address,
          phv_spec[mask].phvs16bit[idx]);*/
    }
    /* write 32 bit phvs */
    for (idx = 0; idx < PIPE_MGR_TOF_NUM_32BIT_PHV; idx++) {
      address = offsetof(Tofino,
                         pipes[phy_pipe]
                             .mau[stage]
                             .dp.snapshot_dp.snapshot_match
                             .mau_snapshot_match_subword32b_lo[idx][mask]);
      pipe_mgr_snapshot_write_register(
          dev, address, phv_spec[mask].phvs32bit_lo[idx]);
      /*LOG_TRACE(
          "32bit_lo: Dev %d: pipe %d, mask %d, idx %d,"
          " Writing address 0x%x, data 0x%x",
          dev,
          pipe_idx,
          mask,
          idx,
          address,
          phv_spec[mask].phvs32bit_lo[idx]);*/

      address = offsetof(Tofino,
                         pipes[phy_pipe]
                             .mau[stage]
                             .dp.snapshot_dp.snapshot_match
                             .mau_snapshot_match_subword32b_hi[idx][mask]);
      pipe_mgr_snapshot_write_register(
          dev, address, phv_spec[mask].phvs32bit_hi[idx]);
      /*LOG_TRACE(
          "32bit_hi: Dev %d: pipe %d, mask %d, idx %d,"
          " Writing address 0x%x, data 0x%x",
          dev,
          pipe_idx,
          mask,
          idx,
          address,
          phv_spec[mask].phvs32bit_hi[idx]);*/
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_snapshot_fsm_state_set_tof(
    rmt_dev_info_t *dev_info,
    bf_dev_pipe_t pipe_idx,
    dev_stage_t stage,
    bf_snapshot_dir_t dir,
    pipe_snapshot_fsm_state_t fsm_state) {
  bf_dev_id_t dev = dev_info->dev_id;
  uint32_t data = fsm_state;
  uint32_t address = 0;
  bf_dev_pipe_t phy_pipe = 0;

  /* Get physical pipe from logical */
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe_idx, &phy_pipe);

  address = offsetof(Tofino,
                     pipes[phy_pipe]
                         .mau[stage]
                         .dp.snapshot_ctl.mau_fsm_snapshot_cur_stateq[dir]);
  pipe_mgr_snapshot_write_register(dev, address, data);
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_snapshot_fsm_state_get_tof(
    rmt_dev_info_t *dev_info,
    bf_dev_pipe_t pipe,
    dev_stage_t stage,
    bf_snapshot_dir_t dir,
    pipe_snapshot_fsm_state_t *fsm_state) {
  bf_dev_id_t dev = dev_info->dev_id;
  uint32_t address = 0, data = 0;
  bf_dev_pipe_t phy_pipe = 0;

  /* Get physical pipe from logical pipe */
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  address = offsetof(Tofino,
                     pipes[phy_pipe]
                         .mau[stage]
                         .dp.snapshot_ctl.mau_fsm_snapshot_cur_stateq[dir]);
  pipe_mgr_snapshot_read_register(dev, address, &data);

  *fsm_state = data & 0x3;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_get_snapshot_captured_data_tof(
    rmt_dev_info_t *dev_info,
    bf_dev_pipe_t pipe,
    dev_stage_t stage,
    bf_snapshot_dir_t dir,
    pipe_mgr_phv_spec_t *phv_spec,
    pipe_mgr_snapshot_capture_data_t *capture) {
  bf_dev_id_t dev = dev_info->dev_id;
  int half = 0, idx = 0, key = 0;
  bf_dev_pipe_t phy_pipe = 0;
  uint32_t address = 0;

  /* Get physical pipe from logical */
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  /* read 8 bit phvs */
  half = PIPE_MGR_TOF_NUM_8BIT_PHV / 2;
  for (idx = 0; idx < PIPE_MGR_TOF_NUM_8BIT_PHV; idx++) {
    /* 8 bit regs are split into two regions */
    address = offsetof(Tofino,
                       pipes[phy_pipe]
                           .mau[stage]
                           .dp.snapshot_dp.snapshot_capture[idx / half]
                           .mau_snapshot_capture_subword8b[idx % half]);
    pipe_mgr_snapshot_read_register(
        dev, address, &(phv_spec[key].phvs8bit[idx]));
  }
  /* read 16 bit phvs */
  half = PIPE_MGR_TOF_NUM_16BIT_PHV / 2;
  for (idx = 0; idx < PIPE_MGR_TOF_NUM_16BIT_PHV; idx++) {
    /* 16 bit regs are split into two regions */
    address = offsetof(Tofino,
                       pipes[phy_pipe]
                           .mau[stage]
                           .dp.snapshot_dp.snapshot_capture[idx / half]
                           .mau_snapshot_capture_subword16b[idx % half]);
    pipe_mgr_snapshot_read_register(
        dev, address, &(phv_spec[key].phvs16bit[idx]));
  }
  /* read 32 bit phvs */
  half = PIPE_MGR_TOF_NUM_32BIT_PHV / 2;
  for (idx = 0; idx < PIPE_MGR_TOF_NUM_32BIT_PHV; idx++) {
    /* 32 bit regs are split into two regions */
    address = offsetof(Tofino,
                       pipes[phy_pipe]
                           .mau[stage]
                           .dp.snapshot_dp.snapshot_capture[idx / half]
                           .mau_snapshot_capture_subword32b_lo[idx % half]);
    pipe_mgr_snapshot_read_register(
        dev, address, &(phv_spec[key].phvs32bit_lo[idx]));

    address = offsetof(Tofino,
                       pipes[phy_pipe]
                           .mau[stage]
                           .dp.snapshot_dp.snapshot_capture[idx / half]
                           .mau_snapshot_capture_subword32b_hi[idx % half]);
    pipe_mgr_snapshot_read_register(
        dev, address, &(phv_spec[key].phvs32bit_hi[idx]));
  }

  /* Read snapshot registers */
  address = offsetof(Tofino,
                     pipes[phy_pipe]
                         .mau[stage]
                         .dp.snapshot_ctl.mau_snapshot_datapath_capture[dir]);
  pipe_mgr_snapshot_read_register(dev, address, &(capture->datapath_capture));

  address = offsetof(Tofino,
                     pipes[phy_pipe]
                         .mau[stage]
                         .rams.match.merge.mau_snapshot_logical_table_hit);
  pipe_mgr_snapshot_read_register(dev, address, &(capture->logical_table_hit));

  address = offsetof(
      Tofino,
      pipes[phy_pipe]
          .mau[stage]
          .rams.match.merge.mau_snapshot_gateway_table_inhibit_logical);
  pipe_mgr_snapshot_read_register(
      dev, address, &(capture->gateway_table_inhibit_logical));

  for (idx = 0; idx < PIPE_MGR_MAX_PHY_BUS; idx++) {
    address = offsetof(Tofino,
                       pipes[phy_pipe]
                           .mau[stage]
                           .rams.match.merge
                           .mau_snapshot_physical_exact_match_hit_address[idx]);
    pipe_mgr_snapshot_read_register(
        dev, address, &(capture->physical_exact_match_hit_address[idx]));

    address = offsetof(
        Tofino,
        pipes[phy_pipe]
            .mau[stage]
            .rams.match.merge.mau_snapshot_physical_tcam_hit_address[idx]);
    pipe_mgr_snapshot_read_register(
        dev, address, &(capture->physical_tcam_hit_address[idx]));
  }

  address = offsetof(
      Tofino,
      pipes[phy_pipe].mau[stage].rams.match.merge.mau_snapshot_table_active);
  pipe_mgr_snapshot_read_register(dev, address, &(capture->table_active));

  address = offsetof(Tofino,
                     pipes[phy_pipe]
                         .mau[stage]
                         .rams.match.merge.mau_snapshot_next_table_out[dir]);
  pipe_mgr_snapshot_read_register(dev, address, &(capture->next_table_out));

  for (int i = 0; i < PIPE_MGR_TOF_NUM_32BIT_PHV; ++i) {
    LOG_DBG("Dev %d Pipe %d Stage %d Dir %d PHV[%3d] = 0x%x%s",
            dev,
            pipe,
            stage,
            dir,
            i,
            (phv_spec[key].phvs32bit_hi[i] << 16) |
                (phv_spec[key].phvs32bit_lo[i] & 0xFFFF),
            (phv_spec[key].phvs32bit_hi[i] & 0x10000) ? "" : " Invalid");
  }
  for (int i = 0; i < PIPE_MGR_TOF_NUM_8BIT_PHV; ++i) {
    LOG_DBG("Dev %d Pipe %d Stage %d Dir %d PHV[%3d] = 0x%x%s",
            dev,
            pipe,
            stage,
            dir,
            i + PIPE_MGR_TOF_NUM_32BIT_PHV,
            phv_spec[key].phvs8bit[i] & 0xFF,
            (phv_spec[key].phvs8bit[i] & 0x100) ? "" : " Invalid");
  }
  for (int i = 0; i < PIPE_MGR_TOF_NUM_16BIT_PHV; ++i) {
    LOG_DBG("Dev %d Pipe %d Stage %d Dir %d PHV[%3d] = 0x%x%s",
            dev,
            pipe,
            stage,
            dir,
            i + PIPE_MGR_TOF_NUM_32BIT_PHV + PIPE_MGR_TOF_NUM_8BIT_PHV,
            phv_spec[key].phvs16bit[i] & 0xFFFF,
            (phv_spec[key].phvs16bit[i] & 0x10000) ? "" : " Invalid");
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

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_snapshot_interrupt_clear_tof(rmt_dev_info_t *dev_info,
                                                    bf_dev_pipe_t pipe,
                                                    dev_stage_t stage,
                                                    bf_snapshot_dir_t dir) {
  bf_dev_id_t dev = dev_info->dev_id;
  uint32_t address = 0, data = 0;
  bf_dev_pipe_t phy_pipe = 0;

  /* Get physical pipe from logical */
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  address =
      offsetof(Tofino, pipes[phy_pipe].mau[stage].dp.intr_status_mau_snapshot);
  /* Set the bit to clear the interrupt (0-ingress,  1-egress) */
  data = 0x1 << dir;

  LOG_TRACE("%s: Clearing interrupt for pipe %d, stage %d, dir %d",
            __func__,
            pipe,
            stage,
            dir);
  pipe_mgr_snapshot_write_register(dev, address, data);

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_snapshot_interrupt_get_tof(rmt_dev_info_t *dev_info,
                                                  bf_dev_pipe_t pipe,
                                                  dev_stage_t stage,
                                                  bf_snapshot_dir_t dir,
                                                  bool *is_set) {
  bf_dev_id_t dev = dev_info->dev_id;
  uint32_t address = 0, data = 0;
  bf_dev_pipe_t phy_pipe = 0;

  /* Get physical pipe from logical */
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  address =
      offsetof(Tofino, pipes[phy_pipe].mau[stage].dp.intr_status_mau_snapshot);

  pipe_mgr_snapshot_read_register(dev, address, &data);
  *is_set = (data >> dir) & 1;

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_snapshot_captured_trigger_type_get_tof(
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

  address = offsetof(Tofino,
                     pipes[phy_pipe]
                         .mau[stage]
                         .dp.snapshot_ctl.mau_snapshot_datapath_capture[dir]);
  pipe_mgr_snapshot_read_register(dev, address, &data);
  *prev_stage_trig = (data >> dir) & 0x1;
  *timer_trig = (data >> 2) & 0x1;
  *local_stage_trig = (data >> (3 + dir)) & 0x1;

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_snapshot_captured_thread_get_tof(
    rmt_dev_info_t *dev_info,
    bf_dev_pipe_t pipe,
    dev_stage_t stage,
    int dir,
    bool *ingress,
    bool *egress) {
  bf_dev_id_t dev = dev_info->dev_id;
  bf_dev_pipe_t phy_pipe = 0;
  uint32_t address = 0, data = 0;

  /* Get physical pipe from logical pipe */
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  address = offsetof(Tofino,
                     pipes[phy_pipe]
                         .mau[stage]
                         .dp.snapshot_ctl.mau_snapshot_datapath_capture[dir]);
  pipe_status_t status = pipe_mgr_snapshot_read_register(dev, address, &data);
  if (status) return status;

  *ingress = (data >> 11) & 0x1;
  *egress = (data >> 12) & 0x1;

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_snapshot_dp_reset_tof(rmt_dev_info_t *dev_info,
                                             bf_dev_pipe_t pipe,
                                             dev_stage_t stage,
                                             int dir) {
  bf_dev_id_t dev = dev_info->dev_id;
  bf_dev_pipe_t phy_pipe = 0;
  uint32_t address = 0, data = 0;

  /* Get physical pipe from logical pipe */
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  address = offsetof(Tofino,
                     pipes[phy_pipe]
                         .mau[stage]
                         .dp.snapshot_ctl.mau_snapshot_datapath_capture[dir]);
  pipe_status_t status = pipe_mgr_snapshot_write_register(dev, address, data);

  return status;
}
