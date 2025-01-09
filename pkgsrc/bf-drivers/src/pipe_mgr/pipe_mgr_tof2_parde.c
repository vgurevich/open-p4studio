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
 * @file pipe_mgr_tof2_parde.c
 * @date
 *
 * Implementation of Tof2 ParDe management.
 */

#include "pipe_mgr_int.h"
#include <tof2_regs/tof2_reg_drv.h>
#include "pipe_mgr_tof2_parde.h"
#include "pipe_mgr_drv_intf.h"
#include "pipe_mgr_drv.h"
#include "lld/lld_efuse.h"
struct pipe_mgr_tof2_ebuf_pipe_reg_ctx {
  /* Shadows of the following register field:
   *  pipes.pardereg.pgstnreg.epbprsr4reg.epbref.chanX_group.chnl_ctrl.meta_opt
   */
  uint16_t meta_opt[TOF2_NUM_EPB];
};

enum tof2_ipb_dprsr_credit {
  tof2_ipb_dprsr_credit_none = 0,
  tof2_ipb_dprsr_credit_50g = 0x2C,
  tof2_ipb_dprsr_credit_100g = 0x058,
  tof2_ipb_dprsr_credit_200g = 0x0B0,
  tof2_ipb_dprsr_credit_400g = 0x160,
};
enum tof2_ipb_dprsr_barrel_credit {
  tof2_ipb_dprsr_barrel_credit_none = 0,
  tof2_ipb_dprsr_barrel_credit_50g = 1,
  tof2_ipb_dprsr_barrel_credit_100g = 2,
  tof2_ipb_dprsr_barrel_credit_200g = 4,
  tof2_ipb_dprsr_barrel_credit_400g = 8,
};

static int tof2_parde_speed_to_chan_cnt(bf_port_speeds_t speed) {
  switch (speed) {
    case BF_SPEED_NONE:
    case BF_SPEED_1G:
    case BF_SPEED_10G:
    case BF_SPEED_25G:
    case BF_SPEED_40G:
    case BF_SPEED_40G_R2:
    case BF_SPEED_50G:
    case BF_SPEED_50G_CONS:
      return 1;
    case BF_SPEED_100G:
      return 2;
    case BF_SPEED_200G:
      return 4;
    case BF_SPEED_400G:
      return 8;
    default:
      PIPE_MGR_DBGCHK(0);
      return 0;
  }
}

static int tof2_parde_speed_to_port_rate(bf_port_speeds_t speed) {
  switch (speed) {
    case BF_SPEED_NONE:
      return 4;
    case BF_SPEED_1G:
    case BF_SPEED_10G:
      return 0;
    case BF_SPEED_25G:
      return 1;
    case BF_SPEED_40G:
    case BF_SPEED_40G_R2:
      return 2;
    case BF_SPEED_50G:
    case BF_SPEED_50G_CONS:
      return 3;
    case BF_SPEED_100G:
      return 5;
    case BF_SPEED_200G:
      return 6;
    case BF_SPEED_400G:
      return 7;
    default:
      PIPE_MGR_DBGCHK(0);
      return 0;
  }
}

static int tof2_ipb_local_port_to_ipb(int local_port) {
  if (local_port < 0 || local_port >= TOF2_NUM_PORTS_PER_PIPE) {
    PIPE_MGR_DBGCHK(0);
    return -1;
  }
  int ibuf = local_port / TOF2_NUM_CHN_PER_IPB;
  if (ibuf >= TOF2_NUM_IPB) {
    PIPE_MGR_DBGCHK(0);
    return -1;
  }
  return ibuf;
}

static int tof2_ipb_local_port_to_chan(int local_port) {
  if (local_port < 0 || local_port >= TOF2_NUM_PORTS_PER_PIPE) {
    PIPE_MGR_DBGCHK(0);
    return -1;
  }
  int chan = local_port % TOF2_NUM_CHN_PER_IPB;
  return chan;
}
static int tof2_local_port_to_ebuf_grp(int local_port) {
  if (local_port < 8) return local_port / 2;
  return (local_port - 8) / 16;
}
static bool tof2_ebuf_is400(int local_port) { return local_port > 7; }
static int tof2_local_port_to_ebuf_grp_idx(int local_port) {
  if (!tof2_ebuf_is400(local_port)) {
    return 0;
  }
  return ((local_port - 8) >> 3) & 1;
}

static void tof2_prsr_chan_speed_to_prsr_id(int chan_num,
                                            bf_port_speeds_t speed,
                                            int *first_prsr_id,
                                            int *prsr_cnt) {
  *first_prsr_id = chan_num / 2;
  switch (speed) {
    case BF_SPEED_1G:
    case BF_SPEED_10G:
    case BF_SPEED_25G:
    case BF_SPEED_40G:
    case BF_SPEED_40G_R2:
    case BF_SPEED_50G:
    case BF_SPEED_50G_CONS:
    case BF_SPEED_100G:
      *prsr_cnt = 1;
      break;
    case BF_SPEED_200G:
      *prsr_cnt = 2;
      break;
    case BF_SPEED_400G:
      *prsr_cnt = 4;
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      *prsr_cnt = 0;
      break;
  }
  return;
}

static uint32_t tof2_parde_get_port_rate_cfg(rmt_dev_info_t *dev_info,
                                             int logical_pipe,
                                             int ipb_num,
                                             int port_to_skip) {
  /* Initialize all channel values to unused.  Note that we are using a setp
   * function for the glb_group.port_rates register but it is actually all the
   * port_rate registers share a common template so we can use the setp/getp
   * functions for any of them. */
  uint32_t val = 0;
  for (int i = 0; i < TOF2_NUM_CHN_PER_IPB; ++i) {
    int x = tof2_parde_speed_to_port_rate(BF_SPEED_NONE);
    setp_tof2_glb_group_port_rates_chnl_rate(&val, i, x);
  }
  /* For each channel in the IPB check if a port exists for it and program
   * based on it's speed.  Then skip over the number of channels it uses and
   * look for the next port. */
  for (int i = 0; i < TOF2_NUM_CHN_PER_IPB;) {
    bf_dev_port_t p_local = ipb_num * TOF2_NUM_CHN_PER_IPB + i;
    bf_dev_port_t p = dev_info->dev_cfg.make_dev_port(logical_pipe, p_local);
    rmt_port_info_t *port_info = pipe_mgr_get_port_info(dev_info->dev_id, p);
    bf_port_speed_t speed = port_info ? port_info->speed : BF_SPEED_NONE;
    int step = tof2_parde_speed_to_chan_cnt(speed);
    int x = tof2_parde_speed_to_port_rate(speed);
    if (port_to_skip != p) setp_tof2_glb_group_port_rates_chnl_rate(&val, i, x);
    i += step;
  }
  return val;
}
static uint32_t tof2_parde_xpb_convert_port_rate_cfg(uint32_t port_rate_cfg) {
  int rate_0 = tof2_parde_speed_to_port_rate(BF_SPEED_NONE);
  int rate_50 = tof2_parde_speed_to_port_rate(BF_SPEED_50G);
  uint32_t val = port_rate_cfg;

  for (int i = 0; i < TOF2_NUM_CHN_PER_IPB; ++i) {
    int rate = getp_tof2_glb_group_port_rates_chnl_rate(&port_rate_cfg, i);
    if (rate == rate_0) {
      /* Convert all the rate-none (unused) to 50g for IPB and EPB for
       * jbay-2683.  */
      setp_tof2_glb_group_port_rates_chnl_rate(&val, i, rate_50);
    }
  }
  return val;
}
static uint32_t tof2_parde_pmerge_convert_port_rate_cfg(uint32_t port_rate_cfg,
                                                        bool is_port_rmv) {
  int rate_0 = tof2_parde_speed_to_port_rate(BF_SPEED_NONE);
  int rate_10 = tof2_parde_speed_to_port_rate(BF_SPEED_10G);
  int rate_25 = tof2_parde_speed_to_port_rate(BF_SPEED_25G);
  int rate_40 = tof2_parde_speed_to_port_rate(BF_SPEED_40G);
  int rate_50 = tof2_parde_speed_to_port_rate(BF_SPEED_50G);
  uint32_t val = port_rate_cfg;

  for (int i = 0; i < TOF2_NUM_CHN_PER_IPB; i += 2) {
    int r0 = getp_tof2_glb_group_port_rates_chnl_rate(&port_rate_cfg, i);
    int r1 = getp_tof2_glb_group_port_rates_chnl_rate(&port_rate_cfg, i + 1);
    if (r0 == rate_0 &&
        (r1 == rate_10 || r1 == rate_25 || r1 == rate_40 || r1 == rate_50)) {
      /* Channel i is unused but channel i+1 is used.  Set the unused channel to
       * the value of the used channel.  If we are removing the port then don't
       * set it to the same value (the HW needs to see the port-mode change),
       * set it to another single channel speed. */
      if (is_port_rmv) {
        if (r1 == rate_10)
          r1 = rate_50;
        else if (r1 == rate_25)
          r1 = rate_50;
        else if (r1 == rate_40)
          r1 = rate_50;
        else if (r1 == rate_50)
          r1 = rate_10;
      }
      setp_tof2_glb_group_port_rates_chnl_rate(&val, i, r1);
    }
    if (r1 == rate_0 &&
        (r0 == rate_10 || r0 == rate_25 || r0 == rate_40 || r0 == rate_50)) {
      /* Channel i+1 is unused but channel i is used.  Set the unused channel to
       * the value of the used channel.  If we are removing the port then don't
       * set it to the same value (the HW needs to see the port-mode change),
       * set it to another single channel speed. */
      if (is_port_rmv) {
        if (r0 == rate_10)
          r0 = rate_50;
        else if (r0 == rate_25)
          r0 = rate_50;
        else if (r0 == rate_40)
          r0 = rate_50;
        else if (r0 == rate_50)
          r0 = rate_10;
      }
      setp_tof2_glb_group_port_rates_chnl_rate(&val, i + 1, r0);
    }
  }
  return val;
}

static uint8_t tof2_parde_get_chan_en_mask(rmt_dev_info_t *dev_info,
                                           int logical_pipe,
                                           int ipb_num) {
  uint8_t enable_mask = 0;
  /* For each channel in the IPB check if a port exists for it.  Then skip over
   * the number of channels it uses and look for the next port. */
  for (int i = 0; i < TOF2_NUM_CHN_PER_IPB;) {
    bf_dev_port_t p_local = ipb_num * TOF2_NUM_CHN_PER_IPB + i;
    bf_dev_port_t p = dev_info->dev_cfg.make_dev_port(logical_pipe, p_local);
    rmt_port_info_t *port_info = pipe_mgr_get_port_info(dev_info->dev_id, p);
    if (port_info) {
      enable_mask |= 1u << i;
      i += tof2_parde_speed_to_chan_cnt(port_info->speed);
    } else {
      ++i;
    }
  }
  return enable_mask;
}

static void tof2_chan_to_dprsr_info(int local_port,
                                    int *slice,
                                    int *mac,
                                    int *dprsr_chan) {
  int xpb = local_port / TOF2_NUM_CHN_PER_IPB;
  int xpb_chan = local_port % TOF2_NUM_CHN_PER_IPB;
  if (0 == xpb) {
    /* IPB/EPB zero's channels are spread across all slices. */
    if (slice) *slice = local_port / 2;
    if (mac) *mac = 0;
    if (dprsr_chan) *dprsr_chan = xpb_chan % 2;
  } else {
    if (slice) *slice = (xpb - 1) / 2;
    if (mac) *mac = 1 + ((xpb - 1) % 2);
    if (dprsr_chan) *dprsr_chan = xpb_chan;
  }
}

static pipe_status_t tof2_ipb_write_chan_ctrl(pipe_sess_hdl_t shdl,
                                              bf_dev_port_t port_id,
                                              int version,
                                              rmt_dev_info_t *dev_info) {
  bf_dev_id_t dev_id = dev_info->dev_id;
  rmt_port_info_t *port_info = pipe_mgr_get_port_info(dev_id, port_id);
  uint32_t val_lo = 0, val_hi = 0;

  int disable_cong_notif = port_info ? port_info->disable_cong_notif : 1;
  int pause_mac = port_info ? port_info->pause_mac : 1;
  int local_port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  int logical_pipe = dev_info->dev_cfg.dev_port_to_pipe(port_id);
  int credits = tof2_ipb_dprsr_credit_none;
  int barrel_credits = tof2_ipb_dprsr_barrel_credit_none;
  if (port_info) {
    switch (port_info->speed) {
      case BF_SPEED_1G:
      case BF_SPEED_10G:
      case BF_SPEED_25G:
      case BF_SPEED_40G:
      case BF_SPEED_40G_R2:
      case BF_SPEED_50G:
      case BF_SPEED_50G_CONS:
        credits = tof2_ipb_dprsr_credit_50g;
        barrel_credits = tof2_ipb_dprsr_barrel_credit_50g;
        break;
      case BF_SPEED_100G:
        credits = tof2_ipb_dprsr_credit_100g;
        barrel_credits = tof2_ipb_dprsr_barrel_credit_100g;
        break;
      case BF_SPEED_200G:
        credits = tof2_ipb_dprsr_credit_200g;
        barrel_credits = tof2_ipb_dprsr_barrel_credit_200g;
        break;
      case BF_SPEED_400G:
        credits = tof2_ipb_dprsr_credit_400g;
        barrel_credits = tof2_ipb_dprsr_barrel_credit_400g;
        break;
      default:
        credits = tof2_ipb_dprsr_credit_none;
        barrel_credits = tof2_ipb_dprsr_barrel_credit_none;
        PIPE_MGR_DBGCHK(0);
        break;
    }
  }

  setp_tof2_chnl_ctrl_chnl_ctrl_0_2_dis_cong(&val_lo, disable_cong_notif);
  setp_tof2_chnl_ctrl_chnl_ctrl_0_2_en_tx_xoff(&val_lo, pause_mac);
  setp_tof2_chnl_ctrl_chnl_ctrl_0_2_allow_tm_pfc(&val_lo, 1);
  setp_tof2_chnl_ctrl_chnl_ctrl_0_2_allow_tm_xoff(&val_lo, 1);
  setp_tof2_chnl_ctrl_chnl_ctrl_0_2_ingress_port(&val_lo, port_id);
  setp_tof2_chnl_ctrl_chnl_ctrl_0_2_version(&val_lo, version);
  setp_tof2_chnl_ctrl_chnl_ctrl_0_2_overwrite_meta1(&val_lo, 1);
  setp_tof2_chnl_ctrl_chnl_ctrl_0_2_init_dprsr_credit(&val_lo, credits);
  setp_tof2_chnl_ctrl_chnl_ctrl_0_2_en_clean_drop(&val_lo, 1);
  setp_tof2_chnl_ctrl_chnl_ctrl_1_2_init_dprsr_barrel_credit(&val_hi,
                                                             barrel_credits);
  setp_tof2_chnl_ctrl_chnl_ctrl_1_2_mchn_dprsr_port(&val_hi, local_port);

  pipe_bitmap_t pipe_bit_map;
  PIPE_BITMAP_INIT(&pipe_bit_map, PIPE_BMP_SIZE);
  PIPE_BITMAP_SET(&pipe_bit_map, logical_pipe);

  int ipb_num = tof2_ipb_local_port_to_ipb(local_port);
  if (-1 == ipb_num) return PIPE_INVALID_ARG;
  int chan_num = tof2_ipb_local_port_to_chan(local_port);
  if (-1 == chan_num) return PIPE_INVALID_ARG;
  uint32_t base_addr =
      offsetof(tof2_reg,
               pipes[0]
                   .pardereg.pgstnreg.ipbprsr4reg[ipb_num]
                   .ipbreg.chan0_group.chnl_ctrl.chnl_ctrl_0_2);
  uint32_t chan_step =
      offsetof(tof2_reg,
               pipes[0]
                   .pardereg.pgstnreg.ipbprsr4reg[ipb_num]
                   .ipbreg.chan1_group.chnl_ctrl.chnl_ctrl_0_2) -
      base_addr;
  int chnl_cnt = tof2_parde_speed_to_chan_cnt(port_info ? port_info->speed
                                                        : BF_SPEED_NONE);
  for (int chnl = chan_num; chnl < chan_num + chnl_cnt; ++chnl) {
    uint32_t addr = base_addr + chan_step * chnl;
    uint32_t stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(addr);
    /* mchn_dprsr_port should always be set to the channel number (0-71). */
    setp_tof2_chnl_ctrl_chnl_ctrl_1_2_mchn_dprsr_port(
        &val_hi, local_port + (chnl - chan_num));
    pipe_instr_write_reg_t instr[2];
    construct_instr_reg_write(dev_id, &instr[0], addr, val_lo);
    construct_instr_reg_write(dev_id, &instr[1], addr + 4, val_hi);
    for (size_t i = 0; i < sizeof instr / sizeof instr[0]; ++i) {
      pipe_status_t sts = pipe_mgr_drv_ilist_add(&shdl,
                                                 dev_info,
                                                 &pipe_bit_map,
                                                 stage,
                                                 (uint8_t *)&instr[i],
                                                 sizeof instr[i]);
      if (PIPE_SUCCESS != sts) {
        LOG_ERROR("Dev %d port %d Failed to add ibp chnl_ctrl instr (%s)",
                  dev_info->dev_id,
                  port_id,
                  pipe_str_err(sts));
        PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
        return sts;
      }
    }
  }
  return PIPE_SUCCESS;
}

static pipe_status_t tof2_ipb_write_chan_ctrl_all(pipe_sess_hdl_t shdl,
                                                  int version,
                                                  rmt_dev_info_t *dev_info) {
  pipe_status_t sts = PIPE_SUCCESS;
  for (bf_dev_pipe_t pipe = 0; pipe < dev_info->num_active_pipes; ++pipe) {
    for (int port = 0; port < TOF2_NUM_PORTS_PER_PIPE; ++port) {
      bf_dev_port_t port_id = dev_info->dev_cfg.make_dev_port(pipe, port);
      sts = tof2_ipb_write_chan_ctrl(shdl, port_id, version, dev_info);
      if (sts != PIPE_SUCCESS) {
        LOG_ERROR("Dev %d port %d Failed to set ipb chan ctrl, sts %s",
                  dev_info->dev_id,
                  port_id,
                  pipe_str_err(sts));
        return sts;
      }
    }
  }
  return sts;
}

static pipe_status_t tof2_ipb_epb_write_chan_en(pipe_sess_hdl_t shdl,
                                                int logical_pipe,
                                                int ipb_num,
                                                rmt_dev_info_t *dev_info,
                                                bool ing_0_egr_1,
                                                uint8_t enable_bit_map,
                                                bool use_dma) {
  if (ipb_num < 0 || ipb_num >= TOF2_NUM_IPB) {
    LOG_ERROR("Dev %d, invalid ipb %d while setting channel enable",
              dev_info->dev_id,
              ipb_num);
    PIPE_MGR_DBGCHK(ipb_num >= 0);
    PIPE_MGR_DBGCHK(ipb_num < TOF2_NUM_IPB);
    return PIPE_INVALID_ARG;
  }

  if (logical_pipe >= (int)dev_info->num_active_pipes) {
    LOG_ERROR("Dev %d, invalid pipe 0x%x while setting channel enable",
              dev_info->dev_id,
              logical_pipe);
    return PIPE_INVALID_ARG;
  }

  int p = 0;
  if (!use_dma) {
    /* If we are going to use DMA the pipe doesn't affect the address in the
     * instruction, it is set when the instruction is added to the ilist.  But,
     * if we are doing direct register writes we need to conver the logical
     * pipe to the physical pipe. */
    bf_dev_pipe_t p_pipe;
    pipe_status_t s =
        pipe_mgr_map_pipe_id_log_to_phy(dev_info, logical_pipe, &p_pipe);
    p = p_pipe;
    if (s != PIPE_SUCCESS) return s;
  }

  uint32_t val = 0;
  setp_tof2_glb_group_port_en_enbl(&val, enable_bit_map);
  uint32_t addr = ing_0_egr_1
                      ? offsetof(tof2_reg,
                                 pipes[p]
                                     .pardereg.pgstnreg.epbprsr4reg[ipb_num]
                                     .epbreg.glb_group.port_en)
                      : offsetof(tof2_reg,
                                 pipes[p]
                                     .pardereg.pgstnreg.ipbprsr4reg[ipb_num]
                                     .ipbreg.glb_group.port_en);
  if (use_dma) {
    pipe_bitmap_t pipe_bit_map;
    PIPE_BITMAP_INIT(&pipe_bit_map, PIPE_BMP_SIZE);
    PIPE_BITMAP_SET(&pipe_bit_map, logical_pipe);
    pipe_instr_write_reg_t instr;
    construct_instr_reg_write(dev_info->dev_id, &instr, addr, val);
    uint32_t stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(addr);
    pipe_status_t sts = pipe_mgr_drv_ilist_add(
        &shdl, dev_info, &pipe_bit_map, stage, (uint8_t *)&instr, sizeof instr);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Dev %d ipb %d Failed to add %s port_en instr (%s)",
                dev_info->dev_id,
                ipb_num,
                ing_0_egr_1 ? "egr" : "ing",
                pipe_str_err(sts));
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
      return sts;
    }
  } else {
    lld_write_register(dev_info->dev_id, addr, val);
  }
  LOG_TRACE("Dev %d LogPipe %d %cPB %d en map 0x%02x",
            dev_info->dev_id,
            logical_pipe,
            ing_0_egr_1 ? 'E' : 'I',
            ipb_num,
            enable_bit_map);
  return PIPE_SUCCESS;
}

static pipe_status_t tof2_ipb_write_meta_fifo_ctrl(pipe_sess_hdl_t shdl,
                                                   rmt_dev_info_t *dev_info,
                                                   bf_dev_port_t port_id,
                                                   bf_port_speeds_t speed) {
  /* The FIFO has 448 entries in it, divide them up equally per channel. */
  const int meta_fifo_total_size = 448;
  const int meta_fifo_sz_per_chan = meta_fifo_total_size / TOF2_NUM_CHN_PER_IPB;
  int local_port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  int logical_pipe = dev_info->dev_cfg.dev_port_to_pipe(port_id);
  int ipb_num = tof2_ipb_local_port_to_ipb(local_port);
  if (-1 == ipb_num) return PIPE_INVALID_ARG;
  int chan_num = tof2_ipb_local_port_to_chan(local_port);
  int chan_cnt = tof2_parde_speed_to_chan_cnt(speed);

  pipe_bitmap_t pipe_bit_map;
  PIPE_BITMAP_INIT(&pipe_bit_map, PIPE_BMP_SIZE);
  PIPE_BITMAP_SET(&pipe_bit_map, logical_pipe);

  int meta_fifo_start = chan_num * meta_fifo_sz_per_chan;
  int meta_fifo_sz = chan_cnt * meta_fifo_sz_per_chan;
  int meta_fifo_end = meta_fifo_start + meta_fifo_sz - 1;
  uint32_t val = 0;
  setp_tof2_meta_fifo_ctrl_meta_fifo_start(&val, meta_fifo_start);
  setp_tof2_meta_fifo_ctrl_meta_fifo_end(&val, meta_fifo_end);
  setp_tof2_meta_fifo_ctrl_meta_fifo_size(&val, meta_fifo_sz);
  uint32_t base_addr = offsetof(tof2_reg,
                                pipes[0]
                                    .pardereg.pgstnreg.ipbprsr4reg[ipb_num]
                                    .ipbreg.chan0_group.meta_fifo_ctrl);
  uint32_t chan_step = offsetof(tof2_reg,
                                pipes[0]
                                    .pardereg.pgstnreg.ipbprsr4reg[ipb_num]
                                    .ipbreg.chan1_group.meta_fifo_ctrl) -
                       base_addr;
  uint32_t stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(base_addr);
  for (int i = 0; i < chan_cnt; ++i) {
    int this_chan = chan_num + i;
    uint32_t addr = base_addr + chan_step * this_chan;
    pipe_instr_write_reg_t instr;
    construct_instr_reg_write(dev_info->dev_id, &instr, addr, val);
    pipe_status_t sts = pipe_mgr_drv_ilist_add(
        &shdl, dev_info, &pipe_bit_map, stage, (uint8_t *)&instr, sizeof instr);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Dev %d port %d Failed to add ibp meta_fifo_ctrl instr (%s)",
                dev_info->dev_id,
                port_id,
                pipe_str_err(sts));
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
      return sts;
    }
    /* Set all channels used by this port except the first to zero. */
    val = 0;
  }
  return PIPE_SUCCESS;
}

