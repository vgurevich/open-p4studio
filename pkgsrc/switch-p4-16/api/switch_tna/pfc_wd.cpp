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


#include <math.h>

#include <vector>
#include <set>

#include "switch_tna/utils.h"
#include "switch_tna/p4_16_types.h"

#define QID_INVALID 255

namespace smi {
using namespace smi::bf_rt;  // NOLINT(build/namespaces)
using ::smi::logging::switch_log;

class ingress_pfc_wd : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_INGRESS_PFC_WD;
  static const switch_attr_id_t status_attr_id =
      SWITCH_INGRESS_PFC_WD_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INGRESS_PFC_WD_ATTR_PARENT_HANDLE;

  switch_enum_t direction = {SWITCH_PFC_WD_ATTR_DIRECTION_NONE};

 public:
  ingress_pfc_wd(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_INGRESS_PFC_WD_ACL,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_object_id_t port_handle = {0};
    uint8_t qid = QID_INVALID;
    uint16_t dev_port = 0;

    status |=
        switch_store::v_get(parent, SWITCH_PFC_WD_ATTR_DIRECTION, direction);
    if (direction.enumdata != SWITCH_PFC_WD_ATTR_DIRECTION_INGRESS) {
      return;
    }

    status |= switch_store::v_get(
        parent, SWITCH_PFC_WD_ATTR_PORT_HANDLE, port_handle);
    if (port_handle.data == 0) {
      return;
    }

    status |= switch_store::v_get(parent, SWITCH_PFC_WD_ATTR_QID, qid);
    status |=
        switch_store::v_get(port_handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);

    device_tgt_set(compute_dev_target_for_table(
        dev_port, smi_id::T_INGRESS_PFC_WD_ACL, true));

    status |= match_key.set_exact(smi_id::F_INGRESS_PFC_WD_ACL_PORT, dev_port);
    status |= match_key.set_exact(smi_id::F_INGRESS_PFC_WD_ACL_QID, qid);
    action_entry.init_action_data(smi_id::A_INGRESS_PFC_WD_ACL_DENY);
  }

  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    (void)handle;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t pkts = 0;
    uint64_t bytes = 0;

    if (direction.enumdata != SWITCH_PFC_WD_ATTR_DIRECTION_INGRESS) {
      return SWITCH_STATUS_SUCCESS;
    }

    status = p4_object_match_action::data_get();
    if (status != SWITCH_STATUS_SUCCESS) return SWITCH_STATUS_INVALID_PARAMETER;

    action_entry.get_arg(smi_id::P_INGRESS_PFC_WD_ACL_STATS_COUNTER_SPEC_PKTS,
                         smi_id::A_INGRESS_PFC_WD_ACL_DENY,
                         &pkts);
    action_entry.get_arg(smi_id::P_INGRESS_PFC_WD_ACL_STATS_COUNTER_SPEC_BYTES,
                         smi_id::A_INGRESS_PFC_WD_ACL_DENY,
                         &bytes);

    switch_counter_t cntr;
    cntr.counter_id = SWITCH_PFC_WD_COUNTER_ID_PACKETS;
    cntr.count = pkts;
    cntrs[SWITCH_PFC_WD_COUNTER_ID_PACKETS] = cntr;
    cntr.counter_id = SWITCH_PFC_WD_COUNTER_ID_BYTES;
    cntr.count = bytes;
    cntrs[SWITCH_PFC_WD_COUNTER_ID_BYTES] = cntr;

