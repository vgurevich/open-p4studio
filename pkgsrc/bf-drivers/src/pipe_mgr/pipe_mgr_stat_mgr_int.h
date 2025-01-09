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
 * @file pipe_mgr_stats_mgr_int.h
 * @date
 *
 *
 * Contains internal definitions relating to pipe mgr's statistics table
 *management
 */

#ifndef _PIPE_MGR_STATS_MGR_INT_H
#define _PIPE_MGR_STATS_MGR_INT_H

/* Global header includes */
#include <time.h>

/* Module header includes */
#include <target-utils/id/id.h>
#include <target-utils/map/map.h>

/* Local header includes */
#include "pipe_mgr/pipe_mgr_intf.h"
#include "pipe_mgr_int.h"
#include "pipe_mgr_stats_tbl.h"

#define PIPE_MGR_STAT_TRACE(_device_id_,                \
                            _tbl_hdl_,                  \
                            _tbl_name_,                 \
                            _ent_idx_,                  \
                            _stage_ent_idx_,            \
                            _pipe_id_,                  \
                            _stage_id_,                 \
                            _str_,                      \
                            ...)                        \
  LOG_TRACE("STAT %s:%d %d:%s:0x%x:%d:%d:%d:%d " _str_, \
            __func__,                                   \
            __LINE__,                                   \
            _device_id_,                                \
            _tbl_name_,                                 \
            _tbl_hdl_,                                  \
            _ent_idx_,                                  \
            _stage_ent_idx_,                            \
            _pipe_id_,                                  \
            _stage_id_,                                 \
            __VA_ARGS__)

/* Structure definition for stat ram alloc info */
typedef struct pipe_mgr_stat_ram_alloc_info_t {
  /* Number of wide-word blocks allocated */
  uint8_t num_wide_word_blks;
  /* An array containing RAM ids and VPN per wide-word blocks */
  rmt_tbl_word_blk_t *tbl_word_blk;

} pipe_mgr_stat_ram_alloc_info_t;

/* Structure definition of stage level info for a stats table */
typedef struct pipe_mgr_stat_tbl_stage_info_t {
  /* Ram alloc info */
  pipe_mgr_stat_ram_alloc_info_t *stat_ram_alloc_info;
  /* Number of entries in this stage */
  uint32_t num_entries;
  /* Entry index offset -> Quantity which should be added to an index in this
   * stage to get the global statistics entry index.  */
  uint32_t ent_idx_offset;
  /* A boolean value to indicate if the stat table stage is locked. */
  bool locked;
  /* The lock id with which the stage is locked */
  lock_id_t lock_id;
  /* Stage id  */
  dev_stage_t stage_id;
  /* Number of stat entries per RAM line */
  uint8_t num_entries_per_line;
  /* Stage table handle */
  uint8_t stage_table_handle;
} pipe_mgr_stat_tbl_stage_info_t;

/* Structure definition for per-entry index info */
typedef struct pipe_mgr_stat_entry_info_t {
  /* Accumulated packet and byte counter values */
  pipe_stat_data_t stat_data;
  pipe_mgr_mutex_t mtx;
  uint32_t user_set_in_progress;
} pipe_mgr_stat_entry_info_t;

/* Structure definition for per entry index info */
typedef struct pipe_mgr_stat_ent_idx_info_t {
  pipe_mgr_stat_entry_info_t entry_info;
} pipe_mgr_stat_ent_idx_info_t;

typedef struct pipe_mgr_stat_lrt_dbg_info_t {
  uint32_t num_lrt_evicts_rcvd;
  pipe_stat_data_t stat_data;
} pipe_mgr_stat_lrt_dbg_info_t;

typedef struct pipe_mgr_stat_mgr_task_type_move_t {
  pipe_mat_ent_hdl_t mat_ent_hdl;
  dev_stage_t src_stage_id;
  dev_stage_t dst_stage_id;
  pipe_stat_stage_ent_idx_t src_ent_idx;
  pipe_stat_stage_ent_idx_t dst_ent_idx;
} pipe_mgr_stat_mgr_task_type_move_t;

typedef struct pipe_mgr_stat_mgr_task_type_ent_add_t {
  pipe_stat_stage_ent_idx_t stage_ent_idx;
  pipe_mat_ent_hdl_t mat_ent_hdl;
  dev_stage_t stage_id;
} pipe_mgr_stat_mgr_task_type_ent_add_t;

