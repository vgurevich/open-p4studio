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


#include "acl.h"
#ifndef __SMI_HOSTIF_TRAP_H
#define __SMI_HOSTIF_TRAP_H

namespace smi {

#define SWITCH_HOSTIF_TRAP_EXCLUSION_PORT_LIST_VALUE(exclusion_id)         \
  exclusion_id                                                             \
      ? (1 << (SWITCH_ACL_EXCLUDE_PORT_LIST_LABEL_POS + exclusion_id - 1)) \
      : 0

class ExclusionListManager {
 private:
  std::unique_ptr<idAllocator> trap_exclusion_port_list_ids;
  std::map<std::set<switch_object_id_t>, uint8_t> _trap_exclusion_id_map;
  uint8_t
      _trap_exclusion_id_refcnt[SWITCH_ACL_EXCLUDE_PORT_LIST_LABEL_WIDTH + 1];

 public:
  ExclusionListManager() {
    trap_exclusion_port_list_ids = std::unique_ptr<idAllocator>(
        new idAllocator(SWITCH_ACL_EXCLUDE_PORT_LIST_LABEL_WIDTH));
  }
  static ExclusionListManager &instance() {
    static ExclusionListManager _instance;
    return _instance;
  }
  switch_status_t port_exclusion_list_id_allocate(
      const std::set<switch_object_id_t> port_handles, uint8_t &exclusion_id);
  switch_status_t port_exclusion_list_id_reserve(
      const std::set<switch_object_id_t> port_handles, uint8_t exclusion_id);
  switch_status_t port_exclusion_list_id_release(
      const std::set<switch_object_id_t> &port_handles);
  switch_status_t port_exclusion_list_id_release(const uint8_t exclusion_id);
  switch_status_t port_exclusion_list_refcount(const uint8_t exclusion_id,
                                               uint32_t &ref_count);
};

}  // namespace smi

#endif /* __SMI_ACL_H__ */
