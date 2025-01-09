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


#ifndef __TM_HW_ACCESS_H__
#define __TM_HW_ACCESS_H__

#include "tm_ctx.h"

/* ----- DMA Buffer sizing for Tofino ------ */
// Came up with number 32 by checking logs; At the TM init time 24 buffers of
// 32KB
// were built before pushing to HW.
#define BF_TM_TOFINO_DMA_BUFFER_SIZE (1 << 15) /* 32 Kilobyte */
#define BF_TM_TOFINO_DMA_POOL_SIZE ((32) * BF_TM_TOFINO_DMA_BUFFER_SIZE)

/* ----- DMA Buffer sizing for TofinoLite ------ */
#define BF_TM_TOFINOLITE_DMA_BUFFER_SIZE (1 << 15) /* 32 Kilobyte */
#define BF_TM_TOFINOLITE_DMA_POOL_SIZE ((32) * BF_TM_TOFINO_DMA_BUFFER_SIZE)

/* ----- Add Here : DMA Buffer sizing for New-ASIC ------ */

#define BF_TM_FLUSH_WL(dev) bf_tm_flush_wlist(dev)

void bf_tm_flush_wlist(bf_dev_id_t dev);
void bf_tm_cleanup_wlist(bf_dev_id_t dev);
void bf_tm_complete_ops(bf_dev_id_t dev);
bf_tm_status_t bf_tm_write_register(bf_dev_id_t dev,
                                    uint32_t offset,
                                    uint32_t data);
bf_tm_status_t bf_tm_write_memory(bf_dev_id_t dev,
                                  uint64_t ind_addr,
                                  uint8_t wr_sz,
                                  uint64_t hi,
                                  uint64_t lo);
bf_tm_status_t bf_tm_read_register(bf_dev_id_t dev,
                                   uint32_t offset,
                                   uint32_t *data);
bf_tm_status_t bf_tm_read_memory(bf_dev_id_t dev,
                                 uint64_t ind_addr,
                                 uint64_t *hi,
                                 uint64_t *lo);

bf_tm_status_t bf_tm_subdev_write_register(bf_dev_id_t dev,
                                           bf_subdev_id_t subdev_id,
                                           uint32_t offset,
                                           uint32_t data);
bf_tm_status_t bf_tm_subdev_write_memory(bf_dev_id_t dev,
                                         bf_subdev_id_t subdev_id,
                                         uint64_t ind_addr,
                                         uint8_t wr_sz,
                                         uint64_t hi,
                                         uint64_t lo);
bf_tm_status_t bf_tm_subdev_read_register(bf_dev_id_t dev,
                                          bf_subdev_id_t subdev_id,
                                          uint32_t offset,
                                          uint32_t *data);
bf_tm_status_t bf_tm_subdev_read_memory(bf_dev_id_t dev,
                                        bf_subdev_id_t subdev_id,
                                        uint64_t ind_addr,
                                        uint64_t *hi,
                                        uint64_t *lo);

bf_tm_status_t bf_tm_setup_dma_sizes(bf_dev_id_t dev,
                                     bf_subdev_id_t subdev_id,
                                     uint32_t poolsize,
                                     uint32_t buffersize);

bf_tm_status_t bf_tm_setup_dma(bf_dev_id_t dev,
                               bf_subdev_id_t subdev_id,
                               bf_sys_dma_pool_handle_t hdl);

#endif
