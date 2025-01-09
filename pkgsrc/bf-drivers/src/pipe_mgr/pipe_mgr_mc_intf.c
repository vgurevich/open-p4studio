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


#include <dvm/bf_drv_intf.h>
#include <lld/bf_dma_if.h>
#include <lld/lld_reg_if.h>
#include <pipe_mgr/pipe_mgr_mc_intf.h>
#include <tof3_regs/tof3_reg_drv.h>
#include <tof3_regs/tof3_mem_drv.h>

#include "pipe_mgr_int.h"
#include "pipe_mgr_drv.h"
#include "pipe_mgr_tof_mc_intf.h"
#include "pipe_mgr_tof2_mc_intf.h"
#include "pipe_mgr_tof3_mc_intf.h"
#include "mc_mgr/mc_mgr_drv.h"
extern pipe_mgr_ctx_t *pipe_mgr_ctx;

// forward declaration
static pipe_status_t pipe_mgr_mc_pipe_msk_set_internal_reg(
    pipe_sess_hdl_t shdl,
    dev_target_t dev,
    rmt_dev_info_t *dev_info,
    uint32_t tb0_addr,
    uint32_t tb1_addr,
    uint32_t table_msk,
    uint32_t masks,
    bool push,
    bool complete);

pipe_status_t pipe_mgr_mc_pipe_msk_set(pipe_sess_hdl_t shdl,
                                       dev_target_t dev,
                                       int mgid_grp,
                                       mc_mgr_pvt_entry_t masks,
                                       uint32_t table_msk,
                                       bool push,
                                       bool complete,
                                       bool rebuilding) {
  pipe_status_t sts = PIPE_SUCCESS;
  /* Only take the api lock for normal APIs, as rebuilding will use the
   * exclusive lock instead.
   */
  if (!rebuilding) {
    sts = pipe_mgr_api_enter(shdl);
    if (sts != PIPE_SUCCESS) {
      return PIPE_SESSION_NOT_FOUND;
    }
  }

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev.device_id);
  if (dev_info == NULL) {
    LOG_ERROR("%s:%d Device info for device id %d not found",
              __func__,
              __LINE__,
              dev.device_id);
    sts = PIPE_OBJ_NOT_FOUND;
    goto done;
  }

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
    case BF_DEV_FAMILY_TOFINO2: {
      uint32_t pv_tbl0_addr = 0, pv_tbl1_addr = 0;
      if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO) {
        sts = pipe_mgr_tof_mc_mgid_grp_addr_get(
            mgid_grp, &pv_tbl0_addr, &pv_tbl1_addr);
      } else if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO2) {
        sts = pipe_mgr_tof2_mc_mgid_grp_addr_get(
            mgid_grp, &pv_tbl0_addr, &pv_tbl1_addr);
      } else {
        sts = PIPE_INVALID_ARG;
      }
      if (sts != PIPE_SUCCESS) {
        goto done;
      }
      sts = pipe_mgr_mc_pipe_msk_set_internal_reg(shdl,
                                                  dev,
                                                  dev_info,
                                                  pv_tbl0_addr,
                                                  pv_tbl1_addr,
                                                  table_msk,
                                                  (uint32_t)masks.d,
                                                  push,
                                                  complete);
      if (sts != PIPE_SUCCESS) {
        goto done;
      }
    }
    case BF_DEV_FAMILY_TOFINO3: {
      /* Mc-mgr programs the PVT table for tofino3 */
      break;
    }
    default:
      PIPE_MGR_DBGCHK(0);
      sts = PIPE_UNEXPECTED;
      break;
  }
done:
  if (!rebuilding) {
    pipe_mgr_api_exit(shdl);
  }
  return sts;
}