    return status;
  }

  switch_status_t counters_set(const switch_object_id_t handle) {
    (void)handle;
    if (direction.enumdata == SWITCH_PFC_WD_ATTR_DIRECTION_INGRESS) {
      uint64_t value = 0;
      action_entry.set_arg(smi_id::P_INGRESS_PFC_WD_ACL_STATS_COUNTER_SPEC_PKTS,
                           value);
      action_entry.set_arg(
          smi_id::P_INGRESS_PFC_WD_ACL_STATS_COUNTER_SPEC_BYTES, value);
      return p4_object_match_action::data_set();
    }
    return SWITCH_STATUS_SUCCESS;
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    (void)handle;
    if (direction.enumdata == SWITCH_PFC_WD_ATTR_DIRECTION_INGRESS) {
      uint64_t value = 0;
      p4_object_match_action::data_get();

      for (auto id : cntr_ids) {
        switch (id) {
          case SWITCH_PFC_WD_COUNTER_ID_PACKETS:
            action_entry.set_arg(
                smi_id::P_INGRESS_PFC_WD_ACL_STATS_COUNTER_SPEC_PKTS, value);
            break;
          case SWITCH_PFC_WD_COUNTER_ID_BYTES:
            action_entry.set_arg(
                smi_id::P_INGRESS_PFC_WD_ACL_STATS_COUNTER_SPEC_BYTES, value);
            break;
          default:
            break;
        }
      }
      return p4_object_match_action::data_set();
    }
    return SWITCH_STATUS_SUCCESS;
  }

  switch_status_t counters_save(const switch_object_id_t parent) {
    (void)parent;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t ctr_bytes = 0, ctr_pkt = 0;

    if (get_auto_oid() == 0) return status;

    if (direction.enumdata != SWITCH_PFC_WD_ATTR_DIRECTION_INGRESS) {
      return SWITCH_STATUS_SUCCESS;
    }

    // MAU counter
    std::vector<uint64_t> ctr_list;
    status = p4_object_match_action::data_get();
    if (status == SWITCH_STATUS_SUCCESS) {
      ctr_bytes = 0, ctr_pkt = 0;
      action_entry.get_arg(
          smi_id::P_INGRESS_PFC_WD_ACL_STATS_COUNTER_SPEC_BYTES,
          smi_id::A_INGRESS_PFC_WD_ACL_DENY,
          &ctr_bytes);
      action_entry.get_arg(smi_id::P_INGRESS_PFC_WD_ACL_STATS_COUNTER_SPEC_PKTS,
                           smi_id::A_INGRESS_PFC_WD_ACL_DENY,
                           &ctr_pkt);
      ctr_list.push_back(static_cast<uint64_t>(ctr_pkt));
      ctr_list.push_back(static_cast<uint64_t>(ctr_bytes));

      attr_w ctr_attr_list(SWITCH_INGRESS_PFC_WD_ATTR_MAU_STATS_CACHE);
      ctr_attr_list.v_set(ctr_list);
      switch_store::attribute_set(get_auto_oid(), ctr_attr_list);
    }

    return status;
  }

  switch_status_t counters_restore(const switch_object_id_t parent) {
    (void)parent;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t ctr_bytes = 0, ctr_pkt = 0;

    if (get_auto_oid() == 0) return status;

    if (direction.enumdata != SWITCH_PFC_WD_ATTR_DIRECTION_INGRESS) {
      return SWITCH_STATUS_SUCCESS;
    }

    // MAU counter
    std::vector<uint64_t> ctr_list;
    status |= switch_store::v_get(
        get_auto_oid(), SWITCH_INGRESS_PFC_WD_ATTR_MAU_STATS_CACHE, ctr_list);
    if (ctr_list.empty()) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_NONE,
                 "{}.{}: No stat cache to restore mau stats, "
                 "ingress_pfc_wd cache list empty, status {}",
                 __func__,
                 __LINE__,
                 status);
      return SWITCH_STATUS_SUCCESS;
    }

    if (ctr_list.size() == SWITCH_ACL_ENTRY_COUNTER_ID_MAX) {
      ctr_pkt = ctr_list[SWITCH_ACL_ENTRY_COUNTER_ID_PKTS];
      ctr_bytes = ctr_list[SWITCH_ACL_ENTRY_COUNTER_ID_BYTES];
    }
    action_entry.set_arg(smi_id::P_INGRESS_PFC_WD_ACL_STATS_COUNTER_SPEC_BYTES,
                         ctr_bytes);
    action_entry.set_arg(smi_id::P_INGRESS_PFC_WD_ACL_STATS_COUNTER_SPEC_PKTS,
                         ctr_pkt);
    status = p4_object_match_action::data_set();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_NONE,
                 "{}.{}: Fail to restore mau stats cache,  "
                 "ingress_pfc_wd status {} ",
                 __func__,
                 __LINE__,
                 status);
    }

    return status;
  }
};

