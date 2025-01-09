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


#include <array>
#include <bf_rt_common/bf_rt_init_impl.hpp>
#include <bf_rt_common/bf_rt_pipe_mgr_intf.hpp>

#include "bf_rt_mirror_table_impl.hpp"
#include "bf_rt_mirror_table_data_impl.hpp"
#include "bf_rt_mirror_table_key_impl.hpp"

namespace bfrt {

const std::map<std::string, bf_mirror_direction_e>
    BfRtMirrorCfgTable::mirror_str_to_dir_map{{"INGRESS", BF_DIR_INGRESS},
                                              {"EGRESS", BF_DIR_EGRESS},
                                              {"BOTH", BF_DIR_BOTH}};

const std::map<bf_mirror_direction_e, std::string>
    BfRtMirrorCfgTable::mirror_dir_to_str_map{{BF_DIR_INGRESS, "INGRESS"},
                                              {BF_DIR_EGRESS, "EGRESS"},
                                              {BF_DIR_BOTH, "BOTH"},
                                              {BF_DIR_NONE, "NONE"}};

const std::map<std::string, bf_tm_color_t>
    BfRtMirrorCfgTable::mirror_str_to_packet_color_map{
        {"GREEN", BF_TM_COLOR_GREEN},
        {"YELLOW", BF_TM_COLOR_YELLOW},
        {"RED", BF_TM_COLOR_RED}};

const std::map<bf_tm_color_t, std::string>
    BfRtMirrorCfgTable::mirror_packet_color_to_str_map{
        {BF_TM_COLOR_GREEN, "GREEN"},
        {BF_TM_COLOR_YELLOW, "YELLOW"},
        {BF_TM_COLOR_RED, "RED"}};

bf_status_t BfRtMirrorCfgTable::tableEntryAdd(const BfRtSession &session,
                                              const bf_rt_target_t &dev_tgt,
                                              const uint64_t & /*flags*/,
                                              const BfRtTableKey &key,
                                              const BfRtTableData &data) const {
  bf_status_t status;

  const BfRtMirrorCfgTableKey &mirror_key =
      static_cast<const BfRtMirrorCfgTableKey &>(key);

  // First get the mirror session id from passed in key
  const int &session_id = static_cast<const int &>(mirror_key.getId());

  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  // Get the mirror session_info struct & session_enable field from pipe manager
  bf_mirror_session_info_t s_info;
  // Initialize the mirror_session_info struct to all 0
  std::memset(&s_info, 0, sizeof(s_info));
  status = pipeMgr->pipeMgrMirrorSessionGet(
      session.sessHandleGet(), pipe_dev_tgt, session_id, &s_info);
  // a success for this pipe_mgr call is actually an error for `Add`
  // since the entry should not exist before add
  if (status == BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s: Error: mirror session with id %d already exists, status "
        "= %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        session_id,
        status);
    return BF_INVALID_ARG;
  }
  // Else, for mirror config table, add and modify operations are same.
  // This is true for pipe_mgr API as well.
  return this->tableEntryModInternal(session, dev_tgt, key, data);
}

bf_status_t BfRtMirrorCfgTable::tableEntryMod(const BfRtSession &session,
                                              const bf_rt_target_t &dev_tgt,
                                              const uint64_t & /*flags*/,
                                              const BfRtTableKey &key,
                                              const BfRtTableData &data) const {
  bf_status_t status;

  const BfRtMirrorCfgTableKey &mirror_key =
      static_cast<const BfRtMirrorCfgTableKey &>(key);

  // First get the mirror session id from passed in key
  const int &session_id = static_cast<const int &>(mirror_key.getId());

  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  // Get the mirror session_info struct & session_enable field from pipe manager
  bf_mirror_session_info_t s_info;
  // Initialize the mirror_session_info struct to all 0
  std::memset(&s_info, 0, sizeof(s_info));
  status = pipeMgr->pipeMgrMirrorSessionGet(
      session.sessHandleGet(), pipe_dev_tgt, session_id, &s_info);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s: Error: mirror session with id %d does not exist, status = "
        "%d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        session_id,
        status);
    return BF_INVALID_ARG;
  }
  // Else, for mirror config table, add and modify operations are same.
  // This is true for pipe_mgr API as well.
  return this->tableEntryModInternal(session, dev_tgt, key, data);
}

