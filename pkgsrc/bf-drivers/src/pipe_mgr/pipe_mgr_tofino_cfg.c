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
 * @file pipe_mgr_tofino_cfg.c
 * @date
 *
 * Definitions for Tofino device configuration database
 */
#include "pipe_mgr_tofino_cfg.h"
#include "lld/lld_inst_list_fmt.h"

#define TOF_NUM_PIPELINES 4
#define TOF_NUM_LOGICAL_TABLES 16
#define TOF_NUM_STAGES 12

#define TOF_NUM_MAC_BLKS 65

#define TOF_NUM_SRAM_ROWS 8
#define TOF_NUM_SRAM_COLS 12
#define TOF_NUM_SRAM_UNITS (TOF_NUM_SRAM_ROWS * TOF_NUM_SRAM_COLS)
#define TOF_NUM_TCAM_ROWS 12
#define TOF_NUM_TCAM_COLS 2
#define TOF_NUM_TCAM_UNITS (TOF_NUM_TCAM_ROWS * TOF_NUM_TCAM_COLS)
#define TOF_NUM_MAP_RAM_ROWS 8
#define TOF_NUM_MAP_RAM_COLS 6
#define TOF_NUM_MAP_RAM_UNITS (TOF_NUM_MAP_RAM_COLS * TOF_NUM_MAP_RAM_ROWS)
#define TOF_NUM_METER_ALUS 4
#define TOF_GRESS_COUNT 1
/* Tofino device related configuration */

static bool tof_is_pipe_addr(uint64_t address) { return (address >> 40) == 2; }
static bool tof_is_pcie_pipe_addr(uint32_t address) { return address >> 25; }
static pipe_ring_addr_type_e tof_get_addr_type_from_pipe_addr(
    uint64_t pipe_addr) {
  pipe_physical_addr_t addr;
  addr.addr = pipe_addr;
  return addr.tof.pipe_ring_addr_type;
}
static pipe_tbl_dir_t tof_get_gress(uint64_t address) {
  (void)address;
  /*Return 0 tofino1/2/3 does not care about gress in phy_address.
   */
  return BF_TBL_DIR_INGRESS;
}
static uint8_t tof_memid_get_row(mem_id_t mem_id, pipe_mem_type_t mem_type) {
  switch (mem_type) {
    case pipe_mem_type_tcam:
      return ((mem_id % 12) & 0xF);
      break;
    case pipe_mem_type_unit_ram:
      return ((mem_id / 12) & 0xF);
      break;
    case pipe_mem_type_map_ram:
      return ((mem_id / 6) & 0xF);
      break;
    default:
      return (mem_id & 0xF);
      break;
  }
  return 0;
}

static uint8_t tof_memid_get_col(mem_id_t mem_id, pipe_mem_type_t mem_type) {
  switch (mem_type) {
    case pipe_mem_type_tcam:
      return ((mem_id / 12) & 0xF);
      break;
    case pipe_mem_type_unit_ram:
      return ((mem_id % 12) & 0xF);
      break;
    case pipe_mem_type_map_ram:
      return ((6 + (mem_id % 6)) & 0xF);
      break;
    default:
      return ((mem_id >> 4) & 0xF);
      break;
  }
  return 0;
}
static mem_id_t tof_memunit_get_memid(dev_stage_t stage,
                                      uint8_t col,
                                      uint8_t row,
                                      pipe_mem_type_t mem_type) {
  (void)stage;
  switch (mem_type) {
    case pipe_mem_type_tcam:
      return (col * 12) + row;
      break;
    case pipe_mem_type_unit_ram:
      return (row * 12) + col;
      break;
    case pipe_mem_type_map_ram:
      return (row * 6) + col;
      break;
    default:
      return (col * 16) + row;
      break;
  }
  return 0;
}
mem_id_t tof_get_mem_id_from_pipe_addr(uint64_t pipe_addr) {
  pipe_physical_addr_t addr;
  addr.addr = pipe_addr;
  return tof_memunit_get_memid(
      0, addr.tof.mem_col, addr.tof.mem_row, addr.tof.mem_type);
}
static uint64_t tof_set_pipe_id_in_pipe_addr(uint64_t pipe_addr,
                                             uint8_t pipe_id) {
  pipe_physical_addr_t x;
  x.addr = pipe_addr;
  x.tof.pipe_id_39_37 = pipe_id;
  x.tof.pipe_41_40 = 2;
  return x.addr;
}
static uint32_t tof_dir_addr_set_pipe_type(uint32_t addr) {
  return addr | (1 << 25);
}
static uint32_t tof_dir_addr_set_pipe_id(uint32_t addr, uint32_t pipe) {
  return (addr & ~(3u << 23)) | (pipe << 23);
}
static uint32_t tof_dir_addr_get_pipe_id(uint32_t addr) {
  return (addr >> 23) & 0x3;
}
static uint32_t tof_pcie_pipe_addr_get_stage(uint32_t addr) {
  /* Bit 25 must be set for pipe addresses. */
  if (addr & (1u << 25)) {
    /* Bits 22..19 hold the stage id */
    return (addr >> 19) & 0xF;
  } else {
    PIPE_MGR_DBGCHK(0);
    return ~0;
  }
}
static uint32_t tof_pcie_pipe_addr_set_stage(uint32_t addr, dev_stage_t stage) {
  /* Bit 25 must be set for pipe addresses. */
  if (addr & (1u << 25)) {
    /* Bits 22..19 hold the stage id */
    return (addr & ~0x780000) | ((stage & 0xF) << 19);
  } else {
    PIPE_MGR_DBGCHK(0);
    return ~0;
  }
}
static uint64_t tof_pcie_pipe_addr_to_full_addr(uint32_t addr) {
  /* Bit 25 is the pipe type and must be set.
   * Bits 24-23 are the pipe id.
   * Bits 22-19 are the stage id.
   * Bits 18-0 are the register address. */
  pipe_physical_addr_t x;
  x.addr = 0;
  x.tof.pipe_41_40 = 2;
  x.tof.pipe_id_39_37 = (addr >> 23) & 0x3;
  x.tof.pipe_element_36_33 = (addr >> 19) & 0xF;
  x.addr |= addr & 0x7FFFF;
  return x.addr;
}
/* This function is similar to the function below, with the difference
 * being, this forms the physical address of the given location, and not
 * the "full" address which is ready to be used.
 */
