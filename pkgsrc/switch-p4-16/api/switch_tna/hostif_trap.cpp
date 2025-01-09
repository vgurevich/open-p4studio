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


extern "C" {
#include <tofino/pdfixed/pd_common.h>
#include <lld/bf_lld_if.h>
}
#include <netinet/in.h>
#include <arpa/inet.h>

#include <vector>
#include <set>
#include <unordered_map>
#include <utility>

#include "s3/switch_packet.h"
#include "switch_tna/utils.h"
#include "common/hostif.h"
#include "switch_tna/acl.h"
#include "switch_tna/hostif_trap.h"

namespace smi {
using ::smi::logging::switch_log;

switch_status_t ExclusionListManager::port_exclusion_list_id_reserve(
    const std::set<switch_object_id_t> port_handles,
    const uint8_t exclusion_id) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  if (exclusion_id > SWITCH_ACL_EXCLUDE_PORT_LIST_LABEL_WIDTH) {
    status = SWITCH_STATUS_INSUFFICIENT_RESOURCES;
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "Failed to reseve Port Exclusion ID {}, status: {}",
               exclusion_id,
               status);
    return status;
  }
  if (_trap_exclusion_id_map.find(port_handles) ==
      _trap_exclusion_id_map.end()) {
    status = trap_exclusion_port_list_ids->reserve(exclusion_id);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_NONE,
                 "Failed to reseve Port Exclusion ID {}, status: {}",
                 exclusion_id,
                 status);
      return status;
    }
    _trap_exclusion_id_map[port_handles] = exclusion_id;
    _trap_exclusion_id_refcnt[exclusion_id] = 0;
  } else {
    auto curr_id = _trap_exclusion_id_map[port_handles];
    if (curr_id != exclusion_id) {
      status = SWITCH_STATUS_ITEM_ALREADY_EXISTS;
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_NONE,
                 "Failed to reseve Port Exclusion ID {}, for port exclusion "
                 "list {}, already mapped to exclusion ID:{} status: {}",
                 exclusion_id,
                 port_handles,
                 curr_id,
                 status);
      return status;
    }
  }
  _trap_exclusion_id_refcnt[exclusion_id]++;
  return status;
}

switch_status_t ExclusionListManager::port_exclusion_list_id_allocate(
    const std::set<switch_object_id_t> port_handles, uint8_t &exclusion_id) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  if (_trap_exclusion_id_map.find(port_handles) ==
      _trap_exclusion_id_map.end()) {
    exclusion_id =
        static_cast<uint8_t>(trap_exclusion_port_list_ids->allocate());
    if (exclusion_id > SWITCH_ACL_EXCLUDE_PORT_LIST_LABEL_WIDTH) {
      status = SWITCH_STATUS_INSUFFICIENT_RESOURCES;
      trap_exclusion_port_list_ids->release(exclusion_id);
      switch_log(
          SWITCH_API_LEVEL_ERROR,
          SWITCH_OBJECT_TYPE_NONE,
          "Failed to allocate unique id for Port Exclusion List {}, status: {}",
          port_handles,
          status);
      return status;
    }
    _trap_exclusion_id_map[port_handles] = exclusion_id;
    _trap_exclusion_id_refcnt[exclusion_id] = 0;
  } else {
    exclusion_id = _trap_exclusion_id_map[port_handles];
  }
  _trap_exclusion_id_refcnt[exclusion_id]++;
  return status;
}

switch_status_t ExclusionListManager::port_exclusion_list_id_release(
    const std::set<switch_object_id_t> &port_handles) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  if (_trap_exclusion_id_map.find(port_handles) ==
      _trap_exclusion_id_map.end()) {
    status = SWITCH_STATUS_ITEM_NOT_FOUND;
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "Port Exclusion List {}, Exclusion ID release failed, status:{}",
               port_handles,
               status);
    return status;
  }
  auto exclusion_id = _trap_exclusion_id_map[port_handles];
  _trap_exclusion_id_refcnt[exclusion_id]--;
  if (_trap_exclusion_id_refcnt[exclusion_id] == 0) {
    trap_exclusion_port_list_ids->release(exclusion_id);
    _trap_exclusion_id_map.erase(port_handles);
  }
  return status;
}

switch_status_t ExclusionListManager::port_exclusion_list_id_release(
    const uint8_t exclusion_id) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  if (exclusion_id > SWITCH_ACL_EXCLUDE_PORT_LIST_LABEL_WIDTH) {
    status = SWITCH_STATUS_INVALID_PARAMETER;
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "Failed to release Port Exclusion ID {}, status:{}",
               exclusion_id,
               status);
    return status;
  }
  _trap_exclusion_id_refcnt[exclusion_id]--;
  if (_trap_exclusion_id_refcnt[exclusion_id] == 0) {
    trap_exclusion_port_list_ids->release(exclusion_id);
    auto iter = _trap_exclusion_id_map.begin();
    while (iter != _trap_exclusion_id_map.end()) {
      if (iter->second == exclusion_id) {
        _trap_exclusion_id_map.erase(iter);
        break;
      }
      iter++;
    }
  }
  return status;
}

switch_status_t ExclusionListManager::port_exclusion_list_refcount(
    const uint8_t exclusion_id, uint32_t &ref_count) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  if (exclusion_id > SWITCH_ACL_EXCLUDE_PORT_LIST_LABEL_WIDTH) {
    status = SWITCH_STATUS_INVALID_PARAMETER;
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "Failed to get refcount for Port Exclusion ID {}, status:{}",
               exclusion_id,
               status);
    return status;
  }
  ref_count = _trap_exclusion_id_refcnt[exclusion_id];
  return status;
}

switch_status_t hostif_trap_packet_action_to_acl_action(
    switch_hostif_trap_attr_packet_action trap_pkt_action,
    switch_enum_t &packet_action) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  switch (trap_pkt_action) {
    /* As we don't support permit action in system_acl (there is no too much
     * need for it now) the forward action in SAI is mapped to NOP in SMI in
     * order to avoid an error during hostif trap entry with forward action
     * removal
     */
    case SWITCH_HOSTIF_TRAP_ATTR_PACKET_ACTION_NOP:
    case SWITCH_HOSTIF_TRAP_ATTR_PACKET_ACTION_FORWARD:
      packet_action.enumdata = SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_PERMIT;
      break;
    case SWITCH_HOSTIF_TRAP_ATTR_PACKET_ACTION_DROP:
      packet_action.enumdata = SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_DROP;
      break;
    case SWITCH_HOSTIF_TRAP_ATTR_PACKET_ACTION_COPY_TO_CPU:
      packet_action.enumdata = SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_COPY_TO_CPU;
      break;
    case SWITCH_HOSTIF_TRAP_ATTR_PACKET_ACTION_REDIRECT_TO_CPU:
      packet_action.enumdata =
          SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_REDIRECT_TO_CPU;
      break;
    default:
      status = SWITCH_STATUS_INVALID_PARAMETER;
      break;
  }

  return status;
}

static void get_qid(const switch_object_id_t trap_group_handle,
                    switch_object_id_t &meter_handle,
                    uint8_t &qid) {
  switch_object_id_t cpu_queue_handle = {};

  if (trap_group_handle.data) {
    switch_store::v_get(trap_group_handle,
                        SWITCH_HOSTIF_TRAP_GROUP_ATTR_QUEUE_HANDLE,
                        cpu_queue_handle);
    switch_store::v_get(trap_group_handle,
                        SWITCH_HOSTIF_TRAP_GROUP_ATTR_POLICER_HANDLE,
                        meter_handle);
  }
  if (cpu_queue_handle.data)
    switch_store::v_get(cpu_queue_handle, SWITCH_QUEUE_ATTR_QUEUE_ID, qid);
}

#define SYSTEM_ACL_ENTRY                       \
  std::pair<_MatchKey, _ActionEntry>(          \
      _MatchKey(smi_id::T_INGRESS_SYSTEM_ACL), \
      _ActionEntry(smi_id::T_INGRESS_SYSTEM_ACL))
