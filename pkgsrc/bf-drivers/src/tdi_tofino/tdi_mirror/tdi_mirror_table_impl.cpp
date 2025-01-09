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


// #include <tdi_common/tdi_pipe_mgr_intf.hpp>

// tdi include
#include <tdi/common/tdi_init.hpp>
#include "tdi/common/tdi_defs.h"

// local tofino includes
#include <tdi_common/tdi_tofino_target.hpp>
#include <tdi_common/tdi_pipe_mgr_intf.hpp>
#include <tdi_mirror/tdi_mirror_table_impl.hpp>
#include <tdi_mirror/tdi_mirror_table_data_impl.hpp>
#include <tdi_mirror/tdi_mirror_table_key_impl.hpp>

namespace tdi {
namespace tna {
namespace tofino {

const std::map<std::string, bf_mirror_direction_e>
    MirrorCfgTable::mirror_str_to_dir_map{{"INGRESS", BF_DIR_INGRESS},
                                          {"EGRESS", BF_DIR_EGRESS},
                                          {"BOTH", BF_DIR_BOTH}};

const std::map<bf_mirror_direction_e, std::string>
    MirrorCfgTable::mirror_dir_to_str_map{{BF_DIR_INGRESS, "INGRESS"},
                                          {BF_DIR_EGRESS, "EGRESS"},
                                          {BF_DIR_BOTH, "BOTH"},
                                          {BF_DIR_NONE, "NONE"}};

const std::map<std::string, bf_tm_color_t>
    MirrorCfgTable::mirror_str_to_packet_color_map{
        {"GREEN", BF_TM_COLOR_GREEN},
        {"YELLOW", BF_TM_COLOR_YELLOW},
        {"RED", BF_TM_COLOR_RED}};

const std::map<bf_tm_color_t, std::string>
    MirrorCfgTable::mirror_packet_color_to_str_map{
        {BF_TM_COLOR_GREEN, "GREEN"},
        {BF_TM_COLOR_YELLOW, "YELLOW"},
        {BF_TM_COLOR_RED, "RED"}};

tdi_status_t MirrorCfgTable::entryAdd(const tdi::Session &session,
                                      const tdi::Target &dev_tgt,
                                      const tdi::Flags & /*flags*/,
                                      const tdi::TableKey &key,
                                      const tdi::TableData &data) const {
  tdi_status_t status;

  const MirrorCfgTableKey &mirror_key =
      static_cast<const MirrorCfgTableKey &>(key);

  // First get the mirror session id from passed in key
  const int &session_id = static_cast<const int &>(mirror_key.getId());

  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  // Get the mirror session_info struct & session_enable field from pipe manager
  bf_mirror_session_info_t s_info;
  // Initialize the mirror_session_info struct to all 0
  std::memset(&s_info, 0, sizeof(s_info));
  status = pipeMgr->pipeMgrMirrorSessionGet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      session_id,
      &s_info);
  // a success for this pipe_mgr call is actually an error for `Add`
  // since the entry should not exist before add
  if (status == BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s: Error: mirror session with id %d already exists, status "
        "= %d",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        session_id,
        status);
    return TDI_INVALID_ARG;
  }
  // Else, for mirror config table, add and modify operations are same.
  // This is true for pipe_mgr API as well.
  return this->entryModInternal(session, dev_tgt, key, data);
}

tdi_status_t MirrorCfgTable::entryMod(const tdi::Session &session,
                                      const tdi::Target &dev_tgt,
                                      const tdi::Flags & /*flags*/,
                                      const tdi::TableKey &key,
                                      const tdi::TableData &data) const {
  tdi_status_t status;

  const MirrorCfgTableKey &mirror_key =
      static_cast<const MirrorCfgTableKey &>(key);

  // First get the mirror session id from passed in key
  const int &session_id = static_cast<const int &>(mirror_key.getId());

  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  // Get the mirror session_info struct & session_enable field from pipe manager
  bf_mirror_session_info_t s_info;
  // Initialize the mirror_session_info struct to all 0
  std::memset(&s_info, 0, sizeof(s_info));
  status = pipeMgr->pipeMgrMirrorSessionGet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      session_id,
      &s_info);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s: Error: mirror session with id %d does not exist, status = "
        "%d",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        session_id,
        status);
    return TDI_INVALID_ARG;
  }
  // Else, for mirror config table, add and modify operations are same.
  // This is true for pipe_mgr API as well.
  return this->entryModInternal(session, dev_tgt, key, data);
}

