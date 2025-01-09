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


#ifndef _PIPE_MGR_DB_H
#define _PIPE_MGR_DB_H

#define PIPE_MGR_MAX_MEM_ID 256
#define PIPE_MGR_MAX_HDL_PER_MEM_ID 4
#define PIPE_DIR_MAX 2

/* Imem db data */
typedef struct pipe_imem_data_db_ {
  uint8_t *data;
  int data_len;
  uint32_t base_addr;
} pipe_imem_data_db_t;

enum pipe_mgr_tof_imem_idx {
  PIPE_MGR_TOF_IMEM32 = 0,
  PIPE_MGR_TOF_IMEM16 = 1,
  PIPE_MGR_TOF_IMEM8 = 2,
  PIPE_MGR_TOF_IMEM_COUNT = 3
};

struct pipe_mgr_tof_imem_shadow_db {
  pipe_imem_data_db_t imem[PIPE_MGR_TOF_IMEM_COUNT];
};

enum pipe_mgr_tof2_imem_idx {
  PIPE_MGR_TOF2_IMEM32 = 0,
  PIPE_MGR_TOF2_IMEM32_DARK = 1,
  PIPE_MGR_TOF2_IMEM32_MOCHA = 2,
  PIPE_MGR_TOF2_IMEM16 = 3,
  PIPE_MGR_TOF2_IMEM16_DARK = 4,
  PIPE_MGR_TOF2_IMEM16_MOCHA = 5,
  PIPE_MGR_TOF2_IMEM8 = 6,
  PIPE_MGR_TOF2_IMEM8_DARK = 7,
  PIPE_MGR_TOF2_IMEM8_MOCHA = 8,
  PIPE_MGR_TOF2_IMEM_COUNT = 9
};
static inline const char *pipe_mgr_tof2_imem_name(
    enum pipe_mgr_tof2_imem_idx t) {
  switch (t) {
    case PIPE_MGR_TOF2_IMEM32:
      return "Imem32";
    case PIPE_MGR_TOF2_IMEM32_DARK:
      return "Imem32Dark";
    case PIPE_MGR_TOF2_IMEM32_MOCHA:
      return "Imem32Mocha";
    case PIPE_MGR_TOF2_IMEM16:
      return "Imem16";
    case PIPE_MGR_TOF2_IMEM16_DARK:
      return "Imem16Dark";
    case PIPE_MGR_TOF2_IMEM16_MOCHA:
      return "Imem16Mocha";
    case PIPE_MGR_TOF2_IMEM8:
      return "Imem8";
    case PIPE_MGR_TOF2_IMEM8_DARK:
      return "Imem8Dark";
    case PIPE_MGR_TOF2_IMEM8_MOCHA:
      return "Imem8Mocha";
    case PIPE_MGR_TOF2_IMEM_COUNT:
      return "ImemCount";
  }
  return "Unknown";
}

struct pipe_mgr_tof2_imem_shadow_db {
  pipe_imem_data_db_t imem[PIPE_MGR_TOF2_IMEM_COUNT];
};

enum pipe_mgr_tof3_imem_idx {
  PIPE_MGR_TOF3_IMEM32 = 0,
  PIPE_MGR_TOF3_IMEM32_DARK = 1,
  PIPE_MGR_TOF3_IMEM32_MOCHA = 2,
  PIPE_MGR_TOF3_IMEM16 = 3,
  PIPE_MGR_TOF3_IMEM16_DARK = 4,
  PIPE_MGR_TOF3_IMEM16_MOCHA = 5,
  PIPE_MGR_TOF3_IMEM8 = 6,
  PIPE_MGR_TOF3_IMEM8_DARK = 7,
  PIPE_MGR_TOF3_IMEM8_MOCHA = 8,
  PIPE_MGR_TOF3_IMEM_COUNT = 9
};

struct pipe_mgr_tof3_imem_shadow_db {
  pipe_imem_data_db_t imem[PIPE_MGR_TOF3_IMEM_COUNT];
};

union pipe_imem_shadow_db {
  struct pipe_mgr_tof_imem_shadow_db tof;
  struct pipe_mgr_tof2_imem_shadow_db tof2;
  struct pipe_mgr_tof3_imem_shadow_db tof3;
};

#define PIPE_MGR_TOF_HASH_INPUT_PARITY_GROUPS 16
#define PIPE_MGR_TOF_HASH_BYTEPAIRS_PER_INPUT_PARITY_GROUP 4

#define PIPE_MGR_TOF2_HASH_INPUT_PARITY_GROUPS \
  PIPE_MGR_TOF_HASH_INPUT_PARITY_GROUPS
#define PIPE_MGR_TOF2_HASH_BYTEPAIRS_PER_INPUT_PARITY_GROUP \
  PIPE_MGR_TOF_HASH_BYTEPAIRS_PER_INPUT_PARITY_GROUP

#define PIPE_MGR_TOF_MAX_GFM_ROWS 64
#define PIPE_MGR_TOF_MAX_GFM_COLS 64

#define PIPE_MGR_TOF2_MAX_GFM_ROWS PIPE_MGR_TOF_MAX_GFM_ROWS
#define PIPE_MGR_TOF2_MAX_GFM_COLS PIPE_MGR_TOF_MAX_GFM_COLS

