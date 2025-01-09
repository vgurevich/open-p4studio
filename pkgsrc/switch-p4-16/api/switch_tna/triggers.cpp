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


#include <arpa/inet.h>

#include <set>
#include <vector>
#include <unordered_map>
#include <memory>

#include "switch_tna/utils.h"
#include "../../s3/switch_lpm_int.h"

namespace smi {
using ::smi::logging::switch_log;

/**
 * It is becoming tedious to keep adding these triggers for all internal
 * objects. Easier to create/detete these internal objects automatically
 * in the switch_store
 * TODO
 */

switch_status_t before_rpf_member_create(const switch_object_type_t object_type,
                                         std::set<attr_w> &attrs) {
  if (object_type != SWITCH_OBJECT_TYPE_RPF_MEMBER)
    return SWITCH_STATUS_FAILURE;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t rpf_group = {0}, handle = {0}, oid = {0};
  uint64_t pim_mode = 0;
  size_t cnt = 0;

  // Get rpf_group handle
  auto it = attrs.find(SWITCH_RPF_MEMBER_ATTR_RPF_GROUP_HANDLE);
  CHECK_RET(it == attrs.end(), SWITCH_STATUS_FAILURE);

  status = it->v_get(rpf_group);
  if (status != SWITCH_STATUS_SUCCESS) return status;

  // Get vlan or rif handle
  auto it2 = attrs.find(SWITCH_RPF_MEMBER_ATTR_HANDLE);
  CHECK_RET(it2 == attrs.end(), SWITCH_STATUS_FAILURE);

  status = it2->v_get(handle);
  if (status != SWITCH_STATUS_SUCCESS) return status;

  status =
      switch_store::v_get(rpf_group, SWITCH_RPF_GROUP_ATTR_PIM_MODE, pim_mode);
  status =
      switch_store::list_len(rpf_group, SWITCH_RPF_GROUP_ATTR_RPF_MEMBERS, cnt);
  if (pim_mode == SWITCH_IPMC_ROUTE_ATTR_PIM_MODE_SM) {
    if (cnt == 1) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 object_type,
                 "rpf member create failed "
                 "one rpf member allowed per pim sm {}",
                 switch_error_to_string(status));
      return SWITCH_STATUS_FAILURE;
    }
  }
  for (uint32_t i = 0; i < cnt; i++) {
    status = switch_store::list_v_get(
        rpf_group, SWITCH_RPF_GROUP_ATTR_RPF_MEMBERS, i, oid);
    if (handle == oid) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 object_type,
                 "rpf member create failed "
                 "handle already exists in group {}",
                 switch_error_to_string(status));
      return SWITCH_STATUS_FAILURE;
    }
  }
  return status;
}

switch_status_t before_acl_group_member_delete(
    const switch_object_id_t acl_group_member) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t table_handle = {0};
  uint64_t acl_type = SWITCH_ACL_TABLE_ATTR_TYPE_NONE;
  uint64_t dir = SWITCH_ACL_TABLE_ATTR_DIRECTION_NONE;

  status |= switch_store::v_get(acl_group_member,
                                SWITCH_ACL_GROUP_MEMBER_ATTR_ACL_TABLE_HANDLE,
                                table_handle);

  switch_object_id_t device_handle = {0};
  status |= switch_store::v_get(
      table_handle, SWITCH_ACL_TABLE_ATTR_DEVICE, device_handle);
  bool use_ingress_port_group = false;
  status |=
      switch_store::v_get(device_handle,
                          SWITCH_DEVICE_ATTR_INGRESS_ACL_PORT_GROUP_ENABLE,
                          use_ingress_port_group);
  switch_enum_t enum_type;
  status |=
      switch_store::v_get(table_handle, SWITCH_ACL_TABLE_ATTR_TYPE, enum_type);
  acl_type = enum_type.enumdata;

  status = switch_store::v_get(
      table_handle, SWITCH_ACL_TABLE_ATTR_DIRECTION, enum_type);
  dir = enum_type.enumdata;

  if (dir == SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS ||
      use_ingress_port_group == false) {
    status = compute_acl_group_label(acl_group_member, true, false);
    if (status != SWITCH_STATUS_SUCCESS) {
      return status;
    }
  } else {
    // Compute port_group only for ingress V4/V6/Mirror ACLs
    if (use_ingress_port_group &&
        (dir == SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS) &&
        ((acl_type == SWITCH_ACL_TABLE_ATTR_TYPE_IPV4) ||
         (acl_type == SWITCH_ACL_TABLE_ATTR_TYPE_MAC) ||
         (acl_type == SWITCH_ACL_TABLE_ATTR_TYPE_IPV6) ||
         (acl_type == SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR) ||
         (acl_type == SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR) ||
         (acl_type == SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR))) {
      status = update_acl_port_group_index(acl_group_member, acl_type, dir);
      if (status != SWITCH_STATUS_SUCCESS) {
        return status;
      }
    }
  }

  return pfc_wd_on_acl_group_member_remove(acl_group_member);
}