typedef struct pipe_mgr_stat_mgr_task_type_ent_del_t {
  pipe_mat_ent_hdl_t ent_hdl;
  pipe_stat_stage_ent_idx_t stage_ent_idx;
  dev_stage_t stage_id;
} pipe_mgr_stat_mgr_task_type_ent_del_t;

typedef enum pipe_mgr_stat_mgr_task_type_t {
  PIPE_MGR_STAT_TASK_MOVE,
  PIPE_MGR_STAT_TASK_ENTRY_ADD,
  PIPE_MGR_STAT_TASK_ENTRY_DEL,
} pipe_mgr_stat_mgr_task_type_e;

static inline const char *pipe_mgr_stat_mgr_task_type_str(
    pipe_mgr_stat_mgr_task_type_e e) {
  switch (e) {
    case PIPE_MGR_STAT_TASK_MOVE:
      return "Move";
    case PIPE_MGR_STAT_TASK_ENTRY_ADD:
      return "Add";
    case PIPE_MGR_STAT_TASK_ENTRY_DEL:
      return "Del";
  }
  return "Unknown";
}

typedef struct pipe_mgr_stat_mgr_task_node_t {
  struct pipe_mgr_stat_mgr_task_node_t *next;
  struct pipe_mgr_stat_mgr_task_node_t *prev;
  pipe_mgr_stat_mgr_task_type_e type;
  union {
    pipe_mgr_stat_mgr_task_type_move_t move_node;
    pipe_mgr_stat_mgr_task_type_ent_add_t ent_add_node;
    pipe_mgr_stat_mgr_task_type_ent_del_t ent_del_node;
  } u;
} pipe_mgr_stat_mgr_task_node_t;

/* Structure definition for stat entry dump barrier state */
typedef struct pipe_mgr_stat_barrier_entry_dump_t {
  pipe_stat_ent_idx_t ent_idx; /* Entry index for which the dump was issued */
  bf_dev_pipe_t pipe_id;       /* Pipe id for which the dump was issued
                                * BF_DEV_PIPE_ALL, if it was issued to all pipes
                                */
  /* Match table and entry handles of the match entry for which the entry dump
   * was issued.  */
  pipe_mat_tbl_hdl_t mat_tbl_hdl;
  pipe_mat_ent_hdl_t mat_ent_hdl;
} pipe_mgr_stat_barrier_entry_dump_t;

/* Structure definition for stat table dump barrier state */
typedef struct pipe_mgr_stat_barrier_tbl_dump_t {
  bf_dev_pipe_t pipe_id; /* Pipe id for which the dump was issued
                          * BF_DEV_PIPE_ALL, if it was issued to all pipes
                          */
  /* The callback function which needs to be invoked upon processing this
   * barrier ACK.
   */
  pipe_mgr_stat_tbl_sync_cback_fn callback_fn;
  void *user_cookie;

} pipe_mgr_stat_barrier_tbl_dump_t;

/* Structure definition for entry write op */
typedef struct pipe_mgr_stat_barrier_ent_write_t {
  bf_dev_pipe_t pipe_id;
  pipe_stat_ent_idx_t ent_idx;
} pipe_mgr_stat_barrier_ent_write_t;

/* Definition for stat table operations to be recorded in the barrier state */
typedef enum pipe_mgr_stat_mgr_operation_t {
  PIPE_MGR_STAT_ENTRY_DUMP_OP,
  PIPE_MGR_STAT_TBL_DUMP_OP,
  PIPE_MGR_STAT_ENT_WRITE_OP,
  PIPE_MGR_STAT_LOCK_OP,
  PIPE_MGR_STAT_UNLOCK_OP,
} pipe_mgr_stat_mgr_operation_t;
static inline const char *pipe_mgr_stat_mgr_operation_str(
    pipe_mgr_stat_mgr_operation_t op) {
  switch (op) {
    case PIPE_MGR_STAT_ENTRY_DUMP_OP:
      return "EntDump";
    case PIPE_MGR_STAT_TBL_DUMP_OP:
      return "TblDump";
    case PIPE_MGR_STAT_ENT_WRITE_OP:
      return "Write";
    case PIPE_MGR_STAT_LOCK_OP:
      return "Lock";
    case PIPE_MGR_STAT_UNLOCK_OP:
      return "Unlock";
  }
  return "Unknown";
}

