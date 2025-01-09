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


#include <unistd.h>
#include <store.h>

#include <fstream>
#include <vector>
#include <string>
#include <sstream>

#include "s3/meta/meta.h"
#include "s3/attribute_util.h"
#include "s3/switch_store.h"
#include "./log.h"

namespace smi {
namespace db {

using ::smi::logging::switch_log;

db_store *object_attr_hash = nullptr;
const db_store *get_db() { return object_attr_hash; }
std::vector<switch_object_id_t> ordered_create_list;
const std::vector<switch_object_id_t> &get_creation_list() {
  return ordered_create_list;
}

/* Mutex to protect the primary db_store. There is no global lock held anywhere
 * else and this is the only mutex which will provide concurrency
 */
std::recursive_mutex db_mtx;

class topoSort {
  std::unordered_map<switch_object_id_t, bool> vertices;
  std::unordered_map<switch_object_id_t, std::set<switch_object_id_t>> edges;

 public:
  void addEdge(const switch_object_id_t vertex, const switch_object_id_t edge) {
    edges[vertex].insert(edge);
    vertices[vertex] = false;
  }

  // A recursive function used by goSort
  void topologicalSort(const switch_object_id_t v) {
    if (v.data == 0) return;

    if (vertices.find(v) != vertices.end()) {
      // Mark the current node as visited.
      vertices[v] = true;
    } else {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OT_NONE,
                 "{}.{}: Invalid vertex {} in vertices",
                 __func__,
                 __LINE__,
                 v);
      return;
    }

    if (edges.find(v) == edges.end()) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OT_NONE,
                 "{}.{}: Invalid vertex {} in edges",
                 __func__,
                 __LINE__,
                 v);
      return;
    }
    // Recur for all the vertices adjacent to this vertex
    for (const auto edge : edges.at(v)) {
      if (edge.data && !vertices[edge]) topologicalSort(edge);
    }

    // Push current vertex to stack which stores result
    ordered_create_list.push_back(v);
  }

  // The function to do Topological Sort. It uses recursive topologicalSort()
  void goSort() {
    ordered_create_list.clear();
    // Call the recursive helper function to store Topological
    // Sort starting from all vertices one by one
    for (auto it = edges.begin(); it != edges.end(); it++) {
      if (vertices[it->first] == false) topologicalSort(it->first);
    }
  }
};

/*
 * Write one line per object in the format
 * <object_id>,<attr_1_id>#<value_1>,<attr_2_id>#<value_2>
 *
 * "object_id" is always the first field.
 * "," is used as the delimiter between each field. The reason for using "," is
 * so this can be exported as a CSV file.
 * "#" is used as the delimiter between the "attr_id" and the corresponding
 * "value".
 */