switch_status_t after_acl_group_member_create(
    const switch_object_id_t object_id, const std::set<attr_w> &attrs) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t acl_group = {0}, acl_table = {0};
  bool acl_bind_point_attach_flag = false;

  (void)attrs;
  status = compute_acl_group_label(object_id, false, false);
  if (status != SWITCH_STATUS_SUCCESS) {
    return status;
  }

  status |= switch_store::v_get(
      object_id, SWITCH_ACL_GROUP_MEMBER_ATTR_ACL_TABLE_HANDLE, acl_table);
  status |= switch_store::v_get(
      object_id, SWITCH_ACL_GROUP_MEMBER_ATTR_ACL_GROUP_HANDLE, acl_group);

  status |= switch_store::v_get(acl_group,
                                SWITCH_ACL_GROUP_ATTR_BIND_POINT_ATTACH,
                                acl_bind_point_attach_flag);

  if (acl_bind_point_attach_flag) {
    status |= set_bind_point_attach_flag(acl_table, true);
  }

  return pfc_wd_on_acl_group_member_create(object_id);
}

switch_status_t before_acl_table_delete(const switch_object_id_t object_id) {
  return release_acl_table_label(object_id);
}

switch_status_t after_acl_table_create(const switch_object_id_t object_id,
                                       const std::set<attr_w> &attrs) {
  (void)attrs;
  return compute_acl_table_label(object_id);
}

switch_status_t before_acl_table_update(const switch_object_id_t object_id,
                                        const attr_w &attr) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_enum_t attr_enum = {.enumdata = SWITCH_ACL_TABLE_ATTR_TYPE_NONE};
  uint64_t attr_acl_table_type = SWITCH_ACL_TABLE_ATTR_TYPE_NONE;
  switch_attr_id_t attr_id = attr.id_get();

  if (attr_id != SWITCH_ACL_TABLE_ATTR_TYPE) {
    return SWITCH_STATUS_SUCCESS;
  }

  status = attr.v_get(attr_enum);
  if (status == SWITCH_STATUS_ITEM_NOT_FOUND) {
    return SWITCH_STATUS_SUCCESS;
  }
  attr_acl_table_type = attr_enum.enumdata;

  // if the attribute acl table type is none, no-op
  if (attr_acl_table_type == SWITCH_ACL_TABLE_ATTR_TYPE_NONE) {
    return SWITCH_STATUS_SUCCESS;
  }

  uint64_t acl_table_type = SWITCH_ACL_TABLE_ATTR_TYPE_NONE;
  switch_enum_t enum_type;

  status =
      switch_store::v_get(object_id, SWITCH_ACL_TABLE_ATTR_TYPE, enum_type);
  if (status == SWITCH_STATUS_ITEM_NOT_FOUND) {
    return SWITCH_STATUS_SUCCESS;
  }
  acl_table_type = enum_type.enumdata;

  // if the current acl table type is none, no-op
  if (acl_table_type == SWITCH_ACL_TABLE_ATTR_TYPE_NONE) {
    return SWITCH_STATUS_SUCCESS;
  }

  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_ACL_TABLE,
             "Before Acl table {} type update, new type {} current type {}",
             object_id,
             attr_acl_table_type,
             acl_table_type);

  // if the new table type or current acl table type is equal, no-op
  if (attr_acl_table_type == acl_table_type) {
    return SWITCH_STATUS_SUCCESS;
  }

  // first release the existing acl label
  release_acl_table_label(object_id);
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t after_acl_table_update(const switch_object_id_t object_id,
                                       const attr_w &attr) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_enum_t attr_enum = {.enumdata = SWITCH_ACL_TABLE_ATTR_TYPE_NONE};
  uint64_t attr_acl_table_type = SWITCH_ACL_TABLE_ATTR_TYPE_NONE;
  switch_attr_id_t attr_id = attr.id_get();

  if (attr_id != SWITCH_ACL_TABLE_ATTR_TYPE) {
    return SWITCH_STATUS_SUCCESS;
  }

  status = attr.v_get(attr_enum);
  if (status == SWITCH_STATUS_ITEM_NOT_FOUND) {
    return SWITCH_STATUS_SUCCESS;
  }
  attr_acl_table_type = attr_enum.enumdata;

  // if the attribute acl table type is none, no-op
  if (attr_acl_table_type == SWITCH_ACL_TABLE_ATTR_TYPE_NONE) {
    return SWITCH_STATUS_SUCCESS;
  }

  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_ACL_TABLE,
             "After Acl table {} type update  - new type {}",
             object_id,
             attr_acl_table_type);

  // now compute the acl label using the new acl table type
  compute_acl_table_label(object_id);
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t before_acl_entry_delete(const switch_object_id_t object_id) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  release_inout_ports_label(object_id);
  status = pfc_wd_remove_by_acl_entry(object_id);
  if (status != SWITCH_STATUS_SUCCESS) {
    return status;
  }

  return etraps_remove(object_id);
}

