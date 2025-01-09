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


/*!
 * @file pipe_mgr_rmt_cfg.h
 * @date
 *
 * Definitions for Reconfigurable Match Table (RMT) device configuration
 * database.
 */

#ifndef _PIPE_MGR_RMT_CFG_H
#define _PIPE_MGR_RMT_CFG_H

/* Module header files */
#include <dvm/bf_drv_profile.h>
#include <dvm/bf_drv_intf.h>
#include <pipe_mgr/pipe_mgr_config.h>
#include <lld/lld_err.h>
#include <lld/lld_sku.h>
#include <lld/lld_inst_list_fmt.h>

#include <target-utils/hashtbl/bf_hashtbl.h>
#include <target-utils/map/map.h>
#include <bfutils/dynamic_hash/dynamic_hash.h>

#include "pipe_mgr_mutex.h"
#include "pipe_mgr_bitmap.h"

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************
 * RMT device configuration related information structures
 *********************************************************/

typedef uint8_t rmt_tbl_hdl_t;
typedef uint8_t scope_num_t;

/*********************************************************
 * RMT table API related types and constants
 *********************************************************/

#define RMT_MAX_MEM_UNITS_PER_TBL_WORD_BLK 16

/* MAX VPN per table word block is the hardware limit on the number of entries
 * that can be packed in a EXM wide word block. That happens to be 40 on Tofino.
 * There are 8 rows of RAMs, and each RAM can have 5 sub-entries, for a total
 * of 40 packed entries.
 */
#define RMT_MAX_VPN_PER_TBL_WORD_BLK 40

/* Maximum number of synthetic two-port tables for indirect stateful, meter,
 * stats, and selector tables. Limits both the number of memory units in spare
 * banks and the number of ALUs used for these tables.
 */
#define RMT_MAX_S2P_SECTIONS 2

typedef enum pipe_mat_match_type {
  EXACT_MATCH,
  TERNARY_MATCH,
  LONGEST_PREFIX_MATCH,
  ATCAM_MATCH,
  ALPM_MATCH,
} pipe_mat_match_type_t;

typedef enum pipe_stat_type {
  PACKET_COUNT,
  BYTE_COUNT,
  PACKET_AND_BYTE_COUNT
} pipe_stat_type_t;

typedef enum pipe_sel_tbl_mode {
  RESILIENT,
  FAIR,
} pipe_sel_tbl_mode_t;

typedef enum pipe_tbl_ref_type {
  PIPE_TBL_REF_TYPE_INVALID,
  PIPE_TBL_REF_TYPE_DIRECT,
  PIPE_TBL_REF_TYPE_INDIRECT,
} pipe_tbl_ref_type_t;

static inline const char *pipe_mgr_rmt_tbl_ref_type2str(
    pipe_tbl_ref_type_t type) {
  switch (type) {
    case PIPE_TBL_REF_TYPE_INVALID:
      return "Invalid";
    case PIPE_TBL_REF_TYPE_DIRECT:
      return "Direct";
    case PIPE_TBL_REF_TYPE_INDIRECT:
      return "Indirect";
    default:
      return "Unknown";
  }
}

typedef enum rmt_tbl_type {
  RMT_TBL_TYPE_DIRECT_MATCH,
  RMT_TBL_TYPE_HASH_MATCH,
  RMT_TBL_TYPE_TERN_MATCH,
  RMT_TBL_TYPE_TERN_INDIR,
  RMT_TBL_TYPE_ACTION_DATA,
  RMT_TBL_TYPE_STATS,
  RMT_TBL_TYPE_METER,
  RMT_TBL_TYPE_STATEFUL_MEM,
  RMT_TBL_TYPE_IDLE_TMO,
  RMT_TBL_TYPE_SEL_PORT_VEC,
  RMT_TBL_TYPE_PHASE0_MATCH,
  RMT_TBL_TYPE_HASH_ACTION,
  RMT_TBL_TYPE_ATCAM_MATCH,
  RMT_TBL_TYPE_ALPM_MATCH,
  RMT_TBL_TYPE_PROXY_HASH,
  RMT_TBL_TYPE_NO_KEY,
} rmt_tbl_type_t;

/* Table Direction.
 *
 * Internal encoding of the JSON table_direction parameter. The enum was
 * defined after the fact, to replace magic numbers in the context JSON parser.
 *
 * Note that there is code that depends on the specific numeric values, which
 * is why the enum values are defined explicitly.
 *
 * The physical representation is uint8_t or int.
 */
typedef enum {
  BF_TBL_DIR_INGRESS = 0,
  BF_TBL_DIR_EGRESS = 1,
  BF_TBL_DIR_GHOST = 2,
  BF_TBL_DIR_UNKNOWN = 3,
} pipe_tbl_dir_t;

static inline const char *pipe_mgr_rmt_tbl_type2str(rmt_tbl_type_t type) {
  switch (type) {
    case RMT_TBL_TYPE_DIRECT_MATCH:
      return "Direct Lookup";
    case RMT_TBL_TYPE_HASH_MATCH:
      return "Hash Lookup";
    case RMT_TBL_TYPE_TERN_MATCH:
      return "Ternary Lookup";
    case RMT_TBL_TYPE_TERN_INDIR:
      return "Ternary Indirection";
    case RMT_TBL_TYPE_ACTION_DATA:
      return "Action Data";
    case RMT_TBL_TYPE_STATS:
      return "Statistics";
    case RMT_TBL_TYPE_METER:
      return "Meter";
    case RMT_TBL_TYPE_STATEFUL_MEM:
      return "Stateful Memory";
    case RMT_TBL_TYPE_IDLE_TMO:
      return "Idle Timeout";
    case RMT_TBL_TYPE_SEL_PORT_VEC:
      return "Selection Port Vector";
    case RMT_TBL_TYPE_PHASE0_MATCH:
      return "Phase0 Lookup";
    case RMT_TBL_TYPE_HASH_ACTION:
      return "Hash Action";
    case RMT_TBL_TYPE_ATCAM_MATCH:
      return "Algorithmic TCAM Lookup";
    case RMT_TBL_TYPE_ALPM_MATCH:
      return "Algorithmic ALPM Lookup";
    case RMT_TBL_TYPE_PROXY_HASH:
      return "Proxy hash exact match lookup";
    case RMT_TBL_TYPE_NO_KEY:
      return "No Key";
    default:
      return "bad";
  }
}

static inline bool pipe_mgr_rmt_tbl_type_is_s2p(rmt_tbl_type_t type) {
  return type == RMT_TBL_TYPE_STATS || type == RMT_TBL_TYPE_METER ||
         type == RMT_TBL_TYPE_STATEFUL_MEM || type == RMT_TBL_TYPE_SEL_PORT_VEC;
}

