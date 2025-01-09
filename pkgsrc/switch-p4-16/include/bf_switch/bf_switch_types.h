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


#ifndef INCLUDE_BF_SWITCH_BF_SWITCH_TYPES_H_
#define INCLUDE_BF_SWITCH_BF_SWITCH_TYPES_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

/** @defgroup types BF Switch datatypes
 *  Various switch datatypes, structures and definitions
 *  @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
 * DEFINES
 ***************************************************************************/

#define SWITCH_DEFAULT_VRF 1

#define SWITCH_DEFAULT_VLAN 1

#define SWITCH_MAX_VRF 1024

#define SWITCH_MAX_DEVICE 256

#define SWITCH_MAX_PORTS 1024

#define SWITCH_MAX_PORT_PER_PIPE 64

#define SWITCH_MAX_RECIRC_PORTS 8

#define SWITCH_MAX_VLANS 4096

#define SWITCH_CPU_PORT_ETH_DEFAULT 501

#define SWITCH_CPU_PORT_PCIE_DEFAULT 502

#define SWITCH_BUFFER_MAX_THRESHOLD 255

/** Identifies an error code. */
typedef uint32_t switch_status_t;

/**
 * Status success
 */
#define SWITCH_STATUS_SUCCESS 0x00000000L

/**
 *  General failure
 */
#define SWITCH_STATUS_FAILURE 0x00000001L

/**
 *  The request is not supported
 */
#define SWITCH_STATUS_NOT_SUPPORTED 0x00000002L

/**
 *  Not enough memory to complete the operation
 */
#define SWITCH_STATUS_NO_MEMORY 0x00000003L

/**
 *  Insufficient system resources exist to complete the operation
 */
#define SWITCH_STATUS_INSUFFICIENT_RESOURCES 0x00000004L

/**
 *  An invalid parameter was passed to a function
 */
#define SWITCH_STATUS_INVALID_PARAMETER 0x00000005L

/**
 *  An item already exists
 */
#define SWITCH_STATUS_ITEM_ALREADY_EXISTS 0x00000006L

/**
 *  An item was not found
 */
#define SWITCH_STATUS_ITEM_NOT_FOUND 0x00000007L

/**
 *  Resource is currently used by an object
 */
#define SWITCH_STATUS_RESOURCE_IN_USE 0x00000008L

/**
 *  Hardware failure
 */
#define SWITCH_STATUS_HW_FAILURE 0x00000009L

/**
 *  Error returned by pdfixed layer
 */
#define SWITCH_STATUS_PD_FAILURE 0x0000000AL

/**
 *  Object creation failure because dependency not met
 */
#define SWITCH_STATUS_DEPENDENCY_FAILURE 0x0000000BL

/**
 *  Invalid key group for object lookup
 */
#define SWITCH_STATUS_INVALID_KEY_GROUP 0x0000000CL

/**
 *  Object class is not implemented
 */
#define SWITCH_STATUS_NOT_IMPLEMENTED 0x0000000DL

/**
 *  The feature is not supported
 */
#define SWITCH_STATUS_FEATURE_NOT_SUPPORTED 0x0000000EL

/**
 *   Table is full
 */
#define SWITCH_STATUS_TABLE_FULL 0x00000010L

#define SWITCH_NULL_OBJECT_ID 0L
#define SWITCH_INVALID_HW_PORT 0x1FF

/**
 * VLAN ID WIDTH
 */
#define VLAN_ID_WIDTH 12

/**
 *  MAC address length
 */
#define ETH_LEN 6

/**
 *  IPv4 address length
 */
#define IPV4_LEN 4

/**
 *  IPv6 address length
 */
#define IPV6_LEN 16

#define SWITCH_MAX_STRING_LEN 24
#define SWITCH_API_ACL_ENTRY_MINIMUM_PRIORITY 0
#define SWITCH_API_ACL_ENTRY_MAXIMUM_PRIORITY (1 << 14)

/** @brief Object type description
 *
 \verbatim
  "vlan" : {
      "class" : "user",
          "attributes" : {
          }
      }
  }
 \endverbatim
 * This is the basic type for all object types.
 * \n All of the object types are derived at compile time after compiling
 * the object schema. An example type would be SWITCH_OBJECT_TYPE_VLAN.
 * \n They can be looked up in model.h in the install directory.
 */