class hostif_trap_acl : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_HOSTIF_TRAP_ACL;
  static const switch_attr_id_t status_attr_id =
      SWITCH_HOSTIF_TRAP_ACL_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_HOSTIF_TRAP_ACL_ATTR_PARENT_HANDLE;
  switch_enum_t packet_action;
  switch_hostif_trap_attr_type trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_NONE;
  uint16_t cpu_redirect_reason_code;
  uint32_t acl_priority{}, exclusion_port_list_id{},
      exclusion_port_list_index{};
  switch_object_id_t trap_group_handle;
  switch_object_id_t default_trap_group_handle;
  switch_object_id_t device_handle;
  switch_object_id_t parent_handle;
  std::vector<std::pair<_MatchKey, _ActionEntry>>::iterator it;

 public:
  hostif_trap_acl(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_INGRESS_SYSTEM_ACL,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    it = match_action_list.begin();
    switch_hostif_trap_attr_packet_action trap_pkt_action;
    uint32_t trap_priority = 0;
    switch_enum_t enum_trap_type, enum_pkt_action_type;

    parent_handle = parent;
    status |= switch_store::v_get(
        parent, SWITCH_HOSTIF_TRAP_ATTR_TYPE, enum_trap_type);
    status |= switch_store::v_get(
        parent, SWITCH_HOSTIF_TRAP_ATTR_DEVICE, device_handle);
    trap_type =
        static_cast<switch_hostif_trap_attr_type>(enum_trap_type.enumdata);
    status |= switch_store::v_get(
        parent, SWITCH_HOSTIF_TRAP_ATTR_PACKET_ACTION, enum_pkt_action_type);
    trap_pkt_action = static_cast<switch_hostif_trap_attr_packet_action>(
        enum_pkt_action_type.enumdata);
    status |=
        hostif_trap_packet_action_to_acl_action(trap_pkt_action, packet_action);
    status |= switch_store::v_get(
        parent, SWITCH_HOSTIF_TRAP_ATTR_PRIORITY, trap_priority);
    status |=
        switch_store::v_get(parent,
                            SWITCH_HOSTIF_TRAP_ATTR_HOSTIF_TRAP_GROUP_HANDLE,
                            trap_group_handle);
    status |= switch_store::v_get(device_handle,
                                  SWITCH_DEVICE_ATTR_DEFAULT_HOSTIF_TRAP_GROUP,
                                  default_trap_group_handle);
    status |= switch_store::v_get(parent,
                                  SWITCH_HOSTIF_TRAP_ATTR_EXCLUDE_PORT_LIST_ID,
                                  exclusion_port_list_id);
    exclusion_port_list_index =
        SWITCH_HOSTIF_TRAP_EXCLUSION_PORT_LIST_VALUE(exclusion_port_list_id);

    if (!trap_priority) {
      acl_priority = system_acl_priority(SYSTEM_ACL_TYPE_HOSTIF_TRAP);
    } else {
      acl_priority = trap_priority;
    }

    cpu_redirect_reason_code = trap_type;

    switch (trap_type) {
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_ARP_REQUEST:
        status |= create_hostif_trap_type_arp_request();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_ARP_RESPONSE:
        status |= create_hostif_trap_type_arp_response();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_SAMPLEPACKET:
        status |= create_hostif_trap_type_sflow();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_STP:
        status |= create_hostif_trap_type_stp();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_PVRST:
        status |= create_hostif_trap_type_pvrst();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_LACP:
        status |= create_hostif_trap_type_lacp();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_EAPOL:
        status |= create_hostif_trap_type_eapol();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_LLDP:
        status |= create_hostif_trap_type_lldp();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_OSPF:
        status |= create_hostif_trap_type_ospf();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_OSPFV6:
        status |= create_hostif_trap_type_ospfv6();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_BGP:
        status |= create_hostif_trap_type_bgp(false);
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_ISIS:
        status |= create_hostif_trap_type_isis();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_BGPV6:
        status |= create_hostif_trap_type_bgp(true);
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_PIM:
        status |= create_hostif_trap_type_pim();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_IGMP_TYPE_QUERY:
        status |= create_hostif_trap_type_igmp(SWITCH_HOSTIF_IGMP_TYPE_QUERY);
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_IGMP_TYPE_LEAVE:
        status |= create_hostif_trap_type_igmp(SWITCH_HOSTIF_IGMP_TYPE_LEAVE);
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_IGMP_TYPE_V1_REPORT:
        status |=
            create_hostif_trap_type_igmp(SWITCH_HOSTIF_IGMP_TYPE_V1_REPORT);
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_IGMP_TYPE_V2_REPORT:
        status |=
            create_hostif_trap_type_igmp(SWITCH_HOSTIF_IGMP_TYPE_V2_REPORT);
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_IGMP_TYPE_V3_REPORT:
        status |=
            create_hostif_trap_type_igmp(SWITCH_HOSTIF_IGMP_TYPE_V3_REPORT);
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_IPV6_MLD_V1_V2:
        status |= create_hostif_trap_type_ipv6_mld(
            SWITCH_HOSTIF_IPV6_MLD_V1_V2_QUERY);
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_IPV6_MLD_V1_REPORT:
        status |=
            create_hostif_trap_type_ipv6_mld(SWITCH_HOSTIF_IPV6_MLD_V1_REPORT);
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_IPV6_MLD_V1_DONE:
        status |=
            create_hostif_trap_type_ipv6_mld(SWITCH_HOSTIF_IPV6_MLD_V1_DONE);
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_MLD_V2_REPORT:
        status |= create_hostif_trap_type_ipv6_mld(SWITCH_HOSTIF_MLD_V2_REPORT);
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_SSH:
        status |= create_hostif_trap_type_ssh();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_DHCP:
        status |= create_hostif_trap_type_dhcp();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_DHCPV6:
        status |= create_hostif_trap_type_dhcpv6();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_DHCP_L2:
        status |= create_hostif_trap_type_dhcp_l2();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_DHCPV6_L2:
        status |= create_hostif_trap_type_dhcpv6_l2();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_SNMP:
        status |= create_hostif_trap_type_snmp();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_ICMP:
        status |= create_hostif_trap_type_icmp();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_ARP_SUPPRESS:
        cpu_redirect_reason_code = SWITCH_HOSTIF_TRAP_ATTR_TYPE_ARP_REQUEST;
        status |= create_hostif_trap_type_arp_suppress();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_ND_SUPPRESS:
        cpu_redirect_reason_code =
            SWITCH_HOSTIF_TRAP_ATTR_TYPE_IPV6_NEIGHBOR_DISCOVERY;
        status |= create_hostif_trap_type_ipv6_nd_suppress();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_ICMPV6:
        status |= create_hostif_trap_type_icmpv6();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_IPV6_NEIGHBOR_DISCOVERY:
        status |= create_hostif_trap_type_ipv6_nd();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_PTP:
        status |= create_hostif_trap_type_ptp();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_BFD:
        status |= create_hostif_trap_type_bfd();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_BFDV6:
        status |= create_hostif_trap_type_bfdv6();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_VRRP:
        status |= create_hostif_trap_type_vrrp();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_VRRPV6:
        status |= create_hostif_trap_type_vrrpv6();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_L3_MTU_ERROR:
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_TTL_ERROR:
        status |= create_hostif_trap_type_ttl_error();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_MYIP:
        status |= create_hostif_trap_type_my_ip();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_MYIP_SUBNET:
        status |= create_hostif_trap_type_my_ip_subnet();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_UDLD:
        status |= create_hostif_trap_type_udld();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_DNAT_MISS:
        status |= create_hostif_trap_type_dnat_miss();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_SNAT_MISS:
        status |= create_hostif_trap_type_snat_miss();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_MPLS_ROUTER_ALERT:
        status |= create_hostif_trap_type_mpls_router_alert();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_MPLS_TTL_ERROR:
        status |= create_hostif_trap_type_mpls_ttl_error();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_ICCP:
        status |= create_hostif_trap_type_iccp();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_LDP:
        status |= create_hostif_trap_type_ldp();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_GNMI:
        status |= create_hostif_trap_type_gnmi();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_P4RT:
        status |= create_hostif_trap_type_p4rt();
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_NTPCLIENT:
        status |= create_hostif_trap_type_ntpc(false);
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_NTPSERVER:
        status |= create_hostif_trap_type_ntpc(true);
        break;
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_NONE:
        switch_log(SWITCH_API_LEVEL_DEBUG,
                   SWITCH_OBJECT_TYPE_HOSTIF_TRAP,
                   "ignore hostIf Trap notification:");
        break;
      default:
        status |= SWITCH_STATUS_INVALID_PARAMETER;
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_HOSTIF_TRAP,
                   "Hostif trap type: {} not supported",
                   trap_type);
    }

    auto entry_it = match_action_list.begin();
    while (entry_it != match_action_list.end()) {
      status |= entry_it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL,
          static_cast<uint32_t>(0),
          exclusion_port_list_index);
      entry_it++;
    }
  }

  switch_status_t add_stp_forwarding_flag(
      std::vector<std::pair<_MatchKey, _ActionEntry>>::iterator iter) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (feature::is_feature_set(SWITCH_FEATURE_STP)) {
      const uint8_t ingress_stp_state_mask = 0x3;
      if (smi_id::F_SYSTEM_ACL_LOCAL_MD_STP_STATE) {
        status |= iter->first.set_ternary(
            smi_id::F_SYSTEM_ACL_LOCAL_MD_STP_STATE,
            static_cast<uint8_t>(SWITCH_STP_PORT_ATTR_STATE_FORWARDING),
            ingress_stp_state_mask);
      } else {
        status |= iter->first.set_ternary(
            smi_id::F_SYSTEM_ACL_LOCAL_MD_DROP_REASON,
            static_cast<uint8_t>(SWITCH_STP_PORT_ATTR_STATE_FORWARDING),
            static_cast<uint8_t>(0xFF));
      }
    }
    return status;
  }
  switch_status_t setup_action_data(
      std::vector<std::pair<_MatchKey, _ActionEntry>>::iterator iter) {
    return setup_action_data_packet_action(
        iter, packet_action, acl_priority, trap_group_handle);
  }

  switch_status_t setup_action_data_packet_action(
      std::vector<std::pair<_MatchKey, _ActionEntry>>::iterator iter,
      switch_enum_t pkt_action,
      uint32_t prio,
      switch_object_id_t trap_grp) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_object_id_t meter_handle = {};
    uint8_t qid = 0;
    switch_enum_t ing_target_type = {
        .enumdata = SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_COPP};

    status |= iter->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, prio);
    status |=
        iter->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_ACL_DENY,
                                static_cast<uint8_t>(0),
                                static_cast<uint8_t>(1));
    if (feature::is_feature_set(
            SWITCH_FEATURE_INGRESS_MAC_IP_ACL_TRANSIT_ACTION)) {
      status |= iter->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_ACL_COPY_CANCEL,
          static_cast<uint8_t>(0),
          static_cast<uint8_t>(1));
    }

    get_qid(trap_grp, meter_handle, qid);

    switch (pkt_action.enumdata) {
      case SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_PERMIT:
        iter->second.init_action_data(smi_id::A_SYSTEM_ACL_PERMIT);
        break;
      case SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_DROP:
        iter->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
        status |= iter->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                                       cpu_redirect_reason_code);
        break;
      case SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_REDIRECT_SFLOW_TO_CPU:
        iter->second.init_action_data(
            smi_id::A_SYSTEM_ACL_REDIRECT_SFLOW_TO_CPU);
        status |= iter->second.set_arg(
            smi_id::P_SYSTEM_ACL_REDIRECT_SFLOW_TO_CPU_REASON_CODE,
            cpu_redirect_reason_code);
        status |= iter->second.set_arg(
            smi_id::P_SYSTEM_ACL_REDIRECT_SFLOW_TO_CPU_QID, qid);
        status |= iter->second.set_arg(
            smi_id::P_SYSTEM_ACL_REDIRECT_SFLOW_TO_CPU_METER_ID, meter_handle);
        if (meter_handle.data != 0) {
          status |= switch_store::v_set(
              meter_handle, SWITCH_METER_ATTR_TARGET_TYPE, ing_target_type);
        }
        status |= iter->second.set_arg(
            smi_id::P_SYSTEM_ACL_REDIRECT_SFLOW_TO_CPU_DISABLE_LEARNING, false);
        status |= iter->second.set_arg(
            smi_id::P_SYSTEM_ACL_REDIRECT_SFLOW_TO_CPU_OVERWRITE_QID, true);
        break;
      case SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_REDIRECT_TO_CPU:
        iter->second.init_action_data(smi_id::A_SYSTEM_ACL_REDIRECT_TO_CPU);
        status |= iter->second.set_arg(
            smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_REASON_CODE,
            cpu_redirect_reason_code);
        status |=
            iter->second.set_arg(smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_QID, qid);
        status |= iter->second.set_arg(
            smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_METER_ID, meter_handle);
        if (meter_handle.data != 0) {
          status |= switch_store::v_set(
              meter_handle, SWITCH_METER_ATTR_TARGET_TYPE, ing_target_type);
        }
        status |= iter->second.set_arg(
            smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_DISABLE_LEARNING, false);
        status |= iter->second.set_arg(
            smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_OVERWRITE_QID, true);
        break;
      case SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_COPY_TO_CPU:
        iter->second.init_action_data(smi_id::A_SYSTEM_ACL_COPY_TO_CPU);
        status |=
            iter->second.set_arg(smi_id::P_SYSTEM_ACL_COPY_TO_CPU_REASON_CODE,
                                 cpu_redirect_reason_code);
        status |=
            iter->second.set_arg(smi_id::P_SYSTEM_ACL_COPY_TO_CPU_QID, qid);
        status |= iter->second.set_arg(
            smi_id::P_SYSTEM_ACL_COPY_TO_CPU_METER_ID, meter_handle);
        if (meter_handle.data != 0) {
          status |= switch_store::v_set(
              meter_handle, SWITCH_METER_ATTR_TARGET_TYPE, ing_target_type);
        }
        status |= iter->second.set_arg(
            smi_id::P_SYSTEM_ACL_COPY_TO_CPU_OVERWRITE_QID, true);
        break;
      case SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_COPY_SFLOW_TO_CPU:
        iter->second.init_action_data(smi_id::A_SYSTEM_ACL_COPY_SFLOW_TO_CPU);
        status |= iter->second.set_arg(
            smi_id::P_SYSTEM_ACL_COPY_SFLOW_TO_CPU_REASON_CODE,
            cpu_redirect_reason_code);
        status |= iter->second.set_arg(
            smi_id::P_SYSTEM_ACL_COPY_SFLOW_TO_CPU_QID, qid);
        status |= iter->second.set_arg(
            smi_id::P_SYSTEM_ACL_COPY_SFLOW_TO_CPU_METER_ID, meter_handle);
        if (meter_handle.data != 0) {
          status |= switch_store::v_set(
              meter_handle, SWITCH_METER_ATTR_TARGET_TYPE, ing_target_type);
        }
        status |= iter->second.set_arg(
            smi_id::P_SYSTEM_ACL_COPY_SFLOW_TO_CPU_OVERWRITE_QID, true);
        break;
      default:
        break;
    }

    return SWITCH_STATUS_SUCCESS;
  }

  switch_status_t create_hostif_trap_type_sflow() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    cpu_redirect_reason_code = SWITCH_SFLOW_REASON_CODE;
    if (packet_action.enumdata ==
        SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_REDIRECT_TO_CPU) {
      packet_action.enumdata =
          SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_REDIRECT_SFLOW_TO_CPU;
    } else if (packet_action.enumdata ==
               SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_COPY_TO_CPU) {
      packet_action.enumdata =
          SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_COPY_SFLOW_TO_CPU;
    }

    /** create system acl entry to filter sflow packets. */
    uint8_t sample = 1;

    it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
    it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_SAMPLE_PACKET, sample, sample);
    status |= setup_action_data(it);

    return status;
  }

  switch_status_t create_hostif_trap_type_arp_request() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    uint16_t eth_type = SWITCH_ETHERTYPE_ARP;
    uint16_t eth_mask = 0xFFFF;
    uint16_t arp_code = SWITCH_ARP_OPCODE_REQ;
    uint16_t arp_code_mask = 0xFFFF;
    uint8_t pkt_type_mask = 0x3;
    uint8_t ipv4_unicast_enable = 1;
    uint8_t ipv4_unicast_enable_mask = 1;
    uint8_t rmac_hit = 1;
    uint8_t rmac_hit_mask = 1;
    uint8_t pv_miss = 0;
    uint8_t pv_miss_mask = 1;

    /* create system acl entry to flter ARP broadcast packet */
    {
      uint8_t pkt_type = SWITCH_PACKET_TYPE_BROADCAST;
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_PKT_TYPE, pkt_type, pkt_type_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_MAC_TYPE, eth_type, eth_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_ARP_OPCODE, arp_code, arp_code_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV4_MD_UNICAST_ENABLE,
          ipv4_unicast_enable,
          ipv4_unicast_enable_mask);
      status |= add_stp_forwarding_flag(it);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
          pv_miss,
          pv_miss_mask);
      status |= setup_action_data(it);
    }

    /* create system acl_entry to flter unicast ARP request packet */
    {
      uint8_t pkt_type = SWITCH_PACKET_TYPE_UNICAST;
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_PKT_TYPE, pkt_type, pkt_type_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_MAC_TYPE, eth_type, eth_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_ARP_OPCODE, arp_code, arp_code_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV4_MD_UNICAST_ENABLE,
          ipv4_unicast_enable,
          ipv4_unicast_enable_mask);
      status |= add_stp_forwarding_flag(it);
      status |=
          it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_RMAC_HIT,
                                rmac_hit,
                                rmac_hit_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
          pv_miss,
          pv_miss_mask);
      status |= setup_action_data(it);
    }

    return status;
  }

  switch_status_t create_hostif_trap_type_arp_suppress() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    uint16_t eth_type = SWITCH_ETHERTYPE_ARP;
    uint16_t eth_mask = 0xFFFF;
    uint16_t arp_code = SWITCH_ARP_OPCODE_REQ;
    uint16_t arp_code_mask = 0xFFFF;
    uint8_t pkt_type_mask = 0x3;
    uint8_t ipv4_unicast_enable = 1;
    uint8_t ipv4_unicast_enable_mask = 1;
    uint8_t pv_miss = 0;
    uint8_t pv_miss_mask = 1;

    /* create system acl entry to flter ARP broadcast packet */
    {
      uint8_t pkt_type = SWITCH_PACKET_TYPE_BROADCAST;
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_PKT_TYPE, pkt_type, pkt_type_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_MAC_TYPE, eth_type, eth_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_ARP_OPCODE, arp_code, arp_code_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV4_MD_UNICAST_ENABLE,
          ipv4_unicast_enable,
          ipv4_unicast_enable_mask);
      status |= add_stp_forwarding_flag(it);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_ARP_SUPPRESS,
          static_cast<uint8_t>(1),
          static_cast<uint8_t>(1));
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
          pv_miss,
          pv_miss_mask);
      status |= setup_action_data(it);
    }

    return status;
  }

  switch_status_t create_hostif_trap_type_arp_response() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    uint16_t eth_type = SWITCH_ETHERTYPE_ARP;
    uint16_t eth_mask = 0xFFFF;
    uint16_t arp_code = SWITCH_ARP_OPCODE_RES;
    uint16_t arp_code_mask = 0xFFFF;
    uint8_t pkt_type_mask = 0x3;
    uint8_t ipv4_unicast_enable = 1;
    uint8_t ipv4_unicast_enable_mask = 1;
    uint8_t rmac_hit = 1;
    uint8_t rmac_hit_mask = 1;
    uint8_t pv_miss = 0;
    uint8_t pv_miss_mask = 1;

    /* Broadcast ARP Response (very rare) */
    {
      uint8_t pkt_type = SWITCH_PACKET_TYPE_BROADCAST;
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_PKT_TYPE, pkt_type, pkt_type_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_MAC_TYPE, eth_type, eth_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_ARP_OPCODE, arp_code, arp_code_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV4_MD_UNICAST_ENABLE,
          ipv4_unicast_enable,
          ipv4_unicast_enable_mask);
      status |= add_stp_forwarding_flag(it);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
          pv_miss,
          pv_miss_mask);
      status |= setup_action_data(it);
    }

    /* create system acl_entry to flter unicast ARP responce packet */
    {
      uint8_t pkt_type = SWITCH_PACKET_TYPE_UNICAST;
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_PKT_TYPE, pkt_type, pkt_type_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_MAC_TYPE, eth_type, eth_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_ARP_OPCODE, arp_code, arp_code_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV4_MD_UNICAST_ENABLE,
          ipv4_unicast_enable,
          ipv4_unicast_enable_mask);
      status |= add_stp_forwarding_flag(it);
      status |=
          it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_RMAC_HIT,
                                rmac_hit,
                                rmac_hit_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
          pv_miss,
          pv_miss_mask);
      status |= setup_action_data(it);
    }

    return status;
  }

  switch_status_t create_hostif_trap_type_stp() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    switch_mac_addr_t mac_mask = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    switch_mac_addr_t ieee_bpdu_mac = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x00};

    it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_MAC_DST_ADDR, ieee_bpdu_mac, mac_mask);
    status |= setup_action_data(it);

    return status;
  }

  switch_status_t create_hostif_trap_type_pvrst() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    switch_mac_addr_t mac_mask = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    switch_mac_addr_t pvst_bdpu_mac = {0x01, 0x00, 0x0C, 0xCC, 0xCC, 0xCD};

    it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_MAC_DST_ADDR, pvst_bdpu_mac, mac_mask);
    status |= setup_action_data(it);

    return status;
  }

  switch_status_t create_hostif_trap_type_lacp() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    switch_mac_addr_t lacp_bpdu_mac = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x02};
    switch_mac_addr_t mac_mask = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_MAC_DST_ADDR, lacp_bpdu_mac, mac_mask);
    status |= setup_action_data(it);

    return status;
  }

  switch_status_t create_hostif_trap_type_eapol() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    uint16_t eth_type = SWITCH_ETHERTYPE_EAPOL;
    uint16_t eth_mask = 0xFFFF;

    it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_MAC_TYPE, eth_type, eth_mask);
    status |= setup_action_data(it);

    return status;
  }

  switch_status_t create_hostif_trap_type_lldp() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    uint16_t eth_type = SWITCH_ETHERTYPE_LLDP;
    uint16_t eth_mask = 0xFFFF;
    switch_mac_addr_t mac_mask = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    switch_mac_addr_t lldp_bpdu_mac = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x0e};
    switch_mac_addr_t lldp_bpdu_mac_2 = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x03};

    for (auto mac : {lldp_bpdu_mac, lldp_bpdu_mac_2}) {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_MAC_DST_ADDR, mac, mac_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_MAC_TYPE, eth_type, eth_mask);
      status |= setup_action_data(it);
    }

    return status;
  }

  switch_status_t create_hostif_trap_type_isis() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint8_t pv_miss = 0, pv_miss_mask = 1;

    switch_mac_addr_t mac_mask = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    switch_mac_addr_t isis_mac = {0x09, 0x00, 0x2b, 0x00, 0x00, 0x05};
    switch_mac_addr_t isis_mac_2 = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x14};
    switch_mac_addr_t isis_mac_3 = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x15};

    for (auto mac : {isis_mac, isis_mac_2, isis_mac_3}) {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_MAC_DST_ADDR, mac, mac_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
          pv_miss,
          pv_miss_mask);
      status |= add_stp_forwarding_flag(it);
      status |= setup_action_data(it);
    }

    return status;
  }

  switch_status_t create_hostif_trap_type_ospf() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    uint8_t ip_proto = SWITCH_HOSTIF_IP_PROTO_OSPF;
    uint8_t ip_proto_mask = 0xFF;
    uint8_t pv_miss = 0, pv_miss_mask = 1;
    uint8_t ipv4_unicast_enable = 1;
    uint8_t ipv4_unicast_enable_mask = 1;
    switch_ip_address_t ip_addr_mask;
    memset(&ip_addr_mask, 0xFF, sizeof(switch_ip_address_t));
    switch_ip_address_t all_ospf_route_addr;
    all_ospf_route_addr.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
    all_ospf_route_addr.ip4 = 0xE0000005;
    switch_ip_address_t dr_ospf_route_addr;
    dr_ospf_route_addr.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
    dr_ospf_route_addr.ip4 = 0xE0000006;

    /* ceate system acl entry to filter ospf packet */
    /* case1: All OSPF routers 224.0.0.5 */
    /* case2: All OSPF designated routes (DRs) 224.0.0.6, copy to cpu */
    for (auto ip_addr : {all_ospf_route_addr, dr_ospf_route_addr}) {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV4_MD_UNICAST_ENABLE,
          ipv4_unicast_enable,
          ipv4_unicast_enable_mask);
      status |= it->first.set_ip_unified_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_DST_ADDR, ip_addr, ip_addr_mask);
      status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_LKP_IP_TYPE,
                                      static_cast<uint8_t>(1),
                                      static_cast<uint8_t>(3));
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_PROTO, ip_proto, ip_proto_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
          pv_miss,
          pv_miss_mask);
      status |= add_stp_forwarding_flag(it);
      status |= setup_action_data(it);
    }

    return status;
  }

  switch_status_t create_hostif_trap_type_ospfv6() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    uint8_t ip_proto = SWITCH_HOSTIF_IP_PROTO_OSPF;
    uint8_t ip_proto_mask = 0xFF;
    uint8_t pv_miss = 0, pv_miss_mask = 1;
    uint8_t unicast_enable = 1;
    uint8_t unicast_enable_mask = 1;
    switch_ip_address_t ipv6_addr_mask;
    memset(&ipv6_addr_mask, 0xFF, sizeof(switch_ip_address_t));
    switch_ip_address_t all_ospf_route_ipv6_addr = {};
    all_ospf_route_ipv6_addr.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6;
    inet_pton(AF_INET6, "FF02::5", all_ospf_route_ipv6_addr.ip6);
    switch_ip_address_t dr_ospf_route_ipv6_addr = {};
    dr_ospf_route_ipv6_addr.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6;
    inet_pton(AF_INET6, "FF02::6", dr_ospf_route_ipv6_addr.ip6);

    /*
     * create ipv6 acl entry to filter ospf packet
     * case1: All OSPF routers FF02::5
     * case2: DR/BDR OSPF routers FF02::6
     */
    for (auto ip_addr : {all_ospf_route_ipv6_addr, dr_ospf_route_ipv6_addr}) {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV6_MD_UNICAST_ENABLE,
          unicast_enable,
          unicast_enable_mask);
      status |= it->first.set_ip_unified_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_DST_ADDR, ip_addr, ipv6_addr_mask);
      status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_LKP_IP_TYPE,
                                      static_cast<uint8_t>(2),
                                      static_cast<uint8_t>(3));
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_PROTO, ip_proto, ip_proto_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
          pv_miss,
          pv_miss_mask);
      status |= add_stp_forwarding_flag(it);
      status |= setup_action_data(it);
    }

    return status;
  }

  switch_status_t create_hostif_trap_type_bgp(bool ipv6) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    uint8_t ip_proto = SWITCH_HOSTIF_IP_PROTO_TCP;
    uint8_t ip_proto_mask = 0xFF;
    uint16_t bgp_l4_port = SWITCH_HOSTIF_BGP_PORT;
    uint16_t bgp_l4_port_mask = 0xFFFF;
    uint8_t unicast_enable = true;
    uint8_t unicast_enable_mask = true;
    switch_myip_type_t my_ip = SWITCH_DEVICE_ATTR_MYIP_TYPE_HOST;
    switch_myip_type_t my_ip_mask = SWITCH_DEVICE_ATTR_MYIP_TYPE_MASK;
    uint16_t eth_type = ipv6 ? SWITCH_ETHERTYPE_IPV6 : SWITCH_ETHERTYPE_IPV4;
    uint16_t eth_mask = 0xFFFF;
    uint8_t pv_miss = 0, pv_miss_mask = 1;

    /* create system acl entry to filter bgp packet
     * case1: tcp dest port equal to 179(BGP), copy to cpu
     */
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_PROTO, ip_proto, ip_proto_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_MAC_TYPE, eth_type, eth_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_L4_DST_PORT, bgp_l4_port, bgp_l4_port_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_MYIP, my_ip, my_ip_mask);
      if (ipv6) {
        status |= it->first.set_ternary(
            smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV6_MD_UNICAST_ENABLE,
            unicast_enable,
            unicast_enable_mask);
      } else {
        status |= it->first.set_ternary(
            smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV4_MD_UNICAST_ENABLE,
            unicast_enable,
            unicast_enable_mask);
      }
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
          pv_miss,
          pv_miss_mask);
      status |= setup_action_data(it);
    }

    /*
     * case2: tcp src port equal to 179(BGP), copy to cpu
     */
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_PROTO, ip_proto, ip_proto_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_MAC_TYPE, eth_type, eth_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_L4_SRC_PORT, bgp_l4_port, bgp_l4_port_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_MYIP, my_ip, my_ip_mask);
      if (ipv6) {
        status |= it->first.set_ternary(
            smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV6_MD_UNICAST_ENABLE,
            unicast_enable,
            unicast_enable_mask);
      } else {
        status |= it->first.set_ternary(
            smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV4_MD_UNICAST_ENABLE,
            unicast_enable,
            unicast_enable_mask);
      }
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
          pv_miss,
          pv_miss_mask);
      status |= setup_action_data(it);
    }

    return status;
  }

  switch_status_t create_hostif_trap_type_pim() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    /* create ip acl entry to filter pim packet */
    uint8_t ip_proto = SWITCH_HOSTIF_IP_PROTO_PIM;
    uint8_t ip_proto_mask = 0xFF;
    uint8_t unicast = 1;

    it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_IP_PROTO, ip_proto, ip_proto_mask);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV4_MD_UNICAST_ENABLE, unicast, unicast);
    status |= add_stp_forwarding_flag(it);
    status |= setup_action_data(it);

    return status;
  }

  switch_status_t create_hostif_trap_type_igmp(uint16_t msg_type) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (feature::is_feature_set(SWITCH_FEATURE_MULTICAST)) {
      /* create ip acl entry to filter igmp packets */
      uint8_t ip_proto = SWITCH_HOSTIF_IP_PROTO_IGMP;
      uint8_t ip_proto_mask = 0xFF;
      // IGMP type is overloaded with l4 src port in dataplane
      uint16_t l4_port = msg_type;
      uint16_t l4_port_mask = 0xFFFF;

      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_PROTO, ip_proto, ip_proto_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_L4_SRC_PORT, l4_port, l4_port_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV4_MD_MULTICAST_SNOOPING,
          static_cast<uint8_t>(1),
          static_cast<uint8_t>(1));
      status |= setup_action_data(it);
    }

    return status;
  }

  switch_status_t create_hostif_trap_type_ipv6_mld(uint16_t msg_type) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint8_t pv_miss = 0, pv_miss_mask = 1;

    if (feature::is_feature_set(SWITCH_FEATURE_MULTICAST)) {
      /* create ip acl entry to filter ipv6 mld packets */
      uint8_t ip_proto = SWITCH_HOSTIF_IP_PROTO_ICMPV6;
      uint8_t ip_proto_mask = 0xFF;
      // IPV6 MLD type is overloaded with l4 src port in dataplane
      uint16_t l4_port = msg_type;
      uint16_t l4_port_mask = 0xFF;

      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_PROTO, ip_proto, ip_proto_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_L4_SRC_PORT, l4_port, l4_port_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV6_MD_MULTICAST_SNOOPING,
          static_cast<uint8_t>(1),
          static_cast<uint8_t>(1));
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
          pv_miss,
          pv_miss_mask);
      status |= add_stp_forwarding_flag(it);
      status |= setup_action_data(it);
    }

    return status;
  }

  switch_status_t create_hostif_trap_type_ssh() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    uint8_t ip_proto = SWITCH_HOSTIF_IP_PROTO_TCP;
    uint8_t ip_proto_mask = 0xFF;
    switch_myip_type_t my_ip = SWITCH_DEVICE_ATTR_MYIP_TYPE_HOST;
    switch_myip_type_t my_ip_mask = SWITCH_DEVICE_ATTR_MYIP_TYPE_MASK;
    uint16_t l4_port = SWITCH_HOSTIF_SSH_PORT;
    uint16_t l4_port_mask = 0xFFFF;
    uint8_t pv_miss = 0, pv_miss_mask = 1;

    /*
     * case-1: TCP dst port equal to 22(SSH), redirect to cpu
     */
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_PROTO, ip_proto, ip_proto_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_L4_DST_PORT, l4_port, l4_port_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_MYIP, my_ip, my_ip_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
          pv_miss,
          pv_miss_mask);
      status |= setup_action_data(it);
    }

    /*
     * case-2: TCP src port equal to 22(SSH), redirect to cpu
     */
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_PROTO, ip_proto, ip_proto_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_L4_SRC_PORT, l4_port, l4_port_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_MYIP, my_ip, my_ip_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
          pv_miss,
          pv_miss_mask);
      status |= setup_action_data(it);
    }

    return status;
  }

  switch_status_t create_hostif_trap_type_gnmi() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    uint8_t ip_proto = SWITCH_HOSTIF_IP_PROTO_TCP;
    uint8_t ip_proto_mask = 0xFF;
    switch_myip_type_t my_ip = SWITCH_DEVICE_ATTR_MYIP_TYPE_HOST;
    switch_myip_type_t my_ip_mask = SWITCH_DEVICE_ATTR_MYIP_TYPE_MASK;
    uint16_t l4_port = SWITCH_HOSTIF_GNMI_PORT;
    uint16_t l4_port_mask = 0xFFFF;
    uint8_t pv_miss = 0, pv_miss_mask = 1;

    /*
     * case-1: TCP dst port equal to 9559(P4RT), redirect to cpu
     */
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_PROTO, ip_proto, ip_proto_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_L4_DST_PORT, l4_port, l4_port_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_MYIP, my_ip, my_ip_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
          pv_miss,
          pv_miss_mask);
      status |= setup_action_data(it);
    }

    return status;
  }

  switch_status_t create_hostif_trap_type_p4rt() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    uint8_t ip_proto = SWITCH_HOSTIF_IP_PROTO_TCP;
    uint8_t ip_proto_mask = 0xFF;
    switch_myip_type_t my_ip = SWITCH_DEVICE_ATTR_MYIP_TYPE_HOST;
    switch_myip_type_t my_ip_mask = SWITCH_DEVICE_ATTR_MYIP_TYPE_MASK;
    uint16_t l4_port = SWITCH_HOSTIF_P4RT_PORT;
    uint16_t l4_port_mask = 0xFFFF;
    uint8_t pv_miss = 0, pv_miss_mask = 1;

    /*
     * case-1: TCP dst port equal to 9559(P4RT), redirect to cpu
     */
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_PROTO, ip_proto, ip_proto_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_L4_DST_PORT, l4_port, l4_port_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_MYIP, my_ip, my_ip_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
          pv_miss,
          pv_miss_mask);
      status |= setup_action_data(it);
    }

    return status;
  }

  switch_status_t create_hostif_trap_type_ntpc(bool ntpserver) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    std::vector<uint8_t> ip_protos = {SWITCH_HOSTIF_IP_PROTO_TCP,
                                      SWITCH_HOSTIF_IP_PROTO_UDP};
    uint8_t ip_proto_mask = 0xFF;
    switch_myip_type_t my_ip = SWITCH_DEVICE_ATTR_MYIP_TYPE_HOST;
    switch_myip_type_t my_ip_mask = SWITCH_DEVICE_ATTR_MYIP_TYPE_MASK;
    uint16_t l4_port = SWITCH_HOSTIF_NTPCLIENT_PORT;
    uint16_t l4_port_mask = 0xFFFF;
    uint8_t pv_miss = 0, pv_miss_mask = 1;
    auto l4_key = ntpserver ? smi_id::F_SYSTEM_ACL_LKP_L4_DST_PORT
                            : smi_id::F_SYSTEM_ACL_LKP_L4_SRC_PORT;
    /*
     * case-1: TCP/UDP src/dst port equal to 123, redirect to cpu
     */
    for (auto ip_proto : ip_protos) {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_PROTO, ip_proto, ip_proto_mask);
      status |= it->first.set_ternary(l4_key, l4_port, l4_port_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_MYIP, my_ip, my_ip_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
          pv_miss,
          pv_miss_mask);
      status |= setup_action_data(it);
    }

    return status;
  }

  switch_status_t create_hostif_trap_type_dhcp() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    uint8_t ip_proto = SWITCH_HOSTIF_IP_PROTO_UDP;
    uint8_t ip_proto_mask = 0xFF;
    uint8_t pv_miss = 0, pv_miss_mask = 1;
    uint16_t dhcp_l4_port1 = SWITCH_HOSTIF_DHCP_PORT1;
    uint16_t dhcp_l4_port2 = SWITCH_HOSTIF_DHCP_PORT2;
    uint16_t l4_port_mask = 0xFFFF;
    switch_myip_type_t my_ip = SWITCH_DEVICE_ATTR_MYIP_TYPE_HOST;
    switch_myip_type_t my_ip_mask = SWITCH_DEVICE_ATTR_MYIP_TYPE_MASK;
    switch_ip_address_t ip_bcast;
    switch_ip_address_t ip_addr_mask;

    ip_bcast.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
    ip_bcast.ip4 = 0xFFFFFFFF;
    memset(&ip_addr_mask, 0xff, sizeof(switch_ip_address_t));

    /* UDP sport equal to 67 and dport equal to 68(DHCP) */
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_PROTO, ip_proto, ip_proto_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_L4_SRC_PORT, dhcp_l4_port1, l4_port_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_L4_DST_PORT, dhcp_l4_port2, l4_port_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_MYIP, my_ip, my_ip_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_TYPE,
          static_cast<uint8_t>(SWITCH_IP_ADDR_FAMILY_IPV4),
          static_cast<uint8_t>(3));
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
          pv_miss,
          pv_miss_mask);
      status |= add_stp_forwarding_flag(it);
      status |= setup_action_data(it);
    }

    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_PROTO, ip_proto, ip_proto_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_L4_SRC_PORT, dhcp_l4_port1, l4_port_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_L4_DST_PORT, dhcp_l4_port2, l4_port_mask);
      status |= it->first.set_ip_unified_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_DST_ADDR, ip_bcast, ip_addr_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_TYPE,
          static_cast<uint8_t>(SWITCH_IP_ADDR_FAMILY_IPV4),
          static_cast<uint8_t>(3));
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
          pv_miss,
          pv_miss_mask);
      status |= add_stp_forwarding_flag(it);
      status |= setup_action_data(it);
    }

    /*
     * Originally, in client to server DHCP message, UDP sport should be
     * equal to 68 and dport equal to 67(DHCP). However, if the client
     * is SNAT'd, the sport could be changed to a non-standard port
     * (i.e., not 68). So, we should mask it out and match on dport only.
     */
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_PROTO, ip_proto, ip_proto_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_L4_DST_PORT, dhcp_l4_port1, l4_port_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_MYIP, my_ip, my_ip_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_TYPE,
          static_cast<uint8_t>(SWITCH_IP_ADDR_FAMILY_IPV4),
          static_cast<uint8_t>(3));
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
          pv_miss,
          pv_miss_mask);
      status |= add_stp_forwarding_flag(it);
      status |= setup_action_data(it);
    }

    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_PROTO, ip_proto, ip_proto_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_L4_DST_PORT, dhcp_l4_port1, l4_port_mask);
      status |= it->first.set_ip_unified_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_DST_ADDR, ip_bcast, ip_addr_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_TYPE,
          static_cast<uint8_t>(SWITCH_IP_ADDR_FAMILY_IPV4),
          static_cast<uint8_t>(3));
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
          pv_miss,
          pv_miss_mask);
      status |= add_stp_forwarding_flag(it);
      status |= setup_action_data(it);
    }

    return status;
  }

  switch_status_t create_hostif_trap_type_dhcpv6() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    uint8_t ip_proto = SWITCH_HOSTIF_IP_PROTO_UDP;
    uint8_t ip_proto_mask = 0xFF;
    uint8_t pv_miss = 0, pv_miss_mask = 1;
    uint16_t dhcp_l4_client_port = SWITCH_HOSTIF_DHCPV6_PORT1;
    uint16_t dhcp_l4_srv_port = SWITCH_HOSTIF_DHCPV6_PORT2;
    uint16_t l4_port_mask = 0xFFFF;
    uint8_t unicast_enable = true;
    uint8_t unicast_enable_mask = true;
    switch_myip_type_t my_ip = SWITCH_DEVICE_ATTR_MYIP_TYPE_HOST;
    switch_myip_type_t my_ip_mask = SWITCH_DEVICE_ATTR_MYIP_TYPE_MASK;
    switch_ip_address_t ipv6_addr_mask;
    memset(&ipv6_addr_mask, 0xFF, sizeof(switch_ip_address_t));

    switch_ip_address_t all_dhcp_relay_agents_and_servers = {};
    all_dhcp_relay_agents_and_servers.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6;
    inet_pton(AF_INET6, "FF02::1:2", all_dhcp_relay_agents_and_servers.ip6);

    switch_ip_address_t all_dhcp_servers = {};
    all_dhcp_servers.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6;
    inet_pton(AF_INET6, "FF05::1:3", all_dhcp_servers.ip6);

    /*
     * case1: All DHCPv6 relay agents and servers FF02::1:2
     * case2: All DHCPv6 servers FF05::1:3
     */
    for (auto ip_addr : {all_dhcp_relay_agents_and_servers, all_dhcp_servers}) {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV6_MD_UNICAST_ENABLE,
          unicast_enable,
          unicast_enable_mask);
      status |= it->first.set_ip_unified_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_DST_ADDR, ip_addr, ipv6_addr_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_TYPE,
          static_cast<uint8_t>(SWITCH_IP_ADDR_FAMILY_IPV6),
          static_cast<uint8_t>(3));
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_PROTO, ip_proto, ip_proto_mask);
      status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_LKP_L4_SRC_PORT,
                                      dhcp_l4_client_port,
                                      l4_port_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_L4_DST_PORT, dhcp_l4_srv_port, l4_port_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
          pv_miss,
          pv_miss_mask);
      status |= add_stp_forwarding_flag(it);
      status |= setup_action_data(it);
    }

    /* UDP sport - 546 dport - 547 (DHCPv6 unicast client->server msg).*/
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_TYPE,
          static_cast<uint8_t>(SWITCH_IP_ADDR_FAMILY_IPV6),
          static_cast<uint8_t>(3));
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_PROTO, ip_proto, ip_proto_mask);
      status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_LKP_L4_SRC_PORT,
                                      dhcp_l4_client_port,
                                      l4_port_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_L4_DST_PORT, dhcp_l4_srv_port, l4_port_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_MYIP, my_ip, my_ip_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
          pv_miss,
          pv_miss_mask);
      status |= add_stp_forwarding_flag(it);
      status |= setup_action_data(it);
    }

    /* UDP sport - 547 dport - 546 (DHCPv6 unicast server->client msg).*/
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_TYPE,
          static_cast<uint8_t>(SWITCH_IP_ADDR_FAMILY_IPV6),
          static_cast<uint8_t>(3));
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_PROTO, ip_proto, ip_proto_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_L4_SRC_PORT, dhcp_l4_srv_port, l4_port_mask);
      status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_LKP_L4_DST_PORT,
                                      dhcp_l4_client_port,
                                      l4_port_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_MYIP, my_ip, my_ip_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
          pv_miss,
          pv_miss_mask);
      status |= add_stp_forwarding_flag(it);
      status |= setup_action_data(it);
    }

    return status;
  }

  switch_status_t create_hostif_trap_type_dhcp_l2() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    uint8_t ip_proto = SWITCH_HOSTIF_IP_PROTO_UDP;
    uint8_t ip_proto_mask = 0xFF;
    uint16_t dhcp_l4_port1 = SWITCH_HOSTIF_DHCP_PORT1;
    uint16_t dhcp_l4_port2 = SWITCH_HOSTIF_DHCP_PORT2;
    uint16_t l4_port_mask = 0xFFFF;

    /* UDP sport equal to 67 and dport equal to 68(DHCP) */
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_PROTO, ip_proto, ip_proto_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_L4_SRC_PORT, dhcp_l4_port1, l4_port_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_L4_DST_PORT, dhcp_l4_port2, l4_port_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_TYPE,
          static_cast<uint8_t>(SWITCH_IP_ADDR_FAMILY_IPV4),
          static_cast<uint8_t>(3));
      status |= setup_action_data(it);
    }

    /* UDP sport equal to 68 and dport equal to 67(DHCP) */
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_PROTO, ip_proto, ip_proto_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_L4_SRC_PORT, dhcp_l4_port2, l4_port_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_L4_DST_PORT, dhcp_l4_port1, l4_port_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_TYPE,
          static_cast<uint8_t>(SWITCH_IP_ADDR_FAMILY_IPV4),
          static_cast<uint8_t>(3));
      status |= setup_action_data(it);
    }

    return status;
  }

  switch_status_t create_hostif_trap_type_dhcpv6_l2() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    uint8_t ip_proto = SWITCH_HOSTIF_IP_PROTO_UDP;
    uint8_t ip_proto_mask = 0xFF;
    uint16_t dhcp_l4_client_port = SWITCH_HOSTIF_DHCPV6_PORT1;
    uint16_t dhcp_l4_srv_port = SWITCH_HOSTIF_DHCPV6_PORT2;
    uint16_t l4_port_mask = 0xFFFF;

    /* UDP sport - 546 dport - 547 (DHCPv6 unicast client->server msg).*/
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_PROTO, ip_proto, ip_proto_mask);
      status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_LKP_L4_SRC_PORT,
                                      dhcp_l4_client_port,
                                      l4_port_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_L4_DST_PORT, dhcp_l4_srv_port, l4_port_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_TYPE,
          static_cast<uint8_t>(SWITCH_IP_ADDR_FAMILY_IPV6),
          static_cast<uint8_t>(3));
      status |= setup_action_data(it);
    }

    /* UDP sport - 547 dport - 546 (DHCPv6 unicast server->client msg).*/
    {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_PROTO, ip_proto, ip_proto_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_L4_SRC_PORT, dhcp_l4_srv_port, l4_port_mask);
      status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_LKP_L4_DST_PORT,
                                      dhcp_l4_client_port,
                                      l4_port_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_TYPE,
          static_cast<uint8_t>(SWITCH_IP_ADDR_FAMILY_IPV6),
          static_cast<uint8_t>(3));
      status |= setup_action_data(it);
    }

    return status;
  }

  switch_status_t create_hostif_trap_type_snmp() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_myip_type_t my_ip = SWITCH_DEVICE_ATTR_MYIP_TYPE_HOST;
    switch_myip_type_t my_ip_mask = SWITCH_DEVICE_ATTR_MYIP_TYPE_MASK;
    uint8_t pv_miss = 0, pv_miss_mask = 1;

    /*
     * UDP dest port equal to 161(SNMP), redirect to cpu
     */
    uint8_t ip_proto = SWITCH_HOSTIF_IP_PROTO_UDP;
    uint8_t ip_proto_mask = 0xFF;
    uint16_t l4_port = SWITCH_HOSTIF_SNMP_PORT;
    uint16_t l4_port_mask = 0xFFFF;

    it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_IP_PROTO, ip_proto, ip_proto_mask);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_L4_DST_PORT, l4_port, l4_port_mask);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_MYIP, my_ip, my_ip_mask);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
        pv_miss,
        pv_miss_mask);
    status |= setup_action_data(it);

    return status;
  }

  switch_status_t create_hostif_trap_type_icmp() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    /*
     * create catchall for icmp packets
     */

    uint16_t eth_type = SWITCH_ETHERTYPE_IPV4;
    uint16_t eth_mask = 0xFFFF;
    uint8_t ip_proto = SWITCH_HOSTIF_IP_PROTO_ICMP;
    uint8_t ip_proto_mask = 0xFF;
    uint8_t unicast_enable = 1;
    uint8_t pv_miss = 0;
    uint8_t pv_miss_mask = 1;

    it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_IP_PROTO, ip_proto, ip_proto_mask);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_MAC_TYPE, eth_type, eth_mask);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV4_MD_UNICAST_ENABLE,
        unicast_enable,
        unicast_enable);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
        pv_miss,
        pv_miss_mask);
    status |= setup_action_data(it);

    return status;
  }

  switch_status_t create_hostif_trap_type_icmpv6() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    /*
     * create catchall for icmpv6 packets
     * more specific higher priority traps for MLD/ND packets
     */

    uint16_t eth_type = SWITCH_ETHERTYPE_IPV6;
    uint16_t eth_mask = 0xFFFF;
    uint8_t ip_proto = SWITCH_HOSTIF_IP_PROTO_ICMPV6;
    uint8_t ip_proto_mask = 0xFF;
    uint8_t unicast_enable = 1;
    uint8_t pv_miss = 0;
    uint8_t pv_miss_mask = 1;

    it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_IP_PROTO, ip_proto, ip_proto_mask);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_MAC_TYPE, eth_type, eth_mask);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV6_MD_UNICAST_ENABLE,
        unicast_enable,
        unicast_enable);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
        pv_miss,
        pv_miss_mask);
    status |= setup_action_data(it);

    return status;
  }

  switch_status_t create_hostif_trap_type_ipv6_nd() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    /*
     * create ipv6 acl entry to filter ND packets
     * case1: rule to classify NS packet.
     */

    uint16_t eth_type = SWITCH_ETHERTYPE_IPV6;
    uint16_t eth_mask = 0xFFFF;
    uint8_t ip_proto = SWITCH_HOSTIF_IP_PROTO_ICMPV6;
    uint8_t ip_proto_mask = 0xFF;
    uint8_t unicast_enable = 1;
    uint16_t l4_port_mask = 0xFFFF;
    uint8_t pv_miss = 0;
    uint8_t pv_miss_mask = 1;
    // ICMPv6 type is overloaded with l4 src port in dataplane
    for (uint16_t l4_src_port = SWITCH_HOSTIF_IPV6_ICMP_TYPE_RS;
         l4_src_port <= SWITCH_HOSTIF_IPV6_ICMP_REDIRECT;
         l4_src_port++) {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_PROTO, ip_proto, ip_proto_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_MAC_TYPE, eth_type, eth_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV6_MD_UNICAST_ENABLE,
          unicast_enable,
          unicast_enable);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_L4_SRC_PORT, l4_src_port, l4_port_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
          pv_miss,
          pv_miss_mask);
      status |= add_stp_forwarding_flag(it);
      status |= setup_action_data(it);
    }

    return status;
  }

  switch_status_t create_hostif_trap_type_ipv6_nd_suppress() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    /*
     * create ipv6 acl entry to filter ND packets
     * case1: rule to classify NS packet.
     */

    uint16_t eth_type = SWITCH_ETHERTYPE_IPV6;
    uint16_t eth_mask = 0xFFFF;
    uint8_t ip_proto = SWITCH_HOSTIF_IP_PROTO_ICMPV6;
    uint8_t ip_proto_mask = 0xFF;
    uint8_t unicast_enable = 1;
    uint16_t l4_port_mask = 0xFFFF;
    uint8_t pv_miss = 0;
    uint8_t pv_miss_mask = 1;
    // ICMPv6 type is overloaded with l4 src port in dataplane
    for (uint16_t l4_src_port = SWITCH_HOSTIF_IPV6_ICMP_TYPE_RS;
         l4_src_port <= SWITCH_HOSTIF_IPV6_ICMP_REDIRECT;
         l4_src_port++) {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_PROTO, ip_proto, ip_proto_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_MAC_TYPE, eth_type, eth_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV6_MD_UNICAST_ENABLE,
          unicast_enable,
          unicast_enable);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_L4_SRC_PORT, l4_src_port, l4_port_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
          pv_miss,
          pv_miss_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_ARP_SUPPRESS,
          static_cast<uint8_t>(1),
          static_cast<uint8_t>(1));
      status |= add_stp_forwarding_flag(it);
      status |= setup_action_data(it);
    }

    return status;
  }

  switch_status_t create_hostif_trap_type_ptp() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    // FIXME(bfn): fix in p4 also required

    /*
     * create ip acl entry to filter ptp packet
     * UDP dest port equal to 161(SNMP), redirect to cpu
     */

    std::set<attr_w> ptp_d_acl_entry_attrs;
    uint8_t ip_proto = SWITCH_HOSTIF_IP_PROTO_UDP;
    uint8_t ip_proto_mask = 0xFF;
    uint16_t l4_port_mask = 0xFFFF;

    for (uint16_t dst_port :
         {SWITCH_HOSTIF_PTP_DST_PORT1, SWITCH_HOSTIF_PTP_DST_PORT2}) {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_PROTO, ip_proto, ip_proto_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_L4_DST_PORT, dst_port, l4_port_mask);
      status |= setup_action_data(it);
    }

    /** case-2: Match on PTP ethertype and redirect the packet to CPU */
    {
      uint16_t eth_type = SWITCH_ETHERTYPE_PTP, eth_mask = 0xFFFF;
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_MAC_TYPE, eth_type, eth_mask);
      status |= setup_action_data(it);
    }

    return status;
  }

  switch_status_t create_hostif_trap_type_bfd() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    uint8_t ip_proto = SWITCH_HOSTIF_IP_PROTO_UDP;
    uint8_t ip_proto_mask = 0xFF;
    uint16_t bfd_l4_port_mask = 0xFFFF;
    switch_myip_type_t my_ip = SWITCH_DEVICE_ATTR_MYIP_TYPE_HOST;
    switch_myip_type_t my_ip_mask = SWITCH_DEVICE_ATTR_MYIP_TYPE_MASK;
    uint8_t pv_miss = 0, pv_miss_mask = 1;
    uint8_t bfd_to_cpu = 0, bfd_to_cpu_mask = 1;

    /** udp dst port 3784 and 4784 */
    for (uint16_t bfd_l4_port :
         {SWITCH_HOSTIF_BFD_DST_PORT1, SWITCH_HOSTIF_BFD_DST_PORT2}) {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_PROTO, ip_proto, ip_proto_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_L4_DST_PORT, bfd_l4_port, bfd_l4_port_mask);
      status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_LKP_IP_TYPE,
                                      static_cast<uint8_t>(1),
                                      static_cast<uint8_t>(3));
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_MYIP, my_ip, my_ip_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
          pv_miss,
          pv_miss_mask);
      if (feature::is_feature_set(SWITCH_FEATURE_BFD_OFFLOAD)) {
        status |=
            it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_BFD_TO_CPU,
                                  bfd_to_cpu,
                                  bfd_to_cpu_mask);
      }
      status |= setup_action_data(it);
    }

    return status;
  }

  switch_status_t create_hostif_trap_type_bfdv6() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    uint8_t ip_proto = SWITCH_HOSTIF_IP_PROTO_UDP;
    uint8_t ip_proto_mask = 0xFF;
    uint16_t bfd_l4_port_mask = 0xFFFF;
    switch_myip_type_t my_ip = SWITCH_DEVICE_ATTR_MYIP_TYPE_HOST;
    switch_myip_type_t my_ip_mask = SWITCH_DEVICE_ATTR_MYIP_TYPE_MASK;
    uint8_t pv_miss = 0, pv_miss_mask = 1;
    uint8_t bfd_to_cpu = 0, bfd_to_cpu_mask = 1;

    /** udp dst port 3784 and 4784 */
    for (uint16_t bfd_l4_port :
         {SWITCH_HOSTIF_BFD_DST_PORT1, SWITCH_HOSTIF_BFD_DST_PORT2}) {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_IP_PROTO, ip_proto, ip_proto_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LKP_L4_DST_PORT, bfd_l4_port, bfd_l4_port_mask);
      status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_LKP_IP_TYPE,
                                      static_cast<uint8_t>(2),
                                      static_cast<uint8_t>(3));
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_MYIP, my_ip, my_ip_mask);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
          pv_miss,
          pv_miss_mask);
      if (feature::is_feature_set(SWITCH_FEATURE_BFD_OFFLOAD)) {
        status |=
            it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_BFD_TO_CPU,
                                  bfd_to_cpu,
                                  bfd_to_cpu_mask);
      }
      status |= setup_action_data(it);
    }

    return status;
  }

  switch_status_t create_hostif_trap_type_vrrp() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    /*
     * create ip acl entry to filter VRRP packet
     */
    switch_ip_address_t vrrp_dest_addr;
    switch_ip_address_t ip_addr_mask;
    uint8_t ip_proto = SWITCH_HOSTIF_IP_PROTO_VRRP;
    uint8_t ip_proto_mask = 0xFF;
    uint8_t pv_miss = 0, pv_miss_mask = 1;
    uint8_t unicast_enable = 1;

    vrrp_dest_addr.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
    vrrp_dest_addr.ip4 = 0xE0000012;
    memset(&ip_addr_mask, 0xff, sizeof(switch_ip_address_t));

    it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV4_MD_UNICAST_ENABLE,
        unicast_enable,
        unicast_enable);
    status |= it->first.set_ip_unified_ternary(
        smi_id::F_SYSTEM_ACL_LKP_IP_DST_ADDR, vrrp_dest_addr, ip_addr_mask);
    status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_LKP_IP_TYPE,
                                    static_cast<uint8_t>(1),
                                    static_cast<uint8_t>(3));
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_IP_PROTO, ip_proto, ip_proto_mask);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
        pv_miss,
        pv_miss_mask);
    status |= add_stp_forwarding_flag(it);
    status |= setup_action_data(it);

    return status;
  }

  switch_status_t create_hostif_trap_type_vrrpv6() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    /*
     * create ip acl entry to filter VRRPV6 packet
     */
    switch_ip_address_t vrrpv6_dest_addr;
    switch_ip_address_t ipv6_addr_mask;
    uint8_t ip_proto = SWITCH_HOSTIF_IP_PROTO_VRRP;
    uint8_t ip_proto_mask = 0xFF;
    uint8_t pv_miss = 0, pv_miss_mask = 1;
    uint8_t unicast_enable = 1;

    vrrpv6_dest_addr.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6;
    inet_pton(AF_INET6, "FF02::12", vrrpv6_dest_addr.ip6);
    memset(&ipv6_addr_mask, 0xff, sizeof(switch_ip_address_t));

    it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV6_MD_UNICAST_ENABLE,
        unicast_enable,
        unicast_enable);
    status |= it->first.set_ip_unified_ternary(
        smi_id::F_SYSTEM_ACL_LKP_IP_DST_ADDR, vrrpv6_dest_addr, ipv6_addr_mask);
    status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_LKP_IP_TYPE,
                                    static_cast<uint8_t>(2),
                                    static_cast<uint8_t>(3));
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_IP_PROTO, ip_proto, ip_proto_mask);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
        pv_miss,
        pv_miss_mask);
    status |= add_stp_forwarding_flag(it);
    status |= setup_action_data(it);

    return status;
  }

  switch_status_t create_hostif_trap_type_ttl_error() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    auto create_entry = [&](switch_packet_action_t vrf_packet_action_key,
                            uint8_t vrf_action_valid,
                            uint64_t pkt_action,
                            uint32_t prio,
                            switch_object_id_t trap_grp) {
      uint8_t routed = 0x1;
      uint8_t vrf_action_valid_mask = 0x1;
      switch_enum_t pkt_action_e{pkt_action};

      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_DROP_REASON,
          static_cast<uint8_t>(SWITCH_DROP_REASON_OUTER_IP_TTL_ZERO),
          static_cast<uint8_t>(0xFF));

      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_ROUTED, routed, routed);
      if (feature::is_feature_set(SWITCH_FEATURE_MPLS)) {
        status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_HDR_MPLS_0_VALID,
                                        static_cast<uint8_t>(0),
                                        static_cast<uint8_t>(1));
      }

      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_VRF_TTL_VIOLATION_VALID,
          vrf_action_valid,
          vrf_action_valid_mask);
      if (vrf_action_valid) {
        status |= it->first.set_ternary(
            smi_id::F_SYSTEM_ACL_LOCAL_MD_VRF_TTL_VIOLATION,
            vrf_packet_action_key,
            static_cast<switch_packet_action_t>(0x3));
      }

      status |=
          setup_action_data_packet_action(it, pkt_action_e, prio, trap_grp);

      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_DROP_REASON,
                                      static_cast<uint8_t>(0),
                                      static_cast<uint8_t>(0xFF));
      status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_LKP_IP_TTL,
                                      static_cast<uint8_t>(1),
                                      static_cast<uint8_t>(0xFF));
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_ROUTED, routed, routed);
      if (feature::is_feature_set(SWITCH_FEATURE_MPLS)) {
        status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_HDR_MPLS_0_VALID,
                                        static_cast<uint8_t>(0),
                                        static_cast<uint8_t>(1));
      }

      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_VRF_TTL_VIOLATION_VALID,
          vrf_action_valid,
          vrf_action_valid_mask);
      if (vrf_action_valid) {
        status |= it->first.set_ternary(
            smi_id::F_SYSTEM_ACL_LOCAL_MD_VRF_TTL_VIOLATION,
            vrf_packet_action_key,
            static_cast<switch_packet_action_t>(0x3));
      }

      status |=
          setup_action_data_packet_action(it, pkt_action_e, prio, trap_grp);
    };

    create_entry(SWITCH_PACKET_ACTION_DROP,
                 1,
                 SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_DROP,
                 acl_priority,
                 default_trap_group_handle);
    create_entry(SWITCH_PACKET_ACTION_COPY,
                 1,
                 SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_COPY_TO_CPU,
                 acl_priority,
                 default_trap_group_handle);
    create_entry(SWITCH_PACKET_ACTION_PERMIT,
                 1,
                 SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_PERMIT,
                 acl_priority,
                 default_trap_group_handle);
    create_entry(SWITCH_PACKET_ACTION_TRAP,
                 1,
                 SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_REDIRECT_TO_CPU,
                 acl_priority,
                 default_trap_group_handle);
    create_entry(0, 0, packet_action.enumdata, acl_priority, trap_group_handle);

    return status;
  }

  switch_status_t create_hostif_trap_type_broadcast() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    /*
     * create mac acl to filter ieee_bpdu packet
     */
    uint8_t unicast = 1;
    switch_mac_addr_t b_mac = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV4_MD_UNICAST_ENABLE, unicast, unicast);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_MAC_DST_ADDR, b_mac, b_mac);
    status |= setup_action_data(it);

    return status;
  }

  switch_status_t create_hostif_trap_type_my_ip() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    /** create system acl entry to filter my_ip packets. */
    uint8_t unicast = 1;
    uint8_t pv_miss = 0, pv_miss_mask = 1;
    switch_myip_type_t my_ip = SWITCH_DEVICE_ATTR_MYIP_TYPE_HOST;
    switch_myip_type_t my_ip_mask = SWITCH_DEVICE_ATTR_MYIP_TYPE_MASK;

    /* v4 system acl */
    it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV4_MD_UNICAST_ENABLE, unicast, unicast);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_MYIP, my_ip, my_ip_mask);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
        pv_miss,
        pv_miss_mask);
    status |= setup_action_data(it);

    /* v6 system acl */
    it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV6_MD_UNICAST_ENABLE, unicast, unicast);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_MYIP, my_ip, my_ip_mask);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
        pv_miss,
        pv_miss_mask);
    status |= setup_action_data(it);
    return status;
  }

  switch_status_t create_hostif_trap_type_my_ip_subnet() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    /** create system acl entry to filter my_ip_subnet packets. */
    uint8_t unicast = 1;
    uint8_t pv_miss = 0, pv_miss_mask = 1;
    switch_myip_type_t my_ip = SWITCH_DEVICE_ATTR_MYIP_TYPE_SUBNET;
    switch_myip_type_t my_ip_mask = SWITCH_DEVICE_ATTR_MYIP_TYPE_MASK;

    /* v4 system acl */
    it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV4_MD_UNICAST_ENABLE, unicast, unicast);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_MYIP, my_ip, my_ip_mask);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
        pv_miss,
        pv_miss_mask);
    status |= setup_action_data(it);

    /* v6 system acl */
    it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV6_MD_UNICAST_ENABLE, unicast, unicast);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_MYIP, my_ip, my_ip_mask);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
        pv_miss,
        pv_miss_mask);
    status |= setup_action_data(it);

    return status;
  }

  switch_status_t create_hostif_trap_type_udld() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    switch_mac_addr_t udld_mac = {0x01, 0x00, 0x0C, 0xCC, 0xCC, 0xCC};
    switch_mac_addr_t mac_mask = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_MAC_DST_ADDR, udld_mac, mac_mask);
    status |= setup_action_data(it);

    return status;
  }

  switch_status_t create_hostif_trap_type_iccp() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    uint8_t tcp_ip_proto = SWITCH_HOSTIF_IP_PROTO_TCP;
    uint8_t ip_proto_mask = 0xFF;
    uint16_t iccp_l4_port = SWITCH_HOSTIF_ICCP_PORT;
    uint16_t iccp_l4_port_mask = 0xFFFF;
    uint8_t unicast_enable = true;
    uint8_t unicast_enable_mask = true;
    uint16_t eth_type = SWITCH_ETHERTYPE_IPV4;
    uint16_t eth_mask = 0xFFFF;
    uint8_t pv_miss = 0, pv_miss_mask = 1;
    switch_myip_type_t my_ip = SWITCH_DEVICE_ATTR_MYIP_TYPE_HOST;
    switch_myip_type_t my_ip_mask = SWITCH_DEVICE_ATTR_MYIP_TYPE_MASK;

    /* create system acl entry to filter iccp packets
     * locally terminated packets with TCP dst port 8888
     */

    // ipv4, tcp, dst_port 8888, myip
    it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_IP_PROTO, tcp_ip_proto, ip_proto_mask);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_MAC_TYPE, eth_type, eth_mask);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_L4_DST_PORT, iccp_l4_port, iccp_l4_port_mask);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV4_MD_UNICAST_ENABLE,
        unicast_enable,
        unicast_enable_mask);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_MYIP, my_ip, my_ip_mask);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
        pv_miss,
        pv_miss_mask);
    status |= setup_action_data(it);

    eth_type = SWITCH_ETHERTYPE_IPV6;
    // ipv6, tcp, dst_port 8888, myip
    it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_IP_PROTO, tcp_ip_proto, ip_proto_mask);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_MAC_TYPE, eth_type, eth_mask);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_L4_DST_PORT, iccp_l4_port, iccp_l4_port_mask);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV6_MD_UNICAST_ENABLE,
        unicast_enable,
        unicast_enable_mask);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_MYIP, my_ip, my_ip_mask);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
        pv_miss,
        pv_miss_mask);
    status |= setup_action_data(it);
    return status;
  }

  switch_status_t create_hostif_trap_type_ldp() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    uint8_t tcp_ip_proto = SWITCH_HOSTIF_IP_PROTO_TCP;
    uint8_t udp_ip_proto = SWITCH_HOSTIF_IP_PROTO_UDP;
    uint8_t ip_proto_mask = 0xFF;
    uint16_t ldp_l4_port = SWITCH_HOSTIF_LDP_PORT;
    uint16_t ldp_l4_port_mask = 0xFFFF;
    uint8_t unicast_enable = true;
    uint8_t unicast_enable_mask = true;
    uint16_t eth_type = SWITCH_ETHERTYPE_IPV4;
    uint16_t eth_mask = 0xFFFF;
    uint8_t pv_miss = 0, pv_miss_mask = 1;
    switch_myip_type_t my_ip = SWITCH_DEVICE_ATTR_MYIP_TYPE_HOST;
    switch_myip_type_t my_ip_mask = SWITCH_DEVICE_ATTR_MYIP_TYPE_MASK;
    switch_ip_address_t ip_mcast;
    switch_ip_address_t ip_addr_mask;

    ip_mcast.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
    ip_mcast.ip4 = 0xE0000002;
    memset(&ip_addr_mask, 0xff, sizeof(switch_ip_address_t));

    /* create system acl entry to filter ldp packets
     * https://datatracker.ietf.org/doc/html/rfc5036#section-3.10
     * The UDP port for LDP Hello messages is 646.
     * The TCP port for establishing LDP session connections is 646.
     */

    // tcp, dst_port 646, myip
    it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_IP_PROTO, tcp_ip_proto, ip_proto_mask);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_MAC_TYPE, eth_type, eth_mask);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_L4_DST_PORT, ldp_l4_port, ldp_l4_port_mask);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV4_MD_UNICAST_ENABLE,
        unicast_enable,
        unicast_enable_mask);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_MYIP, my_ip, my_ip_mask);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
        pv_miss,
        pv_miss_mask);
    status |= setup_action_data(it);

    // tcp, src_port 646, myip
    it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_IP_PROTO, tcp_ip_proto, ip_proto_mask);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_MAC_TYPE, eth_type, eth_mask);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_L4_SRC_PORT, ldp_l4_port, ldp_l4_port_mask);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV4_MD_UNICAST_ENABLE,
        unicast_enable,
        unicast_enable_mask);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_MYIP, my_ip, my_ip_mask);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
        pv_miss,
        pv_miss_mask);
    status |= setup_action_data(it);

    // udp, dst_port 646, 224.0.0.2
    it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_IP_PROTO, udp_ip_proto, ip_proto_mask);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_MAC_TYPE, eth_type, eth_mask);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_L4_DST_PORT, ldp_l4_port, ldp_l4_port_mask);
    status |= it->first.set_ip_unified_ternary(
        smi_id::F_SYSTEM_ACL_LKP_IP_DST_ADDR, ip_mcast, ip_addr_mask);
    status |= it->first.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS,
        pv_miss,
        pv_miss_mask);
    status |= setup_action_data(it);

    return status;
  }

  switch_status_t create_hostif_trap_type_dnat_miss() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    /* create ip acl entry to filter DNAT miss packets */

    if (feature::is_feature_set(SWITCH_FEATURE_NAT)) {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      uint8_t nat_type_mask = 0xF;
      uint8_t nat_miss_type = SWITCH_NAT_HIT_TYPE_DEST_NONE;
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_NAT_HIT, nat_miss_type, nat_type_mask);
      status |= setup_action_data(it);
    }
    return status;
  }

  switch_status_t create_hostif_trap_type_snat_miss() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    /* create ip acl entry to filter SNAT miss packets */

    if (feature::is_feature_set(SWITCH_FEATURE_NAT)) {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      uint8_t nat_type_mask = 0xF;
      uint8_t nat_miss_type = SWITCH_NAT_HIT_TYPE_SRC_NONE;
      status |= it->first.set_ternary(
          smi_id::F_SYSTEM_ACL_LOCAL_MD_NAT_HIT, nat_miss_type, nat_type_mask);
      uint8_t nat_same_zone_check = 0x1;
      uint8_t nat_same_zone_check_mask = 0x1;
      status |=
          it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_NAT_SAME_ZONE,
                                nat_same_zone_check,
                                nat_same_zone_check_mask);
      status |= setup_action_data(it);
    }
    return status;
  }

  switch_status_t create_hostif_trap_type_mpls_router_alert() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    /* system acl entry to trap MPLS Router alert packets
       When the top label is 1, mpls_router_alert_label flag
       will be set & mpls_pkt flag is not set. Since there will
       not be any mpls fib lookup, routed flag will not be set
    */

    if (feature::is_feature_set(SWITCH_FEATURE_MPLS)) {
      it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
      status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_HDR_MPLS_0_VALID,
                                      static_cast<uint8_t>(1),
                                      static_cast<uint8_t>(1));
      status |=
          it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_MPLS_ROUTER_ALERT,
                                static_cast<uint8_t>(1),
                                static_cast<uint8_t>(1));
      status |= setup_action_data(it);
    }
    return status;
  }

  switch_status_t create_hostif_trap_type_mpls_ttl_error() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    /* system acl entry to trap MPLS ttl 0 or 1.
       Not matching routed flag, as trap must happen
       irrespective of mpls fib lookup
    */

    if (feature::is_feature_set(SWITCH_FEATURE_MPLS)) {
      for (uint8_t ttl : {0, 1}) {
        it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
        status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_HDR_MPLS_0_VALID,
                                        static_cast<uint8_t>(1),
                                        static_cast<uint8_t>(1));
        status |= it->first.set_ternary(smi_id::F_SYSTEM_ACL_HDR_MPLS_0_TTL,
                                        static_cast<uint8_t>(ttl),
                                        static_cast<uint8_t>(0xFF));
        status |= setup_action_data(it);
      }
    }

    return status;
  }
};

