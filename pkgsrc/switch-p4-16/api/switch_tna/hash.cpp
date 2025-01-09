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


#include <string>
#include <vector>

#include "switch_tna/utils.h"

namespace smi {
using namespace smi::bf_rt;  // NOLINT(build/namespaces)
using ::smi::logging::switch_log;

class hash_utility {
  switch_hash_attr_type type;

 public:
  void switch_hash_attr_type_set(switch_hash_attr_type hash_type) {
    type = hash_type;
  }

  bool validate_ipv4_hash() {
    if (type == SWITCH_HASH_ATTR_TYPE_ECMP_IPV4_HASH) {
      return true;
    }
    return false;
  }

  bool validate_ipv6_hash() {
    if (type == SWITCH_HASH_ATTR_TYPE_ECMP_IPV6_HASH) {
      return true;
    }
    return false;
  }

  bool validate_nonip_hash() {
    if (type == SWITCH_HASH_ATTR_TYPE_NON_IP_HASH) {
      return true;
    }
    return false;
  }

  bool validate_lag_v4_hash() {
    if (type == SWITCH_HASH_ATTR_TYPE_LAG_IPV4_HASH) {
      return true;
    }
    return false;
  }

  bool validate_lag_v6_hash() {
    if (type == SWITCH_HASH_ATTR_TYPE_LAG_IPV6_HASH) {
      return true;
    }
    return false;
  }

  bool validate_inner_dtel_v4_hash() {
    if (type == SWITCH_HASH_ATTR_TYPE_INNER_DTEL_IPV4_HASH) {
      return true;
    }
    return false;
  }

  bool validate_inner_dtel_v6_hash() {
    if (type == SWITCH_HASH_ATTR_TYPE_INNER_DTEL_IPV6_HASH) {
      return true;
    }
    return false;
  }

  uint32_t hash_continer_id_get(const uint32_t &field_name) {
    switch (field_name) {
      case SWITCH_HASH_ATTR_FIELD_SRC_ADDR: {
        switch (type) {
          case SWITCH_HASH_ATTR_TYPE_ECMP_IPV4_HASH:
            return smi_id::P_IPV4_HASH_SRC_ADDR;

          case SWITCH_HASH_ATTR_TYPE_ECMP_IPV6_HASH:
            return smi_id::P_IPV6_HASH_SRC_ADDR;

          case SWITCH_HASH_ATTR_TYPE_LAG_IPV4_HASH:
            return smi_id::P_LAG_V4_HASH_SRC_ADDR;

          case SWITCH_HASH_ATTR_TYPE_LAG_IPV6_HASH:
            return smi_id::P_LAG_V6_HASH_SRC_ADDR;

          case SWITCH_HASH_ATTR_TYPE_INNER_DTEL_IPV4_HASH:
            return smi_id::P_INNER_DTEL_V4_HASH_SRC_ADDR;

          case SWITCH_HASH_ATTR_TYPE_INNER_DTEL_IPV6_HASH:
            return smi_id::P_INNER_DTEL_V6_HASH_SRC_ADDR;

          default:
            break;
        }
        break;
      }
      case SWITCH_HASH_ATTR_FIELD_DST_ADDR: {
        switch (type) {
          case SWITCH_HASH_ATTR_TYPE_ECMP_IPV4_HASH:
            return smi_id::P_IPV4_HASH_DST_ADDR;

          case SWITCH_HASH_ATTR_TYPE_ECMP_IPV6_HASH:
            return smi_id::P_IPV6_HASH_DST_ADDR;

          case SWITCH_HASH_ATTR_TYPE_LAG_IPV4_HASH:
            return smi_id::P_LAG_V4_HASH_DST_ADDR;

          case SWITCH_HASH_ATTR_TYPE_LAG_IPV6_HASH:
            return smi_id::P_LAG_V6_HASH_DST_ADDR;

          case SWITCH_HASH_ATTR_TYPE_INNER_DTEL_IPV4_HASH:
            return smi_id::P_INNER_DTEL_V4_HASH_DST_ADDR;

          case SWITCH_HASH_ATTR_TYPE_INNER_DTEL_IPV6_HASH:
            return smi_id::P_INNER_DTEL_V6_HASH_DST_ADDR;

          default:
            break;
        }
        break;
      }
      case SWITCH_HASH_ATTR_FIELD_IP_PROTO: {
        switch (type) {
          case SWITCH_HASH_ATTR_TYPE_ECMP_IPV4_HASH:
            return smi_id::P_IPV4_HASH_IP_PROTO;

          case SWITCH_HASH_ATTR_TYPE_ECMP_IPV6_HASH:
            return smi_id::P_IPV6_HASH_IP_PROTO;

          case SWITCH_HASH_ATTR_TYPE_LAG_IPV4_HASH:
            return smi_id::P_LAG_V4_HASH_IP_PROTO;

          case SWITCH_HASH_ATTR_TYPE_LAG_IPV6_HASH:
            return smi_id::P_LAG_V6_HASH_IP_PROTO;

          case SWITCH_HASH_ATTR_TYPE_INNER_DTEL_IPV4_HASH:
            return smi_id::P_INNER_DTEL_V4_HASH_IP_PROTO;

          case SWITCH_HASH_ATTR_TYPE_INNER_DTEL_IPV6_HASH:
            return smi_id::P_INNER_DTEL_V6_HASH_IP_PROTO;

          default:
            break;
        }
        break;
      }
      case SWITCH_HASH_ATTR_FIELD_SRC_PORT: {
        switch (type) {
          case SWITCH_HASH_ATTR_TYPE_ECMP_IPV4_HASH:
            return smi_id::P_IPV4_HASH_SRC_PORT;

          case SWITCH_HASH_ATTR_TYPE_ECMP_IPV6_HASH:
            return smi_id::P_IPV6_HASH_SRC_PORT;

          case SWITCH_HASH_ATTR_TYPE_LAG_IPV4_HASH:
            return smi_id::P_LAG_V4_HASH_SRC_PORT;

          case SWITCH_HASH_ATTR_TYPE_LAG_IPV6_HASH:
            return smi_id::P_LAG_V6_HASH_SRC_PORT;

          case SWITCH_HASH_ATTR_TYPE_INNER_DTEL_IPV4_HASH:
            return smi_id::P_INNER_DTEL_V4_HASH_SRC_PORT;

          case SWITCH_HASH_ATTR_TYPE_INNER_DTEL_IPV6_HASH:
            return smi_id::P_INNER_DTEL_V6_HASH_SRC_PORT;

          default:
            break;
        }
        break;
      }
      case SWITCH_HASH_ATTR_FIELD_DST_PORT: {
        switch (type) {
          case SWITCH_HASH_ATTR_TYPE_ECMP_IPV4_HASH:
            return smi_id::P_IPV4_HASH_DST_PORT;

          case SWITCH_HASH_ATTR_TYPE_ECMP_IPV6_HASH:
            return smi_id::P_IPV6_HASH_DST_PORT;

          case SWITCH_HASH_ATTR_TYPE_LAG_IPV4_HASH:
            return smi_id::P_LAG_V4_HASH_DST_PORT;

          case SWITCH_HASH_ATTR_TYPE_LAG_IPV6_HASH:
            return smi_id::P_LAG_V6_HASH_DST_PORT;

          case SWITCH_HASH_ATTR_TYPE_INNER_DTEL_IPV4_HASH:
            return smi_id::P_INNER_DTEL_V4_HASH_DST_PORT;

          case SWITCH_HASH_ATTR_TYPE_INNER_DTEL_IPV6_HASH:
            return smi_id::P_INNER_DTEL_V6_HASH_DST_PORT;

          default:
            break;
        }
        break;
      }
      case SWITCH_HASH_ATTR_FIELD_IPV6_FLOW_LABEL: {
        switch (type) {
          case SWITCH_HASH_ATTR_TYPE_ECMP_IPV6_HASH:
            return smi_id::P_IPV6_HASH_IPV6_FLOW_LABEL;

          case SWITCH_HASH_ATTR_TYPE_LAG_IPV6_HASH:
            return smi_id::P_LAG_V6_HASH_FLOW_LABEL;

          case SWITCH_HASH_ATTR_TYPE_INNER_DTEL_IPV6_HASH:
            return smi_id::P_INNER_DTEL_V6_HASH_FLOW_LABEL;

          default:
            break;
        }
        break;
      }
      case SWITCH_HASH_ATTR_FIELD_INGRESS_PORT: {
        return smi_id::P_NONIP_HASH_ING_PORT;
      }
      case SWITCH_HASH_ATTR_FIELD_MAC_TYPE: {
        return smi_id::P_NONIP_HASH_MAC_TYPE;
      }
      case SWITCH_HASH_ATTR_FIELD_SRC_MAC: {
        return smi_id::P_NONIP_HASH_SRC_MAC;
      }
      case SWITCH_HASH_ATTR_FIELD_DST_MAC: {
        return smi_id::P_NONIP_HASH_DST_MAC;
      }
      default:
        break;
    }
    return 0;
  }

