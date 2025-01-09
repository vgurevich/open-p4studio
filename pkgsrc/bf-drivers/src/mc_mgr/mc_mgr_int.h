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


/*******************************************************************************
 *
 * mc_mgr Internal Header
 *
 *****************************************************************************/
#ifndef __MC_MGR_INT_H__
#define __MC_MGR_INT_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <target-utils/id/id.h>
#include <target-utils/map/map.h>
#include <target-utils/list/bf_list.h>
#include <target-utils/bitset/bitset.h>
#include <bf_types/bf_types.h>
#include <pipe_mgr/pipe_mgr_intf.h>
#include <pipe_mgr/pipe_mgr_mc_intf.h>

#include <mc_mgr/mc_mgr_config.h>
#include <mc_mgr/mc_mgr_types.h>
#include "mc_mgr_rdm.h"
#include "mc_mgr_log.h"
#include "mc_mgr_drv.h"
#include "mc_mgr_reg.h"
#include "mc_mgr_mem.h"

/* Tof2 TODOs
 * [ ] - mc_mgr_get_tbl_ver_reg reads only a single deparser slice; decide if
 * this is okay
 * [ ] - PVT has a fifth bit now :( need HA support.
 * [ ] - Tof2 PRE memories don't have "element size" using Tofino sizes for now
 * [ ] - RDM is bigger now!  So we can't use a #define to size it
 */
#define MC_MGID_WIDTH 64
#define MC_ECMP_GROUP_MAX_MBR (32)

#define MC_PVT_MASK_ALL 0x3

struct mc_session_ctx {
  bf_mc_session_hdl_t shdl;
  bool valid;
  bool version_enabled;
  bool in_batch;
};

struct mc_l1_node_t;
struct mc_l1_hw_node_t;
struct mc_ecmp_grp_t {
  bf_mc_ecmp_hdl_t handle;
  uint16_t ecmp_id;
  bf_dev_id_t dev;
  uint32_t valid_map;
  struct mc_l1_node_t *mbrs[MC_ECMP_GROUP_MAX_MBR];
  /* Starting address of member block in each pipe.
   * Zero if using the dummy member. */
  uint32_t base[MC_MGR_NUM_PIPES];
  uint32_t vector_node_addr[2][MC_MGR_NUM_PIPES];
  int allocated_sz;
  struct mc_l1_node_t *refs;
};

struct mc_mgr_tree_size_t {
  int hw_len;
  int sw_len;
  mc_mgr_drv_buf_t *buf;
};

struct mc_mgr_tail_info {
  uint32_t tail_l2_size;
  uint32_t num_tails;
  uint32_t tail_l2_addr[MC_MGR_NUM_PIPES];
  uint32_t tail_base[MC_MGR_NUM_PIPES];
  uint16_t *tail_size;
};

/* Port specific info */
typedef struct mc_mgr_port_info {
  struct mc_mgr_port_info *next;
  struct mc_mgr_port_info *prev;
  bf_dev_port_t port_id;
  bf_port_speeds_t speed; /* 1G, 10G, 25G, 40G, 100G, etc */
} mc_mgr_port_info_t;

/* Structure to store nodes belong to a group */
struct mc_mgr_grp_info {
  /* Group Handle */
  bf_mc_mgrp_hdl_t h;
  /* Group ID. */
  bf_mc_grp_id_t grp_id;
  /* Device ID. */
  bf_dev_id_t dev;
  /* L1 nodes associated to the group */
  bf_map_t node_mbrs;
  /* Non ECMP L1 count */
  uint16_t l1_count;
  /* ECMP L1 count */
  uint16_t ecmp_l1_count;
};

#define MC_MGR_MGID_MAP_SIZE \
  ((BF_MGID_COUNT + (MC_MGID_WIDTH - 1)) / MC_MGID_WIDTH)
#define MC_MGR_SUBDEV_PORT_COUNT 288
#define MC_MGR_PORT_COUNT (288 * 2)
#define MC_MGR_TOF3_SUBDEV_PORT_COUNT 72
#define MC_MGR_TOF3_PORT_COUNT \
  (MC_MGR_TOF3_SUBDEV_PORT_COUNT * BF_SUBDEV_PIPE_COUNT)
#define MC_MGR_BPORT_COUNT MC_MGR_PORT_COUNT
#define MC_MGR_PORT_FWD_STATE_COUNT ((MC_MGR_PORT_COUNT + 31) / 32)
#define MC_MGR_CALC_PORT_FWD_STATE_COUNT(_x) ((_x + 31) / 32)
#define MC_MGR_PMT_SIZE MC_MGR_PORT_COUNT
struct mc_mgr_dev_ctx_t {
  /* The type of device. */
  bf_dev_family_t dev_family;

  /* Num of sub-devices */
  uint32_t num_subdevices;

  /* Num of max pipes */
  uint32_t num_max_pipes;

  /* Num of max ports */
  uint32_t num_max_ports;

  /* A flag indicating if the device is undergoing hitless HA recovery. */
  bool syncing;

  /* A flag indicating if the device is rebuilding its state into DMA buffers
   * to be pushed to HW. */
  bool rebuilding;

  /* RDM Shadow */
  mc_mgr_rdm_t *rdm_map;

  /* Id generator to get the ECMP ids to write into hardware. */
  bf_id_allocator *ecmp_hw_id_gen;

  /* Id generator to get the ECMP sw_ids. */
  bf_id_allocator *ecmp_sw_id_gen;

  /* Id generator to get the node ids. */
  bf_id_allocator *l1_node_id_gen;

  /* Database of ECMP groups. */
  bf_map_t db_ecmp;
  /* Database of L1 nodes. */
  bf_map_t db_l1;

  /* Non ECMP L1 count */
  uint16_t l1_count[BF_MGID_COUNT];

  /* Lag to pipe bitmap */
  uint16_t lag2pipe_mask[BF_LAG_COUNT];

  /* MGIDs map of reference counts */
  bf_map_t lag2node_map[BF_LAG_COUNT];

  /* Bit map of allocated MGIDs. */
  uint64_t mgid_map[MC_MGR_MGID_MAP_SIZE];