typedef struct pipe_mgr_tof_gfm_shadow_db_ {
  uint32_t base_addr;
  int data_len;
  uint32_t data[PIPE_MGR_TOF_MAX_GFM_ROWS][PIPE_MGR_TOF_MAX_GFM_COLS];
} pipe_mgr_tof_gfm_shadow_db_t;

typedef struct pipe_mgr_tof2_gfm_shadow_db_ {
  uint32_t base_addr;
  int data_len;
  uint32_t data[PIPE_MGR_TOF2_MAX_GFM_ROWS][PIPE_MGR_TOF2_MAX_GFM_COLS];
} pipe_mgr_tof2_gfm_shadow_db_t;

#define PIPE_MGR_TOF3_MAX_GFM_ROWS PIPE_MGR_TOF_MAX_GFM_ROWS
#define PIPE_MGR_TOF3_MAX_GFM_COLS PIPE_MGR_TOF_MAX_GFM_COLS
typedef struct pipe_mgr_tof3_gfm_shadow_db_ {
  uint32_t base_addr;
  int data_len;
  uint32_t data[PIPE_MGR_TOF3_MAX_GFM_ROWS][PIPE_MGR_TOF3_MAX_GFM_COLS];
} pipe_mgr_tof3_gfm_shadow_db_t;

union pipe_gfm_shadow_db {
  pipe_mgr_tof_gfm_shadow_db_t tof;
  pipe_mgr_tof2_gfm_shadow_db_t tof2;
  pipe_mgr_tof3_gfm_shadow_db_t tof3;
};

/* Map Ram info details */
typedef struct pipe_map_ram_info_details_ {
  // Table handle.
  pipe_tbl_hdl_t hdl;
  // RMT table type.
  rmt_tbl_type_t type;
} pipe_map_ram_info_details_t;

/* Map Ram info */
typedef struct pipe_map_ram_info_ {
  pipe_map_ram_info_details_t info[PIPE_MGR_MAX_MEM_ID];
} pipe_map_ram_info_t;

/* tbl hdl info details */
typedef struct pipe_intr_tbl_hdl_details_ {
  // Table handle.
  pipe_tbl_hdl_t hdl;
  // RMT table type.
  rmt_tbl_type_t tbl_type;
  // Memory type.
  pipe_mem_type_t mem_type;
  // TCAM base address (only if tbl_type is tcam).
  uint64_t tcam_base_addr;
} pipe_intr_tbl_hdl_details_t;

/* tbl hdl info */
typedef struct pipe_tbl_hdl_info_ {
  pipe_intr_tbl_hdl_details_t info[PIPE_MGR_MAX_MEM_ID]
                                  [PIPE_MGR_MAX_HDL_PER_MEM_ID];
} pipe_tbl_hdl_info_t;

#define PIPE_MGR_TOF_TCAM_WORD_WIDTH 16
#define PIPE_MGR_TOF_PO_WORD_WIDTH 32
#define PIPE_MGR_TOF2_TCAM_WORD_WIDTH 16
#define PIPE_MGR_TOF2_PO_WORD_WIDTH 64
#define PIPE_MGR_TOF3_TCAM_WORD_WIDTH 16
#define PIPE_MGR_TOF3_PO_WORD_WIDTH 64

/* Tofino parser bin shadow DB. */
struct pipe_mgr_tof_prsr_bin_config {
  uint8_t po_action_data[TOF_PARSER_DEPTH][PIPE_MGR_TOF_PO_WORD_WIDTH];
  uint8_t ea_row_data[TOF_PARSER_DEPTH][PIPE_MGR_TOF_TCAM_WORD_WIDTH];
  uint8_t ctr_init_ram_data[TOF_PARSER_INIT_RAM_DEPTH]
                           [PIPE_MGR_TOF_TCAM_WORD_WIDTH];
  uint8_t po_csum_ctr0_data[TOF_PARSER_CSUM_DEPTH]
                           [PIPE_MGR_TOF_TCAM_WORD_WIDTH];
  uint8_t po_csum_ctr1_data[TOF_PARSER_CSUM_DEPTH]
                           [PIPE_MGR_TOF_TCAM_WORD_WIDTH];
  uint32_t prsr_reg_data[TOF_PRSR_REG_DEPTH];
};