void write_one_object(std::ofstream &out,
                      switch_object_id_t object,
                      ModelInfo *model_info) {
  if (object_attr_hash->find(object) == object_attr_hash->end()) return;

  switch_object_type_t object_type = switch_store::object_type_query(object);
  const ObjectInfo *object_info = model_info->get_object_info(object_type);
  if (object_info == NULL) {
    switch_log(
        SWITCH_API_LEVEL_ERROR,
        SWITCH_OT_NONE,
        "{}.{}: Invalid object: {} type: {}. Skipping this object for db save",
        __func__,
        __LINE__,
        object,
        object_type);
    return;
  }
  std::string object_name = object_info->get_object_name_fqn();

  (*object_attr_hash)[object].first++;
  std::stringstream this_object_out;
  this_object_out << object_name << ":";
  this_object_out << object;

  for (auto &ita : (*object_attr_hash)[object].second) {
    if (ita.attr_id == SPECIAL_OBJECT_STATUS_ATTR_ID) continue;
    value_key_t key = {.attr_id = ita.attr_id, .extra = ita.extra};
    const AttributeMetadata *attr_md =
        object_info->get_attr_metadata(key.attr_id);
    if (attr_md == NULL) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OT_NONE,
                 "{}.{}: Failed to query attribute metadata for object:{} "
                 "type:{} attr:{}. Skipping this attribute for db save",
                 __func__,
                 __LINE__,
                 object,
                 object_type,
                 key.attr_id);
      continue;
    }
    std::string attr_name = attr_md->get_attr_name_fqn();
    this_object_out << "," << std::dec << attr_name << "#" << key.extra << "#";
    switch_attribute_value_t m_value = ita.get_value();
    // This is a list attribute and holds the total list count
    if (m_value.type == SWITCH_TYPE_LIST) {
      if (key.extra == 0) {
        this_object_out << m_value.list.count;
      } else {
        if (m_value.list.list) {
          for (uint32_t i = 0; i < m_value.list.count; i++) {
            this_object_out << m_value.list.list[i];
          }
        }
      }
    } else if (m_value.type == SWITCH_TYPE_IP_ADDRESS) {
      this_object_out << static_cast<uint16_t>(m_value.ipaddr.addr_family)
                      << "-";
      this_object_out << ita.get_value();
    } else if (m_value.type == SWITCH_TYPE_IP_PREFIX) {
      this_object_out << static_cast<uint16_t>(
                             m_value.ipprefix.addr.addr_family)
                      << "-";
      this_object_out << ita.get_value();
    } else if (m_value.type == SWITCH_TYPE_OBJECT_ID) {
      switch_object_type_t attr_object_type =
          switch_store::object_type_query(m_value.oid);
      const ObjectInfo *attr_object_info =
          model_info->get_object_info(attr_object_type);
      if (attr_object_info == NULL) {
        switch_log(SWITCH_API_LEVEL_DEBUG,
                   SWITCH_OT_NONE,
                   "{}.{}: Derived Object type as None for object attr: {} "
                   "type: {}, for object {} attr {}",
                   __func__,
                   __LINE__,
                   m_value.oid,
                   attr_object_type,
                   object,
                   key.attr_id);
        this_object_out << "SWITCH_OBJECT_TYPE_NONE:";
      } else {
        std::string attr_object_name = attr_object_info->get_object_name_fqn();
        this_object_out << attr_object_name << ":";
      }
      this_object_out << ita.get_value();
    } else if (m_value.type == SWITCH_TYPE_ENUM) {
      auto value_md = attr_md->get_value_metadata();
      auto enums_md = value_md->get_enum_metadata();
      for (const auto &enum_md : enums_md) {
        if (enum_md.enum_value == m_value.enumdata.enumdata) {
          this_object_out << enum_md.enum_name_fqn;
        }
      }
    } else {
      this_object_out << ita.get_value();
    }
  }
  out << this_object_out.str() << std::endl;
}

/*
 * Write one line per object, per attr_id in the format
 * <object_id>,<attr_1_id>#<value_1>
 */
