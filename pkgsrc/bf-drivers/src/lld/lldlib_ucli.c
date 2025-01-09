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


#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>
#include <target-sys/bf_sal/bf_sys_intf.h>
#include <lld/lldlib_config.h>

#if LLDLIB_CONFIG_INCLUDE_UCLI == 1

#include <dvm/bf_drv_intf.h>
#include <target-utils/uCli/ucli.h>
#include <target-utils/uCli/ucli_argparse.h>
#include <target-utils/uCli/ucli_handler_macros.h>
#include "lld.h"
#include "lld_dev.h"
#include "lld_dev_tof2.h"
#include "lld_dev_tof3.h"
#include "lld_map.h"
#include <lld/lld_err.h>
#include <lld/lld_sku.h>
#include <lld/lld_dr_if.h>
#include <lld/lld_subdev_dr_if.h>
#include <lld/lld_reg_if.h>
#include <port_mgr/bf_port_if.h>
#include <lld/lld_dr_descriptors.h>
#include <lld/lld_inst_list_fmt.h>
#include <lld/lld_tof_addr_conversion.h>
#include <lld/lld_tof2_addr_conversion.h>
#include <lld/lld_tof3_addr_conversion.h>
#include "lld_diag.h"
#include "lld_diag_ext.h"
#include "lld_log.h"
#include "lld_memory_mapping.h"
#include "lld_dev_tof.h"
#include "lldlib_log.h"
#include "lld_csr2jtag.h"
#include <lld/bf_ts_if.h>
#include <target-utils/map/map.h>

#include <tofino_regs/tofino.h>
#include <tof2_regs/tof2_reg_drv.h>
#include <tof3_regs/tof3_reg_drv.h>
#include <lld/tof2_reg_drv_defs.h>
#include <lld/tof3_reg_drv_defs.h>
#include "lld_tof2_eos.h"

