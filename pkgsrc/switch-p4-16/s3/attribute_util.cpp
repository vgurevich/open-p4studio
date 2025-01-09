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


#include "s3/attribute_util.h"

#include <arpa/inet.h>

#include <algorithm>
#include <string>
#include <vector>
#include <functional>

#include "s3/meta/meta.h"
#include "s3/attribute.h"
#include "./log.h"
#include "./third_party/fmtlib/fmt/format.h"

namespace smi {
namespace attr_util {
using ::smi::logging::switch_log;

switch_status_t parse_mac(const std::string &mac_str, switch_mac_addr_t &mac) {
  unsigned int mac_tmp[6];
  if (std::sscanf(mac_str.c_str(),
                  "%x:%x:%x:%x:%x:%x",
                  &mac_tmp[0],
                  &mac_tmp[1],
                  &mac_tmp[2],
                  &mac_tmp[3],
                  &mac_tmp[4],
                  &mac_tmp[5]) != 6) {
    smi::logging::switch_log(SWITCH_API_LEVEL_ERROR,
                             0,
                             "Failed to parse MAC from string: {}",
                             mac_str);
    return SWITCH_STATUS_FAILURE;
  }
  for (size_t i = 0; i < 6; i++) {
    if (mac_tmp[i] > UINT8_MAX) {
      smi::logging::switch_log(
          SWITCH_API_LEVEL_ERROR, 0, "Invalid MAC string: {}", mac_str);
      return SWITCH_STATUS_INVALID_PARAMETER;
    }
    mac.mac[i] = static_cast<char>(mac_tmp[i]);
  }
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t parse_ip_address(const std::string &str,
                                 switch_ip_address_t &ip) {
  struct sockaddr_in sa = {};
  struct sockaddr_in6 sa6 = {};
  auto rc =
      inet_pton(AF_INET, str.c_str(), static_cast<void *>(&(sa.sin_addr)));
  if (rc == 1) {
    ip.ip4 = htonl(sa.sin_addr.s_addr);
    ip.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
  } else {
    // try ipv6
    rc =
        inet_pton(AF_INET6, str.c_str(), static_cast<void *>(&(sa6.sin6_addr)));
    if (rc != 1) {
      smi::logging::switch_log(
          SWITCH_API_LEVEL_ERROR, 0, "Invalid IP address string: {}", str);
      return SWITCH_STATUS_INVALID_PARAMETER;
    }
    ip.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6;
    for (size_t i = 0; i < sizeof(struct in6_addr); i++) {
      ip.ip6[i] = sa6.sin6_addr.s6_addr[i];
    }
  }
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t parse_ip_prefix(const std::string &str,
                                switch_ip_prefix_t &prefix) {
  std::string ip_addr_str, len_str;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  // find and split into two substrings
  auto delimiter = str.find_first_of("/", 0);
  if (delimiter != str.npos) {
    ip_addr_str = str.substr(0, delimiter);
    len_str = str.substr(delimiter + 1, str.npos - delimiter);
  } else {
    ip_addr_str = str;
  }

  switch_ip_address_t ip_addr = {};
  status = parse_ip_address(ip_addr_str, ip_addr);
  if (status != SWITCH_STATUS_SUCCESS) {
    smi::logging::switch_log(
        SWITCH_API_LEVEL_ERROR, 0, "Invalid IP prefix string: {}", str);
    return status;
  }
  if (len_str.empty()) {
    if (ip_addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6) {
      len_str = "128";
    } else {
      len_str = "32";
    }
  }
  prefix.addr = ip_addr;
  prefix.len = std::stoul(len_str, nullptr, 10);
  return status;
}

bool is_dir_bcast_addr(uint16_t prefix_len,
                       const switch_ip_address_t &switch_ip_addr) {
  if ((switch_ip_addr.addr_family != SWITCH_IP_ADDR_FAMILY_IPV4) ||
      (prefix_len > 30)) {
    // There is no dir_bcast address for subnets with 31 and 32 bits prefix
    // length
    return false;
  }

  // Fill the hosts bits with 1s
  switch_ip4_t inverse_mask =
      (1 << (SWITCH_IPV4_MAX_PREFIX_LEN - prefix_len)) - 1;

  return ((switch_ip_addr.ip4 & inverse_mask) == inverse_mask);
}

template <>
switch_status_t v_get(const switch_attribute_value_t &value,
                      switch_enum_t &val_out) {
  const switch_status_t status = v_enum_get(&value, &val_out);
  CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);
  return status;
}

template <>
switch_status_t v_get(const switch_attribute_value_t &value,
                      uint64_t &val_out) {
  const switch_status_t status = v_u64_get(&value, &val_out);
  CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);
  return status;
}

template <>
switch_status_t v_get(const switch_attribute_value_t &value, int64_t &val_out) {
  const switch_status_t status = v_s64_get(&value, &val_out);
  CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);
  return status;
}

template <>
switch_status_t v_get(const switch_attribute_value_t &value,
                      uint32_t &val_out) {
  const switch_status_t status = v_u32_get(&value, &val_out);
  CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);
  return status;
}

