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


#ifndef __MC_MGR_RDM_H__
#define __MC_MGR_RDM_H__

#include <target-utils/bitset/bitset.h>
#include <target-utils/power2_allocator/power2_allocator.h>
#include <mc_mgr/mc_mgr_config.h>
#include <lld/lld_sku.h>

/* BLK_SIZE is describing the number of 40b nodes in one RDM block */
#define TOF_MC_MGR_RDM_BLK_ID_BITWIDTH 2
#define TOF_MC_MGR_RDM_BLK_COUNT 204
#define TOF_MC_MGR_RDM_BLK_SIZE (2 * 2048)
#define TOF_MC_MGR_RDM_LINE_COUNT \
  (TOF_MC_MGR_RDM_BLK_COUNT * (TOF_MC_MGR_RDM_BLK_SIZE / 2))

#define TOF2_MC_MGR_RDM_BLK_ID_BITWIDTH 2
#define TOF2_MC_MGR_RDM_BLK_COUNT 256
#define TOF2_MC_MGR_RDM_BLK_SIZE (2 * 2048)
#define TOF2_MC_MGR_RDM_LINE_COUNT \
  (TOF2_MC_MGR_RDM_BLK_COUNT * (TOF2_MC_MGR_RDM_BLK_SIZE / 2))

#define TOF3_MC_MGR_RDM_BLK_ID_BITWIDTH 4
#define TOF3_MC_MGR_RDM_BLK_COUNT (256)
#define TOF3_MC_MGR_RDM_BLK_SIZE (2 * 2048)
#define TOF3_MC_MGR_RDM_LINE_COUNT \
  (TOF3_MC_MGR_RDM_BLK_COUNT * (TOF3_MC_MGR_RDM_BLK_SIZE / 2))

/* RDM Memory Management. */
typedef struct mc_mgr_rdm_block_t mc_mgr_rdm_block_t;
typedef struct mc_mgr_rdm_t mc_mgr_rdm_t;
typedef struct mc_mgr_rdm_line_t mc_mgr_rdm_line_t;

struct mc_mgr_rdm_block_t {
  /* Memory allocator for the RDM addresses contained in this block. */
  power2_allocator_t *p2a;
  /* Back pointer to the mc_mgr_rdm_t holding this block. */
  mc_mgr_rdm_t *rdm;
  uint8_t id;

  /* Two "lists" of free entries, one tracking entries to be freed as soon as
   * the hardware notified "RDM change complete" and the other tracking
   * entries to be freed on the subsequent HW notification. */
  bf_bitset_t free_entries[2];
  uint64_t *free_entries_[2];
  /* Entries waiting on RDM change from HW to complete.  Points to one of the
   * free_entries bf_bitset_t structures above. */
  bf_bitset_t *waiting_free_entries;
  /* The next batch of "free" entries which will become "waiting" on the next
   * start of an RDM change.  Points to the other free_entries bf_bitset_t
   * structure above. */
  bf_bitset_t *queued_free_entries;

  /* Which pipe this block is assigned to, -1 when free. */
  int pipe;
};

/* RDM Nodes. */
typedef enum mc_mgr_rdm_node_type_e {
  mc_mgr_rdm_node_type_invalid = 0,
  mc_mgr_rdm_node_type_rid,
  mc_mgr_rdm_node_type_xid,
  mc_mgr_rdm_node_type_end,
  mc_mgr_rdm_node_type_ecmp,
  mc_mgr_rdm_node_type_ecmp_xid,
  mc_mgr_rdm_node_type_vector,
  mc_mgr_rdm_node_type_port18,
  mc_mgr_rdm_node_type_port72,
  mc_mgr_rdm_node_type_lag,
} mc_mgr_rdm_node_type_e;