typedef struct pipe_mgr_stat_barrier_state_t {
  /* A bitmap of pipes for which we are expecting responses from. */
  int pipe_ref_map;
  /* The stage ID to which the operation was sent. */
  dev_stage_t stage_id;
  /* The lock id or barrier id associated with the operation. */
  lock_id_t lock_id;
  /* Identifies the type of barrier this is. */
  pipe_mgr_stat_mgr_operation_t operation;
  union {
    /* The following are associated with a barrier operation */
    pipe_mgr_stat_barrier_entry_dump_t entry_dump;
    pipe_mgr_stat_barrier_tbl_dump_t tbl_dump;
    pipe_mgr_stat_barrier_ent_write_t ent_write;
    /* The lock and unlock operations do not require additional state. */
  } op_state;
} pipe_mgr_stat_barrier_state_t;

typedef struct pipe_mgr_stat_deferred_dump_t {
  struct pipe_mgr_stat_deferred_dump_t *next;
  struct pipe_mgr_stat_deferred_dump_t *prev;
  pipe_stat_ent_idx_t stat_ent_idx;
  pipe_stat_data_t stat_data;
} pipe_mgr_stat_deferred_dump_t;

typedef struct pipe_mgr_stat_barrier_list_node_t {
  pipe_mgr_stat_barrier_state_t *barrier_state;
  pipe_mgr_stat_mgr_task_node_t *task_list;
  pipe_mgr_stat_deferred_dump_t *dump_list;
  struct pipe_mgr_stat_barrier_list_node_t *next;
  struct pipe_mgr_stat_barrier_list_node_t *prev;
  bool received_hw_ack;
} pipe_mgr_stat_barrier_list_node_t;

typedef struct pipe_mgr_stat_ent_location_t {
  struct pipe_mgr_stat_ent_location_t *next;
  struct pipe_mgr_stat_ent_location_t *prev;
  bf_dev_pipe_t pipe_id;
  pipe_stat_stage_ent_idx_t cur_ent_idx;
  pipe_stat_stage_ent_idx_t def_ent_idx;
  uint32_t def_set_in_prog;
  bool pending;
  bool entry_del_in_progress;
  dev_stage_t cur_stage_id; /* Current stage id */
  dev_stage_t def_stage_id; /* Deferred stage id */
  /* cur_stage_id/cur_ent_idx: where the entry actually is.
   * def_stage_id/def_ent_idx: historical location, will catch up to cur_* after
   * HW notifications are processed. */
} pipe_mgr_stat_ent_location_t;

/* Structure defintion of entry handle location info */
typedef struct pipe_mgr_stat_mgr_ent_hdl_loc_t {
  /* User set value of the counter */
  pipe_stat_data_t stat_data;
  /* Entry index where the entry handle is located in */
  pipe_mgr_stat_ent_location_t *locations;
} pipe_mgr_stat_mgr_ent_hdl_loc_t;

enum pipe_mgr_stat_trace_op_type {
  BF_STAT_TRACE_TYPE_INVALID = 0,
  BF_STAT_TRACE_TYPE_ADD,
  BF_STAT_TRACE_TYPE_DEF_ADD,
  BF_STAT_TRACE_TYPE_DEL,
  BF_STAT_TRACE_TYPE_DEF_DEL,
  BF_STAT_TRACE_TYPE_MOV,
  BF_STAT_TRACE_TYPE_DEF_MOV,
  BF_STAT_TRACE_TYPE_BAR,
  BF_STAT_TRACE_TYPE_BAR_ACK,
};
typedef struct pipe_mgr_stat_tbl_trace_dir_api_t {
  pipe_mat_ent_hdl_t ent_hdl;
  pipe_stat_stage_ent_idx_t stage_idx;
  dev_stage_t stage_id;
  pipe_stat_stage_ent_idx_t src_stage_idx;
  dev_stage_t src_stage_id;
  uint32_t deferred;
} pipe_mgr_stat_tbl_trace_dir_api_t;
typedef struct pipe_mgr_stat_tbl_trace_task_t {
  pipe_mat_ent_hdl_t ent_hdl;
  pipe_stat_stage_ent_idx_t stage_idx;
  pipe_stat_stage_ent_idx_t src_stage_idx;
  bf_dev_pipe_t pipe_id;
  dev_stage_t stage_id;
  dev_stage_t src_stage_id;
} pipe_mgr_stat_tbl_trace_task_t;
typedef struct pipe_mgr_stat_tbl_trace_bar_ack_t {
  bf_dev_pipe_t pipe_id;
  dev_stage_t stage_id;
  lock_id_t lock_id;
  uint8_t deferred;
} pipe_mgr_stat_tbl_trace_bar_ack_t;
typedef struct pipe_mgr_stat_tbl_trace_entry_t {
  struct timespec ts;
  union {
    pipe_mgr_stat_tbl_trace_dir_api_t dir_api;
    pipe_mgr_stat_tbl_trace_task_t task;
    pipe_mgr_stat_barrier_state_t bar;
    pipe_mgr_stat_tbl_trace_bar_ack_t b_ack;
  } u;
  uint8_t op;
} pipe_mgr_stat_tbl_trace_entry_t;

