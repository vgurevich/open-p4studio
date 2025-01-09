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


#include <stddef.h>
#include <stdint.h>
#include <dvm/bf_drv_intf.h>
#include <lld/bf_dma_if.h>
#include <lld/lld_dr_if.h>
#include <lld/lld_reg_if.h>
#include <lld/lld_tof_addr_conversion.h>
#include <lld/lld_tof2_addr_conversion.h>
#include <lld/lld_tof3_addr_conversion.h>
#include <lld/lld_err.h>
#include <lld/lld_sku.h>
#include <tofino_regs/tofino.h>
#include <tof2_regs/tof2_reg_drv.h>
#include <tof3_regs/tof3_reg_drv.h>
#include <tof3_regs/tof3_mem_drv.h>
#include <tof3_regs/tof3_mem_addr.h>
#include "mc_mgr.h"
#include "mc_mgr_int.h"
#include "mc_mgr_reg.h"

extern pipe_status_t pipe_mgr_tof_mc_mgid_grp_addr_get(int mgid_grp,
                                                       uint32_t *tbl0_addr,
                                                       uint32_t *tbl1_addr);
extern pipe_status_t pipe_mgr_tof2_mc_mgid_grp_addr_get(int mgid_grp,
                                                        uint32_t *tbl0_addr,
                                                        uint32_t *tbl1_addr);

bf_status_t mc_mgr_get_tbl_ver_reg(bf_dev_id_t dev,
                                   bf_dev_pipe_t pipe,
                                   int *ver) {
  if (MC_MGR_INVALID_DEV(dev)) {
    LOG_ERROR("Read tbl ver fails, invalid device %d.%d at %s:%d",
              dev,
              pipe,
              __func__,
              __LINE__);
    return BF_INVALID_ARG;
  }
  if (pipe >= mc_mgr_ctx_num_max_pipes(dev)) {
    LOG_ERROR("Read tbl ver fails, invalid pipe %d.%d at %s:%d",
              dev,
              pipe,
              __func__,
              __LINE__);
    return BF_INVALID_ARG;
  }

  bf_dev_pipe_t phy_pipe = pipe;
  bf_dev_pipe_t log_pipe = pipe;
  uint32_t num_pipe = 0;
  lld_sku_map_phy_pipe_id_to_pipe_id(dev, phy_pipe, &log_pipe);
  lld_sku_get_num_active_pipes(dev, &num_pipe);
  if (num_pipe <= log_pipe) return BF_HW_COMM_FAIL;

  uint32_t addr = 0;
  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      addr = offsetof(Tofino, pipes[pipe].deparser.hdr.hir.ingr.yid_tbl);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      addr = offsetof(
          tof2_reg,
          pipes[pipe].pardereg.dprsrreg.dprsrreg.ho_i[0].hir.meta.pre_version);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      addr = offsetof(
          tof3_reg,
          pipes[pipe].pardereg.dprsrreg.dprsrreg.ho_i[0].hir.meta.pre_version);
      break;
    default:
      MC_MGR_DBGCHK(0);
  }

  uint32_t val = 0;
  int x = lld_read_register(dev, addr, &val);
  if (x) {
    LOG_ERROR("Read reg fails (%d) at %s:%d", x, __func__, __LINE__);
    *ver = 0;
    return BF_HW_COMM_FAIL;
  }
  *ver = val;

  return BF_SUCCESS;
}

