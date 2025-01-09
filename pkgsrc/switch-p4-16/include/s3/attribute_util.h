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


#ifndef INCLUDE_S3_ATTRIBUTE_UTIL_H__
#define INCLUDE_S3_ATTRIBUTE_UTIL_H__

#include <functional>
#include <string>

#include "bf_switch/bf_switch_types.h"

inline bool operator!=(const switch_object_id_t &lhs,
                       const switch_object_id_t &rhs) {
  return !(lhs == rhs);
}
inline bool operator==(const switch_object_id_t &lhs, const uint64_t rhs) {
  return lhs.data == rhs;
}
inline bool operator!=(const switch_object_id_t &lhs, const uint64_t rhs) {
  return lhs.data != rhs;
}

inline bool operator<(const switch_object_id_t &lhs,
                      const switch_object_id_t &rhs) {
  return lhs.data < rhs.data;
}

namespace smi {
namespace attr_util {

switch_status_t parse_mac(const std::string &str, switch_mac_addr_t &mac);
switch_status_t parse_ip_address(const std::string &str,
                                 switch_ip_address_t &ip);
switch_status_t parse_ip_prefix(const std::string &str,
                                switch_ip_prefix_t &prefix);
bool is_dir_bcast_addr(uint16_t prefix_len,
                       const switch_ip_address_t &switch_ip_addr);

template <typename T>
switch_status_t v_get(const switch_attribute_value_t &value, T &val);
template <typename T>
void v_set(switch_attribute_value_t &value, const T val);

class object_and_attribute_t {
 public:
  object_and_attribute_t(switch_object_id_t _oid, switch_attr_id_t _attr_id)
      : oid(_oid), attr_id(_attr_id) {}
  switch_object_id_t oid;
  switch_attr_id_t attr_id;

  inline bool operator==(const object_and_attribute_t &other) const {
    return (attr_id == other.attr_id && oid == other.oid);
  }
};

}  // namespace attr_util
}  // namespace smi
#endif  // INCLUDE_S3_ATTRIBUTE_UTIL_H__
