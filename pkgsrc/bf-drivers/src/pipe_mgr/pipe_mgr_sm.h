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

#ifndef _PIPE_MGR_SM_H
#define _PIPE_MGR_SM_H

/* Standard includes */

/* Module includes */
#include <target-utils/map/map.h>
#include "pipe_mgr_rmt_cfg.h"

struct pipe_mgr_move_list_t;
struct pipe_mgr_adt_move_list_t;
struct pipe_mgr_sel_move_list_t;

enum pipe_mgr_table_owner_t {
  PIPE_MGR_TBL_OWNER_EXM,
  PIPE_MGR_TBL_OWNER_TRN,
  PIPE_MGR_TBL_OWNER_ALPM,
  PIPE_MGR_TBL_OWNER_ADT,
  PIPE_MGR_TBL_OWNER_SELECT,
  PIPE_MGR_TBL_OWNER_STAT,
  PIPE_MGR_TBL_OWNER_METER,
  PIPE_MGR_TBL_OWNER_STFUL,
  PIPE_MGR_TBL_OWNER_PHASE0,
  PIPE_MGR_TBL_OWNER_NO_KEY,
  PIPE_MGR_TBL_OWNER_NONE,
};
static inline const char *pipe_mgr_table_owner_str(
    enum pipe_mgr_table_owner_t x) {
  switch (x) {
    case PIPE_MGR_TBL_OWNER_EXM:
      return "Exm";
    case PIPE_MGR_TBL_OWNER_TRN:
      return "Tcam";
    case PIPE_MGR_TBL_OWNER_ALPM:
      return "ALPM";
    case PIPE_MGR_TBL_OWNER_ADT:
      return "ADT";
    case PIPE_MGR_TBL_OWNER_SELECT:
      return "Sel";
    case PIPE_MGR_TBL_OWNER_STAT:
      return "Stat";
    case PIPE_MGR_TBL_OWNER_METER:
      return "Mtr";
    case PIPE_MGR_TBL_OWNER_STFUL:
      return "Stfl";
    case PIPE_MGR_TBL_OWNER_PHASE0:
      return "P0";
    case PIPE_MGR_TBL_OWNER_NO_KEY:
      return "NoKey";
    case PIPE_MGR_TBL_OWNER_NONE:
      return "None";
    default:
      return "Unknown";
  }
}

typedef struct pipe_mgr_sm_tbl_instance_t {
  /* Pointer back to pipe_mgr_sm_tbl_info_t. */
  struct pipe_mgr_sm_tbl_info_t *tbl;

  /* DLL stored against a session to easily walk all its associated tables */
  struct pipe_mgr_sm_tbl_instance_t *next;
  struct pipe_mgr_sm_tbl_instance_t *prev;
} pipe_mgr_sm_tbl_instance_t;

typedef struct pipe_mgr_sm_tbl_info_t {
  /* Per session table instance. */
  struct pipe_mgr_sm_tbl_instance_t instance[PIPE_MGR_MAX_SESSIONS];

  /* A list of sm_tbl_info_t's which need to be locked along with this one. */
  struct pipe_mgr_sm_tbl_info_t **resources;
  int resource_count;

  /* Which table this is. */
  pipe_tbl_hdl_t id;
  /* Per pipe session using this table or PIPE_MGR_MAX_SESSIONS if not in use.
   */
  uint8_t sid[PIPE_MGR_MAX_PIPES];
  /* Indicates which table management logic is responsible for managing the
   * table. */
  enum pipe_mgr_table_owner_t owner;
  /* Flag indicating if the table has been created by the table manager. */
  bool initialized;
  /* Which profile the table belongs to. */
  profile_id_t prof_id;
  /* Callback for placement decisions. */
  union {
    pipe_mat_update_cb mat_cb; /* For MATs. */
    pipe_adt_update_cb adt_cb; /* For ADTs. */
    pipe_sel_update_cb sel_cb; /* For Selection tables. */
    void *none;                /* No other table type has a callback. */
  } u;
  void *cb_cookie;
} pipe_mgr_sm_tbl_info_t;

typedef struct pipe_mgr_sm_mir_info_t {
  struct pipe_mgr_sm_mir_info_t *next;
  struct pipe_mgr_sm_mir_info_t *prev;
  /* The mirror session id reserved. */
  int mir_sess_id;
  /* Session using this table or PIPE_MGR_MAX_SESSIONS if not in use. */
  pipe_sess_hdl_t sid;
} pipe_mgr_sm_mir_info_t;

struct pipe_mgr_sm_move_list_hdr {
  struct pipe_mgr_sm_move_list_hdr *next;
  struct pipe_mgr_sm_move_list_hdr *prev;
  bf_dev_id_t dev_id;
  pipe_tbl_hdl_t tbl_hdl;
  struct pipe_mgr_move_list_t *ml;
  struct pipe_mgr_move_list_t *ml_tail;
};