class egress_pfc_wd : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_EGRESS_PFC_WD;
  static const switch_attr_id_t status_attr_id =
      SWITCH_EGRESS_PFC_WD_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_EGRESS_PFC_WD_ATTR_PARENT_HANDLE;

  switch_enum_t direction = {SWITCH_PFC_WD_ATTR_DIRECTION_NONE};

 public:
  egress_pfc_wd(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_EGRESS_PFC_WD_ACL,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_object_id_t port_handle = {0};
    uint8_t qid = QID_INVALID;
    uint16_t dev_port = 0;

    status |=
        switch_store::v_get(parent, SWITCH_PFC_WD_ATTR_DIRECTION, direction);
    if (direction.enumdata != SWITCH_PFC_WD_ATTR_DIRECTION_EGRESS) {
      return;
    }

    status |= switch_store::v_get(
        parent, SWITCH_PFC_WD_ATTR_PORT_HANDLE, port_handle);
    if (port_handle.data == 0) {
      return;
    }

    status |= switch_store::v_get(parent, SWITCH_PFC_WD_ATTR_QID, qid);
    status |=
        switch_store::v_get(port_handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);

    device_tgt_set(compute_dev_target_for_table(
        dev_port, smi_id::T_EGRESS_PFC_WD_ACL, false));

    status |= match_key.set_exact(smi_id::F_EGRESS_PFC_WD_ACL_PORT, dev_port);
    status |= match_key.set_exact(smi_id::F_EGRESS_PFC_WD_ACL_QID, qid);
    action_entry.init_action_data(smi_id::A_EGRESS_PFC_WD_ACL_DENY);
  }

  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    (void)handle;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t pkts = 0;
    uint64_t bytes = 0;

    if (direction.enumdata != SWITCH_PFC_WD_ATTR_DIRECTION_EGRESS) {
      return SWITCH_STATUS_SUCCESS;
    }

    status = p4_object_match_action::data_get();
    if (status != SWITCH_STATUS_SUCCESS) return SWITCH_STATUS_INVALID_PARAMETER;

    action_entry.get_arg(smi_id::P_EGRESS_PFC_WD_ACL_STATS_COUNTER_SPEC_PKTS,
                         smi_id::A_EGRESS_PFC_WD_ACL_DENY,
                         &pkts);
    action_entry.get_arg(smi_id::P_EGRESS_PFC_WD_ACL_STATS_COUNTER_SPEC_BYTES,
                         smi_id::A_EGRESS_PFC_WD_ACL_DENY,
                         &bytes);

    switch_counter_t cntr;
    cntr.counter_id = SWITCH_PFC_WD_COUNTER_ID_PACKETS;
    cntr.count = pkts;
    cntrs[SWITCH_PFC_WD_COUNTER_ID_PACKETS] = cntr;
    cntr.counter_id = SWITCH_PFC_WD_COUNTER_ID_BYTES;
    cntr.count = bytes;
    cntrs[SWITCH_PFC_WD_COUNTER_ID_BYTES] = cntr;

    return status;
  }

  switch_status_t counters_set(const switch_object_id_t handle) {
    (void)handle;
    if (direction.enumdata == SWITCH_PFC_WD_ATTR_DIRECTION_EGRESS) {
      uint64_t value = 0;
      action_entry.set_arg(smi_id::P_EGRESS_PFC_WD_ACL_STATS_COUNTER_SPEC_PKTS,
                           value);
      action_entry.set_arg(smi_id::P_EGRESS_PFC_WD_ACL_STATS_COUNTER_SPEC_BYTES,
                           value);
      return p4_object_match_action::data_set();
    }
    return SWITCH_STATUS_SUCCESS;
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    (void)handle;
    if (direction.enumdata == SWITCH_PFC_WD_ATTR_DIRECTION_EGRESS) {
      uint64_t value = 0;
      p4_object_match_action::data_get();
      for (auto id : cntr_ids) {
        switch (id) {
          case SWITCH_PFC_WD_COUNTER_ID_PACKETS:
            action_entry.set_arg(
                smi_id::P_EGRESS_PFC_WD_ACL_STATS_COUNTER_SPEC_PKTS, value);
            break;
          case SWITCH_PFC_WD_COUNTER_ID_BYTES:
            action_entry.set_arg(
                smi_id::P_EGRESS_PFC_WD_ACL_STATS_COUNTER_SPEC_BYTES, value);
            break;
          default:
            break;
        }
      }
      return p4_object_match_action::data_set();
    }
    return SWITCH_STATUS_SUCCESS;
  }

  switch_status_t counters_save(const switch_object_id_t parent) {
    (void)parent;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t ctr_bytes = 0, ctr_pkt = 0;

    if (get_auto_oid() == 0) return status;

    if (direction.enumdata != SWITCH_PFC_WD_ATTR_DIRECTION_EGRESS) {
      return SWITCH_STATUS_SUCCESS;
    }

    // MAU counter
    std::vector<uint64_t> ctr_list;
    status = p4_object_match_action::data_get();
    if (status == SWITCH_STATUS_SUCCESS) {
      ctr_bytes = 0, ctr_pkt = 0;
      action_entry.get_arg(smi_id::P_EGRESS_PFC_WD_ACL_STATS_COUNTER_SPEC_BYTES,
                           smi_id::A_EGRESS_PFC_WD_ACL_DENY,
                           &ctr_bytes);
      action_entry.get_arg(smi_id::P_EGRESS_PFC_WD_ACL_STATS_COUNTER_SPEC_PKTS,
                           smi_id::A_EGRESS_PFC_WD_ACL_DENY,
                           &ctr_pkt);
      ctr_list.push_back(static_cast<uint64_t>(ctr_pkt));
      ctr_list.push_back(static_cast<uint64_t>(ctr_bytes));

      attr_w ctr_attr_list(SWITCH_EGRESS_PFC_WD_ATTR_MAU_STATS_CACHE);
      ctr_attr_list.v_set(ctr_list);
      switch_store::attribute_set(get_auto_oid(), ctr_attr_list);
    }

    return status;
  }

  switch_status_t counters_restore(const switch_object_id_t parent) {
    (void)parent;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t ctr_bytes = 0, ctr_pkt = 0;

    if (get_auto_oid() == 0) return status;

    if (direction.enumdata != SWITCH_PFC_WD_ATTR_DIRECTION_EGRESS) {
      return SWITCH_STATUS_SUCCESS;
    }

    // MAU counter
    std::vector<uint64_t> ctr_list;
    status = switch_store::v_get(
        get_auto_oid(), SWITCH_EGRESS_PFC_WD_ATTR_MAU_STATS_CACHE, ctr_list);
    if (ctr_list.empty()) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_NONE,
                 "{}.{}: No stat cache to restore mau stats, "
                 "egress_pfc_wd cache list empty, status {}",
                 __func__,
                 __LINE__,
                 status);
      return SWITCH_STATUS_SUCCESS;
    }

    if (ctr_list.size() == SWITCH_ACL_ENTRY_COUNTER_ID_MAX) {
      ctr_pkt = ctr_list[SWITCH_ACL_ENTRY_COUNTER_ID_PKTS];
      ctr_bytes = ctr_list[SWITCH_ACL_ENTRY_COUNTER_ID_BYTES];
    }
    action_entry.set_arg(smi_id::P_EGRESS_PFC_WD_ACL_STATS_COUNTER_SPEC_BYTES,
                         ctr_bytes);
    action_entry.set_arg(smi_id::P_EGRESS_PFC_WD_ACL_STATS_COUNTER_SPEC_PKTS,
                         ctr_pkt);
    status = p4_object_match_action::data_set();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_NONE,
                 "{}.{}: Fail to restore mau stats cache,  "
                 "egress_pfc_wd status {} ",
                 __func__,
                 __LINE__,
                 status);
    }

    return status;
  }
};