bf_status_t mc_mgr_get_pvt_reg(bf_dev_id_t dev,
                               int phy_pipe,
                               int table,
                               int mgid_grp,
                               mc_mgr_pvt_entry_t *val) {
  uint32_t log_pipe = phy_pipe;
  uint32_t num_pipe = 0;
  lld_sku_map_phy_pipe_id_to_pipe_id(dev, phy_pipe, &log_pipe);
  lld_sku_get_num_active_pipes(dev, &num_pipe);
  if (num_pipe <= log_pipe) return BF_INVALID_ARG;

  uint32_t addr = 0;
  uint32_t reg_val = 0;
  uint64_t mem_addr = 0;
  switch (mc_mgr_ctx_dev_family(dev)) {
    uint32_t tbl0_addr, tbl1_addr;
    bf_status_t sts;
    case BF_DEV_FAMILY_TOFINO:
      sts = pipe_mgr_tof_mc_mgid_grp_addr_get(mgid_grp, &tbl0_addr, &tbl1_addr);
      if (sts) return sts;
      if (0 == table) {
        if (mgid_grp >=
            (int)tofino_pipes_deparser_hdr_him_hi_pv_table_tbl0_array_count) {
          return BF_INVALID_ARG;
        }
        addr = tbl0_addr;
      } else {
        if (mgid_grp >=
            (int)tofino_pipes_deparser_hdr_him_hi_pv_table_tbl1_array_count) {
          return BF_INVALID_ARG;
        }
        addr = tbl1_addr;
      }
      if (lld_read_register(dev, addr, &reg_val)) {
        LOG_ERROR("Read PVT reg fails at %s:%d", __func__, __LINE__);
        return BF_HW_COMM_FAIL;
      }
      val->d = reg_val;
      break;
    case BF_DEV_FAMILY_TOFINO2:
      sts =
          pipe_mgr_tof2_mc_mgid_grp_addr_get(mgid_grp, &tbl0_addr, &tbl1_addr);
      if (sts) return sts;
      addr = (0 == table) ? tbl0_addr : tbl1_addr;

      if (lld_read_register(dev, addr, &reg_val)) {
        LOG_ERROR("Read PVT reg fails at %s:%d", __func__, __LINE__);
        return BF_HW_COMM_FAIL;
      }
      val->d = reg_val;
      break;
    case BF_DEV_FAMILY_TOFINO3: {
      if (0 == table) {
        mem_addr =
            tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_pvt_table_mgid1_tbl(
                phy_pipe % BF_SUBDEV_PIPE_COUNT, mgid_grp);
      } else {
        mem_addr =
            tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_pvt_table_mgid2_tbl(
                phy_pipe % BF_SUBDEV_PIPE_COUNT, mgid_grp);
      }
      uint64_t data0 = 0, data1 = 0, mem_val_mgid_map = 0;
      uint8_t mem_val_mgid_bool = 0;
      int x = lld_ind_read(dev, mem_addr / 16, &data1, &data0);
      if (x) {
        LOG_ERROR("Read PVT MEM fails, indirect reg read (%d) at %s:%d",
                  x,
                  __func__,
                  __LINE__);
        return BF_HW_COMM_FAIL;
      }
      // convert hardware presentation into software presentation
      uint64_t pvt_mask = 0x1FF;
      uint64_t last_mgid = (data1 << 1) | (data0 >> 63);
      for (int i = 0; i < 7; i++) {
        uint64_t mask = pvt_mask << (9 * i);
        uint64_t mgidmap = ((data0 & mask) >> (9 * i)) & 0xFFllu;
        uint8_t has_400g = ((((data0 & mask) >> (9 * i)) >> 8) & 0x1) << i;
        mem_val_mgid_map |= mgidmap << (8 * i);
        mem_val_mgid_bool |= has_400g;
      }
      mem_val_mgid_map |= ((last_mgid & 0xFF) << (8 * 7));
      mem_val_mgid_bool |= ((last_mgid & 0x100) ? (0x80) : 0);
      val->d = mem_val_mgid_map;
      val->has_400g_map = mem_val_mgid_bool;
      break;
    }
    default:
      MC_MGR_DBGCHK(0);
      return BF_UNEXPECTED;
  }

#ifdef UTEST
  /* Pull the data out of our software shadow and return it since there is no
   * hardware (or sw model) to read in the unit test environment. */
  *val = mc_mgr_ctx_pvt_row(dev, mgid_grp);
#endif
  return BF_SUCCESS;
}

