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
 * @file pipe_mgr_meter_drv_workflows.h
 * @date
 *
 * Definitions for meter hardware interactions.
 */

#ifndef _PIPE_MGR_METER_DRV_WORKFLOWS_H
#define _PIPE_MGR_METER_DRV_WORKFLOWS_H

/* Standard header includes */

/* Module header includes */
#include "pipe_mgr/pipe_mgr_intf.h"

/* Local header includes */
#include "pipe_mgr_meter_mgr_int.h"

/* Defines for meter entry bit positions and field widths */
#define TOF_METER_CIR_BIT_POS 0
#define TOF_METER_PIR_BIT_POS 14
#define TOF_METER_CBS_BIT_POS 28
#define TOF_METER_PBS_BIT_POS 41
#define TOF_METER_COMMITTED_LEVEL_BIT_POS 54
#define TOF_METER_PEAK_LEVEL_BIT_POS 77
#define TOF_METER_TIMESTAMP_BIT_POS 100

#define TOF_METER_CIR_MANTISSA_WIDTH 9
#define TOF_METER_CIR_EXPONENT_WIDTH 5
#define TOF_METER_PIR_MANTISSA_WIDTH 9
#define TOF_METER_PIR_EXPONENT_WIDTH 5
#define TOF_METER_CBS_MANTISSA_WIDTH 8
#define TOF_METER_CBS_EXPONENT_WIDTH 5
#define TOF_METER_PBS_MANTISSA_WIDTH 8
#define TOF_METER_PBS_EXPONENT_WIDTH 5
#define TOF_METER_COMMITTED_LEVEL_BIT_WIDTH 23
#define TOF_METER_PEAK_LEVEL_BIT_WIDTH 23
#define TOF_METER_TIMESTAMP_BIT_WIDTH 28

/* Defines for LPF/RED entry bit positions and field widths */
#define TOF_LPF_RED_RED_LEVEL0_BIT_POS 0
#define TOF_LPF_RED_RED_DLEVEL100_BIT_POS 8
#define TOF_LPF_RED_RED_LEVEL_MAX_BIT_POS 16
#define TOF_LPF_RED_RED_LEVEL_EXPONENT_BIT_POS 24
#define TOF_LPF_RED_RED_PROBABILITY_SCALE_BIT_POS 29
#define TOF_LPF_RED_FALL_TIME_CONSTANT_EXPONENT_BIT_POS 33
#define TOF_LPF_RED_RISE_TIME_CONSTANT_EXPONENT_BIT_POS 38
#define TOF_LPF_RED_TIME_CONSTANT_MANTISSA_BIT_POS 43
#define TOF_LPF_RED_LPF_ACTION_SCALE_BIT_POS 57
#define TOF_LPF_RED_RATE_SAMPLE_ENABLE_BIT_POS 99

#define TOF_LPF_RED_RED_LEVEL0_BIT_WIDTH 8
#define TOF_LPF_RED_RED_DLEVEL100_BIT_WIDTH 8
#define TOF_LPF_RED_RED_LEVEL_MAX_BIT_WIDTH 8
#define TOF_LPF_RED_RED_LEVEL_EXPONENT_BIT_WIDTH 5
#define TOF_LPF_RED_RED_PROBABILITY_SCALE_BIT_WIDTH 3
#define TOF_LPF_RED_FALL_TIME_CONSTANT_EXPONENT_BIT_WIDTH 5
#define TOF_LPF_RED_RISE_TIME_CONSTANT_EXPONENT_BIT_WIDTH 5
#define TOF_LPF_RED_TIME_CONSTANT_MANTISSA_BIT_WIDTH 9
#define TOF_LPF_RED_LPF_ACTION_SCALE_BIT_WIDTH 5
#define TOF_LPF_RED_RATE_SAMPLE_ENABLE_BIT_WIDTH 1

/* Define for number of lower huffman bits in the meter address for tofino */
#define TOF_METER_NUM_LOWER_HUFFMAN_BITS 7

/* Define for number of VPN bits in the meter address for tofino */
#define TOF_METER_NUM_VPN_BITS 6

