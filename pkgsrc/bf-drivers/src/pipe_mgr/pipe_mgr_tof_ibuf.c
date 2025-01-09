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
 * @file pipe_mgr_tof_ibuf.c
 * @date
 *
 * Implementation of IBUF related initializations
 */

#include "pipe_mgr_int.h"
#include <tofino_regs/tofino.h>
#include "pipe_mgr_tof_ibuf.h"
#include "pipe_mgr_drv_intf.h"

pipe_status_t pipe_mgr_ibuf_tof_set_logical_port(pipe_sess_hdl_t sess_hdl,
                                                 rmt_dev_info_t *dev_info) {
  bf_dev_id_t dev_id = dev_info->dev_id;
  int i, port;
  pipe_instr_write_reg_t instr[4];
  pipe_status_t sts = PIPE_SUCCESS;
  pipe_bitmap_t pipe_bit_map = {{0}};

  uint32_t prsr_stage = ~0;
  lld_err_t lld_sts;
  if (LLD_OK != (lld_sts = lld_sku_get_prsr_stage(dev_id, &prsr_stage))) {
    LOG_ERROR("%s:%d Failed to get prsr stage for dev %d, sts %d",
              __func__,
              __LINE__,
              dev_id,
              lld_sts);
    return PIPE_INVALID_ARG;
  }

  int num_prsrs = pipe_mgr_num_prsrs(dev_id);
  for (i = 0; i < (int)pipe_mgr_get_num_active_pipes(dev_id); ++i) {
    PIPE_BITMAP_INIT(&pipe_bit_map, PIPE_BMP_SIZE);
    PIPE_BITMAP_SET(&pipe_bit_map, i);

    for (port = 0; port < num_prsrs; ++port) {
      construct_instr_reg_write(
          dev_id,
          &instr[0],
          offsetof(Tofino,
                   pipes[0]
                       .pmarb.ibp18_reg.ibp_reg[port]
                       .ing_buf_regs.chan0_group.chnl_metadata_fix2),
          (i << 7) | (port * TOF_NUM_CHN_PER_PORT));

      construct_instr_reg_write(
          dev_id,
          &instr[1],
          offsetof(Tofino,
                   pipes[0]
                       .pmarb.ibp18_reg.ibp_reg[port]
                       .ing_buf_regs.chan1_group.chnl_metadata_fix2),
          (i << 7) | ((port * TOF_NUM_CHN_PER_PORT) + 1));
      construct_instr_reg_write(
          dev_id,
          &instr[2],
          offsetof(Tofino,
                   pipes[0]
                       .pmarb.ibp18_reg.ibp_reg[port]
                       .ing_buf_regs.chan2_group.chnl_metadata_fix2),
          (i << 7) | ((port * TOF_NUM_CHN_PER_PORT) + 2));
      construct_instr_reg_write(
          dev_id,
          &instr[3],
          offsetof(Tofino,
                   pipes[0]
                       .pmarb.ibp18_reg.ibp_reg[port]
                       .ing_buf_regs.chan3_group.chnl_metadata_fix2),
          (i << 7) | ((port * TOF_NUM_CHN_PER_PORT) + 3));

      int j;
      for (j = 0; j < 4; ++j) {
        sts = pipe_mgr_drv_ilist_add(&sess_hdl,
                                     dev_info,
                                     &pipe_bit_map,
                                     prsr_stage,
                                     (uint8_t *)(&instr[j]),
                                     sizeof(instr[j]));

        if (PIPE_SUCCESS != sts) {
          LOG_ERROR("Failed to add ibuf logical-port to instruction list (%d)",
                    sts);
          PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
          return sts;
        }
      }
    }
  }

  return sts;
}

pipe_status_t pipe_mgr_ibuf_tof_set_version_bits(pipe_sess_hdl_t sess_hdl,
                                                 rmt_dev_info_t *dev_info,
                                                 uint8_t version) {
  bf_dev_id_t dev_id = dev_info->dev_id;
  int i, chan, port;
  uint32_t ver, stage;
  pipe_status_t sts = PIPE_SUCCESS;
  pipe_bitmap_t pipe_bit_map = {{0}};

  if (LLD_OK != lld_sku_get_prsr_stage(dev_id, &stage)) {
    LOG_ERROR("%s:%d Could not map parser stage for dev %d",
              __func__,
              __LINE__,
              dev_id);
    return PIPE_INVALID_ARG;
  }

  ver = (uint32_t)(version & 0x3) << 12;
  int I = pipe_mgr_get_num_active_pipes(dev_id);
  int num_prsrs = pipe_mgr_num_prsrs(dev_id);
  for (i = 0; i < I; ++i) {
    PIPE_BITMAP_INIT(&pipe_bit_map, PIPE_BMP_SIZE);
    PIPE_BITMAP_SET(&pipe_bit_map, i);
    /* Set all chan0 of every port */
    for (port = 0; port < num_prsrs; ++port) {
      for (chan = 0; chan < 4; ++chan) {
        pipe_instr_write_reg_t instr;
        size_t addr =
            chan == 0
                ? offsetof(Tofino,
                           pipes[0]
                               .pmarb.ibp18_reg.ibp_reg[port]
                               .ing_buf_regs.chan0_group.chnl_metadata_fix2)
                : chan == 1
                      ? offsetof(
                            Tofino,
                            pipes[0]
                                .pmarb.ibp18_reg.ibp_reg[port]
                                .ing_buf_regs.chan1_group.chnl_metadata_fix2)
                      : chan == 2 ? offsetof(Tofino,
                                             pipes[0]
                                                 .pmarb.ibp18_reg.ibp_reg[port]
                                                 .ing_buf_regs.chan2_group
                                                 .chnl_metadata_fix2)
                                  : offsetof(Tofino,
                                             pipes[0]
                                                 .pmarb.ibp18_reg.ibp_reg[port]
                                                 .ing_buf_regs.chan3_group
                                                 .chnl_metadata_fix2);

        construct_instr_reg_write(
            dev_id,
            &instr,
            addr,
            ver | (i << 7) | (port * TOF_NUM_CHN_PER_PORT + chan));

        sts = pipe_mgr_drv_ilist_add(&sess_hdl,
                                     dev_info,
                                     &pipe_bit_map,
                                     stage,
                                     (uint8_t *)&instr,
                                     sizeof instr);

        if (PIPE_SUCCESS != sts) {
          LOG_ERROR(
              "Failed to add ibuf logical-port to instruction list (%s), dev "
              "%d pipe %d prsr %d chan %d",
              pipe_str_err(sts),
              dev_id,
              i,
              port,
              chan);
          PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
          return sts;
        }
      }
    }
  }
  sts = pipe_mgr_drv_ilist_push(&sess_hdl, NULL /*cb*/, NULL /*udata*/);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR(
        "Failed to post instruction to update ibuf version bits dev %d err %s",
        dev_id,
        pipe_str_err(sts));
  }

  return sts;
}

