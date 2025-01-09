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


#ifndef _PIPE_MGR_LEARN_H_
#define _PIPE_MGR_LEARN_H_

#include <target-utils/uCli/ucli.h>

#define PIPE_MGR_DEFAULT_LEARN_TIMEOUT 500  // usec
#define PIPE_MGR_NUM_LEARN_CFG_BITS 3
/* Number of learn types supported */
#define PIPE_MGR_NUM_LEARN_TYPES (1 << PIPE_MGR_NUM_LEARN_CFG_BITS)

/* Size of each learn quanta in bytes - 384 bits/ 48 bytes */
#define PIPE_MGR_LEARNQ_SIZE 48

/* Max number of learn quantas in a single push */
#define PIPE_MGR_TOF1_MAX_LEARNQ_BUF_SIZE 2048
#define PIPE_MGR_TOF2_MAX_LEARNQ_BUF_SIZE 4096
#define PIPE_MGR_TOF3_MAX_LEARNQ_BUF_SIZE 4096


#define PIPE_MGR_LEARN_FILTERS_PER_PIPE 2

#define TOFINO_CBC_CBUS_INT_STAT_LQ_RX_DR_EMPTY 0x80
#define TOFINO2_CBC_CBUS_INT_STAT_LQ_RX_DR_EMPTY 0x80000
#define TOFINO3_CBC_CBUS_INT_STAT_LQ_RX_DR_EMPTY 0x80000

typedef struct pipe_mgr_drv_learn_client_s_ {
  bool inuse;
  pipe_fld_lst_hdl_t fld_lst_hdl;
  pipe_flow_lrn_notify_cb cb_func;
  void *callback_fn_cookie;
  uint32_t num_msgs;
  pipe_flow_lrn_msg_t **msgs;  // Array of size num_msgs

} pipe_mgr_drv_learn_client_t;

typedef struct pipe_mgr_drv_lrn_buf_s {
  /* True when filter is in used, from first DMA buffer received until filter
   * ACK. */
  bool inuse;
  /* True when collecting all DMA buffers of an eviction, cleared once a buffer
   * with "last" set is received. */
  bool processing;
  /* True when sending digests to the client, cleared once all ACKs are received
   * and the filter is ready to be cleared. */
  bool waiting_for_ack;
  uint32_t count;
  // Array of nelems PIPE_MGR_MAX_LEARNQ_BUF_SIZE
  void *lrn_digest_entries[PIPE_MGR_NUM_LEARN_TYPES];
  uint32_t cur_usage[PIPE_MGR_NUM_LEARN_TYPES];
  /* Trailing bytes from a DMA buffer that needs to be appended to the next DMA
   * message
   */
  uint8_t trailing_bytes[PIPE_MGR_LEARNQ_SIZE];
  uint32_t c_trailing_bytes;
} pipe_mgr_drv_lrn_buf_t;

typedef struct pipe_mgr_drv_lrn_pipe_queue_s {
  /* Filters in HW can only be cleared in order, oldest to newest.  For example,
   * if we notified the client of data in filter A and then in filter B and the
   * client ACKed A we would immediately clear it, but if the client ACKed B
   * first and then A we must wait for the second ACK and then clear both
   * filters. */
  /* The cur variable tracks which filter is currently evicting.  Once a first
   * DMA buffer is received head is promoted to cur and then head is advanced.
   * When a middle or end DMA buffer is received cur is used to find which
   * filter we should store its data in.
   * The tail variable tracks the oldest filter we have.  When we initiate a
   * filter clear in HW we advance tail.
   * If head == tail then either both filters have evicted or neither has
   * evicted.  The inuse field on one of the filters (pipe_mgr_drv_lrn_buf_t)
   * can be checked to tell which case we are in. */
  uint32_t tail;
  uint32_t head;
  uint32_t cur;
  pipe_mgr_drv_lrn_buf_t lrn_buf[PIPE_MGR_LEARN_FILTERS_PER_PIPE];
} pipe_mgr_drv_lrn_pipe_queue_t;

enum bf_learn_filter_trace_type {
  BF_LRN_TRACE_INVALID = 0,
  BF_LRN_TRACE_FLTR_CLR,
  BF_LRN_TRACE_CB_SKIP,
  BF_LRN_TRACE_CB_CALL,
  BF_LRN_TRACE_CB_DONE,
  BF_LRN_TRACE_FLTR_ACK,
};

static inline const char *bf_learn_filter_trace_type_str(
    enum bf_learn_filter_trace_type t) {
  switch (t) {
    case BF_LRN_TRACE_INVALID:
      return "NONE";
    case BF_LRN_TRACE_FLTR_CLR:
      return "Fltr Clr";
    case BF_LRN_TRACE_CB_SKIP:
      return "Skip CB";
    case BF_LRN_TRACE_CB_CALL:
      return "Call CB";
    case BF_LRN_TRACE_CB_DONE:
      return "Rtrn CB";
    case BF_LRN_TRACE_FLTR_ACK:
      return "Fltr Ack";
  }
  return "UNKNOWN";
}

typedef struct bf_learn_filter_trace_t {
  enum bf_learn_filter_trace_type trace_type;
  struct timespec ts;
  /* Records the LQ type, useful for callback and ack records. */
  int lrn_type;
  /* Records the number of LQs in a learn buffer, useful for callback and ack
   * records. */
  int buf_cnt;
  /* Records the message pointer passed to the callback and passed in the ack.
   */
  void *ptr;
} bf_learn_filter_trace_t;