#define MC_MGR_RDM_NODE_TYPE_STR(t)                                                  \
  (mc_mgr_rdm_node_type_invalid == (t)                                               \
       ? "Invalid"                                                                   \
       : mc_mgr_rdm_node_type_rid == (t)                                             \
             ? "Rid"                                                                 \
             : mc_mgr_rdm_node_type_xid == (t)                                       \
                   ? "Xid"                                                           \
                   : mc_mgr_rdm_node_type_end == (t)                                 \
                         ? "End"                                                     \
                         : mc_mgr_rdm_node_type_ecmp == (t)                          \
                               ? "Ecmp"                                              \
                               : mc_mgr_rdm_node_type_ecmp_xid == (t)                \
                                     ? "EcmpXid"                                     \
                                     : mc_mgr_rdm_node_type_vector == (t)            \
                                           ? "EcmpVec"                               \
                                           : mc_mgr_rdm_node_type_port18 ==          \
                                                     (t)                             \
                                                 ? "Port18"                          \
                                                 : mc_mgr_rdm_node_type_port72 ==    \
                                                           (t)                       \
                                                       ? "Port72"                    \
                                                       : mc_mgr_rdm_node_type_lag == \
                                                                 (t)                 \
                                                             ? "Lag"                 \
                                                             : "Unknown")

/* L1 RID (0x1) */
struct mc_mgr_rdm_rid {
  uint32_t next_l1;
  uint32_t next_l2;
  uint16_t rid;
};

/* L1 RID XID (0x2) */
struct mc_mgr_rdm_xid {
  uint32_t next_l1;
  uint32_t next_l2;
  uint16_t rid;
  uint16_t xid;
};

/* L1 RID no L1_next (0x3) */
struct mc_mgr_rdm_end {
  uint32_t next_l2;
  uint16_t rid;
};

/* L1 ECMP Pointer (0x5) */
struct mc_mgr_rdm_ecmp {
  uint32_t next_l1;
  uint32_t vector0;
  uint32_t vector1;
};

/* L1 ECMP Pointer XID (0x6) */
struct mc_mgr_rdm_ecmp_xid {
  uint32_t next_l1;
  uint32_t vector0;
  uint32_t vector1;
  uint16_t xid;
};

/* L1 ECMP Vector (0x4) */
struct mc_mgr_rdm_vector {
  uint32_t base_l1;
  uint8_t length;
  uint32_t vector;
  uint16_t id;
};

/* L2 Port 18 (0x8) */
struct mc_mgr_rdm_port18 {
  uint8_t pipe;
  bool last;
  uint8_t spv;
  uint16_t ports;
};

/* L2 Port 72 (0x9) */
struct mc_mgr_rdm_port72 {
  uint8_t pipe;
  bool last;
  uint8_t spv;
  uint64_t ports;
};

/* L2 LAG (0xC) */
struct mc_mgr_rdm_lag {
  uint32_t next_l2;
  uint8_t lag_id;
};

/* RDM line buffer.
 *
 * Represents the contents of one 80b physical word, comprising
 * one 80b or two 40b nodes.
 *
 * Bits 19:1 of the RDM address specify the block and word number.
 * Bit 0 specifies the subword number.
 *
 * In cases where the physical word contains two nodes, the decoded
 * values are represented by two-element arrays. The less significant
 * node is in element [0]; the more significant in element [1].
 */
struct mc_mgr_rdm_line_t {
  /* Decoded node types. */
  mc_mgr_rdm_node_type_e type[2];

  /* Encoded node contents (raw data).
   * Less significant 40b subword is in data[0].
   * More significant 40b subword is in data[1]. */
  uint64_t data[2];

  /* Decoded node contents. */
  union {
    /* L1 RID */
    struct mc_mgr_rdm_rid rid;
    /* L1 RID XID */
    struct mc_mgr_rdm_xid xid;
    /* L1 RID no L1_next */
    struct mc_mgr_rdm_end end[2];
    /* L1 ECMP Pointer */
    struct mc_mgr_rdm_ecmp ecmp;
    /* L1 ECMP Pointer XID */
    struct mc_mgr_rdm_ecmp_xid ecmp_xid;
    /* L1 ECMP Vector */
    struct mc_mgr_rdm_vector vector;
    /* L2 Port 18 */
    struct mc_mgr_rdm_port18 port18[2];
    /* L2 Port 72 */
    struct mc_mgr_rdm_port72 port72;
    /* L2 LAG */
    struct mc_mgr_rdm_lag lag[2];
  } u;
};

