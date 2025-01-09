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
#include <tofino/bf_pal/bf_pal_port_intf.h>
#include <tofino/pdfixed/pd_common.h>
#include <tofino/pdfixed/pd_tm.h>
#include <tofino/bf_pal/pltfm_intf.h>
#include <traffic_mgr/traffic_mgr_counters.h>
}

#include <vector>
#include <set>
#include <memory>
#include <utility>
#include <unordered_map>

#include "switch_tna/utils.h"
#include "common/qos_pdfixed.h"
#include "switch_tna/p4_16_types.h"
#include "common/bfrt_tm.h"

namespace smi {
using namespace smi::bf_rt;  // NOLINT(build/namespaces)
using ::smi::logging::switch_log;

inline bool dscp_tc_map_entry_type(switch_qos_map_ingress_attr_type type) {
  if (type == SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_DSCP_TO_TC ||
      type == SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_DSCP_TO_COLOR ||
      type == SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_DSCP_TO_TC_AND_COLOR ||
      type == SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_TOS_TO_TC ||
      type == SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_TOS_TO_COLOR ||
      type == SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_TOS_TO_TC_AND_COLOR) {
    return true;
  }
  return false;
}

inline bool l3_qos_egress_map_entry_type(switch_qos_map_egress_attr_type type) {
  if (type == SWITCH_QOS_MAP_EGRESS_ATTR_TYPE_TC_TO_DSCP ||
      type == SWITCH_QOS_MAP_EGRESS_ATTR_TYPE_COLOR_TO_DSCP ||
      type == SWITCH_QOS_MAP_EGRESS_ATTR_TYPE_TC_AND_COLOR_TO_DSCP) {
    return true;
  }
  return false;
}

inline bool l2_qos_egress_map_entry_type(switch_qos_map_egress_attr_type type) {
  if (type == SWITCH_QOS_MAP_EGRESS_ATTR_TYPE_TC_TO_PCP ||
      type == SWITCH_QOS_MAP_EGRESS_ATTR_TYPE_COLOR_TO_PCP ||
      type == SWITCH_QOS_MAP_EGRESS_ATTR_TYPE_TC_AND_COLOR_TO_PCP) {
    return true;
  }
  return false;
}

// Update is a problem. If an entirely new list is presented as an update,
// the framework has no idea what the previous values are. Maybe we need a
// temporary store for the list
class dscp_tc_map : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_DSCP_TC_MAP;
  static const switch_attr_id_t status_attr_id = SWITCH_DSCP_TC_MAP_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DSCP_TC_MAP_ATTR_PARENT_HANDLE;
  uint16_t dev_id = 0;
  uint16_t dev_port = 0;
  switch_object_id_t dev_hdl = {};

 public:
  dscp_tc_map(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_DSCP_TC_MAP,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    switch_object_id_t qos_map_ingress = {0};

    status |= switch_store::v_get(
        parent, SWITCH_PORT_ATTR_INGRESS_QOS_DSCP_TOS_GROUP, qos_map_ingress);
    if (qos_map_ingress.data == 0) {
      return;
    }

    switch_enum_t e = {0};
    std::vector<switch_object_id_t> qos_map_list;
    switch_qos_map_ingress_attr_type type;

    status |= switch_store::v_get(
        qos_map_ingress, SWITCH_QOS_MAP_INGRESS_ATTR_TYPE, e);
    type = static_cast<switch_qos_map_ingress_attr_type>(e.enumdata);

    if (dscp_tc_map_entry_type(type) == false) {
      return;
    }

    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_DEVICE, dev_hdl);
    status |= switch_store::v_get(dev_hdl, SWITCH_DEVICE_ATTR_DEV_ID, dev_id);
    device_tgt_set(
        compute_dev_target_for_table(dev_port, smi_id::T_DSCP_TC_MAP, true));

    status |= switch_store::v_get(qos_map_ingress,
                                  SWITCH_QOS_MAP_INGRESS_ATTR_QOS_MAP_LIST,
                                  qos_map_list);

    auto it = match_action_list.begin();
    for (auto const qos_map : qos_map_list) {
      uint8_t dscp = 0, color = 0, tos = 0;
      uint16_t tc = 0;
      switch_enum_t _color = {0};

      // Get the data from the qos_map handle
      status |= switch_store::v_get(qos_map, SWITCH_QOS_MAP_ATTR_DSCP, dscp);
      status |= switch_store::v_get(qos_map, SWITCH_QOS_MAP_ATTR_TOS, tos);
      status |= switch_store::v_get(qos_map, SWITCH_QOS_MAP_ATTR_TC, tc);
      status |= switch_store::v_get(qos_map, SWITCH_QOS_MAP_ATTR_COLOR, _color);
      color = static_cast<uint8_t>(_color.enumdata);
      if (color == SWITCH_QOS_MAP_ATTR_COLOR_RED) color = SWITCH_PKT_COLOR_RED;

      // Initialize entry and set match fields
      it = match_action_list.insert(it,
                                    std::pair<_MatchKey, _ActionEntry>(
                                        _MatchKey(smi_id::T_DSCP_TC_MAP),
                                        _ActionEntry(smi_id::T_DSCP_TC_MAP)));
      status |=
          it->first.set_exact(smi_id::F_DSCP_TC_MAP_QOS_INGRESS_PORT, dev_port);

      if (type == SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_DSCP_TO_TC ||
          type == SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_DSCP_TO_COLOR ||
          type == SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_DSCP_TO_TC_AND_COLOR) {
        status |= it->first.set_exact(smi_id::F_DSCP_TC_MAP_LKP_IP_DSCP, dscp);
      }

      if (type == SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_TOS_TO_TC ||
          type == SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_TOS_TO_COLOR ||
          type == SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_TOS_TO_TC_AND_COLOR) {
        tos = tos >> 2;  // program bit 7-2, ignore bit 1-0
        status |= it->first.set_exact(smi_id::F_DSCP_TC_MAP_LKP_IP_DSCP, tos);
      }

      // Set the action and paramters based on ingress qos map type
      if (type == SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_DSCP_TO_TC ||
          type == SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_TOS_TO_TC) {
        it->second.init_action_data(smi_id::A_DSCP_TC_MAP_SET_INGRESS_TC);
        status |=
            it->second.set_arg(smi_id::F_DSCP_TC_MAP_SET_INGRESS_TC_TC, tc);
      }

      if (type == SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_DSCP_TO_COLOR ||
          type == SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_TOS_TO_COLOR) {
        it->second.init_action_data(smi_id::A_DSCP_TC_MAP_SET_INGRESS_COLOR);
        status |= it->second.set_arg(
            smi_id::F_DSCP_TC_MAP_SET_INGRESS_COLOR_COLOR, color);
      }

      if (type == SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_DSCP_TO_TC_AND_COLOR ||
          type == SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_TOS_TO_TC_AND_COLOR) {
        it->second.init_action_data(
            smi_id::A_DSCP_TC_MAP_SET_INGRESS_TC_AND_COLOR);
        status |= it->second.set_arg(
            smi_id::F_DSCP_TC_MAP_SET_INGRESS_TC_AND_COLOR_TC, tc);
        status |= it->second.set_arg(
            smi_id::F_DSCP_TC_MAP_SET_INGRESS_TC_AND_COLOR_COLOR, color);
      }
    }
  }
};

inline bool pcp_tc_map_entry_type(switch_qos_map_ingress_attr_type type) {
  if (type == SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_PCP_TO_TC ||
      type == SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_PCP_TO_COLOR ||
      type == SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_PCP_TO_TC_AND_COLOR) {
    return true;
  }
  return false;
}

class pcp_tc_map : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_PCP_TC_MAP;
  static const switch_attr_id_t status_attr_id = SWITCH_PCP_TC_MAP_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_PCP_TC_MAP_ATTR_PARENT_HANDLE;
  uint16_t dev_id = 0;
  uint16_t dev_port = 0;
  switch_object_id_t dev_hdl = {};

 public:
  pcp_tc_map(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_PCP_TC_MAP,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    switch_object_id_t qos_map_ingress = {0};

    status |= switch_store::v_get(
        parent, SWITCH_PORT_ATTR_INGRESS_QOS_PCP_GROUP, qos_map_ingress);
    if (qos_map_ingress.data == 0) {
      return;
    }

    switch_enum_t e = {0};
    switch_qos_map_ingress_attr_type type;
    std::vector<switch_object_id_t> qos_map_list;

    status |= switch_store::v_get(
        qos_map_ingress, SWITCH_QOS_MAP_INGRESS_ATTR_TYPE, e);
    type = static_cast<switch_qos_map_ingress_attr_type>(e.enumdata);

    if (pcp_tc_map_entry_type(type) == false) {
      return;
    }

    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_DEVICE, dev_hdl);
    status |= switch_store::v_get(dev_hdl, SWITCH_DEVICE_ATTR_DEV_ID, dev_id);
    device_tgt_set(
        compute_dev_target_for_table(dev_port, smi_id::T_PCP_TC_MAP, true));

    status |= switch_store::v_get(qos_map_ingress,
                                  SWITCH_QOS_MAP_INGRESS_ATTR_QOS_MAP_LIST,
                                  qos_map_list);

    auto it = match_action_list.begin();
    for (auto const qos_map : qos_map_list) {
      uint8_t pcp = 0, color = 0;
      uint16_t tc = 0;
      switch_enum_t _color = {0};

      // Get the data from the qos_map handle
      status |= switch_store::v_get(qos_map, SWITCH_QOS_MAP_ATTR_PCP, pcp);
      status |= switch_store::v_get(qos_map, SWITCH_QOS_MAP_ATTR_TC, tc);
      status |= switch_store::v_get(qos_map, SWITCH_QOS_MAP_ATTR_COLOR, _color);
      color = static_cast<uint8_t>(_color.enumdata);
      if (color == SWITCH_QOS_MAP_ATTR_COLOR_RED) color = SWITCH_PKT_COLOR_RED;

      // Initialize entry and set match fields
      it = match_action_list.insert(it,
                                    std::pair<_MatchKey, _ActionEntry>(
                                        _MatchKey(smi_id::T_PCP_TC_MAP),
                                        _ActionEntry(smi_id::T_PCP_TC_MAP)));

      status |=
          it->first.set_exact(smi_id::F_PCP_TC_MAP_QOS_INGRESS_PORT, dev_port);
      status |= it->first.set_exact(smi_id::F_PCP_TC_MAP_LKP_PCP, pcp);

      // Set the action and paramters based on ingress qos map type
      if (type == SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_PCP_TO_TC) {
        it->second.init_action_data(smi_id::A_PCP_TC_MAP_SET_INGRESS_TC);
        status |=
            it->second.set_arg(smi_id::F_PCP_TC_MAP_SET_INGRESS_TC_TC, tc);
      }

      if (type == SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_PCP_TO_COLOR) {
        it->second.init_action_data(smi_id::A_PCP_TC_MAP_SET_INGRESS_COLOR);
        status |= it->second.set_arg(
            smi_id::F_PCP_TC_MAP_SET_INGRESS_COLOR_COLOR, color);
      }

      if (type == SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_PCP_TO_TC_AND_COLOR) {
        it->second.init_action_data(
            smi_id::A_PCP_TC_MAP_SET_INGRESS_TC_AND_COLOR);
        status |= it->second.set_arg(
            smi_id::F_PCP_TC_MAP_SET_INGRESS_TC_AND_COLOR_TC, tc);
        status |= it->second.set_arg(
            smi_id::F_PCP_TC_MAP_SET_INGRESS_TC_AND_COLOR_COLOR, color);
      }
    }
  }
};

inline bool tc_map_entry_type(switch_qos_map_ingress_attr_type type) {
  if (type == SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_TC_TO_ICOS ||
      type == SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_TC_TO_QUEUE ||
      type == SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_TC_TO_ICOS_AND_QUEUE) {
    return true;
  }
  return false;
}

std::unordered_map<uint8_t, std::pair<switch_object_id_t, switch_object_id_t>>
    tc_map;
