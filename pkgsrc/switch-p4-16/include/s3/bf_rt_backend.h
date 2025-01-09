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


#ifndef INCLUDE_S3_BF_RT_BACKEND_H__
#define INCLUDE_S3_BF_RT_BACKEND_H__

#include <vector>
#include <memory>
#include <string>
#include <utility>

#include "bf_rt/bf_rt_common.h"
#include "bf_rt/bf_rt_init.hpp"
#include "bf_rt/bf_rt_info.hpp"
#include "bf_rt/bf_rt_table.hpp"
#include "bf_rt/bf_rt_table_key.hpp"
#include "bf_rt/bf_rt_table_data.hpp"
#include "bf_rt/bf_rt_learn.hpp"
#include "bf_rt/bf_rt_table_attributes.hpp"
#include "bf_rt/bf_rt_table_operations.hpp"
#include "s3/factory.h"

extern "C" {
#include <tofino/pdfixed/pd_common.h>
}

constexpr const char *SWITCH_P4_FIXED_PIPELINE_NAME = "pipe";
typedef bf_rt_id_t bf_rt_table_id_t;
typedef bf_rt_id_t bf_rt_field_id_t;
typedef bf_rt_id_t bf_rt_action_id_t;

using container_data_map_t = std::unordered_map<bf_rt_id_t, uint64_t>;
typedef struct _bfrt_container_data {
  bf_rt_id_t container_id;
  container_data_map_t container_data_map;
} bfrt_container_data_t;

namespace smi {
namespace bf_rt {

using ::bfrt::BfRtSession;
using ::bfrt::BfRtInfo;
using ::bfrt::BfRtTable;
using ::bfrt::BfRtTableKey;
using ::bfrt::BfRtTableData;
using ::bfrt::GressTarget;

switch_status_t inline static bf_rt_status_xlate(bf_status_t bf_status) {
  switch (bf_status) {
    case BF_SUCCESS:
      return SWITCH_STATUS_SUCCESS;
    case BF_INVALID_ARG:
      return SWITCH_STATUS_INVALID_PARAMETER;
    case BF_NOT_SUPPORTED:
      return SWITCH_STATUS_NOT_SUPPORTED;
    case BF_NO_SYS_RESOURCES:
    case BF_NO_SPACE:
      return SWITCH_STATUS_INSUFFICIENT_RESOURCES;
    case BF_ALREADY_EXISTS:
      return SWITCH_STATUS_ITEM_ALREADY_EXISTS;
    case BF_OBJECT_NOT_FOUND:
      return SWITCH_STATUS_ITEM_NOT_FOUND;
    case BF_NOT_IMPLEMENTED:
      return SWITCH_STATUS_NOT_IMPLEMENTED;
    case BF_HW_COMM_FAIL:
      return SWITCH_STATUS_HW_FAILURE;
    default:
      return SWITCH_STATUS_FAILURE;
  }
}

const BfRtInfo *get_bf_rt_info();
switch_status_t switch_bf_rt_init();
switch_status_t switch_bf_rt_clean();
switch_status_t switch_bf_rt_flush();

const BfRtSession &get_bf_rt_session();
const std::shared_ptr<BfRtSession> get_bf_rt_session_ptr();
bf_rt_target_t get_dev_tgt();
p4_pd_sess_hdl_t get_mc_sess_hdl();

/**
 * @brief Get Active pipes
 *
 * @return Set of all active pipes
 */
const std::set<bf_dev_pipe_t> get_active_pipes();

switch_status_t start_transaction();
switch_status_t commit_transaction();
switch_status_t abort_transaction();
switch_status_t start_batch();
switch_status_t end_batch();
switch_status_t start_batch(std::shared_ptr<BfRtSession> user_session);
switch_status_t end_batch(std::shared_ptr<BfRtSession> user_session);

class _bfrtCacheObject {
 public:
  _bfrtCacheObject(bf_rt_table_id_t table_id,
                   const bf_rt_target_t table_dev_tgt,
                   const BfRtTableKey &table_key,
                   const BfRtTableData &table_data);
  _bfrtCacheObject(bf_rt_table_id_t table_id,
                   const bf_rt_target_t table_dev_tgt,
                   const BfRtTableData &table_data);
  switch_status_t flush();
  switch_status_t defaultEntryFlush();