template <>
switch_status_t v_get(const switch_attribute_value_t &value,
                      uint16_t &val_out) {
  const switch_status_t status = v_u16_get(&value, &val_out);
  CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);
  return status;
}

template <>
switch_status_t v_get(const switch_attribute_value_t &value, uint8_t &val_out) {
  const switch_status_t status = v_u8_get(&value, &val_out);
  CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);
  return status;
}

template <>
switch_status_t v_get(const switch_attribute_value_t &value,
                      switch_mac_addr_t &val_out) {
  const switch_status_t status = v_mac_get(&value, &val_out);
  CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);
  return status;
}

template <>
switch_status_t v_get(const switch_attribute_value_t &value,
                      switch_string_t &val_out) {
  const switch_status_t status = v_string_get(&value, &val_out);
  CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);
  return status;
}

template <>
switch_status_t v_get(const switch_attribute_value_t &value,
                      switch_ip_address_t &val_out) {
  const switch_status_t status = v_ipaddr_get(&value, &val_out);
  CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);
  return status;
}

template <>
switch_status_t v_get(const switch_attribute_value_t &value,
                      switch_ip_prefix_t &val_out) {
  const switch_status_t status = v_ipprefix_get(&value, &val_out);
  CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);
  return status;
}

template <>
switch_status_t v_get(const switch_attribute_value_t &value, bool &val_out) {
  const switch_status_t status = v_bool_get(&value, &val_out);
  CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);
  return status;
}

template <>
switch_status_t v_get(const switch_attribute_value_t &value,
                      switch_object_id_t &val_out) {
  const switch_status_t status = v_oid_get(&value, &val_out);
  CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);
  return status;
}

template <>
switch_status_t v_get(const switch_attribute_value_t &value,
                      switch_range_t &val_out) {
  const switch_status_t status = v_range_get(&value, &val_out);
  CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);
  return status;
}

template <>
void v_set(switch_attribute_value_t &value, const int64_t val) {
  v_s64_set(&value, val);
}

template <>
void v_set(switch_attribute_value_t &value, const uint64_t val) {
  v_u64_set(&value, val);
}

template <>
void v_set(switch_attribute_value_t &value, const uint32_t val) {
  v_u32_set(&value, val);
}

template <>
void v_set(switch_attribute_value_t &value, const switch_mac_addr_t val) {
  v_mac_set(&value, val);
}

template <>
void v_set(switch_attribute_value_t &value, const switch_string_t val) {
  v_string_set(&value, val);
}

template <>
void v_set(switch_attribute_value_t &value, const switch_ip_address_t val) {
  v_ipaddr_set(&value, val);
}

