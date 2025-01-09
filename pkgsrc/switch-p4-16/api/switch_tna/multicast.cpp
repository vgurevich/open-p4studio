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


#include <memory>
#include <vector>
#include <set>
#include <map>
#include <utility>

#include "common/multicast.h"
#include "switch_tna/utils.h"
#include "switch_tna/p4_16_types.h"
#include "s3/bf_rt_backend.h"

namespace smi {
using namespace smi::bf_rt;  // NOLINT(build/namespaces)
using namespace ::bfrt;      // NOLINT(build/namespaces)
using ::smi::logging::switch_log;

p4_pd_dev_target_t p4_pd_device;

class ipv6_multicast_route_s_g : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_IPV6_MULTICAST_ROUTE_S_G;
  static const switch_attr_id_t status_attr_id =
      SWITCH_IPV6_MULTICAST_ROUTE_S_G_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_IPV6_MULTICAST_ROUTE_S_G_ATTR_PARENT_HANDLE;

 public:
  ipv6_multicast_route_s_g(const switch_object_id_t parent,
                           switch_status_t &status)
      : p4_object_match_action(smi_id::T_IPV6_MULTICAST_ROUTE_S_G,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_object_id_t vrf_handle = {}, rpf_handle = {}, mcast_group = {},
                       rpf_member = {}, rpf_rif = {}, bd_handle = {};
    std::vector<switch_object_id_t> rpf_members = {};
    switch_enum_t rif_type = {};
    uint16_t mgid;

    status |= switch_store::v_get(
        parent, SWITCH_IPMC_ROUTE_ATTR_VRF_HANDLE, vrf_handle);

    status |= switch_store::v_get(
        parent, SWITCH_IPMC_ROUTE_ATTR_RPF_GROUP_HANDLE, rpf_handle);

    status |= switch_store::v_get(
        parent, SWITCH_IPMC_ROUTE_ATTR_GROUP_HANDLE, mcast_group);

    status |= match_key.set_exact(smi_id::F_IPV6_MULTICAST_ROUTE_S_G_VRF,
                                  compute_vrf(vrf_handle));
    status |= match_key.set_exact(smi_id::F_IPV6_MULTICAST_ROUTE_S_G_SRC_ADDR,
                                  parent,
                                  SWITCH_IPMC_ROUTE_ATTR_SRC_IP);
    status |= match_key.set_exact(smi_id::F_IPV6_MULTICAST_ROUTE_S_G_GRP_ADDR,
                                  parent,
                                  SWITCH_IPMC_ROUTE_ATTR_GRP_IP);

    mgid = compute_pre_mgid(mcast_group);

    status |= switch_store::v_get(
        parent, SWITCH_IPMC_ROUTE_ATTR_RPF_GROUP_HANDLE, rpf_handle);
    status |= switch_store::v_get(
        rpf_handle, SWITCH_RPF_GROUP_ATTR_RPF_MEMBERS, rpf_members);

    // there may be only one rpf_member object in (default) SM mode
    // it is guarded by before_rpf_member_create()
    if (status == SWITCH_STATUS_SUCCESS && rpf_members.size() == 1) {
      rpf_member = rpf_members[0];
      status |= switch_store::v_get(
          rpf_member, SWITCH_RPF_MEMBER_ATTR_HANDLE, rpf_rif);
      status |= switch_store::v_get(rpf_rif, SWITCH_RIF_ATTR_TYPE, rif_type);

      if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_PORT ||
          rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_SUB_PORT) {
        status |= find_auto_oid(rpf_rif, SWITCH_OBJECT_TYPE_BD, bd_handle);
      } else if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_VLAN) {
        switch_object_id_t vlan_handle = {};
        status |= switch_store::v_get(
            rpf_rif, SWITCH_RIF_ATTR_VLAN_HANDLE, vlan_handle);
        status |= find_auto_oid(vlan_handle, SWITCH_OBJECT_TYPE_BD, bd_handle);
      }
    }

    action_entry.init_action_data(smi_id::A_IPV6_MULTICAST_ROUTE_S_G_HIT);
    status |=
        action_entry.set_arg(smi_id::P_IPV6_MULTICAST_ROUTE_S_G_HIT_MGID, mgid);
    status |=
        action_entry.set_arg(smi_id::P_IPV6_MULTICAST_ROUTE_S_G_HIT_RPF_GROUP,
                             compute_bd(bd_handle));
  }
  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    (void)handle;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t pkts = 0;
    status = p4_object_match_action::data_get();
    action_entry.get_arg(smi_id::P_IPV6_MULTICAST_ROUTE_S_G_HIT_PKTS,
                         smi_id::A_IPV6_MULTICAST_ROUTE_S_G_HIT,
                         &pkts);
    switch_counter_t cntr;
    cntr.counter_id = SWITCH_IPMC_ROUTE_COUNTER_ID_PKTS;
    cntr.count = pkts;
    cntrs[SWITCH_IPMC_ROUTE_COUNTER_ID_PKTS] = cntr;
    return status;
  }
  switch_status_t counters_set(const switch_object_id_t handle) {
    (void)handle;
    uint64_t value = 0;
    action_entry.set_arg(smi_id::P_IPV6_MULTICAST_ROUTE_S_G_HIT_PKTS, value);
    return p4_object_match_action::data_set();
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    (void)handle;
    for (auto cntr_id : cntr_ids) {
      switch (cntr_id) {
        case SWITCH_IPMC_ROUTE_COUNTER_ID_PKTS:
          return counters_set(handle);
        default:
          break;
      }
    }
    return SWITCH_STATUS_SUCCESS;
  }

  switch_status_t counters_save(const switch_object_id_t parent) {
    (void)parent;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t ctr_pkt = 0;

    if (get_auto_oid() == 0) return status;

    // MAU counter
    status = p4_object_match_action::data_get();
    if (status != SWITCH_STATUS_SUCCESS) return status;
    action_entry.get_arg(smi_id::P_IPV6_MULTICAST_ROUTE_S_G_HIT_PKTS,
                         smi_id::A_IPV6_MULTICAST_ROUTE_S_G_HIT,
                         &ctr_pkt);

    status |= switch_store::v_set(
        get_auto_oid(),
        SWITCH_IPV6_MULTICAST_ROUTE_S_G_ATTR_MAU_STATS_CACHE,
        static_cast<uint64_t>(ctr_pkt));

    return status;
  }

  switch_status_t counters_restore(const switch_object_id_t parent) {
    (void)parent;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t ctr_pkt = 0;

    if (get_auto_oid() == 0) return status;

    // MAU counter
    status |= switch_store::v_get(
        get_auto_oid(),
        SWITCH_IPV6_MULTICAST_ROUTE_S_G_ATTR_MAU_STATS_CACHE,
        ctr_pkt);
    if (status != SWITCH_STATUS_SUCCESS) return status;

    action_entry.set_arg(smi_id::P_IPV6_MULTICAST_ROUTE_S_G_HIT_PKTS, ctr_pkt);
    status = p4_object_match_action::data_set();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_NONE,
                 "{}.{}: Fail to restore mau stats cache,  "
                 "ipv6_multicast_route_s_g status {} ",
                 __func__,
                 __LINE__,
                 status);
    }

    return status;
  }
};

class ipv6_multicast_route_x_g : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_IPV6_MULTICAST_ROUTE_X_G;
  static const switch_attr_id_t status_attr_id =
      SWITCH_IPV6_MULTICAST_ROUTE_X_G_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_IPV6_MULTICAST_ROUTE_X_G_ATTR_PARENT_HANDLE;

 public:
  ipv6_multicast_route_x_g(const switch_object_id_t parent,
                           switch_status_t &status)
      : p4_object_match_action(smi_id::T_IPV6_MULTICAST_ROUTE_X_G,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_object_id_t vrf_handle = {}, rpf_handle = {}, mcast_group = {},
                       rpf_member = {}, rpf_rif = {}, bd_handle = {};
    std::vector<switch_object_id_t> rpf_members = {};
    switch_enum_t e = {0};
    switch_enum_t rif_type = {};
    uint16_t mgid;

    status |= switch_store::v_get(parent, SWITCH_IPMC_ROUTE_ATTR_PIM_MODE, e);

    status |= switch_store::v_get(
        parent, SWITCH_IPMC_ROUTE_ATTR_VRF_HANDLE, vrf_handle);

    status |= switch_store::v_get(
        parent, SWITCH_IPMC_ROUTE_ATTR_GROUP_HANDLE, mcast_group);

    mgid = compute_pre_mgid(mcast_group);

    status |= match_key.set_exact(smi_id::F_IPV6_MULTICAST_ROUTE_X_G_VRF,
                                  compute_vrf(vrf_handle));
    status |= match_key.set_exact(smi_id::F_IPV6_MULTICAST_ROUTE_X_G_GRP_ADDR,
                                  parent,
                                  SWITCH_IPMC_ROUTE_ATTR_GRP_IP);
    if (e.enumdata == SWITCH_IPMC_ROUTE_ATTR_PIM_MODE_SM) {
      status |= switch_store::v_get(
          parent, SWITCH_IPMC_ROUTE_ATTR_RPF_GROUP_HANDLE, rpf_handle);
      status |= switch_store::v_get(
          rpf_handle, SWITCH_RPF_GROUP_ATTR_RPF_MEMBERS, rpf_members);

      // there may be only one rpf_member object in SM mode
      // it is guarded by before_rpf_member_create()
      if (status == SWITCH_STATUS_SUCCESS && rpf_members.size() == 1) {
        rpf_member = rpf_members[0];
        status |= switch_store::v_get(
            rpf_member, SWITCH_RPF_MEMBER_ATTR_HANDLE, rpf_rif);

        status |= switch_store::v_get(rpf_rif, SWITCH_RIF_ATTR_TYPE, rif_type);

        if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_PORT ||
            rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_SUB_PORT) {
          status |= find_auto_oid(rpf_rif, SWITCH_OBJECT_TYPE_BD, bd_handle);
        } else if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_VLAN) {
          switch_object_id_t vlan_handle = {};
          status |= switch_store::v_get(
              rpf_rif, SWITCH_RIF_ATTR_VLAN_HANDLE, vlan_handle);
          status |=
              find_auto_oid(vlan_handle, SWITCH_OBJECT_TYPE_BD, bd_handle);
        }
      }

      action_entry.init_action_data(smi_id::A_IPV6_MULTICAST_ROUTE_X_G_HIT_SM);
      status |= action_entry.set_arg(
          smi_id::P_IPV6_MULTICAST_ROUTE_X_G_HIT_SM_MGID, mgid);
      status |= action_entry.set_arg(
          smi_id::P_IPV6_MULTICAST_ROUTE_X_G_HIT_SM_RPF_GROUP,
          compute_bd(bd_handle));
    } else if (e.enumdata == SWITCH_IPMC_ROUTE_ATTR_PIM_MODE_BIDIR) {
      action_entry.init_action_data(
          smi_id::A_IPV6_MULTICAST_ROUTE_X_G_HIT_BIDIR);
      status |= action_entry.set_arg(
          smi_id::P_IPV6_MULTICAST_ROUTE_X_G_HIT_BIDIR_MGID, mgid);
      // TODO(bfn) BIDIR OPTIMIZATION
      status |= action_entry.set_arg(
          smi_id::P_IPV6_MULTICAST_ROUTE_X_G_HIT_BIDIR_RPF_GROUP,
          static_cast<uint16_t>(0xFFFF));
    }
  }
  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    (void)handle;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t pkts = 0;
    status = p4_object_match_action::data_get();
    action_entry.get_arg(smi_id::P_IPV6_MULTICAST_ROUTE_X_G_HIT_SM_PKTS,
                         smi_id::A_IPV6_MULTICAST_ROUTE_X_G_HIT_BIDIR,
                         &pkts);
    switch_counter_t cntr;
    cntr.counter_id = SWITCH_IPMC_ROUTE_COUNTER_ID_PKTS;
    cntr.count = pkts;
    cntrs[SWITCH_IPMC_ROUTE_COUNTER_ID_PKTS] = cntr;
    return status;
  }
  switch_status_t counters_set(const switch_object_id_t handle) {
    (void)handle;
    uint64_t value = 0;
    action_entry.set_arg(smi_id::P_IPV6_MULTICAST_ROUTE_X_G_HIT_SM_PKTS, value);
    return p4_object_match_action::data_set();
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    for (auto cntr_id : cntr_ids) {
      switch (cntr_id) {
        case SWITCH_IPMC_ROUTE_COUNTER_ID_PKTS:
          return counters_set(handle);
        default:
          break;
      }
    }
    return SWITCH_STATUS_SUCCESS;
  }

  switch_status_t counters_save(const switch_object_id_t parent) {
    (void)parent;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t ctr_pkt = 0;

    if (get_auto_oid() == 0) return status;

    // MAU counter
    status = p4_object_match_action::data_get();
    if (status != SWITCH_STATUS_SUCCESS) return status;
    action_entry.get_arg(smi_id::P_IPV6_MULTICAST_ROUTE_X_G_HIT_SM_PKTS,
                         smi_id::A_IPV6_MULTICAST_ROUTE_X_G_HIT_BIDIR,
                         &ctr_pkt);

    status |= switch_store::v_set(
        get_auto_oid(),
        SWITCH_IPV6_MULTICAST_ROUTE_X_G_ATTR_MAU_STATS_CACHE,
        static_cast<uint64_t>(ctr_pkt));

    return status;
  }

  switch_status_t counters_restore(const switch_object_id_t parent) {
    (void)parent;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t ctr_pkt = 0;

    if (get_auto_oid() == 0) return status;

    // MAU counter
    status |= switch_store::v_get(
        get_auto_oid(),
        SWITCH_IPV6_MULTICAST_ROUTE_X_G_ATTR_MAU_STATS_CACHE,
        ctr_pkt);
    if (status != SWITCH_STATUS_SUCCESS) return status;

    action_entry.set_arg(smi_id::P_IPV6_MULTICAST_ROUTE_X_G_HIT_SM_PKTS,
                         ctr_pkt);
    status = p4_object_match_action::data_set();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_NONE,
                 "{}.{}: Fail to restore mau stats cache,  "
                 "ipv6_multicast_route_x_g status {} ",
                 __func__,
                 __LINE__,
                 status);
    }

    return status;
  }
};

class ipv4_multicast_route_s_g : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_IPV4_MULTICAST_ROUTE_S_G;
  static const switch_attr_id_t status_attr_id =
      SWITCH_IPV4_MULTICAST_ROUTE_S_G_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_IPV4_MULTICAST_ROUTE_S_G_ATTR_PARENT_HANDLE;

 public:
  ipv4_multicast_route_s_g(const switch_object_id_t parent,
                           switch_status_t &status)
      : p4_object_match_action(smi_id::T_IPV4_MULTICAST_ROUTE_S_G,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_object_id_t vrf_handle = {}, rpf_handle = {}, mcast_group = {},
                       rpf_member = {}, rpf_rif = {}, bd_handle = {};
    std::vector<switch_object_id_t> rpf_members = {};
    switch_enum_t rif_type = {};
    uint16_t mgid;

    status |= switch_store::v_get(
        parent, SWITCH_IPMC_ROUTE_ATTR_VRF_HANDLE, vrf_handle);

    status |= switch_store::v_get(
        parent, SWITCH_IPMC_ROUTE_ATTR_RPF_GROUP_HANDLE, rpf_handle);

    status |= switch_store::v_get(
        parent, SWITCH_IPMC_ROUTE_ATTR_GROUP_HANDLE, mcast_group);

    status |= match_key.set_exact(smi_id::F_IPV4_MULTICAST_ROUTE_S_G_VRF,
                                  compute_vrf(vrf_handle));
    status |= match_key.set_exact(smi_id::F_IPV4_MULTICAST_ROUTE_S_G_SRC_ADDR,
                                  parent,
                                  SWITCH_IPMC_ROUTE_ATTR_SRC_IP);
    status |= match_key.set_exact(smi_id::F_IPV4_MULTICAST_ROUTE_S_G_GRP_ADDR,
                                  parent,
                                  SWITCH_IPMC_ROUTE_ATTR_GRP_IP);

    mgid = compute_pre_mgid(mcast_group);

    status |= switch_store::v_get(
        parent, SWITCH_IPMC_ROUTE_ATTR_RPF_GROUP_HANDLE, rpf_handle);
    status |= switch_store::v_get(
        rpf_handle, SWITCH_RPF_GROUP_ATTR_RPF_MEMBERS, rpf_members);

    // there may be only one rpf_member object in (default) SM mode
    // it is guarded by before_rpf_member_create()
    if (status == SWITCH_STATUS_SUCCESS && rpf_members.size() == 1) {
      rpf_member = rpf_members[0];
      status |= switch_store::v_get(
          rpf_member, SWITCH_RPF_MEMBER_ATTR_HANDLE, rpf_rif);

      status |= switch_store::v_get(rpf_rif, SWITCH_RIF_ATTR_TYPE, rif_type);

      if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_PORT ||
          rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_SUB_PORT) {
        status |= find_auto_oid(rpf_rif, SWITCH_OBJECT_TYPE_BD, bd_handle);
      } else if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_VLAN) {
        switch_object_id_t vlan_handle = {};
        status |= switch_store::v_get(
            rpf_rif, SWITCH_RIF_ATTR_VLAN_HANDLE, vlan_handle);
        status |= find_auto_oid(vlan_handle, SWITCH_OBJECT_TYPE_BD, bd_handle);
      }
    }

    action_entry.init_action_data(smi_id::A_IPV4_MULTICAST_ROUTE_S_G_HIT);
    status |=
        action_entry.set_arg(smi_id::P_IPV4_MULTICAST_ROUTE_S_G_HIT_MGID, mgid);
    status |=
        action_entry.set_arg(smi_id::P_IPV4_MULTICAST_ROUTE_S_G_HIT_RPF_GROUP,
                             compute_bd(bd_handle));
  }
  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    (void)handle;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t pkts = 0;
    status = p4_object_match_action::data_get();
    action_entry.get_arg(smi_id::P_IPV4_MULTICAST_ROUTE_S_G_HIT_PKTS,
                         smi_id::A_IPV4_MULTICAST_ROUTE_S_G_HIT,
                         &pkts);
    switch_counter_t cntr;
    cntr.counter_id = SWITCH_IPMC_ROUTE_COUNTER_ID_PKTS;
    cntr.count = pkts;
    cntrs[SWITCH_IPMC_ROUTE_COUNTER_ID_PKTS] = cntr;
    return status;
  }
  switch_status_t counters_set(const switch_object_id_t handle) {
    (void)handle;
    uint64_t value = 0;
    action_entry.set_arg(smi_id::P_IPV4_MULTICAST_ROUTE_S_G_HIT_PKTS, value);
    return p4_object_match_action::data_set();
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    (void)handle;
    for (auto cntr_id : cntr_ids) {
      switch (cntr_id) {
        case SWITCH_IPMC_ROUTE_COUNTER_ID_PKTS:
          return counters_set(handle);
        default:
          break;
      }
    }
    return SWITCH_STATUS_SUCCESS;
  }

  switch_status_t counters_save(const switch_object_id_t parent) {
    (void)parent;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t ctr_pkt = 0;

    if (get_auto_oid() == 0) return status;

    // MAU counter
    status = p4_object_match_action::data_get();
    if (status != SWITCH_STATUS_SUCCESS) return status;
    action_entry.get_arg(smi_id::P_IPV4_MULTICAST_ROUTE_S_G_HIT_PKTS,
                         smi_id::A_IPV4_MULTICAST_ROUTE_S_G_HIT,
                         &ctr_pkt);

    status |= switch_store::v_set(
        get_auto_oid(),
        SWITCH_IPV4_MULTICAST_ROUTE_S_G_ATTR_MAU_STATS_CACHE,
        static_cast<uint64_t>(ctr_pkt));

    return status;
  }

  switch_status_t counters_restore(const switch_object_id_t parent) {
    (void)parent;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t ctr_pkt = 0;

    if (get_auto_oid() == 0) return status;

    // MAU counter
    status |= switch_store::v_get(
        get_auto_oid(),
        SWITCH_IPV4_MULTICAST_ROUTE_S_G_ATTR_MAU_STATS_CACHE,
        ctr_pkt);
    if (status != SWITCH_STATUS_SUCCESS) return status;

    action_entry.set_arg(smi_id::P_IPV4_MULTICAST_ROUTE_S_G_HIT_PKTS, ctr_pkt);
    status = p4_object_match_action::data_set();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_NONE,
                 "{}.{}: Fail to restore mau stats cache,  "
                 "ipv4_multicast_route_s_g status {} ",
                 __func__,
                 __LINE__,
                 status);
    }

    return status;
  }
};