class traffic_class : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_TRAFFIC_CLASS;
  static const switch_attr_id_t status_attr_id =
      SWITCH_TRAFFIC_CLASS_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_TRAFFIC_CLASS_ATTR_PARENT_HANDLE;
  switch_qos_map_ingress_attr_type type;
  std::vector<switch_object_id_t> qos_map_list;
  static const bool auto_cache = false;
  std::vector<uint8_t> tc_cache;

 public:
  traffic_class(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_TRAFFIC_CLASS,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent,
                                    auto_cache) {
    status = SWITCH_STATUS_SUCCESS;

    switch_enum_t e = {0};

    status |= switch_store::v_get(parent, SWITCH_QOS_MAP_INGRESS_ATTR_TYPE, e);
    type = static_cast<switch_qos_map_ingress_attr_type>(e.enumdata);
    status |= switch_store::v_get(
        parent, SWITCH_QOS_MAP_INGRESS_ATTR_QOS_MAP_LIST, qos_map_list);
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_object_id_t empty = {};
    std::vector<bool> bf_rt_status;

    if (tc_map_entry_type(type) == false) {
      return status;
    }

    auto it = match_action_list.begin();
    for (auto const qos_map : qos_map_list) {
      uint8_t qid = 0, icos = 0, color = 0, color_mask = 0;
      uint16_t tc = 0;
      bool update = false;
      switch_enum_t _color = {0};

      // Get the data from the qos_map handle
      status |= switch_store::v_get(qos_map, SWITCH_QOS_MAP_ATTR_TC, tc);
      status |= switch_store::v_get(qos_map, SWITCH_QOS_MAP_ATTR_ICOS, icos);
      status |= switch_store::v_get(qos_map, SWITCH_QOS_MAP_ATTR_QID, qid);
      status |= switch_store::v_get(qos_map, SWITCH_QOS_MAP_ATTR_COLOR, _color);
      color = static_cast<uint8_t>(_color.enumdata);
      if (color == SWITCH_QOS_MAP_ATTR_COLOR_RED) color = SWITCH_PKT_COLOR_RED;
      // set mask for any color other than green(0)
      if (color != 0) color_mask = 0x3;

      it = match_action_list.insert(it,
                                    std::pair<_MatchKey, _ActionEntry>(
                                        _MatchKey(smi_id::T_TRAFFIC_CLASS),
                                        _ActionEntry(smi_id::T_TRAFFIC_CLASS)));
      status |= it->first.set_exact(smi_id::F_TRAFFIC_CLASS_QOS_MD_TC, tc);
      status |= it->first.set_ternary(
          smi_id::F_TRAFFIC_CLASS_QOS_MD_COLOR, color, color_mask);

      if (tc_map.find(tc) != tc_map.end()) {
        auto tc_qid_pair = tc_map[tc];
        switch_object_id_t tc_icos_map = tc_qid_pair.first;
        switch_object_id_t tc_qid_map = tc_qid_pair.second;
        if (type == SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_TC_TO_ICOS &&
            tc_qid_map.data != 0) {
          // QID is set for this tc, get qid and change action with new icos
          status |=
              switch_store::v_get(tc_qid_map, SWITCH_QOS_MAP_ATTR_QID, qid);
          it->second.init_action_data(
              smi_id::A_TRAFFIC_CLASS_SET_ICOS_AND_QUEUE);
          status |= it->second.set_arg(
              smi_id::F_TRAFFIC_CLASS_SET_ICOS_AND_QUEUE_ICOS, icos);
          status |= it->second.set_arg(
              smi_id::F_TRAFFIC_CLASS_SET_ICOS_AND_QUEUE_QID, qid);
          tc_map[tc] = std::make_pair(qos_map, tc_qid_map);
          update = true;
          bf_rt_status.push_back(update);
        }
        if (type == SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_TC_TO_QUEUE &&
            tc_icos_map.data != 0) {
          // ICOS is set for this tc, get icos and change action with new qid
          status |=
              switch_store::v_get(tc_icos_map, SWITCH_QOS_MAP_ATTR_ICOS, icos);
          it->second.init_action_data(
              smi_id::A_TRAFFIC_CLASS_SET_ICOS_AND_QUEUE);
          status |= it->second.set_arg(
              smi_id::F_TRAFFIC_CLASS_SET_ICOS_AND_QUEUE_QID, qid);
          status |= it->second.set_arg(
              smi_id::F_TRAFFIC_CLASS_SET_ICOS_AND_QUEUE_ICOS, icos);
          tc_map[tc] = std::make_pair(tc_icos_map, qos_map);
          update = true;
          bf_rt_status.push_back(update);
        }
        if (type == SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_TC_TO_ICOS_AND_QUEUE &&
            (tc_icos_map.data != 0 || tc_qid_map.data != 0)) {
          switch_log(
              SWITCH_API_LEVEL_ERROR,
              SWITCH_OBJECT_TYPE_QOS_MAP_INGRESS,
              "{}.{}: qos map ingress creation failed, only one global tc "
              "to icos/queue map table is allowed, type {}, tc {} icos {} qid "
              "{}",
              "traffic_class",
              __LINE__,
              type,
              tc,
              icos,
              qid);
          status |= SWITCH_STATUS_FAILURE;
          return status;
        }
      } else {
        if (type == SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_TC_TO_ICOS) {
          it->second.init_action_data(smi_id::A_TRAFFIC_CLASS_SET_ICOS);
          status |=
              it->second.set_arg(smi_id::F_TRAFFIC_CLASS_SET_ICOS_ICOS, icos);
          tc_map[tc] = std::make_pair(qos_map, empty);
          bf_rt_status.push_back(false);
        } else if (type == SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_TC_TO_QUEUE) {
          it->second.init_action_data(smi_id::A_TRAFFIC_CLASS_SET_QID);
          status |=
              it->second.set_arg(smi_id::F_TRAFFIC_CLASS_SET_QID_QID, qid);
          tc_map[tc] = std::make_pair(empty, qos_map);
          bf_rt_status.push_back(false);
        } else if (type ==
                   SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_TC_TO_ICOS_AND_QUEUE) {
          it->second.init_action_data(
              smi_id::A_TRAFFIC_CLASS_SET_ICOS_AND_QUEUE);
          status |= it->second.set_arg(
              smi_id::F_TRAFFIC_CLASS_SET_ICOS_AND_QUEUE_ICOS, icos);
          status |= it->second.set_arg(
              smi_id::F_TRAFFIC_CLASS_SET_ICOS_AND_QUEUE_QID, qid);
          tc_map[tc] = std::make_pair(qos_map, qos_map);
          bf_rt_status.push_back(false);
        }
      }
      it++;
      tc_cache.push_back(tc);
    }

    if (switch_store::smiContext::context().in_warm_init()) {
      add_to_cache();
    } else {
      // Let infra know which entries are new and which need to be updated
      auto_obj.create_update();
      attr_w attr(status_attr_id);
      attr.v_set(bf_rt_status);
      status |= switch_store::attribute_set(auto_obj.get_auto_oid(), attr);

      status |= p4_object_match_action_list::create_update();
    }
    return status;
  }

  switch_status_t add_to_cache() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    auto it = match_action_list.begin();
    std::unordered_map<uint8_t, bfrtCacheObject> &cache =
        bfrtCache::cache().traffic_class_cache();
    for (auto &tc : tc_cache) {
      if (cache.find(tc) != cache.end()) {
        cache.erase(tc);
      }
      _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_TRAFFIC_CLASS);
      cache[tc] =
          table.create_cache_object(get_dev_tgt(), it->first, it->second);
      ++it;
    }
    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_object_id_t empty = {};

    if (tc_map_entry_type(type) == false) {
      return status;
    }

    for (auto const qos_map : qos_map_list) {
      uint8_t qid = 0, icos = 0, color = 0, color_mask = 0;
      uint16_t tc = 0;
      bool update = false;
      switch_enum_t _color = {0};

      // Get the data from the qos_map handle
      status |= switch_store::v_get(qos_map, SWITCH_QOS_MAP_ATTR_TC, tc);
      status |= switch_store::v_get(qos_map, SWITCH_QOS_MAP_ATTR_ICOS, icos);
      status |= switch_store::v_get(qos_map, SWITCH_QOS_MAP_ATTR_QID, qid);
      status |= switch_store::v_get(qos_map, SWITCH_QOS_MAP_ATTR_COLOR, _color);
      color = static_cast<uint8_t>(_color.enumdata);
      if (color == SWITCH_QOS_MAP_ATTR_COLOR_RED) color = SWITCH_PKT_COLOR_RED;
      // set mask for any color other than green(0)
      if (color != 0) color_mask = 0x3;

      if (tc_map.find(tc) == tc_map.end()) {
        continue;
      }

      _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_TRAFFIC_CLASS);
      _ActionEntry action_entry(smi_id::T_TRAFFIC_CLASS);
      _MatchKey match_key(smi_id::T_TRAFFIC_CLASS);
      status |= match_key.set_exact(smi_id::F_TRAFFIC_CLASS_QOS_MD_TC, tc);
      status |= match_key.set_ternary(
          smi_id::F_TRAFFIC_CLASS_QOS_MD_COLOR, color, color_mask);

      auto tc_qid_pair = tc_map[tc];
      switch_object_id_t tc_icos_map = tc_qid_pair.first;
      switch_object_id_t tc_qid_map = tc_qid_pair.second;
      if (type == SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_TC_TO_ICOS &&
          tc_qid_map.data != 0) {
        // QID is set for this tc, get qid and change action to set_qid
        action_entry.init_action_data(smi_id::A_TRAFFIC_CLASS_SET_QID);
        status |= action_entry.set_arg(smi_id::F_TRAFFIC_CLASS_SET_QID_QID,
                                       tc_qid_map,
                                       SWITCH_QOS_MAP_ATTR_QID);
        tc_map[tc] = std::make_pair(empty, tc_qid_map);
        update = true;
      }
      if (type == SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_TC_TO_QUEUE &&
          tc_icos_map.data != 0) {
        // ICOS is set for this tc, get icos and change action with new qid
        action_entry.init_action_data(smi_id::A_TRAFFIC_CLASS_SET_ICOS);
        status |= action_entry.set_arg(smi_id::F_TRAFFIC_CLASS_SET_ICOS_ICOS,
                                       tc_icos_map,
                                       SWITCH_QOS_MAP_ATTR_ICOS);
        tc_map[tc] = std::make_pair(tc_icos_map, empty);
        update = true;
      }

      if (update) {
        status |= table.entry_modify(match_key, action_entry);
      } else {
        status |= table.entry_delete(match_key);
        tc_map.erase(tc);
      }
    }
    status |= auto_obj.del();
    return status;
  }
};

class l3_qos_map_table : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_L3_QOS_MAP_TABLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_L3_QOS_MAP_TABLE_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_L3_QOS_MAP_TABLE_ATTR_PARENT_HANDLE;
  uint16_t dev_id = 0;
  uint16_t dev_port = 0;
  switch_object_id_t dev_hdl = {};

 public:
  l3_qos_map_table(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_L3_QOS_MAP,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    switch_object_id_t qos_map_egress = {0};
    switch_enum_t e = {0};
    switch_qos_map_egress_attr_type type;
    status |= switch_store::v_get(
        parent, SWITCH_PORT_ATTR_EGRESS_L3_QOS_GROUP, qos_map_egress);

    if (qos_map_egress.data == 0) {
      return;
    }

    status |=
        switch_store::v_get(qos_map_egress, SWITCH_QOS_MAP_EGRESS_ATTR_TYPE, e);
    type = static_cast<switch_qos_map_egress_attr_type>(e.enumdata);
    if (l3_qos_egress_map_entry_type(type) == false) {
      return;
    }

    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_DEVICE, dev_hdl);
    status |= switch_store::v_get(dev_hdl, SWITCH_DEVICE_ATTR_DEV_ID, dev_id);
    device_tgt_set(
        compute_dev_target_for_table(dev_port, smi_id::T_L3_QOS_MAP, true));
    std::vector<switch_object_id_t> qos_map_list;
    status |= switch_store::v_get(
        qos_map_egress, SWITCH_QOS_MAP_EGRESS_ATTR_QOS_MAP_LIST, qos_map_list);

    auto it = match_action_list.begin();
    for (auto const qos_map : qos_map_list) {
      uint8_t dscp = 0, color = 0;
      uint16_t tc = 0;
      switch_enum_t _color = {0};

      status |= switch_store::v_get(qos_map, SWITCH_QOS_MAP_ATTR_TC, tc);
      status |= switch_store::v_get(qos_map, SWITCH_QOS_MAP_ATTR_COLOR, _color);
      color = static_cast<uint8_t>(_color.enumdata);
      if (color == SWITCH_QOS_MAP_ATTR_COLOR_RED) color = SWITCH_PKT_COLOR_RED;
      status |= switch_store::v_get(qos_map, SWITCH_QOS_MAP_ATTR_DSCP, dscp);

      for (int i = 0; i < 2; i++) {
        it = match_action_list.insert(it,
                                      std::pair<_MatchKey, _ActionEntry>(
                                          _MatchKey(smi_id::T_L3_QOS_MAP),
                                          _ActionEntry(smi_id::T_L3_QOS_MAP)));
        status |= it->first.set_exact(smi_id::F_L3_QOS_MAP_PORT, dev_port);

        if (i == 0)
          status |=
              it->first.set_exact(smi_id::F_L3_QOS_MAP_HDR_IPV4_VALID, true);
        else
          status |=
              it->first.set_exact(smi_id::F_L3_QOS_MAP_HDR_IPV6_VALID, true);

        if (type == SWITCH_QOS_MAP_EGRESS_ATTR_TYPE_TC_TO_DSCP) {
          status |= it->first.set_exact(smi_id::F_L3_QOS_MAP_MD_TC, tc);
        }
        if (type == SWITCH_QOS_MAP_EGRESS_ATTR_TYPE_COLOR_TO_DSCP) {
          status |= it->first.set_exact(smi_id::F_L3_QOS_MAP_MD_COLOR, color);
        }
        if (type == SWITCH_QOS_MAP_EGRESS_ATTR_TYPE_TC_AND_COLOR_TO_DSCP) {
          status |= it->first.set_exact(smi_id::F_L3_QOS_MAP_MD_TC, tc);
          status |= it->first.set_exact(smi_id::F_L3_QOS_MAP_MD_COLOR, color);
        }

        if (type == SWITCH_QOS_MAP_EGRESS_ATTR_TYPE_TC_TO_DSCP ||
            type == SWITCH_QOS_MAP_EGRESS_ATTR_TYPE_COLOR_TO_DSCP ||
            type == SWITCH_QOS_MAP_EGRESS_ATTR_TYPE_TC_AND_COLOR_TO_DSCP) {
          if (i == 0) {
            it->second.init_action_data(smi_id::A_L3_QOS_MAP_SET_IPV4_DSCP);
            status |= it->second.set_arg(
                smi_id::F_L3_QOS_MAP_SET_IPV4_DSCP_DSCP, dscp);
          } else {
            it->second.init_action_data(smi_id::A_L3_QOS_MAP_SET_IPV6_DSCP);
            status |= it->second.set_arg(
                smi_id::F_L3_QOS_MAP_SET_IPV6_DSCP_DSCP, dscp);
          }
        }
      }
    }
  }
};

