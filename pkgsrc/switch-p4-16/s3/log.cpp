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


#include <log.h>

#include <arpa/inet.h>

#include <iostream>
#include <string>
#include <ctime>
#include <set>
#include <vector>
#include <unordered_map>

#include "s3/switch_store.h"
#include "s3/meta/meta.h"
#include "s3/attribute.h"
#include "target-sys/bf_sal/bf_sys_intf.h"
#include "bf_switch/bf_switch_types.h"

bool operationSettings[4] = {true, true, true, true};
static switch_verbosity_t default_verbosity = SWITCH_API_LEVEL_ERROR;

static bool mapInitialized = false;
std::unordered_map<switch_object_type_t, switch_verbosity_t> ObjectVerbosityMap;

void toggle_operation(const switch_operation_t operation,
                      const bool isEnabled) {
  operationSettings[operation] = isEnabled;
}

void toggle_all_operations(bool isEnabled) {
  memset(operationSettings, isEnabled, sizeof(operationSettings));
}

inline uint64_t switch_to_bf_sys_level(switch_verbosity_t verbosity) {
  switch (verbosity) {
    case SWITCH_API_LEVEL_ERROR:
      return BF_LOG_ERR;
    case SWITCH_API_LEVEL_WARN:
      return BF_LOG_WARN;
    case SWITCH_API_LEVEL_INFO:
      return BF_LOG_INFO;
    case SWITCH_API_LEVEL_DEBUG:
      return BF_LOG_DBG;
    case SWITCH_API_LEVEL_DETAIL:
      return BF_LOG_DBG;
    default:
      return BF_LOG_ERR;
  }
}

void set_log_level(switch_verbosity_t new_verbosity) {
  default_verbosity = new_verbosity;
#ifndef TESTING
  bf_sys_trace_level_set(BF_MOD_SWITCHAPI,
                         switch_to_bf_sys_level(new_verbosity));
  bf_sys_log_level_set(BF_MOD_SWITCHAPI,
                       BF_LOG_DEST_FILE,
                       switch_to_bf_sys_level(new_verbosity));
  bf_sys_log_level_set(BF_MOD_SWITCHAPI,
                       BF_LOG_DEST_STDOUT,
                       switch_to_bf_sys_level(new_verbosity));
#endif
}

switch_verbosity_t get_log_level() { return default_verbosity; }

void set_log_level_object(switch_object_type_t object_type,
                          switch_verbosity_t new_verbosity) {
  if (object_type == 0) return;
  smi::ModelInfo *model_info = smi::switch_store::switch_model_info_get();
  if (ObjectVerbosityMap.find(object_type) != ObjectVerbosityMap.end()) {
    ObjectVerbosityMap[object_type] = new_verbosity;
    // recursively set for all auto objects for this user object
    for (const auto ref_ot :
         model_info->priority_inverse_refs_get(object_type)) {
      ObjectVerbosityMap[ref_ot] = new_verbosity;
    }
  }
  return;
}

void set_log_level_all_objects(switch_verbosity_t verbosity) {
  smi::ModelInfo *model_info = smi::switch_store::switch_model_info_get();
  for (auto it = model_info->begin(); it != model_info->end(); it++) {
    auto object_info = *it;
    ObjectVerbosityMap[object_info.object_type] = verbosity;
  }
  mapInitialized = true;
#ifndef TESTING
  bf_sys_trace_level_set(BF_MOD_SWITCHAPI, switch_to_bf_sys_level(verbosity));
  bf_sys_log_level_set(
      BF_MOD_SWITCHAPI, BF_LOG_DEST_FILE, switch_to_bf_sys_level(verbosity));
  bf_sys_log_level_set(
      BF_MOD_SWITCHAPI, BF_LOG_DEST_STDOUT, switch_to_bf_sys_level(verbosity));
#endif
}

// types from switch types

template <typename T>
std::ostream &operator<<(std::ostream &os, const std::vector<T> &vec) {
  os << "[ ";
  for (auto const &value : vec) os << value << " ";
  os << "]";
  return os;
}
template std::ostream &operator<<<bool>(std::ostream &os,
                                        const std::vector<bool> &vec);
template std::ostream &operator<<<uint32_t>(std::ostream &os,
                                            const std::vector<uint32_t> &vec);
