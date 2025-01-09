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
 * @file pipe_mgr_exm_hash.h
 * @date
 *
 *
 * Contains definitions for hash computation for exact match hash tables.
 */

#ifndef _PIPE_MGR_EXM_HASH_H
#define _PIPE_MGR_EXM_HASH_H

/* Module header includes */
#include <pipe_mgr/pipe_mgr_intf.h>

/* Local header includes */
//#include "pipe_mgr_exm_tbl_mgr.h"

pipe_status_t pipe_mgr_exm_hash_compute(bf_dev_id_t dev_id,
                                        profile_id_t profile_id,
                                        pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                        pipe_tbl_match_spec_t *pipe_match_spec,
                                        dev_stage_t stage_id,
                                        pipe_exm_hash_t *hash_container,
                                        uint32_t *num_entries);

uint32_t pipe_mgr_exm_extract_per_hashway_hash(pipe_exm_hash_t *hash,
                                               void *hdata,
                                               uint32_t *subword_loc);

pipe_status_t pipe_mgr_exm_proxy_hash_compute(bf_dev_id_t device_id,
                                              profile_id_t profile_id,
                                              pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                              pipe_tbl_match_spec_t *match_spec,
                                              dev_stage_t stage_id,
                                              uint64_t *proxy_hash);

pipe_status_t pipe_mgr_hash_init(void);

#endif
