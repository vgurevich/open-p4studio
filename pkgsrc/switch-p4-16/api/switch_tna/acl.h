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


#include "utils.h"
#include "p4_16_types.h"

#ifndef __SMI_ACL_H__
#define __SMI_ACL_H__

namespace smi {
using namespace smi::bf_rt;

/* macros */

/*
 * System ACL priority values are 24-bit. The lower the priority
 * value, the higher is the ACL priority in hardware.
 *
 * This is the allocation scheme:
 * 0x000005-0x0000FF: Internal system ACL (high prio; 8-bit)
 * 0x000100-0x8000FF: Hostif Trap ACL (high prio; 23-bit)
 * 0x800100-0x8001FF: Internal system ACL (low prio; 8-bit)
 * 0x800200-0xFFFFFF: Hostif Trap ACL (low prio; unused)
 */
#define SWITCH_INTERNAL_ACL_HIGH_PRIO_START 0x5
#define SWITCH_HOSTIF_ACL_HIGH_PRIO_START 0x100
#define SWITCH_INTERNAL_ACL_LOW_PRIO_START 0x800100
#define SWITCH_HOSTIF_ACL_LOW_PRIO_START 0x800200  // unused currently

/* Set ACL deny to 1 (highest) */
#define SWITCH_INTERNAL_ACL_DENY_HIGH_PRIO 1

/* enums */
typedef enum system_acl_default_internal_types_ {
  SYSTEM_ACL_LOW_PRIORITY_TYPE_START =
      0, /* base for low priority internal system acls  */
  SYSTEM_ACL_HIGH_PRIORITY_TYPE_START =
      0, /* base for high priority internal system acls  */
  SYSTEM_ACL_TYPE_HOSTIF_TRAP = SYSTEM_ACL_HIGH_PRIORITY_TYPE_START,
} system_acl_default_internal_types_t;

typedef enum _acl_hw_entry_attr {
  ACL_HW_ENTRY_ATTR_TABLE_TYPE = 1,
  ACL_HW_ENTRY_ATTR_SRC_MAC,
  ACL_HW_ENTRY_ATTR_DST_MAC,
  ACL_HW_ENTRY_ATTR_ETH_TYPE,
  ACL_HW_ENTRY_ATTR_SRC_IP,
  ACL_HW_ENTRY_ATTR_SRC_IPV6_WORD3,
  ACL_HW_ENTRY_ATTR_SRC_IPV6_WORD2,
  ACL_HW_ENTRY_ATTR_SRC_IPV6_WORD10,
  ACL_HW_ENTRY_ATTR_DST_IP,
  ACL_HW_ENTRY_ATTR_DST_IPV6_WORD3,
  ACL_HW_ENTRY_ATTR_DST_IPV6_WORD2,
  ACL_HW_ENTRY_ATTR_DST_IPV6_WORD10,
  ACL_HW_ENTRY_ATTR_IP_PROTO,
  ACL_HW_ENTRY_ATTR_IP_TOS,
  ACL_HW_ENTRY_ATTR_L4_SRC_PORT,
  ACL_HW_ENTRY_ATTR_L4_DST_PORT,
  ACL_HW_ENTRY_ATTR_TCP_SRC_PORT,
  ACL_HW_ENTRY_ATTR_TCP_DST_PORT,
  ACL_HW_ENTRY_ATTR_UDP_SRC_PORT,
  ACL_HW_ENTRY_ATTR_UDP_DST_PORT,
  ACL_HW_ENTRY_ATTR_TTL,
  ACL_HW_ENTRY_ATTR_IP_FRAG,
  ACL_HW_ENTRY_ATTR_TCP_FLAGS,
  ACL_HW_ENTRY_ATTR_SRC_PORT_RANGE_ID,
  ACL_HW_ENTRY_ATTR_DST_PORT_RANGE_ID,
  ACL_HW_ENTRY_ATTR_PERMIT_USER_METADATA,
  ACL_HW_ENTRY_ATTR_PERMIT_METER_INDEX,
  ACL_HW_ENTRY_ATTR_PERMIT_TRAP_ID,
  ACL_HW_ENTRY_ATTR_REDIRECT_NEXTHOP_USER_METADATA,
  ACL_HW_ENTRY_ATTR_REDIRECT_PORT_USER_METADATA,
  ACL_HW_ENTRY_ATTR_USER_METADATA,
  ACL_HW_ENTRY_ATTR_FIB_LABEL,
  ACL_HW_ENTRY_ATTR_PORT_LAG_LABEL,
  ACL_HW_ENTRY_ATTR_BD_LABEL,
  ACL_HW_ENTRY_ATTR_BD,
  ACL_HW_ENTRY_ATTR_PRIORITY,
  ACL_HW_ENTRY_ATTR_ACL_NO_ACTION,
  ACL_HW_ENTRY_ATTR_NO_ACTION_METER_INDEX,
  ACL_HW_ENTRY_ATTR_ACL_DROP,
  ACL_HW_ENTRY_ATTR_ACL_PERMIT,
  ACL_HW_ENTRY_ATTR_ACL_REDIRECT_NEXTHOP,
  ACL_HW_ENTRY_ATTR_ACL_REDIRECT_PORT,
  ACL_HW_ENTRY_ATTR_REDIRECT_NEXTHOP_HANDLE,
  ACL_HW_ENTRY_ATTR_REDIRECT_PORT_LAG_INDEX,
  ACL_HW_ENTRY_ATTR_ACL_MIRROR_IN,
  ACL_HW_ENTRY_ATTR_MIRROR_IN_HANDLE,
  ACL_HW_ENTRY_ATTR_MIRROR_IN_METER_HANDLE,
  ACL_HW_ENTRY_ATTR_ACL_MIRROR_OUT,
  ACL_HW_ENTRY_ATTR_MIRROR_OUT_HANDLE,
  ACL_HW_ENTRY_ATTR_MIRROR_OUT_METER_HANDLE,
  ACL_HW_ENTRY_ATTR_SET_TC,
  ACL_HW_ENTRY_ATTR_TC,
  ACL_HW_ENTRY_ATTR_TC_METER_INDEX,
  ACL_HW_ENTRY_ATTR_SET_QOS_PARAMS,
  ACL_HW_ENTRY_ATTR_QOS_PARAMS_TC,
  ACL_HW_ENTRY_ATTR_QOS_PARAMS_COLOR,
  ACL_HW_ENTRY_ATTR_QOS_PARAMS_METER_INDEX,
  ACL_HW_ENTRY_ATTR_SET_PCP,
  ACL_HW_ENTRY_ATTR_PCP,
  ACL_HW_ENTRY_ATTR_PCP_METER_INDEX,
  ACL_HW_ENTRY_ATTR_SET_IPV4_TOS,
  ACL_HW_ENTRY_ATTR_IPV4_TOS,
  ACL_HW_ENTRY_ATTR_IPV4_TOS_METER_INDEX,
  ACL_HW_ENTRY_ATTR_SET_IPV6_TOS,
  ACL_HW_ENTRY_ATTR_IPV6_TOS,
  ACL_HW_ENTRY_ATTR_IPV6_TOS_METER_INDEX,
  ACL_HW_ENTRY_ATTR_SET_PCP_IPV4_TOS,
  ACL_HW_ENTRY_ATTR_PCP_IPV4_TOS,
  ACL_HW_ENTRY_ATTR_PCP_IPV4_PCP,
  ACL_HW_ENTRY_ATTR_PCP_IPV4_METER_INDEX,
  ACL_HW_ENTRY_ATTR_SET_PCP_IPV6_TOS,
  ACL_HW_ENTRY_ATTR_PCP_IPV6_TOS,
  ACL_HW_ENTRY_ATTR_PCP_IPV6_PCP,
  ACL_HW_ENTRY_ATTR_PCP_IPV6_METER_INDEX,
  ACL_HW_ENTRY_ATTR_SET_COLOR,
  ACL_HW_ENTRY_ATTR_COLOR,
  ACL_HW_ENTRY_ATTR_COLOR_METER_INDEX,
  ACL_HW_ENTRY_ATTR_SET_METER,
  ACL_HW_ENTRY_ATTR_METER_INDEX,
  ACL_HW_ENTRY_ATTR_DTEL_REPORT,
  ACL_HW_ENTRY_ATTR_REPORT_TYPE,
  ACL_HW_ENTRY_ATTR_TRAP,
  ACL_HW_ENTRY_ATTR_TRAP_TRAP_ID,
  ACL_HW_ENTRY_ATTR_TRAP_METER_INDEX,
  ACL_HW_ENTRY_ATTR_COPY,
  ACL_HW_ENTRY_ATTR_COPY_TRAP_ID,
  ACL_HW_ENTRY_ATTR_COPY_METER_INDEX,
  ACL_HW_ENTRY_ATTR_IFA_CLONE_SAMPLE,
  ACL_HW_ENTRY_ATTR_IFA_CLONE_SESSION_ID,
  ACL_HW_ENTRY_ATTR_IFA_CLONE_AND_DTEL_REPORT,
  ACL_HW_ENTRY_ATTR_IFA_SESSION_ID_WITH_TYPE,
  ACL_HW_ENTRY_ATTR_REPORT_TYPE_WITH_CLONE,
  ACL_HW_ENTRY_ATTR_SAMPLE_SESSION_TABLE,
  ACL_HW_ENTRY_ATTR_SAMPLE_SESSION_REG_INDEX,
  ACL_HW_ENTRY_ATTR_SAMPLE_SESSION_REG_CURRENT,
  ACL_HW_ENTRY_ATTR_SAMPLE_SESSION_REG_RATE,
  ACL_HW_ENTRY_ATTR_ACL_NO_NAT,
  ACL_HW_ENTRY_ATTR_ACL_NO_NAT_DISABLE_NAT,
  ACL_HW_ENTRY_ATTR_SET_VRF,
  ACL_HW_ENTRY_ATTR_VRF,
  ACL_HW_ENTRY_ATTR_STATS_BYTES,
  ACL_HW_ENTRY_ATTR_STATS_PKTS,
  ACL_HW_ENTRY_ATTR_MATCH_PORT_LAG_INDEX,
  ACL_HW_ENTRY_ATTR_PORT_GROUP_INDEX,
  ACL_HW_ENTRY_ATTR_INGRESS_PORT,
  ACL_HW_ENTRY_ATTR_IN_PORTS_GROUP_LABEL,
  ACL_HW_ENTRY_ATTR_OUT_PORTS_GROUP_LABEL,
  ACL_HW_ENTRY_ATTR_EGRESS_PORT_LAG_INDEX,
  ACL_HW_ENTRY_ATTR_VNI,
  ACL_HW_ENTRY_ATTR_VLAN_PRIORITY,
  ACL_HW_ENTRY_ATTR_VLAN_CFI,
  ACL_HW_ENTRY_ATTR_VLAN_HEADER_VALID,
  ACL_HW_ENTRY_ATTR_ACL_DENY,
  ACL_HW_ENTRY_ATTR_ACL_TRANSIT,
  ACL_HW_ENTRY_ATTR_ETYPE_LABEL,
  ACL_HW_ENTRY_ATTR_MACADDR_LABEL,
  ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_TABLE,
  ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_PORT_LAG_INDEX,
  ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_SRC_MAC,
  ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_DST_MAC,
  ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_SET_LABEL,
  ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_SET_LABEL_LABEL,
  ACL_HW_ENTRY_ATTR_MACADDR_COMPRESS_PRIORITY,
  ACL_HW_ENTRY_ATTR_ROUTED,
  ACL_HW_ENTRY_ATTR_CIR,
  ACL_HW_ENTRY_ATTR_PIR,
  ACL_HW_ENTRY_ATTR_CBS,
  ACL_HW_ENTRY_ATTR_PBS,
} acl_hw_entry_attr;

/* function prototypes */

uint32_t system_acl_priority(system_acl_default_internal_types_t acl_type);

// port_lag_label bitmap - unique label space for data ACLs
// Ingress
//        IPv4           IPv6       DSCP-Mirror Mirror      IFA
// |+-+-+-+-+-+-+-+|+-+-+-+-+-+-+-+|+-+-+-+-+-+|+-+-+-+|+-+-+-+-+-+-+-+|
//  0               8              16         20      24              32
// Egress
//     IPv4        IPv6     Mirror
// |+-+-+-+-+-+|+-+-+-+-+-+|+-+-+-+|
//  0           6          12      16
//
// port_lag_label bitmap - shared label space for data ACLs
// Ingress
//        Data            QoS         DTEL   Mirror        RACL
// |+-+-+-+-+-+-+-+|+-+-+-+-+-+-+-+|+-+-+-+|+-+-+-+|+-+-+-+-+-+-+-+|
//  0               8              16      20      24              32
// Egress
//    Data    RACL    QoS    Mirror
// |+-+-+-+|+-+-+-+|+-+-+-+|+-+-+-+|
//  0       4       8      12      16

/*****************************************************************************/
// profiles without racl/qos/mac
// ingress port lag labels
#define SWITCH_ACL_IPV4_ACL_IG_PORT_LABEL_POS 0
#define SWITCH_ACL_IPV4_ACL_IG_PORT_LABEL_WIDTH 8

#define SWITCH_ACL_IPV6_ACL_IG_PORT_LABEL_POS 8
#define SWITCH_ACL_IPV6_ACL_IG_PORT_LABEL_WIDTH 8

#define SWITCH_ACL_DTEL_IFA_ACL_IG_PORT_LABEL_POS 24
#define SWITCH_ACL_DTEL_IFA_ACL_IG_PORT_LABEL_WIDTH 4

#define SWITCH_ACL_EXCLUDE_PORT_LIST_LABEL_POS 28
#define SWITCH_ACL_EXCLUDE_PORT_LIST_LABEL_WIDTH 4

#define SWITCH_ACL_TOS_MIRROR_ACL_IG_PORT_LABEL_POS 16
#define SWITCH_ACL_TOS_MIRROR_ACL_IG_PORT_LABEL_WIDTH 4

// egress port lag labels
#define SWITCH_ACL_IPV4_ACL_EG_PORT_LABEL_POS 0
#define SWITCH_ACL_IPV4_ACL_EG_PORT_LABEL_WIDTH 6

#define SWITCH_ACL_IPV6_ACL_EG_PORT_LABEL_POS 6
#define SWITCH_ACL_IPV6_ACL_EG_PORT_LABEL_WIDTH 6

// bd labels
#define SWITCH_ACL_IPV4_ACL_BD_LABEL_POS 0
#define SWITCH_ACL_IPV4_ACL_BD_LABEL_WIDTH 6

#define SWITCH_ACL_IPV6_ACL_BD_LABEL_POS 6
#define SWITCH_ACL_IPV6_ACL_BD_LABEL_WIDTH 6

#define SWITCH_ACL_DTEL_IFA_ACL_BD_LABEL_POS 12
#define SWITCH_ACL_DTEL_IFA_ACL_BD_LABEL_WIDTH 0

/*****************************************************************************/
// port_lag_label bitmap - unique label space for data ACLs
// Ingress
//        IPv4       IPv6       MAC       Mirror  Qos    IFA    RACL
// |+-+-+-+-+-+-|+-+-+-+-+-|+-+-+-+-+-+|+-+-+-+|+-+-+-+|-+-+-+-+|+-+-+|
//  0           8          16         20      24       28      30    31
// Egress
//     IPv4        IPv6     Mirror
// |+-+-+-+-+-+|+-+-+-+-+-+|+-+-+-+|
//  0           6          12      16
//
/*****************************************************************************/
// profiles with racl/qos/mac
// ingress port lag labels
#define SWITCH_ACL_DATA_IPV4_ACL_IG_PORT_LABEL_POS 0
#define SWITCH_ACL_DATA_IPV4_ACL_IG_PORT_LABEL_WIDTH 8

#define SWITCH_ACL_DATA_IPV6_ACL_IG_PORT_LABEL_POS 8
#define SWITCH_ACL_DATA_IPV6_ACL_IG_PORT_LABEL_WIDTH 8

#define SWITCH_ACL_MAC_ACL_IG_PORT_LABEL_POS 16
#define SWITCH_ACL_MAC_ACL_IG_PORT_LABEL_WIDTH 4

#define SWITCH_ACL_QOS_ACL_IG_PORT_LABEL_POS 24
#define SWITCH_ACL_QOS_ACL_IG_PORT_LABEL_WIDTH 4

#define SWITCH_ACL_DTEL_ACL_IG_PORT_LABEL_POS 28
#define SWITCH_ACL_DTEL_ACL_IG_PORT_LABEL_WIDTH 2

#define SWITCH_ACL_RACL_ACL_IG_PORT_LABEL_POS 30
#define SWITCH_ACL_RACL_ACL_IG_PORT_LABEL_WIDTH 2

// egress port lag labels
#define SWITCH_ACL_DATA_ACL_EG_PORT_LABEL_POS 0
#define SWITCH_ACL_DATA_ACL_EG_PORT_LABEL_WIDTH 4

#define SWITCH_ACL_RACL_ACL_EG_PORT_LABEL_POS 4
#define SWITCH_ACL_RACL_ACL_EG_PORT_LABEL_WIDTH 4

#define SWITCH_ACL_QOS_ACL_EG_PORT_LABEL_POS 8
#define SWITCH_ACL_QOS_ACL_EG_PORT_LABEL_WIDTH 4

#define SWITCH_ACL_TOS_MIRROR_ACL_EG_PORT_LABEL_POS 16
#define SWITCH_ACL_TOS_MIRROR_ACL_EG_PORT_LABEL_WIDTH 4

// bd labels
#define SWITCH_ACL_DATA_ACL_BD_LABEL_POS 0
#define SWITCH_ACL_DATA_ACL_BD_LABEL_WIDTH 4

#define SWITCH_ACL_RACL_ACL_BD_LABEL_POS 4
#define SWITCH_ACL_RACL_ACL_BD_LABEL_WIDTH 4

#define SWITCH_ACL_QOS_ACL_BD_LABEL_POS 8
#define SWITCH_ACL_QOS_ACL_BD_LABEL_WIDTH 4

#define SWITCH_ACL_DTEL_ACL_BD_LABEL_POS 12
#define SWITCH_ACL_DTEL_ACL_BD_LABEL_WIDTH 0

/*****************************************************************************/
// common for all profiles
// port lag labels
#define SWITCH_ACL_IP_MIRROR_ACL_IG_PORT_LABEL_POS 20
#define SWITCH_ACL_IP_MIRROR_ACL_IG_PORT_LABEL_WIDTH 4

#define SWITCH_ACL_IP_MIRROR_ACL_EG_PORT_LABEL_POS 12
#define SWITCH_ACL_IP_MIRROR_ACL_EG_PORT_LABEL_WIDTH 4

// bd labels
#define SWITCH_ACL_IP_MIRROR_ACL_BD_LABEL_POS 12
#define SWITCH_ACL_IP_MIRROR_ACL_BD_LABEL_WIDTH 4
/*****************************************************************************/

#define SWITCH_ACL_LABEL_VALUE_MAX(width) (1 << width)
#define SWITCH_ACL_FEATURE_LABEL_VALUE(label, pos) (label << pos)
#define SWITCH_ACL_FEATURE_LABEL_MASK(pos, width) (((1 << width) - 1) << pos)

#define SWITCH_ACL_LABEL_VALUE(label, pos) (label >> pos)

// in_ports_group_label bitmap -
// Ingress
//        IPv4           IPv6                Mirror
// |+-+-+-+-+-+-+-+|+-+-+-+-+-+-+-+|+-+-+-+-+-+-+-+|
//  0               8              16              24
//
// out_ports_group_lable
// Egress
//  IPv4/Mirror     IPv6/Mirror
// |+-+-+-+-+-+-+-+|+-+-+-+-+-+-+-+|
//  0              8              16

#define SWITCH_ACL_INOUT_PORTS_GROUP_MAX 8

#define SWITCH_ACL_IPV4_ACL_IN_PORTS_LABEL_POS 0
#define SWITCH_ACL_IPV4_ACL_IN_PORTS_LABEL_WIDTH 8

#define SWITCH_ACL_IPV6_ACL_IN_PORTS_LABEL_POS 8
#define SWITCH_ACL_IPV6_ACL_IN_PORTS_LABEL_WIDTH 8

#define SWITCH_ACL_IP_MIRROR_ACL_IN_PORTS_LABEL_POS 16
#define SWITCH_ACL_IP_MIRROR_ACL_IN_PORTS_LABEL_WIDTH 8

#define SWITCH_ACL_IPV4_ACL_OUT_PORTS_LABEL_POS 0
#define SWITCH_ACL_IPV4_ACL_OUT_PORTS_LABEL_WIDTH 8

#define SWITCH_ACL_IPV6_ACL_OUT_PORTS_LABEL_POS 8
#define SWITCH_ACL_IPV6_ACL_OUT_PORTS_LABEL_WIDTH 8

struct id_map_key_t {
  uint64_t acl_type;
  uint64_t direction;
  int acl_attr;
  bool operator<(const id_map_key_t &k) const {
    return (acl_type < k.acl_type) ||
           (acl_type == k.acl_type && direction < k.direction) ||
           (acl_type == k.acl_type && direction == k.direction &&
            acl_attr < k.acl_attr);
  }
};

class IdMap {
 private:
  static IdMap *instance_;
  std::map<id_map_key_t, bf_rt_id_t> acl_id_map;
  void set_id(uint64_t acl_type, uint64_t dir, int acl_attr, bf_rt_id_t id) {
    struct id_map_key_t key = {acl_type, dir, acl_attr};
    acl_id_map[key] = id;
  }
  void updateEgressAclIds();
  void updateIngressAclIds();
  void updateMacLabelAclIds();