static pipe_status_t tof2_ipb_write_chnl_fifo_ctrl(pipe_sess_hdl_t shdl,
                                                   rmt_dev_info_t *dev_info,
                                                   bf_dev_port_t port_id,
                                                   bf_port_speeds_t speed) {
  /* The FIFO has 660 entries in it, divide them up equally per channel. */
  const int chnl_fifo_total_size = 660;
  const int chnl_fifo_sz_per_chan = chnl_fifo_total_size / TOF2_NUM_CHN_PER_IPB;
  int local_port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  int logical_pipe = dev_info->dev_cfg.dev_port_to_pipe(port_id);
  int ipb_num = tof2_ipb_local_port_to_ipb(local_port);
  if (-1 == ipb_num) return PIPE_INVALID_ARG;
  int chan_num = tof2_ipb_local_port_to_chan(local_port);
  int chan_cnt = tof2_parde_speed_to_chan_cnt(speed);

  pipe_bitmap_t pipe_bit_map;
  PIPE_BITMAP_INIT(&pipe_bit_map, PIPE_BMP_SIZE);
  PIPE_BITMAP_SET(&pipe_bit_map, logical_pipe);

  int chnl_fifo_start = chan_num * chnl_fifo_sz_per_chan;
  int chnl_fifo_sz = chan_cnt * chnl_fifo_sz_per_chan;
  int chnl_fifo_end = chnl_fifo_start + chnl_fifo_sz - 1;
  uint32_t vals[3] = {0};
  setp_tof2_chnl_fifo_ctrl_chnl_fifo_ctrl_0_3_pkt_fifo_start(&vals[0],
                                                             chnl_fifo_start);
  setp_tof2_chnl_fifo_ctrl_chnl_fifo_ctrl_0_3_pkt_fifo_end(&vals[0],
                                                           chnl_fifo_end);
  /* Each fifo address has two elements so the size is times two. */
  setp_tof2_chnl_fifo_ctrl_chnl_fifo_ctrl_0_3_pkt_fifo_size(&vals[0],
                                                            2 * chnl_fifo_sz);
  setp_tof2_chnl_fifo_ctrl_chnl_fifo_ctrl_1_3_dprsr_barrel_fifo_start(&vals[1],
                                                                      chan_num);
  setp_tof2_chnl_fifo_ctrl_chnl_fifo_ctrl_1_3_dprsr_barrel_fifo_end(
      &vals[1], chan_num + chan_cnt - 1);
  setp_tof2_chnl_fifo_ctrl_chnl_fifo_ctrl_1_3_lat_fifo_alm_full_off(&vals[1],
                                                                    chan_cnt);
  setp_tof2_chnl_fifo_ctrl_chnl_fifo_ctrl_1_3_ppc_fifo_alm_full_off(&vals[1],
                                                                    chan_cnt);
  setp_tof2_chnl_fifo_ctrl_dppc_fifo_alm_full_off(vals, chan_cnt);
  setp_tof2_chnl_fifo_ctrl_meta_fifo_alm_full_off(vals, 2);
  uint32_t base_addr =
      offsetof(tof2_reg,
               pipes[0]
                   .pardereg.pgstnreg.ipbprsr4reg[ipb_num]
                   .ipbreg.chan0_group.chnl_fifo_ctrl.chnl_fifo_ctrl_0_3);
  uint32_t chan_step =
      offsetof(tof2_reg,
               pipes[0]
                   .pardereg.pgstnreg.ipbprsr4reg[ipb_num]
                   .ipbreg.chan1_group.chnl_fifo_ctrl.chnl_fifo_ctrl_0_3) -
      base_addr;
  uint32_t stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(base_addr);
  for (int i = 0; i < chan_cnt; ++i) {
    int this_chan = chan_num + i;
    uint32_t addr = base_addr + chan_step * this_chan;
    for (unsigned int j = 0; j < sizeof vals / sizeof vals[0]; ++j) {
      pipe_instr_write_reg_t instr;
      construct_instr_reg_write(
          dev_info->dev_id, &instr, addr + 4 * j, vals[j]);
      pipe_status_t sts = pipe_mgr_drv_ilist_add(&shdl,
                                                 dev_info,
                                                 &pipe_bit_map,
                                                 stage,
                                                 (uint8_t *)&instr,
                                                 sizeof instr);
      if (PIPE_SUCCESS != sts) {
        LOG_ERROR("Dev %d port %d Failed to add ibp chnl_fifo instr (%s)",
                  dev_info->dev_id,
                  port_id,
                  pipe_str_err(sts));
        PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
        return sts;
      }
      /* Set all channels used by this port except the first to zero. */
      vals[j] = 0;
    }
  }
  return PIPE_SUCCESS;
}

static pipe_status_t tof2_ipb_write_chnl_acc_ctrl(pipe_sess_hdl_t shdl,
                                                  rmt_dev_info_t *dev_info,
                                                  rmt_port_info_t *port_info) {
  bf_dev_port_t port_id = port_info->port_id;
  /* Take the reset values (for 400g) and scale them by the number of channels.
   */
  const int total_cells = 880;
  const int total_cells_per_chan = total_cells / TOF2_NUM_CHN_PER_IPB;
  const int cong_thr_max = 780;
  const int cong_thr_max_per_chan = cong_thr_max / TOF2_NUM_CHN_PER_IPB;
  const int cong_hys_max = 400;
  const int cong_hys_max_per_chan = cong_hys_max / TOF2_NUM_CHN_PER_IPB;
  const int cong_thr_cell_size = 80;

  int local_port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  int logical_pipe = dev_info->dev_cfg.dev_port_to_pipe(port_id);
  int ipb_num = tof2_ipb_local_port_to_ipb(local_port);
  if (ipb_num < 0) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }

  int chan_num = tof2_ipb_local_port_to_chan(local_port);
  int chan_cnt = tof2_parde_speed_to_chan_cnt(port_info->speed);

  pipe_bitmap_t pipe_bit_map;
  PIPE_BITMAP_INIT(&pipe_bit_map, PIPE_BMP_SIZE);
  PIPE_BITMAP_SET(&pipe_bit_map, logical_pipe);

  int min_cells = chan_cnt * total_cells_per_chan;
  int cong_thr = chan_cnt * cong_thr_max_per_chan;
  int cong_hys = chan_cnt * cong_hys_max_per_chan;
  if (port_info->use_custom_wm_bytes) {
    cong_thr = port_info->hi_wm_bytes / cong_thr_cell_size;
    cong_hys = port_info->lo_wm_bytes / cong_thr_cell_size;
  }
  uint32_t vals[3] = {0, 0, 0};
  setp_tof2_chnl_acc_ctrl_clear_max(vals, 0);
  setp_tof2_chnl_acc_ctrl_share_congest_hys(vals, 0);
  setp_tof2_chnl_acc_ctrl_share_congest_thr(vals, 0);
  setp_tof2_chnl_acc_ctrl_min_congest_hys(vals, cong_hys);
  setp_tof2_chnl_acc_ctrl_min_congest_thr(vals, cong_thr);
  setp_tof2_chnl_acc_ctrl_headroom(vals, 0);
  setp_tof2_chnl_acc_ctrl_share(vals, 0);
  setp_tof2_chnl_acc_ctrl_min(vals, min_cells);

  uint32_t base_addr =
      offsetof(tof2_reg,
               pipes[0]
                   .pardereg.pgstnreg.ipbprsr4reg[ipb_num]
                   .ipbreg.chan0_group.chnl_acc_ctrl.chnl_acc_ctrl_0_3);
  uint32_t chan_step =
      offsetof(tof2_reg,
               pipes[0]
                   .pardereg.pgstnreg.ipbprsr4reg[ipb_num]
                   .ipbreg.chan1_group.chnl_acc_ctrl.chnl_acc_ctrl_0_3) -
      base_addr;
  uint32_t stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(base_addr);
  for (int i = 0; i < chan_cnt; ++i) {
    int this_chan = chan_num + i;
    uint32_t addr = base_addr + chan_step * this_chan;
    pipe_instr_write_reg_t instr[3];
    construct_instr_reg_write(dev_info->dev_id, &instr[0], addr + 0, vals[0]);
    construct_instr_reg_write(dev_info->dev_id, &instr[1], addr + 4, vals[1]);
    construct_instr_reg_write(dev_info->dev_id, &instr[2], addr + 8, vals[2]);
    for (int j = 0; j < 3; ++j) {
      pipe_status_t sts = pipe_mgr_drv_ilist_add(&shdl,
                                                 dev_info,
                                                 &pipe_bit_map,
                                                 stage,
                                                 (uint8_t *)&instr[j],
                                                 sizeof instr[j]);
      if (PIPE_SUCCESS != sts) {
        LOG_ERROR("Dev %d port %d Failed to add ibp chnl_acc_ctrl instr (%s)",
                  dev_info->dev_id,
                  port_id,
                  pipe_str_err(sts));
        PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
        return sts;
      }
    }
    /* Set all channels used by this port except the first to zero. */
    vals[0] = vals[1] = vals[2] = 0;
  }
  return PIPE_SUCCESS;
}

static pipe_status_t tof2_parde_ipb_drain(rmt_dev_info_t *dev_info,
                                          bf_dev_port_t port_id) {
  int local_port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  int logical_pipe = dev_info->dev_cfg.dev_port_to_pipe(port_id);
  int ipb_num = tof2_ipb_local_port_to_ipb(local_port);
  if (ipb_num < 0) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }

  int chan_num = tof2_ipb_local_port_to_chan(local_port);
  bf_dev_pipe_t phy_pipe = 0;
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, logical_pipe, &phy_pipe);

  uint32_t base_addr = offsetof(tof2_reg,
                                pipes[phy_pipe]
                                    .pardereg.pgstnreg.ipbprsr4reg[ipb_num]
                                    .ipbreg.chan0_group.chnl_acc_stat);
  uint32_t chan_step = offsetof(tof2_reg,
                                pipes[phy_pipe]
                                    .pardereg.pgstnreg.ipbprsr4reg[ipb_num]
                                    .ipbreg.chan1_group.chnl_acc_stat) -
                       base_addr;

  uint32_t addr = base_addr + chan_num * chan_step;

  int cnt = 10;
  uint32_t ipb_cnt;
  do {
    uint32_t x = 0;
    lld_read_register(dev_info->dev_id, addr, &x);
    ipb_cnt = getp_tof2_chnl_acc_stat_cur(&x);
  } while (cnt-- && ipb_cnt);

  if (ipb_cnt) {
    bool is_model = false;
    bf_drv_device_type_get(dev_info->dev_id, &is_model);
    if (!is_model) {
      LOG_ERROR("Dev %d port %d IPB drain count %d",
                dev_info->dev_id,
                port_id,
                ipb_cnt);
      return PIPE_TRY_AGAIN;
    }
  }

  return PIPE_SUCCESS;
}

static pipe_status_t tof2_parde_epb_drain(rmt_dev_info_t *dev_info,
                                          bf_dev_port_t port_id) {
  int local_port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  int logical_pipe = dev_info->dev_cfg.dev_port_to_pipe(port_id);
  int epb_num = tof2_ipb_local_port_to_ipb(local_port);
  if (epb_num < 0) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }

  int chan_num = tof2_ipb_local_port_to_chan(local_port);
  bf_dev_pipe_t phy_pipe = 0;
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, logical_pipe, &phy_pipe);

  uint32_t base_prsr = offsetof(tof2_reg,
                                pipes[phy_pipe]
                                    .pardereg.pgstnreg.epbprsr4reg[epb_num]
                                    .epbreg.chan0_group.chnl_pktnum0);
  uint32_t base_bpss = offsetof(tof2_reg,
                                pipes[phy_pipe]
                                    .pardereg.pgstnreg.epbprsr4reg[epb_num]
                                    .epbreg.chan0_group.chnl_pktnum1);
  uint32_t base_inpt = offsetof(tof2_reg,
                                pipes[phy_pipe]
                                    .pardereg.pgstnreg.epbprsr4reg[epb_num]
                                    .epbreg.chan0_group.chnl_pktnum2);
  uint32_t chan_step = offsetof(tof2_reg,
                                pipes[phy_pipe]
                                    .pardereg.pgstnreg.epbprsr4reg[epb_num]
                                    .epbreg.chan1_group.chnl_pktnum0) -
                       base_prsr;

  uint32_t addr_prsr = base_prsr + chan_num * chan_step;
  uint32_t addr_bypass = base_bpss + chan_num * chan_step;
  uint32_t addr_input = base_inpt + chan_num * chan_step;

  int cnt = 10;
  uint32_t in_cnt;
  uint32_t p_cnt;
  uint32_t b_cnt;
  do {
    uint32_t x = 0;
    lld_read_register(dev_info->dev_id, addr_input, &x);
    in_cnt = getp_tof2_epb_chnl_pktnum2_reg_pbc_ff_cnt(&x);

    lld_read_register(dev_info->dev_id, addr_prsr, &x);
    p_cnt = getp_tof2_epb_chnl_pktnum0_reg_ppc_ff_cnt(&x);

    lld_read_register(dev_info->dev_id, addr_bypass, &x);
    b_cnt = getp_tof2_epb_chnl_pktnum1_reg_wpc_ff_cnt(&x);
  } while (cnt-- && (in_cnt || p_cnt || b_cnt));

  if (in_cnt || p_cnt || b_cnt) {
    bool is_model = false;
    bf_drv_device_type_get(dev_info->dev_id, &is_model);
    if (!is_model) {
      LOG_ERROR("Dev %d port %d EPB drain input fifo count %d",
                dev_info->dev_id,
                port_id,
                in_cnt);
      LOG_ERROR("Dev %d port %d EPB drain prsr fifo count %d",
                dev_info->dev_id,
                port_id,
                p_cnt);
      LOG_ERROR("Dev %d port %d EPB drain bypass fifo count %d",
                dev_info->dev_id,
                port_id,
                b_cnt);
      return PIPE_TRY_AGAIN;
    }
  }
  return PIPE_SUCCESS;
}

static pipe_status_t tof2_parde_dprsr_drain(rmt_dev_info_t *dev_info,
                                            bf_dev_port_t port_id,
                                            bool ing_1_egr_0) {
  int local_port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  int logical_pipe = dev_info->dev_cfg.dev_port_to_pipe(port_id);
  bf_dev_pipe_t phy_pipe = 0;
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, logical_pipe, &phy_pipe);
  int slice, mac, dprsr_chan;
  tof2_chan_to_dprsr_info(local_port, &slice, &mac, &dprsr_chan);
  /* There are no fields in the hdr_status and output status registers so we
   * will get a mask ourselves. */
  uint32_t msk = 0;
  if (mac == 0) {
    msk = (1 << dprsr_chan) << 16;
  } else if (mac == 2) {
    msk = (1 << dprsr_chan) << 8;
  } else {
    msk = 1 << dprsr_chan;
  }

  /*
   * First wait for the PHV count to go to zero.
   */
  uint32_t addr_sel = offsetof(
      tof2_reg,
      pipes[phy_pipe].pardereg.dprsrreg.dprsrreg.inp.icr.phv_count_sel);
  uint32_t addr_cnt = offsetof(
      tof2_reg, pipes[phy_pipe].pardereg.dprsrreg.dprsrreg.inp.icr.phv_count);

  lld_write_register(dev_info->dev_id, addr_sel, local_port);
  int cnt = 10;
  uint32_t phv_cnt;
  do {
    uint32_t x = 0;
    lld_read_register(dev_info->dev_id, addr_cnt, &x);
    if (ing_1_egr_0)
      phv_cnt = getp_tof2_dprsr_inp_phv_count_r_i_count(&x);
    else
      phv_cnt = getp_tof2_dprsr_inp_phv_count_r_e_count(&x);
  } while (cnt-- && phv_cnt);

  if (phv_cnt) {
    bool is_model = false;
    bf_drv_device_type_get(dev_info->dev_id, &is_model);
    if (!is_model) {
      LOG_ERROR("Dev %d port %d %sDprsr drain PHV count %d",
                dev_info->dev_id,
                port_id,
                ing_1_egr_0 ? "Ing" : "Egr",
                phv_cnt);
      return PIPE_TRY_AGAIN;
    }
  }

  /*
   * Next, wait for the hdr status to go idle.
   */
  uint32_t addr_hdr = ing_1_egr_0
                          ? offsetof(tof2_reg,
                                     pipes[phy_pipe]
                                         .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                                         .hir.h.hdr_status)
                          : offsetof(tof2_reg,
                                     pipes[phy_pipe]
                                         .pardereg.dprsrreg.dprsrreg.ho_e[slice]
                                         .her.h.hdr_status);
  cnt = 10;
  uint32_t hdr_sts;
  do {
    uint32_t x = 0;
    lld_read_register(dev_info->dev_id, addr_hdr, &x);
    hdr_sts = getp_tof2_dprsr_hdr_status_r_all_quiet(&x);
  } while (cnt-- && (~hdr_sts & msk));

  if (~hdr_sts & msk) {
    bool is_model = false;
    bf_drv_device_type_get(dev_info->dev_id, &is_model);
    if (!is_model) {
      LOG_ERROR("Dev %d port %d %sDprsr drain hdr sts 0x%x mask 0x%x",
                dev_info->dev_id,
                port_id,
                ing_1_egr_0 ? "Ing" : "Egr",
                hdr_sts,
                msk);
      return PIPE_TRY_AGAIN;
    }
  }

  /*
   * Next, wait for the output status to go idle.
   */
  uint32_t addr_out = ing_1_egr_0
                          ? offsetof(tof2_reg,
                                     pipes[phy_pipe]
                                         .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                                         .out_ingr.output_status)
                          : offsetof(tof2_reg,
                                     pipes[phy_pipe]
                                         .pardereg.dprsrreg.dprsrreg.ho_e[slice]
                                         .out_egr.output_status);
  cnt = 10;
  uint32_t out_sts;
  do {
    uint32_t x = 0;
    lld_read_register(dev_info->dev_id, addr_out, &x);
    out_sts = getp_tof2_dprsr_output_status_r_all_quiet(&x);
  } while (cnt-- && (~out_sts & msk));

  if (~out_sts & msk) {
    bool is_model = false;
    bf_drv_device_type_get(dev_info->dev_id, &is_model);
    if (!is_model) {
      LOG_ERROR("Dev %d port %d %sDprsr drain output sts 0x%x mask 0x%x",
                dev_info->dev_id,
                port_id,
                ing_1_egr_0 ? "Ing" : "Egr",
                out_sts,
                msk);
      return PIPE_TRY_AGAIN;
    }
  }

  return PIPE_SUCCESS;
}

static pipe_status_t tof2_parde_ebuf_drain(rmt_dev_info_t *dev_info,
                                           bf_dev_port_t port_id) {
  int local_port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  int logical_pipe = dev_info->dev_cfg.dev_port_to_pipe(port_id);
  bf_dev_pipe_t phy_pipe = 0;
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, logical_pipe, &phy_pipe);
  int chan_num = tof2_ipb_local_port_to_chan(local_port);
  int ebuf_grp = tof2_local_port_to_ebuf_grp(local_port);
  int ebuf_idx = tof2_local_port_to_ebuf_grp_idx(local_port);
  bool is_400 = tof2_ebuf_is400(local_port);

  uint32_t base_addr =
      is_400 ? offsetof(tof2_reg,
                        pipes[phy_pipe]
                            .pardereg.pgstnreg.ebuf900reg[ebuf_grp]
                            .ebuf400reg[ebuf_idx]
                            .chan_group[0]
                            .chnl_fifo_stat.chnl_fifo_stat_0_2)
             : offsetof(tof2_reg,
                        pipes[phy_pipe]
                            .pardereg.pgstnreg.ebuf900reg[ebuf_grp]
                            .ebuf100reg.chan_group[0]
                            .chnl_fifo_stat.chnl_fifo_stat_0_2);
  uint32_t chan_step =
      (is_400 ? offsetof(tof2_reg,
                         pipes[phy_pipe]
                             .pardereg.pgstnreg.ebuf900reg[ebuf_grp]
                             .ebuf400reg[ebuf_idx]
                             .chan_group[1]
                             .chnl_fifo_stat.chnl_fifo_stat_0_2)
              : offsetof(tof2_reg,
                         pipes[phy_pipe]
                             .pardereg.pgstnreg.ebuf900reg[ebuf_grp]
                             .ebuf100reg.chan_group[1]
                             .chnl_fifo_stat.chnl_fifo_stat_0_2)) -
      base_addr;
  uint32_t addr = is_400 ? base_addr + chan_num * chan_step
                         : (base_addr + (chan_num & 1) * chan_step);

  int cnt = 10;
  int bypass_empty, dprsr_empty, mac_empty;
  do {
    uint32_t data[2];
    lld_read_register(dev_info->dev_id, addr, data);
    lld_read_register(dev_info->dev_id, addr + 4, data + 1);
    bypass_empty = getp_tof2_ebuf400_chnl_fifo_stat_warp_fifo_empty(data);
    dprsr_empty = getp_tof2_ebuf400_chnl_fifo_stat_dprsr_fifo_empty(data);
    mac_empty = getp_tof2_ebuf400_chnl_fifo_stat_mac_fifo_empty(data);
  } while (cnt-- && (!bypass_empty || !dprsr_empty || !mac_empty));

  if (!bypass_empty || !dprsr_empty || !mac_empty) {
    bool is_model = false;
    bf_drv_device_type_get(dev_info->dev_id, &is_model);
    if (!is_model) {
      LOG_ERROR(
          "Dev %d port %d ebuf drain bypass_fifo_empty %d dprsr_fifo_empty %d "
          "mac_fifo_empty %d",
          dev_info->dev_id,
          port_id,
          bypass_empty,
          dprsr_empty,
          mac_empty);
      return PIPE_TRY_AGAIN;
    }
  }
  return PIPE_SUCCESS;
}

