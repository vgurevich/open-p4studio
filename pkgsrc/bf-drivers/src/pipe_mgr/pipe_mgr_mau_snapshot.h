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
 * @file pipe_mgr_mau_snapshot.h
 * @date
 *
 * Contains definitions of MAU snapshot
 *
 */
#ifndef _PIPE_MGR_MAU_SNAPSHOT_H
#define _PIPE_MGR_MAU_SNAPSHOT_H

/* Module header includes */
#include <pipe_mgr/pipe_mgr_intf.h>

#define PIPE_MGR_SNAP_STAGE_INVALID 0xFF

#define PIPE_SNAP_TRIG_FIELD_NAME_LEN 100

#define PIPE_MGR_NUM_LONG_BRCH 8
#define PIPE_MGR_INVALID_LT_SRC 0xff

/* Per trigger snapshot info */
typedef struct pipe_snap_trig_field_info {
  bool valid;
  char name[PIPE_SNAP_TRIG_FIELD_NAME_LEN];
  uint8_t value[16];
  uint8_t mask[16];
  uint32_t width;
} pipe_snap_trig_field_info_t;

/* Per field info in a stage */
typedef struct pipe_snap_all_field_info {
  bool valid;
  char name[PIPE_SNAP_TRIG_FIELD_NAME_LEN];
  int container_num;
  int container_width;
  int container_type;
  int field_lsb;
  int field_msb;
  int phv_lsb;
  int phv_msb;
  int byte_offset;  // Offset from the start of list of fields
} pipe_snap_stage_field_info_t;
typedef enum pipe_snap_meter_alu_type_e {
  METER_ALU_NONE = 0,
  METER_ALU_SEL,
  METER_ALU_METER,
  METER_ALU_STFUL
} pipe_snap_meter_alu_type_t;

#define PIPE_MGR_TOF_NUM_32BIT_PHV 64
#define PIPE_MGR_TOF_NUM_16BIT_PHV 96
#define PIPE_MGR_TOF_NUM_8BIT_PHV 64
#define PIPE_MGR_TOF_NUM_TOTAL_PHV                           \
  (PIPE_MGR_TOF_NUM_32BIT_PHV + PIPE_MGR_TOF_NUM_16BIT_PHV + \
   PIPE_MGR_TOF_NUM_8BIT_PHV)
#define PIPE_MGR_TOF2_NUM_32BIT_MATCH_PHV 64
#define PIPE_MGR_TOF2_NUM_16BIT_MATCH_PHV 96
#define PIPE_MGR_TOF2_NUM_8BIT_MATCH_PHV 64
#define PIPE_MGR_TOF2_NUM_32BIT_CAP_PHV 80
#define PIPE_MGR_TOF2_NUM_16BIT_CAP_PHV 120
#define PIPE_MGR_TOF2_NUM_8BIT_CAP_PHV 80

#define PIPE_MGR_TOF3_NUM_32BIT_MATCH_PHV 64
#define PIPE_MGR_TOF3_NUM_16BIT_MATCH_PHV 96
#define PIPE_MGR_TOF3_NUM_8BIT_MATCH_PHV 64
#define PIPE_MGR_TOF3_NUM_32BIT_CAP_PHV 80
#define PIPE_MGR_TOF3_NUM_16BIT_CAP_PHV 120
#define PIPE_MGR_TOF3_NUM_8BIT_CAP_PHV 80

#define PIPE_MGR_MAX_PHY_BUS 16
#define PIPE_MGR_MAX_METER_NUMB 4
/* Snapshot PHV spec */
typedef struct pipe_mgr_phv_spec {
  /* Each phv has 1 valid bit also */
  uint32_t *phvs32bit_lo;
  uint32_t *phvs32bit_hi;  // valid bit here
  uint32_t *phvs16bit;
  uint32_t *phvs8bit;
  /* Offset the container index by these amounts depending on the container
   * size. */
  uint16_t phv_count;
  uint8_t base_32;
  uint8_t base_8;
  uint8_t base_16;
} pipe_mgr_phv_spec_t;

/* Snapshot long branch prediction and MPR data per stage */
typedef struct pipe_mgr_snapshot_stage_long_branch {
  uint8_t pred_lt_src[BF_MAX_LOG_TBLS];
  uint16_t mpr_lut[PIPE_MGR_NUM_LONG_BRCH];
  uint8_t terminate;
} pipe_mgr_snapshot_stage_long_branch_t;
/* Snapshot long branch prediction and MPR data */
typedef struct pipe_mgr_snapshot_long_branch {
  pipe_mgr_snapshot_stage_long_branch_t *data;
  dev_stage_t s_stage;
} pipe_mgr_snapshot_long_branch_t;