void write_one_object_one_attr(std::ofstream &out,
                               switch_object_id_t object,
                               const switch_attr_id_t attr_id,
                               const uint16_t extra,
                               const switch_attribute_value_t &value_in,
                               ModelInfo *model_info) {
  switch_object_type_t object_type = switch_store::object_type_query(object);
  const ObjectInfo *object_info = model_info->get_object_info(object_type);
  if (object_info == NULL) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OT_NONE,
               "{}.{}: Invalid object: {} type: {}",
               __func__,
               __LINE__,
               object,
               object_type);
    return;
  }
  std::string object_name = object_info->get_object_name_fqn();

  (*object_attr_hash)[object].first++;
  std::stringstream this_object_out;
  this_object_out << object_name << ":";
  this_object_out << object;

  value_key_t key = {.attr_id = attr_id, .extra = extra};
  const AttributeMetadata *attr_md =
      object_info->get_attr_metadata(key.attr_id);
  if (attr_md == NULL) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OT_NONE,
               "{}.{}: Failed to query attribute metadata for object:{} "
               "type:{} attr:{}",
               __func__,
               __LINE__,
               object,
               object_type,
               key.attr_id);
    return;
  }

  std::string attr_name = attr_md->get_attr_name_fqn();
  this_object_out << "," << std::dec << attr_name << "#" << key.extra << "#";
  switch_attribute_value_t m_value = value_in;
  // This is a list attribute and holds the total list count
  if (m_value.type == SWITCH_TYPE_LIST) {
    if (key.extra == 0) {
      this_object_out << m_value.list.count;
    } else {
      if (m_value.list.list) {
        for (uint32_t i = 0; i < m_value.list.count; i++) {
          this_object_out << m_value.list.list[i];
        }
      }
    }
  } else if (m_value.type == SWITCH_TYPE_IP_ADDRESS) {
    this_object_out << static_cast<uint16_t>(m_value.ipaddr.addr_family) << "-";
    this_object_out << m_value;
  } else if (m_value.type == SWITCH_TYPE_IP_PREFIX) {
    this_object_out << static_cast<uint16_t>(m_value.ipprefix.addr.addr_family)
                    << "-";
    this_object_out << m_value;
  } else if (m_value.type == SWITCH_TYPE_OBJECT_ID) {
    switch_object_type_t attr_object_type =
        switch_store::object_type_query(m_value.oid);
    const ObjectInfo *attr_object_info =
        model_info->get_object_info(attr_object_type);
    if (attr_object_info == NULL) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OT_NONE,
                 "{}.{}: Derived Object type as None for object attr: {} "
                 "type: {}, for object {} attr {}",
                 __func__,
                 __LINE__,
                 m_value.oid,
                 attr_object_type,
                 object,
                 key.attr_id);
      this_object_out << "SWITCH_OBJECT_TYPE_NONE:";
    } else {
      std::string attr_object_name = attr_object_info->get_object_name_fqn();
      this_object_out << attr_object_name << ":";
    }
    this_object_out << m_value;
  } else if (m_value.type == SWITCH_TYPE_ENUM) {
    auto value_md = attr_md->get_value_metadata();
    auto enums_md = value_md->get_enum_metadata();
    for (const auto &enum_md : enums_md) {
      if (enum_md.enum_value == m_value.enumdata.enumdata) {
        this_object_out << enum_md.enum_name_fqn;
      }
    }
  } else {
    this_object_out << m_value;
  }
  out << this_object_out.str() << std::endl;
}

#if 0
/*
 * debug
 * currently not in use
 */
switch_status_t db_print(bool stats_cache_only) {
  std::lock_guard<std::recursive_mutex> guard(db_mtx);
  uint64_t num_entries = 0;
  uint64_t num_stats_cache = 0;
  uint64_t num_counters = 0;
  ModelInfo *model = NULL;
  model = switch_store::switch_model_info_get();
  if (model == nullptr) {
    return SWITCH_STATUS_SUCCESS;
  }
  switch_log(SWITCH_API_LEVEL_WARN,
        SWITCH_OT_NONE,
             "Store print start {}",
             stats_cache_only ? "- objects with stats-cache" : "");
  for (auto it = object_attr_hash->begin(); it != object_attr_hash->end();
       it++) {
    switch_object_id_t object = it->first;
    switch_object_type_t object_type = switch_store::object_type_query(object);
    const ObjectInfo *object_info = model->get_object_info(object_type);
    if (object_info->get_stats_cache()) {
      num_stats_cache++;
    }
    if (object_info->get_counter()) {
      num_counters++;
    }
    num_entries++;
    (*object_attr_hash)[object].first++;
    std::stringstream this_object_out;
    this_object_out << object;
    for (auto ita = (*object_attr_hash)[object].second.begin();
         ita != (*object_attr_hash)[object].second.end();
         ita++) {
      value_key_t key = ita->first;
      this_object_out << "," << std::dec << key.attr_id << "#" << key.extra
                      << "#";
      switch_attribute_value_t m_value = ita->second->get_value();
      // This is a list attribute and holds the total list count
      if ((m_value.type == SWITCH_TYPE_LIST) && (key.extra == 0)) {
        this_object_out << m_value.list.count;
      } else if (m_value.type == SWITCH_TYPE_IP_ADDRESS) {
        this_object_out << static_cast<uint16_t>(m_value.ipaddr.addr_family)
                        << "-";
        this_object_out << ita->second->get_value();
      } else if (m_value.type == SWITCH_TYPE_IP_PREFIX) {
        this_object_out << static_cast<uint16_t>(
                               m_value.ipprefix.addr.addr_family) << "-";
        this_object_out << ita->second->get_value();
      } else {
        this_object_out << ita->second->get_value();
      }
    }
    if (stats_cache_only == false || object_info->get_stats_cache()) {
      switch_log(SWITCH_API_LEVEL_INFO,
        SWITCH_OT_NONE,
                 "{}", this_object_out.str());
    }
  }
  switch_log(SWITCH_API_LEVEL_WARN,
        SWITCH_OT_NONE,
             "Store print complete: Total objects {}, "
             "Total objects with counters {}, "
             "Total objects with stats cache {}",
             num_entries,
             num_counters,
             num_stats_cache);
  return SWITCH_STATUS_SUCCESS;
}
#endif

