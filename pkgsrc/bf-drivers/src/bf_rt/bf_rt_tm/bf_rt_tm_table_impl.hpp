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


#ifndef _BF_RT_TM_TABLE_IMPL_HPP
#define _BF_RT_TM_TABLE_IMPL_HPP

#include <bf_rt_common/bf_rt_state.hpp>
#include <bf_rt_common/bf_rt_table_impl.hpp>
#include "bf_rt_tm_intf.hpp"
#include "bf_rt_tm_state.hpp"
#include "bf_rt_tm_table_data_impl.hpp"
#include "bf_rt_tm_table_key_impl.hpp"

namespace {
// This map of application pool strings to enums is used to convert
// the string values passed to BF-RT to the respective enums and call
// the TM APIs used for configuring those pools. The order in this map
// does not really matter as it is just used for lookups.
const static std::map<std::string, bf_tm_app_pool_t> appPoolMap = {
    {"IG_APP_POOL_0", BF_TM_IG_APP_POOL_0},
    {"IG_APP_POOL_1", BF_TM_IG_APP_POOL_1},
    {"IG_APP_POOL_2", BF_TM_IG_APP_POOL_2},
    {"IG_APP_POOL_3", BF_TM_IG_APP_POOL_3},












    {"EG_APP_POOL_0", BF_TM_EG_APP_POOL_0},
    {"EG_APP_POOL_1", BF_TM_EG_APP_POOL_1},
    {"EG_APP_POOL_2", BF_TM_EG_APP_POOL_2},
    {"EG_APP_POOL_3", BF_TM_EG_APP_POOL_3},












};

// These are the special pool strings used to identify the special pools over
// the application pools by looking up in this vector. The order here does not
// matter.
const static std::vector<std::string> specialPools = {
    "IG_SKID_POOL",
    "IG_EG_NEGATIVE_MIRROR_POOL",
    "EG_PRE_FIFO",
    "IG_EG_GUARANTEED_MIN",
    "EG_UNICAST_CUT_THROUGH",
    "EG_MULTICAST_CUT_THROUGH"};

// This map of color strings to enums is used to convert the string
// values passed to BF-RT to the respective enums and call
// the TM APIs used for configuring those color related properties.
// The order in this map does not really matter as it is just used for
// lookups.
const static std::map<std::string, bf_tm_color_t> colorMap = {
    {"GREEN", BF_TM_COLOR_GREEN},
    {"YELLOW", BF_TM_COLOR_YELLOW},
    {"RED", BF_TM_COLOR_RED}};

// This map of ingress application pool strings to enums is used to
// convert the string values passed to BF-RT to the respective enums
// and call the TM APIs used for configuring those pools. The order
// in this map does not really matter as it is just used for lookups.
const static std::map<std::string, bf_tm_app_pool_t> igAppPoolMap = {
    {"IG_APP_POOL_0", BF_TM_IG_APP_POOL_0},
    {"IG_APP_POOL_1", BF_TM_IG_APP_POOL_1},
    {"IG_APP_POOL_2", BF_TM_IG_APP_POOL_2},
    {"IG_APP_POOL_3", BF_TM_IG_APP_POOL_3},












};
}  // namespace

namespace bfrt {

#define BFRT_TM_DEBUG (bf_sys_log_is_log_enabled(BF_MOD_BFRT, BF_LOG_DBG) == 1)

class BfRtTMTable : public BfRtTableObj {
 public:
  BfRtTMTable(const std::string &program_name,
              bf_rt_id_t id,
              std::string name,
              const size_t &size,
              const TableType &type,
              const std::set<TableApi> table_apis)
      : BfRtTableObj(program_name, id, name, size, type, table_apis) {}

  ~BfRtTMTable() = default;

  bf_status_t dataAllocate(
      std::unique_ptr<BfRtTableData> *data_ret) const override final;

  bf_status_t dataAllocate(
      const std::vector<bf_rt_id_t> &fields,
      std::unique_ptr<BfRtTableData> *data_ret) const override final;

  bf_status_t dataReset(BfRtTableData *data) const override final;

  bf_status_t dataAllocate(
      const bf_rt_id_t &action_id,
      std::unique_ptr<BfRtTableData> *data_ret) const override final;

  bf_status_t dataAllocate(
      const std::vector<bf_rt_id_t> &fields,
      const bf_rt_id_t &action_id,
      std::unique_ptr<BfRtTableData> *data_ret) const override final;

  bf_status_t dataReset(const bf_rt_id_t &action_id,
                        BfRtTableData *data) const override final;

  bf_status_t dataReset(const std::vector<bf_rt_id_t> &fields,
                        const bf_rt_id_t &action_id,
                        BfRtTableData *data) const override final;

  bf_status_t tableDefaultEntryGet(const BfRtSession &session,
                                   const bf_rt_target_t &dev_tgt,
                                   const uint64_t &flags,
                                   BfRtTableData *data) const override;

  bf_status_t tableDefaultEntrySet(const BfRtSession &session,
                                   const bf_rt_target_t &dev_tgt,
                                   const uint64_t &flags,
                                   const BfRtTableData &t_data) const override;

  bf_status_t tableDefaultEntryReset(const BfRtSession &session,
                                     const bf_rt_target_t &dev_tgt,
                                     const uint64_t &flags) const override;