pipe_status_t pipe_mgr_ibuf_tof_disable_all_chan(pipe_sess_hdl_t sess_hdl,
                                                 rmt_dev_info_t *dev_info) {
  (void)sess_hdl;
  bf_dev_id_t dev_id = dev_info->dev_id;
  bf_dev_pipe_t num_pipes = pipe_mgr_get_num_active_pipes(dev_id);
  int num_prsrs = pipe_mgr_num_prsrs(dev_id);
  for (bf_dev_pipe_t log_pipe = 0; log_pipe < num_pipes; ++log_pipe) {
    bf_dev_pipe_t phy_pipe = 0;
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe, &phy_pipe);
    for (int prsr = 0; prsr < num_prsrs; ++prsr) {
      for (int chan = 0; chan < 4; ++chan) {
        uint32_t addr =
            chan == 0
                ? offsetof(Tofino,
                           pipes[phy_pipe]
                               .pmarb.ibp18_reg.ibp_reg[prsr]
                               .ing_buf_regs.chan0_group.chnl_ctrl)
                : chan == 1
                      ? offsetof(Tofino,
                                 pipes[phy_pipe]
                                     .pmarb.ibp18_reg.ibp_reg[prsr]
                                     .ing_buf_regs.chan1_group.chnl_ctrl)
                      : chan == 2
                            ? offsetof(Tofino,
                                       pipes[phy_pipe]
                                           .pmarb.ibp18_reg.ibp_reg[prsr]
                                           .ing_buf_regs.chan2_group.chnl_ctrl)
                            : offsetof(Tofino,
                                       pipes[phy_pipe]
                                           .pmarb.ibp18_reg.ibp_reg[prsr]
                                           .ing_buf_regs.chan3_group.chnl_ctrl);
        uint32_t val = 0;
        lld_write_register(dev_id, addr, val);
      }
    }
  }
  return PIPE_SUCCESS;
}

static pipe_status_t ibuf_tof_wr_chan_ctrl(pipe_sess_hdl_t shdl,
                                           rmt_dev_info_t *dev_info,
                                           int phy_pipe,
                                           int log_pipe,
                                           int group,
                                           int chan,
                                           uint32_t val,
                                           bool use_dma) {
  uint32_t addr = 0;
  switch (chan) {
    case 0:
      addr = offsetof(Tofino,
                      pipes[phy_pipe]
                          .pmarb.ibp18_reg.ibp_reg[group]
                          .ing_buf_regs.chan0_group.chnl_ctrl);
      break;
    case 1:
      addr = offsetof(Tofino,
                      pipes[phy_pipe]
                          .pmarb.ibp18_reg.ibp_reg[group]
                          .ing_buf_regs.chan1_group.chnl_ctrl);
      break;
    case 2:
      addr = offsetof(Tofino,
                      pipes[phy_pipe]
                          .pmarb.ibp18_reg.ibp_reg[group]
                          .ing_buf_regs.chan2_group.chnl_ctrl);
      break;
    case 3:
      addr = offsetof(Tofino,
                      pipes[phy_pipe]
                          .pmarb.ibp18_reg.ibp_reg[group]
                          .ing_buf_regs.chan3_group.chnl_ctrl);
      break;
  }
  if (!use_dma) {
    return pipe_mgr_write_register(dev_info->dev_id, 0, addr, val);
  } else {
    pipe_bitmap_t pipe_bit_map;
    PIPE_BITMAP_INIT(&pipe_bit_map, PIPE_BMP_SIZE);
    PIPE_BITMAP_SET(&pipe_bit_map, log_pipe);

    uint32_t stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(addr);

    pipe_instr_write_reg_t instr;
    construct_instr_reg_write(dev_info->dev_id, &instr, addr, val);
    return pipe_mgr_drv_ilist_add(
        &shdl, dev_info, &pipe_bit_map, stage, (uint8_t *)&instr, sizeof instr);
  }
}