static pipe_status_t tof2_parde_cfg_ipb(pipe_sess_hdl_t shdl,
                                        rmt_dev_info_t *dev_info,
                                        bf_dev_port_t port_id) {
  pipe_status_t sts = PIPE_SUCCESS;
  rmt_port_info_t *port_info =
      pipe_mgr_get_port_info(dev_info->dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port_id);
    return PIPE_INVALID_ARG;
  }

  /* pipes.pardereg.pgstnreg.ipbprsr4reg[].ipbreg.chanX_group.meta_fifo_ctrl */
  sts =
      tof2_ipb_write_meta_fifo_ctrl(shdl, dev_info, port_id, port_info->speed);
  if (PIPE_SUCCESS != sts) {
    LOG_ERROR("Dev %d Failed to program meta_fifo_ctrl for port %d, sts %s",
              dev_info->dev_id,
              port_id,
              pipe_str_err(sts));
    return sts;
  }

  /* pipes.pardereg.pgstnreg.ipbprsr4reg[].ipbreg.chanX_group.chnl_fifo_ctrl */
  sts =
      tof2_ipb_write_chnl_fifo_ctrl(shdl, dev_info, port_id, port_info->speed);
  if (PIPE_SUCCESS != sts) {
    LOG_ERROR("Dev %d Failed to program fifo_ctrl for port %d, sts %s",
              dev_info->dev_id,
              port_id,
              pipe_str_err(sts));
    return sts;
  }

  /* pipes.pardereg.pgstnreg.ipbprsr4reg[].ipbreg.chanX_group.chnl_acc_ctrl */
  sts = tof2_ipb_write_chnl_acc_ctrl(shdl, dev_info, port_info);
  if (PIPE_SUCCESS != sts) {
    LOG_ERROR("Dev %d Failed to program acc_ctrl for port %d, sts %s",
              dev_info->dev_id,
              port_id,
              pipe_str_err(sts));
    return sts;
  }

  /* pipes.pardereg.pgstnreg.ipbprsr4reg[].ipbreg.chanX_group.chnl_ctrl
   * Program the first channel used by the port only, the others are ignored.
   * Set the following fields based on port speed:
   *  - init_dprsr_credit
   *  - init_dprsr_barrel_credit
   */
  sts = tof2_ipb_write_chan_ctrl(shdl, port_id, 0 /* version */, dev_info);
  if (PIPE_SUCCESS != sts) {
    LOG_ERROR("Dev %d Failed to program chan_ctrl for port %d, sts %s",
              dev_info->dev_id,
              port_id,
              pipe_str_err(sts));
    return sts;
  }

  return PIPE_SUCCESS;
}

static pipe_status_t tof2_prsr_seqno_reset(pipe_sess_hdl_t shdl,
                                           rmt_dev_info_t *dev_info,
                                           bf_dev_port_t port_id,
                                           bool ing_1_egr_0) {
  rmt_port_info_t *port_info =
      pipe_mgr_get_port_info(dev_info->dev_id, port_id);
  if (!port_info) return PIPE_INVALID_ARG;
  int local_port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  int logical_pipe = dev_info->dev_cfg.dev_port_to_pipe(port_id);
  int ipb_num = tof2_ipb_local_port_to_ipb(local_port);
  if (-1 == ipb_num) return PIPE_INVALID_ARG;
  int chan_cnt = tof2_parde_speed_to_chan_cnt(port_info->speed);
  int chan_num = tof2_ipb_local_port_to_chan(local_port);
  if (-1 == chan_num) return PIPE_INVALID_ARG;
  int first_prsr_id = 0, prsr_cnt = 0;
  tof2_prsr_chan_speed_to_prsr_id(
      chan_num, port_info->speed, &first_prsr_id, &prsr_cnt);

  pipe_bitmap_t pipe_bit_map;
  PIPE_BITMAP_INIT(&pipe_bit_map, PIPE_BMP_SIZE);
  PIPE_BITMAP_SET(&pipe_bit_map, logical_pipe);

  uint32_t vals[2] = {0, 0};
  if (chan_cnt == 1) {
    setp_tof2_prsr_reg_main_rspec_seq_reset_reset(vals, chan_num & 1, 1);
  } else {
    setp_tof2_prsr_reg_main_rspec_seq_reset_reset(vals, 0, 1);
    setp_tof2_prsr_reg_main_rspec_seq_reset_reset(vals, 1, 1);
  }
  for (int h = 0; h < 2; ++h) {
    uint32_t val = vals[h];
    for (int i = 0; i < prsr_cnt; ++i) {
      int prsr_id = first_prsr_id + i;
      uint32_t addr = ing_1_egr_0
                          ? offsetof(tof2_reg,
                                     pipes[0]
                                         .pardereg.pgstnreg.ipbprsr4reg[ipb_num]
                                         .prsr[prsr_id]
                                         .seq_reset)
                          : offsetof(tof2_reg,
                                     pipes[0]
                                         .pardereg.pgstnreg.epbprsr4reg[ipb_num]
                                         .prsr[prsr_id]
                                         .seq_reset);
      pipe_instr_write_reg_t instr;
      construct_instr_reg_write(dev_info->dev_id, &instr, addr, val);
      uint32_t stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(addr);
      pipe_status_t sts = pipe_mgr_drv_ilist_add(&shdl,
                                                 dev_info,
                                                 &pipe_bit_map,
                                                 stage,
                                                 (uint8_t *)&instr,
                                                 sizeof instr);
      if (PIPE_SUCCESS != sts) {
        LOG_ERROR("Dev %d port %d prsr %d Failed to add seqno %s (%s)",
                  dev_info->dev_id,
                  port_id,
                  prsr_id,
                  h ? "clear" : "reset",
                  pipe_str_err(sts));
        PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
        return sts;
      }
    }
  }
  return PIPE_SUCCESS;
}
static pipe_status_t tof2_prsr_cfg_chnl_rate(pipe_sess_hdl_t shdl,
                                             rmt_dev_info_t *dev_info,
                                             bf_dev_port_t port_id,
                                             bool ing_1_egr_0) {
  rmt_port_info_t *port_info =
      pipe_mgr_get_port_info(dev_info->dev_id, port_id);
  if (!port_info) return PIPE_INVALID_ARG;
  int local_port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  int logical_pipe = dev_info->dev_cfg.dev_port_to_pipe(port_id);
  int ipb_num = tof2_ipb_local_port_to_ipb(local_port);
  if (-1 == ipb_num) return PIPE_INVALID_ARG;
  int chan_num = tof2_ipb_local_port_to_chan(local_port);
  if (-1 == chan_num) return PIPE_INVALID_ARG;
  int first_prsr_id = 0, prsr_cnt = 0;
  tof2_prsr_chan_speed_to_prsr_id(
      chan_num, port_info->speed, &first_prsr_id, &prsr_cnt);

  pipe_bitmap_t pipe_bit_map;
  PIPE_BITMAP_INIT(&pipe_bit_map, PIPE_BMP_SIZE);
  PIPE_BITMAP_SET(&pipe_bit_map, logical_pipe);

  /* Treat 50g and below (these are the single channel cases for the parser) as
   * 50g.  Each time the port_rate_cfg register is written to a different value
   * it will reset the sequence number tracking by parser.  So, if two adjacent
   * single channel ports are added at different single channel speeds we do
   * not want the sequence number to reset when adding the second port as it can
   * affect the first port. */
  bf_port_speed_t speed = port_info->speed;
  if (speed == BF_SPEED_1G || speed == BF_SPEED_10G || speed == BF_SPEED_25G ||
      speed == BF_SPEED_40G || speed == BF_SPEED_50G ||
      speed == BF_SPEED_40G_R2 || speed == BF_SPEED_50G_CONS) {
    speed = BF_SPEED_50G;
  }
  uint32_t chan_rate = tof2_parde_speed_to_port_rate(speed);
  uint32_t val = 0;
  setp_tof2_prsr_reg_main_rspec_port_rate_cfg_chnl_rate(&val, 0, chan_rate);
  for (int i = 0; i < prsr_cnt; ++i) {
    int prsr_id = first_prsr_id + i;
    uint32_t addr = ing_1_egr_0
                        ? offsetof(tof2_reg,
                                   pipes[0]
                                       .pardereg.pgstnreg.ipbprsr4reg[ipb_num]
                                       .prsr[prsr_id]
                                       .port_rate_cfg)
                        : offsetof(tof2_reg,
                                   pipes[0]
                                       .pardereg.pgstnreg.epbprsr4reg[ipb_num]
                                       .prsr[prsr_id]
                                       .port_rate_cfg);
    pipe_instr_write_reg_t instr;
    construct_instr_reg_write(dev_info->dev_id, &instr, addr, val);
    uint32_t stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(addr);
    pipe_status_t sts = pipe_mgr_drv_ilist_add(
        &shdl, dev_info, &pipe_bit_map, stage, (uint8_t *)&instr, sizeof instr);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Dev %d ipb %d prsr %d Failed to add port_rate instr (%s)",
                dev_info->dev_id,
                ipb_num,
                prsr_id,
                pipe_str_err(sts));
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
      return sts;
    }
  }
  return PIPE_SUCCESS;
}

static pipe_status_t tof2_parde_cfg_i_chnl_rate(pipe_sess_hdl_t shdl,
                                                rmt_dev_info_t *dev_info,
                                                bf_dev_port_t port_id,
                                                bool is_remove) {
  /* Set up the following port rate registers using the ports in the dev_info:
   *  - pipes.pardereg.pgstnreg.s2preg.port_rate_cfg
   *  - pipes.pardereg.pgstnreg.parbreg.port_rate_cfg
   *  - pipes.pardereg.pgstnreg.pmergereg.ul.port_rate_cfg_8_5
   *  - pipes.pardereg.pgstnreg.pmergereg.ur.port_rate_cfg_8_5
   *  - pipes.pardereg.pgstnreg.pmergereg.ll0.port_rate_cfg_4_0
   *  - pipes.pardereg.pgstnreg.pmergereg.lr0.port_rate_cfg_4_0
   *  - pipes.pardereg.pgstnreg.ipbprsr4reg[].ipbreg.glb_group.port_rates
   *  - pipes.pardereg.pgstnreg.ipbprsr4reg[].prsr[].port_rate_cfg
   */
  pipe_status_t sts = PIPE_SUCCESS;
  int local_port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  int logical_pipe = dev_info->dev_cfg.dev_port_to_pipe(port_id);
  int ipb_num = tof2_ipb_local_port_to_ipb(local_port);
  if (-1 == ipb_num) return PIPE_INVALID_ARG;

  pipe_bitmap_t pipe_bit_map;
  PIPE_BITMAP_INIT(&pipe_bit_map, PIPE_BMP_SIZE);
  PIPE_BITMAP_SET(&pipe_bit_map, logical_pipe);

  /* All the registers except the parser use the same data for the port rate so
   * just get it once and then build all the instructions with it.  Parse Merge
   * needs a special port rate value though. */
  uint32_t val = tof2_parde_get_port_rate_cfg(
      dev_info, logical_pipe, ipb_num, is_remove ? port_id : -1);
  uint32_t val_pm = tof2_parde_pmerge_convert_port_rate_cfg(val, is_remove);
  uint32_t val_ipb = tof2_parde_xpb_convert_port_rate_cfg(val);
  uint32_t pmerge_l =
      ipb_num <= 4
          ? offsetof(
                tof2_reg,
                pipes[0]
                    .pardereg.pgstnreg.pmergereg.ll0.port_rate_cfg_4_0[ipb_num])
          : offsetof(tof2_reg,
                     pipes[0]
                         .pardereg.pgstnreg.pmergereg.ul
                         .port_rate_cfg_8_5[ipb_num - 5]);
  uint32_t pmerge_r =
      ipb_num <= 4
          ? offsetof(
                tof2_reg,
                pipes[0]
                    .pardereg.pgstnreg.pmergereg.lr0.port_rate_cfg_4_0[ipb_num])
          : offsetof(tof2_reg,
                     pipes[0]
                         .pardereg.pgstnreg.pmergereg.ur
                         .port_rate_cfg_8_5[ipb_num - 5]);
  uint32_t addr_data_pairs[][2] = {
      /* s2p rate */
      {offsetof(tof2_reg,
                pipes[0].pardereg.pgstnreg.s2preg.port_rate_cfg[ipb_num]),
       val},
      /* parb rate */
      {offsetof(tof2_reg,
                pipes[0].pardereg.pgstnreg.parbreg.left.port_rate_cfg[ipb_num]),
       val},
      {offsetof(
           tof2_reg,
           pipes[0].pardereg.pgstnreg.parbreg.right.port_rate_cfg[ipb_num]),
       val},
      /*pmerge rate */
      {pmerge_l, val_pm},
      /*pmerge rate */
      {pmerge_r, val_pm},
      /* IPB rate */
      {offsetof(tof2_reg,
                pipes[0]
                    .pardereg.pgstnreg.ipbprsr4reg[ipb_num]
                    .ipbreg.glb_group.port_rates),
       val_ipb}};

  for (size_t i = 0; i < sizeof addr_data_pairs / sizeof addr_data_pairs[0];
       ++i) {
    pipe_instr_write_reg_t instr;
    construct_instr_reg_write(
        dev_info->dev_id, &instr, addr_data_pairs[i][0], addr_data_pairs[i][1]);
    uint32_t stage =
        dev_info->dev_cfg.pcie_pipe_addr_get_stage(addr_data_pairs[i][0]);
    sts = pipe_mgr_drv_ilist_add(
        &shdl, dev_info, &pipe_bit_map, stage, (uint8_t *)&instr, sizeof instr);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Dev %d port %d Failed to add ing port_rates instr (%s)",
                dev_info->dev_id,
                port_id,
                pipe_str_err(sts));
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
      return sts;
    }
  }

  /* And also update the parser's port rate config, but only for port add.
   * Port delete does not need to reset the port mode in the parser and in the
   * case of two 50g channels where one is deleted resetting the port mode can
   * affect the adjacent channel. */
  if (!is_remove) sts = tof2_prsr_cfg_chnl_rate(shdl, dev_info, port_id, 1);

  return sts;
}

static pipe_status_t tof2_parde_i_dprsr_mac_en(pipe_sess_hdl_t shdl,
                                               rmt_dev_info_t *dev_info,
                                               bf_dev_port_t port_id,
                                               bool en_1_dis_0,
                                               bool fifo_clean_up) {
  pipe_status_t sts = PIPE_SUCCESS;
  int local_port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  int logical_pipe = dev_info->dev_cfg.dev_port_to_pipe(port_id);
  int ipb_num = tof2_ipb_local_port_to_ipb(local_port);
  if (-1 == ipb_num) return PIPE_INVALID_ARG;
  rmt_port_info_t *port_info =
      pipe_mgr_get_port_info(dev_info->dev_id, port_id);
  if (!port_info) return PIPE_INVALID_ARG;
  bool is_400g = port_info->speed == BF_SPEED_400G;
  int chan_num = tof2_ipb_local_port_to_chan(local_port);
  if (-1 == chan_num) return PIPE_INVALID_ARG;

  pipe_bitmap_t pipe_bit_map;
  PIPE_BITMAP_INIT(&pipe_bit_map, PIPE_BMP_SIZE);
  PIPE_BITMAP_SET(&pipe_bit_map, logical_pipe);

  int channel_count = tof2_parde_speed_to_chan_cnt(port_info->speed);
  uint32_t this_port_chan_mask = ((1u << channel_count) - 1) << chan_num;
  uint32_t dprsr_chan_on =
      tof2_parde_get_chan_en_mask(dev_info, logical_pipe, ipb_num);
  uint32_t dprsr_chan_off = dprsr_chan_on & ~this_port_chan_mask;

  uint32_t base_addr =
      offsetof(tof2_reg, pipes[0].pardereg.dprsrreg.dprsrreg.inp.icr.mac0_en);
  uint32_t mac_step =
      offsetof(tof2_reg, pipes[0].pardereg.dprsrreg.dprsrreg.inp.icr.mac1_en) -
      base_addr;
  uint32_t addr = base_addr + ipb_num * mac_step;
  uint32_t val = en_1_dis_0 ? dprsr_chan_on : dprsr_chan_off;

  pipe_instr_write_reg_t instr;
  construct_instr_reg_write(dev_info->dev_id, &instr, addr, val);
  uint32_t stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(addr);
  sts = pipe_mgr_drv_ilist_add(
      &shdl, dev_info, &pipe_bit_map, stage, (uint8_t *)&instr, sizeof instr);
  if (PIPE_SUCCESS != sts) {
    LOG_ERROR("Dev %d port %d Failed to add idprsr mac_en instr (%s)",
              dev_info->dev_id,
              port_id,
              pipe_str_err(sts));
    PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
    return sts;
  }

  if (en_1_dis_0) {
    /* When enabling toggle the enable from off to on (done just above) then off
     * to on again, done here in the for loop below. */
    for (int i = 0; i < 2; ++i) {
      val = i ? dprsr_chan_on : dprsr_chan_off;
      construct_instr_reg_write(dev_info->dev_id, &instr, addr, val);
      sts = pipe_mgr_drv_ilist_add(&shdl,
                                   dev_info,
                                   &pipe_bit_map,
                                   stage,
                                   (uint8_t *)&instr,
                                   sizeof instr);
      if (PIPE_SUCCESS != sts) {
        LOG_ERROR("Dev %d port %d Failed to add idprsr mac_en instr (%s)",
                  dev_info->dev_id,
                  port_id,
                  pipe_str_err(sts));
        PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
        return sts;
      }
    }
  }

  if (is_400g && !fifo_clean_up) {
    /*
     * Change EBUF credits from 24 to 20 after the enable.
     */

    /* Before changing the credits wait "for a while" */
    pipe_noop_instr_t noop_instr;
    construct_instr_noop(dev_info->dev_id, &noop_instr);
    for (int waiting = 0; waiting < 17; ++waiting) {
      stage = 0;
      sts = pipe_mgr_drv_ilist_add(&shdl,
                                   dev_info,
                                   &pipe_bit_map,
                                   stage,
                                   (uint8_t *)&noop_instr,
                                   sizeof noop_instr);
      if (PIPE_SUCCESS != sts) {
        LOG_ERROR(
            "Dev %d port %d Failed to add dprsr EBUF cred wait instr (%s)",
            dev_info->dev_id,
            port_id,
            pipe_str_err(sts));
        return sts;
      }
    }

    int slice = 0;
    tof2_chan_to_dprsr_info(local_port, &slice, NULL, NULL);
    /* 400g ports can be port 8, 16, 24, ... 56, or 64.  Convert to a zero
     * based index. */
    int which_400g_port = (local_port - 8) / 8;
    /* There are two 400g ports per slice, the channel index will either be zero
     * or eight depending on if we are the first or second 400g interface in the
     * slice. */
    int idx = (which_400g_port & 1) ? 8 : 0;
    addr = offsetof(tof2_reg,
                    pipes[0]
                        .pardereg.dprsrreg.dprsrreg.ho_e[slice]
                        .out_egr.cfg_ebuf.chnl[idx]);
    val = 20;
    construct_instr_reg_write(dev_info->dev_id, &instr, addr, val);
    stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(addr);
    sts = pipe_mgr_drv_ilist_add(
        &shdl, dev_info, &pipe_bit_map, stage, (uint8_t *)&instr, sizeof instr);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Dev %d port %d Failed to add dprsr EBUF cred instr (%s)",
                dev_info->dev_id,
                port_id,
                pipe_str_err(sts));
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
      return sts;
    }
  }

  return PIPE_SUCCESS;
}
static pipe_status_t tof2_parde_cfg_dprsr_chnl_rate(pipe_sess_hdl_t shdl,
                                                    rmt_dev_info_t *dev_info,
                                                    bf_dev_port_t port_id,
                                                    bool is_rmv) {
  pipe_status_t sts = PIPE_SUCCESS;
  int local_port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  int logical_pipe = dev_info->dev_cfg.dev_port_to_pipe(port_id);
  int ipb_num = tof2_ipb_local_port_to_ipb(local_port);
  if (-1 == ipb_num) return PIPE_INVALID_ARG;

  pipe_bitmap_t pipe_bit_map;
  PIPE_BITMAP_INIT(&pipe_bit_map, PIPE_BMP_SIZE);
  PIPE_BITMAP_SET(&pipe_bit_map, logical_pipe);

  /* All the registers except the parser use the same data for the port rate so
   * use a common helper to get its value. */
  uint32_t val = tof2_parde_get_port_rate_cfg(
      dev_info, logical_pipe, ipb_num, is_rmv ? port_id : -1);
  uint32_t base_addr = offsetof(
      tof2_reg, pipes[0].pardereg.dprsrreg.dprsrreg.inp.icr.mac0_rates);
  uint32_t mac_step =
      offsetof(tof2_reg,
               pipes[0].pardereg.dprsrreg.dprsrreg.inp.icr.mac1_rates) -
      base_addr;
  uint32_t addr = base_addr + ipb_num * mac_step;

  /* If this is a port remove then program the channels for the port to 50g. */
  if (is_rmv) {
    rmt_port_info_t *port_info =
        pipe_mgr_get_port_info(dev_info->dev_id, port_id);
    if (!port_info) {
      PIPE_MGR_DBGCHK(port_info);
      return PIPE_UNEXPECTED;
    }
    int x = tof2_parde_speed_to_port_rate(BF_SPEED_50G);
    int ch_cnt = tof2_parde_speed_to_chan_cnt(port_info->speed);
    int chan_base = tof2_ipb_local_port_to_chan(local_port);
    for (int ch = chan_base; ch < chan_base + ch_cnt; ++ch) {
      setp_tof2_dprsr_ic_regs_mac0_rates_chnl_rate(&val, ch, x);
    }
  }

  pipe_instr_write_reg_t instr;
  construct_instr_reg_write(dev_info->dev_id, &instr, addr, val);
  uint32_t stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(addr);
  sts = pipe_mgr_drv_ilist_add(
      &shdl, dev_info, &pipe_bit_map, stage, (uint8_t *)&instr, sizeof instr);
  if (PIPE_SUCCESS != sts) {
    LOG_ERROR("Dev %d port %d Failed to post dprsr port_rates instr (%s)",
              dev_info->dev_id,
              port_id,
              pipe_str_err(sts));
    PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
    return sts;
  }

  return PIPE_SUCCESS;
}