template <>
void v_set(switch_attribute_value_t &value, const switch_ip_prefix_t val) {
  v_ipprefix_set(&value, val);
}
template <>
void v_set(switch_attribute_value_t &value, const switch_enum_t val) {
  v_enum_set(&value, val);
}

template <>
void v_set(switch_attribute_value_t &value, const uint16_t val) {
  v_u16_set(&value, val);
}

template <>
void v_set(switch_attribute_value_t &value, const uint8_t val) {
  v_u8_set(&value, val);
}

template <>
void v_set(switch_attribute_value_t &value, const switch_object_id_t val) {
  v_oid_set(&value, val);
}

template <>
void v_set(switch_attribute_value_t &value, const bool val) {
  v_bool_set(&value, val);
}

template <>
void v_set(switch_attribute_value_t &value, const switch_range_t val) {
  v_range_set(&value, val);
}
}  // namespace attr_util

void attr_w::attr_to_string(std::string &str) const {
  const switch_attribute_t &attr = getattr();

  switch (type_get()) {
    case SWITCH_TYPE_BOOL:
      str += fmt::to_string(attr.value.booldata);
      return;
    case SWITCH_TYPE_UINT8:
      str += fmt::to_string(attr.value.u8);
      return;
    case SWITCH_TYPE_UINT16:
      str += fmt::to_string(attr.value.u16);
      return;
    case SWITCH_TYPE_UINT32:
      str += fmt::to_string(attr.value.u32);
      return;
    case SWITCH_TYPE_UINT64:
      str += fmt::to_string(attr.value.u64);
      return;
    case SWITCH_TYPE_INT64:
      str += fmt::to_string(attr.value.s64);
      return;
    case SWITCH_TYPE_ENUM:
      str += fmt::to_string(attr.value.enumdata.enumdata);
      return;
    case SWITCH_TYPE_OBJECT_ID:
      str += fmt::to_string(attr.value.oid.data);
      return;
    case SWITCH_TYPE_STRING:
      str += attr.value.text.text;
      return;
    case SWITCH_TYPE_RANGE:
      str += fmt::format("{}-{}", attr.value.range.min, attr.value.range.max);
      return;
    case SWITCH_TYPE_MAC:
      str += fmt::format("{:x}:{:x}:{:x}:{:x}:{:x}:{:x}",
                         attr.value.mac.mac[0],
                         attr.value.mac.mac[1],
                         attr.value.mac.mac[2],
                         attr.value.mac.mac[3],
                         attr.value.mac.mac[4],
                         attr.value.mac.mac[5]);
      return;
    case SWITCH_TYPE_IP_ADDRESS:
      if (attr.value.ipaddr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4) {
        struct in_addr ip_addr;
        ip_addr.s_addr = htonl(attr.value.ipaddr.ip4);
        str += inet_ntoa(ip_addr);
      } else if (attr.value.ipaddr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6) {
        char ipv6_str[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, attr.value.ipaddr.ip6, ipv6_str, INET6_ADDRSTRLEN);
        str += ipv6_str;
      } else {
        return;
      }
      return;
    case SWITCH_TYPE_IP_PREFIX:
      if (attr.value.ipprefix.addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4) {
        struct in_addr ip_addr;
        ip_addr.s_addr = htonl(attr.value.ipprefix.addr.ip4);
        str += inet_ntoa(ip_addr);
        str += fmt::format("/{} ", attr.value.ipprefix.len);
      } else if (attr.value.ipprefix.addr.addr_family ==
                 SWITCH_IP_ADDR_FAMILY_IPV6) {
        char ipv6_str[INET6_ADDRSTRLEN];
        inet_ntop(
            AF_INET6, attr.value.ipprefix.addr.ip6, ipv6_str, INET6_ADDRSTRLEN);
        str += ipv6_str;
        str += fmt::format("/{} ", attr.value.ipprefix.len);
      } else {
        return;
      }
      return;
    case SWITCH_TYPE_LIST: {
      for (auto b : this->mlist) {
        switch (list_type_get()) {
          case SWITCH_TYPE_BOOL:
            str += b.booldata ? "true " : "false ";
            break;
          case SWITCH_TYPE_UINT8:
            str += fmt::format("{} ", b.u8);
            break;
          case SWITCH_TYPE_UINT16:
            str += fmt::format("{} ", b.u16);
            break;
          case SWITCH_TYPE_UINT32:
            str += fmt::format("{} ", b.u32);
            break;
          case SWITCH_TYPE_UINT64:
            str += fmt::format("{} ", b.u64);
            break;
          case SWITCH_TYPE_INT64:
            str += fmt::format("{} ", b.s64);
            break;
          case SWITCH_TYPE_OBJECT_ID:
            str += fmt::format("{} ", b.oid.data & 0xffffffff);
            break;
          case SWITCH_TYPE_MAC:
            str += fmt::format("{:x}:{:x}:{:x}:{:x}:{:x}:{:x} ",
                               b.mac.mac[0],
                               b.mac.mac[1],
                               b.mac.mac[2],
                               b.mac.mac[3],
                               b.mac.mac[4],
                               b.mac.mac[5]);
            break;
          case SWITCH_TYPE_IP_ADDRESS:
            if (b.ipaddr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4) {
              struct in_addr ip_addr;
              ip_addr.s_addr = htonl(b.ipaddr.ip4);
              str += inet_ntoa(ip_addr);
              str += " ";
            } else if (b.ipaddr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6) {
              char ipv6_str[INET6_ADDRSTRLEN];
              inet_ntop(AF_INET6, b.ipaddr.ip6, ipv6_str, INET6_ADDRSTRLEN);
              str += ipv6_str;
              str += " ";
            }
            break;
          case SWITCH_TYPE_IP_PREFIX:
            if (b.ipprefix.addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4) {
              struct in_addr ip_addr;
              ip_addr.s_addr = htonl(b.ipprefix.addr.ip4);
              str += inet_ntoa(ip_addr);
              str += fmt::format("/{} ", b.ipprefix.len);
            } else if (b.ipprefix.addr.addr_family ==
                       SWITCH_IP_ADDR_FAMILY_IPV6) {
              char ipv6_str[INET6_ADDRSTRLEN];
              inet_ntop(
                  AF_INET6, b.ipprefix.addr.ip6, ipv6_str, INET6_ADDRSTRLEN);
              str += ipv6_str;
              str += fmt::format("/{} ", b.ipprefix.len);
            }
            break;
          case SWITCH_TYPE_ENUM:
            str += fmt::format("{} ", b.enumdata.enumdata);
            break;
          default:
            break;
        }
      }
    }
      return;
    default:
      smi::logging::switch_log(
          SWITCH_API_LEVEL_ERROR, 0, "unexpected type {}", type_get());
      return;
  }

  return;
}