/* Tofino2 parser bin shadow DB. */
struct pipe_mgr_tof2_prsr_bin_config {
  uint8_t po_action_data[TOF2_PARSER_DEPTH][PIPE_MGR_TOF2_PO_WORD_WIDTH];
  // uint8_t tcam_data[TOF2_PARSER_DEPTH][PIPE_MGR_TOF2_TCAM_WORD_WIDTH];
  uint8_t ea_row_data[TOF2_PARSER_DEPTH][PIPE_MGR_TOF2_TCAM_WORD_WIDTH];
  uint8_t ctr_init_ram_data[TOF2_PARSER_INIT_RAM_DEPTH]
                           [PIPE_MGR_TOF2_TCAM_WORD_WIDTH];
  uint8_t po_csum_ctr0_data[TOF2_PARSER_CSUM_DEPTH]
                           [PIPE_MGR_TOF2_TCAM_WORD_WIDTH];
  uint8_t po_csum_ctr1_data[TOF2_PARSER_CSUM_DEPTH]
                           [PIPE_MGR_TOF2_TCAM_WORD_WIDTH];
  uint8_t po_csum_ctr2_data[TOF2_PARSER_CSUM_DEPTH]
                           [PIPE_MGR_TOF2_TCAM_WORD_WIDTH];
  uint8_t po_csum_ctr3_data[TOF2_PARSER_CSUM_DEPTH]
                           [PIPE_MGR_TOF2_TCAM_WORD_WIDTH];
  uint8_t po_csum_ctr4_data[TOF2_PARSER_CSUM_DEPTH]
                           [PIPE_MGR_TOF2_TCAM_WORD_WIDTH];
  // Add registers required for tof2 multi-parser here.
};
struct pipe_mgr_tof3_prsr_bin_config {
  uint8_t po_action_data[TOF3_PARSER_DEPTH][PIPE_MGR_TOF3_PO_WORD_WIDTH];
  // uint8_t tcam_data[TOF3_PARSER_DEPTH][PIPE_MGR_TOF3_TCAM_WORD_WIDTH];
  uint8_t ea_row_data[TOF3_PARSER_DEPTH][PIPE_MGR_TOF3_TCAM_WORD_WIDTH];
  uint8_t ctr_init_ram_data[TOF3_PARSER_INIT_RAM_DEPTH]
                           [PIPE_MGR_TOF3_TCAM_WORD_WIDTH];
  uint8_t po_csum_ctr0_data[TOF3_PARSER_CSUM_DEPTH]
                           [PIPE_MGR_TOF3_TCAM_WORD_WIDTH];
  uint8_t po_csum_ctr1_data[TOF3_PARSER_CSUM_DEPTH]
                           [PIPE_MGR_TOF3_TCAM_WORD_WIDTH];
  uint8_t po_csum_ctr2_data[TOF3_PARSER_CSUM_DEPTH]
                           [PIPE_MGR_TOF3_TCAM_WORD_WIDTH];
  uint8_t po_csum_ctr3_data[TOF3_PARSER_CSUM_DEPTH]
                           [PIPE_MGR_TOF3_TCAM_WORD_WIDTH];
  uint8_t po_csum_ctr4_data[TOF3_PARSER_CSUM_DEPTH]
                           [PIPE_MGR_TOF3_TCAM_WORD_WIDTH];
  // FIXME for reg
};

/* Parser bin configuration. Indexed by bin_hdl. */
union pipe_parser_bin_config_t {
  struct pipe_mgr_tof_prsr_bin_config tof;
  struct pipe_mgr_tof2_prsr_bin_config tof2;
  struct pipe_mgr_tof3_prsr_bin_config tof3;
};

struct pipe_mgr_tof_prsr_base_addr {
  // memory
  uint64_t po_action_addr;
  uint64_t word0_addr;
  uint64_t word1_addr;
  uint64_t ea_row_addr;
  uint64_t ctr_init_ram_addr;
  uint64_t po_csum_ctr0_addr;
  uint64_t po_csum_ctr1_addr;
  uint64_t prsr_step;
  // register
  uint32_t prsr_reg_addr[TOF_PRSR_REG_DEPTH];
  uint32_t prsr_reg_step;
};

struct pipe_mgr_tof2_prsr_base_addr {
  // memory
  uint64_t po_action_addr;
  uint64_t tcam_addr;
  uint64_t ea_row_addr;
  uint64_t ctr_init_ram_addr;
  uint64_t po_csum_ctr0_addr;
  uint64_t po_csum_ctr1_addr;
  uint64_t po_csum_ctr2_addr;
  uint64_t po_csum_ctr3_addr;
  uint64_t po_csum_ctr4_addr;
  uint64_t prsr_step;
  // register
  // Add registers required for tof-2 multi parser here
};

struct pipe_mgr_tof3_prsr_base_addr {
  // memory
  uint64_t po_action_addr;
  uint64_t tcam_addr;
  uint64_t ea_row_addr;
  uint64_t ctr_init_ram_addr;
  uint64_t po_csum_ctr0_addr;
  uint64_t po_csum_ctr1_addr;
  uint64_t po_csum_ctr2_addr;
  uint64_t po_csum_ctr3_addr;
  uint64_t po_csum_ctr4_addr;
  uint64_t prsr_step;
  // register
  // FIXME for reg
};

union pipe_parser_base_addr_t {
  struct pipe_mgr_tof_prsr_base_addr tof;
  struct pipe_mgr_tof2_prsr_base_addr tof2;
  struct pipe_mgr_tof3_prsr_base_addr tof3;
};

#define PRSR_INSTANCE_NAME_MAX_LEN 128

/* prsr_instance_hdl+log_pipe_id=>prsrs map, pvs/phase0_hdl, bin_hdl */
struct pipe_mgr_prsr_instance_t {
  uint64_t prsr_map_default;
  uint64_t prsr_map;  // only variable can be changed during run-time
  uint32_t *pvs_hdl;
  uint32_t pvs_hdl_numb;
  uint32_t phase0_hdl;
  char *name;  // prsr instance name
  union pipe_parser_bin_config_t bin_cfg;
};

