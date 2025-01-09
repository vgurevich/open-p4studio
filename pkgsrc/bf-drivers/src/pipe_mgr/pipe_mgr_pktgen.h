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


#ifndef _PIPE_MGR_PKTGEN_H
#define _PIPE_MGR_PKTGEN_H

#include <pipe_mgr/pktgen_intf.h>
#include "pipe_mgr_int.h"

/* all chips */
typedef struct pipe_mgr_pg_dev_ctx {
  union {
    struct pipe_mgr_tof_pg_dev_ctx *tof_ctx;
    struct pipe_mgr_tof2_pg_dev_ctx *tof2_ctx;
    struct pipe_mgr_tof3_pg_dev_ctx *tof3_ctx;
  } u;
} pipe_mgr_pg_dev_ctx;

/* Functions */
bf_status_t pipe_mgr_pktgen_port_add(rmt_dev_info_t *dev_info,
                                     bf_dev_port_t port_id,
                                     bf_port_speeds_t speed);
bf_status_t pipe_mgr_pktgen_port_rem(rmt_dev_info_t *dev_info,
                                     bf_dev_port_t port_id);
bf_status_t pipe_mgr_pktgen_add_dev(bf_session_hdl_t shdl, bf_dev_id_t dev);
bf_status_t pipe_mgr_pktgen_rmv_dev(bf_dev_id_t dev);
bf_status_t pipe_mgr_pktgen_warm_init_quick(bf_session_hdl_t shdl,
                                            bf_dev_id_t dev);
void pipe_mgr_pktgen_txn_commit(bf_dev_id_t dev);
void pipe_mgr_pktgen_txn_abort(bf_dev_id_t dev);

uint32_t pipe_mgr_pktgen_get_app_count(bf_dev_id_t dev);
bf_status_t pipe_mgr_pktgen_buffer_write_from_shadow(bf_session_hdl_t shdl,
                                                     bf_dev_target_t dev_tgt);
bf_status_t pipe_mgr_pktgen_create_dma(bf_session_hdl_t shdl,
                                       rmt_dev_info_t *dev_info);
#endif
