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
 * @file pipe_mgr_idle.h
 * @date
 *
 * Idle time related definitions of pipeline manager
 */

#ifndef _PIPE_MGR_IDLE_H
#define _PIPE_MGR_IDLE_H

#include <target-utils/uCli/ucli.h>
#include <target-utils/third-party/cJSON/cJSON.h>
#include <target-sys/bf_sal/bf_sys_timer.h>

#include "pipe_mgr_int.h"
#include "pipe_mgr_drv.h"

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Designing for threading - notes
 *
 * The structures managed by idle timer manager (below) can potentially
 * be accessed from 3 different threads.
 * - API thread, handles all the APIs
 * - Software timer thread - one entry point - pipe_mgr_idle_sw_timer_cb()
 * - Tofino notification thread - one entry point - pipe_mgr_drv_idle_cb()
 * However due to the isolation of the different APIs, the level of
 * locking needed is minimal. When any new structures are added, a careful
 * analysis needs to be done to figure out if locking is needed
 *
 * - The enable/disable APIs create/delete the idle_tbls. So these 2 APIs need
 *   to take a write lock and the other threads need to make sure this
 *   operation is not going on. A RW lock is needed.
 *   This is a rare operation, so should be ok.
 * - All other existing state modification APIs add things down into the
 *   task_list. So task_list has to be locked. In the future it can be
 *   changed to a lock-free queue.
 * - Between the timer thread and the notification thread, exclusive access
 *   is needed for the pending_msgs list. Can be changed to a lock-free queue
 * - The entry states (idle_entry_t) has no concurrent write access. For
 *   all operations, the API thread just updates the task_list.
 *
 * The 2 maps maintained at both the table level and the stage level need RW
 * lock
 */

typedef uint64_t idle_dr_msg_t;

typedef enum idle_time_msg_type_e {
  IDLE_FSM_MSG = 0,
  IDLE_LOCK_ACK_MSG = 1,
  IDLE_DUMP_MSG = 2,
  IDLE_MSG_INVALID
} idle_time_msg_type_e;

#define NONZERO_TTL 50

static inline idle_time_msg_type_e extract_idle_type(bf_dev_family_t dev_fam,
                                                     idle_dr_msg_t *msg) {
  switch (dev_fam) {
    case BF_DEV_FAMILY_TOFINO:
      return *msg & 0x3;
    case BF_DEV_FAMILY_TOFINO2:
    case BF_DEV_FAMILY_TOFINO3:
      return *msg & 0x3;

    case BF_DEV_FAMILY_UNKNOWN:
      PIPE_MGR_DBGCHK(0);
      break;
  }
  return IDLE_MSG_INVALID;
}

static inline void extract_idle_fsm_msg(bf_dev_family_t dev_fam,
                                        idle_dr_msg_t *msg,
                                        uint32_t *data,
                                        int *pipe,
                                        int *stage,
                                        int *logical_tbl,
                                        int *word) {
  switch (dev_fam) {
    case BF_DEV_FAMILY_TOFINO:
      *data = (*msg >> 4) & 0xFF;
      *word = (*msg >> 36) & 0xFFFF;
      *logical_tbl = (*msg >> (36 + 16)) & 0xF;
      *stage = (*msg >> (36 + 16 + 4)) & 0xF;
      *pipe = (*msg >> (36 + 16 + 4 + 4)) & 0x3;
      return;
    case BF_DEV_FAMILY_TOFINO2:
    case BF_DEV_FAMILY_TOFINO3:
      *data = (*msg >> 4) & 0xFF;
      *word = (*msg >> 36) & 0xFFFF;
      *logical_tbl = (*msg >> (36 + 16)) & 0xF;
      *stage = (*msg >> (36 + 16 + 4)) & 0x1F;
      *pipe = (*msg >> (36 + 16 + 4 + 5)) & 0x3;
      return;

    case BF_DEV_FAMILY_UNKNOWN:
      PIPE_MGR_DBGCHK(0);
      break;
  }
  *data = 0;
  *word = 0;
  *logical_tbl = 0;
  *stage = 0;
  *pipe = 0;
}