/* Structure definition of a meter entry */
typedef struct pipe_mgr_meter_tof_entry_ {
  uint32_t cir_mantissa;
  uint32_t cir_exponent;
  uint32_t pir_mantissa;
  uint32_t pir_exponent;
  uint32_t cbs_mantissa;
  uint32_t cbs_exponent;
  uint32_t pbs_mantissa;
  uint32_t pbs_exponent;
  int32_t committed_level; /* Committed bucket level */
  int32_t peak_level;      /* Peak bucket level */
  union {
    uint32_t timestamp;
    bool color_aware;
  } u;
} pipe_mgr_meter_tof_entry_t;

/* Structure definition of a LPF/RED entry */
typedef struct pipe_mgr_lpf_red_tof_entry_ {
  uint8_t red_level0;
  uint8_t red_dlevel100;
  uint8_t red_level_max;
  uint8_t red_level_exponent;
  uint8_t red_probability_scale;
  uint8_t decay_time_constant_exponent;
  uint8_t gain_time_constant_exponent;
  uint16_t time_constant_mantissa;
  uint8_t lpf_action_scale;
  uint8_t rate_sample_enable;
} pipe_mgr_lpf_red_tof_entry_t;

pipe_status_t pipe_mgr_meter_spec_update_drv_workflow(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mgr_meter_tbl_t *meter_tbl,
    pipe_bitmap_t pipe_bmp,
    pipe_mgr_meter_tbl_stage_info_t *meter_tbl_stage_info,
    vpn_id_t vpn,
    mem_id_t ram_id,
    uint32_t ram_line_num,
    pipe_meter_stage_idx_t meter_stage_idx,
    pipe_meter_spec_t *meter_spec);

pipe_status_t pipe_mgr_meter_spec_move_drv_workflow(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mgr_meter_tbl_t *meter_tbl,
    pipe_bitmap_t pipe_bmp,
    uint8_t src_stage_id,
    uint8_t dst_stage_id,
    mem_id_t src_ram_id,
    uint32_t src_ram_line_num,
    mem_id_t dst_ram_id,
    uint32_t dst_ram_line_num,
    vpn_id_t dst_vpn,
    uint8_t ltbl_id);

pipe_status_t pipe_mgr_lpf_spec_update_drv_workflow(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mgr_meter_tbl_t *meter_tbl,
    pipe_bitmap_t pipe_bmp,
    uint8_t ltbl_id,
    uint8_t stage_id,
    vpn_id_t vpn,
    mem_id_t ram_id,
    uint32_t ram_line_num,
    pipe_lpf_spec_t *lpf_spec);

pipe_status_t pipe_mgr_wred_spec_update_drv_workflow(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mgr_meter_tbl_t *meter_tbl,
    pipe_bitmap_t pipe_bmp,
    uint8_t ltbl_id,
    uint8_t stage_id,
    vpn_id_t vpn,
    mem_id_t ram_id,
    uint32_t ram_line_num,
    pipe_wred_spec_t *wred_spec);

pipe_status_t pipe_mgr_meter_max_meter_spec_workflow(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_meter_tbl_t *meter_tbl,
    pipe_bitmap_t pipe_bmp,
    uint8_t ltbl_id,
    uint8_t stage_id,
    vpn_id_t vpn,
    uint32_t ram_line_num);

pipe_status_t pipe_mgr_meter_ent_read_drv_workflow(
    rmt_dev_info_t *dev_info,
    pipe_tbl_dir_t gress,
    bf_dev_pipe_t pipe_id,
    uint8_t stage_id,
    uint8_t ltbl_id,
    vpn_id_t vpn,
    mem_id_t ram_id,
    uint32_t ram_line_num,
    pipe_meter_rate_type_e meter_rate_type,
    pipe_meter_spec_t *meter_specs,
    bool from_hw);

pipe_status_t pipe_mgr_lpf_ent_read_drv_workflow(rmt_dev_info_t *dev_info,
                                                 pipe_tbl_dir_t gress,
                                                 bf_dev_pipe_t pipe_id,
                                                 uint8_t stage_id,
                                                 uint8_t ltbl_id,
                                                 vpn_id_t vpn,
                                                 mem_id_t ram_id,
                                                 uint32_t ram_line_num,
                                                 pipe_lpf_spec_t *lpf_spec,
                                                 bool from_hw);