 private:
  const BfRtTable *table = NULL;
  bf_rt_target_t dev_tgt;
  std::unique_ptr<BfRtTableKey> key;
  std::unique_ptr<BfRtTableData> data;
  void setKey(const BfRtTableKey &table_key);
  void setData(const BfRtTableData &table_data);
  void printKey();
};

typedef std::shared_ptr<_bfrtCacheObject> bfrtCacheObject;
class bfrtCache {
 private:
  bfrtCache(){};
  std::vector<bfrtCacheObject> m_default_entry_cache;
  std::vector<bfrtCacheObject> m_table_cache;
  std::unordered_map<uint64_t, bfrtCacheObject> m_p4_match_action_cache;
  std::unordered_map<uint64_t, std::vector<bfrtCacheObject>>
      m_p4_match_action_list_cache;
  std::unordered_map<uint64_t, bfrtCacheObject> m_p4_selector_cache;
  std::unordered_map<uint64_t, std::vector<bfrtCacheObject>>
      m_p4_selector_list_cache;
  std::unordered_map<uint64_t, bfrtCacheObject> m_p4_selector_group_cache;
  std::unordered_map<uint8_t, bfrtCacheObject> m_traffic_class_cache;
  std::vector<bfrtCacheObject> m_pd_fixed_cache;

 public:
  static bfrtCache &cache() {
    static bfrtCache cache;
    return cache;
  }
  bfrtCache(bfrtCache const &) = delete;
  void operator=(bfrtCache const &) = delete;

  std::vector<bfrtCacheObject> &default_entry_cache() {
    return m_default_entry_cache;
  }
  std::vector<bfrtCacheObject> &table_cache() { return m_table_cache; }
  std::unordered_map<uint64_t, bfrtCacheObject> &p4_match_action_cache() {
    return m_p4_match_action_cache;
  }
  std::unordered_map<uint64_t, std::vector<bfrtCacheObject>>
      &p4_match_action_list_cache() {
    return m_p4_match_action_list_cache;
  }
  std::unordered_map<uint64_t, bfrtCacheObject> &p4_selector_cache() {
    return m_p4_selector_cache;
  }
  std::unordered_map<uint64_t, std::vector<bfrtCacheObject>>
      &p4_selector_list_cache() {
    return m_p4_selector_list_cache;
  }
  std::unordered_map<uint64_t, bfrtCacheObject> &p4_selector_group_cache() {
    return m_p4_selector_group_cache;
  }
  std::unordered_map<uint8_t, bfrtCacheObject> &traffic_class_cache() {
    return m_traffic_class_cache;
  }
  std::vector<bfrtCacheObject> &pd_fixed_cache() { return m_pd_fixed_cache; }
};

/**
 * _MatchKey
 *
 * Wrapper class to simplify BfRtTableKey object operations.
 * Use set_exact(bf_rt_field_id_t, char*, size_t) for >64bit match key fields.
 * For <=64bit fields, the other routines should be used.
 * Applies to set_ternary, set_lpm and set_range also
 */
class _MatchKey {
  friend class _Table;

 public:
  _MatchKey(bf_rt_table_id_t table_id);
  ~_MatchKey() {}

  _MatchKey(const _MatchKey &);                  // copy constructor
  _MatchKey &operator=(const _MatchKey &);       // copy assignment op
  _MatchKey(_MatchKey &&) = default;             // move constructor
  _MatchKey &operator=(_MatchKey &&) = default;  // move assignment op

  switch_status_t get_exact(bf_rt_field_id_t field_id, uint64_t *key) const;
  template <typename T>
  switch_status_t set_exact(bf_rt_field_id_t f_id, T key);
  switch_status_t set_exact(bf_rt_field_id_t f_id, const char *key, size_t s);
  switch_status_t set_exact(const bf_rt_field_id_t field_id,
                            const switch_object_id_t oid,
                            const switch_attr_id_t attr_id);
  template <typename T>
  switch_status_t set_ternary(bf_rt_field_id_t field_id, T key, T mask);
  switch_status_t set_ternary(bf_rt_field_id_t field_id,
                              const char *key,
                              const char *mask,
                              size_t sz);
  switch_status_t set_ternary(bf_rt_field_id_t field_id,
                              const switch_ip_address_t addr,
                              const switch_ip_address_t mask);
  switch_status_t set_ip_unified_ternary(bf_rt_field_id_t field_id,
                                         const switch_ip_address_t addr,
                                         const switch_ip_address_t mask);
  switch_status_t set_ternary(bf_rt_field_id_t field_id,
                              const switch_mac_addr_t addr,
                              const switch_mac_addr_t mask);
  template <typename T>
  switch_status_t get_ternary(bf_rt_field_id_t field_id, T *key, T *mask) const;
  template <typename T>
  switch_status_t set_lpm(bf_rt_field_id_t field_id, T key, size_t len);
  switch_status_t set_lpm(bf_rt_field_id_t field_id,
                          const char *key,
                          size_t sz,
                          size_t len);
  switch_status_t set_range(const bf_rt_field_id_t field_id,
                            const switch_range_t range);