  uint32_t fg_hash_continer_id_get(const uint32_t &field_name) {
    switch (field_name) {
      case SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_SRC_ADDR: {
        switch (type) {
          case SWITCH_HASH_ATTR_TYPE_ECMP_IPV4_HASH:
            return smi_id::P_IPV4_HASH_SRC_ADDR;

          case SWITCH_HASH_ATTR_TYPE_ECMP_IPV6_HASH:
            return smi_id::P_IPV6_HASH_SRC_ADDR;

          case SWITCH_HASH_ATTR_TYPE_LAG_IPV4_HASH:
            return smi_id::P_LAG_V4_HASH_SRC_ADDR;

          case SWITCH_HASH_ATTR_TYPE_LAG_IPV6_HASH:
            return smi_id::P_LAG_V6_HASH_SRC_ADDR;

          case SWITCH_HASH_ATTR_TYPE_INNER_DTEL_IPV4_HASH:
            return smi_id::P_INNER_DTEL_V4_HASH_SRC_ADDR;

          case SWITCH_HASH_ATTR_TYPE_INNER_DTEL_IPV6_HASH:
            return smi_id::P_INNER_DTEL_V6_HASH_SRC_ADDR;

          default:
            break;
        }
        break;
      }
      case SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_DST_ADDR: {
        switch (type) {
          case SWITCH_HASH_ATTR_TYPE_ECMP_IPV4_HASH:
            return smi_id::P_IPV4_HASH_DST_ADDR;

          case SWITCH_HASH_ATTR_TYPE_ECMP_IPV6_HASH:
            return smi_id::P_IPV6_HASH_DST_ADDR;

          case SWITCH_HASH_ATTR_TYPE_LAG_IPV4_HASH:
            return smi_id::P_LAG_V4_HASH_DST_ADDR;

          case SWITCH_HASH_ATTR_TYPE_LAG_IPV6_HASH:
            return smi_id::P_LAG_V6_HASH_DST_ADDR;

          case SWITCH_HASH_ATTR_TYPE_INNER_DTEL_IPV4_HASH:
            return smi_id::P_INNER_DTEL_V4_HASH_DST_ADDR;

          case SWITCH_HASH_ATTR_TYPE_INNER_DTEL_IPV6_HASH:
            return smi_id::P_INNER_DTEL_V6_HASH_DST_ADDR;

          default:
            break;
        }
        break;
      }
      case SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_IP_PROTO: {
        switch (type) {
          case SWITCH_HASH_ATTR_TYPE_ECMP_IPV4_HASH:
            return smi_id::P_IPV4_HASH_IP_PROTO;

          case SWITCH_HASH_ATTR_TYPE_ECMP_IPV6_HASH:
            return smi_id::P_IPV6_HASH_IP_PROTO;

          case SWITCH_HASH_ATTR_TYPE_LAG_IPV4_HASH:
            return smi_id::P_LAG_V4_HASH_IP_PROTO;

          case SWITCH_HASH_ATTR_TYPE_LAG_IPV6_HASH:
            return smi_id::P_LAG_V6_HASH_IP_PROTO;

          case SWITCH_HASH_ATTR_TYPE_INNER_DTEL_IPV4_HASH:
            return smi_id::P_INNER_DTEL_V4_HASH_IP_PROTO;

          case SWITCH_HASH_ATTR_TYPE_INNER_DTEL_IPV6_HASH:
            return smi_id::P_INNER_DTEL_V6_HASH_IP_PROTO;

          default:
            break;
        }
        break;
      }
      case SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_SRC_PORT: {
        switch (type) {
          case SWITCH_HASH_ATTR_TYPE_ECMP_IPV4_HASH:
            return smi_id::P_IPV4_HASH_SRC_PORT;

          case SWITCH_HASH_ATTR_TYPE_ECMP_IPV6_HASH:
            return smi_id::P_IPV6_HASH_SRC_PORT;

          case SWITCH_HASH_ATTR_TYPE_LAG_IPV4_HASH:
            return smi_id::P_LAG_V4_HASH_SRC_PORT;

          case SWITCH_HASH_ATTR_TYPE_LAG_IPV6_HASH:
            return smi_id::P_LAG_V6_HASH_SRC_PORT;

          case SWITCH_HASH_ATTR_TYPE_INNER_DTEL_IPV4_HASH:
            return smi_id::P_INNER_DTEL_V4_HASH_SRC_PORT;

          case SWITCH_HASH_ATTR_TYPE_INNER_DTEL_IPV6_HASH:
            return smi_id::P_INNER_DTEL_V6_HASH_SRC_PORT;

          default:
            break;
        }
        break;
      }
      case SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_DST_PORT: {
        switch (type) {
          case SWITCH_HASH_ATTR_TYPE_ECMP_IPV4_HASH:
            return smi_id::P_IPV4_HASH_DST_PORT;

          case SWITCH_HASH_ATTR_TYPE_ECMP_IPV6_HASH:
            return smi_id::P_IPV6_HASH_DST_PORT;

          case SWITCH_HASH_ATTR_TYPE_LAG_IPV4_HASH:
            return smi_id::P_LAG_V4_HASH_DST_PORT;

          case SWITCH_HASH_ATTR_TYPE_LAG_IPV6_HASH:
            return smi_id::P_LAG_V6_HASH_DST_PORT;

          case SWITCH_HASH_ATTR_TYPE_INNER_DTEL_IPV4_HASH:
            return smi_id::P_INNER_DTEL_V4_HASH_DST_PORT;

          case SWITCH_HASH_ATTR_TYPE_INNER_DTEL_IPV6_HASH:
            return smi_id::P_INNER_DTEL_V6_HASH_DST_PORT;

          default:
            break;
        }
        break;
      }
      case SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_IPV6_FLOW_LABEL: {
        switch (type) {
          case SWITCH_HASH_ATTR_TYPE_ECMP_IPV6_HASH:
            return smi_id::P_IPV6_HASH_IPV6_FLOW_LABEL;

          case SWITCH_HASH_ATTR_TYPE_LAG_IPV6_HASH:
            return smi_id::P_LAG_V6_HASH_FLOW_LABEL;

          case SWITCH_HASH_ATTR_TYPE_INNER_DTEL_IPV6_HASH:
            return smi_id::P_INNER_DTEL_V6_HASH_FLOW_LABEL;

          default:
            break;
        }
        break;
      }
      case SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_INGRESS_PORT: {
        return smi_id::P_NONIP_HASH_ING_PORT;
      }
      case SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_MAC_TYPE: {
        return smi_id::P_NONIP_HASH_MAC_TYPE;
      }
      case SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_SRC_MAC: {
        return smi_id::P_NONIP_HASH_SRC_MAC;
      }
      case SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_DST_MAC: {
        return smi_id::P_NONIP_HASH_DST_MAC;
      }
      default:
        break;
    }
    return 0;
  }

