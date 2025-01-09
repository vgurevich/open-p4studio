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
#include <lld/lld_reg_if.h>
#include <lld/lld_dr_if.h>
#include <tofino_regs/pipe_top_level.h>
#include <tof2_regs/tof2_mem_drv.h>
#include <tof2_regs/tof2_mem_addr.h>
#include <tof3_regs/tof3_mem_drv.h>
#include <tof3_regs/tof3_mem_addr.h>
#include "mc_mgr.h"
#include "mc_mgr_int.h"
#include "mc_mgr_mem.h"

bf_status_t mc_mgr_get_bkup_port_reg(bf_dev_id_t dev,
                                     int ver,
                                     int port_bit_idx,
                                     int *bkup_bit_idx) {
  if (MC_MGR_INVALID_DEV(dev)) {
    LOG_ERROR("Invalid device %d at %s:%d", dev, __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  if (0 != ver && 1 != ver) {
    LOG_ERROR("Invalid version %d at %s:%d", ver, __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  if ((int)(BF_SUBDEV_PORT_COUNT * mc_mgr_ctx_num_subdevices(dev)) <=
          port_bit_idx ||
      0 > port_bit_idx) {
    LOG_ERROR(
        "Invalid port bit index %d at %s:%d", port_bit_idx, __func__, __LINE__);
    return BF_INVALID_ARG;
  }

  uint64_t addr = 0;
  uint64_t hi = 0, lo = 0;
  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      if (0 == ver) {
        addr = port_bit_idx *
                   pipe_top_level_tm_pre_pbt0_mem_word_array_element_size +
               pipe_top_level_tm_pre_pbt0_mem_word_address;
      } else {
        addr = port_bit_idx *
                   pipe_top_level_tm_pre_pbt1_mem_word_array_element_size +
               pipe_top_level_tm_pre_pbt1_mem_word_address;
      }
      break;
    case BF_DEV_FAMILY_TOFINO2:
      if (0 == ver) {
        addr = tof2_mem_tm_tm_pre_pre_common_mem_pbt0_mem_word(port_bit_idx);
      } else {
        addr = tof2_mem_tm_tm_pre_pre_common_mem_pbt1_mem_word(port_bit_idx);
      }
      break;
    case BF_DEV_FAMILY_TOFINO3:
      if ((int)(MC_MGR_TOF3_PORT_COUNT * mc_mgr_ctx_num_subdevices(dev)) <=
              port_bit_idx ||
          0 > port_bit_idx) {
        return BF_SUCCESS;
      }
      if (0 == ver) {
        addr = tof3_mem_tm_tm_pre_pre_common_mem_pbt0_mem_word(port_bit_idx);
      } else {
        addr = tof3_mem_tm_tm_pre_pre_common_mem_pbt1_mem_word(port_bit_idx);
      }
      break;
    default:
      MC_MGR_DBGCHK(0);
  }

  int x = lld_subdev_ind_read(dev, 0, addr / 16, &hi, &lo);
  if (x) {
    LOG_ERROR("Indirect reg read (%d) at %s:%d", x, __func__, __LINE__);
    *bkup_bit_idx = 0;
    return BF_HW_COMM_FAIL;
  }

  for (bf_subdev_id_t subdev = 1; subdev < (int)mc_mgr_ctx_num_subdevices(dev);
       ++subdev) {
    uint64_t temp_hi = 0, temp_low = 0;
    x = lld_subdev_ind_read(dev, subdev, addr / 16, &temp_hi, &temp_low);
    if (x) {
      LOG_ERROR("Indirect reg read (%d) at %s:%d", x, __func__, __LINE__);
      *bkup_bit_idx = 0;
      return BF_HW_COMM_FAIL;
    }
    if (temp_low != lo || temp_hi != hi) {
      LOG_ERROR("Indirect reg read for subdev (%d) match failed %s:%d",
                subdev,
                __func__,
                __LINE__);
      *bkup_bit_idx = 0;
      return BF_UNEXPECTED;
    }
    lo = temp_low;
    hi = temp_hi;
  }

  if (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO3) {
    uint32_t bpt_pipe, bpt_port;
    int dev_port;
    /* For TF3, backup port entry is defined as chip/pipe/port tuple,
     * (chip[8], pipe[7:6], port[5:0]) so we need to convert this to backup
     * port index */
    bpt_pipe = lo >> 6;
    bpt_port = lo & 0x3F;
    dev_port = mc_make_dev_port(dev, bpt_pipe, bpt_port);
    *bkup_bit_idx = mc_dev_port_to_bit_idx(dev, dev_port);
  } else {
    *bkup_bit_idx = lo;
  }

#ifdef UTEST
  /* Pull the data out of our software shadow and return it since there is no
   * hardware (or sw model) to read in the unit test environment. */
  *bkup_bit_idx = mc_mgr_ctx_bkup_port(dev, port_bit_idx);
#endif
  return BF_SUCCESS;
}

bf_status_t mc_mgr_set_bkup_port_wrl(
    int sid, bf_dev_id_t dev, int ver, int port_bit_idx, int bkup_bit_idx) {
  int mc_mgr_2die_factor = 1;
  if (MC_MGR_INVALID_DEV(dev)) {
    LOG_ERROR("Invalid device %d at %s:%d", dev, __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  if (0 != ver && 1 != ver) {
    LOG_ERROR("Invalid version %d at %s:%d", ver, __func__, __LINE__);
    return BF_INVALID_ARG;
  }
#if defined(EMU_2DIE_USING_SW_2DEV)
  mc_mgr_2die_factor = 2;
#else
  mc_mgr_2die_factor = 1;
#endif
  if ((int)(BF_SUBDEV_PORT_COUNT * mc_mgr_ctx_num_subdevices(dev) *
            mc_mgr_2die_factor) <= port_bit_idx ||
      0 > port_bit_idx) {
    LOG_ERROR(
        "Invalid port bit index %d at %s:%d", port_bit_idx, __func__, __LINE__);
    return BF_INVALID_ARG;
  }

  uint64_t addr = 0;
  uint64_t hi = 0, lo = bkup_bit_idx;
  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      if (0 == ver) {
        addr = port_bit_idx *
                   pipe_top_level_tm_pre_pbt0_mem_word_array_element_size +
               pipe_top_level_tm_pre_pbt0_mem_word_address;
      } else {
        addr = port_bit_idx *
                   pipe_top_level_tm_pre_pbt1_mem_word_array_element_size +
               pipe_top_level_tm_pre_pbt1_mem_word_address;
      }
      break;
    case BF_DEV_FAMILY_TOFINO2:
      if (0 == ver) {
        addr = tof2_mem_tm_tm_pre_pre_common_mem_pbt0_mem_word(port_bit_idx);
      } else {
        addr = tof2_mem_tm_tm_pre_pre_common_mem_pbt1_mem_word(port_bit_idx);
      }
      break;
    case BF_DEV_FAMILY_TOFINO3:
      if ((int)(MC_MGR_TOF3_PORT_COUNT * mc_mgr_ctx_num_subdevices(dev) *
                mc_mgr_2die_factor) <= port_bit_idx ||
          0 > port_bit_idx) {
        return BF_SUCCESS;
      }
      if (0 == ver) {
        addr = tof3_mem_tm_tm_pre_pre_common_mem_pbt0_mem_word(port_bit_idx);
      } else {
        addr = tof3_mem_tm_tm_pre_pre_common_mem_pbt1_mem_word(port_bit_idx);
      }
      /* For TF3, BPT entry shall be specified as chip/pipe/port bitfield,
       * so we need to convert backup port index to 9-bit PBT value built as:
       * chip[8], pipe[7:6], port[5:0] */
      uint32_t bpt_val, bpt_pipe, bpt_port;
      int dev_port;
      dev_port = mc_bit_idx_to_dev_port(dev, bkup_bit_idx);

      /* For 3-bit pipe number, most significant bit encodes chip # (0..1),
       * remaining 2-bits encode local pipe number (0..3). */
      bpt_pipe = mc_dev_port_to_pipe(dev, dev_port);
      bpt_port = mc_dev_port_to_local_port(dev, dev_port);
      bpt_val = bpt_port | (bpt_pipe << 6);
      lo = bpt_val;
      break;
    default:
      MC_MGR_DBGCHK(0);
  }

  if (mc_mgr_drv_wrl_append(dev,
                            MC_MGR_DRV_SUBDEV_ID_ALL,
                            sid,
                            /*cmplt,*/ 16,
                            addr,
                            hi,
                            lo,
                            __func__,
                            __LINE__)) {
    MC_MGR_DBGCHK(0);
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t mc_mgr_get_lit_np_reg(
    bf_dev_id_t dev, int ver, int id, int *l_cnt, int *r_cnt) {
  if (MC_MGR_INVALID_DEV(dev)) {
    LOG_ERROR("Read LIT NP fails, invalid device (dev %d ver %d id %d at %s:%d",
              dev,
              ver,
              id,
              __func__,
              __LINE__);
    return BF_INVALID_ARG;
  }
  if (0 != ver && 1 != ver) {
    LOG_ERROR(
        "Read LIT NP fails, invalid version (dev %d ver %d id %d at %s:%d",
        dev,
        ver,
        id,
        __func__,
        __LINE__);
    return BF_INVALID_ARG;
  }
  if (0 > id && BF_LAG_COUNT <= id) {
    LOG_ERROR("Read LIT NP fails, invalid LAG Id (dev %d ver %d id %d at %s:%d",
              dev,
              ver,
              id,
              __func__,
              __LINE__);
    return BF_INVALID_ARG;
  }
  if (!l_cnt || !r_cnt) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  uint64_t addr = 0;
  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      if (!ver)
        addr = id * pipe_top_level_tm_pre_lit0_np_mem_word_array_element_size +
               pipe_top_level_tm_pre_lit0_np_mem_word_address;
      else
        addr = id * pipe_top_level_tm_pre_lit1_np_mem_word_array_element_size +
               pipe_top_level_tm_pre_lit1_np_mem_word_address;
      break;
    case BF_DEV_FAMILY_TOFINO2:
      if (!ver)
        addr = tof2_mem_tm_tm_pre_pre_common_mem_lit0_np_mem_word(id);
      else
        addr = tof2_mem_tm_tm_pre_pre_common_mem_lit1_np_mem_word(id);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      if (!ver)
        addr = tof3_mem_tm_tm_pre_pre_common_mem_lit0_np_mem_word(id);
      else
        addr = tof3_mem_tm_tm_pre_pre_common_mem_lit1_np_mem_word(id);
      break;
    default:
      MC_MGR_DBGCHK(0);
  }

  uint64_t data0 = 0, data1 = 0;
  int x = lld_subdev_ind_read(dev, 0, addr / 16, &data1, &data0);
  if (x) {
    LOG_ERROR("Read LIT NP fails, indirect reg read (%d) at %s:%d",
              x,
              __func__,
              __LINE__);
    return BF_HW_COMM_FAIL;
  }

  for (bf_subdev_id_t subdev = 1; subdev < (int)mc_mgr_ctx_num_subdevices(dev);
       ++subdev) {
    uint64_t temp_hi = 0, temp_low = 0;
    x = lld_subdev_ind_read(dev, subdev, addr / 16, &temp_hi, &temp_low);
    if (x) {
      LOG_ERROR("Indirect reg read (%d) at %s:%d", x, __func__, __LINE__);
      return BF_HW_COMM_FAIL;
    }
    if (temp_low != data0 || temp_hi != data1) {
      LOG_ERROR("Indirect reg read for subdev (%d) match failed %s:%d",
                subdev,
                __func__,
                __LINE__);
      return BF_UNEXPECTED;
    }
    data0 = temp_low;
    data1 = temp_hi;
  }

  *l_cnt = data0 >> 13;
  *r_cnt = data0 & 0x1FFF;
#ifdef UTEST
  /* Pull the data out of our software shadow and return it since there is no
   * hardware (or sw model) to read in the unit test environment. */
  *l_cnt = mc_mgr_ctx_lit_np_l(dev, id);
  *r_cnt = mc_mgr_ctx_lit_np_l(dev, id);
#endif

  return BF_SUCCESS;
}
bf_status_t mc_mgr_set_lit_np_wrl(int sid, bf_dev_id_t dev, int ver, int id) {
  if (MC_MGR_INVALID_DEV(dev)) {
    LOG_ERROR(
        "Write LIT NP fails, invalid device (dev %d ver %d id %d at %s:%d",
        dev,
        ver,
        id,
        __func__,
        __LINE__);
    return BF_INVALID_ARG;
  }
  if (0 != ver && 1 != ver) {
    LOG_ERROR(
        "Write LIT NP fails, invalid version (dev %d ver %d id %d at %s:%d",
        dev,
        ver,
        id,
        __func__,
        __LINE__);
    return BF_INVALID_ARG;
  }
  if (0 > id && BF_LAG_COUNT <= id) {
    LOG_ERROR(
        "Write LIT NP fails, invalid LAG Id (dev %d ver %d id %d at %s:%d",
        dev,
        ver,
        id,
        __func__,
        __LINE__);
    return BF_INVALID_ARG;
  }

  uint64_t l_cnt = mc_mgr_ctx_lit_np_l(dev, id);
  uint64_t r_cnt = mc_mgr_ctx_lit_np_r(dev, id);
  uint64_t hi = 0, lo = 0;
  lo = (l_cnt << 13) | r_cnt;

  uint64_t addr = 0;
  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      if (!ver)
        addr = id * pipe_top_level_tm_pre_lit0_np_mem_word_array_element_size +
               pipe_top_level_tm_pre_lit0_np_mem_word_address;
      else
        addr = id * pipe_top_level_tm_pre_lit1_np_mem_word_array_element_size +
               pipe_top_level_tm_pre_lit1_np_mem_word_address;
      break;
    case BF_DEV_FAMILY_TOFINO2:
      if (!ver)
        addr = tof2_mem_tm_tm_pre_pre_common_mem_lit0_np_mem_word(id);
      else
        addr = tof2_mem_tm_tm_pre_pre_common_mem_lit1_np_mem_word(id);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      if (!ver)
        addr = tof3_mem_tm_tm_pre_pre_common_mem_lit0_np_mem_word(id);
      else
        addr = tof3_mem_tm_tm_pre_pre_common_mem_lit1_np_mem_word(id);
      break;
    default:
      MC_MGR_DBGCHK(0);
  }

  if (mc_mgr_drv_wrl_append(dev,
                            MC_MGR_DRV_SUBDEV_ID_ALL,
                            sid,
                            16,
                            addr,
                            hi,
                            lo,
                            __func__,
                            __LINE__)) {
    LOG_ERROR("Failed to add LIT%d NP update to WRL from %s:%d",
              ver,
              __func__,
              __LINE__);
    MC_MGR_DBGCHK(0);
    return BF_NO_SYS_RESOURCES;
  }

  return BF_SUCCESS;
}

bf_status_t mc_mgr_get_lit_seg_reg(
    bf_dev_id_t dev, int ver, int id, int seg, bf_bitset_t *val) {
  if (MC_MGR_INVALID_DEV(dev)) {
    LOG_ERROR(
        "Read LIT fails, invalid device (dev %d ver %d id %d seg %d at %s:%d",
        dev,
        ver,
        id,
        seg,
        __func__,
        __LINE__);
    return BF_INVALID_ARG;
  }
  if (0 != ver && 1 != ver) {
    LOG_ERROR(
        "Read LIT fails, invalid version (dev %d ver %d id %d seg %d at %s:%d",
        dev,
        ver,
        id,
        seg,
        __func__,
        __LINE__);
    return BF_INVALID_ARG;
  }
  if (0 > id || BF_LAG_COUNT <= id) {
    LOG_ERROR(
        "Read LIT fails, invalid LAG Id (dev %d ver %d id %d seg %d at %s:%d",
        dev,
        ver,
        id,
        seg,
        __func__,
        __LINE__);
    return BF_INVALID_ARG;
  }
  if (0 > seg || (int)(4 * mc_mgr_ctx_num_subdevices(dev)) <= seg) {
    LOG_ERROR(
        "Read LIT fails, invalid segment (dev %d ver %d id %d seg %d at %s:%d",
        dev,
        ver,
        id,
        seg,
        __func__,
        __LINE__);
    return BF_INVALID_ARG;
  }
  if (!val) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  uint64_t addr = 0;
  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      addr =
          id * pipe_top_level_tm_pre_lit0_bm_mem_word0_array_element_size +
          ((ver && 0 == seg)
               ? pipe_top_level_tm_pre_lit1_bm_mem_word0_address
               : (ver && 1 == seg)
                     ? pipe_top_level_tm_pre_lit1_bm_mem_word1_address
                     : (ver && 2 == seg)
                           ? pipe_top_level_tm_pre_lit1_bm_mem_word2_address
                           : (ver)
                                 ? pipe_top_level_tm_pre_lit1_bm_mem_word3_address
                                 : (!ver && 0 == seg)
                                       ? pipe_top_level_tm_pre_lit0_bm_mem_word0_address
                                       : (!ver && 1 == seg)
                                             ? pipe_top_level_tm_pre_lit0_bm_mem_word1_address
                                             : (!ver && 2 == seg)
                                                   ? pipe_top_level_tm_pre_lit0_bm_mem_word2_address
                                                   : pipe_top_level_tm_pre_lit0_bm_mem_word3_address);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      addr =
          ((ver && 0 == seg)
               ? tof2_mem_tm_tm_pre_pre_common_mem_lit1_bm_mem_word0(id)
               : (ver && 1 == seg)
                     ? tof2_mem_tm_tm_pre_pre_common_mem_lit1_bm_mem_word1(id)
                     : (ver && 2 == seg)
                           ? tof2_mem_tm_tm_pre_pre_common_mem_lit1_bm_mem_word2(
                                 id)
                           : (ver)
                                 ? tof2_mem_tm_tm_pre_pre_common_mem_lit1_bm_mem_word3(
                                       id)
                                 : (!ver && 0 == seg)
                                       ? tof2_mem_tm_tm_pre_pre_common_mem_lit0_bm_mem_word0(
                                             id)
                                       : (!ver && 1 == seg)
                                             ? tof2_mem_tm_tm_pre_pre_common_mem_lit0_bm_mem_word1(
                                                   id)
                                             : (!ver && 2 == seg)
                                                   ? tof2_mem_tm_tm_pre_pre_common_mem_lit0_bm_mem_word2(
                                                         id)
                                                   : tof2_mem_tm_tm_pre_pre_common_mem_lit0_bm_mem_word3(
                                                         id));
      break;
    case BF_DEV_FAMILY_TOFINO3:
      if (ver) {
        switch (seg) {
          case 0:
            addr = tof3_mem_tm_tm_pre_pre_common_mem_lit1_bm_mem_word0(id);
            break;
          case 1:
            addr = tof3_mem_tm_tm_pre_pre_common_mem_lit1_bm_mem_word1(id);
            break;
          case 2:
            addr = tof3_mem_tm_tm_pre_pre_common_mem_lit1_bm_mem_word2(id);
            break;
          case 3:
            addr = tof3_mem_tm_tm_pre_pre_common_mem_lit1_bm_mem_word3(id);
            break;
          case 4:
            addr = tof3_mem_tm_tm_pre_pre_common_mem_lit1_bm_mem_word4(id);
            break;
          case 5:
            addr = tof3_mem_tm_tm_pre_pre_common_mem_lit1_bm_mem_word5(id);
            break;
          case 6:
            addr = tof3_mem_tm_tm_pre_pre_common_mem_lit1_bm_mem_word6(id);
            break;
          case 7:
            addr = tof3_mem_tm_tm_pre_pre_common_mem_lit1_bm_mem_word7(id);
            break;
          default:
            MC_MGR_DBGCHK(0);
            break;
        }
      } else {
        switch (seg) {
          case 0:
            addr = tof3_mem_tm_tm_pre_pre_common_mem_lit0_bm_mem_word0(id);
            break;
          case 1:
            addr = tof3_mem_tm_tm_pre_pre_common_mem_lit0_bm_mem_word1(id);
            break;
          case 2:
            addr = tof3_mem_tm_tm_pre_pre_common_mem_lit0_bm_mem_word2(id);
            break;
          case 3:
            addr = tof3_mem_tm_tm_pre_pre_common_mem_lit0_bm_mem_word3(id);
            break;
          case 4:
            addr = tof3_mem_tm_tm_pre_pre_common_mem_lit0_bm_mem_word4(id);
            break;
          case 5:
            addr = tof3_mem_tm_tm_pre_pre_common_mem_lit0_bm_mem_word5(id);
            break;
          case 6:
            addr = tof3_mem_tm_tm_pre_pre_common_mem_lit0_bm_mem_word6(id);
            break;
          case 7:
            addr = tof3_mem_tm_tm_pre_pre_common_mem_lit0_bm_mem_word7(id);
            break;
          default:
            MC_MGR_DBGCHK(0);
            break;
        }
      }

      break;
    default:
      MC_MGR_DBGCHK(0);
  }

  uint64_t data0 = 0, data1 = 0;
  int x = lld_subdev_ind_read(dev, 0, addr / 16, &data1, &data0);
  if (x) {
    LOG_ERROR("Read LIT fails, indirect reg read (%d) at %s:%d",
              x,
              __func__,
              __LINE__);
    return BF_HW_COMM_FAIL;
  }
  for (bf_subdev_id_t subdev = 1; subdev < (int)mc_mgr_ctx_num_subdevices(dev);
       ++subdev) {
    uint64_t temp_hi = 0, temp_low = 0;
    x = lld_subdev_ind_read(dev, subdev, addr / 16, &temp_hi, &temp_low);
    if (x) {
      LOG_ERROR("Indirect reg read (%d) at %s:%d", x, __func__, __LINE__);
      return BF_HW_COMM_FAIL;
    }
    if (temp_low != data0 || temp_hi != data1) {
      LOG_ERROR("Indirect reg read for subdev (%d) match failed %s:%d",
                subdev,
                __func__,
                __LINE__);
      return BF_UNEXPECTED;
    }
    data0 = temp_low;
    data1 = temp_hi;
  }
  bf_bs_set_word(val, 0, 64, data0);
  bf_bs_set_word(val, 64, 8, data1);
#ifdef UTEST
  /* Pull the data out of our software shadow and return it since there is no
   * hardware (or sw model) to read in the unit test environment. */
  bf_bs_set_word(
      val, 0, 64, bf_bs_get_word(mc_mgr_ctx_lit(dev, id, seg), 0, 64));
  bf_bs_set_word(
      val, 64, 8, bf_bs_get_word(mc_mgr_ctx_lit(dev, id, seg), 64, 8));
#endif

  return BF_SUCCESS;
}

bf_status_t mc_mgr_set_lit_wrl(int sid, bf_dev_id_t dev, int ver, int lag_id) {
  uint64_t addr = 0;
  uint64_t hi = 0, lo = 0;

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

  if (lag_id < 0 || lag_id >= BF_LAG_COUNT) {
    MC_MGR_DBGCHK(0);
    return 0;
  }

  /* First setup the number of LIT segments based on the chip type. */
  int num_lit_segments = 4;
  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
    case BF_DEV_FAMILY_TOFINO2:
      /* Tofino-1 and Tofino-2 use the default of 4 LIT segments. */
      break;
    case BF_DEV_FAMILY_TOFINO3:
      /* Tofino-3 has 8 LIT segments regardless of the number of subdevices. */
      num_lit_segments = 8;

    default:
      break;
  }

  for (int i = 0; i < num_lit_segments; ++i) {
    switch (mc_mgr_ctx_dev_family(dev)) {
      case BF_DEV_FAMILY_TOFINO:
        lo = bf_bs_get_word(mc_mgr_ctx_lit(dev, lag_id, i), 0, 64);
        hi = bf_bs_get_word(mc_mgr_ctx_lit(dev, lag_id, i), 64, 64);

        if (!ver) {
          addr =
              lag_id *
                  pipe_top_level_tm_pre_lit0_bm_mem_word0_array_element_size +
              ((0 == i)
                   ? pipe_top_level_tm_pre_lit0_bm_mem_word0_address
                   : (1 == i)
                         ? pipe_top_level_tm_pre_lit0_bm_mem_word1_address
                         : (2 == i)
                               ? pipe_top_level_tm_pre_lit0_bm_mem_word2_address
                               : pipe_top_level_tm_pre_lit0_bm_mem_word3_address);
        } else {
          addr =
              lag_id *
                  pipe_top_level_tm_pre_lit1_bm_mem_word0_array_element_size +
              ((0 == i)
                   ? pipe_top_level_tm_pre_lit1_bm_mem_word0_address
                   : (1 == i)
                         ? pipe_top_level_tm_pre_lit1_bm_mem_word1_address
                         : (2 == i)
                               ? pipe_top_level_tm_pre_lit1_bm_mem_word2_address
                               : pipe_top_level_tm_pre_lit1_bm_mem_word3_address);
        }
        break;
      case BF_DEV_FAMILY_TOFINO2:
        lo = bf_bs_get_word(mc_mgr_ctx_lit(dev, lag_id, i), 0, 64);
        hi = bf_bs_get_word(mc_mgr_ctx_lit(dev, lag_id, i), 64, 64);
        if (!ver) {
          addr =
              ((0 == i)
                   ? tof2_mem_tm_tm_pre_pre_common_mem_lit0_bm_mem_word0(lag_id)
                   : (1 == i)
                         ? tof2_mem_tm_tm_pre_pre_common_mem_lit0_bm_mem_word1(
                               lag_id)
                         : (2 == i)
                               ? tof2_mem_tm_tm_pre_pre_common_mem_lit0_bm_mem_word2(
                                     lag_id)
                               : tof2_mem_tm_tm_pre_pre_common_mem_lit0_bm_mem_word3(
                                     lag_id));
        } else {
          addr =
              ((0 == i)
                   ? tof2_mem_tm_tm_pre_pre_common_mem_lit1_bm_mem_word0(lag_id)
                   : (1 == i)
                         ? tof2_mem_tm_tm_pre_pre_common_mem_lit1_bm_mem_word1(
                               lag_id)
                         : (2 == i)
                               ? tof2_mem_tm_tm_pre_pre_common_mem_lit1_bm_mem_word2(
                                     lag_id)
                               : tof2_mem_tm_tm_pre_pre_common_mem_lit1_bm_mem_word3(
                                     lag_id));
        }
        break;
      case BF_DEV_FAMILY_TOFINO3:
        lo = bf_bs_get_word(mc_mgr_ctx_lit(dev, lag_id, i), 0, 36);
        if (ver) {
          switch (i) {
            case 0:
              addr =
                  tof3_mem_tm_tm_pre_pre_common_mem_lit1_bm_mem_word0(lag_id);
              break;
            case 1:
              addr =
                  tof3_mem_tm_tm_pre_pre_common_mem_lit1_bm_mem_word1(lag_id);
              break;
            case 2:
              addr =
                  tof3_mem_tm_tm_pre_pre_common_mem_lit1_bm_mem_word2(lag_id);
              break;
            case 3:
              addr =
                  tof3_mem_tm_tm_pre_pre_common_mem_lit1_bm_mem_word3(lag_id);
              break;
            case 4:
              addr =
                  tof3_mem_tm_tm_pre_pre_common_mem_lit1_bm_mem_word4(lag_id);
              break;
            case 5:
              addr =
                  tof3_mem_tm_tm_pre_pre_common_mem_lit1_bm_mem_word5(lag_id);
              break;
            case 6:
              addr =
                  tof3_mem_tm_tm_pre_pre_common_mem_lit1_bm_mem_word6(lag_id);
              break;
            case 7:
              addr =
                  tof3_mem_tm_tm_pre_pre_common_mem_lit1_bm_mem_word7(lag_id);
              break;
            default:
              MC_MGR_DBGCHK(0);
              break;
          }
        } else {
          switch (i) {
            case 0:
              addr =
                  tof3_mem_tm_tm_pre_pre_common_mem_lit0_bm_mem_word0(lag_id);
              break;
            case 1:
              addr =
                  tof3_mem_tm_tm_pre_pre_common_mem_lit0_bm_mem_word1(lag_id);
              break;
            case 2:
              addr =
                  tof3_mem_tm_tm_pre_pre_common_mem_lit0_bm_mem_word2(lag_id);
              break;
            case 3:
              addr =
                  tof3_mem_tm_tm_pre_pre_common_mem_lit0_bm_mem_word3(lag_id);
              break;
            case 4:
              addr =
                  tof3_mem_tm_tm_pre_pre_common_mem_lit0_bm_mem_word4(lag_id);
              break;
            case 5:
              addr =
                  tof3_mem_tm_tm_pre_pre_common_mem_lit0_bm_mem_word5(lag_id);
              break;
            case 6:
              addr =
                  tof3_mem_tm_tm_pre_pre_common_mem_lit0_bm_mem_word6(lag_id);
              break;
            case 7:
              addr =
                  tof3_mem_tm_tm_pre_pre_common_mem_lit0_bm_mem_word7(lag_id);
              break;
            default:
              MC_MGR_DBGCHK(0);
              break;
          }
        }

        break;
      default:
        MC_MGR_DBGCHK(0);
    }
    if (mc_mgr_drv_wrl_append(dev,
                              MC_MGR_DRV_SUBDEV_ID_ALL,
                              sid,
                              16,
                              addr,
                              hi,
                              lo,
                              __func__,
                              __LINE__)) {
      LOG_ERROR("Failed to add LIT%d update to WRL from %s:%d",
                ver,
                __func__,
                __LINE__);
      MC_MGR_DBGCHK(0);
      return BF_NO_SYS_RESOURCES;
    }
  }

  return BF_SUCCESS;
}

bf_status_t mc_mgr_get_pmt_seg_reg(
    bf_dev_id_t dev, int ver, int yid, int seg, bf_bitset_t *val) {
  if (0 != ver && 1 != ver) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (0 > dev || MC_MGR_NUM_DEVICES <= dev) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (yid >= (int)(mc_mgr_ctx_pmt_size(dev))) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  int num_of_mems = 0;
  if (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO3) {
    num_of_mems = 8;
  } else {
    num_of_mems = 4;
  }

  if (seg >= num_of_mems) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (!val) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  uint64_t addr = 0;
  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      addr =
          yid * pipe_top_level_tm_pre_pmt0_mem_word0_array_element_size +
          ((ver && 0 == seg)
               ? pipe_top_level_tm_pre_pmt1_mem_word0_address
               : (ver && 1 == seg)
                     ? pipe_top_level_tm_pre_pmt1_mem_word1_address
                     : (ver && 2 == seg)
                           ? pipe_top_level_tm_pre_pmt1_mem_word2_address
                           : (ver)
                                 ? pipe_top_level_tm_pre_pmt1_mem_word3_address
                                 : (!ver && 0 == seg)
                                       ? pipe_top_level_tm_pre_pmt0_mem_word0_address
                                       : (!ver && 1 == seg)
                                             ? pipe_top_level_tm_pre_pmt0_mem_word1_address
                                             : (!ver && 2 == seg)
                                                   ? pipe_top_level_tm_pre_pmt0_mem_word2_address
                                                   : pipe_top_level_tm_pre_pmt0_mem_word3_address);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      addr =
          ((ver && 0 == seg)
               ? tof2_mem_tm_tm_pre_pre_common_mem_pmt1_mem_word0(yid)
               : (ver && 1 == seg)
                     ? tof2_mem_tm_tm_pre_pre_common_mem_pmt1_mem_word1(yid)
                     : (ver && 2 == seg)
                           ? tof2_mem_tm_tm_pre_pre_common_mem_pmt1_mem_word2(
                                 yid)
                           : (ver)
                                 ? tof2_mem_tm_tm_pre_pre_common_mem_pmt1_mem_word3(
                                       yid)
                                 : (!ver && 0 == seg)
                                       ? tof2_mem_tm_tm_pre_pre_common_mem_pmt0_mem_word0(
                                             yid)
                                       : (!ver && 1 == seg)
                                             ? tof2_mem_tm_tm_pre_pre_common_mem_pmt0_mem_word1(
                                                   yid)
                                             : (!ver && 2 == seg)
                                                   ? tof2_mem_tm_tm_pre_pre_common_mem_pmt0_mem_word2(
                                                         yid)
                                                   : tof2_mem_tm_tm_pre_pre_common_mem_pmt0_mem_word3(
                                                         yid));
      break;
    case BF_DEV_FAMILY_TOFINO3:
      if (ver) {
        switch (seg) {
          case 0:
            addr = tof3_mem_tm_tm_pre_pre_common_mem_pmt1_mem_word0(yid);
            break;
          case 1:
            addr = tof3_mem_tm_tm_pre_pre_common_mem_pmt1_mem_word1(yid);
            break;
          case 2:
            addr = tof3_mem_tm_tm_pre_pre_common_mem_pmt1_mem_word2(yid);
            break;
          case 3:
            addr = tof3_mem_tm_tm_pre_pre_common_mem_pmt1_mem_word3(yid);
            break;
          case 4:
            addr = tof3_mem_tm_tm_pre_pre_common_mem_pmt1_mem_word4(yid);
            break;
          case 5:
            addr = tof3_mem_tm_tm_pre_pre_common_mem_pmt1_mem_word5(yid);
            break;
          case 6:
            addr = tof3_mem_tm_tm_pre_pre_common_mem_pmt1_mem_word6(yid);
            break;
          case 7:
            addr = tof3_mem_tm_tm_pre_pre_common_mem_pmt1_mem_word7(yid);
            break;
          default:
            MC_MGR_DBGCHK(0);
            break;
        }
      } else {
        switch (seg) {
          case 0:
            addr = tof3_mem_tm_tm_pre_pre_common_mem_pmt0_mem_word0(yid);
            break;
          case 1:
            addr = tof3_mem_tm_tm_pre_pre_common_mem_pmt0_mem_word1(yid);
            break;
          case 2:
            addr = tof3_mem_tm_tm_pre_pre_common_mem_pmt0_mem_word2(yid);
            break;
          case 3:
            addr = tof3_mem_tm_tm_pre_pre_common_mem_pmt0_mem_word3(yid);
            break;
          case 4:
            addr = tof3_mem_tm_tm_pre_pre_common_mem_pmt0_mem_word4(yid);
            break;
          case 5:
            addr = tof3_mem_tm_tm_pre_pre_common_mem_pmt0_mem_word5(yid);
            break;
          case 6:
            addr = tof3_mem_tm_tm_pre_pre_common_mem_pmt0_mem_word6(yid);
            break;
          case 7:
            addr = tof3_mem_tm_tm_pre_pre_common_mem_pmt0_mem_word7(yid);
            break;
          default:
            MC_MGR_DBGCHK(0);
            break;
        }
      }

      break;
    default:
      MC_MGR_DBGCHK(0);
  }

  uint64_t data0 = 0, data1 = 0;
  int x = lld_subdev_ind_read(dev, 0, addr / 16, &data1, &data0);
  if (x) {
    LOG_ERROR("Read PMT fails, indirect reg read (%d) at %s:%d",
              x,
              __func__,
              __LINE__);
    MC_MGR_DBGCHK(!x);
    return BF_HW_COMM_FAIL;
  }
  for (bf_subdev_id_t subdev = 1; subdev < (int)mc_mgr_ctx_num_subdevices(dev);
       ++subdev) {
    uint64_t temp_hi = 0, temp_low = 0;
    x = lld_subdev_ind_read(dev, subdev, addr / 16, &temp_hi, &temp_low);
    if (x) {
      LOG_ERROR("Indirect reg read (%d) at %s:%d", x, __func__, __LINE__);
      return BF_HW_COMM_FAIL;
    }
    if (temp_low != data0 || temp_hi != data1) {
      LOG_ERROR("Indirect reg read for subdev (%d) match failed %s:%d",
                subdev,
                __func__,
                __LINE__);
      return BF_UNEXPECTED;
    }
    data0 = temp_low;
    data1 = temp_hi;
  }
  bf_bs_set_word(val, 0, 64, data0);
  bf_bs_set_word(val, 64, 8, data1);
#ifdef UTEST
  /* Pull the data out of our software shadow and return it since there is no
   * hardware (or sw model) to read in the unit test environment. */
  int b = seg * 72;
  bf_bs_set_word(val, 0, 64, bf_bs_get_word(mc_mgr_ctx_pmt(dev, yid), b, 64));
  bf_bs_set_word(
      val, 64, 8, bf_bs_get_word(mc_mgr_ctx_pmt(dev, yid), b + 64, 8));
#endif

  return BF_SUCCESS;
}

bf_status_t mc_mgr_set_pmt_wrl(int sid, bf_dev_id_t dev, int ver, int yid) {
  if (0 != ver && 1 != ver) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (0 > sid || MC_MGR_NUM_SESSIONS <= sid) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (0 > dev || MC_MGR_NUM_DEVICES <= dev) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (yid >= (int)(mc_mgr_ctx_pmt_size(dev))) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  int i, num_of_mems = 0;
  if (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO3) {
    num_of_mems = 8;
  } else {
    num_of_mems = 4;
  }

  for (i = 0; i < num_of_mems; ++i) {
    uint64_t addr = 0, lo = 0, hi = 0;
    switch (mc_mgr_ctx_dev_family(dev)) {
      case BF_DEV_FAMILY_TOFINO:
        lo = bf_bs_get_word(
            mc_mgr_ctx_pmt(dev, yid), BF_PIPE_PORT_COUNT * i, 64);
        hi = bf_bs_get_word(
            mc_mgr_ctx_pmt(dev, yid), BF_PIPE_PORT_COUNT * i + 64, 8);
        if (ver) {
          addr =
              yid * pipe_top_level_tm_pre_pmt1_mem_word0_array_element_size +
              ((0 == i)
                   ? pipe_top_level_tm_pre_pmt1_mem_word0_address
                   : (1 == i)
                         ? pipe_top_level_tm_pre_pmt1_mem_word1_address
                         : (2 == i)
                               ? pipe_top_level_tm_pre_pmt1_mem_word2_address
                               : pipe_top_level_tm_pre_pmt1_mem_word3_address);
        } else {
          addr =
              yid * pipe_top_level_tm_pre_pmt0_mem_word0_array_element_size +
              ((0 == i)
                   ? pipe_top_level_tm_pre_pmt0_mem_word0_address
                   : (1 == i)
                         ? pipe_top_level_tm_pre_pmt0_mem_word1_address
                         : (2 == i)
                               ? pipe_top_level_tm_pre_pmt0_mem_word2_address
                               : pipe_top_level_tm_pre_pmt0_mem_word3_address);
        }
        break;
      case BF_DEV_FAMILY_TOFINO2:
        lo = bf_bs_get_word(
            mc_mgr_ctx_pmt(dev, yid), BF_PIPE_PORT_COUNT * i, 64);
        hi = bf_bs_get_word(
            mc_mgr_ctx_pmt(dev, yid), BF_PIPE_PORT_COUNT * i + 64, 8);
        if (ver) {
          addr =
              ((0 == i)
                   ? tof2_mem_tm_tm_pre_pre_common_mem_pmt1_mem_word0(yid)
                   : (1 == i)
                         ? tof2_mem_tm_tm_pre_pre_common_mem_pmt1_mem_word1(yid)
                         : (2 == i)
                               ? tof2_mem_tm_tm_pre_pre_common_mem_pmt1_mem_word2(
                                     yid)
                               : tof2_mem_tm_tm_pre_pre_common_mem_pmt1_mem_word3(
                                     yid));
        } else {
          addr =
              ((0 == i)
                   ? tof2_mem_tm_tm_pre_pre_common_mem_pmt0_mem_word0(yid)
                   : (1 == i)
                         ? tof2_mem_tm_tm_pre_pre_common_mem_pmt0_mem_word1(yid)
                         : (2 == i)
                               ? tof2_mem_tm_tm_pre_pre_common_mem_pmt0_mem_word2(
                                     yid)
                               : tof2_mem_tm_tm_pre_pre_common_mem_pmt0_mem_word3(
                                     yid));
        }
        break;
      case BF_DEV_FAMILY_TOFINO3:
        lo = bf_bs_get_word(
            mc_mgr_ctx_pmt(dev, yid), BF_PIPE_PORT_COUNT * i, 64);
        hi = bf_bs_get_word(
            mc_mgr_ctx_pmt(dev, yid), BF_PIPE_PORT_COUNT * i + 64, 8);
        if (ver) {
          switch (i) {
            case 0:
              addr = tof3_mem_tm_tm_pre_pre_common_mem_pmt1_mem_word0(yid);
              break;
            case 1:
              addr = tof3_mem_tm_tm_pre_pre_common_mem_pmt1_mem_word1(yid);
              break;
            case 2:
              addr = tof3_mem_tm_tm_pre_pre_common_mem_pmt1_mem_word2(yid);
              break;
            case 3:
              addr = tof3_mem_tm_tm_pre_pre_common_mem_pmt1_mem_word3(yid);
              break;
            case 4:
              addr = tof3_mem_tm_tm_pre_pre_common_mem_pmt1_mem_word4(yid);
              break;
            case 5:
              addr = tof3_mem_tm_tm_pre_pre_common_mem_pmt1_mem_word5(yid);
              break;
            case 6:
              addr = tof3_mem_tm_tm_pre_pre_common_mem_pmt1_mem_word6(yid);
              break;
            case 7:
              addr = tof3_mem_tm_tm_pre_pre_common_mem_pmt1_mem_word7(yid);
              break;
            default:
              MC_MGR_DBGCHK(0);
              break;
          }
        } else {
          switch (i) {
            case 0:
              addr = tof3_mem_tm_tm_pre_pre_common_mem_pmt0_mem_word0(yid);
              break;
            case 1:
              addr = tof3_mem_tm_tm_pre_pre_common_mem_pmt0_mem_word1(yid);
              break;
            case 2:
              addr = tof3_mem_tm_tm_pre_pre_common_mem_pmt0_mem_word2(yid);
              break;
            case 3:
              addr = tof3_mem_tm_tm_pre_pre_common_mem_pmt0_mem_word3(yid);
              break;
            case 4:
              addr = tof3_mem_tm_tm_pre_pre_common_mem_pmt0_mem_word4(yid);
              break;
            case 5:
              addr = tof3_mem_tm_tm_pre_pre_common_mem_pmt0_mem_word5(yid);
              break;
            case 6:
              addr = tof3_mem_tm_tm_pre_pre_common_mem_pmt0_mem_word6(yid);
              break;
            case 7:
              addr = tof3_mem_tm_tm_pre_pre_common_mem_pmt0_mem_word7(yid);
              break;
            default:
              MC_MGR_DBGCHK(0);
              break;
          }
        }
        break;
      default:
        MC_MGR_DBGCHK(0);
    }
    if (mc_mgr_drv_wrl_append(dev,
                              MC_MGR_DRV_SUBDEV_ID_ALL,
                              sid,
                              16,
                              addr,
                              hi,
                              lo,
                              __func__,
                              __LINE__)) {
      LOG_ERROR("Failed to add PMT0.%d update to WRL from %s:%d",
                i,
                __func__,
                __LINE__);
      MC_MGR_DBGCHK(0);
      return BF_NO_SYS_RESOURCES;
    }
  }

  /*LOG_TRACE("PMT[%d][%3d] = 0x%02" PRIx64 " %016" PRIx64 " 0x%02" PRIx64
            " %016" PRIx64 " 0x%02" PRIx64 " %016" PRIx64 " 0x%02" PRIx64
            " %016" PRIx64 "",
            dev,
            yid,
            bf_bs_get_word(mc_mgr_ctx_pmt(dev, yid), 72 * 3 + 64, 8),
            bf_bs_get_word(mc_mgr_ctx_pmt(dev, yid), 72 * 3, 64),
            bf_bs_get_word(mc_mgr_ctx_pmt(dev, yid), 72 * 2 + 64, 8),
            bf_bs_get_word(mc_mgr_ctx_pmt(dev, yid), 72 * 2, 64),
            bf_bs_get_word(mc_mgr_ctx_pmt(dev, yid), 72 * 1 + 64, 8),
            bf_bs_get_word(mc_mgr_ctx_pmt(dev, yid), 72 * 1, 64),
            bf_bs_get_word(mc_mgr_ctx_pmt(dev, yid), 72 * 0 + 64, 8),
            bf_bs_get_word(mc_mgr_ctx_pmt(dev, yid), 72 * 0, 64));*/
  return BF_SUCCESS;
}

bf_status_t mc_mgr_get_mit_row_reg(bf_dev_id_t dev,
                                   int pipe,
                                   int row,
                                   uint32_t *mit0,
                                   uint32_t *mit1,
                                   uint32_t *mit2,
                                   uint32_t *mit3) {
  uint64_t addr = 0;
  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      addr = row * pipe_top_level_tm_pre_mit0_mem_word_array_element_size +
             ((0 == pipe)
                  ? pipe_top_level_tm_pre_mit0_mem_word_address
                  : (1 == pipe)
                        ? pipe_top_level_tm_pre_mit1_mem_word_address
                        : (2 == pipe)
                              ? pipe_top_level_tm_pre_mit2_mem_word_address
                              : pipe_top_level_tm_pre_mit3_mem_word_address);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      addr = tof2_mem_tm_tm_pre_pre_pipe_mem_mit_mem_word(pipe, row);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      addr = tof3_mem_tm_tm_pre_pre_pipe_mem_mit_mem_word(pipe, row);
      break;
    default:
      MC_MGR_DBGCHK(0);
  }

  uint64_t data0 = 0, data1 = 0;
  int x = lld_subdev_ind_read(dev, 0, addr / 16, &data1, &data0);
  if (x) {
    LOG_ERROR("Read MIT row fails, indirect reg read (%d) at %s:%d",
              x,
              __func__,
              __LINE__);
    MC_MGR_DBGCHK(!x);
    return BF_HW_COMM_FAIL;
  }
  for (bf_subdev_id_t subdev = 1; subdev < (int)mc_mgr_ctx_num_subdevices(dev);
       ++subdev) {
    uint64_t temp_hi = 0, temp_low = 0;
    x = lld_subdev_ind_read(dev, subdev, addr / 16, &temp_hi, &temp_low);
    if (x) {
      LOG_ERROR("Indirect reg read (%d) at %s:%d", x, __func__, __LINE__);
      return BF_HW_COMM_FAIL;
    }
    if (temp_low != data0 || temp_hi != data1) {
      LOG_ERROR("Indirect reg read for subdev (%d) match failed %s:%d",
                subdev,
                __func__,
                __LINE__);
      return BF_UNEXPECTED;
    }
    data0 = temp_low;
    data1 = temp_hi;
  }
  *mit0 = data0 & 0xFFFFF;
  *mit1 = (data0 >> 20) & 0xFFFFF;
  *mit2 = (data0 >> 40) & 0xFFFFF;
  *mit3 = ((data0 >> 60) & 0xF) | ((data1 << 4) & 0xFFFF0);
#ifdef UTEST
  /* Pull the data out of our software shadow and return it since there is no
   * hardware (or sw model) to read in the unit test environment. */
  x = row << 2;
  *mit0 = mc_mgr_ctx_tree(dev, pipe, x + 0)
              ? mc_mgr_ctx_tree(dev, pipe, x + 0)->rdm_addr
              : 0;
  *mit1 = mc_mgr_ctx_tree(dev, pipe, x + 1)
              ? mc_mgr_ctx_tree(dev, pipe, x + 1)->rdm_addr
              : 0;
  *mit2 = mc_mgr_ctx_tree(dev, pipe, x + 2)
              ? mc_mgr_ctx_tree(dev, pipe, x + 2)->rdm_addr
              : 0;
  *mit3 = mc_mgr_ctx_tree(dev, pipe, x + 3)
              ? mc_mgr_ctx_tree(dev, pipe, x + 3)->rdm_addr
              : 0;
#endif
  return BF_SUCCESS;
}

