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
#include <memory>
#include <vector>
#include <set>
#include <utility>
#include "common/hostif.h"
#include "switch_tna/utils.h"
#include "switch_tna/p4_16_types.h"

// Nat aging timers in msecs
#define NAT_AGING_QUERY_INTERVAL 10000  // 10 seconds, arbitrary
#define NAT_AGING_MAX_TIME 43200000     // 5 days, SONIC timout max
#define NAT_AGING_MIN_TIME 0            // 0 seconds, arbitrary

namespace smi {
using namespace smi::bf_rt;  // NOLINT(build/namespaces)
using namespace ::bfrt;      // NOLINT(build/namespaces)
using ::smi::logging::switch_log;
template <class ACTION_OBJ>
switch_status_t nat_counters_get(ACTION_OBJ &action_ent,
                                 bf_rt_field_id_t action_id,
                                 bf_rt_field_id_t packet_id,
                                 bf_rt_field_id_t byte_id,
                                 std::vector<switch_counter_t> &counters) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  uint64_t pkts = 0, bytes = 0;
  action_ent.get_arg(packet_id, action_id, &pkts);
  action_ent.get_arg(byte_id, action_id, &bytes);

  switch_counter_t cntr_pkts;
  cntr_pkts.counter_id = SWITCH_ACL_ENTRY_COUNTER_ID_PKTS;
  cntr_pkts.count = pkts;
  counters[0] = cntr_pkts;
  switch_counter_t cntr_bytes;
  cntr_bytes.counter_id = SWITCH_ACL_ENTRY_COUNTER_ID_BYTES;
  cntr_bytes.count = bytes;
  counters[1] = cntr_bytes;
  return status;
}

template <class ACTION_OBJ>
switch_status_t nat_counters_set(ACTION_OBJ &action_ent,
                                 bf_rt_field_id_t counter_id) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  uint64_t value = 0;
  action_ent.set_arg(counter_id, value);
  return status;
}

class dnapt_index : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_DNAPT_INDEX;
  static const switch_attr_id_t status_attr_id = SWITCH_DNAPT_INDEX_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DNAPT_INDEX_ATTR_PARENT_HANDLE;

 public:
  dnapt_index(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_INGRESS_NAT_DNAPT_INDEX,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_enum_t e = {0};
    switch_nat_entry_attr_type type;
    uint32_t aging_time = 0;

    if (smi_id::T_INGRESS_NAT_DNAPT_INDEX == 0) {
      return;
    }
    status |= switch_store::v_get(parent, SWITCH_NAT_ENTRY_ATTR_TYPE, e);
    type = static_cast<switch_nat_entry_attr_type>(e.enumdata);
    if (type != SWITCH_NAT_ENTRY_ATTR_TYPE_DESTINATION_NAPT) return;
    uint16_t pat_index = 0;
    status |=
        switch_store::v_get(parent, SWITCH_NAT_ENTRY_ATTR_NAT_INDEX, pat_index);

    match_key.set_exact(smi_id::F_INGRESS_NAT_DNAPT_INDEX_DIP,
                        parent,
                        SWITCH_NAT_ENTRY_ATTR_DST_IP_KEY);
    match_key.set_exact(smi_id::F_INGRESS_NAT_DNAPT_INDEX_DPORT,
                        parent,
                        SWITCH_NAT_ENTRY_ATTR_L4_DST_PORT_KEY);
    match_key.set_exact(smi_id::F_INGRESS_NAT_DNAPT_INDEX_PROTOCOL,
                        parent,
                        SWITCH_NAT_ENTRY_ATTR_IP_PROTO_KEY);

    action_entry.init_action_data(smi_id::A_INGRESS_NAT_DNAPT_SET_INDEX);
    action_entry.set_arg(smi_id::P_INGRESS_NAT_DNAPT_SET_INDEX,
                         parent,
                         SWITCH_NAT_ENTRY_ATTR_NAT_INDEX);
    status |= switch_store::v_get(
        parent, SWITCH_NAT_ENTRY_ATTR_AGING_TIME, aging_time);
    action_entry.set_arg(smi_id::D_INGRESS_NAT_DNAPT_INDEX_TTL,
                         aging_time * 1000);
  }
};

class dest_napt : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DESTINATION_NAPT;
  static const switch_attr_id_t status_attr_id =
      SWITCH_DESTINATION_NAPT_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DESTINATION_NAPT_ATTR_PARENT_HANDLE;

 public:
  dest_napt(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_INGRESS_NAT_DEST_NAPT,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_enum_t e = {0};
    switch_nat_entry_attr_type type;

    status |= switch_store::v_get(parent, SWITCH_NAT_ENTRY_ATTR_TYPE, e);
    type = static_cast<switch_nat_entry_attr_type>(e.enumdata);
    if (type != SWITCH_NAT_ENTRY_ATTR_TYPE_DESTINATION_NAPT) return;
    uint16_t pat_index = 0;
    status |=
        switch_store::v_get(parent, SWITCH_NAT_ENTRY_ATTR_NAT_INDEX, pat_index);

    // tofino2 does not use a pat index
    if (feature::is_feature_set(SWITCH_FEATURE_NAT_NAPT_INDEX_INDIRECTION)) {
      match_key.set_exact(smi_id::F_INGRESS_NAT_DNAPT_INDEX,
                          parent,
                          SWITCH_NAT_ENTRY_ATTR_NAT_INDEX);
    } else {
      match_key.set_exact(smi_id::F_INGRESS_NAT_DEST_NAPT_DIP,
                          parent,
                          SWITCH_NAT_ENTRY_ATTR_DST_IP_KEY);
      match_key.set_exact(smi_id::F_INGRESS_NAT_DEST_NAPT_DPORT,
                          parent,
                          SWITCH_NAT_ENTRY_ATTR_L4_DST_PORT_KEY);
      match_key.set_exact(smi_id::F_INGRESS_NAT_DEST_NAPT_PROTOCOL,
                          parent,
                          SWITCH_NAT_ENTRY_ATTR_IP_PROTO_KEY);
    }

    action_entry.init_action_data(smi_id::A_INGRESS_NAT_DEST_NAPT_REWRITE);
    action_entry.set_arg(smi_id::P_DEST_NAPT_REWRITE_DIP,
                         parent,
                         SWITCH_NAT_ENTRY_ATTR_NAT_DST_IP);
    action_entry.set_arg(smi_id::P_DEST_NAPT_REWRITE_DPORT,
                         parent,
                         SWITCH_NAT_ENTRY_ATTR_NAT_L4_DST_PORT);

    if (!feature::is_feature_set(SWITCH_FEATURE_NAT_NAPT_INDEX_INDIRECTION)) {
      uint32_t aging_time = 0;
      status |= switch_store::v_get(
          parent, SWITCH_NAT_ENTRY_ATTR_AGING_TIME, aging_time);
      action_entry.set_arg(smi_id::D_INGRESS_NAT_DEST_NAPT_TTL,
                           aging_time * 1000);
    }
  }
  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &counters) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t nat_type = {0};
    status |= switch_store::v_get(handle, SWITCH_NAT_ENTRY_ATTR_TYPE, nat_type);
    if (nat_type.enumdata != SWITCH_NAT_ENTRY_ATTR_TYPE_DESTINATION_NAPT)
      return status;

    status = p4_object_match_action::data_get();
    if (status == SWITCH_STATUS_SUCCESS)
      return smi::nat_counters_get<_ActionEntry>(
          action_entry,
          smi_id::A_INGRESS_NAT_DEST_NAPT_REWRITE,
          smi_id::D_INGRESS_NAT_DEST_NAPT_COUNTER_SPEC_PKTS,
          smi_id::D_INGRESS_NAT_DEST_NAPT_COUNTER_SPEC_BYTES,
          counters);
    return status;
  }
  switch_status_t counters_set(const switch_object_id_t handle) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t nat_type = {0};
    uint64_t value = 0;
    status |= switch_store::v_get(handle, SWITCH_NAT_ENTRY_ATTR_TYPE, nat_type);
    if (nat_type.enumdata != SWITCH_NAT_ENTRY_ATTR_TYPE_DESTINATION_NAPT)
      return status;

    action_entry.set_arg(smi_id::D_INGRESS_NAT_DEST_NAPT_COUNTER_SPEC_PKTS,
                         value);
    action_entry.set_arg(smi_id::D_INGRESS_NAT_DEST_NAPT_COUNTER_SPEC_BYTES,
                         value);

    return p4_object_match_action::data_set();
  }
  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t nat_type = {0};
    status |= switch_store::v_get(handle, SWITCH_NAT_ENTRY_ATTR_TYPE, nat_type);
    if (nat_type.enumdata != SWITCH_NAT_ENTRY_ATTR_TYPE_DESTINATION_NAPT)
      return status;

    if (cntr_ids.size() == 0) return status;
    for (auto cntr_id : cntr_ids) {
      if (cntr_id == SWITCH_NAT_ENTRY_COUNTER_ID_PKTS) {
        nat_counters_set<_ActionEntry>(
            action_entry, smi_id::D_INGRESS_NAT_DEST_NAPT_COUNTER_SPEC_PKTS);
      } else if (cntr_id == SWITCH_NAT_ENTRY_COUNTER_ID_BYTES) {
        nat_counters_set<_ActionEntry>(
            action_entry, smi_id::D_INGRESS_NAT_DEST_NAPT_COUNTER_SPEC_BYTES);
      } else {
        return status;
      }
      return p4_object_match_action::data_set();
    }
    return status;
  }
};

