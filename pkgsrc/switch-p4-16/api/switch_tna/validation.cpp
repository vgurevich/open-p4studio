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


#include <utility>
#include <vector>

#include "switch_tna/utils.h"
#include "switch_tna/p4_16_types.h"

namespace smi {
using namespace smi::bf_rt;  // NOLINT(build/namespaces)

class validate_ethernet : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_VALIDATE_ETHERNET;
  static const switch_attr_id_t status_attr_id =
      SWITCH_VALIDATE_ETHERNET_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_VALIDATE_ETHERNET_ATTR_PARENT_HANDLE;

 public:
  validate_ethernet(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_VALIDATE_ETHERNET,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    struct rule_spec {
      const char *srcAddr;
      const char *srcAddr_mask;
      const char *dstAddr;
      const char *dstAddr_mask;
      uint8_t vlan_tag__0__valid;
      uint8_t vlan_tag__0__valid_mask;
      uint8_t vlan_tag__1__valid;
      uint8_t vlan_tag__1__valid_mask;
      bf_rt_action_id_t action_id;
      uint8_t drop_reason;
      uint8_t pkt_type;
    };

    // clang-format off
    const std::vector<rule_spec> rules = {
        // malformed packet, mac sa is zeros
        {eth_0s, eth_1s, eth_0s, eth_0s, 0, 0, 0, 0, smi_id::A_MALFORMED_ETH_PKT, SWITCH_DROP_REASON_OUTER_SRC_MAC_ZERO, 0},
        // malformed packet, mac sa is multicast
        {eth_mc, eth_mc, eth_0s, eth_0s, 0, 0, 0, 0, smi_id::A_MALFORMED_ETH_PKT, SWITCH_DROP_REASON_OUTER_SRC_MAC_MULTICAST, 0},
        // malformed packet, mac da is zeros
        {eth_0s, eth_0s, eth_0s, eth_1s, 0, 0, 0, 0, smi_id::A_MALFORMED_ETH_PKT, SWITCH_DROP_REASON_OUTER_DST_MAC_ZERO, 0},
        // malformed packet, mac da is reserved MAC
        {eth_0s, eth_0s, reserved_mac, reserved_mac_mask, 0, 0, 0, 0, smi_id::A_MALFORMED_ETH_PKT, SWITCH_DROP_REASON_DMAC_RESERVED, 0},


        // valid ethernet double tagged broadcast packet
        {eth_0s, eth_0s, eth_1s, eth_1s, 1, 1, 1, 1, smi_id::A_VALID_PKT_DOUBLE_TAGGED, 0, SWITCH_PKT_TYPE_BROADCAST},
        // valid ethernet tagged broadcast packet
        {eth_0s, eth_0s, eth_1s, eth_1s, 1, 1, 0, 0, smi_id::A_VALID_PKT_TAGGED, 0, SWITCH_PKT_TYPE_BROADCAST},
        // valid ethernet untagged broadcast packet
        {eth_0s, eth_0s, eth_1s, eth_1s, 0, 1, 0, 0, smi_id::A_VALID_PKT_UNTAGGED, 0, SWITCH_PKT_TYPE_BROADCAST},
        // valid ethernet double tagged multicast packet
        {eth_0s, eth_0s, eth_mc, eth_mc, 1, 1, 1, 1, smi_id::A_VALID_PKT_DOUBLE_TAGGED, 0, SWITCH_PKT_TYPE_MULTICAST},
        // valid ethernet tagged multicast packet
        {eth_0s, eth_0s, eth_mc, eth_mc, 1, 1, 0, 0, smi_id::A_VALID_PKT_TAGGED, 0, SWITCH_PKT_TYPE_MULTICAST},
        // valid ethernet untagged multicast packet
        {eth_0s, eth_0s, eth_mc, eth_mc, 0, 1, 0, 0, smi_id::A_VALID_PKT_UNTAGGED, 0, SWITCH_PKT_TYPE_MULTICAST},
        // valid ethernet double tagged unicast packet
        {eth_0s, eth_0s, eth_0s, eth_0s, 1, 1, 1, 1, smi_id::A_VALID_PKT_DOUBLE_TAGGED, 0, SWITCH_PKT_TYPE_UNICAST},
        // valid ethernet tagged unicast packet
        {eth_0s, eth_0s, eth_0s, eth_0s, 1, 1, 0, 0, smi_id::A_VALID_PKT_TAGGED, 0, SWITCH_PKT_TYPE_UNICAST},
        // valid ethernet untagged unicast packet
        {eth_0s, eth_0s, eth_0s, eth_0s, 0, 1, 0, 0, smi_id::A_VALID_PKT_UNTAGGED, 0, SWITCH_PKT_TYPE_UNICAST},
    };
    // clang-format on

    auto it = match_action_list.begin();
    for (uint32_t i = 0; i < rules.size(); i++) {
      if (rules[i].action_id == smi_id::A_VALID_PKT_DOUBLE_TAGGED) {
        if (!feature::is_feature_set(SWITCH_FEATURE_QINQ_RIF)) continue;
      }

      it = match_action_list.insert(
          it,
          std::pair<_MatchKey, _ActionEntry>(
              _MatchKey(smi_id::T_VALIDATE_ETHERNET),
              _ActionEntry(smi_id::T_VALIDATE_ETHERNET)));
      status |= it->first.set_exact(smi_id::F_VALIDATE_ETHERNET_PRIORITY,
                                    static_cast<uint32_t>(i));
      status |=
          it->first.set_ternary(smi_id::F_VALIDATE_ETHERNET_ETHERNET_DST_ADDR,
                                rules[i].dstAddr,
                                rules[i].dstAddr_mask,
                                ETH_LEN);
      status |=
          it->first.set_ternary(smi_id::F_VALIDATE_ETHERNET_ETHERNET_SRC_ADDR,
                                rules[i].srcAddr,
                                rules[i].srcAddr_mask,
                                ETH_LEN);
      status |= it->first.set_ternary<uint8_t>(
          smi_id::F_VALIDATE_ETHERNET_VLAN_0_VALID,
          rules[i].vlan_tag__0__valid,
          rules[i].vlan_tag__0__valid_mask);
      status |= it->first.set_ternary<uint8_t>(
          smi_id::F_VALIDATE_ETHERNET_VLAN_1_VALID,
          rules[i].vlan_tag__1__valid,
          rules[i].vlan_tag__1__valid_mask);

      it->second.init_action_data(rules[i].action_id);
      if (rules[i].action_id == smi_id::A_MALFORMED_ETH_PKT) {
        status |= it->second.set_arg(smi_id::P_MALFORMED_ETH_PKT_REASON,
                                     rules[i].drop_reason);
      } else if (rules[i].action_id == smi_id::A_VALID_PKT_UNTAGGED) {
        status |= it->second.set_arg(smi_id::P_VALID_PKT_UNTAGGED_PKT_TYPE,
                                     rules[i].pkt_type);
      } else if (rules[i].action_id == smi_id::A_VALID_PKT_TAGGED) {
        status |= it->second.set_arg(smi_id::P_VALID_PKT_TAGGED_PKT_TYPE,
                                     rules[i].pkt_type);
      } else if (rules[i].action_id == smi_id::A_VALID_PKT_DOUBLE_TAGGED) {
        status |= it->second.set_arg(smi_id::P_VALID_PKT_DOUBLE_TAGGED_PKT_TYPE,
                                     rules[i].pkt_type);
      }
    }
  }
};