static pipe_status_t ibuf_set_chnl_ctrl_reg(rmt_dev_info_t *dev_info,
                                            rmt_port_info_t *port_info,
                                            uint32_t val,
                                            pipe_sess_hdl_t shdl,
                                            bool use_dma) {
  // Enable mac channel
  // set channel credit based on speed
  int phy_pipe = 0, pg, ch, i;
  int log_pipe = dev_info->dev_cfg.dev_port_to_pipe(port_info->port_id);
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_info->port_id);
  rmt_port_info_t *all_port_info = NULL;
  uint32_t buddy_chnl_val;
  uint8_t buddy_port, buddy_chnl_count = 0;
  bf_dev_port_t first_port_in_pg;
  uint8_t port_speed[TOF_NUM_CHN_PER_PORT];
  bf_dev_id_t dev_id = dev_info->dev_id;

  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);

  phy_pipe = port_info->phy_pipe;

  pg = port / TOF_NUM_CHN_PER_PORT;  // Port group has 4 ports/channels
  ch = port % TOF_NUM_CHN_PER_PORT;

  // write to channel-control register.
  // LOG_TRACE("IBUF %s: pipe = %d, port = %d, channel = %d Control reg set to =
  // %d\n", __func__, phy_pipe, port, ch, val);

  ibuf_tof_wr_chan_ctrl(
      shdl, dev_info, phy_pipe, log_pipe, pg, ch, val, use_dma);

  /*
   * In 100G and 50G mode, clear  channel_enable on unused channels.
   * and set chnl-mode on all channels in port-bundle (2 channel bundle
   * in 40G/50G mode, 4 channel bundle in 1G/10G/25G mode)
   */
  buddy_chnl_val = val;
  setp_chnl_ctrl_chnl_ena(&buddy_chnl_val, 0);  // Clear enable bit
  switch (port_info->speed) {
    case BF_SPEED_50G:
      ibuf_tof_wr_chan_ctrl(shdl,
                            dev_info,
                            phy_pipe,
                            log_pipe,
                            pg,
                            ch + 1,
                            buddy_chnl_val,
                            use_dma);
      break;
    case BF_SPEED_100G:
    case BF_SPEED_40G:
      ibuf_tof_wr_chan_ctrl(
          shdl, dev_info, phy_pipe, log_pipe, pg, 1, buddy_chnl_val, use_dma);
      ibuf_tof_wr_chan_ctrl(
          shdl, dev_info, phy_pipe, log_pipe, pg, 2, buddy_chnl_val, use_dma);
      ibuf_tof_wr_chan_ctrl(
          shdl, dev_info, phy_pipe, log_pipe, pg, 3, buddy_chnl_val, use_dma);
      break;
    default:
      break;
  }

  // Config for other channels in this port/mac-block need to be adjusted.
  // The reason is that channels that are in power on default config
  // (100G mode) need to be adjusted if any of the 4 channels is added in
  // 10G or 25G mode.  Similarly remaining 2 cahnnels of buddy port(50G/40G)
  // need to adjusted. If not adjusted, ibuf to parser path doesn't work.
  // Parser doesn't parse any packets / ibuf doesn't push packets to parser.
  if (port_info->speed != BF_SPEED_100G) {
    first_port_in_pg = port_info->port_id & 0xFFFFFFFC;
    for (i = 0; i < TOF_NUM_CHN_PER_PORT; i++) {
      all_port_info = pipe_mgr_get_port_info(dev_id, first_port_in_pg + i);
      if (!all_port_info) {
        port_speed[i] = BF_SPEED_NONE;  // Unpopulated yet
      } else {
        port_speed[i] = all_port_info->speed;
      }
    }
    switch (port_info->speed) {
      case BF_SPEED_1G:
      case BF_SPEED_10G:
      case BF_SPEED_25G:
        // There can be upto 3 channels that are un-inited or not added.
        buddy_chnl_count = 4;
        buddy_port = 0;  // always start with first channel in the block
                         // skip channel under config.
        break;
      case BF_SPEED_50G:
        if (ch) {
          buddy_port = 0;
        } else {
          buddy_port = 2;
        }
        buddy_chnl_count = 1;
        break;
      default:
        break;
    }
    // If buddy port is NONE or not added, channels correspondig to
    // buddy ports has power on default config (=100G configs).
    // If port/channel under config is in 1G/10G/25G, make buddy
    // channels mode also 10G/25G mode.
    // If port/channel under config is in 40G/50G, make buddy
    // channels mode also 40G/50G mode.
    for (i = 0; i < buddy_chnl_count; i++) {
      if (port_speed[buddy_port + i] == BF_SPEED_NONE) {
        ibuf_tof_wr_chan_ctrl(shdl,
                              dev_info,
                              phy_pipe,
                              log_pipe,
                              pg,
                              buddy_port + i,
                              buddy_chnl_val,
                              use_dma);
      }
    }
  }
  return (PIPE_SUCCESS);
}

pipe_status_t ibuf_set_chnl_ctrl(rmt_dev_info_t *dev_info,
                                 bf_dev_port_t port_id,
                                 bool chnl_enable,
                                 pipe_sess_hdl_t shdl,
                                 bool use_dma) {
  uint32_t val = 0;
  rmt_port_info_t *port_info = NULL;
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  bf_dev_id_t dev_id = dev_info->dev_id;

  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);

  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port_id);
    return PIPE_INVALID_ARG;
  }

  // LOG_TRACE("IBUF %s: pipe = %d, port = %d, port speed = %d\n", __func__,
  // dev_info->dev_cfg.dev_port_to_pipe(port_id), port, port_info->speed);

  if (chnl_enable) {
    setp_chnl_ctrl_chnl_ena(&val, 1);
  } else {
    setp_chnl_ctrl_chnl_ena(&val, 0);
  }
  switch (port_info->speed) {
    case BF_SPEED_50G:
      setp_chnl_ctrl_chnl_mode(&val, 1);
      break;
    case BF_SPEED_100G:
    case BF_SPEED_40G:
      setp_chnl_ctrl_chnl_mode(&val, 0);
      break;
    case BF_SPEED_1G:
    case BF_SPEED_10G:
    case BF_SPEED_25G:
    default:
      setp_chnl_ctrl_chnl_mode(&val, 2);
      break;
  }
  setp_chnl_ctrl_dis_cong(&val, port_info->disable_cong_notif);
  setp_chnl_ctrl_en_tx_xoff(&val, port_info->pause_mac);

  return ibuf_set_chnl_ctrl_reg(dev_info, port_info, val, shdl, use_dma);
}