bf_status_t mc_mgr_get_ver_cnt_reg(bf_dev_id_t dev,
                                   bf_dev_pipe_t pipe,
                                   int ver,
                                   uint32_t *count) {
  if (MC_MGR_INVALID_DEV(dev)) {
    LOG_ERROR("Invalid device %d.%d at %s:%d", dev, pipe, __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  if (pipe >= mc_mgr_ctx_num_max_pipes(dev)) {
    LOG_ERROR("Invalid pipe %d.%d at %s:%d", dev, pipe, __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  if (0 != ver && 1 != ver) {
    LOG_ERROR("Invalid version %d at %s:%d", ver, __func__, __LINE__);
    return BF_INVALID_ARG;
  }

  uint32_t addr = 0;
  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      addr = offsetof(
          Tofino,
          device_select.tm_top.tm_pre_top.pre[pipe].table_ph_count[ver]);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      addr = offsetof(
          tof2_reg,
          device_select.tm_top.tm_pre_top.pre[pipe].table_ph_count[ver]);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      addr = offsetof(
          tof3_reg,
          device_select.tm_top.tm_pre_top.pre[pipe].table_ph_count[ver]);
      break;
    default:
      MC_MGR_DBGCHK(0);
  }

  uint32_t val = 0;
  int x = lld_read_register(dev, addr, &val);
  if (x) {
    LOG_ERROR("Read reg fails (%d) at %s:%d", x, __func__, __LINE__);
    *count = 0;
    return BF_HW_COMM_FAIL;
  }
  *count = val;

  return BF_SUCCESS;
}

bf_status_t mc_mgr_get_global_rid_reg(bf_dev_id_t dev, uint16_t *rid) {
  if (MC_MGR_INVALID_DEV(dev)) {
    LOG_ERROR("Invalid device %d at %s:%d", dev, __func__, __LINE__);
    return BF_INVALID_ARG;
  }

  uint32_t addr = 0;
  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      addr = offsetof(Tofino,
                      device_select.tm_top.tm_pre_top.pre_common.prune_rid);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      addr = offsetof(tof2_reg,
                      device_select.tm_top.tm_pre_top.pre_common.prune_rid);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      addr = offsetof(tof3_reg,
                      device_select.tm_top.tm_pre_top.pre_common.prune_rid);
      break;
    default:
      MC_MGR_DBGCHK(0);
  }

  uint32_t val = 0;
  int x = lld_read_register(dev, addr, &val);
  if (x) {
    LOG_ERROR("Read reg fails (%d) at %s:%d", x, __func__, __LINE__);
    *rid = 0;
    return BF_HW_COMM_FAIL;
  }
  *rid = val;
#ifdef UTEST
  /* Pull the data out of our software shadow and return it since there is no
   * hardware (or sw model) to read in the unit test environment. */
  *rid = mc_mgr_ctx_glb_rid(dev);
#endif

  return BF_SUCCESS;
}
bf_status_t mc_mgr_set_global_rid_reg(bf_dev_id_t dev, uint16_t rid) {
  if (MC_MGR_INVALID_DEV(dev)) {
    LOG_ERROR("Invalid device %d at %s:%d", dev, __func__, __LINE__);
    return BF_INVALID_ARG;
  }

  uint32_t addr = 0;
  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      addr = offsetof(Tofino,
                      device_select.tm_top.tm_pre_top.pre_common.prune_rid);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      addr = offsetof(tof2_reg,
                      device_select.tm_top.tm_pre_top.pre_common.prune_rid);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      addr = offsetof(tof3_reg,
                      device_select.tm_top.tm_pre_top.pre_common.prune_rid);
      break;
    default:
      MC_MGR_DBGCHK(0);
  }

  uint32_t val = rid;
  int x = mc_mgr_write_register(dev, addr, val);
  if (x) {
    LOG_ERROR("Read reg fails (%d) at %s:%d", x, __func__, __LINE__);
    return BF_HW_COMM_FAIL;
  }

  return BF_SUCCESS;
}

bf_status_t mc_mgr_get_rdm_blk_id_grp_reg(bf_dev_id_t dev,
                                          int index,
                                          uint32_t *id) {
  if (MC_MGR_INVALID_DEV(dev)) {
    LOG_ERROR("Invalid device %d at %s:%d", dev, __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  if (0 > index || 15 < index) {
    LOG_ERROR(
        "Invalid block group index %d at %s:%d", index, __func__, __LINE__);
    return BF_INVALID_ARG;
  }

  uint32_t addr = 0;
  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      addr = offsetof(
          Tofino,
          device_select.tm_top.tm_pre_top.pre_common.blk_id.blk_id_0_13);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      addr = offsetof(
          tof2_reg,
          device_select.tm_top.tm_pre_top.pre_common.blk_id.blk_id_0_16);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      addr = offsetof(
          tof3_reg,
          device_select.tm_top.tm_pre_top.pre_common.blk_id.blk_id_0_32);
      break;
    default:
      MC_MGR_DBGCHK(0);
  }
  addr += 4 * index;

  uint32_t val = 0;
  int ret = lld_read_register(dev, addr, &val);
  if (ret) {
    LOG_ERROR("Read reg fails (%d) at %s:%d", ret, __func__, __LINE__);
    return BF_HW_COMM_FAIL;
  }
  *id = val;
  return BF_SUCCESS;
}

bf_status_t mc_mgr_set_rdm_blk_id_grp_wrl(int sid, bf_dev_id_t dev, int grp) {
  if (0 > sid || MC_MGR_NUM_SESSIONS <= sid) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (0 > dev || MC_MGR_NUM_DEVICES <= dev) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  uint64_t hi = 0;
  uint64_t lo = mc_mgr_ctx_rdm_map(dev)->blk_ids[grp];
#if defined(EMU_2DIE_USING_SW_2DEV)
  if (dev == 1) {
    lo = mc_mgr_ctx_rdm_map(dev)->blk_ids_other_die[grp];
  }
#endif
  uint32_t daddr = 0;
  uint64_t addr = 0;
  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      daddr =
          tofino_device_select_tm_top_tm_pre_top_pre_common_blk_id_blk_id_0_13_address +
          4 * grp;
      addr = tof_dir_to_indir_dev_sel(daddr);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      daddr =
          tof2_reg_device_select_tm_top_tm_pre_top_pre_common_blk_id_blk_id_0_16_address +
          4 * grp;
      addr = tof2_dir_to_indir_dev_sel(daddr);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      daddr =
          tof3_reg_device_select_tm_top_tm_pre_top_pre_common_blk_id_blk_id_0_32_address +
          4 * grp;
      addr = tof3_dir_to_indir_dev_sel(daddr);
      break;
    default:
      MC_MGR_DBGCHK(0);
  }

  if (mc_mgr_drv_wrl_append(
          dev,
          (mc_mgr_ctx_num_subdevices(dev) > 1) ? 0 : MC_MGR_DRV_SUBDEV_ID_ALL,
          sid,
          16,
          addr << 4,
          hi,
          lo,
          __func__,
          __LINE__)) {
    LOG_ERROR("Failed to add BlkId update to WRL on dev %d", dev);
    MC_MGR_DBGCHK(0);
    return BF_NO_SYS_RESOURCES;
  }
  if (mc_mgr_ctx_num_subdevices(dev) > 1) {
    lo = mc_mgr_ctx_rdm_map(dev)->blk_ids_other_die[grp];
    if (mc_mgr_drv_wrl_append(
            dev, 1, sid, 16, addr << 4, hi, lo, __func__, __LINE__)) {
      LOG_ERROR("Failed to add BlkId update to WRL on dev %d", dev);
      MC_MGR_DBGCHK(0);
      return BF_NO_SYS_RESOURCES;
    }
  }

  return BF_SUCCESS;
}

bf_status_t mc_mgr_get_port_mask_reg(
    bf_dev_id_t dev, int ver, int port, uint32_t *blk, bool *val) {
  if (0 > dev || MC_MGR_NUM_DEVICES <= dev) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (0 != ver && 1 != ver) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (port < 0 ||
      port >= (int)(BF_SUBDEV_PORT_COUNT * mc_mgr_ctx_num_subdevices(dev))) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  int idx = port / 32;
  int shift = port % 32;
  uint32_t data = 0;
  uint32_t addr = 0;
  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      addr =
          ver *
              tofino_device_select_tm_top_tm_pre_top_pre_common_port_mask_array_element_size +
          tofino_device_select_tm_top_tm_pre_top_pre_common_port_mask_address +
          4 * idx;
      break;
    case BF_DEV_FAMILY_TOFINO2:
      addr =
          ver *
              tof2_reg_device_select_tm_top_tm_pre_top_pre_common_port_mask_array_element_size +
          tof2_reg_device_select_tm_top_tm_pre_top_pre_common_port_mask_address +
          4 * idx;
      break;
    case BF_DEV_FAMILY_TOFINO3:
      if (port < 0 || port >= (int)(MC_MGR_TOF3_PORT_COUNT *
                                    mc_mgr_ctx_num_subdevices(dev))) {
        MC_MGR_DBGCHK(0);
        return BF_INVALID_ARG;
      }
      addr =
          ver *
              tof3_reg_device_select_tm_top_tm_pre_top_pre_common_port_mask_array_element_size +
          tof3_reg_device_select_tm_top_tm_pre_top_pre_common_port_mask_address +
          4 * idx;
      break;
    default:
      MC_MGR_DBGCHK(0);
  }

  int x = lld_read_register(dev, addr, &data);
  if (x) {
    LOG_ERROR("Failed to read PortMask%d.%d (%d) from %s:%d",
              ver,
              idx,
              x,
              __func__,
              __LINE__);
    MC_MGR_DBGCHK(0);
    return BF_HW_COMM_FAIL;
  }
  if (blk) *blk = data;
  if (val) *val = (data >> shift) & 1;
#ifdef UTEST
  /* Pull the data out of our software shadow and return it since there is no
   * hardware (or sw model) to read in the unit test environment. */
  if (blk) *blk = mc_mgr_ctx_port_fwd_state(dev, idx);
  if (val) *val = (mc_mgr_ctx_port_fwd_state(dev, idx) >> shift) & 1;
#endif
  return BF_SUCCESS;
}

