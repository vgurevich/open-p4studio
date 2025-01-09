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


#include <netinet/in.h>
#include <arpa/inet.h>

#include <vector>
#include <set>
#include <unordered_map>
#include <utility>

#include "s3/switch_packet.h"
#include "s3/switch_bfdd.h"
#include "common/utils.h"
#include "common/hostif.h"

namespace smi {
using ::smi::logging::switch_log;

/*
 *  hostif_filter_default-->host_interface-->hostif
 */
class hostif_filter_default : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_HOSTIF_FILTER_DEFAULT;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_HOSTIF_FILTER_DEFAULT_ATTR_PARENT_HANDLE;

 public:
  hostif_filter_default(const switch_object_id_t parent,
                        switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    (void)status;
  }

  switch_status_t hostif_handle_to_rx_filter_type(
      switch_object_type_t handle_type, switch_enum_t &filter_type) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch (handle_type) {
      case SWITCH_OBJECT_TYPE_PORT:
        filter_type.enumdata = SWITCH_HOSTIF_RX_FILTER_ATTR_TYPE_PORT;
        break;
      case SWITCH_OBJECT_TYPE_LAG:
        filter_type.enumdata = SWITCH_HOSTIF_RX_FILTER_ATTR_TYPE_LAG;
        break;
      case SWITCH_OBJECT_TYPE_VLAN:
        filter_type.enumdata = SWITCH_HOSTIF_RX_FILTER_ATTR_TYPE_VLAN;
        break;
      case SWITCH_OBJECT_TYPE_RIF:
        filter_type.enumdata = SWITCH_HOSTIF_RX_FILTER_ATTR_TYPE_RIF;
        break;

      default:
        status |= SWITCH_STATUS_NOT_IMPLEMENTED;
        return status;
    }