static pipe_status_t ibuf_set_port_channel_seq(rmt_dev_info_t *dev_info,
                                               bf_dev_port_t port_id) {
  bf_dev_pipe_t pipe = 0;
  int pg, i;
  uint32_t val = 0, chan_seq = 0;
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  bf_dev_port_t first_port_in_pg;
  uint8_t port_speed[TOF_NUM_CHN_PER_PORT];
  bf_dev_id_t dev_id = dev_info->dev_id;

  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);

  rmt_port_info_t *port_info = NULL, *all_port_info = NULL;

  // Program ibuf.glb_ctrl.channel_seq = 00 (100G) / 88 (both ports in 50G/40G)
  // / d8 / 98 / c8
  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port_id);
    return PIPE_INVALID_ARG;
  }
  pipe = port_info->phy_pipe;
  /*
   *  Port Speed                  Channel Seq
   *  -----------                 -----------
   *
   *  100G                        0
   *  40G                         0
   *
   *  50Gx50G                     0x88
   *  50GxNAxNA  (chan0 is 50G)   0x88
   *  50Gx25GxNA (ch3 is unused)  0xC8
   *  50GxNAx25G (ch0 is 50G,     0xC8
   *              ch2 unused)
   *  50Gx25Gx25G(ch0 is 50G)     0xC8
   *
   *  NAxNAx50G  (chan2 is 50G)   0x88
   *  25GxNAx50G (ch2 is 50G)     0x98
   *  NAx25Gx50G (ch2 is 50G)     0x98
   *  25Gx25Gx50G(ch2 is 50G)     0x98
   *
   *  25Gx25Gx25Gx25G             0xD8
   */
  if ((port_info->speed != BF_SPEED_100G) &&
      (port_info->speed != BF_SPEED_40G)) {
    first_port_in_pg = port_id & 0xFFFFFFFC;
    for (i = 0; i < TOF_NUM_CHN_PER_PORT; i++) {
      all_port_info = pipe_mgr_get_port_info(dev_id, first_port_in_pg + i);
      if (!all_port_info) {
        port_speed[i] = BF_SPEED_NONE;  // Unpopulated yet
      } else {
        port_speed[i] = all_port_info->speed;
      }
    }
    if (port_speed[0] == BF_SPEED_50G && port_speed[2] == BF_SPEED_50G) {
      chan_seq = 0x88;
    } else if (port_speed[0] == BF_SPEED_50G &&
               (port_speed[2] == BF_SPEED_NONE) &&
               (port_speed[3] == BF_SPEED_NONE)) {
      chan_seq = 0x88;
    } else if (port_speed[0] == BF_SPEED_50G &&
               (port_speed[2] == BF_SPEED_25G || port_speed[2] == BF_SPEED_1G ||
                port_speed[2] == BF_SPEED_10G) &&
               (port_speed[3] == BF_SPEED_NONE)) {
      chan_seq = 0xC8;
    } else if (port_speed[0] == BF_SPEED_50G &&
               (port_speed[2] == BF_SPEED_NONE) &&
               (port_speed[3] == BF_SPEED_25G || port_speed[3] == BF_SPEED_1G ||
                port_speed[3] == BF_SPEED_10G)) {
      chan_seq = 0xC8;
    } else if (port_speed[0] == BF_SPEED_50G &&
               (port_speed[2] == BF_SPEED_25G || port_speed[2] == BF_SPEED_1G ||
                port_speed[2] == BF_SPEED_10G) &&
               (port_speed[3] == BF_SPEED_25G || port_speed[3] == BF_SPEED_1G ||
                port_speed[3] == BF_SPEED_10G)) {
      chan_seq = 0xC8;
    } else if (port_speed[2] == BF_SPEED_50G &&
               (port_speed[0] == BF_SPEED_NONE) &&
               (port_speed[1] == BF_SPEED_NONE)) {
      chan_seq = 0x88;
    } else if (port_speed[2] == BF_SPEED_50G &&
               (port_speed[0] == BF_SPEED_25G || port_speed[0] == BF_SPEED_1G ||
                port_speed[0] == BF_SPEED_10G) &&
               (port_speed[1] == BF_SPEED_NONE)) {
      chan_seq = 0x98;
    } else if (port_speed[2] == BF_SPEED_50G &&
               (port_speed[0] == BF_SPEED_NONE) &&
               (port_speed[1] == BF_SPEED_25G || port_speed[1] == BF_SPEED_1G ||
                port_speed[1] == BF_SPEED_10G)) {
      chan_seq = 0x98;
    } else if (port_speed[2] == BF_SPEED_50G &&
               (port_speed[0] == BF_SPEED_25G || port_speed[0] == BF_SPEED_1G ||
                port_speed[0] == BF_SPEED_10G) &&
               (port_speed[1] == BF_SPEED_25G || port_speed[1] == BF_SPEED_1G ||
                port_speed[1] == BF_SPEED_10G)) {
      chan_seq = 0x98;
    } else if ((port_speed[2] == BF_SPEED_25G || port_speed[2] == BF_SPEED_1G ||
                port_speed[2] == BF_SPEED_10G) &&
               (port_speed[0] == BF_SPEED_25G || port_speed[0] == BF_SPEED_1G ||
                port_speed[0] == BF_SPEED_10G)) {
      chan_seq = 0xD8;
    } else {
      // 4x25, 4x10G, 4x1G case
      chan_seq = 0xD8;
    }
  }

  switch (port_info->speed) {
    case BF_SPEED_50G:
      setp_glb_ctrl_channel_seq(&val, chan_seq);
      setp_glb_ctrl_mult_thrd(&val, 0);

      break;
    case BF_SPEED_100G:
    case BF_SPEED_40G:
      setp_glb_ctrl_channel_seq(&val, 0);
      setp_glb_ctrl_mult_thrd(&val, 1);  // only in 100G mode
      break;
    case BF_SPEED_1G:
    case BF_SPEED_10G:
    case BF_SPEED_25G:
    default:
      setp_glb_ctrl_channel_seq(&val, chan_seq);
      setp_glb_ctrl_mult_thrd(&val, 0);
      break;
  }
  LOG_TRACE(
      "IBUF %s: pipe = %d, port = %d, port speed = %d, channel-seq = 0x%x",
      __func__,
      pipe,
      port,
      port_info->speed,
      chan_seq);
  pg = port / TOF_NUM_CHN_PER_PORT;  // Port group has 4 ports/channels
  if (port_info->speed == BF_SPEED_100G || port_info->speed == BF_SPEED_40G) {
    setp_glb_ctrl_mult_thrd(&val, 0);  //  Toggle multi-thread bit
    pipe_mgr_write_register(dev_id,
                            0,
                            offsetof(Tofino,
                                     pipes[pipe]
                                         .pmarb.ibp18_reg.ibp_reg[pg]
                                         .ing_buf_regs.glb_group.glb_ctrl),
                            val);
    setp_glb_ctrl_mult_thrd(&val, 1);  //  Toggle multi-thread bit
  }
  pipe_mgr_write_register(dev_id,
                          0,
                          offsetof(Tofino,
                                   pipes[pipe]
                                       .pmarb.ibp18_reg.ibp_reg[pg]
                                       .ing_buf_regs.glb_group.glb_ctrl),
                          val);

  return (PIPE_SUCCESS);
}

static pipe_status_t ibuf_set_almost_full_threshold(rmt_dev_info_t *dev_info,
                                                    bf_dev_port_t port_id,
                                                    uint16_t low_wm_bytes,
                                                    uint16_t hi_wm_bytes) {
  bf_dev_pipe_t pipe = 0;
  int pg, ch;
  uint32_t val;
  pipe_status_t sts = PIPE_SUCCESS;
  rmt_port_info_t *port_info = NULL;
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);
  bf_dev_id_t dev_id = dev_info->dev_id;

  // enable control bit to turn on notification
  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port_id);
    return PIPE_INVALID_ARG;
  }
  pipe = port_info->phy_pipe;

  // Now set water mark values
  pg = port / TOF_NUM_CHN_PER_PORT;  // Port group has 4 ports/channels
  ch = port % TOF_NUM_CHN_PER_PORT;

  val = 0;
  setp_chnl_afull_chnl_thr_hi_afull(&val, hi_wm_bytes);
  setp_chnl_afull_chnl_thr_lo_afull(&val, low_wm_bytes);
  switch (ch) {
    case 0:
      pipe_mgr_write_register(
          dev_id,
          0,
          offsetof(Tofino,
                   pipes[pipe]
                       .pmarb.ibp18_reg.ibp_reg[pg]
                       .ing_buf_regs.chan0_group.chnl_afull),
          val);
      break;
    case 1:
      pipe_mgr_write_register(
          dev_id,
          0,
          offsetof(Tofino,
                   pipes[pipe]
                       .pmarb.ibp18_reg.ibp_reg[pg]
                       .ing_buf_regs.chan1_group.chnl_afull),
          val);
      break;
    case 2:
      pipe_mgr_write_register(
          dev_id,
          0,
          offsetof(Tofino,
                   pipes[pipe]
                       .pmarb.ibp18_reg.ibp_reg[pg]
                       .ing_buf_regs.chan2_group.chnl_afull),
          val);
      break;
    case 3:
      pipe_mgr_write_register(
          dev_id,
          0,
          offsetof(Tofino,
                   pipes[pipe]
                       .pmarb.ibp18_reg.ibp_reg[pg]
                       .ing_buf_regs.chan3_group.chnl_afull),
          val);
      break;
    default:
      break;
  }
  return sts;
}

