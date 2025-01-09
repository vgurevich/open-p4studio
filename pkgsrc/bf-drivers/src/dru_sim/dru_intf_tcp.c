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
#include <stdbool.h>
#include <sched.h>
#include <string.h>
#include <sys/time.h>
#include <assert.h>
#include <inttypes.h>

#ifdef TARGET_IS_MODEL
#include <pthread.h>
static pthread_mutex_t socket_lock;
static pthread_mutex_t dma_socket_lock;
static pthread_mutex_t read_socket_lock;
#define bf_sys_mutex_lock(x) pthread_mutex_lock(x)
#define bf_sys_mutex_unlock(x) pthread_mutex_unlock(x)
#define bf_sys_mutex_init(x) pthread_mutex_init(x, NULL)
#else
#include <target-sys/bf_sal/bf_sys_sem.h>
static bf_sys_mutex_t socket_lock;
static bf_sys_mutex_t dma_socket_lock;
static bf_sys_mutex_t read_socket_lock;
#endif
#include <bf_types/bf_types.h>
#include <dvm/bf_dma_types.h>
#include <lld/bf_dma_if.h>
#include <lld/lld_bits.h>
#include <dru_sim/dru_sim.h>

extern void *check_dru_ctrl_chnl(unsigned char *buf, int len);
extern int push_dma_from_dru(pcie_msg_t *msg, int len, uint8_t *buf);

extern int dru_sim_debug_mode;
extern dru_write_dma_chnl_fn write_dma_chnl;
extern dru_read_dma_chnl_fn read_dma_chnl;
extern dru_write_vpi_link_fn write_vpi_link;
extern dru_push_to_dru_fn push_to_dru;
extern dru_push_to_model_fn push_to_model;

extern uint64_t dma_rds;
extern uint64_t dma_wrs;
extern uint64_t dma_rd_bytes;
extern uint64_t dma_wr_bytes;

#ifdef TARGET_IS_LLD

uint32_t n_pcie_wr = 0;
uint32_t n_pcie_rd = 0;

/** cpu_to_pcie_wr_tcp
 *
 * write one u32 from CPU to a PCIe mapped address
 */
void cpu_to_pcie_wr_tcp(dru_dev_id_t asic, uint32_t addr, uint32_t value) {
  pcie_msg_t msg;

  bf_sys_mutex_lock(&socket_lock);
  n_pcie_wr++;

  msg.typ = pcie_op_wr;
  msg.asic = asic;
  msg.addr = addr;
  msg.value = value;
  msg.ind_addr = 0;
  msg.len = 0;
  push_to_dru(&msg, sizeof(msg));

  bf_sys_mutex_unlock(&socket_lock);
}

/** cpu_to_pcie_rd_tcp
 *
 * read one u32 from a PCIe mapped address
 */
uint32_t cpu_to_pcie_rd_tcp(dru_dev_id_t asic, uint32_t addr) {
  pcie_msg_t msg;

  bf_sys_mutex_lock(&socket_lock);

  n_pcie_rd++;

  msg.typ = pcie_op_rd;
  msg.asic = asic;
  msg.addr = addr;
  msg.value = 0;
  msg.ind_addr = 0;
  msg.len = 0;
  msg.value = push_to_dru(&msg, sizeof(msg));

  bf_sys_mutex_unlock(&socket_lock);

  return msg.value;
}

#else

/** dru_to_pcie_dma_wr_tcp
 *
 * write "n" u32s from the DRU simulator to CPU
 */
void dru_to_pcie_dma_wr_tcp(dru_dev_id_t asic,
                            uint64_t addr,
                            uint8_t *buf,
                            uint32_t n) {
  pcie_msg_t msg;

  bf_sys_mutex_lock(&dma_socket_lock);

  msg.typ = pcie_op_dma_wr;
  msg.asic = asic;
  msg.addr = 0;
  msg.value = 0;
  msg.ind_addr = addr;
  msg.len = n;

  push_dma_from_dru(&msg, sizeof(msg), buf);

  bf_sys_mutex_unlock(&dma_socket_lock);
}