class l2_qos_map_table : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_L2_QOS_MAP_TABLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_L2_QOS_MAP_TABLE_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_L2_QOS_MAP_TABLE_ATTR_PARENT_HANDLE;
  uint16_t dev_id = 0;
  uint16_t dev_port = 0;
  switch_object_id_t dev_hdl = {};

 public:
  l2_qos_map_table(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_L2_QOS_MAP,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    switch_object_id_t qos_map_egress = {0};
    switch_enum_t e = {0};
    switch_qos_map_egress_attr_type type;
    status |= switch_store::v_get(
        parent, SWITCH_PORT_ATTR_EGRESS_L2_QOS_GROUP, qos_map_egress);

    if (qos_map_egress.data == 0) {
      return;
    }

    status |=
        switch_store::v_get(qos_map_egress, SWITCH_QOS_MAP_EGRESS_ATTR_TYPE, e);
    type = static_cast<switch_qos_map_egress_attr_type>(e.enumdata);
    if (l2_qos_egress_map_entry_type(type) == false) {
      return;
    }

    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_DEVICE, dev_hdl);
    status |= switch_store::v_get(dev_hdl, SWITCH_DEVICE_ATTR_DEV_ID, dev_id);
    device_tgt_set(
        compute_dev_target_for_table(dev_port, smi_id::T_L2_QOS_MAP, true));
    std::vector<switch_object_id_t> qos_map_list;
    status |= switch_store::v_get(
        qos_map_egress, SWITCH_QOS_MAP_EGRESS_ATTR_QOS_MAP_LIST, qos_map_list);

    auto it = match_action_list.begin();
    for (auto const qos_map : qos_map_list) {
      uint8_t pcp = 0, color = 0;
      uint16_t tc = 0;
      switch_enum_t _color = {0};

      status |= switch_store::v_get(qos_map, SWITCH_QOS_MAP_ATTR_TC, tc);
      status |= switch_store::v_get(qos_map, SWITCH_QOS_MAP_ATTR_COLOR, _color);
      color = static_cast<uint8_t>(_color.enumdata);
      if (color == SWITCH_QOS_MAP_ATTR_COLOR_RED) color = SWITCH_PKT_COLOR_RED;
      status |= switch_store::v_get(qos_map, SWITCH_QOS_MAP_ATTR_PCP, pcp);

      it = match_action_list.insert(it,
                                    std::pair<_MatchKey, _ActionEntry>(
                                        _MatchKey(smi_id::T_L2_QOS_MAP),
                                        _ActionEntry(smi_id::T_L2_QOS_MAP)));
      status |= it->first.set_exact(smi_id::F_L2_QOS_MAP_PORT, dev_port);

      if (type == SWITCH_QOS_MAP_EGRESS_ATTR_TYPE_TC_TO_PCP) {
        status |= it->first.set_exact(smi_id::F_L2_QOS_MAP_MD_TC, tc);
      }
      if (type == SWITCH_QOS_MAP_EGRESS_ATTR_TYPE_COLOR_TO_PCP) {
        status |= it->first.set_exact(smi_id::F_L2_QOS_MAP_MD_COLOR, color);
      }
      if (type == SWITCH_QOS_MAP_EGRESS_ATTR_TYPE_TC_AND_COLOR_TO_PCP) {
        status |= it->first.set_exact(smi_id::F_L2_QOS_MAP_MD_TC, tc);
        status |= it->first.set_exact(smi_id::F_L2_QOS_MAP_MD_COLOR, color);
      }
      if (type == SWITCH_QOS_MAP_EGRESS_ATTR_TYPE_TC_TO_PCP ||
          type == SWITCH_QOS_MAP_EGRESS_ATTR_TYPE_COLOR_TO_PCP ||
          type == SWITCH_QOS_MAP_EGRESS_ATTR_TYPE_TC_AND_COLOR_TO_PCP) {
        it->second.init_action_data(smi_id::A_L2_QOS_MAP_SET_VLAN_PCP);
        status |=
            it->second.set_arg(smi_id::F_L2_QOS_MAP_SET_VLAN_PCP_PCP, pcp);
      }
    }
  }
};

class port_ppg_stats : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_PORT_PPG_STATS;
  static const switch_attr_id_t status_attr_id =
      SWITCH_PORT_PPG_STATS_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_PORT_PPG_STATS_ATTR_PARENT_HANDLE;

 public:
  port_ppg_stats(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action_list(
            smi_id::T_PPG, status_attr_id, auto_ot, parent_attr_id, parent) {
    uint16_t dev_port = 0;
    uint64_t pkts = 0;
    uint64_t bytes = 0;
    switch_enum_t port_type = {};

    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_TYPE, port_type);
    if (port_type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL) return;

    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
    auto &switch_ingress_pipes = SWITCH_CONTEXT.get_switch_ingress_pipe_list();

    if (feature::is_feature_set(SWITCH_FEATURE_FOLDED_SWITCH_PIPELINE) &&
        std::find(switch_ingress_pipes.begin(),
                  switch_ingress_pipes.end(),
                  DEV_PORT_TO_PIPE(dev_port)) == switch_ingress_pipes.end()) {
      return;
    }
    device_tgt_set(compute_dev_target_for_table(dev_port, smi_id::T_PPG, true));

    auto it = match_action_list.begin();
    std::vector<bf_rt_id_t> cntrs{smi_id::P_PPG_STATS_BYTES,
                                  smi_id::P_PPG_STATS_PKTS};

    // create match entry for all icos values..
    for (uint8_t icos = 0; icos < SWITCH_BUFFER_PFC_ICOS_MAX; icos++) {
      it = match_action_list.insert(
          it,
          std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_PPG),
                                             _ActionEntry(smi_id::T_PPG)));

      status |=
          it->first.set_exact(smi_id::F_PPG_LOCAL_MD_INGRESS_PORT, dev_port);
      status |= it->first.set_exact(smi_id::F_PPG_IG_INTR_MD_FOR_TM_INGRESS_COS,
                                    icos);
      it->second.init_action_data(smi_id::A_PPG_COUNT, cntrs);
      it->second.set_arg(smi_id::P_PPG_STATS_PKTS, pkts);
      it->second.set_arg(smi_id::P_PPG_STATS_BYTES, bytes);
    }
  }

  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    (void)handle;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t bytes = 0, pkts = 0;
    p4_object_match_action_list::data_get();
    for (auto const &entry : match_action_list) {
      switch_counter_t cntr_pkts = {}, cntr_bytes = {};
      uint64_t icos = 0;
      entry.second.get_arg(
          smi_id::P_PPG_STATS_BYTES, smi_id::A_PPG_COUNT, &bytes);
      entry.second.get_arg(
          smi_id::P_PPG_STATS_PKTS, smi_id::A_PPG_COUNT, &pkts);
      entry.first.get_exact(smi_id::F_PPG_IG_INTR_MD_FOR_TM_INGRESS_COS, &icos);
      cntr_pkts.counter_id = cntr_bytes.counter_id = icos;
      cntr_pkts.count = pkts;
      cntr_bytes.count = bytes;
      cntrs.push_back(cntr_pkts);
      cntrs.push_back(cntr_bytes);
    }
    return status;
  }

  switch_status_t counters_set(const switch_object_id_t handle) {
    (void)handle;
    for (auto &entry : match_action_list) {
      uint64_t value = 0;
      entry.second.set_arg(smi_id::P_PPG_STATS_BYTES, value);
      entry.second.set_arg(smi_id::P_PPG_STATS_PKTS, value);
    }
    return p4_object_match_action_list::data_set();
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    (void)handle;
    p4_object_match_action_list::data_get();

    for (auto id : cntr_ids) {
      for (auto &entry : match_action_list) {
        uint64_t value = 0;
        switch (id) {
          case SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_BYTES:
            entry.second.set_arg(smi_id::P_PPG_STATS_BYTES, value);
            break;
          case SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_PACKETS:
            entry.second.set_arg(smi_id::P_PPG_STATS_PKTS, value);
            break;
          default:
            break;
        }
      }
    }
    return p4_object_match_action_list::data_set();
  }
};

static inline uint32_t switch_pd_pool_max_cells_and_baf_to_shared_cells(
    uint32_t max_cells, p4_pd_tm_ppg_baf_t dyn_baf) {
  uint32_t baf_numerator, baf_denominator;

  switch (dyn_baf) {
    case PD_PPG_BAF_1_POINT_5_PERCENT:
      baf_numerator = 1;
      baf_denominator = 65;
      break;
    case PD_PPG_BAF_3_PERCENT:
      baf_numerator = 1;
      baf_denominator = 33;
      break;
    case PD_PPG_BAF_6_PERCENT:
      baf_numerator = 1;
      baf_denominator = 17;
      break;
    case PD_PPG_BAF_11_PERCENT:
      baf_numerator = 1;
      baf_denominator = 9;
      break;
    case PD_PPG_BAF_20_PERCENT:
      baf_numerator = 1;
      baf_denominator = 5;
      break;
    case PD_PPG_BAF_33_PERCENT:
      baf_numerator = 1;
      baf_denominator = 3;
      break;
    case PD_PPG_BAF_50_PERCENT:
      baf_numerator = 1;
      baf_denominator = 2;
      break;
    case PD_PPG_BAF_66_PERCENT:
      baf_numerator = 2;
      baf_denominator = 3;
      break;
    case PD_PPG_BAF_80_PERCENT:
      baf_numerator = 4;
      baf_denominator = 5;
      break;
    default:
      baf_numerator = 0;
      baf_denominator = 1;
      break;
  }

  return ((max_cells * baf_numerator) + baf_denominator - 1) / baf_denominator;
}