class ipv4_multicast_route_x_g : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_IPV4_MULTICAST_ROUTE_X_G;
  static const switch_attr_id_t status_attr_id =
      SWITCH_IPV4_MULTICAST_ROUTE_X_G_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_IPV4_MULTICAST_ROUTE_X_G_ATTR_PARENT_HANDLE;

 public:
  ipv4_multicast_route_x_g(const switch_object_id_t parent,
                           switch_status_t &status)
      : p4_object_match_action(smi_id::T_IPV4_MULTICAST_ROUTE_X_G,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_object_id_t vrf_handle = {}, rpf_handle = {}, mcast_group = {},
                       rpf_member = {}, rpf_rif = {}, bd_handle = {};
    std::vector<switch_object_id_t> rpf_members = {};
    switch_enum_t e = {0};
    switch_enum_t rif_type = {};
    uint16_t mgid;

    status |= switch_store::v_get(parent, SWITCH_IPMC_ROUTE_ATTR_PIM_MODE, e);

    status |= switch_store::v_get(
        parent, SWITCH_IPMC_ROUTE_ATTR_VRF_HANDLE, vrf_handle);

    status |= switch_store::v_get(
        parent, SWITCH_IPMC_ROUTE_ATTR_GROUP_HANDLE, mcast_group);

    mgid = compute_pre_mgid(mcast_group);

    status |= match_key.set_exact(smi_id::F_IPV4_MULTICAST_ROUTE_X_G_VRF,
                                  compute_vrf(vrf_handle));
    status |= match_key.set_exact(smi_id::F_IPV4_MULTICAST_ROUTE_X_G_GRP_ADDR,
                                  parent,
                                  SWITCH_IPMC_ROUTE_ATTR_GRP_IP);
    if (e.enumdata == SWITCH_IPMC_ROUTE_ATTR_PIM_MODE_SM) {
      status |= switch_store::v_get(
          parent, SWITCH_IPMC_ROUTE_ATTR_RPF_GROUP_HANDLE, rpf_handle);
      status |= switch_store::v_get(
          rpf_handle, SWITCH_RPF_GROUP_ATTR_RPF_MEMBERS, rpf_members);

      // there may be only one rpf_member object in SM mode
      // it is guarded by before_rpf_member_create()
      if (status == SWITCH_STATUS_SUCCESS && rpf_members.size() == 1) {
        rpf_member = rpf_members[0];
        status |= switch_store::v_get(
            rpf_member, SWITCH_RPF_MEMBER_ATTR_HANDLE, rpf_rif);

        status |= switch_store::v_get(rpf_rif, SWITCH_RIF_ATTR_TYPE, rif_type);

        if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_PORT ||
            rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_SUB_PORT) {
          status |= find_auto_oid(rpf_rif, SWITCH_OBJECT_TYPE_BD, bd_handle);
        } else if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_VLAN) {
          switch_object_id_t vlan_handle = {};
          status |= switch_store::v_get(
              rpf_rif, SWITCH_RIF_ATTR_VLAN_HANDLE, vlan_handle);
          status |=
              find_auto_oid(vlan_handle, SWITCH_OBJECT_TYPE_BD, bd_handle);
        }
      }

      action_entry.init_action_data(smi_id::A_IPV4_MULTICAST_ROUTE_X_G_HIT_SM);
      status |= action_entry.set_arg(
          smi_id::P_IPV4_MULTICAST_ROUTE_X_G_HIT_SM_MGID, mgid);
      status |= action_entry.set_arg(
          smi_id::P_IPV4_MULTICAST_ROUTE_X_G_HIT_SM_RPF_GROUP,
          compute_bd(bd_handle));
    } else if (e.enumdata == SWITCH_IPMC_ROUTE_ATTR_PIM_MODE_BIDIR) {
      action_entry.init_action_data(
          smi_id::A_IPV4_MULTICAST_ROUTE_X_G_HIT_BIDIR);
      status |= action_entry.set_arg(
          smi_id::P_IPV4_MULTICAST_ROUTE_X_G_HIT_BIDIR_MGID, mgid);
      // TODO(bfn) BIDIR OPTIMIZATION
      status |= action_entry.set_arg(
          smi_id::P_IPV4_MULTICAST_ROUTE_X_G_HIT_BIDIR_RPF_GROUP,
          static_cast<uint16_t>(0xFFFF));
    }
  }
  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    (void)handle;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t pkts = 0;
    status = p4_object_match_action::data_get();
    action_entry.get_arg(smi_id::P_IPV4_MULTICAST_ROUTE_X_G_HIT_SM_PKTS,
                         smi_id::A_IPV4_MULTICAST_ROUTE_X_G_HIT_BIDIR,
                         &pkts);
    switch_counter_t cntr;
    cntr.counter_id = SWITCH_IPMC_ROUTE_COUNTER_ID_PKTS;
    cntr.count = pkts;
    cntrs[SWITCH_IPMC_ROUTE_COUNTER_ID_PKTS] = cntr;
    return status;
  }
  switch_status_t counters_set(const switch_object_id_t handle) {
    (void)handle;
    uint64_t value = 0;
    action_entry.set_arg(smi_id::P_IPV4_MULTICAST_ROUTE_X_G_HIT_SM_PKTS, value);
    return p4_object_match_action::data_set();
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    for (auto cntr_id : cntr_ids) {
      switch (cntr_id) {
        case SWITCH_IPMC_ROUTE_COUNTER_ID_PKTS:
          return counters_set(handle);
        default:
          break;
      }
    }
    return SWITCH_STATUS_SUCCESS;
  }

  switch_status_t counters_save(const switch_object_id_t parent) {
    (void)parent;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t ctr_pkt = 0;

    if (get_auto_oid() == 0) return status;

    // MAU counter
    status = p4_object_match_action::data_get();
    if (status != SWITCH_STATUS_SUCCESS) return status;
    action_entry.get_arg(smi_id::P_IPV4_MULTICAST_ROUTE_X_G_HIT_SM_PKTS,
                         smi_id::A_IPV4_MULTICAST_ROUTE_X_G_HIT_BIDIR,
                         &ctr_pkt);

    status |= switch_store::v_set(
        get_auto_oid(),
        SWITCH_IPV4_MULTICAST_ROUTE_X_G_ATTR_MAU_STATS_CACHE,
        static_cast<uint64_t>(ctr_pkt));

    return status;
  }

  switch_status_t counters_restore(const switch_object_id_t parent) {
    (void)parent;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t ctr_pkt = 0;

    if (get_auto_oid() == 0) return status;

    // MAU counter
    status |= switch_store::v_get(
        get_auto_oid(),
        SWITCH_IPV4_MULTICAST_ROUTE_X_G_ATTR_MAU_STATS_CACHE,
        ctr_pkt);
    if (status != SWITCH_STATUS_SUCCESS) return status;

    action_entry.set_arg(smi_id::P_IPV4_MULTICAST_ROUTE_X_G_HIT_SM_PKTS,
                         ctr_pkt);
    status = p4_object_match_action::data_set();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_NONE,
                 "{}.{}: Fail to restore mau stats cache,  "
                 "ipv4_multicast_route_x_g status {} ",
                 __func__,
                 __LINE__,
                 status);
    }

    return status;
  }
};

class ipv6_multicast_bridge_s_g : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_IPV6_MULTICAST_BRIDGE_S_G;
  static const switch_attr_id_t status_attr_id =
      SWITCH_IPV6_MULTICAST_BRIDGE_S_G_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_IPV6_MULTICAST_BRIDGE_S_G_ATTR_PARENT_HANDLE;

 public:
  ipv6_multicast_bridge_s_g(const switch_object_id_t parent,
                            switch_status_t &status)
      : p4_object_match_action(smi_id::T_IPV6_MULTICAST_BRIDGE_S_G,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_object_id_t bd_handle = {}, vlan_handle = {};
    uint16_t mgid;

    status |= switch_store::v_get(
        parent, SWITCH_L2MC_BRIDGE_ATTR_VLAN_HANDLE, vlan_handle);
    status |= get_bd_for_object(vlan_handle, bd_handle);

    mgid = compute_pre_mgid(parent);

    status |= match_key.set_exact(smi_id::F_IPV6_MULTICAST_BRIDGE_S_G_BD,
                                  compute_bd(bd_handle));
    status |= match_key.set_exact(smi_id::F_IPV6_MULTICAST_BRIDGE_S_G_SRC_ADDR,
                                  parent,
                                  SWITCH_L2MC_BRIDGE_ATTR_SRC_IP);
    status |= match_key.set_exact(smi_id::F_IPV6_MULTICAST_BRIDGE_S_G_GRP_ADDR,
                                  parent,
                                  SWITCH_L2MC_BRIDGE_ATTR_GRP_IP);
    action_entry.init_action_data(smi_id::A_IPV6_MULTICAST_BRIDGE_S_G_HIT);
    status |= action_entry.set_arg(smi_id::P_IPV6_MULTICAST_BRIDGE_S_G_HIT_MGID,
                                   mgid);
  }
};

class ipv6_multicast_bridge_x_g : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_IPV6_MULTICAST_BRIDGE_X_G;
  static const switch_attr_id_t status_attr_id =
      SWITCH_IPV6_MULTICAST_BRIDGE_X_G_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_IPV6_MULTICAST_BRIDGE_X_G_ATTR_PARENT_HANDLE;

 public:
  ipv6_multicast_bridge_x_g(const switch_object_id_t parent,
                            switch_status_t &status)
      : p4_object_match_action(smi_id::T_IPV6_MULTICAST_BRIDGE_X_G,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_object_id_t bd_handle = {}, vlan_handle = {};
    uint16_t mgid;

    status |= switch_store::v_get(
        parent, SWITCH_L2MC_BRIDGE_ATTR_VLAN_HANDLE, vlan_handle);
    status |= get_bd_for_object(vlan_handle, bd_handle);

    mgid = compute_pre_mgid(parent);

    status |= match_key.set_exact(smi_id::F_IPV6_MULTICAST_BRIDGE_X_G_BD,
                                  compute_bd(bd_handle));
    status |= match_key.set_exact(smi_id::F_IPV6_MULTICAST_BRIDGE_X_G_GRP_ADDR,
                                  parent,
                                  SWITCH_L2MC_BRIDGE_ATTR_GRP_IP);
    action_entry.init_action_data(smi_id::A_IPV6_MULTICAST_BRIDGE_X_G_HIT);
    status |= action_entry.set_arg(smi_id::P_IPV6_MULTICAST_BRIDGE_X_G_HIT_MGID,
                                   mgid);
  }
};

class ipv4_multicast_bridge_s_g : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_IPV4_MULTICAST_BRIDGE_S_G;
  static const switch_attr_id_t status_attr_id =
      SWITCH_IPV4_MULTICAST_BRIDGE_S_G_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_IPV4_MULTICAST_BRIDGE_S_G_ATTR_PARENT_HANDLE;

 public:
  ipv4_multicast_bridge_s_g(const switch_object_id_t parent,
                            switch_status_t &status)
      : p4_object_match_action(smi_id::T_IPV4_MULTICAST_BRIDGE_S_G,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_object_id_t bd_handle = {}, vlan_handle = {};
    uint16_t mgid;

    status |= switch_store::v_get(
        parent, SWITCH_L2MC_BRIDGE_ATTR_VLAN_HANDLE, vlan_handle);
    status |= get_bd_for_object(vlan_handle, bd_handle);

    mgid = compute_pre_mgid(parent);

    status |= match_key.set_exact(smi_id::F_IPV4_MULTICAST_BRIDGE_S_G_BD,
                                  compute_bd(bd_handle));
    status |= match_key.set_exact(smi_id::F_IPV4_MULTICAST_BRIDGE_S_G_SRC_ADDR,
                                  parent,
                                  SWITCH_L2MC_BRIDGE_ATTR_SRC_IP);
    status |= match_key.set_exact(smi_id::F_IPV4_MULTICAST_BRIDGE_S_G_GRP_ADDR,
                                  parent,
                                  SWITCH_L2MC_BRIDGE_ATTR_GRP_IP);
    action_entry.init_action_data(smi_id::A_IPV4_MULTICAST_BRIDGE_S_G_HIT);
    status |= action_entry.set_arg(smi_id::P_IPV4_MULTICAST_BRIDGE_S_G_HIT_MGID,
                                   mgid);
  }
};

class ipv4_multicast_bridge_x_g : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_IPV4_MULTICAST_BRIDGE_X_G;
  static const switch_attr_id_t status_attr_id =
      SWITCH_IPV4_MULTICAST_BRIDGE_X_G_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_IPV4_MULTICAST_BRIDGE_X_G_ATTR_PARENT_HANDLE;

 public:
  ipv4_multicast_bridge_x_g(const switch_object_id_t parent,
                            switch_status_t &status)
      : p4_object_match_action(smi_id::T_IPV4_MULTICAST_BRIDGE_X_G,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_object_id_t bd_handle = {}, vlan_handle = {};
    uint16_t mgid;

    status |= switch_store::v_get(
        parent, SWITCH_L2MC_BRIDGE_ATTR_VLAN_HANDLE, vlan_handle);
    status |= get_bd_for_object(vlan_handle, bd_handle);

    mgid = compute_pre_mgid(parent);

    status |= match_key.set_exact(smi_id::F_IPV4_MULTICAST_BRIDGE_X_G_BD,
                                  compute_bd(bd_handle));
    status |= match_key.set_exact(smi_id::F_IPV4_MULTICAST_BRIDGE_X_G_GRP_ADDR,
                                  parent,
                                  SWITCH_L2MC_BRIDGE_ATTR_GRP_IP);
    action_entry.init_action_data(smi_id::A_IPV4_MULTICAST_BRIDGE_X_G_HIT);
    status |= action_entry.set_arg(smi_id::P_IPV4_MULTICAST_BRIDGE_X_G_HIT_MGID,
                                   mgid);
  }
};

class mc_lag_membership : public p4_object_pd_fixed {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_MC_LAG_MEMBERSHIP;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_MC_LAG_MEMBERSHIP_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_MC_LAG_MEMBERSHIP_ATTR_STATUS;

