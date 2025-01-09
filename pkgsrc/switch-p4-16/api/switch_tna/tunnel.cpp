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


#include <set>
#include <vector>
#include <memory>
#include <utility>

#include "switch_tna/utils.h"
#include "switch_tna/p4_16_types.h"

namespace smi {
using namespace smi::bf_rt;  // NOLINT(build/namespaces)
using ::smi::logging::switch_log;

switch_tunnel_mode_t switch_tunnel_decap_ttl_mode_to_tunnel_mode(
    switch_enum_t switch_tunnel_mode) {
  switch (switch_tunnel_mode.enumdata) {
    case SWITCH_TUNNEL_ATTR_DECAP_TTL_MODE_UNIFORM_MODEL:
      return SWITCH_TUNNEL_MODE_UNIFORM;
      break;
    case SWITCH_TUNNEL_ATTR_DECAP_TTL_MODE_PIPE_MODEL:
      return SWITCH_TUNNEL_MODE_PIPE;
      break;
    default:
      return SWITCH_TUNNEL_MODE_UNIFORM;
  }
}

switch_tunnel_mode_t switch_tunnel_decap_ecn_mode_to_tunnel_mode(
    switch_enum_t switch_tunnel_mode) {
  switch (switch_tunnel_mode.enumdata) {
    case SWITCH_TUNNEL_ATTR_DECAP_ECN_MODE_STANDARD:
      return SWITCH_ECN_MODE_STANDARD;
      break;
    case SWITCH_TUNNEL_ATTR_DECAP_ECN_MODE_COPY_FROM_OUTER:
      return SWITCH_ECN_MODE_COPY_FROM_OUTER;
      break;
    default:
      return SWITCH_ECN_MODE_COPY_FROM_OUTER;
  }
}

switch_tunnel_mode_t switch_tunnel_decap_qos_mode_to_tunnel_mode(
    switch_enum_t switch_tunnel_mode) {
  switch (switch_tunnel_mode.enumdata) {
    case SWITCH_TUNNEL_ATTR_DECAP_QOS_MODE_UNIFORM_MODEL:
      return SWITCH_TUNNEL_MODE_UNIFORM;
      break;
    case SWITCH_TUNNEL_ATTR_DECAP_QOS_MODE_PIPE_MODEL:
      return SWITCH_TUNNEL_MODE_PIPE;
      break;
    default:
      return SWITCH_TUNNEL_MODE_UNIFORM;
  }
}

class ipv4_src_vtep : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_IPV4_SRC_VTEP;
  static const switch_attr_id_t status_attr_id =
      SWITCH_IPV4_SRC_VTEP_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_IPV4_SRC_VTEP_ATTR_PARENT_HANDLE;

 public:
  ipv4_src_vtep(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_IPV4_SRC_VTEP,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_object_id_t tunnel_handle = {}, vrf_handle = {};
    switch_ip_address_t ipaddr;

    status |= switch_store::v_get(
        parent, SWITCH_TUNNEL_TERM_ATTR_TUNNEL_HANDLE, tunnel_handle);
    status |=
        switch_store::v_get(parent, SWITCH_TUNNEL_TERM_ATTR_SRC_IP, ipaddr);

    if (ipaddr.addr_family != SWITCH_IP_ADDR_FAMILY_IPV4) return;

    status |= switch_store::v_get(
        parent, SWITCH_TUNNEL_TERM_ATTR_VRF_HANDLE, vrf_handle);
    status |= match_key.set_exact(smi_id::F_IPV4_SRC_VTEP_LOCAL_MD_VRF,
                                  compute_vrf(vrf_handle));
    status |= match_key.set_exact(smi_id::F_IPV4_SRC_VTEP_SRC_ADDR,
                                  tunnel_handle,
                                  SWITCH_TUNNEL_ATTR_SRC_IP);
    status |= match_key.set_exact(smi_id::F_IPV4_SRC_VTEP_LOCAL_MD_TUNNEL_TYPE,
                                  parent,
                                  SWITCH_TUNNEL_TERM_ATTR_TYPE);
    action_entry.init_action_data(smi_id::A_IPV4_SRC_VTEP_HIT);
    status |= action_entry.set_arg(smi_id::P_IPV4_SRC_VTEP_HIT_IFINDEX,
                                   static_cast<uint16_t>(3));
  }
};

class ipv6_src_vtep : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_IPV6_SRC_VTEP;
  static const switch_attr_id_t status_attr_id =
      SWITCH_IPV6_SRC_VTEP_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_IPV6_SRC_VTEP_ATTR_PARENT_HANDLE;

 public:
  ipv6_src_vtep(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_IPV6_SRC_VTEP,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_object_id_t tunnel_handle = {}, vrf_handle = {};
    switch_ip_address_t ipaddr;

    status |= switch_store::v_get(
        parent, SWITCH_TUNNEL_TERM_ATTR_TUNNEL_HANDLE, tunnel_handle);
    status |=
        switch_store::v_get(parent, SWITCH_TUNNEL_TERM_ATTR_SRC_IP, ipaddr);

    if (ipaddr.addr_family != SWITCH_IP_ADDR_FAMILY_IPV6) return;

    status |= switch_store::v_get(
        parent, SWITCH_TUNNEL_TERM_ATTR_VRF_HANDLE, vrf_handle);
    status |= match_key.set_exact(smi_id::F_IPV6_SRC_VTEP_LOCAL_MD_VRF,
                                  compute_vrf(vrf_handle));
    status |= match_key.set_exact(smi_id::F_IPV6_SRC_VTEP_SRC_ADDR,
                                  parent,
                                  SWITCH_TUNNEL_TERM_ATTR_SRC_IP);
    status |= match_key.set_exact(smi_id::F_IPV6_SRC_VTEP_LOCAL_MD_TUNNEL_TYPE,
                                  parent,
                                  SWITCH_TUNNEL_TERM_ATTR_TYPE);
    action_entry.init_action_data(smi_id::A_IPV6_SRC_VTEP_HIT);
    status |=
        action_entry.set_arg<uint16_t>(smi_id::P_IPV6_SRC_VTEP_HIT_IFINDEX, 0);
  }
};

class ipv4_dst_vtep : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_IPV4_DST_VTEP;
  static const switch_attr_id_t status_attr_id =
      SWITCH_IPV4_DST_VTEP_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_IPV4_DST_VTEP_ATTR_PARENT_HANDLE;

 public:
  ipv4_dst_vtep(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_IPV4_DST_VTEP,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_object_id_t tunnel_handle = {}, vrf_handle = {};
    switch_ip_address_t src_ipaddr, dst_ipaddr;
    switch_ip_address_t mask = {};
    mask.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
    mask.ip4 = 0xFFFFFFFF;
    switch_enum_t term_type = {0}, tunnel_type = {0}, ttl_mode = {0},
                  qos_mode = {0}, ecn_mode = {0};

    status |=
        switch_store::v_get(parent, SWITCH_TUNNEL_TERM_ATTR_TYPE, tunnel_type);
    status |= switch_store::v_get(
        parent, SWITCH_TUNNEL_TERM_ATTR_TERMINATION_TYPE, term_type);
    status |= switch_store::v_get(
        parent, SWITCH_TUNNEL_TERM_ATTR_TUNNEL_HANDLE, tunnel_handle);
    status |=
        switch_store::v_get(parent, SWITCH_TUNNEL_TERM_ATTR_DST_IP, dst_ipaddr);
    status |=
        switch_store::v_get(parent, SWITCH_TUNNEL_TERM_ATTR_SRC_IP, src_ipaddr);

    if (dst_ipaddr.addr_family != SWITCH_IP_ADDR_FAMILY_IPV4) return;

    if (term_type.enumdata == SWITCH_TUNNEL_TERM_ATTR_TERMINATION_TYPE_P2P) {
      status |= match_key.set_ternary(
          smi_id::F_IPV4_DST_VTEP_SRC_ADDR, src_ipaddr, mask);
    }
    status |= match_key.set_ternary(
        smi_id::F_IPV4_DST_VTEP_DST_ADDR, dst_ipaddr, mask);
    status |= switch_store::v_get(
        parent, SWITCH_TUNNEL_TERM_ATTR_VRF_HANDLE, vrf_handle);
    status |= match_key.set_exact(smi_id::F_IPV4_DST_VTEP_LOCAL_MD_VRF,
                                  compute_vrf(vrf_handle));
    status |= match_key.set_exact(smi_id::F_IPV4_DST_VTEP_LOCAL_MD_TUNNEL_TYPE,
                                  parent,
                                  SWITCH_TUNNEL_TERM_ATTR_TYPE);

    status |= switch_store::v_get(
        tunnel_handle, SWITCH_TUNNEL_ATTR_DECAP_TTL_MODE, ttl_mode);
    status |= switch_store::v_get(
        tunnel_handle, SWITCH_TUNNEL_ATTR_DECAP_ECN_MODE, ecn_mode);
    status |= switch_store::v_get(
        tunnel_handle, SWITCH_TUNNEL_ATTR_DECAP_QOS_MODE, qos_mode);

    if (tunnel_type.enumdata == SWITCH_TUNNEL_TERM_ATTR_TYPE_VXLAN) {
      action_entry.init_action_data(smi_id::A_IPV4_DST_VTEP_HIT);
      if (feature::is_feature_set(SWITCH_FEATURE_TUNNEL_TTL_MODE)) {
        status |= action_entry.set_arg(
            smi_id::P_IPV4_DST_VTEP_HIT_TTL_MODE,
            switch_tunnel_decap_ttl_mode_to_tunnel_mode(ttl_mode));
      }
      if (feature::is_feature_set(SWITCH_FEATURE_TUNNEL_QOS_MODE)) {
        status |= action_entry.set_arg(
            smi_id::P_IPV4_DST_VTEP_HIT_QOS_MODE,
            switch_tunnel_decap_qos_mode_to_tunnel_mode(qos_mode));
      }
    } else if ((tunnel_type.enumdata == SWITCH_TUNNEL_TERM_ATTR_TYPE_IPIP) ||
               (tunnel_type.enumdata == SWITCH_TUNNEL_TERM_ATTR_TYPE_IPGRE)) {
      switch_enum_t vrf_ttl_action, vrf_ip_options_action;
      switch_packet_action_t sw_packet_action = 0;
      bool sw_packet_action_is_valid = false;
      switch_object_id_t orif_handle = {}, ovrf_handle = {}, bd_handle = {};
      // Find overlay rif handle and vrf from there
      status |= switch_store::v_get(
          tunnel_handle, SWITCH_TUNNEL_ATTR_OVERLAY_RIF_HANDLE, orif_handle);
      if (orif_handle.data == 0) {
        action_entry.init_action_data(smi_id::A_IPV4_DST_VTEP_HIT);
        return;
      }
      status |= switch_store::v_get(
          orif_handle, SWITCH_RIF_ATTR_VRF_HANDLE, ovrf_handle);
      status |= find_auto_oid(ovrf_handle, SWITCH_OBJECT_TYPE_BD, bd_handle);
      status |= switch_store::v_get(
          ovrf_handle, SWITCH_VRF_ATTR_TTL_ACTION, vrf_ttl_action);
      status |= switch_store::v_get(ovrf_handle,
                                    SWITCH_VRF_ATTR_IP_OPTIONS_ACTION,
                                    vrf_ip_options_action);
      action_entry.init_action_data(
          smi_id::A_IPV4_DST_VTEP_SET_INNER_BD_PROPERTIES);
      status |= action_entry.set_arg(smi_id::P_IPV4_DST_VTEP_SET_PROPERTIES_BD,
                                     compute_bd(bd_handle));
      status |= action_entry.set_arg(smi_id::P_IPV4_DST_VTEP_SET_PROPERTIES_VRF,
                                     compute_vrf(ovrf_handle));
      status |= action_entry.set_arg(
          smi_id::P_IPV4_DST_VTEP_SET_PROPERTIES_IPV4_UNICAST_ENABLE,
          ovrf_handle,
          SWITCH_VRF_ATTR_IPV4_UNICAST);
      status |= action_entry.set_arg(
          smi_id::P_IPV4_DST_VTEP_SET_PROPERTIES_IPV6_UNICAST_ENABLE,
          ovrf_handle,
          SWITCH_VRF_ATTR_IPV6_UNICAST);
      status |= vrf_ttl_action_to_switch_packet_action(
          vrf_ttl_action, sw_packet_action, sw_packet_action_is_valid);
      if (sw_packet_action_is_valid) {
        status |= action_entry.set_arg(
            smi_id::P_IPV4_DST_VTEP_SET_PROPERTIES_VRF_TTL_VIOLATION,
            sw_packet_action);
        status |= action_entry.set_arg(
            smi_id::P_IPV4_DST_VTEP_SET_PROPERTIES_VRF_TTL_VIOLATION_VALID,
            true);
      }
      status |= vrf_ip_options_action_to_switch_packet_action(
          vrf_ip_options_action, sw_packet_action);
      status |= action_entry.set_arg(
          smi_id::P_IPV4_DST_VTEP_SET_PROPERTIES_VRF_IP_OPTIONS_VIOLATION,
          sw_packet_action);
      if (feature::is_feature_set(SWITCH_FEATURE_TUNNEL_TTL_MODE)) {
        status |= action_entry.set_arg(
            smi_id::P_IPV4_DST_VTEP_SET_PROPERTIES_TTL_MODE,
            switch_tunnel_decap_ttl_mode_to_tunnel_mode(ttl_mode));
      }
      if (feature::is_feature_set(SWITCH_FEATURE_TUNNEL_QOS_MODE)) {
        status |= action_entry.set_arg(
            smi_id::P_IPV4_DST_VTEP_SET_PROPERTIES_QOS_MODE,
            switch_tunnel_decap_qos_mode_to_tunnel_mode(qos_mode));
      }
      if (feature::is_feature_set(SWITCH_FEATURE_TUNNEL_ECN_RFC_6040)) {
        status |= action_entry.set_arg(
            smi_id::P_IPV4_DST_VTEP_SET_PROPERTIES_ECN_MODE,
            switch_tunnel_decap_ecn_mode_to_tunnel_mode(ecn_mode));
      }
    }
  }
};

class ipv6_dst_vtep : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_IPV6_DST_VTEP;
  static const switch_attr_id_t status_attr_id =
      SWITCH_IPV6_DST_VTEP_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_IPV6_DST_VTEP_ATTR_PARENT_HANDLE;

 public:
  ipv6_dst_vtep(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_IPV6_DST_VTEP,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_object_id_t tunnel_handle = {}, vrf_handle = {};
    switch_ip_address_t src_ipaddr, dst_ipaddr;
    switch_ip_address_t mask = {};
    memset(&mask, 0xFF, sizeof(switch_ip_address_t));
    mask.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6;
    switch_enum_t term_type = {0}, tunnel_type = {0}, ttl_mode = {0},
                  qos_mode = {0}, ecn_mode = {0};

    status |=
        switch_store::v_get(parent, SWITCH_TUNNEL_TERM_ATTR_TYPE, tunnel_type);
    status |= switch_store::v_get(
        parent, SWITCH_TUNNEL_TERM_ATTR_TERMINATION_TYPE, term_type);
    status |= switch_store::v_get(
        parent, SWITCH_TUNNEL_TERM_ATTR_TUNNEL_HANDLE, tunnel_handle);
    status |=
        switch_store::v_get(parent, SWITCH_TUNNEL_TERM_ATTR_DST_IP, dst_ipaddr);
    status |=
        switch_store::v_get(parent, SWITCH_TUNNEL_TERM_ATTR_SRC_IP, src_ipaddr);

    if (dst_ipaddr.addr_family != SWITCH_IP_ADDR_FAMILY_IPV6) return;

    if (term_type.enumdata == SWITCH_TUNNEL_TERM_ATTR_TERMINATION_TYPE_P2P) {
      status |= match_key.set_ternary(
          smi_id::F_IPV6_DST_VTEP_SRC_ADDR, src_ipaddr, mask);
    }
    status |= match_key.set_ternary(
        smi_id::F_IPV6_DST_VTEP_DST_ADDR, dst_ipaddr, mask);
    status |= switch_store::v_get(
        parent, SWITCH_TUNNEL_TERM_ATTR_VRF_HANDLE, vrf_handle);
    status |= match_key.set_exact(smi_id::F_IPV6_DST_VTEP_LOCAL_MD_VRF,
                                  compute_vrf(vrf_handle));
    status |= match_key.set_exact(smi_id::F_IPV6_DST_VTEP_LOCAL_MD_TUNNEL_TYPE,
                                  parent,
                                  SWITCH_TUNNEL_TERM_ATTR_TYPE);

    status |= switch_store::v_get(
        tunnel_handle, SWITCH_TUNNEL_ATTR_DECAP_TTL_MODE, ttl_mode);
    status |= switch_store::v_get(
        tunnel_handle, SWITCH_TUNNEL_ATTR_DECAP_ECN_MODE, ecn_mode);
    status |= switch_store::v_get(
        tunnel_handle, SWITCH_TUNNEL_ATTR_DECAP_QOS_MODE, qos_mode);

    if (tunnel_type.enumdata == SWITCH_TUNNEL_TERM_ATTR_TYPE_VXLAN) {
      action_entry.init_action_data(smi_id::A_IPV6_DST_VTEP_HIT);
      if (feature::is_feature_set(SWITCH_FEATURE_TUNNEL_TTL_MODE)) {
        status |= action_entry.set_arg(
            smi_id::P_IPV6_DST_VTEP_HIT_TTL_MODE,
            switch_tunnel_decap_ttl_mode_to_tunnel_mode(ttl_mode));
      }
      if (feature::is_feature_set(SWITCH_FEATURE_TUNNEL_QOS_MODE)) {
        status |= action_entry.set_arg(
            smi_id::P_IPV6_DST_VTEP_HIT_QOS_MODE,
            switch_tunnel_decap_qos_mode_to_tunnel_mode(qos_mode));
      }
    } else if ((tunnel_type.enumdata == SWITCH_TUNNEL_TERM_ATTR_TYPE_IPIP) ||
               (tunnel_type.enumdata == SWITCH_TUNNEL_TERM_ATTR_TYPE_IPGRE)) {
      switch_object_id_t orif_handle = {}, ovrf_handle = {}, bd_handle = {};
      // Find overlay rif handle and vrf from there
      status |= switch_store::v_get(
          tunnel_handle, SWITCH_TUNNEL_ATTR_OVERLAY_RIF_HANDLE, orif_handle);
      if (orif_handle.data == 0) {
        action_entry.init_action_data(smi_id::A_IPV6_DST_VTEP_HIT);
        return;
      }
      status |= switch_store::v_get(
          orif_handle, SWITCH_RIF_ATTR_VRF_HANDLE, ovrf_handle);
      status |= find_auto_oid(ovrf_handle, SWITCH_OBJECT_TYPE_BD, bd_handle);

      action_entry.init_action_data(
          smi_id::A_IPV6_DST_VTEP_SET_INNER_BD_PROPERTIES);
      status |= action_entry.set_arg(smi_id::P_IPV6_DST_VTEP_SET_PROPERTIES_BD,
                                     compute_bd(bd_handle));
      status |= action_entry.set_arg(smi_id::P_IPV6_DST_VTEP_SET_PROPERTIES_VRF,
                                     compute_vrf(ovrf_handle));
      status |= action_entry.set_arg(
          smi_id::P_IPV6_DST_VTEP_SET_PROPERTIES_IPV4_UNICAST_ENABLE,
          ovrf_handle,
          SWITCH_VRF_ATTR_IPV4_UNICAST);
      status |= action_entry.set_arg(
          smi_id::P_IPV6_DST_VTEP_SET_PROPERTIES_IPV6_UNICAST_ENABLE,
          ovrf_handle,
          SWITCH_VRF_ATTR_IPV6_UNICAST);
      if (feature::is_feature_set(SWITCH_FEATURE_TUNNEL_TTL_MODE)) {
        status |= action_entry.set_arg(
            smi_id::P_IPV6_DST_VTEP_SET_PROPERTIES_TTL_MODE,
            switch_tunnel_decap_ttl_mode_to_tunnel_mode(ttl_mode));
      }
      if (feature::is_feature_set(SWITCH_FEATURE_TUNNEL_QOS_MODE)) {
        status |= action_entry.set_arg(
            smi_id::P_IPV6_DST_VTEP_SET_PROPERTIES_QOS_MODE,
            switch_tunnel_decap_qos_mode_to_tunnel_mode(qos_mode));
      }
      if (feature::is_feature_set(SWITCH_FEATURE_TUNNEL_ECN_RFC_6040)) {
        status |= action_entry.set_arg(
            smi_id::P_IPV6_DST_VTEP_SET_PROPERTIES_ECN_MODE,
            switch_tunnel_decap_ecn_mode_to_tunnel_mode(ecn_mode));
      }
    }
  }
};