 public:
  IdMap();
  static IdMap *instance() {
    if (instance_ != NULL) {
      return instance_;
    }
    instance_ = new IdMap;
    return instance_;
  }
  inline void Dummy(){};
  bf_rt_id_t get_id(uint64_t acl_type, uint64_t dir, int attr) {
    struct id_map_key_t key = {acl_type, dir, attr};
    return acl_id_map[key];
  }
};

class LabelManager {
 private:
  static LabelManager *instance_;
  std::unique_ptr<idAllocator[]> if_qos_labels_;
  std::unique_ptr<idAllocator[]> if_mac_labels_;
  std::unique_ptr<idAllocator[]> if_data_labels_;
  std::unique_ptr<idAllocator[]> if_racl_labels_;
  std::unique_ptr<idAllocator[]> if_mirror_labels_;
  std::unique_ptr<idAllocator[]> if_ipv4_labels_;
  std::unique_ptr<idAllocator[]> if_ipv6_labels_;
  std::unique_ptr<idAllocator[]> if_dtel_labels_;
  std::unique_ptr<idAllocator[]> if_tos_mirror_labels_;
  std::unique_ptr<idAllocator[]> bd_qos_labels_;
  std::unique_ptr<idAllocator[]> bd_data_labels_;
  std::unique_ptr<idAllocator[]> bd_racl_labels_;
  std::unique_ptr<idAllocator[]> bd_mirror_labels_;
  std::unique_ptr<idAllocator[]> bd_ipv4_labels_;
  std::unique_ptr<idAllocator[]> bd_ipv6_labels_;
  std::unique_ptr<idAllocator[]> bd_dtel_labels_;