class dest_nat : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DESTINATION_NAT;
  static const switch_attr_id_t status_attr_id =
      SWITCH_DESTINATION_NAT_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DESTINATION_NAT_ATTR_PARENT_HANDLE;

 public:
  dest_nat(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_INGRESS_NAT_DEST_NAT,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_enum_t e = {0};
    switch_nat_entry_attr_type type;
    uint32_t aging_time = 0;

    status |= switch_store::v_get(parent, SWITCH_NAT_ENTRY_ATTR_TYPE, e);
    type = static_cast<switch_nat_entry_attr_type>(e.enumdata);

    if (type != SWITCH_NAT_ENTRY_ATTR_TYPE_DESTINATION_NAT) return;

    match_key.set_exact(smi_id::F_INGRESS_NAT_DEST_NAT_DIP,
                        parent,
                        SWITCH_NAT_ENTRY_ATTR_DST_IP_KEY);
    action_entry.init_action_data(smi_id::A_INGRESS_NAT_DEST_NAT_REWRITE);
    action_entry.set_arg(smi_id::P_DEST_NAT_REWRITE_DIP,
                         parent,
                         SWITCH_NAT_ENTRY_ATTR_NAT_DST_IP);
    status |= switch_store::v_get(
        parent, SWITCH_NAT_ENTRY_ATTR_AGING_TIME, aging_time);
    action_entry.set_arg(smi_id::D_INGRESS_NAT_DEST_NAT_TTL, aging_time * 1000);
  }
  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &counters) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t nat_type = {0};
    status |= switch_store::v_get(handle, SWITCH_NAT_ENTRY_ATTR_TYPE, nat_type);
    if (nat_type.enumdata != SWITCH_NAT_ENTRY_ATTR_TYPE_DESTINATION_NAT)
      return status;

    status = p4_object_match_action::data_get();
    if (status == SWITCH_STATUS_SUCCESS)
      return smi::nat_counters_get<_ActionEntry>(
          action_entry,
          smi_id::A_INGRESS_NAT_DEST_NAT_REWRITE,
          smi_id::D_INGRESS_NAT_DEST_NAT_COUNTER_SPEC_PKTS,
          smi_id::D_INGRESS_NAT_DEST_NAT_COUNTER_SPEC_BYTES,
          counters);
    return status;
  }
  switch_status_t counters_set(const switch_object_id_t handle) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t nat_type = {0};
    uint64_t value = 0;
    status |= switch_store::v_get(handle, SWITCH_NAT_ENTRY_ATTR_TYPE, nat_type);
    if (nat_type.enumdata != SWITCH_NAT_ENTRY_ATTR_TYPE_DESTINATION_NAT)
      return status;

    action_entry.set_arg(smi_id::D_INGRESS_NAT_DEST_NAT_COUNTER_SPEC_PKTS,
                         value);
    action_entry.set_arg(smi_id::D_INGRESS_NAT_DEST_NAT_COUNTER_SPEC_BYTES,
                         value);

    return p4_object_match_action::data_set();
  }
  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t nat_type = {0};
    status |= switch_store::v_get(handle, SWITCH_NAT_ENTRY_ATTR_TYPE, nat_type);
    if (nat_type.enumdata != SWITCH_NAT_ENTRY_ATTR_TYPE_DESTINATION_NAT)
      return status;

    if (cntr_ids.size() == 0) return status;

    for (auto cntr_id : cntr_ids) {
      if (cntr_id == SWITCH_NAT_ENTRY_COUNTER_ID_PKTS) {
        nat_counters_set<_ActionEntry>(
            action_entry, smi_id::D_INGRESS_NAT_DEST_NAT_COUNTER_SPEC_PKTS);
      } else if (cntr_id == SWITCH_NAT_ENTRY_COUNTER_ID_BYTES) {
        nat_counters_set<_ActionEntry>(
            action_entry, smi_id::D_INGRESS_NAT_DEST_NAT_COUNTER_SPEC_BYTES);
      } else {
        return status;
      }
      return p4_object_match_action::data_set();
    }
    return status;
  }
};

class dest_nat_pool : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DESTINATION_NAT_POOL;
  static const switch_attr_id_t status_attr_id =
      SWITCH_DESTINATION_NAT_POOL_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DESTINATION_NAT_POOL_ATTR_PARENT_HANDLE;

 public:
  dest_nat_pool(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_INGRESS_NAT_DEST_NAT_POOL,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_enum_t e = {0};
    switch_nat_entry_attr_type type;

    status |= switch_store::v_get(parent, SWITCH_NAT_ENTRY_ATTR_TYPE, e);
    type = static_cast<switch_nat_entry_attr_type>(e.enumdata);

    if (type != SWITCH_NAT_ENTRY_ATTR_TYPE_DESTINATION_NAT_POOL) return;

    match_key.set_exact(smi_id::F_INGRESS_NAT_DEST_NAT_POOL_DIP,
                        parent,
                        SWITCH_NAT_ENTRY_ATTR_DST_IP_KEY);
    action_entry.init_action_data(smi_id::A_INGRESS_NAT_DEST_NAT_POOL_HIT);
  }
  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &counters) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t nat_type = {0};
    status |= switch_store::v_get(handle, SWITCH_NAT_ENTRY_ATTR_TYPE, nat_type);
    if (nat_type.enumdata != SWITCH_NAT_ENTRY_ATTR_TYPE_DESTINATION_NAT_POOL)
      return status;

    status = p4_object_match_action::data_get();
    if (status == SWITCH_STATUS_SUCCESS)
      return smi::nat_counters_get<_ActionEntry>(
          action_entry,
          smi_id::A_INGRESS_NAT_DEST_NAT_POOL_HIT,
          smi_id::D_INGRESS_NAT_DEST_NAT_POOL_COUNTER_SPEC_PKTS,
          smi_id::D_INGRESS_NAT_DEST_NAT_POOL_COUNTER_SPEC_BYTES,
          counters);
    return status;
  }
  switch_status_t counters_set(const switch_object_id_t handle) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t nat_type = {0};
    uint64_t value = 0;
    status |= switch_store::v_get(handle, SWITCH_NAT_ENTRY_ATTR_TYPE, nat_type);
    if (nat_type.enumdata != SWITCH_NAT_ENTRY_ATTR_TYPE_DESTINATION_NAT_POOL)
      return status;

    action_entry.set_arg(smi_id::D_INGRESS_NAT_DEST_NAT_POOL_COUNTER_SPEC_PKTS,
                         value);
    action_entry.set_arg(smi_id::D_INGRESS_NAT_DEST_NAT_POOL_COUNTER_SPEC_BYTES,
                         value);

    return p4_object_match_action::data_set();
  }
  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t nat_type = {0};
    status |= switch_store::v_get(handle, SWITCH_NAT_ENTRY_ATTR_TYPE, nat_type);
    if (nat_type.enumdata != SWITCH_NAT_ENTRY_ATTR_TYPE_DESTINATION_NAT_POOL)
      return status;

    if (cntr_ids.size() == 0) return status;

    for (auto cntr_id : cntr_ids) {
      if (cntr_id == SWITCH_NAT_ENTRY_COUNTER_ID_PKTS) {
        nat_counters_set<_ActionEntry>(
            action_entry,
            smi_id::D_INGRESS_NAT_DEST_NAT_POOL_COUNTER_SPEC_PKTS);
      } else if (cntr_id == SWITCH_NAT_ENTRY_COUNTER_ID_BYTES) {
        nat_counters_set<_ActionEntry>(
            action_entry,
            smi_id::D_INGRESS_NAT_DEST_NAT_POOL_COUNTER_SPEC_BYTES);
      } else {
        return status;
      }
      return p4_object_match_action::data_set();
    }
    return status;
  }
};