typedef enum rmt_mem_type {
  RMT_MEM_SRAM,
  RMT_MEM_TCAM,
  RMT_MEM_MAP_RAM,
  RMT_MEM_INGRESS_BUFFER,




} rmt_mem_type_t;

typedef uint16_t mem_id_t; /* Physical mem unit id */
typedef uint16_t vpn_id_t; /* Virtual page number */
typedef uint32_t rmt_ram_line_t[4];
typedef uint32_t rmt_virt_addr_t;

/* Port specific info */
typedef struct rmt_port_info {
  struct rmt_port_info *next;
  struct rmt_port_info *prev;
  bf_dev_port_t port_id;
  bf_dev_pipe_t phy_pipe;
  bf_port_speeds_t speed; /* 1G, 10G, 25G, 40G, 100G, etc */

  /* Set by bf_pipe_enable_ibuf_congestion_notif_to_parser. */
  uint16_t lo_wm_bytes;
  uint16_t hi_wm_bytes;
  bool use_custom_wm_bytes;

  bool disable_cong_notif;
  bool pause_mac;
  bool high_pri; /* bf_pipe_enable_port_arb_priority_high */

  uint8_t iprsr_pri_thresh;

  /* The following are used to detect port stuck condition */
  uint64_t count;
  uint64_t bypass_count;
} rmt_port_info_t;

/* RMT stage-related configuration */
typedef struct rmt_stage_cfg {
  uint8_t num_logical_tables;
  uint8_t sram_unit_width;  /* in bits */
  uint16_t sram_unit_depth; /* in <sram_unit_wide> words */
  uint8_t num_sram_units;
  uint8_t num_sram_rows;
  uint8_t num_sram_cols;

  uint8_t tcam_unit_width;  /* in bits */
  uint16_t tcam_unit_depth; /* in <tcam_unit_wide> words */
  uint8_t num_tcam_units;
  uint8_t num_tcam_rows;
  uint8_t num_tcam_cols;

  uint8_t map_ram_unit_width;  /* in bits */
  uint16_t map_ram_unit_depth; /* in <map_ram_unit_wide> words */
  uint8_t num_map_ram_units;
  uint8_t num_map_ram_rows;
  uint8_t num_map_ram_cols;
  uint8_t num_meter_alus;












} rmt_stage_cfg_t;

/* RMT device-related configuration */
typedef struct rmt_dev_cfg {
  uint8_t num_pipelines;     /* Number of pipelines on device */
  uint8_t num_prsrs;         /* Number of parsers in a pipeline */
  uint8_t num_stages;        /* Number of stages in a pipeline */
  uint8_t num_ports;         /* Number of ports in a pipeline. */
  uint8_t num_mac_blks;      /* Number of MAC blocks on device */
  uint8_t num_chan_per_port; /* Number of channels per port */
  uint8_t p0_width;          /* Width of the phase0 data in 32 bit chunks. */
  uint8_t num_dflt_reg;      /* Number of default registers */
  rmt_stage_cfg_t stage_cfg;


  uint8_t num_gress;            /*Number of non overlapped gress*/
  /* Function pointers */
  bool (*is_pipe_addr)(uint64_t address);
  bool (*is_pcie_pipe_addr)(uint32_t address);
  uint8_t (*pipe_id_from_addr)(uint64_t address);
  uint8_t (*stage_id_from_addr)(uint64_t address);
  uint8_t (*row_from_addr)(uint64_t address);
  uint8_t (*col_from_addr)(uint64_t address);
  pipe_ring_addr_type_e (*addr_type_from_addr)(uint64_t address);
  pipe_mem_type_t (*mem_type_from_addr)(uint64_t address);
  uint16_t (*mem_addr_from_addr)(uint64_t address);
  mem_id_t (*mem_id_from_addr)(uint64_t address);
  uint8_t (*mem_id_to_row)(mem_id_t mem_id, pipe_mem_type_t mem_type);
  uint8_t (*mem_id_to_col)(mem_id_t mem_id, pipe_mem_type_t mem_type);
  mem_id_t (*mem_id_from_col_row)(dev_stage_t stage,
                                  uint8_t col,
                                  uint8_t row,
                                  pipe_mem_type_t mem_type);
  mem_id_t (*map_ram_from_unit_ram)(mem_id_t unit_ram_mem_id);
  mem_id_t (*unit_ram_from_map_ram)(mem_id_t map_ram_mem_id);
  void (*mem_id_to_dimensions)(
      pipe_mem_type_t mem_type, mem_id_t mem_id, int *d0, int *d1, int *d2);
  uint64_t (*set_pipe_id_in_addr)(uint64_t address, uint8_t pipe_id);
  uint64_t (*get_full_phy_addr)(uint8_t gress,
                                uint8_t pipe_id,
                                uint8_t stage_id,
                                mem_id_t mem_id,
                                uint32_t mem_address,
                                pipe_mem_type_t mem_type);
  uint64_t (*get_relative_phy_addr)(mem_id_t mem_id,
                                    uint32_t mem_address,
                                    pipe_mem_type_t mem_type);
  uint32_t (*dir_addr_set_pipe_type)(uint32_t addr);
  uint32_t (*dir_addr_set_pipe_id)(uint32_t addr, uint32_t pipe_id);
  uint32_t (*dir_addr_get_pipe_id)(uint32_t addr);
  uint32_t (*pcie_pipe_addr_set_stage)(uint32_t addr, dev_stage_t stage);
  uint32_t (*pcie_pipe_addr_get_stage)(uint32_t addr);
  uint64_t (*pcie_pipe_addr_to_full_addr)(uint32_t addr);
  bool (*sram_row_valid)(uint8_t row);
  bool (*sram_col_valid)(uint8_t col);
  bool (*tcam_row_valid)(uint8_t row);
  bool (*tcam_col_valid)(uint8_t col);


  bf_dev_pipe_t (*dev_port_to_pipe)(bf_dev_port_t port);
  bf_dev_port_t (*dev_port_to_local_port)(bf_dev_port_t port);
  bf_dev_port_t (*make_dev_port)(bf_dev_pipe_t pipe, int port);
  bool (*dev_port_validate)(bf_dev_port_t port);
  int (*dev_port_to_bit_idx)(bf_dev_port_t dev_port);
  bf_dev_port_t (*bit_idx_to_dev_port)(int bit_idx);
  pipe_tbl_dir_t (*gress_from_addr)(uint64_t address);
} rmt_dev_cfg_t;

/******************************************************
 * RMT resource mapping related information structures
 ******************************************************/

typedef struct rmt_mem_pack_format {
  uint8_t mem_word_width;
  uint16_t tbl_word_width;
  uint8_t entries_per_tbl_word;
  uint8_t mem_units_per_tbl_word;
} rmt_mem_pack_format_t;

