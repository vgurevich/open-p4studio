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
 * @file pipe_mgr_intf.c
 * @date
 *
 * Implementation of pipeline management interface
 */

/* Module header files */
#include <stdint.h>
#include <stddef.h>
#include <pipe_mgr/pipe_mgr_intf.h>
#include <pipe_mgr/pipe_mgr_dev_intf.h>
#include <dvm/bf_drv_intf.h>
#include <lld/bf_dma_if.h>
#include <lld/bf_dev_if.h>

/* Local header files */
#include "pipe_mgr_int.h"
#include "pipe_mgr_tcam.h"
#include "pipe_mgr_exm_tbl_init.h"
#include "pipe_mgr_exm_tbl_mgr.h"
#include "pipe_mgr_alpm.h"
#include "pipe_mgr_phase0_tbl_mgr.h"
#include "pipe_mgr_adt_init.h"
#include "pipe_mgr_select_tbl.h"
#include "pipe_mgr_act_tbl.h"
#include "pipe_mgr_tbl.h"
#include "pipe_mgr_stats_tbl.h"
#include "pipe_mgr_idle.h"
#include "pipe_mgr_meter_tbl.h"
#include "pipe_mgr_pktgen.h"
#include "pipe_mgr_learn.h"
#include "pipe_mgr_stful_tbl_mgr.h"
#include "pipe_mgr_phy_mem_map.h"
#include "pipe_mgr_interrupt.h"
#include "pipe_mgr_mau_snapshot.h"
#include "pipe_mgr_p4parser.h"
#include "pipe_mgr_hitless_ha.h"
#include "pipe_mgr_dkm.h"
#include "pipe_mgr_hitless_ha.h"
#include "pipe_mgr_exm_ha.h"
#include "pipe_mgr_tcam_ha.h"
#include "pipe_mgr_adt_mgr_ha_int.h"
#include "pipe_mgr_select_ha.h"
#include "pipe_mgr_move_list.h"
#include "pipe_mgr_p4parser.h"
#include "pipe_mgr_rmt_cfg.h"
#include "pipe_mgr_meter_mgr_int.h"
#include "pipe_mgr_parb.h"
#include "pipe_mgr_db.h"

/* Pointer to global pipe_mgr context */
pipe_mgr_ctx_t *pipe_mgr_ctx = NULL;
pipe_mgr_ctx_t *get_pipe_mgr_ctx() { return pipe_mgr_ctx; }

/* Stat table sync request values. Used then fetching multiple entries to sync
 * table only once. */
#define STAT_TBL_NO_SYNC 0
#define STAT_TBL_REQ_SYNC 1
#define STAT_TBL_SYNCED 2

/* Static functions. */
static void commitCb(void *arg, bool hadError);
static void commitCbAtom1(void *arg, bool hadError);
static void commitCbAtom2(void *arg, bool hadError);
static void commitCbAtom3(void *arg, bool hadError);
static void commitCbAtom4(void *arg, bool hadError);
static void clearTxnState(pipe_mgr_sess_ctx_t *s);

static bf_status_t pipe_mgr_add_device(bf_dev_id_t dev_id,
                                       bf_dev_family_t dev_family,
                                       bf_device_profile_t *prof,
                                       bf_dma_info_t *dma_info,
                                       bf_dev_init_mode_t warm_init_mode);
static bf_status_t pipe_mgr_add_virtual_device(
    bf_dev_id_t dev_id,
    bf_dev_type_t dev_type,
    bf_device_profile_t *prof,
    bf_dev_init_mode_t warm_init_mode);
/* API to de-instantiate a device */
static bf_status_t pipe_mgr_remove_device(bf_dev_id_t dev_id);
/* API to dump all pipe mgr state for a device */
static bf_status_t pipe_mgr_log_device(bf_dev_id_t dev_id, cJSON *dev);
/* API to restore pipe mgr state for a device */
static bf_status_t pipe_mgr_restore_device(bf_dev_id_t dev_id, cJSON *dev);

/* Callback functions used by session management to coordinate operations. */
static void commitCb(void *arg, bool hadError) {
  pipe_mgr_sess_ctx_t *s = arg;
  // LOG_TRACE("Entering %s: Session %u", __func__, s->hdl);

  PIPE_MGR_DBGCHK(!hadError);

  clearTxnState(s);
  pipe_mgr_sm_release(s->hdl);
}
/* Callback functions for an atomic operation.
 * Step 1 - Called upon completion of initial ilist.
 *        - Will update Global Version registers.
 * Step 2 - Called upon completion of Global Version update.
 *        - Will poll packet counts until zero.
 *        - Will issue cleanup message to Table Management.
 *        - Will push the cleanup ilist.
 * Step 3 - Called upon completion of cleanup ilist.
 *        - Will reset the Global Version register.
 * Step 4 - Called upon completion of Global Version reset.
 *        - Will send the commit response (in IPC mode).
 */
static void commitCbAtom1(void *arg, bool hadError) {
  commitCbAtom2(arg, hadError);
}
static void commitCbAtom2(void *arg, bool hadError) {
  commitCbAtom3(arg, hadError);
}
static void commitCbAtom3(void *arg, bool hadError) {
  commitCbAtom4(arg, hadError);
}
static void commitCbAtom4(void *arg, bool hadError) {
  pipe_mgr_sess_ctx_t *s = arg;
  // LOG_TRACE("Session %u Entering %s", s->hdl, __func__);
  PIPE_MGR_DBGCHK(!hadError);
  clearTxnState(s);
  pipe_mgr_sm_release(s->hdl);
  // LOG_TRACE("Session %u Exiting %s", s->hdl, __func__);
}

static void clearTxnState(pipe_mgr_sess_ctx_t *s) {
  s->txnInProg = false;
  s->txnIsAtom = false;
}

/* Helper function that checks if both the target pipe_id and the entry
 * handle pipe_id are in the same scope in order to properly reserve the access
 * to the given MAT table. */
static bool validate_pipe_ids(dev_target_t dev_tgt,
                              pipe_mat_tbl_hdl_t tbl_hdl,
                              pipe_mat_ent_hdl_t ent_hdl) {
  bf_dev_pipe_t pipe_id = dev_tgt.dev_pipe_id;

  /* First quick check. */
  if (pipe_id == BF_DEV_PIPE_ALL || (PIPE_GET_HDL_PIPE(ent_hdl) == pipe_id))
    return true;

  pipe_mat_tbl_info_t *mat_tbl_info =
      pipe_mgr_get_tbl_info(dev_tgt.device_id, tbl_hdl, __func__, __LINE__);
  if (!mat_tbl_info) return false;

  if (mat_tbl_info->symmetric) {
    return true;
  } else if (mat_tbl_info->scope_value == 0) {
    /* Asymmetric table. */
    /* If we get here then we already know that both pipe ids are different
     * from the above check, i.e. (PIPE_GET_HDL_PIPE(ent_hdl) == pipe_id) */
    return false;
  } else {
    /* This is a user defined scope. */
    /* Verify if both pipes belong to one of the table pipe's scope. */
    int i;
    pipe_mgr_tbl_prop_args_t args;
    args.value = mat_tbl_info->scope_value;
    bf_dev_pipe_t ent_pipe_id = PIPE_GET_HDL_PIPE(ent_hdl);
    for (i = 0; i < PIPE_MGR_MAX_USER_DEFINED_SCOPES; i++) {
      scope_pipes_t pipes = args.user_defined_entry_scope[i];
      if ((pipes & (0x1 << pipe_id)) && (pipes & (0x1 << ent_pipe_id))) {
        /* Same pipe's scope. */
        return true;
      }
    }
  }
  return false;
}

/*
 *
 * Placement Callbacks
 *
 */
bf_status_t pipe_register_mat_update_cb(pipe_sess_hdl_t sess_hdl,
                                        bf_dev_id_t device_id,
                                        pipe_mat_tbl_hdl_t tbl_hdl,
                                        pipe_mat_update_cb cb,
                                        void *cb_cookie) {
  /* The table must be a MAT. */
  if (PIPE_HDL_TYPE_MAT_TBL != PIPE_GET_HDL_TYPE(tbl_hdl)) {
    return PIPE_INVALID_ARG;
  }

  RMT_API(sess_hdl,
          0,
          pipe_mgr_verify_tbl_access(sess_hdl, device_id, tbl_hdl, true),
          pipe_mgr_sm_set_mat_cb(device_id, tbl_hdl, cb, cb_cookie))
}

bf_status_t pipe_register_adt_update_cb(pipe_sess_hdl_t sess_hdl,
                                        bf_dev_id_t device_id,
                                        pipe_adt_tbl_hdl_t tbl_hdl,
                                        pipe_adt_update_cb cb,
                                        void *cb_cookie) {
  /* The table must be an ADT. */
  if (PIPE_HDL_TYPE_ADT_TBL != PIPE_GET_HDL_TYPE(tbl_hdl)) {
    return PIPE_INVALID_ARG;
  }

  RMT_API(sess_hdl,
          0,
          pipe_mgr_verify_tbl_access(sess_hdl, device_id, tbl_hdl, true),
          pipe_mgr_sm_set_adt_cb(device_id, tbl_hdl, cb, cb_cookie))
}

bf_status_t pipe_register_sel_update_cb(pipe_sess_hdl_t sess_hdl,
                                        bf_dev_id_t device_id,
                                        pipe_sel_tbl_hdl_t tbl_hdl,
                                        pipe_sel_update_cb cb,
                                        void *cb_cookie) {
  /* The table must be a Selection table. */
  if (PIPE_HDL_TYPE_SEL_TBL != PIPE_GET_HDL_TYPE(tbl_hdl)) {
    return PIPE_INVALID_ARG;
  }

  RMT_API(sess_hdl,
          0,
          pipe_mgr_verify_tbl_access(sess_hdl, device_id, tbl_hdl, true),
          pipe_mgr_sm_set_sel_cb(device_id, tbl_hdl, cb, cb_cookie))
}

/*
 *
 * Placement Data Management
 *
 */
bf_status_t pipe_register_plcmnt_mem_fns(pipe_plcmnt_alloc alloc_fn,
                                         pipe_plcmnt_free free_fn) {
  if (alloc_fn)
    get_pipe_mgr_ctx()->alloc_fn = alloc_fn;
  else
    get_pipe_mgr_ctx()->alloc_fn = bf_sys_malloc;

  if (free_fn)
    get_pipe_mgr_ctx()->free_fn = free_fn;
  else
    get_pipe_mgr_ctx()->free_fn = bf_sys_free;
  return BF_SUCCESS;
}

bf_status_t bf_drv_plcmt_data_size(const void *data, size_t *size) {
  if (!data || !size) return BF_INVALID_ARG;
  int tag = *(pipe_mgr_data_tag_t *)data;
  switch (tag) {
    case PIPE_MGR_ENTRY_DATA_MAT:
      *size = mat_ent_data_size((const struct pipe_mgr_mat_data *)data);
      break;
    case PIPE_MGR_ENTRY_DATA_ADT:
      *size = adt_ent_data_size((pipe_mgr_adt_ent_data_t *)data);
      break;
    case PIPE_MGR_ENTRY_DATA_SEL:
      *size = sel_ent_data_size((const struct pipe_mgr_sel_data *)data);
      break;
    default:
      return BF_INVALID_ARG;
      break;
  }
  return BF_SUCCESS;
}

bf_status_t bf_drv_plcmt_copy(const void *src, void *dst) {
  if (!src || !dst) return BF_INVALID_ARG;
  int tag = *(pipe_mgr_data_tag_t *)src;
  switch (tag) {
    case PIPE_MGR_ENTRY_DATA_MAT:
      mat_ent_data_copy((const struct pipe_mgr_mat_data *)src,
                        (struct pipe_mgr_mat_data *)dst);
      break;
    case PIPE_MGR_ENTRY_DATA_ADT:
      adt_ent_data_copy((pipe_mgr_adt_ent_data_t *)src,
                        (pipe_mgr_adt_ent_data_t *)dst);
      break;
    case PIPE_MGR_ENTRY_DATA_SEL:
      sel_ent_data_copy((const struct pipe_mgr_sel_data *)src,
                        (struct pipe_mgr_sel_data *)dst);
      break;
    default:
      return BF_INVALID_ARG;
      break;
  }
  return BF_SUCCESS;
}
bf_status_t bf_drv_plcmt_duplicate(const void *src, void **copy) {
  if (!src || !copy) return BF_INVALID_ARG;
  size_t x = 0;
  bf_status_t r = bf_drv_plcmt_data_size(src, &x);
  if (BF_SUCCESS != r) return r;
  *copy = get_pipe_mgr_ctx()->alloc_fn(x);
  if (!*copy) return BF_NO_SYS_RESOURCES;
  return bf_drv_plcmt_copy(src, *copy);
}

bf_status_t bf_drv_plcmt_free(void *data) {
  get_pipe_mgr_ctx()->free_fn(data);
  return BF_SUCCESS;
}
bf_status_t bf_drv_plcmt_pack_data_size(const void *unpacked_data,
                                        size_t *size) {
  if (!unpacked_data || !size) return BF_INVALID_ARG;
  int tag = *(pipe_mgr_data_tag_t *)unpacked_data;
  switch (tag) {
    case PIPE_MGR_ENTRY_DATA_MAT:
      *size = mat_ent_data_size_packed(
          (const struct pipe_mgr_mat_data *)unpacked_data);
      return PIPE_SUCCESS;
    case PIPE_MGR_ENTRY_DATA_ADT:
      *size = adt_ent_data_size_packed(
          (const pipe_mgr_adt_ent_data_t *)unpacked_data);
      return PIPE_SUCCESS;
    case PIPE_MGR_ENTRY_DATA_SEL:
      *size = sel_ent_data_size_packed(
          (const struct pipe_mgr_sel_data *)unpacked_data);
      return PIPE_SUCCESS;
    default:
      return BF_INVALID_ARG;
  }
}
bf_status_t bf_drv_plcmt_unpack_data_size(const void *packed_data,
                                          size_t *size) {
  if (!packed_data || !size) return BF_INVALID_ARG;
  int tag = *(pipe_mgr_data_tag_t *)packed_data;
  switch (tag) {
    case PIPE_MGR_ENTRY_PACKED_DATA_MAT:
      *size = mat_ent_data_size_unpacked(packed_data);
      return PIPE_SUCCESS;
    case PIPE_MGR_ENTRY_PACKED_DATA_ADT:
      *size = adt_ent_data_size_unpacked(packed_data);
      return PIPE_SUCCESS;
    case PIPE_MGR_ENTRY_DATA_SEL:
      *size = sel_ent_data_size_unpacked(packed_data);
      return PIPE_SUCCESS;
    default:
      return BF_INVALID_ARG;
  }
}
bf_status_t bf_drv_plcmt_copy_pack(const void *unpacked_src, void *packed_dst) {
  if (!unpacked_src || !packed_dst) return BF_INVALID_ARG;
  int tag = *(pipe_mgr_data_tag_t *)unpacked_src;
  switch (tag) {
    case PIPE_MGR_ENTRY_DATA_MAT:
      return mat_ent_data_pack((const struct pipe_mgr_mat_data *)unpacked_src,
                               packed_dst);
    case PIPE_MGR_ENTRY_DATA_ADT:
      return adt_ent_data_pack((const pipe_mgr_adt_ent_data_t *)unpacked_src,
                               packed_dst);
    case PIPE_MGR_ENTRY_DATA_SEL:
      return sel_ent_data_pack((const struct pipe_mgr_sel_data *)unpacked_src,
                               packed_dst);
    default:
      return BF_INVALID_ARG;
  }
}
bf_status_t bf_drv_plcmt_copy_unpack(const void *packed_src,
                                     void *unpacked_dst) {
  if (!packed_src || !unpacked_dst) return BF_INVALID_ARG;
  int tag = *(pipe_mgr_data_tag_t *)packed_src;
  switch (tag) {
    case PIPE_MGR_ENTRY_PACKED_DATA_MAT:
      return mat_ent_data_unpack(packed_src,
                                 (struct pipe_mgr_mat_data *)unpacked_dst);
    case PIPE_MGR_ENTRY_PACKED_DATA_ADT:
      return adt_ent_data_unpack(packed_src,
                                 (pipe_mgr_adt_ent_data_t *)unpacked_dst);
    case PIPE_MGR_ENTRY_DATA_SEL:
      return sel_ent_data_unpack(packed_src,
                                 (struct pipe_mgr_sel_data *)unpacked_dst);
    default:
      return BF_INVALID_ARG;
  }
}

/* Placement Data Decode */
pipe_status_t pipe_mgr_plcmt_mat_data_get_entry(
    void *mat_data,
    pipe_tbl_match_spec_t *pipe_match_spec,
    pipe_action_spec_t *pipe_action_spec,
    pipe_act_fn_hdl_t *act_fn_hdl) {
  if (!mat_data) {
    LOG_ERROR("No data provided to decode");
    return PIPE_INVALID_ARG;
  }
  if (!pipe_match_spec || !pipe_action_spec || !act_fn_hdl) {
    LOG_ERROR("No pointer provided for decode results");
    return PIPE_INVALID_ARG;
  }
  if (*(pipe_mgr_data_tag_t *)mat_data == PIPE_MGR_ENTRY_PACKED_DATA_MAT) {
    /* This is a packed MAT data, unpack it and try again. */
    bf_status_t rc;
    size_t unpack_sz = 0;
    rc = bf_drv_plcmt_unpack_data_size(mat_data, &unpack_sz);
    if (rc != BF_SUCCESS) return rc;
    void *unpacked_data = PIPE_MGR_MALLOC(unpack_sz);
    if (!unpacked_data) return BF_NO_SYS_RESOURCES;
    rc = bf_drv_plcmt_copy_unpack(mat_data, unpacked_data);
    if (rc == BF_SUCCESS) {
      rc = pipe_mgr_plcmt_mat_data_get_entry(
          unpacked_data, pipe_match_spec, pipe_action_spec, act_fn_hdl);
    }
    PIPE_MGR_FREE(unpacked_data);
    return rc;
  } else if (*(pipe_mgr_data_tag_t *)mat_data != PIPE_MGR_ENTRY_DATA_MAT) {
    LOG_ERROR("Data is not MAT entry data");
    return PIPE_INVALID_ARG;
  }

  /* Perform structure copy so that we can overwrite pointers in the copy
   * without changing the original. */
  struct pipe_mgr_mat_data original = *(struct pipe_mgr_mat_data *)mat_data;
  /* Clear and reset the pointers in the entry data since we don't know if they
   * were originally valid or not. */
  mat_ent_data_prep_export(mat_data);
  mat_ent_data_prep_import(mat_data);
  pipe_tbl_match_spec_t *ms_tmp = unpack_mat_ent_data_ms(mat_data);
  pipe_action_spec_t *as_tmp = unpack_mat_ent_data_as(mat_data);
  *act_fn_hdl = unpack_mat_ent_data_afun_hdl(mat_data);
  pipe_mgr_tbl_copy_match_spec(pipe_match_spec, ms_tmp);
  uint8_t *ad_ptr = pipe_action_spec->act_data.action_data_bits;
  *pipe_action_spec = *as_tmp;
  pipe_action_spec->act_data.action_data_bits = ad_ptr;
  if (IS_ACTION_SPEC_ACT_DATA(as_tmp) &&
      as_tmp->act_data.num_action_data_bytes) {
    PIPE_MGR_MEMCPY(pipe_action_spec->act_data.action_data_bits,
                    as_tmp->act_data.action_data_bits,
                    as_tmp->act_data.num_action_data_bytes);
  }
  /* Restore the mat data passed in to its original state. */
  *(struct pipe_mgr_mat_data *)mat_data = original;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_plcmt_adt_data_get_entry(
    void *adt_data,
    pipe_action_data_spec_t *pipe_action_data_spec,
    pipe_act_fn_hdl_t *act_fn_hdl) {
  if (!adt_data) {
    LOG_ERROR("No data provided to decode");
    return PIPE_INVALID_ARG;
  }
  if (!pipe_action_data_spec || !act_fn_hdl) {
    LOG_ERROR("No pointer provided for decode results");
    return PIPE_INVALID_ARG;
  }
  if (*(pipe_mgr_data_tag_t *)adt_data == PIPE_MGR_ENTRY_PACKED_DATA_ADT) {
    /* This is a packed ADT data, unpack it and try again. */
    bf_status_t rc;
    size_t unpack_sz = 0;
    rc = bf_drv_plcmt_unpack_data_size(adt_data, &unpack_sz);
    if (rc != BF_SUCCESS) return rc;
    void *unpacked_data = PIPE_MGR_MALLOC(unpack_sz);
    if (!unpacked_data) return BF_NO_SYS_RESOURCES;
    rc = bf_drv_plcmt_copy_unpack(adt_data, unpacked_data);
    if (rc == BF_SUCCESS) {
      rc = pipe_mgr_plcmt_adt_data_get_entry(
          unpacked_data, pipe_action_data_spec, act_fn_hdl);
    }
    PIPE_MGR_FREE(unpacked_data);
    return rc;
  } else if (*(pipe_mgr_data_tag_t *)adt_data != PIPE_MGR_ENTRY_DATA_ADT) {
    LOG_ERROR("Data is not ADT entry data");
    return PIPE_INVALID_ARG;
  }

  /* Perform structure copy so that we can overwrite pointers in the copy
   * without changing the original. */
  pipe_mgr_adt_ent_data_t original = *(pipe_mgr_adt_ent_data_t *)adt_data;
  /* Clear and reset the pointers in the entry data since we don't know if they
   * were originally valid or not. */
  adt_ent_data_prep_export(adt_data);
  adt_ent_data_prep_import(adt_data);
  pipe_action_data_spec_t *ad_tmp = unpack_adt_ent_data_ad(adt_data);
  if (ad_tmp) {
    pipe_action_data_spec->num_action_data_bytes =
        ad_tmp->num_action_data_bytes;
    pipe_action_data_spec->num_valid_action_data_bits =
        ad_tmp->num_valid_action_data_bits;
    PIPE_MGR_MEMCPY(pipe_action_data_spec->action_data_bits,
                    ad_tmp->action_data_bits,
                    ad_tmp->num_action_data_bytes);
  }
  *act_fn_hdl = unpack_adt_ent_data_afun_hdl(adt_data);
  /* Restore the adt data passed in to its original state. */
  *(pipe_mgr_adt_ent_data_t *)adt_data = original;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_plcmt_mat_data_get(pipe_sess_hdl_t sess_hdl,
                                          bf_dev_id_t dev_id,
                                          pipe_mat_tbl_hdl_t mat_tbl_hdl) {
  pipe_status_t ret = PIPE_SUCCESS;
  /* Take a per session lock to ensure only one thread is accessing our driver
   * on the session at a time.  Note that this also validates the session. */
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }

  /* Verify that the session can access the table. */
  if (PIPE_SUCCESS !=
      (ret = pipe_mgr_verify_tbl_access(sess_hdl, dev_id, mat_tbl_hdl, true))) {
    pipe_mgr_api_exit(sess_hdl);
    return ret;
  }

  /* Get the placement data from the table manager in a move list. */
  pipe_mgr_move_list_t *move_list = NULL;
  enum pipe_mgr_table_owner_t owner;
  owner = pipe_mgr_sm_tbl_owner(dev_id, mat_tbl_hdl);
  if (PIPE_MGR_TBL_OWNER_EXM == owner) {
    ret = pipe_mgr_exm_get_plcmt_data(dev_id, mat_tbl_hdl, &move_list);
  } else if (PIPE_MGR_TBL_OWNER_TRN == owner) {
    ret = pipe_mgr_tcam_get_plcmt_data(dev_id, mat_tbl_hdl, &move_list);
  } else if (PIPE_MGR_TBL_OWNER_PHASE0 == owner) {
    ret = pipe_mgr_phase0_get_plcmt_data(dev_id, mat_tbl_hdl, &move_list);
  } else if (PIPE_MGR_TBL_OWNER_NO_KEY == owner) {
  } else {
    LOG_ERROR("No handler for table owner %u, table 0x%x at %s:%d",
              owner,
              mat_tbl_hdl,
              __func__,
              __LINE__);
    PIPE_MGR_DBGCHK(0);
    ret = PIPE_NOT_SUPPORTED;
  }

  /* If we've succesfully gotten the placement data generate the callbacks for
   * the application. */
  if (ret == PIPE_SUCCESS && move_list) {
    pipe_mgr_sm_issue_mat_cb(dev_id, mat_tbl_hdl, move_list);
  }
  if (move_list) {
    pipe_mgr_tbl_free_move_list(owner, move_list, false);
  }

  /* If we are not batching release the table reservation taken earlier. */
  if (!pipe_mgr_sess_in_batch(sess_hdl)) pipe_mgr_sm_release(sess_hdl);

  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t pipe_mgr_plcmt_adt_data_get(pipe_sess_hdl_t sess_hdl,
                                          bf_dev_id_t dev_id,
                                          pipe_adt_tbl_hdl_t adt_tbl_hdl) {
  pipe_status_t ret = PIPE_SUCCESS;
  /* Take a per session lock to ensure only one thread is accessing our driver
   * on the session at a time.  Note that this also validates the session. */
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }

  /* Verify that the session can access the table. */
  if (PIPE_SUCCESS !=
      (ret = pipe_mgr_verify_tbl_access(sess_hdl, dev_id, adt_tbl_hdl, true))) {
    pipe_mgr_api_exit(sess_hdl);
    return ret;
  }

  /* Get the placement data from the table manager in a move list. */
  pipe_mgr_adt_move_list_t *move_list = NULL;
  ret = pipe_mgr_adt_get_plcmt_data(dev_id, adt_tbl_hdl, &move_list);

  /* If we've succesfully gotten the placement data convert it from a move-list
   * to an ADT update for the caller. */
  if (ret == PIPE_SUCCESS) {
    pipe_mgr_sm_issue_adt_cb(dev_id, adt_tbl_hdl, move_list);
  }
  if (move_list) {
    pipe_mgr_tbl_free_move_list(
        PIPE_MGR_TBL_OWNER_ADT, (pipe_mgr_move_list_t *)move_list, false);
  }

  /* If we are not batching release the table reservation taken earlier. */
  if (!pipe_mgr_sess_in_batch(sess_hdl)) pipe_mgr_sm_release(sess_hdl);

  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t pipe_mgr_plcmt_sel_data_get(pipe_sess_hdl_t sess_hdl,
                                          bf_dev_id_t dev_id,
                                          pipe_sel_tbl_hdl_t sel_tbl_hdl) {
  pipe_status_t ret = PIPE_SUCCESS;
  /* Take a per session lock to ensure only one thread is accessing our driver
   * on the session at a time.  Note that this also validates the session. */
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }

  /* Verify that the session can access the table. */
  if (PIPE_SUCCESS !=
      (ret = pipe_mgr_verify_tbl_access(sess_hdl, dev_id, sel_tbl_hdl, true))) {
    pipe_mgr_api_exit(sess_hdl);
    return ret;
  }

  /* Get the placement data from the table manager in a move list. */
  pipe_mgr_sel_move_list_t *move_list = NULL;
  ret = pipe_mgr_sel_get_plcmt_data(dev_id, sel_tbl_hdl, &move_list);

  /* If we've succesfully gotten the placement data convert it from a move-list
   * to an ADT update for the caller. */
  if (ret == PIPE_SUCCESS) {
    pipe_mgr_sm_issue_sel_cb(dev_id, sel_tbl_hdl, move_list);
  }
  if (move_list) {
    pipe_mgr_tbl_free_move_list(
        PIPE_MGR_TBL_OWNER_SELECT, (pipe_mgr_move_list_t *)move_list, false);
  }

  /* If we are not batching release the table reservation taken earlier. */
  if (!pipe_mgr_sess_in_batch(sess_hdl)) pipe_mgr_sm_release(sess_hdl);

  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

/*
 *
 * Placement Operations
 *
 */
enum pipe_plcmt_instr {
  PIPE_PLACEMENT_OP_MAT_ADD = 1,
  PIPE_PLACEMENT_OP_MAT_MOV = 2,
  PIPE_PLACEMENT_OP_MAT_DEL = 3,
  PIPE_PLACEMENT_OP_MAT_MOD = 4,
  PIPE_PLACEMENT_OP_MAT_ADD_MULTI = 5,
  PIPE_PLACEMENT_OP_MAT_MOV_MULTI = 6,
  PIPE_PLACEMENT_OP_MAT_SET_DFLT = 7,
  PIPE_PLACEMENT_OP_MAT_CLR_DFLT = 8,
  PIPE_PLACEMENT_OP_ADT_ADD = 9,
  PIPE_PLACEMENT_OP_ADT_DEL = 10,
  PIPE_PLACEMENT_OP_ADT_MOD = 11,
  PIPE_PLACEMENT_OP_SEL_GRP_ADD = 12,
  PIPE_PLACEMENT_OP_SEL_GRP_DEL = 13,
  PIPE_PLACEMENT_OP_SEL_GRP_ACTIVATE = 14,
  PIPE_PLACEMENT_OP_SEL_GRP_DEACTIVATE = 15,
};

struct pipe_plcmt_op_multi {
  int array_length;
  pipe_idx_t *logical_index_base_array;
  uint8_t *logical_index_count_array;
  pipe_idx_t *src_logical_index_base_array;
  void *entry_data;
};
struct pipe_plcmt_op {
  uint8_t op_type;
  pipe_ent_hdl_t ent_hdl;
  pipe_idx_t logical_idx;
  pipe_idx_t indirect_selection_idx;
  pipe_idx_t indirect_action_idx;
  union {
    void *entry_data;
    struct pipe_plcmt_op_multi *multi;
  } u;
};
struct pipe_plcmt_info {
  int type;  // 0 = Empty, 1 = MAT, 2 = ADT, 3 = SEL
  void *ml, *ml_tail;
};

struct pipe_plcmt_info *pipe_create_plcmt_info() {
  struct pipe_plcmt_info *p = PIPE_MGR_MALLOC(sizeof(struct pipe_plcmt_info));
  if (p) p->ml = p->ml_tail = NULL;
  if (p) p->type = 0;
  return p;
}
void pipe_destroy_plcmt_info(struct pipe_plcmt_info *info) {
  if (!info) return;
  if (1 == info->type) {
    free_move_list((struct pipe_mgr_move_list_t **)(&info->ml), true);
  } else if (2 == info->type) {
    free_adt_move_list((struct pipe_mgr_adt_move_list_t **)(&info->ml));
  } else if (3 == info->type) {
    free_sel_move_list((struct pipe_mgr_sel_move_list_t **)(&info->ml));
  }
  PIPE_MGR_FREE(info);
}

static inline pipe_status_t set_mat_plcmt_op(struct pipe_plcmt_info *info,
                                             enum pipe_plcmt_instr op_type,
                                             bf_dev_pipe_t pipe,
                                             pipe_ent_hdl_t ent_hdl,
                                             pipe_idx_t logical_idx,
                                             pipe_idx_t indirect_selection_idx,
                                             pipe_idx_t indirect_action_idx,
                                             void *entry_data) {
  if (!info) return PIPE_INVALID_ARG;
  if (info->type == 0) {
    info->type = 1;
  } else if (info->type == 1) {
  } else {
    return PIPE_INVALID_ARG;
  }
  struct pipe_mgr_move_list_t *ml = NULL;
  switch (op_type) {
    case PIPE_PLACEMENT_OP_MAT_ADD:
      ml = alloc_move_list(NULL, PIPE_MAT_UPDATE_ADD, pipe);
      if (!ml) return PIPE_NO_SYS_RESOURCES;
      ml->data = entry_data;
      ml->entry_hdl = ent_hdl;
      ml->logical_sel_idx = indirect_selection_idx;
      ml->logical_action_idx = indirect_action_idx;
      ml->u.single.logical_idx = logical_idx;
      break;
    case PIPE_PLACEMENT_OP_MAT_MOV:
      ml = alloc_move_list(NULL, PIPE_MAT_UPDATE_MOV, pipe);
      if (!ml) return PIPE_NO_SYS_RESOURCES;
      ml->data = entry_data;
      ml->entry_hdl = ent_hdl;
      ml->logical_sel_idx = indirect_selection_idx;
      ml->logical_action_idx = indirect_action_idx;
      ml->u.single.logical_idx = logical_idx;
      break;
    case PIPE_PLACEMENT_OP_MAT_DEL:
      ml = alloc_move_list(NULL, PIPE_MAT_UPDATE_DEL, pipe);
      if (!ml) return PIPE_NO_SYS_RESOURCES;
      ml->data = entry_data;
      ml->entry_hdl = ent_hdl;
      ml->logical_sel_idx = indirect_selection_idx;
      ml->logical_action_idx = indirect_action_idx;
      ml->u.single.logical_idx = logical_idx;
      break;
    case PIPE_PLACEMENT_OP_MAT_MOD:
      ml = alloc_move_list(NULL, PIPE_MAT_UPDATE_MOD, pipe);
      if (!ml) return PIPE_NO_SYS_RESOURCES;
      ml->data = entry_data;
      ml->entry_hdl = ent_hdl;
      ml->logical_sel_idx = indirect_selection_idx;
      ml->logical_action_idx = indirect_action_idx;
      ml->old_data = NULL;
      break;
    case PIPE_PLACEMENT_OP_MAT_ADD_MULTI:
      // Call pipe_set_one_plcmt_op_mat_add_multi instead
      PIPE_MGR_DBGCHK(0);
      return PIPE_INVALID_ARG;
    case PIPE_PLACEMENT_OP_MAT_MOV_MULTI:
      // Call pipe_set_one_plcmt_op_mat_move_multi instead
      PIPE_MGR_DBGCHK(0);
      return PIPE_INVALID_ARG;
    case PIPE_PLACEMENT_OP_MAT_SET_DFLT:
      ml = alloc_move_list(NULL, PIPE_MAT_UPDATE_SET_DFLT, pipe);
      if (!ml) return PIPE_NO_SYS_RESOURCES;
      ml->data = entry_data;
      ml->entry_hdl = ent_hdl;
      ml->logical_sel_idx = indirect_selection_idx;
      ml->logical_action_idx = indirect_action_idx;
      ml->u.single.logical_idx = ~0;
      break;
    case PIPE_PLACEMENT_OP_MAT_CLR_DFLT:
      ml = alloc_move_list(NULL, PIPE_MAT_UPDATE_CLR_DFLT, pipe);
      if (!ml) return PIPE_NO_SYS_RESOURCES;
      ml->data = entry_data;
      ml->entry_hdl = ent_hdl;
      ml->u.single.logical_idx = ~0;
      break;
    case PIPE_PLACEMENT_OP_ADT_ADD:
    case PIPE_PLACEMENT_OP_ADT_DEL:
    case PIPE_PLACEMENT_OP_ADT_MOD:
    case PIPE_PLACEMENT_OP_SEL_GRP_ADD:
    case PIPE_PLACEMENT_OP_SEL_GRP_DEL:
    case PIPE_PLACEMENT_OP_SEL_GRP_ACTIVATE:
    case PIPE_PLACEMENT_OP_SEL_GRP_DEACTIVATE:
    default:
      PIPE_MGR_DBGCHK(0);
      return PIPE_INVALID_ARG;
  }

  /* Since the data may have been generated by another process, any pointers
   * within the data need to be reset for this process. */
  if (ml && ml->data) mat_ent_data_prep_import(ml->data);

  if (!info->ml) {
    info->ml = info->ml_tail = ml;
  } else {
    ((struct pipe_mgr_move_list_t *)(info->ml_tail))->next = ml;
    info->ml_tail = ml;
  }
  return PIPE_SUCCESS;
}
pipe_status_t pipe_set_one_plcmt_op_mat_add(struct pipe_plcmt_info *info,
                                            pipe_ent_hdl_t ent_hdl,
                                            bf_dev_pipe_t pipe,
                                            pipe_idx_t ent_idx,
                                            pipe_idx_t indirect_selection_idx,
                                            pipe_idx_t indirect_action_idx,
                                            void *ent_data) {
  return set_mat_plcmt_op(info,
                          PIPE_PLACEMENT_OP_MAT_ADD,
                          pipe,
                          ent_hdl,
                          ent_idx,
                          indirect_selection_idx,
                          indirect_action_idx,
                          ent_data);
}
pipe_status_t pipe_set_one_plcmt_op_mat_mov(struct pipe_plcmt_info *info,
                                            pipe_ent_hdl_t ent_hdl,
                                            pipe_idx_t ent_idx,
                                            pipe_idx_t indirect_selection_idx,
                                            pipe_idx_t indirect_action_idx,
                                            void *ent_data) {
  return set_mat_plcmt_op(info,
                          PIPE_PLACEMENT_OP_MAT_MOV,
                          BF_DEV_PIPE_ALL,
                          ent_hdl,
                          ent_idx,
                          indirect_selection_idx,
                          indirect_action_idx,
                          ent_data);
}
pipe_status_t pipe_set_one_plcmt_op_mat_mod(struct pipe_plcmt_info *info,
                                            pipe_ent_hdl_t ent_hdl,
                                            pipe_idx_t indirect_selection_idx,
                                            pipe_idx_t indirect_action_idx,
                                            void *ent_data) {
  return set_mat_plcmt_op(info,
                          PIPE_PLACEMENT_OP_MAT_MOD,
                          BF_DEV_PIPE_ALL,
                          ent_hdl,
                          ~0,
                          indirect_selection_idx,
                          indirect_action_idx,
                          ent_data);
}
pipe_status_t pipe_set_one_plcmt_op_mat_del(struct pipe_plcmt_info *info,
                                            pipe_ent_hdl_t ent_hdl) {
  return set_mat_plcmt_op(info,
                          PIPE_PLACEMENT_OP_MAT_DEL,
                          BF_DEV_PIPE_ALL,
                          ent_hdl,
                          ~0,
                          ~0,
                          ~0,
                          NULL);
}
pipe_status_t pipe_set_one_plcmt_op_mat_set_dflt(
    struct pipe_plcmt_info *info,
    pipe_ent_hdl_t ent_hdl,
    bf_dev_pipe_t pipe,
    pipe_idx_t indirect_selection_idx,
    pipe_idx_t indirect_action_idx,
    void *ent_data) {
  return set_mat_plcmt_op(info,
                          PIPE_PLACEMENT_OP_MAT_SET_DFLT,
                          pipe,
                          ent_hdl,
                          ~0,
                          indirect_selection_idx,
                          indirect_action_idx,
                          ent_data);
}
pipe_status_t pipe_set_one_plcmt_op_mat_clr_dflt(struct pipe_plcmt_info *info,
                                                 pipe_ent_hdl_t ent_hdl,
                                                 bf_dev_pipe_t pipe) {
  return set_mat_plcmt_op(info,
                          PIPE_PLACEMENT_OP_MAT_CLR_DFLT,
                          pipe,
                          ent_hdl,
                          ~0,
                          PIPE_MGR_LOGICAL_ACT_IDX_INVALID,
                          ~0,
                          NULL);
}
pipe_status_t pipe_set_one_plcmt_op_mat_add_multi(
    struct pipe_plcmt_info *info,
    pipe_ent_hdl_t ent_hdl,
    bf_dev_pipe_t pipe,
    pipe_idx_t indirect_selection_idx,
    pipe_idx_t indirect_action_idx,
    int array_length,
    struct pipe_multi_index *location_array,
    void *ent_data) {
  if (!info) return PIPE_INVALID_ARG;
  if (info->type == 0) {
    info->type = 1;
  } else if (info->type == 1) {
  } else {
    return PIPE_INVALID_ARG;
  }
  if (!location_array || 0 >= array_length) return PIPE_INVALID_ARG;
  if (!ent_data) return PIPE_INVALID_ARG;
  struct pipe_mgr_move_list_t *ml;
  ml = alloc_move_list(NULL, PIPE_MAT_UPDATE_ADD_MULTI, pipe);
  if (!ml) return PIPE_NO_SYS_RESOURCES;
  ml->entry_hdl = ent_hdl;
  ml->data = ent_data;
  ml->logical_sel_idx = indirect_selection_idx;
  ml->logical_action_idx = indirect_action_idx;
  ml->u.multi.array_sz = array_length;
  ml->u.multi.locations =
      PIPE_MGR_MALLOC(array_length * sizeof(struct pipe_multi_index));
  if (!ml->u.multi.locations) {
    free_move_list(&ml, true);
    return PIPE_NO_SYS_RESOURCES;
  }
  PIPE_MGR_MEMCPY(ml->u.multi.locations,
                  location_array,
                  array_length * sizeof(struct pipe_multi_index));

  /* Since the data may have been generated by another process, any pointers
   * within the data need to be reset for this process. */
  if (ml->data) mat_ent_data_prep_import(ml->data);

  if (!info->ml) {
    info->ml = info->ml_tail = ml;
  } else {
    ((struct pipe_mgr_move_list_t *)(info->ml_tail))->next = ml;
    info->ml_tail = ml;
  }
  return PIPE_SUCCESS;
}
pipe_status_t pipe_set_one_plcmt_op_mat_mov_multi(
    struct pipe_plcmt_info *info,
    pipe_ent_hdl_t ent_hdl,
    pipe_idx_t indirect_selection_idx,
    pipe_idx_t indirect_action_idx,
    int array_length,
    struct pipe_multi_index *location_array,
    void *ent_data) {
  if (!info) return PIPE_INVALID_ARG;
  if (info->type == 0) {
    info->type = 1;
  } else if (info->type == 1) {
  } else {
    return PIPE_INVALID_ARG;
  }
  if (!location_array) return PIPE_INVALID_ARG;
  struct pipe_mgr_move_list_t *ml;
  ml = alloc_move_list(NULL, PIPE_MAT_UPDATE_MOV_MULTI, BF_DEV_PIPE_ALL);
  if (!ml) return PIPE_NO_SYS_RESOURCES;
  ml->entry_hdl = ent_hdl;
  ml->data = ent_data;
  ml->logical_sel_idx = indirect_selection_idx;
  ml->logical_action_idx = indirect_action_idx;
  ml->u.multi.array_sz = array_length;
  ml->u.multi.locations =
      PIPE_MGR_MALLOC(array_length * sizeof(struct pipe_multi_index));
  if (!ml->u.multi.locations) {
    free_move_list(&ml, true);
    return PIPE_NO_SYS_RESOURCES;
  }
  PIPE_MGR_MEMCPY(ml->u.multi.locations,
                  location_array,
                  array_length * sizeof(struct pipe_multi_index));

  /* Since the data may have been generated by another process, any pointers
   * within the data need to be reset for this process. */
  if (ml->data) mat_ent_data_prep_import(ml->data);

  if (!info->ml) {
    info->ml = info->ml_tail = ml;
  } else {
    ((struct pipe_mgr_move_list_t *)(info->ml_tail))->next = ml;
    info->ml_tail = ml;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_set_one_plcmt_op_adt_add(struct pipe_plcmt_info *info,
                                            pipe_ent_hdl_t ent_hdl,
                                            bf_dev_pipe_t pipe,
                                            void *ent_data) {
  if (!info) return PIPE_INVALID_ARG;
  if (info->type == 0) {
    info->type = 2;
  } else if (info->type == 2) {
  } else {
    return PIPE_INVALID_ARG;
  }
  if (!ent_data) return PIPE_INVALID_ARG;
  pipe_mgr_adt_move_list_t *ml = alloc_adt_move_list(NULL, PIPE_ADT_UPDATE_ADD);
  if (!ml) return PIPE_NO_SYS_RESOURCES;
  ml->entry_hdl = ent_hdl;
  ml->data = ent_data;
  ml->pipe_id = pipe;

  /* Since the data may have been generated by another process, any pointers
   * within the data need to be reset for this process. */
  adt_ent_data_prep_import(ml->data);

  if (!info->ml) {
    info->ml = info->ml_tail = ml;
  } else {
    ((struct pipe_mgr_adt_move_list_t *)(info->ml_tail))->next = ml;
    info->ml_tail = ml;
  }
  return PIPE_SUCCESS;
}
pipe_status_t pipe_set_one_plcmt_op_adt_mod(struct pipe_plcmt_info *info,
                                            pipe_ent_hdl_t ent_hdl,
                                            void *ent_data) {
  if (!info) return PIPE_INVALID_ARG;
  if (info->type == 0) {
    info->type = 2;
  } else if (info->type == 2) {
  } else {
    return PIPE_INVALID_ARG;
  }
  if (!ent_data) return PIPE_INVALID_ARG;
  pipe_mgr_adt_move_list_t *ml = alloc_adt_move_list(NULL, PIPE_ADT_UPDATE_MOD);
  if (!ml) return PIPE_NO_SYS_RESOURCES;
  ml->entry_hdl = ent_hdl;
  ml->data = ent_data;

  /* Since the data may have been generated by another process, any pointers
   * within the data need to be reset for this process. */
  adt_ent_data_prep_import(ml->data);

  if (!info->ml) {
    info->ml = info->ml_tail = ml;
  } else {
    ((struct pipe_mgr_adt_move_list_t *)(info->ml_tail))->next = ml;
    info->ml_tail = ml;
  }
  return PIPE_SUCCESS;
}
pipe_status_t pipe_set_one_plcmt_op_adt_del(struct pipe_plcmt_info *info,
                                            pipe_ent_hdl_t ent_hdl) {
  if (!info) return PIPE_INVALID_ARG;
  if (info->type == 0) {
    info->type = 2;
  } else if (info->type == 2) {
  } else {
    return PIPE_INVALID_ARG;
  }
  pipe_mgr_adt_move_list_t *ml = alloc_adt_move_list(NULL, PIPE_ADT_UPDATE_DEL);
  if (!ml) return PIPE_NO_SYS_RESOURCES;
  ml->entry_hdl = ent_hdl;
  ml->data = NULL;

  if (!info->ml) {
    info->ml = info->ml_tail = ml;
  } else {
    ((struct pipe_mgr_adt_move_list_t *)(info->ml_tail))->next = ml;
    info->ml_tail = ml;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_set_one_plcmt_op_sel_grp_create(
    struct pipe_plcmt_info *info,
    pipe_sel_grp_hdl_t grp_hdl,
    bf_dev_pipe_t pipe,
    uint32_t num_indexes,
    uint32_t max_members,
    pipe_idx_t base_logical_index,
    int array_length,
    struct pipe_multi_index *location_array) {
  if (!info) return PIPE_INVALID_ARG;
  if (info->type == 0) {
    info->type = 3;
  } else if (info->type == 3) {
  } else {
    return PIPE_INVALID_ARG;
  }
  pipe_mgr_sel_move_list_t *ml =
      alloc_sel_move_list(NULL, PIPE_SEL_UPDATE_GROUP_CREATE, pipe);
  if (!ml) return PIPE_NO_SYS_RESOURCES;
  ml->sel_grp_hdl = grp_hdl;
  ml->sel_grp_size = num_indexes;
  ml->max_mbrs = max_members;
  ml->logical_sel_index = base_logical_index;
  ml->locations =
      PIPE_MGR_MALLOC(array_length * sizeof(struct pipe_multi_index));
  if (!ml->locations) {
    free_sel_move_list(&ml);
    return PIPE_NO_SYS_RESOURCES;
  }
  ml->locations_length = array_length;
  PIPE_MGR_MEMCPY(ml->locations,
                  location_array,
                  array_length * sizeof(struct pipe_multi_index));

  if (!info->ml) {
    info->ml = info->ml_tail = ml;
  } else {
    ((struct pipe_mgr_sel_move_list_t *)(info->ml_tail))->next = ml;
    info->ml_tail = ml;
  }
  return PIPE_SUCCESS;
}
pipe_status_t pipe_set_one_plcmt_op_sel_grp_destroy(
    struct pipe_plcmt_info *info,
    pipe_sel_grp_hdl_t grp_hdl,
    bf_dev_pipe_t pipe) {
  if (!info) return PIPE_INVALID_ARG;
  if (info->type == 0) {
    info->type = 3;
  } else if (info->type == 3) {
  } else {
    return PIPE_INVALID_ARG;
  }
  pipe_mgr_sel_move_list_t *ml =
      alloc_sel_move_list(NULL, PIPE_SEL_UPDATE_GROUP_DESTROY, pipe);
  if (!ml) return PIPE_NO_SYS_RESOURCES;
  ml->sel_grp_hdl = grp_hdl;

  if (!info->ml) {
    info->ml = info->ml_tail = ml;
  } else {
    ((struct pipe_mgr_sel_move_list_t *)(info->ml_tail))->next = ml;
    info->ml_tail = ml;
  }
  return PIPE_SUCCESS;
}
pipe_status_t pipe_set_one_plcmt_op_sel_add(struct pipe_plcmt_info *info,
                                            pipe_sel_grp_hdl_t grp_hdl,
                                            pipe_ent_hdl_t ent_hdl,
                                            bf_dev_pipe_t pipe,
                                            pipe_idx_t ent_idx,
                                            pipe_idx_t ent_subidx,
                                            void *ent_data) {
  if (!info) return PIPE_INVALID_ARG;
  if (info->type == 0) {
    info->type = 3;
  } else if (info->type == 3) {
  } else {
    return PIPE_INVALID_ARG;
  }
  if (!ent_data) return PIPE_INVALID_ARG;
  pipe_mgr_sel_move_list_t *ml =
      alloc_sel_move_list(NULL, PIPE_SEL_UPDATE_ADD, pipe);
  if (!ml) return PIPE_NO_SYS_RESOURCES;
  ml->sel_grp_hdl = grp_hdl;
  ml->adt_mbr_hdl = ent_hdl;
  ml->logical_sel_index = ent_idx;
  ml->logical_sel_subindex = ent_subidx;
  ml->data = ent_data;

  if (!info->ml) {
    info->ml = info->ml_tail = ml;
  } else {
    ((struct pipe_mgr_sel_move_list_t *)(info->ml_tail))->next = ml;
    info->ml_tail = ml;
  }
  return PIPE_SUCCESS;
}
pipe_status_t pipe_set_one_plcmt_op_sel_del(struct pipe_plcmt_info *info,
                                            pipe_sel_grp_hdl_t grp_hdl,
                                            pipe_ent_hdl_t ent_hdl,
                                            bf_dev_pipe_t pipe,
                                            pipe_idx_t ent_idx,
                                            pipe_idx_t ent_subidx) {
  if (!info) return PIPE_INVALID_ARG;
  if (info->type == 0) {
    info->type = 3;
  } else if (info->type == 3) {
  } else {
    return PIPE_INVALID_ARG;
  }
  pipe_mgr_sel_move_list_t *ml =
      alloc_sel_move_list(NULL, PIPE_SEL_UPDATE_DEL, pipe);
  if (!ml) return PIPE_NO_SYS_RESOURCES;
  ml->sel_grp_hdl = grp_hdl;
  ml->adt_mbr_hdl = ent_hdl;
  ml->logical_sel_index = ent_idx;
  ml->logical_sel_subindex = ent_subidx;

  if (!info->ml) {
    info->ml = info->ml_tail = ml;
  } else {
    ((struct pipe_mgr_sel_move_list_t *)(info->ml_tail))->next = ml;
    info->ml_tail = ml;
  }
  return PIPE_SUCCESS;
}
pipe_status_t pipe_set_one_plcmt_op_sel_activate(struct pipe_plcmt_info *info,
                                                 pipe_sel_grp_hdl_t grp_hdl,
                                                 pipe_ent_hdl_t ent_hdl,
                                                 bf_dev_pipe_t pipe,
                                                 pipe_idx_t ent_idx,
                                                 pipe_idx_t ent_subidx) {
  if (!info) return PIPE_INVALID_ARG;
  if (info->type == 0) {
    info->type = 3;
  } else if (info->type == 3) {
  } else {
    return PIPE_INVALID_ARG;
  }
  pipe_mgr_sel_move_list_t *ml =
      alloc_sel_move_list(NULL, PIPE_SEL_UPDATE_ACTIVATE, pipe);
  if (!ml) return PIPE_NO_SYS_RESOURCES;
  ml->sel_grp_hdl = grp_hdl;
  ml->adt_mbr_hdl = ent_hdl;
  ml->logical_sel_index = ent_idx;
  ml->logical_sel_subindex = ent_subidx;

  if (!info->ml) {
    info->ml = info->ml_tail = ml;
  } else {
    ((struct pipe_mgr_sel_move_list_t *)(info->ml_tail))->next = ml;
    info->ml_tail = ml;
  }
  return PIPE_SUCCESS;
}
pipe_status_t pipe_set_one_plcmt_op_sel_deactivate(struct pipe_plcmt_info *info,
                                                   pipe_sel_grp_hdl_t grp_hdl,
                                                   pipe_ent_hdl_t ent_hdl,
                                                   bf_dev_pipe_t pipe,
                                                   pipe_idx_t ent_idx,
                                                   pipe_idx_t ent_subidx) {
  if (!info) return PIPE_INVALID_ARG;
  if (info->type == 0) {
    info->type = 3;
  } else if (info->type == 3) {
  } else {
    return PIPE_INVALID_ARG;
  }
  pipe_mgr_sel_move_list_t *ml =
      alloc_sel_move_list(NULL, PIPE_SEL_UPDATE_DEACTIVATE, pipe);
  if (!ml) return PIPE_NO_SYS_RESOURCES;
  ml->sel_grp_hdl = grp_hdl;
  ml->adt_mbr_hdl = ent_hdl;
  ml->logical_sel_index = ent_idx;
  ml->logical_sel_subindex = ent_subidx;

  if (!info->ml) {
    info->ml = info->ml_tail = ml;
  } else {
    ((struct pipe_mgr_sel_move_list_t *)(info->ml_tail))->next = ml;
    info->ml_tail = ml;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_set_one_plcmt_op_sel_set_fallback(
    struct pipe_plcmt_info *info,
    pipe_ent_hdl_t ent_hdl,
    bf_dev_pipe_t pipe,
    void *ent_data) {
  if (!info) return PIPE_INVALID_ARG;
  if (info->type == 0) {
    info->type = 3;
  } else if (info->type == 3) {
  } else {
    return PIPE_INVALID_ARG;
  }
  if (!ent_data) return PIPE_INVALID_ARG;
  pipe_mgr_sel_move_list_t *ml =
      alloc_sel_move_list(NULL, PIPE_SEL_UPDATE_SET_FALLBACK, pipe);
  if (!ml) return PIPE_NO_SYS_RESOURCES;
  ml->adt_mbr_hdl = ent_hdl;
  ml->data = ent_data;

  if (!info->ml) {
    info->ml = info->ml_tail = ml;
  } else {
    ((struct pipe_mgr_sel_move_list_t *)(info->ml_tail))->next = ml;
    info->ml_tail = ml;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_set_one_plcmt_op_sel_clr_fallback(
    struct pipe_plcmt_info *info, bf_dev_pipe_t pipe) {
  if (!info) return PIPE_INVALID_ARG;
  if (info->type == 0) {
    info->type = 3;
  } else if (info->type == 3) {
  } else {
    return PIPE_INVALID_ARG;
  }
  pipe_mgr_sel_move_list_t *ml =
      alloc_sel_move_list(NULL, PIPE_SEL_UPDATE_CLR_FALLBACK, pipe);
  if (!ml) return PIPE_NO_SYS_RESOURCES;

  if (!info->ml) {
    info->ml = info->ml_tail = ml;
  } else {
    ((struct pipe_mgr_sel_move_list_t *)(info->ml_tail))->next = ml;
    info->ml_tail = ml;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_process_plcmt_info(pipe_sess_hdl_t sess_hdl,
                                      bf_dev_id_t dev_id,
                                      pipe_tbl_hdl_t tbl_hdl,
                                      struct pipe_plcmt_info *info,
                                      uint32_t pipe_api_flags,
                                      uint32_t *processed) {
  pipe_status_t ret = PIPE_SUCCESS;
  /* Take a per session lock to ensure only one thread is accessing our driver
   * on the session at a time.  Note that this also validates the session. */
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }

  /* Verify that the session can access the table. */
  if (PIPE_SUCCESS !=
      (ret = pipe_mgr_verify_tbl_access(sess_hdl, dev_id, tbl_hdl, true))) {
    pipe_mgr_api_exit(sess_hdl);
    return ret;
  }

  bool batch = pipe_mgr_sess_in_batch(sess_hdl);
  bool sync_api = pipe_api_flags & PIPE_FLAG_SYNC_REQ;
  enum pipe_mgr_table_owner_t owner = pipe_mgr_sm_tbl_owner(dev_id, tbl_hdl);

  *processed = 0;
  ret = pipe_mgr_tbl_process_move_list(
      sess_hdl, dev_id, tbl_hdl, owner, info->ml, processed, false);
  if (PIPE_SUCCESS != ret && batch) {
    /* Roll back the instruction list to the last successfully processed
     * placement data. */
    pipe_mgr_drv_ilist_rollback(sess_hdl);
  }
  if (!batch) {
    // Callback ptr value drives the handling, so set it only if all data
    // is there to call it.
    void *usrData =
        (sync_api) ? pipe_mgr_get_sess_ctx(sess_hdl, __func__, __LINE__) : NULL;
    pipe_mgr_drv_ilist_cb cb = (usrData) ? commitCb : NULL;
    ret = pipe_mgr_drv_ilist_push(&sess_hdl, cb, usrData);
    /* Release the tables now that the operation is complete and the
     * instruction list is pushed to LLD. */
    pipe_mgr_sm_release(sess_hdl);
    if (sync_api) pipe_mgr_drv_i_list_cmplt_all(&sess_hdl);
  }
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t pipe_set_adt_ent_hdl_in_mat_data(void *data,
                                               pipe_adt_ent_hdl_t adt_ent_hdl) {
  struct pipe_mgr_mat_data *mat_data = (struct pipe_mgr_mat_data *)data;
  pipe_action_spec_t *action_spec = unpack_mat_ent_data_as(mat_data);
  action_spec->adt_ent_hdl = adt_ent_hdl;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_set_sel_grp_hdl_in_mat_data(void *data,
                                               pipe_adt_ent_hdl_t sel_grp_hdl) {
  struct pipe_mgr_mat_data *mat_data = (struct pipe_mgr_mat_data *)data;
  pipe_action_spec_t *action_spec = unpack_mat_ent_data_as(mat_data);
  action_spec->sel_grp_hdl = sel_grp_hdl;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_set_ttl_in_mat_data(void *data, uint32_t ttl) {
  set_mat_ent_data_ttl(data, ttl);
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_hitless_ha_restore_virtual_dev_state(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_tbl_hdl_t tbl_hdl,
    struct pipe_plcmt_info *info,
    uint32_t *processed,
    pd_ha_restore_cb_1 cb1) {
  pipe_status_t ret = PIPE_SUCCESS;
  if (!pipe_mgr_is_device_virtual(dev_id)) {
    LOG_ERROR(
        "%s:%d Virtual dev state restore API cannot be called on a non virtual "
        "device id %d",
        __func__,
        __LINE__,
        dev_id);
    return PIPE_NOT_SUPPORTED;
  }

  /* Take a per session lock to ensure only one thread is accessing our driver
   * on the session at a time.  Note that this also validates the session. */
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }

  /* Verify that the session can access the table. */
  if (PIPE_SUCCESS !=
      (ret = pipe_mgr_verify_tbl_access(sess_hdl, dev_id, tbl_hdl, true))) {
    pipe_mgr_api_exit(sess_hdl);
    return ret;
  }
  enum pipe_mgr_table_owner_t owner = pipe_mgr_sm_tbl_owner(dev_id, tbl_hdl);
  *processed = 0;
  switch (owner) {
    case PIPE_MGR_TBL_OWNER_EXM:
      ret =
          pipe_mgr_exm_hlp_restore_state(dev_id, tbl_hdl, info->ml, processed);
      break;
    case PIPE_MGR_TBL_OWNER_TRN:
      ret =
          pipe_mgr_tcam_hlp_restore_state(dev_id, tbl_hdl, info->ml, processed);
      break;
    case PIPE_MGR_TBL_OWNER_ALPM:
      LOG_ERROR(
          "%s:%d ALPM tbl 0x%x, device id %d not supported for HA state "
          "restore",
          __func__,
          __LINE__,
          tbl_hdl,
          dev_id);
      PIPE_MGR_DBGCHK(0);
      break;
    case PIPE_MGR_TBL_OWNER_PHASE0:
      PIPE_MGR_DBGCHK(0);
      break;
    case PIPE_MGR_TBL_OWNER_ADT:
      ret = pipe_mgr_adt_hlp_restore_state(
          sess_hdl, dev_id, tbl_hdl, info->ml, processed, cb1);
      break;
    case PIPE_MGR_TBL_OWNER_SELECT:
      ret = pipe_mgr_sel_hlp_restore_state(
          sess_hdl, dev_id, tbl_hdl, info->ml, processed, cb1);
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      ret = PIPE_INVALID_ARG;
  }

  if ((ret == PIPE_SUCCESS) && pipe_mgr_is_tbl_mat_tbl(dev_id, tbl_hdl) &&
      (PIPE_MGR_TBL_OWNER_PHASE0 != owner)) {
    ret = pipe_mgr_restore_mat_tbl_key_state(dev_id, tbl_hdl, info->ml);
    if (ret != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in restoring match tbl key state for tbl 0x%x, device "
          "id %d, err %s",
          __func__,
          __LINE__,
          tbl_hdl,
          dev_id,
          pipe_str_err(ret));
    }
  }

  pipe_mgr_sm_release(sess_hdl);
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

static bf_status_t pipe_mgr_complete_hitless_hw_read(bf_dev_id_t dev_id) {
  pipe_status_t sts = PIPE_SUCCESS;
  bf_status_t status = BF_SUCCESS;

  LOG_TRACE("Entering %s device %u", __func__, dev_id);

  if (!pipe_mgr_ctx) {
    return BF_NOT_READY;
  }

  pipe_sess_hdl_t sess_hdl = pipe_mgr_get_int_sess_hdl();
  sts = pipe_mgr_api_enter(sess_hdl);
  if (PIPE_SUCCESS != sts) return BF_SESSION_NOT_FOUND;

  sts = pipe_mgr_hitless_ha_complete_hw_read(sess_hdl, dev_id);
  if (PIPE_SUCCESS != sts) {
    LOG_ERROR("%s: failed to complete hitless hw read for dev %d (%s %d)",
              __func__,
              dev_id,
              pipe_str_err(sts),
              sts);
    goto err_cleanup;
  }

  /* Re-add the static and default entries. */
  sts = pipe_mgr_static_and_default_entry_init(sess_hdl, dev_id);
  if (PIPE_SUCCESS != sts) {
    LOG_ERROR(
        "%s: failed to setup static and default entries after hitless hw read "
        "for dev %d (%s %d)",
        __func__,
        dev_id,
        pipe_str_err(sts),
        sts);
    goto err_cleanup;
  }

  pipe_mgr_api_exit(sess_hdl);
  LOG_TRACE("Exiting %s successfully", __func__);
  return status;

err_cleanup:
  pipe_mgr_api_exit(sess_hdl);
  return sts;
}

static bf_status_t pipe_mgr_compute_delta_changes(bf_dev_id_t dev_id,
                                                  bool disable_input_pkts) {
  pipe_status_t sts = PIPE_SUCCESS;
  bf_status_t status = BF_SUCCESS;

  LOG_TRACE("Entering %s device %u", __func__, dev_id);
  (void)disable_input_pkts;
  if (!pipe_mgr_ctx) {
    return BF_NOT_READY;
  }

  pipe_sess_hdl_t sess_hdl = pipe_mgr_get_int_sess_hdl();
  sts = pipe_mgr_api_enter(sess_hdl);
  if (PIPE_SUCCESS != sts) return BF_SESSION_NOT_FOUND;

  sts = pipe_mgr_hitless_ha_compute_delta_changes(sess_hdl, dev_id);
  if (PIPE_SUCCESS != sts) {
    LOG_ERROR(
        "%s: failed to compute delta changes for hitless ha for dev %d (%s %d)",
        __func__,
        dev_id,
        pipe_str_err(sts),
        sts);
    goto err_cleanup;
  }

  pipe_mgr_api_exit(sess_hdl);
  LOG_TRACE("Exiting %s successfully", __func__);
  return status;

err_cleanup:
  pipe_mgr_api_exit(sess_hdl);
  return sts;
}

static bf_status_t pipe_mgr_push_delta_changes(bf_dev_id_t dev_id) {
  pipe_status_t sts = PIPE_SUCCESS;

  LOG_TRACE("Entering %s device %u", __func__, dev_id);

  if (!pipe_mgr_ctx) {
    return BF_NOT_READY;
  }

  pipe_sess_hdl_t sess_hdl = pipe_mgr_get_int_sess_hdl();
  sts = pipe_mgr_api_enter(sess_hdl);
  if (PIPE_SUCCESS != sts) return BF_SESSION_NOT_FOUND;

  sts = pipe_mgr_hitless_ha_push_delta_changes(sess_hdl, dev_id);
  if (PIPE_SUCCESS != sts) {
    LOG_ERROR(
        "%s: failed to push delta changes for hitless ha for dev %d (%s %d)",
        __func__,
        dev_id,
        pipe_str_err(sts),
        sts);
    goto err_cleanup;
  }

#ifndef DEVICE_IS_EMULATOR
  /* This is the end of hitless HA.  Start the TCAM scrub timer now that we are
   * going back to normal operation. */
  sts = pipe_mgr_intr_start_scrub_timer(dev_id);
  if (PIPE_SUCCESS != sts) {
    pipe_mgr_api_exit(sess_hdl);
    return (bf_status_t)sts;
  }

/* Start the timer for port stuck detection */
#define PIPE_MGR_PORT_STUCK_TIMER_DEF_VAL 1000
  sts = pipe_mgr_port_stuck_detect_timer_init(
      dev_id, PIPE_MGR_PORT_STUCK_TIMER_DEF_VAL);
  if ((sts != PIPE_SUCCESS) && (sts != PIPE_NOT_SUPPORTED)) {
    pipe_mgr_api_exit(sess_hdl);
    return (bf_status_t)sts;
  }
#endif

  pipe_mgr_api_exit(sess_hdl);
  LOG_TRACE("Exiting %s successfully", __func__);
  return BF_SUCCESS;

err_cleanup:
  pipe_mgr_api_exit(sess_hdl);
  return sts;
}

static bf_status_t pipe_mgr_set_dev_type_virtual_dev_slave(bf_dev_id_t dev_id) {
  LOG_TRACE("Entering %s device %u", __func__, dev_id);

  if (!pipe_mgr_ctx) {
    return BF_NOT_READY;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s:%d  dev_info is not found for dev_id [%d]",
              __func__,
              __LINE__,
              (int)dev_id);

    return PIPE_OBJ_NOT_FOUND;
  }

  if (dev_info->virtual_device) {
    LOG_ERROR(
        "%s:%d Cannot set device type as virtual_dev Slave for a virtual "
        "device id %d",
        __func__,
        __LINE__,
        dev_id);
    return PIPE_NOT_SUPPORTED;
  }
  dev_info->virtual_dev_slave = true;
  return PIPE_SUCCESS;
}

/* Initialization of pipe_mgr service */
pipe_status_t pipe_mgr_init(void) {
  pipe_mgr_ctx_t *ctx;
  pipe_status_t sts = PIPE_SUCCESS;

  LOG_TRACE("Entering %s", __func__);

  /* If pipe_mgr is already initialized ignore */
  if (pipe_mgr_ctx) {
    return PIPE_SUCCESS;
  }

  /* Initialize context for service */
  ctx = PIPE_MGR_MALLOC(sizeof(pipe_mgr_ctx_t));
  if (!ctx) {
    LOG_ERROR("%s: failed to allocate memory for context", __func__);
    sts = PIPE_NO_SYS_RESOURCES;
    goto err_cleanup;
  }
  PIPE_MGR_MEMSET(ctx, 0, sizeof(pipe_mgr_ctx_t));
  unsigned i;
  for (i = 0; i < PIPE_MGR_MAX_SESSIONS; ++i) {
    ctx->pipe_mgr_sessions[i].hdl = i;
    PIPE_MGR_LOCK_R_INIT(ctx->pipe_mgr_sessions[i].api_in_prog);
  }
  PIPE_MGR_LOCK_INIT(ctx->ses_list_mtx);
  PIPE_MGR_RW_MUTEX_LOCK_INIT(ctx->api_lock);
  bf_map_init(&ctx->dev_info_map);
  bf_map_init(&ctx->dev_ctx);

  /* Store the context pointer */
  pipe_mgr_ctx = ctx;

  pipe_mgr_ctx->alloc_fn = bf_sys_malloc;
  pipe_mgr_ctx->free_fn = bf_sys_free;
  /* Initialize LLD interface. */
  sts = pipe_mgr_drv_init();
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s: failed to initialize drv interface, status %u", __func__, sts);
    goto err_cleanup;
  }

  /* Create a session for internal use */
  sts = pipe_mgr_create_session(&pipe_mgr_ctx->int_ses_hndl);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s: failed to initialize internal session, status %u", __func__, sts);
    goto err_cleanup;
  }

  /* Initalize Exact Match Table Management. */
  sts = pipe_mgr_exm_init();
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s: failed to initialize EXM interface, status %u", __func__, sts);
    goto err_cleanup;
  }

  /* Initialize Algorithmic LPM Management. */
  sts = pipe_mgr_alpm_init();
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s: failed to initialize ALPM interface, status %u", __func__, sts);
    goto err_cleanup;
  }

  /* Initalize Idle time Management. */
  sts = pipe_mgr_idle_init();
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("%s: failed to initialize idle time interface, status %u",
              __func__,
              sts);
    goto err_cleanup;
  }

  /* Register for notificatons */
  bf_drv_client_handle_t bf_drv_hdl;
  bf_status_t r = bf_drv_register("pipe-mgr", &bf_drv_hdl);
  if (r != BF_SUCCESS) {
    sts = PIPE_NOT_READY;
    LOG_ERROR("%s: Registration failed, sts %s", __func__, bf_err_str(r));
    PIPE_MGR_DBGCHK(r == BF_SUCCESS);
    goto err_cleanup;
  }
  bf_drv_client_callbacks_t callbacks = {0};
  callbacks.device_add = pipe_mgr_add_device;
  callbacks.virtual_device_add = pipe_mgr_add_virtual_device;
  callbacks.device_del = pipe_mgr_remove_device;
  callbacks.device_log = pipe_mgr_log_device;
  callbacks.device_restore = pipe_mgr_restore_device;
  callbacks.port_add = pipe_mgr_add_port;
  callbacks.port_del = pipe_mgr_remove_port;
  callbacks.err_intr_mode = pipe_mgr_err_interrupt_mode_set;

  callbacks.lock = pipe_mgr_lock_device;
  callbacks.create_dma = pipe_mgr_reconfig_create_dma;
  callbacks.disable_input_pkts = pipe_mgr_disable_traffic;
  callbacks.wait_for_flush = pipe_mgr_wait_for_traffic_flush;
  callbacks.unlock_reprogram_core = pipe_mgr_unlock_device_cb;
  callbacks.config_complete = pipe_mgr_config_complete;
  callbacks.enable_input_pkts = pipe_mgr_enable_traffic;
  callbacks.error_cleanup = pipe_mgr_reconfig_error_cleanup;
  callbacks.warm_init_quick = pipe_mgr_warm_init_quick;

  callbacks.complete_hitless_hw_read = pipe_mgr_complete_hitless_hw_read;
  callbacks.compute_delta_changes = pipe_mgr_compute_delta_changes;
  callbacks.push_delta_changes = pipe_mgr_push_delta_changes;
  callbacks.device_mode_virtual_dev_slave =
      pipe_mgr_set_dev_type_virtual_dev_slave;

  bf_drv_client_register_callbacks(bf_drv_hdl, &callbacks, BF_CLIENT_PRIO_3);

  /* Init Snapshot management */
  pipe_mgr_snapshot_init();

  LOG_TRACE("Exiting %s successfully", __func__);
  return PIPE_SUCCESS;

err_cleanup:
  if (pipe_mgr_ctx) {
    pipe_mgr_cleanup();
  }
  return sts;
}

/* CLeanup of pipe_mgr service */
void pipe_mgr_cleanup(void) {
  LOG_TRACE("Entering %s", __func__);

  if (!pipe_mgr_ctx) {
    return;
  }

  PIPE_MGR_FREE(pipe_mgr_ctx);

  pipe_mgr_ctx = NULL;
  LOG_TRACE("Exiting %s successfully", __func__);
  return;
}

/* Register a new pipe_mgr client */
pipe_status_t pipe_mgr_client_init(pipe_sess_hdl_t *sess_hdl_p) {
  pipe_status_t sts;

  if (!pipe_mgr_ctx) {
    *sess_hdl_p = 0;
    return PIPE_SUCCESS;
  }

  sts = pipe_mgr_create_session(sess_hdl_p);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("%s: failed to allocate session for client", __func__);
    goto err_cleanup;
  }

  return PIPE_SUCCESS;

err_cleanup:
  return sts;
}

/* De-register a pipe_mgr client */
pipe_status_t pipe_mgr_client_cleanup(pipe_sess_hdl_t def_sess_hdl) {
  pipe_status_t sts;

  sts = pipe_mgr_destroy_session(def_sess_hdl);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("%s: failed to cleanup client session %u, sts %s (%d)",
              __func__,
              def_sess_hdl,
              pipe_str_err(sts),
              sts);
    return sts;
  }

  LOG_TRACE("Cleaned up client session %u", def_sess_hdl);
  return PIPE_SUCCESS;
}

/* Begin a transaction on a session. Only one transaction can be in progress
 * on any given session
 */
pipe_status_t pipe_mgr_begin_txn(pipe_sess_hdl_t shdl, bool isAtomic) {
  pipe_status_t sts = PIPE_SUCCESS;
  if (PIPE_SUCCESS != (sts = pipe_mgr_api_enter(shdl))) {
    return sts;
  }

  pipe_mgr_sess_ctx_t *s = pipe_mgr_get_sess_ctx(shdl, __func__, __LINE__);
  if (!s) {
    sts = PIPE_SESSION_NOT_FOUND;
    goto done;
  }
  if (s->txnInProg) {
    LOG_ERROR("Session %u already has a transaction in progress", shdl);
    sts = PIPE_ALREADY_EXISTS;
    goto done;
  }
  if (s->batchInProg) {
    LOG_ERROR("Session %u already has a batch in progress", shdl);
    sts = PIPE_ALREADY_EXISTS;
    goto done;
  }
  s->txnInProg = true;
  s->txnIsAtom = isAtomic;

done:
  pipe_mgr_api_exit(shdl);
  return sts;
}

pipe_status_t pipe_mgr_verify_txn(pipe_sess_hdl_t shdl) {
  pipe_status_t sts = PIPE_SUCCESS;
  if (PIPE_SUCCESS != (sts = pipe_mgr_api_enter(shdl))) {
    return sts;
  }

  pipe_mgr_sess_ctx_t *s = pipe_mgr_get_sess_ctx(shdl, __func__, __LINE__);
  if (!s) {
    sts = PIPE_SESSION_NOT_FOUND;
    goto done;
  }
done:
  pipe_mgr_api_exit(shdl);
  return sts;
}

pipe_status_t pipe_mgr_abort_txn_int(pipe_sess_hdl_t shdl) {
  pipe_status_t sts = PIPE_SUCCESS;
  pipe_mgr_sess_ctx_t *s = pipe_mgr_get_sess_ctx(shdl, __func__, __LINE__);
  if (!s) {
    sts = PIPE_SESSION_NOT_FOUND;
    goto done;
  }
  /* Clear the instruction list. */
  pipe_mgr_drv_ilist_abort(&shdl);

  /* Clear the transaction state. */
  clearTxnState(s);

  /* Update table management to revert their state. */
  sts = pipe_mgr_sm_abort(shdl);

  /* Clear any table reservations by this session. */
  pipe_mgr_sm_release(shdl);

done:
  return sts;
}
pipe_status_t pipe_mgr_abort_txn(pipe_sess_hdl_t shdl) {
  pipe_status_t sts = PIPE_SUCCESS;
  if (PIPE_SUCCESS != (sts = pipe_mgr_api_enter(shdl))) {
    return sts;
  }

  if (pipe_mgr_sess_in_txn(shdl)) {
    sts = pipe_mgr_abort_txn_int(shdl);
  } else {
    sts = PIPE_OBJ_NOT_FOUND;
  }

  pipe_mgr_api_exit(shdl);
  return sts;
}

pipe_status_t pipe_mgr_commit_txn(pipe_sess_hdl_t shdl, bool hwSynchronous) {
  pipe_status_t sts = PIPE_SUCCESS;
  if (PIPE_SUCCESS != (sts = pipe_mgr_api_enter(shdl))) {
    return sts;
  }
  bool release = false;

  pipe_mgr_sess_ctx_t *s = pipe_mgr_get_sess_ctx(shdl, __func__, __LINE__);
  if (!s) {
    sts = PIPE_SESSION_NOT_FOUND;
    goto done;
  }
  if (!s->txnInProg) {
    sts = PIPE_OBJ_NOT_FOUND;
    goto done;
  }

  /* Update table management to commit their state. */
  pipe_mgr_sm_commit(shdl);

  /* And then only push the ilist */
  if (s->txnIsAtom) {
    sts = pipe_mgr_drv_ilist_push(&shdl, commitCbAtom1, s);
  } else if (hwSynchronous) {
    sts = pipe_mgr_drv_ilist_push(&shdl, commitCb, s);
  } else {
    sts = pipe_mgr_drv_ilist_push(&shdl, NULL, NULL);
    clearTxnState(s);
    release = true;
  }

  if (PIPE_SUCCESS != sts) {
    LOG_ERROR(
        "Table update fails, session %u at %s:%d", shdl, __func__, __LINE__);
  }

  /* The transaction flag will still be set for atomic and synchronous cases.
   * In both of these cases the completion callbacks will finish the
   * transaction so wait here (via cmplt_all) for those completions to
   * happen. */
  if (s->txnInProg) {
    pipe_mgr_drv_i_list_cmplt_all(&shdl);
  }

  /* It could be that the completion callbacks of atomic and synchronous
   * cases released the resources all ready so no need to do that here. */
  if (release) {
    pipe_mgr_sm_release(shdl);
  }

done:
  pipe_mgr_api_exit(shdl);
  return sts;
}

pipe_status_t pipe_mgr_begin_batch(pipe_sess_hdl_t shdl) {
  pipe_status_t sts = PIPE_SUCCESS;
  if (PIPE_SUCCESS != (sts = pipe_mgr_api_enter(shdl))) {
    return sts;
  }

  pipe_mgr_sess_ctx_t *s = pipe_mgr_get_sess_ctx(shdl, __func__, __LINE__);
  if (!s) {
    sts = PIPE_SESSION_NOT_FOUND;
    goto done;
  }
  if (s->txnInProg) {
    LOG_ERROR("Session %u already has a transaction in progress", shdl);
    sts = PIPE_ALREADY_EXISTS;
    goto done;
  }
  if (s->batchInProg) {
    LOG_ERROR("Session %u already has a batch in progress", shdl);
    sts = PIPE_ALREADY_EXISTS;
    goto done;
  }
  s->batchInProg = true;

done:
  pipe_mgr_api_exit(shdl);
  return sts;
}

pipe_status_t pipe_mgr_flush_batch(pipe_sess_hdl_t shdl) {
  pipe_status_t sts = PIPE_SUCCESS;
  if (PIPE_SUCCESS != (sts = pipe_mgr_api_enter(shdl))) {
    return sts;
  }

  if (!pipe_mgr_sess_in_batch(shdl)) {
    sts = PIPE_OBJ_NOT_FOUND;
    goto done;
  }

  sts = pipe_mgr_drv_ilist_push(&shdl, NULL, NULL);

  if (PIPE_SUCCESS != sts) {
    LOG_ERROR("Batch flush fails with \"%s\", session %u at %s:%d",
              pipe_str_err(sts),
              shdl,
              __func__,
              __LINE__);
  }

done:
  pipe_mgr_api_exit(shdl);
  return sts;
}

pipe_status_t pipe_mgr_end_batch(pipe_sess_hdl_t shdl, bool hwSynchronous) {
  pipe_status_t sts = PIPE_SUCCESS;
  if (PIPE_SUCCESS != (sts = pipe_mgr_api_enter(shdl))) {
    return sts;
  }

  pipe_mgr_sess_ctx_t *s = pipe_mgr_get_sess_ctx(shdl, __func__, __LINE__);
  if (!s) {
    sts = PIPE_SESSION_NOT_FOUND;
    goto done;
  }
  if (!s->batchInProg) {
    sts = PIPE_OBJ_NOT_FOUND;
    goto done;
  }

  s->batchInProg = false;
  sts = pipe_mgr_drv_ilist_push(&shdl, NULL, NULL);

  if (PIPE_SUCCESS != sts) {
    LOG_ERROR("Batch push fails with \"%s\", session %u at %s:%d",
              pipe_str_err(sts),
              shdl,
              __func__,
              __LINE__);
  }

  pipe_mgr_sm_release(shdl);

  if (hwSynchronous) {
    pipe_mgr_drv_i_list_cmplt_all(&shdl);
  }

done:
  pipe_mgr_api_exit(shdl);
  return sts;
}

static pipe_status_t complete_operations(pipe_sess_hdl_t shdl) {
  pipe_status_t sts = pipe_mgr_drv_i_list_cmplt_all(&shdl);
  for (int i = 0; i < PIPE_MGR_NUM_DEVICES && PIPE_SUCCESS == sts; ++i)
    sts = pipe_mgr_drv_wr_blk_cmplt_all(shdl, i);
  return sts;
}

pipe_status_t pipe_mgr_complete_operations(pipe_sess_hdl_t shdl) {
  pipe_status_t sts = pipe_mgr_api_enter(shdl);
  if (PIPE_SUCCESS != sts) {
    return sts;
  }

  if (!pipe_mgr_sess_in_batch(shdl)) {
    sts = complete_operations(shdl);
  }

  pipe_mgr_api_exit(shdl);
  return sts;
}

pipe_status_t pipe_mgr_tbl_is_tern(bf_dev_id_t dev_id,
                                   pipe_tbl_hdl_t tbl_hdl,
                                   bool *is_tern) {
  pipe_status_t sts;
  if (!is_tern) return PIPE_INVALID_ARG;
  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  if (PIPE_SUCCESS != (sts = pipe_mgr_api_enter(shdl))) {
    return sts;
  }

  pipe_mat_tbl_info_t *match_table =
      pipe_mgr_get_tbl_info(dev_id, tbl_hdl, __func__, __LINE__);
  if (!match_table) {
    pipe_mgr_api_exit(shdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  switch (match_table->match_type) {
    case TERNARY_MATCH:
    case LONGEST_PREFIX_MATCH:
    case ATCAM_MATCH:
    case ALPM_MATCH:
      *is_tern = true;
      break;
    case EXACT_MATCH:
      *is_tern = false;
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      pipe_mgr_api_exit(shdl);
      return PIPE_UNEXPECTED;
  }

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

static pipe_status_t ml_api_fin(pipe_sess_hdl_t sess_hdl,
                                pipe_status_t api_sts,
                                uint32_t pipe_api_flags,
                                bf_dev_id_t dev_id,
                                pipe_tbl_hdl_t tbl_hdl,
                                pipe_mgr_move_list_t *move_list) {
  enum pipe_mgr_table_owner_t owner = pipe_mgr_sm_tbl_owner(dev_id, tbl_hdl);
  bool txn = pipe_mgr_sess_in_txn(sess_hdl);
  bool batch = pipe_mgr_sess_in_batch(sess_hdl);
  bool virtual_dev = pipe_mgr_is_device_virtual(dev_id);
  bool sync_api = pipe_api_flags & PIPE_FLAG_SYNC_REQ;
  bool internal_api = pipe_api_flags & PIPE_FLAG_INTERNAL;

  if (PIPE_SUCCESS != api_sts) {
    /* The placement operation failed, free the move list and associated entry
     * data. */
    if (move_list) {
      switch (owner) {
        case PIPE_MGR_TBL_OWNER_EXM:
        case PIPE_MGR_TBL_OWNER_TRN:
        case PIPE_MGR_TBL_OWNER_ALPM:
        case PIPE_MGR_TBL_OWNER_PHASE0:
          free_move_list_and_data(&move_list, true);
          break;
        case PIPE_MGR_TBL_OWNER_ADT:
          free_adt_move_list_and_data((pipe_mgr_adt_move_list_t **)&move_list);
          break;
        case PIPE_MGR_TBL_OWNER_SELECT:
          free_sel_move_list_and_data((pipe_mgr_sel_move_list_t **)&move_list);
          break;
        case PIPE_MGR_TBL_OWNER_NO_KEY:
          break;
        case PIPE_MGR_TBL_OWNER_STFUL:
          pipe_mgr_stful_free_ops(
              (struct pipe_mgr_stful_op_list_t **)&move_list);
          break;
        case PIPE_MGR_TBL_OWNER_METER:
          pipe_mgr_meter_free_ops(
              (struct pipe_mgr_meter_op_list_t **)&move_list);
        default:
          break;
      }
    }
    if (txn) {
      /* Implicitly abort transactions after a failure.  The transaction abort
       * will also unlock any tables used by the session. */
      pipe_mgr_abort_txn_int(sess_hdl);
    } else if (batch) {
      /* The driver will no longer end batches implicitly on an API failure. */
    } else {
      /* Unlock all tables used by the failed operation. */
      pipe_mgr_sm_release(sess_hdl);
    }
  } else {
    if (!txn) {
      /* API was successful and it is not part of a transaction so issue the
       * callbacks for all operations on the move list. */
      if (!internal_api) {
        switch (owner) {
          case PIPE_MGR_TBL_OWNER_EXM:
          case PIPE_MGR_TBL_OWNER_TRN:
          case PIPE_MGR_TBL_OWNER_PHASE0:
            pipe_mgr_sm_issue_mat_cb(dev_id, tbl_hdl, move_list);
            break;
          case PIPE_MGR_TBL_OWNER_ADT:
            pipe_mgr_sm_issue_adt_cb(
                dev_id, tbl_hdl, (pipe_mgr_adt_move_list_t *)move_list);
            break;
          case PIPE_MGR_TBL_OWNER_SELECT:
            pipe_mgr_sm_issue_sel_cb(
                dev_id, tbl_hdl, (pipe_mgr_sel_move_list_t *)move_list);
            break;
          case PIPE_MGR_TBL_OWNER_NO_KEY:
            break;
          case PIPE_MGR_TBL_OWNER_STFUL:
            break; /* No callbacks for stateful tables. */
          case PIPE_MGR_TBL_OWNER_METER:
            break; /* No callbacks for meter tables. */
          default:
            break;
        }
      }

      /* If this is a physical device (i.e. not a virtual device) then send the
       * move list back to the table manager for immediate processing. */
      if (!virtual_dev) {
        /* If we are batching log the current instruction list state so we can
         * abort any new instructions if the programming we are about to
         * initiate fails. */
        if (batch) pipe_mgr_drv_ilist_chkpt(sess_hdl);
        uint32_t processed = 0;
        api_sts = pipe_mgr_tbl_process_move_list(
            sess_hdl, dev_id, tbl_hdl, owner, move_list, &processed, true);
      } else {
        pipe_mgr_tbl_free_move_list(owner, move_list, false);
      }
      if (!batch) {
        /* Not a transaction or batch, push or abort the ilist. */
        if (!virtual_dev) {
          if (PIPE_SUCCESS == api_sts) {
            // Callback ptr value drives the handling, so set it only if all
            // data is there to call it.
            void *usrData =
                (sync_api) ? pipe_mgr_get_sess_ctx(sess_hdl, __func__, __LINE__)
                           : NULL;
            pipe_mgr_drv_ilist_cb cb = (usrData) ? commitCb : NULL;
            api_sts = pipe_mgr_drv_ilist_push(&sess_hdl, cb, usrData);
          } else {
            pipe_mgr_drv_ilist_abort(&sess_hdl);
          }
        }
        /* Release the tables now that the operation is complete and the
         * instruction list is pushed to LLD. */
        pipe_mgr_sm_release(sess_hdl);
        if (!virtual_dev && sync_api) pipe_mgr_drv_i_list_cmplt_all(&sess_hdl);
      } else {
        if (!virtual_dev) {
          if (PIPE_SUCCESS != api_sts) {
            /* Programming the physical device failed inside a batch.  Rollback
             * the instruction list to the last successful API in the batch. */
            pipe_mgr_drv_ilist_rollback(sess_hdl);
          }
        }
      }
    } else {
      /* The placement API was successful and it is part of a transaction.  Save
       * the move
       * list against the table for later processing on either a commit or
       * abort. */
      if (move_list) {
        pipe_mgr_sm_save_ml(sess_hdl, dev_id, tbl_hdl, move_list);
      }
    }
  }
  pipe_mgr_api_exit(sess_hdl);
  return api_sts;
}

static pipe_status_t ml_api_prologue_v2(pipe_sess_hdl_t sess_hdl,
                                        dev_target_t dev_tgt,
                                        pipe_tbl_hdl_t tbl_hdl) {
  bf_dev_id_t dev_id = dev_tgt.device_id;
  pipe_status_t ret = PIPE_SUCCESS;

  /* Take a per session lock to ensure only one thread is accessing our driver
   * on the session at a time.  Note that this also validates the session. */
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }

  /* Verify that the session can access the table. */
  if (PIPE_SUCCESS != (ret = pipe_mgr_verify_pipe_tbl_access(
                           sess_hdl, dev_tgt, tbl_hdl, true))) {
    ml_api_fin(sess_hdl, ret, 0, dev_id, tbl_hdl, NULL);
    return ret;
  }

  /* If a transaction, make sure there is memory to hold the resulting move
   * list. */
  if (pipe_mgr_sess_in_txn(sess_hdl)) {
    ret = pipe_mgr_sm_save_ml_prep(sess_hdl, dev_id, tbl_hdl);
    if (PIPE_SUCCESS != ret)
      ml_api_fin(sess_hdl, ret, 0, dev_id, tbl_hdl, NULL);
  }
  return ret;
}

static pipe_status_t ml_api_prologue(pipe_sess_hdl_t sess_hdl,
                                     bf_dev_id_t dev_id,
                                     pipe_tbl_hdl_t tbl_hdl) {
  dev_target_t dev_tgt;
  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = BF_DEV_PIPE_ALL;
  return ml_api_prologue_v2(sess_hdl, dev_tgt, tbl_hdl);
}

pipe_status_t handleTableApiRsp(pipe_sess_hdl_t sess_hdl,
                                pipe_status_t sts,
                                bool isSync,
                                const char *where,
                                const int line) {
  pipe_status_t ret = PIPE_SUCCESS;
  pipe_mgr_sess_ctx_t *s = pipe_mgr_get_sess_ctx(sess_hdl, __func__, __LINE__);
  if (!s) {
    return PIPE_SESSION_NOT_FOUND;
  }

  if (PIPE_SUCCESS != sts) {
    LOG_ERROR("Table update fails \"%s\" (%u), session %u at %s:%d",
              pipe_str_err(sts),
              sts,
              sess_hdl,
              where,
              line);
    ret = sts;
    if (s->txnInProg) {
      /* Implicitly abort transactions after a failure.  The transaction
       * abort will also unlock any tables used by the session. */
      pipe_mgr_abort_txn_int(sess_hdl);
    } else if (s->batchInProg) {
      /* Rollback the batched ilist to the last successful API. */
      pipe_mgr_drv_ilist_rollback(sess_hdl);
    } else {
      /* Abort the ilist and unlock all tables used by the failed operation. */
      pipe_mgr_drv_ilist_abort(&sess_hdl);
      pipe_mgr_sm_release(sess_hdl);
    }
  } else if (!s->txnInProg && !s->batchInProg) {
    /* Non-transaciton case, push the instruction list now since there will
     * not be a commit() call. */
    pipe_mgr_drv_ilist_cb cb = NULL;
    void *userData = NULL;
    if (isSync) {
      cb = commitCb;
      userData = s;
      pipe_mgr_drv_i_list_cmplt_all(&sess_hdl);
    }
    ret = pipe_mgr_drv_ilist_push(&sess_hdl, cb, userData);
    /* Release the tables now that the operation is complete and the
     * instruction list is pushed to LLD. */
    pipe_mgr_sm_release(sess_hdl);
  }

  return ret;
}

/* Function to check if a valid match spec exists for the table */
static inline bool pipe_mgr_match_spec_exists(
    pipe_tbl_match_spec_t *match_spec) {
  if (match_spec == NULL) {
    return false;
  }

  if (match_spec->num_match_bytes > 0) {
    return true;
  }
  return false;
}

/* Function to check if the pipe argument is valid */
pipe_status_t pipe_mgr_verify_pipe_id(dev_target_t dev_tgt,
                                      pipe_mat_tbl_info_t *mat_tbl_info,
                                      bool light_pipe_validation) {
  rmt_dev_profile_info_t *profile_info = NULL;
  if ((mat_tbl_info->symmetric) && (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL)) {
    if (light_pipe_validation) {
      profile_info = pipe_mgr_get_profile_info(
          dev_tgt.device_id, mat_tbl_info->profile_id, __func__, __LINE__);
      if (!profile_info) {
        LOG_ERROR(
            "%s:%d Error, no dev_profile_info found for device_id [%d],"
            " profile_id [%d]",
            __func__,
            __LINE__,
            dev_tgt.device_id,
            mat_tbl_info->profile_id);
        return PIPE_OBJ_NOT_FOUND;
      }
      if (!PIPE_BITMAP_GET(&profile_info->pipe_bmp, dev_tgt.dev_pipe_id)) {
        LOG_ERROR(
            "%s:%d Invalid request to access pipe %x for symmetric table %s "
            "0x%x device id %d",
            __func__,
            __LINE__,
            dev_tgt.dev_pipe_id,
            mat_tbl_info->name,
            mat_tbl_info->handle,
            dev_tgt.device_id);
        return PIPE_INVALID_ARG;
      }
    } else {
      LOG_ERROR(
          "%s:%d Invalid request to access asymmetric pipe %d"
          " in symmetric table 0x%x device id %d. PIPE_ID_ALL is the only pipe"
          " supported by symmetric tables",
          __func__,
          __LINE__,
          dev_tgt.dev_pipe_id,
          mat_tbl_info->handle,
          dev_tgt.device_id);
      return PIPE_INVALID_ARG;
    }
  }
  if (!mat_tbl_info->symmetric) {
    profile_info = pipe_mgr_get_profile_info(
        dev_tgt.device_id, mat_tbl_info->profile_id, __func__, __LINE__);
    if (!profile_info) {
      LOG_ERROR(
          "%s:%d Error, no dev_profile_info found for device_id [%d],"
          " profile_id [%d]",
          __func__,
          __LINE__,
          dev_tgt.device_id,
          mat_tbl_info->profile_id);
      return PIPE_OBJ_NOT_FOUND;
    }
    if (light_pipe_validation && dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL)
      return PIPE_SUCCESS;
    if (!PIPE_BITMAP_GET(&profile_info->pipe_bmp, dev_tgt.dev_pipe_id)) {
      LOG_ERROR(
          "%s:%d Invalid request to access pipe %x for asymmetric table %s "
          "0x%x device id %d",
          __func__,
          __LINE__,
          dev_tgt.dev_pipe_id,
          mat_tbl_info->name,
          mat_tbl_info->handle,
          dev_tgt.device_id);
      return PIPE_INVALID_ARG;
    }
  }
  return PIPE_SUCCESS;
}

/*!
 * API to free a pipe_tbl_match_spec_t
 */
pipe_status_t pipe_mgr_match_spec_free(pipe_tbl_match_spec_t *match_spec) {
  if (match_spec) {
    if (match_spec->match_value_bits) {
      PIPE_MGR_FREE(match_spec->match_value_bits);
      match_spec->match_value_bits = NULL;
    }
    if (match_spec->match_mask_bits) {
      PIPE_MGR_FREE(match_spec->match_mask_bits);
      match_spec->match_mask_bits = NULL;
    }
    PIPE_MGR_FREE(match_spec);
  }
  return PIPE_SUCCESS;
}

/*!
 * API to duplicate an existing match spec to a new match spec
 * It allocates new memory. (dynamic memory)
 */
pipe_status_t pipe_mgr_match_spec_duplicate(
    pipe_tbl_match_spec_t **match_spec_dest,
    pipe_tbl_match_spec_t const *match_spec_src) {
  pipe_tbl_match_spec_t *p = PIPE_MGR_MALLOC(sizeof(pipe_tbl_match_spec_t));
  if (!p) goto err1;
  *p = *match_spec_src;

  if (!p->num_match_bytes) goto err2;

  p->match_value_bits = PIPE_MGR_MALLOC(p->num_match_bytes);
  if (!p->match_value_bits) goto err2;

  p->match_mask_bits = PIPE_MGR_MALLOC(p->num_match_bytes);
  if (!p->match_mask_bits) goto err3;

  PIPE_MGR_MEMCPY(p->match_value_bits,
                  match_spec_src->match_value_bits,
                  p->num_match_bytes);
  PIPE_MGR_MEMCPY(
      p->match_mask_bits, match_spec_src->match_mask_bits, p->num_match_bytes);

  *match_spec_dest = p;
  return PIPE_SUCCESS;

err3:
  PIPE_MGR_FREE(p->match_value_bits);
  p->match_value_bits = NULL;
err2:
  PIPE_MGR_FREE(p);
err1:
  return PIPE_NO_SYS_RESOURCES;
}

/* Function to convert an entry handle to its' match spec */
pipe_status_t pipe_mgr_ent_hdl_to_match_spec_internal(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    bf_dev_pipe_t *ent_pipe_id,
    pipe_tbl_match_spec_t const **match_spec_out) {
  bf_dev_pipe_t pipe_id = -1;
  pipe_tbl_match_spec_t *match_spec = NULL;
  pipe_status_t cur_status = PIPE_SUCCESS;

  enum pipe_mgr_table_owner_t owner;
  owner = pipe_mgr_sm_tbl_owner(dev_id, mat_tbl_hdl);

  /* Validate Table Infomation on getting spec info*/
  pipe_mat_tbl_info_t *mat_tbl_info =
      pipe_mgr_get_tbl_info(dev_id, mat_tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    LOG_ERROR(
        "Error in finding the table info for tbl 0x%x"
        " device id %d",
        mat_tbl_hdl,
        dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  /* End of Validating Table Infomation on getting spec info*/

  /* Convert Entry Handle to Match Spec for the appropriate table manager. */
  if (PIPE_MGR_TBL_OWNER_EXM == owner) {
    cur_status = pipe_mgr_exm_get_match_spec(
        mat_ent_hdl, mat_tbl_hdl, dev_id, &pipe_id, &match_spec);
  } else if (PIPE_MGR_TBL_OWNER_TRN == owner) {
    cur_status = pipe_mgr_tcam_get_match_spec(
        mat_ent_hdl, mat_tbl_hdl, dev_id, &pipe_id, &match_spec);
  } else if (PIPE_MGR_TBL_OWNER_ALPM == owner) {
    cur_status = pipe_mgr_alpm_get_match_spec(
        mat_ent_hdl, mat_tbl_hdl, dev_id, &pipe_id, &match_spec);
  } else if (PIPE_MGR_TBL_OWNER_PHASE0 == owner) {
    cur_status = pipe_mgr_phase0_get_match_spec(
        mat_ent_hdl, mat_tbl_hdl, dev_id, &pipe_id, &match_spec);
  } else if (PIPE_MGR_TBL_OWNER_NO_KEY == owner) {
    // NO-OP
  } else {
    LOG_ERROR("No table owner for table handler 0x%x at %s:%d",
              mat_tbl_hdl,
              __func__,
              __LINE__);
    cur_status = PIPE_UNEXPECTED;
    PIPE_MGR_DBGCHK(0);
  }
  if (cur_status != PIPE_SUCCESS) {
    if (cur_status == PIPE_OBJ_NOT_FOUND) {
      LOG_TRACE("%s:%d Failed to get match spec for table handle 0x%x",
                __func__,
                __LINE__,
                mat_tbl_hdl);

      return PIPE_OBJ_NOT_FOUND;
    } else {
      LOG_ERROR("%s:%d Failed to get match spec for table handle 0x%x",
                __func__,
                __LINE__,
                mat_tbl_hdl);
      return PIPE_UNEXPECTED;
    }
  }

  if (pipe_mgr_match_spec_exists(match_spec)) {
    *match_spec_out = match_spec;
    if (ent_pipe_id) *ent_pipe_id = pipe_id;
    return PIPE_SUCCESS;
  } else {
    LOG_TRACE(
        "Error: match spec does not exist for tbl 0x%x"
        " device id %d handle 0x%x",
        mat_tbl_hdl,
        dev_id,
        mat_ent_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }
}

/* Function to check if provided values match key masks.
 * Returns 1 if value has bits set outside of mask.
 *         0 otherwise. */
static bool pipe_mgr_check_table_global_key_mask(
    pipe_mat_tbl_info_t *mat_tbl_info, pipe_tbl_match_spec_t *match_spec) {
  uint8_t mask_idx;

  if (!mat_tbl_info->tbl_global_key_mask_valid) return 0;

  // mask out the key
  for (mask_idx = 0; mask_idx < mat_tbl_info->tbl_global_key_mask_len;
       mask_idx++) {
    if (match_spec->match_value_bits[mask_idx] &
        ~mat_tbl_info->tbl_global_key_mask_bits[mask_idx])
      return 1;
    if (mat_tbl_info->match_type != EXACT_MATCH) {
      match_spec->match_mask_bits[mask_idx] &=
          mat_tbl_info->tbl_global_key_mask_bits[mask_idx];
    }
  }
  return 0;
}

/* Search in the scope where dev_tgt.dev_pipe_id belongs to. */
static pipe_status_t mat_tbl_key_exists_any_pipe(
    dev_target_t dev_tgt,
    pipe_mat_tbl_info_t *mat_tbl_info,
    pipe_tbl_match_spec_t *match_spec,
    bool *exists,
    pipe_mat_ent_hdl_t *mat_ent_hdl) {
  pipe_status_t ret = PIPE_SUCCESS;
  uint32_t q = 0;
  bool symmetric = false;
  scope_num_t num_scopes;
  scope_pipes_t scope_pipe_bmp[BF_PIPE_COUNT];
  scope_num_t scope_id;

  ret = pipe_mgr_tbl_get_symmetric_mode(dev_tgt.device_id,
                                        mat_tbl_info->handle,
                                        &symmetric,
                                        &num_scopes,
                                        scope_pipe_bmp);
  if (ret != PIPE_SUCCESS) {
    LOG_TRACE("Error in get symmetric mode for tbl 0x%x device id %d err %s",
              mat_tbl_info->handle,
              dev_tgt.device_id,
              pipe_str_err(ret));
    return ret;
  }

  if (symmetric) {
    ret = pipe_mgr_mat_tbl_key_exists(
        mat_tbl_info, match_spec, BF_DEV_PIPE_ALL, exists, mat_ent_hdl);
    if (ret != PIPE_SUCCESS) {
      LOG_TRACE(
          "Error in checking if the match spec exists for symmetric tbl "
          " 0x%x device id %d err %s",
          mat_tbl_info->handle,
          dev_tgt.device_id,
          pipe_str_err(ret));
    }
    return ret;
  }
  if (dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL) {
    LOG_TRACE(
        "%s:%d PIPE_ALL not allowed for nonsymmetric tbl 0x%x device_id [%d]",
        __func__,
        __LINE__,
        mat_tbl_info->handle,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  ret = PIPE_OBJ_NOT_FOUND;
  for (scope_id = 0; scope_id < num_scopes; scope_id++) {
    if ((scope_pipe_bmp[scope_id] & (1 << dev_tgt.dev_pipe_id)) == 0) continue;
    for (q = 0; q < PIPE_MGR_MAX_PIPES; q++) {
      if ((scope_pipe_bmp[scope_id] & (1 << q)) == 0) continue;
      /* This is the first pipe in the scope. */
      ret = pipe_mgr_mat_tbl_key_exists(
          mat_tbl_info, match_spec, q, exists, mat_ent_hdl);
      if (ret != PIPE_SUCCESS)
        LOG_TRACE(
            "Error in checking if the match spec exists for tbl 0x%x "
            "device id %d pipe %x err %s",
            mat_tbl_info->handle,
            dev_tgt.device_id,
            q,
            pipe_str_err(ret));
      return ret;
    }
    return ret;
  }
  return ret;
}

/* Function to convert an entry's match spec to its entry handle */
static pipe_status_t pipe_mgr_match_spec_to_ent_hdl_int(
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_match_spec_t *match_spec,
    pipe_mat_ent_hdl_t *mat_ent_hdl,
    bool light_pipe_validation) {
  pipe_status_t ret = PIPE_SUCCESS;
  enum pipe_mgr_table_owner_t owner;
  owner = pipe_mgr_sm_tbl_owner(dev_tgt.device_id, mat_tbl_hdl);

  if (pipe_mgr_match_spec_exists(match_spec)) {
    if (PIPE_MGR_TBL_OWNER_PHASE0 == owner) {
      ret = pipe_mgr_phase0_get_ent_hdl(
          dev_tgt.device_id, mat_tbl_hdl, match_spec, mat_ent_hdl);
      if (ret != PIPE_SUCCESS) {
        LOG_ERROR(
            "Error converting match spec to entry hdl for phase0 tbl 0x%x"
            " device id %d, err %s",
            mat_tbl_hdl,
            dev_tgt.device_id,
            pipe_str_err(ret));
        return ret;
      }
    } else {
      pipe_mat_tbl_info_t *mat_tbl_info = pipe_mgr_get_tbl_info(
          dev_tgt.device_id, mat_tbl_hdl, __func__, __LINE__);
      if (mat_tbl_info == NULL) {
        LOG_ERROR(
            "Error in finding the table info for tbl 0x%x"
            " device id %d",
            mat_tbl_hdl,
            dev_tgt.device_id);
        return PIPE_OBJ_NOT_FOUND;
      }
      if (!mat_tbl_info->duplicate_entry_check) {
        LOG_ERROR(
            "Error: using the match spec API without activating duplicate"
            "entry check for table 0x%x device id %d ",
            mat_tbl_hdl,
            dev_tgt.device_id);
        return PIPE_NOT_SUPPORTED;
      }
      ret =
          pipe_mgr_verify_pipe_id(dev_tgt, mat_tbl_info, light_pipe_validation);
      if (ret != PIPE_SUCCESS) {
        return ret;
      }
      if ((mat_tbl_info->match_type == LONGEST_PREFIX_MATCH ||
           mat_tbl_info->match_type == ALPM_MATCH) &&
          match_spec->priority > match_spec->num_valid_match_bits) {
        LOG_ERROR(
            "Error: Prefix length for lpm field is larger than field"
            "for table 0x%x device id %d",
            mat_tbl_hdl,
            dev_tgt.device_id);
        return PIPE_INVALID_ARG;
      }

      bool exists;
      // Apply the mask if defined, before looking up the key
      if (pipe_mgr_check_table_global_key_mask(mat_tbl_info, match_spec))
        return PIPE_INVALID_ARG;
      if (light_pipe_validation) {
        ret = mat_tbl_key_exists_any_pipe(
            dev_tgt, mat_tbl_info, match_spec, &exists, mat_ent_hdl);
      } else {
        ret = pipe_mgr_mat_tbl_key_exists(mat_tbl_info,
                                          match_spec,
                                          dev_tgt.dev_pipe_id,
                                          &exists,
                                          mat_ent_hdl);
      }
      if (ret != PIPE_SUCCESS) {
        LOG_ERROR(
            "Error in checking if the match spec exists for tbl 0x%x"
            " device id %d pipe %x err %s",
            mat_tbl_hdl,
            dev_tgt.device_id,
            dev_tgt.dev_pipe_id,
            pipe_str_err(ret));
        return ret;
      }
      if (!exists) {
        LOG_TRACE(
            "Error: match spec does not exist for tbl 0x%x device %d pipe %x",
            mat_tbl_hdl,
            dev_tgt.device_id,
            dev_tgt.dev_pipe_id);
        pipe_mgr_entry_format_log_match_spec(dev_tgt.device_id,
                                             BF_LOG_INFO,
                                             mat_tbl_info->profile_id,
                                             mat_tbl_hdl,
                                             match_spec);
        return PIPE_OBJ_NOT_FOUND;
      }
    }
  }

  return ret;
}

pipe_status_t pipe_mgr_match_spec_to_ent_hdl(pipe_sess_hdl_t sess_hdl,
                                             dev_target_t dev_tgt,
                                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                             pipe_tbl_match_spec_t *match_spec,
                                             pipe_mat_ent_hdl_t *mat_ent_hdl,
                                             bool light_pipe_validation) {
  pipe_status_t ret = pipe_mgr_api_enter(sess_hdl);
  if (PIPE_SUCCESS != ret) {
    return ret;
  }
  if (PIPE_SUCCESS != (ret = pipe_mgr_verify_pipe_tbl_access(
                           sess_hdl, dev_tgt, mat_tbl_hdl, true))) {
    pipe_mgr_api_exit(sess_hdl);
    return ret;
  }

  ret = pipe_mgr_match_spec_to_ent_hdl_int(
      dev_tgt, mat_tbl_hdl, match_spec, mat_ent_hdl, light_pipe_validation);

  if (!pipe_mgr_sess_in_batch(sess_hdl) && !pipe_mgr_sess_in_txn(sess_hdl)) {
    pipe_mgr_sm_release(sess_hdl);
  }
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t pipe_mgr_ent_hdl_to_match_spec(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    bf_dev_pipe_t *ent_pipe_id,
    pipe_tbl_match_spec_t const **match_spec) {
  pipe_status_t ret = pipe_mgr_api_enter(sess_hdl);
  if (PIPE_SUCCESS != ret) {
    return ret;
  }

  if (!validate_pipe_ids(dev_tgt, mat_tbl_hdl, mat_ent_hdl)) {
    LOG_ERROR(
        "Pipe id mismatch for tbl 0x%x, entry 0x%x (dev_tgt.dev_pipe_id %d)"
        "for device id %d",
        mat_tbl_hdl,
        mat_ent_hdl,
        dev_tgt.dev_pipe_id,
        dev_tgt.device_id);
    pipe_mgr_api_exit(sess_hdl);
    return PIPE_INVALID_ARG;
  }

  if (PIPE_SUCCESS != (ret = pipe_mgr_verify_pipe_tbl_access(
                           sess_hdl, dev_tgt, mat_tbl_hdl, true))) {
    pipe_mgr_api_exit(sess_hdl);
    return ret;
  }

  ret = pipe_mgr_ent_hdl_to_match_spec_internal(
      dev_tgt.device_id, mat_tbl_hdl, mat_ent_hdl, ent_pipe_id, match_spec);

  if (!pipe_mgr_sess_in_batch(sess_hdl) && !pipe_mgr_sess_in_txn(sess_hdl)) {
    pipe_mgr_sm_release(sess_hdl);
  }
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t pipe_mgr_match_key_mask_spec_set(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_match_spec_t *match_spec) {
  RMT_API(sess_hdl,
          0,
          pipe_mgr_verify_tbl_access(sess_hdl, device_id, mat_tbl_hdl, true),
          pipe_mgr_dkm_set(sess_hdl, device_id, mat_tbl_hdl, match_spec));
}

pipe_status_t pipe_mgr_match_key_mask_spec_reset(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl) {
  RMT_API(sess_hdl,
          0,
          pipe_mgr_verify_tbl_access(sess_hdl, device_id, mat_tbl_hdl, true),
          pipe_mgr_dkm_reset(sess_hdl, device_id, mat_tbl_hdl));
}

pipe_status_t pipe_mgr_match_key_mask_spec_get(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_match_spec_t *match_spec) {
  RMT_API(sess_hdl,
          0,
          pipe_mgr_verify_tbl_access(sess_hdl, device_id, mat_tbl_hdl, true),
          pipe_mgr_dkm_get(sess_hdl, device_id, mat_tbl_hdl, match_spec));
}

pipe_status_t pipe_mgr_mat_tbl_update_action(pipe_sess_hdl_t sess_hdl,
                                             dev_target_t dev_tgt,
                                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                             pipe_mat_ent_hdl_t mat_ent_hdl,
                                             pipe_mgr_move_list_t **move_list) {
  /* Determines if function was called as part of selector resize.
   * If so, then move list will be handled at the end by selector API fin. */
  bool is_sel_call = true;
  pipe_mgr_move_list_t *mlist = NULL;
  pipe_status_t ret;

  if (!validate_pipe_ids(dev_tgt, mat_tbl_hdl, mat_ent_hdl)) {
    LOG_ERROR(
        "Pipe id mismatch for tbl 0x%x, entry 0x%x (dev_tgt.dev_pipe_id %d)"
        "for device id %d",
        mat_tbl_hdl,
        mat_ent_hdl,
        dev_tgt.dev_pipe_id,
        dev_tgt.device_id);
    pipe_mgr_api_exit(sess_hdl);
    return PIPE_INVALID_ARG;
  }

  if (move_list == NULL) {
    is_sel_call = false;
    ret = ml_api_prologue_v2(sess_hdl, dev_tgt, mat_tbl_hdl);
    if (PIPE_SUCCESS != ret) return ret;
    move_list = &mlist;
  } else {
    // Lock the table
    if (PIPE_SUCCESS != (ret = pipe_mgr_verify_pipe_tbl_access(
                             sess_hdl, dev_tgt, mat_tbl_hdl, true))) {
      return ret;
    }
  }

  /* Prepare flags for the table managers */
  uint32_t flags = pipe_mgr_sess_in_txn(sess_hdl) ? PIPE_MGR_TBL_API_TXN : 0;
  flags |= pipe_mgr_sess_in_atomic_txn(sess_hdl) ? PIPE_MGR_TBL_API_ATOM : 0;

  /* Determine table owner. */
  enum pipe_mgr_table_owner_t owner;
  owner = pipe_mgr_sm_tbl_owner(dev_tgt.device_id, mat_tbl_hdl);

  pipe_mat_tbl_info_t *mat_tbl_info =
      pipe_mgr_get_tbl_info(dev_tgt.device_id, mat_tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    LOG_ERROR(
        "Error in finding the table info for tbl 0x%x"
        " device id %d",
        mat_tbl_hdl,
        dev_tgt.device_id);
    ret = PIPE_OBJ_NOT_FOUND;
    goto done;
  }

  if (PIPE_MGR_TBL_OWNER_EXM == owner) {
    ret = pipe_mgr_exm_tbl_update(
        dev_tgt, mat_tbl_hdl, mat_ent_hdl, flags, move_list);
  } else if (PIPE_MGR_TBL_OWNER_TRN == owner) {
    ret = pipe_mgr_tcam_tbl_update(
        dev_tgt, mat_tbl_hdl, mat_ent_hdl, flags, move_list);
  } else {
    LOG_ERROR("No handler for table owner %u, session %u, table %u at %s:%d",
              owner,
              sess_hdl,
              mat_tbl_hdl,
              __func__,
              __LINE__);
    PIPE_MGR_DBGCHK(0);
  }

done:
  if (!is_sel_call) {
    ret = ml_api_fin(
        sess_hdl, ret, flags, dev_tgt.device_id, mat_tbl_hdl, *move_list);
  }
  return ret;
}

pipe_status_t pipe_mgr_mat_ent_add(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_match_spec_t *match_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *act_spec,
    uint32_t ttl, /*< TTL value in msecs, 0 for disable */
    uint32_t pipe_api_flags,
    pipe_mat_ent_hdl_t *ent_hdl_p) {
  pipe_status_t ret = ml_api_prologue_v2(sess_hdl, dev_tgt, mat_tbl_hdl);
  if (PIPE_SUCCESS != ret) return ret;

  pipe_mgr_move_list_t *move_list = NULL;
  /* Determine table owner. */
  enum pipe_mgr_table_owner_t owner;
  owner = pipe_mgr_sm_tbl_owner(dev_tgt.device_id, mat_tbl_hdl);

  pipe_mat_tbl_info_t *mat_tbl_info =
      pipe_mgr_get_tbl_info(dev_tgt.device_id, mat_tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    LOG_ERROR(
        "Error in finding the table info for tbl 0x%x"
        " device id %d",
        mat_tbl_hdl,
        dev_tgt.device_id);
    ret = PIPE_OBJ_NOT_FOUND;
    goto done;
  }
  if (mat_tbl_info->keyless_info) {
    LOG_ERROR(
        "%s:%d Entry add api is not supported for keyless table 0x%x. Use the "
        "set_default api instead",
        __func__,
        __LINE__,
        mat_tbl_hdl);
    ret = PIPE_NOT_SUPPORTED;
    goto done;
  }

  ret = pipe_mgr_verify_pipe_id(
      dev_tgt, mat_tbl_info, false /* light_pipe_validation */);
  if (ret != PIPE_SUCCESS) {
    goto done;
  }
  if ((mat_tbl_info->match_type == LONGEST_PREFIX_MATCH ||
       mat_tbl_info->match_type == ALPM_MATCH) &&
      match_spec->priority > match_spec->num_valid_match_bits) {
    LOG_ERROR(
        "Error: Prefix length for lpm field is larger than field"
        " for table 0x%x device id %d",
        mat_tbl_hdl,
        dev_tgt.device_id);
    ret = PIPE_INVALID_ARG;
    goto done;
  }
  if (IS_ACTION_SPEC_SEL_GRP(act_spec) && act_fn_hdl == 0) {
    /* The action of a selector group is defined by the group members. An
     * action function hdl of zero means this selector group is currently empty.
     */
    LOG_ERROR(
        "Error: Cannot add a match entry that references empty selector "
        "group %d for table 0x%x device id %d",
        act_spec->sel_grp_hdl,
        mat_tbl_hdl,
        dev_tgt.device_id);
    ret = PIPE_INVALID_ARG;
    goto done;
  }
  /* Perform a sanity check if a ttl is given */
  if (ttl) {
    pipe_idle_time_params_t params = {0};
    ret = rmt_idle_params_get(dev_tgt.device_id, mat_tbl_hdl, &params);
    if (ret != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Failed to get idletime parameters for match table 0x%x "
          "device %d",
          __func__,
          __LINE__,
          mat_tbl_hdl,
          dev_tgt.device_id);
      goto done;
    }
    /* Skip the idletime disable check for virtual devices, as it may be only
     * enabled on the physical device
     */
    if (params.mode != INVALID_MODE ||
        !pipe_mgr_is_device_virtual(dev_tgt.device_id)) {
      /* In poll mode, the only allowed value of ttl is 1. This is because it
       * serves as indicator for default entry state (active vs idle). */
      if ((params.mode == POLL_MODE && ttl != 1) ||
          (params.mode == NOTIFY_MODE &&
           ttl < params.u.notify.ttl_query_interval)) {
        LOG_ERROR(
            "%s:%d Cannot add entry with ttl %d to match table 0x%x device %d "
            "in %s mode, %s %d",
            __func__,
            __LINE__,
            ttl,
            mat_tbl_hdl,
            dev_tgt.device_id,
            idle_time_mode_to_str(params.mode),
            params.mode == POLL_MODE ? "poll-mode can only use TTL of 0 or"
                                     : "TTL must be at least",
            params.mode == POLL_MODE ? 1 : params.u.notify.ttl_query_interval);
        ret = PIPE_INVALID_ARG;
        goto done;
      }
    }
  }

  /* Prepare flags for the table managers */
  uint32_t flags = pipe_mgr_sess_in_txn(sess_hdl) ? PIPE_MGR_TBL_API_TXN : 0;
  flags |= pipe_mgr_sess_in_atomic_txn(sess_hdl) ? PIPE_MGR_TBL_API_ATOM : 0;
  bool exists = false;
  pipe_mat_ent_hdl_t ent_hdl;
  /* Check if the key(match_spec) already exists in the table */
  /* Only do this if a valid match spec exists for the table
     and the owner is not phase0 */
  if (pipe_mgr_match_spec_exists(match_spec) &&
      (PIPE_MGR_TBL_OWNER_PHASE0 != owner)) {
    if (pipe_mgr_check_table_global_key_mask(mat_tbl_info, match_spec)) {
      ret = PIPE_INVALID_ARG;
      goto done;
    }
    ret = pipe_mgr_mat_tbl_key_exists(
        mat_tbl_info, match_spec, dev_tgt.dev_pipe_id, &exists, &ent_hdl);
    if (ret != PIPE_SUCCESS) {
      LOG_ERROR(
          "Error in checking if the match spec already exists for tbl 0x%x"
          " device id %d, err %s",
          mat_tbl_hdl,
          dev_tgt.device_id,
          pipe_str_err(ret));
      goto done;
    }
  }

  if (exists == true) {
    LOG_TRACE(
        "Match spec already exists for tbl 0x%x, device id %d, entry handle %d",
        mat_tbl_hdl,
        dev_tgt.device_id,
        ent_hdl);

    /* Pretty print the duplicate match spec */
    char buf[1000];
    if (PIPE_MGR_TBL_OWNER_EXM == owner) {
      pipe_mgr_exm_print_match_spec(
          dev_tgt.device_id, mat_tbl_hdl, match_spec, buf, sizeof(buf));
      LOG_TRACE("Duplicate match spec for tbl 0x%x :\n%s", mat_tbl_hdl, buf);
    } else if (PIPE_MGR_TBL_OWNER_TRN == owner) {
      pipe_mgr_tcam_print_match_spec(
          dev_tgt.device_id, mat_tbl_hdl, match_spec, buf, sizeof(buf));
      LOG_TRACE("Duplicate match spec for tbl 0x%x :\n%s", mat_tbl_hdl, buf);
    } else if (PIPE_MGR_TBL_OWNER_ALPM == owner) {
      pipe_mgr_alpm_print_match_spec(
          dev_tgt.device_id, mat_tbl_hdl, match_spec, buf, sizeof(buf));
      LOG_TRACE("Duplicate match spec for tbl 0x%x :\n%s", mat_tbl_hdl, buf);
    }
    ret = PIPE_ALREADY_EXISTS;
    goto done;
  }
  if (PIPE_MGR_TBL_OWNER_EXM == owner) {
    ret = pipe_mgr_exm_ent_place(dev_tgt,
                                 mat_tbl_hdl,
                                 match_spec,
                                 act_fn_hdl,
                                 act_spec,
                                 ttl,
                                 flags,
                                 ent_hdl_p,
                                 &move_list);
  } else if (PIPE_MGR_TBL_OWNER_TRN == owner) {
    ret = pipe_mgr_tcam_entry_place(dev_tgt,
                                    mat_tbl_hdl,
                                    match_spec,
                                    act_fn_hdl,
                                    act_spec,
                                    ttl,
                                    flags,
                                    ent_hdl_p,
                                    &move_list);
  } else if (PIPE_MGR_TBL_OWNER_ALPM == owner) {
    ret = pipe_mgr_alpm_entry_place(dev_tgt,
                                    mat_tbl_hdl,
                                    match_spec,
                                    act_fn_hdl,
                                    act_spec,
                                    ttl,
                                    flags,
                                    ent_hdl_p,
                                    &move_list);
  } else if (PIPE_MGR_TBL_OWNER_PHASE0 == owner) {
    ret = pipe_mgr_phase0_ent_place(sess_hdl,
                                    dev_tgt,
                                    mat_tbl_hdl,
                                    match_spec,
                                    act_fn_hdl,
                                    act_spec,
                                    ent_hdl_p,
                                    &move_list);
  } else if (PIPE_MGR_TBL_OWNER_NO_KEY == owner) {
  } else {
    LOG_ERROR("No handler for table owner %u, session %u, table %u at %s:%d",
              owner,
              sess_hdl,
              mat_tbl_hdl,
              __func__,
              __LINE__);
    PIPE_MGR_DBGCHK(0);
  }
  if ((ret == PIPE_SUCCESS) && (PIPE_MGR_TBL_OWNER_PHASE0 != owner)) {
    /* Match entry add went through successfully, add the match spec to the
     * hash table containing the keys for this match table.
     */
    if (pipe_mgr_match_spec_exists(match_spec)) {
      ret = pipe_mgr_mat_tbl_key_insert(dev_tgt.device_id,
                                        mat_tbl_info,
                                        match_spec,
                                        *ent_hdl_p,
                                        dev_tgt.dev_pipe_id,
                                        pipe_mgr_sess_in_txn(sess_hdl));
      if (ret != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in inserting key for tbl 0x%x, device id %d"
            " into key-based hash table",
            __func__,
            __LINE__,
            mat_tbl_hdl,
            dev_tgt.device_id);
      }
    }
  }
done:
  ret = ml_api_fin(
      sess_hdl, ret, pipe_api_flags, dev_tgt.device_id, mat_tbl_hdl, move_list);
  return ret;
}

static bool compare_action_data(dev_target_t dev_tgt,
                                pipe_mat_tbl_info_t *mat_tbl_info,
                                pipe_action_spec_t *act_spec1,
                                pipe_action_spec_t *act_spec2) {
  bool okay = true;

  /* Sanity check */
  if (!mat_tbl_info || !act_spec1 || !act_spec2) return false;

  if (act_spec1->act_data.num_action_data_bytes) {
    okay = act_spec1->act_data.num_action_data_bytes ==
           act_spec2->act_data.num_action_data_bytes;
    if (okay) {
      okay = act_spec1->act_data.action_data_bits &&
             act_spec2->act_data.action_data_bits &&
             0 == PIPE_MGR_MEMCMP(act_spec1->act_data.action_data_bits,
                                  act_spec2->act_data.action_data_bits,
                                  act_spec1->act_data.num_action_data_bytes);
    }
  }

  if (!okay) {
    LOG_ERROR(
        "Dev %d Table %s, 0x%x, cannot set action as the requested action "
        "parameters do not match the constant values specified by the P4 "
        "program",
        dev_tgt.device_id,
        mat_tbl_info->name,
        mat_tbl_info->handle);
  }

  return okay;
}

static pipe_status_t pipe_mgr_mat_ent_set_action_internal(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *act_spec,
    pipe_mgr_move_list_t **move_list) {
  pipe_status_t ret = PIPE_SUCCESS;
  bf_dev_id_t dev_id = dev_tgt.device_id;
  pipe_action_spec_t *static_action_spec = NULL;
  pipe_mat_tbl_info_t *mat_tbl_info =
      pipe_mgr_get_tbl_info(dev_id, mat_tbl_hdl, __func__, __LINE__);
  if (!mat_tbl_info) {
    LOG_ERROR("Error in finding the table info for tbl 0x%x device id %d",
              mat_tbl_hdl,
              dev_id);
    ret = PIPE_OBJ_NOT_FOUND;
    goto done;
  }

  if (mat_tbl_info->static_entries) {
    /* The table has constant entries.  A set is only permitted when both
     * the action function handle and action parameters match the constant
     * default values specified in the P4 program. */
    pipe_tbl_match_spec_t *match_spec = NULL;
    pipe_act_fn_hdl_t action_fn_hdl;
    dev_target_t tmp_dev_tgt = {dev_id, dev_tgt.dev_pipe_id};
    if (mat_tbl_info->symmetric) tmp_dev_tgt.dev_pipe_id = BF_DEV_PIPE_ALL;

    static_action_spec = pipe_mgr_tbl_alloc_action_spec(
        act_spec->act_data.num_action_data_bytes);
    if (!static_action_spec) {
      ret = PIPE_NO_SYS_RESOURCES;
      goto done;
    }

    ret = pipe_mgr_get_entry(sess_hdl,
                             mat_tbl_hdl,
                             tmp_dev_tgt,
                             mat_ent_hdl,
                             match_spec,
                             static_action_spec,
                             &action_fn_hdl,
                             false,
                             PIPE_RES_GET_FLAG_ENTRY,
                             NULL);
    if (ret != PIPE_SUCCESS) {
      goto done;
    }

    if (action_fn_hdl == act_fn_hdl) {
      if (!compare_action_data(
              dev_tgt, mat_tbl_info, static_action_spec, act_spec)) {
        /* Invalid action parameters. */
        ret = PIPE_NOT_SUPPORTED;
        goto done;
      }
    } else {
      /* Cannot change action function handle since it is a static entry. */
      ret = PIPE_NOT_SUPPORTED;
      goto done;
    }
  }

  /* Determine table owner. */
  enum pipe_mgr_table_owner_t owner;
  owner = pipe_mgr_sm_tbl_owner(dev_id, mat_tbl_hdl);

  /* Prepare flags for the table managers. */
  uint32_t flags = pipe_mgr_sess_in_txn(sess_hdl) ? PIPE_MGR_TBL_API_TXN : 0;
  flags |= pipe_mgr_sess_in_atomic_txn(sess_hdl) ? PIPE_MGR_TBL_API_ATOM : 0;

  /* Call the modify function for the appropriate table manager. */
  if (PIPE_MGR_TBL_OWNER_EXM == owner) {
    ret = pipe_mgr_exm_ent_set_action(dev_id,
                                      mat_tbl_hdl,
                                      mat_ent_hdl,
                                      act_fn_hdl,
                                      act_spec,
                                      flags,
                                      move_list);
  } else if (PIPE_MGR_TBL_OWNER_TRN == owner) {
    ret = pipe_mgr_tcam_ent_set_action(dev_id,
                                       mat_tbl_hdl,
                                       mat_ent_hdl,
                                       act_fn_hdl,
                                       act_spec,
                                       flags,
                                       move_list);
  } else if (PIPE_MGR_TBL_OWNER_ALPM == owner) {
    ret = pipe_mgr_alpm_ent_set_action(dev_id,
                                       mat_tbl_hdl,
                                       mat_ent_hdl,
                                       act_fn_hdl,
                                       act_spec,
                                       flags,
                                       move_list);
  } else if (PIPE_MGR_TBL_OWNER_PHASE0 == owner) {
    ret = pipe_mgr_phase0_ent_set_action(sess_hdl,
                                         dev_id,
                                         mat_tbl_hdl,
                                         mat_ent_hdl,
                                         act_fn_hdl,
                                         act_spec,
                                         move_list);
  } else if (PIPE_MGR_TBL_OWNER_NO_KEY == owner) {
  } else {
    LOG_ERROR("No handler for table owner %u, session %u, table %u at %s:%d",
              owner,
              sess_hdl,
              mat_tbl_hdl,
              __func__,
              __LINE__);
    PIPE_MGR_DBGCHK(0);
    ret = PIPE_UNEXPECTED;
  }

done:
  /* Free temporary allocated action spec if needed. */
  if (static_action_spec) {
    if (static_action_spec->act_data.action_data_bits)
      PIPE_MGR_FREE(static_action_spec->act_data.action_data_bits);
    PIPE_MGR_FREE(static_action_spec);
  }
  return ret;
}

pipe_status_t pipe_mgr_mat_ent_add_or_mod(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_match_spec_t *match_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *act_spec,
    uint32_t ttl, /*< TTL value in msecs, 0 for disable */
    uint32_t pipe_api_flags,
    pipe_mat_ent_hdl_t *ent_hdl_p,
    bool *is_added) {
  pipe_status_t ret = ml_api_prologue_v2(sess_hdl, dev_tgt, mat_tbl_hdl);
  if (PIPE_SUCCESS != ret) return ret;
  pipe_mgr_move_list_t *move_list = NULL;
  /* Determine table owner. */
  enum pipe_mgr_table_owner_t owner;
  owner = pipe_mgr_sm_tbl_owner(dev_tgt.device_id, mat_tbl_hdl);

  pipe_mat_tbl_info_t *mat_tbl_info =
      pipe_mgr_get_tbl_info(dev_tgt.device_id, mat_tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    LOG_ERROR(
        "Error in finding the table info for tbl 0x%x"
        " device id %d",
        mat_tbl_hdl,
        dev_tgt.device_id);
    ret = PIPE_OBJ_NOT_FOUND;
    goto done;
  }
  if (mat_tbl_info->keyless_info) {
    LOG_ERROR(
        "%s:%d Entry add api is not supported for keyless table 0x%x. Use the "
        "set_default api instead",
        __func__,
        __LINE__,
        mat_tbl_hdl);
    ret = PIPE_NOT_SUPPORTED;
    goto done;
  }

  ret = pipe_mgr_verify_pipe_id(
      dev_tgt, mat_tbl_info, false /* light_pipe_validation */);
  if (ret != PIPE_SUCCESS) {
    goto done;
  }
  if ((mat_tbl_info->match_type == LONGEST_PREFIX_MATCH ||
       mat_tbl_info->match_type == ALPM_MATCH) &&
      match_spec->priority > match_spec->num_valid_match_bits) {
    LOG_ERROR(
        "Error: Prefix length for lpm field is larger than field"
        " for table 0x%x device id %d",
        mat_tbl_hdl,
        dev_tgt.device_id);
    ret = PIPE_INVALID_ARG;
    goto done;
  }
  if (IS_ACTION_SPEC_SEL_GRP(act_spec) && act_fn_hdl == 0) {
    /* The action of a selector group is defined by the group members. An
     * action function hdl of zero means this selector group is currently empty.
     */
    LOG_ERROR(
        "Error: Cannot add a match entry that references empty selector "
        "group %d for table 0x%x device id %d",
        act_spec->sel_grp_hdl,
        mat_tbl_hdl,
        dev_tgt.device_id);
    ret = PIPE_INVALID_ARG;
    goto done;
  }
  /* Perform a sanity check if a ttl is given */
  if (ttl) {
    pipe_idle_time_params_t params = {0};
    ret = rmt_idle_params_get(dev_tgt.device_id, mat_tbl_hdl, &params);
    if (ret != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Failed to get idletime parameters for match table 0x%x "
          "device %d",
          __func__,
          __LINE__,
          mat_tbl_hdl,
          dev_tgt.device_id);
      goto done;
    }
    /* Skip the idletime disable check for virtual devices, as it may be only
     * enabled on the physical device
     */
    if (params.mode != INVALID_MODE ||
        !pipe_mgr_is_device_virtual(dev_tgt.device_id)) {
      /* In poll mode, the only allowed value of ttl is 1. This is because it
       * serves as indicator for default entry state (active vs idle). */
      if ((params.mode == POLL_MODE && ttl != 1) ||
          (params.mode == NOTIFY_MODE &&
           ttl < params.u.notify.ttl_query_interval)) {
        LOG_ERROR(
            "%s:%d Cannot add entry with ttl %d to match table 0x%x device %d "
            "in %s mode, %s %d",
            __func__,
            __LINE__,
            ttl,
            mat_tbl_hdl,
            dev_tgt.device_id,
            idle_time_mode_to_str(params.mode),
            params.mode == POLL_MODE ? "poll-mode can only use TTL of 0 or"
                                     : "TTL must be at least",
            params.mode == POLL_MODE ? 1 : params.u.notify.ttl_query_interval);
        ret = PIPE_INVALID_ARG;
        goto done;
      }
    }
  }

  /* Prepare flags for the table managers */
  uint32_t flags = pipe_mgr_sess_in_txn(sess_hdl) ? PIPE_MGR_TBL_API_TXN : 0;
  flags |= pipe_mgr_sess_in_atomic_txn(sess_hdl) ? PIPE_MGR_TBL_API_ATOM : 0;
  bool exists = false;
  pipe_mat_ent_hdl_t ent_hdl;
  /* Check if the key(match_spec) already exists in the table */
  /* Only do this if a valid match spec exists for the table
     and the owner is not phase0 */
  if (pipe_mgr_match_spec_exists(match_spec) &&
      (PIPE_MGR_TBL_OWNER_PHASE0 != owner)) {
    if (pipe_mgr_check_table_global_key_mask(mat_tbl_info, match_spec)) {
      ret = PIPE_INVALID_ARG;
      goto done;
    }
    ret = pipe_mgr_mat_tbl_key_exists(
        mat_tbl_info, match_spec, dev_tgt.dev_pipe_id, &exists, &ent_hdl);
    if (ret != PIPE_SUCCESS) {
      LOG_ERROR(
          "Error in checking if the match spec already exists for tbl 0x%x"
          " device id %d, err %s",
          mat_tbl_hdl,
          dev_tgt.device_id,
          pipe_str_err(ret));
      goto done;
    }
  }

  if (is_added) *is_added = !exists;

  if (exists) {
    LOG_TRACE(
        "Match spec already exists for tbl 0x%x, device id %d, entry handle %d",
        mat_tbl_hdl,
        dev_tgt.device_id,
        ent_hdl);

    ret = pipe_mgr_mat_ent_set_action_internal(sess_hdl,
                                               dev_tgt,
                                               mat_tbl_hdl,
                                               ent_hdl,
                                               act_fn_hdl,
                                               act_spec,
                                               &move_list);
    if (ret != PIPE_SUCCESS) {
      LOG_ERROR("Error setting action for tbl 0x%x device id %d",
                mat_tbl_hdl,
                dev_tgt.device_id);
    }

    goto done;
  }
  if (PIPE_MGR_TBL_OWNER_EXM == owner) {
    ret = pipe_mgr_exm_ent_place(dev_tgt,
                                 mat_tbl_hdl,
                                 match_spec,
                                 act_fn_hdl,
                                 act_spec,
                                 ttl,
                                 flags,
                                 ent_hdl_p,
                                 &move_list);
  } else if (PIPE_MGR_TBL_OWNER_TRN == owner) {
    ret = pipe_mgr_tcam_entry_place(dev_tgt,
                                    mat_tbl_hdl,
                                    match_spec,
                                    act_fn_hdl,
                                    act_spec,
                                    ttl,
                                    flags,
                                    ent_hdl_p,
                                    &move_list);
  } else if (PIPE_MGR_TBL_OWNER_ALPM == owner) {
    ret = pipe_mgr_alpm_entry_place(dev_tgt,
                                    mat_tbl_hdl,
                                    match_spec,
                                    act_fn_hdl,
                                    act_spec,
                                    ttl,
                                    flags,
                                    ent_hdl_p,
                                    &move_list);
  } else if (PIPE_MGR_TBL_OWNER_PHASE0 == owner) {
    ret = pipe_mgr_phase0_ent_place(sess_hdl,
                                    dev_tgt,
                                    mat_tbl_hdl,
                                    match_spec,
                                    act_fn_hdl,
                                    act_spec,
                                    ent_hdl_p,
                                    &move_list);
  } else if (PIPE_MGR_TBL_OWNER_NO_KEY == owner) {
  } else {
    LOG_ERROR("No handler for table owner %u, session %u, table %u at %s:%d",
              owner,
              sess_hdl,
              mat_tbl_hdl,
              __func__,
              __LINE__);
    PIPE_MGR_DBGCHK(0);
  }
  if ((ret == PIPE_SUCCESS) && (PIPE_MGR_TBL_OWNER_PHASE0 != owner)) {
    /* Match entry add went through successfully, add the match spec to the
     * hash table containing the keys for this match table.
     */
    if (pipe_mgr_match_spec_exists(match_spec)) {
      ret = pipe_mgr_mat_tbl_key_insert(dev_tgt.device_id,
                                        mat_tbl_info,
                                        match_spec,
                                        *ent_hdl_p,
                                        dev_tgt.dev_pipe_id,
                                        pipe_mgr_sess_in_txn(sess_hdl));
      if (ret != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in inserting key for tbl 0x%x, device id %d"
            " into key-based hash table",
            __func__,
            __LINE__,
            mat_tbl_hdl,
            dev_tgt.device_id);
      }
    }
  }
done:
  ret = ml_api_fin(
      sess_hdl, ret, pipe_api_flags, dev_tgt.device_id, mat_tbl_hdl, move_list);
  return ret;
}

pipe_status_t pipe_mgr_tbl_get_last_index(dev_target_t dev_tgt,
                                          pipe_mat_tbl_hdl_t tbl_hdl,
                                          uint32_t *last_index) {
  pipe_status_t ret;
  enum pipe_mgr_table_owner_t owner =
      pipe_mgr_sm_tbl_owner(dev_tgt.device_id, tbl_hdl);
  switch (owner) {
    case PIPE_MGR_TBL_OWNER_TRN:
      ret = pipe_mgr_tcam_get_last_index(dev_tgt, tbl_hdl, last_index);
      break;
    case PIPE_MGR_TBL_OWNER_EXM:
      ret = pipe_mgr_exm_get_last_index(dev_tgt, tbl_hdl, last_index);
      break;
    case PIPE_MGR_TBL_OWNER_PHASE0:
      ret = pipe_mgr_phase0_get_last_index(dev_tgt, tbl_hdl, last_index);
      break;
    case PIPE_MGR_TBL_OWNER_ALPM:
    case PIPE_MGR_TBL_OWNER_ADT:
    case PIPE_MGR_TBL_OWNER_SELECT:
    case PIPE_MGR_TBL_OWNER_STAT:
    case PIPE_MGR_TBL_OWNER_METER:
    case PIPE_MGR_TBL_OWNER_STFUL:
    case PIPE_MGR_TBL_OWNER_NO_KEY:
    case PIPE_MGR_TBL_OWNER_NONE:
      LOG_ERROR(
          "%s:%d table type for handle 0x%x, device id %d not supported for HW "
          "index get",
          __func__,
          __LINE__,
          tbl_hdl,
          dev_tgt.device_id);
      ret = PIPE_INVALID_ARG;
      break;
    default:
      LOG_ERROR("%s:%d Unknown table type for handle 0x%x, device id %d",
                __func__,
                __LINE__,
                tbl_hdl,
                dev_tgt.device_id);
      PIPE_MGR_DBGCHK(0);
      ret = PIPE_UNEXPECTED;
  }

  return ret;
}

static pipe_status_t pipe_mgr_tbl_get_entry_from_index_hdlr(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t tbl_hdl,
    uint32_t index,
    bool err_correction,
    pipe_tbl_match_spec_t *match_spec,
    pipe_action_spec_t *act_spec,
    pipe_act_fn_hdl_t *act_fn_hdl,
    pipe_ent_hdl_t *entry_hdl,
    bool *is_default,
    uint32_t *next_index) {
  pipe_status_t ret;
  enum pipe_mgr_table_owner_t owner =
      pipe_mgr_sm_tbl_owner(dev_tgt.device_id, tbl_hdl);
  switch (owner) {
    case PIPE_MGR_TBL_OWNER_TRN:
      ret = pipe_mgr_tcam_tbl_raw_entry_get(sess_hdl,
                                            dev_tgt,
                                            tbl_hdl,
                                            index,
                                            err_correction,
                                            match_spec,
                                            act_spec,
                                            act_fn_hdl,
                                            entry_hdl,
                                            is_default,
                                            next_index);
      break;
    case PIPE_MGR_TBL_OWNER_EXM:
      ret = pipe_mgr_exm_tbl_raw_entry_get(sess_hdl,
                                           dev_tgt,
                                           tbl_hdl,
                                           index,
                                           err_correction,
                                           match_spec,
                                           act_spec,
                                           act_fn_hdl,
                                           entry_hdl,
                                           is_default,
                                           next_index);
      break;
    case PIPE_MGR_TBL_OWNER_PHASE0:
      ret = pipe_mgr_phase0_tbl_raw_entry_get(sess_hdl,
                                              dev_tgt,
                                              tbl_hdl,
                                              index,
                                              err_correction,
                                              match_spec,
                                              act_spec,
                                              act_fn_hdl,
                                              entry_hdl,
                                              is_default,
                                              next_index);
      break;
    case PIPE_MGR_TBL_OWNER_ALPM:
    case PIPE_MGR_TBL_OWNER_ADT:
    case PIPE_MGR_TBL_OWNER_SELECT:
    case PIPE_MGR_TBL_OWNER_STAT:
    case PIPE_MGR_TBL_OWNER_METER:
    case PIPE_MGR_TBL_OWNER_STFUL:
    case PIPE_MGR_TBL_OWNER_NO_KEY:
      LOG_ERROR(
          "%s:%d table type for handle 0x%x, device id %d not supported for HW "
          "index get",
          __func__,
          __LINE__,
          tbl_hdl,
          dev_tgt.device_id);
      ret = PIPE_INVALID_ARG;
      break;
    default:
      LOG_ERROR("%s:%d Unknown table type for handle 0x%x, device id %d",
                __func__,
                __LINE__,
                tbl_hdl,
                dev_tgt.device_id);
      PIPE_MGR_DBGCHK(0);
      ret = PIPE_UNEXPECTED;
  }

  if (ret == PIPE_INTERNAL_ERROR) {
    LOG_ERROR("%s:%d Entry at index 0x%x, pipe %d does not match SW state",
              __func__,
              __LINE__,
              index,
              dev_tgt.dev_pipe_id);
  }

  return ret;
}

pipe_status_t pipe_mgr_tbl_get_entry_from_index(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t tbl_hdl,
    uint32_t index,
    bool err_correction,
    pipe_tbl_match_spec_t *match_spec,
    pipe_action_spec_t *act_spec,
    pipe_act_fn_hdl_t *act_fn_hdl,
    pipe_ent_hdl_t *entry_hdl,
    bool *is_default,
    uint32_t *next_index) {
  pipe_status_t ret;

  /* Take a per session lock to ensure only one thread is accessing our driver
   * on the session at a time.  Note that this also validates the session. */
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }

  if (pipe_mgr_sess_in_txn(sess_hdl)) {
    /* This API cannot be called within a batch or within a transaction because
     * of reading from hardware. */
    LOG_ERROR(
        "%s:%d Get entry from hardware API called within a transaction "
        "is not supported, tbl hdl 0x%x, device id %d, entry index %d",
        __func__,
        __LINE__,
        tbl_hdl,
        dev_tgt.device_id,
        index);
    pipe_mgr_api_exit(sess_hdl);
    return PIPE_NOT_SUPPORTED;
  }
  if (pipe_mgr_sess_in_batch(sess_hdl)) {
    pipe_mgr_flush_batch(sess_hdl);
  }
  complete_operations(sess_hdl);

  /* Verify that the session can access the table. */
  if (PIPE_SUCCESS != (ret = pipe_mgr_verify_pipe_tbl_access(
                           sess_hdl, dev_tgt, tbl_hdl, true))) {
    pipe_mgr_api_exit(sess_hdl);
    return ret;
  }

  if (dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL) {
    pipe_mat_tbl_info_t *match_table =
        pipe_mgr_get_tbl_info(dev_tgt.device_id, tbl_hdl, __func__, __LINE__);
    if (!match_table) {
      ret = PIPE_OBJ_NOT_FOUND;
      goto done;
    }

    if (!match_table->symmetric) {
      ret = PIPE_INVALID_ARG;
      goto done;
    }

    rmt_dev_profile_info_t *profile_info = pipe_mgr_get_profile_info(
        dev_tgt.device_id, match_table->profile_id, __func__, __LINE__);
    if (!profile_info) {
      LOG_ERROR(
          "%s:%d Error, no dev_profile_info found for device_id [%d],"
          " profile_id [%d]",
          __func__,
          __LINE__,
          dev_tgt.device_id,
          match_table->profile_id);
      ret = PIPE_OBJ_NOT_FOUND;
      goto done;
    }
    for (int pipe_id = PIPE_BITMAP_GET_FIRST_SET(&profile_info->pipe_bmp);
         pipe_id != -1;
         pipe_id = PIPE_BITMAP_GET_NEXT_BIT(&profile_info->pipe_bmp, pipe_id)) {
      dev_tgt.dev_pipe_id = pipe_id;
      ret = pipe_mgr_tbl_get_entry_from_index_hdlr(sess_hdl,
                                                   dev_tgt,
                                                   tbl_hdl,
                                                   index,
                                                   err_correction,
                                                   match_spec,
                                                   act_spec,
                                                   act_fn_hdl,
                                                   entry_hdl,
                                                   is_default,
                                                   next_index);
      if (ret != PIPE_SUCCESS && ret != PIPE_INTERNAL_ERROR) {
        break;
      }
    }
  } else {
    ret = pipe_mgr_tbl_get_entry_from_index_hdlr(sess_hdl,
                                                 dev_tgt,
                                                 tbl_hdl,
                                                 index,
                                                 err_correction,
                                                 match_spec,
                                                 act_spec,
                                                 act_fn_hdl,
                                                 entry_hdl,
                                                 is_default,
                                                 next_index);
  }

  if (ret == PIPE_INTERNAL_ERROR) {
    if (pipe_mgr_drv_ilist_push(&sess_hdl, NULL, NULL)) {
      LOG_ERROR("%s:%d Unable to push entry fix for handle 0x%x, device id %d",
                __func__,
                __LINE__,
                tbl_hdl,
                dev_tgt.device_id);
    }
  }

done:
  if (!pipe_mgr_sess_in_batch(sess_hdl)) {
    pipe_mgr_sm_release(sess_hdl);
  }

  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t pipe_mgr_mat_default_entry_set(pipe_sess_hdl_t sess_hdl,
                                             dev_target_t dev_tgt,
                                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                             pipe_act_fn_hdl_t act_fn_hdl,
                                             pipe_action_spec_t *act_spec,
                                             uint32_t pipe_api_flags,
                                             pipe_mat_ent_hdl_t *ent_hdl_p) {
  pipe_mgr_move_list_t *move_list = NULL;
  pipe_status_t ret = ml_api_prologue_v2(sess_hdl, dev_tgt, mat_tbl_hdl);
  if (PIPE_SUCCESS != ret) return ret;

  pipe_mat_tbl_info_t *mat_tbl_info =
      pipe_mgr_get_tbl_info(dev_tgt.device_id, mat_tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    LOG_ERROR("Error in finding the table info for tbl 0x%x device id %d",
              mat_tbl_hdl,
              dev_tgt.device_id);
    ret = PIPE_OBJ_NOT_FOUND;
    goto done;
  }
  uint32_t act_idx;
  for (act_idx = 0; act_idx < mat_tbl_info->def_act_blacklist_size; act_idx++) {
    if (mat_tbl_info->def_act_blacklist[act_idx] == act_fn_hdl) {
      LOG_ERROR(
          "%s:%d Action function %u is not allowed as a default action for "
          "table %s, 0x%x, device id %d.",
          __func__,
          __LINE__,
          act_fn_hdl,
          mat_tbl_info->name,
          mat_tbl_hdl,
          dev_tgt.device_id);
      ret = PIPE_INVALID_ARG;
      goto done;
    }
  }

  if (mat_tbl_info->default_info && mat_tbl_info->default_info->is_const) {
    /* The table has a constant default action.  A set is only permitted when
     * the action function matches the default_info and any action parameters
     * match the default_info. */
    if (mat_tbl_info->default_info->action_entry.act_fn_hdl != act_fn_hdl) {
      LOG_ERROR(
          "Dev %d Table %s, 0x%x, cannot set default action to %u as %u is "
          "declared as a const default action",
          dev_tgt.device_id,
          mat_tbl_info->name,
          mat_tbl_hdl,
          act_fn_hdl,
          mat_tbl_info->default_info->action_entry.act_fn_hdl);
      ret = PIPE_INVALID_ARG;
      goto done;
    }

    /* Only compare the action parameters for Action-Data cases. */
    if (IS_ACTION_SPEC_ACT_DATA(act_spec)) {
      pipe_action_spec_t aspec;
      ret = pipe_mgr_create_action_spec(
          dev_tgt.device_id, &mat_tbl_info->default_info->action_entry, &aspec);
      if (ret != PIPE_SUCCESS) {
        LOG_ERROR(
            "Dev %d table %s, 0x%x, failed to create default action spec: %s",
            dev_tgt.device_id,
            mat_tbl_info->name,
            mat_tbl_hdl,
            pipe_str_err(ret));

        if (aspec.act_data.action_data_bits)
          PIPE_MGR_FREE(aspec.act_data.action_data_bits);
        goto done;
      }

      if (!compare_action_data(dev_tgt, mat_tbl_info, &aspec, act_spec)) {
        /* Invalid action parameters. */
        if (aspec.act_data.action_data_bits)
          PIPE_MGR_FREE(aspec.act_data.action_data_bits);

        ret = PIPE_NOT_SUPPORTED;
        goto done;
      }

      /* Free temporary allocated action spec. */
      if (aspec.act_data.action_data_bits)
        PIPE_MGR_FREE(aspec.act_data.action_data_bits);
    }
  }

  ret = pipe_mgr_place_default_entry(sess_hdl,
                                     dev_tgt,
                                     mat_tbl_info,
                                     act_fn_hdl,
                                     act_spec,
                                     ent_hdl_p,
                                     &move_list);
done:
  ret = ml_api_fin(
      sess_hdl, ret, pipe_api_flags, dev_tgt.device_id, mat_tbl_hdl, move_list);
  return ret;
}

/* Get default entry handle */
pipe_status_t pipe_mgr_table_get_default_entry_handle(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t *ent_hdl_p) {
  pipe_status_t ret = PIPE_SUCCESS;

  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }

  pipe_mat_tbl_info_t *mat_tbl_info =
      pipe_mgr_get_tbl_info(dev_tgt.device_id, mat_tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    LOG_ERROR(
        "Error in finding the table info for tbl 0x%x"
        " device id %d",
        mat_tbl_hdl,
        dev_tgt.device_id);
    pipe_mgr_api_exit(sess_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (PIPE_SUCCESS != (ret = pipe_mgr_verify_pipe_tbl_access(
                           sess_hdl, dev_tgt, mat_tbl_hdl, true))) {
    pipe_mgr_api_exit(sess_hdl);
    return ret;
  }

  /* Determine table owner. */
  enum pipe_mgr_table_owner_t owner;
  owner = pipe_mgr_sm_tbl_owner(dev_tgt.device_id, mat_tbl_hdl);

  /* Call the get function for the appropriate table manager. */
  if (PIPE_MGR_TBL_OWNER_EXM == owner) {
    ret = pipe_mgr_exm_default_ent_hdl_get(dev_tgt, mat_tbl_hdl, ent_hdl_p);
  } else if (PIPE_MGR_TBL_OWNER_TRN == owner) {
    ret = pipe_mgr_tcam_default_ent_hdl_get(dev_tgt, mat_tbl_hdl, ent_hdl_p);
  } else if (PIPE_MGR_TBL_OWNER_ALPM == owner) {
    ret = pipe_mgr_alpm_default_ent_hdl_get(dev_tgt, mat_tbl_hdl, ent_hdl_p);
  } else if (PIPE_MGR_TBL_OWNER_PHASE0 == owner) {
    ret = pipe_mgr_phase0_default_ent_hdl_get(dev_tgt, mat_tbl_hdl, ent_hdl_p);
  } else if (PIPE_MGR_TBL_OWNER_NO_KEY == owner) {
    if (!mat_tbl_info->tbl_no_key_is_default_entry_valid) {
      /* Indicates that the default action was either never set or reset
         after being set */
      ret = PIPE_OBJ_NOT_FOUND;
    } else {
      *ent_hdl_p = PIPE_MGR_TBL_NO_KEY_DEFAULT_ENTRY_HDL;
      ret = PIPE_SUCCESS;
    }
  } else {
    LOG_ERROR("No handler for table owner %u, session %u, table %u at %s:%d",
              owner,
              sess_hdl,
              mat_tbl_hdl,
              __func__,
              __LINE__);
    ret = PIPE_OBJ_NOT_FOUND;
  }
  if (!pipe_mgr_sess_in_batch(sess_hdl) && !pipe_mgr_sess_in_txn(sess_hdl)) {
    pipe_mgr_sm_release(sess_hdl);
  }
  pipe_mgr_api_exit(sess_hdl);

  return ret;
}

pipe_status_t pipe_mgr_get_action_dir_res_usage(bf_dev_id_t dev_id,
                                                pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                                pipe_act_fn_hdl_t act_fn_hdl,
                                                bool *has_dir_stats,
                                                bool *has_dir_meter,
                                                bool *has_dir_lpf,
                                                bool *has_dir_wred,
                                                bool *has_dir_stful) {
  pipe_status_t ret = PIPE_SUCCESS;
  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(shdl))) {
    return ret;
  }

  pipe_mat_tbl_info_t *mat_tbl_info =
      pipe_mgr_get_tbl_info(dev_id, mat_tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    LOG_ERROR("Error finding the table info for tbl 0x%x device id %d",
              mat_tbl_hdl,
              dev_id);
    pipe_mgr_api_exit(shdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  for (unsigned int i = 0; i < mat_tbl_info->num_actions; ++i) {
    pipe_act_fn_info_t *act_info = mat_tbl_info->act_fn_hdl_info + i;
    if (act_fn_hdl == act_info->act_fn_hdl) {
      if (has_dir_stats) *has_dir_stats = !!act_info->dir_stat_hdl;
      if (has_dir_stful) *has_dir_stful = !!act_info->dir_stful_hdl;
      if (act_info->dir_meter_hdl &&
          (has_dir_meter || has_dir_lpf || has_dir_wred)) {
        pipe_mgr_meter_tbl_t *meter_tbl =
            pipe_mgr_meter_tbl_get(dev_id, act_info->dir_meter_hdl);
        if (!meter_tbl) {
          LOG_ERROR(
              "Dev %d cannot find meter/lpf/wred table 0x%x referenced by "
              "act_fn_hdl %u on table %s (0x%x)",
              dev_id,
              act_info->dir_meter_hdl,
              act_fn_hdl,
              mat_tbl_info->name,
              mat_tbl_hdl);
          pipe_mgr_api_exit(shdl);
          PIPE_MGR_DBGCHK(meter_tbl);
          return PIPE_UNEXPECTED;
        }
        if (has_dir_meter) {
          *has_dir_meter = PIPE_METER_TYPE_STANDARD == meter_tbl->type;
        }
        if (has_dir_lpf) {
          *has_dir_lpf = PIPE_METER_TYPE_LPF == meter_tbl->type;
        }
        if (has_dir_wred) {
          *has_dir_wred = PIPE_METER_TYPE_WRED == meter_tbl->type;
        }
      }
      pipe_mgr_api_exit(shdl);
      return PIPE_SUCCESS;
    }
  }
  LOG_ERROR("Dev %d, act_fn_hdl %u not used by table %s (0x%x)",
            dev_id,
            act_fn_hdl,
            mat_tbl_info->name,
            mat_tbl_hdl);
  pipe_mgr_api_exit(shdl);
  return PIPE_OBJ_NOT_FOUND;
}

static pipe_status_t pipe_mgr_get_entry_res(pipe_sess_hdl_t sess_hdl,
                                            dev_target_t dev_tgt,
                                            pipe_mat_tbl_info_t *tbl_info,
                                            pipe_mat_ent_hdl_t entry_hdl,
                                            pipe_act_fn_hdl_t act_fn_hdl,
                                            bool from_hw,
                                            int *sync_stat_tbl,
                                            uint32_t res_get_flags,
                                            pipe_res_get_data_t *res_data) {
  pipe_status_t ret = PIPE_SUCCESS;
  pipe_mat_tbl_hdl_t tbl_hdl = tbl_info->handle;
  bf_dev_id_t dev_id = dev_tgt.device_id;
  /* If we are not requesting any resources there is nothing to do. */
  if (!res_get_flags || res_get_flags == PIPE_RES_GET_FLAG_ENTRY)
    return PIPE_SUCCESS;
  /* Resources were requested, data must be valid. */
  if (!res_data) return PIPE_INVALID_ARG;

  /* Lookup the action info so we know what direct resources apply. */
  pipe_act_fn_info_t *a_info = NULL;
  for (uint32_t i = 0; i < tbl_info->num_actions; ++i) {
    if (tbl_info->act_fn_hdl_info[i].act_fn_hdl == act_fn_hdl) {
      a_info = &tbl_info->act_fn_hdl_info[i];
      break;
    }
  }
  if (!a_info) return PIPE_INVALID_ARG;

  bool get_stats = res_get_flags & PIPE_RES_GET_FLAG_CNTR;
  bool get_meter = res_get_flags & PIPE_RES_GET_FLAG_METER;
  bool get_stful = res_get_flags & PIPE_RES_GET_FLAG_STFUL;
  bool get_idle = res_get_flags & PIPE_RES_GET_FLAG_IDLE;

  res_data->has_counter = false;
  if (get_stats && a_info->dir_stat_hdl) {
    if (from_hw) {
      if (*sync_stat_tbl == STAT_TBL_REQ_SYNC) {
        ret = pipe_mgr_stat_tbl_database_sync(
            sess_hdl, dev_tgt, a_info->dir_stat_hdl, NULL, NULL);
        *sync_stat_tbl = STAT_TBL_SYNCED;
      } else if (*sync_stat_tbl == STAT_TBL_NO_SYNC) {
        ret = pipe_mgr_stat_mgr_direct_stat_ent_sync(
            sess_hdl, dev_id, tbl_hdl, entry_hdl, a_info->dir_stat_hdl);
      }
      if (PIPE_SUCCESS != ret) return ret;
      pipe_mgr_flush_batch(sess_hdl);
      ret = pipe_mgr_drv_ilist_push(&sess_hdl, NULL, NULL);
      if (PIPE_SUCCESS != ret) return ret;
      pipe_mgr_stat_mgr_complete_operations(dev_id, a_info->dir_stat_hdl);
    }
    ret = pipe_mgr_mat_ent_direct_stat_query(
        sess_hdl, dev_id, tbl_hdl, entry_hdl, &res_data->counter);
    if (PIPE_SUCCESS != ret) return ret;
    res_data->has_counter = true;
  }

  res_data->has_meter = false;
  res_data->has_lpf = false;
  res_data->has_red = false;
  if (get_meter && a_info->dir_meter_hdl) {
    ret = pipe_mgr_meter_mgr_read_dir_entry(
        dev_id, tbl_hdl, entry_hdl, a_info->dir_meter_hdl, from_hw, res_data);
    if (PIPE_SUCCESS != ret) return ret;
  }

  res_data->has_stful = false;
  if (get_stful && a_info->dir_stful_hdl) {
    int num_stful_specs = 0;
    ret = pipe_stful_query_get_sizes(
        sess_hdl, dev_id, a_info->dir_stful_hdl, &num_stful_specs);
    if (PIPE_SUCCESS != ret) return ret;
    PIPE_MGR_DBGCHK(num_stful_specs);
    res_data->stful.data =
        PIPE_MGR_CALLOC(num_stful_specs, sizeof(pipe_stful_mem_spec_t));
    if (!res_data->stful.data) return PIPE_NO_SYS_RESOURCES;
    ret = pipe_mgr_stful_direct_ent_query(sess_hdl,
                                          dev_id,
                                          tbl_hdl,
                                          entry_hdl,
                                          &res_data->stful,
                                          from_hw ? PIPE_FLAG_SYNC_REQ : 0);
    if (PIPE_SUCCESS != ret) return ret;
    res_data->has_stful = true;
  }

  res_data->has_ttl = false;
  res_data->has_hit_state = false;
  if (get_idle) {
    idle_tbl_info_t *idle_tbl =
        pipe_mgr_idle_tbl_info_get(dev_id, tbl_info->handle);
    if (idle_tbl && idle_tbl->idle_tbls) {
      if (IDLE_TBL_IS_POLL_MODE(idle_tbl)) {
        ret = rmt_idle_time_get_hit_state(
            sess_hdl, dev_id, tbl_hdl, entry_hdl, &res_data->idle.hit_state);
        if (PIPE_SUCCESS != ret) return ret;
        res_data->has_hit_state = true;
      } else if (IDLE_TBL_IS_NOTIFY_MODE(idle_tbl)) {
        ret = rmt_mat_ent_get_idle_ttl(
            sess_hdl, dev_id, tbl_hdl, entry_hdl, &res_data->idle.ttl);
        if (PIPE_SUCCESS != ret) return ret;
        res_data->has_ttl = true;
      }
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_table_get_default_entry(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_action_spec_t *pipe_action_spec,
    pipe_act_fn_hdl_t *act_fn_hdl,
    bool from_hw,
    uint32_t res_get_flags,
    pipe_res_get_data_t *res_data) {
  pipe_status_t ret = PIPE_SUCCESS;
  if (!pipe_action_spec) return PIPE_INVALID_ARG;
  if (pipe_mgr_is_device_virtual(dev_tgt.device_id) && from_hw) {
    LOG_ERROR(
        "%s:%d Cannot get default entry from hardware for tbl 0x%x on virtual "
        "device %d",
        __func__,
        __LINE__,
        mat_tbl_hdl,
        dev_tgt.device_id);
    return PIPE_NOT_SUPPORTED;
  }

  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }

  if (PIPE_SUCCESS != (ret = pipe_mgr_verify_pipe_tbl_access(
                           sess_hdl, dev_tgt, mat_tbl_hdl, true))) {
    pipe_mgr_api_exit(sess_hdl);
    return ret;
  }

  pipe_mat_tbl_info_t *mat_tbl_info =
      pipe_mgr_get_tbl_info(dev_tgt.device_id, mat_tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    LOG_ERROR("Error in finding the table info for tbl 0x%x device id %d",
              mat_tbl_hdl,
              dev_tgt.device_id);
    ret = PIPE_OBJ_NOT_FOUND;
    goto done;
  }

  /* We need the action function handle to get the resources so always call the
   * entry get, but switch it to a sw get if the entry wasn't requested. */
  bool hw = from_hw && (res_get_flags & PIPE_RES_GET_FLAG_ENTRY);

  /* Call the get function for the appropriate table manager. */
  ret = pipe_mgr_get_default_entry(
      sess_hdl, dev_tgt, mat_tbl_info, pipe_action_spec, act_fn_hdl, hw);

  if (PIPE_SUCCESS == ret && res_get_flags) {
    pipe_mat_ent_hdl_t entry_hdl = 0;
    ret = pipe_mgr_table_get_default_entry_handle(
        sess_hdl, dev_tgt, mat_tbl_hdl, &entry_hdl);
    int sync_stat_tbl = STAT_TBL_NO_SYNC;
    if (PIPE_SUCCESS == ret) {
      ret = pipe_mgr_get_entry_res(sess_hdl,
                                   dev_tgt,
                                   mat_tbl_info,
                                   entry_hdl,
                                   *act_fn_hdl,
                                   from_hw,
                                   &sync_stat_tbl,
                                   res_get_flags,
                                   res_data);
    }
  }
done:
  if (!pipe_mgr_sess_in_batch(sess_hdl) && !pipe_mgr_sess_in_txn(sess_hdl)) {
    pipe_mgr_sm_release(sess_hdl);
  }
  pipe_mgr_api_exit(sess_hdl);

  return ret;
}

pipe_status_t pipe_mgr_mat_tbl_clear(pipe_sess_hdl_t sess_hdl,
                                     dev_target_t dev_tgt,
                                     pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                     uint32_t pipe_api_flags) {
  pipe_status_t ret = pipe_mgr_api_enter(sess_hdl);
  if (PIPE_SUCCESS != ret) {
    return ret;
  }
  if (PIPE_SUCCESS != (ret = pipe_mgr_verify_pipe_tbl_access(
                           sess_hdl, dev_tgt, mat_tbl_hdl, true))) {
    pipe_mgr_api_exit(sess_hdl);
    return ret;
  }

  /* Keep getting first handle and delete till it fails */
  int entry_hdl = -1;
  while (ret == PIPE_SUCCESS) {
    ret = pipe_mgr_tbl_get_first_entry_handle(
        sess_hdl, mat_tbl_hdl, dev_tgt, &entry_hdl);
    if (ret == PIPE_OBJ_NOT_FOUND) {
      /* we are done deleting */
      ret = PIPE_SUCCESS;
      goto done;
    } else if (ret != PIPE_SUCCESS) {
      goto done;
    }
    ret = pipe_mgr_mat_ent_del(
        sess_hdl, dev_tgt.device_id, mat_tbl_hdl, entry_hdl, pipe_api_flags);
    if (ret != PIPE_SUCCESS) {
      goto done;
    }
  }

done:
  if (pipe_mgr_sess_in_txn(sess_hdl)) {
    if (ret != PIPE_SUCCESS) {
      /* Abort the transaction upon failure */
      pipe_mgr_abort_txn_int(sess_hdl);
    }
  } else if (!pipe_mgr_sess_in_batch(sess_hdl)) {
    pipe_mgr_sm_release(sess_hdl);
  }
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t pipe_mgr_mat_ent_del(pipe_sess_hdl_t sess_hdl,
                                   bf_dev_id_t dev_id,
                                   pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                   pipe_mat_ent_hdl_t mat_ent_hdl,
                                   uint32_t pipe_api_flags) {
  dev_target_t dev_tgt;
  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = PIPE_GET_HDL_PIPE(mat_ent_hdl);

  pipe_status_t ret = ml_api_prologue_v2(sess_hdl, dev_tgt, mat_tbl_hdl);
  if (PIPE_SUCCESS != ret) return ret;

  pipe_mgr_move_list_t *move_list = NULL;

  /* Determine table owner. */
  enum pipe_mgr_table_owner_t owner;
  owner = pipe_mgr_sm_tbl_owner(dev_id, mat_tbl_hdl);

  /* Prepare flags for the table managers. */
  uint32_t flags = pipe_mgr_sess_in_txn(sess_hdl) ? PIPE_MGR_TBL_API_TXN : 0;
  flags |= pipe_mgr_sess_in_atomic_txn(sess_hdl) ? PIPE_MGR_TBL_API_ATOM : 0;

  bf_dev_pipe_t pipe_id = -1;
  pipe_tbl_match_spec_t *match_spec = NULL;
  pipe_mat_tbl_info_t *mat_tbl_info = NULL;
  pipe_status_t cur_status = PIPE_SUCCESS;
  mat_tbl_info = pipe_mgr_get_tbl_info(dev_id, mat_tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    goto done;
  }

  /* Call the del function for the appropriate table manager. */
  if (PIPE_MGR_TBL_OWNER_EXM == owner) {
    if (mat_tbl_info->duplicate_entry_check == true) {
      cur_status = pipe_mgr_exm_get_match_spec(
          mat_ent_hdl, mat_tbl_hdl, dev_id, &pipe_id, &match_spec);
      if (cur_status != PIPE_SUCCESS) {
        goto done;
      }
    }
    ret = pipe_mgr_exm_entry_del(
        dev_id, mat_tbl_hdl, mat_ent_hdl, flags, &move_list);
  } else if (PIPE_MGR_TBL_OWNER_TRN == owner) {
    if (mat_tbl_info->duplicate_entry_check == true) {
      cur_status = pipe_mgr_tcam_get_match_spec(
          mat_ent_hdl, mat_tbl_hdl, dev_id, &pipe_id, &match_spec);
      if (cur_status != PIPE_SUCCESS) {
        goto done;
      }
    }
    ret = pipe_mgr_tcam_entry_del(
        dev_id, mat_tbl_hdl, mat_ent_hdl, flags, &move_list);
  } else if (PIPE_MGR_TBL_OWNER_ALPM == owner) {
    if (mat_tbl_info->duplicate_entry_check == true) {
      /*
       * If ATCAM subset key width optimization or
       * exclude MSB bits optimization is used for the ALPM table,
       * then get the full match spec from ALPM. Also, the match spec's
       * memory needs to be freed at end
       */
      if (mat_tbl_info->alpm_info->atcam_subset_key_width ||
          mat_tbl_info->alpm_info->num_excluded_bits) {
        cur_status = pipe_mgr_alpm_get_full_match_spec(
            mat_ent_hdl, mat_tbl_hdl, dev_id, &pipe_id, &match_spec);
      } else {
        cur_status = pipe_mgr_alpm_get_match_spec(
            mat_ent_hdl, mat_tbl_hdl, dev_id, &pipe_id, &match_spec);
      }
      if (cur_status != PIPE_SUCCESS) {
        goto done;
      }
    }
    ret = pipe_mgr_alpm_entry_del(
        dev_id, mat_tbl_hdl, mat_ent_hdl, flags, &move_list);
  } else if (PIPE_MGR_TBL_OWNER_PHASE0 == owner) {
    ret = pipe_mgr_phase0_ent_del(
        sess_hdl, dev_id, mat_tbl_hdl, mat_ent_hdl, &move_list);
  } else if (PIPE_MGR_TBL_OWNER_NO_KEY == owner) {
  } else {
    LOG_ERROR("No handler for table owner %u, session %u, table %u at %s:%d",
              owner,
              sess_hdl,
              mat_tbl_hdl,
              __func__,
              __LINE__);
    PIPE_MGR_DBGCHK(0);
    ret = PIPE_UNEXPECTED;
  }
  if (ret == PIPE_SUCCESS) {
    if ((owner != PIPE_MGR_TBL_OWNER_NO_KEY) &&
        (PIPE_MGR_TBL_OWNER_PHASE0 != owner)) {
      if (mat_tbl_info->duplicate_entry_check) {
        if (!match_spec) {
          LOG_ERROR("%s:%d Failed to get match spec for table 0x%x",
                    __func__,
                    __LINE__,
                    mat_tbl_hdl);
          return PIPE_UNEXPECTED;
        }
        ret = pipe_mgr_mat_tbl_key_delete(dev_id,
                                          mat_ent_hdl,
                                          pipe_mgr_sess_in_txn(sess_hdl),
                                          mat_tbl_info,
                                          pipe_id,
                                          match_spec);
        if (ret != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Error in deleting key for tbl 0x%x, device id %d"
              " from key-based hash table",
              __func__,
              __LINE__,
              mat_tbl_hdl,
              dev_id);
        }
      }
    }
  }

done:
  /*
   * For ALPM tables, if ATCAM subset key width optimization
   * or exclude MSB bits optimization
   * is used, we have to free the match spec as memory would
   * have been allocated to get the full match spec from ALPM
   */
  if (mat_tbl_info) {
    if (PIPE_MGR_TBL_OWNER_ALPM == owner &&
        (mat_tbl_info->alpm_info->atcam_subset_key_width ||
         mat_tbl_info->alpm_info->num_excluded_bits)) {
      pipe_mgr_match_spec_free(match_spec);
    }
  }

  ret =
      ml_api_fin(sess_hdl, ret, pipe_api_flags, dev_id, mat_tbl_hdl, move_list);
  return ret;
}

pipe_status_t pipe_mgr_mat_ent_del_by_match_spec(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_match_spec_t *match_spec,
    uint32_t pipe_api_flags) {
  bool skip_txn = pipe_api_flags & PIPE_FLAG_IGNORE_NOT_FOUND;
  if (!match_spec) return PIPE_INVALID_ARG;
  pipe_status_t ret = pipe_mgr_api_enter(sess_hdl);
  if (PIPE_SUCCESS != ret) {
    return ret;
  }
  if (PIPE_SUCCESS != (ret = pipe_mgr_verify_pipe_tbl_access(
                           sess_hdl, dev_tgt, mat_tbl_hdl, true))) {
    pipe_mgr_api_exit(sess_hdl);
    return ret;
  }

  pipe_mat_ent_hdl_t mat_ent_hdl = 0;
  ret = pipe_mgr_match_spec_to_ent_hdl_int(dev_tgt,
                                           mat_tbl_hdl,
                                           match_spec,
                                           &mat_ent_hdl,
                                           false /* light_pipe_validation */);
  if (PIPE_SUCCESS != ret) {
    goto done;
  }

  ret = pipe_mgr_mat_ent_del(
      sess_hdl, dev_tgt.device_id, mat_tbl_hdl, mat_ent_hdl, pipe_api_flags);

done:
  if (pipe_mgr_sess_in_txn(sess_hdl)) {
    if (ret != PIPE_SUCCESS) {
      if (!(skip_txn && ret == PIPE_OBJ_NOT_FOUND)) {
        /* Abort the transaction upon failure */
        pipe_mgr_abort_txn_int(sess_hdl);
      }
    }
  } else if (!pipe_mgr_sess_in_batch(sess_hdl)) {
    pipe_mgr_sm_release(sess_hdl);
  }
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t pipe_mgr_mat_tbl_default_entry_reset(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    uint32_t pipe_api_flags) {
  pipe_mat_tbl_info_t *mat_tbl_info = NULL;
  pipe_action_spec_t aspec;
  aspec.act_data.action_data_bits = NULL;
  pipe_status_t ret = ml_api_prologue_v2(sess_hdl, dev_tgt, mat_tbl_hdl);
  if (PIPE_SUCCESS != ret) return ret;

  mat_tbl_info =
      pipe_mgr_get_tbl_info(dev_tgt.device_id, mat_tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    LOG_ERROR("Error in finding the table info for tbl 0x%x device id %d",
              mat_tbl_hdl,
              dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  pipe_mgr_move_list_t *move_list = NULL;
  ret = pipe_mgr_cleanup_default_entry(
      sess_hdl, dev_tgt, mat_tbl_info, &move_list);

  /* If there was a P4 specified default program that now. */
  if (PIPE_SUCCESS == ret && mat_tbl_info->default_info &&
      mat_tbl_info->default_info->p4_default) {
    /* Build the action spec. */
    ret = pipe_mgr_create_action_spec(
        dev_tgt.device_id, &mat_tbl_info->default_info->action_entry, &aspec);
    if (PIPE_SUCCESS != ret) {
      LOG_ERROR(
          "Error %s creating action spec for default entry on dev %d pipe %x "
          "table %s 0x%x",
          pipe_str_err(ret),
          dev_tgt.device_id,
          dev_tgt.dev_pipe_id,
          mat_tbl_info->name,
          mat_tbl_info->handle);
      goto done;
    }
    /* If this default entry uses an indirect action entry, get the member
     * handle to use. */
    if (mat_tbl_info->num_adt_tbl_refs &&
        mat_tbl_info->adt_tbl_ref[0].ref_type == PIPE_TBL_REF_TYPE_INDIRECT &&
        aspec.act_data.num_valid_action_data_bits) {
      aspec.pipe_action_datatype_bmap = PIPE_ACTION_DATA_HDL_TYPE;
      ret =
          pipe_mgr_adt_init_entry_hdl_get(dev_tgt,
                                          mat_tbl_hdl,
                                          mat_tbl_info->adt_tbl_ref[0].tbl_hdl,
                                          &aspec.adt_ent_hdl);
      if (PIPE_SUCCESS != ret) {
        LOG_ERROR(
            "%s: Error %s getting action member handle.  Dev %d pipe %x table "
            "%s (0x%x)",
            __func__,
            pipe_str_err(ret),
            dev_tgt.device_id,
            dev_tgt.dev_pipe_id,
            mat_tbl_info->name,
            mat_tbl_info->handle);
        goto done;
      }
    }

    pipe_mgr_move_list_t *ml = NULL;
    pipe_mat_ent_hdl_t entry_hdl;
    ret = pipe_mgr_place_default_entry(
        sess_hdl,
        dev_tgt,
        mat_tbl_info,
        mat_tbl_info->default_info->action_entry.act_fn_hdl,
        &aspec,
        &entry_hdl,
        &ml);
    if (PIPE_SUCCESS != ret) {
      LOG_ERROR(
          "%s: Dev %d tbl %s 0x%x pipe %x error %s placing default entry "
          "after removal",
          __func__,
          dev_tgt.device_id,
          mat_tbl_info->name,
          mat_tbl_info->handle,
          dev_tgt.dev_pipe_id,
          pipe_str_err(ret));
    }
    /* Append ml onto move_list. */
    pipe_mgr_move_list_t *i = move_list;
    while (i && i->next) i = i->next;
    if (i) i->next = ml;
  }
done:
  if (aspec.act_data.action_data_bits) {
    PIPE_MGR_FREE(aspec.act_data.action_data_bits);
  }
  ret = ml_api_fin(
      sess_hdl, ret, pipe_api_flags, dev_tgt.device_id, mat_tbl_hdl, move_list);
  return ret;
}

pipe_status_t pipe_mgr_exm_entry_activate(pipe_sess_hdl_t sess_hdl,
                                          bf_dev_id_t device_id,
                                          pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                          pipe_mat_ent_hdl_t mat_ent_hdl) {
  dev_target_t dev_tgt;
  dev_tgt.device_id = device_id;
  dev_tgt.dev_pipe_id = PIPE_GET_HDL_PIPE(mat_ent_hdl);
  pipe_status_t ret;

  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }

  if (pipe_mgr_sess_in_batch(sess_hdl)) {
    pipe_mgr_drv_ilist_chkpt(sess_hdl);
  }

  if (PIPE_SUCCESS != (ret = pipe_mgr_verify_pipe_tbl_access(
                           sess_hdl, dev_tgt, mat_tbl_hdl, true))) {
    goto out;
  }

  enum pipe_mgr_table_owner_t owner;
  owner = pipe_mgr_sm_tbl_owner(device_id, mat_tbl_hdl);

  if (owner != PIPE_MGR_TBL_OWNER_EXM) {
    LOG_ERROR(
        "%s : Entry activate is only supported for exact match tables "
        "Request made for table handle %d, which is not a exact match"
        " table",
        __func__,
        mat_tbl_hdl);
    ret = PIPE_NOT_SUPPORTED;
    goto out;
  }

  ret = pipe_mgr_exm_tbl_mgr_entry_activate(
      sess_hdl, device_id, mat_tbl_hdl, mat_ent_hdl);

out:
  ret = handleTableApiRsp(sess_hdl, ret, false, __func__, __LINE__);
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t pipe_mgr_exm_entry_deactivate(pipe_sess_hdl_t sess_hdl,
                                            bf_dev_id_t device_id,
                                            pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                            pipe_mat_ent_hdl_t mat_ent_hdl) {
  dev_target_t dev_tgt;
  dev_tgt.device_id = device_id;
  dev_tgt.dev_pipe_id = PIPE_GET_HDL_PIPE(mat_ent_hdl);
  pipe_status_t ret;

  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }

  if (pipe_mgr_sess_in_batch(sess_hdl)) {
    pipe_mgr_drv_ilist_chkpt(sess_hdl);
  }

  if (PIPE_SUCCESS != (ret = pipe_mgr_verify_pipe_tbl_access(
                           sess_hdl, dev_tgt, mat_tbl_hdl, true))) {
    goto out;
  }

  if (pipe_mgr_sm_tbl_owner(device_id, mat_tbl_hdl) != PIPE_MGR_TBL_OWNER_EXM) {
    LOG_ERROR(
        "%s : Entry deactivate is only supported for exact match tables "
        "Request made for table handle %d, which is not a exact match"
        " table",
        __func__,
        mat_tbl_hdl);
    ret = PIPE_NOT_SUPPORTED;
    goto out;
  }

  ret = pipe_mgr_exm_tbl_mgr_entry_deactivate(
      sess_hdl, device_id, mat_tbl_hdl, mat_ent_hdl);

out:
  ret = handleTableApiRsp(sess_hdl, ret, false, __func__, __LINE__);
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t pipe_mgr_mat_ent_set_action(pipe_sess_hdl_t sess_hdl,
                                          bf_dev_id_t dev_id,
                                          pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                          pipe_mat_ent_hdl_t mat_ent_hdl,
                                          pipe_act_fn_hdl_t act_fn_hdl,
                                          pipe_action_spec_t *act_spec,
                                          uint32_t pipe_api_flags) {
  pipe_mgr_move_list_t *move_list = NULL;
  dev_target_t dev_tgt = {dev_id, PIPE_GET_HDL_PIPE(mat_ent_hdl)};

  pipe_status_t ret = ml_api_prologue_v2(sess_hdl, dev_tgt, mat_tbl_hdl);
  if (PIPE_SUCCESS != ret) return ret;

  ret = pipe_mgr_mat_ent_set_action_internal(sess_hdl,
                                             dev_tgt,
                                             mat_tbl_hdl,
                                             mat_ent_hdl,
                                             act_fn_hdl,
                                             act_spec,
                                             &move_list);
  if (ret != PIPE_SUCCESS) {
    LOG_ERROR("Error setting action for tbl 0x%x device id %d",
              mat_tbl_hdl,
              dev_tgt.device_id);
    goto done;
  }

done:
  ret =
      ml_api_fin(sess_hdl, ret, pipe_api_flags, dev_id, mat_tbl_hdl, move_list);
  return ret;
}

pipe_status_t pipe_mgr_mat_ent_set_action_by_match_spec(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_match_spec_t *match_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *act_spec,
    uint32_t pipe_api_flags) {
  if (!match_spec) return PIPE_INVALID_ARG;
  pipe_status_t ret = pipe_mgr_api_enter(sess_hdl);
  if (PIPE_SUCCESS != ret) {
    return ret;
  }
  if (PIPE_SUCCESS != (ret = pipe_mgr_verify_pipe_tbl_access(
                           sess_hdl, dev_tgt, mat_tbl_hdl, true))) {
    pipe_mgr_api_exit(sess_hdl);
    return ret;
  }

  pipe_mat_ent_hdl_t mat_ent_hdl = 0;
  ret = pipe_mgr_match_spec_to_ent_hdl_int(dev_tgt,
                                           mat_tbl_hdl,
                                           match_spec,
                                           &mat_ent_hdl,
                                           false /* light_pipe_validation */);
  if (PIPE_SUCCESS != ret) {
    goto done;
  }

  ret = pipe_mgr_mat_ent_set_action(sess_hdl,
                                    dev_tgt.device_id,
                                    mat_tbl_hdl,
                                    mat_ent_hdl,
                                    act_fn_hdl,
                                    act_spec,
                                    pipe_api_flags);

done:
  if (pipe_mgr_sess_in_txn(sess_hdl)) {
    if (ret != PIPE_SUCCESS) {
      /* Abort the transaction upon failure */
      pipe_mgr_abort_txn_int(sess_hdl);
    }
  } else if (!pipe_mgr_sess_in_batch(sess_hdl)) {
    pipe_mgr_sm_release(sess_hdl);
  }
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t pipe_mgr_mat_ent_set_resource(pipe_sess_hdl_t sess_hdl,
                                            bf_dev_id_t dev_id,
                                            pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                            pipe_mat_ent_hdl_t mat_ent_hdl,
                                            pipe_res_spec_t *resources,
                                            int resource_count,
                                            uint32_t pipe_api_flags) {
  dev_target_t dev_tgt;
  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = PIPE_GET_HDL_PIPE(mat_ent_hdl);

  pipe_status_t ret = ml_api_prologue_v2(sess_hdl, dev_tgt, mat_tbl_hdl);
  if (PIPE_SUCCESS != ret) return ret;

  /* Determine table owner. */
  enum pipe_mgr_table_owner_t owner;
  owner = pipe_mgr_sm_tbl_owner(dev_id, mat_tbl_hdl);

  /* Prepare flags for the table managers. */
  uint32_t flags = pipe_mgr_sess_in_txn(sess_hdl) ? PIPE_MGR_TBL_API_TXN : 0;
  flags |= pipe_mgr_sess_in_atomic_txn(sess_hdl) ? PIPE_MGR_TBL_API_ATOM : 0;

  /* Call the modify function for the appropriate table manager. */
  pipe_mgr_move_list_t *move_list = NULL;
  if (PIPE_MGR_TBL_OWNER_EXM == owner) {
    ret = pipe_mgr_exm_ent_set_resource(dev_id,
                                        mat_tbl_hdl,
                                        mat_ent_hdl,
                                        resources,
                                        resource_count,
                                        flags,
                                        &move_list);
  } else if (PIPE_MGR_TBL_OWNER_TRN == owner) {
    ret = pipe_mgr_tcam_ent_set_resource(dev_id,
                                         mat_tbl_hdl,
                                         mat_ent_hdl,
                                         resources,
                                         resource_count,
                                         flags,
                                         &move_list);
  } else if (PIPE_MGR_TBL_OWNER_ALPM == owner) {
    ret = pipe_mgr_alpm_ent_set_resource(dev_id,
                                         mat_tbl_hdl,
                                         mat_ent_hdl,
                                         resources,
                                         resource_count,
                                         flags,
                                         &move_list);
  } else {
    PIPE_MGR_DBGCHK(0);
    ret = PIPE_UNEXPECTED;
  }

  ret =
      ml_api_fin(sess_hdl, ret, pipe_api_flags, dev_id, mat_tbl_hdl, move_list);
  return ret;
}

/* API to instantiate a new device */
static bf_status_t pipe_mgr_add_device(bf_dev_id_t dev_id,
                                       bf_dev_family_t dev_family,
                                       bf_device_profile_t *prof,
                                       bf_dma_info_t *dma_info,
                                       bf_dev_init_mode_t warm_init_mode) {
  (void)dev_family;
  (void)warm_init_mode;

  pipe_status_t sts = PIPE_SUCCESS;
  bf_status_t status = BF_SUCCESS;

  LOG_TRACE("Entering %s device %u", __func__, dev_id);

  if (!pipe_mgr_ctx) {
    return BF_NOT_READY;
  }

  pipe_sess_hdl_t sess_hdl = pipe_mgr_ctx->int_ses_hndl;
  sts = pipe_mgr_api_enter(sess_hdl);
  if (PIPE_SUCCESS != sts) return BF_SESSION_NOT_FOUND;

  bf_dev_type_t dev_type;
  dev_type = lld_sku_get_dev_type(dev_id);
  sts = pipe_mgr_drv_init_dev(dev_id, dma_info);
  if (PIPE_SUCCESS != sts) {
    LOG_ERROR("%s: failed to initialize drv interfaced (%s %d)",
              __func__,
              pipe_str_err(sts),
              sts);
    sts = PIPE_NO_SYS_RESOURCES;
    goto err_cleanup;
  }

  bool is_virtual_dev_slave = false;
  status = bf_drv_device_type_virtual_dev_slave(dev_id, &is_virtual_dev_slave);
  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "%s: Error in getting virtual device slave type for dev id %d, err %s",
        __func__,
        dev_id,
        pipe_str_err(sts));
    goto err_cleanup;
  }

  sts = pipe_mgr_add_rmt_device(sess_hdl,
                                dev_id,
                                false,
                                is_virtual_dev_slave,
                                dev_type,
                                prof,
                                warm_init_mode);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("%s: failed to instantiate device %s %d",
              __func__,
              pipe_str_err(sts),
              sts);
    goto err_cleanup;
  }

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (dev_info == NULL) {
    LOG_ERROR("%s: failed to get dev_info for dev_id %d", __func__, dev_id);
    goto err_cleanup;
  }

  sts = pipe_mgr_pvs_init(sess_hdl, dev_id);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("%s: failed to instantiate pvs manager %s (%d)",
              __func__,
              pipe_str_err(sts),
              sts);
    goto err_cleanup;
  }

  pipe_mgr_api_exit(sess_hdl);







  for (uint32_t subdev_id = 0; subdev_id < dev_info->num_active_subdevices;
       subdev_id++) {
    // Claim the pipe manager interrupts
    status = bf_int_claim_pbus(dev_id, subdev_id);
    if (status != BF_SUCCESS) {
      LOG_ERROR(
          "interrupt registration with LLD failed for device %d, sts %s (%d)",
          dev_id,
          bf_err_str(status),
          status);
      return status;
    }
    // Enable pipe manager interrupts
    status = bf_int_ena_pbus(dev_id, subdev_id);
    if (status != BF_SUCCESS) {
      LOG_ERROR("interrupt enable with LLD failed for device %d, sts %s (%d)",
                dev_id,
                bf_err_str(status),
                status);
      return status;
    }
  }

  LOG_TRACE("Exiting %s successfully", __func__);
  return BF_SUCCESS;

err_cleanup:
  pipe_mgr_api_exit(sess_hdl);
  return sts;
}

static bf_status_t pipe_mgr_add_virtual_device(
    bf_dev_id_t dev_id,
    bf_dev_type_t dev_type,
    bf_device_profile_t *prof,
    bf_dev_init_mode_t warm_init_mode) {
  pipe_status_t sts = PIPE_SUCCESS;

  LOG_TRACE("Entering %s device %u", __func__, dev_id);

  if (!pipe_mgr_ctx) {
    return BF_NOT_READY;
  }

  pipe_sess_hdl_t sess_hdl = pipe_mgr_ctx->int_ses_hndl;
  sts = pipe_mgr_api_enter(sess_hdl);
  if (PIPE_SUCCESS != sts) return BF_SESSION_NOT_FOUND;

  sts = pipe_mgr_add_rmt_device(
      sess_hdl, dev_id, true, false, dev_type, prof, warm_init_mode);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("%s: failed to instantiate device %s %d",
              __func__,
              pipe_str_err(sts),
              sts);
    goto err_cleanup;
  }
  pipe_mgr_api_exit(sess_hdl);

  LOG_TRACE("Exiting %s successfully", __func__);
  return BF_SUCCESS;

err_cleanup:
  pipe_mgr_api_exit(sess_hdl);
  return sts;
}

static pipe_status_t pipe_mgr_stop_timers(bf_dev_id_t dev_id) {
  rmt_dev_info_t *dev_info;

  if (!(dev_info = pipe_mgr_get_dev_info(dev_id))) {
    return PIPE_OBJ_NOT_FOUND;
  }
  if (dev_info->virtual_device) return PIPE_SUCCESS;

  pipe_mgr_port_stuck_detect_timer_cleanup(dev_id);
  pipe_mgr_tcam_scrub_timer_stop(dev_id);
  return PIPE_SUCCESS;
}

/* API to de-instantiate a device */
static bf_status_t pipe_mgr_remove_device(bf_dev_id_t dev_id) {
  pipe_status_t sts = PIPE_SUCCESS;

  LOG_TRACE("Entering %s, dev %d", __func__, dev_id);

  if (!pipe_mgr_ctx) {
    return PIPE_NOT_READY;
  }

  /* Stop the timers before taking the exclusive API lock.  Note that the
   * bf_sys_timer implementation will ensure that stopping the timers will
   * guarantee there are no "last" executions of the timer callbacks in progress
   * by another thread.
   * The timers should be stopped before taking the exclusive API lock to
   * prevent a timer callback from blocking on the exclusive API lock while
   * holding the bf_sys_timer lock.  */
  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  sts = pipe_mgr_api_enter(shdl);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("%s: failed to take api lock device %d, err %s",
              __func__,
              dev_id,
              pipe_str_err(sts));
  }
  pipe_mgr_stop_timers(dev_id);
  pipe_mgr_api_exit(shdl);

  pipe_mgr_exclusive_api_enter(dev_id);
  sts = pipe_mgr_remove_rmt_device(dev_id);
  pipe_mgr_exclusive_api_exit(dev_id);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("%s: failed to de-instantiate device %d, err %s",
              __func__,
              dev_id,
              pipe_str_err(sts));
    return sts;
  }

  LOG_TRACE("Exiting %s successfully", __func__);
  return PIPE_SUCCESS;
}

bf_status_t bf_pipe_mgr_port_ibuf_set_drop_threshold(bf_dev_id_t dev_id,
                                                     bf_dev_port_t port_id,
                                                     uint32_t drop_hi_thrd,
                                                     uint32_t drop_low_thrd) {
  pipe_status_t sts = PIPE_SUCCESS;

  LOG_TRACE("Entering %s", __func__);

  if (!pipe_mgr_ctx) {
    return PIPE_NOT_READY;
  }

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s: Invalid device id %d", __func__, dev_id);
    return PIPE_INVALID_ARG;
  }

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;

  sts = pipe_mgr_api_enter(shdl);
  if (PIPE_SUCCESS != sts) return sts;

  sts = pipe_mgr_parde_port_set_drop_threshold(
      shdl, dev_id, port_id, drop_hi_thrd, drop_low_thrd);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("%s: failed to set drop thresholds for dev %d, port %d",
              __func__,
              dev_id,
              port_id);
    goto err_cleanup;
  }

  sts = pipe_mgr_drv_ilist_push(&shdl, NULL, NULL);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("%s: failed to push instructions", __func__);
    goto err_cleanup;
  }
  pipe_mgr_api_exit(shdl);
  LOG_TRACE("Exiting %s successfully", __func__);
  return PIPE_SUCCESS;

err_cleanup:
  pipe_mgr_drv_ilist_abort(&shdl);
  pipe_mgr_api_exit(shdl);
  return sts;
}

bf_status_t bf_pipe_mgr_port_ibuf_set_afull_threshold(bf_dev_id_t dev_id,
                                                      bf_dev_port_t port_id,
                                                      uint32_t afull_hi_thrd,
                                                      uint32_t afull_low_thrd) {
  pipe_status_t sts = PIPE_SUCCESS;

  LOG_TRACE("Entering %s", __func__);

  if (!pipe_mgr_ctx) {
    return PIPE_NOT_READY;
  }

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s: Invalid device id %d", __func__, dev_id);
    return PIPE_INVALID_ARG;
  }

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;

  sts = pipe_mgr_api_enter(shdl);
  if (PIPE_SUCCESS != sts) return sts;

  sts = pipe_mgr_parde_port_set_afull_threshold(
      shdl, dev_id, port_id, afull_hi_thrd, afull_low_thrd);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("%s: failed to set afull thresholds for dev %d, port %d",
              __func__,
              dev_id,
              port_id);
    goto err_cleanup;
  }

  sts = pipe_mgr_drv_ilist_push(&shdl, NULL, NULL);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("%s: failed to push instructions", __func__);
    goto err_cleanup;
  }
  pipe_mgr_api_exit(shdl);
  LOG_TRACE("Exiting %s successfully", __func__);
  return PIPE_SUCCESS;

err_cleanup:
  pipe_mgr_drv_ilist_abort(&shdl);
  pipe_mgr_api_exit(shdl);
  return sts;
}

/* Exposed through BF_PAL API to enable cut-through in EBUF for a port */
bf_status_t bf_pipe_mgr_port_ebuf_enable_cut_through(bf_dev_id_t dev_id,
                                                     bf_dev_port_t port_id) {
  pipe_status_t sts = PIPE_SUCCESS;

  LOG_TRACE("Entering %s", __func__);

  if (!pipe_mgr_ctx) {
    return PIPE_NOT_READY;
  }

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s: Invalid device id %d", __func__, dev_id);
    return PIPE_INVALID_ARG;
  }

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;

  sts = pipe_mgr_api_enter(shdl);
  if (PIPE_SUCCESS != sts) return sts;

  sts = pipe_mgr_parde_set_port_cut_through(shdl, dev_info, port_id, true);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("%s: failed to enable cut-through for dev %d, port %d",
              __func__,
              dev_id,
              port_id);
    goto err_cleanup;
  }

  sts = pipe_mgr_drv_ilist_push(&shdl, NULL, NULL);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("%s: failed to push instructions", __func__);
    goto err_cleanup;
  }
  pipe_mgr_api_exit(shdl);
  LOG_TRACE("Exiting %s successfully", __func__);
  return PIPE_SUCCESS;

err_cleanup:
  pipe_mgr_drv_ilist_abort(&shdl);
  pipe_mgr_api_exit(shdl);
  return sts;
}

/* Exposed through BF_PAL API to disable cut-through in EBUF for a port */
bf_status_t bf_pipe_mgr_port_ebuf_disable_cut_through(bf_dev_id_t dev_id,
                                                      bf_dev_port_t port_id) {
  pipe_status_t sts = PIPE_SUCCESS;

  LOG_TRACE("Entering %s", __func__);

  if (!pipe_mgr_ctx) {
    return PIPE_NOT_READY;
  }

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s: Invalid device id %d", __func__, dev_id);
    return PIPE_INVALID_ARG;
  }

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;

  sts = pipe_mgr_api_enter(shdl);
  if (PIPE_SUCCESS != sts) return sts;

  sts = pipe_mgr_parde_set_port_cut_through(shdl, dev_info, port_id, false);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("%s: failed to disable cut-through for dev %d, port %d",
              __func__,
              dev_id,
              port_id);
    goto err_cleanup;
  }

  sts = pipe_mgr_drv_ilist_push(&shdl, NULL, NULL);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("%s: failed to push instructions", __func__);
    goto err_cleanup;
  }
  pipe_mgr_api_exit(shdl);
  LOG_TRACE("Exiting %s successfully", __func__);
  return PIPE_SUCCESS;

err_cleanup:
  pipe_mgr_drv_ilist_abort(&shdl);
  pipe_mgr_api_exit(shdl);
  return sts;
}

/* API to add a port to the device */
bf_status_t pipe_mgr_add_port(bf_dev_id_t dev_id,
                              bf_dev_port_t port_id,
                              bf_port_attributes_t *port_attrib,
                              bf_port_cb_direction_t direction) {
  pipe_status_t sts = PIPE_SUCCESS;
  bf_port_speed_t speed = port_attrib->port_speeds;

  LOG_TRACE("Entering %s", __func__);

  if (!pipe_mgr_ctx) {
    return PIPE_NOT_READY;
  }

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;

  sts = pipe_mgr_api_enter(shdl);
  if (PIPE_SUCCESS != sts) return sts;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s: Invalid device id %d", __func__, dev_id);
    sts = PIPE_INVALID_ARG;
    goto err_cleanup;
  }

  /* Do a little validation related to the 25g overspeed feature, since it only
   * applies to TF1 only perform the checks for that device family. */
  if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO && speed == BF_SPEED_25G) {
    bool overspeed = false;
    pipe_mgr_overspeed_map_get(dev_id, port_id, &overspeed);
    if (overspeed) {
      /* Tofino-1 requires two channels for 50g programming and they must be the
       * even channels in a quad.  Verify that is the case now and also that the
       * odd channel in this pair is free. */
      if (port_id & 1) {
        LOG_ERROR(
            "Dev %d port %d: Cannot create %s port, only even channels may "
            "be used in 25G overspeed mode.",
            dev_id,
            port_id,
            bf_port_speed_str(speed));
        sts = BF_INVALID_ARG;
        goto err_cleanup;
      }
      rmt_port_info_t *port_info = pipe_mgr_get_port_info(dev_id, port_id + 1);
      if (port_info) {
        LOG_ERROR(
            "Dev %d port %d: Cannot create %s port, port %d exists as a %s "
            "port",
            dev_id,
            port_id,
            bf_port_speed_str(speed),
            port_id + 1,
            bf_port_speed_str(port_info->speed));
        sts = BF_IN_USE;
        goto err_cleanup;
      }

      /* In the 25G overspeed mode, 25g ports are programmed at 50g rates.
       * Update the speed here from 25 to 50 so the rest of the parde
       * programming can go through without modification. */
      speed = BF_SPEED_50G;
    }
  }

  /* If this is an odd numbered port it is possible that the adjacent channel is
   * a 25g port in overspeed mode (we see it as 50g).  Ensure that is not the
   * case here since we cannot rely on other managers (bf_pm/port_mgr) to
   * validate the channel combinations because we may have changed it from one
   * to two channels. */
  if (port_id & 1 && dev_info->dev_family == BF_DEV_FAMILY_TOFINO) {
    rmt_port_info_t *port_info = pipe_mgr_get_port_info(dev_id, port_id - 1);
    if (port_info && port_info->speed == BF_SPEED_50G) {
      LOG_ERROR(
          "Cannot create dev %d dev_port %d as a %s port, port %d already "
          "exists as a %s port",
          dev_id,
          port_id,
          bf_port_speed_str(speed),
          port_id - 1,
          bf_port_speed_str(port_info->speed));
      sts = BF_IN_USE;
      goto err_cleanup;
    }
  }

  if (direction == BF_PORT_CB_DIRECTION_EGRESS) {
    sts = pipe_mgr_add_rmt_port(dev_id, port_id, speed);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR("%s: failed to add port", __func__);
      goto err_cleanup;
    }
    if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO) {
      /* Setup PGR first for Tofino. */
      bf_status_t x = pipe_mgr_pktgen_port_add(dev_info, port_id, speed);
      if (x != BF_SUCCESS) {
        LOG_ERROR(
            "%s: failed to initialize PGR port controls %d (%s), dev %d port "
            "%d",
            __func__,
            x,
            bf_err_str(x),
            dev_id,
            port_id);
        sts = PIPE_COMM_FAIL;
        goto err_cleanup;
      }
    }
  }

  /* Skip ParDe programming for hitless HA */
  if (!pipe_mgr_hitless_warm_init_in_progress(dev_id)) {
    sts = pipe_mgr_parde_port_add(shdl, dev_info, direction, port_id);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR("%s: failed to add port %d on dev %d, sts %s",
                __func__,
                port_id,
                dev_id,
                pipe_str_err(sts));
      goto err_cleanup;
    }
  }
  if (direction == BF_PORT_CB_DIRECTION_EGRESS) {
    /* Setup PGR last if not on Tofino. */
    if (dev_info->dev_family != BF_DEV_FAMILY_TOFINO) {
      bf_status_t x = pipe_mgr_pktgen_port_add(dev_info, port_id, speed);
      if (x != BF_SUCCESS) {
        LOG_ERROR(
            "%s: failed to initialize PGR port controls %d (%s), dev %d port "
            "%d",
            __func__,
            x,
            bf_err_str(x),
            dev_id,
            port_id);
        sts = PIPE_COMM_FAIL;
        goto err_cleanup;
      }
    }
  }

  sts = pipe_mgr_drv_ilist_push(&shdl, NULL, NULL);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("%s: failed to push instructions", __func__);
    goto err_cleanup;
  }
  sts = pipe_mgr_drv_i_list_cmplt_all(&shdl);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("%s: failed to complete instructions", __func__);
    goto err_cleanup;
  }
  pipe_mgr_api_exit(shdl);
  LOG_TRACE("Exiting %s successfully", __func__);
  return PIPE_SUCCESS;

err_cleanup:
  pipe_mgr_drv_ilist_abort(&shdl);
  pipe_mgr_api_exit(shdl);
  return sts;
}

/* API to remove a port from the device */
bf_status_t pipe_mgr_remove_port(bf_dev_id_t dev_id,
                                 bf_dev_port_t port_id,
                                 bf_port_cb_direction_t direction) {
  pipe_status_t psts = PIPE_SUCCESS;
  bf_status_t sts = BF_SUCCESS;

  LOG_TRACE("Entering %s", __func__);

  if (!pipe_mgr_ctx) {
    return PIPE_NOT_READY;
  }

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  psts = pipe_mgr_api_enter(shdl);
  if (PIPE_SUCCESS != psts) return BF_INVALID_ARG;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s: Invalid device id %d", __func__, dev_id);
    sts = PIPE_INVALID_ARG;
    goto err_cleanup;
  }

  rmt_port_info_t *port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (port_info == NULL) {
    /* pipe_mgr doesn't have this port, return and let other managers process
     * the remove. */
    goto err_cleanup;
  }

  psts = pipe_mgr_parde_port_rmv(shdl, dev_info, direction, port_id);
  if (psts != PIPE_SUCCESS) {
    sts = BF_INVALID_ARG;
    LOG_ERROR("%s: failed to remove port", __func__);
    goto err_cleanup;
  }
  if (direction == BF_PORT_CB_DIRECTION_EGRESS) {
    sts = pipe_mgr_pktgen_port_rem(dev_info, port_id);
    if (BF_SUCCESS != sts) {
      LOG_ERROR(
          "%s: failed to remove port %d (%s)", __func__, sts, bf_err_str(sts));
      goto err_cleanup;
    }
    psts = pipe_mgr_remove_rmt_port(dev_id, port_id);
    if (psts != PIPE_SUCCESS) {
      sts = BF_INVALID_ARG;
      LOG_ERROR("%s failed to remove port %d err %d (%s)",
                __func__,
                port_id,
                psts,
                pipe_str_err(psts));
      goto err_cleanup;
    }
  }

  sts = pipe_mgr_drv_ilist_push(&shdl, NULL, NULL);
  if (sts != PIPE_SUCCESS) {
    goto err_cleanup;
  }
  sts = pipe_mgr_drv_i_list_cmplt_all(&shdl);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("%s: failed to complete instructions", __func__);
    goto err_cleanup;
  }

  pipe_mgr_api_exit(shdl);
  LOG_TRACE("Exiting %s successfully", __func__);
  return BF_SUCCESS;

err_cleanup:
  pipe_mgr_drv_ilist_abort(&shdl);
  pipe_mgr_api_exit(shdl);
  return sts;
}

// Port mode transition issue workaround
// API to complete port mode transition workaround after port add
bf_status_t pipe_mgr_complete_port_mode_transition_wa(bf_dev_id_t dev_id,
                                                      bf_dev_port_t port_id) {
  pipe_status_t sts = PIPE_SUCCESS;

  LOG_TRACE("Entering %s", __func__);

  if (!pipe_mgr_ctx) {
    return PIPE_NOT_READY;
  }

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;

  sts = pipe_mgr_api_enter(shdl);
  if (PIPE_SUCCESS != sts) return sts;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s: Invalid device id %d", __func__, dev_id);
    pipe_mgr_api_exit(shdl);
    return PIPE_INVALID_ARG;
  }

  sts =
      pipe_mgr_parde_complete_port_mode_transition_wa(shdl, dev_info, port_id);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s: failed to complete port mode transition wa for port %d on dev %d, "
        "sts %s",
        __func__,
        port_id,
        dev_id,
        pipe_str_err(sts));
    goto err_cleanup;
  }

  sts = pipe_mgr_drv_ilist_push(&shdl, NULL, NULL);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("%s: failed to push instructions", __func__);
    goto err_cleanup;
  }
  pipe_mgr_api_exit(shdl);
  LOG_TRACE("Exiting %s successfully", __func__);
  return PIPE_SUCCESS;

err_cleanup:
  pipe_mgr_drv_ilist_abort(&shdl);
  pipe_mgr_api_exit(shdl);
  return sts;
}

pipe_status_t bf_dma_service_pipe_ilist_completion(bf_dev_id_t dev_id) {
  return pipe_mgr_drv_service_ilist_drs(dev_id);
}

pipe_status_t bf_dma_service_pipe_learning(bf_dev_id_t dev_id) {
  bool en = false;
  pipe_sess_hdl_t shdl = pipe_mgr_get_int_sess_hdl();
  pipe_status_t sts = pipe_mgr_flow_lrn_get_intr_mode(shdl, dev_id, &en);
  /* Service learning only if interrupt based learning is disabled. */
  if (PIPE_SUCCESS == sts && !en) {
    sts = pipe_mgr_drv_service_learn_drs(dev_id);
  }
  return sts;
}

pipe_status_t bf_dma_service_pipe_idle_time(bf_dev_id_t dev_id) {
  return pipe_mgr_drv_service_idle_time_drs(dev_id);
}

pipe_status_t bf_dma_service_pipe_read_block_completion(bf_dev_id_t dev_id) {
  return pipe_mgr_drv_service_read_blk_drs(dev_id);
}

pipe_status_t bf_dma_service_pipe_write_block_completion(bf_dev_id_t dev_id) {
  return pipe_mgr_drv_service_write_blk_drs(dev_id, true, true);
}

pipe_status_t bf_dma_service_pipe_stats(bf_dev_id_t dev_id) {
  return pipe_mgr_drv_service_stats_drs(dev_id);
}

/*!
 * API function to register a callback function to track updates to groups in
 * the selection table.
 */
pipe_status_t pipe_mgr_sel_tbl_register_cb(pipe_sess_hdl_t sess_hdl,
                                           bf_dev_id_t device_id,
                                           pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                           pipe_mgr_sel_tbl_update_callback cb,
                                           void *cb_cookie) {
  RMT_API(sess_hdl,
          0,
          pipe_mgr_verify_tbl_access(sess_hdl, device_id, sel_tbl_hdl, true),
          rmt_sel_tbl_set_cb(sess_hdl, device_id, sel_tbl_hdl, cb, cb_cookie));
}

/*!
 * API function to set the group profile for a selection table
 */
pipe_status_t pipe_mgr_sel_tbl_profile_set(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_sel_tbl_profile_t *sel_tbl_profile) {
  RMT_API(
      sess_hdl,
      0,
      pipe_mgr_verify_pipe_tbl_access(sess_hdl, dev_tgt, sel_tbl_hdl, true),
      rmt_sel_tbl_profile_set(sess_hdl, dev_tgt, sel_tbl_hdl, sel_tbl_profile));
}

/*!
 * API function to add a new group into a selection table
 */
pipe_status_t pipe_mgr_sel_grp_add(pipe_sess_hdl_t sess_hdl,
                                   dev_target_t dev_tgt,
                                   pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                   pipe_sel_grp_id_t grp_id,
                                   uint32_t max_grp_size,
                                   uint32_t adt_offset,
                                   pipe_sel_grp_hdl_t *sel_grp_hdl_p,
                                   uint32_t pipe_api_flags) {
  pipe_status_t ret = ml_api_prologue_v2(sess_hdl, dev_tgt, sel_tbl_hdl);
  if (PIPE_SUCCESS != ret) return ret;

  /* Prepare flags for the table manager. */
  uint32_t flags = pipe_mgr_sess_in_txn(sess_hdl) ? PIPE_MGR_TBL_API_TXN : 0;
  flags |= pipe_mgr_sess_in_atomic_txn(sess_hdl) ? PIPE_MGR_TBL_API_ATOM : 0;
  flags |= pipe_api_flags & PIPE_FLAG_CACHE_ENT_ID;

  /* Issue the placement function. */
  pipe_mgr_sel_move_list_t *move_list = NULL;
  ret = rmt_sel_grp_add(sess_hdl,
                        dev_tgt,
                        sel_tbl_hdl,
                        grp_id,
                        max_grp_size,
                        adt_offset,
                        sel_grp_hdl_p,
                        flags,
                        &move_list);

  ret = ml_api_fin(sess_hdl,
                   ret,
                   pipe_api_flags,
                   dev_tgt.device_id,
                   sel_tbl_hdl,
                   (pipe_mgr_move_list_t *)move_list);
  return ret;
}

/*!
 * API function to delete a group from a selection table
 */
pipe_status_t pipe_mgr_sel_grp_del(pipe_sess_hdl_t sess_hdl,
                                   bf_dev_id_t device_id,
                                   pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                   pipe_sel_grp_hdl_t sel_grp_hdl,
                                   uint32_t pipe_api_flags) {
  pipe_status_t ret = PIPE_SUCCESS;
  ret = ml_api_prologue(sess_hdl, device_id, sel_tbl_hdl);
  if (PIPE_SUCCESS != ret) return ret;

  /* Prepare flags for the table manager. */
  uint32_t flags = pipe_mgr_sess_in_txn(sess_hdl) ? PIPE_MGR_TBL_API_TXN : 0;
  flags |= pipe_mgr_sess_in_atomic_txn(sess_hdl) ? PIPE_MGR_TBL_API_ATOM : 0;

  /* Issue the placement function. */
  pipe_mgr_sel_move_list_t *move_list = NULL;
  ret = rmt_sel_grp_del(
      sess_hdl, device_id, sel_tbl_hdl, sel_grp_hdl, flags, &move_list);

  ret = ml_api_fin(sess_hdl,
                   ret,
                   pipe_api_flags,
                   device_id,
                   sel_tbl_hdl,
                   (pipe_mgr_move_list_t *)move_list);
  return ret;
}

/*!
 * API function to resize a group from a selection table
 * This function uses 3 different move lists in order to update HW in the
 * proper order.
 *  1. Create new group and move all members to it.
 *  2. Notify EXM/TCAM to refresh action data.
 *  3. Cleanup old group.
 */
pipe_status_t pipe_mgr_sel_grp_size_set(pipe_sess_hdl_t sess_hdl,
                                        dev_target_t dev_tgt,
                                        pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                        pipe_sel_grp_hdl_t sel_grp_hdl,
                                        uint32_t max_grp_size) {
  pipe_status_t ret = pipe_mgr_api_enter(sess_hdl);
  if (PIPE_SUCCESS != ret) {
    return ret;
  }
  /* Since MAT lock will also lock resource tables, locking just selector table
    here should be good enough for the resize operation. */
  if (PIPE_SUCCESS != (ret = pipe_mgr_verify_pipe_tbl_access(
                           sess_hdl, dev_tgt, sel_tbl_hdl, true))) {
    pipe_mgr_api_exit(sess_hdl);
    return ret;
  }

  /* Prepare flags for the table manager. */
  uint32_t flags = pipe_mgr_sess_in_txn(sess_hdl) ? PIPE_MGR_TBL_API_TXN : 0;
  flags |= pipe_mgr_sess_in_atomic_txn(sess_hdl) ? PIPE_MGR_TBL_API_ATOM : 0;

  pipe_mgr_sel_move_list_t *move_list = NULL;
  /* tmp_grp_hdl is used to call cleanup on it */
  pipe_sel_grp_hdl_t tmp_grp_hdl;
  ret = rmt_sel_grp_resize(sess_hdl,
                           dev_tgt,
                           sel_tbl_hdl,
                           sel_grp_hdl,
                           &tmp_grp_hdl,
                           max_grp_size,
                           flags,
                           &move_list);
  if (ret != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Unable to resize selector 0x%x grp 0x%x",
              __func__,
              __LINE__,
              sel_tbl_hdl,
              sel_grp_hdl);
    goto err;
  }
  if (move_list == NULL) {
    /* Nothing to do, size is the same. */
    goto err;
  }

  ret = pipe_mgr_sm_save_ml_prep(sess_hdl, dev_tgt.device_id, sel_tbl_hdl);
  if (ret != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Fail saving ml %d", __func__, __LINE__, ret);
    goto err;
  }
  pipe_mgr_sm_save_ml(sess_hdl,
                      dev_tgt.device_id,
                      sel_tbl_hdl,
                      (pipe_mgr_move_list_t *)move_list);

  /* Handle movelists inside, will be stored in the session. */
  ret = rmt_sel_grp_notify_mat(sess_hdl, dev_tgt, sel_tbl_hdl, sel_grp_hdl);
  if (ret != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Failed to update action rams in MAT %d",
              __func__,
              __LINE__,
              ret);
    goto err;
  }
  move_list = NULL;
  // Setting flag on delete will make it not delete from grp_id_map
  flags |= PIPE_MGR_TBL_API_CACHE_ENT_ID | PIPE_FLAG_SKIP_BACKUP;
  ret = rmt_sel_grp_del(
      sess_hdl, dev_tgt.device_id, sel_tbl_hdl, tmp_grp_hdl, flags, &move_list);
  if (ret != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Fail on temporary group cleanup %d", __func__, __LINE__, ret);
    goto err;
  }
  flags ^= (PIPE_MGR_TBL_API_CACHE_ENT_ID | PIPE_FLAG_SKIP_BACKUP);

  ret = pipe_mgr_sm_save_ml_prep(sess_hdl, dev_tgt.device_id, sel_tbl_hdl);
  if (ret != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Fail saving ml %d", __func__, __LINE__, ret);
    goto err;
  }
  pipe_mgr_sm_save_ml(sess_hdl,
                      dev_tgt.device_id,
                      sel_tbl_hdl,
                      (pipe_mgr_move_list_t *)move_list);
  if (flags == 0) {
    pipe_mgr_sm_commit(sess_hdl);
    // Don't go to err: after commit
    ret = pipe_mgr_drv_ilist_push(&sess_hdl, NULL, NULL);
    if (ret != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Fail while commiting resize changes %d",
                __func__,
                __LINE__,
                ret);
    }
    pipe_mgr_sm_release(sess_hdl);
  }
  pipe_mgr_api_exit(sess_hdl);

  return ret;

err:
  /* Should take care of cleaning up transaction/batch. */
  ret = ml_api_fin(sess_hdl, ret, flags, dev_tgt.device_id, sel_tbl_hdl, NULL);
  return ret;
}
/*!
 * API function to add a member to a group of a selection table
 */
pipe_status_t pipe_mgr_sel_grp_mbr_add(pipe_sess_hdl_t sess_hdl,
                                       bf_dev_id_t device_id,
                                       pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                       pipe_sel_grp_hdl_t sel_grp_hdl,
                                       pipe_act_fn_hdl_t act_fn_hdl,
                                       pipe_adt_ent_hdl_t adt_ent_hdl,
                                       uint32_t pipe_api_flags) {
  dev_target_t dev_tgt;
  dev_tgt.device_id = device_id;
  dev_tgt.dev_pipe_id = PIPE_GET_HDL_PIPE(adt_ent_hdl);

  pipe_status_t ret = ml_api_prologue_v2(sess_hdl, dev_tgt, sel_tbl_hdl);
  if (PIPE_SUCCESS != ret) return ret;

  /* Prepare flags for the table manager. */
  uint32_t flags = pipe_mgr_sess_in_txn(sess_hdl) ? PIPE_MGR_TBL_API_TXN : 0;
  flags |= pipe_mgr_sess_in_atomic_txn(sess_hdl) ? PIPE_MGR_TBL_API_ATOM : 0;

  /* Issue the placement function. */
  pipe_mgr_sel_move_list_t *move_list = NULL;
  ret = rmt_sel_grp_mbr_add(sess_hdl,
                            device_id,
                            sel_tbl_hdl,
                            sel_grp_hdl,
                            act_fn_hdl,
                            adt_ent_hdl,
                            flags,
                            &move_list);

  ret = ml_api_fin(sess_hdl,
                   ret,
                   pipe_api_flags,
                   device_id,
                   sel_tbl_hdl,
                   (pipe_mgr_move_list_t *)move_list);
  return ret;
}

/*!
 * API function to delete a member from a group of a selection table
 */
pipe_status_t pipe_mgr_sel_grp_mbr_del(pipe_sess_hdl_t sess_hdl,
                                       bf_dev_id_t device_id,
                                       pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                       pipe_sel_grp_hdl_t sel_grp_hdl,
                                       pipe_adt_ent_hdl_t adt_ent_hdl,
                                       uint32_t pipe_api_flags) {
  dev_target_t dev_tgt;
  dev_tgt.device_id = device_id;
  dev_tgt.dev_pipe_id = PIPE_GET_HDL_PIPE(adt_ent_hdl);

  pipe_status_t ret = ml_api_prologue_v2(sess_hdl, dev_tgt, sel_tbl_hdl);
  if (PIPE_SUCCESS != ret) return ret;

  /* Prepare flags for the table manager. */
  uint32_t flags = pipe_mgr_sess_in_txn(sess_hdl) ? PIPE_MGR_TBL_API_TXN : 0;
  flags |= pipe_mgr_sess_in_atomic_txn(sess_hdl) ? PIPE_MGR_TBL_API_ATOM : 0;

  /* Issue the placement function. */
  pipe_mgr_sel_move_list_t *move_list = NULL;
  ret = rmt_sel_grp_mbr_del(sess_hdl,
                            device_id,
                            sel_tbl_hdl,
                            sel_grp_hdl,
                            adt_ent_hdl,
                            flags,
                            &move_list);

  ret = ml_api_fin(sess_hdl,
                   ret,
                   pipe_api_flags,
                   device_id,
                   sel_tbl_hdl,
                   (pipe_mgr_move_list_t *)move_list);
  return ret;
}

/*!
 * API function to set membership of a group
 */
pipe_status_t pipe_mgr_sel_grp_mbrs_set(pipe_sess_hdl_t sess_hdl,
                                        bf_dev_id_t device_id,
                                        pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                        pipe_sel_grp_hdl_t sel_grp_hdl,
                                        uint32_t num_mbrs,
                                        pipe_adt_ent_hdl_t *mbrs,
                                        bool *enable,
                                        uint32_t pipe_api_flags) {
  dev_target_t dev_tgt;

  dev_tgt.device_id = device_id;
  dev_tgt.dev_pipe_id = num_mbrs ? PIPE_GET_HDL_PIPE(mbrs[0]) : 0;

  pipe_status_t ret = ml_api_prologue_v2(sess_hdl, dev_tgt, sel_tbl_hdl);
  if (PIPE_SUCCESS != ret) return ret;

  /* Prepare flags for the table manager. */
  uint32_t flags = pipe_mgr_sess_in_txn(sess_hdl) ? PIPE_MGR_TBL_API_TXN : 0;
  flags |= pipe_mgr_sess_in_atomic_txn(sess_hdl) ? PIPE_MGR_TBL_API_ATOM : 0;

  /* Issue the placement function. */
  pipe_mgr_sel_move_list_t *move_list = NULL;
  ret = rmt_sel_grp_mbrs_set(sess_hdl,
                             device_id,
                             sel_tbl_hdl,
                             sel_grp_hdl,
                             num_mbrs,
                             mbrs,
                             enable,
                             flags,
                             &move_list);

  ret = ml_api_fin(sess_hdl,
                   ret,
                   pipe_api_flags,
                   device_id,
                   sel_tbl_hdl,
                   (pipe_mgr_move_list_t *)move_list);
  return ret;
}

/*!
 * API function to get membership of a group
 */
pipe_status_t pipe_mgr_sel_grp_mbrs_get(pipe_sess_hdl_t sess_hdl,
                                        bf_dev_id_t device_id,
                                        pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                        pipe_sel_grp_hdl_t sel_grp_hdl,
                                        uint32_t mbrs_size,
                                        pipe_adt_ent_hdl_t *mbrs,
                                        bool *enable,
                                        uint32_t *mbrs_populated,
                                        bool from_hw) {
  if (from_hw) {
    LOG_WARN("%s:%d Table entry get from hardware not supported",
             __func__,
             __LINE__);
  }

  pipe_status_t ret = pipe_mgr_api_enter(sess_hdl);
  if (PIPE_SUCCESS != ret) {
    return ret;
  }
  if (PIPE_SUCCESS != (ret = pipe_mgr_verify_tbl_access(
                           sess_hdl, device_id, sel_tbl_hdl, true))) {
    pipe_mgr_api_exit(sess_hdl);
    return ret;
  }

  ret = rmt_sel_grp_mbrs_get(device_id,
                             sel_tbl_hdl,
                             sel_grp_hdl,
                             mbrs_size,
                             mbrs,
                             enable,
                             mbrs_populated);

  if (!pipe_mgr_sess_in_batch(sess_hdl) && !pipe_mgr_sess_in_txn(sess_hdl)) {
    pipe_mgr_sm_release(sess_hdl);
  }
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

/* API function to disable a group member of a selection table */
pipe_status_t pipe_mgr_sel_grp_mbr_disable(pipe_sess_hdl_t sess_hdl,
                                           bf_dev_id_t device_id,
                                           pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                           pipe_sel_grp_hdl_t sel_grp_hdl,
                                           pipe_adt_ent_hdl_t adt_ent_hdl,
                                           uint32_t pipe_api_flags) {
  dev_target_t dev_tgt;
  dev_tgt.device_id = device_id;
  dev_tgt.dev_pipe_id = PIPE_GET_HDL_PIPE(adt_ent_hdl);

  pipe_status_t ret = ml_api_prologue_v2(sess_hdl, dev_tgt, sel_tbl_hdl);
  if (PIPE_SUCCESS != ret) return ret;

  /* Prepare flags for the table manager. */
  uint32_t flags = pipe_mgr_sess_in_txn(sess_hdl) ? PIPE_MGR_TBL_API_TXN : 0;
  flags |= pipe_mgr_sess_in_atomic_txn(sess_hdl) ? PIPE_MGR_TBL_API_ATOM : 0;

  /* Issue the placement function. */
  pipe_mgr_sel_move_list_t *move_list = NULL;
  ret = rmt_sel_grp_mbr_disable(sess_hdl,
                                device_id,
                                sel_tbl_hdl,
                                sel_grp_hdl,
                                adt_ent_hdl,
                                flags,
                                &move_list);

  ret = ml_api_fin(sess_hdl,
                   ret,
                   pipe_api_flags,
                   device_id,
                   sel_tbl_hdl,
                   (pipe_mgr_move_list_t *)move_list);
  return ret;
}

/* API function to re-enable a group member of a selection table */
pipe_status_t pipe_mgr_sel_grp_mbr_enable(pipe_sess_hdl_t sess_hdl,
                                          bf_dev_id_t device_id,
                                          pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                          pipe_sel_grp_hdl_t sel_grp_hdl,
                                          pipe_adt_ent_hdl_t adt_ent_hdl,
                                          uint32_t pipe_api_flags) {
  dev_target_t dev_tgt;
  dev_tgt.device_id = device_id;
  dev_tgt.dev_pipe_id = PIPE_GET_HDL_PIPE(adt_ent_hdl);

  pipe_status_t ret = ml_api_prologue_v2(sess_hdl, dev_tgt, sel_tbl_hdl);
  if (PIPE_SUCCESS != ret) return ret;

  /* Prepare flags for the table manager. */
  uint32_t flags = pipe_mgr_sess_in_txn(sess_hdl) ? PIPE_MGR_TBL_API_TXN : 0;
  flags |= pipe_mgr_sess_in_atomic_txn(sess_hdl) ? PIPE_MGR_TBL_API_ATOM : 0;

  /* Issue the placement function. */
  pipe_mgr_sel_move_list_t *move_list = NULL;
  ret = rmt_sel_grp_mbr_enable(sess_hdl,
                               device_id,
                               sel_tbl_hdl,
                               sel_grp_hdl,
                               adt_ent_hdl,
                               flags,
                               &move_list);

  ret = ml_api_fin(sess_hdl,
                   ret,
                   pipe_api_flags,
                   device_id,
                   sel_tbl_hdl,
                   (pipe_mgr_move_list_t *)move_list);
  return ret;
}

/* API function to get the current state of a selection member */
pipe_status_t pipe_mgr_sel_grp_mbr_state_get(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    pipe_adt_ent_hdl_t adt_ent_hdl,
    enum pipe_mgr_grp_mbr_state_e *mbr_state_p) {
  dev_target_t dev_tgt;
  dev_tgt.device_id = device_id;
  dev_tgt.dev_pipe_id = PIPE_GET_HDL_PIPE(adt_ent_hdl);
  pipe_status_t ret;

  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }
  if (PIPE_SUCCESS != (ret = pipe_mgr_verify_pipe_tbl_access(
                           sess_hdl, dev_tgt, sel_tbl_hdl, true))) {
    pipe_mgr_api_exit(sess_hdl);
    return ret;
  }
  ret = rmt_sel_grp_mbr_state_get(
      device_id, sel_tbl_hdl, sel_grp_hdl, adt_ent_hdl, mbr_state_p);

  if (!pipe_mgr_sess_in_batch(sess_hdl) && !pipe_mgr_sess_in_txn(sess_hdl)) {
    pipe_mgr_sm_release(sess_hdl);
  }

  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t pipe_mgr_sel_grp_id_get(pipe_sess_hdl_t sess_hdl,
                                      dev_target_t dev_tgt,
                                      pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                      pipe_sel_grp_hdl_t sel_grp_hdl,
                                      pipe_sel_grp_id_t *sel_grp_id) {
  pipe_status_t ret = PIPE_SUCCESS;
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }
  if (PIPE_SUCCESS != (ret = pipe_mgr_verify_pipe_tbl_access(
                           sess_hdl, dev_tgt, sel_tbl_hdl, true))) {
    pipe_mgr_api_exit(sess_hdl);
    return ret;
  }
  ret = pipe_mgr_sel_get_grp_id_hdl_int(
      dev_tgt, sel_tbl_hdl, 0, sel_grp_hdl, NULL, sel_grp_id);

  if (!pipe_mgr_sess_in_batch(sess_hdl) && !pipe_mgr_sess_in_txn(sess_hdl)) {
    pipe_mgr_sm_release(sess_hdl);
  }

  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t pipe_mgr_sel_grp_hdl_get(pipe_sess_hdl_t sess_hdl,
                                       dev_target_t dev_tgt,
                                       pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                       pipe_sel_grp_hdl_t sel_grp_id,
                                       pipe_sel_grp_id_t *sel_grp_hdl) {
  pipe_status_t ret = PIPE_SUCCESS;
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }
  if (PIPE_SUCCESS != (ret = pipe_mgr_verify_pipe_tbl_access(
                           sess_hdl, dev_tgt, sel_tbl_hdl, true))) {
    pipe_mgr_api_exit(sess_hdl);
    return ret;
  }
  ret = pipe_mgr_sel_get_grp_id_hdl_int(
      dev_tgt, sel_tbl_hdl, sel_grp_id, 0, sel_grp_hdl, NULL);

  if (!pipe_mgr_sess_in_batch(sess_hdl) && !pipe_mgr_sess_in_txn(sess_hdl)) {
    pipe_mgr_sm_release(sess_hdl);
  }

  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t pipe_mgr_sel_grp_mbr_get_from_hash(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_sel_grp_hdl_t grp_hdl,
    uint8_t *hash,
    uint32_t hash_len,
    pipe_adt_ent_hdl_t *adt_ent_hdl_p){
    RMT_API(sess_hdl,
            0,
            pipe_mgr_verify_tbl_access(sess_hdl, dev_id, sel_tbl_hdl, true),
            rmt_sel_grp_mbr_get_from_hash(
                dev_id, sel_tbl_hdl, grp_hdl, hash, hash_len, adt_ent_hdl_p))}

/*!
 * API function to set the fallback member
 */
pipe_status_t pipe_mgr_sel_fallback_mbr_set(pipe_sess_hdl_t sess_hdl,
                                            dev_target_t dev_tgt,
                                            pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                            pipe_adt_ent_hdl_t adt_ent_hdl,
                                            uint32_t pipe_api_flags) {
  pipe_status_t ret = ml_api_prologue_v2(sess_hdl, dev_tgt, sel_tbl_hdl);
  if (PIPE_SUCCESS != ret) return ret;

  /* Prepare flags for the table manager. */
  uint32_t flags = pipe_mgr_sess_in_txn(sess_hdl) ? PIPE_MGR_TBL_API_TXN : 0;
  flags |= pipe_mgr_sess_in_atomic_txn(sess_hdl) ? PIPE_MGR_TBL_API_ATOM : 0;

  /* Issue the placement function. */
  pipe_mgr_sel_move_list_t *move_list = NULL;
  ret = rmt_sel_fallback_mbr_set(
      sess_hdl, dev_tgt, sel_tbl_hdl, adt_ent_hdl, flags, &move_list);

  ret = ml_api_fin(sess_hdl,
                   ret,
                   pipe_api_flags,
                   dev_tgt.device_id,
                   sel_tbl_hdl,
                   (pipe_mgr_move_list_t *)move_list);
  return ret;
}

/*!
 * API function to reset the fallback member
 */
pipe_status_t pipe_mgr_sel_fallback_mbr_reset(pipe_sess_hdl_t sess_hdl,
                                              dev_target_t dev_tgt,
                                              pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                              uint32_t pipe_api_flags) {
  pipe_status_t ret = ml_api_prologue_v2(sess_hdl, dev_tgt, sel_tbl_hdl);
  if (PIPE_SUCCESS != ret) return ret;

  /* Prepare flags for the table manager. */
  uint32_t flags = pipe_mgr_sess_in_txn(sess_hdl) ? PIPE_MGR_TBL_API_TXN : 0;
  flags |= pipe_mgr_sess_in_atomic_txn(sess_hdl) ? PIPE_MGR_TBL_API_ATOM : 0;

  /* Issue the placement function. */
  pipe_mgr_sel_move_list_t *move_list = NULL;
  ret = rmt_sel_fallback_mbr_reset(
      sess_hdl, dev_tgt, sel_tbl_hdl, flags, &move_list);

  ret = ml_api_fin(sess_hdl,
                   ret,
                   pipe_api_flags,
                   dev_tgt.device_id,
                   sel_tbl_hdl,
                   (pipe_mgr_move_list_t *)move_list);
  return ret;
}

/*!
 * Used to get handle for specified entry member index.
 */
pipe_status_t pipe_mgr_adt_ent_hdl_get(pipe_sess_hdl_t shdl,
                                       bf_dev_target_t dev_tgt,
                                       pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                       pipe_adt_mbr_id_t mbr_id,
                                       pipe_adt_ent_hdl_t *adt_ent_hdl,
                                       bool check_only) {
  pipe_status_t ret = PIPE_SUCCESS;

  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(shdl))) {
    return ret;
  }

  /* Verify that the session can access the table. */
  if (PIPE_SUCCESS != (ret = pipe_mgr_verify_pipe_tbl_access(
                           shdl, dev_tgt, adt_tbl_hdl, true))) {
    ml_api_fin(shdl, ret, 0, dev_tgt.device_id, adt_tbl_hdl, NULL);
    return ret;
  }

  ret = pipe_mgr_adt_get_mbr_id_hdl_int(
      dev_tgt, adt_tbl_hdl, mbr_id, 0, adt_ent_hdl, NULL, NULL, NULL);
  if (check_only) {
    // For "add or mod" API this can fail and it should not impact anything else
    ml_api_fin(shdl, PIPE_SUCCESS, 0, dev_tgt.device_id, adt_tbl_hdl, NULL);
  } else {
    ret = ml_api_fin(shdl, ret, 0, dev_tgt.device_id, adt_tbl_hdl, NULL);
  }
  return ret;
}

/*!
 * Used to get entry member index for specified entry handle.
 */
pipe_status_t pipe_mgr_adt_mbr_id_get(pipe_sess_hdl_t shdl,
                                      bf_dev_id_t dev_id,
                                      pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                      pipe_adt_ent_hdl_t ent_hdl,
                                      pipe_adt_mbr_id_t *adt_mbr_id,
                                      bf_dev_pipe_t *adt_mbr_pipe) {
  pipe_status_t ret = PIPE_SUCCESS;

  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(shdl))) {
    return ret;
  }

  /* Note that for symmetric tables the pipe here would be set to zero instead
   * of BF_DEV_PIPE_ALL, but the verify_pipe_tbl_access function can handle
   * that. */
  bf_dev_target_t dev_tgt;
  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = PIPE_GET_HDL_PIPE(ent_hdl);

  /* Verify that the session can access the table. */
  if (PIPE_SUCCESS != (ret = pipe_mgr_verify_pipe_tbl_access(
                           shdl, dev_tgt, adt_tbl_hdl, true))) {
    ml_api_fin(shdl, ret, 0, dev_id, adt_tbl_hdl, NULL);
    return ret;
  }

  ret = pipe_mgr_adt_get_mbr_id_hdl_int(
      dev_tgt, adt_tbl_hdl, 0, ent_hdl, NULL, adt_mbr_id, adt_mbr_pipe, NULL);
  ret = ml_api_fin(shdl, ret, 0, dev_id, adt_tbl_hdl, NULL);
  return ret;
}

/*!
 * Used to get entry data and handle for specified member id.
 */
pipe_status_t pipe_mgr_adt_ent_data_get(pipe_sess_hdl_t shdl,
                                        bf_dev_target_t dev_tgt,
                                        pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                        pipe_adt_mbr_id_t mbr_id,
                                        pipe_adt_ent_hdl_t *adt_ent_hdl,
                                        pipe_mgr_adt_ent_data_t *ent_data) {
  pipe_status_t ret = PIPE_SUCCESS;

  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(shdl))) {
    return ret;
  }

  /* Verify that the session can access the table. */
  if (PIPE_SUCCESS != (ret = pipe_mgr_verify_pipe_tbl_access(
                           shdl, dev_tgt, adt_tbl_hdl, true))) {
    ml_api_fin(shdl, ret, 0, dev_tgt.device_id, adt_tbl_hdl, NULL);
    return ret;
  }

  ret = pipe_mgr_adt_get_mbr_id_hdl_int(
      dev_tgt, adt_tbl_hdl, mbr_id, 0, adt_ent_hdl, NULL, NULL, ent_data);
  ret = ml_api_fin(shdl, ret, 0, dev_tgt.device_id, adt_tbl_hdl, NULL);
  return ret;
}

/*!
 * API function to add a new Action data entry
 */
pipe_status_t pipe_mgr_adt_ent_add(pipe_sess_hdl_t sess_hdl,
                                   dev_target_t dev_tgt,
                                   pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                   pipe_act_fn_hdl_t act_fn_hdl,
                                   pipe_adt_ent_idx_t mbr_id,
                                   pipe_action_spec_t *action_spec,
                                   pipe_adt_ent_hdl_t *adt_ent_hdl_p,
                                   uint32_t pipe_api_flags) {
  pipe_status_t ret = ml_api_prologue_v2(sess_hdl, dev_tgt, adt_tbl_hdl);
  if (PIPE_SUCCESS != ret) return ret;

  /* Prepare flags for the table manager. */
  uint32_t flags = pipe_mgr_sess_in_txn(sess_hdl) ? PIPE_MGR_TBL_API_TXN : 0;
  flags |= pipe_mgr_sess_in_atomic_txn(sess_hdl) ? PIPE_MGR_TBL_API_ATOM : 0;
  flags |= pipe_api_flags & PIPE_FLAG_CACHE_ENT_ID;

  /* Issue the placement function. */
  pipe_mgr_adt_move_list_t *move_list = NULL;
  ret = pipe_mgr_adt_mgr_ent_add(dev_tgt,
                                 adt_tbl_hdl,
                                 action_spec,
                                 act_fn_hdl,
                                 mbr_id,
                                 adt_ent_hdl_p,
                                 flags,
                                 &move_list);

  ret = ml_api_fin(sess_hdl,
                   ret,
                   pipe_api_flags,
                   dev_tgt.device_id,
                   adt_tbl_hdl,
                   (pipe_mgr_move_list_t *)move_list);
  return ret;
}

/* API function to update an action data entry */
pipe_status_t pipe_mgr_adt_ent_set(pipe_sess_hdl_t sess_hdl,
                                   bf_dev_id_t device_id,
                                   pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                   pipe_adt_ent_hdl_t adt_ent_hdl,
                                   pipe_act_fn_hdl_t act_fn_hdl,
                                   pipe_action_spec_t *action_spec,
                                   uint32_t pipe_api_flags) {
  dev_target_t dev_tgt;
  dev_tgt.device_id = device_id;
  dev_tgt.dev_pipe_id = PIPE_GET_HDL_PIPE(adt_ent_hdl);

  pipe_status_t ret = ml_api_prologue_v2(sess_hdl, dev_tgt, adt_tbl_hdl);
  if (PIPE_SUCCESS != ret) return ret;

  /* Prepare flags for the table manager. */
  uint32_t flags = pipe_mgr_sess_in_txn(sess_hdl) ? PIPE_MGR_TBL_API_TXN : 0;
  flags |= pipe_mgr_sess_in_atomic_txn(sess_hdl) ? PIPE_MGR_TBL_API_ATOM : 0;

  /* Issue the placement function. */
  pipe_mgr_adt_move_list_t *move_list = NULL;
  ret = pipe_mgr_adt_mgr_ent_modify(device_id,
                                    adt_tbl_hdl,
                                    adt_ent_hdl,
                                    action_spec,
                                    act_fn_hdl,
                                    flags,
                                    &move_list);

  ret = ml_api_fin(sess_hdl,
                   ret,
                   pipe_api_flags,
                   device_id,
                   adt_tbl_hdl,
                   (pipe_mgr_move_list_t *)move_list);
  return ret;
}

/*!
 * API function to delete a Action data entry
 */
pipe_status_t pipe_mgr_adt_ent_del(pipe_sess_hdl_t sess_hdl,
                                   bf_dev_id_t device_id,
                                   pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                   pipe_adt_ent_hdl_t adt_ent_hdl,
                                   uint32_t pipe_api_flags) {
  dev_target_t dev_tgt;
  dev_tgt.device_id = device_id;
  dev_tgt.dev_pipe_id = PIPE_GET_HDL_PIPE(adt_ent_hdl);

  pipe_status_t ret = ml_api_prologue_v2(sess_hdl, dev_tgt, adt_tbl_hdl);
  if (PIPE_SUCCESS != ret) return ret;

  /* Prepare flags for the table manager. */
  uint32_t flags = pipe_mgr_sess_in_txn(sess_hdl) ? PIPE_MGR_TBL_API_TXN : 0;
  flags |= pipe_mgr_sess_in_atomic_txn(sess_hdl) ? PIPE_MGR_TBL_API_ATOM : 0;

  /* Issue the placement function. */
  pipe_mgr_adt_move_list_t *move_list = NULL;
  ret = pipe_mgr_adt_mgr_ent_del(
      device_id, adt_tbl_hdl, adt_ent_hdl, flags, &move_list);

  ret = ml_api_fin(sess_hdl,
                   ret,
                   pipe_api_flags,
                   device_id,
                   adt_tbl_hdl,
                   (pipe_mgr_move_list_t *)move_list);
  return ret;
}

pipe_status_t pipe_mgr_adt_init_ent_add(pipe_sess_hdl_t shdl,
                                        bf_dev_target_t dev_tgt,
                                        pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                        pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                        pipe_act_fn_hdl_t act_fn_hdl,
                                        pipe_action_spec_t *action_spec,
                                        pipe_adt_ent_hdl_t *adt_ent_hdl_p,
                                        uint32_t pipe_api_flags) {
  if (!adt_ent_hdl_p) return PIPE_INVALID_ARG;
  if (!action_spec) return PIPE_INVALID_ARG;

  pipe_status_t ret = ml_api_prologue_v2(shdl, dev_tgt, adt_tbl_hdl);
  if (PIPE_SUCCESS != ret) return ret;

  /* Issue the placement function. */
  pipe_mgr_adt_move_list_t *move_list = NULL;
  ret = pipe_mgr_adt_add_init_entry(dev_tgt,
                                    mat_tbl_hdl,
                                    adt_tbl_hdl,
                                    action_spec,
                                    act_fn_hdl,
                                    adt_ent_hdl_p,
                                    &move_list);

  ret = ml_api_fin(shdl,
                   ret,
                   pipe_api_flags,
                   dev_tgt.device_id,
                   adt_tbl_hdl,
                   (pipe_mgr_move_list_t *)move_list);
  return ret;
}

/* API function to query a direct stats entry */
pipe_status_t pipe_mgr_mat_ent_direct_stat_query(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_stat_data_t *stat_data_p) {
  dev_target_t dev_tgt;
  dev_tgt.device_id = device_id;
  dev_tgt.dev_pipe_id = PIPE_GET_HDL_PIPE(mat_ent_hdl);
  pipe_status_t ret;
  pipe_stat_tbl_hdl_t stat_tbl_hdl;
  bf_dev_pipe_t pipe_id;
  dev_stage_t stage_id = 0;
  uint32_t ent_idx = 0;

  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }

  if (pipe_mgr_sess_in_batch(sess_hdl)) {
    pipe_mgr_drv_ilist_chkpt(sess_hdl);
  }

  if (PIPE_SUCCESS != (ret = pipe_mgr_verify_pipe_tbl_access(
                           sess_hdl, dev_tgt, mat_tbl_hdl, true))) {
    goto out;
  }

  ret = pipe_mgr_mat_tbl_get_dir_stat_tbl_hdl(
      device_id, mat_tbl_hdl, &stat_tbl_hdl);

  if (ret != PIPE_SUCCESS) {
    goto out;
  }

  PIPE_MGR_MEMSET(stat_data_p, 0, sizeof(pipe_stat_data_t));

  ret = pipe_mgr_mat_ent_get_dir_ent_location(device_id,
                                              mat_tbl_hdl,
                                              mat_ent_hdl,
                                              0,
                                              &pipe_id,
                                              &stage_id,
                                              NULL,
                                              &ent_idx);
  if (ret != PIPE_SUCCESS) {
    goto out;
  }

  ret = rmt_stat_mgr_query_direct_stats(sess_hdl,
                                        device_id,
                                        pipe_id,
                                        mat_tbl_hdl,
                                        mat_ent_hdl,
                                        stat_tbl_hdl,
                                        stat_data_p);
out:
  ret = handleTableApiRsp(sess_hdl, ret, false, __func__, __LINE__);
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

/* API function to set/clear a direct stats entry */
pipe_status_t pipe_mgr_mat_ent_direct_stat_set(pipe_sess_hdl_t sess_hdl,
                                               bf_dev_id_t device_id,
                                               pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                               pipe_mat_ent_hdl_t mat_ent_hdl,
                                               pipe_stat_data_t *stat_data) {
  pipe_status_t ret;
  pipe_stat_tbl_hdl_t stat_tbl_hdl;
  bf_dev_pipe_t pipe_id;
  dev_stage_t stage_id;
  uint32_t ent_idx;
  dev_target_t dev_tgt;
  dev_tgt.device_id = device_id;
  dev_tgt.dev_pipe_id = PIPE_GET_HDL_PIPE(mat_ent_hdl);

  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }

  if (pipe_mgr_sess_in_batch(sess_hdl)) {
    pipe_mgr_drv_ilist_chkpt(sess_hdl);
  }

  if (PIPE_SUCCESS != (ret = pipe_mgr_verify_pipe_tbl_access(
                           sess_hdl, dev_tgt, mat_tbl_hdl, true))) {
    goto out;
  }

  ret = pipe_mgr_mat_tbl_get_dir_stat_tbl_hdl(
      device_id, mat_tbl_hdl, &stat_tbl_hdl);

  if (ret != PIPE_SUCCESS) {
    goto out;
  }

  ret = pipe_mgr_mat_ent_get_dir_ent_location(device_id,
                                              mat_tbl_hdl,
                                              mat_ent_hdl,
                                              0,
                                              &pipe_id,
                                              &stage_id,
                                              NULL,
                                              &ent_idx);

  if (ret != PIPE_SUCCESS) {
    goto out;
  }
  dev_tgt.dev_pipe_id = pipe_id;

  ret = pipe_mgr_stat_mgr_direct_ent_set(sess_hdl,
                                         dev_tgt,
                                         mat_ent_hdl,
                                         stat_tbl_hdl,
                                         stage_id,
                                         ent_idx,
                                         stat_data);
out:
  ret = handleTableApiRsp(sess_hdl, ret, false, __func__, __LINE__);
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

/* API function to load a direct stats entry */
pipe_status_t pipe_mgr_mat_ent_direct_stat_load(pipe_sess_hdl_t sess_hdl,
                                                bf_dev_id_t device_id,
                                                pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                                pipe_mat_ent_hdl_t mat_ent_hdl,
                                                pipe_stat_data_t *stat_data) {
  pipe_status_t ret;
  pipe_stat_tbl_hdl_t stat_tbl_hdl;
  bf_dev_pipe_t pipe_id;
  dev_stage_t stage_id;
  uint32_t ent_idx;
  dev_target_t dev_tgt;
  dev_tgt.device_id = device_id;
  dev_tgt.dev_pipe_id = PIPE_GET_HDL_PIPE(mat_ent_hdl);

  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }

  pipe_mgr_sess_ctx_t *s;
  if (!(s = pipe_mgr_get_sess_ctx(sess_hdl, __func__, __LINE__))) {
    ret = PIPE_SESSION_NOT_FOUND;
    goto done;
  }

  if (pipe_mgr_sess_in_batch(sess_hdl)) {
    pipe_mgr_drv_ilist_chkpt(sess_hdl);
  }

  if (PIPE_SUCCESS != (ret = pipe_mgr_verify_pipe_tbl_access(
                           sess_hdl, dev_tgt, mat_tbl_hdl, true))) {
    goto out;
  }

  ret = pipe_mgr_mat_tbl_get_dir_stat_tbl_hdl(
      device_id, mat_tbl_hdl, &stat_tbl_hdl);

  if (ret != PIPE_SUCCESS) {
    goto out;
  }

  for (uint32_t subindex = 0; ret == PIPE_SUCCESS; ++subindex) {
    ret = pipe_mgr_mat_ent_get_dir_ent_location(device_id,
                                                mat_tbl_hdl,
                                                mat_ent_hdl,
                                                subindex,
                                                &pipe_id,
                                                &stage_id,
                                                NULL,
                                                &ent_idx);

    if (ret != PIPE_SUCCESS) {
      if (subindex != 0) {
        /* We may not have these many subentries */
        ret = PIPE_SUCCESS;
      }
      goto out;
    }
    dev_tgt.device_id = device_id;
    dev_tgt.dev_pipe_id = pipe_id;

    ret = pipe_mgr_stat_mgr_stage_ent_load(
        sess_hdl, dev_tgt, stat_tbl_hdl, stage_id, ent_idx, stat_data);

    if (ret != PIPE_SUCCESS) {
      goto out;
    }
  }
out:
  ret = handleTableApiRsp(sess_hdl, ret, false, __func__, __LINE__);

done:
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

/* API function to query a stats entry */
pipe_status_t pipe_mgr_stat_ent_query(pipe_sess_hdl_t sess_hdl,
                                      dev_target_t dev_tgt,
                                      pipe_stat_tbl_hdl_t stat_tbl_hdl,
                                      pipe_stat_ent_idx_t *stat_ent_idx,
                                      size_t num_entries,
                                      pipe_stat_data_t **stat_data) {
  pipe_status_t ret = PIPE_SUCCESS;
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }

  if (pipe_mgr_sess_in_batch(sess_hdl)) {
    pipe_mgr_drv_ilist_chkpt(sess_hdl);
  }

  if (PIPE_SUCCESS != (ret = pipe_mgr_verify_pipe_tbl_access(
                           sess_hdl, dev_tgt, stat_tbl_hdl, true))) {
    goto out;
  }

  ret = pipe_mgr_stat_mgr_ent_query(
      sess_hdl, dev_tgt, stat_tbl_hdl, stat_ent_idx, num_entries, stat_data);
out:
  ret = handleTableApiRsp(sess_hdl, ret, false, __func__, __LINE__);
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

static void pipe_mgr_stat_set_ent_idx_cnt(bf_dev_pipe_t pipe,
                                          pipe_mgr_stat_tbl_t *tbl,
                                          pipe_mgr_stat_tbl_instance_t *tbl_i,
                                          pipe_stat_ent_idx_t ent_idx,
                                          pipe_stat_data_t local_stat_data) {
  pipe_stat_stage_ent_idx_t stage_ent_idx = 0;
  pipe_status_t status;
  uint8_t stage_id;

  stage_id = pipe_mgr_stat_mgr_ent_get_stage(
      tbl, tbl_i, ent_idx, 0xff, &stage_ent_idx);

  while (stage_id != 0xff) {
    status = pipe_mgr_stat_mgr_set_ent_idx_count(
        tbl, tbl_i, false, pipe, stage_id, stage_ent_idx, &local_stat_data);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Failed to set entry index %d, pipe id %d",
                __func__,
                __LINE__,
                stage_ent_idx,
                pipe);
    }

    stage_id = pipe_mgr_stat_mgr_ent_get_stage(
        tbl, tbl_i, ent_idx, stage_id, &stage_ent_idx);
  }
  return;
}

static void pipe_mgr_stat_table_clear_sw_state(dev_target_t dev_tgt,
                                               pipe_stat_tbl_hdl_t stat_tbl_hdl,
                                               pipe_stat_data_t *stat_data) {
  pipe_mgr_stat_tbl_t *stat_tbl = NULL;
  pipe_stat_data_t zero_stats;
  zero_stats.packets = zero_stats.bytes = 0;

  stat_tbl = pipe_mgr_stat_tbl_get(dev_tgt.device_id, stat_tbl_hdl);
  if (stat_tbl == NULL) {
    LOG_ERROR("%s:%d Stat tbl for device id %d, tbl hdl 0x%x not found",
              __func__,
              __LINE__,
              dev_tgt.device_id,
              stat_tbl_hdl);
    PIPE_MGR_DBGCHK(0);
    return;
  }

  /* We support clearing all instances of an asymmetric table with pipe-all so
   * we will loop over all instances clearing them as we go but skipping over
   * any instances which are not targeted by the dev_tgt. */
  for (uint32_t i = 0; i < stat_tbl->num_instances; i++) {
    pipe_mgr_stat_tbl_instance_t *inst = &stat_tbl->stat_tbl_instances[i];
    if (inst == NULL) {
      PIPE_MGR_DBGCHK(inst);
      continue;
    }
    if (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL &&
        inst->pipe_id != dev_tgt.dev_pipe_id) {
      /* This is not a clear for all instances and this instance isn't the one
       * which was requested to be cleared, skip over it. */
      continue;
    }

    /* Loop over all pipes in the instance, for each pipe loop over all entries.
     * For each entry reset it's count. */
    unsigned pipe_id = 0;
    PIPE_BITMAP_ITER(&inst->pipe_bmp, pipe_id) {
      if (!inst->ent_idx_info || !inst->ent_idx_info[pipe_id]) {
        continue;
      }
      for (uint32_t idx = 0; idx < inst->num_entries; ++idx) {
        /* Update the user set count if one was provided, else reset it to
         * zero. */
        if (stat_data) {
          inst->user_idx_count[idx].packets = stat_data->packets;
          inst->user_idx_count[idx].bytes = stat_data->bytes;
        } else {
          inst->user_idx_count[idx].packets = 0;
          inst->user_idx_count[idx].bytes = 0;
        }
        /* Update the SW shadow of the counter to zero. */
        pipe_mgr_stat_set_ent_idx_cnt(pipe_id, stat_tbl, inst, idx, zero_stats);
      }
    }
  }
}

/* API function to reset an indirectly referenced stat table */
pipe_status_t pipe_mgr_stat_table_reset(pipe_sess_hdl_t shdl,
                                        dev_target_t dev_tgt,
                                        pipe_stat_tbl_hdl_t tbl_hdl,
                                        pipe_stat_data_t *stat_data) {
  pipe_status_t ret = PIPE_SUCCESS;
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(shdl))) {
    return ret;
  }

  if (pipe_mgr_sess_in_txn(shdl)) {
    ret = PIPE_TXN_NOT_SUPPORTED;
    goto done;
  }

  if (PIPE_SUCCESS !=
      (ret = pipe_mgr_verify_pipe_tbl_access(shdl, dev_tgt, tbl_hdl, true))) {
    goto done;
  }

  if (pipe_mgr_sess_in_batch(shdl)) {
    pipe_mgr_drv_ilist_chkpt(shdl);
  }

  /* Call db sync which will clear hw stats.  */
  ret = pipe_mgr_stat_tbl_database_sync(shdl, dev_tgt, tbl_hdl, NULL, NULL);
  if (ret != PIPE_SUCCESS) {
    goto unlock_and_return;
  }

  /* Start the iList to begin the HW clear. */
  ret = pipe_mgr_drv_ilist_push(&shdl, NULL, NULL);
  if (ret != PIPE_SUCCESS) {
    goto unlock_and_return;
  }

  /* Wait for the HW sync to complete. */
  pipe_mgr_stat_mgr_complete_operations(dev_tgt.device_id, tbl_hdl);

  /* Clear the SW count now that the HW is clear and any evictions are
   * processed. */
  pipe_mgr_stat_table_clear_sw_state(dev_tgt, tbl_hdl, stat_data);

unlock_and_return:
  ret = handleTableApiRsp(shdl, ret, false, __func__, __LINE__);
done:
  pipe_mgr_api_exit(shdl);
  return ret;
}

/* API function to set/clear a stats entry */
pipe_status_t pipe_mgr_stat_ent_set(pipe_sess_hdl_t sess_hdl,
                                    dev_target_t dev_tgt,
                                    pipe_stat_tbl_hdl_t stat_tbl_hdl,
                                    pipe_stat_ent_idx_t stat_idx,
                                    pipe_stat_data_t *stat_data) {
  pipe_mgr_sess_ctx_t *s;
  if (!(s = pipe_mgr_get_sess_ctx(sess_hdl, __func__, __LINE__))) {
    return PIPE_SESSION_NOT_FOUND;
  }

  RMT_API(
      sess_hdl,
      0,
      pipe_mgr_verify_pipe_tbl_access(sess_hdl, dev_tgt, stat_tbl_hdl, true),
      pipe_mgr_stat_mgr_ent_set(
          sess_hdl, dev_tgt, stat_tbl_hdl, stat_idx, stat_data););

  return PIPE_SUCCESS;
}

/* API function to load a stats entry (in Hardware) */
pipe_status_t pipe_mgr_stat_ent_load(pipe_sess_hdl_t sess_hdl,
                                     dev_target_t dev_tgt,
                                     pipe_stat_tbl_hdl_t stat_tbl_hdl,
                                     pipe_stat_ent_idx_t stat_idx,
                                     pipe_stat_data_t *stat_data) {
  RMT_API(
      sess_hdl,
      0,
      pipe_mgr_verify_pipe_tbl_access(sess_hdl, dev_tgt, stat_tbl_hdl, true),
      pipe_mgr_stat_mgr_ent_load(
          sess_hdl, dev_tgt, stat_tbl_hdl, stat_idx, stat_data));

  return PIPE_SUCCESS;
}

/* API to trigger a stats database sync on the indirectly referenced
 * stats table.
 */
pipe_status_t pipe_mgr_stat_database_sync(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_stat_tbl_hdl_t stat_tbl_hdl,
    pipe_mgr_stat_tbl_sync_cback_fn cback_fn,
    void *cookie) {
  pipe_status_t ret = PIPE_SUCCESS;
  pipe_status_t api_ret = PIPE_SUCCESS;
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }

  if (pipe_mgr_sess_in_txn(sess_hdl)) {
    ret = PIPE_TXN_NOT_SUPPORTED;
    goto done;
  }

  if (pipe_mgr_sess_in_batch(sess_hdl)) {
    pipe_mgr_drv_ilist_chkpt(sess_hdl);
  }

  if (PIPE_SUCCESS != (ret = pipe_mgr_verify_pipe_tbl_access(
                           sess_hdl, dev_tgt, stat_tbl_hdl, true))) {
    goto out;
  }
  api_ret = pipe_mgr_stat_tbl_database_sync(
      sess_hdl, dev_tgt, stat_tbl_hdl, cback_fn, cookie);
out:
  ret = handleTableApiRsp(sess_hdl, api_ret, 0, __func__, __LINE__);
  if (api_ret == PIPE_SUCCESS && ret == PIPE_SUCCESS &&
      pipe_mgr_sess_in_batch(sess_hdl)) {
    /* Flush the batch to push the sync instruction to hardware */
    pipe_mgr_flush_batch(sess_hdl);
  }
  if (!cback_fn) {
    /* If no callback function is passed, then we make this a blocking call */
    if (api_ret == PIPE_SUCCESS && ret == PIPE_SUCCESS) {
      pipe_mgr_stat_mgr_complete_operations(dev_tgt.device_id, stat_tbl_hdl);
    }
  }
done:
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

/* API to trigger a stats database sync on the directly referenced
 * stats table.
 */
pipe_status_t pipe_mgr_direct_stat_database_sync(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mgr_stat_tbl_sync_cback_fn cback_fn,
    void *cookie) {
  pipe_status_t ret = PIPE_SUCCESS;
  pipe_status_t api_ret = PIPE_SUCCESS;
  pipe_stat_tbl_hdl_t stat_tbl_hdl;

  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }

  if (pipe_mgr_sess_in_txn(sess_hdl)) {
    ret = PIPE_TXN_NOT_SUPPORTED;
    goto done;
  }

  if (pipe_mgr_sess_in_batch(sess_hdl)) {
    pipe_mgr_drv_ilist_chkpt(sess_hdl);
  }

  if (PIPE_SUCCESS != (api_ret = pipe_mgr_verify_pipe_tbl_access(
                           sess_hdl, dev_tgt, mat_tbl_hdl, true))) {
    goto out;
  }

  api_ret = pipe_mgr_mat_tbl_get_dir_stat_tbl_hdl(
      dev_tgt.device_id, mat_tbl_hdl, &stat_tbl_hdl);

  if (api_ret != PIPE_SUCCESS) {
    goto out;
  }

  api_ret = pipe_mgr_stat_tbl_database_sync(
      sess_hdl, dev_tgt, stat_tbl_hdl, cback_fn, cookie);
out:
  ret = handleTableApiRsp(sess_hdl, api_ret, 0, __func__, __LINE__);
  if (api_ret == PIPE_SUCCESS && ret == PIPE_SUCCESS &&
      pipe_mgr_sess_in_batch(sess_hdl)) {
    /* Flush the batch to push the sync instruction to hardware */
    pipe_mgr_flush_batch(sess_hdl);
  }
  if (!cback_fn) {
    /* If no callback function is passed, then we make this a blocking call */
    if (api_ret == PIPE_SUCCESS && ret == PIPE_SUCCESS) {
      pipe_mgr_stat_mgr_complete_operations(dev_tgt.device_id, stat_tbl_hdl);
    }
  }
done:
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

/* API to trigger a stats entry database sync for an indirectly
 * addressed stat table.
 */
pipe_status_t pipe_mgr_stat_ent_database_sync(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_stat_tbl_hdl_t stat_tbl_hdl,
    pipe_stat_ent_idx_t stat_ent_idx) {
  pipe_status_t ret = PIPE_SUCCESS;
  pipe_status_t api_ret = PIPE_SUCCESS;
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }

  if (pipe_mgr_sess_in_txn(sess_hdl)) {
    ret = PIPE_TXN_NOT_SUPPORTED;
    goto done;
  }

  if (pipe_mgr_sess_in_batch(sess_hdl)) {
    pipe_mgr_drv_ilist_chkpt(sess_hdl);
  }

  if (PIPE_SUCCESS != (api_ret = pipe_mgr_verify_pipe_tbl_access(
                           sess_hdl, dev_tgt, stat_tbl_hdl, true))) {
    goto out;
  }

  api_ret =
      pipe_mgr_stat_mgr_stat_ent_database_sync(sess_hdl,
                                               dev_tgt,
                                               0,
                                               PIPE_MAT_ENT_HDL_INVALID_HDL,
                                               stat_tbl_hdl,
                                               stat_ent_idx);
out:
  ret = handleTableApiRsp(sess_hdl, api_ret, 0, __func__, __LINE__);
  if (api_ret == PIPE_SUCCESS && ret == PIPE_SUCCESS &&
      pipe_mgr_sess_in_batch(sess_hdl)) {
    /* Flush the batch to push the sync instruction to hardware */
    pipe_mgr_flush_batch(sess_hdl);
  }
  /* stat_mgr_complete_operations is called to make this call a blocking call
   * since the entry dump instruction is posted to hardware and upon receipt
   * of the response, this call unblocks.
   */
  if (api_ret == PIPE_SUCCESS && ret == PIPE_SUCCESS) {
    pipe_mgr_stat_mgr_complete_operations(dev_tgt.device_id, stat_tbl_hdl);
  }
done:
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

/* API to trigger a stats entry database sync for a MAT entry with directly
 * addressed stats.
 */
pipe_status_t pipe_mgr_direct_stat_ent_database_sync(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl) {
  pipe_stat_tbl_hdl_t stat_tbl_hdl;
  bf_dev_pipe_t pipe_id = 0;
  dev_stage_t stage_id = 0;
  uint32_t ent_idx = 0;
  pipe_status_t ret = PIPE_SUCCESS;
  pipe_status_t api_ret = PIPE_SUCCESS;

  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }

  if (pipe_mgr_sess_in_txn(sess_hdl)) {
    ret = PIPE_TXN_NOT_SUPPORTED;
    goto done;
  }

  if (pipe_mgr_sess_in_batch(sess_hdl)) {
    pipe_mgr_drv_ilist_chkpt(sess_hdl);
  }

  if (PIPE_SUCCESS != (api_ret = pipe_mgr_verify_pipe_tbl_access(
                           sess_hdl, dev_tgt, mat_tbl_hdl, true))) {
    goto out;
  }

  api_ret = pipe_mgr_mat_tbl_get_dir_stat_tbl_hdl(
      dev_tgt.device_id, mat_tbl_hdl, &stat_tbl_hdl);

  if (api_ret != PIPE_SUCCESS) {
    goto out;
  }

  uint32_t subindex = 0;
  for (subindex = 1; api_ret == PIPE_SUCCESS; subindex++) {
    api_ret = pipe_mgr_mat_ent_get_dir_ent_location(dev_tgt.device_id,
                                                    mat_tbl_hdl,
                                                    mat_ent_hdl,
                                                    subindex,
                                                    &pipe_id,
                                                    &stage_id,
                                                    NULL,
                                                    &ent_idx);
    if (api_ret == PIPE_OBJ_NOT_FOUND) {
      api_ret = PIPE_SUCCESS;
      break;
    }

    api_ret = rmt_stat_mgr_direct_stat_ent_database_sync(sess_hdl,
                                                         dev_tgt.device_id,
                                                         mat_tbl_hdl,
                                                         mat_ent_hdl,
                                                         stat_tbl_hdl,
                                                         pipe_id,
                                                         stage_id,
                                                         ent_idx);
  }
  if (api_ret != PIPE_SUCCESS) {
    goto out;
  }

  /* Now the 0th subindex */
  api_ret = pipe_mgr_mat_ent_get_dir_ent_location(dev_tgt.device_id,
                                                  mat_tbl_hdl,
                                                  mat_ent_hdl,
                                                  0,
                                                  &pipe_id,
                                                  &stage_id,
                                                  NULL,
                                                  &ent_idx);

  if (api_ret != PIPE_SUCCESS) {
    goto out;
  }

  api_ret = rmt_stat_mgr_direct_stat_ent_database_sync(sess_hdl,
                                                       dev_tgt.device_id,
                                                       mat_tbl_hdl,
                                                       mat_ent_hdl,
                                                       stat_tbl_hdl,
                                                       pipe_id,
                                                       stage_id,
                                                       ent_idx);

out:
  ret = handleTableApiRsp(sess_hdl, api_ret, 0, __func__, __LINE__);
  if (api_ret == PIPE_SUCCESS && ret == PIPE_SUCCESS &&
      pipe_mgr_sess_in_batch(sess_hdl)) {
    /* Flush the batch to push the sync instruction to hardware */
    pipe_mgr_flush_batch(sess_hdl);
  }
  /* stat_mgr_complete_operations is called to make this call a blocking call
   * since the entry dump instruction is posted to hardware and upon receipt
   * of the response, this call unblocks.
   */
  if (api_ret == PIPE_SUCCESS && ret == PIPE_SUCCESS) {
    pipe_mgr_stat_mgr_complete_operations(dev_tgt.device_id, stat_tbl_hdl);
  }
done:
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

/* API to reset a meter table */
pipe_status_t pipe_mgr_meter_reset(pipe_sess_hdl_t sess_hdl,
                                   dev_target_t dev_tgt,
                                   pipe_meter_tbl_hdl_t meter_tbl_hdl,
                                   uint32_t pipe_api_flags) {
  pipe_status_t ret = ml_api_prologue_v2(sess_hdl, dev_tgt, meter_tbl_hdl);
  if (PIPE_SUCCESS != ret) return ret;

  struct pipe_mgr_meter_op_list_t *l = NULL;
  ret = pipe_mgr_meter_mgr_meter_reset(dev_tgt, meter_tbl_hdl, &l);
  ret = ml_api_fin(sess_hdl,
                   ret,
                   pipe_api_flags,
                   dev_tgt.device_id,
                   meter_tbl_hdl,
                   (pipe_mgr_move_list_t *)l);
  return ret;
}

/* API to reset a lpf table */
pipe_status_t pipe_mgr_lpf_reset(pipe_sess_hdl_t sess_hdl,
                                 dev_target_t dev_tgt,
                                 pipe_lpf_tbl_hdl_t lpf_tbl_hdl,
                                 uint32_t pipe_api_flags) {
  pipe_status_t ret = ml_api_prologue_v2(sess_hdl, dev_tgt, lpf_tbl_hdl);
  if (PIPE_SUCCESS != ret) return ret;

  struct pipe_mgr_meter_op_list_t *l = NULL;
  ret = pipe_mgr_meter_mgr_lpf_reset(dev_tgt, lpf_tbl_hdl, &l);
  ret = ml_api_fin(sess_hdl,
                   ret,
                   pipe_api_flags,
                   dev_tgt.device_id,
                   lpf_tbl_hdl,
                   (pipe_mgr_move_list_t *)l);
  return ret;
}

/* API to reset a wred table */
pipe_status_t pipe_mgr_wred_reset(pipe_sess_hdl_t sess_hdl,
                                  dev_target_t dev_tgt,
                                  pipe_wred_tbl_hdl_t wred_tbl_hdl,
                                  uint32_t pipe_api_flags) {
  pipe_status_t ret = ml_api_prologue_v2(sess_hdl, dev_tgt, wred_tbl_hdl);
  if (PIPE_SUCCESS != ret) return ret;

  struct pipe_mgr_meter_op_list_t *l = NULL;
  ret = pipe_mgr_meter_mgr_wred_reset(dev_tgt, wred_tbl_hdl, &l);
  ret = ml_api_fin(sess_hdl,
                   ret,
                   pipe_api_flags,
                   dev_tgt.device_id,
                   wred_tbl_hdl,
                   (pipe_mgr_move_list_t *)l);
  return ret;
}

/* API to update a meter entry specification */
pipe_status_t pipe_mgr_meter_ent_set(pipe_sess_hdl_t sess_hdl,
                                     dev_target_t dev_tgt,
                                     pipe_meter_tbl_hdl_t meter_tbl_hdl,
                                     pipe_meter_idx_t meter_idx,
                                     pipe_meter_spec_t *meter_spec,
                                     uint32_t pipe_api_flags) {
  pipe_status_t ret = ml_api_prologue_v2(sess_hdl, dev_tgt, meter_tbl_hdl);
  if (PIPE_SUCCESS != ret) return ret;

  struct pipe_mgr_meter_op_list_t *l = NULL;
  ret = pipe_mgr_meter_mgr_meter_ent_set(
      dev_tgt, meter_tbl_hdl, meter_idx, meter_spec, &l);
  ret = ml_api_fin(sess_hdl,
                   ret,
                   pipe_api_flags,
                   dev_tgt.device_id,
                   meter_tbl_hdl,
                   (pipe_mgr_move_list_t *)l);
  return ret;
}

/* API to update a LPF entry specification */
pipe_status_t pipe_mgr_lpf_ent_set(pipe_sess_hdl_t sess_hdl,
                                   dev_target_t dev_tgt,
                                   pipe_lpf_tbl_hdl_t lpf_tbl_hdl,
                                   pipe_lpf_idx_t lpf_idx,
                                   pipe_lpf_spec_t *lpf_spec,
                                   uint32_t pipe_api_flags) {
  pipe_status_t ret = ml_api_prologue_v2(sess_hdl, dev_tgt, lpf_tbl_hdl);
  if (PIPE_SUCCESS != ret) return ret;

  struct pipe_mgr_meter_op_list_t *l = NULL;
  ret = pipe_mgr_meter_mgr_lpf_ent_set(
      dev_tgt, lpf_tbl_hdl, lpf_idx, lpf_spec, &l);
  ret = ml_api_fin(sess_hdl,
                   ret,
                   pipe_api_flags,
                   dev_tgt.device_id,
                   lpf_tbl_hdl,
                   (pipe_mgr_move_list_t *)l);
  return ret;
}

/* API to update a RED entry specification */
pipe_status_t pipe_mgr_wred_ent_set(pipe_sess_hdl_t sess_hdl,
                                    dev_target_t dev_tgt,
                                    pipe_wred_tbl_hdl_t wred_tbl_hdl,
                                    pipe_wred_idx_t wred_idx,
                                    pipe_wred_spec_t *wred_spec,
                                    uint32_t pipe_api_flags) {
  pipe_status_t ret = ml_api_prologue_v2(sess_hdl, dev_tgt, wred_tbl_hdl);
  if (PIPE_SUCCESS != ret) return ret;

  struct pipe_mgr_meter_op_list_t *l = NULL;
  ret = pipe_mgr_meter_mgr_wred_ent_set(
      dev_tgt, wred_tbl_hdl, wred_idx, wred_spec, &l);
  ret = ml_api_fin(sess_hdl,
                   ret,
                   pipe_api_flags,
                   dev_tgt.device_id,
                   wred_tbl_hdl,
                   (pipe_mgr_move_list_t *)l);
  return ret;
}

/* API to set the time that the model sees, purely for testing purposes. */
pipe_status_t pipe_mgr_model_time_advance(pipe_sess_hdl_t sess_hdl,
                                          bf_dev_id_t device_id,
                                          uint64_t tick_time) {
  pipe_status_t ret = PIPE_SUCCESS;

  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }

  if (pipe_mgr_sess_in_batch(sess_hdl)) {
    pipe_mgr_drv_ilist_chkpt(sess_hdl);
  }

  uint64_t addr;
  char *p = (char *)&addr;
  PIPE_MGR_MEMCPY(p, "IncrTime", 8);
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(device_id);

  if (!dev_info) {
    LOG_ERROR("%s:%d  dev_info is not found for dev_id [%d]",
              __func__,
              __LINE__,
              (int)device_id);

    ret = PIPE_OBJ_NOT_FOUND;
    goto done;
  }

  ret = pipe_mgr_drv_blk_wr_data(
      &sess_hdl, dev_info, 8, 1, 1, addr, 1, (uint8_t *)&tick_time);

  ret = handleTableApiRsp(sess_hdl, ret, false, __func__, __LINE__);

done:
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

/* API to set the meter bytecount adjust */
pipe_status_t pipe_mgr_meter_set_bytecount_adjust(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_meter_tbl_hdl_t meter_tbl_hdl,
    int bytecount) {
  pipe_status_t ret = PIPE_SUCCESS;

  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }

  if (pipe_mgr_sess_in_batch(sess_hdl)) {
    pipe_mgr_drv_ilist_chkpt(sess_hdl);
  }

  ret = pipe_mgr_meter_tbl_bytecount_adjust_set(
      sess_hdl, dev_tgt, meter_tbl_hdl, bytecount);
  ret = handleTableApiRsp(sess_hdl, ret, false, __func__, __LINE__);

  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

/* API to get the meter bytecount adjust */
pipe_status_t pipe_mgr_meter_get_bytecount_adjust(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_meter_tbl_hdl_t meter_tbl_hdl,
    int *bytecount) {
  pipe_status_t ret = PIPE_SUCCESS;

  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }

  if (pipe_mgr_sess_in_batch(sess_hdl)) {
    pipe_mgr_drv_ilist_chkpt(sess_hdl);
  }

  ret = pipe_mgr_meter_tbl_bytecount_adjust_get(
      dev_tgt, meter_tbl_hdl, bytecount);
  ret = handleTableApiRsp(sess_hdl, ret, false, __func__, __LINE__);

  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t pipe_mgr_meter_read_entry(pipe_sess_hdl_t sess_hdl,
                                        dev_target_t dev_tgt,
                                        pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                        pipe_mat_ent_hdl_t mat_ent_hdl,
                                        pipe_meter_spec_t *meter_spec) {
  pipe_meter_tbl_hdl_t meter_tbl_hdl;
  bf_dev_pipe_t pipe_id = 0;
  dev_stage_t stage_id = 0;
  uint32_t ent_idx = 0;
  pipe_status_t ret = PIPE_SUCCESS;

  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }

  if (pipe_mgr_sess_in_batch(sess_hdl)) {
    pipe_mgr_drv_ilist_chkpt(sess_hdl);
  }

  if (PIPE_SUCCESS != (ret = pipe_mgr_verify_pipe_tbl_access(
                           sess_hdl, dev_tgt, mat_tbl_hdl, true))) {
    goto out;
  }

  ret = pipe_mgr_mat_tbl_get_dir_meter_tbl_hdl(
      dev_tgt.device_id, mat_tbl_hdl, &meter_tbl_hdl);

  if (ret != PIPE_SUCCESS) {
    goto out;
  }
  uint32_t subindex = 0;
  ret = pipe_mgr_mat_ent_get_dir_ent_location(dev_tgt.device_id,
                                              mat_tbl_hdl,
                                              mat_ent_hdl,
                                              subindex,
                                              &pipe_id,
                                              &stage_id,
                                              NULL,
                                              &ent_idx);

  if (ret != PIPE_SUCCESS) {
    goto out;
  }

  ret =
      pipe_mgr_meter_mgr_read_entry_in_stage(dev_tgt,
                                             meter_tbl_hdl,
                                             stage_id,
                                             ent_idx,
                                             (pipe_res_data_spec_t *)meter_spec,
                                             SPEC_TYPE_METER,
                                             false /* from_hw */);
out:
  ret = handleTableApiRsp(sess_hdl, ret, 0, __func__, __LINE__);
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t pipe_mgr_meter_read_entry_idx(pipe_sess_hdl_t sess_hdl,
                                            dev_target_t dev_tgt,
                                            pipe_meter_tbl_hdl_t meter_tbl_hdl,
                                            pipe_meter_idx_t index,
                                            pipe_meter_spec_t *meter_spec,
                                            bool from_hw) {
  RMT_API(
      sess_hdl,
      0,
      pipe_mgr_verify_pipe_tbl_access(sess_hdl, dev_tgt, meter_tbl_hdl, true),
      pipe_mgr_meter_mgr_read_entry(dev_tgt,
                                    meter_tbl_hdl,
                                    index,
                                    (pipe_res_data_spec_t *)meter_spec,
                                    SPEC_TYPE_METER,
                                    from_hw));
}

pipe_status_t pipe_mgr_lpf_read_entry(pipe_sess_hdl_t sess_hdl,
                                      dev_target_t dev_tgt,
                                      pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                      pipe_mat_ent_hdl_t mat_ent_hdl,
                                      pipe_lpf_spec_t *lpf_spec) {
  pipe_meter_tbl_hdl_t meter_tbl_hdl;
  bf_dev_pipe_t pipe_id = 0;
  dev_stage_t stage_id = 0;
  uint32_t ent_idx = 0;
  pipe_status_t ret = PIPE_SUCCESS;

  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }

  if (pipe_mgr_sess_in_batch(sess_hdl)) {
    pipe_mgr_drv_ilist_chkpt(sess_hdl);
  }

  if (PIPE_SUCCESS != (ret = pipe_mgr_verify_pipe_tbl_access(
                           sess_hdl, dev_tgt, mat_tbl_hdl, true))) {
    goto out;
  }

  ret = pipe_mgr_mat_tbl_get_dir_meter_tbl_hdl(
      dev_tgt.device_id, mat_tbl_hdl, &meter_tbl_hdl);

  if (ret != PIPE_SUCCESS) {
    goto out;
  }
  uint32_t subindex = 0;
  ret = pipe_mgr_mat_ent_get_dir_ent_location(dev_tgt.device_id,
                                              mat_tbl_hdl,
                                              mat_ent_hdl,
                                              subindex,
                                              &pipe_id,
                                              &stage_id,
                                              NULL,
                                              &ent_idx);

  if (ret != PIPE_SUCCESS) {
    goto out;
  }

  ret = pipe_mgr_meter_mgr_read_entry_in_stage(dev_tgt,
                                               meter_tbl_hdl,
                                               stage_id,
                                               ent_idx,
                                               (pipe_res_data_spec_t *)lpf_spec,
                                               SPEC_TYPE_LPF,
                                               false /* from_hw */);
out:
  ret = handleTableApiRsp(sess_hdl, ret, 0, __func__, __LINE__);
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t pipe_mgr_lpf_read_entry_idx(pipe_sess_hdl_t sess_hdl,
                                          dev_target_t dev_tgt,
                                          pipe_meter_tbl_hdl_t meter_tbl_hdl,
                                          pipe_lpf_idx_t index,
                                          pipe_lpf_spec_t *lpf_spec,
                                          bool from_hw) {
  RMT_API(
      sess_hdl,
      0,
      pipe_mgr_verify_pipe_tbl_access(sess_hdl, dev_tgt, meter_tbl_hdl, true),
      pipe_mgr_meter_mgr_read_entry(dev_tgt,
                                    meter_tbl_hdl,
                                    index,
                                    (pipe_res_data_spec_t *)lpf_spec,
                                    SPEC_TYPE_LPF,
                                    from_hw));
}

pipe_status_t pipe_mgr_wred_read_entry(pipe_sess_hdl_t sess_hdl,
                                       dev_target_t dev_tgt,
                                       pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                       pipe_mat_ent_hdl_t mat_ent_hdl,
                                       pipe_wred_spec_t *wred_spec) {
  pipe_meter_tbl_hdl_t meter_tbl_hdl;
  bf_dev_pipe_t pipe_id = 0;
  dev_stage_t stage_id = 0;
  uint32_t ent_idx = 0;
  pipe_status_t ret = PIPE_SUCCESS;

  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }

  if (pipe_mgr_sess_in_batch(sess_hdl)) {
    pipe_mgr_drv_ilist_chkpt(sess_hdl);
  }

  if (PIPE_SUCCESS != (ret = pipe_mgr_verify_pipe_tbl_access(
                           sess_hdl, dev_tgt, mat_tbl_hdl, true))) {
    goto out;
  }

  ret = pipe_mgr_mat_tbl_get_dir_meter_tbl_hdl(
      dev_tgt.device_id, mat_tbl_hdl, &meter_tbl_hdl);

  if (ret != PIPE_SUCCESS) {
    goto out;
  }
  uint32_t subindex = 0;
  ret = pipe_mgr_mat_ent_get_dir_ent_location(dev_tgt.device_id,
                                              mat_tbl_hdl,
                                              mat_ent_hdl,
                                              subindex,
                                              &pipe_id,
                                              &stage_id,
                                              NULL,
                                              &ent_idx);

  if (ret != PIPE_SUCCESS) {
    goto out;
  }

  ret =
      pipe_mgr_meter_mgr_read_entry_in_stage(dev_tgt,
                                             meter_tbl_hdl,
                                             stage_id,
                                             ent_idx,
                                             (pipe_res_data_spec_t *)wred_spec,
                                             SPEC_TYPE_WRED,
                                             false /* from_hw */);
out:
  ret = handleTableApiRsp(sess_hdl, ret, 0, __func__, __LINE__);
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t pipe_mgr_wred_read_entry_idx(pipe_sess_hdl_t sess_hdl,
                                           dev_target_t dev_tgt,
                                           pipe_meter_tbl_hdl_t meter_tbl_hdl,
                                           pipe_wred_idx_t index,
                                           pipe_wred_spec_t *wred_spec,
                                           bool from_hw) {
  RMT_API(
      sess_hdl,
      0,
      pipe_mgr_verify_pipe_tbl_access(sess_hdl, dev_tgt, meter_tbl_hdl, true),
      pipe_mgr_meter_mgr_read_entry(dev_tgt,
                                    meter_tbl_hdl,
                                    index,
                                    (pipe_res_data_spec_t *)wred_spec,
                                    SPEC_TYPE_WRED,
                                    from_hw));
}

bf_dev_pipe_t dev_port_to_pipe_id(uint16_t dev_port_id) {
  return ((dev_port_id >> PIPE_MGR_PORT_TO_PIPE_SHIFT) & 0x3);
}

/* Set scope property */
static pipe_status_t pipe_mgr_tbl_set_property_scope(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    pipe_mgr_tbl_prop_value_t value,
    pipe_mgr_tbl_prop_args_t args) {
  pipe_status_t rc = PIPE_SUCCESS;
  bool symmetric = true;
  scope_num_t num_scopes = 0;
  scope_pipes_t *temp_scope_pipe_bmp = NULL;
  uint32_t i = 0, cntr = 0, num_profile_pipes = 0;
  pipe_mat_tbl_info_t *info = NULL;
  rmt_dev_info_t *dev_info = NULL;
  pipe_bitmap_t pipe_bmp;
  uint32_t scope_value = 0;

  dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("Invalid device %u ", dev_id);
    return PIPE_INVALID_ARG;
  }
  info = pipe_mgr_get_tbl_info(dev_id, tbl_hdl, __func__, __LINE__);
  if (!info) {
    LOG_ERROR(
        "Invalid table_hdl %d (0x%x) on dev %u ", tbl_hdl, tbl_hdl, dev_id);
    return PIPE_INVALID_ARG;
  }

  rc = pipe_mgr_get_pipe_bmp_for_profile(
      dev_info, info->profile_id, &pipe_bmp, __func__, __LINE__);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error getting the pipe-bmp for profile-id %d, dev %d rc 0x%x",
        __func__,
        __LINE__,
        info->profile_id,
        dev_id,
        rc);
    return rc;
  }

  num_profile_pipes = PIPE_BITMAP_COUNT(&pipe_bmp);
  /* In asymmetric case, each scope could have one pipe. Make sure
     the max number of scopes can cover all the pipes.
  */
  if (num_profile_pipes > PIPE_MGR_MAX_USER_DEFINED_SCOPES) {
    LOG_ERROR("%s: Num of pipes in profile %d is greater than max scopes %d ",
              __func__,
              num_profile_pipes,
              PIPE_MGR_ENTRY_SCOPE_USER_DEFINED);
    return PIPE_INVALID_ARG;
  }

  if (sizeof(scope_pipes_t) != sizeof(args.user_defined_entry_scope[0])) {
    LOG_ERROR(
        "%s: Size of pipe_bitmap mismatches in PD (%zd) and pipe_mgr (%zd) ",
        __func__,
        sizeof(args.user_defined_entry_scope[0]),
        sizeof(scope_pipes_t));
    return PIPE_INVALID_ARG;
  }
  /* Allocate memory for storing the scopes */
  temp_scope_pipe_bmp =
      PIPE_MGR_CALLOC(num_profile_pipes, sizeof(scope_pipes_t));
  if (!temp_scope_pipe_bmp) {
    LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  if (value.value == PIPE_MGR_ENTRY_SCOPE_USER_DEFINED) {
    uint32_t pipes = 0;

    symmetric = false;
    scope_value = args.value;
    num_scopes = 0;
    /* Calculate num of scopes */
    for (cntr = 0; cntr < PIPE_MGR_MAX_USER_DEFINED_SCOPES; cntr++) {
      if (args.user_defined_entry_scope[cntr] == 0) {
        continue;
      }
      if (args.user_defined_entry_scope[cntr] &
          ~((1u << dev_info->num_active_pipes) - 1)) {
        LOG_ERROR("%s: User defined Scope set: Invalid pipes specified ",
                  __func__);
        rc = PIPE_INVALID_ARG;
        goto cleanup;
      }
      /* Check for invalid cfg */
      for (i = 0; i < dev_info->num_active_pipes; i++) {
        if (args.user_defined_entry_scope[cntr] & (1u << i)) {
          if (pipes & (1u << i)) {
            LOG_ERROR(
                "%s: User defined Scope set: Pipe %d belongs to multiple "
                "scopes ",
                __func__,
                i);
            rc = PIPE_INVALID_ARG;
            goto cleanup;
          }
          pipes |= (1u << i);
          /* Make sure pipe is in profile */
          if (!PIPE_BITMAP_GET(&pipe_bmp, i)) {
            LOG_ERROR("%s: Pipe %d not part of table profile ", __func__, i);
            rc = PIPE_INVALID_ARG;
            goto cleanup;
          }
        }
      }
      temp_scope_pipe_bmp[num_scopes] = args.user_defined_entry_scope[cntr];
      num_scopes++;
    }

    if (num_scopes == 0) {
      LOG_ERROR("%s: User defined Scope set: Num of scopes specified is zero ",
                __func__);
      rc = PIPE_INVALID_ARG;
      goto cleanup;
    }
  } else if (value.value == PIPE_MGR_ENTRY_SCOPE_SINGLE_PIPELINE) {
    symmetric = false;
    scope_value = 0;
    num_scopes = 0;
    PIPE_BITMAP_ITER(&pipe_bmp, i) {
      temp_scope_pipe_bmp[num_scopes] |= (1 << i);
      num_scopes++;
    }
  } else {
    symmetric = true;
    scope_value = 0;
    /* Use the profile info to get all pipes */
    num_scopes = 1;

    PIPE_BITMAP_ITER(&pipe_bmp, i) { temp_scope_pipe_bmp[0] |= (1 << i); }
  }
  if (num_scopes > num_profile_pipes) {
    LOG_ERROR(
        "%s: User defined Scope set: Num of scopes specified %d is more than "
        "no of pipes %d",
        __func__,
        num_scopes,
        num_profile_pipes);
    rc = PIPE_INVALID_ARG;
    goto cleanup;
  }
  rc = pipe_mgr_tbl_set_scope(sess_hdl,
                              dev_id,
                              tbl_hdl,
                              symmetric,
                              num_scopes,
                              temp_scope_pipe_bmp,
                              true);
  if (rc == PIPE_SUCCESS) {
    info->scope_value = scope_value;
  }

cleanup:
  PIPE_MGR_FREE(temp_scope_pipe_bmp);

  return rc;
}

pipe_status_t pipe_mgr_tbl_set_property(pipe_sess_hdl_t sess_hdl,
                                        bf_dev_id_t dev_id,
                                        pipe_mat_tbl_hdl_t tbl_hdl,
                                        pipe_mgr_tbl_prop_type_t property,
                                        pipe_mgr_tbl_prop_value_t value,
                                        pipe_mgr_tbl_prop_args_t args) {
  pipe_status_t rc = PIPE_SUCCESS;
  bool repeated_notify;
  bool placement_app = false;
  bool duplicate_check_enable = false;

  LOG_TRACE(
      "%s: Invoked for sess %d, dev %d, tbl %#x, property %d, value %d, "
      "args 0x%" PRIx64,
      __func__,
      sess_hdl,
      dev_id,
      tbl_hdl,
      property,
      value.value,
      args.value);

  switch (property) {
    case PIPE_MGR_TABLE_ENTRY_SCOPE:
      rc = pipe_mgr_tbl_set_property_scope(
          sess_hdl, dev_id, tbl_hdl, value, args);

      break;
    case PIPE_MGR_TERN_TABLE_ENTRY_PLACEMENT:
      if (value.value == PIPE_MGR_TERN_ENTRY_PLACEMENT_APP_MANAGED) {
        placement_app = true;
      } else {
        placement_app = false;
      }
      rc = pipe_mgr_tbl_set_placement_mode(
          sess_hdl, dev_id, tbl_hdl, placement_app);
      break;
    case PIPE_MGR_DUPLICATE_ENTRY_CHECK:
      if (value.value == PIPE_MGR_DUPLICATE_ENTRY_CHECK_ENABLE) {
        duplicate_check_enable = true;
      }
      rc = pipe_mgr_tbl_set_duplicate_entry_check(
          sess_hdl, dev_id, tbl_hdl, duplicate_check_enable);
      break;
    case PIPE_MGR_IDLETIME_REPEATED_NOTIFICATION:
      if (value.value == PIPE_MGR_IDLETIME_REPEATED_NOTIFICATION_ENABLE) {
        repeated_notify = true;
      } else {
        repeated_notify = false;
      }
      rc = pipe_mgr_tbl_set_repeated_notify(
          sess_hdl, dev_id, tbl_hdl, repeated_notify);
      break;
    default:
      break;
  }
  return rc;
}

pipe_status_t pipe_mgr_tbl_get_property(pipe_sess_hdl_t sess_hdl,
                                        bf_dev_id_t dev_id,
                                        pipe_mat_tbl_hdl_t tbl_hdl,
                                        pipe_mgr_tbl_prop_type_t property,
                                        pipe_mgr_tbl_prop_value_t *value,
                                        pipe_mgr_tbl_prop_args_t *args) {
  pipe_status_t rc = PIPE_SUCCESS;
  bool repeated_notify = false;
  bool placement_app = false;
  bool duplicate_check_enable = false;
  bool symmetric = false;
  uint32_t scope_value = 0;

  LOG_TRACE("%s: Invoked for sess %d, dev %d, tbl %#x, property %d",
            __func__,
            sess_hdl,
            dev_id,
            tbl_hdl,
            property);

  switch (property) {
    case PIPE_MGR_TABLE_ENTRY_SCOPE:
      rc = pipe_mgr_tbl_get_property_scope(
          sess_hdl, dev_id, tbl_hdl, &symmetric, &scope_value);
      if (symmetric) {
        value->value = PIPE_MGR_ENTRY_SCOPE_ALL_PIPELINES;
        args->value = 0;
      } else if (scope_value) {
        value->value = PIPE_MGR_ENTRY_SCOPE_USER_DEFINED;
        args->value = scope_value;
      } else {
        value->value = PIPE_MGR_ENTRY_SCOPE_SINGLE_PIPELINE;
        args->value = 0;
      }
      break;
    case PIPE_MGR_TERN_TABLE_ENTRY_PLACEMENT:
      rc = pipe_mgr_tbl_get_placement_mode(
          sess_hdl, dev_id, tbl_hdl, &placement_app);
      if (placement_app) {
        value->value = PIPE_MGR_TERN_ENTRY_PLACEMENT_APP_MANAGED;
      } else {
        value->value = PIPE_MGR_TERN_ENTRY_PLACEMENT_DRV_MANAGED;
      }
      args->value = 0;
      break;
    case PIPE_MGR_DUPLICATE_ENTRY_CHECK:
      rc = pipe_mgr_tbl_get_duplicate_entry_check(
          sess_hdl, dev_id, tbl_hdl, &duplicate_check_enable);
      if (duplicate_check_enable) {
        value->value = PIPE_MGR_DUPLICATE_ENTRY_CHECK_ENABLE;
      } else {
        value->value = PIPE_MGR_DUPLICATE_ENTRY_CHECK_DISABLE;
      }
      args->value = 0;
      break;
    case PIPE_MGR_IDLETIME_REPEATED_NOTIFICATION:
      rc = pipe_mgr_tbl_get_repeated_notify(
          sess_hdl, dev_id, tbl_hdl, &repeated_notify);
      if (repeated_notify) {
        value->value = PIPE_MGR_IDLETIME_REPEATED_NOTIFICATION_ENABLE;
      } else {
        value->value = PIPE_MGR_IDLETIME_REPEATED_NOTIFICATION_DISABLE;
      }
      args->value = 0;
      break;
    default:
      break;
  }
  return rc;
}

pipe_status_t pipe_mgr_get_first_entry_handle(pipe_sess_hdl_t sess_hdl,
                                              pipe_mat_tbl_hdl_t tbl_hdl,
                                              dev_target_t dev_tgt,
                                              int *entry_hdl) {
  pipe_status_t ret = PIPE_SUCCESS;
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }
  if (PIPE_SUCCESS != (ret = pipe_mgr_verify_pipe_tbl_access(
                           sess_hdl, dev_tgt, tbl_hdl, true))) {
    pipe_mgr_api_exit(sess_hdl);
    return ret;
  }
  ret = pipe_mgr_tbl_get_first_entry_handle(
      sess_hdl, tbl_hdl, dev_tgt, entry_hdl);
  if (!pipe_mgr_sess_in_batch(sess_hdl) && !pipe_mgr_sess_in_txn(sess_hdl)) {
    pipe_mgr_sm_release(sess_hdl);
  }
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t pipe_mgr_get_next_entry_handles(pipe_sess_hdl_t sess_hdl,
                                              pipe_mat_tbl_hdl_t tbl_hdl,
                                              dev_target_t dev_tgt,
                                              pipe_mat_ent_hdl_t entry_hdl,
                                              int n,
                                              int *next_entry_handles) {
  pipe_status_t ret = PIPE_SUCCESS;
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }
  if (PIPE_SUCCESS != (ret = pipe_mgr_verify_pipe_tbl_access(
                           sess_hdl, dev_tgt, tbl_hdl, true))) {
    pipe_mgr_api_exit(sess_hdl);
    return ret;
  }
  ret = pipe_mgr_tbl_get_next_entry_handles(
      sess_hdl, tbl_hdl, dev_tgt, entry_hdl, n, next_entry_handles);
  if (!pipe_mgr_sess_in_batch(sess_hdl) && !pipe_mgr_sess_in_txn(sess_hdl)) {
    pipe_mgr_sm_release(sess_hdl);
  }
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t pipe_mgr_get_first_group_member(pipe_sess_hdl_t sess_hdl,
                                              pipe_tbl_hdl_t tbl_hdl,
                                              bf_dev_id_t dev_id,
                                              pipe_sel_grp_hdl_t sel_grp_hdl,
                                              pipe_adt_ent_hdl_t *mbr_hdl) {
  pipe_status_t ret = PIPE_SUCCESS;
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }
  if (PIPE_SUCCESS !=
      (ret = pipe_mgr_verify_tbl_access(sess_hdl, dev_id, tbl_hdl, true))) {
    pipe_mgr_api_exit(sess_hdl);
    return ret;
  }
  ret = pipe_sel_get_first_group_member(dev_id, tbl_hdl, sel_grp_hdl, mbr_hdl);
  if (!pipe_mgr_sess_in_batch(sess_hdl) && !pipe_mgr_sess_in_txn(sess_hdl)) {
    pipe_mgr_sm_release(sess_hdl);
  }
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t pipe_mgr_get_next_group_members(
    pipe_sess_hdl_t sess_hdl,
    pipe_tbl_hdl_t tbl_hdl,
    bf_dev_id_t dev_id,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    pipe_adt_ent_hdl_t mbr_hdl,
    int n,
    pipe_adt_ent_hdl_t *next_mbr_hdls) {
  dev_target_t dev_tgt;
  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = PIPE_GET_HDL_PIPE(mbr_hdl);
  pipe_status_t ret;

  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }
  if (PIPE_SUCCESS != (ret = pipe_mgr_verify_pipe_tbl_access(
                           sess_hdl, dev_tgt, tbl_hdl, true))) {
    pipe_mgr_api_exit(sess_hdl);
    return ret;
  }
  ret = pipe_sel_get_next_group_members(
      dev_id, tbl_hdl, sel_grp_hdl, mbr_hdl, n, next_mbr_hdls);
  if (!pipe_mgr_sess_in_batch(sess_hdl) && !pipe_mgr_sess_in_txn(sess_hdl)) {
    pipe_mgr_sm_release(sess_hdl);
  }
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t pipe_mgr_get_word_llp_active_member_count(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_sel_tbl_hdl_t tbl_hdl,
    uint32_t word_index,
    uint32_t *count) {
  pipe_status_t ret = PIPE_SUCCESS;
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }
  if (PIPE_SUCCESS != (ret = pipe_mgr_verify_pipe_tbl_access(
                           sess_hdl, dev_tgt, tbl_hdl, true))) {
    pipe_mgr_api_exit(sess_hdl);
    return ret;
  }
  ret = pipe_sel_get_word_llp_active_member_count(
      dev_tgt, tbl_hdl, word_index, count);
  if (!pipe_mgr_sess_in_batch(sess_hdl) && !pipe_mgr_sess_in_txn(sess_hdl)) {
    pipe_mgr_sm_release(sess_hdl);
  }
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t pipe_mgr_get_word_llp_active_members(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_sel_tbl_hdl_t tbl_hdl,
    uint32_t word_index,
    uint32_t count,
    pipe_adt_ent_hdl_t *mbr_hdls) {
  pipe_status_t ret = PIPE_SUCCESS;
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }
  if (PIPE_SUCCESS != (ret = pipe_mgr_verify_pipe_tbl_access(
                           sess_hdl, dev_tgt, tbl_hdl, true))) {
    pipe_mgr_api_exit(sess_hdl);
    return ret;
  }
  ret = pipe_sel_get_word_llp_active_members(
      dev_tgt, tbl_hdl, word_index, count, mbr_hdls);
  if (!pipe_mgr_sess_in_batch(sess_hdl) && !pipe_mgr_sess_in_txn(sess_hdl)) {
    pipe_mgr_sm_release(sess_hdl);
  }
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t pipe_mgr_get_entry_count(pipe_sess_hdl_t sess_hdl,
                                       dev_target_t dev_tgt,
                                       pipe_mat_tbl_hdl_t tbl_hdl,
                                       bool read_from_hw,
                                       uint32_t *count) {
  pipe_status_t ret = PIPE_SUCCESS;
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }
  if (PIPE_SUCCESS != (ret = pipe_mgr_verify_pipe_tbl_access(
                           sess_hdl, dev_tgt, tbl_hdl, true))) {
    pipe_mgr_api_exit(sess_hdl);
    return ret;
  }
  ret = pipe_mgr_tbl_get_entry_count(dev_tgt, tbl_hdl, read_from_hw, count);
  if (!pipe_mgr_sess_in_batch(sess_hdl) && !pipe_mgr_sess_in_txn(sess_hdl)) {
    pipe_mgr_sm_release(sess_hdl);
  }
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t pipe_mgr_get_reserved_entry_count(pipe_sess_hdl_t sess_hdl,
                                                dev_target_t dev_tgt,
                                                pipe_mat_tbl_hdl_t tbl_hdl,
                                                size_t *count) {
  pipe_status_t ret = PIPE_SUCCESS;
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }
  if (PIPE_SUCCESS != (ret = pipe_mgr_verify_pipe_tbl_access(
                           sess_hdl, dev_tgt, tbl_hdl, true))) {
    pipe_mgr_api_exit(sess_hdl);
    return ret;
  }

  ret = pipe_mgr_tbl_get_reserved_entry_count(dev_tgt, tbl_hdl, count);
  if (!pipe_mgr_sess_in_batch(sess_hdl) && !pipe_mgr_sess_in_txn(sess_hdl)) {
    pipe_mgr_sm_release(sess_hdl);
  }
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t pipe_mgr_get_total_hw_entry_count(pipe_sess_hdl_t sess_hdl,
                                                dev_target_t dev_tgt,
                                                pipe_mat_tbl_hdl_t tbl_hdl,
                                                size_t *count) {
  pipe_status_t ret = PIPE_SUCCESS;
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }
  if (PIPE_SUCCESS != (ret = pipe_mgr_verify_pipe_tbl_access(
                           sess_hdl, dev_tgt, tbl_hdl, true))) {
    pipe_mgr_api_exit(sess_hdl);
    return ret;
  }

  ret = pipe_mgr_tbl_get_total_hw_entry_count(dev_tgt, tbl_hdl, count);
  if (!pipe_mgr_sess_in_batch(sess_hdl) && !pipe_mgr_sess_in_txn(sess_hdl)) {
    pipe_mgr_sm_release(sess_hdl);
  }
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t pipe_mgr_get_sel_grp_mbr_count(pipe_sess_hdl_t sess_hdl,
                                             bf_dev_id_t dev_id,
                                             pipe_sel_tbl_hdl_t tbl_hdl,
                                             pipe_sel_grp_hdl_t grp_hdl,
                                             uint32_t *count) {
  pipe_status_t ret = PIPE_SUCCESS;
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }
  if (PIPE_SUCCESS !=
      (ret = pipe_mgr_verify_tbl_access(sess_hdl, dev_id, tbl_hdl, true))) {
    pipe_mgr_api_exit(sess_hdl);
    return ret;
  }
  ret = pipe_mgr_sel_get_group_member_count(dev_id, tbl_hdl, grp_hdl, count);
  if (!pipe_mgr_sess_in_batch(sess_hdl) && !pipe_mgr_sess_in_txn(sess_hdl)) {
    pipe_mgr_sm_release(sess_hdl);
  }
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t pipe_mgr_get_sel_grp_params(pipe_sess_hdl_t sess_hdl,
                                          bf_dev_id_t dev_id,
                                          pipe_sel_tbl_hdl_t tbl_hdl,
                                          pipe_sel_grp_hdl_t grp_hdl,
                                          uint32_t *max_size,
                                          uint32_t *adt_offset) {
  pipe_status_t ret = PIPE_SUCCESS;
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }
  if (PIPE_SUCCESS !=
      (ret = pipe_mgr_verify_tbl_access(sess_hdl, dev_id, tbl_hdl, true))) {
    pipe_mgr_api_exit(sess_hdl);
    return ret;
  }
  ret = pipe_mgr_sel_grp_get_params(
      dev_id, tbl_hdl, grp_hdl, max_size, adt_offset);
  if (!pipe_mgr_sess_in_batch(sess_hdl) && !pipe_mgr_sess_in_txn(sess_hdl)) {
    pipe_mgr_sm_release(sess_hdl);
  }
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

static bf_status_t pipe_mgr_can_get_entry(pipe_sess_hdl_t sess_hdl,
                                          pipe_mat_tbl_hdl_t tbl_hdl,
                                          dev_target_t dev_tgt,
                                          bool from_hw) {
  if (from_hw) {
    if (pipe_mgr_is_device_virtual(dev_tgt.device_id)) {
      LOG_ERROR(
          "%s Cannot get entry from hardware for tbl 0x%x on virtual device %d",
          __func__,
          tbl_hdl,
          dev_tgt.device_id);
      return PIPE_NOT_SUPPORTED;
    }
    if (pipe_mgr_sess_in_txn(sess_hdl)) {
      /* This API cannot be called within a batch or within a transaction if
         reading from hardware */
      LOG_ERROR(
          "%s:%d Get entry from hardware API called within a transaction "
          "is not supported, tbl hdl 0x%x, device id %d",
          __func__,
          __LINE__,
          tbl_hdl,
          dev_tgt.device_id);
      return PIPE_NOT_SUPPORTED;
    }
    if (pipe_mgr_sess_in_batch(sess_hdl)) {
      pipe_mgr_flush_batch(sess_hdl);
    }
    complete_operations(sess_hdl);
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_get_n_next_entries(
    pipe_sess_hdl_t sess_hdl,
    pipe_mat_tbl_hdl_t tbl_hdl,
    dev_target_t dev_tgt,
    pipe_mat_ent_hdl_t entry_hdl,
    size_t n,
    bool from_hw,
    uint32_t *res_get_flags,
    pipe_tbl_match_spec_t *pipe_match_spec,
    pipe_action_spec_t **pipe_action_spec,
    pipe_act_fn_hdl_t *act_fn_hdl,
    pipe_res_get_data_t *res_data,
    pipe_mat_ent_hdl_t *last_ent_hdl,
    uint32_t *num_returned) {
  pipe_status_t ret;

  if (!pipe_action_spec || !act_fn_hdl || !pipe_match_spec || !res_data ||
      !res_get_flags || !num_returned || !last_ent_hdl || n == 0) {
    return PIPE_INVALID_ARG;
  }
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }
  if (PIPE_SUCCESS !=
      (ret = pipe_mgr_can_get_entry(sess_hdl, tbl_hdl, dev_tgt, from_hw))) {
    pipe_mgr_api_exit(sess_hdl);
    return ret;
  }

  pipe_mat_tbl_info_t *mat_tbl_info =
      pipe_mgr_get_tbl_info(dev_tgt.device_id, tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    LOG_ERROR("Error in finding the table info for tbl 0x%x device id %d",
              tbl_hdl,
              dev_tgt.device_id);
    ret = PIPE_OBJ_NOT_FOUND;
    pipe_mgr_api_exit(sess_hdl);
    return ret;
  }

  /* Stateful table sync requires a table level lock instead of a pipe level
   * lock. This restriction could be removed once the Statful API becomes
   * thread-safe in this regard. */
  dev_target_t tmp_tgt;
  tmp_tgt.device_id = dev_tgt.device_id;
  tmp_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;
  /* Sync whole table in case of counter get from HW, even if only 2 entries
     are specified this will be still faster. */
  int sync_stat_table = STAT_TBL_NO_SYNC;
  for (size_t i = 0; i < n; i++) {
    if (res_get_flags[i] & PIPE_RES_GET_FLAG_CNTR && from_hw) {
      sync_stat_table = STAT_TBL_REQ_SYNC;
    }
    /* Ensure the MAT is referencing a stateful table. */
    if (res_get_flags[i] & PIPE_RES_GET_FLAG_STFUL &&
        mat_tbl_info->stful_tbl_ref) {
      tmp_tgt.dev_pipe_id = BF_DEV_PIPE_ALL;
    }
    if (sync_stat_table == STAT_TBL_REQ_SYNC &&
        tmp_tgt.dev_pipe_id == BF_DEV_PIPE_ALL) {
      break;
    }
  }
  if (PIPE_SUCCESS != (ret = pipe_mgr_verify_pipe_tbl_access(
                           sess_hdl, tmp_tgt, tbl_hdl, true))) {
    pipe_mgr_api_exit(sess_hdl);
    return ret;
  }

  int *next_entry_hdls = PIPE_MGR_CALLOC(n, sizeof(int));
  if (next_entry_hdls == NULL) {
    LOG_ERROR("Memory allocation error");
    ret = PIPE_NO_SYS_RESOURCES;
    goto done;
  }

  ret = pipe_mgr_tbl_get_next_entry_handles(
      sess_hdl, tbl_hdl, dev_tgt, entry_hdl, n, next_entry_hdls);
  if (PIPE_SUCCESS != ret) {
    if (PIPE_OBJ_NOT_FOUND != ret) {
      LOG_ERROR("Error in getting entry handles for tbl 0x%x dev id %d",
                tbl_hdl,
                dev_tgt.device_id);
    }
    goto done;
  }

  *num_returned = 0;
  size_t i;
  for (i = 0; i < n; i++) {
    if (next_entry_hdls[i] == -1) break;
    ret = pipe_mgr_tbl_get_entry_hdlr(tbl_hdl,
                                      dev_tgt,
                                      next_entry_hdls[i],
                                      &pipe_match_spec[i],
                                      pipe_action_spec[i],
                                      &act_fn_hdl[i],
                                      from_hw);
    if (PIPE_SUCCESS == ret)
      ret = pipe_mgr_get_entry_res(sess_hdl,
                                   dev_tgt,
                                   mat_tbl_info,
                                   next_entry_hdls[i],
                                   act_fn_hdl[i],
                                   from_hw,
                                   &sync_stat_table,
                                   res_get_flags[i],
                                   &res_data[i]);
    if (PIPE_SUCCESS != ret) break;
  }
  *num_returned = i;
  if (last_ent_hdl) *last_ent_hdl = next_entry_hdls[i - 1];

done:
  if (next_entry_hdls) PIPE_MGR_FREE(next_entry_hdls);
  if (!pipe_mgr_sess_in_batch(sess_hdl) && !pipe_mgr_sess_in_txn(sess_hdl)) {
    pipe_mgr_sm_release(sess_hdl);
  }
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t pipe_mgr_get_entry(pipe_sess_hdl_t sess_hdl,
                                 pipe_mat_tbl_hdl_t tbl_hdl,
                                 dev_target_t dev_tgt,
                                 pipe_mat_ent_hdl_t entry_hdl,
                                 pipe_tbl_match_spec_t *pipe_match_spec,
                                 pipe_action_spec_t *pipe_action_spec,
                                 pipe_act_fn_hdl_t *act_fn_hdl,
                                 bool from_hw,
                                 uint32_t res_get_flags,
                                 pipe_res_get_data_t *res_data) {
  pipe_status_t ret;

  if (!pipe_action_spec || !pipe_match_spec || !act_fn_hdl)
    return PIPE_INVALID_ARG;
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }
  if (PIPE_SUCCESS !=
      (ret = pipe_mgr_can_get_entry(sess_hdl, tbl_hdl, dev_tgt, from_hw))) {
    pipe_mgr_api_exit(sess_hdl);
    return ret;
  }

  pipe_mat_tbl_info_t *mat_tbl_info =
      pipe_mgr_get_tbl_info(dev_tgt.device_id, tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    LOG_ERROR("Error in finding the table info for tbl 0x%x device id %d",
              tbl_hdl,
              dev_tgt.device_id);
    ret = PIPE_OBJ_NOT_FOUND;
    pipe_mgr_api_exit(sess_hdl);
    return ret;
  }

  /* Stateful table sync requires a table level lock instead of a pipe level
   * lock. This restriction could be removed once the Statful API becomes
   * thread-safe in this regard. */
  dev_target_t tmp_tgt;
  tmp_tgt.device_id = dev_tgt.device_id;

  /* Ensure the MAT is referencing a stateful table. */
  if (res_get_flags & PIPE_RES_GET_FLAG_STFUL && mat_tbl_info->stful_tbl_ref)
    tmp_tgt.dev_pipe_id = BF_DEV_PIPE_ALL;
  else
    tmp_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

  if (PIPE_SUCCESS != (ret = pipe_mgr_verify_pipe_tbl_access(
                           sess_hdl, tmp_tgt, tbl_hdl, true))) {
    pipe_mgr_api_exit(sess_hdl);
    return ret;
  }

  /* We need the action function handle to get the resources so always call the
   * entry get, but switch it to a sw get if the entry wasn't requested. */
  bool entry_get_from_hw = from_hw && (res_get_flags & PIPE_RES_GET_FLAG_ENTRY);
  ret = pipe_mgr_tbl_get_entry_hdlr(tbl_hdl,
                                    dev_tgt,
                                    entry_hdl,
                                    pipe_match_spec,
                                    pipe_action_spec,
                                    act_fn_hdl,
                                    entry_get_from_hw);
  int sync_stat_tbl = STAT_TBL_NO_SYNC;
  if (PIPE_SUCCESS == ret)
    ret = pipe_mgr_get_entry_res(sess_hdl,
                                 dev_tgt,
                                 mat_tbl_info,
                                 entry_hdl,
                                 *act_fn_hdl,
                                 from_hw,
                                 &sync_stat_tbl,
                                 res_get_flags,
                                 res_data);

  if (!pipe_mgr_sess_in_batch(sess_hdl) && !pipe_mgr_sess_in_txn(sess_hdl)) {
    pipe_mgr_sm_release(sess_hdl);
  }

  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t pipe_mgr_get_action_data_entry(
    pipe_adt_tbl_hdl_t tbl_hdl,
    dev_target_t dev_tgt,
    pipe_adt_ent_hdl_t entry_hdl,
    pipe_action_data_spec_t *pipe_action_data_spec,
    pipe_act_fn_hdl_t *act_fn_hdl,
    bool from_hw) {
  return pipe_mgr_tbl_get_action_data_hdlr(
      tbl_hdl, dev_tgt, entry_hdl, pipe_action_data_spec, act_fn_hdl, from_hw);
}

/* Idle timer stuff below */
/* Set the Idle timeout TTL for a given match entry */
pipe_status_t pipe_mgr_mat_ent_set_idle_ttl(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    uint32_t ttl, /*< TTL value in msecs */
    uint32_t pipe_api_flags,
    bool reset) {
  dev_target_t dev_tgt;
  dev_tgt.device_id = device_id;
  dev_tgt.dev_pipe_id = PIPE_GET_HDL_PIPE(mat_ent_hdl);

  RMT_API(sess_hdl,
          pipe_api_flags,
          pipe_mgr_verify_pipe_tbl_access(sess_hdl, dev_tgt, mat_tbl_hdl, true),
          rmt_mat_ent_set_idle_ttl(sess_hdl,
                                   device_id,
                                   mat_tbl_hdl,
                                   mat_ent_hdl,
                                   ttl, /*< TTL value in msecs */
                                   flags,
                                   reset));
}

pipe_status_t pipe_mgr_mat_ent_reset_idle_ttl(pipe_sess_hdl_t sess_hdl,
                                              bf_dev_id_t device_id,
                                              pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                              pipe_mat_ent_hdl_t mat_ent_hdl) {
  dev_target_t dev_tgt;
  dev_tgt.device_id = device_id;
  dev_tgt.dev_pipe_id = PIPE_GET_HDL_PIPE(mat_ent_hdl);

  RMT_API(sess_hdl,
          0,
          pipe_mgr_verify_pipe_tbl_access(sess_hdl, dev_tgt, mat_tbl_hdl, true),
          rmt_mat_ent_reset_idle_ttl(
              sess_hdl, device_id, mat_tbl_hdl, mat_ent_hdl));
}

/* Configure idle timeout at table level */

pipe_status_t pipe_mgr_idle_get_params(pipe_sess_hdl_t sess_hdl,
                                       bf_dev_id_t device_id,
                                       pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                       pipe_idle_time_params_t *params) {
  RMT_API(sess_hdl,
          0,
          pipe_mgr_verify_tbl_access(sess_hdl, device_id, mat_tbl_hdl, true),
          rmt_idle_params_get(device_id, mat_tbl_hdl, params));
}

pipe_status_t pipe_mgr_idle_set_params(pipe_sess_hdl_t sess_hdl,
                                       bf_dev_id_t device_id,
                                       pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                       pipe_idle_time_params_t params) {
  RMT_API(sess_hdl,
          0,
          pipe_mgr_verify_tbl_access(sess_hdl, device_id, mat_tbl_hdl, true),
          rmt_idle_params_set(sess_hdl, device_id, mat_tbl_hdl, params));
}

pipe_status_t pipe_mgr_idle_tmo_set_enable(pipe_sess_hdl_t sess_hdl,
                                           bf_dev_id_t device_id,
                                           pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                           bool enable) {
  if (enable) {
    RMT_API(sess_hdl,
            0,
            pipe_mgr_verify_tbl_access(sess_hdl, device_id, mat_tbl_hdl, true),
            rmt_idle_tmo_enable(sess_hdl, device_id, mat_tbl_hdl));
  } else {
    RMT_API(sess_hdl,
            0,
            pipe_mgr_verify_tbl_access(sess_hdl, device_id, mat_tbl_hdl, true),
            rmt_idle_tmo_disable(sess_hdl, device_id, mat_tbl_hdl));
  }
}

pipe_status_t pipe_mgr_idle_tmo_get_enable(pipe_sess_hdl_t sess_hdl,
                                           bf_dev_id_t device_id,
                                           pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                           bool *enable) {
  RMT_API(sess_hdl,
          0,
          pipe_mgr_verify_tbl_access(sess_hdl, device_id, mat_tbl_hdl, true),
          rmt_idle_tmo_enable_get(device_id, mat_tbl_hdl, enable));
}
pipe_status_t pipe_mgr_idle_register_tmo_cb(pipe_sess_hdl_t sess_hdl,
                                            bf_dev_id_t device_id,
                                            pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                            pipe_idle_tmo_expiry_cb cb,
                                            void *client_data) {
  RMT_API(sess_hdl,
          0,
          pipe_mgr_verify_tbl_access(sess_hdl, device_id, mat_tbl_hdl, true),
          rmt_idle_register_tmo_cb(device_id, mat_tbl_hdl, cb, client_data));
}

pipe_status_t pipe_mgr_idle_register_tmo_cb_with_match_spec_copy(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_idle_tmo_expiry_cb_with_match_spec_copy cb,
    void *client_data) {
  RMT_API(sess_hdl,
          0,
          pipe_mgr_verify_tbl_access(sess_hdl, device_id, mat_tbl_hdl, true),
          rmt_idle_register_tmo_cb_with_match_spec_copy(
              device_id, mat_tbl_hdl, cb, client_data));
}

/* The below APIs are used for Poll mode operation only */

/* API function to poll idle timeout data for a table entry */
pipe_status_t pipe_mgr_idle_time_get_hit_state(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_idle_time_hit_state_e *idle_time_data) {
  dev_target_t dev_tgt;
  dev_tgt.device_id = device_id;
  dev_tgt.dev_pipe_id = PIPE_GET_HDL_PIPE(mat_ent_hdl);

  RMT_API(sess_hdl,
          0,
          pipe_mgr_verify_pipe_tbl_access(sess_hdl, dev_tgt, mat_tbl_hdl, true),
          rmt_idle_time_get_hit_state(
              sess_hdl, device_id, mat_tbl_hdl, mat_ent_hdl, idle_time_data));
}

/* API function to set table entry hit state data */
pipe_status_t pipe_mgr_idle_time_set_hit_state(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_idle_time_hit_state_e idle_time_data) {
  dev_target_t dev_tgt;
  dev_tgt.device_id = device_id;
  dev_tgt.dev_pipe_id = PIPE_GET_HDL_PIPE(mat_ent_hdl);

  RMT_API(sess_hdl,
          0,
          pipe_mgr_verify_pipe_tbl_access(sess_hdl, dev_tgt, mat_tbl_hdl, true),
          rmt_idle_time_set_hit_state(
              sess_hdl, device_id, mat_tbl_hdl, mat_ent_hdl, idle_time_data));
}

/* API function that should be called  periodically or on-demand prior to
 * querying for the hit state The function completes asynchronously and the
 * client will be notified of it's completion via the provided callback function
 */
pipe_status_t pipe_mgr_idle_time_update_hit_state(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_idle_tmo_update_complete_cb callback_fn,
    void *cb_data) {
  RMT_API(sess_hdl,
          0,
          pipe_mgr_verify_tbl_access(sess_hdl, device_id, mat_tbl_hdl, true),
          rmt_idle_time_update_hit_state(
              sess_hdl, device_id, mat_tbl_hdl, callback_fn, cb_data));
}

/* The below APIs are used in notify mode */
/* API function to get the current TTL value of the table entry */
pipe_status_t pipe_mgr_mat_ent_get_idle_ttl(pipe_sess_hdl_t sess_hdl,
                                            bf_dev_id_t device_id,
                                            pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                            pipe_mat_ent_hdl_t mat_ent_hdl,
                                            uint32_t *ttl) {
  dev_target_t dev_tgt;
  dev_tgt.device_id = device_id;
  dev_tgt.dev_pipe_id = PIPE_GET_HDL_PIPE(mat_ent_hdl);

  RMT_API(sess_hdl,
          0,
          pipe_mgr_verify_pipe_tbl_access(sess_hdl, dev_tgt, mat_tbl_hdl, true),
          rmt_mat_ent_get_idle_ttl(
              sess_hdl, device_id, mat_tbl_hdl, mat_ent_hdl, ttl));
}

pipe_status_t pipe_stful_ent_set(pipe_sess_hdl_t sess_hdl,
                                 dev_target_t dev_target,
                                 pipe_stful_tbl_hdl_t stful_tbl_hdl,
                                 pipe_stful_mem_idx_t stful_ent_idx,
                                 pipe_stful_mem_spec_t *stful_spec,
                                 uint32_t pipe_api_flags) {
  pipe_status_t ret = ml_api_prologue_v2(sess_hdl, dev_target, stful_tbl_hdl);
  if (PIPE_SUCCESS != ret) return ret;

  struct pipe_mgr_stful_op_list_t *l = NULL;
  ret = pipe_mgr_stful_word_write(
      dev_target, stful_tbl_hdl, stful_ent_idx, stful_spec, &l);
  ret = ml_api_fin(sess_hdl,
                   ret,
                   pipe_api_flags,
                   dev_target.device_id,
                   stful_tbl_hdl,
                   (pipe_mgr_move_list_t *)l);
  return ret;
}

pipe_status_t pipe_stful_database_sync(pipe_sess_hdl_t sess_hdl,
                                       dev_target_t dev_tgt,
                                       pipe_stful_tbl_hdl_t stful_tbl_hdl,
                                       pipe_stful_tbl_sync_cback_fn cback_fn,
                                       void *cookie) {
  uint32_t pipe_api_flags = 0;

  /* Stateful table sync requires a table level lock instead of a pipe level
   * lock. This restriction could be removed once the Statful API becomes
   * thread-safe in this regard. */
  dev_target_t tmp_tgt = {dev_tgt.device_id, BF_DEV_PIPE_ALL};

  RMT_API(
      sess_hdl,
      pipe_api_flags,
      pipe_mgr_verify_pipe_tbl_access(sess_hdl, tmp_tgt, stful_tbl_hdl, true),
      pipe_mgr_stful_tbl_sync(
          sess_hdl, dev_tgt, stful_tbl_hdl, cback_fn, cookie));
}

pipe_status_t pipe_stful_direct_database_sync(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_stful_tbl_sync_cback_fn cback_fn,
    void *cookie) {
  uint32_t pipe_api_flags = 0;

  pipe_status_t ret = PIPE_SUCCESS;
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }

  if (pipe_mgr_sess_in_batch(sess_hdl)) {
    pipe_mgr_drv_ilist_chkpt(sess_hdl);
  }

  /* Stateful table sync requires a table level lock instead of a pipe level
   * lock. This restriction could be removed once the Statful API becomes
   * thread-safe in this regard. */
  dev_target_t tmp_tgt = {dev_tgt.device_id, BF_DEV_PIPE_ALL};
  ret = pipe_mgr_verify_pipe_tbl_access(sess_hdl, tmp_tgt, mat_tbl_hdl, true);
  if (ret != PIPE_SUCCESS) {
    ret = handleTableApiRsp(
        sess_hdl, ret, pipe_api_flags & PIPE_FLAG_SYNC_REQ, __func__, __LINE__);
    pipe_mgr_api_exit(sess_hdl);
    return ret;
  }

  bool isAtom = pipe_mgr_sess_in_atomic_txn(sess_hdl);
  uint32_t flags = pipe_mgr_sess_in_txn(sess_hdl) ? PIPE_MGR_TBL_API_TXN : 0;
  flags |= isAtom ? PIPE_MGR_TBL_API_ATOM : 0;
  bool cb_now = false;

  ret = pipe_mgr_stful_direct_tbl_sync(
      sess_hdl, dev_tgt, mat_tbl_hdl, cback_fn, cookie, &cb_now);
  ret = handleTableApiRsp(
      sess_hdl, ret, pipe_api_flags & PIPE_FLAG_SYNC_REQ, __func__, __LINE__);
  if (PIPE_SUCCESS == ret) {
    if (cb_now && cback_fn) {
      cback_fn(dev_tgt.device_id, cookie);
    }
    /* If no callback was provided make this a blocking call. */
    else if (!cback_fn) {
      ret = pipe_mgr_drv_ilist_rd_cmplt_all(&sess_hdl);
      if (ret != PIPE_SUCCESS) {
        LOG_ERROR(
            "Failed to cmplt direct stateful tbl sync: Dev %d pipe 0x%x tbl "
            "0x%x sts %s",
            dev_tgt.device_id,
            dev_tgt.dev_pipe_id,
            mat_tbl_hdl,
            pipe_str_err(ret));
      }
    }
  }
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t pipe_stful_query_get_sizes(pipe_sess_hdl_t sess_hdl,
                                         bf_dev_id_t device_id,
                                         pipe_stful_tbl_hdl_t stful_tbl_hdl,
                                         int *num_pipes) {
  return pipe_mgr_stful_query_sizes(
      sess_hdl, device_id, stful_tbl_hdl, num_pipes);
}

pipe_status_t pipe_stful_direct_query_get_sizes(pipe_sess_hdl_t sess_hdl,
                                                bf_dev_id_t device_id,
                                                pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                                int *num_pipes) {
  return pipe_mgr_stful_direct_query_sizes(
      sess_hdl, device_id, mat_tbl_hdl, num_pipes);
}

pipe_status_t pipe_stful_ent_query(pipe_sess_hdl_t sess_hdl,
                                   dev_target_t dev_tgt,
                                   pipe_stful_tbl_hdl_t stful_tbl_hdl,
                                   pipe_stful_mem_idx_t stful_ent_idx,
                                   pipe_stful_mem_query_t *stful_query,
                                   uint32_t pipe_api_flags) {
  /* Stateful table sync requires a table level lock instead of a pipe level
   * lock. This restriction could be removed once the Statful API becomes
   * thread-safe in this regard. */
  dev_target_t tmp_tgt = {dev_tgt.device_id, BF_DEV_PIPE_ALL};

  RMT_API(
      sess_hdl,
      pipe_api_flags,
      pipe_mgr_verify_pipe_tbl_access(sess_hdl, tmp_tgt, stful_tbl_hdl, true),
      pipe_mgr_stful_ent_query(sess_hdl,
                               dev_tgt,
                               stful_tbl_hdl,
                               stful_ent_idx,
                               stful_query,
                               pipe_api_flags));
}

pipe_status_t pipe_stful_ent_query_range(pipe_sess_hdl_t sess_hdl,
                                         dev_target_t dev_tgt,
                                         pipe_stful_tbl_hdl_t stful_tbl_hdl,
                                         pipe_stful_mem_idx_t stful_ent_idx,
                                         uint32_t num_indices_to_read,
                                         pipe_stful_mem_query_t *stful_query,
                                         uint32_t *num_indices_read,
                                         uint32_t pipe_api_flags) {
  /* Stateful table sync requires a table level lock instead of a pipe level
   * lock. This restriction could be removed once the Statful API becomes
   * thread-safe in this regard. */
  dev_target_t tmp_tgt = {dev_tgt.device_id, BF_DEV_PIPE_ALL};

  RMT_API(
      sess_hdl,
      pipe_api_flags,
      pipe_mgr_verify_pipe_tbl_access(sess_hdl, tmp_tgt, stful_tbl_hdl, true),
      pipe_mgr_stful_ent_query_range(sess_hdl,
                                     dev_tgt,
                                     stful_tbl_hdl,
                                     stful_ent_idx,
                                     num_indices_to_read,
                                     stful_query,
                                     num_indices_read,
                                     pipe_api_flags));
}

pipe_status_t pipe_stful_direct_ent_query(pipe_sess_hdl_t sess_hdl,
                                          bf_dev_id_t device_id,
                                          pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                          pipe_mat_ent_hdl_t mat_ent_hdl,
                                          pipe_stful_mem_query_t *stful_query,
                                          uint32_t pipe_api_flags) {
  /* Stateful table sync requires a table level lock instead of a pipe level
   * lock. This restriction could be removed once the Statful API becomes
   * thread-safe in this regard. */
  /* dev_target_t dev_tgt = {device_id, PIPE_GET_HDL_PIPE(mat_ent_hdl)}; */
  dev_target_t dev_tgt = {device_id, BF_DEV_PIPE_ALL};

  RMT_API(sess_hdl,
          pipe_api_flags,
          pipe_mgr_verify_pipe_tbl_access(sess_hdl, dev_tgt, mat_tbl_hdl, true),
          pipe_mgr_stful_direct_ent_query(sess_hdl,
                                          device_id,
                                          mat_tbl_hdl,
                                          mat_ent_hdl,
                                          stful_query,
                                          pipe_api_flags));
}

pipe_status_t pipe_stful_table_reset(pipe_sess_hdl_t sess_hdl,
                                     dev_target_t dev_tgt,
                                     pipe_stful_tbl_hdl_t stful_tbl_hdl,
                                     pipe_stful_mem_spec_t *stful_spec) {
  uint32_t pipe_api_flags = 0;
  RMT_API(
      sess_hdl,
      pipe_api_flags,
      pipe_mgr_verify_pipe_tbl_access(sess_hdl, dev_tgt, stful_tbl_hdl, true),
      pipe_mgr_stful_table_reset(sess_hdl, dev_tgt, stful_tbl_hdl, stful_spec));
}

pipe_status_t pipe_stful_table_reset_range(pipe_sess_hdl_t sess_hdl,
                                           dev_target_t dev_tgt,
                                           pipe_stful_tbl_hdl_t stful_tbl_hdl,
                                           pipe_stful_mem_idx_t stful_ent_idx,
                                           uint32_t num_indices,
                                           pipe_stful_mem_spec_t *stful_spec) {
  uint32_t pipe_api_flags = 0;
  RMT_API(
      sess_hdl,
      pipe_api_flags,
      pipe_mgr_verify_pipe_tbl_access(sess_hdl, dev_tgt, stful_tbl_hdl, true),
      pipe_mgr_stful_table_reset_range(sess_hdl,
                                       dev_tgt,
                                       stful_tbl_hdl,
                                       stful_ent_idx,
                                       num_indices,
                                       stful_spec));
}

pipe_status_t pipe_stful_fifo_occupancy(pipe_sess_hdl_t sess_hdl,
                                        dev_target_t dev_tgt,
                                        pipe_stful_tbl_hdl_t stful_tbl_hdl,
                                        int *occupancy) {
  uint32_t pipe_api_flags = 0;
  RMT_API(
      sess_hdl,
      pipe_api_flags,
      pipe_mgr_verify_pipe_tbl_access(sess_hdl, dev_tgt, stful_tbl_hdl, true),
      pipe_mgr_stful_fifo_occupancy(
          sess_hdl, dev_tgt, stful_tbl_hdl, occupancy));
}
pipe_status_t pipe_stful_fifo_reset(pipe_sess_hdl_t sess_hdl,
                                    dev_target_t dev_tgt,
                                    pipe_stful_tbl_hdl_t stful_tbl_hdl) {
  uint32_t pipe_api_flags = 0;
  RMT_API(
      sess_hdl,
      pipe_api_flags,
      pipe_mgr_verify_pipe_tbl_access(sess_hdl, dev_tgt, stful_tbl_hdl, true),
      pipe_mgr_stful_fifo_reset(sess_hdl, dev_tgt, stful_tbl_hdl));
}
pipe_status_t pipe_stful_fifo_enqueue(pipe_sess_hdl_t sess_hdl,
                                      dev_target_t dev_tgt,
                                      pipe_stful_tbl_hdl_t stful_tbl_hdl,
                                      int num_to_enqueue,
                                      pipe_stful_mem_spec_t *values) {
  uint32_t pipe_api_flags = 0;
  RMT_API(
      sess_hdl,
      pipe_api_flags,
      pipe_mgr_verify_pipe_tbl_access(sess_hdl, dev_tgt, stful_tbl_hdl, true),
      pipe_mgr_stful_fifo_enqueue(
          sess_hdl, dev_tgt, stful_tbl_hdl, num_to_enqueue, values));
}
pipe_status_t pipe_stful_fifo_dequeue(pipe_sess_hdl_t sess_hdl,
                                      dev_target_t dev_tgt,
                                      pipe_stful_tbl_hdl_t stful_tbl_hdl,
                                      int num_to_dequeue,
                                      pipe_stful_mem_spec_t *values,
                                      int *num_dequeued) {
  uint32_t pipe_api_flags = 0;
  RMT_API(
      sess_hdl,
      pipe_api_flags,
      pipe_mgr_verify_pipe_tbl_access(sess_hdl, dev_tgt, stful_tbl_hdl, true),
      pipe_mgr_stful_fifo_dequeue(sess_hdl,
                                  dev_tgt,
                                  stful_tbl_hdl,
                                  num_to_dequeue,
                                  values,
                                  num_dequeued));
}

pipe_status_t pipe_stful_param_set(pipe_sess_hdl_t sess_hdl,
                                   dev_target_t dev_tgt,
                                   pipe_tbl_hdl_t stful_tbl_hdl,
                                   pipe_reg_param_hdl_t rp_hdl,
                                   int64_t value) {
  uint32_t pipe_api_flags = 0;
  RMT_API(
      sess_hdl,
      pipe_api_flags,
      pipe_mgr_verify_pipe_tbl_access(sess_hdl, dev_tgt, stful_tbl_hdl, true),
      pipe_mgr_stful_param_set(
          sess_hdl, dev_tgt, stful_tbl_hdl, rp_hdl, value, false));
}

pipe_status_t pipe_stful_param_get(pipe_sess_hdl_t sess_hdl,
                                   dev_target_t dev_tgt,
                                   pipe_tbl_hdl_t stful_tbl_hdl,
                                   pipe_reg_param_hdl_t rp_hdl,
                                   int64_t *value) {
  uint32_t pipe_api_flags = 0;
  RMT_API(
      sess_hdl,
      pipe_api_flags,
      pipe_mgr_verify_pipe_tbl_access(sess_hdl, dev_tgt, stful_tbl_hdl, true),
      pipe_mgr_stful_param_get(
          sess_hdl, dev_tgt, stful_tbl_hdl, rp_hdl, value));
}

pipe_status_t pipe_stful_param_reset(pipe_sess_hdl_t sess_hdl,
                                     dev_target_t dev_tgt,
                                     pipe_tbl_hdl_t stful_tbl_hdl,
                                     pipe_reg_param_hdl_t rp_hdl) {
  uint32_t pipe_api_flags = 0;
  RMT_API(
      sess_hdl,
      pipe_api_flags,
      pipe_mgr_verify_pipe_tbl_access(sess_hdl, dev_tgt, stful_tbl_hdl, true),
      pipe_mgr_stful_param_set(
          sess_hdl, dev_tgt, stful_tbl_hdl, rp_hdl, 0, true));
}

pipe_status_t pipe_stful_param_get_hdl(bf_dev_id_t dev,
                                       const char *name,
                                       pipe_reg_param_hdl_t *hdl) {
  return pipe_mgr_stful_param_get_hdl(dev, name, hdl);
}

#define PVS_API(sess_hdl, devid, fn)                                     \
  {                                                                      \
    pipe_status_t ret = PIPE_SUCCESS;                                    \
    pipe_status_t sts = PIPE_SUCCESS;                                    \
    if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {          \
      return ret;                                                        \
    }                                                                    \
    pipe_mgr_sess_ctx_t *s_;                                             \
    do {                                                                 \
      if (!(s_ = pipe_mgr_get_sess_ctx(sess_hdl, __func__, __LINE__))) { \
        ret = PIPE_SESSION_NOT_FOUND;                                    \
        pipe_mgr_api_exit(sess_hdl);                                     \
        return ret;                                                      \
      }                                                                  \
      if (s_->batchInProg) {                                             \
        pipe_mgr_drv_ilist_chkpt(sess_hdl);                              \
      }                                                                  \
      sts = pipe_mgr_pvs_parser_exclusive_access_start(devid);           \
      if (sts != PIPE_SUCCESS) {                                         \
        pipe_mgr_api_exit(sess_hdl);                                     \
        return sts;                                                      \
      }                                                                  \
      ret = fn;                                                          \
      if (ret == PIPE_SUCCESS) {                                         \
        pipe_mgr_drv_ilist_push(&sess_hdl, NULL, NULL);                  \
      } else if (s_->batchInProg) {                                      \
        pipe_mgr_drv_ilist_rollback(sess_hdl);                           \
      } else {                                                           \
        pipe_mgr_drv_ilist_abort(&sess_hdl);                             \
      }                                                                  \
      sts = pipe_mgr_pvs_parser_exclusive_access_end(devid);             \
      if (sts != PIPE_SUCCESS) {                                         \
        pipe_mgr_api_exit(sess_hdl);                                     \
        return sts;                                                      \
      }                                                                  \
    } while (0);                                                         \
    pipe_mgr_api_exit(sess_hdl);                                         \
    return ret;                                                          \
  }

pipe_status_t pipe_mgr_pvs_set_property(pipe_sess_hdl_t sess_hdl,
                                        bf_dev_id_t dev_id,
                                        pipe_pvs_hdl_t pvs_handle,
                                        pipe_mgr_pvs_prop_type_t property,
                                        pipe_mgr_pvs_prop_value_t value,
                                        pipe_mgr_pvs_prop_args_t args) {
  PVS_API(sess_hdl,
          dev_id,
          pipe_mgr_pvs_parser_property_set(
              sess_hdl, dev_id, pvs_handle, property, value, args));
}

pipe_status_t pipe_mgr_pvs_get_property(pipe_sess_hdl_t sess_hdl,
                                        bf_dev_id_t dev_id,
                                        pipe_pvs_hdl_t pvs_handle,
                                        pipe_mgr_pvs_prop_type_t property,
                                        pipe_mgr_pvs_prop_value_t *value,
                                        pipe_mgr_pvs_prop_args_t args) {
  PVS_API(sess_hdl,
          dev_id,
          pipe_mgr_pvs_parser_property_get(
              sess_hdl, dev_id, pvs_handle, property, value, args));
}

pipe_status_t pipe_mgr_pvs_entry_add(pipe_sess_hdl_t sess_hdl,
                                     bf_dev_id_t devid,
                                     pipe_pvs_hdl_t pvs_handle,
                                     bf_dev_direction_t gress,
                                     bf_dev_pipe_t pipeid,
                                     uint8_t parser_id,
                                     uint32_t parser_value,
                                     uint32_t parser_value_mask,
                                     pipe_pvs_hdl_t *pvs_entry_handle) {
  PVS_API(sess_hdl,
          devid,
          pipe_mgr_pvs_parser_tcam_add(sess_hdl,
                                       devid,
                                       pvs_handle,
                                       gress,
                                       pipeid,
                                       parser_id,
                                       parser_value,
                                       parser_value_mask,
                                       pvs_entry_handle));
}

pipe_status_t pipe_mgr_pvs_entry_modify(pipe_sess_hdl_t sess_hdl,
                                        bf_dev_id_t devid,
                                        pipe_pvs_hdl_t pvs_handle,
                                        pipe_pvs_hdl_t pvs_entry_handle,
                                        uint32_t parser_value,
                                        uint32_t parser_value_mask) {
  PVS_API(sess_hdl,
          devid,
          pipe_mgr_pvs_parser_tcam_modify(sess_hdl,
                                          devid,
                                          pvs_handle,
                                          pvs_entry_handle,
                                          parser_value,
                                          parser_value_mask));
}

pipe_status_t pipe_mgr_pvs_entry_delete(pipe_sess_hdl_t sess_hdl,
                                        bf_dev_id_t devid,
                                        pipe_pvs_hdl_t pvs_handle,
                                        pipe_pvs_hdl_t pvs_entry_handle) {
  PVS_API(sess_hdl,
          devid,
          pipe_mgr_pvs_parser_tcam_delete(
              sess_hdl, devid, pvs_handle, pvs_entry_handle));
}

pipe_status_t pipe_mgr_pvs_table_clear(pipe_sess_hdl_t sess_hdl,
                                       bf_dev_id_t devid,
                                       pipe_pvs_hdl_t pvs_handle,
                                       bf_dev_direction_t gress,
                                       bf_dev_pipe_t pipeid,
                                       uint8_t parser_id) {
  PVS_API(sess_hdl,
          devid,
          pipe_mgr_pvs_parser_tcam_clear(
              sess_hdl, devid, pvs_handle, gress, pipeid, parser_id));
}

pipe_status_t pipe_mgr_pvs_entry_get(pipe_sess_hdl_t sess_hdl,
                                     bf_dev_id_t devid,
                                     pipe_pvs_hdl_t pvs_handle,
                                     pipe_pvs_hdl_t pvs_entry_handle,
                                     uint32_t *parser_value,
                                     uint32_t *parser_value_mask,
                                     uint8_t *entry_gress,
                                     bf_dev_pipe_t *entry_pipe,
                                     uint8_t *entry_parser_id) {
  PVS_API(sess_hdl,
          devid,
          pipe_mgr_pvs_parser_tcam_get(sess_hdl,
                                       devid,
                                       pvs_handle,
                                       pvs_entry_handle,
                                       parser_value,
                                       parser_value_mask,
                                       entry_gress,
                                       entry_pipe,
                                       entry_parser_id));
}

pipe_status_t pipe_mgr_pvs_entry_hw_get(pipe_sess_hdl_t sess_hdl,
                                        bf_dev_id_t devid,
                                        uint8_t gress,
                                        bf_dev_pipe_t pipeid,
                                        uint8_t parser_id,
                                        pipe_pvs_hdl_t pvs_handle,
                                        pipe_pvs_hdl_t pvs_entry_handle,
                                        uint32_t *parser_value,
                                        uint32_t *parser_value_mask) {
  PVS_API(sess_hdl,
          devid,
          pipe_mgr_pvs_parser_tcam_hw_get(sess_hdl,
                                          devid,
                                          gress,
                                          pipeid,
                                          parser_id,
                                          pvs_handle,
                                          pvs_entry_handle,
                                          parser_value,
                                          parser_value_mask));
}

pipe_status_t pipe_mgr_pvs_entry_handle_get(pipe_sess_hdl_t sess_hdl,
                                            bf_dev_id_t devid,
                                            pipe_pvs_hdl_t pvs_handle,
                                            bf_dev_direction_t gress,
                                            bf_dev_pipe_t pipeid,
                                            uint8_t parser_id,
                                            uint32_t parser_value,
                                            uint32_t parser_value_mask,
                                            pipe_pvs_hdl_t *pvs_entry_handle) {
  PVS_API(sess_hdl,
          devid,
          pipe_mgr_pvs_parser_entry_handle_get(sess_hdl,
                                               devid,
                                               pvs_handle,
                                               gress,
                                               pipeid,
                                               parser_id,
                                               parser_value,
                                               parser_value_mask,
                                               pvs_entry_handle));
}

pipe_status_t pipe_mgr_pvs_entry_get_first(pipe_sess_hdl_t sess_hdl,
                                           bf_dev_id_t dev_id,
                                           pipe_pvs_hdl_t pvs_handle,
                                           bf_dev_direction_t gress,
                                           bf_dev_pipe_t pipe_id,
                                           uint8_t parser_id,
                                           pipe_pvs_hdl_t *entry_handle) {
  PVS_API(
      sess_hdl,
      dev_id,
      pipe_mgr_pvs_get_next_handles(
          dev_id, pvs_handle, gress, pipe_id, parser_id, -1, 1, entry_handle));
}

pipe_status_t pipe_mgr_pvs_entry_get_next(pipe_sess_hdl_t sess_hdl,
                                          bf_dev_id_t dev_id,
                                          pipe_pvs_hdl_t pvs_handle,
                                          bf_dev_direction_t gress,
                                          bf_dev_pipe_t pipe_id,
                                          uint8_t parser_id,
                                          pipe_pvs_hdl_t entry_handle,
                                          int n,
                                          pipe_pvs_hdl_t *next_handles) {
  PVS_API(sess_hdl,
          dev_id,
          pipe_mgr_pvs_get_next_handles(dev_id,
                                        pvs_handle,
                                        gress,
                                        pipe_id,
                                        parser_id,
                                        entry_handle,
                                        n,
                                        next_handles));
}

pipe_status_t pipe_mgr_pvs_entry_get_count(pipe_sess_hdl_t sess_hdl,
                                           bf_dev_id_t dev_id,
                                           pipe_pvs_hdl_t pvs_handle,
                                           bf_dev_direction_t gress,
                                           bf_dev_pipe_t pipe_id,
                                           uint8_t parser_id,
                                           bool read_from_hw,
                                           uint32_t *count) {
  PVS_API(
      sess_hdl,
      dev_id,
      pipe_mgr_pvs_get_count(
          dev_id, pvs_handle, gress, pipe_id, parser_id, read_from_hw, count));
}

/* Hash Calculation APIs */
pipe_status_t pipe_mgr_hash_calc_input_set(pipe_sess_hdl_t sess_hdl,
                                           bf_dev_id_t dev_id,
                                           pipe_hash_calc_hdl_t handle,
                                           pipe_fld_lst_hdl_t fl_handle){
    RMT_API(
        sess_hdl,
        0,
        pipe_mgr_verify_hash_calc_access(sess_hdl, dev_id, true),
        pipe_mgr_hash_calc_input_set_ext(sess_hdl, dev_id, handle, fl_handle))}

pipe_status_t pipe_mgr_hash_calc_input_default_set(pipe_sess_hdl_t sess_hdl,
                                                   bf_dev_id_t dev_id,
                                                   pipe_hash_calc_hdl_t handle){
    RMT_API(sess_hdl,
            0,
            pipe_mgr_verify_hash_calc_access(sess_hdl, dev_id, true),
            pipe_mgr_hash_calc_input_default_set_ext(sess_hdl, dev_id, handle))}

pipe_status_t
    pipe_mgr_hash_calc_input_get(pipe_sess_hdl_t sess_hdl,
                                 bf_dev_id_t dev_id,
                                 pipe_hash_calc_hdl_t handle,
                                 pipe_fld_lst_hdl_t *fl_handle){RMT_API(
        sess_hdl,
        0,
        pipe_mgr_verify_hash_calc_access(sess_hdl, dev_id, true),
        pipe_mgr_hash_calc_input_get_ext(sess_hdl, dev_id, handle, fl_handle))}

pipe_status_t pipe_mgr_hash_calc_input_field_attribute_set(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_hash_calc_hdl_t handle,
    pipe_fld_lst_hdl_t fl_handle,
    uint32_t attr_count,
    pipe_hash_calc_input_field_attribute_t *attr_list){
    RMT_API(sess_hdl,
            0,
            pipe_mgr_verify_hash_calc_access(sess_hdl, dev_id, true),
            pipe_mgr_hash_calc_input_field_attribute_set_ext(
                sess_hdl, dev_id, handle, fl_handle, attr_count, attr_list))}

pipe_status_t pipe_mgr_hash_calc_input_field_attribute_get(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_hash_calc_hdl_t handle,
    pipe_fld_lst_hdl_t fl_handle,
    uint32_t max_attr_count,
    pipe_hash_calc_input_field_attribute_t *attr_list,
    uint32_t *num_attr_filled){RMT_API(
    sess_hdl,
    0,
    pipe_mgr_verify_hash_calc_access(sess_hdl, dev_id, true),
    pipe_mgr_hash_calc_input_field_attribute_get_ext(
        dev_id, handle, fl_handle, max_attr_count, attr_list, num_attr_filled))}

pipe_status_t pipe_mgr_hash_calc_input_field_attribute_2_get(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_hash_calc_hdl_t handle,
    pipe_fld_lst_hdl_t fl_handle,
    pipe_hash_calc_input_field_attribute_t **attr_list,
    uint32_t *num_attr_filled){
    RMT_API(sess_hdl,
            0,
            pipe_mgr_verify_hash_calc_access(sess_hdl, dev_id, true),
            pipe_mgr_hash_calc_input_field_attribute_2_get_ext(
                dev_id, handle, fl_handle, attr_list, num_attr_filled))}

pipe_status_t pipe_mgr_hash_calc_attribute_list_destroy(
    pipe_hash_calc_input_field_attribute_t *attr_list) {
  if (attr_list) PIPE_MGR_FREE(attr_list);
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_hash_calc_input_field_attribute_count_get(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_hash_calc_hdl_t handle,
    pipe_fld_lst_hdl_t fl_handle,
    uint32_t *attr_count){
    RMT_API(sess_hdl,
            0,
            pipe_mgr_verify_hash_calc_access(sess_hdl, dev_id, true),
            pipe_mgr_hash_calc_input_field_attribute_count_get_ext(
                dev_id, handle, fl_handle, attr_count))}

pipe_status_t
    pipe_mgr_hash_calc_algorithm_set(pipe_sess_hdl_t sess_hdl,
                                     bf_dev_id_t dev_id,
                                     pipe_hash_calc_hdl_t handle,
                                     pipe_hash_alg_hdl_t al_handle,
                                     const bfn_hash_algorithm_t *algorithm,
                                     uint64_t rotate){
        RMT_API(sess_hdl,
                0,
                pipe_mgr_verify_hash_calc_access(sess_hdl, dev_id, true),
                pipe_mgr_hash_calc_algorithm_set_ext(
                    sess_hdl, dev_id, handle, al_handle, algorithm, rotate))}

pipe_status_t pipe_mgr_hash_calc_algorithm_get(pipe_sess_hdl_t sess_hdl,
                                               bf_dev_id_t dev_id,
                                               pipe_hash_calc_hdl_t handle,
                                               pipe_hash_alg_hdl_t *al_handle,
                                               bfn_hash_algorithm_t *algorithm,
                                               uint64_t *rotate){
    RMT_API(sess_hdl,
            0,
            pipe_mgr_verify_hash_calc_access(sess_hdl, dev_id, true),
            pipe_mgr_hash_calc_algorithm_get_ext(
                sess_hdl, dev_id, handle, al_handle, algorithm, rotate))}

pipe_status_t pipe_mgr_hash_calc_seed_set(pipe_sess_hdl_t sess_hdl,
                                          bf_dev_id_t dev_id,
                                          pipe_hash_calc_hdl_t handle,
                                          pipe_hash_seed_t seed){
    RMT_API(sess_hdl,
            0,
            pipe_mgr_verify_hash_calc_access(sess_hdl, dev_id, true),
            pipe_mgr_hash_calc_seed_set_ext(sess_hdl, dev_id, handle, seed))}

pipe_status_t pipe_mgr_hash_calc_seed_get(pipe_sess_hdl_t sess_hdl,
                                          bf_dev_id_t dev_id,
                                          pipe_hash_calc_hdl_t handle,
                                          pipe_hash_seed_t *seed){
    RMT_API(sess_hdl,
            0,
            pipe_mgr_verify_hash_calc_access(sess_hdl, dev_id, true),
            pipe_mgr_hash_calc_seed_get_ext(sess_hdl, dev_id, handle, seed))}

pipe_status_t pipe_mgr_hash_calc_algorithm_reset(pipe_sess_hdl_t sess_hdl,
                                                 bf_dev_id_t dev_id,
                                                 pipe_hash_calc_hdl_t handle){
    RMT_API(sess_hdl,
            0,
            pipe_mgr_verify_hash_calc_access(sess_hdl, dev_id, true),
            pipe_mgr_hash_calc_algorithm_reset_ext(sess_hdl, dev_id, handle))}

pipe_status_t
    pipe_mgr_hash_calc_calculate_hash_value(pipe_sess_hdl_t sess_hdl,
                                            bf_dev_id_t dev_id,
                                            pipe_hash_calc_hdl_t handle,
                                            uint8_t *stream,
                                            uint32_t stream_len,
                                            uint8_t *hash,
                                            uint32_t hash_len){
        RMT_API(sess_hdl,
                0,
                pipe_mgr_verify_hash_calc_access(sess_hdl, dev_id, true),
                pipe_mgr_hash_calc_calculate_hash_value_ext(
                    dev_id, handle, stream, stream_len, hash, hash_len))}

pipe_status_t pipe_mgr_hash_calc_calculate_hash_value_with_cfg(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_hash_calc_hdl_t handle,
    uint32_t attr_count,
    pipe_hash_calc_input_field_attribute_t *attrs,
    uint32_t hash_len,
    uint8_t *hash){
    RMT_API(sess_hdl,
            0,
            pipe_mgr_verify_hash_calc_access(sess_hdl, dev_id, true),
            pipe_mgr_hash_calc_calculate_hash_value_with_cfg_ext(
                dev_id, handle, attr_count, attrs, hash_len, hash))}

pipe_status_t pipe_mgr_lrn_digest_notification_register(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_fld_lst_hdl_t flow_lrn_fld_lst_hdl,
    pipe_flow_lrn_notify_cb callback_fn,
    void *callback_fn_cookie){
    RMT_API(sess_hdl,
            0,
            pipe_mgr_verify_lrn_ses_access(sess_hdl, device_id, true),
            pipe_mgr_lrn_notification_register(sess_hdl,
                                               device_id,
                                               flow_lrn_fld_lst_hdl,
                                               callback_fn,
                                               callback_fn_cookie))}

pipe_status_t pipe_mgr_lrn_digest_notification_deregister(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_fld_lst_hdl_t flow_lrn_fld_lst_hdl){
    RMT_API(sess_hdl,
            0,
            pipe_mgr_verify_lrn_ses_access(sess_hdl, device_id, true),
            pipe_mgr_lrn_notification_deregister(sess_hdl,
                                                 device_id,
                                                 flow_lrn_fld_lst_hdl))}

pipe_status_t
    pipe_mgr_flow_lrn_notify_ack(pipe_sess_hdl_t sess_hdl,
                                 pipe_fld_lst_hdl_t flow_lrn_fld_lst_hdl,
                                 pipe_flow_lrn_msg_t *pipe_flow_lrn_msg) {
  pipe_status_t ret = pipe_mgr_api_enter(sess_hdl);
  if (PIPE_SUCCESS != ret) {
    return ret;
  }
  if (!pipe_mgr_valid_deviceId(
          pipe_flow_lrn_msg->dev_tgt.device_id, __func__, __LINE__)) {
    /* If an ack comes after the device has already been removed, free
     * all data and return.
     */
    PIPE_MGR_FREE(pipe_flow_lrn_msg->entries);
    PIPE_MGR_FREE(pipe_flow_lrn_msg);
    goto done;
  }

  if (PIPE_SUCCESS !=
      (ret = pipe_mgr_verify_lrn_ses_access(
           sess_hdl, pipe_flow_lrn_msg->dev_tgt.device_id, true))) {
    goto done;
  }

  ret = pipe_mgr_lrn_notify_ack(
      sess_hdl, flow_lrn_fld_lst_hdl, pipe_flow_lrn_msg);

  if (!pipe_mgr_sess_in_batch(sess_hdl) && !pipe_mgr_sess_in_txn(sess_hdl)) {
    pipe_mgr_sm_release(sess_hdl);
  }

done:
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t pipe_mgr_flow_lrn_set_timeout(pipe_sess_hdl_t sess_hdl,
                                            bf_dev_id_t device_id,
                                            uint32_t usecs){
    RMT_API(sess_hdl,
            0,
            pipe_mgr_verify_lrn_ses_access(sess_hdl, device_id, true),
            pipe_mgr_lrn_set_timeout(device_id, usecs))}

pipe_status_t
    pipe_mgr_flow_lrn_get_timeout(bf_dev_id_t device_id, uint32_t *usecs) {
  return pipe_mgr_lrn_get_timeout(device_id, usecs);
}

pipe_status_t pipe_mgr_flow_lrn_set_network_order_digest(bf_dev_id_t device_id,
                                                         bool network_order) {
  return pipe_mgr_lrn_set_network_order_digest(device_id, network_order);
}

pipe_status_t pipe_mgr_inactive_node_delete_set(pipe_sess_hdl_t sess_hdl,
                                                bf_dev_id_t device_id,
                                                bool en) {
  pipe_status_t ret;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(device_id);
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }
  if (!dev_info) {
    LOG_ERROR("%s: Invalid device id %d", __func__, device_id);
    pipe_mgr_api_exit(sess_hdl);
    return PIPE_INVALID_ARG;
  }

  ret = pipe_mgr_alpm_set_inactive_node_delete(en);
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t pipe_mgr_inactive_node_delete_get(pipe_sess_hdl_t sess_hdl,
                                                bf_dev_id_t device_id,
                                                bool *en) {
  if (!en) return PIPE_INVALID_ARG;
  pipe_status_t ret;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(device_id);
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }
  if (!dev_info) {
    LOG_ERROR("%s: Invalid device id %d", __func__, device_id);
    pipe_mgr_api_exit(sess_hdl);
    return PIPE_INVALID_ARG;
  }
  ret = pipe_mgr_alpm_get_inactive_node_delete(en);
  pipe_mgr_api_exit(sess_hdl);
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_selector_tbl_member_order_set(pipe_sess_hdl_t sess_hdl,
                                                     bf_dev_id_t device_id,
                                                     bool en) {
  pipe_status_t ret;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(device_id);
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }
  if (!dev_info) {
    LOG_ERROR("%s: Invalid device id %d", __func__, device_id);
    pipe_mgr_api_exit(sess_hdl);
    return PIPE_INVALID_ARG;
  }

  ret = pipe_mgr_selector_tbl_set_sequence_order(en);
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t pipe_mgr_selector_tbl_member_order_get(pipe_sess_hdl_t sess_hdl,
                                                     bf_dev_id_t device_id,
                                                     bool *en) {
  if (!en) return PIPE_INVALID_ARG;
  pipe_status_t ret;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(device_id);
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }
  if (!dev_info) {
    LOG_ERROR("%s: Invalid device id %d", __func__, device_id);
    pipe_mgr_api_exit(sess_hdl);
    return PIPE_INVALID_ARG;
  }
  ret = pipe_mgr_selector_tbl_get_sequence_order(en);
  pipe_mgr_api_exit(sess_hdl);
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_flow_lrn_set_intr_mode(pipe_sess_hdl_t sess_hdl,
                                              bf_dev_id_t device_id,
                                              bool en) {
  pipe_status_t ret;
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }
  ret = pipe_mgr_flow_lrn_int_enable(device_id, en);
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t pipe_mgr_flow_lrn_get_intr_mode(pipe_sess_hdl_t sess_hdl,
                                              bf_dev_id_t device_id,
                                              bool *en) {
  if (!en) return PIPE_INVALID_ARG;
  pipe_status_t ret;
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }
  *en = pipe_mgr_flow_lrn_is_int_enabled(device_id);
  pipe_mgr_api_exit(sess_hdl);
  return PIPE_SUCCESS;
}

void pipe_mgr_log_stat(void *cookie, bool hadError) {
  pipe_status_t ret;
  rmt_dev_info_t *dev_info;
  rmt_dev_profile_info_t *profile_info;
  pipe_stat_tbl_info_t *stat_info;
  cJSON *dev, *profiles, *profile, *stat_tbls;
  bf_dev_id_t dev_id;
  uint32_t num_profiles, profile_id;
  uint32_t p, i;

  dev = (cJSON *)cookie;
  dev_id = cJSON_GetObjectItem(dev, "dev_id")->valueint;

  if (hadError) {
    LOG_ERROR("%s:%d Stat hardware sync for device %u failed",
              __func__,
              __LINE__,
              dev_id);
    return;
  }

  if (!(dev_info = pipe_mgr_get_dev_info(dev_id))) {
    LOG_ERROR(
        "%s:%d Device info for id %u not found", __func__, __LINE__, dev_id);
    return;
  }

  profiles = cJSON_GetObjectItem(dev, "profiles");
  num_profiles = cJSON_GetArraySize(profiles);
  for (p = 0; p < num_profiles; p++) {
    profile = cJSON_GetArrayItem(profiles, p);
    profile_id = cJSON_GetObjectItem(profile, "profile_id")->valueint;
    profile_info = dev_info->profile_info[profile_id];
    stat_tbls = cJSON_GetObjectItem(profile, "stat_tbls");
    for (stat_info = profile_info->tbl_info_list.stat_tbl_list, i = 0;
         i < profile_info->tbl_info_list.num_stat_tbls;
         ++stat_info, ++i) {
      pipe_mgr_stat_mgr_complete_operations(dev_id, stat_info->handle);
      ret = pipe_mgr_stat_log_state(dev_id, stat_info->handle, stat_tbls);
      if (ret != PIPE_SUCCESS) {
        LOG_ERROR("%s:%d Stat table with handle 0x%x log failed",
                  __func__,
                  __LINE__,
                  stat_info->handle);
        return;
      }
    }
  }

  return;
}

pipe_status_t pipe_mgr_log_cmplt(rmt_dev_info_t *dev_info) {
  pipe_status_t ret = PIPE_SUCCESS;
  pipe_sess_hdl_t sess_hdl = pipe_mgr_get_int_sess_hdl();
  rmt_dev_profile_info_t *profile_info;
  pipe_stat_tbl_info_t *stat_info;
  pipe_stful_tbl_info_t *stful_info;
  uint32_t p, i;

  ret = pipe_mgr_drv_i_list_cmplt_all(&sess_hdl);
  if (ret != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Stat hardware sync cmplt for device %u failed",
              __func__,
              __LINE__,
              dev_info->dev_id);
    return ret;
  }

  // Complete stat and stful tbl logging
  for (p = 0; p < dev_info->num_pipeline_profiles; ++p) {
    profile_info = dev_info->profile_info[p];
    for (stat_info = profile_info->tbl_info_list.stat_tbl_list, i = 0;
         i < profile_info->tbl_info_list.num_stat_tbls;
         ++stat_info, ++i) {
      ret = pipe_mgr_stat_mgr_complete_operations(dev_info->dev_id,
                                                  stat_info->handle);
      if (ret != PIPE_SUCCESS) {
        LOG_ERROR("%s:%d Stat table with handle 0x%x log failed",
                  __func__,
                  __LINE__,
                  stat_info->handle);
        return ret;
      }
    }

    for (stful_info = profile_info->tbl_info_list.stful_tbl_list, i = 0;
         i < profile_info->tbl_info_list.num_sful_tbls;
         ++stful_info, ++i) {
      ret = pipe_mgr_stful_log_cmplt(dev_info->dev_id, stful_info->handle);
      if (ret != PIPE_SUCCESS) {
        LOG_ERROR("%s:%d Stful table with handle 0x%x log failed",
                  __func__,
                  __LINE__,
                  stful_info->handle);
        return ret;
      }
    }
  }

  return PIPE_SUCCESS;
}

static bf_status_t pipe_mgr_log_device(bf_dev_id_t dev_id, cJSON *dev) {
  pipe_status_t ret = PIPE_SUCCESS;
  pipe_sess_hdl_t sess_hdl = pipe_mgr_get_int_sess_hdl();
  rmt_dev_info_t *dev_info;
  rmt_dev_profile_info_t *profile_info;
  pipe_mat_tbl_info_t *mat_info;
  pipe_adt_tbl_info_t *adt_info;
  pipe_select_tbl_info_t *sel_info;
  pipe_stat_tbl_info_t *stat_info;
  pipe_meter_tbl_info_t *meter_info;
  pipe_stful_tbl_info_t *stful_info;
  cJSON *profiles, *profile;
  cJSON *mat_tbls, *adt_tbls, *sel_tbls;
  cJSON *stat_tbls, *meter_tbls, *stful_tbls, *idle_tbls;
  uint32_t p, i;

  if (!(dev_info = pipe_mgr_get_dev_info(dev_id))) {
    LOG_ERROR(
        "%s:%d Device info for id %u not found", __func__, __LINE__, dev_id);
    return BF_OBJECT_NOT_FOUND;
  }

  pipe_mgr_exclusive_api_enter(dev_id);

  cJSON_AddNumberToObject(dev, "dev_id", dev_id);
  cJSON_AddItemToObject(dev, "profiles", profiles = cJSON_CreateArray());
  for (p = 0; p < dev_info->num_pipeline_profiles; ++p) {
    profile_info = dev_info->profile_info[p];
    cJSON_AddItemToArray(profiles, profile = cJSON_CreateObject());
    cJSON_AddNumberToObject(profile, "profile_id", profile_info->profile_id);
    cJSON_AddItemToObject(profile, "mat_tbls", mat_tbls = cJSON_CreateArray());
    cJSON_AddItemToObject(profile, "adt_tbls", adt_tbls = cJSON_CreateArray());
    cJSON_AddItemToObject(profile, "sel_tbls", sel_tbls = cJSON_CreateArray());
    cJSON_AddItemToObject(
        profile, "stat_tbls", stat_tbls = cJSON_CreateArray());
    cJSON_AddItemToObject(
        profile, "meter_tbls", meter_tbls = cJSON_CreateArray());
    cJSON_AddItemToObject(
        profile, "stful_tbls", stful_tbls = cJSON_CreateArray());
    cJSON_AddItemToObject(
        profile, "idle_tbls", idle_tbls = cJSON_CreateArray());

    // Read most recent stats and stful values from hardware
    for (stat_info = profile_info->tbl_info_list.stat_tbl_list, i = 0;
         i < profile_info->tbl_info_list.num_stat_tbls;
         ++stat_info, ++i) {
      ret = pipe_mgr_stat_tbl_log_database_sync(
          sess_hdl, dev_id, stat_info->handle);
      if (ret != PIPE_SUCCESS) {
        LOG_ERROR("%s:%d Stat table with handle 0x%x hardware sync failed",
                  __func__,
                  __LINE__,
                  stat_info->handle);
        goto cleanup;
      }
    }
    ret = pipe_mgr_drv_ilist_push(&sess_hdl, pipe_mgr_log_stat, (void *)dev);
    if (ret != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Hardware sync push failed for stat tables on device %u",
                __func__,
                __LINE__,
                dev_id);
      goto cleanup;
    }

    for (stful_info = profile_info->tbl_info_list.stful_tbl_list, i = 0;
         i < profile_info->tbl_info_list.num_sful_tbls;
         ++stful_info, ++i) {
      ret = pipe_mgr_stful_log_state(dev_id, stful_info->handle, stful_tbls);
      if (ret != PIPE_SUCCESS) {
        LOG_ERROR("%s:%d Stful table with handle 0x%x hardware sync failed",
                  __func__,
                  __LINE__,
                  stful_info->handle);
        goto cleanup;
      }
    }

    for (mat_info = profile_info->tbl_info_list.mat_tbl_list, i = 0;
         i < profile_info->tbl_info_list.num_mat_tbls;
         ++mat_info, ++i) {
      enum pipe_mgr_table_owner_t owner;
      owner = pipe_mgr_sm_tbl_owner(dev_info->dev_id, mat_info->handle);
      switch (owner) {
        case PIPE_MGR_TBL_OWNER_EXM:
          ret = pipe_mgr_exm_log_state(
              dev_info->dev_id, mat_info, mat_info->handle, mat_tbls);
          break;
        case PIPE_MGR_TBL_OWNER_TRN:
          if (!mat_info->alpm_info) {
            ret = pipe_mgr_tcam_log_state(
                dev_info->dev_id, mat_info, mat_info->handle, mat_tbls);
          }
          break;
        case PIPE_MGR_TBL_OWNER_ALPM:
          ret = pipe_mgr_alpm_log_state(dev_info->dev_id, mat_info, mat_tbls);
          break;
        case PIPE_MGR_TBL_OWNER_PHASE0:
          ret = pipe_mgr_phase0_log_state(
              dev_info->dev_id, mat_info->handle, mat_tbls);
          break;
        case PIPE_MGR_TBL_OWNER_NO_KEY:
          break;
        default:
          PIPE_MGR_DBGCHK(0);
          ret = PIPE_INVALID_ARG;
      }
      if (ret != PIPE_SUCCESS) {
        LOG_ERROR("%s:%d Match table with handle 0x%x log failed",
                  __func__,
                  __LINE__,
                  mat_info->handle);
        goto cleanup;
      }
      if (pipe_mgr_mat_tbl_has_idle(dev_info->dev_id, mat_info->handle)) {
        ret = pipe_mgr_idle_log_state(
            dev_info->dev_id, mat_info->handle, idle_tbls);
        if (ret != PIPE_SUCCESS) {
          LOG_ERROR("%s:%d Idle table with handle 0x%x log failed",
                    __func__,
                    __LINE__,
                    mat_info->handle);
          goto cleanup;
        }
      }
    }

    for (adt_info = profile_info->tbl_info_list.adt_tbl_list, i = 0;
         i < profile_info->tbl_info_list.num_adt_tbls;
         ++adt_info, ++i) {
      ret =
          pipe_mgr_adt_log_state(dev_info->dev_id, adt_info->handle, adt_tbls);
      if (ret != PIPE_SUCCESS) {
        LOG_ERROR("%s:%d Action table with handle 0x%x log failed",
                  __func__,
                  __LINE__,
                  adt_info->handle);
        goto cleanup;
      }
    }

    for (sel_info = profile_info->tbl_info_list.select_tbl_list, i = 0;
         i < profile_info->tbl_info_list.num_select_tbls;
         ++sel_info, ++i) {
      ret =
          pipe_mgr_sel_log_state(dev_info->dev_id, sel_info->handle, sel_tbls);
      if (ret != PIPE_SUCCESS) {
        LOG_ERROR("%s:%d Selector table with handle 0x%x log failed",
                  __func__,
                  __LINE__,
                  sel_info->handle);
        goto cleanup;
      }
    }

    for (meter_info = profile_info->tbl_info_list.meter_tbl_list, i = 0;
         i < profile_info->tbl_info_list.num_meter_tbls;
         ++meter_info, ++i) {
      ret = pipe_mgr_meter_log_state(
          dev_info->dev_id, meter_info->handle, meter_tbls);
      if (ret != PIPE_SUCCESS) {
        LOG_ERROR("%s:%d Meter table with handle 0x%x log failed",
                  __func__,
                  __LINE__,
                  meter_info->handle);
        goto cleanup;
      }
    }
  }

  ret = pipe_mgr_log_cmplt(dev_info);
  pipe_mgr_exclusive_api_exit(dev_id);
  return ret;

cleanup:
  pipe_mgr_drv_ilist_abort(&sess_hdl);
  pipe_mgr_exclusive_api_exit(dev_id);
  return ret;
}

static bf_status_t pipe_mgr_restore_device(bf_dev_id_t dev_id, cJSON *dev) {
  pipe_status_t ret = PIPE_SUCCESS;
  rmt_dev_info_t *dev_info;
  rmt_dev_profile_info_t *profile_info = NULL;
  pipe_mat_tbl_info_t *mat_info = NULL;
  cJSON *profiles, *profile;
  cJSON *tbls, *tbl;
  pipe_sess_hdl_t sess_hdl = pipe_mgr_get_int_sess_hdl();
  pipe_mgr_sess_ctx_t *s = pipe_mgr_get_sess_ctx(sess_hdl, __func__, __LINE__);
  int profile_id;
  uint32_t p;
  pipe_mat_tbl_hdl_t mat_tbl_hdl;
  enum pipe_mgr_table_owner_t owner;

  if (!s) {
    LOG_ERROR("%s:%d No context found for session [%d]",
              __func__,
              __LINE__,
              (unsigned)sess_hdl);
    PIPE_MGR_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  if (!(dev_info = pipe_mgr_get_dev_info(dev_id))) {
    LOG_ERROR(
        "%s:%d Device info for id %u not found", __func__, __LINE__, dev_id);
    return BF_OBJECT_NOT_FOUND;
  }

  pipe_mgr_exclusive_api_enter(dev_id);
  s->batchInProg = true;

  profiles = cJSON_GetObjectItem(dev, "profiles");
  /* Iterate through the various table managers and delegate the corresponding
   * restore tasks to each. The order of the restore is the reverse of the
   * dependency chain between the managers
   * (resources -> action -> selector -> match). As an exception, idletime
   * state is restored after match to prevent premature entry timeouts.
   */
  for (profile = profiles->child; profile; profile = profile->next) {
    profile_id = cJSON_GetObjectItem(profile, "profile_id")->valueint;
    for (p = 0; p < dev_info->num_pipeline_profiles; p++) {
      profile_info = dev_info->profile_info[p];
      if (profile_info->profile_id == profile_id) {
        break;
      }
    }
    if (p == dev_info->num_pipeline_profiles) {
      LOG_ERROR("%s:%d Profile id %d for device %u not found",
                __func__,
                __LINE__,
                profile_id,
                dev_id);
      ret = PIPE_OBJ_NOT_FOUND;
      goto cleanup;
    }

    tbls = cJSON_GetObjectItem(profile, "stat_tbls");
    for (tbl = tbls->child; tbl; tbl = tbl->next) {
      ret = pipe_mgr_stat_restore_state(dev_id, tbl);
      if (ret != PIPE_SUCCESS) {
        LOG_ERROR("%s:%d Stat table with handle 0x%x device %u restore failed",
                  __func__,
                  __LINE__,
                  cJSON_GetObjectItem(tbl, "handle")->valueint,
                  dev_id);
        goto cleanup;
      }
    }
    ret = pipe_mgr_drv_ilist_push(&sess_hdl, NULL, NULL);
    if (ret != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Unable to flush batch for session %d",
                __func__,
                __LINE__,
                sess_hdl);
      goto cleanup;
    }

    tbls = cJSON_GetObjectItem(profile, "meter_tbls");
    for (tbl = tbls->child; tbl; tbl = tbl->next) {
      ret = pipe_mgr_meter_restore_state(dev_id, tbl);
      if (ret != PIPE_SUCCESS) {
        LOG_ERROR("%s:%d Meter table with handle 0x%x device %u restore failed",
                  __func__,
                  __LINE__,
                  cJSON_GetObjectItem(tbl, "handle")->valueint,
                  dev_id);
        goto cleanup;
      }
    }
    ret = pipe_mgr_drv_ilist_push(&sess_hdl, NULL, NULL);
    if (ret != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Unable to flush batch for session %d",
                __func__,
                __LINE__,
                sess_hdl);
      goto cleanup;
    }

    tbls = cJSON_GetObjectItem(profile, "stful_tbls");
    for (tbl = tbls->child; tbl; tbl = tbl->next) {
      ret = pipe_mgr_stful_restore_state(dev_id, tbl);
      if (ret != PIPE_SUCCESS) {
        LOG_ERROR("%s:%d Stful table with handle 0x%x device %u restore failed",
                  __func__,
                  __LINE__,
                  cJSON_GetObjectItem(tbl, "handle")->valueint,
                  dev_id);
        goto cleanup;
      }
    }
    ret = pipe_mgr_drv_ilist_push(&sess_hdl, NULL, NULL);
    if (ret != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Unable to flush batch for session %d",
                __func__,
                __LINE__,
                sess_hdl);
      goto cleanup;
    }

    tbls = cJSON_GetObjectItem(profile, "adt_tbls");
    for (tbl = tbls->child; tbl; tbl = tbl->next) {
      ret = pipe_mgr_adt_restore_state(dev_id, tbl);
      if (ret != PIPE_SUCCESS) {
        LOG_ERROR("%s:%d Act table with handle 0x%x device %u restore failed",
                  __func__,
                  __LINE__,
                  cJSON_GetObjectItem(tbl, "handle")->valueint,
                  dev_id);
        goto cleanup;
      }
    }
    ret = pipe_mgr_drv_ilist_push(&sess_hdl, NULL, NULL);
    if (ret != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Unable to flush batch for session %d",
                __func__,
                __LINE__,
                sess_hdl);
      goto cleanup;
    }

    tbls = cJSON_GetObjectItem(profile, "sel_tbls");
    for (tbl = tbls->child; tbl; tbl = tbl->next) {
      ret = pipe_mgr_sel_restore_state(dev_id, tbl);
      if (ret != PIPE_SUCCESS) {
        LOG_ERROR("%s:%d Sel table with handle 0x%x device %u restore failed",
                  __func__,
                  __LINE__,
                  cJSON_GetObjectItem(tbl, "handle")->valueint,
                  dev_id);
        goto cleanup;
      }
    }
    ret = pipe_mgr_drv_ilist_push(&sess_hdl, NULL, NULL);
    if (ret != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Unable to flush batch for session %d",
                __func__,
                __LINE__,
                sess_hdl);
      goto cleanup;
    }

    tbls = cJSON_GetObjectItem(profile, "idle_tbls");
    for (tbl = tbls->child; tbl; tbl = tbl->next) {
      ret = pipe_mgr_idle_restore_init(dev_id, tbl);
      if (ret != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Idle table with handle 0x%x device %u restore init failed",
            __func__,
            __LINE__,
            cJSON_GetObjectItem(tbl, "handle")->valueint,
            dev_id);
        goto cleanup;
      }
    }

    tbls = cJSON_GetObjectItem(profile, "mat_tbls");
    for (tbl = tbls->child; tbl; tbl = tbl->next) {
      mat_tbl_hdl = cJSON_GetObjectItem(tbl, "handle")->valueint;
      mat_info = pipe_mgr_get_tbl_info(dev_id, mat_tbl_hdl, __func__, __LINE__);
      if (mat_info == NULL) {
        LOG_ERROR("Error in finding the table info for tbl 0x%x device id %d",
                  mat_tbl_hdl,
                  dev_id);
        ret = PIPE_OBJ_NOT_FOUND;
        goto cleanup;
      }
      owner = pipe_mgr_sm_tbl_owner(dev_id, mat_tbl_hdl);
      switch (owner) {
        case PIPE_MGR_TBL_OWNER_EXM:
          ret = pipe_mgr_exm_restore_state(dev_id, mat_info, tbl);
          break;
        case PIPE_MGR_TBL_OWNER_TRN:
          if (!mat_info->alpm_info) {
            ret = pipe_mgr_tcam_restore_state(dev_id, mat_info, tbl, NULL);
          }
          break;
        case PIPE_MGR_TBL_OWNER_ALPM:
          ret = pipe_mgr_alpm_restore_state(dev_id, mat_info, tbl);
          break;
        case PIPE_MGR_TBL_OWNER_PHASE0:
          ret = pipe_mgr_phase0_restore_state(dev_id, tbl);
          break;
        case PIPE_MGR_TBL_OWNER_NO_KEY:
          break;
        default:
          break;
      }
      if (ret != PIPE_SUCCESS) {
        LOG_ERROR("%s:%d Match table with handle 0x%x device %u restore failed",
                  __func__,
                  __LINE__,
                  mat_tbl_hdl,
                  dev_id);
        goto cleanup;
      }
      mat_info->symmetric =
          (cJSON_GetObjectItem(tbl, "symmetric")->type == cJSON_True);
    }
    ret = pipe_mgr_drv_ilist_push(&sess_hdl, NULL, NULL);
    if (ret != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Unable to flush batch for session %d",
                __func__,
                __LINE__,
                sess_hdl);
      goto cleanup;
    }

    tbls = cJSON_GetObjectItem(profile, "idle_tbls");
    for (tbl = tbls->child; tbl; tbl = tbl->next) {
      ret = pipe_mgr_idle_restore_state(dev_id, tbl);
      if (ret != PIPE_SUCCESS) {
        LOG_ERROR("%s:%d Idle table with handle 0x%x device %u restore failed",
                  __func__,
                  __LINE__,
                  cJSON_GetObjectItem(tbl, "handle")->valueint,
                  dev_id);
        goto cleanup;
      }
    }
    ret = pipe_mgr_drv_ilist_push(&sess_hdl, NULL, NULL);
    if (ret != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Unable to flush batch for session %d",
                __func__,
                __LINE__,
                sess_hdl);
      goto cleanup;
    }
  }

  s->batchInProg = false;
  pipe_mgr_drv_i_list_cmplt_all(&sess_hdl);
  pipe_mgr_exclusive_api_exit(dev_id);
  return BF_SUCCESS;

cleanup:
  pipe_mgr_drv_ilist_abort(&sess_hdl);
  pipe_mgr_exclusive_api_exit(dev_id);
  return ret;
}

extern bool pipe_mgr_hitless_ha_issue_callbacks_from_llp;
pipe_status_t pipe_mgr_enable_callbacks_for_hitless_ha(pipe_sess_hdl_t sess_hdl,
                                                       bf_dev_id_t device_id) {
  (void)sess_hdl;
  if (pipe_mgr_is_device_virtual(device_id)) {
    LOG_ERROR(
        "%s:%d Enabling callbacks for hitless HA not supported for virtual "
        "device id %d",
        __func__,
        __LINE__,
        device_id);
    return PIPE_NOT_SUPPORTED;
  }

  if (!pipe_mgr_is_device_virtual_dev_slave(device_id)) {
    LOG_ERROR(
        "%s:%d Enabling callbacks for hitless HA not supported for pure "
        "physical device id %d",
        __func__,
        __LINE__,
        device_id);
    return PIPE_NOT_SUPPORTED;
  }
  pipe_mgr_hitless_ha_issue_callbacks_from_llp = true;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_mat_ha_reconciliation_report_get(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_ha_reconc_report_t *ha_report) {
  pipe_status_t ret = PIPE_SUCCESS;

  pipe_mat_tbl_info_t *mat_tbl_info =
      pipe_mgr_get_tbl_info(dev_tgt.device_id, mat_tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    LOG_ERROR(
        "Error in finding the table info for tbl 0x%x"
        " device id %d",
        mat_tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  ret = pipe_mgr_verify_pipe_id(
      dev_tgt, mat_tbl_info, false /* light_pipe_validation */);
  if (ret != PIPE_SUCCESS) {
    return ret;
  }

  ret = pipe_mgr_hitless_ha_get_reconc_report(
      sess_hdl, dev_tgt, mat_tbl_hdl, ha_report);
  if (ret != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in getting ha reconciliation report for table with handle "
        "0x%x device %d sts %s (%d)",
        __func__,
        __LINE__,
        mat_tbl_hdl,
        dev_tgt.device_id,
        bf_err_str(ret),
        ret);
    return ret;
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_parser_id_get(bf_dev_id_t devid,
                                     bf_dev_port_t port,
                                     pipe_parser_id_t *parser_id) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  if (!dev_info) return PIPE_INVALID_ARG;
  int local_port = dev_info->dev_cfg.dev_port_to_local_port(port);
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      *parser_id = (local_port / TOF_NUM_CHN_PER_PORT);
      break;
    case BF_DEV_FAMILY_TOFINO2:
    case BF_DEV_FAMILY_TOFINO3:
      *parser_id = (local_port / 8) * 4;
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_pipe_id_get(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   bf_dev_pipe_t *pipe_id) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return PIPE_INVALID_ARG;
  *pipe_id = dev_info->dev_cfg.dev_port_to_pipe(dev_port);
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_get_num_pipelines(bf_dev_id_t dev_id,
                                         uint32_t *num_pipes) {
  if (!num_pipes) return PIPE_INVALID_ARG;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return PIPE_INVALID_ARG;
  *num_pipes = dev_info->num_active_pipes;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_get_num_active_stages(bf_dev_id_t dev_id,
                                             uint8_t *num_active_stages) {
  if (!num_active_stages) return PIPE_INVALID_ARG;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return PIPE_INVALID_ARG;
  switch (dev_info->dev_type) {
    case BF_DEV_BFNT10032D:
    case BF_DEV_BFNT10032D018:
    case BF_DEV_BFNT10032D020:
    case BF_DEV_BFNT10032Q:
    case BF_DEV_BFNT10064Q:
    case BF_DEV_BFNT20128QM:
    case BF_DEV_BFNT20080TM:
      *num_active_stages = 12;
      return PIPE_SUCCESS;
    case BF_DEV_BFNT20064D:
    case BF_DEV_BFNT20064Q:
    case BF_DEV_BFNT20128Q:
    case BF_DEV_BFNT20080T:
    case BF_DEV_BFNT31_12Q:
    case BF_DEV_BFNT31_12QH:
      *num_active_stages = 20;
      return PIPE_SUCCESS;
    default:
      *num_active_stages = 0;
      return PIPE_INVALID_ARG;
  }
}

bf_status_t bf_pipe_mgr_port_ebuf_counter_get(bf_dev_id_t dev_id,
                                              bf_dev_port_t port_id,
                                              uint64_t *value) {
  pipe_status_t sts = PIPE_SUCCESS;

  LOG_TRACE("Entering %s", __func__);

  if (!pipe_mgr_ctx) {
    return PIPE_NOT_READY;
  }

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;

  sts = pipe_mgr_api_enter(shdl);
  if (PIPE_SUCCESS != sts) return sts;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s: Invalid device id %d", __func__, dev_id);
    pipe_mgr_api_exit(shdl);
    return PIPE_INVALID_ARG;
  }

  sts = pipe_mgr_parde_port_ebuf_counter_get(shdl, dev_info, port_id, value);
  if (sts != PIPE_SUCCESS)
    LOG_ERROR(
        "%s: failed to get the EBUF packet counter for dev %d, port "
        "%d",
        __func__,
        dev_id,
        port_id);
  else
    LOG_TRACE("Exiting %s successfully", __func__);

  pipe_mgr_api_exit(shdl);
  return sts;
}

pipe_status_t pipe_mgr_pipe_pps_limit_set(dev_target_t dev_tgt,
                                          uint64_t max_pps) {
  pipe_status_t ret;
  pipe_sess_hdl_t shdl = get_pipe_mgr_ctx()->int_ses_hndl;
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(shdl))) {
    return ret;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_tgt.device_id);
  if (!dev_info) {
    ret = PIPE_INVALID_ARG;
    goto done;
  }

  if (dev_info->virtual_device) {
    LOG_ERROR("%s: Dev %d PPS limit does not apply to virtual devices",
              __func__,
              dev_info->dev_id);
    ret = PIPE_NOT_SUPPORTED;
    goto done;
  }

  ret =
      pipe_mgr_parb_pps_limit_set(shdl, dev_info, dev_tgt.dev_pipe_id, max_pps);
  if (ret == PIPE_SUCCESS) {
    ret = pipe_mgr_drv_ilist_push(&shdl, NULL, NULL);
  } else {
    pipe_mgr_drv_ilist_abort(&shdl);
  }

done:
  pipe_mgr_api_exit(shdl);
  return ret;
}

bf_status_t pipe_mgr_pipe_pps_limit_get(dev_target_t dev_tgt,
                                        uint64_t *max_pps) {
  pipe_status_t ret;
  if (!max_pps) return PIPE_INVALID_ARG;
  pipe_sess_hdl_t shdl = get_pipe_mgr_ctx()->int_ses_hndl;
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(shdl))) {
    return ret;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_tgt.device_id);
  if (!dev_info) {
    ret = PIPE_INVALID_ARG;
    goto done;
  }

  ret = pipe_mgr_parb_pps_limit_get(dev_info, dev_tgt.dev_pipe_id, max_pps);

done:
  pipe_mgr_api_exit(shdl);
  return ret;
}

bf_status_t pipe_mgr_pipe_pps_max_get(dev_target_t dev_tgt, uint64_t *max_pps) {
  pipe_status_t ret;
  if (!max_pps) return PIPE_INVALID_ARG;
  pipe_sess_hdl_t shdl = get_pipe_mgr_ctx()->int_ses_hndl;
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(shdl))) {
    return ret;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_tgt.device_id);
  if (!dev_info) {
    ret = PIPE_INVALID_ARG;
    goto done;
  }

  ret = pipe_mgr_parb_pps_limit_max_get(dev_info, dev_tgt.dev_pipe_id, max_pps);

done:
  pipe_mgr_api_exit(shdl);
  return ret;
}

bf_status_t pipe_mgr_pipe_pps_limit_reset(dev_target_t dev_tgt) {
  pipe_status_t ret;
  pipe_sess_hdl_t shdl = get_pipe_mgr_ctx()->int_ses_hndl;
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(shdl))) {
    return ret;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_tgt.device_id);
  if (!dev_info) {
    ret = PIPE_INVALID_ARG;
    goto done;
  }

  if (dev_info->virtual_device) {
    LOG_ERROR("%s: Dev %d PPS limit does not apply to virtual devices",
              __func__,
              dev_info->dev_id);
    ret = PIPE_NOT_SUPPORTED;
    goto done;
  }

  ret = pipe_mgr_parb_pps_limit_reset(shdl, dev_info, dev_tgt.dev_pipe_id);
  if (ret == PIPE_SUCCESS) {
    ret = pipe_mgr_drv_ilist_push(&shdl, NULL, NULL);
  } else {
    pipe_mgr_drv_ilist_abort(&shdl);
  }

done:
  pipe_mgr_api_exit(shdl);
  return ret;
}

bf_status_t bf_pipe_mgr_port_ebuf_bypass_counter_get(bf_dev_id_t dev_id,
                                                     bf_dev_port_t port_id,
                                                     uint64_t *value) {
  if (!value) return PIPE_INVALID_ARG;
  pipe_status_t sts = PIPE_SUCCESS;

  LOG_TRACE("Entering %s", __func__);

  if (!pipe_mgr_ctx) {
    return PIPE_NOT_READY;
  }

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;

  sts = pipe_mgr_api_enter(shdl);
  if (PIPE_SUCCESS != sts) return sts;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s: Invalid device id %d", __func__, dev_id);
    pipe_mgr_api_exit(shdl);
    return PIPE_INVALID_ARG;
  }

  sts = pipe_mgr_parde_port_ebuf_bypass_counter_get(
      shdl, dev_info, port_id, value);
  if (sts != PIPE_SUCCESS)
    LOG_ERROR(
        "%s: failed to get the EBUF packet counter for dev %d, port "
        "%d",
        __func__,
        dev_id,
        port_id);
  else
    LOG_TRACE("Exiting %s successfully", __func__);

  pipe_mgr_api_exit(shdl);
  return sts;
}

bf_status_t bf_pipe_mgr_port_ebuf_100g_credits_get(bf_dev_id_t dev_id,
                                                   bf_dev_port_t port_id,
                                                   uint64_t *value) {
  if (!value) return PIPE_INVALID_ARG;
  pipe_status_t sts = PIPE_SUCCESS;

  LOG_TRACE("Entering %s", __func__);

  if (!pipe_mgr_ctx) {
    return PIPE_NOT_READY;
  }

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;

  sts = pipe_mgr_api_enter(shdl);
  if (PIPE_SUCCESS != sts) return sts;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s: Invalid device id %d", __func__, dev_id);
    pipe_mgr_api_exit(shdl);
    return PIPE_INVALID_ARG;
  }

  sts =
      pipe_mgr_parde_port_ebuf_100g_credits_get(shdl, dev_info, port_id, value);
  if (sts != PIPE_SUCCESS)
    LOG_ERROR(
        "%s: failed to get the EBUF packet counter for dev %d, port "
        "%d",
        __func__,
        dev_id,
        port_id);
  else
    LOG_TRACE("Exiting %s successfully", __func__);

  pipe_mgr_api_exit(shdl);
  return sts;
}

bf_status_t bf_pipe_mgr_port_iprsr_threshold_set(bf_dev_id_t dev_id,
                                                 bf_dev_port_t port_id,
                                                 uint32_t threshold) {
  pipe_status_t sts = PIPE_SUCCESS;

  if (!pipe_mgr_ctx) {
    return PIPE_NOT_READY;
  }

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;

  sts = pipe_mgr_api_enter(shdl);
  if (PIPE_SUCCESS != sts) return sts;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s: Invalid device id %d", __func__, dev_id);
    pipe_mgr_api_exit(shdl);
    return PIPE_INVALID_ARG;
  }

  sts = pipe_mgr_parde_iprsr_pri_threshold_set(dev_info, port_id, threshold);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s: failed to set ingress parser priority threshold for dev %d,"
        " port %d",
        __func__,
        dev_id,
        port_id);
  }

  pipe_mgr_api_exit(shdl);
  return sts;
}

bf_status_t bf_pipe_mgr_port_iprsr_threshold_get(bf_dev_id_t dev_id,
                                                 bf_dev_port_t port_id,
                                                 uint32_t *threshold) {
  pipe_status_t sts = PIPE_SUCCESS;

  if (!threshold) {
    return PIPE_INVALID_ARG;
  }

  if (!pipe_mgr_ctx) {
    return PIPE_NOT_READY;
  }

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;

  sts = pipe_mgr_api_enter(shdl);
  if (PIPE_SUCCESS != sts) return sts;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s: Invalid device id %d", __func__, dev_id);
    pipe_mgr_api_exit(shdl);
    return PIPE_INVALID_ARG;
  }

  sts = pipe_mgr_parde_iprsr_pri_threshold_get(dev_info, port_id, threshold);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s: failed to get ingress parser priority threshold for dev %d,"
        " port %d",
        __func__,
        dev_id,
        port_id);
  }

  pipe_mgr_api_exit(shdl);
  return sts;
}

bf_status_t bf_pipe_mgr_25g_overspeed_mode_set(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               bool enable) {
  if (!pipe_mgr_ctx) return BF_NOT_READY;

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t sts = pipe_mgr_api_enter(shdl);
  if (PIPE_SUCCESS != sts) return sts;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s: Invalid device id %d", __func__, dev_id);
    pipe_mgr_api_exit(shdl);
    return BF_INVALID_ARG;
  }

  if (dev_info->dev_family != BF_DEV_FAMILY_TOFINO) {
    pipe_mgr_api_exit(shdl);
    return BF_NOT_SUPPORTED;
  }

  if (dev_port & 1) {
    /* Only even channels can be used for 25g overspeed since the overspeed mode
     * configures the data path as 50g which requires a channel pair on TF1. */
    pipe_mgr_api_exit(shdl);
    return BF_INVALID_ARG;
  }

  bf_map_sts_t msts;
  if (enable)
    msts = pipe_mgr_overspeed_map_add(dev_id, dev_port);
  else
    msts = pipe_mgr_overspeed_map_rmv(dev_id, dev_port);

  pipe_mgr_api_exit(shdl);

  if (msts == BF_MAP_OK) {
    return BF_SUCCESS;
  }
  return BF_UNEXPECTED;
}

bf_status_t bf_pipe_mgr_25g_overspeed_mode_get(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               bool *enable) {
  if (!enable) return BF_INVALID_ARG;
  if (!pipe_mgr_ctx) return BF_NOT_READY;

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t sts = pipe_mgr_api_enter(shdl);
  if (PIPE_SUCCESS != sts) return sts;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s: Invalid device id %d", __func__, dev_id);
    pipe_mgr_api_exit(shdl);
    return BF_INVALID_ARG;
  }

  bf_map_sts_t msts = pipe_mgr_overspeed_map_get(dev_id, dev_port, enable);

  pipe_mgr_api_exit(shdl);

  if (msts == BF_MAP_OK) {
    return BF_SUCCESS;
  }
  return BF_UNEXPECTED;
}

pipe_status_t pipe_mgr_submodule_debug_set(pipe_mgr_submodule_t submodule) {
  if (!pipe_mgr_ctx) {
    return BF_NOT_READY;
  }
  if (submodule == PIPE_MGR_LOG_DBG_ALL) {
    /* Enable debug logging for all sub-modules */
    pipe_mgr_ctx->pipe_mgr_submodule_debug = PIPE_MGR_DBG_ALL;
    return PIPE_SUCCESS;
  }
  pipe_mgr_ctx->pipe_mgr_submodule_debug =
      (pipe_mgr_ctx->pipe_mgr_submodule_debug | (1 << submodule));
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_submodule_debug_reset(pipe_mgr_submodule_t submodule) {
  if (!pipe_mgr_ctx) {
    return BF_NOT_READY;
  }
  if (submodule == PIPE_MGR_LOG_DBG_ALL) {
    /* Enable debug logging for all sub-modules */
    pipe_mgr_ctx->pipe_mgr_submodule_debug = 0;
    return PIPE_SUCCESS;
  }
  pipe_mgr_ctx->pipe_mgr_submodule_debug =
      (pipe_mgr_ctx->pipe_mgr_submodule_debug & ~(1 << submodule));
  return PIPE_SUCCESS;
}

uint32_t pipe_mgr_submodule_debug_get() {
  return pipe_mgr_ctx->pipe_mgr_submodule_debug;
}

pipe_status_t pipe_mgr_gfm_test_pattern_set(pipe_sess_hdl_t shdl,
                                            bf_dev_target_t dev_tgt,
                                            uint32_t pipe_api_flags,
                                            bf_dev_direction_t gress,
                                            dev_stage_t stage_id,
                                            int num_patterns,
                                            uint64_t *row_patterns,
                                            uint64_t *row_bad_parity) {
  if (!pipe_mgr_ctx) return BF_NOT_READY;

  pipe_status_t sts = pipe_mgr_api_enter(shdl);
  if (PIPE_SUCCESS != sts) return sts;

  if (pipe_mgr_sess_in_batch(shdl)) {
    pipe_mgr_drv_ilist_chkpt(shdl);
  }

  sts = pipe_mgr_gfm_test(shdl,
                          dev_tgt,
                          gress,
                          stage_id,
                          num_patterns,
                          row_patterns,
                          row_bad_parity);

  sts = handleTableApiRsp(
      shdl, sts, pipe_api_flags & PIPE_FLAG_SYNC_REQ, __func__, __LINE__);
  pipe_mgr_api_exit(shdl);
  return sts;
}

pipe_status_t pipe_mgr_gfm_test_col_set(pipe_sess_hdl_t shdl,
                                        bf_dev_target_t dev_tgt,
                                        uint32_t pipe_api_flags,
                                        bf_dev_direction_t gress,
                                        dev_stage_t stage_id,
                                        int column,
                                        uint16_t col_data[64]) {
  if (!pipe_mgr_ctx) return BF_NOT_READY;

  pipe_status_t sts = pipe_mgr_api_enter(shdl);
  if (PIPE_SUCCESS != sts) return sts;

  if (pipe_mgr_sess_in_batch(shdl)) {
    pipe_mgr_drv_ilist_chkpt(shdl);
  }

  sts = pipe_mgr_gfm_test_col(shdl, dev_tgt, gress, stage_id, column, col_data);

  sts = handleTableApiRsp(
      shdl, sts, pipe_api_flags & PIPE_FLAG_SYNC_REQ, __func__, __LINE__);
  pipe_mgr_api_exit(shdl);
  return sts;
}
