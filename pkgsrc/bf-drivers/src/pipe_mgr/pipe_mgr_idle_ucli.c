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


/*******************************************************************************
 *
 *
 *
 *****************************************************************************/
/* Standard includes */
#include <getopt.h>
#include <limits.h>

/* Module includes */
#include <pipe_mgr/pipe_mgr_config.h>
#include <dvm/bf_drv_intf.h>
#include <lld/bf_dma_if.h>

/* Local includes */
#include "pipe_mgr_int.h"
#include "pipe_mgr_drv.h"
#include "pipe_mgr_idle.h"
#include "pipe_mgr_idle_api.h"
#include "pipe_mgr_idle_sweep.h"

#if PIPE_MGR_CONFIG_INCLUDE_UCLI == 1

#include <target-utils/uCli/ucli.h>
#include <target-utils/uCli/ucli_argparse.h>
#include <target-utils/uCli/ucli_handler_macros.h>

#define PIPE_MGR_IDLE_CLI_CMD_HNDLR(name) pipe_mgr_idle_ucli_ucli__##name##__
#define PIPE_MGR_IDLE_CLI_CMD_DECLARE(name) \
  static ucli_status_t PIPE_MGR_IDLE_CLI_CMD_HNDLR(name)(ucli_context_t * uc)

pipe_status_t pipe_mgr_idle_entry_dump_one(ucli_context_t *uc,
                                           idle_tbl_info_t *idle_tbl_info,
                                           idle_entry_t *ient) {
  const char *indent = "\t";
  aim_printf(&uc->pvs, "%s%25s: %d\n", indent, "Index", ient->index);
  if (IDLE_TBL_IS_NOTIFY_MODE(idle_tbl_info)) {
    aim_printf(
        &uc->pvs, "%s%25s: %s\n", indent, "Inuse", ient->inuse ? "Y" : "N");
    aim_printf(&uc->pvs, "%s%25s: 0x%-8x\n", indent, "Ent hdl", ient->ent_hdl);
    aim_printf(&uc->pvs,
               "%s%25s: %s\n",
               indent,
               "Notify state",
               ientry_notify_state_str(ient->notify_state));
    aim_printf(&uc->pvs, "%s%25s: %d ms\n", indent, "Init TTL", ient->init_ttl);
    aim_printf(
        &uc->pvs, "%s%25s: %d ms\n", indent, "Current TTL", ient->cur_ttl);
  } else {
    aim_printf(&uc->pvs,
               "%s%25s: %s\n",
               indent,
               "Poll state",
               (ient->poll_state == ENTRY_IDLE) ? "IDLE" : "ACTIVE");
    aim_printf(
        &uc->pvs, "%s%25s: %d\n", indent, "Update count", ient->update_count);
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_idle_ucli_dump_ent_mdata_info(
    ucli_context_t *uc,
    idle_tbl_stage_info_t *stage_info,
    pipe_mat_ent_hdl_t ent_hdl) {
  idle_tbl_info_t *idle_tbl_info = NULL;
  idle_tbl_info = stage_info->idle_tbl_p->idle_tbl_info;

  idle_entry_location_t *ils = NULL, *il = NULL;
  if (!pipe_mgr_idle_entry_mdata_get(stage_info, ent_hdl, NULL, NULL, &ils)) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }

  aim_printf(&uc->pvs, "Locations\n");
  aim_printf(
      &uc->pvs, "%-5s|%-5s|%-5s|%-5s\n", "-----", "-----", "-----", "-----");
  aim_printf(&uc->pvs, "%-5s|%-5s|%-5s|%-5s\n", "Valid", "Cur", "Dest", "Del");
  aim_printf(
      &uc->pvs, "%-5s|%-5s|%-5s|%-5s\n", "-----", "-----", "-----", "-----");
  for (il = ils; il; il = il->n) {
    aim_printf(&uc->pvs,
               "%-5s|%-5d|%-5d|%-5s\n",
               il->index_valid ? "Y" : "N",
               il->cur_index,
               il->dest_index,
               il->del_in_progress ? "Y" : "N");
  }

  for (il = ils; il; il = il->n) {
    if (il->del_in_progress || (il->index_valid == false)) {
      continue;
    }
    uint32_t index = il->cur_index;
    idle_entry_t *ient = pipe_mgr_idle_entry_get_by_index(stage_info, index);
    if (!ient) {
      aim_printf(&uc->pvs, "%s:%d Error, `ient` is NULL\n", __func__, __LINE__);
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
    }
    pipe_mgr_idle_entry_dump_one(uc, idle_tbl_info, ient);
  }
  destroy_ils(ils);
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_idle_ucli_dump_ent_info(ucli_context_t *uc,
                                               bf_dev_id_t dev_id,
                                               pipe_mat_tbl_hdl_t tbl_hdl,
                                               pipe_mat_ent_hdl_t ent_hdl) {
  idle_tbl_info_t *idle_tbl_info = NULL;
  idle_tbl_info = pipe_mgr_idle_tbl_info_get(dev_id, tbl_hdl);
  if (!idle_tbl_info) {
    aim_printf(
        &uc->pvs, "Table 0x%x on device %d does not exists\n", tbl_hdl, dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (!idle_tbl_info->enabled) {
    aim_printf(&uc->pvs,
               "%s(%d - 0x%x) Idle timeouts are not enabled\n",
               idle_tbl_info->name,
               idle_tbl_info->dev_id,
               idle_tbl_info->tbl_hdl);
    return PIPE_INVALID_ARG;
  }

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    aim_printf(
        &uc->pvs, "`dev_info` is not found for dev_id [%d]\n", (int)dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  pipe_status_t rc = PIPE_SUCCESS;
  bf_dev_pipe_t pipe_id = 0;
  dev_stage_t stage_id = 0;
  rmt_tbl_hdl_t stage_table_handle = 0;
  uint32_t subindex = 0;
  rc = pipe_mgr_mat_ent_get_dir_ent_location(dev_id,
                                             tbl_hdl,
                                             ent_hdl,
                                             subindex,
                                             &pipe_id,
                                             &stage_id,
                                             &stage_table_handle,
                                             NULL);
  if (rc != PIPE_SUCCESS) {
    aim_printf(&uc->pvs,
               "%s (%d-0x%x) "
               "Mat entry 0x%x not found.\n",
               idle_tbl_info->name,
               idle_tbl_info->dev_id,
               idle_tbl_info->tbl_hdl,
               ent_hdl);
    return PIPE_INVALID_ARG;
  }

  for (subindex = 1; rc == PIPE_SUCCESS; subindex++) {
    if (IDLE_TBL_IS_NOTIFY_MODE(idle_tbl_info)) {
      pipe_bitmap_t local_pipe_bmp;
      PIPE_BITMAP_INIT(&local_pipe_bmp, PIPE_BMP_SIZE);
      pipe_mgr_idle_tbl_notify_pipe_list_get(
          idle_tbl_info, pipe_id, &local_pipe_bmp);
      /* We have entries in each pipe */
      int pipe = -1;
      for (pipe = PIPE_BITMAP_GET_NEXT_BIT(&local_pipe_bmp, pipe); pipe != -1;
           pipe = PIPE_BITMAP_GET_NEXT_BIT(&local_pipe_bmp, pipe)) {
        aim_printf(&uc->pvs, "Pipe: %-2d\tStage: %-2d\n", pipe, stage_id);

        PIPE_MGR_DBGCHK(pipe < (int)dev_info->num_active_pipes);
        idle_tbl_stage_info_t *stage_info = NULL;
        stage_info = pipe_mgr_idle_tbl_stage_info_get(
            idle_tbl_info, pipe, stage_id, stage_table_handle);
        if (!stage_info) {
          aim_printf(&uc->pvs,
                     "%s (%d-0x%x) "
                     "Idle table stage %d does not exist on pipe %d\n",
                     idle_tbl_info->name,
                     idle_tbl_info->dev_id,
                     idle_tbl_info->tbl_hdl,
                     stage_id,
                     pipe);
          return PIPE_OBJ_NOT_FOUND;
        }

        pipe_mgr_idle_ucli_dump_ent_mdata_info(uc, stage_info, ent_hdl);
      }
    } else {
      aim_printf(&uc->pvs, "Pipe: %-2d\tStage: %-2d\n", pipe_id, stage_id);

      idle_tbl_stage_info_t *stage_info = NULL;
      stage_info = pipe_mgr_idle_tbl_stage_info_get(
          idle_tbl_info, pipe_id, stage_id, stage_table_handle);
      if (!stage_info) {
        aim_printf(&uc->pvs,
                   "%s (%d-0x%x) "
                   "Idle table stage %d does not exist on pipe_id %d\n",
                   idle_tbl_info->name,
                   idle_tbl_info->dev_id,
                   idle_tbl_info->tbl_hdl,
                   stage_id,
                   pipe_id);
        return PIPE_OBJ_NOT_FOUND;
      }

      pipe_mgr_idle_ucli_dump_ent_mdata_info(uc, stage_info, ent_hdl);
    }
    rc = pipe_mgr_mat_ent_get_dir_ent_location(dev_id,
                                               tbl_hdl,
                                               ent_hdl,
                                               subindex,
                                               &pipe_id,
                                               &stage_id,
                                               &stage_table_handle,
                                               NULL);
  }

  return PIPE_SUCCESS;
}

PIPE_MGR_IDLE_CLI_CMD_DECLARE(ent_info) {
  PIPE_MGR_CLI_PROLOGUE("ent-info",
                        "Print the idle entry info",
                        "-d <dev_id> -h <tbl_handle> -e <ent_handle>");

  bool got_dev = false;
  bool got_tbl_hdl = false;
  bool got_ent_hdl = false;

  bf_dev_id_t dev_id = 0;
  pipe_mat_tbl_hdl_t tbl_hdl = 0;
  pipe_mat_ent_hdl_t ent_hdl = 0;

  int x;
  while (-1 != (x = getopt(argc, argv, "d:h:e:"))) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev_id = strtoul(optarg, NULL, 0);
        got_dev = true;
        break;
      case 'h':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        tbl_hdl = strtoul(optarg, NULL, 0);
        got_tbl_hdl = true;
        break;
      case 'e':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        ent_hdl = strtoul(optarg, NULL, 0);
        got_ent_hdl = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }
  if (!got_dev || !got_tbl_hdl || !got_ent_hdl) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  pipe_mgr_idle_ucli_dump_ent_info(uc, dev_id, tbl_hdl, ent_hdl);
  return UCLI_STATUS_OK;
}

pipe_status_t pipe_mgr_idle_tbl_params_dump(ucli_context_t *uc,
                                            pipe_idle_time_params_t tbl_params,
                                            const char *indent) {
  if (tbl_params.mode == NOTIFY_MODE) {
    aim_printf(&uc->pvs,
               "%s%25s: %d ms\n",
               indent,
               "Query interval",
               tbl_params.u.notify.ttl_query_interval);
    aim_printf(&uc->pvs,
               "%s%25s: %d ms\n",
               indent,
               "Min TTL",
               tbl_params.u.notify.min_ttl);
    aim_printf(&uc->pvs,
               "%s%25s: %d ms\n",
               indent,
               "Max TTL",
               tbl_params.u.notify.max_ttl);
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_idle_tbl_dump(ucli_context_t *uc,
                                     idle_tbl_info_t *idle_tbl_info,
                                     const char *orig_indent) {
  char indent[100];

  snprintf(indent, 100, "%s\t\t\t", orig_indent);
  int i;
  uint32_t q = 0;
  for (i = 0; i < idle_tbl_info->no_idle_tbls; i++) {
    idle_tbl_t *idle_tbl = &idle_tbl_info->idle_tbls[i];

    if (idle_tbl->pipe_id == BF_DEV_PIPE_ALL) {
      aim_printf(&uc->pvs, "%s\t%25s: ALL\n", orig_indent, "Pipe id");
    } else {
      aim_printf(&uc->pvs,
                 "%s\t%25s: %d\n",
                 orig_indent,
                 "Pipe id",
                 idle_tbl->pipe_id);
    }
    aim_printf(&uc->pvs, "%s\tPipes in bitmap: ", orig_indent);
    PIPE_BITMAP_ITER(&idle_tbl->inst_pipe_bmp, q) {
      aim_printf(&uc->pvs, "%d ", q);
    }
    aim_printf(&uc->pvs, "\n");
    aim_printf(&uc->pvs, "%s\tPipes in aymmetric scope bitmap: ", orig_indent);
    PIPE_BITMAP_ITER(&idle_tbl->asym_scope_pipe_bmp, q) {
      aim_printf(&uc->pvs, "%d ", q);
    }
    aim_printf(&uc->pvs, "\n");
    int stage_idx;
    for (stage_idx = 0; stage_idx < idle_tbl->num_stages; stage_idx++) {
      idle_tbl_stage_info_t *stage_info = &idle_tbl->stage_info[stage_idx];
      idle_tbl_stage_hw_info_t *hw_info = &stage_info->hw_info;
      aim_printf(&uc->pvs,
                 "%s\t%25s: %d\n",
                 orig_indent,
                 "Stage id",
                 stage_info->stage_id);
      aim_printf(&uc->pvs,
                 "%s%25s: %d\n",
                 indent,
                 "HW Sweep interval",
                 stage_info->hw_sweep_interval);
      aim_printf(&uc->pvs,
                 "%s%25s: %d ms\n",
                 indent,
                 "  HW Sweep period",
                 stage_info->hw_sweep_period);
      aim_printf(&uc->pvs,
                 "%s%25s: %d ms\n",
                 indent,
                 " HW Notify period",
                 stage_info->hw_notify_period);
      aim_printf(&uc->pvs,
                 "%s%25s: %d ms\n",
                 indent,
                 "  SW Sweep period",
                 stage_info->sw_sweep_period);
      aim_printf(&uc->pvs,
                 "%s%25s: %d\n",
                 indent,
                 "   Mem Word Width",
                 hw_info->pack_format.mem_word_width);
      aim_printf(&uc->pvs,
                 "%s%25s: %d\n",
                 indent,
                 "   Tbl Word Width",
                 hw_info->pack_format.tbl_word_width);
      aim_printf(&uc->pvs,
                 "%s%25s: %d\n",
                 indent,
                 " Entries per Word",
                 hw_info->pack_format.entries_per_tbl_word);
      aim_printf(&uc->pvs,
                 "%s%25s: %d\n",
                 indent,
                 "    RAMs per Word",
                 hw_info->pack_format.mem_units_per_tbl_word);
      aim_printf(&uc->pvs,
                 "%s%25s: %d\n",
                 indent,
                 "          Max VPN",
                 hw_info->max_vpn);
      aim_printf(&uc->pvs,
                 "%s%25s: %d\n",
                 indent,
                 "       Num Blocks",
                 hw_info->num_blks);
      for (unsigned int blk = 0; blk < hw_info->num_blks; ++blk) {
        for (int sub_blk = 0;
             sub_blk < hw_info->pack_format.mem_units_per_tbl_word;
             ++sub_blk) {
          aim_printf(&uc->pvs,
                     "%s%21s[%2d]: unit %d vpn %d\n",
                     indent,
                     "      Block",
                     blk,
                     hw_info->tbl_blk[blk].mem_id[sub_blk],
                     hw_info->tbl_blk[blk].vpn_id[sub_blk]);
        }
      }
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_idle_tbl_info_dump_one(ucli_context_t *uc,
                                              idle_tbl_info_t *idle_tbl_info,
                                              bool detailed) {
  const char *indent = "\t\t";
  uint32_t i = 0;
  aim_printf(&uc->pvs,
             "%-40.40s|0x%-8x|%-3s|%-3s|%-10s\n",
             idle_tbl_info->name,
             idle_tbl_info->tbl_hdl,
             IDLE_TBL_IS_SYMMETRIC(idle_tbl_info) ? "Y" : "N",
             idle_tbl_info->idle_tbls ? "Y" : "N",
             IDLE_TBL_IS_POLL_MODE(idle_tbl_info) ? "Poll" : "Notify");
  if (detailed) {
    aim_printf(&uc->pvs, "\n");
    aim_printf(&uc->pvs,
               "%s%25s: %d\n",
               indent,
               "Num of scopes",
               idle_tbl_info->num_scopes);
    for (i = 0; i < idle_tbl_info->num_scopes; i++) {
      aim_printf(&uc->pvs,
                 "%s%22s[%d]: 0x%x\n",
                 indent,
                 "Scope",
                 i,
                 idle_tbl_info->scope_pipe_bmp[i]);
    }
    aim_printf(&uc->pvs,
               "%s%25s: %s\n",
               indent,
               "Update in progress",
               idle_tbl_info->update_in_progress ? "Y" : "N");
    aim_printf(&uc->pvs,
               "%s%25s: %d\n",
               indent,
               "Update count",
               idle_tbl_info->update_count);
    aim_printf(&uc->pvs,
               "%s%25s: %d\n",
               indent,
               "Stage count",
               idle_tbl_info->stage_count);
    aim_printf(&uc->pvs,
               "%s%25s: %d\n",
               indent,
               "Update barrier lock_id",
               idle_tbl_info->update_barrier_lock_id);
    if (idle_tbl_info->enabled) {
      pipe_mgr_idle_tbl_params_dump(uc, idle_tbl_info->tbl_params, indent);
      pipe_mgr_idle_tbl_dump(uc, idle_tbl_info, indent);
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_idle_ucli_dump_tbl_info(ucli_context_t *uc,
                                               bf_dev_id_t dev_id,
                                               pipe_mat_tbl_hdl_t tbl_hdl,
                                               bool tbl_hdl_valid) {
  aim_printf(&uc->pvs,
             "%-40s|%-10s|%-3s|%-3s|%-10s\n",
             "----------------------------------------",
             "----------",
             "---",
             "---",
             "----------");
  aim_printf(&uc->pvs,
             "%-40s|%-10s|%-3s|%-3s|%-10s\n",
             "Name",
             "tbl_hdl",
             "Sym",
             "En",
             "Mode");
  aim_printf(&uc->pvs,
             "%-40s|%-10s|%-3s|%-3s|%-10s\n",
             "----------------------------------------",
             "----------",
             "---",
             "---",
             "----------");
  idle_tbl_info_t *idle_tbl_info = NULL;
  if (tbl_hdl_valid) {
    idle_tbl_info = pipe_mgr_idle_tbl_info_get(dev_id, tbl_hdl);
    if (!idle_tbl_info) {
      aim_printf(
          &uc->pvs, "Table 0x%x on device %d does not exists", tbl_hdl, dev_id);
      return PIPE_OBJ_NOT_FOUND;
    }
    pipe_mgr_idle_tbl_info_dump_one(uc, idle_tbl_info, true);
  } else {
    idle_tbl_info = pipe_mgr_idle_tbl_info_get_first(dev_id, &tbl_hdl);
    while (idle_tbl_info) {
      pipe_mgr_idle_tbl_info_dump_one(uc, idle_tbl_info, false);
      idle_tbl_info = pipe_mgr_idle_tbl_info_get_next(dev_id, &tbl_hdl);
    }
  }
  return PIPE_SUCCESS;
}

PIPE_MGR_IDLE_CLI_CMD_DECLARE(tbl_info) {
  PIPE_MGR_CLI_PROLOGUE(
      "tbl-info", "Print the idle table info", "-d <dev_id> [-h <tbl_handle>]");

  bool got_dev = false;
  bool got_tbl_hdl = false;

  bf_dev_id_t dev_id = 0;
  pipe_mat_tbl_hdl_t tbl_hdl = 0;

  int x;
  while (-1 != (x = getopt(argc, argv, "d:h:"))) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev_id = strtoul(optarg, NULL, 0);
        got_dev = true;
        break;
      case 'h':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        tbl_hdl = strtoul(optarg, NULL, 0);
        got_tbl_hdl = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }
  if (!got_dev) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  pipe_mgr_idle_ucli_dump_tbl_info(uc, dev_id, tbl_hdl, got_tbl_hdl);
  return UCLI_STATUS_OK;
}

/* <auto.ucli.handlers.start> */
static ucli_command_handler_f pipe_mgr_idle_ucli_ucli_handlers__[] = {
    PIPE_MGR_IDLE_CLI_CMD_HNDLR(tbl_info),
    PIPE_MGR_IDLE_CLI_CMD_HNDLR(ent_info),
    NULL};

/* <auto.ucli.handlers.end> */

static ucli_module_t pipe_mgr_idle_ucli_module__ = {
    "idle_ucli",
    NULL,
    pipe_mgr_idle_ucli_ucli_handlers__,
    NULL,
    NULL,
};

ucli_node_t *pipe_mgr_idle_ucli_node_create(ucli_node_t *n) {
  ucli_node_t *m;
  ucli_module_init(&pipe_mgr_idle_ucli_module__);
  m = ucli_node_create("idle", n, &pipe_mgr_idle_ucli_module__);
  ucli_node_subnode_add(m, ucli_module_log_node_create("idle"));
  return m;
}

#else
void *pipe_mgr_idle_ucli_node_create(void) { return NULL; }
#endif