typedef enum rmt_tbl_hash_fn {
  RMT_TBL_HASH_FN_A = 0,
  RMT_TBL_HASH_FN_B = 1
} rmt_tbl_hash_fn_t;

typedef struct rmt_tbl_hash_sel_bits {
  rmt_tbl_hash_fn_t function;
  uint8_t ram_unit_select_bit_lo;
  uint8_t ram_unit_select_bit_hi;
  uint8_t ram_line_select_bit_lo;
  uint8_t ram_line_select_bit_hi;
  uint8_t num_ram_line_bits;
  uint8_t num_ram_select_bits;
  uint8_t num_subword_bits;
} rmt_tbl_hash_sel_bits_t;

typedef struct rmt_tbl_word_blk {
  mem_id_t mem_id[RMT_MAX_MEM_UNITS_PER_TBL_WORD_BLK];
  bool mrd_terminate[RMT_MAX_MEM_UNITS_PER_TBL_WORD_BLK];
  vpn_id_t vpn_id[RMT_MAX_VPN_PER_TBL_WORD_BLK];
} rmt_tbl_word_blk_t;

typedef struct rmt_tbl_bank_map {
  uint16_t num_tbl_word_blks;        /* how many tbl_word_blk */
  rmt_tbl_word_blk_t *tbl_word_blk;  /* array of tbl_word_blk */
  rmt_tbl_hash_sel_bits_t hash_bits; /* for hash tables only */
} rmt_tbl_bank_map_t;

typedef struct rmt_idle_time_tbl_params_ {
  uint8_t bit_width;
  bool notify_disable;
  bool two_way_notify_enable;
  bool per_flow_enable;
} rmt_idle_time_tbl_params_t;

typedef struct rmt_meter_tbl_params_ {
  /* Number of default lower huffman bits provided by the compiler */
  uint8_t default_lower_huffman_bits;
  /* The number of ALUs allocated for the meter table. */
  uint8_t num_alu_indexes;
  /* Which ALUs are allocated for the meter table. */
  uint8_t alu_indexes[RMT_MAX_S2P_SECTIONS];
} rmt_meter_tbl_params_t;

typedef struct rmt_sel_tbl_params_ {
  /* The number of ALUs allocated for the select table. */
  uint8_t num_alu_indexes;
  /* Which ALUs are allocated for the select table. */
  uint8_t alu_indexes[RMT_MAX_S2P_SECTIONS];
} rmt_sel_tbl_params_t;

typedef struct rmt_stful_tbl_params_ {
  /* The number of ALUs allocated for the stage table. */
  uint8_t num_alu_indexes;
  /* Which ALUs are allocated for the stage table. */
  uint8_t alu_indexes[RMT_MAX_S2P_SECTIONS];
} rmt_stful_tbl_params_t;

typedef union rmt_tbl_params_ {
  rmt_idle_time_tbl_params_t idle;
  rmt_meter_tbl_params_t meter_params;
  rmt_sel_tbl_params_t sel_params;
  rmt_stful_tbl_params_t stful_params;
} rmt_tbl_params_t;

typedef struct rmt_tbl_stash_entry_info_ {
  uint32_t stash_id;
  uint8_t stash_match_data_select;
  uint8_t stash_hashbank_select;
  uint8_t stash_match_result_bus_select;
  uint8_t hash_function_id;
} rmt_tbl_stash_entry_info_t;

typedef struct rmt_tbl_stash_ {
  rmt_mem_pack_format_t pack_format;
  uint32_t num_stash_entries;
  uint32_t number_memory_units_per_table_word;
  rmt_tbl_stash_entry_info_t **stash_entries;
} rmt_tbl_stash_t;

typedef struct rmt_tbl_info {
  // RMT table type.
  rmt_tbl_type_t type;
  // Traffic direction: 0-ingress, 1-egress, 2-ghost.
  uint8_t direction;
  // MAU stage identifier.
  uint8_t stage_id;
  // Logical table handle. Not available in all tables.
  // Please check LLIR.
  rmt_tbl_hdl_t handle;

  uint32_t num_entries;
  rmt_mem_pack_format_t pack_format;
  // RMT memory type.
  rmt_mem_type_t mem_type;

  uint8_t num_spare_rams;
  mem_id_t spare_rams[RMT_MAX_S2P_SECTIONS];
  // Number of banks. Will be 1 for all tables except hash.
  uint8_t num_tbl_banks;
  // Array of bank memory maps.
  rmt_tbl_bank_map_t *bank_map;
  // Array of bank memory maps allocated for color memory.
  // Only applicable for meter tables.
  rmt_tbl_bank_map_t *color_bank_map;
  rmt_tbl_params_t params;
  rmt_tbl_stash_t *stash;
  // Match ram action if ternary indirection stage table (TIND)
  // is not present.
  pipe_act_fn_hdl_t tind_act_fn_hdl;
} rmt_tbl_info_t;

typedef enum pipe_color_mapram_address_type_ {
  COLOR_MAPRAM_ADDR_TYPE_INVALID = 0,
  COLOR_MAPRAM_ADDR_TYPE_IDLE,
  COLOR_MAPRAM_ADDR_TYPE_STATS,
} pipe_color_mapram_address_type_e;

typedef struct pipe_tbl_ref {
  pipe_tbl_hdl_t tbl_hdl;
  pipe_tbl_ref_type_t ref_type;
  pipe_color_mapram_address_type_e color_mapram_addr_type;
} pipe_tbl_ref_t;

typedef struct alpm_field_info {
  uint32_t byte_offset;
  uint32_t bit_width;
  uint32_t num_slices;
  uint32_t *slice_offset;
  uint32_t *slice_width;
} alpm_field_info_t;

typedef struct alpm_mat_tbl_info {
  pipe_mat_tbl_hdl_t preclass_handle;
  pipe_mat_tbl_hdl_t atcam_handle;
  uint32_t partition_depth;
  uint32_t max_subtrees_per_partition;

  /* Preclassifier spec info */
  pipe_act_fn_hdl_t *act_fn_hdls;
  uint8_t num_act_fn_hdls;
  uint16_t partition_idx_start_bit;
  uint16_t partition_idx_field_width;

  /* Trie spec info */
  uint32_t num_fields;
  alpm_field_info_t *field_info;
  uint32_t num_excluded_bits;

  /* ALPM key width optimization related info */
  uint32_t lpm_field_key_width;
  uint32_t atcam_subset_key_width;
  uint32_t shift_granularity;
} alpm_mat_tbl_info_t;

typedef struct pipe_partition_idx_info {
  char *partition_field_name;
  uint32_t partition_idx_start_bit;
  uint32_t partition_idx_field_width;
} pipe_partition_idx_info_t;

typedef struct pipe_mgr_field_info {
  char *name;
  uint8_t match_type;
  uint8_t *value;
  uint8_t *mask;
  uint32_t bit_width;
  uint32_t start_bit;
  uint32_t start_offset;
} pipe_mgr_field_info_t;

