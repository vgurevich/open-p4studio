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


#include <algorithm>

#include <bf_rt/bf_rt_table.hpp>
#include <bf_rt_common/bf_rt_state.hpp>

#include <bf_rt_common/bf_rt_init_impl.hpp>

#include "bf_rt_mc_mgr_intf.hpp"
#include "bf_rt_pre_table_impl.hpp"
#include "bf_rt_pre_table_data_impl.hpp"
#include "bf_rt_pre_table_key_impl.hpp"

namespace bfrt {

namespace {

template <class Table, class Key>
bf_status_t key_reset(const Table &table, Key *key) {
  if (key == nullptr) {
    LOG_TRACE("%s:%d %s Error : key is invalid (NULL)",
              __func__,
              __LINE__,
              table.table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  if (!table.validateTable_from_keyObj(*key)) {
    LOG_TRACE("%s:%d %s ERROR : Key object is not associated with the table",
              __func__,
              __LINE__,
              table.table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  return key->reset();
}

template <class Table, class Data>
bf_status_t data_reset(const Table &table, Data *data) {
  if (data == nullptr) {
    LOG_TRACE("%s:%d %s Error : data is invalid (NULL)",
              __func__,
              __LINE__,
              table.table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  if (!table.validateTable_from_dataObj(*data)) {
    LOG_TRACE("%s:%d %s ERROR : Data object is not associated with the table",
              __func__,
              __LINE__,
              table.table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  return data->reset();
}

template <class Table, class Data>
bf_status_t data_reset(const Table &table,
                       const std::vector<bf_rt_id_t> &fields,
                       Data *data) {
  if (data == nullptr) {
    LOG_TRACE("%s:%d %s Error : data is invalid (NULL)",
              __func__,
              __LINE__,
              table.table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  if (!table.validateTable_from_dataObj(*data)) {
    LOG_TRACE("%s:%d %s ERROR : Data object is not associated with the table",
              __func__,
              __LINE__,
              table.table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  return data->reset(fields);
}

}  // anonymous namespace

// MGID Table APIs
bf_status_t BfRtPREMGIDTable::tableEntryAdd(const BfRtSession &session,
                                            const bf_rt_target_t &dev_tgt,
                                            const uint64_t & /*flags*/,
                                            const BfRtTableKey &key,
                                            const BfRtTableData &data) const {
  bf_status_t status = BF_SUCCESS;
  const BfRtPREMGIDTableKey &mgid_key =
      static_cast<const BfRtPREMGIDTableKey &>(key);
  bf_mc_mgrp_hdl_t mgrp_hdl;
  const uint64_t flags = 0;
  // Get the MGID from key
  bf_mc_grp_id_t mgid = mgid_key.getId();

  // Get the device state and from that get the PRE state to perform
  // various checks and update new states.
  std::lock_guard<std::mutex> lock(state_lock);
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, this->prog_name);
  if (device_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  auto pre_state = device_state->preState.getStateObj();
  if (pre_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  // Check if the MGID exists already or not
  if (pre_state->statePREIdExists(
          BfRtPREStateObj::PREStateIdType::MULTICAST_MGID, mgid)) {
    LOG_TRACE(
        "%s:%d %s : Error in adding PRE MGID %d which already "
        "exists",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        mgid);
    return BF_ALREADY_EXISTS;
  }
  // Call BfRtPREMGIDTable::tableEntryAdd_internal here
  status = this->tableEntryAdd_internal(
      session, dev_tgt, *pre_state, flags, key, data, mgrp_hdl, mgid);
  return status;
}

bf_status_t BfRtPREMGIDTable::tableEntryMod(const BfRtSession &session,
                                            const bf_rt_target_t &dev_tgt,
                                            const uint64_t &flags,
                                            const BfRtTableKey &key,
                                            const BfRtTableData &data) const {
  bf_status_t status = BF_SUCCESS;
  bf_status_t sub_status = BF_SUCCESS;

  // For MGID table entry modify -
  //    - delete the existing MGID entry
  //    - add a new MGID entry with the given data
  // TODO: This can be optimized to identify the exact difference
  //       between existing entry & new entry and implementing only what
  //       is actually needed to have minimal disruption. But this is not
  //       really a must have as we support EntryModInc

  const BfRtPREMGIDTableKey &mgid_key =
      static_cast<const BfRtPREMGIDTableKey &>(key);
  const BfRtPREMGIDTableData &mgid_data =
      static_cast<const BfRtPREMGIDTableData &>(data);

  // Get the MGID from key
  bf_mc_grp_id_t mgid = mgid_key.getId();

  // Get the device state and from that get the PRE state to perform
  // various checks and update new states.
  std::lock_guard<std::mutex> lock(state_lock);
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, this->prog_name);
  if (device_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  auto pre_state = device_state->preState.getStateObj();
  if (pre_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  // Check if the MGID exists already or not
  if (pre_state->statePREIdExists(
          BfRtPREStateObj::PREStateIdType::MULTICAST_MGID, mgid) != true) {
    LOG_TRACE("%s:%d %s : Error in modfying PRE MGID %d which doesn't exist",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              mgid);
    return BF_OBJECT_NOT_FOUND;
  }

  // Get the mgrp handle for the MGID
  bf_mc_mgrp_hdl_t mgrp_hdl;
  status = pre_state->statePREHdlGet(
      BfRtPREStateObj::PREStateIdType::MULTICAST_MGID, mgid, &mgrp_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error in getting PRE MGID handle for MGID %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              mgid);
    return status;
  }
  // Get and validate the data fields
  const std::vector<bf_rt_id_t> &new_multicast_node_ids =
      mgid_data.getMulticastNodeIds();
  const std::vector<bool> &new_node_l1_xids_valid =
      mgid_data.getMulticastNodeL1XIdsValid();
  const std::vector<bf_rt_id_t> &new_node_l1_xids =
      mgid_data.getMulticastNodeL1XIds();
  const std::vector<bf_rt_id_t> &new_multicast_ecmp_ids =
      mgid_data.getMulticastECMPIds();
  const std::vector<bool> &new_ecmp_l1_xids_valid =
      mgid_data.getMulticastECMPL1XIdsValid();
  const std::vector<bf_rt_id_t> &new_ecmp_l1_xids =
      mgid_data.getMulticastECMPL1XIds();

  status = this->mcMGIDValidateDataMbrsSize(new_multicast_node_ids.size(),
                                            new_node_l1_xids.size(),
                                            new_node_l1_xids_valid.size(),
                                            new_multicast_ecmp_ids.size(),
                                            new_ecmp_l1_xids.size(),
                                            new_ecmp_l1_xids_valid.size());
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error in validating Multicast Group parameters. "
        "Wrong size of data members",
        __func__,
        __LINE__,
        table_name_get().c_str());
    return status;
  }

  // Validate ECMP. If any of the ECMP groups is invalid - abort modification
  bf_rt_id_t mc_ecmp_id;
  uint32_t i;
  for (i = 0; i < new_multicast_ecmp_ids.size(); i++) {
    mc_ecmp_id = new_multicast_ecmp_ids[i];
    if (!pre_state->statePREIdExists(
            BfRtPREStateObj::PREStateIdType::MULTICAST_ECMP_ID, mc_ecmp_id)) {
      LOG_TRACE("%s:%d %s : ECMP group %d does not exists",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                mc_ecmp_id);
      return BF_INVALID_ARG;
    }
  }

  // Get current parameters
  std::vector<bf_rt_id_t> multicast_node_ids;
  std::vector<bool> node_l1_xids_valid;
  std::vector<bf_rt_id_t> node_l1_xids;
  std::vector<bf_rt_id_t> multicast_ecmp_ids;
  std::vector<bool> ecmp_l1_xids_valid;
  std::vector<bf_rt_id_t> ecmp_l1_xids;

  status = this->mcMGIDNodeMbrsGet(session,
                                   dev_tgt,
                                   *pre_state,
                                   mgid,
                                   mgrp_hdl,
                                   &multicast_node_ids,
                                   &node_l1_xids_valid,
                                   &node_l1_xids);
  if (status != BF_SUCCESS && status != BF_OBJECT_NOT_FOUND) {
    LOG_TRACE("%s:%d %s : Error in reading Multicast group %d member nodes",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              mgid);
    return status;
  }

  status = this->mcMGIDECMPMbrsGet(session,
                                   dev_tgt,
                                   *pre_state,
                                   mgid,
                                   mgrp_hdl,
                                   &multicast_ecmp_ids,
                                   &ecmp_l1_xids_valid,
                                   &ecmp_l1_xids);
  if (status != BF_SUCCESS && status != BF_OBJECT_NOT_FOUND) {
    // BF_OBJECT_NOT_FOUND is not an error case as there can be multicast
    // group with no ECMP nodes
    // For other error cases, error logging is already done, just return
    return status;
  }

  // Can't call tableEntryDel and tableEntryAdd here, as they taking the same
  // lock as we do. Internal versions must be used instead.
  status = this->tableEntryDel_internal(
      session, dev_tgt, *pre_state, flags, key, mgrp_hdl, mgid);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error in modifying Multicast group %d - the group"
        " could not be deleted",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        mgid);
    return status;
  }

  status = this->tableEntryAdd_internal(
      session, dev_tgt, *pre_state, flags, key, data, mgrp_hdl, mgid);

  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error in modifying Multicast group %d - the group"
        " could not be added",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        mgid);
    // Revert the changes made. Restore previous multicast group
    // the given MGID
    auto *mcMgr = McMgrIntf::getInstance(session);
    sub_status = mcMgr->mcMgrMgrpCreate(
        session.preSessHandleGet(), dev_tgt.dev_id, mgid, &mgrp_hdl);

    if (sub_status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error in restoring multicast group for mgid %d, err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          mgid,
          sub_status);
      // Always return upper level status (the result of Add_internal(..))
      return status;
    }

    // Associate all given multicast nodes to multicast group
    sub_status = this->mcMGIDNodeMbrsAdd(session,
                                         dev_tgt,
                                         *pre_state,
                                         mgid,
                                         mgrp_hdl,
                                         multicast_node_ids,
                                         node_l1_xids_valid,
                                         node_l1_xids);
    if (sub_status != BF_SUCCESS) {
      // If any error is encountered while associating nodes to the
      // multicast group, delete the multicast group and return
      // original error status. This is to keep both BF-RT and MC mgr
      // states in sync.
      LOG_ERROR(
          "%s:%d %s Error in adding Node members to mgid %d, "
          "err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          mgid,
          sub_status);
      bf_status_t temp_sts = mcMgr->mcMgrMgrpDestroy(
          session.preSessHandleGet(), dev_tgt.dev_id, mgrp_hdl);
      if (temp_sts != BF_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s Error in deleting Multicast Group for mgid %d, "
            "err %d",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            mgid,
            temp_sts);
      }
      // Return original error status
      return status;
    }
    // Now associate all given ECMPs  to the newly created multicast group
    sub_status = this->mcMGIDEcmpMbrsAdd(session,
                                         dev_tgt,
                                         *pre_state,
                                         mgid,
                                         mgrp_hdl,
                                         multicast_ecmp_ids,
                                         ecmp_l1_xids_valid,
                                         ecmp_l1_xids);
    if (sub_status != BF_SUCCESS) {
      // If any error is encountered while associating nodes to the
      // multicast group, delete the multicast group and return
      // original error status. This is to keep both BF-RT and MC mgr
      // states in sync.
      LOG_ERROR(
          "%s:%d %s Error in adding ECMP members to mgid %d, "
          "err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          mgid,
          sub_status);
      bf_status_t temp_sts = mcMgr->mcMgrMgrpDestroy(
          session.preSessHandleGet(), dev_tgt.dev_id, mgrp_hdl);
      if (temp_sts != BF_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s Error in deleting Multicast Group for mgid %d, "
            "err %d",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            mgid,
            temp_sts);
      }
      // Return original error status
      return status;
    }
    // Now that mgid is created successfully, update the mgid<--> handle
    // mapping in PRE state object
    sub_status = pre_state->statePREIdAdd(
        BfRtPREStateObj::PREStateIdType::MULTICAST_MGID, mgid, mgrp_hdl);
    if (sub_status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error in updating mgid<-->handle mapping for mgid %d, "
          "err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          mgid,
          sub_status);

      // Failed to update mgid<--> handle mapping in BF-RT (ideally this
      // should never be hit). Delete the
      // MC group to have both BF-RT and MC mgr states in sync.
      bf_status_t temp_sts = mcMgr->mcMgrMgrpDestroy(
          session.preSessHandleGet(), dev_tgt.dev_id, mgrp_hdl);
      if (temp_sts != BF_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s Error in deleting MC group for mgid %d, "
            "err %d",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            mgid,
            temp_sts);
      }
      // Return original error status
      return status;
    }
    return status;
  }
  return status;
}

bf_status_t BfRtPREMGIDTable::tableEntryModInc(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    const BfRtTableData &data) const {
  bf_status_t status = BF_SUCCESS;

  const BfRtPREMGIDTableKey &mgid_key =
      static_cast<const BfRtPREMGIDTableKey &>(key);
  const BfRtPREMGIDTableData &mgid_data =
      static_cast<const BfRtPREMGIDTableData &>(data);

  // Get the MGID from key
  bf_mc_grp_id_t mgid = mgid_key.getId();

  // Get the device state and from that get the PRE state to perform
  // various checks and update new states.
  std::lock_guard<std::mutex> lock(state_lock);
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, this->prog_name);
  if (device_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  auto pre_state = device_state->preState.getStateObj();
  if (pre_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  // Check if the MGID exists already or not
  if (pre_state->statePREIdExists(
          BfRtPREStateObj::PREStateIdType::MULTICAST_MGID, mgid) != true) {
    LOG_TRACE(
        "%s:%d %s : Error in modfying PRE MGID %d incrementally "
        "which doesn't exist",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        mgid);
    return BF_ALREADY_EXISTS;
  }

  // Get the mgrp handle for the MGID
  bf_mc_mgrp_hdl_t mgrp_hdl;
  status = pre_state->statePREHdlGet(
      BfRtPREStateObj::PREStateIdType::MULTICAST_MGID, mgid, &mgrp_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error in getting PRE MGID handle for MGID %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              mgid);
    return status;
  }

  // Get and validate the data fields
  const std::vector<bf_rt_id_t> &multicast_node_ids =
      mgid_data.getMulticastNodeIds();
  const std::vector<bool> &node_l1_xids_valid =
      mgid_data.getMulticastNodeL1XIdsValid();
  const std::vector<bf_rt_id_t> &node_l1_xids =
      mgid_data.getMulticastNodeL1XIds();
  const std::vector<bf_rt_id_t> &multicast_ecmp_ids =
      mgid_data.getMulticastECMPIds();
  const std::vector<bool> &ecmp_l1_xids_valid =
      mgid_data.getMulticastECMPL1XIdsValid();
  const std::vector<bf_rt_id_t> &ecmp_l1_xids =
      mgid_data.getMulticastECMPL1XIds();

  status = this->mcMGIDValidateDataMbrsSize(multicast_node_ids.size(),
                                            node_l1_xids.size(),
                                            node_l1_xids_valid.size(),
                                            multicast_ecmp_ids.size(),
                                            ecmp_l1_xids.size(),
                                            ecmp_l1_xids_valid.size());
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error in modifying PRE MGID %d incrementally due to "
        "invalid number of data members",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        mgid);
    return status;
  }

  // All BF-RT validations are passed, now add or delete
  // all given multicast nodes to the multicast group based on the given flag
  if (BF_RT_FLAG_IS_SET(flags, BF_RT_INC_DEL)) {
    status = this->mcMGIDNodeMbrsDel(
        session, dev_tgt, *pre_state, mgid, mgrp_hdl, multicast_node_ids);
  } else {
    status = this->mcMGIDNodeMbrsAdd(session,
                                     dev_tgt,
                                     *pre_state,
                                     mgid,
                                     mgrp_hdl,
                                     multicast_node_ids,
                                     node_l1_xids_valid,
                                     node_l1_xids);
  }

  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in %s node members with mgid index %d, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              BF_RT_FLAG_IS_SET(flags, BF_RT_INC_DEL) ? "dissociating"
                                                      : "associating",
              mgid,
              status)
    return status;
  }

  // Now add or delete all given multicas ECMPs
  // to the multicast group based on the given flag
  if (BF_RT_FLAG_IS_SET(flags, BF_RT_INC_DEL)) {
    status = this->mcMGIDEcmpMbrsDel(
        session, dev_tgt, *pre_state, mgid, mgrp_hdl, multicast_ecmp_ids);
  } else {
    status = this->mcMGIDEcmpMbrsAdd(session,
                                     dev_tgt,
                                     *pre_state,
                                     mgid,
                                     mgrp_hdl,
                                     multicast_ecmp_ids,
                                     ecmp_l1_xids_valid,
                                     ecmp_l1_xids);
  }

  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in %s ECMP members with mgid index %d, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              BF_RT_FLAG_IS_SET(flags, BF_RT_INC_DEL) ? "dissociating"
                                                      : "associating",
              mgid,
              status)
    return status;
  }

  return status;
}