static inline void extract_idle_lock_msg(bf_dev_family_t dev_fam,
                                         idle_dr_msg_t *msg,
                                         lock_id_t *lock_id,
                                         int *pipe,
                                         int *stage,
                                         int *logical_tbl) {
  switch (dev_fam) {
    case BF_DEV_FAMILY_TOFINO:
      *lock_id = (*msg >> 36) & 0xFFFF;
      *logical_tbl = (*msg >> 52) & 0xF;
      *stage = (*msg >> (52 + 4)) & 0xF;
      *pipe = (*msg >> (52 + 4 + 4)) & 0x3;
      return;
    case BF_DEV_FAMILY_TOFINO2:
    case BF_DEV_FAMILY_TOFINO3:
      *lock_id = (*msg >> 36) & 0xFFFF;
      *logical_tbl = (*msg >> 52) & 0xF;
      *stage = (*msg >> (52 + 4)) & 0x1F;
      *pipe = (*msg >> (52 + 4 + 5)) & 0x3;
      return;

    case BF_DEV_FAMILY_UNKNOWN:
      PIPE_MGR_DBGCHK(0);
      break;
  }
  *lock_id = 0;
  *logical_tbl = 0;
  *stage = 0;
  *pipe = 0;
}

static inline void extract_idle_dump_msg(bf_dev_family_t dev_fam,
                                         idle_dr_msg_t *msg,
                                         uint32_t *dump_word,
                                         int *pipe,
                                         int *stage,
                                         int *logical_tbl,
                                         int *word) {
  switch (dev_fam) {
    case BF_DEV_FAMILY_TOFINO:
      *dump_word = (*msg >> 4) & 0xFF;
      *word = (*msg >> 36) & 0xFFFF;
      *logical_tbl = (*msg >> (36 + 16)) & 0xF;
      *stage = (*msg >> (36 + 16 + 4)) & 0xF;
      *pipe = (*msg >> (36 + 16 + 4 + 4)) & 0x3;
      return;
    case BF_DEV_FAMILY_TOFINO2:
    case BF_DEV_FAMILY_TOFINO3:
      *dump_word = (*msg >> 4) & 0xFF;
      *word = (*msg >> 36) & 0xFFFF;
      *logical_tbl = (*msg >> (36 + 16)) & 0xF;
      *stage = (*msg >> (36 + 16 + 4)) & 0x1F;
      *pipe = (*msg >> (36 + 16 + 4 + 5)) & 0x3;
      return;

    case BF_DEV_FAMILY_UNKNOWN:
      PIPE_MGR_DBGCHK(0);
      break;
  }
  *dump_word = 0;
  *word = 0;
  *logical_tbl = 0;
  *stage = 0;
  *pipe = 0;
}

typedef struct idle_pending_msgs_s {
  struct idle_pending_msgs_s *next;
  struct idle_pending_msgs_s *prev;
  idle_dr_msg_t *msgs;  // Array
  uint32_t msg_count;
} idle_pending_msgs_t;

typedef enum idle_entry_notify_state_e {
  NOTIFY_ENTRY_INVALID = 0,
  NOTIFY_ENTRY_DISABLED,
  NOTIFY_ENTRY_ACTIVE,
  NOTIFY_ENTRY_SWEEP_CANDIDATE,
  NOTIFY_ENTRY_SWEEP,
  NOTIFY_ENTRY_IDLE
} idle_entry_notify_state_e;

static const char *idle_entry_notify_state_str[] = {
    "Invalid", "Disabled", "Active", "Sweep candidate", "Sweep", "Idle"};

static inline const char *ientry_notify_state_str(
    idle_entry_notify_state_e st) {
  return idle_entry_notify_state_str[st];
}

typedef struct idle_entry_s {
  int index;

  /* Below are valid only for notify mode */
  struct idle_entry_s *next_active;
  struct idle_entry_s *prev_active;
  struct idle_entry_s *next_sweep;
  struct idle_entry_s *prev_sweep;
  struct idle_entry_s *next_idle;
  struct idle_entry_s *prev_idle;
  bool inuse;
  pipe_mat_ent_hdl_t ent_hdl;

  idle_entry_notify_state_e notify_state;
  uint32_t init_ttl;
  uint32_t cur_ttl;

  /* Below are valid only for poll mode */
  /*
   * In case of symmetric poll mode entries,
   * update count stores the no of pipes from which we've received
   * updates. When update_count == 0, it indicates that
   * the next update will be a new update and the poll state has to be reset
   * to the current update. In other cases, idle msg is not updated
   * only active messages are updated
   */
  uint32_t update_count;
  pipe_idle_time_hit_state_e poll_state;
} idle_entry_t;

