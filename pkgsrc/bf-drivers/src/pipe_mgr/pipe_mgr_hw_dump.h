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
 * @file pipe_mgr_hw_dump.h
 * @date
 *
 * Contains definitions of pipeline management peek/poke interface
 *
 */
#ifndef _PIPE_MGR_HW_DUMP_H
#define _PIPE_MGR_HW_DUMP_H

/* Standard header includes */
#include "pipe_mgr_act_tbl.h"

/* Module header includes */
#include <pipe_mgr/pipe_mgr_intf.h>

#define PIPE_INVALID_VAL 255
#define STAGE_INVALID_VAL 255
#define PYLD_INVALID_VAL 255

/**
 * Print Match table info.
 * @param dev_id The ASIC id.
 * @param tbl_hdl Table handle.
 * @param ent_hdl Entry handle.
 * @param log_pipe Logical Pipeline-id.
 * @param str String buffer.
 * @param max_str_len Max len of string buffer.
 * @return Status of the API call.
 */
pipe_status_t pipe_mgr_print_mat_hw_tbl_info_w_lock(bf_dev_id_t dev_id,
                                                    pipe_mat_tbl_hdl_t tbl_hdl,
                                                    pipe_mat_ent_hdl_t ent_hdl,
                                                    bf_dev_pipe_t log_pipe,
                                                    uint32_t subindex,
                                                    char *str,
                                                    int max_str_len);
/**
 * Print Action table info.
 * @param dev_id The ASIC id.
 * @param tbl_hdl Table handle.
 * @param ent_hdl Entry handle.
 * @param act_fn_hdl Action-function handle.
 * @param log_pipe Logical Pipeline-id.
 * @param stage Stage-id.
 * @param str String buffer.
 * @param max_str_len Max len of string buffer.
 * @return Status of the API call.
 */
pipe_status_t pipe_mgr_print_act_hw_tbl_info_w_lock(
    bf_dev_id_t dev_id,
    pipe_adt_tbl_hdl_t tbl_hdl,
    pipe_adt_ent_hdl_t ent_hdl,
    pipe_act_fn_hdl_t act_fn_hdl,
    bf_dev_pipe_t log_pipe,
    uint8_t stage,
    char *str,
    int max_str_len);

/**
 * Print Action table info by entry index.
 * @param dev_id The ASIC id.
 * @param tbl_hdl Table handle.
 * @param entry_idx entry index.
 * @param act_fn_hdl Action-function handle.
 * @param log_pipe Logical Pipeline-id.
 * @param stage Stage-id.
 * @param str String buffer.
 * @param max_str_len Max len of string buffer.
 * @return Status of the API call.
 */
pipe_status_t pipe_mgr_print_act_hw_tbl_info_by_idx_w_lock(
    bf_dev_id_t dev_id,
    pipe_adt_tbl_hdl_t tbl_hdl,
    pipe_adt_ent_idx_t entry_idx,
    pipe_act_fn_hdl_t act_fn_hdl,
    bf_dev_pipe_t log_pipe,
    uint8_t stage,
    char *str,
    int max_str_len);

/**
 * Dump hardware memory.
 * @param dev_id The ASIC id.
 * @param addr Address on the asic.
 * @param str String buffer.
 * @param max_str_len Max len of string buffer.
 * @return Status of the API call.
 */
pipe_status_t pipe_mgr_dump_hw_memory(bf_dev_id_t dev_id,
                                      bf_subdev_id_t subdev,
                                      uint64_t addr,
                                      char *str,
                                      int max_str_len);

/**
 * Dump hardware memory using all params.
 * @param dev_id The ASIC id.
 * @param log_pipe_id Logical Pipe Id
 * @param stage_id Stage
 * @param mem_id Memory
 * @param mem_type Memory type
 * @param line_no Line no
 * @param str String buffer.
 * @param max_str_len Max len of string buffer.
 * @return Status of the API call.
 */
pipe_status_t pipe_mgr_dump_hw_memory_full(bf_dev_id_t dev_id,
                                           pipe_tbl_dir_t gress,
                                           bf_dev_pipe_t log_pipe_id,
                                           uint8_t stage_id,
                                           mem_id_t mem_id,
                                           uint8_t mem_type,
                                           uint16_t line_no,
                                           char *str,
                                           int max_len);

/**
 * Write to hardware memory within a range of bits.
 * @param dev_id The ASIC id.
 * @param addr Address on the asic.
 * @param data0 Lower 64 bit value.
 * @param data1 Upper 64 bit value.
 * @param start_bit Start bit.
 * @param end_bit End bit.
 * @param str String buffer.
 * @param max_str_len Max len of string buffer.
 * @return Status of the API call.
 */
pipe_status_t pipe_mgr_write_hw_memory_with_start_end(bf_dev_id_t dev_id,
                                                      bf_subdev_id_t subdev,
                                                      uint64_t addr,
                                                      uint64_t data0,
                                                      uint64_t data1,
                                                      int start_bit,
                                                      int end_bit,
                                                      char *str,
                                                      int max_str_len);