static pipe_status_t ibuf_set_drop_threshold(rmt_dev_info_t *dev_info,
                                             bf_dev_port_t port_id,
                                             uint16_t low_thrd,
                                             uint16_t hi_thrd) {
  bf_dev_pipe_t pipe = 0;
  int pg, ch;
  uint32_t val;
  pipe_status_t sts = PIPE_SUCCESS;
  rmt_port_info_t *port_info = NULL;
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);
  bf_dev_id_t dev_id = dev_info->dev_id;

  // enable control bit to turn on notification
  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port_id);
    return PIPE_INVALID_ARG;
  }
  pipe = port_info->phy_pipe;

  // Now set water mark values
  pg = port / TOF_NUM_CHN_PER_PORT;  // Port group has 4 ports/channels
  ch = port % TOF_NUM_CHN_PER_PORT;

  val = 0;
  setp_chnl_drop_chnl_thr_hi_drop(&val, hi_thrd);
  setp_chnl_drop_chnl_thr_lo_drop(&val, low_thrd);
  switch (ch) {
    case 0:
      pipe_mgr_write_register(dev_id,
                              0,
                              offsetof(Tofino,
                                       pipes[pipe]
                                           .pmarb.ibp18_reg.ibp_reg[pg]
                                           .ing_buf_regs.chan0_group.chnl_drop),
                              val);
      break;
    case 1:
      pipe_mgr_write_register(dev_id,
                              0,
                              offsetof(Tofino,
                                       pipes[pipe]
                                           .pmarb.ibp18_reg.ibp_reg[pg]
                                           .ing_buf_regs.chan1_group.chnl_drop),
                              val);
      break;
    case 2:
      pipe_mgr_write_register(dev_id,
                              0,
                              offsetof(Tofino,
                                       pipes[pipe]
                                           .pmarb.ibp18_reg.ibp_reg[pg]
                                           .ing_buf_regs.chan2_group.chnl_drop),
                              val);
      break;
    case 3:
      pipe_mgr_write_register(dev_id,
                              0,
                              offsetof(Tofino,
                                       pipes[pipe]
                                           .pmarb.ibp18_reg.ibp_reg[pg]
                                           .ing_buf_regs.chan3_group.chnl_drop),
                              val);
      break;
    default:
      break;
  }
  return sts;
}

static pipe_status_t ibuf_set_chnl_ptr_fifo(rmt_dev_info_t *dev_info,
                                            bf_dev_port_t port_id,
                                            uint16_t fifo_size) {
  bf_dev_pipe_t pipe = 0;
  int pg, ch;
  uint32_t val;
  pipe_status_t sts = PIPE_SUCCESS;
  rmt_port_info_t *port_info = NULL;
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);
  bf_dev_id_t dev_id = dev_info->dev_id;

  // enable control bit to turn on notification
  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port_id);
    return PIPE_INVALID_ARG;
  }
  pipe = port_info->phy_pipe;

  // Now set water mark values
  pg = port / TOF_NUM_CHN_PER_PORT;  // Port group has 4 ports/channels
  ch = port % TOF_NUM_CHN_PER_PORT;

  val = 0;
  setp_chnl_ptr_fifo_min_max_chnl_min_ptrff_addr(&val, fifo_size * ch);
  setp_chnl_ptr_fifo_min_max_chnl_max_ptrff_addr(
      &val, (fifo_size * ch) + (fifo_size - 1));
  switch (ch) {
    case 0:
      pipe_mgr_write_register(
          dev_id,
          0,
          offsetof(Tofino,
                   pipes[pipe]
                       .pmarb.ibp18_reg.ibp_reg[pg]
                       .ing_buf_regs.chan0_group.chnl_ptr_fifo_min_max),
          val);
      break;
    case 1:
      pipe_mgr_write_register(
          dev_id,
          0,
          offsetof(Tofino,
                   pipes[pipe]
                       .pmarb.ibp18_reg.ibp_reg[pg]
                       .ing_buf_regs.chan1_group.chnl_ptr_fifo_min_max),
          val);
      break;
    case 2:
      pipe_mgr_write_register(
          dev_id,
          0,
          offsetof(Tofino,
                   pipes[pipe]
                       .pmarb.ibp18_reg.ibp_reg[pg]
                       .ing_buf_regs.chan2_group.chnl_ptr_fifo_min_max),
          val);
      break;
    case 3:
      pipe_mgr_write_register(
          dev_id,
          0,
          offsetof(Tofino,
                   pipes[pipe]
                       .pmarb.ibp18_reg.ibp_reg[pg]
                       .ing_buf_regs.chan3_group.chnl_ptr_fifo_min_max),
          val);
      break;
    default:
      break;
  }
  return sts;
}