    return status;
  }

  switch_status_t hostif_handle_to_tx_filter_type(
      switch_object_type_t handle_type, switch_enum_t &filter_type) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch (handle_type) {
      case SWITCH_OBJECT_TYPE_PORT:
        filter_type.enumdata = SWITCH_HOSTIF_TX_FILTER_ATTR_TYPE_PORT;
        break;
      case SWITCH_OBJECT_TYPE_LAG:
        filter_type.enumdata = SWITCH_HOSTIF_TX_FILTER_ATTR_TYPE_LAG;
        break;
      case SWITCH_OBJECT_TYPE_RIF:
        filter_type.enumdata = SWITCH_HOSTIF_TX_FILTER_ATTR_TYPE_RIF;
        break;
      case SWITCH_OBJECT_TYPE_VLAN:
        filter_type.enumdata = SWITCH_HOSTIF_TX_FILTER_ATTR_TYPE_VLAN;
        break;
      default:
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_HOSTIF,
                   "{}:{}: Invalid hostif target handle type {}",
                   __func__,
                   __LINE__,
                   handle_type);
        return SWITCH_STATUS_INVALID_PARAMETER;
    }

    return status;
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t type = {};
    switch_object_id_t switch_hostif_handle = {},
                       switch_hostif_target_handle = {};
    switch_object_type_t handle_type;

    status |= switch_store::v_get(get_parent(),
                                  SWITCH_HOST_INTERFACE_ATTR_PARENT_HANDLE,
                                  switch_hostif_handle);
    status |= switch_store::v_get(switch_hostif_handle,
                                  SWITCH_HOSTIF_ATTR_HANDLE,
                                  switch_hostif_target_handle);
    handle_type = switch_store::object_type_query(switch_hostif_target_handle);

    if ((get_auto_oid().data != 0) &&
        !switch_store::smiContext::context().in_warm_init()) {
      /* update case. ignore this */
      return set_link_up(
          switch_hostif_handle, switch_hostif_target_handle, handle_type);
    }

    status |= switch_store::v_get(
        switch_hostif_handle, SWITCH_HOSTIF_ATTR_TYPE, type);
    // rx filter genetlink comes from application
    if (type.enumdata == SWITCH_HOSTIF_ATTR_TYPE_GENETLINK) {
      return status;
    }

    if (get_auto_oid().data == 0) {
      std::set<attr_w> rx_filter_attrs;
      switch_object_id_t switch_hostif_device_handle;
      switch_object_id_t rx_filter_default_handle;
      switch_enum_t filter_type = {}, channel_type = {};

      status |= hostif_handle_to_rx_filter_type(handle_type, filter_type);
      channel_type.enumdata = SWITCH_HOSTIF_RX_FILTER_ATTR_CHANNEL_TYPE_PORT;

      status |= switch_store::v_get(switch_hostif_handle,
                                    SWITCH_HOSTIF_ATTR_DEVICE,
                                    switch_hostif_device_handle);
      /* create default rx_filter */
      rx_filter_attrs.insert(attr_w(SWITCH_HOSTIF_RX_FILTER_ATTR_DEVICE,
                                    switch_hostif_device_handle));
      rx_filter_attrs.insert(attr_w(SWITCH_HOSTIF_RX_FILTER_ATTR_HANDLE,
                                    switch_hostif_target_handle));
      rx_filter_attrs.insert(
          attr_w(SWITCH_HOSTIF_RX_FILTER_ATTR_HOSTIF, switch_hostif_handle));
      rx_filter_attrs.insert(
          attr_w(SWITCH_HOSTIF_RX_FILTER_ATTR_TYPE, filter_type));
      rx_filter_attrs.insert(
          attr_w(SWITCH_HOSTIF_RX_FILTER_ATTR_CHANNEL_TYPE, channel_type));

      status |= switch_store::object_create(SWITCH_OBJECT_TYPE_HOSTIF_RX_FILTER,
                                            rx_filter_attrs,
                                            rx_filter_default_handle);
      status |=
          switch_store::v_set(rx_filter_default_handle,
                              SWITCH_HOSTIF_RX_FILTER_ATTR_INTERNAL_OBJECT,
                              true);

      std::set<attr_w> tx_filter_attrs;
      switch_object_id_t tx_filter_default_handle;

      /* create default tx_filter */
      tx_filter_attrs.insert(attr_w(SWITCH_HOSTIF_TX_FILTER_ATTR_DEVICE,
                                    switch_hostif_device_handle));
      tx_filter_attrs.insert(
          attr_w(SWITCH_HOSTIF_TX_FILTER_ATTR_HOSTIF, switch_hostif_handle));
      tx_filter_attrs.insert(attr_w(SWITCH_HOSTIF_TX_FILTER_ATTR_HANDLE,
                                    switch_hostif_target_handle));
      status |= hostif_handle_to_tx_filter_type(handle_type, filter_type);
      tx_filter_attrs.insert(
          attr_w(SWITCH_HOSTIF_TX_FILTER_ATTR_TYPE, filter_type));
      status |= switch_store::object_create(SWITCH_OBJECT_TYPE_HOSTIF_TX_FILTER,
                                            tx_filter_attrs,
                                            tx_filter_default_handle);
      status |=
          switch_store::v_set(tx_filter_default_handle,
                              SWITCH_HOSTIF_TX_FILTER_ATTR_INTERNAL_OBJECT,
                              true);

      /* create hostif_filter_default object. */
      attrs.insert(attr_w(SWITCH_HOSTIF_FILTER_DEFAULT_ATTR_RX_HANDLE,
                          rx_filter_default_handle.data));
      attrs.insert(attr_w(SWITCH_HOSTIF_FILTER_DEFAULT_ATTR_TX_HANDLE,
                          tx_filter_default_handle.data));
      auto_object::create_update();
    }

    // do not enable hostifs yet. The packet driver is not ready
    if (switch_store::smiContext::context().in_warm_init()) return status;

    return set_link_up(
        switch_hostif_handle, switch_hostif_target_handle, handle_type);
  }

  switch_status_t set_link_up(switch_object_id_t switch_hostif_handle,
                              switch_object_id_t switch_hostif_target_handle,
                              switch_object_type_t handle_type) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_string_t intf_name{};
    status |= switch_store::v_get(
        switch_hostif_handle, SWITCH_HOSTIF_ATTR_NAME, intf_name);
    status |=
        switch_pkt_hostif_set_interface_admin_state(0, intf_name.text, true);
    bool oper_status = false;
    status |= switch_store::v_get(
        switch_hostif_handle, SWITCH_HOSTIF_ATTR_OPER_STATUS, oper_status);
    status |= switch_pkt_hostif_set_interface_oper_state(
        0, intf_name.text, oper_status);
    uint16_t dev_port = 0;
    if (handle_type == SWITCH_OBJECT_TYPE_PORT) {
      status |= switch_store::v_get(
          switch_hostif_target_handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
      status |= switch_pkt_dev_port_to_ifindex_set(dev_port, intf_name.text);
    }
    return status;
  }

  switch_status_t del() {
    switch_object_id_t rx_filter_default_handle;
    switch_object_id_t tx_filter_default_handle;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t type = {};
    switch_object_id_t switch_hostif_handle;

    status |= switch_store::v_get(get_parent(),
                                  SWITCH_HOST_INTERFACE_ATTR_PARENT_HANDLE,
                                  switch_hostif_handle);

    status |= switch_store::v_get(
        switch_hostif_handle, SWITCH_HOSTIF_ATTR_TYPE, type);
    // rx filter genetlink comes from application
    if (type.enumdata == SWITCH_HOSTIF_ATTR_TYPE_GENETLINK) {
      return status;
    }

    status |= switch_store::v_get(get_auto_oid(),
                                  SWITCH_HOSTIF_FILTER_DEFAULT_ATTR_RX_HANDLE,
                                  rx_filter_default_handle.data);
    status |= switch_store::v_get(get_auto_oid(),
                                  SWITCH_HOSTIF_FILTER_DEFAULT_ATTR_TX_HANDLE,
                                  tx_filter_default_handle.data);
    /* delete default rx/tx filter object */
    status |= switch_store::object_delete(rx_filter_default_handle);
    status |= switch_store::object_delete(tx_filter_default_handle);
    auto_object::del();
    return status;
  }
};