/*
 * Read each object from DB and store to file
 */
switch_status_t db_clear() {
  std::lock_guard<std::recursive_mutex> guard(db_mtx);
  object_attr_hash->clear();
  return SWITCH_STATUS_SUCCESS;
}

/*
 * Read each object from DB and store to file
 */
switch_status_t db_dump(const char *const dump_file) {
  std::lock_guard<std::recursive_mutex> guard(db_mtx);
  uint64_t num_records = 0;

  ModelInfo *model_info = NULL;
  model_info = switch_store::switch_model_info_get();
  if (model_info == nullptr) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OT_NONE,
               "db_write: Warm reboot failed, Failed to get model info");
    return SWITCH_STATUS_FAILURE;
  }
  std::string out_file = dump_file;
  std::ofstream out;
  out.open(out_file.c_str(), std::ofstream::trunc);
  if (out) {
    for (auto it = object_attr_hash->begin(); it != object_attr_hash->end();
         it++) {
      write_one_object(out, it->first, model_info);
      num_records++;
    }
  }
  out.close();
  switch_log(SWITCH_API_LEVEL_WARN,
             SWITCH_OT_NONE,
             "db_write: Data flushed to persistent storage at {}",
             out_file.c_str());
  switch_log(SWITCH_API_LEVEL_WARN,
             SWITCH_OT_NONE,
             "db_write: Total records written to persistent storage {}",
             num_records);

  return SWITCH_STATUS_SUCCESS;
}

std::vector<std::string> split(const std::string &s, char delimiter) {
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(s);
  while (std::getline(tokenStream, token, delimiter)) {
    tokens.push_back(token);
  }
  return tokens;
}

switch_object_id_t get_new_oid(switch_object_id_t old_oid,
                               switch_object_type_t object_type) {
  uint64_t id = switch_store::handle_to_id(old_oid);
  return switch_store::id_to_handle(object_type, id);
}