struct mc_mgr_mgid_list_t {
  struct mc_mgr_mgid_list_t *next;
  int mgid;
  int update_length;
};

struct mc_mgr_rdm_t {
  bf_bitset_t free_blocks;
  uint64_t *free_blocks_;
  /* Per pipe, use separate blocks for L1 and L2 nodes. */
  bf_bitset_t used_blocks[MC_MGR_NUM_PIPES][2];
  uint64_t *used_blocks_[MC_MGR_NUM_PIPES][2];
  mc_mgr_rdm_block_t *blocks;
  /* Shadows pre_rdm_blk_id registers. */
  uint32_t *blk_ids;
  uint32_t *blk_ids_other_die;
  uint32_t blk_id_width;
  /* RDM contents. */
  mc_mgr_rdm_line_t *rdm;
  uint32_t rdm_line_count;
  uint32_t rdm_blk_count;
  bool rdm_pending[MC_MGR_NUM_PIPES];
  /* Two lists of deferred mgid tail re-evaluations.  The first list,
   * for_next_rdm_change, is to be processed on the next rdm-change-done event.
   * The second, after_next_rdm_change, holds pending updates that will be
   * moved to "for_next_rdm_change" on the next rdm-change-done event, that is,
   * they will be processed after two rdm-change-events. */
  struct mc_mgr_mgid_list_t *for_next_rdm_change[MC_MGR_NUM_PIPES];
  struct mc_mgr_mgid_list_t *after_next_rdm_change[MC_MGR_NUM_PIPES];
  /* A mutex to protect accesses to all lists used to store operations pending
   * on RDM changes.  This is the mgid lists above and the lists in each of the
   * mc_mgr_rdm_block_t array elements. */
  mc_mutex_t rdm_change_list_mtx;
  /* The device id the RDM belongs to. */
  bf_dev_id_t dev;
};

static inline mc_mgr_rdm_line_t *get_rdm_line(mc_mgr_rdm_t *r, int line) {
  if (!(line >= 0 && (line < TOF_MC_MGR_RDM_LINE_COUNT ||
                      line < TOF2_MC_MGR_RDM_LINE_COUNT))) {
    MC_MGR_DBGCHK(0);
    return NULL;
  }
  return &r->rdm[line];
}

static inline uint32_t mc_mgr_rdm_num_max_pipes(int dev) {
  uint32_t num_subdevices = 0;
  int mc_mgr_2die_factor = 1;
  lld_sku_get_num_subdev(dev, &num_subdevices, NULL);
#if defined(EMU_2DIE_USING_SW_2DEV)
  mc_mgr_2die_factor = 2;
#else
  mc_mgr_2die_factor = 1;
#endif
  return (BF_SUBDEV_PIPE_COUNT * num_subdevices * mc_mgr_2die_factor);
}

static inline void rdm_pending_set(mc_mgr_rdm_t *r, int pipe) {
  if (pipe < 0 || pipe >= (int)mc_mgr_rdm_num_max_pipes(r->dev)) {
    MC_MGR_DBGCHK(0);
    return;
  }

  r->rdm_pending[pipe] = true;
}
static inline void rdm_pending_clr(mc_mgr_rdm_t *r, int pipe) {
  if (pipe < 0 || pipe >= (int)mc_mgr_rdm_num_max_pipes(r->dev)) {
    MC_MGR_DBGCHK(0);
    return;
  }

  r->rdm_pending[pipe] = false;
}
static inline bool rdm_pending_get(mc_mgr_rdm_t *r, int pipe) {
  if (pipe < 0 || pipe >= (int)mc_mgr_rdm_num_max_pipes(r->dev)) {
    MC_MGR_DBGCHK(0);
    return false;
  }
  return r->rdm_pending[pipe];
}