pipe_status_t pipe_mgr_mc_pipe_tvt_msk_set(pipe_sess_hdl_t shdl,
                                           dev_target_t dev,
                                           uint16_t mgid_grp,
                                           mc_mgr_tvt_entry_t masks,
                                           bool push,
                                           bool complete,
                                           bool rebuilding) {
  pipe_status_t sts = PIPE_SUCCESS;
  if (!pipe_mgr_ctx) {
    return PIPE_SUCCESS;
  }
  /* Only take the api lock for normal APIs, as rebuilding will use the
   * exclusive lock instead.
   */
  if (!rebuilding) {
    sts = pipe_mgr_api_enter(shdl);
    if (sts != PIPE_SUCCESS) {
      return BF_SESSION_NOT_FOUND;
    }
  }

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev.device_id);
  if (dev_info == NULL) {
    LOG_ERROR("%s:%d Device info for device id %d not found",
              __func__,
              __LINE__,
              dev.device_id);
    sts = PIPE_OBJ_NOT_FOUND;
    goto done;
  }

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO3: {
      /* set tvt */
      uint32_t tv_tbl0_addr = offsetof(
          tof3_reg,
          pipes[0].pardereg.dprsrreg.dprsrreg.inp.iim.tv_table.tbl0[mgid_grp]);
      uint32_t tv_tbl1_addr = offsetof(
          tof3_reg,
          pipes[0].pardereg.dprsrreg.dprsrreg.inp.iim.tv_table.tbl1[mgid_grp]);
      sts = pipe_mgr_mc_pipe_msk_set_internal_reg(shdl,
                                                  dev,
                                                  dev_info,
                                                  tv_tbl0_addr,
                                                  tv_tbl1_addr,
                                                  MC_PVT_MASK_ALL,
                                                  (uint32_t)masks.diemap,
                                                  push,
                                                  complete);
      if (sts != PIPE_SUCCESS) {
        goto done;
      }
      break;
    }
    case BF_DEV_FAMILY_TOFINO:
    case BF_DEV_FAMILY_TOFINO2:
    default:
      PIPE_MGR_DBGCHK(0);
      sts = PIPE_UNEXPECTED;
      break;
  }
done:
  if (!rebuilding) {
    pipe_mgr_api_exit(shdl);
  }
  return sts;
}

static pipe_status_t pipe_mgr_mc_pipe_msk_set_internal_reg(
    pipe_sess_hdl_t shdl,
    dev_target_t dev,
    rmt_dev_info_t *dev_info,
    uint32_t tbl0_addr,
    uint32_t tbl1_addr,
    uint32_t table_msk,
    uint32_t masks,
    bool push,
    bool complete) {
  pipe_instr_write_reg_t instr[2];
  pipe_status_t sts = PIPE_SUCCESS;

  construct_instr_reg_write(dev.device_id, &instr[0], tbl0_addr, masks);
  construct_instr_reg_write(dev.device_id, &instr[1], tbl1_addr, masks);

  pipe_bitmap_t pipe_bit_map = {{0}};
  PIPE_BITMAP_INIT(&pipe_bit_map, PIPE_BMP_SIZE);

  uint32_t pipe_cnt = pipe_mgr_get_num_active_pipes(dev.device_id);
  uint32_t i;
  for (i = 0; i < pipe_cnt; ++i) PIPE_BITMAP_SET(&pipe_bit_map, i);
  uint32_t stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(tbl0_addr);

  for (i = 0; i < 2; ++i) {
    if (table_msk & (1 << i)) {
      sts = pipe_mgr_drv_ilist_add(&shdl,
                                   dev_info,
                                   &pipe_bit_map,
                                   stage,
                                   (uint8_t *)(&instr[i]),
                                   sizeof(instr[i]));
      if (PIPE_SUCCESS != sts) {
        LOG_ERROR("Failed to add pipe mask set to instruction list (%d)", sts);
        PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
        goto done;
      }
    }
  }

  if (push) {
    sts = pipe_mgr_drv_ilist_push(&shdl, NULL, NULL);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Failed to push set-pipe-mask instruction list (%d)", sts);
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
      goto done;
    }

    if (complete) {
      sts = pipe_mgr_complete_operations(shdl);
      if (PIPE_SUCCESS != sts) {
        LOG_ERROR("Failed to complete set-pipe-mask instruction list (%d)",
                  sts);
        PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
        goto done;
      }
    }
  }