 public:
  LabelManager();
  static LabelManager *instance() {
    if (instance_ != NULL) {
      return instance_;
    }
    instance_ = new LabelManager;
    return instance_;
  }
  uint32_t label_position(uint64_t acl_type, uint64_t dir, uint64_t bp_type);
  uint32_t label_width(uint64_t acl_type, uint64_t dir, uint64_t bp_type);
  void label_feature_value(switch_acl_label_t label_index,
                           uint64_t acl_type,
                           uint64_t dir,
                           uint64_t bp_type,
                           switch_acl_label_t &label_value,
                           switch_acl_label_t &label_mask);
  switch_status_t label_allocate(uint64_t acl_type,
                                 uint64_t dir,
                                 uint64_t bp_type,
                                 switch_acl_label_t &label_value,
                                 switch_acl_label_t &label_mask);
  switch_status_t label_release(uint64_t acl_type,
                                uint64_t dir,
                                uint64_t bp_type,
                                switch_acl_label_t label_value);
  switch_status_t label_reserve(uint64_t acl_type,
                                uint64_t dir,
                                uint64_t bp_type,
                                switch_acl_label_t label_value);
  bool label_supp_for_racl_mac_qos_acl();
};

#define SWITCH_ACL_PORT_GROUP_MAX 8

// 32-bit port_group index is split into 4 groups
// Bits 0-7    : IPV4 ACL
// Bits 8-15   : IPV6 ACL
// Bits 16-23  : Mirror ACL
// Bits 24-31  : MAC ACL

#define SWITCH_ACL_DATA_IPV4_ACL_PG_POS 0

#define SWITCH_ACL_DATA_IPV6_ACL_PG_POS 8

#define SWITCH_ACL_MIRROR_ACL_PG_POS 16

#define SWITCH_ACL_DATA_MAC_ACL_PG_POS 24

#define SWITCH_ACL_FEATURE_PORT_GROUP_VALUE(pg_index, pos) \
  (1 << (pos + pg_index - 1))

#define QOS_ACL_ACTION_PCP_DEFAULT_VALUE 127
#define QOS_ACL_ACTION_TOS_DEFAULT_VALUE 127

class PortGroupManager {
 private:
  static PortGroupManager *instance_;
  idAllocator *acl_port_group_[SWITCH_ACL_TABLE_ATTR_TYPE_MAX]
                              [SWITCH_ACL_TABLE_ATTR_DIRECTION_MAX];
  std::map<std::set<switch_object_id_t>, uint8_t>
      acl_port_group_map_[SWITCH_ACL_TABLE_ATTR_TYPE_MAX]
                         [SWITCH_ACL_TABLE_ATTR_DIRECTION_MAX];
  uint8_t acl_port_group_count_[SWITCH_ACL_TABLE_ATTR_TYPE_MAX]
                               [SWITCH_ACL_TABLE_ATTR_DIRECTION_MAX]
                               [SWITCH_ACL_PORT_GROUP_MAX];

