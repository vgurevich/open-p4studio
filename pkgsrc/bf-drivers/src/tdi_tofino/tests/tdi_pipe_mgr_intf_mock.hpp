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

#ifndef _TDI_PIPE_MGR_INTERFACE_MOCK_HPP
#define _TDI_PIPE_MGR_INTERFACE_MOCK_HPP

#ifdef __cplusplus
extern "C" {
#endif
//#include <tdi/tdi_common.h>
#include <pipe_mgr/pipe_mgr_intf.h>
#ifdef __cplusplus
}
#endif

#include "gmock/gmock.h"

#include <map>

/* tdi_includes */
//#include <tdi/tdi_info.hpp>
#include <tdi/common/tdi_info.hpp>
#include <../tdi_common/tdi_pipe_mgr_intf.hpp>
//#include <tdi_common/tdi_table_data_impl.hpp>
//#include <tdi_common/tdi_table_impl.hpp>
#include "../tdi_common/tdi_session_impl.hpp"
#include <tdi/common/tdi_utils.hpp>

#include <mutex>

namespace tdi {
namespace tna {
namespace tofino {
namespace tofino_test {

using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;

#if 0
struct Entry {
 public:
  Entry(const pipe_tbl_match_spec_t *ms,
        const pipe_action_spec_t *as,
        const pipe_act_fn_hdl_t afh,
        const uint64_t &ttl,
        const TdiTable *tbl)
      : match_spec(ms),
        action_spec(as),
        act_fn_hdl(afh),
        table(tbl),
        ttl_val(ttl) {}

  const pipe_tbl_match_spec_t *match_spec{nullptr};
  const pipe_action_spec_t *action_spec{nullptr};
  const pipe_act_fn_hdl_t act_fn_hdl{0};
  const TdiTable *table{nullptr};
  const uint64_t ttl_val{0};
};

struct SelGroupEntry {
 public:
  SelGroupEntry(const uint32_t &max_size) : max_group_size(max_size) {}
  const uint32_t max_group_size{0};
  std::vector<pipe_adt_ent_hdl_t> adt_ent_hdls;
  std::vector<bool> adt_ent_hdl_sts;
};

class MockIPipeMgrIntfHelper {
 public:
  pipe_status_t pipeMgrGetEntry(pipe_sess_hdl_t sess_hdl,
                                pipe_mat_tbl_hdl_t tbl_hdl,
                                tdi_dev_id_t dev_id,
                                pipe_mat_ent_hdl_t entry_hdl,
                                pipe_tbl_match_spec_t *pipe_match_spec,
                                pipe_action_spec_t *pipe_action_spec,
                                pipe_act_fn_hdl_t *act_fn_hdl,
                                bool from_hw,
                                uint32_t res_get_flags,
                                pipe_res_get_data_t *res_data) {
    if (entry_database.find(entry_hdl) == entry_database.end()) {
      // This indicates that there is a problem somewhere in the unit test
      // or tdi logic
      TDI_ASSERT(0);
    }
    // Set the action function handle
    *act_fn_hdl = entry_database[entry_hdl]->act_fn_hdl;

    const auto src_ms = entry_database[entry_hdl]->match_spec;
    const auto src_as = entry_database[entry_hdl]->action_spec;

    // Copy the match spec stored in the entry database
    pipe_match_spec->partition_index = src_ms->partition_index;
    pipe_match_spec->num_valid_match_bits = src_ms->num_valid_match_bits;
    pipe_match_spec->num_match_bytes = src_ms->num_match_bytes;
    pipe_match_spec->priority = src_ms->priority;
    std::memcpy(pipe_match_spec->match_value_bits,
                src_ms->match_value_bits,
                src_ms->num_match_bytes);
    std::memcpy(pipe_match_spec->match_mask_bits,
                src_ms->match_mask_bits,
                src_ms->num_match_bytes);

    // Copy the action spec stored in the entry database
    pipe_action_spec->pipe_action_datatype_bmap =
        src_as->pipe_action_datatype_bmap;
    pipe_action_spec->adt_ent_hdl = src_as->adt_ent_hdl;
    pipe_action_spec->sel_grp_hdl = src_as->sel_grp_hdl;
    pipe_action_spec->resource_count = src_as->resource_count;
    std::memcpy(pipe_action_spec->resources,
                src_as->resources,
                sizeof(src_as->resources));
    pipe_action_spec->act_data.num_valid_action_data_bits =
        src_as->act_data.num_valid_action_data_bits;
    pipe_action_spec->act_data.num_action_data_bytes =
        src_as->act_data.num_action_data_bytes;
    std::memcpy(pipe_action_spec->act_data.action_data_bits,
                src_as->act_data.action_data_bits,
                src_as->act_data.num_action_data_bytes);

    // Set resources
    std::memset(res_data, 0, sizeof(*res_data));

    bool get_stats = res_get_flags & PIPE_RES_GET_FLAG_CNTR;
    bool get_meter = res_get_flags & PIPE_RES_GET_FLAG_METER;
    bool get_stful = res_get_flags & PIPE_RES_GET_FLAG_STFUL;
    bool get_idle = res_get_flags & PIPE_RES_GET_FLAG_IDLE;

    res_data->has_counter = false;
    res_data->has_meter = false;
    res_data->has_lpf = false;
    res_data->has_red = false;
    res_data->has_stful = false;
    res_data->has_ttl = false;
    res_data->has_hit_state = false;

    if (get_stats) {
      if (!pipeMgrResourceReadHelper<pipe_stat_data_t>(
              tbl_hdl,
              entry_hdl,
              DataFieldType::COUNTER_INDEX,
              &res_data->counter)) {
        res_data->has_counter = true;
      }
    }

    if (get_meter) {
      if (!pipeMgrResourceReadHelper<pipe_lpf_spec_t>(tbl_hdl,
                                                      entry_hdl,
                                                      DataFieldType::LPF_INDEX,
                                                      &res_data->mtr.lpf)) {
        res_data->has_lpf = true;
      }
      if (!pipeMgrResourceReadHelper<pipe_meter_spec_t>(
              tbl_hdl,
              entry_hdl,
              DataFieldType::METER_INDEX,
              &res_data->mtr.meter)) {
        res_data->has_meter = true;
      }
      if (!pipeMgrResourceReadHelper<pipe_wred_spec_t>(
              tbl_hdl,
              entry_hdl,
              DataFieldType::WRED_INDEX,
              &res_data->mtr.red)) {
        res_data->has_red = true;
      }
    }

    if (get_idle) {
      res_data->idle.ttl = entry_database[entry_hdl]->ttl_val;
      res_data->has_ttl = true;
      // TODO HIT_STATE
    }
    // TODO: STFUL
    if (get_stful) {
      // if (!pipeMgrResourceReadHelper<pipe_meter_spec_t>(
      //      tbl_hdl, entry_hdl, DataFieldType::REGISTER_INDEX,
      //      &res_data->stful)) {
      //  res_data->has_stful = true;
      //}
      res_data->has_stful = true;
      res_data->stful.pipe_count = 0;
      res_data->stful.data = nullptr;
    }

    return PIPE_SUCCESS;
  }

  template <typename T>
  pipe_status_t pipeMgrResourceReadHelper(pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                          pipe_mat_ent_hdl_t mat_ent_hdl,
                                          DataFieldType data_field_type,
                                          T *rs_spec) {
    if (entry_database.find(mat_ent_hdl) == entry_database.end()) {
      // This indicates that there is a problem somewhere in the unit test
      // or tdi logic
      TDI_ASSERT(0);
    }
    const auto src_as = entry_database[mat_ent_hdl]->action_spec;
    const TdiTable *table = entry_database[mat_ent_hdl]->table;
    pipe_tbl_hdl_t resource_tbl_hdl =
        static_cast<const TdiTableObj &>(*table).getResourceHdl(
            data_field_type);
    // EXPECT_EQ(resource_tbl_hdl, mat_tbl_hdl) << " Resource Get Called for
    // table hdl : "<<mat_tbl_hdl<<" when the resource table handle cached in
    // TdiTable is : "<<resource_tbl_hdl<<" for resource type :
    // "<<static_cast<int>(data_field_type);
    // Once we have the counter table hdl, iterate over the resource spec
    // array in the action spec and get hold of the counter spec
    const T *resource_spec = nullptr;
    for (int i = 0; i < src_as->resource_count; i++) {
      if (src_as->resources[i].tbl_hdl == resource_tbl_hdl) {
        resource_spec = reinterpret_cast<const T *>(&src_as->resources[i].data);
      }
    }
    if (resource_spec == nullptr) {
      // Indicates that we have a field which is of type resource spec but
      // dont have a corresponding resource table handle in our "ground truth"
      // pipe action spec. This indicates an error (mostly in the ground
      // truth action spec generation)
      // TDI_ASSERT(0);
      // Can be valid function call if caller is pipeMgrEntryGet, since caller
      // to that function do not have knowledge about which resources are
      // supported and requests everything that user specifies.
      return PIPE_OBJ_NOT_FOUND;
    }
    // Copy the stored resource data into the out param
    std::memcpy(rs_spec, resource_spec, sizeof(T));
    return PIPE_SUCCESS;
  }

  pipe_status_t pipeMgrLpfReadEntry(pipe_sess_hdl_t sess_hdl,
                                    dev_target_t dev_tgt,
                                    pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                    pipe_mat_ent_hdl_t mat_ent_hdl,
                                    pipe_lpf_spec_t *lpf_spec) {
    // Pass in any Data Field type which will yield a table handle for lpf table
    // Refer getResourceInternal() for applicable types
    return pipeMgrResourceReadHelper<pipe_lpf_spec_t>(
        mat_tbl_hdl, mat_ent_hdl, DataFieldType::LPF_INDEX, lpf_spec);
  }