class ppg_stats_helper : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_PPG_STATS_HELPER;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_PPG_STATS_HELPER_ATTR_PARENT_HANDLE;
  uint16_t dev_id = 0, dev_port = 0;
  switch_object_id_t port_handle = {}, def_ppg_handle = {}, device_handle = {};
  switch_object_id_t buffer_profile_handle = {};
  uint64_t buffer_profile_in_use = 0;
  p4_pd_tm_ppg_t pd_hdl = 0;
  bool created_in_hw = false, is_switch_pipe_port_ppg = true;
  uint64_t cache_packet_count{0}, cache_byte_count{0};

 public:
  ppg_stats_helper(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status |= switch_store::v_get(
        parent, SWITCH_PORT_PRIORITY_GROUP_ATTR_PORT_HANDLE, port_handle);
    status = switch_store::v_get(
        parent, SWITCH_PORT_PRIORITY_GROUP_ATTR_DEVICE, device_handle);
    status |= switch_store::v_get(
        parent, SWITCH_PORT_PRIORITY_GROUP_ATTR_PD_HDL, pd_hdl);
    status |= switch_store::v_get(
        parent, SWITCH_PORT_PRIORITY_GROUP_ATTR_CREATED_IN_HW, created_in_hw);
    status |= switch_store::v_get(
        parent,
        SWITCH_PORT_PRIORITY_GROUP_ATTR_BUFFER_PROFILE_IN_USE,
        buffer_profile_in_use);
    buffer_profile_handle = {buffer_profile_in_use};
    status |= switch_store::v_get(parent,
                                  SWITCH_PORT_PRIORITY_GROUP_ATTR_PACKET_COUNT,
                                  cache_packet_count);
    status |= switch_store::v_get(
        parent, SWITCH_PORT_PRIORITY_GROUP_ATTR_BYTE_COUNT, cache_byte_count);
    status |=
        switch_store::v_get(port_handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
    status |=
        switch_store::v_get(device_handle, SWITCH_DEVICE_ATTR_DEV_ID, dev_id);
    switch_store::v_get(
        port_handle, SWITCH_PORT_ATTR_DEFAULT_PPG, def_ppg_handle.data);

    auto &switch_ingress_pipes = SWITCH_CONTEXT.get_switch_ingress_pipe_list();
    if (feature::is_feature_set(SWITCH_FEATURE_FOLDED_SWITCH_PIPELINE) &&
        std::find(switch_ingress_pipes.begin(),
                  switch_ingress_pipes.end(),
                  DEV_PORT_TO_PIPE(dev_port)) == switch_ingress_pipes.end()) {
      is_switch_pipe_port_ppg = false;
    }
  }

  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    std::vector<switch_counter_t> port_icos_cntrs;
    switch_object_id_t icos_ppg_qos_map_handle = {};
    switch_object_id_t buffer_pool_handle = {}, buffer_pool_helper_handle = {};
    uint32_t gmin_cells = 0, shared_cells = 0, skid_cells = 0, wm_cells = 0;
    uint32_t gmin_max_cells = 0, shared_max_cells = 0, skid_max_cells = 0;
    uint32_t xon_threshold_cells = 0, max_cells = 0;
    uint64_t cell_size = 0;
    p4_pd_status_t pd_status = 0;
    p4_pd_pool_id_t pool_id;
    uint16_t pool_id_tmp;
    p4_pd_tm_ppg_baf_t dyn_baf;

    switch_enum_t port_type = {.enumdata = SWITCH_PORT_ATTR_TYPE_NORMAL};

    status |=
        switch_store::v_get(port_handle, SWITCH_PORT_ATTR_TYPE, port_type);
    if (port_type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL) {
      return status;
    }

    switch_counter_t ppg_counters_pkts = {
        .counter_id = SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_PACKETS,
        .count = cache_packet_count};
    switch_counter_t ppg_counters_bytes = {
        .counter_id = SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_BYTES,
        .count = cache_byte_count};
    switch_counter_t dropped_pkts = {
        .counter_id = SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_DROPPED_PACKETS,
        .count = 0};
    switch_counter_t wm_bytes = {
        .counter_id = SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_WATERMARK_BYTES,
        .count = 0};
    switch_counter_t shared_occ_bytes = {
        .counter_id =
            SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_SHARED_CURR_OCCUPANCY_BYTES,
        .count = 0};
    switch_counter_t occ_bytes = {
        .counter_id =
            SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_CURR_OCCUPANCY_BYTES,
        .count = 0};
    switch_counter_t skid_occ_bytes = {
        .counter_id =
            SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_SKID_CURR_OCCUPANCY_BYTES,
        .count = 0};
    switch_counter_t gmin_max_bytes = {
        .counter_id = SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_GUARANTEED_BYTES,
        .count = 0};
    switch_counter_t shared_max_bytes = {
        .counter_id =
            SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_SHARED_MAX_OCCUPANCY_BYTES,
        .count = 0};
    switch_counter_t skid_max_bytes = {
        .counter_id =
            SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_SKID_MAX_OCCUPANCY_BYTES,
        .count = 0};
    switch_counter_t shared_wm_bytes = {
        .counter_id =
            SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_SHARED_WATERMARK_BYTES,
        .count = 0};

    uint64_t pfc_wd_pkt_cnt = 0;
    uint64_t pfc_wd_bytes_cnt = 0;
    if (is_switch_pipe_port_ppg) {
      port_ppg_stats port_ppg_stats_helper(port_handle, status);
      status |=
          port_ppg_stats_helper.counters_get(port_handle, port_icos_cntrs);

      status |= switch_store::v_get(port_handle,
                                    SWITCH_PORT_ATTR_ICOS_PPG_QOS_MAP_HANDLE,
                                    icos_ppg_qos_map_handle);

      std::set<uint8_t> ppg_icos_qos_map;
      status |= ppg_icos_qos_map_get(handle, ppg_icos_qos_map);

      for (auto icos : ppg_icos_qos_map) {
        ppg_counters_pkts.count +=
            port_icos_cntrs[(SWITCH_BUFFER_PFC_ICOS_MAX - 1 - icos) * 2].count;
        ppg_counters_bytes.count +=
            port_icos_cntrs[(SWITCH_BUFFER_PFC_ICOS_MAX - 1 - icos) * 2 + 1]
                .count;
      }

      std::set<switch_object_id_t> pfc_wd_list;
      status |= pfc_wd_list_get(ppg_icos_qos_map, pfc_wd_list);

      // Now get PFC WD counters
      // as the packets dropped by PFC WD will be counted there instead of PPG.
      for (auto pfc_wd : pfc_wd_list) {
        std::vector<switch_counter_t> pfc_wd_cntrs;
        status = switch_store::object_counters_get(pfc_wd, pfc_wd_cntrs);
        if (status == SWITCH_STATUS_SUCCESS) {
          for (auto counter : pfc_wd_cntrs) {
            if (counter.counter_id == SWITCH_PFC_WD_COUNTER_ID_PACKETS) {
              pfc_wd_pkt_cnt += counter.count;
            } else {
              pfc_wd_bytes_cnt += counter.count;
            }
          }
        }
      }
    }

    if (created_in_hw) {
      _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_TM_CFG);
      _ActionEntry default_entry(smi_id::T_TM_CFG);

      default_entry.init_indirect_data();

      status = table.default_entry_get(default_entry);

      status |= default_entry.get_arg(smi_id::D_TM_CFG_CELL_SIZE, &cell_size);
      if (status != 0) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_PORT_PRIORITY_GROUP,
                   "{}.{}:{}: Failed to get cell_size status {}",
                   "ppg_stats_helper",
                   __func__,
                   __LINE__,
                   status);
        return SWITCH_STATUS_FAILURE;
      }

      pd_status = p4_pd_tm_get_ppg_guaranteed_min_limit(
          dev_id, pd_hdl, &gmin_max_cells);
      if (pd_status != 0) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_PORT_PRIORITY_GROUP,
                   "{}.{}:{}: Failed to get tm ppg gmin limit pd_status {}",
                   "ppg_stats_helper",
                   __func__,
                   __LINE__,
                   pd_status);
        return SWITCH_STATUS_FAILURE;
      }
      gmin_max_bytes.count = gmin_max_cells * cell_size;

      if (buffer_profile_handle.data != 0) {
        status |=
            switch_store::v_get(buffer_profile_handle,
                                SWITCH_BUFFER_PROFILE_ATTR_BUFFER_POOL_HANDLE,
                                buffer_pool_handle);
        status |= find_auto_oid(buffer_pool_handle,
                                SWITCH_OBJECT_TYPE_BUFFER_POOL_HELPER,
                                buffer_pool_helper_handle);
        if (buffer_pool_helper_handle.data == 0) {
          pool_id = static_cast<p4_pd_pool_id_t>(PD_INGRESS_POOL_0);
        } else {
          status |= switch_store::v_get(buffer_pool_helper_handle,
                                        SWITCH_BUFFER_POOL_HELPER_ATTR_POOL_ID,
                                        pool_id_tmp);
          if (status != SWITCH_STATUS_SUCCESS) {
            switch_log(SWITCH_API_LEVEL_ERROR,
                       SWITCH_OBJECT_TYPE_PORT_PRIORITY_GROUP,
                       "{}.{}:{}: Failed to get buffer pool for ppg status {}",
                       "ppg_stats_helper",
                       __func__,
                       __LINE__,
                       switch_error_to_string(status));
            return SWITCH_STATUS_FAILURE;
          }
          pool_id = static_cast<p4_pd_pool_id_t>(pool_id_tmp);
        }
      } else {
        pool_id = static_cast<p4_pd_pool_id_t>(PD_INGRESS_POOL_0);
      }

      pd_status = p4_pd_tm_get_ppg_app_pool_usage(dev_id,
                                                  pd_hdl,
                                                  pool_id,
                                                  &shared_max_cells,
                                                  &dyn_baf,
                                                  &xon_threshold_cells);
      if (pd_status != 0) {
        status = SWITCH_STATUS_FAILURE;
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_PORT_PRIORITY_GROUP,
                   "{}.{}:{}: Failed to get tm ppg baf pd_status {}",
                   "ppg_stats_helper",
                   __func__,
                   __LINE__,
                   pd_status);
        return status;
      }

      status = bfrt_tm_pool_cfg_size_cells_get(pool_id, max_cells);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_BUFFER_POOL,
                   "{}.{}:{}: Failed to get tm pool max status {}",
                   "buffer_pool_helper",
                   __func__,
                   __LINE__,
                   status);
        return status;
      }

      if (dyn_baf != PD_PPG_BAF_DISABLE) {
        shared_max_cells = switch_pd_pool_max_cells_and_baf_to_shared_cells(
            max_cells, dyn_baf);
      }
      shared_max_bytes.count = shared_max_cells * cell_size;

      pd_status = p4_pd_tm_get_ppg_skid_limit(dev_id, pd_hdl, &skid_max_cells);
      if (pd_status != 0) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_PORT_PRIORITY_GROUP,
                   "{}.{}:{}: Failed to get tm ppg skid limit pd_status {}",
                   "ppg_stats_helper",
                   __func__,
                   __LINE__,
                   pd_status);
        return SWITCH_STATUS_FAILURE;
      }
      skid_max_bytes.count = skid_max_cells * cell_size;

      bool sw_model = false;
      bf_pal_pltfm_type_get(dev_id, &sw_model);
      if (!sw_model) {
        pd_status = p4_pd_tm_ppg_usage_get(
            dev_id,
            static_cast<bf_dev_pipe_t> DEV_PORT_TO_PIPE(dev_port),
            pd_hdl,
            &gmin_cells,
            &shared_cells,
            &skid_cells,
            &wm_cells);
        if (pd_status != 0) {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_PORT_PRIORITY_GROUP,
                     "{}.{}:{}: Failed to get tm ppg usage pd_status {}",
                     "ppg_stats_helper",
                     __func__,
                     __LINE__,
                     pd_status);
          return SWITCH_STATUS_FAILURE;
        }
        wm_bytes.count = wm_cells * cell_size;
        shared_occ_bytes.count = shared_cells * cell_size;
        occ_bytes.count = (shared_cells + gmin_cells + skid_cells) * cell_size;
        skid_occ_bytes.count = skid_cells * cell_size;

        if (wm_bytes.count > gmin_max_bytes.count) {
          shared_wm_bytes.count = wm_bytes.count - gmin_max_bytes.count;
        } else {
          shared_wm_bytes.count = 0;
        }

        pd_status = p4_pd_tm_ppg_drop_get(
            dev_id,
            static_cast<bf_dev_pipe_t> DEV_PORT_TO_PIPE(dev_port),
            pd_hdl,
            &dropped_pkts.count);
        if (pd_status != 0) {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_PORT_PRIORITY_GROUP,
                     "{}.{}:{}: Failed to get tm ppg drop pd_status {}",
                     "ppg_stats_helper",
                     __func__,
                     __LINE__,
                     pd_status);
          return SWITCH_STATUS_FAILURE;
        }
      }
    }

    dropped_pkts.count += pfc_wd_pkt_cnt;
    // Packest dropped by PFC WD will be counted as normal ones in PPG stats so
    // need to subtract them
    ppg_counters_pkts.count -= pfc_wd_pkt_cnt;
    ppg_counters_bytes.count -= pfc_wd_bytes_cnt;

    cntrs[SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_PACKETS] = ppg_counters_pkts;
    cntrs[SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_BYTES] = ppg_counters_bytes;
    cntrs[SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_DROPPED_PACKETS] = dropped_pkts;
    cntrs[SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_WATERMARK_BYTES] = wm_bytes;
    cntrs[SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_SHARED_CURR_OCCUPANCY_BYTES] =
        shared_occ_bytes;
    cntrs[SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_CURR_OCCUPANCY_BYTES] =
        occ_bytes;
    cntrs[SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_SKID_CURR_OCCUPANCY_BYTES] =
        skid_occ_bytes;
    cntrs[SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_GUARANTEED_BYTES] =
        gmin_max_bytes;
    cntrs[SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_SHARED_MAX_OCCUPANCY_BYTES] =
        shared_max_bytes;
    cntrs[SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_SKID_MAX_OCCUPANCY_BYTES] =
        skid_max_bytes;
    cntrs[SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_SHARED_WATERMARK_BYTES] =
        shared_wm_bytes;

    return status;
  }

  switch_status_t counters_set(const switch_object_id_t handle) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    p4_pd_status_t pd_status = 0;
    (void)handle;

    if (created_in_hw) {
      pd_status = p4_pd_tm_ppg_watermark_clear(dev_id, pd_hdl);
      if (pd_status != 0) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_PORT_PRIORITY_GROUP,
                   "{}.{}:{}: Failed to clear tm ppg usage pd_status {}",
                   "ppg_stats_helper",
                   __func__,
                   __LINE__,
                   pd_status);
        return SWITCH_STATUS_FAILURE;
      }

      pd_status = p4_pd_tm_ppg_drop_count_clear(dev_id, pd_hdl);
      if (pd_status != 0) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_PORT_PRIORITY_GROUP,
                   "{}.{}:{}: Failed to clear tm ppg drop pd_status {}",
                   "ppg_stats_helper",
                   __func__,
                   __LINE__,
                   pd_status);
        return SWITCH_STATUS_FAILURE;
      }
    }

    if (is_switch_pipe_port_ppg) {
      port_ppg_stats port_ppg_stats_helper(port_handle, status);
      status |= port_ppg_stats_helper.counters_set(port_handle);
    }
    // Clear the software cache for this ppg
    uint64_t reset_count = 0;
    status |= switch_store::v_set(
        get_parent(), SWITCH_PORT_PRIORITY_GROUP_ATTR_BYTE_COUNT, reset_count);
    status |= switch_store::v_set(get_parent(),
                                  SWITCH_PORT_PRIORITY_GROUP_ATTR_PACKET_COUNT,
                                  reset_count);

    std::set<uint8_t> ppg_icos_qos_map;
    status |= ppg_icos_qos_map_get(handle, ppg_icos_qos_map);
    std::set<switch_object_id_t> pfc_wd_list;
    status |= pfc_wd_list_get(ppg_icos_qos_map, pfc_wd_list);

    // Now clear PFC WD counters
    // as the packets dropped by PFC WD will be counted there instead of PPG.
    for (auto pfc_wd : pfc_wd_list) {
      status |= switch_store::object_counters_clear_all(pfc_wd);
    }

    return status;
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    p4_pd_status_t pd_status = 0;
    (void)handle;

    for (auto id : cntr_ids) {
      switch (id) {
        case SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_WATERMARK_BYTES:
        case SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_SHARED_WATERMARK_BYTES:
          if (created_in_hw) {
            pd_status = p4_pd_tm_ppg_watermark_clear(dev_id, pd_hdl);
            if (pd_status != 0) {
              switch_log(SWITCH_API_LEVEL_ERROR,
                         SWITCH_OBJECT_TYPE_PORT_PRIORITY_GROUP,
                         "{}.{}:{}: Failed to clear tm ppg usage pd_status {}",
                         "ppg_stats_helper",
                         __func__,
                         __LINE__,
                         pd_status);
              return SWITCH_STATUS_FAILURE;
            }
          }
          break;
        case SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_DROPPED_PACKETS: {
          if (created_in_hw) {
            pd_status = p4_pd_tm_ppg_drop_count_clear(dev_id, pd_hdl);
            if (pd_status != 0) {
              switch_log(SWITCH_API_LEVEL_ERROR,
                         SWITCH_OBJECT_TYPE_PORT_PRIORITY_GROUP,
                         "{}.{}:{}: Failed to clear tm ppg drop pd_status {}",
                         "ppg_stats_helper",
                         __func__,
                         __LINE__,
                         pd_status);
              return SWITCH_STATUS_FAILURE;
            }
          }

          std::set<uint8_t> ppg_icos_qos_map;
          status |= ppg_icos_qos_map_get(handle, ppg_icos_qos_map);
          std::set<switch_object_id_t> pfc_wd_list;
          status |= pfc_wd_list_get(ppg_icos_qos_map, pfc_wd_list);
          std::vector<uint16_t> pfc_wd_ids = {SWITCH_PFC_WD_COUNTER_ID_PACKETS};

          // Now clear PFC WD counters
          // as the packets dropped by PFC WD will be counted there instead of
          // PPG.
          for (auto pfc_wd : pfc_wd_list) {
            status |= switch_store::object_counters_clear(pfc_wd, pfc_wd_ids);
          }
          break;
        }
        case SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_PACKETS:
        case SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_BYTES: {
          if (is_switch_pipe_port_ppg) {
            std::vector<uint16_t> ids = {id};
            port_ppg_stats port_ppg_stats_helper(port_handle, status);
            status |= port_ppg_stats_helper.counters_set(port_handle, ids);
          }
          break;
        }
        default:
          break;
      }
    }

    return status;
  }

  switch_status_t counters_save(const switch_object_id_t parent) {
    (void)parent;
    bf_status_t bf_status = BF_SUCCESS;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t ctr_value = 0;

    if (get_auto_oid() == 0) return status;

    // TM counter
    std::vector<uint64_t> ctr_list;
    if (created_in_hw) {
      bf_status = bf_tm_ppg_drop_cache_get(
          dev_id,
          static_cast<bf_dev_pipe_t> DEV_PORT_TO_PIPE(dev_port),
          pd_hdl,
          &ctr_value);
      if (bf_status != BF_SUCCESS) {
        switch_log(
            SWITCH_API_LEVEL_ERROR,
            SWITCH_OBJECT_TYPE_PORT_PRIORITY_GROUP,
            "{}.{}:{} Fail to get from tm ppg drop stats history count , "
            "bf_status {}",
            "ppg_stats_helper",
            __func__,
            __LINE__,
            bf_err_str(bf_status));
        return SWITCH_STATUS_FAILURE;
      }
    }

    ctr_list.push_back(static_cast<uint64_t>(ctr_value));
    attr_w tm_ctr_attr_list(SWITCH_PPG_STATS_HELPER_ATTR_TM_STATS_CACHE);
    tm_ctr_attr_list.v_set(ctr_list);
    switch_store::attribute_set(get_auto_oid(), tm_ctr_attr_list);
    return status;
  }

  switch_status_t counters_restore(const switch_object_id_t parent) {
    (void)parent;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    bf_status_t bf_status = BF_SUCCESS;
    uint64_t ctr_value = 0;

    if (get_auto_oid() == 0) return status;

    // TM counter
    std::vector<uint64_t> ctr_list;
    status |= switch_store::v_get(
        get_auto_oid(), SWITCH_PPG_STATS_HELPER_ATTR_TM_STATS_CACHE, ctr_list);
    if (ctr_list.empty()) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_PORT_PRIORITY_GROUP,
                 "{}.{}: Fail to restore tm ppg drop stats cache, status {}"
                 "ppg_stats tm cache list count {} ",
                 __func__,
                 __LINE__,
                 status,
                 ctr_list.size());
      return status;
    }

    if (ctr_list.size() == 1) {
      ctr_value = ctr_list[0];
    }

    // set function
    if (created_in_hw) {
      bf_status = bf_tm_ppg_drop_cache_set(
          dev_id,
          static_cast<bf_dev_pipe_t> DEV_PORT_TO_PIPE(dev_port),
          pd_hdl,
          ctr_value);
      if (bf_status != BF_SUCCESS) {
        switch_log(
            SWITCH_API_LEVEL_ERROR,
            SWITCH_OBJECT_TYPE_PORT_PRIORITY_GROUP,
            "{}.{}:{} Fail to restore tm ppg drop stats cache, bf_status: {}",
            "ppg_stats_helper",
            __func__,
            __LINE__,
            bf_err_str(bf_status));
        return SWITCH_STATUS_FAILURE;
      }
    }

    return status;
  }

 private:
  switch_status_t ppg_icos_qos_map_get(const switch_object_id_t handle,
                                       std::set<uint8_t> &ppg_icos_qos_map) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_object_id_t icos_ppg_qos_map_handle = {};

    status |= switch_store::v_get(port_handle,
                                  SWITCH_PORT_ATTR_ICOS_PPG_QOS_MAP_HANDLE,
                                  icos_ppg_qos_map_handle);

    if (def_ppg_handle == handle) {
      std::vector<bool> default_ppg_icos_qos_map(SWITCH_BUFFER_PFC_ICOS_MAX,
                                                 true);
      /*
       * default ppg case. find out all mapped icos for default ppg.
       */
      if (icos_ppg_qos_map_handle.data) {
        /* find icos which are not mapped will be the potential candidate */

        std::vector<switch_object_id_t> qos_map_list;
        status |= switch_store::v_get(icos_ppg_qos_map_handle,
                                      SWITCH_QOS_MAP_INGRESS_ATTR_QOS_MAP_LIST,
                                      qos_map_list);

        for (auto const qos_map : qos_map_list) {
          uint8_t icos;

          // Get the data from the qos_map handle
          status |=
              switch_store::v_get(qos_map, SWITCH_QOS_MAP_ATTR_ICOS, icos);
          default_ppg_icos_qos_map[icos] = false;
        }
      }

      for (uint8_t icos = 0; icos < SWITCH_BUFFER_PFC_ICOS_MAX; icos++) {
        if (default_ppg_icos_qos_map[icos]) {
          ppg_icos_qos_map.insert(icos);
        }
      }
    } else {
      uint8_t ppg_index;
      status |= switch_store::v_get(
          handle, SWITCH_PORT_PRIORITY_GROUP_ATTR_PPG_INDEX, ppg_index);

      if (icos_ppg_qos_map_handle.data) {
        /* find icos which are mapped will be the potential candidate */
        std::vector<switch_object_id_t> qos_map_list;
        status |= switch_store::v_get(icos_ppg_qos_map_handle,
                                      SWITCH_QOS_MAP_INGRESS_ATTR_QOS_MAP_LIST,
                                      qos_map_list);

        for (auto const qos_map : qos_map_list) {
          uint8_t icos;
          uint8_t mapped_ppg_index;

          // Get the data from the qos_map handle
          status |=
              switch_store::v_get(qos_map, SWITCH_QOS_MAP_ATTR_ICOS, icos);
          status |= switch_store::v_get(
              qos_map, SWITCH_QOS_MAP_ATTR_PPG, mapped_ppg_index);
          if (ppg_index == mapped_ppg_index) {
            ppg_icos_qos_map.insert(icos);
          }
        }
      }
    }
    return status;
  }

  switch_status_t pfc_wd_list_get(const std::set<uint8_t> &ppg_icos_list,
                                  std::set<switch_object_id_t> &pfc_wd_list) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_object_id_t pfc_queue_qos_map_handle = {};
    std::vector<switch_object_id_t> pfc_queue_qos_map_list;
    std::set<uint8_t> qid_list;
    status |=
        switch_store::v_get(port_handle,
                            SWITCH_PORT_ATTR_PFC_PRIORITY_QUEUE_QOS_MAP_HANDLE,
                            pfc_queue_qos_map_handle);

    if (!pfc_queue_qos_map_handle.data) {
      return status;
    }

    status |= switch_store::v_get(pfc_queue_qos_map_handle,
                                  SWITCH_QOS_MAP_EGRESS_ATTR_QOS_MAP_LIST,
                                  pfc_queue_qos_map_list);

    for (auto icos : ppg_icos_list) {
      // Get qid list for icos
      for (auto qos_map : pfc_queue_qos_map_list) {
        uint8_t priority;

        // Get the PFC priority from the qos_map handle
        status |= switch_store::v_get(
            qos_map, SWITCH_QOS_MAP_ATTR_PFC_PRIORITY, priority);

        if (icos == priority) {
          // Got the right priority to queue entry. Take the qid.
          uint8_t qid;
          status |= switch_store::v_get(qos_map, SWITCH_QOS_MAP_ATTR_QID, qid);
          qid_list.insert(qid);
          break;
        }
      }
    }

    // Now get the PFC WD objects for these port and queues and get their
    // counters as the packets dropped by PFC WD will be counted there instead
    // of PPG.
    for (auto qid : qid_list) {
      switch_enum_t pfc_wd_dir = {SWITCH_PFC_WD_ATTR_DIRECTION_INGRESS};
      std::set<attr_w> pfc_wd_attrs;
      switch_object_id_t pfc_wd_handle;

      pfc_wd_attrs.insert(attr_w(SWITCH_PFC_WD_ATTR_DEVICE, device_handle));
      pfc_wd_attrs.insert(attr_w(SWITCH_PFC_WD_ATTR_DIRECTION, pfc_wd_dir));
      pfc_wd_attrs.insert(attr_w(SWITCH_PFC_WD_ATTR_PORT_HANDLE, port_handle));
      pfc_wd_attrs.insert(attr_w(SWITCH_PFC_WD_ATTR_QID, qid));
      status = switch_store::object_id_get_wkey(
          SWITCH_OBJECT_TYPE_PFC_WD, pfc_wd_attrs, pfc_wd_handle);
      if (status == SWITCH_STATUS_SUCCESS) {
        pfc_wd_list.insert(pfc_wd_handle);
      }
    }

    return SWITCH_STATUS_SUCCESS;
  }
};