 public:
  mc_lag_membership(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_pd_fixed(smi_id::T_PRE_LAG,
                           status_attr_id,
                           auto_ot,
                           parent_attr_id,
                           parent) {
    std::vector<switch_object_id_t> lag_members;
    std::vector<uint32_t> port_map;

    status |= match_key.set_exact(smi_id::F_PRE_LAG_LAG_ID,
                                  switch_store::handle_to_id(parent));
    action_entry.init_indirect_data();

    status |=
        switch_store::v_get(parent, SWITCH_LAG_ATTR_LAG_MEMBERS, lag_members);
    for (auto lag_member : lag_members) {
      uint16_t dev_port = 0;
      switch_object_id_t port_handle = {};
      bool egress_disable = false;
      status |= switch_store::v_get(
          parent, SWITCH_LAG_MEMBER_ATTR_EGRESS_DISABLE, egress_disable);
      status |= switch_store::v_get(
          lag_member, SWITCH_LAG_MEMBER_ATTR_PORT_HANDLE, port_handle);
      if (!egress_disable) {
        status |= switch_store::v_get(
            port_handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
        port_map.push_back(dev_port);
      }
    }

    status |= action_entry.set_arg(smi_id::D_PRE_LAG_DEV_PORT, port_map, false);
  }
  // bfrt pre assumes a lag entry is never deleted, which is silly and not
  // really modelling the general bfrt table semantics.
  // So, empty the port_map and delete the auto object manually
  switch_status_t del() {
    std::vector<uint32_t> port_map = {};

    if (auto_obj.get_auto_oid() == 0) return SWITCH_STATUS_SUCCESS;

    // update prune map with empty list
    action_entry.set_arg(smi_id::D_PRE_LAG_DEV_PORT, port_map, false);
    p4_object_pd_fixed::create_update();

    return auto_obj.del();
  }
};

class mc_port_prune : public p4_object_pd_fixed {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_MC_PORT_PRUNE;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_MC_PORT_PRUNE_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_MC_PORT_PRUNE_ATTR_STATUS;

 public:
  mc_port_prune(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_pd_fixed(smi_id::T_PRE_PRUNE,
                           status_attr_id,
                           auto_ot,
                           parent_attr_id,
                           parent) {
    uint16_t yid = 0, dev_port = 0;
    std::vector<uint32_t> port_map;
    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_YID, yid);

    status |= match_key.set_exact(smi_id::F_PRE_PRUNE_YID, yid);

    action_entry.init_indirect_data();
    port_map.push_back(dev_port);
    status |=
        action_entry.set_arg(smi_id::D_PRE_PRUNE_DEV_PORT, port_map, false);
  }
};

/**
 * mc_mgid object represents multicast group programmed inside fixed PRE
 * mgid table. It is created for each vlan (for flooding purposes), each
 * l2mc_bridge, and each ipmc_group object created by user (for multicast
 * traffic).
 * In PRE below schema is preserved:
 *                   +--------------+
 *                   |     mgid     |
 *                   +--------------+
 *                    /            \
 *                   /              \
 *           +----------+      +----------+
 *           |  node_1  |  ..  |  node_n  |  Level1 nodes
 *           +----------+      +----------+
 *            /     |   \       /     |  \
 *           /      |    +-----/---+  |   +----+
 *          /       |         /     \ |         \
 *      +-----+  +-----+  +-----+  +-----+    +-----+
 *      | p_0 |  | p_1 |  | p_3 |  | p_4 | .. | p_n |  Level2 nodes
 *      +-----+  +-----+  +-----+  +-----+    +-----+
 * Level2 nodes are actual ports/LAGs
 */
class mc_mgid : public p4_object_pd_fixed {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_MC_MGID;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_MC_MGID_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id = SWITCH_MC_MGID_ATTR_STATUS;

 public:
  mc_mgid(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_pd_fixed(smi_id::T_PRE_MGID,
                           status_attr_id,
                           auto_ot,
                           parent_attr_id,
                           parent) {
    std::vector<uint32_t> node_list;
    switch_object_id_t pre_node = {};
    uint32_t node_index = 0;
    // these are totally unnecessary except to satisfy BFRT API
    std::vector<uint32_t> l1_xid_list;
    std::vector<bool> l1_xid_list_valid;

    status |=
        match_key.set_exact(smi_id::F_PRE_MGID_MGID, compute_pre_mgid(parent));
    action_entry.init_indirect_data();

    switch_object_type_t parent_ot = switch_store::object_type_query(parent);
    if (parent_ot == SWITCH_OBJECT_TYPE_VLAN) {
      status |=
          find_auto_oid(parent, SWITCH_OBJECT_TYPE_MC_NODE_VLAN, pre_node);
      status |= switch_store::v_get(
          pre_node, SWITCH_MC_NODE_VLAN_ATTR_INDEX, node_index);

      node_list.push_back(node_index);
      l1_xid_list.push_back(0);
      l1_xid_list_valid.push_back(false);

      if (feature::is_feature_set(SWITCH_FEATURE_L2_VXLAN)) {
        std::vector<switch_object_id_t> vlan_members;
        const auto &vlan_members_objects = switch_store::get_object_references(
            parent, SWITCH_OBJECT_TYPE_VLAN_MEMBER);
        for (const auto &item : vlan_members_objects) {
          vlan_members.push_back(item.oid);
        }

        for (auto vlan_member : vlan_members) {
          switch_object_id_t member_handle;
          switch_store::v_get(vlan_member,
                              SWITCH_VLAN_MEMBER_ATTR_MEMBER_HANDLE,
                              member_handle);

          if (switch_store::object_type_query(member_handle) ==
              SWITCH_OBJECT_TYPE_TUNNEL) {
            status |= find_auto_oid(
                vlan_member, SWITCH_OBJECT_TYPE_MC_NODE_VLAN_MEMBER, pre_node);
            status |= switch_store::v_get(
                pre_node, SWITCH_MC_NODE_VLAN_MEMBER_ATTR_INDEX, node_index);

            node_list.push_back(node_index);
            l1_xid_list.push_back(1);
            l1_xid_list_valid.push_back(true);
          }
        }
      }
    } else if (parent_ot == SWITCH_OBJECT_TYPE_L2MC_BRIDGE) {
      std::vector<switch_object_id_t> group_members;
      switch_object_id_t l2mc_group = {}, l2mc_member_tunnel = {},
                         device_handle = {};

      // there is one Level1 node for all ports and LAGs
      status |= find_auto_oid(parent, SWITCH_OBJECT_TYPE_L2MC_NODE, pre_node);
      status |= switch_store::v_get(
          pre_node, SWITCH_L2MC_NODE_ATTR_INDEX, node_index);

      node_list.push_back(node_index);
      l1_xid_list.push_back(0);
      l1_xid_list_valid.push_back(false);

      // there may be many Level1 nodes for tunnels
      status |= switch_store::v_get(
          parent, SWITCH_L2MC_BRIDGE_ATTR_GROUP_HANDLE, l2mc_group);
      status |= switch_store::v_get(
          l2mc_group, SWITCH_L2MC_GROUP_ATTR_L2MC_MEMBERS, group_members);

      status |= switch_store::v_get(
          parent, SWITCH_L2MC_BRIDGE_ATTR_DEVICE, device_handle);

      for (auto l2mc_member : group_members) {
        switch_object_id_t output_handle;
        switch_store::v_get(
            l2mc_member, SWITCH_L2MC_MEMBER_ATTR_OUTPUT_HANDLE, output_handle);

        if (switch_store::object_type_query(output_handle) ==
            SWITCH_OBJECT_TYPE_TUNNEL) {
          // find corresponding l2mc_member_tunnel object
          std::set<attr_w> lookup_attrs;
          lookup_attrs.insert(
              attr_w(SWITCH_L2MC_MEMBER_TUNNEL_ATTR_DEVICE, device_handle));
          lookup_attrs.insert(attr_w(
              SWITCH_L2MC_MEMBER_TUNNEL_ATTR_TUNNEL_HANDLE, output_handle));
          lookup_attrs.insert(attr_w(
              SWITCH_L2MC_MEMBER_TUNNEL_ATTR_L2MC_BRIDGE_HANDLE, parent));

          status |= switch_store::object_id_get_wkey(
              SWITCH_OBJECT_TYPE_L2MC_MEMBER_TUNNEL,
              lookup_attrs,
              l2mc_member_tunnel);

          if (status != SWITCH_STATUS_SUCCESS) {
            // l2mc_member_tunnel will not exist during l2mc_bridge creation,
            // the node will be associated with the mgid when the after-create
            // trigger function is called
            switch_log(SWITCH_API_LEVEL_DEBUG,
                       SWITCH_OBJECT_TYPE_MC_MGID,
                       "{}.{}: Failed to get l2mc_tunnel_member for "
                       "l2mc_member of type tunnel: {}",
                       __func__,
                       __LINE__,
                       status);
            // clear the status
            status = SWITCH_STATUS_SUCCESS;
            continue;
          }

          status |= find_auto_oid(
              l2mc_member_tunnel, SWITCH_OBJECT_TYPE_MC_NODE_TUNNEL, pre_node);

          status |= switch_store::v_get(
              pre_node, SWITCH_MC_NODE_TUNNEL_ATTR_INDEX, node_index);

          if (status != SWITCH_STATUS_SUCCESS) {
            switch_log(SWITCH_API_LEVEL_DEBUG,
                       SWITCH_OBJECT_TYPE_MC_MGID,
                       "{}.{}: Failed to get node index for "
                       "l2mc_member_tunnel object: {}",
                       __func__,
                       __LINE__,
                       status);
            // clear the status
            status = SWITCH_STATUS_SUCCESS;
            continue;
          }

          node_list.push_back(node_index);
          l1_xid_list.push_back(1);
          l1_xid_list_valid.push_back(true);
        }
      }
    } else if (parent_ot == SWITCH_OBJECT_TYPE_IPMC_GROUP) {
      std::vector<switch_object_id_t> group_members;
      switch_object_id_t output_handle = {};
      switch_enum_t rif_type;

      switch_store::v_get(
          parent, SWITCH_IPMC_GROUP_ATTR_IPMC_MEMBERS, group_members);

      for (auto ipmc_member : group_members) {
        switch_store::v_get(
            ipmc_member, SWITCH_IPMC_MEMBER_ATTR_OUTPUT_HANDLE, output_handle);

        if (switch_store::object_type_query(output_handle) ==
            SWITCH_OBJECT_TYPE_RIF) {
          status |= find_auto_oid(
              ipmc_member, SWITCH_OBJECT_TYPE_IPMC_NODE, pre_node);

          status |= switch_store::v_get(
              pre_node, SWITCH_IPMC_NODE_ATTR_INDEX, node_index);

          node_list.push_back(node_index);
          l1_xid_list.push_back(0);
          l1_xid_list_valid.push_back(false);

          status |= switch_store::v_get(
              output_handle, SWITCH_RIF_ATTR_TYPE, rif_type);

          if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_VLAN) {
            // take either l2mc_group or vlan members and find all nodes
            // corresponding to the members of type tunnel
            std::vector<switch_object_id_t> ipmc_members_vlan_tunnel;

            // check if there are corresponding tunnel nodes
            status |=
                switch_store::v_get(ipmc_member,
                                    SWITCH_IPMC_MEMBER_ATTR_VLAN_TUNNEL_MEMBERS,
                                    ipmc_members_vlan_tunnel);

            for (auto member : ipmc_members_vlan_tunnel) {
              status |= find_auto_oid(
                  member, SWITCH_OBJECT_TYPE_MC_NODE_TUNNEL, pre_node);

              status |= switch_store::v_get(
                  pre_node, SWITCH_MC_NODE_TUNNEL_ATTR_INDEX, node_index);

              node_list.push_back(node_index);
              l1_xid_list.push_back(1);
              l1_xid_list_valid.push_back(true);
            }
          }
        }
      }
    }

    status |= action_entry.set_arg(
        smi_id::D_PRE_MGID_MULTICAST_NODE_ID, node_list, false);
    status |= action_entry.set_arg(
        smi_id::D_PRE_MGID_MULTICAST_NODE_L1_XID, l1_xid_list, false);
    status |=
        action_entry.set_arg(smi_id::D_PRE_MGID_MULTICAST_NODE_L1_XID_VALID,
                             l1_xid_list_valid,
                             false);
  }
};

/**
 * mc_node_vlan (with parent vlan) is intended for creating Level1 node
 * corresponding to vlan object. In this case mgid has only one Level1 node
 * and separate Level2 node is created for each vlan_member.
 *                 +--------+
 *                 |  mgid  |
 *                 +--------+
 *                     |
 *                 +--------+
 *                 |  node  |
 *                 +--------+
 *                 /   |  .. \
 *          +-----+ +-----+    +-----+
 *          | p_0 | | p_1 | .. | p_n |
 *          +-----+ +-----+    +-----+
 */
class mc_node_vlan : public p4_object_pd_fixed {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_MC_NODE_VLAN;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_MC_NODE_VLAN_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_MC_NODE_VLAN_ATTR_STATUS;

 public:
  mc_node_vlan(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_pd_fixed(smi_id::T_PRE_NODE,
                           status_attr_id,
                           auto_ot,
                           parent_attr_id,
                           parent) {
    std::vector<uint32_t> port_map;
    std::vector<uint32_t> lag_map;

    std::vector<switch_object_id_t> vlan_members;
    const auto &vlan_members_objects = switch_store::get_object_references(
        parent, SWITCH_OBJECT_TYPE_VLAN_MEMBER);
    for (const auto &item : vlan_members_objects) {
      vlan_members.push_back(item.oid);
    }
    for (auto vlan_member : vlan_members) {
      switch_object_id_t port_lag_handle = {};
      status |= switch_store::v_get(
          vlan_member, SWITCH_VLAN_MEMBER_ATTR_MEMBER_HANDLE, port_lag_handle);
      switch (switch_store::object_type_query(port_lag_handle)) {
        case SWITCH_OBJECT_TYPE_PORT: {
          uint16_t dev_port = 0;
          status |= switch_store::v_get(
              port_lag_handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
          port_map.push_back(dev_port);
        } break;
        case SWITCH_OBJECT_TYPE_LAG:
          lag_map.push_back(switch_store::handle_to_id(port_lag_handle));
          break;
        default:
          break;
      }
    }

    action_entry.init_indirect_data();
    status |=
        action_entry.set_arg(smi_id::D_PRE_NODE_DEV_PORT, port_map, false);
    status |= action_entry.set_arg(
        smi_id::D_PRE_NODE_MULTICAST_LAG_ID, lag_map, false);
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_object_id_t auto_oid = auto_obj.get_auto_oid();
    switch_object_id_t parent = auto_obj.get_parent();
    bool allocate = auto_oid == 0 ? true : false;
    uint32_t node_index = 0;
    uint16_t rid = 0;

    // take VLAN's rid value
    rid = compute_rid(parent);

    if (allocate) {
      // object is created for the first time - allocate node index
      node_index = SWITCH_CONTEXT.pre_node_allocate();

      // store node index
      auto insert_ret = auto_obj.attrs.insert(
          attr_w(SWITCH_MC_NODE_VLAN_ATTR_INDEX, node_index));
      if (insert_ret.second == false) status |= SWITCH_STATUS_FAILURE;
    } else {
      // object already exists - get stored node index
      status |= switch_store::v_get(
          auto_oid, SWITCH_MC_NODE_VLAN_ATTR_INDEX, node_index);

      if (switch_store::smiContext::context().in_warm_init()) {
        SWITCH_CONTEXT.pre_node_reserve(node_index);
      }
    }

    status |=
        match_key.set_exact(smi_id::F_PRE_NODE_MULTICAST_NODE_ID, node_index);
    status |= action_entry.set_arg(smi_id::D_PRE_NODE_MULTICAST_RID, rid);

    status |= p4_object_pd_fixed::create_update();

    // release allocated node index if the object was not created successfully
    if (status != SWITCH_STATUS_SUCCESS && allocate) {
      SWITCH_CONTEXT.pre_node_release(node_index);
    }

    return status;
  }

  switch_status_t del() {
    // release allocated node index
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_object_id_t auto_oid = auto_obj.get_auto_oid();
    switch_object_id_t parent = auto_obj.get_parent();
    uint32_t node_index = 0;
    uint16_t rid = 0;

    status |= switch_store::v_get(
        auto_oid, SWITCH_MC_NODE_VLAN_ATTR_INDEX, node_index);

    rid = compute_rid(parent);

    status |=
        match_key.set_exact(smi_id::F_PRE_NODE_MULTICAST_NODE_ID, node_index);
    status |= action_entry.set_arg(smi_id::D_PRE_NODE_MULTICAST_RID, rid);

    status |= SWITCH_CONTEXT.pre_node_release(node_index);

    status |= p4_object_pd_fixed::del();

    return status;
  }
};

/**
 * mc_node_vlan_member (with parent vlan_member) is intended for creating
 * separate Level1 node for each vlan_member of the type tunnel. In this case
 * each Level1 node has only one corresponding Level2 node. It is required
 * for L2 VxLAN flooding functionality.
 * The object depends on tunnel_replication_resolution, nexthop_resolution, and
 * member's mac_entry.
 *                   +----------+
 *                   |   mgid   |           (per whole vlan)
 *                   +----------+
 *                    /        \
 *            +--------+      +--------+
 *            | node_1 |  ..  | node_n |    (per p2p tunnel)
 *            +--------+      +--------+
 *                |               |
 *             +-----+         +-----+
 *             | p_x |         | p_z |
 *             +-----+         +-----+
 */
class mc_node_vlan_member : public p4_object_pd_fixed {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_MC_NODE_VLAN_MEMBER;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_MC_NODE_VLAN_MEMBER_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_MC_NODE_VLAN_MEMBER_ATTR_STATUS;

 public:
  std::vector<std::unique_ptr<mc_node_vlan_member>> node_overrides;
  mc_node_vlan_member(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_pd_fixed(smi_id::T_PRE_NODE,
                           status_attr_id,
                           auto_ot,
                           parent_attr_id,
                           parent) {
    switch_object_id_t member_handle = {}, dst_ip_handle = {},
                       port_lag_handle = {}, flood_nexthop_handle = {},
                       rif_handle = {};
    std::vector<uint32_t> port_map;
    std::vector<uint32_t> lag_map;
    switch_enum_t rif_type = {};
    switch_object_type_t ot = switch_store::object_type_query(parent);

    if (!feature::is_feature_set(SWITCH_FEATURE_L2_VXLAN)) {
      clear_attrs();
      return;
    }

    // Object id here might not be a parent we find the right one and override
    if (ot == SWITCH_OBJECT_TYPE_TUNNEL_REPLICATION_RESOLUTION ||
        ot == SWITCH_OBJECT_TYPE_NEXTHOP_RESOLUTION) {
      std::set<switch_object_id_t> mc_node_vlan_member_set;
      status |= switch_store::referencing_set_get(
          parent,
          SWITCH_OBJECT_TYPE_MC_NODE_VLAN_MEMBER,
          mc_node_vlan_member_set);

      if (status != SWITCH_STATUS_SUCCESS) {
        clear_attrs();
        return;
      }

      switch_object_id_t vlan_member = {};
      for (auto node : mc_node_vlan_member_set) {
        status |= switch_store::v_get(
            node, SWITCH_MC_NODE_VLAN_MEMBER_ATTR_PARENT_HANDLE, vlan_member);

        if (status != SWITCH_STATUS_SUCCESS) {
          switch_log(SWITCH_API_LEVEL_DEBUG,
                     SWITCH_OBJECT_TYPE_MC_NODE_VLAN_MEMBER,
                     "{}.{}: Cannot get vlan_member for PRE node",
                     __func__,
                     __LINE__);
          continue;
        }

        node_overrides.push_back(std::unique_ptr<mc_node_vlan_member>(
            new mc_node_vlan_member(vlan_member, status)));
      }
      clear_attrs();
      return;
    } else if (ot != SWITCH_OBJECT_TYPE_VLAN_MEMBER) {
      clear_attrs();
      return;
    }

    status |= switch_store::v_get(
        parent, SWITCH_VLAN_MEMBER_ATTR_MEMBER_HANDLE, member_handle);

    if (switch_store::object_type_query(member_handle) !=
        SWITCH_OBJECT_TYPE_TUNNEL) {
      clear_attrs();
      return;
    }

    switch_enum_t tunnel_mode = {};
    switch_store::v_get(
        member_handle, SWITCH_TUNNEL_ATTR_PEER_MODE, tunnel_mode);
    if (tunnel_mode.enumdata != SWITCH_TUNNEL_ATTR_PEER_MODE_P2P) {
      status = SWITCH_STATUS_INVALID_PARAMETER;
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_MC_NODE_VLAN_MEMBER,
                 "{}.{}: Incorrect tunnel peer mode",
                 __func__,
                 __LINE__);
      clear_attrs();
      return;
    }

    status |= switch_store::v_get(
        member_handle, SWITCH_TUNNEL_ATTR_DST_IP_HANDLE, dst_ip_handle);

    switch_object_id_t outer_fib_handle = {};
    status |= find_auto_oid(
        dst_ip_handle, SWITCH_OBJECT_TYPE_OUTER_FIB_TABLE, outer_fib_handle);

    switch_object_id_t tun_rep_res_handle = {};
    status |= find_auto_oid(outer_fib_handle,
                            SWITCH_OBJECT_TYPE_TUNNEL_REPLICATION_RESOLUTION,
                            tun_rep_res_handle);

    // Store tunnel_replication_resolution handle for dependency
    auto_obj.attrs.insert(attr_w(
        SWITCH_MC_NODE_VLAN_MEMBER_ATTR_TUNNEL_REPLICATION_RESOLUTION_HANDLE,
        tun_rep_res_handle));

    status |= switch_store::v_get(
        tun_rep_res_handle,
        SWITCH_TUNNEL_REPLICATION_RESOLUTION_ATTR_FLOOD_NEXTHOP_HANDLE,
        flood_nexthop_handle);

    if (status == SWITCH_STATUS_SUCCESS && flood_nexthop_handle.data != 0) {
      status |= switch_store::v_get(
          flood_nexthop_handle, SWITCH_NEXTHOP_ATTR_HANDLE, rif_handle);

      switch_object_id_t nexthop_resolution_handle = {};
      status |= find_auto_oid(flood_nexthop_handle,
                              SWITCH_OBJECT_TYPE_NEXTHOP_RESOLUTION,
                              nexthop_resolution_handle);

      // Store nexthop_resolution handle for dependency
      auto_obj.attrs.insert(
          attr_w(SWITCH_MC_NODE_VLAN_MEMBER_ATTR_NEXTHOP_RESOLUTION_HANDLE,
                 nexthop_resolution_handle));

      switch_object_id_t mac_entry_handle = {};
      status |=
          switch_store::v_get(nexthop_resolution_handle,
                              SWITCH_NEXTHOP_RESOLUTION_ATTR_MAC_ENTRY_HANDLE,
                              mac_entry_handle);

      status |= switch_store::v_get(rif_handle, SWITCH_RIF_ATTR_TYPE, rif_type);
      if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_PORT ||
          rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_SUB_PORT) {
        status |= switch_store::v_get(
            rif_handle, SWITCH_RIF_ATTR_PORT_HANDLE, port_lag_handle);
      } else if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_VLAN) {
        switch_object_id_t neighbor_handle = {};
        status |=
            switch_store::v_get(nexthop_resolution_handle,
                                SWITCH_NEXTHOP_RESOLUTION_ATTR_NEIGHBOR_HANDLE,
                                neighbor_handle);

        if (neighbor_handle.data != 0 && mac_entry_handle.data != 0) {
          status |=
              switch_store::v_get(mac_entry_handle,
                                  SWITCH_MAC_ENTRY_ATTR_DESTINATION_HANDLE,
                                  port_lag_handle);
        }
      } else {
        switch_log(SWITCH_API_LEVEL_DEBUG,
                   SWITCH_OBJECT_TYPE_MC_NODE_VLAN_MEMBER,
                   "{}.{}: Unsupported RIF type",
                   __func__,
                   __LINE__);
      }

      switch (switch_store::object_type_query(port_lag_handle)) {
        case SWITCH_OBJECT_TYPE_PORT: {
          uint16_t dev_port = 0;
          status |= switch_store::v_get(
              port_lag_handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
          port_map.push_back(dev_port);
        } break;
        case SWITCH_OBJECT_TYPE_LAG:
          lag_map.push_back(switch_store::handle_to_id(port_lag_handle));
          break;
        default:
          break;
      }
    } else {
      switch_object_id_t empty_handle = {};
      auto_obj.attrs.insert(
          attr_w(SWITCH_MC_NODE_VLAN_MEMBER_ATTR_NEXTHOP_RESOLUTION_HANDLE,
                 empty_handle));
    }

    action_entry.init_indirect_data();
    status |=
        action_entry.set_arg(smi_id::D_PRE_NODE_DEV_PORT, port_map, false);
    status |= action_entry.set_arg(
        smi_id::D_PRE_NODE_MULTICAST_LAG_ID, lag_map, false);
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_object_id_t auto_oid = auto_obj.get_auto_oid();
    switch_object_id_t parent = auto_obj.get_parent();
    bool allocate = auto_oid == 0 ? true : false;
    switch_object_id_t member_handle = {}, tun_rep_res_handle = {};
    uint32_t node_index = 0;
    uint16_t rid = 0;

    if (!feature::is_feature_set(SWITCH_FEATURE_L2_VXLAN)) {
      return status;
    }
    switch_object_type_t ot = switch_store::object_type_query(parent);

    if (ot == SWITCH_OBJECT_TYPE_TUNNEL_REPLICATION_RESOLUTION ||
        ot == SWITCH_OBJECT_TYPE_NEXTHOP_RESOLUTION) {
      for (auto &&node_override : node_overrides) {
        status |= node_override->create_update();
      }
      return status;
    }

    if (ot != SWITCH_OBJECT_TYPE_VLAN_MEMBER) {
      return status;
    }

    status |= switch_store::v_get(
        parent, SWITCH_VLAN_MEMBER_ATTR_MEMBER_HANDLE, member_handle);

    if (switch_store::object_type_query(member_handle) !=
        SWITCH_OBJECT_TYPE_TUNNEL) {
      return status;
    }

    switch_enum_t tunnel_mode = {};
    switch_store::v_get(
        member_handle, SWITCH_TUNNEL_ATTR_PEER_MODE, tunnel_mode);
    if (tunnel_mode.enumdata != SWITCH_TUNNEL_ATTR_PEER_MODE_P2P) {
      status = SWITCH_STATUS_INVALID_PARAMETER;
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_MC_NODE_VLAN_MEMBER,
                 "{}.{}: Incorrect tunnel peer mode",
                 __func__,
                 __LINE__);
      return status;
    }

    // get rid of corresponding tunnel_replication_resolution object
    const auto attr_it = auto_obj.attrs.find(static_cast<switch_attr_id_t>(
        SWITCH_MC_NODE_VLAN_MEMBER_ATTR_TUNNEL_REPLICATION_RESOLUTION_HANDLE));
    attr_it->v_get(tun_rep_res_handle);
    rid = compute_rid(tun_rep_res_handle);

    if (allocate) {
      // object is created for the first time - allocate node index
      node_index = SWITCH_CONTEXT.pre_node_allocate();

      // store node index
      auto insert_ret = auto_obj.attrs.insert(
          attr_w(SWITCH_MC_NODE_VLAN_MEMBER_ATTR_INDEX, node_index));
      if (insert_ret.second == false) status |= SWITCH_STATUS_FAILURE;
    } else {
      // object already exists - get stored node index
      status |= switch_store::v_get(
          auto_oid, SWITCH_MC_NODE_VLAN_MEMBER_ATTR_INDEX, node_index);

      if (switch_store::smiContext::context().in_warm_init()) {
        SWITCH_CONTEXT.pre_node_reserve(node_index);
      }
    }

    status |=
        match_key.set_exact(smi_id::F_PRE_NODE_MULTICAST_NODE_ID, node_index);
    status |= action_entry.set_arg(smi_id::D_PRE_NODE_MULTICAST_RID, rid);

    status = p4_object_pd_fixed::create_update();

    // release allocated node index if the object was not created successfully
    if (status != SWITCH_STATUS_SUCCESS && allocate) {
      SWITCH_CONTEXT.pre_node_release(node_index);
    }
    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_object_id_t auto_oid = auto_obj.get_auto_oid();
    switch_object_id_t parent = auto_obj.get_parent();
    switch_object_id_t member_handle = {}, tun_rep_res_handle = {};
    uint32_t node_index = 0;
    uint16_t rid = 0;

    if (!feature::is_feature_set(SWITCH_FEATURE_L2_VXLAN)) {
      return status;
    }

    switch_object_type_t ot = switch_store::object_type_query(parent);

    if (ot != SWITCH_OBJECT_TYPE_VLAN_MEMBER) {
      if (!node_overrides.empty()) {
        switch_log(SWITCH_API_LEVEL_DEBUG,
                   SWITCH_OBJECT_TYPE_MC_NODE_VLAN_MEMBER,
                   "{}.{}: parent obj will not be deleted "
                   "due to existing references",
                   __func__,
                   __LINE__);
      } else {
        switch_log(SWITCH_API_LEVEL_DEBUG,
                   SWITCH_OBJECT_TYPE_MC_NODE_VLAN_MEMBER,
                   "{}.{}: auto_obj will not be deleted "
                   "due to improper parent type",
                   __func__,
                   __LINE__);
      }
      return SWITCH_STATUS_SUCCESS;
    }

    status |= switch_store::v_get(
        parent, SWITCH_VLAN_MEMBER_ATTR_MEMBER_HANDLE, member_handle);

    if (switch_store::object_type_query(member_handle) !=
        SWITCH_OBJECT_TYPE_TUNNEL) {
      return status;
    }

    // release allocated node index
    status |= switch_store::v_get(
        auto_oid, SWITCH_MC_NODE_VLAN_MEMBER_ATTR_INDEX, node_index);

    status |= switch_store::v_get(
        auto_oid,
        SWITCH_MC_NODE_VLAN_MEMBER_ATTR_TUNNEL_REPLICATION_RESOLUTION_HANDLE,
        tun_rep_res_handle);

    rid = compute_rid(tun_rep_res_handle);

    status |=
        match_key.set_exact(smi_id::F_PRE_NODE_MULTICAST_NODE_ID, node_index);
    status |= action_entry.set_arg(smi_id::D_PRE_NODE_MULTICAST_RID, rid);

    status |= SWITCH_CONTEXT.pre_node_release(node_index);

    status |= p4_object_pd_fixed::del();

    return status;
  }
};

/**
 * l2mc_node (with parent l2mc_bridge) is intended for creating Level1 nodes for
 * L2MC functionalities. In this case mgid has one Level1 node for all ports
 * and LAGs and separate nodes for each tunnel belonging to the l2mc_group.
 * This class creates Level1 node for all ports and LAGs with separate Level2
 * nodes for each of these ports and LAGs.
 *                                    +--------+
 *                                    |  mgid  |  (per l2mc_bridge)
 *                                    +--------+
 *                                    /   | ..  \
 *                           +-------+    +-+ .. +----------+
 *                          /                \              |
 *                     +--------+         +--------+    +--------+
 *   (per l2mc_bridge) | node_1 |         | node_2 | .. | node_n | (per
 *                     +--------+         +--------+    +--------+ l2mc_member
 *                     /   |     \             \             |     _tunnel)
 *                    /    |  ..  \          +-----+      +-----+
 *     (per      +-----+ +-----+    +-----+  | p_x |  ..  | p_z |
 *   non-tunnel  | p_0 | | p_1 | .. | p_n |  +-----+      +-----+
 *  l2mc_member) +-----+ +-----+    +-----+
 */
class l2mc_node : public p4_object_pd_fixed {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_L2MC_NODE;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_L2MC_NODE_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id = SWITCH_L2MC_NODE_ATTR_STATUS;

 public:
  l2mc_node(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_pd_fixed(smi_id::T_PRE_NODE,
                           status_attr_id,
                           auto_ot,
                           parent_attr_id,
                           parent) {
    switch_object_id_t l2mc_group = {}, l2mc_output = {};
    std::vector<uint32_t> port_map, lag_map;
    std::vector<switch_object_id_t> l2mc_members;

    status |= switch_store::v_get(
        parent, SWITCH_L2MC_BRIDGE_ATTR_GROUP_HANDLE, l2mc_group);

    status |= switch_store::v_get(
        l2mc_group, SWITCH_L2MC_GROUP_ATTR_L2MC_MEMBERS, l2mc_members);

    for (auto l2mc_member : l2mc_members) {
      status |= switch_store::v_get(
          l2mc_member, SWITCH_L2MC_MEMBER_ATTR_OUTPUT_HANDLE, l2mc_output);

      switch (switch_store::object_type_query(l2mc_output)) {
        case SWITCH_OBJECT_TYPE_PORT: {
          uint16_t dev_port = 0;
          status |= switch_store::v_get(
              l2mc_output, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
          port_map.push_back(dev_port);
        } break;
        case SWITCH_OBJECT_TYPE_LAG: {
          lag_map.push_back(switch_store::handle_to_id(l2mc_output));
        } break;
        case SWITCH_OBJECT_TYPE_TUNNEL: {
          // handled by mc_node_tunnel class
        } break;
        default: {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_L2MC_NODE,
                     "{}.{}: Incorrect object type {} of l2mc_member output",
                     __func__,
                     __LINE__,
                     switch_store::object_name_get_from_type(
                         switch_store::object_type_query(l2mc_output)));
        }
      }
    }

    action_entry.init_indirect_data();
    status |=
        action_entry.set_arg(smi_id::D_PRE_NODE_DEV_PORT, port_map, false);
    status |= action_entry.set_arg(
        smi_id::D_PRE_NODE_MULTICAST_LAG_ID, lag_map, false);
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_object_id_t auto_oid = auto_obj.get_auto_oid();
    switch_object_id_t parent = auto_obj.get_parent();
    bool allocate = auto_oid == 0 ? true : false;
    switch_object_id_t l2mc_group = {}, vlan_handle = {};
    uint32_t node_index = 0;
    uint16_t rid = 0;

    status |= switch_store::v_get(
        parent, SWITCH_L2MC_BRIDGE_ATTR_GROUP_HANDLE, l2mc_group);
    status |= switch_store::v_get(
        parent, SWITCH_L2MC_BRIDGE_ATTR_VLAN_HANDLE, vlan_handle);

    // take rid of corresponding VLAN
    rid = compute_rid(vlan_handle);

    if (allocate) {
      // object is created for the first time - allocate node index
      node_index = SWITCH_CONTEXT.pre_node_allocate();

      // store node index
      auto insert_ret = auto_obj.attrs.insert(
          attr_w(SWITCH_L2MC_NODE_ATTR_INDEX, node_index));
      if (insert_ret.second == false) status |= SWITCH_STATUS_FAILURE;
    } else {
      // object already exists - get stored node index
      status |= switch_store::v_get(
          auto_oid, SWITCH_L2MC_NODE_ATTR_INDEX, node_index);

      if (switch_store::smiContext::context().in_warm_init()) {
        SWITCH_CONTEXT.pre_node_reserve(node_index);
      }
    }

    status |=
        match_key.set_exact(smi_id::F_PRE_NODE_MULTICAST_NODE_ID, node_index);
    status |= action_entry.set_arg(smi_id::D_PRE_NODE_MULTICAST_RID, rid);

    status |= p4_object_pd_fixed::create_update();

    // release allocated node index if the object was not created successfully
    if (status != SWITCH_STATUS_SUCCESS && allocate) {
      SWITCH_CONTEXT.pre_node_release(node_index);
    }

    return status;
  }

  switch_status_t del() {
    // release allocated node index
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_object_id_t auto_oid = auto_obj.get_auto_oid();
    switch_object_id_t parent = auto_obj.get_parent();
    switch_object_id_t l2mc_group = {}, vlan_handle = {};
    uint32_t node_index = 0;
    uint16_t rid = 0;

    status |=
        switch_store::v_get(auto_oid, SWITCH_L2MC_NODE_ATTR_INDEX, node_index);

    status |= switch_store::v_get(
        parent, SWITCH_L2MC_BRIDGE_ATTR_GROUP_HANDLE, l2mc_group);
    status |= switch_store::v_get(
        parent, SWITCH_L2MC_BRIDGE_ATTR_VLAN_HANDLE, vlan_handle);

    rid = compute_rid(vlan_handle);

    status |=
        match_key.set_exact(smi_id::F_PRE_NODE_MULTICAST_NODE_ID, node_index);
    status |= action_entry.set_arg(smi_id::D_PRE_NODE_MULTICAST_RID, rid);

    status |= SWITCH_CONTEXT.pre_node_release(node_index);

    status |= p4_object_pd_fixed::del();

    return status;
  }
};

/**
 * Below functions are intended to handle l2mc_member_tunnel creation and
 * deletion.
 *
 * l2mc_member_tunnel is an internal user object created for each l2mc_bridge
 * and each l2mc_member with output handle of type tunnel.
 * When a new l2mc_bridge is being created, an instance of l2mc_member_tunnel
 * is created for each existing l2mc_member (with object handle of type tunnel)
 * belonging to the destination l2mc_group of the created bridge.
 * When a new l2mc_member (with object handle of type tunnel) is being created,
 * an instance of l2mc_member_tunnel is created for each existing l2mc_bridge
 * that uses the l2mc_group to which the l2mc_member belongs.
 * Analogically, the instances of l2mc_member_tunnel object are removed
 * before l2mc_bridge and l2mc_member (with object handle of type tunnel)
 * deletion.
 */
switch_status_t after_l2mc_bridge_create(const switch_object_id_t object_id,
                                         const std::set<attr_w> &attrs) {
  (void)attrs;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t device_handle = {}, l2mc_group = {}, l2mc_output = {},
                     l2mc_member_tunnel = {}, mgid = {};
  std::vector<switch_object_id_t> l2mc_members;
  switch_enum_t tunnel_mode;

  status |= switch_store::v_get(
      object_id, SWITCH_L2MC_BRIDGE_ATTR_DEVICE, device_handle);

  status |= switch_store::v_get(
      object_id, SWITCH_L2MC_BRIDGE_ATTR_GROUP_HANDLE, l2mc_group);

  status |= switch_store::v_get(
      l2mc_group, SWITCH_L2MC_GROUP_ATTR_L2MC_MEMBERS, l2mc_members);

  for (auto l2mc_member : l2mc_members) {
    status |= switch_store::v_get(
        l2mc_member, SWITCH_L2MC_MEMBER_ATTR_OUTPUT_HANDLE, l2mc_output);
    if (switch_store::object_type_query(l2mc_output) ==
        SWITCH_OBJECT_TYPE_TUNNEL) {
      switch_store::v_get(
          l2mc_output, SWITCH_TUNNEL_ATTR_PEER_MODE, tunnel_mode);

      if (tunnel_mode.enumdata != SWITCH_TUNNEL_ATTR_PEER_MODE_P2P) {
        status = SWITCH_STATUS_INVALID_PARAMETER;
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_MC_NODE_TUNNEL,
                   "{}.{}: Incorrect tunnel peer mode of l2mc_member {}",
                   __func__,
                   __LINE__,
                   l2mc_member.data);
        return status;
      }
      // create l2mc_member_tunnel objects that trigger mc_node_tunnel
      // creation
      std::set<attr_w> obj_attrs;
      obj_attrs.insert(
          attr_w(SWITCH_L2MC_MEMBER_TUNNEL_ATTR_DEVICE, device_handle));
      obj_attrs.insert(
          attr_w(SWITCH_L2MC_MEMBER_TUNNEL_ATTR_TUNNEL_HANDLE, l2mc_output));
      obj_attrs.insert(
          attr_w(SWITCH_L2MC_MEMBER_TUNNEL_ATTR_L2MC_BRIDGE_HANDLE, object_id));

      status = switch_store::object_create(
          SWITCH_OBJECT_TYPE_L2MC_MEMBER_TUNNEL, obj_attrs, l2mc_member_tunnel);
    }
  }

  status |= find_auto_oid(object_id, SWITCH_OBJECT_TYPE_MC_MGID, mgid);

  // update pre.mgid
  if (status == SWITCH_STATUS_SUCCESS && mgid.data != 0) {
    mc_mgid upd_mgid(object_id, status);
    status |= upd_mgid.create_update();
  }

  return status;
}

switch_status_t before_l2mc_bridge_delete(switch_object_id_t object_id) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t device_handle = {}, l2mc_group = {}, l2mc_output = {},
                     l2mc_member_tunnel = {}, mgid = {};
  std::vector<switch_object_id_t> l2mc_members;

  status |= switch_store::v_get(
      object_id, SWITCH_L2MC_BRIDGE_ATTR_DEVICE, device_handle);

  status |= switch_store::v_get(
      object_id, SWITCH_L2MC_BRIDGE_ATTR_GROUP_HANDLE, l2mc_group);

  status |= switch_store::v_get(
      l2mc_group, SWITCH_L2MC_GROUP_ATTR_L2MC_MEMBERS, l2mc_members);

  for (auto l2mc_member : l2mc_members) {
    status |= switch_store::v_get(
        l2mc_member, SWITCH_L2MC_MEMBER_ATTR_OUTPUT_HANDLE, l2mc_output);
    if (switch_store::object_type_query(l2mc_output) ==
        SWITCH_OBJECT_TYPE_TUNNEL) {
      // delete l2mc_member_tunnel objects that trigger mc_node_tunnel
      // deletion
      std::set<attr_w> lookup_attrs;
      lookup_attrs.insert(
          attr_w(SWITCH_L2MC_MEMBER_TUNNEL_ATTR_DEVICE, device_handle));
      lookup_attrs.insert(
          attr_w(SWITCH_L2MC_MEMBER_TUNNEL_ATTR_TUNNEL_HANDLE, l2mc_output));
      lookup_attrs.insert(
          attr_w(SWITCH_L2MC_MEMBER_TUNNEL_ATTR_L2MC_BRIDGE_HANDLE, object_id));

      status |= switch_store::object_id_get_wkey(
          SWITCH_OBJECT_TYPE_L2MC_MEMBER_TUNNEL,
          lookup_attrs,
          l2mc_member_tunnel);
      switch_store::object_delete(l2mc_member_tunnel);
    }
  }

  // update pre.mgid
  if (status == SWITCH_STATUS_SUCCESS && mgid.data != 0) {
    mc_mgid upd_mgid(object_id, status);
    status |= upd_mgid.create_update();
  }

  return status;
}

switch_status_t after_l2mc_member_create(const switch_object_id_t object_id,
                                         const std::set<attr_w> &attrs) {
  (void)attrs;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t device_handle = {}, l2mc_output = {}, l2mc_group = {},
                     l2mc_member_tunnel = {}, mgid = {};
  std::set<switch_object_id_t> l2mc_bridges_set;

  // check if the object type of l2mc_member is tunnel
  status |= switch_store::v_get(
      object_id, SWITCH_L2MC_MEMBER_ATTR_OUTPUT_HANDLE, l2mc_output);
  if (switch_store::object_type_query(l2mc_output) !=
      SWITCH_OBJECT_TYPE_TUNNEL) {
    return status;
  }

  // check if there is any corresponding bridge
  status |= switch_store::v_get(
      object_id, SWITCH_L2MC_MEMBER_ATTR_L2MC_GROUP_HANDLE, l2mc_group);
  status |= switch_store::referencing_set_get(
      l2mc_group, SWITCH_OBJECT_TYPE_L2MC_BRIDGE, l2mc_bridges_set);

  if (l2mc_bridges_set.size() == 0) {
    return status;
  }

  status |= switch_store::v_get(
      object_id, SWITCH_L2MC_MEMBER_ATTR_DEVICE, device_handle);

  for (auto l2mc_bridge : l2mc_bridges_set) {
    std::set<attr_w> obj_attrs;
    obj_attrs.insert(
        attr_w(SWITCH_L2MC_MEMBER_TUNNEL_ATTR_DEVICE, device_handle));
    obj_attrs.insert(
        attr_w(SWITCH_L2MC_MEMBER_TUNNEL_ATTR_TUNNEL_HANDLE, l2mc_output));
    obj_attrs.insert(
        attr_w(SWITCH_L2MC_MEMBER_TUNNEL_ATTR_L2MC_BRIDGE_HANDLE, l2mc_bridge));

    status = switch_store::object_create(
        SWITCH_OBJECT_TYPE_L2MC_MEMBER_TUNNEL, obj_attrs, l2mc_member_tunnel);

    status |= find_auto_oid(l2mc_bridge, SWITCH_OBJECT_TYPE_MC_MGID, mgid);

    // update pre.mgid per each l2mc_bridge
    if (status == SWITCH_STATUS_SUCCESS && mgid.data != 0) {
      mc_mgid upd_mgid(l2mc_bridge, status);
      status |= upd_mgid.create_update();
    }
  }

  return status;
}

switch_status_t before_l2mc_member_delete(switch_object_id_t object_id) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t device_handle = {}, l2mc_output = {}, l2mc_group = {},
                     l2mc_member_tunnel = {}, mgid = {};
  std::set<switch_object_id_t> l2mc_bridges_set;