static switch_enum_t acl_dir_to_pfc_wd_convert(switch_enum_t acl_dir) {
  switch_enum_t pfc_wd_dir = {SWITCH_PFC_WD_ATTR_DIRECTION_NONE};

  switch (acl_dir.enumdata) {
    case SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS:
      pfc_wd_dir.enumdata = SWITCH_PFC_WD_ATTR_DIRECTION_INGRESS;
      break;
    case SWITCH_ACL_TABLE_ATTR_DIRECTION_EGRESS:
      pfc_wd_dir.enumdata = SWITCH_PFC_WD_ATTR_DIRECTION_EGRESS;
      break;
    default:
      break;
  }

  return pfc_wd_dir;
}

static switch_status_t pfc_wd_create(
    const switch_object_id_t port_handle,
    const switch_object_id_t acl_entry_handle) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::set<attr_w> pfc_wd_attrs;
  switch_object_id_t device_handle = {0};
  switch_object_id_t pfc_wd_handle = {0};
  switch_enum_t pfc_wd_dir = {SWITCH_PFC_WD_ATTR_DIRECTION_NONE};
  switch_enum_t acl_type = {SWITCH_ACL_TABLE_ATTR_TYPE_NONE};
  switch_enum_t acl_dir = {SWITCH_ACL_TABLE_ATTR_DIRECTION_NONE};
  uint8_t qid = QID_INVALID;

  if ((port_handle.data == 0) || (acl_entry_handle.data == 0)) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "{}:{}: PFC WD creation failed - invalid arguments passed",
               __func__,
               __LINE__);
    return SWITCH_STATUS_INVALID_PARAMETER;
  }

  status = get_acl_table_type_dir(acl_entry_handle, acl_type, acl_dir);
  if (status != SWITCH_STATUS_SUCCESS) {
    return status;
  }

  if (acl_type.enumdata != SWITCH_ACL_TABLE_ATTR_TYPE_PFCWD) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "{}:{}: PFC WD creation failed - the ACL table is not PFC WD",
               __func__,
               __LINE__);
    return SWITCH_STATUS_FAILURE;
  }

  pfc_wd_dir = acl_dir_to_pfc_wd_convert(acl_dir);
  if (pfc_wd_dir.enumdata == SWITCH_PFC_WD_ATTR_DIRECTION_NONE) {
    return status;
  }

  status |= switch_store::v_get(
      acl_entry_handle, SWITCH_ACL_ENTRY_ATTR_DEVICE, device_handle);
  status |= switch_store::v_get(
      acl_entry_handle, SWITCH_ACL_ENTRY_ATTR_PFC_WD_QID, qid);

  if (qid == QID_INVALID) {
    return status;
  }

  pfc_wd_attrs.insert(attr_w(SWITCH_PFC_WD_ATTR_DEVICE, device_handle));
  pfc_wd_attrs.insert(attr_w(SWITCH_PFC_WD_ATTR_DIRECTION, pfc_wd_dir));
  pfc_wd_attrs.insert(attr_w(SWITCH_PFC_WD_ATTR_PORT_HANDLE, port_handle));
  pfc_wd_attrs.insert(attr_w(SWITCH_PFC_WD_ATTR_QID, qid));

  status |= switch_store::object_create(
      SWITCH_OBJECT_TYPE_PFC_WD, pfc_wd_attrs, pfc_wd_handle);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "{}:{}: PFC WD creation failed, error: {}",
               __func__,
               __LINE__,
               status);
  }
  status |= switch_store::v_set(
      pfc_wd_handle, SWITCH_PFC_WD_ATTR_INTERNAL_OBJECT, true);

  return status;
}

/* API to be called when ACL table or group is bound to a port.
 *
 * If acl_handle PFC WD ACL table (or a group containing PFC ACL table) -
 * loop through all its ACL entries and create pfc_wd objects for each of them.
 */