// Helper class for handling updates to qos_map object
class qos_map_helper : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_QOS_MAP_HELPER;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_QOS_MAP_HELPER_ATTR_PARENT_HANDLE;

 public:
  qos_map_helper(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    /* During warm init sequence, qos map will be created first and then
    referenced by qos_map_ingress/qos_map_egress object. So we can skip
    below update */
    if (switch_store::smiContext::context().in_warm_init()) {
      return;
    }

    std::set<switch_object_id_t> qos_map_ingress_handles;
    status = switch_store::referencing_set_get(
        parent, SWITCH_OBJECT_TYPE_QOS_MAP_INGRESS, qos_map_ingress_handles);
    for (const auto qos_map_ingress : qos_map_ingress_handles) {
      switch_enum_t e = {0};
      switch_qos_map_ingress_attr_type type;
      status |= switch_store::v_get(
          qos_map_ingress, SWITCH_QOS_MAP_INGRESS_ATTR_TYPE, e);
      type = static_cast<switch_qos_map_ingress_attr_type>(e.enumdata);

      if (pcp_tc_map_entry_type(type)) {
        std::set<switch_object_id_t> ports_hdl;
        status = switch_store::referencing_set_get(
            qos_map_ingress, SWITCH_OBJECT_TYPE_PORT, ports_hdl);
        // Even a single qos-map update will completely unconfigure
        // the ingress qos-map list & will reconfigure the new list.
        // This will be improved to update just the modified qos-map entries
        for (const auto port : ports_hdl) {
          std::unique_ptr<object> mobject;
          mobject = std::unique_ptr<pcp_tc_map>(new pcp_tc_map(port, status));
          if (mobject != NULL) {
            mobject->del();
          }
          mobject = std::unique_ptr<pcp_tc_map>(new pcp_tc_map(port, status));
          if (mobject != NULL) {
            mobject->create_update();
          }
        }
        continue;
      }

      if (dscp_tc_map_entry_type(type)) {
        std::set<switch_object_id_t> ports_hdl;
        status = switch_store::referencing_set_get(
            qos_map_ingress, SWITCH_OBJECT_TYPE_PORT, ports_hdl);
        for (const auto port : ports_hdl) {
          // Even a single qos-map update will completely unconfigure
          // the ingress qos-map list & will reconfigure the new list.
          // This will be improved to update just the modified qos-map entries
          std::unique_ptr<object> mobject;
          mobject = std::unique_ptr<dscp_tc_map>(new dscp_tc_map(port, status));
          if (mobject != NULL) {
            mobject->del();
          }
          mobject = std::unique_ptr<dscp_tc_map>(new dscp_tc_map(port, status));
          if (mobject != NULL) {
            mobject->create_update();
          }
        }
        continue;
      }

      traffic_class traffic_cl(qos_map_ingress, status);
      traffic_cl.create_update();

      std::unique_ptr<object> icos_ppg(factory::get_instance().create(
          SWITCH_OBJECT_TYPE_INGRESS_ICOS_PPG_QOS_MAP_HELPER,
          qos_map_ingress,
          status));
      if (icos_ppg != nullptr) {
        icos_ppg->create_update();
      } else {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_QOS_MAP,
                   "{}.{}:{}: failed to create/update icos ppg map {:#x}:{}",
                   "qos_map_helper",
                   __func__,
                   __LINE__,
                   parent.data,
                   status);
      }
    }

    std::set<switch_object_id_t> qos_map_egress_handles;
    status |= switch_store::referencing_set_get(
        parent, SWITCH_OBJECT_TYPE_QOS_MAP_EGRESS, qos_map_egress_handles);
    for (const auto qos_map_egress_handle : qos_map_egress_handles) {
      switch_enum_t e = {0};
      switch_qos_map_egress_attr_type type;
      status |= switch_store::v_get(
          qos_map_egress_handle, SWITCH_QOS_MAP_EGRESS_ATTR_TYPE, e);
      type = static_cast<switch_qos_map_egress_attr_type>(e.enumdata);
      if (l3_qos_egress_map_entry_type(type)) {
        std::set<switch_object_id_t> ports_hdl;
        status = switch_store::referencing_set_get(
            qos_map_egress_handle, SWITCH_OBJECT_TYPE_PORT, ports_hdl);
        for (const auto port : ports_hdl) {
          std::unique_ptr<object> mobject;
          mobject = std::unique_ptr<l3_qos_map_table>(
              new l3_qos_map_table(port, status));
          if (mobject != NULL) {
            mobject->del();
          }
          mobject = std::unique_ptr<l3_qos_map_table>(
              new l3_qos_map_table(port, status));
          if (mobject != NULL) {
            mobject->create_update();
          }
        }
        continue;
      } else if (l2_qos_egress_map_entry_type(type)) {
        std::set<switch_object_id_t> ports_hdl;
        status = switch_store::referencing_set_get(
            qos_map_egress_handle, SWITCH_OBJECT_TYPE_PORT, ports_hdl);
        for (const auto port : ports_hdl) {
          std::unique_ptr<object> mobject;
          mobject = std::unique_ptr<l2_qos_map_table>(
              new l2_qos_map_table(port, status));
          if (mobject != NULL) {
            mobject->del();
          }
          mobject = std::unique_ptr<l2_qos_map_table>(
              new l2_qos_map_table(port, status));
          if (mobject != NULL) {
            mobject->create_update();
          }
        }
        continue;
      }
      std::unique_ptr<object> pfc_queue(factory::get_instance().create(
          SWITCH_OBJECT_TYPE_EGRESS_PFC_PRIORITY_QUEUE_QOS_MAP_HELPER,
          qos_map_egress_handle,
          status));
      if (pfc_queue != nullptr) {
        pfc_queue->create_update();
      } else {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_QOS_MAP,
                   "{}.{}:{}: failed to create/update pfc queue map {:#x}:{}",
                   "qos_map_helper",
                   __func__,
                   __LINE__,
                   parent.data,
                   status);
      }
    }
  }
};