static pipe_status_t ibuf_set_chnl_recirc_fifo(rmt_dev_info_t *dev_info,
                                               bf_dev_port_t port_id,
                                               uint16_t fifo_size) {
  bf_dev_pipe_t pipe = 0;
  int pg, ch;
  uint32_t val;
  pipe_status_t sts = PIPE_SUCCESS;
  rmt_port_info_t *port_info = NULL;
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);
  bf_dev_id_t dev_id = dev_info->dev_id;

  // enable control bit to turn on notification
  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port_id);
    return PIPE_INVALID_ARG;
  }
  pipe = port_info->phy_pipe;

  // Now set water mark values
  pg = port / TOF_NUM_CHN_PER_PORT;  // Port group has 4 ports/channels
  ch = port % TOF_NUM_CHN_PER_PORT;

  val = 0;
  setp_chnl_recirc_fifo_min_max_chnl_min_recirc_addr(&val, fifo_size * ch);
  setp_chnl_recirc_fifo_min_max_chnl_max_recirc_addr(
      &val, (fifo_size * ch) + (fifo_size - 1));
  switch (ch) {
    case 0:
      pipe_mgr_write_register(
          dev_id,
          0,
          offsetof(Tofino,
                   pipes[pipe]
                       .pmarb.ibp18_reg.ibp_reg[pg]
                       .ing_buf_regs.chan0_group.chnl_recirc_fifo_min_max),
          val);
      break;
    case 1:
      pipe_mgr_write_register(
          dev_id,
          0,
          offsetof(Tofino,
                   pipes[pipe]
                       .pmarb.ibp18_reg.ibp_reg[pg]
                       .ing_buf_regs.chan1_group.chnl_recirc_fifo_min_max),
          val);
      break;
    case 2:
      pipe_mgr_write_register(
          dev_id,
          0,
          offsetof(Tofino,
                   pipes[pipe]
                       .pmarb.ibp18_reg.ibp_reg[pg]
                       .ing_buf_regs.chan2_group.chnl_recirc_fifo_min_max),
          val);
      break;
    case 3:
      pipe_mgr_write_register(
          dev_id,
          0,
          offsetof(Tofino,
                   pipes[pipe]
                       .pmarb.ibp18_reg.ibp_reg[pg]
                       .ing_buf_regs.chan3_group.chnl_recirc_fifo_min_max),
          val);
      break;
    default:
      break;
  }
  return sts;
}

static pipe_status_t ibuf_set_port_speed_based_threshold(
    rmt_dev_info_t *dev_info, bf_dev_port_t port_id) {
  bf_dev_id_t dev_id = dev_info->dev_id;
  uint32_t afull_low_thrd, afull_hi_thrd;
  uint32_t drop_low_thrd, drop_hi_thrd;
  uint32_t fifo_size, recirc_size;
  rmt_port_info_t *port_info = NULL;
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  pipe_status_t sts = PIPE_SUCCESS;

  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);

  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port_id);
    return PIPE_INVALID_ARG;
  }
  switch (port_info->speed) {
    case BF_SPEED_50G:
      afull_low_thrd = 0x200;
      afull_hi_thrd = 0x280;
      drop_low_thrd = 0x280;
      drop_hi_thrd = 0x2f8;
      fifo_size = 0x80;
      recirc_size = 0x20;
      if (port >= 64) {
        afull_low_thrd = 0x1c0;
        afull_hi_thrd = 0x1e0;
        drop_low_thrd = 0x1d0;
        drop_hi_thrd = 0x1f0;
      }
      break;
    case BF_SPEED_1G:
    case BF_SPEED_10G:
    case BF_SPEED_25G:
      afull_low_thrd = 0x180;
      afull_hi_thrd = 0x1c0;
      drop_low_thrd = 0x1a0;
      drop_hi_thrd = 0x1e0;
      fifo_size = 0x40;
      recirc_size = 0x10;
      if (port >= 64) {
        afull_low_thrd = 0xa0;
        afull_hi_thrd = 0xc0;
        drop_low_thrd = 0xe0;
        drop_hi_thrd = 0xf0;
      }
      break;
    case BF_SPEED_100G:
    case BF_SPEED_40G:  // 40G config will be like 100G
    default:
      // Need to match reset values...
      // from reg spec..
      afull_low_thrd = 0x400;
      afull_hi_thrd = 0x500;
      drop_low_thrd = 0x500;
      drop_hi_thrd = 0x5f0;
      fifo_size = 0x100;
      recirc_size = 0x40;
      if (port >= 64) {
        afull_low_thrd = 0x3a0;
        afull_hi_thrd = 0x3c0;
        drop_low_thrd = 0x3e0;
        drop_hi_thrd = 0x3f0;
      }
      break;
  }
  sts = ibuf_set_almost_full_threshold(
      dev_info, port_id, afull_low_thrd, afull_hi_thrd);
  sts |=
      ibuf_set_drop_threshold(dev_info, port_id, drop_low_thrd, drop_hi_thrd);
  sts |= ibuf_set_chnl_ptr_fifo(dev_info, port_id, fifo_size);
  sts |= ibuf_set_chnl_recirc_fifo(dev_info, port_id, recirc_size);

  return (sts);
}

static pipe_status_t ibuf_set_bank_watermark_drop_threshold(
    rmt_dev_info_t *dev_info, bf_dev_port_t port_id) {
  bf_dev_pipe_t pipe;
  int pg;
  uint32_t val = 0;
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  rmt_port_info_t *port_info = NULL;
  bf_dev_id_t dev_id = dev_info->dev_id;

  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port_id);
    return PIPE_INVALID_ARG;
  }
  pipe = port_info->phy_pipe;

  setp_bank_watermark_drop_thr_bk_hi_drop(&val, 0xc);
  setp_bank_watermark_drop_thr_bk_lo_drop(&val, 0x10);

  pg = port / TOF_NUM_CHN_PER_PORT;
  return (pipe_mgr_write_register(
      dev_id,
      0,
      offsetof(Tofino,
               pipes[pipe]
                   .pmarb.ibp18_reg.ibp_reg[pg]
                   .ing_buf_regs.glb_group.bank_watermark_drop),
      val));
}

static pipe_status_t ibuf_set_bank_watermark_almostfull_threshold(
    rmt_dev_info_t *dev_info, bf_dev_port_t port_id) {
  bf_dev_pipe_t pipe;
  int pg;
  uint32_t val = 0;
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  rmt_port_info_t *port_info = NULL;
  bf_dev_id_t dev_id = dev_info->dev_id;

  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port_id);
    return PIPE_INVALID_ARG;
  }
  pipe = port_info->phy_pipe;

  setp_bank_watermark_afull_thr_bk_lo_afull(&val, 0x60);
  setp_bank_watermark_afull_thr_bk_hi_afull(&val, 0x50);

  pg = port / TOF_NUM_CHN_PER_PORT;
  return (pipe_mgr_write_register(
      dev_id,
      0,
      offsetof(Tofino,
               pipes[pipe]
                   .pmarb.ibp18_reg.ibp_reg[pg]
                   .ing_buf_regs.glb_group.bank_watermark_afull),
      val));
}

