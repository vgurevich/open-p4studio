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


/*
 * bf_switch_cli_api.cpp: bf_switch api wrapper file.
 *      this file will be used to implement per API custom logic.
 *      currently supports counter API, CLI initialization.
 */
#include <memory>
#include <unordered_map>
#include <vector>
#include <mutex>  // NOLINT(build/c++11)

#include "bf_switch_cli_api.h"
#include "s3/attribute_util.h"
#include "s3/switch_store.h"
#include "../log.h"

namespace smi {
namespace switchcli {

using namespace ::smi;
using namespace smi::logging;

/* global variables */
std::unique_ptr<stats_mgr> counter_stats_mgr;
std::mutex stats_mgr_lock;

/* switch api wrappers */
switch_status_t bf_switch_cli_api_counters_get(
    switch_object_id_t object_handle,
    std::vector<switch_cli_api_counter> &cntrs) {
  switch_status_t status;
  std::vector<switch_counter_t> counters;
  status = switch_store::object_counters_get(object_handle, counters);
  if (status != SWITCH_STATUS_SUCCESS) {
    return status;
  }

  std::lock_guard<std::mutex> guard(stats_mgr_lock);
  for (const auto counter : counters) {
    switch_cli_api_counter api_cntr = {};
    /* skip zero counters */
    if (counter.count == 0) continue;

    switch_counter_t counters_changed = {};
    counter_stats_mgr->get_and_update_cntr_changed_since_last_query(
        object_handle, counter, counters_changed);
    api_cntr.switch_counter = counter;
    api_cntr.delta = counters_changed.count;
    cntrs.emplace_back(std::move(api_cntr));
  }
  return status;
}

switch_status_t stats_mgr::get_and_update_cntr_changed_since_last_query(
    const switch_object_id_t object_handle,
    const switch_counter_t &curr_counter,
    switch_counter_t &counters_changed) {
  auto counter_obj_itr = counter_obj_map.find(object_handle);
  if (counter_obj_itr == counter_obj_map.end()) {
    object_counter_info tmp = {};
    auto res = counter_obj_map.insert(std::make_pair(object_handle, tmp));
    if (!res.second) {
      return SWITCH_STATUS_FAILURE;
    }
    counter_obj_itr = res.first;
  }

  auto &counter_obj_value = counter_obj_itr->second;
  auto counter_itr =
      counter_obj_value.counterId_map.find(curr_counter.counter_id);
  if (counter_itr == counter_obj_value.counterId_map.end()) {
    /* now stats_mgr has to monitor new counter_id. update map */
    cli_counter_metadata stats_cntr(curr_counter);
    counter_obj_value.counterId_map.insert(
        std::make_pair(curr_counter.counter_id, stats_cntr));
    counters_changed.count = 0;
    return SWITCH_STATUS_SUCCESS;
  }

  cli_counter_metadata &last_counter = counter_itr->second;
  if (counter_obj_value.is_dirty) {
    counter_obj_value.is_dirty = false;
    last_counter.last_count = curr_counter.count;
    counters_changed.count = 0;
  } else if (last_counter.last_count > curr_counter.count) {
    /* counter overflow  */
    counters_changed.count = curr_counter.count;
    last_counter.last_count = curr_counter.count;
  } else if (last_counter.last_count == curr_counter.count) {
    counters_changed.count = 0;
  } else {
    counters_changed.count = curr_counter.count - last_counter.last_count;
    last_counter.last_count = curr_counter.count;
  }

  return SWITCH_STATUS_SUCCESS;
}

switch_status_t stats_mgr::clear_counters(
    const switch_object_id_t object_handle) {
  auto itr = counter_obj_map.find(object_handle);
  if (itr == counter_obj_map.end()) {
    return SWITCH_STATUS_FAILURE;
  }
  itr->second.is_dirty = true;

  return SWITCH_STATUS_SUCCESS;
}

switch_status_t stats_mgr::delete_counters(
    const switch_object_id_t object_handle) {
  auto itr = counter_obj_map.find(object_handle);
  if (itr == counter_obj_map.end()) {
    /* stats_mgr not tracking this object. ignore delete event */
    return SWITCH_STATUS_SUCCESS;
  }
  size_t erased = counter_obj_map.erase(object_handle);
  if (erased != 1) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               switch_store::object_type_query(object_handle),
               "CLI(stats_mgr): delete counter failed: erase: {}",
               erased);
  }
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t bf_switch_cli_api_notify_counter_event(
    bf_switch_cli_api_counter_event_t event_type,
    const switch_object_id_t counter_object_handle) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  std::lock_guard<std::mutex> guard(stats_mgr_lock);
  if (event_type == COUNTER_EVENT_TYPE_CLEAR) {
    status = counter_stats_mgr->clear_counters(counter_object_handle);
  } else if (event_type == COUNTER_EVENT_TYPE_OBJECT_DELETE) {
    status = counter_stats_mgr->delete_counters(counter_object_handle);
  }
  return status;
}

switch_status_t before_counter_object_delete_handler(
    const switch_object_id_t object_handle) {
  return bf_switch_cli_api_notify_counter_event(
      COUNTER_EVENT_TYPE_OBJECT_DELETE, object_handle);
}

switch_status_t bf_switch_cli_init() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  ModelInfo *model_info = switch_store::switch_model_info_get();
  if (model_info == NULL) {
    status = SWITCH_STATUS_FAILURE;
    return status;
  }

  try {
    /*
     * @fix this. no need to have this reference
     * if refs class provide APIs to do inverse reference on objects
     */
    std::unique_ptr<stats_mgr> p = std::unique_ptr<stats_mgr>(new stats_mgr());
    counter_stats_mgr = std::move(p);
  } catch (std::exception &e) {
    status = SWITCH_STATUS_NO_MEMORY;
    switch_log(SWITCH_API_LEVEL_ERROR,
               static_cast<uint16_t>(0),
               "cli init failed, switch_status: {}",
               switch_error_to_string(status));
    return status;
  }

  /*
   * register pre-triggers for counter object deletes to
   * manage counter key store.
   */
  for (auto it = model_info->begin(); it != model_info->end(); it++) {
    auto object_info = *it;
    const auto ot = object_info.object_type;
    if (object_info.get_counter()) {
      status = switch_store::reg_delete_trigs_before(
          ot, &before_counter_object_delete_handler);
      if (status != SWITCH_STATUS_SUCCESS) return SWITCH_STATUS_FAILURE;
    }
  }
  return SWITCH_STATUS_SUCCESS;
}
}  // namespace switchcli
}  // namespace smi
