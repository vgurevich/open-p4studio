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
#include <dvm/bf_drv_intf.h>
#include <lld/bf_dma_if.h>

/* Local includes */
#include "pipe_mgr_int.h"
#include "pipe_mgr_drv.h"
#include "pipe_mgr_tcam.h"
#include "pipe_mgr_tcam_hw.h"
#include "pipe_mgr_tind.h"
#include "pipe_mgr_phy_mem_map.h"

#if PIPE_MGR_CONFIG_INCLUDE_UCLI == 1

#include <target-utils/uCli/ucli.h>
#include <target-utils/uCli/ucli_argparse.h>
#include <target-utils/uCli/ucli_handler_macros.h>

#define PIPE_MGR_TCAM_TBL_CLI_CMD_HNDLR(name) \
  pipe_mgr_tcam_tbl_ucli_ucli__##name##__
#define PIPE_MGR_TCAM_TBL_CLI_CMD_DECLARE(name)                               \
  static ucli_status_t PIPE_MGR_TCAM_TBL_CLI_CMD_HNDLR(name)(ucli_context_t * \
                                                             uc)

void dump_bytes(ucli_context_t *uc, uint8_t *in, uint32_t sz, char *indent) {
  uint32_t i = 0;
  uint32_t index = 0;

  for (i = 0; i < sz; i++) {
    index = sz - i - 1;

    if (!(i % 16)) {
      aim_printf(&uc->pvs, "\n%s%4d -- ", indent, i);
    }

    aim_printf(&uc->pvs, "%02x  ", in[index]);
  }
}

