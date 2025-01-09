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
 * @file pipe_mgr_tof2_cfg.c
 * @date
 *
 * Definitions for Tof2 device configuration database
 */
#include "pipe_mgr_tof2_cfg.h"

#define TOF2_NUM_PIPELINES 4
#define TOF2_NUM_LOGICAL_TABLES 16
#define TOF2_NUM_STAGES 20

#define TOF2_NUM_MAC_BLKS 65

#define TOF2_NUM_SRAM_ROWS 8
#define TOF2_NUM_SRAM_COLS 12
#define TOF2_NUM_SRAM_UNITS 96
#define TOF2_NUM_TCAM_ROWS 12
#define TOF2_NUM_TCAM_COLS 2
#define TOF2_NUM_TCAM_UNITS (TOF2_NUM_TCAM_ROWS * TOF2_NUM_TCAM_COLS)
#define TOF2_NUM_MAP_RAM_ROWS 8
#define TOF2_NUM_MAP_RAM_COLS 6
#define TOF2_NUM_MAP_RAM_UNITS (TOF2_NUM_MAP_RAM_COLS * TOF2_NUM_MAP_RAM_ROWS)
#define TOF2_NUM_METER_ALUS 4
#define TOF2_GRESS_COUNT 1
static bool tof2_is_pipe_addr(uint64_t address) {
  return (address >> 41) == 1u;
}
static bool tof2_is_pcie_pipe_addr(uint32_t address) { return address >> 26; }
static uint8_t tof2_get_pipe_id_from_pipe_addr(uint64_t pipe_addr) {
  pipe_physical_addr_t addr;
  addr.addr = pipe_addr;
  return addr.tof2.pipe_id;
}
static uint8_t tof2_get_stage_id_from_pipe_addr(uint64_t pipe_addr) {
  pipe_physical_addr_t addr;
  addr.addr = pipe_addr;
  return addr.tof2.pipe_stage;
}
static uint8_t tof2_get_row_from_pipe_addr(uint64_t pipe_addr) {
  pipe_physical_addr_t addr;
  addr.addr = pipe_addr;
  return addr.tof2.mem_row;
}
static uint8_t tof2_get_col_from_pipe_addr(uint64_t pipe_addr) {
  pipe_physical_addr_t addr;
  addr.addr = pipe_addr;
  return addr.tof2.mem_col;
}
static pipe_mem_type_t tof2_get_mem_type_from_pipe_addr(uint64_t pipe_addr) {
  pipe_physical_addr_t addr;
  addr.addr = pipe_addr;
  return addr.tof2.mem_type;
}
static uint16_t tof2_get_mem_addr_from_pipe_addr(uint64_t pipe_addr) {
  pipe_physical_addr_t addr;
  addr.addr = pipe_addr;
  return addr.tof2.mem_address;
}
static pipe_ring_addr_type_e tof2_get_addr_type_from_pipe_addr(
    uint64_t pipe_addr) {
  pipe_physical_addr_t addr;
  addr.addr = pipe_addr;
  return addr.tof2.pipe_ring_addr_type;
}
static uint8_t tof2_memid_get_row(mem_id_t mem_id, pipe_mem_type_t mem_type) {
  switch (mem_type) {
    case pipe_mem_type_tcam:
      return ((mem_id % 12) & 0xF);
      break;
    case pipe_mem_type_unit_ram:
    case pipe_mem_type_shadow_reg:
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

static uint8_t tof2_memid_get_col(mem_id_t mem_id, pipe_mem_type_t mem_type) {
  switch (mem_type) {
    case pipe_mem_type_tcam:
      return ((mem_id / 12) & 0xF);
      break;
    case pipe_mem_type_unit_ram:
    case pipe_mem_type_shadow_reg:
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
static mem_id_t tof2_memunit_get_memid(dev_stage_t stage,
                                       uint8_t col,
                                       uint8_t row,
                                       pipe_mem_type_t mem_type) {
  (void)stage;
  switch (mem_type) {
    case pipe_mem_type_tcam:
      return (col * 12) + row;
      break;
    case pipe_mem_type_unit_ram:
    case pipe_mem_type_shadow_reg:
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
static pipe_tbl_dir_t tof2_get_gress(uint64_t address) {
  (void)address;
  /*Return 0 tofino1/2/3 does not care about gress in phy_address
   */
  return BF_TBL_DIR_INGRESS;
}
static mem_id_t tof2_get_mem_id_from_pipe_addr(uint64_t pipe_addr) {
  pipe_physical_addr_t addr;
  addr.addr = pipe_addr;
  return tof2_memunit_get_memid(
      0, addr.tof2.mem_col, addr.tof2.mem_row, addr.tof2.mem_type);
}
static uint64_t tof2_set_pipe_id_in_pipe_addr(uint64_t pipe_addr,
                                              uint8_t pipe_id) {
  pipe_physical_addr_t x;
  x.addr = pipe_addr;
  x.tof2.pipe_id = pipe_id;
  x.tof2.pipe_always_1 = 1;
  return x.addr;
}
static uint32_t tof2_dir_addr_set_pipe_type(uint32_t addr) {
  return addr | (1 << 26);
}
static uint32_t tof2_dir_addr_set_pipe_id(uint32_t addr, uint32_t pipe) {
  PIPE_MGR_DBGCHK(!(pipe >> 2)); /* Must be a 2 bit pipe id. */
  return (addr & ~(3u << 24)) | (pipe << 24);
}
static uint32_t tof2_dir_addr_get_pipe_id(uint32_t addr) {
  return (addr >> 24) & 0x3;
}
static uint32_t tof2_pcie_pipe_addr_set_stage(uint32_t addr,
                                              dev_stage_t stage) {
  /* Bit 26 must be set for pipe addresses. */
  if (addr & (1u << 26)) {
    /* Bits 23..19 hold the stage id */
    return (addr & ~0xF80000) | ((stage & 0x1F) << 19);
  } else {
    PIPE_MGR_DBGCHK(0);
    return ~0;
  }
}
static uint32_t tof2_pcie_pipe_addr_get_stage(uint32_t addr) {
  /* Bit 26 must be set for pipe addresses. */
  if (addr & (1u << 26)) {
    /* Bits 23..19 hold the stage id */
    return (addr >> 19) & 0x1F;
  } else {
    PIPE_MGR_DBGCHK(0);
    return ~0;
  }
}
static uint64_t tof2_pcie_pipe_addr_to_full_addr(uint32_t addr) {
  /* Bit 26 is the pipe type and must be set.
   * Bits 25-24 are the pipe id.
   * Bits 23-19 are the stage id.
   * Bits 18-0 are the register address. */
  pipe_physical_addr_t x;
  x.addr = 0;
  x.tof2.pipe_always_1 = 1;
  x.tof2.pipe_id = (addr >> 24) & 0x3;
  x.tof2.pipe_stage = (addr >> 19) & 0x1F;
  x.tof2.mem_type = addr_type_register;
  x.addr |= addr & 0x7FFFF;
  return x.addr;
}
/* This function is similar to the function below, with the difference
 * being, this forms the physical address of the given location, and not
 * the "full" address which is ready to be used.
 */
static uint64_t tof2_get_phy_addr(mem_id_t mem_id,
                                  uint32_t mem_address,
                                  pipe_mem_type_t mem_type) {
  pipe_physical_addr_t addr;

  addr.addr = 0;

  addr.tof2.mem_address = mem_address;
  addr.tof2.mem_col = tof2_memid_get_col(mem_id, mem_type);
  addr.tof2.mem_row = tof2_memid_get_row(mem_id, mem_type);
  addr.tof2.mem_type = mem_type;
  addr.tof2.pipe_ring_addr_type = addr_type_memdata;

  return addr.addr;
}
static uint64_t tof2_get_physical_addr(uint8_t gress,
                                       uint8_t pipe_id,
                                       uint8_t stage_id,
                                       mem_id_t mem_id,
                                       uint32_t mem_address,
                                       pipe_mem_type_t mem_type) {
  pipe_physical_addr_t instr;
  (void)gress;
  instr.addr = tof2_get_phy_addr(mem_id, mem_address, mem_type);

  instr.tof2.pipe_stage = stage_id;
  instr.tof2.pipe_id = pipe_id;
  instr.tof2.pipe_always_1 = 1;

  return instr.addr;
}

static bool tof2_sram_row_valid(uint8_t row) {
  return row < TOF2_NUM_SRAM_ROWS;
}
static bool tof2_sram_col_valid(uint8_t col) {
  return col >= 2 && col < TOF2_NUM_SRAM_COLS;
}
static bool tof2_tcam_row_valid(uint8_t row) { return row < 12; }
static bool tof2_tcam_col_valid(uint8_t col) { return col < 2; }

static mem_id_t tof2_get_map_ram_from_unit_ram(mem_id_t unit_ram_mem_id) {
  uint8_t col = tof2_memid_get_col(unit_ram_mem_id, pipe_mem_type_unit_ram);
  uint8_t row = tof2_memid_get_row(unit_ram_mem_id, pipe_mem_type_unit_ram);

  if (col < 6 || row > 7) {
    LOG_ERROR("ERROR:MAP_RAM ROW %d COL %d INVALID mem_id %d",
              row,
              col,
              unit_ram_mem_id);
    return ~0;
  }

  return 6 * row + (col - 6);
}

static mem_id_t tof2_get_unit_ram_from_map_ram(mem_id_t map_ram_mem_id) {
  uint8_t col = tof2_memid_get_col(map_ram_mem_id, pipe_mem_type_map_ram);
  uint8_t row = tof2_memid_get_row(map_ram_mem_id, pipe_mem_type_map_ram);
  return 12 * row + col;
}

static bf_dev_pipe_t tof2_dev_port_to_pipe(bf_dev_port_t port) {
  return (port >> 7) & 3;
}
static bf_dev_port_t tof2_dev_port_to_local_port(bf_dev_port_t port) {
  return port & 0x7F;
}
static bf_dev_port_t tof2_make_dev_port(bf_dev_pipe_t pipe, int port) {
  return (pipe << 7) | port;
}
static bool tof2_dev_port_validate(bf_dev_port_t port) {
  bf_dev_pipe_t pipe = tof2_dev_port_to_pipe(port);
  bf_dev_port_t local_port = tof2_dev_port_to_local_port(port);
  /* Pipe number is in range, local port number is in range, and there are no
   * extra bits set. */
  return pipe < TOF2_NUM_PIPELINES && local_port < TOF2_NUM_PORTS_PER_PIPE &&
         !(port & ~0x1FF);
}

static int tof2_dev_port_to_bit_idx(bf_dev_port_t port) {
  return ((TOF2_NUM_PORTS_PER_PIPE * tof2_dev_port_to_pipe(port)) +
          tof2_dev_port_to_local_port(port));
}

static bf_dev_port_t tof2_bit_idx_to_dev_port(int bit_idx) {
  return (tof2_make_dev_port(bit_idx / TOF2_NUM_PORTS_PER_PIPE,
                             bit_idx % TOF2_NUM_PORTS_PER_PIPE));
}

rmt_dev_cfg_t tof2_cfg = {
    .num_pipelines = TOF2_NUM_PIPELINES,
    .num_prsrs = TOF2_NUM_PARSERS,
    .num_stages = TOF2_NUM_STAGES,
    .num_ports = TOF2_NUM_PORTS_PER_PIPE,
    .num_mac_blks = TOF2_NUM_MAC_BLKS,
    .num_chan_per_port = TOF2_NUM_CHN_PER_IPB,
    .p0_width = 4, /* Four 32 bit registers make a 128 bit entry. */
    .num_dflt_reg = 9,
    .num_gress = TOF2_GRESS_COUNT,
    .stage_cfg =
        {
            .num_logical_tables = TOF2_NUM_LOGICAL_TABLES,
            .sram_unit_width = TOF2_SRAM_UNIT_WIDTH,
            .sram_unit_depth = TOF2_SRAM_UNIT_DEPTH,
            .num_sram_units = TOF2_NUM_SRAM_UNITS,
            .num_sram_rows = TOF2_NUM_SRAM_ROWS,
            .num_sram_cols = TOF2_NUM_SRAM_COLS,

            .tcam_unit_width = TOF2_TCAM_UNIT_WIDTH,
            .tcam_unit_depth = TOF2_TCAM_UNIT_DEPTH,
            .num_tcam_units = TOF2_NUM_TCAM_UNITS,
            .num_tcam_rows = TOF2_NUM_TCAM_ROWS,
            .num_tcam_cols = TOF2_NUM_TCAM_COLS,

            .map_ram_unit_width = TOF2_MAP_RAM_UNIT_WIDTH,
            .map_ram_unit_depth = TOF2_MAP_RAM_UNIT_DEPTH,
            .num_map_ram_units = TOF2_NUM_MAP_RAM_UNITS,
            .num_map_ram_rows = TOF2_NUM_MAP_RAM_ROWS,
            .num_map_ram_cols = TOF2_NUM_MAP_RAM_COLS,
            .num_meter_alus = TOF2_NUM_METER_ALUS,
        },
    .is_pipe_addr = tof2_is_pipe_addr,
    .is_pcie_pipe_addr = tof2_is_pcie_pipe_addr,
    .pipe_id_from_addr = tof2_get_pipe_id_from_pipe_addr,
    .stage_id_from_addr = tof2_get_stage_id_from_pipe_addr,
    .row_from_addr = tof2_get_row_from_pipe_addr,
    .col_from_addr = tof2_get_col_from_pipe_addr,
    .addr_type_from_addr = tof2_get_addr_type_from_pipe_addr,
    .mem_type_from_addr = tof2_get_mem_type_from_pipe_addr,
    .mem_addr_from_addr = tof2_get_mem_addr_from_pipe_addr,
    .mem_id_from_addr = tof2_get_mem_id_from_pipe_addr,
    .mem_id_to_row = tof2_memid_get_row,
    .mem_id_to_col = tof2_memid_get_col,
    .mem_id_from_col_row = tof2_memunit_get_memid,
    .map_ram_from_unit_ram = tof2_get_map_ram_from_unit_ram,
    .unit_ram_from_map_ram = tof2_get_unit_ram_from_map_ram,
    .set_pipe_id_in_addr = tof2_set_pipe_id_in_pipe_addr,
    .get_full_phy_addr = tof2_get_physical_addr,
    .get_relative_phy_addr = tof2_get_phy_addr,
    .dir_addr_set_pipe_type = tof2_dir_addr_set_pipe_type,
    .dir_addr_set_pipe_id = tof2_dir_addr_set_pipe_id,
    .dir_addr_get_pipe_id = tof2_dir_addr_get_pipe_id,
    .pcie_pipe_addr_set_stage = tof2_pcie_pipe_addr_set_stage,
    .pcie_pipe_addr_get_stage = tof2_pcie_pipe_addr_get_stage,
    .pcie_pipe_addr_to_full_addr = tof2_pcie_pipe_addr_to_full_addr,
    .sram_row_valid = tof2_sram_row_valid,
    .sram_col_valid = tof2_sram_col_valid,
    .tcam_row_valid = tof2_tcam_row_valid,
    .tcam_col_valid = tof2_tcam_col_valid,
    .dev_port_to_pipe = tof2_dev_port_to_pipe,
    .dev_port_to_local_port = tof2_dev_port_to_local_port,
    .make_dev_port = tof2_make_dev_port,
    .dev_port_validate = tof2_dev_port_validate,
    .dev_port_to_bit_idx = tof2_dev_port_to_bit_idx,
    .bit_idx_to_dev_port = tof2_bit_idx_to_dev_port,
    .gress_from_addr = tof2_get_gress};
