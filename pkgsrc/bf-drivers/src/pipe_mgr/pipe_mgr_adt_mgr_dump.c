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
 * @file pipe_mgr_adt_mgr_dump.c
 * @date
 *
 * Action data table manager dump routines to dump debug information.
 */

/* Standard header includes */

/* Module header includes */
#include <pipe_mgr/pipe_mgr_intf.h>

/* Local header includes */
#include "pipe_mgr_int.h"
#include "pipe_mgr_adt_mgr_int.h"
#include "pipe_mgr_act_tbl.h"

#include <target-utils/uCli/ucli.h>
#include <target-utils/uCli/ucli_argparse.h>
#include <target-utils/uCli/ucli_handler_macros.h>

void pipe_mgr_adt_dump_stage_info(ucli_context_t *uc,
                                  pipe_mgr_adt_stage_info_t *adt_stage_info) {
  pipe_mgr_adt_ram_alloc_info_t *ram_alloc_info = NULL;
  rmt_tbl_word_blk_t *tbl_word_blk;
  uint32_t i = 0;
  uint32_t j = 0;
  aim_printf(&uc->pvs,
             "Dumping stage info for stage id %d\n",
             adt_stage_info->stage_id);
  ram_alloc_info = adt_stage_info->ram_alloc_info;

  aim_printf(&uc->pvs, "Number of entries %d\n", adt_stage_info->num_entries);
  aim_printf(&uc->pvs,
             "Number of entries occupied %d\n",
             adt_stage_info->num_entries_occupied);
  aim_printf(&uc->pvs, "Entry packing info:\n");
  aim_printf(&uc->pvs,
             "  Num of ram-units per wide-word %d\n",
             adt_stage_info->ram_alloc_info->num_rams_in_wide_word);
  aim_printf(&uc->pvs,
             "  Num of entries per wide-word %d\n",
             adt_stage_info->ram_alloc_info->num_entries_per_wide_word);

  aim_printf(&uc->pvs, "RAM Allocation info:\n");
  for (i = 0; i < ram_alloc_info->num_wide_word_blks; i++) {
    tbl_word_blk = &(ram_alloc_info->tbl_word_blk[i]);
    for (j = 0; j < ram_alloc_info->num_rams_in_wide_word; j++) {
      aim_printf(&uc->pvs, "%d ", *(mem_id_t *)(&(tbl_word_blk[j])));
    }

    aim_printf(&uc->pvs, "\n");
  }

  return;
}

void pipe_mgr_adt_dump_tbl_info(ucli_context_t *uc,
                                uint8_t device_id,
                                pipe_adt_tbl_hdl_t adt_tbl_hdl) {
  uint32_t tbl_idx = 0, i = 0;
  uint32_t stage_idx = 0;
  pipe_mgr_adt_t *adt = NULL;
  pipe_mgr_adt_data_t *adt_tbl_data = NULL;
  pipe_mgr_adt_stage_info_t *adt_stage_info = NULL;
  dev_target_t dev_tgt = {.device_id = device_id,
                          .dev_pipe_id = BF_DEV_PIPE_ALL};

  adt = pipe_mgr_adt_get(device_id, adt_tbl_hdl);
  if (adt == NULL) {
    aim_printf(&uc->pvs, "No information found for the table specified\n");
    return;
  }

  uint32_t count = 0;
  if (pipe_mgr_is_device_virtual(device_id)) {
    pipe_mgr_adt_tbl_get_num_entries_placed(dev_tgt, adt_tbl_hdl, &count);
  } else {
    pipe_mgr_adt_tbl_get_num_entries_programmed(dev_tgt, adt_tbl_hdl, &count);
  }

  aim_printf(&uc->pvs,
             "Num entries:%d, Entries occupied:%d\n",
             adt->num_entries,
             count);
  aim_printf(&uc->pvs, "Symmetric : %s\n", adt->symmetric ? "true" : "false");
  aim_printf(&uc->pvs, "Num of scopes : %d\n", adt->num_scopes);
  for (i = 0; i < adt->num_scopes; i++) {
    aim_printf(&uc->pvs, "  Scope[%d] : 0x%x\n", i, adt->scope_pipe_bmp[i]);
  }

  if (adt->symmetric) {
    aim_printf(&uc->pvs, "Pipe id : All pipes\n");
    aim_printf(
        &uc->pvs, "Number of stages : %d\n", adt->adt_tbl_data[0].num_stages);
    aim_printf(&uc->pvs, "Stage id : ");
    adt_tbl_data = &(adt->adt_tbl_data[0]);
    for (stage_idx = 0; stage_idx < adt_tbl_data->num_stages; stage_idx++) {
      adt_stage_info = &adt_tbl_data->adt_stage_info[stage_idx];
      pipe_mgr_adt_dump_stage_info(uc, adt_stage_info);
    }
    aim_printf(&uc->pvs, "\n");
  } else {
    uint32_t q = 0;
    for (tbl_idx = 0; tbl_idx < adt->num_tbls; tbl_idx++) {
      aim_printf(&uc->pvs, "Pipe id : ");
      aim_printf(&uc->pvs, "%d\n", adt->adt_tbl_data[tbl_idx].pipe_id);
      aim_printf(&uc->pvs, "Pipes in bitmap : ");
      PIPE_BITMAP_ITER(&(adt->adt_tbl_data[tbl_idx].pipe_bmp), q) {
        aim_printf(&uc->pvs, "%d ", q);
      }
      aim_printf(&uc->pvs, "\n");
      for (stage_idx = 0; stage_idx < adt->adt_tbl_data[tbl_idx].num_stages;
           stage_idx++) {
        adt_stage_info = &adt->adt_tbl_data[tbl_idx].adt_stage_info[stage_idx];
        pipe_mgr_adt_dump_stage_info(uc, adt_stage_info);
      }
      aim_printf(&uc->pvs, "\n");
    }
    aim_printf(&uc->pvs, "\n");
  }
  return;
}