typedef struct pipe_mgr_ind_res_info_t {
  pipe_tbl_hdl_t handle;
  uint32_t idx;
} pipe_mgr_ind_res_info_t;

typedef struct pipe_mgr_dir_res_info_t {
  pipe_tbl_hdl_t handle;
} pipe_mgr_dir_res_info_t;

typedef struct pipe_mgr_action_entry {
  char *name;
  pipe_act_fn_hdl_t act_fn_hdl;
  uint32_t num_act_data;
  uint8_t num_ind_res;
  uint8_t num_dir_res;
  pipe_mgr_field_info_t *act_data;
  pipe_mgr_ind_res_info_t *ind_res;
  pipe_mgr_dir_res_info_t *dir_res;
} pipe_mgr_action_entry_t;

typedef struct pipe_mgr_static_entry_info {
  pipe_mgr_action_entry_t action_entry;
  uint32_t priority;
  bool default_entry;
  int len_bytes;
  int len_bits;
  uint8_t *key;
  uint8_t *msk;
} pipe_mgr_static_entry_info_t;

typedef struct hash_action_tbl_info {
  pipe_mgr_field_info_t *match_param_list;
  uint32_t num_match_params;
} hash_action_tbl_info_t;

typedef struct pipe_act_fn_info_ {
  pipe_act_fn_hdl_t act_fn_hdl;
  uint32_t num_bits;
  uint32_t num_bytes;
  /* If the action uses direct resources the dir_*_hdl fields will be populated
   * with the table handle of the direct resource table. */
  pipe_tbl_hdl_t dir_stat_hdl;
  pipe_tbl_hdl_t dir_meter_hdl;
  pipe_tbl_hdl_t dir_stful_hdl;
} pipe_act_fn_info_t;

typedef struct pipe_keyless_info {
  uint32_t stage_id;
  uint32_t log_tbl_id;

} pipe_keyless_info_t;

typedef struct pipe_def_ind_res {
  pipe_tbl_hdl_t res_tbl_hdl;
  uint32_t res_tbl_idx;
} pipe_def_ind_res_t;

typedef struct pipe_default_info {
  bool p4_default;
  bool is_const;
  pipe_mgr_action_entry_t action_entry;
} pipe_default_info_t;

/* Match-action table information. */
typedef struct pipe_mat_tbl_info {
  const char *name;
  pipe_tbl_dir_t direction;
  pipe_mat_tbl_hdl_t handle;
  profile_id_t profile_id;
  pipe_mat_match_type_t match_type;
  bool uses_range;
  bool disable_atomic_modify;
  // Whether exm table allows run-time key mask.
  bool dynamic_key_mask_table;
  uint16_t match_key_width;  // Unused
  uint32_t size;
  uint32_t num_partitions;
  /* Contains partition idx info if ATCAM */
  pipe_partition_idx_info_t *partition_idx_info;
  uint32_t num_stages;
  uint32_t num_rmt_info;
  uint32_t num_adt_tbl_refs;
  uint32_t num_sel_tbl_refs;
  uint32_t num_stat_tbl_refs;
  uint32_t num_meter_tbl_refs;
  uint32_t num_stful_tbl_refs;
  uint32_t num_static_entries;
  pipe_mgr_static_entry_info_t *static_entries;
  /* Hash-action info */
  hash_action_tbl_info_t *hash_action_info;
  /* Keyless info */
  pipe_keyless_info_t *keyless_info;
  pipe_default_info_t *default_info;
  uint16_t match_key_mask_width;  // width in bytes
  // Key mask (byte array) for all match fields of an exact match table.
  // Format and endianness of match_key_mask are the same as match-spec;
  // hence when new entry is added to table, match_key_mask can be applied
  // byte by byte to match-spec.
  uint8_t *match_key_mask;
  rmt_tbl_info_t *rmt_info;       /* Array of mapped RMT tables */
  alpm_mat_tbl_info_t *alpm_info; /* Reference to alpm data */
  pipe_tbl_ref_t *adt_tbl_ref;    /* Reference to action data table */
  pipe_tbl_ref_t *sel_tbl_ref;    /* Reference to selection table */
  pipe_tbl_ref_t *stat_tbl_ref;   /* Reference to statistic table */
  pipe_tbl_ref_t *meter_tbl_ref;  /* Reference to meter table */
  pipe_tbl_ref_t *stful_tbl_ref;  /* Reference to stateful table */
  // Parser instance handle. Only used by phase0 table.
  pipe_prsr_instance_hdl_t prsr_instance_hdl;
  // Array of action functions that use hash distribution or RNG.
  // These cannot act as default actions.
  uint32_t def_act_blacklist_size;
  pipe_act_fn_hdl_t *def_act_blacklist;
  // Array of hashtables keyed by match-spec, used for duplicate entry detection
  // and access by match spec. ONE, if the table is symmetric, as many as the
  // number of pipe-lines in which the table is present if not.
  bf_hashtable_t **key_htbl;
  // Array of hashtables keyed by the match spec which records the operations
  // done on the above two hash tables for transaction semantics.
  bf_hashtable_t **txn_log_htbl;
  // Whether duplicate entry detection is enabled.
  bool duplicate_entry_check;
  bool symmetric;
  uint32_t scope_value;
  // Number of match bits for this table.
  uint32_t num_match_bits;
  // Number of match bytes for this table.
  uint32_t num_match_bytes;
  uint32_t num_actions;
  pipe_act_fn_info_t *act_fn_hdl_info;
  void *ha_move_list;
  // Whether the default entry for pure keyless table is valid.
  bool tbl_no_key_is_default_entry_valid;
  // The next fields are used to store a mask for a table key fields that
  // have a mask defined in P4 program.
  uint8_t *tbl_global_key_mask_bits; /* Pointer to table  mask */
  uint16_t tbl_global_key_mask_len;  /* Length of the mask in bytes */
  bool tbl_global_key_mask_valid; /* If true, the mask was set and is valid */
} pipe_mat_tbl_info_t;

typedef struct pipe_adt_tbl_info {
  const char *name;
  uint8_t direction;
  pipe_adt_tbl_hdl_t handle;
  pipe_tbl_ref_type_t ref_type;
  uint32_t size;
  uint32_t num_stages;
  uint32_t num_rmt_info;
  rmt_tbl_info_t *rmt_info; /* Array of mapped RMT tables */
  uint32_t num_actions;
  pipe_act_fn_info_t *act_fn_hdl_info;
  bool symmetric;
  void *ha_move_list;
} pipe_adt_tbl_info_t;