  void reset() {
    if (table) table->keyReset(table_key.get());
  }

 private:
  bf_rt_table_id_t _table_id = 0;
  const BfRtTable *table = NULL;
  std::unique_ptr<BfRtTableKey> table_key;
};

/**
 * _ActionEntry
 *
 * Wrapper class to simplify BfRtTableData object operations.
 * Use set_arg(bf_rt_field_id_t, char*, size_t) for >64bit match key fields.
 * For <=64bit fields, the other routines should be used.
 */
class _ActionEntry {
  friend class _Table;

 public:
  _ActionEntry(bf_rt_table_id_t table_id);
  ~_ActionEntry() {}

  _ActionEntry(const _ActionEntry &);
  _ActionEntry &operator=(const _ActionEntry &);
  _ActionEntry(_ActionEntry &&) = default;
  _ActionEntry &operator=(_ActionEntry &&) = default;

  void init_action_data(bf_rt_action_id_t action_id);
  void init_action_data(bf_rt_action_id_t action_id,
                        std::vector<bf_rt_id_t> &fields);
  switch_status_t init_indirect_data();
  bool is_initialized() const { return init_status; }

  template <typename T>
  switch_status_t set_arg(const bf_rt_field_id_t field_id, T key);
  switch_status_t set_arg(const bf_rt_field_id_t field_id,
                          const char *key,
                          size_t sz);
  switch_status_t set_arg(const bf_rt_field_id_t field_id,
                          const switch_object_id_t oid,
                          const switch_attr_id_t attr_id);
  template <typename T>
  switch_status_t set_arg(const bf_rt_field_id_t field_id,
                          std::vector<T> &key,
                          bool dummy);
  switch_status_t set_arg(const bf_rt_field_id_t field_id, float key);
  switch_status_t set_arg(const bf_rt_field_id_t field_id,
                          std::vector<bf_rt_id_t> &key);
  switch_status_t set_arg(const bf_rt_field_id_t field_id, std::string &key);
  switch_status_t set_arg(const bf_rt_field_id_t field_id, bool key);
  switch_status_t set_arg(const bf_rt_field_id_t field_id,
                          switch_ip_address_t key);
  switch_status_t get_arg(const bf_rt_field_id_t field_id, uint64_t *key) const;
  switch_status_t get_arg(const bf_rt_field_id_t field_id, uint32_t *key) const;
  switch_status_t get_arg(const bf_rt_field_id_t field_id, uint16_t *key) const;
  switch_status_t get_arg(const bf_rt_field_id_t field_id, uint8_t *key) const;
  switch_status_t get_arg(const bf_rt_field_id_t field_id,
                          std::string *key) const;
  switch_status_t get_arg(const bf_rt_field_id_t field_id,
                          const bf_rt_action_id_t action_id,
                          uint64_t *val) const;
  switch_status_t get_arg(const bf_rt_field_id_t field_id,
                          std::vector<bf_rt_id_t> *arr) const;
  switch_status_t get_arg(const bf_rt_field_id_t field_id, bool *val) const;

 private:
  bool init_status = false;
  bf_rt_table_id_t _table_id = 0;
  const BfRtTable *table = NULL;
  bf_rt_action_id_t _action_id = 0;
  std::unique_ptr<BfRtTableData> table_data;
  bool indirect = false;
};

/**
 * _Table
 *
 * Wrapper class to simplify BfRtTable object operations.
 */
class _Table {
 public:
  _Table(bf_rt_table_id_t table_id);
  _Table(const bf_rt_target_t dev_tgt,
         const BfRtInfo *bf_rt_info,
         bf_rt_table_id_t table_id);

  _Table(const bf_rt_target_t dev_tgt,
         const BfRtInfo *bf_rt_info,
         bf_rt_table_id_t table_id,
         std::shared_ptr<BfRtSession> session);

