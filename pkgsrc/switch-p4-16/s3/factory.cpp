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


#include "s3/factory.h"

#include <list>
#include <unordered_map>
#include <iterator>
#include <queue>
#include <memory>
#include <map>
#include <vector>
#include <set>
#include <unordered_set>

#include "s3/attribute_util.h"
#include "s3/switch_store.h"
#include "./log.h"

namespace smi {

#define __NS__ "factory"
using ::smi::logging::switch_log;

/*******************************************************************************
 * Object implementations
 ******************************************************************************/

switch_status_t find_auto_oid(const switch_object_id_t parent_oid,
                              const switch_object_type_t object_type,
                              switch_object_id_t &auto_oid) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  auto_oid.data = 0;
  const auto &ref_set =
      switch_store::get_object_references(parent_oid, object_type);

  if (ref_set.size() == 0) return SWITCH_STATUS_SUCCESS;

  auto_oid = ref_set.begin()->oid;

  return status;
}

auto_object::auto_object(const switch_object_type_t auto_ot,
                         const switch_attr_id_t parent_attr_id,
                         const switch_object_id_t parent)
    : _auto_ot(auto_ot),
      _parent_attr_id(parent_attr_id),
      _parent(parent),
      attrs{attr_w(parent_attr_id, parent)} {
  auto status = find_auto_oid(parent, auto_ot, _auto_oid);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               auto_ot,
               "{}.{}:{}: auto object id not found",
               __NS__,
               __func__,
               __LINE__);
    return;
  }
  SWITCH_DEBUG_LOG(logger("construct"));
}

auto_object::auto_object(const switch_object_type_t auto_ot,
                         const switch_attr_id_t parent_attr_id,
                         const switch_object_id_t parent,
                         const switch_attr_id_t status_attr_id,
                         bool status_value)
    : _auto_ot(auto_ot),
      _parent_attr_id(parent_attr_id),
      _parent(parent),
      attrs{attr_w(parent_attr_id, parent),
            attr_w(status_attr_id, status_value)} {
  auto status = find_auto_oid(parent, auto_ot, _auto_oid);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               auto_ot,
               "{}.{}:{}: auto object id not found",
               __NS__,
               __func__,
               __LINE__);
    return;
  }
  SWITCH_DEBUG_LOG(logger("construct"));
}

switch_status_t auto_object::create_update() {
  SWITCH_DEBUG_LOG(logger("create_update"));

  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (attrs.size() == 0) {
    return SWITCH_STATUS_SUCCESS;
  }

  CHECK_RET(_parent == 0, SWITCH_STATUS_FAILURE);

  /* if an object is found, just update */
  if (_auto_oid != 0) {
    for (const auto &attr : attrs) {
      status = switch_store::attribute_set(_auto_oid, attr);
      if (status != SWITCH_STATUS_SUCCESS) return status;
    }
    return SWITCH_STATUS_SUCCESS;
  }

  /* if an object is not found, create one */
  return switch_store::object_create(_auto_ot, attrs, _auto_oid);
}

switch_status_t auto_object::create_update_minimal() {
  SWITCH_DEBUG_LOG(logger("create_update_minimal"));

  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (attrs.size() == 0) {
    return SWITCH_STATUS_SUCCESS;
  }

  CHECK_RET(_parent == 0, SWITCH_STATUS_FAILURE);

  /* if an object is not found, create one */
  if (_auto_oid == 0) {
    return switch_store::object_create_minimal(_auto_ot, attrs, _auto_oid);
  }

  return status;
}

switch_status_t auto_object::del() {
  SWITCH_DEBUG_LOG(logger("delete"));
  CHECK_RET(_parent == 0, SWITCH_STATUS_FAILURE);

  if (_auto_oid == 0) return SWITCH_STATUS_SUCCESS;

  return switch_store::object_delete(_auto_oid);
}

void auto_object::logger(const char *operation) {
  ModelInfo *model_info = switch_store::switch_model_info_get();
  switch_log(
      SWITCH_API_LEVEL_DEBUG,
      _auto_ot,
      "factory::auto_object: parent={}, current={}, auto_oid={:#x}, op={}",
      model_info->get_object_name_from_type(
          switch_store::object_type_query(_parent)),
      model_info->get_object_name_from_type(_auto_ot),
      _auto_oid.data,
      operation);
}

switch_status_t auto_object::counters_get(
    const switch_object_id_t oid, std::vector<switch_counter_t> &cntr_ids) {
  (void)oid;
  (void)cntr_ids;
  return SWITCH_STATUS_NOT_SUPPORTED;
}