static pipe_status_t tof2_parde_cfg_e_chnl_rate(pipe_sess_hdl_t shdl,
                                                rmt_dev_info_t *dev_info,
                                                bf_dev_port_t port_id,
                                                bool is_remove) {
  pipe_status_t sts = PIPE_SUCCESS;
  int local_port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  int logical_pipe = dev_info->dev_cfg.dev_port_to_pipe(port_id);
  int ipb_num = tof2_ipb_local_port_to_ipb(local_port);
  if (-1 == ipb_num) return PIPE_INVALID_ARG;
  rmt_port_info_t *port_info =
      pipe_mgr_get_port_info(dev_info->dev_id, port_id);
  if (!port_info) return PIPE_INVALID_ARG;
  int chan_num = tof2_ipb_local_port_to_chan(local_port);
  if (-1 == chan_num) return PIPE_INVALID_ARG;

  pipe_bitmap_t pipe_bit_map;
  PIPE_BITMAP_INIT(&pipe_bit_map, PIPE_BMP_SIZE);
  PIPE_BITMAP_SET(&pipe_bit_map, logical_pipe);

  /* Get the port rate for EPB and P2S programming. */
  uint32_t val = tof2_parde_get_port_rate_cfg(
      dev_info, logical_pipe, ipb_num, is_remove ? port_id : -1);
  uint32_t val_epb = tof2_parde_xpb_convert_port_rate_cfg(val);
  uint32_t addr_data_pairs[][2] = {
      /* p2s rate */
      {offsetof(tof2_reg,
                pipes[0].pardereg.pgstnreg.p2sreg.port_rate_cfg[ipb_num]),
       val},
      /* EPB rate */
      {offsetof(tof2_reg,
                pipes[0]
                    .pardereg.pgstnreg.epbprsr4reg[ipb_num]
                    .epbreg.glb_group.port_rates),
       val_epb}};

  for (size_t i = 0; i < sizeof addr_data_pairs / sizeof addr_data_pairs[0];
       ++i) {
    pipe_instr_write_reg_t instr;
    construct_instr_reg_write(
        dev_info->dev_id, &instr, addr_data_pairs[i][0], addr_data_pairs[i][1]);
    uint32_t stage =
        dev_info->dev_cfg.pcie_pipe_addr_get_stage(addr_data_pairs[i][0]);
    sts = pipe_mgr_drv_ilist_add(
        &shdl, dev_info, &pipe_bit_map, stage, (uint8_t *)&instr, sizeof instr);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Dev %d port %d Failed to add egr port_rates instr (%s)",
                dev_info->dev_id,
                port_id,
                pipe_str_err(sts));
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
      return sts;
    }
  }

  /* And also update the parser's port rate config, but only for port add.
   * Port delete does not need to reset the port mode in the parser and in the
   * case of two 50g channels where one is deleted resetting the port mode can
   * affect the adjacent channel. */
  if (!is_remove) sts = tof2_prsr_cfg_chnl_rate(shdl, dev_info, port_id, 0);

  return sts;
}

static int tof2_epb_ur_timer_get(bool is_per_clk, bf_port_speeds_t speed) {
  switch (speed) {
    case BF_SPEED_NONE:
      return is_per_clk ? 31 : 18;
    case BF_SPEED_1G:
      return is_per_clk ? 1918 : 6;
    case BF_SPEED_10G:
      return is_per_clk ? 639 : 10;
    case BF_SPEED_25G:
      return is_per_clk ? 390 : 15;
    case BF_SPEED_40G:
    case BF_SPEED_40G_R2:
      return is_per_clk ? 153 : 9;
    case BF_SPEED_50G:
    case BF_SPEED_50G_CONS:
      return is_per_clk ? 307 : 23;
    case BF_SPEED_100G:
      return is_per_clk ? 149 : 22;
    case BF_SPEED_200G:
      return is_per_clk ? 70 : 21;
    case BF_SPEED_400G:
      return is_per_clk ? 31 : 18;
  }
  PIPE_MGR_DBGCHK(0);
  return -1;
}
static pipe_status_t tof2_epb_write_chnl_fifo_ctrl(pipe_sess_hdl_t shdl,
                                                   rmt_dev_info_t *dev_info,
                                                   bf_dev_port_t port_id,
                                                   bf_port_speeds_t speed) {
  const int wpc_fifo_total_size = 960 / 2; /* In units of 2 */
  const int wpc_fifo_sz_per_chan = wpc_fifo_total_size / TOF2_NUM_CHN_PER_IPB;
  const int wpc_byte_cnt_total_size = 0xF000;
  const int wpc_byte_cnt_sz_per_chan =
      wpc_byte_cnt_total_size / TOF2_NUM_CHN_PER_IPB;
  const int port_buf_total_size = 24;
  const int port_buf_sz_per_chan = port_buf_total_size / TOF2_NUM_CHN_PER_IPB;

  int local_port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  int logical_pipe = dev_info->dev_cfg.dev_port_to_pipe(port_id);
  int epb_num = tof2_ipb_local_port_to_ipb(local_port);
  if (-1 == epb_num) return PIPE_INVALID_ARG;
  int chan_num = tof2_ipb_local_port_to_chan(local_port);
  int chan_cnt = tof2_parde_speed_to_chan_cnt(speed);

  pipe_bitmap_t pipe_bit_map;
  PIPE_BITMAP_INIT(&pipe_bit_map, PIPE_BMP_SIZE);
  PIPE_BITMAP_SET(&pipe_bit_map, logical_pipe);

  int ebuf_cntr = tof2_epb_ur_timer_get(false, speed);
  int wpc_fifo_start = chan_num * wpc_fifo_sz_per_chan;
  int wpc_fifo_sz = chan_cnt * wpc_fifo_sz_per_chan;
  int wpc_fifo_end = wpc_fifo_start + wpc_fifo_sz - 1;
  int wpc_byte_cnt = chan_cnt * wpc_byte_cnt_sz_per_chan;
  int pbc_start = chan_num * port_buf_sz_per_chan;
  int pbc_end = pbc_start + chan_cnt * port_buf_sz_per_chan - 1;
  uint32_t vals[2] = {0, 0};
  setp_tof2_epb_chnl_fifo_ctrl_ebuf_counter(vals, ebuf_cntr);
  setp_tof2_epb_chnl_fifo_ctrl_wpc_fifo_begin(vals, wpc_fifo_start);
  setp_tof2_epb_chnl_fifo_ctrl_wpc_fifo_end(vals, wpc_fifo_end);
  setp_tof2_epb_chnl_fifo_ctrl_wpc_fifo_size(vals, 2 * wpc_fifo_sz);
  setp_tof2_epb_chnl_fifo_ctrl_wpc_fifo_bcnt_max(vals, wpc_byte_cnt);
  setp_tof2_epb_chnl_fifo_ctrl_pbc_fifo_begin(vals, pbc_start);
  setp_tof2_epb_chnl_fifo_ctrl_pbc_fifo_end(vals, pbc_end);

  uint32_t base_addr =
      offsetof(tof2_reg,
               pipes[0]
                   .pardereg.pgstnreg.epbprsr4reg[epb_num]
                   .epbreg.chan0_group.chnl_fifo_ctrl.chnl_fifo_ctrl_0_2);
  uint32_t chan_step =
      offsetof(tof2_reg,
               pipes[0]
                   .pardereg.pgstnreg.epbprsr4reg[epb_num]
                   .epbreg.chan1_group.chnl_fifo_ctrl.chnl_fifo_ctrl_0_2) -
      base_addr;
  uint32_t stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(base_addr);
  for (int i = 0; i < chan_cnt; ++i) {
    int this_chan = chan_num + i;
    uint32_t addr = base_addr + chan_step * this_chan;
    for (size_t j = 0; j < sizeof vals / sizeof vals[0]; ++j) {
      pipe_instr_write_reg_t instr;
      construct_instr_reg_write(
          dev_info->dev_id, &instr, addr + 4 * j, vals[j]);
      pipe_status_t sts = pipe_mgr_drv_ilist_add(&shdl,
                                                 dev_info,
                                                 &pipe_bit_map,
                                                 stage,
                                                 (uint8_t *)&instr,
                                                 sizeof instr);
      if (PIPE_SUCCESS != sts) {
        LOG_ERROR("Dev %d port %d Failed to add ebp chnl_fifo_ctrl instr (%s)",
                  dev_info->dev_id,
                  port_id,
                  pipe_str_err(sts));
        PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
        return sts;
      }
      /* Set all channels used by this port except the first to zero. */
      vals[j] = 0;
    }
  }
  return PIPE_SUCCESS;
}

static pipe_status_t tof2_epb_write_chnl_ctrl(pipe_sess_hdl_t shdl,
                                              rmt_dev_info_t *dev_info,
                                              bf_dev_port_t port_id,
                                              bf_port_speeds_t speed) {
  const int dprsr_cred_max = 0x840;
  const int dprsr_cred_per_chan = dprsr_cred_max / TOF2_NUM_CHN_PER_IPB;
  const int ebuf_cred_max = 32;
  const int ebuf_cred_per_chan = ebuf_cred_max / TOF2_NUM_CHN_PER_IPB;

  int local_port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  int logical_pipe = dev_info->dev_cfg.dev_port_to_pipe(port_id);
  int epb_num = tof2_ipb_local_port_to_ipb(local_port);
  if (epb_num < 0) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }

  int chan_num = tof2_ipb_local_port_to_chan(local_port);
  int chan_cnt = tof2_parde_speed_to_chan_cnt(speed);

  struct pipe_mgr_tof2_ebuf_ctx *ctx =
      &pipe_mgr_ebuf_ctx(dev_info->dev_id)->tof2;
  if (!ctx) return PIPE_INVALID_ARG;
  int dprsr_cred = chan_cnt * dprsr_cred_per_chan;
  int ebuf_cred = chan_cnt * ebuf_cred_per_chan;
  uint32_t vals[] = {0, 0};

  setp_tof2_epb_chnl_ctrl_meta_opt(vals,
                                   ctx->reg[logical_pipe].meta_opt[epb_num]);
  setp_tof2_epb_chnl_ctrl_pipeid(vals, logical_pipe);
  setp_tof2_epb_chnl_ctrl_init_dprsr_credit(vals, dprsr_cred);
  setp_tof2_epb_chnl_ctrl_init_ebuf_credit(vals, ebuf_cred);

  pipe_bitmap_t pipe_bit_map;
  PIPE_BITMAP_INIT(&pipe_bit_map, PIPE_BMP_SIZE);
  PIPE_BITMAP_SET(&pipe_bit_map, logical_pipe);

  uint32_t base_addr =
      offsetof(tof2_reg,
               pipes[0]
                   .pardereg.pgstnreg.epbprsr4reg[epb_num]
                   .epbreg.chan0_group.chnl_ctrl.chnl_ctrl_0_2);
  uint32_t chan_step =
      offsetof(tof2_reg,
               pipes[0]
                   .pardereg.pgstnreg.epbprsr4reg[epb_num]
                   .epbreg.chan1_group.chnl_ctrl.chnl_ctrl_0_2) -
      base_addr;
  uint32_t stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(base_addr);
  for (int i = 0; i < chan_cnt; ++i) {
    int this_chan = chan_num + i;
    uint32_t addr = base_addr + chan_step * this_chan;
    pipe_instr_write_reg_t instr[2];
    construct_instr_reg_write(dev_info->dev_id, &instr[0], addr, vals[0]);
    construct_instr_reg_write(dev_info->dev_id, &instr[1], addr + 4, vals[1]);
    for (size_t j = 0; j < sizeof instr / sizeof instr[0]; ++j) {
      pipe_status_t sts = pipe_mgr_drv_ilist_add(&shdl,
                                                 dev_info,
                                                 &pipe_bit_map,
                                                 stage,
                                                 (uint8_t *)&instr[j],
                                                 sizeof instr[j]);
      if (PIPE_SUCCESS != sts) {
        LOG_ERROR("Dev %d port %d Failed to add ebp chnl_ctrl instr (%s)",
                  dev_info->dev_id,
                  port_id,
                  pipe_str_err(sts));
        PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
        return sts;
      }
    }
    /* Set all channels used by this port except the first to zero. */
    vals[0] = vals[1] = 0;
  }
  return PIPE_SUCCESS;
}

static pipe_status_t tof2_parde_cfg_epb(pipe_sess_hdl_t shdl,
                                        rmt_dev_info_t *dev_info,
                                        bf_dev_port_t port_id) {
  pipe_status_t sts = PIPE_SUCCESS;
  rmt_port_info_t *port_info =
      pipe_mgr_get_port_info(dev_info->dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port_id);
    return PIPE_INVALID_ARG;
  }

  sts =
      tof2_epb_write_chnl_fifo_ctrl(shdl, dev_info, port_id, port_info->speed);
  if (PIPE_SUCCESS != sts) {
    LOG_ERROR("Dev %d port %d failed to set epb chan fifo sts %s",
              dev_info->dev_id,
              port_id,
              pipe_str_err(sts));
    return sts;
  }

  sts = tof2_epb_write_chnl_ctrl(shdl, dev_info, port_id, port_info->speed);
  if (PIPE_SUCCESS != sts) {
    LOG_ERROR("Dev %d port %d failed to set epb chan ctrl sts %s",
              dev_info->dev_id,
              port_id,
              pipe_str_err(sts));
    return sts;
  }

  return sts;
}

static int tof2_parde_port_speed_to_p2s_tm_cred(rmt_dev_info_t *dev_info,
                                                bf_port_speeds_t speed) {
  if (dev_info->part_rev == BF_SKU_CHIP_PART_REV_A0) {
    switch (speed) {
      case BF_SPEED_NONE:
      case BF_SPEED_1G:
      case BF_SPEED_10G:
      case BF_SPEED_25G:
      case BF_SPEED_40G:
      case BF_SPEED_40G_R2:
      case BF_SPEED_50G:
      case BF_SPEED_50G_CONS:
        return 6;
      case BF_SPEED_100G:
        return 12;
      case BF_SPEED_200G:
        return 24;
      case BF_SPEED_400G:
        return 48;
    }
  } else {
    switch (speed) {
      case BF_SPEED_NONE:
      case BF_SPEED_1G:
      case BF_SPEED_10G:
      case BF_SPEED_25G:
      case BF_SPEED_40G:
      case BF_SPEED_40G_R2:
      case BF_SPEED_50G:
      case BF_SPEED_50G_CONS:
        return 6;
      case BF_SPEED_100G:
        return 16;
      case BF_SPEED_200G:
        return 32;
      case BF_SPEED_400G:
        return 48;
    }
  }
  return 0;
}
static pipe_status_t tof2_parde_cfg_p2s_cred(pipe_sess_hdl_t shdl,
                                             rmt_dev_info_t *dev_info,
                                             int logical_pipe,
                                             int epb_num) {
  const int epb_cred_400g = 24;
  const int epb_cred_per_chan = epb_cred_400g / TOF2_NUM_CHN_PER_IPB;

  /* Credits are split into two registers each holding four channels. */
  uint32_t epb_credits[2] = {0, 0};
  uint32_t tm_credits[2] = {0, 0};
  for (int i = 0; i < TOF2_NUM_CHN_PER_IPB;) {
    bf_dev_port_t p_local = epb_num * TOF2_NUM_CHN_PER_IPB + i;
    bf_dev_port_t p = dev_info->dev_cfg.make_dev_port(logical_pipe, p_local);
    rmt_port_info_t *port_info = pipe_mgr_get_port_info(dev_info->dev_id, p);
    if (port_info) {
      int chan_cnt = tof2_parde_speed_to_chan_cnt(port_info->speed);
      int tm_cred_val =
          tof2_parde_port_speed_to_p2s_tm_cred(dev_info, port_info->speed);
      setp_tof2_p2s_reg_epb_cred_wr_amt(
          &epb_credits[i >> 2], i & 3, chan_cnt * epb_cred_per_chan);
      setp_tof2_p2s_reg_tm_cred_cred(&tm_credits[i >> 2], i & 3, tm_cred_val);
      i += chan_cnt;
    } else {
      ++i;
    }
  }

  pipe_bitmap_t pipe_bit_map;
  PIPE_BITMAP_INIT(&pipe_bit_map, PIPE_BMP_SIZE);
  PIPE_BITMAP_SET(&pipe_bit_map, logical_pipe);

  uint32_t epb_cred_base_addr =
      offsetof(tof2_reg, pipes[0].pardereg.pgstnreg.p2sreg.epb_cred_wr[0]);
  uint32_t epb_addr = epb_cred_base_addr + 8 * epb_num;
  uint32_t stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(epb_addr);
  for (size_t i = 0; i < sizeof epb_credits / sizeof epb_credits[0]; ++i) {
    pipe_instr_write_reg_t instr;
    construct_instr_reg_write(
        dev_info->dev_id, &instr, epb_addr + 4 * i, epb_credits[i]);
    pipe_status_t sts = pipe_mgr_drv_ilist_add(
        &shdl, dev_info, &pipe_bit_map, stage, (uint8_t *)&instr, sizeof instr);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Dev %d Failed to add p2s EPB cred instr (%s)",
                dev_info->dev_id,
                pipe_str_err(sts));
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
      return sts;
    }
  }

  uint32_t tm_cred_base_addr =
      offsetof(tof2_reg, pipes[0].pardereg.pgstnreg.p2sreg.tm_cred[0]);
  uint32_t tm_addr = tm_cred_base_addr + 8 * epb_num;
  for (size_t i = 0; i < sizeof tm_credits / sizeof tm_credits[0]; ++i) {
    pipe_instr_write_reg_t instr;
    construct_instr_reg_write(
        dev_info->dev_id, &instr, tm_addr + 4 * i, tm_credits[i]);
    pipe_status_t sts = pipe_mgr_drv_ilist_add(
        &shdl, dev_info, &pipe_bit_map, stage, (uint8_t *)&instr, sizeof instr);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Dev %d Failed to add p2s TM cred instr (%s)",
                dev_info->dev_id,
                pipe_str_err(sts));
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
      return sts;
    }
  }
  return PIPE_SUCCESS;
}

static pipe_status_t tof2_parde_ebuf_chnl_fifo_ctrl(pipe_sess_hdl_t shdl,
                                                    rmt_dev_info_t *dev_info,
                                                    bf_dev_port_t port_id) {
  const int w_fifo_total_sz = 32;
  const int w_fifo_sz_per_chan = w_fifo_total_sz / TOF2_NUM_CHN_PER_IPB;
  const int d_fifo_total_sz = 24;
  const int d_fifo_sz_per_chan = d_fifo_total_sz / TOF2_NUM_CHN_PER_IPB;
  const int m_fifo_total_sz = 32;
  const int m_fifo_sz_per_chan = m_fifo_total_sz / TOF2_NUM_CHN_PER_IPB;
  /* Smaller sizes for ebuf100 and only two channels per rather than eight. */
  const int w_fifo100_total_sz = 8;
  const int w_fifo100_sz_per_chan = w_fifo100_total_sz / 2;
  const int d_fifo100_total_sz = 16;
  const int d_fifo100_sz_per_chan = d_fifo100_total_sz / 2;
  const int m_fifo100_total_sz = 8;
  const int m_fifo100_sz_per_chan = m_fifo100_total_sz / 2;

  rmt_port_info_t *port_info =
      pipe_mgr_get_port_info(dev_info->dev_id, port_id);
  if (!port_info) return PIPE_INVALID_ARG;
  int local_port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  int logical_pipe = dev_info->dev_cfg.dev_port_to_pipe(port_id);
  int ebuf_grp = tof2_local_port_to_ebuf_grp(local_port);
  if (-1 == ebuf_grp) return PIPE_INVALID_ARG;
  int ebuf_idx = tof2_local_port_to_ebuf_grp_idx(local_port);
  if (-1 == ebuf_idx) return PIPE_INVALID_ARG;
  bool is_400 = tof2_ebuf_is400(local_port);
  int chan_num = tof2_ipb_local_port_to_chan(local_port);
  int chan_cnt = tof2_parde_speed_to_chan_cnt(port_info->speed);
  if (!is_400) {
    /* Each ebuf100 has 2 channels only so map chan_num to 0 or 1.  Each ebuf100
     * also has its own fifos so the code below that calculates fifo starting
     * offsets should assume chan_num is either 0 or 1. */
    chan_num &= 1;
  }

  pipe_bitmap_t pipe_bit_map;
  PIPE_BITMAP_INIT(&pipe_bit_map, PIPE_BMP_SIZE);
  PIPE_BITMAP_SET(&pipe_bit_map, logical_pipe);

  uint32_t chnl_ctrl_base_addr =
      is_400 ? offsetof(tof2_reg,
                        pipes[0]
                            .pardereg.pgstnreg.ebuf900reg[ebuf_grp]
                            .ebuf400reg[ebuf_idx]
                            .chan_group[0]
                            .chnl_ctrl)
             : offsetof(tof2_reg,
                        pipes[0]
                            .pardereg.pgstnreg.ebuf900reg[ebuf_grp]
                            .ebuf100reg.chan_group[0]
                            .chnl_ctrl);
  uint32_t chnl_ctrl_chan_step =
      (is_400 ? offsetof(tof2_reg,
                         pipes[0]
                             .pardereg.pgstnreg.ebuf900reg[ebuf_grp]
                             .ebuf400reg[ebuf_idx]
                             .chan_group[1]
                             .chnl_ctrl)
              : offsetof(tof2_reg,
                         pipes[0]
                             .pardereg.pgstnreg.ebuf900reg[ebuf_grp]
                             .ebuf100reg.chan_group[1]
                             .chnl_ctrl)) -
      chnl_ctrl_base_addr;
  uint32_t base_addr =
      is_400 ? offsetof(tof2_reg,
                        pipes[0]
                            .pardereg.pgstnreg.ebuf900reg[ebuf_grp]
                            .ebuf400reg[ebuf_idx]
                            .chan_group[0]
                            .chnl_fifo_ctrl.chnl_fifo_ctrl_0_2)
             : offsetof(tof2_reg,
                        pipes[0]
                            .pardereg.pgstnreg.ebuf900reg[ebuf_grp]
                            .ebuf100reg.chan_group[0]
                            .chnl_fifo_ctrl.chnl_fifo_ctrl_0_2);
  uint32_t chan_step =
      (is_400 ? offsetof(tof2_reg,
                         pipes[0]
                             .pardereg.pgstnreg.ebuf900reg[ebuf_grp]
                             .ebuf400reg[ebuf_idx]
                             .chan_group[1]
                             .chnl_fifo_ctrl.chnl_fifo_ctrl_0_2)
              : offsetof(tof2_reg,
                         pipes[0]
                             .pardereg.pgstnreg.ebuf900reg[ebuf_grp]
                             .ebuf100reg.chan_group[1]
                             .chnl_fifo_ctrl.chnl_fifo_ctrl_0_2)) -
      base_addr;
  /* The ebuf100 block only has two channels while the ebuf400 has all eight.
   * The 100 block only needs two because they are spread over four instances
   * of the ebuf900 block. */
  uint32_t chnl_ctrl_addr =
      (chnl_ctrl_base_addr + chan_num * chnl_ctrl_chan_step);
  uint32_t addr = (base_addr + chan_num * chan_step);
  uint32_t stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(addr);

  uint32_t vals[] = {0, 0};
  if (is_400) {
    int w_fifo_begin = chan_num * w_fifo_sz_per_chan;
    int w_fifo_sz = chan_cnt * w_fifo_sz_per_chan;
    int w_fifo_end = w_fifo_begin + w_fifo_sz - 1;
    int d_fifo_begin = chan_num * d_fifo_sz_per_chan;
    int d_fifo_sz = chan_cnt * d_fifo_sz_per_chan;
    int d_fifo_end = d_fifo_begin + d_fifo_sz - 1;
    int m_fifo_begin = chan_num * m_fifo_sz_per_chan;
    int m_fifo_sz = chan_cnt * m_fifo_sz_per_chan;
    int m_fifo_end = m_fifo_begin + m_fifo_sz - 1;
    setp_tof2_ebuf400_chnl_fifo_ctrl_warp_fifo_begin(vals, w_fifo_begin);
    setp_tof2_ebuf400_chnl_fifo_ctrl_warp_fifo_end(vals, w_fifo_end);
    setp_tof2_ebuf400_chnl_fifo_ctrl_warp_fifo_size(vals, w_fifo_sz);
    setp_tof2_ebuf400_chnl_fifo_ctrl_dprsr_fifo_begin(vals, d_fifo_begin);
    setp_tof2_ebuf400_chnl_fifo_ctrl_dprsr_fifo_end(vals, d_fifo_end);
    setp_tof2_ebuf400_chnl_fifo_ctrl_dprsr_fifo_size(vals, d_fifo_sz);
    setp_tof2_ebuf400_chnl_fifo_ctrl_mac_fifo_begin(vals, m_fifo_begin);
    setp_tof2_ebuf400_chnl_fifo_ctrl_mac_fifo_end(vals, m_fifo_end);
    setp_tof2_ebuf400_chnl_fifo_ctrl_mac_fifo_size(vals, m_fifo_sz);
  } else {
    int w_fifo_begin = chan_num * w_fifo100_sz_per_chan;
    int w_fifo_sz = chan_cnt * w_fifo100_sz_per_chan;
    int w_fifo_end = w_fifo_begin + w_fifo_sz - 1;
    int d_fifo_begin = chan_num * d_fifo100_sz_per_chan;
    int d_fifo_sz = chan_cnt * d_fifo100_sz_per_chan;
    int d_fifo_end = d_fifo_begin + d_fifo_sz - 1;
    int m_fifo_begin = chan_num * m_fifo100_sz_per_chan;
    int m_fifo_sz = chan_cnt * m_fifo100_sz_per_chan;
    int m_fifo_end = m_fifo_begin + m_fifo_sz - 1;
    setp_tof2_ebuf100_chnl_fifo_ctrl_warp_fifo_begin(vals, w_fifo_begin);
    setp_tof2_ebuf100_chnl_fifo_ctrl_warp_fifo_end(vals, w_fifo_end);
    setp_tof2_ebuf100_chnl_fifo_ctrl_warp_fifo_size(vals, w_fifo_sz);
    setp_tof2_ebuf100_chnl_fifo_ctrl_dprsr_fifo_begin(vals, d_fifo_begin);
    setp_tof2_ebuf100_chnl_fifo_ctrl_dprsr_fifo_end(vals, d_fifo_end);
    setp_tof2_ebuf100_chnl_fifo_ctrl_dprsr_fifo_size(vals, d_fifo_sz);
    setp_tof2_ebuf100_chnl_fifo_ctrl_mac_fifo_begin(vals, m_fifo_begin);
    setp_tof2_ebuf100_chnl_fifo_ctrl_mac_fifo_end(vals, m_fifo_end);
    setp_tof2_ebuf100_chnl_fifo_ctrl_mac_fifo_size(vals, m_fifo_sz);
  }

  /* Write all channels used by the port.  We'll put the configured data into
   * the first channel and zeros into all other channels. */
  for (int c = 0; c < chan_cnt; ++c) {
    pipe_instr_write_reg_t instr;
    pipe_status_t sts;
    for (size_t i = 0; i < sizeof vals / sizeof vals[0]; ++i) {
      construct_instr_reg_write(
          dev_info->dev_id, &instr, addr + 4 * i, vals[i]);
      sts = pipe_mgr_drv_ilist_add(&shdl,
                                   dev_info,
                                   &pipe_bit_map,
                                   stage,
                                   (uint8_t *)&instr,
                                   sizeof instr);
      if (PIPE_SUCCESS != sts) {
        LOG_ERROR("Dev %d port %d Failed to add ebuf chan fifo instr (%s)",
                  dev_info->dev_id,
                  port_id,
                  pipe_str_err(sts));
        PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
        return sts;
      }
      vals[i] = 0;
    }
    addr += chan_step;

    /* Now write the chnl_ctrl to first set and then clear chnl_ctrl so that
     * the values written above are applied. */
    uint32_t chnl_ctrl_val = 0;
    setp_tof2_ebuf400_chnl_ctrl_chnl_clear(&chnl_ctrl_val, 1);
    setp_tof2_ebuf400_chnl_ctrl_allow_drop(&chnl_ctrl_val, 0);
    setp_tof2_ebuf400_chnl_ctrl_ct_timer(&chnl_ctrl_val, 8);
    construct_instr_reg_write(
        dev_info->dev_id, &instr, chnl_ctrl_addr, chnl_ctrl_val);
    sts = pipe_mgr_drv_ilist_add(
        &shdl, dev_info, &pipe_bit_map, stage, (uint8_t *)&instr, sizeof instr);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Dev %d port %d Failed to add ebuf chan ctrl instr (%s)",
                dev_info->dev_id,
                port_id,
                pipe_str_err(sts));
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
      return sts;
    }

    setp_tof2_ebuf400_chnl_ctrl_chnl_clear(&chnl_ctrl_val, 0);
    construct_instr_reg_write(
        dev_info->dev_id, &instr, chnl_ctrl_addr, chnl_ctrl_val);
    sts = pipe_mgr_drv_ilist_add(
        &shdl, dev_info, &pipe_bit_map, stage, (uint8_t *)&instr, sizeof instr);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Dev %d port %d Failed to add ebuf chan ctrl instr (%s)",
                dev_info->dev_id,
                port_id,
                pipe_str_err(sts));
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
      return sts;
    }
    chnl_ctrl_addr += chnl_ctrl_chan_step;
  }
  return PIPE_SUCCESS;
}