typedef enum idle_move_type_e_ {
  IDLE_ENTRY_MOVE,
  IDLE_ENTRY_DEL,
  IDLE_ENTRY_ADD,
  IDLE_ENTRY_UPDATE_TTL,
  IDLE_ENTRY_RESET_TTL
} idle_move_type_e;

typedef struct idle_move_param_s {
  uint32_t src_idx;
  uint32_t dest_idx;
} idle_move_param_t;

typedef struct idle_del_param_s {
  uint32_t del_idx;
} idle_del_param_t;

typedef struct idle_add_param_s {
  uint32_t add_idx;
  uint32_t ttl;
  /* cur_ttl is needed in case of entry move across stages */
  uint32_t cur_ttl;
  pipe_idle_time_hit_state_e poll_state;
} idle_add_param_t;

typedef struct idle_update_ttl_param_s {
  uint32_t ttl;
} idle_update_ttl_param_t;

typedef struct idle_task_node_s {
  struct idle_task_node_s *next;
  struct idle_task_node_s *prev;
  pipe_mat_ent_hdl_t ent_hdl;
  idle_move_type_e type;
  union {
    idle_move_param_t move;
    idle_del_param_t del;
    idle_add_param_t add;
    idle_update_ttl_param_t update;
  } u;
} idle_task_node_t;

typedef struct idle_task_list_s {
  struct idle_task_list_s *next;
  struct idle_task_list_s *prev;
  /* Add operations do not have a lock. So lock_valid specifies
   * if the lock_id is valid. If lock is not valid, then the
   * item can be removed and processed whenever prior elements from this
   * list are taken off
   */
  bool move_to_front;
  bool lock_valid;
  lock_id_t lock_id;
  idle_task_node_t *tasks;
} idle_task_list_t;

typedef struct idle_tbl_stage_hw_info_s {
  rmt_mem_pack_format_t pack_format;
  uint32_t num_blks;
  rmt_tbl_word_blk_t *tbl_blk;  // array of size num_blks
  uint32_t max_vpn;
} idle_tbl_stage_hw_info_t;

typedef struct idle_entry_location_s_ {
  struct idle_entry_location_s_ *n;
  struct idle_entry_location_s_ *p;
  /* Index valid indicates that the entry is actually
   * present in the cur_index. The index may not be valid
   * when the add operation is still in the task_list
   * In such cases, the ttl value to be used is the new_ttl
   * If the index is valid, then the ttl should be read from
   * the entry at cur_index.
   */
  bool index_valid;
  /* cur_index should be changed when the entry is successfully
   * moved/added
   */
  uint32_t cur_index;
  uint32_t dest_index;
  bool del_in_progress;
} idle_entry_location_t;

typedef struct idle_entry_metadata_s {
  /* List of locations where the entry with the given handle exists.
   * The location is only updated when the entry is actually added
   * at a certain index. If the locations is empty, then the
   * entry hasn't been activated in the stage
   *
   */
  idle_entry_location_t *locations;
  uint32_t refcount;
  int seq_no;
  /* For entry adds, the new_ttl is set to the requested ttl
   * During set_idle_ttl, it'll be set to the requested ttl
   */
  uint32_t new_ttl;
  /* The cur_ttl here is used when entry moves across stages.
   * The ttl of the entry in the source stage is copied and
   * the new entry is added in the destination stage with the
   * cur_ttl set to the below value
   */
  uint32_t cur_ttl;
} idle_entry_metadata_t;

struct idle_tbl_s;