bf_status_t mc_mgr_set_mit_wrl(int sid, bf_dev_id_t dev, int pipe, int mgid) {
  uint64_t vals[4];
  uint16_t x = mgid & 0xFFFC;
  vals[0] = mc_mgr_ctx_tree(dev, pipe, x + 0)
                ? mc_mgr_ctx_tree(dev, pipe, x + 0)->rdm_addr
                : 0;
  vals[1] = mc_mgr_ctx_tree(dev, pipe, x + 1)
                ? mc_mgr_ctx_tree(dev, pipe, x + 1)->rdm_addr
                : 0;
  vals[2] = mc_mgr_ctx_tree(dev, pipe, x + 2)
                ? mc_mgr_ctx_tree(dev, pipe, x + 2)->rdm_addr
                : 0;
  vals[3] = mc_mgr_ctx_tree(dev, pipe, x + 3)
                ? mc_mgr_ctx_tree(dev, pipe, x + 3)->rdm_addr
                : 0;

  uint64_t lo = ((vals[0] & UINT64_C(0xFFFFF)) << 0) |
                ((vals[1] & UINT64_C(0xFFFFF)) << 20) |
                ((vals[2] & UINT64_C(0xFFFFF)) << 40) |
                ((vals[3] & UINT64_C(0x0000F)) << 60);
  uint64_t hi = ((vals[3] & UINT64_C(0xFFFF0)) >> 4);

  uint64_t addr = 0;
  uint64_t addr1 = 0;
  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      addr = (x >> 2) * pipe_top_level_tm_pre_mit0_mem_word_array_element_size +
             ((0 == pipe)
                  ? pipe_top_level_tm_pre_mit0_mem_word_address
                  : (1 == pipe)
                        ? pipe_top_level_tm_pre_mit1_mem_word_address
                        : (2 == pipe)
                              ? pipe_top_level_tm_pre_mit2_mem_word_address
                              : pipe_top_level_tm_pre_mit3_mem_word_address);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      addr = tof2_mem_tm_tm_pre_pre_pipe_mem_mit_mem_word(pipe, (x >> 2));
      break;
    case BF_DEV_FAMILY_TOFINO3:
#if defined(EMU_2DIE_USING_SW_2DEV)
      /* Program local pipes as 0-3 always */
      if (dev == 1) {
        /* Flip the 3rd pipe bit */
        pipe = pipe ^ 0x4;
      }
#endif
      addr = tof3_mem_tm_tm_pre_pre_pipe_mem_mit_mem_word(pipe, (x >> 2));
      if (mc_mgr_ctx_num_subdevices(dev) > 1) {
        int other_die_pipe = 0;
        /* Flip the 3rd pipe bit */
        other_die_pipe = pipe ^ 0x4;
        addr1 = tof3_mem_tm_tm_pre_pre_pipe_mem_mit_mem_word(other_die_pipe,
                                                             (x >> 2));
      }
      break;
    default:
      MC_MGR_DBGCHK(0);
  }
  if (mc_mgr_drv_wrl_append(
          dev,
          (mc_mgr_ctx_num_subdevices(dev) > 1) ? 0 : MC_MGR_DRV_SUBDEV_ID_ALL,
          sid,
          16,
          addr,
          hi,
          lo,
          __func__,
          __LINE__)) {
    return BF_NO_SYS_RESOURCES;
  }

  if ((mc_mgr_ctx_num_subdevices(dev) > 1) && (addr1 != 0)) {
    if (mc_mgr_drv_wrl_append(
            dev, 1, sid, 16, addr1, hi, lo, __func__, __LINE__)) {
      return BF_NO_SYS_RESOURCES;
    }
  }
  /*LOG_TRACE("MIT[%d][%d][%5d] = 0x%05x%s",
            dev,
            pipe,
            x + 0,
            vals[0] & 0xFFFFFFFF,
            mgid == (x + 0) ? " *" : "");
  LOG_TRACE("MIT[%d][%d][%5d] = 0x%05x%s",
            dev,
            pipe,
            x + 1,
            vals[1] & 0xFFFFFFFF,
            mgid == (x + 1) ? " *" : "");
  LOG_TRACE("MIT[%d][%d][%5d] = 0x%05x%s",
            dev,
            pipe,
            x + 2,
            vals[2] & 0xFFFFFFFF,
            mgid == (x + 2) ? " *" : "");
  LOG_TRACE("MIT[%d][%d][%5d] = 0x%05x%s",
            dev,
            pipe,
            x + 3,
            vals[3] & 0xFFFFFFFF,
            mgid == (x + 3) ? " *" : "");*/
  return BF_SUCCESS;
}

