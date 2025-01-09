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


#include <vector>
#include <set>
#include <string>

#include "s3/smi.h"
#include "s3/switch_store.h"
#include "s3/attribute.h"
#include "s3/record.h"
#include "s3/bf_rt_backend.h"
#include "../../s3/log.h"
#include "common/utils.h"

using ::smi::logging::switch_log;

/**
 * This file contains generic C and C++ wrapper functions for the 4 primary
 * switch_store APIs (create/delete/set/get)
 * These APIs act as frontend for external applications, CLI, SAI, thrift, etc
 *
 * For bf_switchd users, "bf_switch_init" defined at the bottom is the entry
 * point to bf_switch
 */

namespace bf_switch {

switch_status_t bf_switch_object_create_by_id(
    const switch_object_type_t object_type,
    const std::set<smi::attr_w> &attrs,
    const uint64_t id) {
  return smi::api::smi_object_create_by_id(object_type, attrs, id);
}

bool bf_switch_object_exists_by_id(const switch_object_type_t object_type,
                                   const uint64_t id) {
  return smi::api::smi_object_exists_by_id(object_type, id);
}

switch_status_t bf_switch_object_delete_by_id(
    const switch_object_type_t object_type, const uint64_t id) {
  return smi::api::smi_object_delete_by_id(object_type, id);
}

switch_status_t bf_switch_object_get(const switch_object_type_t object_type,
                                     const std::set<smi::attr_w> &attrs,
                                     switch_object_id_t &object_handle) {
  return smi::api::smi_object_get(object_type, attrs, object_handle);
}

switch_status_t bf_switch_object_create(const switch_object_type_t object_type,
                                        const std::set<smi::attr_w> &attrs,
                                        switch_object_id_t &object_handle) {
  return smi::api::smi_object_create(object_type, attrs, object_handle);
}

switch_status_t bf_switch_object_delete(
    const switch_object_id_t object_handle) {
  return smi::api::smi_object_delete(object_handle);
}

switch_status_t bf_switch_object_type_get(
    const switch_object_id_t object_handle, switch_object_type_t &object_type) {
  object_type = smi::switch_store::object_type_query(object_handle);
  return SWITCH_STATUS_SUCCESS;
}

std::string bf_switch_object_name_get(const switch_object_id_t object_id) {
  return smi::switch_store::object_name_get_from_object(object_id);
}

switch_status_t bf_switch_attribute_set(const switch_object_id_t object_handle,
                                        const smi::attr_w &attr) {
  return smi::api::smi_attribute_set(object_handle, attr);
}

switch_status_t bf_switch_attribute_get(const switch_object_id_t object_handle,
                                        const switch_attr_id_t attr_id,
                                        smi::attr_w &attr) {
  return smi::api::smi_attribute_get(object_handle, attr_id, attr);
}

switch_status_t bf_switch_counters_get(switch_object_id_t object_id,
                                       std::vector<switch_counter_t> &cntrs) {
  return smi::api::smi_object_counters_get(object_id, cntrs);
}

switch_status_t bf_switch_counters_clear(
    switch_object_id_t object_id, const std::vector<uint16_t> &cntrs_ids) {
  return smi::api::smi_object_counters_clear(object_id, cntrs_ids);
}

switch_status_t bf_switch_counters_clear_all(switch_object_id_t object_id) {
  return smi::api::smi_object_counters_clear_all(object_id);
}

switch_status_t bf_switch_object_flush_all(
    const switch_object_type_t object_type) {
  return smi::api::smi_object_flush_all(object_type);
}

switch_status_t bf_switch_get_first_handle(switch_object_type_t object_type,
                                           switch_object_id_t &object_handle) {
  return smi::api::smi_object_get_first_handle(object_type, object_handle);
}

switch_status_t bf_switch_get_next_handles(
    switch_object_id_t object_handle,
    uint32_t in_num_handles,
    std::vector<switch_object_id_t> &next_handles,
    uint32_t &out_num_handles) {
  return smi::api::smi_object_get_next_handles(
      object_handle, in_num_handles, next_handles, out_num_handles);
}

switch_status_t bf_switch_get_all_handles(
    switch_object_type_t object_type,
    std::vector<switch_object_id_t> &object_handles) {
  return smi::api::smi_object_get_all_handles(object_type, object_handles);
}

switch_status_t bf_switch_object_log_level_set(
    const switch_object_type_t object_type,
    const switch_verbosity_t verbosity) {
  return smi::api::smi_object_log_level_set(object_type, verbosity);
}

switch_status_t bf_switch_global_log_level_set(
    const switch_verbosity_t verbosity) {
  return smi::api::smi_log_level_set(verbosity);
}

switch_status_t bf_switch_start_batch() { return smi::bf_rt::start_batch(); }
switch_status_t bf_switch_end_batch() { return smi::bf_rt::end_batch(); }

void bf_switch_record_comment_mode_set(bool on) {
  smi::record::record_comment_mode_set(on);
}

}  // namespace bf_switch