tdi_status_t MirrorCfgTable::entryModInternal(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::TableKey &key,
    const tdi::TableData &data) const {
  tdi_status_t status = TDI_OBJECT_NOT_FOUND;

  const MirrorCfgTableKey &mirror_key =
      static_cast<const MirrorCfgTableKey &>(key);
  const MirrorCfgTableData &mirror_data =
      static_cast<const MirrorCfgTableData &>(data);

  // First get the mirror session id from passed in key
  const int &session_id = static_cast<const int &>(mirror_key.getId());

  const std::unordered_map<tdi_id_t, bool> &boolField =
      mirror_data.getBoolFieldDataMap();
  const std::unordered_map<tdi_id_t, uint64_t> &u64Field =
      mirror_data.getU64FieldDataMap();
  const std::unordered_map<tdi_id_t, std::string> &strField =
      mirror_data.getStrFieldDataMap();
  const std::unordered_map<tdi_id_t,
                           std::array<uint8_t, TDI_MIRROR_INT_HDR_SIZE>>
      &arrayField = mirror_data.getArrayFieldDataMap();

  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  // Get the action id from data
  auto action_id = mirror_data.actionIdGet();
  // Check and proceed only if action id is not 0
  if (action_id == 0) {
    LOG_TRACE("%s:%d %s : Error : Action id is invalid (0) ",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }

  // Get the action name from action id
  auto actionInfo = tableInfoGet()->actionGet(action_id);
  if (actionInfo == nullptr) {
    LOG_TRACE("%s:%d %s : Error : Failed to get action for action id %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              action_id);
    return TDI_OBJECT_NOT_FOUND;
  }
  auto action_name = actionInfo->nameGet();

  // Initialize the mirror_session_info struct to all 0
  bf_mirror_session_info_t s_info;
  std::memset(&s_info, 0, sizeof(s_info));

  // Set the mirror session type based on action name
  if (action_name == "coalescing") {
    s_info.mirror_type = BF_MIRROR_TYPE_COAL;
  } else {
    s_info.mirror_type = BF_MIRROR_TYPE_NORM;
  }

  // Set the common mirror session parameters
  // For each filed name in the mirror session config, get the
  // field id and check if it is present in the input data and set
  // the value in mirror session info struct to be passed in to
  // drivers API

  // Set the direction for the mirror session
  auto field_id = tableInfoGet()->dataFieldIdGet("direction", action_id);
  if (field_id == 0) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for direction field",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());
    return TDI_OBJECT_NOT_FOUND;
  }
  if (strField.find(field_id) != strField.end()) {
    auto it = strField.find(field_id);
    if (this->mirror_str_to_dir_map.find(it->second) ==
        this->mirror_str_to_dir_map.end()) {
      LOG_ERROR("%s:%d %s : Error : %s is not a valid direction",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                it->second.c_str());
      return TDI_INVALID_ARG;
    }
    s_info.dir = this->mirror_str_to_dir_map.at(it->second);
  }

  // Set the egress port
  field_id = tableInfoGet()->dataFieldIdGet("ucast_egress_port", action_id);
  if (field_id == 0) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for egress port field",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());

    return TDI_OBJECT_NOT_FOUND;
  }
  if (u64Field.find(field_id) != u64Field.end()) {
    auto it = u64Field.find(field_id);
    s_info.ucast_egress_port = static_cast<uint32_t>(it->second);
  }

  // Set the egress port valid
  field_id =
      tableInfoGet()->dataFieldIdGet("ucast_egress_port_valid", action_id);
  if (field_id == BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for egress port valid "
        "field",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());

    return status;
  }
  if (boolField.find(field_id) != boolField.end()) {
    auto it = boolField.find(field_id);
    s_info.ucast_egress_port_v = it->second;
  }

  // Set the egress port queue
  field_id = tableInfoGet()->dataFieldIdGet("egress_port_queue", action_id);
  if (field_id == 0) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for egress port queue "
        "field",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());

    return status;
  }
  if (u64Field.find(field_id) != u64Field.end()) {
    auto it = u64Field.find(field_id);
    s_info.egress_port_queue = static_cast<bf_tm_queue_t>(it->second);
  }

  // Set the ingress cos
  field_id = tableInfoGet()->dataFieldIdGet("ingress_cos", action_id);
  if (field_id == 0) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for ingress_cos field",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());

    return status;
  }
  if (u64Field.find(field_id) != u64Field.end()) {
    auto it = u64Field.find(field_id);
    s_info.ingress_cos = static_cast<uint32_t>(it->second);
  }

  // Set the packet color
  field_id = tableInfoGet()->dataFieldIdGet("packet_color", action_id);
  if (field_id == 0) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for packet color field",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());

    return status;
  }
  if (strField.find(field_id) != strField.end()) {
    auto it = strField.find(field_id);
    if (this->mirror_str_to_packet_color_map.find(it->second) ==
        this->mirror_str_to_packet_color_map.end()) {
      LOG_ERROR("%s:%d %s : Error : %s is not a valid color",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                it->second.c_str());
      return TDI_INVALID_ARG;
    }
    s_info.packet_color = this->mirror_str_to_packet_color_map.at(it->second);
  }

  // Set the L1 hash for multicast
  field_id = tableInfoGet()->dataFieldIdGet("level1_mcast_hash", action_id);
  if (field_id == 0) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for L1 mcast hash field",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());

    return status;
  }
  if (u64Field.find(field_id) != u64Field.end()) {
    auto it = u64Field.find(field_id);
    s_info.level1_mcast_hash = static_cast<uint32_t>(it->second);
  }

  // Set the L2 hash for multicast
  field_id = tableInfoGet()->dataFieldIdGet("level2_mcast_hash", action_id);
  if (field_id == 0) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for L2 mcast hash field",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());

    return status;
  }
  if (u64Field.find(field_id) != u64Field.end()) {
    auto it = u64Field.find(field_id);
    s_info.level2_mcast_hash = static_cast<uint32_t>(it->second);
  }

  // Set the multicast mgid a
  field_id = tableInfoGet()->dataFieldIdGet("mcast_grp_a", action_id);
  if (field_id == 0) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for mcast mgid a field",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());

    return status;
  }
  if (u64Field.find(field_id) != u64Field.end()) {
    auto it = u64Field.find(field_id);
    s_info.mcast_grp_a = static_cast<uint16_t>(it->second);
  }

  // Set the multicast mgid a valid
  field_id = tableInfoGet()->dataFieldIdGet("mcast_grp_a_valid", action_id);
  if (field_id == 0) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for mcast mgid a valid "
        "field",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());

    return status;
  }
  if (boolField.find(field_id) != boolField.end()) {
    auto it = boolField.find(field_id);
    s_info.mcast_grp_a_v = it->second;
  }

  // Set the multicast mgid b
  field_id = tableInfoGet()->dataFieldIdGet("mcast_grp_b", action_id);
  if (field_id == 0) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for mcast mgid b field",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());

    return status;
  }
  if (u64Field.find(field_id) != u64Field.end()) {
    auto it = u64Field.find(field_id);
    s_info.mcast_grp_b = static_cast<uint16_t>(it->second);
  }

  // Set the multicast mgid b valid
  field_id = tableInfoGet()->dataFieldIdGet("mcast_grp_b_valid", action_id);
  if (field_id == 0) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for mcast mgid b valid "
        "field",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());

    return status;
  }
  if (boolField.find(field_id) != boolField.end()) {
    auto it = boolField.find(field_id);
    s_info.mcast_grp_b_v = it->second;
  }

  // Set the multicast L1 xid
  field_id = tableInfoGet()->dataFieldIdGet("mcast_l1_xid", action_id);
  if (field_id == 0) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for mcast l1 xid field",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());

    return status;
  }
  if (u64Field.find(field_id) != u64Field.end()) {
    auto it = u64Field.find(field_id);
    s_info.mcast_l1_xid = static_cast<uint16_t>(it->second);
  }

  // Set the multicast L2 xid
  field_id = tableInfoGet()->dataFieldIdGet("mcast_l2_xid", action_id);
  if (field_id == 0) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for mcast l2 xid field",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());

    return status;
  }
  if (u64Field.find(field_id) != u64Field.end()) {
    auto it = u64Field.find(field_id);
    s_info.mcast_l2_xid = static_cast<uint16_t>(it->second);
  }

  // Set the multicast RID
  field_id = tableInfoGet()->dataFieldIdGet("mcast_rid", action_id);
  if (field_id == 0) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for mcast rid field",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());

    return status;
  }
  if (u64Field.find(field_id) != u64Field.end()) {
    auto it = u64Field.find(field_id);
    s_info.mcast_rid = static_cast<uint16_t>(it->second);
  }

  // Set the iCoS for copy-to-cpu packets
  field_id = tableInfoGet()->dataFieldIdGet("icos_for_copy_to_cpu", action_id);
  if (field_id == 0) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for iCoS for "
        "copy-to-cpu field",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());

    return status;
  }
  if (u64Field.find(field_id) != u64Field.end()) {
    auto it = u64Field.find(field_id);
    s_info.icos_for_copy_to_cpu = static_cast<uint32_t>(it->second);
  }

  // Set the copy-to-cpu field
  field_id = tableInfoGet()->dataFieldIdGet("copy_to_cpu", action_id);
  if (field_id == 0) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for copy-to-cpu field",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());

    return status;
  }
  if (boolField.find(field_id) != boolField.end()) {
    auto it = boolField.find(field_id);
    s_info.copy_to_cpu = it->second;
  }

  // Set the max packet length
  field_id = tableInfoGet()->dataFieldIdGet("max_pkt_len", action_id);
  if (field_id == 0) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for max packet length "
        "field",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());
    return status;
  }
  if (u64Field.find(field_id) != u64Field.end()) {
    auto it = u64Field.find(field_id);
    s_info.max_pkt_len = static_cast<uint16_t>(it->second);
  }

  // If max_pkt_len is not configured or set to 0, then mirrored
  // packet shouldn't be truncated. So, set max_pkt_len to MAX value
  if (s_info.max_pkt_len == 0) {
    s_info.max_pkt_len = 0xFFFF;
  }

  // Set the pipe mask if either multicast or copy-to-cpu is enabled
  // as pipe mask is not supposed to be user configurable & not
  // exposed to user, it is managed internally instead.
  if (s_info.mcast_grp_a_v || s_info.mcast_grp_b_v || s_info.copy_to_cpu) {
    // Only if multicast mirroring, set pipe vector.
    s_info.pipe_mask = 0xF;
  } else {
    s_info.pipe_mask = 0x0;
  }

  // Set the deflect on drop to 0 to avoid negative mirror loop
  s_info.deflect_on_drop = 0;

  // If the mirror session type is COALESCING, set fields
  // specific to coalescing session
  if (s_info.mirror_type == BF_MIRROR_TYPE_COAL) {
    // Set the internal heder
    field_id = tableInfoGet()->dataFieldIdGet("internal_header", action_id);
    if (field_id == 0) {
      LOG_TRACE(
          "%s:%d %s : Error : Failed to get the field id for internal header "
          "field",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str());
      return status;
    }

    if (arrayField.find(field_id) != arrayField.end()) {
      auto it = arrayField.find(field_id);
      std::memcpy(&s_info.header[0], &(it->second), TDI_MIRROR_INT_HDR_SIZE);
    }

    // Set the internal header length
    field_id =
        tableInfoGet()->dataFieldIdGet("internal_header_length", action_id);
    if (field_id == 0) {
      LOG_TRACE(
          "%s:%d %s : Error : Failed to get the field id for internal header "
          "length field",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str());
      return status;
    }
    if (u64Field.find(field_id) != u64Field.end()) {
      auto it = u64Field.find(field_id);
      s_info.header_len = static_cast<uint32_t>(it->second);
    }

    // Set the timeout value
    field_id = tableInfoGet()->dataFieldIdGet("timeout_usec", action_id);
    if (field_id == 0) {
      LOG_TRACE(
          "%s:%d %s : Error : Failed to get the field id for timeout field",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str());
      return status;
    }
    if (u64Field.find(field_id) != u64Field.end()) {
      auto it = u64Field.find(field_id);
      s_info.timeout_usec = static_cast<uint32_t>(it->second);
    }

    // Set the extract length value
    field_id = tableInfoGet()->dataFieldIdGet("extract_len", action_id);
    if (field_id == 0) {
      LOG_TRACE(
          "%s:%d %s : Error : Failed to get the field id for extract length "
          "field",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str());
      return status;
    }
    if (u64Field.find(field_id) != u64Field.end()) {
      auto it = u64Field.find(field_id);
      s_info.extract_len = static_cast<uint32_t>(it->second);
    }

    // If the user doesn't specify extract_length or configures it as 0,
    // then extract length has to be derived from P4.
    if (s_info.extract_len == 0) {
      s_info.extract_len_from_p4 = true;
    }
  }

  // Get the session enable field value
  bool session_enable = false;
  field_id = tableInfoGet()->dataFieldIdGet("session_enable", action_id);
  if (field_id == 0) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for session enable "
        "field",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());
    return status;
  }
  if (boolField.find(field_id) != boolField.end()) {
    auto it = boolField.find(field_id);
    session_enable = it->second;
  }

  // Update the session info now
  status = pipeMgr->pipeMgrMirrorSessionSet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      session_id,
      &s_info,
      session_enable);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s: Error in updating mirror session for id %d, status = %d",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        session_id,
        status);
    return status;
  }

  // Set the session priority if it is present in passed in data
  field_id = tableInfoGet()->dataFieldIdGet("session_priority", action_id);
  if (field_id != 0) {
    if (boolField.find(field_id) != boolField.end()) {
      bool session_priority = boolField.at(field_id);
      status = pipeMgr->pipeMgrMirrorSessionPriorityUpdate(
          session.handleGet(
              static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
          pipe_dev_tgt,
          session_id,
          session_priority);
      if (status != BF_SUCCESS) {
        LOG_TRACE(
            "%s:%d %s: Error in updating mirror session priority for id %d, "
            "status = %d",
            __func__,
            __LINE__,
            tableInfoGet()->nameGet().c_str(),
            session_id,
            status);
        return status;
      }
    }
  }

  // Set the hash cfg flag if it is present in passed in data
  field_id = tableInfoGet()->dataFieldIdGet("hash_cfg_flag", action_id);
  if (field_id != 0) {
    if (boolField.find(field_id) != boolField.end()) {
      bool hash_cfg_flag = boolField.at(field_id);
      status = pipeMgr->pipeMgrMirrorSessionMetaFlagUpdate(
          session.handleGet(
              static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
          pipe_dev_tgt,
          session_id,
          BF_HASH_CFG,
          hash_cfg_flag);
      if (status != BF_SUCCESS) {
        LOG_TRACE(
            "%s:%d %s: Error in updating mirror session hash config flag for "
            "id %d, status = %d",
            __func__,
            __LINE__,
            tableInfoGet()->nameGet().c_str(),
            session_id,
            status);
        return status;
      }
    }
  }

  // Set the hash cfg_p flag if it is present in passed in data
  field_id = tableInfoGet()->dataFieldIdGet("hash_cfg_flag_p", action_id);
  if (field_id != 0) {
    if (boolField.find(field_id) != boolField.end()) {
      bool hash_cfg_flag_p = boolField.at(field_id);
      status = pipeMgr->pipeMgrMirrorSessionMetaFlagUpdate(
          session.handleGet(
              static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
          pipe_dev_tgt,
          session_id,
          BF_HASH_CFG_P,
          hash_cfg_flag_p);
      if (status != BF_SUCCESS) {
        LOG_TRACE(
            "%s:%d %s: Error in updating mirror session hash cfg_p flag for id "
            "%d, status = %d",
            __func__,
            __LINE__,
            tableInfoGet()->nameGet().c_str(),
            session_id,
            status);
        return status;
      }
    }
  }

  // Set the iCoS cfg flag if it is present in passed in data
  field_id = tableInfoGet()->dataFieldIdGet("icos_cfg_flag", action_id);
  if (field_id != 0) {
    if (boolField.find(field_id) != boolField.end()) {
      bool icos_cfg_flag = boolField.at(field_id);
      status = pipeMgr->pipeMgrMirrorSessionMetaFlagUpdate(
          session.handleGet(
              static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
          pipe_dev_tgt,
          session_id,
          BF_ICOS_CFG,
          icos_cfg_flag);
      if (status != BF_SUCCESS) {
        LOG_TRACE(
            "%s:%d %s: Error in updating mirror session iCoS config flag for "
            "id %d, status = %d",
            __func__,
            __LINE__,
            tableInfoGet()->nameGet().c_str(),
            session_id,
            status);
        return status;
      }
    }
  }

  // Set the DoD cfg flag if it is present in passed in data
  field_id = tableInfoGet()->dataFieldIdGet("dod_cfg_flag", action_id);
  if (field_id != 0) {
    if (boolField.find(field_id) != boolField.end()) {
      bool dod_cfg_flag = boolField.at(field_id);
      status = pipeMgr->pipeMgrMirrorSessionMetaFlagUpdate(
          session.handleGet(
              static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
          pipe_dev_tgt,
          session_id,
          BF_DOD_CFG,
          dod_cfg_flag);
      if (status != BF_SUCCESS) {
        LOG_TRACE(
            "%s:%d %s: Error in updating mirror session DoD config flag for id "
            "%d, status = %d",
            __func__,
            __LINE__,
            tableInfoGet()->nameGet().c_str(),
            session_id,
            status);
        return status;
      }
    }
  }

  // Set the copy-to-cpu cfg flag if it is present in passed in data
  field_id = tableInfoGet()->dataFieldIdGet("c2c_cfg_flag", action_id);
  if (field_id != 0) {
    if (boolField.find(field_id) != boolField.end()) {
      bool c2c_cfg_flag = boolField.at(field_id);
      status = pipeMgr->pipeMgrMirrorSessionMetaFlagUpdate(
          session.handleGet(
              static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
          pipe_dev_tgt,
          session_id,
          BF_C2C_CFG,
          c2c_cfg_flag);
      if (status != BF_SUCCESS) {
        LOG_TRACE(
            "%s:%d %s: Error in updating mirror session copy-to-cpu config "
            "flag for id %d, status = %d",
            __func__,
            __LINE__,
            tableInfoGet()->nameGet().c_str(),
            session_id,
            status);
        return status;
      }
    }
  }

  // Set the multicast cfg flag if it is present in passed in data
  field_id = tableInfoGet()->dataFieldIdGet("mc_cfg_flag", action_id);
  if (field_id != 0) {
    if (boolField.find(field_id) != boolField.end()) {
      bool mc_cfg_flag = boolField.at(field_id);
      status = pipeMgr->pipeMgrMirrorSessionMetaFlagUpdate(
          session.handleGet(
              static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
          pipe_dev_tgt,
          session_id,
          BF_MC_CFG,
          mc_cfg_flag);
      if (status != BF_SUCCESS) {
        LOG_TRACE(
            "%s:%d %s: Error in updating mirror session multicast config flag "
            "for id %d, status = %d",
            __func__,
            __LINE__,
            tableInfoGet()->nameGet().c_str(),
            session_id,
            status);
        return status;
      }
    }
  }

  // Set the epipe cfg flag if it is present in passed in data
  field_id = tableInfoGet()->dataFieldIdGet("epipe_cfg_flag", action_id);
  if (field_id != 0) {
    if (boolField.find(field_id) != boolField.end()) {
      bool epipe_cfg_flag = boolField.at(field_id);
      status = pipeMgr->pipeMgrMirrorSessionMetaFlagUpdate(
          session.handleGet(
              static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
          pipe_dev_tgt,
          session_id,
          BF_EPIPE_CFG,
          epipe_cfg_flag);
      if (status != BF_SUCCESS) {
        LOG_TRACE(
            "%s:%d %s: Error in updating mirror session epipe config flag for "
            "id %d, status = %d",
            __func__,
            __LINE__,
            tableInfoGet()->nameGet().c_str(),
            session_id,
            status);
        return status;
      }
    }
  }

  // If the session type is COALESCING, set the coalesing mode
  // if it is present in passed in data
  if (s_info.mirror_type == BF_MIRROR_TYPE_COAL) {
    field_id = tableInfoGet()->dataFieldIdGet("session_coal_mode", action_id);
    if (field_id != 0) {
      if (boolField.find(field_id) != boolField.end()) {
        bool session_coal_mode = boolField.at(field_id);
        status = pipeMgr->pipeMgrMirrorSessionCoalModeUpdate(
            session.handleGet(
                static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
            pipe_dev_tgt,
            session_id,
            session_coal_mode);
        if (status != BF_SUCCESS) {
          LOG_TRACE(
              "%s:%d %s: Error in updating mirror session coalescing mode for "
              "id %d, status = %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              session_id,
              status);
          return status;
        }
      }
    }
  }

  return BF_SUCCESS;
}

tdi_status_t MirrorCfgTable::entryDel(const tdi::Session &session,
                                      const tdi::Target &dev_tgt,
                                      const tdi::Flags & /*flags*/,
                                      const tdi::TableKey &key) const {
  tdi_status_t status = BF_SUCCESS;

  const MirrorCfgTableKey &mirror_key =
      static_cast<const MirrorCfgTableKey &>(key);

  // First get the mirror session id from passed in key
  const int &session_id = static_cast<const int &>(mirror_key.getId());

  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  // Get the mirror session_info struct & session_enable field from pipe manager
  bf_mirror_session_info_t s_info;
  // Initialize the mirror_session_info struct to all 0
  std::memset(&s_info, 0, sizeof(s_info));
  status = pipeMgr->pipeMgrMirrorSessionGet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      session_id,
      &s_info);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s: Error: mirror session with id %d does not exist, status = "
        "%d",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        session_id,
        status);
    return TDI_INVALID_ARG;
  }
  // For mirror config table, deleting the table entry is
  // resetting the mirror session config in HW. So, this
  // operation can be done even if the session entry is not
  // created and hence check for whether table entry exists
  // or not is not needed here.
  status = pipeMgr->pipeMgrMirrorSessionReset(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      session_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s: Error in deleting/resetting mirror session for id %d, "
        "status = %d",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        session_id,
        status);
    return status;
  }

  return status;
}

tdi_status_t MirrorCfgTable::usageGet(const tdi::Session &session,
                                      const tdi::Target &dev_tgt,
                                      const tdi::Flags & /*flags*/,
                                      uint32_t *count) const {
  tdi_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  status = pipeMgr->pipeMgrMirrorSessionCountGet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      count);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "8010d0d2"
        " %s %s: Error in getting mirror table Usage ",
        __func__,
        tableInfoGet()->nameGet().c_str());
    return status;
  }
  return status;
}

tdi_status_t MirrorCfgTable::entryGetFirst(const tdi::Session &session,
                                           const tdi::Target &dev_tgt,
                                           const tdi::Flags & /*flags*/,
                                           tdi::TableKey *key,
                                           tdi::TableData *data) const {
  tdi_status_t status = BF_SUCCESS;
  MirrorCfgTableKey *mirror_key = static_cast<MirrorCfgTableKey *>(key);

  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  bf_mirror_session_info_t s_info;
  bf_mirror_get_id_t first_id = {0};
  memset(&s_info, 0, sizeof(s_info));
  // Get first mirror session id from pipe manager
  status = pipeMgr->pipeMgrMirrorSessionGetFirst(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      &s_info,
      &first_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s: Error in getting first mirror session id, status = %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }
  bool session_enable = false;
  status = pipeMgr->pipeMgrMirrorSessionEnableGet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      first_id.sid,
      &session_enable);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s: Error in getting mirror session_enable field for id %d, "
        "status = %d",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        first_id.sid,
        status);
    return status;
  }

  mirror_key->setId(first_id.sid);
  return this->entryGetInternal(
      session, pipe_dev_tgt, s_info, first_id.sid, session_enable, data);
}