 public:
  PortGroupManager();
  static PortGroupManager *instance() {
    if (instance_ != NULL) {
      return instance_;
    }
    instance_ = new PortGroupManager;
    return instance_;
  }
  switch_status_t port_group_allocate(
      const uint64_t acl_type,
      const uint64_t dir,
      const std::set<switch_object_id_t> port_handles_list,
      uint8_t &group_value);
  switch_status_t port_group_release(const uint64_t acl_type,
                                     const uint64_t dir,
                                     const uint8_t group_value);
  switch_status_t port_group_count(const uint64_t acl_type,
                                   const uint64_t dir,
                                   const uint8_t group_index,
                                   uint32_t &ref_count);
};

/**
 * @brief IN_PORTS and OUT_PORTS for acl entry
 */
class PortGroupLabel {
 private:
  static PortGroupLabel *instance_;
  idAllocator *acl_port_group_[SWITCH_ACL_TABLE_ATTR_TYPE_MAX]
                              [SWITCH_ACL_TABLE_ATTR_DIRECTION_MAX];
  std::map<std::set<switch_object_id_t>, uint8_t>
      acl_port_group_map_[SWITCH_ACL_TABLE_ATTR_TYPE_MAX]
                         [SWITCH_ACL_TABLE_ATTR_DIRECTION_MAX];
  uint8_t acl_port_group_count_[SWITCH_ACL_TABLE_ATTR_TYPE_MAX]
                               [SWITCH_ACL_TABLE_ATTR_DIRECTION_MAX]
                               [SWITCH_ACL_PORT_GROUP_MAX];