#define EG_SYSTEM_ACL_ENTRY                   \
  std::pair<_MatchKey, _ActionEntry>(         \
      _MatchKey(smi_id::T_EGRESS_SYSTEM_ACL), \
      _ActionEntry(smi_id::T_EGRESS_SYSTEM_ACL))
class egress_hostif_trap_acl : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_EGRESS_HOSTIF_TRAP_ACL;
  static const switch_attr_id_t status_attr_id =
      SWITCH_EGRESS_HOSTIF_TRAP_ACL_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_EGRESS_HOSTIF_TRAP_ACL_ATTR_PARENT_HANDLE;
  switch_object_id_t meter_handle = {};
  switch_enum_t packet_action;
  switch_hostif_trap_attr_type trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_NONE;
  uint16_t cpu_redirect_reason_code;
  uint32_t acl_priority{};
  uint8_t qid = 0;
  std::vector<std::pair<_MatchKey, _ActionEntry>>::iterator it;

 public:
  egress_hostif_trap_acl(const switch_object_id_t parent,
                         switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_EGRESS_SYSTEM_ACL,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    it = match_action_list.begin();
    switch_hostif_trap_attr_packet_action trap_pkt_action;
    uint32_t trap_priority = 0;
    switch_enum_t enum_trap_type, enum_pkt_action_type;
    bool egress_trap = true;
    switch_object_id_t trap_group_handle = {};
    switch_status_t (egress_hostif_trap_acl::*create_hostif_trap_fn)();

    status |= switch_store::v_get(
        parent, SWITCH_HOSTIF_TRAP_ATTR_TYPE, enum_trap_type);
    trap_type =
        static_cast<switch_hostif_trap_attr_type>(enum_trap_type.enumdata);
    status |= switch_store::v_get(
        parent, SWITCH_HOSTIF_TRAP_ATTR_PACKET_ACTION, enum_pkt_action_type);
    trap_pkt_action = static_cast<switch_hostif_trap_attr_packet_action>(
        enum_pkt_action_type.enumdata);
    status |=
        hostif_trap_packet_action_to_acl_action(trap_pkt_action, packet_action);
    status |= switch_store::v_get(
        parent, SWITCH_HOSTIF_TRAP_ATTR_PRIORITY, trap_priority);
    status |=
        switch_store::v_get(parent,
                            SWITCH_HOSTIF_TRAP_ATTR_HOSTIF_TRAP_GROUP_HANDLE,
                            trap_group_handle);

    if (!trap_priority) {
      acl_priority = system_acl_priority(SYSTEM_ACL_TYPE_HOSTIF_TRAP);
    } else {
      acl_priority = trap_priority;
    }

    cpu_redirect_reason_code = trap_type;
    get_qid(trap_group_handle, meter_handle, qid);

    switch (trap_type) {
      case SWITCH_HOSTIF_TRAP_ATTR_TYPE_L3_MTU_ERROR:
        create_hostif_trap_fn =
            &egress_hostif_trap_acl::create_hostif_trap_type_l3_mtu_error;
        break;
      default:
        egress_trap = false;
        break;
    }

    if (!egress_trap) return;

    if (packet_action.enumdata ==
            SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_COPY_TO_CPU ||
        packet_action.enumdata ==
            SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_REDIRECT_TO_CPU) {
      if (!feature::is_feature_set(SWITCH_FEATURE_EGRESS_COPP)) {
        status |= SWITCH_STATUS_NOT_SUPPORTED;
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_HOSTIF_TRAP,
                   "Hostif trap {} action: {} not supported at egress",
                   trap_type,
                   packet_action.enumdata);
        return;
      }
    }

    status |= (this->*create_hostif_trap_fn)();
  }

  switch_status_t setup_action_data(
      std::vector<std::pair<_MatchKey, _ActionEntry>>::iterator iter) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t eg_target_type = {
        .enumdata = SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_COPP};

    status |= iter->first.set_exact(smi_id::F_EGRESS_SYSTEM_ACL_PRIORITY,
                                    acl_priority);
    status |= iter->first.set_ternary(
        smi_id::F_EGRESS_SYSTEM_ACL_LOCAL_MD_FLAGS_ACL_DENY,
        static_cast<uint8_t>(0),
        static_cast<uint8_t>(1));

    switch (packet_action.enumdata) {
      case SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_PERMIT:
        iter->second.init_action_data(smi_id::A_NO_ACTION);
        break;
      case SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_DROP:
        iter->second.init_action_data(smi_id::A_EGRESS_SYSTEM_ACL_DROP);
        status |=
            iter->second.set_arg(smi_id::P_EGRESS_SYSTEM_ACL_DROP_REASON_CODE,
                                 cpu_redirect_reason_code);
        break;
      case SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_COPY_TO_CPU:
        iter->second.init_action_data(smi_id::A_EGRESS_SYSTEM_ACL_COPY_TO_CPU);
        status |= iter->second.set_arg(
            smi_id::P_EGRESS_SYSTEM_ACL_COPY_TO_CPU_REASON_CODE,
            cpu_redirect_reason_code);
        status |= iter->second.set_arg(
            smi_id::P_EGRESS_SYSTEM_ACL_COPY_TO_CPU_METER_ID, meter_handle);
        if (meter_handle.data != 0)
          status |= switch_store::v_set(
              meter_handle, SWITCH_METER_ATTR_TARGET_TYPE, eg_target_type);
        break;
      case SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_REDIRECT_TO_CPU:
        iter->second.init_action_data(
            smi_id::A_EGRESS_SYSTEM_ACL_REDIRECT_TO_CPU);
        status |= iter->second.set_arg(
            smi_id::P_EGRESS_SYSTEM_ACL_REDIRECT_TO_CPU_REASON_CODE,
            cpu_redirect_reason_code);
        status |= iter->second.set_arg(
            smi_id::P_EGRESS_SYSTEM_ACL_REDIRECT_TO_CPU_METER_ID, meter_handle);
        if (meter_handle.data != 0)
          status |= switch_store::v_set(
              meter_handle, SWITCH_METER_ATTR_TARGET_TYPE, eg_target_type);
        break;
      default:
        break;
    }

    return SWITCH_STATUS_SUCCESS;
  }

  switch_status_t create_hostif_trap_type_l3_mtu_error() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint16_t l3_mtu = 0, l3_mtu_mask = 0xFFFF;

    it = match_action_list.insert(it, EG_SYSTEM_ACL_ENTRY);
    status |= it->first.set_ternary(
        smi_id::F_EGRESS_SYSTEM_ACL_LOCAL_MD_CHECKS_MTU, l3_mtu, l3_mtu_mask);
    status |= setup_action_data(it);

    return status;
  }
};