typedef struct idle_tbl_stage_info_s {
  struct idle_tbl_s *idle_tbl_p;
  uint8_t stage_id;
  uint8_t stage_idx;
  rmt_tbl_hdl_t stage_table_handle;
  rmt_idle_time_tbl_params_t rmt_params;
  idle_tbl_stage_hw_info_t hw_info;
  int8_t entries_per_word;
  int32_t no_words;  // No of RAM words allocated

  idle_entry_t **entries;  // 2D array of size
                           // no_words * entries_per_word
                           // (based on the bit-width used)

  /* The sweeps list tracks entries which are idle in hardware and are being
   * aged in software by the software sweeper timer. */
  idle_entry_t *sweeps;
  /* The sweep_candidates list tracks entries that need to be added to the
   * sweeps list.  Entries are added here when:
   *  - they first go idle in HW
   *  - their TTL is modified and SW aging is required
   *  - they are under SW aging and move from one stage to another */
  idle_entry_t *sweep_candidates;

  /* Entry add/del/move queues based on the API */
  pipe_mgr_cvar_t tlist_cvar;
  pipe_mgr_mutex_t tlist_mtx;
  idle_task_list_t *task_list;
  idle_task_list_t *trans_list;
  idle_task_list_t *lock_pending_list;

  /* All the operations done within a lock-unlock, go to the lock_pending_list
   * During unlock, things are removed from the lock_pending_list and
   * appended to the trans_list/task_list based on txn is going or not
   * During commit, the trans_list is appended to the task_list
   * During abort, the trans_list is discarded.
   * One special case during appending trans_list to task_list
   * is that if task_list is empty then all the head end items in
   * the trans_list with no valid lock are processed directly
   */

  /* FIFO of the pending notifications received from hardware */
  pipe_mgr_mutex_t pmsg_mtx;
  idle_pending_msgs_t *pending_msgs;

  uint32_t hw_sweep_interval;
  uint32_t hw_sweep_period;
  uint32_t hw_notify_period;
  uint32_t sw_sweep_period;

  bf_sys_timer_t sweep_timer;

  /* Map of the entry hdl to the metadata */
  bf_map_t entry_mdata_map;
  pipe_mgr_mutex_t stage_map_mtx;
} idle_tbl_stage_info_t;

struct idle_tbl_info_s;

typedef struct idle_tbl_s {
  struct idle_tbl_info_s *idle_tbl_info;  // Back pointer to idle_tbl_info
  bf_dev_pipe_t pipe_id;
  pipe_bitmap_t inst_pipe_bmp;
  /* For asymmetric mode, list of all pipes in scope where this pipe belongs */
  pipe_bitmap_t asym_scope_pipe_bmp;
  uint8_t num_stages;
  idle_tbl_stage_info_t *stage_info;  // Array of size num_stages
  pipe_sess_hdl_t cur_sess_hdl;
  uint32_t sess_flags;
} idle_tbl_t;

typedef struct idle_tbl_info_s {
  char *name;
  pipe_tbl_dir_t direction;
  bf_dev_id_t dev_id;
  rmt_dev_info_t *dev_info;
  pipe_mat_tbl_hdl_t tbl_hdl;
  // Only relevant for alpm tables
  pipe_mat_tbl_hdl_t *ll_tbl_hdls;
  uint32_t num_ll_tbls;
  pipe_mat_match_type_t match_type;
  bool is_symmetric;
  scope_num_t num_scopes;
  scope_pipes_t *scope_pipe_bmp;
  bool repeated_notify;
  bool in_state_restore;

  profile_id_t profile_id;
  pipe_bitmap_t pipe_bmp;
  uint32_t pipe_count;

  uint32_t bit_width;
  pipe_idle_time_params_t tbl_params;

  int32_t no_idle_tbls;
  idle_tbl_t *idle_tbls;

  // Count of stages in which this table exists = pipes * num_stages
  uint32_t stage_count;
  uint32_t update_count;
  // When update_count = stage_count, we've received the updates from
  // all stages
  lock_id_t update_barrier_lock_id;
  bool update_in_progress;
  pipe_idle_tmo_update_complete_cb update_complete_cb_fn;
  void *update_complete_cb_data;

  bf_map_t notif_list;
  bf_map_t notif_list_old;
  // Full match spec copy of the expired entries
  // When destory the map, must first destory/move the
  // existing data (match_sepc_free)
  // if bf_map does not pass the ownership of those match_spec_copy,
  // then it needs handle the deallocation.
  // If it passes the ownership, then it only need to free map itself.
  bf_map_t notif_list_spec_copy;
  pipe_mgr_mutex_t notif_list_mtx;

  // Enable-Disable-Read-Write-Lock
  // for Table Level Synchronization
  pipe_mgr_rwlock_t en_dis_rw_lock;
  bool enabled;
} idle_tbl_info_t;