switch_status_t auto_object::counters_set(const switch_object_id_t oid) {
  (void)oid;
  return SWITCH_STATUS_NOT_SUPPORTED;
}

switch_status_t auto_object::counters_set(
    const switch_object_id_t oid, const std::vector<uint16_t> &cntr_ids) {
  (void)oid;
  (void)cntr_ids;
  return SWITCH_STATUS_NOT_SUPPORTED;
}

switch_status_t auto_object::counters_save(const switch_object_id_t parent) {
  (void)parent;
  return SWITCH_STATUS_NOT_SUPPORTED;
}

switch_status_t auto_object::counters_restore(const switch_object_id_t parent) {
  (void)parent;
  return SWITCH_STATUS_NOT_SUPPORTED;
}

/*******************************************************************************
 * Factory implementations
 ******************************************************************************/

switch_status_t factory::initialize() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  ModelInfo *model_info = switch_store::switch_model_info_get();
  if (model_info == nullptr) {
    status = SWITCH_STATUS_FAILURE;
    return status;
  }

  /*
   * register post-triggers for adds and updates, pre-triggers for deletes to
   * manage auto-objects.
   */
  for (auto it = model_info->begin(); it != model_info->end(); it++) {
    ObjectInfo object_info = *it;
    const auto ot = object_info.object_type;

    status = switch_store::reg_create_trigs_after(ot, &after_create_handler);
    if (status != SWITCH_STATUS_SUCCESS) return status;
    status = switch_store::reg_delete_trigs_before(ot, &before_delete_handler);
    if (status != SWITCH_STATUS_SUCCESS) return status;
    status = switch_store::reg_update_trigs_after(ot, &after_update_handler);
    if (status != SWITCH_STATUS_SUCCESS) return status;
    status = switch_store::reg_counter_get_trigs(ot, &get_counters_handler);
    if (status != SWITCH_STATUS_SUCCESS) return status;
    status = switch_store::reg_counters_set_trigs(ot, &set_counters_handler);
    if (status != SWITCH_STATUS_SUCCESS) return status;
    status =
        switch_store::reg_all_counters_set_trigs(ot, &set_all_counters_handler);
    if (status != SWITCH_STATUS_SUCCESS) return status;
    object_map.push_back(nullptr);
  }

  return status;
}

std::unique_ptr<object> factory::create(switch_object_type_t type,
                                        const switch_object_id_t parent,
                                        switch_status_t &status) {
  std::unique_ptr<object> ret = nullptr;
  status = SWITCH_STATUS_SUCCESS;

  if (type > object_map.size()) {
    status = SWITCH_STATUS_FAILURE;
    return ret;
  }

  if (object_map[type]) {
    ret = object_map[type]->create(parent, status);
    if (status != SWITCH_STATUS_SUCCESS) {
      ret.reset();
      ret = nullptr;
    }
  } else {
    /* return success for objects not implemented */
    status = SWITCH_STATUS_SUCCESS;
    ret = nullptr;
  }

  return ret;
}

/* Create auto objects for user or auto object handle passed in
 * First pass, find all available auto objects and start creating
 * them. If there is a dependency on on another object in the same
 * hierarchy, add it to a queue and do a 2nd pass.
 *
 * The dependent object has to notify if it wants to go on the queue.
 *
 * We could continue to recursively try and resolve more than one
 * dependencies if there is ever more than one. For now, leaving it
 * at a single pass.
 *
 * Example: bd_member can only be created after bd is created for rif
 * If bd_member were also dependent on some XX, we won't resolve it.
 *                         rif
 *                     /      \    \
 *                    bd  bd_member XX
 */
switch_status_t factory::create_auto_objects(
    const switch_object_id_t object_handle) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  ModelInfo *model_info = switch_store::switch_model_info_get();

  // using list as the underlying container is a little better for performance
  std::list<switch_object_type_t> cleanup_list;

  const switch_object_type_t object_type =
      switch_store::object_type_query(object_handle);

  SWITCH_DEBUG_LOG(
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 object_type,
                 "{}.{}:{}: Parent object {}",
                 __NS__,
                 __func__,
                 __LINE__,
                 model_info->get_object_name_from_type(object_type)));

  for (const auto &ref_ot :
       model_info->priority_inverse_refs_get(object_type)) {
    /* skip auto objects based on feature/attribute settings */
    if (switch_store::check_skip_auto_objects_for_object(object_handle,
                                                         ref_ot)) {
      continue;
    }

    /* find table object types referencing this object */
    std::unique_ptr<object> object(
        factory::create(ref_ot, object_handle, status));
    if (object != nullptr) {
      status = object->create_update();
      if (status == SWITCH_STATUS_SUCCESS) {
        cleanup_list.push_back(ref_ot);
      } else {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   ref_ot,
                   "{}.{}:{}: {} create_update failure status {}",
                   __NS__,
                   __func__,
                   __LINE__,
                   model_info->get_object_name_from_type(ref_ot),
                   status);
        goto cleanup;
      }
    } else {
      if (status == SWITCH_STATUS_SUCCESS)
        continue;
      else
        goto cleanup;
    }
  }

  return status;