switch_status_t before_acl_entry_create(const switch_object_type_t object_type,
                                        std::set<attr_w> &attrs) {
  (void)object_type;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  status = compute_inout_ports_label(attrs);
  return status;
}

switch_status_t after_acl_entry_create(const switch_object_id_t object_id,
                                       const std::set<attr_w> &attrs) {
  (void)attrs;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t table_handle = {0};
  uint64_t acl_type = SWITCH_ACL_TABLE_ATTR_TYPE_NONE;
  uint64_t dir = SWITCH_ACL_TABLE_ATTR_DIRECTION_NONE;

  status |= switch_store::v_get(
      object_id, SWITCH_ACL_ENTRY_ATTR_TABLE_HANDLE, table_handle);

  switch_object_id_t device_handle = {0};
  status |= switch_store::v_get(
      table_handle, SWITCH_ACL_TABLE_ATTR_DEVICE, device_handle);
  bool use_ingress_port_group = false;
  status |=
      switch_store::v_get(device_handle,
                          SWITCH_DEVICE_ATTR_INGRESS_ACL_PORT_GROUP_ENABLE,
                          use_ingress_port_group);

  // Port group is supported only when device.use_ingress_port_group is set
  if (use_ingress_port_group == true) {
    switch_enum_t enum_type;
    status |= switch_store::v_get(
        table_handle, SWITCH_ACL_TABLE_ATTR_TYPE, enum_type);
    acl_type = enum_type.enumdata;

    status = switch_store::v_get(
        table_handle, SWITCH_ACL_TABLE_ATTR_DIRECTION, enum_type);
    dir = enum_type.enumdata;

    // Compute port_group only for ingress V4/V6/Mirror ACLs
    if (use_ingress_port_group &&
        (dir == SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS) &&
        ((acl_type == SWITCH_ACL_TABLE_ATTR_TYPE_IPV4) ||
         (acl_type == SWITCH_ACL_TABLE_ATTR_TYPE_MAC) ||
         (acl_type == SWITCH_ACL_TABLE_ATTR_TYPE_IPV6) ||
         (acl_type == SWITCH_ACL_TABLE_ATTR_TYPE_IP_MIRROR) ||
         (acl_type == SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_MIRROR) ||
         (acl_type == SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_MIRROR))) {
      status = acl_entry_compute_port_group(object_id);
      if (status != SWITCH_STATUS_SUCCESS) {
        return status;
      }
    }
  }
  update_inout_ports_label(object_id);
  status = pfc_wd_create_for_acl_entry(object_id);
  if (status != SWITCH_STATUS_SUCCESS) {
    return status;
  }

  return etraps_create(object_id);
}