static pipe_status_t pipe_mgr_tcam_ucli_dump_ent_one(
    ucli_context_t *uc,
    tcam_tbl_t *tcam_tbl,
    tcam_hlp_entry_t *tcam_hlp_entry,
    tcam_llp_entry_t *tcam_llp_entry,
    char *indent) {
  tcam_tbl_info_t *tcam_tbl_info = get_tcam_tbl_info(tcam_tbl);
  uint32_t i = 0;
  uint64_t tcam_addr = 0, tind_addr = 0;
  uint32_t stage_line_no = 0, line_no = 0;
  uint32_t tind_block = 0, tind_subword_pos = 0;
  const char *action_type = NULL;
  const char *act_data_s = "Action data";
  const char *act_hdl_s = "Action handle";
  const char *sel_hdl_s = "Selector handle";
  uint32_t stage_id = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  tcam_stage_info_t *stage_data;
  tcam_block_data_t *block_data;

  bf_dev_pipe_t pipe_id;
  if (tcam_tbl_info->is_symmetric) {
    pipe_id = tcam_tbl_info->llp.lowest_pipe_id;
  } else {
    pipe_id = PIPE_BITMAP_GET_FIRST_SET(&get_tcam_pipe_tbl(tcam_tbl)->pipe_bmp);
  }

  tcam_hlp_entry_t *head_hlp_entry = NULL;
  TCAM_HLP_GET_TCAM_HEAD(tcam_hlp_entry, head_hlp_entry);

  pipe_mat_ent_hdl_t entry_hdl;
  bool is_default_entry;
  uint32_t tcam_index;
  uint32_t group, priority;

  PIPE_MGR_TCAM_GET_ENTRY_PROP(tcam_hlp_entry,
                               tcam_llp_entry,
                               tcam_index,
                               is_default_entry,
                               entry_hdl,
                               group,
                               priority);

  tcam_phy_loc_info_t tcam_loc;
  rc = pipe_mgr_tcam_get_phy_loc_for_tcam_entry(
      tcam_tbl, tcam_index, is_default_entry, &tcam_loc);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error getting the physical location info for entry 0x%x "
        "rc 0x%x",
        __func__,
        __LINE__,
        tcam_tbl_info->name,
        tcam_tbl_info->dev_id,
        tcam_tbl_info->tbl_hdl,
        entry_hdl,
        rc);
    return rc;
  }

  bool is_atcam = TCAM_TBL_IS_ATCAM(tcam_tbl_info);
  bool is_keyless = (!is_atcam && tcam_tbl->total_entries == 1);

  line_no = tcam_loc.phy_line_no;
  stage_data = get_tcam_stage_data(tcam_tbl, &tcam_loc);
  block_data = get_tcam_block_data(tcam_tbl, &tcam_loc);
  stage_id = stage_data->stage_id;
  stage_line_no = tcam_loc.stage_line_no;

  if (!is_atcam && !is_keyless &&
      (!is_default_entry || (get_tcam_pipe_tbl(tcam_tbl)->default_ent_type ==
                             TCAM_DEFAULT_ENT_TYPE_DIRECT))) {
    pipe_mgr_tcam_get_instruction_addr(
        tcam_tbl, &tcam_addr, &tind_addr, tcam_index);

    dev_target_t dev_tgt;
    dev_tgt.device_id = tcam_tbl_info->dev_id;
    dev_tgt.dev_pipe_id = pipe_id;
    uint64_t tcam_phy_addr[RMT_MAX_MEM_UNITS_PER_TBL_WORD_BLK];
    int tcam_addr_count;
    int tcam_phy_subword_pos;
    rc = pipe_mgr_get_phy_addr_for_vpn_addr(
        dev_tgt,
        tcam_tbl_info->tbl_hdl,
        stage_id,
        true,
        stage_data->stage_table_handle,
        is_atcam ? RMT_TBL_TYPE_ATCAM_MATCH : RMT_TBL_TYPE_TERN_MATCH,
        stage_line_no,
        tcam_phy_addr,
        &tcam_addr_count,
        &tcam_phy_subword_pos);
    if (rc != PIPE_SUCCESS) {
      PIPE_MGR_DBGCHK(0);
      return rc;
    }
    PIPE_MGR_DBGCHK(tcam_addr_count != 0);
    PIPE_MGR_DBGCHK(tcam_phy_addr[0] == tcam_addr);
    PIPE_MGR_DBGCHK(tcam_phy_subword_pos == 0);
  }

  bool tind_exists = false;
  uint32_t tind_line_no = 0;
  if (!is_default_entry) {
    tind_exists = pipe_mgr_tcam_tind_get_line_no(stage_data,
                                                 stage_line_no,
                                                 &tind_line_no,
                                                 &tind_block,
                                                 &tind_subword_pos);
    dev_target_t dev_tgt;
    dev_tgt.device_id = tcam_tbl_info->dev_id;
    dev_tgt.dev_pipe_id = pipe_id;
    uint64_t tind_phy_addr[RMT_MAX_MEM_UNITS_PER_TBL_WORD_BLK];
    int tind_addr_count;
    int tind_phy_subword_pos;
    rc = pipe_mgr_get_phy_addr_for_vpn_addr(dev_tgt,
                                            tcam_tbl_info->tbl_hdl,
                                            stage_id,
                                            false,
                                            0,
                                            RMT_TBL_TYPE_TERN_INDIR,
                                            stage_line_no,
                                            tind_phy_addr,
                                            &tind_addr_count,
                                            &tind_phy_subword_pos);
    if (tind_exists) {
      if (rc != PIPE_SUCCESS) {
        PIPE_MGR_DBGCHK(0);
        return rc;
      }
      PIPE_MGR_DBGCHK(tind_addr_count == 1);
      PIPE_MGR_DBGCHK(tind_phy_addr[0] == tind_addr);
      PIPE_MGR_DBGCHK(tind_phy_subword_pos == (int)tind_subword_pos);
    } else {
      if (rc == PIPE_SUCCESS) {
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
      }
    }
  }

  aim_printf(&uc->pvs,
             "%s%15s: %s\n",
             indent,
             "Default?",
             is_default_entry ? "Y" : "N");
  aim_printf(&uc->pvs,
             "%s%15s: %d\n",
             indent,
             "Pipe",
             get_tcam_pipe_tbl(tcam_tbl)->pipe_id);
  aim_printf(
      &uc->pvs, "%s%15s: %d\n", indent, "Partition", tcam_tbl->ptn_index);
  aim_printf(&uc->pvs, "%s%15s: %d\n", indent, "Logical Index", tcam_index);
  aim_printf(&uc->pvs, "%s%15s: %d\n", indent, "Stage", stage_id);
  aim_printf(&uc->pvs,
             "%s%15s: %d\n",
             indent,
             "Stage tblhdl",
             stage_data->stage_table_handle);
  aim_printf(
      &uc->pvs, "%s%15s: 0x%x\n", indent, "Match address", stage_line_no);
  aim_printf(&uc->pvs, "%s%15s:", indent, "Mem-id");

  uint32_t wide_tcam_units = stage_data->pack_format.mem_units_per_tbl_word;
  for (i = 0; i < wide_tcam_units; i++) {
    aim_printf(&uc->pvs, " %d", block_data->word_blk.mem_id[i]);
  }
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%s%15s: %d\n", indent, "Line no", line_no);
  aim_printf(&uc->pvs, "%s%15s: %d\n", indent, "Subword", tcam_loc.subword);
  if (tind_exists) {
    aim_printf(&uc->pvs,
               "%s%15s: %d\n",
               indent,
               "TIND mem-id",
               stage_data->tind_blk[tind_block].mem_id[0]);
    aim_printf(&uc->pvs, "%s%15s: %d\n", indent, "TIND line", tind_line_no);
    aim_printf(
        &uc->pvs, "%s%15s: %d\n", indent, "TIND subword", tind_subword_pos);
  }
  aim_printf(&uc->pvs, "%s%15s: %d\n", indent, "Group", group);
  aim_printf(&uc->pvs, "%s%15s: %d\n", indent, "Priority", priority);

  if (head_hlp_entry) {
    pipe_act_fn_hdl_t act_fn_hdl =
        unpack_mat_ent_data_afun_hdl(head_hlp_entry->mat_data);
    pipe_action_spec_t *act_spec =
        unpack_mat_ent_data_as(head_hlp_entry->mat_data);
    aim_printf(&uc->pvs, "%s%15s: 0x%-8x\n", indent, "ACT Fn hdl", act_fn_hdl);
    if (IS_ACTION_SPEC_ACT_DATA(act_spec)) {
      action_type = act_data_s;
    } else if (IS_ACTION_SPEC_ACT_DATA_HDL(act_spec)) {
      action_type = act_hdl_s;
    } else if (IS_ACTION_SPEC_SEL_GRP(act_spec)) {
      action_type = sel_hdl_s;
    } else {
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
    }
    aim_printf(&uc->pvs, "%s%15s: %s\n", indent, "Action type", action_type);
    if (IS_ACTION_SPEC_ACT_DATA_HDL(act_spec)) {
      aim_printf(&uc->pvs,
                 "%s%15s: 0x%-8x\n",
                 indent,
                 "Indirect ADT hdl",
                 act_spec->adt_ent_hdl);
    } else if (IS_ACTION_SPEC_SEL_GRP(act_spec)) {
      aim_printf(&uc->pvs,
                 "%s%15s: 0x%-8x\n",
                 indent,
                 "Sel grp hdl",
                 act_spec->sel_grp_hdl);
    }
  }

  if (tcam_llp_entry) {
    aim_printf(&uc->pvs, "\n\n%sLLP Info\n", indent);
    aim_printf(
        &uc->pvs, "%s%15s: %d\n", indent, "Pipe ID", tcam_llp_entry->pipe_id);
    aim_printf(&uc->pvs,
               "%s%15s: %d\n",
               indent,
               "Partition",
               tcam_llp_entry->ptn_index);
    aim_printf(
        &uc->pvs, "%s%15s: %d\n", indent, "Index", tcam_llp_entry->index);
    aim_printf(&uc->pvs,
               "%s%15s: %d\n",
               indent,
               "Range count",
               tcam_llp_entry->range_count);
    aim_printf(&uc->pvs,
               "%s%15s: %d\n",
               indent,
               "Subentry",
               tcam_llp_entry->subentry_index);
    aim_printf(&uc->pvs,
               "%s%15s: 0x%-8x\n",
               indent,
               "ADT ent hdl",
               tcam_llp_entry->adt_ent_hdl);
    aim_printf(&uc->pvs,
               "%s%15s: 0x%-8x\n",
               indent,
               "Sel grp",
               tcam_llp_entry->sel_grp_hdl);
    aim_printf(&uc->pvs,
               "%s%15s: %d(0x%-8x)\n",
               indent,
               "Log action idx",
               tcam_llp_entry->logical_action_idx,
               tcam_llp_entry->logical_action_idx);
    aim_printf(&uc->pvs,
               "%s%15s: %d(0x%-8x)\n",
               indent,
               "Log sel idx",
               tcam_llp_entry->logical_sel_idx,
               tcam_llp_entry->logical_sel_idx);
    aim_printf(&uc->pvs,
               "%s%15s: %d(0x%-8x)\n",
               indent,
               "ADT addr",
               tcam_llp_entry->addr.indirect_addr_action,
               tcam_llp_entry->addr.indirect_addr_action);
    aim_printf(&uc->pvs,
               "%s%15s: %d(0x%-8x)\n",
               indent,
               "SEL addr",
               tcam_llp_entry->addr.indirect_addr_sel,
               tcam_llp_entry->addr.indirect_addr_sel);
    aim_printf(&uc->pvs,
               "%s%15s: %0d\n",
               indent,
               "SEL len",
               tcam_llp_entry->addr.sel_grp_pvl);
    aim_printf(&uc->pvs,
               "%s%15s: %d(0x%-8x)\n",
               indent,
               "Stats addr",
               tcam_llp_entry->addr.indirect_addr_stats,
               tcam_llp_entry->addr.indirect_addr_stats);
    aim_printf(&uc->pvs,
               "%s%15s: %d(0x%-8x)\n",
               indent,
               "Meter addr",
               tcam_llp_entry->addr.indirect_addr_meter,
               tcam_llp_entry->addr.indirect_addr_meter);
    aim_printf(&uc->pvs,
               "%s%15s: %d(0x%-8x)\n",
               indent,
               "Stful addr",
               tcam_llp_entry->addr.indirect_addr_stful,
               tcam_llp_entry->addr.indirect_addr_stful);
  }

  aim_printf(
      &uc->pvs, "%s%15s: 0x%011" PRIx64 "\n", indent, "TCAM Addr", tcam_addr);
  if (tind_exists) {
    aim_printf(
        &uc->pvs, "%s%15s: 0x%011" PRIx64 "\n", indent, "TIND Addr", tind_addr);
  }

  pipe_mem_type_t pipe_mem_type = TCAM_TBL_IS_ATCAM(tcam_tbl_info)
                                      ? pipe_mem_type_unit_ram
                                      : pipe_mem_type_tcam;

  char buf[1500];
  size_t bytes_written = 0;
  if (!is_default_entry) {
    if (head_hlp_entry) {
      aim_printf(&uc->pvs, "%sMatch spec:\n", indent);
      pipe_tbl_match_spec_t *match_spec =
          unpack_mat_ent_data_ms(head_hlp_entry->mat_data);
      pipe_mgr_entry_format_print_match_spec(tcam_tbl_info->dev_id,
                                             tcam_tbl_info->profile_id,
                                             tcam_tbl_info->tbl_hdl,
                                             match_spec,
                                             buf,
                                             sizeof(buf),
                                             &bytes_written);
      aim_printf(&uc->pvs, "%s", buf);
    }

    if (tcam_llp_entry) {
      uint8_t tcam_word[TOF_BYTES_IN_TCAM_WHOLE_WORD];
      for (i = 0; i < wide_tcam_units; i++) {
        rc = pipe_mgr_phy_mem_map_read(tcam_tbl_info->dev_id,
                                       tcam_tbl_info->direction,
                                       pipe_id,
                                       stage_id,
                                       pipe_mem_type,
                                       block_data->word_blk.mem_id[i],
                                       line_no,
                                       tcam_word,
                                       TOF_BYTES_IN_TCAM_WHOLE_WORD);
        if (rc != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d - %s (%d - 0x%x) "
              "Error reading shadow data for index %d "
              " rc 0x%x",
              __func__,
              __LINE__,
              tcam_tbl_info->name,
              tcam_tbl_info->dev_id,
              tcam_tbl_info->tbl_hdl,
              tcam_loc.index,
              rc);
          return rc;
        }
        dump_bytes(
            uc, tcam_word, TOF_BYTES_IN_TCAM_WHOLE_WORD / 2, "\t\tWord0 ");
        dump_bytes(uc,
                   &tcam_word[TOF_BYTES_IN_TCAM_WHOLE_WORD / 2],
                   TOF_BYTES_IN_TCAM_WHOLE_WORD / 2,
                   "\t\tWord1 ");
      }
    }
  }

  aim_printf(&uc->pvs, "\n\n");

  if (head_hlp_entry) {
    aim_printf(&uc->pvs, "%sAction :\n\n", indent);

    pipe_act_fn_hdl_t act_fn_hdl =
        unpack_mat_ent_data_afun_hdl(head_hlp_entry->mat_data);
    pipe_action_spec_t *act_spec =
        unpack_mat_ent_data_as(head_hlp_entry->mat_data);
    if (act_spec == NULL) {
      LOG_ERROR("%s:%d get action spec", __func__, __LINE__);
      return PIPE_UNEXPECTED;
    }

    if (IS_ACTION_SPEC_ACT_DATA(act_spec)) {
      aim_printf(&uc->pvs, "%sAction Spec:\n", indent);
      pipe_mgr_entry_format_print_action_spec(tcam_tbl_info->dev_id,
                                              tcam_tbl_info->profile_id,
                                              &act_spec->act_data,
                                              act_fn_hdl,
                                              buf,
                                              sizeof(buf),
                                              &bytes_written);
      aim_printf(&uc->pvs, "%s", buf);
    } else if (IS_ACTION_SPEC_ACT_DATA_HDL(act_spec)) {
      aim_printf(&uc->pvs,
                 "%sAction entry handle 0x%x",
                 indent,
                 act_spec->adt_ent_hdl);
    } else if (IS_ACTION_SPEC_SEL_GRP(act_spec)) {
      aim_printf(&uc->pvs,
                 "%sSelector group handle 0x%x",
                 indent,
                 act_spec->sel_grp_hdl);
    }
    if (act_spec) {
      resource_trace(tcam_tbl_info->dev_id,
                     act_spec->resources,
                     act_spec->resource_count,
                     buf,
                     sizeof(buf));
      aim_printf(&uc->pvs, "%s %s\n", indent, buf);
    }
  }

  aim_printf(&uc->pvs, "\n\n");

  if (tind_exists && tcam_llp_entry) {
    uint8_t tind_data[TOF_BYTES_IN_RAM_WORD];
    rc = pipe_mgr_phy_mem_map_read(tcam_tbl_info->dev_id,
                                   tcam_tbl_info->direction,
                                   pipe_id,
                                   stage_id,
                                   pipe_mem_type_unit_ram,
                                   stage_data->tind_blk[tind_block].mem_id[0],
                                   tind_line_no,
                                   tind_data,
                                   TOF_BYTES_IN_RAM_WORD);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error reading tind shadow data for index %d "
          " rc 0x%x",
          __func__,
          __LINE__,
          tcam_tbl_info->name,
          tcam_tbl_info->dev_id,
          tcam_tbl_info->tbl_hdl,
          tcam_loc.index,
          rc);
      return rc;
    }
    dump_bytes(uc, tind_data, TOF_BYTES_IN_RAM_WORD, "\t\tTIND ");
    aim_printf(&uc->pvs, "\n");
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tcam_ucli_dump_ent_info(ucli_context_t *uc,
                                               bf_dev_id_t dev_id,
                                               pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                               pipe_mat_ent_hdl_t mat_ent_hdl,
                                               uint32_t subindex) {
  tcam_hlp_entry_t *tcam_hlp_entry = NULL;
  tcam_tbl_info_t *tcam_tbl_info = NULL;
  tcam_pipe_tbl_t *tcam_pipe_tbl = NULL;
  uint32_t pipe_idx = 0;
  bool tcam_present = false;

  tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_id, mat_tbl_hdl, false);
  if (tcam_tbl_info == NULL) {
    aim_printf(&uc->pvs, "Tcam table not found for handle 0x%x\n", mat_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  for (pipe_idx = 0; pipe_idx < tcam_tbl_info->no_tcam_pipe_tbls; pipe_idx++) {
    tcam_pipe_tbl = &tcam_tbl_info->tcam_pipe_tbl[pipe_idx];

    /* First find the tcam_index based on the mat-ent-hdl */
    tcam_hlp_entry =
        pipe_mgr_tcam_entry_get(tcam_pipe_tbl, mat_ent_hdl, subindex);

    tcam_llp_entry_t *tcam_llp_entry;
    tcam_llp_entry =
        pipe_mgr_tcam_llp_entry_get(tcam_tbl_info, mat_ent_hdl, subindex);

    if (!tcam_hlp_entry && !tcam_llp_entry) {
      continue;
    }
    if (tcam_llp_entry && tcam_llp_entry->pipe_id != tcam_pipe_tbl->pipe_id) {
      continue;
    }

    tcam_present = true;
    aim_printf(&uc->pvs, "%10s: ", "Pipe");
    if (tcam_pipe_tbl->pipe_id == BF_DEV_PIPE_ALL) {
      aim_printf(&uc->pvs, "BF_DEV_PIPE_ALL");
    } else {
      aim_printf(&uc->pvs, "%d", tcam_pipe_tbl->pipe_id);
    }
    aim_printf(&uc->pvs, "\n");
    uint32_t ptn_index =
        tcam_hlp_entry ? tcam_hlp_entry->ptn_index : tcam_llp_entry->ptn_index;
    tcam_tbl_t *tcam_tbl = get_tcam_tbl(tcam_pipe_tbl, ptn_index);
    if (tcam_tbl == NULL) {
      LOG_ERROR("%s:%d get tcam table failed", __func__, __LINE__);
      return PIPE_UNEXPECTED;
    }
    pipe_mgr_tcam_ucli_dump_ent_one(
        uc, tcam_tbl, tcam_hlp_entry, tcam_llp_entry, "\t\t");
  }
  if (!tcam_present) {
    aim_printf(&uc->pvs, "TCAM entry not present\n");
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tcam_tbl_dump_detailed(ucli_context_t *uc,
                                              tcam_pipe_tbl_t *tcam_pipe_tbl,
                                              bool virtual_dev) {
  tcam_tbl_info_t *tcam_tbl_info = tcam_pipe_tbl->tcam_tbl_info_p;
  tcam_tbl_t *tcam_tbl = NULL;
  pipe_status_t rc = PIPE_SUCCESS;

  pipe_mat_ent_hdl_t default_ent_hdl;

  if (virtual_dev) {
    default_ent_hdl = tcam_pipe_tbl->hlp.default_ent_hdl;
  } else {
    default_ent_hdl = tcam_pipe_tbl->llp.default_ent_hdl;
  }

  aim_printf(&uc->pvs, "Default Entry Hdl: 0x%x\n", default_ent_hdl);
  aim_printf(&uc->pvs,
             "Default Entry Type: %s\n",
             tcam_default_ent_type_str(tcam_pipe_tbl->default_ent_type));

  FOR_ALL_PTNS_BEGIN(tcam_pipe_tbl, tcam_tbl) {
    uint32_t i = 0;
    if ((tcam_tbl->hlp.total_usage == 0) && (tcam_tbl->llp.total_usage == 0)) {
      continue;
    }
    aim_printf(&uc->pvs, "Pipes in bitmap : ");
    PIPE_BITMAP_ITER(&tcam_pipe_tbl->pipe_bmp, i) {
      aim_printf(&uc->pvs, "%d ", i);
    }
    aim_printf(&uc->pvs, "\n");

    aim_printf(&uc->pvs, "Partition %d\n", tcam_tbl->ptn_index);
    aim_printf(&uc->pvs, "Total Entries %d\n", tcam_tbl->total_entries);
    aim_printf(&uc->pvs, "HLP usage %d\n", tcam_tbl->hlp.total_usage);
    aim_printf(&uc->pvs, "LLP usage %d\n", tcam_tbl->llp.total_usage);
    aim_printf(&uc->pvs, "Entries\n");

    aim_printf(&uc->pvs,
               "%-8s|%-10s|%-5s|%-8s|%-3s|%-6s|%-8s|%-5s|%-8s\n",
               "--------",
               "----------",
               "-----",
               "--------",
               "---",
               "------",
               "--------",
               "-----",
               "--------");
    aim_printf(&uc->pvs,
               "%-8s|%-10s|%-5s|%-8s|%-3s|%-6s|%-8s|%-5s|%-8s\n",
               "Index",
               "Entry Hdl",
               "Stage",
               "Diradr",
               "Sub",
               "Mem-id",
               "Line-no",
               "Group",
               "Priority");
    aim_printf(&uc->pvs,
               "%-8s|%-10s|%-5s|%-8s|%-3s|%-6s|%-8s|%-5s|%-8s\n",
               "--------",
               "----------",
               "-----",
               "--------",
               "---",
               "------",
               "--------",
               "-----",
               "--------");

    for (i = 0; i < tcam_tbl->total_entries; i++) {
      tcam_hlp_entry_t *tcam_hlp_entry = tcam_tbl->hlp.tcam_entries[i];
      tcam_llp_entry_t *tcam_llp_entry = tcam_tbl->llp.tcam_entries[i];

      if (!tcam_hlp_entry && !tcam_llp_entry) {
        continue;
      }

      pipe_mat_ent_hdl_t entry_hdl;
      bool is_default_entry;
      uint32_t tcam_index;
      uint32_t group, priority;

      PIPE_MGR_TCAM_GET_ENTRY_PROP(tcam_hlp_entry,
                                   tcam_llp_entry,
                                   tcam_index,
                                   is_default_entry,
                                   entry_hdl,
                                   group,
                                   priority);

      PIPE_MGR_DBGCHK(tcam_index == i);
      tcam_phy_loc_info_t tcam_loc;
      rc = pipe_mgr_tcam_get_phy_loc_for_tcam_entry(
          tcam_tbl, tcam_index, is_default_entry, &tcam_loc);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Error getting the physical location info for entry 0x%x "
            "rc 0x%x",
            __func__,
            __LINE__,
            tcam_tbl_info->name,
            tcam_tbl_info->dev_id,
            tcam_tbl_info->tbl_hdl,
            entry_hdl,
            rc);
        return rc;
      }

      tcam_stage_info_t *stage_data;
      tcam_block_data_t *block_data;

      uint32_t stage = 0;
      uint32_t line_no = tcam_loc.phy_line_no;
      stage_data = get_tcam_stage_data(tcam_tbl, &tcam_loc);
      block_data = get_tcam_block_data(tcam_tbl, &tcam_loc);
      stage = stage_data->stage_id;
      uint32_t mem_id = block_data->word_blk.mem_id[0];

      aim_printf(&uc->pvs,
                 "%-8d|0x%-8x|%5d|0x%-6x|%3d|%6d|%8d|%5d|%8d\n",
                 i,
                 entry_hdl,
                 stage,
                 tcam_loc.stage_line_no,
                 tcam_loc.subword,
                 mem_id,
                 line_no,
                 group,
                 priority);
    }
  }
  FOR_ALL_PTNS_END();

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tcam_tbl_dump_one(ucli_context_t *uc,
                                         tcam_tbl_info_t *tcam_tbl_info,
                                         bool detailed) {
  uint32_t count = 0;
  dev_target_t dev_tgt = {.device_id = tcam_tbl_info->dev_id,
                          .dev_pipe_id = BF_DEV_PIPE_ALL};
  if (pipe_mgr_is_device_virtual(tcam_tbl_info->dev_id)) {
    pipe_mgr_tcam_get_placed_entry_count(
        dev_tgt, tcam_tbl_info->tbl_hdl, &count);
  } else {
    pipe_mgr_tcam_get_programmed_entry_count(
        dev_tgt, tcam_tbl_info->tbl_hdl, &count);
  }
  aim_printf(&uc->pvs,
             "%-40.40s|0x%-8x|%-3s|%-6d|%-10u|%-3s|0x%-8x|%-3s|0x%-8x\n",
             tcam_tbl_info->name,
             tcam_tbl_info->tbl_hdl,
             tcam_tbl_info->is_symmetric ? "Y" : "N",
             tcam_tbl_info->num_scopes,
             count,
             tcam_tbl_info->adt_present ? "Y" : "N",
             tcam_tbl_info->adt_tbl_ref.tbl_hdl,
             tcam_tbl_info->sel_present ? "Y" : "N",
             tcam_tbl_info->sel_tbl_ref.tbl_hdl);

  bool virtual_dev = pipe_mgr_is_device_virtual(tcam_tbl_info->dev_id);
  if (detailed) {
    aim_printf(&uc->pvs, "Total Size: %d\n", tcam_tbl_info->tbl_size_max);
    aim_printf(&uc->pvs, "P4 Size: %d\n", tcam_tbl_info->tbl_size_in_p4);
    for (uint8_t i = 0; i < tcam_tbl_info->no_tcam_pipe_tbls; i++) {
      aim_printf(&uc->pvs, "\n\n PIPE %d\n", i);
      pipe_mgr_tcam_tbl_dump_detailed(
          uc, &tcam_tbl_info->tcam_pipe_tbl[i], virtual_dev);
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tcam_ucli_dump_tbl_info(ucli_context_t *uc,
                                               bf_dev_id_t dev_id,
                                               bool valid_tbl_hdl,
                                               pipe_mat_tbl_hdl_t mat_tbl_hdl) {
  tcam_tbl_info_t *tcam_tbl_info = NULL;

  aim_printf(&uc->pvs,
             "%-40s|%-10s|%-3s|%-6s|%-10s|%-3s|%-10s|%-3s|%-10s\n",
             "----------------------------------------",
             "----------",
             "---",
             "------",
             "----------",
             "---",
             "----------",
             "---",
             "----------");
  aim_printf(&uc->pvs,
             "%-40s|%-10s|%-3s|%-6s|%-10s|%-3s|%-10s|%-3s|%-10s\n",
             "Name",
             "tbl_hdl",
             "Sym",
             "Scopes",
             "Occupied",
             "ADT",
             "ADT Hdl",
             "SEL",
             "SEL Hdl");
  aim_printf(&uc->pvs,
             "%-40s|%-10s|%-3s|%-6s|%-10s|%-3s|%-10s|%-3s|%-10s\n",
             "----------------------------------------",
             "----------",
             "---",
             "------",
             "----------",
             "---",
             "----------",
             "---",
             "----------");
  if (valid_tbl_hdl) {
    tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_id, mat_tbl_hdl, false);
    if (tcam_tbl_info == NULL) {
      LOG_ERROR("%s:%d tcam table not found for handle 0x%x",
                __func__,
                __LINE__,
                mat_tbl_hdl);
      return PIPE_OBJ_NOT_FOUND;
    }

    pipe_mgr_tcam_tbl_dump_one(uc, tcam_tbl_info, true);
  } else {
    pipe_mat_tbl_hdl_t tbl_hdl;
    tcam_tbl_info = pipe_mgr_tcam_tbl_info_get_first(dev_id, &tbl_hdl);
    while (tcam_tbl_info) {
      pipe_mgr_tcam_tbl_dump_one(uc, tcam_tbl_info, false);
      tcam_tbl_info = pipe_mgr_tcam_tbl_info_get_next(dev_id, &tbl_hdl);
    }
  }

  return PIPE_SUCCESS;
}

PIPE_MGR_TCAM_TBL_CLI_CMD_DECLARE(ent_info) {
  PIPE_MGR_CLI_PROLOGUE(
      "entry_info",
      "Print the tcam entry info",
      "-d <dev_id> -h <tbl_handle> -e <ent_handle> [-i <subindex>]");

  bool got_dev = false;
  bool got_tbl_hdl = false;
  bool got_ent_hdl = false;

  bf_dev_id_t dev_id = 0;
  pipe_mat_tbl_hdl_t tbl_hdl = 0;
  pipe_mat_ent_hdl_t ent_hdl = 0;
  uint32_t subindex = 0;

  int x;
  while (-1 != (x = getopt(argc, argv, "d:h:e:i:"))) {
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
      case 'i':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        subindex = strtoul(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }
  if (!got_dev || dev_id < 0 || dev_id >= PIPE_MGR_NUM_DEVICES ||
      !got_tbl_hdl || !got_ent_hdl) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  pipe_mgr_tcam_ucli_dump_ent_info(uc, dev_id, tbl_hdl, ent_hdl, subindex);

  return UCLI_STATUS_OK;
}

PIPE_MGR_TCAM_TBL_CLI_CMD_DECLARE(tbl_info) {
  PIPE_MGR_CLI_PROLOGUE("tbl_info",
                        "Dump the table information.",
                        "-d <dev_id> [-h <tbl_handle>]");
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
  if (dev_id < 0 || dev_id >= PIPE_MGR_NUM_DEVICES) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  pipe_mgr_tcam_ucli_dump_tbl_info(uc, dev_id, got_tbl_hdl, tbl_hdl);

  return UCLI_STATUS_OK;
}

PIPE_MGR_TCAM_TBL_CLI_CMD_DECLARE(entry_count) {
  PIPE_MGR_CLI_PROLOGUE("entry_count",
                        "Print the tcam programmed entry count",
                        "-d <dev_id> -h <tbl_handle>");

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
  if (!got_dev || dev_id < 0 || dev_id >= PIPE_MGR_NUM_DEVICES ||
      !got_tbl_hdl) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  tcam_tbl_info_t *tcam_tbl_info = NULL;
  tcam_tbl_info = pipe_mgr_tcam_tbl_info_get(dev_id, tbl_hdl, false);
  if (tcam_tbl_info == NULL) {
    aim_printf(&uc->pvs, "tcam table not found for handle 0x%x\n", tbl_hdl);
    return UCLI_STATUS_OK;
  }

  uint32_t count = 0;
  dev_target_t dev_tgt = {.device_id = dev_id, .dev_pipe_id = BF_DEV_PIPE_ALL};
  if (pipe_mgr_is_device_virtual(dev_id)) {
    pipe_mgr_tcam_get_placed_entry_count(dev_tgt, tbl_hdl, &count);
  } else {
    pipe_mgr_tcam_get_programmed_entry_count(dev_tgt, tbl_hdl, &count);
  }
  aim_printf(&uc->pvs, "tcam tbl 0x%x: %u entries occupied\n", tbl_hdl, count);

  return UCLI_STATUS_OK;
}

/* <auto.ucli.handlers.start> */
static ucli_command_handler_f pipe_mgr_tcam_tbl_ucli_ucli_handlers__[] = {
    PIPE_MGR_TCAM_TBL_CLI_CMD_HNDLR(tbl_info),
    PIPE_MGR_TCAM_TBL_CLI_CMD_HNDLR(ent_info),
    PIPE_MGR_TCAM_TBL_CLI_CMD_HNDLR(entry_count),
    NULL};

/* <auto.ucli.handlers.end> */

static ucli_module_t pipe_mgr_tcam_tbl_ucli_module__ = {
    "tcam_tbl_ucli",
    NULL,
    pipe_mgr_tcam_tbl_ucli_ucli_handlers__,
    NULL,
    NULL,
};

ucli_node_t *pipe_mgr_tcam_tbl_ucli_node_create(ucli_node_t *n) {
  ucli_node_t *m;
  ucli_module_init(&pipe_mgr_tcam_tbl_ucli_module__);
  m = ucli_node_create("tcam_tbl", n, &pipe_mgr_tcam_tbl_ucli_module__);
  ucli_node_subnode_add(m, ucli_module_log_node_create("tcam_tbl"));
  return m;
}

#else
void *pipe_mgr_tcam_tbl_ucli_node_create(void) { return NULL; }
#endif