class queue_stats : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_QUEUE_STATS;
  static const switch_attr_id_t status_attr_id = SWITCH_QUEUE_STATS_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_QUEUE_STATS_ATTR_PARENT_HANDLE;

  uint16_t dev_id = 0;
  uint16_t dev_port = 0;
  uint8_t queue_id = 0;
  switch_object_id_t port_handle = {0};
  switch_object_id_t device_handle = {0};

 public:
  queue_stats(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action_list(
            smi_id::T_QUEUE, status_attr_id, auto_ot, parent_attr_id, parent) {
    uint64_t pkts = 0;
    uint64_t bytes = 0;

    status |=
        switch_store::v_get(parent, SWITCH_QUEUE_ATTR_PORT_HANDLE, port_handle);
    status |=
        switch_store::v_get(port_handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
    status |= switch_store::v_get(parent, SWITCH_QUEUE_ATTR_QUEUE_ID, queue_id);

    status |= switch_store::v_get(
        port_handle, SWITCH_PORT_ATTR_DEVICE, device_handle);
    status |=
        switch_store::v_get(device_handle, SWITCH_DEVICE_ATTR_DEV_ID, dev_id);

    device_tgt_set(
        compute_dev_target_for_table(dev_port, smi_id::T_QUEUE, false));

    auto it = match_action_list.begin();
    std::vector<bf_rt_id_t> cntrs{smi_id::P_QUEUE_STATS_BYTES,
                                  smi_id::P_QUEUE_STATS_PKTS};

    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_QUEUE),
                                           _ActionEntry(smi_id::T_QUEUE)));

    status |= it->first.set_exact(smi_id::F_QUEUE_EG_INTR_MD_PORT, dev_port);
    status |= it->first.set_exact(smi_id::F_QUEUE_LOCAL_MD_QOS_QID, queue_id);
    it->second.init_action_data(smi_id::A_QUEUE_COUNT, cntrs);
    it->second.set_arg(smi_id::P_QUEUE_STATS_PKTS, pkts);
    it->second.set_arg(smi_id::P_QUEUE_STATS_BYTES, bytes);
  }

  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    (void)handle;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t bytes = 0, pkts = 0;
    std::vector<switch_counter_t> pfc_wd_cntrs;
    switch_enum_t pfc_wd_dir = {SWITCH_PFC_WD_ATTR_DIRECTION_EGRESS};
    std::set<attr_w> pfc_wd_attrs;
    switch_object_id_t pfc_wd_handle = {0};
    p4_pd_status_t pd_status = 0;

    p4_object_match_action_list::data_get();

    match_action_list[0].second.get_arg(
        smi_id::P_QUEUE_STATS_BYTES, smi_id::A_QUEUE_COUNT, &bytes);
    match_action_list[0].second.get_arg(
        smi_id::P_QUEUE_STATS_PKTS, smi_id::A_QUEUE_COUNT, &pkts);
    switch_counter_t pkt_cntr = {
        .counter_id = SWITCH_QUEUE_COUNTER_ID_STAT_PACKETS, .count = pkts};
    switch_counter_t byte_cntr = {
        .counter_id = SWITCH_QUEUE_COUNTER_ID_STAT_BYTES, .count = bytes};

    switch_counter_t pkt_drop_cntr = {
        .counter_id = SWITCH_QUEUE_COUNTER_ID_DROPPED_PACKETS, .count = 0};
    uint64_t pfc_wd_pkt_cnt = 0;
    uint64_t pfc_wd_bytes_cnt = 0;
    // In case packets were dropped by egress PFC WD entry they will
    // be counted there instead of queue. So need to pull them from there.
    pfc_wd_attrs.insert(attr_w(SWITCH_PFC_WD_ATTR_DEVICE, device_handle));
    pfc_wd_attrs.insert(attr_w(SWITCH_PFC_WD_ATTR_DIRECTION, pfc_wd_dir));
    pfc_wd_attrs.insert(attr_w(SWITCH_PFC_WD_ATTR_PORT_HANDLE, port_handle));
    pfc_wd_attrs.insert(attr_w(SWITCH_PFC_WD_ATTR_QID, queue_id));
    status = switch_store::object_id_get_wkey(
        SWITCH_OBJECT_TYPE_PFC_WD, pfc_wd_attrs, pfc_wd_handle);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_DETAIL,
                 SWITCH_OBJECT_TYPE_QUEUE,
                 "{}.{}: Cant find PFC_WD object: status={}",
                 __func__,
                 __LINE__,
                 status);
      // It is expected that PFC_WD object could not exist
      // so,clean up status for father operations
      status = SWITCH_STATUS_SUCCESS;
    } else {
      status = switch_store::object_counters_get(pfc_wd_handle, pfc_wd_cntrs);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_INFO,
                   SWITCH_OBJECT_TYPE_QUEUE,
                   "{}.{}: Cant get PFC_WD counters: status={}",
                   __func__,
                   __LINE__,
                   status);
        return status;
      }

      for (auto counter : pfc_wd_cntrs) {
        if (counter.counter_id == SWITCH_PFC_WD_COUNTER_ID_PACKETS) {
          pfc_wd_pkt_cnt += counter.count;
        } else {
          pfc_wd_bytes_cnt += counter.count;
        }
      }
    }

    pkt_drop_cntr.count += pfc_wd_pkt_cnt;
    pkt_cntr.count -= pfc_wd_pkt_cnt;
    byte_cntr.count -= pfc_wd_bytes_cnt;

    bool sw_model = false;
    bf_pal_pltfm_type_get(dev_id, &sw_model);
    if (sw_model) {
      cntrs[SWITCH_QUEUE_COUNTER_ID_DROPPED_PACKETS] = pkt_drop_cntr;
      cntrs[SWITCH_QUEUE_COUNTER_ID_STAT_PACKETS] = pkt_cntr;
      cntrs[SWITCH_QUEUE_COUNTER_ID_STAT_BYTES] = byte_cntr;
      return status;
    }

    // Everything after here is TM fixed counters
    uint64_t tm_q_drop_cnt = 0;
    pd_status = p4_pd_tm_q_drop_get(
        dev_id,
        static_cast<bf_dev_pipe_t> DEV_PORT_TO_PIPE(dev_port),
        dev_port,
        queue_id,
        &tm_q_drop_cnt);
    if (pd_status != 0) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_QUEUE,
                 "{}.{}:{}: Failed to get tm q drop stats pd_status {} "
                 "dev_port {} qid {}",
                 "queue_stats",
                 __func__,
                 __LINE__,
                 pd_status,
                 dev_port,
                 queue_id);
      return SWITCH_STATUS_FAILURE;
    }
    pkt_drop_cntr.count += tm_q_drop_cnt;

    uint32_t inuse_cells = 0, wm_cells = 0;
    pd_status = p4_pd_tm_q_usage_get(
        dev_id,
        static_cast<bf_dev_pipe_t> DEV_PORT_TO_PIPE(dev_port),
        dev_port,
        queue_id,
        &inuse_cells,
        &wm_cells);
    if (pd_status != 0) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_QUEUE,
                 "{}.{}:{}: Failed to get tm q usage stats pd_status {} ",
                 "dev_port {} qid {}",
                 "queue_stats",
                 __func__,
                 __LINE__,
                 pd_status,
                 dev_port,
                 queue_id);
      return SWITCH_STATUS_FAILURE;
    }

    switch_counter_t inuse_bytes_cntr = {
        .counter_id = SWITCH_QUEUE_COUNTER_ID_CURR_OCCUPANCY_BYTES, .count = 0};
    switch_counter_t wm_bytes_cntr = {
        .counter_id = SWITCH_QUEUE_COUNTER_ID_WATERMARK_BYTES, .count = 0};
    compute_pd_buffer_cells_to_bytes(
        dev_id, inuse_cells, &inuse_bytes_cntr.count);
    compute_pd_buffer_cells_to_bytes(dev_id, wm_cells, &wm_bytes_cntr.count);

    uint64_t cell_size = 0;
    uint32_t num_cells = 0;
    pd_status = p4_pd_tm_get_q_guaranteed_min_limit(
        dev_id, dev_port, queue_id, &num_cells);
    if (pd_status != 0) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_QUEUE,
                 "{}.{}:{}: Failed to get tm q gmin limit pd_status {} "
                 "dev_port {} queue_id {}",
                 "queue_stats",
                 __func__,
                 __LINE__,
                 pd_status,
                 dev_port,
                 queue_id);
      return SWITCH_STATUS_FAILURE;
    }
    compute_pd_buffer_cells_to_bytes(dev_id, num_cells, &cell_size);
    switch_counter_t shared_inuse_bytes_cntr = {
        .counter_id = SWITCH_QUEUE_COUNTER_ID_SHARED_CURR_OCCUPANCY_BYTES,
        .count = 0};
    switch_counter_t shared_wm_bytes_cntr = {
        .counter_id = SWITCH_QUEUE_COUNTER_ID_SHARED_WATERMARK_BYTES,
        .count = 0};
    if (inuse_bytes_cntr.count > cell_size)
      shared_inuse_bytes_cntr.count = inuse_bytes_cntr.count - cell_size;
    else
      shared_inuse_bytes_cntr.count = 0;
    if (wm_bytes_cntr.count > cell_size)
      shared_wm_bytes_cntr.count = wm_bytes_cntr.count - cell_size;
    else
      shared_wm_bytes_cntr.count = 0;

    p4_pd_pool_id_t pool_id;
    p4_pd_tm_queue_baf_t dyn_baf;
    uint32_t base_lim = 0, hysteresis = 0;
    pd_status = p4_pd_tm_get_q_app_pool_usage(
        dev_id, dev_port, queue_id, &pool_id, &base_lim, &dyn_baf, &hysteresis);
    if (pd_status != 0) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_QUEUE,
                 "{}.{}:{}: Failed to get tm q pool and baf pd_status {} "
                 "dev_port {} queue_id {}",
                 "queue_stats",
                 __func__,
                 __LINE__,
                 pd_status,
                 dev_port,
                 queue_id);
      return SWITCH_STATUS_FAILURE;
    }

    uint32_t max_cells = 0;
    status = bfrt_tm_pool_cfg_size_cells_get(pool_id, max_cells);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_BUFFER_POOL,
                 "{}.{}:{}: Failed to get tm pool max status {}",
                 "buffer_pool_helper",
                 __func__,
                 __LINE__,
                 status);
      return status;
    }

    switch_counter_t max_bytes_cntr = {
        .counter_id = SWITCH_QUEUE_COUNTER_ID_MAX_OCCUPANCY_BYTES, .count = 0};
    uint32_t shared_max_cells = base_lim;
    if (dyn_baf != PD_Q_BAF_DISABLE) {
      shared_max_cells = switch_pd_pool_max_cells_and_baf_to_shared_cells(
          max_cells, (p4_pd_tm_ppg_baf_t)dyn_baf);
    }
    compute_pd_buffer_cells_to_bytes(
        dev_id, (shared_max_cells + num_cells), &max_bytes_cntr.count);

    cntrs[SWITCH_QUEUE_COUNTER_ID_DROPPED_PACKETS] = pkt_drop_cntr;
    cntrs[SWITCH_QUEUE_COUNTER_ID_WATERMARK_BYTES] = wm_bytes_cntr;
    cntrs[SWITCH_QUEUE_COUNTER_ID_CURR_OCCUPANCY_BYTES] = inuse_bytes_cntr;
    cntrs[SWITCH_QUEUE_COUNTER_ID_STAT_PACKETS] = pkt_cntr;
    cntrs[SWITCH_QUEUE_COUNTER_ID_STAT_BYTES] = byte_cntr;
    cntrs[SWITCH_QUEUE_COUNTER_ID_SHARED_WATERMARK_BYTES] =
        shared_wm_bytes_cntr;
    cntrs[SWITCH_QUEUE_COUNTER_ID_SHARED_CURR_OCCUPANCY_BYTES] =
        shared_inuse_bytes_cntr;
    cntrs[SWITCH_QUEUE_COUNTER_ID_MAX_OCCUPANCY_BYTES] = max_bytes_cntr;
    return status;
  }

  switch_status_t counters_set(const switch_object_id_t handle) {
    (void)handle;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t pfc_wd_dir = {SWITCH_PFC_WD_ATTR_DIRECTION_EGRESS};
    std::set<attr_w> pfc_wd_attrs;
    switch_object_id_t pfc_wd_handle = {0};
    p4_pd_status_t pd_status = 0;

    // In case there is egress PFC WD object for these queue id and port
    // need to clear its counters as well
    pfc_wd_attrs.insert(attr_w(SWITCH_PFC_WD_ATTR_DEVICE, device_handle));
    pfc_wd_attrs.insert(attr_w(SWITCH_PFC_WD_ATTR_DIRECTION, pfc_wd_dir));
    pfc_wd_attrs.insert(attr_w(SWITCH_PFC_WD_ATTR_PORT_HANDLE, port_handle));
    pfc_wd_attrs.insert(attr_w(SWITCH_PFC_WD_ATTR_QID, queue_id));
    status = switch_store::object_id_get_wkey(
        SWITCH_OBJECT_TYPE_PFC_WD, pfc_wd_attrs, pfc_wd_handle);
    if (status == SWITCH_STATUS_SUCCESS) {
      status = switch_store::object_counters_clear_all(pfc_wd_handle);
    } else if (status == SWITCH_STATUS_ITEM_NOT_FOUND) {
      // PFC WD object not found. Skip pfc wd counter clear and continue
      status = SWITCH_STATUS_SUCCESS;
    } else {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_QUEUE,
                 "{}.{}:{}: Failed to clear PFC WD counter for port: {}, queue "
                 "id: {}, status: {} "
                 "queue_stats",
                 __func__,
                 __LINE__,
                 dev_port,
                 queue_id,
                 status);
      return status;
    }

    for (auto &entry : match_action_list) {
      uint64_t value = 0;
      entry.second.set_arg(smi_id::P_QUEUE_STATS_PKTS, value);
      entry.second.set_arg(smi_id::P_QUEUE_STATS_BYTES, value);
    }
    p4_object_match_action_list::data_set();

    bool sw_model = false;
    bf_pal_pltfm_type_get(dev_id, &sw_model);
    if (sw_model) return status;

    pd_status = p4_pd_tm_q_drop_count_clear(dev_id, dev_port, queue_id);
    if (pd_status != 0) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_QUEUE,
                 "{}.{}:{}: Failed to clear tm q drop stats pd_status {} "
                 "dev_port {} qid {}",
                 "queue_stats",
                 __func__,
                 __LINE__,
                 pd_status,
                 dev_port,
                 queue_id);
      return SWITCH_STATUS_FAILURE;
    }

    pd_status = p4_pd_tm_q_watermark_clear(dev_id, dev_port, queue_id);
    if (pd_status != 0) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_QUEUE,
                 "{}.{}:{}: Failed to clear tm q usage stats pd_status {} ",
                 "dev_port {} qid {}",
                 "queue_stats",
                 __func__,
                 __LINE__,
                 pd_status,
                 dev_port,
                 queue_id);
      return SWITCH_STATUS_FAILURE;
    }
    return status;
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    (void)handle;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    p4_pd_status_t pd_status = 0;

    bool sw_model = false;
    bf_pal_pltfm_type_get(dev_id, &sw_model);

    for (auto id : cntr_ids) {
      switch (id) {
        case SWITCH_QUEUE_COUNTER_ID_STAT_PACKETS:
        case SWITCH_QUEUE_COUNTER_ID_STAT_BYTES:
          p4_object_match_action_list::data_get();
          for (auto &entry : match_action_list) {
            uint64_t value = 0;
            entry.second.set_arg((id == SWITCH_QUEUE_COUNTER_ID_STAT_PACKETS)
                                     ? smi_id::P_QUEUE_STATS_PKTS
                                     : smi_id::P_QUEUE_STATS_BYTES,
                                 value);
          }
          p4_object_match_action_list::data_set();
          break;
        case SWITCH_QUEUE_COUNTER_ID_DROPPED_PACKETS: {
          // In case there is an egress PFC WD object for these queue id and
          // port
          // need to clear its counters as well
          switch_enum_t pfc_wd_dir = {SWITCH_PFC_WD_ATTR_DIRECTION_EGRESS};
          std::set<attr_w> pfc_wd_attrs;
          switch_object_id_t pfc_wd_handle = {0};

          pfc_wd_attrs.insert(attr_w(SWITCH_PFC_WD_ATTR_DEVICE, device_handle));
          pfc_wd_attrs.insert(attr_w(SWITCH_PFC_WD_ATTR_DIRECTION, pfc_wd_dir));
          pfc_wd_attrs.insert(
              attr_w(SWITCH_PFC_WD_ATTR_PORT_HANDLE, port_handle));
          pfc_wd_attrs.insert(attr_w(SWITCH_PFC_WD_ATTR_QID, queue_id));
          status = switch_store::object_id_get_wkey(
              SWITCH_OBJECT_TYPE_PFC_WD, pfc_wd_attrs, pfc_wd_handle);
          if (status == SWITCH_STATUS_SUCCESS) {
            std::vector<uint16_t> pfc_wd_ids = {
                SWITCH_PFC_WD_COUNTER_ID_PACKETS};
            status =
                switch_store::object_counters_clear(pfc_wd_handle, pfc_wd_ids);
          } else if (status == SWITCH_STATUS_ITEM_NOT_FOUND) {
            status = SWITCH_STATUS_SUCCESS;
          }

          if (sw_model) continue;
          pd_status = p4_pd_tm_q_drop_count_clear(dev_id, dev_port, queue_id);
          if (pd_status != 0) {
            switch_log(SWITCH_API_LEVEL_ERROR,
                       SWITCH_OBJECT_TYPE_QUEUE,
                       "{}.{}:{}: Failed to clear tm q drop stats pd_status {} "
                       "dev_port {} qid {}",
                       "queue_stats",
                       __func__,
                       __LINE__,
                       pd_status,
                       dev_port,
                       queue_id);
            return SWITCH_STATUS_FAILURE;
          }

          break;
        }
        case SWITCH_QUEUE_COUNTER_ID_WATERMARK_BYTES:
        case SWITCH_QUEUE_COUNTER_ID_SHARED_WATERMARK_BYTES:
          if (sw_model) continue;
          pd_status = p4_pd_tm_q_watermark_clear(dev_id, dev_port, queue_id);
          if (pd_status != 0) {
            switch_log(
                SWITCH_API_LEVEL_ERROR,
                SWITCH_OBJECT_TYPE_QUEUE,
                "{}.{}:{}: Failed to clear tm q usage stats pd_status {} ",
                "dev_port {} qid {}",
                "queue_stats",
                __func__,
                __LINE__,
                pd_status,
                dev_port,
                queue_id);
            return SWITCH_STATUS_FAILURE;
          }
          break;
        default:
          break;
      }
    }

    return status;
  }

  switch_status_t counters_save(const switch_object_id_t parent) {
    (void)parent;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    p4_pd_status_t pd_status = 0;

    uint64_t ctr_value = 0;
    uint64_t ctr_bytes = 0, ctr_pkt = 0;

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    p4_object_match_action_list::data_get();
    for (auto const &entry : match_action_list) {
      ctr_bytes = 0, ctr_pkt = 0;
      entry.second.get_arg(
          smi_id::P_QUEUE_STATS_BYTES, smi_id::A_QUEUE_COUNT, &ctr_bytes);
      entry.second.get_arg(
          smi_id::P_QUEUE_STATS_PKTS, smi_id::A_QUEUE_COUNT, &ctr_pkt);
      ctr_list.push_back(static_cast<uint64_t>(ctr_bytes));
      ctr_list.push_back(static_cast<uint64_t>(ctr_pkt));
    }
    attr_w ctr_attr_list(SWITCH_QUEUE_STATS_ATTR_MAU_STATS_CACHE);
    ctr_attr_list.v_set(ctr_list);
    switch_store::attribute_set(get_auto_oid(), ctr_attr_list);

    bool sw_model = false;
    bf_pal_pltfm_type_get(dev_id, &sw_model);
    if (sw_model) return status;

    // TM counter
    pd_status = p4_pd_tm_q_drop_get(
        dev_id,
        static_cast<bf_dev_pipe_t> DEV_PORT_TO_PIPE(dev_port),
        dev_port,
        queue_id,
        &ctr_value);
    if (pd_status != 0) {
      switch_log(
          SWITCH_API_LEVEL_ERROR,
          SWITCH_OBJECT_TYPE_QUEUE,
          "{}.{}:{} Fail to get from tm q drop stats cache, pd_status {}",
          "queue_stats",
          __func__,
          __LINE__,
          pd_status);
      return SWITCH_STATUS_FAILURE;
    }

    ctr_list.clear();
    ctr_list.push_back(static_cast<uint64_t>(ctr_value));
    attr_w tm_ctr_attr_list(SWITCH_QUEUE_STATS_ATTR_TM_STATS_CACHE);
    tm_ctr_attr_list.v_set(ctr_list);
    switch_store::attribute_set(get_auto_oid(), tm_ctr_attr_list);
    return status;
  }

  switch_status_t counters_restore(const switch_object_id_t parent) {
    (void)parent;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    bf_status_t bf_status = BF_SUCCESS;
    uint64_t ctr_value = 0;
    size_t list_i;

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    status |= switch_store::v_get(
        get_auto_oid(), SWITCH_QUEUE_STATS_ATTR_MAU_STATS_CACHE, ctr_list);
    if (ctr_list.empty()) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_QUEUE,
                 "{}.{}: No stat cache to restore mau stats, "
                 "queue_stats cache list empty, status {}",
                 __func__,
                 __LINE__,
                 status);
      return SWITCH_STATUS_SUCCESS;
    }

    list_i = 0;
    for (auto &entry : match_action_list) {
      entry.second.set_arg(smi_id::P_QUEUE_STATS_BYTES, ctr_list[list_i]);
      list_i++;
      entry.second.set_arg(smi_id::P_QUEUE_STATS_PKTS, ctr_list[list_i]);
      list_i++;
    }
    status = p4_object_match_action_list::data_set();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_QUEUE,
                 "{}.{}: Fail to restore mau stats cache,  "
                 "queue_stats status {} ",
                 __func__,
                 __LINE__,
                 status);
    }

    bool sw_model = false;
    bf_pal_pltfm_type_get(dev_id, &sw_model);
    if (sw_model) return status;

    // TM counter
    std::vector<uint64_t> tm_ctr_list;
    status |= switch_store::v_get(
        get_auto_oid(), SWITCH_QUEUE_STATS_ATTR_TM_STATS_CACHE, tm_ctr_list);
    if (tm_ctr_list.empty()) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_QUEUE,
                 "{}.{}: Fail to restore tm queue drop stats cache,"
                 "queue_stats tm cache list count status {} ",
                 __func__,
                 __LINE__,
                 status,
                 tm_ctr_list.size());
      return status;
    }

    if (tm_ctr_list.size() == 1) {
      ctr_value = tm_ctr_list[0];
    }

    bf_status = bf_tm_q_drop_cache_set(
        dev_id,
        static_cast<bf_dev_pipe_t> DEV_PORT_TO_PIPE(dev_port),
        dev_port,
        queue_id,
        ctr_value);
    if (bf_status != BF_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_QUEUE,
                 "{}.{}:{} Fail to restore tm q drop stats cache, bf_status {}",
                 "queue_stats",
                 __func__,
                 __LINE__,
                 bf_err_str(bf_status));
      return SWITCH_STATUS_FAILURE;
    }

    return status;
  }
};