/* Snapshot captured data from asic */
typedef struct pipe_mgr_snapshot_capture_data {
  uint32_t datapath_capture;
  uint32_t logical_table_hit;
  uint32_t gateway_table_inhibit_logical;
  uint32_t physical_exact_match_hit_address[PIPE_MGR_MAX_PHY_BUS];
  uint32_t physical_tcam_hit_address[PIPE_MGR_MAX_PHY_BUS];
  uint32_t table_active;
  uint32_t next_table_out;
  uint32_t mpr_next_table_out;
  uint16_t enabled_next_tables;
  uint32_t global_exec_out;
  uint16_t pred_global_exec_out;
  uint32_t long_branch_out;
  uint32_t mpr_global_exec_out;
  uint16_t enabled_global_exec_out;
  uint32_t mpr_long_branch_out;
  pipe_mgr_snapshot_long_branch_t long_branch;
  uint32_t meter_adr[PIPE_MGR_MAX_METER_NUMB];
  uint32_t datapath_error;
  pipe_mgr_phv_spec_t phv_spec;
} pipe_mgr_snapshot_capture_data_t;

/* -------------  FUNCTIONS ----------- */

/**
 * Add device handling for snapshot
 * @param dev_id The ASIC id.
 * @return Status of the API call.
 */
pipe_status_t pipe_mgr_snapshot_add_device(bf_dev_id_t dev);

/**
 * Remove device handling for snapshot
 * @param dev_id The ASIC id.
 * @return Status of the API call.
 */
pipe_status_t pipe_mgr_snapshot_remove_device(bf_dev_id_t dev);

/**
 * Handle snapshot interrupt
 * @param dev_id The ASIC id.
 * @param pipe Pipeline
 * @param stage Stage
 * @param dir Direction
 * @return Status of the API call.
 */
pipe_status_t pipe_mgr_snapshot_handle_interrupt(bf_dev_id_t dev,
                                                 bf_dev_pipe_t pipe,
                                                 dev_stage_t stage,
                                                 bf_snapshot_dir_t dir);
/**
 * The function is used to enable snapshot
 *
 * @return                    Status of the API call
 */
pipe_status_t pipe_mgr_snapshot_init();

/* ---- Debug API's start ---- */

/**
 * Clear an interrupt
 * @param hdl Snapshot handle
 * @param pipe Pipeline
 * @return Status of the API call.
 */
pipe_status_t bf_snapshot_interrupt_clear(pipe_snapshot_hdl_t hdl,
                                          bf_dev_pipe_t pipe_input,
                                          dev_stage_t stage);

/**
 * Dump PHV allocation.
 * @param dev_id The ASIC id.
 * @param log_pipe Logical Pipe
 * @param stage Stage
 * @param dir dir
 * @param str String buffer.
 * @param max_str_len Max len of string buffer.
 * @return Status of the API call.
 */
pipe_status_t pipe_mgr_phv_allocation_dump(bf_dev_id_t dev,
                                           bf_dev_pipe_t log_pipe,
                                           dev_stage_t stage,
                                           bf_snapshot_dir_t dir,
                                           char *str,
                                           int max_len);

/**
 * Show snapshot state info from asic
 * @param hdl Snapshot handle
 * @param pipe_input Pipeline
 * @param str String buffer.
 * @param max_str_len Max len of string buffer.
 * @return Status of the API call.
 */
pipe_status_t pipe_mgr_snapshot_state_show(pipe_snapshot_hdl_t hdl,
                                           bf_dev_pipe_t pipe_input,
                                           dev_stage_t stage,
                                           char *str,
                                           int max_len);

/**
 * Show snapshot config
 * @param hdl Snapshot handle
 * @param pipe_input Pipeline
 * @param str String buffer.
 * @param max_str_len Max len of string buffer.
 * @return Status of the API call.
 */
pipe_status_t pipe_mgr_snapshot_config_dump(pipe_snapshot_hdl_t hdl,
                                            bf_dev_pipe_t pipe_input,
                                            dev_stage_t stage,
                                            char *str,
                                            int max_len);

/**
 * Show snapshot capture from asic
 * @param hdl Snapshot handle
 * @param pipe_input Pipeline
 * @param str String buffer.
 * @param max_str_len Max len of string buffer.
 * @return Status of the API call.
 */
pipe_status_t pipe_mgr_snapshot_capture_show(pipe_snapshot_hdl_t hdl,
                                             bf_dev_pipe_t pipe_input,
                                             dev_stage_t stage,
                                             char *str,
                                             int max_len);

/**
 * Dump all snapshot handles
 * @param dev_id The ASIC id.
 * @param hdl Snapshot handle
 * @param str String buffer.
 * @param max_str_len Max len of string buffer.
 * @return Status of the API call.
 */
pipe_status_t pipe_mgr_snapshot_hdls_dump(bf_dev_id_t dev,
                                          pipe_snapshot_hdl_t hdl,
                                          char *str,
                                          int max_len);

pipe_snap_stage_field_info_t *pipe_mgr_get_snap_stage_field_info_dict(
    bf_dev_id_t dev,
    bf_dev_pipe_t pipe,
    int stage,
    int direction,
    int *fields_in_dict);

/* ---- Debug API's end ---- */

#endif