struct pipe_mgr_prsr_tcam_tof_shadow {
  uint8_t word0_data[TOF_PARSER_DEPTH][PIPE_MGR_TOF_TCAM_WORD_WIDTH];
  uint8_t word1_data[TOF_PARSER_DEPTH][PIPE_MGR_TOF_TCAM_WORD_WIDTH];
};

struct pipe_mgr_prsr_tcam_tof2_shadow {
  uint8_t tcam_data[TOF2_PARSER_DEPTH][PIPE_MGR_TOF2_TCAM_WORD_WIDTH];
};
struct pipe_mgr_prsr_tcam_tof3_shadow {
  uint8_t tcam_data[TOF3_PARSER_DEPTH][PIPE_MGR_TOF3_TCAM_WORD_WIDTH];
};
/* Parser tcam shadow DB. */
typedef union pipe_mgr_prsrtcam_shadow_db_ {
  struct pipe_mgr_prsr_tcam_tof_shadow tof[PIPE_DIR_MAX];
  struct pipe_mgr_prsr_tcam_tof2_shadow tof2[PIPE_DIR_MAX];
  struct pipe_mgr_prsr_tcam_tof3_shadow tof3[PIPE_DIR_MAX];
} pipe_mgr_prsrtcam_shadow_db_t;

#define PIPE_MGR_TOF2_MIRRTBL_WORD_WIDTH 16
#define PIPE_MGR_TOF2_MIRRTBL_ENTRY_NUMB 16

/* Mirror Table shadow DB (tof2 only). */
typedef struct pipe_mgr_mirrtbl_tof2_shadow_db_ {
  uint32_t base_address;
  uint32_t data[PIPE_MGR_TOF2_MIRRTBL_WORD_WIDTH];
} pipe_mgr_mirrtbl_tof2_shadow_db_t;

#define PIPE_MGR_TOF3_MIRRTBL_WORD_WIDTH 16
#define PIPE_MGR_TOF3_MIRRTBL_ENTRY_NUMB 16
typedef struct pipe_mgr_mirrtbl_tof3_shadow_db_ {
  uint32_t base_address;
  uint32_t data[PIPE_MGR_TOF3_MIRRTBL_WORD_WIDTH];
} pipe_mgr_mirrtbl_tof3_shadow_db_t;

typedef union pipe_mgr_mirrtbl_shadow_db_ {
  struct pipe_mgr_mirrtbl_tof2_shadow_db_
      tof2[PIPE_DIR_MAX][PIPE_MGR_TOF2_MIRRTBL_ENTRY_NUMB];
  struct pipe_mgr_mirrtbl_tof3_shadow_db_
      tof3[PIPE_DIR_MAX][PIPE_MGR_TOF3_MIRRTBL_ENTRY_NUMB];
} pipe_mgr_mirrtbl_shadow_db_t;

/* Pipe Manager DB Entry. */
typedef struct pipe_mgr_db_ {
  // Device ID.
  bf_dev_id_t dev;

  // Instruction Memory (IMEM) content.
  // Indexed by phy-pipe and stage.
  union pipe_imem_shadow_db **imem_db;

  // Whether GFM parity check is enabled by the compiler.
  pipe_bitmap_t gfm_hash_parity_enabled;

  // Galois Field Matrix (GFM) content.
  // Indexed by phy-pipe and stage.
  union pipe_gfm_shadow_db **gfm_db;

  // Map Ram details.
  // Indexed by phy-pipe and stage.
  pipe_map_ram_info_t **map_ram;

  // Parser instance map.
  // Maps a handle (prsr_instance_hdl) to pipe_mgr_prsr_instance_t.
  // One map per profile. Indexed by pipe and gress.
  bf_map_t **prsr_db;

  // Parser-related register/memory base address.
  // Indexed by gress.
  union pipe_parser_base_addr_t *prsr_base_addr;

  // Parser tcam shadow content.
  // Indexed by phy-pipe and parser (Tof) or prsr_group (Tof2).
  pipe_mgr_prsrtcam_shadow_db_t **prsr_tcam_db;

  // Mirror table content.
  // Indexed by phy-pipe. Tof2 only.
  pipe_mgr_mirrtbl_shadow_db_t *mirrtbl;

  // Table handle info.
  // Indexed by phy-pipe and stage.
  pipe_tbl_hdl_info_t **tbl_hdl;

} pipe_mgr_db_t;

/* Pipe Manager DB.
 *
 * One entry per device. Keeps track of information (particularly shadow RAM)
 * needed to repair MAU tables in the event of an ECC or parity error.
 * Also called the Interrupt or Shadow DB and the Parser DB.
 */
extern pipe_mgr_db_t *pipe_db[PIPE_MGR_NUM_DEVICES];

/* Selector shadow DB to store register values shared between
 * compiler and bf-drivers
 * TODO: move this inside the pipe_mgr_db
 */
