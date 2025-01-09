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
#include "switch_tna/utils.h"
#include "switch_tna/p4_16_types.h"

namespace smi {
using namespace smi::bf_rt;  // NOLINT(build/namespaces)
using ::smi::logging::switch_log;

class neighbor_rewrite : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_NEIGHBOR_REWRITE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_NEIGHBOR_REWRITE_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_NEIGHBOR_REWRITE_ATTR_PARENT_HANDLE;

 public:
  neighbor_rewrite(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_NEIGHBOR,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_object_id_t neighbor_handle = {0}, nexthop_handle = {0};
    switch_enum_t nhop_type = {0};
    switch_object_type_t parent_ot = switch_store::object_type_query(parent);

    if (parent_ot == SWITCH_OBJECT_TYPE_NEXTHOP_RESOLUTION) {
      status |=
          switch_store::v_get(parent,
                              SWITCH_NEXTHOP_RESOLUTION_ATTR_NEIGHBOR_HANDLE,
                              neighbor_handle);
      status |= switch_store::v_get(
          parent, SWITCH_NEXTHOP_RESOLUTION_ATTR_PARENT_HANDLE, nexthop_handle);
      if (switch_store::object_type_query(nexthop_handle) !=
          SWITCH_OBJECT_TYPE_NEXTHOP) {
        clear_attrs();
        return;
      }
    } else if (parent_ot == SWITCH_OBJECT_TYPE_NEXTHOP) {
      nexthop_handle = parent;
    } else {
      switch_log(
          SWITCH_API_LEVEL_ERROR,
          SWITCH_OBJECT_TYPE_NEXTHOP,
          "{}:{}: Invalid parent object {:#x} type {} of neighbor_rewrite",
          __func__,
          __LINE__,
          parent.data,
          switch_store::object_name_get_from_type(parent_ot));
      clear_attrs();
      return;
    }
    if (nexthop_handle.data == 0) {
      clear_attrs();
      return;
    }
    status |= switch_store::v_get(
        nexthop_handle, SWITCH_NEXTHOP_ATTR_TYPE, nhop_type);
    if (nhop_type.enumdata != SWITCH_NEXTHOP_ATTR_TYPE_IP &&
        nhop_type.enumdata != SWITCH_NEXTHOP_ATTR_TYPE_MPLS) {
      clear_attrs();
      return;
    }

    status |= match_key.set_exact(smi_id::F_NEIGHBOR_LOCAL_MD_NEXTHOP,
                                  compute_nexthop_index(parent));
    if (parent_ot == SWITCH_OBJECT_TYPE_NEXTHOP_RESOLUTION) {
      if (neighbor_handle.data == 0) return;
      action_entry.init_action_data(smi_id::A_NEIGHBOR_REWRITE_L2);
      status |= action_entry.set_arg(smi_id::P_NEIGHBOR_REWRITE_L2_DMAC,
                                     neighbor_handle,
                                     SWITCH_NEIGHBOR_ATTR_MAC_ADDRESS);
    } else if (parent_ot == SWITCH_OBJECT_TYPE_NEXTHOP) {
      action_entry.init_action_data(smi_id::A_NEIGHBOR_REWRITE_L2);
      status |= action_entry.set_arg(smi_id::P_NEIGHBOR_REWRITE_L2_DMAC,
                                     nexthop_handle,
                                     SWITCH_NEXTHOP_ATTR_MAC_ADDRESS);
    }
    return;
  }
};

class outer_nexthop : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_OUTER_NEXTHOP;
  static const switch_attr_id_t status_attr_id =
      SWITCH_OUTER_NEXTHOP_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_OUTER_NEXTHOP_ATTR_PARENT_HANDLE;

 public:
  outer_nexthop(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_OUTER_NEXTHOP,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_object_id_t neighbor_handle{}, nexthop_handle{}, rif_handle{},
        vlan_handle{}, bd_handle{};
    switch_enum_t nhop_type{}, rif_type{};
    switch_object_type_t parent_ot = switch_store::object_type_query(parent);

    if (parent_ot == SWITCH_OBJECT_TYPE_NEXTHOP_RESOLUTION) {
      status |=
          switch_store::v_get(parent,
                              SWITCH_NEXTHOP_RESOLUTION_ATTR_NEIGHBOR_HANDLE,
                              neighbor_handle);
      status |= switch_store::v_get(
          parent, SWITCH_NEXTHOP_RESOLUTION_ATTR_PARENT_HANDLE, nexthop_handle);
      if (switch_store::object_type_query(nexthop_handle) !=
          SWITCH_OBJECT_TYPE_NEXTHOP) {
        clear_attrs();
        return;
      }
    } else if (parent_ot == SWITCH_OBJECT_TYPE_NEXTHOP) {
      nexthop_handle = parent;
    } else {
      switch_log(
          SWITCH_API_LEVEL_ERROR,
          SWITCH_OBJECT_TYPE_NEXTHOP,
          "{}:{}: Invalid parent object {:#x} type {} of outer_nexthop object",
          __func__,
          __LINE__,
          parent.data,
          switch_store::object_name_get_from_type(parent_ot));
      clear_attrs();
      return;
    }

    if (nexthop_handle.data == 0) {
      clear_attrs();
      return;
    }
    status |= switch_store::v_get(
        nexthop_handle, SWITCH_NEXTHOP_ATTR_TYPE, nhop_type);

    if (nhop_type.enumdata != SWITCH_NEXTHOP_ATTR_TYPE_IP &&
        nhop_type.enumdata != SWITCH_NEXTHOP_ATTR_TYPE_MPLS) {
      clear_attrs();
      return;
    }
    status |= match_key.set_exact(smi_id::F_OUTER_NEXTHOP_LOCAL_MD_NEXTHOP,
                                  compute_nexthop_index(parent));

    if (parent_ot == SWITCH_OBJECT_TYPE_NEXTHOP_RESOLUTION) {
      if (neighbor_handle.data == 0) return;
    }
    status |= switch_store::v_get(
        nexthop_handle, SWITCH_NEXTHOP_ATTR_HANDLE, rif_handle);
    if (rif_handle.data == 0) return;
    status |= switch_store::v_get(rif_handle, SWITCH_RIF_ATTR_TYPE, rif_type);
    if ((rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_QINQ_PORT) ||
        (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_PORT) ||
        (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_SUB_PORT)) {
      status |= find_auto_oid(rif_handle, SWITCH_OBJECT_TYPE_BD, bd_handle);
    } else if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_VLAN) {
      status |= switch_store::v_get(
          rif_handle, SWITCH_RIF_ATTR_VLAN_HANDLE, vlan_handle);
      status |= find_auto_oid(vlan_handle, SWITCH_OBJECT_TYPE_BD, bd_handle);
    }
    action_entry.init_action_data(smi_id::A_OUTER_NEXTHOP_REWRITE_L2);
    status |= action_entry.set_arg(smi_id::P_OUTER_NEXTHOP_REWRITE_L2_BD,
                                   compute_bd(bd_handle));

    return;
  }
};