cleanup:
  switch_status_t fail_status = status;
  std::reverse(cleanup_list.begin(), cleanup_list.end());
  for (const auto ref_ot : cleanup_list) {
    std::unique_ptr<object> object(
        factory::create(ref_ot, object_handle, status));
    if (object != nullptr) {
      status = object->del();
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   ref_ot,
                   "{}.{}:{}: {} create cleanup failure status {}",
                   __NS__,
                   __func__,
                   __LINE__,
                   model_info->get_object_name_from_type(ref_ot),
                   status);
      }
    }
  }
  return fail_status;
}

switch_status_t factory::update_auto_objects(
    const switch_object_id_t previous_object_handle,
    const switch_object_id_t current_object_handle,
    const switch_attr_id_t source_attr_id) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  ModelInfo *model_info = switch_store::switch_model_info_get();
  const switch_object_type_t current_ot =
      switch_store::object_type_query(current_object_handle);

  // context needs to be cleared after each invocation
  thread_local std::set<switch_object_type_t> context;

  /* go over object types refering to this (attr id) */
  const auto dep_ots = model_info->dep_ots_get(source_attr_id);
  const auto dep_path_ots = model_info->dep_path_ots_get(source_attr_id);

  if (dep_ots.find(current_ot) != dep_ots.end()) {
    /* found! */
    std::unique_ptr<object> object(
        factory::create(current_ot, previous_object_handle, status));
    if (object != nullptr) {
      status = object->create_update();
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   current_ot,
                   "{}.{}:{}: {} create_update failure status {}",
                   __NS__,
                   __func__,
                   __LINE__,
                   model_info->get_object_name_from_type(current_ot),
                   status);
        return status;
      }
    }
  }

  const auto ref_ots = model_info->priority_inverse_refs_get(
      current_ot); /* OTs that reference current OT */

  for (const auto ref_ot : ref_ots) {
    /* skip auto objects based on feature/attribute settings */
    if (switch_store::check_skip_auto_objects_for_object(current_object_handle,
                                                         ref_ot)) {
      continue;
    }

    if (dep_path_ots.find(ref_ot) != dep_path_ots.end()) {
      const auto ret = context.insert(ref_ot);
      if (ret.second == true) {
        /* ref ot is in the path of dependencies. find any instances and
         * recurse */
        std::set<switch_object_id_t> ref_oids;
        status |= switch_store::referencing_set_get(
            current_object_handle, ref_ot, ref_oids);
        if (status != SWITCH_STATUS_SUCCESS) {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     ref_ot,
                     "{}.{}:{}: {} ref set failure status {}",
                     __NS__,
                     __func__,
                     __LINE__,
                     model_info->get_object_name_from_type(ref_ot),
                     status);
          goto end;
        }

        for (const auto ref_oid : ref_oids) {
          status = update_auto_objects(
              current_object_handle, ref_oid, source_attr_id);
          if (status != SWITCH_STATUS_SUCCESS) {
            switch_log(SWITCH_API_LEVEL_ERROR,
                       ref_ot,
                       "{}.{}:{}: {} update failure status {}",
                       __NS__,
                       __func__,
                       __LINE__,
                       model_info->get_object_name_from_type(ref_ot),
                       status);
            goto end;
          }
        }
        const auto ret_erase = context.erase(ref_ot);
        if (ret_erase != 1) {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     ref_ot,
                     "{}.{}:{}: {} update failure status {}",
                     __NS__,
                     __func__,
                     __LINE__,
                     model_info->get_object_name_from_type(ref_ot),
                     ret_erase);
          status = SWITCH_STATUS_FAILURE;
          goto end;
        }
      }
    }
  }

  return status;
