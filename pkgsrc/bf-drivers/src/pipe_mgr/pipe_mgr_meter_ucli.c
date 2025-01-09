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
 * @file pipe_mgr_meter_ucli.c
 * @date
 *
 * Meter table manager ucli commands, command-handlers and the world.
 */

/* Standard header includes */
#include <getopt.h>
#include <limits.h>
#include <endian.h>
#include <math.h>

/* Module header includes */
#include <dvm/bf_drv_intf.h>
#include "lld/bf_dma_if.h"
#include <lld/lld_reg_if.h>
#include "lld/lld_dr_if.h"

/* Local header includes */
#include "pipe_mgr_int.h"
#include "pipe_mgr_meter_mgr_int.h"
#include "pipe_mgr_meter_drv_workflows.h"

#if PIPE_MGR_CONFIG_INCLUDE_UCLI == 1

#include <target-utils/uCli/ucli.h>
#include <target-utils/uCli/ucli_argparse.h>
#include <target-utils/uCli/ucli_handler_macros.h>

#define PIPE_MGR_METER_TBL_CLI_CMD_HNDLR(name) \
  pipe_mgr_meter_tbl_ucli_ucli__##name##__
#define PIPE_MGR_METER_TBL_CLI_CMD_DECLARE(name)                               \
  static ucli_status_t PIPE_MGR_METER_TBL_CLI_CMD_HNDLR(name)(ucli_context_t * \
                                                              uc)

