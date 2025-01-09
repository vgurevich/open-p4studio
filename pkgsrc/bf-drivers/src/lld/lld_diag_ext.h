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


#ifndef LLD_DIAG_EXT_H_INCLUDED
#define LLD_DIAG_EXT_H_INCLUDED

#include <diag/bf_dev_diag.h>
#include <lld/bf_dev_if.h>
#include <lld/lld_interrupt_if.h>

#define LLD_DIAG_MAGIC_DATA 0xdeadbeef
#define LLD_INTR_MAX_SRAM_ROWS 8
#define LLD_INTR_MAX_SRAM_COLS 12
#define LLD_INTR_MAX_TCAM_ROWS 12
#define LLD_DIAG_CHECKERBOARD_PATTERN 0x5555555555555555ull
#define LLD_DIAG_INV_CHECKERBOARD_PATTERN 0xaaaaaaaaaaaaaaaaull
#define LLD_DIAG_ULONG_DATA_MASK 0xffffffffffffffffull

#define LLD_DIAG_TF1_STAGE_GET(stage) ((stage >> 19) & 0xf)
#define LLD_DIAG_TF2_STAGE_GET(stage) ((stage >> 19) & 0x1f)
#define LLD_DIAG_TF3_STAGE_GET(stage) ((stage >> 19) & 0x1f)

#define LLD_DIAG_TF1_PHY_PIPE_GET(pipe) ((pipe >> 23) & 0x3)
#define LLD_DIAG_TF2_PHY_PIPE_GET(pipe) ((pipe >> 24) & 0x3)
#define LLD_DIAG_TF3_PHY_PIPE_GET(pipe) ((pipe >> 24) & 0x3)

#define LLD_DIAG_TF1_PIPE_BASE_ADDR 0x20000000000ull
#define LLD_DIAG_WAC_QID_MAP_TF2_ENTRIES 144
#define LLD_DIAG_WAC_QID_MAP_TF3_ENTRIES 288

// In Micro secs
#define LLD_DIAG_MILLI_SEC(ms) (ms * 1000)
#define LLD_DIAG_SEC(sec) (sec * 1000 * 1000)

#define LLD_DIAG_TCAM_PARITY_BITS (3ull << 45ull)
#define LLD_DIAG_MAX_ALLOWED_FAILURES 3

// DMA test
typedef enum diag_dma_cb_type {
  BF_DIAG_REG_DMA_CB = 1,
  BF_DIAG_MEM_DMA_CB = 2,
} diag_dma_cb_type_t;

typedef enum diag_mem_type {
  BF_DIAG_MEM_UNIT_RAM,
  BF_DIAG_MEM_MAP_RAM,
  BF_DIAG_MEM_STATS_RAM,
  BF_DIAG_MEM_METERS_RAM,
  BF_DIAG_MEM_TCAM,
  BF_DIAG_MEM_MAX
} bf_diag_mem_type_t;

/*************************************************
 * Restrict diagnostic tests to the configured
 * limits.
 *************************************************/
typedef struct lld_diag_cfg_t {
  uint32_t pipe_mask;
  uint32_t stage_mask;
  int quick;
  int do_tm;
  bf_diag_test_pattern_t pattern;
  uint64_t pattern_data0;
  uint64_t pattern_data1;
} lld_diag_cfg_t;

typedef struct lld_diag_results_ {
  bf_diag_mem_results_t mem_results;             /* memory test results */
  bf_diag_reg_results_t reg_results;             /* register test results */
  bf_diag_interrupt_results_t interrupt_results; /* interrupt test results */
} lld_diag_results_t;

typedef struct lld_reg_dma_cmplt_info_ {
  diag_dma_cb_type_t dma_cb_type;  // Holds type of dma completion
  uint32_t reg_addr;
  uint8_t *vaddr;
  uint64_t phy_addr;
  uint32_t buf_size;
  bf_dma_type_t type;
} lld_reg_dma_cmplt_info_t;