class tunnel_nexthop : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_TUNNEL_NEXTHOP;
  static const switch_attr_id_t status_attr_id =
      SWITCH_TUNNEL_NEXTHOP_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_TUNNEL_NEXTHOP_ATTR_PARENT_HANDLE;
  switch_tunnel_mode_t switch_nexthop_mpls_encap_ttl_mode_to_tunnel_mode(
      switch_enum_t switch_tunnel_mode) {
    switch (switch_tunnel_mode.enumdata) {
      case SWITCH_NEXTHOP_ATTR_MPLS_ENCAP_TTL_MODE_UNIFORM_MODEL:
        return SWITCH_TUNNEL_MODE_UNIFORM;
        break;
      case SWITCH_NEXTHOP_ATTR_MPLS_ENCAP_TTL_MODE_PIPE_MODEL:
        return SWITCH_TUNNEL_MODE_PIPE;
        break;
      default:
        return SWITCH_TUNNEL_MODE_UNIFORM;
    }
  }
  switch_tunnel_mode_t switch_nexthop_mpls_encap_qos_mode_to_tunnel_mode(
      switch_enum_t switch_tunnel_mode) {
    switch (switch_tunnel_mode.enumdata) {
      case SWITCH_NEXTHOP_ATTR_MPLS_ENCAP_QOS_MODE_UNIFORM_MODEL:
        return SWITCH_TUNNEL_MODE_UNIFORM;
        break;
      case SWITCH_NEXTHOP_ATTR_MPLS_ENCAP_QOS_MODE_PIPE_MODEL:
        return SWITCH_TUNNEL_MODE_PIPE;
        break;
      default:
        return SWITCH_TUNNEL_MODE_UNIFORM;
    }
  }

 public:
  bool create_p4_entry = true;
  tunnel_nexthop(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_TUNNEL_NEXTHOP,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_object_id_t nexthop_handle = {0}, tunnel_handle = {0},
                       sidlist_handle = {0}, nr_parent_handle = {0},
                       device_handle = {0};
    switch_ip_address_t src_ip = {};
    switch_enum_t nhop_type = {0}, tunnel_type = {0}, tunnel_rw_type = {0},
                  sidlist_type = {0}, mapper_type = {0};
    std::vector<switch_object_id_t> ingress_mapper_handles;
    switch_ip_address_t nhop_ip_addr = {};
    switch_ip_addr_family_t ip_family = SWITCH_IP_ADDR_FAMILY_IPV4;
    uint64_t dest_ip_index;
    uint16_t tunnel_id;
    uint16_t tunnel_mapper_id = 0;
    uint32_t tunnel_vni = 0;
    switch_object_type_t parent_ot = switch_store::object_type_query(parent);
    switch_object_type_t nr_parent_ot;

    if (parent_ot == SWITCH_OBJECT_TYPE_NEXTHOP_RESOLUTION) {
      status |=
          switch_store::v_get(parent,
                              SWITCH_NEXTHOP_RESOLUTION_ATTR_PARENT_HANDLE,
                              nr_parent_handle);
      nr_parent_ot = switch_store::object_type_query(nr_parent_handle);
      if (nr_parent_ot == SWITCH_OBJECT_TYPE_NEXTHOP) {
        nexthop_handle = nr_parent_handle;
      } else if (nr_parent_ot == SWITCH_OBJECT_TYPE_TUNNEL) {
        status |= switch_store::v_get(
            nr_parent_handle, SWITCH_TUNNEL_ATTR_DST_IP_INDEX, dest_ip_index);
        status |= switch_store::v_get(nr_parent_handle,
                                      SWITCH_TUNNEL_ATTR_INGRESS_MAPPER_HANDLES,
                                      ingress_mapper_handles);
        for (const auto handle : ingress_mapper_handles) {
          status |= switch_store::v_get(
              handle, SWITCH_TUNNEL_MAPPER_ATTR_TYPE, mapper_type);
          if (mapper_type.enumdata ==
              SWITCH_TUNNEL_MAPPER_ATTR_TYPE_VLAN_HANDLE_TO_VNI) {
            status |=
                switch_store::v_get(handle,
                                    SWITCH_TUNNEL_MAPPER_ATTR_TUNNEL_MAPPER_ID,
                                    tunnel_mapper_id);
          }
        }

        tunnel_rw_type.enumdata = SWITCH_NEXTHOP_ATTR_RW_TYPE_L2;
        status |=
            find_corresponding_p2mp_tunnel(nr_parent_handle, tunnel_handle);

        status |= switch_store::v_get(
            nr_parent_handle, SWITCH_TUNNEL_ATTR_SRC_IP, src_ip);
        ip_family = src_ip.addr_family;

      } else {
        switch_log(
            SWITCH_API_LEVEL_ERROR,
            SWITCH_OBJECT_TYPE_NEXTHOP,
            "{}:{}: Invalid parent object {:#x} type {} of {} object {:#x}",
            __func__,
            __LINE__,
            nr_parent_handle.data,
            switch_store::object_name_get_from_type(nr_parent_ot),
            switch_store::object_name_get_from_type(parent_ot),
            parent.data);
        clear_attrs();
        return;
      }
    } else if (parent_ot == SWITCH_OBJECT_TYPE_NEXTHOP) {
      nexthop_handle = parent;
    } else {
      switch_log(
          SWITCH_API_LEVEL_ERROR,
          SWITCH_OBJECT_TYPE_NEXTHOP,
          "{}:{}: Invalid parent object {:#x} type {} of tunnel_nexthop object",
          __func__,
          __LINE__,
          parent.data,
          switch_store::object_name_get_from_type(parent_ot));
      clear_attrs();
      return;
    }

    status |=
        match_key.set_exact(smi_id::F_TUNNEL_NEXTHOP_LOCAL_MD_TUNNEL_NEXTHOP,
                            compute_nexthop_index(parent));

    if (nexthop_handle.data != 0) {
      status |= switch_store::v_get(
          nexthop_handle, SWITCH_NEXTHOP_ATTR_TYPE, nhop_type);
      if (nhop_type.enumdata != SWITCH_NEXTHOP_ATTR_TYPE_TUNNEL &&
          nhop_type.enumdata != SWITCH_NEXTHOP_ATTR_TYPE_SRV6_SIDLIST &&
          nhop_type.enumdata != SWITCH_NEXTHOP_ATTR_TYPE_MPLS &&
          nhop_type.enumdata != SWITCH_NEXTHOP_ATTR_TYPE_IP) {
        clear_attrs();
        return;
      }

      if (nhop_type.enumdata == SWITCH_NEXTHOP_ATTR_TYPE_IP) {
        switch_object_id_t rif_handle = {0};
        switch_enum_t rif_type = {0};

        status |= switch_store::v_get(
            nexthop_handle, SWITCH_NEXTHOP_ATTR_HANDLE, rif_handle);
        switch_object_type_t rif_ot =
            switch_store::object_type_query(rif_handle);
        if (rif_ot != SWITCH_OBJECT_TYPE_RIF) {
          clear_attrs();
          return;
        }
        status |=
            switch_store::v_get(rif_handle, SWITCH_RIF_ATTR_TYPE, rif_type);
        if (rif_type.enumdata != SWITCH_RIF_ATTR_TYPE_VLAN) {
          clear_attrs();
          return;
        }
        if (parent_ot != SWITCH_OBJECT_TYPE_NEXTHOP_RESOLUTION) {
          clear_attrs();
          return;
        }
        switch_object_id_t mac_handle = {}, destination_handle = {};
        status |=
            switch_store::v_get(parent,
                                SWITCH_NEXTHOP_RESOLUTION_ATTR_MAC_ENTRY_HANDLE,
                                mac_handle);

        if (mac_handle.data != 0) {
          status |=
              switch_store::v_get(mac_handle,
                                  SWITCH_MAC_ENTRY_ATTR_DESTINATION_HANDLE,
                                  destination_handle);

          if (switch_store::object_type_query(destination_handle) ==
              SWITCH_OBJECT_TYPE_TUNNEL) {
            status |= find_corresponding_p2mp_tunnel(destination_handle,
                                                     tunnel_handle);

            status |= switch_store::v_get(destination_handle,
                                          SWITCH_TUNNEL_ATTR_DST_IP_INDEX,
                                          dest_ip_index);
            status |= switch_store::v_get(
                destination_handle, SWITCH_TUNNEL_ATTR_DST_IP, nhop_ip_addr);
            ip_family = nhop_ip_addr.addr_family;
            status |= switch_store::v_get(
                destination_handle, SWITCH_TUNNEL_ATTR_DEVICE, device_handle);
            tunnel_rw_type.enumdata = SWITCH_NEXTHOP_ATTR_RW_TYPE_L3_VNI;

            switch_object_id_t vlan_handle;
            uint16_t vlan_id;

            status |= switch_store::v_get(
                rif_handle, SWITCH_RIF_ATTR_VLAN_HANDLE, vlan_handle);
            status |= switch_store::v_get(
                vlan_handle, SWITCH_VLAN_ATTR_VLAN_ID, vlan_id);
            status |=
                switch_store::v_get(tunnel_handle,
                                    SWITCH_TUNNEL_ATTR_INGRESS_MAPPER_HANDLES,
                                    ingress_mapper_handles);
            for (auto mapper : ingress_mapper_handles) {
              status |= switch_store::v_get(
                  mapper, SWITCH_TUNNEL_MAPPER_ATTR_TYPE, mapper_type);
              if (mapper_type.enumdata ==
                  SWITCH_TUNNEL_MAPPER_ATTR_TYPE_VLAN_HANDLE_TO_VNI) {
                const auto &mapper_entries =
                    switch_store::get_object_references(
                        mapper, SWITCH_OBJECT_TYPE_TUNNEL_MAPPER_ENTRY);
                for (auto mapper_entry : mapper_entries) {
                  uint16_t temp_vlan_id;
                  switch_object_id_t network_handle;
                  // if type is vlan to vni there's no need to check if network
                  // handle is not vrf
                  status |= switch_store::v_get(
                      mapper_entry.oid,
                      SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_NETWORK_HANDLE,
                      network_handle);
                  status |= switch_store::v_get(
                      network_handle, SWITCH_VLAN_ATTR_VLAN_ID, temp_vlan_id);

                  if (vlan_id == temp_vlan_id) {
                    status |= switch_store::v_get(
                        mapper_entry.oid,
                        SWITCH_TUNNEL_MAPPER_ENTRY_ATTR_TUNNEL_VNI,
                        tunnel_vni);
                  }
                }
              }
            }
          } else {
            create_p4_entry = false;
            return;
          }
        } else {
          create_p4_entry = false;
          return;
        }
      } else {
        status |= switch_store::v_get(
            nexthop_handle, SWITCH_NEXTHOP_ATTR_DEVICE, device_handle);
        status |= switch_store::v_get(
            nexthop_handle, SWITCH_NEXTHOP_ATTR_HANDLE, tunnel_handle);
        status |= switch_store::v_get(
            nexthop_handle, SWITCH_NEXTHOP_ATTR_DEST_IP, nhop_ip_addr);
        ip_family = nhop_ip_addr.addr_family;
        status |= switch_store::v_get(nexthop_handle,
                                      SWITCH_NEXTHOP_ATTR_TUNNEL_DEST_IP_INDEX,
                                      dest_ip_index);
        status |= switch_store::v_get(
            nexthop_handle, SWITCH_NEXTHOP_ATTR_TUNNEL_VNI, tunnel_vni);
        status |= switch_store::v_get(
            nexthop_handle, SWITCH_NEXTHOP_ATTR_RW_TYPE, tunnel_rw_type);
      }
    }

    if (nhop_type.enumdata == SWITCH_NEXTHOP_ATTR_TYPE_SRV6_SIDLIST) {
      std::vector<switch_ip_address_t> sidlist;
      status |= switch_store::v_get(
          nexthop_handle, SWITCH_NEXTHOP_ATTR_SRV6_SIDLIST_ID, sidlist_handle);
      status |=
          switch_store::v_get(sidlist_handle,
                              SWITCH_SEGMENTROUTE_SIDLIST_ATTR_SEGMENT_LIST,
                              sidlist);
      status |= switch_store::v_get(
          sidlist_handle, SWITCH_SEGMENTROUTE_SIDLIST_ATTR_TYPE, sidlist_type);
      status |= switch_store::v_get(
          tunnel_handle, SWITCH_TUNNEL_ATTR_TUNNEL_ID, tunnel_id);
      if (sidlist.size() == 0) return;
      if (sidlist_type.enumdata ==
          SWITCH_SEGMENTROUTE_SIDLIST_ATTR_TYPE_H_INSERT_RED) {
        action_entry.init_action_data(smi_id::A_TUNNEL_NEXTHOP_SRV6_INSERT);
        action_entry.set_arg(smi_id::P_SRV6_INSERT_SEG_LEN, sidlist.size());
      } else {
        action_entry.init_action_data(smi_id::A_TUNNEL_NEXTHOP_SRV6_ENCAP);
        action_entry.set_arg(smi_id::P_SRV6_ENCAP_SEG_LEN, sidlist.size());
        status |=
            action_entry.set_arg(smi_id::P_SRV6_ENCAP_TUNNEL_INDEX, tunnel_id);
      }
      return;
    }

    if (nhop_type.enumdata == SWITCH_NEXTHOP_ATTR_TYPE_MPLS) {
      uint8_t num_labels = 0;
      switch_enum_t labelop = {0}, ttl_mode = {0}, qos_mode = {0};
      std::vector<uint32_t> label_list;
      status |= switch_store::v_get(
          nexthop_handle, SWITCH_NEXTHOP_ATTR_LABELOP, labelop);
      status |= switch_store::v_get(
          nexthop_handle, SWITCH_NEXTHOP_ATTR_LABELSTACK, label_list);
      num_labels = label_list.size();
      uint8_t ttl;
      uint8_t exp;
      status |= switch_store::v_get(
          nexthop_handle, SWITCH_NEXTHOP_ATTR_MPLS_ENCAP_TTL_MODE, ttl_mode);
      status |= switch_store::v_get(
          nexthop_handle, SWITCH_NEXTHOP_ATTR_MPLS_ENCAP_QOS_MODE, qos_mode);
      status |= switch_store::v_get(
          nexthop_handle, SWITCH_NEXTHOP_ATTR_MPLS_ENCAP_TTL, ttl);
      status |= switch_store::v_get(
          nexthop_handle, SWITCH_NEXTHOP_ATTR_MPLS_ENCAP_EXP, exp);
      if (num_labels > 0) {
        action_entry.init_action_data(smi_id::A_TUNNEL_NEXTHOP_MPLS_PUSH);
        action_entry.set_arg(smi_id::P_MPLS_PUSH_LABEL_COUNT,
                             (uint8_t)num_labels);
        action_entry.set_arg(smi_id::P_MPLS_PUSH_ENCAP_TTL, ttl);
        action_entry.set_arg(smi_id::P_MPLS_PUSH_ENCAP_EXP, exp);
        if (labelop.enumdata == SWITCH_NEXTHOP_ATTR_LABELOP_SWAP) {
          action_entry.set_arg(smi_id::P_MPLS_PUSH_SWAP, (uint8_t)1);
        }
        if (feature::is_feature_set(SWITCH_FEATURE_TUNNEL_TTL_MODE)) {
          status |= action_entry.set_arg(
              smi_id::P_MPLS_PUSH_ENCAP_TTL_MODE,
              switch_nexthop_mpls_encap_ttl_mode_to_tunnel_mode(ttl_mode));
        }
        if (feature::is_feature_set(SWITCH_FEATURE_TUNNEL_QOS_MODE)) {
          status |= action_entry.set_arg(
              smi_id::P_MPLS_PUSH_ENCAP_QOS_MODE,
              switch_nexthop_mpls_encap_qos_mode_to_tunnel_mode(qos_mode));
        }
      }
      return;
    }

    status |= switch_store::v_get(
        tunnel_handle, SWITCH_TUNNEL_ATTR_TUNNEL_ID, tunnel_id);
    status |= switch_store::v_get(
        tunnel_handle, SWITCH_TUNNEL_ATTR_TYPE, tunnel_type);

    if (tunnel_rw_type.enumdata == SWITCH_NEXTHOP_ATTR_RW_TYPE_L2) {
      action_entry.init_action_data(smi_id::A_TUNNEL_NEXTHOP_L2_TUNNEL_ENCAP);
      status |=
          action_entry.set_arg(smi_id::P_TUNNEL_NEXTHOP_L2_TUNNEL_ENCAP_TYPE,
                               ip_family == SWITCH_IP_ADDR_FAMILY_IPV4
                                   ? static_cast<uint8_t>(SWITCH_IPV4_VXLAN)
                                   : static_cast<uint8_t>(SWITCH_IPV6_VXLAN));
      status |= action_entry.set_arg(
          smi_id::P_TUNNEL_NEXTHOP_L2_TUNNEL_ENCAP_DIP_INDEX, dest_ip_index);
      status |= action_entry.set_arg(
          smi_id::P_TUNNEL_NEXTHOP_L2_TUNNEL_ENCAP_TUNNEL_INDEX, tunnel_id);
      status |= action_entry.set_arg(
          smi_id::P_TUNNEL_NEXTHOP_L2_TUNNEL_ENCAP_TUNNEL_MAPPER_INDEX,
          tunnel_mapper_id);
    } else if (tunnel_rw_type.enumdata == SWITCH_NEXTHOP_ATTR_RW_TYPE_L3_VNI) {
      action_entry.init_action_data(
          smi_id::A_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_WITH_VNI);
      switch_mac_addr_t mac;

      switch_object_id_t neighbor_handle = {0};
      status |=
          switch_store::v_get(parent,
                              SWITCH_NEXTHOP_RESOLUTION_ATTR_NEIGHBOR_HANDLE,
                              neighbor_handle);
      if (neighbor_handle.data == 0) {
        // When nhop_type == TUNNEL, neighbor_handle is always 0
        // When nhop_type == IP,
        // nexthop_resolution, and neighbor_handle.data == 0,
        // SWITCH_NEXTHOP_ATTR_MAC_ADDRESS will not be valid
        // will fall through to tunnel dmac case below
        status |= switch_store::v_get(
            nexthop_handle, SWITCH_NEXTHOP_ATTR_MAC_ADDRESS, mac);

      } else {
        status |= switch_store::v_get(
            neighbor_handle, SWITCH_NEIGHBOR_ATTR_MAC_ADDRESS, mac);
      }

      // Use mac from nexthop/neighbor or get from device tunnel dmac
      if (SWITCH_MAC_VALID(mac)) {
        status |= action_entry.set_arg(
            smi_id::P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_WITH_VNI_DMAC, mac);
      } else {
        status |= action_entry.set_arg(
            smi_id::P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_WITH_VNI_DMAC,
            device_handle,
            SWITCH_DEVICE_ATTR_TUNNEL_DMAC);
      }
      status |= action_entry.set_arg(
          smi_id::P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_WITH_VNI_TYPE,
          ip_family == SWITCH_IP_ADDR_FAMILY_IPV4
              ? static_cast<uint8_t>(SWITCH_IPV4_VXLAN)
              : static_cast<uint8_t>(SWITCH_IPV6_VXLAN));
      status |= action_entry.set_arg(
          smi_id::P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_WITH_VNI_VNI, tunnel_vni);
      status |= action_entry.set_arg(
          smi_id::P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_WITH_VNI_DIP_INDEX,
          dest_ip_index);
      status |= action_entry.set_arg(
          smi_id::P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_WITH_VNI_TUNNEL_INDEX,
          tunnel_id);
    } else if (tunnel_rw_type.enumdata == SWITCH_NEXTHOP_ATTR_RW_TYPE_L3) {
      action_entry.init_action_data(smi_id::A_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP);
      switch_mac_addr_t mac;
      status |= switch_store::v_get(
          nexthop_handle, SWITCH_NEXTHOP_ATTR_MAC_ADDRESS, mac);
      // Use mac from nexthop/neighbor or get from device tunnel dmac
      if (SWITCH_MAC_VALID(mac)) {
        status |=
            action_entry.set_arg(smi_id::P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_DMAC,
                                 nexthop_handle,
                                 SWITCH_NEXTHOP_ATTR_MAC_ADDRESS);
      } else {
        status |=
            action_entry.set_arg(smi_id::P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_DMAC,
                                 device_handle,
                                 SWITCH_DEVICE_ATTR_TUNNEL_DMAC);
      }
      if (tunnel_type.enumdata == SWITCH_TUNNEL_ATTR_TYPE_IPIP) {
        status |=
            action_entry.set_arg(smi_id::P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_TYPE,
                                 ip_family == SWITCH_IP_ADDR_FAMILY_IPV4
                                     ? static_cast<uint8_t>(SWITCH_IPV4_IPIP)
                                     : static_cast<uint8_t>(SWITCH_IPV6_IPIP));
      } else if (tunnel_type.enumdata == SWITCH_TUNNEL_ATTR_TYPE_IPGRE) {
        status |=
            action_entry.set_arg(smi_id::P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_TYPE,
                                 ip_family == SWITCH_IP_ADDR_FAMILY_IPV4
                                     ? static_cast<uint8_t>(SWITCH_IPV4_GRE)
                                     : static_cast<uint8_t>(SWITCH_IPV6_GRE));
      } else {
        status |=
            action_entry.set_arg(smi_id::P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_TYPE,
                                 ip_family == SWITCH_IP_ADDR_FAMILY_IPV4
                                     ? static_cast<uint8_t>(SWITCH_IPV4_VXLAN)
                                     : static_cast<uint8_t>(SWITCH_IPV6_VXLAN));
      }

      status |= action_entry.set_arg(
          smi_id::P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_DIP_INDEX, dest_ip_index);
      status |= action_entry.set_arg(
          smi_id::P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_TUNNEL_INDEX, tunnel_id);
    }
  }

  switch_status_t find_corresponding_p2mp_tunnel(
      const switch_object_id_t tunnel_handle,
      switch_object_id_t &p2mp_tunnel_handle) {
    switch_object_id_t device_handle = {0}, underlay_rif_handle = {0},
                       overlay_rif_handle = {0};
    switch_enum_t tunnel_type = {0};
    switch_ip_address_t src_ip = {}, dst_ip_addr = {};
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    std::set<attr_w> lookup_attrs;

    switch_enum_t peer_mode = {.enumdata = SWITCH_TUNNEL_ATTR_PEER_MODE_P2MP};
    status |= switch_store::v_get(
        tunnel_handle, SWITCH_TUNNEL_ATTR_DEVICE, device_handle);
    status |= switch_store::v_get(
        tunnel_handle, SWITCH_TUNNEL_ATTR_TYPE, tunnel_type);
    status |= switch_store::v_get(tunnel_handle,
                                  SWITCH_TUNNEL_ATTR_UNDERLAY_RIF_HANDLE,
                                  underlay_rif_handle);
    status |= switch_store::v_get(tunnel_handle,
                                  SWITCH_TUNNEL_ATTR_OVERLAY_RIF_HANDLE,
                                  overlay_rif_handle);
    status |=
        switch_store::v_get(tunnel_handle, SWITCH_TUNNEL_ATTR_SRC_IP, src_ip);
    lookup_attrs.insert(attr_w(SWITCH_TUNNEL_ATTR_DEVICE, device_handle));
    lookup_attrs.insert(attr_w(SWITCH_TUNNEL_ATTR_TYPE, tunnel_type));
    lookup_attrs.insert(attr_w(SWITCH_TUNNEL_ATTR_PEER_MODE, peer_mode));
    lookup_attrs.insert(
        attr_w(SWITCH_TUNNEL_ATTR_UNDERLAY_RIF_HANDLE, underlay_rif_handle));
    lookup_attrs.insert(
        attr_w(SWITCH_TUNNEL_ATTR_OVERLAY_RIF_HANDLE, overlay_rif_handle));
    lookup_attrs.insert(attr_w(SWITCH_TUNNEL_ATTR_SRC_IP, src_ip));
    dst_ip_addr.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
    dst_ip_addr.ip4 = 0;
    lookup_attrs.insert(attr_w(SWITCH_TUNNEL_ATTR_DST_IP, dst_ip_addr));
    status |= switch_store::object_id_get_wkey(
        SWITCH_OBJECT_TYPE_TUNNEL, lookup_attrs, p2mp_tunnel_handle);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_TUNNEL,
                 "{}.{}:{}: Unable to find p2mp tunnel corresponding to "
                 "p2p tunnel {:#x} :{}",
                 "tunnel_nexthop",
                 __func__,
                 __LINE__,
                 tunnel_handle.data,
                 status);
    }
    return status;
  }

  switch_status_t create_update() {
    switch_status_t status;
    bool bf_rt_status = false;
    if (create_p4_entry) {
      status = p4_object_match_action::create_update();
    } else {
      status = p4_object_match_action::auto_obj.create_update();
      status |= switch_store::v_get(
          auto_obj.get_auto_oid(), status_attr_id, bf_rt_status);
      if (bf_rt_status == true) {
        status = p4_object_match_action::del_pi_only();
        if (status == SWITCH_STATUS_SUCCESS) {
          auto auto_oid = get_auto_oid();
          status |= switch_store::v_set(
              auto_oid, SWITCH_TUNNEL_NEXTHOP_ATTR_STATUS, false);
        }
      }
    }
    return status;
  }
};