switch_status_t before_acl_entry_update(const switch_object_id_t object_id,
                                        const attr_w &attr) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (attr.id_get() == SWITCH_ACL_ENTRY_ATTR_PFC_WD_QID ||
      attr.id_get() == SWITCH_ACL_ENTRY_ATTR_IN_PORTS) {
    status = pfc_wd_remove_by_acl_entry(object_id);
  } else if (attr.id_get() == SWITCH_ACL_ENTRY_ATTR_ACTION_METER_HANDLE) {
    status = etrap_meters_remove(object_id);
  }

  return status;
}

switch_status_t after_acl_entry_update(const switch_object_id_t object_id,
                                       const attr_w &attr) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  switch (attr.id_get()) {
    case SWITCH_ACL_ENTRY_ATTR_PFC_WD_QID:
    case SWITCH_ACL_ENTRY_ATTR_IN_PORTS:
      status = pfc_wd_create_for_acl_entry(object_id);
      break;
    case SWITCH_ACL_ENTRY_ATTR_ACTION_METER_HANDLE:
      status = etrap_meters_create(object_id);
      break;
    case SWITCH_ACL_ENTRY_ATTR_ACTION_SET_TC:
      status = etraps_update(object_id, attr);
      break;
    default:
      break;
  }

  return status;
}

switch_status_t before_rif_create(const switch_object_type_t object_type,
                                  std::set<attr_w> &attrs) {
  CHECK_RET(object_type != SWITCH_OBJECT_TYPE_RIF, SWITCH_STATUS_FAILURE);
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_enum_t e = {0};
  switch_object_type_t ot = 0;
  switch_object_id_t oid = {}, rif_handle = {};

  auto it = attrs.find(SWITCH_RIF_ATTR_TYPE);
  CHECK_RET(it == attrs.end(), SWITCH_STATUS_FAILURE);
  status = it->v_get(e);

  if (e.enumdata == SWITCH_RIF_ATTR_TYPE_LOOPBACK) {
    return status;
  } else if (e.enumdata == SWITCH_RIF_ATTR_TYPE_PORT ||
             e.enumdata == SWITCH_RIF_ATTR_TYPE_SUB_PORT) {
    it = attrs.find(SWITCH_RIF_ATTR_PORT_HANDLE);
    if (it == attrs.end()) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 object_type,
                 SMI_CREATE_OPERATION,
                 "{}.{}: Port attribute not found for RIF type {}",
                 __func__,
                 __LINE__,
                 e.enumdata == SWITCH_RIF_ATTR_TYPE_PORT ? "port" : "subport");
      return SWITCH_STATUS_INVALID_PARAMETER;
    }
    status = it->v_get(oid);
    if (oid.data == 0) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 object_type,
                 SMI_CREATE_OPERATION,
                 "{}.{}: Invalid Port/LAG handle for RIF type {}",
                 __func__,
                 __LINE__,
                 e.enumdata == SWITCH_RIF_ATTR_TYPE_PORT ? "port" : "subport");
      return SWITCH_STATUS_INVALID_PARAMETER;
    }

    // check if rif type PORT is already present
    if (e.enumdata == SWITCH_RIF_ATTR_TYPE_PORT) {
      ot = switch_store::object_type_query(oid);
      if (ot == SWITCH_OBJECT_TYPE_PORT) {
        status |=
            switch_store::v_get(oid, SWITCH_PORT_ATTR_RIF_HANDLE, rif_handle);
      } else if (ot == SWITCH_OBJECT_TYPE_LAG) {
        status |=
            switch_store::v_get(oid, SWITCH_LAG_ATTR_RIF_HANDLE, rif_handle);
      }
    }

    // make sure outer_vlan_id is non-zero for sub_port type RIF
    if (e.enumdata == SWITCH_RIF_ATTR_TYPE_SUB_PORT) {
      uint16_t outer_vlan_id = 0;
      it = attrs.find(SWITCH_RIF_ATTR_OUTER_VLAN_ID);
      if (it == attrs.end()) {
        switch_log(
            SWITCH_API_LEVEL_ERROR,
            object_type,
            SMI_CREATE_OPERATION,
            "{}.{}: outer_vlan_id attribute not found for RIF type sub_port",
            __func__,
            __LINE__);
        return SWITCH_STATUS_INVALID_PARAMETER;
      }
      status = it->v_get(outer_vlan_id);
      if (outer_vlan_id == 0) {
        switch_log(
            SWITCH_API_LEVEL_ERROR,
            object_type,
            SMI_CREATE_OPERATION,
            "{}.{}: Invalid outer_vlan_id {} attribute for RIF type sub_port",
            __func__,
            __LINE__,
            outer_vlan_id);
        return SWITCH_STATUS_INVALID_PARAMETER;
      }
    }
  }

  return status;
}