static pipe_status_t tof2_parde_ebuf_chnl_seq(pipe_sess_hdl_t shdl,
                                              rmt_dev_info_t *dev_info,
                                              bf_dev_port_t port_id) {
  uint8_t chan_assignment[TOF2_NUM_CHN_PER_EPB] = {0, 1, 2, 3, 4, 5, 6, 7};
  uint8_t chan_mapping[TOF2_NUM_CHN_PER_EPB] = {0, 4, 2, 6, 1, 5, 3, 7};

  /* EBuf100 in 100g mode uses a channel sequence of 0 (chan 0 each cycle). */
  uint32_t chan_assignment_100_100g = 0;
  /* EBuf100 in non-100g mode uses an alternating channel sequence of 0 and 1.
   */
  uint32_t chan_assignment_100_50g = 0;
  for (int i = 0; i < TOF2_NUM_CHN_PER_EPB; ++i)
    chan_assignment_100_50g |= (i & 1) << (3 * i);

  int local_port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  int logical_pipe = dev_info->dev_cfg.dev_port_to_pipe(port_id);
  int epb_num = tof2_ipb_local_port_to_ipb(local_port);
  if (-1 == epb_num) return PIPE_INVALID_ARG;
  int ebuf_grp = tof2_local_port_to_ebuf_grp(local_port);
  if (-1 == ebuf_grp) return PIPE_INVALID_ARG;
  int ebuf_idx = tof2_local_port_to_ebuf_grp_idx(local_port);
  if (-1 == ebuf_idx) return PIPE_INVALID_ARG;
  bool is_400 = tof2_ebuf_is400(local_port);

  for (int i = 0; i < TOF2_NUM_CHN_PER_EPB;) {
    bf_dev_port_t p_local = epb_num * TOF2_NUM_CHN_PER_EPB + i;
    bf_dev_port_t p = dev_info->dev_cfg.make_dev_port(logical_pipe, p_local);
    rmt_port_info_t *port_info = pipe_mgr_get_port_info(dev_info->dev_id, p);
    bf_port_speed_t speed = port_info ? port_info->speed : BF_SPEED_NONE;
    int step = tof2_parde_speed_to_chan_cnt(speed);
    for (int j = 0; j < step; ++j) {
      if ((i + j) >= TOF2_NUM_CHN_PER_EPB) {
        LOG_ERROR("Dev %d port %d channels mapping failure",
                  dev_info->dev_id,
                  port_id);
        return PIPE_UNEXPECTED;
      }
      chan_assignment[i + j] = i;
    }
    i += step;
  }
  uint32_t vals[2] = {0, 0};
  if (is_400) {
    uint32_t seq = 0;
    for (int i = 0; i < TOF2_NUM_CHN_PER_EPB; ++i) {
      int val = chan_assignment[chan_mapping[i]];
      seq |= val << (3 * i);
    }
    setp_tof2_ebuf400_glb_ctrl_mac_channel_seq(vals, seq);
    setp_tof2_ebuf400_glb_ctrl_channel_seq(vals, seq);
  } else {
    /* Set the channel sequence based on port speed. */
    rmt_port_info_t *port_info =
        pipe_mgr_get_port_info(dev_info->dev_id, port_id);
    bf_port_speed_t speed = port_info ? port_info->speed : BF_SPEED_NONE;
    uint32_t seq = speed == BF_SPEED_100G ? chan_assignment_100_100g
                                          : chan_assignment_100_50g;
    setp_tof2_ebuf100_glb_ctrl_mac_channel_seq(vals, seq);
    setp_tof2_ebuf100_glb_ctrl_channel_seq(vals, seq);
  }

  pipe_bitmap_t pipe_bit_map;
  PIPE_BITMAP_INIT(&pipe_bit_map, PIPE_BMP_SIZE);
  PIPE_BITMAP_SET(&pipe_bit_map, logical_pipe);

  uint32_t addrs[2];
  addrs[0] = is_400 ? offsetof(tof2_reg,
                               pipes[0]
                                   .pardereg.pgstnreg.ebuf900reg[ebuf_grp]
                                   .ebuf400reg[ebuf_idx]
                                   .glb_group.glb_ctrl.glb_ctrl_0_2)
                    : offsetof(tof2_reg,
                               pipes[0]
                                   .pardereg.pgstnreg.ebuf900reg[ebuf_grp]
                                   .ebuf100reg.glb_group.glb_ctrl.glb_ctrl_0_2);
  addrs[1] = is_400 ? offsetof(tof2_reg,
                               pipes[0]
                                   .pardereg.pgstnreg.ebuf900reg[ebuf_grp]
                                   .ebuf400reg[ebuf_idx]
                                   .glb_group.glb_ctrl.glb_ctrl_1_2)
                    : offsetof(tof2_reg,
                               pipes[0]
                                   .pardereg.pgstnreg.ebuf900reg[ebuf_grp]
                                   .ebuf100reg.glb_group.glb_ctrl.glb_ctrl_1_2);
  uint32_t stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(addrs[0]);

  for (size_t i = 0; i < sizeof addrs / sizeof addrs[0]; ++i) {
    pipe_instr_write_reg_t instr;
    construct_instr_reg_write(dev_info->dev_id, &instr, addrs[i], vals[i]);
    pipe_status_t sts = pipe_mgr_drv_ilist_add(
        &shdl, dev_info, &pipe_bit_map, stage, (uint8_t *)&instr, sizeof instr);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Dev %d port %d Failed to add ebuf chan seq instr (%s)",
                dev_info->dev_id,
                port_id,
                pipe_str_err(sts));
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
      return sts;
    }
  }
  return PIPE_SUCCESS;
}

static pipe_status_t tof2_parde_ebuf_port_en(pipe_sess_hdl_t shdl,
                                             rmt_dev_info_t *dev_info,
                                             bf_dev_port_t port_id,
                                             bool en_1_dis_0) {
  rmt_port_info_t *port_info =
      pipe_mgr_get_port_info(dev_info->dev_id, port_id);
  if (!port_info) return PIPE_INVALID_ARG;
  int local_port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  int logical_pipe = dev_info->dev_cfg.dev_port_to_pipe(port_id);
  int ipb_num = tof2_ipb_local_port_to_ipb(local_port);
  if (-1 == ipb_num) return PIPE_INVALID_ARG;
  int ebuf_grp = tof2_local_port_to_ebuf_grp(local_port);
  if (-1 == ebuf_grp) return PIPE_INVALID_ARG;
  int ebuf_idx = tof2_local_port_to_ebuf_grp_idx(local_port);
  if (-1 == ebuf_idx) return PIPE_INVALID_ARG;
  bool is_400 = tof2_ebuf_is400(local_port);
  uint8_t chan_en_mask =
      tof2_parde_get_chan_en_mask(dev_info, logical_pipe, ipb_num);
  if (!en_1_dis_0) {
    int chan_num = tof2_ipb_local_port_to_chan(local_port);
    int chan_cnt = tof2_parde_speed_to_chan_cnt(port_info->speed);
    if (chan_num == -1 || chan_cnt == 0) return PIPE_INVALID_ARG;
    chan_en_mask &= ~(((1u << chan_cnt) - 1) << chan_num);
  }
  /* For the ebuf100 blocks the 8 bit channel enable is actually spread across
   * all four ebuf100s, two bits in each. */
  if (!is_400) {
    chan_en_mask = (chan_en_mask >> (2 * ebuf_grp)) & 3;
  }

  pipe_bitmap_t pipe_bit_map;
  PIPE_BITMAP_INIT(&pipe_bit_map, PIPE_BMP_SIZE);
  PIPE_BITMAP_SET(&pipe_bit_map, logical_pipe);

  uint32_t addrs[2];
  addrs[0] = is_400 ? offsetof(tof2_reg,
                               pipes[0]
                                   .pardereg.pgstnreg.ebuf900reg[ebuf_grp]
                                   .ebuf400reg[ebuf_idx]
                                   .glb_group.port_en_dprsr)
                    : offsetof(tof2_reg,
                               pipes[0]
                                   .pardereg.pgstnreg.ebuf900reg[ebuf_grp]
                                   .ebuf100reg.glb_group.port_en_dprsr);
  addrs[1] = is_400 ? offsetof(tof2_reg,
                               pipes[0]
                                   .pardereg.pgstnreg.ebuf900reg[ebuf_grp]
                                   .ebuf400reg[ebuf_idx]
                                   .glb_group.port_en_warp)
                    : offsetof(tof2_reg,
                               pipes[0]
                                   .pardereg.pgstnreg.ebuf900reg[ebuf_grp]
                                   .ebuf100reg.glb_group.port_en_warp);
  uint32_t stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(addrs[0]);

  uint32_t val = 0;
  setp_tof2_glb_group_port_en_enbl(&val, chan_en_mask);

  for (size_t i = 0; i < sizeof addrs / sizeof addrs[0]; ++i) {
    pipe_instr_write_reg_t instr;
    construct_instr_reg_write(dev_info->dev_id, &instr, addrs[i], val);
    pipe_status_t sts = pipe_mgr_drv_ilist_add(
        &shdl, dev_info, &pipe_bit_map, stage, (uint8_t *)&instr, sizeof instr);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Dev %d port %d Failed to add ebuf chan en instr (%s)",
                dev_info->dev_id,
                port_id,
                pipe_str_err(sts));
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
      return sts;
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_ibuf_tof2_set_version_bits(pipe_sess_hdl_t shdl,
                                                  rmt_dev_info_t *dev_info,
                                                  uint8_t version) {
  return tof2_ipb_write_chan_ctrl_all(shdl, version, dev_info);
}

pipe_status_t pipe_mgr_ibuf_tof2_config_congestion_notif_to_parser(
    pipe_sess_hdl_t shdl,
    rmt_dev_info_t *dev_info,
    rmt_port_info_t *port_info) {
  pipe_status_t sts;

  /* Program the thresholds in the chnl_acc_ctrl register. */
  sts = tof2_ipb_write_chnl_acc_ctrl(shdl, dev_info, port_info);
  if (PIPE_SUCCESS != sts) {
    LOG_ERROR("%s: Dev %d Failed to program acc_ctrl for port %d, sts %s",
              __func__,
              dev_info->dev_id,
              port_info->port_id,
              pipe_str_err(sts));
    return sts;
  }

  /* Program the dis_cong bit in the chnl_ctrl register. */
  sts = tof2_ipb_write_chan_ctrl(
      shdl, port_info->port_id, 0 /* version */, dev_info);
  if (PIPE_SUCCESS != sts) {
    LOG_ERROR("%s: Dev %d Failed to program chan_ctrl for port %d, sts %s",
              __func__,
              dev_info->dev_id,
              port_info->port_id,
              pipe_str_err(sts));
    return sts;
  }

  return sts;
}

static pipe_status_t tof2_parde_parb_chnl_ctrl(pipe_sess_hdl_t shdl,
                                               rmt_dev_info_t *dev_info,
                                               bf_dev_port_t port_id,
                                               bool en_1_dis_0,
                                               bool ing_1_egr_0) {
  const int dprsr_cred_400g = 384;
  const int dprsr_cred_per_chan = dprsr_cred_400g / TOF2_NUM_CHN_PER_IPB;

  rmt_port_info_t *port_info =
      pipe_mgr_get_port_info(dev_info->dev_id, port_id);
  if (!port_info) return PIPE_INVALID_ARG;
  bf_port_speed_t speed = port_info->speed;

  int local_port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  int logical_pipe = dev_info->dev_cfg.dev_port_to_pipe(port_id);
  int ipb_num = tof2_ipb_local_port_to_ipb(local_port);
  if (-1 == ipb_num) return PIPE_INVALID_ARG;
  int chan_num = tof2_ipb_local_port_to_chan(local_port);
  if (-1 == chan_num) return PIPE_INVALID_ARG;
  int channel_count = tof2_parde_speed_to_chan_cnt(speed);

  pipe_bitmap_t pipe_bit_map;
  PIPE_BITMAP_INIT(&pipe_bit_map, PIPE_BMP_SIZE);
  PIPE_BITMAP_SET(&pipe_bit_map, logical_pipe);

  /* Setup the value to write. */
  uint32_t val = 0;
  setp_tof2_parb_chnl_ctrl_en(&val, en_1_dis_0 ? 1 : 0);
  /* Leave at reset value. */
  setp_tof2_parb_chnl_ctrl_dist(&val, 2);
  /* Set credits based on port speed (number of channels used) only when the
   * channel is being enabled. */
  if (en_1_dis_0) {
    setp_tof2_parb_chnl_ctrl_cred(&val, channel_count * dprsr_cred_per_chan);
  }
  uint32_t addr =
      ing_1_egr_0
          ? offsetof(tof2_reg,
                     pipes[0].pardereg.pgstnreg.parbreg.left.i_chnl_ctrl)
          : offsetof(tof2_reg,
                     pipes[0].pardereg.pgstnreg.parbreg.right.e_chnl_ctrl);
  uint32_t stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(addr);
  for (int c = local_port; c < local_port + channel_count; ++c) {
    pipe_instr_write_reg_t instr;
    construct_instr_reg_write(dev_info->dev_id, &instr, addr + 4 * c, val);
    pipe_status_t sts = pipe_mgr_drv_ilist_add(
        &shdl, dev_info, &pipe_bit_map, stage, (uint8_t *)&instr, sizeof instr);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Dev %d port %d Failed to add parb chnl_ctrl instr (%s)",
                dev_info->dev_id,
                port_id,
                pipe_str_err(sts));
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
      return sts;
    }
    /* Program all channels after the first to zero. */
    val = 0;
  }
  return PIPE_SUCCESS;
}

static pipe_status_t tof2_parde_ipb_epb_en_set(pipe_sess_hdl_t shdl,
                                               rmt_dev_info_t *dev_info,
                                               bf_dev_port_t port_id,
                                               bool en_1_dis_0,
                                               bool ing_0_egr_1,
                                               bool use_dma) {
  int local_port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  int logical_pipe = dev_info->dev_cfg.dev_port_to_pipe(port_id);
  int ipb_num = tof2_ipb_local_port_to_ipb(local_port);
  if (-1 == ipb_num) return PIPE_INVALID_ARG;
  int chan_num = tof2_ipb_local_port_to_chan(local_port);
  if (-1 == chan_num) return PIPE_INVALID_ARG;
  uint8_t enable_mask =
      tof2_parde_get_chan_en_mask(dev_info, logical_pipe, ipb_num);
  if (!en_1_dis_0) {
    /* Remove this port from it since we want to disable the channel. */
    enable_mask &= ~(1u << chan_num);
  }

  /* If we do not have any pipeline profiles leave the channels disabled since
   * there is no parser/MAU/deparser configuration programmed. */
  if (dev_info->num_pipeline_profiles == 0) {
    enable_mask = 0;
  }

  pipe_status_t sts = tof2_ipb_epb_write_chan_en(
      shdl, logical_pipe, ipb_num, dev_info, ing_0_egr_1, enable_mask, use_dma);
  if (PIPE_SUCCESS != sts) {
    LOG_ERROR("Dev %d failed to %s channel for port %d, %s",
              dev_info->dev_id,
              en_1_dis_0 ? "enable" : "disable",
              port_id,
              pipe_str_err(sts));
    return sts;
  }
  return PIPE_SUCCESS;
}

static pipe_status_t tof2_parde_s2p_en_set(pipe_sess_hdl_t shdl,
                                           rmt_dev_info_t *dev_info,
                                           bf_dev_port_t port_id,
                                           bool enable) {
  int local_port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  int logical_pipe = dev_info->dev_cfg.dev_port_to_pipe(port_id);
  int ipb_num = tof2_ipb_local_port_to_ipb(local_port);
  if (-1 == ipb_num) return PIPE_INVALID_ARG;
  int chan_num = tof2_ipb_local_port_to_chan(local_port);
  if (-1 == chan_num) return PIPE_INVALID_ARG;
  uint8_t enable_mask =
      tof2_parde_get_chan_en_mask(dev_info, logical_pipe, ipb_num);
  if (!enable) {
    /* Remove this port from it since we want to disable the channel. */
    enable_mask &= ~(1u << chan_num);
  }

  pipe_bitmap_t pipe_bit_map;
  PIPE_BITMAP_INIT(&pipe_bit_map, PIPE_BMP_SIZE);
  PIPE_BITMAP_SET(&pipe_bit_map, logical_pipe);

  uint32_t addr =
      offsetof(tof2_reg, pipes[0].pardereg.pgstnreg.s2preg.chan_en[ipb_num]);
  pipe_instr_write_reg_t instr;
  construct_instr_reg_write(dev_info->dev_id, &instr, addr, enable_mask);
  uint32_t stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(addr);
  pipe_status_t sts = pipe_mgr_drv_ilist_add(
      &shdl, dev_info, &pipe_bit_map, stage, (uint8_t *)&instr, sizeof instr);
  if (PIPE_SUCCESS != sts) {
    LOG_ERROR("Dev %d port %d Failed to add s2p chnl_en instr (%s)",
              dev_info->dev_id,
              port_id,
              pipe_str_err(sts));
    PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
    return sts;
  }
  return PIPE_SUCCESS;
}

static pipe_status_t tof2_parde_port_dis_egr(pipe_sess_hdl_t shdl,
                                             rmt_dev_info_t *dev_info,
                                             bf_dev_port_t port_id) {
  if (!pipe_mgr_is_device_locked(dev_info->dev_id)) {
    /* Wait for packets in EPB to drain.  TM queue rx should be disabled and MAC
     * backpressure should be disabled so we are waiting for currently queued
     * traffic to make it to the MAC where it is dropped. */
    tof2_parde_epb_drain(dev_info, port_id);

    /* Wait for packets in deparser to drain. */
    tof2_parde_dprsr_drain(dev_info, port_id, 0 /* Egress */);

    /* Wait for packets in the ebuf to drain. */
    tof2_parde_ebuf_drain(dev_info, port_id);
  }

  /* Now that the EPB is totally empty (EPB drain, deparser drain, and ebuf
   * drain are complete) the channels can be disabled safely. */

  /* Write pipes.pardereg.pgstnreg.epbprsr4reg[].epbreg.glb_group.port_en */
  pipe_status_t sts =
      tof2_parde_ipb_epb_en_set(shdl, dev_info, port_id, 0, 1, true);
  if (PIPE_SUCCESS != sts) return sts;

  /* Write pipes.pardereg.pgstnreg.parbreg.e_chnl_ctrl */
  sts = tof2_parde_parb_chnl_ctrl(shdl, dev_info, port_id, 0, 0);
  if (PIPE_SUCCESS != sts) return sts;

  /* Write pipes.pardereg.deparser.deparser.inp.icr.mac0_en */
  sts = tof2_parde_i_dprsr_mac_en(shdl, dev_info, port_id, 0, false);
  if (PIPE_SUCCESS != sts) return sts;

  /* Write pipes.pardereg.pgstnreg.ebuf900reg.ebuf400reg.glb_group.port_en_dprsr
   * and pipes.pardereg.pgstnreg.ebuf900reg.ebuf400reg.glb_group.port_en_warp */
  sts = tof2_parde_ebuf_port_en(shdl, dev_info, port_id, 0);
  if (PIPE_SUCCESS != sts) return sts;

  return sts;
}

