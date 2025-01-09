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
 * @file pipe_mgr_drv.c
 * @date
 *
 * Implementation of pipeline management driver interface
 */

/* Module header files */
#include <dvm/bf_drv_intf.h>
#include <lld/bf_dma_if.h>
#include <pipe_mgr/pipe_mgr_mirror_intf.h>

/* Local header files */
#include "pipe_mgr_int.h"
#include "pipe_mgr_sm.h"
#include "pipe_mgr_tcam.h"
#include "pipe_mgr_tcam_hw.h"
#include "pipe_mgr_alpm.h"
#include "pipe_mgr_exm_tbl_init.h"
#include "pipe_mgr_adt_init.h"
#include "pipe_mgr_select_tbl.h"
#include "pipe_mgr_stats_tbl.h"
#include "pipe_mgr_meter_tbl_init.h"
#include "pipe_mgr_stat_tbl_init.h"
#include "pipe_mgr_idle.h"
#include "pipe_mgr_db.h"
#include "pipe_mgr_phase0_tbl_mgr.h"
#include "pipe_mgr_stful_tbl_mgr.h"
#include "pipe_mgr_pktgen.h"
#include "pipe_mgr_tbl.h"
#include "pipe_mgr_mirror_buffer_comm.h"
#include "pipe_mgr_mirror_buffer.h"

static pipe_status_t static_entry_add_helper(pipe_sess_hdl_t sess_hdl,
                                             dev_target_t dev_tgt,
                                             pipe_mat_tbl_info_t *t);

static void get_sm_tbl_pipes_list(pipe_mgr_sm_tbl_info_t *t,
                                  pipe_sess_hdl_t sh,
                                  bf_dev_pipe_t *pipes_list,
                                  unsigned *nb_pipes);

pipe_status_t pipe_mgr_exm_atom_cleanup(bf_dev_id_t dev_id,
                                        pipe_mat_tbl_hdl_t tbl_hdl) {
  (void)dev_id;
  (void)tbl_hdl;
  return PIPE_SUCCESS;
}

static pipe_mgr_sm_tbl_info_t *getTblInfoByHdl(bf_dev_id_t dev_id,
                                               pipe_tbl_hdl_t id) {
  pipe_mgr_sm_tbl_info_t *t = NULL;
  if (!pipe_mgr_device_present(dev_id)) {
    LOG_ERROR("Device %u is not present", dev_id);
    return NULL;
  }
  /* Look up the table handle in a session management database. */
  bf_map_sts_t msts = bf_map_get(pipe_mgr_sm_tbl_db(dev_id), id, (void **)&t);
  if (BF_MAP_OK != msts) return NULL;
  /* The table found must be the one requested. */
  PIPE_MGR_DBGCHK(!t || t->id == id);
  return t;
}

pipe_mgr_sm_tbl_info_t *pipe_mgr_sm_get_tbl_info(bf_dev_id_t dev_id,
                                                 pipe_tbl_hdl_t id) {
  pipe_mgr_sm_tbl_info_t *t = NULL;
  if (!pipe_mgr_device_present(dev_id)) {
    LOG_ERROR("Device %u is not present", dev_id);
    return NULL;
  }
  bf_map_sts_t msts = bf_map_get(pipe_mgr_sm_tbl_db(dev_id), id, (void **)&t);
  if ((BF_MAP_OK != msts) || (!t) || (t && (t->id != id))) {
    return NULL;
  }
  return t;
}

enum pipe_mgr_table_owner_t pipe_mgr_sm_tbl_owner(bf_dev_id_t dev_id,
                                                  pipe_tbl_hdl_t id) {
  if (!pipe_mgr_device_present(dev_id)) {
    LOG_ERROR("Device %u is not present", dev_id);
    return PIPE_MGR_TBL_OWNER_NONE;
  }
  pipe_mgr_sm_tbl_info_t *t = NULL;
  bf_map_sts_t msts = bf_map_get(pipe_mgr_sm_tbl_db(dev_id), id, (void **)&t);
  if ((BF_MAP_OK != msts) || (!t) || (t && (t->id != id))) {
    return PIPE_MGR_TBL_OWNER_NONE;
  }
  return t->owner;
}

bool pipe_mgr_is_tbl_mat_tbl(bf_dev_id_t dev_id, pipe_tbl_hdl_t id) {
  if (!pipe_mgr_device_present(dev_id)) {
    LOG_ERROR("Device %u is not present", dev_id);
    return PIPE_MGR_TBL_OWNER_NONE;
  }
  pipe_mgr_sm_tbl_info_t *t = NULL;
  bf_map_sts_t msts = bf_map_get(pipe_mgr_sm_tbl_db(dev_id), id, (void **)&t);
  if ((BF_MAP_OK != msts) || (!t) || (t && (t->id != id))) {
    return false;
  }
  switch (t->owner) {
    case PIPE_MGR_TBL_OWNER_EXM:
    case PIPE_MGR_TBL_OWNER_TRN:
    case PIPE_MGR_TBL_OWNER_ALPM:
    case PIPE_MGR_TBL_OWNER_PHASE0:
      return true;
      break;
    default:
      return false;
      break;
  }
  return false;
}

bool pipe_mgr_is_txn_using_dev(pipe_sess_hdl_t sh, bf_dev_id_t dev_id) {
  pipe_mgr_sess_ctx_t *s = pipe_mgr_get_sess_ctx(sh, __func__, __LINE__);
  if (s == NULL) {
    PIPE_MGR_DBGCHK(0);
    return false;
  }

  /* First check if the packet generator is used. */
  bool pg = false;
  int i;
  for (i = 0; i < PIPE_MGR_NUM_DEVICES; ++i) {
    if (!pipe_mgr_device_present(i)) continue;
    pg |= pipe_mgr_get_pkt_gen_hdl(i) == sh;
  }

  /* If a session is using a device then it will have at least one table on
   * its reservation list for that device so the list will not be NULL.  If
   * a session is not using a device then the table reservation list for that
   * device will be NULL. */
  return !!s->tables[dev_id] || pg;
}

pipe_status_t pipe_mgr_sm_abort(pipe_sess_hdl_t sh) {
  bf_dev_pipe_t pipes_list[PIPE_MGR_MAX_PIPES];
  pipe_status_t sts, first_err = PIPE_SUCCESS;
  unsigned nb_pipes = 0;
  unsigned d;

  pipe_mgr_sess_ctx_t *s = pipe_mgr_get_sess_ctx(sh, __func__, __LINE__);
  if (s == NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }
  for (d = 0; d < PIPE_MGR_NUM_DEVICES; ++d) {
    pipe_mgr_sm_tbl_instance_t *sess_tbl;
    pipe_mgr_sm_tbl_info_t *t;
    for (sess_tbl = s->tables[d]; sess_tbl; sess_tbl = sess_tbl->next) {
      t = sess_tbl->tbl;
      get_sm_tbl_pipes_list(t, sh, pipes_list, &nb_pipes);
      if (PIPE_MGR_TBL_OWNER_EXM == t->owner) {
        sts = pipe_mgr_exm_tbl_txn_abort(d, t->id, pipes_list, nb_pipes);
        if (PIPE_SUCCESS == first_err) first_err = sts;
        /* Drive the abort on the match table info which is keeping track
         * of match specs in the table.
         */
        pipe_mgr_mat_tbl_txn_abort(d, t->id, pipes_list, nb_pipes);
      } else if (PIPE_MGR_TBL_OWNER_TRN == t->owner) {
        sts = pipe_mgr_tcam_abort(d, t->id, pipes_list, nb_pipes);
        if (PIPE_SUCCESS == first_err) first_err = sts;
        pipe_mgr_mat_tbl_txn_abort(d, t->id, pipes_list, nb_pipes);
        if (PIPE_SUCCESS == first_err) first_err = sts;
      } else if (PIPE_MGR_TBL_OWNER_ALPM == t->owner) {
        sts = pipe_mgr_alpm_abort(d, t->id, pipes_list, nb_pipes);
        if (PIPE_SUCCESS == first_err) first_err = sts;
        pipe_mgr_mat_tbl_txn_abort(d, t->id, pipes_list, nb_pipes);
      } else if (PIPE_MGR_TBL_OWNER_ADT == t->owner) {
        sts = pipe_mgr_adt_txn_abort(d, t->id, pipes_list, nb_pipes);
        if (PIPE_SUCCESS == first_err) first_err = sts;
      } else if (PIPE_MGR_TBL_OWNER_SELECT == t->owner) {
        sts = pipe_mgr_sel_abort(d, t->id, pipes_list, nb_pipes);
        if (PIPE_SUCCESS == first_err) first_err = sts;
      } else if (PIPE_MGR_TBL_OWNER_STAT == t->owner) {
        /* Nothing needed. */
      } else if (PIPE_MGR_TBL_OWNER_METER == t->owner) {
        /* Nothing needed. */
      } else if (PIPE_MGR_TBL_OWNER_STFUL == t->owner) {
        /* Nothing needed. */
      } else if (PIPE_MGR_TBL_OWNER_PHASE0 == t->owner) {
        pipe_mgr_phase0_tbl_txn_abort(d, t->id, pipes_list, nb_pipes);
        pipe_mgr_mat_tbl_txn_abort(d, t->id, pipes_list, nb_pipes);
      } else if (PIPE_MGR_TBL_OWNER_NO_KEY == t->owner) {
        /* Nothing needed. */
      } else {
        PIPE_MGR_DBGCHK(0);
      }
    }
    /* Clean up any move lists saved as part of the transaction. */
    struct pipe_mgr_sm_move_list_hdr *hdr = s->txn_mls[d];
    while (hdr) {
      /* Free the move list since it is being discarded. */
      if (hdr->ml) {
        switch (pipe_mgr_sm_tbl_owner(d, hdr->tbl_hdl)) {
          case PIPE_MGR_TBL_OWNER_EXM:
          case PIPE_MGR_TBL_OWNER_TRN:
          case PIPE_MGR_TBL_OWNER_ALPM:
          case PIPE_MGR_TBL_OWNER_PHASE0:
            free_move_list_and_data(&hdr->ml, true);
            break;
          case PIPE_MGR_TBL_OWNER_ADT:
            free_adt_move_list_and_data((pipe_mgr_adt_move_list_t **)&hdr->ml);
            break;
          case PIPE_MGR_TBL_OWNER_SELECT:
            free_sel_move_list_and_data((pipe_mgr_sel_move_list_t **)&hdr->ml);
            break;
          case PIPE_MGR_TBL_OWNER_STFUL:
            pipe_mgr_stful_free_ops(
                (struct pipe_mgr_stful_op_list_t **)&hdr->ml);
            break;
          case PIPE_MGR_TBL_OWNER_METER:
            pipe_mgr_meter_free_ops(
                (struct pipe_mgr_meter_op_list_t **)&hdr->ml);
            break;
          default:
            PIPE_MGR_DBGCHK(0);
        }
      }

      /* Free this header node since the move list is freed. */
      struct pipe_mgr_sm_move_list_hdr *tmp = hdr->next;
      PIPE_MGR_FREE(hdr);
      hdr = tmp;
    }
    s->txn_mls[d] = NULL;
    if (pipe_mgr_device_present(d) && sh == pipe_mgr_get_pkt_gen_hdl(d)) {
      pipe_mgr_pktgen_txn_abort(d);
    }
  }
  return first_err;
}

void pipe_mgr_sm_commit(pipe_sess_hdl_t sh) {
  bf_dev_pipe_t pipes_list[PIPE_MGR_MAX_PIPES];
  unsigned nb_pipes = 0;
  unsigned d;

  pipe_mgr_sess_ctx_t *s = pipe_mgr_get_sess_ctx(sh, __func__, __LINE__);
  if (s == NULL) {
    PIPE_MGR_DBGCHK(0);
    return;
  }

  for (d = 0; d < PIPE_MGR_NUM_DEVICES; ++d) {
    bool virtual_dev = pipe_mgr_is_device_virtual(d);
    pipe_mgr_sm_tbl_instance_t *sess_tbl;
    pipe_mgr_sm_tbl_info_t *t;
    for (sess_tbl = s->tables[d]; sess_tbl; sess_tbl = sess_tbl->next) {
      t = sess_tbl->tbl;
      get_sm_tbl_pipes_list(t, sh, pipes_list, &nb_pipes);
      if (PIPE_MGR_TBL_OWNER_EXM == t->owner) {
        pipe_mgr_exm_tbl_txn_commit(d, t, pipes_list, nb_pipes);
        /* Drive the txn commit for the match table info which is keeping
         * track of the match specs in the match table.
         */
        pipe_mgr_mat_tbl_txn_commit(d, t->id, pipes_list, nb_pipes);
      } else if (PIPE_MGR_TBL_OWNER_TRN == t->owner) {
        pipe_mgr_tcam_commit(d, t->id, pipes_list, nb_pipes);
        pipe_mgr_mat_tbl_txn_commit(d, t->id, pipes_list, nb_pipes);
      } else if (PIPE_MGR_TBL_OWNER_ALPM == t->owner) {
        pipe_mgr_alpm_commit(d, t->id, pipes_list, nb_pipes);
        pipe_mgr_mat_tbl_txn_commit(d, t->id, pipes_list, nb_pipes);
      } else if (PIPE_MGR_TBL_OWNER_ADT == t->owner) {
        pipe_mgr_adt_txn_commit(d, t->id, pipes_list, nb_pipes);
      } else if (PIPE_MGR_TBL_OWNER_SELECT == t->owner) {
        pipe_mgr_sel_commit(d, t->id, pipes_list, nb_pipes);
      } else if (PIPE_MGR_TBL_OWNER_STAT == t->owner) {
        /* Nothing needed. */
      } else if (PIPE_MGR_TBL_OWNER_METER == t->owner) {
        /* Nothing needed. */
      } else if (PIPE_MGR_TBL_OWNER_STFUL == t->owner) {
        /* Nothing needed. */
      } else if (PIPE_MGR_TBL_OWNER_PHASE0 == t->owner) {
        pipe_mgr_phase0_tbl_txn_commit(d, t->id, pipes_list, nb_pipes);
        pipe_mgr_mat_tbl_txn_commit(d, t->id, pipes_list, nb_pipes);
      } else if (PIPE_MGR_TBL_OWNER_NO_KEY == t->owner) {
        /* Nothing needed. */
      } else {
        PIPE_MGR_DBGCHK(0);
      }
    }

    /* Issue callbacks for the move lists in this transaction. */
    struct pipe_mgr_sm_move_list_hdr *hdr = s->txn_mls[d];
    while (hdr) {
      if (hdr->ml) {
        switch (pipe_mgr_sm_tbl_owner(d, hdr->tbl_hdl)) {
          case PIPE_MGR_TBL_OWNER_EXM:
          case PIPE_MGR_TBL_OWNER_TRN:
          case PIPE_MGR_TBL_OWNER_PHASE0:
            pipe_mgr_sm_issue_mat_cb(d, hdr->tbl_hdl, hdr->ml);
            break;
          case PIPE_MGR_TBL_OWNER_ADT:
            pipe_mgr_sm_issue_adt_cb(
                d, hdr->tbl_hdl, (pipe_mgr_adt_move_list_t *)hdr->ml);
            break;
          case PIPE_MGR_TBL_OWNER_SELECT:
            pipe_mgr_sm_issue_sel_cb(
                d, hdr->tbl_hdl, (pipe_mgr_sel_move_list_t *)hdr->ml);
            break;
          default:
            break;
        }

        /* Process the move lists in this transaction if the device is physical.
         */
        if (!virtual_dev) {
          uint32_t processed = 0;
          pipe_status_t sts = PIPE_SUCCESS;
          switch (pipe_mgr_sm_tbl_owner(d, hdr->tbl_hdl)) {
            case PIPE_MGR_TBL_OWNER_EXM:
              sts = pipe_mgr_exm_tbl_process_move_list(
                  sh, d, hdr->tbl_hdl, hdr->ml, &processed);
              free_move_list(&hdr->ml, true);
              break;
            case PIPE_MGR_TBL_OWNER_TRN:
              sts = pipe_mgr_tcam_process_move_list(
                  sh, d, hdr->tbl_hdl, hdr->ml, &processed);
              free_move_list(&hdr->ml, true);
              break;
            case PIPE_MGR_TBL_OWNER_ALPM:
              sts = pipe_mgr_alpm_process_move_list(
                  sh, d, hdr->tbl_hdl, hdr->ml, &processed);
              free_move_list(&hdr->ml, true);
              break;
            case PIPE_MGR_TBL_OWNER_PHASE0:
              sts = pipe_mgr_phase0_ent_program(
                  sh, d, hdr->tbl_hdl, hdr->ml, &processed);
              free_move_list(&hdr->ml, true);
              break;
            case PIPE_MGR_TBL_OWNER_ADT:
              sts = pipe_adt_process_move_list(
                  sh,
                  d,
                  hdr->tbl_hdl,
                  (pipe_mgr_adt_move_list_t *)hdr->ml,
                  &processed);
              free_adt_move_list((pipe_mgr_adt_move_list_t **)&hdr->ml);
              break;
            case PIPE_MGR_TBL_OWNER_SELECT:
              sts = pipe_sel_process_move_list(
                  sh,
                  d,
                  hdr->tbl_hdl,
                  (pipe_mgr_sel_move_list_t *)hdr->ml,
                  &processed);
              free_sel_move_list((pipe_mgr_sel_move_list_t **)&hdr->ml);
              break;
            case PIPE_MGR_TBL_OWNER_STFUL:
              sts = pipe_mgr_stful_process_op_list(
                  sh,
                  d,
                  hdr->tbl_hdl,
                  (struct pipe_mgr_stful_op_list_t *)hdr->ml,
                  &processed);
              pipe_mgr_stful_free_ops(
                  (struct pipe_mgr_stful_op_list_t **)&hdr->ml);
              break;
            case PIPE_MGR_TBL_OWNER_METER:
              sts = pipe_mgr_meter_process_op_list(
                  sh,
                  d,
                  hdr->tbl_hdl,
                  (struct pipe_mgr_meter_op_list_t *)hdr->ml,
                  &processed);
              pipe_mgr_meter_free_ops(
                  (struct pipe_mgr_meter_op_list_t **)&hdr->ml);
              break;
            default:
              PIPE_MGR_DBGCHK(0);
              break;
          }
          /* This is not expected to fail since we just generated the move lists
           * being processed.  We also don't have a good way to undo the
           * operations that were issued to the table managers prior to the
           * failure. */
          PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
        } else {
          switch (pipe_mgr_sm_tbl_owner(d, hdr->tbl_hdl)) {
            case PIPE_MGR_TBL_OWNER_EXM:
            case PIPE_MGR_TBL_OWNER_TRN:
            case PIPE_MGR_TBL_OWNER_ALPM:
            case PIPE_MGR_TBL_OWNER_PHASE0:
              free_move_list(&hdr->ml, true);
              break;
            case PIPE_MGR_TBL_OWNER_ADT:
              free_adt_move_list((pipe_mgr_adt_move_list_t **)&hdr->ml);
              break;
            case PIPE_MGR_TBL_OWNER_SELECT:
              free_sel_move_list((pipe_mgr_sel_move_list_t **)&hdr->ml);
              break;
            case PIPE_MGR_TBL_OWNER_STFUL:
              pipe_mgr_stful_free_ops(
                  (struct pipe_mgr_stful_op_list_t **)&hdr->ml);
              break;
            case PIPE_MGR_TBL_OWNER_METER:
              pipe_mgr_meter_free_ops(
                  (struct pipe_mgr_meter_op_list_t **)&hdr->ml);
              break;
            default:
              PIPE_MGR_DBGCHK(0);
          }
        }
      } else {
        enum pipe_mgr_table_owner_t owner;
        owner = pipe_mgr_sm_tbl_owner(d, hdr->tbl_hdl);
        if (PIPE_MGR_TBL_OWNER_NO_KEY != owner &&
            PIPE_MGR_TBL_OWNER_SELECT != owner) {
          PIPE_MGR_DBGCHK(hdr->ml);
        }
      }

      /* Free this header node since it has been processed. */
      struct pipe_mgr_sm_move_list_hdr *tmp = hdr->next;
      PIPE_MGR_FREE(hdr);
      hdr = tmp;
    }
    s->txn_mls[d] = NULL;

    if (pipe_mgr_device_present(d) && sh == pipe_mgr_get_pkt_gen_hdl(d)) {
      pipe_mgr_pktgen_txn_commit(d);
    }
  }
}