  switch_status_t fg_hash_info_get(switch_object_id_t &fg_hash_attr,
                                   uint32_t &start_bit,
                                   uint32_t &length,
                                   uint32_t &order,
                                   std::string &log_str) {
    uint8_t proto_mask = 0;
    uint16_t port_mask = 0, mac_type_mask = 0;
    uint32_t flow_label_mask = 0;
    switch_mac_addr_t mac_mask = {0};
    switch_ip_address_t ip_addr_mask = {};
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    memset(&ip_addr_mask, 0x00, sizeof(switch_ip_address_t));

    switch_enum_t fg_field_name = {0};
    status |= switch_store::v_get(
        fg_hash_attr, SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME, fg_field_name);
    status |= switch_store::v_get(
        fg_hash_attr, SWITCH_FINE_GRAINED_HASH_ATTR_SEQUENCE, order);

    switch (fg_field_name.enumdata) {
      case SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_SRC_ADDR:
        status |= switch_store::v_get(fg_hash_attr,
                                      SWITCH_FINE_GRAINED_HASH_ATTR_SRC_IP_MASK,
                                      ip_addr_mask);
        get_ip_addr_mask_info(ip_addr_mask, start_bit, length);
        log_str.append("src_addr ");
        break;

      case SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_DST_ADDR:
        status |= switch_store::v_get(fg_hash_attr,
                                      SWITCH_FINE_GRAINED_HASH_ATTR_DST_IP_MASK,
                                      ip_addr_mask);
        get_ip_addr_mask_info(ip_addr_mask, start_bit, length);
        log_str.append("dst_addr ");
        break;

      case SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_IP_PROTO:
        status |=
            switch_store::v_get(fg_hash_attr,
                                SWITCH_FINE_GRAINED_HASH_ATTR_IP_PROTO_MASK,
                                proto_mask);
        get_mask_info(proto_mask, start_bit, length);
        log_str.append("ip_proto ");
        break;

      case SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_SRC_PORT:
        status |=
            switch_store::v_get(fg_hash_attr,
                                SWITCH_FINE_GRAINED_HASH_ATTR_SRC_PORT_MASK,
                                port_mask);
        get_mask_info(port_mask, start_bit, length);
        log_str.append("src_port ");
        break;

      case SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_DST_PORT:
        status |=
            switch_store::v_get(fg_hash_attr,
                                SWITCH_FINE_GRAINED_HASH_ATTR_DST_PORT_MASK,
                                port_mask);
        get_mask_info(port_mask, start_bit, length);
        log_str.append("dst_port ");
        break;

      case SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_INGRESS_PORT:
        status |=
            switch_store::v_get(fg_hash_attr,
                                SWITCH_FINE_GRAINED_HASH_ATTR_INGRESS_PORT_MASK,
                                port_mask);
        get_mask_info(port_mask, start_bit, length);
        log_str.append("ingress_port ");
        break;

      case SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_MAC_TYPE:
        status |=
            switch_store::v_get(fg_hash_attr,
                                SWITCH_FINE_GRAINED_HASH_ATTR_MAC_TYPE_MASK,
                                mac_type_mask);
        get_mask_info(mac_type_mask, start_bit, length);
        log_str.append("mac_type ");
        break;

      case SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_SRC_MAC:
        status |= switch_store::v_get(
            fg_hash_attr, SWITCH_FINE_GRAINED_HASH_ATTR_SRC_MAC_MASK, mac_mask);
        get_mac_mask_info(mac_mask, start_bit, length);
        log_str.append("src_mac ");
        break;

      case SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_DST_MAC:
        status |=
            switch_store::v_get(fg_hash_attr,
                                SWITCH_FINE_GRAINED_HASH_ATTR_DEST_MAC_MASK,
                                mac_mask);
        get_mac_mask_info(mac_mask, start_bit, length);
        log_str.append("dst_mac ");
        break;

      case SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_IPV6_FLOW_LABEL:
        status |= switch_store::v_get(
            fg_hash_attr,
            SWITCH_FINE_GRAINED_HASH_ATTR_IPV6_FLOW_LABEL_MASK,
            flow_label_mask);
        get_mask_info(flow_label_mask, start_bit, length);
        log_str.append("ipv6_flow_label ");
        break;
    }
    return status;
  }

  switch_status_t program_fg_hash(
      std::vector<switch_object_id_t> &fg_hash_attr_list,
      std::vector<bfrt_container_data_t> &data_list,
      std::string &log_str) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint32_t priority = 1;
    uint32_t container_id = 0;