class vni_to_bd_mapping : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_VNI_TO_BD_MAPPING;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_VNI_TO_BD_MAPPING_ATTR_PARENT_HANDLE;
  switch_status_t find_overlapping_vrf_entry(
      switch_object_id_t &vrf_tunnel_mapper_entry_handle);
  switch_status_t find_overlapping_vlan_entry(
      switch_object_id_t &vrf_tunnel_mapper_entry_handle);
  switch_enum_t type = {0};
  switch_object_id_t vrf_handle = {0};
  uint32_t vni = 0;

  _MatchKey match_key;
  _ActionEntry action_entry;

 public:
  vni_to_bd_mapping(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent),
        match_key(smi_id::T_VNI_TO_BD_MAPPING),
        action_entry(smi_id::T_VNI_TO_BD_MAPPING) {
    switch_object_id_t network_handle = {0}, bd_handle = {0}, rif_handle = {0};
    switch_object_type_t ot = 0;
    status |=
        switch_store::v_get(parent, SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TYPE, type);
    if (!(type.enumdata ==
              SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TYPE_VNI_TO_VRF_HANDLE ||
          type.enumdata ==
              SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TYPE_VNI_TO_VLAN_HANDLE ||
          type.enumdata ==
              SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TYPE_VNI_TO_BRIDGE_HANDLE)) {
      return;
    }

    status |= switch_store::v_get(
        parent, SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_NETWORK_HANDLE, network_handle);
    status |= switch_store::v_get(
        parent, SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TUNNEL_VNI, vni);
    status |= find_auto_oid(network_handle, SWITCH_OBJECT_TYPE_BD, bd_handle);

    status |= match_key.set_exact(
        smi_id::F_VNI_TO_BD_MAPPING_LOCAL_MD_TUNNEL_VNI, vni);

    action_entry.init_action_data(
        smi_id::A_VNI_TO_BD_MAPPING_SET_INNER_BD_PROPERTIES);
    status |= action_entry.set_arg(smi_id::P_VNI_SET_PROPERTIES_BD,
                                   compute_bd(bd_handle));
    ot = switch_store::object_type_query(network_handle);
    if (ot == SWITCH_OBJECT_TYPE_VRF) {
      vrf_handle = network_handle;
    } else if (ot == SWITCH_OBJECT_TYPE_VLAN) {
      std::vector<switch_object_id_t> rif_handles;
      status |= switch_store::v_get(
          network_handle, SWITCH_VLAN_ATTR_RIF_HANDLES, rif_handles);
      if (rif_handles.size() > 0) {
        rif_handle = rif_handles[0];
        status |= switch_store::v_get(
            rif_handle, SWITCH_RIF_ATTR_VRF_HANDLE, vrf_handle);
      }
    }
    if (vrf_handle.data != 0) {
      switch_enum_t vrf_ttl_action, vrf_ip_options_action;
      switch_packet_action_t sw_packet_action = 0;
      bool sw_packet_action_is_valid = false;
      status |= action_entry.set_arg(smi_id::P_VNI_SET_PROPERTIES_VRF,
                                     compute_vrf(vrf_handle));
      status |= switch_store::v_get(
          vrf_handle, SWITCH_VRF_ATTR_TTL_ACTION, vrf_ttl_action);
      status |= switch_store::v_get(
          vrf_handle, SWITCH_VRF_ATTR_IP_OPTIONS_ACTION, vrf_ip_options_action);
      status |=
          action_entry.set_arg(smi_id::P_VNI_SET_PROPERTIES_IPV4_UNICAST_ENABLE,
                               vrf_handle,
                               SWITCH_VRF_ATTR_IPV4_UNICAST);
      status |=
          action_entry.set_arg(smi_id::P_VNI_SET_PROPERTIES_IPV6_UNICAST_ENABLE,
                               vrf_handle,
                               SWITCH_VRF_ATTR_IPV6_UNICAST);

      status |= vrf_ttl_action_to_switch_packet_action(
          vrf_ttl_action, sw_packet_action, sw_packet_action_is_valid);
      if (sw_packet_action_is_valid) {
        status |= action_entry.set_arg(
            smi_id::P_VNI_SET_PROPERTIES_VRF_TTL_VIOLATION, sw_packet_action);
        status |= action_entry.set_arg(
            smi_id::P_VNI_SET_PROPERTIES_VRF_TTL_VIOLATION_VALID, true);
      }
      status |= vrf_ip_options_action_to_switch_packet_action(
          vrf_ip_options_action, sw_packet_action);
      status |= action_entry.set_arg(
          smi_id::P_VNI_SET_PROPERTIES_VRF_IP_OPTIONS_VIOLATION,
          sw_packet_action);
    }
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_VNI_TO_BD_MAPPING);
    bool bf_rt_status = false;
    bool add = (get_auto_oid() == 0 ||
                switch_store::smiContext::context().in_warm_init());

    if (!(type.enumdata ==
              SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TYPE_VNI_TO_VRF_HANDLE ||
          type.enumdata ==
              SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TYPE_VNI_TO_VLAN_HANDLE ||
          type.enumdata ==
              SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TYPE_VNI_TO_BRIDGE_HANDLE)) {
      return status;
    }

    /* Due to SONiC L3 EVPN implementation's use of overlapping
     * tunnel_mapper_entries, one for vrf and another for a corresponding
     * dummy vlan, allow for two entries with the same vni to co-exist.
     *
     * When only one entry for a particular vni is present, that entry is
     * programmed using the values in the corresponding tunnel_mapper_entry.
     * When two entries are present, then the vrf entry is preferred.
     *
     * New entry type | Existing entry type  | Behavior
     * vlan           | none                 | hw add
     * vlan           | vrf (same as in rif) | hw noop since vrf is preferred
     * vlan           | other                | new will be rejected during
     *                |                      | hw add since entry already exists
     * vrf            | none                 | hw add
     * vrf            | vlan (w rif in vrf)  | hw modify to vrf's values
     * vlan           | other                | new will be rejected during
     *                |                      | hw add since entry already exists
     */

    // Check for overlapping vrf tunnel mapper entries with the same vni
    if (type.enumdata ==
            SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TYPE_VNI_TO_VLAN_HANDLE &&
        vrf_handle.data != 0) {
      switch_object_id_t vrf_tunnel_mapper_entry_handle = {0};
      status |= find_overlapping_vrf_entry(vrf_tunnel_mapper_entry_handle);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_TUNNEL_MAPPER_ENTRY,
                   "{}.{}:{}: Error while trying to find overlapping "
                   "tunnel_mapper_entries for tunnel_mapper_entry {:#x} :{}",
                   "vni_to_bd_mapping",
                   __func__,
                   __LINE__,
                   (get_parent()).data,
                   status);
        return status;
      }
      if (vrf_tunnel_mapper_entry_handle.data != 0) {
        // Prefer vrf entry, presumably already programmed,
        // so don't program in hw
        switch_log(SWITCH_API_LEVEL_DEBUG,
                   SWITCH_OBJECT_TYPE_TUNNEL_MAPPER_ENTRY,
                   "{}.{}:{}: Not programming tunnel_mapper_entry {:#x} in hw "
                   "due to overlapping tunnel_mapper_entry {:#x} :{}",
                   "vni_to_bd_mapping",
                   __func__,
                   __LINE__,
                   (get_parent()).data,
                   vrf_tunnel_mapper_entry_handle.data,
                   status);
        status |= auto_object::create_update();
        return status;
      }
    } else if (type.enumdata ==
               SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TYPE_VNI_TO_VRF_HANDLE) {
      switch_object_id_t vlan_tunnel_mapper_entry_handle = {0};
      status |= find_overlapping_vlan_entry(vlan_tunnel_mapper_entry_handle);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_TUNNEL_MAPPER_ENTRY,
                   "{}.{}:{}: Error while trying to find overlapping "
                   "tunnel_mapper_entries for tunnel_mapper_entry {:#x} :{}",
                   "vni_to_bd_mapping",
                   __func__,
                   __LINE__,
                   (get_parent()).data,
                   status);
        return status;
      }
      if (vlan_tunnel_mapper_entry_handle.data != 0) {
        // Check that vlan has a rif in the same vrf, otherwise reject
        switch_object_id_t vlan_handle = {0}, rif_handle_for_vlan = {0},
                           vrf_handle_for_vlan = {0};
        status |=
            switch_store::v_get(vlan_tunnel_mapper_entry_handle,
                                SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_NETWORK_HANDLE,
                                vlan_handle);
        std::vector<switch_object_id_t> rif_handles;
        status |= switch_store::v_get(
            vlan_handle, SWITCH_VLAN_ATTR_RIF_HANDLES, rif_handles);
        if (rif_handles.size() > 0) {
          rif_handle_for_vlan = rif_handles[0];
          status |= switch_store::v_get(rif_handle_for_vlan,
                                        SWITCH_RIF_ATTR_VRF_HANDLE,
                                        vrf_handle_for_vlan);
        }
        if (vrf_handle != vrf_handle_for_vlan) {
          status |= SWITCH_STATUS_ITEM_ALREADY_EXISTS;
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_TUNNEL_MAPPER_ENTRY,
                     "{}.{}:{}: Failed to create tunnel_mapper_entry {:#x} "
                     "due to overlapping tunnel_mapper_entry {:#x} :{}",
                     "vrf_to_vni_mapping",
                     __func__,
                     __LINE__,
                     (get_parent()).data,
                     vlan_tunnel_mapper_entry_handle.data,
                     status);
          return status;
        }
        // Overwrite vlan entry since vrf entry is preferred
        switch_log(SWITCH_API_LEVEL_DEBUG,
                   SWITCH_OBJECT_TYPE_TUNNEL_MAPPER_ENTRY,
                   "{}.{}:{}: Overwriting hw for "
                   "tunnel_mapper_entry {:#x} due to "
                   "new overlapping tunnel_mapper_entry {:#x} :{}",
                   "vni_to_bd_mapping",
                   __func__,
                   __LINE__,
                   vlan_tunnel_mapper_entry_handle.data,
                   (get_parent()).data,
                   status);
        // Except during warm_init, assume the vlan tunnel_mapper_entry
        // already pushed an entry into hw
        if (!switch_store::smiContext::context().in_warm_init()) {
          add = false;
        }
      }
    }

    if (add) {
      status |= table.entry_add(match_key, action_entry, bf_rt_status);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_TUNNEL_MAPPER_ENTRY,
                   "{}.{}:{}: Failed hw add for tunnel_mapper_entry {:#x} :{}",
                   "vni_to_bd_mapping",
                   __func__,
                   __LINE__,
                   (get_parent()).data,
                   status);
      }
    } else {
      status |= table.entry_modify(match_key, action_entry);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(
            SWITCH_API_LEVEL_ERROR,
            SWITCH_OBJECT_TYPE_TUNNEL_MAPPER_ENTRY,
            "{}.{}:{}: Failed hw modify for tunnel_mapper_entry {:#x} :{}",
            "vni_to_bd_mapping",
            __func__,
            __LINE__,
            (get_parent()).data,
            status);
      }
    }
    status |= auto_object::create_update();
    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_VNI_TO_BD_MAPPING);

    if (!(type.enumdata ==
              SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TYPE_VNI_TO_VRF_HANDLE ||
          type.enumdata ==
              SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TYPE_VNI_TO_VLAN_HANDLE ||
          type.enumdata ==
              SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TYPE_VNI_TO_BRIDGE_HANDLE)) {
      return status;
    }

    /* Due to SONiC L3 EVPN implementation's use of overlapping
     * tunnel_mapper_entries, one for vrf and another for a corresponding
     * dummy vlan, allow for two entries with the same vni to co-exist.
     *
     * When only one entry for a particular vni is present, that entry is
     * programmed using the values in the corresponding tunnel_mapper_entry.
     * When two entries are present, then the vrf entry is preferred.
     *
     * Deleting entry | Overlapping entry    | Behavior
     * type           | type                 |
     * -------------------------------------------------------------------------
     * vlan           | none                 | hw delete
     * vlan           | vrf (same as in rif) | hw noop since vrf is preferred
     * vrf            | none                 | hw delete
     * vrf            | vlan                 | hw modify to vlan's values
     */

    // Check for overlapping mapper entries with the same VNI
    if (type.enumdata ==
            SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TYPE_VNI_TO_VLAN_HANDLE &&
        vrf_handle.data != 0) {
      switch_object_id_t vrf_tunnel_mapper_entry_handle = {0};
      status |= find_overlapping_vrf_entry(vrf_tunnel_mapper_entry_handle);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_TUNNEL_MAPPER_ENTRY,
                   "{}.{}:{}: Error while trying to find overlapping "
                   "tunnel_mapper_entries for tunnel_mapper_entry {:#x} :{}",
                   "vni_to_bd_mapping",
                   __func__,
                   __LINE__,
                   (get_parent()).data,
                   status);
        return status;
      }
      if (vrf_tunnel_mapper_entry_handle.data != 0) {
        // Since vrf entry is preferred, presumably already programmed,
        // don't delete anything from hw
        switch_log(SWITCH_API_LEVEL_DEBUG,
                   SWITCH_OBJECT_TYPE_TUNNEL_MAPPER_ENTRY,
                   "{}.{}:{}: Not deleting tunnel_mapper_entry {:#x} in hw "
                   "due to overlapping tunnel_mapper_entry {:#x} :{}",
                   "vni_to_bd_mapping",
                   __func__,
                   __LINE__,
                   (get_parent()).data,
                   vrf_tunnel_mapper_entry_handle.data,
                   status);
        status |= auto_object::del();
        return status;
      }
    } else if (type.enumdata ==
               SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TYPE_VNI_TO_VRF_HANDLE) {
      switch_object_id_t vlan_tunnel_mapper_entry_handle = {0};
      status |= find_overlapping_vlan_entry(vlan_tunnel_mapper_entry_handle);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_TUNNEL_MAPPER_ENTRY,
                   "{}.{}:{}: Error while trying to find overlapping "
                   "tunnel_mapper_entries for tunnel_mapper_entry {:#x} :{}",
                   "vni_to_bd_mapping",
                   __func__,
                   __LINE__,
                   (get_parent()).data,
                   status);
        return status;
      }
      if (vlan_tunnel_mapper_entry_handle.data != 0) {
        // Reprogram to vlan entry values
        switch_object_id_t vlan_handle = {0}, bd_handle_for_vlan = {0};
        switch_log(SWITCH_API_LEVEL_DEBUG,
                   SWITCH_OBJECT_TYPE_TUNNEL_MAPPER_ENTRY,
                   "{}.{}:{}: Reprogramming hw to values in "
                   "vlan tunnel_mapper_entry {:#x}, since "
                   "overlapping tunnel_mapper_entry {:#x} is being deleted :{}",
                   "vni_to_bd_mapping",
                   __func__,
                   __LINE__,
                   vlan_tunnel_mapper_entry_handle.data,
                   (get_parent()).data,
                   status);
        status |=
            switch_store::v_get(vlan_tunnel_mapper_entry_handle,
                                SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_NETWORK_HANDLE,
                                vlan_handle);
        status |= find_auto_oid(
            vlan_handle, SWITCH_OBJECT_TYPE_BD, bd_handle_for_vlan);
        status |= action_entry.set_arg(smi_id::P_VNI_SET_PROPERTIES_BD,
                                       compute_bd(bd_handle_for_vlan));
        status |= table.entry_modify(match_key, action_entry);
        if (status != SWITCH_STATUS_SUCCESS) {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_TUNNEL_MAPPER_ENTRY,
                     "{}.{}:{}: Failed hw modify during deletion of "
                     "tunnel_mapper_entry {:#x} :{}",
                     "vni_to_bd_mapping",
                     __func__,
                     __LINE__,
                     (get_parent()).data,
                     status);
        }

        status |= auto_object::del();
        return status;
      }
    }

    status |= table.entry_delete(match_key);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_TUNNEL_MAPPER_ENTRY,
                 "{}.{}:{}: Failed hw delete for tunnel_mapper_entry {:#x} "
                 ":{}",
                 "vni_to_bd_mapping",
                 __func__,
                 __LINE__,
                 (get_parent()).data,
                 status);
    }
    status |= auto_object::del();
    return status;
  }
};