static pipe_status_t tof2_parde_port_ena_egr(pipe_sess_hdl_t shdl,
                                             rmt_dev_info_t *dev_info,
                                             bf_dev_port_t port_id) {
  pipe_status_t sts = PIPE_SUCCESS;
  /* Write pipes.pardereg.pgstnreg.parbreg.e_chnl_ctrl */
  sts = tof2_parde_parb_chnl_ctrl(shdl, dev_info, port_id, 1, 0);
  if (PIPE_SUCCESS != sts) return sts;

  /* Write pipes.pardereg.pgstnreg.epbprsr4reg[].epbreg.glb_group.port_en */
  sts = tof2_parde_ipb_epb_en_set(shdl, dev_info, port_id, 1, 1, true);
  if (PIPE_SUCCESS != sts) return sts;

  /* Write pipes.pardereg.pgstnreg.ebuf900reg.ebuf400reg.glb_group.port_en_dprsr
   * and pipes.pardereg.pgstnreg.ebuf900reg.ebuf400reg.glb_group.port_en_warp */
  sts = tof2_parde_ebuf_port_en(shdl, dev_info, port_id, 1);
  if (PIPE_SUCCESS != sts) return sts;

  return sts;
}

static pipe_status_t tof2_parde_port_dis_ing(pipe_sess_hdl_t shdl,
                                             rmt_dev_info_t *dev_info,
                                             bf_dev_port_t port_id) {
  if (!pipe_mgr_is_device_locked(dev_info->dev_id)) {
    /* Wait for packets in IPB to drain.  MAC should be disabled. */
    tof2_parde_ipb_drain(dev_info, port_id);

    /* Wait for packets in deparser to drain. */
    tof2_parde_dprsr_drain(dev_info, port_id, true /* Ingress */);
  }

  /* Now that the IPB is totally empty (IPB drain and deparser drain complete)
   * the channels can be disabled safely. */

  /* Write pipes.pardereg.pgstnreg.ipbprsr4reg[].ipbreg.glb_group.port_en */
  pipe_status_t sts =
      tof2_parde_ipb_epb_en_set(shdl, dev_info, port_id, 0, 0, true);
  if (PIPE_SUCCESS != sts) return sts;

  /* Write pipes.pardereg.pgstnreg.parbreg.i_chnl_ctrl */
  sts = tof2_parde_parb_chnl_ctrl(shdl, dev_info, port_id, 0, 1);
  if (PIPE_SUCCESS != sts) return sts;

  sts = tof2_parde_s2p_en_set(shdl, dev_info, port_id, 0);
  if (PIPE_SUCCESS != sts) return sts;

  return sts;
}

static pipe_status_t tof2_edprsr_ur_thres_cfg(pipe_sess_hdl_t shdl,
                                              rmt_dev_info_t *dev_info,
                                              bool is_clk) {
  uint32_t val_clk_400g = 38;
  uint32_t val_clk_200g = 85;
  uint32_t val_clk_100g = 178;
  uint32_t val_clk_50g = 336;
  uint32_t val_clk_40g = 209;
  uint32_t val_clk_25g = 490;
  uint32_t val_clk_10g = 863;

  uint32_t val_cell_400g = 22;
  uint32_t val_cell_200g = 25;
  uint32_t val_cell_100g = 26;
  uint32_t val_cell_50g = 27;
  uint32_t val_cell_40g = 13;
  uint32_t val_cell_25g = 18;
  uint32_t val_cell_10g = 13;

  pipe_bitmap_t pipe_bit_map;
  PIPE_BITMAP_INIT(&pipe_bit_map, PIPE_BMP_SIZE);
  for (unsigned int i = 0; i < dev_info->num_active_pipes; ++i) {
    PIPE_BITMAP_SET(&pipe_bit_map, i);
  }

  uint32_t addr = offsetof(
      tof2_reg,
      pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[0].out_egr.u_thresh_10G);
  uint32_t stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(addr);
  pipe_instr_write_reg_t instr;
  pipe_status_t sts = PIPE_SUCCESS;

  for (int slice = 0; slice < 4; ++slice) {
    addr = offsetof(
        tof2_reg,
        pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[slice].out_egr.u_thresh_10G);
    construct_instr_reg_write(
        dev_info->dev_id, &instr, addr, is_clk ? val_clk_10g : val_cell_10g);
    sts = pipe_mgr_drv_ilist_add(
        &shdl, dev_info, &pipe_bit_map, stage, (uint8_t *)&instr, sizeof instr);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Dev %d slice %d Failed to init edprsr uthresh(%s)",
                dev_info->dev_id,
                slice,
                pipe_str_err(sts));
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
      return sts;
    }

    addr = offsetof(
        tof2_reg,
        pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[slice].out_egr.u_thresh_25G);
    construct_instr_reg_write(
        dev_info->dev_id, &instr, addr, is_clk ? val_clk_25g : val_cell_25g);
    sts = pipe_mgr_drv_ilist_add(
        &shdl, dev_info, &pipe_bit_map, stage, (uint8_t *)&instr, sizeof instr);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Dev %d slice %d Failed to init edprsr uthresh(%s)",
                dev_info->dev_id,
                slice,
                pipe_str_err(sts));
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
      return sts;
    }

    addr = offsetof(
        tof2_reg,
        pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[slice].out_egr.u_thresh_40G);
    construct_instr_reg_write(
        dev_info->dev_id, &instr, addr, is_clk ? val_clk_40g : val_cell_40g);
    sts = pipe_mgr_drv_ilist_add(
        &shdl, dev_info, &pipe_bit_map, stage, (uint8_t *)&instr, sizeof instr);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Dev %d slice %d Failed to init edprsr uthresh(%s)",
                dev_info->dev_id,
                slice,
                pipe_str_err(sts));
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
      return sts;
    }

    addr = offsetof(
        tof2_reg,
        pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[slice].out_egr.u_thresh_50G);
    construct_instr_reg_write(
        dev_info->dev_id, &instr, addr, is_clk ? val_clk_50g : val_cell_50g);
    sts = pipe_mgr_drv_ilist_add(
        &shdl, dev_info, &pipe_bit_map, stage, (uint8_t *)&instr, sizeof instr);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Dev %d slice %d Failed to init edprsr uthresh(%s)",
                dev_info->dev_id,
                slice,
                pipe_str_err(sts));
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
      return sts;
    }

    addr = offsetof(
        tof2_reg,
        pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[slice].out_egr.u_thresh_100G);
    construct_instr_reg_write(
        dev_info->dev_id, &instr, addr, is_clk ? val_clk_100g : val_cell_100g);
    sts = pipe_mgr_drv_ilist_add(
        &shdl, dev_info, &pipe_bit_map, stage, (uint8_t *)&instr, sizeof instr);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Dev %d slice %d Failed to init edprsr uthresh(%s)",
                dev_info->dev_id,
                slice,
                pipe_str_err(sts));
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
      return sts;
    }

    addr = offsetof(
        tof2_reg,
        pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[slice].out_egr.u_thresh_200G);
    construct_instr_reg_write(
        dev_info->dev_id, &instr, addr, is_clk ? val_clk_200g : val_cell_200g);
    sts = pipe_mgr_drv_ilist_add(
        &shdl, dev_info, &pipe_bit_map, stage, (uint8_t *)&instr, sizeof instr);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Dev %d slice %d Failed to init edprsr uthresh(%s)",
                dev_info->dev_id,
                slice,
                pipe_str_err(sts));
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
      return sts;
    }

    addr = offsetof(
        tof2_reg,
        pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[slice].out_egr.u_thresh_400G);
    construct_instr_reg_write(
        dev_info->dev_id, &instr, addr, is_clk ? val_clk_400g : val_cell_400g);
    sts = pipe_mgr_drv_ilist_add(
        &shdl, dev_info, &pipe_bit_map, stage, (uint8_t *)&instr, sizeof instr);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Dev %d slice %d Failed to init edprsr uthresh(%s)",
                dev_info->dev_id,
                slice,
                pipe_str_err(sts));
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
      return sts;
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_parde_tof2_device_add(pipe_sess_hdl_t shdl,
                                             rmt_dev_info_t *dev_info) {
  /* Take pipes.pardereg.pgstnreg.epbprsr4reg[].epbreg.chanX_group.chnl_ctrl
   * from config cache. */
  bf_dev_id_t dev_id = dev_info->dev_id;
  struct pipe_mgr_tof2_ebuf_ctx *ctx = &pipe_mgr_ebuf_ctx(dev_id)->tof2;
  if (!ctx) return PIPE_INVALID_ARG;

  if (!pipe_mgr_init_mode_fast_recfg_quick(dev_id)) {
    PIPE_MGR_DBGCHK(!ctx->reg);
    ctx->reg = PIPE_MGR_CALLOC(dev_info->num_active_pipes, sizeof *ctx->reg);
  }
  if (!ctx->reg) return PIPE_NO_SYS_RESOURCES;
  for (uint32_t prof_id = 0; prof_id < dev_info->num_pipeline_profiles;
       ++prof_id) {
    struct pipe_config_cache_reg_t *meta_opt;
    bf_map_sts_t m = bf_map_get(&dev_info->profile_info[prof_id]->config_cache,
                                pipe_cck_meta_opt_ctrl,
                                (void **)&meta_opt);
    if (BF_MAP_OK != m) {
      LOG_ERROR("Dev %d Failed to get parser_chnl_ctrl from profile %d",
                dev_info->dev_id,
                prof_id);
      return PIPE_INIT_ERROR;
    }
    unsigned int pipe_id;
    PIPE_BITMAP_ITER(&dev_info->profile_info[prof_id]->pipe_bmp, pipe_id) {
      for (int epb_num = 0; epb_num < TOF2_NUM_EPB; ++epb_num) {
        ctx->reg[pipe_id].meta_opt[epb_num] = meta_opt[epb_num].val;
      }
    }
  }

  if (pipe_mgr_hitless_warm_init_in_progress(dev_info->dev_id)) {
    return PIPE_SUCCESS;
  }

  /* Set wide bubble duration based on clock_cycles from MAU stage
   * characteristics. */
  for (uint32_t prof_id = 0; prof_id < dev_info->num_pipeline_profiles;
       ++prof_id) {
    rmt_dev_profile_info_t *profile = dev_info->profile_info[prof_id];
    pipe_mgr_stage_char_t *entry;
    unsigned long key;
    int pddi = 0;
    int pdde = 0;
    bf_map_sts_t m = bf_map_get_first(
        &profile->stage_characteristics, &key, (void **)&entry);
    while (BF_MAP_OK == m) {
      bool dir = key & 0x1;
      if (dir && entry->clock_cycles > pdde) pdde = entry->clock_cycles;
      if (!dir && entry->clock_cycles > pddi) pddi = entry->clock_cycles;
      m = bf_map_get_next(
          &profile->stage_characteristics, &key, (void **)&entry);
    }
    if (m != BF_MAP_NO_KEY) {
      LOG_ERROR(
          "Dev %d Failed to get MAU stage characteristics from profile %d",
          dev_info->dev_id,
          prof_id);
      return PIPE_INIT_ERROR;
    }
    /* 64 is the maximum number of CSRs updated atomically. */
    pddi += (7 + 64);
    pdde += (7 + 64);
    uint32_t data[2] = {0, 0};
    uint32_t base_addr[2];
    base_addr[0] =
        offsetof(tof2_reg, pipes[0].pardereg.pgstnreg.parbreg.left.i_wb_ctrl);
    base_addr[1] =
        offsetof(tof2_reg, pipes[0].pardereg.pgstnreg.parbreg.right.e_wb_ctrl);
    pipe_instr_write_reg_t instr[2];
    setp_tof2_parb_wb_ctrl_duration(&data[0], pddi);
    setp_tof2_parb_wb_ctrl_duration(&data[1], pdde);
    pipe_bitmap_t *pipe_bit_map = &dev_info->profile_info[prof_id]->pipe_bmp;
    for (int j = 0; j < 2; ++j) {
      construct_instr_reg_write(dev_id, &instr[j], base_addr[j], data[j]);
      uint32_t stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(base_addr[j]);
      pipe_status_t sts = pipe_mgr_drv_ilist_add(&shdl,
                                                 dev_info,
                                                 pipe_bit_map,
                                                 stage,
                                                 (uint8_t *)&instr[j],
                                                 sizeof instr[j]);
      if (PIPE_SUCCESS != sts) {
        LOG_ERROR("Dev %d j %d Failed to init wb_ctrl (%s)",
                  dev_info->dev_id,
                  j,
                  pipe_str_err(sts));
        PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
        return sts;
      }
    }
  }

  pipe_bitmap_t pipe_bit_map;
  PIPE_BITMAP_INIT(&pipe_bit_map, PIPE_BMP_SIZE);
  for (unsigned int i = 0; i < dev_info->num_active_pipes; ++i) {
    PIPE_BITMAP_SET(&pipe_bit_map, i);
  }
  /* Set free_list_init in IPB's glb_ctrl. */
  for (int i = 0; i < TOF2_NUM_IPB; ++i) {
    uint32_t base_addr = offsetof(tof2_reg,
                                  pipes[0]
                                      .pardereg.pgstnreg.ipbprsr4reg[i]
                                      .ipbreg.glb_group.glb_ctrl.glb_ctrl_0_2);
    uint32_t data[2] = {0, 0};
    uint32_t stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(base_addr);
    setp_tof2_glb_ctrl_en_resubmit(data, 1);
    setp_tof2_glb_ctrl_free_list_init(data, 1);
    setp_tof2_glb_ctrl_prsr_crd(data, 0x88888888);
    pipe_instr_write_reg_t instr[2];
    construct_instr_reg_write(dev_id, &instr[0], base_addr, data[0]);
    construct_instr_reg_write(dev_id, &instr[1], base_addr + 4, data[1]);
    for (int j = 0; j < 2; ++j) {
      pipe_status_t sts = pipe_mgr_drv_ilist_add(&shdl,
                                                 dev_info,
                                                 &pipe_bit_map,
                                                 stage,
                                                 (uint8_t *)&instr[j],
                                                 sizeof instr[j]);
      if (PIPE_SUCCESS != sts) {
        LOG_ERROR("Dev %d ipb %d Failed to init glb_ctrl (%s)",
                  dev_info->dev_id,
                  i,
                  pipe_str_err(sts));
        PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
        return sts;
      }
    }
  }

  /* Set glb_parser_maxbyte to maximum in all IPBs. */
  for (int i = 0; i < TOF2_NUM_IPB; ++i) {
    uint32_t base_addr = offsetof(tof2_reg,
                                  pipes[0]
                                      .pardereg.pgstnreg.ipbprsr4reg[i]
                                      .ipbreg.glb_group.glb_parser_maxbyte);
    uint32_t data = 0;
    uint32_t stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(base_addr);
    setp_tof2_glb_parser_maxbyte_prsr_dph_max(&data, 0x3FF);
    pipe_instr_write_reg_t instr;
    construct_instr_reg_write(dev_id, &instr, base_addr, data);
    pipe_status_t sts = pipe_mgr_drv_ilist_add(
        &shdl, dev_info, &pipe_bit_map, stage, (uint8_t *)&instr, sizeof instr);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Dev %d ipb %d Failed to init parser_max_byte (%s)",
                dev_info->dev_id,
                i,
                pipe_str_err(sts));
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
      return sts;
    }
  }

  for (int i = 0; i < TOF2_NUM_IPB; ++i) {
    uint32_t base_addr = offsetof(
        tof2_reg,
        pipes[0].pardereg.pgstnreg.epbprsr4reg[i].epbreg.glb_group.glb_ctrl);
    uint32_t val = 0;
    uint32_t stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(base_addr);
    /* Remember to update EPB chnl_fifo_ctrl's ebuf_counter when switching
     * between cell_inc and clk_inc. */
    setp_tof2_epb_ctrl_ebuf_cnt_cell_inc(&val, 1);
    setp_tof2_epb_ctrl_ebuf_cnt_clk_inc(&val, 0);
    setp_tof2_epb_ctrl_p2s_chnl_hi_lo(&val, (i + 1) & 1);
    pipe_instr_write_reg_t instr;
    construct_instr_reg_write(dev_id, &instr, base_addr, val);
    pipe_status_t sts = pipe_mgr_drv_ilist_add(
        &shdl, dev_info, &pipe_bit_map, stage, (uint8_t *)&instr, sizeof instr);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Dev %d epb %d Failed to init glb_ctrl (%s)",
                dev_info->dev_id,
                i,
                pipe_str_err(sts));
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
      return sts;
    }
  }

  /* Write all the hold_es and hold_ms registers in s2p to ensure that we
   * send three cells at a time to TM. */
  uint32_t addr_data[12][2] = {
      {offsetof(tof2_reg, pipes[0].pardereg.pgstnreg.s2preg.hold_es_400g), 8},
      {offsetof(tof2_reg, pipes[0].pardereg.pgstnreg.s2preg.hold_es_200g), 17},
      {offsetof(tof2_reg, pipes[0].pardereg.pgstnreg.s2preg.hold_es_100g), 35},
      {offsetof(tof2_reg, pipes[0].pardereg.pgstnreg.s2preg.hold_es_50g), 71},
      {offsetof(tof2_reg, pipes[0].pardereg.pgstnreg.s2preg.hold_es_25g), 71},
      {offsetof(tof2_reg, pipes[0].pardereg.pgstnreg.s2preg.hold_es_10g), 0},
      {offsetof(tof2_reg, pipes[0].pardereg.pgstnreg.s2preg.hold_sm_400g), 8},
      {offsetof(tof2_reg, pipes[0].pardereg.pgstnreg.s2preg.hold_sm_200g), 17},
      {offsetof(tof2_reg, pipes[0].pardereg.pgstnreg.s2preg.hold_sm_100g), 35},
      {offsetof(tof2_reg, pipes[0].pardereg.pgstnreg.s2preg.hold_sm_50g), 71},
      {offsetof(tof2_reg, pipes[0].pardereg.pgstnreg.s2preg.hold_sm_25g), 71},
      {offsetof(tof2_reg, pipes[0].pardereg.pgstnreg.s2preg.hold_sm_10g), 0},
  };

  uint32_t s2p_stage =
      dev_info->dev_cfg.pcie_pipe_addr_get_stage(addr_data[0][0]);
  uint32_t p2s_stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(
      offsetof(tof2_reg, pipes[0].pardereg.pgstnreg.p2sreg.port_rate_cfg[0]));
  for (size_t i = 0; i < sizeof(addr_data) / sizeof(addr_data[0]); ++i) {
    pipe_instr_write_reg_t instr;
    construct_instr_reg_write(dev_id, &instr, addr_data[i][0], addr_data[i][1]);
    pipe_status_t sts = pipe_mgr_drv_ilist_add(&shdl,
                                               dev_info,
                                               &pipe_bit_map,
                                               s2p_stage,
                                               (uint8_t *)&instr,
                                               sizeof instr);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Dev %d Failed to init hold_es/sm(%s)",
                dev_info->dev_id,
                pipe_str_err(sts));
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
      return sts;
    }
  }

  /* Setup the pipe_map for non-A0 Tofino2s. */
  bf_sku_chip_part_rev_t rev = BF_SKU_CHIP_PART_REV_B0;
  lld_sku_get_chip_part_revision_number(dev_id, &rev);
  if (rev != BF_SKU_CHIP_PART_REV_A0) {
    pipe_status_t sts;
    uint32_t l2p_map = 0, p2l_map = 0;
    uint32_t num_pipes = dev_info->num_active_pipes;
    bool rotated = lld_efuse_get_die_rotated(dev_id);
    if (num_pipes == 4 || num_pipes == 2 || num_pipes == 1) {
      /* Logical  Physical (non-rot)  Physical (rot)
       *  0        0                   2
       *  1        1                   3
       *  2        2                   0
       *  3        3                   1
       */
      if (rotated) {
        l2p_map = (1 << 6) | (0 << 4) | (3 << 2) | (2 << 0);
        p2l_map = l2p_map;
      } else {
        l2p_map = (3 << 6) | (2 << 4) | (1 << 2) | (0 << 0);
        p2l_map = l2p_map;
      }
    } else if (num_pipes == 3) {
      /* Logical  Physical (non-rot)  Physical (rot)
       *  0        1                   3
       *  1        0                   2
       *  2        3                   1
       *  X        2                   X
       */
      if (rotated) {
        l2p_map = (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0);
        p2l_map = l2p_map;
      } else {
        l2p_map = (2 << 6) | (3 << 4) | (0 << 2) | (1 << 0);
        p2l_map = l2p_map;
      }
    } else {
      LOG_ERROR("Unexpected number of active pipes, %u", num_pipes);
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
    }
    pipe_instr_write_reg_t instr;
    uint32_t addr =
        offsetof(tof2_reg, pipes[0].pardereg.pgstnreg.s2preg.pipe_map);
    construct_instr_reg_write(dev_id, &instr, addr, l2p_map);
    sts = pipe_mgr_drv_ilist_add(&shdl,
                                 dev_info,
                                 &pipe_bit_map,
                                 s2p_stage,
                                 (uint8_t *)&instr,
                                 sizeof instr);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Dev %d Failed to init S2P pipe_map (%s)",
                dev_info->dev_id,
                pipe_str_err(sts));
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
      return sts;
    }

    addr = offsetof(tof2_reg, pipes[0].pardereg.pgstnreg.p2sreg.pipe_map);
    construct_instr_reg_write(dev_id, &instr, addr, p2l_map);
    sts = pipe_mgr_drv_ilist_add(&shdl,
                                 dev_info,
                                 &pipe_bit_map,
                                 p2s_stage,
                                 (uint8_t *)&instr,
                                 sizeof instr);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Dev %d Failed to init P2S pipe_map (%s)",
                dev_info->dev_id,
                pipe_str_err(sts));
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
      return sts;
    }
  }

  /* Write the S2P port rate to unused.  It defaults to 400g but needs to see a
   * transition from unused to used in order to properly load credits.  By
   * setting it to unused now we ensure the first port add will cause an unused
   * to used transition. */
  for (int i = 0; i < TOF2_NUM_IPB; ++i) {
    uint32_t addr =
        offsetof(tof2_reg, pipes[0].pardereg.pgstnreg.p2sreg.port_rate_cfg[i]);
    uint32_t val = 0;
    for (int j = 0; j < 8; ++j)
      setp_tof2_s2p_reg_port_rate_cfg_chnl_rate(
          &val, j, tof2_parde_speed_to_port_rate(BF_SPEED_NONE));

    pipe_instr_write_reg_t instr;
    construct_instr_reg_write(dev_id, &instr, addr, val);
    pipe_status_t sts = pipe_mgr_drv_ilist_add(&shdl,
                                               dev_info,
                                               &pipe_bit_map,
                                               p2s_stage,
                                               (uint8_t *)&instr,
                                               sizeof instr);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Dev %d epb %d Failed to init P2S port_rate (%s)",
                dev_info->dev_id,
                i,
                pipe_str_err(sts));
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
      return sts;
    }
  }

  /* Set up IPB and EPB timestamp offsets.  These are written per pipe since
   * the values are unique across pipes.
   * First dimension, 0: IPB 1:EPB
   * Second dimension, phy-pipe
   * Third dimension, IPB/EPB number */
  int xpb_ts_off[2][4][9] = {{{81, 75, 76, 79, 77, 73, 72, 70, 69},
                              {81, 74, 76, 79, 77, 73, 71, 70, 69},
                              {81, 75, 76, 79, 77, 73, 72, 70, 69},
                              {81, 75, 76, 79, 77, 73, 72, 70, 69}},
                             {{69, 71, 72, 75, 76, 68, 66, 77, 79},
                              {69, 71, 72, 74, 76, 68, 66, 77, 79},
                              {69, 71, 72, 75, 76, 68, 66, 77, 79},
                              {69, 71, 72, 75, 76, 68, 66, 77, 79}}};
  for (unsigned int i = 0; i < dev_info->num_active_pipes; ++i) {
    pipe_bitmap_t pbm;
    PIPE_BITMAP_INIT(&pbm, PIPE_BMP_SIZE);
    PIPE_BITMAP_SET(&pbm, i);
    bf_dev_pipe_t phy_pipe;
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, i, &phy_pipe);
    for (int j = 0; j < 2; ++j) {
      for (int k = 0; k < TOF2_NUM_IPB; ++k) {
        uint32_t addr = j ? offsetof(tof2_reg,
                                     pipes[0]
                                         .pardereg.pgstnreg.epbprsr4reg[k]
                                         .epbreg.glb_group.glb_epb_tim_off)
                          : offsetof(tof2_reg,
                                     pipes[0]
                                         .pardereg.pgstnreg.ipbprsr4reg[k]
                                         .ipbreg.glb_group.glb_ipb_tim_off);
        uint32_t val = xpb_ts_off[j][phy_pipe][k] << 12;
        uint32_t stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(addr);
        pipe_instr_write_reg_t instr;
        construct_instr_reg_write(dev_id, &instr, addr, val);
        pipe_status_t sts = pipe_mgr_drv_ilist_add(
            &shdl, dev_info, &pbm, stage, (uint8_t *)&instr, sizeof instr);
        if (PIPE_SUCCESS != sts) {
          LOG_ERROR("Dev %d Failed to init xPB TS offset %s)",
                    dev_info->dev_id,
                    pipe_str_err(sts));
          PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
          return sts;
        }
      }
    }
  }

  /* Setup the four deparser inp counters to capture something meaningful. */
  for (int i = 0; i < 4; ++i) {
    uint32_t addr[] = {
        offsetof(tof2_reg,
                 pipes[0].pardereg.dprsrreg.dprsrreg.inp.icr.cfg48_data[i]),
        offsetof(tof2_reg,
                 pipes[0].pardereg.dprsrreg.dprsrreg.inp.icr.cfg48_mask[i])};
    uint32_t data[] = {0, 0};

    /* Counter zero is ingress drops. */
    if (i == 0) {
      data[0] = data[1] = 7;
    }
    /* Counter one is resubmit. */
    if (i == 1) {
      data[0] = 5;
      data[1] = 7;
    }
    /* Counter two is ingress fwd (non-resubmit). */
    if (i == 2) {
      data[0] = 3;
      data[1] = 7;
    }
    /* Counter three is all egress drops */
    if (i == 3) {
      data[0] = data[1] = 0xA0000;
    }

    for (int j = 0; j < 2; ++j) {
      uint32_t stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(addr[j]);
      pipe_instr_write_reg_t instr;
      construct_instr_reg_write(dev_id, &instr, addr[j], data[j]);
      pipe_status_t sts = pipe_mgr_drv_ilist_add(&shdl,
                                                 dev_info,
                                                 &pipe_bit_map,
                                                 stage,
                                                 (uint8_t *)&instr,
                                                 sizeof instr);
      if (PIPE_SUCCESS != sts) {
        LOG_ERROR("Dev %d Failed to init dprsr cntr[%d][%d] (%s)",
                  dev_info->dev_id,
                  i,
                  j,
                  pipe_str_err(sts));
        PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
        return sts;
      }
    }
  }
  /* Setup the deparser header phase counters for forward and drop cases. */
  for (int slice = 0; slice < 4; ++slice) {
    for (int i = 0; i < 2; ++i) {
      uint32_t addr[2][2] = {
          {offsetof(tof2_reg,
                    pipes[0]
                        .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                        .hir.h.cfg48_data[i]),
           offsetof(tof2_reg,
                    pipes[0]
                        .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                        .hir.h.cfg48_mask[i])},
          {offsetof(tof2_reg,
                    pipes[0]
                        .pardereg.dprsrreg.dprsrreg.ho_e[slice]
                        .her.h.cfg48_data[i]),
           offsetof(tof2_reg,
                    pipes[0]
                        .pardereg.dprsrreg.dprsrreg.ho_e[slice]
                        .her.h.cfg48_mask[i])}};
      uint32_t data[] = {0, 0};
      if (i == 0) {
        data[0] = 0x41;
        data[1] = 0xC1;
      }
      if (i == 1) {
        data[0] = 0x81;
        data[1] = 0x81;
      }
      for (int gress = 0; gress < 2; ++gress) {
        for (int j = 0; j < 2; ++j) {
          uint32_t stage =
              dev_info->dev_cfg.pcie_pipe_addr_get_stage(addr[gress][j]);
          pipe_instr_write_reg_t instr;
          construct_instr_reg_write(dev_id, &instr, addr[gress][j], data[j]);
          pipe_status_t sts = pipe_mgr_drv_ilist_add(&shdl,
                                                     dev_info,
                                                     &pipe_bit_map,
                                                     stage,
                                                     (uint8_t *)&instr,
                                                     sizeof instr);
          if (PIPE_SUCCESS != sts) {
            LOG_ERROR("Dev %d Failed to init dprsr hdr cntr[%d][%d] (%s)",
                      dev_info->dev_id,
                      gress,
                      slice,
                      pipe_str_err(sts));
            PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
            return sts;
          }
        }
      }
    }

    for (int i = 0; i < 4; ++i) {
      uint32_t addr[2][3] = {
          {offsetof(tof2_reg,
                    pipes[0]
                        .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                        .out_ingr.cfg48_data[i]),
           offsetof(tof2_reg,
                    pipes[0]
                        .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                        .out_ingr.cfg48_mask[i]),
           offsetof(tof2_reg,
                    pipes[0]
                        .pardereg.dprsrreg.dprsrreg.ho_i[slice]
                        .out_ingr.cfg48_data_sel)},

          {offsetof(tof2_reg,
                    pipes[0]
                        .pardereg.dprsrreg.dprsrreg.ho_e[slice]
                        .out_egr.cfg48_data[i]),
           offsetof(tof2_reg,
                    pipes[0]
                        .pardereg.dprsrreg.dprsrreg.ho_e[slice]
                        .out_egr.cfg48_mask[i]),
           offsetof(tof2_reg,
                    pipes[0]
                        .pardereg.dprsrreg.dprsrreg.ho_e[slice]
                        .out_egr.cfg48_data_sel)}};
      uint32_t data[] = {0, 0, 0};
      if (i == 0) {
        /* Count pkts forwarded to mirror block. */
        data[0] = data[1] = 0x80001;
        data[2] = 0xe;
      }
      if (i == 1) {
        /* Count EOPs on slice mac group 0. */
        data[0] = data[1] = 9;
        data[2] = 0xe;
      }
      if (i == 2) {
        /* Count EOPs on slice mac group 1. */
        data[0] = data[1] = 0x420;
        data[2] = 0xe;
      }
      if (i == 3) {
        /* Count EOPs on slice mac group 2. */
        data[0] = data[1] = 0x21000;
        data[2] = 0xe;
      }

      for (int gress = 0; gress < 2; ++gress) {
        for (int j = 0; j < 3; ++j) {
          uint32_t stage =
              dev_info->dev_cfg.pcie_pipe_addr_get_stage(addr[gress][j]);
          pipe_instr_write_reg_t instr;
          construct_instr_reg_write(dev_id, &instr, addr[gress][j], data[j]);
          pipe_status_t sts = pipe_mgr_drv_ilist_add(&shdl,
                                                     dev_info,
                                                     &pipe_bit_map,
                                                     stage,
                                                     (uint8_t *)&instr,
                                                     sizeof instr);
          if (PIPE_SUCCESS != sts) {
            LOG_ERROR("Dev %d Failed to init dprsr hdr2 cntr[%d][%d] (%s)",
                      dev_info->dev_id,
                      gress,
                      slice,
                      pipe_str_err(sts));
            PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
            return sts;
          }
        }
      }
    }
  }

  /* Clear the false fire of the EPB credit underflow interrupt in P2S so we can
   * catch real occurances of it in the future. */
  uint32_t addr =
      offsetof(tof2_reg, pipes[0].pardereg.pgstnreg.p2sreg.intr.stat);
  uint32_t stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(addr);
  uint32_t data = 0;
  setp_tof2_p2s_reg_intr_stat_epb_cred_uflow(&data, 1);
  pipe_instr_write_reg_t instr;
  construct_instr_reg_write(dev_id, &instr, addr, data);
  pipe_status_t sts = pipe_mgr_drv_ilist_add(
      &shdl, dev_info, &pipe_bit_map, stage, (uint8_t *)&instr, sizeof instr);
  if (PIPE_SUCCESS != sts) {
    LOG_ERROR("Dev %d Failed to clear P2S intr (%s)",
              dev_info->dev_id,
              pipe_str_err(sts));
    PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
    return sts;
  }

  /* Initialize deparser channels to setup wptr_prev and max_ptr correctly. */
  for (int i = 0; i < TOF2_NUM_IPB; ++i) {
    /* For each port group in the deparser, set all channels to 50g and toggle
     * the channel enables, enable-disable-enable-disable.  The first
     * enable will load all pointers except wptr_prev and the second enable
     * will load wptr_prev. */
    uint32_t base_addr = offsetof(
        tof2_reg, pipes[0].pardereg.dprsrreg.dprsrreg.inp.icr.mac0_rates);
    uint32_t mac_step =
        offsetof(tof2_reg,
                 pipes[0].pardereg.dprsrreg.dprsrreg.inp.icr.mac1_rates) -
        base_addr;
    addr = base_addr + i * mac_step;
    data = 0;
    for (int ch = 0; ch < TOF2_NUM_CHN_PER_IPB; ++ch) {
      /* Can use the mac0 setp function for all eight inp.icr.macX_rates
       * registers since they all have the same format. */
      setp_tof2_dprsr_ic_regs_mac0_rates_chnl_rate(
          &data, ch, tof2_parde_speed_to_port_rate(BF_SPEED_50G));
    }
    construct_instr_reg_write(dev_info->dev_id, &instr, addr, data);
    stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(addr);
    sts = pipe_mgr_drv_ilist_add(
        &shdl, dev_info, &pipe_bit_map, stage, (uint8_t *)&instr, sizeof instr);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Dev %d: Failed to post dprsr mac%d_rates instr (%s)",
                dev_id,
                i,
                pipe_str_err(sts));
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
      return sts;
    }

    /* inp.icr.macX_en is always the next address after the macX_rates
     * register so just increment the address. */
    addr += 4;
    data = 0xFF;
    for (int j = 0; j <= 4; ++j) {
      /* Since we want to write the macX_en to 0->0xFF->0->0xFF->0 we start
       * with data as 0xFF, then bitwise invert it on each loop iteration. */
      data = (~data) & 0xFF;
      construct_instr_reg_write(dev_info->dev_id, &instr, addr, data);
      stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(addr);
      sts = pipe_mgr_drv_ilist_add(&shdl,
                                   dev_info,
                                   &pipe_bit_map,
                                   stage,
                                   (uint8_t *)&instr,
                                   sizeof instr);
      if (PIPE_SUCCESS != sts) {
        LOG_ERROR("Dev %d: Failed to post dprsr mac%d_en instr %d (%s)",
                  dev_id,
                  i,
                  j,
                  pipe_str_err(sts));
        PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
        return sts;
      }
    }
  }

  /* Turn off the cycle timer in the egress parsers since the parser may hold a
   * packet for an extended period of time if a pause frame stops the MAC but
   * the TM to EPB interface still has enough credit to send a cell or two of a
   * new packet.  In this case EPB would get the first part of a packet and give
   * it to the parser but then stall until more credit is available. */
  for (int i = 0; i < TOF2_NUM_EPB; ++i) {
    for (int j = 0; j < 4; ++j) {
      uint32_t max_cycle =
          offsetof(tof2_reg,
                   pipes[0].pardereg.pgstnreg.epbprsr4reg[i].prsr[j].max_cycle);
      construct_instr_reg_write(dev_info->dev_id, &instr, max_cycle, 0);
      stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(max_cycle);
      sts = pipe_mgr_drv_ilist_add(&shdl,
                                   dev_info,
                                   &pipe_bit_map,
                                   stage,
                                   (uint8_t *)&instr,
                                   sizeof instr);
      if (sts != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s : Instruction list add failed for parser max_cycle "
            "configuration, EPB[%d].prsr[%d], error %s",
            __func__,
            i,
            j,
            pipe_str_err(sts));
        PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
        return sts;
      }
    }
  }

  /* Adjust underrun timer values. */
  return tof2_edprsr_ur_thres_cfg(shdl, dev_info, true);
}

