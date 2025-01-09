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


#ifndef _PI_ALLOCATORS_H__
#define _PI_ALLOCATORS_H__

#include <PI/pi.h>

#include <pipe_mgr/pipe_mgr_intf.h>

void allocate_pipe_match_spec(pi_p4_id_t table_id,
                              const pi_p4info_t *p4info,
                              pipe_tbl_match_spec_t *pipe_match_spec);

void release_pipe_match_spec(pipe_tbl_match_spec_t *pipe_match_spec);

void allocate_pipe_action_data_spec(
    pi_p4_id_t action_id,
    const pi_p4info_t *p4info,
    pipe_action_data_spec_t *pipe_action_data_spec);

// allocate space to accomodate any action belonging to the table or action
// profile with the provided id
void allocate_pipe_action_data_spec_any(
    pi_p4_id_t id,
    const pi_p4info_t *p4info,
    pipe_action_data_spec_t *pipe_action_data_spec);

void release_pipe_action_data_spec(
    pipe_action_data_spec_t *pipe_action_data_spec);

void allocate_pi_match_key(pi_p4_id_t table_id,
                           const pi_p4info_t *p4info,
                           pi_match_key_t *match_key);

void release_pi_match_key(pi_match_key_t *match_key);

#endif  // _PI_ALLOCATORS_H__