switch_status_t before_qos_map_ingress_update(const switch_object_id_t hdl,
                                              const attr_w &attr) {
  (void)attr;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::unique_ptr<object> mobject;
  switch_enum_t e = {0};
  switch_qos_map_ingress_attr_type type;
  status |= switch_store::v_get(hdl, SWITCH_QOS_MAP_INGRESS_ATTR_TYPE, e);
  type = static_cast<switch_qos_map_ingress_attr_type>(e.enumdata);

  if (pcp_tc_map_entry_type(type)) {
    std::set<switch_object_id_t> ports_hdl;
    status = switch_store::referencing_set_get(
        hdl, SWITCH_OBJECT_TYPE_PORT, ports_hdl);
    for (const auto port : ports_hdl) {
      mobject = std::unique_ptr<pcp_tc_map>(new pcp_tc_map(port, status));
      if (mobject != NULL) {
        mobject->del();
      }
    }
  } else if (dscp_tc_map_entry_type(type)) {
    std::set<switch_object_id_t> ports_hdl;
    status = switch_store::referencing_set_get(
        hdl, SWITCH_OBJECT_TYPE_PORT, ports_hdl);
    for (const auto port : ports_hdl) {
      mobject = std::unique_ptr<dscp_tc_map>(new dscp_tc_map(port, status));
      if (mobject != NULL) {
        mobject->del();
      }
    }
  } else if (tc_map_entry_type(type)) {
    mobject = std::unique_ptr<traffic_class>(new traffic_class(hdl, status));
    if (mobject != NULL) {
      mobject->del();
    }
  }

  return status;
}

