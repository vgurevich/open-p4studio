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


#ifndef INCLUDE_S3_META_META_H__
#define INCLUDE_S3_META_META_H__

#include <memory>
#include <vector>
#include <set>
#include <unordered_map>
#include <string>
#include <cassert>

#include "bf_switch/bf_switch_types.h"

/* Maximum objects an auto object is allowed to be a child of */
#define MAX_ALLOWED_OBJECTS 10

namespace smi {

typedef struct _switch_attr_flags_t {
  bool is_mandatory;    // Attribute required during object create, modifiable
                        // later if is_create_only is false
  bool is_create_only;  // Attribute allowed only during object create and not
                        // modifiable later
  // if both mandatory and create_only flags are specified, then the attribute
  // cannot be modified later
  bool is_immutable;  // Attribute not allowed at create or update
  bool is_read_only;  // Attribute is read only
  bool is_internal;   // Internal attribute and not modifiable
  bool is_counter;    // Special attribute to query counter
  bool re_evaluate;   // Special attribute to trigger auto object CRUD for the
                      // object referred by this attribute
  bool is_status;     // indicates this is a status attribute in auto object
} switch_attr_flags_t;

typedef enum _switch_object_class_t {
  OBJECT_CLASS_NONE = 0,
  OBJECT_CLASS_AUTO,
  OBJECT_CLASS_USER,
  OBJECT_CLASS_CONTEXT
} switch_object_class_t;

class EnumMetadata {
 public:
  uint64_t enum_value;
  std::string enum_name;
  std::string enum_name_fqn;
};

class ObjectTypePrio {
 public:
  switch_object_type_t object_type;
  uint16_t prio;
};
struct priority_comparer {
  bool operator()(const ObjectTypePrio &lhs, const ObjectTypePrio &rhs) const {
    return lhs.prio >= rhs.prio;
  }
};

class ObjectAttrPair {
 public:
  switch_object_type_t object_type;
  switch_attr_id_t attr_id;
};

class KeyGroup {
 public:
  std::vector<switch_attr_id_t> attr_list;
};

class CliMetadata {
 private:
  std::vector<ObjectAttrPair> key_attrs_list;
  std::vector<switch_attr_id_t> table_view_attr_list;

 public:
  void add_to_key_attrs(ObjectAttrPair mem_pair) {
    key_attrs_list.push_back(mem_pair);
    return;
  }
  void add_to_table_view_attr_list(switch_attr_id_t attr_id) {
    table_view_attr_list.push_back(attr_id);
    return;
  }
  const std::vector<ObjectAttrPair> &get_cli_key_attrs_list() const {
    return key_attrs_list;
  }
  const std::vector<switch_attr_id_t> &get_table_view_attr_list() const {
    return table_view_attr_list;
  }
};

class ValueMetadata {
 private:
  std::vector<switch_object_type_t> allowed_object_types;
  switch_attribute_value_t default_value;
  std::vector<EnumMetadata> enums;
  size_t max_enum_width;

 public:
  // This is different from the AttributeMetadata type. It holds the type of the
  // composite types like lists
  switch_attr_type_t type;
  ValueMetadata() {
    type = SWITCH_TYPE_UINT64;
    default_value.type = SWITCH_TYPE_UINT64;
    max_enum_width = 0;
  }
  // Get
  bool is_allowed_object_type(const switch_object_type_t ot) const {
    for (const auto object_type : allowed_object_types) {
      if (ot == object_type) return true;
    }
    return false;
  }
  bool is_allowed_enum_value(const uint64_t value) const {
    for (const auto &values : enums) {
      if (value == values.enum_value) return true;
    }
    return false;
  }
  const std::vector<EnumMetadata> &get_enum_metadata() const { return enums; }
  const std::vector<switch_object_type_t> &get_allowed_object_types() const {
    return allowed_object_types;
  }
  const switch_attribute_value_t &get_default_value() const {
    return default_value;
  }