  // check if the object type of l2mc_member is tunnel
  status |= switch_store::v_get(
      object_id, SWITCH_L2MC_MEMBER_ATTR_OUTPUT_HANDLE, l2mc_output);
  if (switch_store::object_type_query(l2mc_output) !=
      SWITCH_OBJECT_TYPE_TUNNEL) {
    return status;
  }

  // check if there is any corresponding bridge
  status |= switch_store::v_get(
      object_id, SWITCH_L2MC_MEMBER_ATTR_L2MC_GROUP_HANDLE, l2mc_group);
  status |= switch_store::referencing_set_get(
      l2mc_group, SWITCH_OBJECT_TYPE_L2MC_BRIDGE, l2mc_bridges_set);

  if (l2mc_bridges_set.size() == 0) {
    return status;
  }

  status |= switch_store::v_get(
      object_id, SWITCH_L2MC_MEMBER_ATTR_DEVICE, device_handle);

  for (auto l2mc_bridge : l2mc_bridges_set) {
    std::set<attr_w> obj_attrs;
    obj_attrs.insert(
        attr_w(SWITCH_L2MC_MEMBER_TUNNEL_ATTR_DEVICE, device_handle));
    obj_attrs.insert(
        attr_w(SWITCH_L2MC_MEMBER_TUNNEL_ATTR_TUNNEL_HANDLE, l2mc_output));
    obj_attrs.insert(
        attr_w(SWITCH_L2MC_MEMBER_TUNNEL_ATTR_L2MC_BRIDGE_HANDLE, l2mc_bridge));

    status |= switch_store::object_id_get_wkey(
        SWITCH_OBJECT_TYPE_L2MC_MEMBER_TUNNEL, obj_attrs, l2mc_member_tunnel);
    switch_store::object_delete(l2mc_member_tunnel);

    status |= find_auto_oid(l2mc_bridge, SWITCH_OBJECT_TYPE_MC_MGID, mgid);
  }

