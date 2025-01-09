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


#ifndef S3_SWITCH_LPM_INT_H_
#define S3_SWITCH_LPM_INT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <target-utils/third-party/judy-1.0.5/src/Judy.h>
#include <stdbool.h>
#include <stddef.h>
#include "./log.h"

typedef unsigned char byte_t;
typedef uint64_t value_t;

typedef struct switch_trie_node_s {
  Pvoid_t PJLarray_branches;
  Pvoid_t PJLarray_prefixes;
  unsigned char branch_num;
  unsigned char pref_num;
  struct switch_trie_node_s *parent;
  byte_t child_id;
} switch_trie_node_t;

typedef struct switch_lpm_trie_s {
  switch_trie_node_t *root;
  size_t key_width_bytes;
  bool release_memory;
  unsigned int num_entries;
} switch_lpm_trie_t;

switch_status_t switch_lpm_trie_create(size_t key_width_bytes,
                                       bool auto_shrink,
                                       switch_lpm_trie_t **trie);

size_t switch_lpm_trie_size(switch_lpm_trie_t *trie);

switch_status_t switch_lpm_trie_destroy(switch_lpm_trie_t *trie);

switch_status_t switch_lpm_trie_insert(switch_lpm_trie_t *trie,
                                       const uint8_t *prefix,
                                       size_t prefix_length,
                                       const value_t value);

bool switch_lpm_trie_has_prefix(const switch_lpm_trie_t *trie,
                                const uint8_t *prefix,
                                size_t prefix_length);

switch_status_t switch_lpm_trie_lookup(const switch_lpm_trie_t *trie,
                                       const uint8_t *key,
                                       value_t *pvalue);

switch_status_t switch_lpm_trie_delete(switch_lpm_trie_t *trie,
                                       const uint8_t *prefix,
                                       size_t prefix_length);

#ifdef __cplusplus
}
#endif

#endif  // S3_SWITCH_LPM_INT_H_