void pipe_mgr_adt_dump_entry_info(ucli_context_t *uc,
                                  uint8_t device_id,
                                  pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                  pipe_adt_ent_hdl_t adt_ent_hdl) {
  pipe_mgr_adt_t *adt = NULL;
  pipe_mgr_adt_entry_phy_info_t *entry_info = NULL;
  char buf[1000];
  size_t bytes_written = 0;
  pipe_action_data_spec_t *act_data_spec = NULL;
  pipe_act_fn_hdl_t act_fn_hdl = 0;

  adt = pipe_mgr_adt_get(device_id, adt_tbl_hdl);

  if (adt == NULL) {
    aim_printf(&uc->pvs, "No information found for the table specified\n");
    return;
  }
  if (adt->ref_type == PIPE_TBL_REF_TYPE_DIRECT) {
    aim_printf(&uc->pvs,
               "Command not supported for directly referenced action table\n");
    return;
  }
  entry_info = pipe_mgr_adt_get_entry_phy_info(adt, adt_ent_hdl);
  if (entry_info == NULL) {
    aim_printf(&uc->pvs, "Entry info for handle %d not found\n", adt_ent_hdl);
    return;
  }
  act_data_spec = unpack_adt_ent_data_ad(entry_info->entry_data);
  act_fn_hdl = unpack_adt_ent_data_afun_hdl(entry_info->entry_data);
  aim_printf(&uc->pvs, "Entry handle information\n");
  aim_printf(&uc->pvs, "------------------------\n");

  aim_printf(&uc->pvs, "  Action spec :\n");
  aim_printf(&uc->pvs,
             "      Num action bytes : %d\n",
             act_data_spec->num_action_data_bytes);
  aim_printf(&uc->pvs,
             "      Num valid action bits : %d\n",
             act_data_spec->num_valid_action_data_bits);
  pipe_mgr_entry_format_print_action_spec(device_id,
                                          adt->profile_id,
                                          act_data_spec,
                                          act_fn_hdl,
                                          buf,
                                          sizeof(buf),
                                          &bytes_written);
  aim_printf(&uc->pvs, "    %s\n", buf);
  aim_printf(&uc->pvs, "Location information\n");
  if (entry_info->sharable_stage_location) {
    pipe_mgr_adt_stage_location_t *location =
        entry_info->sharable_stage_location;
    aim_printf(&uc->pvs, "Sharable location (one per stage)\n");
    while (location) {
      aim_printf(&uc->pvs,
                 "\tStage id %d, idx %d, reference count %d\n",
                 location->stage_id,
                 location->entry_idx,
                 location->ref_count);
      location = location->next;
    }
  }
  if (entry_info->non_sharable_stage_location) {
    pipe_mgr_adt_stage_location_t *location =
        entry_info->non_sharable_stage_location;
    aim_printf(&uc->pvs, "Non-Sharable location\n");
    while (location) {
      aim_printf(&uc->pvs,
                 "\tStage id %d, idx %d, reference count %d\n",
                 location->stage_id,
                 location->entry_idx,
                 location->ref_count);
      location = location->next;
    }
  }
  return;
}