#define PIPE_SEL_PGM_ROWS 8
#define PIPE_SEL_PGM_COLS 2
typedef struct pipe_mgr_sel_pgm_db_ {
  uint32_t pgm[PIPE_SEL_PGM_ROWS][PIPE_SEL_PGM_COLS];
} pipe_mgr_sel_pgm_db_t;

#define PIPE_SEL_HASH_SEEDS_NUM 52
typedef struct pipe_mgr_sel_hash_seed_db_ {
  uint32_t hash_seed[PIPE_SEL_HASH_SEEDS_NUM];
} pipe_mgr_sel_hash_seed_db_t;

#define PIPE_SEL_POWER_CTL_ROWS 2
#define PIPE_SEL_POWER_CTL_COLS 16
typedef struct pipe_mgr_sel_power_db_ {
  uint32_t power_ctl[PIPE_SEL_POWER_CTL_ROWS][PIPE_SEL_POWER_CTL_COLS];
} pipe_mgr_sel_power_db_t;

/* Selector shadow DB */
typedef struct pipe_mgr_sel_shadow_db_ {
  bf_dev_id_t dev;
  pipe_mgr_sel_power_db_t **power_db;     // power DB
  pipe_mgr_sel_hash_seed_db_t **seed_db;  // seed DB
  pipe_mgr_sel_pgm_db_t **pgm_db;         // parity group mask DB
} pipe_mgr_sel_shadow_db_t;

extern pipe_mgr_sel_shadow_db_t *pipe_sel_shadow_db[PIPE_MGR_NUM_DEVICES];
#define PIPE_SEL_SHADOW_DB(dev) (pipe_sel_shadow_db[dev])

/* Maximum number of entries in error event log. */
#define PIPE_MGR_ERR_EVT_LOG_MAX 500

#define PIPE_DB_DATA(dev) (*pipe_db[dev])

/* Interrupt Data. */
#define PIPE_INTR_IMEM_DATA(dev, pipe, stage) \
  (PIPE_DB_DATA(dev).imem_db[pipe][stage])
#define PIPE_INTR_GFM_DATA(dev, pipe, stage) \
  (PIPE_DB_DATA(dev).gfm_db[pipe][stage])
#define PIPE_INTR_MIRR_DATA(dev, pipe) (PIPE_DB_DATA(dev).mirrtbl[pipe])
#define PIPE_INTR_MAP_RAM_TYPE(dev, pipe, stage, mem_id) \
  (PIPE_DB_DATA(dev).map_ram[pipe][stage].info[mem_id].type)
#define PIPE_INTR_MAP_RAM_TBL_HDL(dev, pipe, stage, mem_id) \
  (PIPE_DB_DATA(dev).map_ram[pipe][stage].info[mem_id].hdl)
#define PIPE_INTR_TBL_HDL(dev, pipe, stage, mem_id, index) \
  (PIPE_DB_DATA(dev).tbl_hdl[pipe][stage].info[mem_id][index].hdl)
#define PIPE_INTR_TBL_TYPE(dev, pipe, stage, mem_id, index) \
  (PIPE_DB_DATA(dev).tbl_hdl[pipe][stage].info[mem_id][index].tbl_type)
#define PIPE_INTR_TBL_MEM_TYPE(dev, pipe, stage, mem_id, index) \
  (PIPE_DB_DATA(dev).tbl_hdl[pipe][stage].info[mem_id][index].mem_type)
#define PIPE_INTR_TBL_TCAM_BASE_ADDR(dev, pipe, stage, mem_id, index) \
  (PIPE_DB_DATA(dev).tbl_hdl[pipe][stage].info[mem_id][index].tcam_base_addr)
#define PIPE_INTR_PRSR_TCAM_DATA(dev, pipe, prsr) \
  (PIPE_DB_DATA(dev).prsr_tcam_db[pipe][prsr])

/* Parser Data. */
#define PIPE_PRSR_DATA(dev, pipe, gress) \
  (PIPE_DB_DATA(dev).prsr_db[pipe][gress])
#define PIPE_PRSR_ADDR(dev, gress) (PIPE_DB_DATA(dev).prsr_base_addr[gress])

#define DEFAULT_PRSR_INSTANCE_HDL 0xffffffff

pipe_status_t pipe_mgr_prsr_db_init(rmt_dev_info_t *dev_info);

/**
 * The function is used to init the shadow DB for a device
 *
 * @param  dev_info              The device info struct for the chip
 * @return                       Status of the API call
 */
pipe_status_t pipe_mgr_db_init(rmt_dev_info_t *dev_info);

/**
 * The function is used to clean up state for a device
 *
 * @param  dev                   ASIC device identifier
 */
void pipe_mgr_db_cleanup(bf_dev_id_t dev);

/**
 * The function is used to set the gfm hash parity enable
 *
 * @param  dev                   ASIC device id
 * @param  pipe_bmp              Bit map of logical pipes
 * @param  enable                Enable/Disable status
 * @return                       Status of the API call
 */
pipe_status_t pipe_mgr_set_gfm_hash_parity_enable(bf_dev_id_t dev,
                                                  pipe_bitmap_t *pipe_bmp,
                                                  bool enabled);