    for (auto fg_hash_attr : fg_hash_attr_list) {
      bfrt_container_data_t data;
      uint32_t start_bit = 0, length = 0, order = 0;

      switch_enum_t fg_field_name = {0};
      status |= switch_store::v_get(fg_hash_attr,
                                    SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME,
                                    fg_field_name);
      container_id = fg_hash_continer_id_get(fg_field_name.enumdata);
      if (container_id == 0) {
        switch_log(SWITCH_API_LEVEL_WARN,
                   SWITCH_OBJECT_TYPE_HASH,
                   "{}.{}: Invalid FG hash field for attribute {}",
                   __func__,
                   __LINE__,
                   fg_field_name.enumdata);
      }

      status |=
          fg_hash_info_get(fg_hash_attr, start_bit, length, order, log_str);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(
            SWITCH_API_LEVEL_ERROR,
            SWITCH_OBJECT_TYPE_HASH,
            "{}:{} Failed to retrive the ipv4 fg-hash attribute, status: {}",
            __func__,
            __LINE__,
            status);
        return status;
      }
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_HASH,
                 "Order: {}, container_id: {}, start_bit: {}, length: {}",
                 order,
                 container_id,
                 start_bit,
                 length);

      if (order == 0) {
        order = priority;
      }
      data.container_id = container_id;
      data.container_data_map[smi_id::P_HASH_CONTAINER_ORDER] = order;
      if (length != 0) {
        data.container_data_map[smi_id::P_HASH_CONTAINER_START_BIT] = start_bit;
        data.container_data_map[smi_id::P_HASH_CONTAINER_LENGTH] = length;
      }
      data_list.emplace_back(data);
      ++priority;
    }
    return status;
  }

  switch_status_t reset_hash_seed(switch_object_id_t hash_obj,
                                  switch_object_type_t ot) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_DEVICE_ECMP_HASH_ALGO;
    switch_object_id_t device_obj{};
    status |=
        switch_store::v_get(hash_obj, SWITCH_HASH_ATTR_DEVICE, device_obj);
    uint32_t seed = 0;
    switch_attr_id_t attr_id = SWITCH_DEVICE_ATTR_ECMP_DEFAULT_HASH_SEED;
    switch (ot) {
      case SWITCH_OBJECT_TYPE_IPV4_HASH:
      case SWITCH_OBJECT_TYPE_IPV6_HASH:
        // fall through
        attr_id = SWITCH_DEVICE_ATTR_ECMP_DEFAULT_HASH_SEED;
        auto_ot = SWITCH_OBJECT_TYPE_DEVICE_ECMP_HASH_ALGO;
        break;
      case SWITCH_OBJECT_TYPE_LAG_V4_HASH:
      case SWITCH_OBJECT_TYPE_LAG_V6_HASH:
      case SWITCH_OBJECT_TYPE_NON_IP_HASH:
        // fall through
        attr_id = SWITCH_DEVICE_ATTR_LAG_DEFAULT_HASH_SEED;
        auto_ot = SWITCH_OBJECT_TYPE_DEVICE_LAG_HASH_ALGO;
        break;
      default:
        return status;
    }
    status = switch_store::v_get(device_obj, attr_id, seed);
    CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);
    attr_w attr(attr_id);
    attr.v_set(seed);
    auto hash_seed =
        factory::get_instance().create(auto_ot, device_obj, status);
    if (status == SWITCH_STATUS_SUCCESS && hash_seed) {
      status = hash_seed->create_update();
    }
    return status;
  }
};

class ipv4_hash : public auto_object, public hash_utility {
 private:
  bool fghash = false;
  bool symmetric_hash = false;
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_IPV4_HASH;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_IPV4_HASH_ATTR_PARENT_HANDLE;
  std::vector<switch_object_id_t> fg_hash_attr_list;
  std::vector<uint32_t> hash_attr_list;

  // Program fine-grained IPv4 hash
  switch_status_t program_fine_grained_hash() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (false == validate_ipv4_hash()) {
      return status;
    }

    std::vector<bfrt_container_data_t> container_data_list;
    std::string log_str("Field_list: ");

    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_IPV4_HASH);
    status |= program_fg_hash(fg_hash_attr_list, container_data_list, log_str);
    table.dynamic_hash_field_set(container_data_list);
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_HASH,
               "fg_ipv4_hash: {}",
               log_str);

    return status;
  }

  // Program IPv4 hash
  switch_status_t program_hash() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (false == validate_ipv4_hash()) {
      return status;
    }

    std::string log_str("Field_list: ");
    std::vector<bfrt_container_data_t> container_data_list;

    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_IPV4_HASH);

    // Priority is one to one map with position of the element in the list
    uint32_t priority = 1;
    for (auto const hash_attr : hash_attr_list) {
      bfrt_container_data_t data;
      data.container_id = hash_continer_id_get(hash_attr);
      if (symmetric_hash) {
        if ((hash_attr == SWITCH_HASH_ATTR_FIELD_SRC_ADDR) ||
            (hash_attr == SWITCH_HASH_ATTR_FIELD_DST_ADDR)) {
          data.container_data_map[smi_id::P_HASH_CONTAINER_ORDER] = 20;
        } else if ((hash_attr == SWITCH_HASH_ATTR_FIELD_SRC_PORT) ||
                   (hash_attr == SWITCH_HASH_ATTR_FIELD_DST_PORT)) {
          data.container_data_map[smi_id::P_HASH_CONTAINER_ORDER] = 40;
        } else {
          data.container_data_map[smi_id::P_HASH_CONTAINER_ORDER] = priority;
        }
      } else {
        data.container_data_map[smi_id::P_HASH_CONTAINER_ORDER] = priority;
      }
      container_data_list.emplace_back(data);
      ++priority;
    }

    table.dynamic_hash_field_set(container_data_list);
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_HASH,
               "ipv4_hash: {}",
               log_str);

    return status;
  }

 public:
  ipv4_hash(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    ENTER();
    // Extracting the type of hash
    switch_enum_t e = {0};
    status |= switch_store::v_get(parent, SWITCH_HASH_ATTR_TYPE, e);
    switch_hash_attr_type_set(static_cast<switch_hash_attr_type>(e.enumdata));

    if (false == validate_ipv4_hash()) {
      return;
    }

    // Extractig fine grained attribute list
    status |= switch_store::v_get(
        parent, SWITCH_HASH_ATTR_FINE_GRAINED_FIELD_LIST, fg_hash_attr_list);
    fghash = !fg_hash_attr_list.empty();

    // Extracting the hash attribute list
    status |= switch_store::v_get(
        parent, SWITCH_HASH_ATTR_FIELD_LIST, hash_attr_list);

    status |= switch_store::v_get(
        parent, SWITCH_HASH_ATTR_SYMMETRIC_HASH, symmetric_hash);
  }  // End of ipv4_hash

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    // Creation of the oject in s3
    status = auto_object::create_update();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_HASH,
                 "{}: auto_obj.create_update failure status {}",
                 __func__,
                 status);
      return status;
    }

    // Program the attribute
    if (fghash) {
      status |= program_fine_grained_hash();
    } else {
      status |= program_hash();
    }

    status |= reset_hash_seed(get_parent(), auto_ot);
    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (validate_ipv4_hash()) {
      hash_attr_list.clear();
      hash_attr_list.push_back(SWITCH_HASH_ATTR_FIELD_SRC_ADDR);
      hash_attr_list.push_back(SWITCH_HASH_ATTR_FIELD_DST_ADDR);
      hash_attr_list.push_back(SWITCH_HASH_ATTR_FIELD_IP_PROTO);
      hash_attr_list.push_back(SWITCH_HASH_ATTR_FIELD_DST_PORT);
      hash_attr_list.push_back(SWITCH_HASH_ATTR_FIELD_SRC_PORT);
      status |= program_hash();
    }

    // Deleting the auto object
    status |= auto_object::del();

    return status;
  }
};