class hostif_trap_stats : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_HOSTIF_TRAP_STATS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_HOSTIF_TRAP_STATS_ATTR_PARENT_HANDLE;
  uint16_t reason_code;

 public:
  hostif_trap_stats(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    switch_enum_t trap_type = {};
    status |=
        switch_store::v_get(parent, SWITCH_HOSTIF_TRAP_ATTR_TYPE, trap_type);
    reason_code = static_cast<uint16_t>(trap_type.enumdata);
  }

  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    (void)handle;
    uint64_t pkts = 0, bytes = 0;
    status =
        switch_pktdriver_reason_code_stats_get(0, reason_code, &pkts, &bytes);
    cntrs[SWITCH_HOSTIF_TRAP_COUNTER_ID_RX_PKTS].count = pkts;
    cntrs[SWITCH_HOSTIF_TRAP_COUNTER_ID_RX_BYTES].count = bytes;
    return status;
  }
  switch_status_t counters_set(const switch_object_id_t handle) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    (void)handle;
    status = switch_pktdriver_reason_code_stats_clear(0, reason_code);
    return status;
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    (void)handle;
    for (auto cntr_id : cntr_ids) {
      switch (cntr_id) {
        case SWITCH_HOSTIF_TRAP_COUNTER_ID_RX_PKTS:
          return switch_pktdriver_reason_code_pkts_clear(0, reason_code);
        case SWITCH_HOSTIF_TRAP_COUNTER_ID_RX_BYTES:
          return switch_pktdriver_reason_code_bytes_clear(0, reason_code);
        default:
          break;
      }
    }
    return SWITCH_STATUS_SUCCESS;
  }
};

class host_interface : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_HOST_INTERFACE;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_HOST_INTERFACE_ATTR_PARENT_HANDLE;
  switch_pkt_hostif_info_t hostif_info = {};
  uint64_t flags = 0;
  switch_enum_t type = {};

 public:
  host_interface(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    if (switch_store::v_get(parent,
                            SWITCH_HOSTIF_ATTR_NAME,
                            hostif_info.intf_name) == SWITCH_STATUS_SUCCESS) {
      flags |= SWITCH_PKT_HOSTIF_ATTR_INTERFACE_NAME;
    }

    if (switch_store::v_get(parent,
                            SWITCH_HOSTIF_ATTR_IFINDEX,
                            hostif_info.ifindex) == SWITCH_STATUS_SUCCESS) {
      flags |= SWITCH_PKT_HOSTIF_ATTR_IFINDEX;
    }

    if (switch_store::v_get(parent,
                            SWITCH_HOSTIF_ATTR_USE_IFINDEX,
                            hostif_info.use_ifindex) == SWITCH_STATUS_SUCCESS) {
      flags |= SWITCH_PKT_HOSTIF_ATTR_USE_IFINDEX;
    }

    if (switch_store::v_get(parent,
                            SWITCH_HOSTIF_ATTR_IP_ADDR,
                            hostif_info.v4addr) == SWITCH_STATUS_SUCCESS) {
      if (hostif_info.v4addr.addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4) {
        if (hostif_info.v4addr.addr.ip4) {
          flags |= SWITCH_PKT_HOSTIF_ATTR_IPV4_ADDRESS;
        }
      }
    }

    if (switch_store::v_get(parent, SWITCH_HOSTIF_ATTR_MAC, hostif_info.mac) ==
        SWITCH_STATUS_SUCCESS) {
      static switch_mac_addr_t hostif_zero_mac;

      if (memcmp(hostif_info.mac.mac, hostif_zero_mac.mac, ETH_LEN)) {
        flags |= SWITCH_PKT_HOSTIF_ATTR_MAC_ADDRESS;
      }
    }

    if (switch_store::v_get(parent,
                            SWITCH_HOSTIF_ATTR_OPER_STATUS,
                            hostif_info.operstatus) == SWITCH_STATUS_SUCCESS) {
      // don't enable yet. get's enabled after rx/tx filters are created
      if (!hostif_info.operstatus ||
          !switch_store::smiContext::context().in_warm_init()) {
        flags |= SWITCH_PKT_HOSTIF_ATTR_OPER_STATUS;
      }
    }

    status |= switch_store::v_get(parent, SWITCH_HOSTIF_ATTR_TYPE, type);
    status |= switch_store::v_get(parent,
                                  SWITCH_HOSTIF_ATTR_GENL_MCGRP_NAME,
                                  hostif_info.genetlink_mcgrp_name);
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    int fd = 0;
    uint64_t knet_hostif_handle = 0;

    if (get_auto_oid().data == 0 ||
        (switch_store::smiContext::context().in_warm_init())) {
      if (type.enumdata == SWITCH_HOSTIF_ATTR_TYPE_NETDEV) {
        /* Both @oper_status and @admin_state will be updated on
         * hostif_filter_default object create */
        hostif_info.admin_state = false;
        status = switch_pkt_hostif_create(
            0, &hostif_info, flags, &fd, &knet_hostif_handle);
        if (status != SWITCH_STATUS_SUCCESS) return status;
        SWITCH_CONTEXT.add_hostif_fd_map(get_parent(), fd, knet_hostif_handle);
      } else if (type.enumdata == SWITCH_HOSTIF_ATTR_TYPE_GENETLINK) {
        status = switch_pkt_genetlink_create(0, &hostif_info);
      }
    } else {
      if (type.enumdata == SWITCH_HOSTIF_ATTR_TYPE_NETDEV) {
        if (flags & SWITCH_PKT_HOSTIF_ATTR_OPER_STATUS) {
          switch_pkt_hostif_set_interface_oper_state(
              0, hostif_info.intf_name.text, hostif_info.operstatus);
        }

        if ((flags & SWITCH_PKT_HOSTIF_ATTR_IPV4_ADDRESS) ||
            (flags & SWITCH_PKT_HOSTIF_ATTR_MAC_ADDRESS)) {
          status |= switch_pkt_hostif_update(0, &hostif_info, flags);
        }
      }
    }

    auto_object::create_update();
    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    int fd = 0;
    uint64_t knet_hostif_handle = 0;

    if (type.enumdata == SWITCH_HOSTIF_ATTR_TYPE_NETDEV) {
      SWITCH_CONTEXT.find_hostif_fd_map(get_parent(), &fd, &knet_hostif_handle);
      status |= switch_pkt_hostif_delete(0, fd, knet_hostif_handle);
      if (status != SWITCH_STATUS_SUCCESS) return status;
      SWITCH_CONTEXT.clear_hostif_fd_map(get_parent());
    } else if (type.enumdata == SWITCH_HOSTIF_ATTR_TYPE_GENETLINK) {
      status = switch_pkt_genetlink_delete(0, &hostif_info);
    }

    auto_object::del();
    return status;
  }
};