/* Structure definition for a cookie - which is obtained by clients
 * during HA, for calling into idle table manager APIs. The cookie stores
 * the table info and stage info, which can be used, in each API call, which
 * prevents us from getting the table info and stage info for each API call.
 * Typically during HA reconcilation process, clients make such calls for
 * each match entry in the table. Thus, this optimization prevents us from
 * making such table metadata lookup calls for each such call from a client.
 */
typedef struct idle_tbl_ha_cookie_ {
  idle_tbl_info_t *tbl_info;
  uint32_t num_stage_info;
  idle_tbl_stage_info_t **stage_info;
  bf_dev_pipe_t pipe_id;
  uint8_t stage_id;
} idle_tbl_ha_cookie_t;

#define IDLE_TBL_IS_SYMMETRIC(x) (x->is_symmetric == true)
#define IDLE_TBL_IS_POLL_MODE(x) (x->tbl_params.mode == POLL_MODE)
#define IDLE_TBL_IS_NOTIFY_MODE(x) (x->tbl_params.mode == NOTIFY_MODE)
#define IDLE_SESS_IS_TXN(x) ((x->sess_flags & PIPE_MGR_TBL_API_TXN) != 0)
#define IDLE_TBL_SEND_REPEATED_NOTIF(x) (x->repeated_notify == true)

typedef struct idle_mgr_dev_info_s {
  /* We maintain 2 kinds of mappings to get the idle_tbl_info_t structure.
   * - Using a map which maps based on the tbl-hdl
   * - For the HW notification path, based on the dev-id, pipe, stage and
   *    logical table id
   */
  bf_map_t tbl_hdl_to_tbl_map;
  idle_tbl_info_t ****tbl_lookup /*[# pipes][# stages][# log tbls]*/;
  /* Hopefully some higher entity takes care that if a device is being removed,
   * No API calls make it to us. Otherwise we'll need locking here
   * for every API
   */
  /* Buffers allocated for idle-time notification during init. */
  pipe_mgr_drv_buf_t *bufs[BF_MAX_SUBDEV_COUNT];

  /* An array of the idle-time notification DMA buffers physical address and an
   * index into this array.  This is used to ensure we process notifications in
   * the order they are generated when multiple threads are taking the buffers
   * from the DR. */
  bf_phys_addr_t *buf_addrs[BF_MAX_SUBDEV_COUNT];
  uint32_t buf_addr_idx[BF_MAX_SUBDEV_COUNT];
  uint32_t buf_cnt[BF_MAX_SUBDEV_COUNT];
  bf_sys_mutex_t buf_addrs_mtx[BF_MAX_SUBDEV_COUNT];
  bf_sys_cond_t buf_addrs_cv[BF_MAX_SUBDEV_COUNT];

#define PIPE_MGR_NUM_IDLE_MOVEREG_REGS 2
  uint32_t ***movereg_ctl /*[# pipes][# stages][# move regs]*/;
  pipe_mgr_mutex_t ***movereg_ctl_mtx /*[# pipes][# stages][# move regs]*/;
} idle_mgr_dev_ctx_t;

typedef struct idle_mgr_ctx_s {
  idle_mgr_dev_ctx_t *dev_ctx[PIPE_MGR_NUM_DEVICES];
} idle_mgr_ctx_t;

idle_tbl_info_t *pipe_mgr_idle_tbl_info_get(bf_dev_id_t dev_id,
                                            pipe_mat_tbl_hdl_t tbl_hdl);

/*
 ******************************************************
 *        APIs to be used by session manager          *
 ******************************************************
 */

/** \brief pipe_mgr_idle_init
 *        Initialize the idle data structures
 *
 * This function initializes all the global data structures used for idle
 * management
 *
 * \return pipe_status_t The status of the operation
 */
pipe_status_t pipe_mgr_idle_init(void);