typedef struct mc_mgr_rdm_addr_list_t {
  struct mc_mgr_rdm_addr_list_t *next;
  struct mc_mgr_rdm_addr_list_t *prev;
  uint32_t addr;
} mc_mgr_rdm_addr_list_t;
static inline bool mc_mgr_rdm_addr_in(mc_mgr_rdm_addr_list_t *list,
                                      uint32_t addr) {
  while (list) {
    if (list->addr == addr) return true;
    list = list->next;
  }
  return false;
}
static inline void mc_mgr_rdm_addr_append(mc_mgr_rdm_addr_list_t **list,
                                          uint32_t addr) {
  if (!list) {
    MC_MGR_DBGCHK(0);
    return;
  }
  mc_mgr_rdm_addr_list_t *x = MC_MGR_MALLOC(sizeof(mc_mgr_rdm_addr_list_t));
  if (!x) {
    MC_MGR_DBGCHK(0);
    return;
  }
  x->addr = addr;
  BF_LIST_DLL_AP(*list, x, next, prev);
}
static inline void mc_mgr_rdm_addr_pop(mc_mgr_rdm_addr_list_t **list,
                                       uint32_t *addr) {
  if (!list) {
    MC_MGR_DBGCHK(0);
    return;
  }

  if (!addr) {
    MC_MGR_DBGCHK(0);
    return;
  }

  mc_mgr_rdm_addr_list_t *x = *list;
  BF_LIST_DLL_REM(*list, x, next, prev);
  *addr = x->addr;
  MC_MGR_FREE(x);
}

static inline void mc_mgr_rdm_addr_peek_by_value(mc_mgr_rdm_addr_list_t **list,
                                                 uint32_t *addr) {
  if (!list) {
    MC_MGR_DBGCHK(0);
    return;
  }

  if (!addr || *addr) {
    MC_MGR_DBGCHK(0);
    return;
  }

  mc_mgr_rdm_addr_list_t *x = *list;
  *addr = x ? x->addr : 0;
}

static inline int tof_rdm_addr_to_blk(bf_dev_id_t dev, int addr) {
  (void)dev;
  return addr / TOF_MC_MGR_RDM_BLK_SIZE;
}
static inline int tof2_rdm_addr_to_blk(bf_dev_id_t dev, int addr) {
  (void)dev;
  return addr / TOF2_MC_MGR_RDM_BLK_SIZE;
}
static inline int tof3_rdm_addr_to_blk(bf_dev_id_t dev, int addr) {
  (void)dev;
  return addr / TOF3_MC_MGR_RDM_BLK_SIZE;
}
static inline int rdm_addr_to_blk_addr(bf_dev_id_t dev, int addr) {
  (void)dev;
  return addr & 0xFFF;
}
static inline int rdm_addr_to_line(bf_dev_id_t dev, int addr) {
  (void)dev;
  return addr >> 1;
}
static inline void mark_block_used(mc_mgr_rdm_t *rdm_map,
                                   int pipe,
                                   int blk,
                                   bool is_l2_node) {
  if (pipe < 0 || pipe >= (int)mc_mgr_rdm_num_max_pipes(rdm_map->dev)) {
    MC_MGR_DBGCHK(0);
    return;
  }
  MC_MGR_DBGCHK(-1 == rdm_map->blocks[blk].pipe ||
                pipe == rdm_map->blocks[blk].pipe);
  /* Remove from free block list. */
  bf_bs_set(&rdm_map->free_blocks, blk, 0);
  /* Add to the pipe's used block list. */
  bf_bs_set(&rdm_map->used_blocks[pipe][is_l2_node], blk, 1);
  rdm_map->blocks[blk].pipe = pipe;
  /* Update the shadow of block to pipe mapping registers. */
  unsigned int p = pipe;
  unsigned int p_other_die = p ^ 0x4;
  if (rdm_map->blk_id_width == TOF3_MC_MGR_RDM_BLK_ID_BITWIDTH) {
    rdm_map->blk_ids[blk / 8] =
        (rdm_map->blk_ids[blk / 8] & ~(0xf << (4 * (blk & 0x7)))) |
        (p << (4 * (blk & 0x7)));
    rdm_map->blk_ids_other_die[blk / 8] =
        (rdm_map->blk_ids_other_die[blk / 8] & ~(0xf << (4 * (blk & 0x7)))) |
        (p_other_die << (4 * (blk & 0x7)));
  } else {
    rdm_map->blk_ids[blk / 16] =
        (rdm_map->blk_ids[blk / 16] & ~(3u << (2 * (blk & 0xF)))) |
        (p << (2 * (blk & 0xF)));
    rdm_map->blk_ids_other_die[blk / 16] =
        (rdm_map->blk_ids[blk / 16] & ~(3u << (2 * (blk & 0xF)))) |
        (p_other_die << (2 * (blk & 0xF)));
  }
}
static inline void mark_block_free(mc_mgr_rdm_t *rdm_map, int pipe, int blk) {
  if (pipe < 0 || pipe >= (int)mc_mgr_rdm_num_max_pipes(rdm_map->dev)) {
    MC_MGR_DBGCHK(0);
    return;
  }
  MC_MGR_DBGCHK(pipe == rdm_map->blocks[blk].pipe);
  /* Remove from used block list. */
  bf_bs_set(&rdm_map->used_blocks[pipe][0], blk, 0);
  bf_bs_set(&rdm_map->used_blocks[pipe][1], blk, 0);
  /* Add to the free block list. */
  bf_bs_set(&rdm_map->free_blocks, blk, 1);
  rdm_map->blocks[blk].pipe = -1;
  if (rdm_map->blk_id_width == TOF3_MC_MGR_RDM_BLK_ID_BITWIDTH) {
    rdm_map->blk_ids[blk / 8] &= ~(0xf << (4 * (blk & 0x7)));
    rdm_map->blk_ids_other_die[blk / 8] &= ~(0xf << (4 * (blk & 0x7)));
  } else {
    rdm_map->blk_ids[blk / 16] &= ~(0x3 << (2 * (blk & 0xF)));
    rdm_map->blk_ids_other_die[blk / 16] &= ~(0x3 << (2 * (blk & 0xF)));
  }
}