end:
  context.clear();
  return status;
}
switch_status_t factory::delete_auto_objects(
    const switch_object_id_t object_handle) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  ModelInfo *model_info = switch_store::switch_model_info_get();
  const switch_object_type_t object_type =
      switch_store::object_type_query(object_handle);

  const auto ref_ots = model_info->priority_inverse_refs_get(object_type);
  // for (const auto ref_ot : std::reverse(ref_ots.begin(), ref_ots.end())) {
  for (auto rit = ref_ots.rbegin(); rit != ref_ots.rend(); ++rit) {
    /* skip auto objects based on feature/attribute settings */
    if (switch_store::check_skip_auto_objects_for_object(object_handle, *rit)) {
      continue;
    }

    /* find table object types referencing this object */
    std::unique_ptr<object> object(
        factory::create(*rit, object_handle, status));
    if (object != nullptr) {
      status = object->del();
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   *rit,
                   "{}.{}:{}: {} obj delete failure status {}",
                   __NS__,
                   __func__,
                   __LINE__,
                   model_info->get_object_name_from_type(*rit),
                   status);
        return status;
      }
    } else if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 *rit,
                 "{}.{}:{}: {} delete failure status {}",
                 __NS__,
                 __func__,
                 __LINE__,
                 model_info->get_object_name_from_type(*rit),
                 status);
    }
  }
  return status;
}

switch_status_t factory::get_counters(const switch_object_id_t object_handle,
                                      std::vector<switch_counter_t> &cntrs) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  ModelInfo *model_info = switch_store::switch_model_info_get();
  const switch_object_type_t object_type =
      switch_store::object_type_query(object_handle);
  const ObjectInfo *object_info = model_info->get_object_info(object_type);
  if (object_info == nullptr) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               object_type,
               "{}.{}:{}: failed to get object info type: {}",
               __NS__,
               __func__,
               __LINE__,
               object_type);
    return SWITCH_STATUS_FAILURE;
  }
  if (object_info->get_counter()) {
    const AttributeMetadata *attr_md =
        object_info->get_attr_metadata(object_info->get_counter_attr_id());
    const ValueMetadata *value_md = attr_md->get_value_metadata();
    cntrs.reserve(value_md->get_enum_metadata().size());
    switch_counter_t cntr = {.counter_id = 0, .count = 0};
    for (size_t i = 0; i < value_md->get_enum_metadata().size(); i++) {
      cntr.counter_id = i;
      cntrs.push_back(cntr);
    }
  } else {
    return SWITCH_STATUS_NOT_SUPPORTED;
  }
  for (const auto ref_ot : model_info->counter_refs_get(object_type)) {
    std::unique_ptr<object> object(
        factory::create(ref_ot, object_handle, status));
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 ref_ot,
                 "{}.{}:{}: factory::create({}) failed with status {}",
                 __NS__,
                 __func__,
                 __LINE__,
                 model_info->get_object_name_from_type(ref_ot),
                 status);
      return status;
    }

    // If status == SWITCH STATUS_SUCCESS
    // object should be not nullptr,
    // but to be safe in case ASIC object
    // constructor is written in improper way
    if (object != nullptr) {
      status = object->counters_get(object_handle, cntrs);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   ref_ot,
                   "{}.{}:{}: {} counters_get failed with status {}",
                   __NS__,
                   __func__,
                   __LINE__,
                   model_info->get_object_name_from_type(ref_ot),
                   status);
        return status;
      }
    }
  }
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t factory::set_counters(const switch_object_id_t object_handle,
                                      const std::vector<uint16_t> &cntr_ids) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  const switch_object_type_t object_type =
      switch_store::object_type_query(object_handle);
  ModelInfo *model_info = switch_store::switch_model_info_get();

  if (cntr_ids.size() == 0) {
    return status;
  }

  SWITCH_DEBUG_LOG(
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 object_type,
                 "{}.{}:{}: Parent object {}",
                 __NS__,
                 __func__,
                 __LINE__,
                 model_info->get_object_name_from_type(object_type)));

  for (const auto ref_ot : model_info->counter_refs_get(object_type)) {
    switch_status_t rc = SWITCH_STATUS_SUCCESS;
    std::unique_ptr<object> object(factory::create(ref_ot, object_handle, rc));
    if (rc != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 ref_ot,
                 "{}.{}:{}: factory::create({}) failed with status {}",
                 __NS__,
                 __func__,
                 __LINE__,
                 model_info->get_object_name_from_type(ref_ot),
                 rc);
      status = SWITCH_STATUS_FAILURE;
      continue;
    }

    if (object != nullptr) {
      rc = object->counters_set(object_handle, cntr_ids);
      if (rc != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   ref_ot,
                   "{}.{}:{}: {} counters_set failed with status {}",
                   __NS__,
                   __func__,
                   __LINE__,
                   model_info->get_object_name_from_type(ref_ot),
                   rc);
        status = SWITCH_STATUS_FAILURE;
      }
    }
  }

  return status;
}