  /* Map of mgid group nodes */
  bf_map_t mgrpinfo_map;

  /* Global RID Shadow */
  uint16_t g_rid;

  /* Shadow of the max L1 and L2 node counts. */
  uint32_t max_l1, max_l2;

  /* Shadow of the max L1 time slice. */
  uint8_t max_l1_slice;

  /* Shadow of the configured CPU port. */
  bf_dev_port_t cpu_port;
  bool cpu_port_en;

  /* PVT Shadow */
  uint32_t pvt_sz;
  mc_mgr_pvt_entry_t *pvt;

  /* TVT Shadow */
  uint32_t tvt_sz;
  mc_mgr_tvt_entry_t *tvt;

  /* SW copy of the tree for each MGID, MIT shadow is derived from this. */
  struct mc_l1_hw_node_t *htrees[MC_MGR_NUM_PIPES][BF_MGID_COUNT];

  /* PMT size */
  uint32_t pmt_size;
  /* PMT Shadow */
  bf_bitset_t pmt[MC_MGR_PMT_SIZE];
  uint64_t pmt_[MC_MGR_PMT_SIZE][BF_BITSET_ARRAY_SIZE(MC_MGR_PMT_SIZE)];

  /* LIT Shadow */
  bf_bitset_t lit[BF_LAG_COUNT][MC_MGR_NUM_PIPES];
  uint64_t lit_[BF_LAG_COUNT][MC_MGR_NUM_PIPES]
               [BF_BITSET_ARRAY_SIZE(BF_PIPE_PORT_COUNT)];
  uint16_t lit_cnt_l[BF_LAG_COUNT];
  uint16_t lit_cnt_r[BF_LAG_COUNT];

  /* Multicast Version Shadow. */
  bool tbl_version;

  /* Port Mask register shadow. */
  uint32_t port_fwd_state[MC_MGR_PORT_FWD_STATE_COUNT];

  /* Shadow of backup port table.  Both array index and contents are bit-index
   * not dev-port. */
  uint16_t bkup_ports[MC_MGR_BPORT_COUNT];
  bool bkup_port_en;

  /* Hardware fast failover (HW detection of port down) enable state. */
  bool ff_en;

  /* Pipe read select (for reading MIT and LIT from HW). */
  uint8_t pipe_rd_sel;

  /* RDM address of the block of L2 nodes used for tails and the set of L1
   * nodes which reference them. */
  struct mc_mgr_tail_info tail_info;

  /*  Buffer Pool */
  mc_mgr_drv_buf_pool_t dma_buffer_pool;

  /* State read back from hardware during a hitless restart. */
  struct mc_mgr_dev_hw_state *ha_state;

  /* Per session write-list.  Holds the write list currently being built up
   * by the client. Once it is pushed to LLD it is removed from this array. */
  mc_mgr_drv_wr_list_t write_list[MC_MGR_NUM_SESSIONS][BF_MAX_SUBDEV_COUNT];

  /* Map an mgid to an array of struct mc_mgr_tree_size_t, one for each pipe.
   * This is accessed by the API thread as well as the DMA completion thread
   * and RDM change thread (assuming those all can happen from different
   * threads). */
  bf_map_t tree_len[BF_MAX_SUBDEV_COUNT];

  /* Holds MGIDs being updated by a session. */
  bf_map_t trees_with_len_updated[BF_MAX_SUBDEV_COUNT][MC_MGR_NUM_PIPES];

  mc_mutex_t tree_len_mtx[BF_MAX_SUBDEV_COUNT];

  /* Per session set of RDM addresses to reclaim once DMA completes. */
  bf_map_t rdm_addrs_to_free[MC_MGR_NUM_SESSIONS];

  /* Ports in this device */
  mc_mgr_port_info_t *port_list;
};

struct mc_mgr_ctx {
  mc_rmutex_t mtx_top;
  /* List of session states. */
  struct mc_session_ctx session_ctx[MC_MGR_NUM_SESSIONS];
  /* Pipe Manager session handle. */
  pipe_sess_hdl_t pipe_mgr_session;
  /* MC Manager internal session handle. */
  bf_mc_session_hdl_t int_sess;
  /* Per device context */
  struct mc_mgr_dev_ctx_t *dev_ctx[MC_MGR_NUM_DEVICES];
  bool dev_locked[MC_MGR_NUM_DEVICES];
};

struct mc_mgr_dev_hw_state {
  mc_mgr_rdm_t *rdm_map;
  uint8_t max_l1_slice;
  bf_dev_port_t cpu_port;
  bool cpu_port_en;
  unsigned int max_l1_nodes;
  unsigned int max_l2_nodes;
  bool ff_en;
  bool bkup_en;
  uint16_t g_rid;
  uint32_t port_fwd_state[MC_MGR_PORT_FWD_STATE_COUNT];
  uint16_t bkup_ports[MC_MGR_BPORT_COUNT];
  uint16_t lit_cnt_l[2][BF_LAG_COUNT];
  uint16_t lit_cnt_r[2][BF_LAG_COUNT];
  bf_bitset_t lit[BF_LAG_COUNT][MC_MGR_NUM_PIPES];
  uint64_t lit_[BF_LAG_COUNT][MC_MGR_NUM_PIPES]
               [BF_BITSET_ARRAY_SIZE(BF_PIPE_PORT_COUNT)];
  bf_bitset_t pmt[MC_MGR_PMT_SIZE];
  uint64_t pmt_[MC_MGR_PMT_SIZE][BF_BITSET_ARRAY_SIZE(MC_MGR_PMT_SIZE)];
  uint32_t mit[MC_MGR_NUM_PIPES][BF_MGID_COUNT];
  bf_map_t mit_dirty[MC_MGR_NUM_PIPES];
  mc_mgr_pvt_entry_t pvt[BF_MGID_COUNT / 8];
  mc_mgr_tvt_entry_t tvt[BF_MGID_COUNT / 4];
  uint32_t c2c_pipe_mask;
  bool tbl_ver;
  struct mc_mgr_tail_info *tail_info;
  bf_map_t rdm_to_node;
  /* List of all L1 nodes read from RDM on a pipe. */
  struct mc_l1_hw_node_t *htrees[MC_MGR_NUM_PIPES][BF_MGID_COUNT];
  bf_map_t ecmp_grps;
  bf_id_allocator *ecmp_hw_id_gen;
  struct mc_l1_node_t *dummy;
  bf_map_t dirty_rdm;