bf_status_t BfRtMirrorCfgTable::tableEntryModInternal(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const BfRtTableKey &key,
    const BfRtTableData &data) const {
  bf_status_t status;

  const BfRtMirrorCfgTableKey &mirror_key =
      static_cast<const BfRtMirrorCfgTableKey &>(key);
  const BfRtMirrorCfgTableData &mirror_data =
      static_cast<const BfRtMirrorCfgTableData &>(data);

  // First get the mirror session id from passed in key
  const int &session_id = static_cast<const int &>(mirror_key.getId());

  const std::unordered_map<bf_rt_id_t, bool> &boolField =
      mirror_data.getBoolFieldDataMap();
  const std::unordered_map<bf_rt_id_t, uint64_t> &u64Field =
      mirror_data.getU64FieldDataMap();
  const std::unordered_map<bf_rt_id_t, std::string> &strField =
      mirror_data.getStrFieldDataMap();
  const std::unordered_map<bf_rt_id_t,
                           std::array<uint8_t, BF_RT_MIRROR_INT_HDR_SIZE>>
      &arrayField = mirror_data.getArrayFieldDataMap();

  // Get the action id from data
  bf_rt_id_t action_id;
  mirror_data.actionIdGet(&action_id);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  // Check and proceed only if action id is not 0
  if (action_id == 0) {
    LOG_TRACE("%s:%d %s : Error : Action id is invalid (0) ",
              __func__,
              __LINE__,
              table_name_get().c_str());

    return BF_INVALID_ARG;
  }

  // Get the action name from action id
  std::string action_name;
  status = this->actionNameGet(action_id, &action_name);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get action name for action id %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              action_id);

    return status;
  }

  // Initialize the mirror_session_info struct to all 0
  bf_mirror_session_info_t s_info;
  std::memset(&s_info, 0, sizeof(s_info));

  // Set the mirror session type based on action name
  if (action_name == "$coalescing") {
    s_info.mirror_type = BF_MIRROR_TYPE_COAL;
  } else {
    s_info.mirror_type = BF_MIRROR_TYPE_NORM;
  }

  bf_rt_id_t field_id;
  // Set the common mirror session parameters
  // For each filed name in the mirror session config, get the
  // field id and check if it is present in the input data and set
  // the value in mirror session info struct to be passed in to
  // drivers API

  // Set the direction for the mirror session
  status = this->dataFieldIdGet("$direction", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for direction field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  if (strField.find(field_id) != strField.end()) {
    auto it = strField.find(field_id);
    if (this->mirror_str_to_dir_map.find(it->second) ==
        this->mirror_str_to_dir_map.end()) {
      LOG_ERROR("%s:%d %s : Error : %s is not a valid direction",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                it->second.c_str());
      return BF_INVALID_ARG;
    }
    s_info.dir = this->mirror_str_to_dir_map.at(it->second);
  }

  // Set the egress port
  status = this->dataFieldIdGet("$ucast_egress_port", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for egress port field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  if (u64Field.find(field_id) != u64Field.end()) {
    auto it = u64Field.find(field_id);
    s_info.ucast_egress_port = static_cast<uint32_t>(it->second);
  }

  // Set the egress port valid
  status =
      this->dataFieldIdGet("$ucast_egress_port_valid", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for egress port valid "
        "field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  if (boolField.find(field_id) != boolField.end()) {
    auto it = boolField.find(field_id);
    s_info.ucast_egress_port_v = it->second;
  }

  // Set the egress port queue
  status = this->dataFieldIdGet("$egress_port_queue", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for egress port queue "
        "field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  if (u64Field.find(field_id) != u64Field.end()) {
    auto it = u64Field.find(field_id);
    s_info.egress_port_queue = static_cast<bf_tm_queue_t>(it->second);
  }

  // Set the ingress cos
  status = this->dataFieldIdGet("$ingress_cos", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for ingress_cos field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  if (u64Field.find(field_id) != u64Field.end()) {
    auto it = u64Field.find(field_id);
    s_info.ingress_cos = static_cast<uint32_t>(it->second);
  }

  // Set the packet color
  status = this->dataFieldIdGet("$packet_color", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for packet color field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  if (strField.find(field_id) != strField.end()) {
    auto it = strField.find(field_id);
    if (this->mirror_str_to_packet_color_map.find(it->second) ==
        this->mirror_str_to_packet_color_map.end()) {
      LOG_ERROR("%s:%d %s : Error : %s is not a valid color",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                it->second.c_str());
      return BF_INVALID_ARG;
    }
    s_info.packet_color = this->mirror_str_to_packet_color_map.at(it->second);
  }

  // Set the L1 hash for multicast
  status = this->dataFieldIdGet("$level1_mcast_hash", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for L1 mcast hash field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  if (u64Field.find(field_id) != u64Field.end()) {
    auto it = u64Field.find(field_id);
    s_info.level1_mcast_hash = static_cast<uint32_t>(it->second);
  }

  // Set the L2 hash for multicast
  status = this->dataFieldIdGet("$level2_mcast_hash", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for L2 mcast hash field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  if (u64Field.find(field_id) != u64Field.end()) {
    auto it = u64Field.find(field_id);
    s_info.level2_mcast_hash = static_cast<uint32_t>(it->second);
  }

  // Set the multicast mgid a
  status = this->dataFieldIdGet("$mcast_grp_a", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for mcast mgid a field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  if (u64Field.find(field_id) != u64Field.end()) {
    auto it = u64Field.find(field_id);
    s_info.mcast_grp_a = static_cast<uint16_t>(it->second);
  }

  // Set the multicast mgid a valid
  status = this->dataFieldIdGet("$mcast_grp_a_valid", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for mcast mgid a valid "
        "field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  if (boolField.find(field_id) != boolField.end()) {
    auto it = boolField.find(field_id);
    s_info.mcast_grp_a_v = it->second;
  }

  // Set the multicast mgid b
  status = this->dataFieldIdGet("$mcast_grp_b", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for mcast mgid b field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  if (u64Field.find(field_id) != u64Field.end()) {
    auto it = u64Field.find(field_id);
    s_info.mcast_grp_b = static_cast<uint16_t>(it->second);
  }

  // Set the multicast mgid b valid
  status = this->dataFieldIdGet("$mcast_grp_b_valid", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for mcast mgid b valid "
        "field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  if (boolField.find(field_id) != boolField.end()) {
    auto it = boolField.find(field_id);
    s_info.mcast_grp_b_v = it->second;
  }

  // Set the multicast L1 xid
  status = this->dataFieldIdGet("$mcast_l1_xid", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for mcast l1 xid field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  if (u64Field.find(field_id) != u64Field.end()) {
    auto it = u64Field.find(field_id);
    s_info.mcast_l1_xid = static_cast<uint16_t>(it->second);
  }

  // Set the multicast L2 xid
  status = this->dataFieldIdGet("$mcast_l2_xid", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for mcast l2 xid field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  if (u64Field.find(field_id) != u64Field.end()) {
    auto it = u64Field.find(field_id);
    s_info.mcast_l2_xid = static_cast<uint16_t>(it->second);
  }

  // Set the multicast RID
  status = this->dataFieldIdGet("$mcast_rid", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for mcast rid field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  if (u64Field.find(field_id) != u64Field.end()) {
    auto it = u64Field.find(field_id);
    s_info.mcast_rid = static_cast<uint16_t>(it->second);
  }

  // Set the iCoS for copy-to-cpu packets
  status = this->dataFieldIdGet("$icos_for_copy_to_cpu", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for iCoS for "
        "copy-to-cpu field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  if (u64Field.find(field_id) != u64Field.end()) {
    auto it = u64Field.find(field_id);
    s_info.icos_for_copy_to_cpu = static_cast<uint32_t>(it->second);
  }

  // Set the copy-to-cpu field
  status = this->dataFieldIdGet("$copy_to_cpu", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for copy-to-cpu field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  if (boolField.find(field_id) != boolField.end()) {
    auto it = boolField.find(field_id);
    s_info.copy_to_cpu = it->second;
  }

  // Set the max packet length
  status = this->dataFieldIdGet("$max_pkt_len", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for max packet length "
        "field",
        __func__,
        __LINE__,
        table_name_get().c_str());

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
    status = this->dataFieldIdGet("$internal_header", action_id, &field_id);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s : Error : Failed to get the field id for internal header "
          "field",
          __func__,
          __LINE__,
          table_name_get().c_str());

      return status;
    }

    if (arrayField.find(field_id) != arrayField.end()) {
      auto it = arrayField.find(field_id);
      std::memcpy(&s_info.header[0], &(it->second), BF_RT_MIRROR_INT_HDR_SIZE);
    }

    // Set the internal header length
    status =
        this->dataFieldIdGet("$internal_header_length", action_id, &field_id);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s : Error : Failed to get the field id for internal header "
          "length field",
          __func__,
          __LINE__,
          table_name_get().c_str());

      return status;
    }
    if (u64Field.find(field_id) != u64Field.end()) {
      auto it = u64Field.find(field_id);
      s_info.header_len = static_cast<uint32_t>(it->second);
    }

    // Set the timeout value
    status = this->dataFieldIdGet("$timeout_usec", action_id, &field_id);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s : Error : Failed to get the field id for timeout field",
          __func__,
          __LINE__,
          table_name_get().c_str());

      return status;
    }
    if (u64Field.find(field_id) != u64Field.end()) {
      auto it = u64Field.find(field_id);
      s_info.timeout_usec = static_cast<uint32_t>(it->second);
    }

    // Set the extract length value
    status = this->dataFieldIdGet("$extract_len", action_id, &field_id);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s : Error : Failed to get the field id for extract length "
          "field",
          __func__,
          __LINE__,
          table_name_get().c_str());

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
  status = this->dataFieldIdGet("$session_enable", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for session enable "
        "field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  if (boolField.find(field_id) != boolField.end()) {
    auto it = boolField.find(field_id);
    session_enable = it->second;
  }

  // Update the session info now
  status = pipeMgr->pipeMgrMirrorSessionSet(session.sessHandleGet(),
                                            pipe_dev_tgt,
                                            session_id,
                                            &s_info,
                                            session_enable);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s: Error in updating mirror session for id %d, status = %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        session_id,
        status);
    return status;
  }

  // Set the session priority if it is present in passed in data
  status = this->dataFieldIdGet("$session_priority", action_id, &field_id);
  if (status == BF_SUCCESS) {
    if (boolField.find(field_id) != boolField.end()) {
      bool session_priority = boolField.at(field_id);
      status = pipeMgr->pipeMgrMirrorSessionPriorityUpdate(
          session.sessHandleGet(), pipe_dev_tgt, session_id, session_priority);
      if (status != BF_SUCCESS) {
        LOG_TRACE(
            "%s:%d %s: Error in updating mirror session priority for id %d, "
            "status = %d",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            session_id,
            status);
        return status;
      }
    }
  }

  // Set the hash cfg flag if it is present in passed in data
  status = this->dataFieldIdGet("$hash_cfg_flag", action_id, &field_id);
  if (status == BF_SUCCESS) {
    if (boolField.find(field_id) != boolField.end()) {
      bool hash_cfg_flag = boolField.at(field_id);
      status =
          pipeMgr->pipeMgrMirrorSessionMetaFlagUpdate(session.sessHandleGet(),
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
            table_name_get().c_str(),
            session_id,
            status);
        return status;
      }
    }
  }

  // Set the hash cfg_p flag if it is present in passed in data
  status = this->dataFieldIdGet("$hash_cfg_flag_p", action_id, &field_id);
  if (status == BF_SUCCESS) {
    if (boolField.find(field_id) != boolField.end()) {
      bool hash_cfg_flag_p = boolField.at(field_id);
      status =
          pipeMgr->pipeMgrMirrorSessionMetaFlagUpdate(session.sessHandleGet(),
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
            table_name_get().c_str(),
            session_id,
            status);
        return status;
      }
    }
  }

  // Set the iCoS cfg flag if it is present in passed in data
  status = this->dataFieldIdGet("$icos_cfg_flag", action_id, &field_id);
  if (status == BF_SUCCESS) {
    if (boolField.find(field_id) != boolField.end()) {
      bool icos_cfg_flag = boolField.at(field_id);
      status =
          pipeMgr->pipeMgrMirrorSessionMetaFlagUpdate(session.sessHandleGet(),
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
            table_name_get().c_str(),
            session_id,
            status);
        return status;
      }
    }
  }

  // Set the DoD cfg flag if it is present in passed in data
  status = this->dataFieldIdGet("$dod_cfg_flag", action_id, &field_id);
  if (status == BF_SUCCESS) {
    if (boolField.find(field_id) != boolField.end()) {
      bool dod_cfg_flag = boolField.at(field_id);
      status =
          pipeMgr->pipeMgrMirrorSessionMetaFlagUpdate(session.sessHandleGet(),
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
            table_name_get().c_str(),
            session_id,
            status);
        return status;
      }
    }
  }

  // Set the copy-to-cpu cfg flag if it is present in passed in data
  status = this->dataFieldIdGet("$c2c_cfg_flag", action_id, &field_id);
  if (status == BF_SUCCESS) {
    if (boolField.find(field_id) != boolField.end()) {
      bool c2c_cfg_flag = boolField.at(field_id);
      status =
          pipeMgr->pipeMgrMirrorSessionMetaFlagUpdate(session.sessHandleGet(),
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
            table_name_get().c_str(),
            session_id,
            status);
        return status;
      }
    }
  }

  // Set the multicast cfg flag if it is present in passed in data
  status = this->dataFieldIdGet("$mc_cfg_flag", action_id, &field_id);
  if (status == BF_SUCCESS) {
    if (boolField.find(field_id) != boolField.end()) {
      bool mc_cfg_flag = boolField.at(field_id);
      status =
          pipeMgr->pipeMgrMirrorSessionMetaFlagUpdate(session.sessHandleGet(),
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
            table_name_get().c_str(),
            session_id,
            status);
        return status;
      }
    }
  }

  // Set the epipe cfg flag if it is present in passed in data
  status = this->dataFieldIdGet("$epipe_cfg_flag", action_id, &field_id);
  if (status == BF_SUCCESS) {
    if (boolField.find(field_id) != boolField.end()) {
      bool epipe_cfg_flag = boolField.at(field_id);
      status =
          pipeMgr->pipeMgrMirrorSessionMetaFlagUpdate(session.sessHandleGet(),
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
            table_name_get().c_str(),
            session_id,
            status);
        return status;
      }
    }
  }

  // If the session type is COALESCING, set the coalesing mode
  // if it is present in passed in data
  if (s_info.mirror_type == BF_MIRROR_TYPE_COAL) {
    status = this->dataFieldIdGet("$session_coal_mode", action_id, &field_id);
    if (status == BF_SUCCESS) {
      if (boolField.find(field_id) != boolField.end()) {
        bool session_coal_mode = boolField.at(field_id);
        status =
            pipeMgr->pipeMgrMirrorSessionCoalModeUpdate(session.sessHandleGet(),
                                                        pipe_dev_tgt,
                                                        session_id,
                                                        session_coal_mode);
        if (status != BF_SUCCESS) {
          LOG_TRACE(
              "%s:%d %s: Error in updating mirror session coalescing mode for "
              "id %d, status = %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              session_id,
              status);
          return status;
        }
      }
    }
  }

  return BF_SUCCESS;
}