class ipv6_hash : public auto_object, public hash_utility {
 private:
  bool fghash = false;
  bool symmetric_hash = false;
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_IPV6_HASH;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_IPV6_HASH_ATTR_PARENT_HANDLE;
  std::vector<switch_object_id_t> fg_hash_attr_list;
  std::vector<uint32_t> hash_attr_list;

  // Program fine-grained IPv6 hash
  switch_status_t program_fine_grained_hash() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (false == validate_ipv6_hash()) {
      return status;
    }

    std::vector<bfrt_container_data_t> container_data_list;
    std::string log_str("Field_list: ");

    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_IPV6_HASH);
    status |= program_fg_hash(fg_hash_attr_list, container_data_list, log_str);
    table.dynamic_hash_field_set(container_data_list);
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_HASH,
               "fg_ipv6_hash: {}",
               log_str);

    return status;
  }

  // Program IPv6 hash
  switch_status_t program_hash() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (false == validate_ipv6_hash()) {
      return status;
    }

    std::string log_str("Field_list: ");
    std::vector<bfrt_container_data_t> container_data_list;

    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_IPV6_HASH);

    // Priority is one to one map with position of the element in the list
    uint32_t priority = 1;
    for (auto const hash_attr : hash_attr_list) {
      bfrt_container_data_t data;
      data.container_id = hash_continer_id_get(hash_attr);
      if (symmetric_hash) {
        if ((hash_attr == SWITCH_HASH_ATTR_FIELD_SRC_ADDR) ||
            (hash_attr == SWITCH_HASH_ATTR_FIELD_DST_ADDR)) {
          data.container_data_map[smi_id::P_HASH_CONTAINER_ORDER] = 20;
        } else if ((hash_attr == SWITCH_HASH_ATTR_FIELD_SRC_PORT) ||
                   (hash_attr == SWITCH_HASH_ATTR_FIELD_DST_PORT)) {
          data.container_data_map[smi_id::P_HASH_CONTAINER_ORDER] = 40;
        } else {
          data.container_data_map[smi_id::P_HASH_CONTAINER_ORDER] = priority;
        }
      } else {
        data.container_data_map[smi_id::P_HASH_CONTAINER_ORDER] = priority;
      }
      container_data_list.emplace_back(data);
      ++priority;
    }

    table.dynamic_hash_field_set(container_data_list);
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_HASH,
               "ipv6_hash: {}",
               log_str);

    return status;
  }

 public:
  ipv6_hash(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    ENTER();

    // Extracting the type of hash
    switch_enum_t e = {0};
    status |= switch_store::v_get(parent, SWITCH_HASH_ATTR_TYPE, e);
    switch_hash_attr_type_set(static_cast<switch_hash_attr_type>(e.enumdata));

    if (false == validate_ipv6_hash()) {
      return;
    }

    // Extractig fine grained attribute list
    status |= switch_store::v_get(
        parent, SWITCH_HASH_ATTR_FINE_GRAINED_FIELD_LIST, fg_hash_attr_list);
    fghash = !fg_hash_attr_list.empty();

    // Extracting the hash attribute list
    status |= switch_store::v_get(
        parent, SWITCH_HASH_ATTR_FIELD_LIST, hash_attr_list);

    status |= switch_store::v_get(
        parent, SWITCH_HASH_ATTR_SYMMETRIC_HASH, symmetric_hash);
  }  // End of ipv6_hash

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    // Creation of the oject in s3
    status = auto_object::create_update();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_HASH,
                 "{}: auto_obj.create_update failure status {}",
                 __func__,
                 status);
      return status;
    }

    // Program the attribute
    if (fghash) {
      status |= program_fine_grained_hash();
    } else {
      status |= program_hash();
    }

    status |= reset_hash_seed(get_parent(), auto_ot);
    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (validate_ipv6_hash()) {
      hash_attr_list.clear();
      hash_attr_list.push_back(SWITCH_HASH_ATTR_FIELD_SRC_ADDR);
      hash_attr_list.push_back(SWITCH_HASH_ATTR_FIELD_DST_ADDR);
      hash_attr_list.push_back(SWITCH_HASH_ATTR_FIELD_IP_PROTO);
      hash_attr_list.push_back(SWITCH_HASH_ATTR_FIELD_DST_PORT);
      hash_attr_list.push_back(SWITCH_HASH_ATTR_FIELD_SRC_PORT);
      status |= program_hash();
    }

    // Deleting the auto object
    status |= auto_object::del();

    return status;
  }
};

class non_ip_hash : public auto_object, public hash_utility {
 private:
  bool fghash = false;
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_NON_IP_HASH;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_NON_IP_HASH_ATTR_PARENT_HANDLE;
  std::vector<switch_object_id_t> fg_hash_attr_list;
  std::vector<uint32_t> hash_attr_list;

  // Program fine-grained non-ip hash
  switch_status_t program_fine_grained_hash() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (false == validate_nonip_hash()) {
      return status;
    }