// FIXME
// requires port_mgr ucli file
uint32_t port_mgr_mac_get_glb_mode(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
uint32_t port_mgr_mac_get_slot2ch_map(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port);
int port_mgr_mac_read_counter(bf_dev_id_t dev_id,
                              bf_dev_port_t dev_port,
                              int ctr,
                              uint64_t *ctr_value);
extern bf_status_t lld_dump_all_ints(bf_dev_id_t dev_id,
                                     bf_subdev_id_t subdev_id);
extern bf_status_t lld_enable_all_ints(bf_dev_id_t dev_id,
                                       bf_subdev_id_t subdev_id);
extern bf_status_t lld_int_disable_all(bf_dev_id_t dev_id,
                                       bf_subdev_id_t subdev_id);
extern bf_status_t lld_inject_all_ints(bf_dev_id_t dev_id,
                                       bf_subdev_id_t subdev_id);
extern int dump_by_offset(int dev_id,
                          bf_subdev_id_t subdev_id,
                          uint32_t target_offset);
extern bf_status_t lld_inject_reg_with_offset(bf_dev_id_t input_dev,
                                              bf_dev_id_t subdev_id,
                                              uint32_t target_offset);
extern void reg_parse_main(void);
extern void reg_set_instream(FILE *fp);
extern void reg_set_outstream(FILE *fp);
extern void find_string_in_regs(char *pattern, bf_dev_id_t dev_id);
extern void consistency_check_pipes(void);
extern bf_status_t lld_port_add(bf_dev_id_t dev_id,
                                bf_dev_port_t dev_port,
                                bf_port_attributes_t *port_attrib,
                                bf_port_cb_direction_t direction);
extern bf_status_t lld_port_enable(bf_dev_id_t dev_id,
                                   bf_dev_port_t port,
                                   bool enable);
extern lld_err_t lld_port_remove(bf_dev_id_t dev_id,
                                 int port,
                                 bf_port_cb_direction_t direction);
extern int lld_diag_dma_alloc(bf_dev_id_t dev_id,
                              bf_subdev_id_t subdev_id,
                              uint32_t sz,
                              void **vaddr,
                              uint64_t *paddr);
extern ucli_node_t *lldlib_gpio_ucli_node_create(ucli_node_t *n);
extern bf_status_t port_mgr_tof2_microp_pcie_debug_fifo_dump(
    bf_dev_id_t device_id, uint8_t mode);
extern bf_status_t port_mgr_tof2_microp_pcie_debug_fifo_clear(
    bf_dev_id_t dev_id, uint8_t mode);

extern lld_diag_cfg_t lld_diag_cfg;
extern void lld_debug_bus_init(bf_dev_id_t dev_id);
extern FILE *reg_get_outstream();

static bool tof2_pcie_is_pipe_addr(uint32_t addr) {
  /* Bit 26 must be set for pipe addresses. */
  if (addr & (1u << 26)) {
    return true;
  }
  return false;
}

static uint32_t tof2_pcie_pipe_addr_get_pipe(uint32_t addr) {
  /* Bit 26 must be set for pipe addresses. */
  if (addr & (1u << 26)) {
    /* Bits 25:24 hold the pipe id */
    return (addr >> 24) & 0x3;
  } else {
    return ~0;
  }
}

static uint32_t tof_pcie_pipe_addr_get_stage(uint32_t addr) {
  /* Bit 25 must be set for pipe addresses. */
  if (addr & (1u << 25)) {
    /* Bits 22..19 hold the stage id */
    return (addr >> 19) & 0xF;
  } else {
    return ~0;
  }
}

static uint32_t tof2_pcie_pipe_addr_get_stage(uint32_t addr) {
  /* Bit 26 must be set for pipe addresses. */
  if (addr & (1u << 26)) {
    /* Bits 23:19 hold the stage id */
    return (addr >> 19) & 0x1F;
  } else {
    return ~0;
  }
}

static uint32_t tof3_pcie_pipe_addr_get_stage(uint32_t addr) {
  /* Bit 26 must be set for pipe addresses. */
  if (addr & (1u << 26)) {
    /* Bits 23..19 hold the stage id */
    return (addr >> 19) & 0x1F;
  } else {
    return ~0;
  }
}

bool tof2_pcie_is_pipe_stage_filtered_out(uint32_t addr) {
  uint32_t p, s;

  if (!tof2_pcie_is_pipe_addr(addr)) return false;

  p = tof2_pcie_pipe_addr_get_pipe(addr);
  s = tof2_pcie_pipe_addr_get_stage(addr);

  if ((lld_diag_cfg.pipe_mask & (1 << p)) &&
      (lld_diag_cfg.stage_mask & (1 << s))) {
    return false;
  }
  return true;
}

static bool tof2_u64_is_pipe_addr(uint64_t addr) {
  /* Bit 41 must be set for pipe addresses. */
  if (addr & (1ull << 41ull)) {
    return true;
  }
  return false;
}

static uint64_t tof2_u64_pipe_addr_get_pipe(uint64_t addr) {
  /* Bit 26 must be set for pipe addresses. */
  if (addr & (1ull << 41ull)) {
    /* Bits 40:39 hold the pipe id */
    return (addr >> 39ull) & 0x3ull;
  } else {
    return ~0ull;
  }
}

static uint64_t tof2_u64_pipe_addr_get_stage(uint64_t addr) {
  /* Bit 26 must be set for pipe addresses. */
  if (addr & (1ull << 41ull)) {
    /* Bits 23:19 hold the stage id */
    return (addr >> 34ull) & 0x1Full;
  } else {
    return ~0ull;
  }
}

bool tof2_u64_is_pipe_stage_filtered_out(uint64_t addr) {
  uint32_t p, s;

  if (!tof2_u64_is_pipe_addr(addr)) return false;

  p = (uint32_t)tof2_u64_pipe_addr_get_pipe(addr);
  s = (uint32_t)tof2_u64_pipe_addr_get_stage(addr);

  if ((lld_diag_cfg.pipe_mask & (1 << p)) &&
      (lld_diag_cfg.stage_mask & (1 << s))) {
    return false;
  }
  return true;
}

static ucli_status_t lld_ucli_ucli__dump__(ucli_context_t *uc) {
  UCLI_COMMAND_INFO(uc, "dump", 0, "Dump FIFO and DR summary.");

  lld_print_dr_stats();
  return 0;
}

static ucli_status_t lld_ucli_ucli__dr_dump__(ucli_context_t *uc) {
  UCLI_COMMAND_INFO(
      uc, "dr_dump", 3, "Dump DR contents. <dev_id> <subdev_id> <dr_id>");

  bf_dev_id_t asic;
  bf_subdev_id_t subdev;
  unsigned int dr_id;
  asic = atoi(uc->pargs->args[0]);
  subdev = atoi(uc->pargs->args[1]);
  dr_id = atoi(uc->pargs->args[2]);

  if (asic >= BF_MAX_DEV_COUNT || subdev >= BF_MAX_SUBDEV_COUNT) {
    aim_printf(&uc->pvs, "Error: Invalid dev_id/subdev_id\n");
    return 0;
  }
  if (dr_id >= BF_DMA_MAX_DR) {
    aim_printf(&uc->pvs, "Error: Invalid descriptor\n");
    return 0;
  }

  lld_print_dr_contents(asic, subdev, dr_id);
  return 0;
}

static ucli_status_t lld_ucli_ucli__reg_rd__(ucli_context_t *uc) {
  bf_dev_id_t asic;
  bf_subdev_id_t subdev;
  unsigned int reg;  //, reg_rd_data;
  int rc;

  UCLI_COMMAND_INFO(
      uc,
      "reg_rd",
      3,
      "Read a Tofino register via PCIe: reg_rd <asic> <subdev> <reg>");

  asic = atoi(uc->pargs->args[0]);
  subdev = atoi(uc->pargs->args[1]);
  reg = strtoul(uc->pargs->args[2], NULL, 16);

  if (asic >= BF_MAX_DEV_COUNT || subdev >= BF_MAX_SUBDEV_COUNT) {
    aim_printf(&uc->pvs,
               "Only %d:%d chips defined\n",
               BF_MAX_DEV_COUNT,
               BF_MAX_SUBDEV_COUNT);
    return 0;
  }

  rc = dump_by_offset(asic, subdev, reg);
  if (rc == -1) {
    uint32_t data = 0xdeadbeef;

    rc = lld_subdev_read_register(asic, subdev, reg, &data);
    if (rc != 0) {
      aim_printf(&uc->pvs, "Error: %d: from PCIe read\n", rc);
    } else {
      aim_printf(&uc->pvs, "%d:%d : %08x : %08x\n", asic, subdev, reg, data);
    }
  }

  return 0;
}

static ucli_status_t lld_ucli_ucli__reg_rdo__(ucli_context_t *uc) {
  bf_dev_id_t asic;
  bf_subdev_id_t subdev;
  unsigned int reg, data;

  UCLI_COMMAND_INFO(uc,
                    "reg_rdo",
                    3,
                    "Read (but do not decode) a Tofino register via PCIe: "
                    "reg_rd <asic> <subdev> <reg>");

  asic = atoi(uc->pargs->args[0]);
  subdev = atoi(uc->pargs->args[1]);
  reg = strtoul(uc->pargs->args[2], NULL, 16);

  if (asic >= BF_MAX_DEV_COUNT || subdev >= BF_MAX_SUBDEV_COUNT) {
    aim_printf(&uc->pvs,
               "Only %d:%d chips defined\n",
               BF_MAX_DEV_COUNT,
               BF_MAX_SUBDEV_COUNT);
    return 0;
  }
  lld_subdev_read_register(asic, subdev, reg, &data);
  aim_printf(&uc->pvs, "%d:%d : %08x : %08x\n", asic, subdev, reg, data);

  return 0;
}

static ucli_status_t lld_ucli_ucli__reg_rdm__(ucli_context_t *uc) {
  bf_dev_id_t asic;
  bf_subdev_id_t subdev;
  unsigned int reg, data, i, n, max = 1024;

  UCLI_COMMAND_INFO(uc,
                    "reg_rdm",
                    4,
                    "Read (but do not decode) multiple Tofino registers via "
                    "PCIe: reg_rd <asic> <subdev> <reg> <num_reg> "
                    "reg and num_reg are in hex");

  asic = atoi(uc->pargs->args[0]);
  subdev = atoi(uc->pargs->args[1]);
  reg = strtoul(uc->pargs->args[2], NULL, 16);
  n = strtoul(uc->pargs->args[3], NULL, 16);

  if (asic >= BF_MAX_DEV_COUNT || subdev >= BF_MAX_SUBDEV_COUNT) {
    aim_printf(&uc->pvs,
               "Only %d:%d chips defined\n",
               BF_MAX_DEV_COUNT,
               BF_MAX_SUBDEV_COUNT);
    return 0;
  }
  if (n > max) {
    n = max;
    aim_printf(&uc->pvs, "Warning: Limiting to %d registers (max)\n", max);
  }
  for (i = 0; i < n; i++) {
    lld_subdev_read_register(asic, subdev, reg + (4 * i), &data);
    aim_printf(
        &uc->pvs, "%d%d : %08x : %08x\n", asic, subdev, reg + (4 * i), data);
  }
  return 0;
}

static ucli_status_t lld_ucli_ucli__reg_wr__(ucli_context_t *uc) {
  bf_dev_id_t asic;
  bf_subdev_id_t subdev;
  unsigned int reg, reg_val;
  int rc;

  UCLI_COMMAND_INFO(
      uc,
      "reg_wr",
      4,
      "Write a Tofino register via PCIe: reg_wr <asic> <subdev> <reg> <value>");

  asic = atoi(uc->pargs->args[0]);
  subdev = atoi(uc->pargs->args[1]);
  reg = strtoul(uc->pargs->args[2], NULL, 16);
  reg_val = strtoul(uc->pargs->args[3], NULL, 16);

  if (asic >= BF_MAX_DEV_COUNT || subdev >= BF_MAX_SUBDEV_COUNT) {
    aim_printf(&uc->pvs,
               "Only %d:%d chips defined\n",
               BF_MAX_DEV_COUNT,
               BF_MAX_SUBDEV_COUNT);
    return 0;
  }

  lld_subdev_write_register(asic, subdev, (uint64_t)reg, reg_val);
  rc = dump_by_offset(asic, subdev, reg);
  if (rc == -1) {
    uint32_t data = 0xdeadbeef;

    rc = lld_subdev_read_register(asic, subdev, reg, &data);
    if (rc != 0) {
      aim_printf(&uc->pvs, "Error: %d: from PCIe read-back\n", rc);
    } else {
      aim_printf(&uc->pvs, "%d%d : %08x : %08x\n", asic, subdev, reg, data);
    }
  }

  return 0;
}

static ucli_status_t lld_ucli_ucli__reg_wro__(ucli_context_t *uc) {
  bf_dev_id_t asic;
  bf_subdev_id_t subdev;
  unsigned int reg, reg_val;

  UCLI_COMMAND_INFO(uc,
                    "reg_wro",
                    4,
                    "Write (only) a Tofino register via PCIe: reg_wr <asic> "
                    "<subdev> <reg> <value>");

  asic = atoi(uc->pargs->args[0]);
  subdev = atoi(uc->pargs->args[1]);
  reg = strtoul(uc->pargs->args[2], NULL, 16);
  reg_val = strtoul(uc->pargs->args[3], NULL, 16);

  if (asic >= BF_MAX_DEV_COUNT || subdev >= BF_MAX_SUBDEV_COUNT) {
    aim_printf(&uc->pvs,
               "Only %d:%d chips defined\n",
               BF_MAX_DEV_COUNT,
               BF_MAX_SUBDEV_COUNT);
    return 0;
  }

  lld_subdev_write_register(asic, subdev, (uint64_t)reg, reg_val);

  return 0;
}

static ucli_status_t lld_ucli_ucli__ind_rd__(ucli_context_t *uc) {
  bf_dev_id_t asic;
  bf_subdev_id_t subdev;
  uint64_t addr, data0 = 0ull, data1 = 0ull;

  UCLI_COMMAND_INFO(
      uc,
      "ind_rd",
      3,
      "Indirect Read a Tofino memory: ind_rd <asic> <subdev> <reg>");

  asic = atoi(uc->pargs->args[0]);
  subdev = atoi(uc->pargs->args[1]);
  addr = strtoull(uc->pargs->args[2], NULL, 16);

  if (asic >= BF_MAX_DEV_COUNT || subdev >= BF_MAX_SUBDEV_COUNT) {
    aim_printf(&uc->pvs,
               "Only %d:%d chips defined\n",
               BF_MAX_DEV_COUNT,
               BF_MAX_SUBDEV_COUNT);
    return 0;
  }

  lld_subdev_ind_read(asic, subdev, addr, &data0, &data1);
  aim_printf(&uc->pvs,
             "Rd: %d%d : %016" PRIx64 " : %016" PRIx64 " %016" PRIx64 "\n",
             asic,
             subdev,
             addr,
             data0,
             data1);
  return 0;
}

static ucli_status_t lld_ucli_ucli__ind_wr__(ucli_context_t *uc) {
  bf_dev_id_t asic;
  bf_subdev_id_t subdev;
  uint64_t addr, data0, data1;

  UCLI_COMMAND_INFO(
      uc,
      "ind_wr",
      5,
      "Write a Tofino memory: ind_wr <asic> <subdev> <reg> <data0> <data1>");

  asic = atoi(uc->pargs->args[0]);
  subdev = atoi(uc->pargs->args[1]);
  addr = strtoull(uc->pargs->args[2], NULL, 16);
  data0 = strtoull(uc->pargs->args[3], NULL, 16);
  data1 = strtoull(uc->pargs->args[4], NULL, 16);

  if (asic >= BF_MAX_DEV_COUNT || subdev >= BF_MAX_SUBDEV_COUNT) {
    aim_printf(&uc->pvs,
               "Only %d:%d chips defined\n",
               BF_MAX_DEV_COUNT,
               BF_MAX_SUBDEV_COUNT);
    return 0;
  }

  lld_subdev_ind_write(asic, subdev, (uint64_t)addr, data0, data1);
  aim_printf(&uc->pvs,
             "Wr: %d%d : %016" PRIx64 " : %016" PRIx64 " %016" PRIx64 "\n",
             asic,
             subdev,
             addr,
             data0,
             data1);
  return 0;
}

static void lld_mem_rd_helper(int width, void *p, int count) {
  uint8_t *p8 = p;
  uint16_t *p16 = p;
  uint32_t *p32 = p;
  uint64_t *p64 = p;

  // restrict to 128K count
  if (count >= (128 * 1024)) {
    count = (128 * 1024);
  }
  switch (width) {
    case 8:
      for (int i = 0; i < count; ++i) {
        if ((i % 16) == 0) {
          fprintf(reg_get_outstream(), "\n%p : ", (void *)(p8 + i));
        }
        fprintf(reg_get_outstream(), "%02x ", *(p8 + i));
      }
      fprintf(reg_get_outstream(), "\n");
      return;
    case 16:
      for (int i = 0; i < count; ++i) {
        if ((i % 8) == 0) {
          fprintf(reg_get_outstream(), "\n%p : ", (void *)(p16 + i));
        }
        fprintf(reg_get_outstream(), "%04x ", *(p16 + i));
      }
      fprintf(reg_get_outstream(), "\n");
      return;
    case 32:
      for (int i = 0; i < count; ++i) {
        if ((i % 4) == 0) {
          fprintf(reg_get_outstream(), "\n%p : ", (void *)(p32 + i));
        }
        fprintf(reg_get_outstream(), "%08x ", *(p32 + i));
      }
      fprintf(reg_get_outstream(), "\n");
      return;
    case 64:
      for (int i = 0; i < count; ++i) {
        if ((i % 4) == 0) {
          fprintf(reg_get_outstream(), "\n%p : ", (void *)(p64 + i));
        }
        fprintf(reg_get_outstream(), "%016" PRIx64 " ", *(p64 + i));
      }
      fprintf(reg_get_outstream(), "\n");
      return;
  }
}

static ucli_status_t lld_ucli_ucli__mem_rd8__(ucli_context_t *uc) {
  unsigned int n;
  uint64_t addr;
  uint8_t *p;

  UCLI_COMMAND_INFO(uc, "mem_rd8", 2, "Dump memory as u8's");

  addr = strtoull(uc->pargs->args[0], NULL, 16);
  n = atoi(uc->pargs->args[1]);
  p = (uint8_t *)lld_u64_to_void_ptr(addr);
  if (p != NULL) {
    lld_mem_rd_helper(8, p, n);
  }
  return 0;
}

static ucli_status_t lld_ucli_ucli__mem_rd32__(ucli_context_t *uc) {
  unsigned int n;
  uint64_t addr;
  uint32_t *p;

  UCLI_COMMAND_INFO(uc, "mem_rd32", 2, "Dump memory as u32's");

  addr = strtoull(uc->pargs->args[0], NULL, 16);
  n = atoi(uc->pargs->args[1]);
  p = (uint32_t *)lld_u64_to_void_ptr(addr);
  if (p != NULL) {
    lld_mem_rd_helper(32, p, n);
  }
  return 0;
}

static ucli_status_t lld_ucli_ucli__mem_rd64__(ucli_context_t *uc) {
  unsigned int n;
  uint64_t addr;
  uint64_t *p;

  UCLI_COMMAND_INFO(uc, "mem_rd64", 2, "Dump memory as u64's");

  addr = strtoull(uc->pargs->args[0], NULL, 16);
  n = atoi(uc->pargs->args[1]);
  p = (uint64_t *)lld_u64_to_void_ptr(addr);
  if (p != NULL) {
    lld_mem_rd_helper(64, p, n);
  }
  return 0;
}

static ucli_status_t lld_ucli_ucli__access__(ucli_context_t *uc) {
  UCLI_COMMAND_INFO(uc, "access", 0, "Enter chip access mode.");

  reg_parse_main();
  return 0;
}

static ucli_status_t lld_ucli_ucli__find__(ucli_context_t *uc) {
  bf_dev_id_t dev_id;
  UCLI_COMMAND_INFO(
      uc,
      "find",
      2,
      "Find block/registers matching a pattern <dev_id> <pattern>");

  dev_id = strtol(uc->pargs->args[0], NULL, 10);
  find_string_in_regs((char *)uc->pargs->args[1], dev_id);
  return 0;
}

bf_status_t lld_int_poll(bf_dev_id_t dev_id,
                         bf_subdev_id_t subdev_id,
                         bool all_ints);

static ucli_status_t lld_ucli_ucli__intx_svc__(ucli_context_t *uc) {
  bf_status_t bf_status;
  bf_dev_id_t dev_id;
  bf_subdev_id_t subdev_id;
  bool msk;

  UCLI_COMMAND_INFO(
      uc, "intx_svc", 3, "Poll for interrupts <dev_id> <subdev> <msk>");

  dev_id = atoi(uc->pargs->args[0]);
  subdev_id = atoi(uc->pargs->args[1]);
  msk = (uc->pargs->args[2][0] == '0') ? false : true;

  bf_status = bf_int_int_x_svc(dev_id, subdev_id, msk);
  if (bf_status != BF_SUCCESS) {
    aim_printf(&uc->pvs,
               "Error: %x : from bf_int_int_x_svc %d %d %d\n",
               bf_status,
               dev_id,
               subdev_id,
               msk ? 1 : 0);
  }
  return 0;
}

static ucli_status_t lld_ucli_ucli__msi_svc__(ucli_context_t *uc) {
  bf_status_t bf_status;
  bf_dev_id_t dev_id;
  bf_subdev_id_t subdev_id;
  bool msk;
  uint32_t msi_int;

  UCLI_COMMAND_INFO(
      uc,
      "msi_svc",
      4,
      "Service an MSI interrupt <dev_id> <subdev> <msi_int> <msk>");

  dev_id = atoi(uc->pargs->args[0]);
  subdev_id = atoi(uc->pargs->args[1]);
  msi_int = atoi(uc->pargs->args[2]);
  msk = (uc->pargs->args[3][0] == '0') ? false : true;

  bf_status = bf_int_msi_svc(dev_id, subdev_id, msi_int, msk);
  if (bf_status != BF_SUCCESS) {
    aim_printf(&uc->pvs,
               "Error: %x : from bf_int_msi_svc %d %d i%d %d\n",
               bf_status,
               dev_id,
               subdev_id,
               msi_int,
               msk ? 1 : 0);
  }
  return 0;
}

static ucli_status_t lld_ucli_ucli__msix_svc__(ucli_context_t *uc) {
  bf_status_t bf_status;
  bf_dev_id_t dev_id;
  bf_subdev_id_t subdev_id;
  bool msk;
  uint32_t msi_x_int;

  UCLI_COMMAND_INFO(
      uc,
      "msix_svc",
      4,
      "Service an MSI-x interrupt <dev_id> <subdev> <msi_x_int> <msk>");

  dev_id = atoi(uc->pargs->args[0]);
  subdev_id = atoi(uc->pargs->args[1]);
  msi_x_int = atoi(uc->pargs->args[2]);
  msk = (uc->pargs->args[3][0] == '0') ? false : true;

  bf_status = bf_int_msi_x_svc(dev_id, subdev_id, msi_x_int, msk);
  if (bf_status != BF_SUCCESS) {
    aim_printf(&uc->pvs,
               "Error: %x : from bf_int_msi_svc %d %d %d %d\n",
               bf_status,
               dev_id,
               subdev_id,
               msi_x_int,
               msk ? 1 : 0);
  }
  return 0;
}

static ucli_status_t lld_ucli_ucli__int_poll__(ucli_context_t *uc) {
  bf_dev_id_t dev_id;
  int all_ints = 0;

  UCLI_COMMAND_INFO(
      uc, "int_poll", 2, "Poll for interrupts <dev_id> <all interrupting>");

  dev_id = strtol(uc->pargs->args[0], NULL, 10);

  if ((uc->pargs->args[1][0] == 'a') || (uc->pargs->args[1][0] == 'A')) {
    all_ints = 1;
  }
  lld_int_poll(dev_id, 0, all_ints);
  return 0;
}

void lld_dump_int_list(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id);

static ucli_status_t lld_ucli_ucli__int_dump__(ucli_context_t *uc) {
  bf_dev_id_t dev_id;
  bf_subdev_id_t subdev_id;
  UCLI_COMMAND_INFO(uc, "int_dump", 2, "Dump interrupts <dev_id> <subdev_id>");
  dev_id = strtoul(uc->pargs->args[0], NULL, 10);
  subdev_id = strtoul(uc->pargs->args[1], NULL, 10);
  lld_dump_int_list(dev_id, subdev_id);
  return 0;
}

bf_status_t lld_clear_all_ints(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id);

static ucli_status_t lld_ucli_ucli__int_all__(ucli_context_t *uc) {
  bf_dev_id_t dev_id;
  bf_subdev_id_t subdev_id;
  UCLI_COMMAND_INFO(
      uc, "int_all", 2, "Dump all interrupts <dev_id> <subdev_id>");

  dev_id = strtoul(uc->pargs->args[0], NULL, 10);
  subdev_id = strtoul(uc->pargs->args[1], NULL, 10);
  lld_dump_all_ints(dev_id, subdev_id);
  return 0;
}

bf_status_t lld_dump_new_ints(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id);
bf_status_t lld_clear_new_ints(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id);

static ucli_status_t lld_ucli_ucli__int_new__(ucli_context_t *uc) {
  bf_dev_id_t dev_id;
  bf_subdev_id_t subdev_id;
  UCLI_COMMAND_INFO(
      uc, "int_new", 2, "Dump new interrupts <dev_id> <subdev_id>");

  dev_id = strtoul(uc->pargs->args[0], NULL, 10);
  subdev_id = strtoul(uc->pargs->args[1], NULL, 10);
  if (dev_id >= BF_MAX_DEV_COUNT || subdev_id >= BF_MAX_SUBDEV_COUNT) {
    aim_printf(&uc->pvs, "Error: Invalid dev_id/subdev_id\n");
    return 0;
  }

  lld_int_poll(dev_id, subdev_id, 0);
  lld_dump_new_ints(dev_id, subdev_id);
  return 0;
}

static ucli_status_t lld_ucli_ucli__clr_ints__(ucli_context_t *uc) {
  bf_dev_id_t dev_id;
  bf_subdev_id_t subdev_id;
  UCLI_COMMAND_INFO(
      uc, "clr_ints", 2, "Clear new interrupts <dev_id> <subdev_id>");

  dev_id = strtoul(uc->pargs->args[0], NULL, 10);
  subdev_id = strtoul(uc->pargs->args[1], NULL, 10);
  if (dev_id >= BF_MAX_DEV_COUNT || subdev_id >= BF_MAX_SUBDEV_COUNT) {
    aim_printf(&uc->pvs, "Error: Invalid dev_id/subdev_id\n");
    return 0;
  }

  lld_int_poll(dev_id, subdev_id, 0);
  lld_clear_new_ints(dev_id, subdev_id);
  return 0;
}

static ucli_status_t lld_ucli_ucli__int_clr__(ucli_context_t *uc) {
  return lld_ucli_ucli__clr_ints__(uc);
}

static ucli_status_t lld_ucli_ucli__new_ints__(ucli_context_t *uc) {
  lld_ucli_ucli__int_new__(uc);
  return 0;
}

static ucli_status_t lld_ucli_ucli__int_en_all__(ucli_context_t *uc) {
  UCLI_COMMAND_INFO(
      uc, "int_en_all", 2, "Enable all interrupts <dev_id> <subdev_id>");
  bf_dev_id_t dev_id;
  bf_subdev_id_t subdev_id;

  dev_id = strtoul(uc->pargs->args[0], NULL, 10);
  subdev_id = strtoul(uc->pargs->args[1], NULL, 10);
  lld_enable_all_ints(dev_id, subdev_id);
  return 0;
}

static ucli_status_t lld_ucli_ucli__int_dis_all__(ucli_context_t *uc) {
  UCLI_COMMAND_INFO(
      uc, "int_dis_all", 2, "Disable all interrupts <dev_id> <subdev_id>");
  bf_dev_id_t dev_id;
  bf_subdev_id_t subdev_id;

  dev_id = strtoul(uc->pargs->args[0], NULL, 10);
  subdev_id = strtoul(uc->pargs->args[1], NULL, 10);
  lld_int_disable_all(dev_id, subdev_id);
  return 0;
}

static ucli_status_t lld_ucli_ucli__int_inj_all__(ucli_context_t *uc) {
  UCLI_COMMAND_INFO(
      uc, "int_inj_all", 2, "Inject all interrupts <dev_id> <subdev_id>");
  bf_dev_id_t dev_id;
  bf_subdev_id_t subdev_id;

  dev_id = strtoul(uc->pargs->args[0], NULL, 10);
  subdev_id = strtoul(uc->pargs->args[1], NULL, 10);
  lld_inject_all_ints(dev_id, subdev_id);
  return 0;
}

static ucli_status_t lld_ucli_ucli__int_stress__(ucli_context_t *uc) {
  UCLI_COMMAND_INFO(uc,
                    "int_stress",
                    2,
                    "Inject all interrupts many times <dev_id> <subdev_id>");
  bf_dev_id_t dev_id;
  bf_subdev_id_t subdev_id;

  dev_id = strtoul(uc->pargs->args[0], NULL, 10);
  subdev_id = strtoul(uc->pargs->args[1], NULL, 10);
  if (dev_id >= BF_MAX_DEV_COUNT || subdev_id >= BF_MAX_SUBDEV_COUNT) {
    aim_printf(&uc->pvs, "Error: Invalid dev_id/subdev_id\n");
    return 0;
  }

  for (int j = 0; j < 100; ++j) {
    for (int i = 0; i < 25; ++i) {
      lld_inject_all_ints(dev_id, subdev_id);
    }
    lld_int_poll(dev_id, subdev_id, 0);
  }
  return 0;
}

void lld_test_int_cb(bf_dev_id_t dev_id,
                     bf_subdev_id_t subdev_id,
                     uint32_t offset);

static ucli_status_t lld_ucli_ucli__int_inj_reg__(ucli_context_t *uc) {
  UCLI_COMMAND_INFO(uc,
                    "int_inj_reg",
                    3,
                    "Inject interrupt for <dev_id> <subdev_id> <offset>");
  bf_dev_id_t dev_id = strtoul(uc->pargs->args[0], NULL, 10);
  bf_subdev_id_t subdev_id = strtoul(uc->pargs->args[1], NULL, 10);
  uint32_t offset = strtoul(uc->pargs->args[2], NULL, 16);
  lld_inject_reg_with_offset(dev_id, subdev_id, offset);
  return 0;
}

static ucli_status_t lld_ucli_ucli__int_cb__(ucli_context_t *uc) {
  bf_dev_id_t dev_id;
  bf_subdev_id_t subdev_id;
  uint32_t offset;

  UCLI_COMMAND_INFO(uc,
                    "int_cb",
                    3,
                    "Test interrupt callbacks <dev_id> <subdev_id> <offset>");

  dev_id = strtoul(uc->pargs->args[0], NULL, 10);
  subdev_id = strtoul(uc->pargs->args[1], NULL, 10);
  offset = strtoul(uc->pargs->args[2], NULL, 16);
  if (dev_id >= BF_MAX_DEV_COUNT || subdev_id >= BF_MAX_SUBDEV_COUNT) {
    aim_printf(&uc->pvs, "Error: Invalid dev_id/subdev_id\n");
    return 0;
  }

  lld_test_int_cb(dev_id, subdev_id, offset);
  return 0;
}

static ucli_status_t lld_ucli_ucli__int_test__(ucli_context_t *uc) {
  bf_dev_id_t dev_id;
  UCLI_COMMAND_INFO(uc, "int_test", 1, "Test interrupts <dev_id>");

  dev_id = strtol(uc->pargs->args[0], NULL, 10);
  lld_interrupt_test_extended(dev_id);

  return 0;
}

uint8_t ucli_evt_buf[1024 + 256] = {0};
uint8_t *ucli_evt_buf_aligned = &ucli_evt_buf[0];

void ucli_diag_event_cb(bf_dev_id_t dev_id, int data_sz, uint8_t *address) {
  printf("whoa. I seemed to get a diagnostic event...cool\n");
  printf("chip=%d : data_sz=%d : address=%p\n", dev_id, data_sz, address);
}

static ucli_status_t lld_ucli_ucli__diag__(ucli_context_t *uc) {
  bf_dev_id_t dev_id;
  UCLI_COMMAND_INFO(uc, "diag", 1, "Set up the diag event FM DR <dev_id>");

  dev_id = strtol(uc->pargs->args[0], NULL, 10);
  lld_debug_bus_init(dev_id);
  return 0;
}

extern void lld_dump_dma_log(ucli_context_t *uc, bf_dev_id_t dev_id);
extern char *lld_get_dma_log(bf_dev_id_t dev_id,
                             bf_subdev_id_t subdev_id,
                             uint32_t *drs_to_log,
                             int num_drs_to_log);
extern uint32_t dr_str_to_e(const char *dr_str);

// TBD add subdev_id support
static ucli_status_t lld_ucli_ucli__dma_log__(ucli_context_t *uc) {
  UCLI_COMMAND_INFO(uc, "dma_log", -1, "Dump the DMA log");

  int arg_start = 0;
  for (size_t i = 0;
       i < sizeof(uc->pargs[0].args__) / sizeof(uc->pargs[0].args__[0]);
       ++i) {
    if (!strcmp(uc->pargs[0].args__[i], "dma_log")) {
      arg_start = i;
      break;
    }
  }
  int argc = uc->pargs->count + 1;
  char **argv = (char **)(uc->pargs->args__ + arg_start);

  bf_dev_id_t dev_id = 0;
  bool got_plus = false;
  bool got_minus = false;
  uint32_t dr_include[BF_DMA_MAX_DR];
  uint32_t dr_exclude[BF_DMA_MAX_DR];
  int dr_idx = 0;
  for (int i = 0; i < BF_DMA_MAX_DR; ++i) {
    dr_include[i] = -1;
    dr_exclude[i] = i;
  }
  for (int i = 1; i < argc; ++i) {
    if (argv[i][0] >= '0' && argv[i][0] <= '9') { /* Assume dev-id filter. */
      dev_id = atoi(argv[i]);
    } else if (argv[i][0] == '+' && dr_idx < BF_DMA_MAX_DR) {
      got_plus = true;
      /* Adding a DR to the filter list. */
      if (got_minus) {
        aim_printf(&uc->pvs, "Cannot use '-' option as '+' was specified\n");
        return 0;
      }
      uint32_t dr_id = dr_str_to_e(argv[i] + 1);
      dr_include[dr_idx++] = dr_id;
    } else if (argv[i][0] == '-' && dr_idx < BF_DMA_MAX_DR) {
      got_minus = true;
      /* We want all DRs but this one. */
      if (got_plus) {
        aim_printf(&uc->pvs, "Cannot use '+' option as '-' was specified\n");
        return 0;
      }
      uint32_t dr_id = dr_str_to_e(argv[i] + 1);
      for (int j = dr_idx; j < BF_DMA_MAX_DR; ++j) {
        if (dr_exclude[j] == dr_id) {
          dr_exclude[j] = dr_exclude[dr_idx];
          dr_idx++;
          break;
        }
      }
    }
  }

  uint32_t *dr_filter = NULL;
  uint32_t filter_count = 0;
  if (got_plus) {
    dr_filter = dr_include;
    filter_count = dr_idx;
  } else if (got_minus) {
    dr_filter = dr_exclude + dr_idx;
    filter_count = BF_DMA_MAX_DR - dr_idx;
  }
  char *log_str = lld_get_dma_log(dev_id, 0, dr_filter, filter_count);
  if (!log_str) {
    aim_printf(&uc->pvs, "Unknown error getting DMA log\n");
    return 0;
  }
  aim_printf(&uc->pvs, "%s\n", log_str);
  bf_sys_free(log_str);
  return 0;
}

int wl_dma_count = 0;
int wl_entry_sz = 0;
int wl_entries = 0;
uint64_t wl_addr = 0ull;
bf_dma_addr_t wl_buf_p = 0;
void *wl_buf_v = NULL;
int wl_buf_initd = 0;

int wl_2_dma_count = 0;
int wl_2_entry_sz = 0;
int wl_2_entries = 0;
uint64_t wl_2_addr = 0ull;
bf_dma_addr_t wl_2_buf_p = 0;
void *wl_2_buf_v = NULL;
int wl_2_buf_initd = 0;

int wb_dma_count = 0;
int wb_entry_sz = 0;
int wb_entries = 0;
uint64_t wb_addr = 0ull;
bf_dma_addr_t wb_buf_p = 0;
void *wb_buf_v = NULL;
int wb_buf_initd = 0;

int rb_dma_count = 0;
int rb_entry_sz = 0;
int rb_entries = 0;
uint64_t rb_addr = 0ull;
bf_dma_addr_t rb_buf_p = 0;
void *rb_buf_v = NULL;
int rb_buf_initd = 0;
void ucli_completion_callback_fn(int dev_id,
                                 bf_subdev_id_t subdev_id,
                                 bf_dma_dr_id_t dr,
                                 uint64_t ts_sz,
                                 uint32_t attr,
                                 uint32_t status,
                                 uint32_t type,
                                 uint64_t msg_id,
                                 int s,
                                 int e) {
  int rc;
  (void)s;
  (void)e;

  switch (type) {
    case tx_m_type_wr_blk:
      printf("DMA c/b: dev=%d, subdev_id=%d, dr=%d, sz/tstamp=%" PRIx64
             ", attr=%xh, sts=%xh, type=%d, "
             "id=%" PRIx64 " <write-block: %d>\n",
             dev_id,
             subdev_id,
             dr,
             ts_sz,
             attr,
             status,
             type,
             msg_id,
             wb_dma_count);
      if (--wb_dma_count > 0) {
        rc = lld_subdev_push_wb(dev_id,
                                subdev_id,
                                wb_entry_sz /*bytes ea entry*/,
                                (((wb_addr >> 34ull) & 3ull) == 0) ? 4 : 1,
                                wb_entries,
                                false /* single entry */,
                                wb_buf_p,
                                wb_addr,
                                msg_id);
        if (rc != 0) {
          printf("Error: %d : from lld_push_pipe_write_block\n", rc);
        } else {
          lld_dr_start(dev_id, subdev_id, lld_dr_tx_pipe_write_block);
        }
      }
      break;
    case tx_m_type_mac_wr_blk:
      if (lld_dev_is_tofino(dev_id)) {
        printf("Error: Tofino does not have tx_m_type_mac_wr_blk\n");
        break;
      }
      printf("DMA c/b: dev=%d, subdev_id=%d, dr=%d, sz/tstamp=%" PRIx64
             ", attr=%xh, sts=%xh, type=%d, "
             "id=%" PRIx64 " <write-block: %d>\n",
             dev_id,
             subdev_id,
             dr,
             ts_sz,
             attr,
             status,
             type,
             msg_id,
             wb_dma_count);
      if (--wb_dma_count > 0) {
        rc = lld_subdev_push_mac_wb(dev_id,
                                    subdev_id,
                                    wb_entry_sz /*bytes ea entry*/,
                                    (((wb_addr >> 34ull) & 3ull) == 0) ? 4 : 1,
                                    wb_entries,
                                    false /* single entry */,
                                    wb_buf_p,
                                    wb_addr,
                                    msg_id);
        if (rc != 0) {
          printf("Error: %d : from lld_push_mac_write_block\n", rc);
        } else {
          lld_dr_start(dev_id, subdev_id, lld_dr_tx_mac_write_block);
        }
      }
      break;
    case tx_m_type_que_wr_list:
      if (lld_dev_is_tofino(dev_id) && dr == lld_dr_cmp_que_write_list_1) {
        printf("Error: Tofino does not have lld_dr_tx_que_write_list_1\n");
        break;
      }
      printf("DMA c/b: dev=%d, subdev_id=%d, dr=%d, sz/tstamp=%" PRIx64
             ", attr=%xh, sts=%xh, type=%d, "
             "id=%" PRIx64 " <write-list: %d>\n",
             dev_id,
             subdev_id,
             dr,
             ts_sz,
             attr,
             status,
             type,
             msg_id,
             wl_dma_count);
      if (--wl_dma_count > 0) {
        int i;
        uint32_t wac_data, test_data;
        int dr_0_1 = 0;
        if (dr == lld_dr_cmp_que_write_list_1) dr_0_1 = 1;
        rc = lld_subdev_push_wl(dev_id,
                                subdev_id,
                                dr_0_1,
                                wl_entry_sz,
                                wl_entries,
                                wl_buf_p,
                                msg_id);
        if (rc != 0) {
          printf("Error: %d : from lld_push_que_write_reg_list\n", rc);
        } else {
          if (dr_0_1 == 0)
            lld_dr_start(dev_id, subdev_id, lld_dr_tx_que_write_list);
          else if (dr_0_1 == 1)
            lld_dr_start(dev_id, subdev_id, lld_dr_tx_que_write_list_1);
        }
        lld_subdev_read_register(dev_id, subdev_id, 0x400004, &wac_data);
        for (i = 0; i < 256; i++) {
          lld_subdev_read_register(dev_id, subdev_id, 0x400004, &test_data);
          if (test_data != wac_data) {
            printf("Data Error: adr=%08x : got=%08x : exp=%08x\n",
                   0x400004,
                   test_data,
                   wac_data);
          }
        }
      }
      break;
    // case tx_m_type_que_rd_blk: //fix me add more
    case tx_m_type_rd_blk:
      printf("DMA c/b: dev=%d, subdev_id=%d, dr=%d, sz/tstamp=%" PRIx64
             ", attr=%xh, sts=%xh, type=%d, "
             "id=%" PRIx64 " <read-block: %d>\n",
             dev_id,
             subdev_id,
             dr,
             ts_sz,
             attr,
             status,
             type,
             msg_id,
             rb_dma_count);
      if (lld_dev_is_tofino(dev_id)) {
        int i, skipping = 0;
        uint32_t prev_line[4] = {0x98789798,
                                 0x45646545,
                                 0x56574557,
                                 0x89089090};  // some unlikely pattern
        // uint32_t *p = (uint32_t *)lld_u64_to_void_ptr(msg_id);
        uint32_t *p = (uint32_t *)rb_buf_v;
        printf("Raw Buffer:\n");
        for (i = 0; i < ((int)(ts_sz) * (rb_entry_sz / 4)) / 4; i++) {
          if ((i % 4) == 0) {
            // see if can skip the whole line
            if ((*(p + i + 0) == prev_line[0]) &&
                (*(p + i + 1) == prev_line[1]) &&
                (*(p + i + 2) == prev_line[2]) &&
                (*(p + i + 3) == prev_line[3])) {
              i += 3;
              if (!skipping) {
                skipping = 1;
                printf("\n%p : ...", (void *)(p + i));
              }
              continue;
            }
          }
          skipping = 0;
          if ((i % 4) == 0) {
            printf("\n%p : ", (void *)(p + i));
          }
          printf("%08x ", *(p + i));
          prev_line[i % 4] = *(p + i);
        }
      }
      {
        int i;
        // uint32_t *p = (uint32_t *)lld_u64_to_void_ptr(msg_id);
        uint32_t *p = (uint32_t *)rb_buf_v;
        printf("rb_entries=%d : rb_entry_sz=%d\n", rb_entries, rb_entry_sz);
        printf("\nFormatted:\n");
        for (i = 0; i < rb_entries; i++) {
          if (rb_entry_sz == 4) {
            uint32_t *w32 = (uint32_t *)p;
            printf("%4d : %08x\n", i, *(w32 + i));
          } else if (rb_entry_sz == 8) {
            uint64_t *w64 = (uint64_t *)p;
            printf("%4d : %016" PRIx64 "\n", i, *(w64 + i));
          } else if (rb_entry_sz == 16) {
            uint64_t *w64 = (uint64_t *)p;
            printf("%4d : %016" PRIx64 "_%016" PRIx64 "\n",
                   i,
                   *(w64 + (2 * i) + 1),
                   *(w64 + (2 * i)));
          }
        }
      }
      if (--rb_dma_count > 0) {
        if (dr == lld_dr_cmp_pipe_read_blk) {
          rc = lld_subdev_push_rb(dev_id,
                                  subdev_id,
                                  rb_entry_sz /*bytes ea entry*/,
                                  (((rb_addr >> 34ull) & 3ull) == 0) ? 4 : 1,
                                  rb_entries,
                                  rb_addr,
                                  rb_buf_p,
                                  msg_id);
          if (rc != 0) {
            printf("Error: %d : from lld_push_pipe_read_block\n", rc);
          } else {
            lld_dr_start(dev_id, subdev_id, lld_dr_tx_pipe_read_block);
          }
        } else if (dr == lld_dr_cmp_que_read_block_0) {
          rc =
              lld_subdev_push_que_rb(dev_id,
                                     subdev_id,
                                     0,
                                     rb_entry_sz /*bytes ea entry*/,
                                     (((rb_addr >> 34ull) & 3ull) == 0) ? 4 : 1,
                                     rb_entries,
                                     rb_addr,
                                     rb_buf_p,
                                     msg_id);
          if (rc != 0) {
            printf("Error: %d : from lld_push_que_read_block0\n", rc);
          } else {
            lld_dr_start(dev_id, subdev_id, lld_dr_tx_que_read_block_0);
          }
        } else if (dr == lld_dr_cmp_que_read_block_1) {
          rc =
              lld_subdev_push_que_rb(dev_id,
                                     subdev_id,
                                     1,
                                     rb_entry_sz /*bytes ea entry*/,
                                     (((rb_addr >> 34ull) & 3ull) == 0) ? 4 : 1,
                                     rb_entries,
                                     rb_addr,
                                     rb_buf_p,
                                     msg_id);
          if (rc != 0) {
            printf("Error: %d : from lld_push_que_read_block1\n", rc);
          } else {
            lld_dr_start(dev_id, subdev_id, lld_dr_tx_que_read_block_1);
          }
        }
      }
      break;
  }
}

static ucli_status_t lld_ucli_ucli__wb__(ucli_context_t *uc) {
  bf_dev_id_t dev_id;
  uint64_t addr, calc;
  int w, rc, entries, entry_sz, aligned_to, n_dma = 0, incrementing = 0;
  uint8_t *fbuf, fill_byte;
  bf_dma_addr_t dma_addr_p;
  void *dma_addr_v;
  int dma_buf_len = 0x4000 + 512;

  UCLI_COMMAND_INFO(uc,
                    "wb",
                    7,
                    "issue write-block DMA <dev_id> <addr> <# entries> "
                    "<entry_sz> <alignment> <n_dma> <fill-pattern>");

  dev_id = strtol(uc->pargs->args[0], NULL, 10);
  addr = strtoull(uc->pargs->args[1], NULL, 16);
  entries = strtoul(uc->pargs->args[2], NULL, 10);
  entry_sz = strtoul(uc->pargs->args[3], NULL, 10);
  aligned_to = strtoul(uc->pargs->args[4], NULL, 10);
  n_dma = strtoul(uc->pargs->args[5], NULL, 10);
  if (dev_id >= BF_MAX_DEV_COUNT) {
    aim_printf(&uc->pvs, "Error: Invalid dev_id\n");
    return 0;
  }

  if (uc->pargs->args[6][0] == 'i') {
    incrementing = 1;
  } else {
    fill_byte = strtoul(uc->pargs->args[6], NULL, 16);
  }
  wb_dma_count = n_dma ? n_dma : 1;  // make it at least one

  if ((entry_sz != 4) && (entry_sz != 8) && (entry_sz != 16)) {
    aim_printf(&uc->pvs, "Entry_sz must be one of: 4/8/16 \n");
    return 0;
  }
  if ((aligned_to != 64) && (aligned_to != 128) && (aligned_to != 256) &&
      (aligned_to != 512)) {
    aim_printf(&uc->pvs, "Aligned_to must be one of: 64/128/256/512 \n");
    return 0;
  }

  if (!wb_buf_initd) {
    void *vaddr;
    uint64_t paddr;
    rc = lld_diag_dma_alloc(0, 0, dma_buf_len, &vaddr, &paddr);
    if (rc != 0) {
      aim_printf(&uc->pvs, "Error allocating DMA buffer for WB\n");
      return -1;
    }
    wb_buf_p = paddr;
    wb_buf_v = vaddr;
    wb_buf_initd = 1;
  }
  // length check previously allocated DMA buffer
  if ((entry_sz * entries) > dma_buf_len) {
    aim_printf(&uc->pvs,
               "Error: %d is greater than DMA buffer length: %d\n",
               (entry_sz * entries),
               dma_buf_len);
    return -2;
  }

  calc = (uintptr_t)wb_buf_v;
  dma_addr_p = ((wb_buf_p + (aligned_to - 1)) & ~(aligned_to - 1));
  dma_addr_v =
      lld_u64_to_void_ptr(((calc + (aligned_to - 1)) & ~(aligned_to - 1)));
  fbuf = (uint8_t *)dma_addr_v;

  if (incrementing) {
    for (w = 0; w < (entry_sz * entries); w++) {
      fbuf[w] = w;
    }
  } else {
    for (w = 0; w < (entry_sz * entries); w++) {
      fbuf[w] = fill_byte;
    }
  }

  lld_register_completion_callback(
      dev_id, 0, lld_dr_cmp_pipe_write_blk, ucli_completion_callback_fn);

  wb_entry_sz = entry_sz;
  wb_entries = entries;
  wb_addr = addr;

  const uint32_t addr_inc = (((addr >> 30ull) & 3) == 0) ? 4 : 1;
  rc = lld_push_wb(dev_id,
                   entry_sz /*bytes ea entry*/,
                   addr_inc,
                   entries,
                   false /* single entry */,
                   dma_addr_p,
                   addr,
                   (uint64_t)lld_ptr_to_u64(fbuf));

  if (rc != 0) {
    aim_printf(&uc->pvs, "Error: %d : from lld_push_pipe_write_block\n", rc);
  }

  lld_dr_start(dev_id, 0, lld_dr_tx_pipe_write_block);
  return 0;
}

static ucli_status_t lld_ucli_ucli__wb_mcast__(ucli_context_t *uc) {
  bf_dev_id_t dev_id;
  uint64_t addr, calc;
  int w, rc, entries, entry_sz, aligned_to, n_dma = 0, incrementing = 0;
  uint8_t *fbuf, fill_byte;
  uint32_t mcast_vector;
  bf_dma_addr_t dma_addr_p;
  void *dma_addr_v;
  int dma_buf_len = 0x4000 + 512;

  UCLI_COMMAND_INFO(uc,
                    "wb_mcast",
                    8,
                    "issue write-block DMA <dev_id> <mcast-vector> <addr> "
                    "<# entries> <entry_sz> <alignment> <n_dma> "
                    "<fill-pattern>");

  dev_id = strtol(uc->pargs->args[0], NULL, 10);
  mcast_vector = strtoul(uc->pargs->args[1], NULL, 16);
  addr = strtoull(uc->pargs->args[2], NULL, 16);
  entries = strtoul(uc->pargs->args[3], NULL, 10);
  entry_sz = strtoul(uc->pargs->args[4], NULL, 10);
  aligned_to = strtoul(uc->pargs->args[5], NULL, 10);
  n_dma = strtoul(uc->pargs->args[6], NULL, 10);

  if (dev_id >= BF_MAX_DEV_COUNT) {
    aim_printf(&uc->pvs, "Error: Invalid dev_id\n");
    return 0;
  }

  if (uc->pargs->args[6][0] == 'i') {
    incrementing = 1;
  } else {
    fill_byte = strtoul(uc->pargs->args[7], NULL, 16);
  }
  wb_dma_count = n_dma ? n_dma : 1;  // make it at least one

  if ((entry_sz != 4) && (entry_sz != 8) && (entry_sz != 16)) {
    aim_printf(&uc->pvs, "Entry_sz must be one of: 4/8/16 \n");
    return 0;
  }
  if ((aligned_to != 64) && (aligned_to != 128) && (aligned_to != 256) &&
      (aligned_to != 512)) {
    aim_printf(&uc->pvs, "Aligned_to must be one of: 64/128/256/512 \n");
    return 0;
  }

  if (!wb_buf_initd) {
    void *vaddr;
    uint64_t paddr;
    rc = lld_diag_dma_alloc(0, 0, dma_buf_len, &vaddr, &paddr);
    if (rc != 0) {
      aim_printf(&uc->pvs, "Error allocating DMA buffer for WB\n");
      return -1;
    }
    wb_buf_p = paddr;
    wb_buf_v = vaddr;
    wb_buf_initd = 1;
  }
  // length check previously allocated DMA buffer
  if ((entry_sz * entries) > dma_buf_len) {
    aim_printf(&uc->pvs,
               "Error: %d is greater than DMA buffer length: %d\n",
               (entry_sz * entries),
               dma_buf_len);
    return -2;
  }

  calc = (uintptr_t)wb_buf_v;
  dma_addr_p = ((wb_buf_p + (aligned_to - 1)) & ~(aligned_to - 1));
  dma_addr_v =
      lld_u64_to_void_ptr(((calc + (aligned_to - 1)) & ~(aligned_to - 1)));
  fbuf = (uint8_t *)dma_addr_v;

  if (incrementing) {
    for (w = 0; w < (entry_sz * entries); w++) {
      fbuf[w] = w;
    }
  } else {
    for (w = 0; w < (entry_sz * entries); w++) {
      fbuf[w] = fill_byte;
    }
  }

  lld_register_completion_callback(
      dev_id, 0, lld_dr_cmp_pipe_write_blk, ucli_completion_callback_fn);

  wb_entry_sz = entry_sz;
  wb_entries = entries;
  wb_addr = addr;

  const uint32_t addr_inc = (((addr >> 30ull) & 3) == 0) ? 4 : 1;
  rc = lld_push_wb_mcast(dev_id,
                         entry_sz /*bytes ea entry*/,
                         addr_inc,
                         entries,
                         false /* single entry */,
                         dma_addr_p,
                         addr,
                         mcast_vector,
                         (uint64_t)lld_ptr_to_u64(fbuf));

  if (rc != 0) {
    aim_printf(&uc->pvs, "Error: %d : from lld_push_pipe_write_block\n", rc);
  }
  lld_dr_start(dev_id, 0, lld_dr_tx_pipe_write_block);
  return 0;
}

static ucli_status_t lld_ucli_ucli__wl__(ucli_context_t *uc) {
  uint64_t calc, reg_to_use, reg_data0, reg_data1;
  int w, rc, entries, entry_sz, aligned_to, n_dma = 0;
  uint8_t *fbuf;
  uint32_t *entry;
  bf_dma_addr_t dma_addr_p;
  void *dma_addr_v;
  int dma_buf_len = 0x4000 + 512;
  bool dr_0_1;
  bf_dev_id_t dev_id;
  UCLI_COMMAND_INFO(
      uc,
      "wl",
      7,
      "issue write-list<0/1> DMA <dev_id> <42b-register-addr> <# entries> "
      "<entry_sz> <alignment> <n_dma>");

  dr_0_1 = (strtoul(uc->pargs->args[0], NULL, 10) == 1) ? 1 : 0;
  dev_id = (bf_dev_id_t)(strtoul(uc->pargs->args[1], NULL, 16));
  reg_to_use = strtoull(uc->pargs->args[2], NULL, 16);
  entries = strtoul(uc->pargs->args[3], NULL, 10);
  entry_sz = strtoul(uc->pargs->args[4], NULL, 10);
  aligned_to = strtoul(uc->pargs->args[5], NULL, 10);
  n_dma = strtoul(uc->pargs->args[6], NULL, 10);
  wl_dma_count = n_dma ? n_dma : 1;  // make it at least one

  if (dev_id >= BF_MAX_DEV_COUNT) {
    aim_printf(&uc->pvs, "Error: Invalid dev_id\n");
    return 0;
  }

  if (lld_dev_is_tof2(dev_id) || lld_dev_is_tof3(dev_id)) dma_buf_len = 256;
  if ((entry_sz != 4) && (entry_sz != 8) && (entry_sz != 16)) {
    aim_printf(&uc->pvs, "Entry_sz must be one of: 4/8/16 \n");
    return 0;
  }
  if ((aligned_to != 64) && (aligned_to != 128) && (aligned_to != 256) &&
      (aligned_to != 512)) {
    aim_printf(&uc->pvs, "Aligned_to must be one of: 64/128/256/512 \n");
    return 0;
  }

  if (!wl_buf_initd) {
    void *vaddr;
    uint64_t paddr;
    rc = lld_diag_dma_alloc(dev_id, 0, dma_buf_len, &vaddr, &paddr);
    if (rc != 0) {
      aim_printf(&uc->pvs, "Error allocating DMA buffer for WL\n");
      return -1;
    }
    wl_buf_p = paddr;
    wl_buf_v = vaddr;
    wl_buf_initd = 1;
  }
  // length check previously allocated DMA buffer
  if ((entry_sz * entries) > dma_buf_len) {
    aim_printf(&uc->pvs,
               "Error: %d is greater than DMA buffer length: %d\n",
               (entry_sz * entries),
               dma_buf_len);
    return -2;
  }

  calc = (uintptr_t)wl_buf_v;
  dma_addr_p = ((wl_buf_p + (aligned_to - 1)) & ~(aligned_to - 1));
  dma_addr_v =
      lld_u64_to_void_ptr(((calc + (aligned_to - 1)) & ~(aligned_to - 1)));
  fbuf = (uint8_t *)dma_addr_v;

  entry = (uint32_t *)fbuf;
  for (w = 0; w < entries; w++) {
    uint64_t *p64 = (uint64_t *)entry;

    // reg_data0 = 0xffffffffffffffffull - (uint64_t)w;
    reg_data0 = 0x000000000000011full - (uint64_t)w;
    // reg_data1 = 0xffffffffffffffffull - (uint64_t)w;
    reg_data1 = 0x000000000000011full - (uint64_t)w;
    switch (entry_sz) {
      case 4: {
        *(p64 + 0) = reg_to_use;
        *(entry + 2) = (uint32_t)(reg_data1 & 0xffffffffull);
        entry += 3;
        break;
      }
      case 8: {
        *(p64 + 0) = reg_to_use;
        *(p64 + 1) = reg_data1;
        entry += 4;
        break;
      }
      case 16: {
        *(p64 + 0) = reg_to_use;
        *(p64 + 1) = reg_data1;
        *(p64 + 2) = reg_data0;
        entry += 6;
        break;
      }
      default:
        aim_printf(&uc->pvs, "Bad entry_sz=%d\n", entry_sz);
        return 0;
        break;
    }
  }
  if (dr_0_1 == 0) {
    lld_register_completion_callback(
        dev_id, 0, lld_dr_cmp_que_write_list, ucli_completion_callback_fn);
  } else if (dr_0_1 == 1) {
    lld_register_completion_callback(
        dev_id, 0, lld_dr_cmp_que_write_list_1, ucli_completion_callback_fn);
  }
  wl_entry_sz = entry_sz;
  wl_entries = entries;
  wl_addr = reg_to_use;

  rc = lld_push_wl(
      dev_id, dr_0_1, entry_sz, entries, dma_addr_p, (uint64_t)dma_addr_p);
  if (rc != 0) {
    aim_printf(&uc->pvs, "Error: %d : from lld_push_que_write_reg_list\n", rc);
  }
  if (dr_0_1 == 0) {
    lld_dr_start(dev_id, 0, lld_dr_tx_que_write_list);
  } else if (dr_0_1 == 1) {
    lld_dr_start(dev_id, 0, lld_dr_tx_que_write_list_1);
  }
  return 0;
}

static ucli_status_t lld_ucli_ucli__rb__(ucli_context_t *uc) {
  uint64_t addr;
  int rc, entries, entry_sz, aligned_to, n_dma = 0;
  bf_dma_addr_t dma_addr_p;
  int dma_buf_len = 0x4000 + 512;
  bf_dma_dr_id_t dr_id_cmp, dr_id_tx;
  bf_dev_id_t dev_id;

  UCLI_COMMAND_INFO(uc,
                    "rb",
                    7,
                    "issue read-block DMA <pipe/que0/que1> <dev_id> <addr> <# "
                    "entries> <entry_sz> <alignment> <n_dma>");

  if (strcmp(uc->pargs->args[0], "pipe") == 0) {
    dr_id_tx = lld_dr_tx_pipe_read_block;
    dr_id_cmp = lld_dr_cmp_pipe_read_blk;
  } else if (strcmp(uc->pargs->args[0], "que0") == 0) {
    dr_id_tx = lld_dr_tx_que_read_block_0;
    dr_id_cmp = lld_dr_cmp_que_read_block_0;
  } else if (strcmp(uc->pargs->args[0], "que1") == 0) {
    dr_id_tx = lld_dr_tx_que_read_block_1;
    dr_id_cmp = lld_dr_cmp_que_read_block_1;
  } else {
    aim_printf(
        &uc->pvs,
        "Please input the right name of read-block DMA <pipe/que0/que1>\n");
    return 0;
  }
  dev_id = (bf_dev_id_t)(strtoul(uc->pargs->args[1], NULL, 16));
  addr = strtoull(uc->pargs->args[2], NULL, 16);
  entries = strtoul(uc->pargs->args[3], NULL, 10);
  entry_sz = strtoul(uc->pargs->args[4], NULL, 10);
  aligned_to = strtoul(uc->pargs->args[5], NULL, 10);
  n_dma = strtoul(uc->pargs->args[6], NULL, 10);

  if (dev_id >= BF_MAX_DEV_COUNT) {
    aim_printf(&uc->pvs, "Error: Invalid dev_id\n");
    return 0;
  }

  rb_dma_count = n_dma ? n_dma : 1;  // make it at least one

  if ((entry_sz != 4) && (entry_sz != 8) && (entry_sz != 16)) {
    aim_printf(&uc->pvs, "Entry_sz must be one of: 4/8/16 \n");
    return 0;
  }
  if ((aligned_to != 64) && (aligned_to != 128) && (aligned_to != 256) &&
      (aligned_to != 512)) {
    aim_printf(&uc->pvs, "Aligned_to must be one of: 64/128/256/512 \n");
    return 0;
  }

  if (!rb_buf_initd) {
    void *vaddr;
    uint64_t paddr;
    rc = lld_diag_dma_alloc(dev_id, 0, dma_buf_len, &vaddr, &paddr);
    if (rc != 0) {
      aim_printf(&uc->pvs, "Error allocating DMA buffer for WB\n");
      return -1;
    }
    rb_buf_p = paddr;
    rb_buf_v = vaddr;
    rb_buf_initd = 1;
  }
  // length check previously allocated DMA buffer
  if ((entry_sz * entries) > dma_buf_len) {
    aim_printf(&uc->pvs,
               "Error: %d is greater than DMA buffer length: %d\n",
               (entry_sz * entries),
               dma_buf_len);
    return -2;
  }

  dma_addr_p = ((rb_buf_p + (aligned_to - 1)) & ~(aligned_to - 1));

  lld_register_completion_callback(
      dev_id, 0, dr_id_cmp, ucli_completion_callback_fn);

  rb_entry_sz = entry_sz;
  rb_entries = entries;
  rb_addr = addr;

  if (dr_id_tx == lld_dr_tx_pipe_read_block) {
    rc = lld_push_rb(dev_id,
                     entry_sz /*bytes ea entry*/,
                     (((addr >> 30ull) & 3ull) == 0) ? 4 : 1,
                     entries,
                     addr,
                     dma_addr_p,
                     (uint64_t)dma_addr_p);
  } else if (dr_id_tx == lld_dr_tx_que_read_block_0) {
    rc = lld_push_que_rb(dev_id,
                         0,
                         entry_sz /*bytes ea entry*/,
                         (((addr >> 30ull) & 3ull) == 0) ? 4 : 1,
                         entries,
                         addr,
                         dma_addr_p,
                         (uint64_t)dma_addr_p);
  } else if (dr_id_tx == lld_dr_tx_que_read_block_1) {
    rc = lld_push_que_rb(dev_id,
                         1,
                         entry_sz /*bytes ea entry*/,
                         (((addr >> 30ull) & 3ull) == 0) ? 4 : 1,
                         entries,
                         addr,
                         dma_addr_p,
                         (uint64_t)dma_addr_p);
  }
  if (rc != 0) {
    aim_printf(&uc->pvs, "Error: %d : from lld_push_pipe_read_block\n", rc);
  }
  lld_dr_start(dev_id, 0, dr_id_tx);
  return 0;
}

static ucli_status_t lld_ucli_ucli__dma_service__(ucli_context_t *uc) {
  bf_dev_id_t dev_id;
  bf_subdev_id_t subdev_id;
  int rc, dr, n_dma = 0;

  UCLI_COMMAND_INFO(
      uc,
      "dma_service",
      4,
      "Service a DMA descriptor ring (DR) <dev_id> <subdev_id> <dr> <cnt>");

  dev_id = strtoul(uc->pargs->args[0], NULL, 10);
  subdev_id = strtoul(uc->pargs->args[1], NULL, 10);
  dr = strtoul(uc->pargs->args[2], NULL, 10);
  n_dma = strtoul(uc->pargs->args[3], NULL, 10);
  if (dev_id >= BF_MAX_DEV_COUNT || subdev_id >= BF_MAX_SUBDEV_COUNT) {
    aim_printf(&uc->pvs, "Error: Invalid dev_id/subdev_id\n");
    return 0;
  }
  if (dr >= BF_DMA_MAX_DR) {
    aim_printf(&uc->pvs, "Error: Invalid descriptor\n");
    return 0;
  }

  rc = lld_dr_service(dev_id, subdev_id, dr, n_dma);
  aim_printf(&uc->pvs, "Descriptors serviced: %d\n", rc);

  return 0;
}

static ucli_status_t lld_ucli_ucli__dma_start__(ucli_context_t *uc) {
  bf_dev_id_t dev_id;
  bf_subdev_id_t subdev_id;
  int dr;

  UCLI_COMMAND_INFO(
      uc,
      "dma_start",
      3,
      "Start a DMA descriptor ring (DR) <dev_id> <subdev_id> <dr>");

  dev_id = strtoul(uc->pargs->args[0], NULL, 10);
  subdev_id = strtoul(uc->pargs->args[1], NULL, 10);
  dr = strtoul(uc->pargs->args[2], NULL, 10);
  if (dev_id >= BF_MAX_DEV_COUNT || subdev_id >= BF_MAX_SUBDEV_COUNT) {
    aim_printf(&uc->pvs, "Error: Invalid dev_id/subdev_id\n");
    return 0;
  }
  if (dr >= BF_DMA_MAX_DR) {
    aim_printf(&uc->pvs, "Error: Invalid descriptor\n");
    return 0;
  }

  lld_dr_start(dev_id, subdev_id, dr);

  return 0;
}

bf_dma_addr_t il_buf_p = 0;
void *il_buf_v = 0;
int il_buf_initd = 0;

bf_dma_addr_t il2_buf_p = 0;
void *il2_buf_v = 0;
int il2_buf_initd = 0;

void ucli_ilist_completion_callback_fn(bf_dev_id_t dev_id,
                                       bf_subdev_id_t subdev_id,
                                       bf_dma_dr_id_t dr,
                                       uint64_t ts_sz,
                                       uint32_t attr,
                                       uint32_t status,
                                       uint32_t type,
                                       uint64_t msg_id,
                                       int s,
                                       int e) {
  (void)s;
  (void)e;

  printf("ilist cmpl: dev=%d:%d : dr=%d : ts/sz=%" PRIx64
         " : attr=%x : sts=%d : typ=%d : "
         "id=%" PRIx64 "\n",
         dev_id,
         subdev_id,
         dr,
         ts_sz,
         attr,
         status,
         type,
         msg_id);
}

void construct_ilist(int pipe_id,
                     int stage_id,
                     int dr_0_3,
                     uint64_t addr,
                     int n,
                     int alignment) {
  pipe_instr_write_reg_t *instr;
  int i, rc;
  int dma_buf_len = 1024 + 256;
  uint8_t *ilist_buf_aligned;

  lld_register_completion_callback(0,
                                   0,
                                   lld_dr_cmp_pipe_inst_list_0 + dr_0_3,
                                   ucli_ilist_completion_callback_fn);

  if (n >= (1024 / 4)) {
    n = (1024 / 4) - 1;
  }

  if (!il_buf_initd) {
    void *vaddr;
    uint64_t paddr;
    rc = lld_diag_dma_alloc(0, 0, dma_buf_len, &vaddr, &paddr);
    if (rc != 0) {
      printf("Error allocating DMA buffer for WB\n");
      return;
    }
    il_buf_p = paddr;
    il_buf_v = vaddr;
    il_buf_initd = 1;
  }
  // length check previously allocated DMA buffer
  // if ((entry_sz * entries) > dma_buf_len) {
  //  aim_printf(&uc->pvs,"Error: %d is greater than DMA buffer length: %d\n",
  //         (entry_sz * entries), dma_buf_len);
  //  return;
  //}
  ilist_buf_aligned = (uint8_t *)lld_u64_to_void_ptr(
      (((uint64_t)lld_ptr_to_u64(il_buf_v) + ((uint64_t)alignment - 1ull)) &
       ~((uint64_t)alignment - 1ull)));

  instr = (pipe_instr_write_reg_t *)ilist_buf_aligned;

  TOF_CONSTRUCT_DEST_SELECT_INSTR(
      (dest_select_t *)(&instr[0]), pipe_id, stage_id);
  TOF_CONSTRUCT_DEST_SELECT_STAGE_INSTR((dest_select_stage_t *)(&instr[1]),
                                        stage_id);

  for (i = 2; i < (n + 2); i++) {
    TOF_CONSTRUCT_WRITE_REG_INSTR(&instr[i], addr + 4 * (i - 2), ~(i - 2));
  }
  rc = lld_push_ilist(0,
                      dr_0_3,
                      il_buf_p,
                      (n + 2) * sizeof(pipe_instr_write_reg_t),
                      0,
                      0,
                      0,
                      (uint64_t)i);
  if (rc != 0) {
    printf("Error: %d : from lld_push_pipe_inst_list\n", rc);
  } else {
    lld_dr_start(0, 0, lld_dr_tx_pipe_inst_list_0 + dr_0_3);
  }
}

static ucli_status_t lld_ucli_ucli__ilist__(ucli_context_t *uc) {
  uint64_t addr;
  int entries, aligned_to, pipe, stage, dr;

  UCLI_COMMAND_INFO(
      uc,
      "ilist",
      6,
      "issue ilist DMA <pipe> <stage> <0-3> <addr> <# entries> <alignment>");

  pipe = strtoul(uc->pargs->args[0], NULL, 10);
  stage = strtoul(uc->pargs->args[1], NULL, 10);
  dr = strtoul(uc->pargs->args[2], NULL, 10);
  addr = strtoull(uc->pargs->args[3], NULL, 16);
  entries = strtoul(uc->pargs->args[4], NULL, 10);
  aligned_to = strtoul(uc->pargs->args[5], NULL, 10);

  if ((dr < 0) || (dr > 3)) {
    aim_printf(&uc->pvs, "dr should be 0-3\n");
    return 0;
  }
  if ((aligned_to != 64) && (aligned_to != 128) && (aligned_to != 256) &&
      (aligned_to != 512)) {
    aim_printf(&uc->pvs, "Aligned_to must be one of: 64/128/256/512 \n");
    return 0;
  }

  // Just validating it for MIN and MAX value as we do not have any defined
  // value
  if ((addr == 0) || (addr == ULONG_MAX)) {
    aim_printf(&uc->pvs, "Invalid address \n");
    return 0;
  }

  construct_ilist(pipe, stage, dr, addr, entries, aligned_to);
  return 0;
}

void ucli_ilist2_completion_callback_fn(bf_dev_id_t dev_id,
                                        bf_subdev_id_t subdev_id,
                                        bf_dma_dr_id_t dr,
                                        uint64_t ts_sz,
                                        uint32_t attr,
                                        uint32_t status,
                                        uint32_t type,
                                        uint64_t msg_id,
                                        int s,
                                        int e) {
  int i;
  uint8_t *rsp_p = (uint8_t *)lld_u64_to_void_ptr(msg_id);
  (void)s;
  (void)e;

  printf("ilist2 cmpl: dev=%d:%d : dr=%d : sz=%" PRIx64
         " : attr=%x : sts=%d : typ=%d : "
         "id=%" PRIx64 "\n",
         dev_id,
         subdev_id,
         dr,
         ts_sz,
         attr,
         status,
         type,
         msg_id);
  if (lld_dev_is_tof2(dev_id) || lld_dev_is_tof3(dev_id)) return;
  printf("\nilist read response:\n");
  for (i = 0; i < (int)ts_sz; i++) {
    if ((i % 16) == 0) {
      printf("\n%016" PRIx64 " : ", msg_id + i);
    }
    printf("%02x ", *(rsp_p + i));
  }
  printf("\n");
}

void construct_ilist2(int pipe_id,
                      int stage_id,
                      int dr_0_3,
                      uint64_t addr,
                      int n,
                      int alignment) {
  pipe_instr_read_reg_t *instr;
  pipe_instr_write_reg_t *wr_instr;
  int i, rc;
  int dma_buf_len = 1024 + 256;
  uint8_t *ilist_buf_aligned;
  uint8_t *ilist2_buf_aligned;

  lld_register_completion_callback(0,
                                   0,
                                   lld_dr_cmp_pipe_inst_list_0 + dr_0_3,
                                   ucli_ilist2_completion_callback_fn);

  if (n >= (1024 / 4)) {
    n = (1024 / 4) - 1;
  }

  if (!il_buf_initd) {
    void *vaddr;
    uint64_t paddr;
    rc = lld_diag_dma_alloc(0, 0, dma_buf_len, &vaddr, &paddr);
    if (rc != 0) {
      printf("Error allocating DMA buffer for IL\n");
      return;
    }
    il_buf_p = paddr;
    il_buf_v = vaddr;
    il_buf_initd = 1;
  }
  if (!il2_buf_initd) {
    void *vaddr;
    uint64_t paddr;
    rc = lld_diag_dma_alloc(0, 0, dma_buf_len, &vaddr, &paddr);
    if (rc != 0) {
      printf("Error allocating DMA buffer for IL\n");
      return;
    }
    il2_buf_p = paddr;
    il2_buf_v = vaddr;
    il2_buf_initd = 1;
  }
  ilist_buf_aligned = (uint8_t *)lld_u64_to_void_ptr(
      (((uint64_t)lld_ptr_to_u64(il_buf_v) + ((uint64_t)alignment - 1ull)) &
       ~((uint64_t)alignment - 1ull)));
  ilist2_buf_aligned = (uint8_t *)lld_u64_to_void_ptr(
      (((uint64_t)lld_ptr_to_u64(il2_buf_v) + ((uint64_t)alignment - 1ull)) &
       ~((uint64_t)alignment - 1ull)));

  wr_instr = (pipe_instr_write_reg_t *)ilist_buf_aligned;

  TOF_CONSTRUCT_DEST_SELECT_INSTR(
      (dest_select_t *)(&wr_instr[0]), pipe_id, stage_id);
  TOF_CONSTRUCT_DEST_SELECT_STAGE_INSTR((dest_select_stage_t *)(&wr_instr[1]),
                                        stage_id);

  instr = (pipe_instr_read_reg_t *)&wr_instr[2];

  for (i = 0; i < n; i++) {
    TOF_CONSTRUCT_READ_REG_INSTR(&instr[i], addr + 4 * i);
  }

  // paint the response buffer
  memset(il2_buf_v, 0xee, dma_buf_len);

  rc = lld_push_ilist(0,
                      dr_0_3,
                      il_buf_p,
                      (2 * sizeof(pipe_instr_write_reg_t)) +
                          (n * sizeof(pipe_instr_read_reg_t)),
                      4,
                      0,
                      il2_buf_p,
                      (uint64_t)lld_ptr_to_u64(ilist2_buf_aligned));

  if (rc != 0) {
    printf("Error: %d : from lld_push_ilist\n", rc);
  } else {
    lld_dr_start(0, 0, lld_dr_tx_pipe_inst_list_0 + dr_0_3);
  }
}

static ucli_status_t lld_ucli_ucli__ilist2__(ucli_context_t *uc) {
  uint64_t addr;
  int entries, aligned_to, pipe, stage, dr;

  UCLI_COMMAND_INFO(uc,
                    "ilist2",
                    6,
                    "issue ilist (rad-capable) DMA <pipe> <stage> <0-3> <addr> "
                    "<# entries> <alignment>");

  pipe = strtoul(uc->pargs->args[0], NULL, 10);
  stage = strtoul(uc->pargs->args[1], NULL, 10);
  dr = strtoul(uc->pargs->args[2], NULL, 10);
  addr = strtoull(uc->pargs->args[3], NULL, 16);
  entries = strtoul(uc->pargs->args[4], NULL, 10);
  aligned_to = strtoul(uc->pargs->args[5], NULL, 10);

  if ((dr < 0) || (dr > 3)) {
    aim_printf(&uc->pvs, "dr should be 0-3\n");
    return 0;
  }
  if ((aligned_to != 64) && (aligned_to != 128) && (aligned_to != 256) &&
      (aligned_to != 512)) {
    aim_printf(&uc->pvs, "Aligned_to must be one of: 64/128/256/512 \n");
    return 0;
  }
  // Just validating it for MIN and MAX value
  if ((addr == 0) || (addr == ULONG_MAX)) {
    aim_printf(&uc->pvs, "Invalid address \n");
    return 0;
  }
  construct_ilist2(pipe, stage, dr, addr, entries, aligned_to);
  return 0;
}

typedef struct mem_test_sm_t {
  uint8_t *base_p;  // original, unaligned ptr from bf_sys_malloc
  uint8_t *wb_buf;  // above, properly aligned
  uint64_t tofino_addr;
  int entry_sz;
  int n_entry;
  uint64_t mask64_0;
  uint64_t mask64_1;
  char *name;
  char name_storage[255];
} mem_test_sm_t;

int ind_regs_lock_initd = 0;
bf_sys_mutex_t ind_regs_lock;

bf_status_t lld_ind_reg_lock_init() {
  if (!ind_regs_lock_initd) {
    if (bf_sys_mutex_init(&ind_regs_lock) != 0) {
      printf("Failed to init ind reg lock mutex \n");
      return BF_INVALID_ARG;
    }
    ind_regs_lock_initd = 1;
  }
  return BF_SUCCESS;
}

int new_hole_test = 1;

bf_status_t hole_test_cb(bf_dev_id_t dev_id,
                         bf_subdev_id_t subdev_id,
                         uint64_t offset,
                         uint32_t len,
                         char *key_wd) {
  // uint64_t mask64_0, mask64_1;
  uint64_t data64_0, data64_1;
  // uint64_t exp64_0,  exp64_1;
  // int      i, chip=0, rc, n_entry = len/16, consecutive_failures = 0;
  // int      passed = 1;
  int rc, n_entry = len / 16;  //, entry_sz = 16;
  static uint64_t next_hole;
  static char *last_key_wd;
  bool is_tofino = lld_dev_is_tofino(dev_id);
  bool is_tofino2 = lld_dev_is_tof2(dev_id);
  bool is_tofino3 = lld_dev_is_tof3(dev_id);
  pipe_physical_addr_t addr;
  addr.addr = offset;

  if (new_hole_test) {
    next_hole = 0ull;
    last_key_wd = "base";
    new_hole_test = 0;
  }
  //    aim_printf(&uc->pvs,"Mem test: %016"PRIx64" : %x entries: %s\n", offset,
  //    n_entry,
  //    key_wd );

  if ((is_tofino && 2 == ((offset >> 40) & 3)) ||
      (is_tofino2 && addr.tof2.pipe_always_1 == 1) ||
      (is_tofino3 && addr.tof3.pipe_always_1 == 1)) {
    int pipe = 0, stage = 0;

    if (lld_diag_cfg.do_tm == 1) return BF_SUCCESS;  // only do TM mems

    if (is_tofino) {
      pipe = addr.tof.pipe_id_39_37;
      stage = addr.tof.pipe_element_36_33;
    } else if (is_tofino2) {
      pipe = addr.tof2.pipe_id;
      stage = addr.tof2.pipe_stage;
    } else if (is_tofino3) {
      pipe = addr.tof3.pipe_id;
      stage = addr.tof3.pipe_stage;
    }
    if (!(lld_diag_cfg.pipe_mask & (1 << pipe))) return BF_SUCCESS;
    if ((is_tofino && stage < 12) || (is_tofino2 && stage < 20) ||
        (is_tofino3 && stage < 20)) {
      if (!(lld_diag_cfg.stage_mask & (1 << stage))) return BF_SUCCESS;
    }
  } else {                                           // not a pipe address
    if (lld_diag_cfg.do_tm == 0) return BF_SUCCESS;  // only do pipe mems
  }
  if (offset != next_hole) {
    printf("Hole: %016" PRIx64 " - %016" PRIx64 " : above: %s\n",
           next_hole,
           offset,
           last_key_wd);

    if (!lld_diag_cfg.quick) {
      // now read the first and last addresses in the hole..
      rc = lld_subdev_ind_read(
          dev_id, subdev_id, next_hole, &data64_0, &data64_1);
      if (rc != 0) {
        printf("Error: %d <%xh> : addr=%016" PRIx64 "\n", rc, rc, next_hole);
        return rc;
      }
      rc = lld_subdev_ind_read(
          dev_id, subdev_id, offset - 1, &data64_0, &data64_1);
      if (rc != 0) {
        printf("Error: %d <%xh> : addr=%016" PRIx64 "\n", rc, rc, offset - 1);
        return rc;
      }
    }
  }

  next_hole = offset + n_entry;
  last_key_wd = key_wd;
  return BF_SUCCESS;
}

static void traverse_mau_memories_for_holes(bf_dev_id_t dev_id,
                                            bf_subdev_id_t subdev_id,
                                            int quick) {
  uint64_t pipe_base = 0x20000000000ull;
  uint64_t pipe, stage, memtype, row, col;
  uint64_t pipe_limit, stage_limit;
  uint64_t data64_0, data64_1;  //, exp64_0, exp64_1;
  int rc;
  uint64_t last_hole = 0ull;
  bool is_tofino = false;
  bool is_tofino2 = false;
  bool is_tofino3 = false;

  (void)quick;
  if (lld_dev_is_tofino(dev_id)) {
    pipe_limit = 4;
    stage_limit = 12;
    is_tofino = true;
  } else if (lld_dev_is_tof2(dev_id)) {
    pipe_limit = 4;
    stage_limit = 20;
    is_tofino2 = true;
  } else if (lld_dev_is_tof3(dev_id)) {
    pipe_limit = 4;
    stage_limit = 20;
    is_tofino3 = true;
  } else {
    return;
  }

  for (pipe = 0; pipe < pipe_limit; pipe++) {
    // make sure pipe is configured for testing
    if (!(lld_diag_cfg.pipe_mask & (1 << pipe))) continue;

    for (stage = 0; stage < stage_limit; stage++) {
      // make sure stage is configured for testing
      if (!(lld_diag_cfg.stage_mask & (1 << stage))) continue;

      for (memtype = 0; memtype < 5; memtype++) {
        uint64_t min_row, max_row, min_col, max_col, min_ofs = 0, max_ofs = 0;

        switch (memtype) {
          case 0: /*unit rams*/
            min_row = 0;
            max_row = 7;
            min_col = 1;
            max_col = 11;
            max_ofs = 0x3ff;
            break;
          case 1: /*map  rams*/
            min_row = 0;
            max_row = 7;
            min_col = 6;
            max_col = 11;
            max_ofs = 0x3ff;
            break;
          case 2: /*stats deferred access rams*/
            min_row = 0;
            max_row = 3;
            min_col = 0;
            max_col = 0;
            max_ofs = 143;
            break;
          case 3: /*meters deferred access rams*/
            min_row = 0;
            max_row = 3;
            min_col = 0;
            max_col = 0;
            max_ofs = 143;
            break;
          case 4: /*tcams*/
            min_row = 0;
            max_row = 11;
            min_col = 0;
            max_col = 1;
            max_ofs = 0x1ff;
            break;
          default:
            bf_sys_assert(0);
            return;
        }

        for (row = min_row; row <= max_row; row++) {
          for (col = min_col; col <= max_col; col++) {
            uint64_t addr = 0;
            if (is_tofino) {
              addr = pipe_base | (pipe << 37ull) | (stage << 33ull) |
                     (2ull << 30ull) | (memtype << 18ull) | (row << 14ull) |
                     (col << 10ull) | 0;
            } else if (is_tofino2) {
              pipe_physical_addr_t a;
              a.tof2.pipe_always_1 = 1;
              a.tof2.pipe_id = pipe;
              a.tof2.pipe_stage = stage;
              a.tof2.pipe_ring_addr_type = addr_type_memdata;
              a.tof2.mem_type = memtype;
              a.tof2.mem_row = row;
              a.tof2.mem_col = col;
              a.tof2.mem_address = 0;
              addr = a.addr;
            } else if (is_tofino3) {
              pipe_physical_addr_t a;
              a.tof3.pipe_always_1 = 1;
              a.tof3.pipe_id = pipe;
              a.tof3.pipe_stage = stage;
              a.tof3.pipe_ring_addr_type = addr_type_memdata;
              a.tof3.mem_type = memtype;
              a.tof3.mem_row = row;
              a.tof3.mem_col = col;
              a.tof3.mem_address = 0;
              addr = a.addr;
            }

            if ((addr + min_ofs) > last_hole) {
              printf(
                  "Hole: %016" PRIx64 " - %016" PRIx64
                  " : Before: pipe=%d : stg=%d : memtyp=%d : row=%d : col=%d\n",
                  last_hole,
                  addr,
                  (int)pipe,
                  (int)stage,
                  (int)memtype,
                  (int)row,
                  (int)col);
              if (!quick) {
                // now read the first and last entry in the hole
                rc = lld_subdev_ind_read(
                    dev_id, subdev_id, last_hole, &data64_0, &data64_1);
                if (rc != 0) {
                  printf("Error: %d <%xh> : Rd : addr=%016" PRIx64 "\n",
                         rc,
                         rc,
                         addr);
                  return;
                }
                rc = lld_subdev_ind_read(dev_id,
                                         subdev_id,
                                         (addr + min_ofs - 1),
                                         &data64_0,
                                         &data64_1);
                if (rc != 0) {
                  printf("Error: %d <%xh> : Rd : addr=%016" PRIx64 "\n",
                         rc,
                         rc,
                         addr);
                  return;
                }
              }
            }
            last_hole = (addr + max_ofs + 1);

            printf(
                "    : %016" PRIx64 " - %016" PRIx64
                " :       : pipe=%d : stg=%d : memtyp=%d : row=%d : col=%d\n",
                addr + min_ofs,
                addr + max_ofs + 1,
                (int)pipe,
                (int)stage,
                (int)memtype,
                (int)row,
                (int)col);
          }
        }
      }
    }
  }
}

static ucli_status_t lld_ucli_ucli__mem_test__(ucli_context_t *uc) {
  bf_dev_id_t dev_id;
  bf_subdev_id_t subdev_id = 0;
  int quick = 1;

  UCLI_COMMAND_INFO(uc,
                    "mem_test",
                    5,
                    "mem_test <dev_id> <subdev_id> <mau prsr tm> <reg dma> "
                    "<quick extended>: Run tests on all memories");

  lld_ind_reg_lock_init();

  dev_id = strtol(uc->pargs->args[0], NULL, 10);
  if (dev_id >= BF_MAX_DEV_COUNT) {
    aim_printf(&uc->pvs, "Error: Invalid dev_id\n");
    return 0;
  }
  subdev_id = strtol(uc->pargs->args[1], NULL, 10);
  if (subdev_id >= BF_MAX_SUBDEV_COUNT) {
    aim_printf(&uc->pvs, "Error: Invalid subdev_id\n");
    return 0;
  }

  if (uc->pargs->args[4][0] == 'e') {
    quick = 0;
  }

  aim_printf(&uc->pvs, "Diagnostic configuration:\n");
  aim_printf(&uc->pvs, "   Pipe-mask: %1x\n", lld_diag_cfg.pipe_mask);
  aim_printf(&uc->pvs, "  Stage-mask: %05x\n", lld_diag_cfg.stage_mask);
  aim_printf(&uc->pvs, "       Quick: %s\n", quick ? "yes" : "no");

  if ((uc->pargs->args[2][0] == 'm') || (uc->pargs->args[2][0] == 'M')) {
    lld_diag_cfg.quick = quick;
    lld_diag_cfg.do_tm = 0;
    if ((uc->pargs->args[3][0] == 'd') || (uc->pargs->args[3][0] == 'D')) {
      lld_traverse_mau_memories_dma(dev_id);
    } else {
      lld_traverse_mau_memories(dev_id);
    }
  }
  if ((uc->pargs->args[2][0] == 'p') || (uc->pargs->args[2][0] == 'P')) {
    lld_diag_cfg.quick = quick;
    lld_diag_cfg.do_tm = 0;
    if ((uc->pargs->args[3][0] == 'd') || (uc->pargs->args[3][0] == 'D')) {
      lld_traverse_all_mems(dev_id, lld_mem_test_dma_cb, true, false);
    } else {
      lld_traverse_all_mems(dev_id, lld_mem_test_cb, true, false);
    }
  }
  if ((uc->pargs->args[2][0] == 't') || (uc->pargs->args[2][0] == 'T')) {
    lld_diag_cfg.quick = quick;
    lld_diag_cfg.do_tm = 1;
    if ((uc->pargs->args[3][0] == 'd') || (uc->pargs->args[3][0] == 'D')) {
      lld_traverse_all_mems(dev_id, lld_mem_test_dma_cb, true, false);
    } else {
      lld_traverse_all_mems(dev_id, lld_mem_test_cb, true, false);
    }
  }

  bf_diag_mem_results_t *r = lld_memtest_results_get(0);
  if (r) {
    if (r->overall_success && !r->ind_write_error && !r->ind_read_error &&
        !r->write_list_error && !r->write_block_error) {
      aim_printf(&uc->pvs, "Memory test passed\n");
    } else {
      /* If DMAs have not completed wait for a bit longer. */
      if (r->num_dma_cmplts_rcvd != r->num_dma_msgs_sent) {
        for (int i = 0; i < 10; ++i) {
          aim_printf(&uc->pvs,
                     "Waiting for DMAs to complete, %d send %d received\n",
                     r->num_dma_msgs_sent,
                     r->num_dma_cmplts_rcvd);
          bf_sys_sleep(1);
        }
      }
      /* If DMAs have still not completed report error. */
      if (r->num_dma_cmplts_rcvd != r->num_dma_msgs_sent) {
        aim_printf(&uc->pvs,
                   "ERROR: Memory test failed, DMA did not complete %d/%d\n",
                   r->num_dma_msgs_sent,
                   r->num_dma_cmplts_rcvd);
        return 0;
      }
      /* DMAs have completed, check the result again. */
      if (r->overall_success && !r->ind_write_error && !r->ind_read_error &&
          !r->write_list_error && !r->write_block_error) {
        aim_printf(&uc->pvs, "Memory test passed\n");
      } else {
        aim_printf(&uc->pvs, "Memory test failed\n");
      }
    }
  } else {
    aim_printf(&uc->pvs, "Unable to get memtest results\n");
  }
  return 0;
}

typedef struct chip_lvl_hole_t {
  uint64_t lo;
  uint64_t hi;
} chip_lvl_hole_t;

chip_lvl_hole_t chip_lvl_hole[] = {
    // address space holes
    {0x08000000000, 0x0ffffffffff},
    {0x28000000000, 0x2ffffffffff},
    {0x30000000000, 0x3ffffffffff},

    // address space dev holes
    {0x01c00000000, 0x01cffffffff},
    {0x03000000000, 0x03fffffffff},
    {0x05800000000, 0x058ffffffff},
    {0x06800000000, 0x07fffffffff},

    // address space mac holes
    {0x12700000000, 0x17fffffffff},  //(mac number holes)
    {0x10000020000, 0x100ffffffff},  //(mac0 register holes)
    // This will need to be expanded to all macs

    // address space sbus holes
    {0x18000400000, 0x1dfffffffff},  //(sbus0 register holes)
    {0x1c000400000, 0x1ffffffffff},  //(sbus1 register holes)

    // address space pipe holes
    {0x28000000000, 0x2ffffffffff},  //(pipe holes)
    {0x21800000000, 0x21bffffffff},  //(pipe0 stage holes)
    {0x23800000000, 0x23dffffffff},  //(pipe1 stage holes)
    {0x25800000000, 0x25dffffffff},  //(pipe2 stage holes)
    {0x27800000000, 0x27dffffffff},  //(pipe3 stage holes)

    {0x20100000000, 0x201ffffffff},  //(pipe0,stage0 holes)
    {0x20000080000, 0x2003fffffff},  //(pipe0,stage0 register holes)
    {0x20080200000, 0x200802fffff},  //(pipe0,stage0 physical memory holes)
    {0x200d0000000, 0x200dfffffff},  //(pipe0,stage0 virtual memory holes)
    // This will need to be expanded to all pipes and stages.
};

void traverse_chip_lvl_holes(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  uint32_t i;
  uint64_t data64_lo, data64_hi;
  uint64_t pipe, stage;

  for (i = 0; i < sizeof(chip_lvl_hole) / sizeof(chip_lvl_hole[0]); i++) {
    if ((chip_lvl_hole[i].lo & 0x2F000000000ull) ==
        0x20000000000ull) {  // expand to all pipes/stages
      uint64_t p, s, adr_hi, adr_lo;

      for (p = 0; p < 4; p++) {
        if (!(lld_diag_cfg.pipe_mask & (1 << p))) continue;
        for (s = 0; s < 16; s++) {
          if ((s == 12) || (s == 13)) continue;  // no such stage
          if (!(lld_diag_cfg.stage_mask & (1 << s))) continue;
          adr_lo = chip_lvl_hole[i].lo & ((~0x7full) << 33ull);
          adr_lo |= (p << 37ull);
          adr_lo |= (s << 33ull);
          adr_hi = chip_lvl_hole[i].hi & ((~0x7full) << 33ull);
          adr_hi |= (p << 37ull);
          adr_hi |= (s << 33ull);

          printf("Hole : %016" PRIx64 " - %016" PRIx64 "\n", adr_lo, adr_hi);
          if (lld_diag_cfg.quick) {
          } else {
            uint64_t mid_range = (adr_hi - adr_lo) / 2;
            lld_subdev_ind_read(
                dev_id, subdev_id, adr_lo, &data64_hi, &data64_lo);
            lld_subdev_ind_read(
                dev_id, subdev_id, adr_lo + mid_range, &data64_hi, &data64_lo);
            lld_subdev_ind_read(
                dev_id, subdev_id, adr_hi, &data64_hi, &data64_lo);
          }
        }
      }
      continue;
    }
    if ((chip_lvl_hole[i].lo >> 40ull) == 0x2ull) {  // pipe address
      pipe = (chip_lvl_hole[i].lo >> 37ull) & 0x3;
      stage = (chip_lvl_hole[i].lo >> 33ull) & 0xf;
      if (!(lld_diag_cfg.pipe_mask & (1 << pipe))) continue;
      // make sure stage is configured for testing
      if (!(lld_diag_cfg.stage_mask & (1 << stage))) continue;
    }
    printf("Hole : %016" PRIx64 " - %016" PRIx64 "\n",
           chip_lvl_hole[i].lo,
           chip_lvl_hole[i].hi);
    if (lld_diag_cfg.quick) {
    } else {
      uint64_t mid_range = (chip_lvl_hole[i].hi - chip_lvl_hole[i].lo) / 2;
      lld_subdev_ind_read(
          dev_id, subdev_id, chip_lvl_hole[i].lo, &data64_hi, &data64_lo);
      lld_subdev_ind_read(dev_id,
                          subdev_id,
                          chip_lvl_hole[i].lo + mid_range,
                          &data64_hi,
                          &data64_lo);
      lld_subdev_ind_read(
          dev_id, subdev_id, chip_lvl_hole[i].hi, &data64_hi, &data64_hi);
    }
  }
}

static ucli_status_t lld_ucli_ucli__holetest__(ucli_context_t *uc) {
  bf_dev_id_t dev_id;
  bf_subdev_id_t subdev_id;
  int quick = 1;

  UCLI_COMMAND_INFO(uc,
                    "holetest",
                    4,
                    "holetest <dev_id> <subdev_id> <mau prsr tm chiplvl> "
                    "<quick extended>: Run "
                    "tests on all memories");

  dev_id = strtoul(uc->pargs->args[0], NULL, 10);
  subdev_id = strtoul(uc->pargs->args[1], NULL, 10);
  if (!ind_regs_lock_initd) {
    if (bf_sys_mutex_init(&ind_regs_lock) != 0) {
      bf_sys_assert(0);
    }
    ind_regs_lock_initd = 1;
  }

  if (uc->pargs->args[3][0] == 'e') {
    quick = 0;
  }

  aim_printf(&uc->pvs, "Diagnostic configuration:\n");
  aim_printf(&uc->pvs, "   Pipe-mask: %1x\n", lld_diag_cfg.pipe_mask);
  aim_printf(&uc->pvs, "  Stage-mask: %03x\n", lld_diag_cfg.stage_mask);
  aim_printf(&uc->pvs, "       Quick: %s\n", quick ? "yes" : "no");

  new_hole_test = 1;  // reset some static var's

  if ((uc->pargs->args[1][0] == 'm') || (uc->pargs->args[1][0] == 'M')) {
    traverse_mau_memories_for_holes(dev_id, subdev_id, quick);
  }
  if ((uc->pargs->args[1][0] == 'p') || (uc->pargs->args[1][0] == 'P')) {
    lld_diag_cfg.quick = quick;
    lld_diag_cfg.do_tm = 0;
    lld_traverse_all_mems(dev_id, hole_test_cb, true, false);
  }
  if ((uc->pargs->args[1][0] == 't') || (uc->pargs->args[1][0] == 'T')) {
    lld_diag_cfg.quick = quick;
    lld_diag_cfg.do_tm = 1;
    lld_traverse_all_mems(dev_id, hole_test_cb, true, false);
  }
  if ((uc->pargs->args[1][0] == 'c') || (uc->pargs->args[1][0] == 'C')) {
    lld_diag_cfg.quick = quick;
    traverse_chip_lvl_holes(dev_id, subdev_id);
  }
  return 0;
}

bf_dma_addr_t pkt_buf_p;
void *pkt_buf_v;
int pkt_buf_initd = 0;

uint8_t canned_packet_data[] = {
    0x00, 0x11, 0x11, 0x11, 0x11, 0x11, 0x00, 0x55, 0x55, 0x55, 0x55,
    0x55, 0x08, 0x00, 0x45, 0x00, 0x00, 0x56, 0x00, 0x65, 0x00, 0x00,
    0x40, 0x06, 0xaf, 0x93, 0xc0, 0xa8, 0x00, 0x01, 0x0a, 0x00, 0x00,
    0x01, 0x04, 0xd2, 0x00, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x50, 0x02, 0x20, 0x00, 0xc3, 0xd5, 0x00, 0x00, 0x00,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09};

void ucli_tx_pkt_completion_callback_fn(int dev_id,
                                        bf_subdev_id_t subdev_id,
                                        bf_dma_dr_id_t dr,
                                        uint64_t ts_sz,
                                        uint32_t attr,
                                        uint32_t status,
                                        uint32_t type,
                                        uint64_t msg_id,
                                        int s,
                                        int e) {
  (void)s;
  (void)e;
  (void)subdev_id;
  switch (type) {
    case tx_m_type_pkt:
      // aim_printf(&uc->pvs,"Tx pkt cmpl: dev=%d : dr=%d : data_sz=%d : attr=%d
      // : sts=%d :
      // msg_id=%016"PRIx64"\n",
      //       dev_id, dr, data_sz, attr, status, msg_id );
      (void)dev_id;
      (void)dr;
      (void)ts_sz;
      (void)attr;
      (void)status;
      (void)msg_id;
      break;
    default:
      printf("Incorrect typ: %d\n", type);
      break;
  }
}

void lld_diag_push_tx_pkt(bf_dev_id_t dev_id, int len, int cos, int burst_sz) {
  int rc, i;
  uint8_t *packet_data;

  lld_register_completion_callback(0,
                                   0,
                                   lld_dr_cmp_tx_pkt_0 + (cos / 2),
                                   ucli_tx_pkt_completion_callback_fn);

  if (!pkt_buf_initd) {
    void *vaddr;
    uint64_t paddr;
    rc = lld_diag_dma_alloc(0, 0, 9216, &vaddr, &paddr);
    if (rc != 0) {
      printf("Error allocating DMA buffer for WB\n");
      return;
    }
    pkt_buf_p = paddr;
    pkt_buf_v = vaddr;
    pkt_buf_initd = 1;
  }
  packet_data = pkt_buf_v;
  memcpy(packet_data, canned_packet_data, sizeof(canned_packet_data));

  // "cos" is the first byte of the packet
  packet_data[0] = cos;

  for (i = 0; i < burst_sz; i++) {
    // for Tx, cos=0-3 which specifies the Tx DR to use
    // for Rx, cos=0-7 which specifies the Rx DR to use
    // so, accept cos=0-7 and insert that in pkt, but /2 to specify Tx DR
    rc = lld_push_tx_packet(
        dev_id, cos / 2, 1 /*s*/, 1 /*e*/, len, pkt_buf_p, i);
    if (rc != BF_SUCCESS) {
      printf("Error: %d : Pushing packet: %d\n", rc, i);
      if (i > 0) {
        lld_dr_start(0, 0, lld_dr_tx_pkt_0 + (cos / 2));
      }
      return;
    }
  }

  rc = lld_dr_start(0, 0, lld_dr_tx_pkt_0 + (cos / 2));
  if (rc != BF_SUCCESS) {
    printf("Error: %d : on DMA start\n", rc);
  }
  return;
}

static ucli_status_t lld_ucli_ucli__tx_pkt__(ucli_context_t *uc) {
  bf_dev_id_t dev_id;
  int burst_sz, cos, len;

  UCLI_COMMAND_INFO(uc, "tx_pkt", 4, "tx_pkt <dev_id> <len> <cos> <burst_sz>");

  dev_id = strtol(uc->pargs->args[0], NULL, 10);
  len = strtoul(uc->pargs->args[1], NULL, 10);
  cos = strtoul(uc->pargs->args[2], NULL, 10);
  burst_sz = strtoul(uc->pargs->args[3], NULL, 10);

  if (cos > 7) {
    aim_printf(&uc->pvs, "cos must be 0-7\n");
    return 0;
  }
  if ((len < 64) || (len > 9216)) {
    aim_printf(&uc->pvs, "pkt len must be 64-9216\n");
    return 0;
  }
  lld_diag_push_tx_pkt(dev_id, len, cos, burst_sz);
  return 0;
}

typedef void (*reg_traverse_cb)(bf_dev_id_t dev_id,
                                bf_subdev_id_t subdev_id,
                                char *reg_name,
                                uint64_t offset);
int get_reg_width(bf_dev_id_t dev_id, uint32_t offset);
void traverse_regs(bf_dev_id_t dev_id,
                   bf_subdev_id_t subdev_id,
                   reg_traverse_cb fn);

static int reg_test_result = 0;
void reg_test_traverse_cb(bf_dev_id_t dev_id,
                          bf_subdev_id_t subdev_id,
                          char *reg_name,
                          uint64_t offset) {
  int width = 0;

  // If this is not a scratch register ignore it.
  if (!strstr(reg_name, "scratch")) return;

  width = get_reg_width(dev_id, offset);
  if (width == 0) {
    printf("Cant determine width for reg at offset=%08x\n", (uint32_t)offset);
    return;
  }
  if (width <= 32) {
    uint32_t original, val;
    uint32_t test_data[] = {0xFFFFFFFE, 0, 0x7FFFFFFF};
    unsigned int i;

    /* Preserve original value. */
    lld_subdev_read_register(dev_id, subdev_id, offset, &original);

    for (i = 0; i < sizeof test_data / sizeof test_data[0]; ++i) {
      lld_subdev_write_register(dev_id, subdev_id, offset, test_data[i]);
      lld_subdev_read_register(dev_id, subdev_id, offset, &val);
      if (val != test_data[i]) {
        printf("Warning: expected 0x%x read back 0x%x %s\n",
               test_data[i],
               val,
               reg_name);
        reg_test_result = 1;
        break;
      }
    }
    // aim_printf(&uc->pvs,"%08x : %s ...\n", (uint32_t)offset, reg_name );
    lld_subdev_write_register(dev_id, subdev_id, offset, original);
  } else if (width <= 64) {
    // aim_printf(&uc->pvs,"Skip: width=%3d : %s\n", width, reg_name );
    return;
  } else {
    // aim_printf(&uc->pvs,"Skip: width=%3d : %s\n", width, reg_name );
    return;
  }
}

void int_reg_test_traverse_cb(bf_dev_id_t dev_id,
                              bf_subdev_id_t subdev_id,
                              char *reg_name,
                              uint64_t offset) {
  (void)dev_id;
  (void)subdev_id;
  printf("%08" PRIx64 " : %s\n", offset, reg_name);
}

#define MEM_NAME_SIZE 256
#define CUR_ARRAY_NAME_SIZE 1024

typedef struct tm_mem_info_t {
  uint32_t addr;
  int len;  // max_index + 1
  int width_in_bytes;
  uint32_t msk;
  uint32_t msk2;
  char mem_name[MEM_NAME_SIZE];
} tm_mem_info_t;

int next_mem = 0;
tm_mem_info_t g_tm_mem_info[512] = {{0}};

int cur_is_array = 0;
uint64_t cur_array_addr = 0ull;
int cur_array_width = 0;
int cur_array_len = 0;
int cur_array_name_len = 0;
char cur_array_name[CUR_ARRAY_NAME_SIZE] = {0};

static void find_tm_mems_traverse_cb(bf_dev_id_t dev_id,
                                     bf_subdev_id_t subdev_id,
                                     char *reg_name,
                                     uint64_t offset) {
  (void)subdev_id;  // TBD Tf3-fix
  int it = 0;

  if (*reg_name != 'd') return;

  // only care about tm_top
  if (strncmp(reg_name, "device_select__tm_top", 21) != 0) return;

  // check for this reg being an array element
  if (reg_name[strlen(reg_name) - 1] == ']') {
    if (!cur_is_array) {
      cur_is_array = 1;
      cur_array_len = 1;
      cur_array_addr = offset;
      cur_array_width = get_reg_width(dev_id, offset);
      // find beginning of last array subscript
      cur_array_name_len = strlen(reg_name) - 1;

      if (cur_array_name_len < 0) cur_array_name_len = 0;

      while (cur_array_name_len > 0 && reg_name[--cur_array_name_len] != '[')
        ;

      for (it = 0; it < cur_array_name_len && it < (CUR_ARRAY_NAME_SIZE - 1);
           it++) {
        cur_array_name[it] = reg_name[it];
      }
      cur_array_name[it] = '\0';
      cur_array_name_len = it;
      return;
    } else {  // either continuation of cur_array or a new one
      if (strncmp(cur_array_name, reg_name, cur_array_name_len) ==
          0) {  // continuation
        cur_array_len++;
      } else {  // new array. Dump prev if any
        g_tm_mem_info[next_mem].addr = cur_array_addr;
        g_tm_mem_info[next_mem].len = cur_array_len;
        g_tm_mem_info[next_mem].width_in_bytes = cur_array_width / 8;

        if (cur_array_name_len < 0) cur_array_name_len = 0;

        for (it = 0; it < cur_array_name_len && it < (MEM_NAME_SIZE - 1);
             it++) {
          g_tm_mem_info[next_mem].mem_name[it] = cur_array_name[it];
        }
        g_tm_mem_info[next_mem].mem_name[it] = '\0';
        next_mem++;

        printf("%016" PRIx64 " : %d : %s : 0 - %d\n",
               cur_array_addr,
               cur_array_width,
               cur_array_name,
               cur_array_len);
        // and start a new one
        cur_is_array = 1;
        cur_array_len = 1;
        cur_array_addr = offset;
        cur_array_width = get_reg_width(dev_id, offset);
        // find beginning of last array subscript
        cur_array_name_len = strlen(reg_name) - 1;

        if (cur_array_name_len < 0) cur_array_name_len = 0;

        while (cur_array_name_len > 0 && reg_name[--cur_array_name_len] != '[')
          ;

        for (it = 0; it < cur_array_name_len && it < (CUR_ARRAY_NAME_SIZE - 1);
             it++) {
          cur_array_name[it] = reg_name[it];
        }
        cur_array_name[it] = '\0';
        cur_array_name_len = it;
        return;
      }
    }
  } else {
    if (cur_is_array) {  // end of a prior array
      g_tm_mem_info[next_mem].addr = cur_array_addr;
      g_tm_mem_info[next_mem].len = cur_array_len;
      g_tm_mem_info[next_mem].width_in_bytes = cur_array_width / 8;

      if (cur_array_name_len < 0) cur_array_name_len = 0;

      for (it = 0; it < cur_array_name_len && it < (MEM_NAME_SIZE - 1); it++) {
        g_tm_mem_info[next_mem].mem_name[it] = cur_array_name[it];
      }
      g_tm_mem_info[next_mem].mem_name[it] = '\0';
      next_mem++;

      printf("%016" PRIx64 " : %d : %s : 0 - %d\n",
             cur_array_addr,
             cur_array_width,
             cur_array_name,
             cur_array_len);
      // clear out
      cur_is_array = 0;
      cur_array_len = 0;
      cur_array_addr = 0ull;
      cur_array_name_len = 0;
      memset(cur_array_name, 0, sizeof(cur_array_name));
    }
  }
}

int n_mems = 0;

static void test_tm_memories(bf_dev_id_t dev_id,
                             tm_mem_info_t *tm_mem_info,
                             int n_mems);

static ucli_status_t lld_ucli_ucli__find_tm_mems__(ucli_context_t *uc) {
  UCLI_COMMAND_INFO(uc,
                    "find_tm_mems",
                    1,
                    "Find tm memories in the register hierarchy <dev_id>");
  bf_dev_id_t dev_id = 0;
  cur_is_array = 0;
  cur_array_len = 0;
  cur_array_addr = 0ull;
  cur_array_name_len = 0;
  memset(cur_array_name, 0, sizeof(cur_array_name));

  memset(g_tm_mem_info, 0, sizeof(g_tm_mem_info));
  next_mem = 0;

  dev_id = strtol(uc->pargs->args[0], NULL, 10);

  traverse_regs(dev_id, 0, find_tm_mems_traverse_cb);

  return 0;
}

static ucli_status_t lld_ucli_ucli__test_tm_mems__(ucli_context_t *uc) {
  UCLI_COMMAND_INFO(uc,
                    "test_tm_mems",
                    1,
                    "Test tm memories in the register hierarchy <dev_id>");
  bf_dev_id_t dev_id = 0;
  cur_is_array = 0;
  cur_array_len = 0;
  cur_array_addr = 0ull;
  cur_array_name_len = 0;
  memset(cur_array_name, 0, sizeof(cur_array_name));

  memset(g_tm_mem_info, 0, sizeof(g_tm_mem_info));
  next_mem = 0;

  dev_id = strtol(uc->pargs->args[0], NULL, 10);
  if (dev_id >= BF_MAX_DEV_COUNT) {
    aim_printf(&uc->pvs, "Error: Invalid dev_id\n");
    return 0;
  }

  traverse_regs(dev_id, 0, find_tm_mems_traverse_cb);

  n_mems = next_mem;
  test_tm_memories(dev_id, g_tm_mem_info, n_mems);

  return 0;
}

static void zero_this_tm_memory(bf_dev_id_t dev_id,
                                tm_mem_info_t *tm_mem_info,
                                int mem_n) {
  int i;

  if (tm_mem_info[mem_n].width_in_bytes == 4) {
    for (i = 0; i < tm_mem_info[mem_n].len; i++) {
      lld_write_register(dev_id, tm_mem_info[mem_n].addr + (4 * i), 0);
    }
  } else {
    for (i = 0; i < tm_mem_info[mem_n].len; i++) {
      lld_write_register(dev_id, tm_mem_info[mem_n].addr + (8 * i), 0);
      lld_write_register(dev_id, tm_mem_info[mem_n].addr + (8 * i + 4), 0);
    }
  }
}

int verify_this_tm_memory_is_0(tm_mem_info_t *tm_mem_info, int mem_n) {
  int i, failed = 0;
  uint32_t d32;

  if (tm_mem_info[mem_n].width_in_bytes == 4) {
    for (i = 0; i < tm_mem_info[mem_n].len; i++) {
      lld_read_register(0, tm_mem_info[mem_n].addr + (4 * i), &d32);
      if (d32 != 0) {
        printf(" : failed\n");
        failed = 1;
        printf("Tm : Aliased : %08x : %s\n",
               tm_mem_info[mem_n].addr + (4 * i),
               tm_mem_info[mem_n].mem_name);
        printf("             : Exp=0 : Got=%08x\n", d32);
      }
    }
  } else {
    for (i = 0; i < tm_mem_info[mem_n].len; i++) {
      lld_read_register(0, tm_mem_info[mem_n].addr + (8 * i), &d32);
      if (d32 != 0) {
        printf(" : failed\n");
        failed = 1;
        printf("Tm : Aliased : %08x : %s\n",
               tm_mem_info[mem_n].addr + (8 * i),
               tm_mem_info[mem_n].mem_name);
        printf("             : Exp=0 : Got=%08x\n", d32);
      }
      lld_read_register(0, tm_mem_info[mem_n].addr + (8 * i + 4), &d32);
      if (d32 != 0) {
        printf(" : failed\n");
        failed = 1;
        printf("Tm : Aliased : %08x : %s\n",
               tm_mem_info[mem_n].addr + (8 * i + 4),
               tm_mem_info[mem_n].mem_name);
        printf("             : Exp=0 : Got=%08x\n", d32);
      }
    }
  }
  return failed;
}

static void verify_this_tm_memory(bf_dev_id_t dev_id,
                                  tm_mem_info_t *tm_mem_info,
                                  int mem_n) {
  int i, n_entries, failed = 0;
  // int m;

  printf("Tm : Verify : %08x : %s [0-%d]",
         tm_mem_info[mem_n].addr,
         tm_mem_info[mem_n].mem_name,
         tm_mem_info[mem_n].len - 1);

  // verify each entry written properly
  n_entries = tm_mem_info[mem_n].len;

  // foreach memory "m" != "n", verify contents are "0"
  for (i = 0; i < n_entries; i++) {
    uint32_t d32, d32_2;
    if (tm_mem_info[mem_n].width_in_bytes == 4) {
      lld_read_register(dev_id, tm_mem_info[mem_n].addr + (4 * i), &d32);
      if ((d32 & tm_mem_info[mem_n].msk) !=
          ((uint32_t)(0 - i) & tm_mem_info[mem_n].msk)) {
        printf(" : failed\n");
        failed = 1;
        printf("Tm : Wl : Data error : %08x : msk=%08x : %s\n",
               tm_mem_info[mem_n].addr + (4 * i),
               tm_mem_info[mem_n].msk,
               tm_mem_info[mem_n].mem_name);
        printf("                     : exp=%08x\n", (0 - i));
        printf("                     : got=%08x\n", d32);
      }
    } else if (tm_mem_info[mem_n].width_in_bytes == 8) {
      // note: order is important, for Rd must be lo then hi
      lld_read_register(dev_id, tm_mem_info[mem_n].addr + (8 * i), &d32);
      lld_read_register(dev_id, tm_mem_info[mem_n].addr + (8 * i + 4), &d32_2);
      if (((d32 & tm_mem_info[mem_n].msk) !=
           ((uint32_t)(0 - i) & tm_mem_info[mem_n].msk)) ||
          ((d32_2 & tm_mem_info[mem_n].msk2) !=
           ((uint32_t)(0 - i) & tm_mem_info[mem_n].msk2))) {
        printf(" : failed\n");
        failed = 1;
        printf("Tm : Wl : Data error : %08x : msk=%08x %08x : %s\n",
               tm_mem_info[mem_n].addr + (8 * i),
               tm_mem_info[mem_n].msk,
               tm_mem_info[mem_n].msk2,
               tm_mem_info[mem_n].mem_name);
        printf("                     : exp=%08x : %08x\n", (0 - i), (0 - i));
        printf("                     : got=%08x : %08x\n", d32, d32_2);
      }
    } else
      bf_sys_assert(0);
  }
#if 0
    for (m = mem_n + 1; m < n_mems; m++) {
        // determine mem width and zero out
        failed |= verify_this_tm_memory_is_0( tm_mem_info, m );
    }
#endif
  if (!failed) {
    printf(" : Passed\n");
  }
  // reset contents to "0"
  zero_this_tm_memory(dev_id, tm_mem_info, mem_n);
}

uint8_t tm_mem_test_unaligned_wl_buf[0x4000 + 512] = {0};

uint64_t tof3_map_32b_to_42b_dev_sel_addr(uint32_t a32) {
  return tof3_dir_to_indir_dev_sel(a32);
}
uint64_t tof2_map_32b_to_42b_dev_sel_addr(uint32_t a32) {
  return tof2_dir_to_indir_dev_sel(a32);
}
uint64_t tof_map_32b_to_42b_dev_sel_addr(uint32_t a32) {
  return tof_dir_to_indir_dev_sel(a32);
}

static int wl_this_tm_memory(bf_dev_id_t dev_id,
                             tm_mem_info_t *tm_mem_info,
                             int mem_n);

void tm_mem_test_completion_callback_fn(int dev_id,
                                        bf_subdev_id_t subdev_id,
                                        bf_dma_dr_id_t dr,
                                        uint64_t ts_sz,
                                        uint32_t attr,
                                        uint32_t status,
                                        uint32_t type,
                                        uint64_t msg_id,
                                        int s,
                                        int e) {
  int mem_n = (int)msg_id;

  (void)dev_id;
  (void)s;
  (void)e;
  (void)dr;
  (void)ts_sz;
  (void)attr;
  (void)status;
  (void)subdev_id;

  if (type != tx_m_type_que_wr_list) {
    printf("Hmm, not a write-blk or write-list completion? <typ=%d>\n", type);
    return;
  }

  // verify
  verify_this_tm_memory(dev_id, g_tm_mem_info, mem_n);

  if (mem_n < n_mems) {
    // launch next write-list
    wl_this_tm_memory(dev_id, g_tm_mem_info, mem_n + 1);
  } else {
    printf("TM memtest done.\n");
  }
}

static int wl_this_tm_memory(bf_dev_id_t dev_id,
                             tm_mem_info_t *tm_mem_info,
                             int mem_n) {
  // launch wl for this mem
  uint64_t calc, reg_to_use /*, reg_data0*/, reg_data1, w;
  int rc, entries, entry_sz, aligned_to;
  uint32_t *entry;
  entries = tm_mem_info[mem_n].len;
  entry_sz = tm_mem_info[mem_n].width_in_bytes;
  aligned_to = 256;
  int dma_buf_len = entries * entry_sz;
  bf_dma_addr_t dma_addr_p;
  void *dma_addr_v;
  if (lld_dev_is_tofino(dev_id))
    reg_to_use = tof_map_32b_to_42b_dev_sel_addr(tm_mem_info[mem_n].addr);
  else if (lld_dev_is_tof2(dev_id))
    reg_to_use = tof2_map_32b_to_42b_dev_sel_addr(tm_mem_info[mem_n].addr);
  else if (lld_dev_is_tof3(dev_id))
    reg_to_use = tof3_map_32b_to_42b_dev_sel_addr(tm_mem_info[mem_n].addr);
  else
    reg_to_use = 0;

  if ((entry_sz != 4) && (entry_sz != 8)) {
    printf("Entry_sz must be one of: 4/8/16 \n");
    return 0;
  }

  if (!wl_buf_initd) {
    void *vaddr;
    uint64_t paddr;
    rc = lld_diag_dma_alloc(dev_id, 0, dma_buf_len, &vaddr, &paddr);
    if (rc != 0) {
      printf("Error allocating DMA buffer for WB\n");
      return -1;
    }
    wl_buf_p = paddr;
    wl_buf_v = vaddr;
    wl_buf_initd = 1;
  }
  // length check previously allocated DMA buffer
  if ((entry_sz * entries) > dma_buf_len) {
    printf("Error: %d is greater than DMA buffer length: %d\n",
           (entry_sz * entries),
           dma_buf_len);
    return -2;
  }
  calc = (uintptr_t)wl_buf_v;
  dma_addr_p = ((wl_buf_p + (aligned_to - 1)) & ~(aligned_to - 1));
  dma_addr_v =
      lld_u64_to_void_ptr(((calc + (aligned_to - 1)) & ~(aligned_to - 1)));

  entry = (uint32_t *)dma_addr_v;
  for (w = 0; w < (uint64_t)entries; w++) {
    uint64_t *p64 = (uint64_t *)entry;
    uint32_t exp32 = 0x0 - w;

    reg_data1 = ((uint64_t)exp32 << 32ull) | ((uint64_t)exp32);

    /* Note: the 64b registers in TM are not directly accessible as 64b.
     *       they must be accessed 32b at a time and in a specific order,
     *       Wr: hi-addr, then lo-addr
     *       Rd: lo-addr, then hi-addr
     */
    switch (entry_sz) {
      case 4: {
        *(p64 + 0) = reg_to_use;
        *(entry + 2) = (uint32_t)(reg_data1 & 0xffffffffull);
        entry += 3;
        reg_to_use += 4;
        break;
      }
      case 8: {
        *(p64 + 0) = reg_to_use + 4ull;
        *(entry + 2) = (uint32_t)((reg_data1 >> 32ull) & 0xffffffffull);
        entry += 3;

        p64 = (uint64_t *)entry;
        *(p64 + 0) = reg_to_use + 0ull;
        *(entry + 2) = (uint32_t)(reg_data1 & 0xffffffffull);
        entry += 3;
        reg_to_use += 8;

        break;
      }
      default:
        printf("Bad entry_sz=%d\n", entry_sz);
        return 0;
        break;
    }
  }

  lld_register_completion_callback(
      dev_id, 0, lld_dr_cmp_que_write_list, tm_mem_test_completion_callback_fn);

  // 2 entries for each 64b register
  if (entry_sz == 8) {
    entries *= 2;
    entry_sz = 4;
  }
  rc = lld_push_wl(dev_id, 0, entry_sz, entries, dma_addr_p, (uint64_t)mem_n);
  if (rc != 0) {
    printf("Error: %d : from lld_push_que_write_reg_list\n", rc);
  }
  lld_dr_start(dev_id, 0, lld_dr_tx_que_write_list);
  return 0;
}

static void test_tm_memories(bf_dev_id_t dev_id,
                             tm_mem_info_t *tm_mem_info,
                             int num_mems) {
  int n;
  uint32_t data32;

  for (n = 0; n < num_mems; n++) {
    // determine mem width and zero out
    if (tm_mem_info[n].width_in_bytes > 4) {
      lld_write_register(dev_id, tm_mem_info[n].addr + 4, 0xffffffff);
      lld_write_register(dev_id, tm_mem_info[n].addr, 0xffffffff);
      lld_read_register(dev_id, tm_mem_info[n].addr, &data32);
      tm_mem_info[n].msk = data32;
      lld_read_register(dev_id, tm_mem_info[n].addr + 4, &data32);
      tm_mem_info[n].msk2 = data32;
    } else {
      lld_write_register(dev_id, tm_mem_info[n].addr, 0xffffffff);
      lld_read_register(dev_id, tm_mem_info[n].addr, &data32);
      if (data32 == 0) {
        printf("Warning: %s: mask is 0!\n", tm_mem_info[n].mem_name);
      }
      tm_mem_info[n].msk = data32;
    }
    // now 0 it out
    zero_this_tm_memory(dev_id, tm_mem_info, n);
  }
  // lanuch first wl
  wl_this_tm_memory(dev_id, tm_mem_info, 0);
}

static ucli_status_t lld_ucli_ucli__reg_test__(ucli_context_t *uc) {
  UCLI_COMMAND_INFO(uc, "reg_test", 2, "reg_test <dev_id> <subdev_id>");
  bf_subdev_id_t subdev_id;
  bf_dev_id_t dev_id = 0;
  reg_test_result = 0;
  dev_id = strtoul(uc->pargs->args[0], NULL, 10);
  subdev_id = strtoul(uc->pargs->args[1], NULL, 16);
  traverse_regs(dev_id, subdev_id, reg_test_traverse_cb);
  if (reg_test_result)
    aim_printf(&uc->pvs, "ERROR: reg_test failed\n");
  else
    aim_printf(&uc->pvs, "reg_test passed\n");
  return 0;
}

void traverse_int_regs(bf_dev_id_t dev_id, reg_traverse_cb fn);

static ucli_status_t lld_ucli_ucli__find_int_regs__(ucli_context_t *uc) {
  bf_dev_id_t dev_id = 0;
  UCLI_COMMAND_INFO(uc, "find_int_regs", 1, "find_int_regs <dev_id>");

  dev_id = strtol(uc->pargs->args[0], NULL, 10);

  traverse_int_regs(dev_id, int_reg_test_traverse_cb);
  return 0;
}

/*********************************************************************
 * TM indirect memories are read-only.
 */
void tm_ind_mem_test(
    uint32_t ind_addr, uint32_t ind_data, int sz, char *name, int quick) {
  uint32_t exp_data;
  int i, stride = quick ? (sz - 1) : 1;

  // determine valid bits
  // lld_write_register( 0, ind_addr, 0 );
  exp_data = 0;
  for (i = 0; i < sz; i += stride) {
    uint32_t rd_data;

    lld_write_register(0, ind_addr, i);
    lld_read_register(0, ind_data, &rd_data);
    if (rd_data != exp_data) {
      printf("Data Error: TM ind. mem: %s : offset=%08x, got=%x, exp=%08x\n",
             name,
             i,
             rd_data,
             exp_data);
      return;
    }
  }
}

/*
This is the address map from cdm memory
5 bit blk address: 0-27(msb)
3 bit bank address:0-4
11 bit row address: 0-2047
*/
void tm_pex_ind_mem_test(
    uint32_t ind_addr, uint32_t ind_data, int sz, char *name, int quick) {
  uint32_t exp_data;
  int i, stride = quick ? (sz - 1) : 1;

  exp_data = 0;
  if (quick) {
    for (i = 0; i < sz; i += stride) {
      uint32_t rd_data;

      lld_write_register(0, ind_addr, i);
      lld_read_register(0, ind_data, &rd_data);
      if (rd_data != exp_data) {
        printf("Data Error: TM ind. mem: %s : offset=%08x, got=%x, exp=%08x\n",
               name,
               i,
               rd_data,
               exp_data);
        return;
      }
    }
  } else {
    uint32_t blk, bank, row;

    for (blk = 0; blk <= 27; blk++) {
      for (bank = 0; bank <= 4; bank++) {
        for (row = 0; row <= 2047; row++) {
          uint32_t rd_data;
          uint32_t tgt_addr = (blk << 14) | (bank << 11) | (row);
          lld_write_register(0, ind_addr, tgt_addr);
          lld_read_register(0, ind_data, &rd_data);
          if (rd_data != exp_data) {
            printf(
                "Data Error: TM ind. mem: %s : offset=%08x, got=%x, exp=%08x\n",
                name,
                tgt_addr,
                rd_data,
                exp_data);
            return;
          }
        }
      }
    }
  }
}

void tm_prc_ind_mem_test(int quick) {
  uint32_t exp_data;
  uint32_t ind_addr, ind_target, ind_data;

  ind_addr = tofino_device_select_tm_top_tm_prc_top_prc_ind_addr_address;
  ind_data = tofino_device_select_tm_top_tm_prc_top_prc_ind_data_0_address;

  exp_data = 0;

  for (ind_target = 0; ind_target < 5; ind_target++) {
    uint32_t rd_data;
    switch (ind_target) {
      case 0:  // PRM, [8:0] row address,[16:9] bank address 0-139 --cmd rd
               // bothinterface, only take one back
      {
        int row, bank;
        uint32_t tgt_addr;
        int row_stride, bank_stride;

        row_stride = quick ? 511 : 1;
        bank_stride = quick ? 139 : 1;

        for (row = 0; row < 512; row += row_stride) {
          for (bank = 0; bank < 140; bank += bank_stride) {
            tgt_addr = (ind_target << 20) | (bank << 9) | row;
            lld_write_register(0, ind_addr, tgt_addr);
            lld_read_register(0, ind_data, &rd_data);
            if (rd_data != exp_data) {
              printf(
                  "Data Error: TM ind. prc.PRM_0: offset=%x : row=%x : "
                  "bank=%x "
                  ": got=%08x, exp=%08x\n",
                  tgt_addr,
                  row,
                  bank,
                  rd_data,
                  exp_data);
              return;
            }
            lld_read_register(0, ind_data + 4, &rd_data);
            if (rd_data != exp_data) {
              printf(
                  "Data Error: TM ind. prc.PRM_1: offset=%x : row=%x : "
                  "bank=%x "
                  ": got=%08x, exp=%08x\n",
                  tgt_addr,
                  row,
                  bank,
                  rd_data,
                  exp_data);
              return;
            }
            lld_read_register(0, ind_data + 8, &rd_data);
            if (rd_data != exp_data) {
              printf(
                  "Data Error: TM ind. prc.PRM_2: offset=%x : row=%x : "
                  "bank=%x "
                  ": got=%08x, exp=%08x\n",
                  tgt_addr,
                  row,
                  bank,
                  rd_data,
                  exp_data);
              return;
            }
          }
        }
        printf("Passed : TM PRM\n");
        break;
      }
      case 1:  // MAP, RO, [1:0] 32 bit [9:2]row address 0-139,[14:10] bank addr
               // 0-31 (really 0-27)
      {
        int row, bank;
        uint32_t tgt_addr;
        int row_stride, bank_stride;

        row_stride = quick ? 139 : 1;
        bank_stride = quick ? 27 : 1;

        for (row = 0; row < 140; row += row_stride) {
          for (bank = 0; bank < 28; bank += bank_stride) {
            tgt_addr = (ind_target << 20) | (row << 2) | (bank << 10);
            lld_write_register(0, ind_addr, tgt_addr);
            lld_read_register(0, ind_data, &rd_data);
            if (rd_data != exp_data) {
              printf(
                  "Data Error: TM ind. prc.MAP_0: offset=%x : row=%x : "
                  "bank=%x "
                  ": got=%08x, exp=%08x\n",
                  tgt_addr,
                  row,
                  bank,
                  rd_data,
                  exp_data);
              return;
            }
            lld_read_register(0, ind_data + 4, &rd_data);
            if (rd_data != exp_data) {
              printf(
                  "Data Error: TM ind. prc.MAP_1: offset=%x : row=%x : "
                  "bank=%x "
                  ": got=%08x, exp=%08x\n",
                  tgt_addr,
                  row,
                  bank,
                  rd_data,
                  exp_data);
              return;
            }
            lld_read_register(0, ind_data + 8, &rd_data);
            if (rd_data != exp_data) {
              printf(
                  "Data Error: TM ind. prc.MAP_2: offset=%x : row=%x : "
                  "bank=%x "
                  ": got=%08x, exp=%08x\n",
                  tgt_addr,
                  row,
                  bank,
                  rd_data,
                  exp_data);
              return;
            }
          }
        }
        printf("Passed : TM MAP\n");
        break;
      }
      case 2:  // t2,,RO, 0-511
      {
        int offset;
        uint32_t tgt_addr;
        int offset_stride;

        offset_stride = quick ? 511 : 1;

        for (offset = 0; offset < 512; offset += offset_stride) {
          tgt_addr = (ind_target << 20) | offset;
          lld_write_register(0, ind_addr, tgt_addr);
          lld_read_register(0, ind_data, &rd_data);
          /* Note:
           * This memory is only writable from the datapath (packets). There
           * is no initialization value and no way to preset it from the
           * PCIe side (RO). So, can only check for 0x0bad0bad
           */
          if (rd_data == 0x0bad0bad) {
            printf(
                "Data Error: TM ind. prc.t2_0: offset=%x : got=%08x, "
                "exp=%08x\n",
                tgt_addr,
                rd_data,
                exp_data);
            return;
          }
          lld_read_register(0, ind_data + 4, &rd_data);
          if (rd_data == 0x0bad0bad) {
            printf(
                "Data Error: TM ind. prc.t2_1: offset=%08x : got=%x, "
                "exp=%08x\n",
                tgt_addr,
                rd_data,
                exp_data);
            return;
          }
          lld_read_register(0, ind_data + 8, &rd_data);
          if (rd_data == 0x0bad0bad) {
            printf(
                "Data Error: TM ind. prc.t2_2: offset=%x : got=%08x, "
                "exp=%08x\n",
                tgt_addr,
                rd_data,
                exp_data);
            return;
          }
        }
        printf("Passed : TM t2\n");
        break;
      }
      case 3:  // t3_0, t_pointer,RO, [10:2] row address, [1:0] seg
      {
        int row, seg;
        uint32_t tgt_addr;
        int row_stride, seg_stride;

        row_stride = quick ? 511 : 1;
        seg_stride = quick ? 3 : 1;

        for (row = 0; row < 512; row += row_stride) {
          for (seg = 0; seg < 4; seg += seg_stride) {
            tgt_addr = (ind_target << 20) | (row << 2) | seg;
            lld_write_register(0, ind_addr, tgt_addr);
            lld_read_register(0, ind_data, &rd_data);
            if (rd_data != exp_data) {
              printf(
                  "Data Error: TM ind. prc.t3_0_0: offset=%x : row=%x : seg=%x "
                  ": got=%08x, exp=%08x\n",
                  tgt_addr,
                  row,
                  seg,
                  rd_data,
                  exp_data);
              return;
            }
            lld_read_register(0, ind_data + 4, &rd_data);
            if (rd_data != exp_data) {
              printf(
                  "Data Error: TM ind. prc.t3_0_1: offset=%x : row=%x : seg=%x "
                  ": got=%08x, exp=%08x\n",
                  tgt_addr,
                  row,
                  seg,
                  rd_data,
                  exp_data);
              return;
            }
            lld_read_register(0, ind_data + 8, &rd_data);
            if (rd_data != exp_data) {
              printf(
                  "Data Error: TM ind. prc.t3_0_2: offset=%x : row=%x : seg=%x "
                  ": got=%08x, exp=%08x\n",
                  tgt_addr,
                  row,
                  seg,
                  rd_data,
                  exp_data);
              return;
            }
          }
        }
        printf("Passed : TM t3_0\n");
        break;
      }
      case 4:  //[10:2] row address, [1:0] seg
      {
        int row, seg;
        uint32_t tgt_addr;
        int row_stride, seg_stride;

        row_stride = quick ? 511 : 1;
        seg_stride = quick ? 3 : 1;

        for (row = 0; row < 512; row += row_stride) {
          for (seg = 0; seg < 4; seg += seg_stride) {
            tgt_addr = (ind_target << 20) | (row << 2) | seg;
            lld_write_register(0, ind_addr, tgt_addr);
            lld_read_register(0, ind_data, &rd_data);
            if (rd_data != exp_data) {
              printf(
                  "Data Error: TM ind. prc.t3_1_0: offset=%x : row=%x : seg=%x "
                  ": got=%08x, exp=%08x\n",
                  tgt_addr,
                  row,
                  seg,
                  rd_data,
                  exp_data);
              return;
            }
            lld_read_register(0, ind_data + 4, &rd_data);
            if (rd_data != exp_data) {
              printf(
                  "Data Error: TM ind. prc.t3_1_1: offset=%x : row=%x : seg=%x "
                  ": got=%08x, exp=%08x\n",
                  tgt_addr,
                  row,
                  seg,
                  rd_data,
                  exp_data);
              return;
            }
            lld_read_register(0, ind_data + 8, &rd_data);
            if (rd_data != exp_data) {
              printf(
                  "Data Error: TM ind. prc.t3_1_2: offset=%x : row=%x : seg=%x "
                  ": got=%08x, exp=%08x\n",
                  tgt_addr,
                  row,
                  seg,
                  rd_data,
                  exp_data);
              return;
            }
          }
        }
        printf("Passed : TM t3_1\n");
        break;
      }
      default:
        break;
    }
  }
}

static ucli_status_t lld_ucli_ucli__tm_ind_test__(ucli_context_t *uc) {
  int quick = 1;  // default to quick mode

  UCLI_COMMAND_INFO(
      uc,
      "tm_ind_test",
      1,
      "tm_ind_test <quick | extended> : TM indirect memory w/r test");

  if (uc->pargs->args[0][0] == 'e') {  // check for extended mode
    quick = 0;
  }

  tm_prc_ind_mem_test(quick);

  tm_ind_mem_test(tofino_device_select_tm_top_tm_clc_top_clc_ind_addr_address,
                  tofino_device_select_tm_top_tm_clc_top_clc_ind_data_address,
                  512 * 1024,
                  "clc_ind",
                  quick);

  tm_pex_ind_mem_test(
      tofino_device_select_tm_top_tm_clc_top_pex_ind_addr_address,
      tofino_device_select_tm_top_tm_clc_top_pex_ind_data_address,
      0x6e800,
      "pex_ind",
      quick);

  return 0;
}

static ucli_status_t lld_ucli_ucli__cfg_diags__(ucli_context_t *uc) {
  uint32_t pipe_mask, stage_mask;

  UCLI_COMMAND_INFO(uc, "cfg_diag", 2, "cfg_diag <pipe-mask> <stage-mask>");

  pipe_mask = strtoul(uc->pargs->args[0], NULL, 16) & 0xff;
  stage_mask = strtoul(uc->pargs->args[1], NULL, 16) & 0xfffff;

  aim_printf(&uc->pvs, "Configure Diagnostic limits:\n");

  if (lld_diag_cfg.pipe_mask != pipe_mask) {
    aim_printf(&uc->pvs,
               "pipe mask changed from %1x -> %1x\n",
               lld_diag_cfg.pipe_mask,
               pipe_mask);
    lld_diag_cfg.pipe_mask = pipe_mask;
  }
  if (lld_diag_cfg.stage_mask != stage_mask) {
    aim_printf(&uc->pvs,
               "stage mask changed from %05x -> %05x\n",
               lld_diag_cfg.stage_mask,
               stage_mask);
    lld_diag_cfg.stage_mask = stage_mask;
  }

  aim_printf(&uc->pvs, "New Diagnostic limits:\n");
  aim_printf(&uc->pvs, "  pipe mask  : %1x\n", lld_diag_cfg.pipe_mask);
  aim_printf(&uc->pvs, "  stage mask : %05x\n", lld_diag_cfg.stage_mask);
  return 0;
}

static ucli_status_t lld_ucli_ucli__cfg_diff__(ucli_context_t *uc) {
  FILE *fd;
  uint32_t addr_hi, addr_lo, data_127_96, data_95_64, data_63_32, data_31_0;
  bf_dev_id_t dev_id;
  char *path, reg_name[256], op[16];
  int rc;

  UCLI_COMMAND_INFO(uc, "cfg_diff", 2, "cfg_diff <dev_id> <file-path>");
  dev_id = strtol(uc->pargs->args[0], NULL, 10);
  path = (char *)uc->pargs->args[1];
  fd = fopen(path, "r");
  if (fd == NULL) {
    aim_printf(&uc->pvs, "Error opening file: %s\n", path);
    return 0;
  }
  while (!feof(fd)) {
    uint64_t addr, data_0, data_1, file_data_0, file_data_1;

    rc = fscanf(fd,
                "%x %x %x %x %x %x %15s %255s\n",
                &addr_hi,
                &addr_lo,
                &data_127_96,
                &data_95_64,
                &data_63_32,
                &data_31_0,
                op,
                reg_name);
    if (!rc) {
      bf_sys_assert(0);
    }

    addr = ((uint64_t)addr_hi << 32ull) | (uint64_t)addr_lo;
    file_data_0 = ((uint64_t)data_127_96 << 32ull) | data_95_64;
    file_data_1 = ((uint64_t)data_63_32 << 32ull) | data_31_0;

    if (addr_hi != 0) {
      rc = lld_subdev_ind_read(0, 0, addr, &data_0, &data_1);
      if (rc != 0) {
        aim_printf(&uc->pvs,
                   "Error: %d : reading %016" PRIx64 " : %s\n",
                   rc,
                   addr,
                   reg_name);
        fclose(fd);
        return 0;
      }
    } else {
      uint32_t data32 = 0;
      rc = lld_subdev_read_register(dev_id, 0, addr_lo, &data32);
      if (rc != 0) {
        aim_printf(&uc->pvs,
                   "Error: %d : reading %016" PRIx64 " : %s\n",
                   rc,
                   addr,
                   reg_name);
        fclose(fd);
        return 0;
      }
      data_0 = 0ull;
      data_1 = (uint64_t)data32;
    }
    if ((data_0 != file_data_0) || (data_1 != file_data_1)) {
      aim_printf(&uc->pvs, "%016" PRIx64 " : %s\n", addr, reg_name);
      aim_printf(&uc->pvs,
                 "             exp : %016" PRIx64 "_%016" PRIx64 "\n",
                 file_data_0,
                 file_data_1);
      aim_printf(&uc->pvs,
                 "             got : %016" PRIx64 "_%016" PRIx64 "\n",
                 data_0,
                 data_1);
    }
  }
  fclose(fd);
  return 0;
}

static ucli_status_t lld_ucli_ucli__cfg_load__(ucli_context_t *uc) {
  FILE *fd;
  uint32_t addr_hi, addr_lo, data_127_96, data_95_64, data_63_32, data_31_0;
  bf_dev_id_t dev_id;
  char *path, reg_name[256], op[16];
  int rc, progress;

  UCLI_COMMAND_INFO(uc, "cfg_load", 2, "cfg_load <dev_id> <file-path>");
  dev_id = strtol(uc->pargs->args[0], NULL, 10);
  path = (char *)uc->pargs->args[1];
  fd = fopen(path, "r");
  if (fd == NULL) {
    aim_printf(&uc->pvs, "Error opening file: %s\n", path);
    return 0;
  }

  aim_printf(&uc->pvs, "Configuration load from : %s ...\n", path);
  progress = 0;
  while (!feof(fd)) {
    uint64_t addr, file_data_0, file_data_1;
    char progress_clk[4] = {'|', '/', '-', '\\'};

    if ((progress++ % 1024) == 0) {
      aim_printf(&uc->pvs, "%c\r", progress_clk[(progress / 1024) % 4]);
    }
    rc = fscanf(fd,
                "%x %x %x %x %x %x %15s %255s\n",
                &addr_hi,
                &addr_lo,
                &data_127_96,
                &data_95_64,
                &data_63_32,
                &data_31_0,
                op,
                reg_name);
    if (!rc) {
      bf_sys_assert(0);
    }

    addr = ((uint64_t)addr_hi << 32ull) | (uint64_t)addr_lo;
    file_data_0 = ((uint64_t)data_127_96 << 32ull) | data_95_64;
    file_data_1 = ((uint64_t)data_63_32 << 32ull) | data_31_0;

    if (addr_hi != 0) {
      rc = lld_subdev_ind_write(dev_id, 0, addr, file_data_0, file_data_1);
      if (rc != 0) {
        aim_printf(&uc->pvs,
                   "Error: %d : writing %016" PRIx64 " : %s\n",
                   rc,
                   addr,
                   reg_name);
        fclose(fd);
        return 0;
      }
    } else {
      uint32_t data32 = data_31_0;
      rc = lld_subdev_write_register(dev_id, 0, addr_lo, data32);
      if (rc != 0) {
        aim_printf(&uc->pvs,
                   "Error: %d : writing %016" PRIx64 " : %s\n",
                   rc,
                   addr,
                   reg_name);
        fclose(fd);
        return 0;
      }
    }
  }
  fclose(fd);
  aim_printf(&uc->pvs, "Configuration load from : %s : Done.\n", path);

  return 0;
}

extern int dump_reg_name_by_offset(bf_dev_id_t dev_id, uint32_t target_offset);

static ucli_status_t lld_ucli_ucli__cfg_verify__(ucli_context_t *uc) {
  FILE *fd;
  char *path;
  int progress;
  bf_dev_id_t dev_id;

  UCLI_COMMAND_INFO(uc, "cfg_verify", 2, "cfg_verify <dev_id> <file-path>");
  dev_id = strtol(uc->pargs->args[0], NULL, 10);
  path = (char *)uc->pargs->args[1];
  fd = fopen(path, "r");
  if (fd == NULL) {
    aim_printf(&uc->pvs, "Error opening file: %s\n", path);
    return 0;
  }

  aim_printf(&uc->pvs, "Configuration verify from : %s ...\n", path);
  progress = 0;
  while (!feof(fd)) {
    char progress_clk[4] = {'|', '/', '-', '\\'};
    char typ;

    if ((progress++ % 1024) == 0) {
      aim_printf(&uc->pvs, "%c\r", progress_clk[(progress / 1024) % 4]);
    }
    if (0 == fscanf(fd, "%c ", &typ)) bf_sys_assert(0);
    if (typ == 'I') {  // indirect access
      uint64_t i_addr, i_data_hi, i_data_lo, data_hi, data_lo;

      if (0 == fscanf(fd,
                      "%" PRIx64 " %" PRIx64 " %" PRIx64 "\n",
                      &i_addr,
                      &i_data_hi,
                      &i_data_lo)) {
        bf_sys_assert(0);
      }
      lld_subdev_ind_read(dev_id, 0, i_addr, &data_hi, &data_lo);
      if ((data_hi != i_data_hi) || (data_lo != i_data_lo)) {
        aim_printf(&uc->pvs, "Mismatch: Addr : %016" PRIx64 "\n", i_addr);
        aim_printf(&uc->pvs,
                   "          Exp  : %016" PRIx64 "_%016" PRIx64 "\n",
                   i_data_hi,
                   i_data_lo);
        aim_printf(&uc->pvs,
                   "          Got  : %016" PRIx64 "_%016" PRIx64 "\n",
                   data_hi,
                   data_lo);
      }
    } else if (typ == 'D') {  // Direct access
      uint32_t d_addr, d_data, data;

      if (0 == fscanf(fd, "%x %x\n", &d_addr, &d_data)) bf_sys_assert(0);
      lld_subdev_read_register(dev_id, 0, d_addr, &data);
      if (data != d_data) {
        aim_printf(&uc->pvs, "Mismatch: Addr : %08x : ", d_addr);
        dump_reg_name_by_offset(dev_id, d_addr);
        aim_printf(&uc->pvs, "\n");
        aim_printf(&uc->pvs, "          Exp  : %08x\n", d_data);
        aim_printf(&uc->pvs, "          Got  : %08x\n", data);
      }
    } else {
      aim_printf(&uc->pvs, "Error: Invalid access type: %c\n", typ);
      fclose(fd);
      return 0;
    }
  }
  fclose(fd);
  aim_printf(&uc->pvs, "Configuration verify from : %s : Done.\n", path);

  return 0;
}

void lgset_help(void) {
  printf("e.g. lgset <glbl dev_id mac port dma> <id1> <id2> <p3=verbosity>\n");
  printf("     lgset glbl 0 0 1   <- enable glbl\n");
  printf("     lgset dev_id 2 0 255 <- enable chip #2 logs\n");
  printf("     lgset mac  3 2 255 <- enable chip #3, mac_blk #2 logs\n");
  printf("     lgset port 4 3 2   <- enable chip #4, port #3 logs\n");
  printf("     lgset dma  5 4 0   <- disable chip #5, dr #4 logs\n");
}
static ucli_status_t lld_ucli_ucli__lgset__(ucli_context_t *uc) {
  int rc = 0, p1 = 0, p2 = 0, p3 = 0;
  char *typ_str = "?";

  UCLI_COMMAND_INFO(uc, "lgset", -1, "Set logging parameters");

  if (uc->pargs->count == 0) {
    lld_log_settings();
    return 0;
  }
  if (uc->pargs->count != 4) {
    lgset_help();
    return 0;
  }
  if (uc->pargs->count >= 1) typ_str = (char *)uc->pargs->args[0];
  if (uc->pargs->count >= 2) p1 = atoi(uc->pargs->args[1]);
  if (uc->pargs->count >= 3) p2 = atoi(uc->pargs->args[2]);
  if (uc->pargs->count >= 4) p3 = atoi(uc->pargs->args[3]);

  if (!strcmp(typ_str, "glbl")) {
    rc = lld_log_set(LOG_TYP_GLBL, p1, p2, p3);
  } else if (!strcmp(typ_str, "chip")) {
    rc = lld_log_set(LOG_TYP_CHIP, p1, p2, p3);
  } else if (!strcmp(typ_str, "mac")) {
    rc = lld_log_set(LOG_TYP_MAC, p1, p2, p3);
  } else if (!strcmp(typ_str, "port")) {
    rc = lld_log_set(LOG_TYP_PORT, p1, p2, p3);
  } else if (!strcmp(typ_str, "dma")) {
    rc = lld_log_set(LOG_TYP_DMA, p1, p2, p3);
  } else {
    printf("Unrecognized log class: %s (glbl, chip, mac, port, dma)\n",
           typ_str);
    rc = -1;
  }
  if (rc != 0) {
    lgset_help();
  }
  return 0;
}

bf_status_t dump_mem_info_cb(bf_dev_id_t dev_id,
                             bf_subdev_id_t subdev_id,
                             uint64_t offset,
                             uint32_t len,
                             char *key_wd) {
  (void)dev_id;
  (void)subdev_id;
  printf("%016" PRIx64 " : %08x : %s\n", offset, len, key_wd);
  return BF_SUCCESS;
}

static ucli_status_t lld_ucli_ucli__mem_info__(ucli_context_t *uc) {
  bf_dev_id_t dev_id;
  UCLI_COMMAND_INFO(uc, "mem_info", 1, "dump memory info <dev_id>");

  dev_id = strtol(uc->pargs->args[0], NULL, 10);
  lld_traverse_all_mems(dev_id, dump_mem_info_cb, true, false);
  return 0;
}

uint32_t volatile sysex_end[BF_MAX_DEV_COUNT][BF_MAX_SUBDEV_COUNT];
uint32_t sysex_ilist_outstanding[BF_MAX_DEV_COUNT][BF_MAX_SUBDEV_COUNT];
uint32_t sysex_tx_pkt_outstanding[BF_MAX_DEV_COUNT][BF_MAX_SUBDEV_COUNT];
uint32_t sysex_wl_outstanding[BF_MAX_DEV_COUNT][BF_MAX_SUBDEV_COUNT];
uint32_t sysex_wl_2_outstanding[BF_MAX_DEV_COUNT][BF_MAX_SUBDEV_COUNT];

typedef struct wl_entry_t {
  uint64_t addr;
  uint32_t data;
} wl_entry_t;

#if 0
wl_entry_t tm_scratch_regs[] = {
    {offsetof(Tofino, device_select.tm_top.tm_caa.scratch),
     offsetof(Tofino, device_select.tm_top.tm_caa.scratch)},
    {offsetof(Tofino, device_select.tm_top.tm_sch_top.sch[0].scratch),
     offsetof(Tofino, device_select.tm_top.tm_sch_top.sch[0].scratch)},
    {offsetof(Tofino, device_select.tm_top.tm_sch_top.sch[1].scratch),
     offsetof(Tofino, device_select.tm_top.tm_sch_top.sch[1].scratch)},
    {offsetof(Tofino, device_select.tm_top.tm_sch_top.sch[2].scratch),
     offsetof(Tofino, device_select.tm_top.tm_sch_top.sch[2].scratch)},
    {offsetof(Tofino, device_select.tm_top.tm_sch_top.sch[3].scratch),
     offsetof(Tofino, device_select.tm_top.tm_sch_top.sch[3].scratch)},
    {offsetof(Tofino, device_select.tm_top.tm_clc_top.clc_common.scratch),
     offsetof(Tofino, device_select.tm_top.tm_clc_top.clc_common.scratch)},
    {offsetof(Tofino, device_select.tm_top.tm_qlc_top.qlc_common.scratch),
     offsetof(Tofino, device_select.tm_top.tm_qlc_top.qlc_common.scratch)},
    {offsetof(Tofino, device_select.tm_top.tm_prc_top.prc_common.scratch),
     offsetof(Tofino, device_select.tm_top.tm_prc_top.prc_common.scratch)},
    {offsetof(Tofino, device_select.tm_top.tm_pre_top.pre_common.scratch),
     offsetof(Tofino, device_select.tm_top.tm_pre_top.pre_common.scratch)},
    {offsetof(Tofino, device_select.tm_top.tm_psc_top.psc_common.scratch),
     offsetof(Tofino, device_select.tm_top.tm_psc_top.psc_common.scratch)}};
#else
wl_entry_t tm_scratch_regs[] = {
    {DEF_tof2_reg_device_select_tm_top_tm_wac_top_wac_common_wac_common_wac_scratch_address,
     DEF_tof2_reg_device_select_tm_top_tm_wac_top_wac_common_wac_common_wac_scratch_address},
    {DEF_tof2_reg_device_select_tm_top_tm_caa_top_scratch_address,
     DEF_tof2_reg_device_select_tm_top_tm_caa_top_scratch_address},
    {DEF_tof2_reg_device_select_tm_top_tm_qac_top_qac_pipe_qac_reg_scratch_address,
     DEF_tof2_reg_device_select_tm_top_tm_qac_top_qac_pipe_qac_reg_scratch_address},
    {DEF_tof2_reg_device_select_tm_top_tm_scha_top_sch_scratch_address,
     DEF_tof2_reg_device_select_tm_top_tm_scha_top_sch_scratch_address},
    {DEF_tof2_reg_device_select_tm_top_tm_schb_top_sch_scratch_address,
     DEF_tof2_reg_device_select_tm_top_tm_schb_top_sch_scratch_address},
    {DEF_tof2_reg_device_select_tm_top_tm_clc_top_clc_scratch_address,
     DEF_tof2_reg_device_select_tm_top_tm_clc_top_clc_scratch_address},
    {DEF_tof2_reg_device_select_tm_top_tm_pex_top_pex_scratch_address,
     DEF_tof2_reg_device_select_tm_top_tm_pex_top_pex_scratch_address}};

wl_entry_t tm_scratch_regs_2[] = {
    {DEF_tof2_reg_device_select_tm_top_tm_qlc_top_qlc_scratch_address,
     DEF_tof2_reg_device_select_tm_top_tm_qlc_top_qlc_scratch_address},
    {DEF_tof2_reg_device_select_tm_top_tm_qlc_top_qlc_common_scratch_address,
     DEF_tof2_reg_device_select_tm_top_tm_qlc_top_qlc_common_scratch_address},
    {DEF_tof2_reg_device_select_tm_top_tm_prc_top_prc_scratch_address,
     DEF_tof2_reg_device_select_tm_top_tm_prc_top_prc_scratch_address},
    {DEF_tof2_reg_device_select_tm_top_tm_prc_top_prc_common_scratch_address,
     DEF_tof2_reg_device_select_tm_top_tm_prc_top_prc_common_scratch_address},
#if 1
    // these seem to read back 0bad0bad on model, so set to,
    // "0" on model
    // "1" on emulator
    {DEF_tof2_reg_device_select_tm_top_tm_pre_top_pre_scratch_address,
     DEF_tof2_reg_device_select_tm_top_tm_pre_top_pre_scratch_address},
    {DEF_tof2_reg_device_select_tm_top_tm_pre_top_pre_common_scratch_address,
     DEF_tof2_reg_device_select_tm_top_tm_pre_top_pre_common_scratch_address},
#endif
    {DEF_tof2_reg_device_select_tm_top_tm_psc_top_psc_scratch_address,
     DEF_tof2_reg_device_select_tm_top_tm_psc_top_psc_scratch_address},
    {DEF_tof2_reg_device_select_tm_top_tm_psc_top_psc_common_scratch_address,
     DEF_tof2_reg_device_select_tm_top_tm_psc_top_psc_common_scratch_address}};
#endif

/* */

void sysex_push_wl(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  uint64_t calc;
  int i, rc, entry_sz, n_entries = 0, aligned_to;
  uint32_t *wd;
  int dma_buf_len = 0x400 + 512;
  bf_dma_addr_t dma_addr_p;
  void *dma_addr_v;
  bool is_tofino = lld_dev_is_tofino(dev_id);
  bool is_tofino2 = lld_dev_is_tof2(dev_id);
  bool is_tofino3 = lld_dev_is_tof3(dev_id);

  entry_sz = 4;
  aligned_to = 64;

  if (!wl_buf_initd) {
    void *vaddr;
    uint64_t paddr;
    rc = lld_diag_dma_alloc(dev_id, subdev_id, dma_buf_len, &vaddr, &paddr);
    if (rc != 0) {
      printf("Error allocating DMA buffer for WB\n");
      return;
    }
    wl_buf_p = paddr;
    wl_buf_v = vaddr;
    wl_buf_initd = 1;
  }

  calc = (uintptr_t)wl_buf_v;
  dma_addr_p = ((wl_buf_p + (aligned_to - 1)) & ~(aligned_to - 1));
  dma_addr_v =
      lld_u64_to_void_ptr(((calc + (aligned_to - 1)) & ~(aligned_to - 1)));

  wd = (uint32_t *)dma_addr_v;
  for (i = 0; i < (int)(sizeof(tm_scratch_regs) / sizeof(tm_scratch_regs[0]));
       i++) {
    uint64_t addr_42b = 0;
    if (is_tofino) {
      addr_42b = tof_map_32b_to_42b_dev_sel_addr(tm_scratch_regs[i].addr);
    } else if (is_tofino2) {
      addr_42b = tof2_map_32b_to_42b_dev_sel_addr(tm_scratch_regs[i].addr);
    } else if (is_tofino3) {
      addr_42b = tof3_map_32b_to_42b_dev_sel_addr(tm_scratch_regs[i].addr);
    }
    *wd++ = (addr_42b & 0xffffffff);
    *wd++ = ((addr_42b >> 32ull) & 0xffffffff);
    *wd++ = tm_scratch_regs[i].data;
    n_entries++;
  }

  rc = lld_subdev_push_wl(dev_id,
                          subdev_id,
                          0,
                          entry_sz,
                          n_entries,
                          dma_addr_p,
                          (uint64_t)lld_ptr_to_u64(dma_addr_v));
  if (rc != 0) {
    printf("Error: %d : from lld_push_que_write_reg_list\n", rc);
  } else {
    sysex_wl_outstanding[dev_id][subdev_id]++;
    lld_dr_start(dev_id, subdev_id, lld_dr_tx_que_write_list);
  }
}

void sysex_push_wl_2(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  uint64_t calc;
  int i, rc, entry_sz, n_entries = 0, aligned_to;
  uint32_t *wd;
  int dma_buf_len = 0x400 + 512;
  bf_dma_addr_t dma_addr_p;
  void *dma_addr_v;
  bool is_tofino = lld_dev_is_tofino(dev_id);
  bool is_tofino2 = lld_dev_is_tof2(dev_id);
  bool is_tofino3 = lld_dev_is_tof3(dev_id);

  entry_sz = 4;
  aligned_to = 64;

  if (!wl_2_buf_initd) {
    void *vaddr;
    uint64_t paddr;
    rc = lld_diag_dma_alloc(dev_id, subdev_id, dma_buf_len, &vaddr, &paddr);
    if (rc != 0) {
      printf("Error allocating DMA buffer for WB\n");
      return;
    }
    wl_2_buf_p = paddr;
    wl_2_buf_v = vaddr;
    wl_2_buf_initd = 1;
  }

  calc = (uintptr_t)wl_2_buf_v;
  dma_addr_p = ((wl_2_buf_p + (aligned_to - 1)) & ~(aligned_to - 1));
  dma_addr_v =
      lld_u64_to_void_ptr(((calc + (aligned_to - 1)) & ~(aligned_to - 1)));

  wd = (uint32_t *)dma_addr_v;
  for (i = 0;
       i < (int)(sizeof(tm_scratch_regs_2) / sizeof(tm_scratch_regs_2[0]));
       i++) {
    uint64_t addr_42b = 0;
    if (is_tofino) {
      addr_42b = tof_map_32b_to_42b_dev_sel_addr(tm_scratch_regs_2[i].addr);
    } else if (is_tofino2) {
      addr_42b = tof2_map_32b_to_42b_dev_sel_addr(tm_scratch_regs_2[i].addr);
    } else if (is_tofino3) {
      addr_42b = tof3_map_32b_to_42b_dev_sel_addr(tm_scratch_regs_2[i].addr);
    }
    *wd++ = (addr_42b & 0xffffffff);
    *wd++ = ((addr_42b >> 32ull) & 0xffffffff);
    *wd++ = tm_scratch_regs_2[i].data;
    n_entries++;
  }

  rc = lld_subdev_push_wl(dev_id,
                          subdev_id,
                          1,
                          entry_sz,
                          n_entries,
                          dma_addr_p,
                          (uint64_t)lld_ptr_to_u64(dma_addr_v));
  if (rc != 0) {
    printf("Error: %d : from lld_push_que_write_reg_list\n", rc);
  } else {
    sysex_wl_2_outstanding[dev_id][subdev_id]++;
    lld_dr_start(dev_id, subdev_id, lld_dr_tx_que_write_list_1);
  }
}

void sysex_wl_completion_callback_fn(int dev_id,
                                     bf_subdev_id_t subdev_id,
                                     bf_dma_dr_id_t dr,
                                     uint64_t ts_sz,
                                     uint32_t attr,
                                     uint32_t status,
                                     uint32_t type,
                                     uint64_t msg_id,
                                     int s,
                                     int e) {
  int i, err = 0;
  uint32_t addr, data;

  sysex_wl_outstanding[dev_id][subdev_id]--;

  for (i = 0; i < (int)(sizeof(tm_scratch_regs) / sizeof(tm_scratch_regs[0]));
       i++) {
    addr = tm_scratch_regs[i].addr;
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    if (addr != data) {
      err++;
      printf("SYSEX : Error : Exp=%08x : Got=%08x\n", addr, data);
    }
    lld_subdev_write_register(dev_id, subdev_id, addr, ~addr);
  }

  if (err) {
    printf("SYSEX : Error : WL : %d errors\n", err);
    printf("DMA c/b: dev=%d, subdev=%d, dr=%d, ts/sz=%" PRIx64
           ", attr=%xh, sts=%xh, type=%d, id=%" PRIx64 " <write-list: %d>\n",
           dev_id,
           subdev_id,
           dr,
           ts_sz,
           attr,
           status,
           type,
           msg_id,
           wl_dma_count);
  }

  if (sysex_end[dev_id][subdev_id]) {
    if (sysex_wl_outstanding[dev_id][subdev_id] > 0) {
      printf("SYSEX : Waiting for %d wl completions ..\n",
             sysex_wl_outstanding[dev_id][subdev_id]);
    } else {
      printf("SYSEX : wl done.\n");
    }
    return;
  }
  sysex_push_wl(dev_id, subdev_id);

  (void)s;
  (void)e;
}

void sysex_wl_2_completion_callback_fn(int dev_id,
                                       bf_subdev_id_t subdev_id,
                                       bf_dma_dr_id_t dr,
                                       uint64_t ts_sz,
                                       uint32_t attr,
                                       uint32_t status,
                                       uint32_t type,
                                       uint64_t msg_id,
                                       int s,
                                       int e) {
  int i, err = 0;
  uint32_t addr, data;

  sysex_wl_2_outstanding[dev_id][subdev_id]--;

  for (i = 0;
       i < (int)(sizeof(tm_scratch_regs_2) / sizeof(tm_scratch_regs_2[0]));
       i++) {
    addr = tm_scratch_regs_2[i].addr;
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    if (addr != data) {
      err++;
      printf("SYSEX : Error : Exp=%08x : Got=%08x\n", addr, data);
    }
    lld_subdev_write_register(dev_id, subdev_id, addr, ~addr);
  }

  if (err) {
    printf("SYSEX : Error : WL2 : %d errors\n", err);
    printf("DMA c/b: dev=%d, subdev=%d, dr=%d, ts/sz=%" PRIx64
           ", attr=%xh, sts=%xh, type=%d, id=%" PRIx64 " <write-list: %d>\n",
           dev_id,
           subdev_id,
           dr,
           ts_sz,
           attr,
           status,
           type,
           msg_id,
           wl_2_dma_count);
  }

  if (sysex_end[dev_id][subdev_id]) {
    if (sysex_wl_2_outstanding[dev_id][subdev_id] > 0) {
      printf("SYSEX : Waiting for %d wl2 completions ..\n",
             sysex_wl_2_outstanding[dev_id][subdev_id]);
    } else {
      printf("SYSEX : wl2 done.\n");
    }
    return;
  }
  sysex_push_wl_2(dev_id, subdev_id);

  (void)s;
  (void)e;
}

void sysex_wl(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  lld_register_completion_callback(dev_id,
                                   subdev_id,
                                   lld_dr_cmp_que_write_list,
                                   sysex_wl_completion_callback_fn);
  sysex_push_wl(dev_id, subdev_id);
}

void sysex_wl_2(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  lld_register_completion_callback(dev_id,
                                   subdev_id,
                                   lld_dr_cmp_que_write_list_1,
                                   sysex_wl_2_completion_callback_fn);
  sysex_push_wl_2(dev_id, subdev_id);
}

/* Construct an instruction to set the pipe-id and stage-id for an instruction
 * list DR. */
void lld_construct_instr_dest_select(bf_dev_id_t dev_id,
                                     dest_select_t *instr,
                                     bf_dev_pipe_t pipe_id,
                                     int stage_id) {
  pipe_physical_addr_t x;

  memset(instr, 0, sizeof *instr);
  instr->opcode = INSTR_OPCODE_SELECT_DEST;
  instr->data_width = 1;
  instr->pipe_ring_addr_type = addr_type_instruction;

  if (lld_dev_is_tofino(dev_id)) {
    instr->pipe_element_36_33 = stage_id;
    instr->pipe_id_39_37 = pipe_id;
    instr->pipe_41_40 = (PIPE_TOP_LEVEL_PIPES_ADDRESS >> 44);
  } else if (lld_dev_is_tof2(dev_id)) {
    x.addr = 0;
    x.tof2.pipe_always_1 = 1;
    x.tof2.pipe_id = pipe_id;
    x.tof2.pipe_stage = stage_id;
    *(((uint32_t *)instr) + 1) = x.addr >> 32;
  } else if (lld_dev_is_tof3(dev_id)) {
    x.addr = 0;
    x.tof3.pipe_always_1 = 1;
    x.tof3.pipe_id = pipe_id;
    x.tof3.pipe_stage = stage_id;
    *(((uint32_t *)instr) + 1) = x.addr >> 32;
  }
}

// one entry per pipe/stage
// uint8_t sysex_ilist_buf[4][12][1024 + 256];
uint32_t ilist_addr_list[] = {
    // pipes[0] pardereg pgstnreg ebuf900reg[0-3] ebuf400reg[0-1] scratch
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ebuf900reg_ebuf400reg_scratch_address +
     (0 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ebuf900reg_array_element_size) +
     (0 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ebuf900reg_ebuf400reg_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ebuf900reg_ebuf400reg_scratch_address +
     (0 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ebuf900reg_array_element_size) +
     (1 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ebuf900reg_ebuf400reg_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ebuf900reg_ebuf400reg_scratch_address +
     (1 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ebuf900reg_array_element_size) +
     (0 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ebuf900reg_ebuf400reg_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ebuf900reg_ebuf400reg_scratch_address +
     (1 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ebuf900reg_array_element_size) +
     (1 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ebuf900reg_ebuf400reg_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ebuf900reg_ebuf400reg_scratch_address +
     (2 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ebuf900reg_array_element_size) +
     (0 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ebuf900reg_ebuf400reg_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ebuf900reg_ebuf400reg_scratch_address +
     (2 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ebuf900reg_array_element_size) +
     (1 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ebuf900reg_ebuf400reg_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ebuf900reg_ebuf400reg_scratch_address +
     (3 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ebuf900reg_array_element_size) +
     (0 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ebuf900reg_ebuf400reg_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ebuf900reg_ebuf400reg_scratch_address +
     (3 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ebuf900reg_array_element_size) +
     (1 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ebuf900reg_ebuf400reg_array_element_size)),

    // pipes[0] pardereg pgstnreg ipbprsr4reg[0-8] ipbreg scratch
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_ipbreg_scratch_address +
     (0 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_ipbreg_scratch_address +
     (1 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_ipbreg_scratch_address +
     (2 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_ipbreg_scratch_address +
     (3 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_ipbreg_scratch_address +
     (4 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_ipbreg_scratch_address +
     (5 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_ipbreg_scratch_address +
     (6 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_ipbreg_scratch_address +
     (7 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_ipbreg_scratch_address +
     (8 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size)),

    // pipes[0] pardereg pgstnreg ipbprsr4reg[0-8] prsr[0-3] scratch
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_scratch_address +
     (0 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size) +
     (0 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_scratch_address +
     (0 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size) +
     (1 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_scratch_address +
     (0 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size) +
     (2 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_scratch_address +
     (0 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size) +
     (3 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_scratch_address +
     (1 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size) +
     (0 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_scratch_address +
     (1 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size) +
     (1 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_scratch_address +
     (1 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size) +
     (2 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_scratch_address +
     (1 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size) +
     (3 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_scratch_address +
     (2 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size) +
     (0 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_scratch_address +
     (2 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size) +
     (1 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_scratch_address +
     (2 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size) +
     (2 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_scratch_address +
     (2 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size) +
     (3 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_scratch_address +
     (3 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size) +
     (0 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_scratch_address +
     (3 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size) +
     (1 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_scratch_address +
     (3 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size) +
     (2 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_scratch_address +
     (3 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size) +
     (3 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_scratch_address +
     (4 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size) +
     (0 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_scratch_address +
     (4 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size) +
     (1 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_scratch_address +
     (4 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size) +
     (2 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_scratch_address +
     (4 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size) +
     (3 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_scratch_address +
     (5 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size) +
     (0 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_scratch_address +
     (5 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size) +
     (1 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_scratch_address +
     (5 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size) +
     (2 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_scratch_address +
     (5 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size) +
     (3 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_scratch_address +
     (6 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size) +
     (0 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_scratch_address +
     (6 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size) +
     (1 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_scratch_address +
     (6 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size) +
     (2 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_scratch_address +
     (6 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size) +
     (3 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_scratch_address +
     (7 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size) +
     (0 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_scratch_address +
     (7 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size) +
     (1 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_scratch_address +
     (7 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size) +
     (2 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_scratch_address +
     (7 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size) +
     (3 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_scratch_address +
     (8 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size) +
     (0 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_scratch_address +
     (8 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size) +
     (1 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_scratch_address +
     (8 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size) +
     (2 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_scratch_address +
     (8 * DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_array_element_size) +
     (3 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_ipbprsr4reg_prsr_array_element_size)),

    // pipes[0] pardereg pgstnreg epbprsr4reg[0-8] prsr[0-3] scratch
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_scratch_address +
     (0 * DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_array_element_size) +
     (0 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_scratch_address +
     (0 * DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_array_element_size) +
     (1 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_scratch_address +
     (0 * DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_array_element_size) +
     (2 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_scratch_address +
     (0 * DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_array_element_size) +
     (3 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_scratch_address +
     (1 * DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_array_element_size) +
     (0 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_scratch_address +
     (1 * DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_array_element_size) +
     (1 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_scratch_address +
     (1 * DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_array_element_size) +
     (2 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_scratch_address +
     (1 * DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_array_element_size) +
     (3 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_scratch_address +
     (2 * DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_array_element_size) +
     (0 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_scratch_address +
     (2 * DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_array_element_size) +
     (1 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_scratch_address +
     (2 * DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_array_element_size) +
     (2 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_scratch_address +
     (2 * DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_array_element_size) +
     (3 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_scratch_address +
     (3 * DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_array_element_size) +
     (0 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_scratch_address +
     (3 * DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_array_element_size) +
     (1 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_scratch_address +
     (3 * DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_array_element_size) +
     (2 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_scratch_address +
     (3 * DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_array_element_size) +
     (3 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_scratch_address +
     (4 * DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_array_element_size) +
     (0 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_scratch_address +
     (4 * DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_array_element_size) +
     (1 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_scratch_address +
     (4 * DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_array_element_size) +
     (2 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_scratch_address +
     (4 * DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_array_element_size) +
     (3 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_scratch_address +
     (5 * DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_array_element_size) +
     (0 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_scratch_address +
     (5 * DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_array_element_size) +
     (1 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_scratch_address +
     (5 * DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_array_element_size) +
     (2 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_scratch_address +
     (5 * DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_array_element_size) +
     (3 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_scratch_address +
     (6 * DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_array_element_size) +
     (0 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_scratch_address +
     (6 * DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_array_element_size) +
     (1 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_scratch_address +
     (6 * DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_array_element_size) +
     (2 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_scratch_address +
     (6 * DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_array_element_size) +
     (3 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_scratch_address +
     (7 * DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_array_element_size) +
     (0 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_scratch_address +
     (7 * DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_array_element_size) +
     (1 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_scratch_address +
     (7 * DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_array_element_size) +
     (2 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_scratch_address +
     (7 * DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_array_element_size) +
     (3 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_scratch_address +
     (8 * DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_array_element_size) +
     (0 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_scratch_address +
     (8 * DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_array_element_size) +
     (1 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_scratch_address +
     (8 * DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_array_element_size) +
     (2 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_scratch_address +
     (8 * DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_array_element_size) +
     (3 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_epbprsr4reg_prsr_array_element_size)),

    // pipes[0] pardereg pgstnreg p2sreg scratch
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_p2sreg_scratch_address),

    // pipes[0] pardereg pgstnreg s2preg scratch
    (DEF_tof2_reg_pipes_pardereg_mirreg_mirror_s2p_regs_scratch_address),

    // pipes[0] pardereg pgstnreg parbreg left scratch
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_parbreg_left_scratch_address),

    // pipes[0] pardereg pgstnreg parbreg right scratch
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_parbreg_right_scratch_address),

    // pipes[0] pardereg pgstnreg pgrreg pgr_app[0-15] scratch
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_app_scratch_address +
     (0 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_app_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_app_scratch_address +
     (1 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_app_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_app_scratch_address +
     (2 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_app_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_app_scratch_address +
     (3 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_app_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_app_scratch_address +
     (4 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_app_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_app_scratch_address +
     (5 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_app_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_app_scratch_address +
     (6 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_app_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_app_scratch_address +
     (7 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_app_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_app_scratch_address +
     (8 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_app_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_app_scratch_address +
     (9 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_app_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_app_scratch_address +
     (10 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_app_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_app_scratch_address +
     (11 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_app_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_app_scratch_address +
     (12 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_app_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_app_scratch_address +
     (13 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_app_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_app_scratch_address +
     (14 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_app_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_app_scratch_address +
     (15 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_app_array_element_size)),

    // pipes[0] pardereg pgstnreg pgrreg pgr_common scratch[0-3]
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_common_scratch_address +
     (0 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_common_scratch_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_common_scratch_address +
     (1 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_common_scratch_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_common_scratch_address +
     (2 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_common_scratch_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_common_scratch_address +
     (3 *
      DEF_tof2_reg_pipes_pardereg_pgstnreg_pgrreg_pgr_common_scratch_array_element_size)),

    // pipes[0] pardereg pgstnreg pmergereg ll0 scratch
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_pmergereg_ll0_scratch_address),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_pmergereg_ll1_scratch_address),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_pmergereg_ul_scratch_address),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_pmergereg_lr0_scratch_address),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_pmergereg_lr1_scratch_address),
    (DEF_tof2_reg_pipes_pardereg_pgstnreg_pmergereg_ur_scratch_address),

    // pipes[0] pardereg dprsrreg dprsrreg ho_e[0-3] her scratch
    (DEF_tof2_reg_pipes_pardereg_dprsrreg_dprsrreg_ho_e_her_scratch_address +
     (0 *
      DEF_tof2_reg_pipes_pardereg_dprsrreg_dprsrreg_ho_e_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_dprsrreg_dprsrreg_ho_e_her_scratch_address +
     (1 *
      DEF_tof2_reg_pipes_pardereg_dprsrreg_dprsrreg_ho_e_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_dprsrreg_dprsrreg_ho_e_her_scratch_address +
     (2 *
      DEF_tof2_reg_pipes_pardereg_dprsrreg_dprsrreg_ho_e_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_dprsrreg_dprsrreg_ho_e_her_scratch_address +
     (3 *
      DEF_tof2_reg_pipes_pardereg_dprsrreg_dprsrreg_ho_e_array_element_size)),

    // pipes[0] pardereg dprsrreg dprsrreg ho_i[0-3] hir scratch
    (DEF_tof2_reg_pipes_pardereg_dprsrreg_dprsrreg_ho_i_hir_scratch_address +
     (0 *
      DEF_tof2_reg_pipes_pardereg_dprsrreg_dprsrreg_ho_i_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_dprsrreg_dprsrreg_ho_i_hir_scratch_address +
     (1 *
      DEF_tof2_reg_pipes_pardereg_dprsrreg_dprsrreg_ho_i_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_dprsrreg_dprsrreg_ho_i_hir_scratch_address +
     (2 *
      DEF_tof2_reg_pipes_pardereg_dprsrreg_dprsrreg_ho_i_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_dprsrreg_dprsrreg_ho_i_hir_scratch_address +
     (3 *
      DEF_tof2_reg_pipes_pardereg_dprsrreg_dprsrreg_ho_i_array_element_size)),

    // pipes[0] pardereg dprsrreg dprsrreg ho_e[0-3] out_egr scratch
    (DEF_tof2_reg_pipes_pardereg_dprsrreg_dprsrreg_ho_e_out_egr_scratch_address +
     (0 *
      DEF_tof2_reg_pipes_pardereg_dprsrreg_dprsrreg_ho_e_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_dprsrreg_dprsrreg_ho_e_out_egr_scratch_address +
     (1 *
      DEF_tof2_reg_pipes_pardereg_dprsrreg_dprsrreg_ho_e_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_dprsrreg_dprsrreg_ho_e_out_egr_scratch_address +
     (2 *
      DEF_tof2_reg_pipes_pardereg_dprsrreg_dprsrreg_ho_e_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_dprsrreg_dprsrreg_ho_e_out_egr_scratch_address +
     (3 *
      DEF_tof2_reg_pipes_pardereg_dprsrreg_dprsrreg_ho_e_array_element_size)),

    (DEF_tof2_reg_pipes_pardereg_dprsrreg_dprsrreg_inp_ipp_main_i_scratch_address),
    (DEF_tof2_reg_pipes_pardereg_dprsrreg_dprsrreg_inp_ipp_main_e_scratch_address),
    (DEF_tof2_reg_pipes_pardereg_dprsrreg_dprsrreg_inp_ipp_scratch_address),
    (DEF_tof2_reg_pipes_pardereg_dprsrreg_dprsrreg_inp_icr_scratch_address),
    (DEF_tof2_reg_pipes_pardereg_dprsrreg_dprsrreg_inp_icr_scratch2_address),
    (DEF_tof2_reg_pipes_pardereg_dprsrreg_dprsrreg_dprsr_pbus_scratch_address),
    (DEF_tof2_reg_pipes_pardereg_dprsrreg_dprsrreg_dprsr_csr_ring_scratch_address),

    //
    (DEF_tof2_reg_pipes_pardereg_dprsrreg_dprsrreg_inpslice_scratch_address +
     (0 *
      DEF_tof2_reg_pipes_pardereg_dprsrreg_dprsrreg_inpslice_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_dprsrreg_dprsrreg_inpslice_scratch_address +
     (1 *
      DEF_tof2_reg_pipes_pardereg_dprsrreg_dprsrreg_inpslice_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_dprsrreg_dprsrreg_inpslice_scratch_address +
     (2 *
      DEF_tof2_reg_pipes_pardereg_dprsrreg_dprsrreg_inpslice_array_element_size)),
    (DEF_tof2_reg_pipes_pardereg_dprsrreg_dprsrreg_inpslice_scratch_address +
     (3 *
      DEF_tof2_reg_pipes_pardereg_dprsrreg_dprsrreg_inpslice_array_element_size)),
};

uint32_t num_inst_to_pgm = sizeof(ilist_addr_list) / sizeof(ilist_addr_list[0]);

void sysex_push_ilist(bf_dev_id_t dev_id,
                      bf_subdev_id_t subdev_id,
                      int pipe,
                      int stage) {
  pipe_instr_write_reg_t *instr;
  int rc, alignment = 64;
  uint8_t *sysex_ilist_buf_aligned = NULL;
  uint32_t i;
  uint32_t pipe_offset = (pipe * 0x1000000);
  uint64_t msg_id = pipe_offset;
  int dma_buf_len = (4 * 20 * (256 + 256));
  bf_dma_addr_t dma_addr_p;
  uint64_t buf_offset = (pipe * 20 * (256ull + 256) + stage * (256ull + 256));
  uint32_t cur_stage = 0xffffffff, ilist_entry = 0;

  if (!il_buf_initd) {
    void *vaddr;
    uint64_t paddr;
    rc = lld_diag_dma_alloc(dev_id, subdev_id, dma_buf_len, &vaddr, &paddr);
    if (rc != 0) {
      printf("Error allocating DMA buffer for IL\n");
      return;
    }
    il_buf_p = paddr;
    il_buf_v = vaddr;
    il_buf_initd = 1;
  }
  sysex_ilist_buf_aligned = (uint8_t *)(uintptr_t)(
      ((uintptr_t)il_buf_v + buf_offset + ((uint64_t)alignment - 1ull)) &
      ~((uint64_t)alignment - 1ull));
  dma_addr_p = (il_buf_p + buf_offset + ((uint64_t)alignment - 1ull)) &
               ~((uint64_t)alignment - 1ull);

  instr = (pipe_instr_write_reg_t *)(sysex_ilist_buf_aligned);

  for (i = 0; i < num_inst_to_pgm; i++) {
    uint32_t this_stage = 0;
    if (lld_dev_is_tofino(dev_id)) {
      this_stage =
          tof_pcie_pipe_addr_get_stage(pipe_offset + ilist_addr_list[i]);
    } else if (lld_dev_is_tof2(dev_id)) {
      this_stage =
          tof2_pcie_pipe_addr_get_stage(pipe_offset + ilist_addr_list[i]);
    } else if (lld_dev_is_tof3(dev_id)) {
      this_stage =
          tof3_pcie_pipe_addr_get_stage(pipe_offset + ilist_addr_list[i]);
    }

    if (cur_stage != this_stage) {
      lld_construct_instr_dest_select(
          0, (dest_select_t *)&instr[ilist_entry], pipe, this_stage);
      cur_stage = this_stage;
      ilist_entry++;
    }
    TOF_CONSTRUCT_WRITE_REG_INSTR(&instr[ilist_entry],
                                  (ilist_addr_list[i]),
                                  (pipe_offset + ilist_addr_list[i]));
    ilist_entry++;
  }

  rc = lld_subdev_push_ilist(dev_id,
                             subdev_id,
                             pipe,
                             dma_addr_p,
                             (ilist_entry) * sizeof(pipe_instr_write_reg_t),
                             0,
                             0,
                             0,
                             msg_id);
  if (rc != 0) {
    printf("SYSEX : Error: %d : from lld_push_pipe_inst_list\n", rc);
  } else {
    sysex_ilist_outstanding[dev_id][subdev_id]++;
    lld_dr_start(dev_id, subdev_id, lld_dr_tx_pipe_inst_list_0 + pipe);
  }
}

void sysex_ilist_completion_callback_fn(bf_dev_id_t dev_id,
                                        bf_subdev_id_t subdev_id,
                                        bf_dma_dr_id_t dr,
                                        uint64_t ts_sz,
                                        uint32_t attr,
                                        uint32_t status,
                                        uint32_t type,
                                        uint64_t msg_id,
                                        int s,
                                        int e) {
  uint32_t addr;
  uint32_t pipe = (uint32_t)(msg_id >> 24ull) & 0x3;
  uint32_t stage = 0;  // unused for prsr/dprsr addresses
  uint32_t pipe_offset = (pipe * 0x1000000);
  uint32_t data;
  uint32_t i;

  sysex_ilist_outstanding[dev_id][subdev_id]--;

  for (i = 0; i < num_inst_to_pgm; i++) {
    addr = (pipe_offset + ilist_addr_list[i]);
    // verify
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    if (data != addr) {
      printf("SYSEX : Error : Exp=%x : Got=%x\n", addr, data);
      printf("SYSEX : ilist cmpl: dev=%d : dr=%d : ts/sz=%" PRIx64
             " : attr=%x : sts=%d : "
             "typ=%d : id=%" PRIx64 "\n",
             dev_id,
             dr,
             ts_sz,
             attr,
             status,
             type,
             msg_id);
    }
    // now change the value
    lld_subdev_write_register(dev_id, subdev_id, addr, ~addr);
  }
  if (sysex_end[dev_id][subdev_id]) {
    if (sysex_ilist_outstanding[dev_id][subdev_id] > 0) {
      printf("SYSEX : Waiting for %d ilist completions ..\n",
             sysex_ilist_outstanding[dev_id][subdev_id]);
    } else {
      printf("SYSEX : ilists done.\n");
    }
    return;
  }

  // then push again
  sysex_push_ilist(dev_id, subdev_id, pipe, stage);
  (void)s;
  (void)e;
}

void sysex_ilist(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  uint32_t s, num_pipes, num_stages;
  bf_dev_pipe_t p;
  bf_dev_pipe_t phy_pipe;

  lld_register_completion_callback(dev_id,
                                   subdev_id,
                                   lld_dr_cmp_pipe_inst_list_0 + 0,
                                   sysex_ilist_completion_callback_fn);
  lld_register_completion_callback(dev_id,
                                   subdev_id,
                                   lld_dr_cmp_pipe_inst_list_0 + 1,
                                   sysex_ilist_completion_callback_fn);
  lld_register_completion_callback(dev_id,
                                   subdev_id,
                                   lld_dr_cmp_pipe_inst_list_0 + 2,
                                   sysex_ilist_completion_callback_fn);
  lld_register_completion_callback(dev_id,
                                   subdev_id,
                                   lld_dr_cmp_pipe_inst_list_0 + 3,
                                   sysex_ilist_completion_callback_fn);

  lld_sku_get_num_active_pipes(dev_id, &num_pipes);

  printf("cfg_diag: pipe-mask=%x : stage_mask=%x\n",
         lld_diag_cfg.pipe_mask,
         lld_diag_cfg.stage_mask);

  for (p = 0; p < num_pipes; p++) {
    if (((int)p / BF_SUBDEV_PIPE_COUNT) != subdev_id) continue;
    int local_pipe = p % BF_SUBDEV_PIPE_COUNT;
    lld_sku_map_pipe_id_to_phy_pipe_id(dev_id, p, &phy_pipe);
    lld_sku_get_num_active_mau_stages(dev_id, &num_stages, phy_pipe);
    printf("pipe=%d : num_stages=%d\n", p, num_stages);
    for (s = 0; s < num_stages; s++) {
      if ((lld_diag_cfg.pipe_mask & (1 << p)) &&
          (lld_diag_cfg.stage_mask & (1 << s))) {
        sysex_push_ilist(dev_id, subdev_id, local_pipe, s);
      }
    }
  }
}

uint8_t sysex_canned_packet_data[] = {
    0x00, 0x11, 0x11, 0x11, 0x11, 0x11, 0x00, 0x55, 0x55, 0x55, 0x55,
    0x55, 0x08, 0x00, 0x45, 0x00, 0x00, 0x56, 0x00, 0x65, 0x00, 0x00,
    0x40, 0x06, 0xaf, 0x93, 0xc0, 0xa8, 0x00, 0x01, 0x0a, 0x00, 0x00,
    0x01, 0x04, 0xd2, 0x00, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x50, 0x02, 0x20, 0x00, 0xc3, 0xd5, 0x00, 0x00, 0x00,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09};

void sysex_push_tx_pkt(bf_dev_id_t dev_id,
                       bf_subdev_id_t subdev_id,
                       int len,
                       int cos,
                       int burst_sz) {
  int rc, i;
  uint8_t *sysex_packet_data;
  bf_dma_addr_t dma_addr_p;

  if (!pkt_buf_initd) {
    void *vaddr;
    uint64_t paddr;
    rc = lld_diag_dma_alloc(dev_id, subdev_id, 9216, &vaddr, &paddr);
    if (rc != 0) {
      printf("Error allocating DMA buffer for WB\n");
      return;
    }
    pkt_buf_p = paddr;
    pkt_buf_v = vaddr;
    pkt_buf_initd = 1;
  }
  sysex_packet_data = (uint8_t *)pkt_buf_v;
  dma_addr_p = pkt_buf_p;

  memcpy(sysex_packet_data,
         sysex_canned_packet_data,
         ((size_t)len <= sizeof(sysex_canned_packet_data)
              ? (size_t)len
              : sizeof(sysex_canned_packet_data)));
  // "cos" is the first byte of the packet
  sysex_packet_data[0] = cos;

  for (i = 0; i < burst_sz; i++) {
    // for Tx, cos=0-3 which specifies the Tx DR to use
    // for Rx, cos=0-7 which specifies the Rx DR to use
    // so, accept cos=0-7 and insert that in pkt, but /2 to specify Tx DR
    //
    // msg_id = cos[18:16] | len[15:0]
    //
    rc = lld_subdev_push_tx_packet(dev_id,
                                   subdev_id,
                                   cos / 2,
                                   1 /*s*/,
                                   1 /*e*/,
                                   len,
                                   dma_addr_p,
                                   (uint64_t)((cos << 16) | len));
    if (rc != BF_SUCCESS) {
      printf("SYSEX : Error: %d : Pushing packet: %d\n", rc, i);
      if (i > 0) {
        lld_dr_start(dev_id, subdev_id, lld_dr_tx_pkt_0 + (cos / 2));
      }
      return;
    }
    sysex_tx_pkt_outstanding[dev_id][subdev_id]++;
  }

  rc = lld_dr_start(dev_id, subdev_id, lld_dr_tx_pkt_0 + (cos / 2));
  if (rc != BF_SUCCESS) {
    printf("SYSEX : Error: %d : on DMA start\n", rc);
  }
}

void sysex_tx_pkt_completion_callback_fn(int dev_id,
                                         bf_subdev_id_t subdev_id,
                                         bf_dma_dr_id_t dr,
                                         uint64_t ts_sz,
                                         uint32_t attr,
                                         uint32_t status,
                                         uint32_t type,
                                         uint64_t msg_id,
                                         int s,
                                         int e) {
  // msg_id = cos[18:16] | len[15:0]
  int len = (int)(msg_id & 0xffff);
  int cos = (int)(msg_id >> 16) & 0x7;
  (void)subdev_id;

  sysex_tx_pkt_outstanding[dev_id][subdev_id]--;

  switch (type) {
    case tx_m_type_pkt:
      if (status != 0) {
        printf("SYSEX : Tx pkt cmpl: dev=%d : dr=%d : ts/sz=%" PRIx64
               " : attr=%d : "
               "sts=%d : msg_id=%016" PRIx64 "\n",
               dev_id,
               dr,
               ts_sz,
               attr,
               status,
               msg_id);
      }
      if (sysex_end[dev_id][subdev_id]) {
        if (sysex_tx_pkt_outstanding[dev_id][subdev_id] > 0) {
          printf("SYSEX : Waiting for %d tx pkt completions ..\n",
                 sysex_tx_pkt_outstanding[dev_id][subdev_id]);
        } else {
          printf("SYSEX : tx pkts done.\n");
        }
        return;
      }
      sysex_push_tx_pkt(dev_id, subdev_id, len, cos, 1);
      break;
    default:
      printf("Incorrect typ: %d\n", type);
      break;
  }
  (void)s;
  (void)e;
}

void sysex_tx_pkt(bf_dev_id_t dev_id,
                  bf_subdev_id_t subdev_id,
                  int len,
                  int cos,
                  int burst_sz) {
  // register callback for completion so can push next pkt
  lld_register_completion_callback(dev_id,
                                   subdev_id,
                                   lld_dr_cmp_tx_pkt_0 + (cos / 2),
                                   sysex_tx_pkt_completion_callback_fn);

  // call common func to send it
  sysex_push_tx_pkt(dev_id, subdev_id, len, cos, burst_sz);
}

static ucli_status_t lld_ucli_ucli__sysex__(ucli_context_t *uc) {
  UCLI_COMMAND_INFO(
      uc, "sysex", 3, " sysex <dev_id>  <subdev_id> <ilist wl2 wl tx_pkt end>");
  bf_dev_id_t dev_id = 0;
  bf_subdev_id_t subdev_id = 0;

  dev_id = atoi(uc->pargs->args[0]);
  subdev_id = atoi(uc->pargs->args[1]);
  lld_dev_t *dev_p = lld_map_subdev_id_to_dev_p(dev_id, subdev_id);
  if (dev_p == NULL) {
    aim_printf(&uc->pvs,
               "Error: device %d  subdevice %d does not exist \n",
               dev_id,
               subdev_id);
    return 0;
  }

  /* Set all the pipes in the pipe-mask */
  lld_diag_cfg_set(0xff, 0xffff, false, false, 0, 0, 0);

  if (!strcmp(uc->pargs->args[2], "tx_pkt")) {
    sysex_end[dev_id][subdev_id] = 0;
    sysex_tx_pkt(dev_id, subdev_id, 64, 0, 2);
    sysex_tx_pkt(dev_id, subdev_id, 128, 2, 2);
    sysex_tx_pkt(dev_id, subdev_id, 256, 4, 2);
    sysex_tx_pkt(dev_id, subdev_id, 512, 6, 2);
  } else if (!strcmp(uc->pargs->args[2], "ilist")) {
    sysex_end[dev_id][subdev_id] = 0;
    sysex_ilist(dev_id, subdev_id);
  } else if (!strcmp(uc->pargs->args[2], "wl2")) {
    sysex_end[dev_id][subdev_id] = 0;
    sysex_wl_2(dev_id, subdev_id);
  } else if (!strcmp(uc->pargs->args[2], "wl")) {
    sysex_end[dev_id][subdev_id] = 0;
    sysex_wl(dev_id, subdev_id);
  } else if (!strcmp(uc->pargs->args[2], "end")) {
    sysex_end[dev_id][subdev_id] = 1;
  }
  return 0;
}

void efuse_display(ucli_context_t *uc,
                   bf_dev_id_t dev_id,
                   bf_subdev_id_t subdev_id) {
  char wafer_str[256] = {0};
  uint64_t hi, lo;

  aim_printf(&uc->pvs, "\n----------------------------------\n");
  lld_efuse_wafer_str_get(dev_id, subdev_id, 256, wafer_str);
  aim_printf(&uc->pvs, "Wafer-id         : %s\n", wafer_str);
  aim_printf(&uc->pvs, "-----------------+----------------\n");

  aim_printf(&uc->pvs, "Device         %d :\n", dev_id);
  aim_printf(&uc->pvs, "Subdevice      %d :\n", subdev_id);
  aim_printf(&uc->pvs, "Efuse settings   :\n");

  aim_printf(&uc->pvs,
             "        %8x : resubmit_disable\n",
             lld_subdev_efuse_get_resubmit_disable(dev_id, subdev_id));
  aim_printf(&uc->pvs,
             "        %8x : mau_tcam_reduction\n",
             lld_subdev_efuse_get_mau_tcam_reduction(dev_id, subdev_id));
  aim_printf(&uc->pvs,
             "        %8x : mau_sram_reduction\n",
             lld_subdev_efuse_get_mau_sram_reduction(dev_id, subdev_id));
  aim_printf(&uc->pvs,
             "        %8x : packet_generator_disable\n",
             lld_subdev_efuse_get_packet_generator_disable(dev_id, subdev_id));
  aim_printf(&uc->pvs,
             "        %8x : pipe_disable\n",
             lld_efuse_get_pipe_disable(dev_id));

  lld_efuse_get_port_disable(dev_id, &hi, &lo);
  if (lld_dev_is_tofino(dev_id)) {
    aim_printf(&uc->pvs,
               "        %8x : mau_stage_disable\n",
               lld_subdev_efuse_get_mau_stage_disable(dev_id, subdev_id, 0));
    aim_printf(&uc->pvs, "%016" PRIx64 " : port_disable (hi)\n", hi);
    aim_printf(&uc->pvs, "%016" PRIx64 " : port_disable (lo)\n", lo);
    aim_printf(&uc->pvs,
               "%016" PRIx64 " : tm_memory_disable\n",
               lld_efuse_get_tm_memory_disable(dev_id));
    aim_printf(&uc->pvs,
               "        %8x : port_speed_reduction\n",
               lld_subdev_efuse_get_port_speed_reduction(dev_id, subdev_id));
  } else if (lld_dev_is_tof2(dev_id) || lld_dev_is_tof3(dev_id)) {
    aim_printf(&uc->pvs,
               "        %8x : soft_pipe_disable\n",
               lld_subdev_efuse_get_soft_pipe_dis(dev_id, subdev_id));
    aim_printf(
        &uc->pvs, "        %8x : rotated\n", lld_efuse_get_die_rotated(dev_id));
    aim_printf(&uc->pvs,
               "        %8x : mau_stage_disable phy-pipe 0\n",
               lld_subdev_efuse_get_mau_stage_disable(dev_id, subdev_id, 0));
    aim_printf(&uc->pvs,
               "        %8x : mau_stage_disable phy-pipe 1\n",
               lld_subdev_efuse_get_mau_stage_disable(dev_id, subdev_id, 1));
    aim_printf(&uc->pvs,
               "        %8x : mau_stage_disable phy-pipe 2\n",
               lld_subdev_efuse_get_mau_stage_disable(dev_id, subdev_id, 2));
    aim_printf(&uc->pvs,
               "        %8x : mau_stage_disable phy-pipe 3\n",
               lld_subdev_efuse_get_mau_stage_disable(dev_id, subdev_id, 3));
    aim_printf(&uc->pvs, "%016" PRIx64 " : port_disable\n", lo);
    aim_printf(&uc->pvs,
               "%016" PRIx64 " : tm_memory_disable\n",
               lld_subdev_efuse_get_tm_memory_disable(dev_id, subdev_id));
    aim_printf(
        &uc->pvs,
        "%016" PRIx64 " : port_speed_reduction\n",
        lld_subdev_efuse_get_eth_port_speed_reduction(dev_id, subdev_id));
  }
  aim_printf(&uc->pvs,
             "        %8x : cpu_port_speed_reduction\n",
             lld_subdev_efuse_get_cpu_port_speed_reduction(dev_id, subdev_id));
  aim_printf(&uc->pvs,
             "        %8x : pcie_lane_reduction\n",
             lld_subdev_efuse_get_pcie_lane_reduction(dev_id, subdev_id));
  aim_printf(&uc->pvs,
             "        %8x : baresync_disable\n",
             lld_subdev_efuse_get_baresync_disable(dev_id, subdev_id));
  if (lld_dev_is_tofino(dev_id)) {
    aim_printf(&uc->pvs,
               "        %8x : frequency_reduction\n",
               lld_efuse_get_frequency_reduction(dev_id));
  } else if (lld_dev_is_tof2(dev_id) || lld_dev_is_tof3(dev_id)) {
    aim_printf(&uc->pvs,
               "        %8x : BPS frequency_reduction BPS\n",
               lld_efuse_get_frequency_reduction_bps(dev_id));
    aim_printf(&uc->pvs,
               "        %8x : PPS frequency_reduction PPS\n",
               lld_efuse_get_frequency_reduction_pps(dev_id));
  }
  aim_printf(&uc->pvs,
             "        %8x : frequency_check_disable\n",
             lld_efuse_get_frequency_check_disable(dev_id));
  aim_printf(&uc->pvs,
             "        %8x : versioning\n",
             lld_subdev_efuse_get_versioning(dev_id, subdev_id));
  if (lld_dev_is_tof2(dev_id) || lld_dev_is_tof3(dev_id)) {
    aim_printf(
        &uc->pvs, "        %8x : device_id\n", lld_efuse_get_device_id(dev_id));
  }

  aim_printf(&uc->pvs,
             "        %8x : chip_part_number\n",
             lld_subdev_efuse_get_chip_part_number(dev_id, subdev_id));
  aim_printf(&uc->pvs,
             "        %8x : part_revision_number\n",
             lld_subdev_efuse_get_part_revision_number(dev_id, subdev_id));
  aim_printf(
      &uc->pvs, "        %8x : package_id\n", lld_efuse_get_package_id(dev_id));
  aim_printf(&uc->pvs,
             "%016" PRIx64 " : chip_id\n",
             lld_subdev_efuse_get_chip_id(dev_id, subdev_id));
  aim_printf(&uc->pvs,
             "        %8x : pmro_and_skew\n",
             lld_subdev_efuse_get_pmro_and_skew(dev_id, subdev_id));
  aim_printf(&uc->pvs,
             "        %8x : voltage_scaling\n",
             lld_efuse_get_voltage_scaling(dev_id));
  aim_printf(&uc->pvs,
             "        %8x : die-config\n",
             lld_subdev_efuse_get_die_config(dev_id, subdev_id));
  aim_printf(&uc->pvs,
             "        %8x : constant0\n",
             lld_efuse_get_constant0(dev_id, subdev_id));
  aim_printf(&uc->pvs,
             "        %8x : constant1\n",
             lld_efuse_get_constant1(dev_id, subdev_id));
  aim_printf(&uc->pvs,
             "        %8x : serdes disable even\n",
             lld_efuse_get_serdes_dis_even(dev_id, subdev_id));
  aim_printf(&uc->pvs,
             "        %8x : serdes disable odd\n",
             lld_efuse_get_serdes_dis_odd(dev_id, subdev_id));
  return;
}

static ucli_status_t lld_ucli_ucli__efuse__(ucli_context_t *uc) {
  UCLI_COMMAND_INFO(
      uc, "efuse", 2, "Dump EFUSE settings <dev_id>  <subdev_id>");
  bf_dev_id_t dev_id;
  bf_subdev_id_t subdev_id;
  lld_dev_t *dev_p = NULL;

  dev_id = atoi(uc->pargs->args[0]);
  subdev_id = atoi(uc->pargs->args[1]);
  dev_p = lld_map_subdev_id_to_dev_p(dev_id, subdev_id);
  if (dev_p == NULL) {
    aim_printf(&uc->pvs,
               "Warning: device %d  subdevice %d not added. Displaying data "
               "for device=0\n",
               dev_id,
               subdev_id);
    dev_id = 0;     // just in case
    subdev_id = 0;  // just in case
    dev_p = lld_map_subdev_id_to_dev_p(dev_id, subdev_id);
    if (dev_p == NULL) {
      aim_printf(&uc->pvs,
                 "Warning: device %d subdevice %d not added either\n",
                 dev_id,
                 subdev_id);
      return 0;
    }
  }
  efuse_display(uc, dev_id, subdev_id);
  return 0;
}

static ucli_command_handler_f bf_drv_show_tech_ucli_lld_handlers__[] = {
    lld_ucli_ucli__dump__,
    lld_ucli_ucli__efuse__,
    lld_ucli_ucli__int_dump__,
    lld_ucli_ucli__int_new__,
    NULL};

ucli_status_t bf_drv_show_tech_ucli_lld__(ucli_context_t *uc) {
  uint32_t hdl_iter = 0;
  aim_printf(&uc->pvs, "-------------------- LLD --------------------\n");
  while (bf_drv_show_tech_ucli_lld_handlers__[hdl_iter]) {
    bf_drv_show_tech_ucli_lld_handlers__[hdl_iter++](uc);
  }
  return 0;
}

static ucli_status_t lld_ucli_ucli__global_ts_enable__(ucli_context_t *uc) {
  bf_dev_id_t asic;
  unsigned int enable;

  UCLI_COMMAND_INFO(
      uc,
      "global_ts_enable",
      2,
      " Enable/Disable global time stamp <asic> <0: disable 1: enable>");

  asic = atoi(uc->pargs->args[0]);
  enable = atoi(uc->pargs->args[1]) ? 1 : 0;

  if (asic >= BF_MAX_DEV_COUNT) {
    aim_printf(&uc->pvs, "Only %d chips defined\n", BF_MAX_DEV_COUNT);
    return 0;
  }

  bf_ts_global_ts_state_set(asic, enable);
  return 0;
}

static ucli_status_t lld_ucli_ucli__global_ts_get__(ucli_context_t *uc) {
  bf_dev_id_t asic;
  uint64_t global_ts = 0xff, baresync_ts = 0xff;

  UCLI_COMMAND_INFO(
      uc, "global_ts_get", 1, " Get global time stamp: global_ts_get <asic> ");

  asic = atoi(uc->pargs->args[0]);

  if (asic >= BF_MAX_DEV_COUNT) {
    aim_printf(&uc->pvs, "Only %d chips defined\n", BF_MAX_DEV_COUNT);
    return 0;
  }

  bf_ts_global_baresync_ts_get(asic, &global_ts, &baresync_ts);

  aim_printf(&uc->pvs,
             "TS: %d : gl_ts: %016" PRIx64 " bs_ts: %016" PRIx64 "\n",
             asic,
             global_ts,
             baresync_ts);
  return 0;
}

static ucli_status_t lld_ucli_ucli__global_tm_set__(ucli_context_t *uc) {
  bf_dev_id_t asic;
  uint64_t ts = 0;

  UCLI_COMMAND_INFO(uc,
                    "global_ts_set",
                    2,
                    " Set global time stamp: global_ts_set <asic> <ts_value>");

  asic = atoi(uc->pargs->args[0]);
  ts = strtoull(uc->pargs->args[1], NULL, 16);

  if (asic >= BF_MAX_DEV_COUNT) {
    aim_printf(&uc->pvs, "Only %d chips defined\n", BF_MAX_DEV_COUNT);
    return 0;
  }

  bf_ts_global_ts_value_set(asic, ts);
  aim_printf(&uc->pvs, "Wr TS: %d : %016" PRIx64 "\n", asic, ts);
  return 0;
}

ucli_context_t *ucli_log_uc;
char ucli_log_line[256];

void ucli_log(char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  vsnprintf(ucli_log_line, sizeof(ucli_log_line) - 1, fmt, args);
  aim_printf(&ucli_log_uc->pvs, "%s", ucli_log_line);

  va_end(args);
}

#include "lld_tof3_eos.h"

void lld_tof2_eos_tm_audit(bf_dev_id_t dev_id, uint32_t phys_pipe_msk);
void lld_tof3_eos_tm_audit(bf_dev_id_t dev_id,
                           bf_subdev_id_t subdev_id,
                           uint32_t phys_pipe_msk);
bool eos_verbose = false;

void lld_tof2_eos_checks_cmd(bf_dev_id_t dev_id, bf_subdev_id_t inp_subdev_id) {
  uint32_t pipe, stage;

  if (lld_dev_is_tof3(dev_id)) {
    uint32_t num_subdev = 0;
    bf_subdev_id_t subdev_id = 0;
    lld_sku_get_num_subdev(dev_id, &num_subdev, NULL);
    for (subdev_id = 0; (uint32_t)subdev_id < num_subdev; subdev_id++) {
      if ((inp_subdev_id != 0xf) && (inp_subdev_id != subdev_id)) {
        continue;
      }
      for (pipe = 0; pipe < 4; pipe++) {
        if (lld_diag_cfg.pipe_mask & (1 << pipe)) {
          for (stage = 0; stage < 20; stage++) {
            if (lld_diag_cfg.stage_mask & (1 << stage)) {
              lld_tof3_eos_mau_audit(dev_id, subdev_id, pipe, stage);
            }
          }
          lld_tof3_non_tm_eos_audit(dev_id, subdev_id, pipe);
        }
      }
      lld_tof3_eos_tm_audit(dev_id, subdev_id, lld_diag_cfg.pipe_mask);
    }
  } else if (lld_dev_is_tof2(dev_id)) {
    for (pipe = 0; pipe < 4; pipe++) {
      if (lld_diag_cfg.pipe_mask & (1 << pipe)) {
        for (stage = 0; stage < 20; stage++) {
          if (lld_diag_cfg.stage_mask & (1 << stage)) {
            lld_tof2_eos_mau_audit(dev_id, pipe, stage);
          }
        }
        lld_tof2_eos_dprsr_audit(dev_id, pipe);
        lld_tof2_eos_ebuf_audit(dev_id, pipe);
        lld_tof2_eos_epb_audit(dev_id, pipe);
        lld_tof2_eos_ibuf_audit(dev_id, pipe);
        lld_tof2_eos_ipb_audit(dev_id, pipe);
        lld_tof2_eos_mirror_audit(dev_id, pipe);
        lld_tof2_eos_prsr_audit(dev_id, pipe);
        lld_tof2_eos_s2p_audit(dev_id, pipe);
      }
    }
    lld_tof2_eos_tm_audit(dev_id, lld_diag_cfg.pipe_mask);
  }
}

static ucli_status_t lld_ucli_ucli__eos__(ucli_context_t *uc) {
  bf_dev_id_t dev_id = 0;
  bf_subdev_id_t subdev_id = 0;
  UCLI_COMMAND_INFO(
      uc, "eos", 2, "Run End-of-Simulation checks <dev_id> <subdev_id>");

  // save context for ucli_log callbacks
  ucli_log_uc = uc;

  dev_id = strtol(uc->pargs->args[0], NULL, 10);
  subdev_id = strtol(uc->pargs->args[1], NULL, 10);

  aim_printf(&uc->pvs,
             "pipe-msk=%x : stage-msk=%x\n",
             lld_diag_cfg.pipe_mask,
             lld_diag_cfg.stage_mask);
  aim_printf(&uc->pvs, "dev=%x : subdev=%x\n", dev_id, subdev_id);

  eos_verbose = false;
  lld_tof2_eos_checks_cmd(dev_id, subdev_id);

  return 0;
}

static ucli_status_t lld_ucli_ucli__eosv__(ucli_context_t *uc) {
  bf_dev_id_t dev_id = 0;
  bf_subdev_id_t subdev_id = 0;
  UCLI_COMMAND_INFO(
      uc,
      "eosv",
      2,
      "Run verbose End-of-Simulation checks <dev_id> <subdev_id>");

  // save context for ucli_log callbacks
  ucli_log_uc = uc;

  dev_id = strtol(uc->pargs->args[0], NULL, 10);
  subdev_id = strtol(uc->pargs->args[1], NULL, 10);

  aim_printf(&uc->pvs,
             "pipe-msk=%x : stage-msk=%x\n",
             lld_diag_cfg.pipe_mask,
             lld_diag_cfg.stage_mask);
  aim_printf(&uc->pvs, "dev=%x : subdev=%x\n", dev_id, subdev_id);

  eos_verbose = true;
  lld_tof2_eos_checks_cmd(dev_id, subdev_id);
  eos_verbose = false;

  return 0;
}

extern void lld_tof2_reg_hole_test_run(bf_dev_id_t dev_id);
extern void lld_tof2_mem_hole_test_run(bf_dev_id_t dev_id);

static ucli_status_t lld_ucli_ucli__reg_holes__(ucli_context_t *uc) {
  bf_dev_id_t dev_id;
  UCLI_COMMAND_INFO(
      uc, "reg_holes", 1, "Run TOF2 CSR register hole checks <dev_id>");

  // save context for ucli_log callbacks
  ucli_log_uc = uc;
  dev_id = strtol(uc->pargs->args[0], NULL, 10);
  lld_tof2_reg_hole_test_run(dev_id);
  return 0;
}

static ucli_status_t lld_ucli_ucli__mem_holes__(ucli_context_t *uc) {
  bf_dev_id_t dev_id;
  UCLI_COMMAND_INFO(uc, "mem_holes", 1, "Run TOF2 memory hole checks <dev_id>");

  // save context for ucli_log callbacks
  ucli_log_uc = uc;
  dev_id = strtol(uc->pargs->args[0], NULL, 10);
  lld_tof2_mem_hole_test_run(dev_id);
  return 0;
}

static ucli_status_t lld_ucli_ucli__core_clk_set__(ucli_context_t *uc) {
  bf_dev_id_t asic;
  bf_subdev_id_t subdev_id;
  uint32_t clk_val = 0, read_clk_val = 0;
  bf_status_t status = BF_SUCCESS;

  UCLI_COMMAND_INFO(
      uc, "core-clk-set", 3, "Set the core clock <asic> <subdev> <value>");

  asic = atoi(uc->pargs->args[0]);
  subdev_id = atoi(uc->pargs->args[1]);
  clk_val = strtoul(uc->pargs->args[2], NULL, 0);

  // Just validating it for MAX value
  if (clk_val == UINT_MAX) {
    aim_printf(&uc->pvs, "Invalid core clock 0x%x\n", clk_val);
    return 0;
  }

  if (asic >= BF_MAX_DEV_COUNT || subdev_id >= BF_MAX_SUBDEV_COUNT) {
    aim_printf(&uc->pvs,
               "Only %d:%d chips defined\n",
               BF_MAX_DEV_COUNT,
               BF_MAX_SUBDEV_COUNT);
    return 0;
  }

  if (lld_dev_is_tofino(asic)) {
    aim_printf(&uc->pvs, "Setting core clock to 0x%x\n", clk_val);

    status = lld_dev_tof_change_core_clk(asic, clk_val);
    if (status == BF_SUCCESS) {
      aim_printf(
          &uc->pvs, "Success: Core clock successfully set to 0x%x\n", clk_val);
    } else {
      aim_printf(&uc->pvs, "Error: Core clock set failed\n");
    }
    aim_printf(&uc->pvs, "--------------------------------------\n");

    aim_printf(&uc->pvs,
               "Clock Verification: Reading the Clock value in asic now ..\n");
    status = lld_dev_tof_get_core_clk(asic, &read_clk_val);
    if (status == BF_SUCCESS) {
      /* Bit 31 gets cleared by asic after it is set, account for that */
      if (clk_val & 0x80000000) {
        read_clk_val |= 0x80000000;
      }
      if (clk_val != read_clk_val) {
        aim_printf(&uc->pvs,
                   "Error: Core clock value was not correctly set in asic\n");
        aim_printf(&uc->pvs,
                   "Read Core clock value from asic is 0x%x \n",
                   read_clk_val);
      } else {
        aim_printf(&uc->pvs, "Success: Core clock was correctly set in asic\n");
      }
    } else {
      aim_printf(&uc->pvs, "Error: Core clock read failed\n");
    }
  } else if (lld_dev_is_tof2(asic)) {
    status = lld_dev_tof2_set_core_clock(asic, clk_val);
    if (status != BF_SUCCESS) {
      aim_printf(&uc->pvs, "Error: Core clock set failed\n");
    } else {
      aim_printf(
          &uc->pvs, "Success: Core clock successfully set to 0x%x\n", clk_val);
    }
  } else if (lld_dev_is_tof3(asic)) {
    status = lld_dev_tof3_set_core_clock(asic, subdev_id, clk_val);
    if (status != BF_SUCCESS) {
      aim_printf(&uc->pvs, "Error: Core clock set failed\n");
    } else {
      aim_printf(
          &uc->pvs, "Success: Core clock successfully set to 0x%x\n", clk_val);
    }
  } else {
    aim_printf(&uc->pvs, "Error: Chip type not supported\n");
  }

  return 0;
}

static ucli_status_t lld_ucli_ucli__core_clk_get__(ucli_context_t *uc) {
  bf_dev_id_t asic;
  bf_subdev_id_t subdev_id;
  uint32_t clk_val = 0;
  bf_status_t status = BF_SUCCESS;

  UCLI_COMMAND_INFO(
      uc, "core-clk-get", 2, "Get the core clock <asic> <subdev>");

  asic = atoi(uc->pargs->args[0]);
  subdev_id = atoi(uc->pargs->args[1]);

  if (asic >= BF_MAX_DEV_COUNT || subdev_id >= BF_MAX_SUBDEV_COUNT) {
    aim_printf(&uc->pvs,
               "Only %d:%d chips defined\n",
               BF_MAX_DEV_COUNT,
               BF_MAX_SUBDEV_COUNT);
    return 0;
  }

  if (lld_dev_is_tofino(asic)) {
    status = lld_dev_tof_get_core_clk(asic, &clk_val);
    if (status == BF_SUCCESS) {
      /* Bit 31 gets cleared by asic after it is set, account for that */
      clk_val |= 0x80000000;
      aim_printf(
          &uc->pvs, "Current Core clock value in asic is 0x%x\n", clk_val);
    } else {
      aim_printf(&uc->pvs, "Error: Core clock read failed\n");
    }
  } else if (lld_dev_is_tof2(asic)) {
    int freq = 0;
    status = lld_dev_tof2_get_core_clock(asic, &freq, &clk_val);
    if (status == BF_SUCCESS) {
      aim_printf(&uc->pvs,
                 "Current Core clock value in asic is 0x%x (%dHz)\n",
                 clk_val,
                 freq);
    } else {
      aim_printf(&uc->pvs, "Error: Core clock read failed\n");
    }
  } else if (lld_dev_is_tof3(asic)) {
    int freq = 0;
    status = lld_dev_tof3_get_core_clock(asic, subdev_id, &freq, &clk_val);
    if (status == BF_SUCCESS) {
      aim_printf(&uc->pvs,
                 "Current Core clock value in asic is 0x%x (%dHz)\n",
                 clk_val,
                 freq);
    } else {
      aim_printf(&uc->pvs, "Error: Core clock read failed\n");
    }
  } else {
    aim_printf(&uc->pvs, "Error: Chip type not supported\n");
  }

  return 0;
}

static ucli_status_t lld_ucli_ucli__pps_clk_set__(ucli_context_t *uc) {
  bf_dev_id_t asic;
  bf_subdev_id_t subdev_id;
  uint32_t clk_val = 0;
  bf_status_t status = BF_SUCCESS;

  UCLI_COMMAND_INFO(
      uc, "pps-clk-set", 3, "Set the core clock <asic> <subdev> <Freq>");

  asic = atoi(uc->pargs->args[0]);
  subdev_id = atoi(uc->pargs->args[1]);
  clk_val = strtoul(uc->pargs->args[2], NULL, 0);

  // Just validating it for MAX value
  if (clk_val == UINT_MAX) {
    aim_printf(&uc->pvs, "Invalid core clock 0x%x\n", clk_val);
    return 0;
  }

  if (asic >= BF_MAX_DEV_COUNT || subdev_id >= BF_MAX_SUBDEV_COUNT) {
    aim_printf(&uc->pvs,
               "Only %d:%d chips defined\n",
               BF_MAX_DEV_COUNT,
               BF_MAX_SUBDEV_COUNT);
    return 0;
  }

  if (lld_dev_is_tof2(asic)) {
    status = lld_dev_tof2_set_pps_clock(asic, clk_val);
    if (status != BF_SUCCESS) {
      aim_printf(&uc->pvs, "Error: PPS clock set failed\n");
    } else {
      aim_printf(
          &uc->pvs, "Success: PPS clock successfully set to 0x%x\n", clk_val);
    }
  } else if (lld_dev_is_tof3(asic)) {
    status = lld_dev_tof3_set_pps_clock(asic, subdev_id, clk_val);
    if (status != BF_SUCCESS) {
      aim_printf(&uc->pvs, "Error: PPS clock set failed\n");
    } else {
      aim_printf(
          &uc->pvs, "Success: PPS clock successfully set to 0x%x\n", clk_val);
    }
  } else {
    aim_printf(&uc->pvs, "Error: Chip type not supported\n");
  }

  return 0;
}
static ucli_status_t lld_ucli_ucli__pps_clk_get__(ucli_context_t *uc) {
  bf_dev_id_t asic;
  bf_subdev_id_t subdev_id;
  uint32_t clk_val = 0;
  bf_status_t status = BF_SUCCESS;

  UCLI_COMMAND_INFO(uc, "pps-clk-get", 2, "Get the pps clock <asic> <subdev>");

  asic = atoi(uc->pargs->args[0]);
  subdev_id = atoi(uc->pargs->args[1]);

  if (asic >= BF_MAX_DEV_COUNT || subdev_id >= BF_MAX_SUBDEV_COUNT) {
    aim_printf(&uc->pvs,
               "Only %d:%d chips defined\n",
               BF_MAX_DEV_COUNT,
               BF_MAX_SUBDEV_COUNT);
    return 0;
  }

  if (lld_dev_is_tof2(asic)) {
    int freq = 0;
    status = lld_dev_tof2_get_pps_clock(asic, &freq, &clk_val);
    if (status == BF_SUCCESS) {
      aim_printf(&uc->pvs,
                 "Current PPS clock value in asic is 0x%x (%dHz)\n",
                 clk_val,
                 freq);
    } else {
      aim_printf(&uc->pvs, "Error: PPS clock read failed\n");
    }
  } else if (lld_dev_is_tof3(asic)) {
    int freq = 0;
    status = lld_dev_tof3_get_pps_clock(asic, subdev_id, &freq, &clk_val);
    if (status == BF_SUCCESS) {
      aim_printf(&uc->pvs,
                 "Current PPS clock value in asic is 0x%x (%dHz)\n",
                 clk_val,
                 freq);
    } else {
      aim_printf(&uc->pvs, "Error: PPS clock read failed\n");
    }
  } else {
    aim_printf(&uc->pvs, "Error: Chip type not supported\n");
  }

  return 0;
}

static ucli_status_t lld_ucli_ucli__tm_clk_set__(ucli_context_t *uc) {
  bf_dev_id_t asic;
  bf_subdev_id_t subdev_id;
  uint32_t clk_val = 0;
  bf_status_t status = BF_SUCCESS;

  UCLI_COMMAND_INFO(
      uc, "tm-clk-set", 3, "Set the TM clock <asic> <subdev> <Freq>");

  asic = atoi(uc->pargs->args[0]);
  subdev_id = atoi(uc->pargs->args[1]);
  clk_val = strtoul(uc->pargs->args[2], NULL, 0);

  // Just validating it for MAX value
  if (clk_val == UINT_MAX) {
    aim_printf(&uc->pvs, "Invalid core clock 0x%x\n", clk_val);
    return 0;
  }

  if (asic >= BF_MAX_DEV_COUNT || subdev_id >= BF_MAX_SUBDEV_COUNT) {
    aim_printf(&uc->pvs,
               "Only %d:%d chips defined\n",
               BF_MAX_DEV_COUNT,
               BF_MAX_SUBDEV_COUNT);
    return 0;
  }

  if (lld_dev_is_tof3(asic)) {
    status = lld_dev_tof3_set_tm_clock(asic, subdev_id, clk_val);
    if (status != BF_SUCCESS) {
      aim_printf(&uc->pvs, "Error: TM clock set failed\n");
    } else {
      aim_printf(
          &uc->pvs, "Success: TM clock successfully set to 0x%x\n", clk_val);
    }
  } else {
    aim_printf(&uc->pvs, "Error: Chip type not supported\n");
  }

  return 0;
}
static ucli_status_t lld_ucli_ucli__tm_clk_get__(ucli_context_t *uc) {
  bf_dev_id_t asic;
  bf_subdev_id_t subdev_id;
  uint32_t clk_val = 0;
  bf_status_t status = BF_SUCCESS;

  UCLI_COMMAND_INFO(uc, "tm-clk-get", 2, "Get the TM clock <asic> <subdev>");

  asic = atoi(uc->pargs->args[0]);
  subdev_id = atoi(uc->pargs->args[1]);

  if (asic >= BF_MAX_DEV_COUNT || subdev_id >= BF_MAX_SUBDEV_COUNT) {
    aim_printf(&uc->pvs,
               "Only %d:%d chips defined\n",
               BF_MAX_DEV_COUNT,
               BF_MAX_SUBDEV_COUNT);
    return 0;
  }

  if (lld_dev_is_tof3(asic)) {
    int freq = 0;
    status = lld_dev_tof3_get_tm_clock(asic, subdev_id, &freq, &clk_val);
    if (status == BF_SUCCESS) {
      aim_printf(&uc->pvs,
                 "Current TM clock value in asic is 0x%x (%dHz)\n",
                 clk_val,
                 freq);
    } else {
      aim_printf(&uc->pvs, "Error: TM clock read failed\n");
    }
  } else {
    aim_printf(&uc->pvs, "Error: Chip type not supported\n");
  }

  return 0;
}
static ucli_status_t lld_ucli_ucli__csr2jtag__(ucli_context_t *uc) {
  bf_dev_id_t asic;
  bf_status_t status = BF_SUCCESS;

  UCLI_COMMAND_INFO(uc, "csr2jtag", 2, "Run csr2jtag script <dev_id> <file>");

  asic = atoi(uc->pargs->args[0]);
  const char *f_name = uc->pargs->args[1];

  if (asic >= BF_MAX_DEV_COUNT) {
    aim_printf(&uc->pvs, "Only %d chips defined\n", BF_MAX_DEV_COUNT);
    return 0;
  }

  aim_printf(&uc->pvs, "Running \"%s\" on dev %d\n", f_name, asic);
  status = lld_csr2jtag_run_one_file(asic, 0, f_name);
  aim_printf(&uc->pvs, "Done, status %s\n", bf_err_str(status));

  return 0;
}

static ucli_status_t lld_ucli_ucli__csr2jtag_suite__(ucli_context_t *uc) {
  bf_dev_id_t asic;
  bf_status_t status = BF_SUCCESS;

  UCLI_COMMAND_INFO(
      uc, "csr2jtag_suite", 2, "Run csr2jtag script-list <dev_id> <file>");

  asic = atoi(uc->pargs->args[0]);
  const char *f_name = uc->pargs->args[1];

  if (asic >= BF_MAX_DEV_COUNT) {
    aim_printf(&uc->pvs, "Only %d chips defined\n", BF_MAX_DEV_COUNT);
    return 0;
  }

  aim_printf(&uc->pvs, "Running \"%s\" on dev %d\n", f_name, asic);
  status = lld_csr2jtag_run_suite(asic, 0, f_name);
  aim_printf(&uc->pvs, "Done, status %s\n", bf_err_str(status));

  return 0;
}

static ucli_status_t lld_ucli_ucli__pcie_fifo_dump__(ucli_context_t *uc) {
  bf_dev_id_t device_id;
  uint16_t mode;
  bf_status_t status = BF_SUCCESS;

  UCLI_COMMAND_INFO(
      uc,
      "pcie_fifo_dump",
      2,
      "<dev_id> <mode> go through T2 PCIe debug FIFO and dump non-zero "
      "entries. mode 0 goes through all 4096 entries. mode 1 goes through "
      "entries between current hw head and tail pointers. default logging "
      "level dumps into bf_drivers.log");

  device_id = strtoul(uc->pargs->args[0], NULL, 0);
  mode = strtoul(uc->pargs->args[1], NULL, 0);

  if (device_id >= BF_MAX_DEV_COUNT) {
    aim_printf(&uc->pvs, "Only %d chips defined\n", BF_MAX_DEV_COUNT);
    return 0;
  }  // end if

  status = port_mgr_tof2_microp_pcie_debug_fifo_dump(device_id, mode);
  aim_printf(&uc->pvs, "status: %s\n", bf_err_str(status));

  return 0;
}  // end lld_ucli_ucli__pcie_fifo_dump__

static ucli_status_t lld_ucli_ucli__pcie_fifo_clear__(ucli_context_t *uc) {
  bf_dev_id_t device_id;
  uint16_t mode;
  bf_status_t status = BF_SUCCESS;

  UCLI_COMMAND_INFO(uc,
                    "pcie_fifo_clear",
                    2,
                    "<dev_id> <mode> clear T2 PCIe debug FIFO. mode determines "
                    "the entries to clear. 0 clears all 4096 entries. 1 clears "
                    "entries between current hw head and tail pointers. "
                    "default logging level logs into bf_drivers.log");

  device_id = strtoul(uc->pargs->args[0], NULL, 0);
  mode = strtoul(uc->pargs->args[1], NULL, 0);

  if (device_id >= BF_MAX_DEV_COUNT) {
    aim_printf(&uc->pvs, "Only %d chips defined\n", BF_MAX_DEV_COUNT);
    return 0;
  }  // end if

  status = port_mgr_tof2_microp_pcie_debug_fifo_clear(device_id, mode);
  aim_printf(&uc->pvs, "status: %s\n", bf_err_str(status));

  return 0;
}  // end lld_ucli_ucli__pcie_fifo_clear__

/* <auto.ucli.handlers.start> */
/* <auto.ucli.handlers.end> */
static ucli_command_handler_f lld_ucli_ucli_handlers__[] = {
    // interactive, command-completion prompt
    lld_ucli_ucli__access__,
    lld_ucli_ucli__find__,
    // basic register and memory access
    lld_ucli_ucli__reg_wr__,
    lld_ucli_ucli__reg_wro__,
    lld_ucli_ucli__reg_rd__,
    lld_ucli_ucli__reg_rdo__,
    lld_ucli_ucli__reg_rdm__,
    lld_ucli_ucli__ind_wr__,
    lld_ucli_ucli__ind_rd__,
    lld_ucli_ucli__mem_rd8__,
    lld_ucli_ucli__mem_rd32__,
    lld_ucli_ucli__mem_rd64__,
    // interrupt related commands
    lld_ucli_ucli__find_int_regs__,
    lld_ucli_ucli__int_dump__,
    lld_ucli_ucli__int_poll__,
    lld_ucli_ucli__int_new__,
    lld_ucli_ucli__int_clr__,
    lld_ucli_ucli__clr_ints__,
    lld_ucli_ucli__new_ints__,
    lld_ucli_ucli__int_all__,
    lld_ucli_ucli__int_en_all__,
    lld_ucli_ucli__int_dis_all__,
    lld_ucli_ucli__int_inj_all__,
    lld_ucli_ucli__int_stress__,
    lld_ucli_ucli__int_inj_reg__,
    lld_ucli_ucli__int_cb__,
    lld_ucli_ucli__intx_svc__,
    lld_ucli_ucli__msi_svc__,
    lld_ucli_ucli__msix_svc__,
    // displaying or configuring internal logs or data
    lld_ucli_ucli__lgset__,
    lld_ucli_ucli__dma_log__,
    lld_ucli_ucli__dump__,
    lld_ucli_ucli__dr_dump__,
    // diagnostic commands
    lld_ucli_ucli__cfg_diags__,
    lld_ucli_ucli__wl__,
    lld_ucli_ucli__wb__,
    lld_ucli_ucli__wb_mcast__,
    lld_ucli_ucli__rb__,
    lld_ucli_ucli__ilist__,
    lld_ucli_ucli__ilist2__,
    lld_ucli_ucli__mem_test__,
    lld_ucli_ucli__int_test__,
    lld_ucli_ucli__holetest__,
    lld_ucli_ucli__sysex__,
    lld_ucli_ucli__reg_test__,
    lld_ucli_ucli__tm_ind_test__,
    lld_ucli_ucli__tx_pkt__,
    lld_ucli_ucli__diag__,
    // miscellaneous commands
    lld_ucli_ucli__mem_info__,
    lld_ucli_ucli__dma_start__,
    lld_ucli_ucli__dma_service__,
    lld_ucli_ucli__efuse__,
    lld_ucli_ucli__cfg_diff__,
    lld_ucli_ucli__cfg_load__,
    lld_ucli_ucli__cfg_verify__,
    lld_ucli_ucli__find_tm_mems__,
    lld_ucli_ucli__test_tm_mems__,
    lld_ucli_ucli__global_ts_enable__,
    lld_ucli_ucli__global_ts_get__,
    lld_ucli_ucli__global_tm_set__,
    lld_ucli_ucli__eos__,
    lld_ucli_ucli__eosv__,
    lld_ucli_ucli__reg_holes__,
    lld_ucli_ucli__mem_holes__,
    lld_ucli_ucli__core_clk_set__,
    lld_ucli_ucli__core_clk_get__,
    lld_ucli_ucli__pps_clk_set__,
    lld_ucli_ucli__pps_clk_get__,
    lld_ucli_ucli__tm_clk_set__,
    lld_ucli_ucli__tm_clk_get__,
    lld_ucli_ucli__csr2jtag__,
    lld_ucli_ucli__csr2jtag_suite__,
    lld_ucli_ucli__pcie_fifo_dump__,
    lld_ucli_ucli__pcie_fifo_clear__,
    NULL};

static ucli_module_t lld_ucli_module__ = {
    "lld_ucli",
    NULL,
    lld_ucli_ucli_handlers__,
    NULL,
    NULL,
};

ucli_node_t *lldlib_ucli_node_create(void) {
  ucli_node_t *n;
  reg_set_instream(stdin);
  reg_set_outstream(stdout);
  ucli_module_init(&lld_ucli_module__);
  n = ucli_node_create("lld", NULL, &lld_ucli_module__);
  ucli_node_subnode_add(n, ucli_module_log_node_create("lld"));
  lldlib_gpio_ucli_node_create(n);
  return n;
}

#else
void *lldlib_ucli_node_create(void) { return NULL; }
#endif