pipe_status_t pipe_mgr_ibuf_tof_port_set_drop_threshold(
    rmt_dev_info_t *dev_info,
    bf_dev_port_t port_id,
    uint32_t drop_hi_thrd,
    uint32_t drop_low_thrd) {
  bf_dev_id_t dev_id = dev_info->dev_id;
  rmt_port_info_t *port_info = NULL;
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  pipe_status_t sts = PIPE_SUCCESS;

  if (port >= TOF_NUM_PORTS_PER_PIPE) {
    return (PIPE_INVALID_ARG);
  }

  /* Validate the threshold values */
  if (drop_hi_thrd > BF_TOFINO_IBUF_SIZE ||
      drop_low_thrd > BF_TOFINO_IBUF_SIZE || drop_low_thrd > drop_hi_thrd) {
    LOG_ERROR("%s: Invalid values for high threshold %d or low threshold %d",
              __func__,
              drop_hi_thrd,
              drop_low_thrd);
    return (PIPE_INVALID_ARG);
  }

  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port_id);
    return (PIPE_INVALID_ARG);
  }

  /* Convert the thresholds to unit of 16bytes */
  drop_hi_thrd = (drop_hi_thrd >> 4);
  drop_low_thrd = (drop_low_thrd >> 4);

  sts = ibuf_set_drop_threshold(dev_info, port_id, drop_low_thrd, drop_hi_thrd);

  return (sts);
}

pipe_status_t pipe_mgr_ibuf_tof_port_set_afull_threshold(
    rmt_dev_info_t *dev_info,
    bf_dev_port_t port_id,
    uint32_t afull_hi_thrd,
    uint32_t afull_low_thrd) {
  bf_dev_id_t dev_id = dev_info->dev_id;
  rmt_port_info_t *port_info = NULL;
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  pipe_status_t sts = PIPE_SUCCESS;

  if (port >= TOF_NUM_PORTS_PER_PIPE) {
    return (PIPE_INVALID_ARG);
  }

  /* Validate the threshold values */
  if (afull_hi_thrd > BF_TOFINO_IBUF_SIZE ||
      afull_low_thrd > BF_TOFINO_IBUF_SIZE || afull_low_thrd > afull_hi_thrd) {
    LOG_ERROR("%s: Invalid values for high threshold %d or low threshold %d",
              __func__,
              afull_hi_thrd,
              afull_low_thrd);
    return (PIPE_INVALID_ARG);
  }

  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port_id);
    return (PIPE_INVALID_ARG);
  }

  /* Convert the thresholds to unit of 16bytes */
  afull_hi_thrd = (afull_hi_thrd >> 4);
  afull_low_thrd = (afull_low_thrd >> 4);

  sts = ibuf_set_almost_full_threshold(
      dev_info, port_id, afull_low_thrd, afull_hi_thrd);

  return (sts);
}

pipe_status_t pipe_mgr_ibuf_tof_set_port_speed_based_cfg(
    rmt_dev_info_t *dev_info, bf_dev_port_t port_id) {
  pipe_status_t sts = PIPE_SUCCESS;

  sts = ibuf_set_port_channel_seq(dev_info, port_id);
  sts |= ibuf_set_port_speed_based_threshold(dev_info, port_id);
  sts |= ibuf_set_bank_watermark_drop_threshold(dev_info, port_id);
  sts |= ibuf_set_bank_watermark_almostfull_threshold(dev_info, port_id);

  /* Push all the config with the channel disabled.  The caller can use
   * pipe_mgr_ibuf_enable_channel() to do the final step of enabling the
   * channel. */
  sts |= ibuf_set_chnl_ctrl(dev_info, port_id, false, 0, false);

  return sts;
}

pipe_status_t pipe_mgr_ibuf_tof_enable_channel(rmt_dev_info_t *dev_info,
                                               bf_dev_port_t port_id) {
  return ibuf_set_chnl_ctrl(dev_info, port_id, true, 0, false);
}

pipe_status_t pipe_mgr_ibuf_tof_enable_channel_all(pipe_sess_hdl_t shdl,
                                                   rmt_dev_info_t *dev_info) {
  rmt_port_info_t *port_info = dev_info->port_list;
  for (; port_info; port_info = port_info->next) {
    pipe_status_t sts =
        ibuf_set_chnl_ctrl(dev_info, port_info->port_id, true, shdl, true);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Failed to enable channel for dev %d port %d sts %s",
                dev_info->dev_id,
                port_info->port_id,
                pipe_str_err(sts));
      return sts;
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_ibuf_tof_enable_congestion_notif_to_parser(
    rmt_dev_info_t *dev_info, rmt_port_info_t *port_info) {
  pipe_status_t sts = PIPE_SUCCESS;
  bf_dev_id_t dev_id = dev_info->dev_id;

  sts = ibuf_set_chnl_ctrl(dev_info, port_info->port_id, true, 0, false);
  if (PIPE_SUCCESS != sts) {
    LOG_ERROR("Dev %d Failed to write ibuf channel control, port %d (%s)",
              dev_id,
              port_info->port_id,
              pipe_str_err(sts));
    PIPE_MGR_DBGCHK(0);
    return sts;
  }
  return ibuf_set_almost_full_threshold(dev_info,
                                        port_info->port_id,
                                        port_info->lo_wm_bytes,
                                        port_info->hi_wm_bytes);
}

pipe_status_t pipe_mgr_ibuf_tof_parb_enable_flow_control_to_mac(
    rmt_dev_info_t *dev_info,
    bf_dev_port_t port_id,
    uint16_t low_wm_bytes,
    uint16_t hi_wm_bytes) {
  bf_dev_pipe_t pipe = 0;
  int pg, ch;
  pipe_status_t sts = PIPE_SUCCESS;
  rmt_port_info_t *port_info = NULL;
  uint32_t val;
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);
  bf_dev_id_t dev_id = dev_info->dev_id;

  // enable control bit to turn on pause to mac
  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port_id);
    return PIPE_INVALID_ARG;
  }
  pipe = port_info->phy_pipe;
  port_info->pause_mac = true;
  sts = ibuf_set_chnl_ctrl(dev_info, port_id, true, 0, false);
  if (PIPE_SUCCESS != sts) {
    LOG_ERROR("Failed to add ibuf channel control to instruction list (%d)",
              sts);
    PIPE_MGR_DBGCHK(0);
    return sts;
  }

  pg = port / TOF_NUM_CHN_PER_PORT;  // Port group has 4 ports/channels
  ch = port % TOF_NUM_CHN_PER_PORT;

  val = 0;
  setp_chnl_tx_xoff_chnl_thr_hi_tx_xoff(&val, hi_wm_bytes);
  setp_chnl_tx_xoff_chnl_thr_lo_tx_xoff(&val, low_wm_bytes);

  switch (ch) {
    case 0:
      pipe_mgr_write_register(
          dev_id,
          0,
          offsetof(Tofino,
                   pipes[pipe]
                       .pmarb.ibp18_reg.ibp_reg[pg]
                       .ing_buf_regs.chan0_group.chnl_tx_xoff),
          val);
      break;
    case 1:
      pipe_mgr_write_register(
          dev_id,
          0,
          offsetof(Tofino,
                   pipes[pipe]
                       .pmarb.ibp18_reg.ibp_reg[pg]
                       .ing_buf_regs.chan1_group.chnl_tx_xoff),
          val);
      break;
    case 2:
      pipe_mgr_write_register(
          dev_id,
          0,
          offsetof(Tofino,
                   pipes[pipe]
                       .pmarb.ibp18_reg.ibp_reg[pg]
                       .ing_buf_regs.chan2_group.chnl_tx_xoff),
          val);
      break;
    case 3:
      pipe_mgr_write_register(
          dev_id,
          0,
          offsetof(Tofino,
                   pipes[pipe]
                       .pmarb.ibp18_reg.ibp_reg[pg]
                       .ing_buf_regs.chan3_group.chnl_tx_xoff),
          val);
      break;
    default:
      break;
  }
  return sts;
}