pipe_status_t pipe_mgr_write_gfm_from_shadow(pipe_sess_hdl_t shdl,
                                             bf_dev_id_t dev,
                                             bf_dev_pipe_t log_pipe,
                                             dev_stage_t stage,
                                             int grp,
                                             bool ingress);

pipe_status_t pipe_mgr_write_mirrtbl_entry_from_shadow(pipe_sess_hdl_t shdl,
                                                       bf_dev_id_t dev,
                                                       bf_dev_pipe_t log_pipe,
                                                       uint32_t entry,
                                                       bool direction);

pipe_status_t pipe_mgr_mirrtbl_write(pipe_sess_hdl_t shdl,
                                     rmt_dev_info_t *dev_info);

/**
 * Get GFM shadow bitpair
 *
 * @param[in]  dev_info              ASIC device info
 * @param[in]  log_pipe              logical pipe
 * @param[in]  stage                 Stage
 * @param[in]  row                   Row
 * @param[in]  col                   Column
 * @param[out]  val                  Value of data
 * @return                           Status of the API call
 */
pipe_status_t pipe_mgr_gfm_shadow_entry_get(rmt_dev_info_t *dev_info,
                                            bf_dev_pipe_t log_pipe,
                                            uint32_t stage,
                                            uint32_t row,
                                            uint32_t col,
                                            uint32_t *val);

/**
 * Updates GFM shadow memory.
 *
 * @param  dev_info              ASIC device info
 * @param  pipe_bmp              Bit map of logical pipes
 * @param  stage                 Stage
 * @param  row                   Row
 * @param  col                   Column
 * @param  val                   Value of data
 * @return                       Status of the API call
 */
pipe_status_t pipe_mgr_update_gfm_shadow(rmt_dev_info_t *dev_info,
                                         pipe_bitmap_t *pipe_bmp,
                                         uint32_t stage,
                                         uint32_t row,
                                         uint32_t col,
                                         uint32_t val);

rmt_tbl_type_t pipe_mgr_map_ram_type_get(bf_dev_id_t dev,
                                         bf_dev_pipe_t pipe,
                                         dev_stage_t stage,
                                         mem_id_t mem_id);

/**
 * Sets the mem-id to tbl-hdl mapping.
 *
 * @param  dev                   ASIC device identifier
 * @param  tbl_hdl               Table handle
 * @param  pipe_bmp              Pipe bitmap
 * @return                       Type of ram map
 */
pipe_status_t pipe_mgr_set_mem_id_to_tbl_hdl_mapping(bf_dev_id_t dev,
                                                     rmt_tbl_info_t *rmt_info_p,
                                                     uint32_t num_rmt_info,
                                                     pipe_tbl_hdl_t tbl_hdl,
                                                     pipe_bitmap_t *pipe_bmp);

/**
 * Gets the table handle and type for a mem-id.
 *
 * @param  dev                  ASIC device identifier
 * @param  pipe                 Pipe
 * @param  stage                Stage
 * @param  mem_id               Memory unit identifier
 * @param  mem_type             Memory type
 * @param  tbl_hdl              Table handle (return param)
 * @param  tbl_type             Table type (return param)
 */
pipe_status_t pipe_mgr_get_mem_id_to_tbl_hdl_mapping(bf_dev_id_t dev,
                                                     bf_dev_pipe_t pipe,
                                                     dev_stage_t stage,
                                                     mem_id_t mem_id,
                                                     pipe_mem_type_t mem_type,
                                                     pipe_tbl_hdl_t *hdl,
                                                     rmt_tbl_type_t *tbl_type);

/**
 * The function is used to dump the mem-id to tbl-hdl mapping
 *
 * @param  uc                   uCLI context
 * @param  dev                  ASIC device identifier
 * @param  in_pipe              Pipe
 * @param  in_stage             Stage
 * @param  mem_id               Memory unit identifier
 */
void pipe_mgr_dump_mem_id_to_tbl_hdl_mapping(ucli_context_t *uc,
                                             bf_dev_id_t dev,
                                             bf_dev_pipe_t in_pipe,
                                             dev_stage_t in_stage,
                                             mem_id_t in_mem_id);

/**
 * The function is used to dump the GFM shadow data
 *
 * @param  uc                   uCLI context
 * @param  dev                  ASIC device identifier
 * @param  in_pipe              Pipe
 * @param  in_stage             Stage
 */
void pipe_mgr_dump_gfm_shadow(ucli_context_t *uc,
                              bf_dev_id_t dev,
                              bf_dev_pipe_t in_pipe,
                              dev_stage_t in_stage);

/**
 * The function is used to dump the hash seed shadow data
 *
 * @param  uc                   uCLI context
 * @param  dev                  ASIC device identifier
 * @param  in_pipe              Pipe
 * @param  in_stage             Stage
 */
void pipe_mgr_dump_hash_seed_shadow(ucli_context_t *uc,
                                    bf_dev_id_t dev,
                                    bf_dev_pipe_t in_pipe,
                                    dev_stage_t in_stage);

