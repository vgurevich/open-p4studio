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


#include <target-utils/third-party/judy-1.0.5/src/Judy.h>
#include <assert.h>

/* Local header includes */
#include "switch_lpm_int.h"

static inline switch_status_t switch_trie_node_allocate(
    switch_trie_node_t **node) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  *node = malloc(sizeof(switch_trie_node_t));
  if (!(*node)) {
    status = SWITCH_STATUS_NO_MEMORY;
    return status;
  }

  memset(*node, 0x0, sizeof(switch_trie_node_t));
  return status;
}

switch_status_t switch_lpm_trie_create(size_t key_width_bytes,
                                       bool auto_shrink,
                                       switch_lpm_trie_t **trie) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  assert(key_width_bytes <= 64);

  *trie = malloc(sizeof(switch_lpm_trie_t));
  if (!(*trie)) {
    status = SWITCH_STATUS_NO_MEMORY;
    return status;
  }

  status = switch_trie_node_allocate(&(*trie)->root);
  assert(status == SWITCH_STATUS_SUCCESS);

  (*trie)->key_width_bytes = key_width_bytes;
  (*trie)->release_memory = auto_shrink;
  (*trie)->num_entries = 0;

  return status;
}

size_t switch_lpm_trie_size(switch_lpm_trie_t *trie) {
  assert(trie != NULL);
  if (!trie) {
    return 0;
  }

  return trie->num_entries;
}

switch_status_t switch_trie_node_destroy(switch_trie_node_t *node) {
  Word_t index = 0;
  Word_t *pnode = NULL;
  Word_t rc_word;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  JLF(pnode, node->PJLarray_branches, index);
  while (pnode != NULL) {
    status = switch_trie_node_destroy(((switch_trie_node_t *)*pnode));
    JLN(pnode, node->PJLarray_branches, index);
  }

  JLFA(rc_word, node->PJLarray_branches);
  (void)rc_word;
  JLFA(rc_word, node->PJLarray_prefixes);
  (void)rc_word;
  free(node);

  return status;
}

switch_status_t switch_lpm_trie_destroy(switch_lpm_trie_t *trie) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  status = switch_trie_node_destroy(trie->root);
  assert(status == SWITCH_STATUS_SUCCESS);

  free(trie);
  return status;
}

static inline switch_trie_node_t *switch_trie_next_node_get(
    const switch_trie_node_t *current_node, byte_t byte) {
  Word_t *pnode = NULL;
  JLG(pnode, current_node->PJLarray_branches, (Word_t)byte);
  if (!pnode) return NULL;
  return (switch_trie_node_t *)*pnode;
}

static inline void switch_trie_next_node_set(switch_trie_node_t *current_node,
                                             byte_t byte,
                                             switch_trie_node_t *next_node) {
  Word_t *pnode = NULL;
  JLI(pnode, current_node->PJLarray_branches, (Word_t)byte);
  *pnode = (Word_t)next_node;
  return;
}

static inline int switch_trie_branch_delete(switch_trie_node_t *current_node,
                                            byte_t byte) {
  int rc;
  JLD(rc, current_node->PJLarray_branches, (Word_t)byte);
  return rc;
}

static inline uint16_t switch_trie_prefix_key_get(uint32_t prefix_length,
                                                  byte_t byte) {
  return prefix_length ? (byte >> (8 - prefix_length)) + (prefix_length << 8)
                       : 0;
}

static inline int switch_trie_prefix_insert(switch_trie_node_t *current_node,
                                            uint16_t prefix_key,
                                            const value_t value) {
  Word_t *pvalue;
  int rc;
  JLI(pvalue, current_node->PJLarray_prefixes, (Word_t)prefix_key);
  rc = (*pvalue) ? 1 : 0;
  *pvalue = (Word_t)value;
  return rc;
}

static inline value_t *switch_trie_prefix_ptr_get(
    const switch_trie_node_t *current_node, uint16_t prefix_key) {
  Word_t *pvalue = NULL;
  JLG(pvalue, current_node->PJLarray_prefixes, (Word_t)prefix_key);
  return (value_t *)pvalue;
}

static inline int32_t switch_trie_prefix_delete(
    switch_trie_node_t *current_node, uint16_t prefix_key) {
  int rc = 0;
  JLD(rc, current_node->PJLarray_prefixes, (Word_t)prefix_key);
  return rc;
}