bf_status_t BfRtMirrorCfgTable::tableEntryDel(const BfRtSession &session,
                                              const bf_rt_target_t &dev_tgt,
                                              const uint64_t & /*flags*/,
                                              const BfRtTableKey &key) const {
  bf_status_t status = BF_SUCCESS;

  const BfRtMirrorCfgTableKey &mirror_key =
      static_cast<const BfRtMirrorCfgTableKey &>(key);

  // First get the mirror session id from passed in key
  const int &session_id = static_cast<const int &>(mirror_key.getId());

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  // Get the mirror session_info struct & session_enable field from pipe manager
  bf_mirror_session_info_t s_info;
  // Initialize the mirror_session_info struct to all 0
  std::memset(&s_info, 0, sizeof(s_info));
  status = pipeMgr->pipeMgrMirrorSessionGet(
      session.sessHandleGet(), pipe_dev_tgt, session_id, &s_info);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s: Error: mirror session with id %d does not exist, status = "
        "%d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        session_id,
        status);
    return BF_INVALID_ARG;
  }
  // For mirror config table, deleting the table entry is
  // resetting the mirror session config in HW. So, this
  // operation can be done even if the session entry is not
  // created and hence check for whether table entry exists
  // or not is not needed here.
  status = pipeMgr->pipeMgrMirrorSessionReset(
      session.sessHandleGet(), pipe_dev_tgt, session_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s: Error in deleting/resetting mirror session for id %d, "
        "status = %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        session_id,
        status);
    return status;
  }

  return status;
}