class pktdriver_rx_filter : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_PKTDRIVER_RX_FILTER;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_PKTDRIVER_RX_FILTER_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t fid_attr_id =
      SWITCH_PKTDRIVER_RX_FILTER_ATTR_FILTER_ID;
  uint64_t flags = 0;
  switch_pktdriver_rx_filter_key_t rx_nf_key = {};
  switch_pktdriver_rx_filter_action_t rx_nf_action = {};
  switch_pktdriver_rx_filter_priority_t rx_nf_priority =
      SWITCH_PKTDRIVER_RX_FILTER_PRIORITY_MIN;

 public:
  pktdriver_rx_filter(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    switch_object_id_t handle = {0}, bd_handle = {0}, hostif_handle = {0},
                       hostif_trap_handle = {0};
    uint16_t dev_port = 0;
    switch_enum_t e = {}, rcode = {}, hostif_type = {}, hostif_trap_type = {},
                  channel_type = {};
    int fd = -1;
    uint64_t knet_hostif_handle = 0;
    switch_enum_t vlan_action = {};

    status = switch_store::v_get(
        parent, SWITCH_HOSTIF_RX_FILTER_ATTR_HANDLE, handle);

    status |= switch_store::v_get(parent, SWITCH_HOSTIF_RX_FILTER_ATTR_TYPE, e);
    switch (e.enumdata) {
      case SWITCH_HOSTIF_RX_FILTER_ATTR_TYPE_PORT:
        status |=
            switch_store::v_get(handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
        flags = SWITCH_PKTDRIVER_RX_FILTER_ATTR_DEV_PORT;
        rx_nf_priority = SWITCH_PKTDRIVER_RX_FILTER_PRIORITY_PORT;
        rx_nf_key.dev_port = dev_port;
        break;
      case SWITCH_HOSTIF_RX_FILTER_ATTR_TYPE_LAG:
        rx_nf_key.port_lag_index = compute_port_lag_index(handle);
        flags = SWITCH_PKTDRIVER_RX_FILTER_ATTR_PORT_LAG_INDEX;
        rx_nf_priority = SWITCH_PKTDRIVER_RX_FILTER_PRIORITY_INTERFACE;
        break;
      case SWITCH_HOSTIF_RX_FILTER_ATTR_TYPE_VLAN:
        status |= find_auto_oid(handle, SWITCH_OBJECT_TYPE_BD, bd_handle);
        rx_nf_key.bd = compute_bd(bd_handle);
        flags = SWITCH_PKTDRIVER_RX_FILTER_ATTR_BD;
        rx_nf_priority = SWITCH_PKTDRIVER_RX_FILTER_PRIORITY_VLAN;
        break;
      case SWITCH_HOSTIF_RX_FILTER_ATTR_TYPE_RIF:
        status |= find_auto_oid(handle, SWITCH_OBJECT_TYPE_BD, bd_handle);
        rx_nf_key.bd = compute_bd(bd_handle);
        flags = SWITCH_PKTDRIVER_RX_FILTER_ATTR_BD;
        rx_nf_priority = SWITCH_PKTDRIVER_RX_FILTER_PRIORITY_RIF;
        break;
      case SWITCH_HOSTIF_RX_FILTER_ATTR_TYPE_TRAP:
        flags = SWITCH_PKTDRIVER_RX_FILTER_ATTR_REASON_CODE;

        status |=
            switch_store::v_get(parent,
                                SWITCH_HOSTIF_RX_FILTER_ATTR_HOSTIF_TRAP_HANDLE,
                                hostif_trap_handle);
        rx_nf_action.hostif_trap_handle = hostif_trap_handle.data;
        if (switch_store::object_type_query(hostif_trap_handle) ==
            SWITCH_OBJECT_TYPE_HOSTIF_USER_DEFINED_TRAP) {
          rcode.enumdata = SWITCH_UDT_REASON_CODE;
          rcode.enumdata += switch_store::handle_to_id(hostif_trap_handle);
        } else {
          status |= switch_store::v_get(hostif_trap_handle,
                                        SWITCH_HOSTIF_TRAP_ATTR_TYPE,
                                        hostif_trap_type);
          rcode.enumdata = hostif_trap_type.enumdata;
          // dataplane sends (SWITCH_SFLOW_REASON_CODE + sflow_id) as reason
          // code
          // packet driver assumption is reason code for sample packet is 0x1000
          // This filter gets max priority so we can override all other per
          // interface filters
        }
        if (hostif_trap_type.enumdata ==
            SWITCH_HOSTIF_TRAP_ATTR_TYPE_SAMPLEPACKET) {
          rx_nf_key.reason_code = SWITCH_SFLOW_REASON_CODE;
          rx_nf_key.reason_code_mask = SWITCH_REASON_CODE_TYPE_MASK;
          rx_nf_priority = SWITCH_PKTDRIVER_RX_FILTER_PRIORITY_MAX;
        } else {
          rx_nf_key.reason_code = rcode.enumdata;
          rx_nf_key.reason_code_mask = 0xFFFF;
          rx_nf_priority = SWITCH_PKTDRIVER_RX_FILTER_PRIORITY_TRAP;
        }
        if (handle.data) {
          switch_object_type_t obj_type =
              switch_store::object_type_query(handle);
          switch (obj_type) {
            case SWITCH_OBJECT_TYPE_PORT:
              status |= switch_store::v_get(
                  handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
              rx_nf_key.dev_port = dev_port;
              break;
            case SWITCH_OBJECT_TYPE_VLAN:
            case SWITCH_OBJECT_TYPE_RIF:
              status |= find_auto_oid(handle, SWITCH_OBJECT_TYPE_BD, bd_handle);
              rx_nf_key.bd = compute_bd(bd_handle);
              break;
            case SWITCH_OBJECT_TYPE_LAG:
              rx_nf_key.port_lag_index = compute_port_lag_index(handle);
              break;
            default:
              switch_log(SWITCH_API_LEVEL_ERROR,
                         SWITCH_OBJECT_TYPE_HOSTIF_RX_FILTER,
                         "{}:{}: Invalid hostif_rx filter handle type {}",
                         __func__,
                         __LINE__,
                         obj_type);
              return;
          }
        }
        break;
      case SWITCH_HOSTIF_RX_FILTER_ATTR_TYPE_WILDCARD:
        flags = SWITCH_PKTDRIVER_RX_FILTER_ATTR_GLOBAL;
        return;
      default:
        status |= SWITCH_STATUS_INVALID_PARAMETER;
    }

    switch_store::v_get(
        parent, SWITCH_HOSTIF_RX_FILTER_ATTR_CHANNEL_TYPE, channel_type);
    switch (channel_type.enumdata) {
      case SWITCH_HOSTIF_RX_FILTER_ATTR_CHANNEL_TYPE_CB:
        rx_nf_action.channel_type = SWITCH_PKTDRIVER_CHANNEL_TYPE_CB;
        // higher priority than default per port hostif rx filters
        rx_nf_priority = SWITCH_PKTDRIVER_RX_FILTER_PRIORITY_CB;
        break;
      case SWITCH_HOSTIF_RX_FILTER_ATTR_CHANNEL_TYPE_PORT:
      case SWITCH_HOSTIF_RX_FILTER_ATTR_CHANNEL_TYPE_LAG:
        rx_nf_action.channel_type = SWITCH_PKTDRIVER_CHANNEL_TYPE_NETDEV;
        status |= switch_store::v_get(
            parent, SWITCH_HOSTIF_RX_FILTER_ATTR_HOSTIF, hostif_handle);
        SWITCH_CONTEXT.find_hostif_fd_map(
            hostif_handle, &fd, &knet_hostif_handle);
        rx_nf_action.knet_hostif_handle = knet_hostif_handle;
        rx_nf_action.fd = fd;
        break;
      case SWITCH_HOSTIF_RX_FILTER_ATTR_CHANNEL_TYPE_GENETLINK:
        rx_nf_action.channel_type = SWITCH_PKTDRIVER_CHANNEL_TYPE_GENL;
        status |= switch_store::v_get(
            parent, SWITCH_HOSTIF_RX_FILTER_ATTR_HOSTIF, hostif_handle);
        break;
      case SWITCH_HOSTIF_RX_FILTER_ATTR_CHANNEL_TYPE_FD:
        rx_nf_action.channel_type = SWITCH_PKTDRIVER_CHANNEL_TYPE_FD;
        status |= switch_store::v_get(
            parent, SWITCH_HOSTIF_RX_FILTER_ATTR_HOSTIF, hostif_handle);
        SWITCH_CONTEXT.find_hostif_fd_map(
            hostif_handle, &fd, &knet_hostif_handle);
        rx_nf_action.knet_hostif_handle = knet_hostif_handle;
        rx_nf_action.fd = fd;
      default:
        break;
    }

    // hostif invalid for CB, check for validity
    if (hostif_handle.data) {
      status |= switch_store::v_get(
          hostif_handle, SWITCH_HOSTIF_ATTR_TYPE, hostif_type);
      if (hostif_type.enumdata == SWITCH_HOSTIF_ATTR_TYPE_NETDEV) {
        status |= switch_store::v_get(
            hostif_handle, SWITCH_HOSTIF_ATTR_VLAN_ACTION, vlan_action);
        switch (vlan_action.enumdata) {
          case SWITCH_HOSTIF_ATTR_VLAN_ACTION_STRIP:
            rx_nf_action.vlan_action = SWITCH_PACKET_VLAN_ACTION_REMOVE;
            break;
          case SWITCH_HOSTIF_ATTR_VLAN_ACTION_KEEP:
            rx_nf_action.vlan_action = SWITCH_PACKET_VLAN_ACTION_ADD;
            break;
          default:
            rx_nf_action.vlan_action = SWITCH_PACKET_VLAN_ACTION_NONE;
            break;
        }
      }
    }
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t filter_id = 0;
    if (flags != SWITCH_PKTDRIVER_RX_FILTER_ATTR_GLOBAL) {
      switch_object_id_t auto_oid = get_auto_oid();
      if (auto_oid.data &&
          !switch_store::smiContext::context().in_warm_init()) {
        switch_store::v_get(get_auto_oid(), fid_attr_id, filter_id);
        if (filter_id) {
          status |= switch_pktdriver_rx_filter_delete(0, &rx_nf_key, filter_id);
        }
      }

      status |= switch_pktdriver_rx_filter_create(
          0, rx_nf_priority, flags, &rx_nf_key, &rx_nf_action, &filter_id);
      attrs.insert(attr_w(fid_attr_id, filter_id));
    }
    auto_object::create_update();
    return status;
  }

  switch_status_t del() {
    uint64_t filter_id;
    auto status = switch_store::v_get(get_auto_oid(), fid_attr_id, filter_id);
    if (filter_id) {
      status |= switch_pktdriver_rx_filter_delete(0, &rx_nf_key, filter_id);
    }
    auto_object::del();
    return status;
  }
  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    (void)handle;
    uint64_t cntr = 0;
    uint64_t filter_id = 0;
    status = switch_store::v_get(get_auto_oid(), fid_attr_id, filter_id);
    if (filter_id) {
      status |= switch_pktdriver_rx_filter_num_packets_get(0, filter_id, &cntr);
    }
    cntrs[SWITCH_HOSTIF_COUNTER_ID_RX_PKT].count += cntr;
    return status;
  }
  switch_status_t counters_set(const switch_object_id_t handle) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    (void)handle;
    uint64_t filter_id = 0;
    status = switch_store::v_get(get_auto_oid(), fid_attr_id, filter_id);
    if (filter_id)
      status |= switch_pktdriver_rx_filter_num_packets_clear(0, filter_id);
    return status;
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    for (auto cntr_id : cntr_ids) {
      switch (cntr_id) {
        case SWITCH_HOSTIF_COUNTER_ID_RX_PKT:
          return counters_set(handle);
        default:
          break;
      }
    }
    return SWITCH_STATUS_SUCCESS;
  }
};