class validate_ip : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_VALIDATE_IP;
  static const switch_attr_id_t status_attr_id = SWITCH_VALIDATE_IP_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_VALIDATE_IP_ATTR_PARENT_HANDLE;

 public:
  validate_ip(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_VALIDATE_IP,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    struct rule_spec {
      uint8_t arp_valid;
      uint8_t arp_valid_mask;
      uint8_t ipv4_valid;
      uint8_t ipv4_valid_mask;
      uint8_t ipv4_version;
      uint8_t ipv4_version_mask;
      uint8_t ipv4_chksum;
      uint8_t ipv4_chksum_mask;
      uint8_t ipv4_ihl;
      uint8_t ipv4_ihl_mask;
      uint8_t ipv4_ttl;
      uint8_t ipv4_ttl_mask;
      uint8_t ipv4_flags;
      uint8_t ipv4_flags_mask;
      uint16_t ipv4_frag_offset;
      uint16_t ipv4_frag_offset_mask;
      uint32_t ipv4_srcAddr;
      uint32_t ipv4_srcAddr_mask;
      uint8_t ipv6_valid;
      uint8_t ipv6_valid_mask;
      uint8_t ipv6_version;
      uint8_t ipv6_version_mask;
      uint8_t ipv6_hopLimit;
      uint8_t ipv6_hopLimit_mask;
      const char *ipv6_srcAddr;  // only 8 MSB bits
      const char *ipv6_srcAddr_mask;
      bf_rt_action_id_t action_id;
      uint8_t drop_reason;
      uint8_t link_local;
      uint8_t frag;
      uint8_t mpls_0_valid;
      uint8_t mpls_0_valid_mask;
      uint32_t mpls_0_label;
      bool mpls_0_label_mask;
      uint8_t mpls_1_valid;
      uint8_t mpls_1_valid_mask;
      uint8_t inner_ipv4_valid;
      uint8_t inner_ipv4_valid_mask;
      uint8_t inner_ipv6_valid;
      uint8_t inner_ipv6_valid_mask;
    };

    (void)status;
    // clang-format off
    std::vector<rule_spec> rules = {
        // IPv4 rules
        // ipv4 src is loopback
        {0, 0, 1, 1, 0x4, 0xf, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x7f000000, 0xff000000,                        0, 0, 0, 0, 0, 0, ipv6_0s, ipv6_0s, smi_id::A_MALFORMED_IPV4_PKT, SWITCH_DROP_REASON_OUTER_IP_SRC_LOOPBACK, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0},
        // ipv4 src is multicast
        {0, 0, 1, 1, 0x4, 0xf, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xe0000000, 0xf0000000,                        0, 0, 0, 0, 0, 0, ipv6_0s, ipv6_0s, smi_id::A_MALFORMED_IPV4_PKT, SWITCH_DROP_REASON_OUTER_IP_SRC_MULTICAST, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0},
        // ttl is zero
        {0, 0, 1, 1, 0x4, 0xf, 0, 0, 0, 0, 0x00, 0xff, 0, 0, 0, 0, 0, 0,                                    0, 0, 0, 0, 0, 0, ipv6_0s, ipv6_0s, smi_id::A_MALFORMED_IPV4_PKT, SWITCH_DROP_REASON_OUTER_IP_TTL_ZERO, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0},
        // invalid ihl
        {0, 0, 1, 1, 0x4, 0xf, 0, 0, 0x00, 0x0c, 0, 0, 0, 0, 0, 0, 0, 0,                                    0, 0, 0, 0, 0, 0, ipv6_0s, ipv6_0s, smi_id::A_MALFORMED_IPV4_PKT, SWITCH_DROP_REASON_OUTER_IP_IHL_INVALID, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0},
        // invalid ihl
        {0, 0, 1, 1, 0x4, 0xf, 0, 0, 0x04, 0x0f, 0, 0, 0, 0, 0, 0, 0, 0,                                    0, 0, 0, 0, 0, 0, ipv6_0s, ipv6_0s, smi_id::A_MALFORMED_IPV4_PKT, SWITCH_DROP_REASON_OUTER_IP_IHL_INVALID, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0},
        // ipv4 src is broadcast
        {0, 0, 1, 1, 0x4, 0xf, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xffffffff, 0xffffffff,                        0, 0, 0, 0, 0, 0, ipv6_0s, ipv6_0s, smi_id::A_MALFORMED_IPV4_PKT, SWITCH_DROP_REASON_SIP_BC, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0},
        // ipv4 src is class e
        {0, 0, 1, 1, 0x4, 0xf, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xF0000000, 0xF0000000,                        0, 0, 0, 0, 0, 0, ipv6_0s, ipv6_0s, smi_id::A_MALFORMED_IPV4_PKT, SWITCH_DROP_REASON_OUTER_IP_SRC_CLASS_E, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0},
        // ipv4 src is unspecified
        {0, 0, 1, 1, 0x4, 0xf, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0, 0xffffffff,                               0, 0, 0, 0, 0, 0, ipv6_0s, ipv6_0s, smi_id::A_MALFORMED_IPV4_PKT, SWITCH_DROP_REASON_OUTER_IP_SRC_UNSPECIFIED, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0},
        // link_local ipv4, non-frag
        {0, 0, 1, 1, 0x4, 0xf, 0x0, 0x1, 0, 0, 0, 0, 0x0, 0x1, 0, 0x1fff, 0xA9FE0000, 0xFFFF0000,       0, 0, 0, 0, 0, 0, ipv6_0s, ipv6_0s, smi_id::A_VALID_IPV4_PKT, 0, 1, SWITCH_IP_FRAG_NON_FRAG, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0},
        // link_local ipv4, first frag
        {0, 0, 1, 1, 0x4, 0xf, 0x0, 0x1, 0, 0, 0, 0, 0x1, 0x1, 0, 0x1fff,0xA9FE0000, 0xFFFF0000,        0, 0, 0, 0, 0, 0, ipv6_0s, ipv6_0s, smi_id::A_VALID_IPV4_PKT, 0, 1, SWITCH_IP_FRAG_HEAD, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0},
        // link_local ipv4, more frags
        {0, 0, 1, 1, 0x4, 0xf, 0x0, 0x1, 0, 0, 0, 0, 0, 0, 0, 0, 0xA9FE0000, 0xFFFF0000,                0, 0, 0, 0, 0, 0, ipv6_0s, ipv6_0s, smi_id::A_VALID_IPV4_PKT, 0, 1, SWITCH_IP_FRAG_NON_HEAD, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0},
        // ipv4 non-frag
        {0, 0, 1, 1, 0x4, 0xf, 0x0, 0x1, 0, 0, 0, 0, 0x0, 0x1, 0, 0x1fff, 0, 0,                         0, 0, 0, 0, 0, 0, ipv6_0s, ipv6_0s, smi_id::A_VALID_IPV4_PKT, 0, 0, SWITCH_IP_FRAG_NON_FRAG, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0},
        // ipv4 first frag
        {0, 0, 1, 1, 0x4, 0xf, 0x0, 0x1, 0, 0, 0, 0, 0x1, 0x1, 0, 0x1fff, 0, 0,                         0, 0, 0, 0, 0, 0, ipv6_0s, ipv6_0s, smi_id::A_VALID_IPV4_PKT, 0, 0, SWITCH_IP_FRAG_HEAD, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0},
        // ipv4 more frags
        {0, 0, 1, 1, 0x4, 0xf, 0x0, 0x1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                                  0, 0, 0, 0, 0, 0, ipv6_0s, ipv6_0s, smi_id::A_VALID_IPV4_PKT, 0, 0, SWITCH_IP_FRAG_NON_HEAD, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0},
        // ipv4 invalid checksum
        {0, 0, 1, 1, 0x4, 0xf, 0x1, 0x1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                                  0, 0, 0, 0, 0, 0, ipv6_0s, ipv6_0s, smi_id::A_MALFORMED_IPV4_PKT, SWITCH_DROP_REASON_OUTER_IP_INVALID_CHECKSUM, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0},
        // ipv4 invalid version
        // ipv4.valid=True, ip_version!=4,
        // this this rule should be after all IpV4 rules
        {0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                                          0, 0, 0, 0, 0, 0, ipv6_0s, ipv6_0s, smi_id::A_MALFORMED_IPV4_PKT, SWITCH_DROP_REASON_OUTER_IP_VERSION_INVALID, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0},

        // IPv6 rules
        // ipv6 src is unspecified
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                                          1, 1, 0x6, 0xf, 0, 0, ipv6_0s, ipv6_1s, smi_id::A_MALFORMED_IPV6_PKT, SWITCH_DROP_REASON_OUTER_IP_SRC_UNSPECIFIED, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0},
        // ipv6 src is multicast
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                                          1, 1, 0x6, 0xf, 0, 0, ipv6_mc, ipv6_mc, smi_id::A_MALFORMED_IPV6_PKT, SWITCH_DROP_REASON_OUTER_IP_SRC_MULTICAST, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0},
        // hop limit is zero
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                                          1, 1, 0x6, 0xf, 0, 0xff, ipv6_0s, ipv6_0s, smi_id::A_MALFORMED_IPV6_PKT, SWITCH_DROP_REASON_OUTER_IP_TTL_ZERO, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0},
        // ipv6 valid
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                                          1, 1, 0x6, 0xf, 0, 0, ipv6_0s, ipv6_0s, smi_id::A_VALID_IPV6_PKT, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0},
        // invalid ipv6 version
        // ipv6.valid=True, ip_version!=6,
        // this this rule should be after all IpV6 rules
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                                          1, 1, 0, 0, 0, 0, ipv6_0s, ipv6_0s, smi_id::A_MALFORMED_IPV6_PKT, SWITCH_DROP_REASON_OUTER_IP_VERSION_INVALID, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0},

        // Non IP rules
        // valid arp packet
        {1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                                          0, 1, 0, 0, 0, 0, ipv6_0s, ipv6_0s, smi_id::A_VALID_ARP_PKT, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0},
    };
    std::vector<rule_spec> mpls_rules = {
        // MPLS packet router-alert, mpls-valid=1, mpls-label=1
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                                          0, 0, 0, 0, 0, 0, ipv6_0s, ipv6_0s, smi_id::A_VALID_MPLS_ROUTER_ALERT_LABEL, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0},
        // MPLS NULL IPV4
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                                          0, 0, 0, 0, 0, 0, ipv6_0s, ipv6_0s, smi_id::A_VALID_MPLS_NULL_IPV4_PKT, 0, 0, 0, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1},
        // MPLS NULL IPV6
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                                          0, 0, 0, 0, 0, 0, ipv6_0s, ipv6_0s, smi_id::A_VALID_MPLS_NULL_IPV6_PKT, 0, 0, 0, 1, 1, 2, 1, 0, 1, 0, 1, 1, 1},
        // MPLS NULL+valid label
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                                          0, 0, 0, 0, 0, 0, ipv6_0s, ipv6_0s, smi_id::A_VALID_MPLS_NULL_PKT, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                                          0, 0, 0, 0, 0, 0, ipv6_0s, ipv6_0s, smi_id::A_VALID_MPLS_NULL_PKT, 0, 0, 0, 1, 1, 2, 1, 1, 1, 0, 0, 0, 0},
        // MPLS packet
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                                          0, 0, 0, 0, 0, 0, ipv6_0s, ipv6_0s, smi_id::A_VALID_MPLS_PKT, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    };
    if (feature::is_feature_set(SWITCH_FEATURE_MPLS)) {
      rules.insert(rules.end(), mpls_rules.begin(), mpls_rules.end());
    }
    // clang-format on

    auto it = match_action_list.begin();
    for (uint32_t i = 0; i < rules.size(); i++) {
      it = match_action_list.insert(it,
                                    std::pair<_MatchKey, _ActionEntry>(
                                        _MatchKey(smi_id::T_VALIDATE_IP),
                                        _ActionEntry(smi_id::T_VALIDATE_IP)));
      status |= it->first.set_exact(smi_id::F_VALIDATE_IP_PRIORITY,
                                    static_cast<uint32_t>(i));

      status |= it->first.set_ternary(smi_id::F_VALIDATE_IP_ARP_VALID,
                                      rules[i].arp_valid,
                                      rules[i].arp_valid_mask);
      status |= it->first.set_ternary(smi_id::F_VALIDATE_IP_IPV4_VALID,
                                      rules[i].ipv4_valid,
                                      rules[i].ipv4_valid_mask);
      status |= it->first.set_ternary(smi_id::F_VALIDATE_IP_IPV4_VERSION,
                                      rules[i].ipv4_version,
                                      rules[i].ipv4_version_mask);
      status |= it->first.set_ternary(smi_id::F_VALIDATE_IP_IPV4_CHKSUM_ERR,
                                      rules[i].ipv4_chksum,
                                      rules[i].ipv4_chksum_mask);
      status |= it->first.set_ternary(smi_id::F_VALIDATE_IP_IPV4_IHL,
                                      rules[i].ipv4_ihl,
                                      rules[i].ipv4_ihl_mask);
      status |= it->first.set_ternary(smi_id::F_VALIDATE_IP_IPV4_FLAGS,
                                      rules[i].ipv4_flags,
                                      rules[i].ipv4_flags_mask);
      status |= it->first.set_ternary(smi_id::F_VALIDATE_IP_IPV4_FRAG_OFFSET,
                                      rules[i].ipv4_frag_offset,
                                      rules[i].ipv4_frag_offset_mask);
      status |= it->first.set_ternary(smi_id::F_VALIDATE_IP_IPV4_TTL,
                                      rules[i].ipv4_ttl,
                                      rules[i].ipv4_ttl_mask);
      status |= it->first.set_ternary(smi_id::F_VALIDATE_IP_IPV4_SRC_ADDR,
                                      rules[i].ipv4_srcAddr,
                                      rules[i].ipv4_srcAddr_mask);
      status |= it->first.set_ternary(smi_id::F_VALIDATE_IP_IPV6_VALID,
                                      rules[i].ipv6_valid,
                                      rules[i].ipv6_valid_mask);
      status |= it->first.set_ternary(smi_id::F_VALIDATE_IP_IPV6_VERSION,
                                      rules[i].ipv6_version,
                                      rules[i].ipv6_version_mask);
      status |= it->first.set_ternary(smi_id::F_VALIDATE_IP_IPV6_HOP_LIMIT,
                                      rules[i].ipv6_hopLimit,
                                      rules[i].ipv6_hopLimit_mask);
      status |= it->first.set_ternary(smi_id::F_VALIDATE_IP_IPV6_SRC_ADDR,
                                      rules[i].ipv6_srcAddr,
                                      rules[i].ipv6_srcAddr_mask,
                                      IPV6_LEN);

      status |= it->first.set_ternary(smi_id::F_VALIDATE_IP_MPLS_0_VALID,
                                      rules[i].mpls_0_valid,
                                      rules[i].mpls_0_valid_mask);
      uint32_t label_mask = 0;
      label_mask = rules[i].mpls_0_label_mask ? 0xFFFFF : 0;
      status |= it->first.set_ternary(smi_id::F_VALIDATE_IP_MPLS_0_LABEL,
                                      rules[i].mpls_0_label,
                                      label_mask);
      status |= it->first.set_ternary(smi_id::F_VALIDATE_IP_MPLS_1_VALID,
                                      rules[i].mpls_1_valid,
                                      rules[i].mpls_1_valid_mask);
      status |= it->first.set_ternary(smi_id::F_VALIDATE_IP_INNER_IPV4_VALID,
                                      rules[i].inner_ipv4_valid,
                                      rules[i].inner_ipv4_valid_mask);
      status |= it->first.set_ternary(smi_id::F_VALIDATE_IP_INNER_IPV6_VALID,
                                      rules[i].inner_ipv6_valid,
                                      rules[i].inner_ipv6_valid_mask);

      it->second.init_action_data(rules[i].action_id);
      if (rules[i].action_id == smi_id::A_MALFORMED_IPV4_PKT) {
        status |= it->second.set_arg(smi_id::P_MALFORMED_IPV4_PKT_REASON,
                                     rules[i].drop_reason);
      } else if (rules[i].action_id == smi_id::A_MALFORMED_IPV6_PKT) {
        status |= it->second.set_arg(smi_id::P_MALFORMED_IPV6_PKT_REASON,
                                     rules[i].drop_reason);
      } else if (rules[i].action_id == smi_id::A_VALID_IPV4_PKT) {
        status |=
            it->second.set_arg(smi_id::P_VALID_IPV4_PKT_IP_FRAG, rules[i].frag);
        status |= it->second.set_arg(smi_id::P_VALID_IPV4_PKT_IS_LINK_LOCAL,
                                     rules[i].link_local);
      }
    }
  }
};