typedef uint16_t switch_object_type_t;

/** @brief Attribute ID description
 *
 \verbatim
  "vlan" : {
      "class" : "user",
          "attributes" : {
          "vlan_id" : {
              "description": "Vlan ID",
              "is_mandatory": true,
              "type_info" : {
                  "max" : 4095,
                  "type" : "SWITCH_TYPE_UINT16"
              }
          }
      }
  }
 \endverbatim
 * This is the basic type for all attributes of an object
 * \n All of the attributes IDs are derived at compile time after compiling
 * the object schema.
 * \n The attribute ID is structured thus,
 * \n         <prefix>_<object_type>_ATTR_<attr_name>
 * \n Ex:      SWITCH_VLAN_ATTR_VLAN_ID
 * \n They can be looked up in model.h in the install directory.
 */
typedef uint16_t switch_attr_id_t;

/**
 * @brief Various attribute types
 */
typedef enum _switch_attr_type_t {
  SWITCH_TYPE_NONE = 0,
  SWITCH_TYPE_FIRST = 1,
  /** bool data type. Valid for bool data type */
  SWITCH_TYPE_BOOL = SWITCH_TYPE_FIRST,
  /** uint8 data type. Valid for uint8_t data type */
  SWITCH_TYPE_UINT8,
  /** uint16 data type. Valid for uint16_t data type */
  SWITCH_TYPE_UINT16,
  /** uint32 data type. Valid for uint32_t data type */
  SWITCH_TYPE_UINT32,
  /** uint64 data type. Valid for uint64_t data type */
  SWITCH_TYPE_UINT64,
  /** int64 data type. Valid for int64_t data type */
  SWITCH_TYPE_INT64,
  /** enum data type. Valid for \ref switch_enum_t data type */
  SWITCH_TYPE_ENUM,
  /** MAC data type. Valid for \ref switch_mac_addr_t data type */
  SWITCH_TYPE_MAC,
  /** string data type. Valid for \ref switch_string_t data type */
  SWITCH_TYPE_STRING,
  /** ip address data type. Valid for \ref switch_ip_address_t data type */
  SWITCH_TYPE_IP_ADDRESS,
  /** ip prefix data type. Valid for \ref switch_ip_prefix_t data type */
  SWITCH_TYPE_IP_PREFIX,
  /** object handle data type. Valid for \ref switch_object_id_t data type */
  SWITCH_TYPE_OBJECT_ID,
  /** range data type. Valid for \ref switch_range_t data type */
  SWITCH_TYPE_RANGE,
  /** list data type. Valid for \ref switch_attr_list_t data type */
  SWITCH_TYPE_LIST,

  SWITCH_TYPE_MAX,
} switch_attr_type_t;

/** Data type to hold an enum value */
typedef struct switch_enum_t switch_enum_t;
struct switch_enum_t {
  uint64_t enumdata;
#ifdef __cplusplus
  inline bool operator==(const switch_enum_t &val) const {
    return enumdata == val.enumdata;
  }
  inline bool operator<(const switch_enum_t &val) const {
    return enumdata < val.enumdata;
  }
#endif
};

/** Data type to hold a MAC address */
typedef struct switch_mac_addr_t switch_mac_addr_t;
struct switch_mac_addr_t {
  unsigned char mac[ETH_LEN];
#ifdef __cplusplus
  inline bool operator==(const switch_mac_addr_t &val) const {
    return !memcmp(mac, val.mac, ETH_LEN);
  }
#endif
};

/** Data type to hold a string */
typedef struct switch_string_t switch_string_t;
struct switch_string_t {
  char text[SWITCH_MAX_STRING_LEN];
#ifdef __cplusplus
  inline bool operator==(const switch_string_t &val) const {
    return !memcmp(text, val.text, SWITCH_MAX_STRING_LEN);
  }
#endif
};

