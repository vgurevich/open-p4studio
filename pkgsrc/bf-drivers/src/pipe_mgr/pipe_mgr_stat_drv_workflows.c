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
 * @file pipe_mgr_stat_drv_workflows.c
 * @date
 *
 *
 * Contains core interactions with the hardware for statistics management
 */

/* Global header includes */

/* Module header includes */
#include <dvm/bf_drv_intf.h>
#include <pipe_mgr/pipe_mgr_intf.h>
#include <lld/bf_dma_if.h>
#include <lld/lld_reg_if.h>

/* Local header includes */
#include "pipe_mgr_int.h"
#include "pipe_mgr_stat_mgr_int.h"
#include "pipe_mgr_stat_drv_workflows.h"
#include "pipe_mgr_drv.h"
#include "pipe_mgr_drv_intf.h"

extern bool stat_mgr_enable_detail_trace;

static inline uint64_t pipe_mgr_stat_get_barrier_id(
    pipe_mgr_stat_tbl_instance_t *stat_tbl_instance) {
  /* We use just a single barrier id in stats. Barrier ACKS come back in
   * order they were issued, so we rely on this to retrieve the barrier state
   * for a given barrier ACK. The ordering of Barrier ACK is guaranteed in a
   * given pipe and stage. */
  uint32_t barr_id = stat_tbl_instance->next_barrier_id++;
  uint64_t rv;
  PIPE_MGR_FORM_LOCK_ID(rv, LOCK_ID_TYPE_STAT_BARRIER, barr_id);
  return rv;
}