class pktdriver_tx_filter : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_PKTDRIVER_TX_FILTER;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_PKTDRIVER_TX_FILTER_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t fid_attr_id =
      SWITCH_PKTDRIVER_TX_FILTER_ATTR_FILTER_ID;
  uint64_t flags = 0;
  switch_pktdriver_tx_filter_key_t tx_nf_key = {};
  switch_pktdriver_tx_filter_action_t tx_nf_action = {};
  switch_pktdriver_tx_filter_priority_t tx_nf_priority =
      SWITCH_PKTDRIVER_TX_FILTER_PRIORITY_MIN;

 public:
  pktdriver_tx_filter(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    switch_object_id_t handle = {0}, bd_handle = {0}, hostif_handle = {0},
                       port_handle = {0}, device_handle = {0},
                       cpu_port_handle = {0};
    uint16_t dev_port = 0;
    switch_enum_t e = {};
    switch_enum_t rif_type = {};
    int fd = 0;
    uint64_t knet_hostif_handle = 0;

    status = switch_store::v_get(
        parent, SWITCH_HOSTIF_TX_FILTER_ATTR_DEVICE, device_handle);
    status |= switch_store::v_get(
        device_handle, SWITCH_DEVICE_ATTR_CPU_PORT, cpu_port_handle);
    status = switch_store::v_get(
        parent, SWITCH_HOSTIF_TX_FILTER_ATTR_HANDLE, handle);
    status |= switch_store::v_get(parent, SWITCH_HOSTIF_TX_FILTER_ATTR_TYPE, e);
    switch (e.enumdata) {
      case SWITCH_HOSTIF_TX_FILTER_ATTR_TYPE_PORT: {
        status |=
            switch_store::v_get(handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
        tx_nf_action.dev_port = dev_port;
        tx_nf_action.port_lag_index = compute_port_lag_index(handle);
        if (handle == cpu_port_handle) {
          tx_nf_action.bypass_flags = SWITCH_BYPASS_ROUTING_CHECK;
        } else {
          tx_nf_action.bypass_flags = SWITCH_BYPASS_ALL;
        }
      } break;
      case SWITCH_HOSTIF_TX_FILTER_ATTR_TYPE_LAG:
        tx_nf_action.port_lag_index = compute_port_lag_index(handle);
        tx_nf_action.bypass_flags = SWITCH_BYPASS_ALL;
        break;
      case SWITCH_HOSTIF_TX_FILTER_ATTR_TYPE_VLAN:
        status |= find_auto_oid(handle, SWITCH_OBJECT_TYPE_BD, bd_handle);
        tx_nf_action.bd = switch_store::handle_to_id(bd_handle);
        flags |= SWITCH_PKTDRIVER_TX_FILTER_ATTR_VLAN_ID;
        tx_nf_action.bypass_flags =
            SWITCH_BYPASS_NONE | SWITCH_BYPASS_SYSTEM_ACL;
        break;
      case SWITCH_HOSTIF_TX_FILTER_ATTR_TYPE_RIF:
        flags |= SWITCH_PKTDRIVER_TX_FILTER_ATTR_VLAN_ID;
        status |= switch_store::v_get(handle, SWITCH_RIF_ATTR_TYPE, rif_type);
        if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_PORT ||
            rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_SUB_PORT) {
          status |= find_auto_oid(handle, SWITCH_OBJECT_TYPE_BD, bd_handle);
          tx_nf_action.bd = switch_store::handle_to_id(bd_handle);
          status |= switch_store::v_get(
              handle, SWITCH_RIF_ATTR_PORT_HANDLE, port_handle);
          tx_nf_action.port_lag_index = compute_port_lag_index(port_handle);
          tx_nf_action.dev_port = 0;
          tx_nf_action.bypass_flags = SWITCH_BYPASS_ALL;
        } else if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_VLAN) {
          switch_object_id_t vlan_handle = {};
          status |= switch_store::v_get(
              handle, SWITCH_RIF_ATTR_VLAN_HANDLE, vlan_handle);
          status |=
              find_auto_oid(vlan_handle, SWITCH_OBJECT_TYPE_BD, bd_handle);

          tx_nf_action.bypass_flags =
              SWITCH_BYPASS_NONE | SWITCH_BYPASS_SYSTEM_ACL;
        } else {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_HOSTIF_TX_FILTER,
                     "{}:{}: Invalid rif_type, type {}",
                     __func__,
                     __LINE__,
                     rif_type.enumdata);

          status |= SWITCH_STATUS_INVALID_PARAMETER;
          return;
        }
        tx_nf_action.bd = compute_bd(bd_handle);

        break;
      default:
        status |= SWITCH_STATUS_INVALID_PARAMETER;
    }

    status |= switch_store::v_get(
        parent, SWITCH_HOSTIF_TX_FILTER_ATTR_HOSTIF, hostif_handle);
    SWITCH_CONTEXT.find_hostif_fd_map(hostif_handle, &fd, &knet_hostif_handle);
    flags |= SWITCH_PKTDRIVER_TX_FILTER_ATTR_HOSTIF_FD;
    tx_nf_key.hostif_fd = fd;
    tx_nf_key.knet_hostif_handle = knet_hostif_handle;
    tx_nf_priority = SWITCH_PKTDRIVER_TX_FILTER_PRIORITY_HOSTIF;
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t filter_id = 0;
    status = switch_pktdriver_tx_filter_create(
        0, tx_nf_priority, flags, &tx_nf_key, &tx_nf_action, &filter_id);
    attrs.insert(attr_w(fid_attr_id, filter_id));
    auto_object::create_update();
    return status;
  }

  switch_status_t del() {
    uint64_t filter_id;
    auto status = switch_store::v_get(get_auto_oid(), fid_attr_id, filter_id);
    if (filter_id) {
      status |= switch_pktdriver_tx_filter_delete(0, &tx_nf_key, filter_id);
    }
    auto_object::del();
    return status;
  }
  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    (void)handle;
    uint64_t cntr = 0;
    uint64_t filter_id = 0;
    status = switch_store::v_get(get_auto_oid(), fid_attr_id, filter_id);
    if (filter_id) {
      status |= switch_pktdriver_tx_filter_num_packets_get(0, filter_id, &cntr);
    }
    cntrs[SWITCH_HOSTIF_COUNTER_ID_TX_PKT].count += cntr;
    return status;
  }
  switch_status_t counters_set(const switch_object_id_t handle) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    (void)handle;
    uint64_t filter_id = 0;
    status = switch_store::v_get(get_auto_oid(), fid_attr_id, filter_id);
    if (filter_id)
      status |= switch_pktdriver_tx_filter_num_packets_clear(0, filter_id);
    return status;
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    for (auto cntr_id : cntr_ids) {
      switch (cntr_id) {
        case SWITCH_HOSTIF_COUNTER_ID_TX_PKT:
          return counters_set(handle);
        default:
          break;
      }
    }
    return SWITCH_STATUS_SUCCESS;
  }
};

