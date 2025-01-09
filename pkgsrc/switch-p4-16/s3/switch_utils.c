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


#include "s3/switch_packet.h"
#include "switch_utils.h"
#ifdef TESTING
#include "test/test_packet.h"
#endif
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

switch_uint32_t MurmurHash(const void *key,
                           switch_uint32_t length,
                           switch_uint32_t seed) {
// 'm' and 'r' are mixing constants generated offline.
// They're not really 'magic', they just happen to work well.
#define m 0x5bd1e995
#define r 24

  // Initialize the hash to a 'random' value
  switch_uint32_t h = seed ^ (switch_uint32_t)length;

  // Mix 4 bytes at a time into the hash
  const unsigned char *data = (const unsigned char *)key;

  while (length >= 4) {
    uint32_t k = *(uint32_t *)data;

    k *= m;
    k ^= k >> r;
    k *= m;
    h *= m;
    h ^= k;

    data += 4;
    length -= 4;
  }

  // Handle the last few bytes of the input array

  switch (length) {
    case 3:
      h ^= data[2] << 16;
    /* fall through */
    case 2:
      h ^= data[1] << 8;
    /* fall through */
    case 1:
      h ^= data[0];
      h *= m;
  };

  // Do a few final mixes of the hash to ensure the last few
  // bytes are well-incorporated.

  h ^= h >> 13;
  h *= m;
  h ^= h >> 15;

  return h;
}

switch_status_t SWITCH_ARRAY_INIT(switch_array_t *array) {
  SWITCH_ASSERT(array != NULL);
  array->array = NULL;
  array->num_entries = 0;
  return SWITCH_STATUS_SUCCESS;
}

switch_uint32_t SWITCH_ARRAY_COUNT(switch_array_t *array) {
  SWITCH_ASSERT(array != NULL);
  return array->num_entries;
}

switch_status_t SWITCH_ARRAY_INSERT(switch_array_t *array,
                                    switch_uint64_t index,
                                    void *data) {
  SWITCH_ASSERT(array != NULL);
  SWITCH_ASSERT(data != NULL);

  void *p = NULL;
  JLI(p, array->array, (switch_uint64_t)index);
  if (p) {
    *(switch_uint64_t *)p = (uintptr_t)data;
    array->num_entries++;
    return SWITCH_STATUS_SUCCESS;
  } else {
    return SWITCH_STATUS_NO_MEMORY;
  }
}

switch_status_t SWITCH_ARRAY_GET(switch_array_t *array,
                                 switch_uint64_t index,
                                 void **data) {
  SWITCH_ASSERT(array != NULL);
  SWITCH_ASSERT(data != NULL);

  void *p = NULL;
  JLG(p, array->array, (switch_uint64_t)index);
  if (p) {
    *(switch_uint64_t *)data = *(switch_uint64_t *)p;
    return SWITCH_STATUS_SUCCESS;
  } else {
    return SWITCH_STATUS_ITEM_NOT_FOUND;
  }
}

switch_status_t SWITCH_ARRAY_DELETE(switch_array_t *array,
                                    switch_uint64_t index) {
  SWITCH_ASSERT(array != NULL);

  switch_int32_t rc = 0;
  JLD(rc, array->array, (switch_uint64_t)index);
  if (rc == 1) {
    array->num_entries--;
    return SWITCH_STATUS_SUCCESS;
  } else {
    return SWITCH_STATUS_ITEM_NOT_FOUND;
  }
}

switch_status_t SWITCH_LIST_INIT(switch_list_t *list) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (!list) {
    status = SWITCH_STATUS_INVALID_PARAMETER;
    SWITCH_LOG_ERROR("list insert failed(%s)\n",
                     switch_error_to_string(status));
    return status;
  }
  tommy_list_init(&list->list);
  list->num_entries = 0;
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t SWITCH_LIST_SORT(switch_list_t *list,
                                 switch_list_compare_func_t compare_func) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (!list) {
    status = SWITCH_STATUS_INVALID_PARAMETER;
    SWITCH_LOG_ERROR("list insert failed(%s)\n",
                     switch_error_to_string(status));
    return status;
  }
  tommy_list_sort(&list->list, compare_func);
  return SWITCH_STATUS_SUCCESS;
}