void pipe_mgr_sm_atom_cleanup(pipe_sess_hdl_t sh) {
  bf_dev_pipe_t pipes_list[PIPE_MGR_MAX_PIPES];
  unsigned nb_pipes = 0;

  pipe_mgr_sess_ctx_t *s = pipe_mgr_get_sess_ctx(sh, __func__, __LINE__);
  if (s == NULL) {
    PIPE_MGR_DBGCHK(0);
    return;
  }
  unsigned d;
  for (d = 0; d < PIPE_MGR_NUM_DEVICES; ++d) {
    pipe_mgr_sm_tbl_instance_t *sess_tbl;
    pipe_mgr_sm_tbl_info_t *t;
    for (sess_tbl = s->tables[d]; sess_tbl; sess_tbl = sess_tbl->next) {
      t = sess_tbl->tbl;
      get_sm_tbl_pipes_list(t, sh, pipes_list, &nb_pipes);
      if (PIPE_MGR_TBL_OWNER_EXM == t->owner) {
        pipe_mgr_exm_atom_cleanup(d, t->id);
      } else if (PIPE_MGR_TBL_OWNER_TRN == t->owner) {
        pipe_mgr_tcam_atom_cleanup(d, t->id, pipes_list, nb_pipes);
      } else if (PIPE_MGR_TBL_OWNER_ALPM == t->owner) {
        pipe_mgr_alpm_atom_cleanup(d, t->id, pipes_list, nb_pipes);
      } else {
        PIPE_MGR_DBGCHK(0);
      }
    }
  }
}

void pipe_mgr_sm_release(pipe_sess_hdl_t sh) {
  pipe_mgr_sess_ctx_t *s = pipe_mgr_get_sess_ctx(sh, __func__, __LINE__);
  struct pipe_mgr_dev_ctx *dev_ctx = NULL;
  if (s == NULL) {
    PIPE_MGR_DBGCHK(0);
    return;
  }
  for (bf_dev_id_t d = 0; d < PIPE_MGR_NUM_DEVICES; ++d) {
    if (!pipe_mgr_is_device_present(d)) continue;
    PIPE_MGR_LOCK_R(pipe_mgr_smtbl_mtx(d));
    dev_ctx = pipe_mgr_dev_ctx_(d);
    if (!dev_ctx) {
      PIPE_MGR_UNLOCK_R(pipe_mgr_smtbl_mtx(d));
      continue;
    }
    if (sh == dev_ctx->pkt_gen) {
      dev_ctx->pkt_gen = PIPE_MGR_MAX_SESSIONS;
    }
    if (sh == dev_ctx->hash_calc) {
      dev_ctx->hash_calc = PIPE_MGR_MAX_SESSIONS;
    }
    if (sh == dev_ctx->learn_ses) {
      dev_ctx->learn_ses = PIPE_MGR_MAX_SESSIONS;
    }
    while (s->mir_ses[d]) {
      pipe_mgr_sm_mir_info_t *m = s->mir_ses[d];
      m->sid = PIPE_MGR_MAX_SESSIONS;
      PIPE_MGR_DLL_REM(s->mir_ses[d], m, next, prev);
    }
    while (s->tables[d]) {
      int p;
      pipe_mgr_sm_tbl_instance_t *sess_tbl = s->tables[d];
      pipe_mgr_sm_tbl_info_t *t = sess_tbl->tbl;

      for (p = 0; p < PIPE_MGR_MAX_PIPES; p++)
        if (t->sid[p] == sh) t->sid[p] = PIPE_MGR_MAX_SESSIONS;
      sess_tbl->tbl = NULL;
      PIPE_MGR_DLL_REM(s->tables[d], sess_tbl, next, prev);
    }
    PIPE_MGR_UNLOCK_R(pipe_mgr_smtbl_mtx(d));
  }
}

static pipe_status_t create_virtual_dev_res_tbl(pipe_sess_hdl_t sh,
                                                bf_dev_id_t dev_id,
                                                pipe_mgr_sm_tbl_info_t *ti) {
  pipe_status_t rc;
  if (ti->initialized) return PIPE_SUCCESS;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return PIPE_INVALID_ARG;

  if (!pipe_mgr_is_device_virtual(dev_id)) {
    return PIPE_INVALID_ARG;
  }

  pipe_bitmap_t pbm;
  profile_id_t prof_id = ti->prof_id;
  rc = pipe_mgr_get_pipe_bmp_for_profile(
      dev_info, prof_id, &pbm, __func__, __LINE__);
  if (rc != PIPE_SUCCESS) {
    return rc;
  }

  rc = PIPE_UNEXPECTED;
  switch (ti->owner) {
    case PIPE_MGR_TBL_OWNER_EXM:
    case PIPE_MGR_TBL_OWNER_TRN:
    case PIPE_MGR_TBL_OWNER_ALPM:
    case PIPE_MGR_TBL_OWNER_PHASE0:
    case PIPE_MGR_TBL_OWNER_NO_KEY:
      /* Match tables should not be here... */
      PIPE_MGR_DBGCHK(0);
      return PIPE_INVALID_ARG;
    case PIPE_MGR_TBL_OWNER_ADT:
      rc = pipe_mgr_adt_tbl_init(dev_id, ti->id, prof_id, &pbm);
      break;
    case PIPE_MGR_TBL_OWNER_SELECT:
      rc = pipe_mgr_sel_tbl_create(sh, dev_id, ti->id, prof_id, &pbm);
      break;
    case PIPE_MGR_TBL_OWNER_STAT:
      rc = pipe_mgr_stat_tbl_init(sh, dev_id, ti->id, prof_id, &pbm);
      break;
    case PIPE_MGR_TBL_OWNER_METER:
      rc = pipe_mgr_meter_tbl_init(sh, dev_id, ti->id, prof_id, &pbm);
      break;
    case PIPE_MGR_TBL_OWNER_STFUL:
      rc = pipe_mgr_stful_tbl_add(sh, dev_id, ti->id, prof_id, &pbm);
      break;
    case PIPE_MGR_TBL_OWNER_NONE:
      /* Nothing should land here... */
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }
  if (rc == PIPE_SUCCESS) {
    ti->initialized = true;
  }

  /* Create all dependent resource tables as well. */
  for (int i = 1; i < ti->resource_count; ++i) {
    rc = create_virtual_dev_res_tbl(sh, dev_id, ti->resources[i]);
    if (rc != PIPE_SUCCESS && rc != PIPE_ALREADY_EXISTS) {
      return rc;
    }
  }
  return rc;
}