bf_status_t BfRtPREMGIDTable::tableEntryDel(const BfRtSession &session,
                                            const bf_rt_target_t &dev_tgt,
                                            const uint64_t & /*flags*/,
                                            const BfRtTableKey &key) const {
  bf_status_t status = BF_SUCCESS;
  const uint64_t flags = 0;
  const BfRtPREMGIDTableKey &mgid_key =
      static_cast<const BfRtPREMGIDTableKey &>(key);

  // Get the MGID from key
  bf_mc_grp_id_t mgid = mgid_key.getId();

  // Get the device state and from that get the PRE state to perform
  // various checks and update new states.
  std::lock_guard<std::mutex> lock(state_lock);
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, this->prog_name);
  if (device_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  auto pre_state = device_state->preState.getStateObj();
  if (pre_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  // Check if the MGID exists already or not
  if (pre_state->statePREIdExists(
          BfRtPREStateObj::PREStateIdType::MULTICAST_MGID, mgid) != true) {
    LOG_TRACE(
        "%s:%d %s : Error in deleting PRE MGID %d which doesn't "
        "exist",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        mgid);
    return BF_OBJECT_NOT_FOUND;
  }

  // Get the mgrp handle for the MGID
  bf_mc_mgrp_hdl_t mgrp_hdl;
  status = pre_state->statePREHdlGet(
      BfRtPREStateObj::PREStateIdType::MULTICAST_MGID, mgid, &mgrp_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error in getting PRE MGID handle for MGID %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              mgid);
    return status;
  }
  // Call the internal function
  status = this->tableEntryDel_internal(
      session, dev_tgt, *pre_state, flags, key, mgrp_hdl, mgid);
  return status;
}

// Exactly the same as BfRtPREMGIDTable::tableEntryDel, but the function
// assumes it is called with state_lock already taken, so it
// doesn't take it. For internal use only.
bf_status_t BfRtPREMGIDTable::tableEntryDel_internal(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    BfRtPREStateObj &pre_state,
    const uint64_t & /*flags*/,
    const BfRtTableKey & /*key*/,
    bf_mc_mgrp_hdl_t &mgrp_hdl,
    bf_mc_grp_id_t &mgid) const {
  bf_status_t status = BF_SUCCESS;

  // First delete the mgid<-->mgrp_hdl mapping and in case
  // deleting MC group in MC mgr fails for any reason, the same
  // mgid<-->mgrp_hdl mapping can be added again to have
  // BF-RT state in sync with MC mgr state.
  status = pre_state.statePREIdDel(
      BfRtPREStateObj::PREStateIdType::MULTICAST_MGID, mgid);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error in deleting mgid<-->handle mapping for mgid %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              mgid);
    return status;
  }

  // All BF-RT validations have passed, delete MGID in MC mgr now
  // We don't have to dissociate node and ECMP members in this MGID tree,
  // MC mgr will take care of this
  auto *mcMgr = McMgrIntf::getInstance(session);
  status = mcMgr->mcMgrMgrpDestroy(
      session.preSessHandleGet(), dev_tgt.dev_id, mgrp_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s Error in deleting MC group for mgid %d, "
        "err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        mgid,
        status);

    // Since MC mgr failed to delete the MGID, restore the mgid<-->handle
    // mapping in BF-RT. This is to make sure BF_RT states and MC mgr
    // states are in sync.
    bf_status_t temp_sts = pre_state.statePREIdAdd(
        BfRtPREStateObj::PREStateIdType::MULTICAST_MGID, mgid, mgrp_hdl);
    if (temp_sts != BF_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s Error in updating mgid<-->handle mapping for mgid %d, "
          "err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          mgid,
          temp_sts);
    }
  }

  return status;
}

// Exactly the same as BfRtPREMGIDTable::tableEntryAdd, but the function
// assumes it is called with state_lock already taken, so it
// doesn't take it. For internal use only.
bf_status_t BfRtPREMGIDTable::tableEntryAdd_internal(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    BfRtPREStateObj &pre_state,
    const uint64_t & /*flags*/,
    const BfRtTableKey & /*key*/,
    const BfRtTableData &data,
    bf_mc_mgrp_hdl_t &mgrp_hdl,
    bf_mc_grp_id_t &mgid) const {
  bf_status_t status = BF_SUCCESS;

  const BfRtPREMGIDTableData &mgid_data =
      static_cast<const BfRtPREMGIDTableData &>(data);

  // Get and validate the data fields
  const std::vector<bf_rt_id_t> &multicast_node_ids =
      mgid_data.getMulticastNodeIds();
  const std::vector<bool> &node_l1_xids_valid =
      mgid_data.getMulticastNodeL1XIdsValid();
  const std::vector<bf_rt_id_t> &node_l1_xids =
      mgid_data.getMulticastNodeL1XIds();
  const std::vector<bf_rt_id_t> &multicast_ecmp_ids =
      mgid_data.getMulticastECMPIds();
  const std::vector<bool> &ecmp_l1_xids_valid =
      mgid_data.getMulticastECMPL1XIdsValid();
  const std::vector<bf_rt_id_t> &ecmp_l1_xids =
      mgid_data.getMulticastECMPL1XIds();

  status = this->mcMGIDValidateDataMbrsSize(multicast_node_ids.size(),
                                            node_l1_xids.size(),
                                            node_l1_xids_valid.size(),
                                            multicast_ecmp_ids.size(),
                                            ecmp_l1_xids.size(),
                                            ecmp_l1_xids_valid.size());
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error in adding PRE MGID %d due to invalid "
        "number of data members",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        mgid);
    return status;
  }

  // All BF-RT validations are passed, now create multicast group for
  // the given MGID
  auto *mcMgr = McMgrIntf::getInstance(session);
  status = mcMgr->mcMgrMgrpCreate(
      session.preSessHandleGet(), dev_tgt.dev_id, mgid, &mgrp_hdl);

  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s Error in creating multicast group for mgid index %d, err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        mgid,
        status);
    return status;
  }

  // Associate all given multicast nodes to the newly created multicast group

  status = this->mcMGIDNodeMbrsAdd(session,
                                   dev_tgt,
                                   pre_state,
                                   mgid,
                                   mgrp_hdl,
                                   multicast_node_ids,
                                   node_l1_xids_valid,
                                   node_l1_xids);
  if (status != BF_SUCCESS) {
    // If any error is encountered while associating nodes to the
    // multicast group, delete the multicast group and return
    // original error status. This is to keep both BF-RT and MC mgr
    // states in sync.
    bf_status_t temp_sts = mcMgr->mcMgrMgrpDestroy(
        session.preSessHandleGet(), dev_tgt.dev_id, mgrp_hdl);
    if (temp_sts != BF_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s Error in deleting newly created MC group for mgid %d, "
          "err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          mgid,
          temp_sts);
    }
    // Return original error status
    return status;
  }

  // Now associate all given ECMPs  to the newly created multicast group
  status = this->mcMGIDEcmpMbrsAdd(session,
                                   dev_tgt,
                                   pre_state,
                                   mgid,
                                   mgrp_hdl,
                                   multicast_ecmp_ids,
                                   ecmp_l1_xids_valid,
                                   ecmp_l1_xids);
  if (status != BF_SUCCESS) {
    // If any error is encountered while associating nodes to the
    // multicast group, delete the multicast group and return
    // original error status. This is to keep both BF-RT and MC mgr
    // states in sync.
    bf_status_t temp_sts = mcMgr->mcMgrMgrpDestroy(
        session.preSessHandleGet(), dev_tgt.dev_id, mgrp_hdl);
    if (temp_sts != BF_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s Error in deleting newly created MC group for mgid %d, "
          "err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          mgid,
          temp_sts);
    }
    // Return original error status
    return status;
  }

  // Now that mgid is created successfully, update the mgid<--> handle
  // mapping in PRE state object
  status = pre_state.statePREIdAdd(
      BfRtPREStateObj::PREStateIdType::MULTICAST_MGID, mgid, mgrp_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s Error in updating mgid<-->handle mapping for mgid %d, err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        mgid,
        status);

    // Failed to update mgid<--> handle mapping in BF-RT (ideally this
    // should never be hit). Delete the
    // MC group to have both BF-RT and MC mgr states in sync.
    bf_status_t temp_sts = mcMgr->mcMgrMgrpDestroy(
        session.preSessHandleGet(), dev_tgt.dev_id, mgrp_hdl);
    if (temp_sts != BF_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s Error in deleting MC group for mgid %d, "
          "err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          mgid,
          temp_sts);
    }
    // Return original error status
    return status;
  }

  return status;
}

bf_status_t BfRtPREMGIDTable::tableEntryGet(const BfRtSession &session,
                                            const bf_rt_target_t &dev_tgt,
                                            const uint64_t & /*flags*/,
                                            const BfRtTableKey &key,
                                            BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;

  const BfRtPREMGIDTableKey &mgid_key =
      static_cast<const BfRtPREMGIDTableKey &>(key);
  BfRtPREMGIDTableData *mgid_data = static_cast<BfRtPREMGIDTableData *>(data);

  // Get the MGID from key
  bf_mc_grp_id_t mgid = mgid_key.getId();

  // Get the device state and from that get the PRE state to perform
  // various checks and get IDs from handles for MC objects.
  std::lock_guard<std::mutex> lock(state_lock);
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, this->prog_name);
  if (device_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  auto pre_state = device_state->preState.getStateObj();
  if (pre_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  // Check if the MGID exists already or not
  if (pre_state->statePREIdExists(
          BfRtPREStateObj::PREStateIdType::MULTICAST_MGID, mgid) != true) {
    LOG_TRACE(
        "%s:%d %s : Error in EntryGet for PRE MGID %d which doesn't "
        "exist",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        mgid);
    return BF_OBJECT_NOT_FOUND;
  }

  // First get the mgrp handle for the MGID
  bf_mc_mgrp_hdl_t mgrp_hdl;
  status = pre_state->statePREHdlGet(
      BfRtPREStateObj::PREStateIdType::MULTICAST_MGID, mgid, &mgrp_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error in getting PRE MGID handle for MGID %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              mgid);
    return status;
  }

  // Get the data for the given MGID and return
  return this->tableEntryGet_internal(
      session, dev_tgt, *pre_state, mgid, mgrp_hdl, mgid_data);
}

bf_status_t BfRtPREMGIDTable::tableEntryGetFirst(const BfRtSession &session,
                                                 const bf_rt_target_t &dev_tgt,
                                                 const uint64_t & /*flags*/,
                                                 BfRtTableKey *key,
                                                 BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;

  BfRtPREMGIDTableKey *mgid_key = static_cast<BfRtPREMGIDTableKey *>(key);
  BfRtPREMGIDTableData *mgid_data = static_cast<BfRtPREMGIDTableData *>(data);

  // Get the device state and from that get the PRE state to
  // get the first MGID and its handle
  std::lock_guard<std::mutex> lock(state_lock);
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, this->prog_name);
  if (device_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  auto pre_state = device_state->preState.getStateObj();
  if (pre_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  // Get the first MGID and its handle from PRE State
  bf_rt_id_t mgid;
  bf_mc_mgrp_hdl_t mgrp_hdl;

  status = pre_state->statePREIdGetFirst(
      BfRtPREStateObj::PREStateIdType::MULTICAST_MGID, &mgid, &mgrp_hdl);
  if (status != BF_SUCCESS && status != BF_OBJECT_NOT_FOUND) {
    LOG_TRACE("%s:%d %s: Error in getting first MGID",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  if (status == BF_OBJECT_NOT_FOUND) {
    // This means that we don't have any MGIDs added in the system
    LOG_DBG("%s:%d %s : No MGIDs added in the system",
            __func__,
            __LINE__,
            table_name_get().c_str());
    return status;
  }

  // Set the MGID in the key that to be returned
  mgid_key->setId(static_cast<bf_mc_grp_id_t>(mgid));

  // Get the data for the first MGID and return the status
  return this->tableEntryGet_internal(
      session, dev_tgt, *pre_state, mgid, mgrp_hdl, mgid_data);
}

bf_status_t BfRtPREMGIDTable::tableEntryGetNext_n(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    const uint32_t &n,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  bf_status_t status = BF_SUCCESS;

  // First get the MGID from key
  const BfRtPREMGIDTableKey &mgid_key =
      static_cast<const BfRtPREMGIDTableKey &>(key);
  bf_mc_grp_id_t mgid = mgid_key.getId();

  // Get the device state and from that get the PRE state to
  // get the next MGIDs and its handle
  std::lock_guard<std::mutex> lock(state_lock);
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, this->prog_name);
  if (device_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  auto pre_state = device_state->preState.getStateObj();
  if (pre_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  // Get the next_n entries
  bf_rt_id_t next_mgid;
  bf_mc_mgrp_hdl_t next_mgrp_hdl;
  uint32_t i;
  // For each iteration, get the next MGID & its handle from PRE state and
  // then get the data for the next MGID from MC mgr
  for (i = 0; i < n; i++) {
    auto this_mgid_key =
        static_cast<BfRtPREMGIDTableKey *>((*key_data_pairs)[i].first);
    auto this_mgid_data =
        static_cast<BfRtPREMGIDTableData *>((*key_data_pairs)[i].second);

    // Get the next MGID and it handle from PRE state
    status = pre_state->statePREIdGetNext(
        BfRtPREStateObj::PREStateIdType::MULTICAST_MGID,
        mgid,
        &next_mgid,
        &next_mgrp_hdl);
    if (BF_SUCCESS != status && BF_OBJECT_NOT_FOUND != status) {
      LOG_ERROR("%s:%d %s: Error in getting next MGID for curr MGID %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                mgid);
      break;
    }
    if (BF_OBJECT_NOT_FOUND == status) {
      // This means that we have successfully read all the MGIDs
      break;
    }

    // Set the MGID in the key that to be returned
    this_mgid_key->setId(static_cast<bf_mc_grp_id_t>(next_mgid));

    // Get the next MGID's data from MC mgr
    status = this->tableEntryGet_internal(
        session, dev_tgt, *pre_state, next_mgid, next_mgrp_hdl, this_mgid_data);
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s: Error in getting data of MGID %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                next_mgid);
      break;
    }

    // Update the MGID for next iteration
    mgid = next_mgid;
  }

  // Update the number of entries that are read successfully
  if (num_returned) {
    *num_returned = i;
  }

  return status;
}

bf_status_t BfRtPREMGIDTable::tableUsageGet(const BfRtSession &session,
                                            const bf_rt_target_t &dev_tgt,
                                            const uint64_t & /*flags*/,
                                            uint32_t *count) const {
  bf_status_t status = BF_SUCCESS;

  // Call the MC mgr API to get the number of MGIDs created
  auto *mcMgr = McMgrIntf::getInstance(session);
  status = mcMgr->mcMgrMgrpGetCount(
      session.preSessHandleGet(), dev_tgt.dev_id, count);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in getting the number of MGIDs, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }

  return status;
}

bf_status_t BfRtPREMGIDTable::keyReset(BfRtTableKey *key) const {
  BfRtPREMGIDTableKey *mgid_key = static_cast<BfRtPREMGIDTableKey *>(key);

  return key_reset<BfRtPREMGIDTable, BfRtPREMGIDTableKey>(*this, mgid_key);
}

bf_status_t BfRtPREMGIDTable::dataReset(BfRtTableData *data) const {
  BfRtPREMGIDTableData *mgid_data = static_cast<BfRtPREMGIDTableData *>(data);

  return data_reset<BfRtPREMGIDTable, BfRtPREMGIDTableData>(*this, mgid_data);
}

bf_status_t BfRtPREMGIDTable::tableEntryGet_internal(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const BfRtPREStateObj &pre_state,
    const bf_rt_id_t &mgid,
    const bf_mc_mgrp_hdl_t &mgrp_hdl,
    BfRtPREMGIDTableData *mgid_data) const {
  bf_status_t status = BF_SUCCESS;

  // Each MGID entry consists of L1 nodes and ECMPs. Use the helper
  // methods to get the L1 nodes and ECMPs

  std::vector<bf_rt_id_t> multicast_node_ids;
  std::vector<bool> node_l1_xids_valid;
  std::vector<bf_rt_id_t> node_l1_xids;
  std::vector<bf_rt_id_t> multicast_ecmp_ids;
  std::vector<bool> ecmp_l1_xids_valid;
  std::vector<bf_rt_id_t> ecmp_l1_xids;

  status = this->mcMGIDNodeMbrsGet(session,
                                   dev_tgt,
                                   pre_state,
                                   mgid,
                                   mgrp_hdl,
                                   &multicast_node_ids,
                                   &node_l1_xids_valid,
                                   &node_l1_xids);
  if (status != BF_SUCCESS && status != BF_OBJECT_NOT_FOUND) {
    // BF_OBJECT_NOT_FOUND is not an error case as there can be multicast
    // group with no L1 nodes
    // For other error cases, error logging is already done, just return
    return status;
  }

  status = this->mcMGIDECMPMbrsGet(session,
                                   dev_tgt,
                                   pre_state,
                                   mgid,
                                   mgrp_hdl,
                                   &multicast_ecmp_ids,
                                   &ecmp_l1_xids_valid,
                                   &ecmp_l1_xids);
  if (status != BF_SUCCESS && status != BF_OBJECT_NOT_FOUND) {
    // BF_OBJECT_NOT_FOUND is not an error case as there can be multicast
    // group with no ECMP nodes
    // For other error cases, error logging is already done, just return
    return status;
  }

  // Update the data fields with members returned by MC mgr
  // TODO: For better performance, try to avoid this copying
  // like how it's done for EntryAdd and EntryMod
  mgid_data->setMulticastNodeIds(multicast_node_ids);
  mgid_data->setMulticastNodeL1XIdsValid(node_l1_xids_valid);
  mgid_data->setMulticastNodeL1XIds(node_l1_xids);
  mgid_data->setMulticastECMPIds(multicast_ecmp_ids);
  mgid_data->setMulticastECMPL1XIdsValid(ecmp_l1_xids_valid);
  mgid_data->setMulticastECMPL1XIds(ecmp_l1_xids);

  return BF_SUCCESS;
}

bf_status_t BfRtPREMGIDTable::mcMGIDValidateDataMbrsSize(
    const std::vector<bf_rt_id_t>::size_type &nodes_size,
    const std::vector<bf_rt_id_t>::size_type &node_xids_size,
    const std::vector<bool>::size_type &node_xids_valid_size,
    const std::vector<bf_rt_id_t>::size_type &ecmps_size,
    const std::vector<bf_rt_id_t>::size_type &ecmp_xids_size,
    const std::vector<bool>::size_type &ecmp_xids_valid_size) const {
  bf_status_t status = BF_SUCCESS;

  // Number of entries of node ids, node l1_xids and node l1_xids_valid
  // should all be equal
  if (nodes_size != node_xids_size) {
    LOG_TRACE(
        "%s:%d %s Error : Multicast Node IDs size %zu and "
        "Node L1 XIDs size %zu do not match",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        nodes_size,
        node_xids_size);
    return BF_INVALID_ARG;
  }

  if (node_xids_size != node_xids_valid_size) {
    LOG_TRACE(
        "%s:%d %s Error : Multicast Node L1 XIDs size %zu and "
        "Node L1 XIDs Valid size %zu do not match",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        node_xids_size,
        node_xids_valid_size);
    return BF_INVALID_ARG;
  }

  // Number of entries of ECMP ids, ECMP l1_xids and ECMP l1_xids_valid
  // should all be equal
  if (ecmps_size != ecmp_xids_size) {
    LOG_TRACE(
        "%s:%d %s Error : Multicast ECMP IDs size %zu and "
        "ECMP L1 XIDs size %zu do not match",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        ecmps_size,
        ecmp_xids_size);
    return BF_INVALID_ARG;
  }

  if (ecmp_xids_size != ecmp_xids_valid_size) {
    LOG_TRACE(
        "%s:%d %s Error : Multicast ECMP L1 XIDs size %zu and "
        "ECMP L1 XIDs Valid size %zu do not match",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        ecmp_xids_size,
        ecmp_xids_valid_size);
    return BF_INVALID_ARG;
  }

  return status;
}

bf_status_t BfRtPREMGIDTable::mcMGIDNodeMbrsAdd(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const BfRtPREStateObj &pre_state,
    const bf_rt_id_t &mgid,
    const bf_mc_mgrp_hdl_t &mgrp_hdl,
    const std::vector<bf_rt_id_t> &multicast_node_ids,
    const std::vector<bool> &node_l1_xids_valid,
    const std::vector<bf_rt_id_t> &node_l1_xids) const {
  bf_status_t status = BF_SUCCESS;

  // Associate all given multicast nodes to the given multicast group
  auto *mcMgr = McMgrIntf::getInstance(session);
  bf_mc_node_hdl_t node_hdl;
  for (uint32_t i = 0; i < multicast_node_ids.size(); i++) {
    status = pre_state.statePREHdlGet(
        BfRtPREStateObj::PREStateIdType::MULTICAST_NODE_ID,
        multicast_node_ids[i],
        &node_hdl);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error in getting node handle for node id %d while creating "
          "MC group for mgid %d, err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          multicast_node_ids[i],
          mgid,
          status);
      return status;
    }

    status = mcMgr->mcMgrAssociateNode(session.preSessHandleGet(),
                                       dev_tgt.dev_id,
                                       mgrp_hdl,
                                       node_hdl,
                                       node_l1_xids_valid[i],
                                       node_l1_xids[i]);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error in associating node id %d with mgid index %d, err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          multicast_node_ids[i],
          mgid,
          status);

      return status;
    }
  }

  return status;
}