class flow_napt : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_FLOW_NAPT;
  static const switch_attr_id_t status_attr_id = SWITCH_FLOW_NAPT_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_FLOW_NAPT_ATTR_PARENT_HANDLE;

 public:
  flow_napt(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_INGRESS_NAT_FLOW_NAPT,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_enum_t e = {0};
    switch_nat_entry_attr_type type;

    status |= switch_store::v_get(parent, SWITCH_NAT_ENTRY_ATTR_TYPE, e);
    type = static_cast<switch_nat_entry_attr_type>(e.enumdata);

    if (type != SWITCH_NAT_ENTRY_ATTR_TYPE_DOUBLE_NAPT) return;
    match_key.set_exact(smi_id::F_INGRESS_NAT_FLOW_NAPT_DIP,
                        parent,
                        SWITCH_NAT_ENTRY_ATTR_DST_IP_KEY);
    match_key.set_exact(smi_id::F_INGRESS_NAT_FLOW_NAPT_DPORT,
                        parent,
                        SWITCH_NAT_ENTRY_ATTR_L4_DST_PORT_KEY);
    match_key.set_exact(smi_id::F_INGRESS_NAT_FLOW_NAPT_SIP,
                        parent,
                        SWITCH_NAT_ENTRY_ATTR_SRC_IP_KEY);
    match_key.set_exact(smi_id::F_INGRESS_NAT_FLOW_NAPT_SPORT,
                        parent,
                        SWITCH_NAT_ENTRY_ATTR_L4_SRC_PORT_KEY);
    match_key.set_exact(smi_id::F_INGRESS_NAT_FLOW_NAPT_PROTOCOL,
                        parent,
                        SWITCH_NAT_ENTRY_ATTR_IP_PROTO_KEY);

    action_entry.init_action_data(smi_id::A_INGRESS_NAT_FLOW_NAPT_REWRITE);
    action_entry.set_arg(smi_id::P_FLOW_NAPT_REWRITE_DIP,
                         parent,
                         SWITCH_NAT_ENTRY_ATTR_NAT_DST_IP);
    action_entry.set_arg(smi_id::P_FLOW_NAPT_REWRITE_DPORT,
                         parent,
                         SWITCH_NAT_ENTRY_ATTR_NAT_L4_DST_PORT);
    action_entry.set_arg(smi_id::P_FLOW_NAPT_REWRITE_SIP,
                         parent,
                         SWITCH_NAT_ENTRY_ATTR_NAT_SRC_IP);
    action_entry.set_arg(smi_id::P_FLOW_NAPT_REWRITE_SPORT,
                         parent,
                         SWITCH_NAT_ENTRY_ATTR_NAT_L4_SRC_PORT);
  }
  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &counters) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t nat_type = {0};
    status |= switch_store::v_get(handle, SWITCH_NAT_ENTRY_ATTR_TYPE, nat_type);
    if (nat_type.enumdata != SWITCH_NAT_ENTRY_ATTR_TYPE_DOUBLE_NAPT)
      return status;

    status = p4_object_match_action::data_get();
    if (status == SWITCH_STATUS_SUCCESS)
      return smi::nat_counters_get<_ActionEntry>(
          action_entry,
          smi_id::A_INGRESS_NAT_FLOW_NAPT_REWRITE,
          smi_id::D_INGRESS_NAT_FLOW_NAPT_COUNTER_SPEC_PKTS,
          smi_id::D_INGRESS_NAT_FLOW_NAPT_COUNTER_SPEC_BYTES,
          counters);
    return status;
  }
  switch_status_t counters_set(const switch_object_id_t handle) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t nat_type = {0};
    uint64_t value = 0;
    status |= switch_store::v_get(handle, SWITCH_NAT_ENTRY_ATTR_TYPE, nat_type);
    if (nat_type.enumdata != SWITCH_NAT_ENTRY_ATTR_TYPE_DOUBLE_NAPT)
      return status;

    action_entry.set_arg(smi_id::D_INGRESS_NAT_FLOW_NAPT_COUNTER_SPEC_PKTS,
                         value);
    action_entry.set_arg(smi_id::D_INGRESS_NAT_FLOW_NAPT_COUNTER_SPEC_BYTES,
                         value);

    return p4_object_match_action::data_set();
  }
  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t nat_type = {0};
    status |= switch_store::v_get(handle, SWITCH_NAT_ENTRY_ATTR_TYPE, nat_type);
    if (nat_type.enumdata != SWITCH_NAT_ENTRY_ATTR_TYPE_DOUBLE_NAPT)
      return status;

    if (cntr_ids.size() == 0) return status;

    for (auto cntr_id : cntr_ids) {
      if (cntr_id == SWITCH_NAT_ENTRY_COUNTER_ID_PKTS) {
        nat_counters_set<_ActionEntry>(
            action_entry, smi_id::D_INGRESS_NAT_FLOW_NAPT_COUNTER_SPEC_PKTS);
      } else if (cntr_id == SWITCH_NAT_ENTRY_COUNTER_ID_BYTES) {
        nat_counters_set<_ActionEntry>(
            action_entry, smi_id::D_INGRESS_NAT_FLOW_NAPT_COUNTER_SPEC_BYTES);
      } else {
        return status;
      }
      return p4_object_match_action::data_set();
    }
    return status;
  }
};

class flow_nat : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_FLOW_NAT;
  static const switch_attr_id_t status_attr_id = SWITCH_FLOW_NAT_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_FLOW_NAT_ATTR_PARENT_HANDLE;

 public:
  flow_nat(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_INGRESS_NAT_FLOW_NAT,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_enum_t e = {0};
    switch_nat_entry_attr_type type;

    status |= switch_store::v_get(parent, SWITCH_NAT_ENTRY_ATTR_TYPE, e);
    type = static_cast<switch_nat_entry_attr_type>(e.enumdata);

    if (type != SWITCH_NAT_ENTRY_ATTR_TYPE_DOUBLE_NAT) return;
    match_key.set_exact(smi_id::F_INGRESS_NAT_FLOW_NAT_DIP,
                        parent,
                        SWITCH_NAT_ENTRY_ATTR_DST_IP_KEY);
    match_key.set_exact(smi_id::F_INGRESS_NAT_FLOW_NAT_SIP,
                        parent,
                        SWITCH_NAT_ENTRY_ATTR_SRC_IP_KEY);

    action_entry.init_action_data(smi_id::A_INGRESS_NAT_FLOW_NAT_REWRITE);
    action_entry.set_arg(smi_id::P_FLOW_NAT_REWRITE_DIP,
                         parent,
                         SWITCH_NAT_ENTRY_ATTR_NAT_DST_IP);
    action_entry.set_arg(smi_id::P_FLOW_NAT_REWRITE_SIP,
                         parent,
                         SWITCH_NAT_ENTRY_ATTR_NAT_SRC_IP);
  }
  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &counters) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t nat_type = {0};
    status |= switch_store::v_get(handle, SWITCH_NAT_ENTRY_ATTR_TYPE, nat_type);
    if (nat_type.enumdata != SWITCH_NAT_ENTRY_ATTR_TYPE_DOUBLE_NAT)
      return status;

    status = p4_object_match_action::data_get();
    if (status == SWITCH_STATUS_SUCCESS)
      return smi::nat_counters_get<_ActionEntry>(
          action_entry,
          smi_id::A_INGRESS_NAT_FLOW_NAT_REWRITE,
          smi_id::D_INGRESS_NAT_FLOW_NAT_COUNTER_SPEC_PKTS,
          smi_id::D_INGRESS_NAT_FLOW_NAT_COUNTER_SPEC_BYTES,
          counters);
    return status;
  }
  switch_status_t counters_set(const switch_object_id_t handle) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t nat_type = {0};
    uint64_t value = 0;
    status |= switch_store::v_get(handle, SWITCH_NAT_ENTRY_ATTR_TYPE, nat_type);
    if (nat_type.enumdata != SWITCH_NAT_ENTRY_ATTR_TYPE_DOUBLE_NAT)
      return status;

    action_entry.set_arg(smi_id::D_INGRESS_NAT_FLOW_NAT_COUNTER_SPEC_PKTS,
                         value);
    action_entry.set_arg(smi_id::D_INGRESS_NAT_FLOW_NAT_COUNTER_SPEC_BYTES,
                         value);

    return p4_object_match_action::data_set();
  }
  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t nat_type = {0};
    status |= switch_store::v_get(handle, SWITCH_NAT_ENTRY_ATTR_TYPE, nat_type);
    if (nat_type.enumdata != SWITCH_NAT_ENTRY_ATTR_TYPE_DOUBLE_NAT)
      return status;

    if (cntr_ids.size() == 0) return status;

    for (auto cntr_id : cntr_ids) {
      if (cntr_id == SWITCH_NAT_ENTRY_COUNTER_ID_PKTS) {
        nat_counters_set<_ActionEntry>(
            action_entry, smi_id::D_INGRESS_NAT_FLOW_NAT_COUNTER_SPEC_PKTS);
      } else if (cntr_id == SWITCH_NAT_ENTRY_COUNTER_ID_BYTES) {
        nat_counters_set<_ActionEntry>(
            action_entry, smi_id::D_INGRESS_NAT_FLOW_NAT_COUNTER_SPEC_BYTES);
      } else {
        return status;
      }
      return p4_object_match_action::data_set();
    }
    return status;
  }
};