pipe_status_t pipe_mgr_ibuf_tof_disable_congestion_notif_to_parser(
    rmt_dev_info_t *dev_info, bf_dev_port_t port_id) {
  pipe_status_t sts = PIPE_SUCCESS;
  rmt_port_info_t *port_info = NULL;
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);
  bf_dev_id_t dev_id = dev_info->dev_id;

  // enable control bit to turn on notification
  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port_id);
    return PIPE_INVALID_ARG;
  }
  port_info->disable_cong_notif = true;
  sts = ibuf_set_chnl_ctrl(dev_info, port_id, true, 0, false);
  if (PIPE_SUCCESS != sts) {
    LOG_ERROR("Failed to add ibuf channel control to instruction list (%d)",
              sts);
    PIPE_MGR_DBGCHK(0);
    return sts;
  }

  return sts;
}

pipe_status_t pipe_mgr_ibuf_tof_parb_disable_flow_control_to_mac(
    rmt_dev_info_t *dev_info, bf_dev_port_t port_id) {
  pipe_status_t sts = PIPE_SUCCESS;
  rmt_port_info_t *port_info = NULL;
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  bf_dev_id_t dev_id = dev_info->dev_id;

  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);

  // enable control bit to turn on pause to mac
  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port_id);
    return PIPE_INVALID_ARG;
  }
  port_info->pause_mac = false;
  sts = ibuf_set_chnl_ctrl(dev_info, port_id, true, 0, false);
  if (PIPE_SUCCESS != sts) {
    LOG_ERROR("Failed to add ibuf channel control to instruction list (%d)",
              sts);
    PIPE_MGR_DBGCHK(0);
    return sts;
  }

  return sts;
}

pipe_status_t pipe_mgr_ibuf_tof_disable_chnl(rmt_dev_info_t *dev_info,
                                             bf_dev_port_t port_id) {
  uint32_t val = 0;
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  rmt_port_info_t *port_info = NULL;
  bf_dev_id_t dev_id = dev_info->dev_id;

  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);
  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port_id);
    return PIPE_INVALID_ARG;
  }
  LOG_TRACE("IBUF %s: pipe = %d, port = %d, port speed = %d",
            __func__,
            dev_info->dev_cfg.dev_port_to_pipe(port_id),
            port,
            port_info->speed);
  setp_chnl_ctrl_chnl_ena(&val, 0);
  /* For 25G and 10G set the channel mode value to 2 so that
     traffic on other channels is not affected.
  */
  if ((port_info->speed == BF_SPEED_100G) ||
      (port_info->speed == BF_SPEED_40G) ||
      (port_info->speed == BF_SPEED_25G) ||
      (port_info->speed == BF_SPEED_10G)) {
    setp_chnl_ctrl_chnl_mode(&val, 2);  // This helps to reset data_seq when
                                        // port transitions from 100G-->100G
                                        // or 40G-->40G
  }
  return ibuf_set_chnl_ctrl_reg(dev_info, port_info, val, 0, false);
}

pipe_status_t pipe_mgr_ibuf_tof_set_1588_timestamp_offset(
    pipe_sess_hdl_t sess_hdl, rmt_dev_info_t *dev_info) {
  uint32_t ts_1588_offset[18] = {0x5000,
                                 0x6000,
                                 0x13000,
                                 0x14000,
                                 0x4000,
                                 0x7000,
                                 0x12000,
                                 0x15000,
                                 0x3000,
                                 0x8000,
                                 0x11000,
                                 0x16000,
                                 0x2000,
                                 0x9000,
                                 0x10000,
                                 0x17000,
                                 0x1000,
                                 0};
  int i, port;
  pipe_instr_write_reg_t instr;
  pipe_status_t sts = PIPE_SUCCESS;
  bf_dev_id_t dev_id = dev_info->dev_id;
  uint32_t prsr_stage;
  lld_err_t lld_sts;
  if (LLD_OK != (lld_sts = lld_sku_get_prsr_stage(dev_id, &prsr_stage))) {
    LOG_ERROR("%s:%d Failed to get prsr stage for dev %d, sts %d",
              __func__,
              __LINE__,
              dev_id,
              lld_sts);
    return PIPE_INVALID_ARG;
  }

  pipe_bitmap_t pipe_bit_map = {{0}};
  PIPE_BITMAP_INIT(&pipe_bit_map, PIPE_BMP_SIZE);
  for (i = 0; i < (int)pipe_mgr_get_num_active_pipes(dev_id); ++i) {
    PIPE_BITMAP_SET(&pipe_bit_map, i);
  }
  /* Set timestamp on every port */
  int num_prsrs = pipe_mgr_num_prsrs(dev_id);
  for (port = 0; port < num_prsrs; ++port) {
    construct_instr_reg_write(dev_id,
                              &instr,
                              offsetof(Tofino,
                                       pipes[0]
                                           .pmarb.ibp18_reg.ibp_reg[port]
                                           .ing_buf_regs.glb_group.tim_off),
                              ts_1588_offset[port]);
    sts = pipe_mgr_drv_ilist_add(&sess_hdl,
                                 dev_info,
                                 &pipe_bit_map,
                                 prsr_stage,
                                 (uint8_t *)&instr,
                                 sizeof instr);

    if (PIPE_SUCCESS != sts) {
      LOG_ERROR(
          "Failed to add ibuf timestamp offset value to instruction list for "
          "ibuf %d, rc = (%d)",
          port,
          sts);
      PIPE_MGR_DBGCHK(0);
      return sts;
    }
  }

  return sts;
}