template <typename T>
switch_status_t attr_w::v_get(T &val) const {
  return attr_util::v_get(this->m_attr.value, val);
}

template switch_status_t attr_w::v_get(switch_object_id_t &val) const;
template switch_status_t attr_w::v_get(uint64_t &val) const;
template switch_status_t attr_w::v_get(int64_t &val) const;
template switch_status_t attr_w::v_get(uint32_t &val) const;
template switch_status_t attr_w::v_get(uint16_t &val) const;
template switch_status_t attr_w::v_get(uint8_t &val) const;
template switch_status_t attr_w::v_get(switch_mac_addr_t &val) const;
template switch_status_t attr_w::v_get(switch_string_t &val) const;
template switch_status_t attr_w::v_get(switch_ip_address_t &val) const;
template switch_status_t attr_w::v_get(switch_ip_prefix_t &val) const;
template switch_status_t attr_w::v_get(bool &val) const;
template switch_status_t attr_w::v_get(switch_enum_t &val) const;
template switch_status_t attr_w::v_get(switch_range_t &val) const;

template <typename T>
void attr_w::v_set(const T val) {
  attr_util::v_set(this->m_attr.value, val);
}
template void attr_w::v_set(const switch_object_id_t val);
template void attr_w::v_set(const uint8_t val);
template void attr_w::v_set(const uint16_t val);
template void attr_w::v_set(const uint32_t val);
template void attr_w::v_set(const uint64_t val);
template void attr_w::v_set(const int64_t val);
template void attr_w::v_set(const bool val);
template void attr_w::v_set(const switch_mac_addr_t val);
template void attr_w::v_set(const switch_string_t val);
template void attr_w::v_set(const switch_ip_address_t val);
template void attr_w::v_set(const switch_ip_prefix_t val);
template void attr_w::v_set(const switch_enum_t val);
template void attr_w::v_set(const switch_range_t val);