  /**
   * @brief Getting the port group label position
   *
   * @param[in] acl_type acl table type (switch_acl_table_attr_type)
   * @param[in] dir acl entry direction (switch_acl_table_attr_direction)
   *
   * @return bit position in 32bit scale
   */
  uint32_t label_position(uint64_t acl_type, uint64_t dir);

  /**
   * @brief Getting label width used for port group label mask
   *
   * @param[in] acl_type acl table type (switch_acl_table_attr_type)
   * @param[in] dir acl entry direction (switch_acl_table_attr_direction)
   *
   * @return bit position in 32bit scale
   */
  uint32_t label_width(uint64_t acl_type, uint64_t dir);

 public:
  /**
   * @brief Create the port group label single object
   *
   * @param[in] None
   *
   * @return None
   */
  PortGroupLabel();
  static PortGroupLabel *instance() {
    if (instance_ != NULL) {
      return instance_;
    }
    instance_ = new PortGroupLabel;
    return instance_;
  }

  /**
   * @brief Allocating the port group index beased on acl table type and
   *direction
   *
   * @param[in] acl_type acl table type (switch_acl_table_attr_type)
   * @param[in] dir acl entry direction (switch_acl_table_attr_direction)
   * @param[in] port_handle_list list of the port handles
   * @param[out] group_index created max 8bits
   *
   * @return #SWITCH_STATUS_SUCCESS on success, failure status code on error
   */
  switch_status_t label_allocate(
      const uint64_t acl_type,
      const uint64_t dir,
      const std::set<switch_object_id_t> &port_handles_list,
      uint8_t &group_index);