/*
 * copy-to-cpu action different from redirect action. In user ACLs, trap sets
 * acl deny. copy does not set deny.
 * We will need an additional PHV to differentiate these 2 actions in user acls
 * and pass it on to system ACL. For now, this is a workaround without
 * introducing a new PHV
 */
class hostif_user_defined_trap_acl : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_HOSTIF_USER_DEFINED_TRAP_ACL;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_HOSTIF_USER_DEFINED_TRAP_ACL_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_HOSTIF_USER_DEFINED_TRAP_ACL_ATTR_STATUS;
  uint16_t cpu_redirect_reason_code = SWITCH_UDT_REASON_CODE;
  uint8_t qid = 0;
  switch_object_id_t meter_handle = {};

  switch_status_t setup_acl_action(
      std::vector<std::pair<_MatchKey, _ActionEntry>>::iterator it,
      uint64_t action) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch (action) {
      case SWITCH_PACKET_ACTION_TRAP:
        it->second.init_action_data(smi_id::A_SYSTEM_ACL_REDIRECT_TO_CPU);
        status |=
            it->second.set_arg(smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_REASON_CODE,
                               cpu_redirect_reason_code);
        status |=
            it->second.set_arg(smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_QID, qid);
        status |= it->second.set_arg(
            smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_METER_ID, meter_handle);
        status |= it->second.set_arg(
            smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_OVERWRITE_QID, true);
        break;
      case SWITCH_PACKET_ACTION_COPY:
        it->second.init_action_data(smi_id::A_SYSTEM_ACL_COPY_TO_CPU);
        status |=
            it->second.set_arg(smi_id::P_SYSTEM_ACL_COPY_TO_CPU_REASON_CODE,
                               cpu_redirect_reason_code);
        status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_COPY_TO_CPU_QID, qid);
        status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_COPY_TO_CPU_METER_ID,
                                     meter_handle);
        status |= it->second.set_arg(
            smi_id::P_SYSTEM_ACL_COPY_TO_CPU_OVERWRITE_QID, true);
        break;
      case SWITCH_PACKET_ACTION_DROP:
        it->second.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
        status |= it->second.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                                     SWITCH_DROP_REASON_INGRESS_ACL_METER);
        break;
      default:
        status = SWITCH_STATUS_NOT_SUPPORTED;
        return status;
    }
    return status;
  }

 public:
  hostif_user_defined_trap_acl(const switch_object_id_t parent,
                               switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_INGRESS_SYSTEM_ACL,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    switch_object_id_t cpu_queue_handle = {}, hostif_trap_group_handle = {};
    std::vector<std::pair<_MatchKey, _ActionEntry>>::iterator it;
    uint32_t priority = 0;
    switch_enum_t udt_type = {SWITCH_HOSTIF_USER_DEFINED_TRAP_ATTR_TYPE_ACL};

    status |= switch_store::v_get(
        parent,
        SWITCH_HOSTIF_USER_DEFINED_TRAP_ATTR_HOSTIF_TRAP_GROUP_HANDLE,
        hostif_trap_group_handle);

    status |= switch_store::v_get(
        parent, SWITCH_HOSTIF_USER_DEFINED_TRAP_ATTR_PRIORITY, priority);

    status |= switch_store::v_get(
        parent, SWITCH_HOSTIF_USER_DEFINED_TRAP_ATTR_TYPE, udt_type);

    if (hostif_trap_group_handle.data) {
      switch_store::v_get(hostif_trap_group_handle,
                          SWITCH_HOSTIF_TRAP_GROUP_ATTR_QUEUE_HANDLE,
                          cpu_queue_handle);
      switch_store::v_get(hostif_trap_group_handle,
                          SWITCH_HOSTIF_TRAP_GROUP_ATTR_POLICER_HANDLE,
                          meter_handle);
    }
    if (cpu_queue_handle.data)
      switch_store::v_get(cpu_queue_handle, SWITCH_QUEUE_ATTR_QUEUE_ID, qid);

    cpu_redirect_reason_code += switch_store::handle_to_id(parent);

    if (udt_type.enumdata ==
            SWITCH_HOSTIF_USER_DEFINED_TRAP_ATTR_TYPE_NEIGHBOR ||
        feature::is_feature_set(SWITCH_FEATURE_INGRESS_ACL_METER)) {
      // redirect to cpu, meter packet action trap
      // copy to cpu, meter packet action copy
      // drop, meter packet action drop
      std::vector<uint8_t> actions = {SWITCH_PACKET_ACTION_TRAP,
                                      SWITCH_PACKET_ACTION_COPY,
                                      SWITCH_PACKET_ACTION_DROP};
      for (auto action : actions) {
        it = match_action_list.insert(it, SYSTEM_ACL_ENTRY);
        status |=
            it->first.set_ternary(smi_id::F_SYSTEM_ACL_LOCAL_MD_HOSTIF_TRAP_ID,
                                  static_cast<uint8_t>(parent.data),
                                  static_cast<uint8_t>(0xFF));
        status |= it->first.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, priority);
        if (udt_type.enumdata ==
            SWITCH_HOSTIF_USER_DEFINED_TRAP_ATTR_TYPE_ACL) {
          status |= it->first.set_ternary(
              smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_METER_PACKET_ACTION,
              action,
              static_cast<uint8_t>(0x3));
        }
        status |= setup_acl_action(it, action);
        priority++;
      }
    }

    if (meter_handle.data) {
      switch_enum_t target_type = {
          .enumdata = SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_COPP};
      status |= switch_store::v_set(
          meter_handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    }
  }
};