void pipe_mgr_parde_tof2_device_rmv(rmt_dev_info_t *dev_info) {
  bf_dev_id_t dev_id = dev_info->dev_id;
  struct pipe_mgr_tof2_ebuf_ctx *ctx = &pipe_mgr_ebuf_ctx(dev_id)->tof2;
  if (ctx) {
    PIPE_MGR_DBGCHK(ctx->reg);
    PIPE_MGR_FREE(ctx->reg);
    ctx->reg = NULL;
  }
}

pipe_status_t pipe_mgr_parde_tof2_port_add_ing(pipe_sess_hdl_t shdl,
                                               rmt_dev_info_t *dev_info,
                                               bf_dev_port_t port_id) {
  pipe_status_t sts = PIPE_SUCCESS;
  int local_port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  int ipb_num = tof2_ipb_local_port_to_ipb(local_port);
  if (-1 == ipb_num) return PIPE_INVALID_ARG;
  int chan_num = tof2_ipb_local_port_to_chan(local_port);
  if (-1 == chan_num) return PIPE_INVALID_ARG;

  /*
   * Step 1 - Disable Channels
   *  - pipes.pardereg.pgstnreg.ipbprsr4reg[].ipbreg.glb_group.port_en
   *  - pipes.pardereg.pgstnreg.parbreg.i_chnl_ctrl
   *  - pipes.dprsrreg.dprsrreg.inp.icr.macX_en
   *  - pipes.pardereg.pgstnreg.s2preg.chan_en
   */
  sts = tof2_parde_port_dis_ing(shdl, dev_info, port_id);
  if (PIPE_SUCCESS != sts) return sts;

  /*
   * Step 2 - Set channel rate
   */
  sts = tof2_parde_cfg_i_chnl_rate(shdl, dev_info, port_id, false);
  if (PIPE_SUCCESS != sts) return sts;

  /*
   * Step 3 - Configure dprsr
   *  - pipes.dprsrreg.dprsrreg.inp.icr.macX_rates
   */
  sts = tof2_parde_cfg_dprsr_chnl_rate(shdl, dev_info, port_id, false);
  if (PIPE_SUCCESS != sts) return sts;

  /*
   * Step 4 - Setup IPB channel
   *  - pipes.pardereg.pgstnreg.ipbprsr4reg[].ipbreg.chanX_group.chnl_acc_ctrl
   *  - pipes.pardereg.pgstnreg.ipbprsr4reg[].ipbreg.chanX_group.chnl_fifo_ctrl
   *  - pipes.pardereg.pgstnreg.ipbprsr4reg[].ipbreg.chanX_group.meta_fifo_ctrl
   *  - pipes.pardereg.pgstnreg.ipbprsr4reg[].ipbreg.chanX_group.chnl_ctrl
   */
  sts = tof2_parde_cfg_ipb(shdl, dev_info, port_id);
  if (PIPE_SUCCESS != sts) return sts;

  /*
   * Step 5 - Enable Channels except for IPB.
   */
  sts = tof2_parde_s2p_en_set(shdl, dev_info, port_id, 1);
  if (PIPE_SUCCESS != sts) return sts;
  sts = tof2_parde_i_dprsr_mac_en(shdl, dev_info, port_id, 1, false);
  if (PIPE_SUCCESS != sts) return sts;
  sts = tof2_parde_parb_chnl_ctrl(shdl, dev_info, port_id, 1, 1);
  if (PIPE_SUCCESS != sts) return sts;

  /*
   * Step 6 - Enable IPB channel.
   */
  sts = tof2_parde_ipb_epb_en_set(shdl, dev_info, port_id, 1, 0, true);
  if (sts != PIPE_SUCCESS) return sts;
  return sts;
}

pipe_status_t pipe_mgr_parde_tof2_port_add_egr(pipe_sess_hdl_t shdl,
                                               rmt_dev_info_t *dev_info,
                                               bf_dev_port_t port_id) {
  pipe_status_t sts = PIPE_SUCCESS;
  /* Step 1 - Disable channels
   *  - pipes.pardereg.pgstnreg.epbprsr4reg[].epbreg.glb_group.port_en
   *  - pipes.pardereg.pgstnreg.parbreg.e_chnl_ctrl
   *
   * Step 2 - Setup P2S
   *  - pipes.pardereg.pgstnreg.p2sreg.epb_cred_wr
   *  - pipes.pardereg.pgstnreg.p2sreg.tm_cred
   *
   * Step 3 - Set channel rate everywhere
   *  - pipes.pardereg.pgstnreg.epbprsr4reg[].prsr[].port_rate_cfg
   *  - pipes.pardereg.pgstnreg.epbprsr4reg[].epbreg.glb_group.port_rates
   *  - pipes.pardereg.pgstnreg.p2sreg.port_rate_cfg
   *
   * Step 4 - Setup EPB Channel
   *  - pipes.pardereg.pgstnreg.epbprsr4reg[].epbreg.chanX_group.chnl_ctrl
   *  - pipes.pardereg.pgstnreg.epbprsr4reg[].epbreg.chanX_group.chnl_fifo_ctrl
   *
   * Step 5 - Enable Channels
   *  - pipes.pardereg.pgstnreg.parbreg.e_chnl_ctrl
   *  - pipes.pardereg.pgstnreg.epbprsr4reg[].epbreg.glb_group.port_en
   */

  /*
   * Step 1 - Disable Channels.
   */
  sts = tof2_parde_port_dis_egr(shdl, dev_info, port_id);
  if (PIPE_SUCCESS != sts) return sts;

  /*
   * Step 2 - Setup P2S Credits
   */
  int local_port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  int logical_pipe = dev_info->dev_cfg.dev_port_to_pipe(port_id);
  int epb_num = tof2_ipb_local_port_to_ipb(local_port);
  if (-1 == epb_num) return PIPE_INVALID_ARG;
  sts = tof2_parde_cfg_p2s_cred(shdl, dev_info, logical_pipe, epb_num);
  if (PIPE_SUCCESS != sts) return sts;

  /*
   * Step 3 - Set Channel Rate.
   */
  sts = tof2_parde_cfg_e_chnl_rate(shdl, dev_info, port_id, false);
  if (PIPE_SUCCESS != sts) return sts;

  /*
   * Step 4 - Setup EPB.
   */
  sts = tof2_parde_cfg_epb(shdl, dev_info, port_id);
  if (PIPE_SUCCESS != sts) return sts;

  /*
   * Step 5 - Setup EBUF.
   */
  sts = tof2_parde_ebuf_chnl_fifo_ctrl(shdl, dev_info, port_id);
  if (PIPE_SUCCESS != sts) return sts;
  sts = tof2_parde_ebuf_chnl_seq(shdl, dev_info, port_id);
  if (PIPE_SUCCESS != sts) return sts;

  /*
   * Step 6 - Enable channels.
   */
  sts = tof2_parde_port_ena_egr(shdl, dev_info, port_id);
  if (PIPE_SUCCESS != sts) return sts;

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_parde_tof2_port_rmv_egr(pipe_sess_hdl_t shdl,
                                               rmt_dev_info_t *dev_info,
                                               bf_dev_port_t port_id) {
  pipe_status_t sts;

  /* Drain EPB, deparser, and EBUF and then disable channels. */
  sts = tof2_parde_port_dis_egr(shdl, dev_info, port_id);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("Dev %d port %d, error %s disabling egress",
              dev_info->dev_id,
              port_id,
              pipe_str_err(sts));
    return sts;
  }

  /* Reset channel rates. */
  sts = tof2_parde_cfg_e_chnl_rate(shdl, dev_info, port_id, true);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("Dev %d port %d, error %s resetting egress port mode",
              dev_info->dev_id,
              port_id,
              pipe_str_err(sts));
    return sts;
  }

  /* Reset sequence numbers in the parser. */
  sts = tof2_prsr_seqno_reset(shdl, dev_info, port_id, false /* Egress*/);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("Dev %d port %d, error %s resetting egress sequence number",
              dev_info->dev_id,
              port_id,
              pipe_str_err(sts));
    return sts;
  }

  /* Reset deparser FIFO pointers.
   * Reset channels to 50g mode
   * Toggle enable from 0->1->0->1->0.  On the first 0->1 transition all
   * pointers except wptr_prev are reset, on the second 0->1 transtion the
   * wptr_prev is reset, then a final 1->0 transtion to leave it in the disabled
   * state. */
  sts = tof2_parde_cfg_dprsr_chnl_rate(shdl, dev_info, port_id, true);
  if (PIPE_SUCCESS != sts) return sts;
  sts = tof2_parde_i_dprsr_mac_en(shdl, dev_info, port_id, false, true);
  if (PIPE_SUCCESS != sts) return sts;
  sts = tof2_parde_i_dprsr_mac_en(shdl, dev_info, port_id, true, true);
  if (PIPE_SUCCESS != sts) return sts;
  sts = tof2_parde_i_dprsr_mac_en(shdl, dev_info, port_id, false, true);
  if (PIPE_SUCCESS != sts) return sts;
  return sts;
}