template std::ostream &operator<<<switch_object_id_t>(
    std::ostream &os, const std::vector<switch_object_id_t> &vec);

template <typename T>
std::ostream &operator<<(std::ostream &os, const std::set<T> &vec) {
  os << "[ ";
  for (auto const &value : vec) os << value << " ";
  os << "]";
  return os;
}
template std::ostream &operator<<<switch_object_id_t>(
    std::ostream &os, const std::set<switch_object_id_t> &vec);

std::ostream &operator<<(std::ostream &os,
                         const switch_object_id_t &object_id) {
  os << std::hex << "0x" << object_id.data << std::dec;
  return os;
}

std::ostream &operator<<(std::ostream &os,
                         const switch_operation_t &operation) {
  switch (operation) {
    case SMI_CREATE_OPERATION:
      os << "create";
      break;
    case SMI_GET_OPERATION:
      os << "get";
      break;
    case SMI_DELETE_OPERATION:
      os << "delete";
      break;
    case SMI_SET_OPERATION:
      os << "set";
      break;
  }
  return os;
}

std::ostream &operator<<(std::ostream &os,
                         const switch_mac_addr_t &mac_struct) {
  char mac_str[18];
  snprintf(mac_str,
           sizeof(mac_str),
           "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_struct.mac[0],
           mac_struct.mac[1],
           mac_struct.mac[2],
           mac_struct.mac[3],
           mac_struct.mac[4],
           mac_struct.mac[5]);
  os << mac_str;
  return os;
}
std::ostream &operator<<(std::ostream &os, const switch_string_t &string_data) {
  os << string_data.text;
  return os;
}

std::ostream &operator<<(std::ostream &os, const switch_enum_t &enum_data) {
  os << enum_data.enumdata;
  return os;
}

std::ostream &operator<<(std::ostream &os, const switch_ip6_t &ipv6_address) {
  char ipv6_str[INET6_ADDRSTRLEN];
  inet_ntop(AF_INET6, ipv6_address, ipv6_str, INET6_ADDRSTRLEN);
  os << ipv6_str;
  return os;
}

std::ostream &operator<<(std::ostream &os,
                         const switch_ip_addr_family_t &family) {
  if (family == SWITCH_IP_ADDR_FAMILY_IPV4) {
    os << "SWITCH_IP_ADDR_FAMILY_IPV4";
  } else if (family == SWITCH_IP_ADDR_FAMILY_IPV6) {
    os << "SWITCH_IP_ADDR_FAMILY_IPV6";
  } else {
    int value = family;
    os << value;
  }
  return os;
}

std::ostream &operator<<(std::ostream &os,
                         const switch_ip_address_t &ip_address) {
  if (ip_address.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4) {
    struct in_addr ip_addr;
    ip_addr.s_addr = htonl(ip_address.ip4);
    os << inet_ntoa(ip_addr);
  } else if (ip_address.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6) {
    char ipv6_str[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, ip_address.ip6, ipv6_str, INET6_ADDRSTRLEN);
    os << ipv6_str;
  } else {
    os << "0.0.0.0";
  }
  return os;
}

std::ostream &operator<<(std::ostream &os,
                         const switch_ip_prefix_t &ip_prefix) {
  if (ip_prefix.addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4) {
    struct in_addr ip_addr;
    ip_addr.s_addr = htonl(ip_prefix.addr.ip4);
    os << inet_ntoa(ip_addr) << "/" << ip_prefix.len;
  } else if (ip_prefix.addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6) {
    char ipv6_str[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, ip_prefix.addr.ip6, ipv6_str, INET6_ADDRSTRLEN);
    os << ipv6_str << "/" << ip_prefix.len;
  } else {
    os << "0.0.0.0/0";
  }
  return os;
}

std::ostream &operator<<(std::ostream &os, const switch_range_t &range) {
  os << range.min << "-" << range.max;
  return os;
}

// types from switch attributes

std::ostream &operator<<(std::ostream &os,
                         const switch_attr_type_t &attribute_type) {
  os << switch_attr_type_str(attribute_type);
  return os;
}

std::ostream &operator<<(std::ostream &os, const std::set<smi::attr_w> &attrs) {
  for (auto &attr : attrs) {
    os << attr << std::endl;
  }
  return os;
}