void set_attribute_value(switch_attribute_value_t &value_in,
                         switch_attr_type_t type,
                         std::string value,
                         const AttributeMetadata *attr_md) {
  switch (type) {
    case SWITCH_TYPE_BOOL:
      std::istringstream(value) >> std::boolalpha >> value_in.booldata >>
          std::noboolalpha;
      break;
    case SWITCH_TYPE_UINT8:
      value_in.u8 = static_cast<uint8_t>(std::stoul(value.c_str(), nullptr, 0));
      break;
    case SWITCH_TYPE_UINT16:
      value_in.u16 =
          static_cast<uint16_t>(std::stoul(value.c_str(), nullptr, 0));
      break;
    case SWITCH_TYPE_UINT32:
      value_in.u32 =
          static_cast<uint32_t>(std::stoul(value.c_str(), nullptr, 0));
      break;
    case SWITCH_TYPE_UINT64:
      value_in.u64 = strtoul(value.c_str(), NULL, 0);
      break;
    case SWITCH_TYPE_INT64:
      value_in.s64 = strtol(value.c_str(), NULL, 0);
      break;
    case SWITCH_TYPE_ENUM: {
      auto value_md = attr_md->get_value_metadata();
      auto enums_md = value_md->get_enum_metadata();
      value_in.enumdata.enumdata = 0;
      for (const auto &enum_md : enums_md) {
        if (enum_md.enum_name_fqn.compare(value) == 0) {
          value_in.enumdata.enumdata = enum_md.enum_value;
          break;
        }
      }
    } break;
    case SWITCH_TYPE_MAC:
      attr_util::parse_mac(value, value_in.mac);
      break;
    case SWITCH_TYPE_STRING:
      snprintf(value_in.text.text,
               SWITCH_MAX_STRING_LEN,
               "%s",
               value.substr(1, value.length() - 2).c_str());
      break;
    case SWITCH_TYPE_RANGE: {
      std::vector<std::string> tokens = split(value, '-');
      value_in.range.min =
          static_cast<uint32_t>(std::stoul(tokens[0].c_str(), nullptr, 0));
      value_in.range.max =
          static_cast<uint32_t>(std::stoul(tokens[1].c_str(), nullptr, 0));
    } break;
    case SWITCH_TYPE_IP_ADDRESS: {
      std::vector<std::string> tokens = split(value, '-');
      attr_util::parse_ip_address(tokens[1], value_in.ipaddr);
      value_in.ipaddr.addr_family = static_cast<switch_ip_addr_family_t>(
          std::stoul(tokens[0].c_str(), nullptr, 10));
    } break;
    case SWITCH_TYPE_IP_PREFIX: {
      std::vector<std::string> tokens = split(value, '-');
      attr_util::parse_ip_prefix(tokens[1], value_in.ipprefix);
      value_in.ipprefix.addr.addr_family = static_cast<switch_ip_addr_family_t>(
          std::stoul(tokens[0].c_str(), nullptr, 10));
    } break;
    case SWITCH_TYPE_OBJECT_ID: {
      std::vector<std::string> obj_string = split(value, ':');
      if (obj_string.size() == 2) {
        ModelInfo *model_info = NULL;
        model_info = switch_store::switch_model_info_get();
        if (model_info != nullptr) {
          switch_object_type_t object_type = static_cast<switch_object_type_t>(
              model_info->get_id_from_name(obj_string[0]));
          switch_object_id_t old_oid = {
              .data = strtoul(obj_string[1].c_str(), NULL, 0)};
          switch_object_id_t oid = get_new_oid(old_oid, object_type);
          value_in.oid.data = oid.data;
        }
      } else {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OT_NONE,
                   "{}.{}: Invalid format for attr {}, should be of format "
                   "<object_type>:<oid>",
                   __func__,
                   __LINE__,
                   value);
        assert(0);
        return;
      }
    } break;
    case SWITCH_TYPE_NONE:
    case SWITCH_TYPE_MAX:
    /* fall through */
    default:
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OT_NONE,
                 "{}.{}: Unsupported attr type: {}",
                 __func__,
                 __LINE__,
                 type);
      assert(0);
      break;
  }
}

void set_list_value(switch_attribute_value_t &value_in,
                    switch_attr_type_t list_type,
                    std::string value,
                    uint64_t extra,
                    const AttributeMetadata *attr_md) {
  switch (value_in.type) {
    case SWITCH_TYPE_LIST:
      if (extra == 0) {
        value_in.type = SWITCH_TYPE_LIST;
        value_in.list.count = strtoul(value.c_str(), NULL, 0);
        value_in.list.list_type = list_type;
      } else {
        value_in.type = list_type;
        set_attribute_value(value_in, list_type, value, attr_md);
      }
      break;
    default:
      break;
  }
}

/*
 * Read each object from file and store to DB
 * Parse the object_id first and reserve it in oid_store
 * Then start adding each attribute to store
 */
