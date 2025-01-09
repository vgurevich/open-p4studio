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
 * @file pipe_mgr_stat_mgr_dump.c
 * @date
 *
 * Stat table manager dump routines to dump debug information.
 */

/* Standard header includes */

/* Module header includes */
#include <pipe_mgr/pipe_mgr_intf.h>

/* Local header includes */
#include "pipe_mgr_int.h"
#include "pipe_mgr_stat_mgr_int.h"

#include <target-utils/uCli/ucli.h>
#include <target-utils/uCli/ucli_argparse.h>
#include <target-utils/uCli/ucli_handler_macros.h>

void pipe_mgr_stat_dump_tbl_info(ucli_context_t *uc,
                                 bf_dev_id_t device_id,
                                 pipe_stat_tbl_hdl_t stat_tbl_hdl) {
  pipe_mgr_stat_tbl_t *stat_tbl =
      pipe_mgr_stat_tbl_get(device_id, stat_tbl_hdl);
  if (stat_tbl == NULL) {
    aim_printf(&uc->pvs, "No information found for the stat table specified\n");
    return;
  }

  pipe_stat_tbl_info_t *stat_tbl_info =
      pipe_mgr_get_stat_tbl_info(device_id, stat_tbl_hdl, __func__, __LINE__);
  if (stat_tbl_info == NULL) {
    aim_printf(&uc->pvs,
               "No RMT information found for the stat table specified\n");
    return;
  }

  aim_printf(&uc->pvs,
             "Stat table info for tbl handle 0x%x, device id %d\n",
             stat_tbl_hdl,
             device_id);

  aim_printf(&uc->pvs, "-------------------------------------------\n");
  aim_printf(&uc->pvs, "Name: \"%s\"\n", stat_tbl->name);
  aim_printf(&uc->pvs, "ProfileId: %d\n", stat_tbl->profile_id);
  uint32_t x = 0, y;
  PIPE_BITMAP_ITER(&stat_tbl->pipe_bmp, y) { x |= 1u << y; }
  aim_printf(&uc->pvs, "Pipe Mask: 0x%x\n", x);

  if (stat_tbl->ref_type == PIPE_TBL_REF_TYPE_DIRECT) {
    aim_printf(&uc->pvs, "RefType: Direct\n");
  } else if (stat_tbl->ref_type == PIPE_TBL_REF_TYPE_INDIRECT) {
    aim_printf(&uc->pvs, "RefType: Indirect\n");
  } else {
    aim_printf(&uc->pvs, "RefType: Unknown\n");
  }
  /* Find all MATs to display the references. */
  rmt_dev_profile_info_t *prof_info = pipe_mgr_get_profile(
      stat_tbl->dev_info, stat_tbl->profile_id, __func__, __LINE__);
  if (prof_info) {
    for (uint8_t i = 0; i < prof_info->tbl_info_list.num_mat_tbls; ++i) {
      pipe_mat_tbl_info_t *mat = prof_info->tbl_info_list.mat_tbl_list + i;
      for (uint32_t j = 0; j < mat->num_stat_tbl_refs; ++j) {
        if (mat->stat_tbl_ref[j].tbl_hdl == stat_tbl->stat_tbl_hdl) {
          aim_printf(
              &uc->pvs,
              "Ref: %s 0x%x \"%s\"\n",
              mat->stat_tbl_ref[j].ref_type == PIPE_TBL_REF_TYPE_DIRECT
                  ? "Direct"
                  : mat->stat_tbl_ref[j].ref_type == PIPE_TBL_REF_TYPE_INDIRECT
                        ? "Indirect"
                        : "Unknown",
              mat->handle,
              mat->name);
        }
      }
    }
  }

  if (stat_tbl->counter_type == PACKET_COUNT) {
    aim_printf(&uc->pvs, "Counter type : PACKET\n");
    aim_printf(&uc->pvs,
               "Packet counter resolution %d\n",
               stat_tbl->packet_counter_resolution);
  } else if (stat_tbl->counter_type == BYTE_COUNT) {
    aim_printf(&uc->pvs, "Counter type : BYTE\n");
    aim_printf(&uc->pvs,
               "Byte counter resolution %d\n",
               stat_tbl->byte_counter_resolution);
  } else if (stat_tbl->counter_type == PACKET_AND_BYTE_COUNT) {
    aim_printf(&uc->pvs, "Counter type : PACKET AND BYTE\n");
    aim_printf(&uc->pvs,
               "Packet counter resolution %d\n",
               stat_tbl->packet_counter_resolution);
    aim_printf(&uc->pvs,
               "Byte counter resolution %d\n",
               stat_tbl->byte_counter_resolution);
  }
  if (stat_tbl->enable_per_flow_enable) {
    aim_printf(
        &uc->pvs, "PFE Bit: %d\n", stat_tbl->per_flow_enable_bit_position);
  } else {
    aim_printf(&uc->pvs, "PFE Bit: None\n");
  }

  uint32_t live_stages = 0;
  if (stat_tbl->stat_tbl_instances) {
    /* Dump some resource allocation info for the table.  We are using the first
     * instance since this information is the same for all instances. */
    pipe_mgr_stat_tbl_instance_t *inst = stat_tbl->stat_tbl_instances;
    aim_printf(&uc->pvs, "Capacity: %d\n", inst->capacity);
    aim_printf(&uc->pvs, "Actual Size: %d\n", inst->num_entries);
    for (uint8_t stage_idx = 0; stage_idx < inst->num_stages; ++stage_idx) {
      pipe_mgr_stat_tbl_stage_info_t *sinfo =
          &inst->stat_tbl_stage_info[stage_idx];
      aim_printf(&uc->pvs,
                 "Stage Info: stage_idx %d Stage-Id %d LT-Id %d\n",
                 stage_idx,
                 sinfo->stage_id,
                 sinfo->stage_table_handle);
      aim_printf(&uc->pvs, "  Num Entries: %d\n", sinfo->num_entries);
      aim_printf(&uc->pvs, "  Stage Idx Offset: %d\n", sinfo->ent_idx_offset);
      live_stages |= 1u << sinfo->stage_id;
      /* The memory allocation is only stored in the RMT info, not in the
       * stats_mgr struct, so go over all the per-stage RMT info for this table
       * to get the per-stage memory allocation. */
      aim_printf(&uc->pvs, "  Mem-ids:");
      for (unsigned i = 0; i < stat_tbl_info->num_rmt_info; ++i) {
        rmt_tbl_info_t *rmt_stage = stat_tbl_info->rmt_info + i;
        int num_printed = 0, num_per_line = 12;
        if (rmt_stage->stage_id != sinfo->stage_id ||
            rmt_stage->handle != sinfo->stage_table_handle)
          continue;
        for (int bank = 0; bank < rmt_stage->num_tbl_banks; ++bank) {
          rmt_tbl_bank_map_t *bank_map = rmt_stage->bank_map + bank;
          for (int block = 0; block < bank_map->num_tbl_word_blks; ++block) {
            if (num_printed == num_per_line) {
              num_printed = 0;
              aim_printf(&uc->pvs, "\n          ");
            }
            aim_printf(
                &uc->pvs, "  %2d", bank_map->tbl_word_blk[block].mem_id[0]);
            ++num_printed;
          }
        }
      }
      aim_printf(&uc->pvs, "\n  VPNs:   ");
      for (unsigned i = 0; i < stat_tbl_info->num_rmt_info; ++i) {
        rmt_tbl_info_t *rmt_stage = stat_tbl_info->rmt_info + i;
        int num_printed = 0, num_per_line = 12;
        if (rmt_stage->stage_id != sinfo->stage_id ||
            rmt_stage->handle != sinfo->stage_table_handle)
          continue;
        for (int bank = 0; bank < rmt_stage->num_tbl_banks; ++bank) {
          rmt_tbl_bank_map_t *bank_map = rmt_stage->bank_map + bank;
          for (int block = 0; block < bank_map->num_tbl_word_blks; ++block) {
            if (num_printed == num_per_line) {
              num_printed = 0;
              aim_printf(&uc->pvs, "\n          ");
            }
            aim_printf(
                &uc->pvs, "  %2d", bank_map->tbl_word_blk[block].vpn_id[0]);
            ++num_printed;
          }
        }
      }
      aim_printf(&uc->pvs, "\n  Spare RAM Mem-Ids:");
      for (unsigned i = 0; i < stat_tbl_info->num_rmt_info; ++i) {
        rmt_tbl_info_t *rmt_stage = stat_tbl_info->rmt_info + i;
        if (rmt_stage->stage_id != sinfo->stage_id ||
            rmt_stage->handle != sinfo->stage_table_handle)
          continue;
        for (int block = 0; block < rmt_stage->num_spare_rams; ++block) {
          aim_printf(&uc->pvs, "  %d", rmt_stage->spare_rams[block]);
        }
      }
      aim_printf(&uc->pvs, "\n");
    }
  } else {
    aim_printf(&uc->pvs,
               "Table %s (0x%x) has no instances\n",
               stat_tbl->name,
               stat_tbl->stat_tbl_hdl);
    return;
  }

  aim_printf(&uc->pvs, "Num of scopes : %d\n", stat_tbl->num_scopes);
  for (int j = 0; j < stat_tbl->num_scopes; j++) {
    aim_printf(
        &uc->pvs, "  Scope[%d] : 0x%x\n", j, stat_tbl->scope_pipe_bmp[j]);
  }

  aim_printf(&uc->pvs,
             "Stat table is %s\n",
             stat_tbl->symmetric ? "symmetric" : "asymmetric");
  for (unsigned i = 0; i < stat_tbl->num_instances; i++) {
    pipe_mgr_stat_tbl_instance_t *inst = &stat_tbl->stat_tbl_instances[i];
    aim_printf(&uc->pvs, "Stat table instance %d info\n", i);
    aim_printf(&uc->pvs, "----------------------------\n");
    aim_printf(&uc->pvs, "  Pipe: 0x%x\n", inst->pipe_id);
    aim_printf(&uc->pvs, "  Pipes in bitmap :");
    uint32_t pipe;
    PIPE_BITMAP_ITER(&inst->pipe_bmp, pipe) {
      aim_printf(&uc->pvs, " %d", pipe);
    }
    aim_printf(&uc->pvs, "\n");

    aim_printf(&uc->pvs, "Barrier Data\n");
    PIPE_MGR_LOCK(&inst->barrier_data_mtx);
    for (unsigned p = 0; p < stat_tbl->dev_info->num_active_pipes; ++p) {
      if (!PIPE_BITMAP_GET(&inst->pipe_bmp, p)) continue;
      if (!inst->barrier_list) continue;
      bool first_print = true;
      for (pipe_mgr_stat_barrier_list_node_t *node = inst->barrier_list[p];
           node;
           node = node->next) {
        if (first_print) {
          aim_printf(&uc->pvs, "Pipe %2d     : ", p);
          first_print = false;
        } else {
          aim_printf(&uc->pvs, "            : ");
        }
        pipe_mgr_stat_barrier_state_t *st = node->barrier_state;
        switch (st->operation) {
          case PIPE_MGR_STAT_ENTRY_DUMP_OP:
            aim_printf(&uc->pvs,
                       "Entry Dump: ref 0x%x stage %d id 0x%04x MAT 0x%x ent "
                       "%u idx %d pipe %X",
                       st->pipe_ref_map,
                       st->stage_id,
                       st->lock_id,
                       st->op_state.entry_dump.mat_tbl_hdl,
                       st->op_state.entry_dump.mat_ent_hdl,
                       st->op_state.entry_dump.ent_idx,
                       st->op_state.entry_dump.pipe_id);
            break;
          case PIPE_MGR_STAT_TBL_DUMP_OP:
            aim_printf(&uc->pvs,
                       "Table Dump: ref 0x%x stage %d id 0x%04x pipe %X",
                       st->pipe_ref_map,
                       st->stage_id,
                       st->lock_id,
                       st->op_state.tbl_dump.pipe_id);
            break;
          case PIPE_MGR_STAT_ENT_WRITE_OP:
            aim_printf(
                &uc->pvs,
                "Entry Write: ref 0x%x stage %d id 0x%04x idx %d pipe %X",
                st->pipe_ref_map,
                st->stage_id,
                st->lock_id,
                st->op_state.ent_write.ent_idx,
                st->op_state.ent_write.pipe_id);
            break;
          case PIPE_MGR_STAT_LOCK_OP:
            aim_printf(&uc->pvs,
                       "Lock: ref 0x%x stage %d id 0x%04x",
                       st->pipe_ref_map,
                       st->stage_id,
                       st->lock_id);
            break;
          case PIPE_MGR_STAT_UNLOCK_OP:
            aim_printf(&uc->pvs,
                       "Unlock: ref 0x%x stage %d id 0x%04x",
                       st->pipe_ref_map,
                       st->stage_id,
                       st->lock_id);
            break;
          default:
            aim_printf(&uc->pvs,
                       "Unknown (%d) refCnt %d",
                       st->operation,
                       st->pipe_ref_map);
            break;
        }
        aim_printf(&uc->pvs, "%s\n", node->received_hw_ack ? " HW ACKED" : "");

        for (pipe_mgr_stat_mgr_task_node_t *t = node->task_list; t;
             t = t->next) {
          aim_printf(&uc->pvs, "            :     ");
          switch (t->type) {
            case PIPE_MGR_STAT_TASK_MOVE:
              aim_printf(&uc->pvs,
                         "Task Move Hdl %6d %2d.%5d --> %2d.%5d\n",
                         t->u.move_node.mat_ent_hdl,
                         t->u.move_node.src_stage_id,
                         t->u.move_node.src_ent_idx,
                         t->u.move_node.dst_stage_id,
                         t->u.move_node.dst_ent_idx);
              break;
            case PIPE_MGR_STAT_TASK_ENTRY_ADD:
              aim_printf(&uc->pvs,
                         "Task Add Hdl %6u index %d stage %d\n",
                         t->u.ent_add_node.mat_ent_hdl,
                         t->u.ent_add_node.stage_ent_idx,
                         t->u.ent_add_node.stage_id);
              break;
            case PIPE_MGR_STAT_TASK_ENTRY_DEL:
              aim_printf(&uc->pvs,
                         "Task Del Hdl %6d index %d\n",
                         t->u.ent_del_node.ent_hdl,
                         t->u.ent_del_node.stage_ent_idx);
              break;
          }
        }
        for (pipe_mgr_stat_deferred_dump_t *d = node->dump_list; d;
             d = d->next) {
          aim_printf(&uc->pvs,
                     "Deferred Dump/Evict: Index %d count %" PRIu64 " %" PRIu64
                     "\n",
                     d->stat_ent_idx,
                     d->stat_data.packets,
                     d->stat_data.bytes);
        }
      }
      for (dev_stage_t stg = 0; stg < stat_tbl->dev_info->num_active_mau;
           ++stg) {
        if (inst->def_barrier_node[p][stg]) {
          aim_printf(&uc->pvs, "Stage %2d: Deferred Barrier List Node\n", stg);
        }
      }
    }
    aim_printf(&uc->pvs, "\n");
    PIPE_MGR_UNLOCK(&inst->barrier_data_mtx);
  }

  return;
}