  /**
   * @brief Releasing the port group index beased on acl table type and
   * direction
   *
   * @param[in] acl_type acl table type (switch_acl_table_attr_type)
   * @param[in] dir acl entry direction (switch_acl_table_attr_direction)
   * @param[in] group_index created group index
   *
   * @return #SWITCH_STATUS_SUCCESS on success, failure status code on error
   */
  switch_status_t label_release(const uint64_t acl_type,
                                const uint64_t dir,
                                const uint8_t group_index);

  /**
   * @brief  Count the port group index beased on acl table type and direction
   *
   * @param[in] acl_type acl table type (switch_acl_table_attr_type)
   * @param[in] dir acl entry direction (switch_acl_table_attr_direction)
   * @param[in] group_index created group index
   *
   * @return unit32_t ref_count number of entries
   */
  uint32_t label_count(const uint64_t acl_type,
                       const uint64_t dir,
                       const uint8_t group_index);

  /**
   * @brief  Update count the port group index beased on acl table type and
   *direction
   *
   * @param[in] acl_type acl table type (switch_acl_table_attr_type)
   * @param[in] dir acl entry direction (switch_acl_table_attr_direction)
   * @param[in] group_index created in out group index
   * @param[in] is_reduce should be false if we want to increase the count
   *
   * @return uint32_t updated count value
   */
  uint32_t label_count_update(const uint64_t acl_type,
                              const uint64_t dir,
                              const uint8_t group_index,
                              bool is_reduce = true);
  /**
   * @brief calculate port group label and respective mask
   *
   * @param[in] group_index previously generated inout group index
   * @param[in] acl_type acl table type (switch_acl_table_attr_type)
   * @param[in] dir acl entry direction (switch_acl_table_attr_direction)
   * @param[out] label_value calculated label value
   *
   * @return None
   */
  void get_label_value(uint8_t group_index,
                       uint64_t acl_type,
                       uint64_t dir,
                       uint32_t &label_value);
};

// Map {port_lag_handle, smac, smac_mask, dmac, dmac_mask}
//                 -> {mac_label, refcount}
class MacAddrLabel {
 private:
  idAllocator idalloc;
  struct macaddr_compress_key_t {
    switch_object_id_t port_lag_handle;
    switch_mac_addr_t src_mac;
    switch_mac_addr_t src_mac_mask;
    switch_mac_addr_t dst_mac;
    switch_mac_addr_t dst_mac_mask;