switch_status_t factory::set_all_counters(
    const switch_object_id_t object_handle) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  const switch_object_type_t object_type =
      switch_store::object_type_query(object_handle);
  ModelInfo *model_info = switch_store::switch_model_info_get();

  SWITCH_DEBUG_LOG(
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 object_type,
                 "{}.{}:{}: Parent object {}",
                 __NS__,
                 __func__,
                 __LINE__,
                 model_info->get_object_name_from_type(object_type)));

  for (const auto ref_ot : model_info->counter_refs_get(object_type)) {
    std::unique_ptr<object> object(
        factory::create(ref_ot, object_handle, status));
    if (object != nullptr) {
      status = object->counters_set(object_handle);
    }
  }
  return status;
}

switch_status_t factory::get_counters_handler(
    const switch_object_id_t object_id, std::vector<switch_counter_t> &cntrs) {
  return get_instance().get_counters(object_id, cntrs);
}
switch_status_t factory::set_counters_handler(
    const switch_object_id_t object_id,
    const std::vector<uint16_t> &cntrs_ids) {
  return get_instance().set_counters(object_id, cntrs_ids);
}
switch_status_t factory::set_all_counters_handler(
    const switch_object_id_t object_id) {
  return get_instance().set_all_counters(object_id);
}
switch_status_t factory::after_create_handler(
    const switch_object_id_t object_id, const std::set<attr_w> &attrs) {
  (void)attrs;
  if (switch_store::smiContext::context().in_warm_init())
    return SWITCH_STATUS_SUCCESS;
  return get_instance().create_auto_objects(object_id);
}
switch_status_t factory::create_auto_objects_warm_init(
    const switch_object_id_t object_id) {
  return get_instance().create_auto_objects(object_id);
}
switch_status_t factory::before_delete_handler(
    const switch_object_id_t object_id) {
  return get_instance().delete_auto_objects(object_id);
}
switch_status_t factory::after_update_handler(
    const switch_object_id_t object_id, const attr_w &attr) {
  if (switch_store::smiContext::context().in_warm_init())
    return SWITCH_STATUS_SUCCESS;
  const switch_object_id_t p = {};
  return get_instance().update_auto_objects(p, object_id, attr.id_get());
}

// Recursively create auto objects for a given user object
switch_status_t factory_create_all_auto_objects(
    const switch_object_id_t object_handle) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  ModelInfo *model_info = switch_store::switch_model_info_get();
  const switch_object_type_t object_type =
      switch_store::object_type_query(object_handle);

  SWITCH_DEBUG_LOG(
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 object_type,
                 "{}.{}:{}: Parent object {}",
                 __NS__,
                 __func__,
                 __LINE__,
                 model_info->get_object_name_from_type(object_type)));

  status = factory::get_instance().create_auto_objects_warm_init(object_handle);
  for (const auto ref_ot : model_info->priority_inverse_refs_get(object_type)) {
    /* skip auto objects based on feature/attribute settings */
    if (switch_store::check_skip_auto_objects_for_object(object_handle,
                                                         ref_ot)) {
      continue;
    }

    std::set<switch_object_id_t> ref_oids;
    status = switch_store::referencing_set_get(object_handle, ref_ot, ref_oids);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 ref_ot,
                 "{}.{}:{}: {} ref set failure status {}",
                 __NS__,
                 __func__,
                 __LINE__,
                 model_info->get_object_name_from_type(ref_ot),
                 status);
      return status;
    }

    const ObjectInfo *object_info = model_info->get_object_info(ref_ot);
    if (object_info == nullptr) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 ref_ot,
                 "{}.{}:{}: failed to get object info type: {}",
                 __NS__,
                 __func__,
                 __LINE__,
                 ref_ot);
      return SWITCH_STATUS_FAILURE;
    }
    if (object_info->get_object_class() == OBJECT_CLASS_USER) continue;

    for (const auto ref_oid : ref_oids) {
      status = factory_create_all_auto_objects(ref_oid);
    }
  }
  return status;
}

switch_status_t factory_init() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  ModelInfo *model_info = switch_store::switch_model_info_get();
  if (model_info == nullptr) {
    status = SWITCH_STATUS_FAILURE;
    return status;
  }

  status = factory::get_instance().initialize();

  return status;
}

switch_status_t factory_clean() {
  factory::get_instance().clear_object_map();
  return SWITCH_STATUS_SUCCESS;
}

}  // namespace smi
