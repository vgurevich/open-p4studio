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


//
//  dru_sim.c
//
//

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <sched.h>
#include <string.h>
#include <sys/time.h>
#include <assert.h>

#ifdef TARGET_IS_MODEL
#include <pthread.h>
#define bf_sys_mutex_t pthread_mutex_t
#define bf_sys_mutex_lock(x) pthread_mutex_lock(x)
#define bf_sys_mutex_unlock(x) pthread_mutex_unlock(x)
#define bf_sys_mutex_init(x) pthread_mutex_init(x, NULL)
#else
#include <target-sys/bf_sal/bf_sys_sem.h>
#endif
#include <lld/lld_bits.h>
#include <dru_sim/dru_sim.h>

static bf_sys_mutex_t fifo_lock;

extern int dru_sim_debug_mode;
extern dru_write_dma_chnl_fn write_dma_chnl;
extern dru_read_dma_chnl_fn read_dma_chnl;
extern dru_write_vpi_link_fn write_vpi_link;
extern dru_push_to_dru_fn push_to_dru;
extern dru_push_to_model_fn push_to_model;

#define PCIE_FIFO_DEPTH 102400

typedef struct pcie_wr_pipe_s {
  int volatile hd;
  int volatile tl;
  pcie_msg_t volatile fifo[PCIE_FIFO_DEPTH];
} pcie_wr_pipe_t;

typedef struct pcie_rd_pipe_s {
  int volatile hd;
  int volatile tl;
  pcie_msg_t volatile fifo[2];  // synchronous
} pcie_rd_pipe_t;

pcie_wr_pipe_t to_pcie = {0};
pcie_wr_pipe_t from_pcie = {0};

pcie_rd_pipe_t rd_from_pcie = {0};
pcie_rd_pipe_t rd_from_cpu = {0};

static dru_sim_dma2virt_dbg_callback_fn_fifo bf_mem_dma2virt_dbg = NULL;

/** cpu_to_pcie_wr_fifo
 *
 * write one u32 from CPU to a PCIe mapped address
 */
void cpu_to_pcie_wr_fifo(dru_dev_id_t asic, uint32_t addr, uint32_t value) {
  pcie_wr_pipe_t *pipe = &to_pcie;
  pcie_msg_t volatile *msg;

  bf_sys_mutex_lock(&fifo_lock);
  msg = &pipe->fifo[pipe->tl];

  if (((pipe->tl + 1) % PCIE_FIFO_DEPTH) == pipe->hd) {
    // printf("PCIe Fifo full..\n");
    while (((pipe->tl + 1) % PCIE_FIFO_DEPTH) == pipe->hd) {
      sched_yield();
    }
    // printf("PCIe Fifo space available\n");
  }
  msg->typ = pcie_op_wr;
  msg->asic = asic;
  msg->addr = addr;
  msg->value = value;
  pipe->tl = (pipe->tl + 1) % PCIE_FIFO_DEPTH;

  bf_sys_mutex_unlock(&fifo_lock);
}

/** cpu_to_pcie_rd_fifo
 *
 * read one u32 from a PCIe mapped address
 */
uint32_t cpu_to_pcie_rd_fifo(dru_dev_id_t asic, uint32_t addr) {
  bf_sys_mutex_lock(&fifo_lock);
  pcie_wr_pipe_t *wr_pipe = &to_pcie;
  pcie_msg_t volatile *msg = &wr_pipe->fifo[wr_pipe->tl];
#ifdef INCLUDE_READ_TESTS
  pcie_rd_pipe_t *rd_pipe = &rd_from_pcie;
#endif  // INCLUDE_READ_TESTS

  msg->typ = pcie_op_rd;
  msg->asic = asic;
  msg->addr = addr;
  msg->value = 0;
  wr_pipe->tl = (wr_pipe->tl + 1) % PCIE_FIFO_DEPTH;

#ifdef INCLUDE_READ_TESTS
  while (rd_pipe->hd == rd_pipe->tl) {
    sched_yield();
  }
  msg = &rd_pipe->fifo[rd_pipe->hd];
  rd_pipe->hd = (rd_pipe->hd + 1) % PCIE_FIFO_DEPTH;
#endif  // INCLUDE_READ_TESTS
  bf_sys_mutex_unlock(&fifo_lock);
  return (msg->value);
}