  return status;
}

/**
 * A helper function for retrieving output port or LAG maps from tunnel object.
 * The function pushes device port number or LAG handle to a list that is
 * further used to program pre.node entry.
 */
switch_status_t get_port_lag_map_from_tunnel(
    const switch_object_id_t tunnel_oid,
    std::vector<uint32_t> &port_map,
    std::vector<uint32_t> &lag_map) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t port_lag_handle = {}, dst_ip_handle = {},
                     outer_fib_handle = {}, trr_handle = {},
                     flood_nexthop_handle = {}, rif_handle = {},
                     nexthop_resolution_handle = {};
  switch_enum_t rif_type;

  status |= switch_store::v_get(
      tunnel_oid, SWITCH_TUNNEL_ATTR_DST_IP_HANDLE, dst_ip_handle);

  status |= find_auto_oid(
      dst_ip_handle, SWITCH_OBJECT_TYPE_OUTER_FIB_TABLE, outer_fib_handle);
  status |= find_auto_oid(outer_fib_handle,
                          SWITCH_OBJECT_TYPE_TUNNEL_REPLICATION_RESOLUTION,
                          trr_handle);

  status |= switch_store::v_get(
      trr_handle,
      SWITCH_TUNNEL_REPLICATION_RESOLUTION_ATTR_FLOOD_NEXTHOP_HANDLE,
      flood_nexthop_handle);
  status |= find_auto_oid(flood_nexthop_handle,
                          SWITCH_OBJECT_TYPE_NEXTHOP_RESOLUTION,
                          nexthop_resolution_handle);

  if (status == SWITCH_STATUS_SUCCESS && flood_nexthop_handle.data != 0) {
    status |= switch_store::v_get(
        flood_nexthop_handle, SWITCH_NEXTHOP_ATTR_HANDLE, rif_handle);

    status |= switch_store::v_get(rif_handle, SWITCH_RIF_ATTR_TYPE, rif_type);
    if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_PORT ||
        rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_SUB_PORT) {
      status |= switch_store::v_get(
          rif_handle, SWITCH_RIF_ATTR_PORT_HANDLE, port_lag_handle);
    } else if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_VLAN) {
      switch_object_id_t neighbor_handle = {}, mac_entry_handle = {};
      status |=
          switch_store::v_get(nexthop_resolution_handle,
                              SWITCH_NEXTHOP_RESOLUTION_ATTR_NEIGHBOR_HANDLE,
                              neighbor_handle);
      status |=
          switch_store::v_get(nexthop_resolution_handle,
                              SWITCH_NEXTHOP_RESOLUTION_ATTR_MAC_ENTRY_HANDLE,
                              mac_entry_handle);

      if (neighbor_handle.data != 0 && mac_entry_handle != 0) {
        status |= switch_store::v_get(mac_entry_handle,
                                      SWITCH_MAC_ENTRY_ATTR_DESTINATION_HANDLE,
                                      port_lag_handle);
      } else {
        switch_log(SWITCH_API_LEVEL_DEBUG,
                   SWITCH_OBJECT_TYPE_MC_NODE_VLAN_MEMBER,
                   "{}.{}: Failed to find neighbour or mac_entry for tunnel",
                   __func__,
                   __LINE__);
        return SWITCH_STATUS_ITEM_NOT_FOUND;
      }
    } else {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_MC_NODE_VLAN_MEMBER,
                 "{}.{}: Unsupported RIF type",
                 __func__,
                 __LINE__);
      return SWITCH_STATUS_NOT_SUPPORTED;
    }
    switch (switch_store::object_type_query(port_lag_handle)) {
      case SWITCH_OBJECT_TYPE_PORT: {
        uint16_t dev_port = 0;
        status |= switch_store::v_get(
            port_lag_handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
        port_map.push_back(dev_port);
      } break;
      case SWITCH_OBJECT_TYPE_LAG:
        lag_map.push_back(switch_store::handle_to_id(port_lag_handle));
        break;
      default:
        break;
    }
  }

  return status;
}

/**
 * A helper function that handle creation of PRE nodes for tunnels that are
 * members of VLANs or L2MC groups corresponding to some IPMC group member.
 *
 * If there is an IPMC member with output handle of type RIF and this RIF is
 * an SVI there are two ways of forwarding packets to this SVI. If there is
 * no l2mc_group_handle attribute specified for the ipmc_member instance, a
 * packet will be forwarded to all VLAN members. If the attribute is
 * specified, the packet will go only to the L2MC group members.
 *
 * The function takes care of creating and removing instances of
 * ipmc_member_vlan_tunnel class (objects of type user) that triggers creation
 * of mc_node_tunnel objects. Each of these objects programs single entry of
 * pre.node for every tunnel that is a member of a VLAN on which a RIF and then
 * IPMC member is created or a member of L2MC group assigned to such IPMC
 * member.
 *
 * object_id here may be an oid of a vlan or l2mc_group
 */
switch_status_t evaluate_tunnel_members(switch_object_id_t parent_ipmc_member,
                                        switch_object_id_t object_id,
                                        bool cleanup) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t device_handle = {}, member_handle = {},
                     ipmc_member_vlan_tunnel = {}, ipmc_group_handle = {};
  std::set<switch_object_id_t> ipmc_member_vlan_tunnel_set;
  std::vector<switch_object_id_t> existing_tunnel_members;
  std::vector<switch_object_id_t> group_members;
  switch_attr_id_t member_output_attr_id;
  switch_attr_id_t device_attr_id;
  bool update_mgid = false;
  uint16_t rid;

  // determine attr_ids according to object type
  switch_object_type_t ot = switch_store::object_type_query(object_id);
  switch (ot) {
    case SWITCH_OBJECT_TYPE_VLAN: {
      device_attr_id = SWITCH_VLAN_ATTR_DEVICE;
      member_output_attr_id = SWITCH_VLAN_MEMBER_ATTR_MEMBER_HANDLE;

      const auto &vlan_members_objects = switch_store::get_object_references(
          object_id, SWITCH_OBJECT_TYPE_VLAN_MEMBER);
      for (const auto &item : vlan_members_objects) {
        group_members.push_back(item.oid);
      }
    } break;
    case SWITCH_OBJECT_TYPE_L2MC_GROUP: {
      device_attr_id = SWITCH_L2MC_GROUP_ATTR_DEVICE;
      member_output_attr_id = SWITCH_L2MC_MEMBER_ATTR_OUTPUT_HANDLE;

      status |= switch_store::v_get(
          object_id, SWITCH_L2MC_GROUP_ATTR_L2MC_MEMBERS, group_members);
    } break;
    default: {
      status |= SWITCH_STATUS_INVALID_PARAMETER;
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_MC_NODE_TUNNEL,
                 "{}.{}: Incorrect object type {}; status={}",
                 __func__,
                 __LINE__,
                 switch_store::object_name_get_from_type(ot),
                 status);
      return status;
    }
  }
  // get existing instances of ipmc_member_vlan_tunnel corresponding to tunnel
  // objects that are members of a vlan being a member of an ipmc_group
  status |= switch_store::v_get(parent_ipmc_member,
                                SWITCH_IPMC_MEMBER_ATTR_VLAN_TUNNEL_MEMBERS,
                                existing_tunnel_members);

  status |= switch_store::v_get(object_id, device_attr_id, device_handle);

  if (!cleanup) {
    for (auto member : group_members) {
      status |=
          switch_store::v_get(member, member_output_attr_id, member_handle);

      if (switch_store::object_type_query(member_handle) ==
          SWITCH_OBJECT_TYPE_TUNNEL) {
        std::set<attr_w> obj_attrs;
        obj_attrs.insert(
            attr_w(SWITCH_IPMC_MEMBER_VLAN_TUNNEL_ATTR_DEVICE, device_handle));
        obj_attrs.insert(attr_w(
            SWITCH_IPMC_MEMBER_VLAN_TUNNEL_ATTR_TUNNEL_HANDLE, member_handle));
        obj_attrs.insert(
            attr_w(SWITCH_IPMC_MEMBER_VLAN_TUNNEL_ATTR_IPMC_MEMBER_HANDLE,
                   parent_ipmc_member));

        status |= switch_store::object_id_get_wkey(
            SWITCH_OBJECT_TYPE_IPMC_MEMBER_VLAN_TUNNEL,
            obj_attrs,
            ipmc_member_vlan_tunnel);
        if (status == SWITCH_STATUS_ITEM_NOT_FOUND) {
          // allocate rid for tunnel
          rid = compute_rid({.data = SWITCH_NULL_OBJECT_ID});
          obj_attrs.insert(
              attr_w(SWITCH_IPMC_MEMBER_VLAN_TUNNEL_ATTR_RID, rid));
          // create new ipmc_member_vlan_tunnel if not exists
          status = switch_store::object_create(
              SWITCH_OBJECT_TYPE_IPMC_MEMBER_VLAN_TUNNEL,
              obj_attrs,
              ipmc_member_vlan_tunnel);

          if (status != SWITCH_STATUS_SUCCESS) {
            // release allocated rid value
            status |= SWITCH_CONTEXT.rid_release(rid);
          } else {
            update_mgid = true;
          }
        } else {
          // remove the ipmc_member_vlan_tunnel instance from the list if it
          // already exists the rest of instances will be further deleted
          existing_tunnel_members.erase(find(existing_tunnel_members.begin(),
                                             existing_tunnel_members.end(),
                                             ipmc_member_vlan_tunnel));
        }
      }
    }
  }

  // now remove all remaining instances of ipmc_member_vlan_tunnel that are
  // not relevant anymore
  if (existing_tunnel_members.size() != 0) update_mgid = true;
  for (auto member : existing_tunnel_members) {
    // release allocated rid value
    status |= switch_store::v_get(
        member, SWITCH_IPMC_MEMBER_VLAN_TUNNEL_ATTR_RID, rid);
    status |= SWITCH_CONTEXT.rid_release(rid);

    status |= switch_store::object_delete(member);
  }

  if (update_mgid) {
    // Refresh mc_mgid entry
    status |= switch_store::v_get(parent_ipmc_member,
                                  SWITCH_IPMC_MEMBER_ATTR_IPMC_GROUP_HANDLE,
                                  ipmc_group_handle);
    mc_mgid upd_mgid(ipmc_group_handle, status);
    upd_mgid.create_update();
  }

  return status;
}