  /* Fields that must be consistent over different pipes include these valid
   * bits to indicate if they are consistent or not. */
  bool max_l1_slice_valid;
  bool cpu_port_valid;
  bool max_l1_nodes_valid;
  bool max_l2_nodes_valid;
  bool tbl_ver_valid;
  bool c2c_pipe_mask_valid;
  /* Fields that must be consistent over versions include these valid bits to
   * indicate if they are consistent or not. */
  bool port_fwd_state_valid;
  bool bkup_ports_valid;
  bool lit_cnt_valid;
  bool lit_valid;
  bool pmt_valid;
  bool pvt_valid;
  bool tvt_valid;
  int dev;
};

extern struct mc_mgr_ctx *mc_mgr_ctx_p;
static inline struct mc_mgr_ctx *mc_mgr_ctx() { return mc_mgr_ctx_p; }
static inline bool mc_mgr_ready() { return !!mc_mgr_ctx(); }
static inline mc_rmutex_t *mc_mgr_ctx_lock() { return &mc_mgr_ctx()->mtx_top; }
static inline struct mc_mgr_dev_ctx_t *mc_mgr_ctx_dev(bf_dev_id_t dev) {
  if (0 > dev || MC_MGR_NUM_DEVICES <= dev) {
    MC_MGR_DBGCHK(0);
    return NULL;
  }
  MC_MGR_DBGCHK(mc_mgr_ctx()->dev_ctx[dev]);
  return mc_mgr_ctx()->dev_ctx[dev];
}
static inline void mc_mgr_ctx_dev_set(bf_dev_id_t dev,
                                      struct mc_mgr_dev_ctx_t *x) {
  if (0 > dev || MC_MGR_NUM_DEVICES <= dev) {
    MC_MGR_DBGCHK(0);
    return;
  }
  mc_mgr_ctx()->dev_ctx[dev] = x;
}
static inline bool mc_mgr_dev_present(bf_dev_id_t dev) {
  if (0 > dev || MC_MGR_NUM_DEVICES <= dev) {
    return false;
  }
  return mc_mgr_ctx() ? !!(mc_mgr_ctx()->dev_ctx[dev]) : false;
}
static inline bf_dev_family_t mc_mgr_ctx_dev_family(bf_dev_id_t dev) {
  return mc_mgr_ctx_dev(dev)->dev_family;
}
static inline uint32_t mc_mgr_ctx_num_subdevices(int dev) {
  return mc_mgr_ctx_dev(dev)->num_subdevices;
}
static inline uint32_t mc_mgr_ctx_num_max_pipes(int dev) {
  return mc_mgr_ctx_dev(dev)->num_max_pipes;
}
static inline uint32_t mc_mgr_ctx_num_max_ports(int dev) {
  return mc_mgr_ctx_dev(dev)->num_max_ports;
}
static inline mc_mgr_rdm_t *mc_mgr_ctx_rdm_map(bf_dev_id_t dev) {
  return mc_mgr_ctx_dev(dev)->rdm_map;
}
static inline bool mc_mgr_ctx_syncing(bf_dev_id_t dev) {
  return mc_mgr_ctx_dev(dev)->syncing;
}
static inline void mc_mgr_ctx_syncing_clr(bf_dev_id_t dev) {
  mc_mgr_ctx_dev(dev)->syncing = false;
}
static inline bool mc_mgr_ctx_rebuilding(bf_dev_id_t dev) {
  return mc_mgr_ctx_dev(dev)->rebuilding;
}
static inline void mc_mgr_ctx_rebuilding_set(bf_dev_id_t dev) {
  mc_mgr_ctx_dev(dev)->rebuilding = true;
}

static inline uint16_t mc_mgr_ctx_lag2pipe_mask(bf_dev_id_t dev, int lag) {
  return mc_mgr_ctx_dev(dev)->lag2pipe_mask[lag];
}