switch_status_t vni_to_bd_mapping::find_overlapping_vrf_entry(
    switch_object_id_t &vrf_tunnel_mapper_entry_handle) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::set<switch_object_id_t> same_vrf_tunnel_mapper_entry_handles;
  status |=
      switch_store::referencing_set_get(vrf_handle,
                                        SWITCH_OBJECT_TYPE_TUNNEL_MAPPER_ENTRY,
                                        same_vrf_tunnel_mapper_entry_handles);
  for (auto const tunnel_mapper_entry_handle :
       same_vrf_tunnel_mapper_entry_handles) {
    switch_enum_t tunnel_mapper_entry_type = {0};
    uint32_t tunnel_mapper_entry_vni = 0;
    status |= switch_store::v_get(tunnel_mapper_entry_handle,
                                  SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TYPE,
                                  tunnel_mapper_entry_type);
    status |= switch_store::v_get(tunnel_mapper_entry_handle,
                                  SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TUNNEL_VNI,
                                  tunnel_mapper_entry_vni);
    if (tunnel_mapper_entry_type.enumdata ==
            SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TYPE_VNI_TO_VRF_HANDLE &&
        tunnel_mapper_entry_vni == vni) {
      vrf_tunnel_mapper_entry_handle = tunnel_mapper_entry_handle;
      return status;
    }
  }
  vrf_tunnel_mapper_entry_handle = {0};
  return status;
}

switch_status_t vni_to_bd_mapping::find_overlapping_vlan_entry(
    switch_object_id_t &vlan_tunnel_mapper_entry_handle) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::vector<switch_object_id_t> tunnel_mapper_handles;
  status |= switch_store::object_get_all_handles(
      SWITCH_OBJECT_TYPE_TUNNEL_MAPPER, tunnel_mapper_handles);
  for (auto const tunnel_mapper_handle : tunnel_mapper_handles) {
    switch_enum_t tunnel_mapper_type = {0};
    status |= switch_store::v_get(tunnel_mapper_handle,
                                  SWITCH_TUNNEL_MAPPER_ATTR_TYPE,
                                  tunnel_mapper_type);
    if (tunnel_mapper_type.enumdata ==
        SWITCH_TUNNEL_MAPPER_ATTR_TYPE_VNI_TO_VLAN_HANDLE) {
      std::set<switch_object_id_t> vlan_tunnel_mapper_entry_handles;
      status |= switch_store::referencing_set_get(
          tunnel_mapper_handle,
          SWITCH_OBJECT_TYPE_TUNNEL_MAPPER_ENTRY,
          vlan_tunnel_mapper_entry_handles);
      for (auto const tunnel_mapper_entry_handle :
           vlan_tunnel_mapper_entry_handles) {
        switch_enum_t tunnel_mapper_entry_type = {0};
        uint32_t tunnel_mapper_entry_vni = 0;
        status |= switch_store::v_get(tunnel_mapper_entry_handle,
                                      SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TYPE,
                                      tunnel_mapper_entry_type);
        status |=
            switch_store::v_get(tunnel_mapper_entry_handle,
                                SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TUNNEL_VNI,
                                tunnel_mapper_entry_vni);
        if (tunnel_mapper_entry_type.enumdata ==
                SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TYPE_VNI_TO_VLAN_HANDLE &&
            tunnel_mapper_entry_vni == vni) {
          vlan_tunnel_mapper_entry_handle = tunnel_mapper_entry_handle;
          return status;
        }
      }
    }
  }
  vlan_tunnel_mapper_entry_handle = {0};
  return status;
}

class bd_to_vni_mapping : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_BD_TO_VNI_MAPPING;
  static const switch_attr_id_t status_attr_id =
      SWITCH_BD_TO_VNI_MAPPING_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_BD_TO_VNI_MAPPING_ATTR_PARENT_HANDLE;

 public:
  bd_to_vni_mapping(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_BD_TO_VNI_MAPPING,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_enum_t type = {};
    switch_object_id_t network_handle = {0}, bd_handle = {0},
                       tunnel_mapper_handle = {0};
    uint16_t tunnel_mapper_index = 0;
    uint32_t vni = 0;

    if (!feature::is_feature_set(SWITCH_FEATURE_L2_VXLAN)) {
      clear_attrs();
      return;
    }

    status |=
        switch_store::v_get(parent, SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TYPE, type);
    if (type.enumdata !=
        SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TYPE_VLAN_HANDLE_TO_VNI) {
      clear_attrs();
      return;
    }

    status |= switch_store::v_get(
        parent, SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_NETWORK_HANDLE, network_handle);
    if (switch_store::object_type_query(network_handle) !=
        SWITCH_OBJECT_TYPE_VLAN) {
      clear_attrs();
      return;
    }

    status |= switch_store::v_get(
        parent,
        SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TUNNEL_MAPPER_HANDLE,
        tunnel_mapper_handle);

    status |= switch_store::v_get(tunnel_mapper_handle,
                                  SWITCH_TUNNEL_MAPPER_ATTR_TUNNEL_MAPPER_ID,
                                  tunnel_mapper_index);

    status |= switch_store::v_get(
        parent, SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TUNNEL_VNI, vni);
    status |= find_auto_oid(network_handle, SWITCH_OBJECT_TYPE_BD, bd_handle);

    status |= match_key.set_exact(smi_id::F_BD_TO_VNI_MAPPING_LOCAL_MD_BD,
                                  compute_bd(bd_handle));
    status |= match_key.set_exact(
        smi_id::F_BD_TO_VNI_MAPPING_LOCAL_MD_TUNNEL_MAPPER_INDEX,
        tunnel_mapper_index);

    action_entry.init_action_data(smi_id::A_BD_TO_VNI_MAPPING_SET_VNI);
    status |= action_entry.set_arg(smi_id::P_BD_SET_PROPERTIES_VNI, vni);
  }
};

class outer_ecmp_selector_group : public p4_object_selector_group {
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_OUTER_ECMP_SELECTOR_GROUP;
  static const switch_attr_id_t status_attr_id =
      SWITCH_OUTER_ECMP_SELECTOR_GROUP_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_OUTER_ECMP_SELECTOR_GROUP_ATTR_PARENT_HANDLE;

 public:
  outer_ecmp_selector_group(const switch_object_id_t parent,
                            switch_status_t &status)
      : p4_object_selector_group(
            smi_id::SG_OUTER_ECMP_SELECTOR_GROUP,
            status_attr_id,
            smi_id::P_OUTER_ECMP_SELECTOR_GROUP_MAX_GROUP_SIZE,
            smi_id::P_OUTER_ECMP_SELECTOR_GROUP_MAX_MEMBER_ARRAY,
            smi_id::P_OUTER_ECMP_SELECTOR_GROUP_MAX_MEMBER_STATUS_ARRAY,
            auto_ot,
            parent_attr_id,
            parent) {
    status |=
        match_key.set_exact(smi_id::F_OUTER_ECMP_SELECTOR_GROUP_ID, parent);
    status |= action_entry.init_indirect_data();
    status |=
        action_entry.set_arg(smi_id::P_OUTER_ECMP_SELECTOR_GROUP_MAX_GROUP_SIZE,
                             parent,
                             SWITCH_ECMP_ATTR_CONFIGURED_SIZE);
  }
};

class outer_ecmp_selector : public p4_object_action_selector {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_OUTER_ECMP_SELECTOR;
  static const switch_attr_id_t status_attr_id =
      SWITCH_OUTER_ECMP_SELECTOR_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_OUTER_ECMP_SELECTOR_ATTR_PARENT_HANDLE;
  switch_object_id_t nexthop_handle = {};
  bool do_nothing = false;

 public:
  outer_ecmp_selector(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_action_selector(
            smi_id::AP_OUTER_ECMP_SELECTOR,
            smi_id::F_OUTER_ECMP_SELECTOR_ACTION_MEMBER_ID,
            status_attr_id,
            auto_ot,
            parent_attr_id,
            parent) {
    switch_object_id_t neighbor_handle = {}, port_lag_handle = {};
    switch_enum_t nhop_type = {};
    std::set<switch_object_id_t> tunnel_nexthop_handles;
    uint32_t action_mem_id = 0;

    status |= switch_store::v_get(
        parent, SWITCH_NEXTHOP_RESOLUTION_ATTR_PARENT_HANDLE, nexthop_handle);
    if (switch_store::object_type_query(nexthop_handle) !=
        SWITCH_OBJECT_TYPE_NEXTHOP) {
      clear_attrs();
      do_nothing = true;
      return;
    }
    action_mem_id = switch_store::handle_to_id(nexthop_handle);

    // skip this for tunnel nexthop
    status |= switch_store::v_get(
        nexthop_handle, SWITCH_NEXTHOP_ATTR_TYPE, nhop_type);
    if ((nhop_type.enumdata == SWITCH_NEXTHOP_ATTR_TYPE_TUNNEL) ||
        (nhop_type.enumdata == SWITCH_NEXTHOP_ATTR_TYPE_SRV6_SIDLIST)) {
      clear_attrs();
      do_nothing = true;
      return;
    }

    status |= match_key.set_exact(
        smi_id::F_OUTER_ECMP_SELECTOR_ACTION_MEMBER_ID, action_mem_id);

    if (nhop_type.enumdata == SWITCH_NEXTHOP_ATTR_TYPE_IP) {
      switch_object_id_t urif_handle = {};
      switch_enum_t rif_type = {};
      status |= switch_store::v_get(
          nexthop_handle, SWITCH_NEXTHOP_ATTR_HANDLE, urif_handle);
      status |=
          switch_store::v_get(urif_handle, SWITCH_RIF_ATTR_TYPE, rif_type);
      if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_PORT ||
          rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_SUB_PORT) {
        status |= switch_store::v_get(
            urif_handle, SWITCH_RIF_ATTR_PORT_HANDLE, port_lag_handle);
      } else if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_VLAN) {
        switch_object_id_t mac_entry_handle = {};
        status |=
            switch_store::v_get(parent,
                                SWITCH_NEXTHOP_RESOLUTION_ATTR_NEIGHBOR_HANDLE,
                                neighbor_handle);
        status |=
            switch_store::v_get(parent,
                                SWITCH_NEXTHOP_RESOLUTION_ATTR_MAC_ENTRY_HANDLE,
                                mac_entry_handle);
        if (neighbor_handle.data != 0 && mac_entry_handle != 0) {
          status |=
              switch_store::v_get(mac_entry_handle,
                                  SWITCH_MAC_ENTRY_ATTR_DESTINATION_HANDLE,
                                  port_lag_handle);
        }
      }
    }

    // we have the nexthop resolved now, add the entry
    action_entry.init_action_data(smi_id::A_SET_OUTER_NEXTHOP_PROPERTIES);
    status |= action_entry.set_arg(
        smi_id::P_SET_OUTER_NEXTHOP_PROPERTIES_PORT_LAG_INDEX,
        compute_port_lag_index(port_lag_handle));
    status |= action_entry.set_arg(
        smi_id::P_SET_OUTER_NEXTHOP_PROPERTIES_NEXTHOP_INDEX,
        compute_nexthop_index(nexthop_handle));
  }
  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (do_nothing) return status;

    status |= p4_object_action_selector::create_update();
    reevaluate_tunnel_nexthops();
    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (do_nothing) return status;

    reevaluate_tunnel_nexthops();
    status |= p4_object_action_selector::del();
    return status;
  }
};

class outer_ecmp_membership : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_OUTER_ECMP_MEMBERSHIP;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_OUTER_ECMP_MEMBERSHIP_ATTR_PARENT_HANDLE;
  switch_object_id_t ecmp_handle = {}, nhop_handle = {},
                     ecmp_selector_handle = {};

 public:
  outer_ecmp_membership(const switch_object_id_t parent,
                        switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status |= switch_store::v_get(
        parent, SWITCH_ECMP_MEMBER_ATTR_NEXTHOP_HANDLE, nhop_handle);
    status |= find_auto_oid(
        nhop_handle, SWITCH_OBJECT_TYPE_ECMP_SELECTOR, ecmp_selector_handle);
    status |= switch_store::v_get(
        parent, SWITCH_ECMP_MEMBER_ATTR_ECMP_HANDLE, ecmp_handle);
  }

  bool does_ip_nhop_resolve_to_tunnel(switch_object_id_t nhop) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    bool nhop_resolution_on = true;
    switch_object_id_t rif_handle = {0}, dev_handle = {0},
                       nhop_res_handle = {0}, mac_handle = {0},
                       dest_handle = {0};
    switch_enum_t rif_type = {};
    status |= switch_store::v_get(nhop, SWITCH_NEXTHOP_ATTR_HANDLE, rif_handle);
    status |= switch_store::v_get(rif_handle, SWITCH_RIF_ATTR_TYPE, rif_type);
    if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_VLAN) {
      status |=
          switch_store::v_get(nhop, SWITCH_NEXTHOP_ATTR_DEVICE, dev_handle);
      nhop_resolution_on = is_nexthop_resolution_feature_on(dev_handle);
      if (nhop_resolution_on) {
        status |= find_auto_oid(
            nhop, SWITCH_OBJECT_TYPE_NEXTHOP_RESOLUTION, nhop_res_handle);
        status |=
            switch_store::v_get(nhop_res_handle,
                                SWITCH_NEXTHOP_RESOLUTION_ATTR_MAC_ENTRY_HANDLE,
                                mac_handle);
        if (mac_handle.data != 0) {
          status |=
              switch_store::v_get(mac_handle,
                                  SWITCH_MAC_ENTRY_ATTR_DESTINATION_HANDLE,
                                  dest_handle);
        }
      }
    }
    if (switch_store::object_type_query(dest_handle) ==
        SWITCH_OBJECT_TYPE_TUNNEL)
      return true;
    else
      return false;
  }

  // add to group
  // ACTION_MEMBER_ID is derived from nexthop_handle of the ecmp member
  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    bool mbr_st_temp = true;
    if (ecmp_handle == 0) return SWITCH_STATUS_INVALID_PARAMETER;

    std::vector<switch_object_id_t> ecmp_members;
    std::vector<bf_rt_id_t> mbrs;
    std::vector<bool> mbr_status;
    bool is_update = false;
    status |= switch_store::v_get(
        ecmp_handle, SWITCH_ECMP_ATTR_ECMP_MEMBERS, ecmp_members);
    // Check if parent is already a member, else add to list
    for (auto const mbr : ecmp_members) {
      if (mbr.data == get_parent().data) {
        if (!switch_store::smiContext::context().in_warm_init())
          is_update = true;
      } else {
        switch_object_id_t mbr_nhop_handle = {0};
        status |= switch_store::v_get(
            mbr, SWITCH_ECMP_MEMBER_ATTR_NEXTHOP_HANDLE, mbr_nhop_handle);
        switch_enum_t nhop_type = {};
        status |= switch_store::v_get(
            mbr_nhop_handle, SWITCH_NEXTHOP_ATTR_TYPE, nhop_type);
        if ((nhop_type.enumdata == SWITCH_NEXTHOP_ATTR_TYPE_IP) &&
            (!does_ip_nhop_resolve_to_tunnel(mbr_nhop_handle))) {
          mbrs.push_back(switch_store::handle_to_id(mbr_nhop_handle));
          mbr_status.push_back(mbr_st_temp);
        } else if ((nhop_type.enumdata != SWITCH_NEXTHOP_ATTR_TYPE_TUNNEL) &&
                   (nhop_type.enumdata !=
                    SWITCH_NEXTHOP_ATTR_TYPE_SRV6_SIDLIST)) {
          mbrs.push_back(switch_store::handle_to_id(mbr_nhop_handle));
          mbr_status.push_back(mbr_st_temp);
        }
      }
    }

    switch_enum_t nhop_type = {};
    status |=
        switch_store::v_get(nhop_handle, SWITCH_NEXTHOP_ATTR_TYPE, nhop_type);
    if ((nhop_type.enumdata == SWITCH_NEXTHOP_ATTR_TYPE_IP) &&
        (!does_ip_nhop_resolve_to_tunnel(nhop_handle))) {
      mbrs.push_back(switch_store::handle_to_id(nhop_handle));
      mbr_status.push_back(mbr_st_temp);
    } else if ((nhop_type.enumdata != SWITCH_NEXTHOP_ATTR_TYPE_TUNNEL) &&
               (nhop_type.enumdata != SWITCH_NEXTHOP_ATTR_TYPE_SRV6_SIDLIST)) {
      // add this member
      mbrs.push_back(switch_store::handle_to_id(nhop_handle));
      mbr_status.push_back(mbr_st_temp);
    }

    outer_ecmp_selector_group ecmp_group_obj(ecmp_handle, status);
    status = ecmp_group_obj.add_delete_member(mbrs, mbr_status);
    if (status) return SWITCH_STATUS_FAILURE;

    if (is_update) return status;

    if (mbrs.size() == 1) {
      // add the ecmp table entry pointing to the selector group when first
      // member is added
      // The match key is not known since the ecmp selector group can be pointed
      // to by any tunnel nexthop. Evaluate all tunnel nexthops
      bool empty = false;
      status = switch_store::v_set(ecmp_handle, SWITCH_ECMP_ATTR_EMPTY, empty);
      reevaluate_tunnel_nexthops();
    }

    status |= auto_object::create_update();
    return status;
  }

  // remove from group
  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    bool mbr_st_temp = true;
    if (ecmp_handle == 0) return SWITCH_STATUS_INVALID_PARAMETER;

    std::vector<switch_object_id_t> ecmp_members;
    std::vector<bf_rt_id_t> mbrs;
    std::vector<bool> mbr_status;
    status |= switch_store::v_get(
        ecmp_handle, SWITCH_ECMP_ATTR_ECMP_MEMBERS, ecmp_members);
    // Exclude the parent from the member list
    for (auto const mbr : ecmp_members) {
      switch_object_id_t mbr_nhop_handle = {0};
      status |= switch_store::v_get(
          mbr, SWITCH_ECMP_MEMBER_ATTR_NEXTHOP_HANDLE, mbr_nhop_handle);
      switch_enum_t nhop_type = {};
      status |= switch_store::v_get(
          mbr_nhop_handle, SWITCH_NEXTHOP_ATTR_TYPE, nhop_type);
      if ((nhop_type.enumdata == SWITCH_NEXTHOP_ATTR_TYPE_IP) &&
          (!does_ip_nhop_resolve_to_tunnel(mbr_nhop_handle))) {
        mbrs.push_back(switch_store::handle_to_id(mbr_nhop_handle));
        mbr_status.push_back(mbr_st_temp);
      } else if ((nhop_type.enumdata != SWITCH_NEXTHOP_ATTR_TYPE_TUNNEL) &&
                 (nhop_type.enumdata !=
                  SWITCH_NEXTHOP_ATTR_TYPE_SRV6_SIDLIST)) {
        mbrs.push_back(switch_store::handle_to_id(mbr_nhop_handle));
        mbr_status.push_back(mbr_st_temp);
      }
    }

    if (mbrs.size() == 0) {
      // delete the ecmp table entry to the selector group
      // The match key is not known since the ecmp selector group can be pointed
      // to by any tunnel nexthop. Evaluate all tunnel nexthops
      bool empty = true;
      status = switch_store::v_set(ecmp_handle, SWITCH_ECMP_ATTR_EMPTY, empty);
      reevaluate_tunnel_nexthops();
    }

    outer_ecmp_selector_group ecmp_group_obj(ecmp_handle, status);
    status = ecmp_group_obj.add_delete_member(mbrs, mbr_status);
    if (status) return SWITCH_STATUS_FAILURE;

    status |= auto_object::del();
    return status;
  }
};