class snapt_index : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_SNAPT_INDEX;
  static const switch_attr_id_t status_attr_id = SWITCH_SNAPT_INDEX_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_SNAPT_INDEX_ATTR_PARENT_HANDLE;

 public:
  snapt_index(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_INGRESS_NAT_SNAPT_INDEX,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_enum_t e = {0};
    switch_nat_entry_attr_type type;
    uint32_t aging_time = 0;

    if (smi_id::T_INGRESS_NAT_SNAPT_INDEX == 0) {
      return;
    }
    status |= switch_store::v_get(parent, SWITCH_NAT_ENTRY_ATTR_TYPE, e);
    type = static_cast<switch_nat_entry_attr_type>(e.enumdata);
    if (type != SWITCH_NAT_ENTRY_ATTR_TYPE_SOURCE_NAPT) return;
    uint16_t pat_index = 0;
    status |=
        switch_store::v_get(parent, SWITCH_NAT_ENTRY_ATTR_NAT_INDEX, pat_index);
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_NAT_ENTRY,
               "{}: Index stored in NAT entry: {}",
               __LINE__,
               pat_index);

    match_key.set_exact(smi_id::F_INGRESS_NAT_SNAPT_INDEX_IPSA,
                        parent,
                        SWITCH_NAT_ENTRY_ATTR_SRC_IP_KEY);
    match_key.set_exact(smi_id::F_INGRESS_NAT_SNAPT_INDEX_IP_L4_SRC_PORT,
                        parent,
                        SWITCH_NAT_ENTRY_ATTR_L4_SRC_PORT_KEY);
    match_key.set_exact(smi_id::F_INGRESS_NAT_SNAPT_INDEX_IP_PROTO,
                        parent,
                        SWITCH_NAT_ENTRY_ATTR_IP_PROTO_KEY);

    action_entry.init_action_data(smi_id::A_INGRESS_NAT_SNAPT_SET_INDEX);
    action_entry.set_arg(smi_id::P_INGRESS_NAT_SNAPT_SET_INDEX,
                         parent,
                         SWITCH_NAT_ENTRY_ATTR_NAT_INDEX);
    status |= switch_store::v_get(
        parent, SWITCH_NAT_ENTRY_ATTR_AGING_TIME, aging_time);
    action_entry.set_arg(smi_id::D_INGRESS_NAT_SNAPT_INDEX_TTL,
                         aging_time * 1000);
  }
};
class source_napt : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_SOURCE_NAPT;
  static const switch_attr_id_t status_attr_id = SWITCH_SOURCE_NAPT_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_SOURCE_NAPT_ATTR_PARENT_HANDLE;

 public:
  source_napt(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_INGRESS_NAT_SNAPT,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_enum_t e = {0};
    switch_nat_entry_attr_type type;

    status |= switch_store::v_get(parent, SWITCH_NAT_ENTRY_ATTR_TYPE, e);
    type = static_cast<switch_nat_entry_attr_type>(e.enumdata);

    if (type != SWITCH_NAT_ENTRY_ATTR_TYPE_SOURCE_NAPT) return;
    uint16_t pat_index = 0;
    status |=
        switch_store::v_get(parent, SWITCH_NAT_ENTRY_ATTR_NAT_INDEX, pat_index);

    // tofino2 does not use a pat index
    if (feature::is_feature_set(SWITCH_FEATURE_NAT_NAPT_INDEX_INDIRECTION)) {
      match_key.set_exact(smi_id::F_INGRESS_NAT_SNAPT_INDEX,
                          parent,
                          SWITCH_NAT_ENTRY_ATTR_NAT_INDEX);
    } else {
      match_key.set_exact(smi_id::F_INGRESS_NAT_SNAPT_IPSA,
                          parent,
                          SWITCH_NAT_ENTRY_ATTR_SRC_IP_KEY);
      match_key.set_exact(smi_id::F_INGRESS_NAT_SNAPT_IP_L4_SRC_PORT,
                          parent,
                          SWITCH_NAT_ENTRY_ATTR_L4_SRC_PORT_KEY);
      match_key.set_exact(smi_id::F_INGRESS_NAT_SNAPT_IP_PROTO,
                          parent,
                          SWITCH_NAT_ENTRY_ATTR_IP_PROTO_KEY);
    }

    action_entry.init_action_data(smi_id::A_INGRESS_NAT_SNAPT_REWRITE);
    action_entry.set_arg(smi_id::P_SOURCE_NAPT_REWRITE_SPORT,
                         parent,
                         SWITCH_NAT_ENTRY_ATTR_NAT_L4_SRC_PORT);
    action_entry.set_arg(smi_id::P_SOURCE_NAPT_REWRITE_SIP,
                         parent,
                         SWITCH_NAT_ENTRY_ATTR_NAT_SRC_IP);

    if (!feature::is_feature_set(SWITCH_FEATURE_NAT_NAPT_INDEX_INDIRECTION)) {
      uint32_t aging_time = 0;
      status |= switch_store::v_get(
          parent, SWITCH_NAT_ENTRY_ATTR_AGING_TIME, aging_time);
      action_entry.set_arg(smi_id::D_INGRESS_NAT_SNAPT_TTL, aging_time * 1000);
    }
  }
  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &counters) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t nat_type = {0};
    status |= switch_store::v_get(handle, SWITCH_NAT_ENTRY_ATTR_TYPE, nat_type);
    if (nat_type.enumdata != SWITCH_NAT_ENTRY_ATTR_TYPE_SOURCE_NAPT)
      return status;

    status = p4_object_match_action::data_get();
    if (status == SWITCH_STATUS_SUCCESS) {
      return smi::nat_counters_get<_ActionEntry>(
          action_entry,
          smi_id::A_INGRESS_NAT_SNAPT_REWRITE,
          smi_id::D_INGRESS_NAT_SNAPT_COUNTER_SPEC_PKTS,
          smi_id::D_INGRESS_NAT_SNAPT_COUNTER_SPEC_BYTES,
          counters);
    }
    return status;
  }

  switch_status_t counters_set(const switch_object_id_t handle) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t nat_type = {0};
    uint64_t value = 0;
    status |= switch_store::v_get(handle, SWITCH_NAT_ENTRY_ATTR_TYPE, nat_type);
    if (nat_type.enumdata != SWITCH_NAT_ENTRY_ATTR_TYPE_SOURCE_NAPT)
      return status;

    action_entry.set_arg(smi_id::D_INGRESS_NAT_SNAPT_COUNTER_SPEC_PKTS, value);
    action_entry.set_arg(smi_id::D_INGRESS_NAT_SNAPT_COUNTER_SPEC_BYTES, value);

    return p4_object_match_action::data_set();
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t nat_type = {0};
    status |= switch_store::v_get(handle, SWITCH_NAT_ENTRY_ATTR_TYPE, nat_type);
    if (nat_type.enumdata != SWITCH_NAT_ENTRY_ATTR_TYPE_SOURCE_NAPT)
      return status;

    if (cntr_ids.size() == 0) return status;

    for (auto cntr_id : cntr_ids) {
      if (cntr_id == SWITCH_NAT_ENTRY_COUNTER_ID_PKTS) {
        nat_counters_set<_ActionEntry>(
            action_entry, smi_id::D_INGRESS_NAT_SNAPT_COUNTER_SPEC_PKTS);
      } else if (cntr_id == SWITCH_NAT_ENTRY_COUNTER_ID_BYTES) {
        nat_counters_set<_ActionEntry>(
            action_entry, smi_id::D_INGRESS_NAT_SNAPT_COUNTER_SPEC_BYTES);
      } else {
        return status;
      }
      return p4_object_match_action::data_set();
    }
    return status;
  }
};

class source_nat : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_SOURCE_NAT;
  static const switch_attr_id_t status_attr_id = SWITCH_SOURCE_NAT_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_SOURCE_NAT_ATTR_PARENT_HANDLE;

 public:
  source_nat(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_INGRESS_NAT_SNAT,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_enum_t e = {0};
    switch_nat_entry_attr_type type;
    uint32_t aging_time = 0;

    status |= switch_store::v_get(parent, SWITCH_NAT_ENTRY_ATTR_TYPE, e);
    type = static_cast<switch_nat_entry_attr_type>(e.enumdata);

    if (type != SWITCH_NAT_ENTRY_ATTR_TYPE_SOURCE_NAT) return;
    match_key.set_exact(smi_id::F_INGRESS_NAT_SNAT_IPSA,
                        parent,
                        SWITCH_NAT_ENTRY_ATTR_SRC_IP_KEY);
    action_entry.init_action_data(smi_id::A_INGRESS_NAT_SNAT_REWRITE);
    action_entry.set_arg(smi_id::P_SOURCE_NAT_REWRITE_SIP,
                         parent,
                         SWITCH_NAT_ENTRY_ATTR_NAT_SRC_IP);
    status |= switch_store::v_get(
        parent, SWITCH_NAT_ENTRY_ATTR_AGING_TIME, aging_time);
    action_entry.set_arg(smi_id::D_INGRESS_NAT_SNAT_TTL, aging_time * 1000);
  }
  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &counters) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t nat_type = {0};
    status |= switch_store::v_get(handle, SWITCH_NAT_ENTRY_ATTR_TYPE, nat_type);
    if (nat_type.enumdata != SWITCH_NAT_ENTRY_ATTR_TYPE_SOURCE_NAT)
      return status;

    status = p4_object_match_action::data_get();
    if (status == SWITCH_STATUS_SUCCESS)
      return smi::nat_counters_get<_ActionEntry>(
          action_entry,
          smi_id::A_INGRESS_NAT_SNAT_REWRITE,
          smi_id::D_INGRESS_NAT_SNAT_COUNTER_SPEC_PKTS,
          smi_id::D_INGRESS_NAT_SNAT_COUNTER_SPEC_BYTES,
          counters);
    return status;
  }
  switch_status_t counters_set(const switch_object_id_t handle) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t nat_type = {0};
    uint64_t value = 0;
    status |= switch_store::v_get(handle, SWITCH_NAT_ENTRY_ATTR_TYPE, nat_type);
    if (nat_type.enumdata != SWITCH_NAT_ENTRY_ATTR_TYPE_SOURCE_NAT)
      return status;

    action_entry.set_arg(smi_id::D_INGRESS_NAT_SNAT_COUNTER_SPEC_PKTS, value);
    action_entry.set_arg(smi_id::D_INGRESS_NAT_SNAT_COUNTER_SPEC_BYTES, value);

    return p4_object_match_action::data_set();
  }
  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t nat_type = {0};
    status |= switch_store::v_get(handle, SWITCH_NAT_ENTRY_ATTR_TYPE, nat_type);
    if (nat_type.enumdata != SWITCH_NAT_ENTRY_ATTR_TYPE_SOURCE_NAT)
      return status;

    if (cntr_ids.size() == 0) return status;

    for (auto cntr_id : cntr_ids) {
      if (cntr_id == SWITCH_NAT_ENTRY_COUNTER_ID_PKTS) {
        nat_counters_set<_ActionEntry>(
            action_entry, smi_id::D_INGRESS_NAT_SNAT_COUNTER_SPEC_PKTS);
      } else if (cntr_id == SWITCH_NAT_ENTRY_COUNTER_ID_BYTES) {
        nat_counters_set<_ActionEntry>(
            action_entry, smi_id::D_INGRESS_NAT_SNAT_COUNTER_SPEC_BYTES);
      } else {
        return status;
      }
      return p4_object_match_action::data_set();
    }
    return status;
  }
};