/**
 * mc_node_tunnel (with parent l2mc_member_tunnel or ipmc_member_vlan_tunnel) is
 * intended for creating Level1 nodes for L2MC functionalities and IPMC support
 * for SVI members of type tunnel. In this case mgid has one Level1 node
 * for all ports and LAGs and separate nodes for each tunnel belonging to the
 * l2mc_group (in case of L2MC) or SVI. This class creates Level1 nodes for
 * tunnels. Each such node will have only one corresponding Level2 node,
 * determined from nexthop resolution of tunnel replication resolution's flood
 * nexthop handle.
 */
class mc_node_tunnel : public p4_object_pd_fixed {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_MC_NODE_TUNNEL;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_MC_NODE_TUNNEL_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_MC_NODE_TUNNEL_ATTR_STATUS;

 public:
  switch_attr_id_t tunnel_attr_id;
  mc_node_tunnel(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_pd_fixed(smi_id::T_PRE_NODE,
                           status_attr_id,
                           auto_ot,
                           parent_attr_id,
                           parent) {
    switch_object_id_t tunnel_handle = {};
    std::vector<uint32_t> port_map, lag_map;

    switch (switch_store::object_type_query(parent)) {
      case SWITCH_OBJECT_TYPE_L2MC_MEMBER_TUNNEL:
        tunnel_attr_id = SWITCH_L2MC_MEMBER_TUNNEL_ATTR_TUNNEL_HANDLE;
        break;
      case SWITCH_OBJECT_TYPE_IPMC_MEMBER_VLAN_TUNNEL:
        tunnel_attr_id = SWITCH_IPMC_MEMBER_VLAN_TUNNEL_ATTR_TUNNEL_HANDLE;
        break;
      default:
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_MC_NODE_TUNNEL,
                   "{}.{}: Incorrect parent object type {}",
                   __func__,
                   __LINE__,
                   switch_store::object_name_get_from_type(
                       switch_store::object_type_query(parent)));
        status |= SWITCH_STATUS_INVALID_PARAMETER;
        clear_attrs();
        return;
    }

    status |= switch_store::v_get(parent, tunnel_attr_id, tunnel_handle);

    status |= get_port_lag_map_from_tunnel(tunnel_handle, port_map, lag_map);

    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_MC_NODE_VLAN_MEMBER,
                 "{}.{}: Failed to get port_lag_map from tunnel",
                 __func__,
                 __LINE__);
      clear_attrs();
      return;
    }

    action_entry.init_indirect_data();
    status |=
        action_entry.set_arg(smi_id::D_PRE_NODE_DEV_PORT, port_map, false);
    status |= action_entry.set_arg(
        smi_id::D_PRE_NODE_MULTICAST_LAG_ID, lag_map, false);
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_object_id_t auto_oid = auto_obj.get_auto_oid();
    switch_object_id_t parent = auto_obj.get_parent();
    bool allocate = auto_oid == 0 ? true : false;
    switch_object_id_t tunnel_handle = {};
    uint32_t node_index = 0;

    status |= switch_store::v_get(parent, tunnel_attr_id, tunnel_handle);

    if (switch_store::object_type_query(tunnel_handle) !=
        SWITCH_OBJECT_TYPE_TUNNEL) {
      return SWITCH_STATUS_SUCCESS;
    }

    if (allocate) {
      // object is created for the first time - allocate node index
      node_index = SWITCH_CONTEXT.pre_node_allocate();

      auto insert_ret = auto_obj.attrs.insert(
          attr_w(SWITCH_MC_NODE_TUNNEL_ATTR_INDEX, node_index));
      if (insert_ret.second == false) status |= SWITCH_STATUS_FAILURE;
    } else {
      // object already exists - get stored node index
      status |= switch_store::v_get(
          auto_oid, SWITCH_MC_NODE_TUNNEL_ATTR_INDEX, node_index);

      if (switch_store::smiContext::context().in_warm_init()) {
        SWITCH_CONTEXT.pre_node_reserve(node_index);
      }
    }

    status |=
        match_key.set_exact(smi_id::F_PRE_NODE_MULTICAST_NODE_ID, node_index);

    switch (switch_store::object_type_query(parent)) {
      case SWITCH_OBJECT_TYPE_L2MC_MEMBER_TUNNEL: {
        switch_object_id_t dst_ip_handle = {}, outer_fib_handle = {},
                           trr_handle = {};

        status |= switch_store::v_get(
            tunnel_handle, SWITCH_TUNNEL_ATTR_DST_IP_HANDLE, dst_ip_handle);
        status |= find_auto_oid(dst_ip_handle,
                                SWITCH_OBJECT_TYPE_OUTER_FIB_TABLE,
                                outer_fib_handle);
        status |=
            find_auto_oid(outer_fib_handle,
                          SWITCH_OBJECT_TYPE_TUNNEL_REPLICATION_RESOLUTION,
                          trr_handle);
        status |= action_entry.set_arg(smi_id::D_PRE_NODE_MULTICAST_RID,
                                       compute_rid(trr_handle));
      } break;
      case SWITCH_OBJECT_TYPE_IPMC_MEMBER_VLAN_TUNNEL: {
        status |= action_entry.set_arg(smi_id::D_PRE_NODE_MULTICAST_RID,
                                       compute_rid(parent));
      } break;
      default:
        break;
    }

    status = p4_object_pd_fixed::create_update();

    // release allocated node index if the object was not created successfully
    if (status != SWITCH_STATUS_SUCCESS && allocate) {
      SWITCH_CONTEXT.pre_node_release(node_index);
    }

    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_object_id_t auto_oid = auto_obj.get_auto_oid();
    switch_object_id_t parent = auto_obj.get_parent();
    switch_object_id_t tunnel_handle = {}, dst_ip_handle = {},
                       outer_fib_handle = {}, trr_handle = {};
    uint32_t node_index = 0;

    status |= switch_store::v_get(parent, tunnel_attr_id, tunnel_handle);

    if (switch_store::object_type_query(tunnel_handle) !=
        SWITCH_OBJECT_TYPE_TUNNEL) {
      return SWITCH_STATUS_SUCCESS;
    }

    // release allocated node index
    status |= switch_store::v_get(
        auto_oid, SWITCH_MC_NODE_TUNNEL_ATTR_INDEX, node_index);

    status |=
        match_key.set_exact(smi_id::F_PRE_NODE_MULTICAST_NODE_ID, node_index);

    status |= switch_store::v_get(
        tunnel_handle, SWITCH_TUNNEL_ATTR_DST_IP_HANDLE, dst_ip_handle);
    status |= find_auto_oid(
        dst_ip_handle, SWITCH_OBJECT_TYPE_OUTER_FIB_TABLE, outer_fib_handle);
    status |= find_auto_oid(outer_fib_handle,
                            SWITCH_OBJECT_TYPE_TUNNEL_REPLICATION_RESOLUTION,
                            trr_handle);
    status |= action_entry.set_arg(smi_id::D_PRE_NODE_MULTICAST_RID,
                                   compute_rid(trr_handle));

    status |= SWITCH_CONTEXT.pre_node_release(node_index);

    status |= p4_object_pd_fixed::del();

    return status;
  }
};

/**
 * ipmc_node (with parent ipmc_member) is intended for creating Level1 nodes for
 * IPMC functionalities. In this case mgid has separate nodes for each
 * ipmc_member regardless its type is RIF or tunnel.
 * This class creates Level1 nodes for RIFs. Each of such nodes will have single
 * Level2 node if the type of the RIF is port or sub-port or several Level2
 * nodes if the type of the RIF is VLAN (then the node will be created for each
 * vlan_member unless there is SWITCH_IPMC_MEMBER_ATTR_L2MC_GROUP_HANDLE
 * attribute specified - in this case there will be nodes for particular members
 * of given l2mc_group).
 * While ipmc_node does not directly create/update/delete any nodes for tunnel
 * members, it does trigger the (re-)evaluation of tunnel members of the
 * ipmc_member. This indirectly leads to create/update/delete of
 * ipmc_member_vlan_tunnel user objects, which leads to create/update/delete of
 * mc_node_tunnel objects that directly program nodes for tunnel members.
 *                        +--------+
 *                        |  mgid  |  (per ipmc_group)
 *                        +--------+
 * (per vlan rif          / |    \ \_ _ _ _ _ _ _ _ _ _
 *  ipmc_member) +-------+  +     +---------+           \
 *              /          /                |            \
 *         +--------+    +--------+     +--------+    +--------+
 *         | node_a | .. | node_k |     | node_m | .. | node_z | (per non-vlan
 *         +--------+    +--------+     +--------+    +--------+  rif member)
 *          /  |   \        |   \_ _ _ _         \            \
 *         /   |    \     +------+      \       +-----+      +-----+
 *        /    |     \    | p_k0 | .. +------+  | p_m |  ..  | p_z |
 *       /     |  ..  \   +------+    | p_kn |  +-----+      +-----+
 * +------+ +------+    +------+      +------+
 * | p_a0 | | p_a1 | .. | p_an |
 * +------+ +------+    +------+
 */
class ipmc_node : public p4_object_pd_fixed {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_IPMC_NODE;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_IPMC_NODE_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id = SWITCH_IPMC_NODE_ATTR_STATUS;

 public:
  ipmc_node(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_pd_fixed(smi_id::T_PRE_NODE,
                           status_attr_id,
                           auto_ot,
                           parent_attr_id,
                           parent) {
    switch_object_type_t parent_ot = switch_store::object_type_query(parent);
    switch_object_id_t ipmc_output = {}, member_handle = {};
    std::vector<uint32_t> port_map, lag_map;
    switch_enum_t rif_type;

    if (parent_ot != SWITCH_OBJECT_TYPE_IPMC_MEMBER) {
      clear_attrs();
      return;
    }

    status |= switch_store::v_get(
        parent, SWITCH_IPMC_MEMBER_ATTR_OUTPUT_HANDLE, ipmc_output);

    switch (switch_store::object_type_query(ipmc_output)) {
      case SWITCH_OBJECT_TYPE_RIF: {
        status |=
            switch_store::v_get(ipmc_output, SWITCH_RIF_ATTR_TYPE, rif_type);
        if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_PORT ||
            rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_SUB_PORT) {
          status |= switch_store::v_get(
              ipmc_output, SWITCH_RIF_ATTR_PORT_HANDLE, member_handle);

          switch (switch_store::object_type_query(member_handle)) {
            case SWITCH_OBJECT_TYPE_PORT: {
              uint16_t dev_port = 0;
              status |= switch_store::v_get(
                  member_handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
              port_map.push_back(dev_port);
            } break;
            case SWITCH_OBJECT_TYPE_LAG:
              lag_map.push_back(switch_store::handle_to_id(member_handle));
              break;
            default:
              break;
          }
        } else if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_VLAN) {
          std::vector<switch_object_id_t> group_members;
          switch_object_id_t vlan_handle = {}, l2mc_group_handle = {};
          switch_attr_id_t member_handle_attr_id;

          // check whether there is corresponding l2mc_group assigned
          status |=
              switch_store::v_get(parent,
                                  SWITCH_IPMC_MEMBER_ATTR_L2MC_GROUP_HANDLE,
                                  l2mc_group_handle);

          if (l2mc_group_handle.data != 0) {
            // there is corresponding l2mc_group, retrieve its members
            if (switch_store::object_type_query(l2mc_group_handle) !=
                SWITCH_OBJECT_TYPE_L2MC_GROUP) {
              switch_log(
                  SWITCH_API_LEVEL_ERROR,
                  SWITCH_OBJECT_TYPE_IPMC_NODE,
                  "{}.{}: Incorrect object type {} of object assigned to "
                  "l2mc_group_handle attribute of parent",
                  __func__,
                  __LINE__,
                  switch_store::object_name_get_from_type(
                      switch_store::object_type_query(l2mc_group_handle)));
              return;
            }

            status |= switch_store::v_get(l2mc_group_handle,
                                          SWITCH_L2MC_GROUP_ATTR_L2MC_MEMBERS,
                                          group_members);

            member_handle_attr_id = SWITCH_L2MC_MEMBER_ATTR_OUTPUT_HANDLE;
          } else {
            // there is no corresponding l2mc_group, retrieve all VLAN members
            status |= switch_store::v_get(
                ipmc_output, SWITCH_RIF_ATTR_VLAN_HANDLE, vlan_handle);

            // store vlan object ID for dependency
            auto_obj.attrs.insert(
                attr_w(SWITCH_IPMC_NODE_ATTR_VLAN_HANDLE, vlan_handle));

            const auto &vlan_members_objects =
                switch_store::get_object_references(
                    vlan_handle, SWITCH_OBJECT_TYPE_VLAN_MEMBER);
            for (const auto &item : vlan_members_objects) {
              group_members.push_back(item.oid);
            }

            member_handle_attr_id = SWITCH_VLAN_MEMBER_ATTR_MEMBER_HANDLE;
          }

          for (auto member : group_members) {
            status |= switch_store::v_get(
                member, member_handle_attr_id, member_handle);
            switch (switch_store::object_type_query(member_handle)) {
              case SWITCH_OBJECT_TYPE_PORT: {
                uint16_t dev_port = 0;
                status |= switch_store::v_get(
                    member_handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
                port_map.push_back(dev_port);
              } break;
              case SWITCH_OBJECT_TYPE_LAG:
                lag_map.push_back(switch_store::handle_to_id(member_handle));
                break;
              case SWITCH_OBJECT_TYPE_TUNNEL:
                // handled by mc_node_tunnel class
              default:
                break;
            }
          }
        } else {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_IPMC_NODE,
                     "{}.{}: Incorrect RIF type of RIF assigned to ipmc_member "
                     "output_handle",
                     __func__,
                     __LINE__);
          status |= SWITCH_STATUS_INVALID_PARAMETER;
          clear_attrs();
          return;
        }
      } break;
      case SWITCH_OBJECT_TYPE_TUNNEL: {
        // not yet supported
        // will be handled by ipmc_node_tunnel class
        clear_attrs();
        return;
      } break;
      default: {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_IPMC_NODE,
                   "{}.{}: Incorrect object type {} of parent ipmc_member "
                   "output_handle",
                   __func__,
                   __LINE__,
                   switch_store::object_name_get_from_type(
                       switch_store::object_type_query(ipmc_output)));
        status |= SWITCH_STATUS_INVALID_PARAMETER;
        clear_attrs();
        return;
      }
    }

    action_entry.init_indirect_data();
    status |=
        action_entry.set_arg(smi_id::D_PRE_NODE_DEV_PORT, port_map, false);
    status |= action_entry.set_arg(
        smi_id::D_PRE_NODE_MULTICAST_LAG_ID, lag_map, false);
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_object_id_t auto_oid = auto_obj.get_auto_oid();
    switch_object_id_t parent = auto_obj.get_parent();
    bool allocate = auto_oid == 0 ? true : false;
    switch_object_id_t rif_handle = {};
    switch_object_type_t parent_ot;
    switch_enum_t rif_type;
    uint32_t node_index = 0;
    uint16_t rid = 0;

    parent_ot = switch_store::object_type_query(parent);
    if (parent_ot != SWITCH_OBJECT_TYPE_IPMC_MEMBER) {
      return status;
    }

    status |= switch_store::v_get(
        parent, SWITCH_IPMC_MEMBER_ATTR_OUTPUT_HANDLE, rif_handle);

    if (switch_store::object_type_query(rif_handle) != SWITCH_OBJECT_TYPE_RIF) {
      // other cases not supported yet
      // tunnels will be handled by ipmc_node_tunnel class
      return status;
    }

    rid = compute_rid(rif_handle);

    if (allocate) {
      // object is created for the first time - allocate node index
      node_index = SWITCH_CONTEXT.pre_node_allocate();

      // store node index
      auto insert_ret = auto_obj.attrs.insert(
          attr_w(SWITCH_IPMC_NODE_ATTR_INDEX, node_index));
      if (insert_ret.second == false) status |= SWITCH_STATUS_FAILURE;
    } else {
      // object already exists get stored node index
      status |= switch_store::v_get(
          auto_oid, SWITCH_IPMC_NODE_ATTR_INDEX, node_index);

      if (switch_store::smiContext::context().in_warm_init()) {
        SWITCH_CONTEXT.pre_node_reserve(node_index);
      }
    }

    status |=
        match_key.set_exact(smi_id::F_PRE_NODE_MULTICAST_NODE_ID, node_index);
    status |= action_entry.set_arg(smi_id::D_PRE_NODE_MULTICAST_RID, rid);

    status |= p4_object_pd_fixed::create_update();

    // release allocated node index if the object was not created successfully
    if (status != SWITCH_STATUS_SUCCESS && allocate) {
      SWITCH_CONTEXT.pre_node_release(node_index);
    } else {
      // take care of creating/updating pre.nodes for tunnel SVI members
      status |= switch_store::v_get(rif_handle, SWITCH_RIF_ATTR_TYPE, rif_type);

      if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_VLAN) {
        switch_object_id_t l2mc_group_handle = {};
        // check whether there is corresponding l2mc_group assigned
        status |= switch_store::v_get(parent,
                                      SWITCH_IPMC_MEMBER_ATTR_L2MC_GROUP_HANDLE,
                                      l2mc_group_handle);
        if (l2mc_group_handle.data != 0) {
          status |= evaluate_tunnel_members(parent, l2mc_group_handle, false);
        } else {
          switch_object_id_t vlan_handle;
          status |= switch_store::v_get(
              rif_handle, SWITCH_RIF_ATTR_VLAN_HANDLE, vlan_handle);
          status |= evaluate_tunnel_members(parent, vlan_handle, false);
        }
      }
    }

    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_object_id_t auto_oid = auto_obj.get_auto_oid();
    switch_object_id_t parent = auto_obj.get_parent();
    switch_object_id_t rif_handle = {};
    switch_enum_t rif_type;
    uint32_t node_index = 0;
    uint16_t rid = 0;

    if (switch_store::object_type_query(parent) !=
        SWITCH_OBJECT_TYPE_IPMC_MEMBER)
      return status;

    status |=
        switch_store::v_get(auto_oid, SWITCH_IPMC_NODE_ATTR_INDEX, node_index);

    status |= switch_store::v_get(
        parent, SWITCH_IPMC_MEMBER_ATTR_OUTPUT_HANDLE, rif_handle);

    if (switch_store::object_type_query(rif_handle) != SWITCH_OBJECT_TYPE_RIF) {
      // other cases not supported yet
      // tunnels will be handled by ipmc_node_tunnel class
      return status;
    }

    rid = compute_rid(rif_handle);

    status |=
        match_key.set_exact(smi_id::F_PRE_NODE_MULTICAST_NODE_ID, node_index);
    action_entry.init_indirect_data();
    status |= action_entry.set_arg(smi_id::D_PRE_NODE_MULTICAST_RID, rid);

    status |= SWITCH_CONTEXT.pre_node_release(node_index);

    status |= switch_store::v_get(rif_handle, SWITCH_RIF_ATTR_TYPE, rif_type);

    if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_VLAN) {
      switch_object_id_t l2mc_group_handle = {};
      // check whether there is corresponding l2mc_group assigned
      status |= switch_store::v_get(
          parent, SWITCH_IPMC_MEMBER_ATTR_L2MC_GROUP_HANDLE, l2mc_group_handle);
      if (l2mc_group_handle.data != 0) {
        status |= evaluate_tunnel_members(parent, l2mc_group_handle, true);
      } else {
        switch_object_id_t vlan_handle;
        status |= switch_store::v_get(
            rif_handle, SWITCH_RIF_ATTR_VLAN_HANDLE, vlan_handle);
        status |= evaluate_tunnel_members(parent, vlan_handle, true);
      }
    }

    status |= p4_object_pd_fixed::del();

    return status;
  }
};