/** dru_to_pcie_dma_rd_tcp
 *
 * read "n" u32s from the CPU to the DRU simulator
 */
void dru_to_pcie_dma_rd_tcp(dru_dev_id_t asic,
                            uint64_t addr,
                            uint8_t *buf,
                            uint32_t n) {
  pcie_msg_t msg;

  bf_sys_mutex_lock(&dma_socket_lock);

  msg.typ = pcie_op_dma_rd;
  msg.asic = asic;
  msg.addr = 0;
  msg.value = 0;
  msg.ind_addr = addr;
  msg.len = n;

  push_dma_from_dru(&msg, sizeof(msg), buf);

  bf_sys_mutex_unlock(&dma_socket_lock);
}

/** dru_rtn_pcie_rd_data_tcp
 *
 * return synchrouns read data
 */
void dru_rtn_pcie_rd_data_tcp(pcie_msg_t *msg, uint32_t value) {
  bf_sys_mutex_lock(&read_socket_lock);

  pcie_msg_t rtn_msg = *msg;
  rtn_msg.value = value;

  write_vpi_link(&rtn_msg);

  bf_sys_mutex_unlock(&read_socket_lock);
}

/** dru_next_pcie_msg_tcp
 *
 * return next msg from cpu or NULL.
 */
extern pcie_msg_t static_pcie_msg;
pcie_msg_t *dru_next_pcie_msg_tcp(void) {
  if (check_dru_ctrl_chnl((uint8_t *)&static_pcie_msg, sizeof(pcie_msg_t)) ==
      NULL) {
    return NULL;
  } else {
    return &static_pcie_msg;
  }
}

#endif  // TAGET_IS_LLD

/** dru_init_tcp
 *
 * return synchronous read data
 */
extern dru_intf_t g_dru_intf;

static dru_sim_dma2virt_dbg_callback_fn bf_mem_dma2virt_dbg = NULL;

void dru_init_tcp(int debug_mode,
                  int emu_integ,
                  int parallel_mode,
                  dru_write_dma_chnl_fn wr_dma_fn,
                  dru_read_dma_chnl_fn rd_dma_fn,
                  dru_write_vpi_link_fn wr_vpi_fn,
                  dru_push_to_dru_fn push_to_dru_fn,
                  dru_push_to_model_fn push_to_model_fn,
                  dru_sim_dma2virt_dbg_callback_fn dma2virt_fn) {
  dru_intf_t *dru_intf = &g_dru_intf;
#ifdef TARGET_IS_LLD
  dru_intf->cpu_to_pcie_rd = cpu_to_pcie_rd_tcp;
  dru_intf->cpu_to_pcie_wr = cpu_to_pcie_wr_tcp;
  dru_intf->dru_to_pcie_dma_rd = NULL;
  dru_intf->dru_to_pcie_dma_wr = NULL;
  dru_intf->dru_rtn_pcie_rd_data = NULL;
  dru_intf->dru_next_pcie_msg = NULL;
#else
  dru_intf->cpu_to_pcie_rd = NULL;
  dru_intf->cpu_to_pcie_wr = NULL;
  dru_intf->dru_to_pcie_dma_rd = dru_to_pcie_dma_rd_tcp;
  dru_intf->dru_to_pcie_dma_wr = dru_to_pcie_dma_wr_tcp;
  dru_intf->dru_rtn_pcie_rd_data = dru_rtn_pcie_rd_data_tcp;
  dru_intf->dru_next_pcie_msg = dru_next_pcie_msg_tcp;
#endif  // TARGET_IS_LLD

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

  if (bf_sys_mutex_init(&socket_lock) != 0) {
    assert(0);
  }
  if (bf_sys_mutex_init(&dma_socket_lock) != 0) {
    assert(0);
  }
  if (bf_sys_mutex_init(&read_socket_lock) != 0) {
    assert(0);
  }
  dru_init(emu_integ, parallel_mode);
}

