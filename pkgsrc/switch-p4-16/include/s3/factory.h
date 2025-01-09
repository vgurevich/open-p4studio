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


#ifndef INCLUDE_S3_FACTORY_H__
#define INCLUDE_S3_FACTORY_H__

#include <memory>
#include <map>
#include <vector>
#include <set>
#include <utility>

#include "bf_switch/bf_switch_types.h"

namespace smi {

switch_status_t find_auto_oid(const switch_object_id_t parent_oid,
                              const switch_object_type_t object_type,
                              switch_object_id_t &auto_oid);

class object {
 public:
  virtual switch_status_t create_update() = 0;
  virtual switch_status_t del() = 0;
  virtual switch_status_t counters_get(const switch_object_id_t oid,
                                       std::vector<switch_counter_t> &cntrs) {
    (void)oid;
    (void)cntrs;
    return 0;
  }
  virtual switch_status_t counters_set(const switch_object_id_t oid) {
    (void)oid;
    return 0;
  }
  virtual switch_status_t counters_set(const switch_object_id_t oid,
                                       const std::vector<uint16_t> &cntr_ids) {
    (void)oid;
    (void)cntr_ids;
    return 0;
  }

  virtual switch_status_t counters_save(const switch_object_id_t parent) {
    (void)parent;
    return 0;
  }
  virtual switch_status_t counters_restore(const switch_object_id_t parent) {
    (void)parent;
    return 0;
  }

  virtual ~object() {}
};

class auto_object : public object {
 protected:
  const switch_object_type_t _auto_ot;
  const switch_attr_id_t _parent_attr_id;
  switch_object_id_t _parent;
  switch_object_id_t _auto_oid;

 public:
  auto_object(const switch_object_type_t auto_ot,
              const switch_attr_id_t parent_attr_id,
              const switch_object_id_t parent);
  auto_object(const switch_object_type_t auto_ot,
              const switch_attr_id_t parent_attr_id,
              const switch_object_id_t parent,
              const switch_attr_id_t status_attr_id,
              bool status_value);
  ~auto_object() {}
  switch_status_t create_update();
  switch_status_t create_update_minimal();
  switch_status_t del();
  switch_status_t counters_get(const switch_object_id_t oid,
                               std::vector<switch_counter_t> &attrs);
  switch_status_t counters_set(const switch_object_id_t oid);
  switch_status_t counters_set(const switch_object_id_t oid,
                               const std::vector<uint16_t> &cntr_ids);
  switch_object_id_t get_auto_oid() { return _auto_oid; }
  switch_object_id_t get_parent() { return _parent; }
  switch_status_t counters_save(const switch_object_id_t parent);
  switch_status_t counters_restore(const switch_object_id_t parent);
  std::set<attr_w> attrs;
  void logger(const char *op);
  void clear_attrs() { attrs.clear(); }
};

// A basic interface using which the derived classes instantiate a class
class ObjectCreator {
 public:
  ObjectCreator() {}
  virtual ~ObjectCreator() {}
  virtual std::unique_ptr<object> create(const switch_object_id_t parent,
                                         switch_status_t &status) = 0;
};

/*
 * A creator class per auto class
 * Every time a new class is defined it must also be accompanied by its own
 * specialized Creator derived class that is responsible for instantiating
 * instances of the new class. The template type T is the auto class.
 * The factory calls the create member to intantiate a new auto class
 */
template <class T>
class CreatorImpl : public ObjectCreator {
 public:
  CreatorImpl(const switch_object_type_t object_type) : ObjectCreator() {
    (void)object_type;
  }
  ~CreatorImpl() {}
  std::unique_ptr<object> create(const switch_object_id_t parent,
                                 switch_status_t &status) {
    return std::unique_ptr<T>(new T(parent, status));
  }
};

/*
 * This is a dynamic implementation of the factory pattern, Each class registers
 * itself with an object type which is saved in a map. At runtime, the correct
 * class instance is created based on the object type requested
 */
class factory {
 public:
  factory() {}
  ~factory() {}
  switch_status_t initialize();
  std::unique_ptr<object> create(switch_object_type_t type,
                                 const switch_object_id_t parent,
                                 switch_status_t &status);
  // register the auto class using a creator class instance
  template <typename T>
  void register_object(const switch_object_type_t type) {
    std::unique_ptr<ObjectCreator> x_obj =
        std::unique_ptr<ObjectCreator>(new CreatorImpl<T>(type));
    object_map[type] = std::move(x_obj);
  }
  void clear_object_map() {
    for (auto &obj : object_map) obj = nullptr;
  }
  static factory &get_instance() {
    static factory instance;
    return instance;
  }
  static switch_status_t get_counters_handler(
      const switch_object_id_t object_id, std::vector<switch_counter_t> &cntrs);
  static switch_status_t set_counters_handler(
      const switch_object_id_t object_id,
      const std::vector<uint16_t> &cntrs_ids);
  static switch_status_t set_all_counters_handler(
      const switch_object_id_t object_id);
  static switch_status_t after_create_handler(
      const switch_object_id_t object_id, const std::set<attr_w> &attrs);
  static switch_status_t before_delete_handler(
      const switch_object_id_t object_id);
  static switch_status_t after_update_handler(
      const switch_object_id_t object_id, const attr_w &attr);
  switch_status_t create_auto_objects_warm_init(
      const switch_object_id_t object_handle);

 private:
  switch_status_t get_counters(const switch_object_id_t object_handle,
                               std::vector<switch_counter_t> &cntrs);
  switch_status_t set_counter(const switch_object_id_t object_handle);
  switch_status_t set_counters(const switch_object_id_t object_handle,
                               const std::vector<uint16_t> &cntr_ids);
  switch_status_t set_all_counters(const switch_object_id_t object_handle);
  switch_status_t create_auto_objects(const switch_object_id_t object_handle);
  switch_status_t update_auto_objects(
      const switch_object_id_t previous_object_handle,
      const switch_object_id_t current_object_handle,
      const switch_attr_id_t source_attr_id);
  switch_status_t delete_auto_objects(const switch_object_id_t object_handle);
  std::vector<std::unique_ptr<ObjectCreator>> object_map;
};

// This memory is deallocated in factory::clear_object_map
#define REGISTER_OBJECT(x, y)                      \
  do {                                             \
    factory::get_instance().register_object<x>(y); \
  } while (0)

switch_status_t factory_init();
switch_status_t factory_clean();
switch_status_t factory_create_all_auto_objects(
    const switch_object_id_t object_handle);

}  // namespace smi
#endif  // INCLUDE_S3_FACTORY_H__