static inline uint16_t mc_mgr_ctx_lag2pipe_mask_set(bf_dev_id_t dev,
                                                    int lag,
                                                    int pm) {
  return mc_mgr_ctx_dev(dev)->lag2pipe_mask[lag] = pm;
}
static inline void mc_mgr_ctx_rebuilding_clr(bf_dev_id_t dev) {
  mc_mgr_ctx_dev(dev)->rebuilding = false;
}
static inline void mc_mgr_ctx_rdm_pending_set(bf_dev_id_t dev, int pipe) {
  rdm_pending_set(mc_mgr_ctx_dev(dev)->rdm_map, pipe);
}
static inline void mc_mgr_ctx_rdm_pending_clr(bf_dev_id_t dev, int pipe) {
  rdm_pending_clr(mc_mgr_ctx_dev(dev)->rdm_map, pipe);
}
static inline bool mc_mgr_ctx_rdm_pending_get(bf_dev_id_t dev, int pipe) {
  return rdm_pending_get(mc_mgr_ctx_dev(dev)->rdm_map, pipe);
}
static inline uint64_t mc_mgr_ctx_mgid_blk(bf_dev_id_t dev, int blk) {
  if (blk < 0 || blk >= MC_MGR_MGID_MAP_SIZE) {
    MC_MGR_DBGCHK(0);
    return 0;
  }
  return mc_mgr_ctx_dev(dev)->mgid_map[blk];
}
static inline void mc_mgr_ctx_mgid_blk_set(bf_dev_id_t dev,
                                           int blk,
                                           uint64_t x) {
  if (blk < 0 || blk >= MC_MGR_MGID_MAP_SIZE) {
    MC_MGR_DBGCHK(0);
    return;
  }
  mc_mgr_ctx_dev(dev)->mgid_map[blk] = x;
}
static inline pipe_sess_hdl_t mc_mgr_ctx_pipe_sess() {
  return mc_mgr_ctx()->pipe_mgr_session;
}
static inline void mc_mgr_ctx_pipe_sess_set(pipe_sess_hdl_t x) {
  mc_mgr_ctx()->pipe_mgr_session = x;
}
static inline bf_mc_session_hdl_t mc_mgr_ctx_int_sess() {
  return mc_mgr_ctx()->int_sess;
}
static inline void mc_mgr_ctx_int_sess_set(bf_mc_session_hdl_t x) {
  mc_mgr_ctx()->int_sess = x;
}
static inline bf_map_t *mc_mgr_ctx_db_l1(bf_dev_id_t dev) {
  return &mc_mgr_ctx_dev(dev)->db_l1;
}
static inline bf_map_t *mc_mgr_ctx_lag_to_node_map(bf_dev_id_t dev,
                                                   int lag_id) {
  return &mc_mgr_ctx_dev(dev)->lag2node_map[lag_id];
}
static inline bf_map_t *mc_mgr_ctx_db_ecmp(bf_dev_id_t dev) {
  return &mc_mgr_ctx_dev(dev)->db_ecmp;
}
static inline bf_map_t *mc_mgr_ctx_mgrp_info_map(bf_dev_id_t dev) {
  return &mc_mgr_ctx_dev(dev)->mgrpinfo_map;
}
static inline bf_id_allocator *mc_mgr_ctx_l1_id_gen(bf_dev_id_t dev) {
  return mc_mgr_ctx_dev(dev)->l1_node_id_gen;
}
static inline bf_id_allocator *mc_mgr_ctx_ecmp_sw_id_gen(bf_dev_id_t dev) {
  return mc_mgr_ctx_dev(dev)->ecmp_sw_id_gen;
}
static inline bf_id_allocator *mc_mgr_ctx_ecmp_hw_id_gen(bf_dev_id_t dev) {
  return mc_mgr_ctx_dev(dev)->ecmp_hw_id_gen;
}
static inline bool mc_mgr_ctx_tbl_ver(bf_dev_id_t dev) {
  return mc_mgr_ctx_dev(dev)->tbl_version;
}
static inline void mc_mgr_ctx_tbl_ver_set(bf_dev_id_t dev, bool ver) {
  mc_mgr_ctx_dev(dev)->tbl_version = ver;
}
static inline uint32_t mc_mgr_ctx_port_fwd_state(bf_dev_id_t dev, int idx) {
  if (idx < 0 || idx >= (int)MC_MGR_CALC_PORT_FWD_STATE_COUNT(
                            mc_mgr_ctx_num_max_ports(dev))) {
    MC_MGR_DBGCHK(0);
    return 0;
  }
  return mc_mgr_ctx_dev(dev)->port_fwd_state[idx];
}
static inline void mc_mgr_ctx_port_fwd_state_set(bf_dev_id_t dev,
                                                 int idx,
                                                 uint32_t x) {
  if (idx < 0 || idx >= (int)MC_MGR_CALC_PORT_FWD_STATE_COUNT(
                            mc_mgr_ctx_num_max_ports(dev))) {
    MC_MGR_DBGCHK(0);
    return;
  }
  mc_mgr_ctx_dev(dev)->port_fwd_state[idx] = x;
}
static inline void mc_mgr_ctx_port_fwd_state_get_one(bf_dev_id_t dev,
                                                     int port_bit_index,
                                                     bool *is_active) {
  if (port_bit_index < 0 || port_bit_index >= MC_MGR_PORT_COUNT) {
    MC_MGR_DBGCHK(0);
    return;
  }
  int idx = port_bit_index / 32;
  int shift = port_bit_index % 32;
  uint32_t mask = 1u << shift;
  /* A bit set in port_fwd_state means forwarding is disabled. */
  *is_active = ~mc_mgr_ctx_dev(dev)->port_fwd_state[idx] & mask;
}
static inline void mc_mgr_ctx_port_fwd_state_set_one(bf_dev_id_t dev,
                                                     int port_bit_index,
                                                     bool x) {
  if (port_bit_index < 0 || port_bit_index >= MC_MGR_PORT_COUNT) {
    MC_MGR_DBGCHK(0);
    return;
  }
  int idx = port_bit_index / 32;
  int shift = port_bit_index % 32;
  uint32_t mask = 1u << shift;
  uint32_t y = mc_mgr_ctx_dev(dev)->port_fwd_state[idx] & ~mask;
  mc_mgr_ctx_dev(dev)->port_fwd_state[idx] = y | ((x ? 1u : 0) << shift);
}
static inline uint16_t mc_mgr_ctx_bkup_port(bf_dev_id_t dev, int bit_idx) {
  if (bit_idx < 0 || bit_idx >= (int)mc_mgr_ctx_num_max_ports(dev)) {
    MC_MGR_DBGCHK(0);
    return 0;
  }
  return mc_mgr_ctx_dev(dev)->bkup_ports[bit_idx];
}
static inline void mc_mgr_ctx_bkup_port_set(bf_dev_id_t dev,
                                            int bit_idx,
                                            uint16_t x) {
  if (bit_idx < 0 || bit_idx >= (int)mc_mgr_ctx_num_max_ports(dev)) {
    MC_MGR_DBGCHK(0);
    return;
  }
  if (x >= mc_mgr_ctx_num_max_ports(dev)) {
    MC_MGR_DBGCHK(0);
    return;
  }
  mc_mgr_ctx_dev(dev)->bkup_ports[bit_idx] = x;
}
static inline bool mc_mgr_ctx_bkup_port_en(bf_dev_id_t dev) {
  return mc_mgr_ctx_dev(dev)->bkup_port_en;
}
static inline void mc_mgr_ctx_bkup_port_en_set(bf_dev_id_t dev, bool x) {
  mc_mgr_ctx_dev(dev)->bkup_port_en = x;
}
static inline bool mc_mgr_ctx_ff_en(bf_dev_id_t dev) {
  return mc_mgr_ctx_dev(dev)->ff_en;
}
static inline void mc_mgr_ctx_ff_en_set(bf_dev_id_t dev, bool x) {
  mc_mgr_ctx_dev(dev)->ff_en = x;
}
static inline uint8_t mc_mgr_ctx_pipe_rd_sel(bf_dev_id_t dev) {
  return mc_mgr_ctx_dev(dev)->pipe_rd_sel;
}
static inline void mc_mgr_ctx_pipe_rd_sel_set(bf_dev_id_t dev, uint8_t x) {
  mc_mgr_ctx_dev(dev)->pipe_rd_sel = x;
}
static inline uint16_t mc_mgr_ctx_glb_rid(bf_dev_id_t dev) {
  return mc_mgr_ctx_dev(dev)->g_rid;
}
static inline void mc_mgr_ctx_glb_rid_set(bf_dev_id_t dev, uint16_t x) {
  mc_mgr_ctx_dev(dev)->g_rid = x;
}
static inline uint32_t mc_mgr_ctx_max_l1(bf_dev_id_t dev) {
  return mc_mgr_ctx_dev(dev)->max_l1;
}
static inline void mc_mgr_ctx_max_l1_set(bf_dev_id_t dev, uint32_t x) {
  mc_mgr_ctx_dev(dev)->max_l1 = x;
}
static inline uint32_t mc_mgr_ctx_max_l2(bf_dev_id_t dev) {
  return mc_mgr_ctx_dev(dev)->max_l2;
}
static inline void mc_mgr_ctx_max_l2_set(bf_dev_id_t dev, uint32_t x) {
  mc_mgr_ctx_dev(dev)->max_l2 = x;
}
static inline uint8_t mc_mgr_ctx_max_l1_ts(bf_dev_id_t dev) {
  return mc_mgr_ctx_dev(dev)->max_l1_slice;
}
static inline void mc_mgr_ctx_max_l1_ts_set(bf_dev_id_t dev, uint8_t x) {
  mc_mgr_ctx_dev(dev)->max_l1_slice = x;
}
static inline uint8_t mc_mgr_ctx_cpu_port_en(bf_dev_id_t dev) {
  return mc_mgr_ctx_dev(dev)->cpu_port_en;
}
static inline void mc_mgr_ctx_cpu_port_en_set(bf_dev_id_t dev, bool x) {
  mc_mgr_ctx_dev(dev)->cpu_port_en = x;
}
static inline uint16_t mc_mgr_ctx_cpu_port(bf_dev_id_t dev) {
  return mc_mgr_ctx_dev(dev)->cpu_port;
}
static inline void mc_mgr_ctx_cpu_port_set(bf_dev_id_t dev, bf_dev_port_t x) {
  mc_mgr_ctx_dev(dev)->cpu_port = x;
}
static inline struct mc_session_ctx *mc_mgr_ctx_session_state(int sid) {
  if (0 > sid || MC_MGR_NUM_SESSIONS <= sid) {
    MC_MGR_DBGCHK(0);
    return NULL;
  }
  return &mc_mgr_ctx()->session_ctx[sid];
}
static inline bool mc_mgr_ctx_in_batch(int sid) {
  struct mc_session_ctx *ctx = mc_mgr_ctx_session_state(sid);
  if (ctx) {
    return ctx->in_batch;
  }
  return false;
}
static inline void mc_mgr_ctx_in_batch_set(int sid, bool x) {
  struct mc_session_ctx *ctx = mc_mgr_ctx_session_state(sid);
  if (ctx) {
    ctx->in_batch = x;
  }
}
static inline bool mc_mgr_ctx_dev_locked(bf_dev_id_t dev) {
  if (0 > dev || MC_MGR_NUM_DEVICES <= dev) {
    MC_MGR_DBGCHK(0);
    return false;
  }
  return mc_mgr_ctx()->dev_locked[dev];
}
static inline void mc_mgr_ctx_dev_locked_set(bf_dev_id_t dev, bool x) {
  if (0 > dev || MC_MGR_NUM_DEVICES <= dev) {
    MC_MGR_DBGCHK(0);
    return;
  }
  mc_mgr_ctx()->dev_locked[dev] = x;
}
static inline struct mc_l1_hw_node_t *mc_mgr_ctx_tree(bf_dev_id_t dev,
                                                      int pipe,
                                                      int mgid) {
  if (pipe < 0 || pipe >= (int)mc_mgr_ctx_num_max_pipes(dev)) {
    MC_MGR_DBGCHK(0);
    return NULL;
  }

  if (mgid < 0 || mgid >= BF_MGID_COUNT) {
    MC_MGR_DBGCHK(0);
    return NULL;
  }
  return mc_mgr_ctx_dev(dev)->htrees[pipe][mgid];
}
static inline struct mc_l1_hw_node_t **mc_mgr_ctx_tree_ref(bf_dev_id_t dev,
                                                           int pipe,
                                                           int mgid) {
  if (pipe < 0 || pipe >= (int)mc_mgr_ctx_num_max_pipes(dev)) {
    MC_MGR_DBGCHK(0);
    return NULL;
  }

  if (mgid < 0 || mgid >= BF_MGID_COUNT) {
    MC_MGR_DBGCHK(0);
    return NULL;
  }
  return &mc_mgr_ctx_dev(dev)->htrees[pipe][mgid];
}
static inline uint32_t mc_mgr_ctx_pmt_size(bf_dev_id_t dev) {
  return mc_mgr_ctx_dev(dev)->pmt_size;
}
static inline bf_bitset_t *mc_mgr_ctx_pmt(bf_dev_id_t dev, int id) {
  if (id < 0 || id >= (int)mc_mgr_ctx_num_max_ports(dev)) {
    MC_MGR_DBGCHK(0);
    return NULL;
  }
  return &mc_mgr_ctx_dev(dev)->pmt[id];
}
static inline mc_mgr_drv_buf_pool_t *mc_mgr_ctx_dma(bf_dev_id_t dev) {
  return &mc_mgr_ctx_dev(dev)->dma_buffer_pool;
}
static inline struct mc_mgr_dev_hw_state **mc_mgr_ctx_ha_state(
    bf_dev_id_t dev) {
  return &mc_mgr_ctx_dev(dev)->ha_state;
}
static inline mc_mgr_drv_wr_list_t *mc_mgr_ctx_wl(bf_dev_id_t dev,
                                                  bf_subdev_id_t subdev,
                                                  int sid) {
  struct mc_session_ctx *ctx = mc_mgr_ctx_session_state(sid);
  if (!ctx || !ctx->valid) {
    MC_MGR_DBGCHK(0);
    return NULL;
  }
  return &mc_mgr_ctx_dev(dev)->write_list[sid][subdev];
}
static inline void mc_mgr_ctx_tree_len_lock(bf_dev_id_t dev,
                                            bf_subdev_id_t subdev) {
  MC_MGR_LOCK(&mc_mgr_ctx_dev(dev)->tree_len_mtx[subdev]);
}
static inline void mc_mgr_ctx_tree_len_unlock(bf_dev_id_t dev,
                                              bf_subdev_id_t subdev) {
  MC_MGR_UNLOCK(&mc_mgr_ctx_dev(dev)->tree_len_mtx[subdev])
}
static inline bf_map_t *mc_mgr_ctx_tree_len(bf_dev_id_t dev,
                                            bf_subdev_id_t subdev) {
  return &mc_mgr_ctx_dev(dev)->tree_len[subdev];
}
static inline bf_status_t mc_mgr_ctx_tree_len_add(
    bf_dev_id_t dev,
    bf_subdev_id_t subdev,
    int mgid,
    struct mc_mgr_tree_size_t *t) {
  mc_mgr_ctx_tree_len_lock(dev, subdev);
  bf_map_sts_t s = bf_map_add(&mc_mgr_ctx_dev(dev)->tree_len[subdev], mgid, t);
  mc_mgr_ctx_tree_len_unlock(dev, subdev);
  if (s != BF_MAP_OK) {
    return BF_INVALID_ARG;
  }
  return BF_SUCCESS;
}
static inline void mc_mgr_ctx_tree_len_rmv(bf_dev_id_t dev,
                                           bf_subdev_id_t subdev,
                                           int mgid) {
  struct mc_mgr_tree_size_t *t = NULL;
  mc_mgr_ctx_tree_len_lock(dev, subdev);
  bf_map_sts_t s =
      bf_map_get_rmv(&mc_mgr_ctx_dev(dev)->tree_len[subdev], mgid, (void *)&t);
  mc_mgr_ctx_tree_len_unlock(dev, subdev);
  if (s != BF_MAP_OK) {
    MC_MGR_DBGCHK(BF_MAP_OK == s);
  } else {
    if (t) MC_MGR_FREE(t);
  }
}
static inline void mc_mgr_ctx_tree_len_rmv_all(bf_dev_id_t dev,
                                               bf_subdev_id_t subdev) {
  struct mc_mgr_tree_size_t *t = NULL;
  unsigned long key;
  bf_map_sts_t s;
  mc_mgr_ctx_tree_len_lock(dev, subdev);
  do {
    s = bf_map_get_first_rmv(
        &mc_mgr_ctx_dev(dev)->tree_len[subdev], &key, (void **)&t);
    if (BF_MAP_OK == s && t) MC_MGR_FREE(t);
  } while (s == BF_MAP_OK);
  mc_mgr_ctx_tree_len_unlock(dev, subdev);
}
static inline bf_map_t *mc_mgr_ctx_trees_with_len_updated(bf_dev_id_t dev,
                                                          bf_subdev_id_t subdev,
                                                          int pipe) {
  if (pipe < 0 || pipe >= (int)mc_mgr_ctx_num_max_pipes(dev)) {
    MC_MGR_DBGCHK(0);
    return NULL;
  }
  return &mc_mgr_ctx_dev(dev)->trees_with_len_updated[subdev][pipe];
}
static inline void mc_mgr_ctx_mark_len_update(
    int sid, bf_dev_id_t dev, bf_subdev_id_t subdev, int pipe, int mgid) {
  (void)sid;
  bf_map_t *m = mc_mgr_ctx_trees_with_len_updated(dev, subdev, pipe);
  bf_map_add(m, mgid, NULL);
  return;
}
static inline void mc_mgr_ctx_clear_len_update(
    int sid, bf_dev_id_t dev, bf_subdev_id_t subdev, int pipe, int mgid) {
  (void)sid;
  bf_map_t *m = mc_mgr_ctx_trees_with_len_updated(dev, subdev, pipe);
  bf_map_rmv(m, mgid);
  return;
}
static inline int mc_mgr_ctx_get_rmv_len_update(int sid,
                                                bf_dev_id_t dev,
                                                bf_subdev_id_t subdev,
                                                int pipe) {
  (void)sid;
  bf_map_t *m = mc_mgr_ctx_trees_with_len_updated(dev, subdev, pipe);
  unsigned long mgid_key = 0;
  void *unused = NULL;
  bf_map_sts_t s = bf_map_get_first_rmv(m, &mgid_key, &unused);
  int mgid = mgid_key;
  return s == BF_MAP_OK ? mgid : -1;
}
static inline bf_map_t *mc_mgr_ctx_rdm_free_addrs(int sid, bf_dev_id_t dev) {
  return &mc_mgr_ctx_dev(dev)->rdm_addrs_to_free[sid];
}
static inline struct mc_mgr_tail_info *mc_mgr_ctx_tail_info(bf_dev_id_t dev) {
  return &mc_mgr_ctx_dev(dev)->tail_info;
}
static inline mc_mgr_pvt_entry_t mc_mgr_ctx_pvt_row(bf_dev_id_t dev, int row) {
  if (row < 0 || row >= (int)mc_mgr_ctx_dev(dev)->pvt_sz) {
    MC_MGR_DBGCHK(0);
    mc_mgr_pvt_entry_t zero = {0};
    return zero;
  }
  return mc_mgr_ctx_dev(dev)->pvt[row];
}
static inline void mc_mgr_ctx_pvt_set(bf_dev_id_t dev,
                                      int row,
                                      mc_mgr_pvt_entry_t x) {
  if (row < 0 || row >= (int)mc_mgr_ctx_dev(dev)->pvt_sz) {
    MC_MGR_DBGCHK(0);
    return;
  }
  mc_mgr_ctx_dev(dev)->pvt[row] = x;
}
static inline uint32_t mc_mgr_ctx_pvt_sz(bf_dev_id_t dev) {
  return mc_mgr_ctx_dev(dev)->pvt_sz;
}
static inline void mc_mgr_ctx_pvt_sz_set(bf_dev_id_t dev, uint64_t x) {
  mc_mgr_ctx_dev(dev)->pvt_sz = x;
}
static inline mc_mgr_tvt_entry_t mc_mgr_ctx_tvt_row(bf_dev_id_t dev, int row) {
  if (row < 0 || row >= (int)mc_mgr_ctx_dev(dev)->tvt_sz) {
    MC_MGR_DBGCHK(0);
    mc_mgr_tvt_entry_t zero = {0};
    return zero;
  }
  return mc_mgr_ctx_dev(dev)->tvt[row];
}
static inline void mc_mgr_ctx_tvt_set(bf_dev_id_t dev,
                                      int row,
                                      mc_mgr_tvt_entry_t x) {
  if (row < 0 || row >= (int)mc_mgr_ctx_dev(dev)->tvt_sz) {
    MC_MGR_DBGCHK(0);
    return;
  }
  mc_mgr_ctx_dev(dev)->tvt[row] = x;
}
static inline uint32_t mc_mgr_ctx_tvt_sz(bf_dev_id_t dev) {
  return mc_mgr_ctx_dev(dev)->tvt_sz;
}
static inline void mc_mgr_ctx_tvt_sz_set(bf_dev_id_t dev, uint64_t x) {
  mc_mgr_ctx_dev(dev)->tvt_sz = x;
}
static inline bf_bitset_t *mc_mgr_ctx_lit(bf_dev_id_t dev, int id, int seg) {
  if (id < 0 || id >= BF_LAG_COUNT) {
    MC_MGR_DBGCHK(0);
    return NULL;
  }

  if (seg < 0 || seg >= MC_MGR_NUM_PIPES) {
    MC_MGR_DBGCHK(0);
    return NULL;
  }
  return &mc_mgr_ctx_dev(dev)->lit[id][seg];
}
static inline uint16_t mc_mgr_ctx_lit_np_l(bf_dev_id_t dev, int lag) {
  if (lag < 0 || lag >= BF_LAG_COUNT) {
    MC_MGR_DBGCHK(0);
    return 0;
  }
  return mc_mgr_ctx_dev(dev)->lit_cnt_l[lag];
}
static inline uint16_t mc_mgr_ctx_lit_np_r(bf_dev_id_t dev, int lag) {
  if (lag < 0 || lag >= BF_LAG_COUNT) {
    MC_MGR_DBGCHK(0);
    return 0;
  }
  return mc_mgr_ctx_dev(dev)->lit_cnt_r[lag];
}
static inline void mc_mgr_ctx_lit_np_l_set(bf_dev_id_t dev,
                                           int lag,
                                           uint16_t x) {
  if (lag < 0 || lag >= BF_LAG_COUNT) {
    MC_MGR_DBGCHK(0);
    return;
  }
  mc_mgr_ctx_dev(dev)->lit_cnt_l[lag] = x;
}
static inline void mc_mgr_ctx_lit_np_r_set(bf_dev_id_t dev,
                                           int lag,
                                           uint16_t x) {
  if (lag < 0 || lag >= BF_LAG_COUNT) {
    MC_MGR_DBGCHK(0);
    return;
  }
  mc_mgr_ctx_dev(dev)->lit_cnt_r[lag] = x;
}

