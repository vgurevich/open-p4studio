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


#include <bitset>

#include "common/utils.h"
#include "switch_tna/bf_rt_ids.h"

namespace smi {
using ::smi::bf_rt::smi_id;

/**
 * Create a feature list based on br_rt_info and smi_ids
 * Obviously, this routine has to be called after smi_id::init_bf_rt_ids()
 */
void feature::init_features() {
  feature_bmap.reset();

  // shared ip lpm
  if (smi_id::T_IP_FIB_LPM64) feature_set(SWITCH_FEATURE_SHARED_ALPM);

  // ipv6 lpm64
  if (smi_id::T_IPV6_FIB_LPM64) feature_set(SWITCH_FEATURE_IPV6_LPM64);

  // ipv6 host64
  if (smi_id::T_IPV6_FIB_HOST64) feature_set(SWITCH_FEATURE_IPV6_HOST64);

  // ipv6 lpm tcam
  if (smi_id::T_IPV6_FIB_LPM_TCAM)
    feature_set(SWITCH_FEATURE_IPV6_FIB_LPM_TCAM);

  // stp
  if (smi_id::T_INGRESS_STP0_CHECK) feature_set(SWITCH_FEATURE_STP);
  if (smi_id::T_INGRESS_STP_GROUP) feature_set(SWITCH_FEATURE_MSTP);

  // l2 multicast
  if (smi_id::T_IPV4_MULTICAST_BRIDGE_S_G ||
      smi_id::T_IPV4_MULTICAST_BRIDGE_X_G)
    feature_set(SWITCH_FEATURE_MULTICAST);

  // l3 multicast
  if (smi_id::T_IPV4_MULTICAST_ROUTE_S_G || smi_id::T_IPV4_MULTICAST_ROUTE_X_G)
    feature_set(SWITCH_FEATURE_MULTICAST);

  // ingress qos map
  if (smi_id::T_DSCP_TC_MAP && smi_id::T_PCP_TC_MAP)
    feature_set(SWITCH_FEATURE_INGRESS_QOS_MAP);

  // ingress qos acl
  if (smi_id::T_INGRESS_IP_QOS_ACL)
    feature_set(SWITCH_FEATURE_INGRESS_IP_QOS_ACL);

  //  // MAC qos acl
  //  if (smi_id::T_INGRESS_MAC_QOS_ACL)
  //    feature_set(SWITCH_FEATURE_INGRESS_MAC_QOS_ACL);

  // copp
  if (smi_id::T_COPP && smi_id::T_COPP_METER) feature_set(SWITCH_FEATURE_COPP);

  // storm control
  if (smi_id::T_STORM_CONTROL && smi_id::T_STORM_CONTROL_METER)
    feature_set(SWITCH_FEATURE_STORM_CONTROL);

  // PPG Stats
  if (smi_id::T_PPG) feature_set(SWITCH_FEATURE_PPG_STATS);

  // Ingress Port meter
  if (smi_id::T_INGRESS_PORT_METER_ACTION)
    feature_set(SWITCH_FEATURE_INGRESS_PORT_METER);

  // Ingress Acl meter
  if (smi_id::T_INGRESS_ACL_METER_ACTION ||
      smi_id::T_INGRESS_IP_QOS_ACL_METER_ACTION ||
      smi_id::T_INGRESS_IP_MIRROR_ACL_METER_ACTION)
    feature_set(SWITCH_FEATURE_INGRESS_ACL_METER);

  // Egress Port meter
  if (smi_id::T_EGRESS_PORT_METER_ACTION)
    feature_set(SWITCH_FEATURE_EGRESS_PORT_METER);

  // Egress Acl meter
  if (smi_id::T_EGRESS_ACL_METER_ACTION ||
      smi_id::T_EGRESS_IP_QOS_ACL_METER_ACTION ||
      smi_id::T_EGRESS_IP_MIRROR_ACL_METER_ACTION)
    feature_set(SWITCH_FEATURE_EGRESS_ACL_METER);

  // ingress mac/v4/v6 acl - transit actions
  if (smi_id::A_INGRESS_MAC_ACL_TRANSIT && smi_id::A_INGRESS_IPV4_ACL_TRANSIT &&
      smi_id::A_INGRESS_IPV6_ACL_TRANSIT) {
    feature_set(SWITCH_FEATURE_INGRESS_MAC_IP_ACL_TRANSIT_ACTION);
  }

  // ingress mac/v4/v6 acl - deny actions
  if (smi_id::A_INGRESS_MAC_ACL_DENY && smi_id::A_INGRESS_IPV4_ACL_DENY &&
      smi_id::A_INGRESS_IPV6_ACL_DENY) {
    feature_set(SWITCH_FEATURE_INGRESS_MAC_IP_ACL_DENY_ACTION);
  }

  // ingress port mirror
  if (smi_id::T_INGRESS_PORT_MIRROR)
    feature_set(SWITCH_FEATURE_INGRESS_PORT_MIRROR);

  // egress port mirror
  if (smi_id::T_EGRESS_PORT_MIRROR)
    feature_set(SWITCH_FEATURE_EGRESS_PORT_MIRROR);

  // ingress mirror acl
  if (smi_id::T_INGRESS_IP_MIRROR_ACL) {
    feature_set(SWITCH_FEATURE_INGRESS_MIRROR_ACL);
  }

  // ingress ip acl, mirror in/out actions
  if (smi_id::A_INGRESS_IP_MIRROR_ACL_MIRROR &&
      smi_id::A_INGRESS_IP_MIRROR_ACL_MIRROR_OUT) {
    feature_set(SWITCH_FEATURE_INGRESS_MIRROR_ACL_MIRROR_IN_OUT);
  }

  // egress mirror acl
  if (smi_id::T_EGRESS_IPV4_MIRROR_ACL && smi_id::T_EGRESS_IPV6_MIRROR_ACL)
    feature_set(SWITCH_FEATURE_EGRESS_MIRROR_ACL);

  // egress ipv4/ipv6 acl, mirror in/out actions
  if (smi_id::A_EGRESS_IPV4_MIRROR_ACL_MIRROR &&
      smi_id::A_EGRESS_IPV6_MIRROR_ACL_MIRROR &&
      smi_id::A_EGRESS_IPV4_MIRROR_ACL_MIRROR_IN &&
      smi_id::A_EGRESS_IPV6_MIRROR_ACL_MIRROR_IN) {
    feature_set(SWITCH_FEATURE_EGRESS_MIRROR_ACL_MIRROR_IN_OUT);
  }

  // Mirror Meter
  if (smi_id::T_EGRESS_MIRROR_METER || smi_id::T_INGRESS_MIRROR_METER)
    feature_set(SWITCH_FEATURE_MIRROR_METER);

  // wred
  if (smi_id::T_WRED_INDEX && smi_id::T_V4_WRED_ACTION &&
      smi_id::T_V6_WRED_ACTION)
    feature_set(SWITCH_FEATURE_WRED);

  // ptp

  // ACL Redirect nexthop
  if (smi_id::A_INGRESS_IP_ACL_REDIRECT_NEXTHOP ||
      smi_id::A_INGRESS_IPV4_ACL_REDIRECT_NEXTHOP ||
      smi_id::A_INGRESS_IPV6_ACL_REDIRECT_NEXTHOP)
    feature_set(SWITCH_FEATURE_ACL_REDIRECT_NEXTHOP);

  // ACL Redirect port
  if (smi_id::A_INGRESS_IP_ACL_REDIRECT_PORT ||
      smi_id::A_INGRESS_IPV4_ACL_REDIRECT_PORT ||
      smi_id::A_INGRESS_IPV6_ACL_REDIRECT_PORT)
    feature_set(SWITCH_FEATURE_ACL_REDIRECT_PORT);

  // egress ip acl
  if (smi_id::T_EGRESS_IPV4_ACL && smi_id::T_EGRESS_IPV6_ACL)
    feature_set(SWITCH_FEATURE_EGRESS_IP_ACL);

  // egress mac-acl
  if (smi_id::T_EGRESS_MAC_ACL) feature_set(SWITCH_FEATURE_EGRESS_MAC_ACL);

  // ingress l4 port range
  if (smi_id::T_INGRESS_L4_SRC_PORT && smi_id::T_INGRESS_L4_DST_PORT)
    feature_set(SWITCH_FEATURE_INGRESS_L4_PORT_RANGE);

  // egress l4 port range
  if (smi_id::T_EGRESS_L4_SRC_PORT && smi_id::T_EGRESS_L4_DST_PORT)
    feature_set(SWITCH_FEATURE_EGRESS_L4_PORT_RANGE);

  // rspan
  if (smi_id::A_REWRITE_RSPAN) feature_set(SWITCH_FEATURE_RSPAN);

  // erspan
  if (smi_id::A_REWRITE_ERSPAN_TYPE3_PLAT ||
      smi_id::A_REWRITE_ERSPAN_TYPE3_PLAT_VLAN)
    feature_set(SWITCH_FEATURE_ERSPAN_PLATFORM_INFO);

  // erspan type 2
  if (smi_id::A_REWRITE_ERSPAN_TYPE2) feature_set(SWITCH_FEATURE_ERSPAN_TYPE2);

  // ipv4 tunnel
  if (smi_id::T_IPV4_DST_VTEP) feature_set(SWITCH_FEATURE_IPV4_TUNNEL);

  // ipv6 tunnel
  if (smi_id::T_IPV6_DST_VTEP || smi_id::T_MY_SID)
    feature_set(SWITCH_FEATURE_IPV6_TUNNEL);

  // tunnel encap
  if (smi_id::T_TUNNEL_TABLE) feature_set(SWITCH_FEATURE_TUNNEL_ENCAP);

  // tunnel decap
  if (smi_id::T_IPV4_DST_VTEP || smi_id::T_IPV6_DST_VTEP)
    feature_set(SWITCH_FEATURE_TUNNEL_DECAP);

  // vxlan
  if (smi_id::T_VNI_TO_BD_MAPPING) feature_set(SWITCH_FEATURE_VXLAN);

  // mac acl
  if (smi_id::T_INGRESS_MAC_ACL) feature_set(SWITCH_FEATURE_INGRESS_MAC_ACL);

  // l3 unicast self fwd check
  if (smi_id::F_SYSTEM_ACL_LOCAL_MD_CHECKS_SAME_BD) {
    feature_set(SWITCH_FEATURE_L3_UNICAST_SELF_FWD_CHECK);
  }

  // QinQ RIF
  if (smi_id::T_PORT_DOUBLE_TAG_TO_BD_MAPPING)
    feature_set(SWITCH_FEATURE_QINQ_RIF);

  // ipv4_local_host
  if (smi_id::T_IPV4_FIB_LOCAL_HOST) {
    feature_set(SWITCH_FEATURE_IPV4_LOCAL_HOST);
  }

  // dtel drop reports
  if (smi_id::T_MOD_CONFIG) feature_set(SWITCH_FEATURE_DROP_REPORT);

  // dtel queue reports
  if (smi_id::T_QUEUE_REPORT_ALERT) feature_set(SWITCH_FEATURE_QUEUE_REPORT);

  // dtel flow reports
  if (smi_id::T_EGRESS_FLOW_REPORT_BLOOM_FILTER_1 ||
      smi_id::T_EGRESS_FLOW_REPORT_BLOOM_FILTER_2)
    feature_set(SWITCH_FEATURE_FLOW_REPORT);

  // dtel report suppression
  if (smi_id::T_EGRESS_DROP_REPORT_BLOOM_FILTER_1 ||
      smi_id::T_EGRESS_DROP_REPORT_BLOOM_FILTER_2 ||
      smi_id::T_EGRESS_FLOW_REPORT_BLOOM_FILTER_1 ||
      smi_id::T_EGRESS_FLOW_REPORT_BLOOM_FILTER_2)
    feature_set(SWITCH_FEATURE_REPORT_SUPPRESSION);

  // dtel ifa
  if (smi_id::A_INGRESS_IP_DTEL_ACL_IFA_CLONE_SAMPLE ||
      smi_id::A_INGRESS_IP_DTEL_ACL_IFA_CLONE_AND_SET_REPORT_TYPE ||
      smi_id::A_REWRITE_DTEL_IFA_CLONE || smi_id::A_DTEL_CONFIG_MIRROR_CLONE ||
      smi_id::A_INT_EDGE_SET_CLONE_MIRROR_SESSION_ID ||
      smi_id::F_DTEL_CONFIG_IFA_CLONED)
    feature_set(SWITCH_FEATURE_DTEL_IFA_CLONE);
  if (smi_id::A_DTEL_CONFIG_MIRROR_SWITCH_LOCAL_SET_F_AND_DROP ||
      smi_id::A_INT_EDGE_SET_IFA_EDGE)
    feature_set(SWITCH_FEATURE_DTEL_IFA_EDGE);

  // dtel int v2
  if (smi_id::D_DTEL_CONFIG_UPDATE_MD_LENGTH ||
      smi_id::D_DTEL_CONFIG_UPDATE_REP_MD_BITS)
    feature_set(SWITCH_FEATURE_INT_V2);

  // dtel inner
  if (smi_id::T_INGRESS_INNER_DTEL_IPV4_ACL ||
      smi_id::T_INGRESS_INNER_DTEL_IPV6_ACL)
    feature_set(SWITCH_FEATURE_INNER_DTEL);

  // pfc wd
  if (smi_id::T_INGRESS_PFC_WD_ACL) feature_set(SWITCH_FEATURE_PFC_WD);

  // ACL etype in ACL
  if (smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_MAC_TYPE &&
      smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_MAC_TYPE)
    feature_set(SWITCH_FEATURE_ETYPE_IN_ACL);

  // BD Label in Egress  ACL
  if (smi_id::F_EGRESS_IPV4_ACL_LOCAL_MD_BD_LABEL)
    feature_set(SWITCH_FEATURE_BD_LABEL_IN_EGRESS_ACL);

  // Intenal Vlan/BD in Egress ACL
  if (smi_id::F_EGRESS_IPV4_ACL_LOCAL_MD_BD ||
      smi_id::F_EGRESS_IPV6_ACL_LOCAL_MD_BD ||
      smi_id::F_EGRESS_IPV4_MIRROR_ACL_LOCAL_MD_BD ||
      smi_id::F_EGRESS_IPV6_MIRROR_ACL_LOCAL_MD_BD)
    feature_set(SWITCH_FEATURE_BD_IN_EGRESS_ACL);

  // BD Label in Egress  ACL
  if (smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_BD_LABEL)
    feature_set(SWITCH_FEATURE_BD_LABEL_IN_INGRESS_ACL);

  // Internal Vlan/BD in Ingress  ACL
  if (smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_BD ||
      smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_BD ||
      smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_BD)
    feature_set(SWITCH_FEATURE_BD_IN_INGRESS_ACL);

  // ACL user metadata
  if (smi_id::F_EGRESS_IPV4_ACL_LOCAL_MD_USER_METADATA &&
      smi_id::P_INGRESS_IPV4_ACL_PERMIT_USER_METADATA)
    feature_set(SWITCH_FEATURE_ACL_USER_META);

  // ACL port-group support
  if (smi_id::F_IPV4_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL_7_0)
    feature_set(SWITCH_FEATURE_ACL_PORT_GROUP);

  // ACL IPv6 Upper64
  if (smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD3 &&
      smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD2 &&
      smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD3 &&
      smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD2)
    feature_set(SWITCH_FEATURE_IPV6_ACL_UPPER64);

  // sflow
  if (smi_id::T_INGRESS_SFLOW_SESSION) feature_set(SWITCH_FEATURE_SFLOW);

  // Same MAC Check
  if (smi_id::T_INGRESS_SAME_MAC_CHECK)
    feature_set(SWITCH_FEATURE_SAME_MAC_CHECK);

  // shared ingress ip acl
  if (smi_id::T_INGRESS_IP_ACL)
    feature_set(SWITCH_FEATURE_SHARED_INGRESS_IP_ACL);

  // ingress v4 acl
  if (smi_id::T_INGRESS_IPV4_ACL) feature_set(SWITCH_FEATURE_INGRESS_IPV4_ACL);

  // ingress v6 acl
  if (smi_id::T_INGRESS_IPV6_ACL) feature_set(SWITCH_FEATURE_INGRESS_IPV6_ACL);

  // pre ingress acl
  if (smi_id::T_PRE_INGRESS_ACL) feature_set(SWITCH_FEATURE_PRE_INGRESS_ACL);

  // etrap
  if (smi_id::T_ETRAP_IPV4_ACL || smi_id::T_ETRAP_IPV6_ACL)
    feature_set(SWITCH_FEATURE_ETRAP);

  // in_ports/ out_ports
  if (smi_id::P_SET_PORT_PROPERTIES_IN_PORTS_GROUP_LABEL_IPV4 ||
      smi_id::P_SET_PORT_PROPERTIES_IN_PORTS_GROUP_LABEL_IPV6)
    feature_set(SWITCH_FEATURE_IN_PORTS_IN_DATA);
  if (smi_id::P_SET_PORT_PROPERTIES_IN_PORTS_GROUP_LABEL_MIRROR)
    feature_set(SWITCH_FEATURE_IN_PORTS_IN_MIRROR);
  if (smi_id::P_PORT_NORMAL_OUT_PORTS_GROUP_LABEL_IPV4 ||
      smi_id::P_PORT_NORMAL_OUT_PORTS_GROUP_LABEL_IPV6)
    feature_set(SWITCH_FEATURE_OUT_PORTS);

  // NAT
  if (smi_id::T_INGRESS_NAT_DEST_NAT || smi_id::T_INGRESS_NAT_DEST_NAPT ||
      smi_id::T_INGRESS_NAT_FLOW_NAT)
    feature_set(SWITCH_FEATURE_NAT);
  if (smi_id::T_INGRESS_NAT_DEST_NAT) feature_set(SWITCH_FEATURE_BASIC_NAT);
  if (smi_id::T_INGRESS_NAT_DEST_NAPT) feature_set(SWITCH_FEATURE_NAPT);
  if (smi_id::T_INGRESS_NAT_FLOW_NAT) feature_set(SWITCH_FEATURE_FLOW_NAT);
  if (smi_id::T_INGRESS_NAT_DNAPT_INDEX)
    feature_set(SWITCH_FEATURE_NAT_NAPT_INDEX_INDIRECTION);

  // CPU BD
  if (smi_id::T_CPU_TO_BD_MAPPING) feature_set(SWITCH_FEATURE_CPU_BD_MAP);

  // SRv6
  if (smi_id::T_MY_SID) feature_set(SWITCH_FEATURE_SRV6);

  // MPLS
  if (smi_id::T_MPLS_FIB) feature_set(SWITCH_FEATURE_MPLS);

  // Tunnels with inner L2 header
  if (smi_id::F_INNER_VALIDATE_ETHERNET_ETHERNET_DST_ADDR)
    feature_set(SWITCH_FEATURE_INNER_L2);

  // Tunnel TTL mode
  if (smi_id::T_TUNNEL_REWRITE_ENCAP_TTL)
    feature_set(SWITCH_FEATURE_TUNNEL_TTL_MODE);

  // Tunnel ECN RFC 6040 support (otherwise default is copy from outer)
  if (smi_id::F_MODE_ECN) feature_set(SWITCH_FEATURE_TUNNEL_ECN_RFC_6040);

  // Tunnel QoS (DSCP) mode
  if (smi_id::A_TUNNEL_REWRITE_ENCAP_DSCP_V4_IN_V4_ECN)
    feature_set(SWITCH_FEATURE_TUNNEL_QOS_MODE);

  // Asymmetric Folded Pipeline
  if (smi_id::P_PORT_METADATA_EXT_INGRESS_PORT)
    feature_set(SWITCH_FEATURE_ASYMMETRIC_FOLDED_PIPELINE);

  // port isolation
  if (smi_id::T_EGRESS_EGRESS_PORT_ISOLATION)
    feature_set(SWITCH_FEATURE_PORT_ISOLATION);

  // peer_link to tunnel isolation
  if (smi_id::T_EGRESS_PEER_LINK_TUNNEL_ISOLATION)
    feature_set(SWITCH_FEATURE_PEER_LINK_TUNNEL_ISOLATION);

  // Egress CoPP
  if (smi_id::T_EGRESS_COPP) feature_set(SWITCH_FEATURE_EGRESS_COPP);

  // Source Flow Control (SFC)
  if (smi_id::T_SFC_FILTER_EPOCH_REG && smi_id::P_SFC_FILTER_EPOCH_REG_DURATION)
    feature_set(SWITCH_FEATURE_SFC);

  // L2 VXLAN
  if (smi_id::T_BD_TO_VNI_MAPPING) feature_set(SWITCH_FEATURE_L2_VXLAN);

  // Fib Label
  if (smi_id::F_INGRESS_IP_ACL_LOCAL_MD_FIB_LABEL ||
      smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_FIB_LABEL ||
      smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_FIB_LABEL)
    feature_set(SWITCH_FEATURE_FIB_ACL_LABEL);

  // Ingress DSCP Mirror ACL
  if (smi_id::T_INGRESS_TOS_MIRROR_ACL)
    feature_set(SWITCH_FEATURE_INGRESS_TOS_MIRROR_ACL);

  // Egress DSCP Mirror ACL
  if (smi_id::T_EGRESS_TOS_MIRROR_ACL)
    feature_set(SWITCH_FEATURE_EGRESS_TOS_MIRROR_ACL);

  // Ingress/Egress IP Port Stats
  if (smi_id::T_INGRESS_PORT_IP_STATS || smi_id::T_EGRESS_PORT_IP_STATS)
    feature_set(SWITCH_FEATURE_IP_STATS);

  if (smi_id::A_TUNNEL_TABLE_ENCAP_IPV4_GRE ||
      smi_id::A_TUNNEL_TABLE_ENCAP_IPV6_GRE)
    feature_set(SWITCH_FEATURE_IPGRE);

  // ECMP rotate hash offset
  if (smi_id::T_ROTATE_HASH)
    feature_set(SWITCH_FEATURE_ECMP_DEFAULT_HASH_OFFSET);

  // multiple rifs per port
  if (smi_id::F_PORT_VLAN_TO_BD_MAPPING_SRC_MAC_ADDRESS)
    feature_set(SWITCH_FEATURE_MULTIPLE_RIFS_PER_PORT);

  // Folded pipeline
  if (smi_id::T_INGRESS_BD_STATE_IG_1 && smi_id::T_INGRESS_PORT_STATE_IG_1 &&
      smi_id::T_INGRESS_PORT_STATE_EG_1)
    feature_set(SWITCH_FEATURE_FOLDED_SWITCH_PIPELINE);

  // Neighbor Trap
  if (smi_id::P_SET_NEXTHOP_PROPERTIES_GLEAN_TRAP_ID)
    feature_set(SWITCH_FEATURE_UDT_TYPE_NEIGHBOR);

  // ingress acl
  if (smi_id::T_INGRESS_IPV4_ACL || smi_id::T_INGRESS_IPV6_ACL ||
      smi_id::T_PRE_INGRESS_ACL || smi_id::T_INGRESS_IP_ACL ||
      smi_id::T_INGRESS_MAC_ACL || smi_id::T_INGRESS_IP_MIRROR_ACL ||
      smi_id::T_INGRESS_IP_DTEL_ACL) {
    feature_set(SWITCH_FEATURE_INGRESS_ACL);
  }

  // egress acl
  if (smi_id::T_EGRESS_MAC_ACL || smi_id::T_EGRESS_IPV4_ACL ||
      smi_id::T_EGRESS_IPV6_ACL || smi_id::T_EGRESS_IPV4_MIRROR_ACL ||
      smi_id::T_EGRESS_IPV6_MIRROR_ACL) {
    feature_set(SWITCH_FEATURE_EGRESS_ACL);
  }

  if (smi_id::T_BFD_RX_SESSION) {
    feature_set(SWITCH_FEATURE_BFD_OFFLOAD);
  }

  if (smi_id::T_INGRESS_PKTGEN_PORT) {
    feature_set(SWITCH_FEATURE_PKTGEN);
  }

  // Egress system acl stats
  if (smi_id::D_EGRESS_SYSTEM_ACL_DROP_STATS_PKTS)
    feature_set(SWITCH_FEATURE_EGRESS_SYSTEM_ACL_STATS);
}

void feature::clear_features() { feature_bmap.reset(); }

void feature::feature_set(switch_feature_id_t feature) {
  if (feature < 0 || feature >= SWITCH_FEATURE_MAX) {
    return;
  }
  feature_bmap.set(feature);
}

bool feature::is_feature_set(switch_feature_id_t feature) {
  if (feature < 0 || feature >= SWITCH_FEATURE_MAX) {
    return false;
  }
  return feature_bmap.test(feature);
}

std::bitset<SWITCH_FEATURE_MAX> feature::feature_bmap;
}  // namespace smi
