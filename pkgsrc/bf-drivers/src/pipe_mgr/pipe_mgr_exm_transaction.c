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
 * @file pipe_exm_transaction.c
 * @date
 *
 * Exact-match table transaction handling.
 * Contains all code for dealing with transactions on exact match tables.
 */

/* Standard header includes */

/* Module header includes */

/* Local header includes */
#include "pipe_mgr_exm_tbl_mgr_int.h"
#include "pipe_mgr_exm_transaction.h"
#include "cuckoo_move.h"
#include "pipe_mgr_exm_tbl_init.h"

static void pipe_mgr_exm_ent_idx_txn_commit(
    pipe_mgr_exm_tbl_data_t *exm_tbl_data) {
  pipe_mgr_exm_idx_txn_node_t *txn_node = NULL;
  unsigned long key = 0;
  bf_map_sts_t map_sts = BF_MAP_OK;
  while ((map_sts = bf_map_get_first_rmv(
              &exm_tbl_data->dirtied_ent_idx_htbl, &key, (void **)&txn_node)) ==
         BF_MAP_OK) {
    PIPE_MGR_FREE(txn_node);
  }
  return;
}

pipe_status_t pipe_mgr_exm_tbl_txn_commit(bf_dev_id_t dev_id,
                                          pipe_mgr_sm_tbl_info_t *t,
                                          bf_dev_pipe_t *pipes_list,
                                          unsigned nb_pipes) {
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  pipe_mgr_exm_stage_info_t *exm_stage_info = NULL;
  pipe_mgr_exm_txn_node_t *txn_node = NULL;
  bf_map_sts_t map_sts = BF_MAP_OK;
  unsigned long key = 0;
  unsigned i, j, pipe;
  bool symmetric;

  exm_tbl = pipe_mgr_exm_tbl_get(dev_id, t->id);
  if (!exm_tbl) {
    LOG_ERROR(
        "%s : Could not find the exact match table info for table with"
        " handle %d, device id %d",
        __func__,
        t->id,
        dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Process only table session's related pipes. */
  i = 0;
  symmetric = false;
  while (i < nb_pipes && !symmetric) {
    if (exm_tbl->symmetric == true) {
      /* For symmetric table, we loop only once here. */
      symmetric = true;
      pipe = BF_DEV_PIPE_ALL;
    } else {
      pipe = pipes_list[i++];
    }
    exm_tbl_data = pipe_mgr_exm_tbl_get_instance(exm_tbl, pipe);
    if (!exm_tbl_data) continue;

    /* Firt cleanup the dirtied entry handle txn hash table for that instance */
    while ((map_sts = bf_map_get_first_rmv(&exm_tbl_data->dirtied_ent_hdl_htbl,
                                           &key,
                                           (void **)&txn_node)) == BF_MAP_OK) {
      if (txn_node->entry_info) {
        PIPE_MGR_FREE(txn_node->entry_info);
      }
      PIPE_MGR_FREE(txn_node);
    }

    for (j = 0; j < exm_tbl_data->num_stages; j++) {
      exm_stage_info = &exm_tbl_data->exm_stage_info[j];
      cuckoo_move_graph_t *x =
          pipe_mgr_exm_get_or_create_cuckoo_move_graph(exm_stage_info);

      if (!x) {
        return PIPE_NO_SYS_RESOURCES;
      }

      cuckoo_move_graph_txn_commit(x);
    }
    pipe_mgr_exm_ent_idx_txn_commit(exm_tbl_data);
    exm_tbl_data->default_entry_backed_up = false;
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_exm_txn_abort_for_entry_add(
    pipe_mgr_exm_tbl_t *exm_tbl, pipe_mgr_exm_txn_node_t *txn_node) {
  /* An entry was added, need to reverse the state update */
  bf_map_sts_t map_sts = BF_MAP_OK;
  unsigned long key = txn_node->mat_ent_hdl;
  pipe_mgr_exm_entry_info_t *entry_info = NULL;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  pipe_mgr_exm_stage_info_t *exm_stage_info = NULL;

  exm_tbl_data = pipe_mgr_exm_tbl_get_instance(exm_tbl, txn_node->pipe_id);
  if (exm_tbl_data == NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_OBJ_NOT_FOUND;
  }

  map_sts =
      bf_map_get_rmv(&exm_tbl_data->entry_info_htbl, key, (void **)&entry_info);
  if (map_sts == BF_MAP_NO_KEY) {
    return PIPE_SUCCESS;
  }
  if (map_sts != BF_MAP_OK) {
    LOG_ERROR(
        "%s:%d Error in removing entry info for entry handle %d, tbl 0x%x, "
        "device id %d, err 0x%x",
        __func__,
        __LINE__,
        txn_node->mat_ent_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        map_sts);
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }

  if (!pipe_mgr_exm_is_ent_hdl_default(exm_tbl_data, txn_node->mat_ent_hdl)) {
    exm_stage_info = pipe_mgr_exm_tbl_get_stage_info(
        exm_tbl, txn_node->pipe_id, txn_node->stage_id);
    if (exm_stage_info == NULL) {
      PIPE_MGR_DBGCHK(0);
      return PIPE_OBJ_NOT_FOUND;
    }

    if (exm_tbl_data->num_entries_placed > 0) {
      exm_tbl_data->num_entries_placed--;
    }
    if (exm_stage_info->num_entries_placed > 0) {
      exm_stage_info->num_entries_placed--;
    }
    pipe_mgr_exm_deallocate_entry_hdl(
        exm_tbl, exm_tbl_data, txn_node->mat_ent_hdl);
  }
  PIPE_MGR_FREE(entry_info);
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_exm_txn_abort_for_entry_move_modify_delete(
    pipe_mgr_exm_tbl_t *exm_tbl, pipe_mgr_exm_txn_node_t *txn_node) {
  bf_map_sts_t map_sts = BF_MAP_OK;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  pipe_mgr_exm_stage_info_t *exm_stage_info = NULL;
  pipe_mgr_exm_entry_info_t *entry_info = NULL;
  unsigned long key = txn_node->entry_info->mat_ent_hdl;

  exm_tbl_data = pipe_mgr_exm_tbl_get_instance(exm_tbl, txn_node->pipe_id);
  if (exm_tbl_data == NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_OBJ_NOT_FOUND;
  }
  map_sts =
      bf_map_get(&exm_tbl_data->entry_info_htbl, key, (void **)&entry_info);
  if (map_sts == BF_MAP_NO_KEY) {
    entry_info = PIPE_MGR_CALLOC(1, sizeof(pipe_mgr_exm_entry_info_t));
    if (entry_info == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      PIPE_MGR_DBGCHK(0);
      return PIPE_NO_SYS_RESOURCES;
    }
    map_sts =
        bf_map_add(&exm_tbl_data->entry_info_htbl, key, (void *)entry_info);
    if (map_sts != BF_MAP_OK) {
      LOG_ERROR(
          "%s:%d Error adding entry info for entry handle %d, tbl 0x%x, "
          "device id %d, err 0x%x",
          __func__,
          __LINE__,
          txn_node->mat_ent_hdl,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          map_sts);
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
    }

    exm_stage_info = pipe_mgr_exm_tbl_get_stage_info(
        exm_tbl, txn_node->pipe_id, txn_node->stage_id);
    if (exm_stage_info == NULL) {
      PIPE_MGR_DBGCHK(0);
      return PIPE_OBJ_NOT_FOUND;
    }

    if (!exm_tbl_data->default_entry_backed_up ||
        key != exm_tbl_data->backup_default_entry_hdl) {
      exm_tbl_data->num_entries_placed++;
      exm_stage_info->num_entries_placed++;
      /* Resurrect the entry handle */
      pipe_mgr_exm_set_ent_hdl(exm_tbl_data, txn_node->entry_info->mat_ent_hdl);
    }
  } else if (map_sts != BF_MAP_OK) {
    LOG_ERROR(
        "%s:%d Error in getting entry info for entry handle %d, tbl 0x%x, "
        "device id %d, err 0x%x",
        __func__,
        __LINE__,
        txn_node->mat_ent_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        map_sts);
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }
  /* Restore the entry info the location where it was earlier */
  PIPE_MGR_MEMCPY(
      entry_info, txn_node->entry_info, sizeof(pipe_mgr_exm_entry_info_t));
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_exm_ent_hdl_txn_abort(
    pipe_mgr_exm_tbl_t *exm_tbl, pipe_mgr_exm_tbl_data_t *exm_tbl_data) {
  pipe_status_t status = PIPE_SUCCESS;
  bf_map_sts_t map_sts = BF_MAP_OK;
  pipe_mgr_exm_txn_node_t *txn_node = NULL;
  unsigned long key = 0;

  if (!exm_tbl_data) return PIPE_INVALID_ARG;

  while ((map_sts = bf_map_get_first_rmv(
              &exm_tbl_data->dirtied_ent_hdl_htbl, &key, (void **)&txn_node)) ==
         BF_MAP_OK) {
    switch (txn_node->operation) {
      case PIPE_MGR_EXM_OPERATION_ADD:
        status = pipe_mgr_exm_txn_abort_for_entry_add(exm_tbl, txn_node);
        if (status != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Error in txn abort for entry add for entry hdl %d, "
              "tbl 0x%x, device id %d, err %s",
              __func__,
              __LINE__,
              txn_node->mat_ent_hdl,
              exm_tbl->mat_tbl_hdl,
              exm_tbl->dev_id,
              pipe_str_err(status));
          PIPE_MGR_DBGCHK(0);
          return status;
        }
        break;
      case PIPE_MGR_EXM_OPERATION_MODIFY:
        status = pipe_mgr_exm_txn_abort_for_entry_move_modify_delete(exm_tbl,
                                                                     txn_node);
        if (status != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Error in txn abort for entry modify for entry hdl %d, "
              "tbl 0x%x, device id %d, err %s",
              __func__,
              __LINE__,
              txn_node->mat_ent_hdl,
              exm_tbl->mat_tbl_hdl,
              exm_tbl->dev_id,
              pipe_str_err(status));
          PIPE_MGR_DBGCHK(0);
          return status;
        }
        break;
      case PIPE_MGR_EXM_OPERATION_MOVE:
        status = pipe_mgr_exm_txn_abort_for_entry_move_modify_delete(exm_tbl,
                                                                     txn_node);
        if (status != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Error in txn abort for entry move for entry hdl %d, "
              "tbl 0x%x, device id %d, err %s",
              __func__,
              __LINE__,
              txn_node->mat_ent_hdl,
              exm_tbl->mat_tbl_hdl,
              exm_tbl->dev_id,
              pipe_str_err(status));
          PIPE_MGR_DBGCHK(0);
          return status;
        }
        break;
      case PIPE_MGR_EXM_OPERATION_DELETE:
        status = pipe_mgr_exm_txn_abort_for_entry_move_modify_delete(exm_tbl,
                                                                     txn_node);
        if (status != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Error in txn abort for entry delete for entry hdl %d, "
              "tbl 0x%x, device id %d, err %s",
              __func__,
              __LINE__,
              txn_node->mat_ent_hdl,
              exm_tbl->mat_tbl_hdl,
              exm_tbl->dev_id,
              pipe_str_err(status));
          PIPE_MGR_DBGCHK(0);
          return status;
        }
        break;
      default:
        PIPE_MGR_DBGCHK(0);
        break;
    }
    if (txn_node->entry_info) {
      PIPE_MGR_FREE(txn_node->entry_info);
    }
    PIPE_MGR_FREE(txn_node);
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_exm_ent_idx_txn_abort_int(
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_idx_txn_node_t *txn_node,
    pipe_mat_ent_idx_t entry_idx) {
  bf_map_sts_t map_sts = BF_MAP_OK;
  unsigned long key = entry_idx;
  uintptr_t ent_hdl = 0;
  pipe_mgr_exm_stage_info_t *exm_stage_info = NULL;
  uint8_t num_entries_per_wide_word = 0;
  pipe_mat_ent_idx_t stage_ent_idx = 0;
  pipe_mat_ent_idx_t edge_idx = 0;
  uint8_t subentry = 0;
  Word_t Index1 = 0;
  Word_t Index2 = -1;
  Word_t Rc_word = 0;
  int Rc_int = 0;

  exm_stage_info = exm_tbl_data->stage_info_ptrs[txn_node->stage_id];
  if (exm_stage_info == NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }
  stage_ent_idx = entry_idx - exm_stage_info->stage_offset;
  map_sts = bf_map_get(
      &exm_tbl_data->ent_idx_to_ent_hdl_htbl, key, (void **)&ent_hdl);
  if (map_sts == BF_MAP_NO_KEY) {
    if (txn_node->entry_hdl == PIPE_MAT_ENT_HDL_INVALID_HDL) {
      return PIPE_SUCCESS;
    } else {
      ent_hdl = txn_node->entry_hdl;
      map_sts = bf_map_add(
          &exm_tbl_data->ent_idx_to_ent_hdl_htbl, key, (void *)ent_hdl);
      if (map_sts != BF_MAP_OK) {
        LOG_ERROR(
            "%s:%d Error adding entry idx to entry hdl mapping for idx %d, "
            "pipe id %d, err 0x%x",
            __func__,
            __LINE__,
            entry_idx,
            exm_tbl_data->pipe_id,
            map_sts);
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
      }
      // Default entries do not set cuckoo state
      if (!exm_tbl_data->default_entry_backed_up ||
          txn_node->entry_hdl != exm_tbl_data->backup_default_entry_hdl) {
        /* Here the entry which was deleted has been added back. Hence mark the
         * sub-entry within the wide word as occupied, and if all the
         * sub-entries within the wide word is occupied, mark the edge as
         * occupied. The same thing is done during entry add as well.
         */
        num_entries_per_wide_word =
            exm_stage_info->pack_format->num_entries_per_wide_word;
        subentry = stage_ent_idx % num_entries_per_wide_word;
        edge_idx = stage_ent_idx - subentry;
        J1S(Rc_int, exm_stage_info->PJ1Array[edge_idx], subentry);
        if (Rc_int == JERR) {
          return PIPE_NO_SYS_RESOURCES;
        }
        J1C(Rc_word, exm_stage_info->PJ1Array[edge_idx], Index1, Index2);
        if (Rc_word == num_entries_per_wide_word) {
          cuckoo_move_graph_t *x =
              pipe_mgr_exm_get_or_create_cuckoo_move_graph(exm_stage_info);
          cuckoo_mark_edge_occupied(x, edge_idx);
        }
      }
    }
  } else if (map_sts == BF_MAP_OK) {
    if (txn_node->entry_hdl == PIPE_MAT_ENT_HDL_INVALID_HDL) {
      /* If the entry handle pointed to by this entry idx is invalid, then
       * it implies that the entry index was empty prior to txn and hence
       * need to delete the entry index to entry handle mapping.
       */
      map_sts = bf_map_get_rmv(
          &exm_tbl_data->ent_idx_to_ent_hdl_htbl, key, (void **)&ent_hdl);
      if (map_sts != BF_MAP_OK) {
        LOG_ERROR(
            "%s:%d Error in removing entry idx to entry hdl mapping for idx "
            "%d, pipe id %d, err 0x%x",
            __func__,
            __LINE__,
            entry_idx,
            exm_tbl_data->pipe_id,
            map_sts);
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
      }

      if (!pipe_mgr_exm_is_ent_hdl_default(exm_tbl_data,
                                           (pipe_ent_hdl_t)ent_hdl)) {
        exm_stage_info = exm_tbl_data->stage_info_ptrs[txn_node->stage_id];
        if (exm_stage_info == NULL) {
          PIPE_MGR_DBGCHK(0);
          return PIPE_UNEXPECTED;
        }
        num_entries_per_wide_word =
            exm_stage_info->pack_format->num_entries_per_wide_word;
        subentry = stage_ent_idx % num_entries_per_wide_word;
        edge_idx = stage_ent_idx - subentry;
        J1U(Rc_int, exm_stage_info->PJ1Array[edge_idx], subentry);
        cuckoo_move_graph_t *x =
            pipe_mgr_exm_get_or_create_cuckoo_move_graph(exm_stage_info);
        cuckoo_mark_edge_free(x, edge_idx);
      }

      return PIPE_SUCCESS;
    }
  } else {
    LOG_ERROR(
        "%s:%d Error in getting entry idx to entry hdl mapping for idx %d, "
        "pipe id %d, err 0x%x",
        __func__,
        __LINE__,
        entry_idx,
        exm_tbl_data->pipe_id,
        map_sts);
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_exm_ent_idx_txn_abort(
    pipe_mgr_exm_tbl_t *exm_tbl, pipe_mgr_exm_tbl_data_t *exm_tbl_data) {
  pipe_status_t status = PIPE_SUCCESS;
  bf_map_sts_t map_sts = BF_MAP_OK;
  pipe_mgr_exm_idx_txn_node_t *txn_node = NULL;
  unsigned long key = 0;

  if (!exm_tbl_data) return PIPE_INVALID_ARG;

  while ((map_sts = bf_map_get_first_rmv(
              &exm_tbl_data->dirtied_ent_idx_htbl, &key, (void **)&txn_node)) ==
         BF_MAP_OK) {
    status |= pipe_mgr_exm_ent_idx_txn_abort_int(
        exm_tbl_data, txn_node, (pipe_mat_ent_idx_t)key);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in txn abort for entry idx %ld, tbl 0x%x, pipe "
          "id %d, device id %d, err %s",
          __func__,
          __LINE__,
          key,
          exm_tbl->mat_tbl_hdl,
          exm_tbl_data->pipe_id,
          exm_tbl->dev_id,
          pipe_str_err(status));
      PIPE_MGR_DBGCHK(0);
    }
    PIPE_MGR_FREE(txn_node);
  }
  return status;
}

pipe_status_t pipe_mgr_exm_tbl_txn_abort(bf_dev_id_t dev_id,
                                         pipe_mat_tbl_hdl_t tbl_hdl,
                                         bf_dev_pipe_t *pipes_list,
                                         unsigned nb_pipes) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  pipe_mgr_exm_stage_info_t *exm_stage_info = NULL;
  unsigned i, j, pipe;
  bool symmetric;

  exm_tbl = pipe_mgr_exm_tbl_get(dev_id, tbl_hdl);

  if (!exm_tbl) {
    LOG_ERROR(
        "%s : Could not find the exact match table info for table with"
        " handle %d, device id %d",
        __func__,
        tbl_hdl,
        dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Process only table session's related pipes. */
  i = 0;
  symmetric = false;
  while (i < nb_pipes && !symmetric) {
    if (exm_tbl->symmetric == true) {
      /* For symmetric table, we loop only once here. */
      symmetric = true;
      pipe = BF_DEV_PIPE_ALL;
    } else {
      pipe = pipes_list[i++];
    }
    exm_tbl_data = pipe_mgr_exm_tbl_get_instance(exm_tbl, pipe);
    if (!exm_tbl_data) continue;

    for (j = 0; j < exm_tbl_data->num_stages; j++) {
      exm_stage_info = &exm_tbl_data->exm_stage_info[j];
      cuckoo_move_graph_t *x =
          pipe_mgr_exm_get_or_create_cuckoo_move_graph(exm_stage_info);
      if (!x) {
        return PIPE_NO_SYS_RESOURCES;
      }
      cuckoo_move_graph_txn_abort(x);
    }

    status = pipe_mgr_exm_ent_hdl_txn_abort(exm_tbl, exm_tbl_data);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in entry handle txn abort for tbl 0x%x, device id %d, "
          "err "
          "%s",
          __func__,
          __LINE__,
          exm_tbl->mat_tbl_hdl,
          dev_id,
          pipe_str_err(status));
      PIPE_MGR_DBGCHK(0);
      return status;
    }

    status = pipe_mgr_exm_ent_idx_txn_abort(exm_tbl, exm_tbl_data);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in entry handle txn abort for tbl 0x%x, device id %d, "
          "err "
          "%s",
          __func__,
          __LINE__,
          exm_tbl->mat_tbl_hdl,
          dev_id,
          pipe_str_err(status));
      PIPE_MGR_DBGCHK(0);
      return status;
    }

    if (exm_tbl_data->default_entry_backed_up) {
      if (exm_tbl_data->backup_default_entry_hdl ==
          PIPE_MAT_ENT_HDL_INVALID_HDL) {
        pipe_mgr_exm_reset_def_ent_placed(exm_tbl_data);
      } else {
        pipe_mgr_exm_set_def_ent_placed(exm_tbl_data);
      }
      exm_tbl_data->default_entry_backed_up = false;
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_exm_ent_hdl_txn_add(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_mgr_exm_entry_info_t *entry_info,
    pipe_mgr_exm_operation_e operation,
    pipe_mat_ent_idx_t src_entry_idx,
    pipe_mat_ent_idx_t dst_entry_idx) {
  bf_map_sts_t map_sts = BF_MAP_OK;
  pipe_mgr_exm_txn_node_t *txn_node = NULL;
  pipe_mgr_exm_idx_txn_node_t *txn_node1 = NULL;
  unsigned long key = mat_ent_hdl;
  map_sts =
      bf_map_get(&exm_tbl_data->dirtied_ent_hdl_htbl, key, (void **)&txn_node);
  if (map_sts == BF_MAP_NO_KEY) {
    txn_node = (pipe_mgr_exm_txn_node_t *)PIPE_MGR_CALLOC(
        1, sizeof(pipe_mgr_exm_txn_node_t));
    if (txn_node == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      PIPE_MGR_DBGCHK(0);
      return PIPE_NO_SYS_RESOURCES;
    }
    txn_node->operation = operation;
    txn_node->mat_ent_hdl = mat_ent_hdl;
    txn_node->pipe_id = exm_tbl_data->pipe_id;
    txn_node->stage_id = exm_stage_info->stage_id;
    if (operation != PIPE_MGR_EXM_OPERATION_ADD) {
      txn_node->entry_info = (pipe_mgr_exm_entry_info_t *)PIPE_MGR_CALLOC(
          1, sizeof(pipe_mgr_exm_entry_info_t));
      if (txn_node->entry_info == NULL) {
        LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
        PIPE_MGR_DBGCHK(0);
        return PIPE_NO_SYS_RESOURCES;
      }
      PIPE_MGR_MEMCPY(
          txn_node->entry_info, entry_info, sizeof(pipe_mgr_exm_entry_info_t));
    }
    map_sts =
        bf_map_add(&exm_tbl_data->dirtied_ent_hdl_htbl, key, (void *)txn_node);
    if (map_sts != BF_MAP_OK) {
      LOG_ERROR(
          "%s:%d Error in adding entry hdl into txn htbl for entry%d, tbl "
          "0x%x, device id %d, err 0x%x",
          __func__,
          __LINE__,
          mat_ent_hdl,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          map_sts);
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
    }
    if (pipe_mgr_exm_is_ent_hdl_default(exm_tbl_data, mat_ent_hdl) &&
        !exm_tbl_data->default_entry_backed_up) {
      if (pipe_mgr_exm_is_default_ent_placed(exm_tbl_data)) {
        exm_tbl_data->backup_default_entry_hdl = mat_ent_hdl;
      } else {
        exm_tbl_data->backup_default_entry_hdl = PIPE_MAT_ENT_HDL_INVALID_HDL;
      }
      exm_tbl_data->default_entry_backed_up = true;
    }
  }
  if (operation != PIPE_MGR_EXM_OPERATION_MODIFY) {
    key = src_entry_idx;
    map_sts = bf_map_get(
        &exm_tbl_data->dirtied_ent_idx_htbl, key, (void **)&txn_node1);
    if (map_sts == BF_MAP_NO_KEY) {
      txn_node1 = (pipe_mgr_exm_idx_txn_node_t *)PIPE_MGR_CALLOC(
          1, sizeof(pipe_mgr_exm_idx_txn_node_t));
      if (txn_node1 == NULL) {
        LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
        PIPE_MGR_DBGCHK(0);
        return PIPE_NO_SYS_RESOURCES;
      }
      txn_node1->operation = operation;
      txn_node1->pipe_id = exm_tbl_data->pipe_id;
      txn_node1->stage_id = exm_stage_info->stage_id;
      if (operation == PIPE_MGR_EXM_OPERATION_ADD) {
        txn_node1->entry_hdl = PIPE_MAT_ENT_HDL_INVALID_HDL;
      } else {
        txn_node1->entry_hdl = mat_ent_hdl;
      }
      map_sts = bf_map_add(
          &exm_tbl_data->dirtied_ent_idx_htbl, key, (void *)txn_node1);
      if (map_sts != BF_MAP_OK) {
        LOG_ERROR(
            "%s:%d Error in adding entry idx %d into txn htbl for tbl 0x%x, "
            "device id %d, pipe id %d, err 0x%x",
            __func__,
            __LINE__,
            src_entry_idx,
            exm_tbl->mat_tbl_hdl,
            exm_tbl->dev_id,
            exm_tbl_data->pipe_id,
            map_sts);
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
      }
    }
    if (operation == PIPE_MGR_EXM_OPERATION_MOVE) {
      key = dst_entry_idx;
      map_sts = bf_map_get(
          &exm_tbl_data->dirtied_ent_idx_htbl, key, (void **)&txn_node1);
      if (map_sts == BF_MAP_NO_KEY) {
        txn_node1 = (pipe_mgr_exm_idx_txn_node_t *)PIPE_MGR_CALLOC(
            1, sizeof(pipe_mgr_exm_idx_txn_node_t));
        if (txn_node1 == NULL) {
          LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
          PIPE_MGR_DBGCHK(0);
          return PIPE_NO_SYS_RESOURCES;
        }
        txn_node1->operation = operation;
        txn_node1->entry_hdl = PIPE_MAT_ENT_HDL_INVALID_HDL;
        txn_node1->pipe_id = exm_tbl_data->pipe_id;
        txn_node1->stage_id = exm_stage_info->stage_id;
        map_sts = bf_map_add(
            &exm_tbl_data->dirtied_ent_idx_htbl, key, (void *)txn_node1);
        if (map_sts != BF_MAP_OK) {
          LOG_ERROR(
              "%s:%d Error in adding entry idx %d into txn htbl for tbl 0x%x, "
              "device id %d, pipe id %d, err 0x%x",
              __func__,
              __LINE__,
              dst_entry_idx,
              exm_tbl->mat_tbl_hdl,
              exm_tbl->dev_id,
              exm_tbl_data->pipe_id,
              map_sts);
          PIPE_MGR_DBGCHK(0);
          return PIPE_UNEXPECTED;
        }
      }
    }
  }
  return PIPE_SUCCESS;
}