/** IP address family wrapper for v4 and v6 types. None is ignored */
typedef enum _switch_ip_addr_family_t {
  SWITCH_IP_ADDR_FAMILY_NONE,
  SWITCH_IP_ADDR_FAMILY_IPV4,
  SWITCH_IP_ADDR_FAMILY_IPV6
} switch_ip_addr_family_t;

/** Max prefix length in bits */
typedef enum _switch_ip_max_prefix_len_t {
  SWITCH_IPV4_MAX_PREFIX_LEN = 32,
  SWITCH_IPV6_MAX_PREFIX_LEN = 128
} switch_ip_max_prefix_len_t;

typedef uint32_t switch_ip4_t;
typedef uint8_t switch_ip6_t[IPV6_LEN];

/** IP address wrapper for v4 and v6 addresses */
typedef struct switch_ip_address_t switch_ip_address_t;
struct switch_ip_address_t {
  switch_ip_addr_family_t addr_family;
  union {
    switch_ip4_t ip4;
    switch_ip6_t ip6;
  };
#ifdef __cplusplus
  inline bool operator==(const switch_ip_address_t &val) const {
    return ((addr_family == val.addr_family) &&
            ((addr_family == SWITCH_IP_ADDR_FAMILY_IPV4)
                 ? (ip4 == val.ip4)
                 : (!memcmp(ip6, val.ip6, IPV6_LEN))));
  }
#endif
};

/** IP prefix wrapper for v4 and v6 addresses with prefix length */
typedef struct switch_ip_prefix_t switch_ip_prefix_t;
struct switch_ip_prefix_t {
  uint16_t len;
  switch_ip_address_t addr;
#ifdef __cplusplus
  inline bool operator==(const switch_ip_prefix_t &val) const {
    return ((len == val.len) && (addr == val.addr));
  }
#endif
};

/**
 * In a switch_object_id_t, 48 LSB bits is object ID and 16 MSB bits are type
 */
#define OBJECT_ID_WIDTH 48

/** Object handle for all object types */
typedef struct switch_object_id_t switch_object_id_t;
struct switch_object_id_t {
  uint64_t data;
#ifdef __cplusplus
  inline bool operator==(const switch_object_id_t &val) const {
    return data == val.data;
  }
#endif
};

/** Range data type to carry min/max values */
typedef struct switch_range_t switch_range_t;
struct switch_range_t {
  uint32_t min;
  uint32_t max;
#ifdef __cplusplus
  inline bool operator==(const switch_range_t &val) const {
    return ((min == val.min) && (max == val.max));
  }
#endif
};

typedef struct _switch_attribute_value_t switch_attribute_value_t;

/** List data type for various attribute types */
typedef struct _switch_attr_list_t {
  switch_attr_type_t list_type;
  size_t count;
  switch_attribute_value_t *list;
} switch_attr_list_t;

/** Attribute data storage mirroring \ref switch_attr_type_t */
struct _switch_attribute_value_t {
  /* base types */
  union {
    bool booldata;
    uint8_t u8;
    uint16_t u16;
    uint32_t u32;
    uint64_t u64;
    int64_t s64;
    switch_enum_t enumdata;
    switch_mac_addr_t mac;
    switch_string_t text;
    switch_ip_address_t ipaddr;
    switch_ip_prefix_t ipprefix;
    switch_object_id_t oid;
    switch_range_t range;
    /* composite types */
    switch_attr_list_t list;
  };
  switch_attr_type_t type;
#ifdef __cplusplus
  bool operator==(const switch_attribute_value_t &val) const {
    if (type != val.type) return false;
    switch (type) {
      case SWITCH_TYPE_BOOL:
        return booldata == val.booldata;
      case SWITCH_TYPE_UINT8:
        return u8 == val.u8;
      case SWITCH_TYPE_UINT16:
        return u16 == val.u16;
      case SWITCH_TYPE_UINT32:
        return u32 == val.u32;
      case SWITCH_TYPE_UINT64:
        return u64 == val.u64;
      case SWITCH_TYPE_INT64:
        return s64 == val.s64;
      case SWITCH_TYPE_ENUM:
        return enumdata == val.enumdata;
      case SWITCH_TYPE_MAC:
        return mac == val.mac;
      case SWITCH_TYPE_STRING:
        return text == val.text;
      case SWITCH_TYPE_IP_ADDRESS:
        return ipaddr == val.ipaddr;
      case SWITCH_TYPE_IP_PREFIX:
        return ipprefix == val.ipprefix;
      case SWITCH_TYPE_OBJECT_ID:
        return oid == val.oid;
      case SWITCH_TYPE_RANGE:
        return range == val.range;
      case SWITCH_TYPE_LIST:
        return false;
      default:
        return false;
    }
  }
#endif
};

