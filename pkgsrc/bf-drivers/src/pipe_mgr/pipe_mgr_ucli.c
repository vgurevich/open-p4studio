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


/* Standard includes */
#include <getopt.h>
#include <limits.h>
#include <unistd.h>
#include <time.h>

/* Module includes */
#include <target-sys/bf_sal/bf_sys_intf.h>
#include <dvm/bf_drv_intf.h>
#include <lld/bf_dma_if.h>
#include <lld/lld_reg_if.h>
#include <lld/lld_tof_addr_conversion.h>
#include <lld/lld_tof2_addr_conversion.h>
#include <tofino_regs/tofino.h>
#include <tofino_regs/pipe_top_level.h>
#include <tof2_regs/tof2_mem_drv.h>
#include <tof3_regs/tof3_mem_drv.h>

/* Local includes */
#include "pipe_mgr_int.h"
#include "pipe_mgr_drv.h"
#include "pipe_mgr_exm_ucli.h"
#include "pipe_mgr_adt_ucli.h"
#include "pipe_mgr_stat_ucli.h"
#include "pipe_mgr_meter_ucli.h"
#include "pipe_mgr_stful_ucli.h"
#include "pipe_mgr_select_tbl.h"
#include "pipe_mgr_tcam.h"
#include "pipe_mgr_tcam_hw.h"
#include "pipe_mgr_learn.h"
#include "pipe_mgr_idle.h"
#include "pipe_mgr_hw_dump.h"
#include "pipe_mgr_phy_mem_map.h"
#include "pipe_mgr_db.h"
#include "pipe_mgr_interrupt.h"
#include "pipe_mgr_tof2_interrupt.h"
#include "pipe_mgr_mau_snapshot.h"
#include "pipe_mgr_mau_tbl_dbg_counters.h"
#include "src/pipe_mgr/pipe_mgr_alpm.h"

#if PIPE_MGR_CONFIG_INCLUDE_UCLI == 1

#include <target-utils/uCli/ucli.h>
#include <target-utils/uCli/ucli_argparse.h>
#include <target-utils/uCli/ucli_handler_macros.h>

extern char *lld_reg_parse_get_full_reg_path_name(bf_dev_family_t dev_family,
                                                  uint32_t offset);

#define PIPE_MGR_CLI_CMD_HNDLR(name) pipe_mgr_ucli_ucli__##name##__
#define PIPE_MGR_CLI_CMD_DECLARE(name) \
  static ucli_status_t PIPE_MGR_CLI_CMD_HNDLR(name)(ucli_context_t * uc)
#define PIPE_MGR_SNAPSHOT_HW_DUMP_STR_LEN 550000
#define VALID_PRSR_STEP(prsr_step) ((prsr_step) == 1 || (prsr_step) == 4)
/* Routine to dump device level info */
static void pipe_mgr_ucli_dump_dev_info(ucli_context_t *uc, unsigned dev_id) {
  rmt_dev_info_t *dev_info;
  extern pipe_mgr_ctx_t *pipe_mgr_ctx;
  uint8_t num_stages;
  aim_printf(&uc->pvs,
             "%-6s-%-19s-%-5s-%-4s-%-5s-%-7s-%-8s\n",
             "------",
             "-------------------",
             "-----",
             "----",
             "-----",
             "-------",
             "--------");
  aim_printf(&uc->pvs,
             "%-6s|%-19s|%-5s|%-4s|%-5s|%-7s|%-8s\n",
             "Device",
             "Type",
             "#pipe",
             "#stg",
             "#prsr",
             "#macblk",
             "#sub-dev");
  aim_printf(&uc->pvs,
             "%-6s|%-19s|%-5s|%-4s|%-5s|%-7s|%-8s\n",
             "------",
             "-------------------",
             "-----",
             "----",
             "-----",
             "-------",
             "--------");

  if (dev_id != 0xff) {
    dev_info = pipe_mgr_get_dev_info(dev_id);
    if (dev_info == NULL) {
      aim_printf(&uc->pvs, "device <%d> not found\n", dev_id);
      return;
    }
    pipe_mgr_get_num_active_stages(dev_id, &num_stages);
    aim_printf(&uc->pvs,
               "%-6d|%-19s|%-5d|%-4d|%-5d|%-7d|%-8d\n",
               dev_info->dev_id,
               pipe_mgr_dev_type2str(dev_info->dev_type),
               dev_info->num_active_pipes,
               num_stages,
               dev_info->dev_cfg.num_prsrs,
               (dev_info->num_active_pipes *
                lld_get_max_frontport_mac_per_pipe(dev_id)) +
                   1, /*CPU port MAC*/
               dev_info->num_active_subdevices);
  } else {
    if (!pipe_mgr_ctx) return;
    bf_map_sts_t s;
    unsigned long key;
    for (s = bf_map_get_first(
             &pipe_mgr_ctx->dev_info_map, &key, (void **)&dev_info);
         s == BF_MAP_OK;
         s = bf_map_get_next(
             &pipe_mgr_ctx->dev_info_map, &key, (void **)&dev_info)) {
      if (dev_info) {
        pipe_mgr_get_num_active_stages(dev_info->dev_id, &num_stages);
        aim_printf(&uc->pvs,
                   "%-6d|%-19s|%-5d|%-4d|%-5d|%-7d|%-8d\n",
                   dev_info->dev_id,
                   pipe_mgr_dev_type2str(dev_info->dev_type),
                   dev_info->num_active_pipes,
                   num_stages,
                   dev_info->dev_cfg.num_prsrs,
                   (dev_info->num_active_pipes *
                    lld_get_max_frontport_mac_per_pipe(dev_id)) +
                       1, /*CPU port MAC*/
                   dev_info->num_active_subdevices);
      }
    }
  }
}

static void pipe_mgr_ucli_dump_profile_info(ucli_context_t *uc,
                                            unsigned dev_id) {
  rmt_dev_info_t *dev_info = NULL;
  rmt_dev_profile_info_t *profile_info = NULL;
  uint32_t p = 0, pipe = 0;
  char pipe_str[200];

  dev_info = pipe_mgr_get_dev_info(dev_id);
  if (dev_info == NULL) {
    aim_printf(&uc->pvs, "device <%d> not found\n", dev_id);
    return;
  }

  aim_printf(&uc->pvs,
             "Num of pipeline profiles: %d \n",
             dev_info->num_pipeline_profiles);
  aim_printf(&uc->pvs, "---------------------------------------------\n");
  aim_printf(&uc->pvs,
             "%-10s | %-15s | %-15s\n",
             "Profile-id",
             "Name",
             "Pipes in scope");
  aim_printf(&uc->pvs, "---------------------------------------------\n");

  for (p = 0; p < dev_info->num_pipeline_profiles; p++) {
    profile_info = dev_info->profile_info[p];
    memset(pipe_str, 0, sizeof(pipe_str));
    PIPE_BITMAP_ITER(&profile_info->pipe_bmp, pipe) {
      snprintf(
          pipe_str + strlen(pipe_str), 200 - strlen(pipe_str) - 1, "%d ", pipe);
    }
    aim_printf(&uc->pvs,
               "%-10d | %-15s | %-15s\n",
               profile_info->profile_id,
               profile_info->pipeline_name,
               pipe_str);
  }
}

static char *pipe_mgr_tbl_type2str(pipe_mat_match_type_t match_type) {
  switch (match_type) {
    case EXACT_MATCH:
      return "exm";
    case TERNARY_MATCH:
      return "term";
    case LONGEST_PREFIX_MATCH:
      return "lpm";
    case ATCAM_MATCH:
      return "atcam";
    case ALPM_MATCH:
      return "alpm";
    default:
      return "na";
  }
}

static char *pipe_mgr_tbl_ref2str(pipe_tbl_ref_type_t ref_type) {
  switch (ref_type) {
    case PIPE_TBL_REF_TYPE_DIRECT:
      return "dir";
    case PIPE_TBL_REF_TYPE_INDIRECT:
      return "ind";
    default:
      return "bad";
  }
}

static char *pipe_mgr_rmt_mem_type2str(rmt_mem_type_t mem_type) {
  switch (mem_type) {
    case RMT_MEM_SRAM:
      return "SRAM";
    case RMT_MEM_TCAM:
      return "TCAM";
    case RMT_MEM_MAP_RAM:
      return "MAP RAM";




    default:
      return "bad";
  }
}

const char *pipe_mgr_mem_type2str(pipe_mem_type_t mem_type) {
  switch (mem_type) {
    case pipe_mem_type_unit_ram:
      return "Sram";
    case pipe_mem_type_map_ram:
      return "Map Ram";
    case pipe_mem_type_stats_deferred_access_ram:
      return "Stats Ram";
    case pipe_mem_type_meter_deferred_access_ram:
      return "Meter Ram";
    case pipe_mem_type_tcam:
      return "Tcam";
    default:
      return "Unknown";
  }
}

static void pipe_mgr_ucli_print_pipe_info(ucli_context_t *uc,
                                          rmt_dev_info_t *dev_info,
                                          uint8_t pipe_id) {
  bf_dev_pipe_t phy_pipe = 0;
  aim_printf(&uc->pvs, "Pipeline %d:\n", pipe_id);
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe_id, &phy_pipe);
  aim_printf(&uc->pvs,
             "    Logical pipe %d maps to Physical pipe %d\n",
             pipe_id,
             phy_pipe);
}

/* Routine to dump device pipeline level info */
static void pipe_mgr_ucli_dump_pipe_info(ucli_context_t *uc,
                                         bf_dev_id_t dev_id,
                                         uint8_t pipe_id) {
  rmt_dev_info_t *dev_info;
  extern pipe_mgr_ctx_t *pipe_mgr_ctx;
  uint8_t i;

  dev_info = pipe_mgr_get_dev_info(dev_id);
  if (dev_info == NULL) {
    aim_printf(&uc->pvs, "device <%d> not found\n", dev_id);
    return;
  }

  if (pipe_id != 0xff) {
    if (pipe_id >= dev_info->num_active_pipes) {
      aim_printf(&uc->pvs, "Invalid pipe <%d> \n", pipe_id);
      return;
    }
    pipe_mgr_ucli_print_pipe_info(uc, dev_info, pipe_id);
  } else {
    for (i = 0; i < dev_info->num_active_pipes; i++) {
      pipe_mgr_ucli_print_pipe_info(uc, dev_info, i);
    }
  }
}

static void pipe_mgr_ucli_print_mat_tbl_summary(ucli_context_t *uc,
                                                rmt_dev_info_t *dev_info) {
  int i;
  pipe_mat_tbl_info_t *mat;
  uint32_t p = 0;
  uint32_t name_len = 24;

  for (p = 0; p < dev_info->num_pipeline_profiles; p++) {
    for (i = 0; i < dev_info->profile_info[p]->tbl_info_list.num_mat_tbls;
         i++) {
      mat = &(dev_info->profile_info[p]->tbl_info_list.mat_tbl_list[i]);
      if (mat && mat->name && strlen(mat->name) > name_len) {
        name_len = strlen(mat->name);
      }
    }
  }

  char name_div[name_len + 1];
  PIPE_MGR_MEMSET(name_div, '-', name_len);
  name_div[name_len] = '\0';

  aim_printf(&uc->pvs, "\nMatch-Action Tables:\n\n");
  aim_printf(&uc->pvs,
             "%-*s|%-10s|%-5s|%-8s|%-5s|%-4s|%-3s|%-3s|%-3s|"
             "%-3s|%-4s|%-4s|%-5s\n",
             name_len,
             name_div,
             "----------",
             "-----",
             "--------",
             "-----",
             "----",
             "---",
             "---",
             "---",
             "---",
             "----",
             "----",
             "-----");
  aim_printf(&uc->pvs,
             "%-*s|%-10s|%-5s|%-8s|%-5s|%-4s|%-3s|%-3s|%-3s|"
             "%-3s|%-4s|%-4s|%-5s\n",
             name_len,
             "Name",
             "Handle",
             "Type",
             "Entries",
             "Keysz",
             "Stgs",
             "Adt",
             "Sel",
             "Sta",
             "Met",
             "Sful",
             "Prof",
             "Owner");
  aim_printf(&uc->pvs,
             "%-*s|%-10s|%-5s|%-8s|%-5s|%-4s|%-3s|%-3s|%-3s|"
             "%-3s|%-4s|%-4s|%-5s\n",
             name_len,
             name_div,
             "----------",
             "-----",
             "--------",
             "-----",
             "----",
             "---",
             "---",
             "---",
             "---",
             "----",
             "----",
             "-----");

  for (p = 0; p < dev_info->num_pipeline_profiles; p++) {
    for (i = 0; i < dev_info->profile_info[p]->tbl_info_list.num_mat_tbls;
         i++) {
      mat = &(dev_info->profile_info[p]->tbl_info_list.mat_tbl_list[i]);
      if (mat) {
        aim_printf(&uc->pvs,
                   "%-*s|0x%-8x|%-5s|%-8d|%-5x|%-4d|%-3s|%-3s|%-3s|"
                   "%-3s|%-4s|%-4d|%-5s\n",
                   name_len,
                   mat->name ? mat->name : "null",
                   mat->handle,
                   pipe_mgr_tbl_type2str(mat->match_type),
                   mat->size,
                   mat->match_key_width,
                   mat->num_stages,
                   mat->adt_tbl_ref
                       ? (pipe_mgr_tbl_ref2str(mat->adt_tbl_ref->ref_type))
                       : "NA",
                   mat->sel_tbl_ref
                       ? (pipe_mgr_tbl_ref2str(mat->sel_tbl_ref->ref_type))
                       : "NA",
                   "NA",
                   "NA",
                   "NA",
                   dev_info->profile_info[p]->profile_id,
                   pipe_mgr_table_owner_str(
                       pipe_mgr_sm_tbl_owner(dev_info->dev_id, mat->handle)));
      }
    }
  }
}

static void pipe_mgr_print_bank_tbl_word_blks(ucli_context_t *uc,
                                              rmt_tbl_bank_map_t *bank_map,
                                              uint8_t mem_units_per_tbl_word,
                                              uint8_t vpns_per_tbl_word) {
  int i, j;
  char mem_str[128];
  char vpn_str[128];

  aim_printf(&uc->pvs, "\t\t%-50s:%s\n", "[Unit Mems]", "[VPNs]");
  for (i = 0; i < bank_map->num_tbl_word_blks; i++) {
    int mem_str_pos = 0, vpn_str_pos = 0;
    /* Construct mem_id string */
    mem_str_pos += sprintf(&mem_str[mem_str_pos], "[");
    for (j = 0; j < mem_units_per_tbl_word; j++) {
      mem_str_pos += sprintf(
          &mem_str[mem_str_pos], "%d ", bank_map->tbl_word_blk[i].mem_id[j]);
    }
    sprintf(&mem_str[mem_str_pos], "]");

    /* Construct vpn_id string */
    vpn_str_pos += sprintf(&vpn_str[vpn_str_pos], "[");
    for (j = 0; j < vpns_per_tbl_word; j++) {
      vpn_str_pos += sprintf(
          &vpn_str[vpn_str_pos], "%d ", bank_map->tbl_word_blk[i].vpn_id[j]);
    }
    sprintf(&vpn_str[vpn_str_pos], "]");
    aim_printf(&uc->pvs, "\t\t%-50s:%s\n", mem_str, vpn_str);
  }
}

static void pipe_mgr_print_rmt_tbl_specific_info(ucli_context_t *uc,
                                                 rmt_tbl_info_t *rmt_info) {
  switch (rmt_info->type) {
    case RMT_TBL_TYPE_IDLE_TMO: {
      aim_printf(&uc->pvs,
                 "\t%-20s: %d\n",
                 "Bit width",
                 rmt_info->params.idle.bit_width);
      aim_printf(&uc->pvs,
                 "\t%-20s: %s\n",
                 "Notification",
                 rmt_info->params.idle.notify_disable ? "Disabled" : "Enabled");
      aim_printf(
          &uc->pvs,
          "\t%-20s: %s\n",
          "2-way notify",
          rmt_info->params.idle.two_way_notify_enable ? "Enabled" : "Disabled");
      aim_printf(&uc->pvs,
                 "\t%-20s: %s\n",
                 "Per-flow enable",
                 rmt_info->params.idle.per_flow_enable ? "True" : "False");
    } break;
    default:
      break;
  }
}

static void pipe_mgr_ucli_print_rmt_tbl_info(ucli_context_t *uc,
                                             rmt_tbl_info_t *rmt_info) {
  rmt_tbl_bank_map_t *bank_map;
  int i;

  aim_printf(&uc->pvs, "\t%-20s: %d\n", "Stage Id", rmt_info->stage_id);
  aim_printf(&uc->pvs,
             "\t%-20s: %s\n",
             "Stage Table Type",
             pipe_mgr_rmt_tbl_type2str(rmt_info->type));
  aim_printf(&uc->pvs, "\t%-20s: %x\n", "Stage Handle", rmt_info->handle);
  aim_printf(
      &uc->pvs, "\t%-20s: %d\n", "Num Stage Entries", rmt_info->num_entries);
  aim_printf(&uc->pvs,
             "\t%-20s: %s\n",
             "Memory Type",
             pipe_mgr_rmt_mem_type2str(rmt_info->mem_type));
  aim_printf(&uc->pvs, "\t%-20s:\n", "Pack Format");
  aim_printf(&uc->pvs,
             "\t\t%-20s: %d\n",
             "Mem Word Width",
             rmt_info->pack_format.mem_word_width);
  aim_printf(&uc->pvs,
             "\t\t%-20s: %d\n",
             "Table Word Width",
             rmt_info->pack_format.tbl_word_width);
  aim_printf(&uc->pvs,
             "\t\t%-20s: %d\n",
             "Entries/Table Word",
             rmt_info->pack_format.entries_per_tbl_word);
  aim_printf(&uc->pvs,
             "\t\t%-20s: %d\n",
             "Mem Units/Table Word",
             rmt_info->pack_format.mem_units_per_tbl_word);

  pipe_mgr_print_rmt_tbl_specific_info(uc, rmt_info);

  switch (rmt_info->type) {
    case RMT_TBL_TYPE_HASH_MATCH:
    case RMT_TBL_TYPE_PROXY_HASH:
      if (rmt_info->num_tbl_banks) PIPE_MGR_ASSERT(rmt_info->bank_map);
      for (i = 0; i < rmt_info->num_tbl_banks; i++) {
        char hash_bits[80];
        aim_printf(&uc->pvs, "\t[%d]-%-30s:\n", i, "Hash Way Mem Resources");
        bank_map = &(rmt_info->bank_map[i]);
        pipe_mgr_print_bank_tbl_word_blks(
            uc,
            bank_map,
            rmt_info->pack_format.mem_units_per_tbl_word,
            rmt_info->pack_format.entries_per_tbl_word);
        sprintf(hash_bits,
                "%s:(%d-%d) (%d-%d)",
                (bank_map->hash_bits.function == RMT_TBL_HASH_FN_A)
                    ? "Hash_Fn_A"
                    : "Hash_Fn_B",
                bank_map->hash_bits.ram_unit_select_bit_lo,
                bank_map->hash_bits.ram_unit_select_bit_hi,
                bank_map->hash_bits.ram_line_select_bit_lo,
                bank_map->hash_bits.ram_line_select_bit_hi);
        aim_printf(&uc->pvs, "\n\t\t%s:%s\n", "Hash Bits", hash_bits);
      }
      break;
    case RMT_TBL_TYPE_ATCAM_MATCH:
      PIPE_MGR_ASSERT(rmt_info->bank_map);
      for (i = 0; i < rmt_info->num_tbl_banks; i++) {
        aim_printf(&uc->pvs, "\t[%d]-%-30s:\n", i, "Bin depth Mem Resources");
        bank_map = &(rmt_info->bank_map[i]);
        pipe_mgr_print_bank_tbl_word_blks(
            uc,
            bank_map,
            rmt_info->pack_format.mem_units_per_tbl_word,
            rmt_info->pack_format.entries_per_tbl_word);
      }
      break;
    default:
      if (rmt_info->num_tbl_banks == 1) {
        aim_printf(&uc->pvs, "\t%-20s:\n", "Mem Resources");
        bank_map = &(rmt_info->bank_map[0]);
        pipe_mgr_print_bank_tbl_word_blks(
            uc, bank_map, rmt_info->pack_format.mem_units_per_tbl_word, 1);
      }
      break;
  }
  aim_printf(&uc->pvs, "\n");
}

static void pipe_mgr_ucli_print_mat_tbl_info(ucli_context_t *uc,
                                             rmt_dev_info_t *dev_info,
                                             unsigned handle,
                                             uint8_t stage_id) {
  unsigned i;
  uint32_t p = 0;
  pipe_mat_tbl_info_t *mat = NULL;
  rmt_tbl_info_t *rmt_info = NULL;

  /* Locate table info for handle */
  bool found = false;
  for (p = 0; p < dev_info->num_pipeline_profiles; p++) {
    for (i = 0; i < dev_info->profile_info[p]->tbl_info_list.num_mat_tbls;
         i++) {
      mat = &(dev_info->profile_info[p]->tbl_info_list.mat_tbl_list[i]);
      if (mat && (mat->handle == handle)) {
        found = true;
        break;
      }
    }
    if (found) {
      break;
    }
  }
  if (!mat) {
    aim_printf(&uc->pvs, "tbl: Handle %x not found\n", handle);
    return;
  }

  aim_printf(&uc->pvs,
             "\nMatch-Action Table Info: %s(%x)\n\n",
             mat->name ? mat->name : "null",
             mat->handle);
  aim_printf(&uc->pvs,
             "%-20s: %s\n",
             "Match Type",
             pipe_mgr_tbl_type2str(mat->match_type));
  aim_printf(&uc->pvs, "%-20s: %d\n", "Num Entries", mat->size);
  aim_printf(&uc->pvs, "%-20s: %d\n", "Match Key Width", mat->match_key_width);
  aim_printf(&uc->pvs, "%-20s: %d\n", "Num Stages", mat->num_stages);
  aim_printf(&uc->pvs, "%-20s: %d\n", "Num Partitions", mat->num_partitions);
  if (mat->adt_tbl_ref) {
    aim_printf(&uc->pvs,
               "%-20s: %x(%s)\n",
               "Action Table",
               mat->adt_tbl_ref->tbl_hdl,
               pipe_mgr_tbl_ref2str(mat->adt_tbl_ref->ref_type));
  }
  if (mat->sel_tbl_ref) {
    aim_printf(&uc->pvs,
               "%-20s: %x(%s)\n",
               "Selection Table",
               mat->sel_tbl_ref->tbl_hdl,
               pipe_mgr_tbl_ref2str(mat->sel_tbl_ref->ref_type));
  }
  if (mat->stat_tbl_ref) {
    aim_printf(&uc->pvs,
               "%-20s: %x(%s)\n",
               "Statistics Table",
               mat->stat_tbl_ref->tbl_hdl,
               pipe_mgr_tbl_ref2str(mat->stat_tbl_ref->ref_type));
  }
  if (mat->meter_tbl_ref) {
    aim_printf(&uc->pvs,
               "%-20s: %x(%s)\n",
               "Meters Table",
               mat->meter_tbl_ref->tbl_hdl,
               pipe_mgr_tbl_ref2str(mat->meter_tbl_ref->ref_type));
  }
  if (mat->stful_tbl_ref) {
    aim_printf(&uc->pvs,
               "%-20s: %x(%s)\n",
               "Stateful Table",
               mat->stful_tbl_ref->tbl_hdl,
               pipe_mgr_tbl_ref2str(mat->stful_tbl_ref->ref_type));
  }

  aim_printf(&uc->pvs, "%-20s:\n", "Stage Table Info");
  for (i = 0; i < mat->num_rmt_info; i++) {
    rmt_info = &(mat->rmt_info[i]);
    if (rmt_info) {
      if ((stage_id == 0xff) || (rmt_info->stage_id == stage_id)) {
        pipe_mgr_ucli_print_rmt_tbl_info(uc, rmt_info);
      }
    }
  }
}

static void pipe_mgr_ucli_print_adt_tbl_info(ucli_context_t *uc,
                                             rmt_dev_info_t *dev_info,
                                             unsigned handle,
                                             uint8_t stage_id) {
  unsigned int i;
  uint32_t p = 0;
  pipe_adt_tbl_info_t *adt = NULL;
  rmt_tbl_info_t *rmt_info = NULL;

  /* Locate table info for handle */
  bool found = false;
  for (p = 0; p < dev_info->num_pipeline_profiles; p++) {
    for (i = 0; i < dev_info->profile_info[p]->tbl_info_list.num_adt_tbls;
         i++) {
      adt = &(dev_info->profile_info[p]->tbl_info_list.adt_tbl_list[i]);
      if (adt && (adt->handle == handle)) {
        found = true;
        break;
      }
    }
    if (found) {
      break;
    }
  }
  if (!adt) {
    aim_printf(&uc->pvs, "tbl: Handle %x not foundn", handle);
    return;
  }

  aim_printf(&uc->pvs,
             "\nAction-Data Table Info: %s(%x)\n\n",
             adt->name ? adt->name : "null",
             adt->handle);
  aim_printf(&uc->pvs, "%-20s: %d\n", "Num Entries", adt->size);
  aim_printf(&uc->pvs, "%-20s: %d\n", "Num Stages", adt->num_stages);

  aim_printf(&uc->pvs, "%-20s:\n", "Stage Table Info");
  for (i = 0; i < adt->num_stages; i++) {
    rmt_info = &(adt->rmt_info[i]);
    if (rmt_info) {
      if ((stage_id == 0xff) || (rmt_info->stage_id == stage_id)) {
        pipe_mgr_ucli_print_rmt_tbl_info(uc, rmt_info);
      }
    }
  }
}

static void pipe_mgr_ucli_print_sel_tbl_info(ucli_context_t *uc,
                                             rmt_dev_info_t *dev_info,
                                             unsigned handle,
                                             uint8_t stage_id) {
  unsigned int i;
  uint32_t p = 0;
  pipe_select_tbl_info_t *sel = NULL;
  rmt_tbl_info_t *rmt_info = NULL;

  /* Locate table info for handle */
  bool found = false;
  for (p = 0; p < dev_info->num_pipeline_profiles; p++) {
    for (i = 0; i < dev_info->profile_info[p]->tbl_info_list.num_select_tbls;
         i++) {
      sel = &(dev_info->profile_info[p]->tbl_info_list.select_tbl_list[i]);
      if (sel && (sel->handle == handle)) {
        found = true;
        break;
      }
    }
    if (found) {
      break;
    }
  }
  if (!sel) {
    aim_printf(&uc->pvs, "tbl: Handle %x not foundn", handle);
    return;
  }

  aim_printf(&uc->pvs,
             "\nSelection Table Info: %s(%x)\n\n",
             sel->name ? sel->name : "null",
             sel->handle);
  aim_printf(&uc->pvs, "%-20s: %d\n", "ADT handle", sel->adt_tbl_hdl);
  aim_printf(&uc->pvs, "%-20s: %d\n", "Max group size", sel->max_group_size);
  aim_printf(&uc->pvs, "%-20s: %d\n", "Num Stages", sel->num_stages);
  aim_printf(&uc->pvs,
             "%-20s: %s\n",
             "Mode",
             (sel->mode == RESILIENT) ? "Resilient" : "Fair");

  aim_printf(&uc->pvs, "%-20s:\n", "Stage Table Info");
  for (i = 0; i < sel->num_stages; i++) {
    rmt_info = &(sel->rmt_info[i]);
    if (rmt_info) {
      if ((stage_id == 0xff) || (rmt_info->stage_id == stage_id)) {
        pipe_mgr_ucli_print_rmt_tbl_info(uc, rmt_info);
      }
    }
  }
  aim_printf(&uc->pvs, "Symmetric: %d\n", sel->symmetric);
}

static char *pipe_mgr_stat_type2str(pipe_stat_type_t stat_type) {
  switch (stat_type) {
    case PACKET_COUNT:
      return "Packets";
    case BYTE_COUNT:
      return "Bytes";
    case PACKET_AND_BYTE_COUNT:
      return "Packets and Bytes";
    default:
      return "Bad";
  }
}

static void pipe_mgr_ucli_print_stat_tbl_info(ucli_context_t *uc,
                                              rmt_dev_info_t *dev_info,
                                              unsigned handle,
                                              uint8_t stage_id) {
  unsigned int i;
  pipe_stat_tbl_info_t *stat = NULL;
  rmt_tbl_info_t *rmt_info = NULL;

  stat =
      pipe_mgr_get_stat_tbl_info(dev_info->dev_id, handle, __func__, __LINE__);

  if (!stat) {
    aim_printf(&uc->pvs, "tbl : Handle %x not found\n", handle);
    return;
  }
  aim_printf(&uc->pvs,
             "\nStat Table Info: %s(%x)\n\n",
             stat->name ? stat->name : "null",
             stat->handle);
  aim_printf(&uc->pvs,
             "%-20s: %s\n",
             "Stat type",
             pipe_mgr_stat_type2str(stat->stat_type));
  aim_printf(&uc->pvs, "%-20s: %d\n", "Size", stat->size);
  aim_printf(
      &uc->pvs,
      "%-20s: %s\n",
      "Reference type",
      ((stat->ref_type == PIPE_TBL_REF_TYPE_INDIRECT) ? "INDIRECT" : "DIRECT"));
  aim_printf(&uc->pvs,
             "%-20s: %d\n",
             "Pkt Cntr Resolution",
             stat->packet_counter_resolution);
  aim_printf(&uc->pvs,
             "%-20s: %d\n",
             "Byte Cntr Resolution",
             stat->byte_counter_resolution);
  aim_printf(&uc->pvs,
             "%-20s: %s\n",
             "Per flow enable",
             (stat->enable_per_flow_enable ? "TRUE" : "FALSE"));
  if (stat->enable_per_flow_enable) {
    aim_printf(&uc->pvs,
               "%-20s: %d\n",
               "Per flow bit pos",
               stat->per_flow_enable_bit_position);
  }
  aim_printf(&uc->pvs, "%-20s:\n", "Stage Table Info");
  for (i = 0; i < stat->num_rmt_info; i++) {
    rmt_info = &(stat->rmt_info[i]);
    if (rmt_info) {
      if ((stage_id == 0xff) || (rmt_info->stage_id == stage_id)) {
        pipe_mgr_ucli_print_rmt_tbl_info(uc, rmt_info);
      }
    }
  }
}

static char *pipe_mgr_meter_impl2str(pipe_meter_impl_type_e meter_type) {
  switch (meter_type) {
    case PIPE_METER_TYPE_STANDARD:
      return "Standard";
    case PIPE_METER_TYPE_LPF:
      return "LPF";
    case PIPE_METER_TYPE_WRED:
      return "RED";
    default:
      return "Bad";
  }
}

static void pipe_mgr_ucli_print_meter_tbl_info(ucli_context_t *uc,
                                               rmt_dev_info_t *dev_info,
                                               unsigned handle,
                                               uint8_t stage_id) {
  unsigned int i;
  pipe_meter_tbl_info_t *meter = NULL;
  rmt_tbl_info_t *rmt_info = NULL;

  meter =
      pipe_mgr_get_meter_tbl_info(dev_info->dev_id, handle, __func__, __LINE__);

  if (!meter) {
    aim_printf(&uc->pvs, "tbl : Handle %x not found\n", handle);
    return;
  }
  aim_printf(&uc->pvs,
             "\nMeter Table Info: %s(%x)\n\n",
             meter->name ? meter->name : "null",
             meter->handle);
  aim_printf(&uc->pvs,
             "%-20s: %s\n",
             "Meter impl type",
             pipe_mgr_meter_impl2str(meter->meter_type));
  aim_printf(&uc->pvs, "%-20s: %d\n", "Size", meter->size);
  aim_printf(&uc->pvs,
             "%-20s: %s\n",
             "Reference type",
             ((meter->ref_type == PIPE_TBL_REF_TYPE_INDIRECT) ? "INDIRECT"
                                                              : "DIRECT"));
  aim_printf(&uc->pvs,
             "%-20s: %s\n",
             "Color aware capable",
             (meter->enable_color_aware ? "YES" : "NO"));
  aim_printf(&uc->pvs,
             "%-20s: %s\n",
             "Per flow enable",
             (meter->enable_per_flow_enable ? "YES" : "NO"));
  if (meter->enable_per_flow_enable) {
    aim_printf(&uc->pvs,
               "%-20s: %d\n",
               "Per flow bit pos",
               meter->per_flow_enable_bit_position);
  }
  if (meter->enable_color_aware) {
    aim_printf(&uc->pvs,
               "%-20s: %s\n",
               "Per flow color aware",
               (meter->enable_color_aware_per_flow_enable ? "YES" : "NO"));
    if (meter->enable_color_aware_per_flow_enable) {
      aim_printf(&uc->pvs,
                 "%-20s: %d\n",
                 "Color aware Bit pos",
                 meter->color_aware_per_flow_enable_address_type_bit_position);
    }
  }
  aim_printf(&uc->pvs, "%-20s:\n", "Stage Table Info");
  for (i = 0; i < meter->num_rmt_info; i++) {
    rmt_info = &(meter->rmt_info[i]);
    if (rmt_info) {
      if ((stage_id == 0xff) || (rmt_info->stage_id == stage_id)) {
        pipe_mgr_ucli_print_rmt_tbl_info(uc, rmt_info);
      }
    }
  }
}

static void pipe_mgr_ucli_print_stful_tbl_info(ucli_context_t *uc,
                                               rmt_dev_info_t *dev_info,
                                               unsigned handle,
                                               uint8_t stage_id) {
  unsigned int i;
  pipe_stful_tbl_info_t *stful = NULL;
  rmt_tbl_info_t *rmt_info = NULL;

  stful =
      pipe_mgr_get_stful_tbl_info(dev_info->dev_id, handle, __func__, __LINE__);

  if (!stful) {
    aim_printf(&uc->pvs, "tbl : Handle %x not found\n", handle);
    return;
  }
  aim_printf(&uc->pvs,
             "\nStateful Table Info: %s(%x)\n\n",
             stful->name ? stful->name : "null",
             stful->handle);
  aim_printf(&uc->pvs,
             "%-20s: %d%s\n",
             "Width",
             stful->width,
             stful->dbl_width ? "x2" : "");
  aim_printf(&uc->pvs, "%-20s: %d\n", "Size", stful->size);
  aim_printf(
      &uc->pvs, "%-20s: %d\n", "Initial Value (high)", stful->initial_val_hi);
  aim_printf(
      &uc->pvs, "%-20s: %d\n", "Initial Value (low)", stful->initial_val_lo);

  aim_printf(
      &uc->pvs,
      "%-20s: %s\n",
      "Reference type",
      (stful->ref_type == PIPE_TBL_REF_TYPE_INDIRECT) ? "INDIRECT" : "DIRECT");
  aim_printf(&uc->pvs, "%-20s: %d\n", "ALU Index", stful->alu_idx);
  aim_printf(
      &uc->pvs, "%-20s: %s\n", "Type", (stful->is_fifo ? "FIFO" : "Normal"));
  if (stful->is_fifo) {
    aim_printf(&uc->pvs, "%-20s: %d\n", "Counter Index", stful->cntr_index);
    aim_printf(&uc->pvs,
               "%-20s: %s\n",
               "CPU-Push",
               stful->fifo_can_cpu_push ? "Yes" : "No");
    aim_printf(&uc->pvs,
               "%-20s: %s\n",
               "CPU-Pop",
               stful->fifo_can_cpu_pop ? "Yes" : "No");
  }
  aim_printf(&uc->pvs, "%-20s:\n", "Stage Table Info");
  for (i = 0; i < stful->num_rmt_info; i++) {
    rmt_info = &(stful->rmt_info[i]);
    if (rmt_info) {
      if ((stage_id == 0xff) || (rmt_info->stage_id == stage_id)) {
        pipe_mgr_ucli_print_rmt_tbl_info(uc, rmt_info);
      }
    }
  }
}

static void pipe_mgr_ucli_print_adt_tbl_summary(ucli_context_t *uc,
                                                rmt_dev_info_t *dev_info) {
  int i;
  uint32_t p = 0;
  pipe_adt_tbl_info_t *adt;
  uint32_t name_len = 24;

  for (p = 0; p < dev_info->num_pipeline_profiles; p++) {
    for (i = 0; i < dev_info->profile_info[p]->tbl_info_list.num_adt_tbls;
         i++) {
      adt = &(dev_info->profile_info[p]->tbl_info_list.adt_tbl_list[i]);
      if (adt && adt->name && strlen(adt->name) > name_len) {
        name_len = strlen(adt->name);
      }
    }
  }

  char name_div[name_len + 1];
  PIPE_MGR_MEMSET(name_div, '-', name_len);
  name_div[name_len] = '\0';

  aim_printf(&uc->pvs, "\nAction-Data Tables:\n\n");
  aim_printf(&uc->pvs,
             "%-*s|%-10s|%-8s|%-5s|%-4s|%-4s\n",
             name_len,
             name_div,
             "----------",
             "--------",
             "-----",
             "----",
             "----");
  aim_printf(&uc->pvs,
             "%-*s|%-10s|%-8s|%-5s|%-4s|%-4s\n",
             name_len,
             "Name",
             "Handle",
             "Entries",
             "EntSz",
             "Stgs",
             "Prof");
  aim_printf(&uc->pvs,
             "%-*s|%-10s|%-8s|%-5s|%-4s|%-4s\n",
             name_len,
             name_div,
             "----------",
             "--------",
             "-----",
             "----",
             "----");

  for (p = 0; p < dev_info->num_pipeline_profiles; p++) {
    for (i = 0; i < dev_info->profile_info[p]->tbl_info_list.num_adt_tbls;
         i++) {
      adt = &(dev_info->profile_info[p]->tbl_info_list.adt_tbl_list[i]);
      if (adt) {
        aim_printf(&uc->pvs,
                   "%-*s|0x%-8x|%-8d|%-5d|%-4d\n",
                   name_len,
                   adt->name ? adt->name : "null",
                   adt->handle,
                   adt->size,
                   adt->num_stages,
                   dev_info->profile_info[p]->profile_id);
      }
    }
  }
}

static void pipe_mgr_ucli_print_sel_tbl_summary(ucli_context_t *uc,
                                                rmt_dev_info_t *dev_info) {
  int i;
  uint32_t p = 0;
  pipe_select_tbl_info_t *sel;
  uint32_t name_len = 24;

  for (p = 0; p < dev_info->num_pipeline_profiles; p++) {
    for (i = 0; i < dev_info->profile_info[p]->tbl_info_list.num_select_tbls;
         i++) {
      sel = &(dev_info->profile_info[p]->tbl_info_list.select_tbl_list[i]);
      if (sel && sel->name && strlen(sel->name) > name_len) {
        name_len = strlen(sel->name);
      }
    }
  }

  char name_div[name_len + 1];
  PIPE_MGR_MEMSET(name_div, '-', name_len);
  name_div[name_len] = '\0';

  aim_printf(&uc->pvs, "\nSelection Tables:\n\n");
  aim_printf(&uc->pvs,
             "%-*s|%-10s|%-8s|%-8s|%-8s|%-4s|%-4s|%-4s\n",
             name_len,
             name_div,
             "----------",
             "--------",
             "--------",
             "--------",
             "----",
             "----",
             "----");
  aim_printf(&uc->pvs,
             "%-*s|%-10s|%-8s|%-8s|%-8s|%-4s|%-4s|%-4s\n",
             name_len,
             "Name",
             "Handle",
             "Grp sz",
             "Max Grps",
             "Max Mbrs",
             "Stgs",
             "Mode",
             "Prof");
  aim_printf(&uc->pvs,
             "%-*s|%-10s|%-8s|%-8s|%-8s|%-4s|%-4s|%-4s\n",
             name_len,
             name_div,
             "----------",
             "--------",
             "--------",
             "--------",
             "----",
             "----",
             "----");

  for (p = 0; p < dev_info->num_pipeline_profiles; p++) {
    for (i = 0; i < dev_info->profile_info[p]->tbl_info_list.num_select_tbls;
         i++) {
      sel = &(dev_info->profile_info[p]->tbl_info_list.select_tbl_list[i]);
      if (sel) {
        aim_printf(&uc->pvs,
                   "%-*s|0x%-8x|%-8d|%-8d|%-8d|%-4d|%-4s|%-4d\n",
                   name_len,
                   sel->name ? sel->name : "null",
                   sel->handle,
                   sel->max_group_size,
                   sel->max_groups,
                   sel->max_mbrs,
                   sel->num_stages,
                   (sel->mode == RESILIENT) ? "R" : "F",
                   dev_info->profile_info[p]->profile_id);
      }
    }
  }
}

static void pipe_mgr_ucli_print_stat_tbl_summary(ucli_context_t *uc,
                                                 rmt_dev_info_t *dev_info) {
  int i;
  uint32_t p = 0;
  pipe_stat_tbl_info_t *stat;
  uint32_t name_len = 24;

  for (p = 0; p < dev_info->num_pipeline_profiles; p++) {
    for (i = 0; i < dev_info->profile_info[p]->tbl_info_list.num_stat_tbls;
         i++) {
      stat = &(dev_info->profile_info[p]->tbl_info_list.stat_tbl_list[i]);
      if (stat && stat->name && strlen(stat->name) > name_len) {
        name_len = strlen(stat->name);
      }
    }
  }

  char name_div[name_len + 1];
  PIPE_MGR_MEMSET(name_div, '-', name_len);
  name_div[name_len] = '\0';

  aim_printf(&uc->pvs, "\nStatistics Tables:\n\n");
  aim_printf(&uc->pvs,
             "%-*s|%-10s|%-8s|%-20s|%-20s|%-4s|%-4s\n",
             name_len,
             name_div,
             "----------",
             "--------",
             "--------------------",
             "--------------------",
             "----",
             "----");
  aim_printf(&uc->pvs,
             "%-*s|%-10s|%-8s|%-20s|%-20s|%-4s|%-4s\n",
             name_len,
             "Name",
             "Handle",
             "Size",
             "Pkt Cntr Resolution",
             "Byte Cntr Resolution",
             "Stgs",
             "Prof");
  aim_printf(&uc->pvs,
             "%-*s|%-10s|%-8s|%-20s|%-20s|%-4s|%-4s\n",
             name_len,
             name_div,
             "----------",
             "--------",
             "--------------------",
             "--------------------",
             "----",
             "----");

  for (p = 0; p < dev_info->num_pipeline_profiles; p++) {
    for (i = 0; i < dev_info->profile_info[p]->tbl_info_list.num_stat_tbls;
         i++) {
      stat = &(dev_info->profile_info[p]->tbl_info_list.stat_tbl_list[i]);
      if (stat) {
        aim_printf(&uc->pvs,
                   "%-*s|0x%-8x|%-8d|%-20d|%-20d|%-4d|%-4d\n",
                   name_len,
                   stat->name ? stat->name : "null",
                   stat->handle,
                   stat->size,
                   stat->packet_counter_resolution,
                   stat->byte_counter_resolution,
                   stat->num_rmt_info,
                   dev_info->profile_info[p]->profile_id);
      }
    }
  }
}

static void pipe_mgr_ucli_print_meter_tbl_summary(ucli_context_t *uc,
                                                  rmt_dev_info_t *dev_info) {
  int i;
  uint32_t p = 0;
  pipe_meter_tbl_info_t *meter;
  uint32_t name_len = 24;

  for (p = 0; p < dev_info->num_pipeline_profiles; p++) {
    for (i = 0; i < dev_info->profile_info[p]->tbl_info_list.num_meter_tbls;
         i++) {
      meter = &(dev_info->profile_info[p]->tbl_info_list.meter_tbl_list[i]);
      if (meter && meter->name && strlen(meter->name) > name_len) {
        name_len = strlen(meter->name);
      }
    }
  }

  char name_div[name_len + 1];
  PIPE_MGR_MEMSET(name_div, '-', name_len);
  name_div[name_len] = '\0';

  aim_printf(&uc->pvs, "\nMeter Tables:\n\n");
  aim_printf(&uc->pvs,
             "%-*s|%-10s|%-8s|%-20s|%-4s|%-4s\n",
             name_len,
             name_div,
             "----------",
             "--------",
             "--------------------",
             "----",
             "----");
  aim_printf(&uc->pvs,
             "%-*s|%-10s|%-8s|%-20s|%-4s|%-4s\n",
             name_len,
             "Name",
             "Handle",
             "Size",
             "Meter implementation",
             "Stgs",
             "Prof");
  aim_printf(&uc->pvs,
             "%-*s|%-10s|%-8s|%-20s|%-4s|%-4s\n",
             name_len,
             name_div,
             "----------",
             "--------",
             "--------------------",
             "----",
             "----");

  for (p = 0; p < dev_info->num_pipeline_profiles; p++) {
    for (i = 0; i < dev_info->profile_info[p]->tbl_info_list.num_meter_tbls;
         i++) {
      meter = &(dev_info->profile_info[p]->tbl_info_list.meter_tbl_list[i]);
      if (meter) {
        aim_printf(&uc->pvs,
                   "%-*s|0x%-8x|%-8d|%-20s|%-4d|%-4d\n",
                   name_len,
                   meter->name ? meter->name : "null",
                   meter->handle,
                   meter->size,
                   pipe_mgr_meter_impl2str(meter->meter_type),
                   meter->num_rmt_info,
                   dev_info->profile_info[p]->profile_id);
      }
    }
  }
}

static void pipe_mgr_ucli_print_stful_tbl_summary(ucli_context_t *uc,
                                                  rmt_dev_info_t *dev_info) {
  int i;
  uint32_t p = 0;
  pipe_stful_tbl_info_t *stful;
  uint32_t name_len = 24;

  for (p = 0; p < dev_info->num_pipeline_profiles; p++) {
    for (i = 0; i < dev_info->profile_info[p]->tbl_info_list.num_sful_tbls;
         i++) {
      stful = &(dev_info->profile_info[p]->tbl_info_list.stful_tbl_list[i]);
      if (stful && stful->name && strlen(stful->name) > name_len) {
        name_len = strlen(stful->name);
      }
    }
  }

  char name_div[name_len + 1];
  PIPE_MGR_MEMSET(name_div, '-', name_len);
  name_div[name_len] = '\0';

  aim_printf(&uc->pvs, "\nStateful Tables:\n\n");
  aim_printf(&uc->pvs,
             "%-*s|%-10s|%-8s|%-5s|%-10s|%-4s|%-4s\n",
             name_len,
             name_div,
             "----------",
             "--------",
             "-----",
             "----------",
             "----",
             "----");
  aim_printf(&uc->pvs,
             "%-*s|%-10s|%-8s|%-5s|%-10s|%-4s|%-4s\n",
             name_len,
             "Name",
             "Handle",
             "Size",
             "Width",
             "Type",
             "Stgs",
             "Prof");
  aim_printf(&uc->pvs,
             "%-*s|%-10s|%-8s|%-5s|%-10s|%-4s|%-4s\n",
             name_len,
             name_div,
             "----------",
             "--------",
             "-----",
             "----------",
             "----",
             "----");

  for (p = 0; p < dev_info->num_pipeline_profiles; p++) {
    for (i = 0; i < dev_info->profile_info[p]->tbl_info_list.num_sful_tbls;
         i++) {
      stful = &(dev_info->profile_info[p]->tbl_info_list.stful_tbl_list[i]);
      if (stful) {
        aim_printf(&uc->pvs,
                   "%-*s|0x%-8x|%-8d|%-3d%s|%-10s|%-4d|%-4d\n",
                   name_len,
                   stful->name ? stful->name : "null",
                   stful->handle,
                   stful->size,
                   stful->width,
                   stful->dbl_width ? "x2" : "  ",
                   stful->is_fifo ? "FIFO" : "Normal",
                   stful->num_rmt_info,
                   dev_info->profile_info[p]->profile_id);
      }
    }
  }
}

/* Routine to dump device table info */
static void pipe_mgr_ucli_dump_tbl_info(ucli_context_t *uc,
                                        bf_dev_id_t dev_id,
                                        unsigned handle,
                                        uint8_t stage_id) {
  rmt_dev_info_t *dev_info;
  extern pipe_mgr_ctx_t *pipe_mgr_ctx;

  dev_info = pipe_mgr_get_dev_info(dev_id);
  if (dev_info == NULL) {
    aim_printf(&uc->pvs, "device <%d> not found\n", dev_id);
    return;
  }

  if (handle == 0) {
    /* Dump summary of all tables */
    pipe_mgr_ucli_print_mat_tbl_summary(uc, dev_info);
    pipe_mgr_ucli_print_adt_tbl_summary(uc, dev_info);
    pipe_mgr_ucli_print_sel_tbl_summary(uc, dev_info);
    pipe_mgr_ucli_print_stat_tbl_summary(uc, dev_info);
    pipe_mgr_ucli_print_meter_tbl_summary(uc, dev_info);
    pipe_mgr_ucli_print_stful_tbl_summary(uc, dev_info);
  } else {
    /* Dump information specific to a table */
    switch (PIPE_GET_HDL_TYPE(handle)) {
      case PIPE_HDL_TYPE_MAT_TBL:
        pipe_mgr_ucli_print_mat_tbl_info(uc, dev_info, handle, stage_id);
        break;
      case PIPE_HDL_TYPE_ADT_TBL:
        pipe_mgr_ucli_print_adt_tbl_info(uc, dev_info, handle, stage_id);
        break;
      case PIPE_HDL_TYPE_SEL_TBL:
        pipe_mgr_ucli_print_sel_tbl_info(uc, dev_info, handle, stage_id);
        break;
      case PIPE_HDL_TYPE_STAT_TBL:
        pipe_mgr_ucli_print_stat_tbl_info(uc, dev_info, handle, stage_id);
        break;
      case PIPE_HDL_TYPE_METER_TBL:
        pipe_mgr_ucli_print_meter_tbl_info(uc, dev_info, handle, stage_id);
        break;
      case PIPE_HDL_TYPE_STFUL_TBL:
        pipe_mgr_ucli_print_stful_tbl_info(uc, dev_info, handle, stage_id);
        break;
      default:
        aim_printf(&uc->pvs, "tbl: Invalid handle %x\n", handle);
    }
  }
}

PIPE_MGR_CLI_CMD_DECLARE(drv_state) {
  UCLI_COMMAND_INFO(uc, "drv-state", 0, "Dump the driver interface state.");

  pipe_mgr_drv_dump_state();

  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(log_ilist) {
  PIPE_MGR_CLI_PROLOGUE("log-ilist",
                        " Start/stop logging of ilist contents",
                        "-d <dev> -e <0/1 to stop/start> -v");

  bf_dev_id_t dev_id = -1;
  bool got_dev = false;
  int enable = 0;
  bool got_enable = false;
  bool very_verbose = false;

  int c;
  while ((c = getopt(argc, argv, "d:e:v")) != -1) {
    switch (c) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev_id = strtoul(optarg, NULL, 0);
        got_dev = true;
        break;
      case 'e':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        enable = !!strtoul(optarg, NULL, 0);
        got_enable = true;
        break;
      case 'v':
        very_verbose = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }
  if (!got_dev || !got_enable) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  pipe_mgr_set_log_ilist(dev_id, enable ? (very_verbose ? 2 : 1) : 0);
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(decode_ilist) {
  PIPE_MGR_CLI_PROLOGUE(
      "decode-ilist",
      " Decode the contents of an instruction list",
      "-d <dev> -a <address> -l <length in bytes> [-p <log pipe>]");

  bf_dev_id_t dev_id = -1;
  bool got_dev = false;
  uint64_t addr_int = 0;
  uint32_t *addr = NULL;
  bool got_addr = false;
  int len = 0;
  bool got_len = false;
  bf_dev_pipe_t log_pipe = 0;

  int c;
  while ((c = getopt(argc, argv, "d:a:l:p:")) != -1) {
    switch (c) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev_id = strtoul(optarg, NULL, 0);
        got_dev = true;
        break;
      case 'a':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        addr_int = strtoul(optarg, NULL, 0);
        addr = (uint32_t *)(uintptr_t)addr_int;
        got_addr = true;
        break;
      case 'l':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        len = strtoul(optarg, NULL, 0);
        got_len = true;
        break;
      case 'p':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        log_pipe = strtoul(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }
  if (!got_dev || !got_addr || !got_len) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  char *msg_log = NULL;
  int msg_log_sz = 2048;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  pipe_status_t msg_log_sts = PIPE_SUCCESS;
  do {
    msg_log = PIPE_MGR_MALLOC(msg_log_sz);
    msg_log_sts = pipe_mgr_decode_ilist(
        dev_info, log_pipe, addr, len, msg_log, msg_log_sz);
    if (PIPE_SUCCESS == msg_log_sts) {
      aim_printf(&uc->pvs, "%s\n", msg_log);
    }
    if (msg_log) PIPE_MGR_FREE(msg_log);
    msg_log_sz *= 2;
  } while (msg_log_sts == PIPE_NO_SYS_RESOURCES);

  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(dev) {
  UCLI_COMMAND_INFO(uc,
                    "dev",
                    -1,
                    "Dump device info."
                    " Usage: dev [-d <dev_id>]");
  int c, dflag = 0;
  char *d_arg = NULL;
  unsigned dev_id = 0xff;
  static char usage[] = "Usage: dev [-d <dev_id>]\n";
  int argc;
  char *const *argv;
  int arg_start = 0;
  size_t i;

  for (i = 0; i < sizeof(uc->pargs[0].args__) / sizeof(uc->pargs[0].args__[0]);
       ++i) {
    if (!strncmp(uc->pargs[0].args__[i], "dev", strlen("dev"))) {
      arg_start = i;
      break;
    }
  }
  optind = 0; /* reset optind value */
  argc = (uc->pargs->count + 1);
  argv = (char *const *)&(uc->pargs->args__[arg_start]);

  while ((c = getopt(argc, argv, "d:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        d_arg = optarg;
        if (!d_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (dflag == 1) {
    dev_id = strtoul(d_arg, NULL, 0);
    if ((dev_id == 0x3FF) || (dev_id > (PIPE_MGR_NUM_DEVICES - 1))) {
      aim_printf(&uc->pvs, "dev: Invalid dev_id %s\n", d_arg);
      return UCLI_STATUS_OK;
    }
  }
  pipe_mgr_ucli_dump_dev_info(uc, dev_id);

  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(pipe) {
  int c, dflag = 0, pflag = 0;
  char *d_arg = NULL, *p_arg = NULL;
  unsigned dev_id = -1, pipe_id = 0xff;

  PIPE_MGR_CLI_PROLOGUE(
      "pipe", " Dump pipeline info.", "-d <dev_id> [-p <pipe_id>]");

  while ((c = getopt(argc, argv, "d:p:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        d_arg = optarg;
        if (!d_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'p':
        pflag = 1;
        p_arg = optarg;
        if (!p_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (dflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else if (dflag == 1) {
    dev_id = strtoul(d_arg, NULL, 0);
    if ((dev_id == 0x3FF) || (dev_id > (PIPE_MGR_NUM_DEVICES - 1))) {
      aim_printf(&uc->pvs, "pipe: Invalid dev_id %s\n", d_arg);
      return UCLI_STATUS_OK;
    }
  }

  if (pflag == 1) {
    pipe_id = strtoul(p_arg, NULL, 0);
    if (pipe_id == 0x3FF) {
      aim_printf(&uc->pvs, "pipe: Invalid pipe_id %s\n", p_arg);
      return UCLI_STATUS_OK;
    }
  }

  pipe_mgr_ucli_dump_pipe_info(uc, dev_id, pipe_id);
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(profile) {
  int c, dflag = 0;
  char *d_arg = NULL;
  unsigned dev_id = 0xff;

  PIPE_MGR_CLI_PROLOGUE(
      "profile", " Dump pipeline profile info.", "-d <dev_id>");

  while ((c = getopt(argc, argv, "d:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        d_arg = optarg;
        if (!d_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (dflag == 1) {
    dev_id = strtoul(d_arg, NULL, 0);
    if ((dev_id == 0x3FF) || (dev_id > (PIPE_MGR_NUM_DEVICES - 1))) {
      aim_printf(&uc->pvs, "dev: Invalid dev_id %s\n", d_arg);
      return UCLI_STATUS_OK;
    }
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  pipe_mgr_ucli_dump_profile_info(uc, dev_id);

  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(tbl) {
  int c, dflag, hflag, sflag;
  char *d_arg, *h_arg, *s_arg;
  unsigned dev_id = 0, handle = 0, stage_id = 0xff;

  PIPE_MGR_CLI_PROLOGUE(
      "tbl", "Dump table info.", "[-d <dev_id>] [-h <hdl> [-s <stg>]]");

  dflag = hflag = sflag = 0;
  d_arg = h_arg = s_arg = NULL;

  while ((c = getopt(argc, argv, "d:h:s:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        d_arg = optarg;
        if (!d_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'h':
        hflag = 1;
        h_arg = optarg;
        if (!h_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 's':
        sflag = 1;
        s_arg = optarg;
        if (!s_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (dflag == 1) {
    dev_id = strtoul(d_arg, NULL, 0);
    if ((dev_id == 0x3FF) || (dev_id > (PIPE_MGR_NUM_DEVICES - 1))) {
      aim_printf(&uc->pvs, "tbl: Invalid dev_id %s\n", d_arg);
      return UCLI_STATUS_OK;
    }
  }

  if (hflag == 1) {
    handle = strtoul(h_arg, NULL, 0);
    if (sflag == 1) {
      stage_id = strtoul(s_arg, NULL, 0);
    } else {
      stage_id = 0xff;
    }
  } else if (sflag == 1) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  pipe_mgr_ucli_dump_tbl_info(uc, dev_id, handle, stage_id);
  return UCLI_STATUS_OK;
}

struct dup_ent_hash_tbl_dump_ctx {
  bf_dev_id_t dev_id;
  profile_id_t profile_id;
  pipe_mat_tbl_hdl_t tbl_hdl;
  ucli_context_t *uc;
};
static void dup_ent_hash_tbl_dump(void *arg, void *obj) {
  if (!arg) return;
  struct dup_ent_hash_tbl_dump_ctx *ctx = arg;

  if (!obj) {
    aim_printf(&ctx->uc->pvs, "NULL object\n");
    return;
  }
  pipe_mgr_mat_key_htbl_node_t *ent = bf_hashtbl_get_cmp_data(obj);
  if (!ent) {
    aim_printf(&ctx->uc->pvs, "NULL entry\n");
    return;
  }
  if (!ent->match_spec) {
    aim_printf(&ctx->uc->pvs, "Entry %u has no match spec\n", ent->mat_ent_hdl);
    return;
  }

  pipe_status_t sts = PIPE_SUCCESS;
  const size_t buf_sz = 8192;
  size_t buf_used = 0;
  char *buf = PIPE_MGR_CALLOC(buf_sz, 1);
  if (!buf) {
    sts = PIPE_NO_SYS_RESOURCES;
  }
  if (sts == PIPE_SUCCESS) {
    sts = pipe_mgr_entry_format_print_match_spec(ctx->dev_id,
                                                 ctx->profile_id,
                                                 ctx->tbl_hdl,
                                                 ent->match_spec,
                                                 buf,
                                                 buf_sz,
                                                 &buf_used);
  }
  if (sts != PIPE_SUCCESS) {
    aim_printf(&ctx->uc->pvs,
               "Entry %u cannot display match spec: %s\n",
               ent->mat_ent_hdl,
               pipe_str_err(sts));
  } else {
    aim_printf(&ctx->uc->pvs, "Entry %u\n%s\n", ent->mat_ent_hdl, buf);
  }

  if (buf) PIPE_MGR_FREE(buf);
}

PIPE_MGR_CLI_CMD_DECLARE(dup_ent_hash_dump) {
  PIPE_MGR_CLI_PROLOGUE("dup-ent-hash-dump",
                        "Dump hash table for duplicate entry checks.",
                        "-d <dev_id> -h <table handle> -p <pipe-id>");
  int c;
  bf_dev_id_t dev_id = 0;
  pipe_mat_tbl_hdl_t tbl_hdl = 0;
  bf_dev_pipe_t pipe = BF_DEV_PIPE_ALL;

  while ((c = getopt(argc, argv, "d:h:p:")) != -1) {
    switch (c) {
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
        break;
      case 'p':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        pipe = strtoul(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    aim_printf(&uc->pvs, "Cannot find device %d\n", dev_id);
    return UCLI_STATUS_OK;
  }

  pipe_mat_tbl_info_t *tbl_info =
      pipe_mgr_get_tbl_info(dev_id, tbl_hdl, __func__, __LINE__);
  if (!tbl_info) {
    aim_printf(&uc->pvs, "Cannot find table 0x%x\n", tbl_hdl);
    return UCLI_STATUS_OK;
  }

  if (pipe != BF_DEV_PIPE_ALL && pipe >= dev_info->num_active_pipes) {
    aim_printf(&uc->pvs, "Pipe %d not valid for device %d\n", pipe, dev_id);
    return UCLI_STATUS_OK;
  }

  if (!tbl_info->key_htbl) {
    aim_printf(&uc->pvs,
               "Table %s (0x%x) does not have a key hash table\n",
               tbl_info->name,
               tbl_info->handle);
    return UCLI_STATUS_OK;
  }

  if (pipe == BF_DEV_PIPE_ALL) {
    pipe = 0;
  }
  bf_hashtable_t *htbl = tbl_info->key_htbl[pipe];
  struct dup_ent_hash_tbl_dump_ctx ctx;
  ctx.dev_id = dev_id;
  ctx.profile_id = tbl_info->profile_id;
  ctx.tbl_hdl = tbl_hdl;
  ctx.uc = uc;
  bf_hashtbl_foreach_fn(htbl, dup_ent_hash_tbl_dump, &ctx);
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(entry_count) {
  PIPE_MGR_CLI_PROLOGUE(
      "entry_count", "Dump table entry count.", "-d <dev_id> -h <hdl>");
  int c, dflag, hflag;
  char *d_arg, *h_arg;
  unsigned dev_id = -1, handle = 0;

  dflag = hflag = 0;
  d_arg = h_arg = NULL;

  while ((c = getopt(argc, argv, "d:h:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        d_arg = optarg;
        if (!d_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'h':
        hflag = 1;
        h_arg = optarg;
        if (!h_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (dflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else if (dflag == 1) {
    dev_id = strtoul(d_arg, NULL, 0);
    if ((dev_id == 0x3FF) || (dev_id > (PIPE_MGR_NUM_DEVICES - 1))) {
      aim_printf(&uc->pvs, "entry_count: Invalid dev_id %s\n", d_arg);
      return UCLI_STATUS_OK;
    }
  }

  if (hflag == 1) {
    handle = strtoul(h_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  uint32_t count = 0;
  dev_target_t dev_tgt = {.device_id = dev_id, .dev_pipe_id = BF_DEV_PIPE_ALL};
  if (pipe_mgr_tbl_get_entry_count(dev_tgt, handle, true, &count) ==
      PIPE_SUCCESS) {
    aim_printf(&uc->pvs, "table 0x%x: %u entries occupied\n", handle, count);
  } else {
    aim_printf(&uc->pvs, "No information found for the table specified\n");
  }
  return UCLI_STATUS_OK;
}

static pipe_status_t pipe_mgr_ucli_invalidate_tbl_index(ucli_context_t *uc,
                                                        unsigned dev_id,
                                                        unsigned pipe,
                                                        pipe_tbl_hdl_t tbl_hdl,
                                                        uint32_t index) {
  pipe_status_t status;
  pipe_sess_hdl_t sess_hdl;
  dev_target_t dev_tgt;

  if (PIPE_SUCCESS != (status = pipe_mgr_client_init(&sess_hdl))) {
    aim_printf(&uc->pvs,
               "Client init failed with pipe_mgr with error %s\n",
               pipe_str_err(status));
    return status;
  }

  if (PIPE_SUCCESS != (status = pipe_mgr_api_enter(sess_hdl))) {
    pipe_mgr_client_cleanup(sess_hdl);
    return status;
  }

  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = pipe;

  if (PIPE_SUCCESS != (status = pipe_mgr_verify_pipe_tbl_access(
                           sess_hdl, dev_tgt, tbl_hdl, true))) {
    pipe_mgr_api_exit(sess_hdl);
    pipe_mgr_client_cleanup(sess_hdl);
    return status;
  }

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return PIPE_INVALID_ARG;
  for (bf_dev_pipe_t log_pipe = 0; log_pipe < dev_info->num_active_pipes;
       ++log_pipe) {
    /* Skip pipes we are not working on. */
    if (pipe != BF_DEV_PIPE_ALL && pipe != log_pipe) continue;

    status =
        pipe_mgr_invalidate_tbl_idx(sess_hdl, dev_id, log_pipe, tbl_hdl, index);
    if (status != PIPE_SUCCESS) {
      aim_printf(&uc->pvs,
                 "Failed to invalidate entry at index %d, rc %s\n",
                 index,
                 pipe_str_err(status));
    }
  }

  ;
  if (PIPE_SUCCESS !=
      (status = pipe_mgr_drv_ilist_push(&sess_hdl, NULL, NULL))) {
    LOG_ERROR("%s:%d Unable to push ilist for handle 0x%x, device id %d",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
  }

  pipe_mgr_sm_release(sess_hdl);
  pipe_mgr_api_exit(sess_hdl);
  pipe_mgr_client_cleanup(sess_hdl);
  return status;
}

static pipe_status_t pipe_mgr_ucli_dump_hw_entry(ucli_context_t *uc,
                                                 unsigned dev_id,
                                                 unsigned pipe,
                                                 pipe_tbl_hdl_t tbl_hdl,
                                                 uint32_t *index,
                                                 bool err_corr,
                                                 bool print_info) {
  pipe_status_t status;

  bool is_def = false;
  uint32_t index_in = *index;
  pipe_sess_hdl_t sess_hdl;

  dev_target_t dev_tgt;
  pipe_tbl_match_spec_t m_spec;
  pipe_action_spec_t act_spec;
  pipe_act_fn_hdl_t act_fn_hdl;

  PIPE_MGR_MEMSET(&m_spec, 0, sizeof(pipe_tbl_match_spec_t));
  PIPE_MGR_MEMSET(&act_spec, 0, sizeof(pipe_action_spec_t));
  PIPE_MGR_MEMSET(&dev_tgt, 0, sizeof(dev_target_t));
  pipe_ent_hdl_t entry_hdl = 0xdeadbeef;

  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = pipe;

  status = pipe_mgr_client_init(&sess_hdl);

  if (status != PIPE_SUCCESS) {
    aim_printf(&uc->pvs,
               "Client init failed with pipe_mgr with error %s\n",
               pipe_str_err(status));
    return status;
  }

  if (print_info) {
    aim_printf(&uc->pvs, "Entry Index : %d\n", index_in);
  }

  status = pipe_mgr_tbl_get_entry_from_index(sess_hdl,
                                             dev_tgt,
                                             tbl_hdl,
                                             *index,
                                             err_corr,
                                             &m_spec,
                                             &act_spec,
                                             &act_fn_hdl,
                                             &entry_hdl,
                                             &is_def,
                                             index);

  if (status != PIPE_SUCCESS) {
    if (status == PIPE_INTERNAL_ERROR) {
      aim_printf(&uc->pvs,
                 "tbl 0x%x: Found HW inconsistency at index %d\n",
                 tbl_hdl,
                 index_in);
    } else {
      aim_printf(&uc->pvs,
                 "tbl 0x%x: error for index %d, rc %s\n",
                 tbl_hdl,
                 index_in,
                 pipe_str_err(status));
      goto done;
    }
  }

  if (print_info && status == PIPE_SUCCESS) {
    pipe_mat_tbl_info_t *mat_tbl_info =
        pipe_mgr_get_tbl_info(dev_tgt.device_id, tbl_hdl, __func__, __LINE__);
    if (mat_tbl_info == NULL) {
      LOG_ERROR(
          "Error in finding the table info for tbl 0x%x"
          " device id %d",
          tbl_hdl,
          dev_tgt.device_id);
      pipe_mgr_client_cleanup(sess_hdl);
      goto done;
    }

    aim_printf(&uc->pvs, "Entry Handle : %d (0x%x)\n", entry_hdl, entry_hdl);

    char buf[1500];
    size_t bytes_written;
    pipe_mgr_entry_format_print_match_spec(dev_tgt.device_id,
                                           mat_tbl_info->profile_id,
                                           tbl_hdl,
                                           &m_spec,
                                           buf,
                                           sizeof(buf),
                                           &bytes_written);
    aim_printf(&uc->pvs, "%s", buf);

    if (IS_ACTION_SPEC_ACT_DATA(&act_spec)) {
      aim_printf(&uc->pvs, "  Action Spec:\n");
      pipe_mgr_entry_format_print_action_spec(dev_tgt.device_id,
                                              mat_tbl_info->profile_id,
                                              &act_spec.act_data,
                                              act_fn_hdl,
                                              buf,
                                              sizeof(buf),
                                              &bytes_written);
      aim_printf(&uc->pvs, "%s", buf);
    } else if (IS_ACTION_SPEC_ACT_DATA_HDL(&act_spec)) {
      aim_printf(&uc->pvs, "  Action entry handle 0x%x", act_spec.adt_ent_hdl);
    } else if (IS_ACTION_SPEC_SEL_GRP(&act_spec)) {
      aim_printf(
          &uc->pvs, "  Selector group handle 0x%x", act_spec.sel_grp_hdl);
    }
    resource_trace(dev_tgt.device_id,
                   act_spec.resources,
                   act_spec.resource_count,
                   buf,
                   sizeof(buf));
    aim_printf(&uc->pvs, "   %s\n", buf);

    aim_printf(&uc->pvs, "\n\n");
  }

done:
  if (m_spec.match_mask_bits) PIPE_MGR_FREE(m_spec.match_mask_bits);
  if (m_spec.match_value_bits) PIPE_MGR_FREE(m_spec.match_value_bits);
  if (act_spec.act_data.action_data_bits)
    PIPE_MGR_FREE(act_spec.act_data.action_data_bits);

  pipe_mgr_client_cleanup(sess_hdl);
  return status;
}

PIPE_MGR_CLI_CMD_DECLARE(tbl_hw_verify) {
  int c, dflag, pflag, hflag, iflag, cflag;
  char *d_arg, *p_arg, *h_arg, *i_arg, *c_arg;
  unsigned dev_id = 0, pipe = 0, handle = 0;
  bool err_corr = false;

  PIPE_MGR_CLI_PROLOGUE(
      "tbl_hw_verify",
      "Verify all table hw entries.",
      "-d <dev_id> -p <pipe> -h <hdl> [-c <err_corr 0/1 disable/enable]");

  dflag = pflag = hflag = iflag = cflag = 0;
  d_arg = p_arg = h_arg = i_arg = c_arg = NULL;

  while ((c = getopt(argc, argv, "d:p:h:c:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        d_arg = optarg;
        if (!d_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'p':
        pflag = 1;
        p_arg = optarg;
        if (!p_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'h':
        hflag = 1;
        h_arg = optarg;
        if (!h_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'c':
        cflag = 1;
        c_arg = optarg;
        if (!c_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (hflag != 1 || pflag != 1 || dflag != 1) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  dev_id = strtoul(d_arg, NULL, 0);
  if ((dev_id == 0x3FF) || (dev_id > (PIPE_MGR_NUM_DEVICES - 1))) {
    aim_printf(&uc->pvs, "tbl: Invalid dev_id %s\n", d_arg);
    return UCLI_STATUS_OK;
  }

  if (cflag == 1) {
    err_corr = (bool)strtoul(c_arg, NULL, 0);
  }

  pipe = strtoul(p_arg, NULL, 0);
  handle = strtoul(h_arg, NULL, 0);
  if (pipe != BF_DEV_PIPE_ALL && pipe >= BF_PIPE_COUNT) {
    aim_printf(&uc->pvs, "Pipe id should be less then %d\n", BF_PIPE_COUNT);
    return UCLI_STATUS_OK;
  }
  uint32_t last_index = 0, index = 0;
  dev_target_t dev_tgt = {0};
  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = pipe;
  pipe_status_t rc = pipe_mgr_tbl_get_last_index(dev_tgt, handle, &last_index);
  if (rc) {
    aim_printf(&uc->pvs, "tbl 0x%x: cannot get last index.\n", handle);
    return UCLI_STATUS_OK;
  }
  while (index <= last_index && rc == PIPE_SUCCESS) {
    rc = pipe_mgr_ucli_dump_hw_entry(
        uc, dev_id, pipe, handle, &index, err_corr, false /*print entry*/);
  }

  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(tbl_hw_ent_get) {
  int c, dflag, pflag, hflag, iflag, cflag;
  char *d_arg, *p_arg, *h_arg, *i_arg, *c_arg;
  unsigned dev_id = 0, pipe = 0, handle = 0, idx = 0;
  bool err_corr = false;

  PIPE_MGR_CLI_PROLOGUE("tbl_hw_ent_get",
                        "Dump hw entry info.",
                        "-d <dev_id> -p <pipe> -h <hdl> -i <idx> [-c <err_corr "
                        "0/1 disable/enable>]");

  dflag = pflag = hflag = iflag = cflag = 0;
  d_arg = p_arg = h_arg = i_arg = c_arg = NULL;

  while ((c = getopt(argc, argv, "d:p:h:i:c:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        d_arg = optarg;
        if (!d_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'p':
        pflag = 1;
        p_arg = optarg;
        if (!p_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'h':
        hflag = 1;
        h_arg = optarg;
        if (!h_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'i':
        iflag = 1;
        i_arg = optarg;
        if (!i_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'c':
        cflag = 1;
        c_arg = optarg;
        if (!c_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (iflag != 1 || hflag != 1 || pflag != 1 || dflag != 1) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  dev_id = strtoul(d_arg, NULL, 0);
  if ((dev_id == 0x3FF) || (dev_id > (PIPE_MGR_NUM_DEVICES - 1))) {
    aim_printf(&uc->pvs, "tbl: Invalid dev_id %s\n", d_arg);
    return UCLI_STATUS_OK;
  }

  if (cflag == 1) {
    err_corr = (bool)strtoul(c_arg, NULL, 0);
  }

  pipe = strtoul(p_arg, NULL, 0);
  handle = strtoul(h_arg, NULL, 0);
  idx = strtoul(i_arg, NULL, 0);

  if (pipe != BF_DEV_PIPE_ALL && pipe >= BF_PIPE_COUNT) {
    aim_printf(&uc->pvs, "Pipe id should be less then %d\n", BF_PIPE_COUNT);
    return UCLI_STATUS_OK;
  }
  pipe_mgr_ucli_dump_hw_entry(
      uc, dev_id, pipe, handle, &idx, err_corr, true /*print entry*/);
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(invalidate_table_entry) {
  int c, dflag, pflag, hflag, iflag;
  char *d_arg, *p_arg, *h_arg, *i_arg;
  unsigned dev_id = 0, pipe = 0, handle = 0, idx = 0;

  PIPE_MGR_CLI_PROLOGUE("inval_tbl_idx",
                        "Invalidate table index.",
                        "-d <dev_id> -p <pipe> -h <hdl> -i <idx>]]");

  dflag = pflag = hflag = iflag = 0;
  d_arg = p_arg = h_arg = i_arg = NULL;

  while ((c = getopt(argc, argv, "d:p:h:i:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        d_arg = optarg;
        if (!d_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'p':
        pflag = 1;
        p_arg = optarg;
        if (!p_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'h':
        hflag = 1;
        h_arg = optarg;
        if (!h_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'i':
        iflag = 1;
        i_arg = optarg;
        if (!i_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (iflag != 1 || hflag != 1 || pflag != 1 || dflag != 1) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  dev_id = strtoul(d_arg, NULL, 0);
  if ((dev_id == 0x3FF) || (dev_id > (PIPE_MGR_NUM_DEVICES - 1))) {
    aim_printf(&uc->pvs, "tbl: Invalid dev_id %s\n", d_arg);
    return UCLI_STATUS_OK;
  }

  pipe = strtoul(p_arg, NULL, 0);
  handle = strtoul(h_arg, NULL, 0);
  idx = strtoul(i_arg, NULL, 0);
  if (pipe != BF_DEV_PIPE_ALL && pipe >= BF_PIPE_COUNT) {
    aim_printf(&uc->pvs, "Pipe id should be less then %d\n", BF_PIPE_COUNT);
    return UCLI_STATUS_OK;
  }
  pipe_mgr_ucli_invalidate_tbl_index(uc, dev_id, pipe, handle, idx);
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(ipv4_route_tcam_add) {
  pipe_status_t status = PIPE_SUCCESS;
  rmt_dev_info_t *dev_info = NULL;
  pipe_mat_tbl_info_t *mat_tbl_info = NULL;
  pipe_mat_tbl_hdl_t tbl_hdl;
  pipe_action_spec_t act_spec;
  pipe_tbl_match_spec_t match_spec;
  uint16_t act_data_bits;
  uint32_t ip_addr;
  uint32_t mask;
  uint32_t i;
  uint32_t p = 0;
  pipe_mat_ent_hdl_t ent_hdl;
  pipe_sess_hdl_t sess_hdl;
  dev_target_t dev_tgt;

  UCLI_COMMAND_INFO(
      uc, "ipv4-route-tcam-add", 0, " Add an IPv4 route to the route TCAM");

  PIPE_MGR_MEMSET(&dev_tgt, 0, sizeof(dev_target_t));
  dev_tgt.device_id = 0;
  dev_tgt.dev_pipe_id = BF_DEV_PIPE_ALL;

  dev_info = pipe_mgr_get_dev_info(0);

  if (dev_info == NULL) {
    return UCLI_STATUS_OK;
  }
  bool found = false;
  /* Find out the tcam table in a device and add an entry */
  for (p = 0; p < dev_info->num_pipeline_profiles; p++) {
    for (i = 0; i < dev_info->profile_info[p]->tbl_info_list.num_mat_tbls;
         i++) {
      mat_tbl_info = &dev_info->profile_info[p]->tbl_info_list.mat_tbl_list[0];

      if (strcmp(mat_tbl_info->name, "ipv4_routing__action__") == 0 &&
          mat_tbl_info->match_type == TERNARY_MATCH) {
        found = true;
        break;
      } else {
        mat_tbl_info = NULL;
      }
    }
    if (found) {
      break;
    }
  }

  if (mat_tbl_info == NULL) {
    aim_printf(&uc->pvs, "No ipv4 route tcam found in the device");
    return UCLI_STATUS_OK;
  }

  tbl_hdl = mat_tbl_info->handle;

  status = pipe_mgr_client_init(&sess_hdl);

  if (status != PIPE_SUCCESS) {
    aim_printf(&uc->pvs,
               "Client init failed with pipe_mgr with error %s\n",
               pipe_str_err(status));
    return UCLI_STATUS_OK;
  }

  PIPE_MGR_MEMSET(&match_spec, 0, sizeof(pipe_tbl_match_spec_t));
  PIPE_MGR_MEMSET(&act_spec, 0, sizeof(pipe_action_spec_t));

  match_spec.num_valid_match_bits = 32;
  /* Add an IP route of 10.10.1.1 */
  ip_addr = 0x0a0a001;
  PIPE_MGR_MEMCPY(match_spec.match_value_bits, &ip_addr, 4);
  /* /32 */
  mask = 0xffffffff;
  PIPE_MGR_MEMCPY(match_spec.match_mask_bits, &mask, 4);

  /* 16 bit next-hop */
  act_spec.act_data.num_valid_action_data_bits = 16;
  act_data_bits = 0xabcd;
  PIPE_MGR_MEMCPY(act_spec.act_data.action_data_bits, &act_data_bits, 2);

  status = pipe_mgr_mat_ent_add(
      sess_hdl, dev_tgt, tbl_hdl, &match_spec, 0, &act_spec, 0, 0, &ent_hdl);

  if (status != PIPE_SUCCESS) {
    aim_printf(&uc->pvs,
               "Error in adding a ipv4 route into the tcam, error %s",
               pipe_str_err(status));
    return UCLI_STATUS_OK;
  }

  aim_printf(&uc->pvs, "IPv4 route successfully added into the TCAM");
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(show_mat_tbl_entry) {
  int c, dflag, hflag, eflag, pflag;
  char *d_arg, *h_arg, *e_arg, *p_arg;
  bf_dev_id_t dev_id = 0;
  uint8_t log_pipe = PIPE_INVALID_VAL;
  uint32_t tbl_hdl = 0, ent_hdl = 0;
  uint32_t subindex = 0;
  char str[PIPE_MGR_HW_DUMP_STR_LEN];

  PIPE_MGR_CLI_PROLOGUE("show-mat-tbl-entry",
                        " Dump Match table HW info by entry handle.",
                        "-d <dev_id> -h <tbl_hdl> -e <entry_hdl> [-p "
                        "<logical-pipe>] [-i <subindex>]");

  dflag = hflag = eflag = pflag = 0;
  d_arg = h_arg = e_arg = p_arg = NULL;
  str[0] = '\0';

  while ((c = getopt(argc, argv, "d:h:e:p:i:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        d_arg = optarg;
        if (!d_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'h':
        hflag = 1;
        h_arg = optarg;
        if (!h_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'e':
        eflag = 1;
        e_arg = optarg;
        if (!e_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'p':
        pflag = 1;
        p_arg = optarg;
        if (!p_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
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

  if (dflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else if (dflag == 1) {
    dev_id = strtoul(d_arg, NULL, 0);
    if (dev_id > (PIPE_MGR_NUM_DEVICES - 1)) {
      aim_printf(&uc->pvs, "tbl: Invalid dev_id %s\n", d_arg);
      return UCLI_STATUS_OK;
    }
  }

  if (hflag == 1) {
    tbl_hdl = strtoul(h_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (eflag == 1) {
    ent_hdl = strtoul(e_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (pflag == 1) {
    log_pipe = strtoul(p_arg, NULL, 0);
  }

  pipe_mgr_print_mat_hw_tbl_info_w_lock(
      dev_id, tbl_hdl, ent_hdl, log_pipe, subindex, str, sizeof(str));

  aim_printf(&uc->pvs, "\n%s\n", str);
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(show_act_tbl_entry) {
  int c, dflag, hflag, eflag, pflag, sflag, fflag;
  char *d_arg, *h_arg, *e_arg, *p_arg, *s_arg, *f_arg;
  bf_dev_id_t dev_id = 0;
  uint8_t log_pipe = PIPE_INVALID_VAL, stage = STAGE_INVALID_VAL;
  uint32_t tbl_hdl = 0, ent_hdl = 0;
  uint32_t act_fn_hdl = 0;
  char str[PIPE_MGR_HW_DUMP_STR_LEN];

  PIPE_MGR_CLI_PROLOGUE("show-act-tbl-entry",
                        " Dump Action table HW info by entry handle.",
                        "-d <dev_id> -h <tbl_hdl> -e <entry_hdl> -f "
                        "<act_fn_hdl> -p <logical-pipe> -s <stage>");

  dflag = hflag = eflag = pflag = sflag = fflag = 0;
  d_arg = h_arg = e_arg = p_arg = s_arg = f_arg = NULL;
  str[0] = '\0';

  while ((c = getopt(argc, argv, "d:h:e:p:s:f:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        d_arg = optarg;
        if (!d_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'h':
        hflag = 1;
        h_arg = optarg;
        if (!h_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'e':
        eflag = 1;
        e_arg = optarg;
        if (!e_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'p':
        pflag = 1;
        p_arg = optarg;
        if (!p_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 's':
        sflag = 1;
        s_arg = optarg;
        if (!s_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'f':
        fflag = 1;
        f_arg = optarg;
        if (!f_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (dflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else if (dflag == 1) {
    dev_id = strtoul(d_arg, NULL, 0);
    if (dev_id > (PIPE_MGR_NUM_DEVICES - 1)) {
      aim_printf(&uc->pvs, "tbl: Invalid dev_id %s\n", d_arg);
      return UCLI_STATUS_OK;
    }
  }

  if (hflag == 1) {
    tbl_hdl = strtoul(h_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (eflag == 1) {
    ent_hdl = strtoul(e_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (fflag == 1) {
    act_fn_hdl = strtoul(f_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (pflag == 1) {
    unsigned long log_pipe_ul = strtoul(p_arg, NULL, 0);
    if (log_pipe_ul >= PIPE_MGR_MAX_PIPES) {
      aim_printf(&uc->pvs, "Invalid log pipe %lu", log_pipe_ul);
      return UCLI_STATUS_OK;
    }
    log_pipe = (bf_dev_pipe_t)log_pipe_ul;
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (sflag == 1) {
    stage = strtoul(s_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  pipe_mgr_print_act_hw_tbl_info_w_lock(
      dev_id, tbl_hdl, ent_hdl, act_fn_hdl, log_pipe, stage, str, sizeof(str));

  aim_printf(&uc->pvs, "\n%s\n", str);
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(show_act_tbl_entry_by_idx) {
  int c, dflag, hflag, iflag, pflag, sflag, fflag;
  char *d_arg, *h_arg, *i_arg, *p_arg, *s_arg, *f_arg;
  bf_dev_id_t dev_id = 0;
  uint8_t log_pipe = PIPE_INVALID_VAL, stage = STAGE_INVALID_VAL;
  uint32_t tbl_hdl = 0, entry_idx = 0;
  uint32_t act_fn_hdl = 0;
  char str[PIPE_MGR_HW_DUMP_STR_LEN];

  PIPE_MGR_CLI_PROLOGUE("show-act-tbl-entry-idx",
                        " Dump Action table HW info by entry index.",
                        "-d <dev_id> -h <tbl_hdl> -i <entry_idx> -f "
                        "<act_fn_hdl> -p <logical-pipe> -s <stage>");

  dflag = hflag = iflag = pflag = sflag = fflag = 0;
  d_arg = h_arg = i_arg = p_arg = s_arg = f_arg = NULL;
  str[0] = '\0';

  while ((c = getopt(argc, argv, "d:h:i:p:s:f:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        d_arg = optarg;
        if (!d_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'h':
        hflag = 1;
        h_arg = optarg;
        if (!h_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'i':
        iflag = 1;
        i_arg = optarg;
        if (!i_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'p':
        pflag = 1;
        p_arg = optarg;
        if (!p_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 's':
        sflag = 1;
        s_arg = optarg;
        if (!s_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'f':
        fflag = 1;
        f_arg = optarg;
        if (!f_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (dflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else if (dflag == 1) {
    dev_id = strtoul(d_arg, NULL, 0);
    if (dev_id > (PIPE_MGR_NUM_DEVICES - 1)) {
      aim_printf(&uc->pvs, "tbl: Invalid dev_id %s\n", d_arg);
      return UCLI_STATUS_OK;
    }
  }

  if (hflag == 1) {
    tbl_hdl = strtoul(h_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (iflag == 1) {
    entry_idx = strtoul(i_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (fflag == 1) {
    act_fn_hdl = strtoul(f_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (pflag == 1) {
    unsigned long log_pipe_ul = strtoul(p_arg, NULL, 0);
    if (log_pipe_ul >= PIPE_MGR_MAX_PIPES) {
      aim_printf(&uc->pvs, "Invalid log pipe %lu", log_pipe_ul);
      return UCLI_STATUS_OK;
    }
    log_pipe = (bf_dev_pipe_t)log_pipe_ul;
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (sflag == 1) {
    stage = strtoul(s_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  pipe_mgr_print_act_hw_tbl_info_by_idx_w_lock(dev_id,
                                               tbl_hdl,
                                               entry_idx,
                                               act_fn_hdl,
                                               log_pipe,
                                               stage,
                                               str,
                                               sizeof(str));

  aim_printf(&uc->pvs, "\n%s\n", str);
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(dump_mem) {
  int c, dflag, aflag;
  char *d_arg, *a_arg, *s_arg;
  bf_dev_id_t dev_id = 0;
  uint64_t addr = 0;
  bf_subdev_id_t subdev = 0;
  char str[PIPE_MGR_HW_DUMP_STR_LEN];

  PIPE_MGR_CLI_PROLOGUE("dump-mem",
                        " Dump physical HW memory.",
                        "-d <dev_id> -a <phy_addr> [-s <subdev>]");

  dflag = aflag = 0;
  d_arg = a_arg = s_arg = NULL;
  str[0] = '\0';

  while ((c = getopt(argc, argv, "d:a:s:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        d_arg = optarg;
        if (!d_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'a':
        aflag = 1;
        a_arg = optarg;
        if (!a_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 's':
        s_arg = optarg;
        if (!s_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        subdev = strtoull(s_arg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (dflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else if (dflag == 1) {
    dev_id = strtoul(d_arg, NULL, 0);
    if (dev_id > (PIPE_MGR_NUM_DEVICES - 1)) {
      aim_printf(&uc->pvs, "tbl: Invalid dev_id %s\n", d_arg);
      return UCLI_STATUS_OK;
    }
  }

  if (aflag == 1) {
    addr = strtoull(a_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  pipe_mgr_dump_hw_memory(dev_id, subdev, addr, str, sizeof(str));

  aim_printf(&uc->pvs, "\n%s\n", str);
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(dump_mem_full) {
  int c, dflag, pflag, sflag, mflag, tflag, lflag, gflag;
  char *d_arg, *p_arg, *s_arg, *m_arg, *t_arg, *l_arg, *g_arg;
  bf_dev_id_t dev_id = 0;
  bf_dev_pipe_t log_pipe_id = 0;
  uint8_t stage_id = 0, mem_type = 0, gress = 0;
  mem_id_t mem_id = 0;
  uint16_t line_no = 0;
  char str[PIPE_MGR_HW_DUMP_STR_LEN];

  PIPE_MGR_CLI_PROLOGUE("dump-mem-full",
                        " Dump physical memory.",
                        "-d <dev_id> -p <logical pipe_id>"
                        "-s <stage_id> -m <mem_id> -t "
                        "<mem_type> -l <line_no> -[g <in(0)/egress(1)>]");

  dflag = pflag = sflag = mflag = tflag = lflag = gflag = 0;
  d_arg = p_arg = s_arg = m_arg = t_arg = l_arg = g_arg = NULL;
  str[0] = '\0';

  while ((c = getopt(argc, argv, "d:p:s:m:t:l:g:")) != -1) {
    switch (c) {
      case 'g':
        gflag = 1;
        g_arg = optarg;
        if (!g_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'd':
        dflag = 1;
        d_arg = optarg;
        if (!d_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'p':
        pflag = 1;
        p_arg = optarg;
        if (!p_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 's':
        sflag = 1;
        s_arg = optarg;
        if (!s_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'm':
        mflag = 1;
        m_arg = optarg;
        if (!m_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 't':
        tflag = 1;
        t_arg = optarg;
        if (!t_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'l':
        lflag = 1;
        l_arg = optarg;
        if (!l_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (dflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else if (dflag == 1) {
    dev_id = strtoul(d_arg, NULL, 0);
    if (dev_id > (PIPE_MGR_NUM_DEVICES - 1)) {
      aim_printf(&uc->pvs, "Invalid dev_id %s\n", d_arg);
      return UCLI_STATUS_OK;
    }
  }
  if (gflag == 1) {
    gress = strtoul(g_arg, NULL, 0);
    if ((gress != 0) && (gress != 1)) {
      aim_printf(&uc->pvs, "Invalid direction, 0:ingress, 1:egress");
      return UCLI_STATUS_OK;
    }
  } else {
    /* By default pass gress as ingress.if
     * its tf1,tf2,tf3 it doesn't matter
     */
    gress = 0;
  }
  if (pflag == 1) {
    log_pipe_id = strtoul(p_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (sflag == 1) {
    stage_id = strtoul(s_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (mflag == 1) {
    mem_id = strtoul(m_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (tflag == 1) {
    mem_type = strtoul(t_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (lflag == 1) {
    line_no = strtoul(l_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  pipe_mgr_dump_hw_memory_full(dev_id,
                               gress,
                               log_pipe_id,
                               stage_id,
                               mem_id,
                               mem_type,
                               line_no,
                               str,
                               sizeof(str));

  aim_printf(&uc->pvs, "\n%s\n", str);
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(read_map_ram) {
  PIPE_MGR_CLI_PROLOGUE("read-map-ram",
                        " Read a mapRAM by unit-id or row+column",
                        "-d <dev_id> -p <logical pipe>\n\t\t\t-s <stage_id> "
                        "<-u <Unit id> | -r <row> -c <column>>\n\t\t\t-l "
                        "<line> -L <end line> -[g <in(0)/egress(1)>]");

  bool d, p, s, u, U, r, c, l, L, g;
  d = p = s = u = U = r = c = l = L = g = false;
  bf_dev_id_t dev = 0;
  int log_pipe = 0, stage = 0, unit = 0, unit_end = 0, row = 0, col = 0,
      line = 0, end_line = 0, gress = 0;

  int C;
  while ((C = getopt(argc, argv, "d:p:s:u:U:r:c:l:L:g")) != -1) {
    switch (C) {
      case 'd':
        d = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoul(optarg, NULL, 0);
        if ((dev < 0) || (dev >= PIPE_MGR_NUM_DEVICES)) {
          aim_printf(&uc->pvs, "Invalid device %d\n", dev);
          return UCLI_STATUS_OK;
        }
        break;
      case 'g':
        g = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        gress = strtoul(optarg, NULL, 0);
        if (gress < 0 || gress > 2) {
          aim_printf(&uc->pvs, "Invalid gress %d\n", gress);
          return UCLI_STATUS_OK;
        }
        break;
      case 'p':
        p = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        log_pipe = strtoul(optarg, NULL, 0);
        if (log_pipe < 0) {
          aim_printf(&uc->pvs, "Invalid pipe %d\n", log_pipe);
          return UCLI_STATUS_OK;
        }
        break;
      case 's':
        s = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        stage = strtoul(optarg, NULL, 0);
        if (stage < 0) {
          aim_printf(&uc->pvs, "Invalid stage %d\n", stage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'u':
        u = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        unit = strtoul(optarg, NULL, 0);
        if (unit < 0) {
          aim_printf(&uc->pvs, "Invalid unit-id %d\n", unit);
          return UCLI_STATUS_OK;
        }
        break;
      case 'U':
        U = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        unit_end = strtoul(optarg, NULL, 0);
        if (unit_end < 0) {
          aim_printf(&uc->pvs, "Invalid unit-id %d\n", unit);
          return UCLI_STATUS_OK;
        }
        break;
      case 'r':
        r = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        row = strtoul(optarg, NULL, 0);
        if (row < 0) {
          aim_printf(&uc->pvs, "Invalid row %d\n", row);
          return UCLI_STATUS_OK;
        }
        break;
      case 'c':
        c = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        col = strtoul(optarg, NULL, 0);
        if (col < 0) {
          aim_printf(&uc->pvs, "Invalid column %d\n", col);
          return UCLI_STATUS_OK;
        }
        break;
      case 'l':
        l = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        line = strtoul(optarg, NULL, 0);
        if (line < 0) {
          aim_printf(&uc->pvs, "Invalid map ram line %d\n", line);
          return UCLI_STATUS_OK;
        }
        break;
      case 'L':
        L = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        end_line = strtoul(optarg, NULL, 0);
        if (end_line < 0) {
          aim_printf(&uc->pvs, "Invalid map ram end_line %d\n", end_line);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!d || !p || !s || !l) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  if (!L) end_line = line;
  if (end_line < line) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (dev_info == NULL) {
    aim_printf(&uc->pvs, "Invalid device %d\n", dev);
    return UCLI_STATUS_OK;
  }
  if ((u || U) && (c || r)) {
    aim_printf(&uc->pvs,
               "Cannot provide both unit (-u) and row/col (-r -c)\n%s",
               usage);
    return UCLI_STATUS_OK;
  } else if (!u && c && r) {
    unit = row * dev_info->dev_cfg.stage_cfg.num_map_ram_cols + col;
  } else if (!u) {
    aim_printf(&uc->pvs, "Must provide either unit or row+col\n%s", usage);
    return UCLI_STATUS_OK;
  }
  if (log_pipe >= (int)dev_info->num_active_pipes) {
    aim_printf(&uc->pvs, "Invalid pipe %d\n", log_pipe);
    return UCLI_STATUS_OK;
  }
  if (stage >= dev_info->num_active_mau) {
    aim_printf(&uc->pvs, "Invalid stage %d\n", stage);
    return UCLI_STATUS_OK;
  }
  if (c && col >= dev_info->dev_cfg.stage_cfg.num_map_ram_cols) {
    aim_printf(&uc->pvs, "Invalid column %d\n", col);
    return UCLI_STATUS_OK;
  }
  if (r && row >= dev_info->dev_cfg.stage_cfg.num_map_ram_rows) {
    aim_printf(&uc->pvs, "Invalid row %d\n", row);
    return UCLI_STATUS_OK;
  }
  if (u && unit >= dev_info->dev_cfg.stage_cfg.num_map_ram_units) {
    aim_printf(&uc->pvs, "Invalid unit-id %d\n", unit);
    return UCLI_STATUS_OK;
  }
  if (U && unit_end >= dev_info->dev_cfg.stage_cfg.num_map_ram_units) {
    aim_printf(&uc->pvs, "Invalid end unit-id %d\n", unit_end);
    return UCLI_STATUS_OK;
  }
  if (l && line >= dev_info->dev_cfg.stage_cfg.map_ram_unit_depth) {
    aim_printf(&uc->pvs, "Invalid map ram line %d\n", line);
    return UCLI_STATUS_OK;
  }
  if (L && end_line >= dev_info->dev_cfg.stage_cfg.map_ram_unit_depth) {
    aim_printf(&uc->pvs, "Invalid map ram end line %d\n", end_line);
    return UCLI_STATUS_OK;
  }
  if (!U) unit_end = unit;
  if (unit_end < unit) {
    aim_printf(&uc->pvs,
               "Invalid map ram end unit %d, start unit %d\n",
               unit_end,
               unit);
    return UCLI_STATUS_OK;
  }

  bf_dev_pipe_t phy_pipe_id = log_pipe;
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe, &phy_pipe_id);
  bf_subdev_id_t subdev = pipe_mgr_subdev_id_from_pipe(phy_pipe_id);
  for (; unit <= unit_end; ++unit) {
    for (int j = line; j <= end_line; ++j) {
      uint64_t addr = dev_info->dev_cfg.get_full_phy_addr(
          gress, phy_pipe_id, stage, unit, j, pipe_mem_type_map_ram);
      uint64_t data_lo = 0, data_hi = 0;

      int x = lld_subdev_ind_read(dev, subdev, addr, &data_hi, &data_lo);
      if (x) {
        aim_printf(
            &uc->pvs, "Read failed, status %d, addr 0x%" PRIx64 "\n", x, addr);
      } else {
        uint16_t data = data_lo;
        uint16_t vpn = ~data & 0x3F;
        aim_printf(
            &uc->pvs,
            "0x%" PRIx64
            " LogPipe %d Stage %d mapRAM %2d line %4d data 0x%03x (VPN %d%s)\n",
            addr,
            log_pipe,
            stage,
            unit,
            j,
            data,
            vpn,
            vpn == 0x3F ? " Spare" : "");
      }
    }
  }
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(write_map_ram) {
  PIPE_MGR_CLI_PROLOGUE("write-map-ram",
                        " Write a mapRAM by unit-id or row+column",
                        "-d <dev_id> -p <log-pipe>\n\t\t\t-s <stage_id> <-u "
                        "<Unit id> | -r <row> -c <column>>\n\t\t\t-l <line> -L "
                        "<end line> -v <value>");

  bool d, p, s, u, r, c, l, L, v;
  d = p = s = u = r = c = l = L = v = false;
  bf_dev_id_t dev = 0;
  int log_pipe = 0, stage = 0, unit = 0, row = 0, col = 0, line = 0,
      end_line = 0, val = 0;

  int C;
  while ((C = getopt(argc, argv, "d:p:s:u:r:c:l:L:v:")) != -1) {
    switch (C) {
      case 'd':
        d = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoul(optarg, NULL, 0);
        if ((dev < 0) || (dev >= PIPE_MGR_NUM_DEVICES)) {
          aim_printf(&uc->pvs, "Invalid device %d\n", dev);
          return UCLI_STATUS_OK;
        }
        break;
      case 'p':
        p = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        log_pipe = strtoul(optarg, NULL, 0);
        if (log_pipe < 0) {
          aim_printf(&uc->pvs, "Invalid pipe %d\n", log_pipe);
          return UCLI_STATUS_OK;
        }
        break;
      case 's':
        s = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        stage = strtoul(optarg, NULL, 0);
        if (stage < 0) {
          aim_printf(&uc->pvs, "Invalid stage %d\n", stage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'u':
        u = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        unit = strtoul(optarg, NULL, 0);
        if (unit < 0) {
          aim_printf(&uc->pvs, "Invalid unit-id %d\n", unit);
          return UCLI_STATUS_OK;
        }
        break;
      case 'r':
        r = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        row = strtoul(optarg, NULL, 0);
        if (row < 0) {
          aim_printf(&uc->pvs, "Invalid row %d\n", row);
          return UCLI_STATUS_OK;
        }
        break;
      case 'c':
        c = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        col = strtoul(optarg, NULL, 0);
        if (col < 0) {
          aim_printf(&uc->pvs, "Invalid column %d\n", col);
          return UCLI_STATUS_OK;
        }
        break;
      case 'l':
        l = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        line = strtoul(optarg, NULL, 0);
        if (line < 0) {
          aim_printf(&uc->pvs, "Invalid map ram line %d\n", line);
          return UCLI_STATUS_OK;
        }
        break;
      case 'L':
        L = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        end_line = strtoul(optarg, NULL, 0);
        if (end_line < 0) {
          aim_printf(&uc->pvs, "Invalid map ram end_line %d\n", end_line);
          return UCLI_STATUS_OK;
        }
        break;
      case 'v':
        v = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        val = strtoul(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!d || !p || !s || !l || !v) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  if (!L) end_line = line;
  if (end_line < line) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    aim_printf(&uc->pvs, "Error in getting dev info for device %d\n", dev);
    return UCLI_STATUS_OK;
  }
  if (u && (c || r)) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else if (!u && c && r) {
    unit = row * dev_info->dev_cfg.stage_cfg.num_map_ram_cols + col;
  } else if (!u) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  if (log_pipe >= (int)dev_info->num_active_pipes) {
    aim_printf(&uc->pvs, "Invalid pipe %d\n", log_pipe);
    return UCLI_STATUS_OK;
  }
  if (stage >= dev_info->num_active_mau) {
    aim_printf(&uc->pvs, "Invalid stage %d\n", stage);
    return UCLI_STATUS_OK;
  }
  if (c && col >= dev_info->dev_cfg.stage_cfg.num_map_ram_cols) {
    aim_printf(&uc->pvs, "Invalid column %d\n", col);
    return UCLI_STATUS_OK;
  }
  if (r && row >= dev_info->dev_cfg.stage_cfg.num_map_ram_rows) {
    aim_printf(&uc->pvs, "Invalid row %d\n", row);
    return UCLI_STATUS_OK;
  }
  if (u && unit >= dev_info->dev_cfg.stage_cfg.num_map_ram_units) {
    aim_printf(&uc->pvs, "Invalid unit-id %d\n", unit);
    return UCLI_STATUS_OK;
  }
  if (l && line >= dev_info->dev_cfg.stage_cfg.map_ram_unit_depth) {
    aim_printf(&uc->pvs, "Invalid map ram line %d\n", line);
    return UCLI_STATUS_OK;
  }
  if (L && end_line >= dev_info->dev_cfg.stage_cfg.map_ram_unit_depth) {
    aim_printf(&uc->pvs, "Invalid map ram end line %d\n", end_line);
    return UCLI_STATUS_OK;
  }

  bf_dev_pipe_t phy_pipe_id = log_pipe;
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe, &phy_pipe_id);
  bf_subdev_id_t subdev = pipe_mgr_subdev_id_from_pipe(phy_pipe_id);
  for (; line <= end_line; ++line) {
    uint64_t addr = dev_info->dev_cfg.get_full_phy_addr(
        0, phy_pipe_id, stage, unit, line, pipe_mem_type_map_ram);
    uint64_t data_lo = val, data_hi = 0;

    int x = lld_subdev_ind_write(dev, subdev, addr, data_hi, data_lo);
    if (x) {
      aim_printf(&uc->pvs,
                 "Write failed, status %d, addr 0x%" PRIx64 " data 0x%" PRIx64
                 "\n",
                 x,
                 addr,
                 data_lo);
    } else {
      aim_printf(&uc->pvs,
                 "0x%" PRIx64
                 " LogPipe %d Stage %d mapRAM %d line %4d, wrote 0x%x\n",
                 addr,
                 log_pipe,
                 stage,
                 unit,
                 line,
                 val);
    }
  }
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(read_unit_ram) {
  PIPE_MGR_CLI_PROLOGUE("read-unit-ram",
                        " Read a unitRAM by unit-id or row+column",
                        "-d <dev_id> -p <log_pipe>\n\t\t\t-s <stage_id> <-u "
                        "<Unit id> | -r <row> -c <column>>\n\t\t\t-l <line> -L "
                        "<end line>");

  bool d, p, s, u, r, c, l, L;
  d = p = s = u = r = c = l = L = false;
  bf_dev_id_t dev = 0;
  int log_pipe = 0, stage = 0, unit = 0, row = 0, col = 0, line = 0,
      end_line = 0;

  int C;
  while ((C = getopt(argc, argv, "d:p:s:u:r:c:l:L:")) != -1) {
    switch (C) {
      case 'd':
        d = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoul(optarg, NULL, 0);
        if ((dev < 0) || (dev >= PIPE_MGR_NUM_DEVICES)) {
          aim_printf(&uc->pvs, "Invalid device %d\n", dev);
          return UCLI_STATUS_OK;
        }
        break;
      case 'p':
        p = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        log_pipe = strtoul(optarg, NULL, 0);
        if (log_pipe < 0) {
          aim_printf(&uc->pvs, "Invalid pipe %d\n", log_pipe);
          return UCLI_STATUS_OK;
        }
        break;
      case 's':
        s = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        stage = strtoul(optarg, NULL, 0);
        if (stage < 0) {
          aim_printf(&uc->pvs, "Invalid stage %d\n", stage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'u':
        u = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        unit = strtoul(optarg, NULL, 0);
        if (unit < 0) {
          aim_printf(&uc->pvs, "Invalid unit-id %d\n", unit);
          return UCLI_STATUS_OK;
        }
        break;
      case 'r':
        r = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        row = strtoul(optarg, NULL, 0);
        if (row < 0) {
          aim_printf(&uc->pvs, "Invalid row %d\n", row);
          return UCLI_STATUS_OK;
        }
        break;
      case 'c':
        c = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        col = strtoul(optarg, NULL, 0);
        break;
      case 'l':
        l = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        line = strtoul(optarg, NULL, 0);
        if (line < 0) {
          aim_printf(&uc->pvs, "Invalid unit ram line %d\n", line);
          return UCLI_STATUS_OK;
        }
        break;
      case 'L':
        L = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        end_line = strtoul(optarg, NULL, 0);
        if (end_line < 0) {
          aim_printf(&uc->pvs, "Invalid unit ram end line %d\n", end_line);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    aim_printf(&uc->pvs, "Invalid device %d\n", dev);
    return UCLI_STATUS_OK;
  }
  rmt_dev_cfg_t *cfg = &dev_info->dev_cfg;
  if (u && (c || r)) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else if (!u && c && r) {
    unit = row * cfg->stage_cfg.num_sram_cols + col;
  } else if (!u) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else {
    col = cfg->mem_id_to_col(unit, pipe_mem_type_unit_ram);
    row = cfg->mem_id_to_row(unit, pipe_mem_type_unit_ram);
    if (!cfg->sram_col_valid(col) || !cfg->sram_row_valid(row)) {
      aim_printf(&uc->pvs, "Invalid unit-id %d\n", unit);
      return UCLI_STATUS_OK;
    }
  }
  if (!d || !p || !s || !l) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  if (!L) end_line = line;
  if (end_line < line) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  if (log_pipe >= (int)dev_info->num_active_pipes) {
    aim_printf(&uc->pvs, "Invalid pipe %d\n", log_pipe);
    return UCLI_STATUS_OK;
  }
  if (stage >= cfg->num_stages) {
    aim_printf(&uc->pvs, "Invalid stage %d\n", stage);
    return UCLI_STATUS_OK;
  }
  if (!cfg->sram_row_valid(row)) {
    aim_printf(&uc->pvs, "Invalid row %d\n", row);
    return UCLI_STATUS_OK;
  }
  if (!cfg->sram_col_valid(col)) {
    aim_printf(&uc->pvs, "Invalid col %d\n", col);
    return UCLI_STATUS_OK;
  }
  if (line >= cfg->stage_cfg.sram_unit_depth) {
    aim_printf(&uc->pvs, "Invalid unit ram line %d\n", line);
    return UCLI_STATUS_OK;
  }
  if (end_line >= cfg->stage_cfg.sram_unit_depth) {
    aim_printf(&uc->pvs, "Invalid unit ram end line %d\n", end_line);
    return UCLI_STATUS_OK;
  }

  bf_dev_pipe_t phy_pipe_id = log_pipe;
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe, &phy_pipe_id);
  bf_subdev_id_t subdev = pipe_mgr_subdev_id_from_pipe(phy_pipe_id);
  for (; line <= end_line; ++line) {
    uint64_t addr = cfg->get_full_phy_addr(
        0, phy_pipe_id, stage, unit, line, pipe_mem_type_unit_ram);
    uint64_t data_lo = 0, data_hi = 0;

    int x = lld_subdev_ind_read(dev, subdev, addr, &data_hi, &data_lo);
    if (x) {
      aim_printf(
          &uc->pvs, "Read failed, status %d, addr 0x%" PRIx64 "\n", x, addr);
    } else {
      aim_printf(&uc->pvs,
                 "0x%" PRIx64
                 " LogPipe %d Stage %d unit %d line %4d, read 0x%016" PRIx64
                 "_%016" PRIx64 "\n",
                 addr,
                 log_pipe,
                 stage,
                 unit,
                 line,
                 data_hi,
                 data_lo);
    }
  }
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(write_unit_ram) {
  PIPE_MGR_CLI_PROLOGUE("write-unit-ram",
                        " Write a unit RAM by unit-id or row+column",
                        "-d <dev_id> -p <log_pipe>\n\t\t\t-s <stage_id> <-u "
                        "<Unit id> | -r <row> -c <column>>\n\t\t\t-l <line> -L "
                        "<end line> -v <value_lo> [-v <value_hi>]");

  bool d, p, s, u, r, c, l, L, v;
  d = p = s = u = r = c = l = L = v = false;
  bf_dev_id_t dev = 0;
  int log_pipe = 0, stage = 0, unit = 0, row = 0, col = 0, line = 0,
      end_line = 0;
  uint64_t data_hi = 0, data_lo = 0;

  int C;
  while ((C = getopt(argc, argv, "d:p:s:u:r:c:l:L:v:")) != -1) {
    switch (C) {
      case 'd':
        d = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoul(optarg, NULL, 0);
        if ((dev < 0) || (dev >= PIPE_MGR_NUM_DEVICES)) {
          aim_printf(&uc->pvs, "Invalid device %d\n", dev);
          return UCLI_STATUS_OK;
        }
        break;
      case 'p':
        p = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        log_pipe = strtoul(optarg, NULL, 0);
        if (log_pipe < 0) {
          aim_printf(&uc->pvs, "Invalid pipe %d\n", log_pipe);
          return UCLI_STATUS_OK;
        }
        break;
      case 's':
        s = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        stage = strtoul(optarg, NULL, 0);
        if (stage < 0) {
          aim_printf(&uc->pvs, "Invalid stage %d\n", stage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'u':
        u = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        unit = strtoul(optarg, NULL, 0);
        if (unit < 0) {
          aim_printf(&uc->pvs, "Invalid unit-id %d\n", unit);
          return UCLI_STATUS_OK;
        }
        break;
      case 'r':
        r = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        row = strtoul(optarg, NULL, 0);
        if (row < 0) {
          aim_printf(&uc->pvs, "Invalid row %d\n", row);
          return UCLI_STATUS_OK;
        }
        break;
      case 'c':
        c = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        col = strtoul(optarg, NULL, 0);
        break;
      case 'l':
        l = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        line = strtoul(optarg, NULL, 0);
        if (line < 0) {
          aim_printf(&uc->pvs, "Invalid unit ram line %d\n", line);
          return UCLI_STATUS_OK;
        }
        break;
      case 'L':
        L = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        end_line = strtoul(optarg, NULL, 0);
        if (end_line < 0) {
          aim_printf(&uc->pvs, "Invalid unit ram end line %d\n", end_line);
          return UCLI_STATUS_OK;
        }
        break;
      case 'v':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        if (!v)
          data_lo = strtoull(optarg, NULL, 0);
        else
          data_hi = strtoull(optarg, NULL, 0);
        v = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    aim_printf(&uc->pvs, "Invalid device %d\n", dev);
    return UCLI_STATUS_OK;
  }
  rmt_dev_cfg_t *cfg = &dev_info->dev_cfg;
  if (u && (c || r)) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else if (!u && c && r) {
    unit = row * cfg->stage_cfg.num_sram_cols + col;
  } else if (!u) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else {
    col = cfg->mem_id_to_col(unit, pipe_mem_type_unit_ram);
    row = cfg->mem_id_to_row(unit, pipe_mem_type_unit_ram);
    if (!cfg->sram_col_valid(col) || !cfg->sram_row_valid(row)) {
      aim_printf(&uc->pvs, "Invalid unit-id %d\n", unit);
      return UCLI_STATUS_OK;
    }
  }
  if (!d || !p || !s || !l || !v) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  if (!L) end_line = line;
  if (end_line < line) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  if (log_pipe >= (int)dev_info->num_active_pipes) {
    aim_printf(&uc->pvs, "Invalid pipe %d\n", log_pipe);
    return UCLI_STATUS_OK;
  }
  if (stage >= cfg->num_stages) {
    aim_printf(&uc->pvs, "Invalid stage %d\n", stage);
    return UCLI_STATUS_OK;
  }
  if (!cfg->sram_row_valid(row)) {
    aim_printf(&uc->pvs, "Invalid row %d\n", row);
    return UCLI_STATUS_OK;
  }
  if (!cfg->sram_col_valid(col)) {
    aim_printf(&uc->pvs, "Invalid col %d\n", col);
    return UCLI_STATUS_OK;
  }
  if (line >= cfg->stage_cfg.sram_unit_depth) {
    aim_printf(&uc->pvs, "Invalid unit ram line %d\n", line);
    return UCLI_STATUS_OK;
  }
  if (end_line >= cfg->stage_cfg.sram_unit_depth) {
    aim_printf(&uc->pvs, "Invalid unit ram end line %d\n", end_line);
    return UCLI_STATUS_OK;
  }

  bf_dev_pipe_t phy_pipe_id = log_pipe;
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe, &phy_pipe_id);
  bf_subdev_id_t subdev = pipe_mgr_subdev_id_from_pipe(phy_pipe_id);
  for (; line <= end_line; ++line) {
    uint64_t addr = cfg->get_full_phy_addr(
        0, phy_pipe_id, stage, unit, line, pipe_mem_type_unit_ram);

    int x = lld_subdev_ind_write(dev, subdev, addr, data_hi, data_lo);
    if (x) {
      aim_printf(&uc->pvs,
                 "Write failed, status %d, addr 0x%" PRIx64
                 " data 0x%016" PRIx64 "_%016" PRIx64 "\n",
                 x,
                 addr,
                 data_hi,
                 data_lo);
    } else {
      aim_printf(&uc->pvs,
                 "0x%" PRIx64
                 " LogPipe %d Stage %d unit %d line %4d, wrote 0x%016" PRIx64
                 "_%016" PRIx64 "\n",
                 addr,
                 log_pipe,
                 stage,
                 unit,
                 line,
                 data_hi,
                 data_lo);
    }
  }
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(read_virt) {
  PIPE_MGR_CLI_PROLOGUE(
      "read-virt",
      " Virtually read a logical table",
      "-d <dev_id> -p <log-pipe> -s <stage_id> -l <logical table "
      "id> -v <vpn> -w <ram word> -t "
      "<stats|meter|stateful|selection|idle>");

  bool d, p, s, l, v, w, t;
  d = p = s = l = v = w = t = false;
  bf_dev_id_t dev = 0;
  int log_pipe = 0, stage = 0, lt = 0, vpn = 0, word = 0, type = 0;

  int C;
  while ((C = getopt(argc, argv, "d:p:s:l:v:w:t:")) != -1) {
    switch (C) {
      case 'd':
        d = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoul(optarg, NULL, 0);
        if ((dev < 0) || (dev >= PIPE_MGR_NUM_DEVICES)) {
          aim_printf(&uc->pvs, "Invalid device %d\n", dev);
          return UCLI_STATUS_OK;
        }
        break;
      case 'p':
        p = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        log_pipe = strtoul(optarg, NULL, 0);
        if (log_pipe < 0) {
          aim_printf(&uc->pvs, "Invalid pipe %d\n", log_pipe);
          return UCLI_STATUS_OK;
        }
        break;
      case 's':
        s = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        stage = strtoul(optarg, NULL, 0);
        if (stage < 0) {
          aim_printf(&uc->pvs, "Invalid stage %d\n", stage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'l':
        l = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        lt = strtoul(optarg, NULL, 0);
        if (lt < 0 || stage >= 16) {
          aim_printf(&uc->pvs, "Invalid logical table %d\n", lt);
          return UCLI_STATUS_OK;
        }
        break;
      case 'v':
        v = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        vpn = strtoul(optarg, NULL, 0);
        if (vpn < 0 || vpn >= 0x3F) {
          aim_printf(&uc->pvs, "Invalid vpn %d\n", vpn);
          return UCLI_STATUS_OK;
        }
        break;
      case 'w':
        w = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        word = strtoul(optarg, NULL, 0);
        if (word < 0) {
          aim_printf(&uc->pvs, "Invalid ram word %d\n", word);
          return UCLI_STATUS_OK;
        }
        break;
      case 't':
        t = true;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        if (0 == strncmp(optarg, "stats", 5)) {
          type = pipe_virt_mem_type_stat;
        } else if (0 == strncmp(optarg, "meter", 5)) {
          type = pipe_virt_mem_type_meter;
        } else if (0 == strncmp(optarg, "stateful", 5)) {
          type = pipe_virt_mem_type_sel_stful;
        } else if (0 == strncmp(optarg, "selection", 5)) {
          type = pipe_virt_mem_type_sel_stful;
        } else if (0 == strncmp(optarg, "idle", 4)) {
          type = pipe_virt_mem_type_idle;
        } else {
          aim_printf(&uc->pvs, "Invalid type %s\n", optarg);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!d || !p || !s || !l || !v || !w || !t) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) return UCLI_STATUS_OK;

  if (log_pipe >= (int)dev_info->num_active_pipes) {
    aim_printf(&uc->pvs, "Invalid pipe %d\n", log_pipe);
    return UCLI_STATUS_OK;
  }
  if (stage >= dev_info->num_active_mau) {
    aim_printf(&uc->pvs, "Invalid stage %d\n", stage);
    return UCLI_STATUS_OK;
  }
  if (word >= dev_info->dev_cfg.stage_cfg.sram_unit_depth) {
    aim_printf(&uc->pvs, "Invalid ram word %d\n", word);
    return UCLI_STATUS_OK;
  }

  pipe_full_virt_addr_t vaddr;
  uint32_t lower_vaddr = 0;
  if (pipe_virt_mem_type_stat == type) {
    lower_vaddr = (vpn << 13) | (word << 3);
  } else if (pipe_virt_mem_type_meter == type) {
    lower_vaddr = (vpn << 10) | word;
  } else if (pipe_virt_mem_type_sel_stful == type) {
    lower_vaddr = (vpn << 15) | (word << 5);
  } else {
    lower_vaddr = (vpn << 14) | (word << 4);
  }

  bf_dev_pipe_t phy_pipe_id = log_pipe;
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe, &phy_pipe_id);
  bf_subdev_id_t subdev = pipe_mgr_subdev_id_from_pipe(phy_pipe_id);

  construct_full_virt_addr(
      dev_info, &vaddr, lt, type, lower_vaddr, phy_pipe_id, stage);
  uint64_t full_addr = vaddr.addr;
  uint64_t data_hi = 0, data_lo = 0;
  int x = lld_subdev_ind_read(dev, subdev, full_addr, &data_hi, &data_lo);
  if (x) {
    aim_printf(&uc->pvs, "Indirect read failed: %d\n", x);
    return UCLI_STATUS_OK;
  } else {
    aim_printf(
        &uc->pvs,
        "0x%" PRIx64
        " on dev %d (type %d log-pipe %d stage %d table %d vpn %d line %d) "
        "== 0x%016" PRIx64 "_%016" PRIx64 "\n",
        full_addr,
        dev,
        type,
        log_pipe,
        stage,
        lt,
        vpn,
        word,
        data_hi,
        data_lo);
  }

  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(write_mem) {
  int c, dflag, aflag, iflag, sflag, eflag;
  char *d_arg, *a_arg, *i_arg, *s_arg, *e_arg, *u_arg;
  bf_dev_id_t dev_id = 0;
  uint64_t addr = 0, data0 = 0, data1 = 0;
  int start_bit = -1, end_bit = -1;
  char str[PIPE_MGR_HW_DUMP_STR_LEN];
  bf_subdev_id_t subdev = 0;

  PIPE_MGR_CLI_PROLOGUE("write-mem",
                        " Write to physical HW memory.",
                        "-d <dev_id> -a <phy_addr> -i \"data\" [-s <start-bit> "
                        "-e <end-bit> -u <subdev>]");

  dflag = aflag = iflag = sflag = eflag = 0;
  d_arg = a_arg = i_arg = s_arg = e_arg = u_arg = NULL;
  str[0] = '\0';

  while ((c = getopt(argc, argv, "d:a:i:s:e:u:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        d_arg = optarg;
        if (!d_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'a':
        aflag = 1;
        a_arg = optarg;
        if (!a_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'i':
        iflag = 1;
        i_arg = optarg;
        if (!i_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 's':
        sflag = 1;
        s_arg = optarg;
        if (!s_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'e':
        eflag = 1;
        e_arg = optarg;
        if (!e_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'u':
        u_arg = optarg;
        if (!u_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        subdev = strtoul(u_arg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (dflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else if (dflag == 1) {
    dev_id = strtoul(d_arg, NULL, 0);
    if (dev_id > (PIPE_MGR_NUM_DEVICES - 1)) {
      aim_printf(&uc->pvs, "tbl: Invalid dev_id %s\n", d_arg);
      return UCLI_STATUS_OK;
    }
  }

  if (aflag == 1) {
    addr = strtoull(a_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (((sflag == 1) && (eflag == 0)) || ((sflag == 0) && (eflag == 1))) {
    aim_printf(&uc->pvs, "%s", "Both Start and End bit needed");
    return UCLI_STATUS_OK;
  }

  if (sflag == 1) {
    start_bit = strtoul(s_arg, NULL, 0);
  }
  if (eflag == 1) {
    end_bit = strtoul(e_arg, NULL, 0);
  }
  if ((sflag == 1) && (eflag == 1)) {
    if (end_bit < start_bit) {
      aim_printf(&uc->pvs, "%s", "End bit is less than start bit ");
      return UCLI_STATUS_OK;
    } else if ((end_bit - start_bit) >= 64) {
      aim_printf(
          &uc->pvs,
          "%s",
          "Range greater than 64 bits not supported, use without range ");
      return UCLI_STATUS_OK;
    } else if ((start_bit < 64) && (end_bit > 64)) {
      aim_printf(
          &uc->pvs,
          "%s",
          "Range across 64 bit boundaries not supported, use without range ");
      return UCLI_STATUS_OK;
    }
  }

  if (iflag == 1) {
    char temp_str[100];
    char *str_d;
    bool data1_done = false;
    int len = 0, i = 0, shift = 0, data_cnt = 0;
    int data_size = 16;  // 16 bytes

    str_d = i_arg;
    len = strlen(str_d);

    for (i = 0; i < len;) {
      if (str_d[i] == ' ') {
        i++;
        continue;
      }
      temp_str[0] = str_d[i];
      temp_str[1] = str_d[i + 1];
      temp_str[2] = '\0';
      if (data1_done) {
        data0 |= (uint64_t)strtoul(temp_str, NULL, 16) << ((8 - shift - 1) * 8);
      } else {
        data1 |= (uint64_t)strtoul(temp_str, NULL, 16) << ((8 - shift - 1) * 8);
      }
      data_cnt++;
      shift++;
      i = i + 2;  // incr by 2 as we extracted two bytes
      if (data_cnt == 8) {
        data1_done = true;
        shift = 0;
      }
    }
    if ((sflag == 0) && (data_cnt != data_size)) {
      aim_printf(&uc->pvs,
                 "%s",
                 "Invalid data specified, space separated byte"
                 " format expected in double quotes ");
      return UCLI_STATUS_OK;
    }
    if (sflag == 1) {
      int num_bytes = 0;
      num_bytes = (end_bit - start_bit + 1) / 8;
      if (((end_bit - start_bit + 1) % 8) > 0) num_bytes++;
      if (data_cnt < num_bytes) {
        aim_printf(&uc->pvs,
                   "%s",
                   "Invalid data specified, length of data"
                   " less than bit range ");
        return UCLI_STATUS_OK;
      }
    }
    // printf("\ndata0 0x%lx, data1 0x%lx \n", data0, data1);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  pipe_mgr_write_hw_memory_with_start_end(
      dev_id, subdev, addr, data0, data1, start_bit, end_bit, str, sizeof(str));

  aim_printf(&uc->pvs, "\n%s\n", str);
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(write_tcam) {
  int c, dflag, aflag, kflag, mflag;
  char *d_arg, *a_arg, *k_arg, *m_arg;
  int yflag, tflag, vflag, hflag, bflag, eflag;
  char *y_arg, *t_arg, *v_arg, *h_arg, *b_arg, *e_arg;
  int pflag, sflag, rflag, cflag, lflag;
  char *p_arg, *s_arg, *r_arg, *c_arg, *l_arg, *u_arg;
  bf_dev_id_t dev_id = 0;
  uint8_t payload = PYLD_INVALID_VAL, mrd = PYLD_INVALID_VAL,
          version = PYLD_INVALID_VAL;
  uint64_t addr = 0, key = 0, mask = 0;
  uint32_t tbl_hdl = 0;
  int start_bit = -1, end_bit = -1;
  char str[PIPE_MGR_HW_DUMP_STR_LEN];
  int log_pipe = 0, stage = 0, row = 0, col = 0, line = 0;
  bf_subdev_id_t subdev = 0;

  PIPE_MGR_CLI_PROLOGUE(
      "write-tcam",
      " Write to tcam physical memory.",
      "-d <dev_id> { <-h <tbl_hdl> -a <phy_addr> [-u subdev]> |"
      " <-p <log_pipe> -s <stage_id> "
      "-r <row> -c <col> -l <line> }"
      " -k \"key\" -m "
      "\"mask\" -y <payload> -t <mrd> -v <version> "
      "[-b <start-bit> -e <end-bit>]");

  dflag = aflag = kflag = mflag = 0;
  d_arg = a_arg = k_arg = m_arg = NULL;
  yflag = tflag = vflag = hflag = bflag = eflag = 0;
  y_arg = t_arg = v_arg = h_arg = b_arg = e_arg = NULL;
  pflag = sflag = rflag = cflag = lflag = 0;
  p_arg = s_arg = r_arg = c_arg = l_arg = u_arg = NULL;
  str[0] = '\0';

  while ((c = getopt(argc, argv, "d:a:k:m:y:t:v:h:b:e:p:s:r:c:l:u:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        d_arg = optarg;
        if (!d_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'a':
        aflag = 1;
        a_arg = optarg;
        if (!a_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'k':
        kflag = 1;
        k_arg = optarg;
        if (!k_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'm':
        mflag = 1;
        m_arg = optarg;
        if (!m_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'y':
        yflag = 1;
        y_arg = optarg;
        if (!y_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 't':
        tflag = 1;
        t_arg = optarg;
        if (!t_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'v':
        vflag = 1;
        v_arg = optarg;
        if (!v_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'h':
        hflag = 1;
        h_arg = optarg;
        if (!h_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'b':
        bflag = 1;
        b_arg = optarg;
        if (!b_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'e':
        eflag = 1;
        e_arg = optarg;
        if (!e_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'p':
        pflag = 1;
        p_arg = optarg;
        if (!p_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        log_pipe = strtoul(p_arg, NULL, 0);
        if (log_pipe < 0) {
          aim_printf(&uc->pvs, "Invalid pipe %d\n", log_pipe);
          return UCLI_STATUS_OK;
        }
        break;
      case 's':
        sflag = 1;
        s_arg = optarg;
        if (!s_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        stage = strtoul(s_arg, NULL, 0);
        if (stage < 0) {
          aim_printf(&uc->pvs, "Invalid stage %d\n", stage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'r':
        rflag = 1;
        r_arg = optarg;
        if (!r_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        row = strtoul(r_arg, NULL, 0);
        break;
      case 'c':
        cflag = 1;
        c_arg = optarg;
        if (!c_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        col = strtoul(c_arg, NULL, 0);
        break;
      case 'l':
        lflag = 1;
        l_arg = optarg;
        if (!l_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        line = strtoul(l_arg, NULL, 0);
        if (line < 0) {
          aim_printf(&uc->pvs, "Invalid unit ram line %d\n", line);
          return UCLI_STATUS_OK;
        }
        break;
      case 'u':
        u_arg = optarg;
        if (!u_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        subdev = strtoul(u_arg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (dflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else if (dflag == 1) {
    dev_id = strtoul(d_arg, NULL, 0);
    if (dev_id > (PIPE_MGR_NUM_DEVICES - 1)) {
      aim_printf(&uc->pvs, "tbl: Invalid dev_id %s\n", d_arg);
      return UCLI_STATUS_OK;
    }
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    aim_printf(&uc->pvs, "tbl: Invalid dev_id %s\n", d_arg);
    return UCLI_STATUS_OK;
  }

  if (aflag == 1) {
    addr = strtoull(a_arg, NULL, 0);
  } else {
    rmt_tbl_type_t tbl_type = 0;
    pipe_mem_type_t mem_type = pipe_mem_type_tcam;
    if (!pflag || !sflag || !rflag || !cflag || !lflag || hflag) {
      aim_printf(&uc->pvs, "%s", usage);
      return UCLI_STATUS_OK;
    }
    mem_id_t mem_id =
        dev_info->dev_cfg.mem_id_from_col_row(stage, col, row, mem_type);
    aim_printf(&uc->pvs,
               "dev_id %d, log-pipe %d, stage %d, row %d, col %d, line %d, "
               "mem-id %d\n",
               dev_id,
               log_pipe,
               stage,
               row,
               col,
               line,
               mem_id);
    bf_dev_pipe_t phy_pipe_id = log_pipe;
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe, &phy_pipe_id);
    subdev = pipe_mgr_subdev_id_from_pipe(phy_pipe_id);
    addr = dev_info->dev_cfg.get_full_phy_addr(
        0, phy_pipe_id, stage, mem_id, line, mem_type);
    aim_printf(&uc->pvs, "tbl: Address is 0x%" PRIx64 "\n", addr);
    if (pipe_mgr_get_mem_id_to_tbl_hdl_mapping(
            dev_id, log_pipe, stage, mem_id, mem_type, &tbl_hdl, &tbl_type) !=
        PIPE_SUCCESS) {
      aim_printf(&uc->pvs,
                 "Unable to find tcam mem-id %d to tbl-hdl mapping, Mem-id not "
                 "in use\n",
                 mem_id);
      return UCLI_STATUS_OK;
    }
    aim_printf(&uc->pvs,
               "tcam mem-id %d maps to tbl-hdl 0x%x, tbl-type: %s\n",
               mem_id,
               tbl_hdl,
               pipe_mgr_rmt_tbl_type2str(tbl_type));
  }

  if (yflag == 1) {
    payload = strtoull(y_arg, NULL, 0);
  } else {
    /* Ignore payload and mrd only when start and end are specified */
    if (bflag == 0) {
      aim_printf(&uc->pvs, "%s", usage);
      return UCLI_STATUS_OK;
    }
  }

  if (tflag == 1) {
    mrd = strtoull(t_arg, NULL, 0);
  } else {
    /* Ignore payload and mrd only when start and end are specified */
    if (bflag == 0) {
      aim_printf(&uc->pvs, "%s", usage);
      return UCLI_STATUS_OK;
    }
  }

  if (vflag == 1) {
    version = strtoull(v_arg, NULL, 0);
  }

  if (hflag == 1) {
    tbl_hdl = strtoul(h_arg, NULL, 0);
  } else {
    if (aflag == 1) {
      aim_printf(&uc->pvs, "%s", usage);
      return UCLI_STATUS_OK;
    }
  }

  if (((kflag == 1) && (mflag == 0)) || ((kflag == 0) && (mflag == 1))) {
    aim_printf(&uc->pvs, "%s", "Both Key and mask needed");
    return UCLI_STATUS_OK;
  }

  if (((bflag == 1) && (eflag == 0)) || ((bflag == 0) && (eflag == 1))) {
    aim_printf(&uc->pvs, "%s", "Both Start and End bit needed");
    return UCLI_STATUS_OK;
  }

  if (bflag == 1) {
    if (!s_arg) {
      aim_printf(&uc->pvs, "%s", usage);
      return UCLI_STATUS_OK;
    }
    start_bit = strtoul(s_arg, NULL, 0);
  }
  if (eflag == 1) {
    end_bit = strtoul(e_arg, NULL, 0);
  }
  if ((bflag == 1) && (eflag == 1)) {
    if (end_bit < start_bit) {
      aim_printf(&uc->pvs, "%s", "End bit is less than start bit ");
      return UCLI_STATUS_OK;
    }
  }

  if (kflag == 1) {
    char temp_str[100];
    char *str_d;
    int len = 0, i = 0, shift = 0, data_cnt = 0;
    int data_size = 6;  // 6 bytes

    str_d = k_arg;
    len = strlen(str_d);

    for (i = 0; i < len;) {
      if (str_d[i] == ' ') {
        i++;
        continue;
      }
      temp_str[0] = str_d[i];
      temp_str[1] = str_d[i + 1];
      temp_str[2] = '\0';
      key |= (uint64_t)strtoul(temp_str, NULL, 16) << ((6 - shift - 1) * 8);
      data_cnt++;
      shift++;
      i = i + 2;  // incr by 2 as we extracted two bytes
    }
    if (data_cnt != data_size) {
      if (bflag == 0) {
        aim_printf(&uc->pvs,
                   "%s",
                   "Invalid Key specified, space separated byte"
                   " format expected in double quotes ");
        return UCLI_STATUS_OK;
      } else {  // right shift to bring bytes at lowest address
        key >>= ((data_size - data_cnt) * 8);
      }
    }
    if (bflag == 1) {
      int num_bytes = 0;
      num_bytes = (end_bit - start_bit + 1) / 8;
      if (((end_bit - start_bit + 1) % 8) > 0) num_bytes++;
      if (data_cnt < num_bytes) {
        aim_printf(&uc->pvs,
                   "%s",
                   "Invalid key specified, length of key"
                   " less than bit range ");
        return UCLI_STATUS_OK;
      }
    }
    // printf("\nkey 0x%lx \n", key);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (mflag == 1) {
    char temp_str[100];
    char *str_d;
    int len = 0, i = 0, shift = 0, data_cnt = 0;
    int data_size = 6;  // 6 bytes

    str_d = m_arg;
    len = strlen(str_d);

    for (i = 0; i < len;) {
      if (str_d[i] == ' ') {
        i++;
        continue;
      }
      temp_str[0] = str_d[i];
      temp_str[1] = str_d[i + 1];
      temp_str[2] = '\0';
      mask |= (uint64_t)strtoul(temp_str, NULL, 16) << ((6 - shift - 1) * 8);
      data_cnt++;
      shift++;
      i = i + 2;  // incr by 2 as we extracted two bytes
    }
    if (data_cnt != data_size) {
      if (bflag == 0) {
        aim_printf(&uc->pvs,
                   "%s",
                   "Invalid mask specified, space separated byte"
                   " format expected in double quotes ");
        return UCLI_STATUS_OK;
      } else {  // right shift to bring bytes at lowest address
        mask >>= ((data_size - data_cnt) * 8);
      }
    }
    if (bflag == 1) {
      int num_bytes = 0;
      num_bytes = (end_bit - start_bit + 1) / 8;
      if (((end_bit - start_bit + 1) % 8) > 0) num_bytes++;
      if (data_cnt < num_bytes) {
        aim_printf(&uc->pvs,
                   "%s",
                   "Invalid mask specified, length of mask"
                   " less than bit range ");
        return UCLI_STATUS_OK;
      }
    }
    // printf("\nMask 0x%lx \n", mask);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  pipe_mgr_write_tcam_memory_with_start_end(dev_id,
                                            subdev,
                                            tbl_hdl,
                                            addr,
                                            key,
                                            mask,
                                            payload,
                                            mrd,
                                            version,
                                            start_bit,
                                            end_bit,
                                            str,
                                            sizeof(str));
  aim_printf(&uc->pvs, "\n%s\n", str);
  return UCLI_STATUS_OK;
}

static void prsr_tcam_rd_one(aim_pvs_t *pvs,
                             rmt_dev_info_t *dev_info,
                             bf_dev_pipe_t phy_pipe,
                             bool ing_0_egr_1,
                             int prsr_id,
                             int line) {
  uint64_t base0, base1, pipe_step, prsr_step, a0, a1, w0, w1, dont_care;
  bf_subdev_id_t subdev = pipe_mgr_subdev_id_from_pipe(phy_pipe);
  if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO) {
    base0 = ing_0_egr_1 ? pipe_top_level_pipes_e_prsr_ml_tcam_row_word0_address
                        : pipe_top_level_pipes_i_prsr_ml_tcam_row_word0_address;
    base1 = ing_0_egr_1 ? pipe_top_level_pipes_e_prsr_ml_tcam_row_word1_address
                        : pipe_top_level_pipes_i_prsr_ml_tcam_row_word1_address;
    pipe_step = pipe_top_level_pipes_array_element_size;
    prsr_step = ing_0_egr_1 ? pipe_top_level_pipes_e_prsr_array_element_size
                            : pipe_top_level_pipes_i_prsr_array_element_size;
    a0 = (base0 + phy_pipe * pipe_step + prsr_id * prsr_step) >> 4;
    a1 = (base1 + phy_pipe * pipe_step + prsr_id * prsr_step) >> 4;
    lld_subdev_ind_read(dev_info->dev_id, subdev, a0 + line, &dont_care, &w0);
    lld_subdev_ind_read(dev_info->dev_id, subdev, a1 + line, &dont_care, &w1);
  } else if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO2) {
    base0 = ing_0_egr_1 ? tof2_mem_pipes_parde_e_prsr_mem_ml_tcam_row_address
                        : tof2_mem_pipes_parde_i_prsr_mem_ml_tcam_row_address;
    pipe_step = tof2_mem_pipes_array_element_size;
    prsr_step = ing_0_egr_1
                    ? tof2_mem_pipes_parde_e_prsr_mem_array_element_size
                    : tof2_mem_pipes_parde_i_prsr_mem_array_element_size;
    a0 = (base0 + phy_pipe * pipe_step + prsr_id * prsr_step) >> 4;
    lld_subdev_ind_read(dev_info->dev_id, subdev, a0 + line, &w1, &w0);
  } else if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO3) {
    base0 = ing_0_egr_1 ? tof3_mem_pipes_parde_e_prsr_mem_ml_tcam_row_address
                        : tof3_mem_pipes_parde_i_prsr_mem_ml_tcam_row_address;
    pipe_step = tof3_mem_pipes_array_element_size;
    prsr_step = ing_0_egr_1
                    ? tof3_mem_pipes_parde_e_prsr_mem_array_element_size
                    : tof3_mem_pipes_parde_i_prsr_mem_array_element_size;
    a0 = (base0 + phy_pipe * pipe_step + prsr_id * prsr_step) >> 4;
    lld_subdev_ind_read(dev_info->dev_id, subdev, a0 + line, &w1, &w0);
  } else {
    aim_printf(pvs, "Chip type not implemented\n");
    return;
  }
  /* If a bit is zero in both W0 and W1 the entry is invalid (won't match). */
  bool invalid = ((w0 | w1) & 0xFFFFFFFFFFF) != 0xFFFFFFFFFFF;
  /* For 1 bit values index with w0 and w1 bits to get decoded value. */
  char encoding[2][2] = {{'?', '1'}, {'0', 'X'}};
  /* When decoding state and lookup don't worry about invalid entries (0/0)
   * since we won't display them anyways. */
  uint8_t state_key = ~(w0 >> 32) & 0xFF;
  uint8_t state_mask = ((w0 ^ w1) >> 32) & 0xFF;
  uint32_t lkup_key = ~w0 & 0xFFFFFFFF;
  uint32_t lkup_mask = (w0 ^ w1) & 0xFFFFFFFF;
  if (invalid)
    aim_printf(pvs,
               "pipe %d %cprsr[%2d][%3d]: %11" PRIx64 " %11" PRIx64
               " Invalid\n",
               phy_pipe,
               ing_0_egr_1 ? 'e' : 'i',
               prsr_id,
               line,
               w0,
               w1);
  else
    aim_printf(pvs,
               "pipe %d %cprsr[%2d][%3d]: %11" PRIx64 " %11" PRIx64
               " Ver %c%c Neg %c Zero %c State %02x/%02x Lkup %08x/%08x\n",
               phy_pipe,
               ing_0_egr_1 ? 'e' : 'i',
               prsr_id,
               line,
               w0,
               w1,
               encoding[(w0 >> 43) & 1][(w1 >> 43) & 1],
               encoding[(w0 >> 42) & 1][(w1 >> 42) & 1],
               encoding[(w0 >> 41) & 1][(w1 >> 41) & 1],
               encoding[(w0 >> 40) & 1][(w1 >> 40) & 1],
               state_key,
               state_mask,
               lkup_key,
               lkup_mask);
}

static void prsr_tcam_rd(aim_pvs_t *pvs,
                         bf_dev_id_t dev_id,
                         bf_dev_pipe_t log_pipe,
                         bf_dev_pipe_t end_log_pipe,
                         int prsr_id,
                         int end_prsr_id,
                         int prsr_step,
                         int line,
                         int end_line,
                         bool ing_0_egr_1) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    aim_printf(pvs, "Device %d not found\n", dev_id);
    return;
  }

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      if (prsr_id < 0 || end_prsr_id >= 18 || line < 0 || end_line >= 256 ||
          end_log_pipe >= dev_info->num_active_pipes) {
        aim_printf(pvs, "Invalid arguments\n");
        return;
      }
      break;
    case BF_DEV_FAMILY_TOFINO2:
      if (prsr_id < 0 || end_prsr_id >= 36 || line < 0 || end_line >= 256 ||
          end_log_pipe >= dev_info->num_active_pipes) {
        aim_printf(pvs, "Invalid arguments\n");
        return;
      }
      break;
    case BF_DEV_FAMILY_TOFINO3:
      if (prsr_id < 0 || end_prsr_id >= 36 || line < 0 || end_line >= 256 ||
          end_log_pipe >= dev_info->num_active_pipes) {
        aim_printf(pvs, "Invalid arguments\n");
        return;
      }
      break;

    case BF_DEV_FAMILY_UNKNOWN:
    default:
      aim_printf(pvs, "Invalid device family\n");
      return;
  }

  for (bf_dev_pipe_t pipe = log_pipe; pipe <= end_log_pipe; ++pipe) {
    bf_dev_pipe_t phy_pipe = log_pipe;
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);
    for (int prsr = prsr_id; prsr <= end_prsr_id; prsr += prsr_step) {
      for (int l = line; l <= end_line; ++l) {
        prsr_tcam_rd_one(pvs, dev_info, phy_pipe, ing_0_egr_1, prsr, l);
      }
    }
  }
}

PIPE_MGR_CLI_CMD_DECLARE(prsr_tcam_rd) {
  int c;
  PIPE_MGR_CLI_PROLOGUE("prsr-tcam-rd",
                        " Read the parser TCAM.",
                        "-d <dev_id> -p <log_pipe>\n\t\t\t-g <0:ingress "
                        "1:egress> -i <prsr id> -s <prsr step> -l "
                        "<line>\n\t\t\t[-P <end log_pipe> -I <end prsr_id> -L "
                        "<end line>]");
  bf_dev_id_t dev_id = 0;
  bf_dev_pipe_t log_pipe = 0, end_log_pipe = 0;
  bool gress = false;
  int prsr_id = 0, end_prsr_id = 0;
  int prsr_step = 1;
  int line = 0, end_line = 0;
  bool got_end_pipe = false, got_end_prsr_id = false, got_end_line = false;

  while ((c = getopt(argc, argv, "d:p:g:i:s:l:P:I:L:")) != -1) {
    switch (c) {
      case 'd':
        dev_id = strtoul(optarg, NULL, 0);
        break;
      case 'p':
        log_pipe = strtoul(optarg, NULL, 0);
        break;
      case 'g':
        gress = strtoul(optarg, NULL, 0);
        break;
      case 'i':
        prsr_id = strtoul(optarg, NULL, 0);
        break;
      case 's':
        prsr_step = strtoul(optarg, NULL, 0);
        break;
      case 'l':
        line = strtoul(optarg, NULL, 0);
        break;
      case 'P':
        end_log_pipe = strtoul(optarg, NULL, 0);
        got_end_pipe = true;
        break;
      case 'I':
        end_prsr_id = strtoul(optarg, NULL, 0);
        got_end_prsr_id = true;
        break;
      case 'L':
        end_line = strtoul(optarg, NULL, 0);
        got_end_line = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }
  if (!got_end_pipe) end_log_pipe = log_pipe;
  if (!got_end_prsr_id) end_prsr_id = prsr_id;
  if (!got_end_line) end_line = line;
  if (end_log_pipe < log_pipe || end_prsr_id < prsr_id || end_line < line ||
      prsr_step < 1) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!VALID_PRSR_STEP(prsr_step)) {
    aim_printf(&uc->pvs,
               "Invalid arguments: prsr_step unsupported value: %d\n",
               prsr_step);
    return UCLI_STATUS_OK;
  }

  prsr_tcam_rd(&uc->pvs,
               dev_id,
               log_pipe,
               end_log_pipe,
               prsr_id,
               end_prsr_id,
               prsr_step,
               line,
               end_line,
               gress);

  return UCLI_STATUS_OK;
}

static void prsr_tcam_wr_one(aim_pvs_t *pvs,
                             rmt_dev_info_t *dev_info,
                             bf_dev_pipe_t phy_pipe,
                             bool ing_0_egr_1,
                             int prsr_id,
                             int line,
                             int ver,
                             int ver_mask,
                             int ctr_neg,
                             int ctr_neg_mask,
                             int ctr_zero,
                             int ctr_zero_mask,
                             int state,
                             int state_mask,
                             uint32_t data,
                             uint32_t data_mask) {
  uint64_t base0, base1, pipe_step, prsr_step, a0, a1, w0, w1;
  ver &= ver_mask;
  ver_mask = ~(ver ^ ver_mask);
  ver = ~ver;

  ctr_neg &= ctr_neg_mask;
  ctr_neg_mask = ~(ctr_neg ^ ctr_neg_mask);
  ctr_neg = ~ctr_neg;

  ctr_zero &= ctr_zero_mask;
  ctr_zero_mask = ~(ctr_zero ^ ctr_zero_mask);
  ctr_zero = ~ctr_zero;

  state &= state_mask;
  state_mask = ~(state ^ state_mask);
  state = ~state;

  data &= data_mask;
  data_mask = ~(data ^ data_mask);
  data = ~data;

  w0 = ver & 3;
  w0 = (w0 << 1) | (ctr_neg & 1);
  w0 = (w0 << 1) | (ctr_zero & 1);
  w0 = (w0 << 8) | (state & 0xFF);
  w0 = (w0 << 32) | (data & 0xFFFFFFFF);
  w1 = ver_mask & 3;
  w1 = (w1 << 1) | (ctr_neg_mask & 1);
  w1 = (w1 << 1) | (ctr_zero_mask & 1);
  w1 = (w1 << 8) | (state_mask & 0xFF);
  w1 = (w1 << 32) | (data_mask & 0xFFFFFFFF);

  bf_subdev_id_t subdev = pipe_mgr_subdev_id_from_pipe(phy_pipe);
  if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO) {
    base0 = ing_0_egr_1 ? pipe_top_level_pipes_e_prsr_ml_tcam_row_word0_address
                        : pipe_top_level_pipes_i_prsr_ml_tcam_row_word0_address;
    base1 = ing_0_egr_1 ? pipe_top_level_pipes_e_prsr_ml_tcam_row_word1_address
                        : pipe_top_level_pipes_i_prsr_ml_tcam_row_word1_address;
    pipe_step = pipe_top_level_pipes_array_element_size;
    prsr_step = ing_0_egr_1 ? pipe_top_level_pipes_e_prsr_array_element_size
                            : pipe_top_level_pipes_i_prsr_array_element_size;
    a0 = (base0 + phy_pipe * pipe_step + prsr_id * prsr_step) >> 4;
    a1 = (base1 + phy_pipe * pipe_step + prsr_id * prsr_step) >> 4;
    lld_subdev_ind_write(dev_info->dev_id, subdev, a0 + line, 0, w0);
    lld_subdev_ind_write(dev_info->dev_id, subdev, a1 + line, 0, w1);
  } else if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO2) {
    base0 = ing_0_egr_1 ? tof2_mem_pipes_parde_e_prsr_mem_ml_tcam_row_address
                        : tof2_mem_pipes_parde_i_prsr_mem_ml_tcam_row_address;
    pipe_step = tof2_mem_pipes_array_element_size;
    prsr_step = ing_0_egr_1
                    ? tof2_mem_pipes_parde_e_prsr_mem_array_element_size
                    : tof2_mem_pipes_parde_i_prsr_mem_array_element_size;
    a0 = (base0 + phy_pipe * pipe_step + prsr_id * prsr_step) >> 4;
    lld_subdev_ind_write(dev_info->dev_id, subdev, a0 + line, w1, w0);
  } else if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO3) {
    base0 = ing_0_egr_1 ? tof3_mem_pipes_parde_e_prsr_mem_ml_tcam_row_address
                        : tof3_mem_pipes_parde_i_prsr_mem_ml_tcam_row_address;
    pipe_step = tof3_mem_pipes_array_element_size;
    prsr_step = ing_0_egr_1
                    ? tof3_mem_pipes_parde_e_prsr_mem_array_element_size
                    : tof3_mem_pipes_parde_i_prsr_mem_array_element_size;
    a0 = (base0 + phy_pipe * pipe_step + prsr_id * prsr_step) >> 4;
    lld_subdev_ind_write(dev_info->dev_id, subdev, a0 + line, w1, w0);
  } else {
    aim_printf(pvs, "Chip type not implemented\n");
    return;
  }
}

static void prsr_tcam_wr(aim_pvs_t *pvs,
                         bf_dev_id_t dev_id,
                         bf_dev_pipe_t log_pipe,
                         bf_dev_pipe_t end_log_pipe,
                         int prsr_id,
                         int end_prsr_id,
                         int prsr_step,
                         int line,
                         int end_line,
                         bool ing_0_egr_1,
                         int ver,
                         int ver_mask,
                         int ctr_neg,
                         int ctr_neg_mask,
                         int ctr_zero,
                         int ctr_zero_mask,
                         int state,
                         int state_mask,
                         uint32_t data,
                         uint32_t data_mask) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    aim_printf(pvs, "Device %d not found\n", dev_id);
    return;
  }
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      if (prsr_id < 0 || end_prsr_id >= 18 || line < 0 || end_line >= 256 ||
          end_log_pipe >= dev_info->num_active_pipes) {
        aim_printf(pvs, "Invalid arguments\n");
        return;
      }
      break;
    case BF_DEV_FAMILY_TOFINO2:
      if (prsr_id < 0 || end_prsr_id >= 36 || line < 0 || end_line >= 256 ||
          end_log_pipe >= dev_info->num_active_pipes) {
        aim_printf(pvs, "Invalid arguments\n");
        return;
      }
      break;
    case BF_DEV_FAMILY_TOFINO3:
      if (prsr_id < 0 || end_prsr_id >= 36 || line < 0 || end_line >= 256 ||
          end_log_pipe >= dev_info->num_active_pipes) {
        aim_printf(pvs, "Invalid arguments\n");
        return;
      }
      break;

    case BF_DEV_FAMILY_UNKNOWN:
    default:
      aim_printf(pvs, "Invalid device family\n");
      return;
  }

  for (bf_dev_pipe_t pipe = log_pipe; pipe <= end_log_pipe; ++pipe) {
    bf_dev_pipe_t phy_pipe = log_pipe;
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);
    for (int prsr = prsr_id; prsr <= end_prsr_id; prsr += prsr_step) {
      for (int l = line; l <= end_line; ++l) {
        prsr_tcam_wr_one(pvs,
                         dev_info,
                         phy_pipe,
                         ing_0_egr_1,
                         prsr,
                         l,
                         ver,
                         ver_mask,
                         ctr_neg,
                         ctr_neg_mask,
                         ctr_zero,
                         ctr_zero_mask,
                         state,
                         state_mask,
                         data,
                         data_mask);
      }
    }
  }
}

PIPE_MGR_CLI_CMD_DECLARE(prsr_tcam_wr) {
  int c;
  PIPE_MGR_CLI_PROLOGUE(
      "prsr-tcam-wr",
      " Write the parser TCAM.",
      "-d <dev_id> -p <log_pipe>\n\t\t\t-g <0:ingress 1:egress> -i <prsr id> "
      "-s <prsr step> -l <line>\n\t\t\t[-P <end log_pipe> -I <end prsr_id> -L "
      "<end line>] -v <ver> -V<ver Mask>\n\t\t\t-n <ctr-neg> -N <ctr-neg mask> "
      "-z <ctr-zero> -Z <ctr-zero mask>\n\t\t\t-c <cur-state> -C <cur-state "
      "mask> -m <match data> -M <match data mask>");
  bf_dev_id_t dev_id = 0;
  bf_dev_pipe_t log_pipe = 0, end_log_pipe = 0;
  bool gress = false;
  int prsr_id = 0, end_prsr_id = 0;
  int prsr_step = 1;
  int line = 0, end_line = 0;
  int ver = 0, ver_mask = 0;
  int neg = 0, neg_mask = 0;
  int zero = 0, zero_mask = 0;
  int state = 0, state_mask = 0;
  uint32_t data = 0, data_mask = 0;
  bool got_end_pipe = false, got_end_prsr_id = false, got_end_line = false;
  bool got_ver = false, got_neg = false, got_zero = false, got_state = false,
       got_data = false;
  bool got_ver_mask = false, got_neg_mask = false, got_zero_mask = false,
       got_state_mask = false, got_data_mask = false;

  while ((c = getopt(argc, argv, "d:p:g:i:s:l:P:I:L:v:V:n:N:z:Z:c:C:m:M:")) !=
         -1) {
    switch (c) {
      case 'd':
        dev_id = strtoul(optarg, NULL, 0);
        break;
      case 'p':
        log_pipe = strtoul(optarg, NULL, 0);
        break;
      case 'g':
        gress = strtoul(optarg, NULL, 0);
        break;
      case 'i':
        prsr_id = strtoul(optarg, NULL, 0);
        break;
      case 's':
        prsr_step = strtoul(optarg, NULL, 0);
        break;
      case 'l':
        line = strtoul(optarg, NULL, 0);
        break;
      case 'P':
        end_log_pipe = strtoul(optarg, NULL, 0);
        got_end_pipe = true;
        break;
      case 'I':
        end_prsr_id = strtoul(optarg, NULL, 0);
        got_end_prsr_id = true;
        break;
      case 'L':
        end_line = strtoul(optarg, NULL, 0);
        got_end_line = true;
        break;
      case 'v':
        ver = strtoul(optarg, NULL, 0);
        got_ver = true;
        break;
      case 'V':
        ver_mask = strtoul(optarg, NULL, 0);
        got_ver_mask = true;
        break;
      case 'n':
        neg = strtoul(optarg, NULL, 0);
        got_neg = true;
        break;
      case 'N':
        neg_mask = strtoul(optarg, NULL, 0);
        got_neg_mask = true;
        break;
      case 'z':
        zero = strtoul(optarg, NULL, 0);
        got_zero = true;
        break;
      case 'Z':
        zero_mask = strtoul(optarg, NULL, 0);
        got_zero_mask = true;
        break;
      case 'c':
        state = strtoul(optarg, NULL, 0);
        got_state = true;
        break;
      case 'C':
        state_mask = strtoul(optarg, NULL, 0);
        got_state_mask = true;
        break;
      case 'm':
        data = strtoul(optarg, NULL, 0);
        got_data = true;
        break;
      case 'M':
        data_mask = strtoul(optarg, NULL, 0);
        got_data_mask = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }
  if (!got_end_pipe) end_log_pipe = log_pipe;
  if (!got_end_prsr_id) end_prsr_id = prsr_id;
  if (!got_end_line) end_line = line;
  if (got_ver && !got_ver_mask) ver_mask = 3;
  if (!got_ver) ver = ver_mask = 0;
  if (got_neg && !got_neg_mask) neg_mask = 1;
  if (!got_neg) neg = neg_mask = 0;
  if (got_zero && !got_zero_mask) zero_mask = 1;
  if (!got_zero) zero = zero_mask = 0;
  if (got_state && !got_state_mask) state_mask = 0xFF;
  if (!got_state) state = state_mask = 0;
  if (got_data && !got_data_mask) data_mask = 0xFFFFFFFF;
  if (!got_data) data = data_mask = 0;

  if (end_log_pipe < log_pipe || end_prsr_id < prsr_id || end_line < line ||
      prsr_step < 1) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!VALID_PRSR_STEP(prsr_step)) {
    aim_printf(&uc->pvs,
               "Invalid arguments: prsr_step unsupported value: %d\n",
               prsr_step);
    return UCLI_STATUS_OK;
  }

  prsr_tcam_wr(&uc->pvs,
               dev_id,
               log_pipe,
               end_log_pipe,
               prsr_id,
               end_prsr_id,
               prsr_step,
               line,
               end_line,
               gress,
               ver,
               ver_mask,
               neg,
               neg_mask,
               zero,
               zero_mask,
               state,
               state_mask,
               data,
               data_mask);

  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(write_reg) {
  int c, dflag, aflag, iflag, sflag;
  char *d_arg, *a_arg, *i_arg, *s_arg;
  bf_dev_id_t dev_id = 0;
  bf_subdev_id_t subdev_id = 0;
  uint32_t addr = 0, data = 0;
  char str[PIPE_MGR_HW_DUMP_STR_LEN];

  PIPE_MGR_CLI_PROLOGUE("write-reg",
                        " Write to registers.",
                        "-d <dev_id> -s <subdev_id> -a <reg_addr> -i <data>");

  dflag = aflag = iflag = sflag = 0;
  d_arg = a_arg = i_arg = s_arg = NULL;
  str[0] = '\0';

  while ((c = getopt(argc, argv, "d:s:a:i:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        d_arg = optarg;
        if (!d_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 's':
        sflag = 1;
        s_arg = optarg;
        if (!s_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'a':
        aflag = 1;
        a_arg = optarg;
        if (!a_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'i':
        iflag = 1;
        i_arg = optarg;
        if (!i_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (dflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else if (dflag == 1) {
    dev_id = strtoul(d_arg, NULL, 0);
    if (dev_id > (PIPE_MGR_NUM_DEVICES - 1)) {
      aim_printf(&uc->pvs, "tbl: Invalid dev_id %s\n", d_arg);
      return UCLI_STATUS_OK;
    }
  }

  if (sflag == 1) {
    subdev_id = strtoul(s_arg, NULL, 0);
    if (subdev_id > (BF_MAX_SUBDEV_COUNT - 1)) {
      aim_printf(&uc->pvs, "tbl: Invalid dev_id %s\n", d_arg);
      return UCLI_STATUS_OK;
    }
  }

  if (aflag == 1) {
    addr = strtoul(a_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (iflag == 1) {
    data = strtoul(i_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  pipe_mgr_dbg_write_register(dev_id, subdev_id, addr, data, str, sizeof(str));

  aim_printf(&uc->pvs, "\n%s\n", str);
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(show_act_tbl_vaddr) {
  int c, dflag, aflag, hflag, pflag, sflag, fflag;
  char *d_arg, *a_arg, *h_arg, *p_arg, *s_arg, *f_arg;
  bf_dev_id_t dev_id = 0;
  uint8_t log_pipe = 0, stage = 0;
  uint32_t addr = 0;
  uint32_t tbl_hdl = 0, act_fn_hdl = 0;
  char str[PIPE_MGR_HW_DUMP_STR_LEN];

  PIPE_MGR_CLI_PROLOGUE("show-act-tbl-vaddr",
                        " Dump Action table by Virtual address",
                        "-d <dev_id> -a <vpn_addr> -h <tbl_hdl> -f "
                        "<act_fn_hdl> -p <log-pipe> -s <stage_id>");

  dflag = aflag = hflag = pflag = sflag = fflag = 0;
  d_arg = a_arg = h_arg = p_arg = s_arg = f_arg = NULL;
  str[0] = '\0';

  while ((c = getopt(argc, argv, "d:a:h:p:s:f:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        d_arg = optarg;
        if (!d_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'a':
        aflag = 1;
        a_arg = optarg;
        if (!a_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'h':
        hflag = 1;
        h_arg = optarg;
        if (!h_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'p':
        pflag = 1;
        p_arg = optarg;
        if (!p_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 's':
        sflag = 1;
        s_arg = optarg;
        if (!s_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'f':
        fflag = 1;
        f_arg = optarg;
        if (!f_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (dflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else if (dflag == 1) {
    dev_id = strtoul(d_arg, NULL, 0);
    if (dev_id > (PIPE_MGR_NUM_DEVICES - 1)) {
      aim_printf(&uc->pvs, "tbl: Invalid dev_id %s\n", d_arg);
      return UCLI_STATUS_OK;
    }
  }

  if (aflag == 1) {
    addr = strtoul(a_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (hflag == 1) {
    tbl_hdl = strtoul(h_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (pflag == 1) {
    log_pipe = strtoul(p_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (sflag == 1) {
    stage = strtoul(s_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (fflag == 1) {
    act_fn_hdl = strtoul(f_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  pipe_mgr_dump_act_tbl_by_virtaddr(
      dev_id, addr, tbl_hdl, log_pipe, stage, act_fn_hdl, str, sizeof(str));

  aim_printf(&uc->pvs, "\n%s\n", str);
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(show_mat_tbl_vaddr) {
  int c, dflag, aflag, hflag, pflag, sflag, tflag;
  char *d_arg, *a_arg, *h_arg, *p_arg, *s_arg, *t_arg;
  bf_dev_id_t dev_id = 0;
  uint8_t log_pipe = 0, stage = 0;
  uint32_t addr = 0;
  uint32_t tbl_hdl = 0;
  int is_tind = 0;
  char str[PIPE_MGR_HW_DUMP_STR_LEN];

  PIPE_MGR_CLI_PROLOGUE(
      "show-mat-tbl-vaddr",
      " Dump Match table by Virtual address",
      "-d <dev_id> -a <vpn_addr> -h <tbl_hdl> -p <log_pipe_id> "
      "-s <stage_id> [-t <is_tind>] [-l <logical_table_id>]");

  dflag = aflag = hflag = pflag = sflag = tflag = 0;
  d_arg = a_arg = h_arg = p_arg = s_arg = t_arg = NULL;
  str[0] = '\0';

  uint8_t stage_table_handle = 0;

  while ((c = getopt(argc, argv, "d:a:h:p:s:t:l:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        d_arg = optarg;
        if (!d_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'a':
        aflag = 1;
        a_arg = optarg;
        if (!a_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'h':
        hflag = 1;
        h_arg = optarg;
        if (!h_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'p':
        pflag = 1;
        p_arg = optarg;
        if (!p_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 's':
        sflag = 1;
        s_arg = optarg;
        if (!s_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 't':
        tflag = 1;
        t_arg = optarg;
        if (!t_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'l':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        stage_table_handle = strtoull(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (dflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else if (dflag == 1) {
    dev_id = strtoul(d_arg, NULL, 0);
    if (dev_id > (PIPE_MGR_NUM_DEVICES - 1)) {
      aim_printf(&uc->pvs, "tbl: Invalid dev_id %s\n", d_arg);
      return UCLI_STATUS_OK;
    }
  }

  if (aflag == 1) {
    addr = strtoul(a_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (hflag == 1) {
    tbl_hdl = strtoul(h_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (pflag == 1) {
    log_pipe = strtoul(p_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (sflag == 1) {
    stage = strtoul(s_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (tflag == 1) {
    is_tind = strtoul(t_arg, NULL, 0);
  }

  pipe_mgr_dump_mat_tbl_by_virtaddr(dev_id,
                                    addr,
                                    tbl_hdl,
                                    log_pipe,
                                    stage,
                                    stage_table_handle,
                                    is_tind,
                                    str,
                                    sizeof(str));

  aim_printf(&uc->pvs, "\n%s\n", str);
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(shadow_mem) {
  int c, dflag, pflag, sflag, mflag, tflag, lflag, gflag;
  char *d_arg, *p_arg, *s_arg, *m_arg, *t_arg, *l_arg, *g_arg;
  bf_dev_id_t dev_id = 0;
  bf_dev_pipe_t log_pipe_id = 0;
  uint8_t stage_id = 0, mem_type = 0, gress = 0;
  mem_id_t mem_id = 0;
  uint16_t line_no = 0;
  bool all_lines = false;
  char str[PIPE_MGR_SHADOW_MEM_DUMP_STR_LEN];

  PIPE_MGR_CLI_PROLOGUE("shadow-mem",
                        " Dump shadow memory.",
                        "-d <dev_id> -p <log_pipe_id> -s"
                        "<stage_id> -m <mem_id> -t "
                        "<mem_type> [-l <line_no> -g <in(0)/egress(1)>]");

  dflag = pflag = sflag = mflag = tflag = lflag = gflag = 0;
  d_arg = p_arg = s_arg = m_arg = t_arg = l_arg = g_arg = NULL;
  str[0] = '\0';

  while ((c = getopt(argc, argv, "d:p:s:m:t:l:g:")) != -1) {
    switch (c) {
      case 'g':
        gflag = 1;
        g_arg = optarg;
        if (!g_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'd':
        dflag = 1;
        d_arg = optarg;
        if (!d_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'p':
        pflag = 1;
        p_arg = optarg;
        if (!p_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 's':
        sflag = 1;
        s_arg = optarg;
        if (!s_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'm':
        mflag = 1;
        m_arg = optarg;
        if (!m_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 't':
        tflag = 1;
        t_arg = optarg;
        if (!t_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'l':
        lflag = 1;
        l_arg = optarg;
        if (!l_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (dflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else if (dflag == 1) {
    dev_id = strtoul(d_arg, NULL, 0);
    if (dev_id > (PIPE_MGR_NUM_DEVICES - 1)) {
      aim_printf(&uc->pvs, "Invalid dev_id %s\n", d_arg);
      return UCLI_STATUS_OK;
    }
  }
  if (gflag == 1) {
    gress = strtoul(g_arg, NULL, 0);
    if ((gress != 0) && (gress != 1)) {
      aim_printf(&uc->pvs, "Invalid direction, 0:ingress, 1:egress");
      return UCLI_STATUS_OK;
    }
  } else {
    /* By default pass gress as ingress.if
     * its tf1,tf2,tf3 it doesn't matter.
     */
    gress = 0;
  }
  if (pflag == 1) {
    log_pipe_id = strtoul(p_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (sflag == 1) {
    stage_id = strtoul(s_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (mflag == 1) {
    mem_id = strtoul(m_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (tflag == 1) {
    mem_type = strtoul(t_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (lflag == 1) {
    all_lines = false;
    line_no = strtoul(l_arg, NULL, 0);
  } else {
    all_lines = true;
  }

  pipe_mgr_dump_phy_shadow_memory(dev_id,
                                  gress,
                                  log_pipe_id,
                                  stage_id,
                                  mem_id,
                                  mem_type,
                                  line_no,
                                  all_lines,
                                  str,
                                  sizeof(str));

  aim_printf(&uc->pvs, "\n%s\n", str);
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(phv_dump) {
  int c, dflag, pflag, sflag, iflag;
  char *d_arg, *p_arg, *s_arg, *i_arg;
  rmt_dev_info_t *dev_info;
  bf_dev_id_t dev_id = 0;
  bf_dev_pipe_t log_pipe_id = 0;
  uint8_t stage_id = 0, direction = 0;
  char str[PIPE_MGR_HW_DUMP_STR_LEN];

  PIPE_MGR_CLI_PROLOGUE(
      "phv-dump",
      " Dump PHV allocation",
      "-d <dev_id> -p <log_pipe_id> -s <stage_id> -i <direction>");

  dflag = pflag = sflag = iflag = 0;
  d_arg = p_arg = s_arg = i_arg = NULL;
  str[0] = '\0';

  while ((c = getopt(argc, argv, "d:p:s:i:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        d_arg = optarg;
        if (!d_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'p':
        pflag = 1;
        p_arg = optarg;
        if (!p_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 's':
        sflag = 1;
        s_arg = optarg;
        if (!s_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'i':
        iflag = 1;
        i_arg = optarg;
        if (!i_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (dflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else if (dflag == 1) {
    dev_id = strtoul(d_arg, NULL, 0);
    dev_info = pipe_mgr_get_dev_info(dev_id);
    if (!dev_info) {
      aim_printf(&uc->pvs, "Invalid dev_id %s\n", d_arg);
      return UCLI_STATUS_OK;
    }
  }

  if (pflag == 1) {
    log_pipe_id = strtoul(p_arg, NULL, 0);
    if (log_pipe_id >= dev_info->num_active_pipes) {
      aim_printf(&uc->pvs, "Invalid pipe");
      return UCLI_STATUS_OK;
    }
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (sflag == 1) {
    stage_id = strtoul(s_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (iflag == 1) {
    direction = strtoul(i_arg, NULL, 0);
    if ((direction != 0) && (direction != 1)) {
      aim_printf(&uc->pvs, "Invalid direction, 0:ingress, 1:egress");
      return UCLI_STATUS_OK;
    }
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  pipe_mgr_phv_allocation_dump(
      dev_id, log_pipe_id, stage_id, direction, str, sizeof(str));

  aim_printf(&uc->pvs, "\n%s\n", str);
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(snap_create) {
  int c, dflag, pflag, sflag, iflag, eflag;
  char *d_arg, *p_arg, *s_arg, *i_arg, *e_arg;
  bf_dev_id_t dev_id = 0;
  bf_dev_pipe_t pipe_id = 0;
  uint8_t start_stage = 0, end_stage = 0, direction = 0;
  pipe_snapshot_hdl_t hdl = 0;
  pipe_status_t status = PIPE_SUCCESS;

  PIPE_MGR_CLI_PROLOGUE(
      "snap-create",
      " Create a snapshot",
      "-d <dev_id> -p <pipe_id: all-pipes=0xFFFF> -s "
      "<start_stage> -e <end_stage> -i <direction 0:ingress 1:egress>");

  dflag = pflag = sflag = iflag = eflag = 0;
  d_arg = p_arg = s_arg = i_arg = e_arg = NULL;

  while ((c = getopt(argc, argv, "d:p:s:i:e:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        d_arg = optarg;
        if (!d_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'p':
        pflag = 1;
        p_arg = optarg;
        if (!p_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 's':
        sflag = 1;
        s_arg = optarg;
        if (!s_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'i':
        iflag = 1;
        i_arg = optarg;
        if (!i_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'e':
        eflag = 1;
        e_arg = optarg;
        if (!e_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (dflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else if (dflag == 1) {
    dev_id = strtoul(d_arg, NULL, 0);
    if (dev_id > (PIPE_MGR_NUM_DEVICES - 1)) {
      aim_printf(&uc->pvs, "Invalid dev_id %s\n", d_arg);
      return UCLI_STATUS_OK;
    }
  }

  if (pflag == 1) {
    pipe_id = strtoul(p_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (sflag == 1) {
    start_stage = strtoul(s_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (iflag == 1) {
    direction = strtoul(i_arg, NULL, 0);
    if ((direction != 0) && (direction != 1)) {
      aim_printf(&uc->pvs, "Invalid direction, 0:ingress, 1:egress");
      return UCLI_STATUS_OK;
    }
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  if (eflag == 1) {
    end_stage = strtoul(e_arg, NULL, 0);
    if (end_stage < start_stage) {
      aim_printf(&uc->pvs, "End stage cant be less than start stage");
      return UCLI_STATUS_OK;
    }
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  status = bf_snapshot_create(
      dev_id, pipe_id, start_stage, end_stage, direction, &hdl);
  if (status == PIPE_SUCCESS) {
    aim_printf(&uc->pvs, "Snapshot created with handle 0x%x \n", hdl);
  } else {
    aim_printf(&uc->pvs, "Snapshot creation failed with error %d \n", status);
  }

  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(snap_delete) {
  int c, hflag;
  char *h_arg;
  pipe_snapshot_hdl_t hdl = 0;
  pipe_status_t status = PIPE_SUCCESS;

  PIPE_MGR_CLI_PROLOGUE("snap-delete", " Delete a snapshot", "-h <handle>");

  hflag = 0;
  h_arg = NULL;

  while ((c = getopt(argc, argv, "h:")) != -1) {
    switch (c) {
      case 'h':
        hflag = 1;
        h_arg = optarg;
        if (!h_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (hflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else if (hflag == 1) {
    hdl = strtoul(h_arg, NULL, 0);
  }

  status = bf_snapshot_delete(hdl);
  if (status == PIPE_SUCCESS) {
    aim_printf(
        &uc->pvs, "Snapshot with handle 0x%x successfuly deleted \n", hdl);
  } else {
    aim_printf(&uc->pvs,
               "Snapshot deletion (handle 0x%x) failed with error %d \n",
               hdl,
               status);
  }

  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(snap_trig_add) {
  int c, hflag, nflag, vflag, mflag;
  char *h_arg, *n_arg, *v_arg, *m_arg;
  pipe_snapshot_hdl_t hdl = 0;
  uint64_t value = 0, mask = 0;
  char name[PIPE_SNAP_TRIG_FIELD_NAME_LEN];
  pipe_status_t status = PIPE_SUCCESS;

  PIPE_MGR_CLI_PROLOGUE("snap-trig-add",
                        " Add a field to snapshot trigger",
                        "-h <handle> -n <field_name> -v <value> -m <mask>");

  hflag = nflag = vflag = mflag = 0;
  h_arg = n_arg = v_arg = m_arg = NULL;
  PIPE_MGR_MEMSET(name, 0, PIPE_SNAP_TRIG_FIELD_NAME_LEN);

  while ((c = getopt(argc, argv, "h:n:v:m:")) != -1) {
    switch (c) {
      case 'h':
        hflag = 1;
        h_arg = optarg;
        if (!h_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'n':
        nflag = 1;
        n_arg = optarg;
        if (!n_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'v':
        vflag = 1;
        v_arg = optarg;
        if (!v_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'm':
        mflag = 1;
        m_arg = optarg;
        if (!m_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (hflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else if (hflag == 1) {
    hdl = strtoul(h_arg, NULL, 0);
  }

  if (nflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else if (nflag == 1) {
    strncpy(name, n_arg, PIPE_SNAP_TRIG_FIELD_NAME_LEN - 1);
  }

  if (vflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else if (vflag == 1) {
    value = strtoull(v_arg, NULL, 0);
  }

  if (mflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else if (mflag == 1) {
    mask = strtoull(m_arg, NULL, 0);
  }

  aim_printf(&uc->pvs,
             "\nTrigger: Adding Field %s, value 0x%" PRIx64 ", mask 0x%" PRIx64
             "\n",
             name,
             value,
             mask);

  status = bf_snapshot_capture_trigger_field_add(hdl, name, value, mask);
  aim_printf(&uc->pvs,
             "%s field %s to trigger \n",
             (status == PIPE_SUCCESS) ? "Success in adding" : "Failed to add",
             name);

  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(snap_trig_clr) {
  int c, hflag;
  char *h_arg;
  pipe_snapshot_hdl_t hdl = 0;
  pipe_status_t status = PIPE_SUCCESS;

  PIPE_MGR_CLI_PROLOGUE(
      "snap-trig-clr", " Delete all fields of snapshot trigger", "-h <handle>");

  hflag = 0;
  h_arg = NULL;

  while ((c = getopt(argc, argv, "h:")) != -1) {
    switch (c) {
      case 'h':
        hflag = 1;
        h_arg = optarg;
        if (!h_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (hflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else if (hflag == 1) {
    hdl = strtoul(h_arg, NULL, 0);
  }

  status = bf_snapshot_capture_trigger_fields_clr(hdl);
  aim_printf(
      &uc->pvs,
      "%s all trigger fields for handle 0x%x\n",
      (status == PIPE_SUCCESS) ? "Success in clearing" : "Failed to clear",
      hdl);

  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(snap_intr_clr) {
  int c, hflag, pflag, sflag;
  char *h_arg, *p_arg, *s_arg;
  pipe_snapshot_hdl_t hdl = 0;
  dev_stage_t stage = PIPE_MGR_SNAP_STAGE_INVALID;
  bf_dev_pipe_t pipe_id = BF_DEV_PIPE_ALL;

  PIPE_MGR_CLI_PROLOGUE("snap-intr-clr",
                        " Clear snapshot interrupts",
                        "-h <handle> [-p <pipe_id> -s <stage_id>]");

  hflag = pflag = sflag = 0;
  h_arg = p_arg = s_arg = NULL;

  while ((c = getopt(argc, argv, "h:p:s:")) != -1) {
    switch (c) {
      case 'h':
        hflag = 1;
        h_arg = optarg;
        if (!h_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'p':
        pflag = 1;
        p_arg = optarg;
        if (!p_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 's':
        sflag = 1;
        s_arg = optarg;
        if (!s_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (hflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else if (hflag == 1) {
    hdl = strtoul(h_arg, NULL, 0);
  }

  if (pflag == 1) {
    pipe_id = strtoul(p_arg, NULL, 0);
  }
  if (sflag == 1) {
    stage = strtoul(s_arg, NULL, 0);
  }

  bf_snapshot_interrupt_clear(hdl, pipe_id, stage);
  aim_printf(&uc->pvs, "Snapshot interrupt cleared for handle 0x%x \n", hdl);

  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(snap_state_set) {
  int c, hflag, eflag, tflag;
  char *h_arg, *e_arg, *t_arg;
  bf_snapshot_state_t en_state = 0;
  pipe_snapshot_hdl_t hdl = 0;
  uint32_t usec = 0;

  PIPE_MGR_CLI_PROLOGUE(
      "snap-state-set",
      " Enable/Disable Snapshot state",
      "-h <handle> -e <enable (0:disable, 1:enable)> [-t <timeout_usec>]");

  hflag = eflag = tflag = 0;
  h_arg = e_arg = t_arg = NULL;

  while ((c = getopt(argc, argv, "h:e:t:")) != -1) {
    switch (c) {
      case 'h':
        hflag = 1;
        h_arg = optarg;
        if (!h_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'e':
        eflag = 1;
        e_arg = optarg;
        if (!e_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 't':
        tflag = 1;
        t_arg = optarg;
        if (!t_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (hflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else if (hflag == 1) {
    hdl = strtoul(h_arg, NULL, 0);
  }

  if (eflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else if (eflag == 1) {
    en_state = strtoul(e_arg, NULL, 0);
    if (en_state > 1) {
      aim_printf(&uc->pvs, "Invalid state (0:disable, 1:enable)\n");
      return UCLI_STATUS_OK;
    }
  }

  if (tflag == 1) {
    usec = strtoul(t_arg, NULL, 0);
  }

  bf_snapshot_state_set(hdl, en_state, usec);
  aim_printf(&uc->pvs, "Snapshot state set to %d \n", en_state);

  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(snap_timer_en) {
  int c, hflag, eflag;
  char *h_arg, *e_arg;
  bool enable = false;
  pipe_snapshot_hdl_t hdl = 0;
  bf_snapshot_ig_mode_t mode = 0;
  uint32_t usec;

  PIPE_MGR_CLI_PROLOGUE("snap-timer-en",
                        " Enable/Disable Snapshot timer",
                        "-h <handle> -e <enable> (0: disable, 1: enable)");

  hflag = eflag = 0;
  h_arg = e_arg = NULL;

  while ((c = getopt(argc, argv, "h:e:")) != -1) {
    switch (c) {
      case 'h':
        hflag = 1;
        h_arg = optarg;
        if (!h_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'e':
        eflag = 1;
        e_arg = optarg;
        if (!e_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (hflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else if (hflag == 1) {
    hdl = strtoul(h_arg, NULL, 0);
  }

  bf_snapshot_cfg_get(hdl, &enable, &usec, &mode);
  if (eflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else if (eflag == 1) {
    uint32_t tmp = strtoul(e_arg, NULL, 0);
    if (tmp != 0 && tmp != 1) {
      aim_printf(&uc->pvs, "Invalid enable state (0:disable, 1:enable)");
      return UCLI_STATUS_OK;
    }
    enable = (bool)tmp;
  }
  bf_snapshot_cfg_set(hdl, !enable, mode);
  aim_printf(&uc->pvs, "Snapshot timer state set to %d \n", enable);

  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(snap_state_get) {
  int c, hflag, pflag, sflag;
  char *h_arg, *p_arg, *s_arg;
  bf_dev_pipe_t pipe_id = BF_DEV_PIPE_ALL;
  dev_stage_t stage = PIPE_MGR_SNAP_STAGE_INVALID;
  pipe_snapshot_hdl_t hdl = 0;
  char str[PIPE_MGR_HW_DUMP_STR_LEN];

  PIPE_MGR_CLI_PROLOGUE("snap-state-get",
                        " Show Snapshot state in ASIC",
                        "-h <handle> [-p <pipe_id> -s <stage_id>]");

  hflag = pflag = sflag = 0;
  h_arg = p_arg = s_arg = NULL;
  str[0] = '\0';

  while ((c = getopt(argc, argv, "h:p:s:")) != -1) {
    switch (c) {
      case 'h':
        hflag = 1;
        h_arg = optarg;
        if (!h_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'p':
        pflag = 1;
        p_arg = optarg;
        if (!p_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 's':
        sflag = 1;
        s_arg = optarg;
        if (!s_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (hflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else if (hflag == 1) {
    hdl = strtoul(h_arg, NULL, 0);
  }

  if (pflag == 1) {
    pipe_id = strtoul(p_arg, NULL, 0);
  }
  if (sflag == 1) {
    stage = strtoul(s_arg, NULL, 0);
  }

  pipe_mgr_snapshot_state_show(hdl, pipe_id, stage, str, sizeof(str));

  aim_printf(&uc->pvs, "\n%s\n", str);
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(snap_capture_get) {
  int c, hflag, pflag, sflag;
  char *h_arg, *p_arg, *s_arg;
  bf_dev_pipe_t pipe_id = BF_DEV_PIPE_ALL;
  dev_stage_t stage = PIPE_MGR_SNAP_STAGE_INVALID;
  pipe_snapshot_hdl_t hdl = 0;
  char *str = NULL;
  uint32_t buflen = sizeof(char) * PIPE_MGR_SNAPSHOT_HW_DUMP_STR_LEN;

  PIPE_MGR_CLI_PROLOGUE("snap-capture-get",
                        " Show Snapshot capture from Asic",
                        "-h <handle> [-p <pipe_id> -s <stage_id>]");

  hflag = pflag = sflag = 0;
  h_arg = p_arg = s_arg = NULL;

  while ((c = getopt(argc, argv, "h:p:s:")) != -1) {
    switch (c) {
      case 'h':
        hflag = 1;
        h_arg = optarg;
        if (!h_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'p':
        pflag = 1;
        p_arg = optarg;
        if (!p_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 's':
        sflag = 1;
        s_arg = optarg;
        if (!s_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (hflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else if (hflag == 1) {
    hdl = strtoul(h_arg, NULL, 0);
  }

  if (pflag == 1) {
    pipe_id = strtoul(p_arg, NULL, 0);
    if (pipe_id != BF_DEV_PIPE_ALL && BF_PIPE_COUNT <= pipe_id) {
      aim_printf(&uc->pvs, "Invalid pipe_id %s\n", p_arg);
      return UCLI_STATUS_OK;
    }
  }
  if (sflag == 1) {
    stage = strtoul(s_arg, NULL, 0);
  }

  str = (char *)bf_sys_malloc(buflen);
  if (!str) {
    aim_printf(&uc->pvs, "Out of memory");
    return UCLI_STATUS_OK;
  }
  str[0] = '\0';

  pipe_mgr_snapshot_capture_show(hdl, pipe_id, stage, str, buflen);
  /* If we ran out of buffer, retry with a larger buffer size */
  if ((strlen(str) + 300) > buflen) {
    /* Free existing memory */
    bf_sys_free(str);
    /* Increase memory by 50% */
    buflen += buflen / 2;
    str = (char *)bf_sys_malloc(buflen);
    if (!str) {
      aim_printf(&uc->pvs, "Out of memory");
      return UCLI_STATUS_OK;
    }
    str[0] = '\0';
    pipe_mgr_snapshot_capture_show(hdl, pipe_id, stage, str, buflen);
  }

  aim_printf(&uc->pvs, "\n%s\n", str);
  bf_sys_free(str);
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(snap_hdl_dump) {
  int c, dflag, hflag;
  char *d_arg, *h_arg;
  bf_dev_id_t dev_id = 0;
  pipe_snapshot_hdl_t hdl = 0;
  char str[PIPE_MGR_HW_DUMP_STR_LEN];

  PIPE_MGR_CLI_PROLOGUE("snap-hdl-dump",
                        " Dump all Snapshot handles on device",
                        "-d <dev_id> [-h <handle>]");

  dflag = hflag = 0;
  d_arg = h_arg = NULL;
  str[0] = '\0';

  while ((c = getopt(argc, argv, "d:h:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        d_arg = optarg;
        if (!d_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'h':
        hflag = 1;
        h_arg = optarg;
        if (!h_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (dflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else if (dflag == 1) {
    dev_id = strtoul(d_arg, NULL, 0);
  }

  if (hflag == 1) {
    hdl = strtoul(h_arg, NULL, 0);
  }

  pipe_mgr_snapshot_hdls_dump(dev_id, hdl, str, sizeof(str));

  aim_printf(&uc->pvs, "\n%s\n", str);
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(snap_ig_mode_set) {
  int c, mflag, hflag;
  char *m_arg, *h_arg;
  bf_snapshot_ig_mode_t mode = 0;
  pipe_snapshot_hdl_t hdl = 0;
  bool enable = false;
  uint32_t usec;

  PIPE_MGR_CLI_PROLOGUE("snap-ig-mode-set",
                        " Set snapshot ingress trigger mode on device",
                        "-h <handle> -m <0:ingress, 1:ghost, 2:both, 3:any>");

  mflag = hflag = 0;
  m_arg = h_arg = NULL;

  while ((c = getopt(argc, argv, "h:m:")) != -1) {
    switch (c) {
      case 'm':
        mflag = 1;
        m_arg = optarg;
        if (!m_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'h':
        hflag = 1;
        h_arg = optarg;
        if (!h_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (mflag == 0 || hflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  hdl = strtoul(h_arg, NULL, 0);
  bf_snapshot_cfg_get(hdl, &enable, &usec, &mode);
  mode = strtoul(m_arg, NULL, 0);

  bf_snapshot_cfg_set(hdl, !enable, mode);
  aim_printf(&uc->pvs, "Snapshot ingress trigger mode set to %d \n", mode);

  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(snap_cfg_dump) {
  int c, hflag, pflag, sflag;
  char *h_arg, *p_arg, *s_arg;
  bf_dev_pipe_t pipe_id = BF_DEV_PIPE_ALL;
  dev_stage_t stage = PIPE_MGR_SNAP_STAGE_INVALID;
  pipe_snapshot_hdl_t hdl = 0;
  char str[PIPE_MGR_HW_DUMP_STR_LEN];

  PIPE_MGR_CLI_PROLOGUE("snap-cfg-dump",
                        " Show Snapshot config",
                        "-h <handle> [-p <pipe_id> -s <stage_id>]");

  hflag = pflag = sflag = 0;
  h_arg = p_arg = s_arg = NULL;
  str[0] = '\0';

  while ((c = getopt(argc, argv, "h:p:s:")) != -1) {
    switch (c) {
      case 'h':
        hflag = 1;
        h_arg = optarg;
        if (!h_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'p':
        pflag = 1;
        p_arg = optarg;
        if (!p_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 's':
        sflag = 1;
        s_arg = optarg;
        if (!s_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (hflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else if (hflag == 1) {
    hdl = strtoul(h_arg, NULL, 0);
  }

  if (pflag == 1) {
    pipe_id = strtoul(p_arg, NULL, 0);
    if (pipe_id != BF_DEV_PIPE_ALL && BF_PIPE_COUNT <= pipe_id) {
      aim_printf(&uc->pvs, "Invalid pipe_id %s\n", p_arg);
      return UCLI_STATUS_OK;
    }
  }
  if (sflag == 1) {
    stage = strtoul(s_arg, NULL, 0);
  }

  pipe_mgr_snapshot_config_dump(hdl, pipe_id, stage, str, sizeof(str));

  aim_printf(&uc->pvs, "\n%s\n", str);
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(tbl_cntr_type_set) {
  int c, dflag, pflag, sflag, nflag, tflag, aflag;
  char *d_arg, *p_arg, *s_arg, *n_arg, *t_arg, *a_arg;
  bf_dev_id_t dev_id = 0;
  bf_dev_pipe_t log_pipe_id = 0;
  dev_stage_t stage = 0;
  int type;
  char tbl_name[PIPE_MGR_TBL_NAME_LEN];
  dev_target_t dev_tgt;
  rmt_dev_info_t *dev_info = NULL;

  PIPE_MGR_CLI_PROLOGUE(
      "tbl-cntr-type-set",
      " Set type for logical table counter",
      "-d <dev_id> -p <log_pipe_id> { -n <tbl_name> | -s <stage> "
      "| -a <all_stages> } -t <type: "
      "0=dis,1=tbl-miss,2=tbl-hit,3=gw-miss,4=gw-hit,5=gw-"
      "inhibit>");

  dflag = pflag = sflag = nflag = tflag = aflag = 0;
  d_arg = p_arg = s_arg = n_arg = t_arg = a_arg = NULL;
  PIPE_MGR_MEMSET(tbl_name, 0, PIPE_MGR_TBL_NAME_LEN);

  while ((c = getopt(argc, argv, "d:p:t:s:n:a:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        d_arg = optarg;
        if (!d_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'p':
        pflag = 1;
        p_arg = optarg;
        if (!p_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 's':
        sflag = 1;
        s_arg = optarg;
        if (!s_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'n':
        nflag = 1;
        n_arg = optarg;
        if (!n_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'a':
        aflag = 1;
        a_arg = optarg;
        if (!a_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 't':
        tflag = 1;
        t_arg = optarg;
        if (!t_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (dflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else if (dflag == 1) {
    dev_id = strtoul(d_arg, NULL, 0);
    if (dev_id > (PIPE_MGR_NUM_DEVICES - 1)) {
      aim_printf(&uc->pvs, "Invalid dev_id %s\n", d_arg);
      return UCLI_STATUS_OK;
    }
  }

  if (pflag == 1) {
    log_pipe_id = strtoul(p_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  if (sflag == 1) {
    stage = strtoul(s_arg, NULL, 0);
  }
  if (nflag == 1) {
    strncpy(tbl_name, n_arg, PIPE_MGR_TBL_NAME_LEN - 1);
  }
  if (tflag == 1) {
    type = strtoul(t_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if ((sflag + nflag + aflag) != 1) {
    aim_printf(&uc->pvs,
               "Either table name or stage or all stages"
               " should be specified \n");
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = log_pipe_id;

  dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    aim_printf(&uc->pvs, "Invalid dev_id %d\n", dev_id);
    return UCLI_STATUS_OK;
  }
  if (log_pipe_id >= dev_info->num_active_pipes) {
    aim_printf(&uc->pvs, "Invalid pipe_id %d\n", log_pipe_id);
    return UCLI_STATUS_OK;
  }

  if (nflag == 1) {
    aim_printf(
        &uc->pvs, "Setting counter type to %d for table %s \n", type, tbl_name);
    bf_tbl_dbg_counter_type_set(dev_tgt, tbl_name, type);
  } else if (sflag == 1) {
    aim_printf(&uc->pvs,
               "Setting counter type to %d for all tables in stage %d \n",
               type,
               stage);
    bf_tbl_dbg_counter_type_stage_set(dev_tgt, stage, type);
  } else if (aflag == 1) {
    aim_printf(&uc->pvs,
               "Setting counter type to %d for all tables in all stages \n",
               type);
    for (stage = 0; stage < dev_info->num_active_mau; stage++) {
      bf_tbl_dbg_counter_type_stage_set(dev_tgt, stage, type);
    }
  }

  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(tbl_cntr_clr) {
  int c, dflag, pflag, sflag, nflag, aflag;
  char *d_arg, *p_arg, *s_arg, *n_arg, *a_arg;
  bf_dev_id_t dev_id = 0;
  bf_dev_pipe_t log_pipe_id = 0;
  dev_stage_t stage = 0;
  char tbl_name[PIPE_MGR_TBL_NAME_LEN];
  dev_target_t dev_tgt;
  rmt_dev_info_t *dev_info = NULL;

  PIPE_MGR_CLI_PROLOGUE(
      "tbl-cntr-clr",
      " Clear the logical table counter value",
      "-d <dev_id> -p <log_pipe_id>{ -n <tbl_name> | -s <stage> "
      "| -a <all_stages> }");

  dflag = pflag = sflag = nflag = aflag = 0;
  d_arg = p_arg = s_arg = n_arg = a_arg = NULL;
  PIPE_MGR_MEMSET(tbl_name, 0, PIPE_MGR_TBL_NAME_LEN);

  while ((c = getopt(argc, argv, "d:p:s:n:a:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        d_arg = optarg;
        if (!d_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'p':
        pflag = 1;
        p_arg = optarg;
        if (!p_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 's':
        sflag = 1;
        s_arg = optarg;
        if (!s_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'n':
        nflag = 1;
        n_arg = optarg;
        if (!n_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'a':
        aflag = 1;
        a_arg = optarg;
        if (!a_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (dflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else if (dflag == 1) {
    dev_id = strtoul(d_arg, NULL, 0);
    if (dev_id > (PIPE_MGR_NUM_DEVICES - 1)) {
      aim_printf(&uc->pvs, "Invalid dev_id %s\n", d_arg);
      return UCLI_STATUS_OK;
    }
  }

  if (pflag == 1) {
    log_pipe_id = strtoul(p_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  if (sflag == 1) {
    stage = strtoul(s_arg, NULL, 0);
  }
  if (nflag == 1) {
    strncpy(tbl_name, n_arg, PIPE_MGR_TBL_NAME_LEN - 1);
  }

  if ((sflag + nflag + aflag) != 1) {
    aim_printf(&uc->pvs,
               "Either table name or stage or all stages"
               " should be specified \n");
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = log_pipe_id;

  dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    aim_printf(&uc->pvs, "Invalid dev_id %d\n", dev_id);
    return UCLI_STATUS_OK;
  }
  if (log_pipe_id >= dev_info->num_active_pipes) {
    aim_printf(&uc->pvs, "Invalid pipe_id %d\n", log_pipe_id);
    return UCLI_STATUS_OK;
  }

  if (nflag == 1) {
    aim_printf(&uc->pvs, "Clearing counter for table %s \n", tbl_name);
    bf_tbl_dbg_counter_clear(dev_tgt, tbl_name);
  } else if (sflag == 1) {
    aim_printf(
        &uc->pvs, "Clearing counter for all tables in stage %d \n", stage);
    bf_tbl_dbg_counter_stage_clear(dev_tgt, stage);
  } else if (aflag == 1) {
    aim_printf(&uc->pvs, "Clearing counter for all tables in all stages \n");
    for (stage = 0; stage < dev_info->num_active_mau; stage++) {
      bf_tbl_dbg_counter_stage_clear(dev_tgt, stage);
    }
  }

  return UCLI_STATUS_OK;
}

void pipe_mgr_print_stage_tbl_dbg_counters(aim_pvs_t *pvs,
                                           dev_target_t dev_tgt,
                                           dev_stage_t stage) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_tgt.device_id);
  if (!dev_info) return;
  uint32_t value_arr[dev_info->dev_cfg.stage_cfg.num_logical_tables];
  char tbl_name[dev_info->dev_cfg.stage_cfg.num_logical_tables]
               [PIPE_MGR_TBL_NAME_LEN];
  char buf[PIPE_MGR_TBL_NAME_LEN];
  int num_counters = 0, idx = 0;
  bf_tbl_dbg_counter_type_t
      type_arr[dev_info->dev_cfg.stage_cfg.num_logical_tables];

  aim_printf(pvs,
             "--------------------- Stage %d "
             "------------------------------------------ \n",
             stage);
  aim_printf(pvs,
             "%-45s %-3s %-15s %-13s\n",
             "Table",
             "Dir",
             "Counter-Type",
             "Counter-Value");
  aim_printf(pvs,
             "-----------------------------------------------------------------"
             "-------- \n");

  bf_tbl_dbg_counter_stage_get(
      dev_tgt, stage, &type_arr[0], &value_arr[0], tbl_name, &num_counters);
  for (idx = 0; idx < num_counters; idx++) {
    int dir = 0;
    dir = pipe_mgr_tbl_name_to_dir(
        dev_tgt.device_id, dev_tgt.dev_pipe_id, tbl_name[idx]);
    PIPE_MGR_MEMSET(buf, 0, PIPE_MGR_TBL_NAME_LEN);
    aim_printf(pvs,
               "%-45s %-3s %-15s %-13d \n",
               tbl_name[idx],
               (dir == 0) ? "Ig" : "Eg",
               pipe_mgr_tbl_dbg_counter_type_to_string(type_arr[idx], buf),
               value_arr[idx]);
  }
  aim_printf(pvs, "\n");
  return;
}

PIPE_MGR_CLI_CMD_DECLARE(tbl_cntr_print) {
  int c, dflag, pflag, sflag, nflag, aflag;
  char *d_arg, *p_arg, *s_arg, *n_arg, *a_arg;
  bf_dev_id_t dev_id = 0;
  bf_dev_pipe_t log_pipe_id = 0;
  dev_stage_t stage = 0;
  char tbl_name[PIPE_MGR_TBL_NAME_LEN];
  dev_target_t dev_tgt;
  bf_tbl_dbg_counter_type_t type = 0;
  uint32_t value = 0;
  rmt_dev_info_t *dev_info = NULL;

  PIPE_MGR_CLI_PROLOGUE(
      "tbl-cntr-print",
      " Print logical table counter value",
      "-d <dev_id> -p <log_pipe_id>{ -n <tbl_name> | -s <stage> "
      "| -a <all_stages>}");

  dflag = pflag = sflag = nflag = aflag = 0;
  d_arg = p_arg = s_arg = n_arg = a_arg = NULL;
  PIPE_MGR_MEMSET(tbl_name, 0, PIPE_MGR_TBL_NAME_LEN);

  while ((c = getopt(argc, argv, "d:p:s:n:a:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        d_arg = optarg;
        if (!d_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'p':
        pflag = 1;
        p_arg = optarg;
        if (!p_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 's':
        sflag = 1;
        s_arg = optarg;
        if (!s_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'n':
        nflag = 1;
        n_arg = optarg;
        if (!n_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'a':
        aflag = 1;
        a_arg = optarg;
        if (!a_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (dflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else if (dflag == 1) {
    dev_id = strtoul(d_arg, NULL, 0);
    if (dev_id > (PIPE_MGR_NUM_DEVICES - 1)) {
      aim_printf(&uc->pvs, "Invalid dev_id %s\n", d_arg);
      return UCLI_STATUS_OK;
    }
  }

  if (pflag == 1) {
    log_pipe_id = strtoul(p_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  if (sflag == 1) {
    stage = strtoul(s_arg, NULL, 0);
  }
  if (nflag == 1) {
    strncpy(tbl_name, n_arg, PIPE_MGR_TBL_NAME_LEN - 1);
  }

  if ((sflag + nflag + aflag) != 1) {
    aim_printf(&uc->pvs,
               "Either table name or stage or all stages"
               " should be specified \n");
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = log_pipe_id;

  dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    aim_printf(&uc->pvs, "Invalid dev_id %d\n", dev_id);
    return UCLI_STATUS_OK;
  }
  if (log_pipe_id >= dev_info->num_active_pipes) {
    aim_printf(&uc->pvs, "Invalid pipe_id %d\n", log_pipe_id);
    return UCLI_STATUS_OK;
  }

  if (nflag == 1) {
    char buf[PIPE_MGR_TBL_NAME_LEN];
    int dir = 0;
    PIPE_MGR_MEMSET(buf, 0, PIPE_MGR_TBL_NAME_LEN);

    aim_printf(&uc->pvs, "Getting counter for table %s \n", tbl_name);
    aim_printf(&uc->pvs,
               "---------------------------------------------------------------"
               "---------- \n");
    aim_printf(&uc->pvs,
               "%-45s %-3s %-15s %-13s\n",
               "Table",
               "Dir",
               "Counter-Type",
               "Counter-Value");
    aim_printf(&uc->pvs,
               "---------------------------------------------------------------"
               "---------- \n");
    bf_tbl_dbg_counter_get(dev_tgt, tbl_name, &type, &value);
    dir = pipe_mgr_tbl_name_to_dir(
        dev_tgt.device_id, dev_tgt.dev_pipe_id, tbl_name);
    aim_printf(&uc->pvs,
               "%-45s %-3s %-15s %-13d \n",
               tbl_name,
               (dir == 0) ? "Ig" : "Eg",
               pipe_mgr_tbl_dbg_counter_type_to_string(type, buf),
               value);
  } else if (sflag == 1) {
    aim_printf(
        &uc->pvs, "Getting counter for all tables in stage %d \n", stage);
    pipe_mgr_print_stage_tbl_dbg_counters(&uc->pvs, dev_tgt, stage);
  } else if (aflag == 1) {
    aim_printf(&uc->pvs, "Getting counter for all tables in all stages \n");

    for (stage = 0; stage < dev_info->num_active_mau; stage++) {
      pipe_mgr_print_stage_tbl_dbg_counters(&uc->pvs, dev_tgt, stage);
    }
  }

  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(batch_begin) {
  PIPE_MGR_CLI_PROLOGUE(
      "batch-begin", " Open a batch for a session", "-s <session>");

  int c, sflag = 0;
  char *s_arg = NULL;
  pipe_sess_hdl_t shdl = 0;

  while ((c = getopt(argc, argv, "s:")) != -1) {
    switch (c) {
      case 's':
        sflag = 1;
        s_arg = optarg;
        if (!s_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        shdl = strtoul(s_arg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (sflag != 1) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  pipe_status_t x = pipe_mgr_begin_batch(shdl);
  if (PIPE_SUCCESS != x) {
    aim_printf(&uc->pvs, "Error \"%s\" opening batch\n", pipe_str_err(x));
  }
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(batch_end) {
  PIPE_MGR_CLI_PROLOGUE(
      "batch-end", " Close and push a session's batch", "-s <session>");

  int c, sflag = 0;
  char *s_arg = NULL;
  pipe_sess_hdl_t shdl = 0;

  while ((c = getopt(argc, argv, "s:")) != -1) {
    switch (c) {
      case 's':
        sflag = 1;
        s_arg = optarg;
        if (!s_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        shdl = strtoul(s_arg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (sflag != 1) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  pipe_status_t x = pipe_mgr_end_batch(shdl, true);
  if (PIPE_SUCCESS != x) {
    aim_printf(&uc->pvs, "Error \"%s\" closing batch\n", pipe_str_err(x));
  }
  return UCLI_STATUS_OK;
}

static void sram_rd_verify(pipe_mgr_drv_buf_t *b,
                           uint32_t offset,
                           uint32_t count,
                           bool had_err,
                           void *arg) {
  (void)offset;
  (void)count;
  PIPE_MGR_ASSERT(!had_err);
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(0);
  if (!dev_info) {
    PIPE_MGR_ASSERT(0);
    return;
  }
  uint32_t sram_depth = dev_info->dev_cfg.stage_cfg.sram_unit_depth;
  uint32_t sram_width = dev_info->dev_cfg.stage_cfg.sram_unit_width;
  uint32_t tmp_sz = pipe_mgr_drv_buf_size(0, PIPE_MGR_DRV_BUF_BWR);
  size_t sz = tmp_sz < (sram_depth * sram_width / 8)
                  ? tmp_sz
                  : (sram_depth * sram_width / 8);
  PIPE_MGR_ASSERT(b->used == sz);
  int x = PIPE_MGR_MEMCMP(b->addr, arg, sz);
  PIPE_MGR_ASSERT(!x);
}

static void *pbus_irritator_sram(void *arg) {
  volatile bool *run = arg;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(0);
  if (!dev_info) {
    PIPE_MGR_ASSERT(0);
    return NULL;
  }
  uint32_t sram_depth = dev_info->dev_cfg.stage_cfg.sram_unit_depth;
  uint32_t sram_width = dev_info->dev_cfg.stage_cfg.sram_unit_width;
  /* Open session. */
  pipe_sess_hdl_t shdl = -1;
  if (PIPE_SUCCESS != pipe_mgr_client_init(&shdl)) PIPE_MGR_ASSERT(0);

  /* Prepare fixed data to write into the SRAM blocks. */
  uint32_t tmp_sz = pipe_mgr_drv_buf_size(0, PIPE_MGR_DRV_BUF_BWR);
  size_t sz = tmp_sz < (sram_depth * sram_width / 8)
                  ? tmp_sz
                  : (sram_depth * sram_width / 8);
  int ram_ids[] = {3, 4, 5, 10, 11};
  uint32_t **ram_data = PIPE_MGR_MALLOC(sizeof(uint32_t *) * sizeof ram_ids);
  size_t i;
  for (i = 0; i < sizeof ram_ids / sizeof ram_ids[0]; ++i) {
    ram_data[i] = PIPE_MGR_MALLOC(sz);
    unsigned int j;
    for (j = 0; j < sz / sizeof(int); j += 4) {
      ram_data[i][j + 0] = ram_ids[i];
      ram_data[i][j + 1] = j + 1;
      ram_data[i][j + 2] = j + 2;
      ram_data[i][j + 3] = (~j + 3);
    }
  }

  int num_stages = dev_info->num_active_mau;
  while (*run) {
    int rw;
    for (rw = 0; rw < 2; ++rw) { /* Alternate reads and writes. */
      int s;
      for (s = 0; s < num_stages; ++s) {
        for (i = 0; i < sizeof ram_ids / sizeof ram_ids[0]; ++i) {
          uint64_t addr = dev_info->dev_cfg.get_full_phy_addr(
              0, 0, s, ram_ids[i], 0, pipe_mem_type_unit_ram);
          pipe_status_t sts = PIPE_SUCCESS;
          if (!rw) {
            /* Get a buffer. */
            pipe_mgr_drv_buf_t *b = NULL;
            b = pipe_mgr_drv_buf_alloc(shdl, 0, sz, PIPE_MGR_DRV_BUF_BWR, true);
            PIPE_MGR_ASSERT(b);
            /* Fill it with our data to write. */
            PIPE_MGR_MEMCPY(b->addr, ram_data[i], sz);
            /* Issue the write. */
            sts = pipe_mgr_drv_blk_wr(
                &shdl, sram_width / 8, sz / (sram_width / 8), 1, addr, 1, b);
            PIPE_MGR_ASSERT(PIPE_SUCCESS == sts);
          } else {
            sts = pipe_mgr_drv_blk_rd(&shdl,
                                      0,
                                      sram_width / 8,
                                      sz / (sram_width / 8),
                                      1,
                                      addr,
                                      sram_rd_verify,
                                      ram_data[i]);
            PIPE_MGR_ASSERT(PIPE_SUCCESS == sts);
            pipe_mgr_drv_rd_blk_cmplt_all(shdl, 0);
          }
        }
      }
      if (!rw) pipe_mgr_drv_wr_blk_cmplt_all(shdl, 0);
    }
  }

  for (i = 0; i < sizeof ram_ids / sizeof ram_ids[0]; ++i) {
    PIPE_MGR_FREE(ram_data[i]);
  }
  PIPE_MGR_FREE(ram_data);

  /* Close the session. */
  if (PIPE_SUCCESS != pipe_mgr_client_cleanup(shdl)) PIPE_MGR_ASSERT(0);
  return NULL;
}

static void reg_rd_verify(pipe_mgr_drv_buf_t *b,
                          uint32_t offset,
                          uint32_t count,
                          bool had_err,
                          void *arg) {
  (void)arg;
  (void)offset;
  (void)count;
  PIPE_MGR_ASSERT(!had_err);

  uint32_t *x = (uint32_t *)b->addr;
  unsigned int i, j;
  for (i = 0; i < 96; ++i) {
    for (j = 0; j < 32; ++j, ++x) {
      if (j < 5) continue;
      PIPE_MGR_ASSERT(*x == ((i << 5) | j));
    }
  }
}

static void *pbus_irritator_reg(void *arg) {
  volatile bool *run = arg;
  /* Open session. */
  pipe_sess_hdl_t shdl = -1;
  if (PIPE_SUCCESS != pipe_mgr_client_init(&shdl)) PIPE_MGR_ASSERT(0);

  pipe_bitmap_t pbm = {{0}};
  PIPE_BITMAP_INIT(&pbm, PIPE_BMP_SIZE);
  PIPE_BITMAP_SET(&pbm, 0);

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(0);
  if (!dev_info) return NULL;
  int num_stages = dev_info->num_active_mau;
  while (*run) {
    pipe_status_t sts = PIPE_SUCCESS;
    int s;
    for (s = 0; s < num_stages; ++s) {
      int phv = 0, vliw = 0;
      for (; phv < 96; ++phv) {
        for (vliw = 5; vliw < 32; ++vliw) {
          pipe_instr_write_reg_t instr = {0};
          // uint64_t addr = offsetof(Tofino,
          // pipes[0].mau[s].dp.imem.imem_subword16[phv][vliw]);
          uint64_t step1 =
              tofino_pipes_mau_dp_imem_imem_subword16_array_element_size *
              tofino_pipes_mau_dp_imem_imem_subword16_array_dim_1_count;
          uint64_t step2 =
              tofino_pipes_mau_dp_imem_imem_subword16_array_element_size;
          uint64_t addr = tofino_pipes_mau_dp_imem_imem_subword16_address +
                          phv * step1 + vliw * step2;
          construct_instr_reg_write(0, &instr, addr, (phv << 5) | vliw);
          sts = pipe_mgr_drv_ilist_add(
              &shdl, 0, &pbm, s, (uint8_t *)&instr, sizeof instr);
          PIPE_MGR_ASSERT(PIPE_SUCCESS == sts);
        }
      }
    }
    pipe_mgr_drv_ilist_push(&shdl, NULL, NULL);
    pipe_mgr_drv_i_list_cmplt_all(&shdl);

    for (s = 0; s < num_stages; ++s) {
      uint64_t addr = tofino_pipes_mau_dp_imem_imem_subword16_address;
      addr = (UINT64_C(2) << 40) | ((uint64_t)s << 33) | (addr & 0x7FFFF);
      sts = pipe_mgr_drv_blk_rd(
          &shdl, 0, 4, 96 * 32, 4, addr, reg_rd_verify, NULL);
      PIPE_MGR_ASSERT(PIPE_SUCCESS == sts);
      pipe_mgr_drv_rd_blk_cmplt_all(shdl, 0);
    }
  }

  /* Close the session. */
  if (PIPE_SUCCESS != pipe_mgr_client_cleanup(shdl)) PIPE_MGR_ASSERT(0);
  return NULL;
}

PIPE_MGR_CLI_CMD_DECLARE(pbus_irritator) {
  PIPE_MGR_CLI_PROLOGUE("pbus-irritator",
                        " Start/stop background pbus traffic",
                        "-c <start|stop>");

  int c, flag = -1;
  static bf_sys_thread_t sram_thread;
  static bf_sys_thread_t reg_thread;
  static bool go_ = false;
  volatile bool *go = &go_;

  while ((c = getopt(argc, argv, "c:")) != -1) {
    switch (c) {
      case 'c':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        if (0 == strncmp(optarg, "start", 5)) {
          flag = 0;
        } else if (0 == strncmp(optarg, "stop", 4)) {
          flag = 1;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (flag == 0) {
    /* Start irritator threads. */
    *go = true;
    bf_sys_thread_create(&sram_thread, pbus_irritator_sram, &go_, 0);
    bf_sys_thread_set_name(sram_thread, "bf_pbus_irsram");
    bf_sys_thread_create(&reg_thread, pbus_irritator_reg, &go_, 0);
    bf_sys_thread_set_name(reg_thread, "bf_pbus_irreg");
  } else if (flag == 1) {
    /* Stop irritator threads. */
    *go = false;
    bf_sys_thread_join(sram_thread, NULL);
    bf_sys_thread_join(reg_thread, NULL);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  return UCLI_STATUS_OK;
}

static void *background_stat_dump(void *arg) {
  volatile bool *run = arg;
  /* Open session. */
  pipe_sess_hdl_t shdl = -1;
  if (PIPE_SUCCESS != pipe_mgr_client_init(&shdl)) PIPE_MGR_ASSERT(0);

  pipe_status_t sts = PIPE_SUCCESS;
  dev_target_t dev_tgt = {.device_id = 0, .dev_pipe_id = 0};
  while (*run) {
    sts = pipe_mgr_stat_database_sync(shdl, dev_tgt, 0x4000005, NULL, NULL);
    PIPE_MGR_ASSERT(PIPE_SUCCESS == sts);
    sts = pipe_mgr_stat_database_sync(shdl, dev_tgt, 0x4000006, NULL, NULL);
    PIPE_MGR_ASSERT(PIPE_SUCCESS == sts);
    sts = pipe_mgr_stat_database_sync(shdl, dev_tgt, 0x4000007, NULL, NULL);
    PIPE_MGR_ASSERT(PIPE_SUCCESS == sts);
    sts = pipe_mgr_stat_database_sync(shdl, dev_tgt, 0x4000008, NULL, NULL);
    PIPE_MGR_ASSERT(PIPE_SUCCESS == sts);
    sleep(5);
  }

  /* Close the session. */
  if (PIPE_SUCCESS != pipe_mgr_client_cleanup(shdl)) PIPE_MGR_ASSERT(0);
  return NULL;
}

PIPE_MGR_CLI_CMD_DECLARE(bkgrnd_stat_dump) {
  PIPE_MGR_CLI_PROLOGUE("bkgrnd-stat-dump",
                        " Start/stop background stat table",
                        "-c <start|stop>");

  int c, flag = -1;
  static bf_sys_thread_t wrkr_thread;
  static bool go_ = false;
  volatile bool *go = &go_;

  while ((c = getopt(argc, argv, "c:")) != -1) {
    switch (c) {
      case 'c':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        if (0 == strncmp(optarg, "start", 5)) {
          flag = 0;
        } else if (0 == strncmp(optarg, "stop", 4)) {
          flag = 1;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (flag == 0) {
    /* Start irritator threads. */
    *go = true;
    bf_sys_thread_create(&wrkr_thread, background_stat_dump, &go_, 0);
    bf_sys_thread_set_name(wrkr_thread, "bf_stat_dump");
  } else if (flag == 1) {
    /* Stop irritator threads. */
    *go = false;
    bf_sys_thread_join(wrkr_thread, NULL);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  return UCLI_STATUS_OK;
}





































PIPE_MGR_CLI_CMD_DECLARE(intr_dump) {
  PIPE_MGR_CLI_PROLOGUE("intr_dump",
                        " Dump device interrupts.",
                        "Usage: intr_dump -d <dev_id> [-n <count>]");
  int c, n = -1;
  bf_dev_id_t dev_id = 0;

  while ((c = getopt(argc, argv, "d:n:")) != -1) {
    switch (c) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev_id = strtoul(optarg, NULL, 0);
        break;
      case 'n':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        n = strtoul(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  pipe_mgr_err_evt_log_dump(uc, dev_id, n);

  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(tcam_scrub_set) {
  UCLI_COMMAND_INFO(uc,
                    "tcam-scrub-set",
                    -1,
                    "Set the tcam scrub timer value"
                    " Usage: tcam-scrub-set -d <dev_id> -t <msec>");
  int c, dflag = 0, tflag = 0;
  char *d_arg = NULL, *t_arg = NULL;
  bf_dev_id_t dev_id = 0xff;
  static char usage[] = "Usage: tcam-scrub-set -d <dev_id> -t <msec>\n";
  int argc;
  char *const *argv;
  int arg_start = 0;
  size_t i;
  uint32_t msec = 0;
  pipe_status_t status = PIPE_SUCCESS;

  for (i = 0; i < sizeof(uc->pargs[0].args__) / sizeof(uc->pargs[0].args__[0]);
       ++i) {
    if (!strncmp(uc->pargs[0].args__[i],
                 "tcam-scrub-set",
                 strlen("tcam-scrub-set"))) {
      arg_start = i;
      break;
    }
  }
  optind = 0; /* reset optind value */
  argc = (uc->pargs->count + 1);
  argv = (char *const *)&(uc->pargs->args__[arg_start]);

  while ((c = getopt(argc, argv, "d:t:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        d_arg = optarg;
        if (!d_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 't':
        tflag = 1;
        t_arg = optarg;
        if (!t_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (dflag == 1) {
    dev_id = strtoul(d_arg, NULL, 0);
    if ((dev_id == 0x3FF) || (dev_id > (PIPE_MGR_NUM_DEVICES - 1))) {
      aim_printf(&uc->pvs, "dev: Invalid dev_id %s\n", d_arg);
      return UCLI_STATUS_OK;
    }
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (tflag == 1) {
    msec = strtoul(t_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  status = pipe_mgr_tcam_scrub_timer_set(dev_id, msec);
  if (status == PIPE_SUCCESS) {
    aim_printf(&uc->pvs, "Tcam scrub timer successfully set to %d\n", msec);
  } else {
    aim_printf(&uc->pvs, "Tcam scrub timer set failed, rc=%d\n", status);
  }
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(tcam_scrub_get) {
  UCLI_COMMAND_INFO(uc,
                    "tcam-scrub-get",
                    -1,
                    "Get the tcam scrub timer value"
                    " Usage: tcam-scrub-get -d <dev_id>");
  int c, dflag = 0;
  char *d_arg = NULL;
  bf_dev_id_t dev_id = 0xff;
  static char usage[] = "Usage: tcam-scrub-get -d <dev_id>\n";
  int argc;
  char *const *argv;
  int arg_start = 0;
  size_t i;
  uint32_t msec = 0;

  for (i = 0; i < sizeof(uc->pargs[0].args__) / sizeof(uc->pargs[0].args__[0]);
       ++i) {
    if (!strncmp(uc->pargs[0].args__[i],
                 "tcam-scrub-get",
                 strlen("tcam-scrub-get"))) {
      arg_start = i;
      break;
    }
  }
  optind = 0; /* reset optind value */
  argc = (uc->pargs->count + 1);
  argv = (char *const *)&(uc->pargs->args__[arg_start]);

  while ((c = getopt(argc, argv, "d:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        d_arg = optarg;
        if (!d_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (dflag == 1) {
    dev_id = strtoul(d_arg, NULL, 0);
    if ((dev_id == 0x3FF) || (dev_id > (PIPE_MGR_NUM_DEVICES - 1))) {
      aim_printf(&uc->pvs, "dev: Invalid dev_id %s\n", d_arg);
      return UCLI_STATUS_OK;
    }
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  msec = pipe_mgr_tcam_scrub_timer_get(dev_id);
  aim_printf(&uc->pvs, "Tcam scrub timer value in msec: %d\n", msec);
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(alpm_inactive_node_delete_set) {
  UCLI_COMMAND_INFO(uc,
                    "alpm_inactive_node_delete_set",
                    -1,
                    "  Enable ALPM inactive node deletion"
                    " Usage: alpm_inactive_node_delete_set -e <enable "
                    "(0:disable, 1:enable)>");
  int c, enable = 0;
  static char usage[] =
      "Usage: alpm_inactive_node_delete_set -e <enable (0:disable, "
      "1:enable)>\n";
  int argc;
  char *const *argv;
  int arg_start = 0;
  size_t i;
  pipe_status_t status = PIPE_SUCCESS;

  for (i = 0; i < sizeof(uc->pargs[0].args__) / sizeof(uc->pargs[0].args__[0]);
       ++i) {
    if (!strncmp(uc->pargs[0].args__[i],
                 "alpm_inactive_node_delete_set",
                 strlen("alpm_inactive_node_delete_set"))) {
      arg_start = i;
      break;
    }
  }
  optind = 0; /* reset optind value */
  argc = (uc->pargs->count + 1);
  argv = (char *const *)&(uc->pargs->args__[arg_start]);

  while ((c = getopt(argc, argv, "e:")) != -1) {
    switch (c) {
      case 'e':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_E_ARG;
        }
        enable = !!strtoul(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  status = pipe_mgr_alpm_set_inactive_node_delete((enable ? true : false));
  if (status == PIPE_SUCCESS) {
    aim_printf(&uc->pvs, "alpm node deletion setting successfully set\n");
  } else {
    aim_printf(&uc->pvs, "Unable to set alpm node deletion setting\n");
  }
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(alpm_inactive_node_delete_get) {
  UCLI_COMMAND_INFO(uc,
                    "alpm_inactive_node_delete_get",
                    -1,
                    "  show ALPM inactive node deletion setting"
                    " Usage: alpm_inactive_node_delete_get ");
  pipe_status_t status = PIPE_SUCCESS;
  bool enable = false;

  status = pipe_mgr_alpm_get_inactive_node_delete(&enable);
  if (status == PIPE_SUCCESS) {
    if (enable) {
      aim_printf(&uc->pvs, "alpm node deletion is enabled\n");
    } else {
      aim_printf(&uc->pvs, "alpm node deletion is disabled\n");
    }
  } else {
    aim_printf(&uc->pvs, "Unable to fetch alpm node deletion setting\n");
  }
  return UCLI_STATUS_OK;
}

/**
 * Selector table sequence order UCLI
 */

PIPE_MGR_CLI_CMD_DECLARE(selector_tbl_sequence_set) {
  UCLI_COMMAND_INFO(uc,
                    "selector-tbl-sequence-set",
                    -1,
                    "  Enable Selector table sequence order"
                    " Usage: selector-tbl-sequence-set -e <enable "
                    "(0:disable, 1:enable)>");
  int c, enable = 0, eflag = 0;
  static char usage[] =
      "Usage: selector-tbl-sequence-set -e <enable (0:disable, "
      "1:enable)>\n";
  int argc;
  char *const *argv;
  int arg_start = 0;
  size_t i;
  pipe_status_t status = PIPE_SUCCESS;

  for (i = 0; i < sizeof(uc->pargs[0].args__) / sizeof(uc->pargs[0].args__[0]);
       ++i) {
    if (!strncmp(uc->pargs[0].args__[i],
                 "selector-tbl-sequence-set",
                 strlen("selector-tbl-sequence-set"))) {
      arg_start = i;
      break;
    }
  }
  optind = 0; /* reset optind value */
  argc = (uc->pargs->count + 1);
  argv = (char *const *)&(uc->pargs->args__[arg_start]);

  while ((c = getopt(argc, argv, "e:")) != -1) {
    switch (c) {
      case 'e':
        eflag = 1;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_E_ARG;
        }
        enable = strtoul(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (eflag && (enable == 0 || enable == 1)) {
    status = pipe_mgr_selector_tbl_set_sequence_order(enable);
    if (status == PIPE_SUCCESS) {
      aim_printf(&uc->pvs,
                 "Selector table sequence order %s\n",
                 enable ? "enabled" : "disabled");
    } else {
      aim_printf(&uc->pvs, "Unable to set selector table sequence order\n");
    }
    return UCLI_STATUS_OK;
  }

  aim_printf(&uc->pvs, "%s", usage);
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(selector_tbl_sequence_get) {
  UCLI_COMMAND_INFO(uc,
                    "selector-tbl-sequence-get",
                    -1,
                    "  show selector table sequence order setting"
                    " Usage: selector-tbl-sequence-get ");
  pipe_status_t status = PIPE_SUCCESS;
  bool enable = false;

  status = pipe_mgr_selector_tbl_get_sequence_order(&enable);
  if (status == PIPE_SUCCESS) {
    aim_printf(&uc->pvs,
               "Selector table sequence order is %s\n",
               enable ? "enabled" : "disabled");
  } else {
    aim_printf(&uc->pvs,
               "Unable to fetch Selector table sequence order setting\n");
  }
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(port_stuck_detect_timer_on) {
  UCLI_COMMAND_INFO(uc,
                    "port-stuck-detect-timer-on",
                    -1,
                    "Enable the port stuck detection timer"
                    " Usage: port-stuck-detect-timer-on -d <dev_id> -t <msec>");
  int c, dflag = 0, tflag = 0;
  char *d_arg = NULL, *t_arg = NULL;
  bf_dev_id_t dev_id = 0xff;
  static char usage[] =
      "Usage: port-stuck-detect-timer-on -d <dev_id> -t <msec>\n";
  int argc;
  char *const *argv;
  int arg_start = 0;
  size_t i;
  uint32_t msec = 0;
  pipe_status_t status = PIPE_SUCCESS;

  for (i = 0; i < sizeof(uc->pargs[0].args__) / sizeof(uc->pargs[0].args__[0]);
       ++i) {
    if (!strncmp(uc->pargs[0].args__[i],
                 "port-stuck-detect-timer-on",
                 strlen("port-stuck-detect-timer-on"))) {
      arg_start = i;
      break;
    }
  }
  optind = 0; /* reset optind value */
  argc = (uc->pargs->count + 1);
  argv = (char *const *)&(uc->pargs->args__[arg_start]);

  while ((c = getopt(argc, argv, "d:t:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        d_arg = optarg;
        if (!d_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 't':
        tflag = 1;
        t_arg = optarg;
        if (!t_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (dflag == 1) {
    dev_id = strtoul(d_arg, NULL, 0);
    if ((dev_id == 0x3FF) || (dev_id > (PIPE_MGR_NUM_DEVICES - 1))) {
      aim_printf(&uc->pvs, "dev: Invalid dev_id %s\n", d_arg);
      return UCLI_STATUS_OK;
    }
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (tflag == 1) {
    msec = strtoul(t_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  status = pipe_mgr_port_stuck_detect_timer_init(dev_id, msec);
  if (status == PIPE_SUCCESS) {
    aim_printf(
        &uc->pvs, "Port stuck detect timer successfully set to %d\n", msec);
  } else {
    aim_printf(&uc->pvs,
               "Port stuck detect timer set failed, rc=%d (%s)\n",
               status,
               pipe_str_err(status));
  }
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(port_stuck_detect_timer_off) {
  UCLI_COMMAND_INFO(uc,
                    "port-stuck-detect-timer-off",
                    -1,
                    "Disable the port stuck detection timer"
                    " Usage: port-stuck-detect-timer-off -d <dev_id>");
  int c, dflag = 0;
  char *d_arg = NULL;
  bf_dev_id_t dev_id = 0xff;
  static char usage[] = "Usage: port-stuck-detect-timer-off -d <dev_id>\n";
  int argc;
  char *const *argv;
  int arg_start = 0;
  size_t i;
  pipe_status_t status = PIPE_SUCCESS;

  for (i = 0; i < sizeof(uc->pargs[0].args__) / sizeof(uc->pargs[0].args__[0]);
       ++i) {
    if (!strncmp(uc->pargs[0].args__[i],
                 "port-stuck-detect-timer-off",
                 strlen("port-stuck-detect-timer-off"))) {
      arg_start = i;
      break;
    }
  }
  optind = 0; /* reset optind value */
  argc = (uc->pargs->count + 1);
  argv = (char *const *)&(uc->pargs->args__[arg_start]);

  while ((c = getopt(argc, argv, "d:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        d_arg = optarg;
        if (!d_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (dflag == 1) {
    dev_id = strtoul(d_arg, NULL, 0);
    if ((dev_id == 0x3FF) || (dev_id > (PIPE_MGR_NUM_DEVICES - 1))) {
      aim_printf(&uc->pvs, "dev: Invalid dev_id %s\n", d_arg);
      return UCLI_STATUS_OK;
    }
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  status = pipe_mgr_port_stuck_detect_timer_cleanup(dev_id);
  if (status == PIPE_SUCCESS) {
    aim_printf(&uc->pvs, "Port stuck detect timer successfully turned off\n");
  } else {
    aim_printf(&uc->pvs,
               "Could not turn off port stuck detect timer, rc=%d (%s)\n",
               status,
               pipe_str_err(status));
  }
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(port_set_iprsr_thresh) {
  PIPE_MGR_CLI_PROLOGUE(
      "port-set-iprsr-thresh",
      " Set priority threshold for ingress parser for port",
      " Usage: port-set-iprsr-thresh [-d <dev_id>] -p <dev_port> -v <0-3>");

  int c, pflag = 0, vflag = 0;
  char *p_arg = NULL, *v_arg = NULL;
  bf_dev_id_t dev_id = 0;
  bf_dev_port_t port_id;
  uint32_t threshold = 0;

  while ((c = getopt(argc, argv, "d:p:v:")) != -1) {
    switch (c) {
      case 'd':
        // Will default to 0 on fail, leave it as is
        dev_id = strtoul(optarg, NULL, 0);
        if (dev_id > (PIPE_MGR_NUM_DEVICES - 1)) {
          aim_printf(&uc->pvs, "dev: Invalid dev_id %s\n", optarg);
          return UCLI_STATUS_E_ARG;
        }
        break;
      case 'p':
        pflag = 1;
        p_arg = optarg;
        break;
      case 'v':
        vflag = 1;
        v_arg = optarg;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_E_ARG;
    }
  }

  if (vflag != 1 || pflag != 1) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_E_ARG;
  }

  port_id = strtoul(p_arg, NULL, 0);
  if (!DEV_PORT_VALIDATE(port_id)) {
    aim_printf(&uc->pvs, "port: Invalid port_id %s\n", p_arg);
    return UCLI_STATUS_E_ARG;
  }

  threshold = strtoul(v_arg, NULL, 0);

  pipe_status_t status =
      bf_pipe_mgr_port_iprsr_threshold_set(dev_id, port_id, threshold);
  if (status != PIPE_SUCCESS) {
    aim_printf(&uc->pvs,
               "Dev %d: Failed to set iprsr pri threshold %d for port %d\n",
               dev_id,
               threshold,
               port_id);
  }

  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(port_get_iprsr_thresh) {
  PIPE_MGR_CLI_PROLOGUE(
      "port-get-iprsr-thresh",
      " Set priority threshold for ingress parseri for port",
      " Usage: port-get-iprsr-thresh [-d <dev_id>] -p <dev_port>");

  int c, pflag = 0;
  char *p_arg = NULL;
  bf_dev_id_t dev_id = 0;
  bf_dev_port_t port_id;
  uint32_t threshold = 0;

  while ((c = getopt(argc, argv, "d:p:")) != -1) {
    switch (c) {
      case 'd':
        // Will default to 0 on fail, leave it as is
        dev_id = strtoul(optarg, NULL, 0);
        if (dev_id > (PIPE_MGR_NUM_DEVICES - 1)) {
          aim_printf(&uc->pvs, "dev: Invalid dev_id %s\n", optarg);
          return UCLI_STATUS_E_ARG;
        }
        break;
      case 'p':
        pflag = 1;
        p_arg = optarg;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_E_ARG;
    }
  }

  if (pflag == 1) {
    port_id = strtoul(p_arg, NULL, 0);
    if (!DEV_PORT_VALIDATE(port_id)) {
      aim_printf(&uc->pvs, "port: Invalid port_id %s\n", p_arg);
      return UCLI_STATUS_E_ARG;
    }
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_E_ARG;
  }

  pipe_status_t status =
      bf_pipe_mgr_port_iprsr_threshold_get(dev_id, port_id, &threshold);
  if (status == PIPE_SUCCESS) {
    aim_printf(
        &uc->pvs, "Port %d iprsr pri threshold: %d\n", port_id, threshold);
  } else {
    aim_printf(&uc->pvs,
               "Dev %d: Failed to get iprsr pri for port %d\n",
               dev_id,
               port_id);
  }
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(memid_hdl) {
  UCLI_COMMAND_INFO(uc,
                    "memid-hdl",
                    -1,
                    "Dump memory unit to handle mapping info"
                    " Usage: memid-hdl -d <dev_id> [-p <log_pipe_id>] [-s "
                    "<stage_id>] [-m <mem_id>]");
  static char usage[] =
      "Usage: memid-hdl -d <dev_id> [-p <log_pipe_id>] [-s <stage_id>] [-m "
      "<mem_id>]\n";
  int c, dflag, pflag, sflag, mflag;
  char *d_arg, *p_arg, *s_arg, *m_arg;
  bf_dev_id_t dev_id = 0;
  bf_dev_pipe_t log_pipe_id = 0xffff;
  uint8_t stage_id = 0xff;
  mem_id_t mem_id = 0xff;
  size_t i = 0;
  int arg_start = 0;
  int argc;
  char *const *argv;

  for (i = 0; i < sizeof(uc->pargs[0].args__) / sizeof(uc->pargs[0].args__[0]);
       ++i) {
    if (!strncmp(uc->pargs[0].args__[i], "memid-hdl", strlen("memid-hdl"))) {
      arg_start = i;
      break;
    }
  }
  optind = 0; /* reset optind value */
  argc = (uc->pargs->count + 1);
  argv = (char *const *)&(uc->pargs->args__[arg_start]);

  dflag = pflag = sflag = mflag = 0;
  d_arg = p_arg = s_arg = m_arg = NULL;

  while ((c = getopt(argc, argv, "d:p:s:m:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        d_arg = optarg;
        if (!d_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'p':
        pflag = 1;
        p_arg = optarg;
        if (!p_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 's':
        sflag = 1;
        s_arg = optarg;
        if (!s_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'm':
        mflag = 1;
        m_arg = optarg;
        if (!m_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (dflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else if (dflag == 1) {
    dev_id = strtoul(d_arg, NULL, 0);
    if (dev_id > (PIPE_MGR_NUM_DEVICES - 1)) {
      aim_printf(&uc->pvs, "Invalid dev_id %s\n", d_arg);
      return UCLI_STATUS_OK;
    }
  }

  if (pflag == 1) {
    log_pipe_id = strtoul(p_arg, NULL, 0);
  }

  if (sflag == 1) {
    stage_id = strtoul(s_arg, NULL, 0);
  }

  if (mflag == 1) {
    mem_id = strtoul(m_arg, NULL, 0);
  }

  pipe_mgr_dump_mem_id_to_tbl_hdl_mapping(
      uc, dev_id, log_pipe_id, stage_id, mem_id);

  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(tbl_hdl_conv) {
  UCLI_COMMAND_INFO(
      uc,
      "tbl-hdl-conv",
      -1,
      "Convert bf-drivers tbl-hdl to the one published in context.json"
      " Usage: tbl-hdl-conv -h <tbl-hdl> ");
  static char usage[] = "Usage: tbl-hdl-conv -h <tbl-hdl> \n";
  int c, hflag;
  char *h_arg;
  pipe_tbl_hdl_t tbl_hdl = 0, new_tbl_hdl = 0;
  size_t i = 0;
  int arg_start = 0;
  int argc;
  char *const *argv;

  for (i = 0; i < sizeof(uc->pargs[0].args__) / sizeof(uc->pargs[0].args__[0]);
       ++i) {
    if (!strncmp(
            uc->pargs[0].args__[i], "tbl-hdl-conv", strlen("tbl-hdl-conv"))) {
      arg_start = i;
      break;
    }
  }
  optind = 0; /* reset optind value */
  argc = (uc->pargs->count + 1);
  argv = (char *const *)&(uc->pargs->args__[arg_start]);

  hflag = 0;
  h_arg = NULL;

  while ((c = getopt(argc, argv, "h:")) != -1) {
    switch (c) {
      case 'h':
        hflag = 1;
        h_arg = optarg;
        if (!h_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (hflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else if (hflag == 1) {
    tbl_hdl = strtoul(h_arg, NULL, 0);
  }

  new_tbl_hdl = tbl_hdl & 0x3fffffff;
  aim_printf(&uc->pvs, "Table handle in bf-drivers   : 0x%x\n", tbl_hdl);
  aim_printf(&uc->pvs, "Table handle in context.json : 0x%x\n", new_tbl_hdl);

  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(blk_wr_perf) {
  PIPE_MGR_CLI_PROLOGUE(
      "blk-wr-perf",
      " Estimate block write performance",
      "-d <device> -m <mau-id> -p <prsr-id> -t <logical pipe bitmap>");

  bf_dev_id_t dev_id = 0;
  int mau_ids[64];
  int prsr_ids[64];
  int mau_count = 0;
  int prsr_count = 0;
  int pipe_map = 0;

  int c;
  while ((c = getopt(argc, argv, "d:m:p:t:")) != -1) {
    switch (c) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev_id = strtoul(optarg, NULL, 0);
        break;
      case 'm':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        mau_ids[mau_count++] = strtoul(optarg, NULL, 0);
        break;
      case 'p':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        prsr_ids[prsr_count++] = strtoul(optarg, NULL, 0);
        break;
      case 't':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        pipe_map = strtoul(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }
  if (!pipe_map || (!mau_count && !prsr_count)) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  for (int i = 0; i < mau_count; ++i) {
    if (mau_ids[i] >= (int)dev_info->num_active_mau || mau_ids[i] < 0) {
      aim_printf(&uc->pvs,
                 "mau-id %d outside of range 0-%d",
                 mau_ids[i],
                 dev_info->num_active_mau - 1);
      return UCLI_STATUS_OK;
    }
  }
  for (int i = 0; i < prsr_count; ++i) {
    if (prsr_ids[i] >= (int)dev_info->dev_cfg.num_prsrs || prsr_ids[i] < 0) {
      aim_printf(&uc->pvs,
                 "prsr-id %d outside of range 0-%d",
                 prsr_ids[i],
                 dev_info->dev_cfg.num_prsrs - 1);
      return UCLI_STATUS_OK;
    }
  }

  pipe_sess_hdl_t shdl = get_pipe_mgr_ctx()->int_ses_hndl;
  int dr_size;
  lld_dr_max_dr_depth_get(dev_id, BF_DMA_PIPE_BLOCK_WRITE, &dr_size, &dr_size);
  struct timespec t1, t2, t3;

  /* Put the MAUs into fast mode. */
  pipe_mgr_set_mem_slow_mode(dev_info, false);
  pipe_mgr_drv_i_list_cmplt_all(&shdl);

  /* Turn off the DR by setting its weight to zero. */
  uint32_t addr = 0, data;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      addr = offsetof(Tofino, device_select.pbc.pbc_pbus.arb_ctrl0);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      addr = offsetof(tof2_reg, device_select.pbc.pbc_pbus.arb_ctrl0);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      addr = offsetof(tof3_reg, device_select.pbc.pbc_pbus.arb_ctrl0);
      break;

    case BF_DEV_FAMILY_UNKNOWN:
      return UCLI_STATUS_OK;
  }
  lld_read_register(dev_id, addr, &data);
  uint32_t new_data = data;
  setp_pbus_arb_ctrl0_wb_req_weight(&new_data, 0);
  lld_write_register(dev_id, addr, new_data);

  /* Fill the DR with our write data. */
  for (int i = 0; i < mau_count; ++i) {
    for (int row = 0; row < dev_info->dev_cfg.stage_cfg.num_sram_rows; ++row) {
      for (int col = 2; col < dev_info->dev_cfg.stage_cfg.num_sram_cols;
           ++col) {
        int mem_id = dev_info->dev_cfg.mem_id_from_col_row(
            mau_ids[i], col, row, pipe_mem_type_unit_ram);
        uint64_t full_addr = dev_info->dev_cfg.get_full_phy_addr(
            0, 0, mau_ids[i], mem_id, 0, pipe_mem_type_unit_ram);

        pipe_mgr_drv_buf_t *b = pipe_mgr_drv_buf_alloc(
            0, dev_id, 16 * 1024, PIPE_MGR_DRV_BUF_BWR, false);
        if (!b) {
          aim_printf(&uc->pvs,
                     "%s:%d Error in allocating drv buffer, device %d\n",
                     __func__,
                     __LINE__,
                     dev_id);
          return UCLI_STATUS_OK;
        }
        PIPE_MGR_MEMSET(b->addr, mem_id, 16 * 1024);

        pipe_mgr_drv_blk_wr(&shdl, 16, 1024, 1, full_addr, pipe_map, b);
      }
    }
    for (int row = 0; row < dev_info->dev_cfg.stage_cfg.num_map_ram_rows;
         ++row) {
      for (int col = 0; col < dev_info->dev_cfg.stage_cfg.num_map_ram_cols;
           ++col) {
        int mem_id = dev_info->dev_cfg.mem_id_from_col_row(
            mau_ids[i], col, row, pipe_mem_type_map_ram);
        uint64_t full_addr = dev_info->dev_cfg.get_full_phy_addr(
            0, 0, mau_ids[i], mem_id, 0, pipe_mem_type_map_ram);

        pipe_mgr_drv_buf_t *b = pipe_mgr_drv_buf_alloc(
            0, dev_id, 4 * 4 * 1024, PIPE_MGR_DRV_BUF_BWR, false);
        if (!b) {
          aim_printf(&uc->pvs,
                     "%s:%d Error in allocating drv buffer, device %d\n",
                     __func__,
                     __LINE__,
                     dev_id);
          return UCLI_STATUS_OK;
        }
        PIPE_MGR_MEMSET(b->addr, 0, 4 * 4 * 1024);

        pipe_mgr_drv_blk_wr(&shdl, 4, 4 * 1024, 1, full_addr, pipe_map, b);

        full_addr = dev_info->dev_cfg.get_full_phy_addr(
            0, 0, mau_ids[i], mem_id + 4, 0, pipe_mem_type_map_ram);
        b = pipe_mgr_drv_buf_alloc(
            0, dev_id, 2 * 4 * 1024, PIPE_MGR_DRV_BUF_BWR, false);
        if (!b) {
          aim_printf(&uc->pvs,
                     "%s:%d Error in allocating drv buffer, device %d\n",
                     __func__,
                     __LINE__,
                     dev_id);
          return UCLI_STATUS_OK;
        }
        PIPE_MGR_MEMSET(b->addr, 0, 2 * 4 * 1024);

        pipe_mgr_drv_blk_wr(&shdl, 4, 2 * 1024, 1, full_addr, pipe_map, b);
      }
    }
    for (int row = 0; row < dev_info->dev_cfg.stage_cfg.num_tcam_rows; ++row) {
      for (int col = 0; col < dev_info->dev_cfg.stage_cfg.num_tcam_cols;
           ++col) {
        int mem_id = dev_info->dev_cfg.mem_id_from_col_row(
            mau_ids[i], col, row, pipe_mem_type_tcam);
        uint64_t full_addr = dev_info->dev_cfg.get_full_phy_addr(
            0, 0, mau_ids[i], mem_id, 0, pipe_mem_type_tcam);

        pipe_mgr_drv_buf_t *b = pipe_mgr_drv_buf_alloc(
            0, dev_id, 16 * 1024, PIPE_MGR_DRV_BUF_BWR, false);
        if (!b) {
          aim_printf(&uc->pvs,
                     "%s:%d Error in allocating drv buffer, device %d\n",
                     __func__,
                     __LINE__,
                     dev_id);
          return UCLI_STATUS_OK;
        }
        PIPE_MGR_MEMSET(b->addr, mem_id, 16 * 1024);

        pipe_mgr_drv_blk_wr(&shdl, 16, 1024, 1, full_addr, pipe_map, b);
      }
    }
  }
  for (int i = 0; i < prsr_count; ++i) {
    int p = prsr_ids[i];
    uint64_t iprsr_step = pipe_top_level_pipes_i_prsr_array_element_size;
    uint64_t iw0 = (pipe_top_level_pipes_i_prsr_ml_tcam_row_word0_address +
                    p * iprsr_step) /
                   16;
    uint64_t iw1 = (pipe_top_level_pipes_i_prsr_ml_tcam_row_word1_address +
                    p * iprsr_step) /
                   16;
    uint64_t eprsr_step = pipe_top_level_pipes_e_prsr_array_element_size;
    uint64_t ew0 = (pipe_top_level_pipes_e_prsr_ml_tcam_row_word0_address +
                    p * eprsr_step) /
                   16;
    uint64_t ew1 = (pipe_top_level_pipes_e_prsr_ml_tcam_row_word1_address +
                    p * eprsr_step) /
                   16;
    pipe_mgr_drv_buf_t *b = pipe_mgr_drv_buf_alloc(
        0, dev_id, 16 * 256, PIPE_MGR_DRV_BUF_BWR, false);
    if (!b) {
      aim_printf(&uc->pvs,
                 "%s:%d Error in allocating drv buffer, device %d\n",
                 __func__,
                 __LINE__,
                 dev_id);
      return UCLI_STATUS_OK;
    }
    PIPE_MGR_MEMSET(b->addr, p, 16 * 256);
    pipe_mgr_drv_blk_wr(&shdl, 16, 256, 1, iw0, pipe_map, b);

    b = pipe_mgr_drv_buf_alloc(
        0, dev_id, 16 * 256, PIPE_MGR_DRV_BUF_BWR, false);
    if (!b) {
      aim_printf(&uc->pvs,
                 "%s:%d Error in allocating drv buffer, device %d\n",
                 __func__,
                 __LINE__,
                 dev_id);
      return UCLI_STATUS_OK;
    }
    PIPE_MGR_MEMSET(b->addr, p, 16 * 256);
    pipe_mgr_drv_blk_wr(&shdl, 16, 256, 1, iw1, pipe_map, b);

    b = pipe_mgr_drv_buf_alloc(
        0, dev_id, 16 * 256, PIPE_MGR_DRV_BUF_BWR, false);
    if (!b) {
      aim_printf(&uc->pvs,
                 "%s:%d Error in allocating drv buffer, device %d\n",
                 __func__,
                 __LINE__,
                 dev_id);
      return UCLI_STATUS_OK;
    }
    PIPE_MGR_MEMSET(b->addr, p, 16 * 256);
    pipe_mgr_drv_blk_wr(&shdl, 16, 256, 1, ew0, pipe_map, b);

    b = pipe_mgr_drv_buf_alloc(
        0, dev_id, 16 * 256, PIPE_MGR_DRV_BUF_BWR, false);
    if (!b) {
      aim_printf(&uc->pvs,
                 "%s:%d Error in allocating drv buffer, device %d\n",
                 __func__,
                 __LINE__,
                 dev_id);
      return UCLI_STATUS_OK;
    }
    PIPE_MGR_MEMSET(b->addr, p, 16 * 256);
    pipe_mgr_drv_blk_wr(&shdl, 16, 256, 1, ew1, pipe_map, b);
  }

  /* Get start time. */
  clock_gettime(CLOCK_MONOTONIC, &t1);
  /* Reset the weights so the DR is processed again. */
  lld_write_register(dev_id, addr, data);

  /* Wait for all operations to complete. */
  pipe_mgr_drv_wr_blk_cmplt_all(shdl, dev_id);

  /* Get end time. */
  clock_gettime(CLOCK_MONOTONIC, &t2);

  if (t2.tv_nsec < t1.tv_nsec) {
    t2.tv_sec -= 1;
    t2.tv_nsec += 1000000000;
  }
  t3.tv_nsec = t2.tv_nsec - t1.tv_nsec;
  t3.tv_sec = t2.tv_sec - t1.tv_sec;
  aim_printf(&uc->pvs, "Start %ld.%09ld\n", t1.tv_sec, t1.tv_nsec);
  aim_printf(&uc->pvs, "End   %ld.%09ld\n", t2.tv_sec, t2.tv_nsec);
  aim_printf(&uc->pvs, "Delta %ld.%09ld\n", t3.tv_sec, t3.tv_nsec);

  uint32_t size_rx = lld_dr_used_get(dev_id, lld_dr_cmp_pipe_write_blk);
  PIPE_MGR_DBGCHK(size_rx == 0);

  /* Put the MAUs back to slow mode. */
  pipe_mgr_set_mem_slow_mode(dev_info, true);
  pipe_mgr_drv_i_list_cmplt_all(&shdl);

  return UCLI_STATUS_OK;
}

static void pipe_mgr_imem_rd_cb(pipe_mgr_drv_buf_t *b,
                                uint32_t offset,
                                uint32_t entry_count,
                                bool had_error,
                                void *userData) {
  (void)had_error;
  uint32_t *state = userData;
  uint32_t *rd_data = (uint32_t *)b->addr;
  for (uint32_t i = 0; i < entry_count; ++i) {
    state[offset + i] = rd_data[i];
  }
}
static pipe_status_t pipe_mgr_tof2_imem_test_one_stage(ucli_context_t *uc,
                                                       pipe_sess_hdl_t shdl,
                                                       rmt_dev_info_t *dev_info,
                                                       bf_dev_pipe_t pipe_id,
                                                       int stage,
                                                       int gress,
                                                       bool verbose) {
  if (dev_info->dev_family != BF_DEV_FAMILY_TOFINO2) {
    /* This test is TF2 specific. */
    return PIPE_INVALID_ARG;
  }
  bf_dev_id_t dev_id = dev_info->dev_id;
  pipe_status_t rc = PIPE_SUCCESS;
  const int max_printed_error_cnt = 256;
  /* Read imem_parity_ctl so we can preserve the original value and restore it
   * when done.  Read the imem thread control registers to preserve their
   * original value as well. */
  const int phv_thread_side_cnt = 2;
  const int phv_thread_vec_cnt = 14;
  uint32_t imem_parity_ctl[dev_info->dev_cfg.num_pipelines];
  uint32_t phv_ingress_thread[phv_thread_side_cnt][phv_thread_vec_cnt]
                             [dev_info->dev_cfg.num_pipelines];
  uint32_t phv_egress_thread[phv_thread_side_cnt][phv_thread_vec_cnt]
                            [dev_info->dev_cfg.num_pipelines];
  for (bf_dev_pipe_t log_pipe = 0; log_pipe < dev_info->num_active_pipes;
       ++log_pipe) {
    /* Skip pipes we are not working on. */
    if (pipe_id != BF_DEV_PIPE_ALL && pipe_id != log_pipe) continue;

    /* Convert logical to physical pipe. */
    bf_dev_pipe_t phy_pipe = log_pipe;
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe, &phy_pipe);

    /* Read imem_parity_ctl. */
    lld_read_register(
        dev_id,
        offsetof(tof2_reg, pipes[phy_pipe].mau[stage].dp.imem_parity_ctl),
        &imem_parity_ctl[log_pipe]);

    /* Read PHV thread registers. */
    for (int i = 0; i < phv_thread_side_cnt; ++i) {
      for (int j = 0; j < phv_thread_vec_cnt; ++j) {
        lld_read_register(
            dev_id,
            offsetof(tof2_reg,
                     pipes[phy_pipe].mau[stage].dp.phv_ingress_thread[i][j]),
            &phv_ingress_thread[i][j][log_pipe]);
        lld_read_register(
            dev_id,
            offsetof(tof2_reg,
                     pipes[phy_pipe].mau[stage].dp.phv_egress_thread[i][j]),
            &phv_egress_thread[i][j][log_pipe]);
      }
    }
  }

  /* Build a pipe bit map for DMA writes. */
  pipe_bitmap_t pbm;
  PIPE_BITMAP_INIT(&pbm, PIPE_BMP_SIZE);
  int log_pipe_msk = 0;
  for (bf_dev_pipe_t log_pipe = 0; log_pipe < dev_info->num_active_pipes;
       ++log_pipe) {
    if (pipe_id == BF_DEV_PIPE_ALL || pipe_id == log_pipe) {
      PIPE_BITMAP_SET(&pbm, log_pipe);
      log_pipe_msk |= 1u << log_pipe;
    }
  }

  /* Setup some state to define the parameters of the imem sections which
   * will be read and verified. */
  const uint32_t msk_mocha = 0x1FF;
  const uint32_t msk_dark = 0xFF;
  const uint32_t msk_norm8 = 0x7FFFFF;
  const uint32_t msk_norm16 = 0x3FFFFFF;
  const uint32_t msk_norm32 = 0x1FFFFFFF;
  struct imem_section {
    char name[8];
    enum pipe_mgr_tof2_imem_idx imem_type;
    uint32_t start_addr;
    uint32_t end_addr;
    uint32_t mask;
    int num_entries;
    uint32_t **rd_data; /* [num_pipes][num_entries] */
    int num_exclude_ranges;
    struct {
      int lo;
      int hi;
    } * exclude_ranges;
  };
  enum imem_section_type {
    dark16 = 0,
    dark32 = 1,
    dark8 = 2,
    mocha16 = 3,
    mocha32 = 4,
    mocha8 = 5,
    norm16 = 6,
    norm32 = 7,
    norm8 = 8,
    num_imem_sections = 9
  };
  struct imem_section imem_sections[num_imem_sections] = {
      /* Three sections for dark PHVs */
      {.name = "Dark16",
       .imem_type = PIPE_MGR_TOF2_IMEM16_DARK,
       .start_addr =
           offsetof(tof2_reg, pipes[0].mau[stage].dp.imem.imem_dark_subword16),
       .end_addr = offsetof(
           tof2_reg,
           pipes[0].mau[stage].dp.imem.imem_dark_subword16[1][2][3][31]),
       .mask = msk_dark,
       .num_entries = 0,
       .rd_data = NULL,
       .num_exclude_ranges = 2,
       .exclude_ranges = NULL},
      {.name = "Dark32",
       .imem_type = PIPE_MGR_TOF2_IMEM32_DARK,
       .start_addr =
           offsetof(tof2_reg, pipes[0].mau[stage].dp.imem.imem_dark_subword32),
       .end_addr = offsetof(
           tof2_reg,
           pipes[0].mau[stage].dp.imem.imem_dark_subword32[1][1][3][31]),
       .mask = msk_dark,
       .num_entries = 0,
       .rd_data = NULL,
       .num_exclude_ranges = 0,
       .exclude_ranges = NULL},
      {.name = "Dark8",
       .imem_type = PIPE_MGR_TOF2_IMEM8_DARK,
       .start_addr =
           offsetof(tof2_reg, pipes[0].mau[stage].dp.imem.imem_dark_subword8),
       .end_addr = offsetof(
           tof2_reg,
           pipes[0].mau[stage].dp.imem.imem_dark_subword8[1][1][3][31]),
       .mask = msk_dark,
       .num_entries = 0,
       .rd_data = NULL,
       .num_exclude_ranges = 0,
       .exclude_ranges = NULL},

      /* Three sections for mocha PHVs */
      {.name = "Mocha16",
       .imem_type = PIPE_MGR_TOF2_IMEM16_MOCHA,
       .start_addr =
           offsetof(tof2_reg, pipes[0].mau[stage].dp.imem.imem_mocha_subword16),
       .end_addr = offsetof(
           tof2_reg,
           pipes[0].mau[stage].dp.imem.imem_mocha_subword16[1][2][3][31]),
       .mask = msk_mocha,
       .num_entries = 0,
       .rd_data = NULL,
       .num_exclude_ranges = 2,
       .exclude_ranges = NULL},
      {.name = "Mocha32",
       .imem_type = PIPE_MGR_TOF2_IMEM32_MOCHA,
       .start_addr =
           offsetof(tof2_reg, pipes[0].mau[stage].dp.imem.imem_mocha_subword32),
       .end_addr = offsetof(
           tof2_reg,
           pipes[0].mau[stage].dp.imem.imem_mocha_subword32[1][1][3][31]),
       .mask = msk_mocha,
       .num_entries = 0,
       .rd_data = NULL,
       .num_exclude_ranges = 0,
       .exclude_ranges = NULL},
      {.name = "Mocha8",
       .imem_type = PIPE_MGR_TOF2_IMEM8_MOCHA,
       .start_addr =
           offsetof(tof2_reg, pipes[0].mau[stage].dp.imem.imem_mocha_subword8),
       .end_addr = offsetof(
           tof2_reg,
           pipes[0].mau[stage].dp.imem.imem_mocha_subword8[1][1][3][31]),
       .mask = msk_mocha,
       .num_entries = 0,
       .rd_data = NULL,
       .num_exclude_ranges = 0,
       .exclude_ranges = NULL},

      /* Three sections for normal PHVs */
      {.name = "Norm16",
       .imem_type = PIPE_MGR_TOF2_IMEM16,
       .start_addr =
           offsetof(tof2_reg, pipes[0].mau[stage].dp.imem.imem_subword16),
       .end_addr = offsetof(
           tof2_reg, pipes[0].mau[stage].dp.imem.imem_subword16[1][2][11][31]),
       .mask = msk_norm16,
       .num_entries = 0,
       .rd_data = NULL,
       .num_exclude_ranges = 6,
       .exclude_ranges = NULL},
      {.name = "Norm32",
       .imem_type = PIPE_MGR_TOF2_IMEM32,
       .start_addr =
           offsetof(tof2_reg, pipes[0].mau[stage].dp.imem.imem_subword32),
       .end_addr = offsetof(
           tof2_reg, pipes[0].mau[stage].dp.imem.imem_subword32[1][1][11][31]),
       .mask = msk_norm32,
       .num_entries = 0,
       .rd_data = NULL,
       .num_exclude_ranges = 3,
       .exclude_ranges = NULL},
      {.name = "Norm8",
       .imem_type = PIPE_MGR_TOF2_IMEM8,
       .start_addr =
           offsetof(tof2_reg, pipes[0].mau[stage].dp.imem.imem_subword8),
       .end_addr = offsetof(
           tof2_reg, pipes[0].mau[stage].dp.imem.imem_subword8[1][1][11][31]),
       .mask = msk_norm8,
       .num_entries = 0,
       .rd_data = NULL,
       .num_exclude_ranges = 3,
       .exclude_ranges = NULL}};
  /* The num_entries is calculated based on starting and ending CSR addresses.
   */
  for (int i = 0; i < num_imem_sections; ++i) {
    imem_sections[i].num_entries =
        (imem_sections[i].end_addr - imem_sections[i].start_addr + 4) / 4;
  }
  /* The excluded ranges is based on the CSR spec.  Certain imem sections
   * have defined their array sizes to be a power of two but do not use the
   * full range.  This leads to "holes" in the imem registers that will
   * always read back as zero.  Fill in the correct values for the imem
   * sections which have holes. */
  for (int i = 0; i < num_imem_sections; ++i) {
    if (!imem_sections[i].num_exclude_ranges) continue;
    imem_sections[i].exclude_ranges =
        PIPE_MGR_MALLOC(imem_sections[i].num_exclude_ranges *
                        sizeof imem_sections[i].exclude_ranges[0]);
    if (!imem_sections[i].exclude_ranges) {
      aim_printf(&uc->pvs, "Malloc failure\n");
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
  }
  /* Dark and mocha 16-bit PHVs have a hole in their imem where the second
   * dimension is four but only three are used.  Need to skip over
   * [0-1][3][0-3][0-31]. */
  uint32_t base = offsetof(
      tof2_reg, pipes[0].mau[stage].dp.imem.imem_dark_subword16[0][0][0][0]);
  imem_sections[dark16].exclude_ranges[0].lo =
      (offsetof(tof2_reg,
                pipes[0].mau[stage].dp.imem.imem_dark_subword16[0][3][0][0]) -
       base) /
      4;
  imem_sections[dark16].exclude_ranges[0].hi =
      (offsetof(tof2_reg,
                pipes[0].mau[stage].dp.imem.imem_dark_subword16[0][3][3][31]) -
       base) /
      4;
  imem_sections[dark16].exclude_ranges[1].lo =
      (offsetof(tof2_reg,
                pipes[0].mau[stage].dp.imem.imem_dark_subword16[1][3][0][0]) -
       base) /
      4;
  imem_sections[dark16].exclude_ranges[1].hi =
      (offsetof(tof2_reg,
                pipes[0].mau[stage].dp.imem.imem_dark_subword16[1][3][3][31]) -
       base) /
      4;
  if (verbose) {
    aim_printf(&uc->pvs,
               "Skipping dark16 %d-%d\n",
               imem_sections[dark16].exclude_ranges[0].lo,
               imem_sections[dark16].exclude_ranges[0].hi);
    aim_printf(&uc->pvs,
               "Skipping dark16 %d-%d\n",
               imem_sections[dark16].exclude_ranges[1].lo,
               imem_sections[dark16].exclude_ranges[1].hi);
  }
  base = offsetof(tof2_reg,
                  pipes[0].mau[stage].dp.imem.imem_mocha_subword16[0][0][0][0]);
  imem_sections[mocha16].exclude_ranges[0].lo =
      (offsetof(tof2_reg,
                pipes[0].mau[stage].dp.imem.imem_mocha_subword16[0][3][0][0]) -
       base) /
      4;
  imem_sections[mocha16].exclude_ranges[0].hi =
      (offsetof(tof2_reg,
                pipes[0].mau[stage].dp.imem.imem_mocha_subword16[0][3][3][31]) -
       base) /
      4;
  imem_sections[mocha16].exclude_ranges[1].lo =
      (offsetof(tof2_reg,
                pipes[0].mau[stage].dp.imem.imem_mocha_subword16[1][3][0][0]) -
       base) /
      4;
  imem_sections[mocha16].exclude_ranges[1].hi =
      (offsetof(tof2_reg,
                pipes[0].mau[stage].dp.imem.imem_mocha_subword16[1][3][3][31]) -
       base) /
      4;
  if (verbose) {
    aim_printf(&uc->pvs,
               "Skipping mocha16 %d-%d\n",
               imem_sections[mocha16].exclude_ranges[0].lo,
               imem_sections[mocha16].exclude_ranges[0].hi);
    aim_printf(&uc->pvs,
               "Skipping mocha16 %d-%d\n",
               imem_sections[mocha16].exclude_ranges[1].lo,
               imem_sections[mocha16].exclude_ranges[1].hi);
  }

  /* Normal PHVs have a hole in their imem where the third dimension is 16
   * but only 12 are used. Need to skip [0-1][0-3][12-15][0-31]. The normal
   * 16-bit PHVs also have a hole where the second dimension was sized at
   * four but only three are used.  Also need to skip over
   * [0-1][3][0-15][0-31] for these. */
  base = offsetof(tof2_reg,
                  pipes[0].mau[stage].dp.imem.imem_subword16[0][0][0][0]);
  imem_sections[norm16].exclude_ranges[0].lo =
      (offsetof(tof2_reg,
                pipes[0].mau[stage].dp.imem.imem_subword16[0][0][12][0]) -
       base) /
      4;
  imem_sections[norm16].exclude_ranges[0].hi =
      (offsetof(tof2_reg,
                pipes[0].mau[stage].dp.imem.imem_subword16[0][0][15][31]) -
       base) /
      4;
  imem_sections[norm16].exclude_ranges[1].lo =
      (offsetof(tof2_reg,
                pipes[0].mau[stage].dp.imem.imem_subword16[0][1][12][0]) -
       base) /
      4;
  imem_sections[norm16].exclude_ranges[1].hi =
      (offsetof(tof2_reg,
                pipes[0].mau[stage].dp.imem.imem_subword16[0][1][15][31]) -
       base) /
      4;
  imem_sections[norm16].exclude_ranges[2].lo =
      (offsetof(tof2_reg,
                pipes[0].mau[stage].dp.imem.imem_subword16[0][2][12][0]) -
       base) /
      4;
  imem_sections[norm16].exclude_ranges[2].hi =
      (offsetof(tof2_reg,
                pipes[0].mau[stage].dp.imem.imem_subword16[0][2][15][31]) -
       base) /
      4;
  imem_sections[norm16].exclude_ranges[3].lo =
      (offsetof(tof2_reg,
                pipes[0].mau[stage].dp.imem.imem_subword16[0][3][0][0]) -
       base) /
      4;
  imem_sections[norm16].exclude_ranges[3].hi =
      (offsetof(tof2_reg,
                pipes[0].mau[stage].dp.imem.imem_subword16[0][3][15][31]) -
       base) /
      4;
  imem_sections[norm16].exclude_ranges[4].lo =
      (offsetof(tof2_reg,
                pipes[0].mau[stage].dp.imem.imem_subword16[1][0][12][0]) -
       base) /
      4;
  imem_sections[norm16].exclude_ranges[4].hi =
      (offsetof(tof2_reg,
                pipes[0].mau[stage].dp.imem.imem_subword16[1][0][15][31]) -
       base) /
      4;
  imem_sections[norm16].exclude_ranges[5].lo =
      (offsetof(tof2_reg,
                pipes[0].mau[stage].dp.imem.imem_subword16[1][1][12][0]) -
       base) /
      4;
  imem_sections[norm16].exclude_ranges[5].hi =
      (offsetof(tof2_reg,
                pipes[0].mau[stage].dp.imem.imem_subword16[1][1][15][31]) -
       base) /
      4;
  if (verbose) {
    aim_printf(&uc->pvs,
               "Skipping norm16 %d-%d\n",
               imem_sections[norm16].exclude_ranges[0].lo,
               imem_sections[norm16].exclude_ranges[0].hi);
    aim_printf(&uc->pvs,
               "Skipping norm16 %d-%d\n",
               imem_sections[norm16].exclude_ranges[1].lo,
               imem_sections[norm16].exclude_ranges[1].hi);
    aim_printf(&uc->pvs,
               "Skipping norm16 %d-%d\n",
               imem_sections[norm16].exclude_ranges[2].lo,
               imem_sections[norm16].exclude_ranges[2].hi);
    aim_printf(&uc->pvs,
               "Skipping norm16 %d-%d\n",
               imem_sections[norm16].exclude_ranges[3].lo,
               imem_sections[norm16].exclude_ranges[3].hi);
    aim_printf(&uc->pvs,
               "Skipping norm16 %d-%d\n",
               imem_sections[norm16].exclude_ranges[4].lo,
               imem_sections[norm16].exclude_ranges[4].hi);
    aim_printf(&uc->pvs,
               "Skipping norm16 %d-%d\n",
               imem_sections[norm16].exclude_ranges[5].lo,
               imem_sections[norm16].exclude_ranges[5].hi);
  }

  base = offsetof(tof2_reg,
                  pipes[0].mau[stage].dp.imem.imem_subword32[0][0][0][0]);
  imem_sections[norm32].exclude_ranges[0].lo =
      (offsetof(tof2_reg,
                pipes[0].mau[stage].dp.imem.imem_subword32[0][0][12][0]) -
       base) /
      4;
  imem_sections[norm32].exclude_ranges[0].hi =
      (offsetof(tof2_reg,
                pipes[0].mau[stage].dp.imem.imem_subword32[0][0][15][31]) -
       base) /
      4;
  imem_sections[norm32].exclude_ranges[1].lo =
      (offsetof(tof2_reg,
                pipes[0].mau[stage].dp.imem.imem_subword32[0][1][12][0]) -
       base) /
      4;
  imem_sections[norm32].exclude_ranges[1].hi =
      (offsetof(tof2_reg,
                pipes[0].mau[stage].dp.imem.imem_subword32[0][1][15][31]) -
       base) /
      4;
  imem_sections[norm32].exclude_ranges[2].lo =
      (offsetof(tof2_reg,
                pipes[0].mau[stage].dp.imem.imem_subword32[1][0][12][0]) -
       base) /
      4;
  imem_sections[norm32].exclude_ranges[2].hi =
      (offsetof(tof2_reg,
                pipes[0].mau[stage].dp.imem.imem_subword32[1][0][15][31]) -
       base) /
      4;
  if (verbose) {
    aim_printf(&uc->pvs,
               "Skipping norm32 %d-%d\n",
               imem_sections[norm32].exclude_ranges[0].lo,
               imem_sections[norm32].exclude_ranges[0].hi);
    aim_printf(&uc->pvs,
               "Skipping norm32 %d-%d\n",
               imem_sections[norm32].exclude_ranges[1].lo,
               imem_sections[norm32].exclude_ranges[1].hi);
    aim_printf(&uc->pvs,
               "Skipping norm32 %d-%d\n",
               imem_sections[norm32].exclude_ranges[2].lo,
               imem_sections[norm32].exclude_ranges[2].hi);
  }

  base =
      offsetof(tof2_reg, pipes[0].mau[stage].dp.imem.imem_subword8[0][0][0][0]);
  imem_sections[norm8].exclude_ranges[0].lo =
      (offsetof(tof2_reg,
                pipes[0].mau[stage].dp.imem.imem_subword8[0][0][12][0]) -
       base) /
      4;
  imem_sections[norm8].exclude_ranges[0].hi =
      (offsetof(tof2_reg,
                pipes[0].mau[stage].dp.imem.imem_subword8[0][0][15][31]) -
       base) /
      4;
  imem_sections[norm8].exclude_ranges[1].lo =
      (offsetof(tof2_reg,
                pipes[0].mau[stage].dp.imem.imem_subword8[0][1][12][0]) -
       base) /
      4;
  imem_sections[norm8].exclude_ranges[1].hi =
      (offsetof(tof2_reg,
                pipes[0].mau[stage].dp.imem.imem_subword8[0][1][15][31]) -
       base) /
      4;
  imem_sections[norm8].exclude_ranges[2].lo =
      (offsetof(tof2_reg,
                pipes[0].mau[stage].dp.imem.imem_subword8[1][0][12][0]) -
       base) /
      4;
  imem_sections[norm8].exclude_ranges[2].hi =
      (offsetof(tof2_reg,
                pipes[0].mau[stage].dp.imem.imem_subword8[1][0][15][31]) -
       base) /
      4;
  if (verbose) {
    aim_printf(&uc->pvs,
               "Skipping norm8 %d-%d\n",
               imem_sections[norm8].exclude_ranges[0].lo,
               imem_sections[norm8].exclude_ranges[0].hi);
    aim_printf(&uc->pvs,
               "Skipping norm8 %d-%d\n",
               imem_sections[norm8].exclude_ranges[1].lo,
               imem_sections[norm8].exclude_ranges[1].hi);
    aim_printf(&uc->pvs,
               "Skipping norm8 %d-%d\n",
               imem_sections[norm8].exclude_ranges[2].lo,
               imem_sections[norm8].exclude_ranges[2].hi);
  }

  /* Allocate memory to hold the imem values read from HW. */
  for (int i = 0; i < num_imem_sections; ++i) {
    imem_sections[i].rd_data = PIPE_MGR_CALLOC(
        dev_info->num_active_pipes, sizeof *imem_sections[i].rd_data);
    for (bf_dev_pipe_t log_pipe = 0; log_pipe < dev_info->num_active_pipes;
         ++log_pipe) {
      if (pipe_id != BF_DEV_PIPE_ALL && pipe_id != log_pipe) continue;
      imem_sections[i].rd_data[log_pipe] =
          PIPE_MGR_MALLOC(4 * imem_sections[i].num_entries);
      if (!imem_sections[i].rd_data[log_pipe]) {
        aim_printf(&uc->pvs, "Malloc failure\n");
        rc = PIPE_NO_SYS_RESOURCES;
        goto cleanup;
      }
    }
  }

  /* Put the MAUs into fast mode. */
  pipe_mgr_set_mem_slow_mode(dev_info, false);

  /* Loop over ingress and egress. */
  for (int g = 0; g < 2; ++g) {
    if (gress != -1 && g != gress) continue;
    uint32_t val = 0;
    uint32_t addr = 0;
    pipe_instr_write_reg_t instr;

    /* Write imem_parity_ctl to:
     *  - Do not regenerate parity on write
     *  - return parity bit in read data (required to read dark/mocha imem
     *    sections)
     *  - do not check parity on read
     *  - disable slow mode
     *  - set thread to the current gress */
    setp_tof2_imem_parity_ctl_imem_parity_generate(&val, 0);
    setp_tof2_imem_parity_ctl_imem_parity_read_mask(&val, 0);
    setp_tof2_imem_parity_ctl_imem_parity_check_enable(&val, 0);
    setp_tof2_imem_parity_ctl_imem_slow_mode(&val, 0);
    setp_tof2_imem_parity_ctl_imem_read_egress_thread(&val, g);
    addr = offsetof(tof2_reg, pipes[0].mau[0].dp.imem_parity_ctl);
    construct_instr_reg_write(dev_id, &instr, addr, val);
    pipe_mgr_drv_ilist_add(
        &shdl, dev_info, &pbm, stage, (uint8_t *)&instr, sizeof instr);

    /* Write phv_ingress_thread/phv_ingress_thread_imem and
     * phv_egress_thread/phv_egress_thread_imem to set complete ownership to the
     * current gress. */
    for (int i = 0; i < phv_thread_side_cnt; ++i) {
      for (int j = 0; j < phv_thread_vec_cnt; ++j) {
        val = g == 0 ? 0xFFFFFFFF : 0;
        addr = offsetof(tof2_reg, pipes[0].mau[0].dp.phv_ingress_thread[i][j]);
        construct_instr_reg_write(dev_id, &instr, addr, val);
        pipe_mgr_drv_ilist_add(
            &shdl, dev_info, &pbm, stage, (uint8_t *)&instr, sizeof instr);
        addr = offsetof(tof2_reg,
                        pipes[0].mau[0].dp.phv_ingress_thread_imem[i][j]);
        construct_instr_reg_write(dev_id, &instr, addr, val);
        pipe_mgr_drv_ilist_add(
            &shdl, dev_info, &pbm, stage, (uint8_t *)&instr, sizeof instr);

        val = ~val;
        addr = offsetof(tof2_reg, pipes[0].mau[0].dp.phv_egress_thread[i][j]);
        construct_instr_reg_write(dev_id, &instr, addr, val);
        pipe_mgr_drv_ilist_add(
            &shdl, dev_info, &pbm, stage, (uint8_t *)&instr, sizeof instr);
        addr =
            offsetof(tof2_reg, pipes[0].mau[0].dp.phv_egress_thread_imem[i][j]);
        construct_instr_reg_write(dev_id, &instr, addr, val);
        pipe_mgr_drv_ilist_add(
            &shdl, dev_info, &pbm, stage, (uint8_t *)&instr, sizeof instr);
      }
    }

    /* Config is now setup, start the ilist and wait for it to complete before
     * performing the writes and reads to the imem. */
    pipe_mgr_drv_ilist_push(&shdl, NULL, NULL);
    pipe_mgr_drv_i_list_cmplt_all(&shdl);

    /* Back up the original imem data.
     * Allocate memory to hold a new imem "shadow" that we populate with the
     * pattern data. */
    uint8_t
        *imem_data[PIPE_MGR_TOF2_IMEM_COUNT][dev_info->dev_cfg.num_pipelines];
    uint32_t *imem_ptrn[PIPE_MGR_TOF2_IMEM_COUNT];
    for (int i = 0; i < PIPE_MGR_TOF2_IMEM_COUNT; ++i) {
      for (uint32_t p = 0; p < dev_info->dev_cfg.num_pipelines; ++p) {
        imem_data[i][p] =
            PIPE_INTR_IMEM_DATA(dev_id, p, stage).tof2.imem[i].data;
      }
      imem_ptrn[i] = PIPE_MGR_MALLOC(
          PIPE_INTR_IMEM_DATA(dev_id, 0, stage).tof2.imem[i].data_len);
      if (!imem_ptrn[i]) {
        for (; i >= 0; --i) {
          PIPE_MGR_FREE(imem_ptrn[i]);
        }
        goto restore_cfg;
      }
    }
    /* Point our shadow to the new buffers which will hold the imem patterns
     * to write. */
    for (int i = 0; i < PIPE_MGR_TOF2_IMEM_COUNT; ++i) {
      for (uint32_t p = 0; p < dev_info->dev_cfg.num_pipelines; ++p) {
        PIPE_INTR_IMEM_DATA(dev_id, p, stage).tof2.imem[i].data =
            (uint8_t *)imem_ptrn[i];
      }
    }
    /* Write the complete imem with a pattern and then read it back.  Write the
     * entire imem in one shot, including the address holes.  Read the imem back
     * section by section though .*/
    const int pattern_cnt = 7;
    for (int p = 0; p < pattern_cnt; ++p) {
      /* Write the current pattern into our temp shadows. */
      for (int i = 0; i < PIPE_MGR_TOF2_IMEM_COUNT; ++i) {
        /* Just use pipe 0 since all pipes have the same size. */
        uint32_t J =
            PIPE_INTR_IMEM_DATA(dev_id, 0, stage).tof2.imem[i].data_len / 4;
        for (uint32_t j = 0; j < J; ++j) {
          if (p == 0) {
            imem_ptrn[i][j] = (j & 1) ? 0 : 0xFFFFFFFF;
          } else if (p == 1) {
            imem_ptrn[i][j] = (j & 1) ? 0xFFFFFFFF : 0;
          } else if (p == 2) {
            imem_ptrn[i][j] = (j & 1) ? 0xAAAAAAAA : 0x55555555;
          } else if (p == 3) {
            imem_ptrn[i][j] = (j & 1) ? 0x55555555 : 0xAAAAAAAA;
          } else if (p == 4) {
            imem_ptrn[i][j] = (j & 1) ? 0x0F0F0F0F : 0xF0F0F0F0;
          } else if (p == 5) {
            imem_ptrn[i][j] = (j & 1) ? 0xF0F0F0F0 : 0x0F0F0F0F;
          } else {
            imem_ptrn[i][j] = j + 1;
          }
        }
      }
      /* Write imem from shadow. */
      bf_dev_pipe_t pipe_to_write = pipe_id;
      if (pipe_id != BF_DEV_PIPE_ALL) {
        pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe_id, &pipe_to_write);
      }
      pipe_mgr_imem_write(shdl, dev_info, pipe_to_write, stage, true);
      pipe_mgr_drv_ilist_push(&shdl, NULL, NULL);
      pipe_mgr_drv_i_list_cmplt_all(&shdl);
      pipe_mgr_drv_wr_blk_cmplt_all(shdl, dev_id);

      /*
       * Read the data back, note the reads must be per pipe.
       */

      /* Issue a read block for each imem section and pipe being tested. */
      for (bf_dev_pipe_t log_pipe = 0; log_pipe < dev_info->num_active_pipes;
           ++log_pipe) {
        if (pipe_id != BF_DEV_PIPE_ALL && pipe_id != log_pipe) continue;
        bf_dev_pipe_t phy_pipe = log_pipe;
        pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe, &phy_pipe);
        for (int i = 0; i < num_imem_sections; ++i) {
          uint32_t rd_addr = imem_sections[i].start_addr;
          uint32_t rd_cnt = imem_sections[i].num_entries;
          uint64_t imem_addr =
              dev_info->dev_cfg.pcie_pipe_addr_to_full_addr(rd_addr);
          uint64_t imem_pipe_addr =
              dev_info->dev_cfg.set_pipe_id_in_addr(imem_addr, phy_pipe);
          if (verbose) {
            aim_printf(
                &uc->pvs,
                "%-8s: Reading pipe %d, addr 0x%x, cnt %d, fullAddr 0x%" PRIx64
                "\n",
                imem_sections[i].name,
                log_pipe,
                rd_addr,
                rd_cnt,
                imem_pipe_addr);
          }
          pipe_mgr_drv_blk_rd(&shdl,
                              dev_id,
                              4,
                              rd_cnt,
                              4,
                              imem_pipe_addr,
                              pipe_mgr_imem_rd_cb,
                              imem_sections[i].rd_data[log_pipe]);
        } /* For each imem section */
      }   /* For each pipe */

      /* Wait for the read blocks to complete. */
      pipe_mgr_drv_rd_blk_cmplt_all(shdl, dev_id);

      /* Validate the data which was read back, it should match the pattern
       * written however the width of the register must be considered as well.
       * The parity bit in each imem register will be read back as whatever
       * value the HW set it to when the entry was written and parity was
       * calculated.  We will ignore it in the returned data with the masks used
       * in the comparision with the expected value. */
      for (bf_dev_pipe_t log_pipe = 0; log_pipe < dev_info->num_active_pipes;
           ++log_pipe) {
        if (pipe_id != BF_DEV_PIPE_ALL && pipe_id != log_pipe) continue;
        bf_dev_pipe_t phy_pipe = log_pipe;
        pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe, &phy_pipe);
        for (int i = 0; i < num_imem_sections; ++i) {
          uint32_t *expected = imem_ptrn[imem_sections[i].imem_type];
          int mismatched = 0;
          int rd_cnt = imem_sections[i].num_entries;
          for (int rd_idx = 0; rd_idx < rd_cnt; ++rd_idx) {
            /* Check if this index is in an exclude range. */
            bool exclude = false;
            for (int k = 0; k < imem_sections[i].num_exclude_ranges; ++k) {
              if (rd_idx == imem_sections[i].exclude_ranges[k].lo) {
                /* This index should be skipped, advance our iterator to the end
                 * of the skip range and the loop's increment will advance it to
                 * the next entry to check. */
                rd_idx = imem_sections[i].exclude_ranges[k].hi;
                exclude = true;
                break;
              }
            }
            if (exclude) continue;
            /* Check if the data read from imem matches the expected value. */
            if (imem_sections[i].rd_data[log_pipe][rd_idx] !=
                (expected[rd_idx] & imem_sections[i].mask)) {
              ++mismatched;
              if (mismatched < max_printed_error_cnt) {
                uint32_t failing_addr =
                    imem_sections[i].start_addr + rd_idx * 4;
                failing_addr = dev_info->dev_cfg.dir_addr_set_pipe_id(
                    failing_addr, phy_pipe);
                char *reg_name = lld_reg_parse_get_full_reg_path_name(
                    BF_DEV_FAMILY_TOFINO2, failing_addr);
                aim_printf(
                    &uc->pvs,
                    "Log Pipe %d Gress %d Stage %d imem-section %s entry %5d: "
                    "Got 0x%08x Expected 0x%08x pattern 0x%08x %s\n",
                    log_pipe,
                    g,
                    stage,
                    imem_sections[i].name,
                    rd_idx,
                    imem_sections[i].rd_data[log_pipe][rd_idx],
                    expected[rd_idx] & imem_sections[i].mask,
                    expected[rd_idx],
                    reg_name);
              } else if (mismatched == max_printed_error_cnt) {
                aim_printf(&uc->pvs,
                           "Additional mismatches found, logs suppressed\n");
              }
            }
          }
          if (!mismatched) {
            aim_printf(&uc->pvs,
                       "Section %-8s passed for pattern %d pipe %d stage %d "
                       "direction %s\n",
                       imem_sections[i].name,
                       p,
                       log_pipe,
                       stage,
                       g ? "egress" : "ingress");
          } else {
            rc = PIPE_COMM_FAIL;
            aim_printf(&uc->pvs,
                       "Section %-8s failed with %d mismatches for pattern "
                       "%d pipe %d stage %d direction %s\n",
                       imem_sections[i].name,
                       mismatched,
                       p,
                       log_pipe,
                       stage,
                       g ? "egress" : "ingress");
          }
        } /* For each imem type */
      }   /* For each pipe */
    }     /* For each pattern */

    /* Restore the imem shadows. */
    for (int i = 0; i < PIPE_MGR_TOF2_IMEM_COUNT; ++i) {
      for (uint32_t p = 0; p < dev_info->dev_cfg.num_pipelines; ++p) {
        PIPE_INTR_IMEM_DATA(dev_id, p, stage).tof2.imem[i].data =
            imem_data[i][p];
      }
      PIPE_MGR_FREE(imem_ptrn[i]);
    }
    /* Restore the imem contents. */
    bf_dev_pipe_t pipe_to_write = pipe_id;
    if (pipe_id != BF_DEV_PIPE_ALL) {
      pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe_id, &pipe_to_write);
    }
    pipe_mgr_imem_write(shdl, dev_info, pipe_to_write, stage, true);
    pipe_mgr_drv_ilist_push(&shdl, NULL, NULL);
    pipe_mgr_drv_i_list_cmplt_all(&shdl);
    pipe_mgr_drv_wr_blk_cmplt_all(shdl, dev_id);

    /* Restore thread-ctrl registers. */
    for (bf_dev_pipe_t log_pipe = 0; log_pipe < dev_info->num_active_pipes;
         ++log_pipe) {
      /* Skip pipes we are not working on. */
      if (pipe_id != BF_DEV_PIPE_ALL && pipe_id != log_pipe) continue;

      /* Convert logical to physical pipe. */
      bf_dev_pipe_t phy_pipe = log_pipe;
      pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe, &phy_pipe);

      /* Read PHV thread registers. */
      for (int i = 0; i < phv_thread_side_cnt; ++i) {
        for (int j = 0; j < phv_thread_vec_cnt; ++j) {
          lld_write_register(
              dev_id,
              offsetof(tof2_reg,
                       pipes[phy_pipe].mau[stage].dp.phv_ingress_thread[i][j]),
              phv_ingress_thread[i][j][log_pipe]);
          lld_write_register(
              dev_id,
              offsetof(
                  tof2_reg,
                  pipes[phy_pipe].mau[stage].dp.phv_ingress_thread_imem[i][j]),
              phv_ingress_thread[i][j][log_pipe]);
          lld_write_register(
              dev_id,
              offsetof(tof2_reg,
                       pipes[phy_pipe].mau[stage].dp.phv_egress_thread[i][j]),
              phv_egress_thread[i][j][log_pipe]);
          lld_write_register(
              dev_id,
              offsetof(
                  tof2_reg,
                  pipes[phy_pipe].mau[stage].dp.phv_egress_thread_imem[i][j]),
              phv_egress_thread[i][j][log_pipe]);
        }
      }
    }
  } /* For each gress */

restore_cfg:
  /* Put things back the way they were. */
  pipe_mgr_set_mem_slow_mode(dev_info, true);
  for (bf_dev_pipe_t log_pipe = 0; log_pipe < dev_info->num_active_pipes;
       ++log_pipe) {
    if (pipe_id != BF_DEV_PIPE_ALL && pipe_id != log_pipe) continue;
    bf_dev_pipe_t phy_pipe = log_pipe;
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe, &phy_pipe);

    lld_write_register(
        dev_id,
        offsetof(tof2_reg, pipes[phy_pipe].mau[stage].dp.imem_parity_ctl),
        imem_parity_ctl[log_pipe]);

    for (int i = 0; i < phv_thread_side_cnt; ++i) {
      for (int j = 0; j < phv_thread_vec_cnt; ++j) {
        lld_write_register(
            dev_id,
            offsetof(tof2_reg,
                     pipes[phy_pipe].mau[stage].dp.phv_ingress_thread[i][j]),
            phv_ingress_thread[i][j][log_pipe]);
        lld_write_register(
            dev_id,
            offsetof(tof2_reg,
                     pipes[phy_pipe].mau[stage].dp.phv_egress_thread[i][j]),
            phv_egress_thread[i][j][log_pipe]);
      }
    }

    pipe_mgr_tof2_imem_rewrite(shdl, dev_info, log_pipe, phy_pipe, stage);
  }

cleanup:
  for (int i = 0; i < num_imem_sections; ++i) {
    if (imem_sections[i].exclude_ranges) {
      PIPE_MGR_FREE(imem_sections[i].exclude_ranges);
      imem_sections[i].exclude_ranges = NULL;
    }
    if (imem_sections[i].rd_data) {
      for (bf_dev_pipe_t log_pipe = 0; log_pipe < dev_info->num_active_pipes;
           ++log_pipe) {
        if (imem_sections[i].rd_data[log_pipe]) {
          PIPE_MGR_FREE(imem_sections[i].rd_data[log_pipe]);
        }
      }

      PIPE_MGR_FREE(imem_sections[i].rd_data);
      imem_sections[i].rd_data = NULL;
    }
  }
  return rc;
}

PIPE_MGR_CLI_CMD_DECLARE(imem_test) {
  PIPE_MGR_CLI_PROLOGUE("imem-test",
                        " Write/read imem",
                        "-d <device> -p <logical pipe> -i <gress 0:ingress "
                        "1:egress> -s <stage-id> [-v]");

  pipe_sess_hdl_t shdl = 0;
  bf_dev_id_t dev_id = 0;
  bf_dev_pipe_t log_pipe = BF_DEV_PIPE_ALL;
  int gress = -1;
  int stage = -1;
  bool verbose = false;

  int c;
  while ((c = getopt(argc, argv, "d:p:i:s:v")) != -1) {
    switch (c) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev_id = strtoul(optarg, NULL, 0);
        break;
      case 'p':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        log_pipe = strtoul(optarg, NULL, 0);
        break;
      case 'i':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        gress = strtoul(optarg, NULL, 0);
        break;
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        stage = strtoul(optarg, NULL, 0);
        break;
      case 'v':
        verbose = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    aim_printf(&uc->pvs, "Dev %d not ready\n", dev_id);
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  if (dev_info->dev_family != BF_DEV_FAMILY_TOFINO2) {
    aim_printf(&uc->pvs, "imem_test only applies to Tofino-2\n");
    return UCLI_STATUS_OK;
  }
  if (log_pipe != BF_DEV_PIPE_ALL && log_pipe >= dev_info->num_active_pipes) {
    aim_printf(&uc->pvs,
               "Logical pipe %d not valid, only %d pipes active\n",
               log_pipe,
               dev_info->num_active_pipes);
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  if (gress != 0 && gress != 1 && gress != -1) {
    aim_printf(&uc->pvs,
               "gress %d not valid, use 0 for ingress or 1 for egress\n",
               gress);
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  if (stage != -1 && stage >= dev_info->num_active_mau) {
    aim_printf(&uc->pvs,
               "stage-id %d not valid, only %d stages\n",
               stage,
               dev_info->num_active_mau);
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  pipe_status_t rc = PIPE_SUCCESS;
  for (int s = 0; s < dev_info->num_active_mau; ++s) {
    if (stage != -1 && stage != s) continue;
    for (int g = 0; g < 2; ++g) {
      if (gress != -1 && gress != g) continue;
      if (verbose) {
        aim_printf(&uc->pvs,
                   "Testing dev %d pipe %x stage %d %s\n",
                   dev_id,
                   log_pipe,
                   s,
                   g == 0 ? "ingress" : "egress");
      }
      rc = pipe_mgr_tof2_imem_test_one_stage(
          uc, shdl, dev_info, log_pipe, s, g, verbose);
    }
  }
  if (rc == PIPE_SUCCESS) {
    aim_printf(&uc->pvs, "SUCCESS\n");
  } else {
    aim_printf(&uc->pvs, "FAILURE\n");
  }
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(imem_rewrite) {
  PIPE_MGR_CLI_PROLOGUE("imem-rewrite",
                        " Rewrite imem from SW shadow",
                        "-d <device> -p <logical pipe> -s <stage-id>");

  pipe_sess_hdl_t shdl = 0;
  bf_dev_id_t dev_id = 0;
  bf_dev_pipe_t log_pipe = BF_DEV_PIPE_ALL;
  int stage = -1;

  int c;
  while ((c = getopt(argc, argv, "d:p:i:s:v")) != -1) {
    switch (c) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev_id = strtoul(optarg, NULL, 0);
        break;
      case 'p':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        log_pipe = strtoul(optarg, NULL, 0);
        break;
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        stage = strtoul(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    aim_printf(&uc->pvs, "Dev %d not ready\n", dev_id);
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  if (log_pipe != BF_DEV_PIPE_ALL && log_pipe >= dev_info->num_active_pipes) {
    aim_printf(&uc->pvs,
               "Logical pipe %d not valid, only %d pipes active\n",
               log_pipe,
               dev_info->num_active_pipes);
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  bf_dev_pipe_t phy_pipe = log_pipe;
  if (log_pipe != BF_DEV_PIPE_ALL) {
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe, &phy_pipe);
  }

  pipe_mgr_drv_ilist_chkpt(shdl);
  pipe_status_t rc = pipe_mgr_imem_write(shdl, dev_info, phy_pipe, stage, true);
  if (rc == PIPE_SUCCESS) {
    pipe_mgr_drv_ilist_push(&shdl, NULL, NULL);
  } else {
    pipe_mgr_drv_ilist_rollback(shdl);
  }
  aim_printf(&uc->pvs, "Status: %s\n", pipe_str_err(rc));
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(prsr_cfg_dump) {
  PIPE_MGR_CLI_PROLOGUE("prsr-cfg-dump",
                        " dump parser configuration",
                        "-d <device> -p <logical pipe-id> -i <gress 0:ingress "
                        "1:egress>  -m <prsr-id>");

  bf_dev_id_t dev_id = 0;
  int log_pipe = 0;
  int dir = 0;
  int prsr_id = 0;

  int c;
  while ((c = getopt(argc, argv, "d:p:i:m:")) != -1) {
    switch (c) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev_id = strtoul(optarg, NULL, 0);
        break;
      case 'p':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        log_pipe = strtoul(optarg, NULL, 0);
        break;
      case 'i':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dir = strtoul(optarg, NULL, 0);
        break;
      case 'm':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        prsr_id = strtoul(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    aim_printf(&uc->pvs, "Dev %d not ready\n", dev_id);
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  if ((unsigned int)log_pipe >= dev_info->num_active_pipes) {
    aim_printf(&uc->pvs,
               "Logical pipe %d not valid, only %d pipes active\n",
               log_pipe,
               dev_info->num_active_pipes);
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  if (dir != 0 && dir != 1) {
    aim_printf(&uc->pvs,
               "gress %d not valid, use 0 for ingress or 1 for egress\n",
               dir);
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  if (prsr_id >= dev_info->dev_cfg.num_prsrs) {
    aim_printf(&uc->pvs,
               "prsr-id %d not valid, only %d parsers\n",
               prsr_id,
               dev_info->dev_cfg.num_prsrs);
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  bf_dev_pipe_t phy_pipe = log_pipe;
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe, &phy_pipe);
  bf_subdev_id_t subdev = pipe_mgr_subdev_id_from_pipe(phy_pipe);

  struct prsr_info {
    uint64_t addr;
    uint32_t count;
    uint32_t width;
  };
  struct prsr_info mem[9], reg[1];
  uint64_t pipe_step_mem = 0;
  uint64_t prsr_step_mem = 0;
  uint32_t pipe_step_reg = 0;
  PIPE_MGR_MEMSET(mem, 0, sizeof mem);
  PIPE_MGR_MEMSET(reg, 0, sizeof reg);
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      pipe_step_mem = pipe_top_level_pipes_array_element_size;
      prsr_step_mem = dir ? pipe_top_level_pipes_e_prsr_array_element_size
                          : pipe_top_level_pipes_i_prsr_array_element_size;
      pipe_step_reg = offsetof(Tofino, pipes[1]) - offsetof(Tofino, pipes[0]);
      /* Seven memories in total: Tcam Word0, Tcam Word1, Early Action, PO
       * Action, Counter Init, Checksum0, and Checksum1. */
      mem[0].addr = dir ? pipe_top_level_pipes_e_prsr_ml_tcam_row_word0_address
                        : pipe_top_level_pipes_i_prsr_ml_tcam_row_word0_address;
      mem[0].count = pipe_top_level_pipes_i_prsr_ml_tcam_row_word0_array_count;
      mem[0].width =
          pipe_top_level_pipes_i_prsr_ml_tcam_row_word0_array_element_size;
      mem[1].addr = dir ? pipe_top_level_pipes_e_prsr_ml_tcam_row_word1_address
                        : pipe_top_level_pipes_i_prsr_ml_tcam_row_word1_address;
      mem[1].count = pipe_top_level_pipes_i_prsr_ml_tcam_row_word1_array_count;
      mem[1].width =
          pipe_top_level_pipes_i_prsr_ml_tcam_row_word1_array_element_size;
      mem[2].addr = dir ? pipe_top_level_pipes_e_prsr_ml_ea_row_address
                        : pipe_top_level_pipes_i_prsr_ml_ea_row_address;
      mem[2].count = pipe_top_level_pipes_i_prsr_ml_ea_row_array_count;
      mem[2].width = pipe_top_level_pipes_i_prsr_ml_ea_row_array_element_size;
      mem[3].addr = dir ? pipe_top_level_pipes_e_prsr_po_action_row_address
                        : pipe_top_level_pipes_i_prsr_po_action_row_address;
      mem[3].count = pipe_top_level_pipes_i_prsr_po_action_row_array_count;
      mem[3].width =
          pipe_top_level_pipes_i_prsr_po_action_row_array_element_size;
      mem[4].addr = dir ? pipe_top_level_pipes_e_prsr_ml_ctr_init_ram_address
                        : pipe_top_level_pipes_i_prsr_ml_ctr_init_ram_address;
      mem[4].count = pipe_top_level_pipes_i_prsr_ml_ctr_init_ram_array_count;
      mem[4].width =
          pipe_top_level_pipes_i_prsr_ml_ctr_init_ram_array_element_size;
      mem[5].addr =
          dir ? pipe_top_level_pipes_e_prsr_po_csum_ctrl_0_row_address
              : pipe_top_level_pipes_i_prsr_po_csum_ctrl_0_row_address;
      mem[5].count = pipe_top_level_pipes_i_prsr_po_csum_ctrl_0_row_array_count;
      mem[5].width =
          pipe_top_level_pipes_i_prsr_po_csum_ctrl_0_row_array_element_size;
      mem[6].addr =
          dir ? pipe_top_level_pipes_e_prsr_po_csum_ctrl_1_row_address
              : pipe_top_level_pipes_i_prsr_po_csum_ctrl_1_row_address;
      mem[6].count = pipe_top_level_pipes_i_prsr_po_csum_ctrl_1_row_array_count;
      mem[6].width =
          pipe_top_level_pipes_i_prsr_po_csum_ctrl_1_row_array_element_size;
      /* One register block. */
      reg[0].addr =
          dir ? offsetof(
                    Tofino,
                    pipes[0].pmarb.ebp18_reg.ebp_reg[prsr_id].prsr_reg.scratch)
              : offsetof(
                    Tofino,
                    pipes[0].pmarb.ibp18_reg.ibp_reg[prsr_id].prsr_reg.scratch);
      reg[0].count = ((dir ? offsetof(Tofino,
                                      pipes[0]
                                          .pmarb.ebp18_reg.ebp_reg[prsr_id]
                                          .prsr_reg.debug_ctrl)
                           : offsetof(Tofino,
                                      pipes[0]
                                          .pmarb.ibp18_reg.ibp_reg[prsr_id]
                                          .prsr_reg.debug_ctrl)) -
                      reg[0].addr + 4) /
                     4;
      reg[0].width = 0;
      break;
    case BF_DEV_FAMILY_TOFINO2:
      pipe_step_mem = tof2_mem_pipes_array_element_size;
      prsr_step_mem = dir ? tof2_mem_pipes_parde_e_prsr_mem_array_element_size
                          : tof2_mem_pipes_parde_i_prsr_mem_array_element_size;
      pipe_step_reg =
          offsetof(tof2_reg, pipes[1]) - offsetof(tof2_reg, pipes[0]);
      /* Nine memories in total: PO Action, TCAM, Early Action, Ctr Init, and
       * five Checksum memories. */
      mem[0].addr = dir ? tof2_mem_pipes_parde_e_prsr_mem_po_action_row_address
                        : tof2_mem_pipes_parde_i_prsr_mem_po_action_row_address;
      mem[0].count = tof2_mem_pipes_parde_i_prsr_mem_po_action_row_array_count;
      mem[0].width =
          tof2_mem_pipes_parde_i_prsr_mem_po_action_row_array_element_size;
      mem[1].addr = dir ? tof2_mem_pipes_parde_e_prsr_mem_ml_tcam_row_address
                        : tof2_mem_pipes_parde_i_prsr_mem_ml_tcam_row_address;
      mem[1].count = tof2_mem_pipes_parde_i_prsr_mem_ml_tcam_row_array_count;
      mem[1].width =
          tof2_mem_pipes_parde_i_prsr_mem_ml_tcam_row_array_element_size;
      mem[2].addr = dir ? tof2_mem_pipes_parde_e_prsr_mem_ml_ea_row_address
                        : tof2_mem_pipes_parde_i_prsr_mem_ml_ea_row_address;
      mem[2].count = tof2_mem_pipes_parde_i_prsr_mem_ml_ea_row_array_count;
      mem[2].width =
          tof2_mem_pipes_parde_i_prsr_mem_ml_ea_row_array_element_size;
      mem[3].addr =
          dir ? tof2_mem_pipes_parde_e_prsr_mem_ml_ctr_init_ram_address
              : tof2_mem_pipes_parde_i_prsr_mem_ml_ctr_init_ram_address;
      mem[3].count =
          tof2_mem_pipes_parde_i_prsr_mem_ml_ctr_init_ram_array_count;
      mem[3].width =
          tof2_mem_pipes_parde_i_prsr_mem_ml_ctr_init_ram_array_element_size;
      mem[4].addr =
          dir ? tof2_mem_pipes_parde_e_prsr_mem_po_csum_ctrl_0_row_address
              : tof2_mem_pipes_parde_i_prsr_mem_po_csum_ctrl_0_row_address;
      mem[4].count =
          tof2_mem_pipes_parde_i_prsr_mem_po_csum_ctrl_0_row_array_count;
      mem[4].width =
          tof2_mem_pipes_parde_i_prsr_mem_po_csum_ctrl_0_row_array_element_size;
      mem[5].addr =
          dir ? tof2_mem_pipes_parde_e_prsr_mem_po_csum_ctrl_1_row_address
              : tof2_mem_pipes_parde_i_prsr_mem_po_csum_ctrl_1_row_address;
      mem[5].count =
          tof2_mem_pipes_parde_i_prsr_mem_po_csum_ctrl_1_row_array_count;
      mem[5].width =
          tof2_mem_pipes_parde_i_prsr_mem_po_csum_ctrl_1_row_array_element_size;
      mem[6].addr =
          dir ? tof2_mem_pipes_parde_e_prsr_mem_po_csum_ctrl_2_row_address
              : tof2_mem_pipes_parde_i_prsr_mem_po_csum_ctrl_2_row_address;
      mem[6].count =
          tof2_mem_pipes_parde_i_prsr_mem_po_csum_ctrl_2_row_array_count;
      mem[6].width =
          tof2_mem_pipes_parde_i_prsr_mem_po_csum_ctrl_2_row_array_element_size;
      mem[7].addr =
          dir ? tof2_mem_pipes_parde_e_prsr_mem_po_csum_ctrl_3_row_address
              : tof2_mem_pipes_parde_i_prsr_mem_po_csum_ctrl_3_row_address;
      mem[7].count =
          tof2_mem_pipes_parde_i_prsr_mem_po_csum_ctrl_3_row_array_count;
      mem[7].width =
          tof2_mem_pipes_parde_i_prsr_mem_po_csum_ctrl_3_row_array_element_size;
      mem[8].addr =
          dir ? tof2_mem_pipes_parde_e_prsr_mem_po_csum_ctrl_4_row_address
              : tof2_mem_pipes_parde_i_prsr_mem_po_csum_ctrl_4_row_address;
      mem[8].count =
          tof2_mem_pipes_parde_i_prsr_mem_po_csum_ctrl_4_row_array_count;
      mem[8].width =
          tof2_mem_pipes_parde_i_prsr_mem_po_csum_ctrl_4_row_array_element_size;
      /* One register block. */
      reg[0].addr =
          dir ? offsetof(tof2_reg,
                         pipes[0]
                             .pardereg.pgstnreg.epbprsr4reg[prsr_id / 4]
                             .prsr[prsr_id % 4]
                             .scratch)
              : offsetof(tof2_reg,
                         pipes[0]
                             .pardereg.pgstnreg.ipbprsr4reg[prsr_id / 4]
                             .prsr[prsr_id % 4]
                             .scratch);
      reg[0].count =
          ((dir ? offsetof(tof2_reg,
                           pipes[0]
                               .pardereg.pgstnreg.epbprsr4reg[prsr_id / 4]
                               .prsr[prsr_id % 4]
                               .phv_clr_on_wr.phv_clr_on_wr_7_8)
                : offsetof(tof2_reg,
                           pipes[0]
                               .pardereg.pgstnreg.ipbprsr4reg[prsr_id / 4]
                               .prsr[prsr_id % 4]
                               .phv_clr_on_wr.phv_clr_on_wr_7_8)) -
           reg[0].addr + 4) /
          4;
      reg[0].width = 0;
      break;
    case BF_DEV_FAMILY_TOFINO3:
      aim_printf(&uc->pvs, "Not supporrted for Tofino3\n");
      return 0;
      break;

    case BF_DEV_FAMILY_UNKNOWN:
      break;
  }

  for (size_t i = 0; i < sizeof(reg) / sizeof(reg[0]); ++i) {
    if (!reg[i].addr) break;
    uint32_t x, a = reg[i].addr + phy_pipe * pipe_step_reg;
    for (uint32_t j = 0; j < reg[0].count; ++j, a += 4) {
      lld_subdev_read_register(dev_id, subdev, a, &x);
      aim_printf(&uc->pvs, "R%08x %08x\n", a, x);
    }
  }

  for (size_t i = 0; i < sizeof(mem) / sizeof(mem[0]); ++i) {
    if (!mem[i].addr) break;
    uint64_t a =
        (mem[i].addr + phy_pipe * pipe_step_mem + prsr_id * prsr_step_mem) >> 4;
    int R = 0;

    aim_printf(&uc->pvs, "D%016" PRIx64 ":\n", a);

    struct prsr_mem_row {
      uint64_t hi;
      uint64_t lo;
    };
    struct prsr_mem_row *row = PIPE_MGR_CALLOC(
        mem[i].count * (mem[i].width / 16), sizeof(struct prsr_mem_row));
    if (!row) {
      aim_printf(&uc->pvs, "Memory Alloc Failure");
      return UCLI_STATUS_OK;
    }
    for (uint32_t j = 0; j < mem[i].count; ++j) {
      for (uint32_t k = 0; k < mem[i].width; k += 16) {
        lld_subdev_ind_read(dev_id, subdev, a, &row[R].hi, &row[R].lo);
        ++a;
        ++R;
      }
    }

    uint64_t l_hi, l_lo;
    uint64_t r_hi, r_lo;
    int repeat = 1;
    bool l_valid = false, r_valid = false;
    for (int r = 0; r < R; ++r) {
      if (!l_valid) {
        l_hi = row[r].hi;
        l_lo = row[r].lo;
        l_valid = true;
        repeat = 1;
      } else if (!r_valid) {
        if (l_hi == row[r].hi && l_lo == row[r].lo) {
          ++repeat;
        } else if (repeat == 1) {
          r_hi = row[r].hi;
          r_lo = row[r].lo;
          r_valid = true;
        } else {
          aim_printf(&uc->pvs,
                     "    %016" PRIx64 "%016" PRIx64 " x%d\n",
                     l_hi,
                     l_lo,
                     repeat);
          l_hi = row[r].hi;
          l_lo = row[r].lo;
          repeat = 1;
        }
      } else {
        if (r_hi == row[r].hi && r_lo == row[r].lo) {
          ++repeat;
        } else {
          if (repeat == 1) {
            aim_printf(&uc->pvs,
                       "    %016" PRIx64 "%016" PRIx64 " %016" PRIx64
                       "%016" PRIx64 "\n",
                       l_hi,
                       l_lo,
                       r_hi,
                       r_lo);
          } else {
            aim_printf(&uc->pvs,
                       "    %016" PRIx64 "%016" PRIx64 " %016" PRIx64
                       "%016" PRIx64 " x%d\n",
                       l_hi,
                       l_lo,
                       r_hi,
                       r_lo,
                       repeat);
          }
          l_hi = row[r].hi;
          l_lo = row[r].lo;
          repeat = 1;
          r_valid = false;
        }
      }
    }
    if (l_valid && r_valid && repeat == 1) {
      aim_printf(&uc->pvs,
                 "    %016" PRIx64 "%016" PRIx64 " %016" PRIx64 "%016" PRIx64
                 "\n",
                 l_hi,
                 l_lo,
                 r_hi,
                 r_lo);
    } else if (l_valid && r_valid) {
      aim_printf(&uc->pvs,
                 "    %016" PRIx64 "%016" PRIx64 " %016" PRIx64 "%016" PRIx64
                 " x%d\n",
                 l_hi,
                 l_lo,
                 r_hi,
                 r_lo,
                 repeat);
    } else if (l_valid && repeat == 1) {
      aim_printf(&uc->pvs, "    %016" PRIx64 "%016" PRIx64 "\n", l_hi, l_lo);
    } else if (l_valid) {
      aim_printf(&uc->pvs,
                 "    %016" PRIx64 "%016" PRIx64 " x%d\n",
                 l_hi,
                 l_lo,
                 repeat);
    }
    PIPE_MGR_FREE(row);
  }

  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(pps_set) {
  PIPE_MGR_CLI_PROLOGUE("pps-set",
                        " Configure pipe PPS limit",
                        "-d <device> -p <pipe-id> -r <PPS>");

  bf_dev_id_t dev_id = 0;
  int pipe_id = BF_DEV_PIPE_ALL;
  uint64_t pps = 0;

  int c;
  while ((c = getopt(argc, argv, "d:p:r:")) != -1) {
    switch (c) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev_id = strtoul(optarg, NULL, 0);
        break;
      case 'p':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        pipe_id = strtoul(optarg, NULL, 0);
        break;
      case 'r':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        pps = strtoul(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }
  if (!pps) {
    aim_printf(&uc->pvs, "PPS must be provided and non-zero\n");
    return UCLI_STATUS_OK;
  }

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    aim_printf(&uc->pvs, "Device %d not found\n", dev_id);
    return UCLI_STATUS_OK;
  }

  if (pipe_id != BF_DEV_PIPE_ALL &&
      pipe_id >= (int)dev_info->num_active_pipes) {
    aim_printf(&uc->pvs, "Pipe %d not valid\n", pipe_id);
    return UCLI_STATUS_OK;
  }

  dev_target_t dt;
  dt.device_id = dev_id;
  dt.dev_pipe_id = pipe_id;
  pipe_status_t sts = pipe_mgr_pipe_pps_limit_set(dt, pps);
  if (sts != PIPE_SUCCESS) {
    aim_printf(&uc->pvs,
               "Set PPS failed, dev %d pipe %x PPS %" PRIu64 " sts %s\n",
               dev_id,
               pipe_id,
               pps,
               pipe_str_err(sts));
  } else {
    uint64_t cur_pps = 0, max_pps = 0;
    pipe_mgr_pipe_pps_limit_get(dt, &cur_pps);
    pipe_mgr_pipe_pps_max_get(dt, &max_pps);
    aim_printf(
        &uc->pvs,
        "Dev %d pipe %x requested PPS %" PRIu64 " actual %" PRIu64
        " max %" PRIu64 " -- %d%% of max\n",
        dev_id,
        pipe_id,
        pps,
        cur_pps,
        max_pps,
        max_pps ? (int)(0.5 + 100.0 * ((double)cur_pps / (double)max_pps)) : 0);
  }
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(pps_get) {
  PIPE_MGR_CLI_PROLOGUE(
      "pps-get", " Get pipe PPS limit", "-d <device> -p <pipe-id>");

  bf_dev_id_t dev_id = 0;
  int pipe_id = BF_DEV_PIPE_ALL;
  uint64_t pps = 0;

  int c;
  while ((c = getopt(argc, argv, "d:p:")) != -1) {
    switch (c) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev_id = strtoul(optarg, NULL, 0);
        break;
      case 'p':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        pipe_id = strtoul(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    aim_printf(&uc->pvs, "Device %d not found\n", dev_id);
    return UCLI_STATUS_OK;
  }

  if (pipe_id != BF_DEV_PIPE_ALL &&
      pipe_id >= (int)dev_info->num_active_pipes) {
    aim_printf(&uc->pvs, "Pipe %d not valid\n", pipe_id);
    return UCLI_STATUS_OK;
  }

  dev_target_t dt;
  dt.device_id = dev_id;
  dt.dev_pipe_id = pipe_id;
  pipe_status_t sts = pipe_mgr_pipe_pps_limit_get(dt, &pps);
  if (sts != PIPE_SUCCESS) {
    aim_printf(&uc->pvs,
               "Get PPS failed, dev %d pipe %x sts %s\n",
               dev_id,
               pipe_id,
               pipe_str_err(sts));
  } else {
    uint64_t max_pps = 0;
    pipe_mgr_pipe_pps_max_get(dt, &max_pps);
    aim_printf(
        &uc->pvs,
        "Dev %d pipe %x current PPS limit %" PRIu64 " max %" PRIu64
        " -- %d%% of max\n",
        dev_id,
        pipe_id,
        pps,
        max_pps,
        max_pps ? (int)(0.5 + 100.0 * ((double)pps / (double)max_pps)) : 0);
  }
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(pps_reset) {
  PIPE_MGR_CLI_PROLOGUE(
      "pps-reset", " Reset pipe PPS limit", "-d <device> -p <pipe-id>");

  bf_dev_id_t dev_id = 0;
  int pipe_id = BF_DEV_PIPE_ALL;

  int c;
  while ((c = getopt(argc, argv, "d:p:")) != -1) {
    switch (c) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev_id = strtoul(optarg, NULL, 0);
        break;
      case 'p':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        pipe_id = strtoul(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    aim_printf(&uc->pvs, "Device %d not found\n", dev_id);
    return UCLI_STATUS_OK;
  }

  if (pipe_id != BF_DEV_PIPE_ALL &&
      pipe_id >= (int)dev_info->num_active_pipes) {
    aim_printf(&uc->pvs, "Pipe %d not valid\n", pipe_id);
    return UCLI_STATUS_OK;
  }

  dev_target_t dt;
  dt.device_id = dev_id;
  dt.dev_pipe_id = pipe_id;
  pipe_status_t sts = pipe_mgr_pipe_pps_limit_reset(dt);
  if (sts != PIPE_SUCCESS) {
    aim_printf(&uc->pvs,
               "Get PPS failed, dev %d pipe %x sts %s\n",
               dev_id,
               pipe_id,
               pipe_str_err(sts));
  } else {
    uint64_t cur_pps = 0, max_pps = 0;
    pipe_mgr_pipe_pps_limit_get(dt, &cur_pps);
    pipe_mgr_pipe_pps_max_get(dt, &max_pps);
    aim_printf(
        &uc->pvs,
        "Dev %d pipe %x reset PPS to %" PRIu64 " max %" PRIu64
        " -- %d%% of max\n",
        dev_id,
        pipe_id,
        cur_pps,
        max_pps,
        max_pps ? (int)(0.5 + 100.0 * ((double)cur_pps / (double)max_pps)) : 0);
  }
  return UCLI_STATUS_OK;
}

struct pipe_mgr_ucli_rd_blk_ctx {
  ucli_context_t *uc;
  int width;
  int count;
  int step;
  uint64_t addr;
};
static void blk_rd_cb(pipe_mgr_drv_buf_t *b,
                      uint32_t offset,
                      uint32_t entry_count,
                      bool had_error,
                      void *user_data) {
  struct pipe_mgr_ucli_rd_blk_ctx *ctx = user_data;
  if (!ctx) return;
  ucli_context_t *uc = ctx->uc;
  aim_printf(&uc->pvs,
             "Block read completion: Offset %d Count %d Error %c\n",
             offset,
             entry_count,
             had_error ? 'Y' : 'N');
  for (uint32_t i = 0; i < entry_count; ++i) {
    switch (ctx->width) {
      case 4:
        aim_printf(&uc->pvs,
                   "0x%011" PRIx64 ": %08x\n",
                   ctx->addr + ctx->step * (offset + i),
                   ((uint32_t *)b->addr)[i]);
        break;
      case 8:
        aim_printf(&uc->pvs,
                   "0x%011" PRIx64 ": %08x %08x\n",
                   ctx->addr + ctx->step * (offset + i),
                   ((uint32_t *)b->addr)[2 * i],
                   ((uint32_t *)b->addr)[2 * i + 1]);
        break;
      case 16:
        aim_printf(&uc->pvs,
                   "0x%011" PRIx64 ": %08x %08x %08x %08x\n",
                   ctx->addr + ctx->step * (offset + i),
                   ((uint32_t *)b->addr)[4 * i],
                   ((uint32_t *)b->addr)[4 * i + 1],
                   ((uint32_t *)b->addr)[4 * i + 2],
                   ((uint32_t *)b->addr)[4 * i + 3]);
        break;
    }
  }
}
PIPE_MGR_CLI_CMD_DECLARE(blk_rd) {
  PIPE_MGR_CLI_PROLOGUE("blk-rd",
                        " Issue Block-Read DMA request",
                        "-d <device> -a <full addr> -c <entry count> -s "
                        "<address step> -w <entry width 4/8/16>");

  bf_dev_id_t dev_id = 0;
  uint64_t addr = 0;
  int count = 0;
  int step = 0;
  int width = 0;
  pipe_sess_hdl_t shdl = get_pipe_mgr_ctx()->int_ses_hndl;

  int c;
  while ((c = getopt(argc, argv, "d:a:c:s:w:")) != -1) {
    switch (c) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev_id = strtoul(optarg, NULL, 0);
        break;
      case 'a':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        addr = strtoull(optarg, NULL, 0);
        break;
      case 'c':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        count = strtoul(optarg, NULL, 0);
        break;
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        step = strtoul(optarg, NULL, 0);
        break;
      case 'w':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        width = strtoul(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    aim_printf(&uc->pvs, "Device %d not found\n", dev_id);
    return UCLI_STATUS_OK;
  }

  if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO) {
    if (step != 1 && step != 4) {
      aim_printf(&uc->pvs, "Step must be 1 or 4\n");
      return UCLI_STATUS_OK;
    }
  } else {
    if (step != 1 && step != 2 && step != 4 && step != 8 && step != 16 &&
        step != 32) {
      aim_printf(&uc->pvs, "Step must be 1, 2, 4, 8, 16, or 32\n");
      return UCLI_STATUS_OK;
    }
  }

  if (width != 4 && width != 8 && width != 16) {
    aim_printf(&uc->pvs, "Width must be 4, 8, or 16\n");
    return UCLI_STATUS_OK;
  }

  if (!dev_info->dev_cfg.is_pipe_addr(addr)) {
    aim_printf(
        &uc->pvs, "Address 0x%" PRIx64 " is not a pipe ring address\n", addr);
    return UCLI_STATUS_OK;
  }

  struct pipe_mgr_ucli_rd_blk_ctx *ctx = PIPE_MGR_MALLOC(sizeof *ctx);
  if (!ctx) {
    aim_printf(&uc->pvs, "malloc failure\n");
    return UCLI_STATUS_OK;
  }
  ctx->uc = uc;
  ctx->width = width;
  ctx->count = count;
  ctx->step = step;
  ctx->addr = addr;

  pipe_status_t sts = pipe_mgr_drv_blk_rd(&shdl,
                                          dev_info->dev_id,
                                          width,
                                          count,
                                          step,
                                          addr,
                                          blk_rd_cb,
                                          (void *)ctx);
  if (sts != PIPE_SUCCESS) {
    aim_printf(&uc->pvs, "Read block failed, error %s\n", pipe_str_err(sts));
    PIPE_MGR_FREE(ctx);
    return UCLI_STATUS_OK;
  }

  pipe_mgr_drv_rd_blk_cmplt_all(shdl, dev_info->dev_id);

  PIPE_MGR_FREE(ctx);
  return UCLI_STATUS_OK;
}

#ifdef TOF2_SBC_VAL
static int ilist_len[4] = {0, 0, 0, 0};
static int ilist_snf_len[4] = {0, 0, 0, 0};
static int wlist_len[2] = {0, 0};
void pipe_mgr_ucli_dbg_ilist_cmplt_cb(bf_dev_id_t dev_id,
                                      bf_dma_dr_id_t dr,
                                      uint64_t ts_sz,
                                      uint32_t attr,
                                      uint32_t status,
                                      uint32_t type,
                                      uint64_t msg_id,
                                      int s,
                                      int e) {
  (void)ts_sz;
  (void)attr;
  (void)status;
  (void)type;
  (void)s;
  (void)e;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return;
  int phy_pipe_msk = 0;
  for (unsigned int i = 0; i < dev_info->num_active_pipes; ++i) {
    bf_dev_pipe_t I = i;
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, i, &I);
    phy_pipe_msk |= 1 << I;
  }

  int which_dr = dr - lld_dr_cmp_pipe_inst_list_0;
  if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO)
    lld_push_ilist(
        dev_id, which_dr, msg_id, ilist_len[which_dr], 0, false, 0, msg_id);
  else
    lld_push_ilist_mcast(dev_id,
                         which_dr,
                         msg_id /* dma addr */,
                         ilist_len[which_dr],
                         0,
                         false,
                         phy_pipe_msk,
                         0,
                         msg_id);
  lld_dr_start(dev_id, 0, lld_dr_tx_pipe_inst_list_0 + which_dr);
}
void pipe_mgr_ucli_dbg_ilist_snf_cmplt_cb(bf_dev_id_t dev_id,
                                          bf_dma_dr_id_t dr,
                                          uint64_t ts_sz,
                                          uint32_t attr,
                                          uint32_t status,
                                          uint32_t type,
                                          uint64_t msg_id,
                                          int s,
                                          int e) {
  (void)ts_sz;
  (void)attr;
  (void)status;
  (void)type;
  (void)s;
  (void)e;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return;

  int phy_pipe_msk = 0;
  for (unsigned int i = 0; i < dev_info->num_active_pipes; ++i) {
    bf_dev_pipe_t I = i;
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, i, &I);
    phy_pipe_msk |= 1 << I;
  }

  int which_dr = dr - lld_dr_cmp_pipe_inst_list_0;
  lld_push_ilist_mcast(dev_id,
                       which_dr,
                       msg_id /* dma addr */,
                       ilist_snf_len[which_dr],
                       0,
                       true,
                       phy_pipe_msk,
                       0,
                       msg_id);
  lld_dr_start(dev_id, 0, lld_dr_tx_pipe_inst_list_0 + which_dr);
}
void pipe_mgr_ucli_dbg_wb_cb(bf_dev_id_t dev_id,
                             bf_dma_dr_id_t dr,
                             uint64_t ts_sz,
                             uint32_t attr,
                             uint32_t status,
                             uint32_t type,
                             uint64_t msg_id,
                             int s,
                             int e) {
  (void)dr;
  (void)ts_sz;
  (void)attr;
  (void)status;
  (void)type;
  (void)s;
  (void)e;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return;
  int phy_pipe_msk = 0;
  for (unsigned int i = 0; i < dev_info->num_active_pipes; ++i) {
    bf_dev_pipe_t I = i;
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, i, &I);
    phy_pipe_msk |= 1 << I;
  }

  pipe_physical_addr_t a;
  a.addr = 0;
  if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO) {
    a.tof.mem_col = 2;
    a.tof.mem_row = 0;
    a.tof.mem_type = pipe_mem_type_unit_ram;
    a.tof.pipe_ring_addr_type = addr_type_memdata;
    a.tof.pipe_element_36_33 = 1;
    a.tof.pipe_41_40 = 2;
  } else if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO2) {
    a.tof2.mem_col = 2;
    a.tof2.mem_row = 0;
    a.tof2.mem_type = pipe_mem_type_unit_ram;
    a.tof2.pipe_ring_addr_type = addr_type_memdata;
    a.tof2.pipe_stage = 1;
    a.tof2.pipe_always_1 = 1;
  } else if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO3) {
    a.tof3.mem_col = 2;
    a.tof3.mem_row = 0;
    a.tof3.mem_type = pipe_mem_type_unit_ram;
    a.tof3.pipe_ring_addr_type = addr_type_memdata;
    a.tof3.pipe_stage = 1;
    a.tof3.pipe_always_1 = 1;
  }

  lld_push_wb_mcast(
      dev_id, 16, 1, 1024, false, msg_id, a.addr, phy_pipe_msk, msg_id);
  lld_dr_start(dev_id, 0, lld_dr_tx_pipe_write_block);
}
void pipe_mgr_ucli_dbg_wl_cb(bf_dev_id_t dev_id,
                             bf_dma_dr_id_t dr,
                             uint64_t ts_sz,
                             uint32_t attr,
                             uint32_t status,
                             uint32_t type,
                             uint64_t msg_id,
                             int s,
                             int e) {
  (void)ts_sz;
  (void)attr;
  (void)status;
  (void)type;
  (void)s;
  (void)e;
  int which_dr = dr == lld_dr_cmp_que_write_list ? 0 : 1;
  lld_push_wl(dev_id, which_dr, 4, wlist_len[which_dr], msg_id, msg_id);
  if (which_dr == 0)
    lld_dr_start(dev_id, 0, lld_dr_tx_que_write_list);
  else
    lld_dr_start(dev_id, 0, lld_dr_tx_que_write_list_1);
}
void pipe_mgr_ucli_dbg_pkt_tx_cb(bf_dev_id_t dev_id,
                                 bf_dma_dr_id_t dr,
                                 uint64_t ts_sz,
                                 uint32_t attr,
                                 uint32_t status,
                                 uint32_t type,
                                 uint64_t msg_id,
                                 int s,
                                 int e) {
  (void)ts_sz;
  (void)attr;
  (void)status;
  (void)type;
  (void)s;
  (void)e;
  int which_dr = dr - lld_dr_cmp_tx_pkt_0;
  lld_push_tx_packet(dev_id, which_dr, 1, 1, 9 * 1024, msg_id, msg_id);
  lld_dr_start(dev_id, 0, lld_dr_tx_pkt_0 + which_dr);
}
void pipe_mgr_ucli_dbg_pkt_rx_cb(bf_dev_id_t dev_id,
                                 int data_sz,
                                 bf_dma_addr_t address,
                                 int s,
                                 int e,
                                 int cos) {
  (void)data_sz;
  (void)s;
  (void)e;
  lld_push_fm(dev_id, lld_dr_fm_pkt_0 + cos, address, 2 * 1024);
  lld_dr_start(dev_id, 0, lld_dr_fm_pkt_0 + cos);
}
void pipe_mgr_ucli_dbg_lrt_cb(bf_dev_id_t dev_id,
                              int size,
                              bf_dma_addr_t dma_addr) {
  (void)size;
  lld_push_fm(dev_id, lld_dr_fm_lrt, dma_addr, 512);
  lld_dr_start(dev_id, 0, lld_dr_fm_lrt);
}
void pipe_mgr_ucli_dbg_idle_cb(bf_dev_id_t dev_id,
                               int size,
                               bf_dma_addr_t dma_addr) {
  (void)size;
  lld_push_fm(dev_id, lld_dr_fm_idle, dma_addr, 32 * 1024);
  lld_dr_start(dev_id, 0, lld_dr_fm_idle);
}

struct ilist_noise_args {
  bf_dev_id_t dev_id;
  int which_dr;
  int stage;
  int idle_lt;
  int stat_lt;
};
void ilist_noise(struct ilist_noise_args arg) {
  bf_dev_id_t dev_id = arg.dev_id;
  pipe_mgr_drv_buf_t *b =
      pipe_mgr_drv_buf_alloc(0, dev_id, 16 * 1024, PIPE_MGR_DRV_BUF_IL, false);
  if (!b) return;
  uint8_t *buf = b->addr;

  int stage = arg.stage;      // 9;
  int idle_lt = arg.idle_lt;  // 1;
  int stat_lt = arg.stat_lt;  // 1;
  int which_dr = arg.which_dr;
  ilist_len[which_dr] = 0;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return;
  int phy_pipe_msk = 0;
  for (unsigned int i = 0; i < dev_info->num_active_pipes; ++i) {
    bf_dev_pipe_t I = i;
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, i, &I);
    phy_pipe_msk |= 1 << I;
  }

  /* Set pipe and stage. */
  dest_select_stage_t dest;
  construct_instr_dest_select_stage(dev_id, &dest, stage);
  PIPE_MGR_MEMCPY(buf, &dest, sizeof dest);
  buf += sizeof dest; /* 8 */
  ilist_len[which_dr] += sizeof dest;
  for (int i = 0; i < 1023; ++i) {
    /* Add idle and stats barrier instructions. */
    pipe_barrier_lock_instr_t bar;
    construct_instr_barrier_idle(dev_id, &bar, 0x0917, idle_lt);
    PIPE_MGR_MEMCPY(buf, &bar, sizeof bar);
    buf += sizeof bar; /* 4 */
    ilist_len[which_dr] += sizeof bar;
    construct_instr_barrier_stats(dev_id, &bar, 0x0128, stat_lt);
    PIPE_MGR_MEMCPY(buf, &bar, sizeof bar);
    buf += sizeof bar; /* 4 */
    ilist_len[which_dr] += sizeof bar;
    /* Write a scratch register. */
    pipe_instr_write_reg_t reg;
    construct_instr_reg_write(
        dev_id,
        &reg,
        dev_info->dev_family == BF_DEV_FAMILY_TOFINO
            ? offsetof(Tofino, pipes[0].mau[stage].dp.mau_scratch)
            : offsetof(tof2_reg, pipes[0].mau[stage].dp.mau_scratch),
        i);
    PIPE_MGR_MEMCPY(buf, &reg, sizeof reg);
    buf += sizeof reg; /* 8 */
    ilist_len[which_dr] += sizeof reg;
  }

  bf_dma_addr_t dma_addr;
  bf_sys_dma_map(
      b->pool, b->addr, b->phys_addr, b->size, &dma_addr, BF_DMA_FROM_CPU);
  uint64_t msg_id = dma_addr;
  int ret;
  do {
    if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO)
      ret = lld_push_ilist(
          dev_id, which_dr, dma_addr, ilist_len[which_dr], 0, false, 0, msg_id);
    else
      ret = lld_push_ilist_mcast(dev_id,
                                 which_dr,
                                 dma_addr,
                                 ilist_len[which_dr],
                                 0,
                                 false,
                                 phy_pipe_msk,
                                 0,
                                 msg_id);
  } while (ret == 0);

  lld_register_completion_callback(dev_id,
                                   0,
                                   lld_dr_cmp_pipe_inst_list_0 + which_dr,
                                   pipe_mgr_ucli_dbg_ilist_cmplt_cb);
  lld_register_rx_lrt_callback(dev_id, 0, pipe_mgr_ucli_dbg_lrt_cb);
  lld_register_rx_idle_callback(dev_id, 0, pipe_mgr_ucli_dbg_idle_cb);

  lld_dr_start(dev_id, 0, lld_dr_tx_pipe_inst_list_0 + which_dr);
}
void ilist_snf(bf_dev_id_t dev_id, int which_dr, int stage) {
  pipe_mgr_drv_buf_t *b =
      pipe_mgr_drv_buf_alloc(0, dev_id, 4 * 1024, PIPE_MGR_DRV_BUF_IL, false);
  if (!b) return;
  uint8_t *buf = b->addr;

  ilist_snf_len[which_dr] = 0;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return;
  if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO) return;

  int phy_pipe_msk = 0;
  for (unsigned int i = 0; i < dev_info->num_active_pipes; ++i) {
    bf_dev_pipe_t I = i;
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, i, &I);
    phy_pipe_msk |= 1 << I;
  }

  /* Set pipe and stage. */
  dest_select_stage_t dest;
  construct_instr_dest_select_stage(dev_id, &dest, stage);
  PIPE_MGR_MEMCPY(buf, &dest, sizeof dest);
  buf += sizeof dest; /* 8 */
  ilist_snf_len[which_dr] += sizeof dest;
  while (true) {
    /* Write a scratch register. */
    pipe_instr_write_reg_t reg;
    if (ilist_snf_len[which_dr] <= (4096 - 16)) {
      construct_instr_reg_write(
          dev_id,
          &reg,
          offsetof(tof2_reg, pipes[0].mau[stage].dp.mau_scratch),
          0x1027);
    } else if (ilist_snf_len[which_dr] <= (4096 - 8)) {
      construct_instr_reg_write(
          dev_id,
          &reg,
          offsetof(tof2_reg, pipes[0].mau[stage].dp.mau_scratch),
          0x917);
    } else {
      break;
    }
    PIPE_MGR_MEMCPY(buf, &reg, sizeof reg);
    buf += sizeof reg; /* 8 */
    ilist_snf_len[which_dr] += sizeof reg;
  }

  bf_dma_addr_t dma_addr;
  bf_sys_dma_map(
      b->pool, b->addr, b->phys_addr, b->size, &dma_addr, BF_DMA_FROM_CPU);
  uint64_t msg_id = dma_addr;
  int ret;
  do {
    ret = lld_push_ilist_mcast(dev_id,
                               which_dr,
                               dma_addr,
                               ilist_snf_len[which_dr],
                               0,
                               true,
                               phy_pipe_msk,
                               0,
                               msg_id);
  } while (ret == 0);

  lld_register_completion_callback(dev_id,
                                   0,
                                   lld_dr_cmp_pipe_inst_list_0 + which_dr,
                                   pipe_mgr_ucli_dbg_ilist_snf_cmplt_cb);

  lld_dr_start(dev_id, 0, lld_dr_tx_pipe_inst_list_0 + which_dr);
}

struct wb_noise_args {
  bf_dev_id_t dev_id;
  int stage;
  int row;
  int col;
};
void wb_noise(struct wb_noise_args arg) {
  bf_dev_id_t dev_id = arg.dev_id;
  int stage = arg.stage;
  int row = arg.row;
  int col = arg.col;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return;
  int phy_pipe_msk = 0;
  for (unsigned int i = 0; i < dev_info->num_active_pipes; ++i) {
    bf_dev_pipe_t I = i;
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, i, &I);
    phy_pipe_msk |= 1 << I;
  }

  pipe_mgr_drv_buf_t *b =
      pipe_mgr_drv_buf_alloc(0, dev_id, 16 * 1024, PIPE_MGR_DRV_BUF_IL, false);
  if (!b) return;
  PIPE_MGR_MEMSET(b->addr, 0xA1, 16 * 1024);
  bf_dma_addr_t dma_addr;
  bf_sys_dma_map(
      b->pool, b->addr, b->phys_addr, b->size, &dma_addr, BF_DMA_FROM_CPU);
  uint64_t msg_id = dma_addr;
  pipe_physical_addr_t a;
  a.addr = 0;
  if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO) {
    a.tof.mem_col = col;
    a.tof.mem_row = row;
    a.tof.mem_type = pipe_mem_type_unit_ram;
    a.tof.pipe_ring_addr_type = addr_type_memdata;
    a.tof.pipe_element_36_33 = stage;
    a.tof.pipe_41_40 = 2;
  } else if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO2) {
    a.tof2.mem_col = col;
    a.tof2.mem_row = row;
    a.tof2.mem_type = pipe_mem_type_unit_ram;
    a.tof2.pipe_ring_addr_type = addr_type_memdata;
    a.tof2.pipe_stage = stage;
    a.tof2.pipe_always_1 = 1;
  } else if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO3) {
    a.tof3.mem_col = col;
    a.tof3.mem_row = row;
    a.tof3.mem_type = pipe_mem_type_unit_ram;
    a.tof3.pipe_ring_addr_type = addr_type_memdata;
    a.tof3.pipe_stage = stage;
    a.tof3.pipe_always_1 = 1;
  }
  int ret;
  do {
    ret = lld_push_wb_mcast(
        dev_id, 16, 1, 1024, false, dma_addr, a.addr, phy_pipe_msk, msg_id);
  } while (ret == 0);

  lld_register_completion_callback(
      dev_id, 0, lld_dr_cmp_pipe_write_blk, pipe_mgr_ucli_dbg_wb_cb);

  lld_dr_start(dev_id, 0, lld_dr_tx_pipe_write_block);
}

struct wl_noise_args {
  bf_dev_id_t dev_id;
  int which_dr;
};
void wl_noise(struct wl_noise_args arg) {
  if (arg.which_dr != 0 && arg.which_dr != 1) return;

  bf_dev_id_t dev_id = arg.dev_id;
  int which_dr = arg.which_dr;
  int dr =
      which_dr == 0 ? lld_dr_tx_que_write_list : lld_dr_tx_que_write_list_1;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return;

  if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO && which_dr == 1) return;

  uint64_t mem_addr = 0;
  if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO) {
    uint32_t reg_addr = offsetof(
        Tofino,
        device_select.tm_top.tm_wac_top.wac_common.wac_common.wac_scratch);
    mem_addr = tof_dir_to_indir_dev_sel(reg_addr);
  } else if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO2) {
    uint32_t reg_addr = offsetof(
        tof2_reg,
        device_select.tm_top.tm_wac_top.wac_common.wac_common.wac_scratch);
    mem_addr = tof2_dir_to_indir_dev_sel(reg_addr);
  } else if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO3) {
    uint32_t reg_addr = offsetof(
        tof3_reg,
        device_select.tm_top.tm_wac_top.wac_common.wac_common.wac_scratch);
    mem_addr = tof3_dir_to_indir_dev_sel(reg_addr);
  }

  pipe_mgr_drv_buf_t *b =
      pipe_mgr_drv_buf_alloc(0, dev_id, 16 * 1024, PIPE_MGR_DRV_BUF_IL, false);
  if (!b) return;

  int wl_len = 0;
  for (int i = 0; i < 16 * 1024 / 12; ++i) {
    b->addr[wl_len++] = (mem_addr >> 0) & 0xFF;
    b->addr[wl_len++] = (mem_addr >> 8) & 0xFF;
    b->addr[wl_len++] = (mem_addr >> 16) & 0xFF;
    b->addr[wl_len++] = (mem_addr >> 24) & 0xFF;
    b->addr[wl_len++] = (mem_addr >> 32) & 0xFF;
    b->addr[wl_len++] = (mem_addr >> 40) & 0xFF;
    b->addr[wl_len++] = (mem_addr >> 48) & 0xFF;
    b->addr[wl_len++] = (mem_addr >> 56) & 0xFF;
    b->addr[wl_len++] = (i >> 0) & 0xFF;
    b->addr[wl_len++] = (i >> 8) & 0xFF;
    b->addr[wl_len++] = (i >> 16) & 0xFF;
    b->addr[wl_len++] = (i >> 24) & 0xFF;
  }
  wlist_len[which_dr] = wl_len;

  bf_dma_addr_t dma_addr;
  bf_sys_dma_map(
      b->pool, b->addr, b->phys_addr, b->size, &dma_addr, BF_DMA_FROM_CPU);
  uint64_t msg_id = dma_addr;
  int ret;
  do {
    ret =
        lld_push_wl(dev_id, which_dr, 4, wlist_len[which_dr], dma_addr, msg_id);
  } while (ret == 0);

  lld_register_completion_callback(
      dev_id, 0, lld_dr_cmp_que_write_list, pipe_mgr_ucli_dbg_wl_cb);
  lld_register_completion_callback(
      dev_id, 0, lld_dr_cmp_que_write_list_1, pipe_mgr_ucli_dbg_wl_cb);

  lld_dr_start(dev_id, 0, dr);
}

struct pkt_noise_args {
  bf_dev_id_t dev_id;
  int which_dr;
};
static void pkt_noise(struct pkt_noise_args arg) {
  bf_dev_id_t dev_id = arg.dev_id;
  int which_dr = arg.which_dr;
  int i;

  pipe_mgr_drv_buf_t *b[8];
  for (i = 0; i < 8; ++i) {
    b[i] = pipe_mgr_drv_buf_alloc(
        0, dev_id, 16 * 1024, PIPE_MGR_DRV_BUF_IL, false);
    if (!b[i]) return;
    PIPE_MGR_MEMSET(b[i]->addr, 0x55, 16 * 1024);
    b[i]->addr[0] = i;
  }

  int ret;
  i = 0;
  do {
    bf_dma_addr_t dma_addr;
    bf_sys_dma_map(b[i]->pool,
                   b[i]->addr,
                   b[i]->phys_addr,
                   b[i]->size,
                   &dma_addr,
                   BF_DMA_FROM_CPU);
    uint64_t msg_id = dma_addr;
    ret =
        lld_push_tx_packet(dev_id, which_dr, 1, 1, 9 * 1024, dma_addr, msg_id);
    i = (i + 1) & 7;
  } while (ret == 0);

  lld_register_completion_callback(
      dev_id, 0, lld_dr_cmp_tx_pkt_0 + which_dr, pipe_mgr_ucli_dbg_pkt_tx_cb);

  lld_register_rx_packet_callback(dev_id, 0, 0, pipe_mgr_ucli_dbg_pkt_rx_cb);
  lld_register_rx_packet_callback(dev_id, 0, 1, pipe_mgr_ucli_dbg_pkt_rx_cb);
  lld_register_rx_packet_callback(dev_id, 0, 2, pipe_mgr_ucli_dbg_pkt_rx_cb);
  lld_register_rx_packet_callback(dev_id, 0, 3, pipe_mgr_ucli_dbg_pkt_rx_cb);
  lld_register_rx_packet_callback(dev_id, 0, 4, pipe_mgr_ucli_dbg_pkt_rx_cb);
  lld_register_rx_packet_callback(dev_id, 0, 5, pipe_mgr_ucli_dbg_pkt_rx_cb);
  lld_register_rx_packet_callback(dev_id, 0, 6, pipe_mgr_ucli_dbg_pkt_rx_cb);
  lld_register_rx_packet_callback(dev_id, 0, 7, pipe_mgr_ucli_dbg_pkt_rx_cb);

  lld_dr_start(dev_id, 0, lld_dr_tx_pkt_0 + which_dr);
}

PIPE_MGR_CLI_CMD_DECLARE(dma_noise) {
  PIPE_MGR_CLI_PROLOGUE("dma-noise", " Start DMA", "-d <device>");
  bf_dev_id_t dev_id = 0;
  int c;
  while ((c = getopt(argc, argv, "d:")) != -1) {
    switch (c) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev_id = strtoul(optarg, NULL, 0);
        break;
    }
  }

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return UCLI_STATUS_OK;

  struct ilist_noise_args il0, il1, il2, il3;
  il0.dev_id = dev_id;
  /* table t2 */
  il0.stage = 9;
  il0.idle_lt = 1;
  il0.stat_lt = 1;
  il0.which_dr = 0;

  il1.dev_id = dev_id;
  /* table e2 */
  il1.stage = 9;
  il1.idle_lt = 0;
  il1.stat_lt = 0;
  il1.which_dr = 1;

  il2.dev_id = dev_id;
  /* table t1 */
  il2.stage = 9;
  il2.idle_lt = 3;
  il2.stat_lt = 3;
  il2.which_dr = 2;

  il3.dev_id = dev_id;
  /* table e1 */
  il3.stage = 9;
  il3.idle_lt = 2;
  il3.stat_lt = 2;
  il3.which_dr = 3;

  ilist_noise(il0);
  ilist_noise(il1);
  ilist_noise(il2);
  ilist_noise(il3);

  struct wb_noise_args wb;
  wb.dev_id = dev_id;
  wb.stage = 1;
  wb.row = 7;
  wb.col = 2;
  wb_noise(wb);

  for (int i = 0; i < 2; ++i) {
    struct wl_noise_args a;
    a.dev_id = dev_id;
    a.which_dr = i;
    wl_noise(a);
  }

  for (int i = 0; i < 4; ++i) {
    struct pkt_noise_args a;
    a.dev_id = dev_id;
    a.which_dr = i;
    pkt_noise(a);
  }
  return UCLI_STATUS_OK;
}
PIPE_MGR_CLI_CMD_DECLARE(dma_snf) {
  PIPE_MGR_CLI_PROLOGUE("dma-snf", " Start DMA", "-d <device>");
  bf_dev_id_t dev_id = 0;
  int c;
  while ((c = getopt(argc, argv, "d:")) != -1) {
    switch (c) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev_id = strtoul(optarg, NULL, 0);
        break;
    }
  }

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return UCLI_STATUS_OK;

  ilist_snf(dev_id, 0, 0);
  ilist_snf(dev_id, 1, 1);
  ilist_snf(dev_id, 2, 2);
  ilist_snf(dev_id, 3, 3);

  return UCLI_STATUS_OK;
}

#endif

PIPE_MGR_CLI_CMD_DECLARE(overspeed_25g_set) {
  PIPE_MGR_CLI_PROLOGUE("overspeed_25g_set",
                        " Set 25g overspeed mode",
                        "Usage: overspeed_25g_set -d <dev_id> -p <dev_port> -e "
                        "<0:disable 1:enable>");
  int c;
  bf_dev_id_t dev_id = 0;
  bf_dev_port_t dev_port = 0;
  bool enable = true;

  while ((c = getopt(argc, argv, "d:p:e:")) != -1) {
    switch (c) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev_id = strtoul(optarg, NULL, 0);
        break;
      case 'p':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev_port = strtoul(optarg, NULL, 0);
        break;
      case 'e':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        enable = strtoul(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }
  bf_status_t sts =
      bf_pipe_mgr_25g_overspeed_mode_set(dev_id, dev_port, enable);
  if (sts == BF_SUCCESS) {
    aim_printf(&uc->pvs,
               "Set 25g overspeed mode for port %d on device %d to %s\n",
               dev_port,
               dev_id,
               enable ? "enabled" : "disabled");
  } else {
    aim_printf(
        &uc->pvs,
        "Failed to %s 25g overspeed mode for port %d on device %d, error %s\n",
        enable ? "enable" : "disable",
        dev_port,
        dev_id,
        bf_err_str(sts));
  }
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(overspeed_25g_get) {
  PIPE_MGR_CLI_PROLOGUE("overspeed_25g_get",
                        " Get 25g overspeed mode",
                        "Usage: overspeed_25g_set -d <dev_id> [-p <dev_port>]");
  int c;
  bf_dev_id_t dev_id = 0;
  bf_dev_port_t dev_port = 0;
  bool all = true;

  while ((c = getopt(argc, argv, "d:p:")) != -1) {
    switch (c) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev_id = strtoul(optarg, NULL, 0);
        break;
      case 'p':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev_port = strtoul(optarg, NULL, 0);
        all = false;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }
  bool enabled = false;
  if (!all) {
    bf_status_t sts =
        bf_pipe_mgr_25g_overspeed_mode_get(dev_id, dev_port, &enabled);
    if (sts == BF_SUCCESS) {
      aim_printf(&uc->pvs,
                 "Dev %d port %d 25g overspeed is %s\n",
                 dev_id,
                 dev_port,
                 enabled ? "enabled" : "disabled");
    } else {
      aim_printf(&uc->pvs,
                 "Dev %d port %d, unable to get 25g overspeed mode, error %s\n",
                 dev_id,
                 dev_port,
                 bf_err_str(sts));
    }
  } else {
    bf_map_sts_t msts;
    for (msts = pipe_mgr_overspeed_map_get_first(dev_id, &dev_port);
         msts == BF_MAP_OK;
         msts = pipe_mgr_overspeed_map_get_next(dev_id, &dev_port)) {
      aim_printf(&uc->pvs,
                 "Dev %d port %3d 25g overspeed is enabled\n",
                 dev_id,
                 dev_port);
    }
  }
  return UCLI_STATUS_OK;
}

/**
 * Pipemgr Sub-module debug log enable
 */

PIPE_MGR_CLI_CMD_DECLARE(debug_log_enable) {
  PIPE_MGR_CLI_PROLOGUE(
      "debug-log-enable",
      " submodule level debug log enable",
      "[ALL|ACT|ADT|ALPM|EXM|IDLE|INTR|METER|MIRROR|PKTGEN|SEL|"
      "SESS|STAT|STFUL|TCM]");
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_submodule_t module = PIPE_MGR_LOG_DBG_ALL;

  if (argc > 2) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_E_ARG;
  }

  if (argc == 2) {
    if (!strncmp("ALL", argv[1], 3)) {
      module = PIPE_MGR_LOG_DBG_ALL;
    } else if (!strncmp("ACT", argv[1], 3)) {
      module = PIPE_MGR_LOG_DBG_ACTION;
    } else if (!strncmp("ADT", argv[1], 3)) {
      module = PIPE_MGR_LOG_DBG_ADT;
    } else if (!strncmp("ALPM", argv[1], 4)) {
      module = PIPE_MGR_LOG_DBG_ALPM;
    } else if (!strncmp("EXM", argv[1], 3)) {
      module = PIPE_MGR_LOG_DBG_EXM;
    } else if (!strncmp("IDLE", argv[1], 4)) {
      module = PIPE_MGR_LOG_DBG_IDLE;
    } else if (!strncmp("INTR", argv[1], 4)) {
      module = PIPE_MGR_LOG_DBG_INTERRUPT;
    } else if (!strncmp("METER", argv[1], 5)) {
      module = PIPE_MGR_LOG_DBG_METER;
    } else if (!strncmp("MIRROR", argv[1], 6)) {
      module = PIPE_MGR_LOG_DBG_MIRROR;
    } else if (!strncmp("PKTGEN", argv[1], 6)) {
      module = PIPE_MGR_LOG_DBG_PKTGEN;
    } else if (!strncmp("SEL", argv[1], 3)) {
      module = PIPE_MGR_LOG_DBG_SELECTOR;
    } else if (!strncmp("SESS", argv[1], 4)) {
      module = PIPE_MGR_LOG_DBG_SESSION;
    } else if (!strncmp("STAT", argv[1], 4)) {
      module = PIPE_MGR_LOG_DBG_STAT;
    } else if (!strncmp("STFUL", argv[1], 5)) {
      module = PIPE_MGR_LOG_DBG_STFUL;
    } else if (!strncmp("TCM", argv[1], 3)) {
      module = PIPE_MGR_LOG_DBG_TCAM;
    } else {
      aim_printf(&uc->pvs, "%s\n", usage);
      return UCLI_STATUS_E_ARG;
    }
  }
  status = pipe_mgr_submodule_debug_set(module);
  if (status != PIPE_SUCCESS) {
    aim_printf(&uc->pvs, "Error enabling submodule debug log\n");
  } else {
    aim_printf(&uc->pvs, "Enabling pipemgr %s debug log\n", argv[1]);
  }
  return UCLI_STATUS_OK;
}

/**
 * Pipemgr Sub-module debug log disable
 */

PIPE_MGR_CLI_CMD_DECLARE(debug_log_disable) {
  PIPE_MGR_CLI_PROLOGUE(
      "debug-log-disable",
      " submodule level debug log disable",
      "[ALL|ACT|ADT|ALPM|EXM|IDLE|INTR|METER|MIRROR|PKTGEN|SEL|"
      "SESS|STAT|STFUL|TCM]");
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_submodule_t module = PIPE_MGR_LOG_DBG_ALL;

  if (argc > 2) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_E_ARG;
  }

  if (argc == 2) {
    if (!strncmp("ALL", argv[1], 3)) {
      module = PIPE_MGR_LOG_DBG_ALL;
    } else if (!strncmp("ACT", argv[1], 3)) {
      module = PIPE_MGR_LOG_DBG_ACTION;
    } else if (!strncmp("ADT", argv[1], 3)) {
      module = PIPE_MGR_LOG_DBG_ADT;
    } else if (!strncmp("ALPM", argv[1], 4)) {
      module = PIPE_MGR_LOG_DBG_ALPM;
    } else if (!strncmp("EXM", argv[1], 3)) {
      module = PIPE_MGR_LOG_DBG_EXM;
    } else if (!strncmp("IDLE", argv[1], 4)) {
      module = PIPE_MGR_LOG_DBG_IDLE;
    } else if (!strncmp("INTR", argv[1], 4)) {
      module = PIPE_MGR_LOG_DBG_INTERRUPT;
    } else if (!strncmp("METER", argv[1], 5)) {
      module = PIPE_MGR_LOG_DBG_METER;
    } else if (!strncmp("MIRROR", argv[1], 6)) {
      module = PIPE_MGR_LOG_DBG_MIRROR;
    } else if (!strncmp("PKTGEN", argv[1], 6)) {
      module = PIPE_MGR_LOG_DBG_PKTGEN;
    } else if (!strncmp("SEL", argv[1], 3)) {
      module = PIPE_MGR_LOG_DBG_SELECTOR;
    } else if (!strncmp("SESS", argv[1], 4)) {
      module = PIPE_MGR_LOG_DBG_SESSION;
    } else if (!strncmp("STAT", argv[1], 4)) {
      module = PIPE_MGR_LOG_DBG_STAT;
    } else if (!strncmp("STFUL", argv[1], 5)) {
      module = PIPE_MGR_LOG_DBG_STFUL;
    } else if (!strncmp("TCM", argv[1], 3)) {
      module = PIPE_MGR_LOG_DBG_TCAM;
    } else {
      aim_printf(&uc->pvs, "%s\n", usage);
      return UCLI_STATUS_E_ARG;
    }
  }
  status = pipe_mgr_submodule_debug_reset(module);
  if (status != PIPE_SUCCESS) {
    aim_printf(&uc->pvs, "Error disabling submodule debug log\n");
  } else {
    aim_printf(&uc->pvs, "Disabling pipemgr %s debug log\n", argv[1]);
  }
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(gfm_rewrite) {
  PIPE_MGR_CLI_PROLOGUE(
      "gfm-rewrite",
      " Rewrite the GFM from shadow, also recomputes GFM parity",
      "-d <device> -p <log_pipe> -s <stage id>");

  bf_dev_id_t dev_id = 0;
  bf_dev_pipe_t log_pipe = BF_DEV_PIPE_ALL;
  dev_stage_t stage_id = 0xFF;
  pipe_sess_hdl_t shdl = get_pipe_mgr_ctx()->int_ses_hndl;

  int c;
  while ((c = getopt(argc, argv, "d:p:s:")) != -1) {
    switch (c) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev_id = strtoul(optarg, NULL, 0);
        break;
      case 'p':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        log_pipe = strtoull(optarg, NULL, 0);
        break;
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        stage_id = strtoul(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    aim_printf(&uc->pvs, "Device %d not found\n", dev_id);
    return UCLI_STATUS_OK;
  }

  if (stage_id != 0xFF && stage_id >= dev_info->num_active_mau) {
    aim_printf(&uc->pvs,
               "Stage %d invalid, device %d has %d stages\n",
               stage_id,
               dev_id,
               dev_info->num_active_mau);
    return UCLI_STATUS_OK;
  }

  if (log_pipe != BF_DEV_PIPE_ALL && log_pipe >= dev_info->num_active_pipes) {
    aim_printf(&uc->pvs,
               "Pipe %d invalid, device %d has %d pipes\n",
               log_pipe,
               dev_id,
               dev_info->num_active_pipes);
    return UCLI_STATUS_OK;
  }

  for (unsigned int i = 0; i < dev_info->num_active_pipes; ++i) {
    if (log_pipe != BF_DEV_PIPE_ALL && i != log_pipe) continue;
    for (unsigned int j = 0; j < dev_info->num_active_mau; ++j) {
      if (stage_id != 0xFF && j != stage_id) continue;
      pipe_status_t rc =
          pipe_mgr_write_gfm_from_shadow(shdl, dev_id, i, j, 0, 0);
      if (rc != PIPE_SUCCESS) {
        aim_printf(&uc->pvs,
                   "Error %s writing logical pipe %d stage %d\n",
                   pipe_str_err(rc),
                   i,
                   j);
      }
      rc = pipe_mgr_drv_ilist_push(&shdl, NULL, NULL);
      pipe_mgr_drv_i_list_cmplt_all(&shdl);
      pipe_mgr_drv_wr_blk_cmplt_all(shdl, dev_id);
    }
  }

  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(gfm_test) {
  PIPE_MGR_CLI_PROLOGUE("gfm-test",
                        " Rewrite the GFM with test patterns",
                        "-d <device> -p <51-bit pattern> -e <row num "
                        "(0..1023), set bad parity on row> -g <gress> "
                        "-s <stage-id, 0xFF for all>");

  bf_dev_target_t dev_tgt = {.device_id = 0, .dev_pipe_id = BF_DEV_PIPE_ALL};
  pipe_sess_hdl_t shdl = get_pipe_mgr_ctx()->int_ses_hndl;
  const int num_gfm_rows = 1024;
  int num_patterns = 0;
  uint64_t *patterns = NULL;
  uint64_t *parity_errors = NULL;
  uint64_t x;
  bf_dev_direction_t gress = BF_DEV_DIR_EGRESS;
  dev_stage_t stage_id = 0xFF;

  int c;
  while ((c = getopt(argc, argv, "d:p:e:g:s:")) != -1) {
    switch (c) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev_tgt.device_id = strtoul(optarg, NULL, 0);
        break;
      case 'g':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        x = strtoul(optarg, NULL, 0);
        gress = x ? BF_DEV_DIR_EGRESS : BF_DEV_DIR_INGRESS;
        break;
      case 'p':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        x = strtoull(optarg, NULL, 0);
        if (!patterns) {
          patterns = PIPE_MGR_MALLOC(num_gfm_rows * sizeof(uint64_t));
          if (!patterns) {
            aim_printf(&uc->pvs, "Failed to allocate memory for pattern\n");
            goto cleanup;
          }
        }
        if (num_patterns >= num_gfm_rows) {
          aim_printf(&uc->pvs, "Too many patterns: %d\n", num_patterns + 1);
          goto cleanup;
        }
        patterns[num_patterns++] = x;
        break;
      case 'e':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        x = strtoul(optarg, NULL, 0);
        if (x >= (uint64_t)num_gfm_rows) {
          aim_printf(&uc->pvs, "Bad row index: %d\n", (int)x);
          goto cleanup;
        }
        if (!parity_errors) {
          parity_errors = PIPE_MGR_CALLOC(num_gfm_rows / 64, sizeof(uint64_t));
          if (!parity_errors) {
            aim_printf(&uc->pvs,
                       "Failed to allocate memory for parity errors\n");
            goto cleanup;
          }
        }
        parity_errors[x / 64] |= 1ull << (x % 64);
        break;
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        stage_id = strtoul(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  uint32_t pipe_api_flags = PIPE_FLAG_SYNC_REQ;
  aim_printf(&uc->pvs,
             "Dev=%d, gress=%d, %d patterns:\n",
             dev_tgt.device_id,
             gress,
             num_patterns);
  for (int i = 0; i < num_patterns && patterns; ++i) {
    aim_printf(&uc->pvs, "0x%013" PRIx64 "\n", patterns[i]);
  }
  for (int i = 0; i < (num_gfm_rows / 64) && parity_errors; ++i) {
    if (!parity_errors[i]) continue;
    aim_printf(&uc->pvs,
               "Error rows %4d...%4d: 0x%" PRIx64 "\n",
               i * 64 + 63,
               i * 64,
               parity_errors[i]);
  }
  pipe_status_t rc = pipe_mgr_gfm_test_pattern_set(shdl,
                                                   dev_tgt,
                                                   pipe_api_flags,
                                                   gress,
                                                   stage_id,
                                                   num_patterns,
                                                   patterns,
                                                   parity_errors);
  aim_printf(&uc->pvs, "Status %s\n", pipe_str_err(rc));

cleanup:
  if (patterns) PIPE_MGR_FREE(patterns);
  if (parity_errors) PIPE_MGR_FREE(parity_errors);
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(gfm_col_test) {
  PIPE_MGR_CLI_PROLOGUE(
      "gfm-col-test",
      " Rewrite one GFM column",
      "-d <device> -c <column 0..51> -v <default value for unspecified rows> "
      "-r <row, 0..1023, to be assigned opposite of default value> -g <gress> "
      "-s <stage-id, 0xFF for all>");

  bf_dev_target_t dev_tgt = {.device_id = 0, .dev_pipe_id = BF_DEV_PIPE_ALL};
  pipe_sess_hdl_t shdl = get_pipe_mgr_ctx()->int_ses_hndl;
  int column = 51;
  const unsigned int num_gfm_rows = 1024;
  uint16_t col_data[64] = {0};
  int dflt = 0;
  unsigned int x;
  bf_dev_direction_t gress = BF_DEV_DIR_EGRESS;
  dev_stage_t stage_id = 0xFF;

  int c;
  while ((c = getopt(argc, argv, "d:c:v:r:g:s:")) != -1) {
    switch (c) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev_tgt.device_id = strtoul(optarg, NULL, 0);
        break;
      case 'c':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        column = strtoul(optarg, NULL, 0);
        break;
      case 'v':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dflt = strtoull(optarg, NULL, 0);
        if (dflt != 0 && dflt != 1) {
          aim_printf(&uc->pvs, "default value (%d) must be 0 or 1\n", dflt);
          return UCLI_STATUS_OK;
        }
        break;
      case 'r':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        x = strtoul(optarg, NULL, 0);
        if (x >= num_gfm_rows) {
          aim_printf(&uc->pvs, "Bad row index: %d\n", (int)x);
          return UCLI_STATUS_OK;
        }
        col_data[x / 16] |= 1ull << (x % 16);
        break;
      case 'g':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        x = strtoul(optarg, NULL, 0);
        gress = x ? BF_DEV_DIR_EGRESS : BF_DEV_DIR_INGRESS;
        break;
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        stage_id = strtoul(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (dflt) {
    /* We assumed a default of zero and the specifed rows would have a value of
     * one, but we guessed incorrectly.  Flip the data to have a default of one
     * and a value of zero for the requested rows. */
    for (int i = 0; i < 64; ++i) col_data[i] = ~col_data[i];
  }
  uint32_t pipe_api_flags = PIPE_FLAG_SYNC_REQ;
  pipe_status_t rc = pipe_mgr_gfm_test_col_set(
      shdl, dev_tgt, pipe_api_flags, gress, stage_id, column, col_data);
  aim_printf(&uc->pvs, "Status %s\n", pipe_str_err(rc));

  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(gfm_dump) {
  PIPE_MGR_CLI_PROLOGUE("gfm-dump",
                        " Dumps the GFM shadow data",
                        "-d <device> -p <log pipe> "
                        "-s <stage-id, 0xFF for all>");

  dev_stage_t stage_id = 0xFF;
  bf_dev_pipe_t log_pipe = 0;
  bf_dev_id_t dev = 0;

  int c;
  while ((c = getopt(argc, argv, "d:p:e:g:s:")) != -1) {
    switch (c) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoul(optarg, NULL, 0);
        break;
      case 'p':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        log_pipe = strtoull(optarg, NULL, 0);
        break;
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        stage_id = strtoul(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  pipe_mgr_dump_gfm_shadow(uc, dev, log_pipe, stage_id);
  return UCLI_STATUS_OK;
}

PIPE_MGR_CLI_CMD_DECLARE(hash_seed_dump) {
  PIPE_MGR_CLI_PROLOGUE("hash-seed-dump",
                        " Dumps the hash seed shadow data",
                        "-d <device> -p <log pipe> "
                        "-s <stage-id, 0xFF for all>");

  dev_stage_t stage_id = 0xFF;
  bf_dev_pipe_t log_pipe = 0;
  bf_dev_id_t dev = 0;

  int c;
  while ((c = getopt(argc, argv, "d:p:e:g:s:")) != -1) {
    switch (c) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoul(optarg, NULL, 0);
        break;
      case 'p':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        log_pipe = strtoull(optarg, NULL, 0);
        break;
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        stage_id = strtoul(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  pipe_mgr_dump_hash_seed_shadow(uc, dev, log_pipe, stage_id);
  return UCLI_STATUS_OK;
}

/* <auto.ucli.handlers.start> */
static ucli_command_handler_f pipe_mgr_ucli_ucli_handlers__[] = {
    PIPE_MGR_CLI_CMD_HNDLR(log_ilist),
    PIPE_MGR_CLI_CMD_HNDLR(drv_state),
    PIPE_MGR_CLI_CMD_HNDLR(decode_ilist),
    PIPE_MGR_CLI_CMD_HNDLR(dev),
    PIPE_MGR_CLI_CMD_HNDLR(pipe),
    PIPE_MGR_CLI_CMD_HNDLR(profile),
    PIPE_MGR_CLI_CMD_HNDLR(tbl),
    PIPE_MGR_CLI_CMD_HNDLR(dup_ent_hash_dump),
    PIPE_MGR_CLI_CMD_HNDLR(entry_count),
    PIPE_MGR_CLI_CMD_HNDLR(tbl_hw_ent_get),
    PIPE_MGR_CLI_CMD_HNDLR(tbl_hw_verify),
    PIPE_MGR_CLI_CMD_HNDLR(ipv4_route_tcam_add),
    PIPE_MGR_CLI_CMD_HNDLR(show_mat_tbl_entry),
    PIPE_MGR_CLI_CMD_HNDLR(show_act_tbl_entry),
    PIPE_MGR_CLI_CMD_HNDLR(show_act_tbl_entry_by_idx),
    PIPE_MGR_CLI_CMD_HNDLR(dump_mem),
    PIPE_MGR_CLI_CMD_HNDLR(dump_mem_full),
    PIPE_MGR_CLI_CMD_HNDLR(read_map_ram),
    PIPE_MGR_CLI_CMD_HNDLR(write_map_ram),
    PIPE_MGR_CLI_CMD_HNDLR(read_unit_ram),
    PIPE_MGR_CLI_CMD_HNDLR(write_unit_ram),
    PIPE_MGR_CLI_CMD_HNDLR(read_virt),
    PIPE_MGR_CLI_CMD_HNDLR(write_mem),
    PIPE_MGR_CLI_CMD_HNDLR(write_tcam),
    PIPE_MGR_CLI_CMD_HNDLR(invalidate_table_entry),
    PIPE_MGR_CLI_CMD_HNDLR(write_reg),
    PIPE_MGR_CLI_CMD_HNDLR(prsr_tcam_rd),
    PIPE_MGR_CLI_CMD_HNDLR(prsr_tcam_wr),
    PIPE_MGR_CLI_CMD_HNDLR(show_act_tbl_vaddr),
    PIPE_MGR_CLI_CMD_HNDLR(show_mat_tbl_vaddr),
    PIPE_MGR_CLI_CMD_HNDLR(shadow_mem),
    PIPE_MGR_CLI_CMD_HNDLR(phv_dump),
    PIPE_MGR_CLI_CMD_HNDLR(snap_create),
    PIPE_MGR_CLI_CMD_HNDLR(snap_delete),
    PIPE_MGR_CLI_CMD_HNDLR(snap_trig_add),
    PIPE_MGR_CLI_CMD_HNDLR(snap_trig_clr),
    PIPE_MGR_CLI_CMD_HNDLR(snap_state_set),
    PIPE_MGR_CLI_CMD_HNDLR(snap_timer_en),
    PIPE_MGR_CLI_CMD_HNDLR(snap_intr_clr),
    PIPE_MGR_CLI_CMD_HNDLR(snap_hdl_dump),
    PIPE_MGR_CLI_CMD_HNDLR(snap_cfg_dump),
    PIPE_MGR_CLI_CMD_HNDLR(snap_ig_mode_set),
    PIPE_MGR_CLI_CMD_HNDLR(snap_state_get),
    PIPE_MGR_CLI_CMD_HNDLR(snap_capture_get),
    PIPE_MGR_CLI_CMD_HNDLR(tbl_cntr_type_set),
    PIPE_MGR_CLI_CMD_HNDLR(tbl_cntr_clr),
    PIPE_MGR_CLI_CMD_HNDLR(tbl_cntr_print),
    PIPE_MGR_CLI_CMD_HNDLR(batch_begin),
    PIPE_MGR_CLI_CMD_HNDLR(batch_end),
    PIPE_MGR_CLI_CMD_HNDLR(pbus_irritator),
    PIPE_MGR_CLI_CMD_HNDLR(bkgrnd_stat_dump),
    PIPE_MGR_CLI_CMD_HNDLR(intr_dump),
    PIPE_MGR_CLI_CMD_HNDLR(tcam_scrub_set),
    PIPE_MGR_CLI_CMD_HNDLR(tcam_scrub_get),
    PIPE_MGR_CLI_CMD_HNDLR(port_stuck_detect_timer_on),
    PIPE_MGR_CLI_CMD_HNDLR(port_stuck_detect_timer_off),
    PIPE_MGR_CLI_CMD_HNDLR(port_set_iprsr_thresh),
    PIPE_MGR_CLI_CMD_HNDLR(port_get_iprsr_thresh),
    PIPE_MGR_CLI_CMD_HNDLR(memid_hdl),
    PIPE_MGR_CLI_CMD_HNDLR(tbl_hdl_conv),
    PIPE_MGR_CLI_CMD_HNDLR(blk_wr_perf),
    PIPE_MGR_CLI_CMD_HNDLR(imem_test),
    PIPE_MGR_CLI_CMD_HNDLR(imem_rewrite),
    PIPE_MGR_CLI_CMD_HNDLR(prsr_cfg_dump),
    PIPE_MGR_CLI_CMD_HNDLR(pps_set),
    PIPE_MGR_CLI_CMD_HNDLR(pps_get),
    PIPE_MGR_CLI_CMD_HNDLR(pps_reset),
    PIPE_MGR_CLI_CMD_HNDLR(alpm_inactive_node_delete_get),
    PIPE_MGR_CLI_CMD_HNDLR(alpm_inactive_node_delete_set),
    PIPE_MGR_CLI_CMD_HNDLR(selector_tbl_sequence_get),
    PIPE_MGR_CLI_CMD_HNDLR(selector_tbl_sequence_set),
    PIPE_MGR_CLI_CMD_HNDLR(overspeed_25g_set),
    PIPE_MGR_CLI_CMD_HNDLR(overspeed_25g_get),
    PIPE_MGR_CLI_CMD_HNDLR(debug_log_enable),
    PIPE_MGR_CLI_CMD_HNDLR(debug_log_disable),

#ifdef TOF2_SBC_VAL
    PIPE_MGR_CLI_CMD_HNDLR(dma_noise),
    PIPE_MGR_CLI_CMD_HNDLR(dma_snf),
#endif
    PIPE_MGR_CLI_CMD_HNDLR(blk_rd),
    PIPE_MGR_CLI_CMD_HNDLR(gfm_rewrite),
    PIPE_MGR_CLI_CMD_HNDLR(gfm_test),
    PIPE_MGR_CLI_CMD_HNDLR(gfm_col_test),
    PIPE_MGR_CLI_CMD_HNDLR(gfm_dump),
    PIPE_MGR_CLI_CMD_HNDLR(hash_seed_dump),
    NULL};

/* <auto.ucli.handlers.end> */

static ucli_module_t pipe_mgr_ucli_module__ = {
    "pipe_mgr_ucli",
    NULL,
    pipe_mgr_ucli_ucli_handlers__,
    NULL,
    NULL,
};

extern ucli_node_t *packet_path_ucli_node_create(ucli_node_t *);

ucli_node_t *pipe_mgr_ucli_node_create(void) {
  ucli_node_t *n;
  ucli_module_init(&pipe_mgr_ucli_module__);
  n = ucli_node_create("pipe_mgr", NULL, &pipe_mgr_ucli_module__);
  ucli_node_subnode_add(n, ucli_module_log_node_create("pipe_mgr"));
  pipe_mgr_sel_tbl_ucli_node_create(n);
  pipe_mgr_exm_tbl_ucli_node_create(n);
  pipe_mgr_adt_tbl_ucli_node_create(n);
  pipe_mgr_stat_tbl_ucli_node_create(n);
  pipe_mgr_tcam_tbl_ucli_node_create(n);
  pipe_mgr_learn_ucli_node_create(n);
  pipe_mgr_idle_ucli_node_create(n);
  pipe_mgr_meter_tbl_ucli_node_create(n);
  pipe_mgr_stful_tbl_ucli_node_create(n);
  packet_path_ucli_node_create(n);

  return n;
}

ucli_status_t bf_drv_show_tech_ucli_pipe__(ucli_context_t *uc) {
  bf_dev_id_t dev_id;
  int c, dflag = 0;
  int arg_start = 0;
  size_t i;
  int argc;
  char *const *argv;

  for (i = 0; i < sizeof(uc->pargs[0].args__) / sizeof(uc->pargs[0].args__[0]);
       ++i) {
    if (!strncmp(uc->pargs[0].args__[i],
                 "show_tech_drivers",
                 strlen("show_tech_drivers"))) {
      arg_start = i;
      break;
    }
  }

  optind = 0; /* reset optind value */
  argc = (uc->pargs->count + 1);
  argv = (char *const *)&(uc->pargs->args__[arg_start]);

  while ((c = getopt(argc, argv, "d:m:f:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        if (!optarg) {
          return UCLI_STATUS_E_ARG;
        }
        dev_id = strtoul(optarg, NULL, 0);
        break;
      default:
        break;
    }
  }

  if (!dflag) return UCLI_STATUS_E_ARG;

  uint8_t pipe = 0xff;
  aim_printf(&uc->pvs, "-------------------- PIPE --------------------\n");
  pipe_mgr_ucli_dump_pipe_info(uc, dev_id, pipe);
  pipe_mgr_ucli_dump_dev_info(uc, dev_id);
  pipe_mgr_ucli_dump_profile_info(uc, dev_id);
  pipe_mgr_ucli_dump_tbl_info(uc, dev_id, 0, 0xff);
  pipe_mgr_err_evt_log_dump(uc, dev_id, -1);
  packet_path_ucli_show_tech_drivers(uc);
  return 0;
}

bool is_dev_id_valid(unsigned dev_id) {
  rmt_dev_info_t *dev_info = NULL;
  dev_info = pipe_mgr_get_dev_info((bf_dev_id_t)dev_id);
  if (dev_info != NULL)
    return true;
  else
    return false;
}

#else
void *pipe_mgr_ucli_node_create(void) { return NULL; }
#endif