bool SWITCH_LIST_EMPTY(switch_list_t *list) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  bool is_empty = false;

  if (!list) {
    status = SWITCH_STATUS_INVALID_PARAMETER;
    SWITCH_LOG_ERROR("list empty get failed(%s)\n",
                     switch_error_to_string(status));
    return false;
  }

  is_empty = tommy_list_empty(&list->list);
  return is_empty;
}

switch_size_t SWITCH_LIST_COUNT(switch_list_t *list) {
  if (!list) {
    return 0;
  }

  return list->num_entries;
}

switch_status_t SWITCH_LIST_INSERT(switch_list_t *list,
                                   switch_node_t *node,
                                   void *data) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (!list || !node || !data) {
    status = SWITCH_STATUS_INVALID_PARAMETER;
    SWITCH_LOG_ERROR("list insert failed(%s)\n",
                     switch_error_to_string(status));
    return status;
  }
  tommy_list_insert_head(&list->list, node, data);
  list->num_entries++;
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t SWITCH_LIST_DELETE(switch_list_t *list, switch_node_t *node) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (!list || !node) {
    status = SWITCH_STATUS_INVALID_PARAMETER;
    SWITCH_LOG_ERROR("list delete failed(%s)\n",
                     switch_error_to_string(status));
    return status;
  }
  tommy_list_remove_existing(&list->list, node);
  list->num_entries--;
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t SWITCH_HASHTABLE_INIT(switch_hashtable_t *hashtable) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (!hashtable || hashtable->size == 0) {
    status = SWITCH_STATUS_INVALID_PARAMETER;
    SWITCH_LOG_ERROR("hashtable init failed(%s)\n",
                     switch_error_to_string(status));
    return status;
  }
  tommy_hashtable_init(&hashtable->table, hashtable->size);
  hashtable->num_entries = 0;
  return SWITCH_STATUS_SUCCESS;
}

switch_size_t SWITCH_HASHTABLE_COUNT(switch_hashtable_t *hashtable) {
  SWITCH_ASSERT(hashtable != NULL);

  if (!hashtable) {
    return 0;
  }

  return hashtable->num_entries;
}

