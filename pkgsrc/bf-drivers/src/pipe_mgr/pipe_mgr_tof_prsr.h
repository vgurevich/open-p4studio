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
 * @file pipe_mgr_tof_prsr.h
 * @date
 *
 * Implementation/Configuration of Ingress parser, parser-merge and Egress
 * parser based on port speed.
 */
#ifndef __PIPE_MGR_TOF_PRSR_H__
#define __PIPE_MGR_TOF_PRSR_H__

#include <pipe_mgr/pipe_mgr_intf.h>

pipe_status_t pipe_mgr_tof_iprsr_port_speed_based_cfg(rmt_dev_info_t *dev_info,
                                                      bf_dev_port_t port_id);

pipe_status_t pipe_mgr_tof_eprsr_port_speed_based_cfg(rmt_dev_info_t *dev_info,
                                                      bf_dev_port_t port_id);

pipe_status_t pipe_mgr_tof_eprsr_complete_port_mode_transition_wa(
    rmt_dev_info_t *dev_info, bf_dev_port_t port_id);

pipe_status_t pipe_mgr_tof_iprsr_get_pri_thresh(rmt_dev_info_t *dev_info,
                                                rmt_port_info_t *port_info,
                                                uint32_t *val);

pipe_status_t pipe_mgr_tof_iprsr_set_pri_thresh(rmt_dev_info_t *dev_info,
                                                rmt_port_info_t *port_info,
                                                uint32_t val);
#endif