template <>
switch_status_t attr_w::v_get(std::vector<bool> &val) const {
  for (const auto &b : this->mlist) val.push_back(b.booldata);
  return SWITCH_STATUS_SUCCESS;
}
template <>
switch_status_t attr_w::v_get(std::vector<uint8_t> &val) const {
  for (const auto &u8 : this->mlist) val.push_back(u8.u8);
  return SWITCH_STATUS_SUCCESS;
}
template <>
switch_status_t attr_w::v_get(std::vector<uint16_t> &val) const {
  for (const auto &u16 : this->mlist) val.push_back(u16.u16);
  return SWITCH_STATUS_SUCCESS;
}
template <>
switch_status_t attr_w::v_get(std::vector<uint32_t> &val) const {
  for (const auto &u32 : this->mlist) val.push_back(u32.u32);
  return SWITCH_STATUS_SUCCESS;
}
template <>
switch_status_t attr_w::v_get(std::vector<uint64_t> &val) const {
  for (const auto &u64 : this->mlist) val.push_back(u64.u64);
  return SWITCH_STATUS_SUCCESS;
}
template <>
switch_status_t attr_w::v_get(std::vector<int64_t> &val) const {
  for (const auto &s64 : this->mlist) val.push_back(s64.s64);
  return SWITCH_STATUS_SUCCESS;
}
template <>
switch_status_t attr_w::v_get(std::vector<switch_string_t> &val) const {
  for (const auto &text : this->mlist) val.push_back(text.text);
  return SWITCH_STATUS_SUCCESS;
}
template <>
switch_status_t attr_w::v_get(std::vector<switch_object_id_t> &val) const {
  for (const auto &oid : this->mlist) val.push_back(oid.oid);
  return SWITCH_STATUS_SUCCESS;
}
template <>
switch_status_t attr_w::v_get(std::vector<switch_mac_addr_t> &val) const {
  for (const auto &mac : this->mlist) val.push_back(mac.mac);
  return SWITCH_STATUS_SUCCESS;
}
template <>
switch_status_t attr_w::v_get(std::vector<switch_ip_address_t> &val) const {
  for (const auto &ip : this->mlist) val.push_back(ip.ipaddr);
  return SWITCH_STATUS_SUCCESS;
}
template <>
switch_status_t attr_w::v_get(std::vector<switch_ip_prefix_t> &val) const {
  for (const auto &ip : this->mlist) val.push_back(ip.ipprefix);
  return SWITCH_STATUS_SUCCESS;
}
template <>
switch_status_t attr_w::v_get(std::vector<switch_enum_t> &val) const {
  for (const auto &e : this->mlist) val.push_back(e.enumdata);
  return SWITCH_STATUS_SUCCESS;
}
template <>
switch_status_t attr_w::v_get(std::vector<switch_range_t> &val) const {
  (void)val;
  smi::logging::switch_log(SWITCH_API_LEVEL_ERROR,
                           "Range vector get not supported");
  return SWITCH_STATUS_INVALID_PARAMETER;
}

