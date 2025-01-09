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
#include "pipe_mgr_select_tbl.h"

#if PIPE_MGR_CONFIG_INCLUDE_UCLI == 1

#include <target-utils/uCli/ucli.h>
#include <target-utils/uCli/ucli_argparse.h>
#include <target-utils/uCli/ucli_handler_macros.h>

#define PIPE_MGR_SEL_TBL_CLI_CMD_HNDLR(name) \
  pipe_mgr_sel_tbl_ucli_ucli__##name##__
#define PIPE_MGR_SEL_TBL_CLI_CMD_DECLARE(name) \
  static ucli_status_t PIPE_MGR_SEL_TBL_CLI_CMD_HNDLR(name)(ucli_context_t * uc)

static pipe_status_t pipe_mgr_sel_ucli_dump_word_data(
    ucli_context_t *uc,
    sel_tbl_stage_info_t *stage_info,
    uint32_t word_idx,
    char *indent) {
  uint32_t index = 0;

  sel_hlp_word_data_t *hlp_word;
  sel_llp_word_data_t *llp_word;

  if (word_idx >= stage_info->no_words) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  hlp_word = &stage_info->hlp.hlp_word[word_idx];
  llp_word = &stage_info->llp.llp_word[word_idx];

  uint32_t word_width = hlp_word->word_width;

  aim_printf(&uc->pvs, "\n%sUsage: %d\t", indent, hlp_word->usage);
  aim_printf(&uc->pvs, "\n%sActive: %d\t", indent, llp_word->no_bits_set);
  aim_printf(
      &uc->pvs, "\n%sHighest idx: %d\t", indent, llp_word->highest_mbr_idx);
  aim_printf(&uc->pvs,
             "\n%sADT Base(H): %d(0x%x)\t",
             indent,
             hlp_word->adt_base_idx,
             hlp_word->adt_base_idx);
  aim_printf(&uc->pvs,
             "\n%sADT Base(L): %d(0x%x)\t",
             indent,
             llp_word->adt_base_idx,
             llp_word->adt_base_idx);

  for (index = 0; index < word_width; index += 16) {
    uint32_t j;
    aim_printf(&uc->pvs, "\n%s%d\t", indent, index);
    aim_printf(&uc->pvs, "\n%sMBR\t", indent);

    for (j = 0; (j < 16) && (index + j < word_width); j++) {
      aim_printf(&uc->pvs, "0x%-8x ", hlp_word->mbrs[index + j]);
    }

    if (llp_word->highest_mbr_idx >= (int32_t)index) {
      aim_printf(&uc->pvs, "\n%sSts\t", indent);
    }

    for (j = 0; (j < 16) && (index + j < word_width); j++) {
      if (llp_word->highest_mbr_idx >= (int32_t)(index + j)) {
        aim_printf(
            &uc->pvs, "%-10d ", GET_MBR_DATA_FOR_IDX(llp_word, (index + j)));
      }
    }
    aim_printf(&uc->pvs, "\n");
  }

  if (word_width == 0) {
    int32_t i;
    int32_t j;
    for (i = 0; i <= llp_word->highest_mbr_idx; i += 16) {
      aim_printf(&uc->pvs, "\n%s%d\t", indent, i);
      aim_printf(&uc->pvs, "\n%sMBR\t", indent);

      for (j = 0; (j < 16) && (i + j <= llp_word->highest_mbr_idx); j++) {
        aim_printf(&uc->pvs, "0x%-8x ", llp_word->mbrs[i + j]);
      }

      aim_printf(&uc->pvs, "\n%sSts\t", indent);
      for (j = 0; (j < 16) && (i + j <= llp_word->highest_mbr_idx); j++) {
        aim_printf(&uc->pvs, "%-10d ", GET_MBR_DATA_FOR_IDX(llp_word, (i + j)));
      }
      aim_printf(&uc->pvs, "\n");
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_ucli_dump_grp_stage(
    ucli_context_t *uc, sel_grp_stage_info_t *grp_stage) {
  uint32_t i = 0;

  if (grp_stage == NULL) {
    return PIPE_SUCCESS;
  }

  aim_printf(&uc->pvs, "GRP HDL: 0x%-8x\n", grp_stage->grp_hdl);
  aim_printf(&uc->pvs,
             "\t%-10s: %-s\n"
             "\t%-10s: %-d\n"
             "\t%-10s: %-s\n"
             "\t%-10s: %-d\n",
             "Static",
             grp_stage->isstatic ? "Y" : "N",
             "No words",
             grp_stage->no_words,
             "Duplicated",
             grp_stage->mbrs_duplicated ? "Y" : "N",
             "Sel Base idx",
             grp_stage->sel_base_idx);

  sel_tbl_stage_info_t *stage_info = grp_stage->stage_p;
  if (grp_stage->inuse) {
    aim_printf(&uc->pvs, "\t%-10s: %-d\n", "Cur usage", grp_stage->cur_usage);

    aim_printf(&uc->pvs, "\t%-10s\n", "Words");
    for (i = 0; i < grp_stage->no_words; i++) {
      aim_printf(&uc->pvs, "\t%-10s%d\n", "Word ", i);
      pipe_mgr_sel_ucli_dump_word_data(
          uc, stage_info, grp_stage->sel_base_idx + i, "\t\t");
      aim_printf(&uc->pvs, "\n\n");
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_ucli_dump_stage_info(ucli_context_t *uc,
                                                bf_dev_id_t dev_id,
                                                pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                                uint32_t stage_id,
                                                uint32_t word_idx,
                                                bool word_only) {
  sel_tbl_info_t *sel_tbl_info = NULL;
  sel_grp_stage_info_t *grp_stage = NULL;
  sel_tbl_t *sel_tbl = NULL;
  sel_tbl_stage_info_t *stage_info = NULL;
  uint32_t stage_idx = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t i = 0;

  sel_tbl_info = pipe_mgr_sel_tbl_info_get(dev_id, sel_tbl_hdl, false);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d sel table not found for handle 0x%x",
              __func__,
              __LINE__,
              sel_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  for (i = 0; i < sel_tbl_info->no_sel_tbls; i++) {
    sel_tbl = &sel_tbl_info->sel_tbl[i];
    stage_idx = pipe_mgr_sel_tbl_get_stage_idx(sel_tbl, stage_id);
    if (stage_idx == (uint32_t)-1) {
      rc = PIPE_INVALID_ARG;
      return rc;
    }

    stage_info = &sel_tbl->sel_tbl_stage_info[stage_idx];

    aim_printf(&uc->pvs,
               "%-10s: %-d\n"
               "%-10s: %-d\n",
               "Stage ID",
               stage_info->stage_id,
               "No. words",
               stage_info->no_words);

    aim_printf(&uc->pvs, "\n\n");

    if (!word_only) {
      aim_printf(&uc->pvs, "FREE Groups\n");

      for (grp_stage = stage_info->sel_grp_stage_free_list; grp_stage;
           grp_stage = grp_stage->next) {
        pipe_mgr_sel_ucli_dump_grp_stage(uc, grp_stage);
      }

      aim_printf(&uc->pvs, "\n\nInuse Groups\n");

      for (grp_stage = stage_info->sel_grp_stage_inuse_list; grp_stage;
           grp_stage = grp_stage->next) {
        pipe_mgr_sel_ucli_dump_grp_stage(uc, grp_stage);
      }
    } else {
      pipe_mgr_sel_ucli_dump_word_data(uc, stage_info, word_idx, "\t\t");
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_ucli_dump_grp_info(ucli_context_t *uc,
                                              bf_dev_id_t dev_id,
                                              pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                              pipe_sel_grp_hdl_t sel_grp_hdl,
                                              bool stage_print,
                                              uint8_t stage_id) {
  sel_tbl_info_t *sel_tbl_info = NULL;
  // pipe_status_t               rc = PIPE_SUCCESS;
  sel_grp_info_t *sel_grp_info = NULL;
  sel_tbl_t *sel_tbl = NULL;
  sel_tbl_stage_info_t *sel_stage = NULL;
  Pvoid_t stage_lookup;
  uint32_t pipe_idx;
  uint32_t stage_idx;
  sel_grp_stage_info_t *sel_grp_stage_info = NULL;
  sel_grp_stage_info_t *sel_grp_stage_info_print = NULL;
  bf_dev_pipe_t pipe_id;

  sel_tbl_info = pipe_mgr_sel_tbl_info_get(dev_id, sel_tbl_hdl, false);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d sel table not found for handle 0x%x",
              __func__,
              __LINE__,
              sel_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (SEL_TBL_IS_SYMMETRIC(sel_tbl_info))
    pipe_id = BF_DEV_PIPE_ALL;
  else
    pipe_id = PIPE_GET_HDL_PIPE(sel_grp_hdl);

  sel_tbl = get_sel_tbl_by_pipe_id(sel_tbl_info, pipe_id);
  if (!sel_tbl) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Selector table for pipe 0x%x (grp_hdl %x) not found",
        __func__,
        __LINE__,
        sel_tbl_info->name,
        sel_tbl_info->dev_id,
        sel_tbl_info->tbl_hdl,
        pipe_id,
        sel_grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  sel_grp_info = pipe_mgr_sel_grp_get(sel_tbl, sel_grp_hdl);
  if (sel_grp_info == NULL) {
    aim_printf(&uc->pvs, "Group %d not found\n", sel_grp_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  aim_printf(&uc->pvs, "ACTIVATED IN STAGES:\n");

  aim_printf(&uc->pvs, "%-7s|%-8s\n", "-------", "--------");
  aim_printf(&uc->pvs, "%-7s|%-8s\n", "pipe_id", "stage_id");
  aim_printf(&uc->pvs, "%-7s|%-8s\n", "-------", "--------");

  JUDYL_FOREACH(
      sel_grp_info->sel_grp_pipe_lookup, pipe_idx, Pvoid_t, stage_lookup) {
    sel_tbl = &sel_tbl_info->sel_tbl[pipe_idx];
    JUDYL_FOREACH2(stage_lookup,
                   stage_idx,
                   sel_grp_stage_info_t *,
                   sel_grp_stage_info,
                   2) {
      sel_stage = &sel_tbl->sel_tbl_stage_info[stage_idx];

      if (stage_print && (sel_stage->stage_id == stage_id)) {
        sel_grp_stage_info_print = sel_grp_stage_info;
      }
      //        sel_grp_stage_info = sel_grp_hw_locator->grp_stage_p;
      aim_printf(
          &uc->pvs, "%-7d|%-8d\n", sel_tbl->pipe_id, sel_stage->stage_id);
    }
  }

  pipe_mgr_sel_ucli_dump_grp_stage(uc, sel_grp_stage_info_print);

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_tbl_dump_one(ucli_context_t *uc,
                                        sel_tbl_info_t *sel_tbl_info,
                                        bool detailed) {
  sel_grp_info_t *sel_grp;
  sel_tbl_t *sel_tbl;
  Word_t count;
  Word_t grp_hdl;
  PWord_t Pvalue;

  count = pipe_mgr_sel_group_count(sel_tbl_info, BF_DEV_PIPE_ALL);
  aim_printf(&uc->pvs,
             "0x%-8x|%-4s|%-3s|%-6d|%-8lu\n",
             sel_tbl_info->tbl_hdl,
             SEL_TBL_IS_RESILIENT(sel_tbl_info) ? "R" : "F",
             sel_tbl_info->is_symmetric ? "Y" : "N",
             sel_tbl_info->num_scopes,
             count);

  if (detailed) {
    uint32_t q = 0, i = 0;
    aim_printf(&uc->pvs, "\nGROUPS\n");
    aim_printf(&uc->pvs, "%-10s\t%-10s\n", "GRP HDL", "MBR Count");

    for (i = 0; i < sel_tbl_info->no_sel_tbls; i++) {
      sel_tbl = &sel_tbl_info->sel_tbl[i];
      grp_hdl = 0;
      JLF(Pvalue, sel_tbl->sel_grp_array, grp_hdl);
      while (Pvalue) {
        sel_grp = (sel_grp_info_t *)*Pvalue;

        aim_printf(&uc->pvs, "0x%-8lx\t%10d\n", grp_hdl, sel_grp->mbr_count);

        uint32_t mbr_count;
        J1C(mbr_count, sel_grp->sel_grp_mbrs, 0, -1);
        PIPE_MGR_DBGCHK(sel_grp->mbr_count == mbr_count);
        JLN(Pvalue, sel_tbl->sel_grp_array, grp_hdl);
      }
    }

    aim_printf(&uc->pvs, "\nSel Tbl instances: \n");
    for (i = 0; i < sel_tbl_info->no_sel_tbls; i++) {
      sel_tbl = &sel_tbl_info->sel_tbl[i];
      aim_printf(&uc->pvs, "Pipe : %d\n", sel_tbl->pipe_id);
      aim_printf(&uc->pvs, "    Pipes in bitmap : ");
      PIPE_BITMAP_ITER(&sel_tbl->inst_pipe_bmp, q) {
        aim_printf(&uc->pvs, "%d ", q);
      }
      aim_printf(&uc->pvs, "\n");
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_ucli_dump_tbl_info(ucli_context_t *uc,
                                              bf_dev_id_t dev_id,
                                              bool valid_tbl_hdl,
                                              pipe_sel_tbl_hdl_t sel_tbl_hdl) {
  sel_tbl_info_t *sel_tbl_info = NULL;

  aim_printf(&uc->pvs,
             "%-10s|%-4s|%-3s|%-6s|%-8s\n",
             "----------",
             "----",
             "---",
             "------",
             "--------");
  aim_printf(&uc->pvs,
             "%-10s|%-4s|%-3s|%-6s|%-8s\n",
             "tbl_hdl",
             "Mode",
             "Sym",
             "Scopes",
             "No. grps");
  aim_printf(&uc->pvs,
             "%-10s|%-4s|%-3s|%-6s|%-8s\n",
             "----------",
             "----",
             "---",
             "------",
             "--------");
  if (valid_tbl_hdl) {
    sel_tbl_info = pipe_mgr_sel_tbl_info_get(dev_id, sel_tbl_hdl, false);
    if (sel_tbl_info == NULL) {
      LOG_ERROR("%s:%d sel table not found for handle 0x%x",
                __func__,
                __LINE__,
                sel_tbl_hdl);
      return PIPE_OBJ_NOT_FOUND;
    }

    pipe_mgr_sel_tbl_dump_one(uc, sel_tbl_info, true);
  } else {
    pipe_sel_tbl_hdl_t tbl_hdl;
    sel_tbl_info = pipe_mgr_sel_tbl_info_get_first(dev_id, &tbl_hdl);
    while (sel_tbl_info) {
      pipe_mgr_sel_tbl_dump_one(uc, sel_tbl_info, false);
      sel_tbl_info = pipe_mgr_sel_tbl_info_get_next(dev_id, &tbl_hdl);
    }
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_sel_ucli_dump_word_data_hw(
    ucli_context_t *uc,
    bf_dev_id_t dev_id,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    bf_dev_pipe_t pipe_id,
    uint32_t stage_id,
    uint32_t word_idx) {
  sel_tbl_info_t *sel_tbl_info = NULL;
  sel_tbl_stage_info_t *sel_stage_info = NULL;
  sel_tbl_stage_hw_info_t *stage_hw = NULL;
  uint32_t i = 0, j = 0;
  uint32_t pv_subword = 0, pv_line = 0, pv_block = 0;
  uint32_t index = 0, no_subword_bits = 0;
  uint32_t no_huffman_bits = 0, huffman_bits = 0, virt_addr = 0;
  pipe_full_virt_addr_t full_virt_addr;
  uint64_t ind_addr = 0;
  uint64_t data[2] = {0, 0};
  uint32_t word_data_width = SEL_GRP_WORD_WIDTH / 8;
  uint8_t word_data[word_data_width];
  uint8_t word_byte = 0;

  sel_tbl_info = pipe_mgr_sel_tbl_info_get(dev_id, sel_tbl_hdl, false);
  if (sel_tbl_info == NULL) {
    LOG_ERROR("%s:%d sel table not found for handle 0x%x device %d",
              __func__,
              __LINE__,
              sel_tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (pipe_id / PIPE_BITMAP_BITS_PER_WORD >=
      (uint32_t)sel_tbl_info->pipe_bmp.hdr.wordcount) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }
  if (!PIPE_BITMAP_GET(&sel_tbl_info->pipe_bmp, pipe_id)) {
    LOG_ERROR("%s:%d sel pipe %d not available for table 0x%x device %d",
              __func__,
              __LINE__,
              pipe_id,
              sel_tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  for (i = 0; i < sel_tbl_info->sel_tbl[0].num_stages; i++) {
    sel_stage_info = &sel_tbl_info->sel_tbl[0].sel_tbl_stage_info[i];
    if (stage_id == sel_stage_info->stage_id) {
      break;
    }
  }
  if (sel_stage_info == NULL) {
    LOG_ERROR(
        "%s:%d sel stage info with id %d not found"
        " for handle 0x%x device %d",
        __func__,
        __LINE__,
        (int)stage_id,
        sel_tbl_hdl,
        dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (i == sel_tbl_info->sel_tbl[0].num_stages) {
    LOG_ERROR("%s:%d sel stage %d not found for table 0x%x device %d",
              __func__,
              __LINE__,
              stage_id,
              sel_tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  stage_hw = &sel_stage_info->pv_hw;

  pipe_mgr_sel_tbl_get_phy_addr(stage_hw->pack_format,
                                word_idx,
                                &pv_subword,
                                &pv_line,
                                &pv_block,
                                TOF_SRAM_UNIT_DEPTH);
  index = (stage_hw->tbl_blk[pv_block].vpn_id[0]
           << log2_uint32_ceil(pipe_mgr_get_sram_unit_depth(dev_id))) |
          pv_line;
  no_subword_bits = 0;
  no_huffman_bits = TOF_SEL_SUBWORD_VPN_BITS - no_subword_bits;
  huffman_bits = (1 << (no_huffman_bits - 1)) - 1;
  virt_addr = (index << no_huffman_bits) | huffman_bits;
  construct_full_virt_addr(sel_tbl_info->dev_info,
                           &full_virt_addr,
                           stage_hw->tbl_id,
                           pipe_virt_mem_type_sel_stful,
                           virt_addr,
                           pipe_id,
                           stage_id);
  ind_addr = full_virt_addr.addr;
  bf_subdev_id_t subdev = pipe_mgr_subdev_id_from_pipe(pipe_id);
  lld_subdev_ind_read(dev_id, subdev, ind_addr, &data[1], &data[0]);
  PIPE_MGR_MEMCPY(word_data, data, word_data_width);

  aim_printf(&uc->pvs, "\n\t\tSelector word idx %d\t", word_idx);
  if (SEL_TBL_IS_RESILIENT(sel_tbl_info)) {
    aim_printf(&uc->pvs,
               "\n\n\t\tHighest mbr index: %d\t",
               word_data[word_data_width - 1]);
  } else {
    aim_printf(&uc->pvs,
               "\n\n\t\tNumber of active members: %d\t",
               word_data[word_data_width - 1]);
  }

  aim_printf(&uc->pvs, "\n\n\t\tMember bitmap:\t\n\t\t");
  for (i = 0; i < word_data_width - 1; i++) {
    // Make bytestream human-readable (left to right)
    word_byte = 0;
    for (j = 0; j < 4; j++) {
      word_byte |= ((word_data[i] & (1 << j)) << (7 - 2 * j));
      word_byte |= ((word_data[i] & (1 << (4 + j))) >> (2 * j + 1));
    }
    aim_printf(&uc->pvs, "%02hhX\t", word_byte);
  }
  aim_printf(&uc->pvs, "\n\n\n");

  return PIPE_SUCCESS;
}

PIPE_MGR_SEL_TBL_CLI_CMD_DECLARE(grp_info) {
  PIPE_MGR_CLI_PROLOGUE(
      "grp-info",
      "Dump the group information.",
      "-d <dev_id> -h <tbl_handle> -g <grp_handle> [-s <stage_id>]");

  bool got_dev = false;
  bool got_tbl_hdl = false;
  bool got_stage_id = false;
  bool got_grp_hdl = false;

  bf_dev_id_t dev_id = 0;
  pipe_mat_tbl_hdl_t tbl_hdl = 0;
  pipe_sel_grp_hdl_t grp_hdl = 0;
  uint32_t stage_id = 0;

  int x;
  while (-1 != (x = getopt(argc, argv, "d:h:g:s:"))) {
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
      case 'g':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        grp_hdl = strtoul(optarg, NULL, 0);
        got_grp_hdl = true;
        break;
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        stage_id = strtoul(optarg, NULL, 0);
        got_stage_id = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }
  if (!got_dev || !got_tbl_hdl || !got_grp_hdl) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  pipe_mgr_sel_ucli_dump_grp_info(
      uc, dev_id, tbl_hdl, grp_hdl, got_stage_id, stage_id);

  return UCLI_STATUS_OK;
}

PIPE_MGR_SEL_TBL_CLI_CMD_DECLARE(tbl_info) {
  PIPE_MGR_CLI_PROLOGUE("tbl-info",
                        "Print the select table info",
                        "-d <dev_id> [-h <tbl_handle>]");

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

  pipe_mgr_sel_ucli_dump_tbl_info(uc, dev_id, got_tbl_hdl, tbl_hdl);

  return UCLI_STATUS_OK;
}

PIPE_MGR_SEL_TBL_CLI_CMD_DECLARE(stage_info) {
  PIPE_MGR_CLI_PROLOGUE(
      "stage-info",
      "Dump the stage information.",
      "-d <dev_id> -h <tbl_handle> -s <stage_id> [-w <word_idx>]");

  bool got_dev = false;
  bool got_tbl_hdl = false;
  bool got_stage_id = false;
  bool got_word_idx = false;

  bf_dev_id_t dev_id = 0;
  pipe_mat_tbl_hdl_t tbl_hdl = 0;
  uint32_t stage_id = 0;
  uint32_t word_idx = 0;

  int x;
  while (-1 != (x = getopt(argc, argv, "d:h:s:w:"))) {
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
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        stage_id = strtoul(optarg, NULL, 0);
        got_stage_id = true;
        break;
      case 'w':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        word_idx = strtoul(optarg, NULL, 0);
        got_word_idx = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }
  if (!got_dev || !got_tbl_hdl || !got_stage_id) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  pipe_mgr_sel_ucli_dump_stage_info(
      uc, dev_id, tbl_hdl, stage_id, word_idx, got_word_idx);

  return UCLI_STATUS_OK;
}

PIPE_MGR_SEL_TBL_CLI_CMD_DECLARE(entry_count) {
  PIPE_MGR_CLI_PROLOGUE("entry-count",
                        "Print the select table entry count",
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
  if (!got_dev || !got_tbl_hdl) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  uint32_t count = 0;
  dev_target_t dev_tgt = {.device_id = dev_id, .dev_pipe_id = BF_DEV_PIPE_ALL};
  pipe_status_t sts = PIPE_OBJ_NOT_FOUND;
  if (pipe_mgr_is_device_virtual(dev_id)) {
    sts = pipe_mgr_sel_tbl_get_placed_entry_count(dev_tgt, tbl_hdl, &count);
  } else {
    sts = pipe_mgr_sel_tbl_get_programmed_entry_count(dev_tgt, tbl_hdl, &count);
  }

  if (sts == PIPE_SUCCESS) {
    aim_printf(
        &uc->pvs, "select table 0x%x: %u group(s) occupied\n", tbl_hdl, count);
  } else {
    aim_printf(&uc->pvs, "No information found for the table specified\n");
  }

  return UCLI_STATUS_OK;
}

PIPE_MGR_SEL_TBL_CLI_CMD_DECLARE(ram_info) {
  PIPE_MGR_CLI_PROLOGUE(
      "ram-info",
      "Dump the selector ram line from hardware.",
      "-d <dev_id> -h <tbl_handle> -p <pipe_id> -s <stage_id> -w <word_idx>");

  bool got_dev = false;
  bool got_tbl_hdl = false;
  bool got_pipe_id = false;
  bool got_stage_id = false;
  bool got_word_idx = false;

  bf_dev_id_t dev_id = 0;
  pipe_mat_tbl_hdl_t tbl_hdl = 0;
  bf_dev_pipe_t pipe_id = 0;
  uint32_t stage_id = 0;
  uint32_t sel_word_idx = 0;

  int x;
  while (-1 != (x = getopt(argc, argv, "d:h:p:s:w:"))) {
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
      case 'p':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        pipe_id = strtoul(optarg, NULL, 0);
        got_pipe_id = true;
        break;
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        stage_id = strtoul(optarg, NULL, 0);
        got_stage_id = true;
        break;
      case 'w':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        sel_word_idx = strtoul(optarg, NULL, 0);
        got_word_idx = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }
  if (!got_dev || !got_tbl_hdl || !got_pipe_id || !got_stage_id ||
      !got_word_idx) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (pipe_mgr_is_device_virtual(dev_id)) {
    aim_printf(
        &uc->pvs, "Cannot fetch hardware data from virtual device %d", dev_id);
    return UCLI_STATUS_OK;
  }

  pipe_mgr_sel_ucli_dump_word_data_hw(
      uc, dev_id, tbl_hdl, pipe_id, stage_id, sel_word_idx);

  return UCLI_STATUS_OK;
}

/* <auto.ucli.handlers.start> */
static ucli_command_handler_f pipe_mgr_sel_tbl_ucli_ucli_handlers__[] = {
    PIPE_MGR_SEL_TBL_CLI_CMD_HNDLR(tbl_info),
    PIPE_MGR_SEL_TBL_CLI_CMD_HNDLR(grp_info),
    PIPE_MGR_SEL_TBL_CLI_CMD_HNDLR(stage_info),
    PIPE_MGR_SEL_TBL_CLI_CMD_HNDLR(entry_count),
    PIPE_MGR_SEL_TBL_CLI_CMD_HNDLR(ram_info),
    NULL};

/* <auto.ucli.handlers.end> */

static ucli_module_t pipe_mgr_sel_tbl_ucli_module__ = {
    "sel_tbl_ucli",
    NULL,
    pipe_mgr_sel_tbl_ucli_ucli_handlers__,
    NULL,
    NULL,
};

ucli_node_t *pipe_mgr_sel_tbl_ucli_node_create(ucli_node_t *n) {
  ucli_node_t *m;
  ucli_module_init(&pipe_mgr_sel_tbl_ucli_module__);
  m = ucli_node_create("sel_tbl", n, &pipe_mgr_sel_tbl_ucli_module__);
  ucli_node_subnode_add(m, ucli_module_log_node_create("sel_tbl"));
  return m;
}

#else
void *pipe_mgr_sel_tbl_ucli_node_create(void) { return NULL; }
#endif