  pipe_status_t pipeMgrMeterReadEntry(pipe_sess_hdl_t sess_hdl,
                                      dev_target_t dev_tgt,
                                      pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                      pipe_mat_ent_hdl_t mat_ent_hdl,
                                      pipe_meter_spec_t *meter_spec) {
    // Pass in any Data Field type which will yield a table handle for meter
    // table
    // Refer getResourceInternal() for applicable types
    return pipeMgrResourceReadHelper<pipe_meter_spec_t>(
        mat_tbl_hdl, mat_ent_hdl, DataFieldType::METER_INDEX, meter_spec);
  }

  pipe_status_t pipeMgrMatEntDirectStatQuery(pipe_sess_hdl_t sess_hdl,
                                             tdi_dev_id_t device_id,
                                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                             pipe_mat_ent_hdl_t mat_ent_hdl,
                                             pipe_stat_data_t *stat_data) {
    // Pass in any Data Field type which will yield a table handle for counter
    // table
    // Refer getResourceInternal() for applicable types
    return pipeMgrResourceReadHelper<pipe_stat_data_t>(
        mat_tbl_hdl, mat_ent_hdl, DataFieldType::COUNTER_INDEX, stat_data);
  }

  pipe_status_t pipeMgrMatEntGetIdleTtl(pipe_sess_hdl_t sess_hdl,
                                        tdi_dev_id_t device_id,
                                        pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                        pipe_mat_ent_hdl_t mat_ent_hdl,
                                        uint32_t *ttl) {
    if (entry_database.find(mat_ent_hdl) == entry_database.end()) {
      // This indicates that there is a problem somewhere in the unit test
      // or tdi logic
      TDI_ASSERT(0);
    }
    *ttl = entry_database[mat_ent_hdl]->ttl_val;
    return PIPE_SUCCESS;
  }

  pipe_status_t pipeMgrGetFirstEntryHandle(pipe_sess_hdl_t sess_hdl,
                                           pipe_mat_tbl_hdl_t tbl_hdl,
                                           dev_target_t dev_tgt,
                                           int *entry_handle) {
    if (entry_database.size()) {
      *entry_handle = entry_database.begin()->first;
    } else {
      // FIXME Confirm what exactly pipe mgr does when there are no entries
      // in the table
      *entry_handle = -1;
    }
    return PIPE_SUCCESS;
  }

  pipe_status_t pipeMgrGetNextEntryHandles(pipe_sess_hdl_t sess_hdl,
                                           pipe_mat_tbl_hdl_t tbl_hdl,
                                           dev_target_t dev_tgt,
                                           pipe_mat_ent_hdl_t entry_handle,
                                           int n,
                                           int *next_entry_handles) {
    // If the number of entries requested is greater than the num entries
    // installed in the table, then pipe mgr sets the remaining (unused)
    // entry handles to -1. So emulate that
    for (int i = 0; i < n; i++) {
      next_entry_handles[i] = -1;
    }
    // Now iterate over the map and start assigning entry handles after
    // the passed in entry handle
    int count = 0;
    for (const auto &iter : entry_database) {
      if (count >= n) break;
      if (iter.first > entry_handle) {
        next_entry_handles[count] = iter.first;
        count++;
      }
    }
    return PIPE_SUCCESS;
  }

  pipe_status_t pipeMgrGetActionDataEntry(
      pipe_adt_tbl_hdl_t tbl_hdl,
      tdi_dev_id_t dev_id,
      pipe_adt_ent_hdl_t entry_hdl,
      pipe_action_data_spec_t *pipe_action_data_spec,
      pipe_act_fn_hdl_t *act_fn_hdl,
      bool from_hw) {
    if (entry_database.find(entry_hdl) == entry_database.end()) {
      // This indicates that there is a problem somewhere in the unit test
      // or tdi logic
      TDI_ASSERT(0);
    }
    // Set the action function handle
    *act_fn_hdl = entry_database[entry_hdl]->act_fn_hdl;

    const auto src_as = entry_database[entry_hdl]->action_spec;

    // Copy the action spec stored in the entry database
    pipe_action_data_spec->num_valid_action_data_bits =
        src_as->act_data.num_valid_action_data_bits;
    pipe_action_data_spec->num_action_data_bytes =
        src_as->act_data.num_action_data_bytes;
    std::memcpy(pipe_action_data_spec->action_data_bits,
                src_as->act_data.action_data_bits,
                src_as->act_data.num_action_data_bytes);
    return PIPE_SUCCESS;
  }

  pipe_status_t pipeMgrGetSelGrpMbrCount(pipe_sess_hdl_t sess_hdl,
                                         tdi_dev_id_t dev_id,
                                         pipe_sel_tbl_hdl_t tbl_hdl,
                                         pipe_sel_grp_hdl_t sel_grp_hdl,
                                         uint32_t *count) {
    if (sel_group_database.find(sel_grp_hdl) == sel_group_database.end()) {
      // This indicates an error in the unit test logic as each entry must have
      // a unique entry hdl
      TDI_ASSERT(0);
    }
    const auto &sel_entry = *sel_group_database.at(sel_grp_hdl);
    TDI_ASSERT(sel_entry.adt_ent_hdls.size() ==
                 sel_entry.adt_ent_hdl_sts.size());
    *count = sel_entry.adt_ent_hdls.size();
    return PIPE_SUCCESS;
  }

  pipe_status_t pipeMgrSelGrpMbrsGet(pipe_sess_hdl_t sess_hdl,
                                     tdi_dev_id_t device_id,
                                     pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                     pipe_sel_grp_hdl_t sel_grp_hdl,
                                     uint32_t mbrs_size,
                                     pipe_adt_ent_hdl_t *mbrs,
                                     bool *enable,
                                     uint32_t *mbrs_populated) {
    if (sel_group_database.find(sel_grp_hdl) == sel_group_database.end()) {
      // This indicates an error in the unit test logic as each entry must have
      // a unique entry hdl
      TDI_ASSERT(0);
    }
    const auto &sel_entry = *sel_group_database.at(sel_grp_hdl);
    TDI_ASSERT(sel_entry.adt_ent_hdls.size() ==
                 sel_entry.adt_ent_hdl_sts.size());
    TDI_ASSERT(mbrs_size == sel_entry.adt_ent_hdls.size());
    *mbrs_populated = 0;
    for (uint32_t i = 0; i < sel_entry.adt_ent_hdls.size(); i++) {
      mbrs[i] = sel_entry.adt_ent_hdls[i];
      enable[i] = sel_entry.adt_ent_hdl_sts[i];
      (*mbrs_populated)++;
    }
    return PIPE_SUCCESS;
  }

  void addEntryToDatabase(uint32_t ent_hdl, const Entry *entry) {
    if (entry_database.find(ent_hdl) != entry_database.end()) {
      // This indicates an error in the unit test logic as each entry must have
      // a unique entry hdl
      TDI_ASSERT(0);
    }
    // Here are doing a shallow copy of the match and action specs stored in
    // Entry object passed in. This implies that the memory for
    // those things must be valid for the entrie lifetime of the tests
    std::unique_ptr<Entry> new_entry(new Entry(entry->match_spec,
                                               entry->action_spec,
                                               entry->act_fn_hdl,
                                               entry->ttl_val,
                                               entry->table));
    entry_database.insert(std::make_pair(ent_hdl, std::move(new_entry)));
  }

  void addSelGroupToDatabase(const pipe_sel_grp_hdl_t &grp_hdl,
                             const SelGroupEntry *sel_grp) {
    if (sel_group_database.find(grp_hdl) != sel_group_database.end()) {
      // This indicates an error in the unit test logic as each entry must have
      // a unique entry hdl
      TDI_ASSERT(0);
    }
    std::unique_ptr<SelGroupEntry> new_entry(
        new SelGroupEntry(sel_grp->max_group_size));
    sel_group_database.insert(std::make_pair(grp_hdl, std::move(new_entry)));
  }

  void addActMbrsToSelGroup(const pipe_sel_grp_hdl_t &grp_hdl,
                            const std::vector<pipe_adt_ent_hdl_t> &adt_ent_hdls,
                            const std::vector<bool> &adt_ent_hdl_sts) {
    if (sel_group_database.find(grp_hdl) == sel_group_database.end()) {
      // This indicates an error in the unit test logic as each entry must have
      // a unique entry hdl
      TDI_ASSERT(0);
    }
    sel_group_database[grp_hdl]->adt_ent_hdls = adt_ent_hdls;
    sel_group_database[grp_hdl]->adt_ent_hdl_sts = adt_ent_hdl_sts;
  }

  void databaseCleanup() {
    entry_database.clear();
    sel_group_database.clear();
  }