bf_status_t mc_mgr_get_rdm_reg(
    int sid, bf_dev_id_t dev, int line, uint64_t *hi, uint64_t *lo) {
  (void)sid;

#ifdef UTEST
  /* Make a copy of the software shadow because the lld read overwrites it. */
  mc_mgr_rdm_line_t *l = &mc_mgr_ctx_rdm_map(dev)->rdm[line];
  uint64_t hi_cpy = l->data[1];
  uint64_t lo_cpy = l->data[0];
#endif
  uint64_t addr = 0;
  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      addr = line * pipe_top_level_tm_pre_rdm_mem_word_array_element_size +
             pipe_top_level_tm_pre_rdm_mem_word_address;
      break;
    case BF_DEV_FAMILY_TOFINO2:
      addr = tof2_mem_tm_tm_pre_pre_common_mem_rdm_mem_word(line);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      addr = tof3_mem_tm_tm_pre_pre_common_mem_rdm_mem_word(line);
      break;
    default:
      MC_MGR_DBGCHK(0);
  }
  int x = lld_subdev_ind_read(dev, 0, addr / 16, hi, lo);
  if (x) {
    LOG_ERROR("Read RDM row %d on dev %d fails, status %d", line, dev, x);
    MC_MGR_DBGCHK(0);
    return BF_HW_COMM_FAIL;
  }
  for (bf_subdev_id_t subdev = 1; subdev < (int)mc_mgr_ctx_num_subdevices(dev);
       ++subdev) {
    uint64_t temp_hi = 0, temp_low = 0;
    x = lld_subdev_ind_read(dev, subdev, addr / 16, &temp_hi, &temp_low);
    if (x) {
      LOG_ERROR("Indirect reg read (%d) at %s:%d", x, __func__, __LINE__);
      return BF_HW_COMM_FAIL;
    }
    if (temp_low != *lo || temp_hi != *hi) {
      LOG_ERROR("Indirect reg read for subdev (%d) match failed %s:%d",
                subdev,
                __func__,
                __LINE__);
      return BF_UNEXPECTED;
    }
    *lo = temp_low;
    *hi = temp_hi;
  }