  // Use these from outside of framework
  switch_status_t entry_add(const _MatchKey &match_key,
                            const _ActionEntry &action_entry,
                            bool &bf_rt_status);
  switch_status_t entry_delete(const _MatchKey &match_key);
  switch_status_t entry_modify(const _MatchKey &match_key,
                               const _ActionEntry &action_entry);

  // These are internal calls and should ideally be private
  // FIXME(bfn): make these private
  switch_status_t entry_add(const _MatchKey &match_key,
                            const _ActionEntry &action_entry,
                            bool &bf_rt_status,
                            const bool cache);
  switch_status_t entry_modify(const _MatchKey &match_key,
                               const _ActionEntry &action_entry,
                               const bool cache);
  switch_status_t clear_entries();
  switch_status_t default_entry_set(const _ActionEntry &action_entry,
                                    const bool cache);
  switch_status_t default_entry_get(const _ActionEntry &action_entry);
  /* TODO no defaults */
  switch_status_t pvs_scope_set(const GressTarget &gress_target,
                                bool is_asym = false);
  switch_status_t dynamic_hash_algo_set(bf_rt_id_t action_id,
                                        std::string algo,
                                        uint32_t seed,
                                        uint32_t rotate);
  switch_status_t dynamic_hash_algo_get(bf_rt_id_t action_id,
                                        std::string &algo,
                                        uint32_t &seed,
                                        uint32_t &rotate);

  switch_status_t dynamic_hash_algo_set(bf_rt_id_t action_id,
                                        bool reverse,
                                        uint64_t polynomial,
                                        uint64_t init,
                                        uint64_t final_xor,
                                        uint64_t final_bit_width,
                                        uint32_t seed,
                                        uint32_t rotate);
  switch_status_t dynamic_hash_algo_get(bf_rt_id_t action_id,
                                        bool &reverse,
                                        uint64_t &polynomial,
                                        uint64_t &init,
                                        uint64_t &final_xor,
                                        uint64_t &final_bit_width,
                                        uint32_t &seed,
                                        uint32_t &rotate);

  switch_status_t dynamic_hash_field_set(
      std::vector<bfrt_container_data_t> &container_data_list);
  switch_status_t asymmetric_scope_set();
  switch_status_t do_hw_stats_sync();
  switch_status_t table_size_get(size_t *size);
  switch_status_t table_usage_get(uint32_t *usage);
  switch_status_t entry_get(const _MatchKey &match_key,
                            const _ActionEntry &action_entry);
  switch_status_t name_get(std::string &name);
  switch_status_t table_bfrt_session_set(
      const std::shared_ptr<BfRtSession> session);
  switch_status_t table_bfrt_session_get(std::shared_ptr<BfRtSession> &session);
  switch_status_t match_key_field_size_get(bf_rt_id_t field_id, size_t *size);
  std::shared_ptr<_bfrtCacheObject> create_cache_object(
      const bf_rt_target_t dev_tgt,
      const _MatchKey &match_key,
      const _ActionEntry &action_entry);
  std::set<bf_dev_pipe_t> get_active_pipes();

 private:
  const BfRtTable *table = NULL;
  const bf_rt_table_id_t _table_id = 0;
  bf_rt_target_t table_dev_tgt;
  std::shared_ptr<BfRtSession> table_session;
};

/**
 * p4_object_match_action
 *
 * This class is a wrapper to add a single entry to a MAU table.
 */
class p4_object_match_action : public object {
 public:
  p4_object_match_action(const bf_rt_table_id_t table_id,
                         const switch_attr_id_t status_attr_id,
                         const switch_object_type_t auto_ot,
                         const switch_attr_id_t parent_attr_id,
                         const switch_object_id_t parent);
  ~p4_object_match_action() {}
  switch_status_t create_update();
  switch_status_t del();
  switch_status_t del_pi_only();
  switch_status_t data_get();
  switch_status_t data_set();
  virtual switch_status_t counters_get(const switch_object_id_t handle,
                                       std::vector<switch_counter_t> &cntrs);
  virtual switch_status_t counters_set(const switch_object_id_t handle);
  switch_object_id_t get_auto_oid() { return auto_obj.get_auto_oid(); }
  void device_tgt_set(bf_rt_target_t dev_tgt) { table_dev_tgt = dev_tgt; }
  void device_session_set(const std::shared_ptr<BfRtSession> user_session) {
    table_session = user_session;
  }
  void clear_attrs() { auto_obj.attrs.clear(); }