tdi_status_t MirrorCfgTable::entryGetNextN(const tdi::Session &session,
                                           const tdi::Target &dev_tgt,
                                           const tdi::Flags & /*flags*/,
                                           const tdi::TableKey &key,
                                           const uint32_t &n,
                                           keyDataPairs *key_data_pairs,
                                           uint32_t *num_returned) const {
  tdi_status_t status = BF_SUCCESS;
  if (!num_returned) {
    LOG_TRACE("%s:%d %s: Invalid Argument : num_returned cannot be NULL",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  const MirrorCfgTableKey &mirror_key =
      static_cast<const MirrorCfgTableKey &>(key);
  const auto session_id = mirror_key.getId();

  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  uint32_t i = 0;
  bf_mirror_get_id_t current_id = {session_id, pipe_dev_tgt.dev_pipe_id};
  for (i = 0; i < n; i++) {
    bf_mirror_session_info_t next_info;
    bf_mirror_get_id_t next_id;
    status = pipeMgr->pipeMgrMirrorSessionGetNext(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
        pipe_dev_tgt,
        current_id,
        &next_info,
        &next_id);
    if (status != BF_SUCCESS) {
      if (BF_OBJECT_NOT_FOUND == status) {
        break;
      } else {
        LOG_ERROR(
            "%s:%d %s: Error in getting %dth mirror session after key with "
            "session id %d, status = %d",
            __func__,
            __LINE__,
            tableInfoGet()->nameGet().c_str(),
            i,
            session_id,
            status);
      }
      break;
    }
    bool session_enable = false;
    status = pipeMgr->pipeMgrMirrorSessionEnableGet(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
        pipe_dev_tgt,
        session_id,
        &session_enable);
    if (status != BF_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s: Error in getting mirror session_enable field for id %d, "
          "status = %d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          session_id,
          status);
      break;
    }

    auto this_key =
        static_cast<MirrorCfgTableKey *>((*key_data_pairs)[i].first);
    auto this_data = (*key_data_pairs)[i].second;
    this_key->setId(next_id.sid);
    status = this->entryGetInternal(session,
                                    pipe_dev_tgt,
                                    next_info,
                                    next_id.sid,
                                    session_enable,
                                    this_data);
    if (BF_SUCCESS != status) {
      LOG_ERROR(
          "%s:%d %s: Error in getting %dth entry after key with session id "
          "%d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          i,
          session_id);
      break;
    }
    current_id = next_id;
    *num_returned += 1;
  }
  return status;
}

tdi_status_t MirrorCfgTable::entryGet(const tdi::Session &session,
                                      const tdi::Target &dev_tgt,
                                      const tdi::Flags & /*flags*/,
                                      const tdi::TableKey &key,
                                      tdi::TableData *data) const {
  tdi_status_t status = BF_SUCCESS;

  const MirrorCfgTableKey &mirror_key =
      static_cast<const MirrorCfgTableKey &>(key);

  // First get the mirror session id from passed in key
  const int &session_id = static_cast<const int &>(mirror_key.getId());

  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  dev_target_t pipe_dev_tgt;
  auto tof_target = static_cast<const tdi::tna::tofino::Target *>(&dev_tgt);
  tof_target->getTargetVals(&pipe_dev_tgt, nullptr, nullptr);

  // Get the mirror session_info struct & session_enable field from pipe manager
  bf_mirror_session_info_t s_info;
  // Initialize the mirror_session_info struct to all 0
  std::memset(&s_info, 0, sizeof(s_info));
  status = pipeMgr->pipeMgrMirrorSessionGet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      session_id,
      &s_info);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s: Error in getting mirror session info for id %d, status = %d",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        session_id,
        status);
    return status;
  }
  bool session_enable = false;
  status = pipeMgr->pipeMgrMirrorSessionEnableGet(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      session_id,
      &session_enable);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s: Error in getting mirror session_enable field for id %d, "
        "status = %d",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        session_id,
        status);
    return status;
  }

  return this->entryGetInternal(
      session, pipe_dev_tgt, s_info, session_id, session_enable, data);
}