class outer_fib_table : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_OUTER_FIB_TABLE;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_OUTER_FIB_TABLE_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_OUTER_FIB_TABLE_ATTR_STATUS;
  switch_object_id_t outer_ecmp_nhop_handle = {0};

 public:
  outer_fib_table(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_OUTER_FIB,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    size_t len = 0;
    bool nexthop_resolution_on = false;
    switch_object_id_t device_handle = {0};

    if (switch_store::object_type_query(parent) !=
        SWITCH_OBJECT_TYPE_TUNNEL_DEST_IP) {
      clear_attrs();
      return;
    }

    status |= switch_store::v_get(
        parent, SWITCH_TUNNEL_DEST_IP_ATTR_DEVICE, device_handle);
    nexthop_resolution_on = is_nexthop_resolution_feature_on(device_handle);
    if (!nexthop_resolution_on) {
      clear_attrs();
      return;
    }

    status |= match_key.set_exact(smi_id::F_OUTER_FIB_LOCAL_MD_TUNNEL_DIP_INDEX,
                                  switch_store::handle_to_id(parent));
    status |= compute_outer_nexthop(parent, outer_ecmp_nhop_handle);
    if (status == SWITCH_STATUS_ITEM_NOT_FOUND) {
      switch_log(
          SWITCH_API_LEVEL_DEBUG,
          SWITCH_OBJECT_TYPE_TUNNEL_DEST_IP,
          "{}:{}: failed to compute outer nexthop for tunnel_dest_ip {:#x}: {}",
          __func__,
          __LINE__,
          (auto_obj.get_parent()).data,
          switch_error_to_string(status));
      // Reset status so that object creation does not fail,
      // since underlay route may appear later
      status = SWITCH_STATUS_SUCCESS;
    }

    auto_obj.attrs.insert(
        attr_w(SWITCH_OUTER_FIB_TABLE_ATTR_OUTER_NEXTHOP_HANDLE,
               outer_ecmp_nhop_handle));
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(
          SWITCH_API_LEVEL_ERROR,
          SWITCH_OBJECT_TYPE_OUTER_FIB_TABLE,
          "{}:{}: failed to set outer nexthop nexthop for outer_fib_table {}",
          __func__,
          __LINE__,
          get_auto_oid().data);
      return;
    }

    if (switch_store::object_type_query(outer_ecmp_nhop_handle) ==
        SWITCH_OBJECT_TYPE_ECMP) {
      status = switch_store::list_len(
          outer_ecmp_nhop_handle, SWITCH_ECMP_ATTR_ECMP_MEMBERS, len);
      bool empty = true;
      status = switch_store::v_get(
          outer_ecmp_nhop_handle, SWITCH_ECMP_ATTR_EMPTY, empty);
      // len = 0 and empty = false for add case
      // len = 0 and empty = true for delete case
      if (len == 0) {
        if (empty) {
          status |= action_entry.init_indirect_data();
          status |= action_entry.set_arg(
              smi_id::D_OUTER_FIB_ACTION_MEMBER_ID,
              static_cast<uint32_t>(DEFAULT_ACTION_MEMBER_ID));
          return;
        }
      }
      status |= action_entry.init_indirect_data();
      status |= action_entry.set_arg(smi_id::D_OUTER_FIB_SELECTOR_GROUP_ID,
                                     outer_ecmp_nhop_handle);
    } else if (switch_store::object_type_query(outer_ecmp_nhop_handle) ==
               SWITCH_OBJECT_TYPE_NEXTHOP) {
      status |= action_entry.init_indirect_data();
      status |= action_entry.set_arg(smi_id::D_OUTER_FIB_ACTION_MEMBER_ID,
                                     outer_ecmp_nhop_handle);
    } else {
      status |= action_entry.init_indirect_data();
      status |=
          action_entry.set_arg(smi_id::D_OUTER_FIB_ACTION_MEMBER_ID,
                               static_cast<uint32_t>(DEFAULT_ACTION_MEMBER_ID));
    }
  }
};

class tunnel_src_addr_rewrite : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_TUNNEL_SRC_ADDR_REWRITE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_TUNNEL_SRC_ADDR_REWRITE_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_TUNNEL_SRC_ADDR_REWRITE_ATTR_PARENT_HANDLE;

 public:
  tunnel_src_addr_rewrite(const switch_object_id_t parent,
                          switch_status_t &status)
      : p4_object_match_action(smi_id::T_TUNNEL_SRC_ADDR_REWRITE,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_ip_prefix_t ip_prefix = {};
    uint16_t tunnel_id = 0;
    switch_enum_t peer_mode = {0};

    status =
        switch_store::v_get(parent, SWITCH_TUNNEL_ATTR_PEER_MODE, peer_mode);
    if (peer_mode.enumdata != SWITCH_TUNNEL_ATTR_PEER_MODE_P2MP) {
      clear_attrs();
      return;
    }

    status =
        switch_store::v_get(parent, SWITCH_TUNNEL_ATTR_SRC_IP, ip_prefix.addr);
    if (ip_prefix.addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4) {
      if (ip_prefix.addr.ip4 == 0) {
        clear_attrs();
        return;
      }
    } else if (ip_prefix.addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6) {
      switch_ip6_t zero_ip = {0};
      if (memcmp(ip_prefix.addr.ip6, zero_ip, IPV6_LEN) == 0) {
        clear_attrs();
        return;
      }
    }

    status |=
        switch_store::v_get(parent, SWITCH_TUNNEL_ATTR_TUNNEL_ID, tunnel_id);

    status |= match_key.set_exact(
        smi_id::F_TUNNEL_SRC_ADDR_REWRITE_TUNNEL_INDEX, tunnel_id);
    if (ip_prefix.addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4) {
      action_entry.init_action_data(
          smi_id::A_TUNNEL_SRC_ADDR_REWRITE_IPV4_SIP_REWRITE);
      status |= action_entry.set_arg(
          smi_id::P_TUNNEL_SRC_ADDR_REWRITE_IPV4_SIP_REWRITE_SRC_ADDR,
          parent,
          SWITCH_TUNNEL_ATTR_SRC_IP);
      if (!feature::is_feature_set(SWITCH_FEATURE_TUNNEL_TTL_MODE)) {
        status |= action_entry.set_arg(
            smi_id::P_TUNNEL_SRC_ADDR_REWRITE_IPV4_SIP_REWRITE_TTL_VAL,
            parent,
            SWITCH_TUNNEL_ATTR_TTL);
      }
      if (feature::is_feature_set(SWITCH_FEATURE_TUNNEL_QOS_MODE)) {
        status |= action_entry.set_arg(
            smi_id::P_TUNNEL_SRC_ADDR_REWRITE_IPV4_SIP_REWRITE_DSCP_VAL,
            parent,
            SWITCH_TUNNEL_ATTR_DSCP_VAL);
      }
    } else if (ip_prefix.addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6) {
      action_entry.init_action_data(
          smi_id::A_TUNNEL_SRC_ADDR_REWRITE_IPV6_SIP_REWRITE);
      status |= action_entry.set_arg(
          smi_id::P_TUNNEL_SRC_ADDR_REWRITE_IPV6_SIP_REWRITE_SRC_ADDR,
          parent,
          SWITCH_TUNNEL_ATTR_SRC_IP);
      if (!feature::is_feature_set(SWITCH_FEATURE_TUNNEL_TTL_MODE)) {
        status |= action_entry.set_arg(
            smi_id::P_TUNNEL_SRC_ADDR_REWRITE_IPV6_SIP_REWRITE_TTL_VAL,
            parent,
            SWITCH_TUNNEL_ATTR_TTL);
      }
      if (feature::is_feature_set(SWITCH_FEATURE_TUNNEL_QOS_MODE)) {
        status |= action_entry.set_arg(
            smi_id::P_TUNNEL_SRC_ADDR_REWRITE_IPV6_SIP_REWRITE_DSCP_VAL,
            parent,
            SWITCH_TUNNEL_ATTR_DSCP_VAL);
      }
    } else {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_TUNNEL,
                 "{}:{}: Invalid IP address family {}",
                 __func__,
                 __LINE__,
                 ip_prefix.addr.addr_family);
      status = SWITCH_STATUS_INVALID_PARAMETER;
    }
  }
};

class tunnel_dst_addr_rewrite : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_TUNNEL_DST_ADDR_REWRITE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_TUNNEL_DST_ADDR_REWRITE_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_TUNNEL_DST_ADDR_REWRITE_ATTR_PARENT_HANDLE;

 public:
  tunnel_dst_addr_rewrite(const switch_object_id_t parent,
                          switch_status_t &status)
      : p4_object_match_action(smi_id::T_TUNNEL_DST_ADDR_REWRITE,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_ip_address_t dst_ip_addr;

    status |= switch_store::v_get(
        parent, SWITCH_TUNNEL_DEST_IP_ATTR_DEST_IP, dst_ip_addr);

    status |= match_key.set_exact(
        smi_id::F_TUNNEL_DST_ADDR_REWRITE_LOCAL_MD_TUNNEL_DIP_INDEX,
        switch_store::handle_to_id(parent));

    if (dst_ip_addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4) {
      action_entry.init_action_data(
          smi_id::A_TUNNEL_DST_ADDR_REWRITE_IPV4_DIP_REWRITE);
      status |= action_entry.set_arg(
          smi_id::P_TUNNEL_DST_ADDR_REWRITE_IPV4_DIP_REWRITE_DST_ADDR,
          parent,
          SWITCH_TUNNEL_DEST_IP_ATTR_DEST_IP);
    } else {
      action_entry.init_action_data(
          smi_id::A_TUNNEL_DST_ADDR_REWRITE_IPV6_DIP_REWRITE);
      status |= action_entry.set_arg(
          smi_id::P_TUNNEL_DST_ADDR_REWRITE_IPV6_DIP_REWRITE_DST_ADDR,
          parent,
          SWITCH_TUNNEL_DEST_IP_ATTR_DEST_IP);
    }
  }
};

class tunnel_replication_resolution : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_TUNNEL_REPLICATION_RESOLUTION;
  static const switch_attr_id_t status_attr_id =
      SWITCH_TUNNEL_REPLICATION_RESOLUTION_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_TUNNEL_REPLICATION_RESOLUTION_ATTR_PARENT_HANDLE;

 public:
  tunnel_replication_resolution(const switch_object_id_t parent,
                                switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    switch_object_id_t outer_nexthop_handle = {0};

    if (switch_store::object_type_query(parent) !=
        SWITCH_OBJECT_TYPE_OUTER_FIB_TABLE) {
      clear_attrs();
      return;
    }

    // set flood_nexthop_handle according to outer_fib_table attribute
    // outer_nexthop_handle
    //  - if it is regular nexthop - take it directly
    //  - if it is ecmp - take one ecmp member randomly
    status |=
        switch_store::v_get(parent,
                            SWITCH_OUTER_FIB_TABLE_ATTR_OUTER_NEXTHOP_HANDLE,
                            outer_nexthop_handle);

    if (switch_store::object_type_query(outer_nexthop_handle) ==
        SWITCH_OBJECT_TYPE_ECMP) {
      std::vector<switch_object_id_t> ecmp_members;

      status |= switch_store::v_get(
          outer_nexthop_handle, SWITCH_ECMP_ATTR_ECMP_MEMBERS, ecmp_members);

      if (ecmp_members.size() != 0) {
        size_t hash_parent = std::hash<uint64_t>{}(parent.data);
        size_t rand_mbr_no = hash_parent % ecmp_members.size();

        status |= switch_store::v_get(ecmp_members[rand_mbr_no],
                                      SWITCH_ECMP_MEMBER_ATTR_NEXTHOP_HANDLE,
                                      outer_nexthop_handle);
      } else {
        outer_nexthop_handle = {0};
      }
    }

    // store nexthop handle
    attrs.insert(
        attr_w(SWITCH_TUNNEL_REPLICATION_RESOLUTION_ATTR_FLOOD_NEXTHOP_HANDLE,
               outer_nexthop_handle));
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_object_id_t auto_oid = get_auto_oid();
    switch_object_id_t parent = get_parent();
    bool allocate = auto_oid == 0 ? true : false;
    uint16_t rid = 0;

    if (switch_store::object_type_query(parent) !=
        SWITCH_OBJECT_TYPE_OUTER_FIB_TABLE) {
      return status;
    }

    if (allocate) {
      // object is created for the first time - allocate rid
      rid = compute_rid(auto_oid);

      // store rid
      auto insert_ret = attrs.insert(
          attr_w(SWITCH_TUNNEL_REPLICATION_RESOLUTION_ATTR_RID, rid));
      if (insert_ret.second == false) status |= SWITCH_STATUS_FAILURE;
    } else {
      // object already exists - get stored rid
      status |= switch_store::v_get(
          auto_oid, SWITCH_TUNNEL_REPLICATION_RESOLUTION_ATTR_RID, rid);

      if (switch_store::smiContext::context().in_warm_init()) {
        SWITCH_CONTEXT.rid_reserve(rid);
      }
    }

    status |= auto_object::create_update();

    // release allocated rid if the object was not created successfully
    if (status != SWITCH_STATUS_SUCCESS && allocate) {
      SWITCH_CONTEXT.rid_release(rid);
    }

    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_object_id_t parent = get_parent();
    uint16_t rid = 0;

    if (switch_store::object_type_query(parent) !=
        SWITCH_OBJECT_TYPE_OUTER_FIB_TABLE) {
      return status;
    }

    // release allocated rid value
    status |= switch_store::v_get(
        get_auto_oid(), SWITCH_TUNNEL_REPLICATION_RESOLUTION_ATTR_RID, rid);
    status |= SWITCH_CONTEXT.rid_release(rid);

    status |= auto_object::del();

    return status;
  }
};

class egress_vrf_mapping : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_EGRESS_VRF_MAPPING;
  static const switch_attr_id_t status_attr_id =
      SWITCH_EGRESS_VRF_MAPPING_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_EGRESS_VRF_MAPPING_ATTR_PARENT_HANDLE;

  bool parent_is_vrf = false;

 public:
  egress_vrf_mapping(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_EGRESS_VRF_MAPPING,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_object_id_t vrf_handle{}, device_handle{};
    bool overlay_vrf_mac = false;
    switch_mac_addr_t src_mac;

    if (switch_store::object_type_query(parent) != SWITCH_OBJECT_TYPE_VRF)
      return;
    vrf_handle = parent;
    parent_is_vrf = true;

    status |=
        switch_store::v_get(vrf_handle, SWITCH_VRF_ATTR_DEVICE, device_handle);
    status |=
        switch_store::v_get(device_handle,
                            SWITCH_DEVICE_ATTR_INNER_SRC_MAC_FROM_OVERLAY_VRF,
                            overlay_vrf_mac);

    if (overlay_vrf_mac) {
      status |=
          switch_store::v_get(vrf_handle, SWITCH_VRF_ATTR_SRC_MAC, src_mac);
    } else {
      status |= switch_store::v_get(
          device_handle, SWITCH_DEVICE_ATTR_SRC_MAC, src_mac);
    }

    status |= match_key.set_exact(smi_id::F_EGRESS_VRF_MAPPING_LOCAL_MD_VRF,
                                  vrf_handle);
    action_entry.init_action_data(
        smi_id::A_EGRESS_VRF_MAPPING_SET_VRF_PROPERTIES);
    status |= action_entry.set_arg(
        smi_id::P_EGRESS_VRF_MAPPING_SET_VRF_PROPERTIES_SMAC, src_mac);
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (!parent_is_vrf) return status;

    return p4_object_match_action::create_update();
  }

  switch_status_t del() {
    if (!parent_is_vrf) return SWITCH_STATUS_SUCCESS;
    return p4_object_match_action::del();
  }
};