static inline bool mc_dev_port_validate(bf_dev_id_t dev, bf_dev_port_t dp) {
  switch (mc_mgr_ctx_dev(dev)->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return ((dp & 0x7F) < BF_PIPE_PORT_COUNT) && !(dp & ~0x1FF);
    case BF_DEV_FAMILY_TOFINO2:
      return ((dp & 0x7F) < BF_PIPE_PORT_COUNT) && !(dp & ~0x1FF);
    case BF_DEV_FAMILY_TOFINO3:
      return ((dp & 0x7F) < BF_PIPE_PORT_COUNT) && !(dp & ~0x3FF);
    default:
      MC_MGR_DBGCHK(0);
  }
  return false;
}
static inline bf_dev_pipe_t mc_dev_port_to_pipe(bf_dev_id_t dev,
                                                bf_dev_port_t dp) {
  switch (mc_mgr_ctx_dev(dev)->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return (dp >> 7) & 3;
    case BF_DEV_FAMILY_TOFINO2:
      return (dp >> 7) & 3;
    case BF_DEV_FAMILY_TOFINO3:
      return (dp >> 7) & 7;
    default:
      MC_MGR_DBGCHK(0);
  }
  return (bf_dev_pipe_t)~0;
}
static inline bf_dev_pipe_t mc_dev_port_to_local_port(bf_dev_id_t dev,
                                                      bf_dev_port_t dp) {
  switch (mc_mgr_ctx_dev(dev)->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return dp & 0x7F;
    case BF_DEV_FAMILY_TOFINO2:
      return dp & 0x7F;
    case BF_DEV_FAMILY_TOFINO3:
      return dp & 0x7F;
    default:
      MC_MGR_DBGCHK(0);
  }
  return (bf_dev_pipe_t)~0;
}
static inline bf_dev_port_t mc_make_dev_port(bf_dev_id_t dev,
                                             bf_dev_pipe_t pipe,
                                             bf_dev_port_t port) {
  switch (mc_mgr_ctx_dev(dev)->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return (pipe << 7) | port;
    case BF_DEV_FAMILY_TOFINO2:
      return (pipe << 7) | port;
    case BF_DEV_FAMILY_TOFINO3:
      return (pipe << 7) | port;
    default:
      MC_MGR_DBGCHK(0);
  }
  return ~0;
}
static inline int mc_dev_port_to_bit_idx(bf_dev_id_t dev, bf_dev_port_t dp) {
  switch (mc_mgr_ctx_dev(dev)->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return BF_PIPE_PORT_COUNT * mc_dev_port_to_pipe(dev, dp) +
             mc_dev_port_to_local_port(dev, dp);
    case BF_DEV_FAMILY_TOFINO2:
      return BF_PIPE_PORT_COUNT * mc_dev_port_to_pipe(dev, dp) +
             mc_dev_port_to_local_port(dev, dp);
    case BF_DEV_FAMILY_TOFINO3:
      return (BF_PIPE_PORT_COUNT >> 1) * mc_dev_port_to_pipe(dev, dp) +
             mc_dev_port_to_local_port(dev, dp);
    default:
      MC_MGR_DBGCHK(0);
  }
  return ~0;
}
static inline bf_dev_port_t mc_bit_idx_to_dev_port(bf_dev_id_t dev, int i) {
  switch (mc_mgr_ctx_dev(dev)->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return mc_make_dev_port(
          dev, i / BF_PIPE_PORT_COUNT, i % BF_PIPE_PORT_COUNT);
    case BF_DEV_FAMILY_TOFINO2:
      return mc_make_dev_port(
          dev, i / BF_PIPE_PORT_COUNT, i % BF_PIPE_PORT_COUNT);
    case BF_DEV_FAMILY_TOFINO3:
      return mc_make_dev_port(
          dev, i / (BF_PIPE_PORT_COUNT >> 1), i % (BF_PIPE_PORT_COUNT >> 1));
    default:
      MC_MGR_DBGCHK(0);
  }
  return ~0;
}
static inline bf_dev_pipe_t mc_bit_idx_to_pipe(bf_dev_id_t dev, int i) {
  switch (mc_mgr_ctx_dev(dev)->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return mc_dev_port_to_pipe(dev, mc_bit_idx_to_dev_port(dev, i));
    case BF_DEV_FAMILY_TOFINO2:
      return mc_dev_port_to_pipe(dev, mc_bit_idx_to_dev_port(dev, i));
    case BF_DEV_FAMILY_TOFINO3:
      return mc_dev_port_to_pipe(dev, mc_bit_idx_to_dev_port(dev, i));
    default:
      MC_MGR_DBGCHK(0);
  }
  return BF_INVALID_PIPE;
}
static inline bf_dev_pipe_t mc_bit_idx_to_local_port(bf_dev_id_t dev, int i) {
  switch (mc_mgr_ctx_dev(dev)->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return mc_dev_port_to_local_port(dev, mc_bit_idx_to_dev_port(dev, i));
    case BF_DEV_FAMILY_TOFINO2:
      return mc_dev_port_to_local_port(dev, mc_bit_idx_to_dev_port(dev, i));
    case BF_DEV_FAMILY_TOFINO3:
      return mc_dev_port_to_local_port(dev, mc_bit_idx_to_dev_port(dev, i));
    default:
      MC_MGR_DBGCHK(0);
  }
  return BF_INVALID_PIPE;
}
static inline void mc_mgr_mgrp_inc_l1_count(bf_dev_id_t dev,
                                            struct mc_mgr_grp_info *mgrp_info) {
  MC_MGR_DBGCHK(mgrp_info->dev == dev);
  mgrp_info->l1_count++;
  /* Assert to check overflow. */
  MC_MGR_DBGCHK(mgrp_info->l1_count);
}
static inline void mc_mgr_mgrp_dec_l1_count(bf_dev_id_t dev,
                                            struct mc_mgr_grp_info *mgrp_info) {
  MC_MGR_DBGCHK(mgrp_info->dev == dev);
  /* Underflow check */
  MC_MGR_DBGCHK(mgrp_info->l1_count);
  mgrp_info->l1_count--;
}
static inline uint16_t mc_mgr_mgrp_l1_count(bf_dev_id_t dev,
                                            struct mc_mgr_grp_info *mgrp_info) {
  MC_MGR_DBGCHK(mgrp_info->dev == dev);
  return mgrp_info->l1_count;
}
static inline void mc_mgr_mgrp_ecmp_inc_l1_count(
    bf_dev_id_t dev, struct mc_mgr_grp_info *mgrp_info) {
  MC_MGR_DBGCHK(mgrp_info->dev == dev);
  mgrp_info->ecmp_l1_count++;
  /* Assert to check overflow. */
  MC_MGR_DBGCHK(mgrp_info->ecmp_l1_count);
}
static inline void mc_mgr_mgrp_ecmp_dec_l1_count(
    bf_dev_id_t dev, struct mc_mgr_grp_info *mgrp_info) {
  MC_MGR_DBGCHK(mgrp_info->dev == dev);
  /* Underflow check */
  MC_MGR_DBGCHK(mgrp_info->ecmp_l1_count);
  mgrp_info->ecmp_l1_count--;
}
static inline uint16_t mc_mgr_mgrp_ecmp_l1_count(
    bf_dev_id_t dev, struct mc_mgr_grp_info *mgrp_info) {
  MC_MGR_DBGCHK(mgrp_info->dev == dev);
  return mgrp_info->ecmp_l1_count;
}
bool sync_l1_chain(int sid,
                   bf_dev_id_t dev,
                   int pipe,
                   int mgid,
                   struct mc_mgr_dev_hw_state *st,
                   bf_map_t *ecmp_grps,
                   uint32_t l1_addr);
bool mc_mgr_rdm_sync_is_tail_addr(struct mc_mgr_dev_hw_state *st,
                                  int pipe,
                                  uint32_t addr);
bool mc_mgr_rdm_sync_tails(int sid,
                           bf_dev_id_t dev,
                           int pipe,
                           struct mc_mgr_dev_hw_state *st);

ucli_status_t bf_drv_show_tech_ucli_mc__(ucli_context_t *uc);

#define MC_MGR_VALID_DEV(dev)                                     \
  ((MC_MGR_NUM_DEVICES > (dev) && 0 <= (dev) && mc_mgr_ready() && \
    mc_mgr_ctx()->dev_ctx[(dev)]))
#define MC_MGR_INVALID_DEV(dev) (!(MC_MGR_VALID_DEV(dev)))
#define MC_MGR_VALID_SID(sid) ((MC_MGR_NUM_SESSIONS > (sid) && 0 <= (sid)))
#define MC_MGR_INVALID_SID(sid) (!((MC_MGR_NUM_SESSIONS > (sid) && 0 <= (sid))))

#include "mc_mgr_node.h"
#endif /* __MC_MGR_INT_H__ */