bf_status_t BfRtPREMGIDTable::mcMGIDNodeMbrsDel(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const BfRtPREStateObj &pre_state,
    const bf_rt_id_t &mgid,
    const bf_mc_mgrp_hdl_t &mgrp_hdl,
    const std::vector<bf_rt_id_t> &multicast_node_ids) const {
  bf_status_t status = BF_SUCCESS;

  // Dissociate all given multicast nodes from the given multicast group
  auto *mcMgr = McMgrIntf::getInstance(session);
  bf_mc_node_hdl_t node_hdl;
  for (uint32_t i = 0; i < multicast_node_ids.size(); i++) {
    status = pre_state.statePREHdlGet(
        BfRtPREStateObj::PREStateIdType::MULTICAST_NODE_ID,
        multicast_node_ids[i],
        &node_hdl);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error in getting node handle for node id %d to "
          "delete it from MC group for mgid %d, err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          multicast_node_ids[i],
          mgid,
          status);

      return status;
    }

    status = mcMgr->mcMgrDissociateNode(
        session.preSessHandleGet(), dev_tgt.dev_id, mgrp_hdl, node_hdl);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error in dissociating node id %d from mgid index %d, err "
          "%d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          multicast_node_ids[i],
          mgid,
          status);

      return status;
    }
  }

  return status;
}

bf_status_t BfRtPREMGIDTable::mcMGIDEcmpMbrsAdd(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const BfRtPREStateObj &pre_state,
    const bf_rt_id_t &mgid,
    const bf_mc_mgrp_hdl_t &mgrp_hdl,
    const std::vector<bf_rt_id_t> &multicast_ecmp_ids,
    const std::vector<bool> &ecmp_l1_xids_valid,
    const std::vector<bf_rt_id_t> &ecmp_l1_xids) const {
  bf_status_t status = BF_SUCCESS;

  // Associate all given multicast ECMPs to the given multicast group
  auto *mcMgr = McMgrIntf::getInstance(session);
  bf_mc_ecmp_hdl_t ecmp_hdl;
  for (uint32_t i = 0; i < multicast_ecmp_ids.size(); i++) {
    status = pre_state.statePREHdlGet(
        BfRtPREStateObj::PREStateIdType::MULTICAST_ECMP_ID,
        multicast_ecmp_ids[i],
        &ecmp_hdl);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error in getting ECMP handle for ECMP id %d while adding "
          "it to MC group for mgid %d, err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          multicast_ecmp_ids[i],
          mgid,
          status);

      return status;
    }

    status = mcMgr->mcMgrAssociateEcmp(session.preSessHandleGet(),
                                       dev_tgt.dev_id,
                                       mgrp_hdl,
                                       ecmp_hdl,
                                       ecmp_l1_xids_valid[i],
                                       ecmp_l1_xids[i]);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error in associating ECMP id %d with  mgid index %d, err "
          "%d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          multicast_ecmp_ids[i],
          mgid,
          status);

      return status;
    }
  }

  return status;
}

bf_status_t BfRtPREMGIDTable::mcMGIDEcmpMbrsDel(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const BfRtPREStateObj &pre_state,
    const bf_rt_id_t &mgid,
    const bf_mc_mgrp_hdl_t &mgrp_hdl,
    const std::vector<bf_rt_id_t> &multicast_ecmp_ids) const {
  bf_status_t status = BF_SUCCESS;

  // Dissociate all given multicast ECMPs to the given multicast group
  auto *mcMgr = McMgrIntf::getInstance(session);
  bf_mc_ecmp_hdl_t ecmp_hdl;
  for (uint32_t i = 0; i < multicast_ecmp_ids.size(); i++) {
    status = pre_state.statePREHdlGet(
        BfRtPREStateObj::PREStateIdType::MULTICAST_ECMP_ID,
        multicast_ecmp_ids[i],
        &ecmp_hdl);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error in getting ECMP handle for ECMP id %d to delete "
          "it from MC group for mgid %d, err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          multicast_ecmp_ids[i],
          mgid,
          status);

      return status;
    }

    status = mcMgr->mcMgrDissociateEcmp(
        session.preSessHandleGet(), dev_tgt.dev_id, mgrp_hdl, ecmp_hdl);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error in dissociating ECMP id %d from  mgid index %d, err "
          "%d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          multicast_ecmp_ids[i],
          mgid,
          status);

      return status;
    }
  }

  return status;
}

bf_status_t BfRtPREMGIDTable::mcMGIDNodeMbrsGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const BfRtPREStateObj &pre_state,
    const bf_rt_id_t &mgid,
    const bf_mc_mgrp_hdl_t &mgrp_hdl,
    std::vector<bf_rt_id_t> *multicast_node_ids,
    std::vector<bool> *node_l1_xids_valid,
    std::vector<bf_rt_id_t> *node_l1_xids) const {
  bf_status_t status = BF_SUCCESS;

  // Get the list of nodes in the multicast group as below -
  //      - Get the first node member of the MGID
  //      - If at least one node member exists, get the total number of
  //         node members
  //      - Get the next (count - 1) members after the first node

  auto *mcMgr = McMgrIntf::getInstance(session);

  // Get the first node member of the MGID
  bf_mc_node_hdl_t node_hdl;
  bool node_l1_xid_valid;
  bf_mc_l1_xid_t node_l1_xid;
  status = mcMgr->mcMgrMgrpGetFirstNodeMbr(session.preSessHandleGet(),
                                           dev_tgt.dev_id,
                                           mgrp_hdl,
                                           &node_hdl,
                                           &node_l1_xid_valid,
                                           &node_l1_xid);

  if (status == BF_OBJECT_NOT_FOUND) {
    LOG_DBG("%s:%d %s No node members in multicast group for mgid index %d",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            mgid);
    return status;
  }

  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s Error in getting first node member in multicast group "
        "for mgid index %d, err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        mgid,
        status);
    return status;
  }

  // Get the node id for the returned node handle
  bf_rt_id_t mc_node_id;
  status = pre_state.statePREIdGet(
      BfRtPREStateObj::PREStateIdType::MULTICAST_NODE_ID,
      node_hdl,
      &mc_node_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error in getting PRE node id for node handle %#x",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              node_hdl);
    return BF_UNEXPECTED;
  }

  // Add the node_id, l1_xid_valid and l1_xid to corresponding vectors
  multicast_node_ids->push_back(mc_node_id);
  node_l1_xids_valid->push_back(node_l1_xid_valid);
  node_l1_xids->push_back(node_l1_xid);

  // Get the total number of L1 nodes in the multicast group
  uint32_t node_count;
  status = mcMgr->mcMgrMgrpGetNodeMbrCount(
      session.preSessHandleGet(), dev_tgt.dev_id, mgrp_hdl, &node_count);
  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s Error in getting total count of node members in multicast "
        "group for mgid index %d, err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        mgid,
        status);
  }

  uint32_t i = node_count - 1;  // Exclude first node member
  if (i == 0) {
    // No other node members exist in the multicast group
    return status;
  }

  // Get the remaining node members
  // For this, allocate vectors for 'i' members and pass the vector memory
  // to MC mgr to return the remaining node members
  std::vector<bf_mc_node_hdl_t> node_hdls(i, 0);
  // Note:
  //     For l1_xids_valid, ideally we need to use vector or array of bool.
  //     vector of bool is a special container that is optimized for efficient
  //     memory usage and hence it may not be contiguous and vector::data
  //     wouldn't work for vector of bools. For std::array of bool, the size
  //     has to be known at compile time. So, we use vector of uint8_t
  //     and cast the underlying memory to bool to pass it to MC mgr.
  //     This is all done to avoid raw new[] and delete[].
  std::vector<uint8_t> l1_xids_valid(i, 0);
  std::vector<bf_mc_l1_xid_t> l1_xids(i, 0);
  bf_mc_node_hdl_t *node_hdls_ptr = node_hdls.data();
  bool *node_l1_xids_valid_ptr = reinterpret_cast<bool *>(l1_xids_valid.data());
  bf_mc_l1_xid_t *node_l1_xids_ptr = l1_xids.data();

  status = mcMgr->mcMgrMgrpGetNextINodeMbr(session.preSessHandleGet(),
                                           dev_tgt.dev_id,
                                           mgrp_hdl,
                                           node_hdl,
                                           i,
                                           node_hdls_ptr,
                                           node_l1_xids_valid_ptr,
                                           node_l1_xids_ptr);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s Error in getting next i node members in multicast group "
        "for mgid index %d, err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        mgid,
        status);
    return status;
  }

  for (uint32_t index = 0; index < i; index++) {
    if (node_hdls_ptr[index] == (~0U)) {
      // If number of nodes is less than i for
      // any valid reason, MC mgr sets the remaining entries
      // to -1. We need to stop processing if we hit the first
      // such node handle.
      break;
    }

    // Get the node id from the node handle
    status = pre_state.statePREIdGet(
        BfRtPREStateObj::PREStateIdType::MULTICAST_NODE_ID,
        node_hdls_ptr[index],
        &mc_node_id);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s : Error in getting PRE node id for node handle %#x, "
          "index %u",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          node_hdls_ptr[index],
          index);
      return BF_UNEXPECTED;
    }

    // Add the node_id, l1_xid_valid and l1_xid to corresponding vectors
    multicast_node_ids->push_back(mc_node_id);
    node_l1_xids_valid->push_back(node_l1_xids_valid_ptr[index]);
    node_l1_xids->push_back(node_l1_xids_ptr[index]);
  }

  return status;
}

bf_status_t BfRtPREMGIDTable::mcMGIDECMPMbrsGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const BfRtPREStateObj &pre_state,
    const bf_rt_id_t &mgid,
    const bf_mc_mgrp_hdl_t &mgrp_hdl,
    std::vector<bf_rt_id_t> *multicast_ecmp_ids,
    std::vector<bool> *ecmp_l1_xids_valid,
    std::vector<bf_rt_id_t> *ecmp_l1_xids) const {
  bf_status_t status = BF_SUCCESS;

  // Get the list of ECMPs in the multicast group as below -
  //      - Get the first ECMP member of the MGID
  //      - If at least one ECMP member exists, get the total number of
  //         ECMP members
  //      - Get the next (count - 1) members after the first ECMP

  auto *mcMgr = McMgrIntf::getInstance(session);

  // Get the first ECMP member of the MGID
  bf_mc_ecmp_hdl_t ecmp_hdl;
  bool ecmp_l1_xid_valid;
  bf_mc_l1_xid_t ecmp_l1_xid;
  status = mcMgr->mcMgrMgrpGetFirstEcmpMbr(session.preSessHandleGet(),
                                           dev_tgt.dev_id,
                                           mgrp_hdl,
                                           &ecmp_hdl,
                                           &ecmp_l1_xid_valid,
                                           &ecmp_l1_xid);

  if (status == BF_OBJECT_NOT_FOUND) {
    LOG_DBG("%s:%d %s No ECMP members in multicast group for mgid index %d",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            mgid);
    return status;
  }

  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s Error in getting first ECMP member in multicast group "
        "for mgid index %d, err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        mgid,
        status);
    return status;
  }

  // Get the ECMP id for the returned ECMP handle
  bf_rt_id_t mc_ecmp_id;
  status = pre_state.statePREIdGet(
      BfRtPREStateObj::PREStateIdType::MULTICAST_ECMP_ID,
      ecmp_hdl,
      &mc_ecmp_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error in getting PRE ECMP id for ECMP handle %#x",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              ecmp_hdl);
    return BF_UNEXPECTED;
  }

  // Add the ecmp_id, l1_xid_valid and l1_xid to corresponding vectors
  multicast_ecmp_ids->push_back(mc_ecmp_id);
  ecmp_l1_xids_valid->push_back(ecmp_l1_xid_valid);
  ecmp_l1_xids->push_back(ecmp_l1_xid);

  // Get the total number of ECMP nodes in the multicast group
  uint32_t ecmp_count;
  status = mcMgr->mcMgrMgrpGetEcmpMbrCount(
      session.preSessHandleGet(), dev_tgt.dev_id, mgrp_hdl, &ecmp_count);
  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s Error in getting total count of ECMP members in multicast "
        "group for mgid index %d, err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        mgid,
        status);
  }

  uint32_t i = ecmp_count - 1;  // Exclude first node member
  if (i == 0) {
    // No other ECMP members exist in the multicast group
    return status;
  }

  // Get the remaining ECMP members
  // For this, allocate vectors for 'i' members and pass the vector memory
  // to MC mgr to return the remaining ECMP members
  std::vector<bf_mc_ecmp_hdl_t> ecmp_hdls(i, 0);
  // Note:
  //     For l1_xids_valid, ideally we need to use vector or array of bool.
  //     vector of bool is a special container that is optimized for efficient
  //     memory usage and hence it may not be contiguous and vector::data
  //     wouldn't work for vector of bools. For std::array of bool, the size
  //     has to be known at compile time. So, we use vector of uint8_t
  //     and cast the underlying memory to bool to pass it to MC mgr.
  //     This is all done to avoid raw new[] and delete[].
  std::vector<uint8_t> l1_xids_valid(i, 0);
  std::vector<bf_mc_l1_xid_t> l1_xids(i, 0);
  bf_mc_node_hdl_t *ecmp_hdls_ptr = ecmp_hdls.data();
  bool *ecmp_l1_xids_valid_ptr = reinterpret_cast<bool *>(l1_xids_valid.data());
  bf_mc_l1_xid_t *ecmp_l1_xids_ptr = l1_xids.data();

  status = mcMgr->mcMgrMgrpGetNextIEcmpMbr(session.preSessHandleGet(),
                                           dev_tgt.dev_id,
                                           mgrp_hdl,
                                           ecmp_hdl,
                                           i,
                                           ecmp_hdls_ptr,
                                           ecmp_l1_xids_valid_ptr,
                                           ecmp_l1_xids_ptr);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s Error in getting next i ECMP members in multicast group "
        "for mgid index %d, err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        mgid,
        status);
    return status;
  }

  for (uint32_t index = 0; index < i; index++) {
    if (ecmp_hdls_ptr[index] == (~0U)) {
      // If number of ecmps is less than i for
      // any valid reason, MC mgr sets the remaining entries
      // to -1. We need to stop processing if we hit the first
      // such ecmp handle.
      break;
    }

    // Get ECMP ids from ECMP handles
    status = pre_state.statePREIdGet(
        BfRtPREStateObj::PREStateIdType::MULTICAST_ECMP_ID,
        ecmp_hdls_ptr[index],
        &mc_ecmp_id);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s : Error in getting PRE node id for node handle %#x, "
          "index %u",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          ecmp_hdls_ptr[index],
          index);
      return BF_UNEXPECTED;
    }

    // Add the ecmp_id, l1_xid_valid and l1_xid to corresponding vectors
    multicast_ecmp_ids->push_back(mc_ecmp_id);
    ecmp_l1_xids_valid->push_back(ecmp_l1_xids_valid_ptr[index]);
    ecmp_l1_xids->push_back(ecmp_l1_xids_ptr[index]);
  }

  return status;
}