#define REWRITE_NAT_ENTRY                       \
  std::pair<_MatchKey, _ActionEntry>(           \
      _MatchKey(smi_id::T_INGRESS_NAT_REWRITE), \
      _ActionEntry(smi_id::T_INGRESS_NAT_REWRITE))

class rewrite_nat : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_REWRITE_NAT;
  static const switch_attr_id_t status_attr_id = SWITCH_REWRITE_NAT_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_REWRITE_NAT_ATTR_PARENT_HANDLE;

 public:
  rewrite_nat(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_INGRESS_NAT_REWRITE,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    // Flow NAPT, rewrite ipsa, ipda, TCP port.
    uint8_t tcp_proto = SWITCH_HOSTIF_IP_PROTO_TCP;
    uint8_t udp_proto = SWITCH_HOSTIF_IP_PROTO_UDP;
    uint8_t icmp_proto = SWITCH_HOSTIF_IP_PROTO_ICMP;
    uint8_t same_zone_check = 1;
    uint8_t same_zone_check_mask = 1;

    auto it = match_action_list.begin();

    it = match_action_list.insert(it, REWRITE_NAT_ENTRY);
    // ignore same_zone_check for flow napt
    // Flow NAPT, rewrite ipsa, ipda, TCP port.
    same_zone_check = 0;
    same_zone_check_mask = 0;
    status |= it->first.set_exact(smi_id::F_INGRESS_NAT_REWRITE_NAT_HIT,
                                  SWITCH_NAT_HIT_TYPE_FLOW_NAPT);
    status |=
        it->first.set_exact(smi_id::F_INGRESS_NAT_REWRITE_IP_PROTO, tcp_proto);
    status |=
        it->first.set_ternary(smi_id::F_INGRESS_NAT_REWRITE_NAT_SAME_ZONE_CHECK,
                              same_zone_check,
                              same_zone_check_mask);
    it->second.init_action_data(smi_id::A_INGRESS_NAT_REWRITE_TCP_FLOW);

    // Flow NAPT, rewrite ipsa, ipda, UDP port.
    it = match_action_list.insert(it, REWRITE_NAT_ENTRY);
    status |= it->first.set_exact(smi_id::F_INGRESS_NAT_REWRITE_NAT_HIT,
                                  SWITCH_NAT_HIT_TYPE_FLOW_NAPT);
    status |=
        it->first.set_exact(smi_id::F_INGRESS_NAT_REWRITE_IP_PROTO, udp_proto);
    status |=
        it->first.set_ternary(smi_id::F_INGRESS_NAT_REWRITE_NAT_SAME_ZONE_CHECK,
                              same_zone_check,
                              same_zone_check_mask);
    it->second.init_action_data(smi_id::A_INGRESS_NAT_REWRITE_UDP_FLOW);

    // Flow NAT, TCP rewrite ipsa, ipda
    it = match_action_list.insert(it, REWRITE_NAT_ENTRY);
    status |= it->first.set_exact(smi_id::F_INGRESS_NAT_REWRITE_NAT_HIT,
                                  SWITCH_NAT_HIT_TYPE_FLOW_NAT);
    status |=
        it->first.set_exact(smi_id::F_INGRESS_NAT_REWRITE_IP_PROTO, tcp_proto);
    status |=
        it->first.set_ternary(smi_id::F_INGRESS_NAT_REWRITE_NAT_SAME_ZONE_CHECK,
                              same_zone_check,
                              same_zone_check_mask);
    it->second.init_action_data(smi_id::A_INGRESS_NAT_REWRITE_IPSA_IPDA);

    // Flow NAT, UDP rewrite ipsa, ipda
    it = match_action_list.insert(it, REWRITE_NAT_ENTRY);
    status |= it->first.set_exact(smi_id::F_INGRESS_NAT_REWRITE_NAT_HIT,
                                  SWITCH_NAT_HIT_TYPE_FLOW_NAT);
    status |=
        it->first.set_exact(smi_id::F_INGRESS_NAT_REWRITE_IP_PROTO, udp_proto);
    status |=
        it->first.set_ternary(smi_id::F_INGRESS_NAT_REWRITE_NAT_SAME_ZONE_CHECK,
                              same_zone_check,
                              same_zone_check_mask);
    it->second.init_action_data(smi_id::A_INGRESS_NAT_REWRITE_IPSA_IPDA);

    // Dest NAPT, rewrite ipda, TCP port
    same_zone_check = 1;
    same_zone_check_mask = 1;
    it = match_action_list.insert(it, REWRITE_NAT_ENTRY);
    status |= it->first.set_exact(smi_id::F_INGRESS_NAT_REWRITE_NAT_HIT,
                                  SWITCH_NAT_HIT_TYPE_DEST_NAPT);
    status |=
        it->first.set_exact(smi_id::F_INGRESS_NAT_REWRITE_IP_PROTO, tcp_proto);
    status |=
        it->first.set_ternary(smi_id::F_INGRESS_NAT_REWRITE_NAT_SAME_ZONE_CHECK,
                              same_zone_check,
                              same_zone_check_mask);
    it->second.init_action_data(smi_id::A_INGRESS_NAT_REWRITE_TCP_DPORT_IPDA);

    // Dest NAPT, rewrite ipda, UDP port
    it = match_action_list.insert(it, REWRITE_NAT_ENTRY);
    status |= it->first.set_exact(smi_id::F_INGRESS_NAT_REWRITE_NAT_HIT,
                                  SWITCH_NAT_HIT_TYPE_DEST_NAPT);
    status |=
        it->first.set_exact(smi_id::F_INGRESS_NAT_REWRITE_IP_PROTO, udp_proto);
    status |=
        it->first.set_ternary(smi_id::F_INGRESS_NAT_REWRITE_NAT_SAME_ZONE_CHECK,
                              same_zone_check,
                              same_zone_check_mask);
    it->second.init_action_data(smi_id::A_INGRESS_NAT_REWRITE_UDP_DPORT_IPDA);

    // Dest NAT, TCP, rewrite ipda
    it = match_action_list.insert(it, REWRITE_NAT_ENTRY);
    status |= it->first.set_exact(smi_id::F_INGRESS_NAT_REWRITE_NAT_HIT,
                                  SWITCH_NAT_HIT_TYPE_DEST_NAT);
    status |=
        it->first.set_exact(smi_id::F_INGRESS_NAT_REWRITE_IP_PROTO, tcp_proto);
    status |=
        it->first.set_ternary(smi_id::F_INGRESS_NAT_REWRITE_NAT_SAME_ZONE_CHECK,
                              same_zone_check,
                              same_zone_check_mask);
    it->second.init_action_data(smi_id::A_INGRESS_NAT_REWRITE_IPDA);

    // Dest NAT, UDP, rewrite ipda
    it = match_action_list.insert(it, REWRITE_NAT_ENTRY);
    status |= it->first.set_exact(smi_id::F_INGRESS_NAT_REWRITE_NAT_HIT,
                                  SWITCH_NAT_HIT_TYPE_DEST_NAT);
    status |=
        it->first.set_exact(smi_id::F_INGRESS_NAT_REWRITE_IP_PROTO, udp_proto);
    status |=
        it->first.set_ternary(smi_id::F_INGRESS_NAT_REWRITE_NAT_SAME_ZONE_CHECK,
                              same_zone_check,
                              same_zone_check_mask);
    it->second.init_action_data(smi_id::A_INGRESS_NAT_REWRITE_IPDA);

    // Dest NAT, ICMP, rewrite ipda
    it = match_action_list.insert(it, REWRITE_NAT_ENTRY);
    status |= it->first.set_exact(smi_id::F_INGRESS_NAT_REWRITE_NAT_HIT,
                                  SWITCH_NAT_HIT_TYPE_DEST_NAT);
    status |=
        it->first.set_exact(smi_id::F_INGRESS_NAT_REWRITE_IP_PROTO, icmp_proto);
    status |=
        it->first.set_ternary(smi_id::F_INGRESS_NAT_REWRITE_NAT_SAME_ZONE_CHECK,
                              same_zone_check,
                              same_zone_check_mask);
    it->second.init_action_data(smi_id::A_INGRESS_NAT_REWRITE_IPDA);

    same_zone_check = 1;
    same_zone_check_mask = 1;
    // Source NAPT, rewrite ipsa, UDP port
    it = match_action_list.insert(it, REWRITE_NAT_ENTRY);
    status |= it->first.set_exact(smi_id::F_INGRESS_NAT_REWRITE_NAT_HIT,
                                  SWITCH_NAT_HIT_TYPE_SRC_NAPT);
    status |=
        it->first.set_exact(smi_id::F_INGRESS_NAT_REWRITE_IP_PROTO, udp_proto);
    status |=
        it->first.set_ternary(smi_id::F_INGRESS_NAT_REWRITE_NAT_SAME_ZONE_CHECK,
                              same_zone_check,
                              same_zone_check_mask);
    it->second.init_action_data(smi_id::A_INGRESS_NAT_REWRITE_UDP_SPORT_IPSA);

    // Source NAPT, TCP, rewrite ipsa
    it = match_action_list.insert(it, REWRITE_NAT_ENTRY);
    status |= it->first.set_exact(smi_id::F_INGRESS_NAT_REWRITE_NAT_HIT,
                                  SWITCH_NAT_HIT_TYPE_SRC_NAPT);
    status |=
        it->first.set_exact(smi_id::F_INGRESS_NAT_REWRITE_IP_PROTO, tcp_proto);
    status |=
        it->first.set_ternary(smi_id::F_INGRESS_NAT_REWRITE_NAT_SAME_ZONE_CHECK,
                              same_zone_check,
                              same_zone_check_mask);
    it->second.init_action_data(smi_id::A_INGRESS_NAT_REWRITE_TCP_SPORT_IPSA);

    // Source NAT, ICMP, rewrite ipsa
    it = match_action_list.insert(it, REWRITE_NAT_ENTRY);
    status |= it->first.set_exact(smi_id::F_INGRESS_NAT_REWRITE_NAT_HIT,
                                  SWITCH_NAT_HIT_TYPE_SRC_NAT);
    status |=
        it->first.set_exact(smi_id::F_INGRESS_NAT_REWRITE_IP_PROTO, icmp_proto);
    status |=
        it->first.set_ternary(smi_id::F_INGRESS_NAT_REWRITE_NAT_SAME_ZONE_CHECK,
                              same_zone_check,
                              same_zone_check_mask);
    it->second.init_action_data(smi_id::A_INGRESS_NAT_REWRITE_IPSA);

    // Source NAT, rewrite ipsa, UDP port
    it = match_action_list.insert(it, REWRITE_NAT_ENTRY);
    status |= it->first.set_exact(smi_id::F_INGRESS_NAT_REWRITE_NAT_HIT,
                                  SWITCH_NAT_HIT_TYPE_SRC_NAT);
    status |=
        it->first.set_exact(smi_id::F_INGRESS_NAT_REWRITE_IP_PROTO, udp_proto);
    status |=
        it->first.set_ternary(smi_id::F_INGRESS_NAT_REWRITE_NAT_SAME_ZONE_CHECK,
                              same_zone_check,
                              same_zone_check_mask);
    it->second.init_action_data(smi_id::A_INGRESS_NAT_REWRITE_IPSA);

    // Source NAT, TCP, rewrite ipsa
    it = match_action_list.insert(it, REWRITE_NAT_ENTRY);
    status |= it->first.set_exact(smi_id::F_INGRESS_NAT_REWRITE_NAT_HIT,
                                  SWITCH_NAT_HIT_TYPE_SRC_NAT);
    status |=
        it->first.set_exact(smi_id::F_INGRESS_NAT_REWRITE_IP_PROTO, tcp_proto);
    status |=
        it->first.set_ternary(smi_id::F_INGRESS_NAT_REWRITE_NAT_SAME_ZONE_CHECK,
                              same_zone_check,
                              same_zone_check_mask);
    it->second.init_action_data(smi_id::A_INGRESS_NAT_REWRITE_IPSA);
  }
};