switch_status_t db_load(bool warm_init, const char *const warm_init_file) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  ModelInfo *model_info = NULL;
  uint64_t num_records = 0;
  topoSort topo;
  switch_object_id_t empty = {};

  object_attr_hash = new db_store();

  // nothing to load if not warm_init_mode
  if (!warm_init) return status;
  switch_log(
      SWITCH_API_LEVEL_WARN, SWITCH_OT_NONE, "db_read: Warm boot initiated");

  model_info = switch_store::switch_model_info_get();
  if (model_info == nullptr) {
    return SWITCH_STATUS_FAILURE;
  }

  std::string in_file = warm_init_file;
  switch_log(SWITCH_API_LEVEL_WARN,
             SWITCH_OT_NONE,
             "db_read: Data reading from persistent storage at {}",
             in_file.c_str());
  std::ifstream infile;
  infile.open(in_file.c_str());
  std::string line;
  while (std::getline(infile, line)) {
    // Line is
    // <object_type>:<object_id>,<attr_1_id>#<extra>#<value_1>,<attr_2_id>#<extra<#<value_2>
    // First get the object_id
    std::vector<std::string> tokens = split(line, ',');
    std::vector<std::string> obj_string = split(tokens[0], ':');
    if (obj_string.size() != 2) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OT_NONE,
                 "{}.{}: Invalid format for <object_type>:<oid> on {}:{}",
                 __func__,
                 __LINE__,
                 in_file.c_str(),
                 line);
      continue;
    }
    switch_object_type_t object_type = static_cast<switch_object_type_t>(
        model_info->get_id_from_name(obj_string[0]));
    switch_object_id_t old_oid = {.data =
                                      strtoul(obj_string[1].c_str(), NULL, 0)};
    // The numerical value for switch_object_type_t can change across warm
    // reboot. Since object type is encoded within
    // switch_object_id_t, we derive the new oid for the pre warm reboot object
    // ids using the new numerical value for
    // switch_object_type_t. If there is no change in switch_object_type_t
    // across reboot, then the new oid is the same
    // as old oid.
    switch_object_id_t object = get_new_oid(old_oid, object_type);
    const ObjectInfo *object_info = model_info->get_object_info(object_type);
    if (object_info == NULL) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OT_NONE,
                 "{}.{}: Invalid object: {} type: {}",
                 __func__,
                 __LINE__,
                 object,
                 object_type);
      continue;
    }
    std::string object_name = object_info->get_object_name();
    bool is_device = object_name.compare("device") == 0;
    topo.addEdge(object, empty);

    // Create in DB
    attribute_map *db_ptr = nullptr;
    db_ptr = object_create(object, object_info);
    if (db_ptr == nullptr) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 object_type,
                 "{}.{}: Duplicate object: {} type: {}",
                 __func__,
                 __LINE__,
                 object,
                 object_type);
      continue;
    }

    // Reserve the OID
    status = switch_store::oid_create(object_type, object, true);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 object_type,
                 "{}.{}: OID reserve fail for object: {} type: {}",
                 __func__,
                 __LINE__,
                 object,
                 object_type);
      continue;
    }

    // Get all attr_id and values and store them
    for (size_t i = 1; i < tokens.size(); i++) {
      std::vector<std::string> attr_val_pair = split(tokens[i].c_str(), '#');
      switch_attr_id_t attr_id = static_cast<switch_attr_id_t>(
          model_info->get_id_from_name(attr_val_pair[0]));
      const AttributeMetadata *attr_md =
          object_info->get_attr_metadata(attr_id);
      if (attr_md == nullptr) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   object_type,
                   "{}.{}: Invalid attribute ID: {} object: {} type: {}",
                   __func__,
                   __LINE__,
                   attr_id,
                   object,
                   object_type);
        continue;
      }
      switch_attribute_value_t value_in = {};
      uint64_t extra = strtoul(attr_val_pair[1].c_str(), NULL, 0);
      value_in.type = attr_md->type;
      switch_attr_flags_t flags = attr_md->get_flags();
      if (flags.is_status) attr_val_pair[2] = "0";
      // if this is bf_rt_status, reset to 0
      if (attr_md->type == SWITCH_TYPE_LIST) {
        const ValueMetadata *value_md = attr_md->get_value_metadata();
        set_list_value(
            value_in, value_md->type, attr_val_pair[2], extra, attr_md);
      } else {
        set_attribute_value(value_in, value_in.type, attr_val_pair[2], attr_md);
        // create the topo graph ordered object creation
        // the device object is ignored so there will be no cyclic dependencies
        if (!is_device) {
          if (value_in.type == SWITCH_TYPE_OBJECT_ID && !flags.is_internal &&
              !flags.is_read_only) {
            topo.addEdge(object, value_in.oid);
          }
        }
      }
      status = value_set(object, attr_id, extra, value_in);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   object_type,
                   "{}.{}: value_set fail for attr: {} object: {} "
                   "type: {}",
                   __func__,
                   __LINE__,
                   value_in,
                   object,
                   object_type);
        continue;
      }
    }
    num_records++;
  }
  infile.close();

  // now create the full ordered insertion list
  topo.goSort();

  switch_log(SWITCH_API_LEVEL_WARN,
             SWITCH_OT_NONE,
             "db_read: Total records read from persistent storage {}",
             num_records);

  return status;
}

