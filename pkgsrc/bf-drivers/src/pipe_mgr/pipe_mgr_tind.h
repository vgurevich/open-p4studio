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
 * @file pipe_mgr_tcam.h
 * @date
 *
 * TCAM related definitions of pipeline manager
 */

#ifndef _PIPE_MGR_TIND_H_
#define _PIPE_MGR_TIND_H_

#include "pipe_mgr_tcam.h"

pipe_status_t pipe_mgr_tcam_tind_tbl_alloc(tcam_stage_info_t *stage_data,
                                           pipe_mat_tbl_info_t *mat_tbl_info);

bool pipe_mgr_tcam_tind_get_line_no(tcam_stage_info_t *stage_data,
                                    uint32_t physical_line_index,
                                    uint32_t *tind_line_no,
                                    uint32_t *tind_block,
                                    uint32_t *subword_pos);








bool pipe_mgr_tcam_tind_exists(tcam_stage_info_t *stage_data);

#endif