    std::vector<bfrt_container_data_t> container_data_list;
    std::string log_str("Field_list: ");

    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_NON_IP_HASH);
    status |= program_fg_hash(fg_hash_attr_list, container_data_list, log_str);
    table.dynamic_hash_field_set(container_data_list);
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_HASH,
               "fg_non_ip_hash: {}",
               log_str);

    return status;
  }

  // Program non-ip hash
  switch_status_t program_hash() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (false == validate_nonip_hash()) {
      return status;
    }

    std::string log_str("Field_list: ");
    std::vector<bfrt_container_data_t> container_data_list;

    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_NON_IP_HASH);

    // Priority is one to one map with position of the element in the list
    uint32_t priority = 1;
    for (auto const hash_attr : hash_attr_list) {
      bfrt_container_data_t data;
      data.container_id = hash_continer_id_get(hash_attr);
      data.container_data_map[smi_id::P_HASH_CONTAINER_ORDER] = priority;
      container_data_list.emplace_back(data);
      ++priority;
    }

    table.dynamic_hash_field_set(container_data_list);
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_HASH,
               "nonip_hash: {}",
               log_str);

    return status;
  }

 public:
  non_ip_hash(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    ENTER();
    std::string log_str("Field_list: ");

    // Extracting the type of hash
    switch_enum_t e = {0};
    status |= switch_store::v_get(parent, SWITCH_HASH_ATTR_TYPE, e);
    switch_hash_attr_type_set(static_cast<switch_hash_attr_type>(e.enumdata));

    if (false == validate_nonip_hash()) {
      return;
    }

    // Extractig fine grained attribute list
    status |= switch_store::v_get(
        parent, SWITCH_HASH_ATTR_FINE_GRAINED_FIELD_LIST, fg_hash_attr_list);
    fghash = !fg_hash_attr_list.empty();

    // Extracting the hash attribute list
    status |= switch_store::v_get(
        parent, SWITCH_HASH_ATTR_FIELD_LIST, hash_attr_list);
  }  // End of non_ip_hash

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    // Creation of the oject in s3
    status = auto_object::create_update();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_HASH,
                 "{}: auto_obj.create_update failure status {}",
                 __func__,
                 status);
      return status;
    }

    // Program the attribute
    if (fghash) {
      status |= program_fine_grained_hash();
    } else {
      status |= program_hash();
    }

    status |= reset_hash_seed(get_parent(), auto_ot);
    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (validate_nonip_hash()) {
      hash_attr_list.clear();
      hash_attr_list.push_back(SWITCH_HASH_ATTR_FIELD_INGRESS_PORT);
      hash_attr_list.push_back(SWITCH_HASH_ATTR_FIELD_MAC_TYPE);
      hash_attr_list.push_back(SWITCH_HASH_ATTR_FIELD_SRC_MAC);
      hash_attr_list.push_back(SWITCH_HASH_ATTR_FIELD_DST_MAC);
      status |= program_hash();
    }

    // Deleting the auto object
    status |= auto_object::del();

    return status;
  }
};

class lag_v4_hash : public auto_object, public hash_utility {
 private:
  bool fghash = false;
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_LAG_V4_HASH;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_LAG_V4_HASH_ATTR_PARENT_HANDLE;
  std::vector<switch_object_id_t> fg_hash_attr_list;
  std::vector<uint32_t> hash_attr_list;

  // Program fine-grained LAG v4 hash
  switch_status_t program_fine_grained_hash() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (false == validate_lag_v4_hash()) {
      return status;
    }

    std::vector<bfrt_container_data_t> container_data_list;
    std::string log_str("Field_list: ");

    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_LAG_V4_HASH);
    status |= program_fg_hash(fg_hash_attr_list, container_data_list, log_str);
    table.dynamic_hash_field_set(container_data_list);
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_HASH,
               "fg_lag_v4_hash: {}",
               log_str);

    return status;
  }

  // Program LAG v4 hash
  switch_status_t program_hash() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (false == validate_lag_v4_hash()) {
      return status;
    }

    std::string log_str("Field_list: ");
    std::vector<bfrt_container_data_t> container_data_list;

    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_LAG_V4_HASH);

    // Priority is one to one map with position of the element in the list
    uint32_t priority = 1;
    for (auto const hash_attr : hash_attr_list) {
      bfrt_container_data_t data;
      data.container_id = hash_continer_id_get(hash_attr);
      data.container_data_map[smi_id::P_HASH_CONTAINER_ORDER] = priority;
      container_data_list.emplace_back(data);
      ++priority;
    }

    table.dynamic_hash_field_set(container_data_list);
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_HASH,
               "lag_v4_hash: {}",
               log_str);

    return status;
  }

 public:
  lag_v4_hash(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    ENTER();

    // Extracting the type of hash
    switch_enum_t e = {0};
    status |= switch_store::v_get(parent, SWITCH_HASH_ATTR_TYPE, e);
    switch_hash_attr_type_set(static_cast<switch_hash_attr_type>(e.enumdata));

    if (false == validate_lag_v4_hash()) {
      return;
    }

    // Extractig fine grained attribute list
    status |= switch_store::v_get(
        parent, SWITCH_HASH_ATTR_FINE_GRAINED_FIELD_LIST, fg_hash_attr_list);
    fghash = !fg_hash_attr_list.empty();

    // Extracting the hash attribute list
    status |= switch_store::v_get(
        parent, SWITCH_HASH_ATTR_FIELD_LIST, hash_attr_list);
  }  // End of lag_v4_hash

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    // Creation of the oject in s3
    status = auto_object::create_update();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_HASH,
                 "{}: auto_obj.create_update failure status {}",
                 __func__,
                 status);
      return status;
    }

    // Program the attribute
    if (fghash) {
      status |= program_fine_grained_hash();
    } else {
      status |= program_hash();
    }
    status |= reset_hash_seed(get_parent(), auto_ot);

    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (validate_lag_v4_hash()) {
      hash_attr_list.clear();
      hash_attr_list.push_back(SWITCH_HASH_ATTR_FIELD_SRC_ADDR);
      hash_attr_list.push_back(SWITCH_HASH_ATTR_FIELD_DST_ADDR);
      hash_attr_list.push_back(SWITCH_HASH_ATTR_FIELD_IP_PROTO);
      hash_attr_list.push_back(SWITCH_HASH_ATTR_FIELD_DST_PORT);
      hash_attr_list.push_back(SWITCH_HASH_ATTR_FIELD_SRC_PORT);
      status |= program_hash();
    }

    // Deleting the auto object
    status |= auto_object::del();

    return status;
  }
};

class lag_v6_hash : public auto_object, public hash_utility {
 private:
  bool fghash = false;
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_LAG_V6_HASH;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_LAG_V6_HASH_ATTR_PARENT_HANDLE;
  std::vector<switch_object_id_t> fg_hash_attr_list;
  std::vector<uint32_t> hash_attr_list;

  // Program fine-grained LAG v6 hash
  switch_status_t program_fine_grained_hash() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (false == validate_lag_v6_hash()) {
      return status;
    }