static inline int node_width(mc_mgr_rdm_node_type_e t) {
  switch (t) {
    case mc_mgr_rdm_node_type_invalid:
      return 1;
    case mc_mgr_rdm_node_type_rid:
      return 2;
    case mc_mgr_rdm_node_type_xid:
      return 2;
    case mc_mgr_rdm_node_type_end:
      return 1;
    case mc_mgr_rdm_node_type_ecmp:
      return 2;
    case mc_mgr_rdm_node_type_ecmp_xid:
      return 2;
    case mc_mgr_rdm_node_type_vector:
      return 2;
    case mc_mgr_rdm_node_type_port18:
      return 1;
    case mc_mgr_rdm_node_type_port72:
      return 2;
    case mc_mgr_rdm_node_type_lag:
      return 1;
    default:
      break;
  }
  MC_MGR_DBGCHK(0);
  return 0;
}
static inline bool node_is_l1(mc_mgr_rdm_node_type_e t) {
  switch (t) {
    case mc_mgr_rdm_node_type_invalid:
      return false;
    case mc_mgr_rdm_node_type_rid:
      return true;
    case mc_mgr_rdm_node_type_xid:
      return true;
    case mc_mgr_rdm_node_type_end:
      return true;
    case mc_mgr_rdm_node_type_ecmp:
      return true;
    case mc_mgr_rdm_node_type_ecmp_xid:
      return true;
    case mc_mgr_rdm_node_type_vector:
      return true;
    case mc_mgr_rdm_node_type_port18:
      return false;
    case mc_mgr_rdm_node_type_port72:
      return false;
    case mc_mgr_rdm_node_type_lag:
      return false;
    default:
      break;
  }
  MC_MGR_DBGCHK(0);
  return false;
}
static inline bool node_is_first_class_l1(mc_mgr_rdm_node_type_e t) {
  switch (t) {
    case mc_mgr_rdm_node_type_invalid:
      return false;
    case mc_mgr_rdm_node_type_rid:
      return true;
    case mc_mgr_rdm_node_type_xid:
      return true;
    case mc_mgr_rdm_node_type_end:
      return true;
    case mc_mgr_rdm_node_type_ecmp:
      return true;
    case mc_mgr_rdm_node_type_ecmp_xid:
      return true;
    case mc_mgr_rdm_node_type_vector:
      return false;
    case mc_mgr_rdm_node_type_port18:
      return false;
    case mc_mgr_rdm_node_type_port72:
      return false;
    case mc_mgr_rdm_node_type_lag:
      return false;
    default:
      break;
  }
  MC_MGR_DBGCHK(0);
  return false;
}
static inline bool node_is_l2(mc_mgr_rdm_node_type_e t) {
  switch (t) {
    case mc_mgr_rdm_node_type_invalid:
      return false;
    case mc_mgr_rdm_node_type_rid:
      return false;
    case mc_mgr_rdm_node_type_xid:
      return false;
    case mc_mgr_rdm_node_type_end:
      return false;
    case mc_mgr_rdm_node_type_ecmp:
      return false;
    case mc_mgr_rdm_node_type_ecmp_xid:
      return false;
    case mc_mgr_rdm_node_type_vector:
      return false;
    case mc_mgr_rdm_node_type_port18:
      return true;
    case mc_mgr_rdm_node_type_port72:
      return true;
    case mc_mgr_rdm_node_type_lag:
      return true;
    default:
      break;
  }
  MC_MGR_DBGCHK(0);
  return false;
}