bf_status_t BfRtPREMGIDTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtPREMGIDTableKey(this));
  if (*key_ret == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to allocate key",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPREMGIDTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret = std::unique_ptr<BfRtTableData>(new BfRtPREMGIDTableData(this));
  if (*data_ret == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to allocate data",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPREMGIDTable::attributeAllocate(
    const TableAttributesType &type,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  bf_status_t status = BF_SUCCESS;

  std::set<TableAttributesType> attribute_type_set;
  status = tableAttributesSupported(&attribute_type_set);
  if (status != BF_SUCCESS ||
      (attribute_type_set.find(type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Error : Attribute %d is not supported",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              static_cast<int>(type));
    return BF_NOT_SUPPORTED;
  }

  *attr = std::unique_ptr<BfRtTableAttributes>(
      new BfRtTableAttributesImpl(this, type));
  if (*attr == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to allocate attribute object",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtPREMGIDTable::attributeReset(
    const TableAttributesType &type,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  auto &tbl_attr_impl = static_cast<BfRtTableAttributesImpl &>(*(attr->get()));

  // Make sure attribute type is valid
  if (type != TableAttributesType::PRE_DEVICE_CONFIG) {
    LOG_TRACE("%s:%d %s Error in trying to reset Invalid Attribute Type %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              static_cast<int>(type));
    return BF_INVALID_ARG;
  }

  return tbl_attr_impl.resetAttributeType(type);
}

bf_status_t BfRtPREMGIDTable::tableAttributesSet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableAttributes &tableAttributes) const {
  bf_status_t status = BF_SUCCESS;

  auto tbl_attr_impl =
      static_cast<const BfRtTableAttributesImpl *>(&tableAttributes);
  const auto attr_type = tbl_attr_impl->getAttributeType();

  std::set<TableAttributesType> attribute_type_set;
  status = tableAttributesSupported(&attribute_type_set);
  if (status != BF_SUCCESS ||
      (attribute_type_set.find(attr_type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Error : Attribute %d is not supported",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              static_cast<int>(attr_type));
    return BF_NOT_SUPPORTED;
  }

  // Make sure attribute type is valid
  if (attr_type != TableAttributesType::PRE_DEVICE_CONFIG) {
    LOG_TRACE("%s:%d %s Error in trying to reset Invalid Attribute Type %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              static_cast<int>(attr_type));
    return BF_INVALID_ARG;
  }

  // Try to get each PRE attribute separately from the attribute
  // object and set it in MC mgr if the GET operation succeeds.
  // Note:
  //    - Since application can do selective attribute set for PRE
  //      attributes, not all of the PRE attibutes would be set always,
  //      status BF_OBJECT_NOT_FOUND is returned if attribute is
  //      not set and hence it's not an error case

  // Get and set PRE global rid
  uint32_t global_rid;
  status = tbl_attr_impl->preGlobalRidGet(&global_rid);
  if (status != BF_SUCCESS && status != BF_OBJECT_NOT_FOUND) {
    LOG_TRACE(
        "%s:%d %s Error : Failed to get PRE attribute global rid value "
        "from attribute object, status = %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        status);
    return status;
  }

  auto *mcMgr = McMgrIntf::getInstance(session);
  if (status == BF_SUCCESS) {
    // Value validation for uint16_t is done during attribute object set
    status = mcMgr->mcMgrSetGlobalRid(session.preSessHandleGet(),
                                      dev_tgt.dev_id,
                                      static_cast<uint16_t>(global_rid));
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error : Failed to set PRE attribute global rid value, "
          "status = %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          status);
      return status;
    }
  }

  // Get and set PRE port protection state
  bool enable;
  status = tbl_attr_impl->prePortProtectionGet(&enable);
  if (status != BF_SUCCESS && status != BF_OBJECT_NOT_FOUND) {
    LOG_TRACE(
        "%s:%d %s Error : Failed to get PRE attribute port protection value "
        "from attribute object, status = %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        status);
    return status;
  }

  if (status == BF_SUCCESS) {
    if (enable) {
      status = mcMgr->mcMgrEnablePortProtection(session.preSessHandleGet(),
                                                dev_tgt.dev_id);
    } else {
      status = mcMgr->mcMgrDisablePortProtection(session.preSessHandleGet(),
                                                 dev_tgt.dev_id);
    }
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error : Failed to set PRE attribute port protection, "
          "status = %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          status);
      return status;
    }
  }

  // Get and set PRE fast failover state
  status = tbl_attr_impl->preFastFailoverGet(&enable);
  if (status != BF_SUCCESS && status != BF_OBJECT_NOT_FOUND) {
    LOG_TRACE(
        "%s:%d %s Error : Failed to get PRE attribute fast failover value "
        "from attribute object, status = %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        status);
    return status;
  }

  if (status == BF_SUCCESS) {
    if (enable) {
      status = mcMgr->mcMgrEnablePortFastFailover(session.preSessHandleGet(),
                                                  dev_tgt.dev_id);
    } else {
      status = mcMgr->mcMgrDisablePortFastFailover(session.preSessHandleGet(),
                                                   dev_tgt.dev_id);
    }
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error : Failed to set PRE attribute fast failover, "
          "status = %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          status);
      return status;
    }
  }

  // Get and set max nodes before yield
  uint32_t count;
  status = tbl_attr_impl->preMaxNodesBeforeYieldGet(&count);
  if (status != BF_SUCCESS && status != BF_OBJECT_NOT_FOUND) {
    LOG_TRACE(
        "%s:%d %s Error : Failed to get PRE attribute max nodes before yield "
        "value "
        "from attribute object, status = %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        status);
    return status;
  }

  if (status == BF_SUCCESS) {
    status = mcMgr->mcMgrSetMaxNodesBeforeYield(
        session.preSessHandleGet(), dev_tgt.dev_id, count);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error : Failed to set PRE attribute max nodes before yield "
          "threshold value, "
          "status = %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          status);
      return status;
    }
  }

  // Get and set PRE max node threshold values
  uint32_t node_count, node_port_lag_count;
  status =
      tbl_attr_impl->preMaxNodeThresholdGet(&node_count, &node_port_lag_count);
  if (status != BF_SUCCESS && status != BF_OBJECT_NOT_FOUND) {
    LOG_TRACE(
        "%s:%d %s Error : Failed to get PRE attribute max node threshold "
        "values "
        "from attribute object, status = %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        status);
    return status;
  }

  if (status == BF_SUCCESS) {
    status = mcMgr->mcMgrSetMaxNodeThreshold(session.preSessHandleGet(),
                                             dev_tgt.dev_id,
                                             node_count,
                                             node_port_lag_count);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error : Failed to set PRE attribute max node threshold "
          "values, "
          "status = %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          status);
      return status;
    }
  }

  return BF_SUCCESS;
}

// Node Table APIs
bf_status_t BfRtPREMulticastNodeTable::tableEntryAdd(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    const BfRtTableData &data) const {
  bf_status_t status = BF_SUCCESS;

  const BfRtPREMulticastNodeTableKey &mc_node_key =
      static_cast<const BfRtPREMulticastNodeTableKey &>(key);
  const BfRtPREMulticastNodeTableData &mc_node_data =
      static_cast<const BfRtPREMulticastNodeTableData &>(data);

  // Get the node id from key
  bf_rt_id_t mc_node_id = mc_node_key.getId();

  // Get the device state and from that get the PRE state to perform
  // various checks and update new states.
  std::lock_guard<std::mutex> lock(state_lock);
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, this->prog_name);
  if (device_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  auto pre_state = device_state->preState.getStateObj();
  if (pre_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  // Check if the node id exists already or not
  if (pre_state->statePREIdExists(
          BfRtPREStateObj::PREStateIdType::MULTICAST_NODE_ID, mc_node_id)) {
    LOG_TRACE(
        "%s:%d %s : Error in adding PRE node id %d which already "
        "exists",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        mc_node_id);
    return BF_ALREADY_EXISTS;
  }

  // Create the node in MC mgr using passed in data
  auto *mcMgr = McMgrIntf::getInstance(session);
  bf_mc_node_hdl_t node_hdl;
  status = mcMgr->mcMgrNodeCreate(
      session.preSessHandleGet(),
      dev_tgt.dev_id,
      mc_node_data.getRId(),
      const_cast<uint8_t *>(mc_node_data.getPortBitmap()),
      const_cast<uint8_t *>(mc_node_data.getLAGBitmap()),
      &node_hdl);

  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in creating node for node id %d, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              mc_node_id,
              status);
    return status;
  }

  // Now that node id is created successfully, update the node_id<--> handle
  // mapping in PRE state object
  status = pre_state->statePREIdAdd(
      BfRtPREStateObj::PREStateIdType::MULTICAST_NODE_ID, mc_node_id, node_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s Error in updating node_id<-->handle mapping for node_id "
        "%d, "
        "err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        mc_node_id,
        status);

    // Failed to update node_id<--> handle mapping in BF-RT. Delete the
    // node to have both BF-RT and MC mgr states in sync.
    bf_status_t temp_sts = mcMgr->mcMgrNodeDestroy(
        session.preSessHandleGet(), dev_tgt.dev_id, node_hdl);
    if (temp_sts != BF_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s Error in deleting node for node id %d, "
          "err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          mc_node_id,
          temp_sts);
    }

    // Return original error status
    return status;
  }

  return status;
}

bf_status_t BfRtPREMulticastNodeTable::tableEntryMod(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    const BfRtTableData &data) const {
  bf_status_t status = BF_SUCCESS;

  const BfRtPREMulticastNodeTableKey &mc_node_key =
      static_cast<const BfRtPREMulticastNodeTableKey &>(key);
  const BfRtPREMulticastNodeTableData &mc_node_data =
      static_cast<const BfRtPREMulticastNodeTableData &>(data);

  // Get the node id from key
  bf_rt_id_t mc_node_id = mc_node_key.getId();

  // Get the device state and from that get the PRE state to perform
  // various checks and get node handle.
  std::lock_guard<std::mutex> lock(state_lock);
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, this->prog_name);
  if (device_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  auto pre_state = device_state->preState.getStateObj();
  if (pre_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  // Check if the node id exists already or not
  if (pre_state->statePREIdExists(
          BfRtPREStateObj::PREStateIdType::MULTICAST_NODE_ID, mc_node_id) !=
      true) {
    LOG_TRACE(
        "%s:%d %s : Error in updating PRE node id %d which doesn't "
        "exist",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        mc_node_id);
    return BF_OBJECT_NOT_FOUND;
  }

  // First get the node handle for the given node id
  bf_mc_node_hdl_t node_hdl;
  status = pre_state->statePREHdlGet(
      BfRtPREStateObj::PREStateIdType::MULTICAST_NODE_ID,
      mc_node_id,
      &node_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error in getting PRE node handle for node id %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              mc_node_id);
    return status;
  }

  // All BF-RT validations have passed, update the node in MC mgr
  // with new data now
  auto *mcMgr = McMgrIntf::getInstance(session);
  status = mcMgr->mcMgrNodeUpdate(
      session.preSessHandleGet(),
      dev_tgt.dev_id,
      node_hdl,
      const_cast<uint8_t *>(mc_node_data.getPortBitmap()),
      const_cast<uint8_t *>(mc_node_data.getLAGBitmap()));
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in modifying node id %d, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              mc_node_id,
              status);
    return status;
  }

  return status;
}

bf_status_t BfRtPREMulticastNodeTable::tableEntryDel(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key) const {
  bf_status_t status = BF_SUCCESS;

  const BfRtPREMulticastNodeTableKey &mc_node_key =
      static_cast<const BfRtPREMulticastNodeTableKey &>(key);

  // Get the node id from key
  bf_rt_id_t mc_node_id = mc_node_key.getId();

  // Get the device state and from that get the PRE state to perform
  // various checks and update new states.
  std::lock_guard<std::mutex> lock(state_lock);
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, this->prog_name);
  if (device_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  auto pre_state = device_state->preState.getStateObj();
  if (pre_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  // Check if the node id exists already or not
  if (pre_state->statePREIdExists(
          BfRtPREStateObj::PREStateIdType::MULTICAST_NODE_ID, mc_node_id) !=
      true) {
    LOG_TRACE(
        "%s:%d %s : Error in deleting PRE node id %d which doesn't "
        "exist",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        mc_node_id);
    return BF_OBJECT_NOT_FOUND;
  }

  // Get the node handle for the given node id
  bf_mc_node_hdl_t node_hdl;
  status = pre_state->statePREHdlGet(
      BfRtPREStateObj::PREStateIdType::MULTICAST_NODE_ID,
      mc_node_id,
      &node_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error in getting PRE node handle for node id %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              mc_node_id);
    return status;
  }

  // First delete the node_id<-->handle mapping and in case
  // deleting node in MC mgr fails for any reason, the same
  // node_id<-->handle mapping can be added again to have
  // BF-RT state in sync with MC mgr state.
  status = pre_state->statePREIdDel(
      BfRtPREStateObj::PREStateIdType::MULTICAST_NODE_ID, mc_node_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error in deleting node_id<-->handle mapping for node id %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        mc_node_id);
    return status;
  }

  // All BF-RT validations have passed, delete the node in MC mgr now
  auto *mcMgr = McMgrIntf::getInstance(session);
  status = mcMgr->mcMgrNodeDestroy(
      session.preSessHandleGet(), dev_tgt.dev_id, node_hdl);

  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in deleting node id %d, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              mc_node_id,
              status);
    // Since MC mgr failed to delete the node, restore the node_id<-->handle
    // mapping in BF-RT for consistency.
    bf_status_t temp_sts = pre_state->statePREIdAdd(
        BfRtPREStateObj::PREStateIdType::MULTICAST_NODE_ID,
        mc_node_id,
        node_hdl);
    if (temp_sts != BF_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s Error in updating node_id<-->handle mapping for node id "
          "%d, "
          "err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          mc_node_id,
          temp_sts);
    }

    // Return original status
    return status;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtPREMulticastNodeTable::tableEntryGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;

  const BfRtPREMulticastNodeTableKey &mc_node_key =
      static_cast<const BfRtPREMulticastNodeTableKey &>(key);
  BfRtPREMulticastNodeTableData *mc_node_data =
      static_cast<BfRtPREMulticastNodeTableData *>(data);

  // Get the node id from key
  bf_rt_id_t mc_node_id = mc_node_key.getId();

  // Get the device state and from that get the PRE state to perform
  // various checks and get IDs from handles for MC objects.
  std::lock_guard<std::mutex> lock(state_lock);
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, this->prog_name);
  if (device_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  auto pre_state = device_state->preState.getStateObj();
  if (pre_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  // Check if the node id exists already or not
  if (pre_state->statePREIdExists(
          BfRtPREStateObj::PREStateIdType::MULTICAST_NODE_ID, mc_node_id) !=
      true) {
    LOG_TRACE(
        "%s:%d %s : Error in EntryGet for PRE node id %d which doesn't "
        "exist",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        mc_node_id);
    return BF_OBJECT_NOT_FOUND;
  }

  // Get the node handle for the given node id
  bf_mc_node_hdl_t node_hdl;
  status = pre_state->statePREHdlGet(
      BfRtPREStateObj::PREStateIdType::MULTICAST_NODE_ID,
      mc_node_id,
      &node_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error in getting PRE node handle for node id %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              mc_node_id);
    return status;
  }

  // Get the data for the given node id and return
  return this->tableEntryGet_internal(
      session, dev_tgt, *pre_state, mc_node_id, node_hdl, mc_node_data);
}

