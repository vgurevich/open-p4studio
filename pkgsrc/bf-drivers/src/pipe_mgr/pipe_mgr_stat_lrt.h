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
 * @file pipe_mgr_stat_lrt.h
 * @date
 *
 *
 * Contains definitions relating to lr(t) message handling.
 */

#ifndef _PIPE_MGR_STAT_LRT_H
#define _PIPE_MGR_STAT_LRT_H

/* Global header includes */
#include <stdint.h>

/* Module header includes */
#include "bf_types/bf_types.h"
#include "pipe_mgr/pipe_mgr_err.h"
#include "lld/lld_bits.h"

/* Local header includes */
#include "pipe_mgr_stat_mgr_int.h"
#include "pipe_mgr_drv_intf.h"

/* Definitions for stat data message type */
typedef enum pipe_stat_data_msg_type_ {
  PIPE_STAT_MSG_TYPE_EVICT,
  PIPE_STAT_MSG_TYPE_LOCK_ACK,
  PIPE_STAT_MSG_TYPE_STAT_DUMP,
} pipe_stat_data_msg_type_t;

/* Structure definition for statistics data */
typedef struct pipe_mgr_stat_msg_data_ {
  uint64_t word0;
  uint64_t data;
} pipe_mgr_stat_msg_data_t;

/* Structure definition of stat ent address in the format in which
 * the hardware sends.
 */
typedef union rmt_stat_ent_addr_ {
  uint64_t addr;
  struct {
#if __BYTE_ORDER == __LITTLE_ENDIAN
    uint32_t stat_ent_virt_addr : 19; /* Stat entry virtual address */
    uint32_t log_tbl_id : 4;          /* Logical table id in this stage */
    uint32_t stage_id : 4;            /* Stage id */
    uint32_t pipe_id : 2;             /* Pipe id */
#else
    uint32_t reserved0 : 32;
    uint32_t reserved1 : 3;
    uint32_t pipe_id : 2;
    uint32_t stage_id : 4;
    uint32_t log_tbl_id : 4;
    uint32_t stat_ent_virt_addr : 19;
#endif
  } tof;
  struct {
    uint32_t stat_ent_virt_addr : 19; /* Stat entry virtual address */
    uint32_t log_tbl_id : 4;          /* Logical table id in this stage */
    uint32_t stage_id : 5;            /* Stage id */
    uint32_t pipe_id : 2;             /* Pipe id */
  } tof2;
  struct {
    uint32_t stat_ent_virt_addr : 19; /* Stat entry virtual address */
    uint32_t log_tbl_id : 4;          /* Logical table id in this stage */
    uint32_t stage_id : 5;            /* Stage id */
    uint32_t pipe_id : 4;             /* Pipe id */
  } tof3;
} rmt_stat_ent_addr_t;

typedef struct pipe_mgr_drv_lrt_cfg_ {
  uint32_t num_buffers;
  uint32_t buf_size;
  pipe_mgr_drv_buf_t **bufs;
  bf_phys_addr_t *buffer_array;
  uint32_t next_expected_buffer_idx;
  bf_sys_mutex_t lrt_buffer_array_mtx;
  bf_sys_cond_t lrt_buffer_array_condvar;
} pipe_mgr_drv_lrt_cfg_t;

static inline void extract_dr_stat_data_wd0(bf_dev_family_t dev_family,
                                            pipe_mgr_stat_msg_data_t *msg,
                                            pipe_stat_data_msg_type_t *msg_type,
                                            uint64_t *addr_or_lock_id) {
  if (!msg || !msg_type || !addr_or_lock_id) {
    LOG_ERROR("%s:%d Null pointer arguments passed", __func__, __LINE__);
    return;
  }

  switch (dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      *msg_type = msg->word0 & 0x3;
      *addr_or_lock_id = (msg->word0 >> 16) & 0x1FFFFFFF;
      return;
    case BF_DEV_FAMILY_TOFINO2:
      *msg_type = msg->word0 & 0x3;
      *addr_or_lock_id = (msg->word0 >> 16) & 0x3FFFFFFF;
      return;
    case BF_DEV_FAMILY_TOFINO3:
      *msg_type = msg->word0 & 0x3;
      *addr_or_lock_id = (msg->word0 >> 16) & 0xFFFFFFFF;
      return;

    case BF_DEV_FAMILY_UNKNOWN:
      break;
  }
  PIPE_MGR_DBGCHK(0);
  *msg_type = 0;
  *addr_or_lock_id = 0;
  return;
}