class tunnel_encap_ttl : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_TUNNEL_ENCAP_TTL;
  static const switch_attr_id_t status_attr_id =
      SWITCH_TUNNEL_ENCAP_TTL_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_TUNNEL_ENCAP_TTL_ATTR_PARENT_HANDLE;

 public:
  tunnel_encap_ttl(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_TUNNEL_REWRITE_ENCAP_TTL,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    switch_ip_prefix_t ip_prefix = {};
    switch_enum_t ttl_mode = {0}, peer_mode = {0};
    bool outer_ipv4_valid = false;
    bool outer_ipv6_valid = false;
    uint16_t tunnel_id = 0;

    status =
        switch_store::v_get(parent, SWITCH_TUNNEL_ATTR_PEER_MODE, peer_mode);
    if (peer_mode.enumdata != SWITCH_TUNNEL_ATTR_PEER_MODE_P2MP) {
      clear_attrs();
      return;
    }

    status |= switch_store::v_get(
        parent, SWITCH_TUNNEL_ATTR_ENCAP_TTL_MODE, ttl_mode);
    status =
        switch_store::v_get(parent, SWITCH_TUNNEL_ATTR_SRC_IP, ip_prefix.addr);
    if (ip_prefix.addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4) {
      outer_ipv4_valid = true;
      outer_ipv6_valid = false;
    } else if (ip_prefix.addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6) {
      outer_ipv4_valid = false;
      outer_ipv6_valid = true;
    } else {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_TUNNEL,
                 "{}:{}: Invalid IP address family {}",
                 __func__,
                 __LINE__,
                 ip_prefix.addr.addr_family);
      status = SWITCH_STATUS_INVALID_PARAMETER;
      return;
    }

    // getting tunnel-id
    status |=
        switch_store::v_get(parent, SWITCH_TUNNEL_ATTR_TUNNEL_ID, tunnel_id);

    auto it = match_action_list.begin();
    // Inner IPv4 encap_ttl entry
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(
            _MatchKey(smi_id::T_TUNNEL_REWRITE_ENCAP_TTL),
            _ActionEntry(smi_id::T_TUNNEL_REWRITE_ENCAP_TTL)));
    status |= it->first.set_exact(
        smi_id::F_TUNNEL_REWRITE_ENCAP_TTL_LOCAL_MD_TUNNEL_INDEX, tunnel_id);
    status |= it->first.set_exact(smi_id::F_TUNNEL_REWRITE_ENCAP_TTL_IPV4_VALID,
                                  outer_ipv4_valid);
    status |= it->first.set_exact(smi_id::F_TUNNEL_REWRITE_ENCAP_TTL_IPV6_VALID,
                                  outer_ipv6_valid);
    status |= it->first.set_exact(
        smi_id::F_TUNNEL_REWRITE_ENCAP_TTL_INNER_IPV4_VALID, true);
    status |= it->first.set_exact(
        smi_id::F_TUNNEL_REWRITE_ENCAP_TTL_INNER_IPV6_VALID, false);
    switch (ttl_mode.enumdata) {
      case SWITCH_TUNNEL_ATTR_ENCAP_TTL_MODE_PIPE_MODEL:
        if (outer_ipv4_valid) {
          it->second.init_action_data(
              smi_id::A_TUNNEL_REWRITE_ENCAP_TTL_V4_IN_V4_PIPE);
          status |= it->second.set_arg(
              smi_id::P_TUNNEL_REWRITE_ENCAP_TTL_V4_IN_V4_PIPE_TTL_VAL,
              parent,
              SWITCH_TUNNEL_ATTR_TTL);
        } else {
          it->second.init_action_data(
              smi_id::A_TUNNEL_REWRITE_ENCAP_TTL_V4_IN_V6_PIPE);
          status |= it->second.set_arg(
              smi_id::P_TUNNEL_REWRITE_ENCAP_TTL_V4_IN_V6_PIPE_TTL_VAL,
              parent,
              SWITCH_TUNNEL_ATTR_TTL);
        }
        break;
      case SWITCH_TUNNEL_ATTR_ENCAP_TTL_MODE_UNIFORM_MODEL:
        if (outer_ipv4_valid) {
          it->second.init_action_data(smi_id::A_NO_ACTION);
        } else {
          it->second.init_action_data(
              smi_id::A_TUNNEL_REWRITE_ENCAP_TTL_V4_IN_V6_UNIFORM);
        }
        break;
    }

    // Inner IPv6 encap_ttl entry
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(
            _MatchKey(smi_id::T_TUNNEL_REWRITE_ENCAP_TTL),
            _ActionEntry(smi_id::T_TUNNEL_REWRITE_ENCAP_TTL)));
    status |= it->first.set_exact(
        smi_id::F_TUNNEL_REWRITE_ENCAP_TTL_LOCAL_MD_TUNNEL_INDEX, tunnel_id);
    status |= it->first.set_exact(smi_id::F_TUNNEL_REWRITE_ENCAP_TTL_IPV4_VALID,
                                  outer_ipv4_valid);
    status |= it->first.set_exact(smi_id::F_TUNNEL_REWRITE_ENCAP_TTL_IPV6_VALID,
                                  outer_ipv6_valid);
    status |= it->first.set_exact(
        smi_id::F_TUNNEL_REWRITE_ENCAP_TTL_INNER_IPV4_VALID, false);
    status |= it->first.set_exact(
        smi_id::F_TUNNEL_REWRITE_ENCAP_TTL_INNER_IPV6_VALID, true);
    switch (ttl_mode.enumdata) {
      case SWITCH_TUNNEL_ATTR_ENCAP_TTL_MODE_PIPE_MODEL:
        if (outer_ipv4_valid) {
          it->second.init_action_data(
              smi_id::A_TUNNEL_REWRITE_ENCAP_TTL_V6_IN_V4_PIPE);
          status |= it->second.set_arg(
              smi_id::P_TUNNEL_REWRITE_ENCAP_TTL_V6_IN_V4_PIPE_TTL_VAL,
              parent,
              SWITCH_TUNNEL_ATTR_TTL);
        } else {
          it->second.init_action_data(
              smi_id::A_TUNNEL_REWRITE_ENCAP_TTL_V6_IN_V6_PIPE);
          status |= it->second.set_arg(
              smi_id::P_TUNNEL_REWRITE_ENCAP_TTL_V6_IN_V6_PIPE_TTL_VAL,
              parent,
              SWITCH_TUNNEL_ATTR_TTL);
        }
        break;
      case SWITCH_TUNNEL_ATTR_ENCAP_TTL_MODE_UNIFORM_MODEL:
        if (outer_ipv4_valid) {
          it->second.init_action_data(
              smi_id::A_TUNNEL_REWRITE_ENCAP_TTL_V6_IN_V4_UNIFORM);
        } else {
          it->second.init_action_data(smi_id::A_NO_ACTION);
        }
        break;
    }

    // For non-ip traffic ttl_mode will be pipe_mode
    if (feature::is_feature_set(SWITCH_FEATURE_L2_VXLAN)) {
      it = match_action_list.insert(
          it,
          std::pair<_MatchKey, _ActionEntry>(
              _MatchKey(smi_id::T_TUNNEL_REWRITE_ENCAP_TTL),
              _ActionEntry(smi_id::T_TUNNEL_REWRITE_ENCAP_TTL)));

      status |= it->first.set_exact(
          smi_id::F_TUNNEL_REWRITE_ENCAP_TTL_LOCAL_MD_TUNNEL_INDEX, tunnel_id);
      status |= it->first.set_exact(
          smi_id::F_TUNNEL_REWRITE_ENCAP_TTL_IPV4_VALID, outer_ipv4_valid);
      status |= it->first.set_exact(
          smi_id::F_TUNNEL_REWRITE_ENCAP_TTL_IPV6_VALID, outer_ipv6_valid);
      status |= it->first.set_exact(
          smi_id::F_TUNNEL_REWRITE_ENCAP_TTL_INNER_IPV4_VALID, false);
      status |= it->first.set_exact(
          smi_id::F_TUNNEL_REWRITE_ENCAP_TTL_INNER_IPV6_VALID, false);

      if (outer_ipv6_valid) {
        it->second.init_action_data(
            smi_id::A_TUNNEL_REWRITE_ENCAP_TTL_V6_IN_V6_PIPE);
        status |= it->second.set_arg(
            smi_id::P_TUNNEL_REWRITE_ENCAP_TTL_V6_IN_V6_PIPE_TTL_VAL,
            parent,
            SWITCH_TUNNEL_ATTR_TTL);
      } else if (outer_ipv4_valid) {
        it->second.init_action_data(
            smi_id::A_TUNNEL_REWRITE_ENCAP_TTL_V4_IN_V4_PIPE);
        status |= it->second.set_arg(
            smi_id::P_TUNNEL_REWRITE_ENCAP_TTL_V4_IN_V4_PIPE_TTL_VAL,
            parent,
            SWITCH_TUNNEL_ATTR_TTL);
      }
    }
  }
};

class tunnel_encap_dscp : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_TUNNEL_ENCAP_DSCP;
  static const switch_attr_id_t status_attr_id =
      SWITCH_TUNNEL_ENCAP_DSCP_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_TUNNEL_ENCAP_DSCP_ATTR_PARENT_HANDLE;

 public:
  tunnel_encap_dscp(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_TUNNEL_REWRITE_ENCAP_DSCP,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    switch_ip_prefix_t ip_prefix = {};
    switch_enum_t qos_mode = {0}, peer_mode = {0};
    bool outer_ipv4_valid = false;
    bool outer_ipv6_valid = false;
    uint16_t tunnel_id = 0;

    status =
        switch_store::v_get(parent, SWITCH_TUNNEL_ATTR_PEER_MODE, peer_mode);
    if (peer_mode.enumdata != SWITCH_TUNNEL_ATTR_PEER_MODE_P2MP) {
      clear_attrs();
      return;
    }

    status |= switch_store::v_get(
        parent, SWITCH_TUNNEL_ATTR_ENCAP_QOS_MODE, qos_mode);
    status =
        switch_store::v_get(parent, SWITCH_TUNNEL_ATTR_SRC_IP, ip_prefix.addr);
    if (ip_prefix.addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4) {
      outer_ipv4_valid = true;
      outer_ipv6_valid = false;
    } else if (ip_prefix.addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6) {
      outer_ipv4_valid = false;
      outer_ipv6_valid = true;
    } else {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_TUNNEL,
                 "{}:{}: Invalid IP address family {}",
                 __func__,
                 __LINE__,
                 ip_prefix.addr.addr_family);
      status = SWITCH_STATUS_INVALID_PARAMETER;
      return;
    }

    if (!feature::is_feature_set(SWITCH_FEATURE_TUNNEL_QOS_MODE) &&
        qos_mode.enumdata == SWITCH_TUNNEL_ATTR_ENCAP_QOS_MODE_PIPE_MODEL) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_TUNNEL,
                 "{}:{}: Invalid qos mode {}",
                 __func__,
                 __LINE__,
                 qos_mode.enumdata);
      status = SWITCH_STATUS_INVALID_PARAMETER;
      return;
    }
    status |=
        switch_store::v_get(parent, SWITCH_TUNNEL_ATTR_TUNNEL_ID, tunnel_id);

    auto it = match_action_list.begin();
    // Inner IPv4 encap_dscp entry
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(
            _MatchKey(smi_id::T_TUNNEL_REWRITE_ENCAP_DSCP),
            _ActionEntry(smi_id::T_TUNNEL_REWRITE_ENCAP_DSCP)));
    status |= it->first.set_exact(
        smi_id::F_TUNNEL_REWRITE_ENCAP_DSCP_LOCAL_MD_TUNNEL_INDEX, tunnel_id);
    status |= it->first.set_exact(
        smi_id::F_TUNNEL_REWRITE_ENCAP_DSCP_IPV4_VALID, outer_ipv4_valid);
    status |= it->first.set_exact(
        smi_id::F_TUNNEL_REWRITE_ENCAP_DSCP_IPV6_VALID, outer_ipv6_valid);
    status |= it->first.set_exact(
        smi_id::F_TUNNEL_REWRITE_ENCAP_DSCP_INNER_IPV4_VALID, true);
    status |= it->first.set_exact(
        smi_id::F_TUNNEL_REWRITE_ENCAP_DSCP_INNER_IPV6_VALID, false);
    switch (qos_mode.enumdata) {
      case SWITCH_TUNNEL_ATTR_ENCAP_QOS_MODE_PIPE_MODEL:
        if (outer_ipv4_valid) {
          it->second.init_action_data(
              smi_id::A_TUNNEL_REWRITE_ENCAP_DSCP_V4_IN_V4_ECN);
        } else {
          it->second.init_action_data(
              smi_id::A_TUNNEL_REWRITE_ENCAP_DSCP_V4_IN_V6_ECN);
        }
        break;
      case SWITCH_TUNNEL_ATTR_ENCAP_QOS_MODE_UNIFORM_MODEL:
        if (outer_ipv4_valid) {
          it->second.init_action_data(
              smi_id::A_TUNNEL_REWRITE_ENCAP_DSCP_V4_IN_V4_UNIFORM);
        } else {
          it->second.init_action_data(
              smi_id::A_TUNNEL_REWRITE_ENCAP_DSCP_V4_IN_V6_UNIFORM);
        }
        break;
    }

    // Inner IPv6 encap_dscp entry
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(
            _MatchKey(smi_id::T_TUNNEL_REWRITE_ENCAP_DSCP),
            _ActionEntry(smi_id::T_TUNNEL_REWRITE_ENCAP_DSCP)));
    status |= it->first.set_exact(
        smi_id::F_TUNNEL_REWRITE_ENCAP_DSCP_LOCAL_MD_TUNNEL_INDEX, tunnel_id);
    status |= it->first.set_exact(
        smi_id::F_TUNNEL_REWRITE_ENCAP_DSCP_IPV4_VALID, outer_ipv4_valid);
    status |= it->first.set_exact(
        smi_id::F_TUNNEL_REWRITE_ENCAP_DSCP_IPV6_VALID, outer_ipv6_valid);
    status |= it->first.set_exact(
        smi_id::F_TUNNEL_REWRITE_ENCAP_DSCP_INNER_IPV4_VALID, false);
    status |= it->first.set_exact(
        smi_id::F_TUNNEL_REWRITE_ENCAP_DSCP_INNER_IPV6_VALID, true);
    switch (qos_mode.enumdata) {
      case SWITCH_TUNNEL_ATTR_ENCAP_QOS_MODE_PIPE_MODEL:
        if (outer_ipv4_valid) {
          it->second.init_action_data(
              smi_id::A_TUNNEL_REWRITE_ENCAP_DSCP_V6_IN_V4_ECN);
        } else {
          it->second.init_action_data(
              smi_id::A_TUNNEL_REWRITE_ENCAP_DSCP_V6_IN_V6_ECN);
        }
        break;
      case SWITCH_TUNNEL_ATTR_ENCAP_QOS_MODE_UNIFORM_MODEL:
        if (outer_ipv4_valid) {
          it->second.init_action_data(
              smi_id::A_TUNNEL_REWRITE_ENCAP_DSCP_V6_IN_V4_UNIFORM);
        } else {
          it->second.init_action_data(
              smi_id::A_TUNNEL_REWRITE_ENCAP_DSCP_V6_IN_V6_UNIFORM);
        }
        break;
    }

    // For non-ip traffic dscp_mode will be pipe_mode
    if (feature::is_feature_set(SWITCH_FEATURE_L2_VXLAN)) {
      it = match_action_list.insert(
          it,
          std::pair<_MatchKey, _ActionEntry>(
              _MatchKey(smi_id::T_TUNNEL_REWRITE_ENCAP_DSCP),
              _ActionEntry(smi_id::T_TUNNEL_REWRITE_ENCAP_DSCP)));
      status |= it->first.set_exact(
          smi_id::F_TUNNEL_REWRITE_ENCAP_DSCP_LOCAL_MD_TUNNEL_INDEX, tunnel_id);
      status |= it->first.set_exact(
          smi_id::F_TUNNEL_REWRITE_ENCAP_DSCP_IPV4_VALID, outer_ipv4_valid);
      status |= it->first.set_exact(
          smi_id::F_TUNNEL_REWRITE_ENCAP_DSCP_IPV6_VALID, outer_ipv6_valid);
      status |= it->first.set_exact(
          smi_id::F_TUNNEL_REWRITE_ENCAP_DSCP_INNER_IPV4_VALID, false);
      status |= it->first.set_exact(
          smi_id::F_TUNNEL_REWRITE_ENCAP_DSCP_INNER_IPV6_VALID, false);

      if (outer_ipv4_valid) {
        it->second.init_action_data(
            smi_id::A_TUNNEL_REWRITE_ENCAP_DSCP_V4_PIPE);
        status |= it->second.set_arg(
            smi_id::P_TUNNEL_REWRITE_ENCAP_DSCP_V4_PIPE_DSCP_VAL,
            parent,
            SWITCH_TUNNEL_ATTR_DSCP_VAL);
      } else {
        it->second.init_action_data(
            smi_id::A_TUNNEL_REWRITE_ENCAP_DSCP_V6_PIPE);
        status |= it->second.set_arg(
            smi_id::P_TUNNEL_REWRITE_ENCAP_DSCP_V6_PIPE_DSCP_VAL,
            parent,
            SWITCH_TUNNEL_ATTR_DSCP_VAL);
      }
    }
  }
};