bf_status_t BfRtPREMulticastNodeTable::tableEntryGetFirst(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    BfRtTableKey *key,
    BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;

  BfRtPREMulticastNodeTableKey *node_key =
      static_cast<BfRtPREMulticastNodeTableKey *>(key);
  BfRtPREMulticastNodeTableData *node_data =
      static_cast<BfRtPREMulticastNodeTableData *>(data);

  // Get the device state and from that get the PRE state to
  // get the first node id and its handle
  std::lock_guard<std::mutex> lock(state_lock);
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, this->prog_name);
  if (device_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  auto pre_state = device_state->preState.getStateObj();
  if (pre_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  // Get the first node id and its handle from PRE State
  bf_rt_id_t mc_node_id;
  bf_mc_ecmp_hdl_t node_hdl;

  status = pre_state->statePREIdGetFirst(
      BfRtPREStateObj::PREStateIdType::MULTICAST_NODE_ID,
      &mc_node_id,
      &node_hdl);
  if (status != BF_SUCCESS && status != BF_OBJECT_NOT_FOUND) {
    LOG_TRACE("%s:%d %s: Error in getting first MC node id",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  if (status == BF_OBJECT_NOT_FOUND) {
    // This means that we don't have any L1 nodes added in the system
    LOG_DBG("%s:%d %s : No Multicast L1 nodes added in the system",
            __func__,
            __LINE__,
            table_name_get().c_str());
    return status;
  }

  // Set the node id in the key that to be returned
  node_key->setId(mc_node_id);

  // Get the data for the first node id and return the status
  return this->tableEntryGet_internal(
      session, dev_tgt, *pre_state, mc_node_id, node_hdl, node_data);
}

bf_status_t BfRtPREMulticastNodeTable::tableEntryGetNext_n(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    const uint32_t &n,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  bf_status_t status = BF_SUCCESS;

  // First get the node id from key
  const BfRtPREMulticastNodeTableKey &node_key =
      static_cast<const BfRtPREMulticastNodeTableKey &>(key);
  bf_rt_id_t mc_node_id = node_key.getId();

  // Get the device state and from that get the PRE state to
  // get the next node ids and its handle
  std::lock_guard<std::mutex> lock(state_lock);
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, this->prog_name);
  if (device_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  auto pre_state = device_state->preState.getStateObj();
  if (pre_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  // Get the next_n entries
  bf_rt_id_t next_node_id;
  bf_mc_mgrp_hdl_t next_node_hdl;
  uint32_t i;
  // For each iteration, get the next node id & its handle from PRE state and
  // then get the data for the next node id from MC mgr
  for (i = 0; i < n; i++) {
    auto this_node_key =
        static_cast<BfRtPREMulticastNodeTableKey *>((*key_data_pairs)[i].first);
    auto this_node_data = static_cast<BfRtPREMulticastNodeTableData *>(
        (*key_data_pairs)[i].second);

    // Get the next node id and it handle from PRE state
    status = pre_state->statePREIdGetNext(
        BfRtPREStateObj::PREStateIdType::MULTICAST_NODE_ID,
        mc_node_id,
        &next_node_id,
        &next_node_hdl);
    if (BF_SUCCESS != status && BF_OBJECT_NOT_FOUND != status) {
      LOG_ERROR("%s:%d %s: Error in getting next node id for curr ECMP id %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                mc_node_id);
      break;
    }
    if (BF_OBJECT_NOT_FOUND == status) {
      // This means that we have successfully read all the L1 nodes
      break;
    }

    // Set the node id in the key that to be returned
    this_node_key->setId(next_node_id);

    // Get the next L1 node's data from MC mgr
    status = this->tableEntryGet_internal(session,
                                          dev_tgt,
                                          *pre_state,
                                          next_node_id,
                                          next_node_hdl,
                                          this_node_data);
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s: Error in getting data of node id %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                next_node_id);
      break;
    }

    // Update the node id for next iteration
    mc_node_id = next_node_id;
  }

  // Update the number of entries that are read successfully
  if (num_returned) {
    *num_returned = i;
  }

  return status;
}

bf_status_t BfRtPREMulticastNodeTable::tableUsageGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    uint32_t *count) const {
  bf_status_t status = BF_SUCCESS;

  // Call the MC mgr API to get the number of L1 nodes created
  auto *mcMgr = McMgrIntf::getInstance(session);
  status = mcMgr->mcMgrNodeGetCount(
      session.preSessHandleGet(), dev_tgt.dev_id, count);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in getting the number of L1 nodes, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }

  return status;
}

bf_status_t BfRtPREMulticastNodeTable::tableEntryGet_internal(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const BfRtPREStateObj & /* pre_state */,
    const bf_rt_id_t &mc_node_id,
    const bf_mc_node_hdl_t &node_hdl,
    BfRtPREMulticastNodeTableData *mc_node_data) const {
  bf_status_t status = BF_SUCCESS;

  // Get the node attributes from MC mgr
  bf_mc_rid_t rid;
  bf_mc_port_map_t port_map;
  bf_mc_lag_map_t lag_map;
  auto *mcMgr = McMgrIntf::getInstance(session);

  status = mcMgr->mcMgrNodeGetAttr(session.preSessHandleGet(),
                                   dev_tgt.dev_id,
                                   node_hdl,
                                   &rid,
                                   port_map,
                                   lag_map);

  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s Error in getting node attributes for node index %d, "
        "err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        mc_node_id,
        status);
    return status;
  }

  mc_node_data->setRId(rid);
  mc_node_data->setPortBitmap(port_map);
  mc_node_data->setLAGBitmap(lag_map);

  return BF_SUCCESS;
}

bf_status_t BfRtPREMulticastNodeTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret =
      std::unique_ptr<BfRtTableKey>(new BfRtPREMulticastNodeTableKey(this));
  if (*key_ret == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to allocate key",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPREMulticastNodeTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret =
      std::unique_ptr<BfRtTableData>(new BfRtPREMulticastNodeTableData(this));
  if (*data_ret == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to allocate data",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPREMulticastNodeTable::keyReset(BfRtTableKey *key) const {
  BfRtPREMulticastNodeTableKey *node_key =
      static_cast<BfRtPREMulticastNodeTableKey *>(key);

  return key_reset<BfRtPREMulticastNodeTable, BfRtPREMulticastNodeTableKey>(
      *this, node_key);
}

bf_status_t BfRtPREMulticastNodeTable::dataReset(BfRtTableData *data) const {
  BfRtPREMulticastNodeTableData *node_data =
      static_cast<BfRtPREMulticastNodeTableData *>(data);

  return data_reset<BfRtPREMulticastNodeTable, BfRtPREMulticastNodeTableData>(
      *this, node_data);
}

// ECMP Table APIs
bf_status_t BfRtPREECMPTable::tableEntryAdd(const BfRtSession &session,
                                            const bf_rt_target_t &dev_tgt,
                                            const uint64_t & /*flags*/,
                                            const BfRtTableKey &key,
                                            const BfRtTableData &data) const {
  bf_status_t status = BF_SUCCESS;

  const BfRtPREECMPTableKey &ecmp_key =
      static_cast<const BfRtPREECMPTableKey &>(key);
  const BfRtPREECMPTableData &ecmp_data =
      static_cast<const BfRtPREECMPTableData &>(data);

  // Get the node id from key
  bf_rt_id_t mc_ecmp_id = ecmp_key.getId();

  // Get the device state and from that get the PRE state to perform
  // various checks and update new states.
  std::lock_guard<std::mutex> lock(state_lock);
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, this->prog_name);
  if (device_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  auto pre_state = device_state->preState.getStateObj();
  if (pre_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  // Check if the node id exists already or not
  if (pre_state->statePREIdExists(
          BfRtPREStateObj::PREStateIdType::MULTICAST_ECMP_ID, mc_ecmp_id)) {
    LOG_TRACE(
        "%s:%d %s : Error in adding PRE ECMP id %d which already "
        "exists",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        mc_ecmp_id);
    return BF_ALREADY_EXISTS;
  }

  // Create the ECMP in MC mgr using passed in data
  auto *mcMgr = McMgrIntf::getInstance(session);
  bf_mc_ecmp_hdl_t ecmp_hdl;
  status = mcMgr->mcMgrEcmpCreate(
      session.preSessHandleGet(), dev_tgt.dev_id, &ecmp_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in creating ecmp for index %d, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              mc_ecmp_id,
              status);
    return status;
  }

  // Get the list of nodes from data and associate with the ECMP id
  const std::vector<bf_rt_id_t> &multicast_node_ids =
      ecmp_data.getMulticastNodeIds();

  status = this->mcECMPNodeMbrsAdd(
      session, dev_tgt, *pre_state, mc_ecmp_id, ecmp_hdl, multicast_node_ids);
  if (status != BF_SUCCESS) {
    // If any error is encountered while associating L1 nodes to the
    // ECMP node, delete the ECMP node and return
    // original error status.
    bf_status_t temp_sts = mcMgr->mcMgrEcmpDestroy(
        session.preSessHandleGet(), dev_tgt.dev_id, ecmp_hdl);
    if (temp_sts != BF_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s Error in deleting newly created ECMP node for ECMP %d, "
          "err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          mc_ecmp_id,
          temp_sts);
    }
    // Return original error status
    return status;
  }

  // Now that ECMP id is created successfully, update the ecmp_id<--> handle
  // mapping in PRE state object
  status = pre_state->statePREIdAdd(
      BfRtPREStateObj::PREStateIdType::MULTICAST_ECMP_ID, mc_ecmp_id, ecmp_hdl);
  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s Error in updating ecmp_id<-->handle mapping for ECMP %d, err "
        "%d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        mc_ecmp_id,
        status);

    // Failed to update ecmp_id<--> handle mapping in BF-RT. Delete the
    // ECMP node to have both BF-RT and MC mgr states in sync.
    bf_status_t temp_sts = mcMgr->mcMgrEcmpDestroy(
        session.preSessHandleGet(), dev_tgt.dev_id, ecmp_hdl);
    if (temp_sts != BF_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s Error in deleting newly created ECMP node for ECMP %d, "
          "err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          mc_ecmp_id,
          temp_sts);
    }
    // Return original error status
    return status;
  }

  return status;
}

bf_status_t BfRtPREECMPTable::tableEntryMod(const BfRtSession &session,
                                            const bf_rt_target_t &dev_tgt,
                                            const uint64_t & /*flags*/,
                                            const BfRtTableKey &key,
                                            const BfRtTableData &data) const {
  bf_status_t status = BF_SUCCESS;
  const BfRtPREECMPTableKey &ecmp_key =
      static_cast<const BfRtPREECMPTableKey &>(key);
  const BfRtPREECMPTableData &ecmp_data =
      static_cast<const BfRtPREECMPTableData &>(data);

  // Get the node id from key
  bf_rt_id_t mc_ecmp_id = ecmp_key.getId();
  // Get the list of nodes from data
  const std::vector<bf_rt_id_t> &multicast_node_ids =
      ecmp_data.getMulticastNodeIds();

  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, this->prog_name);
  if (device_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }
  auto pre_state = device_state->preState.getStateObj();
  if (pre_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }
  // Get the device state and from that get the PRE state to perform
  // various checks and update new states.
  std::lock_guard<std::mutex> lock(state_lock);
  // Check if the ecmp id exists already or not
  if (pre_state->statePREIdExists(
          BfRtPREStateObj::PREStateIdType::MULTICAST_ECMP_ID, mc_ecmp_id) !=
      true) {
    LOG_TRACE(
        "%s:%d %s : Error in modifying PRE ECMP id %d which doesn't exist",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        mc_ecmp_id);
    return BF_OBJECT_NOT_FOUND;
  }

  // Check if the node id exists already or not
  for (auto mc_node_id : multicast_node_ids) {
    if (pre_state->statePREIdExists(
            BfRtPREStateObj::PREStateIdType::MULTICAST_NODE_ID, mc_node_id) !=
        true) {
      LOG_TRACE(
          "%s:%d %s : Error in updating PRE ecmp with invalid node id %d "
          "which "
          "doesn't "
          "exist",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          mc_node_id);
      return BF_OBJECT_NOT_FOUND;
    }
  }

  // Get the ecmp handle for the MGID
  bf_mc_mgrp_hdl_t ecmp_hdl;
  status = pre_state->statePREHdlGet(
      BfRtPREStateObj::PREStateIdType::MULTICAST_ECMP_ID,
      mc_ecmp_id,
      &ecmp_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error in getting PRE ECMP handle for ECMP id %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              mc_ecmp_id);
    return status;
  }
  std::vector<bf_mc_node_hdl_t> node_hdls(multicast_node_ids.size(), 0);
  for (uint32_t i = 0; i < multicast_node_ids.size(); i++) {
    bf_mc_node_hdl_t node_hdl;
    status = pre_state->statePREHdlGet(
        BfRtPREStateObj::PREStateIdType::MULTICAST_NODE_ID,
        multicast_node_ids[i],
        &node_hdl);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error in getting node handle for node id %d while "
          "adding it to ECMP %d, err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          multicast_node_ids[i],
          mc_ecmp_id,
          status);
      return status;
    }
    node_hdls[i] = node_hdl;
  }
  auto *mcMgr = McMgrIntf::getInstance(session);
  status = mcMgr->mcMgrEcmpMbrMod(session.preSessHandleGet(),
                                  dev_tgt.dev_id,
                                  ecmp_hdl,
                                  node_hdls.data(),
                                  node_hdls.size());

  return status;
}

bf_status_t BfRtPREECMPTable::tableEntryModInc(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    const BfRtTableData &data) const {
  bf_status_t status = BF_SUCCESS;

  const BfRtPREECMPTableKey &ecmp_key =
      static_cast<const BfRtPREECMPTableKey &>(key);
  const BfRtPREECMPTableData &ecmp_data =
      static_cast<const BfRtPREECMPTableData &>(data);

  // Get the ecmp id from key
  bf_rt_id_t mc_ecmp_id = ecmp_key.getId();

  // Get the device state and from that get the PRE state to perform
  // various checks and update new states.
  std::lock_guard<std::mutex> lock(state_lock);
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, this->prog_name);
  if (device_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }
  auto pre_state = device_state->preState.getStateObj();
  if (pre_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  // Check if the node id exists already or not
  if (pre_state->statePREIdExists(
          BfRtPREStateObj::PREStateIdType::MULTICAST_ECMP_ID, mc_ecmp_id) !=
      true) {
    LOG_TRACE(
        "%s:%d %s : Error in modifying PRE ECMP id %d incrementally "
        "which doesn't exist",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        mc_ecmp_id);
    return BF_ALREADY_EXISTS;
  }

  // Get the ecmp handle for the MGID
  bf_mc_mgrp_hdl_t ecmp_hdl;
  status = pre_state->statePREHdlGet(
      BfRtPREStateObj::PREStateIdType::MULTICAST_ECMP_ID,
      mc_ecmp_id,
      &ecmp_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error in getting PRE ECMP handle for ECMP id %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              mc_ecmp_id);
    return status;
  }

  // Get the list of nodes from data and associate or dissociate with
  // the ECMP id
  const std::vector<bf_rt_id_t> &multicast_node_ids =
      ecmp_data.getMulticastNodeIds();

  if (BF_RT_FLAG_IS_SET(flags, BF_RT_INC_DEL)) {
    status = this->mcECMPNodeMbrsDel(
        session, dev_tgt, *pre_state, mc_ecmp_id, ecmp_hdl, multicast_node_ids);
  } else {
    status = this->mcECMPNodeMbrsAdd(
        session, dev_tgt, *pre_state, mc_ecmp_id, ecmp_hdl, multicast_node_ids);
  }

  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in %s node members with ECMP id %d, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              BF_RT_FLAG_IS_SET(flags, BF_RT_INC_DEL) ? "dissociating"
                                                      : "associating",
              mc_ecmp_id,
              status);
    return status;
  }

  return status;
}