PIPE_MGR_METER_TBL_CLI_CMD_DECLARE(show_vaddr) {
  extern char *optarg;
  extern int optind;
  int c, dflag, hflag, pflag, eflag;
  bf_dev_id_t dev_id = 0;
  pipe_meter_tbl_hdl_t tbl_hdl = 0;
  uint32_t entry_idx = 0;
  bf_dev_pipe_t pipe_id = 0;
  int argc;
  char *const *argv;
  static char usage[] =
      "usage : show-vaddr -d <dev_id> -h <tbl_hdl> -p <pipe_id> -e <entry idx "
      "or handle>\n";

  UCLI_COMMAND_INFO(uc,
                    "show-vaddr",
                    -1,
                    "Get an entry's virtual address"
                    " usage : show-vaddr -d <dev_id> -h <tbl_hdl> -p <pipe_id> "
                    "-e <entry idx or handle>\n");

  dflag = hflag = pflag = eflag = 0;
  optind = 0;
  argc = (uc->pargs->count + 1);
  argv = (char *const *)&(uc->pargs->args__);

  while ((c = getopt(argc, argv, "d:h:p:e:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev_id = strtoul(optarg, NULL, 0);
        if ((dev_id < 0) || (dev_id >= PIPE_MGR_NUM_DEVICES)) {
          aim_printf(&uc->pvs, "Invalid dev_id %d\n", dev_id);
          return UCLI_STATUS_OK;
        }
        break;
      case 'h':
        hflag = 1;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        tbl_hdl = strtoul(optarg, NULL, 0);
        break;
      case 'p':
        pflag = 1;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        pipe_id = strtoul(optarg, NULL, 0);
        break;
      case 'e':
        eflag = 1;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        entry_idx = strtoul(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!dflag || !hflag || !pflag || !eflag) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  dev_stage_t stage_id = 0xFF;
  if (PIPE_HDL_TYPE_MAT_TBL == PIPE_GET_HDL_TYPE(tbl_hdl)) {
    /* Direct reference meter table. */
    bf_dev_pipe_t pipe;
    uint32_t idx;
    if (PIPE_SUCCESS !=
        pipe_mgr_mat_ent_get_dir_ent_location(
            dev_id, tbl_hdl, entry_idx, 0, &pipe, &stage_id, NULL, &idx)) {
      aim_printf(
          &uc->pvs, "Cannot find entry location for entry %#x\n", entry_idx);
      return UCLI_STATUS_OK;
    }
    if (pipe != BF_DEV_PIPE_ALL && pipe_id != pipe) {
      aim_printf(
          &uc->pvs,
          "Requested entry %#x in pipe %d, but entry is actually in pipe %d\n",
          entry_idx,
          pipe_id,
          pipe);
      return UCLI_STATUS_OK;
    }
    entry_idx = idx;

    pipe_mat_tbl_info_t *info =
        pipe_mgr_get_tbl_info(dev_id, tbl_hdl, __func__, __LINE__);
    if (!info) {
      aim_printf(&uc->pvs, "Cannot find table with handle %#x\n", tbl_hdl);
      return UCLI_STATUS_OK;
    }
    tbl_hdl = info->meter_tbl_ref[0].tbl_hdl;

  } else if (PIPE_HDL_TYPE_METER_TBL == PIPE_GET_HDL_TYPE(tbl_hdl)) {
  } else {
    aim_printf(&uc->pvs, "Invalid table handle type: %#x\n", tbl_hdl);
    return UCLI_STATUS_OK;
  }

  /* Look up the meter table. */
  pipe_mgr_meter_tbl_t *meter_tbl = pipe_mgr_meter_tbl_get(dev_id, tbl_hdl);
  if (!meter_tbl) {
    aim_printf(&uc->pvs, "Cannot find table with handle %#x\n", tbl_hdl);
    return UCLI_STATUS_OK;
  }

  /* Get the instance for the requested pipe. */
  pipe_mgr_meter_tbl_instance_t *tbl_instance = NULL;
  if (meter_tbl->symmetric) {
    tbl_instance = &meter_tbl->meter_tbl_instances[0];
  } else {
    tbl_instance = pipe_mgr_meter_tbl_get_instance(meter_tbl, pipe_id);
    if (!tbl_instance) {
      aim_printf(&uc->pvs, "Invalid pipe %d\n", pipe_id);
      return UCLI_STATUS_OK;
    }
  }

  /* Ensure the index is valid in each stage. */
  bool direct_ref = stage_id != 0xFF;
  unsigned int stage = 0;
  for (; stage < tbl_instance->num_stages; ++stage) {
    if (entry_idx >= tbl_instance->meter_tbl_stage_info[stage].num_entries) {
      aim_printf(&uc->pvs, "Invalid index %d\n", entry_idx);
      return UCLI_STATUS_OK;
    }
    if (direct_ref &&
        stage_id != tbl_instance->meter_tbl_stage_info[stage].stage_id) {
      continue;
    } else if (!direct_ref) {
      stage_id = tbl_instance->meter_tbl_stage_info[stage].stage_id;
    }
    int lt_id = tbl_instance->meter_tbl_stage_info[stage].stage_table_handle;
    int ram = entry_idx / TOF_SRAM_UNIT_DEPTH;
    int line = entry_idx % TOF_SRAM_UNIT_DEPTH;
    int vpn = tbl_instance->meter_tbl_stage_info[stage]
                  .ram_alloc_info->tbl_word_blk[ram]
                  .vpn_id[0];

    int lower_addr = pipe_mgr_meter_compute_ent_virt_addr(vpn, line);
    pipe_full_virt_addr_t vaddr;
    construct_full_virt_addr(meter_tbl->dev_info,
                             &vaddr,
                             lt_id,
                             pipe_virt_mem_type_meter,
                             lower_addr,
                             pipe_id,
                             stage_id);
    uint64_t full_addr = vaddr.addr;
    aim_printf(&uc->pvs,
               "Table %#x Index %5d Stage %2d Virtual Addr 0x%" PRIx64 "\n",
               tbl_hdl,
               entry_idx,
               stage_id,
               full_addr);

    if (direct_ref) break;
  }

  return UCLI_STATUS_OK;
}

PIPE_MGR_METER_TBL_CLI_CMD_DECLARE(rd_by_index) {
  extern char *optarg;
  extern int optind;
  int c, dflag, hflag, pflag, eflag;
  bf_dev_id_t dev_id = 0;
  pipe_meter_tbl_hdl_t tbl_hdl = 0;
  uint32_t entry_idx = 0;
  bf_dev_pipe_t pipe_id = 0;
  int argc;
  char *const *argv;
  static char usage[] =
      "usage : rd-by-index -d <dev_id> -h <tbl_hdl> -p <pipe_id> -e <entry idx "
      "or handle>\n";

  UCLI_COMMAND_INFO(uc,
                    "rd-by-index",
                    -1,
                    "Virtually read a meter index"
                    " usage : rd-by-index -d <dev_id> -h <tbl_hdl> -p "
                    "<pipe_id> -e <entry idx or handle>\n");

  dflag = hflag = pflag = eflag = 0;
  optind = 0;
  argc = (uc->pargs->count + 1);
  argv = (char *const *)&(uc->pargs->args__);

  while ((c = getopt(argc, argv, "d:h:p:e:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev_id = strtoul(optarg, NULL, 0);
        if ((dev_id < 0) || (dev_id >= PIPE_MGR_NUM_DEVICES)) {
          aim_printf(&uc->pvs, "Invalid dev_id %d\n", dev_id);
          return UCLI_STATUS_OK;
        }
        break;
      case 'h':
        hflag = 1;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        tbl_hdl = strtoul(optarg, NULL, 0);
        break;
      case 'p':
        pflag = 1;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        pipe_id = strtoul(optarg, NULL, 0);
        break;
      case 'e':
        eflag = 1;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        entry_idx = strtoul(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!dflag || !hflag || !pflag || !eflag) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  dev_stage_t stage_id = 0xFF;
  if (PIPE_HDL_TYPE_MAT_TBL == PIPE_GET_HDL_TYPE(tbl_hdl)) {
    /* Direct reference meter table. */
    bf_dev_pipe_t pipe;
    uint32_t idx;
    if (PIPE_SUCCESS !=
        pipe_mgr_mat_ent_get_dir_ent_location(
            dev_id, tbl_hdl, entry_idx, 0, &pipe, &stage_id, NULL, &idx)) {
      aim_printf(
          &uc->pvs, "Cannot find entry location for entry %#x\n", entry_idx);
      return UCLI_STATUS_OK;
    }
    if (pipe != BF_DEV_PIPE_ALL && pipe_id != pipe) {
      aim_printf(
          &uc->pvs,
          "Requested entry %#x in pipe %d, but entry is actually in pipe %d\n",
          entry_idx,
          pipe_id,
          pipe);
      return UCLI_STATUS_OK;
    }
    entry_idx = idx;

    pipe_mat_tbl_info_t *info =
        pipe_mgr_get_tbl_info(dev_id, tbl_hdl, __func__, __LINE__);
    if (!info) {
      aim_printf(&uc->pvs, "Cannot find table with handle %#x\n", tbl_hdl);
      return UCLI_STATUS_OK;
    }
    tbl_hdl = info->meter_tbl_ref[0].tbl_hdl;

  } else if (PIPE_HDL_TYPE_METER_TBL == PIPE_GET_HDL_TYPE(tbl_hdl)) {
  } else {
    aim_printf(&uc->pvs, "Invalid table handle type: %#x\n", tbl_hdl);
    return UCLI_STATUS_OK;
  }

  /* Look up the meter table. */
  pipe_mgr_meter_tbl_t *meter_tbl = pipe_mgr_meter_tbl_get(dev_id, tbl_hdl);
  if (!meter_tbl) {
    aim_printf(&uc->pvs, "Cannot find table with handle %#x\n", tbl_hdl);
    return UCLI_STATUS_OK;
  }

  /* Get the instance for the requested pipe. */
  pipe_mgr_meter_tbl_instance_t *tbl_instance = NULL;
  if (meter_tbl->symmetric) {
    tbl_instance = &meter_tbl->meter_tbl_instances[0];
  } else {
    tbl_instance = pipe_mgr_meter_tbl_get_instance(meter_tbl, pipe_id);
    if (!tbl_instance) {
      aim_printf(&uc->pvs, "Invalid pipe %d\n", pipe_id);
      return UCLI_STATUS_OK;
    }
  }

  /* Ensure the index is valid in each stage. */
  bool direct_ref = stage_id != 0xFF;
  unsigned int stage = 0;
  for (; stage < tbl_instance->num_stages; ++stage) {
    if (entry_idx >= tbl_instance->meter_tbl_stage_info[stage].num_entries) {
      aim_printf(&uc->pvs, "Invalid index %d\n", entry_idx);
      return UCLI_STATUS_OK;
    }
    if (direct_ref &&
        stage_id != tbl_instance->meter_tbl_stage_info[stage].stage_id) {
      continue;
    } else if (!direct_ref) {
      stage_id = tbl_instance->meter_tbl_stage_info[stage].stage_id;
    }
    int lt_id = tbl_instance->meter_tbl_stage_info[stage].stage_table_handle;
    int ram = entry_idx / TOF_SRAM_UNIT_DEPTH;
    int line = entry_idx % TOF_SRAM_UNIT_DEPTH;
    int vpn = tbl_instance->meter_tbl_stage_info[stage]
                  .ram_alloc_info->tbl_word_blk[ram]
                  .vpn_id[0];

    int lower_addr = pipe_mgr_meter_compute_ent_virt_addr(vpn, line);
    pipe_full_virt_addr_t vaddr;
    /* Convert the logical pipe id supplied to physical pipe id */
    pipe_status_t sts;
    bf_dev_pipe_t phy_pipe;
    sts = pipe_mgr_map_pipe_id_log_to_phy(
        meter_tbl->dev_info, pipe_id, &phy_pipe);

    if (sts != PIPE_SUCCESS) {
      aim_printf(
          &uc->pvs,
          "Error in converting logical pipe id %d to physical pipe id, err %s",
          pipe_id,
          pipe_str_err(sts));
      return UCLI_STATUS_OK;
    }
    bf_subdev_id_t subdev = pipe_mgr_subdev_id_from_pipe(phy_pipe);

    construct_full_virt_addr(meter_tbl->dev_info,
                             &vaddr,
                             lt_id,
                             pipe_virt_mem_type_meter,
                             lower_addr,
                             phy_pipe,
                             stage_id);
    uint64_t data_hi = 0, data_lo = 0;
    uint64_t full_addr = vaddr.addr;
    int x = lld_subdev_ind_read(dev_id, subdev, full_addr, &data_hi, &data_lo);
    if (x) {
      aim_printf(&uc->pvs, "Indirect read failed: %d\n", x);
      return UCLI_STATUS_OK;
    } else {
      aim_printf(&uc->pvs,
                 "Table %#x Index %5d Stage %2d Virtual Addr 0x%" PRIx64
                 " DataHi/Lo 0x%016" PRIx64 "_%016" PRIx64 "\n",
                 tbl_hdl,
                 entry_idx,
                 stage_id,
                 full_addr,
                 data_hi,
                 data_lo);
      if (meter_tbl->type == PIPE_METER_TYPE_STANDARD) {
        int map_ram_line = (entry_idx / 4) % TOF_MAP_RAM_UNIT_DEPTH;
        int map_ram_idx = entry_idx / (4 * TOF_MAP_RAM_UNIT_DEPTH);
        int map_ram_subword = entry_idx % 4;
        mem_id_t map_ram_id =
            tbl_instance->meter_tbl_stage_info[stage]
                .ram_alloc_info->color_tbl_word_blk[map_ram_idx]
                .mem_id[0];
        uint64_t map_ram_addr = meter_tbl->dev_info->dev_cfg.get_full_phy_addr(
            meter_tbl->direction,
            phy_pipe,
            stage_id,
            map_ram_id,
            map_ram_line,
            pipe_mem_type_map_ram);
        uint64_t color_hi = 0, color_lo = 0;
        lld_subdev_ind_read(dev_id, subdev, map_ram_addr, &color_hi, &color_lo);
        int color = (color_lo >> (map_ram_subword * 2)) & 3;
        aim_printf(&uc->pvs,
                   " MapRAM %d Line %d Subword %d Addr 0x%" PRIx64
                   " Data 0x%" PRIx64 " Color %d\n",
                   map_ram_id,
                   map_ram_line,
                   map_ram_subword,
                   map_ram_addr,
                   color_lo,
                   color);

        pipe_mgr_meter_tof_entry_t m = {0};
        pipe_mgr_meter_decode_hw_meter_entry(&data_lo, &data_hi, &m);
        aim_printf(&uc->pvs,
                   "  CIR Mantissa/RelExp/Exponent %d/%d/%d PIR "
                   "Mantissa/RelExp/Exponent %d/%d/%d\n",
                   m.cir_mantissa,
                   m.cir_exponent,
                   31 - m.cir_exponent,
                   m.pir_mantissa,
                   m.pir_exponent,
                   31 - m.pir_exponent);
        aim_printf(
            &uc->pvs,
            "  CBS Mantissa/Exponent %d/%d PBS Mantissa/Exponent %d/%d\n",
            m.cbs_mantissa,
            m.cbs_exponent,
            m.pbs_mantissa,
            m.pbs_exponent);
        aim_printf(&uc->pvs,
                   "  Commited/Peak Bucket Levels %d/%d\n",
                   m.committed_level,
                   m.peak_level);
        aim_printf(&uc->pvs, "  Timestamp %" PRId64 "\n", data_hi >> 36);

        bool is_bytes =
            meter_tbl->meter_granularity == PIPE_METER_GRANULARITY_BYTES;
        uint64_t clock_speed = pipe_mgr_get_sp_clock_speed(dev_id);
        uint64_t rate, bs;

        double per_cycle = m.cir_mantissa / pow(2, 31 - m.cir_exponent);
        rate = clock_speed * per_cycle;
        aim_printf(&uc->pvs,
                   "  CIR: %" PRIu64 " %s/Sec  %f k%s/Sec\n",
                   rate,
                   is_bytes ? "Bytes" : "Packets",
                   is_bytes ? rate * 8 / 1000.0 : (rate / 1000.0),
                   is_bytes ? "Bits" : "Packets");
        bs = (uint64_t)m.cbs_mantissa << m.cbs_exponent;
        aim_printf(&uc->pvs,
                   "  CBS: %" PRIu64 " %s  %f %s\n",
                   bs,
                   is_bytes ? "Bytes" : "Packets",
                   is_bytes ? bs * 8 / 1000.0 : (bs * 1.0),
                   is_bytes ? "kBits" : "Packets");

        per_cycle = m.pir_mantissa / pow(2, 31 - m.pir_exponent);
        rate = clock_speed * per_cycle;
        aim_printf(&uc->pvs,
                   "  PIR: %" PRIu64 " %s/Sec  %f k%s/Sec\n",
                   rate,
                   is_bytes ? "Bytes" : "Packets",
                   is_bytes ? rate * 8 / 1000.0 : rate / 1000.0,
                   is_bytes ? "Bits" : "Packets");
        bs = (uint64_t)m.pbs_mantissa << m.pbs_exponent;
        aim_printf(&uc->pvs,
                   "  PBS: %" PRIu64 " %s  %f %s\n",
                   bs,
                   is_bytes ? "Bytes" : "Packets",
                   is_bytes ? bs * 8 / 1000.0 : (bs * 1.0),
                   is_bytes ? "kBits" : "Packets");
      } else if (meter_tbl->type == PIPE_METER_TYPE_WRED) {
        pipe_status_t status = PIPE_SUCCESS;
        pipe_wred_spec_t wred_spec = {0};
        rmt_ram_line_t ram_line;
        PIPE_MGR_MEMSET(ram_line, 0, sizeof(rmt_ram_line_t));
        uint8_t *ram_line_ptr = (uint8_t *)ram_line;
        /* Data_lo and data_hi need to be marshalled into a byte array in
         * little-endian order to be supplied to the decode function
         */
        data_lo = htole64(data_lo);
        data_hi = htole64(data_hi);
        PIPE_MGR_MEMCPY(ram_line_ptr, &data_lo, 8);
        PIPE_MGR_MEMCPY(ram_line_ptr + 8, &data_hi, 8);
        status =
            pipe_mgr_meter_tof_decode_wred_spec(dev_id, &wred_spec, &ram_line);
        if (status != PIPE_SUCCESS) {
          aim_printf(&uc->pvs,
                     "Decoding WRED spec failed with err %s",
                     pipe_str_err(status));
        }
        aim_printf(&uc->pvs,
                   "Time constant %f\nMin threshold %d\nMax threshold %d\nMax "
                   "drop probability %f\n",
                   wred_spec.time_constant,
                   wred_spec.red_min_threshold,
                   wred_spec.red_max_threshold,
                   wred_spec.max_probability);
      }
    }
    if (direct_ref) break;
  }
  return UCLI_STATUS_OK;
}

static ucli_command_handler_f pipe_mgr_meter_tbl_ucli_ucli_handlers__[] = {
    PIPE_MGR_METER_TBL_CLI_CMD_HNDLR(show_vaddr),
    PIPE_MGR_METER_TBL_CLI_CMD_HNDLR(rd_by_index),
    NULL};

static ucli_module_t pipe_mgr_meter_tbl_ucli_module__ = {
    "meter_tbl_ucli",
    NULL,
    pipe_mgr_meter_tbl_ucli_ucli_handlers__,
    NULL,
    NULL,
};

ucli_node_t *pipe_mgr_meter_tbl_ucli_node_create(ucli_node_t *n) {
  ucli_node_t *m;
  ucli_module_init(&pipe_mgr_meter_tbl_ucli_module__);
  m = ucli_node_create("meter_mgr", n, &pipe_mgr_meter_tbl_ucli_module__);
  ucli_node_subnode_add(m, ucli_module_log_node_create("meter_mgr"));
  return m;
}

#else

void *pipe_mgr_meter_tbl_ucli_node_create(void) { return NULL; }

#endif