class vrf_to_vni_mapping : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_VRF_TO_VNI_MAPPING;
  static const switch_attr_id_t status_attr_id =
      SWITCH_VRF_TO_VNI_MAPPING_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_VRF_TO_VNI_MAPPING_ATTR_PARENT_HANDLE;
  switch_object_id_t vrf_handle{};

 public:
  vrf_to_vni_mapping(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_VRF_TO_VNI_MAPPING,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_enum_t mapper_type{};
    uint32_t vni = 0;

    switch_store::v_get(
        parent, SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TYPE, mapper_type);
    if (mapper_type.enumdata !=
        SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TYPE_VRF_HANDLE_TO_VNI) {
      clear_attrs();
      return;
    }
    switch_store::v_get(
        parent, SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_NETWORK_HANDLE, vrf_handle);
    if (switch_store::object_type_query(vrf_handle) != SWITCH_OBJECT_TYPE_VRF) {
      status |= SWITCH_STATUS_INVALID_PARAMETER;
      return;
    }

    status |= switch_store::v_get(
        parent, SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TUNNEL_VNI, vni);
    status |= match_key.set_exact(smi_id::F_VRF_TO_VNI_MAPPING_LOCAL_MD_VRF,
                                  vrf_handle);

    action_entry.init_action_data(smi_id::A_VRF_TO_VNI_MAPPING_SET_VNI);
    status |= action_entry.set_arg(smi_id::P_VRF_SET_PROPERTIES_VNI, vni);
  }
};

#define MY_SID_ENTRY                                              \
  std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_MY_SID), \
                                     _ActionEntry(smi_id::T_MY_SID))