switch_status_t pfc_wd_on_acl_to_port_bound(
    const switch_object_id_t port_handle, const switch_object_id_t acl_handle) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_type_t ot = 0;
  std::vector<switch_object_id_t> table_list;

  if ((port_handle.data == 0) || (acl_handle.data == 0)) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "{}:{}: PFC WD creation failed - invalid arguments passed",
               __func__,
               __LINE__);
    return SWITCH_STATUS_INVALID_PARAMETER;
  }

  ot = switch_store::object_type_query(acl_handle);
  if (ot == SWITCH_OBJECT_TYPE_ACL_GROUP) {
    std::vector<switch_object_id_t> group_members;

    switch_store::v_get(
        acl_handle, SWITCH_ACL_GROUP_ATTR_ACL_GROUP_MEMBERS, group_members);

    for (const auto member : group_members) {
      switch_object_id_t table_handle = {0};
      switch_enum_t table_type = {SWITCH_ACL_TABLE_ATTR_TYPE_NONE};

      if (member.data == 0) continue;

      switch_store::v_get(
          member, SWITCH_ACL_GROUP_MEMBER_ATTR_ACL_TABLE_HANDLE, table_handle);

      if (table_handle.data == 0) continue;

      switch_store::v_get(table_handle, SWITCH_ACL_TABLE_ATTR_TYPE, table_type);

      if (table_type.enumdata == SWITCH_ACL_TABLE_ATTR_TYPE_PFCWD) {
        table_list.push_back(table_handle);
      }
    }
  } else if (ot == SWITCH_OBJECT_TYPE_ACL_TABLE) {
    switch_enum_t table_type = {SWITCH_ACL_TABLE_ATTR_TYPE_NONE};

    switch_store::v_get(acl_handle, SWITCH_ACL_TABLE_ATTR_TYPE, table_type);

    if (table_type.enumdata == SWITCH_ACL_TABLE_ATTR_TYPE_PFCWD) {
      table_list.push_back(acl_handle);
    }
  }

  for (const auto table : table_list) {
    std::vector<switch_object_id_t> entry_handles;

    switch_store::v_get(
        table, SWITCH_ACL_TABLE_ATTR_ACL_ENTRY_HANDLES, entry_handles);

    for (auto entry : entry_handles) {
      status = pfc_wd_create(port_handle, entry);
      if (status != SWITCH_STATUS_SUCCESS) {
        return status;
      }
    }
  }

  return SWITCH_STATUS_SUCCESS;
}

/* API to be called when ACL table or group is unbound from a port.
 *
 * If acl_handle is PFC WD ACL table(or a group containing PFC ACL table) -
 * get the pfc_wd reference set
 * for this port handle and remove all the objects from this set.
 */
switch_status_t pfc_wd_on_acl_to_port_unbound(
    const switch_object_id_t port_handle, uint8_t direction) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::set<switch_object_id_t> pfc_wd_handles;

  status |= switch_store::referencing_set_get(
      port_handle, SWITCH_OBJECT_TYPE_PFC_WD, pfc_wd_handles);

  for (auto pfc_wd : pfc_wd_handles) {
    switch_enum_t pfc_wd_dir = {SWITCH_PFC_WD_ATTR_DIRECTION_NONE};
    status |=
        switch_store::v_get(pfc_wd, SWITCH_PFC_WD_ATTR_DIRECTION, pfc_wd_dir);

    if (pfc_wd_dir.enumdata == direction) {
      status |= switch_store::object_delete(pfc_wd);
    }
  }

  return SWITCH_STATUS_SUCCESS;
}

/* API to be called from after_acl_entry_create() and
 * after_acl_entry_update() triggers.
 *
 * In case this entry belongs to a PFC WD table -
 * get the set of port handles referenced to the table (or to the ACL group
 * this table belongs to) and create pfc_wd objects for all of them.
 */