typedef struct pipe_stat_tbl_info {
  const char *name;
  pipe_tbl_dir_t direction;
  pipe_stat_tbl_hdl_t handle;
  pipe_stat_type_t stat_type;
  uint32_t size;
  pipe_tbl_ref_type_t ref_type; /* How this table is referenced */
  uint32_t num_rmt_info;
  rmt_tbl_info_t *rmt_info; /* Array of mapped RMT tables */
  /* Resolution of the byte counter */
  uint32_t byte_counter_resolution;
  /* Resolution of the packet counter */
  uint32_t packet_counter_resolution;
  bool enable_per_flow_enable;
  uint32_t per_flow_enable_bit_position;
  /* A flag to indicate if LR(t) is enabled */
  bool lrt_enabled;
  bool symmetric;
} pipe_stat_tbl_info_t;

/* Enum definition for meter implementation type */
typedef enum pipe_meter_type_ {
  PIPE_METER_TYPE_STANDARD = 0,
  PIPE_METER_TYPE_LPF = 1,
  PIPE_METER_TYPE_WRED = 2,
} pipe_meter_impl_type_e;

/* Enum definition for meter granularity.
 * Applicable only for standard color-based meters.
 */
typedef enum pipe_meter_granularity_ {
  PIPE_METER_GRANULARITY_INVALID = 0,
  PIPE_METER_GRANULARITY_BYTES = 1,
  PIPE_METER_GRANULARITY_PACKETS = 2,
} pipe_meter_granularity_e;

typedef struct pipe_meter_tbl_info {
  const char *name;
  pipe_tbl_dir_t direction;
  pipe_meter_tbl_hdl_t handle;
  pipe_meter_impl_type_e meter_type;
  pipe_meter_granularity_e meter_granularity;
  uint32_t size;
  uint32_t num_rmt_info;
  rmt_tbl_info_t *rmt_info; /* Array of mapped RMT tables */
  /* Reference type from the match table to the meter table */
  pipe_tbl_ref_type_t ref_type;
  /* A flag indicating if the meter is color-aware capable */
  bool enable_color_aware;
  /* A flag indicating if per-flow is enabled on the meter table */
  bool enable_per_flow_enable;
  /* Bit position of the per-flow enable in the meter address */
  uint32_t per_flow_enable_bit_position;
  /* A flag indicating if per-flow color aware is enabled */
  bool enable_color_aware_per_flow_enable;
  /* Bit position of the per-flow color aware enable in the meter address.
   * Valid only if the above flag is set.
   */
  uint32_t color_aware_per_flow_enable_address_type_bit_position;
  bool symmetric;
  uint64_t max_rate;       /* The max cir/pir that can be configured */
  uint64_t max_burst_size; /* The max pburst/cburst that can be configured */
} pipe_meter_tbl_info_t;

typedef struct pipe_stful_action_instr_t {
  pipe_act_fn_hdl_t act_hdl;
  int instr_number;
} pipe_stful_action_instr_t;

typedef struct pipe_stful_register_param_t {
  char *name;
  int reg_file_index;
  int64_t init_value;
  /* Register params has its own handle */
  pipe_tbl_hdl_t handle;
} pipe_stful_register_param_t;

typedef struct pipe_stful_tbl_info {
  const char *name;
  pipe_tbl_dir_t direction;
  pipe_stful_tbl_hdl_t handle;
  pipe_tbl_ref_type_t ref_type;
  uint32_t size;
  bool dbl_width;
  int width;
  int set_instr_at;
  int set_instr;
  int clr_instr_at;
  int clr_instr;
  int cntr_index;
  bool is_fifo;
  bool fifo_can_cpu_push;
  bool fifo_can_cpu_pop;
  uint32_t initial_val_lo;
  uint32_t initial_val_hi;
  pipe_sel_tbl_hdl_t sel_tbl_hdl;
  uint32_t num_actions;
  pipe_stful_action_instr_t *actions;
  uint32_t num_reg_params;
  pipe_stful_register_param_t *reg_params;
  uint32_t num_rmt_info;
  rmt_tbl_info_t *rmt_info; /* Array of mapped RMT tables */
  bool symmetric;
  uint8_t alu_idx;
} pipe_stful_tbl_info_t;

/* Similar structure to that of galois_field_matrix_delta_t */
typedef struct pipe_dhash_crossbar_data {
  /* Total bytes pairs for tofino
   * are 16 * 4 = 64. Because total bits in ixbar
   * are 16 * 72 and each byte pair = 18 bits
   */
  uint32_t byte_pair_index;

  uint8_t byte0;
  uint8_t valid0;
  uint8_t byte1;
  uint8_t valid1;
} pipe_dhash_crossbar_data_t;

typedef struct pipe_dhash_field {
  char *name;
  uint32_t start_bit;
  uint32_t bit_width;
  bool optional;
} pipe_dhash_field_t;

typedef struct pipe_dhash_crossbar {
  char *name;
  uint32_t byte_number;
  uint32_t bit_in_byte;
  uint32_t field_bit;
} pipe_dhash_crossbar_t;

typedef struct pipe_dhash_crossbar_config {
  uint32_t stage_id;
  uint32_t num_crossbars;
  pipe_dhash_crossbar_t *crossbars;
  ixbar_init_t ixbar_init;
  uint32_t num_crossbar_mods;
  pipe_dhash_crossbar_t *crossbar_mods;
  ixbar_init_t ixbar_mod_init;
} pipe_dhash_crossbar_config_t;

typedef struct pipe_dhash_field_list {
  char *name;
  pipe_hash_calc_hdl_t handle;
  bool can_permute;
  bool can_rotate;
  uint32_t num_fields;
  pipe_dhash_field_t *fields;
  uint32_t num_crossbar_configs;
  pipe_dhash_crossbar_config_t *crossbar_configs;
} pipe_dhash_field_list_t;

typedef struct pipe_dhash_alg {
  char *name;
  pipe_hash_calc_hdl_t handle;
  bfn_hash_algorithm_t hash_alg;
} pipe_dhash_alg_t;

typedef struct pipe_dhash_hash {
  uint32_t hash_id;
  uint32_t num_hash_bits;
  hash_matrix_output_t *hash_bits;
} pipe_dhash_hash_t;

typedef struct pipe_dhash_hash_config {
  uint32_t stage_id;
  pipe_dhash_hash_t hash;
  pipe_dhash_hash_t hash_mod;
} pipe_dhash_hash_config_t;