tdi_status_t MirrorCfgTable::entryGetInternal(
    const tdi::Session &session,
    const dev_target_t pipe_dev_tgt,
    const bf_mirror_session_info_t &s_info,
    const bf_mirror_id_t &session_id,
    const bool &session_enable,
    tdi::TableData *data) const {
  tdi_status_t status = BF_SUCCESS;

  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  MirrorCfgTableData *mirror_data = static_cast<MirrorCfgTableData *>(data);
  bool is_active = false;
  auto action_id = mirror_data->actionIdGet();
  tdi_id_t pipe_action_id;

  // Identify the mirror session type and get the corresponding action id
  if (s_info.mirror_type == BF_MIRROR_TYPE_COAL) {
    auto actionInfo = tableInfoGet()->actionGet("coalescing");
    pipe_action_id = actionInfo->idGet();
  } else {
    auto actionInfo = tableInfoGet()->actionGet("normal");
    pipe_action_id = actionInfo->idGet();
  }
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s: Error in getting action id for mirror session type %d for "
        "id %d, status = %d",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        s_info.mirror_type,
        session_id,
        status);
    return status;
  }

  // Make sure both action ids are same
  if (action_id != pipe_action_id) {
    if (0 == action_id) {
      // action id 0 is used when *data is a pure output param
      // We have to use pipe_action_id to set data's action_id
      this->dataReset(pipe_action_id, data);
      action_id = pipe_action_id;
    } else {
      LOG_TRACE(
          "%s:%d %s: Error in getting mirror session info%d for id %d, given "
          "action %d doesn't match with configured action id %d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          s_info.mirror_type,
          session_id,
          action_id,
          pipe_action_id);
      return BF_UNEXPECTED;
    }
  }

  // Set the common mirror session parameters in data
  // For each filed name in the mirror session config, get the
  // field id and check if it is activet in the data and set
  // the value in mirror config table data to be returned  to caller
  tdi_id_t field_id;

  // Set the session_enable for the mirror session in data
  field_id = tableInfoGet()->dataFieldIdGet("session_enable", action_id);
  if (field_id == 0) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for session_enable "
        "field",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());
    return status;
  }
  mirror_data->isActive(field_id, &is_active);
  if (is_active) {
    // Get the mirror sessionEnable Field info from pipe manager
    mirror_data->setValue(field_id, session_enable);
  }

  // Set the direction for the mirror session in data
  field_id = tableInfoGet()->dataFieldIdGet("direction", action_id);
  if (field_id == 0) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for direction field",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());
    return status;
  }
  mirror_data->isActive(field_id, &is_active);
  if (is_active) {
    std::string direction = this->mirror_dir_to_str_map.at(s_info.dir);
    mirror_data->setValue(field_id, direction);
  }

  // Set the egress port
  field_id = tableInfoGet()->dataFieldIdGet("ucast_egress_port", action_id);
  if (field_id == 0) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for egress port field",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());
    return status;
  }
  mirror_data->isActive(field_id, &is_active);
  if (is_active) {
    mirror_data->setValue(field_id,
                          static_cast<uint64_t>(s_info.ucast_egress_port));
  }

  // Set the egress port valid
  field_id =
      tableInfoGet()->dataFieldIdGet("ucast_egress_port_valid", action_id);
  if (field_id == 0) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for egress port valid "
        "field",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());
    return status;
  }
  mirror_data->isActive(field_id, &is_active);
  if (is_active) {
    mirror_data->setValue(field_id,
                          static_cast<bool>(s_info.ucast_egress_port_v));
  }

  // Set the egress port queue
  field_id = tableInfoGet()->dataFieldIdGet("egress_port_queue", action_id);
  if (field_id == 0) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for egress port queue "
        "field",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());
    return status;
  }
  mirror_data->isActive(field_id, &is_active);
  if (is_active) {
    mirror_data->setValue(field_id,
                          static_cast<uint64_t>(s_info.egress_port_queue));
  }

  // Set the ingress cos
  field_id = tableInfoGet()->dataFieldIdGet("ingress_cos", action_id);
  if (field_id == 0) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for ingress_cos field",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());
    return status;
  }
  mirror_data->isActive(field_id, &is_active);
  if (is_active) {
    mirror_data->setValue(field_id, static_cast<uint64_t>(s_info.ingress_cos));
  }

  // Set the packet color
  field_id = tableInfoGet()->dataFieldIdGet("packet_color", action_id);
  if (field_id == 0) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for packet color field",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());
    return status;
  }
  mirror_data->isActive(field_id, &is_active);
  if (is_active) {
    std::string packet_color =
        this->mirror_packet_color_to_str_map.at(s_info.packet_color);
    mirror_data->setValue(field_id, packet_color);
  }

  // Set the L1 hash for multicast
  field_id = tableInfoGet()->dataFieldIdGet("level1_mcast_hash", action_id);
  if (field_id == 0) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for L1 mcast hash field",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());
    return status;
  }
  mirror_data->isActive(field_id, &is_active);
  if (is_active) {
    mirror_data->setValue(field_id,
                          static_cast<uint64_t>(s_info.level1_mcast_hash));
  }

  // Set the L2 hash for multicast
  field_id = tableInfoGet()->dataFieldIdGet("level2_mcast_hash", action_id);
  if (field_id == 0) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for L2 mcast hash field",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());
    return status;
  }
  mirror_data->isActive(field_id, &is_active);
  if (is_active) {
    mirror_data->setValue(field_id,
                          static_cast<uint64_t>(s_info.level2_mcast_hash));
  }

  // Set the multicast mgid a
  field_id = tableInfoGet()->dataFieldIdGet("mcast_grp_a", action_id);
  if (field_id == 0) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for mcast mgid a field",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());
    return status;
  }
  mirror_data->isActive(field_id, &is_active);
  if (is_active) {
    mirror_data->setValue(field_id, static_cast<uint64_t>(s_info.mcast_grp_a));
  }

  // Set the multicast mgid a valid
  field_id = tableInfoGet()->dataFieldIdGet("mcast_grp_a_valid", action_id);
  if (field_id == 0) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for mcast mgid a valid "
        "field",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());
    return status;
  }
  mirror_data->isActive(field_id, &is_active);
  if (is_active) {
    mirror_data->setValue(field_id, static_cast<bool>(s_info.mcast_grp_a_v));
  }

  // Set the multicast mgid b
  field_id = tableInfoGet()->dataFieldIdGet("mcast_grp_b", action_id);
  if (field_id == 0) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for mcast mgid b field",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());
    return status;
  }
  mirror_data->isActive(field_id, &is_active);
  if (is_active) {
    mirror_data->setValue(field_id, static_cast<uint64_t>(s_info.mcast_grp_b));
  }

  // Set the multicast mgid b valid
  field_id = tableInfoGet()->dataFieldIdGet("mcast_grp_b_valid", action_id);
  if (field_id == 0) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for mcast mgid b valid "
        "field",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());
    return status;
  }
  mirror_data->isActive(field_id, &is_active);
  if (is_active) {
    mirror_data->setValue(field_id, static_cast<bool>(s_info.mcast_grp_b_v));
  }

  // Set the multicast L1 xid
  field_id = tableInfoGet()->dataFieldIdGet("mcast_l1_xid", action_id);
  if (field_id == 0) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for mcast l1 xid field",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());
    return status;
  }
  mirror_data->isActive(field_id, &is_active);
  if (is_active) {
    mirror_data->setValue(field_id, static_cast<uint64_t>(s_info.mcast_l1_xid));
  }

  // Set the multicast L2 xid
  field_id = tableInfoGet()->dataFieldIdGet("mcast_l2_xid", action_id);
  if (field_id == 0) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for mcast l2 xid field",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());
    return status;
  }
  mirror_data->isActive(field_id, &is_active);
  if (is_active) {
    mirror_data->setValue(field_id, static_cast<uint64_t>(s_info.mcast_l2_xid));
  }

  // Set the multicast RID
  field_id = tableInfoGet()->dataFieldIdGet("mcast_rid", action_id);
  if (field_id == 0) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for mcast rid field",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());
    return status;
  }
  mirror_data->isActive(field_id, &is_active);
  if (is_active) {
    mirror_data->setValue(field_id, static_cast<uint64_t>(s_info.mcast_rid));
  }

  // Set the iCoS for copy-to-cpu packets
  field_id = tableInfoGet()->dataFieldIdGet("icos_for_copy_to_cpu", action_id);
  if (field_id == 0) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for iCoS for "
        "copy-to-cpu field",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());
    return status;
  }
  mirror_data->isActive(field_id, &is_active);
  if (is_active) {
    mirror_data->setValue(field_id,
                          static_cast<uint64_t>(s_info.icos_for_copy_to_cpu));
  }

  // Set the copy-to-cpu field
  field_id = tableInfoGet()->dataFieldIdGet("copy_to_cpu", action_id);
  if (field_id == 0) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for copy-to-cpu field",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());

    return status;
  }
  mirror_data->isActive(field_id, &is_active);
  if (is_active) {
    mirror_data->setValue(field_id, static_cast<bool>(s_info.copy_to_cpu));
  }

  // Set the max packet length
  field_id = tableInfoGet()->dataFieldIdGet("max_pkt_len", action_id);
  if (field_id == 0) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for max packet length "
        "field",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());

    return status;
  }
  mirror_data->isActive(field_id, &is_active);
  if (is_active) {
    uint16_t max_pkt_len = s_info.max_pkt_len;
    // If max pkt len is 0xFFFF, then set its value to 0;
    if (max_pkt_len == 0xFFFF) {
      max_pkt_len = 0;
    }
    mirror_data->setValue(field_id, static_cast<uint64_t>(max_pkt_len));
  }

  // If the mirror session type is COALESCING, set fields
  // specific to coalescing session
  if (s_info.mirror_type == BF_MIRROR_TYPE_COAL) {
    // Set the internal heder
    field_id = tableInfoGet()->dataFieldIdGet("internal_header", action_id);
    if (field_id == 0) {
      LOG_TRACE(
          "%s:%d %s : Error : Failed to get the field id for internal header "
          "field",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str());
      return status;
    }
    mirror_data->isActive(field_id, &is_active);
    if (is_active) {
      mirror_data->setValue(
          field_id,
          reinterpret_cast<const uint8_t *>(&s_info.header[0]),
          TDI_MIRROR_INT_HDR_SIZE);
    }

    // Set the internal header length
    field_id =
        tableInfoGet()->dataFieldIdGet("internal_header_length", action_id);
    if (field_id == 0) {
      LOG_TRACE(
          "%s:%d %s : Error : Failed to get the field id for internal header "
          "length field",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str());

      return status;
    }
    mirror_data->isActive(field_id, &is_active);
    if (is_active) {
      mirror_data->setValue(field_id, static_cast<uint64_t>(s_info.header_len));
    }

    // Set the timeout value
    field_id = tableInfoGet()->dataFieldIdGet("timeout_usec", action_id);
    if (field_id == 0) {
      LOG_TRACE(
          "%s:%d %s : Error : Failed to get the field id for timeout field",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str());

      return status;
    }
    mirror_data->isActive(field_id, &is_active);
    if (is_active) {
      mirror_data->setValue(field_id,
                            static_cast<uint64_t>(s_info.timeout_usec));
    }

    // Set the extract length value
    field_id = tableInfoGet()->dataFieldIdGet("extract_len", action_id);
    if (field_id == 0) {
      LOG_TRACE(
          "%s:%d %s : Error : Failed to get the field id for extract length "
          "field",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str());

      return status;
    }
    mirror_data->isActive(field_id, &is_active);
    if (is_active) {
      mirror_data->setValue(field_id,
                            static_cast<uint64_t>(s_info.extract_len));
    }
  }

  // Set the session priority in data for output tofino2 only
  field_id = tableInfoGet()->dataFieldIdGet("session_priority", action_id);
  if (field_id != 0) {
    bool session_priority;
    status = pipeMgr->pipeMgrMirrorSessionPriorityGet(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
        pipe_dev_tgt,
        session_id,
        &session_priority);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s: Error: Failed to get mirror session priority for id %d, "
          "status = %d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          session_id,
          status);
      return status;
    }
    mirror_data->isActive(field_id, &is_active);
    if (is_active) {
      mirror_data->setValue(field_id, static_cast<bool>(session_priority));
    }
  }

  // Set the hash cfg flag if it is present in passed in data
  field_id = tableInfoGet()->dataFieldIdGet("hash_cfg_flag", action_id);
  if (field_id != 0) {
    bool hash_cfg_flag;
    status = pipeMgr->pipeMgrMirrorSessionMetaFlagGet(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
        pipe_dev_tgt,
        session_id,
        BF_HASH_CFG,
        &hash_cfg_flag);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s: Error: Failed to get mirror session hash config flag for "
          "id %d, status = %d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          session_id,
          status);
      return status;
    }
    mirror_data->isActive(field_id, &is_active);
    if (is_active) {
      mirror_data->setValue(field_id, static_cast<bool>(hash_cfg_flag));
    }
  }

  // Set the hash cfg flag p if it is present in passed in data
  field_id = tableInfoGet()->dataFieldIdGet("hash_cfg_flag_p", action_id);
  if (field_id != 0) {
    bool hash_cfg_flag_p;
    status = pipeMgr->pipeMgrMirrorSessionMetaFlagGet(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
        pipe_dev_tgt,
        session_id,
        BF_HASH_CFG_P,
        &hash_cfg_flag_p);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s: Error: Failed to get mirror session hash cfg_p flag for "
          "id %d, status = %d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          session_id,
          status);
      return status;
    }
    mirror_data->isActive(field_id, &is_active);
    if (is_active) {
      mirror_data->setValue(field_id, static_cast<bool>(hash_cfg_flag_p));
    }
  }

  field_id = tableInfoGet()->dataFieldIdGet("icos_cfg_flag", action_id);
  if (field_id != 0) {
    bool icos_cfg_flag;
    status = pipeMgr->pipeMgrMirrorSessionMetaFlagGet(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
        pipe_dev_tgt,
        session_id,
        BF_ICOS_CFG,
        &icos_cfg_flag);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s: Error: Failed to get mirror session iCos config flag for "
          "id %d, status = %d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          session_id,
          status);
      return status;
    }
    mirror_data->isActive(field_id, &is_active);
    if (is_active) {
      mirror_data->setValue(field_id, static_cast<bool>(icos_cfg_flag));
    }
  }

  field_id = tableInfoGet()->dataFieldIdGet("dod_cfg_flag", action_id);
  if (field_id != 0) {
    bool dod_cfg_flag;
    status = pipeMgr->pipeMgrMirrorSessionMetaFlagGet(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
        pipe_dev_tgt,
        session_id,
        BF_DOD_CFG,
        &dod_cfg_flag);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s: Error: Failed to get mirror session DoD config flag for "
          "id %d, status = %d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          session_id,
          status);
      return status;
    }
    mirror_data->isActive(field_id, &is_active);
    if (is_active) {
      mirror_data->setValue(field_id, static_cast<bool>(dod_cfg_flag));
    }
  }

  field_id = tableInfoGet()->dataFieldIdGet("c2c_cfg_flag", action_id);
  if (field_id != 0) {
    bool c2c_cfg_flag;
    status = pipeMgr->pipeMgrMirrorSessionMetaFlagGet(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
        pipe_dev_tgt,
        session_id,
        BF_C2C_CFG,
        &c2c_cfg_flag);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s: Error: Failed to get mirror session copy-to-cpu config "
          "flag for "
          "id %d, status = %d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          session_id,
          status);
      return status;
    }
    mirror_data->isActive(field_id, &is_active);
    if (is_active) {
      mirror_data->setValue(field_id, static_cast<bool>(c2c_cfg_flag));
    }
  }

  field_id = tableInfoGet()->dataFieldIdGet("mc_cfg_flag", action_id);
  if (field_id != 0) {
    bool mc_cfg_flag;
    status = pipeMgr->pipeMgrMirrorSessionMetaFlagGet(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
        pipe_dev_tgt,
        session_id,
        BF_MC_CFG,
        &mc_cfg_flag);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s: Error: Failed to get mirror session multicast config flag "
          "for "
          "id %d, status = %d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          session_id,
          status);
      return status;
    }
    mirror_data->isActive(field_id, &is_active);
    if (is_active) {
      mirror_data->setValue(field_id, static_cast<bool>(mc_cfg_flag));
    }
  }

  field_id = tableInfoGet()->dataFieldIdGet("epipe_cfg_flag", action_id);
  if (field_id != 0) {
    bool epipe_cfg_flag;
    status = pipeMgr->pipeMgrMirrorSessionMetaFlagGet(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
        pipe_dev_tgt,
        session_id,
        BF_EPIPE_CFG,
        &epipe_cfg_flag);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s: Error: Failed to get mirror session epipe config flag for "
          "id %d, status = %d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          session_id,
          status);
      return status;
    }
    mirror_data->isActive(field_id, &is_active);
    if (is_active) {
      mirror_data->setValue(field_id, static_cast<bool>(epipe_cfg_flag));
    }
  }

  // If the session type is COALESCING, get the coalesing mode in tf2
  // if it is present in passed in data
  if (s_info.mirror_type == BF_MIRROR_TYPE_COAL) {
    field_id = tableInfoGet()->dataFieldIdGet("session_coal_mode", action_id);
    if (field_id != 0) {
      bool session_coal_mode;
      status = pipeMgr->pipeMgrMirrorSessionCoalModeGet(
          session.handleGet(
              static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_PIPE_MGR)),
          pipe_dev_tgt,
          session_id,
          &session_coal_mode);
      if (status != BF_SUCCESS) {
        LOG_TRACE(
            "%s:%d %s: Error in updating mirror session coalescing mode for "
            "id %d, status = %d",
            __func__,
            __LINE__,
            tableInfoGet()->nameGet().c_str(),
            session_id,
            status);
        return status;
      }
      mirror_data->isActive(field_id, &is_active);
      if (is_active) {
        mirror_data->setValue(field_id, static_cast<bool>(session_coal_mode));
      }
    }
  }

  return BF_SUCCESS;
}