/* Structure definition of a managed statistic table instance */
typedef struct pipe_mgr_stat_tbl_instance_t {
  /* Pipe id of this managed instance. BF_DEV_PIPE_ALL if
   * a single instance exists for all pipes */
  bf_dev_pipe_t pipe_id;
  /* Same as pipe_id except in the PIPE_ALL case, then it is the first pipe the
   * table belongs to. */
  bf_dev_pipe_t lowest_pipe_id;
  /* The set of pipes managed by this instance. */
  pipe_bitmap_t pipe_bmp;
  /* Number of entries allocated in this instance */
  uint32_t num_entries;
  /* Capacity of the instance; the size as defined in P4. */
  uint32_t capacity;
  /* Next location to use in the debug trace. */
  uint32_t trace_idx;
  /* Pointer to a trace of events. */
  pipe_mgr_stat_tbl_trace_entry_t *trace;
  pipe_mgr_mutex_t trace_mtx;
#define PIPE_MGR_STAT_TRACE_BITS 10
#define PIPE_MGR_STAT_TRACE_SIZE (1u << PIPE_MGR_STAT_TRACE_BITS)
#define PIPE_MGR_STAT_TRACE_MASK (PIPE_MGR_STAT_TRACE_SIZE - 1)

  /* An id allocator (incrementing count) for barrier ID generation */
  uint16_t next_barrier_id;

  /* Number of pipe-line stages in which the table is present */
  uint8_t num_stages;
  /* Array of stage level info */
  pipe_mgr_stat_tbl_stage_info_t *stat_tbl_stage_info;

  /* Array of per-index info, per-pipe and per-stage where this managed
   * instance of the stat table is present.  The first array dimension is the
   * logical pipe, the second is the stage, and the third is the index of an
   * entry within the stage. */
  pipe_mgr_stat_ent_idx_info_t ***ent_idx_info;
  /* An array of count, one per entry index to hold the user set count. When
   * queried for a counter index, the summation of the user set count along
   * with the live data is done to return the resulting count.
   * This is only for indirectly addressed counter tables.
   */
  pipe_stat_data_t *user_idx_count;

  /* Barrier state data per-logical pipe */
  pipe_mgr_mutex_t barrier_data_mtx;
  pipe_mgr_stat_barrier_list_node_t **barrier_list;
  /* The deferred barrier node is per logical pipe per stage and a pointer into
   * the barrier_list.  In the event that stage B's notifications come ahead of
   * stage A's the processing of B's notifications will be deferred until the
   * notifications from stage A are processed.  In this case the
   * def_barrier_node for stage B will point to the barrier_list node for the
   * dependent notifications from stage A. */
  pipe_mgr_stat_barrier_list_node_t ***def_barrier_node;

  /* A map which maps entry handle to location. This is applicable
   * only for direct addressed stat tables, since the entries move around and
   * the software move can potentially be deferred pending a response from
   * the hardware. A query on the count (software value) should still read
   * from the older location, since the move has not yet been done.
   * key is entry handle, data is pipe_mgr_stat_mgr_ent_hdl_loc_t which store a
   * linked list of pipe_mgr_stat_ent_location_t objects, oldest at the head and
   * newest at the tail. */
  bf_map_t ent_hdl_loc;
  pipe_mgr_mutex_t ent_hdl_loc_mtx;

  pipe_mgr_stat_lrt_dbg_info_t ***ent_idx_lrt_dbg_info;
} pipe_mgr_stat_tbl_instance_t;