switch_status_t after_rif_create(const switch_object_id_t object_id,
                                 const std::set<attr_w> &attrs) {
  (void)attrs;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_enum_t rif_type = {};
  switch_object_id_t vlan_handle = {0}, port_lag_handle = {0};
  switch_object_type_t ot = 0;

  status = switch_store::v_get(object_id, SWITCH_RIF_ATTR_TYPE, rif_type);
  if (status != SWITCH_STATUS_SUCCESS) return status;

  // Return for loopback RIF
  if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_LOOPBACK) return status;

  if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_VLAN) {
    status |= switch_store::v_get(
        object_id, SWITCH_RIF_ATTR_VLAN_HANDLE, vlan_handle);

    if (!switch_store::smiContext::context().in_warm_init()) {
      status |= switch_store::list_v_push(
          vlan_handle, SWITCH_VLAN_ATTR_RIF_HANDLES, object_id);
    }
    if (status != SWITCH_STATUS_SUCCESS) return status;
    status |=
        switch_store::v_set(vlan_handle, SWITCH_VLAN_ATTR_IS_ROUTABLE, true);
  } else if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_PORT) {
    // no need for RIF reference in port for multiple RIF scenario
    if (feature::is_feature_set(SWITCH_FEATURE_MULTIPLE_RIFS_PER_PORT))
      return status;
    bool is_virtual = false;
    status |=
        switch_store::v_get(object_id, SWITCH_RIF_ATTR_IS_VIRTUAL, is_virtual);
    if (is_virtual) return status;
    status |= switch_store::v_get(
        object_id, SWITCH_RIF_ATTR_PORT_HANDLE, port_lag_handle);
    ot = switch_store::object_type_query(port_lag_handle);
    if (ot == SWITCH_OBJECT_TYPE_PORT) {
      status |= switch_store::v_set(
          port_lag_handle, SWITCH_PORT_ATTR_RIF_HANDLE, object_id);
    } else if (ot == SWITCH_OBJECT_TYPE_LAG) {
      status |= switch_store::v_set(
          port_lag_handle, SWITCH_LAG_ATTR_RIF_HANDLE, object_id);
    }
    if (status != SWITCH_STATUS_SUCCESS) return status;
  }
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t before_rif_delete(const switch_object_id_t object_id) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t empty_handle = {0};
  switch_enum_t rif_type = {};
  switch_object_id_t vlan_handle = {0}, port_lag_handle = {0};
  switch_object_type_t ot = 0;

  status = switch_store::v_get(object_id, SWITCH_RIF_ATTR_TYPE, rif_type);
  if (status != SWITCH_STATUS_SUCCESS) return status;

  // Return for loopback RIF
  if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_LOOPBACK) return status;

  if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_VLAN) {
    status |= switch_store::v_get(
        object_id, SWITCH_RIF_ATTR_VLAN_HANDLE, vlan_handle);
    status |= switch_store::list_v_del(
        vlan_handle, SWITCH_VLAN_ATTR_RIF_HANDLES, object_id);
    if (status != SWITCH_STATUS_SUCCESS) return status;
    std::vector<switch_object_id_t> rif_handles;
    status |= switch_store::v_get(
        vlan_handle, SWITCH_VLAN_ATTR_RIF_HANDLES, rif_handles);
    if (rif_handles.size() == 0) {
      status |=
          switch_store::v_set(vlan_handle, SWITCH_VLAN_ATTR_IS_ROUTABLE, false);
    } else {
      status |=
          switch_store::v_set(vlan_handle, SWITCH_VLAN_ATTR_IS_ROUTABLE, true);
    }
  } else if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_PORT) {
    // no need for RIF reference in port for multiple RIF scenario
    if (feature::is_feature_set(SWITCH_FEATURE_MULTIPLE_RIFS_PER_PORT))
      return status;
    status |= switch_store::v_get(
        object_id, SWITCH_RIF_ATTR_PORT_HANDLE, port_lag_handle);
    ot = switch_store::object_type_query(port_lag_handle);
    if (ot == SWITCH_OBJECT_TYPE_PORT) {
      bool is_virtual = false;
      status |= switch_store::v_get(
          object_id, SWITCH_RIF_ATTR_IS_VIRTUAL, is_virtual);
      if (is_virtual) return status;
      status |= switch_store::v_set(
          port_lag_handle, SWITCH_PORT_ATTR_RIF_HANDLE, empty_handle);
    } else if (ot == SWITCH_OBJECT_TYPE_LAG) {
      status |= switch_store::v_set(
          port_lag_handle, SWITCH_LAG_ATTR_RIF_HANDLE, empty_handle);
    }
    if (status != SWITCH_STATUS_SUCCESS) return status;
  }
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t after_rif_update(const switch_object_id_t object_id,
                                 const attr_w &attr) {
  (void)attr;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_enum_t rif_type = {};
  switch_object_id_t vlan_handle = {0};

  status = switch_store::v_get(object_id, SWITCH_RIF_ATTR_TYPE, rif_type);
  if (status != SWITCH_STATUS_SUCCESS) return status;

  if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_VLAN) {
    status |= switch_store::v_get(
        object_id, SWITCH_RIF_ATTR_VLAN_HANDLE, vlan_handle);
    status |=
        switch_store::v_set(vlan_handle, SWITCH_VLAN_ATTR_IS_ROUTABLE, true);
    if (status != SWITCH_STATUS_SUCCESS) return status;
  }

  return status;
}