#ifdef __cplusplus
extern "C" {
#endif

switch_status_t bf_switch_object_get_c(
    switch_object_type_t object_type,
    uint32_t attr_count,
    const switch_attribute_t *const attr_list,
    switch_object_id_t *const object_handle) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  smi::ModelInfo *model_info = smi::switch_store::switch_model_info_get();

  switch_object_id_t object_handle_tmp = {0};
  std::set<smi::attr_w> attrs;
  for (uint32_t i = 0; i < attr_count; i++) {
    smi::attr_w attr(0);
    status = attr.attr_import(attr_list[i]);
    if (status != SWITCH_STATUS_SUCCESS) return status;

    attrs.insert(attr);
  }
  status = smi::api::smi_object_get(object_type, attrs, object_handle_tmp);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               object_type,
               "{}:{}: failed status={} ot={}",
               __func__,
               __LINE__,
               status,
               model_info->get_object_name_from_type(object_type));
    return status;
  }

  *object_handle = object_handle_tmp;
  return status;
}

switch_status_t bf_switch_object_create_c(
    switch_object_type_t object_type,
    uint32_t attr_count,
    const switch_attribute_t *const attr_list,
    switch_object_id_t *const object_handle) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  smi::ModelInfo *model_info = smi::switch_store::switch_model_info_get();

  switch_object_id_t object_handle_tmp = {0};
  std::set<smi::attr_w> attrs;
  for (uint32_t i = 0; i < attr_count; i++) {
    smi::attr_w attr(0);
    status = attr.attr_import(attr_list[i]);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 object_type,
                 "{}: attr_import failed status={} ot={} attr {}",
                 __func__,
                 status,
                 model_info->get_object_name_from_type(object_type),
                 attr.getattr());
      return status;
    }

    attrs.insert(attr);
  }
  status = smi::api::smi_object_create(object_type, attrs, object_handle_tmp);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               object_type,
               "{}:{}: failed status={} ot={}",
               __func__,
               __LINE__,
               status,
               model_info->get_object_name_from_type(object_type));
    return status;
  }

  *object_handle = object_handle_tmp;
  return status;
}

switch_status_t bf_switch_object_delete_c(switch_object_id_t object_handle) {
  return smi::api::smi_object_delete(object_handle);
}

switch_status_t bf_switch_object_type_get_c(
    switch_object_id_t object_handle, switch_object_type_t *const object_type) {
  if (object_type == NULL) {
    return SWITCH_STATUS_INVALID_PARAMETER;
  }
  *object_type = smi::switch_store::object_type_query(object_handle);
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t bf_switch_attribute_set_c(
    switch_object_id_t object_handle, const switch_attribute_t *const attr_in) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  smi::attr_w attr(0);
  status = attr.attr_import(*attr_in);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               smi::switch_store::object_type_query(object_handle),
               "{}:{}: attr_import failed status={} oid={:#x}",
               __func__,
               __LINE__,
               status,
               object_handle.data);
    return status;
  }

  status = smi::api::smi_attribute_set(object_handle, attr);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               smi::switch_store::object_type_query(object_handle),
               "{}:{}: status={} oid={:#x} attr {}",
               __func__,
               __LINE__,
               status,
               object_handle.data,
               attr.getattr());
    return status;
  }

  return status;
}

switch_status_t bf_switch_attribute_get_c(switch_object_id_t object_handle,
                                          switch_attr_id_t attr_id,
                                          switch_attribute_t *const attr_out) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_attribute_t attribute = {.id = attr_id, .value = {}};

  smi::attr_w attr(attr_id);

  status = smi::api::smi_attribute_get(object_handle, attr_id, attr);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               smi::switch_store::object_type_query(object_handle),
               "{}:{}: failed status={} oid={:#x} attr_id {}",
               __func__,
               __LINE__,
               status,
               object_handle.data,
               attribute);
    return status;
  }

  status = attr.attr_export(attr_out);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               smi::switch_store::object_type_query(object_handle),
               "{}:{}: attr_copy failed status={} oid={:#x} attr_id {}",
               __func__,
               __LINE__,
               status,
               object_handle.data,
               attribute);
    return status;
  }

  return status;
}