 protected:
  auto_object auto_obj;
  _MatchKey match_key;
  _ActionEntry action_entry;

 private:
  switch_status_t pi_create_update(bool &bf_rt_status);
  switch_status_t pi_del(bool &bf_rt_status);
  switch_status_t pi_data_get();
  switch_status_t pi_data_set();
  switch_status_t add_to_cache();

  const bf_rt_table_id_t _table_id = 0;
  bf_rt_target_t table_dev_tgt;
  const switch_attr_id_t _status_attr_id = 0;
  std::shared_ptr<BfRtSession> table_session;
};

/**
 * p4_object_match_action_list
 *
 * This class is a wrapper to add a list of entries to a MAU table.
 */
class p4_object_match_action_list : public object {
 public:
  p4_object_match_action_list(const bf_rt_table_id_t table_id,
                              const switch_attr_id_t status_attr_id,
                              const switch_object_type_t auto_ot,
                              const switch_attr_id_t parent_attr_id,
                              const switch_object_id_t parent,
                              const bool auto_cache = true);
  ~p4_object_match_action_list() {}
  switch_status_t create_update();
  switch_status_t del();
  switch_status_t data_get();
  switch_status_t data_set();
  virtual switch_status_t counters_get(const switch_object_id_t handle,
                                       std::vector<switch_counter_t> &cntrs);
  virtual switch_status_t counters_set(const switch_object_id_t handle);
  switch_object_id_t get_auto_oid() { return auto_obj.get_auto_oid(); }
  void device_tgt_set(bf_rt_target_t dev_tgt) { table_dev_tgt = dev_tgt; }
  void clear_attrs() { auto_obj.attrs.clear(); }

 protected:
  auto_object auto_obj;
  std::vector<std::pair<_MatchKey, _ActionEntry>> match_action_list;

 private:
  switch_status_t pi_create_update(std::vector<bool> &bf_rt_status);
  switch_status_t pi_del(std::vector<bool> &bf_rt_status);
  switch_status_t pi_data_get();
  switch_status_t pi_data_set();
  switch_status_t add_to_cache();

  switch_status_t get_status_from_list_attr(const switch_object_id_t object_id,
                                            const switch_attr_id_t attr_id,
                                            std::vector<bool> &bf_rt_status);
  switch_status_t set_status_to_list_attr(const switch_object_id_t object_id,
                                          const switch_attr_id_t attr_id,
                                          std::vector<bool> &bf_rt_status);

  const bf_rt_table_id_t _table_id = 0;
  bf_rt_target_t table_dev_tgt;
  const switch_attr_id_t _status_attr_id = 0;
  // This controls whether or not the infra will cache this object
  // during warm init. Setting this flag to false prevents infra from auto
  // caching
  // these objects. This means either the user is not interested in caching the
  // object
  // or has a custom logic for caching these entries.
  // Check traffic_class for an example.
  const bool _auto_cache = true;
};

/**
 * p4_object_action_selector
 *
 * This class is a wrapper to add a member to an action selector table.
 * Note that the member_id is automatically picked from the object_id.
 * The derived class has to simple fill in the table_data
 */
class p4_object_action_selector : public object {
 public:
  p4_object_action_selector(const bf_rt_table_id_t act_prof_id,
                            const bf_rt_field_id_t action_member_id,
                            const switch_attr_id_t status_attr_id,
                            const switch_object_type_t auto_ot,
                            const switch_attr_id_t parent_attr_id,
                            const switch_object_id_t parent);
  ~p4_object_action_selector() {}
  switch_status_t create_update();
  switch_status_t del();
  void clear_attrs() { auto_obj.attrs.clear(); }
  size_t attrs_size() { return auto_obj.attrs.size(); }

 private:
  switch_status_t pi_create_update();
  switch_status_t pi_del(bool &bf_rt_status);
  switch_status_t add_to_cache();

  auto_object auto_obj;

  const bf_rt_table_id_t _act_prof_id = 0;
  bf_rt_target_t table_dev_tgt;
  const bf_rt_field_id_t _action_member_id = 0;
  const switch_attr_id_t _status_attr_id = 0;