class inner_validate_ethernet : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_INNER_VALIDATE_ETHERNET;
  static const switch_attr_id_t status_attr_id =
      SWITCH_INNER_VALIDATE_ETHERNET_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INNER_VALIDATE_ETHERNET_ATTR_PARENT_HANDLE;

 public:
  inner_validate_ethernet(const switch_object_id_t parent,
                          switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_INNER_VALIDATE_ETHERNET,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    struct rule_spec {
      const char *dstAddr;
      const char *dstAddr_mask;
      uint8_t ethernet_valid;
      uint8_t ethernet_valid_mask;
      uint8_t ipv4_valid;
      uint8_t ipv4_valid_mask;
      uint8_t ipv4_chksum;
      uint8_t ipv4_chksum_mask;
      uint8_t ipv4_version;
      uint8_t ipv4_version_mask;
      uint8_t ipv4_ihl;
      uint8_t ipv4_ihl_mask;
      uint8_t ipv4_ttl;
      uint8_t ipv4_ttl_mask;
      uint8_t ipv6_valid;
      uint8_t ipv6_valid_mask;
      uint8_t ipv6_version;
      uint8_t ipv6_version_mask;
      uint8_t ipv6_hop_limit;
      uint8_t ipv6_hop_limit_mask;
      uint8_t tcp_valid;
      uint8_t tcp_valid_mask;
      uint8_t udp_valid;
      uint8_t udp_valid_mask;
      bf_rt_action_id_t action_id;
      uint8_t drop_reason;
      uint8_t pkt_type;
    };

    // clang-format off
    const std::vector<rule_spec> rules = {
        // mac da is zeros
        {eth_0s, eth_1s, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, smi_id::A_INNER_L2_MALFORMED_PKT, SWITCH_DROP_REASON_DST_MAC_ZERO, 0},

        // ttl is zeros
        {eth_0s, eth_0s, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0xff,          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, smi_id::A_INNER_L3_MALFORMED_PKT, SWITCH_DROP_REASON_IP_TTL_ZERO, 0},
        // invalid ihl
        {eth_0s, eth_0s, 1, 1, 1, 1, 0, 0, 0, 0, 0x0, 0xc, 0, 0,         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, smi_id::A_INNER_L3_MALFORMED_PKT, SWITCH_DROP_REASON_IP_IHL_INVALID, 0},
        // invalid ihl
        {eth_0s, eth_0s, 1, 1, 1, 1, 0, 0, 0, 0, 0x4, 0xf, 0, 0,         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, smi_id::A_INNER_L3_MALFORMED_PKT, SWITCH_DROP_REASON_IP_IHL_INVALID, 0},

        // v6 hop limit is zero
        {eth_0s, eth_0s, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,             1, 1, 0, 0, 0, 0xff, 0, 0, 0, 0, smi_id::A_INNER_L3_MALFORMED_PKT, SWITCH_DROP_REASON_IP_TTL_ZERO, 0},

        // ethernet validity is don't care for IPinIP packets
        // valid eth, ipv4 tcp broadcast
        {eth_1s, eth_1s, 1, 1, 1, 1, 0x0, 0x1, 0x4, 0xf, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 1, 1, 0, 0, smi_id::A_INNER_VALID_IPV4_TCP_PKT, 0, SWITCH_PKT_TYPE_BROADCAST},
        // valid eth, valid ipv4 udp broadcast
        {eth_1s, eth_1s, 1, 1, 1, 1, 0x0, 0x1, 0x4, 0xf, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0, 1, 1, smi_id::A_INNER_VALID_IPV4_UDP_PKT, 0, SWITCH_PKT_TYPE_BROADCAST},
        // valid eth, valid ipv4 tcp multicast
        {eth_mc, eth_mc, 1, 1, 1, 1, 0x0, 0x1, 0x4, 0xf, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 1, 1, 0, 0, smi_id::A_INNER_VALID_IPV4_TCP_PKT, 0, SWITCH_PKT_TYPE_MULTICAST},
        // valid eth, valid ipv4 udp multicast
        {eth_mc, eth_mc, 1, 1, 1, 1, 0x0, 0x1, 0x4, 0xf, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0, 1, 1, smi_id::A_INNER_VALID_IPV4_UDP_PKT, 0, SWITCH_PKT_TYPE_MULTICAST},
        // valid eth, valid ipv4 tcp unicast
        {eth_0s, eth_0s, 1, 1, 1, 1, 0x0, 0x1, 0x4, 0xf, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 1, 1, 0, 0, smi_id::A_INNER_VALID_IPV4_TCP_PKT, 0, SWITCH_PKT_TYPE_UNICAST},
        // valid eth, valid ipv4 udp unicast
        {eth_0s, eth_0s, 1, 1, 1, 1, 0x0, 0x1, 0x4, 0xf, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0, 1, 1, smi_id::A_INNER_VALID_IPV4_UDP_PKT, 0, SWITCH_PKT_TYPE_UNICAST},

        // valid eth, valid ipv6 tcp broadcast
        {eth_1s, eth_1s, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,             1, 1, 0x6, 0xf, 0, 0, 1, 1, 0, 0, smi_id::A_INNER_VALID_IPV6_TCP_PKT, 0, SWITCH_PKT_TYPE_BROADCAST},
        // valid eth, valid ipv6 udp broadcast
        {eth_1s, eth_1s, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,             1, 1, 0x6, 0xf, 0, 0, 0, 0, 1, 1, smi_id::A_INNER_VALID_IPV6_UDP_PKT, 0, SWITCH_PKT_TYPE_BROADCAST},
        // valid eth, valid ipv6 tcp multicast
        {eth_mc, eth_mc, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,             1, 1, 0x6, 0xf, 0, 0, 1, 1, 0, 0, smi_id::A_INNER_VALID_IPV6_TCP_PKT, 0, SWITCH_PKT_TYPE_MULTICAST},
        // valid eth, valid ipv6 udp multicast
        {eth_mc, eth_mc, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,             1, 1, 0x6, 0xf, 0, 0, 0, 0, 1, 1, smi_id::A_INNER_VALID_IPV6_UDP_PKT, 0, SWITCH_PKT_TYPE_MULTICAST},
        // valid eth, valid ipv6 tcp unicast
        {eth_0s, eth_0s, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,             1, 1, 0x6, 0xf, 0, 0, 1, 1, 0, 0, smi_id::A_INNER_VALID_IPV6_TCP_PKT, 0, SWITCH_PKT_TYPE_UNICAST},
        // valid eth, valid ipv6 udp unicast
        {eth_0s, eth_0s, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,             1, 1, 0x6, 0xf, 0, 0, 0, 0, 1, 1, smi_id::A_INNER_VALID_IPV6_UDP_PKT, 0, SWITCH_PKT_TYPE_UNICAST},

        // non udp/tcp IP packets
        // valid eth, valid ipv4 broadcast
        {eth_1s, eth_1s, 1, 1, 1, 1, 0x0, 0x1, 0x4, 0xf, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 1, 0, 1, smi_id::A_INNER_VALID_IPV4_PKT, 0, SWITCH_PKT_TYPE_BROADCAST},
        // valid eth, valid ipv4 multicast
        {eth_mc, eth_mc, 1, 1, 1, 1, 0x0, 0x1, 0x4, 0xf, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 1, 0, 1, smi_id::A_INNER_VALID_IPV4_PKT, 0, SWITCH_PKT_TYPE_MULTICAST},
        // valid eth, valid ipv4 unicast
        {eth_0s, eth_0s, 1, 1, 1, 1, 0x0, 0x1, 0x4, 0xf, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 1, 0, 1, smi_id::A_INNER_VALID_IPV4_PKT, 0, SWITCH_PKT_TYPE_UNICAST},
        // valid eth, valid ipv6 broadcast
        {eth_1s, eth_1s, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,             1, 1, 0x6, 0xf, 0, 0, 0, 1, 0, 1, smi_id::A_INNER_VALID_IPV6_PKT, 0, SWITCH_PKT_TYPE_BROADCAST},
        // valid eth, valid ipv6 multicast
        {eth_mc, eth_mc, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,             1, 1, 0x6, 0xf, 0, 0, 0, 1, 0, 1, smi_id::A_INNER_VALID_IPV6_PKT, 0, SWITCH_PKT_TYPE_MULTICAST},
        // valid eth, valid ipv6 unicast
        {eth_0s, eth_0s, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,             1, 1, 0x6, 0xf, 0, 0, 0, 1, 0, 1, smi_id::A_INNER_VALID_IPV6_PKT, 0, SWITCH_PKT_TYPE_UNICAST},

        // IPinIP packets validity
        // set pkt_type as unicast for IPinIP
        // valid ipv4 tcp unicast
        {eth_0s, eth_0s, 0, 0, 1, 1, 0x0, 0x1, 0x4, 0xf, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 1, 1, 0, 0, smi_id::A_INNER_VALID_IPV4_TCP_PKT, 0, SWITCH_PKT_TYPE_UNICAST},
        // valid ipv4 udp unicast
        {eth_0s, eth_0s, 0, 0, 1, 1, 0x0, 0x1, 0x4, 0xf, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0, 1, 1, smi_id::A_INNER_VALID_IPV4_UDP_PKT, 0, SWITCH_PKT_TYPE_UNICAST},

        // valid ipv6 tcp unicast
        {eth_0s, eth_0s, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,             1, 1, 0x6, 0xf, 0, 0, 1, 1, 0, 0, smi_id::A_INNER_VALID_IPV6_TCP_PKT, 0, SWITCH_PKT_TYPE_UNICAST},
        // valid ipv6 udp unicast
        {eth_0s, eth_0s, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,             1, 1, 0x6, 0xf, 0, 0, 0, 0, 1, 1, smi_id::A_INNER_VALID_IPV6_UDP_PKT, 0, SWITCH_PKT_TYPE_UNICAST},

        // non udp/tcp IP packets
        // valid ipv4 unicast
        {eth_0s, eth_0s, 0, 0, 1, 1, 0x0, 0x1, 0x4, 0xf, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 1, 0, 1, smi_id::A_INNER_VALID_IPV4_PKT, 0, SWITCH_PKT_TYPE_UNICAST},
        // valid ipv6 unicast
        {eth_0s, eth_0s, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,             1, 1, 0x6, 0xf, 0, 0, 0, 1, 0, 1, smi_id::A_INNER_VALID_IPV6_PKT, 0, SWITCH_PKT_TYPE_UNICAST},

        // valid ethernet broadcast packet and non IP
        {eth_1s, eth_1s, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,             0, 1, 0, 0, 0, 0, 0, 0, 0, 0, smi_id::A_INNER_VALID_ETHERNET_PKT, 0, SWITCH_PKT_TYPE_BROADCAST},
        // valid ethernet multicast packet and non IP
        {eth_mc, eth_mc, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,             0, 1, 0, 0, 0, 0, 0, 0, 0, 0, smi_id::A_INNER_VALID_ETHERNET_PKT, 0, SWITCH_PKT_TYPE_MULTICAST},
        // valid ethernet unicast packet and non IP
        {eth_0s, eth_0s, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,             0, 1, 0, 0, 0, 0, 0, 0, 0, 0, smi_id::A_INNER_VALID_ETHERNET_PKT, 0, SWITCH_PKT_TYPE_UNICAST},

        // invalid ipv4 checksum
        {eth_0s, eth_0s, 0, 0, 1, 1, 1, 1, 0x4, 0xf, 0, 0, 0, 0,         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, smi_id::A_INNER_L3_MALFORMED_PKT, SWITCH_DROP_REASON_IP_INVALID_CHECKSUM, 0},
        // invalid ipv4 version
        {eth_0s, eth_0s, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, smi_id::A_INNER_L3_MALFORMED_PKT, SWITCH_DROP_REASON_IP_VERSION_INVALID, 0},
        // invalid ipv6 version
        {eth_0s, eth_0s, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,             1, 1, 0, 0, 0, 0, 0, 0, 0, 0, smi_id::A_INNER_L3_MALFORMED_PKT, SWITCH_DROP_REASON_IP_VERSION_INVALID, 0},
    };
    // clang-format on

    auto it = match_action_list.begin();
    for (uint32_t i = 0; i < rules.size(); i++) {
      if (!feature::is_feature_set(SWITCH_FEATURE_INNER_L2)) {
        if (rules[i].action_id == smi_id::A_INNER_VALID_ETHERNET_PKT ||
            // Skip all multicast/broadcast rules for tunnel terminate cases
            // when there is no inner L2 header.
            rules[i].drop_reason == SWITCH_DROP_REASON_DST_MAC_ZERO ||
            rules[i].pkt_type == SWITCH_PKT_TYPE_MULTICAST ||
            rules[i].pkt_type == SWITCH_PKT_TYPE_BROADCAST) {
          continue;
        }
      }
      it = match_action_list.insert(
          it,
          std::pair<_MatchKey, _ActionEntry>(
              _MatchKey(smi_id::T_INNER_VALIDATE_ETHERNET),
              _ActionEntry(smi_id::T_INNER_VALIDATE_ETHERNET)));
      status |= it->first.set_exact(smi_id::F_INNER_VALIDATE_ETHERNET_PRIORITY,
                                    static_cast<uint32_t>(i));
      status |= it->first.set_ternary(
          smi_id::F_INNER_VALIDATE_ETHERNET_ETHERNET_VALID,
          rules[i].ethernet_valid,
          rules[i].ethernet_valid_mask);
      if (feature::is_feature_set(SWITCH_FEATURE_INNER_L2)) {
        status |= it->first.set_ternary(
            smi_id::F_INNER_VALIDATE_ETHERNET_ETHERNET_DST_ADDR,
            rules[i].dstAddr,
            rules[i].dstAddr_mask,
            ETH_LEN);
      }
      status |=
          it->first.set_ternary(smi_id::F_INNER_VALIDATE_ETHERNET_IPV4_VALID,
                                rules[i].ipv4_valid,
                                rules[i].ipv4_valid_mask);
      status |= it->first.set_ternary(
          smi_id::F_INNER_VALIDATE_ETHERNET_IPV4_CHKSUM_ERR,
          rules[i].ipv4_chksum,
          rules[i].ipv4_chksum_mask);
      status |=
          it->first.set_ternary(smi_id::F_INNER_VALIDATE_ETHERNET_IPV4_VERSION,
                                rules[i].ipv4_version,
                                rules[i].ipv4_version_mask);
      status |=
          it->first.set_ternary(smi_id::F_INNER_VALIDATE_ETHERNET_IPV4_IHL,
                                rules[i].ipv4_ihl,
                                rules[i].ipv4_ihl_mask);
      status |=
          it->first.set_ternary(smi_id::F_INNER_VALIDATE_ETHERNET_IPV4_TTL,
                                rules[i].ipv4_ttl,
                                rules[i].ipv4_ttl_mask);
      status |=
          it->first.set_ternary(smi_id::F_INNER_VALIDATE_ETHERNET_IPV6_VALID,
                                rules[i].ipv6_valid,
                                rules[i].ipv6_valid_mask);
      status |=
          it->first.set_ternary(smi_id::F_INNER_VALIDATE_ETHERNET_IPV6_VERSION,
                                rules[i].ipv6_version,
                                rules[i].ipv6_version_mask);
      status |= it->first.set_ternary(
          smi_id::F_INNER_VALIDATE_ETHERNET_IPV6_HOP_LIMIT,
          rules[i].ipv6_hop_limit,
          rules[i].ipv6_hop_limit_mask);
      status |=
          it->first.set_ternary(smi_id::F_INNER_VALIDATE_ETHERNET_TCP_VALID,
                                rules[i].tcp_valid,
                                rules[i].tcp_valid_mask);
      status |=
          it->first.set_ternary(smi_id::F_INNER_VALIDATE_ETHERNET_UDP_VALID,
                                rules[i].udp_valid,
                                rules[i].udp_valid_mask);

      it->second.init_action_data(rules[i].action_id);
      if (rules[i].action_id == smi_id::A_INNER_L2_MALFORMED_PKT) {
        status |= it->second.set_arg(smi_id::P_INNER_L2_MALFORMED_PKT_REASON,
                                     rules[i].drop_reason);
      } else if (rules[i].action_id == smi_id::A_INNER_L3_MALFORMED_PKT) {
        status |= it->second.set_arg(smi_id::P_INNER_L3_MALFORMED_PKT_REASON,
                                     rules[i].drop_reason);
      } else if (rules[i].action_id == smi_id::A_INNER_VALID_IPV4_TCP_PKT) {
        status |= it->second.set_arg(smi_id::P_INNER_VALID_IPV4_TCP_PKT_TYPE,
                                     rules[i].pkt_type);
      } else if (rules[i].action_id == smi_id::A_INNER_VALID_IPV4_UDP_PKT) {
        status |= it->second.set_arg(smi_id::P_INNER_VALID_IPV4_UDP_PKT_TYPE,
                                     rules[i].pkt_type);
      } else if (rules[i].action_id == smi_id::A_INNER_VALID_IPV6_TCP_PKT) {
        status |= it->second.set_arg(smi_id::P_INNER_VALID_IPV6_TCP_PKT_TYPE,
                                     rules[i].pkt_type);
      } else if (rules[i].action_id == smi_id::A_INNER_VALID_IPV6_UDP_PKT) {
        status |= it->second.set_arg(smi_id::P_INNER_VALID_IPV6_UDP_PKT_TYPE,
                                     rules[i].pkt_type);
      } else if (rules[i].action_id == smi_id::A_INNER_VALID_ETHERNET_PKT) {
        status |= it->second.set_arg(smi_id::P_INNER_VALID_ETHERNET_PKT_TYPE,
                                     rules[i].pkt_type);
      } else if (rules[i].action_id == smi_id::A_INNER_VALID_IPV4_PKT) {
        status |= it->second.set_arg(smi_id::P_INNER_VALID_IPV4_PKT_TYPE,
                                     rules[i].pkt_type);
      } else if (rules[i].action_id == smi_id::A_INNER_VALID_IPV6_PKT) {
        status |= it->second.set_arg(smi_id::P_INNER_VALID_IPV6_PKT_TYPE,
                                     rules[i].pkt_type);
      }
    }
  }
};

switch_status_t validation_init() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  REGISTER_OBJECT(validate_ethernet, SWITCH_OBJECT_TYPE_VALIDATE_ETHERNET);
  REGISTER_OBJECT(validate_ip, SWITCH_OBJECT_TYPE_VALIDATE_IP);
  REGISTER_OBJECT(inner_validate_ethernet,
                  SWITCH_OBJECT_TYPE_INNER_VALIDATE_ETHERNET);

  return status;
}

switch_status_t validation_clean() { return SWITCH_STATUS_SUCCESS; }

}  // namespace smi