static uint64_t tof_get_phy_addr(mem_id_t mem_id,
                                 uint32_t mem_address,
                                 pipe_mem_type_t mem_type) {
  pipe_physical_addr_t addr;

  addr.addr = 0;

  addr.tof.mem_address = mem_address;
  addr.tof.mem_col = tof_memid_get_col(mem_id, mem_type);
  addr.tof.mem_row = tof_memid_get_row(mem_id, mem_type);
  addr.tof.mem_type = mem_type;
  addr.tof.pipe_ring_addr_type = addr_type_memdata;

  return addr.addr;
}
static uint64_t tof_get_physical_addr(uint8_t gress,
                                      uint8_t pipe_id,
                                      dev_stage_t stage_id,
                                      mem_id_t mem_id,
                                      uint32_t mem_address,
                                      pipe_mem_type_t mem_type) {
  pipe_physical_addr_t instr;
  (void)gress;
  instr.addr = tof_get_phy_addr(mem_id, mem_address, mem_type);

  instr.tof.pipe_element_36_33 = stage_id;
  instr.tof.pipe_id_39_37 = pipe_id;
  /* 44 = 32 + 4 (to get 128 bit address) + 8 (bits for stage) */
  instr.tof.pipe_41_40 = PIPE_TOP_LEVEL_PIPES_ADDRESS >> 44;

  return instr.addr;
}

static bool tof_sram_row_valid(uint8_t row) { return row < TOF_NUM_SRAM_ROWS; }
static bool tof_sram_col_valid(uint8_t col) {
  return col >= 2 && col < TOF_NUM_SRAM_COLS;
}
static bool tof_tcam_row_valid(uint8_t row) { return row < 12; }
static bool tof_tcam_col_valid(uint8_t col) { return col < 2; }

static mem_id_t tof_get_map_ram_from_unit_ram(mem_id_t unit_ram_mem_id) {
  uint8_t col = tof_memid_get_col(unit_ram_mem_id, pipe_mem_type_unit_ram);
  uint8_t row = tof_memid_get_row(unit_ram_mem_id, pipe_mem_type_unit_ram);
  if (col < 6) return ~0;
  return 6 * row + (col - 6);
}

static mem_id_t tof_get_unit_ram_from_map_ram(mem_id_t map_ram_mem_id) {
  uint8_t col = tof_memid_get_col(map_ram_mem_id, pipe_mem_type_map_ram);
  uint8_t row = tof_memid_get_row(map_ram_mem_id, pipe_mem_type_map_ram);
  return 12 * row + col;
}

static bf_dev_pipe_t tof_dev_port_to_pipe(bf_dev_port_t port) {
  return (port >> 7) & 3;
}
static bf_dev_port_t tof_dev_port_to_local_port(bf_dev_port_t port) {
  return port & 0x7F;
}
static bf_dev_port_t tof_make_dev_port(bf_dev_pipe_t pipe, int port) {
  return (pipe << 7) | port;
}
static bool tof_dev_port_validate(bf_dev_port_t port) {
  bf_dev_pipe_t pipe = tof_dev_port_to_pipe(port);
  bf_dev_port_t local_port = tof_dev_port_to_local_port(port);
  /* Pipe number is in range, local port number is in range, and there are no
   * extra bits set. */
  return pipe < TOF_NUM_PIPELINES && local_port < TOF_NUM_PORTS_PER_PIPE &&
         !(port & ~0x1FF);
}

static int tof_dev_port_to_bit_idx(bf_dev_port_t port) {
  return ((TOF_NUM_PORTS_PER_PIPE * tof_dev_port_to_pipe(port)) +
          tof_dev_port_to_local_port(port));
}