/** Wrapper attribute structure to carry both attribute ID and value
 \verbatim
  "port" : {
      "class" : "user",
          "attributes" : {
          "port_id" : {
              "type_info" : {
                  "type" : "SWITCH_TYPE_UINT16"
              }
          }
      }
  }
 \endverbatim
 * For this example, switch_attribute_t can be used the following way,
 * \n switch_attribute_t attr;
 * \n uint16_t port_id = 10;
 * \n attr.id = SWITCH_PORT_ATTR_PORT_ID;
 * \n attr.value.type = SWITCH_TYPE_UINT16;
 * \n attr.value.u16 = port_id;
 */
typedef struct _switch_attribute_t {
  switch_attr_id_t id;
  switch_attribute_value_t value;
} switch_attribute_t;

/** Data type to carry counter information */
typedef struct _switch_counter_t {
  uint16_t counter_id;
  uint64_t count;
} switch_counter_t;

/** Data type to carry table sizes information */
typedef struct _switch_table_info_t {
  size_t table_size;
  uint32_t table_usage;
  uint32_t bf_rt_table_id;
} switch_table_info_t;

/**
 * @brief Packet type. Derived from switch.p4
 */
typedef enum _switch_packet_type_t {
  SWITCH_PACKET_TYPE_UNICAST = 0,
  SWITCH_PACKET_TYPE_MULTICAST = 1,
  SWITCH_PACKET_TYPE_BROADCAST = 2,
  SWITCH_PACKET_TYPE_MAX = SWITCH_PACKET_TYPE_BROADCAST + 1
} switch_packet_type_t;

/**
 * @brief Drop report types. Derived from switch.p4
 */
typedef enum _switch_dtel_report_type_t {
  SWITCH_DTEL_REPORT_TYPE_NONE = 0,   // 0b000
  SWITCH_DTEL_REPORT_TYPE_FLOW = 1,   // 0b001
  SWITCH_DTEL_REPORT_TYPE_QUEUE = 2,  // 0b010
  SWITCH_DTEL_REPORT_TYPE_DROP = 4    // 0b100
} switch_dtel_report_type_t;

/**
 * @brief Logging levels
 */
typedef enum _switch_verbosity_t {
  SWITCH_API_LEVEL_ERROR,
  SWITCH_API_LEVEL_WARN,
  SWITCH_API_LEVEL_INFO,
  SWITCH_API_LEVEL_DEBUG,
  SWITCH_API_LEVEL_DETAIL,
  SWITCH_API_LEVEL_MAX
} switch_verbosity_t;

static inline const char *switch_error_to_string(switch_status_t status) {
  switch (status) {
    case SWITCH_STATUS_SUCCESS:
      return "Success";
    case SWITCH_STATUS_FAILURE:
      return "General failure";
    case SWITCH_STATUS_NOT_SUPPORTED:
      return "Request not supported";
    case SWITCH_STATUS_NO_MEMORY:
      return "Not enough memory to complete the operation";
    case SWITCH_STATUS_INSUFFICIENT_RESOURCES:
      return "Insufficient system resources";
    case SWITCH_STATUS_INVALID_PARAMETER:
      return "Invalid parameter";
    case SWITCH_STATUS_ITEM_ALREADY_EXISTS:
      return "Object already exists";
    case SWITCH_STATUS_ITEM_NOT_FOUND:
      return "Item not found";
    case SWITCH_STATUS_RESOURCE_IN_USE:
      return "Resource is currently used by an object";
    case SWITCH_STATUS_HW_FAILURE:
      return "Hardware failure";
    case SWITCH_STATUS_PD_FAILURE:
      return "Error returned by PD";
    case SWITCH_STATUS_DEPENDENCY_FAILURE:
      return "Object creation failure because dependency not met";
    case SWITCH_STATUS_INVALID_KEY_GROUP:
      return "Invalid key group for object lookup";
    case SWITCH_STATUS_NOT_IMPLEMENTED:
      return "Object class is not implemented";
    case SWITCH_STATUS_FEATURE_NOT_SUPPORTED:
      return "Feature not supported";
    case SWITCH_STATUS_TABLE_FULL:
      return "Table full";
    default:
      return "err: unknown error code";
  }
}

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include <string>
#include <vector>
#include <list>