class sid_rewrite : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_SID_REWRITE;
  static const switch_attr_id_t status_attr_id = SWITCH_SID_REWRITE_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_SID_REWRITE_ATTR_PARENT_HANDLE;

 public:
  sid_rewrite(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_SID_REWRITE,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_object_id_t nexthop_handle = {0}, sidlist_handle = {0};
    switch_enum_t nhop_type = {0}, sidlist_type = {0};

    if (switch_store::object_type_query(parent) == SWITCH_OBJECT_TYPE_NEXTHOP) {
      nexthop_handle = parent;
    } else {
      status |= switch_store::v_get(
          parent, SWITCH_NEXTHOP_RESOLUTION_ATTR_PARENT_HANDLE, nexthop_handle);
      if (switch_store::object_type_query(nexthop_handle) !=
          SWITCH_OBJECT_TYPE_NEXTHOP) {
        clear_attrs();
        return;
      }
    }

    status |= switch_store::v_get(
        nexthop_handle, SWITCH_NEXTHOP_ATTR_TYPE, nhop_type);

    if (nhop_type.enumdata != SWITCH_NEXTHOP_ATTR_TYPE_SRV6_SIDLIST) {
      clear_attrs();
      return;
    }

    match_key.set_exact(smi_id::F_SID_REWRITE_LOCAL_MD_TUNNEL_NEXTHOP,
                        compute_nexthop_index(nexthop_handle));

    std::vector<switch_ip_address_t> sidlist;
    status |= switch_store::v_get(
        nexthop_handle, SWITCH_NEXTHOP_ATTR_SRV6_SIDLIST_ID, sidlist_handle);
    status |= switch_store::v_get(
        sidlist_handle, SWITCH_SEGMENTROUTE_SIDLIST_ATTR_SEGMENT_LIST, sidlist);
    status |= switch_store::v_get(
        sidlist_handle, SWITCH_SEGMENTROUTE_SIDLIST_ATTR_TYPE, sidlist_type);

    if (sidlist_type.enumdata ==
        SWITCH_SEGMENTROUTE_SIDLIST_ATTR_TYPE_H_ENCAPS_RED) {
      switch (sidlist.size()) {
        case 1:
          action_entry.init_action_data(smi_id::A_SRV6_ENCAPS_SID_REWRITE_0);
          status |= action_entry.set_arg(smi_id::D_SRV6_ENCAPS_SID_REWRITE_0_S0,
                                         sidlist[0]);
          break;
        case 2:
          action_entry.init_action_data(smi_id::A_SRV6_ENCAPS_SID_REWRITE_1);
          status |= action_entry.set_arg(smi_id::D_SRV6_ENCAPS_SID_REWRITE_1_S0,
                                         sidlist[0]);
          status |= action_entry.set_arg(smi_id::D_SRV6_ENCAPS_SID_REWRITE_1_S1,
                                         sidlist[1]);
          break;
        case 3:
          action_entry.init_action_data(smi_id::A_SRV6_ENCAPS_SID_REWRITE_2);
          status |= action_entry.set_arg(smi_id::D_SRV6_ENCAPS_SID_REWRITE_2_S0,
                                         sidlist[0]);
          status |= action_entry.set_arg(smi_id::D_SRV6_ENCAPS_SID_REWRITE_2_S1,
                                         sidlist[1]);
          status |= action_entry.set_arg(smi_id::D_SRV6_ENCAPS_SID_REWRITE_2_S2,
                                         sidlist[2]);
          break;
        default:
          status = SWITCH_STATUS_FAILURE;
          return;
      }
    } else if (sidlist_type.enumdata ==
               SWITCH_SEGMENTROUTE_SIDLIST_ATTR_TYPE_H_INSERT_RED) {
      switch (sidlist.size()) {
        case 1:
          action_entry.init_action_data(smi_id::A_SRV6_INSERT_SID_REWRITE_0);
          status |= action_entry.set_arg(smi_id::D_SRV6_INSERT_SID_REWRITE_0_S0,
                                         sidlist[0]);
          break;
        case 2:
          action_entry.init_action_data(smi_id::A_SRV6_INSERT_SID_REWRITE_1);
          status |= action_entry.set_arg(smi_id::D_SRV6_INSERT_SID_REWRITE_1_S0,
                                         sidlist[0]);
          status |= action_entry.set_arg(smi_id::D_SRV6_INSERT_SID_REWRITE_1_S1,
                                         sidlist[1]);
          break;
        default:
          status = SWITCH_STATUS_FAILURE;
          return;
      }
    } else {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_SID_REWRITE,
                 "{}.{}: Invalid sidlist type",
                 __func__,
                 __LINE__);
      status = SWITCH_STATUS_INVALID_PARAMETER;
      return;
    }
  }

  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t pkts = 0, bytes = 0;

    switch_enum_t sidlist_type = {0};
    switch_object_id_t sidlist_handle = {0};
    std::vector<switch_ip_address_t> sidlist;

    status |= switch_store::v_get(
        handle, SWITCH_NEXTHOP_ATTR_SRV6_SIDLIST_ID, sidlist_handle);
    status |= switch_store::v_get(
        sidlist_handle, SWITCH_SEGMENTROUTE_SIDLIST_ATTR_SEGMENT_LIST, sidlist);
    status |= switch_store::v_get(
        sidlist_handle, SWITCH_SEGMENTROUTE_SIDLIST_ATTR_TYPE, sidlist_type);

    status = p4_object_match_action::data_get();

    if (sidlist_type.enumdata ==
        SWITCH_SEGMENTROUTE_SIDLIST_ATTR_TYPE_H_ENCAPS_RED) {
      switch (sidlist.size()) {
        case 1:
          action_entry.get_arg(smi_id::D_SRV6_SID_REWRITE_COUNTER_SPEC_PKTS,
                               smi_id::A_SRV6_ENCAPS_SID_REWRITE_0,
                               &pkts);
          action_entry.get_arg(smi_id::D_SRV6_SID_REWRITE_COUNTER_SPEC_BYTES,
                               smi_id::A_SRV6_ENCAPS_SID_REWRITE_0,
                               &bytes);
          break;
        case 2:
          action_entry.get_arg(smi_id::D_SRV6_SID_REWRITE_COUNTER_SPEC_PKTS,
                               smi_id::A_SRV6_ENCAPS_SID_REWRITE_1,
                               &pkts);
          action_entry.get_arg(smi_id::D_SRV6_SID_REWRITE_COUNTER_SPEC_BYTES,
                               smi_id::A_SRV6_ENCAPS_SID_REWRITE_1,
                               &bytes);
          break;
        case 3:
          action_entry.get_arg(smi_id::D_SRV6_SID_REWRITE_COUNTER_SPEC_PKTS,
                               smi_id::A_SRV6_ENCAPS_SID_REWRITE_2,
                               &pkts);
          action_entry.get_arg(smi_id::D_SRV6_SID_REWRITE_COUNTER_SPEC_BYTES,
                               smi_id::A_SRV6_ENCAPS_SID_REWRITE_2,
                               &bytes);
          break;
        default:
          status = SWITCH_STATUS_FAILURE;
      }
    } else if (sidlist_type.enumdata ==
               SWITCH_SEGMENTROUTE_SIDLIST_ATTR_TYPE_H_INSERT_RED) {
      switch (sidlist.size()) {
        case 1:
          action_entry.get_arg(smi_id::D_SRV6_SID_REWRITE_COUNTER_SPEC_PKTS,
                               smi_id::A_SRV6_INSERT_SID_REWRITE_0,
                               &pkts);
          action_entry.get_arg(smi_id::D_SRV6_SID_REWRITE_COUNTER_SPEC_BYTES,
                               smi_id::A_SRV6_INSERT_SID_REWRITE_0,
                               &bytes);
          break;
        case 2:
          action_entry.get_arg(smi_id::D_SRV6_SID_REWRITE_COUNTER_SPEC_PKTS,
                               smi_id::A_SRV6_INSERT_SID_REWRITE_1,
                               &pkts);
          action_entry.get_arg(smi_id::D_SRV6_SID_REWRITE_COUNTER_SPEC_BYTES,
                               smi_id::A_SRV6_INSERT_SID_REWRITE_1,
                               &bytes);
          break;
        default:
          status = SWITCH_STATUS_FAILURE;
      }
    }

    cntrs[SWITCH_SEGMENTROUTE_SIDLIST_COUNTER_ID_PKTS].count += pkts;
    cntrs[SWITCH_SEGMENTROUTE_SIDLIST_COUNTER_ID_BYTES].count += bytes;

    return status;
  }

  switch_status_t counters_set(const switch_object_id_t handle) {
    (void)handle;
    uint64_t value = 0;
    action_entry.set_arg(smi_id::D_SRV6_SID_REWRITE_COUNTER_SPEC_PKTS, value);
    action_entry.set_arg(smi_id::D_SRV6_SID_REWRITE_COUNTER_SPEC_BYTES, value);

    return p4_object_match_action::data_set();
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    (void)handle;
    uint64_t value = 0;
    p4_object_match_action::data_get();

    for (auto cntr_id : cntr_ids) {
      switch (cntr_id) {
        case SWITCH_SEGMENTROUTE_SIDLIST_COUNTER_ID_PKTS:
          action_entry.set_arg(smi_id::D_SRV6_SID_REWRITE_COUNTER_SPEC_PKTS,
                               value);
          break;
        case SWITCH_SEGMENTROUTE_SIDLIST_COUNTER_ID_BYTES:
          action_entry.set_arg(smi_id::D_SRV6_SID_REWRITE_COUNTER_SPEC_BYTES,
                               value);
          break;
        default:
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_SEGMENTROUTE_SIDLIST,
                     "{}.{}: Invalid counter ID",
                     __func__,
                     __LINE__);
          return SWITCH_STATUS_INVALID_PARAMETER;
      }
    }
    return p4_object_match_action::data_set();
  }

  switch_status_t counters_save(const switch_object_id_t parent) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t cntr_pkts = 0, cntr_bytes = 0;

    if (get_auto_oid() == 0) return status;

    switch_enum_t sidlist_type = {0};
    switch_object_id_t sidlist_handle = {0};
    std::vector<switch_ip_address_t> sidlist;

    status |= switch_store::v_get(
        parent, SWITCH_NEXTHOP_ATTR_SRV6_SIDLIST_ID, sidlist_handle);
    status |= switch_store::v_get(
        sidlist_handle, SWITCH_SEGMENTROUTE_SIDLIST_ATTR_SEGMENT_LIST, sidlist);
    status |= switch_store::v_get(
        sidlist_handle, SWITCH_SEGMENTROUTE_SIDLIST_ATTR_TYPE, sidlist_type);

    // MAU counter
    std::vector<uint64_t> cntr_list;
    status = p4_object_match_action::data_get();
    if (status == SWITCH_STATUS_SUCCESS) {
      if (sidlist_type.enumdata ==
          SWITCH_SEGMENTROUTE_SIDLIST_ATTR_TYPE_H_ENCAPS_RED) {
        switch (sidlist.size()) {
          case 1:
            action_entry.get_arg(smi_id::D_SRV6_SID_REWRITE_COUNTER_SPEC_PKTS,
                                 smi_id::A_SRV6_ENCAPS_SID_REWRITE_0,
                                 &cntr_pkts);
            action_entry.get_arg(smi_id::D_SRV6_SID_REWRITE_COUNTER_SPEC_BYTES,
                                 smi_id::A_SRV6_ENCAPS_SID_REWRITE_0,
                                 &cntr_bytes);
            break;
          case 2:
            action_entry.get_arg(smi_id::D_SRV6_SID_REWRITE_COUNTER_SPEC_PKTS,
                                 smi_id::A_SRV6_ENCAPS_SID_REWRITE_1,
                                 &cntr_pkts);
            action_entry.get_arg(smi_id::D_SRV6_SID_REWRITE_COUNTER_SPEC_BYTES,
                                 smi_id::A_SRV6_ENCAPS_SID_REWRITE_1,
                                 &cntr_bytes);
            break;
          case 3:
            action_entry.get_arg(smi_id::D_SRV6_SID_REWRITE_COUNTER_SPEC_PKTS,
                                 smi_id::A_SRV6_ENCAPS_SID_REWRITE_2,
                                 &cntr_pkts);
            action_entry.get_arg(smi_id::D_SRV6_SID_REWRITE_COUNTER_SPEC_BYTES,
                                 smi_id::A_SRV6_ENCAPS_SID_REWRITE_2,
                                 &cntr_bytes);
            break;
          default:
            status = SWITCH_STATUS_FAILURE;
        }
      } else if (sidlist_type.enumdata ==
                 SWITCH_SEGMENTROUTE_SIDLIST_ATTR_TYPE_H_INSERT_RED) {
        switch (sidlist.size()) {
          case 1:
            action_entry.get_arg(smi_id::D_SRV6_SID_REWRITE_COUNTER_SPEC_PKTS,
                                 smi_id::A_SRV6_INSERT_SID_REWRITE_0,
                                 &cntr_pkts);
            action_entry.get_arg(smi_id::D_SRV6_SID_REWRITE_COUNTER_SPEC_BYTES,
                                 smi_id::A_SRV6_INSERT_SID_REWRITE_0,
                                 &cntr_bytes);
            break;
          case 2:
            action_entry.get_arg(smi_id::D_SRV6_SID_REWRITE_COUNTER_SPEC_PKTS,
                                 smi_id::A_SRV6_INSERT_SID_REWRITE_1,
                                 &cntr_pkts);
            action_entry.get_arg(smi_id::D_SRV6_SID_REWRITE_COUNTER_SPEC_BYTES,
                                 smi_id::A_SRV6_INSERT_SID_REWRITE_1,
                                 &cntr_bytes);
            break;
          default:
            status = SWITCH_STATUS_FAILURE;
        }
      }
      cntr_list.push_back(static_cast<uint64_t>(cntr_pkts));
      cntr_list.push_back(static_cast<uint64_t>(cntr_bytes));

      attr_w cntr_attr_list(SWITCH_SID_REWRITE_ATTR_MAU_STATS_CACHE);
      cntr_attr_list.v_set(cntr_list);
      switch_store::attribute_set(get_auto_oid(), cntr_attr_list);
    }
    return status;
  }

  switch_status_t counters_restore(const switch_object_id_t parent) {
    (void)parent;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t cntr_pkts = 0, cntr_bytes = 0;

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> cntr_list;
    status |= switch_store::v_get(
        get_auto_oid(), SWITCH_SID_REWRITE_ATTR_MAU_STATS_CACHE, cntr_list);
    if (cntr_list.empty()) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_SID_REWRITE,
                 "{}.{}: No stat cache to restore mau stats, "
                 "sid_rewrite cache list empty, status {}",
                 __func__,
                 __LINE__,
                 status);
      return SWITCH_STATUS_SUCCESS;
    }

    if (cntr_list.size() == SWITCH_SEGMENTROUTE_SIDLIST_COUNTER_ID_MAX) {
      cntr_pkts = cntr_list[SWITCH_SEGMENTROUTE_SIDLIST_COUNTER_ID_PKTS];
      cntr_bytes = cntr_list[SWITCH_SEGMENTROUTE_SIDLIST_COUNTER_ID_BYTES];
    }

    action_entry.set_arg(smi_id::D_SRV6_SID_REWRITE_COUNTER_SPEC_PKTS,
                         cntr_pkts);
    action_entry.set_arg(smi_id::D_SRV6_SID_REWRITE_COUNTER_SPEC_BYTES,
                         cntr_bytes);
    status = p4_object_match_action::data_set();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_SID_REWRITE,
                 "{}.{}: Fail to restore mau stats cache,  "
                 "sid_rewrite status {} ",
                 __func__,
                 __LINE__,
                 status);
    }

    return status;
  }
};