  const size_t &get_max_enum_width() const { return max_enum_width; }

  // Set
  void add_enum_metadata(EnumMetadata em) {
    enums.push_back(em);
    if (em.enum_name.size() > max_enum_width)
      max_enum_width = em.enum_name.size();
    return;
  }
  void set_default_value(switch_attribute_value_t value) {
    default_value = value;
    return;
  }
  void add_allowed_object_types(switch_object_type_t ot) {
    allowed_object_types.push_back(ot);
  }
};

class AttributeMetadata {
 private:
  switch_object_type_t parent_object_type;
  std::string attr_name;
  std::string attr_name_fqn;
  std::string attr_desc;
  switch_attr_flags_t flags;
  ValueMetadata value_md;

 public:
  switch_attr_id_t attr_id;
  switch_attr_type_t type;
  // Set
  void set_attr_name(char *name) {
    attr_name = name;
    return;
  }
  void set_attr_name_fqn(std::string name) {
    attr_name_fqn = name;
    return;
  }
  void set_parent_object_type(switch_object_type_t ot) {
    parent_object_type = ot;
    return;
  }
  void set_flags(switch_attr_flags_t in_flags) {
    flags = in_flags;
    return;
  }
  void set_description(char *desc) {
    attr_desc = desc;
    return;
  }

  // Get
  bool is_allowed_enum_value(const uint64_t value) const {
    return value_md.is_allowed_enum_value(value);
  }
  std::string get_attr_name() const { return attr_name; }
  std::string get_attr_name_fqn() const { return attr_name_fqn; }
  std::string get_attr_desc() const { return attr_desc; }
  switch_object_type_t get_parent_object_type() const {
    return parent_object_type;
  }
  const switch_attr_flags_t &get_flags() const { return flags; }
  const ValueMetadata *get_value_metadata() const { return &value_md; }
  ValueMetadata *get_value_metadata_for_update() { return &value_md; }
};

class ObjectInfo {
 private:
  std::string object_name;
  std::string object_name_fqn;
  switch_object_class_t object_class;
  std::string object_desc;
  std::vector<AttributeMetadata> attribute_metadata_list;
  std::vector<ObjectAttrPair> dependency_list;
  std::vector<ObjectAttrPair> membership_list;
  std::vector<KeyGroup> key_groups;
  bool has_counter;
  // id and stats are attrs that go into counter tag
  switch_attr_id_t counter_attr_id;
  switch_attr_id_t counter_attr_stats;
  CliMetadata cli_md;
  bool has_stats_cache;

 public:
  switch_object_type_t object_type;
  uint16_t auto_obj_prio = 0;
  // Set
  void set_object_name(char const *obj_name) {
    object_name = obj_name;
    return;
  }
  void set_object_name_fqn(std::string obj_name_fqn) {
    object_name_fqn = obj_name_fqn;
    return;
  }
  void set_object_class(switch_object_class_t obj_class) {
    object_class = obj_class;
    return;
  }
  void set_object_desc(char *desc) {
    object_desc = desc;
    return;
  }
  void set_counter(bool val) {
    has_counter = val;
    return;
  }
  void set_stats_cache(bool val) {
    has_stats_cache = val;
    return;
  }
  void set_counter_attr_id(switch_attr_id_t attr_id) {
    counter_attr_id = attr_id;
    return;
  }
  void set_counter_attr_stats(switch_attr_id_t attr_id) {
    counter_attr_stats = attr_id;
    return;
  }
  void add_to_attribute_metadata_list(AttributeMetadata &attr_md) {
    attribute_metadata_list.push_back(attr_md);
    return;
  }
  void add_to_dependency_list(ObjectAttrPair dep_pair) {
    dependency_list.push_back(dep_pair);
    return;
  }
  void add_to_membership_list(ObjectAttrPair mem_pair) {
    membership_list.push_back(mem_pair);
    return;
  }
  void add_to_key_groups(KeyGroup kg) {
    key_groups.push_back(kg);
    return;
  }

