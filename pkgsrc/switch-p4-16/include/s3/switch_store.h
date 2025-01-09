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


#ifndef INCLUDE_S3_SWITCH_STORE_H__
#define INCLUDE_S3_SWITCH_STORE_H__

#include <set>
#include <vector>
#include <unordered_set>
#include <functional>
#include <mutex>

#include "s3/meta/meta.h"
#include "s3/attribute_util.h"

namespace smi {
namespace switch_store {

using ::smi::attr_util::object_and_attribute_t;
class smiContext {
  smiContext() : warm_init(false), stats_timer_off(false) {}
  bool warm_init;
  bool stats_timer_off;

 public:
  static smiContext &context() {
    static smiContext instance;
    return instance;
  }

  smiContext(smiContext const &) = delete;
  void operator=(smiContext const &) = delete;

  inline bool in_warm_init() const { return warm_init; }
  inline void warm_init_begin() { warm_init = true; }
  inline void warm_init_end() { warm_init = false; }
  inline bool is_stats_timer_off() const { return stats_timer_off; }
  inline void stats_timer_turn_off() { stats_timer_off = true; }

  std::mutex fdb_lock;
};

static inline bool object_handle_valid(switch_object_id_t &object_id) {
  return (object_id.data != SWITCH_NULL_OBJECT_ID);
}

ModelInfo *switch_model_info_get();

void switch_store_lock(void);
void switch_store_unlock(void);

int object_try_lock(const switch_object_id_t oid);
void object_unlock(const switch_object_id_t oid);
bool object_exists(const switch_object_id_t oid);

switch_status_t object_info_init(const char *const object_info_path,
                                 bool warm_init,
                                 const char *const warm_init_file,
                                 bool override_log_level = true);
switch_status_t object_info_dump(const char *dump_file);
switch_status_t object_info_clean();
switch_status_t object_replay(bool warm_init);

inline switch_object_type_t object_type_query(switch_object_id_t object_id) {
  return (switch_object_type_t)(object_id.data >> 48);
}

switch_status_t object_create(const switch_object_type_t object_type,
                              const std::set<attr_w> &attrs,
                              switch_object_id_t &object_id);
switch_status_t object_create_by_hdl(const switch_object_type_t object_type,
                                     const std::set<attr_w> &attrs,
                                     const switch_object_id_t object_id);

switch_status_t object_create_by_id(const switch_object_type_t object_type,
                                    const std::set<attr_w> &attrs,
                                    const uint64_t id);
switch_status_t object_create_minimal(const switch_object_type_t object_type,
                                      const std::set<attr_w> &attrs,
                                      switch_object_id_t &object_id);

switch_status_t object_delete(const switch_object_id_t object_id);
switch_status_t object_delete_by_id(const switch_object_type_t object_type,
                                    const uint64_t id);

switch_object_id_t object_get_by_id(const switch_object_type_t object_type,
                                    const uint64_t id);

switch_status_t attribute_set(const switch_object_id_t object_id,
                              const attr_w &attr);

switch_status_t attribute_get(const switch_object_id_t object_id,
                              const switch_attr_id_t attr_id,
                              attr_w &attr);
switch_status_t attribute_get_all(const switch_object_id_t object_id,
                                  std::set<attr_w> &attrs);
switch_status_t attribute_get_all(
    const switch_object_id_t object_id,
    std::vector<std::reference_wrapper<const switch_attribute_t>> &value_out);
switch_status_t object_id_get_wkey(switch_object_type_t ot,
                                   std::set<attr_w> attrs,
                                   switch_object_id_t &object_id);
switch_status_t object_counters_get(switch_object_id_t object_id,
                                    std::vector<switch_counter_t> &cntrs);
switch_status_t object_counters_clear(switch_object_id_t object_id,
                                      const std::vector<uint16_t> &cntrs_ids);
switch_status_t object_counters_clear_all(switch_object_id_t object_id);
switch_status_t object_backup_stats_cache();
switch_status_t object_restore_stats_cache(bool warm_init);
switch_status_t oid_create(switch_object_type_t object_type,
                           switch_object_id_t &object_id_out,
                           bool set);
switch_status_t object_dump(const char *const out_file);
switch_status_t oid_get_all_handles(
    switch_object_type_t object_type,
    std::vector<switch_object_id_t> &object_handles);
switch_status_t oid_free(switch_object_id_t oid);
switch_status_t check_for_existing(switch_object_type_t object_type,
                                   const std::set<attr_w> &attrs,
                                   switch_object_id_t &object_id);
switch_status_t validate_attrs(const ObjectInfo *object_info,
                               const std::set<attr_w> &attrs);

// Delete one item at any index in list
switch_status_t list_v_del(const switch_object_id_t object_id,
                           const switch_attr_id_t attr_id,
                           const switch_object_id_t value_out);
// Push one item to tail of list
switch_status_t list_v_push(const switch_object_id_t object_id,
                            const switch_attr_id_t attr_id,
                            const switch_object_id_t value_in);
// Get one item at specified index
template <typename T>
switch_status_t list_v_get(const switch_object_id_t object_id,
                           const switch_attr_id_t attr_id,
                           const size_t index,
                           T &value_out);
// Get count of items in list
switch_status_t list_len(const switch_object_id_t object_id,
                         const switch_attr_id_t attr_id,
                         size_t &len_out);
// Clear all items from list
switch_status_t list_clear(const switch_object_id_t object_id,
                           const switch_attr_id_t attr_id);

// Both of the below APIs return references
// If using this API, be sure to make a copy if the items are to be iterated
const std::vector<object_and_attribute_t> &get_object_references(
    const switch_object_id_t dst, const switch_object_type_t type);
// This API makes a copy of the set
switch_status_t referencing_set_get(const switch_object_id_t dst,
                                    const switch_object_type_t type,
                                    std::set<switch_object_id_t> &ref_set);

switch_status_t object_ready_for_delete(const switch_object_id_t oid,
                                        bool &del);

template <typename T>
switch_status_t v_get(const switch_object_id_t object_id,
                      const switch_attr_id_t attr_id,
                      T &val);
template <typename T>
switch_status_t v_set(const switch_object_id_t object_id,
                      const switch_attr_id_t attr_id,
                      const T val);

static constexpr uint64_t MASK_OBJECT_TYPE() {
  return (static_cast<uint64_t>(1) << OBJECT_ID_WIDTH) - 1;
}
inline static uint64_t handle_to_id(switch_object_id_t object_id) {
  if (object_id.data == 0) return 0;
  return (object_id.data & MASK_OBJECT_TYPE());
}

inline static switch_object_id_t id_to_handle(switch_object_type_t object_type,
                                              uint64_t id) {
  return {.data = static_cast<uint64_t>(object_type) << OBJECT_ID_WIDTH | id};
}

typedef switch_status_t (*create_handler_before_fn)(
    const switch_object_type_t object_type, std::set<attr_w> &attrs);
typedef switch_status_t (*create_handler_after_fn)(
    const switch_object_id_t object_id, const std::set<attr_w> &attrs);

typedef switch_status_t (*delete_handler_before_fn)(
    const switch_object_id_t object_id);
typedef switch_status_t (*delete_handler_after_fn)(
    const switch_object_type_t object_type, const std::set<attr_w> &attrs);

typedef switch_status_t (*update_handler_before_fn)(
    const switch_object_id_t object_id, const attr_w &attr);
typedef switch_status_t (*update_handler_after_fn)(
    const switch_object_id_t object_id, const attr_w &attr);

typedef switch_status_t (*counter_get_handler_fn)(
    const switch_object_id_t object_id, std::vector<switch_counter_t> &cntrs);
typedef switch_status_t (*counters_set_handler_fn)(
    const switch_object_id_t object_id, const std::vector<uint16_t> &cntr_ids);
typedef switch_status_t (*all_counters_set_handler_fn)(
    const switch_object_id_t object_id);

typedef bool (*skip_auto_objects_fn)(const switch_object_id_t object_id,
                                     const switch_object_type_t auto_ot);

switch_status_t reg_create_trigs_after(const switch_object_type_t object_type,
                                       const create_handler_after_fn fn);

switch_status_t reg_create_trigs_before(const switch_object_type_t object_type,
                                        const create_handler_before_fn fn);

switch_status_t reg_delete_trigs_after(const switch_object_type_t object_type,
                                       const delete_handler_after_fn fn);

switch_status_t reg_delete_trigs_before(const switch_object_type_t object_type,
                                        const delete_handler_before_fn fn);

switch_status_t reg_update_trigs_after(const switch_object_type_t object_type,
                                       const update_handler_after_fn fn);

switch_status_t reg_update_trigs_before(const switch_object_type_t object_type,
                                        const update_handler_before_fn fn);

switch_status_t reg_counter_get_trigs(const switch_object_type_t object_type,
                                      const counter_get_handler_fn fn);

switch_status_t reg_counters_set_trigs(const switch_object_type_t object_type,
                                       const counters_set_handler_fn fn);

switch_status_t reg_skip_auto_object_trigs(
    const switch_object_type_t object_type, const skip_auto_objects_fn fn);

switch_status_t reg_all_counters_set_trigs(
    const switch_object_type_t object_type,
    const all_counters_set_handler_fn fn);

switch_status_t object_get_first_handle(switch_object_type_t object_type,
                                        switch_object_id_t &object_handle);
switch_status_t object_get_next_handles(
    switch_object_id_t &cur_object_handle,
    uint32_t num_handles,
    std::vector<switch_object_id_t> &next_object_handles,
    uint32_t &out_num_ids);

switch_status_t object_get_all_handles(
    const switch_object_type_t object_type,
    std::vector<switch_object_id_t> &object_handles);

bool check_skip_auto_objects_for_object(const switch_object_id_t object_id,
                                        const switch_object_type_t auto_ot);

bool is_object_type_valid(switch_object_type_t object_type);
std::string object_name_get_from_object(switch_object_id_t object_id);
std::string object_name_get_from_type(switch_object_type_t object_type);

typedef std::string (*debug_cli_fn)(const switch_object_id_t object_id);

switch_status_t reg_debug_cli_callback(const switch_object_type_t object_type,
                                       const debug_cli_fn fn);
std::string object_pkt_path_counter_print(const switch_object_id_t oid);
}  // namespace switch_store
}  // namespace smi

#endif  // INCLUDE_S3_SWITCH_STORE_H__