switch_status_t pfc_wd_create_for_acl_entry(
    const switch_object_id_t acl_entry_handle) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t table_handle = {0};
  switch_enum_t acl_type = {SWITCH_ACL_TABLE_ATTR_TYPE_NONE};
  switch_enum_t acl_dir = {0};
  std::set<switch_object_id_t> table_port_handles;
  std::set<switch_object_id_t> group_member_handles;

  if (acl_entry_handle.data == 0) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "{}:{}: PFC WD creation failed - invalid arguments passed",
               __func__,
               __LINE__);
    return SWITCH_STATUS_INVALID_PARAMETER;
  }

  status |= switch_store::v_get(
      acl_entry_handle, SWITCH_ACL_ENTRY_ATTR_TABLE_HANDLE, table_handle);
  status |=
      switch_store::v_get(table_handle, SWITCH_ACL_TABLE_ATTR_TYPE, acl_type);

  if (acl_type.enumdata != SWITCH_ACL_TABLE_ATTR_TYPE_PFCWD) {
    // As this function will be called from after_acl_entry_create() trigger
    // we don't have to do anything for non PFC WD tables.
    return status;
  }

  status |= switch_store::v_get(
      table_handle, SWITCH_ACL_TABLE_ATTR_DIRECTION, acl_dir);

  if (acl_dir.enumdata == SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS) {
    std::vector<switch_object_id_t> port_list_handle;
    status |= switch_store::v_get(
        acl_entry_handle, SWITCH_ACL_ENTRY_ATTR_IN_PORTS, port_list_handle);

    for (auto port : port_list_handle) {
      status = pfc_wd_create(port, acl_entry_handle);
      if (status != SWITCH_STATUS_SUCCESS) {
        return status;
      }
    }

    return status;
  }

  switch_store::referencing_set_get(
      table_handle, SWITCH_OBJECT_TYPE_PORT, table_port_handles);

  // Create PFC WDs for all ports this table is bound to
  for (auto port : table_port_handles) {
    status = pfc_wd_create(port, acl_entry_handle);
    if (status != SWITCH_STATUS_SUCCESS) {
      return status;
    }
  }

  // The table also can be a member of some groups, so loop through all of them
  // and create PFC WDs for the ports these groups are bound to
  switch_store::referencing_set_get(
      table_handle, SWITCH_OBJECT_TYPE_ACL_GROUP_MEMBER, group_member_handles);

  for (auto group_member : group_member_handles) {
    switch_object_id_t group_handle = {0};
    std::set<switch_object_id_t> group_port_handles;

    status = switch_store::v_get(group_member,
                                 SWITCH_ACL_GROUP_MEMBER_ATTR_ACL_GROUP_HANDLE,
                                 group_handle);
    if (status != SWITCH_STATUS_SUCCESS) {
      return status;
    }

    switch_store::referencing_set_get(
        group_handle, SWITCH_OBJECT_TYPE_PORT, group_port_handles);

    // Create PFC WDs for all ports this group is bound to
    for (auto port : group_port_handles) {
      status = pfc_wd_create(port, acl_entry_handle);
      if (status != SWITCH_STATUS_SUCCESS) {
        return status;
      }
    }
  }

  return SWITCH_STATUS_SUCCESS;
}

/* API to be called from before_acl_entry_delete() and
 * before_acl_entry_update() triggers .
 *
 * In case this acl entry belongs to PFC WD ACL table -
 * get the PFC wd reference set for this acl entry and remove all the
 * objects from this set.
 */