switch_status_t SWITCH_HASHTABLE_INSERT(switch_hashtable_t *hashtable,
                                        switch_hashnode_t *node,
                                        void *key,
                                        void *data) {
  switch_uint8_t hash_key[SWITCH_LOG_BUFFER_SIZE];
  switch_uint32_t hash_length = 0;
  switch_uint32_t hash = 0;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (!hashtable || !node || !key || !data) {
    status = SWITCH_STATUS_INVALID_PARAMETER;
    SWITCH_LOG_ERROR("hashtable insert failed(%s)\n",
                     switch_error_to_string(status));
    return status;
  }

  memset(hash_key, 0x0, SWITCH_LOG_BUFFER_SIZE);

  status = hashtable->key_func(key, hash_key, &hash_length);
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_LOG_ERROR("hashtable insert failed(%s)\n",
                     switch_error_to_string(status));
    return status;
  }

  hash = MurmurHash(hash_key, hash_length, hashtable->hash_seed);
  tommy_hashtable_insert(&hashtable->table, node, data, hash);
  hashtable->num_entries++;
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t SWITCH_HASHTABLE_DELETE(switch_hashtable_t *hashtable,
                                        void *key,
                                        void **data) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_uint8_t hash_key[SWITCH_LOG_BUFFER_SIZE];
  switch_uint32_t hash_length = 0;
  switch_uint32_t hash = 0;

  if (!hashtable || !key || !data) {
    status = SWITCH_STATUS_INVALID_PARAMETER;
    SWITCH_LOG_ERROR("hashtable delete failed(%s)\n",
                     switch_error_to_string(status));
    return status;
  }

  memset(hash_key, 0x0, SWITCH_LOG_BUFFER_SIZE);

  status = hashtable->key_func(key, hash_key, &hash_length);
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_LOG_ERROR("hashtable delete failed(%s)\n",
                     switch_error_to_string(status));
    return status;
  }

  hash = MurmurHash(hash_key, hash_length, hashtable->hash_seed);
  *data = (void *)tommy_hashtable_remove(
      &hashtable->table, hashtable->compare_func, hash_key, hash);
  if (!(*data)) {
    status = SWITCH_STATUS_ITEM_NOT_FOUND;
    SWITCH_LOG_ERROR("hashtable delete failed(%s)\n",
                     switch_error_to_string(status));
    return status;
  }

  hashtable->num_entries--;
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t SWITCH_HASHTABLE_DELETE_NODE(switch_hashtable_t *hashtable,
                                             switch_hashnode_t *node) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (!hashtable || !node) {
    status = SWITCH_STATUS_INVALID_PARAMETER;
    SWITCH_LOG_ERROR("hashtable delete node failed(%s)\n",
                     switch_error_to_string(status));
    return status;
  }
  tommy_hashtable_remove_existing(&hashtable->table, node);
  hashtable->num_entries--;
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t SWITCH_HASHTABLE_SEARCH(switch_hashtable_t *hashtable,
                                        void *key,
                                        void **data) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_uint8_t hash_key[SWITCH_LOG_BUFFER_SIZE];
  switch_uint32_t hash_length = 0;
  switch_uint32_t hash = 0;

  if (!hashtable || !key || !data) {
    status = SWITCH_STATUS_INVALID_PARAMETER;
    SWITCH_LOG_ERROR("hashtable search failed(%s)\n",
                     switch_error_to_string(status));
    return status;
  }

  memset(hash_key, 0x0, SWITCH_LOG_BUFFER_SIZE);

  status = hashtable->key_func(key, hash_key, &hash_length);
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_LOG_ERROR("hashtable search failed(%s)\n",
                     switch_error_to_string(status));
    return status;
  }

  hash = MurmurHash(hash_key, hash_length, hashtable->hash_seed);
  *data = (void *)tommy_hashtable_search(
      &hashtable->table, hashtable->compare_func, hash_key, hash);
  if (!(*data)) {
    status = SWITCH_STATUS_ITEM_NOT_FOUND;
    SWITCH_LOG_DEBUG("hashtable search failed(%s)\n",
                     switch_error_to_string(status));
    return status;
  }

  return status;
}

switch_status_t SWITCH_HASHTABLE_FOREACH_ARG(switch_hashtable_t *hashtable,
                                             void *func,
                                             void *arg) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (!hashtable || !func || !arg) {
    status = SWITCH_STATUS_INVALID_PARAMETER;
    SWITCH_LOG_ERROR("hashtable foreach arg failed(%s)\n",
                     switch_error_to_string(status));
    return status;
  }
  tommy_hashtable_foreach_arg(&hashtable->table, func, arg);

  return SWITCH_STATUS_SUCCESS;
}

switch_status_t SWITCH_HASHTABLE_DONE(switch_hashtable_t *hashtable) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (!hashtable) {
    status = SWITCH_STATUS_INVALID_PARAMETER;
    SWITCH_LOG_ERROR("hashtable done failed(%s)\n",
                     switch_error_to_string(status));
    return status;
  }
  tommy_hashtable_done(&hashtable->table);
  return SWITCH_STATUS_SUCCESS;
}

char *switch_strncpy(char *dest, const char *src, size_t n) {
  size_t i;

  for (i = 0; i < n - 1 && src[i] != '\0'; i++) dest[i] = src[i];
  dest[i] = '\0';

  if (strlen(src) >= n) {
    SWITCH_LOG_ERROR(
        "Source string \"%s\" is bigger than maximum size to copy %zu", src, n);
  }

  return dest;
}

#ifdef __cplusplus
}
#endif