class bd_flood : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_BD_FLOOD;
  static const switch_attr_id_t status_attr_id = SWITCH_BD_FLOOD_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_BD_FLOOD_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t flood_attr_id =
      SWITCH_BD_FLOOD_ATTR_FLOOD_HANDLE;

 public:
  bd_flood(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_BD_FLOOD,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    status = SWITCH_STATUS_SUCCESS;
    auto it = match_action_list.begin();
    switch_object_id_t vlan_handle = {0}, rif_handle = {0},
                       mrouter_mc_handle = {0}, bridge_handle = {0},
                       vrf_handle = {0};
    uint32_t mgid = 0;

    // do not create flood entry for rif or vrf
    status |= get_parent_of_bd(
        parent, vlan_handle, bridge_handle, rif_handle, vrf_handle);
    if (vlan_handle.data == 0 && bridge_handle.data == 0) return;

    mgid = compute_pre_mgid(vlan_handle);
    std::map<uint8_t, uint32_t> mgid_map{{SWITCH_PACKET_TYPE_UNICAST, mgid},
                                         {SWITCH_PACKET_TYPE_MULTICAST, mgid},
                                         {SWITCH_PACKET_TYPE_BROADCAST, mgid}};
    // set mgid = 0 when flooding is disabled on the vlan packet type
    if (vlan_handle != 0) {
      switch_enum_t flood_type;
      status |= switch_store::v_get(
          vlan_handle, SWITCH_VLAN_ATTR_UNKNOWN_UCAST_FLOOD_TYPE, flood_type);
      if (flood_type.enumdata !=
          SWITCH_VLAN_ATTR_UNKNOWN_UCAST_FLOOD_TYPE_ALL) {
        mgid_map[SWITCH_PACKET_TYPE_UNICAST] = 0;
      }

      status |= switch_store::v_get(
          vlan_handle, SWITCH_VLAN_ATTR_UNKNOWN_MCAST_FLOOD_TYPE, flood_type);
      if (flood_type.enumdata !=
          SWITCH_VLAN_ATTR_UNKNOWN_MCAST_FLOOD_TYPE_ALL) {
        mgid_map[SWITCH_PACKET_TYPE_MULTICAST] = 0;
      }

      status |= switch_store::v_get(
          vlan_handle, SWITCH_VLAN_ATTR_BCAST_FLOOD_TYPE, flood_type);
      if (flood_type.enumdata != SWITCH_VLAN_ATTR_BCAST_FLOOD_TYPE_ALL) {
        mgid_map[SWITCH_PACKET_TYPE_BROADCAST] = 0;
      }
    }

    // Install U/M/C entries
    std::set<uint8_t> l2_pkt_types({SWITCH_PACKET_TYPE_UNICAST,
                                    SWITCH_PACKET_TYPE_MULTICAST,
                                    SWITCH_PACKET_TYPE_BROADCAST});
    for (auto pkt_type : l2_pkt_types) {
      it = match_action_list.insert(
          it,
          std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_BD_FLOOD),
                                             _ActionEntry(smi_id::T_BD_FLOOD)));
      status |= it->first.set_exact(smi_id::F_BD_FLOOD_PKT_TYPE, pkt_type);
      status |= it->first.set_exact(smi_id::F_BD_FLOOD_BD, compute_bd(parent));

      it->second.init_action_data(smi_id::A_FLOOD);
      status |= it->second.set_arg(smi_id::P_FLOOD_MGID,
                                   static_cast<uint16_t>(mgid_map[pkt_type]));
    }

    // Now add the mrouter entry also after the fetching mrouter_handle
    if (feature::is_feature_set(SWITCH_FEATURE_MULTICAST)) {
      uint8_t flood_to_mrouters = 1;
      status |= switch_store::v_get(
          vlan_handle, SWITCH_VLAN_ATTR_MROUTER_MC_HANDLE, mrouter_mc_handle);
      if (mrouter_mc_handle.data != 0) {
        mgid = compute_pre_mgid(vlan_handle);
      }

      it = match_action_list.insert(
          it,
          std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_BD_FLOOD),
                                             _ActionEntry(smi_id::T_BD_FLOOD)));
      status |= it->first.set_exact(smi_id::F_BD_FLOOD_BD, compute_bd(parent));
      status |= it->first.set_exact(
          smi_id::F_BD_FLOOD_PKT_TYPE,
          static_cast<uint8_t>(SWITCH_PACKET_TYPE_MULTICAST));
      status |= it->first.set_exact(smi_id::F_BD_FLOOD_FLOOD_TO_MROUTERS,
                                    flood_to_mrouters);

      it->second.init_action_data(smi_id::A_FLOOD);
      status |=
          it->second.set_arg(smi_id::P_FLOOD_MGID, static_cast<uint16_t>(mgid));
    }
  }
};

class rid_table : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_RID_TABLE;
  static const switch_attr_id_t status_attr_id = SWITCH_RID_TABLE_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_RID_TABLE_ATTR_PARENT_HANDLE;
  std::unique_ptr<object> mobject;

 public:
  rid_table(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(
            smi_id::T_RID, status_attr_id, auto_ot, parent_attr_id, parent) {
    switch_object_type_t parent_ot = switch_store::object_type_query(parent);
    switch_object_id_t bd_handle = {}, nexthop = {}, outer_fib = {},
                       tunnel_dest_ip = {};

    if (parent_ot == SWITCH_OBJECT_TYPE_VLAN) {
      action_entry.init_action_data(smi_id::A_RID_HIT);
      status |= match_key.set_exact(smi_id::F_RID_REPLICATION_ID,
                                    compute_rid(parent));

      status |= find_auto_oid(parent, SWITCH_OBJECT_TYPE_BD, bd_handle);
      status |=
          action_entry.set_arg(smi_id::P_RID_HIT_BD, compute_bd(bd_handle));
    } else if (parent_ot == SWITCH_OBJECT_TYPE_TUNNEL_REPLICATION_RESOLUTION &&
               feature::is_feature_set(SWITCH_FEATURE_L2_VXLAN)) {
      status |= match_key.set_exact(smi_id::F_RID_REPLICATION_ID,
                                    compute_rid(parent));

      status |= switch_store::v_get(
          parent,
          SWITCH_TUNNEL_REPLICATION_RESOLUTION_ATTR_FLOOD_NEXTHOP_HANDLE,
          nexthop);

      action_entry.init_action_data(smi_id::A_RID_TUNNEL_RID_HIT);
      status |= action_entry.set_arg(smi_id::P_RID_TUNNEL_RID_HIT_NEXTHOP,
                                     compute_nexthop_index(nexthop));

      status |= switch_store::v_get(
          parent,
          SWITCH_TUNNEL_REPLICATION_RESOLUTION_ATTR_PARENT_HANDLE,
          outer_fib);

      status |= switch_store::v_get(
          outer_fib, SWITCH_OUTER_FIB_TABLE_ATTR_PARENT_HANDLE, tunnel_dest_ip);

      // look at tunnel_members to determine corresponding tunnel_handle
      std::vector<switch_object_id_t> tunnel_members;
      status |= switch_store::v_get(tunnel_dest_ip,
                                    SWITCH_TUNNEL_DEST_IP_ATTR_TUNNEL_MEMBERS,
                                    tunnel_members);

      // There are two cases where tunnel_members may be empty:
      // 1. When tunnel_dest_ip creation is triggered by a L3 nexthop, and
      //    there is no corresponding p2p tunnel for L2 flooding.
      //    In this case there will be no PRE nodes programmed with this rid
      //    value. No traffic will be received matching this entry, so
      //    tunnel_nexthop programming does not matter.
      // 2. Since tunnel_dest_ip creation is triggered in before_tunnel_create,
      //    when tunnel_rid is created it is possible for tunnel_members to be
      //    empty. Soon afterwards, the nexthop for the p2p tunnel will be
      //    created, causing the tunnel_members attribute of tunnel_dest_ip
      //    to be populated. This will happen before any PRE nodes are
      //    programmed with this rid value, since those PRE nodes are created
      //    when a vlan_member based on the p2p tunnel is created.
      if (!tunnel_members.empty()) {
        status |=
            action_entry.set_arg(smi_id::P_RID_TUNNEL_RID_HIT_TUNNEL_NEXTHOP,
                                 compute_nexthop_index(tunnel_members[0]));
      }
    } else if (parent_ot == SWITCH_OBJECT_TYPE_IPMC_MEMBER) {
      // this is not the actual parent - we must have rid_table entry per each
      // rif object that is used by any ipmc_member
      switch_object_id_t output_handle = {};
      switch_object_type_t output_type;
      switch_enum_t rif_type;

      status |= switch_store::v_get(
          parent, SWITCH_IPMC_MEMBER_ATTR_OUTPUT_HANDLE, output_handle);

      output_type = switch_store::object_type_query(output_handle);

      if (output_type == SWITCH_OBJECT_TYPE_RIF) {
        status |=
            switch_store::v_get(output_handle, SWITCH_RIF_ATTR_TYPE, rif_type);
        if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_PORT ||
            rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_SUB_PORT) {
          mobject =
              std::unique_ptr<rid_table>(new rid_table(output_handle, status));
        }  // for SWITCH_RIF_ATTR_TYPE_VLAN rid_table entry is handled with a
           // parent of type SWITCH_OBJECT_TYPE_IPMC_MEMBER_VLAN_TUNNEL
      }
      clear_attrs();
      return;
    } else if (parent_ot == SWITCH_OBJECT_TYPE_RIF) {
      switch_object_id_t ipmc_member = {};
      switch_enum_t rif_type;

      // check if there is at least one corresponding ipmc_member
      status |=
          find_auto_oid(parent, SWITCH_OBJECT_TYPE_IPMC_MEMBER, ipmc_member);
      if (status != SWITCH_STATUS_SUCCESS || ipmc_member.data == 0) {
        // no corresponding ipmc_member, rid_table entry will not be created
        clear_attrs();
        return;
      }
      status |= switch_store::v_get(parent, SWITCH_RIF_ATTR_TYPE, rif_type);
      if (rif_type.enumdata != SWITCH_RIF_ATTR_TYPE_PORT &&
          rif_type.enumdata != SWITCH_RIF_ATTR_TYPE_SUB_PORT) {
        // SWITCH_RIF_ATTR_TYPE_VLAN does not require separate entry
        // other types are not supported
        clear_attrs();
        return;
      }

      action_entry.init_action_data(smi_id::A_RID_HIT);

      status |= find_auto_oid(parent, SWITCH_OBJECT_TYPE_BD, bd_handle);
      status |=
          action_entry.set_arg(smi_id::P_RID_HIT_BD, compute_bd(bd_handle));

      status |= match_key.set_exact(smi_id::F_RID_REPLICATION_ID,
                                    compute_rid(parent));
    } else if (parent_ot == SWITCH_OBJECT_TYPE_IPMC_MEMBER_VLAN_TUNNEL) {
      switch_object_id_t tunnel_handle = {}, dst_ip_handle = {},
                         outer_fib_handle = {}, trr_handle = {},
                         nexthop_handle = {}, ipmc_member_handle = {},
                         svi_handle = {}, vlan_handle = {};

      status |= match_key.set_exact(smi_id::F_RID_REPLICATION_ID,
                                    compute_rid(parent));

      action_entry.init_action_data(smi_id::A_RID_TUNNEL_MC_RID_HIT);

      status |=
          switch_store::v_get(parent,
                              SWITCH_IPMC_MEMBER_VLAN_TUNNEL_ATTR_TUNNEL_HANDLE,
                              tunnel_handle);

      status |=
          action_entry.set_arg(smi_id::P_RID_TUNNEL_MC_RID_HIT_TUNNEL_NEXTHOP,
                               compute_nexthop_index(tunnel_handle));

      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_RID_TABLE,
                   "{}.{}: Failed to set tunnel_nexthop, status: {}",
                   __func__,
                   __LINE__,
                   status);
      }

      status |= switch_store::v_get(
          tunnel_handle, SWITCH_TUNNEL_ATTR_DST_IP_HANDLE, dst_ip_handle);
      status |= find_auto_oid(
          dst_ip_handle, SWITCH_OBJECT_TYPE_OUTER_FIB_TABLE, outer_fib_handle);
      status |= find_auto_oid(outer_fib_handle,
                              SWITCH_OBJECT_TYPE_TUNNEL_REPLICATION_RESOLUTION,
                              trr_handle);

      status |= switch_store::v_get(
          trr_handle,
          SWITCH_TUNNEL_REPLICATION_RESOLUTION_ATTR_FLOOD_NEXTHOP_HANDLE,
          nexthop_handle);
      status |= action_entry.set_arg(smi_id::P_RID_TUNNEL_MC_RID_HIT_NEXTHOP,
                                     compute_nexthop_index(nexthop_handle));

      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_RID_TABLE,
                   "{}.{}: Failed to set nexthop_handle, status: {}",
                   __func__,
                   __LINE__,
                   status);
      }

      status |= switch_store::v_get(
          parent,
          SWITCH_IPMC_MEMBER_VLAN_TUNNEL_ATTR_IPMC_MEMBER_HANDLE,
          ipmc_member_handle);
      status |= switch_store::v_get(ipmc_member_handle,
                                    SWITCH_IPMC_MEMBER_ATTR_OUTPUT_HANDLE,
                                    svi_handle);
      status |= switch_store::v_get(
          svi_handle, SWITCH_RIF_ATTR_VLAN_HANDLE, vlan_handle);
      status |= get_bd_for_object(vlan_handle, bd_handle);
      status |= action_entry.set_arg(smi_id::P_RID_TUNNEL_MC_RID_HIT_BD,
                                     compute_bd(bd_handle));
    }
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_object_id_t parent = auto_obj.get_parent();
    switch_object_type_t ot = switch_store::object_type_query(parent);

    if (ot == SWITCH_OBJECT_TYPE_IPMC_MEMBER && mobject != NULL) {
      return mobject->create_update();
    } else if (ot == SWITCH_OBJECT_TYPE_RIF) {
      switch_object_id_t ipmc_member = {};

      status |=
          find_auto_oid(parent, SWITCH_OBJECT_TYPE_IPMC_MEMBER, ipmc_member);
      if (status != SWITCH_STATUS_SUCCESS || ipmc_member.data == 0) {
        // no corresponding ipmc_member, rid_table entry will not be created
        return status;
      }
    }

    return p4_object_match_action::create_update();
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_object_id_t parent = auto_obj.get_parent();
    switch_object_type_t ot = switch_store::object_type_query(parent);

    if (ot == SWITCH_OBJECT_TYPE_IPMC_MEMBER) {
      switch_object_id_t rif_handle = {};

      if (mobject == NULL) {
        return status;
      }

      status |= switch_store::v_get(
          parent, SWITCH_IPMC_MEMBER_ATTR_OUTPUT_HANDLE, rif_handle);

      // check whether there is more corresponding ipmc_members
      // if it was the last ipmc_member using given RIF, remove the entry
      const auto &ref_set = switch_store::get_object_references(
          rif_handle, SWITCH_OBJECT_TYPE_IPMC_MEMBER);

      if (ref_set.size() == 1) {
        status |= mobject->del();
      }
      return status;
    }

    return p4_object_match_action::del();
  }
};

class multicast_route_factory : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_MULTICAST_ROUTE_FACTORY;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_MULTICAST_ROUTE_FACTORY_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_MULTICAST_ROUTE_FACTORY_ATTR_STATUS;
  std::unique_ptr<object> mobject;

 public:
  multicast_route_factory(const switch_object_id_t parent,
                          switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    bool is_ipv4;
    switch_ip_prefix_t src_ip = {}, grp_ip = {};

    status |=
        switch_store::v_get(parent, SWITCH_IPMC_ROUTE_ATTR_SRC_IP, src_ip);
    status |=
        switch_store::v_get(parent, SWITCH_IPMC_ROUTE_ATTR_GRP_IP, grp_ip);
    is_ipv4 = (grp_ip.addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4);

    if (is_ipv4) {
      if (src_ip.addr.ip4 == 0) {
        // (X, G)
        mobject = std::unique_ptr<ipv4_multicast_route_x_g>(
            new ipv4_multicast_route_x_g(parent, status));
      } else {
        // (S, G)
        mobject = std::unique_ptr<ipv4_multicast_route_s_g>(
            new ipv4_multicast_route_s_g(parent, status));
      }
    } else {
      switch_ip6_t zero_ip = {0};
      if (memcmp(src_ip.addr.ip6, zero_ip, IPV6_LEN) == 0) {
        // (X, G)
        mobject = std::unique_ptr<ipv6_multicast_route_x_g>(
            new ipv6_multicast_route_x_g(parent, status));
      } else {
        // (S, G)
        mobject = std::unique_ptr<ipv6_multicast_route_s_g>(
            new ipv6_multicast_route_s_g(parent, status));
      }
    }
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (mobject != NULL) {
      status = mobject->create_update();
      if (status != SWITCH_STATUS_SUCCESS) return status;
    }

    return auto_object::create_update();
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (mobject != NULL) {
      status = mobject->del();
      if (status != SWITCH_STATUS_SUCCESS) return status;
    }

    return auto_object::del();
  }

  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    if (mobject != NULL) {
      return mobject->counters_get(handle, cntrs);
    }

    return SWITCH_STATUS_SUCCESS;
  }

  switch_status_t counters_set(const switch_object_id_t handle) {
    if (mobject != NULL) {
      return mobject->counters_set(handle);
    }

    return SWITCH_STATUS_SUCCESS;
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    if (mobject != NULL) {
      return mobject->counters_set(handle, cntr_ids);
    }

    return SWITCH_STATUS_SUCCESS;
  }

  switch_status_t counters_save(const switch_object_id_t parent) {
    if (mobject != NULL) {
      return mobject->counters_save(parent);
    }

    return SWITCH_STATUS_SUCCESS;
  }

  switch_status_t counters_restore(const switch_object_id_t parent) {
    if (mobject != NULL) {
      return mobject->counters_restore(parent);
    }

    return SWITCH_STATUS_SUCCESS;
  }
};

