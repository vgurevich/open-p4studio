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
 * @file pipe_mgr_drv.h
 * @date
 *
 * Internal definitions for pipe_mgr's interface to the Low Level Driver.
 */

#ifndef _PIPE_MGR_DRV_H
#define _PIPE_MGR_DRV_H

/* Standard includes */
#include <sys/types.h>
#include <stddef.h>

/* Module includes */
#include <lld/bf_dma_if.h>
#include <lld/lld_dr_if.h>
#include <lld/lld_subdev_dr_if.h>
extern void dru_hdl_pcie_fifo(void);

/* Local header files */
#include "pipe_mgr_int.h"
#include "pipe_mgr_drv_intf.h"
#include "pipe_mgr_learn.h"
#include "pipe_mgr_stat_lrt.h"

/* Defines for the minimum and maximum size of an instruction list entry. */
#define PIPE_MGR_DRV_ILIST_ENTRY_MIN_SZ (32 / 8)
#define PIPE_MGR_DRV_ILIST_ENTRY_MAX_SZ ((32 * 5) / 8)

/* Number of request FIFOs that Pipeline Manager uses. */
#define PIPE_MGR_DRV_FIFO_IDX_CNT 10

/* Callback functions for LLD FIFOs. */
void pipe_mgr_drv_completion_cb(bf_dev_id_t logical_device,
                                bf_subdev_id_t subdev_id,
                                bf_dma_dr_id_t fifo,
                                uint64_t s_or_t,
                                uint32_t attr,
                                uint32_t status,
                                uint32_t type,
                                uint64_t msgId,
                                int s,
                                int e);

void pipe_mgr_drv_buf_check(bool allFree);
void pipe_mgr_drv_dump_state(void);

pipe_status_t pipe_mgr_drv_get_dr_size(bf_dev_id_t devId);
pipe_status_t pipe_mgr_drv_prepare_device(pipe_sess_hdl_t shdl,
                                          rmt_dev_info_t *dev_info);

pipe_status_t pipe_mgr_drv_setup_fm_buffers(pipe_sess_hdl_t shdl,
                                            bf_dev_id_t devId);
pipe_status_t pipe_mgr_drv_load_fm_buffers(bf_dev_id_t devId,
                                           bf_dev_init_mode_t warm_init_mode);
void pipe_mgr_drv_cleanup_fm_buffers(bf_dev_id_t devId);

/* pipe_mgr_drv_device_cleanup
 * Called during the removal of a device to cleanup state
 */
pipe_status_t pipe_mgr_drv_device_cleanup(bf_dev_id_t devId);

/* A per-device structure to track the DMA buffers for that device. */
typedef struct pipe_mgr_drv_buf_pool_t {
  bf_sys_dma_pool_handle_t pool[PIPE_MGR_DRV_BUF_CNT];
  unsigned int buf_sz[PIPE_MGR_DRV_BUF_CNT];
  unsigned int buf_cnt[PIPE_MGR_DRV_BUF_CNT];

  /* Arrays of used buffers. */
  pipe_mgr_drv_buf_t *used[PIPE_MGR_DRV_BUF_CNT];

} pipe_mgr_drv_buf_pool_t;