bf_status_t mc_mgr_set_port_mask_reg(bf_dev_id_t dev, int ver, int port) {
  if (0 > dev || MC_MGR_NUM_DEVICES <= dev) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (0 != ver && 1 != ver) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (port < 0 ||
      port >= (int)(BF_SUBDEV_PORT_COUNT * mc_mgr_ctx_num_subdevices(dev))) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  int idx = port / 32;
  uint32_t data = mc_mgr_ctx_port_fwd_state(dev, idx);
  uint32_t addr = 0;
  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      addr =
          ver *
              tofino_device_select_tm_top_tm_pre_top_pre_common_port_mask_array_element_size +
          tofino_device_select_tm_top_tm_pre_top_pre_common_port_mask_address +
          4 * idx;
      break;
    case BF_DEV_FAMILY_TOFINO2:
      addr =
          ver *
              tof2_reg_device_select_tm_top_tm_pre_top_pre_common_port_mask_array_element_size +
          tof2_reg_device_select_tm_top_tm_pre_top_pre_common_port_mask_address +
          4 * idx;
      break;
    case BF_DEV_FAMILY_TOFINO3:
      if (port < 0 || port >= (int)(MC_MGR_TOF3_PORT_COUNT *
                                    mc_mgr_ctx_num_subdevices(dev))) {
        MC_MGR_DBGCHK(0);
        return BF_INVALID_ARG;
      }
      addr =
          ver *
              tof3_reg_device_select_tm_top_tm_pre_top_pre_common_port_mask_array_element_size +
          tof3_reg_device_select_tm_top_tm_pre_top_pre_common_port_mask_address +
          4 * idx;
      break;
    default:
      MC_MGR_DBGCHK(0);
  }

  int x = mc_mgr_write_register(dev, addr, data);
  if (x) {
    LOG_ERROR("Failed to write PortMask%d.%d (%d) from %s:%d",
              ver,
              idx,
              x,
              __func__,
              __LINE__);
    MC_MGR_DBGCHK(0);
    return BF_HW_COMM_FAIL;
  }
  return BF_SUCCESS;
}

