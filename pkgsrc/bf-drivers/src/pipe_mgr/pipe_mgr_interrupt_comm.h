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
 * @file pipe_mgr_interrupt_comm.h
 * @date
 *
 * Pipe Manager interrupt handling common definitions.
 */

#ifndef _PIPE_MGR_INTERRUPT_COMM_H
#define _PIPE_MGR_INTERRUPT_COMM_H

#define PIPE_MGR_INTR_MAX_METERS 4
#define PIPE_MGR_INTR_MAX_STATS 4
#define PIPE_MGR_INTR_MAX_SEL_ALU_ROWS 4
#define PIPE_MGR_INTR_MAX_SEL_ALU_DIR 2
#define PIPE_MGR_INTR_MAX_IMEM_DIR 2
#define PIPE_MGR_INTR_TM_PRE_NUM_ERR 32
#define PIPE_MGR_INTR_GFM_NUM_ECC_ERR 16
#define PIPE_MGR_INTR_FULL_STATUS_VAL 0xffffffff

/*
 * PRE ram type.
 * Note that these enum values are tightly coupled to the Tofino1
 * and Tofino2 hardware.
 */
typedef enum {
  PIPE_INTR_PRE_RAM_FIFO = 0,
  PIPE_INTR_PRE_RAM_MIT,
  PIPE_INTR_PRE_RAM_LIT0_BM,
  PIPE_INTR_PRE_RAM_LIT1_BM,
  PIPE_INTR_PRE_RAM_LIT0_NP,
  PIPE_INTR_PRE_RAM_LIT1_NP,
  PIPE_INTR_PRE_RAM_PMT0,
  PIPE_INTR_PRE_RAM_PMT1,
  PIPE_INTR_PRE_RAM_RDM,
  PIPE_INTR_PRE_RAM_TOF_MAX = PIPE_INTR_PRE_RAM_RDM,
  PIPE_INTR_PRE_RAM_FIFO_BANKID,  // ONLY FOR TOF2
  PIPE_INTR_PRE_RAM_TOF2_MAX = PIPE_INTR_PRE_RAM_FIFO_BANKID
} pipe_intr_pre_ram_type_t;

/*
 * Test whether the ram_type of the sbe or mbe err_log value
 * matches the specified pipe_intr_pre_ram_type_t type.
 *
 * Note that this encoding is correct for Tofino1 but not for Tofino2.
 */
#define PIPE_MGR_INTR_PRE_RAM_TYPE_SET(bit_no, type) (type & (1u << bit_no))

/* Signal error event for entire pipe. */
#define BF_ERR_EVT(sev, dev, pipe, stage, addr, type, blk, loc, ...) \
  pipe_mgr_log_err_evt(                                              \
      sev, dev, pipe, stage, addr, type, blk, loc, __VA_ARGS__);     \
  bf_notify_error_events(sev,                                        \
                         dev,                                        \
                         pipe,                                       \
                         stage,                                      \
                         addr,                                       \
                         type,                                       \
                         blk,                                        \
                         loc,                                        \
                         NULL,                                       \
                         true,                                       \
                         NULL,                                       \
                         0,                                          \
                         __VA_ARGS__)

/* Signal error event for p4 object in pipe. */
#define BF_ERR_EVT_OBJ(                                          \
    sev, dev, pipe, stage, addr, obj_name, type, blk, loc, ...)  \
  if (obj_name) {                                                \
    LOG_TRACE("Error in P4 object %s", obj_name);                \
  }                                                              \
  pipe_mgr_log_err_evt(                                          \
      sev, dev, pipe, stage, addr, type, blk, loc, __VA_ARGS__); \
  bf_notify_error_events(sev,                                    \
                         dev,                                    \
                         pipe,                                   \
                         stage,                                  \
                         addr,                                   \
                         type,                                   \
                         blk,                                    \
                         loc,                                    \
                         obj_name,                               \
                         true,                                   \
                         NULL,                                   \
                         0,                                      \
                         __VA_ARGS__)

/* Signal error event for selected ports in pipe (not used). */
#define BF_ERR_EVT_PORT(                                             \
    sev, dev, pipe, addr, type, blk, loc, port_list, num_ports, ...) \
  pipe_mgr_log_err_evt(                                              \
      sev, dev, pipe, stage, addr, type, blk, loc, __VA_ARGS__);     \
  bf_notify_error_events(sev,                                        \
                         dev,                                        \
                         pipe,                                       \
                         addr,                                       \
                         type,                                       \
                         blk,                                        \
                         loc,                                        \
                         NULL,                                       \
                         false,                                      \
                         port_list,                                  \
                         num_ports,                                  \
                         __VA_ARGS__)

/* Size of the error event log string buffer, including the
 * terminating null character. */
#define PIPE_MGR_ERR_EVT_LOG_STR_LEN 125

/* An entry in the error event log. */
typedef struct pipe_mgr_err_evt_log_ {
  bool valid;
  bf_error_sev_level_t severity;
  bf_dev_id_t dev;
  bf_dev_pipe_t pipe;
  uint8_t stage;
  uint64_t address;
  bf_error_type_t type;
  bf_error_block_t blk;
  bf_error_block_location_t loc;
  char err_string[PIPE_MGR_ERR_EVT_LOG_STR_LEN];
} pipe_mgr_err_evt_log_t;