class hostif_stats : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_HOSTIF_STATS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_HOSTIF_STATS_ATTR_PARENT_HANDLE;
  std::set<switch_object_id_t> rx_filter_handles, tx_filter_handles;

 public:
  hostif_stats(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status |= switch_store::referencing_set_get(
        parent, SWITCH_OBJECT_TYPE_HOSTIF_RX_FILTER, rx_filter_handles);
    status |= switch_store::referencing_set_get(
        parent, SWITCH_OBJECT_TYPE_HOSTIF_TX_FILTER, tx_filter_handles);
  }

  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    (void)handle;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    for (const auto rx_filter : rx_filter_handles) {
      pktdriver_rx_filter pkt_rx(rx_filter, status);
      status |= pkt_rx.counters_get(rx_filter, cntrs);
    }
    for (const auto tx_filter : tx_filter_handles) {
      pktdriver_tx_filter pkt_tx(tx_filter, status);
      status |= pkt_tx.counters_get(tx_filter, cntrs);
    }
    return status;
  }

  switch_status_t counters_set(const switch_object_id_t handle) {
    (void)handle;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    for (const auto rx_filter : rx_filter_handles) {
      pktdriver_rx_filter pkt_rx(rx_filter, status);
      status |= pkt_rx.counters_set(rx_filter);
    }
    for (const auto tx_filter : tx_filter_handles) {
      pktdriver_tx_filter pkt_tx(tx_filter, status);
      status |= pkt_tx.counters_set(tx_filter);
    }
    return status;
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    (void)handle;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    for (const auto rx_filter : rx_filter_handles) {
      pktdriver_rx_filter pkt_rx(rx_filter, status);
      status |= pkt_rx.counters_set(rx_filter, cntr_ids);
    }
    for (const auto tx_filter : tx_filter_handles) {
      pktdriver_tx_filter pkt_tx(tx_filter, status);
      status |= pkt_tx.counters_set(tx_filter, cntr_ids);
    }
    return status;
  }
};