/**
 * Looks up imem registers and stores their contents.
 *
 * @param [in] dev_info          ASIC device info
 * @param [in] log_pipe_mask     Bitmap of logical pipes
 * @param [in] stage             Stage
 * @param [in] base_address      Address of register, assumes pipe 0
 * @param [in] data              Value of register
 * @param [in] data_len          The number of bytes of data
 * @param [out] shadowed         True if the value was saved in the SW shadow
 * @return                       Status of the API call
 */
pipe_status_t pipe_mgr_lookup_cache_imem_register_val(rmt_dev_info_t *dev_info,
                                                      uint32_t log_pipe_mask,
                                                      dev_stage_t stage,
                                                      uint32_t base_address,
                                                      uint8_t *data,
                                                      int data_len,
                                                      bool *shadowed);

/**
 * Write imem from SW shadow via DMA.
 *
 * @param [in] shdl            Session handle for the writes
 * @param [in] dev_info        ASIC device info
 * @param [in] phy_pipe_filter Which pipe to write, BF_DEV_PIPE_ALL supported
 * @param [in] stage_filter    Which stage to write, -1 for all stages
 * @param [in] chip_init       True if it is guaranteed no traffic is flowing
 *                             and the MAUs are in fast-mode.
 * @return                       Status of the API call
 */
pipe_status_t pipe_mgr_imem_write(pipe_sess_hdl_t shdl,
                                  rmt_dev_info_t *dev_info,
                                  bf_dev_pipe_t phy_pipe_filter,
                                  int stage_filter,
                                  bool chip_init);

/**
 * Looks up parser memory content and stores it.
 *
 * @param  dev_info             ASIC device info
 * @param  prsr_instance_hdl    Parser instance handle
 * @param  log_pipe_mask        Bit map of logical pipes
 * @param  prof_id              Parser profile identifier
 * @param  address              Address of memory
 * @param  data                 Value of register
 * @param  data_len             Length of data
 * @param  shadowed             Whether data has been shadowed
 * @return                      Status of the API call
 */
pipe_status_t pipe_mgr_lookup_cache_parser_bin_cfg(
    rmt_dev_info_t *dev_info,
    pipe_prsr_instance_hdl_t prsr_instance_hdl,
    profile_id_t prof_id,
    uint64_t address,
    uint8_t *data,
    int data_len,
    bool *shadowed);

pipe_status_t pipe_mgr_lookup_cache_parser_bin_reg_cfg(
    rmt_dev_info_t *dev_info,
    pipe_prsr_instance_hdl_t prsr_instance_hdl,
    profile_id_t prof_id,
    uint32_t address,
    uint32_t data,
    bool *shadowed);

pipe_status_t pipe_mgr_set_parser_tcam_shadow(rmt_dev_info_t *dev_info,
                                              bf_dev_pipe_t pipe,
                                              bool ing0_egr1,
                                              int prsr_id,
                                              int tcam_index,
                                              uint8_t data_len,
                                              uint8_t *word0,
                                              uint8_t *word1);
pipe_status_t pipe_mgr_lookup_cache_mirrtbl_register_content(
    rmt_dev_info_t *dev_info,
    uint32_t log_pipe_mask,
    uint32_t base_address,
    uint8_t *data,
    int data_len);

/**
 * The function is used to lookup GFM memory content and store it
 *
 * @param  dev_info              ASIC device info
 * @param  log_pipe_mask         Bit map of logical pipes
 * @param  stage                 Stage
 * @param  address               Address of memory
 * @param  data                  Value of register
 * @param  data_len              Length of data
 * @return                       Status of the API call
 */
pipe_status_t pipe_mgr_lookup_cache_gfm(rmt_dev_info_t *dev_info,
                                        uint32_t log_pipe_mask,
                                        dev_stage_t stage,
                                        uint32_t address,
                                        uint8_t *data,
                                        int data_len);
/**
 * The function is used to recalculate and write the seed parity
 *
 * @param  sess_hdl              Session Handle
 * @param  dev_info              ASIC device info
 * @param  pipe_bmp              Bit map of logical pipes
 * @param  stage                 Stage
 * @return                       Status of the API call
 */
pipe_status_t pipe_mgr_recalc_write_seed_parity(pipe_sess_hdl_t sess_hdl,
                                                rmt_dev_info_t *dev_info,
                                                pipe_bitmap_t *pipe_bmp,
                                                dev_stage_t stage);

/**
 * The function is used to recalculate and write the GFM parity
 *
 * @param  sess_hdl              Session Handle
 * @param  dev_info              ASIC device info
 * @param  pipe_bmp              Bit map of logical pipes
 * @param  stage                 Stage
 * @param  skip_write            Skip the write to asic
 * @return                       Status of the API call
 */
pipe_status_t pipe_mgr_recalc_write_gfm_parity(pipe_sess_hdl_t sess_hdl,
                                               rmt_dev_info_t *dev_info,
                                               pipe_bitmap_t *pipe_bmp,
                                               dev_stage_t stage,
                                               bool skip_write);
/**
 * The function is used to set the type for all map rams
 *
 * @param  dev                   ASIC device identifier
 * @param  tbl_hdl               Table handle
 * @param  pipe_bmp              Pipe bitmap
 * @return                       Type of ram map
 */
pipe_status_t pipe_mgr_set_all_map_ram_type(bf_dev_id_t dev,
                                            pipe_mat_tbl_hdl_t tbl_hdl,
                                            pipe_bitmap_t *pipe_bmp);