done:
  return sts;
}

pipe_status_t pipe_mgr_mc_c2c_pipe_msk_set(pipe_sess_hdl_t shdl,
                                           dev_target_t dev,
                                           uint32_t mask,
                                           bool push,
                                           bool complete) {
  pipe_instr_write_reg_t instr;
  pipe_instr_write_reg_t c2c_tvt_instr;
  pipe_status_t sts = PIPE_SUCCESS;
  uint32_t addr = 0;
  uint32_t c2c_tvt_addr = 0;

  if (PIPE_SUCCESS != pipe_mgr_api_enter(shdl)) return PIPE_SESSION_NOT_FOUND;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev.device_id);
  if (dev_info == NULL) {
    LOG_ERROR("%s:%d Device info for device id %d not found",
              __func__,
              __LINE__,
              dev.device_id);
    pipe_mgr_api_exit(shdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* For TF3, issue the Die mask instead of c2c_pipe_mask to the deparser block
   */
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      sts = pipe_mgr_tof_mc_copy_to_cpu_pv_addr_get(0, &addr);
      break;

    case BF_DEV_FAMILY_TOFINO2:
      sts = pipe_mgr_tof2_mc_copy_to_cpu_pv_addr_get(0, &addr);
      break;

    case BF_DEV_FAMILY_TOFINO3:
      sts = pipe_mgr_tof3_mc_copy_to_cpu_tvt_addr_get(0, &c2c_tvt_addr);
      break;

    default:
      PIPE_MGR_DBGCHK(0);
      sts = PIPE_UNEXPECTED;
      break;
  }
  if (sts != PIPE_SUCCESS) {
    pipe_mgr_api_exit(shdl);
    return sts;
  }

  pipe_bitmap_t pipe_bit_map = {{0}};
  PIPE_BITMAP_INIT(&pipe_bit_map, PIPE_BMP_SIZE);

  uint32_t pipe_cnt = pipe_mgr_get_num_active_pipes(dev.device_id);
  uint32_t i;
  for (i = 0; i < pipe_cnt; ++i) PIPE_BITMAP_SET(&pipe_bit_map, i);

  if (dev_info->dev_family != BF_DEV_FAMILY_TOFINO3) {
    construct_instr_reg_write(dev.device_id, &instr, addr, mask);

    uint32_t stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(addr);

    sts = pipe_mgr_drv_ilist_add(&shdl,
                                 dev_info,
                                 &pipe_bit_map,
                                 stage,
                                 (uint8_t *)(&instr),
                                 sizeof(instr));
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Failed to add c2c pipe mask set to instruction list (%d)",
                sts);
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
      pipe_mgr_api_exit(shdl);
      return sts;
    }
  } else {
    uint32_t die_mask = 0, pipe = 0;

    /* Construct the die mask */
    for (pipe = 0; pipe < BF_PIPE_COUNT; pipe++) {
      if (mask & (0x1u << pipe)) {
        if (pipe < BF_SUBDEV_PIPE_COUNT) {
          /* left die present */
          die_mask |= 0x1;
        } else {
          /* right die present */
          die_mask |= 0x2;
        }
      }
    }

    /* Set the default die-mask to 1 as HW expects packets to be forwarded
     * to TM */
    if (!die_mask) die_mask = 1;
    construct_instr_reg_write(
        dev.device_id, &c2c_tvt_instr, c2c_tvt_addr, die_mask);
    uint32_t c2c_tvt_stage =
        dev_info->dev_cfg.pcie_pipe_addr_get_stage(c2c_tvt_addr);
    sts = pipe_mgr_drv_ilist_add(&shdl,
                                 dev_info,
                                 &pipe_bit_map,
                                 c2c_tvt_stage,
                                 (uint8_t *)(&c2c_tvt_instr),
                                 sizeof(c2c_tvt_instr));
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Failed to add c2c pipe tvt mask set to instruction list (%d)",
                sts);
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
      pipe_mgr_api_exit(shdl);
      return sts;
    }
  }

  if (push) {
    sts = pipe_mgr_drv_ilist_push(&shdl, NULL, NULL);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Failed to push set-c2c-pipe-mask instruction list (%d)", sts);
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
      pipe_mgr_api_exit(shdl);
      return sts;
    }

    if (complete) {
      sts = pipe_mgr_complete_operations(shdl);
      if (PIPE_SUCCESS != sts) {
        LOG_ERROR("Failed to complete set-c2c-pipe-mask instruction list (%d)",
                  sts);
        PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
        pipe_mgr_api_exit(shdl);
        return sts;
      }
    }
  }

  pipe_mgr_api_exit(shdl);
  return sts;
}