/**
 * Write to Tcam memory within a range of bits.
 * @param dev_id The ASIC id.
 * @param tbl_hdl Table handle.
 * @param addr Address on the asic.
 * @param key Key.
 * @param mask Mask.
 * @param payload payload bit.
 * @param mrd Mrd bit.
 * @param version Version bits.
 * @param start_bit Start bit.
 * @param end_bit End bit.
 * @param str String buffer.
 * @param max_str_len Max len of string buffer.
 * @return Status of the API call.
 */
pipe_status_t pipe_mgr_write_tcam_memory_with_start_end(
    bf_dev_id_t dev_id,
    bf_subdev_id_t subdev,
    pipe_mat_tbl_hdl_t tbl_hdl,
    uint64_t addr,
    uint64_t key,
    uint64_t mask,
    uint8_t payload,
    uint8_t mrd,
    uint64_t version,
    int start_bit,
    int end_bit,
    char *str,
    int max_str_len);

/**
 * Dump action table by virtual address.
 * @param dev_id The ASIC id.
 * @param addr Address on the asic.
 * @param tbl_hdl Table handle.
 * @param log_pipe_id Logical Pipeline-id.
 * @param stage_id Stage-id.
 * @param act_fn_hdl Action function handle.
 * @param str String buffer.
 * @param max_str_len Max len of string buffer.
 * @return Status of the API call.
 */
pipe_status_t pipe_mgr_dump_act_tbl_by_virtaddr(bf_dev_id_t dev_id,
                                                uint32_t addr,
                                                pipe_mat_tbl_hdl_t tbl_hdl,
                                                bf_dev_pipe_t log_pipe_id,
                                                uint8_t stage_id,
                                                pipe_act_fn_hdl_t act_fn_hdl,
                                                char *str,
                                                int max_str_len);

/**
 * Dump match table by virtual address.
 * @param dev_id The ASIC id.
 * @param addr Address on the asic.
 * @param tbl_hdl Table handle.
 * @param log_pipe_id Logical Pipeline-id.
 * @param stage_id Stage-id.
 * @param stage_table_handle The stage table handle assigned for this address
 * @param is_tind Is it tind table.
 * @param str String buffer.
 * @param max_str_len Max len of string buffer.
 * @return Status of the API call.
 */
pipe_status_t pipe_mgr_dump_mat_tbl_by_virtaddr(bf_dev_id_t dev_id,
                                                uint32_t addr,
                                                pipe_mat_tbl_hdl_t tbl_hdl,
                                                bf_dev_pipe_t log_pipe_id,
                                                uint8_t stage_id,
                                                uint8_t stage_table_handle,
                                                int is_tind,
                                                char *str,
                                                int max_str_len);

/**
 * Write to a register.
 * @param dev_id The ASIC id.
 * @param addr Address on the asic.
 * @param data Value.
 * @param str String buffer.
 * @param max_str_len Max len of string buffer.
 * @return Status of the API call.
 */
pipe_status_t pipe_mgr_dbg_write_register(bf_dev_id_t dev_id,
                                          bf_subdev_id_t subdev_id,
                                          uint32_t addr,
                                          uint32_t data,
                                          char *str,
                                          int max_str_len);

/**
 * Dump any table by physical addresses.
 * @param dev_id The ASIC id.
 * @param subdev Number of subdevice to access.
 * @param tbl_hdl Table handle.
 * @param stage_table_handle The stage table handle assigned
 * @param stage_id Stage-id.
 * @param tbl_type table type (exact-match, tcam, tind, action).
 * @param addr Array of physical addresses.
 * @param num_phy_addrs Num of physical addresses.
 * @param entry_position Entry position.
 * @param act_fn_hdl Action function handle.
 * @param c_str_len Current string length.
 * @param str String buffer.
 * @param max_str_len Max len of string buffer.
 * @param read_data If provided read values will be placed in provided memory.
 * @param phy_addrs_map Used with read_data for HASH/ATCAM read.
 * @param sess_hdl If session hdl is provided function will use DMA instead of
 *                 lld indirect read.
 * @return Status of the API call.
 */
pipe_status_t pipe_mgr_dump_any_tbl_by_addr(bf_dev_id_t dev_id,
                                            bf_subdev_id_t subdev,
                                            pipe_mat_tbl_hdl_t tbl_hdl,
                                            uint8_t stage_table_handle,
                                            uint8_t stage_id,
                                            rmt_tbl_type_t tbl_type,
                                            uint64_t *addr,
                                            int num_phy_addrs,
                                            int entry_position,
                                            pipe_act_fn_hdl_t act_fn_hdl,
                                            int *c_str_len,
                                            char *str,
                                            int max_len,
                                            uint8_t **read_data,
                                            uint8_t *phy_addrs_map,
                                            pipe_sess_hdl_t *sess_hdl);

pipe_status_t pipe_mgr_invalidate_tbl_idx(pipe_sess_hdl_t sess_hdl,
                                          bf_dev_id_t dev_id,
                                          bf_dev_pipe_t pipe_id,
                                          pipe_tbl_hdl_t tbl_hdl,
                                          uint32_t entry_index);

#endif