/**
 * The function is used to get all pvs hdls bond with the prsr_instance_hdl
 * @param  dev                   ASIC device identifier
 * @param  pipeid                Pipe id
 * @param  gress                 direction,0-ingress,1-egress
 * @param  prsr_instance_hdl     Parser instance handler
 * @param  pvs_hdl               Pointer to the set of pvs handlers, read only,
 * should not modify
 * @param  pvs_hdl_numb          Number of pvs handlers
 * @return                       Status of the API call
 */
pipe_status_t pipe_mgr_prsr_instance_get_pvs_hdls(
    bf_dev_id_t dev,
    bf_dev_pipe_t pipeid,
    uint8_t gress,
    pipe_prsr_instance_hdl_t prsr_instance_hdl,
    uint32_t **pvs_hdl,
    uint32_t *pvs_hdl_numb);

/**
 * The function is used to get the phase0_hdl bond with the prsr_instance_hdl
 * @param  dev                   ASIC device identifier
 * @param  pipeid                Pipe id
 * @param  gress                 direction,0-ingress,1-egress
 * @param  prsr_instance_hdl     Parser instance handler
 * @param  phase0_hdl            Phase0 handler
 * @return                       Status of the API call
 */
pipe_status_t pipe_mgr_prsr_instance_get_phase0_hdl(
    bf_dev_id_t dev,
    bf_dev_pipe_t pipeid,
    uint8_t gress,
    pipe_prsr_instance_hdl_t prsr_instance_hdl,
    uint32_t *phase0_hdl);

pipe_status_t pipe_mgr_set_prsr_instance_phase0_hdl(
    bf_dev_id_t dev,
    bf_dev_pipe_t pipeid,
    uint8_t gress,
    pipe_prsr_instance_hdl_t prsr_instance_hdl,
    uint32_t phase0_table_hdl);

pipe_status_t pipe_mgr_prsr_instance_get_bin_cfg(
    bf_dev_id_t dev,
    bf_dev_pipe_t pipeid,
    uint8_t gress,
    pipe_prsr_instance_hdl_t prsr_instance_hdl,
    union pipe_parser_bin_config_t **bin_cfg);

pipe_status_t pipe_mgr_get_prsr_instance_hdl(
    bf_dev_id_t dev,
    bf_dev_pipe_t pipeid,
    uint8_t gress,
    uint8_t prsr_id,
    pipe_prsr_instance_hdl_t *prsr_instance_hdl);

pipe_status_t pipe_mgr_get_prsr_instance_hdl_from_name(
    bf_dev_id_t dev,
    bf_dev_pipe_t pipeid,
    uint8_t gress,
    char *prsr_instance_name,
    pipe_prsr_instance_hdl_t *prsr_instance_hdl);

pipe_status_t pipe_mgr_get_prsr_instance_name_from_hdl(
    bf_dev_id_t dev,
    bf_dev_pipe_t pipeid,
    uint8_t gress,
    pipe_prsr_instance_hdl_t prsr_instance_hdl,
    char *prsr_instance_name);

pipe_status_t pipe_mgr_get_prsr_default_instance_hdl(
    bf_dev_id_t dev,
    bf_dev_pipe_t pipeid,
    uint8_t gress,
    uint8_t prsr_id,
    pipe_prsr_instance_hdl_t *default_prsr_instance_hdl);

pipe_status_t pipe_mgr_prsr_instance_get_default_profile(
    bf_dev_id_t dev,
    bf_dev_pipe_t pipeid,
    uint8_t gress,
    pipe_prsr_instance_hdl_t prsr_instance_hdl,
    uint64_t *prsr_map);

pipe_status_t pipe_mgr_prsr_instance_get_profile(
    bf_dev_id_t dev,
    bf_dev_pipe_t pipeid,
    uint8_t gress,
    pipe_prsr_instance_hdl_t prsr_instance_hdl,
    uint64_t *prsr_map);

pipe_status_t pipe_mgr_prsr_instance_set_profile(
    bf_dev_id_t dev,
    bf_dev_pipe_t pipeid,
    uint8_t gress,
    pipe_prsr_instance_hdl_t prsr_instance_hdl,
    uint64_t prsr_map);

pipe_status_t pipe_mgr_prsr_instance_reset_profile(
    bf_dev_id_t dev,
    bf_dev_pipe_t pipeid,
    uint8_t gress,
    pipe_prsr_instance_hdl_t prsr_instance_hdl);

pipe_status_t pipe_mgr_gfm_test(pipe_sess_hdl_t shdl,
                                bf_dev_target_t dev_tgt,
                                bf_dev_direction_t gress,
                                dev_stage_t stage_id,
                                int num_patterns,
                                uint64_t *row_patterns,
                                uint64_t *row_bad_parity);

pipe_status_t pipe_mgr_gfm_test_col(pipe_sess_hdl_t shdl,
                                    bf_dev_target_t dev_tgt,
                                    bf_dev_direction_t gress,
                                    dev_stage_t stage_id,
                                    int column,
                                    uint16_t col_data[64]);
#endif