/** \brief pipe_mgr_idle_tbl_create
 *         Create the local structures to hold data about a new idle tbl
 *
 * This routine creates the local data structures needed for idle tbl
 * management. It creates a structure and adds it into the global hash tbl
 *
 * \param dev_id Device ID
 * \param tbl_hdl Table hdl of the idle tbl
 * \return pipe_status_t The status of the operation
 */
pipe_status_t pipe_mgr_idle_tbl_create(bf_dev_id_t dev_id,
                                       pipe_mat_tbl_hdl_t tbl_hdl,
                                       profile_id_t profile_id,
                                       pipe_bitmap_t *pipe_bmp);

/** \brief pipe_mgr_idle_tbl_delete
 *         Deletes the local structures that hold data about a idle tbl
 *
 * This routine deletes the local data structures needed for idle tbl
 * management. It removes it from the global hash tbl and frees all the
 * memory allocated
 *
 * \param tbl_hdl Table hdl of the idle tbl
 * \return pipe_status_t The status of the operation
 */

pipe_status_t pipe_mgr_idle_tbl_delete(bf_dev_id_t dev_id,
                                       pipe_mat_tbl_hdl_t tbl_hdl);

/* Configure idle timeout at table level */
pipe_status_t rmt_idle_params_get(bf_dev_id_t dev_id,
                                  pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                  pipe_idle_time_params_t *params);

pipe_status_t rmt_idle_params_set(pipe_sess_hdl_t sess_hdl,
                                  bf_dev_id_t dev_id,
                                  pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                  pipe_idle_time_params_t params);

pipe_status_t rmt_idle_tmo_enable_get(bf_dev_id_t dev_id,
                                      pipe_mat_tbl_hdl_t tbl_hdl,
                                      bool *enable);

pipe_status_t rmt_idle_tmo_enable(pipe_sess_hdl_t sess_hdl,
                                  bf_dev_id_t dev_id,
                                  pipe_mat_tbl_hdl_t tbl_hdl);

pipe_status_t rmt_idle_register_tmo_cb(bf_dev_id_t dev_id,
                                       pipe_mat_tbl_hdl_t tbl_hdl,
                                       pipe_idle_tmo_expiry_cb callback_fn,
                                       void *client_data);

pipe_status_t rmt_idle_register_tmo_cb_with_match_spec_copy(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    pipe_idle_tmo_expiry_cb_with_match_spec_copy callback_fn2,
    void *client_data);

pipe_status_t rmt_idle_tmo_disable(pipe_sess_hdl_t sess_hdl,
                                   bf_dev_id_t dev_id,
                                   pipe_mat_tbl_hdl_t tbl_hdl);

/* Set the Idle timeout TTL for a given match entry */
pipe_status_t rmt_mat_ent_set_idle_ttl(pipe_sess_hdl_t sess_hdl,
                                       bf_dev_id_t dev_id,
                                       pipe_mat_tbl_hdl_t tbl_hdl,
                                       pipe_mat_ent_hdl_t mat_ent_hdl,
                                       uint32_t ttl, /*< TTL value in msecs */
                                       uint32_t pipe_api_flags,
                                       bool reset);

pipe_status_t rmt_mat_ent_reset_idle_ttl(pipe_sess_hdl_t sess_hdl,
                                         bf_dev_id_t dev_id,
                                         pipe_mat_tbl_hdl_t tbl_hdl,
                                         pipe_mat_ent_hdl_t mat_ent_hdl);

/* API function to poll idle timeout data for a table entry */
pipe_status_t rmt_idle_time_get_hit_state(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_idle_time_hit_state_e *idle_time_data);

/* API function to poll idle timeout data for a table entry */
pipe_status_t rmt_idle_time_set_hit_state(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_idle_time_hit_state_e idle_time_data);

/* API function that should be called
 *  periodically or on-demand prior to querying for the hit state
 * The function completes asynchronously and the client will
 * be notified of it's completion via the provided callback function
 */
pipe_status_t rmt_idle_time_update_hit_state(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    pipe_idle_tmo_update_complete_cb callback_fn,
    void *cb_data);