typedef struct pipe_mgr_stat_tbl_t {
  /* Name of the table */
  char *name;
  pipe_tbl_dir_t direction;
  /* LR(t) enable : For 64 bit resolution, hardware LR(t) is disabled
   * since 64 bit resolution has no possibility of overflow.  */
  bool lrt_enabled;
  /* A count of number of LR(t) evicts received on this table */
  uint64_t num_lrt_evicts_rcvd;
  /* Device id with which the table is associated with */
  bf_dev_id_t device_id;
  /* Pointer to the dev info */
  rmt_dev_info_t *dev_info;
  /* Profile id with which the table is associated with */
  profile_id_t profile_id;
  /* Pipe bitmap indicating the association of this table with
   * one or many Pipes in the device.  */
  pipe_bitmap_t pipe_bmp;
  /* A flag indicating whether the table is symmetric or not */
  bool symmetric;
  /* Scope info */
  scope_num_t num_scopes;
  scope_pipes_t *scope_pipe_bmp;
  /* Table handle */
  pipe_stat_tbl_hdl_t stat_tbl_hdl;
  /* Counter type : Byte, packet or Byte and packet */
  pipe_stat_type_t counter_type;
  /* Resolution of the byte counter */
  uint32_t byte_counter_resolution;
  /* Resolution of the packet counter */
  uint32_t packet_counter_resolution;
  /* Number of managed instances of the table */
  uint8_t num_instances;
  /* Array of managed instances */
  pipe_mgr_stat_tbl_instance_t *stat_tbl_instances;
  /* How this stat table entry is referred to */
  pipe_tbl_ref_type_t ref_type;
  /* A flag indicating if the table has been over-allocated */
  bool over_allocated;
  bool enable_per_flow_enable;
  uint32_t per_flow_enable_bit_position;
} pipe_mgr_stat_tbl_t;

typedef struct pipe_mgr_stat_ent_worklist_t {
  dev_stage_t stage_id;
  pipe_stat_stage_ent_idx_t entry_idx;
  struct pipe_mgr_stat_ent_worklist_t *next;
  struct pipe_mgr_stat_ent_worklist_t *prev;
} pipe_mgr_stat_ent_worklist_t;

typedef struct pipe_mgr_stat_ent_read_output_t {
  bf_dev_pipe_t pipe_id;
  dev_stage_t stage_id;
  uint64_t data0;
  uint64_t data1;
  struct pipe_mgr_stat_ent_read_output_t *next;
  struct pipe_mgr_stat_ent_read_output_t *prev;
} pipe_mgr_stat_ent_read_output_t;

/* Function prototypes */

pipe_status_t pipe_mgr_stat_mgr_stat_ent_database_sync(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_stat_tbl_hdl_t stat_tbl_hdl,
    pipe_stat_ent_idx_t stat_ent_idx);

pipe_mgr_stat_tbl_instance_t *pipe_mgr_stat_tbl_get_instance(
    pipe_mgr_stat_tbl_t *stat_tbl, bf_dev_pipe_t pipe_id);

pipe_mgr_stat_tbl_instance_t *pipe_mgr_stat_tbl_get_instance_any_pipe(
    pipe_mgr_stat_tbl_t *stat_tbl, bf_dev_pipe_t pipe_id);

pipe_mgr_stat_tbl_t *pipe_mgr_stat_tbl_get(bf_dev_id_t device_id,
                                           pipe_stat_tbl_hdl_t stat_tbl_hdl);

pipe_mgr_stat_tbl_stage_info_t *pipe_mgr_stat_mgr_get_stage_info(
    pipe_mgr_stat_tbl_t *stat_tbl, bf_dev_pipe_t pipe_id, uint8_t stage_id);

rmt_virt_addr_t pipe_mgr_stat_mgr_compute_ent_virt_addr(
    pipe_mgr_stat_tbl_stage_info_t *stat_tbl_stage_info,
    pipe_stat_stage_ent_idx_t stage_ent_idx);

void pipe_mgr_stat_mgr_ent_free_worklist(
    pipe_mgr_stat_ent_worklist_t *worklist);

void pipe_mgr_stat_mgr_free_ent_read_output(
    pipe_mgr_stat_ent_read_output_t *output);

void pipe_mgr_stat_mgr_encode_stat_entry(
    pipe_mgr_stat_tbl_t *stat_tbl,
    pipe_mgr_stat_tbl_stage_info_t *stat_tbl_stage_info,
    dev_stage_t stage_id,
    uint8_t subword,
    uint64_t byte_counter,
    uint64_t packet_counter,
    rmt_ram_line_t *ram_line);

pipe_status_t pipe_mgr_stat_mgr_decode_stat_entry(pipe_mgr_stat_tbl_t *stat_tbl,
                                                  bf_dev_pipe_t pipe_id,
                                                  uint8_t stage_id,
                                                  uint8_t subword,
                                                  uint64_t *byte_counter,
                                                  uint64_t *packet_counter,
                                                  uint64_t val0,
                                                  uint64_t val1);