bf_status_t BfRtMirrorCfgTable::tableClear(const BfRtSession &session,
                                           const bf_rt_target_t &dev_tgt,
                                           const uint64_t &flags) const {
  // steps:
  // get the count
  // get_first
  // get_next_n
  // entry_del each entry
  bf_status_t sts = BF_SUCCESS;
  uint32_t count = 0;
  this->tableUsageGet(session, dev_tgt, flags, &count);
  if (count == 0) {
    return BF_SUCCESS;
  } else {
    std::unique_ptr<bfrt::BfRtTableKey> first_key;
    std::unique_ptr<bfrt::BfRtTableData> first_data;
    sts = this->keyAllocate(&first_key);
    if (sts != BF_SUCCESS) {
      goto error;
    }
    sts = this->dataAllocate(&first_data);
    if (sts != BF_SUCCESS) {
      goto error;
    }
    sts = this->tableEntryGetFirst(
        session, dev_tgt, flags, first_key.get(), first_data.get());
    if (sts != BF_SUCCESS) {
      goto error;
    }

    BfRtTable::keyDataPairs key_data_pairs;
    std::vector<std::unique_ptr<BfRtTableKey>> keys(count - 1);
    std::vector<std::unique_ptr<BfRtTableData>> data(count - 1);

    // Allocate KEY and DATA for all counts
    for (uint32_t i = 0; i < count - 1; ++i) {
      sts = this->keyAllocate(&keys[i]);
      if (sts != BF_SUCCESS) {
        goto error;
      }
      sts = this->dataAllocate(&data[i]);
      if (sts != BF_SUCCESS) {
        goto error;
      }
      key_data_pairs.push_back(std::make_pair(keys[i].get(), data[i].get()));
    }

    // Get next N
    uint32_t num_returned = 0;
    sts = this->tableEntryGetNext_n(session,
                                    dev_tgt,
                                    flags,
                                    *first_key.get(),
                                    count - 1,
                                    &key_data_pairs,
                                    &num_returned);
    if (sts != BF_SUCCESS) {
      goto error;
    }

    for (uint32_t i = 0; i < num_returned; i++) {
      this->tableEntryDel(session, dev_tgt, flags, *keys[i].get());
    }
    this->tableEntryDel(session, dev_tgt, flags, *first_key.get());
  }
  return BF_SUCCESS;
error:
  LOG_ERROR("%s:%d %s: Cannot Clear the MirrorCfgTable with status %d",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            sts);
  return sts;
}