    std::vector<bfrt_container_data_t> container_data_list;
    std::string log_str("Field_list: ");

    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_LAG_V6_HASH);
    status |= program_fg_hash(fg_hash_attr_list, container_data_list, log_str);
    table.dynamic_hash_field_set(container_data_list);
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_HASH,
               "fg_lag_v6_hash: {}",
               log_str);

    return status;
  }

  // Program LAG v6 hash
  switch_status_t program_hash() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (false == validate_lag_v6_hash()) {
      return status;
    }

    std::string log_str("Field_list: ");
    std::vector<bfrt_container_data_t> container_data_list;

    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_LAG_V6_HASH);

    // Priority is one to one map with position of the element in the list
    uint32_t priority = 1;
    for (auto const hash_attr : hash_attr_list) {
      bfrt_container_data_t data;
      data.container_id = hash_continer_id_get(hash_attr);
      data.container_data_map[smi_id::P_HASH_CONTAINER_ORDER] = priority;
      container_data_list.emplace_back(data);
      ++priority;
    }

    table.dynamic_hash_field_set(container_data_list);
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_HASH,
               "lag_v6_hash: {}",
               log_str);

    return status;
  }

 public:
  lag_v6_hash(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    ENTER();

    // Extracting the type of hash
    switch_enum_t e = {0};
    status |= switch_store::v_get(parent, SWITCH_HASH_ATTR_TYPE, e);
    switch_hash_attr_type_set(static_cast<switch_hash_attr_type>(e.enumdata));

    if (false == validate_lag_v6_hash()) {
      return;
    }

    // Extractig fine grained attribute list
    status |= switch_store::v_get(
        parent, SWITCH_HASH_ATTR_FINE_GRAINED_FIELD_LIST, fg_hash_attr_list);
    fghash = !fg_hash_attr_list.empty();

    // Extracting the hash attribute list
    status |= switch_store::v_get(
        parent, SWITCH_HASH_ATTR_FIELD_LIST, hash_attr_list);
  }  // End of lag_v6_hash

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    // Creation of the oject in s3
    status = auto_object::create_update();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_HASH,
                 "{}: auto_obj.create_update failure status {}",
                 __func__,
                 status);
      return status;
    }

    // Program the attribute
    if (fghash) {
      status |= program_fine_grained_hash();
    } else {
      status |= program_hash();
    }
    status |= reset_hash_seed(get_parent(), auto_ot);

    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (false == validate_lag_v6_hash()) {
      hash_attr_list.clear();
      hash_attr_list.push_back(SWITCH_HASH_ATTR_FIELD_SRC_ADDR);
      hash_attr_list.push_back(SWITCH_HASH_ATTR_FIELD_DST_ADDR);
      hash_attr_list.push_back(SWITCH_HASH_ATTR_FIELD_IP_PROTO);
      hash_attr_list.push_back(SWITCH_HASH_ATTR_FIELD_DST_PORT);
      hash_attr_list.push_back(SWITCH_HASH_ATTR_FIELD_SRC_PORT);
      status |= program_hash();
    }

    // Deleting the auto object
    status |= auto_object::del();

    return status;
  }
};

class ecmp_default_hash_offset : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_ECMP_DEFAULT_HASH_OFFSET;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_ECMP_DEFAULT_HASH_OFFSET_ATTR_PARENT_HANDLE;

 public:
  ecmp_default_hash_offset(const switch_object_id_t parent,
                           switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    uint8_t hash_offset = 0;
    // ECMP rotate hash table
    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_ROTATE_HASH);
    _ActionEntry action_entry(smi_id::T_ROTATE_HASH);
    status |= switch_store::v_get(
        parent, SWITCH_DEVICE_ATTR_ECMP_DEFAULT_HASH_OFFSET, hash_offset);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_DEVICE,
                 "Failed to get the attribute for ecmp hash offset status {}",
                 status);
      return;
    }

    if (0 == hash_offset) {
      action_entry.init_action_data(smi_id::A_ROTATE_BY_0);
    } else if (1 == hash_offset) {
      action_entry.init_action_data(smi_id::A_ROTATE_BY_1);
    } else if (2 == hash_offset) {
      action_entry.init_action_data(smi_id::A_ROTATE_BY_2);
    } else if (3 == hash_offset) {
      action_entry.init_action_data(smi_id::A_ROTATE_BY_3);
    } else if (4 == hash_offset) {
      action_entry.init_action_data(smi_id::A_ROTATE_BY_4);
    } else if (5 == hash_offset) {
      action_entry.init_action_data(smi_id::A_ROTATE_BY_5);
    } else if (6 == hash_offset) {
      action_entry.init_action_data(smi_id::A_ROTATE_BY_6);
    } else if (7 == hash_offset) {
      action_entry.init_action_data(smi_id::A_ROTATE_BY_7);
    } else if (8 == hash_offset) {
      action_entry.init_action_data(smi_id::A_ROTATE_BY_8);
    } else if (9 == hash_offset) {
      action_entry.init_action_data(smi_id::A_ROTATE_BY_9);
    } else if (10 == hash_offset) {
      action_entry.init_action_data(smi_id::A_ROTATE_BY_10);
    } else if (11 == hash_offset) {
      action_entry.init_action_data(smi_id::A_ROTATE_BY_11);
    } else if (12 == hash_offset) {
      action_entry.init_action_data(smi_id::A_ROTATE_BY_12);
    } else if (13 == hash_offset) {
      action_entry.init_action_data(smi_id::A_ROTATE_BY_13);
    } else if (14 == hash_offset) {
      action_entry.init_action_data(smi_id::A_ROTATE_BY_14);
    } else if (15 == hash_offset) {
      action_entry.init_action_data(smi_id::A_ROTATE_BY_15);
    }
    status |= table.default_entry_set(action_entry, false);
  }
};

class inner_dtel_v4_hash : public auto_object, public hash_utility {
 private:
  bool fghash = false;
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_INNER_DTEL_V4_HASH;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INNER_DTEL_V4_HASH_ATTR_PARENT_HANDLE;
  std::vector<switch_object_id_t> fg_hash_attr_list;
  std::vector<uint32_t> hash_attr_list;

  // Program fine-grained inner dtel v4 hash
  switch_status_t program_fine_grained_hash() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (false == validate_inner_dtel_v4_hash()) {
      return status;
    }

    std::vector<bfrt_container_data_t> container_data_list;
    std::string log_str("Field_list: ");

    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_INNER_DTEL_V4_HASH);
    status |= program_fg_hash(fg_hash_attr_list, container_data_list, log_str);
    table.dynamic_hash_field_set(container_data_list);
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_HASH,
               "fg_inner_dtel_v4_hash: {}",
               log_str);

    return status;
  }
  // Program inner dtel v4 hash
  switch_status_t program_hash() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (false == validate_inner_dtel_v4_hash()) {
      return status;
    }

    std::string log_str("Field_list: ");
    std::vector<bfrt_container_data_t> container_data_list;

    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_INNER_DTEL_V4_HASH);

    // Priority is one to one map with position of the element in the list
    uint32_t priority = 1;
    for (auto const hash_attr : hash_attr_list) {
      bfrt_container_data_t data;
      data.container_id = hash_continer_id_get(hash_attr);
      if (data.container_id == 0) {
        switch_log(SWITCH_API_LEVEL_WARN,
                   SWITCH_OBJECT_TYPE_HASH,
                   "{}.{}: Invalid hash field for attribute {}",
                   __func__,
                   __LINE__,
                   hash_attr);
      }
      data.container_data_map[smi_id::P_HASH_CONTAINER_ORDER] = priority;
      container_data_list.emplace_back(data);
      ++priority;
    }

    table.dynamic_hash_field_set(container_data_list);
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_HASH,
               "inner_dtel_v4_hash: {}",
               log_str);

    return status;
  }

 public:
  inner_dtel_v4_hash(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    ENTER();

    // Extracting the type of hash
    switch_enum_t e = {0};
    status |= switch_store::v_get(parent, SWITCH_HASH_ATTR_TYPE, e);
    switch_hash_attr_type_set(static_cast<switch_hash_attr_type>(e.enumdata));

    if (false == validate_inner_dtel_v4_hash()) {
      return;
    }

    // Extractig fine grained attribute list
    status |= switch_store::v_get(
        parent, SWITCH_HASH_ATTR_FINE_GRAINED_FIELD_LIST, fg_hash_attr_list);
    fghash = !fg_hash_attr_list.empty();

    // Extracting the hash attribute list
    status |= switch_store::v_get(
        parent, SWITCH_HASH_ATTR_FIELD_LIST, hash_attr_list);
  }  // End of inner_dtel_v4_hash

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    // Creation of the oject in s3
    status = auto_object::create_update();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_HASH,
                 "{}: auto_obj.create_update failure status {}",
                 __func__,
                 status);
      return status;
    }
    // Program the attribute
    if (fghash) {
      status |= program_fine_grained_hash();
    } else {
      status |= program_hash();
    }

    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (validate_inner_dtel_v4_hash()) {
      hash_attr_list.clear();
      hash_attr_list.push_back(SWITCH_HASH_ATTR_FIELD_SRC_ADDR);
      hash_attr_list.push_back(SWITCH_HASH_ATTR_FIELD_DST_ADDR);
      hash_attr_list.push_back(SWITCH_HASH_ATTR_FIELD_IP_PROTO);
      // hash_attr_list.push_back(SWITCH_HASH_ATTR_FIELD_DST_PORT);
      hash_attr_list.push_back(SWITCH_HASH_ATTR_FIELD_SRC_PORT);
      status |= program_hash();
    }

    // Deleting the auto object
    status |= auto_object::del();

    return status;
  }
};