class hostif_trap_group_helper : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_HOSTIF_TRAP_GROUP_HELPER;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_HOSTIF_TRAP_GROUP_HELPER_ATTR_PARENT_HANDLE;

 public:
  hostif_trap_group_helper(const switch_object_id_t parent,
                           switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    // Get all traps associated with the hostif trap group
    std::vector<switch_object_id_t> hostif_traps;

    status |= switch_store::object_get_all_handles(
        SWITCH_OBJECT_TYPE_HOSTIF_TRAP, hostif_traps);

    for (auto trap : hostif_traps) {
      switch_status_t obj_status = SWITCH_STATUS_SUCCESS;
      switch_object_id_t device = {}, trap_grp = {}, dflt_trap_grp = {};
      obj_status |=
          switch_store::v_get(trap, SWITCH_HOSTIF_TRAP_ATTR_DEVICE, device);
      obj_status |= switch_store::v_get(
          trap, SWITCH_HOSTIF_TRAP_ATTR_HOSTIF_TRAP_GROUP_HANDLE, trap_grp);
      obj_status |= switch_store::v_get(
          device, SWITCH_DEVICE_ATTR_DEFAULT_HOSTIF_TRAP_GROUP, dflt_trap_grp);

      if (obj_status != SWITCH_STATUS_SUCCESS) {
        status |= obj_status;
        continue;
      }

      if (parent == trap_grp || parent == dflt_trap_grp) {
        obj_status = SWITCH_STATUS_SUCCESS;
        hostif_trap_acl itrap_acl(trap, obj_status);
        if (obj_status == SWITCH_STATUS_SUCCESS) {
          itrap_acl.create_update();
        }
        status |= obj_status;

        obj_status = SWITCH_STATUS_SUCCESS;
        egress_hostif_trap_acl etrap_acl(trap, obj_status);
        if (obj_status == SWITCH_STATUS_SUCCESS) {
          etrap_acl.create_update();
        }
        status |= obj_status;
      }
    }
  }
};