pipe_status_t pipe_mgr_stat_mgr_ent_dump_drv_workflow(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mgr_stat_tbl_t *stat_tbl,
    pipe_mgr_stat_tbl_instance_t *stat_tbl_instance,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_stat_ent_idx_t stat_ent_idx,
    pipe_mgr_stat_ent_worklist_t *worklist) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_stat_tbl_stage_info_t *stat_tbl_stage_info = NULL;
  pipe_bitmap_t *pipe_bmp = &stat_tbl_instance->pipe_bmp;
  rmt_virt_addr_t ent_virt_addr = 0;
  bf_dev_pipe_t pipe_id = dev_tgt.dev_pipe_id;

  /* Loop over all stages passed in by the caller. */
  for (; worklist; worklist = worklist->next) {
    dev_stage_t stage_id = worklist->stage_id;
    lock_id_t barrier_id = pipe_mgr_stat_get_barrier_id(stat_tbl_instance);
    pipe_mgr_stat_barrier_state_t *barrier_state =
        pipe_mgr_stat_mgr_alloc_barrier_state(pipe_bmp, stage_id, barrier_id);
    if (barrier_state == NULL) {
      LOG_ERROR("%s/%d : Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    barrier_state->operation = PIPE_MGR_STAT_ENTRY_DUMP_OP;
    barrier_state->op_state.entry_dump.ent_idx = stat_ent_idx;
    barrier_state->op_state.entry_dump.pipe_id = pipe_id;
    barrier_state->op_state.entry_dump.mat_tbl_hdl = mat_tbl_hdl;
    barrier_state->op_state.entry_dump.mat_ent_hdl = mat_ent_hdl;

    stat_tbl_stage_info =
        pipe_mgr_stat_mgr_get_stage_info(stat_tbl, pipe_id, stage_id);
    if (!stat_tbl_stage_info) {
      LOG_ERROR(
          "%s : Stat table stage info not found. Tbl 0x%x, pipe id %x, stage "
          "id %d",
          __func__,
          stat_tbl->stat_tbl_hdl,
          pipe_id,
          stage_id);
      PIPE_MGR_DBGCHK(stat_tbl_stage_info);
      return PIPE_UNEXPECTED;
    }

    uint8_t ltbl_id = stat_tbl_stage_info->stage_table_handle;

    ent_virt_addr = pipe_mgr_stat_mgr_compute_ent_virt_addr(
        stat_tbl_stage_info, worklist->entry_idx);

    /* Construct the stat entry dump instruction */
    pipe_dump_stat_ent_instr_t ent_dump_instr;
    construct_instr_stat_dump_entry(
        dev_tgt.device_id, &ent_dump_instr, ent_virt_addr, ltbl_id);
    status = pipe_mgr_drv_ilist_add(&sess_hdl,
                                    stat_tbl->dev_info,
                                    pipe_bmp,
                                    stage_id,
                                    (uint8_t *)&ent_dump_instr,
                                    sizeof(pipe_dump_stat_ent_instr_t));
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s/%d: Error in adding stat ent dump instruction to ilist"
          " tbl %d, entry idx %d",
          __func__,
          __LINE__,
          stat_tbl->stat_tbl_hdl,
          worklist->entry_idx);
      return status;
    }

    if (stat_mgr_enable_detail_trace) {
      PIPE_MGR_STAT_TRACE(
          dev_tgt.device_id,
          stat_tbl->stat_tbl_hdl,
          stat_tbl->name,
          stat_ent_idx,
          -1,
          pipe_id,
          stage_id,
          "Issuing entry dump instruction for entry idx %d, stage id %d",
          stat_ent_idx,
          stage_id);
    }

    /* Now store state barrier state in the queue of barrier state in this pipe
     * and stage */
    pipe_mgr_stat_mgr_add_barrier_state(
        stat_tbl, stat_tbl_instance, pipe_bmp, barrier_state);
    /* Now form the barrier instruction */
    pipe_barrier_lock_instr_t barrier_instr;
    construct_instr_barrier_stats(
        dev_tgt.device_id, &barrier_instr, barrier_state->lock_id, ltbl_id);
    status = pipe_mgr_drv_ilist_add(&sess_hdl,
                                    stat_tbl->dev_info,
                                    pipe_bmp,
                                    stage_id,
                                    (uint8_t *)&barrier_instr,
                                    sizeof(pipe_barrier_lock_instr_t));

    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s/%d: Error in adding barrier instruction to ilist tbl 0x%x, "
          "entry idx %d",
          __func__,
          __LINE__,
          stat_tbl->stat_tbl_hdl,
          worklist->entry_idx);
      return status;
    }

    if (stat_mgr_enable_detail_trace) {
      PIPE_MGR_STAT_TRACE(
          dev_tgt.device_id,
          stat_tbl->stat_tbl_hdl,
          stat_tbl->name,
          stat_ent_idx,
          worklist->entry_idx,
          dev_tgt.dev_pipe_id,
          stage_id,
          "Adding barrier instruction for idx %d with barrier_id "
          "0x%x, stage id %d",
          stat_ent_idx,
          barrier_id,
          stage_id);
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_stat_tbl_dump_drv_workflow(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mgr_stat_tbl_t *stat_tbl,
    pipe_mgr_stat_tbl_instance_t *stat_tbl_instance,
    pipe_mgr_stat_tbl_sync_cback_fn cback_fn,
    void *cookie) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_bitmap_t *pipe_bmp = &stat_tbl_instance->pipe_bmp;
  bf_dev_pipe_t pipe_id = dev_tgt.dev_pipe_id;

  /* The instruction needs to be issued in each of the stage where the
   * table is present.  */
  unsigned stage_idx = 0;
  for (stage_idx = 0; stage_idx < stat_tbl_instance->num_stages; stage_idx++) {
    lock_id_t barrier_id = pipe_mgr_stat_get_barrier_id(stat_tbl_instance);

    pipe_mgr_stat_tbl_stage_info_t *stat_tbl_stage_info = NULL;
    stat_tbl_stage_info = &stat_tbl_instance->stat_tbl_stage_info[stage_idx];
    dev_stage_t stage_id = stat_tbl_stage_info->stage_id;
    uint8_t ltbl_id = stat_tbl_stage_info->stage_table_handle;

    pipe_mgr_stat_barrier_state_t *barrier_state = NULL;
    barrier_state =
        pipe_mgr_stat_mgr_alloc_barrier_state(pipe_bmp, stage_id, barrier_id);
    if (barrier_state == NULL) {
      LOG_ERROR("%s/%d : Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    barrier_state->operation = PIPE_MGR_STAT_TBL_DUMP_OP;
    barrier_state->op_state.tbl_dump.pipe_id = pipe_id;
    /* Only attach the callback to the final stage's barrier state to ensure it
     * is called just once after all stages have completed their dumps. */
    if (stage_idx == stat_tbl_instance->num_stages - 1u) {
      barrier_state->op_state.tbl_dump.callback_fn = cback_fn;
      barrier_state->op_state.tbl_dump.user_cookie = cookie;
    }

    pipe_dump_stat_tbl_instr_t tbl_dump_instr;
    construct_instr_stat_dump_tbl(dev_tgt.device_id, &tbl_dump_instr, ltbl_id);
    status = pipe_mgr_drv_ilist_add(&sess_hdl,
                                    stat_tbl->dev_info,
                                    pipe_bmp,
                                    stage_id,
                                    (uint8_t *)&tbl_dump_instr,
                                    sizeof(pipe_dump_stat_tbl_instr_t));
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d : Error in adding stat tbl dump instruction to ilist"
          " tbl 0x%x, device_id %d pipe id %x, stage id %d",
          __func__,
          __LINE__,
          stat_tbl->stat_tbl_hdl,
          dev_tgt.device_id,
          dev_tgt.dev_pipe_id,
          stage_id);
      return status;
    }

    if (stat_mgr_enable_detail_trace) {
      PIPE_MGR_STAT_TRACE(
          dev_tgt.device_id,
          stat_tbl->stat_tbl_hdl,
          stat_tbl->name,
          -1,
          -1,
          pipe_id,
          stage_id,
          "Adding table dump instruction for tbl 0x%x, stage id %d",
          stat_tbl->stat_tbl_hdl,
          stage_id);
    }

    pipe_mgr_stat_mgr_add_barrier_state(
        stat_tbl, stat_tbl_instance, pipe_bmp, barrier_state);

    /* Now form the barrier instruction */
    pipe_barrier_lock_instr_t barrier_instr;
    construct_instr_barrier_stats(
        dev_tgt.device_id, &barrier_instr, barrier_id, ltbl_id);
    status = pipe_mgr_drv_ilist_add(&sess_hdl,
                                    stat_tbl->dev_info,
                                    pipe_bmp,
                                    stage_id,
                                    (uint8_t *)&barrier_instr,
                                    sizeof(pipe_barrier_lock_instr_t));
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s/%d: Error in adding barrier instruction to ilist"
          " tbl 0x%x, device id %d",
          __func__,
          __LINE__,
          stat_tbl->stat_tbl_hdl,
          dev_tgt.device_id);
      return status;
    }

    if (stat_mgr_enable_detail_trace) {
      PIPE_MGR_STAT_TRACE(
          dev_tgt.device_id,
          stat_tbl->stat_tbl_hdl,
          stat_tbl->name,
          -1,
          -1,
          dev_tgt.dev_pipe_id,
          stage_id,
          "Adding barrier instruction for tbl 0x%x, stage id %d, "
          "barrier id 0x%x",
          stat_tbl->stat_tbl_hdl,
          stage_id,
          barrier_id);
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_stat_mgr_ent_write_drv_workflow(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_pipe_t pipe_id,
    pipe_mgr_stat_tbl_t *stat_tbl,
    pipe_mgr_stat_tbl_instance_t *stat_tbl_instance,
    pipe_mgr_stat_ent_worklist_t *worklist,
    pipe_stat_data_t *stat_data) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_bitmap_t *pipe_bmp = NULL;
  rmt_virt_addr_t ent_virt_addr = 0;
  rmt_ram_line_t ram_line;
  uint8_t subword = 0;

  if (!worklist || !stat_data) {
    LOG_ERROR("%s:%d Null pointer arguments passed", __func__, __LINE__);
    return PIPE_INVALID_ARG;
  }

  PIPE_MGR_MEMSET(ram_line, 0, sizeof(rmt_ram_line_t));

  pipe_bmp = &stat_tbl_instance->pipe_bmp;

  for (; worklist; worklist = worklist->next) {
    dev_stage_t stage_id = worklist->stage_id;
    lock_id_t barrier_id = pipe_mgr_stat_get_barrier_id(stat_tbl_instance);

    pipe_mgr_stat_barrier_state_t *barrier_state =
        pipe_mgr_stat_mgr_alloc_barrier_state(pipe_bmp, stage_id, barrier_id);
    if (barrier_state == NULL) {
      LOG_ERROR("%s/%d : Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    barrier_state->operation = PIPE_MGR_STAT_ENT_WRITE_OP;
    barrier_state->op_state.ent_write.pipe_id = pipe_id;
    barrier_state->op_state.ent_write.ent_idx = worklist->entry_idx;

    pipe_mgr_stat_tbl_stage_info_t *stat_tbl_stage_info = NULL;
    stat_tbl_stage_info =
        pipe_mgr_stat_mgr_get_stage_info(stat_tbl, pipe_id, stage_id);
    if (!stat_tbl_stage_info) {
      LOG_ERROR(
          "%s : Stat table stage info not found. Tbl 0x%x, pipe id %x, stage "
          "id %d",
          __func__,
          stat_tbl->stat_tbl_hdl,
          pipe_id,
          stage_id);
      PIPE_MGR_DBGCHK(stat_tbl_stage_info);
      return PIPE_UNEXPECTED;
    }

    uint8_t ltbl_id = stat_tbl_stage_info->stage_table_handle;

    /* Do a virtual write to update the entry in the counter table. */
    ent_virt_addr = pipe_mgr_stat_mgr_compute_ent_virt_addr(
        stat_tbl_stage_info, worklist->entry_idx);
    subword = worklist->entry_idx % stat_tbl_stage_info->num_entries_per_line;
    pipe_mgr_stat_mgr_encode_stat_entry(stat_tbl,
                                        stat_tbl_stage_info,
                                        stage_id,
                                        subword,
                                        stat_data->bytes,
                                        stat_data->packets,
                                        &ram_line);
    pipe_instr_set_memdata_v_t instr;
    construct_instr_set_v_memdata(stat_tbl->device_id,
                                  &instr,
                                  (uint8_t *)ram_line,
                                  TOF_SRAM_UNIT_WIDTH / 8,
                                  ltbl_id,
                                  pipe_virt_mem_type_stat,
                                  ent_virt_addr);
    status = pipe_mgr_drv_ilist_add(&sess_hdl,
                                    stat_tbl->dev_info,
                                    pipe_bmp,
                                    stage_id,
                                    (uint8_t *)&instr,
                                    sizeof(pipe_instr_set_memdata_v_t));

    if (status != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Error in ilist add, err %s",
                __func__,
                __LINE__,
                pipe_str_err(status));
      return status;
    }

    /* Follow the virtual write (to set the stats entry) with a barrier and mark
     * the entry as user-set-in-progress.  Any notifications for the entry
     * received before the barrier would be ignored.  First setup barrier state,
     * then increment a count of outstanding barriers, then mark the entry, and
     * finally do the ilist-add to post the barrier instruction. */
    pipe_mgr_stat_mgr_add_barrier_state(
        stat_tbl, stat_tbl_instance, pipe_bmp, barrier_state);

    /* Now form the barrier instruction */
    pipe_barrier_lock_instr_t barrier_instr;
    construct_instr_barrier_stats(
        stat_tbl->device_id, &barrier_instr, barrier_id, ltbl_id);
    status = pipe_mgr_drv_ilist_add(&sess_hdl,
                                    stat_tbl->dev_info,
                                    pipe_bmp,
                                    stage_id,
                                    (uint8_t *)&barrier_instr,
                                    sizeof(pipe_barrier_lock_instr_t));

    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s/%d: Error in adding barrier instruction to ilist"
          " tbl %#x, entry idx %d",
          __func__,
          __LINE__,
          stat_tbl->stat_tbl_hdl,
          worklist->entry_idx);
      return status;
    }
  }

  return PIPE_SUCCESS;
}

pipe_mgr_stat_barrier_state_t *pipe_mgr_stat_mgr_alloc_barrier_state(
    pipe_bitmap_t *pipe_bmp, dev_stage_t stage_id, lock_id_t lock_id) {
  pipe_mgr_stat_barrier_state_t *barrier_state = NULL;
  barrier_state = PIPE_MGR_CALLOC(1, sizeof *barrier_state);
  if (barrier_state == NULL) {
    LOG_ERROR("%s/%d : Malloc failure", __func__, __LINE__);
    return NULL;
  }
  barrier_state->pipe_ref_map = 0;
  bf_dev_pipe_t pipe_iter;
  PIPE_BITMAP_ITER(pipe_bmp, pipe_iter) {
    barrier_state->pipe_ref_map |= 1u << pipe_iter;
  }
  barrier_state->stage_id = stage_id;
  barrier_state->lock_id = lock_id;
  return barrier_state;
}