typedef struct pipe_dhash_info {
  char *name;
  pipe_hash_calc_hdl_t handle;
  pipe_fld_lst_hdl_t def_field_list_hdl;
  pipe_fld_lst_hdl_t curr_field_list_hdl;
  pipe_hash_alg_hdl_t def_algo_hdl;
  pipe_hash_alg_hdl_t curr_algo_hdl;
  pipe_hash_seed_t def_seed_value;
  pipe_hash_seed_t curr_seed_value;
  uint64_t rotate;
  uint32_t num_curr_fields;
  pipe_hash_calc_input_field_attribute_t *curr_field_attrs;
  uint32_t num_field_lists;
  pipe_dhash_field_list_t *field_lists;
  bool any_hash_algorithm_allowed;
  uint32_t hash_bit_width;
  uint32_t num_algorithms;
  pipe_dhash_alg_t *algorithms;
  uint32_t num_hash_configs;
  pipe_dhash_hash_config_t *hash_configs;
  bf_dev_id_t dev_id;
  pipe_bitmap_t pipe_bmp;
} pipe_dhash_info_t;

typedef struct pipe_select_tbl_info {
  const char *name;
  pipe_tbl_dir_t direction;
  pipe_sel_tbl_hdl_t handle;
  pipe_sel_tbl_mode_t mode;
  pipe_adt_tbl_hdl_t adt_tbl_hdl;
  pipe_stful_tbl_hdl_t stful_tbl_hdl;
  pipe_tbl_ref_type_t ref_type; /* How is this selection table referred */
  uint32_t max_group_size;
  uint32_t max_groups;  // Not used
  uint32_t max_mbrs;    /* max_members across all groups */
  uint32_t num_stages;
  uint32_t num_rmt_info;
  rmt_tbl_info_t *rmt_info; /* Array of mapped RMT tables */
  bool symmetric;
  bool sps_enable;
  void *ha_move_list;
} pipe_select_tbl_info_t;

/* Table configurations of an RMT device */
typedef struct rmt_dev_tbl_info {
  uint8_t num_mat_tbls;
  uint8_t num_adt_tbls;
  uint8_t num_stat_tbls;
  uint8_t num_meter_tbls;
  uint8_t num_sful_tbls;
  uint8_t num_select_tbls;
  pipe_mat_tbl_info_t *mat_tbl_list;       /* array */
  pipe_adt_tbl_info_t *adt_tbl_list;       /* array */
  pipe_stat_tbl_info_t *stat_tbl_list;     /* array */
  pipe_meter_tbl_info_t *meter_tbl_list;   /* array */
  pipe_stful_tbl_info_t *stful_tbl_list;   /* array */
  pipe_select_tbl_info_t *select_tbl_list; /* array */
} rmt_dev_tbl_info_t;

typedef enum prsr_multi_threading_mode {
  PRSR_MULTI_THREADING_MODE_DEFAULT = 0,
  PRSR_MULTI_THREADING_MODE_DISABLE = 1,
  PRSR_MULTI_THREADING_MODE_ENABLE = 2
} prsr_multi_threading_mode_t;

typedef struct pipe_driver_options_ {
  bool hash_parity_enabled;
  prsr_multi_threading_mode_t prsr_multi_threading_enable;
} pipe_driver_options_t;

#define PIPE_MGR_PROG_NAME_LEN 256

/* Profile of a device. */
typedef struct rmt_dev_profile_info {
  profile_id_t profile_id;
  char prog_name[PIPE_MGR_PROG_NAME_LEN];
  char pipeline_name[PIPE_MGR_PROG_NAME_LEN];
  char cfg_file[PIPE_MGR_CFG_FILE_LEN];

  /* Bitmap of Logical pipe ids. */
  pipe_bitmap_t pipe_bmp;
  /* List of tables on the device. */
  rmt_dev_tbl_info_t tbl_info_list;
  /* Maps stage+dir to stage characteristics (match dependency and clock
   * cycles). Not all chips have this info.
   * Tofino does not have, Tof2 has. */
  bf_map_t stage_characteristics;
  /* A set of shared (between compiler and driver) config values on the device.
   * The keys are of type enum pipe_config_cache_key and the values are one of
   * the pipe_config_cache_X structs depending on the key.  */
  bf_map_t config_cache;
  bf_map_t dhash_info;                  /* Dynamic Hash calculations */
  pipe_driver_options_t driver_options; /* Driver options */
  bool uses_bfrt;                       /* uses BF-RT json */
  bf_dev_pipe_t lowest_pipe;            /* lowest pipe of this profile */
  int num_stages;     /* Number of stages the profile bin file configured. */
  uint64_t pps_limit; /* Compiler specified PPS cap. */
  /* Compiler version [major, minor, maintenance] */
  int compiler_version[3];
  /* Schema version [major, minor, maintenance] */
  int schema_version[3];
} rmt_dev_profile_info_t;

enum pipe_config_cache_key {
  pipe_cck_meta_opt_ctrl,
  pipe_cck_parser_multi_threading,
  pipe_cck_meter_sweep_ctrl,
  pipe_cck_meter_ctrl,
  pipe_cck_xbar_din_power_ctrl,
  pipe_cck_hash_seed,
  pipe_cck_hash_parity_group_mask,
  pipe_cck_mau_stage_ext
};

struct pipe_config_cache_reg_t {
  uint32_t val;
};

struct pipe_config_cache_meter_sweep_t {
  uint32_t **val; /* Per stage, per ALU. */
};

struct pipe_config_cache_meter_ctl_t {
  uint32_t **val; /* Per stage, per ALU. */
};

struct pipe_config_cache_2d_byte_array_t {
  uint8_t **val;
  int size1;
  int size2;
};

struct pipe_config_cache_bypass_stage_reg_t {
  /* An array of stage offsets (pcie address would be stage base plus offset)
   * to write the configuration at. Note that this is a base address, so a
   * wide register or register array would have num_vals > 1 and values would
   * be written starting at the offset. */
  int num_offsets;
  uint32_t *offset;

  /* Three mask values:
   *  0 - Old-Last. This is for programming the last stage set by the bin file.
   *  1 - Intermediate. This is for programming all stages after Old-Last up
   *      to Real-Last.
   *  2 - Real-Last. This is for programming the actual last stage on the chip.
   * If the mask value is zero then the register is not programmed.
   * If the mask is non-zero, the register is written with the value bitwise
   * ANDed with the mask. */
  uint32_t mask[3];

  /* An array of values to write. If more than one, it is either a wide
   * register or a register array, and the values should be written at
   * consecutive addresses starting at the offset. */
  int num_vals;
  uint32_t *vals;
};

struct pipe_config_cache_bypass_stage_t {
  /* Last stage programmed by the tofino.bin file (as specified in the
   * context.json). */
  int last_stage_programmed;
  /* Length of the regs array. */
  int reg_cnt;
  struct pipe_config_cache_bypass_stage_reg_t *regs;
};