switch_status_t switch_lpm_trie_insert(switch_lpm_trie_t *trie,
                                       const uint8_t *prefix,
                                       size_t prefix_length,
                                       const value_t value) {
  switch_trie_node_t *current_node = trie->root;
  byte_t byte = 0;
  uint16_t prefix_key = 0;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  while (prefix_length >= 8) {
    byte = (byte_t)*prefix;
    switch_trie_node_t *node = switch_trie_next_node_get(current_node, byte);
    if (!node) {
      status = switch_trie_node_allocate(&node);
      if (node) {
        node->parent = current_node;
        node->child_id = byte;
        switch_trie_next_node_set(current_node, byte, node);
        current_node->branch_num++;
      } else {
        assert(status == SWITCH_STATUS_SUCCESS);
        return status;
      }
    }

    prefix++;
    prefix_length -= 8;
    current_node = node;
  }

  if (prefix_length != 0) {
    prefix_key =
        switch_trie_prefix_key_get((unsigned)prefix_length, (byte_t)*prefix);
  }

  if (!switch_trie_prefix_insert(current_node, prefix_key, value))
    current_node->pref_num++;

  trie->num_entries++;
  return status;
}

bool switch_lpm_trie_has_prefix(const switch_lpm_trie_t *trie,
                                const uint8_t *prefix,
                                size_t prefix_length) {
  switch_trie_node_t *current_node = trie->root;
  byte_t byte = 0;
  uint16_t prefix_key = 0;

  while (prefix_length >= 8) {
    byte = (byte_t)*prefix;
    switch_trie_node_t *node = switch_trie_next_node_get(current_node, byte);
    if (!node) return false;
    prefix++;
    prefix_length -= 8;
    current_node = node;
  }

  if (prefix_length != 0) {
    prefix_key =
        switch_trie_prefix_key_get((unsigned)prefix_length, (byte_t)*prefix);
  }

  return (switch_trie_prefix_ptr_get(current_node, prefix_key) != NULL);
}

switch_status_t switch_lpm_trie_lookup(const switch_lpm_trie_t *trie,
                                       const uint8_t *key,
                                       value_t *pvalue) {
  const switch_trie_node_t *current_node = trie->root;
  size_t key_width = trie->key_width_bytes;
  byte_t byte;
  value_t *pdata = NULL;
  uint16_t prefix_key;
  unsigned i;
  switch_status_t status = SWITCH_STATUS_ITEM_NOT_FOUND;

  while (current_node) {
    pdata = switch_trie_prefix_ptr_get(current_node, 0);
    if (pdata) {
      *pvalue = *pdata;
      status = SWITCH_STATUS_SUCCESS;
    }

    if (key_width > 0) {
      byte = (byte_t)*key;
      for (i = 7; i > 0; i--) {
        prefix_key = switch_trie_prefix_key_get((unsigned)i, byte);
        pdata = switch_trie_prefix_ptr_get(current_node, prefix_key);
        if (pdata) {
          *pvalue = *pdata;
          status = SWITCH_STATUS_SUCCESS;
          break;
        }
      }

      current_node = switch_trie_next_node_get(current_node, byte);
      key++;
      key_width--;
    } else {
      break;
    }
  }

  return status;
}

switch_status_t switch_lpm_trie_delete(switch_lpm_trie_t *trie,
                                       const uint8_t *prefix,
                                       size_t prefix_length) {
  switch_trie_node_t *current_node = trie->root;
  byte_t byte = 0;
  uint16_t prefix_key = 0;
  value_t *pdata = NULL;

  while (prefix_length >= 8) {
    byte = (byte_t)*prefix;
    switch_trie_node_t *node = switch_trie_next_node_get(current_node, byte);
    if (!node) return SWITCH_STATUS_FAILURE;

    prefix++;
    prefix_length -= 8;
    current_node = node;
  }

  if (prefix_length != 0) {
    prefix_key =
        switch_trie_prefix_key_get((unsigned)prefix_length, (byte_t)*prefix);
  }

  pdata = switch_trie_prefix_ptr_get(current_node, prefix_key);
  if (!pdata) return SWITCH_STATUS_FAILURE;

  if (trie->release_memory) {
    if (switch_trie_prefix_delete(current_node, prefix_key) != 1) {
      return SWITCH_STATUS_FAILURE;
    }
    current_node->pref_num--;
    while (current_node->pref_num == 0 && current_node->branch_num == 0) {
      switch_trie_node_t *tmp_node = current_node;
      current_node = current_node->parent;
      if (!current_node) break;
      if (switch_trie_branch_delete(current_node, tmp_node->child_id) != 1) {
        return SWITCH_STATUS_FAILURE;
      }
      free(tmp_node);
      current_node->branch_num--;
    }
  }

  trie->num_entries--;
  return SWITCH_STATUS_SUCCESS;
}