// updates pktdriver cache for fast lookup
class hostif_port_metadata : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_HOSTIF_PORT_METADATA;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_HOSTIF_PORT_METADATA_ATTR_PARENT_HANDLE;
  uint16_t dev_port = 0;

 public:
  hostif_port_metadata(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
  }
  switch_status_t create_update() {
    switch_pkt_dev_port_to_port_handle_set(dev_port, get_parent().data);
    return auto_object::create_update();
  }
  switch_status_t del() {
    switch_pkt_dev_port_to_port_handle_set(dev_port, 0);
    return auto_object::del();
  }
};

#ifdef __cplusplus
extern "C" {
#endif

void hostif_switch_packet_cb(char *pkt,
                             int pkt_size,
                             uint64_t port_lag_handle,
                             uint64_t hostif_trap_handle) {
  switch_packet_event_data_t pkt_event;
  pkt_event.pkt = pkt;
  pkt_event.pkt_size = pkt_size;
  pkt_event.port_handle.data = port_lag_handle;
  pkt_event.hostif_trap_handle.data = hostif_trap_handle;
  smi::event::packet_event_notify(pkt_event);
}

#ifdef __cplusplus
}
#endif

switch_status_t hostif_init() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  REGISTER_OBJECT(host_interface, SWITCH_OBJECT_TYPE_HOST_INTERFACE);
  REGISTER_OBJECT(hostif_stats, SWITCH_OBJECT_TYPE_HOSTIF_STATS);
  REGISTER_OBJECT(hostif_port_metadata,
                  SWITCH_OBJECT_TYPE_HOSTIF_PORT_METADATA);
  REGISTER_OBJECT(pktdriver_rx_filter, SWITCH_OBJECT_TYPE_PKTDRIVER_RX_FILTER);
  REGISTER_OBJECT(pktdriver_tx_filter, SWITCH_OBJECT_TYPE_PKTDRIVER_TX_FILTER);
  REGISTER_OBJECT(hostif_trap_stats, SWITCH_OBJECT_TYPE_HOSTIF_TRAP_STATS);
  REGISTER_OBJECT(hostif_filter_default,
                  SWITCH_OBJECT_TYPE_HOSTIF_FILTER_DEFAULT);

  return status;
}