switch_status_t after_vrf_create(const switch_object_id_t object_id,
                                 const std::set<attr_w> &attrs) {
  (void)attrs;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  uint16_t max_vrf;
  uint64_t oid = switch_store::handle_to_id(object_id);
  switch_object_id_t device_handle = {0};

  if (object_id.data == 0) return SWITCH_STATUS_INVALID_PARAMETER;

  status |=
      switch_store::v_get(object_id, SWITCH_VRF_ATTR_DEVICE, device_handle);

  switch_store::v_get(device_handle, SWITCH_DEVICE_ATTR_MAX_VRF, max_vrf);
  if (oid >= max_vrf) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ROUTE,
               "{}:{}: Fail on create vrf, due to max number of VRFs {}",
               __func__,
               __LINE__,
               max_vrf);
    return SWITCH_STATUS_INSUFFICIENT_RESOURCES;
  }

  switch_lpm_trie_t *trie = NULL;
  if (SWITCH_CONTEXT.ipv4_tries[object_id.data] == NULL) {
    status |= switch_lpm_trie_create(sizeof(switch_ip4_t), true, &trie);
    SWITCH_CONTEXT.ipv4_tries[object_id.data] = trie;
  }

  if (SWITCH_CONTEXT.ipv6_tries[object_id.data] == NULL) {
    status |= switch_lpm_trie_create(sizeof(switch_ip6_t), true, &trie);
    SWITCH_CONTEXT.ipv6_tries[object_id.data] = trie;
  }

  return status;
}

switch_status_t before_vrf_delete(const switch_object_id_t object_id) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (object_id.data == 0) return SWITCH_STATUS_INVALID_PARAMETER;

  if (SWITCH_CONTEXT.ipv4_tries[object_id.data] != NULL) {
    status = switch_lpm_trie_destroy(SWITCH_CONTEXT.ipv4_tries[object_id.data]);
    SWITCH_CONTEXT.ipv4_tries.erase(object_id.data);
  }
  if (SWITCH_CONTEXT.ipv6_tries[object_id.data] != NULL) {
    status = switch_lpm_trie_destroy(SWITCH_CONTEXT.ipv6_tries[object_id.data]);
    SWITCH_CONTEXT.ipv6_tries.erase(object_id.data);
  }

  return status;
}

switch_status_t after_route_create(const switch_object_id_t object_id,
                                   const std::set<attr_w> &attrs) {
  (void)attrs;
  (void)object_id;

  reevaluate_tunnel_nexthops();

  return SWITCH_STATUS_SUCCESS;
}