    bool operator==(const macaddr_compress_key_t &other) const {
      return port_lag_handle == other.port_lag_handle &&
             src_mac == other.src_mac && src_mac_mask == other.src_mac_mask &&
             dst_mac == other.dst_mac && dst_mac_mask == other.dst_mac_mask;
    }
  };
  struct port_mac_hash {
    std::size_t operator()(const macaddr_compress_key_t &k) const {
      std::size_t seed = 0;
      hash_combine(seed, k.port_lag_handle);
      hash_combine(seed, k.src_mac);
      hash_combine(seed, k.src_mac_mask);
      hash_combine(seed, k.dst_mac);
      hash_combine(seed, k.dst_mac_mask);

      return seed;
    }
  };
  std::unordered_map<macaddr_compress_key_t,
                     std::pair<uint8_t, uint16_t>,
                     port_mac_hash>
      macaddr_compr_map;

 public:
  switch_status_t is_empty() { return macaddr_compr_map.empty(); }
  switch_status_t allocate(switch_object_id_t port_lag_handle,
                           switch_mac_addr_t src_mac,
                           switch_mac_addr_t src_mac_mask,
                           switch_mac_addr_t dst_mac,
                           switch_mac_addr_t dst_mac_mask,
                           uint8_t &macaddr_label);
  switch_status_t reserve(switch_object_id_t port_lag_handle,
                          switch_mac_addr_t src_mac,
                          switch_mac_addr_t src_mac_mask,
                          switch_mac_addr_t dst_mac,
                          switch_mac_addr_t dst_mac_mask,
                          uint8_t macaddr_label);
  switch_status_t release(switch_object_id_t port_lag_handle,
                          switch_mac_addr_t src_mac,
                          switch_mac_addr_t src_mac_mask,
                          switch_mac_addr_t dst_mac,
                          switch_mac_addr_t dst_mac_mask,
                          uint8_t macaddr_label);
  switch_status_t get_refcount(switch_object_id_t port_lag_handle,
                               switch_mac_addr_t src_mac,
                               switch_mac_addr_t src_mac_mask,
                               switch_mac_addr_t dst_mac,
                               switch_mac_addr_t dst_mac_mask,
                               uint16_t &refcount);
};

// Map of <acl_type, direction> -> MacAddrLabel
class MacAddrLabelManager {
 private:
  static MacAddrLabelManager *instance_;
  struct type_dir_hash {
    std::size_t operator()(const std::pair<uint64_t, uint64_t> &p) const {
      std::size_t seed = 0;
      hash_combine(seed, p.first);
      hash_combine(seed, p.second);
      return seed;
    }
  };
  std::unordered_map<std::pair<uint64_t, uint64_t>,
                     MacAddrLabel *,
                     type_dir_hash>
      mac_addr_labels;