tdi_status_t MirrorCfgTable::keyAllocate(
    std::unique_ptr<tdi::TableKey> *key_ret) const {
  *key_ret = std::unique_ptr<tdi::TableKey>(new MirrorCfgTableKey(this));

  if (*key_ret == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to allocate key",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return BF_NO_SYS_RESOURCES;
  }

  return BF_SUCCESS;
}

tdi_status_t MirrorCfgTable::keyReset(tdi::TableKey *key) const {
  if (key == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to reset key",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return BF_OBJECT_NOT_FOUND;
  }
  return (static_cast<MirrorCfgTableKey *>(key))->reset();
}

tdi_status_t MirrorCfgTable::dataReset(tdi::TableData *data) const {
  std::vector<tdi_id_t> emptyFields;
  return this->dataReset(emptyFields, 0, data);
}

tdi_status_t MirrorCfgTable::dataReset(const tdi_id_t &action_id,
                                       tdi::TableData *data) const {
  std::vector<tdi_id_t> emptyFields;
  return this->dataReset(emptyFields, action_id, data);
}

tdi_status_t MirrorCfgTable::dataReset(const std::vector<tdi_id_t> &fields,
                                       const tdi_id_t &action_id,
                                       tdi::TableData *data) const {
  if (data == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to reset data",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return BF_OBJECT_NOT_FOUND;
  }
  return (static_cast<MirrorCfgTableData *>(data))->reset(action_id, 0, fields);
}