pipe_status_t pipe_mgr_parde_tof2_port_rmv_ing(pipe_sess_hdl_t shdl,
                                               rmt_dev_info_t *dev_info,
                                               bf_dev_port_t port_id) {
  pipe_status_t sts;
  /* Drain and then disable IPB, parb, and s2p. */
  sts = tof2_parde_port_dis_ing(shdl, dev_info, port_id);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("Dev %d port %d, error %s disabling ingress",
              dev_info->dev_id,
              port_id,
              pipe_str_err(sts));
    return sts;
  }
  /* Set the following:
   *  - pipes.pardereg.pgstnreg.ipbprsr4reg[].ipbreg.glb_group.port_rates
   *  - pipes.pardereg.pgstnreg.pmergereg.ul.port_rate_cfg_8_5
   *  - pipes.pardereg.pgstnreg.pmergereg.ur.port_rate_cfg_8_5
   *  - pipes.pardereg.pgstnreg.pmergereg.ll0.port_rate_cfg_4_0
   *  - pipes.pardereg.pgstnreg.pmergereg.lr0.port_rate_cfg_4_0
   *  - pipes.pardereg.pgstnreg.s2preg.port_rate_cfg
   *  - pipes.pardereg.pgstnreg.parbreg.port_rate_cfg
   * Do not set the parser's port_rate_cfg as that can affect an adjacent
   * channel in 50g mode.
   *  - pipes.pardereg.pgstnreg.ipbprsr4reg[].prsr[].port_rate_cfg
   */
  sts = tof2_parde_cfg_i_chnl_rate(shdl, dev_info, port_id, true);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("Dev %d port %d, error %s resetting ingress rate",
              dev_info->dev_id,
              port_id,
              pipe_str_err(sts));
    return sts;
  }
  /* Set the following:
   *  - pipes.pardereg.pgstnreg.ipbprsr4reg[].prsr[].seq_reset
   */
  sts = tof2_prsr_seqno_reset(shdl, dev_info, port_id, true /* Ingress */);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("Dev %d port %d, error %s resetting ingress sequence number",
              dev_info->dev_id,
              port_id,
              pipe_str_err(sts));
    return sts;
  }
  return sts;
}

pipe_status_t pipe_mgr_parde_tof2_port_ena_ing_all(pipe_sess_hdl_t shdl,
                                                   rmt_dev_info_t *dev_info) {
  /* Write pipes.pardereg.pgstnreg.ipbprsr4reg[].ipbreg.glb_group.port_en */

  pipe_status_t sts = PIPE_SUCCESS;
  for (unsigned int logical_pipe = 0; logical_pipe < dev_info->num_active_pipes;
       ++logical_pipe) {
    for (int ipb_num = 0; ipb_num < TOF2_NUM_IPB; ++ipb_num) {
      uint8_t enable_mask =
          tof2_parde_get_chan_en_mask(dev_info, logical_pipe, ipb_num);

      pipe_status_t rc = tof2_ipb_epb_write_chan_en(
          shdl, logical_pipe, ipb_num, dev_info, 0, enable_mask, true);
      if (PIPE_SUCCESS != rc) {
        LOG_ERROR("Dev %d pipe %d ipb %d failed to enable channels, %s",
                  dev_info->dev_id,
                  logical_pipe,
                  ipb_num,
                  pipe_str_err(rc));
        sts = rc;
      }
    }
  }
  return sts;
}

pipe_status_t pipe_mgr_parde_tof2_port_ena_one(pipe_sess_hdl_t shdl,
                                               rmt_dev_info_t *dev_info,
                                               uint8_t logical_pipe,
                                               int ipb_num,
                                               bool ing_0_egr_1) {
  uint8_t enable_mask = 0;
  if (ing_0_egr_1 == 0) {
    enable_mask = tof2_parde_get_chan_en_mask(dev_info, logical_pipe, ipb_num);
  } else {
    // egress enable all
    enable_mask = 0xff;  // TOF2_NUM_CHN_PER_IPB
  }
  pipe_status_t rc = tof2_ipb_epb_write_chan_en(
      shdl, logical_pipe, ipb_num, dev_info, ing_0_egr_1, enable_mask, true);
  if (PIPE_SUCCESS != rc) {
    LOG_ERROR("Dev %d pipe %d %s ipb %d failed to enable channels, %s",
              dev_info->dev_id,
              logical_pipe,
              ing_0_egr_1 == 0 ? "ingress" : "egress",
              ipb_num,
              pipe_str_err(rc));
  }
  return rc;
}

pipe_status_t pipe_mgr_parde_tof2_port_dis_ing_all_with_dma(
    pipe_sess_hdl_t shdl, rmt_dev_info_t *dev_info) {
  pipe_bitmap_t pipe_bit_map;
  uint32_t val = 0;
  PIPE_BITMAP_INIT(&pipe_bit_map, PIPE_BMP_SIZE);
  for (unsigned int i = 0; i < dev_info->num_active_pipes; ++i) {
    PIPE_BITMAP_SET(&pipe_bit_map, i);
  }
  for (int ipb_num = 0; ipb_num < TOF2_NUM_IPB; ++ipb_num) {
    uint32_t addr = offsetof(tof2_reg,
                             pipes[0]
                                 .pardereg.pgstnreg.ipbprsr4reg[ipb_num]
                                 .ipbreg.glb_group.port_en);
    // lld_write_register(dev_info->dev_id, addr, val);
    pipe_instr_write_reg_t instr;
    construct_instr_reg_write(dev_info->dev_id, &instr, addr, val);
    uint32_t stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(addr);
    pipe_status_t sts = pipe_mgr_drv_ilist_add(
        &shdl, dev_info, &pipe_bit_map, stage, (uint8_t *)&instr, sizeof instr);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Dev %d ipb %d Failed to disable ingress port_en instr (%s)",
                dev_info->dev_id,
                ipb_num,
                pipe_str_err(sts));
      return sts;
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_parde_tof2_port_dis_ing_all(pipe_sess_hdl_t shdl,
                                                   rmt_dev_info_t *dev_info) {
  (void)shdl;
  for (unsigned int i = 0; i < dev_info->num_active_pipes; ++i) {
    bf_dev_pipe_t phy_pipe = 0;
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, i, &phy_pipe);
    for (int ipb_num = 0; ipb_num < TOF2_NUM_IPB; ++ipb_num) {
      uint32_t addr = offsetof(tof2_reg,
                               pipes[phy_pipe]
                                   .pardereg.pgstnreg.ipbprsr4reg[ipb_num]
                                   .ipbreg.glb_group.port_en);
      uint32_t val = 0;
      lld_write_register(dev_info->dev_id, addr, val);
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_parde_tof2_port_dis_one(pipe_sess_hdl_t shdl,
                                               rmt_dev_info_t *dev_info,
                                               uint8_t logical_pipe,
                                               int ipb_num,
                                               bool ing_0_egr_1) {
  pipe_bitmap_t pipe_bit_map;
  pipe_instr_write_reg_t instr;
  PIPE_BITMAP_INIT(&pipe_bit_map, PIPE_BMP_SIZE);
  PIPE_BITMAP_SET(&pipe_bit_map, logical_pipe);
  uint32_t addr;
  uint32_t val = 0;
  if (ing_0_egr_1 == 0) {
    addr = offsetof(tof2_reg,
                    pipes[0]
                        .pardereg.pgstnreg.ipbprsr4reg[ipb_num]
                        .ipbreg.glb_group.port_en);
  } else {
    addr = offsetof(tof2_reg,
                    pipes[0]
                        .pardereg.pgstnreg.epbprsr4reg[ipb_num]
                        .epbreg.glb_group.port_en);
  }
  construct_instr_reg_write(dev_info->dev_id, &instr, addr, val);
  uint32_t stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(addr);
  pipe_status_t sts = pipe_mgr_drv_ilist_add(
      &shdl, dev_info, &pipe_bit_map, stage, (uint8_t *)&instr, sizeof instr);
  if (PIPE_SUCCESS != sts) {
    LOG_ERROR("Dev %d ipb %d pipe %d, %s, Failed to disable port_en instr (%s)",
              dev_info->dev_id,
              ipb_num,
              logical_pipe,
              (ing_0_egr_1 == 0 ? "ingress" : "egress"),
              pipe_str_err(sts));
  }
  return sts;
}
static pipe_status_t tof2_prsr_write_po_action(
    pipe_sess_hdl_t shdl,
    const rmt_dev_info_t *dev_info,
    uint8_t log_pipe_mask,
    const struct pipe_mgr_tof2_prsr_bin_config *cfg,
    uint64_t dst_addr) {
  /* Memory writes are always 16B */
  int width = 16;
  int count = TOF2_PARSER_DEPTH * PIPE_MGR_TOF2_PO_WORD_WIDTH / width;

  pipe_mgr_drv_buf_t *b;
  pipe_mgr_drv_ses_state_t *st =
      pipe_mgr_drv_get_ses_state(&shdl, __func__, __LINE__);
  if (!st) {
    LOG_ERROR("%s : Fail to get ses state", __func__);
    return PIPE_UNEXPECTED;
  }

  int bwr_size = pipe_mgr_drv_buf_size(dev_info->dev_id, PIPE_MGR_DRV_BUF_BWR);
  b = pipe_mgr_drv_buf_alloc(
      st->sid, dev_info->dev_id, bwr_size, PIPE_MGR_DRV_BUF_BWR, true);
  if (!b) {
    LOG_ERROR(
        "%s : Error allocating buffer for dev %d", __func__, dev_info->dev_id);
    return PIPE_NO_SYS_RESOURCES;
  }

  PIPE_MGR_MEMCPY(
      (void *)b->addr, (uint8_t *)cfg->po_action_data, width * count);
  pipe_status_t status =
      pipe_mgr_drv_blk_wr(&shdl, width, count, 1, dst_addr, log_pipe_mask, b);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s : Write block push for prsr memory error %s",
              __func__,
              pipe_str_err(status));
    pipe_mgr_drv_buf_free(b);
    PIPE_MGR_DBGCHK(PIPE_SUCCESS == status);
    return PIPE_UNEXPECTED;
  }
  return PIPE_SUCCESS;
}
static pipe_status_t tof2_prsr_cfg_get_one_buf(pipe_sess_hdl_t shdl,
                                               const rmt_dev_info_t *dev_info,
                                               pipe_mgr_drv_buf_t **b) {
  pipe_mgr_drv_ses_state_t *st =
      pipe_mgr_drv_get_ses_state(&shdl, __func__, __LINE__);
  if (!st) {
    LOG_ERROR("%s : Fail to get ses state", __func__);
    return PIPE_UNEXPECTED;
  }
  int bwr_size = pipe_mgr_drv_buf_size(dev_info->dev_id, PIPE_MGR_DRV_BUF_BWR);
  if (bwr_size < 16 * 1024) {
    LOG_ERROR("Minimum 16kB DMA buffer required for parser init, got size %d",
              bwr_size);
    return PIPE_INVALID_ARG;
  }
  *b = pipe_mgr_drv_buf_alloc(
      st->sid, dev_info->dev_id, bwr_size, PIPE_MGR_DRV_BUF_BWR, false);
  if (!*b) {
    LOG_ERROR(
        "%s : Error allocating buffer for dev %d", __func__, dev_info->dev_id);
    return PIPE_NO_SYS_RESOURCES;
  }
  return PIPE_SUCCESS;
}

struct tof2_prsr_mem_chunk {
  uint8_t *src_data;
  uint64_t dst_addr;
  int num_lines;
};

pipe_status_t pipe_mgr_parser_config_tof2(
    pipe_sess_hdl_t sess_hdl,
    rmt_dev_info_t *dev_info,
    uint8_t gress,
    pipe_bitmap_t pipe_bmp,
    uint64_t prsr_grp_map,
    struct pipe_mgr_tof2_prsr_bin_config *cfg) {
  struct pipe_mgr_tof2_prsr_base_addr *addr_db =
      &PIPE_PRSR_ADDR(dev_info->dev_id, gress).tof2;
  pipe_status_t sts;
  uint8_t log_pipe_mask = 0;
  // extend prsr group to prsr
  for (uint32_t i = 0; i < dev_info->num_active_pipes; i++) {
    if (PIPE_BITMAP_GET(&pipe_bmp, i)) log_pipe_mask |= (1u << i);
  }
  /* Step through the parsers in groups of four since memory writes to a single
   * block of four parsers can be multicast (i.e. writes to the lowest address
   * in the group go to all four parsers in the group). */
  for (uint32_t prsr_id = 0; prsr_id < TOF2_NUM_PARSERS / 4; prsr_id++) {
    if ((prsr_grp_map & (1 << prsr_id)) == 0) continue;
    uint64_t prsr_offset = (4 * prsr_id * addr_db->prsr_step);
    /* po_action_data */
    sts = tof2_prsr_write_po_action(sess_hdl,
                                    dev_info,
                                    log_pipe_mask,
                                    cfg,
                                    addr_db->po_action_addr + prsr_offset);
    if (sts != PIPE_SUCCESS) return sts;

    /* If PVSs (parser value set) are in use and they are programmed
     * asymmetrically, that is differently across pipes, we cannot load a single
     * TCAM image to all pipes in the pipeline profile.  Here we go over all
     * pipes in the profile and check if their data is identical, as long as it
     * is they can share the image, otherwise they need separate writes. */
    unsigned int to_be_written = log_pipe_mask;
    for (uint32_t i = 0; i < dev_info->num_active_pipes; ++i) {
      if (((to_be_written >> i) & 1) == 0) continue;
      /* Lookup the TCAM data for pipe i. */
      bf_dev_pipe_t i_phy = i;
      pipe_mgr_map_pipe_id_log_to_phy(dev_info, i, &i_phy);
      uint8_t *pipe_i_tcam_data =
          (uint8_t *)PIPE_INTR_PRSR_TCAM_DATA(dev_info->dev_id, i_phy, prsr_id)
              .tof2[gress]
              .tcam_data;
      /* Start with a write mask of only pipe i, then look at other pipes and
       * include them if their data is the same. */
      unsigned int cur_write_mask = 1u << i;
      for (uint32_t j = i + 1; j < dev_info->num_active_pipes; ++j) {
        if (((to_be_written >> j) & 1) == 0) continue;
        /* If the two pipes (i and j) have the same parser data then include
         * pipe j in the write to pipe i. */
        bf_dev_pipe_t j_phy = j;
        pipe_mgr_map_pipe_id_log_to_phy(dev_info, j, &j_phy);
        uint8_t *pipe_j_tcam_data = (uint8_t *)PIPE_INTR_PRSR_TCAM_DATA(
                                        dev_info->dev_id, j_phy, prsr_id)
                                        .tof2[gress]
                                        .tcam_data;
        if (!PIPE_MGR_MEMCMP(
                pipe_i_tcam_data,
                pipe_j_tcam_data,
                TOF2_PARSER_DEPTH * PIPE_MGR_TOF2_TCAM_WORD_WIDTH)) {
          cur_write_mask |= 1u << j;
        }
      }
      /* Take the current pipe set out of the to_be_written set since they are
       * being written. */
      to_be_written &= ~cur_write_mask;

      /* Get a DMA buffer for the write. */
      pipe_mgr_drv_buf_t *b = NULL;
      sts = tof2_prsr_cfg_get_one_buf(sess_hdl, dev_info, &b);
      if (sts != PIPE_SUCCESS) return sts;

      /* Populate the TCAM data in the write buffer. */
      uint8_t *buf = b->addr;
      int total_sz = 0;
      int data_sz = TOF2_PARSER_DEPTH * PIPE_MGR_TOF2_TCAM_WORD_WIDTH;
      PIPE_MGR_MEMCPY(buf, pipe_i_tcam_data, data_sz);
      buf += data_sz;
      total_sz += data_sz;

      /* Populate the Early Action data in the buffer. */
      data_sz = TOF2_PARSER_DEPTH * PIPE_MGR_TOF2_TCAM_WORD_WIDTH;
      PIPE_MGR_MEMCPY(buf, cfg->ea_row_data, data_sz);
      buf += data_sz;
      total_sz += data_sz;

      /* Populate the counter init data in the buffer. */
      data_sz = TOF2_PARSER_INIT_RAM_DEPTH * PIPE_MGR_TOF2_TCAM_WORD_WIDTH;
      PIPE_MGR_MEMCPY(buf, (uint8_t *)cfg->ctr_init_ram_data, data_sz);
      buf += data_sz;
      total_sz += data_sz;

      /* There is a hole of 16 16B entries in the address map.
       * Write the counter-init memory again as the hole maps back to those
       * entries. */
      data_sz = TOF2_PARSER_INIT_RAM_DEPTH * PIPE_MGR_TOF2_TCAM_WORD_WIDTH;
      PIPE_MGR_MEMCPY(buf, (uint8_t *)cfg->ctr_init_ram_data, data_sz);
      buf += data_sz;
      total_sz += data_sz;

      /* Populate the 5 checksum control blocks in the buffer. */
      data_sz = TOF2_PARSER_CSUM_DEPTH * PIPE_MGR_TOF2_TCAM_WORD_WIDTH;
      PIPE_MGR_MEMCPY(buf, (uint8_t *)cfg->po_csum_ctr0_data, data_sz);
      buf += data_sz;
      total_sz += data_sz;
      PIPE_MGR_MEMCPY(buf, (uint8_t *)cfg->po_csum_ctr1_data, data_sz);
      buf += data_sz;
      total_sz += data_sz;
      PIPE_MGR_MEMCPY(buf, (uint8_t *)cfg->po_csum_ctr2_data, data_sz);
      buf += data_sz;
      total_sz += data_sz;
      PIPE_MGR_MEMCPY(buf, (uint8_t *)cfg->po_csum_ctr3_data, data_sz);
      buf += data_sz;
      total_sz += data_sz;
      PIPE_MGR_MEMCPY(buf, (uint8_t *)cfg->po_csum_ctr4_data, data_sz);
      buf += data_sz;
      total_sz += data_sz;

      /* Post the block write.  Note that all these memory writes are 16B and
       * the destination HW addresses are consecutive and start with the parser
       * TCAM. */
      uint64_t dst_addr = addr_db->tcam_addr + prsr_offset;
      int width = 16; /* All memory writes are 16B. */
      sts = pipe_mgr_drv_blk_wr(
          &sess_hdl, width, total_sz / width, 1, dst_addr, cur_write_mask, b);
      if (sts != PIPE_SUCCESS) return sts;
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tof2_iprsr_set_pri_thresh(rmt_dev_info_t *dev_info,
                                                 rmt_port_info_t *port_info,
                                                 uint32_t val) {
  bf_dev_pipe_t pipe = port_info->phy_pipe;
  int local_port = dev_info->dev_cfg.dev_port_to_local_port(port_info->port_id);
  int ipb_num = tof2_ipb_local_port_to_ipb(local_port);
  int chan_cnt = tof2_parde_speed_to_chan_cnt(port_info->speed);
  int chan_num = tof2_ipb_local_port_to_chan(local_port);
  if (-1 == chan_num || -1 == ipb_num) return PIPE_INVALID_ARG;
  int first_prsr_id = 0, prsr_cnt = 0;
  tof2_prsr_chan_speed_to_prsr_id(
      chan_num, port_info->speed, &first_prsr_id, &prsr_cnt);

  uint32_t csr = 0;
  // Possible cases are 1, 2, 4 and 8. In case of 2, 4 and 8 all parser indexes.
  // should be overwrriten, so only in 1 we have to reconstruct register value
  // first.
  if (chan_cnt == 1) {
    // In this case we have single parser with 2 ports, so lookup is for
    // port that has the same pipe, ipb and parser id.
    for (rmt_port_info_t *p_info = dev_info->port_list; p_info;
         p_info = p_info->next) {
      if (p_info->phy_pipe != pipe) continue;
      // Skip port that is being set.
      if (p_info->port_id == port_info->port_id) continue;
      int l_port = dev_info->dev_cfg.dev_port_to_local_port(p_info->port_id);
      int p_ipb_num = tof2_ipb_local_port_to_ipb(l_port);
      if (p_ipb_num != ipb_num) continue;
      int p_chan_num = tof2_ipb_local_port_to_chan(l_port);
      int p_first_prsr_id = 0, p_prsr_cnt = 0;
      tof2_prsr_chan_speed_to_prsr_id(
          p_chan_num, p_info->speed, &p_first_prsr_id, &p_prsr_cnt);
      if (p_first_prsr_id != first_prsr_id) continue;
      PIPE_MGR_DBGCHK(p_chan_num != chan_num);
      setp_tof2_prsr_reg_main_rspec_pri_thresh_pri(
          &csr, p_chan_num & 1, p_info->iprsr_pri_thresh);
      setp_tof2_prsr_reg_main_rspec_pri_thresh_pri(&csr, chan_num & 1, val);
      // Port was found, safe to stop looking.
      break;
    }
  } else {
    setp_tof2_prsr_reg_main_rspec_pri_thresh_pri(&csr, 0, val);
    setp_tof2_prsr_reg_main_rspec_pri_thresh_pri(&csr, 1, val);
  }
  for (int i = 0; i < prsr_cnt; ++i) {
    // Following part of the code will do only one write in case of 1 chnl.
    int prsr_id = first_prsr_id + i;
    uint32_t addr = offsetof(tof2_reg,
                             pipes[pipe]
                                 .pardereg.pgstnreg.ipbprsr4reg[ipb_num]
                                 .prsr[prsr_id]
                                 .pri_thresh);

    pipe_status_t sts = pipe_mgr_write_register(dev_info->dev_id, 0, addr, csr);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Dev %d port %d prsr %d Failed to set pri threshold (%s)",
                dev_info->dev_id,
                port_info->port_id,
                prsr_id,
                pipe_str_err(sts));
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
      return sts;
    }
  }

  LOG_TRACE("%s: Port %d, Configured ingress parser pririoty threshold = 0x%x",
            __func__,
            port_info->port_id,
            val);

  return (PIPE_SUCCESS);
}

pipe_status_t pipe_mgr_tof2_iprsr_get_pri_thresh(rmt_dev_info_t *dev_info,
                                                 rmt_port_info_t *port_info,
                                                 uint32_t *val) {
  bf_dev_pipe_t pipe = port_info->phy_pipe;
  int local_port = dev_info->dev_cfg.dev_port_to_local_port(port_info->port_id);
  int ipb_num = tof2_ipb_local_port_to_ipb(local_port);
  if (-1 == ipb_num) return PIPE_INVALID_ARG;
  int chan_num = tof2_ipb_local_port_to_chan(local_port);
  if (-1 == chan_num) return PIPE_INVALID_ARG;
  int first_prsr_id = 0, prsr_cnt = 0;
  tof2_prsr_chan_speed_to_prsr_id(
      chan_num, port_info->speed, &first_prsr_id, &prsr_cnt);

  uint32_t csr = 0;
  uint32_t addr = offsetof(tof2_reg,
                           pipes[pipe]
                               .pardereg.pgstnreg.ipbprsr4reg[ipb_num]
                               .prsr[first_prsr_id]
                               .pri_thresh);
  pipe_status_t sts = lld_read_register(dev_info->dev_id, addr, &csr);
  if (PIPE_SUCCESS != sts) {
    LOG_ERROR("Dev %d port %d prsr %d Failed to get pri threshold (%s)",
              dev_info->dev_id,
              port_info->port_id,
              first_prsr_id,
              pipe_str_err(sts));
    PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
    return sts;
  }

  // For chan_cnt == 1, need to get specific channel, but of all other cases
  // (2, 4, 8) values should be the same at both indices.
  *val = getp_tof2_prsr_reg_main_rspec_pri_thresh_pri(&csr, chan_num & 1);

  return (PIPE_SUCCESS);
}