class my_sid_entry : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_MY_SID;
  static const switch_attr_id_t status_attr_id = SWITCH_MY_SID_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_MY_SID_ATTR_PARENT_HANDLE;

 public:
  my_sid_entry(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action_list(
            smi_id::T_MY_SID, status_attr_id, auto_ot, parent_attr_id, parent) {
    switch_object_id_t vrf_handle = {}, nexthop_handle = {};
    switch_object_id_t sid_vrf_handle = {};
    switch_ip_address_t sid;
    switch_ip_address_t sid_mask = {};
    switch_enum_t ep_type = {}, ep_flavor = {};
    const size_t usid_block_len = 4;  // 4 x uint8_t
    const size_t usid_id_len = 2;     // 2 x uint8_t
    switch_ip6_t zero_ip = {0};

    status |= switch_store::v_get(
        parent, SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE, ep_type);
    status |= switch_store::v_get(
        parent, SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_FLAVOR, ep_flavor);
    status |= switch_store::v_get(parent, SWITCH_MY_SID_ENTRY_ATTR_SID, sid);
    status |= switch_store::v_get(
        parent, SWITCH_MY_SID_ENTRY_ATTR_VRF_HANDLE, vrf_handle);
    status |= switch_store::v_get(
        parent, SWITCH_MY_SID_ENTRY_ATTR_NEXTHOP_HANDLE, nexthop_handle);
    status |= switch_store::v_get(
        parent, SWITCH_MY_SID_ENTRY_ATTR_SID_VRF_HANDLE, sid_vrf_handle);
    switch_enum_t packet_action;
    status |= switch_store::v_get(
        parent, SWITCH_MY_SID_ENTRY_ATTR_PACKET_ACTION, packet_action);

    if (ep_type.enumdata == SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE_END_UN)
      // mask only uSID block and uSID ID (Active uSID) bits
      memset(&sid_mask.ip6, 0xFF, (usid_block_len + usid_id_len));
    else if (ep_type.enumdata == SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE_END_UA)
      // mask uSID block, uSID ID (Active uSID) and Next uSID bits
      memset(&sid_mask.ip6, 0xFF, (usid_block_len + (2 * usid_id_len)));
    else
      memset(&sid_mask, 0xFF, sizeof(switch_ip_address_t));

    sid_mask.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6;

    if (sid.addr_family != SWITCH_IP_ADDR_FAMILY_IPV6) return;

    typedef struct entries_ {
      uint8_t seg_left;
      uint8_t seg_left_mask;
    } entries;

    entries ignore_seglen = {0, 0x0};
    entries zero_seglen = {0, 0xff};  // USD
    entries one_seglen = {1, 0xff};   // PSP

    auto it = match_action_list.begin();
    it = match_action_list.insert(it, MY_SID_ENTRY);
    status |=
        it->first.set_ternary(smi_id::F_MY_SID_IPV6_DST_ADDR, sid, sid_mask);
    status |=
        it->first.set_exact(smi_id::F_MY_SID_VRF, compute_vrf(sid_vrf_handle));
    status |= it->first.set_ternary(smi_id::F_MY_SID_SRH_SEG_LEFT,
                                    ignore_seglen.seg_left,
                                    ignore_seglen.seg_left_mask);
    uint16_t prio = 1;
    bool srh_valid = 1;
    bool uSID = 0;

    if (packet_action.enumdata == SWITCH_MY_SID_ENTRY_ATTR_PACKET_ACTION_TRAP) {
      it->second.init_action_data(smi_id::A_ENDPOINT_ACTION_TRAP);
      return;
    } else if (packet_action.enumdata ==
               SWITCH_MY_SID_ENTRY_ATTR_PACKET_ACTION_DROP) {
      it->second.init_action_data(smi_id::A_ENDPOINT_ACTION_DROP);
      return;
    }
    switch (ep_type.enumdata) {
      case SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE_END_UN:
        // verify if uSID is correct
        // last 128 - 8*(usid_block_len + usid_id_len) should be 0
        if (memcmp(&sid.ip6[usid_block_len + usid_id_len],
                   zero_ip,
                   IPV6_LEN - (usid_block_len + usid_id_len)) != 0) {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_MY_SID,
                     "{}.{}: Error while creating uSID entry. Invalid uSID. "
                     "Last bits should be 0",
                     __func__,
                     __LINE__);
          status = SWITCH_STATUS_INVALID_PARAMETER;
        }

        it->second.init_action_data(smi_id::A_ENDPOINT_ACTION_END_UN);
        status |= it->first.set_exact(
            smi_id::F_MY_SID_MATCH_PRIORITY,
            (uint16_t)(prio + 3));  // priority aware of end-of-carrier cases
        uSID = 1;
      /* fall through */
      case SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE_END:
        // End + USD
        if ((ep_flavor.enumdata ==
             SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_FLAVOR_USD) ||
            (ep_flavor.enumdata ==
             SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_FLAVOR_PSP_AND_USD)) {
          if (uSID) {
            // for end-of-carrier case mask also Next uSID bits
            memset(&sid_mask.ip6, 0xFF, (usid_block_len + 2 * usid_id_len));
            it = match_action_list.insert(it, MY_SID_ENTRY);
            status |= it->first.set_ternary(
                smi_id::F_MY_SID_IPV6_DST_ADDR, sid, sid_mask);
            status |= it->first.set_exact(smi_id::F_MY_SID_VRF,
                                          compute_vrf(sid_vrf_handle));
          }
          status |= it->first.set_ternary(smi_id::F_MY_SID_SRH_SEG_LEFT,
                                          zero_seglen.seg_left,
                                          zero_seglen.seg_left_mask);
          status |=
              it->first.set_exact(smi_id::F_MY_SID_MATCH_PRIORITY, prio++);
          it->second.init_action_data(smi_id::A_ENDPOINT_ACTION_END_WITH_USD);
        }
        // End + PSP
        if ((ep_flavor.enumdata ==
             SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_FLAVOR_PSP) ||
            (ep_flavor.enumdata ==
             SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_FLAVOR_PSP_AND_USD)) {
          it = match_action_list.insert(it, MY_SID_ENTRY);
          status |= it->first.set_ternary(
              smi_id::F_MY_SID_IPV6_DST_ADDR, sid, sid_mask);
          status |= it->first.set_exact(smi_id::F_MY_SID_VRF,
                                        compute_vrf(sid_vrf_handle));
          status |= it->first.set_ternary(smi_id::F_MY_SID_SRH_SEG_LEFT,
                                          one_seglen.seg_left,
                                          one_seglen.seg_left_mask);
          status |=
              it->first.set_exact(smi_id::F_MY_SID_MATCH_PRIORITY, prio++);
          it->second.init_action_data(smi_id::A_ENDPOINT_ACTION_END_WITH_PSP);
        }
        // End
        it = match_action_list.insert(it, MY_SID_ENTRY);
        status |= it->first.set_ternary(
            smi_id::F_MY_SID_IPV6_DST_ADDR, sid, sid_mask);
        status |= it->first.set_exact(smi_id::F_MY_SID_VRF,
                                      compute_vrf(sid_vrf_handle));
        status |= it->first.set_ternary(smi_id::F_MY_SID_SRH_SEG_LEFT,
                                        ignore_seglen.seg_left,
                                        ignore_seglen.seg_left_mask);
        status |= it->first.set_exact(smi_id::F_MY_SID_MATCH_PRIORITY, prio++);
        it->second.init_action_data(smi_id::A_ENDPOINT_ACTION_END);

        break;
      case SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE_END_UA:
        // verify if uSID is correct
        // last 128 - 8*(usid_block_len + 2 * usid_id_len) should be 0
        if (memcmp(&sid.ip6[usid_block_len + (2 * usid_id_len)],
                   zero_ip,
                   IPV6_LEN - (usid_block_len + (2 * usid_id_len))) != 0) {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_MY_SID,
                     "{}.{}: Error while creating uSID entry. Invalid uSID. "
                     "Last bits should be 0",
                     __func__,
                     __LINE__);
          status = SWITCH_STATUS_INVALID_PARAMETER;
        }
        it->second.init_action_data(smi_id::A_ENDPOINT_ACTION_END_UA);
        status |= it->second.set_arg(smi_id::D_ENDPOINT_ACTION_END_UA_NEXTHOP,
                                     compute_nexthop_index(nexthop_handle));
        status |= it->first.set_exact(
            smi_id::F_MY_SID_MATCH_PRIORITY,
            (uint16_t)(prio + 3));  // priority aware of end-of-carrier cases
        uSID = 1;
      /* fall through */
      case SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE_END_X:
        // End.X + USD
        if ((ep_flavor.enumdata ==
             SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_FLAVOR_USD) ||
            (ep_flavor.enumdata ==
             SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_FLAVOR_PSP_AND_USD)) {
          if (uSID) {
            // TO DO
            // for end-of-carrier case mask also two? Next uSID bits
            memset(&sid_mask.ip6, 0xFF, (usid_block_len + 3 * usid_id_len));
            it = match_action_list.insert(it, MY_SID_ENTRY);
            status |= it->first.set_ternary(
                smi_id::F_MY_SID_IPV6_DST_ADDR, sid, sid_mask);
            status |= it->first.set_exact(smi_id::F_MY_SID_VRF,
                                          compute_vrf(sid_vrf_handle));
          }
          status |= it->first.set_ternary(smi_id::F_MY_SID_SRH_SEG_LEFT,
                                          zero_seglen.seg_left,
                                          zero_seglen.seg_left_mask);
          status |=
              it->first.set_exact(smi_id::F_MY_SID_MATCH_PRIORITY, prio++);
          it->second.init_action_data(smi_id::A_ENDPOINT_ACTION_END_X_WITH_USD);
          status |= it->second.set_arg(
              smi_id::D_ENDPOINT_ACTION_END_X_WITH_USD_NEXTHOP,
              compute_nexthop_index(nexthop_handle));
        }
        // End.X + PSP
        if ((ep_flavor.enumdata ==
             SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_FLAVOR_PSP) ||
            (ep_flavor.enumdata ==
             SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_FLAVOR_PSP_AND_USD)) {
          it = match_action_list.insert(it, MY_SID_ENTRY);
          status |= it->first.set_ternary(
              smi_id::F_MY_SID_IPV6_DST_ADDR, sid, sid_mask);
          status |= it->first.set_exact(smi_id::F_MY_SID_VRF,
                                        compute_vrf(sid_vrf_handle));
          status |= it->first.set_ternary(smi_id::F_MY_SID_SRH_SEG_LEFT,
                                          one_seglen.seg_left,
                                          one_seglen.seg_left_mask);
          status |=
              it->first.set_exact(smi_id::F_MY_SID_MATCH_PRIORITY, prio++);
          it->second.init_action_data(smi_id::A_ENDPOINT_ACTION_END_X_WITH_PSP);
          status |= it->second.set_arg(
              smi_id::D_ENDPOINT_ACTION_END_X_WITH_PSP_NEXTHOP,
              compute_nexthop_index(nexthop_handle));
        }
        // End.X
        it = match_action_list.insert(it, MY_SID_ENTRY);
        status |= it->first.set_ternary(
            smi_id::F_MY_SID_IPV6_DST_ADDR, sid, sid_mask);
        status |= it->first.set_exact(smi_id::F_MY_SID_VRF,
                                      compute_vrf(sid_vrf_handle));
        status |= it->first.set_ternary(smi_id::F_MY_SID_SRH_SEG_LEFT,
                                        ignore_seglen.seg_left,
                                        ignore_seglen.seg_left_mask);
        status |= it->first.set_exact(smi_id::F_MY_SID_MATCH_PRIORITY, prio++);
        it->second.init_action_data(smi_id::A_ENDPOINT_ACTION_END_X);
        status |= it->second.set_arg(smi_id::D_ENDPOINT_ACTION_END_X_NEXTHOP,
                                     compute_nexthop_index(nexthop_handle));
        break;
      case SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE_END_T:
        it->second.init_action_data(smi_id::A_ENDPOINT_ACTION_END_T);
        status |= it->second.set_arg(smi_id::D_ENDPOINT_ACTION_END_T_VRF,
                                     compute_vrf(vrf_handle));
        break;
      case SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE_END_DT4:
        // SRH valid, SL=0, DT4
        status |=
            it->first.set_exact(smi_id::F_MY_SID_SRH_HDR_VALID, srh_valid);
        status |= it->first.set_ternary(smi_id::F_MY_SID_SRH_SEG_LEFT,
                                        zero_seglen.seg_left,
                                        zero_seglen.seg_left_mask);
        status |= it->first.set_exact(smi_id::F_MY_SID_MATCH_PRIORITY, prio++);
        it->second.init_action_data(smi_id::A_ENDPOINT_ACTION_END_DT4);
        status |= it->second.set_arg(smi_id::D_ENDPOINT_ACTION_END_DT4_VRF,
                                     compute_vrf(vrf_handle));
        // SRH valid, SL=don't care, drop.
        // Can be modified to trap for icmp error with kernel srv6 support
        it = match_action_list.insert(it, MY_SID_ENTRY);
        status |= it->first.set_ternary(
            smi_id::F_MY_SID_IPV6_DST_ADDR, sid, sid_mask);
        status |= it->first.set_exact(smi_id::F_MY_SID_VRF,
                                      compute_vrf(sid_vrf_handle));
        status |=
            it->first.set_exact(smi_id::F_MY_SID_SRH_HDR_VALID, srh_valid);
        status |= it->first.set_ternary(smi_id::F_MY_SID_SRH_SEG_LEFT,
                                        ignore_seglen.seg_left,
                                        ignore_seglen.seg_left_mask);
        status |= it->first.set_exact(smi_id::F_MY_SID_MATCH_PRIORITY, prio++);
        it->second.init_action_data(smi_id::A_ENDPOINT_ACTION_DROP);
        // SRH invalid, SL=0, DT4
        srh_valid = 0;
        it = match_action_list.insert(it, MY_SID_ENTRY);
        status |= it->first.set_ternary(
            smi_id::F_MY_SID_IPV6_DST_ADDR, sid, sid_mask);
        status |= it->first.set_exact(smi_id::F_MY_SID_VRF,
                                      compute_vrf(sid_vrf_handle));
        status |=
            it->first.set_exact(smi_id::F_MY_SID_SRH_HDR_VALID, srh_valid);
        status |= it->first.set_exact(smi_id::F_MY_SID_MATCH_PRIORITY, prio++);
        it->second.init_action_data(smi_id::A_ENDPOINT_ACTION_END_DT4);
        status |= it->second.set_arg(smi_id::D_ENDPOINT_ACTION_END_DT4_VRF,
                                     compute_vrf(vrf_handle));
        break;
      case SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE_END_DT6:
        // SRH valid, SL=0, DT6
        status |=
            it->first.set_exact(smi_id::F_MY_SID_SRH_HDR_VALID, srh_valid);
        status |= it->first.set_ternary(smi_id::F_MY_SID_SRH_SEG_LEFT,
                                        zero_seglen.seg_left,
                                        zero_seglen.seg_left_mask);
        status |= it->first.set_exact(smi_id::F_MY_SID_MATCH_PRIORITY, prio++);
        it->second.init_action_data(smi_id::A_ENDPOINT_ACTION_END_DT6);
        status |= it->second.set_arg(smi_id::D_ENDPOINT_ACTION_END_DT6_VRF,
                                     compute_vrf(vrf_handle));
        // SRH valid, SL=don't care, drop.
        // Can be modified to trap for icmp error with kernel srv6 support
        it = match_action_list.insert(it, MY_SID_ENTRY);
        status |= it->first.set_ternary(
            smi_id::F_MY_SID_IPV6_DST_ADDR, sid, sid_mask);
        status |= it->first.set_exact(smi_id::F_MY_SID_VRF,
                                      compute_vrf(sid_vrf_handle));
        status |=
            it->first.set_exact(smi_id::F_MY_SID_SRH_HDR_VALID, srh_valid);
        status |= it->first.set_ternary(smi_id::F_MY_SID_SRH_SEG_LEFT,
                                        ignore_seglen.seg_left,
                                        ignore_seglen.seg_left_mask);
        status |= it->first.set_exact(smi_id::F_MY_SID_MATCH_PRIORITY, prio++);
        it->second.init_action_data(smi_id::A_ENDPOINT_ACTION_DROP);
        // SRH invalid, SL=0, DT6
        srh_valid = 0;
        it = match_action_list.insert(it, MY_SID_ENTRY);
        status |= it->first.set_ternary(
            smi_id::F_MY_SID_IPV6_DST_ADDR, sid, sid_mask);
        status |= it->first.set_exact(smi_id::F_MY_SID_VRF,
                                      compute_vrf(sid_vrf_handle));
        status |=
            it->first.set_exact(smi_id::F_MY_SID_SRH_HDR_VALID, srh_valid);
        status |= it->first.set_exact(smi_id::F_MY_SID_MATCH_PRIORITY, prio++);
        it->second.init_action_data(smi_id::A_ENDPOINT_ACTION_END_DT6);
        status |= it->second.set_arg(smi_id::D_ENDPOINT_ACTION_END_DT6_VRF,
                                     compute_vrf(vrf_handle));
        break;
      case SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE_END_DT46:
        // SRH valid, SL=0, DT46
        status |=
            it->first.set_exact(smi_id::F_MY_SID_SRH_HDR_VALID, srh_valid);
        status |= it->first.set_ternary(smi_id::F_MY_SID_SRH_SEG_LEFT,
                                        zero_seglen.seg_left,
                                        zero_seglen.seg_left_mask);
        status |= it->first.set_exact(smi_id::F_MY_SID_MATCH_PRIORITY, prio++);
        it->second.init_action_data(smi_id::A_ENDPOINT_ACTION_END_DT46);
        status |= it->second.set_arg(smi_id::D_ENDPOINT_ACTION_END_DT46_VRF,
                                     compute_vrf(vrf_handle));
        // SRH valid, SL=don't care, drop.
        // Can be modified to trap for icmp error with kernel srv6 support
        it = match_action_list.insert(it, MY_SID_ENTRY);
        status |= it->first.set_ternary(
            smi_id::F_MY_SID_IPV6_DST_ADDR, sid, sid_mask);
        status |= it->first.set_exact(smi_id::F_MY_SID_VRF,
                                      compute_vrf(sid_vrf_handle));
        status |=
            it->first.set_exact(smi_id::F_MY_SID_SRH_HDR_VALID, srh_valid);
        status |= it->first.set_ternary(smi_id::F_MY_SID_SRH_SEG_LEFT,
                                        ignore_seglen.seg_left,
                                        ignore_seglen.seg_left_mask);
        status |= it->first.set_exact(smi_id::F_MY_SID_MATCH_PRIORITY, prio++);
        it->second.init_action_data(smi_id::A_ENDPOINT_ACTION_DROP);
        // SRH invalid, DT46
        srh_valid = 0;
        it = match_action_list.insert(it, MY_SID_ENTRY);
        status |= it->first.set_ternary(
            smi_id::F_MY_SID_IPV6_DST_ADDR, sid, sid_mask);
        status |= it->first.set_exact(smi_id::F_MY_SID_VRF,
                                      compute_vrf(sid_vrf_handle));
        status |=
            it->first.set_exact(smi_id::F_MY_SID_SRH_HDR_VALID, srh_valid);
        status |= it->first.set_exact(smi_id::F_MY_SID_MATCH_PRIORITY, prio++);
        it->second.init_action_data(smi_id::A_ENDPOINT_ACTION_END_DT46);
        status |= it->second.set_arg(smi_id::D_ENDPOINT_ACTION_END_DT46_VRF,
                                     compute_vrf(vrf_handle));
        break;
      case SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE_END_DX4:
        it->second.init_action_data(smi_id::A_ENDPOINT_ACTION_END_DX4);
        status |= it->second.set_arg(smi_id::D_ENDPOINT_ACTION_END_DX4_NEXTHOP,
                                     compute_nexthop_index(nexthop_handle));
        break;
      case SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE_END_DX6:
        it->second.init_action_data(smi_id::A_ENDPOINT_ACTION_END_DX6);
        status |= it->second.set_arg(smi_id::D_ENDPOINT_ACTION_END_DX6_NEXTHOP,
                                     compute_nexthop_index(nexthop_handle));
        break;
      case SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE_END_B6_ENCAPS_RED:
        it->second.init_action_data(
            smi_id::A_ENDPOINT_ACTION_END_B6_ENCAPS_RED);
        status |= it->second.set_arg(
            smi_id::D_ENDPOINT_ACTION_END_B6_ENCAPS_RED_NEXTHOP,
            compute_nexthop_index(nexthop_handle));
        break;
      case SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE_END_B6_INSERT_RED:
        it->second.init_action_data(
            smi_id::A_ENDPOINT_ACTION_END_B6_INSERT_RED);
        status |= it->second.set_arg(
            smi_id::D_ENDPOINT_ACTION_END_B6_INSERT_RED_NEXTHOP,
            compute_nexthop_index(nexthop_handle));
        break;
    }
  }

  switch_status_t counters_get(const switch_object_id_t sid_handle,
                               std::vector<switch_counter_t> &counters) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t pkts = 0, bytes = 0;
    switch_counter_t cntr_pkts = {.counter_id =
                                      SWITCH_MY_SID_ENTRY_COUNTER_ID_PKTS};
    switch_counter_t cntr_bytes = {.counter_id =
                                       SWITCH_MY_SID_ENTRY_COUNTER_ID_BYTES};
    switch_enum_t ep_type = {};
    status = switch_store::v_get(
        sid_handle, SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE, ep_type);

    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_MY_SID,
                 "{}.{}: Couldn't get my_sid endpoint type",
                 __func__,
                 __LINE__);
      return status;
    }

    p4_object_match_action_list::data_get();

    for (auto const &entry : match_action_list) {
      switch (ep_type.enumdata) {
        case SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE_END_UN:
        /* fall through */
        case SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE_END:
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_PKTS,
                               smi_id::A_ENDPOINT_ACTION_END,
                               &pkts);
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_BYTES,
                               smi_id::A_ENDPOINT_ACTION_END,
                               &bytes);
          break;
        case SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE_END_UA:
        /* fall through */
        case SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE_END_X:
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_PKTS,
                               smi_id::A_ENDPOINT_ACTION_END_X,
                               &pkts);
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_BYTES,
                               smi_id::A_ENDPOINT_ACTION_END_X,
                               &bytes);
          break;
        case SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE_END_T:
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_PKTS,
                               smi_id::A_ENDPOINT_ACTION_END_T,
                               &pkts);
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_BYTES,
                               smi_id::A_ENDPOINT_ACTION_END_T,
                               &bytes);
          break;
        case SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE_END_DT4:
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_PKTS,
                               smi_id::A_ENDPOINT_ACTION_END_DT4,
                               &pkts);
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_BYTES,
                               smi_id::A_ENDPOINT_ACTION_END_DT4,
                               &bytes);
          break;
        case SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE_END_DT6:
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_PKTS,
                               smi_id::A_ENDPOINT_ACTION_END_DT6,
                               &pkts);
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_BYTES,
                               smi_id::A_ENDPOINT_ACTION_END_DT6,
                               &bytes);
          break;
        case SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE_END_DT46:
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_PKTS,
                               smi_id::A_ENDPOINT_ACTION_END_DT46,
                               &pkts);
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_BYTES,
                               smi_id::A_ENDPOINT_ACTION_END_DT46,
                               &bytes);
          break;
        case SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE_END_DX4:
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_PKTS,
                               smi_id::A_ENDPOINT_ACTION_END_DX4,
                               &pkts);
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_BYTES,
                               smi_id::A_ENDPOINT_ACTION_END_DX4,
                               &bytes);
          break;
        case SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE_END_DX6:
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_PKTS,
                               smi_id::A_ENDPOINT_ACTION_END_DX6,
                               &pkts);
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_BYTES,
                               smi_id::A_ENDPOINT_ACTION_END_DX6,
                               &bytes);
          break;
        case SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE_END_B6_ENCAPS_RED:
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_PKTS,
                               smi_id::A_ENDPOINT_ACTION_END_B6_ENCAPS_RED,
                               &pkts);
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_BYTES,
                               smi_id::A_ENDPOINT_ACTION_END_B6_ENCAPS_RED,
                               &bytes);
          break;
        case SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE_END_B6_INSERT_RED:
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_PKTS,
                               smi_id::A_ENDPOINT_ACTION_END_B6_INSERT_RED,
                               &pkts);
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_BYTES,
                               smi_id::A_ENDPOINT_ACTION_END_B6_INSERT_RED,
                               &bytes);
          break;
        default:
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_MY_SID,
                     "{}.{}: Unknown entry type",
                     __func__,
                     __LINE__);
          return SWITCH_STATUS_INVALID_PARAMETER;
      }
      cntr_pkts.count += pkts;
      cntr_bytes.count += bytes;
    }
    counters[SWITCH_MY_SID_ENTRY_COUNTER_ID_PKTS] = cntr_pkts;
    counters[SWITCH_MY_SID_ENTRY_COUNTER_ID_BYTES] = cntr_bytes;

    return status;
  }

  switch_status_t counters_set(const switch_object_id_t handle) {
    (void)handle;
    uint64_t value = 0;
    for (auto &entry : match_action_list) {
      entry.second.set_arg(smi_id::D_MY_SID_COUNTER_SPEC_PKTS, value);
      entry.second.set_arg(smi_id::D_MY_SID_COUNTER_SPEC_BYTES, value);
    }
    return p4_object_match_action_list::data_set();
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    (void)handle;
    uint64_t value = 0;
    p4_object_match_action_list::data_get();

    for (auto &entry : match_action_list) {
      for (auto cntr_id : cntr_ids) {
        switch (cntr_id) {
          case SWITCH_MY_SID_ENTRY_COUNTER_ID_PKTS:
            entry.second.set_arg(smi_id::D_MY_SID_COUNTER_SPEC_PKTS, value);
            break;
          case SWITCH_MY_SID_ENTRY_COUNTER_ID_BYTES:
            entry.second.set_arg(smi_id::D_MY_SID_COUNTER_SPEC_BYTES, value);
            break;
          default:
            switch_log(SWITCH_API_LEVEL_ERROR,
                       SWITCH_OBJECT_TYPE_MY_SID,
                       "{}.{}: Invalid counter ID",
                       __func__,
                       __LINE__);
            return SWITCH_STATUS_INVALID_PARAMETER;
        }
      }
    }
    return p4_object_match_action_list::data_set();
  }

  switch_status_t counters_save(const switch_object_id_t parent) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t cntr_pkts = 0, cntr_bytes = 0;

    if (get_auto_oid() == 0) return status;

    switch_enum_t ep_type = {};
    status = switch_store::v_get(
        parent, SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE, ep_type);

    // MAU counter
    std::vector<uint64_t> cntr_list;
    p4_object_match_action_list::data_get();

    for (auto const &entry : match_action_list) {
      switch (ep_type.enumdata) {
        case SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE_END_UN:
        /* fall through */
        case SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE_END:
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_PKTS,
                               smi_id::A_ENDPOINT_ACTION_END,
                               &cntr_pkts);
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_BYTES,
                               smi_id::A_ENDPOINT_ACTION_END,
                               &cntr_bytes);
          break;
        case SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE_END_UA:
        /* fall through */
        case SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE_END_X:
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_PKTS,
                               smi_id::A_ENDPOINT_ACTION_END_X,
                               &cntr_pkts);
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_BYTES,
                               smi_id::A_ENDPOINT_ACTION_END_X,
                               &cntr_bytes);
          break;
        case SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE_END_T:
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_PKTS,
                               smi_id::A_ENDPOINT_ACTION_END_T,
                               &cntr_pkts);
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_BYTES,
                               smi_id::A_ENDPOINT_ACTION_END_T,
                               &cntr_bytes);
          break;
        case SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE_END_DT4:
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_PKTS,
                               smi_id::A_ENDPOINT_ACTION_END_DT4,
                               &cntr_pkts);
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_BYTES,
                               smi_id::A_ENDPOINT_ACTION_END_DT4,
                               &cntr_bytes);
          break;
        case SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE_END_DT6:
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_PKTS,
                               smi_id::A_ENDPOINT_ACTION_END_DT6,
                               &cntr_pkts);
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_BYTES,
                               smi_id::A_ENDPOINT_ACTION_END_DT6,
                               &cntr_bytes);
          break;
        case SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE_END_DT46:
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_PKTS,
                               smi_id::A_ENDPOINT_ACTION_END_DT46,
                               &cntr_pkts);
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_BYTES,
                               smi_id::A_ENDPOINT_ACTION_END_DT46,
                               &cntr_bytes);
          break;
        case SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE_END_DX4:
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_PKTS,
                               smi_id::A_ENDPOINT_ACTION_END_DX4,
                               &cntr_pkts);
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_BYTES,
                               smi_id::A_ENDPOINT_ACTION_END_DX4,
                               &cntr_bytes);
          break;
        case SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE_END_DX6:
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_PKTS,
                               smi_id::A_ENDPOINT_ACTION_END_DX6,
                               &cntr_pkts);
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_BYTES,
                               smi_id::A_ENDPOINT_ACTION_END_DX6,
                               &cntr_bytes);
          break;
        case SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE_END_B6_ENCAPS_RED:
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_PKTS,
                               smi_id::A_ENDPOINT_ACTION_END_B6_ENCAPS_RED,
                               &cntr_pkts);
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_BYTES,
                               smi_id::A_ENDPOINT_ACTION_END_B6_ENCAPS_RED,
                               &cntr_bytes);
          break;
        case SWITCH_MY_SID_ENTRY_ATTR_ENDPOINT_TYPE_END_B6_INSERT_RED:
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_PKTS,
                               smi_id::A_ENDPOINT_ACTION_END_B6_INSERT_RED,
                               &cntr_pkts);
          entry.second.get_arg(smi_id::D_MY_SID_COUNTER_SPEC_BYTES,
                               smi_id::A_ENDPOINT_ACTION_END_B6_INSERT_RED,
                               &cntr_bytes);
          break;
        default:
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_MY_SID,
                     "{}.{}: Unknown entry type",
                     __func__,
                     __LINE__);
          return SWITCH_STATUS_INVALID_PARAMETER;
      }
      cntr_list.push_back(static_cast<uint64_t>(cntr_pkts));
      cntr_list.push_back(static_cast<uint64_t>(cntr_bytes));
    }
    attr_w cntr_attr_list(SWITCH_MY_SID_ATTR_MAU_STATS_CACHE);
    cntr_attr_list.v_set(cntr_list);
    switch_store::attribute_set(get_auto_oid(), cntr_attr_list);

    return status;
  }

  switch_status_t counters_restore(switch_object_id_t parent) {
    (void)parent;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    size_t list_i;

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> cntr_list;
    status |= switch_store::v_get(
        get_auto_oid(), SWITCH_MY_SID_ATTR_MAU_STATS_CACHE, cntr_list);
    if (cntr_list.empty()) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_MY_SID,
                 "{}.{}: No stat cache to restore mau stats, "
                 "my_sid cache list empty, status {}",
                 __func__,
                 __LINE__,
                 status);
      return SWITCH_STATUS_SUCCESS;
    }

    list_i = 0;
    for (auto &entry : match_action_list) {
      entry.second.set_arg(smi_id::D_MY_SID_COUNTER_SPEC_PKTS,
                           cntr_list[list_i]);
      list_i++;
      entry.second.set_arg(smi_id::D_MY_SID_COUNTER_SPEC_BYTES,
                           cntr_list[list_i]);
      list_i++;
    }
    status = p4_object_match_action_list::data_set();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_MY_SID,
                 "{}.{}: Fail to restore mau stats cache,  "
                 "my_sid status {} ",
                 __func__,
                 __LINE__,
                 status);
    }

    return status;
  }
};

switch_status_t before_my_sid_update(const switch_object_id_t handle,
                                     const attr_w &attr) {
  (void)attr;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::unique_ptr<object> mobject;

  mobject = std::unique_ptr<my_sid_entry>(new my_sid_entry(handle, status));
  if (mobject != NULL) {
    mobject->del();
  }
  return status;
}

switch_status_t after_my_sid_update(const switch_object_id_t handle,
                                    const attr_w &attr) {
  (void)attr;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::unique_ptr<object> mobject;

  mobject = std::unique_ptr<my_sid_entry>(new my_sid_entry(handle, status));
  if (mobject != NULL) {
    mobject->create_update();
  }
  return status;
}

static switch_status_t get_tunnel_terms_by_overlay_vrf(
    const switch_object_id_t vrf, std::set<switch_object_id_t> &objs) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::vector<switch_object_id_t> terms;

  status |= switch_store::object_get_all_handles(SWITCH_OBJECT_TYPE_TUNNEL_TERM,
                                                 terms);

  for (auto term : terms) {
    switch_enum_t tunnel_type = {};
    switch_object_id_t tunnel = {}, orif = {}, ovrf = {};

    status |= switch_store::v_get(
        term, SWITCH_TUNNEL_TERM_ATTR_TUNNEL_HANDLE, tunnel);
    status |=
        switch_store::v_get(term, SWITCH_TUNNEL_TERM_ATTR_TYPE, tunnel_type);

    if ((tunnel_type.enumdata != SWITCH_TUNNEL_TERM_ATTR_TYPE_IPIP) &&
        (tunnel_type.enumdata != SWITCH_TUNNEL_TERM_ATTR_TYPE_IPGRE)) {
      continue;
    }

    status |= switch_store::v_get(
        tunnel, SWITCH_TUNNEL_ATTR_OVERLAY_RIF_HANDLE, orif);
    if (!orif.data) {
      continue;
    }

    status |= switch_store::v_get(orif, SWITCH_RIF_ATTR_VRF_HANDLE, ovrf);
    if (vrf == ovrf) {
      objs.insert(term);
    }
  }

  return status;
}