tdi_status_t MirrorCfgTable::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  std::vector<tdi_id_t> fields;

  *data_ret =
      std::unique_ptr<tdi::TableData>(new MirrorCfgTableData(this, 0, fields));
  if (*data_ret == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to allocate data",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return BF_NO_SYS_RESOURCES;
  }

  return BF_SUCCESS;
}

tdi_status_t MirrorCfgTable::dataAllocate(
    const tdi_id_t &action_id,
    std::unique_ptr<tdi::TableData> *data_ret) const {
  std::vector<tdi_id_t> fields;

  *data_ret = std::unique_ptr<tdi::TableData>(
      new MirrorCfgTableData(this, action_id, fields));
  if (*data_ret == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to allocate data",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return BF_NO_SYS_RESOURCES;
  }

  return BF_SUCCESS;
}

tdi_status_t MirrorCfgTable::dataAllocate(
    const std::vector<tdi_id_t> &fields,
    std::unique_ptr<tdi::TableData> *data_ret) const {
  *data_ret =
      std::unique_ptr<tdi::TableData>(new MirrorCfgTableData(this, 0, fields));

  if (*data_ret == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to allocate data",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return BF_NO_SYS_RESOURCES;
  }

  return BF_SUCCESS;
}

tdi_status_t MirrorCfgTable::dataAllocate(
    const std::vector<tdi_id_t> &fields,
    const tdi_id_t &action_id,
    std::unique_ptr<tdi::TableData> *data_ret) const {
  *data_ret = std::unique_ptr<tdi::TableData>(
      new MirrorCfgTableData(this, action_id, fields));

  if (*data_ret == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to allocate data",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return BF_NO_SYS_RESOURCES;
  }

  return BF_SUCCESS;
}

}  // namespace tofino
}  // namespace tna
}  // namespace tdi