template <>
void attr_w::v_set(const std::vector<bool> val) {
  this->m_attr.value.type = SWITCH_TYPE_LIST;
  this->m_attr.value.list.count = val.size();
  this->list_type = SWITCH_TYPE_BOOL;
  for (const auto &b : val) {
    switch_attribute_value_t value = {};
    attr_util::v_set(value, b);
    this->mlist.push_back(value);
  }
  return;
}
template <>
void attr_w::v_set(const std::vector<uint8_t> val) {
  this->m_attr.value.type = SWITCH_TYPE_LIST;
  this->m_attr.value.list.count = val.size();
  this->list_type = SWITCH_TYPE_UINT8;
  for (const auto &b : val) {
    switch_attribute_value_t value = {};
    attr_util::v_set(value, b);
    this->mlist.push_back(value);
  }
  return;
}
template <>
void attr_w::v_set(const std::vector<uint16_t> val) {
  this->m_attr.value.type = SWITCH_TYPE_LIST;
  this->m_attr.value.list.count = val.size();
  this->list_type = SWITCH_TYPE_UINT16;
  for (const auto &b : val) {
    switch_attribute_value_t value = {};
    attr_util::v_set(value, b);
    this->mlist.push_back(value);
  }
  return;
}
template <>
void attr_w::v_set(const std::vector<uint32_t> val) {
  this->m_attr.value.type = SWITCH_TYPE_LIST;
  this->m_attr.value.list.count = val.size();
  this->list_type = SWITCH_TYPE_UINT32;
  for (const auto &b : val) {
    switch_attribute_value_t value = {};
    attr_util::v_set(value, b);
    this->mlist.push_back(value);
  }
  return;
}
template <>
void attr_w::v_set(const std::vector<uint64_t> val) {
  this->m_attr.value.type = SWITCH_TYPE_LIST;
  this->m_attr.value.list.count = val.size();
  this->list_type = SWITCH_TYPE_UINT64;
  for (const auto &b : val) {
    switch_attribute_value_t value = {};
    attr_util::v_set(value, b);
    this->mlist.push_back(value);
  }
  return;
}
template <>
void attr_w::v_set(const std::vector<int64_t> val) {
  this->m_attr.value.type = SWITCH_TYPE_LIST;
  this->m_attr.value.list.count = val.size();
  this->list_type = SWITCH_TYPE_INT64;
  for (const auto &b : val) {
    switch_attribute_value_t value = {};
    attr_util::v_set(value, b);
    this->mlist.push_back(value);
  }
  return;
}
template <>
void attr_w::v_set(const std::vector<switch_string_t> val) {
  this->m_attr.value.type = SWITCH_TYPE_LIST;
  this->m_attr.value.list.count = val.size();
  this->list_type = SWITCH_TYPE_STRING;
  for (const auto &b : val) {
    switch_attribute_value_t value = {};
    attr_util::v_set(value, b);
    this->mlist.push_back(value);
  }
  return;
}
template <>
void attr_w::v_set(const std::vector<switch_object_id_t> val) {
  this->m_attr.value.type = SWITCH_TYPE_LIST;
  this->m_attr.value.list.count = val.size();
  this->list_type = SWITCH_TYPE_OBJECT_ID;
  for (const auto &b : val) {
    switch_attribute_value_t value = {};
    attr_util::v_set(value, b);
    this->mlist.push_back(value);
  }
  return;
}
template <>
void attr_w::v_set(const std::vector<switch_mac_addr_t> val) {
  this->m_attr.value.type = SWITCH_TYPE_LIST;
  this->m_attr.value.list.count = val.size();
  this->list_type = SWITCH_TYPE_MAC;
  for (const auto &b : val) {
    switch_attribute_value_t value = {};
    attr_util::v_set(value, b);
    this->mlist.push_back(value);
  }
  return;
}
template <>
void attr_w::v_set(const std::vector<switch_ip_address_t> val) {
  this->m_attr.value.type = SWITCH_TYPE_LIST;
  this->m_attr.value.list.count = val.size();
  this->list_type = SWITCH_TYPE_IP_ADDRESS;
  for (const auto &b : val) {
    switch_attribute_value_t value = {};
    attr_util::v_set(value, b);
    this->mlist.push_back(value);
  }
  return;
}
template <>
void attr_w::v_set(const std::vector<switch_ip_prefix_t> val) {
  this->m_attr.value.type = SWITCH_TYPE_LIST;
  this->m_attr.value.list.count = val.size();
  this->list_type = SWITCH_TYPE_IP_PREFIX;
  for (const auto &b : val) {
    switch_attribute_value_t value = {};
    attr_util::v_set(value, b);
    this->mlist.push_back(value);
  }
  return;
}
template <>
void attr_w::v_set(const std::vector<switch_enum_t> val) {
  this->m_attr.value.type = SWITCH_TYPE_LIST;
  this->m_attr.value.list.count = val.size();
  this->list_type = SWITCH_TYPE_ENUM;
  for (const auto &b : val) {
    switch_attribute_value_t value = {};
    attr_util::v_set(value, b);
    this->mlist.push_back(value);
  }
  return;
}
template <>
void attr_w::v_set(const std::vector<switch_range_t> val) {
  (void)val;
  smi::logging::switch_log(SWITCH_API_LEVEL_ERROR,
                           "Range vector set not supported");
  return;
}

