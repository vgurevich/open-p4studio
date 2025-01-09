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


#ifndef _PIPE_MGR_PKTGEN_COMM_H
#define _PIPE_MGR_PKTGEN_COMM_H

#define PIPE_MGR_PKT_BUFFER_MEM_ROWS 1024
#define PIPE_MGR_PKT_BUFFER_WIDTH 16
#define PIPE_MGR_PKT_BUFFER_SIZE \
  (PIPE_MGR_PKT_BUFFER_MEM_ROWS * PIPE_MGR_PKT_BUFFER_WIDTH)
#define PIPE_MGR_PKTGEN_SRC_PRT_MAX 127
struct pkt_buffer_shadow_t {
  uint8_t data[PIPE_MGR_PKT_BUFFER_SIZE];
  uint8_t txn_data[PIPE_MGR_PKT_BUFFER_SIZE];
  bool txn_data_valid;
};

int pg_log_pipe_mask(bf_dev_target_t dev_tgt);
bf_status_t pg_write_one_pipe_reg(pipe_sess_hdl_t sid,
                                  bf_dev_id_t dev,
                                  uint8_t pm,
                                  uint32_t addr,
                                  uint32_t data);

#endif
