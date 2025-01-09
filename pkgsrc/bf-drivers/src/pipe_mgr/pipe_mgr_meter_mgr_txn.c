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
 * @file pipe_mgr_meter_txn.c
 * @date
 *
 * Meter table manager transaction implementation.
 */

/* Standard header includes */

/* Module header includes */
#include "pipe_mgr/pipe_mgr_intf.h"

/* Local header includes */
#include "pipe_mgr_int.h"
#include "pipe_mgr_meter_mgr_int.h"

pipe_status_t pipe_mgr_meter_update_txn_state(
    pipe_mgr_meter_txn_state_t *txn_state,
    uint8_t pipe_id,
    pipe_meter_idx_t meter_idx) {
  bf_map_sts_t map_sts = BF_MAP_OK;
  unsigned long key = ((uint64_t)pipe_id << 32 | meter_idx);
  pipe_mgr_meter_txn_state_node_t *txn_state_node = NULL;

  map_sts = bf_map_get(&txn_state->txn_map, key, (void **)&txn_state_node);

  if (map_sts != BF_MAP_OK) {
    txn_state_node = (pipe_mgr_meter_txn_state_node_t *)PIPE_MGR_CALLOC(
        1, sizeof(pipe_mgr_meter_txn_state_node_t));

    if (txn_state_node == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
  } else {
    /* Transaction state already exists */
    return PIPE_SUCCESS;
  }

  txn_state_node->meter_idx = meter_idx;
  txn_state_node->pipe_id = pipe_id;

  /* Now, insert into the transaction map */
  map_sts = bf_map_add(&txn_state->txn_map, key, (void *)txn_state_node);

  if (map_sts != BF_MAP_OK) {
    LOG_ERROR("%s:%d Error in adding txn state for index %d, pipe id %d",
              __func__,
              meter_idx,
              pipe_id);
    return PIPE_UNEXPECTED;
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_meter_backup_entry_info(
    pipe_mgr_meter_tbl_instance_t *meter_tbl_instance,
    pipe_meter_idx_t meter_idx) {
  pipe_mgr_meter_ent_info_node_t *ent_info_node = NULL;

  ent_info_node =
      pipe_mgr_meter_get_ent_info_node(meter_tbl_instance, meter_idx);

  if (ent_info_node == NULL) {
    LOG_ERROR("%s:%d Meter entry info node not found for idx %d",
              __func__,
              __LINE__,
              meter_idx);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* When a backup is initiatied, always expect a backup not to be present */
  PIPE_MGR_DBGCHK(ent_info_node->backup == NULL);

  /* Also, if the entry info itself is not present, no backup to be taken.
   * The caller would have ensured that, but do that here as well. As good
   * as a no-op.
   */
  if (ent_info_node->backup == NULL) {
    ent_info_node->backup = (pipe_mgr_meter_entry_info_t *)PIPE_MGR_CALLOC(
        1, sizeof(pipe_mgr_meter_entry_info_t));

    if (ent_info_node->backup == NULL) {
      LOG_ERROR("%s:%d : Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }

    /* Take a backup */
    PIPE_MGR_MEMCPY(ent_info_node->backup,
                    &ent_info_node->entry_info,
                    sizeof(pipe_mgr_meter_entry_info_t));
  }

  return PIPE_SUCCESS;
}