template <typename T>
attr_w::attr_w(const switch_attr_id_t attr_id, const T val) {
  memset(&m_attr, 0, sizeof(switch_attribute_t));
  attr_w::id_set(attr_id);
  attr_w::v_set(val);
}
template <typename T>
attr_w::attr_w(const switch_attr_id_t attr_id, const std::vector<T> val) {
  memset(&m_attr, 0, sizeof(switch_attribute_t));
  attr_w::id_set(attr_id);
  attr_w::v_set(val);
}
// simple types
template attr_w::attr_w(const switch_attr_id_t attr_id,
                        const switch_object_id_t val);
template attr_w::attr_w(const switch_attr_id_t attr_id, const uint64_t val);
template attr_w::attr_w(const switch_attr_id_t attr_id, const uint32_t val);
template attr_w::attr_w(const switch_attr_id_t attr_id, const uint16_t val);
template attr_w::attr_w(const switch_attr_id_t attr_id, const uint8_t val);
template attr_w::attr_w(const switch_attr_id_t attr_id, const int64_t val);
template attr_w::attr_w(const switch_attr_id_t attr_id, const bool val);
template attr_w::attr_w(const switch_attr_id_t attr_id,
                        const switch_mac_addr_t val);
template attr_w::attr_w(const switch_attr_id_t attr_id,
                        const switch_string_t val);
template attr_w::attr_w(const switch_attr_id_t attr_id,
                        const switch_ip_address_t val);
template attr_w::attr_w(const switch_attr_id_t attr_id,
                        const switch_ip_prefix_t val);
template attr_w::attr_w(const switch_attr_id_t attr_id,
                        const switch_enum_t val);
template attr_w::attr_w(const switch_attr_id_t attr_id,
                        const switch_range_t val);
// list types
template attr_w::attr_w(const switch_attr_id_t attr_id,
                        const std::vector<switch_object_id_t> val);
template attr_w::attr_w(const switch_attr_id_t attr_id,
                        const std::vector<uint64_t> val);
template attr_w::attr_w(const switch_attr_id_t attr_id,
                        const std::vector<uint32_t> val);
template attr_w::attr_w(const switch_attr_id_t attr_id,
                        const std::vector<uint16_t> val);
template attr_w::attr_w(const switch_attr_id_t attr_id,
                        const std::vector<uint8_t> val);
template attr_w::attr_w(const switch_attr_id_t attr_id,
                        const std::vector<int64_t> val);