static inline void extract_dr_stat_data_lock_ack_addr(
    bf_dev_family_t dev_family,
    pipe_mgr_stat_msg_data_t *msg,
    int *pipe,
    dev_stage_t *stage,
    uint8_t *ltbl_id) {
  switch (dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      *ltbl_id = (msg->word0 >> (16 + 19)) & 0xF;
      *stage = (msg->word0 >> (16 + 19 + 4)) & 0xF;
      *pipe = (msg->word0 >> (16 + 19 + 4 + 4)) & 0x3;
      return;
    case BF_DEV_FAMILY_TOFINO2:
      *ltbl_id = (msg->word0 >> (16 + 19)) & 0xF;
      *stage = (msg->word0 >> (16 + 19 + 4)) & 0x1F;
      *pipe = (msg->word0 >> (16 + 19 + 4 + 5)) & 0x3;
      return;
    case BF_DEV_FAMILY_TOFINO3:
      *ltbl_id = (msg->word0 >> (16 + 19)) & 0xF;
      *stage = (msg->word0 >> (16 + 19 + 4)) & 0x1F;
      *pipe = (msg->word0 >> (16 + 19 + 4 + 5)) & 0xF;
      return;

    case BF_DEV_FAMILY_UNKNOWN:
      break;
  }
  PIPE_MGR_DBGCHK(0);
  *pipe = 0;
  *stage = 0;
  *ltbl_id = 0;
  return;
}

void pipe_mgr_drv_lrt_cb(bf_dev_id_t logical_device,
                         bf_subdev_id_t subdev_id,
                         int size,
                         bf_dma_addr_t dma_addr);

pipe_status_t pipe_mgr_drv_lrt_buf_init(pipe_sess_hdl_t h, bf_dev_id_t devId);
pipe_status_t pipe_mgr_drv_lrt_buf_warm_init_quick(bf_dev_id_t devId);
pipe_status_t pipe_mgr_lrt_buf_load(bf_dev_id_t devId);

pipe_status_t pipe_mgr_drv_lrt_buf_cleanup(bf_dev_id_t devId);

pipe_status_t pipe_mgr_stat_mgr_complete_operations(
    bf_dev_id_t device_id, pipe_stat_tbl_hdl_t stat_tbl_hdl);

typedef enum pipe_mgr_stat_mgr_dump_type_ {
  PIPE_MGR_STAT_DUMP_TYPE_PACKET_COUNT,
  PIPE_MGR_STAT_DUMP_TYPE_BYTE_COUNT
} pipe_mgr_stat_mgr_dump_type_e;

static inline pipe_mgr_stat_mgr_dump_type_e
pipe_mgr_stat_mgr_get_dump_type_from_addr(rmt_virt_addr_t virt_addr) {
  /* When a stat dump is issued for a stats table containing 64 bits of packet
   * count and 64 bits of byte count, the response comes in two messages, since
   * the stats dump message only contains 64 bits. The virtual address in the
   * response is used as an indicator of which message contains which count.
   * For the stats table format of 64bits Packet & byte counter, the address
   * has three bits of trailing ZEROs. Packet count comes with the virtual
   * address
   * of the entry, and the byte count comes with the virtual address of the
   * entry + 1.
   */
  PIPE_MGR_DBGCHK(((virt_addr & 0x7) == 0) || ((virt_addr & 0x7) == 4));

  if (virt_addr & 0x7) {
    return PIPE_MGR_STAT_DUMP_TYPE_BYTE_COUNT;
  }

  return PIPE_MGR_STAT_DUMP_TYPE_PACKET_COUNT;
}
#endif  // _PIPE_MGR_STAT_LRT_H