/* Device level data structure */
/* RMT device specific info */
typedef struct rmt_dev_info {
  bf_dev_id_t dev_id;         /* Unique identifier */
  bf_dev_type_t dev_type;     /* Device type */
  bf_dev_family_t dev_family; /* Device family */
  bool virtual_device;        /* True when virtual device */
  bool virtual_dev_slave;     /* True when device is a Virtual device slave,
                               * Cannot be set for a virtual device.  */
  rmt_dev_cfg_t dev_cfg;      /* Static configuration */
  rmt_port_info_t *port_list; /* ports in this device */
  bf_dev_pipe_t *pipe_log_phy_map;       /* logical to physical pipe mapping */
  bf_dev_pipe_t *pipe_phy_log_map;       /* physical to logical pipe mapping */
  uint32_t num_pipeline_profiles;        /* No of profiles */
  rmt_dev_profile_info_t **profile_info; /* profile info */
  uint32_t num_active_pipes;             /* No of active pipes on this device */
  uint8_t num_active_mau; /* No of active MAU for each logical pipe on this
                             device */
  uint32_t num_active_subdevices; /* Active sub-devices */
  uint64_t clock_speed;           /* pps, Clock frequency in Hz */
  uint64_t bps_clock_speed;       /* bps, Clock frequency in Hz. TOF:=pps */
  uint64_t sp_clock_speed; /* idletime and meter use only, Clock frequency in
                              Hz. TOF:=pps */
  bool fake_rmt_cfg;       /* set to true during unit test */
  bf_sku_chip_part_rev_t part_rev; /* Part revision A0/B0 ... */

  pipe_mgr_mutex_t mau_scratch_mtx;
  uint32_t **mau_scratch_val; /* [# pipes][# stages] Scratch register in MAU */


  int coal_mirror_enable;
  int coal_sessions_num;
  int coal_min;
} rmt_dev_info_t;

typedef struct pipe_mgr_blob_dnld_params {
  bf_dev_id_t dev_id;
  profile_id_t prof_id;
  char cfg_file[PIPE_MGR_CFG_FILE_LEN];
} pipe_mgr_blob_dnld_params_t;

extern bf_sys_timer_t port_stuck_detect_timers[PIPE_MGR_NUM_DEVICES];

/* Function prototypes */
pipe_status_t pipe_mgr_add_rmt_device(pipe_sess_hdl_t sess_hdl,
                                      bf_dev_id_t dev_id,
                                      bool virtual_device,
                                      bool virtual_device_slave,
                                      bf_dev_type_t dev_type,
                                      bf_device_profile_t *profile,
                                      bf_dev_init_mode_t dev_init_mode);

pipe_status_t pipe_mgr_remove_rmt_device(bf_dev_id_t dev_id);
bf_status_t pipe_mgr_warm_init_quick(bf_dev_id_t dev_id);

pipe_status_t pipe_mgr_add_rmt_port(bf_dev_id_t dev_id,
                                    bf_dev_port_t port_id,
                                    bf_port_speed_t speed);

pipe_status_t pipe_mgr_remove_rmt_port(bf_dev_id_t dev_id, uint16_t port_id);

pipe_mat_tbl_info_t *pipe_mgr_get_tbl_info(bf_dev_id_t dev_id,
                                           pipe_mat_tbl_hdl_t h,
                                           const char *where,
                                           const int line);

pipe_adt_tbl_info_t *pipe_mgr_get_adt_tbl_info(bf_dev_id_t dev_id,
                                               pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                               const char *where,
                                               const int line);

pipe_select_tbl_info_t *pipe_mgr_get_select_tbl_info(bf_dev_id_t dev_id,
                                                     pipe_sel_tbl_hdl_t h);

pipe_stat_tbl_info_t *pipe_mgr_get_stat_tbl_info(
    bf_dev_id_t dev_id,
    pipe_stat_tbl_hdl_t stat_tbl_hdl,
    const char *where,
    const int line);

pipe_stful_tbl_info_t *pipe_mgr_get_stful_tbl_info(
    bf_dev_id_t dev_id,
    pipe_stful_tbl_hdl_t stful_tbl_hdl,
    const char *where,
    const int line);

pipe_meter_tbl_info_t *pipe_mgr_get_meter_tbl_info(
    bf_dev_id_t dev_id,
    pipe_meter_tbl_hdl_t meter_tbl_hdl,
    const char *where,
    const int line);

const char *pipe_mgr_get_tbl_name(bf_dev_id_t dev_id,
                                  pipe_tbl_hdl_t handle,
                                  const char *where,
                                  const int line);

pipe_dhash_info_t *pipe_mgr_get_hash_calc_info(bf_dev_id_t dev_id,
                                               pipe_hash_calc_hdl_t h);

void pipe_mgr_rmt_dynamic_selector_free(bf_map_t *dhash_info);

bool pipe_mgr_mat_tbl_has_idle(bf_dev_id_t dev_id, pipe_mat_tbl_hdl_t tbl_hdl);

bool pipe_mgr_get_meter_impl_type(pipe_res_hdl_t tbl_hdl,
                                  bf_dev_id_t device_id,
                                  pipe_meter_impl_type_e *meter_type);

bool pipe_mgr_tbl_ref_is_direct(bf_dev_id_t dev_id,
                                pipe_mat_tbl_hdl_t h,
                                pipe_mat_tbl_hdl_t res,
                                const char *where,
                                const int line);

bool pipe_mgr_mat_tbl_is_hash_action(bf_dev_id_t dev_id,
                                     pipe_mat_tbl_hdl_t tbl_hdl);

/* Accessor routines */
uint64_t pipe_mgr_get_clock_speed(bf_dev_id_t dev_id);

uint64_t pipe_mgr_get_bps_clock_speed(bf_dev_id_t dev_id);

uint64_t pipe_mgr_get_sp_clock_speed(bf_dev_id_t dev_id);

uint32_t pipe_mgr_nsec_to_clock(bf_dev_id_t dev, uint32_t ns);

uint32_t pipe_mgr_clock_to_nsec(bf_dev_id_t dev, uint32_t clock);

uint64_t pipe_mgr_usec_to_clock(bf_dev_id_t dev, uint32_t us);

uint32_t pipe_mgr_clock_to_usec(bf_dev_id_t dev, uint64_t clock);

uint64_t pipe_mgr_usec_to_bps_clock(bf_dev_id_t dev, uint32_t us);

uint8_t pipe_mgr_get_num_ram_select_bits(rmt_tbl_info_t *rmt_tbl_info,
                                         uint32_t hash_way_idx);

uint8_t pipe_mgr_get_ram_select_offset(rmt_tbl_info_t *rmt_tbl_info,
                                       uint32_t hash_way_idx);

uint16_t pipe_mgr_get_tcam_unit_depth(bf_dev_id_t dev_id);

uint16_t pipe_mgr_get_sram_unit_depth(bf_dev_id_t dev_id);

uint16_t pipe_mgr_get_mapram_unit_depth(bf_dev_id_t dev_id);



uint16_t pipe_mgr_get_mem_unit_depth(bf_dev_id_t dev_id,
                                     rmt_mem_type_t mem_type);