  // Get
  inline std::string get_object_name() const { return object_name; }
  inline std::string get_object_name_fqn() const { return object_name_fqn; }
  inline switch_object_class_t get_object_class() const { return object_class; }
  inline std::string get_object_desc() const { return object_desc; }
  inline bool get_counter() const { return has_counter; }
  inline bool get_stats_cache() const { return has_stats_cache; }
  inline switch_attr_id_t get_counter_attr_id() const {
    return counter_attr_id;
  }
  inline switch_attr_id_t get_counter_attr_stats() const {
    return counter_attr_stats;
  }
  switch_attr_id_t get_attr_id_from_name(std::string attr_name) const {
    for (const auto &attr_md : attribute_metadata_list) {
      if (attr_name.compare(attr_md.get_attr_name()) == 0)
        return attr_md.attr_id;
    }
    return 0;
  }
  inline const AttributeMetadata *get_attr_metadata(
      switch_attr_id_t attr_id) const {
    for (const auto &attr_md : attribute_metadata_list) {
      if (attr_id == attr_md.attr_id) return &attr_md;
    }
    return NULL;
  }
  const AttributeMetadata *get_attr_metadata_from_name(
      std::string attr_name) const {
    for (const auto &attr_md : attribute_metadata_list) {
      if (attr_name.compare(attr_md.get_attr_name()) == 0) return &attr_md;
    }
    return NULL;
  }
  AttributeMetadata *get_attr_metadata_by_name(std::string attr_name) {
    for (auto &attr_md : attribute_metadata_list) {
      if (attr_name.compare(attr_md.get_attr_name()) == 0) return &attr_md;
    }
    return NULL;
  }
  inline const std::vector<AttributeMetadata> &get_attribute_list() const {
    return attribute_metadata_list;
  }
  inline const std::vector<ObjectAttrPair> &get_dependency_list() const {
    return dependency_list;
  }
  inline const std::vector<ObjectAttrPair> &get_membership_list() const {
    return membership_list;
  }
  const std::vector<KeyGroup> &get_key_groups() const { return key_groups; }
  CliMetadata *get_cli_metadata_mutable() { return &cli_md; }
  const CliMetadata *get_cli_metadata() const { return &cli_md; }
};

class ModelInfo {
 private:
  uint32_t version = 0;
  // ObjectMap is indexed based on switch_object_type_t, since the ids are
  // contiguous
  typedef std::vector<ObjectInfo> ObjectMap;
  ObjectMap objects;
  // type_id_name_map stores a map of Object FQN Str:switch_object_type_t, Attr
  // FQN Str:switch_attr_id_t. Since both
  // OT and attr_id share the same number space, we use only one map. In future
  // if this design changes then this will
  // need to be reworked
  std::unordered_map<std::string, uint64_t> id_name_map;
  std::unordered_map<switch_attr_id_t, switch_object_type_t> attr_ot_map;

  // object references
  std::unordered_map<switch_object_type_t, std::set<switch_object_type_t>> refs;
  std::unordered_map<switch_object_type_t, std::set<switch_object_type_t>>
      inverse_refs;
  std::unordered_map<switch_object_type_t, std::vector<switch_object_type_t>>
      priority_inverse_refs;
  std::unordered_map<switch_object_type_t, std::set<switch_object_type_t>>
      counter_refs;
  // object attribute dependencies
  std::unordered_map<switch_attr_id_t, std::set<switch_object_type_t>> dep_ots;
  std::unordered_map<switch_attr_id_t, std::set<switch_object_type_t>>
      dep_path_ots;

  std::set<switch_object_type_t> empty_set = {};
  std::vector<switch_object_type_t> empty_vector = {};