std::ostream &operator<<(std::ostream &os,
                         const std::vector<smi::attr_w> &attrs) {
  for (auto &attr : attrs) {
    os << attr << std::endl;
  }
  return os;
}

std::string getEnumName(const smi::AttributeMetadata *attr_md, uint64_t value) {
  const smi::ValueMetadata *val_md = attr_md->get_value_metadata();
  if (val_md) {
    auto &enums = val_md->get_enum_metadata();
    auto match = std::find_if(
        enums.begin(), enums.end(), [value](const smi::EnumMetadata &e) {
          return value == e.enum_value;
        });
    if (match != enums.end()) {
      return match->enum_name;
    }
  }
  return "";
}

std::ostream &operator<<(std::ostream &os, const smi::attr_w &attr) {
  auto attr_id = attr.id_get();
  smi::ModelInfo *model_info = smi::switch_store::switch_model_info_get();
  switch_object_type_t ot = model_info->get_object_type_from_attr_id(attr_id);
  const smi::ObjectInfo *object_info = model_info->get_object_info(ot);
  const smi::AttributeMetadata *attr_md =
      object_info->get_attr_metadata(attr_id);
  os << attr_md->get_attr_name() << "=";
  if (attr.type_get() == SWITCH_TYPE_LIST) {
    os << "[";
    auto end = attr.getattr_list().end();
    auto begin = attr.getattr_list().begin();
    if (SWITCH_TYPE_ENUM == attr.list_type_get()) {
      for (auto it = begin; it != end; it++) {
        if (it != begin) {
          os << ", ";
        }
        uint64_t value = it->enumdata.enumdata;
        auto name = getEnumName(attr_md, value);
        if (name.empty()) {
          os << value;
        } else {
          os << name << "(" << value << ")";
        }
      }
    } else {
      for (auto it = begin; it != end; it++) {
        if (it != begin) {
          os << ", ";
        }
        os << *it;
      }
    }
    os << "]";
  } else if (attr.type_get() == SWITCH_TYPE_ENUM) {
    uint64_t value = attr.value_get().enumdata.enumdata;
    auto name = getEnumName(attr_md, value);
    if (name.empty()) {
      os << value;
    } else {
      os << name << "(" << value << ")";
    }
  } else {
    os << attr.value_get();
  }
  return os;
}

std::ostream &operator<<(std::ostream &os,
                         const switch_attribute_value_t &attribute_value) {
  switch (attribute_value.type) {
    case SWITCH_TYPE_NONE:
      break;
    case SWITCH_TYPE_BOOL:
      os << std::boolalpha << attribute_value.booldata << std::noboolalpha;
      break;
    case SWITCH_TYPE_UINT8:
      os << unsigned(attribute_value.u8);
      break;
    case SWITCH_TYPE_UINT16:
      os << attribute_value.u16;
      break;
    case SWITCH_TYPE_UINT32:
      os << attribute_value.u32;
      break;
    case SWITCH_TYPE_UINT64:
      os << attribute_value.u64;
      break;
    case SWITCH_TYPE_INT64:
      os << attribute_value.s64;
      break;
    case SWITCH_TYPE_ENUM:
      os << attribute_value.enumdata;
      break;
    case SWITCH_TYPE_MAC:
      os << attribute_value.mac;
      break;
    case SWITCH_TYPE_IP_ADDRESS:
      os << attribute_value.ipaddr;
      break;
    case SWITCH_TYPE_IP_PREFIX:
      os << attribute_value.ipprefix;
      break;
    case SWITCH_TYPE_OBJECT_ID:
      os << attribute_value.oid;
      break;
    case SWITCH_TYPE_RANGE:
      os << attribute_value.range;
      break;
    case SWITCH_TYPE_LIST:
      os << attribute_value.list;
      break;
    case SWITCH_TYPE_STRING:
      os << "\"" << attribute_value.text << "\"";
      break;
    default:
      break;
  }
  return os;
}