switch_status_t before_route_delete(const switch_object_id_t object_id) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t vrf_handle = {0};
  switch_ip_prefix_t prefix = {};
  switch_ip4_t v4addr;
  switch_lpm_trie_t *trie = NULL;
  uint8_t *prefix_ptr = NULL;
  bool is_nbr_installed = false;

  status |= switch_store::v_get(
      object_id, SWITCH_ROUTE_ATTR_IS_NBR_SOURCED, is_nbr_installed);

  status |=
      switch_store::v_get(object_id, SWITCH_ROUTE_ATTR_VRF_HANDLE, vrf_handle);
  if (vrf_handle.data == 0) return SWITCH_STATUS_INVALID_PARAMETER;

  status |= switch_store::v_get(object_id, SWITCH_ROUTE_ATTR_IP_PREFIX, prefix);
  if (status != SWITCH_STATUS_SUCCESS) return status;

  if (prefix.addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4) {
    trie = SWITCH_CONTEXT.ipv4_tries[vrf_handle.data];
    v4addr = htonl(prefix.addr.ip4);
    prefix_ptr = reinterpret_cast<uint8_t *>(&v4addr);
    CHECK_RET(prefix.len > sizeof(prefix.addr.ip4) * 8, SWITCH_STATUS_FAILURE);
  } else if (prefix.addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6) {
    trie = SWITCH_CONTEXT.ipv6_tries[vrf_handle.data];
    prefix_ptr = reinterpret_cast<uint8_t *>(prefix.addr.ip6);
    CHECK_RET(prefix.len > sizeof(prefix.addr.ip6) * 8, SWITCH_STATUS_FAILURE);
  } else {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ROUTE,
               "{}:{}: Invalid IP address family {}",
               __func__,
               __LINE__,
               prefix.addr.addr_family);
    return SWITCH_STATUS_INVALID_PARAMETER;
  }

  if (!is_nbr_installed) {
    status = switch_lpm_trie_delete(trie, prefix_ptr, prefix.len);
    CHECK_RET(status != SWITCH_STATUS_SUCCESS, SWITCH_STATUS_FAILURE);
  }

  reevaluate_tunnel_nexthops();

  return status;
}