class inner_dtel_v6_hash : public auto_object, public hash_utility {
 private:
  bool fghash = false;
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_INNER_DTEL_V6_HASH;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INNER_DTEL_V6_HASH_ATTR_PARENT_HANDLE;
  std::vector<switch_object_id_t> fg_hash_attr_list;
  std::vector<uint32_t> hash_attr_list;

  // Program fine-grained inner dtel v6 hash
  switch_status_t program_fine_grained_hash() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (false == validate_inner_dtel_v6_hash()) {
      return status;
    }

    std::vector<bfrt_container_data_t> container_data_list;
    std::string log_str("Field_list: ");

    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_INNER_DTEL_V6_HASH);
    status |= program_fg_hash(fg_hash_attr_list, container_data_list, log_str);
    table.dynamic_hash_field_set(container_data_list);
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_HASH,
               "fg_lag_v6_hash: {}",
               log_str);

    return status;
  }
  // Program inner v6 hash
  switch_status_t program_hash() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (false == validate_inner_dtel_v6_hash()) {
      return status;
    }

    std::string log_str("Field_list: ");
    std::vector<bfrt_container_data_t> container_data_list;

    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_INNER_DTEL_V6_HASH);

    // Priority is one to one map with position of the element in the list
    uint32_t priority = 1;
    for (auto const hash_attr : hash_attr_list) {
      bfrt_container_data_t data;
      data.container_id = hash_continer_id_get(hash_attr);
      if (data.container_id == 0) {
        switch_log(SWITCH_API_LEVEL_WARN,
                   SWITCH_OBJECT_TYPE_HASH,
                   "{}.{}: Invalid hash field for attribute {}",
                   __func__,
                   __LINE__,
                   hash_attr);
      }
      data.container_data_map[smi_id::P_HASH_CONTAINER_ORDER] = priority;
      container_data_list.emplace_back(data);
      ++priority;
    }

    table.dynamic_hash_field_set(container_data_list);
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_HASH,
               "inner_dtel_v6_hash: {}",
               log_str);

    return status;
  }

 public:
  inner_dtel_v6_hash(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    ENTER();

    // Extracting the type of hash
    switch_enum_t e = {0};
    status |= switch_store::v_get(parent, SWITCH_HASH_ATTR_TYPE, e);
    switch_hash_attr_type_set(static_cast<switch_hash_attr_type>(e.enumdata));

    if (false == validate_inner_dtel_v6_hash()) {
      return;
    }

    // Extractig fine grained attribute list
    status |= switch_store::v_get(
        parent, SWITCH_HASH_ATTR_FINE_GRAINED_FIELD_LIST, fg_hash_attr_list);
    fghash = !fg_hash_attr_list.empty();

    // Extracting the hash attribute list
    status |= switch_store::v_get(
        parent, SWITCH_HASH_ATTR_FIELD_LIST, hash_attr_list);
  }  // End of inner_dtel_v6_hash

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    // Creation of the object in s3
    status = auto_object::create_update();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_HASH,
                 "{}: auto_obj.create_update failure status {}",
                 __func__,
                 status);
      return status;
    }

    // Program the attribute
    if (fghash) {
      status |= program_fine_grained_hash();
    } else {
      status |= program_hash();
    }

    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (false == validate_inner_dtel_v6_hash()) {
      hash_attr_list.clear();
      hash_attr_list.push_back(SWITCH_HASH_ATTR_FIELD_SRC_ADDR);
      hash_attr_list.push_back(SWITCH_HASH_ATTR_FIELD_DST_ADDR);
      hash_attr_list.push_back(SWITCH_HASH_ATTR_FIELD_IP_PROTO);
      // hash_attr_list.push_back(SWITCH_HASH_ATTR_FIELD_DST_PORT);
      hash_attr_list.push_back(SWITCH_HASH_ATTR_FIELD_SRC_PORT);
      status |= program_hash();
    }

    // Deleting the auto object
    status |= auto_object::del();

    return status;
  }
};

switch_status_t hash_initialize() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  REGISTER_OBJECT(ipv4_hash, SWITCH_OBJECT_TYPE_IPV4_HASH);
  REGISTER_OBJECT(ipv6_hash, SWITCH_OBJECT_TYPE_IPV6_HASH);
  REGISTER_OBJECT(non_ip_hash, SWITCH_OBJECT_TYPE_NON_IP_HASH);
  REGISTER_OBJECT(lag_v4_hash, SWITCH_OBJECT_TYPE_LAG_V4_HASH);
  REGISTER_OBJECT(lag_v6_hash, SWITCH_OBJECT_TYPE_LAG_V6_HASH);
  // REGISTER_OBJECT(ecmp_default_hash_offset,
  //                SWITCH_OBJECT_TYPE_ECMP_DEFAULT_HASH_OFFSET);
  REGISTER_OBJECT(inner_dtel_v4_hash, SWITCH_OBJECT_TYPE_INNER_DTEL_V4_HASH);
  REGISTER_OBJECT(inner_dtel_v6_hash, SWITCH_OBJECT_TYPE_INNER_DTEL_V6_HASH);
  return status;
}

switch_status_t hash_clean() {
  ENTER();
  return SWITCH_STATUS_SUCCESS;
}

} /* namespace smi */