struct pipe_mgr_drv_rd_blk_sub_op_t {
  pipe_mgr_drv_buf_t *buf;
  uint64_t msgId;
  uint32_t offset;
  uint32_t entryCount;
  bool done;
};
typedef struct pipe_mgr_drv_rd_blk_op_t {
  struct pipe_mgr_drv_rd_blk_op_t *next;
  struct pipe_mgr_drv_rd_blk_op_t *prev;
  struct pipe_mgr_drv_rd_blk_sub_op_t *sub_ops;
  int num_sub_ops;
  pipe_mgr_drv_rd_blk_cb cb_func;
  void *usrData;
} pipe_mgr_drv_rd_blk_op_t;
typedef struct pipe_mgr_drv_wr_blk_op_t pipe_mgr_drv_wr_blk_op_t;
struct pipe_mgr_drv_wr_blk_op_t {
  pipe_mgr_drv_wr_blk_op_t *next;
  pipe_mgr_drv_wr_blk_op_t *prev;
  pipe_mgr_drv_buf_t *buf;
  uint64_t msgId;
  uint64_t addr;
  uint32_t data_sz;
  uint8_t entry_sz;
  uint8_t addr_step;
  bool single_data;
};
typedef struct pipe_mgr_drv_list_op_t pipe_mgr_drv_list_op_t;
struct pipe_mgr_drv_list_op_t {
  pipe_mgr_drv_list_op_t *next;
  pipe_mgr_drv_list_op_t *prev;
  pipe_mgr_drv_buf_t *bufs[PIPE_MGR_NUM_DEVICES]
                          [BF_MAX_SUBDEV_COUNT];  // List of buffers per device
  uint32_t firstLock[PIPE_MGR_NUM_DEVICES][BF_MAX_SUBDEV_COUNT];  // Position of
                                                                  // first lock
                                                                  // in the
                                                                  // buffer
  uint16_t lockCnt[PIPE_MGR_NUM_DEVICES][BF_MAX_SUBDEV_COUNT];
  uint16_t instrCnt[PIPE_MGR_NUM_DEVICES][BF_MAX_SUBDEV_COUNT];
  uint16_t bufCnt;
  uint8_t firstLockStage[PIPE_MGR_NUM_DEVICES][BF_MAX_SUBDEV_COUNT];  // Stage
                                                                      // first
                                                                      // lock is
                                                                      // in
  uint8_t stage[PIPE_MGR_NUM_DEVICES][BF_MAX_SUBDEV_COUNT];
  uint8_t curDest[PIPE_MGR_NUM_DEVICES]
                 [BF_MAX_SUBDEV_COUNT];  // Bit mask of pipes
  bool needsAck;
  pipe_mgr_drv_ilist_cb cb_func;
  void *usrData;
};
struct pipe_mgr_drv_list_chkpt {
  /* Pointer to last verified buffer. */
  pipe_mgr_drv_buf_t *buf[PIPE_MGR_NUM_DEVICES][BF_MAX_SUBDEV_COUNT];
  /* State of the buffer and ilist that we are saving. */
  uint32_t used[PIPE_MGR_NUM_DEVICES][BF_MAX_SUBDEV_COUNT];
  uint32_t firstLock[PIPE_MGR_NUM_DEVICES][BF_MAX_SUBDEV_COUNT];
  uint16_t instrCnt[PIPE_MGR_NUM_DEVICES][BF_MAX_SUBDEV_COUNT];
  uint16_t lockCnt[PIPE_MGR_NUM_DEVICES][BF_MAX_SUBDEV_COUNT];
  uint16_t bufCnt;
  uint8_t firstLockStage[PIPE_MGR_NUM_DEVICES][BF_MAX_SUBDEV_COUNT];
  uint8_t stage[PIPE_MGR_NUM_DEVICES][BF_MAX_SUBDEV_COUNT];
  bool is_valid;
};

typedef struct pipe_mgr_drv_rd_list_op_t pipe_mgr_drv_rd_list_op_t;
struct pipe_mgr_drv_rd_list_op_t {
  pipe_mgr_drv_rd_list_op_t *next;
  pipe_mgr_drv_rd_list_op_t *prev;
  bf_dev_id_t dev_id;
  pipe_mgr_drv_buf_t **bufs;  // Array per pipe
  uint16_t bufCnt;
  uint8_t *stage;  // Array per pipe
  pipe_mgr_drv_rd_ilist_cb cb_func;
  void *usrData;
};
static inline void free_pipe_mgr_drv_rd_list_op_t(
    struct pipe_mgr_drv_rd_list_op_t *rd_op) {
  if (!rd_op) return;
  if (rd_op->bufs) PIPE_MGR_FREE(rd_op->bufs);
  if (rd_op->stage) PIPE_MGR_FREE(rd_op->stage);
  PIPE_MGR_FREE(rd_op);
}
static inline struct pipe_mgr_drv_rd_list_op_t *alloc_pipe_mgr_drv_rd_list_op_t(
    bf_dev_id_t dev_id) {
  struct pipe_mgr_drv_rd_list_op_t *rd_op = PIPE_MGR_CALLOC(1, sizeof *rd_op);
  if (!rd_op) return NULL;
  uint32_t p;
  rd_op->dev_id = dev_id;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    free_pipe_mgr_drv_rd_list_op_t(rd_op);
    return NULL;
  }
  rd_op->bufs =
      PIPE_MGR_CALLOC(dev_info->dev_cfg.num_pipelines, sizeof *rd_op->bufs);
  if (!rd_op->bufs) {
    free_pipe_mgr_drv_rd_list_op_t(rd_op);
    return NULL;
  }
  rd_op->stage =
      PIPE_MGR_CALLOC(dev_info->dev_cfg.num_pipelines, sizeof *rd_op->stage);
  if (!rd_op->stage) {
    free_pipe_mgr_drv_rd_list_op_t(rd_op);
    return NULL;
  }
  for (p = 0; p < dev_info->dev_cfg.num_pipelines; ++p) {
    rd_op->stage[p] = 0xFF;
  }
  return rd_op;
}

struct pipe_mgr_drv_ses_cntr_t {
  uint32_t rdBlkReq;
  uint32_t rdBlkRsp;
  uint32_t wrBlkReq;
  uint32_t wrBlkRsp;
  uint32_t rdReg;
  uint32_t wrRegReq;
  uint32_t wrRegRsp;
  uint32_t iListAdd;
  uint32_t iListPush;
  uint32_t iListPushBuf;
  uint32_t iListAbort;
  uint32_t iListRsp[BF_MAX_SUBDEV_COUNT][4];
  uint32_t iListRspBuf;
  uint32_t fifoRsvNoWait;
  uint32_t fifoRsvWait;
  uint32_t bufAllocSesNoWait;
  uint32_t bufAllocSesWait;
  uint32_t bufAllocGlbNoWait;
  uint32_t bufAllocGlbWait;
};