/* The below APIs are used in notify mode */
/* API function to get the current TTL value of the table entry */
pipe_status_t rmt_mat_ent_get_idle_ttl(pipe_sess_hdl_t sess_hdl,
                                       bf_dev_id_t dev_id,
                                       pipe_mat_tbl_hdl_t tbl_hdl,
                                       pipe_mat_ent_hdl_t mat_ent_hdl,
                                       uint32_t *ttl);

/************************************************
 *     APIs to be called by the mat-tbl manager *
 ************************************************/

pipe_status_t rmt_idle_add_entry(pipe_sess_hdl_t sess_hdl,
                                 bf_dev_id_t dev_id,
                                 pipe_mat_tbl_hdl_t tbl_hdl,
                                 pipe_mat_ent_hdl_t ent_hdl,
                                 bf_dev_pipe_t pipe_id,
                                 uint8_t stage_id,
                                 rmt_tbl_hdl_t stage_table_hdl,
                                 uint32_t index,
                                 uint32_t ttl,
                                 bool end_of_move_add,
                                 uint32_t pipe_api_flags);

pipe_status_t rmt_idle_delete_entry(pipe_sess_hdl_t sess_hdl,
                                    bf_dev_id_t dev_id,
                                    pipe_mat_tbl_hdl_t tbl_hdl,
                                    pipe_mat_ent_hdl_t ent_hdl,
                                    bf_dev_pipe_t pipe_id,
                                    uint8_t stage_id,
                                    rmt_tbl_hdl_t stage_table_handle,
                                    uint32_t index,
                                    uint32_t pipe_api_flags);

pipe_status_t rmt_idle_move_entry(pipe_sess_hdl_t sess_hdl,
                                  bf_dev_id_t dev_id,
                                  pipe_mat_tbl_hdl_t tbl_hdl,
                                  pipe_mat_ent_hdl_t ent_hdl,
                                  bf_dev_pipe_t pipe_id,
                                  uint8_t src_stage_id,
                                  rmt_tbl_hdl_t src_stage_table_hdl,
                                  uint8_t dest_stage_id,
                                  rmt_tbl_hdl_t dest_stage_table_hdl,
                                  uint32_t src_index,
                                  uint32_t dest_index,
                                  uint32_t pipe_api_flags);

pipe_status_t rmt_idle_tbl_unlock(pipe_sess_hdl_t sess_hdl,
                                  bf_dev_id_t dev_id,
                                  pipe_mat_tbl_hdl_t tbl_hdl,
                                  lock_id_t lock_id,
                                  bf_dev_pipe_t pipe_id,
                                  uint8_t stage_id,
                                  rmt_tbl_hdl_t stage_table_handle,
                                  uint32_t pipe_api_flags);

pipe_status_t rmt_idle_tbl_lock(pipe_sess_hdl_t sess_hdl,
                                bf_dev_id_t dev_id,
                                pipe_mat_tbl_hdl_t tbl_hdl,
                                lock_id_t lock_id,
                                bf_dev_pipe_t pipe_id,
                                uint8_t stage_id,
                                rmt_tbl_hdl_t stage_table_handle,
                                uint32_t pipe_api_flags);

pipe_status_t rmt_idle_abort_txn(bf_dev_id_t dev_id,
                                 pipe_mat_tbl_hdl_t tbl_hdl,
                                 bf_dev_pipe_t *pipes_list,
                                 unsigned nb_pipes);

pipe_status_t rmt_idle_commit_txn(bf_dev_id_t dev_id,
                                  pipe_mat_tbl_hdl_t tbl_hdl,
                                  bf_dev_pipe_t *pipes_list,
                                  unsigned nb_pipes);

pipe_status_t pipe_mgr_idle_tbl_set_symmetric_mode(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    bool symmetric,
    scope_num_t num_scopes,
    scope_pipes_t *scope_pipe_bmp);
pipe_status_t pipe_mgr_idle_tbl_notify_pipe_list_get(
    idle_tbl_info_t *idle_tbl_info,
    bf_dev_pipe_t pipe_id,
    pipe_bitmap_t *local_pipe_bmp);

pipe_status_t pipe_mgr_idle_tbl_set_repeated_notify(bf_dev_id_t dev_id,
                                                    pipe_mat_tbl_hdl_t tbl_hdl,
                                                    bool repeated_notify);

