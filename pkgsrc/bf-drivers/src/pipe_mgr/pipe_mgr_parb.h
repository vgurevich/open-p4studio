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


#ifndef __PIPE_MGR_PARB_H__
#define __PIPE_MGR_PARB_H__

#include <pipe_mgr/pipe_mgr_intf.h>

pipe_status_t parb_set_port_ingress_chnl_control(bf_dev_id_t dev_id,
                                                 bf_dev_port_t port_id);

pipe_status_t parb_set_port_egress_chnl_control(bf_dev_id_t dev_id,
                                                bf_dev_port_t port_id);

pipe_status_t pipe_mgr_parb_pps_limit_set(pipe_sess_hdl_t shdl,
                                          rmt_dev_info_t *dev_info,
                                          bf_dev_pipe_t log_pipe,
                                          uint64_t pps);
pipe_status_t pipe_mgr_parb_pps_limit_get(rmt_dev_info_t *dev_info,
                                          bf_dev_pipe_t log_pipe,
                                          uint64_t *max_pps);
pipe_status_t pipe_mgr_parb_pps_limit_max_get(rmt_dev_info_t *dev_info,
                                              bf_dev_pipe_t log_pipe,
                                              uint64_t *max_pps);
pipe_status_t pipe_mgr_parb_pps_limit_reset(pipe_sess_hdl_t shdl,
                                            rmt_dev_info_t *dev_info,
                                            bf_dev_pipe_t log_pipe);
#endif