switch_status_t bf_switch_counters_get_c(switch_object_id_t object_handle,
                                         uint16_t *num_counters,
                                         switch_counter_t **counters) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::vector<switch_counter_t> cntrs;

  status = smi::api::smi_object_counters_get(object_handle, cntrs);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               smi::switch_store::object_type_query(object_handle),
               "{}:{}: failed status={} oid={:#x}",
               __func__,
               __LINE__,
               status,
               object_handle.data);
    return status;
  }

  *num_counters = cntrs.size();
  *counters =  // NOLINTNEXTLINE
      (switch_counter_t *)calloc(sizeof(switch_counter_t), *num_counters);
  if (*counters == NULL) {
    status = SWITCH_STATUS_NO_MEMORY;
    switch_log(SWITCH_API_LEVEL_ERROR,
               smi::switch_store::object_type_query(object_handle),
               "{}:{}: failed to allocate memory status={} "
               "oid={:#x} num_counters {}",
               __func__,
               __LINE__,
               status,
               object_handle.data,
               *num_counters);
    return status;
  }

  uint16_t i = 0;
  for (const auto &cntr : cntrs) {
    (*counters)[i].counter_id = cntr.counter_id;
    (*counters)[i].count = cntr.count;
    i++;
  }
  return status;
}

switch_status_t bf_switch_counters_clear_c(switch_object_id_t object_handle,
                                           uint16_t *cntr_ids,
                                           uint32_t cntrs_num) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::vector<uint16_t> _cntrs_ids;

  for (uint32_t i = 0; i < cntrs_num; i++) {
    _cntrs_ids.push_back(cntr_ids[i]);
  }

  if (_cntrs_ids.size()) {
    status = smi::api::smi_object_counters_clear(object_handle, _cntrs_ids);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 smi::switch_store::object_type_query(object_handle),
                 "{}:{}: failed to clear counters status={} oid={:#x}",
                 __func__,
                 __LINE__,
                 status,
                 object_handle.data);
      return status;
    }
  }
  return status;
}

switch_status_t bf_switch_counters_clear_all_c(
    switch_object_id_t object_handle) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  status = smi::api::smi_object_counters_clear_all(object_handle);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               smi::switch_store::object_type_query(object_handle),
               "{}:{}: failed to clear all counters status={} oid={:#x}",
               __func__,
               __LINE__,
               status,
               object_handle.data);
    return status;
  }

  return status;
}

switch_status_t bf_switch_get_first_handle_c(
    switch_object_type_t object_type, switch_object_id_t *const object_handle) {
  if (object_handle == NULL) {
    return SWITCH_STATUS_INVALID_PARAMETER;
  }
  return smi::api::smi_object_get_first_handle(object_type, *object_handle);
}

switch_status_t bf_switch_get_next_handles_c(
    switch_object_id_t object_handle,
    uint32_t in_num_handles,
    switch_object_id_t *const next_handles,
    uint32_t *const out_num_handles) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::vector<switch_object_id_t> handles;
  uint32_t i = 0;
  if (next_handles == NULL || out_num_handles == NULL) {
    return SWITCH_STATUS_INVALID_PARAMETER;
  }
  status = smi::api::smi_object_get_next_handles(
      object_handle, in_num_handles, handles, *out_num_handles);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               smi::switch_store::object_type_query(object_handle),
               "{}:{}: failed to get handles status={}",
               __func__,
               __LINE__,
               status);
    return status;
  }
  // if size of next_handles is less than out_num_handles, user needs to call
  // this API again
  for (auto it = handles.begin(); it != handles.end(); it++) {
    if (i == in_num_handles) break;
    next_handles[i++] = *it;
  }
  return status;
}

switch_status_t bf_switch_get_all_handles_c(switch_object_type_t object_type,
                                            switch_object_id_t **out_handles,
                                            uint32_t *const out_num_handles) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  int i = 0;
  std::vector<switch_object_id_t> handles;
  switch_object_id_t *local_handles = NULL;
  status = smi::api::smi_object_get_all_handles(object_type, handles);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "{}:{}: failed to get all handles type {} status={}",
               __func__,
               __LINE__,
               object_type,
               status);
    return status;
  }

  local_handles = static_cast<switch_object_id_t *>(
      calloc(handles.size(), sizeof(switch_object_id_t)));
  if (!local_handles) {
    return SWITCH_STATUS_NO_MEMORY;
  }

  for (auto it = handles.begin(); it != handles.end(); it++) {
    local_handles[i++] = *it;
  }
  *out_handles = local_handles;
  *out_num_handles = handles.size();
  return status;
}

switch_status_t bf_switch_object_log_level_set_c(
    switch_object_type_t object_type, switch_verbosity_t verbosity) {
  return smi::api::smi_object_log_level_set(object_type, verbosity);
}

switch_status_t bf_switch_global_log_level_set_c(switch_verbosity_t verbosity) {
  return smi::api::smi_log_level_set(verbosity);
}

#ifdef __cplusplus
}
#endif