void mc_mgr_rdm_map_init(mc_mgr_rdm_t **rdm_map_p,
                         bf_dev_id_t dev,
                         bf_dev_family_t dev_family,
                         uint32_t max_pipes);
void mc_mgr_rdm_map_cleanup(mc_mgr_rdm_t **rdm_map);
static inline bool tof_rdm_addr_valid(uint32_t addr) {
  return addr && addr < (TOF_MC_MGR_RDM_BLK_COUNT * TOF_MC_MGR_RDM_BLK_SIZE);
}
static inline bool tof2_rdm_addr_valid(uint32_t addr) {
  return addr && addr < (TOF2_MC_MGR_RDM_BLK_COUNT * TOF2_MC_MGR_RDM_BLK_SIZE);
}
static inline bool tof3_rdm_addr_valid(uint32_t addr) {
  return addr && addr < (TOF3_MC_MGR_RDM_BLK_COUNT * TOF3_MC_MGR_RDM_BLK_SIZE);
}
static inline mc_mgr_rdm_node_type_e mc_mgr_rdm_type(mc_mgr_rdm_t *rdm_map,
                                                     uint32_t addr) {
  return rdm_map->rdm[addr / 2].type[addr & 1];
}
static inline bool rdm_line_invalid(mc_mgr_rdm_line_t *line) {
  return line->type[0] == mc_mgr_rdm_node_type_invalid &&
         line->type[1] == mc_mgr_rdm_node_type_invalid;
}
static inline int get_pipe_from_port_node(mc_mgr_rdm_t *rdm_map,
                                          uint32_t addr) {
  if (rdm_map->rdm[addr / 2].type[addr & 1] == mc_mgr_rdm_node_type_port72) {
    return rdm_map->rdm[addr / 2].u.port72.pipe;
  } else if (rdm_map->rdm[addr / 2].type[addr & 1] ==
             mc_mgr_rdm_node_type_port18) {
    return rdm_map->rdm[addr / 2].u.port18[addr & 1].pipe;
  } else {
    MC_MGR_DBGCHK(0);
    return ~0;
  }
}
bf_status_t mc_mgr_rdm_decode_line(bf_dev_id_t dev, mc_mgr_rdm_line_t *line);
uint32_t mc_mgr_rdm_next_l1(mc_mgr_rdm_t *rdm_map, uint32_t addr);
void mc_mgr_rdm_set_next_l1(mc_mgr_rdm_t *rdm_map,
                            uint32_t addr,
                            uint32_t next_l1);
void mc_mgr_rdm_write_ptr(int sid,
                          bf_dev_id_t dev,
                          uint32_t addr,
                          uint32_t p0,
                          uint32_t p1,
                          uint32_t next,
                          uint16_t xid,
                          bool use_xid);