switch_status_t pfc_wd_remove_by_acl_entry(
    const switch_object_id_t acl_entry_handle) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  uint8_t qid = QID_INVALID;
  switch_object_id_t device_handle = {0};
  switch_object_id_t table_handle = {0};
  switch_enum_t acl_type = {SWITCH_ACL_TABLE_ATTR_TYPE_NONE};
  switch_enum_t pfc_wd_dir = {SWITCH_PFC_WD_ATTR_DIRECTION_NONE};
  switch_enum_t acl_dir = {SWITCH_ACL_TABLE_ATTR_DIRECTION_NONE};
  std::set<switch_object_id_t> table_port_handles;
  std::set<switch_object_id_t> group_member_handles;

  if (acl_entry_handle.data == 0) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "{}:{}: PFC WD removal failed - invalid arguments passed",
               __func__,
               __LINE__);
    return SWITCH_STATUS_INVALID_PARAMETER;
  }

  status |= switch_store::v_get(
      acl_entry_handle, SWITCH_ACL_ENTRY_ATTR_PFC_WD_QID, qid);

  if (qid == QID_INVALID) {
    return status;
  }

  status |= switch_store::v_get(
      acl_entry_handle, SWITCH_ACL_ENTRY_ATTR_DEVICE, device_handle);
  status |= get_acl_table_type_dir(acl_entry_handle, acl_type, acl_dir);

  pfc_wd_dir = acl_dir_to_pfc_wd_convert(acl_dir);
  if ((acl_type.enumdata != SWITCH_ACL_TABLE_ATTR_TYPE_PFCWD) ||
      (pfc_wd_dir.enumdata == SWITCH_PFC_WD_ATTR_DIRECTION_NONE)) {
    return status;
  }

  if (acl_dir.enumdata == SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS) {
    switch_object_id_t pfc_wd_handle = {0};
    std::vector<switch_object_id_t> port_list_handle;
    std::set<attr_w> pfc_wd_attrs;

    status |= switch_store::v_get(
        acl_entry_handle, SWITCH_ACL_ENTRY_ATTR_IN_PORTS, port_list_handle);

    for (auto port : port_list_handle) {
      pfc_wd_attrs.clear();
      pfc_wd_attrs.insert(attr_w(SWITCH_PFC_WD_ATTR_DEVICE, device_handle));
      pfc_wd_attrs.insert(attr_w(SWITCH_PFC_WD_ATTR_DIRECTION, pfc_wd_dir));
      pfc_wd_attrs.insert(attr_w(SWITCH_PFC_WD_ATTR_PORT_HANDLE, port));
      pfc_wd_attrs.insert(attr_w(SWITCH_PFC_WD_ATTR_QID, qid));

      status = switch_store::object_id_get_wkey(
          SWITCH_OBJECT_TYPE_PFC_WD, pfc_wd_attrs, pfc_wd_handle);
      if (status == SWITCH_STATUS_SUCCESS) {
        status = switch_store::object_delete(pfc_wd_handle);
      }
    }

    return status;
  }

  status |= switch_store::v_get(
      acl_entry_handle, SWITCH_ACL_ENTRY_ATTR_TABLE_HANDLE, table_handle);

  switch_store::referencing_set_get(
      table_handle, SWITCH_OBJECT_TYPE_PORT, table_port_handles);

  // Remove PFC WDs for all ports this table is bound to
  for (auto port : table_port_handles) {
    switch_object_id_t pfc_wd_handle = {0};
    std::set<attr_w> pfc_wd_attrs;

    pfc_wd_attrs.insert(attr_w(SWITCH_PFC_WD_ATTR_DEVICE, device_handle));
    pfc_wd_attrs.insert(attr_w(SWITCH_PFC_WD_ATTR_DIRECTION, pfc_wd_dir));
    pfc_wd_attrs.insert(attr_w(SWITCH_PFC_WD_ATTR_PORT_HANDLE, port));
    pfc_wd_attrs.insert(attr_w(SWITCH_PFC_WD_ATTR_QID, qid));

    status = switch_store::object_id_get_wkey(
        SWITCH_OBJECT_TYPE_PFC_WD, pfc_wd_attrs, pfc_wd_handle);
    if (status == SWITCH_STATUS_SUCCESS) {
      status = switch_store::object_delete(pfc_wd_handle);
      if (status != SWITCH_STATUS_SUCCESS) {
        return status;
      }
    }
  }

  // The table also can be a member of some groups, so loop through all of them
  // and remove PFC WDs for the ports these groups are bound to
  switch_store::referencing_set_get(
      table_handle, SWITCH_OBJECT_TYPE_ACL_GROUP_MEMBER, group_member_handles);

  for (auto group_member : group_member_handles) {
    switch_object_id_t group_handle = {0};
    std::set<switch_object_id_t> group_port_handles;

    status = switch_store::v_get(group_member,
                                 SWITCH_ACL_GROUP_MEMBER_ATTR_ACL_GROUP_HANDLE,
                                 group_handle);
    if (status != SWITCH_STATUS_SUCCESS) {
      return status;
    }

    switch_store::referencing_set_get(
        group_handle, SWITCH_OBJECT_TYPE_PORT, group_port_handles);

    // Remove PFC WDs for all ports this group is bound to
    for (auto port : group_port_handles) {
      switch_object_id_t pfc_wd_handle = {0};
      std::set<attr_w> pfc_wd_attrs;

      pfc_wd_attrs.insert(attr_w(SWITCH_PFC_WD_ATTR_DEVICE, device_handle));
      pfc_wd_attrs.insert(attr_w(SWITCH_PFC_WD_ATTR_DIRECTION, pfc_wd_dir));
      pfc_wd_attrs.insert(attr_w(SWITCH_PFC_WD_ATTR_PORT_HANDLE, port));
      pfc_wd_attrs.insert(attr_w(SWITCH_PFC_WD_ATTR_QID, qid));

      status = switch_store::object_id_get_wkey(
          SWITCH_OBJECT_TYPE_PFC_WD, pfc_wd_attrs, pfc_wd_handle);
      if (status == SWITCH_STATUS_SUCCESS) {
        status = switch_store::object_delete(pfc_wd_handle);
        if (status != SWITCH_STATUS_SUCCESS) {
          return status;
        }
      }
    }
  }

  return SWITCH_STATUS_SUCCESS;
}

/* API to be called from after_acl_group_member_create() trigger.
 *
 * In case the table is PFC WD one - get the set of port handles
 * referenced to the ACL group and create pfc_wd objects for all of them
 * and for the all table entries
 */
switch_status_t pfc_wd_on_acl_group_member_create(
    const switch_object_id_t acl_group_member_handle) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t group_handle = {0};
  switch_object_id_t table_handle = {0};
  switch_enum_t acl_type = {SWITCH_ACL_TABLE_ATTR_TYPE_NONE};
  std::set<switch_object_id_t> group_port_handles;
  std::vector<switch_object_id_t> entry_handles;

  if (acl_group_member_handle.data == 0) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "{}:{}: PFC WD creation failed - invalid arguments passed",
               __func__,
               __LINE__);
    return SWITCH_STATUS_INVALID_PARAMETER;
  }

  status |= switch_store::v_get(acl_group_member_handle,
                                SWITCH_ACL_GROUP_MEMBER_ATTR_ACL_GROUP_HANDLE,
                                group_handle);
  status |= switch_store::v_get(acl_group_member_handle,
                                SWITCH_ACL_GROUP_MEMBER_ATTR_ACL_TABLE_HANDLE,
                                table_handle);
  status |=
      switch_store::v_get(table_handle, SWITCH_ACL_TABLE_ATTR_TYPE, acl_type);

  if (acl_type.enumdata != SWITCH_ACL_TABLE_ATTR_TYPE_PFCWD) {
    // As this function will be called from after_acl_group_member_create()
    // trigger
    // we don't have to do anything for non PFC WD tables.
    return status;
  }

  // Get list of table entries
  switch_store::v_get(
      table_handle, SWITCH_ACL_TABLE_ATTR_ACL_ENTRY_HANDLES, entry_handles);

  // Get list of ports this group is bound to
  switch_store::referencing_set_get(
      group_handle, SWITCH_OBJECT_TYPE_PORT, group_port_handles);

  // Now need to create PFC WD objects for all the port-entry pairs
  for (auto port : group_port_handles) {
    for (auto entry : entry_handles) {
      status = pfc_wd_create(port, entry);
      if (status != SWITCH_STATUS_SUCCESS) {
        return status;
      }
    }
  }

  return SWITCH_STATUS_SUCCESS;
}