#ifdef UTEST
  /* Restore the clobbered software shadow */
  l->data[1] = hi_cpy;
  l->data[0] = lo_cpy;
  /* Pull the data out of our software shadow and return it since there is no
   * hardware (or sw model) to read in the unit test environment. */
  *hi = l->data[1];
  *lo = l->data[0];
#endif
  return BF_SUCCESS;
}
bf_status_t mc_mgr_set_rdm_wrl(
    int sid, bf_dev_id_t dev, int line, uint64_t hi, uint64_t lo) {
  uint64_t addr = 0;
  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      addr = line * pipe_top_level_tm_pre_rdm_mem_word_array_element_size +
             pipe_top_level_tm_pre_rdm_mem_word_address;
      break;
    case BF_DEV_FAMILY_TOFINO2:
      addr = tof2_mem_tm_tm_pre_pre_common_mem_rdm_mem_word(line);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      addr = tof3_mem_tm_tm_pre_pre_common_mem_rdm_mem_word(line);
      break;
    default:
      MC_MGR_DBGCHK(0);
  }
  bf_status_t sts = mc_mgr_drv_wrl_append(
      dev, MC_MGR_DRV_SUBDEV_ID_ALL, sid, 16, addr, hi, lo, __func__, __LINE__);
  /*  LOG_TRACE(
        "  %s: addr 0x%lX data 0x%016lX.%016lX",
        __func__,
        addr,
        hi,
        lo);*/
  if (BF_SUCCESS != sts) {
    MC_MGR_DBGCHK(BF_SUCCESS == sts);
  }
  return sts;
}