void mc_mgr_rdm_write_vec(int sid,
                          bf_dev_id_t dev,
                          uint32_t addr,
                          uint32_t vec,
                          uint32_t base,
                          uint16_t id);
uint16_t mc_mgr_rdm_get_id_from_vec(mc_mgr_rdm_t *rdm_map, uint32_t addr);
uint32_t mc_mgr_rdm_l1_node_get_l2_ptr(mc_mgr_rdm_t *rdm_map, uint32_t addr);
uint32_t mc_mgr_rdm_next_l2(mc_mgr_rdm_t *rdm_map,
                            uint32_t addr,
                            bool block_level);
void mc_mgr_rdm_get_port_node_from_l1(bf_dev_id_t dev,
                                      uint32_t addr,
                                      uint32_t *port_node,
                                      uint32_t *prev_node);
void mc_mgr_rdm_unlink_and_get_lag_node_by_lag_id(
    int sid, bf_dev_id_t dev, uint32_t l1_addr, uint32_t *lag_node, int lag_id);
uint32_t mc_mgr_rdm_get_next_l1(bf_dev_id_t dev, uint32_t addr);
void mc_mgr_rdm_update_next_l1(int sid,
                               bf_dev_id_t dev,
                               uint32_t addr,
                               uint32_t next_l1);
void mc_mgr_rdm_update_next_l2(int sid,
                               bf_dev_id_t dev,
                               uint32_t addr,
                               uint32_t next_l2);
void mc_mgr_rdm_invalidate_node(int sid, bf_dev_id_t dev, uint32_t addr);
void mc_mgr_rdm_write_l1(int sid,
                         bf_dev_id_t dev,
                         uint32_t addr,
                         uint32_t next_l1,
                         uint32_t l2,
                         uint16_t rid,
                         uint16_t xid,
                         bool xid_val);
void mc_mgr_rdm_write_l1_end(
    int sid, bf_dev_id_t dev, uint32_t addr, uint32_t l2, uint16_t rid);
void mc_mgr_rdm_write_port72(int sid,
                             bf_dev_id_t dev,
                             uint32_t addr,
                             uint8_t pipe,
                             bool last,
                             bf_bitset_t *ports);
void mc_mgr_rdm_write_port18(int sid,
                             bf_dev_id_t dev,
                             uint8_t pipe,
                             uint32_t addr,
                             bool last,
                             bf_bitset_t *ports);
void mc_mgr_rdm_write_lag(
    int sid, bf_dev_id_t dev, uint32_t addr, uint8_t lag_id, uint32_t next_l2);
uint32_t mc_mgr_rdm_map_get(
    int sid, bf_dev_id_t dev, int pipe, mc_mgr_rdm_node_type_e type, int count);
bool mc_mgr_mark_addr_used(mc_mgr_rdm_t *rdm_map,
                           bf_dev_id_t dev,
                           int addr,
                           int pipe,
                           int sz,
                           bool is_l2_node);
bool mc_mgr_rdm_mark_addr_free(mc_mgr_rdm_t *rdm_map,
                               bf_dev_id_t dev,
                               int addr,
                               int pipe);
void mc_mgr_rdm_map_return(bf_dev_id_t dev, uint32_t addr);
void mc_mgr_rdm_map_free(bf_dev_id_t dev, uint32_t addr);
void mc_mgr_rdm_map_enqueue_free(int sid, bf_dev_id_t dev, uint32_t addr);
void mc_mgr_start_rdm_change_all_pipes(bf_dev_id_t dev, const char *who);
void mc_mgr_rdm_change_done(int sid,
                            bf_dev_id_t dev,
                            int pipe,
                            bool restartable);
void mc_mgr_wait_rdm_change(int sid, bf_dev_id_t dev, int pipe);
void mc_mgr_enqueue_mgid_tail_update(bf_dev_id_t dev,
                                     int pipe,
                                     int mgid,
                                     int length);
bf_status_t mc_mgr_rdm_reserve_tails(int sid, bf_dev_id_t dev, int pipe);
uint32_t mc_mgr_rdm_get_l2_lagid(bf_dev_id_t dev, uint32_t addr);
#endif