/* Learn config pipeline profile based */
typedef struct pipe_mgr_drv_learn_profile_s_ {
  profile_id_t prof_id;
  /* Flag to track whether a buffer should be discarded */
  bool discard;
  /* Whether a sess hdl is set or not. In tune with whether a client
   * is registered or not */
  bool learn_sess_hdl_set;
  /* Sess hdl which will be sent back to users in the cb so that it
   * can be reused for acks*/
  pipe_sess_hdl_t learn_sess_hdl;
  /* Learn clients*/
  pipe_mgr_drv_learn_client_t learn_clients[PIPE_MGR_NUM_LEARN_TYPES];
} pipe_mgr_drv_learn_profile_t;

/* Array of size PIPE_MGR_NUM_DEVICES of this struct is
 * kept in pipe_mgr_drv_ctx_t
 */
typedef struct pipe_mgr_drv_learn_cfg_s_ {
  /* To track number of pipeline profiles since
   * this struct is per device */
  uint32_t num_pipe_profiles;
  /* Buffers allocated for learning during init */
  uint32_t num_bufs;
  uint32_t buf_size;
  pipe_mgr_drv_buf_t **bufs;  // Array of pointers to buffers allocated

  /* Array of size of num_pipe_profiles.
   * Since profiles are kept serially indexed wrt their profile IDs
   * in rmt_dev_info_t, we can do the same here safely too*/
  pipe_mgr_drv_learn_profile_t *learn_profiles;

  uint32_t lrn_dr_tmo_usecs;

  /* num_pipes is the number of active logical pipes and is the length of the
   * lrn_pipe_q array.  The array is indexed by logical pipe. */
  uint32_t num_pipes;
  pipe_mgr_drv_lrn_pipe_queue_t *lrn_pipe_q;

  /* Per-device mutex to protect learning structures */
  pipe_mgr_mutex_t learn_mtx;

  /* Flag to determine endianness of returned digests */
  bool network_order;

  /* Flag to track whether interrupt-based learning is turned on */
  bool intr_learn;

  /* Trace of learn related events, there is a trace per logical pipe.
   * The mutex protects access to the trace to allow safe updates by multiple
   * threads.  The trace_idx keeps track of the location where the next record
   * will go in the per-pipe trace array. */
  pipe_mgr_mutex_t trace_mtx;
  unsigned int *trace_idx;
  bf_learn_filter_trace_t **trace;
#define PIPE_MGR_LEARN_TRACE_SIZE 32
#define PIPE_MGR_LEARN_TRACE_MASK 0x1F

} pipe_mgr_drv_learn_cfg_t;

pipe_status_t pipe_mgr_lrn_notification_register(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_fld_lst_hdl_t flow_lrn_fld_lst_hdl,
    pipe_flow_lrn_notify_cb callback_fn,
    void *callback_fn_cookie);

pipe_status_t pipe_mgr_lrn_notification_deregister(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_fld_lst_hdl_t flow_lrn_fld_lst_hdl);

pipe_status_t pipe_mgr_lrn_notify_ack(pipe_sess_hdl_t sess_hdl,
                                      pipe_fld_lst_hdl_t flow_lrn_fld_lst_hdl,
                                      pipe_flow_lrn_msg_t *pipe_flow_lrn_msg);

pipe_status_t pipe_mgr_lrn_set_timeout(bf_dev_id_t device_id, uint32_t usecs);
pipe_status_t pipe_mgr_lrn_get_timeout(bf_dev_id_t device_id, uint32_t *usecs);

pipe_status_t pipe_mgr_lrn_set_network_order_digest(bf_dev_id_t device_id,
                                                    bool network_order);

void pipe_mgr_drv_learn_cb(bf_dev_id_t logical_device,
                           bf_subdev_id_t subdev_id,
                           int size,
                           bf_dma_addr_t dma_addr,
                           int start,
                           int end,
                           bf_dev_pipe_t pipe);
pipe_status_t pipe_mgr_drv_learn_buf_init(pipe_sess_hdl_t h, bf_dev_id_t devId);
pipe_status_t pipe_mgr_learn_buf_load(bf_dev_id_t devId,
                                      bf_dev_init_mode_t warm_init_mode);
pipe_status_t pipe_mgr_drv_learn_buf_cleanup(bf_dev_id_t devId);

/* Return total count of learn DRs processed */
uint64_t pipe_mgr_flow_lrn_dr_count(bf_dev_id_t dev_id);

/* Reset count of learn DRs processed */
pipe_status_t pipe_mgr_flow_lrn_dr_count_reset(bf_dev_id_t dev_id);

/* Enable or disable learn DR interrupt processing */
pipe_status_t pipe_mgr_flow_lrn_int_enable(bf_dev_id_t dev_id, bool en);

bool pipe_mgr_flow_lrn_is_int_enabled(bf_dev_id_t dev_id);

#if PIPE_MGR_CONFIG_INCLUDE_UCLI == 1
ucli_node_t *pipe_mgr_learn_ucli_node_create(ucli_node_t *n);
#else
void *pipe_mgr_learn_ucli_node_create(void);
#endif

#endif