class nat_factory : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_NAT_FACTORY;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_NAT_FACTORY_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id = SWITCH_NAT_FACTORY_ATTR_STATUS;
  std::unique_ptr<object> asic_object;
  std::unique_ptr<object> index_object;

 public:
  nat_factory(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    switch_enum_t e = {0};
    switch_nat_entry_attr_type type;

    status |= switch_store::v_get(parent, SWITCH_NAT_ENTRY_ATTR_TYPE, e);
    type = static_cast<switch_nat_entry_attr_type>(e.enumdata);
    switch (type) {
      case SWITCH_NAT_ENTRY_ATTR_TYPE_DESTINATION_NAPT:
        index_object =
            std::unique_ptr<dnapt_index>(new dnapt_index(parent, status));
        asic_object = std::unique_ptr<dest_napt>(new dest_napt(parent, status));
        break;
      case SWITCH_NAT_ENTRY_ATTR_TYPE_DESTINATION_NAT:
        asic_object = std::unique_ptr<dest_nat>(new dest_nat(parent, status));
        break;
      case SWITCH_NAT_ENTRY_ATTR_TYPE_DESTINATION_NAT_POOL:
        asic_object =
            std::unique_ptr<dest_nat_pool>(new dest_nat_pool(parent, status));
        break;
      case SWITCH_NAT_ENTRY_ATTR_TYPE_SOURCE_NAPT:
        index_object =
            std::unique_ptr<snapt_index>(new snapt_index(parent, status));
        asic_object =
            std::unique_ptr<source_napt>(new source_napt(parent, status));
        break;
      case SWITCH_NAT_ENTRY_ATTR_TYPE_SOURCE_NAT:
        asic_object =
            std::unique_ptr<source_nat>(new source_nat(parent, status));
        break;
      case SWITCH_NAT_ENTRY_ATTR_TYPE_DOUBLE_NAT:
        asic_object = std::unique_ptr<flow_nat>(new flow_nat(parent, status));
        break;
      case SWITCH_NAT_ENTRY_ATTR_TYPE_DOUBLE_NAPT:
        asic_object = std::unique_ptr<flow_napt>(new flow_napt(parent, status));
        break;
      case SWITCH_NAT_ENTRY_ATTR_TYPE_NONE:
      case SWITCH_NAT_ENTRY_ATTR_TYPE_MAX:
        break;
    }
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (index_object != NULL) {
      status = index_object->create_update();
      if (status != SWITCH_STATUS_SUCCESS) return status;
    }
    if (asic_object != NULL) {
      status = asic_object->create_update();
      if (status != SWITCH_STATUS_SUCCESS) return status;
    }
    return auto_object::create_update();
  }
  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (index_object != NULL) {
      status = index_object->del();
      if (status != SWITCH_STATUS_SUCCESS) return status;
    }
    if (asic_object != NULL) {
      status = asic_object->del();
      if (status != SWITCH_STATUS_SUCCESS) return status;
    }
    return auto_object::del();
  }
  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (asic_object != NULL) {
      status = asic_object->counters_get(handle, cntrs);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_NAT_ENTRY,
                   "{}.{}: Failed counters_get for NAT entry, status:{}",
                   "nat_factory",
                   __LINE__,
                   status);
        return status;
      }
    }
    return status;
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (asic_object != NULL) {
      status = asic_object->counters_set(handle, cntr_ids);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_NAT_ENTRY,
                   "{}.{}: Failed counters_set for NAT entry, status:{}",
                   "nat_factory",
                   __LINE__,
                   status);
        return status;
      }
    }
    return status;
  }
};

/*
 * For SNAPT and DNAPT, use indexer to allocate index for every NAT entry and
 * store in
 * NAT object. For warmboot case, reserve the index cached in the object.
 */