bool object_exists(const switch_object_id_t object_id) {
  return (object_attr_hash->find(object_id) != object_attr_hash->end());
}

switch_status_t object_delete(const switch_object_id_t object_id) {
  CHECK_RET(!object_exists(object_id), SWITCH_STATUS_FAILURE);

  // SWITCH_LOG_DEBUG("store: removing object %lx", object_id.data);

  std::lock_guard<std::recursive_mutex> guard(db_mtx);
  size_t erased = object_attr_hash->erase(object_id);
  CHECK_RET(erased != 1, SWITCH_STATUS_FAILURE);
  return SWITCH_STATUS_SUCCESS;
}

attribute_map *object_create(const switch_object_id_t object_id,
                             const ObjectInfo *object_info) {
  std::lock_guard<std::recursive_mutex> guard(db_mtx);

  // this creates an entry and returns the reference
  attribute_map *attr_map = &(*object_attr_hash)[object_id];

  // reserve attr_count number of entries and assign to map
  // 1 extra for lock special attr
  const auto &attr_md_list = object_info->get_attribute_list();
  attribute_wrapper object_attrs;
  object_attrs.reserve(attr_md_list.size() + 1);

  // push special lock attr
  switch_attribute_value_t special_value_in = {};
  object_attrs.emplace_back(SPECIAL_OBJECT_STATUS_ATTR_ID,
                            static_cast<uint64_t>(0),
                            special_value_in);

  // push the actual object attrs
  for (const auto &attr_md : attr_md_list) {
    switch_attribute_value_t value_in = {};
    object_attrs.emplace_back(
        attr_md.attr_id, static_cast<uint64_t>(0), value_in);
  }
  attr_map->second = std::move(object_attrs);

  return attr_map;
}

switch_status_t object_create_with_attrs(
    const switch_object_id_t object_id,
    std::vector<value_wrapper> &object_attrs) {
  std::lock_guard<std::recursive_mutex> guard(db_mtx);

  // this creates an entry and returns the reference
  attribute_map *attr_map = &(*object_attr_hash)[object_id];

  switch_attribute_value_t value_in = {};
  object_attrs.emplace_back(
      SPECIAL_OBJECT_STATUS_ATTR_ID, static_cast<uint64_t>(0), value_in);

  attr_map->second = std::move(object_attrs);

  return SWITCH_STATUS_SUCCESS;
}

inline attribute_wrapper::iterator find_value_wrapper(
    attribute_map *attr_map,
    const switch_attr_id_t attr_id,
    const uint16_t extra) {
  return std::find_if(attr_map->second.begin(),
                      attr_map->second.end(),
                      [&](value_wrapper const &object) {
                        return (object.attr_id == attr_id) &&
                               (object.extra == extra);
                      });
}
switch_status_t value_create(attribute_map *attr_map,
                             const switch_attr_id_t attr_id,
                             const uint64_t extra,
                             const switch_attribute_value_t &value_in) {
  std::lock_guard<std::recursive_mutex> guard(db_mtx);
  auto ret = find_value_wrapper(attr_map, attr_id, extra);
  if (ret != attr_map->second.end())
    ret->set_value(value_in);
  else
    return SWITCH_STATUS_FAILURE;

  return SWITCH_STATUS_SUCCESS;
}
switch_status_t value_delete(const switch_object_id_t object_id,
                             const switch_attr_id_t attr_id,
                             const uint64_t extra) {
  std::lock_guard<std::recursive_mutex> guard(db_mtx);
  auto it = object_attr_hash->find(object_id);

  if (it == object_attr_hash->end()) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               switch_store::object_type_query(object_id),
               SMI_DELETE_OPERATION,
               "store.value_delete: object key {:#x} not found",
               object_id.data);
    return SWITCH_STATUS_ITEM_NOT_FOUND;
  }

  auto ret = find_value_wrapper(&it->second, attr_id, extra);
  if (ret != it->second.second.end()) it->second.second.erase(ret);

  return SWITCH_STATUS_SUCCESS;
}
switch_status_t value_set(const switch_object_id_t object_id,
                          const switch_attr_id_t attr_id,
                          const uint64_t extra,
                          const switch_attribute_value_t &value_in) {
  std::lock_guard<std::recursive_mutex> guard(db_mtx);
  auto it = object_attr_hash->find(object_id);

  if (it == object_attr_hash->end()) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               switch_store::object_type_query(object_id),
               SMI_DELETE_OPERATION,
               "store.value_delete: object key {:#x} not found",
               object_id.data);
    return SWITCH_STATUS_ITEM_NOT_FOUND;
  }

  auto ret = find_value_wrapper(&it->second, attr_id, extra);
  if (ret != it->second.second.end())
    ret->set_value(value_in);
  else
    it->second.second.push_back(value_wrapper(attr_id, extra, value_in));
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t value_get(const switch_object_id_t object_id,
                          const switch_attr_id_t attr_id,
                          const uint64_t extra,
                          switch_attribute_value_t &value_out) {
  std::lock_guard<std::recursive_mutex> guard(db_mtx);
  auto it = object_attr_hash->find(object_id);

  if (it == object_attr_hash->end()) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               switch_store::object_type_query(object_id),
               SMI_GET_OPERATION,
               "store.value_get: object key {:#x} not found",
               object_id.data);
    return SWITCH_STATUS_ITEM_NOT_FOUND;
  }

  auto ret = find_value_wrapper(&it->second, attr_id, extra);
  if (ret != it->second.second.end()) {
    const switch_attribute_value_t &value = ret->get_value();
    value_out = value;
    return SWITCH_STATUS_SUCCESS;
  } else {
    return SWITCH_STATUS_ITEM_NOT_FOUND;
  }
}