/* API to be called from before_acl_group_member_delete() trigger.
 *
 * In case this ACL group member is PFC WD table - get the list
 * of port handles bound to this table group as well as the table entries
 * and remove all the PFC WD objects with these ports and entries.
 */
switch_status_t pfc_wd_on_acl_group_member_remove(
    const switch_object_id_t acl_group_member_handle) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t group_handle = {0};
  switch_object_id_t table_handle = {0};
  switch_object_id_t device_handle = {0};
  switch_enum_t table_type = {SWITCH_ACL_TABLE_ATTR_TYPE_NONE};
  switch_enum_t table_dir = {SWITCH_ACL_TABLE_ATTR_DIRECTION_NONE};
  switch_enum_t pfc_wd_dir = {SWITCH_PFC_WD_ATTR_DIRECTION_NONE};
  std::set<switch_object_id_t> group_port_handles;
  std::vector<switch_object_id_t> entry_handles;

  if (acl_group_member_handle.data == 0) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "{}:{}: PFC WD creation failed - invalid arguments passed",
               __func__,
               __LINE__);
    return SWITCH_STATUS_INVALID_PARAMETER;
  }

  status |= switch_store::v_get(acl_group_member_handle,
                                SWITCH_ACL_GROUP_MEMBER_ATTR_ACL_GROUP_HANDLE,
                                group_handle);
  status |= switch_store::v_get(acl_group_member_handle,
                                SWITCH_ACL_GROUP_MEMBER_ATTR_ACL_TABLE_HANDLE,
                                table_handle);
  status |=
      switch_store::v_get(table_handle, SWITCH_ACL_TABLE_ATTR_TYPE, table_type);
  status |= switch_store::v_get(
      table_handle, SWITCH_ACL_TABLE_ATTR_DIRECTION, table_dir);
  status |= switch_store::v_get(
      table_handle, SWITCH_ACL_TABLE_ATTR_DEVICE, device_handle);

  pfc_wd_dir = acl_dir_to_pfc_wd_convert(table_dir);

  if ((table_type.enumdata != SWITCH_ACL_TABLE_ATTR_TYPE_PFCWD) ||
      (pfc_wd_dir.enumdata == SWITCH_PFC_WD_ATTR_DIRECTION_NONE)) {
    // As this function will be called from before_acl_group_member_remove()
    // trigger
    // we don't have to do anything for non PFC WD tables.
    return status;
  }

  // Get list of table entries
  status |= switch_store::v_get(
      table_handle, SWITCH_ACL_TABLE_ATTR_ACL_ENTRY_HANDLES, entry_handles);

  // Get list of ports this group is bound to
  switch_store::referencing_set_get(
      group_handle, SWITCH_OBJECT_TYPE_PORT, group_port_handles);

  // Now need to delete PFC WD objects for all the port-entry pairs
  for (auto port : group_port_handles) {
    for (auto entry : entry_handles) {
      switch_object_id_t pfc_wd_handle = {0};
      uint8_t qid = QID_INVALID;
      std::set<attr_w> pfc_wd_attrs;

      status |=
          switch_store::v_get(entry, SWITCH_ACL_ENTRY_ATTR_PFC_WD_QID, qid);

      pfc_wd_attrs.insert(attr_w(SWITCH_PFC_WD_ATTR_DEVICE, device_handle));
      pfc_wd_attrs.insert(attr_w(SWITCH_PFC_WD_ATTR_DIRECTION, pfc_wd_dir));
      pfc_wd_attrs.insert(attr_w(SWITCH_PFC_WD_ATTR_PORT_HANDLE, port));
      pfc_wd_attrs.insert(attr_w(SWITCH_PFC_WD_ATTR_QID, qid));

      status = switch_store::object_id_get_wkey(
          SWITCH_OBJECT_TYPE_PFC_WD, pfc_wd_attrs, pfc_wd_handle);
      if (status == SWITCH_STATUS_SUCCESS) {
        status = switch_store::object_delete(pfc_wd_handle);
        if (status != SWITCH_STATUS_SUCCESS) {
          return status;
        }
      }
    }
  }

  return SWITCH_STATUS_SUCCESS;
}

switch_status_t pfc_wd_init() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  REGISTER_OBJECT(ingress_pfc_wd, SWITCH_OBJECT_TYPE_INGRESS_PFC_WD);
  REGISTER_OBJECT(egress_pfc_wd, SWITCH_OBJECT_TYPE_EGRESS_PFC_WD);

  return status;
}

switch_status_t pfc_wd_clean() { return SWITCH_STATUS_SUCCESS; }

}  // namespace smi