pipe_status_t pipe_mgr_wred_ent_read_drv_workflow(rmt_dev_info_t *dev_info,
                                                  pipe_tbl_dir_t gress,
                                                  bf_dev_pipe_t pipe_id,
                                                  uint8_t stage_id,
                                                  uint8_t ltbl_id,
                                                  vpn_id_t vpn,
                                                  mem_id_t ram_id,
                                                  uint32_t ram_line_num,
                                                  pipe_wred_spec_t *wred_spec,
                                                  bool from_hw);

pipe_status_t pipe_mgr_meter_tof_encode_lpf_spec(bf_dev_id_t device_id,
                                                 pipe_lpf_spec_t *lpf_spec,
                                                 rmt_ram_line_t *ram_line);

pipe_status_t pipe_mgr_meter_tof_encode_wred_spec(bf_dev_id_t device_id,
                                                  pipe_wred_spec_t *wred_spec,
                                                  rmt_ram_line_t *ram_line);

pipe_status_t pipe_mgr_meter_tof_decode_lpf_spec(bf_dev_id_t device_id,
                                                 pipe_lpf_spec_t *lpf_spec,
                                                 rmt_ram_line_t *ram_line);

pipe_status_t pipe_mgr_meter_tof_decode_wred_spec(bf_dev_id_t device_id,
                                                  pipe_wred_spec_t *wred_spec,
                                                  rmt_ram_line_t *ram_line);

pipe_status_t pipe_mgr_meter_compute_wred_params(
    pipe_mgr_lpf_red_tof_entry_t *wred_entry, pipe_wred_spec_t *wred_spec);

pipe_status_t pipe_mgr_meter_get_red_exponent(uint32_t dlevel100,
                                              uint32_t level0,
                                              uint32_t levelmax,
                                              uint8_t *exponent);

uint8_t pipe_mgr_meter_get_red_mantissa(uint32_t red_level, uint8_t exponent);

uint8_t pipe_mgr_meter_get_red_probability_scale(float rng_value,
                                                 uint32_t max_drop_probability);

pipe_status_t pipe_mgr_meter_tof_encode_max_meter_spec(
    rmt_ram_line_t *ram_line);

rmt_virt_addr_t pipe_mgr_meter_compute_ent_virt_addr(vpn_id_t vpn,
                                                     uint32_t ram_line_num);

void pipe_mgr_meter_drv_get_max_burstsize_params(uint32_t *burst_mantissa,
                                                 uint32_t *burst_exponent);

void pipe_mgr_meter_drv_get_max_rate_params(uint32_t *exponent,
                                            uint32_t *mantissa);

pipe_status_t pipe_mgr_meter_set_bytecount_adjust_drv_workflow(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev,
    pipe_bitmap_t pbm,
    uint32_t *data_db,
    uint8_t stage_id,
    uint8_t alu_id,
    int bytecount);

pipe_status_t pipe_mgr_meter_get_bytecount_adjust_drv_workflow(uint32_t data_db,
                                                               int *bytecount);

void pipe_mgr_lpf_get_exponent_mantissa(double decay_time_constant_in_cycles,
                                        int *exponent,
                                        int *mantissa);

void pipe_mgr_meter_encode_meter_entry(
    rmt_ram_line_t *ram_line, pipe_mgr_meter_tof_entry_t tof_meter_entry);

void pipe_mgr_meter_decode_meter_entry(
    rmt_ram_line_t *ram_line, pipe_mgr_meter_tof_entry_t *tof_meter_entry);

void pipe_mgr_meter_decode_hw_meter_entry(
    uint64_t *data0,
    uint64_t *data1,
    pipe_mgr_meter_tof_entry_t *tof_meter_entry);

void pipe_mgr_lpf_red_encode_entry(rmt_ram_line_t *ram_line,
                                   pipe_mgr_lpf_red_tof_entry_t lpf_red_entry);

void pipe_mgr_lpf_red_decode_entry(rmt_ram_line_t *ram_line,
                                   pipe_mgr_lpf_red_tof_entry_t *lpf_red_entry);

void meter_set_val(uint8_t *dst,
                   uint32_t dst_offset,
                   uint32_t len,
                   uint32_t val);

#endif  //_PIPE_MGR_METER_DRV_WORKFLOWS_H