switch_status_t after_qos_map_ingress_update(const switch_object_id_t hdl,
                                             const attr_w &attr) {
  (void)attr;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::unique_ptr<object> mobject;
  switch_enum_t e = {0};
  switch_qos_map_ingress_attr_type type;
  status |= switch_store::v_get(hdl, SWITCH_QOS_MAP_INGRESS_ATTR_TYPE, e);
  type = static_cast<switch_qos_map_ingress_attr_type>(e.enumdata);

  if (pcp_tc_map_entry_type(type)) {
    std::set<switch_object_id_t> ports_hdl;
    status = switch_store::referencing_set_get(
        hdl, SWITCH_OBJECT_TYPE_PORT, ports_hdl);
    for (const auto port : ports_hdl) {
      mobject = std::unique_ptr<pcp_tc_map>(new pcp_tc_map(port, status));
      if (mobject != NULL) {
        mobject->create_update();
      }
    }
  } else if (dscp_tc_map_entry_type(type)) {
    std::set<switch_object_id_t> ports_hdl;
    status = switch_store::referencing_set_get(
        hdl, SWITCH_OBJECT_TYPE_PORT, ports_hdl);
    for (const auto port : ports_hdl) {
      mobject = std::unique_ptr<dscp_tc_map>(new dscp_tc_map(port, status));
      if (mobject != NULL) {
        mobject->create_update();
      }
    }
  } else if (tc_map_entry_type(type)) {
    mobject = std::unique_ptr<traffic_class>(new traffic_class(hdl, status));
    if (mobject != NULL) {
      mobject->create_update();
    }
  }

  return status;
}

switch_status_t before_qos_map_egress_update(const switch_object_id_t handle,
                                             const attr_w &attr) {
  (void)attr;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::unique_ptr<object> mobject;

  switch_enum_t e = {0};
  switch_qos_map_egress_attr_type type;
  status |= switch_store::v_get(handle, SWITCH_QOS_MAP_EGRESS_ATTR_TYPE, e);
  type = static_cast<switch_qos_map_egress_attr_type>(e.enumdata);

  if (l3_qos_egress_map_entry_type(type)) {
    std::set<switch_object_id_t> ports_hdl;
    status = switch_store::referencing_set_get(
        handle, SWITCH_OBJECT_TYPE_PORT, ports_hdl);
    for (const auto port : ports_hdl) {
      mobject =
          std::unique_ptr<l3_qos_map_table>(new l3_qos_map_table(port, status));
      if (mobject != NULL) {
        mobject->del();
      }
    }
  } else if (l2_qos_egress_map_entry_type(type)) {
    std::set<switch_object_id_t> ports_hdl;
    status = switch_store::referencing_set_get(
        handle, SWITCH_OBJECT_TYPE_PORT, ports_hdl);
    for (const auto port : ports_hdl) {
      mobject =
          std::unique_ptr<l2_qos_map_table>(new l2_qos_map_table(port, status));
      if (mobject != NULL) {
        mobject->del();
      }
    }
  }
  return status;
}

switch_status_t after_qos_map_egress_update(const switch_object_id_t handle,
                                            const attr_w &attr) {
  (void)attr;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::unique_ptr<object> mobject;

  switch_enum_t e = {0};
  switch_qos_map_egress_attr_type type;
  status |= switch_store::v_get(handle, SWITCH_QOS_MAP_EGRESS_ATTR_TYPE, e);
  type = static_cast<switch_qos_map_egress_attr_type>(e.enumdata);
  if (l3_qos_egress_map_entry_type(type)) {
    std::set<switch_object_id_t> ports_hdl;
    status = switch_store::referencing_set_get(
        handle, SWITCH_OBJECT_TYPE_PORT, ports_hdl);
    for (const auto port : ports_hdl) {
      mobject =
          std::unique_ptr<l3_qos_map_table>(new l3_qos_map_table(port, status));
      if (mobject != NULL) {
        mobject->create_update();
      }
    }
  } else if (l2_qos_egress_map_entry_type(type)) {
    std::set<switch_object_id_t> ports_hdl;
    status = switch_store::referencing_set_get(
        handle, SWITCH_OBJECT_TYPE_PORT, ports_hdl);
    for (const auto port : ports_hdl) {
      mobject =
          std::unique_ptr<l2_qos_map_table>(new l2_qos_map_table(port, status));
      if (mobject != NULL) {
        mobject->create_update();
      }
    }
  }
  return status;
}

switch_status_t qos_init() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  status |= switch_store::reg_update_trigs_before(
      SWITCH_OBJECT_TYPE_QOS_MAP_INGRESS, &before_qos_map_ingress_update);
  status |= switch_store::reg_update_trigs_after(
      SWITCH_OBJECT_TYPE_QOS_MAP_INGRESS, &after_qos_map_ingress_update);
  status |= switch_store::reg_update_trigs_before(
      SWITCH_OBJECT_TYPE_QOS_MAP_EGRESS, &before_qos_map_egress_update);
  status |= switch_store::reg_update_trigs_after(
      SWITCH_OBJECT_TYPE_QOS_MAP_EGRESS, &after_qos_map_egress_update);
  REGISTER_OBJECT(dscp_tc_map, SWITCH_OBJECT_TYPE_DSCP_TC_MAP);
  REGISTER_OBJECT(pcp_tc_map, SWITCH_OBJECT_TYPE_PCP_TC_MAP);
  REGISTER_OBJECT(traffic_class, SWITCH_OBJECT_TYPE_TRAFFIC_CLASS);
  REGISTER_OBJECT(l3_qos_map_table, SWITCH_OBJECT_TYPE_L3_QOS_MAP_TABLE);
  REGISTER_OBJECT(l2_qos_map_table, SWITCH_OBJECT_TYPE_L2_QOS_MAP_TABLE);
  REGISTER_OBJECT(port_ppg_stats, SWITCH_OBJECT_TYPE_PORT_PPG_STATS);
  REGISTER_OBJECT(ppg_stats_helper, SWITCH_OBJECT_TYPE_PPG_STATS_HELPER);
  REGISTER_OBJECT(queue_stats, SWITCH_OBJECT_TYPE_QUEUE_STATS);
  REGISTER_OBJECT(qos_map_helper, SWITCH_OBJECT_TYPE_QOS_MAP_HELPER);

  return status;
}

switch_status_t qos_clean() {
  tc_map.erase(tc_map.begin(), tc_map.end());
  return SWITCH_STATUS_SUCCESS;
}
}  // namespace smi