static bf_dev_port_t tof_bit_idx_to_dev_port(int bit_idx) {
  return (tof_make_dev_port(bit_idx / TOF_NUM_PORTS_PER_PIPE,
                            bit_idx % TOF_NUM_PORTS_PER_PIPE));
}

rmt_dev_cfg_t tofino_cfg = {
    .num_pipelines = TOF_NUM_PIPELINES,
    .num_gress = TOF_GRESS_COUNT,
    .num_prsrs = TOF_NUM_PARSERS,
    .num_stages = TOF_NUM_STAGES,
    .num_ports = TOF_NUM_PORTS_PER_PIPE,
    .num_mac_blks = TOF_NUM_MAC_BLKS,
    .num_chan_per_port = TOF_NUM_CHN_PER_PORT,
    .p0_width = 2, /* Two 32 bit registers make a 64 bit entry. */
    .num_dflt_reg = 7,
    .stage_cfg =
        {
            .num_logical_tables = TOF_NUM_LOGICAL_TABLES,
            .sram_unit_width = TOF_SRAM_UNIT_WIDTH,
            .sram_unit_depth = TOF_SRAM_UNIT_DEPTH,
            .num_sram_units = TOF_NUM_SRAM_UNITS,
            .num_sram_rows = TOF_NUM_SRAM_ROWS,
            .num_sram_cols = TOF_NUM_SRAM_COLS,

            .tcam_unit_width = TOF_TCAM_UNIT_WIDTH,
            .tcam_unit_depth = TOF_TCAM_UNIT_DEPTH,
            .num_tcam_units = TOF_NUM_TCAM_UNITS,
            .num_tcam_rows = TOF_NUM_TCAM_ROWS,
            .num_tcam_cols = TOF_NUM_TCAM_COLS,

            .map_ram_unit_width = TOF_MAP_RAM_UNIT_WIDTH,
            .map_ram_unit_depth = TOF_MAP_RAM_UNIT_DEPTH,
            .num_map_ram_units = TOF_NUM_MAP_RAM_UNITS,
            .num_map_ram_rows = TOF_NUM_MAP_RAM_ROWS,
            .num_map_ram_cols = TOF_NUM_MAP_RAM_COLS,
            .num_meter_alus = TOF_NUM_METER_ALUS,
        },
    .is_pipe_addr = tof_is_pipe_addr,
    .is_pcie_pipe_addr = tof_is_pcie_pipe_addr,
    .pipe_id_from_addr = tf1_get_pipe_id_from_pipe_addr,
    .stage_id_from_addr = tf1_get_stage_id_from_pipe_addr,
    .row_from_addr = tf1_get_row_from_pipe_addr,
    .col_from_addr = tf1_get_col_from_pipe_addr,
    .addr_type_from_addr = tof_get_addr_type_from_pipe_addr,
    .mem_type_from_addr = tf1_get_mem_type_from_pipe_addr,
    .mem_addr_from_addr = tf1_get_mem_addr_from_pipe_addr,
    .mem_id_from_addr = tof_get_mem_id_from_pipe_addr,
    .mem_id_to_row = tof_memid_get_row,
    .mem_id_to_col = tof_memid_get_col,
    .mem_id_from_col_row = tof_memunit_get_memid,
    .map_ram_from_unit_ram = tof_get_map_ram_from_unit_ram,
    .unit_ram_from_map_ram = tof_get_unit_ram_from_map_ram,
    .set_pipe_id_in_addr = tof_set_pipe_id_in_pipe_addr,
    .get_full_phy_addr = tof_get_physical_addr,
    .get_relative_phy_addr = tof_get_phy_addr,
    .dir_addr_set_pipe_type = tof_dir_addr_set_pipe_type,
    .dir_addr_set_pipe_id = tof_dir_addr_set_pipe_id,
    .dir_addr_get_pipe_id = tof_dir_addr_get_pipe_id,
    .pcie_pipe_addr_set_stage = tof_pcie_pipe_addr_set_stage,
    .pcie_pipe_addr_get_stage = tof_pcie_pipe_addr_get_stage,
    .pcie_pipe_addr_to_full_addr = tof_pcie_pipe_addr_to_full_addr,
    .sram_row_valid = tof_sram_row_valid,
    .sram_col_valid = tof_sram_col_valid,
    .tcam_row_valid = tof_tcam_row_valid,
    .tcam_col_valid = tof_tcam_col_valid,
    .dev_port_to_pipe = tof_dev_port_to_pipe,
    .dev_port_to_local_port = tof_dev_port_to_local_port,
    .make_dev_port = tof_make_dev_port,
    .dev_port_validate = tof_dev_port_validate,
    .dev_port_to_bit_idx = tof_dev_port_to_bit_idx,
    .bit_idx_to_dev_port = tof_bit_idx_to_dev_port,
    .gress_from_addr = tof_get_gress,
};
