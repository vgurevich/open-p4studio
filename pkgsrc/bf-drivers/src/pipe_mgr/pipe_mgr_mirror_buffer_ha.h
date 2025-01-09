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


#ifndef _PIPE_MGR_MIRROR_BUFFER_HA_H_
#define _PIPE_MGR_MIRROR_BUFFER_HA_H_

#include "pipe_mgr/pipe_mgr_mirror_intf.h"
#include "pipe_mgr_mirror_buffer_comm.h"
#include "pipe_mgr_mirror_buffer.h"

pipe_status_t pipe_mgr_mirror_hitless_ha_init(pipe_sess_hdl_t sess_hdl,
                                              rmt_dev_info_t *dev_info);
pipe_status_t pipe_mgr_mirror_ha_compute_delta_changes(
    pipe_sess_hdl_t shdl, rmt_dev_info_t *dev_info);
#endif  // _PIPE_MGR_MIRROR_BUFFER_HA_H_