bf_status_t BfRtPREECMPTable::tableEntryDel(const BfRtSession &session,
                                            const bf_rt_target_t &dev_tgt,
                                            const uint64_t & /*flags*/,
                                            const BfRtTableKey &key) const {
  bf_status_t status = BF_SUCCESS;

  const BfRtPREECMPTableKey &ecmp_key =
      static_cast<const BfRtPREECMPTableKey &>(key);

  // Get the ECMP id from key
  bf_rt_id_t mc_ecmp_id = ecmp_key.getId();

  // Get the device state and from that get the PRE state to perform
  // various checks and update new states.
  std::lock_guard<std::mutex> lock(state_lock);
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, this->prog_name);
  if (device_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  auto pre_state = device_state->preState.getStateObj();
  if (pre_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  // Check if the ECMP id exists already or not
  if (pre_state->statePREIdExists(
          BfRtPREStateObj::PREStateIdType::MULTICAST_ECMP_ID, mc_ecmp_id) !=
      true) {
    LOG_TRACE(
        "%s:%d %s : Error in deleting PRE ECMP id %d which doesn't "
        "exist",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        mc_ecmp_id);
    return BF_OBJECT_NOT_FOUND;
  }

  // Get the ecmp handle for the MGID
  bf_mc_mgrp_hdl_t ecmp_hdl;
  status = pre_state->statePREHdlGet(
      BfRtPREStateObj::PREStateIdType::MULTICAST_ECMP_ID,
      mc_ecmp_id,
      &ecmp_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error in getting PRE ECMP handle for ECMP id %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              mc_ecmp_id);
    return status;
  }

  // First delete the ecmp_id<-->handle mapping and in case
  // deleting ECMP node in MC mgr fails for any reason, the same
  // ecmp_id<-->handle mapping can be added again to have
  // BF-RT state in sync with MC mgr state.
  status = pre_state->statePREIdDel(
      BfRtPREStateObj::PREStateIdType::MULTICAST_ECMP_ID, mc_ecmp_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error in deleting ecmp_id<-->handle mapping for ECMP id %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        mc_ecmp_id);
    return status;
  }

  // All BF-RT validations have passed, delete ECMP id in MC mgr now
  auto *mcMgr = McMgrIntf::getInstance(session);
  status = mcMgr->mcMgrEcmpDestroy(
      session.preSessHandleGet(), dev_tgt.dev_id, ecmp_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s Error in deleting MC group for mgid %d, "
        "err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        mc_ecmp_id,
        status);

    // Since MC mgr failed to delete the ECMP id, restore the ecmp_id<-->handle
    // mapping in BF-RT for consistency.
    bf_status_t temp_sts = pre_state->statePREIdAdd(
        BfRtPREStateObj::PREStateIdType::MULTICAST_MGID, mc_ecmp_id, ecmp_hdl);
    if (temp_sts != BF_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s Error in updating ecmp_id<-->handle mapping for ECMP id "
          "%d, "
          "err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          mc_ecmp_id,
          temp_sts);
    }

    // Return original status
    return status;
  }

  return status;
}

bf_status_t BfRtPREECMPTable::tableEntryGet(const BfRtSession &session,
                                            const bf_rt_target_t &dev_tgt,
                                            const uint64_t & /*flags*/,
                                            const BfRtTableKey &key,
                                            BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;

  const BfRtPREECMPTableKey &ecmp_key =
      static_cast<const BfRtPREECMPTableKey &>(key);
  BfRtPREECMPTableData *ecmp_data = static_cast<BfRtPREECMPTableData *>(data);

  // Get the ECMP id from key
  bf_rt_id_t mc_ecmp_id = ecmp_key.getId();

  // Get the device state and from that get the PRE state to perform
  // various checks and get IDs from handles for MC objects.
  std::lock_guard<std::mutex> lock(state_lock);
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, this->prog_name);
  if (device_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  auto pre_state = device_state->preState.getStateObj();
  if (pre_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  // Check if the ECMP id exists already or not
  if (pre_state->statePREIdExists(
          BfRtPREStateObj::PREStateIdType::MULTICAST_ECMP_ID, mc_ecmp_id) !=
      true) {
    LOG_TRACE(
        "%s:%d %s : Error in EntryGet for PRE ECMP id %d which doesn't "
        "exist",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        mc_ecmp_id);
    return BF_OBJECT_NOT_FOUND;
  }

  // Get the ecmp handle for the MGID
  bf_mc_mgrp_hdl_t ecmp_hdl;
  status = pre_state->statePREHdlGet(
      BfRtPREStateObj::PREStateIdType::MULTICAST_ECMP_ID,
      mc_ecmp_id,
      &ecmp_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error in getting PRE ECMP handle for ECMP id %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              mc_ecmp_id);
    return status;
  }

  // Get the data for the given ECMP and return
  return this->tableEntryGet_internal(
      session, dev_tgt, *pre_state, mc_ecmp_id, ecmp_hdl, ecmp_data);
}

bf_status_t BfRtPREECMPTable::tableEntryGetFirst(const BfRtSession &session,
                                                 const bf_rt_target_t &dev_tgt,
                                                 const uint64_t & /*flags*/,
                                                 BfRtTableKey *key,
                                                 BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;

  BfRtPREECMPTableKey *ecmp_key = static_cast<BfRtPREECMPTableKey *>(key);
  BfRtPREECMPTableData *ecmp_data = static_cast<BfRtPREECMPTableData *>(data);

  // Get the device state and from that get the PRE state to
  // get the first ECMP id and its handle
  std::lock_guard<std::mutex> lock(state_lock);
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, this->prog_name);
  if (device_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  auto pre_state = device_state->preState.getStateObj();
  if (pre_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  // Get the first ECMP id and its handle from PRE State
  bf_rt_id_t mc_ecmp_id;
  bf_mc_ecmp_hdl_t ecmp_hdl;

  status = pre_state->statePREIdGetFirst(
      BfRtPREStateObj::PREStateIdType::MULTICAST_ECMP_ID,
      &mc_ecmp_id,
      &ecmp_hdl);
  if (status != BF_SUCCESS && status != BF_OBJECT_NOT_FOUND) {
    LOG_TRACE("%s:%d %s: Error in getting first ECMP id",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  if (status == BF_OBJECT_NOT_FOUND) {
    // This means that we don't have any ECMPs added in the system
    LOG_DBG("%s:%d %s : No Multicast ECMPs added in the system",
            __func__,
            __LINE__,
            table_name_get().c_str());
    return status;
  }

  // Set the ECMP id in the key that to be returned
  ecmp_key->setId(mc_ecmp_id);

  // Get the data for the first ECMP id and return the status
  return this->tableEntryGet_internal(
      session, dev_tgt, *pre_state, mc_ecmp_id, ecmp_hdl, ecmp_data);
}

bf_status_t BfRtPREECMPTable::tableEntryGetNext_n(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    const uint32_t &n,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  bf_status_t status = BF_SUCCESS;

  // First get the ECMP id from key
  const BfRtPREECMPTableKey &ecmp_key =
      static_cast<const BfRtPREECMPTableKey &>(key);
  bf_rt_id_t mc_ecmp_id = ecmp_key.getId();

  // Get the device state and from that get the PRE state to
  // get the next ECMP ids and its handle
  std::lock_guard<std::mutex> lock(state_lock);
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, this->prog_name);
  if (device_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  auto pre_state = device_state->preState.getStateObj();
  if (pre_state == nullptr) {
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  // Get the next_n entries
  bf_rt_id_t next_ecmp_id;
  bf_mc_mgrp_hdl_t next_ecmp_hdl;
  uint32_t i;
  // For each iteration, get the next ECMP id & its handle from PRE state and
  // then get the data for the next ECMP id from MC mgr
  for (i = 0; i < n; i++) {
    auto this_ecmp_key =
        static_cast<BfRtPREECMPTableKey *>((*key_data_pairs)[i].first);
    auto this_ecmp_data =
        static_cast<BfRtPREECMPTableData *>((*key_data_pairs)[i].second);

    // Get the next ECMP id and it handle from PRE state
    status = pre_state->statePREIdGetNext(
        BfRtPREStateObj::PREStateIdType::MULTICAST_ECMP_ID,
        mc_ecmp_id,
        &next_ecmp_id,
        &next_ecmp_hdl);
    if (BF_SUCCESS != status && BF_OBJECT_NOT_FOUND != status) {
      LOG_ERROR("%s:%d %s: Error in getting next ECMP id for curr ECMP id %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                mc_ecmp_id);
      break;
    }
    if (BF_OBJECT_NOT_FOUND == status) {
      // This means that we have successfully read all the ECMPs
      break;
    }

    // Set the ECMP id in the key that to be returned
    this_ecmp_key->setId(next_ecmp_id);

    // Get the next ECMP's data from MC mgr
    status = this->tableEntryGet_internal(session,
                                          dev_tgt,
                                          *pre_state,
                                          next_ecmp_id,
                                          next_ecmp_hdl,
                                          this_ecmp_data);
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s: Error in getting data of ECMP id %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                next_ecmp_id);
      break;
    }

    // Update the ECMP id for next iteration
    mc_ecmp_id = next_ecmp_id;
  }

  // Update the number of entries that are read successfully
  if (num_returned) {
    *num_returned = i;
  }

  return status;
}

bf_status_t BfRtPREECMPTable::tableUsageGet(const BfRtSession &session,
                                            const bf_rt_target_t &dev_tgt,
                                            const uint64_t & /*flags*/,
                                            uint32_t *count) const {
  bf_status_t status = BF_SUCCESS;

  // Call the MC mgr API to get the number of ECMPs created
  auto *mcMgr = McMgrIntf::getInstance(session);
  status = mcMgr->mcMgrEcmpGetCount(
      session.preSessHandleGet(), dev_tgt.dev_id, count);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in getting the number of ECMPs, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }

  return status;
}

bf_status_t BfRtPREECMPTable::tableEntryGet_internal(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const BfRtPREStateObj &pre_state,
    const bf_rt_id_t &mc_ecmp_id,
    const bf_mc_mgrp_hdl_t &ecmp_hdl,
    BfRtPREECMPTableData *ecmp_data) const {
  bf_status_t status = BF_SUCCESS;

  // Vector to hold the node members
  std::vector<bf_rt_id_t> multicast_node_ids;

  // Get the first node member of the ECMP
  bf_mc_node_hdl_t node_hdl;
  auto *mcMgr = McMgrIntf::getInstance(session);
  status = mcMgr->mcMgrEcmpGetFirstMbr(
      session.preSessHandleGet(), dev_tgt.dev_id, ecmp_hdl, &node_hdl);

  if (status == BF_OBJECT_NOT_FOUND) {
    // This is not an error case. ECMP table created with
    // no node members is valid. Just return SUCCESS.
    LOG_DBG("%s:%d %s No node members for ECMP index %d",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            mc_ecmp_id);
    return BF_SUCCESS;
  }

  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s Error in getting first node member for ECMP "
        "index %d, err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        mc_ecmp_id,
        status);
    return status;
  }

  // Get the node id for the returned node handle
  bf_rt_id_t mc_node_id;
  status = pre_state.statePREIdGet(
      BfRtPREStateObj::PREStateIdType::MULTICAST_NODE_ID,
      node_hdl,
      &mc_node_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error in getting PRE node id for node handle %#x",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              node_hdl);
    return status;
  }

  // Add the node_id to the vector
  multicast_node_ids.push_back(mc_node_id);

  // Get the total number of nodes in the ECMP
  uint32_t node_count;
  status = mcMgr->mcMgrEcmpGetMbrCount(
      session.preSessHandleGet(), dev_tgt.dev_id, ecmp_hdl, &node_count);
  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s Error in getting total count of node members in ECMP "
        "for id %d, err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        mc_ecmp_id,
        status);
  }

  uint32_t i = node_count - 1;  // Exclude first node member
  if (i == 0) {
    // No other node members exist in the ECMP, return the first member
    ecmp_data->setMulticastNodeIds(multicast_node_ids);
    return status;
  }

  // Get the remaining node members
  // For this, allocate vectors for 'i' members and pass the vector memory
  // to MC mgr to return the remaining node members
  std::vector<bf_mc_node_hdl_t> node_hdls(i, 0);
  bf_mc_node_hdl_t *node_hdls_ptr = node_hdls.data();

  status = mcMgr->mcMgrEcmpGetNextIMbr(session.preSessHandleGet(),
                                       dev_tgt.dev_id,
                                       ecmp_hdl,
                                       node_hdl,
                                       i,
                                       node_hdls_ptr);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s Error in getting next i node members in ECMP id %d, "
        "err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        mc_ecmp_id,
        status);
    return status;
  }

  for (uint32_t index = 0; index < i; index++) {
    if (node_hdls_ptr[index] == (~0U)) {
      // If number of nodes is less than i for
      // any valid reason, MC mgr sets the remaining entries
      // to -1. We need to stop processing if we hit the first
      // such node handle.
      break;
    }

    // Get the node id from the node handle
    status = pre_state.statePREIdGet(
        BfRtPREStateObj::PREStateIdType::MULTICAST_NODE_ID,
        node_hdls_ptr[index],
        &mc_node_id);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s : Error in getting PRE node id for node handle %#x, "
          "index %u",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          node_hdls_ptr[index],
          index);
      return BF_UNEXPECTED;
    }

    // Add the node_id to the vector
    multicast_node_ids.push_back(mc_node_id);
  }

  // Update the data fields with members returned by MC mgr
  // TODO: For better performance, try to avoid this copying
  // like how it's done for EntryAdd and EntryMod
  ecmp_data->setMulticastNodeIds(multicast_node_ids);

  return status;
}

bf_status_t BfRtPREECMPTable::mcECMPNodeMbrsAdd(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const BfRtPREStateObj &pre_state,
    const bf_rt_id_t &mc_ecmp_id,
    const bf_mc_ecmp_hdl_t &ecmp_hdl,
    const std::vector<bf_rt_id_t> &multicast_node_ids) const {
  bf_status_t status = BF_SUCCESS;

  // Add all given multicast nodes to the given ECMP
  auto *mcMgr = McMgrIntf::getInstance(session);
  bf_mc_node_hdl_t node_hdl;
  for (uint32_t i = 0; i < multicast_node_ids.size(); i++) {
    status = pre_state.statePREHdlGet(
        BfRtPREStateObj::PREStateIdType::MULTICAST_NODE_ID,
        multicast_node_ids[i],
        &node_hdl);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error in getting node handle for node id %d while "
          "adding it to ECMP %d, err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          multicast_node_ids[i],
          mc_ecmp_id,
          status);

      return status;
    }

    status = mcMgr->mcMgrEcmpMbrAdd(
        session.preSessHandleGet(), dev_tgt.dev_id, ecmp_hdl, node_hdl);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error in adding node id %d with ECMP index %d, err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          multicast_node_ids[i],
          mc_ecmp_id,
          status);

      return status;
    }
  }

  return status;
}

bf_status_t BfRtPREECMPTable::mcECMPNodeMbrsDel(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const BfRtPREStateObj &pre_state,
    const bf_rt_id_t &mc_ecmp_id,
    const bf_mc_ecmp_hdl_t &ecmp_hdl,
    const std::vector<bf_rt_id_t> &multicast_node_ids) const {
  bf_status_t status = BF_SUCCESS;

  // Delete all given multicast nodes from the given ECMP
  auto *mcMgr = McMgrIntf::getInstance(session);
  bf_mc_node_hdl_t node_hdl;
  for (uint32_t i = 0; i < multicast_node_ids.size(); i++) {
    status = pre_state.statePREHdlGet(
        BfRtPREStateObj::PREStateIdType::MULTICAST_NODE_ID,
        multicast_node_ids[i],
        &node_hdl);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error in getting node handle for node id %d to delete "
          "it from ECMP %d, err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          multicast_node_ids[i],
          mc_ecmp_id,
          status);

      return status;
    }

    status = mcMgr->mcMgrEcmpMbrRem(
        session.preSessHandleGet(), dev_tgt.dev_id, ecmp_hdl, node_hdl);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error in deleting node id %d from ECMP index %d, err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          multicast_node_ids[i],
          mc_ecmp_id,
          status);

      return status;
    }
  }

  return status;
}