pipe_status_t pipe_mgr_idle_tbl_get_repeated_notify(bf_dev_id_t dev_id,
                                                    pipe_mat_tbl_hdl_t tbl_hdl,
                                                    bool *repeated_notify);

void pipe_mgr_drv_idle_cb(bf_dev_id_t logical_device,
                          bf_subdev_id_t subdev_id,
                          int size,
                          bf_dma_addr_t dma_addr);

pipe_status_t pipe_mgr_drv_idl_buf_init(pipe_sess_hdl_t h, bf_dev_id_t devId);
pipe_status_t pipe_mgr_idl_buf_load(bf_dev_id_t devId);
pipe_status_t pipe_mgr_drv_idl_buf_cleanup(uint8_t devId);

pipe_status_t pipe_mgr_idle_write_all_idle_ctrl_regs(pipe_sess_hdl_t sess_hdl,
                                                     rmt_dev_info_t *dev_info);
pipe_status_t pipe_mgr_idle_fast_reconfig_push_state(pipe_sess_hdl_t sess_hdl,
                                                     bf_dev_id_t dev_id);

pipe_status_t pipe_mgr_idle_add_device(bf_dev_id_t dev_id);

pipe_status_t pipe_mgr_idle_remove_device(bf_dev_id_t dev_id);

pipe_status_t pipe_mgr_idle_log_state(bf_dev_id_t dev_id,
                                      pipe_mat_tbl_hdl_t tbl_hdl,
                                      cJSON *idle_tbls);

pipe_status_t pipe_mgr_idle_restore_init(bf_dev_id_t dev_id, cJSON *idle_tbl);

pipe_status_t pipe_mgr_idle_restore_state(bf_dev_id_t dev_id, cJSON *idle_tbl);

/************************************************************************
 *     APIs to be called by the mat-tbl manager during HA reconcilation *
 ************************************************************************/
pipe_status_t rmt_idle_get_cookie(bf_dev_id_t dev_id,
                                  pipe_mat_tbl_hdl_t tbl_hdl,
                                  bf_dev_pipe_t pipe_id,
                                  uint8_t stage_id,
                                  rmt_tbl_hdl_t stage_table_handle,
                                  idle_tbl_ha_cookie_t *cookie);

pipe_status_t rmt_mat_ent_update_ttl(bf_dev_id_t dev_id,
                                     pipe_mat_tbl_hdl_t tbl_hdl,
                                     pipe_mat_ent_hdl_t ent_hdl,
                                     bf_dev_pipe_t pipe_id,
                                     uint8_t stage_id,
                                     rmt_tbl_hdl_t stage_table_handle,
                                     idle_tbl_ha_cookie_t *cookie,
                                     uint32_t ttl);

static inline uint32_t pipe_mgr_idle_get_ttl_for_ha(
    bf_dev_id_t device_id, pipe_mat_tbl_hdl_t mat_tbl_hdl) {
  idle_tbl_info_t *idle_tbl_info =
      pipe_mgr_idle_tbl_info_get(device_id, mat_tbl_hdl);
  if (!IDLE_TBL_IS_NOTIFY_MODE(idle_tbl_info)) {
    return 0;
  }
  return NONZERO_TTL;
}

pipe_status_t pipe_mgr_idle_get_init_ttl(bf_dev_id_t dev_id,
                                         pipe_mat_tbl_hdl_t tbl_hdl,
                                         pipe_mat_ent_hdl_t ent_hdl,
                                         bf_dev_pipe_t pipe_id,
                                         uint32_t stage_id,
                                         rmt_tbl_hdl_t stage_table_handle,
                                         uint32_t *init_ttl_p);

pipe_status_t pipe_mgr_idle_phy_write_line(bf_dev_id_t dev_id,
                                           pipe_mat_tbl_hdl_t tbl_hdl,
                                           uint32_t stage,
                                           mem_id_t mem_id,
                                           uint32_t mem_offset);

#if PIPE_MGR_CONFIG_INCLUDE_UCLI == 1
ucli_node_t *pipe_mgr_idle_ucli_node_create(ucli_node_t *n);
#else
void *pipe_mgr_idle_ucli_node_create(void);
#endif

#ifdef __cplusplus
}
#endif /* C++ */

#endif /* _PIPE_MGR_IDLE_H */