class sidlist_stats : public auto_object {
 private:
  std::set<switch_object_id_t> nexthop_handles;

  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_SIDLIST_STATS;
  static const switch_attr_id_t status_attr_id =
      SWITCH_SIDLIST_STATS_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_SIDLIST_STATS_ATTR_PARENT_HANDLE;

 public:
  sidlist_stats(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    if (switch_store::object_type_query(parent) !=
        SWITCH_OBJECT_TYPE_SEGMENTROUTE_SIDLIST) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_SEGMENTROUTE_SIDLIST,
                 "{}.{}: Invalid parent object type",
                 __func__,
                 __LINE__);
      status = SWITCH_STATUS_INVALID_PARAMETER;
    }

    switch_store::referencing_set_get(
        parent, SWITCH_OBJECT_TYPE_NEXTHOP, nexthop_handles);
  }

  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    (void)handle;
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    for (const auto nexthop : nexthop_handles) {
      sid_rewrite rewrite_object(nexthop, status);
      rewrite_object.counters_get(nexthop, cntrs);
    }

    std::vector<uint64_t> cntr_list;
    uint64_t stored_pkts = 0, stored_bytes = 0;
    status = switch_store::v_get(
        get_auto_oid(), SWITCH_SIDLIST_STATS_ATTR_STORED_CNTRS, cntr_list);

    if (cntr_list.size() == SWITCH_SEGMENTROUTE_SIDLIST_COUNTER_ID_MAX) {
      stored_pkts = cntr_list[SWITCH_SEGMENTROUTE_SIDLIST_COUNTER_ID_PKTS];
      stored_bytes = cntr_list[SWITCH_SEGMENTROUTE_SIDLIST_COUNTER_ID_BYTES];
    }

    cntrs[SWITCH_SEGMENTROUTE_SIDLIST_COUNTER_ID_PKTS].count += stored_pkts;
    cntrs[SWITCH_SEGMENTROUTE_SIDLIST_COUNTER_ID_BYTES].count += stored_bytes;

    return status;
  }

  switch_status_t counters_set(const switch_object_id_t handle) {
    (void)handle;
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    for (const auto nexthop : nexthop_handles) {
      sid_rewrite rewrite_object(nexthop, status);
      rewrite_object.counters_set(nexthop);
    }

    std::vector<uint64_t> null_vector;
    attr_w stored_cntrs_attr(SWITCH_SIDLIST_STATS_ATTR_STORED_CNTRS);
    stored_cntrs_attr.v_set(null_vector);
    status = switch_store::attribute_set(get_auto_oid(), stored_cntrs_attr);

    return status;
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    (void)handle;
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    for (const auto nexthop : nexthop_handles) {
      sid_rewrite rewrite_object(nexthop, status);
      rewrite_object.counters_set(nexthop, cntr_ids);
    }

    uint64_t pkts, bytes;
    std::vector<uint64_t> cntr_list;

    status = switch_store::v_get(
        get_auto_oid(), SWITCH_SIDLIST_STATS_ATTR_STORED_CNTRS, cntr_list);

    if (cntr_list.size() == SWITCH_SEGMENTROUTE_SIDLIST_COUNTER_ID_MAX) {
      pkts = cntr_list[SWITCH_SEGMENTROUTE_SIDLIST_COUNTER_ID_PKTS];
      bytes = cntr_list[SWITCH_SEGMENTROUTE_SIDLIST_COUNTER_ID_BYTES];

      for (auto cntr_id : cntr_ids) {
        switch (cntr_id) {
          case SWITCH_SEGMENTROUTE_SIDLIST_COUNTER_ID_PKTS:
            pkts = 0;
            break;
          case SWITCH_SEGMENTROUTE_SIDLIST_COUNTER_ID_BYTES:
            bytes = 0;
            break;
        }
      }

      cntr_list.clear();

      cntr_list.push_back(static_cast<uint64_t>(pkts));
      cntr_list.push_back(static_cast<uint64_t>(bytes));
    }

    attr_w stored_cntrs_attr(SWITCH_SIDLIST_STATS_ATTR_STORED_CNTRS);
    stored_cntrs_attr.v_set(cntr_list);
    status = switch_store::attribute_set(get_auto_oid(), stored_cntrs_attr);

    return status;
  }
};

switch_status_t rewrite_init() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  REGISTER_OBJECT(tunnel_nexthop, SWITCH_OBJECT_TYPE_TUNNEL_NEXTHOP);
  REGISTER_OBJECT(neighbor_rewrite, SWITCH_OBJECT_TYPE_NEIGHBOR_REWRITE);
  REGISTER_OBJECT(outer_nexthop, SWITCH_OBJECT_TYPE_OUTER_NEXTHOP);
  REGISTER_OBJECT(sid_rewrite, SWITCH_OBJECT_TYPE_SID_REWRITE);
  REGISTER_OBJECT(sidlist_stats, SWITCH_OBJECT_TYPE_SIDLIST_STATS);
  return status;
}

switch_status_t rewrite_clean() { return SWITCH_STATUS_SUCCESS; }

} /* namespace smi */