/* Error event log structure. */
typedef struct pipe_mgr_err_evt_all_logs_ {
  // Circular buffer (array) of log entries.
  pipe_mgr_err_evt_log_t *log;
  // Index of the last entry added to the circular buffer.
  // Will be -1 if the error log is empty.
  int front;
  // Write lock.
  pipe_mgr_mutex_t mtx;
} pipe_mgr_err_evt_all_logs_t;

/* User data passed to interrupt callback function.
 * Field names reflect their nominal usage for MAU ECC errors.
 * Actual usage varies according to the needs of the interrupt handler.
 */
typedef struct pipe_mgr_intr_userdata_ {
  // Pipe identifier. Usually the logical pipe.
  bf_dev_pipe_t pipe;
  // Stage number or other index.
  dev_stage_t stage;
  // Row number or other index.
  uint8_t row;
} pipe_mgr_intr_userdata_t;

/* Pipe manager interrupt database entry. */
typedef struct pipe_mgr_intr_db_ {
  bf_dev_id_t dev;
  // Timer to refresh TCAM contents.
  bf_sys_timer_t tcam_scrub_timer;
  uint32_t tcam_scrub_timer_val;
  // Callback data. Indexed by [pipe][stage][row].
  pipe_mgr_intr_userdata_t ***cb_data;
  // Error event log for this device.
  pipe_mgr_err_evt_all_logs_t err_evt_logs;
} pipe_mgr_intr_db_t;

/* Pipe manager interrupt database. */
extern pipe_mgr_intr_db_t *pipe_intr_evt_db[PIPE_MGR_NUM_DEVICES];

#define PIPE_INTR_ERR_EVT_LOG(dev, _index) \
  (pipe_intr_evt_db[dev]->err_evt_logs.log[_index])
#define PIPE_INTR_ERR_EVT_FRONT(dev) (pipe_intr_evt_db[dev]->err_evt_logs.front)
#define PIPE_INTR_ERR_EVT_MTX(dev) (pipe_intr_evt_db[dev]->err_evt_logs.mtx)
#define PIPE_INTR_CALLBACK_DATA(dev, pipe, stage, row) \
  (pipe_intr_evt_db[dev]->cb_data[pipe][stage][row])

/* - ECC Handling internal use - */
pipe_status_t pipe_mgr_log_err_evt(bf_error_sev_level_t severity,
                                   bf_dev_id_t dev_id,
                                   bf_dev_pipe_t pipe,
                                   uint8_t stage,
                                   uint64_t address,
                                   bf_error_type_t type,
                                   bf_error_block_t blk,
                                   bf_error_block_location_t loc,
                                   const char *format,
                                   ...);
int pipe_mgr_interrupt_read_register(bf_dev_id_t dev,
                                     uint32_t reg_addr,
                                     uint32_t *reg_data);
pipe_status_t pipe_mgr_interrupt_write_register(bf_dev_id_t dev,
                                                bf_subdev_id_t subdev,
                                                uint32_t reg_addr,
                                                uint32_t reg_data);
bool tbl_is_s2p(pipe_tbl_hdl_t hdl);
pipe_status_t pipe_mgr_intr_sram_tcam_ecc_correct(pipe_sess_hdl_t sess_hdl,
                                                  bf_dev_id_t dev,
                                                  pipe_tbl_hdl_t tbl_hdl,
                                                  uint64_t phy_addr);

pipe_status_t pipe_mgr_interrupt_set_enable_val(bf_dev_id_t dev,
                                                bf_subdev_id_t subdev,
                                                uint32_t enable_hi_addr,
                                                uint32_t clear_val);
pipe_status_t pipe_mgr_non_pipe_interrupt_set_enable_val(
    bf_dev_id_t dev,
    bf_subdev_id_t subdev,
    uint32_t enable_hi_addr,
    uint32_t clear_val);
pipe_status_t s2p_ram_sbe_correct(pipe_status_t sess_hdl,
                                  bf_dev_id_t dev,
                                  pipe_tbl_hdl_t tbl_hdl,
                                  bf_dev_pipe_t log_pipe_id,
                                  dev_stage_t stage_id,
                                  int mem_offset);
uint32_t pipe_mgr_gfm_err_handle(bf_dev_id_t dev,
                                 bf_subdev_id_t subdev_id,
                                 uint32_t intr_address,
                                 uint32_t intr_status_val,
                                 uint32_t enable_hi_addr,
                                 uint32_t enable_lo_addr,
                                 void *userdata);
pipe_status_t pipe_mgr_intr_mirr_tbl_ecc_correct(pipe_sess_hdl_t sess_hdl,
                                                 bf_dev_id_t dev,
                                                 bf_dev_pipe_t log_pipe,
                                                 uint32_t entry,
                                                 bool direction);
pipe_status_t pipe_mgr_intr_map_ram_idle_ecc_correct(pipe_sess_hdl_t sess_hdl,
                                                     bf_dev_id_t dev,
                                                     pipe_tbl_hdl_t tbl_hdl,
                                                     bf_dev_pipe_t pipe,
                                                     dev_stage_t stage,
                                                     mem_id_t mem_id,
                                                     uint32_t mem_offset);
void pipe_mgr_err_evt_log_dump_by_index(ucli_context_t *uc,
                                        bf_dev_id_t dev,
                                        int index,
                                        int intr_num);
#endif