typedef struct lld_mem_dma_cmplt_info_ {
  diag_dma_cb_type_t dma_cb_type;  // Holds type of dma completion
  uint8_t *base_p;                 // original, unaligned ptr from bf_sys_malloc
  uint8_t *wb_buf;                 // above, properly aligned
  uint64_t tofino_addr;
  int entry_sz;
  int n_entry;
  uint64_t mask64_0;
  uint64_t mask64_1;
  char *name;
  char name_storage[255];
  uint8_t *vaddr;
  uint64_t phy_addr;
  uint32_t buf_size;
  bf_dma_type_t type;
  bf_diag_test_pattern_t pattern;
  uint64_t pattern_data0;
  uint64_t pattern_data1;
} lld_mem_dma_cmplt_info_t;

/* Interrupt tests callback info */
#define LLD_INT_TEST_CACHED_DATA_MAX 1000
typedef struct lld_int_test_cached_data_ {
  bool valid;
  lld_int_cb cb_fn;
  void *userdata;
  bf_subdev_id_t subdev_id;
  uint32_t status_addr;
  uint32_t exp_int_status_val;
  uint32_t enable_addr;
  uint32_t enable_val;
  bool poll;
  int seen_cnt;
} lld_int_test_cached_data_t;

/* ------------- INTERNAL FUNCTIONS ------------------- */

int lld_diag_dma_alloc(bf_dev_id_t dev_id,
                       bf_subdev_id_t subdev_id,
                       uint32_t sz,
                       void **vaddr,
                       uint64_t *paddr);
int lld_diag_dma_free(bf_dev_id_t dev_id,
                      bf_subdev_id_t subdev_id,
                      uint8_t *vaddr);
void lld_diag_cfg_set(uint32_t pipe_mask,
                      uint32_t stage_mask,
                      int quick,
                      int tm,
                      bf_diag_test_pattern_t pattern,
                      uint64_t data0,
                      uint64_t data1);
void lld_diag_cfg_reset();
void lld_diag_test_pattern_prbs_get(uint64_t *data0, uint64_t *data1);
bf_status_t lld_ilist_add_register_write(bf_dev_id_t dev_id,
                                         bf_subdev_id_t subdev_id,
                                         uint32_t reg_addr,
                                         uint32_t reg_data);
bf_status_t lld_wlist_add_register_write(bf_dev_id_t dev_id,
                                         bf_subdev_id_t subdev_id,
                                         uint32_t reg_addr,
                                         uint32_t reg_data);
/* Memory test */
void lld_memtest_results_clear();
void lld_memtest_result_set(bool success);
void lld_memtest_read_error_set(uint64_t addr);
void lld_memtest_write_error_set(uint64_t addr);
void lld_memtest_write_list_error_set();
void lld_memtest_write_block_error_set();
void lld_memtest_data_error_set(uint64_t addr,
                                uint64_t exp_0,
                                uint64_t exp_1,
                                uint64_t data_0,
                                uint64_t data_1,
                                uint64_t mask_0,
                                uint64_t mask_1);
void lld_memtest_dma_msgs_sent_inc();
void lld_memtest_dma_cmplts_rcvd_inc();

void lld_reg_mem_test_completion_cb(int dev_id,
                                    bf_subdev_id_t subdev_id,
                                    bf_dma_dr_id_t dr,
                                    uint64_t ts_sz,
                                    uint32_t attr,
                                    uint32_t status,
                                    uint32_t type,
                                    uint64_t msg_id,
                                    int s,
                                    int e);

/* Register test */
bf_status_t lld_diag_reg_test(bf_dev_id_t dev_id, int quick);
void lld_regtest_dma_msgs_sent_inc();
void lld_regtest_dma_cmplts_rcvd_inc();

/* Interrupt test */
void lld_regtest_results_clear();
void lld_regtest_result_set(bool success);
void lld_inttest_results_clear();
void lld_inttest_result_set(bool success);
void lld_inttest_error_set(char *reg_name,
                           uint32_t addr,
                           uint32_t exp_status,
                           uint32_t rcvd_status);
void lld_inttest_events_exp_inc();
void lld_inttest_events_rcvd_inc();
void lld_interrupt_test_restore_callbacks(bf_dev_id_t dev_id);

#endif  // LLD_DIAG_EXT_H_INCLUDED
