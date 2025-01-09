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


#ifndef S3_STORE_H__
#define S3_STORE_H__

#include <mutex>  // NOLINT(build/c++11)
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <set>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <algorithm>
#include <functional>

#include "bf_switch/bf_switch_types.h"
#include "s3/attribute_util.h"
#include "s3/meta/meta.h"
#include "./log.h"

namespace smi {
using ::smi::attr_util::object_and_attribute_t;

struct secondaryIndexKeyHasher {
  std::size_t operator()(const std::vector<attr_w> &keys) const {
    return hash_range(keys.begin(), keys.end());
  }
};

/** @brief Wrapper class to maintain a secondary index of the primary
 * db_store
 *  A simple map is used to maintain a string to object_id mapping. The key
 * is
 *  the set of key group attributes defined in the schema per object
 *  key format:
 *  object_type::serialized(attr1)::serialized(attr2):: ...
 *  attr order in order of attr id
 */
struct compare {
  bool operator()(const std::reference_wrapper<const attr_w> &lhs,
                  const std::reference_wrapper<const attr_w> &rhs) const {
    return (lhs.get() < rhs.get());
  }
};
class secondaryIndex {
 private:
  typedef std::unordered_map<std::vector<attr_w>,
                             switch_object_id_t,
                             secondaryIndexKeyHasher>
      secIndex;
  std::vector<secIndex> si;
  std::mutex mtx;
  typedef std::lock_guard<std::mutex> LOCK_GUARD;

 public:
  secondaryIndex(size_t object_count) {
    si = std::vector<secIndex>(object_count);
    for (auto &secondary_index : si) {
      secondary_index.reserve(512);
    }
  }

  void clear() {
    LOCK_GUARD guard(mtx);
    si.clear();
    return;
  }

  inline std::pair<secIndex::iterator, bool> insert(
      const std::vector<attr_w> &key, switch_object_id_t oid) {
    LOCK_GUARD guard(mtx);
    return si[oid.data >> OBJECT_ID_WIDTH].emplace(
        std::piecewise_construct, std::make_tuple(key), std::make_tuple(oid));
  }

  inline size_t erase(switch_object_type_t ot, const std::vector<attr_w> &key) {
    LOCK_GUARD guard(mtx);
    return si[ot].erase(key);
  }

  inline secIndex::iterator find(switch_object_type_t ot,
                                 const std::vector<attr_w> &key) {
    LOCK_GUARD guard(mtx);
    return si[ot].find(key);
  }

  inline secIndex::iterator end(switch_object_type_t ot) {
    LOCK_GUARD guard(mtx);
    return si[ot].end();
  }
};

/** @brief Wrapper class to maintain a per object references to other objects
 *  referencing this object
 *  Structure:
 *    this_object ->
 *                -> ref_object_type1
 *                                   -> (ref_object, <attr_1,.. attr_n>)
 *                                   -> (ref_object, <attr_1,.. attr_n>)
 *                                   -> (ref_object, <attr_1,.. attr_n>)
 *                -> ref_object_type2
 *                                   -> (ref_object, <attr_1,.. attr_n>)
 *                                   -> (ref_object, <attr_1,.. attr_n>)
 *                                   -> (ref_object, <attr_1,.. attr_n>)
 */
static const std::vector<object_and_attribute_t> empty_object_refs;
class objectGraph {
 private:
  /* One object may refer to another one with several attributes (i.e. port
   * refers to the same mirror session with ingress_mirror_handle and
   * egress_mirror_handle attributes so we have to maintain attributes list as
   * well */
  typedef std::map<switch_object_type_t, std::vector<object_and_attribute_t>>
      objRefs;
  typedef std::unordered_map<uint64_t, objRefs> graph;
  std::vector<graph> refs;
  std::mutex mtx;
  typedef std::lock_guard<std::mutex> LOCK_GUARD;

 public:
  objectGraph(size_t object_count) { refs = std::vector<graph>(object_count); }

  const std::vector<object_and_attribute_t> &getRefs(
      const switch_object_id_t &dst, const switch_object_type_t src_type) {
    LOCK_GUARD guard(mtx);
    graph &ref = refs[dst.data >> OBJECT_ID_WIDTH];
    auto it = ref.find(dst.data);
    if (it != ref.end()) {
      auto it2 = it->second.find(src_type);
      if (it2 != it->second.end()) {
        return it2->second;
      }
    }
    return empty_object_refs;
  }

  void clear() {
    LOCK_GUARD guard(mtx);
    refs.clear();
    return;
  }

  bool insert(const switch_object_id_t &dst,
              switch_object_type_t src_type,
              const switch_object_id_t &src,
              switch_attr_id_t src_attr_id) {
    LOCK_GUARD guard(mtx);

    refs[dst.data >> OBJECT_ID_WIDTH][dst.data][src_type].emplace_back(
        src, src_attr_id);
    return true;
  }

  /* Removes the whole ref object */
  size_t erase(const switch_object_id_t &dst,
               const switch_object_type_t src_type,
               const switch_object_id_t &src) {
    LOCK_GUARD guard(mtx);

    auto &ref = refs[dst.data >> OBJECT_ID_WIDTH][dst.data][src_type];
    size_t num = 1;

    ref.erase(std::remove_if(ref.begin(),
                             ref.end(),
                             [&](object_and_attribute_t const &object) {
                               return (object.oid == src);
                             }),
              ref.end());
    return num;
  }