switch_status_t before_hostif_trap_update(const switch_object_id_t object_id,
                                          const attr_w &attr) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  auto attr_id = attr.id_get();
  if (attr_id == SWITCH_HOSTIF_TRAP_ATTR_PRIORITY) {
    // Since priority is an entry key we can't just update it but have to remove
    // the entry with old value and then add the new one after update
    hostif_trap_acl itrap_acl(object_id, status);
    itrap_acl.del();
    egress_hostif_trap_acl etrap_acl(object_id, status);
    etrap_acl.del();
  } else if (attr_id == SWITCH_HOSTIF_TRAP_ATTR_EXCLUDE_PORT_LIST) {
    std::vector<switch_object_id_t> old_exclusion_list;
    status |= switch_store::v_get(object_id,
                                  SWITCH_HOSTIF_TRAP_ATTR_EXCLUDE_PORT_LIST,
                                  old_exclusion_list);
    std::vector<switch_object_id_t> new_exclusion_list;
    status |= attr.v_get(new_exclusion_list);
    uint32_t exclusion_id{};
    status |= switch_store::v_get(
        object_id, SWITCH_HOSTIF_TRAP_ATTR_EXCLUDE_PORT_LIST_ID, exclusion_id);
    CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);
    // Since exclusion index is an entry key we can't update it. Hence we
    // remove the old entry and install a new one
    hostif_trap_acl itrap_acl(object_id, status);
    status = itrap_acl.del();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_HOSTIF_TRAP,
                 "Failed to update Hostif trap {:#x} with exclusion list {}. "
                 "Failed to delete old trap entry with Port"
                 "Exclusion List {}, status: {}",
                 object_id,
                 new_exclusion_list,
                 old_exclusion_list,
                 status);
      return status;
    }
    if (!old_exclusion_list.empty()) {
      std::set<switch_object_id_t> release_list(old_exclusion_list.begin(),
                                                old_exclusion_list.end());
      status = ExclusionListManager::instance().port_exclusion_list_id_release(
          release_list);
      CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);
      uint32_t refcount{1};
      status = ExclusionListManager::instance().port_exclusion_list_refcount(
          exclusion_id, refcount);
      CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);
      if (!refcount) {
        uint32_t exclusion_port_index =
            SWITCH_HOSTIF_TRAP_EXCLUSION_PORT_LIST_VALUE(exclusion_id);
        for (auto &port : old_exclusion_list) {
          uint32_t port_exclusion_bit_map{};
          status |= switch_store::v_get(port,
                                        SWITCH_PORT_ATTR_PORT_EXCLUSION_BIT_MAP,
                                        port_exclusion_bit_map);
          if ((port_exclusion_bit_map & exclusion_port_index)) {
            status |= switch_store::v_set(
                port,
                SWITCH_PORT_ATTR_PORT_EXCLUSION_BIT_MAP,
                port_exclusion_bit_map & ~(exclusion_port_index));
          }
        }
      }
    }
    // We dont support exclusion list on egress hostif traps, so need no
    // evaluate egress hostif trap entry
  }
  return status;
}