rmt_port_info_t *pipe_mgr_get_port_info(bf_dev_id_t dev_id, uint16_t port_id);

bool pipe_mgr_is_device_present(bf_dev_id_t dev_id);
bool pipe_mgr_is_device_virtual(bf_dev_id_t dev_id);
bool pipe_mgr_is_device_virtual_dev_slave(bf_dev_id_t dev_id);
void pipe_mgr_tstamp_init(const rmt_dev_info_t *dev_info);
bool pipe_mgr_mat_tbl_uses_only_tcam(bf_dev_id_t dev_id, pipe_mat_tbl_hdl_t h);
bool pipe_mgr_mat_tbl_is_no_key(bf_dev_id_t dev_id, pipe_mat_tbl_hdl_t h);
bool pipe_mgr_mat_tbl_is_phase0(bf_dev_id_t dev_id, pipe_mat_tbl_hdl_t h);
bool pipe_mgr_mat_tbl_uses_tcam(bf_dev_id_t dev_id, pipe_mat_tbl_hdl_t h);
bool pipe_mgr_mat_tbl_uses_alpm(bf_dev_id_t dev_id, pipe_mat_tbl_hdl_t h);

bf_dev_id_t pipe_mgr_transform_dev_id_for_hitless_ha(bf_dev_id_t device_id);

rmt_dev_profile_info_t *pipe_mgr_get_profile_info(bf_dev_id_t dev_id,
                                                  profile_id_t profile_id,
                                                  const char *where,
                                                  const int line);

rmt_dev_profile_info_t *pipe_mgr_get_profile(const rmt_dev_info_t *dev_info,
                                             profile_id_t profile_id,
                                             const char *where,
                                             const int line);

pipe_status_t pipe_mgr_get_pipe_bmp_for_profile(const rmt_dev_info_t *dev_info,
                                                profile_id_t profile_id,
                                                pipe_bitmap_t *pipe_bmp_p,
                                                const char *where,
                                                const int line);
bool pipe_mgr_is_p4_skipped(const rmt_dev_info_t *dev_info);

pipe_status_t pipe_mgr_pipe_to_profile(const rmt_dev_info_t *dev_info,
                                       bf_dev_pipe_t pipe_id,
                                       profile_id_t *profile_id,
                                       const char *where,
                                       const int line);

profile_id_t pipe_mgr_get_tbl_profile(bf_dev_id_t dev_id,
                                      pipe_mat_tbl_hdl_t h,
                                      const char *where,
                                      const int line);

pipe_status_t pipe_mgr_download_blob_to_asic(
    pipe_sess_hdl_t sess_hdl,
    rmt_dev_info_t *dev_info,
    pipe_mgr_blob_dnld_params_t *param);

pipe_status_t pipe_mgr_map_pipe_id_log_to_phy(const rmt_dev_info_t *dev_info,
                                              bf_dev_pipe_t pipe_id,
                                              bf_dev_pipe_t *phy_pipe_id);

pipe_status_t pipe_mgr_map_phy_pipe_id_to_log_pipe_id(
    bf_dev_id_t dev_id, bf_dev_pipe_t pipe_id, bf_dev_pipe_t *log_pipe_id);

pipe_status_t pipe_mgr_map_phy_pipe_id_to_log_pipe_id_optimized(
    const rmt_dev_info_t *dev_info,
    bf_dev_pipe_t pipe_id,
    bf_dev_pipe_t *log_pipe_id);

uint32_t pipe_mgr_get_num_active_pipes(bf_dev_id_t dev_id);
uint32_t pipe_mgr_get_num_active_subdevices(bf_dev_id_t dev_id);

size_t p4_fake_lrn_cfg_type_sz(uint8_t lrn_cfg_type);

uint8_t p4_fake_fld_lst_hdl_to_lq_cfg_type(
    pipe_fld_lst_hdl_t flow_lrn_fld_lst_hdl);

pipe_status_t p4_fake_lrn_decode(uint8_t pipe,
                                 uint8_t learn_cfg_type,
                                 uint8_t lq_data[48],
                                 void *lrn_digest_entry,
                                 uint32_t index);

pipe_status_t pipe_mgr_rmt_tbl_type_to_mem_type(rmt_tbl_type_t type,
                                                pipe_mem_type_t *mem_type);

/** \brief pipe_mgr_get_phy_addr_for_vpn_addr
 *         Convert a virtual address to an array of physical addresses.
 *
 * NOTE: The virt_addr passed should not have any huffman bits.
 *
 * \param in dev_tgt The device target
 * \param in tbl_hdl The table handle (match, action, selection etc)
 * \param in stage_id The stage id
 * \param in check_stage_table_handle Match the stage table handle
 * \param in stage_table_handle The stage table handle
 * \param in rmt_tbl_type The table type
 * \param in virt_addr The virtual address (without any huffman bits)
 * \param out phy_addr Array to hold the physical addresses.
 *                     The array should be of size
 *                     RMT_MAX_MEM_UNITS_PER_TBL_WORD_BLK.
 * \param out addr_count The count of valid addresses in the phy_addr array
 * \param out subword_pos The position of the subword
 *
 * \return The status of the operation.
 *         PIPE_OBJ_NOT_FOUND if the virtual address is not valid
 *         PIPE_SUCCESS in case of success
 */
pipe_status_t pipe_mgr_get_phy_addr_for_vpn_addr(dev_target_t dev_tgt,
                                                 pipe_tbl_hdl_t tbl_hdl,
                                                 int stage_id,
                                                 bool check_stage_table_handle,
                                                 uint8_t stage_table_handle,
                                                 rmt_tbl_type_t rmt_tbl_type,
                                                 rmt_virt_addr_t virt_addr,
                                                 uint64_t *phy_addr,
                                                 int *addr_count,
                                                 int *subword_pos);

int pipe_mgr_tbl_hdl_set_pipe(bf_dev_id_t dev_id,
                              profile_id_t profile_id,
                              pipe_tbl_hdl_t handle,
                              pipe_tbl_hdl_t *ret_handle);

/**
 * The function is used to initialize the timer used to detect port stuck
 *condition
 *
 * @param  dev                  ASIC device identifier
 * @return                      Status of the API call
 */
pipe_status_t pipe_mgr_port_stuck_detect_timer_init(bf_dev_id_t dev,
                                                    uint32_t timer_msec);

/**
 * The function is used to stop and delete the timer used to detect port stuck
 *condition
 *
 * @param  dev                  ASIC device identifier
 * @return                      Status of the API call
 */
pipe_status_t pipe_mgr_port_stuck_detect_timer_cleanup(bf_dev_id_t dev);

#ifdef __cplusplus
}
#endif /* C++ */

#endif /* _PIPE_MGR_RMT_CFG_H */