static switch_status_t get_tunnel_mapper_entries_by_vrf(
    const switch_object_id_t vrf, std::set<switch_object_id_t> &objs) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::vector<switch_object_id_t> entries;

  status |= switch_store::object_get_all_handles(
      SWITCH_OBJECT_TYPE_TUNNEL_MAPPER_ENTRY, entries);

  for (auto entry : entries) {
    switch_object_id_t network_handle = {}, rif = {}, netvrf = {};
    switch_object_type_t ot = 0;

    status |= switch_store::v_get(
        entry, SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_NETWORK_HANDLE, network_handle);
    ot = switch_store::object_type_query(network_handle);
    if (ot == SWITCH_OBJECT_TYPE_VRF) {
      netvrf = network_handle;
    } else if (ot == SWITCH_OBJECT_TYPE_VLAN) {
      std::vector<switch_object_id_t> rif_handles;
      status |= switch_store::v_get(
          network_handle, SWITCH_VLAN_ATTR_RIF_HANDLES, rif_handles);
      if (rif_handles.size() > 0) {
        rif = rif_handles[0];
        status |= switch_store::v_get(rif, SWITCH_RIF_ATTR_VRF_HANDLE, netvrf);
      }
    }

    if (vrf == netvrf) {
      objs.insert(entry);
    }
  }

  return status;
}

switch_status_t after_vrf_update(const switch_object_id_t vrf,
                                 const attr_w &attr) {
  (void)attr;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::set<switch_object_id_t> objs;

  status |= get_tunnel_terms_by_overlay_vrf(vrf, objs);

  for (auto term : objs) {
    switch_status_t obj_status = SWITCH_STATUS_SUCCESS;

    ipv4_dst_vtep obj{term, obj_status};
    if (obj_status != SWITCH_STATUS_SUCCESS) {
      status |= obj_status;
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_VRF,
                 "{}.{}: Failed ipv4_dst_vtep::ipv4_dst_vtep, "
                 "status {} ",
                 __func__,
                 __LINE__,
                 obj_status);
      continue;
    }

    obj_status = obj.create_update();
    if (obj_status != SWITCH_STATUS_SUCCESS) {
      status |= obj_status;
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_VRF,
                 "{}.{}: Failed ipv4_dst_vtep::create_update, "
                 "status {} ",
                 __func__,
                 __LINE__,
                 obj_status);
      continue;
    }
  }

  objs.clear();
  status |= get_tunnel_mapper_entries_by_vrf(vrf, objs);

  for (auto entry : objs) {
    switch_status_t obj_status = SWITCH_STATUS_SUCCESS;

    vni_to_bd_mapping obj{entry, obj_status};
    if (obj_status != SWITCH_STATUS_SUCCESS) {
      status |= obj_status;
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_VRF,
                 "{}.{}: Failed vni_to_bd_mapping::vni_to_bd_mapping, "
                 "status {} ",
                 __func__,
                 __LINE__,
                 obj_status);
      continue;
    }
    obj_status = obj.create_update();
    if (obj_status != SWITCH_STATUS_SUCCESS) {
      status |= obj_status;
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_VRF,
                 "{}.{}: Failed vni_to_bd_mapping::create_update, "
                 "status {} ",
                 __func__,
                 __LINE__,
                 obj_status);
      continue;
    }
  }

  return status;
}

switch_status_t before_tunnel_create(const switch_object_type_t object_type,
                                     std::set<attr_w> &attrs) {
  (void)object_type;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  uint32_t alloc_tunnel_id = 0;
  uint16_t tunnel_id = 0;
  switch_enum_t tunnel_peer_mode = {.enumdata =
                                        SWITCH_TUNNEL_ATTR_PEER_MODE_P2MP};
  switch_ip_address_t tunnel_dst_ip = {};
  switch_object_id_t device_handle = {}, rif_handle = {}, uvrf_handle = {},
                     tunnel_dst_ip_handle = {};

  const auto tunnel_peer_mode_it =
      attrs.find(static_cast<switch_attr_id_t>(SWITCH_TUNNEL_ATTR_PEER_MODE));

  if (tunnel_peer_mode_it != attrs.end()) {
    status |= tunnel_peer_mode_it->v_get(tunnel_peer_mode);
    if (status != SWITCH_STATUS_SUCCESS) return status;
  }

  if (tunnel_peer_mode.enumdata != SWITCH_TUNNEL_ATTR_PEER_MODE_P2P) {
    // allocate tunnel_id
    const auto tunnel_id_attrs_it =
        attrs.find(static_cast<switch_attr_id_t>(SWITCH_TUNNEL_ATTR_TUNNEL_ID));
    if (tunnel_id_attrs_it == attrs.end()) {
      alloc_tunnel_id = SWITCH_CONTEXT.tunnel_index_allocate();
      tunnel_id = static_cast<uint16_t>(alloc_tunnel_id);

      const auto insert_ret =
          attrs.insert(attr_w(SWITCH_TUNNEL_ATTR_TUNNEL_ID, tunnel_id));
      CHECK_RET(insert_ret.second == false, SWITCH_STATUS_FAILURE);
    } else {
      status |= (*tunnel_id_attrs_it).v_get(tunnel_id);
      SWITCH_CONTEXT.tunnel_index_reserve(tunnel_id);
    }

    return status;
  }

  if (!feature::is_feature_set(SWITCH_FEATURE_L2_VXLAN) ||
      switch_store::smiContext::context().in_warm_init())
    return status;

  // find referenced tunnel_dest_ip object, create if not found
  const auto tunnel_dst_ip_it =
      attrs.find(static_cast<switch_attr_id_t>(SWITCH_TUNNEL_ATTR_DST_IP));
  CHECK_RET(tunnel_dst_ip_it == attrs.end(), SWITCH_STATUS_FAILURE);
  status |= tunnel_dst_ip_it->v_get(tunnel_dst_ip);

  const auto tunnel_device_it =
      attrs.find(static_cast<switch_attr_id_t>(SWITCH_TUNNEL_ATTR_DEVICE));
  CHECK_RET(tunnel_device_it == attrs.end(), SWITCH_STATUS_FAILURE);
  status |= tunnel_device_it->v_get(device_handle);

  const auto tunnel_underlay_rif_it = attrs.find(
      static_cast<switch_attr_id_t>(SWITCH_TUNNEL_ATTR_UNDERLAY_RIF_HANDLE));
  CHECK_RET(tunnel_underlay_rif_it == attrs.end(), SWITCH_STATUS_FAILURE);
  status |= tunnel_underlay_rif_it->v_get(rif_handle);
  status |=
      switch_store::v_get(rif_handle, SWITCH_RIF_ATTR_VRF_HANDLE, uvrf_handle);
  CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);

  std::set<attr_w> lookup_attrs;
  lookup_attrs.insert(attr_w(SWITCH_TUNNEL_DEST_IP_ATTR_DEVICE, device_handle));
  lookup_attrs.insert(
      attr_w(SWITCH_TUNNEL_DEST_IP_ATTR_UNDERLAY_VRF_HANDLE, uvrf_handle));
  lookup_attrs.insert(
      attr_w(SWITCH_TUNNEL_DEST_IP_ATTR_DEST_IP, tunnel_dst_ip));
  status |= switch_store::object_id_get_wkey(
      SWITCH_OBJECT_TYPE_TUNNEL_DEST_IP, lookup_attrs, tunnel_dst_ip_handle);
  if (status == SWITCH_STATUS_ITEM_NOT_FOUND) {
    status = switch_store::object_create(
        SWITCH_OBJECT_TYPE_TUNNEL_DEST_IP, lookup_attrs, tunnel_dst_ip_handle);
  }
  CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);

  const auto tunnel_dst_ip_handle_it = attrs.find(
      static_cast<switch_attr_id_t>(SWITCH_TUNNEL_ATTR_DST_IP_HANDLE));
  if (tunnel_dst_ip_handle_it != attrs.end()) {
    attrs.erase(tunnel_dst_ip_handle_it);
  }
  attrs.insert(attr_w(SWITCH_TUNNEL_ATTR_DST_IP_HANDLE, tunnel_dst_ip_handle));
  attrs.insert(attr_w(SWITCH_TUNNEL_ATTR_DST_IP_INDEX,
                      switch_store::handle_to_id(tunnel_dst_ip_handle)));
  return status;
}

switch_status_t after_tunnel_delete(const switch_object_type_t object_type,
                                    const std::set<attr_w> &attrs) {
  (void)object_type;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  uint16_t alloc_tunnel_id = 0;
  switch_enum_t tunnel_peer_mode = {.enumdata =
                                        SWITCH_TUNNEL_ATTR_PEER_MODE_P2MP};
  switch_object_id_t tunnel_dst_ip_handle = {};

  // release tunnel_id
  const auto tunnel_id_attrs_it =
      attrs.find(static_cast<switch_attr_id_t>(SWITCH_TUNNEL_ATTR_TUNNEL_ID));
  if (tunnel_id_attrs_it != attrs.end()) {
    status |= (*tunnel_id_attrs_it).v_get(alloc_tunnel_id);
  }
  if (alloc_tunnel_id) {
    status |= SWITCH_CONTEXT.tunnel_index_release(alloc_tunnel_id);
  }

  if (!feature::is_feature_set(SWITCH_FEATURE_L2_VXLAN) ||
      switch_store::smiContext::context().in_warm_init())
    return status;

  const auto tunnel_peer_mode_it =
      attrs.find(static_cast<switch_attr_id_t>(SWITCH_TUNNEL_ATTR_PEER_MODE));
  if (tunnel_peer_mode_it != attrs.end()) {
    status |= tunnel_peer_mode_it->v_get(tunnel_peer_mode);
  }
  CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);

  if (tunnel_peer_mode.enumdata != SWITCH_TUNNEL_ATTR_PEER_MODE_P2P)
    return status;

  // delete tunnel_dest_ip if no other objects refer to it
  const auto tunnel_dst_ip_handle_it = attrs.find(
      static_cast<switch_attr_id_t>(SWITCH_TUNNEL_ATTR_DST_IP_HANDLE));
  if (tunnel_dst_ip_handle_it != attrs.end()) {
    status |= tunnel_dst_ip_handle_it->v_get(tunnel_dst_ip_handle);
  }
  CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);
  if (tunnel_dst_ip_handle.data == 0) return status;

  auto const &nexthops_set = switch_store::get_object_references(
      tunnel_dst_ip_handle, SWITCH_OBJECT_TYPE_NEXTHOP);
  auto const &tunnels_set = switch_store::get_object_references(
      tunnel_dst_ip_handle, SWITCH_OBJECT_TYPE_TUNNEL);
  if (nexthops_set.size() == 0 && tunnels_set.size() == 0) {
    status |= switch_store::object_delete(tunnel_dst_ip_handle);
  }
  return status;
}

switch_status_t before_tunnel_mapper_create(
    const switch_object_type_t object_type, std::set<attr_w> &attrs) {
  (void)object_type;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  uint32_t alloc_tunnel_mapper_id = 0;
  uint16_t tunnel_mapper_id = 0;
  switch_enum_t tunnel_mapper_type = {
      .enumdata = SWITCH_TUNNEL_MAPPER_ATTR_TYPE_VLAN_HANDLE_TO_VNI};

  const auto tunnel_mapper_type_it =
      attrs.find(static_cast<switch_attr_id_t>(SWITCH_TUNNEL_MAPPER_ATTR_TYPE));

  if (tunnel_mapper_type_it != attrs.end()) {
    status |= tunnel_mapper_type_it->v_get(tunnel_mapper_type);
    if (status != SWITCH_STATUS_SUCCESS) return status;
  }

  if (tunnel_mapper_type.enumdata ==
      SWITCH_TUNNEL_MAPPER_ATTR_TYPE_VLAN_HANDLE_TO_VNI) {
    // allocate tunnel_mapper_id
    const auto tunnel_mapper_id_attrs_it =
        attrs.find(static_cast<switch_attr_id_t>(
            SWITCH_TUNNEL_MAPPER_ATTR_TUNNEL_MAPPER_ID));
    if (tunnel_mapper_id_attrs_it == attrs.end()) {
      alloc_tunnel_mapper_id = SWITCH_CONTEXT.tunnel_mapper_index_allocate();
      tunnel_mapper_id = static_cast<uint16_t>(alloc_tunnel_mapper_id);

      const auto insert_ret = attrs.insert(
          attr_w(SWITCH_TUNNEL_MAPPER_ATTR_TUNNEL_MAPPER_ID, tunnel_mapper_id));
      CHECK_RET(insert_ret.second == false, SWITCH_STATUS_FAILURE);
    } else {
      status |= (*tunnel_mapper_id_attrs_it).v_get(tunnel_mapper_id);
      SWITCH_CONTEXT.tunnel_mapper_index_reserve(tunnel_mapper_id);
    }
  }
  return status;
}

switch_status_t after_tunnel_mapper_delete(
    const switch_object_type_t object_type, const std::set<attr_w> &attrs) {
  (void)object_type;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  uint16_t alloc_tunnel_mapper_id = 0;

  // release tunnel_mapper_id
  const auto tunnel_mapper_id_attrs_it =
      attrs.find(static_cast<switch_attr_id_t>(
          SWITCH_TUNNEL_MAPPER_ATTR_TUNNEL_MAPPER_ID));
  if (tunnel_mapper_id_attrs_it != attrs.end()) {
    status |= (*tunnel_mapper_id_attrs_it).v_get(alloc_tunnel_mapper_id);
  }
  if (alloc_tunnel_mapper_id) {
    status |=
        SWITCH_CONTEXT.tunnel_mapper_index_release(alloc_tunnel_mapper_id);
  }
  return status;
}

switch_status_t tunnel_init() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  status |= switch_store::reg_create_trigs_before(SWITCH_OBJECT_TYPE_TUNNEL,
                                                  &before_tunnel_create);
  status |= switch_store::reg_delete_trigs_after(SWITCH_OBJECT_TYPE_TUNNEL,
                                                 &after_tunnel_delete);
  status |= switch_store::reg_create_trigs_before(
      SWITCH_OBJECT_TYPE_TUNNEL_MAPPER, &before_tunnel_mapper_create);
  status |= switch_store::reg_delete_trigs_after(
      SWITCH_OBJECT_TYPE_TUNNEL_MAPPER, &after_tunnel_mapper_delete);

  // REGISTER_OBJECT(ipv4_src_vtep, SWITCH_OBJECT_TYPE_IPV4_SRC_VTEP);
  REGISTER_OBJECT(ipv4_dst_vtep, SWITCH_OBJECT_TYPE_IPV4_DST_VTEP);
  // REGISTER_OBJECT(ipv6_src_vtep, SWITCH_OBJECT_TYPE_IPV6_SRC_VTEP);
  REGISTER_OBJECT(ipv6_dst_vtep, SWITCH_OBJECT_TYPE_IPV6_DST_VTEP);
  REGISTER_OBJECT(vni_to_bd_mapping, SWITCH_OBJECT_TYPE_VNI_TO_BD_MAPPING);
  REGISTER_OBJECT(bd_to_vni_mapping, SWITCH_OBJECT_TYPE_BD_TO_VNI_MAPPING);
  REGISTER_OBJECT(tunnel_src_addr_rewrite,
                  SWITCH_OBJECT_TYPE_TUNNEL_SRC_ADDR_REWRITE);
  REGISTER_OBJECT(tunnel_dst_addr_rewrite,
                  SWITCH_OBJECT_TYPE_TUNNEL_DST_ADDR_REWRITE);
  REGISTER_OBJECT(tunnel_replication_resolution,
                  SWITCH_OBJECT_TYPE_TUNNEL_REPLICATION_RESOLUTION);
  REGISTER_OBJECT(outer_ecmp_selector_group,
                  SWITCH_OBJECT_TYPE_OUTER_ECMP_SELECTOR_GROUP);
  REGISTER_OBJECT(outer_ecmp_membership,
                  SWITCH_OBJECT_TYPE_OUTER_ECMP_MEMBERSHIP);
  REGISTER_OBJECT(outer_ecmp_selector, SWITCH_OBJECT_TYPE_OUTER_ECMP_SELECTOR);
  REGISTER_OBJECT(outer_fib_table, SWITCH_OBJECT_TYPE_OUTER_FIB_TABLE);
  REGISTER_OBJECT(egress_vrf_mapping, SWITCH_OBJECT_TYPE_EGRESS_VRF_MAPPING);
  REGISTER_OBJECT(tunnel_encap_ttl, SWITCH_OBJECT_TYPE_TUNNEL_ENCAP_TTL);
  REGISTER_OBJECT(tunnel_encap_dscp, SWITCH_OBJECT_TYPE_TUNNEL_ENCAP_DSCP);
  REGISTER_OBJECT(vrf_to_vni_mapping, SWITCH_OBJECT_TYPE_VRF_TO_VNI_MAPPING);
  REGISTER_OBJECT(my_sid_entry, SWITCH_OBJECT_TYPE_MY_SID);
  status |= switch_store::reg_update_trigs_before(
      SWITCH_OBJECT_TYPE_MY_SID_ENTRY, &before_my_sid_update);
  status |= switch_store::reg_update_trigs_after(
      SWITCH_OBJECT_TYPE_MY_SID_ENTRY, &after_my_sid_update);
  status |= switch_store::reg_update_trigs_after(SWITCH_OBJECT_TYPE_VRF,
                                                 &after_vrf_update);

  return status;
}

switch_status_t tunnel_clean() { return SWITCH_STATUS_SUCCESS; }

} /* namespace smi */