 private:
  std::map<uint32_t, std::unique_ptr<Entry>> entry_database;
  std::map<pipe_sel_grp_hdl_t, std::unique_ptr<SelGroupEntry>>
      sel_group_database;
};

#endif
class MockIPipeMgrIntf : public IPipeMgrIntf {
 public:
  static MockIPipeMgrIntf *getInstance(const tdi::Session &session) {
    return MockIPipeMgrIntf::getInstance();
  }
  static MockIPipeMgrIntf *getInstance() {
    if (instance.get() == nullptr) {
      pipe_mgr_intf_mtx.lock();
      if (instance.get() == nullptr) {
        instance.reset(new NiceMock<MockIPipeMgrIntf>());
      }
      pipe_mgr_intf_mtx.unlock();
    }
    return static_cast<MockIPipeMgrIntf *>(IPipeMgrIntf::instance.get());
  }
  // We need the following function only in case of MockIPipeMgrIntf and not
  // in PipeMgrIntf. This is because "instance" variable in IPipeMgrIntf is
  // a static global. Thus it will go out of scope only when the program
  // terminates, which means that the MockIPipeMgrIntf or PipeMgrIntf object
  // which it owns will be destroyed only at the end of the program. This is
  // exactly what we want for PipeMgrIntf but not for MockIPipeMgrIntf as while
  // running the tests, we want the mock object to be created and destroyed
  // for every test in the test suite in all the test fixtures. This is
  // required as the gtest framework automatically verifies all the
  // expectations on the mock object when the mock object is destroyed and
  // generates failure reports if the expectations have not been met. Thus
  // to enable the testing framework to actually cause the mock object to be
  // destroyed, we need to pass it a reference to the mock object
  static std::unique_ptr<IPipeMgrIntf> &getInstanceToUniquePtr() {
    pipe_mgr_intf_mtx.lock();
    if (instance.get() == nullptr) {
      instance.reset(new NiceMock<MockIPipeMgrIntf>());
    }
    pipe_mgr_intf_mtx.unlock();
    return instance;
  }
  ~MockIPipeMgrIntf() { int i = 0; }
  MOCK_METHOD0(pipeMgrInit, pipe_status_t(void));
  MOCK_METHOD0(pipeMgrCleanup, void(void));
  MOCK_METHOD1(pipeMgrClientInit, pipe_status_t(pipe_sess_hdl_t *sess_hdl));
  MOCK_METHOD1(pipeMgrClientCleanup,
               pipe_status_t(pipe_sess_hdl_t def_sess_hdl));
  MOCK_METHOD1(pipeMgrCompleteOperations,
               pipe_status_t(pipe_sess_hdl_t def_sess_hdl));
  MOCK_METHOD2(pipeMgrBeginTxn,
               pipe_status_t(pipe_sess_hdl_t shdl, bool isAtomic));
  MOCK_METHOD1(pipeMgrVerifyTxn, pipe_status_t(pipe_sess_hdl_t shdl));
  MOCK_METHOD1(pipeMgrAbortTxn, pipe_status_t(pipe_sess_hdl_t shdl));
  MOCK_METHOD2(pipeMgrCommitTxn,
               pipe_status_t(pipe_sess_hdl_t shdl, bool hwSynchronous));
  MOCK_METHOD1(pipeMgrBeginBatch, pipe_status_t(pipe_sess_hdl_t shdl));
  MOCK_METHOD1(pipeMgrFlushBatch, pipe_status_t(pipe_sess_hdl_t shdl));
  MOCK_METHOD2(pipeMgrEndBatch,
               pipe_status_t(pipe_sess_hdl_t shdl, bool hwSynchronous));
  MOCK_METHOD1(pipeMgrMatchSpecFree,
               pipe_status_t(pipe_tbl_match_spec_t *match_spec));
  MOCK_METHOD8(pipeMgrGetActionDirectResUsage,
               pipe_status_t(tdi_dev_id_t,
                             pipe_mat_tbl_hdl_t,
                             pipe_act_fn_hdl_t,
                             bool *,
                             bool *,
                             bool *,
                             bool *,
                             bool *));
  MOCK_METHOD6(pipeMgrMatchSpecToEntHdl,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                             pipe_tbl_match_spec_t *match_spec,
                             pipe_mat_ent_hdl_t *mat_ent_hdl,
                             bool light_pipe_validation));
  MOCK_METHOD6(pipeMgrEntHdlToMatchSpec,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                             pipe_mat_ent_hdl_t mat_ent_hdl,
                             bf_dev_pipe_t *ent_pipe_id,
                             const pipe_tbl_match_spec_t **match_spec));
  MOCK_METHOD4(pipeMgrMatchKeyMaskSpecSet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                             pipe_tbl_match_spec_t *match_spec));
  MOCK_METHOD3(pipeMgrMatchKeyMaskSpecReset,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl));
  MOCK_METHOD4(pipeMgrMatchKeyMaskSpecGet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                             pipe_tbl_match_spec_t *match_spec));
  MOCK_METHOD9(
      pipeMgrMatEntAdd,
      pipe_status_t(
          pipe_sess_hdl_t sess_hdl,
          dev_target_t dev_tgt,
          pipe_mat_tbl_hdl_t mat_tbl_hdl,
          pipe_tbl_match_spec_t *match_spec,
          pipe_act_fn_hdl_t act_fn_hdl,
          const pipe_action_spec_t *act_data_spec,
          uint32_t ttl,
          /*< TTL value in msecs, 0 for disable */ uint32_t pipe_api_flags,
          pipe_mat_ent_hdl_t *ent_hdl_p));
  MOCK_METHOD7(pipeMgrMatDefaultEntrySet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                             pipe_act_fn_hdl_t act_fn_hdl,
                             const pipe_action_spec_t *act_spec,
                             uint32_t pipe_api_flags,
                             pipe_mat_ent_hdl_t *ent_hdl_p));

  MOCK_METHOD8(pipeMgrTableGetDefaultEntry,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                             pipe_action_spec_t *pipe_action_spec,
                             pipe_act_fn_hdl_t *act_fn_hdl,
                             bool from_hw,
                             uint32_t reg_get_flags,
                             pipe_res_get_data_t *res_data));

  MOCK_METHOD4(pipeMgrTableGetDefaultEntryHandle,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                             pipe_mat_ent_hdl_t *ent_hdl_p));
  MOCK_METHOD4(pipeMgrMatTblClear,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                             uint32_t pipe_api_flags));
  MOCK_METHOD5(pipeMgrMatEntDel,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                             pipe_mat_ent_hdl_t mat_ent_hdl,
                             uint32_t pipe_api_flags));
  MOCK_METHOD5(pipeMgrMatEntDelByMatchSpec,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                             pipe_tbl_match_spec_t *match_spec,
                             uint32_t pipe_api_flags));
  MOCK_METHOD4(pipeMgrMatTblDefaultEntryReset,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                             uint32_t pipe_api_flags));
  MOCK_METHOD7(pipeMgrMatEntSetAction,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                             pipe_mat_ent_hdl_t mat_ent_hdl,
                             pipe_act_fn_hdl_t act_fn_hdl,
                             pipe_action_spec_t *act_spec,
                             uint32_t pipe_api_flags));
  MOCK_METHOD7(pipeMgrMatEntSetActionByMatchSpec,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                             pipe_tbl_match_spec_t *match_spec,
                             pipe_act_fn_hdl_t act_fn_hdl,
                             const pipe_action_spec_t *act_spec,
                             uint32_t pipe_api_flags));
  MOCK_METHOD7(pipeMgrMatEntSetResource,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                             pipe_mat_ent_hdl_t mat_ent_hdl,
                             pipe_res_spec_t *resources,
                             int resource_count,
                             uint32_t pipe_api_flags));
  MOCK_METHOD8(pipeMgrAdtEntAdd,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_adt_tbl_hdl_t adt_tbl_hdl,
                             pipe_adt_mbr_id_t mbr_id,
                             pipe_act_fn_hdl_t act_fn_hdl,
                             const pipe_action_spec_t *action_spec,
                             pipe_adt_ent_hdl_t *adt_ent_hdl_p,
                             uint32_t pipe_api_flags));
  MOCK_METHOD5(pipeMgrAdtEntDel,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             pipe_adt_tbl_hdl_t adt_tbl_hdl,
                             pipe_adt_ent_hdl_t adt_ent_hdl,
                             uint32_t pipe_api_flags));
  MOCK_METHOD7(pipeMgrAdtEntSet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             pipe_adt_tbl_hdl_t adt_tbl_hdl,
                             pipe_adt_ent_hdl_t adt_ent_hdl,
                             pipe_act_fn_hdl_t act_fn_hdl,
                             const pipe_action_spec_t *action_spec,
                             uint32_t pipe_api_flags));
  MOCK_METHOD3(pipeMgrTblIsTern,
               pipe_status_t(tdi_dev_id_t device_id,
                             pipe_tbl_hdl_t tbl_hdl,
                             bool *is_tern));

  MOCK_METHOD5(pipeMgrSelTblRegisterCb,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             pipe_sel_tbl_hdl_t sel_tbl_hdl,
                             pipe_mgr_sel_tbl_update_callback cb,
                             void *cb_cookie));
  MOCK_METHOD4(pipeMgrSelTblProfileSet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_sel_tbl_hdl_t sel_tbl_hdl,
                             pipe_sel_tbl_profile_t *sel_tbl_profile));
  MOCK_METHOD7(pipeMgrSelGrpAdd,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_sel_tbl_hdl_t sel_tbl_hdl,
                             pipe_sel_grp_id_t sel_grp_id,
                             uint32_t max_grp_size,
                             pipe_sel_grp_hdl_t *sel_grp_hdl_p,
                             uint32_t pipe_api_flags));
  MOCK_METHOD5(pipeMgrSelGrpDel,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             pipe_sel_tbl_hdl_t sel_tbl_hdl,
                             pipe_sel_grp_hdl_t sel_grp_hdl,
                             uint32_t pipe_api_flags));
  MOCK_METHOD7(pipeMgrSelGrpMbrAdd,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             pipe_sel_tbl_hdl_t sel_tbl_hdl,
                             pipe_sel_grp_hdl_t sel_grp_hdl,
                             pipe_act_fn_hdl_t act_fn_hdl,
                             pipe_adt_ent_hdl_t adt_ent_hdl,
                             uint32_t pipe_api_flags));
  MOCK_METHOD6(pipeMgrSelGrpMbrDel,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             pipe_sel_tbl_hdl_t sel_tbl_hdl,
                             pipe_sel_grp_hdl_t sel_grp_hdl,
                             pipe_adt_ent_hdl_t adt_ent_hdl,
                             uint32_t pipe_api_flags));
  MOCK_METHOD8(pipeMgrSelGrpMbrsSet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             pipe_sel_tbl_hdl_t sel_tbl_hdl,
                             pipe_sel_grp_hdl_t sel_grp_hdl,
                             uint32_t num_mbrs,
                             pipe_adt_ent_hdl_t *mbrs,
                             bool *enable,
                             uint32_t pipe_api_flags));
  MOCK_METHOD8(pipeMgrSelGrpMbrsGet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             pipe_sel_tbl_hdl_t sel_tbl_hdl,
                             pipe_sel_grp_hdl_t sel_grp_hdl,
                             uint32_t mbrs_size,
                             pipe_adt_ent_hdl_t *mbrs,
                             bool *enable,
                             uint32_t *mbrs_populated));
  MOCK_METHOD6(pipeMgrSelGrpMbrDisable,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             pipe_sel_tbl_hdl_t sel_tbl_hdl,
                             pipe_sel_grp_hdl_t sel_grp_hdl,
                             pipe_adt_ent_hdl_t adt_ent_hdl,
                             uint32_t pipe_api_flags));
  MOCK_METHOD6(pipeMgrSelGrpMbrEnable,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             pipe_sel_tbl_hdl_t sel_tbl_hdl,
                             pipe_sel_grp_hdl_t sel_grp_hdl,
                             pipe_adt_ent_hdl_t adt_ent_hdl,
                             uint32_t pipe_api_flags));
  MOCK_METHOD6(pipeMgrSelGrpMbrStateGet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             pipe_sel_tbl_hdl_t sel_tbl_hdl,
                             pipe_sel_grp_hdl_t sel_grp_hdl,
                             pipe_adt_ent_hdl_t adt_ent_hdl,
                             enum pipe_mgr_grp_mbr_state_e *mbr_state_p));
  MOCK_METHOD5(pipeMgrSelFallbackMbrSet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_sel_tbl_hdl_t sel_tbl_hdl,
                             pipe_adt_ent_hdl_t adt_ent_hdl,
                             uint32_t pipe_api_flags));
  MOCK_METHOD4(pipeMgrSelFallbackMbrReset,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_sel_tbl_hdl_t sel_tbl_hdl,
                             uint32_t pipe_api_flags));
  MOCK_METHOD7(pipeMgrSelGrpMbrGetFromHash,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t dev_id,
                             pipe_sel_tbl_hdl_t sel_tbl_hdl,
                             pipe_sel_grp_hdl_t grp_hdl,
                             uint8_t *hash,
                             uint32_t hash_len,
                             pipe_adt_ent_hdl_t *adt_ent_hdl_p));
  MOCK_METHOD5(pipeMgrSelGrpSizeSet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_sel_tbl_hdl_t sel_tbl_hdl,
                             pipe_sel_grp_hdl_t sel_grp_hdl,
                             uint32_t max_grp_size));
  MOCK_METHOD5(pipeMgrLrnDigestNotificationRegister,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             pipe_fld_lst_hdl_t flow_lrn_fld_lst_hdl,
                             pipe_flow_lrn_notify_cb callback_fn,
                             void *callback_fn_cookie));
  MOCK_METHOD3(pipeMgrLrnDigestNotificationDeregister,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             pipe_fld_lst_hdl_t flow_lrn_fld_lst_hdl));
  MOCK_METHOD3(pipeMgrFlowLrnNotifyAck,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             pipe_fld_lst_hdl_t flow_lrn_fld_lst_hdl,
                             pipe_flow_lrn_msg_t *pipe_flow_lrn_msg));
  MOCK_METHOD3(pipeMgrFlowLrnTimeoutSet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             uint32_t usecs));
  MOCK_METHOD2(pipeMgrFlowLrnTimeoutGet,
               pipe_status_t(tdi_dev_id_t device_id, uint32_t *usecs));
  MOCK_METHOD3(pipeMgrFlowLrnIntrModeSet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             bool en));
  MOCK_METHOD3(pipeMgrFlowLrnIntrModeGet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             bool *en));
  MOCK_METHOD2(pipeMgrFlowLrnSetNetworkOrderDigest,
               pipe_status_t(tdi_dev_id_t device_id, bool network_order));
  MOCK_METHOD5(pipeMgrMatEntDirectStatQuery,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                             pipe_mat_ent_hdl_t mat_ent_hdl,
                             pipe_stat_data_t *stat_data));
  MOCK_METHOD5(pipeMgrMatEntDirectStatSet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                             pipe_mat_ent_hdl_t mat_ent_hdl,
                             pipe_stat_data_t *stat_data));
  MOCK_METHOD5(pipeMgrMatEntDirectStatLoad,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                             pipe_mat_ent_hdl_t mat_ent_hdl,
                             pipe_stat_data_t *stat_data));
  MOCK_METHOD6(pipeMgrStatEntQuery,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_target,
                             pipe_stat_tbl_hdl_t stat_tbl_hdl,
                             pipe_stat_ent_idx_t *stat_ent_idx,
                             size_t num_entries,
                             pipe_stat_data_t **stat_data));
  MOCK_METHOD4(pipeMgrStatTableReset,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_stat_tbl_hdl_t stat_tbl_hdl,
                             pipe_stat_data_t *stat_data));
  MOCK_METHOD5(pipeMgrStatEntSet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_stat_tbl_hdl_t stat_tbl_hdl,
                             pipe_stat_ent_idx_t stat_ent_idx,
                             pipe_stat_data_t *stat_data));
  MOCK_METHOD5(pipeMgrStatEntLoad,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_stat_tbl_hdl_t stat_tbl_hdl,
                             pipe_stat_ent_idx_t stat_idx,
                             pipe_stat_data_t *stat_data));
  MOCK_METHOD5(pipeMgrStatDatabaseSync,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_stat_tbl_hdl_t stat_tbl_hdl,
                             pipe_mgr_stat_tbl_sync_cback_fn cback_fn,
                             void *cookie));
  MOCK_METHOD5(pipeMgrDirectStatDatabaseSync,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                             pipe_mgr_stat_tbl_sync_cback_fn cback_fn,
                             void *cookie));
  MOCK_METHOD4(pipeMgrStatEntDatabaseSync,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_stat_tbl_hdl_t stat_tbl_hdl,
                             pipe_stat_ent_idx_t stat_ent_idx));
  MOCK_METHOD4(pipeMgrDirectStatEntDatabaseSync,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                             pipe_mat_ent_hdl_t mat_ent_hdl));
  MOCK_METHOD4(pipeMgrMeterByteCountSet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_meter_tbl_hdl_t meter_tbl_hdl,
                             int byte_count));
  MOCK_METHOD4(pipeMgrMeterByteCountGet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_meter_tbl_hdl_t meter_tbl_hdl,
                             int *byte_count));
  MOCK_METHOD6(pipeMgrMeterEntSet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_meter_tbl_hdl_t meter_tbl_hdl,
                             pipe_meter_idx_t meter_idx,
                             pipe_meter_spec_t *meter_spec,
                             uint32_t pipe_api_flags));
  MOCK_METHOD3(pipeMgrModelTimeAdvance,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             uint64_t tick_time));
  MOCK_METHOD4(pipeMgrMeterReset,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_meter_tbl_hdl_t meter_tbl_hdl,
                             uint32_t pipe_api_flags));
  MOCK_METHOD4(pipeMgrLpfReset,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_lpf_tbl_hdl_t lpf_tbl_hdl,
                             uint32_t pipe_api_flags));
  MOCK_METHOD4(pipeMgrWredReset,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_wred_tbl_hdl_t wred_tbl_hdl,
                             uint32_t pipe_api_flags));
  MOCK_METHOD5(pipeMgrMeterReadEntry,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                             pipe_mat_ent_hdl_t mat_ent_hdl,
                             pipe_meter_spec_t *meter_spec));
  MOCK_METHOD6(pipeMgrMeterReadEntryIdx,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_meter_tbl_hdl_t meter_tbl_hdl,
                             pipe_meter_idx_t index,
                             pipe_meter_spec_t *meter_spec,
                             bool from_hw));
  MOCK_METHOD6(pipeMgrLpfEntSet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_meter_tbl_hdl_t meter_tbl_hdl,
                             pipe_lpf_idx_t lpf_idx,
                             pipe_lpf_spec_t *lpf_spec,
                             uint32_t pipe_api_flags));
  MOCK_METHOD6(pipeMgrWredEntSet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_meter_tbl_hdl_t meter_tbl_hdl,
                             pipe_wred_idx_t red_idx,
                             pipe_wred_spec_t *wred_spec,
                             uint32_t pipe_api_flags));
  MOCK_METHOD4(pipeMgrExmEntryActivate,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                             pipe_mat_ent_hdl_t mat_ent_hdl));
  MOCK_METHOD4(pipeMgrExmEntryDeactivate,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                             pipe_mat_ent_hdl_t mat_ent_hdl));
  MOCK_METHOD7(pipeMgrMatEntSetIdleTtl,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                             pipe_mat_ent_hdl_t mat_ent_hdl,
                             uint32_t ttl,
                             /*< TTL value in msecs */ uint32_t pipe_api_flags,
                             bool reset));
  MOCK_METHOD4(pipeMgrMatEntResetIdleTtl,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                             pipe_mat_ent_hdl_t mat_ent_hdl));
  MOCK_METHOD4(pipeMgrIdleTmoEnableSet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                             bool enable));
  MOCK_METHOD5(pipeMgrIdleRegisterTmoCb,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                             pipe_idle_tmo_expiry_cb cb,
                             void *client_data));
  MOCK_METHOD5(pipeMgrIdleRegisterTmoCbWithMatchSpecCopy,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                             pipe_idle_tmo_expiry_cb_with_match_spec_copy cb,
                             void *client_data));
  MOCK_METHOD5(pipeMgrIdleTimeGetHitState,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                             pipe_mat_ent_hdl_t mat_ent_hdl,
                             pipe_idle_time_hit_state_e *idle_time_data));
  MOCK_METHOD5(pipeMgrIdleTimeSetHitState,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                             pipe_mat_ent_hdl_t mat_ent_hdl,
                             pipe_idle_time_hit_state_e idle_time_data));
  MOCK_METHOD5(pipeMgrIdleTimeUpdateHitState,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                             pipe_idle_tmo_update_complete_cb callback_fn,
                             void *cb_data));
  MOCK_METHOD5(pipeMgrMatEntGetIdleTtl,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                             pipe_mat_ent_hdl_t mat_ent_hdl,
                             uint32_t *ttl));
  MOCK_METHOD4(pipeMgrIdleParamsGet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                             pipe_idle_time_params_t *params));
  MOCK_METHOD4(pipeMgrIdleParamsSet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                             pipe_idle_time_params_t params));
  MOCK_METHOD6(pipeStfulEntSet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_target,
                             pipe_stful_tbl_hdl_t stful_tbl_hdl,
                             pipe_stful_mem_idx_t stful_ent_idx,
                             pipe_stful_mem_spec_t *stful_spec,
                             uint32_t pipe_api_flags));
  MOCK_METHOD5(pipeStfulDatabaseSync,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_stful_tbl_hdl_t stful_tbl_hdl,
                             pipe_stful_tbl_sync_cback_fn cback_fn,
                             void *cookie));
  MOCK_METHOD5(pipeStfulDirectDatabaseSync,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                             pipe_stful_tbl_sync_cback_fn cback_fn,
                             void *cookie));
  MOCK_METHOD4(pipeStfulQueryGetSizes,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             pipe_stful_tbl_hdl_t stful_tbl_hdl,
                             int *num_pipes));
  MOCK_METHOD4(pipeStfulDirectQueryGetSizes,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                             int *num_pipes));
  MOCK_METHOD6(pipeStfulEntQuery,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_stful_tbl_hdl_t stful_tbl_hdl,
                             pipe_stful_mem_idx_t stful_ent_idx,
                             pipe_stful_mem_query_t *stful_query,
                             uint32_t pipe_api_flags));
  MOCK_METHOD6(pipeStfulDirectEntQuery,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t device_id,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                             pipe_mat_ent_hdl_t mat_ent_hdl,
                             pipe_stful_mem_query_t *stful_query,
                             uint32_t pipe_api_flags));
  MOCK_METHOD4(pipeStfulTableReset,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_stful_tbl_hdl_t stful_tbl_hdl,
                             pipe_stful_mem_spec_t *stful_spec));
  MOCK_METHOD6(pipeStfulTableResetRange,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_stful_tbl_hdl_t stful_tbl_hdl,
                             pipe_stful_mem_idx_t stful_ent_idx,
                             uint32_t num_indices,
                             pipe_stful_mem_spec_t *stful_spec));
  MOCK_METHOD5(pipeStfulParamSet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_tbl_hdl_t tbl_hdl,
                             pipe_reg_param_hdl_t rp_hdl,
                             int64_t value));
  MOCK_METHOD5(pipeStfulParamGet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_tbl_hdl_t tbl_hdl,
                             pipe_reg_param_hdl_t rp_hdl,
                             int64_t *value));
  MOCK_METHOD4(pipeStfulParamReset,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_tbl_hdl_t tbl_hdl,
                             pipe_reg_param_hdl_t rp_hdl));
  MOCK_METHOD3(pipeStfulParamGetHdl,
               pipe_status_t(tdi_dev_id_t dev,
                             const char *name,
                             pipe_reg_param_hdl_t *rp_hdl));
  MOCK_METHOD1(devPortToPipeId, bf_dev_pipe_t(uint16_t dev_port_id));
  MOCK_METHOD4(pipeMgrGetFirstEntryHandle,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             pipe_mat_tbl_hdl_t tbl_hdl,
                             dev_target_t dev_tgt,
                             int *entry_handle));
  MOCK_METHOD6(pipeMgrGetNextEntryHandles,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             pipe_mat_tbl_hdl_t tbl_hdl,
                             dev_target_t dev_tgt,
                             pipe_mat_ent_hdl_t entry_handle,
                             int n,
                             int *next_entry_handles));
  MOCK_METHOD5(pipeMgrGetFirstGroupMember,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             pipe_tbl_hdl_t tbl_hdl,
                             tdi_dev_id_t dev_id,
                             pipe_sel_grp_hdl_t sel_grp_hdl,
                             pipe_adt_ent_hdl_t *mbr_hdl));
  MOCK_METHOD7(pipeMgrGetNextGroupMembers,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             pipe_tbl_hdl_t tbl_hdl,
                             tdi_dev_id_t dev_id,
                             pipe_sel_grp_hdl_t sel_grp_hdl,
                             pipe_adt_ent_hdl_t mbr_hdl,
                             int n,
                             pipe_adt_ent_hdl_t *next_mbr_hdls));
  MOCK_METHOD5(pipeMgrGetSelGrpMbrCount,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t dev_id,
                             pipe_sel_tbl_hdl_t tbl_hdl,
                             pipe_sel_grp_hdl_t sel_grp_hdl,
                             uint32_t *count));
  MOCK_METHOD5(pipeMgrAdtEntHdlGet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_adt_tbl_hdl_t adt_tbl_hdl,
                             pipe_adt_mbr_id_t mbr_id,
                             pipe_adt_ent_hdl_t *adt_ent_hdl));
  MOCK_METHOD6(pipeMgrAdtEntMbrIdGet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             bf_dev_id_t dev_id,
                             pipe_adt_tbl_hdl_t adt_tbl_hdl,
                             pipe_adt_ent_hdl_t adt_ent_hdl,
                             pipe_adt_mbr_id_t *mbr_id,
                             bf_dev_pipe_t *mbr_pipe));
  MOCK_METHOD6(pipeMgrAdtEntDataGet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_adt_tbl_hdl_t adt_tbl_hdl,
                             pipe_adt_mbr_id_t mbr_id,
                             pipe_adt_ent_hdl_t *adt_ent_hdl,
                             pipe_mgr_adt_ent_data_t *ap_ent_data));
  MOCK_METHOD5(pipeMgrSelGrpIdGet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_sel_tbl_hdl_t sel_tbl_hdl,
                             pipe_sel_grp_hdl_t sel_grp_hdl,
                             pipe_sel_grp_id_t *sel_grp_id));
  MOCK_METHOD5(pipeMgrSelGrpHdlGet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_sel_tbl_hdl_t sel_tbl_hdl,
                             pipe_sel_grp_id_t sel_grp_id,
                             pipe_sel_grp_hdl_t *sel_grp_hdl));
  MOCK_METHOD5(pipeMgrSelGrpMaxSizeGet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t dev_id,
                             pipe_sel_tbl_hdl_t tbl_hdl,
                             pipe_sel_grp_hdl_t sel_grp_hdl,
                             uint32_t *max_size));
  MOCK_METHOD4(pipeMgrGetReservedEntryCount,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_mat_tbl_hdl_t tbl_hdl,
                             size_t *count));
  MOCK_METHOD5(pipeMgrGetEntryCount,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_mat_tbl_hdl_t tbl_hdl,
                             bool read_from_hw,
                             uint32_t *count));
  MOCK_METHOD10(pipeMgrGetEntry,
                pipe_status_t(pipe_sess_hdl_t sess_hdl,
                              pipe_mat_tbl_hdl_t tbl_hdl,
                              dev_target_t dev_tgt,
                              pipe_mat_ent_hdl_t entry_hdl,
                              pipe_tbl_match_spec_t *pipe_match_spec,
                              pipe_action_spec_t *pipe_action_spec,
                              pipe_act_fn_hdl_t *act_fn_hdl,
                              bool from_hw,
                              uint32_t res_get_flags,
                              pipe_res_get_data_t *res_data));
  MOCK_METHOD6(pipeMgrGetActionDataEntry,
               pipe_status_t(pipe_adt_tbl_hdl_t tbl_hdl,
                             dev_target_t dev_tgt,
                             pipe_adt_ent_hdl_t entry_hdl,
                             pipe_action_data_spec_t *pipe_action_data_spec,
                             pipe_act_fn_hdl_t *act_fn_hdl,
                             bool from_hw));
  MOCK_METHOD6(pipeMgrTblSetProperty,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t dev_id,
                             pipe_mat_tbl_hdl_t tbl_hdl,
                             pipe_mgr_tbl_prop_type_t property,
                             pipe_mgr_tbl_prop_value_t value,
                             pipe_mgr_tbl_prop_args_t args));
  MOCK_METHOD6(pipeMgrTblGetProperty,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t dev_id,
                             pipe_mat_tbl_hdl_t tbl_hdl,
                             pipe_mgr_tbl_prop_type_t property,
                             pipe_mgr_tbl_prop_value_t *value,
                             pipe_mgr_tbl_prop_args_t *args));
  MOCK_METHOD9(pipeMgrPvsEntryAdd,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t dev_id,
                             pipe_pvs_hdl_t pvs_handle,
                             bf_dev_direction_t gress,
                             bf_dev_pipe_t pipeid,
                             uint8_t parser_id,
                             uint32_t parser_value,
                             uint32_t parser_mask,
                             pipe_pvs_hdl_t *pvs_entry_handle));
  MOCK_METHOD6(pipeMgrPvsEntryModify,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t dev_id,
                             pipe_pvs_hdl_t pvs_handle,
                             pipe_pvs_hdl_t pvs_entry_handle,
                             uint32_t parser_value,
                             uint32_t parser_mask));
  MOCK_METHOD4(pipeMgrPvsEntryDelete,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t dev_id,
                             pipe_pvs_hdl_t pvs_handle,
                             pipe_pvs_hdl_t pvs_entry_handle));
  MOCK_METHOD6(pipeMgrPvsClear,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t dev_id,
                             pipe_pvs_hdl_t pvs_handle,
                             bf_dev_direction_t gress,
                             bf_dev_pipe_t pipeid,
                             uint8_t parser_id));
  MOCK_METHOD9(pipeMgrPvsEntryGet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t dev_id,
                             pipe_pvs_hdl_t pvs_handle,
                             pipe_pvs_hdl_t pvs_entry_handle,
                             uint32_t *parser_value,
                             uint32_t *parser_value_mask,
                             uint8_t *entry_gress,
                             bf_dev_pipe_t *entry_pipe,
                             uint8_t *entry_parser_id));
  MOCK_METHOD9(pipeMgrPvsEntryGetHw,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t dev_id,
                             bf_dev_direction_t gress,
                             bf_dev_pipe_t pipeid,
                             uint8_t parser_id,
                             pipe_pvs_hdl_t pvs_handle,
                             pipe_pvs_hdl_t pvs_entry_handle,
                             uint32_t *parser_value,
                             uint32_t *parser_value_mask));
  MOCK_METHOD9(pipeMgrPvsEntryHandleGet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t dev_id,
                             pipe_pvs_hdl_t pvs_handle,
                             bf_dev_direction_t gress,
                             bf_dev_pipe_t pipeid,
                             uint8_t parser_id,
                             uint32_t parser_value,
                             uint32_t parser_mask,
                             pipe_pvs_hdl_t *pvs_entry_handle));
  MOCK_METHOD7(pipeMgrPvsEntryGetFirst,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t dev_id,
                             pipe_pvs_hdl_t pvs_handle,
                             bf_dev_direction_t gress,
                             bf_dev_pipe_t pipe_id,
                             uint8_t parser_id,
                             pipe_pvs_hdl_t *entry_handle));

  MOCK_METHOD9(pipeMgrPvsEntryGetNext,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t devid,
                             pipe_pvs_hdl_t pvs_handle,
                             bf_dev_direction_t gress,
                             bf_dev_pipe_t pipe_id,
                             uint8_t parser_id,
                             pipe_pvs_hdl_t entry_handle,
                             int n,
                             pipe_pvs_hdl_t *next_handles));

  MOCK_METHOD8(pipeMgrPvsEntryGetCount,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t devid,
                             pipe_pvs_hdl_t pvs_handle,
                             bf_dev_direction_t gress,
                             bf_dev_pipe_t pipe_id,
                             uint8_t parser_id,
                             bool from_hw,
                             uint32_t *count));
  MOCK_METHOD6(pipeMgrPvsSetProperty,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t dev_id,
                             pipe_pvs_hdl_t pvs_handle,
                             pipe_mgr_pvs_prop_type_t property,
                             pipe_mgr_pvs_prop_value_t value,
                             pipe_mgr_pvs_prop_args_t args));
  MOCK_METHOD6(pipeMgrPvsGetProperty,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t dev_id,
                             pipe_pvs_hdl_t pvs_handle,
                             pipe_mgr_pvs_prop_type_t property,
                             pipe_mgr_pvs_prop_value_t *value,
                             pipe_mgr_pvs_prop_args_t args));
  MOCK_METHOD4(pipeMgrHashCalcInputSet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t dev_id,
                             pipe_hash_calc_hdl_t handle,
                             pipe_fld_lst_hdl_t fl_handle));
  MOCK_METHOD3(pipeMgrHashCalcInputDefaultSet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t dev_id,
                             pipe_hash_calc_hdl_t handle));
  MOCK_METHOD4(pipeMgrHashCalcInputGet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t dev_id,
                             pipe_hash_calc_hdl_t handle,
                             pipe_fld_lst_hdl_t *fl_handle));
  MOCK_METHOD6(
      pipeMgrHashCalcInputFieldAttrSet,
      pipe_status_t(pipe_sess_hdl_t sess_hdl,
                    tdi_dev_id_t dev_id,
                    pipe_hash_calc_hdl_t handle,
                    pipe_fld_lst_hdl_t fl_handle,
                    uint32_t attr_count,
                    pipe_hash_calc_input_field_attribute_t *attr_list));
  MOCK_METHOD7(pipeMgrHashCalcInputFieldAttributeGet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t dev_id,
                             pipe_hash_calc_hdl_t handle,
                             pipe_fld_lst_hdl_t fl_handle,
                             uint32_t max_attr_count,
                             pipe_hash_calc_input_field_attribute_t *attr_list,
                             uint32_t *num_attr_filled));
  MOCK_METHOD6(pipeMgrHashCalcInputFieldAttribute2Get,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t dev_id,
                             pipe_hash_calc_hdl_t handle,
                             pipe_fld_lst_hdl_t fl_handle,
                             pipe_hash_calc_input_field_attribute_t **attr_list,
                             uint32_t *num_attr_filled));
  MOCK_METHOD1(
      pipeMgrHashCalcAttributeListDestroy,
      pipe_status_t(pipe_hash_calc_input_field_attribute_t *attr_list));

  MOCK_METHOD5(pipeMgrHashCalcInputFieldAttrCountGet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t dev_id,
                             pipe_hash_calc_hdl_t handle,
                             pipe_fld_lst_hdl_t fl_handle,
                             uint32_t *attr_count));
  MOCK_METHOD6(pipeMgrHashCalcAlgorithmSet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t dev_id,
                             pipe_hash_calc_hdl_t handle,
                             pipe_hash_alg_hdl_t al_handle,
                             const bfn_hash_algorithm_t *algorithm,
                             uint64_t rotate));
  MOCK_METHOD6(pipeMgrHashCalcAlgorithmGet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t dev_id,
                             pipe_hash_calc_hdl_t handle,
                             pipe_hash_alg_hdl_t *al_handle,
                             bfn_hash_algorithm_t *algorithm,
                             uint64_t *rotate));
  MOCK_METHOD3(pipeMgrHashCalcAlgorithmReset,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t dev_id,
                             pipe_hash_calc_hdl_t handle));
  MOCK_METHOD7(pipeMgrHashCalcCalculateHashValueWithCfg,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t dev_id,
                             pipe_hash_calc_hdl_t handle,
                             uint32_t attr_count,
                             pipe_hash_calc_input_field_attribute_t *attrs,
                             uint32_t hash_len,
                             uint8_t *hash));
  MOCK_METHOD4(pipeMgrHashCalcSeedSet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t dev_id,
                             pipe_hash_calc_hdl_t handle,
                             pipe_hash_seed_t seed));
  MOCK_METHOD4(pipeMgrHashCalcSeedGet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t dev_id,
                             pipe_hash_calc_hdl_t handle,
                             pipe_hash_seed_t *seed));
  MOCK_METHOD1(pipeGetHdlPipe, bf_dev_pipe_t(pipe_mat_ent_hdl_t entry_hdl));

  MOCK_METHOD3(bfSnapshotMonitoringMode,
               tdi_status_t(tdi_dev_id_t dev_id,
                            bool interrupt_or_polling,
                            bf_snapshot_triggered_cb trig_cb));
  MOCK_METHOD1(bfSnapshotDoPolling, tdi_status_t(tdi_dev_id_t dev));
  MOCK_METHOD6(bfSnapshotCreate,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            dev_stage_t start_stage,
                            dev_stage_t end_stage,
                            bf_snapshot_dir_t dir,
                            pipe_snapshot_hdl_t *hdl));

  MOCK_METHOD1(bfSnapshotDelete, tdi_status_t(pipe_snapshot_hdl_t hdl));
  MOCK_METHOD1(bfSnapshotClear, tdi_status_t(tdi_dev_id_t dev));
  MOCK_METHOD3(bfSnapshotCaptureTriggerSet,
               tdi_status_t(pipe_snapshot_hdl_t hdl,
                            void *trig_spec,
                            void *trig_mask));
  MOCK_METHOD4(bfSnapshotCaptureTriggerFieldAdd,
               tdi_status_t(pipe_snapshot_hdl_t hdl,
                            char *field_name,
                            uint64_t value,
                            uint64_t mask));
  MOCK_METHOD1(bfSnapshotCaptureTriggerFieldsClr,
               tdi_status_t(pipe_snapshot_hdl_t hdl));
  MOCK_METHOD5(bfSnapshotCaptureGet,
               tdi_status_t(pipe_snapshot_hdl_t hdl,
                            bf_dev_pipe_t pipe,
                            uint8_t *capture,
                            bf_snapshot_capture_ctrl_info_arr_t *ctrl_info_arr,
                            int *num_captures));
  MOCK_METHOD8(bfSnapshotCaptureDecodeFieldValue,
               tdi_status_t(pipe_snapshot_hdl_t hdl,
                            bf_dev_pipe_t pipe,
                            dev_stage_t stage,
                            uint8_t *capture,
                            int num_captures,
                            char *field_name,
                            uint64_t *field_value,
                            bool *field_valid));
  MOCK_METHOD3(bfSnapshotStateSet,
               tdi_status_t(pipe_snapshot_hdl_t hdl,
                            bf_snapshot_state_t state,
                            uint32_t usec));
  MOCK_METHOD3(bfSnapshotStateGet,
               tdi_status_t(pipe_snapshot_hdl_t hdl,
                            bf_dev_pipe_t pipe,
                            int *state));
  MOCK_METHOD3(bfSnapshotCfgSet,
               tdi_status_t(pipe_snapshot_hdl_t hdl,
                            bool timer_disable,
                            bf_snapshot_ig_mode_t mode));
  MOCK_METHOD6(bfSnapshotFieldInScope,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            dev_stage_t stage,
                            bf_snapshot_dir_t dir,
                            char *field_name,
                            bool *exists));
  MOCK_METHOD6(bfSnapshotTriggerFieldInScope,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            dev_stage_t stage,
                            bf_snapshot_dir_t dir,
                            char *field_name,
                            bool *exists));
  MOCK_METHOD6(bfSnapshotHandleGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            dev_stage_t start_stage,
                            dev_stage_t end_stage,
                            bf_snapshot_dir_t *dir,
                            pipe_snapshot_hdl_t *hdl));
  MOCK_METHOD3(bfSnapshotCapturePhvFieldsDictSize,
               tdi_status_t(pipe_snapshot_hdl_t hdl,
                            uint32_t *total_size,
                            uint32_t *per_stage_size));
  MOCK_METHOD2(bfSnapshotTotalPhvCountGet,
               tdi_status_t(tdi_dev_id_t dev_id, uint32_t *count));
  MOCK_METHOD4(bfSnapshotStagesGet,
               tdi_status_t(tdi_dev_id_t dev_id,
                            bf_dev_pipe_t pipe,
                            uint32_t size,
                            int *stages));
  MOCK_METHOD6(bfSnapshotRawCaptureGet,
               tdi_status_t(tdi_dev_id_t dev_id,
                            bf_dev_pipe_t pipe,
                            dev_stage_t stage,
                            uint32_t size,
                            uint32_t *fields,
                            bool *field_v));
  MOCK_METHOD6(bfSnapshotEntryParamsGet,
               tdi_status_t(pipe_snapshot_hdl_t hdl,
                            tdi_dev_id_t *dev,
                            bf_dev_pipe_t *pipe,
                            dev_stage_t *s_stage,
                            dev_stage_t *e_stage,
                            bf_snapshot_dir_t *dir));
  MOCK_METHOD4(bfSnapshotCaptureTriggerFieldGet,
               tdi_status_t(pipe_snapshot_hdl_t hdl,
                            char *field_name,
                            uint64_t *value,
                            uint64_t *mask));
  MOCK_METHOD3(bfSnapshotFirstHandleGet,
               tdi_status_t(tdi_dev_id_t dev_id,
                            bf_dev_pipe_t pipe,
                            pipe_snapshot_hdl_t *entry_hdl));
  MOCK_METHOD4(bfSnapshotNextEntryHandlesGet,
               tdi_status_t(tdi_dev_id_t dev_id,
                            pipe_snapshot_hdl_t hdl,
                            int n,
                            int *next_entry_handles));
  MOCK_METHOD3(bfSnapshotUsageGet,
               tdi_status_t(tdi_dev_id_t dev_id,
                            bf_dev_pipe_t pipe,
                            uint32_t *count));
  MOCK_METHOD4(bfSnapshotStateGet,
               tdi_status_t(pipe_snapshot_hdl_t hdl,
                            uint32_t size,
                            pipe_snapshot_fsm_state_t *fsm_state,
                            bool *state));
  MOCK_METHOD4(bfSnapshotCfgGet,
               tdi_status_t(pipe_snapshot_hdl_t hdl,
                            bool *timer_enabled,
                            uint32_t *usec,
                            bf_snapshot_ig_mode_t *mode));
  MOCK_METHOD3(bfSnapshotThreadGet,
               tdi_status_t(pipe_snapshot_hdl_t hdl,
                            uint32_t size,
                            int *threads));
  MOCK_METHOD2(bfSnapshotNumTrigFieldsGet,
               tdi_status_t(pipe_snapshot_hdl_t hdl, int *count));

  MOCK_METHOD4(bfDbgCounterGet,
               tdi_status_t(bf_dev_target_t dev_tgt,
                            char *tbl_name,
                            bf_tbl_dbg_counter_type_t *type,
                            uint32_t *value));
  MOCK_METHOD5(bfDbgCounterGet,
               tdi_status_t(bf_dev_target_t dev_tgt,
                            uint32_t stage,
                            uint32_t log_tbl,
                            bf_tbl_dbg_counter_type_t *type,
                            uint32_t *value));
  MOCK_METHOD3(bfDbgCounterSet,
               tdi_status_t(bf_dev_target_t dev_tgt,
                            char *tbl_name,
                            bf_tbl_dbg_counter_type_t type));
  MOCK_METHOD4(bfDbgCounterSet,
               tdi_status_t(bf_dev_target_t dev_tgt,
                            uint32_t stage,
                            uint32_t log_tbl,
                            bf_tbl_dbg_counter_type_t type));
  MOCK_METHOD2(bfDbgCounterClear,
               tdi_status_t(bf_dev_target_t dev_tgt, char *tbl_name));
  MOCK_METHOD3(bfDbgCounterClear,
               tdi_status_t(bf_dev_target_t dev_tgt,
                            uint32_t stage,
                            uint32_t log_tbl));
  MOCK_METHOD3(bfDbgCounterTableListGet,
               tdi_status_t(bf_dev_target_t dev_tgt,
                            char **tbl_list,
                            int *num_tbls));

  MOCK_METHOD6(pipeMgrHitlessHaRestoreVirtualDevState,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             tdi_dev_id_t dev_id,
                             pipe_tbl_hdl_t tbl_hdl,
                             struct pipe_plcmt_info *info,
                             uint32_t *processed,
                             pd_ha_restore_cb_1 cb1));
  MOCK_METHOD6(pipeMgrLpfReadEntryIdx,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_lpf_tbl_hdl_t lpf_tbl_hdl,
                             pipe_lpf_idx_t index,
                             pipe_lpf_spec_t *lpf_spec,
                             bool from_hw));
  MOCK_METHOD5(pipeMgrLpfReadEntry,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                             pipe_mat_ent_hdl_t mat_ent_hdl,
                             pipe_lpf_spec_t *lpf_spec));
  MOCK_METHOD5(pipeMgrWredReadEntry,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                             pipe_mat_ent_hdl_t mat_ent_hdl,
                             pipe_wred_spec_t *wred_spec));
  MOCK_METHOD6(pipeMgrWredReadEntSet,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_meter_tbl_hdl_t meter_tbl_hdl,
                             pipe_wred_idx_t red_index,
                             pipe_wred_spec_t *wred_spec,
                             uint32_t pipe_api_flags));
  MOCK_METHOD6(pipeMgrWredReadEntryIdx,
               pipe_status_t(pipe_sess_hdl_t sess_hdl,
                             dev_target_t dev_tgt,
                             pipe_wred_tbl_hdl_t wred_tbl_hdl,
                             pipe_wred_idx_t index,
                             pipe_wred_spec_t *wred_spec,
                             bool from_hw));

  MOCK_CONST_METHOD5(pipeRegisterMatUpdateCb,
                     tdi_status_t(pipe_sess_hdl_t sess_hdl,
                                  tdi_dev_id_t device_id,
                                  pipe_mat_tbl_hdl_t tbl_hdl,
                                  pipe_mat_update_cb cb,
                                  void *cookie));
  MOCK_CONST_METHOD5(pipeRegisterAdtUpdateCb,
                     tdi_status_t(pipe_sess_hdl_t sess_hdl,
                                  tdi_dev_id_t device_id,
                                  pipe_adt_tbl_hdl_t tbl_hdl,
                                  pipe_adt_update_cb cb,
                                  void *cookie));
  MOCK_CONST_METHOD5(pipeRegisterSelUpdateCb,
                     tdi_status_t(pipe_sess_hdl_t sess_hdl,
                                  tdi_dev_id_t device_id,
                                  pipe_sel_tbl_hdl_t tbl_hdl,
                                  pipe_sel_update_cb cb,
                                  void *cookie));

  MOCK_METHOD2(pipeSetAdtEntHdlInMatData,
               pipe_status_t(void *data, pipe_adt_ent_hdl_t adt_ent_hdl));
  MOCK_METHOD2(pipeSetSelGrpHdlInMatData,
               pipe_status_t(void *data, pipe_adt_ent_hdl_t sel_grp_hdl));
  MOCK_METHOD2(pipeSetTtlInMatData, pipe_status_t(void *data, uint32_t ttl));
  MOCK_METHOD2(pipeMgrTcamScrubTimerSet,
               pipe_status_t(tdi_dev_id_t dev, uint32_t msec_timer));
  MOCK_METHOD1(pipeMgrTcamScrubTimerGet, uint32_t(tdi_dev_id_t dev));
  MOCK_CONST_METHOD4(pipeMgrTblHdlPipeMaskGet,
                     tdi_status_t(tdi_dev_id_t dev_id,
                                  const std::string &prog_name,
                                  const std::string &pipeline_name,
                                  uint32_t *pipe_mask));
  MOCK_CONST_METHOD2(pipeMgrGetNumPipelines,
                     pipe_status_t(tdi_dev_id_t dev_id, uint32_t *num_pipes));

  MOCK_METHOD2(pipeMgrLrtDrTimeoutSet,
               pipe_status_t(tdi_dev_id_t dev_id, uint32_t timeout_ms));

  MOCK_CONST_METHOD2(pipeMgrLrtDrTimeoutGet,
                     pipe_status_t(tdi_dev_id_t dev_id, uint32_t *timeout_ms));

  MOCK_METHOD4(pipeMgrRecirEnableSet,
               tdi_status_t(pipe_sess_hdl_t sess_hdl,
                            tdi_dev_id_t dev_id,
                            bf_dev_port_t dev_port,
                            bool enable));

  MOCK_CONST_METHOD3(pipeMgrRecirEnableGet,
                     tdi_status_t(tdi_dev_id_t dev_id,
                                  bf_dev_port_t dev_port,
                                  bool *enable));

  MOCK_METHOD4(pipeMgrPktgenEnableSet,
               tdi_status_t(pipe_sess_hdl_t sess_hdl,
                            tdi_dev_id_t dev_id,
                            bf_dev_port_t dev_port,
                            bool enable));

  MOCK_CONST_METHOD4(pipeMgrPktgenEnableGet,
                     tdi_status_t(pipe_sess_hdl_t sess_hdl,
                                  tdi_dev_id_t dev_id,
                                  bf_dev_port_t dev_port,
                                  bool *enable));

  MOCK_METHOD4(pipeMgrPktgenRecirPatternMatchingEnableSet,
               tdi_status_t(pipe_sess_hdl_t sess_hdl,
                            tdi_dev_id_t dev_id,
                            bf_dev_port_t dev_port,
                            bool enable));

  MOCK_CONST_METHOD4(pipeMgrPktgenRecirPatternMatchingEnableGet,
                     tdi_status_t(pipe_sess_hdl_t sess_hdl,
                                  tdi_dev_id_t dev_id,
                                  bf_dev_port_t dev_port,
                                  bool *enable));

  MOCK_METHOD3(pipeMgrPktgenClearPortDownSet,
               tdi_status_t(pipe_sess_hdl_t sess_hdl,
                            tdi_dev_id_t dev_id,
                            bf_dev_port_t dev_port));

  MOCK_METHOD4(pipeMgrPktgenClearPortDownGet,
               tdi_status_t(pipe_sess_hdl_t sess_hdl,
                            tdi_dev_id_t dev_id,
                            bf_dev_port_t dev_port,
                            bool *cleared));

  MOCK_METHOD4(pipeMgrPktgenPortDownMaskSet,
               tdi_status_t(pipe_sess_hdl_t sess_hdl,
                            bf_dev_target_t dev_tgt,
                            uint32_t port_mask_sel,
                            struct bf_tof2_port_down_sel *mask));

  MOCK_CONST_METHOD3(pipeMgrPktgenPortDownMaskGet,
                     tdi_status_t(bf_dev_target_t dev_tgt,
                                  uint32_t port_mask_sel,
                                  struct bf_tof2_port_down_sel *mask));

  MOCK_METHOD3(pipeMgrPktgenPortDownReplayModeSet,
               tdi_status_t(pipe_sess_hdl_t sess_hdl,
                            bf_dev_target_t dev_tgt,
                            bf_pktgen_port_down_mode_t mode));

  MOCK_CONST_METHOD3(pipeMgrPktgenPortDownReplayModeGet,
                     tdi_status_t(pipe_sess_hdl_t sess_hdl,
                                  bf_dev_target_t dev_tgt,
                                  bf_pktgen_port_down_mode_t *mode));

  MOCK_METHOD4(pipeMgrPktgenAppSet,
               tdi_status_t(pipe_sess_hdl_t sess_hdl,
                            bf_dev_target_t dev_tgt,
                            int app_id,
                            bf_pktgen_app_cfg_t *cfg));

  MOCK_CONST_METHOD4(pipeMgrPktgenAppGet,
                     tdi_status_t(pipe_sess_hdl_t sess_hdl,
                                  bf_dev_target_t dev_tgt,
                                  int app_id,
                                  bf_pktgen_app_cfg_t *cfg));

  MOCK_METHOD4(pipeMgrPktgenAppEnableSet,
               tdi_status_t(pipe_sess_hdl_t sess_hdl,
                            bf_dev_target_t dev_tgt,
                            int app_id,
                            bool enable));

  MOCK_CONST_METHOD4(pipeMgrPktgenAppEnableGet,
                     tdi_status_t(pipe_sess_hdl_t sess_hdl,
                                  bf_dev_target_t dev_tgt,
                                  int app_id,
                                  bool *enable));

  MOCK_METHOD5(pipeMgrPktgenWritePktBuffer,
               tdi_status_t(pipe_sess_hdl_t sess_hdl,
                            bf_dev_target_t dev_tgt,
                            uint32_t pktgen_byte_buf_offset,
                            uint32_t size,
                            const uint8_t *buf));

  MOCK_CONST_METHOD5(pipeMgrPktgenPktBufferGet,
                     tdi_status_t(pipe_sess_hdl_t sess_hdl,
                                  bf_dev_target_t dev_tgt,
                                  uint32_t pktgen_byte_buf_offset,
                                  uint32_t size,
                                  uint8_t *buf));

  MOCK_METHOD4(pipeMgrPktgenBatchCntSet,
               tdi_status_t(pipe_sess_hdl_t sess_hdl,
                            bf_dev_target_t dev_tgt,
                            int app_id,
                            uint64_t batch_cnt));
  MOCK_METHOD4(pipeMgrPktgenPktCntSet,
               tdi_status_t(pipe_sess_hdl_t sess_hdl,
                            bf_dev_target_t dev_tgt,
                            int app_id,
                            uint64_t pkt_cnt));

  MOCK_METHOD4(pipeMgrPktgenTriggerCntSet,
               tdi_status_t(pipe_sess_hdl_t sess_hdl,
                            bf_dev_target_t dev_tgt,
                            int app_id,
                            uint64_t trigger_cnt));
  MOCK_CONST_METHOD4(pipeMgrPktgenBatchCntGet,
                     tdi_status_t(pipe_sess_hdl_t sess_hdl,
                                  bf_dev_target_t dev_tgt,
                                  int app_id,
                                  uint64_t *batch_cnt));

  MOCK_CONST_METHOD4(pipeMgrPktgenPktCntGet,
                     tdi_status_t(pipe_sess_hdl_t sess_hdl,
                                  bf_dev_target_t dev_tgt,
                                  int app_id,
                                  uint64_t *pkt_cnt));

  MOCK_CONST_METHOD4(pipeMgrPktgenTriggerCntGet,
                     tdi_status_t(pipe_sess_hdl_t sess_hdl,
                                  bf_dev_target_t dev_tgt,
                                  int app_id,
                                  uint64_t *trigger_cnt));

  MOCK_CONST_METHOD1(pipeMgrPktgenAppCountGet, uint32_t(tdi_dev_id_t dev));

  MOCK_CONST_METHOD1(pipeMgrPktgenPortGet, bf_dev_port_t(tdi_dev_id_t dev_id));

  MOCK_CONST_METHOD2(pipeMgrPktgenPortGetNext,
                     tdi_status_t(tdi_dev_id_t dev_id, bf_dev_port_t *port));

  MOCK_CONST_METHOD1(pipeMgrPktgenMaxPortGet,
                     bf_dev_port_t(tdi_dev_id_t dev_id));

  MOCK_METHOD5(pipeMgrMirrorSessionSet,
               tdi_status_t(pipe_sess_hdl_t sess_hdl,
                            dev_target_t dev_target,
                            bf_mirror_id_t sid,
                            bf_mirror_session_info_t *s_info,
                            bool enable));

  MOCK_METHOD3(pipeMgrMirrorSessionReset,
               tdi_status_t(pipe_sess_hdl_t sess_hdl,
                            dev_target_t dev_target,
                            bf_mirror_id_t sid));

  MOCK_METHOD4(pipeMgrMirrorSessionEnable,
               tdi_status_t(pipe_sess_hdl_t sess_hdl,
                            dev_target_t dev_target,
                            bf_mirror_direction_e dir,
                            bf_mirror_id_t sid));

  MOCK_METHOD4(pipeMgrMirrorSessionDisable,
               tdi_status_t(pipe_sess_hdl_t sess_hdl,
                            dev_target_t dev_target,
                            bf_mirror_direction_e dir,
                            bf_mirror_id_t sid));

  MOCK_METHOD5(pipeMgrMirrorSessionMetaFlagUpdate,
               tdi_status_t(pipe_sess_hdl_t sess_hdl,
                            dev_target_t dev_target,
                            bf_mirror_id_t sid,
                            bf_mirror_meta_flag_e mirror_flag,
                            bool value));

  MOCK_METHOD5(pipeMgrMirrorSessionMetaFlagGet,
               tdi_status_t(pipe_sess_hdl_t sess_hdl,
                            dev_target_t dev_target,
                            bf_mirror_id_t sid,
                            bf_mirror_meta_flag_e mirror_flag,
                            bool *value));

  MOCK_METHOD4(pipeMgrMirrorSessionPriorityUpdate,
               tdi_status_t(pipe_sess_hdl_t sess_hdl,
                            dev_target_t dev_target,
                            bf_mirror_id_t sid,
                            bool value));

  MOCK_METHOD4(pipeMgrMirrorSessionPriorityGet,
               tdi_status_t(pipe_sess_hdl_t sess_hdl,
                            dev_target_t dev_target,
                            bf_mirror_id_t sid,
                            bool *value));

  MOCK_METHOD4(pipeMgrMirrorSessionCoalModeUpdate,
               tdi_status_t(pipe_sess_hdl_t sess_hdl,
                            dev_target_t dev_target,
                            bf_mirror_id_t sid,
                            bool value));

  MOCK_METHOD4(pipeMgrMirrorSessionCoalModeGet,
               tdi_status_t(pipe_sess_hdl_t sess_hdl,
                            dev_target_t dev_target,
                            bf_mirror_id_t sid,
                            bool *value));

  MOCK_CONST_METHOD4(pipeMgrMirrorSessionGet,
                     tdi_status_t(pipe_sess_hdl_t sess_hdl,
                                  dev_target_t dev_target,
                                  bf_mirror_id_t sid,
                                  bf_mirror_session_info_t *s_info));
  MOCK_CONST_METHOD4(pipeMgrMirrorSessionEnableGet,
                     tdi_status_t(pipe_sess_hdl_t sess_hdl,
                                  dev_target_t dev_target,
                                  bf_mirror_id_t sid,
                                  bool *session_enable));

  MOCK_CONST_METHOD4(pipeMgrMirrorSessionGetFirst,
                     tdi_status_t(pipe_sess_hdl_t sess_hdl,
                                  dev_target_t dev_target,
                                  bf_mirror_session_info_t *s_info,
                                  bf_mirror_get_id_t *first));

  MOCK_CONST_METHOD5(pipeMgrMirrorSessionGetNext,
                     tdi_status_t(pipe_sess_hdl_t sess_hdl,
                                  dev_target_t dev_target,
                                  bf_mirror_get_id_t current,
                                  bf_mirror_session_info_t *next_info,
                                  bf_mirror_get_id_t *next));

  MOCK_CONST_METHOD3(pipeMgrMirrorSessionCountGet,
                     tdi_status_t(pipe_sess_hdl_t sess_hdl,
                                  dev_target_t dev_target,
                                  uint32_t *count));

  // MockIPipeMgrIntfHelper &getMockIPipeMgrIntfHelper() { return helper; }

 private:
  // MockIPipeMgrIntfHelper helper;
};

}  // namespace tofino_test
}  // namespace tofino
}  // namespace tna
}  // namespace tdi

#endif  // _TDI_PIPE_MGR_INTERFACE_MOCK_HPP