// borrowed from Boost
template <typename T>
void hash_combine(size_t &seed, T const &v) {
  seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}
template <typename It>
std::size_t hash_range(It first, It last) {
  size_t seed = 0;
  for (; first != last; ++first) {
    hash_combine(seed, *first);
  }
  return seed;
}

template <typename It>
void hash_range(std::size_t &seed, It first, It last) {
  for (; first != last; ++first) {
    hash_combine(seed, *first);
  }
}

namespace smi {

/**
 * @brief C++ wrapper for \ref switch_attribute_t
 */
class attr_w {
 public:
  /* @brief Constructor for this attribute. val is of type \ref
   * switch_attr_type_t
   */
  template <typename T>
  attr_w(const switch_attr_id_t attr_id, const T val);
  template <typename T>
  attr_w(const switch_attr_id_t attr_id, const std::vector<T> val);
  attr_w(const switch_attr_id_t attr_id) {
    m_attr.id = attr_id;
    seed = std::hash<uint16_t>{}(attr_id);
  }

  /**
   * @brief Utility to build an attr_w given a C type switch_attribute_t object
   */
  switch_status_t attr_import(const switch_attribute_t &attr);

  /**
   * @brief Utility to build a \ref switch_attribute_t object given an attr_w.
   * The caller is responsible for releasing the memory allocated if the
   * attribute type is a list
   */
  switch_status_t attr_export(switch_attribute_t *attr);

  /**
   * @brief Return the attribute id (\ref switch_attr_id_t)
   */
  inline switch_attr_id_t id_get() const { return m_attr.id; }
  inline size_t seed_get() const { return seed; }
  inline void id_set(const switch_attr_id_t id) {
    m_attr.id = id;
    seed = std::hash<uint16_t>{}(id);
  }

  /**
   * @brief Return the attribute value type (\ref switch_attr_type_t)
   */
  inline switch_attr_type_t type_get() const { return m_attr.value.type; }

  /**
   * @brief Return the list type if value type is SWITCH_TYPE_LIST
   */
  inline switch_attr_type_t list_type_get() const { return list_type; }

  /**
   * @brief Get a string representation of the attribute
   */
  void attr_to_string(std::string &str) const;

  /**
   * @brief Return the attribute value. The user has to pass the correct type
   * expected
   */
  template <typename T>
  switch_status_t v_get(T &val) const;
  template <typename T>
  void v_set(const T val);

  /**
   * @brief Return the attribute value list. The user has to pass the correct
   * type expected
   */
  template <typename T>
  switch_status_t v_get(std::vector<T> &val) const;
  template <typename T>
  void v_set(const std::vector<T> val);

  const switch_attribute_t &getattr() const { return m_attr; };
  switch_attribute_t &getattr_mutable() { return m_attr; };

  const std::list<switch_attribute_value_t> &getattr_list() const {
    return mlist;
  }

  inline bool operator==(const attr_w &other) const {
    if (id_get() != other.id_get()) return false;
    switch (type_get()) {
      case SWITCH_TYPE_LIST: {
        const auto &other_list = other.value_list_get();
        return ((mlist.size() == other_list.size()) && (mlist == other_list));
      } break;
      default: {
        const auto &value = other.value_get();
        return (m_attr.value == value);
      }
    }
  }

  inline const switch_attribute_value_t &value_get() const {
    return m_attr.value;
  }

  inline const std::list<switch_attribute_value_t> &value_list_get() const {
    return mlist;
  }