switch_status_t after_hostif_trap_update(const switch_object_id_t object_id,
                                         const attr_w &attr) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  auto attr_id = attr.id_get();
  if (attr_id == SWITCH_HOSTIF_TRAP_ATTR_PRIORITY) {
    // Add entries with updated priority
    hostif_trap_acl itrap_acl(object_id, status);
    itrap_acl.create_update();
    egress_hostif_trap_acl etrap_acl(object_id, status);
    etrap_acl.create_update();
  } else if (attr_id == SWITCH_HOSTIF_TRAP_ATTR_EXCLUDE_PORT_LIST) {
    std::vector<switch_object_id_t> new_exclusion_list;
    status |= attr.v_get(new_exclusion_list);
    CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);
    uint8_t exclusion_id{};
    std::set<switch_object_id_t> new_exclusion_set(new_exclusion_list.begin(),
                                                   new_exclusion_list.end());
    if (!new_exclusion_list.empty()) {
      status |=
          ExclusionListManager::instance().port_exclusion_list_id_allocate(
              new_exclusion_set, exclusion_id);
    }
    status |= switch_store::v_set(object_id,
                                  SWITCH_HOSTIF_TRAP_ATTR_EXCLUDE_PORT_LIST_ID,
                                  static_cast<uint32_t>(exclusion_id));
    // Add entries with updated exclusion list index
    hostif_trap_acl itrap_acl(object_id, status);
    status |= itrap_acl.create_update();
    if (!new_exclusion_list.empty()) {
      uint32_t refcount{};
      status = ExclusionListManager::instance().port_exclusion_list_refcount(
          exclusion_id, refcount);
      CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);
      // Update Port lag label with exclusion List bmap if this is the first
      // association of the exclusion list
      if (refcount == 1) {
        uint32_t exclusion_port_index =
            SWITCH_HOSTIF_TRAP_EXCLUSION_PORT_LIST_VALUE(exclusion_id);
        for (auto &port : new_exclusion_list) {
          uint32_t port_exclusion_bit_map{};
          status |= switch_store::v_get(port,
                                        SWITCH_PORT_ATTR_PORT_EXCLUSION_BIT_MAP,
                                        port_exclusion_bit_map);
          status |= switch_store::v_set(
              port,
              SWITCH_PORT_ATTR_PORT_EXCLUSION_BIT_MAP,
              port_exclusion_bit_map | exclusion_port_index);
        }
      }
    }
    // We dont support exclusion list on egress hostif traps, so need no
    // evaluate egress hostif trap entry
  }

  return status;
}