/** dru_to_pcie_dma_wr_fifo
 *
 * write "n" u32s from the DRU simulator to CPU
 */
void dru_to_pcie_dma_wr_fifo(dru_dev_id_t asic,
                             uint64_t addr,
                             uint8_t *buf,
                             uint32_t n) {
  uint8_t *vaddr;
  vaddr = (uint8_t *)bf_mem_dma2virt_dbg(asic, addr);
  memcpy(vaddr, buf, n);
}

/** dru_to_pcie_dma_rd_fifo
 *
 * read "n" u32s from the CPU to the DRU simulator
 */
void dru_to_pcie_dma_rd_fifo(dru_dev_id_t asic,
                             uint64_t addr,
                             uint8_t *buf,
                             uint32_t n) {
  uint8_t *vaddr;
  vaddr = (uint8_t *)bf_mem_dma2virt_dbg(asic, addr);
  memcpy(buf, vaddr, n);
}

/** dru_rtn_pcie_rd_data_fifo
 *
 * return synchronous read data
 */
void dru_rtn_pcie_rd_data_fifo(pcie_msg_t *msg, uint32_t value) {
  // handle read by pcie device. Can only be a register
  pcie_rd_pipe_t *rd_pipe = &rd_from_pcie;
  pcie_msg_t volatile *rtn_msg = &rd_pipe->fifo[rd_pipe->tl];

  rtn_msg->asic = msg->asic;
  rtn_msg->value = value;
  rtn_msg->addr = msg->addr;
  rd_pipe->tl = (rd_pipe->tl + 1) % 2;
}

extern pcie_msg_t static_pcie_msg;

pcie_msg_t *dru_next_pcie_msg_fifo(void) {
  pcie_wr_pipe_t *pipe = &to_pcie;

  while (1) {
    if (pipe->hd != pipe->tl) {
      // note: this must be a structure copy since we are
      // going to update the hd_ptr. Otherwise the producer
      // could overwrite this entry.
      //
      static_pcie_msg = pipe->fifo[pipe->hd];  // structure copy
      pipe->hd = (pipe->hd + 1) % PCIE_FIFO_DEPTH;
      return &static_pcie_msg;
    } else {
      sched_yield();
    }
  }

  return NULL;
}

/** dru_init_fifo
 *
 * return synchronous read data
 */
extern dru_intf_t g_dru_intf;
void dru_init_fifo(int debug_mode,
                   int emu_integ,
                   int parallel_mode,
                   dru_write_dma_chnl_fn wr_dma_fn,
                   dru_read_dma_chnl_fn rd_dma_fn,
                   dru_write_vpi_link_fn wr_vpi_fn,
                   dru_push_to_dru_fn push_to_dru_fn,
                   dru_push_to_model_fn push_to_model_fn,
                   dru_sim_dma2virt_dbg_callback_fn_fifo dma2virt_fn) {
  (void)emu_integ;
  (void)parallel_mode;
  dru_intf_t *dru_intf = &g_dru_intf;
  dru_intf->cpu_to_pcie_rd = cpu_to_pcie_rd_fifo;
  dru_intf->cpu_to_pcie_wr = cpu_to_pcie_wr_fifo;
  dru_intf->dru_to_pcie_dma_rd = dru_to_pcie_dma_rd_fifo;
  dru_intf->dru_to_pcie_dma_wr = dru_to_pcie_dma_wr_fifo;
  dru_intf->dru_rtn_pcie_rd_data = dru_rtn_pcie_rd_data_fifo;
  dru_intf->dru_next_pcie_msg = dru_next_pcie_msg_fifo;
  bf_mem_dma2virt_dbg = dma2virt_fn;
  dru_sim_debug_mode = debug_mode;

  if (wr_dma_fn) {
    write_dma_chnl = wr_dma_fn;
  }
  if (rd_dma_fn) {
    read_dma_chnl = rd_dma_fn;
  }
  if (wr_vpi_fn) {
    write_vpi_link = wr_vpi_fn;
  }
  if (push_to_dru_fn) {
    push_to_dru = push_to_dru_fn;
  }
  if (push_to_model_fn) {
    push_to_model = push_to_model_fn;
  }

  if (bf_sys_mutex_init(&fifo_lock) != 0) {
    assert(0);
  }

  dru_create_service_thread();
}