/* Snapshot of shadow physical memory data before txn start */
typedef struct pipe_mgr_pre_txn_mem_state_t pipe_mgr_pre_txn_mem_state_t;
struct pipe_mgr_pre_txn_mem_state_t {
  pipe_mgr_pre_txn_mem_state_t *next;
  pipe_mgr_pre_txn_mem_state_t *prev;
  pipe_mem_type_t mem_type;
  bf_dev_pipe_t pipe_id;
  uint8_t stage_id;
  mem_id_t mem_id;
  uint32_t line_num;
  uint8_t data[PIPE_MGR_MAU_WORD_WIDTH];
};

typedef struct pipe_mgr_drv_ses_state_t {
  /* Next message id this session will use when sending to LLD.  This is only
   * the lower portion of the id, the upper portion will be made of other
   * fields such as session id, device id, etc. */
  uint64_t msgId;
  /* The session id of this state block.  Basically the index into the driver
   * interface's array of session states stored in the global driver ctx. */
  uint8_t sid;
  /* Read Block operations are in list form because a client may start a new
   * operation before the completion from the previous operation has been
   * processed - particularly during hitless warm init. */
  pipe_mgr_drv_rd_blk_op_t *rdBlk;
  /* Write Block operations are in list form because a client may start a new
   * operation before the completion from the previous operation has been
   * processed. */
  pipe_mgr_drv_wr_blk_op_t *wrBlk;
  /* A single instruction list which the session is currently working on;
   * that is, the table management layer is still adding instructions to it
   * and it is not yet pushed to LLD. */
  pipe_mgr_drv_list_op_t *iListPending;
  /* Saves state for the pending ilist to record where the last successful
   * API's instructions end. */
  struct pipe_mgr_drv_list_chkpt iListPendingChkpt;
  pipe_mgr_drv_rd_list_op_t *iListRdPending;
  /* List of instruction lists this session has pushed to LLD but has not yet
   * cleaned up. */
  pipe_mgr_drv_list_op_t *iList;
  pipe_mgr_drv_rd_list_op_t *iListRd;
  /* Pending ilist used for locked devices during fast-reonfig */
  pipe_mgr_drv_list_op_t *iListReconfig[PIPE_MGR_NUM_DEVICES];
  /* Debug counters. */
  struct pipe_mgr_drv_ses_cntr_t cntrs;

  /* A mutex to protect this session state if multiple threads are working
   * on it.  For example, inserting a new instruction list while a completion
   * callback is removing a completed list. */
  pipe_mgr_mutex_t mtx_ses;
} pipe_mgr_drv_ses_state_t;

struct pipe_mgr_drv_ctx_t {
  pipe_mgr_drv_buf_pool_t gBufPool[PIPE_MGR_NUM_DEVICES][BF_MAX_SUBDEV_COUNT];
  pipe_mgr_drv_ses_state_t sesStates[PIPE_MGR_MAX_SESSIONS];
  pipe_mgr_drv_learn_cfg_t learn_cfg[PIPE_MGR_NUM_DEVICES][BF_MAX_SUBDEV_COUNT];
  pipe_mgr_drv_lrt_cfg_t lrt_cfg[PIPE_MGR_NUM_DEVICES][BF_MAX_SUBDEV_COUNT];
  bool valid[PIPE_MGR_NUM_DEVICES];
  pipe_mgr_mutex_t drv_ctx_mtx;
  int ilist_dr_size[PIPE_MGR_NUM_DEVICES][BF_MAX_SUBDEV_COUNT]
                   [4];  // One per ilistCmpltion DR
  int ilist_pending_cnt[PIPE_MGR_NUM_DEVICES][BF_MAX_SUBDEV_COUNT][4];  // One
                                                                        // per
                                                                        // ilist
                                                                        // DR
};

struct pipe_mgr_drv_ctx_t *pipe_mgr_drv_ctx();

pipe_status_t pipe_mgr_drv_push_learn_drs(bf_dev_id_t dev_id);

pipe_status_t pipe_mgr_drv_push_lrt_drs(bf_dev_id_t dev_id);

pipe_status_t pipe_mgr_drv_push_idle_time_drs(bf_dev_id_t dev_id);

pipe_mgr_drv_ses_state_t *pipe_mgr_drv_get_ses_state(pipe_sess_hdl_t *sess_hdl,
                                                     const char *where,
                                                     const int line);

pipe_sess_hdl_t pipe_mgr_get_int_sess_hdl(void);

uint32_t pipe_mgr_drv_ilist_locked_size_remaining(pipe_sess_hdl_t sess,
                                                  bf_dev_id_t dev_id);

#endif /* _PIPE_MGR_DRV_H */