static pipe_status_t create_virtual_dev_mat(pipe_sess_hdl_t sh,
                                            bf_dev_id_t dev_id,
                                            pipe_mgr_sm_tbl_info_t *ti) {
  pipe_status_t rc;
  pipe_tbl_hdl_t tbl_hdl = ti->id;
  if (ti->initialized) return PIPE_SUCCESS;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return PIPE_INVALID_ARG;

  if (!pipe_mgr_is_device_virtual(dev_id)) {
    return PIPE_INVALID_ARG;
  }

  pipe_mat_tbl_info_t *mat_info =
      pipe_mgr_get_tbl_info(dev_id, tbl_hdl, __func__, __LINE__);
  if (!mat_info) {
    LOG_ERROR("%s:%d Table 0x%x not found in RMT database for device %d",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  /* Sanity check. */
  if (ti->prof_id != mat_info->profile_id) {
    PIPE_MGR_DBGCHK(ti->prof_id == mat_info->profile_id);
    return PIPE_UNEXPECTED;
  }

  pipe_bitmap_t pbm;
  profile_id_t prof_id = mat_info->profile_id;
  rc = pipe_mgr_get_pipe_bmp_for_profile(
      dev_info, prof_id, &pbm, __func__, __LINE__);
  if (rc != PIPE_SUCCESS) {
    return rc;
  }

  /* Create the match table. */
  rc = PIPE_UNEXPECTED;
  switch (ti->owner) {
    case PIPE_MGR_TBL_OWNER_EXM:
      rc = pipe_mgr_exm_tbl_init(dev_id, tbl_hdl, prof_id, &pbm);
      break;
    case PIPE_MGR_TBL_OWNER_TRN:
      rc = pipe_mgr_tcam_tbl_create(dev_id, tbl_hdl, prof_id, &pbm);
      break;
    case PIPE_MGR_TBL_OWNER_PHASE0:
      rc = pipe_mgr_phase0_tbl_init(dev_id, tbl_hdl, prof_id, &pbm);
      break;
    case PIPE_MGR_TBL_OWNER_NO_KEY:
      rc = PIPE_SUCCESS;
      break;
    case PIPE_MGR_TBL_OWNER_ALPM:
      /* ALPM is not supported for virtual devices. */
      PIPE_MGR_DBGCHK(0);
      return PIPE_INVALID_ARG;
    case PIPE_MGR_TBL_OWNER_ADT:
    case PIPE_MGR_TBL_OWNER_SELECT:
    case PIPE_MGR_TBL_OWNER_STAT:
    case PIPE_MGR_TBL_OWNER_METER:
    case PIPE_MGR_TBL_OWNER_STFUL:
      /* Resource tables should not be here, expect MATs only. */
      PIPE_MGR_DBGCHK(0);
      return PIPE_INVALID_ARG;
    case PIPE_MGR_TBL_OWNER_NONE:
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }
  if (rc != PIPE_SUCCESS) return rc;

  /* Create any direct and/or indirect resource tables, including idletime.
   * Some resource tables may have already been created if they are shared
   * across multiple MATs so ignore any "already exists" status codes. */
  if (pipe_mgr_mat_tbl_has_idle(dev_id, tbl_hdl)) {
    pipe_mgr_idle_tbl_create(dev_id, tbl_hdl, prof_id, &pbm);
  }
  for (int i = 1; i < ti->resource_count; ++i) {
    rc = create_virtual_dev_res_tbl(sh, dev_id, ti->resources[i]);
    if (rc != PIPE_SUCCESS && rc != PIPE_ALREADY_EXISTS) {
      return rc;
    }
  }

  /* Mark the table as initialized now since the default and static entry
   * helpers may attempt to reserve the table again which would attempt to
   * create the table again. */
  ti->initialized = true;

  /* Setup the initial default entry and any static entries on the table. */
  if (mat_info->default_info) {
    pipe_mgr_tbl_add_init_default_entry(sh, dev_info, mat_info);
  }
  /* If the table has static entries (e.g. a "const entries" block in the P4)
   * add them now since the table has been reated.  Since the table has just
   * been created it must be in symmetric mode so use PIPE_ALL in the static
   * entry helper. */
  dev_target_t dev_tgt;
  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = BF_DEV_PIPE_ALL;
  return static_entry_add_helper(sh, dev_tgt, mat_info);
}

static pipe_status_t create_virtual_dev_tbl(pipe_sess_hdl_t sh,
                                            bf_dev_id_t dev_id,
                                            pipe_mgr_sm_tbl_info_t *ti) {
  if (PIPE_HDL_TYPE_MAT_TBL == PIPE_GET_HDL_TYPE(ti->id)) {
    return create_virtual_dev_mat(sh, dev_id, ti);
  } else {
    return create_virtual_dev_res_tbl(sh, dev_id, ti);
  }
}

/** \brief get_session_pipes_list:
 *         Gets the list of table session relevant pipes.
 *
 * \param t points to the session manager table info structure.
 * \param sh is the session ID.
 * \param pipes_list points to the caller allocated variable to store the list
 *        of the relevant pipes.
 * \param nb_pipes points to the caller allocated variable to store the number
 *        of relevant pipes.
 */
static void get_sm_tbl_pipes_list(pipe_mgr_sm_tbl_info_t *t,
                                  pipe_sess_hdl_t sh,
                                  bf_dev_pipe_t *pipes_list,
                                  unsigned *nb_pipes) {
  int i, j = 0;

  for (i = 0; i < PIPE_MGR_MAX_PIPES; i++)
    if (t->sid[i] == sh) pipes_list[j++] = i;
  *nb_pipes = j;
}

/** \brief in_table_list:
 *         Checks if a given table is already in a DLL table list.
 *
 * \param first_tbl Pointer to the first table (head) of the DLL table list.
 * \param t Pointer to the table to check if it is already in the table list.
 * \return TRUE if the table is in the table list or FALSE if not.
 */
static bool in_table_list(pipe_mgr_sm_tbl_instance_t *first_tbl,
                          pipe_mgr_sm_tbl_instance_t *t) {
  pipe_mgr_sm_tbl_instance_t *cur;

  if (!t) {
    /* Should not happen. */
    PIPE_MGR_DBGCHK(t);
    return true;
  }

  for (cur = first_tbl; cur; cur = cur->next) {
    if (cur == t) {
      /* The given table (t) is already in the table list. */
      return true;
    }
  }

  return false;
}

/** \brief get_mat_pipes_list:
 *         Gets the list of the relevant pipes for a match table access.
 *
 * - If the table is symmetric or dev_tgt has BF_DEV_PIPE_ALL, then all pipes of
 *   the profile are reserved.
 *
 * - If the table is asymmetric, the single pipe specified in the dev_tgt is
 *   reserved. We also make sure that the provided pipe belongs to the profile.
 *
 * - If the table has custom scopes, then all pipes which are part of the scope
 *   identified by the dev_tgt are reserved.
 *
 * \param dev_tgt is the device target structure.
 * \param mat_tbl_hdl is the match table handle.
 * \param pipes_list points to the caller allocated variable to store the list
 *        of the relevant pipes.
 * \return the number of relevant pipes
 */
static int get_mat_pipes_list(dev_target_t dev_tgt,
                              pipe_mat_tbl_hdl_t mat_tbl_hdl,
                              bf_dev_pipe_t *pipes_list) {
  rmt_dev_profile_info_t *profile_info = NULL;
  int nb_pipes = 0;
  uint32_t q = 0;
  int i = 0;
  int j;

  pipe_mat_tbl_info_t *mat_tbl_info =
      pipe_mgr_get_tbl_info(dev_tgt.device_id, mat_tbl_hdl, __func__, __LINE__);
  if (!mat_tbl_info) return 0;

  profile_info = pipe_mgr_get_profile_info(
      dev_tgt.device_id, mat_tbl_info->profile_id, __func__, __LINE__);
  if (!profile_info) return 0;

  if (mat_tbl_info->symmetric || dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL) {
    /* Verify table access on all pipes. */
    PIPE_BITMAP_ITER(&profile_info->pipe_bmp, q) pipes_list[i++] = q;
    nb_pipes = i;
  } else if (mat_tbl_info->scope_value == 0) {
    /* Asymmetric table, verify table access on the given pipe. Validate first
     * that the given pipe belongs to the profile. */
    PIPE_BITMAP_ITER(&profile_info->pipe_bmp, q) {
      if (q == dev_tgt.dev_pipe_id) {
        pipes_list[i++] = dev_tgt.dev_pipe_id;
        nb_pipes = 1;
        break;
      }
    }
  } else {
    /* This is a user defined scope. */
    /* Verify table access on all pipes of the given pipe's scope. */
    pipe_mgr_tbl_prop_args_t args;
    args.value = mat_tbl_info->scope_value;
    for (i = 0; i < PIPE_MGR_MAX_USER_DEFINED_SCOPES; i++) {
      scope_pipes_t pipes = args.user_defined_entry_scope[i];
      if (pipes & (0x1 << dev_tgt.dev_pipe_id)) {
        for (j = 0; j < (int)(sizeof(scope_pipes_t) * 8); j++) {
          if (pipes & (0x1 << j)) pipes_list[q++] = j;
        }
        nb_pipes = q;
        break;
      }
    }
  }
  return nb_pipes;
}

/** \brief get_adt_info:
 *         Gets the ADT resource information.
 *
 * \param dev_tgt is the device target structure.
 * \param adt_tbl_hdl is the ADT table handle.
 * \param profile_info points to a variable to store the table profile info.
 * \param symmetric points to a variable to store the table symmetric state.
 * \param num_scopes points to a variable to store the table num_scopes.
 * \param scope_pipe_bmp points to a variable to store the table scope_bit_bpm.
 * \return PIPE_SUCCESS if the info is correctly retrieved.
 */
static pipe_status_t get_adt_info(dev_target_t dev_tgt,
                                  pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                  rmt_dev_profile_info_t **profile_info,
                                  bool *symmetric,
                                  scope_num_t *num_scopes,
                                  scope_pipes_t **scope_pipe_bmp) {
  pipe_mgr_adt_t *adt_tbl = NULL;

  adt_tbl = pipe_mgr_adt_get(dev_tgt.device_id, adt_tbl_hdl);
  if (!adt_tbl) {
    LOG_ERROR(
        "%s:%d : Could not get the adt table info for table hdl 0x%x"
        " device id %d",
        __func__,
        __LINE__,
        adt_tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  *profile_info = pipe_mgr_get_profile_info(
      dev_tgt.device_id, adt_tbl->profile_id, __func__, __LINE__);
  if (*profile_info == NULL) return PIPE_OBJ_NOT_FOUND;

  *symmetric = adt_tbl->symmetric;
  *num_scopes = adt_tbl->num_scopes;
  *scope_pipe_bmp = &adt_tbl->scope_pipe_bmp[0];
  return PIPE_SUCCESS;
}

/** \brief get_select_info:
 *         Gets the selector table resource information.
 *
 * \param dev_tgt is the device target structure.
 * \param tbl_hdl is the selector table handle.
 * \param profile_info points to a variable to store the table profile info.
 * \param symmetric points to a variable to store the table symmetric state.
 * \param num_scopes points to a variable to store the table num_scopes.
 * \param scope_pipe_bmp points to a variable to store the table scope_bit_bpm.
 * \return PIPE_SUCCESS if the info is correctly retrieved.
 */
static pipe_status_t get_select_info(dev_target_t dev_tgt,
                                     pipe_sel_tbl_hdl_t tbl_hdl,
                                     rmt_dev_profile_info_t **profile_info,
                                     bool *symmetric,
                                     scope_num_t *num_scopes,
                                     scope_pipes_t **scope_pipe_bmp) {
  sel_tbl_info_t *sel_tbl = NULL;

  sel_tbl = pipe_mgr_sel_tbl_info_get(dev_tgt.device_id, tbl_hdl, false);
  if (!sel_tbl) {
    LOG_ERROR("%s:%d Selector table for device id %d, tbl hdl 0x%x not found",
              __func__,
              __LINE__,
              dev_tgt.device_id,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  *profile_info = pipe_mgr_get_profile_info(
      dev_tgt.device_id, sel_tbl->profile_id, __func__, __LINE__);
  if (*profile_info == NULL) return PIPE_OBJ_NOT_FOUND;

  *symmetric = sel_tbl->is_symmetric;
  *num_scopes = sel_tbl->num_scopes;
  *scope_pipe_bmp = &sel_tbl->scope_pipe_bmp[0];
  return PIPE_SUCCESS;
}

/** \brief get_stat_info:
 *         Gets the statistic table resource information.
 *
 * \param dev_tgt is the device target structure.
 * \param stat_tbl_hdl is the statistic table handle.
 * \param profile_info points to a variable to store the table profile info.
 * \param symmetric points to a variable to store the table symmetric state.
 * \param num_scopes points to a variable to store the table num_scopes.
 * \param scope_pipe_bmp points to a variable to store the table scope_bit_bpm.
 * \return PIPE_SUCCESS if the info is correctly retrieved.
 */
static pipe_status_t get_stat_info(dev_target_t dev_tgt,
                                   pipe_stat_tbl_hdl_t stat_tbl_hdl,
                                   rmt_dev_profile_info_t **profile_info,
                                   bool *symmetric,
                                   scope_num_t *num_scopes,
                                   scope_pipes_t **scope_pipe_bmp) {
  pipe_mgr_stat_tbl_t *stat_tbl = NULL;

  stat_tbl = pipe_mgr_stat_tbl_get(dev_tgt.device_id, stat_tbl_hdl);
  if (!stat_tbl) {
    LOG_ERROR("%s:%d Stat table for device id %d, tbl hdl 0x%x not found",
              __func__,
              __LINE__,
              dev_tgt.device_id,
              stat_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  *profile_info = pipe_mgr_get_profile_info(
      dev_tgt.device_id, stat_tbl->profile_id, __func__, __LINE__);
  if (*profile_info == NULL) return PIPE_OBJ_NOT_FOUND;

  *symmetric = stat_tbl->symmetric;
  *num_scopes = stat_tbl->num_scopes;
  *scope_pipe_bmp = &stat_tbl->scope_pipe_bmp[0];
  return PIPE_SUCCESS;
}

/** \brief get_meter_info:
 *         Gets the meter table resource information.
 *
 * \param dev_tgt is the device target structure.
 * \param meter_tbl_hdl is the meter table handle.
 * \param profile_info points to a variable to store the table profile info.
 * \param symmetric points to a variable to store the table symmetric state.
 * \param num_scopes points to a variable to store the table num_scopes.
 * \param scope_pipe_bmp points to a variable to store the table scope_bit_bpm.
 * \return PIPE_SUCCESS if the info is correctly retrieved.
 */
static pipe_status_t get_meter_info(dev_target_t dev_tgt,
                                    pipe_meter_tbl_hdl_t meter_tbl_hdl,
                                    rmt_dev_profile_info_t **profile_info,
                                    bool *symmetric,
                                    scope_num_t *num_scopes,
                                    scope_pipes_t **scope_pipe_bmp) {
  pipe_mgr_meter_tbl_t *meter_tbl = NULL;

  meter_tbl = pipe_mgr_meter_tbl_get(dev_tgt.device_id, meter_tbl_hdl);
  if (!meter_tbl) {
    LOG_ERROR("%s:%d Meter table for device id %d, tbl hdl 0x%x not found",
              __func__,
              __LINE__,
              dev_tgt.device_id,
              meter_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  *profile_info = pipe_mgr_get_profile_info(
      dev_tgt.device_id, meter_tbl->profile_id, __func__, __LINE__);
  if (*profile_info == NULL) return PIPE_OBJ_NOT_FOUND;

  *symmetric = meter_tbl->symmetric;
  *num_scopes = meter_tbl->num_scopes;
  *scope_pipe_bmp = &meter_tbl->scope_pipe_bmp[0];
  return PIPE_SUCCESS;
}

/** \brief get_stful_info:
 *         Gets the stateful table resource information.
 *
 * \param dev_tgt is the device target structure.
 * \param stful_tbl_hdl is the stateful table handle.
 * \param profile_info points to a variable to store the table profile info.
 * \param symmetric points to a variable to store the table symmetric state.
 * \param num_scopes points to a variable to store the table num_scopes.
 * \param scope_pipe_bmp points to a variable to store the table scope_bit_bpm.
 * \return PIPE_SUCCESS if the info is correctly retrieved.
 */
static pipe_status_t get_stful_info(dev_target_t dev_tgt,
                                    pipe_stful_tbl_hdl_t stful_tbl_hdl,
                                    rmt_dev_profile_info_t **profile_info,
                                    bool *symmetric,
                                    scope_num_t *num_scopes,
                                    scope_pipes_t **scope_pipe_bmp) {
  struct pipe_mgr_stful_tbl *stful_tbl = NULL;

  stful_tbl = pipe_mgr_stful_tbl_get(dev_tgt.device_id, stful_tbl_hdl);
  if (!stful_tbl) {
    LOG_ERROR("%s:%d Stateful table for device id %d, tbl hdl 0x%x not found",
              __func__,
              __LINE__,
              dev_tgt.device_id,
              stful_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  *profile_info = pipe_mgr_get_profile_info(
      dev_tgt.device_id, stful_tbl->profile_id, __func__, __LINE__);
  if (*profile_info == NULL) return PIPE_OBJ_NOT_FOUND;

  *symmetric = stful_tbl->symmetric;
  *num_scopes = stful_tbl->num_scopes;
  *scope_pipe_bmp = &stful_tbl->scope_pipe_bmp[0];
  return PIPE_SUCCESS;
}

/** \brief get_resource_pipes_list:
 *         Gets the list of the relevant pipes for a resource table access.
 *
 * - If the table is symmetric or dev_tgt has BF_DEV_PIPE_ALL, then all pipes of
 *   the profile are reserved.
 *
 * - If the table is asymmetric, then all pipes belonging to the scope of the
 *   single pipe specified in the dev_tgt are reserved.
 *
 * \param dev_tgt is the device target structure.
 * \param tbl_hdl is the resource table handle.
 * \param pipes_list points to the caller allocated variable to store the list
 *        of the relevant pipes.
 * \return the number of relevant pipes
 */
static int get_resource_pipes_list(dev_target_t dev_tgt,
                                   pipe_tbl_hdl_t tbl_hdl,
                                   bf_dev_pipe_t *pipes_list) {
  pipe_hdl_type_t tbl_type = PIPE_GET_HDL_TYPE(tbl_hdl);
  rmt_dev_profile_info_t *profile = NULL;
  scope_pipes_t *scope_pipe_bmp = NULL;
  scope_num_t num_scopes = 0;
  bool symmetric = false;
  pipe_status_t rc;
  int nb_pipes = 0;
  uint32_t q = 0;
  int i = 0;

  if (tbl_type == PIPE_HDL_TYPE_ADT_TBL)
    rc = get_adt_info(
        dev_tgt, tbl_hdl, &profile, &symmetric, &num_scopes, &scope_pipe_bmp);
  else if (tbl_type == PIPE_HDL_TYPE_SEL_TBL)
    rc = get_select_info(
        dev_tgt, tbl_hdl, &profile, &symmetric, &num_scopes, &scope_pipe_bmp);
  else if (tbl_type == PIPE_HDL_TYPE_STAT_TBL)
    rc = get_stat_info(
        dev_tgt, tbl_hdl, &profile, &symmetric, &num_scopes, &scope_pipe_bmp);
  else if (tbl_type == PIPE_HDL_TYPE_METER_TBL)
    rc = get_meter_info(
        dev_tgt, tbl_hdl, &profile, &symmetric, &num_scopes, &scope_pipe_bmp);
  else if (tbl_type == PIPE_HDL_TYPE_STFUL_TBL)
    rc = get_stful_info(
        dev_tgt, tbl_hdl, &profile, &symmetric, &num_scopes, &scope_pipe_bmp);
  else
    rc = PIPE_INVALID_ARG;

  if (rc != PIPE_SUCCESS || !profile) return 0;

  if (symmetric || dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL) {
    /* Verify table access on all pipes. */
    PIPE_BITMAP_ITER(&profile->pipe_bmp, q) pipes_list[i++] = q;
    nb_pipes = i;
  } else {
    /* Verify table on all pipes of the given pipe_id's scope */
    pipe_bitmap_t local_pipe_bmp;
    PIPE_BITMAP_INIT(&local_pipe_bmp, PIPE_BMP_SIZE);
    PIPE_BITMAP_SET(&local_pipe_bmp, dev_tgt.dev_pipe_id);
    pipe_mgr_get_all_pipes_in_scope(
        dev_tgt.dev_pipe_id, num_scopes, &scope_pipe_bmp[0], &local_pipe_bmp);
    PIPE_BITMAP_ITER(&local_pipe_bmp, q) pipes_list[i++] = q;
    nb_pipes = i;
  }
  return nb_pipes;
}

/** \brief get_pipes_list:
 *         Gets the list of the relevant pipes for a table access.
 *
 * \param dev_tgt is the device target structure.
 * \param sh is the session handle.
 * \param tbl points to the session manager table info structure.
 * \param pipes_list points to the caller allocated variable to store the list
 *        of the relevant pipes.
 * \return the number of relevant pipes
 */
static int get_pipes_list(dev_target_t dev_tgt,
                          pipe_sess_hdl_t sh,
                          pipe_mgr_sm_tbl_info_t *tbl,
                          bf_dev_pipe_t *pipes_list) {
  pipe_hdl_type_t tbl_type = PIPE_GET_HDL_TYPE(tbl->id);
  int nb_pipes = 0;

  if (tbl_type == PIPE_HDL_TYPE_MAT_TBL) {
    nb_pipes = get_mat_pipes_list(dev_tgt, tbl->id, pipes_list);
  } else if (tbl_type == PIPE_HDL_TYPE_ADT_TBL ||
             tbl_type == PIPE_HDL_TYPE_SEL_TBL ||
             tbl_type == PIPE_HDL_TYPE_STAT_TBL ||
             tbl_type == PIPE_HDL_TYPE_METER_TBL ||
             tbl_type == PIPE_HDL_TYPE_STFUL_TBL) {
    nb_pipes = get_resource_pipes_list(dev_tgt, tbl->id, pipes_list);
  } else {
    LOG_ERROR("%s:%d Invalid table type %d for tbl hdl 0x%x",
              __func__,
              __LINE__,
              tbl_type,
              tbl->id);
  }

  if (!nb_pipes) {
    LOG_ERROR("%s:%d Invalid pipe_id (%d) for session %u on dev %u",
              __func__,
              __LINE__,
              dev_tgt.dev_pipe_id,
              sh,
              dev_tgt.device_id);
  }

  return nb_pipes;
}

static void verify_tbl_access(pipe_sess_hdl_t sh,
                              dev_target_t dev_tgt,
                              pipe_mgr_sm_tbl_info_t **tbl,
                              int count,
                              bool rsvRsc,
                              bool log,
                              bool ignore_err,
                              bool *okay,
                              bool *held) {
  bf_dev_id_t dev_id = dev_tgt.device_id;
  bf_dev_pipe_t pipes_list[PIPE_MGR_MAX_PIPES];
  pipe_mgr_sm_tbl_info_t *t = NULL;
  bool in_use = false, by_txn_or_batch = false;
  bool cur_in_use;
  int nb_pipes = 0;
  uint32_t p;
  int i, j;

  if (0 >= count) {
    if (okay) *okay = true;
    if (held) *held = false;
    return;
  }

#ifndef PIPE_MGR_PER_PIPE_TABLE_LOCK_ENABLE
  /* Lock the table for all pipes if the per pipe table lock is not enabled. */
  dev_tgt.dev_pipe_id = BF_DEV_PIPE_ALL;
#endif

  PIPE_MGR_LOCK_R(pipe_mgr_smtbl_mtx(dev_id));

  for (i = 0; i < count; i++) {
    t = tbl[i];
    if (!t) {
      continue;
    }

    /* Tables for virtual devices are initialized at their first access. */
    if (!t->initialized) create_virtual_dev_tbl(sh, dev_id, t);

    if (!nb_pipes) {
      /* This code is only executed once and must be run after the virtual
       * device's table initialization when the latter is required. */
      nb_pipes = get_pipes_list(dev_tgt, sh, tbl[0], pipes_list);
      if (!nb_pipes) {
        /* Should not happen. */
        if (okay) *okay = false;
        if (held) *held = true;
        PIPE_MGR_UNLOCK_R(pipe_mgr_smtbl_mtx(dev_id));
        return;
      }
    }

    cur_in_use = false;
    for (j = 0; j < nb_pipes && !cur_in_use; j++) {
      p = pipes_list[j];
      if ((t->sid[p] != PIPE_MGR_MAX_SESSIONS) && (t->sid[p] != sh)) {
        /* Another session is accessing the table. */
        cur_in_use = in_use = true;
        by_txn_or_batch = by_txn_or_batch || pipe_mgr_sess_in_txn(t->sid[p]) ||
                          pipe_mgr_sess_in_batch(t->sid[p]);
        if (by_txn_or_batch && !ignore_err) {
          LOG_ERROR(
              "Session %u trying to access table handle %#x "
              "on dev %u, locked by session %u pipe %d "
              "provided dev_tgt.dev_pipe_id is 0x%x",
              sh,
              t->id,
              dev_id,
              t->sid[p],
              p,
              dev_tgt.dev_pipe_id);
        } else if (log) {
          LOG_DBG(
              "Session %u trying to access table handle %#x "
              "on dev %u, used by session %u pipe %d "
              "provided dev_tgt.dev_pipe_id is 0x%x",
              sh,
              t->id,
              dev_id,
              t->sid[p],
              p,
              dev_tgt.dev_pipe_id);
        }
      }
    }
  }

  if (!in_use) {
    bool same_sh;
    for (i = 0; i < count; i++) {
      t = tbl[i];
      if (!t) {
        continue;
      }
      same_sh = true;
      for (j = 0; j < nb_pipes; j++) {
        p = pipes_list[j];
        if (t->sid[p] != sh) {
          same_sh = false;
        } else {
          /* if one of the pipe, in pipe_list, is alrealdy using the same
           * session hanlde (sh) then we mark all other pipes in that list as
           * in use for that table. */
          same_sh = true;
          int k;
          for (k = 0; k < nb_pipes; k++) {
            p = pipes_list[k];
            t->sid[p] = sh;
          }
          break;
        }
      }
      if (same_sh) continue;

      if (rsvRsc) {
        /* Reserve the table by writting the session id to the table's state. */
        for (j = 0; j < nb_pipes; j++) {
          p = pipes_list[j];
          t->sid[p] = sh;
        }

        pipe_mgr_sess_ctx_t *s = pipe_mgr_get_sess_ctx(sh, __func__, __LINE__);
        if (s == NULL) {
          LOG_ERROR(
              "%s:%d Error in getting session ctx for session %u dev id %d",
              __func__,
              __LINE__,
              sh,
              dev_id);
          PIPE_MGR_UNLOCK_R(pipe_mgr_smtbl_mtx(dev_id));
          PIPE_MGR_DBGCHK(0);
          return;
        }

        /* Make sure that the session is not already accessing this table. */
        if (!in_table_list(s->tables[dev_id], &t->instance[sh])) {
          /* The session is not accessing this table; link the table into a list
           * against the session and device so that it can be quickly found once
           * the session is done with it. */
          t->instance[sh].tbl = t;
          PIPE_MGR_DLL_PP(s->tables[dev_id], &t->instance[sh], next, prev);
        }
      }
    }
    if (okay) *okay = true;
    if (held) *held = false;
  } else {
    if (okay) *okay = false;
    if (held) *held = by_txn_or_batch;
  }
  PIPE_MGR_UNLOCK_R(pipe_mgr_smtbl_mtx(dev_id));
}

static bool verify_tbl_access_blocking(pipe_sess_hdl_t sh,
                                       dev_target_t dev_tgt,
                                       pipe_mgr_sm_tbl_info_t **tbl,
                                       int count,
                                       bool rsvRsc,
                                       bool ignore_err) {
  bool available = false, held = false, first = true;
  do {
    verify_tbl_access(
        sh, dev_tgt, tbl, count, rsvRsc, first, ignore_err, &available, &held);
    first = false;
  } while (!available && !held);
  PIPE_MGR_DBGCHK((available && !held) || (!available && held));
  return available;
}

pipe_status_t pipe_mgr_verify_pkt_gen_access(pipe_sess_hdl_t sh,
                                             bf_dev_id_t dev_id,
                                             bool rsvRsc) {
  bool available = false, held = false;
  if (!pipe_mgr_device_present(dev_id)) {
    LOG_ERROR("Device %u is not present", dev_id);
    return PIPE_INVALID_ARG;
  }
  do {
    PIPE_MGR_LOCK_R(pipe_mgr_smtbl_mtx(dev_id));

    available = pipe_mgr_get_pkt_gen_hdl(dev_id) == sh ||
                pipe_mgr_get_pkt_gen_hdl(dev_id) == PIPE_MGR_MAX_SESSIONS;
    held = available
               ? false
               : (pipe_mgr_sess_in_txn(pipe_mgr_get_pkt_gen_hdl(dev_id)) ||
                  pipe_mgr_sess_in_batch(pipe_mgr_get_pkt_gen_hdl(dev_id)));

    if (available && rsvRsc) {
      pipe_mgr_set_pkt_gen_hdl(dev_id, sh);
    }

    PIPE_MGR_UNLOCK_R(pipe_mgr_smtbl_mtx(dev_id));
  } while (!available && !held);
  PIPE_MGR_DBGCHK((available && !held) || (!available && held));
  return available ? PIPE_SUCCESS : PIPE_TABLE_LOCKED;
}

pipe_status_t pipe_mgr_verify_hash_calc_access(pipe_sess_hdl_t sh,
                                               bf_dev_id_t dev_id,
                                               bool rsvRsc) {
  bool available = false, held = false;
  if (!pipe_mgr_device_present(dev_id)) {
    LOG_ERROR("Device %u is not present", dev_id);
    return PIPE_INVALID_ARG;
  }
  do {
    PIPE_MGR_LOCK_R(pipe_mgr_smtbl_mtx(dev_id));

    available = pipe_mgr_get_hash_calc_hdl(dev_id) == sh ||
                pipe_mgr_get_hash_calc_hdl(dev_id) == PIPE_MGR_MAX_SESSIONS;
    held = available
               ? false
               : (pipe_mgr_sess_in_txn(pipe_mgr_get_hash_calc_hdl(dev_id)) ||
                  pipe_mgr_sess_in_batch(pipe_mgr_get_hash_calc_hdl(dev_id)));

    if (available && rsvRsc) {
      pipe_mgr_set_hash_calc_hdl(dev_id, sh);
    }

    PIPE_MGR_UNLOCK_R(pipe_mgr_smtbl_mtx(dev_id));
  } while (!available && !held);
  PIPE_MGR_DBGCHK((available && !held) || (!available && held));
  return available ? PIPE_SUCCESS : PIPE_TABLE_LOCKED;
}

pipe_status_t pipe_mgr_verify_lrn_ses_access(pipe_sess_hdl_t sh,
                                             bf_dev_id_t dev_id,
                                             bool rsvRsc) {
  bool available = false, held = false;
  if (!pipe_mgr_device_present(dev_id)) {
    LOG_ERROR("Device %u is not present", dev_id);
    return PIPE_INVALID_ARG;
  }
  do {
    PIPE_MGR_LOCK_R(pipe_mgr_smtbl_mtx(dev_id));

    available = pipe_mgr_get_lrn_ses_hdl(dev_id) == sh ||
                pipe_mgr_get_lrn_ses_hdl(dev_id) == PIPE_MGR_MAX_SESSIONS;
    held = available
               ? false
               : (pipe_mgr_sess_in_txn(pipe_mgr_get_lrn_ses_hdl(dev_id)) ||
                  pipe_mgr_sess_in_batch(pipe_mgr_get_lrn_ses_hdl(dev_id)));

    if (available && rsvRsc) {
      pipe_mgr_set_lrn_ses_hdl(dev_id, sh);
    }

    PIPE_MGR_UNLOCK_R(pipe_mgr_smtbl_mtx(dev_id));
  } while (!available && !held);
  PIPE_MGR_DBGCHK((available && !held) || (!available && held));
  return available ? PIPE_SUCCESS : PIPE_TABLE_LOCKED;
}

pipe_status_t pipe_mgr_verify_mir_ses_access(pipe_sess_hdl_t sh,
                                             bf_dev_id_t dev_id,
                                             bf_mirror_id_t sid,
                                             bool rsvRsc) {
  pipe_status_t rc;
  bool available = false, held = false;
  if (!pipe_mgr_device_present(dev_id)) {
    LOG_ERROR("Device %u is not present", dev_id);
    return PIPE_INVALID_ARG;
  }
  do {
    PIPE_MGR_LOCK_R(pipe_mgr_smtbl_mtx(dev_id));
    pipe_sess_hdl_t current;
    rc = pipe_mgr_get_mir_ses_hdl(dev_id, sid, &current);
    if (rc != PIPE_SUCCESS) {
      PIPE_MGR_UNLOCK_R(pipe_mgr_smtbl_mtx(dev_id));
      LOG_ERROR(
          "Dev %d: Error %s while session %u verified access for mirror-id %u",
          dev_id,
          pipe_str_err(rc),
          sh,
          sid);
      return rc;
    }

    available = current == sh || current == PIPE_MGR_MAX_SESSIONS;
    held = available ? false
                     : (pipe_mgr_sess_in_txn(current) ||
                        pipe_mgr_sess_in_batch(current));

    if (available && rsvRsc) {
      rc = pipe_mgr_set_mir_ses_hdl(dev_id, sid, sh);
      if (rc != PIPE_SUCCESS) {
        PIPE_MGR_UNLOCK_R(pipe_mgr_smtbl_mtx(dev_id));
        return rc;
      }
    }

    PIPE_MGR_UNLOCK_R(pipe_mgr_smtbl_mtx(dev_id));
  } while (!available && !held);
  PIPE_MGR_DBGCHK((available && !held) || (!available && held));
  return available ? PIPE_SUCCESS : PIPE_TABLE_LOCKED;
}

static pipe_status_t pipe_mgr_verify_tbl_access_int(pipe_sess_hdl_t sh,
                                                    dev_target_t dev_tgt,
                                                    pipe_tbl_hdl_t th,
                                                    bool rsvRsc,
                                                    bool ignore_err) {
  /* Look up the table to be sure it exists. */
  bf_dev_id_t dev_id = dev_tgt.device_id;
  bool ret = false;
  if (!pipe_mgr_device_present(dev_id)) {
    LOG_ERROR("Device %u is not present", dev_id);
    return PIPE_INVALID_ARG;
  }
  pipe_mgr_sm_tbl_info_t *t = getTblInfoByHdl(dev_id, th);
  if (!t) {
    LOG_ERROR(
        "Invalid tbl %#x on device %u requested by session %u", th, dev_id, sh);
    ret = false;
    return PIPE_INVALID_ARG;
  }

  ret = verify_tbl_access_blocking(
      sh, dev_tgt, t->resources, t->resource_count, rsvRsc, ignore_err);

  if (!ret) {
    uint32_t p;
    int i;

#ifndef PIPE_MGR_PER_PIPE_TABLE_LOCK_ENABLE
    dev_tgt.dev_pipe_id = BF_DEV_PIPE_ALL;
#endif
    for (i = 0; i < t->resource_count; ++i) {
      for (p = 0; p < PIPE_MGR_MAX_PIPES; p++) {
        if (t->resources[i]->sid[p] != PIPE_MGR_MAX_SESSIONS &&
            t->resources[i]->sid[p] != sh && !ignore_err) {
          LOG_ERROR(
              "Session %u trying to access table %#x (for table %#x) on pipe"
              "%u dev %u, held by session %u",
              sh,
              t->resources[i]->id,
              th,
              p,
              dev_id,
              t->resources[i]->sid[p]);
        }
      }
    }
    return PIPE_TABLE_LOCKED;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_verify_pipe_tbl_access(pipe_sess_hdl_t sh,
                                              dev_target_t dev_tgt,
                                              pipe_tbl_hdl_t th,
                                              bool rsvRsc) {
  return pipe_mgr_verify_tbl_access_int(sh, dev_tgt, th, rsvRsc, false);
}

pipe_status_t pipe_mgr_verify_tbl_access(pipe_sess_hdl_t sh,
                                         bf_dev_id_t dev_id,
                                         pipe_tbl_hdl_t th,
                                         bool rsvRsc) {
  dev_target_t dev_tgt = {dev_id, BF_DEV_PIPE_ALL};
  return pipe_mgr_verify_tbl_access_int(sh, dev_tgt, th, rsvRsc, false);
}

pipe_status_t pipe_mgr_verify_tbl_access_ignore_err(pipe_sess_hdl_t sh,
                                                    bf_dev_id_t dev_id,
                                                    pipe_tbl_hdl_t th,
                                                    bool rsvRsc) {
  dev_target_t dev_tgt = {dev_id, BF_DEV_PIPE_ALL};
  return pipe_mgr_verify_tbl_access_int(sh, dev_tgt, th, rsvRsc, true);
}

pipe_status_t pipe_mgr_sm_init(pipe_sess_hdl_t sh, bf_dev_id_t dev_id) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_sm_tbl_info_t **dst_smti, *smti, *t;
  bf_map_t *sm_tbl_db;
  rmt_dev_tbl_info_t *tbl_info = NULL;
  unsigned tblCnt = 0;
  uint32_t p = 0;
  uint32_t pipe;
  uint32_t i;
  profile_id_t profile_id = 0;
  pipe_bitmap_t *pipe_bmp;
  rmt_dev_info_t *di = pipe_mgr_get_dev_info(dev_id);
  if (!di) {
    return PIPE_INVALID_ARG;
  }
  bool v = di->virtual_device;

  if (!(dst_smti = pipe_mgr_sm_tbl_info(dev_id)) ||
      !(sm_tbl_db = pipe_mgr_sm_tbl_db(dev_id))) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }

  /* Allocate a hash table to hold information about which sessions use which
   * tables.  Allocate memory for the Session Management table info structs
   * as well. */
  for (p = 0; p < di->num_pipeline_profiles; p++) {
    tbl_info = &(di->profile_info[p]->tbl_info_list);
    tblCnt += tbl_info->num_mat_tbls + tbl_info->num_adt_tbls +
              tbl_info->num_stat_tbls + tbl_info->num_meter_tbls +
              tbl_info->num_sful_tbls + tbl_info->num_select_tbls;
  }
  smti = PIPE_MGR_MALLOC(tblCnt * sizeof(pipe_mgr_sm_tbl_info_t));
  if (!smti) {
    LOG_ERROR("Failed to allocate table info (0x%zx bytes) at %s:%d",
              tblCnt * sizeof(pipe_mgr_sm_tbl_info_t),
              __func__,
              __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  PIPE_MGR_MEMSET(smti, 0, tblCnt * sizeof(pipe_mgr_sm_tbl_info_t));

  *dst_smti = smti;
  *sm_tbl_db = NULL;

  pipe_mgr_set_pkt_gen_hdl(dev_id, PIPE_MGR_MAX_SESSIONS);
  pipe_mgr_set_hash_calc_hdl(dev_id, PIPE_MGR_MAX_SESSIONS);
  pipe_mgr_set_lrn_ses_hdl(dev_id, PIPE_MGR_MAX_SESSIONS);

  bf_mirror_id_t mirror_sess_max = 0;
  if (pipe_mgr_get_max_mirror_sessions(
          dev_id, BF_MIRROR_TYPE_COAL, &mirror_sess_max) != PIPE_SUCCESS) {
    LOG_ERROR("Error getting max mirror session id at device %d", dev_id);
    return PIPE_INVALID_ARG;
  }

  pipe_mgr_sm_mir_info_t **dev_mir_sess;
  if (!(dev_mir_sess = pipe_mgr_mir_ses_hdl_array_get(dev_id))) {
    LOG_ERROR("Error getting mirror session array on device %d", dev_id);
    PIPE_MGR_DBGCHK(dev_mir_sess != NULL);
    return PIPE_UNEXPECTED;
  }
  if (*dev_mir_sess) {
    LOG_ERROR("Error, mirror session array already allocated on device %d",
              dev_id);
    PIPE_MGR_DBGCHK(*dev_mir_sess == NULL);
    return PIPE_UNEXPECTED;
  }

  *dev_mir_sess = PIPE_MGR_CALLOC(mirror_sess_max + 1, sizeof **dev_mir_sess);
  if (!*dev_mir_sess) {
    LOG_ERROR("%s:%d Memory alloc failed, dev %d", __func__, __LINE__, dev_id);
    return PIPE_NO_SYS_RESOURCES;
  }
  for (i = 0; i <= mirror_sess_max; ++i) {
    (*dev_mir_sess)[i].mir_sess_id = i;
    (*dev_mir_sess)[i].sid = PIPE_MGR_MAX_SESSIONS;
  }
  pipe_mgr_set_mir_ses_hdl_array_len(dev_id, mirror_sess_max + 1);

  /* Populate the hash table using the information from the RMT database. */
  t = smti;
  bool virtual_dev = pipe_mgr_is_device_virtual(dev_id);

  for (p = 0; p < di->num_pipeline_profiles; p++) {
    tbl_info = &(di->profile_info[p]->tbl_info_list);
    profile_id = di->profile_info[p]->profile_id;
    pipe_bmp = &di->profile_info[p]->pipe_bmp;
    /* Add all match tables. */
    for (i = 0; i < tbl_info->num_mat_tbls; ++i, ++t) {
      tbl_info->mat_tbl_list[i].profile_id = profile_id;
      pipe_mat_tbl_hdl_t h = tbl_info->mat_tbl_list[i].handle;
      t->id = h;
      for (pipe = 0; pipe < PIPE_MGR_MAX_PIPES; pipe++)
        t->sid[pipe] = PIPE_MGR_MAX_SESSIONS;
      t->prof_id = profile_id;
      t->initialized = !virtual_dev;
      /* LPM and Ternary tables are always owned by PIPE_MGR_TBL_OWNER_TRN
       * but Direct and Hash match tables can be owned by either
       * PIPE_MGR_TBL_OWNER_EXM or PIPE_MGR_TBL_OWNER_TRN.
       * It all comes down to whether the table uses TCAM memory or not, if
       * it uses all TCAM memory let the owner be _TRN, but if it uses TCAM
       * and SRAM memory let the owner be _EXM. */
      if (pipe_mgr_mat_tbl_is_no_key(dev_id, h)) {
        t->owner = PIPE_MGR_TBL_OWNER_NO_KEY;
      } else if (pipe_mgr_mat_tbl_is_phase0(dev_id, h)) {
        t->owner = PIPE_MGR_TBL_OWNER_PHASE0;
        if (t->initialized) /* Table needs to be initialized, do it now. */
          pipe_mgr_phase0_tbl_init(dev_id, h, profile_id, pipe_bmp);
      } else if (pipe_mgr_mat_tbl_uses_alpm(dev_id, h)) {
        t->owner = PIPE_MGR_TBL_OWNER_ALPM;
        if (t->initialized) /* Table needs to be initialized, do it now. */
          pipe_mgr_alpm_tbl_create(dev_id, h, profile_id, pipe_bmp);
      } else {
        t->owner = pipe_mgr_mat_tbl_uses_only_tcam(dev_id, h)
                       ? PIPE_MGR_TBL_OWNER_TRN
                       : PIPE_MGR_TBL_OWNER_EXM;
        /* Add the table to the TCAM Table Manager if it uses TCAMs. */
        if (pipe_mgr_mat_tbl_uses_tcam(dev_id, h)) {
          if (t->initialized) /* Table needs to be initialized, do it now. */
            pipe_mgr_tcam_tbl_create(dev_id, h, profile_id, pipe_bmp);
        }
        /* Add the table to the EXM Table Manager if it does not use only
         * TCAMs. */
        if (!pipe_mgr_mat_tbl_uses_only_tcam(dev_id, h)) {
          if (t->initialized) /* Table needs to be initialized, do it now. */
            pipe_mgr_exm_tbl_init(dev_id, h, profile_id, pipe_bmp);
        }
      }
      bf_map_sts_t msts = bf_map_add(pipe_mgr_sm_tbl_db(dev_id), h, t);
      PIPE_MGR_ASSERT(BF_MAP_OK == msts);
      if (pipe_mgr_mat_tbl_has_idle(dev_id, h)) {
        if (t->initialized) /* Table needs to be initialized, do it now. */
          pipe_mgr_idle_tbl_create(dev_id, h, profile_id, pipe_bmp);
      }
      if (!v && t->owner != PIPE_MGR_TBL_OWNER_ALPM) {
        /* Map-ram mem-id to table type map */
        pipe_mgr_set_all_map_ram_type(dev_id, h, pipe_bmp);
        /* Mem-id to table handle map */
        pipe_mgr_set_mem_id_to_tbl_hdl_mapping(
            dev_id,
            tbl_info->mat_tbl_list[i].rmt_info,
            tbl_info->mat_tbl_list[i].num_rmt_info,
            t->id,
            pipe_bmp);
      }
    }
    /* Add all action data tables. */
    for (i = 0; i < tbl_info->num_adt_tbls; ++i, ++t) {
      t->id = tbl_info->adt_tbl_list[i].handle;
      for (pipe = 0; pipe < PIPE_MGR_MAX_PIPES; pipe++)
        t->sid[pipe] = PIPE_MGR_MAX_SESSIONS;
      t->prof_id = profile_id;
      t->owner = PIPE_MGR_TBL_OWNER_ADT;
      t->initialized = !virtual_dev;
      t->resource_count = 1;
      t->resources = PIPE_MGR_MALLOC(sizeof t->resources[0]);
      if (!t->resources) return PIPE_NO_SYS_RESOURCES;
      t->resources[0] = t;
      if (t->initialized) {
        status = pipe_mgr_adt_tbl_init(dev_id, t->id, profile_id, pipe_bmp);
        if (status != PIPE_SUCCESS) {
          LOG_ERROR("Error initializing %s tbl %#x dev id %d error %s",
                    "action",
                    t->id,
                    dev_id,
                    pipe_str_err(status));
          PIPE_MGR_DBGCHK(status == PIPE_SUCCESS);
          return status;
        }
      }
      bf_map_sts_t msts = bf_map_add(pipe_mgr_sm_tbl_db(dev_id), t->id, t);
      PIPE_MGR_ASSERT(BF_MAP_OK == msts);
      if (!v) {
        /* Mem-id to table handle map */
        pipe_mgr_set_mem_id_to_tbl_hdl_mapping(
            dev_id,
            tbl_info->adt_tbl_list[i].rmt_info,
            tbl_info->adt_tbl_list[i].num_rmt_info,
            t->id,
            pipe_bmp);
      }
    }
    /* Add all stats tables. */
    for (i = 0; i < tbl_info->num_stat_tbls; ++i, ++t) {
      t->id = tbl_info->stat_tbl_list[i].handle;
      for (pipe = 0; pipe < PIPE_MGR_MAX_PIPES; pipe++)
        t->sid[pipe] = PIPE_MGR_MAX_SESSIONS;
      t->prof_id = profile_id;
      t->owner = PIPE_MGR_TBL_OWNER_STAT;
      t->initialized = !virtual_dev;
      t->resource_count = 1;
      t->resources = PIPE_MGR_MALLOC(sizeof t->resources[0]);
      if (!t->resources) return PIPE_NO_SYS_RESOURCES;
      t->resources[0] = t;
      if (t->initialized) {
        status =
            pipe_mgr_stat_tbl_init(sh, dev_id, t->id, profile_id, pipe_bmp);
        if (status != PIPE_SUCCESS) {
          LOG_ERROR("Error initializing %s tbl %#x dev id %d error %s",
                    "stats",
                    t->id,
                    dev_id,
                    pipe_str_err(status));
          PIPE_MGR_DBGCHK(status == PIPE_SUCCESS);
          return status;
        }
      }
      bf_map_sts_t msts = bf_map_add(pipe_mgr_sm_tbl_db(dev_id), t->id, t);
      PIPE_MGR_ASSERT(BF_MAP_OK == msts);
      if (!v) {
        /* Mem-id to table handle map */
        pipe_mgr_set_mem_id_to_tbl_hdl_mapping(
            dev_id,
            tbl_info->stat_tbl_list[i].rmt_info,
            tbl_info->stat_tbl_list[i].num_rmt_info,
            t->id,
            pipe_bmp);
      }
    }
    /* Add all meter tables. */
    for (i = 0; i < tbl_info->num_meter_tbls; ++i, ++t) {
      t->id = tbl_info->meter_tbl_list[i].handle;
      for (pipe = 0; pipe < PIPE_MGR_MAX_PIPES; pipe++)
        t->sid[pipe] = PIPE_MGR_MAX_SESSIONS;
      t->prof_id = profile_id;
      t->owner = PIPE_MGR_TBL_OWNER_METER;
      t->initialized = !virtual_dev;
      t->resource_count = 1;
      t->resources = PIPE_MGR_MALLOC(sizeof t->resources[0]);
      if (!t->resources) return PIPE_NO_SYS_RESOURCES;
      t->resources[0] = t;
      if (t->initialized) {
        status =
            pipe_mgr_meter_tbl_init(sh, dev_id, t->id, profile_id, pipe_bmp);
        if (status != PIPE_SUCCESS) {
          LOG_ERROR("Error initializing %s tbl %#x dev id %d error %s",
                    "meter",
                    t->id,
                    dev_id,
                    pipe_str_err(status));
          PIPE_MGR_DBGCHK(status == PIPE_SUCCESS);
          return status;
        }
      }
      bf_map_sts_t msts = bf_map_add(pipe_mgr_sm_tbl_db(dev_id), t->id, t);
      PIPE_MGR_ASSERT(BF_MAP_OK == msts);
      if (!v) {
        /* Mem-id to table handle map */
        pipe_mgr_set_mem_id_to_tbl_hdl_mapping(
            dev_id,
            tbl_info->meter_tbl_list[i].rmt_info,
            tbl_info->meter_tbl_list[i].num_rmt_info,
            t->id,
            pipe_bmp);
      }
    }
    /* Add all stateful tables. */
    for (i = 0; i < tbl_info->num_sful_tbls; ++i, ++t) {
      t->id = tbl_info->stful_tbl_list[i].handle;
      for (pipe = 0; pipe < PIPE_MGR_MAX_PIPES; pipe++)
        t->sid[pipe] = PIPE_MGR_MAX_SESSIONS;
      t->prof_id = profile_id;
      t->owner = PIPE_MGR_TBL_OWNER_STFUL;
      t->initialized = !virtual_dev;
      t->resource_count = 1;
      t->resources = PIPE_MGR_MALLOC(sizeof t->resources[0]);
      if (!t->resources) return PIPE_NO_SYS_RESOURCES;
      t->resources[0] = t;
      if (t->initialized) {
        status =
            pipe_mgr_stful_tbl_add(sh, dev_id, t->id, profile_id, pipe_bmp);
        if (status != PIPE_SUCCESS) {
          LOG_ERROR("Error initializing %s tbl %#x dev id %d error %s",
                    "stful",
                    t->id,
                    dev_id,
                    pipe_str_err(status));
          PIPE_MGR_DBGCHK(status == PIPE_SUCCESS);
          return status;
        }
      }
      bf_map_sts_t msts = bf_map_add(pipe_mgr_sm_tbl_db(dev_id), t->id, t);
      PIPE_MGR_ASSERT(BF_MAP_OK == msts);
      if (!v) {
        /* Mem-id to table handle map */
        pipe_mgr_set_mem_id_to_tbl_hdl_mapping(
            dev_id,
            tbl_info->stful_tbl_list[i].rmt_info,
            tbl_info->stful_tbl_list[i].num_rmt_info,
            t->id,
            pipe_bmp);
      }
    }
    /* Add all selection tables. */
    for (i = 0; i < tbl_info->num_select_tbls; ++i, ++t) {
      pipe_sel_tbl_hdl_t h = tbl_info->select_tbl_list[i].handle;
      t->id = h;
      for (pipe = 0; pipe < PIPE_MGR_MAX_PIPES; pipe++)
        t->sid[pipe] = PIPE_MGR_MAX_SESSIONS;
      t->prof_id = profile_id;
      t->owner = PIPE_MGR_TBL_OWNER_SELECT;
      t->initialized = !virtual_dev;

      /* The resources will be set down at the end of this function. */
      t->resources = NULL;

      if (t->initialized) {
        status = pipe_mgr_sel_tbl_create(sh, dev_id, h, profile_id, pipe_bmp);
        if (status != PIPE_SUCCESS) {
          LOG_ERROR("Error initializing %s tbl %#x dev id %d error %s",
                    "selector",
                    t->id,
                    dev_id,
                    pipe_str_err(status));
          PIPE_MGR_DBGCHK(status == PIPE_SUCCESS);
          return status;
        }
      }
      bf_map_sts_t msts = bf_map_add(pipe_mgr_sm_tbl_db(dev_id), h, t);
      PIPE_MGR_ASSERT(BF_MAP_OK == msts);
      if (!v) {
        /* Mem-id to table handle map */
        pipe_mgr_set_mem_id_to_tbl_hdl_mapping(
            dev_id,
            tbl_info->select_tbl_list[i].rmt_info,
            tbl_info->select_tbl_list[i].num_rmt_info,
            t->id,
            pipe_bmp);
      }
    }
  }

  for (p = 0; p < di->num_pipeline_profiles; p++) {
    tbl_info = &(di->profile_info[p]->tbl_info_list);
    for (i = 0; i < tbl_info->num_mat_tbls; ++i) {
      pipe_mat_tbl_info_t *ti = &tbl_info->mat_tbl_list[i];
      t = getTblInfoByHdl(dev_id, ti->handle);
      if (t == NULL) {
        LOG_ERROR("%s:%d Error in getting table info for tbl %#x dev id %d",
                  __func__,
                  __LINE__,
                  ti->handle,
                  dev_id);
        PIPE_MGR_DBGCHK(0);
        return PIPE_OBJ_NOT_FOUND;
      }

      t->resource_count = 1;
      if (ti->adt_tbl_ref) ++t->resource_count;
      if (ti->sel_tbl_ref) ++t->resource_count;
      if (ti->stat_tbl_ref) ++t->resource_count;
      if (ti->meter_tbl_ref) ++t->resource_count;
      if (ti->stful_tbl_ref) ++t->resource_count;
      t->resources =
          PIPE_MGR_MALLOC(t->resource_count * sizeof t->resources[0]);
      if (!t->resources) return PIPE_NO_SYS_RESOURCES;
      int index = 0;
      t->resources[index++] = t;
      if (ti->adt_tbl_ref) {
        t->resources[index++] =
            getTblInfoByHdl(dev_id, ti->adt_tbl_ref->tbl_hdl);
      }
      if (ti->sel_tbl_ref) {
        t->resources[index++] =
            getTblInfoByHdl(dev_id, ti->sel_tbl_ref->tbl_hdl);
      }
      if (ti->stat_tbl_ref) {
        t->resources[index++] =
            getTblInfoByHdl(dev_id, ti->stat_tbl_ref->tbl_hdl);
      }
      if (ti->meter_tbl_ref) {
        t->resources[index++] =
            getTblInfoByHdl(dev_id, ti->meter_tbl_ref->tbl_hdl);
      }
      if (ti->stful_tbl_ref) {
        t->resources[index++] =
            getTblInfoByHdl(dev_id, ti->stful_tbl_ref->tbl_hdl);
      }
    }
    for (i = 0; i < tbl_info->num_select_tbls; ++i) {
      int index = 0;
      pipe_select_tbl_info_t *ti = &tbl_info->select_tbl_list[i];
      t = getTblInfoByHdl(dev_id, ti->handle);
      if (!t) {
        LOG_ERROR("%s:%d Error in getting table info for tbl %#x dev id %d",
                  __func__,
                  __LINE__,
                  ti->handle,
                  dev_id);
        PIPE_MGR_DBGCHK(0);
        return PIPE_OBJ_NOT_FOUND;
      }
      t->resource_count = 1;
      if (ti->adt_tbl_hdl) ++t->resource_count;
      if (ti->stful_tbl_hdl) ++t->resource_count;
      t->resources =
          PIPE_MGR_MALLOC(t->resource_count * sizeof t->resources[0]);
      if (!t->resources) return PIPE_NO_SYS_RESOURCES;
      t->resources[index++] = t;
      PIPE_MGR_DBGCHK(t->resource_count > 1); /* Must have at least ADT. */
      if (ti->adt_tbl_hdl) {
        t->resources[index++] = getTblInfoByHdl(dev_id, ti->adt_tbl_hdl);
      }
      if (ti->stful_tbl_hdl) {
        t->resources[index++] = getTblInfoByHdl(dev_id, ti->stful_tbl_hdl);
      }
    }
  }

  /* Set the driver options */
  for (p = 0; p < di->num_pipeline_profiles; p++) {
    pipe_driver_options_t *options = &(di->profile_info[p]->driver_options);
    pipe_bmp = &di->profile_info[p]->pipe_bmp;
    if (!v) {
      /* Set the GFM hash parity enable/disable */
      pipe_mgr_set_gfm_hash_parity_enable(
          dev_id, pipe_bmp, options->hash_parity_enabled);
    }
  }

  /* Init dynamic hash calculation defaults */
  pipe_mgr_hash_calc_init_defaults(dev_id);

  /* Initialize a mutex to protect the hash table from multiple accesses. */
  PIPE_MGR_LOCK_R_INIT(*pipe_mgr_smtbl_mtx(dev_id));

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sm_cleanup(bf_dev_id_t dev_id) {
  rmt_dev_tbl_info_t *tbl_info = NULL;
  uint32_t p = 0;
  pipe_mgr_sm_mir_info_t **smmi = NULL;
  pipe_mgr_sm_tbl_info_t **smti = NULL;
  rmt_dev_info_t *di = pipe_mgr_get_dev_info(dev_id);
  if (!di) {
    return PIPE_INVALID_ARG;
  }

  if (!(smmi = pipe_mgr_mir_ses_hdl_array_get(dev_id))) {
    LOG_ERROR("%s:%d mir array is NULL, dev id %d", __func__, __LINE__, dev_id);
  } else {
    PIPE_MGR_FREE(*smmi);
    *smmi = NULL;
  }

  /* Remove the tables from the clients. */
  unsigned i;
  for (p = 0; p < di->num_pipeline_profiles; p++) {
    tbl_info = &(di->profile_info[p]->tbl_info_list);
    for (i = 0; i < tbl_info->num_mat_tbls; ++i) {
      pipe_mat_tbl_hdl_t h = tbl_info->mat_tbl_list[i].handle;
      pipe_mgr_sm_tbl_info_t *t = getTblInfoByHdl(dev_id, h);
      if (t == NULL) {
        LOG_ERROR("%s:%d Error in getting table info for tbl %#x dev id %d",
                  __func__,
                  __LINE__,
                  h,
                  dev_id);
        PIPE_MGR_DBGCHK(0);
        return PIPE_OBJ_NOT_FOUND;
      }

      PIPE_MGR_FREE(t->resources);
      t->resources = NULL;

      if (pipe_mgr_mat_tbl_is_phase0(dev_id, h)) {
        pipe_mgr_phase0_tbl_delete(dev_id, h);
      } else if (pipe_mgr_mat_tbl_uses_alpm(dev_id, h)) {
        /* Delete the table from the ALPM Table Manager if it uses ALPM. */
        pipe_mgr_alpm_tbl_delete(dev_id, h);
      } else {
        /* Delete the table from the TCAM Table Manager if it uses TCAMs. */
        if (pipe_mgr_mat_tbl_uses_tcam(dev_id, h)) {
          pipe_mgr_tcam_tbl_delete(dev_id, h);
        }
        /* Delete the table from the EXM Table Manager if it does not use only
         * TCAMs. */
        if (!pipe_mgr_mat_tbl_uses_only_tcam(dev_id, h)) {
          pipe_mgr_exm_tbl_delete(dev_id, h);
        }
      }
      if (pipe_mgr_mat_tbl_has_idle(dev_id, h)) {
        pipe_mgr_idle_tbl_delete(dev_id, h);
      }
    }
    for (i = 0; i < tbl_info->num_adt_tbls; ++i) {
      pipe_adt_tbl_hdl_t h = tbl_info->adt_tbl_list[i].handle;
      pipe_mgr_adt_tbl_delete(dev_id, h);
      pipe_mgr_sm_tbl_info_t *t = getTblInfoByHdl(dev_id, h);
      if (t) {
        PIPE_MGR_FREE(t->resources);
      }
    }
    for (i = 0; i < tbl_info->num_stat_tbls; ++i) {
      pipe_stat_tbl_hdl_t h = tbl_info->stat_tbl_list[i].handle;
      pipe_mgr_stat_mgr_tbl_cleanup(dev_id, h);
      pipe_mgr_sm_tbl_info_t *t = getTblInfoByHdl(dev_id, h);
      if (t) {
        PIPE_MGR_FREE(t->resources);
      }
    }
    for (i = 0; i < tbl_info->num_meter_tbls; ++i) {
      pipe_meter_tbl_hdl_t h = tbl_info->meter_tbl_list[i].handle;
      pipe_mgr_meter_mgr_tbl_cleanup(dev_id, h);
      pipe_mgr_sm_tbl_info_t *t = getTblInfoByHdl(dev_id, h);
      if (t) {
        PIPE_MGR_FREE(t->resources);
      }
    }
    for (i = 0; i < tbl_info->num_sful_tbls; ++i) {
      pipe_stful_tbl_hdl_t h = tbl_info->stful_tbl_list[i].handle;
      pipe_mgr_stful_tbl_del(dev_id, tbl_info->stful_tbl_list[i].handle);
      pipe_mgr_sm_tbl_info_t *t = getTblInfoByHdl(dev_id, h);
      if (t) {
        PIPE_MGR_FREE(t->resources);
      }
    }
    for (i = 0; i < tbl_info->num_select_tbls; ++i) {
      pipe_sel_tbl_hdl_t h = tbl_info->select_tbl_list[i].handle;
      pipe_mgr_sel_tbl_delete(dev_id, h);
      pipe_mgr_sm_tbl_info_t *t = getTblInfoByHdl(dev_id, h);
      if (t) {
        PIPE_MGR_FREE(t->resources);
      }
    }
  }

  bf_map_destroy(pipe_mgr_sm_tbl_db(dev_id));
  *pipe_mgr_sm_tbl_db(dev_id) = NULL;

  if (!(smti = pipe_mgr_sm_tbl_info(dev_id))) {
    LOG_ERROR("%s:%d smti is NULL, dev id %d", __func__, __LINE__, dev_id);
  } else {
    PIPE_MGR_FREE(*smti);
    *smti = NULL;
  }

  PIPE_MGR_LOCK_R_DESTROY(pipe_mgr_smtbl_mtx(dev_id));

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sm_set_mat_cb(bf_dev_id_t dev_id,
                                     pipe_mat_tbl_hdl_t th,
                                     pipe_mat_update_cb cb,
                                     void *cb_cookie) {
  pipe_mgr_sm_tbl_info_t *t = getTblInfoByHdl(dev_id, th);
  if (!t) return PIPE_INVALID_ARG;

  /* If this is a virtual device and the first time the callback is registered
   * the MAT table and all of its resource tables must be created. */
  if (pipe_mgr_is_device_virtual(dev_id) && !t->u.mat_cb) {
    PIPE_MGR_LOCK_R(pipe_mgr_smtbl_mtx(dev_id));
    pipe_status_t rc =
        create_virtual_dev_mat(pipe_mgr_get_int_sess_hdl(), dev_id, t);
    PIPE_MGR_UNLOCK_R(pipe_mgr_smtbl_mtx(dev_id));
    if (rc != PIPE_SUCCESS) return rc;
  }
  t->u.mat_cb = cb;
  t->cb_cookie = cb_cookie;
  return PIPE_SUCCESS;
}
static void pipe_mgr_sm_get_mat_cb(bf_dev_id_t dev_id,
                                   pipe_mat_tbl_hdl_t th,
                                   pipe_mat_update_cb *cb,
                                   void **cb_cookie) {
  pipe_mgr_sm_tbl_info_t *t = getTblInfoByHdl(dev_id, th);
  if (!t) {
    *cb = NULL;
    *cb_cookie = NULL;
    return;
  }
  *cb = t->u.mat_cb;
  *cb_cookie = t->cb_cookie;
  return;
}
pipe_status_t pipe_mgr_sm_set_adt_cb(bf_dev_id_t dev_id,
                                     pipe_adt_tbl_hdl_t th,
                                     pipe_adt_update_cb cb,
                                     void *cb_cookie) {
  pipe_mgr_sm_tbl_info_t *t = getTblInfoByHdl(dev_id, th);
  if (!t) return PIPE_INVALID_ARG;
  /* If this is a virtual device and the first time the callback is registered
   * the table may need to be created. */
  if (pipe_mgr_is_device_virtual(dev_id) && !t->u.adt_cb) {
    PIPE_MGR_LOCK_R(pipe_mgr_smtbl_mtx(dev_id));
    pipe_status_t rc =
        create_virtual_dev_res_tbl(pipe_mgr_get_int_sess_hdl(), dev_id, t);
    PIPE_MGR_UNLOCK_R(pipe_mgr_smtbl_mtx(dev_id));
    if (rc != PIPE_SUCCESS) return rc;
  }
  t->u.adt_cb = cb;
  t->cb_cookie = cb_cookie;
  return PIPE_SUCCESS;
}
static void pipe_mgr_sm_get_adt_cb(bf_dev_id_t dev_id,
                                   pipe_adt_tbl_hdl_t th,
                                   pipe_adt_update_cb *cb,
                                   void **cb_cookie) {
  pipe_mgr_sm_tbl_info_t *t = getTblInfoByHdl(dev_id, th);
  if (!t) {
    *cb = NULL;
    *cb_cookie = NULL;
    return;
  }
  *cb = t->u.adt_cb;
  *cb_cookie = t->cb_cookie;
  return;
}
pipe_status_t pipe_mgr_sm_set_sel_cb(bf_dev_id_t dev_id,
                                     pipe_sel_tbl_hdl_t th,
                                     pipe_sel_update_cb cb,
                                     void *cb_cookie) {
  pipe_mgr_sm_tbl_info_t *t = getTblInfoByHdl(dev_id, th);
  if (!t) return PIPE_INVALID_ARG;
  /* If this is a virtual device and the first time the callback is registered
   * the table may need to be created. */
  if (pipe_mgr_is_device_virtual(dev_id) && !t->u.sel_cb) {
    PIPE_MGR_LOCK_R(pipe_mgr_smtbl_mtx(dev_id));
    pipe_status_t rc =
        create_virtual_dev_res_tbl(pipe_mgr_get_int_sess_hdl(), dev_id, t);
    PIPE_MGR_UNLOCK_R(pipe_mgr_smtbl_mtx(dev_id));
    if (rc != PIPE_SUCCESS) return rc;
  }
  t->u.sel_cb = cb;
  t->cb_cookie = cb_cookie;
  return PIPE_SUCCESS;
}
static void pipe_mgr_sm_get_sel_cb(bf_dev_id_t dev_id,
                                   pipe_sel_tbl_hdl_t th,
                                   pipe_sel_update_cb *cb,
                                   void **cb_cookie) {
  pipe_mgr_sm_tbl_info_t *t = getTblInfoByHdl(dev_id, th);
  if (!t) {
    *cb = NULL;
    *cb_cookie = NULL;
    return;
  }
  *cb = t->u.sel_cb;
  *cb_cookie = t->cb_cookie;
  return;
}

pipe_status_t pipe_mgr_sm_save_ml_prep(pipe_sess_hdl_t sess_hdl,
                                       bf_dev_id_t dev_id,
                                       pipe_tbl_hdl_t tbl_hdl) {
  /* If this is the first operation in the transaction (saved moved list header
   * is NULL) or the most recent operation (end of the list) was to a different
   * table than the current operation then a new header ndoe must be appended
   * to the list. */
  pipe_mgr_sess_ctx_t *s = pipe_mgr_get_sess_ctx(sess_hdl, __func__, __LINE__);
  if (s == NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_OBJ_NOT_FOUND;
  }

  struct pipe_mgr_sm_move_list_hdr *x = NULL;
  PIPE_MGR_DLL_LAST(s->txn_mls[dev_id], x, next, prev);
  if (!x || tbl_hdl != x->tbl_hdl) {
    x = PIPE_MGR_MALLOC(sizeof(struct pipe_mgr_sm_move_list_hdr));
    if (!x) return PIPE_NO_SYS_RESOURCES;
    PIPE_MGR_DLL_AP(s->txn_mls[dev_id], x, next, prev);
    x->dev_id = dev_id;
    x->tbl_hdl = tbl_hdl;
    x->ml = x->ml_tail = NULL;
  }
  return PIPE_SUCCESS;
}
void pipe_mgr_sm_save_ml(pipe_sess_hdl_t sess_hdl,
                         bf_dev_id_t dev_id,
                         pipe_mat_tbl_hdl_t mat_tbl_hdl,
                         struct pipe_mgr_move_list_t *ml) {
  pipe_mgr_sess_ctx_t *s = pipe_mgr_get_sess_ctx(sess_hdl, __func__, __LINE__);
  if (s == NULL) {
    PIPE_MGR_DBGCHK(0);
    return;
  }

  struct pipe_mgr_sm_move_list_hdr *x = NULL;
  PIPE_MGR_DLL_LAST(s->txn_mls[dev_id], x, next, prev);
  /* Since we supposedly called save_ml_prep the following MUST be true. */
  if (x == NULL) {
    PIPE_MGR_DBGCHK(0);
    return;
  }
  PIPE_MGR_DBGCHK(x->tbl_hdl == mat_tbl_hdl);

  /* At this point, x now refers to the last ml-header in the list.  Attach the
   * move list to this header node.  If there is already a move list present
   * then tack this one onto the end of it. */
  if (x->ml_tail) {
    x->ml_tail->next = ml;
  } else {
    x->ml = x->ml_tail = ml;
  }

  /* Since a new move list has been saved on this header node, walk it to find
   * the new tail pointer. */
  for (; x->ml_tail->next; x->ml_tail = x->ml_tail->next) {
  }
}

void pipe_mgr_sm_issue_mat_cb(bf_dev_id_t dev_id,
                              pipe_mat_tbl_hdl_t mat_tbl_hdl,
                              pipe_mgr_move_list_t *ml) {
  pipe_mat_update_cb cb = NULL;
  void *cb_cookie = NULL;
  pipe_mgr_sm_get_mat_cb(dev_id, mat_tbl_hdl, &cb, &cb_cookie);
  if (!cb) return;
  while (ml) {
    union pipe_mat_update_params params = {{0}};
    switch (ml->op) {
      case PIPE_MAT_UPDATE_ADD:
        params.add.ent_hdl = ml->entry_hdl;
        params.add.priority = ml->data->match_spec.priority;
        params.add.logical_index = ml->u.single.logical_idx;
        params.add.action_profile_mbr = ml->adt_ent_hdl;
        params.add.indirect_action_index = ml->logical_action_idx;
        params.add.action_profile_mbr_exists = ml->adt_ent_hdl_valid;
        params.add.sel_grp_hdl = ml->data->action_spec.sel_grp_hdl;
        params.add.indirect_selection_index = ml->logical_sel_idx;
        params.add.num_selector_words = ml->selector_len;
        params.add.sel_grp_exists =
            IS_ACTION_SPEC_SEL_GRP(&ml->data->action_spec);
        params.add.data = ml->data;
        LOG_TRACE(
            "MAT: Dev %d Tbl %#x Pipe %X Add Entry %#x Idx %d Act-%s %#x@%d "
            "Sel-%s %#x@%d Len %d",
            dev_id,
            mat_tbl_hdl,
            get_move_list_pipe(ml),
            ml->entry_hdl,
            ml->u.single.logical_idx,
            ml->adt_ent_hdl_valid ? "Valid" : "Invalid",
            ml->adt_ent_hdl,
            ml->logical_action_idx,
            IS_ACTION_SPEC_SEL_GRP(&ml->data->action_spec) ? "Valid"
                                                           : "Invalid",
            ml->data->action_spec.sel_grp_hdl,
            ml->logical_sel_idx,
            ml->selector_len);
        break;
      case PIPE_MAT_UPDATE_ADD_MULTI:
        params.add_multi.ent_hdl = ml->entry_hdl;
        params.add_multi.action_profile_mbr = ml->adt_ent_hdl;
        params.add_multi.indirect_action_index = ml->logical_action_idx;
        params.add_multi.action_profile_mbr_exists = ml->adt_ent_hdl_valid;
        params.add_multi.sel_grp_hdl = ml->data->action_spec.sel_grp_hdl;
        params.add_multi.indirect_selection_index = ml->logical_sel_idx;
        params.add_multi.num_selector_words = ml->selector_len;
        params.add_multi.sel_grp_exists =
            IS_ACTION_SPEC_SEL_GRP(&ml->data->action_spec);
        params.add_multi.logical_index_array_length = ml->u.multi.array_sz;
        params.add_multi.location_array = ml->u.multi.locations;
        params.add_multi.data = ml->data;
        LOG_TRACE(
            "MAT: Dev %d Tbl %#x Pipe %X AddMulti Entry %#x Idx %d Count %d "
            "Act-%s %#x@%d Sel-%s %#x@%d Len %d",
            dev_id,
            mat_tbl_hdl,
            get_move_list_pipe(ml),
            ml->entry_hdl,
            ml->u.single.logical_idx,
            ml->u.multi.array_sz,
            ml->adt_ent_hdl_valid ? "Valid" : "Invalid",
            ml->adt_ent_hdl,
            ml->logical_action_idx,
            IS_ACTION_SPEC_SEL_GRP(&ml->data->action_spec) ? "Valid"
                                                           : "Invalid",
            ml->data->action_spec.sel_grp_hdl,
            ml->logical_sel_idx,
            ml->selector_len);
        break;
      case PIPE_MAT_UPDATE_SET_DFLT:
        params.set_dflt.ent_hdl = ml->entry_hdl;
        params.set_dflt.action_profile_mbr = ml->adt_ent_hdl;
        params.set_dflt.indirect_action_index = ml->logical_action_idx;
        params.set_dflt.action_profile_mbr_exists = ml->adt_ent_hdl_valid;
        params.set_dflt.sel_grp_hdl = ml->data->action_spec.sel_grp_hdl;
        params.set_dflt.indirect_selection_index = ml->logical_sel_idx;
        params.set_dflt.num_selector_words = ml->selector_len;
        params.set_dflt.sel_grp_exists =
            IS_ACTION_SPEC_SEL_GRP(&ml->data->action_spec);
        params.set_dflt.data = ml->data;
        LOG_TRACE(
            "MAT: Dev %d Tbl %#x Pipe %X SetDflt Entry %#x Act-%s %#x@%d "
            "Sel-%s %#x@%d Len %d",
            dev_id,
            mat_tbl_hdl,
            get_move_list_pipe(ml),
            ml->entry_hdl,
            ml->adt_ent_hdl_valid ? "Valid" : "Invalid",
            ml->adt_ent_hdl,
            ml->logical_action_idx,
            IS_ACTION_SPEC_SEL_GRP(&ml->data->action_spec) ? "Valid"
                                                           : "Invalid",
            ml->data->action_spec.sel_grp_hdl,
            ml->logical_sel_idx,
            ml->selector_len);
        break;
      case PIPE_MAT_UPDATE_CLR_DFLT:
        params.clr_dflt.ent_hdl = ml->entry_hdl;
        LOG_TRACE("MAT: Dev %d Tbl %#x Pipe %X ClrDflt Entry %#x",
                  dev_id,
                  mat_tbl_hdl,
                  get_move_list_pipe(ml),
                  ml->entry_hdl);
        break;
      case PIPE_MAT_UPDATE_DEL:
        params.del.ent_hdl = ml->entry_hdl;
        LOG_TRACE("MAT: Dev %d Tbl %#x Pipe %X Del Entry %#x",
                  dev_id,
                  mat_tbl_hdl,
                  get_move_list_pipe(ml),
                  ml->entry_hdl);
        break;
      case PIPE_MAT_UPDATE_MOD:
        params.mod.ent_hdl = ml->entry_hdl;
        params.mod.action_profile_mbr = ml->adt_ent_hdl;
        params.mod.indirect_action_index = ml->logical_action_idx;
        params.mod.action_profile_mbr_exists = ml->adt_ent_hdl_valid;
        params.mod.sel_grp_hdl = ml->data->action_spec.sel_grp_hdl;
        params.mod.indirect_selection_index = ml->logical_sel_idx;
        params.mod.num_selector_words = ml->selector_len;
        params.mod.sel_grp_exists =
            IS_ACTION_SPEC_SEL_GRP(&ml->data->action_spec);
        params.mod.data = ml->data;
        LOG_TRACE(
            "MAT: Dev %d Tbl %#x Pipe %X Mod Entry %#x Act-%s %#x@%d Sel-%s "
            "%#x@%d Len %d",
            dev_id,
            mat_tbl_hdl,
            get_move_list_pipe(ml),
            ml->entry_hdl,
            ml->adt_ent_hdl_valid ? "Valid" : "Invalid",
            ml->adt_ent_hdl,
            ml->logical_action_idx,
            IS_ACTION_SPEC_SEL_GRP(&ml->data->action_spec) ? "Valid"
                                                           : "Invalid",
            ml->data->action_spec.sel_grp_hdl,
            ml->logical_sel_idx,
            ml->selector_len);
        break;
      case PIPE_MAT_UPDATE_MOV:
        params.mov.ent_hdl = ml->entry_hdl;
        params.mov.logical_index = ml->u.single.logical_idx;
        params.mov.action_profile_mbr = ml->adt_ent_hdl;
        params.mov.indirect_action_index = ml->logical_action_idx;
        params.mov.action_profile_mbr_exists = ml->adt_ent_hdl_valid;
        params.mov.sel_grp_hdl = ml->data->action_spec.sel_grp_hdl;
        params.mov.indirect_selection_index = ml->logical_sel_idx;
        params.mov.num_selector_words = ml->selector_len;
        params.mov.sel_grp_exists =
            IS_ACTION_SPEC_SEL_GRP(&ml->data->action_spec);
        params.mov.data = ml->data;
        LOG_TRACE(
            "MAT: Dev %d Tbl %#x Pipe %X Mov Entry %#x Idx %d Act-%s %#x@%d "
            "Sel-%s %#x@%d Len %d",
            dev_id,
            mat_tbl_hdl,
            get_move_list_pipe(ml),
            ml->entry_hdl,
            ml->u.single.logical_idx,
            ml->adt_ent_hdl_valid ? "Valid" : "Invalid",
            ml->adt_ent_hdl,
            ml->logical_action_idx,
            IS_ACTION_SPEC_SEL_GRP(&ml->data->action_spec) ? "Valid"
                                                           : "Invalid",
            ml->data->action_spec.sel_grp_hdl,
            ml->logical_sel_idx,
            ml->selector_len);
        break;
      case PIPE_MAT_UPDATE_MOV_MULTI:
        params.mov_multi.ent_hdl = ml->entry_hdl;
        params.mov_multi.action_profile_mbr = ml->adt_ent_hdl;
        params.mov_multi.indirect_action_index = ml->logical_action_idx;
        params.mov_multi.action_profile_mbr_exists = ml->adt_ent_hdl_valid;
        params.mov_multi.sel_grp_hdl = ml->data->action_spec.sel_grp_hdl;
        params.mov_multi.indirect_selection_index = ml->logical_sel_idx;
        params.mov_multi.num_selector_words = ml->selector_len;
        params.mov_multi.sel_grp_exists =
            IS_ACTION_SPEC_SEL_GRP(&ml->data->action_spec);
        params.mov_multi.logical_index_array_length = ml->u.multi.array_sz;
        params.mov_multi.location_array = ml->u.multi.locations;
        params.mov_multi.data = ml->data;
        LOG_TRACE(
            "MAT: Dev %d Tbl %#x Pipe %X MovMulti Entry %#x Count %d Act-%s "
            "%#x@%d Sel-%s %#x@%d Len %d",
            dev_id,
            mat_tbl_hdl,
            get_move_list_pipe(ml),
            ml->entry_hdl,
            ml->u.multi.array_sz,
            ml->adt_ent_hdl_valid ? "Valid" : "Invalid",
            ml->adt_ent_hdl,
            ml->logical_action_idx,
            IS_ACTION_SPEC_SEL_GRP(&ml->data->action_spec) ? "Valid"
                                                           : "Invalid",
            ml->data->action_spec.sel_grp_hdl,
            ml->logical_sel_idx,
            ml->selector_len);
        break;
    }
    bf_dev_target_t dev_tgt = {dev_id, get_move_list_pipe(ml)};
    /* Assume the callback will copy this data so prepare it for export. */
    if (ml->data) mat_ent_data_prep_export(ml->data);
    cb(dev_tgt, mat_tbl_hdl, ml->op, &params, cb_cookie);
    /* Now that the callback has potentially copied the data, reset it for use
     * in this process. */
    if (ml->data) mat_ent_data_prep_import(ml->data);
    ml = ml->next;
  }
}

void pipe_mgr_sm_issue_adt_cb(bf_dev_id_t dev_id,
                              pipe_adt_tbl_hdl_t adt_tbl_hdl,
                              struct pipe_mgr_adt_move_list_t *ml) {
  pipe_adt_update_cb cb = NULL;
  void *cb_cookie = NULL;
  pipe_mgr_sm_get_adt_cb(dev_id, adt_tbl_hdl, &cb, &cb_cookie);
  if (!cb) return;
  while (ml) {
    union pipe_adt_update_params params;
    switch (ml->op) {
      case PIPE_ADT_UPDATE_ADD:
        params.add.ent_hdl = ml->entry_hdl;
        params.add.data = ml->data;
        LOG_TRACE("ADT: Dev %d Tbl %#x Pipe %X Add Hdl %#x",
                  dev_id,
                  adt_tbl_hdl,
                  ml->pipe_id,
                  ml->entry_hdl);
        break;
      case PIPE_ADT_UPDATE_DEL:
        params.del.ent_hdl = ml->entry_hdl;
        LOG_TRACE("ADT: Dev %d Tbl %#x Pipe %X Del Hdl %#x",
                  dev_id,
                  adt_tbl_hdl,
                  ml->pipe_id,
                  ml->entry_hdl);
        break;
      case PIPE_ADT_UPDATE_MOD:
        params.mod.ent_hdl = ml->entry_hdl;
        params.mod.data = ml->data;
        LOG_TRACE("ADT: Dev %d Tbl %#x Pipe %X Mod Hdl %#x",
                  dev_id,
                  adt_tbl_hdl,
                  ml->pipe_id,
                  ml->entry_hdl);
        break;
    }
    bf_dev_target_t dev_tgt = {dev_id, ml->pipe_id};
    /* Assume the callback will copy this data so prepare it for export. */
    if (ml->data) adt_ent_data_prep_export(ml->data);
    cb(dev_tgt, adt_tbl_hdl, ml->op, &params, cb_cookie);
    /* Now that the callback has potentially copied the data, reset it for use
     * in this process. */
    if (ml->data) adt_ent_data_prep_import(ml->data);
    ml = ml->next;
  }
}

void pipe_mgr_sm_issue_sel_cb(bf_dev_id_t dev_id,
                              pipe_sel_tbl_hdl_t sel_tbl_hdl,
                              struct pipe_mgr_sel_move_list_t *ml) {
  int i;
  pipe_sel_update_cb cb = NULL;
  void *cb_cookie = NULL;
  pipe_mgr_sm_get_sel_cb(dev_id, sel_tbl_hdl, &cb, &cb_cookie);
  if (!cb) return;
  while (ml) {
    union pipe_sel_update_params params;
    switch (ml->op) {
      case PIPE_SEL_UPDATE_GROUP_CREATE:
        params.grp_create.grp_hdl = ml->sel_grp_hdl;
        params.grp_create.num_indexes = ml->sel_grp_size;
        params.grp_create.max_members = ml->max_mbrs;
        params.grp_create.base_logical_index = ml->logical_sel_index;
        params.grp_create.logical_adt_indexes = ml->locations;
        params.grp_create.logical_adt_index_array_length = ml->locations_length;
        LOG_TRACE(
            "SEL: Dev %d Tbl %#x Pipe %X Grp Create Hdl %#x GrpMaxSz %d IdxCnt "
            "%d Base %#x "
            "NumADTLoc %d",
            dev_id,
            sel_tbl_hdl,
            ml->pipe,
            ml->sel_grp_hdl,
            ml->max_mbrs,
            ml->sel_grp_size,
            ml->logical_sel_index,
            ml->locations_length);
        for (i = 0; i < ml->locations_length; ++i) {
          LOG_TRACE("SEL: Loc %d is logical idx %#x through %#x, size %d",
                    i,
                    ml->locations[i].logical_index_base,
                    ml->locations[i].logical_index_base +
                        ml->locations[i].logical_index_count - 1,
                    ml->locations[i].logical_index_count);
        }
        break;
      case PIPE_SEL_UPDATE_GROUP_DESTROY:
        params.grp_destroy.grp_hdl = ml->sel_grp_hdl;
        LOG_TRACE("SEL: Dev %d Tbl %#x Pipe %X Grp Destroy Hdl %#x",
                  dev_id,
                  sel_tbl_hdl,
                  ml->pipe,
                  ml->sel_grp_hdl);
        break;
      case PIPE_SEL_UPDATE_ADD:
        params.add.grp_hdl = ml->sel_grp_hdl;
        params.add.ent_hdl = ml->adt_mbr_hdl;
        params.add.logical_index = ml->logical_sel_index;
        params.add.logical_subindex = ml->logical_sel_subindex;
        params.add.data = ml->data;
        LOG_TRACE(
            "SEL: Dev %d Tbl %#x Pipe %X MbrAdd Hdl %#x Mbr %#x Idx %d.%d",
            dev_id,
            sel_tbl_hdl,
            ml->pipe,
            ml->sel_grp_hdl,
            ml->adt_mbr_hdl,
            ml->logical_sel_index,
            ml->logical_sel_subindex);
        break;
      case PIPE_SEL_UPDATE_DEL:
        params.del.grp_hdl = ml->sel_grp_hdl;
        params.del.ent_hdl = ml->adt_mbr_hdl;
        params.del.logical_index = ml->logical_sel_index;
        params.del.logical_subindex = ml->logical_sel_subindex;
        LOG_TRACE(
            "SEL: Dev %d Tbl %#x Pipe %X MbrDel Hdl %#x Mbr %#x Idx %d.%d",
            dev_id,
            sel_tbl_hdl,
            ml->pipe,
            ml->sel_grp_hdl,
            ml->adt_mbr_hdl,
            ml->logical_sel_index,
            ml->logical_sel_subindex);
        break;
      case PIPE_SEL_UPDATE_ACTIVATE:
        params.activate.grp_hdl = ml->sel_grp_hdl;
        params.activate.ent_hdl = ml->adt_mbr_hdl;
        params.activate.logical_index = ml->logical_sel_index;
        params.activate.logical_subindex = ml->logical_sel_subindex;
        LOG_TRACE(
            "SEL: Dev %d Tbl %#x Pipe %X Activate Hdl %#x Mbr %#x Idx %d.%d",
            dev_id,
            sel_tbl_hdl,
            ml->pipe,
            ml->sel_grp_hdl,
            ml->adt_mbr_hdl,
            ml->logical_sel_index,
            ml->logical_sel_subindex);
        break;
      case PIPE_SEL_UPDATE_DEACTIVATE:
        params.deactivate.grp_hdl = ml->sel_grp_hdl;
        params.deactivate.ent_hdl = ml->adt_mbr_hdl;
        params.deactivate.logical_index = ml->logical_sel_index;
        params.deactivate.logical_subindex = ml->logical_sel_subindex;
        LOG_TRACE(
            "SEL: Dev %d Tbl %#x Pipe %X Deactivate Hdl %#x Mbr %#x Idx %d.%d",
            dev_id,
            sel_tbl_hdl,
            ml->pipe,
            ml->sel_grp_hdl,
            ml->adt_mbr_hdl,
            ml->logical_sel_index,
            ml->logical_sel_subindex);
        break;
      case PIPE_SEL_UPDATE_SET_FALLBACK:
        params.set_fallback.ent_hdl = ml->adt_mbr_hdl;
        params.set_fallback.data = ml->data;
        LOG_TRACE("SEL: Dev %d Tbl %#x Pipe %X SetFallback Mbr %#x",
                  dev_id,
                  sel_tbl_hdl,
                  ml->pipe,
                  ml->adt_mbr_hdl);
        break;
      case PIPE_SEL_UPDATE_CLR_FALLBACK:
        LOG_TRACE("SEL: Dev %d Tbl %#x Pipe %X ClrFallback",
                  dev_id,
                  sel_tbl_hdl,
                  ml->pipe);
        break;
    }
    bf_dev_target_t dev_tgt = {dev_id, ml->pipe};
    cb(dev_tgt, sel_tbl_hdl, ml->op, &params, cb_cookie);
    ml = ml->next;
  }
}

static pipe_status_t pipe_mgr_mat_default_entry_reprogram_helper(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_info_t *mat_tbl_info) {
  pipe_action_spec_t action_spec;
  pipe_act_fn_hdl_t act_fn_hdl = 0;
  pipe_mat_ent_hdl_t entry_hdl = 0;
  pipe_status_t ret = PIPE_SUCCESS;
  pipe_mat_tbl_hdl_t mat_tbl_hdl = mat_tbl_info->handle;

  ret = pipe_mgr_table_get_default_entry_handle(
      sess_hdl, dev_tgt, mat_tbl_hdl, &entry_hdl);
  if (ret != PIPE_SUCCESS) {
    /* Default entry not present, nothing to program */
    return PIPE_SUCCESS;
  }

  memset(&action_spec, 0, sizeof(action_spec));
  ret = pipe_mgr_table_get_default_entry(sess_hdl,
                                         dev_tgt,
                                         mat_tbl_hdl,
                                         &action_spec,
                                         &act_fn_hdl,
                                         false,
                                         PIPE_RES_GET_FLAG_ENTRY,
                                         NULL);
  if (ret != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s: Error in getting default entry for table 0x%x for dev id %d, err "
        "%s",
        __func__,
        mat_tbl_hdl,
        dev_tgt.device_id,
        pipe_str_err(ret));
    return ret;
  }

  ret = pipe_mgr_mat_default_entry_set(
      sess_hdl, dev_tgt, mat_tbl_hdl, act_fn_hdl, &action_spec, 0, &entry_hdl);
  if (ret != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s: Error in setting default entry for table 0x%x for dev id %d, err "
        "%s",
        __func__,
        mat_tbl_hdl,
        dev_tgt.device_id,
        pipe_str_err(ret));
    return ret;
  }

  return PIPE_SUCCESS;
}

/* Re-download the default entry */
static pipe_status_t pipe_mgr_mat_default_entry_reprogram(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_mat_tbl_info_t *mat_tbl_info) {
  dev_target_t dev_tgt;
  pipe_mgr_tbl_prop_value_t prop_val;
  pipe_mgr_tbl_prop_args_t args_val;
  int i = 0, j = 0;
  pipe_status_t ret = PIPE_SUCCESS;
  pipe_mat_tbl_hdl_t mat_tbl_hdl = mat_tbl_info->handle;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s: Invalid device id %d", __func__, dev_id);
    return PIPE_INVALID_ARG;
  }

  dev_tgt.device_id = dev_id;

  /* Check if table is symmetric or not */
  ret = pipe_mgr_tbl_get_property(sess_hdl,
                                  dev_id,
                                  mat_tbl_hdl,
                                  PIPE_MGR_TABLE_ENTRY_SCOPE,
                                  &prop_val,
                                  &args_val);
  if (ret != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s: Error in getting tbl scope of table 0x%x for dev id %d, err %s",
        __func__,
        mat_tbl_hdl,
        dev_id,
        pipe_str_err(ret));
    return ret;
  }

  if (prop_val.scope == PIPE_MGR_ENTRY_SCOPE_ALL_PIPELINES) {
    dev_tgt.dev_pipe_id = BF_DEV_PIPE_ALL;
    ret = pipe_mgr_mat_default_entry_reprogram_helper(
        sess_hdl, dev_tgt, mat_tbl_info);
    if (ret != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s: Error in default entry programming for tbl 0x%x on dev id %d, "
          "err %s",
          __func__,
          mat_tbl_hdl,
          dev_id,
          pipe_str_err(ret));
      return ret;
    }
  } else if (prop_val.scope == PIPE_MGR_ENTRY_SCOPE_SINGLE_PIPELINE) {
    for (i = 0; i < (int)dev_info->num_active_pipes; i++) {
      dev_tgt.dev_pipe_id = i;
      ret = pipe_mgr_mat_default_entry_reprogram_helper(
          sess_hdl, dev_tgt, mat_tbl_info);
      if (ret != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s: Error in default entry programming for tbl 0x%x on dev id %d, "
            "err %s",
            __func__,
            mat_tbl_hdl,
            dev_id,
            pipe_str_err(ret));
        return ret;
      }
    }
  } else if (prop_val.scope == PIPE_MGR_ENTRY_SCOPE_USER_DEFINED) {
    for (i = 0; i < PIPE_MGR_MAX_USER_DEFINED_SCOPES; i++) {
      scope_pipes_t pipes = args_val.user_defined_entry_scope[i];
      if (pipes == 0) {
        continue;
      }
      for (j = 0; j < (int)sizeof(scope_pipes_t); j++) {
        if (pipes & (0x1 << j)) {
          break;
        }
      }
      if (j == sizeof(scope_pipes_t)) {
        continue;
      }
      dev_tgt.dev_pipe_id = j;
      ret = pipe_mgr_mat_default_entry_reprogram_helper(
          sess_hdl, dev_tgt, mat_tbl_info);
      if (ret != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s: Error in default entry programming for tbl 0x%x on dev id %d, "
            "err %s",
            __func__,
            mat_tbl_hdl,
            dev_id,
            pipe_str_err(ret));
        return ret;
      }
    }
  }

  return ret;
}

/* Add default entry during initialization and scope change. */
pipe_status_t pipe_mgr_tbl_add_init_default_entry(
    pipe_sess_hdl_t sess_hdl,
    rmt_dev_info_t *dev_info,
    pipe_mat_tbl_info_t *mat_tbl_info) {
  pipe_status_t ret = PIPE_SUCCESS;
  bf_dev_id_t dev_id = dev_info->dev_id;

  if (!mat_tbl_info) {
    return PIPE_INVALID_ARG;
  }

  /* Check for p4-specified default action if any */
  if (!mat_tbl_info->default_info || !mat_tbl_info->default_info->p4_default) {
    return PIPE_SUCCESS;
  }

  /* Build the action spec. */
  pipe_action_spec_t aspec;
  ret = pipe_mgr_create_action_spec(
      dev_id, &mat_tbl_info->default_info->action_entry, &aspec);
  if (PIPE_SUCCESS != ret) {
    LOG_ERROR(
        "Error %s creating action spec for default entry on dev %d table %s",
        pipe_str_err(ret),
        dev_id,
        mat_tbl_info->name);
    return ret;
  }

  /* Get the symmetric mode and pipe scopes for the table.  This may be called
   * after a HA event where the table scopes have already been set. */
  scope_pipes_t *scopes =
      PIPE_MGR_CALLOC(dev_info->dev_cfg.num_pipelines, sizeof *scopes);
  if (!scopes) return PIPE_NO_SYS_RESOURCES;
  scope_num_t num_scopes = 0;
  bool symmetric = true;
  ret = pipe_mgr_tbl_get_symmetric_mode(
      dev_id, mat_tbl_info->handle, &symmetric, &num_scopes, scopes);
  if (ret != PIPE_SUCCESS) {
    LOG_ERROR("%s: Dev %d tbl %s 0x%x: cannot get symmetric mode, %s",
              __func__,
              dev_id,
              mat_tbl_info->name,
              mat_tbl_info->handle,
              pipe_str_err(ret));
    PIPE_MGR_DBGCHK(ret == PIPE_SUCCESS);
    goto done;
  }

  /* Check for indirect action cases which need the default entry setup in the
   * ADT before the MAT. */
  bool indirect_adt = false;
  pipe_tbl_hdl_t adt_hdl;
  if (mat_tbl_info->num_adt_tbl_refs &&
      mat_tbl_info->adt_tbl_ref[0].ref_type == PIPE_TBL_REF_TYPE_INDIRECT) {
    /* The MAT does have an indirect ADT, if the action has action params then
     * the indirect ADT must be used.  Note that we don't worry about params
     * being allocated to immediate in the match overhead since the compiler
     * does not use immediate for indirect action cases. */
    if (aspec.act_data.num_valid_action_data_bits) {
      indirect_adt = true;
      adt_hdl = mat_tbl_info->adt_tbl_ref[0].tbl_hdl;
    }
  }

  for (scope_num_t s = 0; s < num_scopes; ++s) {
    dev_target_t dev_tgt;
    dev_tgt.device_id = dev_id;
    dev_tgt.dev_pipe_id = DEV_PIPE_ALL;
    for (unsigned lpipe = 0; !symmetric && lpipe < dev_info->num_active_pipes;
         ++lpipe) {
      if (scopes[s] & (1u << lpipe)) {
        dev_tgt.dev_pipe_id = lpipe;
        break;
      }
    }

    /* Setup the ADT entry if needed. */
    if (indirect_adt) {
      ret = pipe_mgr_adt_init_ent_add(
          sess_hdl,
          dev_tgt,
          mat_tbl_info->handle,
          adt_hdl,
          mat_tbl_info->default_info->action_entry.act_fn_hdl,
          &aspec,
          &aspec.adt_ent_hdl,
          0);
      if (ret != PIPE_SUCCESS) {
        goto done;
      }
      aspec.pipe_action_datatype_bmap = PIPE_ACTION_DATA_HDL_TYPE;
    }

    pipe_mat_ent_hdl_t entry_handle;
    ret = pipe_mgr_mat_default_entry_set(
        sess_hdl,
        dev_tgt,
        mat_tbl_info->handle,
        mat_tbl_info->default_info->action_entry.act_fn_hdl,
        &aspec,
        0,
        &entry_handle);
    if (ret != PIPE_SUCCESS) {
      LOG_ERROR("%s: Dev %d tbl %s 0x%x pipe %x: failed to init dflt entry, %s",
                __func__,
                dev_id,
                mat_tbl_info->name,
                mat_tbl_info->handle,
                dev_tgt.dev_pipe_id,
                pipe_str_err(ret));
      PIPE_MGR_DBGCHK(ret == PIPE_SUCCESS);
      goto done;
    }
  }

done:
  PIPE_MGR_FREE(scopes);
  if (aspec.act_data.action_data_bits)
    PIPE_MGR_FREE(aspec.act_data.action_data_bits);

  return ret;
}

static pipe_status_t static_entry_add_helper(pipe_sess_hdl_t sess_hdl,
                                             dev_target_t dev_tgt,
                                             pipe_mat_tbl_info_t *t) {
  pipe_mat_ent_hdl_t entry_handle;
  pipe_status_t rc = PIPE_SUCCESS;
  if (!t->num_static_entries) return rc;

  for (uint32_t i = 0; i < t->num_static_entries; ++i) {
    pipe_mgr_static_entry_info_t *e = t->static_entries + i;

    /* Build the action spec. */
    pipe_action_spec_t aspec;
    rc = pipe_mgr_create_action_spec(
        dev_tgt.device_id, &e->action_entry, &aspec);
    if (PIPE_SUCCESS != rc) {
      LOG_ERROR(
          "Error %s creating action spec for static entry %d in dev %d pipe %x "
          "table %s",
          pipe_str_err(rc),
          i,
          dev_tgt.device_id,
          dev_tgt.dev_pipe_id,
          t->name);
      return rc;
    }

    if (e->default_entry) {
      rc = pipe_mgr_mat_default_entry_set(sess_hdl,
                                          dev_tgt,
                                          t->handle,
                                          e->action_entry.act_fn_hdl,
                                          &aspec,
                                          0,
                                          &entry_handle);

      if (aspec.act_data.action_data_bits)
        PIPE_MGR_FREE(aspec.act_data.action_data_bits);

    } else {
      /* Create a match spec for the entry. */
      pipe_tbl_match_spec_t mspec;
      rc = pipe_mgr_create_match_spec(
          e->key, e->msk, e->len_bytes, e->len_bits, e->priority, &mspec);
      if (PIPE_SUCCESS != rc) {
        LOG_ERROR(
            "Error %s creating match spec for static entry %d in dev %d pipe "
            "%x table %s",
            pipe_str_err(rc),
            i,
            dev_tgt.device_id,
            dev_tgt.dev_pipe_id,
            t->name);
        if (aspec.act_data.action_data_bits)
          PIPE_MGR_FREE(aspec.act_data.action_data_bits);
        return rc;
      }

      rc = pipe_mgr_mat_ent_add(sess_hdl,
                                dev_tgt,
                                t->handle,
                                &mspec,
                                e->action_entry.act_fn_hdl,
                                &aspec,
                                0, /* TTL */
                                0, /* Flags */
                                &entry_handle);

      if (mspec.match_value_bits) PIPE_MGR_FREE(mspec.match_value_bits);
      if (mspec.match_mask_bits) PIPE_MGR_FREE(mspec.match_mask_bits);
      if (aspec.act_data.action_data_bits)
        PIPE_MGR_FREE(aspec.act_data.action_data_bits);
    }

    if (PIPE_SUCCESS != rc) {
      LOG_ERROR("Error %s adding static entry %d in dev %d pipe %x table %s",
                pipe_str_err(rc),
                i,
                dev_tgt.device_id,
                dev_tgt.dev_pipe_id,
                t->name);
      return rc;
    }
  }

  return rc;
}

/* Add static and default entries */
pipe_status_t pipe_mgr_static_and_default_entry_init(pipe_sess_hdl_t sess_hdl,
                                                     bf_dev_id_t dev_id) {
  pipe_status_t rc = PIPE_SUCCESS;

  LOG_TRACE("%s: Entered ", __func__);

  rmt_dev_info_t *di = pipe_mgr_get_dev_info(dev_id);
  if (!di) {
    return PIPE_INVALID_ARG;
  }

  bool fast_recfg_quick = pipe_mgr_init_mode_fast_recfg_quick(dev_id);

  /* Loop over all tables in all profiles and handle default and static entries
   * in each. */
  for (uint32_t p = 0; p < di->num_pipeline_profiles; p++) {
    rmt_dev_tbl_info_t *tbl_info = &(di->profile_info[p]->tbl_info_list);

    for (uint32_t i = 0; i < tbl_info->num_mat_tbls; ++i) {
      pipe_mat_tbl_hdl_t mat_tbl_hdl = tbl_info->mat_tbl_list[i].handle;

      pipe_mat_tbl_info_t *mat_tbl_info =
          pipe_mgr_get_tbl_info(dev_id, mat_tbl_hdl, __func__, __LINE__);
      if (!mat_tbl_info) {
        continue;
      }

      /* Program the initial default entry and any static entries the table
       * uses.  */
      if (!fast_recfg_quick) {
        if (mat_tbl_info->default_info) {
          pipe_mgr_tbl_add_init_default_entry(sess_hdl, di, mat_tbl_info);
        }

        dev_target_t dev_tgt;
        dev_tgt.device_id = dev_id;
        dev_tgt.dev_pipe_id = BF_DEV_PIPE_ALL;
        rc = static_entry_add_helper(sess_hdl, dev_tgt, mat_tbl_info);
        if (PIPE_SUCCESS != rc) {
          return rc;
        }
      } else {
        /* Reprogram the default entry in case of warm init quick */
        pipe_mgr_mat_default_entry_reprogram(sess_hdl, dev_id, mat_tbl_info);
      }
    }
  }

  return PIPE_SUCCESS;
}