bf_status_t mc_mgr_set_port_mask_wrl(int sid,
                                     bf_dev_id_t dev,
                                     int ver,
                                     int port) {
  if (0 > sid || MC_MGR_NUM_SESSIONS <= sid) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (0 > dev || MC_MGR_NUM_DEVICES <= dev) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (0 != ver && 1 != ver) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (port < 0 ||
      port >= (int)(BF_SUBDEV_PORT_COUNT * mc_mgr_ctx_num_subdevices(dev))) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  int idx = port / 32;
  uint64_t hi = 0;
  uint64_t lo = mc_mgr_ctx_port_fwd_state(dev, idx);
  uint32_t daddr = 0;
  uint64_t addr = 0;
  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      daddr =
          ver *
              tofino_device_select_tm_top_tm_pre_top_pre_common_port_mask_array_element_size +
          tofino_device_select_tm_top_tm_pre_top_pre_common_port_mask_address +
          4 * idx;
      addr = tof_dir_to_indir_dev_sel(daddr);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      daddr =
          ver *
              tof2_reg_device_select_tm_top_tm_pre_top_pre_common_port_mask_array_element_size +
          tof2_reg_device_select_tm_top_tm_pre_top_pre_common_port_mask_address +
          4 * idx;
      addr = tof2_dir_to_indir_dev_sel(daddr);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      if (port < 0 || port >= (int)(MC_MGR_TOF3_PORT_COUNT *
                                    mc_mgr_ctx_num_subdevices(dev))) {
        MC_MGR_DBGCHK(0);
        return BF_INVALID_ARG;
      }
      daddr =
          ver *
              tof3_reg_device_select_tm_top_tm_pre_top_pre_common_port_mask_array_element_size +
          tof3_reg_device_select_tm_top_tm_pre_top_pre_common_port_mask_address +
          4 * idx;
      addr = tof3_dir_to_indir_dev_sel(daddr);
      break;
    default:
      MC_MGR_DBGCHK(0);
  }

  if (mc_mgr_drv_wrl_append(dev,
                            MC_MGR_DRV_SUBDEV_ID_ALL,
                            sid,
                            16,
                            addr << 4,
                            hi,
                            lo,
                            __func__,
                            __LINE__)) {
    LOG_ERROR("Failed to add PortMask%d.%d update to WRL from %s:%d",
              ver,
              idx,
              __func__,
              __LINE__);
    MC_MGR_DBGCHK(0);
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t mc_mgr_set_comm_ctrl_wrl(int sid, bf_dev_id_t dev) {
  if (0 > dev || MC_MGR_NUM_DEVICES <= dev) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  uint32_t pipe_rd_sel = mc_mgr_ctx_pipe_rd_sel(dev);
  uint32_t ff_mode = mc_mgr_ctx_ff_en(dev);
  uint32_t bkup_port_en = mc_mgr_ctx_bkup_port_en(dev);

  uint32_t addr = 0;
  uint32_t data = 0;

  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      addr =
          tofino_device_select_tm_top_tm_pre_top_pre_common_common_ctrl_address;
      setp_pre_common_ctrl_pipe_mem_read_sel(&data, pipe_rd_sel);
      setp_pre_common_ctrl_hw_port_liveness_en(&data, ff_mode);
      setp_pre_common_ctrl_backup_port_en(&data, bkup_port_en);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      addr =
          tof2_reg_device_select_tm_top_tm_pre_top_pre_common_common_ctrl_address;
      setp_tof2_pre_common_ctrl_pipe_mem_read_sel(&data, pipe_rd_sel);
      setp_tof2_pre_common_ctrl_hw_port_liveness_en(&data, ff_mode);
      setp_tof2_pre_common_ctrl_backup_port_en(&data, bkup_port_en);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      addr =
          tof3_reg_device_select_tm_top_tm_pre_top_pre_common_common_ctrl_address;
      setp_tof3_pre_common_ctrl_pipe_mem_read_sel(&data, pipe_rd_sel);
      setp_tof3_pre_common_ctrl_hw_port_liveness_en(&data, ff_mode);
      setp_tof3_pre_common_ctrl_backup_port_en(&data, bkup_port_en);
      break;
    default:
      MC_MGR_DBGCHK(0);
  }

  if (mc_mgr_drv_wrl_append_reg(dev, sid, addr, data, __func__, __LINE__)) {
    LOG_ERROR("Failed to post common_ctrl update, dev %d", dev);
    return BF_HW_COMM_FAIL;
  }
  return BF_SUCCESS;
}

bf_status_t mc_mgr_get_port_down_reg(bf_dev_id_t dev, int port, bool *down) {
  if (0 > dev || MC_MGR_NUM_DEVICES <= dev) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (port < 0 ||
      port >= (int)(BF_SUBDEV_PORT_COUNT * mc_mgr_ctx_num_subdevices(dev))) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  int idx = port / 32;
  int shift = port % 32;
  uint32_t addr = 0;
  uint32_t data = 0;

  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      addr =
          4 * idx +
          tofino_device_select_tm_top_tm_pre_top_pre_common_port_down_address;
      break;
    case BF_DEV_FAMILY_TOFINO2:
      addr =
          4 * idx +
          tof2_reg_device_select_tm_top_tm_pre_top_pre_common_port_down_address;
      break;
    case BF_DEV_FAMILY_TOFINO3:
      if (port < 0 || port >= (int)(MC_MGR_TOF3_PORT_COUNT *
                                    mc_mgr_ctx_num_subdevices(dev))) {
        return BF_SUCCESS;
      }
      addr =
          4 * idx +
          tof3_reg_device_select_tm_top_tm_pre_top_pre_common_port_down_address;
      break;
    default:
      MC_MGR_DBGCHK(0);
  }

  int x = lld_read_register(dev, addr, &data);
  if (x) {
    LOG_ERROR(
        "Failed to read port down mask (%d) from %s:%d", x, __func__, __LINE__);
    MC_MGR_DBGCHK(0);
    return BF_HW_COMM_FAIL;
  }

  *down = (data >> shift) & 1;

  return BF_SUCCESS;
}
bf_status_t mc_mgr_clr_port_down_reg(bf_dev_id_t dev, int port) {
  if (0 > dev || MC_MGR_NUM_DEVICES <= dev) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (port < 0 ||
      port >= (int)(BF_SUBDEV_PORT_COUNT * mc_mgr_ctx_num_subdevices(dev))) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  int idx = port / 32;
  int shift = port % 32;
  uint32_t addr = 0;
  uint32_t data = 1u << shift;

  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      addr =
          4 * idx +
          tofino_device_select_tm_top_tm_pre_top_pre_common_port_down_address;
      break;
    case BF_DEV_FAMILY_TOFINO2:
      addr =
          4 * idx +
          tof2_reg_device_select_tm_top_tm_pre_top_pre_common_port_down_address;
      break;
    case BF_DEV_FAMILY_TOFINO3:
      if (port < 0 || port >= (int)(MC_MGR_TOF3_PORT_COUNT *
                                    mc_mgr_ctx_num_subdevices(dev))) {
        MC_MGR_DBGCHK(0);
        return BF_INVALID_ARG;
      }
      addr =
          4 * idx +
          tof3_reg_device_select_tm_top_tm_pre_top_pre_common_port_down_address;
      break;
    default:
      MC_MGR_DBGCHK(0);
  }
  int x = mc_mgr_write_register(dev, addr, data);
  if (x) {
    LOG_ERROR("Failed to write port down mask (%d) from %s:%d",
              x,
              __func__,
              __LINE__);
    MC_MGR_DBGCHK(0);
    return BF_HW_COMM_FAIL;
  }

  return BF_SUCCESS;
}