void pipe_mgr_stat_mgr_add_barrier_state(
    pipe_mgr_stat_tbl_t *stat_tbl,
    pipe_mgr_stat_tbl_instance_t *stat_tbl_instance,
    pipe_bitmap_t *pipe_bmp,
    pipe_mgr_stat_barrier_state_t *barrier_state);

void pipe_mgr_stat_tbl_lkup(bf_dev_id_t device_id,
                            uint8_t ltbl_id,
                            bf_dev_pipe_t pipe_id,
                            dev_stage_t stage_id,
                            pipe_mgr_stat_tbl_t **stat_tbl,
                            pipe_mgr_stat_tbl_instance_t **stat_tbl_inst);

pipe_status_t pipe_mgr_stat_mgr_get_ent_idx_from_virt_addr(
    pipe_mgr_stat_tbl_t *stat_tbl,
    rmt_virt_addr_t virt_addr,
    bf_dev_pipe_t pipe_id,
    dev_stage_t stage_id,
    pipe_stat_ent_idx_t *stat_ent_idx);

pipe_status_t pipe_mgr_stat_mgr_set_ent_idx_count(
    pipe_mgr_stat_tbl_t *stat_tbl,
    pipe_mgr_stat_tbl_instance_t *stat_tbl_instance,
    bool set_in_prog,
    bf_dev_pipe_t pipe_id,
    dev_stage_t stage_id,
    pipe_stat_stage_ent_idx_t stat_ent_idx,
    pipe_stat_data_t *stat_data);

pipe_status_t pipe_mgr_stat_mgr_incr_ent_idx_count(
    pipe_mgr_stat_tbl_t *stat_tbl,
    pipe_mgr_stat_tbl_instance_t *stat_tbl_instance,
    bf_dev_pipe_t pipe_id,
    dev_stage_t stage_id,
    pipe_stat_stage_ent_idx_t stage_ent_idx,
    pipe_stat_data_t *stat_data);

pipe_status_t pipe_mgr_stat_mgr_get_ent_idx_count(
    pipe_mgr_stat_tbl_t *stat_tbl,
    pipe_mgr_stat_tbl_instance_t *stat_tbl_instance,
    bf_dev_pipe_t pipe_id,
    dev_stage_t stage_id,
    pipe_stat_stage_ent_idx_t stage_ent_idx,
    pipe_stat_data_t *stat_data);

void pipe_mgr_stat_mgr_execute_task_list(
    pipe_mgr_stat_tbl_t *stat_tbl,
    pipe_mgr_stat_tbl_instance_t *stat_tbl_instance,
    bf_dev_pipe_t pipe_id,
    pipe_mgr_stat_mgr_task_node_t *head);

pipe_status_t pipe_mgr_stat_mgr_ent_set(pipe_sess_hdl_t sess_hdl,
                                        dev_target_t dev_tgt,
                                        pipe_stat_tbl_hdl_t stat_tbl_hdl,
                                        pipe_stat_ent_idx_t stat_ent_idx,
                                        pipe_stat_data_t *stat_data);

void pipe_mgr_stat_mgr_destroy_ent_hdl_loc(
    pipe_mgr_stat_tbl_instance_t *stat_tbl_instance);

void pipe_mgr_stat_mgr_reset_ent_write_in_progress(
    pipe_mgr_stat_tbl_t *stat_tbl,
    pipe_mgr_stat_tbl_instance_t *stat_tbl_instance,
    bf_dev_pipe_t pipe_id,
    dev_stage_t stage_id,
    pipe_stat_stage_ent_idx_t stat_ent_idx);

void pipe_mgr_stat_mgr_update_tbl_hdl(bf_dev_id_t dev_id,
                                      bf_dev_pipe_t pipe_id,
                                      dev_stage_t stage_id,
                                      uint8_t ltbl_id,
                                      pipe_mgr_stat_tbl_t *stat_tbl);

uint8_t pipe_mgr_stat_mgr_ent_get_stage(
    pipe_mgr_stat_tbl_t *stat_tbl,
    pipe_mgr_stat_tbl_instance_t *stat_tbl_instance,
    pipe_stat_ent_idx_t stat_ent_idx,
    dev_stage_t stage_id,
    pipe_stat_stage_ent_idx_t *stage_ent_idx);

#endif