pipe_status_t pipe_mgr_sm_init(pipe_sess_hdl_t sh, bf_dev_id_t dev_id);
pipe_status_t pipe_mgr_sm_cleanup(bf_dev_id_t dev_id);
pipe_status_t pipe_mgr_verify_pkt_gen_access(pipe_sess_hdl_t sh,
                                             bf_dev_id_t dev_id,
                                             bool rsvRsc);
pipe_status_t pipe_mgr_verify_hash_calc_access(pipe_sess_hdl_t sh,
                                               bf_dev_id_t dev_id,
                                               bool rsvRsc);
pipe_status_t pipe_mgr_verify_lrn_ses_access(pipe_sess_hdl_t sh,
                                             bf_dev_id_t dev_id,
                                             bool rsvRsc);
pipe_status_t pipe_mgr_verify_mir_ses_access(pipe_sess_hdl_t sh,
                                             bf_dev_id_t dev_id,
                                             bf_mirror_id_t sid,
                                             bool rsvRsc);
pipe_status_t pipe_mgr_verify_tbl_access(pipe_sess_hdl_t sh,
                                         bf_dev_id_t dev_id,
                                         pipe_tbl_hdl_t th,
                                         bool rsvRsc);
pipe_status_t pipe_mgr_verify_pipe_tbl_access(pipe_sess_hdl_t sh,
                                              dev_target_t dev_tgt,
                                              pipe_tbl_hdl_t th,
                                              bool rsvRsc);
pipe_status_t pipe_mgr_verify_tbl_access_ignore_err(pipe_sess_hdl_t sh,
                                                    bf_dev_id_t dev_id,
                                                    pipe_tbl_hdl_t th,
                                                    bool rsvRsc);

enum pipe_mgr_table_owner_t pipe_mgr_sm_tbl_owner(bf_dev_id_t dev_id,
                                                  pipe_tbl_hdl_t id);
bool pipe_mgr_is_tbl_mat_tbl(bf_dev_id_t dev_id, pipe_tbl_hdl_t id);

bool pipe_mgr_is_txn_using_dev(pipe_sess_hdl_t sh, bf_dev_id_t dev_id);
void pipe_mgr_sm_commit(pipe_sess_hdl_t sh);
pipe_status_t pipe_mgr_sm_abort(pipe_sess_hdl_t sh);
void pipe_mgr_sm_atom_cleanup(pipe_sess_hdl_t sh);
void pipe_mgr_sm_release(pipe_sess_hdl_t sh);

pipe_mgr_sm_tbl_info_t *pipe_mgr_sm_get_tbl_info(bf_dev_id_t dev_id,
                                                 pipe_tbl_hdl_t id);
/* To register table placement operation callbacks. */
pipe_status_t pipe_mgr_sm_set_mat_cb(bf_dev_id_t dev_id,
                                     pipe_mat_tbl_hdl_t th,
                                     pipe_mat_update_cb cb,
                                     void *cb_cookie);
pipe_status_t pipe_mgr_sm_set_adt_cb(bf_dev_id_t dev_id,
                                     pipe_adt_tbl_hdl_t th,
                                     pipe_adt_update_cb cb,
                                     void *cb_cookie);
pipe_status_t pipe_mgr_sm_set_sel_cb(bf_dev_id_t dev_id,
                                     pipe_sel_tbl_hdl_t th,
                                     pipe_sel_update_cb cb,
                                     void *cb_cookie);

pipe_status_t pipe_mgr_sm_save_ml_prep(pipe_sess_hdl_t sess_hdl,
                                       bf_dev_id_t dev_id,
                                       pipe_tbl_hdl_t tbl_hdl);
void pipe_mgr_sm_save_ml(pipe_sess_hdl_t sess_hdl,
                         bf_dev_id_t dev_id,
                         pipe_tbl_hdl_t tbl_hdl,
                         struct pipe_mgr_move_list_t *ml);

void pipe_mgr_sm_issue_mat_cb(bf_dev_id_t dev_id,
                              pipe_mat_tbl_hdl_t mat_tbl_hdl,
                              struct pipe_mgr_move_list_t *ml);
void pipe_mgr_sm_issue_adt_cb(bf_dev_id_t dev_id,
                              pipe_adt_tbl_hdl_t adt_tbl_hdl,
                              struct pipe_mgr_adt_move_list_t *ml);
void pipe_mgr_sm_issue_sel_cb(bf_dev_id_t dev_id,
                              pipe_sel_tbl_hdl_t sel_tbl_hdl,
                              struct pipe_mgr_sel_move_list_t *ml);
pipe_status_t pipe_mgr_tbl_add_init_default_entry(
    pipe_sess_hdl_t sess_hdl,
    rmt_dev_info_t *dev_info,
    pipe_mat_tbl_info_t *mat_tbl_info);
pipe_status_t pipe_mgr_static_and_default_entry_init(pipe_sess_hdl_t sess_hdl,
                                                     bf_dev_id_t dev_id);

#endif /* _PIPE_MGR_SM_H */