  // takes an attribute metadata, returns set of referenced object type
  switch_status_t populate_object_types_from_attr(
      const ValueMetadata *value_md,
      std::set<switch_object_type_t> &object_types) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (value_md->type == SWITCH_TYPE_OBJECT_ID) {
      for (auto ot : value_md->get_allowed_object_types()) {
        object_types.insert(ot);
      }
    } else if (value_md->type == SWITCH_TYPE_LIST) {
      for (auto ot : value_md->get_allowed_object_types()) {
        object_types.insert(ot);
      }
    }

    return status;
  }

  // recursively try to find satisfying attribute, subscribe all object types in
  // path to updates for target attribute
  switch_status_t recur_find_attr(const switch_object_type_t current_node,
                                  const switch_object_type_t target_ot,
                                  const switch_attr_id_t target_attr) {
    switch_status_t status = SWITCH_STATUS_FAILURE;
    thread_local std::set<switch_object_type_t> context;
    bool found_parent_path = false;
    // found
    if (current_node == target_ot) return SWITCH_STATUS_SUCCESS;

    // if target_ot is not a ref for current_node, nothing to do here
    const ObjectInfo *object_info = get_object_info(current_node);
    if (object_info == nullptr) return SWITCH_STATUS_FAILURE;
    if (object_info->get_object_class() == OBJECT_CLASS_USER &&
        refs[current_node].find(target_ot) == refs[current_node].end()) {
      return SWITCH_STATUS_SUCCESS;
    }

    // any referenced nodes satisfy, subscribe current node and return success
    for (const auto ot : refs[current_node]) {
      const auto ret = context.insert(ot);
      if (ret.second == true) {
        status = recur_find_attr(ot, target_ot, target_attr);
        if (status == SWITCH_STATUS_SUCCESS) {
          // atleast one referenced nodes satisfied
          found_parent_path = true;
          dep_path_ots[target_attr].insert(current_node);
        }
        const auto ret_erase = context.erase(ot);
        if (ret_erase != 1) return SWITCH_STATUS_FAILURE;
      }
    }
    if (found_parent_path) return SWITCH_STATUS_SUCCESS;
    return status;
  }

  switch_status_t compute_dag_internal() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    for (const auto &object_info : objects) {
      for (const auto &attr_md : object_info.get_attribute_list()) {
        // if this attr is a counter attribute, add ref in counter_refs
        if (object_info.get_counter_attr_stats() == attr_md.attr_id) {
          status = populate_object_types_from_attr(
              attr_md.get_value_metadata(),
              counter_refs[object_info.object_type]);
        } else {
          status = populate_object_types_from_attr(
              attr_md.get_value_metadata(), refs[object_info.object_type]);
        }
        assert(status == SWITCH_STATUS_SUCCESS);
      }
    }

    // populate inverse dag
    for (const auto &ot_set : refs) {
      for (const auto ot : ot_set.second) {
        inverse_refs[ot].insert(ot_set.first);
      }
    }

    // build the prioritized inverse references
    for (const auto &irefs : inverse_refs) {
      std::set<ObjectTypePrio, priority_comparer> auto_obj_priority_list;
      for (const auto ot : irefs.second) {
        ObjectTypePrio otp = {};
        const ObjectInfo *info = this->get_object_info(ot);
        if (info == nullptr) continue;
        otp.object_type = ot;
        otp.prio = info->auto_obj_prio;
        auto_obj_priority_list.insert(otp);
      }
      for (const auto otp : auto_obj_priority_list) {
        priority_inverse_refs[irefs.first].push_back(otp.object_type);
      }
    }