std::ostream &operator<<(std::ostream &os,
                         const switch_attribute_t &attribute) {
  smi::ModelInfo *model_info = smi::switch_store::switch_model_info_get();
  switch_object_type_t ot =
      model_info->get_object_type_from_attr_id(attribute.id);
  const smi::ObjectInfo *object_info = model_info->get_object_info(ot);
  const smi::AttributeMetadata *attr_md =
      object_info->get_attr_metadata(attribute.id);
  if (attribute.value.type == SWITCH_TYPE_ENUM) {
    const smi::ValueMetadata *value_md = attr_md->get_value_metadata();
    const std::vector<smi::EnumMetadata> &enums = value_md->get_enum_metadata();
    for (const auto &sw_enum_vals : enums) {
      if (sw_enum_vals.enum_value == attribute.value.enumdata.enumdata) {
        os << "id: " << attr_md->get_attr_name()
           << " value: " << sw_enum_vals.enum_name;
        return os;
      }
    }
    os << "id: " << attr_md->get_attr_name() << ", value: invalid enum value "
       << attribute.value.enumdata.enumdata;
  } else {
    os << "id: " << attr_md->get_attr_name() << ", value: " << attribute.value;
  }
  return os;
}

std::ostream &operator<<(std::ostream &os,
                         const switch_attr_list_t &attribute_list) {
  os << "[";
  if (attribute_list.list) {
    for (uint32_t i = 0; i < attribute_list.count; i++) {
      os << attribute_list.list[i]
         << (i + 1 < attribute_list.count ? ", " : "");
    }
  }
  os << "]";
  return os;
}

namespace smi {
namespace logging {

switch_status_t logging_init(switch_verbosity_t verbosity,
                             bool override_log_level = true) {
  set_log_level_all_objects(verbosity);
  default_verbosity = verbosity;
  // driver has default as debug for the following components in bf_drivers.log
  // and the log is unnecessarily filled up leaving out useful information from
  // BMAI. We will suppress these to limit amount of logging. They can always be
  // enabled via ucli on a live system for debugging.
  if (override_log_level) {
#ifndef TESTING
    bf_sys_trace_level_set(BF_MOD_PM, BF_LOG_ERR);
    bf_sys_log_level_set(BF_MOD_PM, BF_LOG_DEST_FILE, BF_LOG_ERR);
    bf_sys_trace_level_set(BF_MOD_PORT, BF_LOG_ERR);
    bf_sys_log_level_set(BF_MOD_PORT, BF_LOG_DEST_FILE, BF_LOG_ERR);
    bf_sys_trace_level_set(BF_MOD_MC, BF_LOG_ERR);
    bf_sys_log_level_set(BF_MOD_MC, BF_LOG_DEST_FILE, BF_LOG_ERR);
    bf_sys_trace_level_set(BF_MOD_TM, BF_LOG_ERR);
    bf_sys_log_level_set(BF_MOD_TM, BF_LOG_DEST_FILE, BF_LOG_ERR);
    bf_sys_trace_level_set(BF_MOD_PLTFM, BF_LOG_ERR);
    bf_sys_log_level_set(BF_MOD_PLTFM, BF_LOG_DEST_FILE, BF_LOG_ERR);
#endif
  }
  return SWITCH_STATUS_SUCCESS;
}

static inline void switch_log_internal(switch_verbosity_t verbosity,
                                       const char *message) {
  (void)verbosity;
#ifdef TESTING
  std::cout << message << std::endl;
#else
  bf_sys_log_and_trace(
      BF_MOD_SWITCHAPI, switch_to_bf_sys_level(verbosity), "%s", message);
#endif
}

bool switch_log(switch_verbosity_t verbosity, const char *message) {
  if (verbosity <= default_verbosity) {
    switch_log_internal(verbosity, message);
    return true;
  }
  return false;
}

bool switch_log(switch_verbosity_t verbosity,
                switch_object_type_t object_type,
                const char *message) {
  if (object_type == 0) return switch_log(verbosity, message);

  if (ObjectVerbosityMap.find(object_type) != ObjectVerbosityMap.end()) {
    if (verbosity <= ObjectVerbosityMap[object_type]) {
      return switch_log(verbosity, message);
    }
  } else {
    return switch_log(verbosity, message);
  }
  return false;
}

bool switch_log(switch_verbosity_t verbosity,
                switch_object_type_t object_type,
                switch_operation_t operation,
                const char *message) {
  if (operationSettings[operation]) {
    return switch_log(verbosity, object_type, message);
  }
  return false;
}

const std::unordered_map<switch_object_type_t, switch_verbosity_t>
    &get_log_level_all_objects() {
  return ObjectVerbosityMap;
}

}  // namespace logging
}  // namespace smi