bf_status_t BfRtMirrorCfgTable::tableUsageGet(const BfRtSession &session,
                                              const bf_rt_target_t &dev_tgt,
                                              const uint64_t & /*flags*/,
                                              uint32_t *count) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  status = pipeMgr->pipeMgrMirrorSessionCountGet(
      session.sessHandleGet(), pipe_dev_tgt, count);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "8010d0d2"
        " %s %s: Error in getting mirror table Usage ",
        __func__,
        table_name_get().c_str());
    return status;
  }
  return status;
}

bf_status_t BfRtMirrorCfgTable::tableEntryGetFirst(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    BfRtTableKey *key,
    BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;
  BfRtMirrorCfgTableKey *mirror_key = static_cast<BfRtMirrorCfgTableKey *>(key);

  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  bf_mirror_session_info_t s_info;
  bf_mirror_get_id_t first_id = {0};
  memset(&s_info, 0, sizeof(s_info));
  // Get first mirror session id from pipe manager
  status = pipeMgr->pipeMgrMirrorSessionGetFirst(
      session.sessHandleGet(), pipe_dev_tgt, &s_info, &first_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s: Error in getting first mirror session id, status = %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  bool session_enable = false;
  status = pipeMgr->pipeMgrMirrorSessionEnableGet(
      session.sessHandleGet(), pipe_dev_tgt, first_id.sid, &session_enable);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s: Error in getting mirror session_enable field for id %d, "
        "status = %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        first_id.sid,
        status);
    return status;
  }

  mirror_key->setId(first_id.sid);
  return this->tableEntryGetInternal(
      session, pipe_dev_tgt, s_info, first_id.sid, session_enable, data);
}

bf_status_t BfRtMirrorCfgTable::tableEntryGetNext_n(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    const uint32_t &n,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  bf_status_t status = BF_SUCCESS;
  if (!num_returned) {
    LOG_TRACE("%s:%d %s: Invalid Argument : num_returned cannot be NULL",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  const BfRtMirrorCfgTableKey &mirror_key =
      static_cast<const BfRtMirrorCfgTableKey &>(key);
  const auto session_id = mirror_key.getId();

  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  uint32_t i = 0;
  bf_mirror_get_id_t current_id = {session_id, dev_tgt.pipe_id};
  for (i = 0; i < n; i++) {
    bf_mirror_session_info_t next_info;
    bf_mirror_get_id_t next_id;
    status = pipeMgr->pipeMgrMirrorSessionGetNext(session.sessHandleGet(),
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
            table_name_get().c_str(),
            i,
            session_id,
            status);
      }
      break;
    }
    bool session_enable = false;
    status = pipeMgr->pipeMgrMirrorSessionEnableGet(
        session.sessHandleGet(), pipe_dev_tgt, session_id, &session_enable);
    if (status != BF_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s: Error in getting mirror session_enable field for id %d, "
          "status = %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          session_id,
          status);
      break;
    }

    auto this_key =
        static_cast<BfRtMirrorCfgTableKey *>((*key_data_pairs)[i].first);
    auto this_data = (*key_data_pairs)[i].second;
    this_key->setId(next_id.sid);
    status = this->tableEntryGetInternal(session,
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
          table_name_get().c_str(),
          i,
          session_id);
      break;
    }
    current_id = next_id;
    *num_returned += 1;
  }
  return status;
}

bf_status_t BfRtMirrorCfgTable::tableEntryGet(const BfRtSession &session,
                                              const bf_rt_target_t &dev_tgt,
                                              const uint64_t & /*flags*/,
                                              const BfRtTableKey &key,
                                              BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;

  const BfRtMirrorCfgTableKey &mirror_key =
      static_cast<const BfRtMirrorCfgTableKey &>(key);

  // First get the mirror session id from passed in key
  const int &session_id = static_cast<const int &>(mirror_key.getId());

  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  // Get the mirror session_info struct & session_enable field from pipe manager
  bf_mirror_session_info_t s_info;
  // Initialize the mirror_session_info struct to all 0
  std::memset(&s_info, 0, sizeof(s_info));
  status = pipeMgr->pipeMgrMirrorSessionGet(
      session.sessHandleGet(), pipe_dev_tgt, session_id, &s_info);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s: Error in getting mirror session info for id %d, status = %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        session_id,
        status);
    return status;
  }
  bool session_enable = false;
  status = pipeMgr->pipeMgrMirrorSessionEnableGet(
      session.sessHandleGet(), pipe_dev_tgt, session_id, &session_enable);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s: Error in getting mirror session_enable field for id %d, "
        "status = %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        session_id,
        status);
    return status;
  }

  return this->tableEntryGetInternal(
      session, pipe_dev_tgt, s_info, session_id, session_enable, data);
}