 protected:
  _MatchKey match_key;
  _ActionEntry action_entry;
};

/**
 * p4_object_action_selector_list
 *
 * This class is a wrapper to add a list of members to an action selector table.
 */
class p4_object_action_selector_list : public object {
 public:
  p4_object_action_selector_list(const bf_rt_table_id_t act_prof_id,
                                 const bf_rt_field_id_t action_member_id,
                                 const switch_attr_id_t status_attr_id,
                                 const switch_object_type_t auto_ot,
                                 const switch_attr_id_t parent_attr_id,
                                 const switch_object_id_t parent);
  ~p4_object_action_selector_list() {}
  switch_status_t create_update();
  switch_status_t del();
  void clear_attrs() { auto_obj.attrs.clear(); }
  size_t attrs_size() { return auto_obj.attrs.size(); }

 private:
  switch_status_t pi_create_update(std::vector<bool> &bf_rt_status);
  switch_status_t pi_del(std::vector<bool> &bf_rt_status);
  switch_status_t add_to_cache();

  auto_object auto_obj;

  bf_rt_target_t table_dev_tgt;
  const bf_rt_table_id_t _act_prof_id = 0;
  const bf_rt_field_id_t _action_member_id = 0;
  const switch_attr_id_t _status_attr_id = 0;

 protected:
  std::vector<std::pair<_MatchKey, _ActionEntry>> match_action_list;
};

/**
 * p4_object_selector_group
 *
 * This class is a wrapper to add a group to a selector group table
 */
class p4_object_selector_group : public object {
 public:
  p4_object_selector_group(const bf_rt_table_id_t selector_grp_table,
                           const switch_attr_id_t status_attr_id,
                           const uint32_t max_grp_size_field,
                           const uint32_t mbr_array_field,
                           const uint32_t mbr_status_field,
                           const switch_object_type_t auto_ot,
                           const switch_attr_id_t parent_attr_id,
                           const switch_object_id_t parent);
  ~p4_object_selector_group() {}
  switch_status_t create_update();
  switch_status_t del();
  switch_status_t add_delete_member(std::vector<bf_rt_id_t> &mbrs,
                                    std::vector<bool> &mbr_status);
  switch_status_t add_delete_member_with_weight(std::vector<bf_rt_id_t> &mbrs,
                                                std::vector<bool> &mbr_status,
                                                std::vector<uint32_t> &weights,
                                                uint32_t sum);
  switch_status_t get_member_size(size_t &size);
  static const uint32_t SELECTOR_GROUP_MAX_GROUP_SIZE = 64;

 protected:
  auto_object auto_obj;
  _MatchKey match_key;
  _ActionEntry action_entry;

 private:
  switch_status_t pi_create_update(bool &bf_rt_status);
  switch_status_t pi_del(bool &bf_rt_status);
  switch_status_t add_to_cache();

  const bf_rt_table_id_t _selector_grp_table = 0;
  bf_rt_target_t table_dev_tgt;
  const uint32_t _max_group_size_field = 0;
  const uint32_t _mbr_array_field = 0;
  const uint32_t _mbr_status_field = 0;
  size_t _max_grp_size = 0;

  const switch_attr_id_t _status_attr_id = 0;
};

/**
 * p4_object_pd_fixed
 *
 * This class is a wrapper to add a single entry to a PD fixed table
 */
class p4_object_pd_fixed : public object {
 public:
  p4_object_pd_fixed(const bf_rt_table_id_t table_id,
                     const switch_attr_id_t status_attr_id,
                     const switch_object_type_t auto_ot,
                     const switch_attr_id_t parent_attr_id,
                     const switch_object_id_t parent);
  ~p4_object_pd_fixed() {}
  switch_status_t create_update();
  switch_status_t del();
  switch_object_id_t get_auto_oid() { return auto_obj.get_auto_oid(); }
  void device_tgt_set(bf_rt_target_t dev_tgt) { table_dev_tgt = dev_tgt; }
  void clear_attrs() { auto_obj.attrs.clear(); }

 protected:
  auto_object auto_obj;
  _MatchKey match_key;
  _ActionEntry action_entry;

 private:
  switch_status_t pi_create_update(bool &bf_rt_status);
  switch_status_t pi_del(bool &bf_rt_status);
  switch_status_t add_to_cache();

  const bf_rt_table_id_t _table_id = 0;
  bf_rt_target_t table_dev_tgt;
  const switch_attr_id_t _status_attr_id = 0;
};
}  // namespace bf_rt
}  // namespace smi

#endif  // INCLUDE_S3_BF_RT_BACKEND_H__