pipe_status_t pipe_mgr_mc_pipe_msk_update_push(pipe_sess_hdl_t shdl,
                                               bool complete) {
  if (!pipe_mgr_ctx) {
    return PIPE_SUCCESS;
  }
  if (PIPE_SUCCESS != pipe_mgr_api_enter(shdl)) return PIPE_SESSION_NOT_FOUND;

  pipe_status_t sts = pipe_mgr_drv_ilist_push(&shdl, NULL, NULL);
  if (PIPE_SUCCESS != sts) {
    LOG_ERROR("Failed to push set-pipe-mask instruction list (%d)", sts);
    PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
    pipe_mgr_api_exit(shdl);
    return sts;
  }

  if (complete) {
    sts = pipe_mgr_complete_operations(shdl);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Failed to complete set-pipe-mask instruction list (%d)", sts);
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
      pipe_mgr_api_exit(shdl);
      return sts;
    }
  }
  pipe_mgr_api_exit(shdl);
  return sts;
}

pipe_status_t pipe_mgr_mc_pipe_msk_get(bf_dev_id_t dev,
                                       int mgid_grp,
                                       int phy_pipe,
                                       uint32_t *masks) {
  pipe_status_t sts = PIPE_SUCCESS;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) return PIPE_INVALID_ARG;
  if (phy_pipe < 0 || phy_pipe >= (int)dev_info->num_active_pipes) {
    return PIPE_INVALID_ARG;
  }
  if (mgid_grp < 0) {
    return PIPE_INVALID_ARG;
  }
  bf_subdev_id_t subdev = phy_pipe / BF_SUBDEV_PIPE_COUNT;

  /* Need to validate that the physical pipe requested is actually present on
   * the chip.  Multicast always works on four pipes so it may request a read
   * for a pipe that is turned off.  Here we will catch those reads and return
   * an error. */
  uint32_t pipe_cnt = pipe_mgr_get_num_active_pipes(dev);
  uint32_t log_pipe;
  for (log_pipe = 0; log_pipe < pipe_cnt; ++log_pipe) {
    /* Map the logical pipe to a physical. */
    bf_dev_pipe_t p;
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe, &p);
    if (p == (unsigned int)phy_pipe) break;
  }
  if (log_pipe < pipe_cnt) {
    /* Found a correct mapping of logical pipe to physical pipe. */
  } else {
    /* The requested physical pipe doesn't map to a logical pipe, return an
     * error. */
    return PIPE_INVALID_ARG;
  }
  /* Do the read with the physical pipe. */
  uint32_t addr = 0;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      if (mgid_grp >= 8 * 1024) return PIPE_INVALID_ARG;
      sts = pipe_mgr_tof_mc_pv_table0_addr_get(phy_pipe, mgid_grp, &addr);
      break;

    case BF_DEV_FAMILY_TOFINO2:
      if (mgid_grp >= 16 * 1024) return PIPE_INVALID_ARG;
      sts = pipe_mgr_tof2_mc_pv_table0_addr_get(phy_pipe, mgid_grp, &addr);
      break;

    case BF_DEV_FAMILY_TOFINO3:
      if (mgid_grp >= 16 * 1024) return PIPE_INVALID_ARG;
      sts = pipe_mgr_tof3_mc_tvt_table0_addr_get(
          phy_pipe % BF_SUBDEV_PIPE_COUNT, mgid_grp, &addr);
      break;

    default:
      PIPE_MGR_DBGCHK(0);
      sts = PIPE_UNEXPECTED;
      break;
  }
  if (sts != PIPE_SUCCESS) {
    return sts;
  }
  int x = lld_subdev_read_register(dev, subdev, addr, masks);
  if (x) {
    *masks = 0;
    return PIPE_COMM_FAIL;
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_mc_tvt_msk_get(bf_dev_id_t dev,
                                      int mgid_grp,
                                      int phy_pipe,
                                      uint32_t *masks) {
  pipe_status_t sts = PIPE_SUCCESS;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) return PIPE_INVALID_ARG;
  if (phy_pipe < 0 || phy_pipe >= (int)dev_info->num_active_pipes) {
    return PIPE_INVALID_ARG;
  }
  if (mgid_grp < 0) {
    return PIPE_INVALID_ARG;
  }
  bf_subdev_id_t subdev = phy_pipe / BF_SUBDEV_PIPE_COUNT;

  /* Need to validate that the physical pipe requested is actually present on
   * the chip.  Multicast always works on four pipes so it may request a read
   * for a pipe that is turned off.  Here we will catch those reads and return
   * an error. */
  uint32_t pipe_cnt = pipe_mgr_get_num_active_pipes(dev);
  uint32_t log_pipe;
  for (log_pipe = 0; log_pipe < pipe_cnt; ++log_pipe) {
    /* Map the logical pipe to a physical. */
    bf_dev_pipe_t p;
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe, &p);
    if (p == (unsigned int)phy_pipe) break;
  }
  if (log_pipe < pipe_cnt) {
    /* Found a correct mapping of logical pipe to physical pipe. */
  } else {
    /* The requested physical pipe doesn't map to a logical pipe, return an
     * error. */
    return PIPE_INVALID_ARG;
  }
  /* Do the read with the physical pipe. */
  uint32_t addr = 0;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO3:
      if (mgid_grp >= 16 * 1024) return PIPE_INVALID_ARG;
      sts = pipe_mgr_tof3_mc_tvt_table0_addr_get(
          phy_pipe % BF_SUBDEV_PIPE_COUNT, mgid_grp, &addr);
      break;
    case BF_DEV_FAMILY_TOFINO:
    case BF_DEV_FAMILY_TOFINO2:
    default:
      PIPE_MGR_DBGCHK(0);
      sts = PIPE_UNEXPECTED;
      break;
  }
  if (sts != PIPE_SUCCESS) {
    return sts;
  }
  int x = lld_subdev_read_register(dev, subdev, addr, masks);
  if (x) {
    *masks = 0;
    return PIPE_COMM_FAIL;
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_mc_c2c_pipe_msk_get(bf_dev_id_t dev,
                                           bf_dev_pipe_t pipe,
                                           uint32_t *mask) {
  pipe_status_t sts = PIPE_SUCCESS;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) return PIPE_INVALID_ARG;
  if (pipe >= dev_info->num_active_pipes) {
    return PIPE_INVALID_ARG;
  }

  /* Map the physical pipe (since multicast always works on physical pipes)
   * to the logical pipe. */
  bf_dev_pipe_t physical_pipe = pipe;
  bf_dev_pipe_t logical_pipe = pipe;
  bf_subdev_id_t subdev = pipe / BF_SUBDEV_PIPE_COUNT;
  pipe_mgr_map_phy_pipe_id_to_log_pipe_id(dev, physical_pipe, &logical_pipe);
  /* Ensure the logical pipe exists on the device. */
  uint32_t logical_pipe_count = pipe_mgr_get_num_active_pipes(dev);
  if (logical_pipe >= logical_pipe_count) return PIPE_COMM_FAIL;

  /* Do the read with the physical pipe. */
  uint32_t addr = 0;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      sts = pipe_mgr_tof_mc_copy_to_cpu_pv_addr_get(pipe, &addr);
      break;

    case BF_DEV_FAMILY_TOFINO2:
      sts = pipe_mgr_tof2_mc_copy_to_cpu_pv_addr_get(pipe, &addr);
      break;

    case BF_DEV_FAMILY_TOFINO3:
      sts = pipe_mgr_tof3_mc_copy_to_cpu_tvt_addr_get(
          pipe % BF_SUBDEV_PIPE_COUNT, &addr);
      break;

    default:
      PIPE_MGR_DBGCHK(0);
      sts = PIPE_UNEXPECTED;
      break;
  }
  if (sts != PIPE_SUCCESS) {
    return sts;
  }
  int x = lld_subdev_read_register(dev, subdev, addr, mask);
  if (x) {
    *mask = 0;
    return PIPE_COMM_FAIL;
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_mc_tbl_ver_set(pipe_sess_hdl_t shdl,
                                      dev_target_t dev,
                                      int ver,
                                      bool rebuilding) {
  pipe_instr_write_reg_t instr;
  pipe_status_t sts = PIPE_SUCCESS;
  uint32_t addr[4] = {0};
  int inst_count = 0;
  /* Only take the api lock for normal APIs, as rebuilding will use the
   * exclusive lock instead.
   */
  if (!rebuilding) {
    sts = pipe_mgr_api_enter(shdl);
    if (sts != PIPE_SUCCESS) {
      return PIPE_SESSION_NOT_FOUND;
    }
  }

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev.device_id);
  if (dev_info == NULL) {
    LOG_ERROR("%s:%d Device info for device id %d not found",
              __func__,
              __LINE__,
              dev.device_id);
    sts = PIPE_OBJ_NOT_FOUND;
    goto done;
  }

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      inst_count = 1;
      sts = pipe_mgr_tof_mc_yid_tbl_addr_get(addr);
      break;

    case BF_DEV_FAMILY_TOFINO2:
      inst_count = 4;
      sts =
          pipe_mgr_tof2_mc_yid_tbl_addr_get(addr, addr + 1, addr + 2, addr + 3);
      break;

    case BF_DEV_FAMILY_TOFINO3:
      inst_count = 4;
      sts =
          pipe_mgr_tof3_mc_yid_tbl_addr_get(addr, addr + 1, addr + 2, addr + 3);
      break;

    default:
      PIPE_MGR_DBGCHK(0);
      sts = PIPE_UNEXPECTED;
      break;
  }
  if (sts != PIPE_SUCCESS) {
    goto done;
  }

  pipe_bitmap_t pipe_bit_map = {{0}};
  PIPE_BITMAP_INIT(&pipe_bit_map, PIPE_BMP_SIZE);
  uint32_t pipe_cnt = pipe_mgr_get_num_active_pipes(dev.device_id);
  uint32_t i;
  for (i = 0; i < pipe_cnt; ++i) PIPE_BITMAP_SET(&pipe_bit_map, i);

  uint32_t stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(addr[0]);

  for (i = 0; (int)i < inst_count; ++i) {
    construct_instr_reg_write(dev.device_id, &instr, addr[i], ver);
    sts = pipe_mgr_drv_ilist_add(&shdl,
                                 dev_info,
                                 &pipe_bit_map,
                                 stage,
                                 (uint8_t *)&instr,
                                 sizeof(instr));
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Failed to add tbl-ver-set to instruction list (%d)", sts);
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
      goto done;
    }
  }

  if (!pipe_mgr_is_device_locked(dev.device_id)) {
    sts = pipe_mgr_drv_ilist_push(&shdl, NULL, NULL);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Failed to push tbl-ver-set instruction list (%d)", sts);
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
      goto done;
    }
    sts = pipe_mgr_complete_operations(shdl);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Failed to complete tbl-ver-set instruction list (%d)", sts);
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
      goto done;
    }
  }

done:
  if (!rebuilding) {
    pipe_mgr_api_exit(shdl);
  }
  return sts;
}