bf_status_t mc_mgr_get_comm_ctrl_reg(bf_dev_id_t dev,
                                     uint8_t *pipe_sel,
                                     bool *ff_en,
                                     bool *bkup_en) {
  if (0 > dev || MC_MGR_NUM_DEVICES <= dev) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  uint32_t addr = 0;
  uint32_t data = 0;

  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      addr =
          tofino_device_select_tm_top_tm_pre_top_pre_common_common_ctrl_address;
      break;
    case BF_DEV_FAMILY_TOFINO2:
      addr =
          tof2_reg_device_select_tm_top_tm_pre_top_pre_common_common_ctrl_address;
      break;
    case BF_DEV_FAMILY_TOFINO3:
      addr =
          tof3_reg_device_select_tm_top_tm_pre_top_pre_common_common_ctrl_address;
      break;
    default:
      MC_MGR_DBGCHK(0);
  }

  int x = lld_read_register(dev, addr, &data);
  if (x) {
    LOG_ERROR(
        "Failed to read common-ctrl (%d) from %s:%d", x, __func__, __LINE__);
    MC_MGR_DBGCHK(0);
    return BF_HW_COMM_FAIL;
  }

  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      *pipe_sel = getp_pre_common_ctrl_pipe_mem_read_sel(&data);
      *ff_en = getp_pre_common_ctrl_hw_port_liveness_en(&data);
      *bkup_en = getp_pre_common_ctrl_backup_port_en(&data);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      *pipe_sel = getp_tof2_pre_common_ctrl_pipe_mem_read_sel(&data);
      *ff_en = getp_tof2_pre_common_ctrl_hw_port_liveness_en(&data);
      *bkup_en = getp_tof2_pre_common_ctrl_backup_port_en(&data);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      *pipe_sel = getp_tof3_pre_common_ctrl_pipe_mem_read_sel(&data);
      *ff_en = getp_tof3_pre_common_ctrl_hw_port_liveness_en(&data);
      *bkup_en = getp_tof3_pre_common_ctrl_backup_port_en(&data);
      break;
    default:
      MC_MGR_DBGCHK(0);
  }
#ifdef UTEST
  /* Pull the data out of our software shadow and return it since there is no
   * hardware (or sw model) to read in the unit test environment. */
  *pipe_sel = 0;
  *ff_en = mc_mgr_ctx_ff_en(dev);
  *bkup_en = mc_mgr_ctx_bkup_port_en(dev);
#endif

  return BF_SUCCESS;
}

bf_status_t mc_mgr_get_pre_ctrl_reg(bf_dev_id_t dev,
                                    bf_dev_pipe_t pipe,
                                    bool *c2c_en,
                                    int *c2c_port,
                                    uint8_t *l1_slice) {
  uint32_t addr = 0;
  int subdev = 0;

  subdev = pipe / BF_SUBDEV_PIPE_COUNT;
  pipe = pipe % BF_SUBDEV_PIPE_COUNT;
  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      addr = offsetof(Tofino, device_select.tm_top.tm_pre_top.pre[pipe].ctrl);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      addr = offsetof(tof2_reg, device_select.tm_top.tm_pre_top.pre[pipe].ctrl);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      addr = offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[pipe].ctrl);
      break;
    default:
      MC_MGR_DBGCHK(0);
  }
  uint32_t data = 0;

  int x = lld_subdev_read_register(dev, subdev, addr, &data);
  if (x) {
    LOG_ERROR("Failed to read dev %d pipe%d-ctrl (%d)", dev, pipe, x);
    MC_MGR_DBGCHK(0);
    return BF_HW_COMM_FAIL;
  }
  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      *c2c_en = getp_pre_ctrl_c2c_enable(&data);
      *c2c_port = getp_pre_ctrl_c2c_port(&data);
      *l1_slice = getp_pre_ctrl_l1_per_slice(&data);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      *c2c_en = getp_tof2_pre_ctrl_c2c_enable(&data);
      *c2c_port = getp_tof2_pre_ctrl_c2c_port(&data);
      *l1_slice = getp_tof2_pre_ctrl_l1_per_slice(&data);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      *c2c_en = getp_tof3_pre_ctrl_c2c_enable(&data);
      *c2c_port = getp_tof3_pre_ctrl_c2c_port(&data);
      *l1_slice = getp_tof3_pre_ctrl_l1_per_slice(&data);
      break;
    default:
      MC_MGR_DBGCHK(0);
  }