 private:
  switch_attribute_t m_attr = {};
  switch_attr_type_t list_type = SWITCH_TYPE_NONE;
  std::list<switch_attribute_value_t> mlist;
  size_t seed;
};

inline bool operator<(const attr_w &lhs, const attr_w &rhs) {
  return lhs.id_get() < rhs.id_get();
}

}  // namespace smi

namespace std {

template <>
struct hash<switch_object_id_t> {
  inline size_t operator()(switch_object_id_t const &object_id) const {
    return std::hash<uint64_t>{}(object_id.data);
  }
};

template <>
struct hash<switch_mac_addr_t> {
  size_t operator()(switch_mac_addr_t const &mac) const noexcept {
    size_t h = 5381;
    for (int i = 0; i < ETH_LEN; i++) {
      h = ((h << 5) + h) + static_cast<int>(mac.mac[i]);
    }
    return h;
  }
};

template <>
struct hash<switch_string_t> {
  size_t operator()(switch_string_t const &switch_string) const noexcept {
    size_t h = 5381;
    for (int i = 0; i < SWITCH_MAX_STRING_LEN; i++) {
      h = ((h << 5) + h) + static_cast<int>(switch_string.text[i]);
    }
    return h;
  }
};

template <>
struct hash<switch_ip6_t> {
  size_t operator()(switch_ip6_t const &addr) const noexcept {
    uint64_t upper;
    uint64_t lower;
    memcpy(&upper, &addr[0], sizeof(upper));
    memcpy(&lower, &addr[8], sizeof(lower));
    auto seed = std::hash<uint64_t>{}(upper);
    hash_combine(seed, lower);
    return seed;
  }
};

template <>
struct hash<switch_ip_address_t> {
  size_t operator()(switch_ip_address_t const &addr) const noexcept {
    size_t seed = static_cast<size_t>(addr.addr_family);
    if (addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4) {
      hash_combine(seed, addr.ip4);
    } else if (addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6) {
      hash_combine(seed, addr.ip6);
    }
    return seed;
  }
};

template <>
struct hash<switch_ip_prefix_t> {
  size_t operator()(switch_ip_prefix_t const &prefix) const noexcept {
    auto seed = std::hash<uint16_t>{}(prefix.len);
    hash_combine(seed, prefix.addr);
    return seed;
  }
};

template <>
struct hash<smi::attr_w> {
  size_t operator()(smi::attr_w const &attr) const noexcept {
    const auto &value = attr.value_get();
    auto seed = attr.seed_get();
    switch (attr.type_get()) {
      case SWITCH_TYPE_BOOL:
        hash_combine(seed, value.booldata);
        return seed;
      case SWITCH_TYPE_UINT8:
        hash_combine(seed, value.u8);
        return seed;
      case SWITCH_TYPE_UINT16:
        hash_combine(seed, value.u16);
        return seed;
      case SWITCH_TYPE_UINT32:
        hash_combine(seed, value.u32);
        return seed;
      case SWITCH_TYPE_UINT64:
        hash_combine(seed, value.u64);
        return seed;
      case SWITCH_TYPE_INT64:
        hash_combine(seed, value.s64);
        return seed;
      case SWITCH_TYPE_ENUM:
        hash_combine<int>(seed, value.enumdata.enumdata);
        return seed;
      case SWITCH_TYPE_MAC:
        hash_combine(seed, value.mac);
        return seed;
      case SWITCH_TYPE_STRING:
        hash_combine(seed, value.text);
        return seed;
      case SWITCH_TYPE_IP_ADDRESS:
        hash_combine(seed, value.ipaddr);
        return seed;
      case SWITCH_TYPE_IP_PREFIX:
        hash_combine(seed, value.ipprefix);
        return seed;
      case SWITCH_TYPE_OBJECT_ID:
        hash_combine(seed, value.oid.data);
        return seed;
      case SWITCH_TYPE_RANGE: {
        hash_combine(seed, value.range.min);
        hash_combine(seed, value.range.max);
        return seed;
      } break;
      case SWITCH_TYPE_LIST:
        return 0;
      default:
        return 0;
    }
  }
};
}  // namespace std

#endif

/** @} */

#endif  // INCLUDE_BF_SWITCH_BF_SWITCH_TYPES_H_
