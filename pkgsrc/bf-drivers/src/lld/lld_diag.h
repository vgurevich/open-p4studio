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


#ifndef LLD_DIAG_H_INCLUDED
#define LLD_DIAG_H_INCLUDED

#include <diag/bf_dev_diag.h>

typedef bf_status_t (*lld_mem_traverse_cb)(bf_dev_id_t dev_id,
                                           bf_subdev_id_t subdev_id,
                                           uint64_t offset,
                                           uint32_t len,
                                           char *key_wd);

/* -------------- EXPORTED APIS to DIAG --------------------- */
bf_status_t lld_traverse_mau_memories(bf_dev_id_t dev_id);
bf_status_t lld_traverse_mau_memories_dma(bf_dev_id_t dev_id);
bf_status_t lld_mem_test_cb(bf_dev_id_t dev_id,
                            bf_subdev_id_t subdev_id,
                            uint64_t offset,
                            uint32_t len,
                            char *key_wd);
bf_status_t lld_mem_test_dma_cb(bf_dev_id_t dev_id,
                                bf_subdev_id_t subdev_id,
                                uint64_t offset,
                                uint32_t len,
                                char *key_wd);
bf_status_t lld_traverse_all_mems(bf_dev_id_t dev_id,
                                  lld_mem_traverse_cb fn,
                                  bool all_mems,
                                  bool tm_only);
void lld_diag_cfg_reset();
void lld_diag_cfg_set(uint32_t pipe_mask,
                      uint32_t stag_mask,
                      int quick,
                      int tm,
                      bf_diag_test_pattern_t pattern,
                      uint64_t data0,
                      uint64_t data1);
void *lld_memtest_results_get(bf_dev_id_t dev_id);
bf_status_t lld_diag_reg_test_mau(bf_dev_id_t dev_id, bool quick, bool dma);
bf_status_t lld_diag_reg_test_parde(bf_dev_id_t dev_id, bool quick, bool dma);
bf_status_t lld_diag_reg_test_tm(bf_dev_id_t dev_id, bool quick, bool dma);
void *lld_regtest_results_get(bf_dev_id_t dev_id);

bf_status_t lld_interrupt_test_extended(bf_dev_id_t dev_id);
bf_status_t lld_interrupt_test_quick(bf_dev_id_t dev_id, uint32_t pipe_bmp);
void *lld_inttest_results_get(bf_dev_id_t dev_id);

bf_status_t lld_mem_test_sch_refresh_enable(bf_dev_id_t dev_id, bool enable);

#endif  // LLD_DIAG_H_INCLUDED