#ifdef UTEST
  /* Pull the data out of our software shadow and return it since there is no
   * hardware (or sw model) to read in the unit test environment. */
  *c2c_en = mc_mgr_ctx_cpu_port_en(dev);
  if (*c2c_en) {
    *c2c_port = mc_dev_port_to_local_port(dev, mc_mgr_ctx_cpu_port(dev));
  } else {
    *c2c_port = 0;
  }
  *l1_slice = mc_mgr_ctx_max_l1_ts(dev);
#endif
  return BF_SUCCESS;
}

bf_status_t mc_mgr_set_pre_ctrl_wrl(int sid, bf_dev_id_t dev) {
  if (0 > dev || MC_MGR_NUM_DEVICES <= dev) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  uint32_t count = mc_mgr_ctx_max_l1_ts(dev);
  int c2c_pipe = mc_dev_port_to_pipe(dev, mc_mgr_ctx_cpu_port(dev));
  int c2c_port = mc_dev_port_to_local_port(dev, mc_mgr_ctx_cpu_port(dev));
  bool c2c_en = mc_mgr_ctx_cpu_port_en(dev);

  int p;
  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    uint32_t addr = 0;
    switch (mc_mgr_ctx_dev_family(dev)) {
      case BF_DEV_FAMILY_TOFINO:
        addr = offsetof(Tofino, device_select.tm_top.tm_pre_top.pre[p].ctrl);
        break;
      case BF_DEV_FAMILY_TOFINO2:
        addr = offsetof(tof2_reg, device_select.tm_top.tm_pre_top.pre[p].ctrl);
        break;
      case BF_DEV_FAMILY_TOFINO3:
        addr = offsetof(tof3_reg, device_select.tm_top.tm_pre_top.pre[p].ctrl);
        break;
      default:
        MC_MGR_DBGCHK(0);
    }
    uint32_t data = 0;
    switch (mc_mgr_ctx_dev_family(dev)) {
      case BF_DEV_FAMILY_TOFINO:
        setp_pre_ctrl_l1_per_slice(&data, count);
        setp_pre_ctrl_c2c_enable(&data, c2c_en && p == c2c_pipe);
        setp_pre_ctrl_c2c_port(&data, p == c2c_pipe ? c2c_port : 64);
        break;
      case BF_DEV_FAMILY_TOFINO2:
        setp_tof2_pre_ctrl_l1_per_slice(&data, count);
        setp_tof2_pre_ctrl_c2c_enable(&data, c2c_en && p == c2c_pipe);
        setp_tof2_pre_ctrl_c2c_port(&data, p == c2c_pipe ? c2c_port : 64);
        break;
      case BF_DEV_FAMILY_TOFINO3:
        setp_tof3_pre_ctrl_l1_per_slice(&data, count);
        setp_tof3_pre_ctrl_c2c_enable(&data, c2c_en && p == c2c_pipe);
        setp_tof3_pre_ctrl_c2c_port(&data, p == c2c_pipe ? c2c_port : 64);
        break;
      default:
        MC_MGR_DBGCHK(0);
    }
    if (mc_mgr_drv_wrl_append_reg(dev, sid, addr, data, __func__, __LINE__)) {
      LOG_ERROR("Failed to post pre_ctrl update, dev %d pipe %d", dev, p);
      return BF_HW_COMM_FAIL;
    }
  }

  return BF_SUCCESS;
}

bf_status_t mc_mgr_get_max_l1_reg(bf_dev_id_t dev,
                                  bf_dev_pipe_t pipe,
                                  int *count) {
  if (0 > dev || MC_MGR_NUM_DEVICES <= dev) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (pipe >= mc_mgr_ctx_num_max_pipes(dev)) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  uint32_t addr = 0;
  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      addr = offsetof(
          Tofino, device_select.tm_top.tm_pre_top.pre[pipe].max_l1_node_ctrl);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      addr = offsetof(
          tof2_reg, device_select.tm_top.tm_pre_top.pre[pipe].max_l1_node_ctrl);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      addr = offsetof(
          tof3_reg, device_select.tm_top.tm_pre_top.pre[pipe].max_l1_node_ctrl);
      break;
    default:
      MC_MGR_DBGCHK(0);
  }
  uint32_t data = 0;

  int x = lld_read_register(dev, addr, &data);
  if (x) {
    LOG_ERROR("Failed to read dev %d pipe%d-max-l1-ctrl (%d) from %s:%d",
              dev,
              pipe,
              x,
              __func__,
              __LINE__);
    MC_MGR_DBGCHK(0);
    return BF_HW_COMM_FAIL;
  }

  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      *count = getp_pre_max_l1_node_ctrl_l1_max_node(&data);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      *count = getp_tof2_pre_max_l1_node_ctrl_l1_max_node(&data);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      *count = getp_tof3_pre_max_l1_node_ctrl_l1_max_node(&data);
      break;
    default:
      MC_MGR_DBGCHK(0);
  }
#ifdef UTEST
  /* Pull the data out of our software shadow and return it since there is no
   * hardware (or sw model) to read in the unit test environment. */
  *count = mc_mgr_ctx_max_l1(dev);
#endif

  return BF_SUCCESS;
}