switch_status_t before_hostif_trap_create(
    const switch_object_type_t object_type, std::set<attr_w> &attrs) {
  (void)object_type;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::vector<switch_object_id_t> port_handles;
  uint8_t exclusion_id{};
  auto it = attrs.find(SWITCH_HOSTIF_TRAP_ATTR_EXCLUDE_PORT_LIST);
  if (it == attrs.end()) return status;
  status = it->v_get(port_handles);
  if (status != SWITCH_STATUS_SUCCESS || port_handles.empty()) return status;
  std::set<switch_object_id_t> exclusion_port_handles(port_handles.begin(),
                                                      port_handles.end());
  it = attrs.find(SWITCH_HOSTIF_TRAP_ATTR_EXCLUDE_PORT_LIST_ID);
  if (switch_store::smiContext::context().in_warm_init()) {
    if (it == attrs.end()) return status;
    status = it->v_get(exclusion_id);
    CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);
    status = ExclusionListManager::instance().port_exclusion_list_id_reserve(
        exclusion_port_handles, exclusion_id);
    return status;
  }
  status = ExclusionListManager::instance().port_exclusion_list_id_allocate(
      exclusion_port_handles, exclusion_id);
  CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);
  if (it != attrs.end()) {
    attrs.erase(it);
  }
  attr_w hostif_trap_attr_exclusion_id(
      SWITCH_HOSTIF_TRAP_ATTR_EXCLUDE_PORT_LIST_ID);
  hostif_trap_attr_exclusion_id.v_set(static_cast<uint32_t>(exclusion_id));
  attrs.insert(hostif_trap_attr_exclusion_id);
  return status;
}

switch_status_t after_hostif_trap_delete(const switch_object_type_t object_type,
                                         const std::set<attr_w> &attrs) {
  (void)object_type;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  uint32_t exclusion_id{};
  auto it = attrs.find(SWITCH_HOSTIF_TRAP_ATTR_EXCLUDE_PORT_LIST_ID);
  if (it == attrs.end()) return status;
  status = it->v_get(exclusion_id);
  CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);
  if (exclusion_id) {
    status = ExclusionListManager::instance().port_exclusion_list_id_release(
        static_cast<uint8_t>(exclusion_id));
  }
  return status;
}

switch_status_t after_hostif_trap_create(const switch_object_id_t object_id,
                                         const std::set<attr_w> &attrs) {
  (void)attrs;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  if (switch_store::smiContext::context().in_warm_init()) return status;
  std::vector<switch_object_id_t> port_handles;
  status = switch_store::v_get(
      object_id, SWITCH_HOSTIF_TRAP_ATTR_EXCLUDE_PORT_LIST, port_handles);
  if (port_handles.empty()) return status;
  uint32_t exclusion_port_id{};
  status = switch_store::v_get(object_id,
                               SWITCH_HOSTIF_TRAP_ATTR_EXCLUDE_PORT_LIST_ID,
                               exclusion_port_id);
  if (exclusion_port_id) {
    uint32_t exclusion_port_index =
        SWITCH_HOSTIF_TRAP_EXCLUSION_PORT_LIST_VALUE(exclusion_port_id);
    uint32_t refcount{};
    status = ExclusionListManager::instance().port_exclusion_list_refcount(
        exclusion_port_id, refcount);
    CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);
    if (refcount == 1) {
      for (auto &port : port_handles) {
        uint32_t port_exclusion_bit_map{};
        status |= switch_store::v_get(port,
                                      SWITCH_PORT_ATTR_PORT_EXCLUSION_BIT_MAP,
                                      port_exclusion_bit_map);
        status |=
            switch_store::v_set(port,
                                SWITCH_PORT_ATTR_PORT_EXCLUSION_BIT_MAP,
                                port_exclusion_bit_map | exclusion_port_index);
      }
    }
  }
  return status;
}

switch_status_t before_hostif_trap_delete(const switch_object_id_t object_id) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::vector<switch_object_id_t> port_handles;
  status = switch_store::v_get(
      object_id, SWITCH_HOSTIF_TRAP_ATTR_EXCLUDE_PORT_LIST, port_handles);
  if (port_handles.empty()) return status;
  uint32_t exclusion_port_id{};
  status = switch_store::v_get(object_id,
                               SWITCH_HOSTIF_TRAP_ATTR_EXCLUDE_PORT_LIST_ID,
                               exclusion_port_id);
  if (exclusion_port_id) {
    uint32_t refcount{};
    status = ExclusionListManager::instance().port_exclusion_list_refcount(
        exclusion_port_id, refcount);
    CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);
    if (refcount == 1) {
      uint32_t exclusion_port_index =
          SWITCH_HOSTIF_TRAP_EXCLUSION_PORT_LIST_VALUE(exclusion_port_id);
      for (auto &port : port_handles) {
        uint32_t port_exclusion_bit_map{};
        status |= switch_store::v_get(port,
                                      SWITCH_PORT_ATTR_PORT_EXCLUSION_BIT_MAP,
                                      port_exclusion_bit_map);
        status |= switch_store::v_set(
            port,
            SWITCH_PORT_ATTR_PORT_EXCLUSION_BIT_MAP,
            port_exclusion_bit_map & ~(exclusion_port_index));
      }
    }
  }
  return status;
}

switch_status_t hostif_trap_init() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  REGISTER_OBJECT(hostif_trap_group_helper,
                  SWITCH_OBJECT_TYPE_HOSTIF_TRAP_GROUP_HELPER);
  REGISTER_OBJECT(hostif_trap_acl, SWITCH_OBJECT_TYPE_HOSTIF_TRAP_ACL);
  REGISTER_OBJECT(egress_hostif_trap_acl,
                  SWITCH_OBJECT_TYPE_EGRESS_HOSTIF_TRAP_ACL);

  status |= switch_store::reg_update_trigs_before(
      SWITCH_OBJECT_TYPE_HOSTIF_TRAP, &before_hostif_trap_update);
  status |= switch_store::reg_update_trigs_after(SWITCH_OBJECT_TYPE_HOSTIF_TRAP,
                                                 &after_hostif_trap_update);
  // Currently we enable hostif trap port exclusion only on X1 profile. Below
  // feature check is to figure out whether the currently run profile ix X1 or
  // not
  if (feature::is_feature_set(SWITCH_FEATURE_DROP_REPORT)) {
    status |= switch_store::reg_create_trigs_before(
        SWITCH_OBJECT_TYPE_HOSTIF_TRAP, &before_hostif_trap_create);
    status |= switch_store::reg_create_trigs_after(
        SWITCH_OBJECT_TYPE_HOSTIF_TRAP, &after_hostif_trap_create);
    status |= switch_store::reg_delete_trigs_before(
        SWITCH_OBJECT_TYPE_HOSTIF_TRAP, &before_hostif_trap_delete);
    status |= switch_store::reg_delete_trigs_after(
        SWITCH_OBJECT_TYPE_HOSTIF_TRAP, &after_hostif_trap_delete);
  }

  REGISTER_OBJECT(hostif_user_defined_trap_acl,
                  SWITCH_OBJECT_TYPE_HOSTIF_USER_DEFINED_TRAP_ACL);
  return status;
}

switch_status_t hostif_trap_clean() { return SWITCH_STATUS_SUCCESS; }
} /* namespace smi */