void bug(void) {}

#ifndef TARGET_IS_MODEL

/**
 * Thread to service DMA read/writes from the DRU linked
 * into the model.
 */
extern void lld_log_dma_op(dru_dev_id_t chip,
                           int is_wr,
                           int len,
                           uint64_t addr);
extern uint64_t map_count;
void *dru_pcie_dma_service_thread_entry(void *arg) {
  pcie_msg_t pcie_msg, *msg = &pcie_msg;
  uint32_t *p_msg = (uint32_t *)&pcie_msg;
  uint32_t *p_data = NULL;
  bf_dma_addr_t dma_addr;
  void *vaddr;

  (void)arg;

  printf("dru_sim: DRU simulator running\n");

  while (1) {
    if (dru_sim_debug_mode) {
      memset((uint8_t *)&pcie_msg, 0xee, sizeof(pcie_msg));
    }

    read_dma_chnl((uint8_t *)msg, (int)sizeof(pcie_msg));

    if (dru_sim_debug_mode) {
      printf("pcie msg: %08x %08x %08x %08x %08x %08x\n",
             *(p_msg + 0),
             *(p_msg + 1),
             *(p_msg + 2),
             *(p_msg + 3),
             *(p_msg + 4),
             *(p_msg + 5));
    }
    if (msg->typ == pcie_op_dma_rd) {  // handle DMA rd from DRU

      dma_rds++;
      dma_rd_bytes += msg->len;

      dma_addr = msg->ind_addr;
      vaddr = bf_mem_dma2virt_dbg(dma_addr);

      if (dru_sim_debug_mode) {
        p_data = (uint32_t *)vaddr;

        printf(
            "DMA rd: msg=%p, addr=%p, len=%d\n", (void *)msg, vaddr, msg->len);
        printf("dma_buffer: %08x %08x %08x %08x %08x %08x\n",
               *(p_data + 0),
               *(p_data + 1),
               *(p_data + 2),
               *(p_data + 3),
               *(p_data + 4),
               *(p_data + 5));
      }
      lld_log_dma_op(0, 0 /*is_wr*/, msg->len, (uint64_t)(uintptr_t)vaddr);
      write_dma_chnl((uint8_t *)vaddr, msg->len);

    } else if (msg->typ == pcie_op_dma_wr) {  // handle DMA wr from DRU

      dma_wrs++;
      dma_wr_bytes += msg->len;

      dma_addr = msg->ind_addr;
      vaddr = bf_mem_dma2virt_dbg(dma_addr);

      /* Catch the pushed-pointer updates (and anything else that happens to be
       * eight bytes) and assign it as a u64 rather than writing it
       * byte-by-byte. */
      if ((int)msg->len == 8) {
        uint64_t x;
        read_dma_chnl((uint8_t *)&x, (int)msg->len);
        *(uint64_t *)vaddr = x;
      } else {
        read_dma_chnl((uint8_t *)vaddr, (int)msg->len);
      }

      lld_log_dma_op(0, 1 /*is_wr*/, msg->len, (uint64_t)(uintptr_t)vaddr);

      if (dru_sim_debug_mode) {
        p_data = (uint32_t *)vaddr;
        printf(
            "DMA wr: msg=%p, addr=%p, len=%d\n", (void *)msg, vaddr, msg->len);
        printf("dma_buffer: %08x %08x %08x %08x %08x %08x\n",
               *(p_data + 0),
               *(p_data + 1),
               *(p_data + 2),
               *(p_data + 3),
               *(p_data + 4),
               *(p_data + 5));
      }
    } else {
      printf("whoops, out of sync i guess\n");
      printf(
          "Fault: msg=%p, typ=%x, len=%d\n", (void *)msg, msg->typ, msg->len);
      bug();
      exit(1);
    }

    sched_yield();
  }
  return 0;
}

#endif