  /* Removes the attribute ID from the ref obj list and in case it becomes empty
   * erases
   * the whole ref object */
  size_t erase(const switch_object_id_t &dst,
               const switch_object_type_t src_type,
               const switch_object_id_t &src,
               const switch_attr_id_t src_attr_id) {
    LOCK_GUARD guard(mtx);

    auto &ref = refs[dst.data >> OBJECT_ID_WIDTH][dst.data][src_type];
    size_t num = 1;

    ref.erase(std::remove_if(ref.begin(),
                             ref.end(),
                             [&](object_and_attribute_t const &object) {
                               return (object.oid == src &&
                                       object.attr_id == src_attr_id);
                             }),
              ref.end());
    return num;
  }

  const graph::iterator find(const switch_object_id_t &oid) {
    LOCK_GUARD guard(mtx);
    return refs[oid.data >> OBJECT_ID_WIDTH].find(oid.data);
  }
  const graph::iterator end(const switch_object_id_t &oid) {
    LOCK_GUARD guard(mtx);
    return refs[oid.data >> OBJECT_ID_WIDTH].end();
  }
};

namespace db {

typedef struct _value_key_t {
  switch_attr_id_t attr_id;
  uint16_t extra;

  bool operator==(const _value_key_t &other) const {
    return (attr_id == other.attr_id && extra == other.extra);
  }
} value_key_t;

class value_wrapper {
 public:
  /* locking thread; the value is only valid if lock counter > 0 */
  pthread_t lock_tid{};
  switch_attr_id_t attr_id = 0;
  uint16_t extra = 0;
  /*  only stores base-type values, doesn't own any extra memory */
  value_wrapper(const switch_attr_id_t &_attr_id,
                uint16_t _extra,
                const switch_attribute_value_t &value) {
    attr_id = _attr_id;
    extra = _extra;
    m_value.id = _attr_id;
    m_value.value = value;
  }
  ~value_wrapper() {}

  inline const switch_attribute_t &get() const { return m_value; }
  inline void set_value(const switch_attribute_value_t &value) {
    m_value.value = value;
  }
  inline const switch_attribute_value_t &get_value() const {
    return m_value.value;
  }
  inline switch_attribute_value_t &get_value_mutable() { return m_value.value; }
  inline bool operator()(const value_wrapper &other) const {
    return (attr_id == other.attr_id && extra == other.extra);
  }

 private:
  switch_attribute_t m_value;
};

struct KeyHasher {
  std::size_t operator()(const value_key_t &k) const {
    using std::size_t;
    using std::hash;

    return ((hash<uint16_t>()(k.attr_id) ^ (hash<uint16_t>()(k.extra) << 1)) >>
            1);
  }
};

// Use this to maintain object status, created, deleted, deleting, etc
#define SPECIAL_OBJECT_STATUS_ATTR_ID 0xFFFF

/*
 *  Value keys would look something like: ObjectID.AttributeID.Extra
 *  Where extra would be used for list indices, key hashes, etc.
 *  object delete is essentially delete of range of values,
 *  if we were to use a single layer map.
 *
 */
typedef std::vector<value_wrapper> attribute_wrapper;
typedef std::pair<uint64_t, attribute_wrapper> attribute_map;
typedef std::unordered_map<switch_object_id_t, attribute_map> db_store;

const db_store *get_db();
switch_status_t db_load(bool warm_init, const char *const warm_init_file);
switch_status_t db_clear();
#if 0
switch_status_t db_print(bool stats_cache_only);
#endif
switch_status_t db_dump(const char *const dump_file);
const std::vector<switch_object_id_t> &get_creation_list();

void switch_store_lock(void);
void switch_store_unlock(void);

int object_lock(const switch_object_id_t object_id);
void object_unlock(const switch_object_id_t object_id);
bool object_exists(const switch_object_id_t object_id);
/*
 * This API adds a new entry in the map and then returns a reference to that
 * entry. In addition it also allocates space for all attributes in a single
 * allocation avoiding multiple new calls for each attribute
 * Use "value_create" to avoid querying the DB for faster insertion after this
 * call
 */
attribute_map *object_create(const switch_object_id_t object_id,
                             const ObjectInfo *object_info);
/*
 * This APIs adds a new entry in the map and and inserts all attributes also
 * The assumption is the attributes have actual values and any updates after
 * this call is a value_set.
 */
switch_status_t object_create_with_attrs(
    const switch_object_id_t object_id,
    std::vector<value_wrapper> &object_attrs);
switch_status_t object_delete(const switch_object_id_t object_id);

switch_status_t value_create(attribute_map *attr_map,
                             const switch_attr_id_t attr_id,
                             const uint64_t extra,
                             const switch_attribute_value_t &value_in);
switch_status_t value_set(const switch_object_id_t object_id,
                          const switch_attr_id_t attr_id,
                          const uint64_t extra,
                          const switch_attribute_value_t &value_in);
switch_status_t value_get(const switch_object_id_t object_id,
                          const switch_attr_id_t attr_id,
                          const uint64_t extra,
                          switch_attribute_value_t &value_out);

switch_status_t value_get_all(
    const switch_object_id_t object_id,
    std::vector<std::reference_wrapper<const switch_attribute_t>> &value_out);
switch_status_t value_delete(const switch_object_id_t object_id,
                             const switch_attr_id_t attr_id,
                             const uint64_t extra);
}  // namespace db
}  // namespace smi
#endif  // S3_STORE_H_