bf_status_t BfRtMirrorCfgTable::tableEntryGetInternal(
    const BfRtSession &session,
    const dev_target_t pipe_dev_tgt,
    const bf_mirror_session_info_t &s_info,
    const bf_mirror_id_t &session_id,
    const bool &session_enable,
    BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;

  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  BfRtMirrorCfgTableData *mirror_data =
      static_cast<BfRtMirrorCfgTableData *>(data);
  const std::set<bf_rt_id_t> &activeDataFields = mirror_data->getActiveFields();

  bf_rt_id_t action_id;
  mirror_data->actionIdGet(&action_id);

  bf_rt_id_t pipe_action_id;
  // Identify the mirror session type and get the corresponding action id
  if (s_info.mirror_type == BF_MIRROR_TYPE_COAL) {
    status = this->actionIdGet("$coalescing", &pipe_action_id);
  } else {
    status = this->actionIdGet("$normal", &pipe_action_id);
  }
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s: Error in getting action id for mirror session type %d for "
        "id %d, status = %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
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
          table_name_get().c_str(),
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
  bf_rt_id_t field_id;

  // Set the session_enable for the mirror session in data
  status = this->dataFieldIdGet("$session_enable", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for session_enable "
        "field",
        __func__,
        __LINE__,
        table_name_get().c_str());
    return status;
  }
  if (activeDataFields.find(field_id) != activeDataFields.end()) {
    // Get the mirror sessionEnable Field info from pipe manager
    mirror_data->setValue(field_id, session_enable);
  }

  // Set the direction for the mirror session in data
  status = this->dataFieldIdGet("$direction", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for direction field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  if (activeDataFields.find(field_id) != activeDataFields.end()) {
    std::string direction = this->mirror_dir_to_str_map.at(s_info.dir);
    mirror_data->setValue(field_id, direction);
  }

  // Set the egress port
  status = this->dataFieldIdGet("$ucast_egress_port", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for egress port field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  if (activeDataFields.find(field_id) != activeDataFields.end()) {
    mirror_data->setValue(field_id,
                          static_cast<uint64_t>(s_info.ucast_egress_port));
  }

  // Set the egress port valid
  status =
      this->dataFieldIdGet("$ucast_egress_port_valid", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for egress port valid "
        "field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  if (activeDataFields.find(field_id) != activeDataFields.end()) {
    mirror_data->setValue(field_id,
                          static_cast<bool>(s_info.ucast_egress_port_v));
  }

  // Set the egress port queue
  status = this->dataFieldIdGet("$egress_port_queue", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for egress port queue "
        "field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  if (activeDataFields.find(field_id) != activeDataFields.end()) {
    mirror_data->setValue(field_id,
                          static_cast<uint64_t>(s_info.egress_port_queue));
  }

  // Set the ingress cos
  status = this->dataFieldIdGet("$ingress_cos", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for ingress_cos field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  if (activeDataFields.find(field_id) != activeDataFields.end()) {
    mirror_data->setValue(field_id, static_cast<uint64_t>(s_info.ingress_cos));
  }

  // Set the packet color
  status = this->dataFieldIdGet("$packet_color", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for packet color field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  if (activeDataFields.find(field_id) != activeDataFields.end()) {
    std::string packet_color =
        this->mirror_packet_color_to_str_map.at(s_info.packet_color);
    mirror_data->setValue(field_id, packet_color);
  }

  // Set the L1 hash for multicast
  status = this->dataFieldIdGet("$level1_mcast_hash", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for L1 mcast hash field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  if (activeDataFields.find(field_id) != activeDataFields.end()) {
    mirror_data->setValue(field_id,
                          static_cast<uint64_t>(s_info.level1_mcast_hash));
  }

  // Set the L2 hash for multicast
  status = this->dataFieldIdGet("$level2_mcast_hash", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for L2 mcast hash field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  if (activeDataFields.find(field_id) != activeDataFields.end()) {
    mirror_data->setValue(field_id,
                          static_cast<uint64_t>(s_info.level2_mcast_hash));
  }

  // Set the multicast mgid a
  status = this->dataFieldIdGet("$mcast_grp_a", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for mcast mgid a field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  if (activeDataFields.find(field_id) != activeDataFields.end()) {
    mirror_data->setValue(field_id, static_cast<uint64_t>(s_info.mcast_grp_a));
  }

  // Set the multicast mgid a valid
  status = this->dataFieldIdGet("$mcast_grp_a_valid", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for mcast mgid a valid "
        "field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  if (activeDataFields.find(field_id) != activeDataFields.end()) {
    mirror_data->setValue(field_id, static_cast<bool>(s_info.mcast_grp_a_v));
  }

  // Set the multicast mgid b
  status = this->dataFieldIdGet("$mcast_grp_b", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for mcast mgid b field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  if (activeDataFields.find(field_id) != activeDataFields.end()) {
    mirror_data->setValue(field_id, static_cast<uint64_t>(s_info.mcast_grp_b));
  }

  // Set the multicast mgid b valid
  status = this->dataFieldIdGet("$mcast_grp_b_valid", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for mcast mgid b valid "
        "field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  if (activeDataFields.find(field_id) != activeDataFields.end()) {
    mirror_data->setValue(field_id, static_cast<bool>(s_info.mcast_grp_b_v));
  }

  // Set the multicast L1 xid
  status = this->dataFieldIdGet("$mcast_l1_xid", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for mcast l1 xid field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  if (activeDataFields.find(field_id) != activeDataFields.end()) {
    mirror_data->setValue(field_id, static_cast<uint64_t>(s_info.mcast_l1_xid));
  }

  // Set the multicast L2 xid
  status = this->dataFieldIdGet("$mcast_l2_xid", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for mcast l2 xid field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  if (activeDataFields.find(field_id) != activeDataFields.end()) {
    mirror_data->setValue(field_id, static_cast<uint64_t>(s_info.mcast_l2_xid));
  }

  // Set the multicast RID
  status = this->dataFieldIdGet("$mcast_rid", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for mcast rid field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  if (activeDataFields.find(field_id) != activeDataFields.end()) {
    mirror_data->setValue(field_id, static_cast<uint64_t>(s_info.mcast_rid));
  }

  // Set the iCoS for copy-to-cpu packets
  status = this->dataFieldIdGet("$icos_for_copy_to_cpu", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for iCoS for "
        "copy-to-cpu field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  if (activeDataFields.find(field_id) != activeDataFields.end()) {
    mirror_data->setValue(field_id,
                          static_cast<uint64_t>(s_info.icos_for_copy_to_cpu));
  }

  // Set the copy-to-cpu field
  status = this->dataFieldIdGet("$copy_to_cpu", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for copy-to-cpu field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  if (activeDataFields.find(field_id) != activeDataFields.end()) {
    mirror_data->setValue(field_id, static_cast<bool>(s_info.copy_to_cpu));
  }

  // Set the max packet length
  status = this->dataFieldIdGet("$max_pkt_len", action_id, &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for max packet length "
        "field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  if (activeDataFields.find(field_id) != activeDataFields.end()) {
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
    status = this->dataFieldIdGet("$internal_header", action_id, &field_id);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s : Error : Failed to get the field id for internal header "
          "field",
          __func__,
          __LINE__,
          table_name_get().c_str());

      return status;
    }
    if (activeDataFields.find(field_id) != activeDataFields.end()) {
      mirror_data->setValue(
          field_id,
          reinterpret_cast<const uint8_t *>(&s_info.header[0]),
          BF_RT_MIRROR_INT_HDR_SIZE);
    }

    // Set the internal header length
    status =
        this->dataFieldIdGet("$internal_header_length", action_id, &field_id);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s : Error : Failed to get the field id for internal header "
          "length field",
          __func__,
          __LINE__,
          table_name_get().c_str());

      return status;
    }
    if (activeDataFields.find(field_id) != activeDataFields.end()) {
      mirror_data->setValue(field_id, static_cast<uint64_t>(s_info.header_len));
    }

    // Set the timeout value
    status = this->dataFieldIdGet("$timeout_usec", action_id, &field_id);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s : Error : Failed to get the field id for timeout field",
          __func__,
          __LINE__,
          table_name_get().c_str());

      return status;
    }
    if (activeDataFields.find(field_id) != activeDataFields.end()) {
      mirror_data->setValue(field_id,
                            static_cast<uint64_t>(s_info.timeout_usec));
    }

    // Set the extract length value
    status = this->dataFieldIdGet("$extract_len", action_id, &field_id);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s : Error : Failed to get the field id for extract length "
          "field",
          __func__,
          __LINE__,
          table_name_get().c_str());

      return status;
    }
    if (activeDataFields.find(field_id) != activeDataFields.end()) {
      mirror_data->setValue(field_id,
                            static_cast<uint64_t>(s_info.extract_len));
    }
  }

  // Set the session priority in data for output tofino2 only
  status = this->dataFieldIdGet("$session_priority", action_id, &field_id);
  if (status == BF_SUCCESS) {
    bool session_priority;
    status = pipeMgr->pipeMgrMirrorSessionPriorityGet(
        session.sessHandleGet(), pipe_dev_tgt, session_id, &session_priority);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s: Error: Failed to get mirror session priority for id %d, "
          "status = %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          session_id,
          status);
      return status;
    }
    if (activeDataFields.find(field_id) != activeDataFields.end()) {
      mirror_data->setValue(field_id, static_cast<bool>(session_priority));
    }
  }

  // Set the hash cfg flag if it is present in passed in data
  status = this->dataFieldIdGet("$hash_cfg_flag", action_id, &field_id);
  if (status == BF_SUCCESS) {
    bool hash_cfg_flag;
    status = pipeMgr->pipeMgrMirrorSessionMetaFlagGet(session.sessHandleGet(),
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
          table_name_get().c_str(),
          session_id,
          status);
      return status;
    }
    if (activeDataFields.find(field_id) != activeDataFields.end()) {
      mirror_data->setValue(field_id, static_cast<bool>(hash_cfg_flag));
    }
  }

  // Set the hash cfg flag p if it is present in passed in data
  status = this->dataFieldIdGet("$hash_cfg_flag_p", action_id, &field_id);
  if (status == BF_SUCCESS) {
    bool hash_cfg_flag_p;
    status = pipeMgr->pipeMgrMirrorSessionMetaFlagGet(session.sessHandleGet(),
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
          table_name_get().c_str(),
          session_id,
          status);
      return status;
    }
    if (activeDataFields.find(field_id) != activeDataFields.end()) {
      mirror_data->setValue(field_id, static_cast<bool>(hash_cfg_flag_p));
    }
  }

  status = this->dataFieldIdGet("$icos_cfg_flag", action_id, &field_id);
  if (status == BF_SUCCESS) {
    bool icos_cfg_flag;
    status = pipeMgr->pipeMgrMirrorSessionMetaFlagGet(session.sessHandleGet(),
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
          table_name_get().c_str(),
          session_id,
          status);
      return status;
    }
    if (activeDataFields.find(field_id) != activeDataFields.end()) {
      mirror_data->setValue(field_id, static_cast<bool>(icos_cfg_flag));
    }
  }

  status = this->dataFieldIdGet("$dod_cfg_flag", action_id, &field_id);
  if (status == BF_SUCCESS) {
    bool dod_cfg_flag;
    status = pipeMgr->pipeMgrMirrorSessionMetaFlagGet(session.sessHandleGet(),
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
          table_name_get().c_str(),
          session_id,
          status);
      return status;
    }
    if (activeDataFields.find(field_id) != activeDataFields.end()) {
      mirror_data->setValue(field_id, static_cast<bool>(dod_cfg_flag));
    }
  }

  status = this->dataFieldIdGet("$c2c_cfg_flag", action_id, &field_id);
  if (status == BF_SUCCESS) {
    bool c2c_cfg_flag;
    status = pipeMgr->pipeMgrMirrorSessionMetaFlagGet(session.sessHandleGet(),
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
          table_name_get().c_str(),
          session_id,
          status);
      return status;
    }
    if (activeDataFields.find(field_id) != activeDataFields.end()) {
      mirror_data->setValue(field_id, static_cast<bool>(c2c_cfg_flag));
    }
  }

  status = this->dataFieldIdGet("$mc_cfg_flag", action_id, &field_id);
  if (status == BF_SUCCESS) {
    bool mc_cfg_flag;
    status = pipeMgr->pipeMgrMirrorSessionMetaFlagGet(session.sessHandleGet(),
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
          table_name_get().c_str(),
          session_id,
          status);
      return status;
    }
    if (activeDataFields.find(field_id) != activeDataFields.end()) {
      mirror_data->setValue(field_id, static_cast<bool>(mc_cfg_flag));
    }
  }

  status = this->dataFieldIdGet("$epipe_cfg_flag", action_id, &field_id);
  if (status == BF_SUCCESS) {
    bool epipe_cfg_flag;
    status = pipeMgr->pipeMgrMirrorSessionMetaFlagGet(session.sessHandleGet(),
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
          table_name_get().c_str(),
          session_id,
          status);
      return status;
    }
    if (activeDataFields.find(field_id) != activeDataFields.end()) {
      mirror_data->setValue(field_id, static_cast<bool>(epipe_cfg_flag));
    }
  }

  // If the session type is COALESCING, get the coalesing mode in tf2
  // if it is present in passed in data
  if (s_info.mirror_type == BF_MIRROR_TYPE_COAL) {
    status = this->dataFieldIdGet("$session_coal_mode", action_id, &field_id);
    if (status == BF_SUCCESS) {
      bool session_coal_mode;
      status = pipeMgr->pipeMgrMirrorSessionCoalModeGet(session.sessHandleGet(),
                                                        pipe_dev_tgt,
                                                        session_id,
                                                        &session_coal_mode);
      if (status != BF_SUCCESS) {
        LOG_TRACE(
            "%s:%d %s: Error in updating mirror session coalescing mode for "
            "id %d, status = %d",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            session_id,
            status);
        return status;
      }
      if (activeDataFields.find(field_id) != activeDataFields.end()) {
        mirror_data->setValue(field_id, static_cast<bool>(session_coal_mode));
      }
    }
  }

  return BF_SUCCESS;
}