switch_status_t after_route_update(const switch_object_id_t object_id,
                                   const attr_w &attr) {
  (void)object_id;
  if (attr.id_get() == SWITCH_ROUTE_ATTR_NEXTHOP_HANDLE) {
    reevaluate_tunnel_nexthops();
  }
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t after_route_delete(const switch_object_type_t object_type,
                                   const std::set<attr_w> &attrs) {
  (void)object_type;
  (void)attrs;
  reevaluate_tunnel_nexthops();
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t after_vlan_member_delete(const switch_object_type_t object_type,
                                         const std::set<attr_w> &attrs) {
  (void)object_type;
  switch_object_id_t vlan_handle = {0};
  switch_status_t status;
  auto it = attrs.find(SWITCH_VLAN_MEMBER_ATTR_VLAN_HANDLE);
  CHECK_RET(it == attrs.end(), SWITCH_STATUS_FAILURE);

  status = it->v_get(vlan_handle);
  if (status != SWITCH_STATUS_SUCCESS) return status;

  if (feature::is_feature_set(SWITCH_FEATURE_L2_VXLAN)) {
    std::unique_ptr<object> mc_mgid_obj(factory::get_instance().create(
        SWITCH_OBJECT_TYPE_MC_MGID, vlan_handle, status));
    if (mc_mgid_obj != nullptr) {
      status = mc_mgid_obj->create_update();
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_MC_MGID,
                   "{}: mc_mgid_obj update failed status={} for vlan {}",
                   __func__,
                   status,
                   vlan_handle);
        return status;
      }
    }
  }
  std::unique_ptr<object> mc_node_vlan_obj(factory::get_instance().create(
      SWITCH_OBJECT_TYPE_MC_NODE_VLAN, vlan_handle, status));
  if (mc_node_vlan_obj != nullptr) {
    status = mc_node_vlan_obj->create_update();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_MC_NODE_VLAN,
                 "{}: mc_node_vlan_obj update failed status={} for vlan {}",
                 __func__,
                 status,
                 vlan_handle);
      return status;
    }
  }
  if (feature::is_feature_set(SWITCH_FEATURE_MULTICAST)) {
    switch_object_id_t ipmc_node_parent = {};
    const auto &ipmc_nodes = switch_store::get_object_references(
        vlan_handle, SWITCH_OBJECT_TYPE_IPMC_NODE);

    for (auto ipmc_node : ipmc_nodes) {
      status |= switch_store::v_get(
          ipmc_node.oid, SWITCH_IPMC_NODE_ATTR_PARENT_HANDLE, ipmc_node_parent);

      std::unique_ptr<object> ipmc_node_obj(factory::get_instance().create(
          SWITCH_OBJECT_TYPE_IPMC_NODE, ipmc_node_parent, status));
      if (ipmc_node_obj != nullptr) {
        status |= ipmc_node_obj->create_update();
        if (status != SWITCH_STATUS_SUCCESS) {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_IPMC_NODE,
                     "{}: ipmc_node_obj update failed status={} for vlan {}",
                     __func__,
                     status,
                     vlan_handle);
        }
      }
    }
  }
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t triggers_init() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  status |= switch_store::reg_create_trigs_after(
      SWITCH_OBJECT_TYPE_ACL_GROUP_MEMBER, &after_acl_group_member_create);
  status |= switch_store::reg_delete_trigs_before(
      SWITCH_OBJECT_TYPE_ACL_GROUP_MEMBER, &before_acl_group_member_delete);
  status |= switch_store::reg_create_trigs_after(SWITCH_OBJECT_TYPE_ACL_TABLE,
                                                 &after_acl_table_create);
  status |= switch_store::reg_delete_trigs_before(SWITCH_OBJECT_TYPE_ACL_TABLE,
                                                  &before_acl_table_delete);
  status |= switch_store::reg_update_trigs_before(SWITCH_OBJECT_TYPE_ACL_TABLE,
                                                  &before_acl_table_update);
  status |= switch_store::reg_update_trigs_after(SWITCH_OBJECT_TYPE_ACL_TABLE,
                                                 &after_acl_table_update);
  status |= switch_store::reg_create_trigs_before(SWITCH_OBJECT_TYPE_ACL_ENTRY,
                                                  &before_acl_entry_create);
  status |= switch_store::reg_create_trigs_after(SWITCH_OBJECT_TYPE_ACL_ENTRY,
                                                 &after_acl_entry_create);
  status |= switch_store::reg_delete_trigs_before(SWITCH_OBJECT_TYPE_ACL_ENTRY,
                                                  &before_acl_entry_delete);
  status |= switch_store::reg_update_trigs_before(SWITCH_OBJECT_TYPE_ACL_ENTRY,
                                                  &before_acl_entry_update);
  status |= switch_store::reg_update_trigs_after(SWITCH_OBJECT_TYPE_ACL_ENTRY,
                                                 &after_acl_entry_update);
  status |= switch_store::reg_update_trigs_after(SWITCH_OBJECT_TYPE_ROUTE,
                                                 &after_route_update);
  status |= switch_store::reg_create_trigs_before(SWITCH_OBJECT_TYPE_RIF,
                                                  &before_rif_create);
  status |= switch_store::reg_create_trigs_after(SWITCH_OBJECT_TYPE_RIF,
                                                 &after_rif_create);
  status |= switch_store::reg_delete_trigs_before(SWITCH_OBJECT_TYPE_RIF,
                                                  &before_rif_delete);
  status |= switch_store::reg_create_trigs_after(SWITCH_OBJECT_TYPE_VRF,
                                                 &after_vrf_create);
  status |= switch_store::reg_delete_trigs_before(SWITCH_OBJECT_TYPE_VRF,
                                                  &before_vrf_delete);
  status |= switch_store::reg_create_trigs_after(SWITCH_OBJECT_TYPE_ROUTE,
                                                 &after_route_create);
  status |= switch_store::reg_delete_trigs_before(SWITCH_OBJECT_TYPE_ROUTE,
                                                  &before_route_delete);
  status |= switch_store::reg_delete_trigs_after(SWITCH_OBJECT_TYPE_ROUTE,
                                                 &after_route_delete);
  status |= switch_store::reg_update_trigs_after(SWITCH_OBJECT_TYPE_RIF,
                                                 &after_rif_update);
  status |= switch_store::reg_delete_trigs_after(SWITCH_OBJECT_TYPE_VLAN_MEMBER,
                                                 &after_vlan_member_delete);

  return status;
}

switch_status_t triggers_clean() { return SWITCH_STATUS_SUCCESS; }

} /* namespace smi */