    // a user object will never have a dependency list
    for (const auto &object_info : objects) {
      for (const auto &dependency : object_info.get_dependency_list()) {
        dep_ots[dependency.attr_id].insert(object_info.object_type);
        // object_info.object_type is for an auto object
        // dependency.objecy_type is a user object
        recur_find_attr(object_info.object_type,
                        dependency.object_type,
                        dependency.attr_id);
      }
    }
    return status;
  }

 public:
  size_t object_name_max_len = 0;
  size_t attr_name_max_len = 0;
  typedef ObjectMap::iterator Iterator;
  Iterator begin() { return objects.begin(); }
  Iterator end() { return objects.end(); }
  void set_version(uint32_t ver) {
    version = ver;
    return;
  }

  inline const ObjectMap &get_objects() const { return objects; }

  switch_status_t compute_directed_object_graph() {
    return compute_dag_internal();
  }
  void add_object_info_entry(ObjectInfo &object_info) {
    objects.push_back(object_info);
    return;
  }
  void update_attr_to_ot_map(switch_attr_id_t attr_id,
                             switch_object_type_t object_type) {
    attr_ot_map[attr_id] = object_type;
  }
  uint32_t get_version() { return version; }
  const ObjectInfo *get_object_info(
      const switch_object_type_t object_type) const {
    if (object_type == 0) return nullptr;
    if (object_type >= objects.size()) return nullptr;
    return &objects[object_type];
  }
  std::string get_object_name_from_type(switch_object_type_t object_type) {
    if (object_type == 0) return "invalid";
    if (object_type >= objects.size()) return "invalid";
    return objects[object_type].get_object_name();
  }
  const ObjectInfo *get_object_info_from_name(
      const std::string &object_name) const {
    for (const auto &object : objects) {
      if (object_name.compare(object.get_object_name()) == 0) return &object;
    }
    return NULL;
  }
  ObjectInfo *get_object_info_from_name_for_update(std::string object_name) {
    for (auto &object : objects) {
      if (object_name.compare(object.get_object_name()) == 0) return &object;
    }
    return NULL;
  }
  switch_object_type_t get_object_type_from_attr_id(switch_attr_id_t attr_id) {
    auto it = attr_ot_map.find(attr_id);
    if (it != attr_ot_map.end()) return it->second;
    return 0;
  }

  // object references
  const std::set<switch_object_type_t> &refs_get(
      const switch_object_type_t ot) const {
    auto it = refs.find(ot);
    if (it != refs.end()) return it->second;
    return empty_set;
  }
  const std::set<switch_object_type_t> &inverse_refs_get(
      const switch_object_type_t ot) const {
    auto it = inverse_refs.find(ot);
    if (it != inverse_refs.end()) return it->second;
    return empty_set;
  }
  const std::vector<switch_object_type_t> &priority_inverse_refs_get(
      const switch_object_type_t ot) const {
    auto it = priority_inverse_refs.find(ot);
    if (it != priority_inverse_refs.end()) return it->second;
    return empty_vector;
  }
  const std::set<switch_object_type_t> &counter_refs_get(
      const switch_object_type_t ot) const {
    auto it = counter_refs.find(ot);
    if (it != counter_refs.end()) return it->second;
    return empty_set;
  }
  // object attribute dependencies
  const std::set<switch_object_type_t> &dep_ots_get(
      const switch_attr_id_t attr_id) const {
    auto it = dep_ots.find(attr_id);
    if (it != dep_ots.end()) return it->second;
    return empty_set;
  }
  const std::set<switch_object_type_t> &dep_path_ots_get(
      const switch_attr_id_t attr_id) const {
    auto it = dep_path_ots.find(attr_id);
    if (it != dep_path_ots.end()) return it->second;
    return empty_set;
  }

  // Object type/ attribute Id: FQN mappings
  void add_id_name_mapping(const std::string name, const uint64_t id) {
    id_name_map[name] = id;
    return;
  }
  uint64_t get_id_from_name(const std::string name) {
    auto it = id_name_map.find(name);
    if (it != id_name_map.end()) return it->second;
    return 0;
  }
};

std::unique_ptr<ModelInfo> build_model_info_from_file(const char *config_file,
                                                      bool verbose);

}  // namespace smi

#endif  // INCLUDE_S3_META_META_H__