  bf_status_t tableEntryGetFirst(const BfRtSession &session,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flags,
                                 BfRtTableKey *key,
                                 BfRtTableData *data) const override;

  bf_status_t tableEntryGetNext_n(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const BfRtTableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override;

  // Returns True if the table has a data field with expected name and id.
  bool dataFieldDeclared(const std::string &expected_name,
                         const bf_rt_id_t action_id,
                         const bf_rt_id_t expected_id) const;

  // The function checks the field_name is legitimate for this table and
  // looks for its presence in the wrk_fields provided. If the field id
  // is present there, it is popped out from wrk_fields to field_id,
  // otherwise zero value means it is not found.
  bf_status_t popWorkField(const std::string &field_name,
                           const bf_rt_id_t action_id,
                           std::set<bf_rt_id_t> &wrk_fields,
                           bf_rt_id_t &field_id) const;

  /**
   * @brief Set the key to point to the very first table entry.
   *
   * @param[inout] dev_tgt Device with pipe_id as implicit key.
   * @param[inout] key     Key object to set.
   *
   * @return BF_SUCCESS On dev_tgt and pKey updated.
   */
  virtual bf_status_t keyFirst(bf_rt_target_t &dev_tgt,
                               BfRtTableKey *key) const;

  /**
   * @brief Advance the key to point to the next table entry.
   *
   * @param[inout] dev_tgt   Device with pipe_id as implicit key.
   * @param[in]    key       Key object.
   * @param[out]   next_key  Next key object.
   *
   * @return BF_SUCCESS           On dev_tgt and pKey updated.
   * @return BF_OBJECT_NOT_FOUND  On no more entries.
   */
  virtual bf_status_t keyNext(bf_rt_target_t &dev_tgt,
                              const BfRtTableKey &key,
                              BfRtTableKey *next_key) const;

 protected:
  virtual bf_status_t tableGetDefaultField(const bf_rt_target_t &dev_tgt,
                                           bf_rt_id_t data_id,
                                           BfRtTMTableData *t_data) const;

  virtual bf_status_t tableGetDefaultEntry(const bf_rt_target_t &dev_tgt,
                                           BfRtTMTableData *t_data) const;

  // Gets a group of related fields at once and removes ids from the given list.
  virtual bf_status_t tableGetDefaultFields(
      const bf_rt_target_t &dev_tgt,
      BfRtTMTableData *t_data,
      std::set<bf_rt_id_t> &wrkFields) const;

  virtual bf_status_t tableSetDefaultEntry(const bf_rt_target_t &dev_tgt,
                                           const BfRtTMTableData &t_data) const;

  virtual bf_status_t tableSetDefaultField(const bf_rt_target_t &dev_tgt,
                                           const BfRtTMTableData &p_data,
                                           bf_rt_id_t data_id) const;

  // Sets a group of related fields at once and removes ids from the given list.
  virtual bf_status_t tableSetDefaultFields(
      const bf_rt_target_t &dev_tgt,
      const BfRtTMTableData &p_data,
      std::set<bf_rt_id_t> &wrkFields) const;

  //--
  virtual bf_status_t tableResetEntry(const bf_rt_target_t &dev_tgt,
                                      BfRtTMTableData *t_data) const;

  virtual bf_status_t tableGetResetValue(const bf_rt_target_t &dev_tgt,
                                         bf_rt_id_t data_id,
                                         BfRtTMTableData *t_data) const;

  // Gets a group of related fields at once and removes ids from the given list.
  virtual bf_status_t tableGetResetValues(
      const bf_rt_target_t &dev_tgt,
      BfRtTMTableData *t_data,
      std::set<bf_rt_id_t> &wrkFields) const;

  // A volatile Table has all its entry fields mutually independent,
  // or if they may change beyond the BFRT TM control.
  // The table-level entry_lock is not needed, but resource lock or state
  // lock might still need to keep the entry id from delete.
  virtual bool isVolatile() const { return false; };

  // The default entry can't change neither with API, nor from HW.
  // No entry_lock is needed to get it.
  virtual bool isConstDefault() const { return true; };

 protected:
  // This lock is to keep the entry fields in mutually consistent state.
  // Each table API uses it when it needs to call several TM APIs for
  // the entry fields with related semantics, and to serialize operations
  // on the table.
  mutable std::mutex entry_lock;

 protected:
  bf_status_t singlePipe_validate(const bf_rt_target_t &dev_tgt) const;

  bf_status_t singleOrPipeAll_validate(const bf_rt_target_t &dev_tgt) const;

  bf_status_t tmDevCfgGet(bf_dev_id_t dev_id,
                          uint8_t *mau_pipes,
                          uint8_t *pg_per_pipe,
                          uint8_t *queues_per_pg,
                          uint8_t *ports_per_pg) const;

  bf_status_t workPipesGet(const bf_rt_target_t &dev_tgt,
                           uint8_t *cnt_pipes) const;

  bf_status_t splitDevPort(const bf_rt_target_t &dev_tgt,
                           bf_dev_port_t port_id,
                           bf_tm_pg_t &pg_id,
                           uint8_t &pg_port_nr) const;
};
}  // namespace bfrt
#endif