bf_status_t BfRtMirrorCfgTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtMirrorCfgTableKey(this));

  if (*key_ret == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to allocate key",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtMirrorCfgTable::keyReset(BfRtTableKey *key) const {
  if (key == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to reset key",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_OBJECT_NOT_FOUND;
  }
  return (static_cast<BfRtMirrorCfgTableKey *>(key))->reset();
}

bf_status_t BfRtMirrorCfgTable::dataReset(BfRtTableData *data) const {
  std::vector<bf_rt_id_t> emptyFields;
  return this->dataReset(emptyFields, 0, data);
}

bf_status_t BfRtMirrorCfgTable::dataReset(const bf_rt_id_t &action_id,
                                          BfRtTableData *data) const {
  std::vector<bf_rt_id_t> emptyFields;
  return this->dataReset(emptyFields, action_id, data);
}

bf_status_t BfRtMirrorCfgTable::dataReset(const std::vector<bf_rt_id_t> &fields,
                                          const bf_rt_id_t &action_id,
                                          BfRtTableData *data) const {
  if (data == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to reset data",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_OBJECT_NOT_FOUND;
  }
  return (static_cast<BfRtMirrorCfgTableData *>(data))
      ->reset(action_id, fields);
}

bf_status_t BfRtMirrorCfgTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  std::vector<bf_rt_id_t> fields;

  *data_ret = std::unique_ptr<BfRtTableData>(
      new BfRtMirrorCfgTableData(this, 0, fields));
  if (*data_ret == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to allocate data",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtMirrorCfgTable::dataAllocate(
    const bf_rt_id_t &action_id,
    std::unique_ptr<BfRtTableData> *data_ret) const {
  std::vector<bf_rt_id_t> fields;

  *data_ret = std::unique_ptr<BfRtTableData>(
      new BfRtMirrorCfgTableData(this, action_id, fields));
  if (*data_ret == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to allocate data",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtMirrorCfgTable::dataAllocate(
    const std::vector<bf_rt_id_t> &fields,
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret = std::unique_ptr<BfRtTableData>(
      new BfRtMirrorCfgTableData(this, 0, fields));

  if (*data_ret == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to allocate data",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtMirrorCfgTable::dataAllocate(
    const std::vector<bf_rt_id_t> &fields,
    const bf_rt_id_t &action_id,
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret = std::unique_ptr<BfRtTableData>(
      new BfRtMirrorCfgTableData(this, action_id, fields));

  if (*data_ret == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to allocate data",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }

  return BF_SUCCESS;
}

}  // namespace bfrt