bf_status_t BfRtPREECMPTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtPREECMPTableKey(this));
  if (*key_ret == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to allocate key",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPREECMPTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret = std::unique_ptr<BfRtTableData>(new BfRtPREECMPTableData(this));
  if (*data_ret == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to allocate data",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPREECMPTable::keyReset(BfRtTableKey *key) const {
  BfRtPREECMPTableKey *ecmp_key = static_cast<BfRtPREECMPTableKey *>(key);

  return key_reset<BfRtPREECMPTable, BfRtPREECMPTableKey>(*this, ecmp_key);
}

bf_status_t BfRtPREECMPTable::dataReset(BfRtTableData *data) const {
  BfRtPREECMPTableData *ecmp_data = static_cast<BfRtPREECMPTableData *>(data);

  return data_reset<BfRtPREECMPTable, BfRtPREECMPTableData>(*this, ecmp_data);
}

// LAG Table APIs
bf_status_t BfRtPRELAGTable::tableEntryAdd(const BfRtSession &session,
                                           const bf_rt_target_t &dev_tgt,
                                           const uint64_t & /*flags*/,
                                           const BfRtTableKey &key,
                                           const BfRtTableData &data) const {
  bf_status_t status = BF_SUCCESS;

  const BfRtPRELAGTableKey &lag_key =
      static_cast<const BfRtPRELAGTableKey &>(key);
  const BfRtPRELAGTableData &lag_data =
      static_cast<const BfRtPRELAGTableData &>(data);

  // Get the LAG id from key
  bf_mc_lag_id_t lag_id = lag_key.getId();

  // Update the LAG membership in MC mgr with the new data
  auto *mcMgr = McMgrIntf::getInstance(session);
  status = mcMgr->mcMgrSetLagMembership(
      session.preSessHandleGet(),
      dev_tgt.dev_id,
      lag_id,
      const_cast<uint8_t *>(lag_data.getPortBitmap()));

  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in adding/modifying lag index %d, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              lag_id,
              status);
    return status;
  }

  // Get the Remote Count Config from lag_data
  uint64_t msb_count = 0;
  uint64_t lsb_count = 0;
  bf_rt_id_t field_id;
  status = this->dataFieldIdGet("$MULTICAST_LAG_REMOTE_MSB_COUNT", &field_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s : Error : Failed to get the field id for "
        "$MULTICAST_LAG_REMOTE_MSB_COUNT",
        __func__,
        __LINE__,
        table_name_get().c_str());
    return status;
  }
  status = lag_data.getValue(field_id, &msb_count);
  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s : Error : Failed to set the data field "
        "$MULTICAST_LAG_REMOTE_MSB_COUNT",
        __func__,
        __LINE__,
        table_name_get().c_str());
    return status;
  }

  // Update the data with remote lsb count config
  status = this->dataFieldIdGet("$MULTICAST_LAG_REMOTE_LSB_COUNT", &field_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s : Error : Failed to get the field id for "
        "$MULTICAST_LAG_REMOTE_LSB_COUNT",
        __func__,
        __LINE__,
        table_name_get().c_str());
    return status;
  }
  status = lag_data.getValue(field_id, &lsb_count);
  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s : Error : Failed to set the data field "
        "$MULTICAST_LAG_REMOTE_LSB_COUNT",
        __func__,
        __LINE__,
        table_name_get().c_str());
    return status;
  }

  status = mcMgr->mcMgrSetLagRemoteCountConfig(session.preSessHandleGet(),
                                               dev_tgt.dev_id,
                                               lag_id,
                                               static_cast<int>(msb_count),
                                               static_cast<int>(lsb_count));

  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s Error in getting lag index %d with remote count, err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        lag_id,
        status);
    return status;
  }

  return status;
}

bf_status_t BfRtPRELAGTable::tableEntryMod(const BfRtSession &session,
                                           const bf_rt_target_t &dev_tgt,
                                           const uint64_t & /*flag*/,
                                           const BfRtTableKey &key,
                                           const BfRtTableData &data) const {
  bf_status_t status = BF_SUCCESS;

  const BfRtPRELAGTableKey &lag_key =
      static_cast<const BfRtPRELAGTableKey &>(key);
  const BfRtPRELAGTableData &lag_data =
      static_cast<const BfRtPRELAGTableData &>(data);

  // Get the LAG id from key
  bf_mc_lag_id_t lag_id = lag_key.getId();

  // only mod neccessary fields
  auto *mcMgr = McMgrIntf::getInstance(session);
  bf_rt_id_t field_id;
  status = this->dataFieldIdGet("$DEV_PORT", &field_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s : Error : Failed to get the field id for "
        "$MULTICAST_LAG_REMOTE_MSB_COUNT",
        __func__,
        __LINE__,
        table_name_get().c_str());
    return status;
  }
  if (lag_data.checkFieldActive(field_id)) {
    status = mcMgr->mcMgrSetLagMembership(
        session.preSessHandleGet(),
        dev_tgt.dev_id,
        lag_id,
        const_cast<uint8_t *>(lag_data.getPortBitmap()));
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s Error in adding/modifying lag index %d, err %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                lag_id,
                status);
      return status;
    }
  }

  // Get the Remote Count Config from MC mgr
  int msb_count = -1;
  int lsb_count = -1;
  status = this->dataFieldIdGet("$MULTICAST_LAG_REMOTE_MSB_COUNT", &field_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s : Error : Failed to get the field id for "
        "$MULTICAST_LAG_REMOTE_MSB_COUNT",
        __func__,
        __LINE__,
        table_name_get().c_str());
    return status;
  }
  if (lag_data.checkFieldActive(field_id)) {
    uint64_t tmp;
    lag_data.getValue(field_id, &tmp);
    msb_count = static_cast<int>(tmp);
  }

  status = this->dataFieldIdGet("$MULTICAST_LAG_REMOTE_LSB_COUNT", &field_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s : Error : Failed to get the field id for "
        "$MULTICAST_LAG_REMOTE_MSB_COUNT",
        __func__,
        __LINE__,
        table_name_get().c_str());
    return status;
  }
  if (lag_data.checkFieldActive(field_id)) {
    uint64_t tmp;
    lag_data.getValue(field_id, &tmp);
    lsb_count = static_cast<int>(tmp);
  }

  if (msb_count == -1 || lsb_count == -1) {
    int tmp_msb = 0;
    int tmp_lsb = 0;
    status = mcMgr->mcMgrGetLagRemoteCountConfig(
        session.preSessHandleGet(), dev_tgt.dev_id, lag_id, &tmp_msb, &tmp_lsb);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error in getting lag index %d with remote count, err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          lag_id,
          status);
      return status;
    }
    if (msb_count == -1) msb_count = tmp_msb;
    if (lsb_count == -1) lsb_count = tmp_lsb;
  }

  status = mcMgr->mcMgrSetLagRemoteCountConfig(
      session.preSessHandleGet(), dev_tgt.dev_id, lag_id, msb_count, lsb_count);

  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s Error in getting lag index %d with remote count, err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        lag_id,
        status);
    return status;
  }

  return status;
}

bf_status_t BfRtPRELAGTable::tableEntryGet(const BfRtSession &session,
                                           const bf_rt_target_t &dev_tgt,
                                           const uint64_t &flags,
                                           const BfRtTableKey &key,
                                           BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;

  const BfRtPRELAGTableKey &lag_key =
      static_cast<const BfRtPRELAGTableKey &>(key);
  BfRtPRELAGTableData *lag_data = static_cast<BfRtPRELAGTableData *>(data);

  // Get the LAG id from key
  bf_mc_lag_id_t lag_id = lag_key.getId();

  // Get the port bitmap from MC mgr
  auto *mcMgr = McMgrIntf::getInstance(session);
  bool from_hw = BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW) ? true : false;

  // Get the Remote Count Config from MC mgr
  int msb_count = 0;
  int lsb_count = 0;
  bool got_remote_count = false;
  // port_map
  bf_mc_port_map_t port_map;

  // Update the data with devport
  bf_rt_id_t field_id;
  status = this->dataFieldIdGet("$DEV_PORT", &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for "
        "$DEV_PORT",
        __func__,
        __LINE__,
        table_name_get().c_str());
    return status;
  }
  if (lag_data->checkFieldActive(field_id)) {
    status = mcMgr->mcMgrGetLagMembership(
        session.preSessHandleGet(), dev_tgt.dev_id, lag_id, port_map, from_hw);

    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error in reading lag table entry for lag id %d, "
          "err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          lag_id,
          status);
      return status;
    }

    // Update the data with port bitmap to be returned
    lag_data->setPortBitmap(port_map);
  }
  status = this->dataFieldIdGet("$MULTICAST_LAG_REMOTE_MSB_COUNT", &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for "
        "$MULTICAST_LAG_REMOTE_MSB_COUNT",
        __func__,
        __LINE__,
        table_name_get().c_str());
    return status;
  }
  if (lag_data->checkFieldActive(field_id)) {
    status = mcMgr->mcMgrGetLagRemoteCountConfig(session.preSessHandleGet(),
                                                 dev_tgt.dev_id,
                                                 lag_id,
                                                 &msb_count,
                                                 &lsb_count);

    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error in getting lag index %d with remote count, err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          lag_id,
          status);
      return status;
    }
    got_remote_count = true;
    status = lag_data->setValue(field_id, static_cast<uint64_t>(msb_count));
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s : Error : Failed to set the data field "
          "$MULTICAST_LAG_REMOTE_MSB_COUNT",
          __func__,
          __LINE__,
          table_name_get().c_str());
      return status;
    }
  }

  // Update the data with remote lsb count config
  status = this->dataFieldIdGet("$MULTICAST_LAG_REMOTE_LSB_COUNT", &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for "
        "$MULTICAST_LAG_REMOTE_LSB_COUNT",
        __func__,
        __LINE__,
        table_name_get().c_str());
    return status;
  }
  if (lag_data->checkFieldActive(field_id)) {
    if (!got_remote_count) {
      status = mcMgr->mcMgrGetLagRemoteCountConfig(session.preSessHandleGet(),
                                                   dev_tgt.dev_id,
                                                   lag_id,
                                                   &msb_count,
                                                   &lsb_count);

      if (status != BF_SUCCESS) {
        LOG_TRACE(
            "%s:%d %s Error in getting lag index %d with remote count, err %d",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            lag_id,
            status);
        return status;
      }
    }
    status = lag_data->setValue(field_id, static_cast<uint64_t>(lsb_count));
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s : Error : Failed to set the data field "
          "$MULTICAST_LAG_REMOTE_LSB_COUNT",
          __func__,
          __LINE__,
          table_name_get().c_str());
      return status;
    }
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPRELAGTable::tableEntryGetFirst(const BfRtSession &session,
                                                const bf_rt_target_t &dev_tgt,
                                                const uint64_t &flags,
                                                BfRtTableKey *key,
                                                BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;
  BfRtPRELAGTableKey *lag_key = static_cast<BfRtPRELAGTableKey *>(key);
  bf_mc_lag_id_t lag_id = 0;
  // Get the port bitmap from MC mgr
  bf_mc_port_map_t port_map;
  auto *mcMgr = McMgrIntf::getInstance(session);
  bool from_hw = BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW) ? true : false;
  status = mcMgr->mcMgrGetLagMembership(
      session.preSessHandleGet(), dev_tgt.dev_id, lag_id, port_map, from_hw);
  if (status != BF_SUCCESS) {
    LOG_TRACE("4b97cd7c : %s %s Error in reading lag table entry for lag id %d",
              __func__,
              table_name_get().c_str(),
              lag_id);
    return status;
  }
  // Update the data with port bitmap to be returned
  lag_key->setId(lag_id);
  return this->tableEntryGet(session, dev_tgt, flags, *key, data);
}

bf_status_t BfRtPRELAGTable::tableEntryGetNext_n(const BfRtSession &session,
                                                 const bf_rt_target_t &dev_tgt,
                                                 const uint64_t &flags,
                                                 const BfRtTableKey &key,
                                                 const uint32_t &n,
                                                 keyDataPairs *key_data_pairs,
                                                 uint32_t *num_returned) const {
  bf_status_t status = BF_SUCCESS;
  if (!num_returned) {
    LOG_TRACE(
        "a4856492 : %s:%d %s: Invalid Argument : num_returned cannot be NULL",
        __func__,
        __LINE__,
        table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  const BfRtPRELAGTableKey &lag_key =
      static_cast<const BfRtPRELAGTableKey &>(key);
  // Get the LAG id from key
  bf_mc_lag_id_t lag_id = lag_key.getId();

  uint32_t max_lag;
  size_t tableSize;
  status = this->tableSizeGet(session, dev_tgt, &tableSize);

  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get LAG table available size, err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        status);
    return status;
  }
  max_lag = (uint32_t)tableSize;

  uint32_t i = 0;
  for (i = 0; i < n; i++) {
    // use next_idx to avoid overflow in bf_mc_lag_id_t
    uint32_t next_idx = lag_id + i + 1;

    if (next_idx >= max_lag) {
      status = BF_OBJECT_NOT_FOUND;
      break;
    }
    bf_mc_lag_id_t next_id = next_idx;
    auto this_key =
        static_cast<BfRtPRELAGTableKey *>((*key_data_pairs)[i].first);
    auto this_data =
        static_cast<BfRtPRELAGTableData *>((*key_data_pairs)[i].second);
    this_key->setId(next_id);
    status = this->tableEntryGet(session, dev_tgt, flags, *this_key, this_data);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s : Error : Failed to get entry for lag id %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                next_id);
      break;
    }
  }
  *num_returned = i;
  return status;
}

bf_status_t BfRtPRELAGTable::tableUsageGet(const BfRtSession &session,
                                           const bf_rt_target_t &dev_tgt,
                                           const uint64_t & /*flags*/,
                                           uint32_t *count) const {
  bf_status_t status = BF_SUCCESS;
  size_t tableSize;
  status = this->tableSizeGet(session, dev_tgt, &tableSize);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in getting max LAG entries, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
  } else {
    *count = (uint32_t)tableSize;
  }
  return status;
}

bf_status_t BfRtPRELAGTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtPRELAGTableKey(this));
  if (*key_ret == nullptr) {
    LOG_TRACE("5d5847ca : %s:%d %s Error : Failed to allocate key",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPRELAGTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret = std::unique_ptr<BfRtTableData>(new BfRtPRELAGTableData(this));
  if (*data_ret == nullptr) {
    LOG_TRACE("2332c7f3 : %s:%d %s Error : Failed to allocate data",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPRELAGTable::keyReset(BfRtTableKey *key) const {
  BfRtPRELAGTableKey *lag_key = static_cast<BfRtPRELAGTableKey *>(key);

  return key_reset<BfRtPRELAGTable, BfRtPRELAGTableKey>(*this, lag_key);
}

bf_status_t BfRtPRELAGTable::dataReset(BfRtTableData *data) const {
  BfRtPRELAGTableData *lag_data = static_cast<BfRtPRELAGTableData *>(data);

  return data_reset<BfRtPRELAGTable, BfRtPRELAGTableData>(*this, lag_data);
}

// Prune Table APIs
bf_status_t BfRtPREMulticastPruneTable::tableEntryAdd(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    const BfRtTableData &data) const {
  bf_status_t status = BF_SUCCESS;

  const BfRtPREMulticastPruneTableKey &prune_key =
      static_cast<const BfRtPREMulticastPruneTableKey &>(key);
  const BfRtPREMulticastPruneTableData &prune_data =
      static_cast<const BfRtPREMulticastPruneTableData &>(data);

  // Get the L2 XID from key
  bf_mc_l2_xid_t l2_xid = prune_key.getId();

  // Update the prune map table in MC mgr with the new data
  auto *mcMgr = McMgrIntf::getInstance(session);
  status = mcMgr->mcMgrSetPortPruneTable(
      session.preSessHandleGet(),
      dev_tgt.dev_id,
      l2_xid,
      const_cast<uint8_t *>(prune_data.getPortBitmap()));

  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in modifying multicast prune index %d, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              l2_xid,
              status);
    return status;
  }

  return status;
}

bf_status_t BfRtPREMulticastPruneTable::tableEntryMod(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    const BfRtTableData &data) const {
  // For Multicast Prune table, table entry add/mod/del are same
  return this->tableEntryAdd(session, dev_tgt, flags, key, data);
}

bf_status_t BfRtPREMulticastPruneTable::tableEntryGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;

  const BfRtPREMulticastPruneTableKey &prune_key =
      static_cast<const BfRtPREMulticastPruneTableKey &>(key);
  BfRtPREMulticastPruneTableData *prune_data =
      static_cast<BfRtPREMulticastPruneTableData *>(data);

  // Get the L2 XID from key
  bf_mc_l2_xid_t l2_xid = prune_key.getId();

  // Get the prune bitmap from MC mgr
  bf_mc_port_map_t port_map;
  auto *mcMgr = McMgrIntf::getInstance(session);
  bool from_hw = BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW) ? true : false;

  status = mcMgr->mcMgrGetPortPruneTable(
      session.preSessHandleGet(), dev_tgt.dev_id, l2_xid, &port_map, from_hw);

  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in reading multicast prune index %d, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              l2_xid,
              status);
    return status;
  }

  // Update the data to return the bitmap
  prune_data->setPortBitmap(port_map);

  return status;
}

bf_status_t BfRtPREMulticastPruneTable::tableEntryGetFirst(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    BfRtTableKey *key,
    BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;
  BfRtPREMulticastPruneTableKey *prune_key =
      static_cast<BfRtPREMulticastPruneTableKey *>(key);
  BfRtPREMulticastPruneTableData *prune_data =
      static_cast<BfRtPREMulticastPruneTableData *>(data);
  bf_mc_l2_xid_t l2_xid = 0;
  // Get the prune bitmap from MC mgr
  bf_mc_port_map_t port_map;
  auto *mcMgr = McMgrIntf::getInstance(session);
  bool from_hw = BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW) ? true : false;

  status = mcMgr->mcMgrGetPortPruneTable(
      session.preSessHandleGet(), dev_tgt.dev_id, l2_xid, &port_map, from_hw);

  if (status != BF_SUCCESS) {
    LOG_TRACE("%s %s 724da5e3 : Error in reading multicast prune index %d",
              __func__,
              table_name_get().c_str(),
              l2_xid);
    return status;
  }
  // Update the data to return the bitmap
  prune_key->setId(l2_xid);
  prune_data->setPortBitmap(port_map);
  return BF_SUCCESS;
}