switch_status_t before_nat_entry_create(const switch_object_type_t object_type,
                                        std::set<attr_w> &attrs) {
  (void)object_type;
  const auto attr_it =
      attrs.find(static_cast<switch_attr_id_t>(SWITCH_NAT_ENTRY_ATTR_TYPE));
  switch_enum_t nat_type = {0};
  switch_nat_entry_attr_type type;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  if (attr_it != attrs.end()) {
    attr_it->v_get(nat_type);
  } else {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NAT_ENTRY,
               "{}: invalid nat type",
               __LINE__);
    return SWITCH_STATUS_INVALID_PARAMETER;
  }
  type = static_cast<switch_nat_entry_attr_type>(nat_type.enumdata);
  if (type != SWITCH_NAT_ENTRY_ATTR_TYPE_DESTINATION_NAPT &&
      type != SWITCH_NAT_ENTRY_ATTR_TYPE_SOURCE_NAPT) {
    return status;
  }

  const auto index_attr_it = attrs.find(
      static_cast<switch_attr_id_t>(SWITCH_NAT_ENTRY_ATTR_NAT_INDEX));
  uint16_t pat_index = 0;
  attr_w pat_index_attr(SWITCH_NAT_ENTRY_ATTR_NAT_INDEX);
  if (index_attr_it == attrs.end()) {
    if (type == SWITCH_NAT_ENTRY_ATTR_TYPE_DESTINATION_NAPT) {
      pat_index = SWITCH_CONTEXT.dnapt_index_allocate();
    } else if (type == SWITCH_NAT_ENTRY_ATTR_TYPE_SOURCE_NAPT) {
      pat_index = SWITCH_CONTEXT.snapt_index_allocate();
    }
    pat_index_attr.v_set(pat_index);
    attrs.insert(pat_index_attr);
  } else {
    status = (*index_attr_it).v_get(pat_index);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_NAT_ENTRY,
                 "{};{}: Failed to get allocated pat index",
                 __func__,
                 __LINE__);
      return status;
    }
    if (type == SWITCH_NAT_ENTRY_ATTR_TYPE_DESTINATION_NAPT) {
      SWITCH_CONTEXT.dnapt_index_reserve(pat_index);
    } else if (type == SWITCH_NAT_ENTRY_ATTR_TYPE_SOURCE_NAPT) {
      SWITCH_CONTEXT.snapt_index_reserve(pat_index);
    }
  }
  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_NAT_ENTRY,
             "{}:{}: Allocated PAT index to be used {} for NAT Type {}",
             __func__,
             __LINE__,
             pat_index,
             type);
  return status;
}

switch_status_t after_nat_entry_delete(const switch_object_type_t object_type,
                                       const std::set<attr_w> &attrs) {
  switch_enum_t nat_type = {0};
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  ModelInfo *model_info = switch_store::switch_model_info_get();
  const auto attr_it =
      attrs.find(static_cast<switch_attr_id_t>(SWITCH_NAT_ENTRY_ATTR_TYPE));
  if (attr_it == attrs.end()) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NAT_ENTRY,
               "{}: invalid nat type",
               __LINE__);
    return SWITCH_STATUS_INVALID_PARAMETER;
  }
  attr_it->v_get(nat_type);
  if (nat_type.enumdata != SWITCH_NAT_ENTRY_ATTR_TYPE_DESTINATION_NAPT &&
      nat_type.enumdata != SWITCH_NAT_ENTRY_ATTR_TYPE_SOURCE_NAPT) {
    return status;
  }

  uint16_t pat_index = 0;
  const auto index_attr_it = attrs.find(
      static_cast<switch_attr_id_t>(SWITCH_NAT_ENTRY_ATTR_NAT_INDEX));
  if (index_attr_it == attrs.end()) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NAT_ENTRY,
               "{};{}: Failed to get allocated pat index",
               __func__,
               __LINE__);
    return SWITCH_STATUS_INVALID_PARAMETER;
  }
  status = (*index_attr_it).v_get(pat_index);

  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NAT_ENTRY,
               "{}:{}: Failed to retrieve PAT index from object type {}",
               __func__,
               __LINE__,
               model_info->get_object_name_from_type(object_type));
    return status;
  }

  if (pat_index != 0) {
    if (nat_type.enumdata == SWITCH_NAT_ENTRY_ATTR_TYPE_DESTINATION_NAPT) {
      SWITCH_CONTEXT.dnapt_index_release(pat_index);
    } else if (nat_type.enumdata == SWITCH_NAT_ENTRY_ATTR_TYPE_SOURCE_NAPT) {
      SWITCH_CONTEXT.snapt_index_release(pat_index);
    }
  }
  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_NAT_ENTRY,
             "{}:{}: Release allocated PAT index {}",
             __func__,
             __LINE__,
             pat_index);
  return status;
}

static void nat_aging_notify(switch_object_id_t nat_entry_handle) {
  switch_nat_payload_t payload = {};
  switch_nat_event_data_t nat_data;

  payload.nat_event = SWITCH_NAT_EVENT_AGED;
  payload.nat_handle = nat_entry_handle;
  nat_data.payload.push_back(payload);
  smi::event::nat_event_notify(nat_data);
}

/*
 * Return nat_handle given the input key parameters
 */
static switch_status_t nat_get_handle(const uint16_t dev_id,
                                      const switch_nat_entry_attr_type type,
                                      const switch_ip4_t dip,
                                      const switch_ip4_t sip,
                                      const uint8_t proto,
                                      const uint16_t dport,
                                      const uint16_t sport,
                                      switch_object_id_t &nat_entry_handle) {
  switch_status_t status;
  switch_object_id_t device_handle = {0};
  switch_enum_t nat_type = {0};
  switch_ip_address_t dst_ip_key, src_ip_key;

  std::set<attr_w> attrs_dev;
  attrs_dev.insert(attr_w(SWITCH_DEVICE_ATTR_DEV_ID, dev_id));
  status = switch_store::object_id_get_wkey(
      SWITCH_OBJECT_TYPE_DEVICE, attrs_dev, device_handle);
  if (device_handle.data == 0) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NAT_ENTRY,
               "{}.{}: failed to find device hdl",
               __func__,
               __LINE__);
    return status;
  }

  std::set<attr_w> nat_attrs;
  nat_attrs.insert(attr_w(SWITCH_NAT_ENTRY_ATTR_DEVICE, device_handle));
  nat_type.enumdata = type;
  nat_attrs.insert(attr_w(SWITCH_NAT_ENTRY_ATTR_TYPE, nat_type));
  dst_ip_key.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
  dst_ip_key.ip4 = htonl(dip);
  nat_attrs.insert(attr_w(SWITCH_NAT_ENTRY_ATTR_DST_IP_KEY, dst_ip_key));
  src_ip_key.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
  src_ip_key.ip4 = htonl(sip);
  nat_attrs.insert(attr_w(SWITCH_NAT_ENTRY_ATTR_SRC_IP_KEY, src_ip_key));
  nat_attrs.insert(attr_w(SWITCH_NAT_ENTRY_ATTR_IP_PROTO_KEY, proto));
  nat_attrs.insert(attr_w(SWITCH_NAT_ENTRY_ATTR_L4_DST_PORT_KEY, htons(dport)));
  nat_attrs.insert(attr_w(SWITCH_NAT_ENTRY_ATTR_L4_SRC_PORT_KEY, htons(sport)));

  status = switch_store::object_id_get_wkey(
      SWITCH_OBJECT_TYPE_NAT_ENTRY, nat_attrs, nat_entry_handle);
  if (nat_entry_handle.data == 0) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NAT_ENTRY,
               "{}.{}: failed to find nat entry type {} dip {} sip {} proto {} "
               "dport {} sport {}",
               __func__,
               __LINE__,
               type,
               dip,
               sip,
               proto,
               dport,
               sport);
    return status;
  }

  return SWITCH_STATUS_SUCCESS;
}

static bf_status_t dnapt_aging_nofity_cb(const bf_rt_target_t &dev_tgt,
                                         const BfRtTableKey *match_spec,
                                         void *client_data) {
  switch_status_t status;
  (void)client_data;
  switch_object_id_t nat_entry_handle = {0};
  uint16_t dev_id = dev_tgt.dev_id;
  switch_ip4_t dip, sip = 0;
  uint16_t dport, sport = 0;
  uint8_t proto;

  if (feature::is_feature_set(SWITCH_FEATURE_NAT_NAPT_INDEX_INDIRECTION)) {
    match_spec->getValue(
        smi_id::F_INGRESS_NAT_DNAPT_INDEX_PROTOCOL, sizeof(proto), &proto);
    match_spec->getValue(smi_id::F_INGRESS_NAT_DNAPT_INDEX_DIP,
                         sizeof(dip),
                         reinterpret_cast<uint8_t *>(&dip));
    match_spec->getValue(smi_id::F_INGRESS_NAT_DNAPT_INDEX_DPORT,
                         sizeof(dport),
                         reinterpret_cast<uint8_t *>(&dport));
  } else {
    match_spec->getValue(
        smi_id::F_INGRESS_NAT_DEST_NAPT_PROTOCOL, sizeof(proto), &proto);
    match_spec->getValue(smi_id::F_INGRESS_NAT_DEST_NAPT_DIP,
                         sizeof(dip),
                         reinterpret_cast<uint8_t *>(&dip));
    match_spec->getValue(smi_id::F_INGRESS_NAT_DEST_NAPT_DPORT,
                         sizeof(dport),
                         reinterpret_cast<uint8_t *>(&dport));
  }

  status = nat_get_handle(dev_id,
                          SWITCH_NAT_ENTRY_ATTR_TYPE_DESTINATION_NAPT,
                          dip,
                          sip,
                          proto,
                          dport,
                          sport,
                          nat_entry_handle);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NAT_ENTRY,
               "{}.{}: failed to get nat handle",
               __func__,
               __LINE__);
    return status;
  }

  nat_aging_notify(nat_entry_handle);

  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_NAT_ENTRY,
             "Aging: dnapt_aging_nofity_cb done");
  return BF_SUCCESS;
}