class multicast_bridge_factory : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_MULTICAST_BRIDGE_FACTORY;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_MULTICAST_BRIDGE_FACTORY_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_MULTICAST_BRIDGE_FACTORY_ATTR_STATUS;
  std::unique_ptr<object> mobject;

 public:
  multicast_bridge_factory(const switch_object_id_t parent,
                           switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    bool is_ipv4;
    switch_ip_prefix_t src_ip = {}, grp_ip = {};

    status |=
        switch_store::v_get(parent, SWITCH_L2MC_BRIDGE_ATTR_SRC_IP, src_ip);
    status |=
        switch_store::v_get(parent, SWITCH_L2MC_BRIDGE_ATTR_GRP_IP, grp_ip);
    is_ipv4 = (grp_ip.addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4);

    if (is_ipv4) {
      if (src_ip.addr.ip4 == 0) {
        // (X, G)
        mobject = std::unique_ptr<ipv4_multicast_bridge_x_g>(
            new ipv4_multicast_bridge_x_g(parent, status));
      } else {
        // (S, G)
        mobject = std::unique_ptr<ipv4_multicast_bridge_s_g>(
            new ipv4_multicast_bridge_s_g(parent, status));
      }
    } else {
      switch_ip6_t zero_ip = {0};
      if (memcmp(src_ip.addr.ip6, zero_ip, IPV6_LEN) == 0) {
        // (X, G)
        mobject = std::unique_ptr<ipv6_multicast_bridge_x_g>(
            new ipv6_multicast_bridge_x_g(parent, status));
      } else {
        // (S, G)
        mobject = std::unique_ptr<ipv6_multicast_bridge_s_g>(
            new ipv6_multicast_bridge_s_g(parent, status));
      }
    }
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (mobject != NULL) {
      status = mobject->create_update();
      if (status != SWITCH_STATUS_SUCCESS) return status;
    }

    return auto_object::create_update();
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (mobject != NULL) {
      status = mobject->del();
      if (status != SWITCH_STATUS_SUCCESS) return status;
    }

    return auto_object::del();
  }

  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    if (mobject != NULL) {
      return mobject->counters_get(handle, cntrs);
    }

    return SWITCH_STATUS_SUCCESS;
  }

  switch_status_t counters_set(const switch_object_id_t handle) {
    if (mobject != NULL) {
      return mobject->counters_set(handle);
    }

    return SWITCH_STATUS_SUCCESS;
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    if (mobject != NULL) {
      return mobject->counters_set(handle, cntr_ids);
    }

    return SWITCH_STATUS_SUCCESS;
  }

  switch_status_t counters_save(const switch_object_id_t parent) {
    if (mobject != NULL) {
      return mobject->counters_save(parent);
    }

    return SWITCH_STATUS_SUCCESS;
  }

  switch_status_t counters_restore(const switch_object_id_t parent) {
    if (mobject != NULL) {
      return mobject->counters_restore(parent);
    }

    return SWITCH_STATUS_SUCCESS;
  }
};

class mcast_fwd_result : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_MCAST_FWD_RESULT;
  static const switch_attr_id_t status_attr_id =
      SWITCH_MCAST_FWD_RESULT_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_MCAST_FWD_RESULT_ATTR_PARENT_HANDLE;

 public:
  mcast_fwd_result(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_MCAST_FWD_RESULT,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    switch_object_id_t cpu_handle = {0};
    status =
        switch_store::v_get(parent, SWITCH_DEVICE_ATTR_CPU_PORT, cpu_handle);
    if (cpu_handle == 0) return;

    struct rule_spec {
      uint8_t mcast_hit;
      uint8_t mcast_hit_mask;
      uint8_t ip_type;
      uint8_t ip_type_mask;
      uint8_t ipv4_snooping_hit;
      uint8_t ipv4_snooping_hit_mask;
      uint8_t ipv6_snooping_hit;
      uint8_t ipv6_snooping_hit_mask;
      uint8_t mcast_mode;
      uint8_t mcast_mode_mask;
      uint16_t rpf_check;
      uint16_t rpf_check_mask;
      bf_rt_action_id_t action_id;
      uint8_t mrpf;
      uint8_t flood;
    };

    std::vector<rule_spec> rules;
    size_t cnt = 0;
    const rule_spec empty_rule = {};

    // mroute = hit, bridge = x, rpf = pass, mode = SM
    rules.resize(++cnt, empty_rule);
    rules[cnt - 1].mcast_hit = 1;
    rules[cnt - 1].mcast_hit_mask = 1;
    rules[cnt - 1].rpf_check = 0;
    rules[cnt - 1].rpf_check_mask = 0xFFFF;
    rules[cnt - 1].mcast_mode = SWITCH_IPMC_ROUTE_ATTR_PIM_MODE_SM;
    rules[cnt - 1].mcast_mode_mask = 0x3;
    rules[cnt - 1].action_id = smi_id::A_MCAST_FWD_RESULT_SET_MULTICAST_ROUTE;

    // mroute = hit, bridge = x, rpf = pass, mode = BIDIR
    rules.resize(++cnt, empty_rule);
    rules[cnt - 1].mcast_hit = 1;
    rules[cnt - 1].mcast_hit_mask = 1;
    // rules[cnt - 1].rpf_check = 0xFFFE;
    // rules[cnt - 1].rpf_check_mask = 0xFFFF;
    rules[cnt - 1].mcast_mode = SWITCH_IPMC_ROUTE_ATTR_PIM_MODE_BIDIR;
    rules[cnt - 1].mcast_mode_mask = 0x3;
    rules[cnt - 1].action_id = smi_id::A_MCAST_FWD_RESULT_SET_MULTICAST_ROUTE;

    // mroute = hit, bridge = hit, rpf = fail, mode = SM
    rules.resize(++cnt, empty_rule);
    rules[cnt - 1].mcast_hit = 1;
    rules[cnt - 1].mcast_hit_mask = 1;
    rules[cnt - 1].mcast_mode = SWITCH_IPMC_ROUTE_ATTR_PIM_MODE_SM;
    rules[cnt - 1].mcast_mode_mask = 0x3;
    rules[cnt - 1].action_id = smi_id::A_MCAST_FWD_RESULT_SET_MULTICAST_BRIDGE;
    rules[cnt - 1].mrpf = 1;

    // mroute = hit, bridge = hit, rpf = fail, mode = BIDIR
    rules.resize(++cnt, empty_rule);
    rules[cnt - 1].mcast_hit = 1;
    rules[cnt - 1].mcast_hit_mask = 1;
    rules[cnt - 1].mcast_mode = SWITCH_IPMC_ROUTE_ATTR_PIM_MODE_BIDIR;
    rules[cnt - 1].mcast_mode_mask = 0x3;
    rules[cnt - 1].action_id = smi_id::A_MCAST_FWD_RESULT_SET_MULTICAST_BRIDGE;
    rules[cnt - 1].mrpf = 1;

    // mroute = hit, bridge = miss, rpf = fail, igmp snooping enabled
    rules.resize(++cnt, empty_rule);
    // rules[cnt - 1].mcast_hit = 1;
    // rules[cnt - 1].mcast_hit_mask = 1;
    rules[cnt - 1].mcast_mode = SWITCH_IPMC_ROUTE_ATTR_PIM_MODE_SM;
    rules[cnt - 1].mcast_mode_mask = 0x3;
    rules[cnt - 1].ip_type = SWITCH_IP_TYPE_IPV4;
    rules[cnt - 1].ip_type_mask = 0x3;
    rules[cnt - 1].ipv4_snooping_hit = 1;
    rules[cnt - 1].ipv4_snooping_hit_mask = 1;
    rules[cnt - 1].action_id = smi_id::A_MCAST_FWD_RESULT_SET_MULTICAST_FLOOD;
    rules[cnt - 1].mrpf = 1;
    rules[cnt - 1].flood = 1;

    // mroute = hit, bridge = miss, rpf = fail, mld snooping enabled
    rules.resize(++cnt, empty_rule);
    // rules[cnt - 1].mcast_hit = 1;
    // rules[cnt - 1].mcast_hit_mask = 1;
    rules[cnt - 1].mcast_mode = SWITCH_IPMC_ROUTE_ATTR_PIM_MODE_SM;
    rules[cnt - 1].mcast_mode_mask = 0x3;
    rules[cnt - 1].ip_type = SWITCH_IP_TYPE_IPV6;
    rules[cnt - 1].ip_type_mask = 0x3;
    rules[cnt - 1].ipv6_snooping_hit = 1;
    rules[cnt - 1].ipv6_snooping_hit_mask = 1;
    rules[cnt - 1].action_id = smi_id::A_MCAST_FWD_RESULT_SET_MULTICAST_FLOOD;
    rules[cnt - 1].mrpf = 1;
    rules[cnt - 1].flood = 1;

    // mroute = hit, bridge = miss, rpf = fail, igmp/mld snooping disabled
    rules.resize(++cnt, empty_rule);
    // rules[cnt - 1].mcast_hit = 1;
    // rules[cnt - 1].mcast_hit_mask = 1;
    rules[cnt - 1].mcast_mode = SWITCH_IPMC_ROUTE_ATTR_PIM_MODE_SM;
    rules[cnt - 1].mcast_mode_mask = 0x3;
    rules[cnt - 1].action_id = smi_id::A_MCAST_FWD_RESULT_SET_MULTICAST_FLOOD;

    // bridge = hit
    rules.resize(++cnt, empty_rule);
    rules[cnt - 1].mcast_hit = 1;
    rules[cnt - 1].mcast_hit_mask = 1;
    rules[cnt - 1].action_id = smi_id::A_MCAST_FWD_RESULT_SET_MULTICAST_BRIDGE;

    // bridge = miss, pkt_type = ipv4 multicast (not link-local), igmp snooping
    rules.resize(++cnt, empty_rule);
    rules[cnt - 1].ip_type = SWITCH_IP_TYPE_IPV4;
    rules[cnt - 1].ip_type_mask = 0x3;
    rules[cnt - 1].ipv4_snooping_hit = 1;
    rules[cnt - 1].ipv4_snooping_hit_mask = 1;
    rules[cnt - 1].action_id = smi_id::A_MCAST_FWD_RESULT_SET_MULTICAST_FLOOD;
    rules[cnt - 1].mrpf = 1;
    rules[cnt - 1].flood = 1;

    // bridge = miss, pkt_type = ipv6 multicast (not link-local), mld snooping
    rules.resize(++cnt, empty_rule);
    rules[cnt - 1].ip_type = SWITCH_IP_TYPE_IPV6;
    rules[cnt - 1].ip_type_mask = 0x3;
    rules[cnt - 1].ipv6_snooping_hit = 1;
    rules[cnt - 1].ipv6_snooping_hit_mask = 1;
    rules[cnt - 1].action_id = smi_id::A_MCAST_FWD_RESULT_SET_MULTICAST_FLOOD;
    rules[cnt - 1].mrpf = 1;
    rules[cnt - 1].flood = 1;

    // bridge = miss, pkt_type = ipv4 multicast, igmp snooping disabled
    rules.resize(++cnt, empty_rule);
    rules[cnt - 1].ip_type = SWITCH_IP_TYPE_IPV4;
    rules[cnt - 1].ip_type_mask = 0x3;
    rules[cnt - 1].action_id = smi_id::A_MCAST_FWD_RESULT_SET_MULTICAST_FLOOD;

    // bridge = miss, pkt_type = ipv6 multicast, mld snooping disabled
    rules.resize(++cnt, empty_rule);
    rules[cnt - 1].ip_type = SWITCH_IP_TYPE_IPV6;
    rules[cnt - 1].ip_type_mask = 0x3;
    rules[cnt - 1].action_id = smi_id::A_MCAST_FWD_RESULT_SET_MULTICAST_FLOOD;

    auto it = match_action_list.begin();
    for (uint32_t i = 0; i < rules.size(); i++) {
      it = match_action_list.insert(
          it,
          std::pair<_MatchKey, _ActionEntry>(
              _MatchKey(smi_id::T_MCAST_FWD_RESULT),
              _ActionEntry(smi_id::T_MCAST_FWD_RESULT)));
      status |= it->first.set_exact(smi_id::F_MCAST_FWD_RESULT_PRIORITY,
                                    static_cast<uint32_t>(i));
      status |= it->first.set_ternary(smi_id::F_MCAST_FWD_RESULT_MULTICAST_HIT,
                                      rules[i].mcast_hit,
                                      rules[i].mcast_hit_mask);
      status |= it->first.set_ternary(smi_id::F_MCAST_FWD_RESULT_LKP_IP_TYPE,
                                      rules[i].ip_type,
                                      rules[i].ip_type_mask);
      status |=
          it->first.set_ternary(smi_id::F_MCAST_FWD_RESULT_IPV4_MCAST_SNOOPING,
                                rules[i].ipv4_snooping_hit,
                                rules[i].ipv4_snooping_hit_mask);
      status |=
          it->first.set_ternary(smi_id::F_MCAST_FWD_RESULT_IPV6_MCAST_SNOOPING,
                                rules[i].ipv6_snooping_hit,
                                rules[i].ipv6_snooping_hit_mask);
      status |= it->first.set_ternary(
          smi_id::F_MCAST_FWD_RESULT_LOCAL_MD_MULTICAST_MODE,
          rules[i].mcast_mode,
          rules[i].mcast_mode_mask);
      status |= it->first.set_ternary(smi_id::F_MCAST_FWD_RESULT_RPF_CHECK,
                                      rules[i].rpf_check,
                                      rules[i].rpf_check_mask);
      it->second.init_action_data(rules[i].action_id);
      if (rules[i].action_id ==
          smi_id::A_MCAST_FWD_RESULT_SET_MULTICAST_BRIDGE) {
        status |= it->second.set_arg(
            smi_id::P_MCAST_FWD_RESULT_SET_MULTICAST_BRIDGE_MRPF,
            rules[i].mrpf);
      } else if (rules[i].action_id ==
                 smi_id::A_MCAST_FWD_RESULT_SET_MULTICAST_FLOOD) {
        status |= it->second.set_arg(
            smi_id::P_MCAST_FWD_RESULT_SET_MULTICAST_FLOOD_MRPF, rules[i].mrpf);
        status |= it->second.set_arg(
            smi_id::P_MCAST_FWD_RESULT_SET_MULTICAST_FLOOD_FLOOD,
            rules[i].flood);
      }
    }
  }
};

switch_status_t multicast_init() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  bf_status_t bf_status = BF_SUCCESS;
  uint32_t global_rid = 0xFFFF;
  const BfRtTable *table = NULL;
  std::unique_ptr<BfRtTableAttributes> table_attributes = nullptr;

  status |= switch_store::reg_create_trigs_after(SWITCH_OBJECT_TYPE_L2MC_BRIDGE,
                                                 &after_l2mc_bridge_create);
  status |= switch_store::reg_delete_trigs_before(
      SWITCH_OBJECT_TYPE_L2MC_BRIDGE, &before_l2mc_bridge_delete);
  status |= switch_store::reg_create_trigs_after(SWITCH_OBJECT_TYPE_L2MC_MEMBER,
                                                 &after_l2mc_member_create);
  status |= switch_store::reg_delete_trigs_before(
      SWITCH_OBJECT_TYPE_L2MC_MEMBER, &before_l2mc_member_delete);

  // REGISTER_OBJECT(mc_lag_membership, SWITCH_OBJECT_TYPE_MC_LAG_MEMBERSHIP);
  REGISTER_OBJECT(mc_port_prune, SWITCH_OBJECT_TYPE_MC_PORT_PRUNE);
  REGISTER_OBJECT(mc_mgid, SWITCH_OBJECT_TYPE_MC_MGID);
  REGISTER_OBJECT(mc_node_vlan, SWITCH_OBJECT_TYPE_MC_NODE_VLAN);
  REGISTER_OBJECT(mc_node_vlan_member, SWITCH_OBJECT_TYPE_MC_NODE_VLAN_MEMBER);
  REGISTER_OBJECT(l2mc_node, SWITCH_OBJECT_TYPE_L2MC_NODE);
  REGISTER_OBJECT(mc_node_tunnel, SWITCH_OBJECT_TYPE_MC_NODE_TUNNEL);
  REGISTER_OBJECT(ipmc_node, SWITCH_OBJECT_TYPE_IPMC_NODE);
  REGISTER_OBJECT(bd_flood, SWITCH_OBJECT_TYPE_BD_FLOOD);
  REGISTER_OBJECT(rid_table, SWITCH_OBJECT_TYPE_RID_TABLE);
  REGISTER_OBJECT(multicast_route_factory,
                  SWITCH_OBJECT_TYPE_MULTICAST_ROUTE_FACTORY);
  REGISTER_OBJECT(multicast_bridge_factory,
                  SWITCH_OBJECT_TYPE_MULTICAST_BRIDGE_FACTORY);
  REGISTER_OBJECT(mcast_fwd_result, SWITCH_OBJECT_TYPE_MCAST_FWD_RESULT);

  // configure global RID
  const BfRtInfo *bfrtinfo = get_bf_rt_info();
  if (!bfrtinfo) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "l2_clean: failed to get bf-runtime info");
    return SWITCH_STATUS_FAILURE;
  }

  bf_status = bfrtinfo->bfrtTableFromIdGet(smi_id::T_PRE_MGID, &table);
  if (!table) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: failed to get table for pre mgid",
               __func__,
               __LINE__);
    return SWITCH_STATUS_FAILURE;
  }
  bf_status = table->attributeAllocate(TableAttributesType::PRE_DEVICE_CONFIG,
                                       &table_attributes);
  if (!table_attributes) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: failed to get table attributes for pre mgid",
               __func__,
               __LINE__);
    return SWITCH_STATUS_FAILURE;
  }

  bf_status = table_attributes->preGlobalRidSet(global_rid);
  if (bf_status != BF_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: status:{} failed asymmetric set for pre mgid",
               __func__,
               __LINE__,
               bf_status);
    return status;
  }

  return status;
}

switch_status_t multicast_clean() { return SWITCH_STATUS_SUCCESS; }

}  // namespace smi