 public:
  static MacAddrLabelManager *instance() {
    if (instance_ == NULL) {
      instance_ = new MacAddrLabelManager;
    }
    return instance_;
  }
  switch_status_t allocate(uint64_t acl_type,
                           uint64_t acl_dir,
                           switch_object_id_t port_lag_handle,
                           switch_mac_addr_t src_mac,
                           switch_mac_addr_t src_mac_mask,
                           switch_mac_addr_t dst_mac,
                           switch_mac_addr_t dst_mac_mask,
                           uint8_t &macaddr_label);
  switch_status_t reserve(uint64_t acl_type,
                          uint64_t acl_dir,
                          switch_object_id_t port_lag_handle,
                          switch_mac_addr_t src_mac,
                          switch_mac_addr_t src_mac_mask,
                          switch_mac_addr_t dst_mac,
                          switch_mac_addr_t dst_mac_mask,
                          uint8_t macaddr_label);
  switch_status_t release(uint64_t acl_type,
                          uint64_t acl_dir,
                          switch_object_id_t port_lag_handle,
                          switch_mac_addr_t src_mac,
                          switch_mac_addr_t src_mac_mask,
                          switch_mac_addr_t dst_mac,
                          switch_mac_addr_t dst_mac_mask,
                          uint8_t macaddr_label);
  switch_status_t get_refcount(uint64_t acl_type,
                               uint64_t acl_dir,
                               switch_object_id_t port_lag_handle,
                               switch_mac_addr_t src_mac,
                               switch_mac_addr_t src_mac_mask,
                               switch_mac_addr_t dst_mac,
                               switch_mac_addr_t dst_mac_mask,
                               uint16_t &refcount);
};

}  // namespace smi

#endif /*__SMI_ACL_H__ */