template attr_w::attr_w(const switch_attr_id_t attr_id,
                        const std::vector<bool> val);
template attr_w::attr_w(const switch_attr_id_t attr_id,
                        const std::vector<switch_mac_addr_t> val);
template attr_w::attr_w(const switch_attr_id_t attr_id,
                        const std::vector<switch_string_t> val);
template attr_w::attr_w(const switch_attr_id_t attr_id,
                        const std::vector<switch_ip_address_t> val);
template attr_w::attr_w(const switch_attr_id_t attr_id,
                        const std::vector<switch_ip_prefix_t> val);
template attr_w::attr_w(const switch_attr_id_t attr_id,
                        const std::vector<switch_enum_t> val);
template <>
attr_w::attr_w(const switch_attr_id_t attr_id,
               const std::vector<switch_range_t> val) {
  (void)attr_id;
  (void)val;
  memset(&m_attr, 0, sizeof(switch_attribute_t));
  attr_w::id_set(attr_id);
  smi::logging::switch_log(SWITCH_API_LEVEL_ERROR,
                           "Range vector is not supported");
}

switch_status_t attr_w::attr_import(const switch_attribute_t &attr) {
  const switch_status_t status = attr_copy(&m_attr, &attr);
  seed = std::hash<uint16_t>{}(m_attr.id);
  uint32_t i = 0;
  if (attr.value.type == SWITCH_TYPE_LIST) {
    const size_t n = attr.value.list.count;
    if (!n) return status;
    this->m_attr.value.type = SWITCH_TYPE_LIST;
    this->list_type = attr.value.list.list_type;
    switch_attribute_value_t *list = attr.value.list.list;
    while (i < n) this->mlist.push_back(list[i++]);
  }
  return status;
}

switch_status_t attr_w::attr_export(switch_attribute_t *attr) {
  const switch_status_t status = attr_copy(attr, &m_attr);
  uint32_t i = 0;
  if (this->m_attr.value.type == SWITCH_TYPE_LIST) {
    attr->value.type = SWITCH_TYPE_LIST;
    attr->value.list.list_type = list_type_get();
    attr->value.list.list = static_cast<switch_attribute_value_t *>(
        calloc(this->mlist.size(), sizeof(switch_attribute_value_t)));
    for (auto b : this->mlist) {
      switch (list_type_get()) {
        case SWITCH_TYPE_BOOL:
          attr_util::v_set(attr->value.list.list[i++], b.booldata);
          break;
        case SWITCH_TYPE_UINT8:
          attr_util::v_set(attr->value.list.list[i++], b.u8);
          break;
        case SWITCH_TYPE_UINT16:
          attr_util::v_set(attr->value.list.list[i++], b.u16);
          break;
        case SWITCH_TYPE_UINT32:
          attr_util::v_set(attr->value.list.list[i++], b.u32);
          break;
        case SWITCH_TYPE_UINT64:
          attr_util::v_set(attr->value.list.list[i++], b.u64);
          break;
        case SWITCH_TYPE_INT64:
          attr_util::v_set(attr->value.list.list[i++], b.s64);
          break;
        case SWITCH_TYPE_STRING:
          attr_util::v_set(attr->value.list.list[i++], b.text);
          break;
        case SWITCH_TYPE_IP_ADDRESS:
          attr_util::v_set(attr->value.list.list[i++], b.ipaddr);
          break;
        case SWITCH_TYPE_IP_PREFIX:
          attr_util::v_set(attr->value.list.list[i++], b.ipprefix);
          break;
        case SWITCH_TYPE_MAC:
          attr_util::v_set(attr->value.list.list[i++], b.mac);
          break;
        case SWITCH_TYPE_OBJECT_ID:
          attr_util::v_set(attr->value.list.list[i++], b.oid);
          break;
        default:
          break;
      }
    }
  }
  return status;
}
} /* namespace smi */