void pipe_mgr_stat_dump_entry_info(ucli_context_t *uc,
                                   bf_dev_id_t device_id,
                                   bf_dev_pipe_t pipe_id,
                                   pipe_stat_tbl_hdl_t stat_tbl_hdl,
                                   pipe_stat_ent_idx_t stat_ent_idx) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_stat_tbl_t *stat_tbl = NULL;
  pipe_mgr_stat_tbl_instance_t *stat_tbl_instance = NULL;
  pipe_stat_stage_ent_idx_t stage_ent_idx = 0;
  bf_dev_pipe_t pipe = 0;
  uint8_t stage_id = 0;
  pipe_stat_data_t local_stat_data = {0};
  pipe_stat_data_t ret_stat_data = {0};
  pipe_bitmap_t pipe_bmp;
  PIPE_BITMAP_INIT(&pipe_bmp, PIPE_BMP_SIZE);

  stat_tbl = pipe_mgr_stat_tbl_get(device_id, stat_tbl_hdl);

  if (stat_tbl == NULL) {
    aim_printf(&uc->pvs, "No information found for the stat table specified\n");
    return;
  }

  if (stat_tbl->symmetric == true) {
    /* If the stat table is a symmetric stat table, the pipe-id specified
     * must be BF_DEV_PIPE_ALL
     */
    if (pipe_id != BF_DEV_PIPE_ALL) {
      aim_printf(&uc->pvs, "Invalid Pipe-id specified for a symmetric table\n");
      return;
    }
  }

  stat_tbl_instance = pipe_mgr_stat_tbl_get_instance(stat_tbl, pipe_id);

  if (stat_tbl_instance == NULL) {
    aim_printf(&uc->pvs,
               "No information found for table specified for pipe-id %d\n",
               pipe_id);
    return;
  }

  if (stat_tbl->symmetric) {
    PIPE_BITMAP_ASSIGN(&pipe_bmp, &(stat_tbl->pipe_bmp));
  } else {
    PIPE_BITMAP_SET(&pipe_bmp, stat_tbl_instance->pipe_id);
  }

  PIPE_BITMAP_ITER(&pipe_bmp, pipe) {
    stage_id = pipe_mgr_stat_mgr_ent_get_stage(
        stat_tbl, stat_tbl_instance, stat_ent_idx, 0xff, &stage_ent_idx);
    while (stage_id != 0xff) {
      PIPE_MGR_MEMSET(&local_stat_data, 0, sizeof(pipe_stat_data_t));
      status = pipe_mgr_stat_mgr_get_ent_idx_count(stat_tbl,
                                                   stat_tbl_instance,
                                                   pipe,
                                                   stage_id,
                                                   stage_ent_idx,
                                                   &local_stat_data);
      if (status != PIPE_SUCCESS) {
        aim_printf(&uc->pvs, "Internal error encountered\n");
        return;
      }
      ret_stat_data.packets += local_stat_data.packets;
      ret_stat_data.bytes += local_stat_data.bytes;

      stage_id = pipe_mgr_stat_mgr_ent_get_stage(
          stat_tbl, stat_tbl_instance, stat_ent_idx, stage_id, &stage_ent_idx);
    }
  }

  if (stat_tbl->counter_type == PACKET_COUNT) {
    aim_printf(&uc->pvs, "Packet Count : %" PRIu64 "\n", ret_stat_data.packets);
  } else if (stat_tbl->counter_type == BYTE_COUNT) {
    aim_printf(&uc->pvs, "Byte Count : %" PRIu64 "\n", ret_stat_data.bytes);
  } else if (stat_tbl->counter_type == PACKET_AND_BYTE_COUNT) {
    aim_printf(&uc->pvs, "Packet Count : %" PRIu64 "\n", ret_stat_data.packets);
    aim_printf(&uc->pvs, "Byte Count : %" PRIu64 "\n", ret_stat_data.bytes);
  }

  return;
}
