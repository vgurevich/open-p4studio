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


#ifndef __MC_MGR_DRV_H__
#define __MC_MGR_DRV_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <dvm/bf_drv_intf.h>
#include <mc_mgr/mc_mgr_config.h>

#define MC_MGR_DRV_SUBDEV_ID_ALL 0xff
#define MC_PVT_MASK_ALL 0x3
typedef struct mc_mgr_drv_buf_t mc_mgr_drv_buf_t;
struct mc_mgr_drv_buf_t {
  mc_mgr_drv_buf_t *next;
  mc_mgr_drv_buf_t *prev;
  bf_map_t mgids_updated;
  bf_map_t freed_rdm_addrs;
  uint8_t *addr;
  bf_sys_dma_pool_handle_t pool;
  bf_phys_addr_t phys_addr;
  uint64_t msgId;
  uint32_t size;
  uint32_t used;
  uint16_t count;
  bf_dev_id_t dev;
  uint8_t buf_pushed;
  uint8_t wr_list_size;
  bf_subdev_id_t subdev_id; /* Set to specific subdevice */
};

typedef struct mc_mgr_drv_buf_pool_t mc_mgr_drv_buf_pool_t;
struct mc_mgr_drv_buf_pool_t {
  bf_sys_dma_pool_handle_t pool;
  unsigned int buf_sz;    /* Size of each DMA buffer. */
  unsigned int buf_cnt;   /* Number of DMA buffers available. */
  int in_use;             /* Count of buffers outstanding. */
  mc_mutex_t mtx;         /* Protects the in_use count. */
  mc_mgr_drv_buf_t *used; /* Allocated with size equal to buf_cnt. */
};

typedef struct mc_mgr_drv_wr_list_t mc_mgr_drv_wr_list_t;
struct mc_mgr_drv_wr_list_t {
  mc_mgr_drv_buf_t *bufList;  // List of buffers
  int count;                  // Number of buffers
};

bf_status_t mc_mgr_drv_init();
bf_status_t mc_mgr_drv_init_dev(bf_dev_id_t dev, bf_dma_info_t *dma_info);
void mc_mgr_drv_remove_dev(bf_dev_id_t dev);
void mc_mgr_drv_warm_init_quick(bf_dev_id_t dev);
void mc_mgr_drv_cmplt_operations(int sid, int dev_id);
int mc_mgr_drv_wrl_append(int dev,
                          bf_subdev_id_t subdev_id,
                          int sid,
                          int width,
                          uint64_t addr,
                          uint64_t hi,
                          uint64_t lo,
                          const char *where,
                          const int line);
int mc_mgr_drv_wrl_append_reg(int dev,
                              int sid,
                              uint32_t addr,
                              uint32_t data,
                              const char *where,
                              const int line);
bf_status_t mc_mgr_drv_wrl_send(int sid, bool is_last);
void mc_mgr_drv_wrl_abort(int sid);
int mc_mgr_drv_start_rdm_change(bf_dev_id_t dev, bf_dev_pipe_t pipe);
int mc_mgr_drv_read_rdm_change(bf_dev_id_t dev, bf_dev_pipe_t pipe);

bf_status_t mc_mgr_write_register(bf_dev_id_t dev_id,
                                  uint32_t reg_addr,
                                  uint32_t reg_data);

bf_status_t mc_mgr_drv_service_dr(bf_dev_id_t dev_id);
#endif
