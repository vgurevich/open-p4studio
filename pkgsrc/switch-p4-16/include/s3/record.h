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


#ifndef INCLUDE_S3_RECORD_H__
#define INCLUDE_S3_RECORD_H__

#include "bf_switch/bf_switch_types.h"

#ifdef __cplusplus
#include <set>
#include <string>
namespace smi {
namespace record {
void record_file_init(std::string record_file);
void record_file_clean(void);
void record_file_replay(std::string replay_file);
void record_comment_mode_set(bool on);
void record_add_create(const switch_object_type_t object_type,
                       const std::set<attr_w> &attrs,
                       switch_object_id_t object_id,
                       switch_status_t status);
void record_add_set(switch_object_id_t object_id,
                    const attr_w &attr,
                    switch_status_t status);
void record_add_get(switch_object_id_t object_id,
                    const switch_attr_id_t attr_id,
                    const attr_w &attr,
                    switch_status_t status);
void record_add_remove(switch_object_id_t object_id, switch_status_t status);
void record_add_notify(std::string notif);
void record_comment_mode_set(bool on);
bool record_comment_mode_get(void);
} /* namespace record */
} /* namespace smi */
#endif  /* __cplusplus */
#endif  // INCLUDE_S3_RECORD_H__