switch_status_t value_get_all(
    const switch_object_id_t object_id,
    std::vector<std::reference_wrapper<const switch_attribute_t>> &value_out) {
  std::lock_guard<std::recursive_mutex> guard(db_mtx);
  auto it = object_attr_hash->find(object_id);

  if (it == object_attr_hash->end()) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               switch_store::object_type_query(object_id),
               SMI_GET_OPERATION,
               "store.value_get: object key {:#x} not found",
               object_id.data);
    return SWITCH_STATUS_ITEM_NOT_FOUND;
  }

  std::for_each(it->second.second.begin(),
                it->second.second.end(),
                [&value_out](value_wrapper &n) {
                  if (n.attr_id != SPECIAL_OBJECT_STATUS_ATTR_ID)
                    value_out.push_back(n.get());
                });
  return SWITCH_STATUS_SUCCESS;
}

void switch_store_lock(void) { db_mtx.lock(); }

void switch_store_unlock(void) { db_mtx.unlock(); }

int object_lock(const switch_object_id_t object_id) {
  pthread_t tid = pthread_self();
  bool locked = false;
  while (!locked) {
    db_mtx.lock();
    auto it = object_attr_hash->find(object_id);
    if (it != object_attr_hash->end()) {
      auto ret = find_value_wrapper(
          &it->second,
          static_cast<switch_attr_id_t>(SPECIAL_OBJECT_STATUS_ATTR_ID),
          static_cast<uint64_t>(0));
      if (ret != it->second.second.end()) {
        switch_attribute_value_t &value = ret->get_value_mutable();
        if (value.u64 != 0 && !pthread_equal(tid, ret->lock_tid)) {
          db_mtx.unlock();
          usleep(100);
          continue;
        } else {
          ret->lock_tid = tid;
          ++value.u64;
          locked = true;
        }
      }
    } else {
      db_mtx.unlock();
      return 0;
    }
    db_mtx.unlock();
  }
  return -1;
}

void object_unlock(const switch_object_id_t object_id) {
  std::lock_guard<std::recursive_mutex> guard(db_mtx);
  auto it = object_attr_hash->find(object_id);
  if (it != object_attr_hash->end()) {
    auto ret = find_value_wrapper(
        &it->second,
        static_cast<switch_attr_id_t>(SPECIAL_OBJECT_STATUS_ATTR_ID),
        static_cast<uint64_t>(0));
    if (ret != it->second.second.end()) {
      pthread_t tid = pthread_self();
      switch_attribute_value_t &value = ret->get_value_mutable();
      if (value.u64 != 0 && pthread_equal(tid, ret->lock_tid)) {
        --value.u64;
      }
    }
  }
}

} /* namespace db */
} /* namespace smi */
