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


#ifndef INCLUDE_S3_SMI_H__
#define INCLUDE_S3_SMI_H__

#include <set>
#include <vector>

#include "bf_switch/bf_switch_types.h"

/** @defgroup bf_smi SMI object API
 *  Main interface for S3
 *  API functions, enums and structures to create/delete/set/get objects
 *  @{
 */

/*******************************************************************************
 * C++ frontend APIs
 ******************************************************************************/
namespace smi {
namespace api {

switch_status_t smi_object_get(const switch_object_type_t object_type,
                               const std::set<attr_w> &attrs,
                               switch_object_id_t &object_id);
bool smi_object_exists_by_id(const switch_object_type_t object_type,
                             const uint64_t id);

switch_status_t smi_object_create(const switch_object_type_t object_type,
                                  const std::set<attr_w> &attrs,
                                  switch_object_id_t &object_id);

switch_status_t smi_object_create_by_id(const switch_object_type_t object_type,
                                        const std::set<attr_w> &attrs,
                                        const uint64_t id);
switch_status_t smi_object_delete(const switch_object_id_t object_id);

switch_status_t smi_object_delete_by_id(const switch_object_type_t object_type,
                                        const uint64_t id);

switch_status_t smi_attribute_set(const switch_object_id_t object_id,
                                  const attr_w &attr);

switch_status_t smi_attribute_get(const switch_object_id_t object_id,
                                  const switch_attr_id_t attr_id,
                                  attr_w &attr);

switch_status_t smi_object_counters_get(const switch_object_id_t object_handle,
                                        std::vector<switch_counter_t> &cntrs);

switch_status_t smi_object_counters_clear(
    const switch_object_id_t object_handle,
    const std::vector<uint16_t> &cntrs_ids);

switch_status_t smi_object_counters_clear_all(switch_object_id_t object_handle);

switch_status_t smi_object_flush_all(const switch_object_type_t object_type);

switch_status_t smi_object_get_first_handle(switch_object_type_t object_type,
                                            switch_object_id_t &object_handle);

switch_status_t smi_object_get_next_handles(
    switch_object_id_t object_handle,
    uint32_t in_num_handles,
    std::vector<switch_object_id_t> &next_handles,
    uint32_t &out_num_handles);

switch_status_t smi_object_get_all_handles(
    switch_object_type_t object_type,
    std::vector<switch_object_id_t> &object_handles);

switch_status_t smi_object_log_level_set(const switch_object_type_t object_type,
                                         const switch_verbosity_t verbosity);

switch_status_t smi_log_level_set(const switch_verbosity_t verbosity);

}  // namespace api
}  // namespace smi

/** @} */

#endif  // INCLUDE_S3_SMI_H__
