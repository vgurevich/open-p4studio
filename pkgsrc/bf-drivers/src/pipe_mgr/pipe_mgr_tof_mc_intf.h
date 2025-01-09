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


#ifndef __PIPE_MGR_TOF_MC_INTF_H__
#define __PIPE_MGR_TOF_MC_INTF_H__

pipe_status_t pipe_mgr_tof_mc_mgid_grp_addr_get(int mgid_grp,
                                                uint32_t *tbl0_addr,
                                                uint32_t *tbl1_addr);

pipe_status_t pipe_mgr_tof_mc_copy_to_cpu_pv_addr_get(bf_dev_pipe_t pipe,
                                                      uint32_t *addr);

pipe_status_t pipe_mgr_tof_mc_pv_table0_addr_get(bf_dev_pipe_t pipe,
                                                 int mgid_grp,
                                                 uint32_t *addr);

pipe_status_t pipe_mgr_tof_mc_yid_tbl_addr_get(uint32_t *addr);

#endif