bf_status_t mc_mgr_set_max_l1_wrl(int sid, bf_dev_id_t dev) {
  if (0 > dev || MC_MGR_NUM_DEVICES <= dev) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  uint32_t data = 0;
  uint32_t count = mc_mgr_ctx_max_l1(dev);
  int p;
  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    uint32_t addr = 0;
    switch (mc_mgr_ctx_dev_family(dev)) {
      case BF_DEV_FAMILY_TOFINO:
        addr = offsetof(
            Tofino, device_select.tm_top.tm_pre_top.pre[p].max_l1_node_ctrl);
        setp_pre_max_l1_node_ctrl_l1_max_node(&data, count);
        break;
      case BF_DEV_FAMILY_TOFINO2:
        addr = offsetof(
            tof2_reg, device_select.tm_top.tm_pre_top.pre[p].max_l1_node_ctrl);
        setp_tof2_pre_max_l1_node_ctrl_l1_max_node(&data, count);
        break;
      case BF_DEV_FAMILY_TOFINO3:
        addr = offsetof(
            tof3_reg, device_select.tm_top.tm_pre_top.pre[p].max_l1_node_ctrl);
        setp_tof3_pre_max_l1_node_ctrl_l1_max_node(&data, count);
        break;
      default:
        MC_MGR_DBGCHK(0);
    }
    if (mc_mgr_drv_wrl_append_reg(dev, sid, addr, data, __func__, __LINE__)) {
      LOG_ERROR("Failed to post max_l1 update, dev %d pipe %d", dev, p);
      return BF_HW_COMM_FAIL;
    }
  }
  return BF_SUCCESS;
}

bf_status_t mc_mgr_get_max_l2_reg(bf_dev_id_t dev,
                                  bf_dev_pipe_t pipe,
                                  int *count) {
  if (0 > dev || MC_MGR_NUM_DEVICES <= dev) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (pipe >= mc_mgr_ctx_num_max_pipes(dev)) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  uint32_t addr = 0;
  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      addr = offsetof(
          Tofino, device_select.tm_top.tm_pre_top.pre[pipe].max_l2_node_ctrl);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      addr = offsetof(
          tof2_reg, device_select.tm_top.tm_pre_top.pre[pipe].max_l2_node_ctrl);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      addr = offsetof(
          tof3_reg, device_select.tm_top.tm_pre_top.pre[pipe].max_l2_node_ctrl);
      break;
    default:
      MC_MGR_DBGCHK(0);
  }
  uint32_t data = 0;

  int x = lld_read_register(dev, addr, &data);
  if (x) {
    LOG_ERROR("Failed to read dev %d pipe%d-max-l2-ctrl (%d) from %s:%d",
              dev,
              pipe,
              x,
              __func__,
              __LINE__);
    MC_MGR_DBGCHK(0);
    return BF_HW_COMM_FAIL;
  }

  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      *count = getp_pre_max_l2_node_ctrl_l2_max_node(&data);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      *count = getp_tof2_pre_max_l2_node_ctrl_l2_max_node(&data);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      *count = getp_tof3_pre_max_l2_node_ctrl_l2_max_node(&data);
      break;
    default:
      MC_MGR_DBGCHK(0);
  }
#ifdef UTEST
  /* Pull the data out of our software shadow and return it since there is no
   * hardware (or sw model) to read in the unit test environment. */
  *count = mc_mgr_ctx_max_l2(dev);
#endif

  return BF_SUCCESS;
}

bf_status_t mc_mgr_set_max_l2_wrl(int sid, bf_dev_id_t dev) {
  if (0 > dev || MC_MGR_NUM_DEVICES <= dev) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  uint32_t data = 0;
  uint32_t count = mc_mgr_ctx_max_l2(dev);
  int p;
  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    uint32_t addr = 0;
    switch (mc_mgr_ctx_dev_family(dev)) {
      case BF_DEV_FAMILY_TOFINO:
        addr = offsetof(
            Tofino, device_select.tm_top.tm_pre_top.pre[p].max_l2_node_ctrl);
        setp_pre_max_l2_node_ctrl_l2_max_node(&data, count);
        break;
      case BF_DEV_FAMILY_TOFINO2:
        addr = offsetof(
            tof2_reg, device_select.tm_top.tm_pre_top.pre[p].max_l2_node_ctrl);
        setp_tof2_pre_max_l2_node_ctrl_l2_max_node(&data, count);
        break;
      case BF_DEV_FAMILY_TOFINO3:
        addr = offsetof(
            tof3_reg, device_select.tm_top.tm_pre_top.pre[p].max_l2_node_ctrl);
        setp_tof3_pre_max_l2_node_ctrl_l2_max_node(&data, count);
        break;
      default:
        MC_MGR_DBGCHK(0);
    }
    if (mc_mgr_drv_wrl_append_reg(dev, sid, addr, data, __func__, __LINE__)) {
      LOG_ERROR("Failed to post max_l2 update, dev %d pipe %d", dev, p);
      return BF_HW_COMM_FAIL;
    }
  }
  return BF_SUCCESS;
}

bf_status_t mc_mgr_c2c_pipe_msk_set_reg_wrl(int sid,
                                            bf_dev_id_t dev,
                                            uint32_t mask) {
  if (mc_mgr_ctx_dev_family(dev) != BF_DEV_FAMILY_TOFINO3) {
    return BF_SUCCESS;
  }

  uint32_t addr =
      tof3_reg_device_select_tm_top_tm_wac_top_wac_common_wac_common_wac_copy_to_cpu_pv_address;

  if (mc_mgr_drv_wrl_append_reg(dev, sid, addr, mask, __func__, __LINE__)) {
    LOG_ERROR("Failed to post wac c2c update, dev %d", dev);
    return BF_HW_COMM_FAIL;
  }
  return BF_SUCCESS;
}