switch_status_t hostif_enable_cb(switch_object_id_t hostif_handle,
                                 switch_attribute_t attr) {
  (void)attr;
  switch_object_id_t host_iface = {};
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  find_auto_oid(hostif_handle, SWITCH_OBJECT_TYPE_HOST_INTERFACE, host_iface);
  if (host_iface.data != 0) {
    hostif_filter_default hif_def(host_iface, status);
    status = hif_def.create_update();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_HOSTIF,
                 "{}: hostif oper state failed status={} for hostif {}",
                 __func__,
                 status,
                 hostif_handle);
    }
  }
  return status;
}

switch_status_t hostif_start_packet_driver() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_attribute_t attr = {};

  if (SWITCH_CONTEXT.is_pkt_drv_init_done()) return status;

  // enable packet driver poll thread
  start_bf_switch_api_packet_driver();

  // set operstate to UP for all hostifs
  status |=
      execute_cb_for_all(SWITCH_OBJECT_TYPE_HOSTIF, hostif_enable_cb, attr);

  SWITCH_CONTEXT.set_pkt_drv_init(true);
  switch_register_callback_rx(hostif_switch_packet_cb);
  return status;
}

switch_status_t hostif_stop_packet_driver() {
  if (SWITCH_CONTEXT.is_pkt_drv_init_done()) {
    SWITCH_CONTEXT.set_pkt_drv_init(false);
    stop_bf_switch_api_packet_driver();
  }
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t hostif_clean() {
  int fd;
  uint64_t knet_hostif_handle;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  // clenaup fd's otherwise while creating back will cause issues on model and
  // ioctl call would fail.
  for (const auto &entry : SWITCH_CONTEXT.get_hostif_fd_map()) {
    fd = entry.second.fd;
    knet_hostif_handle = entry.second.knet_hostif_handle;
    status = switch_pkt_hostif_delete(0, fd, knet_hostif_handle);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_HOSTIF,
                 "Hostif cleanup failed, fd : {},knet_hostif_handle: {}",
                 fd,
                 knet_hostif_handle);
    }
  }
  SWITCH_CONTEXT.clean_hostif_fd_map();
  return status;
}

} /* namespace smi */