bf_status_t BfRtPREMulticastPruneTable::tableEntryGetNext_n(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    const uint32_t &n,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  bf_status_t status = BF_SUCCESS;
  if (!num_returned) {
    LOG_TRACE(
        "d16c1683 : %s:%d %s: Invalid Argument : num_returned cannot be NULL",
        __func__,
        __LINE__,
        table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  const BfRtPREMulticastPruneTableKey &prune_key =
      static_cast<const BfRtPREMulticastPruneTableKey &>(key);
  // Get the L2 XID from key
  bf_mc_l2_xid_t l2_xid = prune_key.getId();
  // Get the prune bitmap from MC mgr
  bf_mc_port_map_t port_map;
  auto *mcMgr = McMgrIntf::getInstance(session);
  bool from_hw = BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW) ? true : false;
  uint32_t i = 0;
  uint32_t num_ports = 0;
  this->tableUsageGet(session, dev_tgt, 0, &num_ports);
  for (i = 0; i < n; i++) {
    // use next_idx to avoid overflow in bf_mc_lag_id_t
    uint32_t next_idx = l2_xid + i + 1;
    if (next_idx >= num_ports) {
      status = BF_OBJECT_NOT_FOUND;
      break;
    }
    bf_mc_l2_xid_t next_id = next_idx;
    status = mcMgr->mcMgrGetPortPruneTable(session.preSessHandleGet(),
                                           dev_tgt.dev_id,
                                           next_id,
                                           &port_map,
                                           from_hw);
    if (status != BF_SUCCESS) {
      if (BF_OBJECT_NOT_FOUND == status) break;
      LOG_ERROR("d5dd4180 : %s %s Error in reading multicast prune index %d",
                __func__,
                table_name_get().c_str(),
                l2_xid);
      break;
    }
    auto this_key = static_cast<BfRtPREMulticastPruneTableKey *>(
        (*key_data_pairs)[i].first);
    auto this_data = static_cast<BfRtPREMulticastPruneTableData *>(
        (*key_data_pairs)[i].second);
    this_key->setId(next_id);
    this_data->setPortBitmap(port_map);
  }
  *num_returned = i;
  return status;
}

bf_status_t BfRtPREMulticastPruneTable::tableUsageGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    uint32_t *count) const {
  bf_status_t status = BF_SUCCESS;
  auto *mcMgr = McMgrIntf::getInstance(session);

  uint32_t prune_tbl_sz = 0;
  status = mcMgr->mcMgrGetPortPruneTableSize(
      session.preSessHandleGet(), dev_tgt.dev_id, &prune_tbl_sz);
  if (status != BF_SUCCESS) {
    LOG_TRACE("Prune Table size get failed");
    return status;
  }

  *count = prune_tbl_sz;
  return BF_SUCCESS;
}

bf_status_t BfRtPREMulticastPruneTable::tableEntryDel(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key) const {
  const BfRtPREMulticastPruneTableData prune_data(this);
  const BfRtTableData &data = prune_data;
  // For Multicast Prune table, table entry add/mod/del are same
  return this->tableEntryAdd(session, dev_tgt, flags, key, data);
}

bf_status_t BfRtPREMulticastPruneTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret =
      std::unique_ptr<BfRtTableKey>(new BfRtPREMulticastPruneTableKey(this));
  if (*key_ret == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to allocate key",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPREMulticastPruneTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret =
      std::unique_ptr<BfRtTableData>(new BfRtPREMulticastPruneTableData(this));
  if (*data_ret == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to allocate data",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPREMulticastPruneTable::keyReset(BfRtTableKey *key) const {
  BfRtPREMulticastPruneTableKey *prune_key =
      static_cast<BfRtPREMulticastPruneTableKey *>(key);

  return key_reset<BfRtPREMulticastPruneTable, BfRtPREMulticastPruneTableKey>(
      *this, prune_key);
}

bf_status_t BfRtPREMulticastPruneTable::dataReset(BfRtTableData *data) const {
  BfRtPREMulticastPruneTableData *prune_data =
      static_cast<BfRtPREMulticastPruneTableData *>(data);

  return data_reset<BfRtPREMulticastPruneTable, BfRtPREMulticastPruneTableData>(
      *this, prune_data);
}

// Port Table APIs
bf_status_t BfRtPREMulticastPortTable::tableEntryAdd(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    const BfRtTableData &data) const {
  bf_status_t status = BF_SUCCESS;

  const BfRtPREMulticastPortTableKey &mc_port_key =
      static_cast<const BfRtPREMulticastPortTableKey &>(key);
  const BfRtPREMulticastPortTableData &mc_port_data =
      static_cast<const BfRtPREMulticastPortTableData &>(data);

  // Get the port id from key
  bf_dev_port_t dev_port = mc_port_key.getId();

  // Get the data field maps from passed in data
  const std::unordered_map<bf_rt_id_t, bool> &boolField =
      mc_port_data.getBoolFieldDataMap();
  const std::unordered_map<bf_rt_id_t, uint64_t> &u64Field =
      mc_port_data.getU64FieldDataMap();

  bf_rt_id_t field_id;
  auto *mcMgr = McMgrIntf::getInstance(session);

  // For each filed name in the MC port table, get the
  // field id and check if it is present in the input data and call
  // the corresponding MC mgr APIs

  // Set the MC backup port for the protected port (key)
  bf_dev_port_t backup_port;
  status = this->dataFieldIdGet("$MULTICAST_BACKUP_PORT", &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for multicast basckup "
        "port field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }

  if (u64Field.find(field_id) != u64Field.end()) {
    auto it = u64Field.find(field_id);
    backup_port = static_cast<bf_dev_port_t>(it->second);
    status = mcMgr->mcMgrSetPortProtection(
        session.preSessHandleGet(), dev_tgt.dev_id, dev_port, backup_port);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error in setting backup port %d for protected port %d, err "
          "%d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          backup_port,
          dev_port,
          status);
      return status;
    }
  }

  // Set the MC forward state for the port
  status = this->dataFieldIdGet("$MULTICAST_FORWARD_STATE", &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for multicast forward "
        "state "
        "field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }

  if (boolField.find(field_id) != boolField.end()) {
    auto it = boolField.find(field_id);
    bool is_active = it->second;
    status = mcMgr->mcMgrSetPortForwardState(
        session.preSessHandleGet(), dev_tgt.dev_id, dev_port, is_active);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error in setting multicast forward state to %d for port "
          "%d, err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          is_active,
          dev_port,
          status);
      return status;
    }
  }

  // Clear muticast fast failover
  status = this->dataFieldIdGet("$MULTICAST_CLEAR_FAST_FAILOVER", &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for multicast clear "
        "fast failover "
        "field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }

  if (boolField.find(field_id) != boolField.end()) {
    auto it = boolField.find(field_id);
    bool is_clear = it->second;
    if (is_clear) {
      status = mcMgr->mcMgrClearFastFailoverState(
          session.preSessHandleGet(), dev_tgt.dev_id, dev_port);
      if (status != BF_SUCCESS) {
        LOG_TRACE(
            "%s:%d %s Error in clearing multicast fast failover state for port "
            "%d, err %d",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            dev_port,
            status);
        return status;
      }
    }
  }

  // Set the copy-to-cpu port
  status = this->dataFieldIdGet("$COPY_TO_CPU_PORT_ENABLE", &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for copy-to-cpu port "
        "enable "
        "field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }

  if (boolField.find(field_id) != boolField.end()) {
    auto it = boolField.find(field_id);
    bool enable = it->second;
    status = mcMgr->mcMgrSetCopyToCPUPort(dev_tgt.dev_id, enable, dev_port);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error in setting copy-cpu-port enable %d for port %d, err "
          "%d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          enable,
          dev_port,
          status);
      return status;
    }
  }

  return status;
}

bf_status_t BfRtPREMulticastPortTable::tableEntryMod(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    const BfRtTableData &data) const {
  // For Multicast Port table, table entry add and modify are same
  return this->tableEntryAdd(session, dev_tgt, flags, key, data);
}

bf_status_t BfRtPREMulticastPortTable::tableEntryGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;

  const BfRtPREMulticastPortTableKey &mc_port_key =
      static_cast<const BfRtPREMulticastPortTableKey &>(key);
  BfRtPREMulticastPortTableData *mc_port_data =
      static_cast<BfRtPREMulticastPortTableData *>(data);

  // Get the port id from key
  bf_rt_id_t dev_port_id = mc_port_key.getId();
  bf_dev_port_t dev_port = dev_port_id;

  bf_rt_id_t field_id;
  auto *mcMgr = McMgrIntf::getInstance(session);

  // For each filed name in the MC port table, get the
  // field id and check if it is present in the input data and call
  // the corresponding MC mgr APIs

  // Get the MC backup port for the protected port (key)
  bf_dev_port_t backup_port;
  status = this->dataFieldIdGet("$MULTICAST_BACKUP_PORT", &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for multicast backup "
        "port field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  status = mcMgr->mcMgrGetPortProtection(
      session.preSessHandleGet(), dev_tgt.dev_id, dev_port, &backup_port);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s Error in getting backup port for protected port %d, err "
        "%d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        dev_port,
        status);
    return status;
  }
  mc_port_data->setValue(field_id, (uint64_t)backup_port);

  // Get the MC forward state for the port
  status = this->dataFieldIdGet("$MULTICAST_FORWARD_STATE", &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for multicast forward "
        "state "
        "field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  bool is_enabled;
  status = mcMgr->mcMgrGetPortForwardState(
      session.preSessHandleGet(), dev_tgt.dev_id, dev_port, &is_enabled);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s Error in getting multicast forward state for port "
        "%d, err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        dev_port,
        status);
    return status;
  }
  mc_port_data->setValue(field_id, is_enabled);

  // Get muticast fast failover state
  status = this->dataFieldIdGet("$MULTICAST_CLEAR_FAST_FAILOVER", &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for multicast clear "
        "fast failover "
        "field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  status = mcMgr->mcMgrGetFastFailoverState(
      session.preSessHandleGet(), dev_tgt.dev_id, dev_port, &is_enabled);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s Error in clearing multicast fast failover state for port "
        "%d, err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        dev_port,
        status);
    return status;
  }
  mc_port_data->setValue(field_id, is_enabled);

  // Get the copy-to-cpu port
  status = this->dataFieldIdGet("$COPY_TO_CPU_PORT_ENABLE", &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error : Failed to get the field id for copy-to-cpu port "
        "enable "
        "field",
        __func__,
        __LINE__,
        table_name_get().c_str());

    return status;
  }
  bf_dev_port_t cpu_port;
  status = mcMgr->mcMgrGetCopyToCPUPort(dev_tgt.dev_id, &cpu_port, &is_enabled);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s Error in setting copy-cpu-port enable flags for port %d, "
        "err "
        "%d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        dev_port,
        status);
    return status;
  }
  if (!is_enabled || cpu_port != dev_port) {
    is_enabled = false;
  }
  mc_port_data->setValue(field_id, is_enabled);

  return status;
}

bf_status_t BfRtPREMulticastPortTable::tableEntryGetFirst(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    BfRtTableKey *key,
    BfRtTableData *data) const {
  BfRtPREMulticastPortTableKey *mc_port_key =
      static_cast<BfRtPREMulticastPortTableKey *>(key);
  bf_rt_id_t dev_port = 0;
  mc_port_key->setId(dev_port);
  return this->tableEntryGet(session, dev_tgt, flags, *key, data);
}

bf_status_t BfRtPREMulticastPortTable::tableEntryGetNext_n(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    const uint32_t &n,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  bf_status_t status = BF_SUCCESS;
  const BfRtPREMulticastPortTableKey &mc_port_key =
      static_cast<const BfRtPREMulticastPortTableKey &>(key);

  bf_rt_id_t dev_port_id = mc_port_key.getId();
  bf_dev_port_t dev_port = dev_port_id;

  uint32_t num_pipe = 0;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  pipe_status_t pipe_status =
      pipeMgr->pipeMgrGetNumPipelines(dev_tgt.dev_id, &num_pipe);
  if (pipe_status != PIPE_SUCCESS) {
    status = BF_INVALID_ARG;
    LOG_TRACE("Failed to get num pipe info on dev %d, sts %s (%d)",
              dev_tgt.dev_id,
              bf_err_str(status),
              status);
    return status;
  }

  bf_dev_pipe_t start_pipe = DEV_PORT_TO_PIPE(dev_port);
  int start_local_port = DEV_PORT_TO_LOCAL_PORT(dev_port);
  uint32_t i = 0;

  for (bf_dev_pipe_t pipe = start_pipe; pipe < num_pipe && i < n; pipe++) {
    for (int local_port = start_local_port + 1;
         local_port < BF_PIPE_PORT_COUNT && i < n;
         local_port++) {
      bf_dev_port_t next_port = MAKE_DEV_PORT(pipe, local_port);
      auto this_key = static_cast<BfRtPREMulticastPortTableKey *>(
          (*key_data_pairs)[i].first);
      auto this_data = static_cast<BfRtPREMulticastPortTableData *>(
          (*key_data_pairs)[i].second);
      this_key->setId(next_port);
      status =
          this->tableEntryGet(session, dev_tgt, flags, *this_key, this_data);
      if (status != BF_SUCCESS) {
        if (BF_OBJECT_NOT_FOUND == status) break;
        LOG_ERROR("%s %s Error in gettting multicast port %d",
                  __func__,
                  table_name_get().c_str(),
                  next_port);
        break;
      }
      i += 1;
    }
    start_local_port = -1;
    if (status != BF_SUCCESS) {
      break;
    }
  }
  *num_returned = i;
  return status;
}

bf_status_t BfRtPREMulticastPortTable::tableUsageGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /* flags */,
    uint32_t *count) const {
  bf_status_t status = BF_SUCCESS;
  uint32_t num_pipe = 0;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  pipe_status_t pipe_status =
      pipeMgr->pipeMgrGetNumPipelines(dev_tgt.dev_id, &num_pipe);
  if (pipe_status != PIPE_SUCCESS) {
    status = BF_INVALID_ARG;
    LOG_TRACE("Failed to get num pipe info on dev %d, sts %s (%d)",
              dev_tgt.dev_id,
              bf_err_str(status),
              status);
    return status;
  }
  *count = num_pipe * BF_PIPE_PORT_COUNT;
  return status;
}

bf_status_t BfRtPREMulticastPortTable::tableEntryDel(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key) const {
  // For Multicast Port table, table entry add/modify/del are the same
  const std::vector<bf_rt_id_t> empty_field_id;
  const BfRtPREMulticastPortTableData mc_port_data(this, empty_field_id);
  const BfRtTableData &data = mc_port_data;
  return this->tableEntryAdd(session, dev_tgt, flags, key, data);
}

bf_status_t BfRtPREMulticastPortTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret =
      std::unique_ptr<BfRtTableKey>(new BfRtPREMulticastPortTableKey(this));
  if (*key_ret == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to allocate key",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPREMulticastPortTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  std::vector<bf_rt_id_t> fields;

  *data_ret = std::unique_ptr<BfRtTableData>(
      new BfRtPREMulticastPortTableData(this, fields));
  if (*data_ret == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to allocate data",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtPREMulticastPortTable::dataAllocate(
    const std::vector<bf_rt_id_t> &fields,
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret = std::unique_ptr<BfRtTableData>(
      new BfRtPREMulticastPortTableData(this, fields));

  if (*data_ret == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to allocate data",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtPREMulticastPortTable::keyReset(BfRtTableKey *key) const {
  BfRtPREMulticastPortTableKey *port_key =
      static_cast<BfRtPREMulticastPortTableKey *>(key);

  return key_reset<BfRtPREMulticastPortTable, BfRtPREMulticastPortTableKey>(
      *this, port_key);
}

bf_status_t BfRtPREMulticastPortTable::dataReset(BfRtTableData *data) const {
  BfRtPREMulticastPortTableData *port_data =
      static_cast<BfRtPREMulticastPortTableData *>(data);

  return data_reset<BfRtPREMulticastPortTable, BfRtPREMulticastPortTableData>(
      *this, port_data);
}

bf_status_t BfRtPREMulticastPortTable::dataReset(
    const std::vector<bf_rt_id_t> &fields, BfRtTableData *data) const {
  BfRtPREMulticastPortTableData *port_data =
      static_cast<BfRtPREMulticastPortTableData *>(data);

  return data_reset<BfRtPREMulticastPortTable, BfRtPREMulticastPortTableData>(
      *this, fields, port_data);
}

}  // namespace bfrt