static bf_status_t snapt_aging_nofity_cb(const bf_rt_target_t &dev_tgt,
                                         const BfRtTableKey *match_spec,
                                         void *client_data) {
  switch_status_t status;
  (void)client_data;
  switch_object_id_t nat_entry_handle = {0};
  uint16_t dev_id = dev_tgt.dev_id;
  switch_ip4_t sip, dip = 0;
  uint16_t sport, dport = 0;
  uint8_t proto;

  if (feature::is_feature_set(SWITCH_FEATURE_NAT_NAPT_INDEX_INDIRECTION)) {
    match_spec->getValue(
        smi_id::F_INGRESS_NAT_SNAPT_INDEX_IP_PROTO, sizeof(proto), &proto);
    match_spec->getValue(smi_id::F_INGRESS_NAT_SNAPT_INDEX_IPSA,
                         sizeof(sip),
                         reinterpret_cast<uint8_t *>(&sip));
    match_spec->getValue(smi_id::F_INGRESS_NAT_SNAPT_INDEX_IP_L4_SRC_PORT,
                         sizeof(sport),
                         reinterpret_cast<uint8_t *>(&sport));
  } else {
    match_spec->getValue(
        smi_id::F_INGRESS_NAT_SNAPT_IP_PROTO, sizeof(proto), &proto);
    match_spec->getValue(smi_id::F_INGRESS_NAT_SNAPT_IPSA,
                         sizeof(sip),
                         reinterpret_cast<uint8_t *>(&sip));
    match_spec->getValue(smi_id::F_INGRESS_NAT_SNAPT_IP_L4_SRC_PORT,
                         sizeof(sport),
                         reinterpret_cast<uint8_t *>(&sport));
  }

  status = nat_get_handle(dev_id,
                          SWITCH_NAT_ENTRY_ATTR_TYPE_SOURCE_NAPT,
                          dip,
                          sip,
                          proto,
                          dport,
                          sport,
                          nat_entry_handle);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NAT_ENTRY,
               "{}.{}: failed to get nat handle",
               __func__,
               __LINE__);
    return status;
  }

  nat_aging_notify(nat_entry_handle);

  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_NAT_ENTRY,
             "Aging: snapt_aging_nofity_cb done");
  return BF_SUCCESS;
}

static bf_status_t dnat_aging_nofity_cb(const bf_rt_target_t &dev_tgt,
                                        const BfRtTableKey *match_spec,
                                        void *client_data) {
  switch_status_t status;
  (void)client_data;
  switch_object_id_t nat_entry_handle = {0};
  uint16_t dev_id = dev_tgt.dev_id;
  switch_ip4_t dip, sip = 0;
  uint16_t sport = 0, dport = 0;
  uint8_t proto = 0;

  match_spec->getValue(smi_id::F_INGRESS_NAT_DEST_NAPT_DIP,
                       sizeof(dip),
                       reinterpret_cast<uint8_t *>(&dip));

  status = nat_get_handle(dev_id,
                          SWITCH_NAT_ENTRY_ATTR_TYPE_DESTINATION_NAT,
                          dip,
                          sip,
                          proto,
                          dport,
                          sport,
                          nat_entry_handle);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NAT_ENTRY,
               "{}.{}: failed to get nat handle",
               __func__,
               __LINE__);
    return status;
  }

  nat_aging_notify(nat_entry_handle);

  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_NAT_ENTRY,
             "Aging: dnat_aging_nofity_cb done");
  return BF_SUCCESS;
}

static bf_status_t snat_aging_nofity_cb(const bf_rt_target_t &dev_tgt,
                                        const BfRtTableKey *match_spec,
                                        void *client_data) {
  switch_status_t status;
  (void)client_data;
  switch_object_id_t nat_entry_handle = {0};
  uint16_t dev_id = dev_tgt.dev_id;
  switch_ip4_t sip, dip = 0;
  uint16_t sport = 0, dport = 0;
  uint8_t proto = 0;

  match_spec->getValue(smi_id::F_INGRESS_NAT_SNAT_IPSA,
                       sizeof(sip),
                       reinterpret_cast<uint8_t *>(&sip));

  status = nat_get_handle(dev_id,
                          SWITCH_NAT_ENTRY_ATTR_TYPE_SOURCE_NAT,
                          dip,
                          sip,
                          proto,
                          dport,
                          sport,
                          nat_entry_handle);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NAT_ENTRY,
               "{}.{}: failed to get nat handle",
               __func__,
               __LINE__);
    return status;
  }

  nat_aging_notify(nat_entry_handle);

  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_NAT_ENTRY,
             "Aging: snat_aging_nofity_cb done");
  return BF_SUCCESS;
}

static switch_status_t nat_aging_callback_register_table(
    bf_rt_id_t table_id, const BfRtIdleTmoExpiryCb &callback) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  bf_status_t rc = BF_SUCCESS;
  const BfRtInfo *bfrtinfo = get_bf_rt_info();
  const bf_rt_target_t dev_tgt = get_dev_tgt();
  const BfRtTable *table = NULL;
  ENTER();

  std::unique_ptr<BfRtTableAttributes> table_attributes;
  rc = bfrtinfo->bfrtTableFromIdGet(table_id, &table);
  if (!table) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: failed to get table for aging object",
               __func__,
               __LINE__);
    return SWITCH_STATUS_FAILURE;
  }

  rc = table->attributeAllocate(TableAttributesType::IDLE_TABLE_RUNTIME,
                                TableAttributesIdleTableMode::NOTIFY_MODE,
                                &table_attributes);
  if (!table_attributes) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: failed to get table for aging object",
               __func__,
               __LINE__);
    return SWITCH_STATUS_FAILURE;
  }

  rc = table_attributes->idleTableNotifyModeSet(true,
                                                callback,
                                                NAT_AGING_QUERY_INTERVAL,
                                                NAT_AGING_MAX_TIME,
                                                NAT_AGING_MIN_TIME,
                                                NULL);
  if (rc != BF_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: failed to register aging callback rc={}",
               __func__,
               __LINE__,
               rc);
    return SWITCH_STATUS_FAILURE;
  }

  rc = table->tableAttributesSet(
      get_bf_rt_session(), dev_tgt, *table_attributes);
  if (rc != BF_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: failed to set table attributes",
               __func__,
               __LINE__);
    return SWITCH_STATUS_FAILURE;
  }

  EXIT();
  return status;
}

switch_status_t nat_aging_callback_register(switch_object_id_t device_handle) {
  (void)device_handle;
  switch_status_t status;

  if (feature::is_feature_set(SWITCH_FEATURE_NAT_NAPT_INDEX_INDIRECTION)) {
    status = nat_aging_callback_register_table(
        smi_id::T_INGRESS_NAT_DNAPT_INDEX, dnapt_aging_nofity_cb);
  } else {
    status = nat_aging_callback_register_table(smi_id::T_INGRESS_NAT_DEST_NAPT,
                                               dnapt_aging_nofity_cb);
  }
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: failed to register callback",
               __func__,
               __LINE__);
    return status;
  }

  if (feature::is_feature_set(SWITCH_FEATURE_NAT_NAPT_INDEX_INDIRECTION)) {
    status = nat_aging_callback_register_table(
        smi_id::T_INGRESS_NAT_SNAPT_INDEX, snapt_aging_nofity_cb);
  } else {
    status = nat_aging_callback_register_table(smi_id::T_INGRESS_NAT_SNAPT,
                                               snapt_aging_nofity_cb);
  }
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: failed to register callback",
               __func__,
               __LINE__);
    return status;
  }

  status = nat_aging_callback_register_table(smi_id::T_INGRESS_NAT_DEST_NAT,
                                             dnat_aging_nofity_cb);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: failed to register callback",
               __func__,
               __LINE__);
    return status;
  }

  status = nat_aging_callback_register_table(smi_id::T_INGRESS_NAT_SNAT,
                                             snat_aging_nofity_cb);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: failed to register callback",
               __func__,
               __LINE__);
    return status;
  }

  return SWITCH_STATUS_SUCCESS;
}

switch_status_t nat_init() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  status |= switch_store::reg_create_trigs_before(SWITCH_OBJECT_TYPE_NAT_ENTRY,
                                                  &before_nat_entry_create);
  status |= switch_store::reg_delete_trigs_after(SWITCH_OBJECT_TYPE_NAT_ENTRY,
                                                 &after_nat_entry_delete);
  REGISTER_OBJECT(rewrite_nat, SWITCH_OBJECT_TYPE_REWRITE_NAT);
  REGISTER_OBJECT(nat_factory, SWITCH_OBJECT_TYPE_NAT_FACTORY);

  return status;
}

}  // namespace smi
