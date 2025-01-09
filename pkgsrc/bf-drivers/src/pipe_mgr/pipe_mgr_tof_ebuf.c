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
 * @file pipe_mgr_tof_ebuf.c
 * @date
 *
 * Implementation of eBUF related initializations
 */

#include "pipe_mgr_int.h"
#include <tofino_regs/tofino.h>
#include "pipe_mgr_tof_ebuf.h"
#include "pipe_mgr_drv_intf.h"
#include "dvm/dvm.h"

struct pipe_mgr_tof_ebuf_pipe_reg_ctx {
  /* Shadows of the following two registers:
   *  pipes[].pmarb.ebp18_reg.ebp_reg[].epb_prsr_port_regs.multi_threading
   *  pipes[].pmarb.ebp18_reg.ebp_reg[].epb_prsr_port_regschnl_ctrl[] */
  uint32_t chan_ctrl[TOF_NUM_PARSERS][TOF_NUM_CHN_PER_PORT];
  uint32_t multi_threading[TOF_NUM_PARSERS];
};

static pipe_status_t ebuf_set_epb_prsr_port_id_reg(rmt_dev_info_t *dev_info,
                                                   bf_dev_port_t port_id) {
  bf_dev_pipe_t pipe = 0;
  int pg;
  pipe_status_t sts = PIPE_SUCCESS;
  uint32_t val = 0;
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  rmt_port_info_t *port_info = NULL;
  bf_dev_id_t dev_id = dev_info->dev_id;

  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);

  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port);
    return PIPE_INVALID_ARG;
  }
  pipe = port_info->phy_pipe;

  pg = port / TOF_NUM_CHN_PER_PORT;  // Port group has 4 ports/channels

  setp_epb_prsr_port_regs_port_id_port_id(&val, pg);
  sts = pipe_mgr_write_register(
      dev_id,
      0,
      offsetof(
          Tofino,
          pipes[pipe].pmarb.ebp18_reg.ebp_reg[pg].epb_prsr_port_regs.port_id),
      val);

  return (sts);
}

pipe_status_t ebuf_set_epb_prsr_port_chnl_ctrl_en_reg(rmt_dev_info_t *dev_info,
                                                      uint8_t logical_pipe,
                                                      bf_dev_port_t local_port,
                                                      bool enable) {
  int pg, ch;
  uint32_t *val;
  bf_dev_pipe_t phy_pipe;
  pipe_status_t sts;
  bf_dev_id_t dev_id = dev_info->dev_id;
  union pipe_mgr_ebuf_ctx *e = pipe_mgr_ebuf_ctx(dev_id);
  struct pipe_mgr_tof_ebuf_ctx *ctx = e ? &e->tof : NULL;
  if (!ctx) return PIPE_INVALID_ARG;
  pg = local_port / TOF_NUM_CHN_PER_PORT;  // Port group has 4 ports/channels
  ch = local_port % TOF_NUM_CHN_PER_PORT;
  val = &ctx->reg[logical_pipe].chan_ctrl[pg][ch];
  setp_epb_prsr_port_regs_chnl_ctrl_chnl_ena(val, enable ? 1 : 0);
  sts = pipe_mgr_map_pipe_id_log_to_phy(dev_info, logical_pipe, &phy_pipe);
  if (sts != PIPE_SUCCESS) return sts;
  sts = pipe_mgr_write_register(dev_id,
                                0,
                                offsetof(Tofino,
                                         pipes[phy_pipe]
                                             .pmarb.ebp18_reg.ebp_reg[pg]
                                             .epb_prsr_port_regs.chnl_ctrl[ch]),
                                *val);
  return sts;
}

static pipe_status_t ebuf_set_epb_prsr_port_chnl_ctrl_reg(
    rmt_dev_info_t *dev_info, bf_dev_port_t port_id) {
  int pg, ch;
  pipe_status_t sts = PIPE_SUCCESS;
  uint32_t *val;
  bf_dev_pipe_t pipe = dev_info->dev_cfg.dev_port_to_pipe(port_id);
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  rmt_port_info_t *port_info = NULL;
  uint32_t pipe_ovr = dev_info->dev_cfg.dev_port_to_pipe(port_id) | 0x4;
  bf_dev_id_t dev_id = dev_info->dev_id;

  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);

  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port);
    return PIPE_INVALID_ARG;
  }
  int phy_pipe = port_info->phy_pipe;

  pg = port / TOF_NUM_CHN_PER_PORT;  // Port group has 4 ports/channels
  ch = port % TOF_NUM_CHN_PER_PORT;

  // Values in this register are updated by compiler too. Hence read-modify
  union pipe_mgr_ebuf_ctx *e = pipe_mgr_ebuf_ctx(dev_id);
  struct pipe_mgr_tof_ebuf_ctx *ctx = e ? &e->tof : NULL;
  if (!ctx) return PIPE_INVALID_ARG;
  val = &ctx->reg[pipe].chan_ctrl[pg][ch];

  setp_epb_prsr_port_regs_chnl_ctrl_chnl_ena(val, 1);

  /*
   * Afull_thr and atem_thr are applicable for rev A0 only.
   * For rev B0 and later parts, these are deprecated.
   */
  if (dev_info->part_rev == BF_SKU_CHIP_PART_REV_A0) {
    switch (port_info->speed) {
      case BF_SPEED_50G:
        setp_epb_prsr_port_regs_chnl_ctrl_afull_thr(val, 3);
        setp_epb_prsr_port_regs_chnl_ctrl_aemp_thr(val, 1);
        break;
      case BF_SPEED_1G:
      case BF_SPEED_10G:
      case BF_SPEED_25G:
        setp_epb_prsr_port_regs_chnl_ctrl_afull_thr(val, 4);
        setp_epb_prsr_port_regs_chnl_ctrl_aemp_thr(val, 2);
        break;
      case BF_SPEED_40G:
      case BF_SPEED_100G:
        setp_epb_prsr_port_regs_chnl_ctrl_afull_thr(val, 2);
        setp_epb_prsr_port_regs_chnl_ctrl_aemp_thr(val, 2);
        break;
      default:
        break;
    }
  }
  /* Set physical to logical pipe mapping */
  setp_epb_prsr_port_regs_chnl_ctrl_pipeid_ovr(val, pipe_ovr);

  /* If are are applying the TF1 port mode transition work around turn off all
   * egress metadata insertion to allow for small packets to be generated. */
  uint32_t chnl_ctrl_val = *val;
  if (bf_drv_check_port_mode_transition_wa(dev_id, port_id, port_info->speed)) {
    setp_epb_prsr_port_regs_chnl_ctrl_meta_opt(&chnl_ctrl_val, 0);
  }

  sts = pipe_mgr_write_register(dev_id,
                                0,
                                offsetof(Tofino,
                                         pipes[phy_pipe]
                                             .pmarb.ebp18_reg.ebp_reg[pg]
                                             .epb_prsr_port_regs.chnl_ctrl[ch]),
                                chnl_ctrl_val);

  // Disable other channels in case of 50G and 100G
  switch (port_info->speed) {
    case BF_SPEED_50G:
      if ((ch != 1) && (ch != 3)) {
        val = &ctx->reg[pipe].chan_ctrl[pg][ch + 1];
        setp_epb_prsr_port_regs_chnl_ctrl_chnl_ena(val, 0);
        sts |= pipe_mgr_write_register(
            dev_id,
            0,
            offsetof(Tofino,
                     pipes[phy_pipe]
                         .pmarb.ebp18_reg.ebp_reg[pg]
                         .epb_prsr_port_regs.chnl_ctrl[ch + 1]),
            *val);
      }
      break;
    case BF_SPEED_100G:
    case BF_SPEED_40G:
      for (ch = 1; ch < TOF_NUM_CHN_PER_PORT; ch++) {
        val = &ctx->reg[pipe].chan_ctrl[pg][ch];
        setp_epb_prsr_port_regs_chnl_ctrl_chnl_ena(val, 0);
        sts |= pipe_mgr_write_register(
            dev_id,
            0,
            offsetof(Tofino,
                     pipes[phy_pipe]
                         .pmarb.ebp18_reg.ebp_reg[pg]
                         .epb_prsr_port_regs.chnl_ctrl[ch]),
            *val);
      }
      break;
    default:
      break;
  }

  return (sts);
}

static pipe_status_t ebuf_set_epb_prsr_port_chnl_ctrl2_reg(
    rmt_dev_info_t *dev_info, bf_dev_port_t port_id) {
  bf_dev_pipe_t pipe = 0;
  int pg, ch;
  pipe_status_t sts = PIPE_SUCCESS;
  uint32_t val = 0;
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  rmt_port_info_t *port_info = NULL;
  bf_dev_id_t dev_id = dev_info->dev_id;
  bf_dev_port_t first_port_in_pg;
  rmt_port_info_t *first_port_info = NULL;

  // This function implements B0 and future revision specific initialization.
  if (dev_info->part_rev == BF_SKU_CHIP_PART_REV_A0) {
    return (sts);
  }

  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);

  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port_id);
    return PIPE_INVALID_ARG;
  }
  pipe = port_info->phy_pipe;

  pg = port / TOF_NUM_CHN_PER_PORT;  // Port group has 4 ports/channels
  ch = port % TOF_NUM_CHN_PER_PORT;

  switch (port_info->speed) {
    case BF_SPEED_50G:
      setp_epb_prsr_port_regs_chnl_ctrl2_chnl_mode(&val, 1);
      setp_epb_prsr_port_regs_chnl_ctrl2_stall_thr(&val, 1);
      break;
    case BF_SPEED_1G:
    case BF_SPEED_10G:
    case BF_SPEED_25G:
      setp_epb_prsr_port_regs_chnl_ctrl2_chnl_mode(&val, 2);
      setp_epb_prsr_port_regs_chnl_ctrl2_stall_thr(&val, 2);
      break;
    case BF_SPEED_40G:
    case BF_SPEED_100G:
      setp_epb_prsr_port_regs_chnl_ctrl2_chnl_mode(&val, 0);
      setp_epb_prsr_port_regs_chnl_ctrl2_stall_thr(&val, 0);
      break;
    default:
      break;
  }

  sts =
      pipe_mgr_write_register(dev_id,
                              0,
                              offsetof(Tofino,
                                       pipes[pipe]
                                           .pmarb.ebp18_reg.ebp_reg[pg]
                                           .epb_prsr_port_regs.chnl_ctrl2[ch]),
                              val);

  /* NOTE:
   *   In case port for channel0 in this port group
   *   is not added but other upper channels' (ch1-ch3) ports get added,
   *   then channel mode for channel0 should be set to non-zero value.
   *   Since 0 is the POR value and it also means that the channel is in 100G
   *   or 40G mode, this would cause issues for added upper channel ports
   *   though channel0 is not enabled.
   */
  if (ch == 0) {
    /* Channel 0 gets added, return */
    return (sts);
  }

  first_port_in_pg = port_id & 0xFFFFFFFC;
  first_port_info = pipe_mgr_get_port_info(dev_id, first_port_in_pg);
  if (first_port_info) {
    /* Channel 0 port is already added, return */
    return (sts);
  }

  val = 0;
  setp_epb_prsr_port_regs_chnl_ctrl2_chnl_mode(&val, 2);
  sts |=
      pipe_mgr_write_register(dev_id,
                              0,
                              offsetof(Tofino,
                                       pipes[pipe]
                                           .pmarb.ebp18_reg.ebp_reg[pg]
                                           .epb_prsr_port_regs.chnl_ctrl2[0]),
                              val);

  return (sts);
}

static pipe_status_t ebuf_set_epb_prsr_port_threading(rmt_dev_info_t *dev_info,
                                                      bf_dev_port_t port_id) {
  int pg;
  pipe_status_t sts = PIPE_SUCCESS;
  uint32_t *val;
  rmt_port_info_t *port_info = NULL;
  bf_dev_pipe_t pipe = dev_info->dev_cfg.dev_port_to_pipe(port_id);
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  bf_dev_id_t dev_id = dev_info->dev_id;
  uint32_t mac_blk, channel;
  bool enabled = false;

  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);

  pg = port / TOF_NUM_CHN_PER_PORT;  // Port group has 4 ports/channels

  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port);
    return PIPE_INVALID_ARG;
  }
  int phy_pipe = port_info->phy_pipe;

  union pipe_mgr_ebuf_ctx *e = pipe_mgr_ebuf_ctx(dev_id);
  struct pipe_mgr_tof_ebuf_ctx *ctx = e ? &e->tof : NULL;
  if (!ctx) return PIPE_INVALID_ARG;
  val = &ctx->reg[pipe].multi_threading[pg];
  prsr_multi_threading_mode_t multi_threading_mode =
      PRSR_MULTI_THREADING_MODE_DEFAULT;
  profile_id_t prof_id = 0;

  switch (port_info->speed) {
    case BF_SPEED_100G:
    case BF_SPEED_40G:
      // Check if port mode transition workaround needs to be applied
      if (bf_drv_check_port_mode_transition_wa(
              dev_id, port_id, port_info->speed)) {
        setp_epb_prsr_port_regs_threading_ctrl_mult_thrd(val, 0);
        setp_epb_prsr_port_regs_threading_ctrl_sngl_thrd(val, 0);
      } else {
        setp_epb_prsr_port_regs_threading_ctrl_mult_thrd(
            val, 0);  // Transition mult_thrd 1->0-1
        setp_epb_prsr_port_regs_threading_ctrl_sngl_thrd(
            val, 0);  // to handle 100G-->100G case.
        pipe_mgr_write_register(
            dev_id,
            0,
            offsetof(Tofino,
                     pipes[phy_pipe]
                         .pmarb.ebp18_reg.ebp_reg[pg]
                         .epb_prsr_port_regs.multi_threading),
            *val);

        lld_err_t lld_err = lld_sku_map_dev_port_id_to_mac_ch(
            dev_id, port_id, &mac_blk, &channel);

        bf_status_t bf_sts = bf_recirculation_get(dev_id, port_id, &enabled);
        if (bf_sts != BF_SUCCESS) {
          LOG_ERROR("%s: Dev %d port %d cannot get recirculation state, %s",
                    __func__,
                    dev_id,
                    port_id,
                    bf_err_str(bf_sts));
          return PIPE_INTERNAL_ERROR;
        }

        if (lld_err == LLD_OK || enabled) {
          /* Port is a 100G MAC Port or a recirc port - Enable multi-threading
           * Dont enable multi-threading for the PCIe CPU port
           */

          if (pipe_mgr_pipe_to_profile(
                  dev_info, pipe, &prof_id, __func__, __LINE__) ==
              PIPE_SUCCESS) {
            multi_threading_mode =
                dev_info->profile_info[prof_id]
                    ->driver_options.prsr_multi_threading_enable;
          }

          setp_epb_prsr_port_regs_threading_ctrl_mult_thrd(val, 1);
          setp_epb_prsr_port_regs_threading_ctrl_sngl_thrd(val, 0);
        }
      }
      break;
    case BF_SPEED_50G:
    case BF_SPEED_1G:
    case BF_SPEED_10G:
    case BF_SPEED_25G:
      setp_epb_prsr_port_regs_threading_ctrl_mult_thrd(val, 0);
      setp_epb_prsr_port_regs_threading_ctrl_sngl_thrd(val, 0);
    default:
      break;
  }

  sts =
      pipe_mgr_write_register(dev_id,
                              0,
                              offsetof(Tofino,
                                       pipes[phy_pipe]
                                           .pmarb.ebp18_reg.ebp_reg[pg]
                                           .epb_prsr_port_regs.multi_threading),
                              *val);

  if (multi_threading_mode == PRSR_MULTI_THREADING_MODE_DISABLE) {
    setp_epb_prsr_port_regs_threading_ctrl_mult_thrd(val, 0);
    setp_epb_prsr_port_regs_threading_ctrl_sngl_thrd(val, 0);

    sts |= pipe_mgr_write_register(
        dev_id,
        0,
        offsetof(Tofino,
                 pipes[phy_pipe]
                     .pmarb.ebp18_reg.ebp_reg[pg]
                     .epb_prsr_port_regs.multi_threading),
        *val);
  }

  return (sts);
}

static pipe_status_t ebuf_set_epb_complete_port_mode_transition_wa(
    rmt_dev_info_t *dev_info, bf_dev_port_t port_id) {
  int pg, ch;
  uint32_t *val;
  rmt_port_info_t *port_info = NULL;
  bf_dev_pipe_t pipe = dev_info->dev_cfg.dev_port_to_pipe(port_id);
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  bf_dev_id_t dev_id = dev_info->dev_id;

  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);

  pg = port / TOF_NUM_CHN_PER_PORT;  // Port group has 4 ports/channels
  ch = port % TOF_NUM_CHN_PER_PORT;

  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port);
    return PIPE_INVALID_ARG;
  }
  int phy_pipe = port_info->phy_pipe;

  if (!(port_info->speed == BF_SPEED_100G ||
        port_info->speed == BF_SPEED_40G)) {
    LOG_ERROR(
        "%s: port (%d) speed %d is invalid", __func__, port, port_info->speed);
    return PIPE_INVALID_ARG;
  }

  union pipe_mgr_ebuf_ctx *e = pipe_mgr_ebuf_ctx(dev_id);
  struct pipe_mgr_tof_ebuf_ctx *ctx = e ? &e->tof : NULL;
  if (!ctx) return PIPE_INVALID_ARG;
  val = &ctx->reg[pipe].multi_threading[pg];

  // Transition mult_thrd 1->0-1
  setp_epb_prsr_port_regs_threading_ctrl_mult_thrd(val, 0);
  // to handle 100G-->100G case.
  setp_epb_prsr_port_regs_threading_ctrl_sngl_thrd(val, 0);
  pipe_mgr_write_register(dev_id,
                          0,
                          offsetof(Tofino,
                                   pipes[phy_pipe]
                                       .pmarb.ebp18_reg.ebp_reg[pg]
                                       .epb_prsr_port_regs.multi_threading),
                          *val);

  setp_epb_prsr_port_regs_threading_ctrl_mult_thrd(val, 1);
  setp_epb_prsr_port_regs_threading_ctrl_sngl_thrd(val, 0);

  pipe_mgr_write_register(dev_id,
                          0,
                          offsetof(Tofino,
                                   pipes[phy_pipe]
                                       .pmarb.ebp18_reg.ebp_reg[pg]
                                       .epb_prsr_port_regs.multi_threading),
                          *val);

  profile_id_t prof_id = 0;
  prsr_multi_threading_mode_t multi_threading_mode =
      PRSR_MULTI_THREADING_MODE_DEFAULT;

  if (pipe_mgr_pipe_to_profile(dev_info, pipe, &prof_id, __func__, __LINE__) ==
      PIPE_SUCCESS) {
    multi_threading_mode = dev_info->profile_info[prof_id]
                               ->driver_options.prsr_multi_threading_enable;
  }

  if (multi_threading_mode == PRSR_MULTI_THREADING_MODE_DISABLE) {
    setp_epb_prsr_port_regs_threading_ctrl_mult_thrd(val, 0);
    setp_epb_prsr_port_regs_threading_ctrl_sngl_thrd(val, 0);

    pipe_mgr_write_register(dev_id,
                            0,
                            offsetof(Tofino,
                                     pipes[phy_pipe]
                                         .pmarb.ebp18_reg.ebp_reg[pg]
                                         .epb_prsr_port_regs.multi_threading),
                            *val);
  }

  /* Reset the meta_opt field to re-enable egress metadata insertion. */
  uint32_t chnl_ctrl = ctx->reg[pipe].chan_ctrl[pg][ch];
  pipe_mgr_write_register(dev_id,
                          0,
                          offsetof(Tofino,
                                   pipes[phy_pipe]
                                       .pmarb.ebp18_reg.ebp_reg[pg]
                                       .epb_prsr_port_regs.chnl_ctrl[ch]),
                          chnl_ctrl);

  return PIPE_SUCCESS;
}

static uint32_t get_channel_seq(bf_dev_id_t dev_id, bf_dev_port_t port_id) {
  rmt_port_info_t *port_info = NULL;
  bf_dev_port_t first_port_in_pg;
  uint8_t port_speed[4], i;
  uint32_t chan_seq = 0;
  rmt_port_info_t *all_port_info = NULL;

  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port_id);
    return PIPE_INVALID_ARG;
  }

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
      // 4x25G, 4x10G, 4x1G case
      chan_seq = 0xD8;
    }
  }

  return (chan_seq);
}

/*
 * Return the port mode port id for EBUF disp register programming.
 *   - If port speed is 100G, return 1
 *   - For other port speeds/config, return 5
 */
static uint32_t get_ebuf_disp_regs_port_id(bf_dev_id_t dev_id,
                                           bf_dev_port_t port_id) {
  rmt_port_info_t *port_info = NULL;
  uint32_t port_mode_port_id = 5;  // Default value is 5

  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port_id);
    return (port_mode_port_id);
  }

  if (port_info->speed == BF_SPEED_100G || port_info->speed == BF_SPEED_40G) {
    port_mode_port_id = 1;
  }

  return (port_mode_port_id);
}

/*
 * Return the port mode port id for EBUF programming.
 *   - If any 10G port is configured for any of the channels in the MAC,
 *      return 0.
 *   - For all other port configs on the MAC, return 1.
 */
static uint32_t get_ebuf_port_id(bf_dev_id_t dev_id, bf_dev_port_t port_id) {
  rmt_port_info_t *port_info = NULL;
  bf_dev_port_t first_port_in_pg;
  uint8_t port_speed[TOF_NUM_CHN_PER_PORT], i;
  uint32_t port_mode_port_id = 1;  // Default value is 1
  rmt_port_info_t *all_port_info = NULL;

  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port_id);
    return (port_mode_port_id);
  }

  if ((port_info->speed != BF_SPEED_100G) &&
      (port_info->speed != BF_SPEED_40G)) {
    /* Breakout config, check if any port is in 10G */
    first_port_in_pg = port_id & 0xFFFFFFFC;
    for (i = 0; i < TOF_NUM_CHN_PER_PORT; i++) {
      all_port_info = pipe_mgr_get_port_info(dev_id, first_port_in_pg + i);
      if (!all_port_info) {
        port_speed[i] = BF_SPEED_NONE;  // Unpopulated yet
      } else {
        port_speed[i] = all_port_info->speed;
      }
    }

    /* Set port_mode_port_id to 0 if any port is in 10G */
    if ((port_speed[0] == BF_SPEED_10G) || (port_speed[1] == BF_SPEED_10G) ||
        (port_speed[2] == BF_SPEED_10G) || (port_speed[3] == BF_SPEED_10G)) {
      port_mode_port_id = 0;
    }
  }

  return (port_mode_port_id);
}

static pipe_status_t ebuf_set_epb_disp_chnl_ctrl_reg(rmt_dev_info_t *dev_info,
                                                     bf_dev_port_t port_id) {
  bf_dev_pipe_t pipe = 0;
  int pg, ch;
  pipe_status_t sts = PIPE_SUCCESS;
  rmt_port_info_t *port_info = NULL;
  uint32_t chnl_ctrl = 0, reset_tm_cred = 0, port_mode = 0, chan_seq = 0;
  uint32_t port_mode_port_id;
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  bool is_100g_mode = false;
  bf_dev_id_t dev_id = dev_info->dev_id;

  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);

  pg = port / TOF_NUM_CHN_PER_PORT;  // Port group has 4 ports/channels
  ch = port % TOF_NUM_CHN_PER_PORT;

  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port);
    return PIPE_INVALID_ARG;
  }
  pipe = port_info->phy_pipe;

  /* Prepare channel control register in ebuf */
  switch (port_info->speed) {
    case BF_SPEED_50G:
      setp_epb_disp_port_regs_chnl_ctrl_chnl_mode(&chnl_ctrl, 1);
      break;
      break;
    case BF_SPEED_100G:
    case BF_SPEED_40G:
      setp_epb_disp_port_regs_chnl_ctrl_chnl_mode(&chnl_ctrl, 0);
      is_100g_mode = true;
      break;
    case BF_SPEED_1G:
    case BF_SPEED_10G:
    case BF_SPEED_25G:
      setp_epb_disp_port_regs_chnl_ctrl_chnl_mode(&chnl_ctrl, 2);
    default:
      break;
  }

  chan_seq = get_channel_seq(dev_id, port_id);
  setp_epb_disp_port_regs_port_mode_port_seq(&port_mode, chan_seq);

  // Step1: Assert TM credit reset signal and disable channel
  setp_epb_disp_port_regs_chnl_ctrl_reset_cred(&reset_tm_cred, 1);
  sts = pipe_mgr_write_register(dev_id,
                                0,
                                offsetof(Tofino,
                                         pipes[pipe]
                                             .pmarb.ebp18_reg.egrNx_reg[pg]
                                             .epb_disp_port_regs.chnl_ctrl[ch]),
                                reset_tm_cred);
  if (is_100g_mode) {
    // For 100G port, enable both channel 0 and 2
    sts |=
        pipe_mgr_write_register(dev_id,
                                0,
                                offsetof(Tofino,
                                         pipes[pipe]
                                             .pmarb.ebp18_reg.egrNx_reg[pg]
                                             .epb_disp_port_regs.chnl_ctrl[2]),
                                reset_tm_cred);
  }

  // Step2: Deassert TM credit reset signal and enable channel
  setp_epb_disp_port_regs_chnl_ctrl_chnl_ena(&chnl_ctrl, 1);
  setp_epb_disp_port_regs_chnl_ctrl_reset_cred(&chnl_ctrl, 0);
  sts |=
      pipe_mgr_write_register(dev_id,
                              0,
                              offsetof(Tofino,
                                       pipes[pipe]
                                           .pmarb.ebp18_reg.egrNx_reg[pg]
                                           .epb_disp_port_regs.chnl_ctrl[ch]),
                              chnl_ctrl);
  if (is_100g_mode) {
    // For 100G port, enable both channel 0 and 2
    sts |=
        pipe_mgr_write_register(dev_id,
                                0,
                                offsetof(Tofino,
                                         pipes[pipe]
                                             .pmarb.ebp18_reg.egrNx_reg[pg]
                                             .epb_disp_port_regs.chnl_ctrl[2]),
                                chnl_ctrl);
  }

  /*
   *For rev B0 and later parts, get the port id based on MAC channels
   * config in the PG. For A0, use PG (MAC) as the port id.
   */
  if (dev_info->part_rev != BF_SKU_CHIP_PART_REV_A0) {
    port_mode_port_id = get_ebuf_port_id(dev_id, port_id);
  } else {
    port_mode_port_id = pg;
  }
  /* Set port mode register in ebuf */
  setp_epb_disp_port_regs_port_mode_port_id(&port_mode, port_mode_port_id);
  sts |= pipe_mgr_write_register(dev_id,
                                 0,
                                 offsetof(Tofino,
                                          pipes[pipe]
                                              .pmarb.ebp18_reg.egrNx_reg[pg]
                                              .epb_disp_port_regs.port_mode),
                                 port_mode);

  // Disable other channels in case of 50G and 100G
  switch (port_info->speed) {
    case BF_SPEED_50G:
      if ((ch != 1) && (ch != 3)) {
        sts |= pipe_mgr_write_register(
            dev_id,
            0,
            offsetof(Tofino,
                     pipes[pipe]
                         .pmarb.ebp18_reg.egrNx_reg[pg]
                         .epb_disp_port_regs.chnl_ctrl[ch + 1]),
            0);
      }
      break;
    case BF_SPEED_100G:
    case BF_SPEED_40G:
      for (ch = 1; ch < TOF_NUM_CHN_PER_PORT; ch++) {
        sts |= pipe_mgr_write_register(
            dev_id,
            0,
            offsetof(Tofino,
                     pipes[pipe]
                         .pmarb.ebp18_reg.egrNx_reg[pg]
                         .epb_disp_port_regs.chnl_ctrl[ch]),
            0);
      }
      break;
    default:
      break;
  }
  return (sts);
}

static pipe_status_t ebuf_set_ebuf_disp_port_seq_reg(rmt_dev_info_t *dev_info,
                                                     bf_dev_port_t port_id) {
  bf_dev_pipe_t pipe = 0;
  int pg;
  pipe_status_t sts = PIPE_SUCCESS;
  rmt_port_info_t *port_info = NULL;
  uint32_t port_mode = 0, chan_seq = 0;
  uint32_t port_mode_port_id;
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  bf_dev_id_t dev_id = dev_info->dev_id;

  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);

  pg = port / TOF_NUM_CHN_PER_PORT;  // Port group has 4 ports/channels

  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port);
    return PIPE_INVALID_ARG;
  }
  pipe = port_info->phy_pipe;

  chan_seq = get_channel_seq(dev_id, port_id);
  setp_ebuf_disp_regs_port_mode_port_seq(&port_mode, chan_seq);

  /*
   *For rev B0 and later parts, get the port id based on MAC channels
   * config in the PG. For A0, use PG (MAC) as the port id.
   */
  if (dev_info->part_rev != BF_SKU_CHIP_PART_REV_A0) {
    port_mode_port_id = get_ebuf_disp_regs_port_id(dev_id, port_id);
  } else {
    port_mode_port_id = pg;
  }

  setp_ebuf_disp_regs_port_mode_port_id(&port_mode, port_mode_port_id);

  sts = pipe_mgr_write_register(
      dev_id,
      0,
      offsetof(
          Tofino,
          pipes[pipe].pmarb.ebp18_reg.egrNx_reg[pg].ebuf_disp_regs.port_mode),
      port_mode);
  return (sts);
}

static pipe_status_t ebuf_set_ebuf_disp_port_chk_start_eop(
    rmt_dev_info_t *dev_info, bf_dev_port_t port_id) {
  bf_dev_pipe_t pipe = 0;
  int pg;
  pipe_status_t sts = PIPE_SUCCESS;
  rmt_port_info_t *port_info = NULL;
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  uint32_t val;
  bf_dev_id_t dev_id = dev_info->dev_id;

  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);

  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port);
    return PIPE_INVALID_ARG;
  }

  pipe = port_info->phy_pipe;
  pg = port / TOF_NUM_CHN_PER_PORT;

  val = 0x81060c14;  // Default credits for all speeds.

  switch (port_info->speed) {
    case BF_SPEED_50G:
      setp_epb_disp_port_regs_cred_ctrl_chk_start(&val, 1);
      setp_epb_disp_port_regs_cred_ctrl_chk_eop_dis(&val, 1);
      break;
    case BF_SPEED_100G:
    case BF_SPEED_40G:
      setp_epb_disp_port_regs_cred_ctrl_chk_start(&val, 1);
      setp_epb_disp_port_regs_cred_ctrl_chk_eop_dis(&val, 1);
      break;
    case BF_SPEED_25G:
      setp_epb_disp_port_regs_cred_ctrl_chk_start(&val, 1);
      setp_epb_disp_port_regs_cred_ctrl_chk_eop_dis(&val, 1);
      break;
    case BF_SPEED_1G:
    case BF_SPEED_10G:
      setp_epb_disp_port_regs_cred_ctrl_chk_start(&val, 1);
      setp_epb_disp_port_regs_cred_ctrl_chk_eop_dis(&val, 1);
    default:
      break;
  }

  sts = pipe_mgr_write_register(dev_id,
                                0,
                                offsetof(Tofino,
                                         pipes[pipe]
                                             .pmarb.ebp18_reg.egrNx_reg[pg]
                                             .epb_disp_port_regs.cred_ctrl),
                                val);
  return (sts);
}

static pipe_status_t ebuf_set_epb_drpr_chnl_ctrl_reg(rmt_dev_info_t *dev_info,
                                                     bf_dev_port_t port_id) {
  bf_dev_pipe_t pipe = 0;
  int pg, ch;
  pipe_status_t sts = PIPE_SUCCESS;
  rmt_port_info_t *port_info = NULL;
  uint32_t val = 0;
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  bf_dev_id_t dev_id = dev_info->dev_id;

  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);

  pg = port / TOF_NUM_CHN_PER_PORT;  // Port group has 4 ports/channels
  ch = port % TOF_NUM_CHN_PER_PORT;

  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port);
    return PIPE_INVALID_ARG;
  }
  pipe = port_info->phy_pipe;

  switch (port_info->speed) {
    case BF_SPEED_50G:
      setp_epb_dprs_regs_chnl_ctrl_chnl_mode(&val, 1);
      break;
    case BF_SPEED_100G:
    case BF_SPEED_40G:
      setp_epb_dprs_regs_chnl_ctrl_chnl_mode(&val, 0);
      break;
    case BF_SPEED_1G:
    case BF_SPEED_10G:
    case BF_SPEED_25G:
      setp_epb_dprs_regs_chnl_ctrl_chnl_mode(&val, 2);
    default:
      break;
  }

  setp_epb_dprs_regs_chnl_ctrl_chnl_ena(&val, 1);
  setp_epb_dprs_regs_chnl_ctrl_crc32_chk_dis(&val, 1);

  sts = pipe_mgr_write_register(dev_id,
                                0,
                                offsetof(Tofino,
                                         pipes[pipe]
                                             .pmarb.ebp18_reg.egrNx_reg[pg]
                                             .epb_dprs_regs.chnl_ctrl[ch]),
                                val);

  val = 0;
  setp_epb_dprs_regs_port_id_port_id(&val, pg);
  sts |= pipe_mgr_write_register(
      dev_id,
      0,
      offsetof(Tofino,
               pipes[pipe].pmarb.ebp18_reg.egrNx_reg[pg].epb_dprs_regs.port_id),
      val);
  // Disable other channels in case of 50G and 100G
  switch (port_info->speed) {
    case BF_SPEED_50G:
      if ((ch != 1) && (ch != 3)) {
        sts = pipe_mgr_write_register(
            dev_id,
            0,
            offsetof(Tofino,
                     pipes[pipe]
                         .pmarb.ebp18_reg.egrNx_reg[pg]
                         .epb_dprs_regs.chnl_ctrl[ch + 1]),
            0);
      }
      break;
    case BF_SPEED_100G:
    case BF_SPEED_40G:
      for (ch = 1; ch < TOF_NUM_CHN_PER_PORT; ch++) {
        sts =
            pipe_mgr_write_register(dev_id,
                                    0,
                                    offsetof(Tofino,
                                             pipes[pipe]
                                                 .pmarb.ebp18_reg.egrNx_reg[pg]
                                                 .epb_dprs_regs.chnl_ctrl[ch]),
                                    0);
      }
      break;
    default:
      break;
  }
  return (sts);
}

static pipe_status_t ebuf_set_ebuf_fifo_chnl_ctrl_reg(rmt_dev_info_t *dev_info,
                                                      bf_dev_port_t port_id) {
  bf_dev_pipe_t pipe = 0;
  int pg, ch;
  pipe_status_t sts = PIPE_SUCCESS;
  rmt_port_info_t *port_info = NULL;
  uint32_t val = 0;
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  bf_dev_id_t dev_id = dev_info->dev_id;

  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);

  pg = port / TOF_NUM_CHN_PER_PORT;  // Port group has 4 ports/channels
  ch = port % TOF_NUM_CHN_PER_PORT;

  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port);
    return PIPE_INVALID_ARG;
  }
  pipe = port_info->phy_pipe;

  switch (port_info->speed) {
    case BF_SPEED_50G:
      setp_ebuf_fifo_regs_dprs_ctrl_chnl_mode(&val, 1);
      setp_ebuf_fifo_regs_dprs_ctrl_cred_thr(&val, 3);
      break;
    case BF_SPEED_100G:
    case BF_SPEED_40G:
      setp_ebuf_fifo_regs_dprs_ctrl_chnl_mode(&val, 0);
      setp_ebuf_fifo_regs_dprs_ctrl_cred_thr(&val, 5);
      break;
    case BF_SPEED_1G:
    case BF_SPEED_10G:
    case BF_SPEED_25G:
      setp_ebuf_fifo_regs_dprs_ctrl_chnl_mode(&val, 2);
      setp_ebuf_fifo_regs_dprs_ctrl_cred_thr(&val, 2);
    default:
      break;
  }

  setp_ebuf_fifo_regs_dprs_ctrl_chnl_ena(&val, 1);
  sts = pipe_mgr_write_register(dev_id,
                                0,
                                offsetof(Tofino,
                                         pipes[pipe]
                                             .pmarb.ebp18_reg.egrNx_reg[pg]
                                             .ebuf_fifo_regs.chnl_ctrl[ch]),
                                val);

  // Disable other channels in case of 50G and 100G
  switch (port_info->speed) {
    case BF_SPEED_50G:
      if ((ch != 1) && (ch != 3)) {
        sts = pipe_mgr_write_register(
            dev_id,
            0,
            offsetof(Tofino,
                     pipes[pipe]
                         .pmarb.ebp18_reg.egrNx_reg[pg]
                         .ebuf_fifo_regs.chnl_ctrl[ch + 1]),
            0);
      }
      break;
    case BF_SPEED_100G:
    case BF_SPEED_40G:
      for (ch = 1; ch < TOF_NUM_CHN_PER_PORT; ch++) {
        sts =
            pipe_mgr_write_register(dev_id,
                                    0,
                                    offsetof(Tofino,
                                             pipes[pipe]
                                                 .pmarb.ebp18_reg.egrNx_reg[pg]
                                                 .ebuf_fifo_regs.chnl_ctrl[ch]),
                                    0);
      }
      break;
    default:
      break;
  }
  return (sts);
}

static pipe_status_t ebuf_set_ebuf_disp_chnl_ctrl_reg(
    rmt_dev_info_t *dev_info, bf_dev_port_t port_id, bool cut_through_enabled) {
  bf_dev_pipe_t pipe = 0;
  int pg, ch;
  pipe_status_t sts = PIPE_SUCCESS;
  rmt_port_info_t *port_info = NULL;
  uint32_t val = 0, reset_dprs_cred = 0;
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  uint32_t port_mode = 0, chan_seq = 0;
  uint32_t port_mode_port_id;
  bool is_100g_mode = false;
  bf_dev_id_t dev_id = dev_info->dev_id;

  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);

  pg = port / TOF_NUM_CHN_PER_PORT;  // Port group has 4 ports/channels
  ch = port % TOF_NUM_CHN_PER_PORT;

  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port);
    return PIPE_INVALID_ARG;
  }
  pipe = port_info->phy_pipe;

  switch (port_info->speed) {
    case BF_SPEED_50G:
      setp_ebuf_disp_regs_dprs_ctrl_chnl_mode(&val, 1);
      break;
    case BF_SPEED_100G:
    case BF_SPEED_40G:
      setp_ebuf_disp_regs_dprs_ctrl_chnl_mode(&val, 0);
      is_100g_mode = true;
      break;
    case BF_SPEED_25G:
      /*
       * For B0 and later parts, set channel mode to 3 if
       * cut-through is enabled. For all other cases (A0 and
       * cut-through disabled for B0 & later parts, set channnel
       * mode to 2.
       */
      if (dev_info->part_rev != BF_SKU_CHIP_PART_REV_A0 &&
          cut_through_enabled) {
        setp_ebuf_disp_regs_dprs_ctrl_chnl_mode(&val, 3);
      } else {
        setp_ebuf_disp_regs_dprs_ctrl_chnl_mode(&val, 2);
      }
      break;
    case BF_SPEED_10G:
      /* For rev B0 and later parts, set chnl mode to 3 for 10G */
      if (dev_info->part_rev != BF_SKU_CHIP_PART_REV_A0) {
        setp_ebuf_disp_regs_dprs_ctrl_chnl_mode(&val, 3);
      } else {
        setp_ebuf_disp_regs_dprs_ctrl_chnl_mode(&val, 2);
      }
      break;
    case BF_SPEED_1G:
      setp_ebuf_disp_regs_dprs_ctrl_chnl_mode(&val, 2);
    default:
      break;
  }

  chan_seq = get_channel_seq(dev_id, port_id);
  setp_epb_disp_port_regs_port_mode_port_seq(&port_mode, chan_seq);

  // Step1: Assert Deparser credit reset signal and disable channel
  setp_ebuf_disp_regs_dprs_ctrl_reset_cred(&reset_dprs_cred, 1);
  setp_ebuf_disp_regs_dprs_ctrl_chnl_ena(&reset_dprs_cred, 0);
  sts = pipe_mgr_write_register(dev_id,
                                0,
                                offsetof(Tofino,
                                         pipes[pipe]
                                             .pmarb.ebp18_reg.egrNx_reg[pg]
                                             .ebuf_disp_regs.chnl_ctrl[ch]),
                                reset_dprs_cred);
  if (is_100g_mode) {
    // For 100G port, enable both channel 0 and 2
    sts |= pipe_mgr_write_register(dev_id,
                                   0,
                                   offsetof(Tofino,
                                            pipes[pipe]
                                                .pmarb.ebp18_reg.egrNx_reg[pg]
                                                .ebuf_disp_regs.chnl_ctrl[2]),
                                   reset_dprs_cred);
  }

  // Step2: Deassert Deparser credit reset signal and Enable channel
  setp_ebuf_disp_regs_dprs_ctrl_reset_cred(&val, 0);
  setp_ebuf_disp_regs_dprs_ctrl_chnl_ena(&val, 1);
  sts |= pipe_mgr_write_register(dev_id,
                                 0,
                                 offsetof(Tofino,
                                          pipes[pipe]
                                              .pmarb.ebp18_reg.egrNx_reg[pg]
                                              .ebuf_disp_regs.chnl_ctrl[ch]),
                                 val);
  if (is_100g_mode) {
    // For 100G port, enable both channel 0 and 2
    sts |= pipe_mgr_write_register(dev_id,
                                   0,
                                   offsetof(Tofino,
                                            pipes[pipe]
                                                .pmarb.ebp18_reg.egrNx_reg[pg]
                                                .ebuf_disp_regs.chnl_ctrl[2]),
                                   val);
  }

  /*
   *For rev B0 and later parts, get the port id based on MAC channels
   * config in the PG. For A0, use PG (MAC) as the port id.
   */
  if (dev_info->part_rev != BF_SKU_CHIP_PART_REV_A0) {
    port_mode_port_id = get_ebuf_disp_regs_port_id(dev_id, port_id);
  } else {
    port_mode_port_id = pg;
  }
  /* Set port mode register in ebuf */
  setp_epb_disp_port_regs_port_mode_port_id(&port_mode, port_mode_port_id);
  sts |= pipe_mgr_write_register(
      dev_id,
      0,
      offsetof(
          Tofino,
          pipes[pipe].pmarb.ebp18_reg.egrNx_reg[pg].ebuf_disp_regs.port_mode),
      port_mode);

  // Disable other channels in case of 50G and 100G
  switch (port_info->speed) {
    case BF_SPEED_50G:
      if ((ch != 1) && (ch != 3)) {
        sts |= pipe_mgr_write_register(
            dev_id,
            0,
            offsetof(Tofino,
                     pipes[pipe]
                         .pmarb.ebp18_reg.egrNx_reg[pg]
                         .ebuf_disp_regs.chnl_ctrl[ch + 1]),
            0);
      }
      break;
    case BF_SPEED_100G:
    case BF_SPEED_40G:
      for (ch = 1; ch < TOF_NUM_CHN_PER_PORT; ch++) {
        sts |=
            pipe_mgr_write_register(dev_id,
                                    0,
                                    offsetof(Tofino,
                                             pipes[pipe]
                                                 .pmarb.ebp18_reg.egrNx_reg[pg]
                                                 .ebuf_disp_regs.chnl_ctrl[ch]),
                                    0);
      }
      break;
    default:
      break;
  }

  return (sts);
}

pipe_status_t pipe_mgr_ebuf_tof_dev_add(rmt_dev_info_t *dev_info) {
  bf_dev_id_t dev_id = dev_info->dev_id;
  union pipe_mgr_ebuf_ctx *e = pipe_mgr_ebuf_ctx(dev_id);
  struct pipe_mgr_tof_ebuf_ctx *ctx = e ? &e->tof : NULL;
  if (!ctx) return PIPE_INVALID_ARG;
  PIPE_MGR_DBGCHK(!ctx->reg);

  ctx->reg = PIPE_MGR_CALLOC(dev_info->num_active_pipes, sizeof *ctx->reg);
  if (!ctx->reg) return PIPE_NO_SYS_RESOURCES;

  for (uint32_t prof_id = 0; prof_id < dev_info->num_pipeline_profiles;
       ++prof_id) {
    struct pipe_config_cache_reg_t *chan_ctrl, *m_thread;
    bf_map_sts_t m = bf_map_get(&dev_info->profile_info[prof_id]->config_cache,
                                pipe_cck_meta_opt_ctrl,
                                (void **)&chan_ctrl);
    if (BF_MAP_OK != m) {
      LOG_ERROR("Dev %d Failed to get parser_chnl_ctrl from profile %d",
                dev_info->dev_id,
                prof_id);
      return PIPE_INIT_ERROR;
    }
    m = bf_map_get(&dev_info->profile_info[prof_id]->config_cache,
                   pipe_cck_parser_multi_threading,
                   (void **)&m_thread);
    if (BF_MAP_OK != m) {
      LOG_ERROR("Dev %d Failed to get parser_multi_thread from profile %d",
                dev_info->dev_id,
                prof_id);
      return PIPE_INIT_ERROR;
    }
    bf_dev_pipe_t pipe_id;
    PIPE_BITMAP_ITER(&dev_info->profile_info[prof_id]->pipe_bmp, pipe_id) {
      for (int prsr = 0; prsr < TOF_NUM_PARSERS; ++prsr) {
        ctx->reg[pipe_id].multi_threading[prsr] = m_thread->val;
        for (int chan = 0; chan < TOF_NUM_CHN_PER_PORT; ++chan)
          ctx->reg[pipe_id].chan_ctrl[prsr][chan] = chan_ctrl->val;
      }
    }
  }
  return PIPE_SUCCESS;
}

void pipe_mgr_ebuf_tof_dev_rmv(rmt_dev_info_t *dev_info) {
  bf_dev_id_t dev_id = dev_info->dev_id;
  union pipe_mgr_ebuf_ctx *e = pipe_mgr_ebuf_ctx(dev_id);
  struct pipe_mgr_tof_ebuf_ctx *ctx = e ? &e->tof : NULL;
  if (ctx && ctx->reg) {
    PIPE_MGR_FREE(ctx->reg);
    ctx->reg = NULL;
  }
}

pipe_status_t pipe_mgr_ebuf_tof_set_port_cut_through(rmt_dev_info_t *dev_info,
                                                     bf_dev_port_t port,
                                                     bool cut_through_enabled) {
  pipe_status_t rc;

  rc = ebuf_set_ebuf_disp_chnl_ctrl_reg(dev_info, port, cut_through_enabled);

  return (rc);
}

pipe_status_t pipe_mgr_ebuf_tof_set_port_chnl_ctrl(rmt_dev_info_t *dev_info,
                                                   bf_dev_port_t port) {
  pipe_status_t rc;

  // Program ebp18_reg.epb_prsr_port_regs.chnl_ctrl
  // Program ebp18_reg.epb_dprs_regs.chnl_ctrl
  // Program ebp18_reg.epb_disp_port_regs.chnl_ctrl
  // Program ebp18_reg.ebuf_fifo_regs.chnl_ctrl
  // Program ebp18_reg.egrNx_reg.ebuf_disp_regs.chnl_ctrl

  rc = ebuf_set_epb_prsr_port_chnl_ctrl_reg(dev_info, port);
  rc |= ebuf_set_epb_prsr_port_chnl_ctrl2_reg(dev_info, port);
  rc |= ebuf_set_epb_prsr_port_id_reg(dev_info, port);
  rc |= ebuf_set_ebuf_disp_port_seq_reg(dev_info, port);
  rc |= ebuf_set_epb_prsr_port_threading(dev_info, port);
  rc |= ebuf_set_epb_disp_chnl_ctrl_reg(dev_info, port);
  rc |= ebuf_set_epb_drpr_chnl_ctrl_reg(dev_info, port);
  rc |= ebuf_set_ebuf_fifo_chnl_ctrl_reg(dev_info, port);
  rc |= ebuf_set_ebuf_disp_chnl_ctrl_reg(
      dev_info, port, false);  // CT disabled when port gets added
  rc |= ebuf_set_ebuf_disp_port_chk_start_eop(dev_info, port);

  return (rc);
}

// Port mode transition issue workaround
pipe_status_t pipe_mgr_ebuf_tof_complete_port_mode_transition_wa(
    rmt_dev_info_t *dev_info, bf_dev_port_t port) {
  pipe_status_t rc;

  rc = ebuf_set_epb_complete_port_mode_transition_wa(dev_info, port);

  return (rc);
}

static pipe_status_t ebuf_disable_epb_prsr_port_chnl_ctrl_reg(
    rmt_dev_info_t *dev_info, bf_dev_port_t port_id) {
  int pg, ch;
  pipe_status_t sts = PIPE_SUCCESS;
  uint32_t *val;
  bf_dev_pipe_t pipe = dev_info->dev_cfg.dev_port_to_pipe(port_id);
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  rmt_port_info_t *port_info = NULL;
  bf_dev_id_t dev_id = dev_info->dev_id;

  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);

  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port);
    return PIPE_INVALID_ARG;
  }
  int phy_pipe = port_info->phy_pipe;

  pg = port / TOF_NUM_CHN_PER_PORT;  // Port group has 4 ports/channels
  ch = port % TOF_NUM_CHN_PER_PORT;

  // Values in this register are updated by compiler too. Hence read-modify
  union pipe_mgr_ebuf_ctx *e = pipe_mgr_ebuf_ctx(dev_id);
  struct pipe_mgr_tof_ebuf_ctx *ctx = e ? &e->tof : NULL;
  if (!ctx) return PIPE_INVALID_ARG;
  val = &ctx->reg[pipe].chan_ctrl[pg][ch];

  // Disable the channel
  setp_epb_prsr_port_regs_chnl_ctrl_chnl_ena(val, 0);

  sts = pipe_mgr_write_register(dev_id,
                                0,
                                offsetof(Tofino,
                                         pipes[phy_pipe]
                                             .pmarb.ebp18_reg.ebp_reg[pg]
                                             .epb_prsr_port_regs.chnl_ctrl[ch]),
                                *val);

  return (sts);
}

static pipe_status_t ebuf_disable_epb_disp_chnl_ctrl_reg(
    rmt_dev_info_t *dev_info, bf_dev_port_t port_id) {
  bf_dev_pipe_t pipe = 0;
  int pg, ch;
  pipe_status_t sts = PIPE_SUCCESS;
  rmt_port_info_t *port_info = NULL;
  uint32_t val = 0;
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  bf_dev_id_t dev_id = dev_info->dev_id;

  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);

  pg = port / TOF_NUM_CHN_PER_PORT;  // Port group has 4 ports/channels
  ch = port % TOF_NUM_CHN_PER_PORT;

  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port);
    return PIPE_INVALID_ARG;
  }
  pipe = port_info->phy_pipe;

  // Disable the channel
  setp_epb_disp_port_regs_chnl_ctrl_chnl_ena(&val, 0);

  sts = pipe_mgr_write_register(dev_id,
                                0,
                                offsetof(Tofino,
                                         pipes[pipe]
                                             .pmarb.ebp18_reg.egrNx_reg[pg]
                                             .epb_disp_port_regs.chnl_ctrl[ch]),
                                val);

  return (sts);
}

static pipe_status_t ebuf_disable_epb_drpr_chnl_ctrl_reg(
    rmt_dev_info_t *dev_info, bf_dev_port_t port_id) {
  bf_dev_pipe_t pipe = 0;
  int pg, ch;
  pipe_status_t sts = PIPE_SUCCESS;
  uint32_t val = 0;
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  rmt_port_info_t *port_info = NULL;
  bf_dev_id_t dev_id = dev_info->dev_id;

  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);

  pg = port / TOF_NUM_CHN_PER_PORT;  // Port group has 4 ports/channels
  ch = port % TOF_NUM_CHN_PER_PORT;

  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port);
    return PIPE_INVALID_ARG;
  }
  pipe = port_info->phy_pipe;

  // Disable the channel
  setp_epb_dprs_regs_chnl_ctrl_chnl_ena(&val, 0);

  sts = pipe_mgr_write_register(dev_id,
                                0,
                                offsetof(Tofino,
                                         pipes[pipe]
                                             .pmarb.ebp18_reg.egrNx_reg[pg]
                                             .epb_dprs_regs.chnl_ctrl[ch]),
                                val);

  return (sts);
}

static pipe_status_t ebuf_disable_ebuf_fifo_chnl_ctrl_reg(
    rmt_dev_info_t *dev_info, bf_dev_port_t port_id) {
  bf_dev_pipe_t pipe = 0;
  int pg, ch;
  pipe_status_t sts = PIPE_SUCCESS;
  rmt_port_info_t *port_info = NULL;
  uint32_t val = 0;
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  bf_dev_id_t dev_id = dev_info->dev_id;

  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);

  pg = port / TOF_NUM_CHN_PER_PORT;  // Port group has 4 ports/channels
  ch = port % TOF_NUM_CHN_PER_PORT;

  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port);
    return PIPE_INVALID_ARG;
  }
  pipe = port_info->phy_pipe;

  // Disable the channel
  setp_ebuf_fifo_regs_dprs_ctrl_chnl_ena(&val, 0);

  sts = pipe_mgr_write_register(dev_id,
                                0,
                                offsetof(Tofino,
                                         pipes[pipe]
                                             .pmarb.ebp18_reg.egrNx_reg[pg]
                                             .ebuf_fifo_regs.chnl_ctrl[ch]),
                                val);

  return (sts);
}

static pipe_status_t ebuf_disable_ebuf_disp_chnl_ctrl_reg(
    rmt_dev_info_t *dev_info, bf_dev_port_t port_id) {
  bf_dev_pipe_t pipe = 0;
  int pg, ch;
  pipe_status_t sts = PIPE_SUCCESS;
  rmt_port_info_t *port_info = NULL;
  uint32_t val = 0;
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  bf_dev_id_t dev_id = dev_info->dev_id;

  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);

  pg = port / TOF_NUM_CHN_PER_PORT;  // Port group has 4 ports/channels
  ch = port % TOF_NUM_CHN_PER_PORT;

  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port);
    return PIPE_INVALID_ARG;
  }
  pipe = port_info->phy_pipe;

  // Disable the channel
  setp_ebuf_disp_regs_dprs_ctrl_chnl_ena(&val, 0);
  sts = pipe_mgr_write_register(dev_id,
                                0,
                                offsetof(Tofino,
                                         pipes[pipe]
                                             .pmarb.ebp18_reg.egrNx_reg[pg]
                                             .ebuf_disp_regs.chnl_ctrl[ch]),
                                val);

  return (sts);
}

pipe_status_t pipe_mgr_ebuf_tof_disable_port_chnl(rmt_dev_info_t *dev_info,
                                                  bf_dev_port_t port_id) {
  pipe_status_t rc;

  // Disable ebp18_reg.ebp_reg.epb_prsr_port_regs.chnl_ctrl.chnl_en
  // Disable ebp18_reg.egrNx_reg.epb_dprs_regs.chnl_ctrl.chnl_en
  // Disable ebp18_reg.egrNx_reg.epb_disp_port_regs.chnl_ctrl.chnl_en
  // Disable ebp18_reg.egrNx_reg.ebuf_fifo_regs.chnl_ctrl.chnl_en
  // Disable ebp18_reg.egrNx_reg.ebuf_disp_regs.chnl_ctrl.chnl_en

  rc = ebuf_disable_epb_prsr_port_chnl_ctrl_reg(dev_info, port_id);
  rc |= ebuf_disable_epb_disp_chnl_ctrl_reg(dev_info, port_id);
  rc |= ebuf_disable_epb_drpr_chnl_ctrl_reg(dev_info, port_id);
  rc |= ebuf_disable_ebuf_fifo_chnl_ctrl_reg(dev_info, port_id);
  rc |= ebuf_disable_ebuf_disp_chnl_ctrl_reg(dev_info, port_id);

  return (rc);
}

pipe_status_t pipe_mgr_ebuf_tof_set_1588_timestamp_offset(
    pipe_sess_hdl_t sess_hdl, rmt_dev_info_t *dev_info) {
  uint32_t ts_1588_offset[18] = {0x9000,
                                 0x9000,
                                 0x9000,
                                 0x10000,
                                 0x10000,
                                 0x10000,
                                 0x11000,
                                 0x11000,
                                 0x11000,
                                 0x12000,
                                 0x12000,
                                 0x12000,
                                 0x13000,
                                 0x13000,
                                 0x13000,
                                 0x14000,
                                 0x14000,
                                 0x14000};
  bf_dev_id_t dev_id = dev_info->dev_id;
  int i, prsr;
  pipe_instr_write_reg_t instr;
  pipe_status_t sts = PIPE_SUCCESS;
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
  /* Set timestamp on every prsr */
  int num_prsrs = pipe_mgr_num_prsrs(dev_id);
  for (prsr = 0; prsr < num_prsrs; ++prsr) {
    construct_instr_reg_write(
        dev_id,
        &instr,
        offsetof(
            Tofino,
            pipes[0].pmarb.ebp18_reg.ebp_reg[prsr].epb_prsr_port_regs.tim_off),
        ts_1588_offset[prsr]);
    sts = pipe_mgr_drv_ilist_add(&sess_hdl,
                                 dev_info,
                                 &pipe_bit_map,
                                 prsr_stage,
                                 (uint8_t *)&instr,
                                 sizeof instr);

    if (PIPE_SUCCESS != sts) {
      LOG_ERROR(
          "Failed to add timestamp offset value to instruction list for epb "
          "%d, rc = (%d)",
          prsr,
          sts);
      PIPE_MGR_DBGCHK(0);
      return sts;
    }
  }

  return sts;
}

pipe_status_t pipe_mgr_ebuf_tof_epb_set_100g_credits(pipe_sess_hdl_t sess_hdl,
                                                     rmt_dev_info_t *dev_info) {
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
  /* Set credit control value for all port speeds.
   * Power on default value for 100G is 0x18. It should be chaanged to 0x14
   * Power on default values for 50G, 25G are good.
   * We need to repush same values as power on default except
   * 100g_cred set to 0x14
   *
   * 32b value will be as below. Unused bits are set to 0x0
   * [4:0] = 0x14
   * [12:8] = 0xc
   * [20:16] = 0x6
   * [28:24] = 0x1
   * [31:31] = 0x1
   *
   *  value = 0x81060c14
   */
  int num_prsrs = pipe_mgr_num_prsrs(dev_id);
  for (port = 0; port < num_prsrs; ++port) {
    construct_instr_reg_write(dev_id,
                              &instr,
                              offsetof(Tofino,
                                       pipes[0]
                                           .pmarb.ebp18_reg.egrNx_reg[port]
                                           .epb_disp_port_regs.cred_ctrl),
                              0x81060c14);
    sts = pipe_mgr_drv_ilist_add(&sess_hdl,
                                 dev_info,
                                 &pipe_bit_map,
                                 prsr_stage,
                                 (uint8_t *)&instr,
                                 sizeof instr);

    if (PIPE_SUCCESS != sts) {
      LOG_ERROR(
          "Failed to add timestamp offset value to instruction list for epb "
          "%d, rc = (%d)",
          port,
          sts);
      PIPE_MGR_DBGCHK(0);
      return sts;
    }
  }

  return sts;
}

/*
 * TODO:
 *    Move these macros to common header file when wait_for_flush
 *    gets implemented for Tofino2 as well.
 */
#define BF_TOF_EBUF_FLUSH_NUM_WAITS 10  // Total time is for 500 microseconds
#define BF_TOF_EBUF_FLUSH_WAIT_TIME 50  // 50 microseconds

/*
 * Wait for packets to drain in EBUF of all ports/channels.
 */
pipe_status_t pipe_mgr_ebuf_tof_wait_for_flush_all_chan(
    pipe_sess_hdl_t sess_hdl, rmt_dev_info_t *dev_info) {
  (void)sess_hdl;
  pipe_status_t sts = PIPE_SUCCESS;
  bf_dev_id_t dev_id = dev_info->dev_id;
  bf_dev_pipe_t log_pipe;
  bf_dev_pipe_t num_pipes = pipe_mgr_get_num_active_pipes(dev_id);
  uint32_t num_fifo_entries_avail = 0;
  int rc;
  uint32_t num_waits = 0;

  LOG_TRACE(
      "%s: Waiting for packets to get flushed in "
      "all EBUF channel for dev %d",
      __func__,
      dev_id);
  for (log_pipe = 0; log_pipe < num_pipes; log_pipe++) {
    bf_dev_pipe_t phy_pipe = 0;
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe, &phy_pipe);

    /*
     * Wait for traffic flush for EBUFs, only if it's connected to MAC.
     */
    int max_mac_pg =
        (log_pipe == 0) ? TOF_NUM_PARSERS - 1 : TOF_NUM_PARSERS - 2;
    for (int pg = 0; pg < max_mac_pg; pg++) {
      num_waits = 0;  // Reset num_waits for each pg
      while (num_waits++ <= BF_TOF_EBUF_FLUSH_NUM_WAITS) {
        num_fifo_entries_avail = 0;
        /*
         * Do direct PCIe read of EBUF dispatch FIFO register and
         * make sure it has full entries available (meaning no packets
         * in EBUF and it's empty) for all ports. This register value is
         * for all 4 channels in the port group.
         */
        LOG_TRACE("%s: checking for packets in EBUF for pipe %d, port group %d",
                  __func__,
                  phy_pipe,
                  pg);

        rc = lld_read_register(
            dev_id,
            offsetof(Tofino,
                     pipes[phy_pipe]
                         .pmarb.ebp18_reg.egrNx_reg[pg]
                         .ebuf_disp_regs.ebuf_disp_fifo_avail),
            &num_fifo_entries_avail);
        if (rc) {
          LOG_ERROR("%s: LLD read register failed for dev%d, rc = %d",
                    __func__,
                    dev_id,
                    rc);
          return PIPE_LLD_FAILED;
        }

        if (num_fifo_entries_avail == BF_TOF_EBUF_DISPATCH_FIFO_NUM_ENTRIES) {
          // All EBUF channels in this port group are empty
          break;
        }

        if (num_waits > BF_TOF_EBUF_FLUSH_NUM_WAITS) {
          // Fatal error, fast reconfig can't be done, return error
          LOG_ERROR(
              "%s:%d  packets NOT getting flushed in EBUF for pipe %d, port "
              "group %d, "
              "num FIFO entries %u",
              __func__,
              __LINE__,
              phy_pipe,
              pg,
              num_fifo_entries_avail);

          return PIPE_UNEXPECTED;
        }

        LOG_TRACE(
            "%s:%d  packets still pending in EBUF for pipe %d, port group %d, "
            "num FIFO entries %u",
            __func__,
            __LINE__,
            phy_pipe,
            pg,
            num_fifo_entries_avail);

        // Sleep and retry
        bf_sys_usleep(BF_TOF_EBUF_FLUSH_WAIT_TIME);
      }
    }
  }

  return sts;
}

pipe_status_t pipe_mgr_ebuf_tof_get_port_counter(rmt_dev_info_t *dev_info,
                                                 bf_dev_port_t port_id,
                                                 uint64_t *value) {
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  bf_dev_id_t dev_id = dev_info->dev_id;
  rmt_port_info_t *port_info = NULL;
  int p, rc, pg, ch;
  uint64_t reg_cnt = 0;
  uint64_t cnt = 0;
  uint32_t val = 0;

  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);

  pg = port / TOF_NUM_CHN_PER_PORT;  // Port group has 4 ports/channels
  ch = port % TOF_NUM_CHN_PER_PORT;

  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port);
    return PIPE_INVALID_ARG;
  }
  p = port_info->phy_pipe;

  cnt = 0;
  rc = lld_read_register(dev_id,
                         offsetof(Tofino,
                                  pipes[p]
                                      .pmarb.ebp18_reg.egrNx_reg[pg]
                                      .epb_disp_port_regs.egr_pipe_count[ch]
                                      .egr_pipe_count_0_2),
                         &val);
  if (rc) {
    LOG_ERROR("%s: LLD read register failed for dev%d, rc = %d",
              __func__,
              dev_id,
              rc);
    return PIPE_LLD_FAILED;
  }

  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  rc = lld_read_register(dev_id,
                         offsetof(Tofino,
                                  pipes[p]
                                      .pmarb.ebp18_reg.egrNx_reg[pg]
                                      .epb_disp_port_regs.egr_pipe_count[ch]
                                      .egr_pipe_count_1_2),
                         &val);
  if (rc) {
    LOG_ERROR("%s: LLD read register failed for dev%d, rc = %d",
              __func__,
              dev_id,
              rc);
    return PIPE_LLD_FAILED;
  }

  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  *value = cnt;

  return (rc);
}

pipe_status_t pipe_mgr_ebuf_tof_get_port_bypass_counter(
    rmt_dev_info_t *dev_info, bf_dev_port_t port_id, uint64_t *value) {
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  bf_dev_id_t dev_id = dev_info->dev_id;
  rmt_port_info_t *port_info = NULL;
  int p, rc, pg, ch;
  uint64_t reg_cnt = 0;
  uint64_t cnt = 0;
  uint32_t val = 0;

  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);

  pg = port / TOF_NUM_CHN_PER_PORT;  // Port group has 4 ports/channels
  ch = port % TOF_NUM_CHN_PER_PORT;

  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port);
    return PIPE_INVALID_ARG;
  }
  p = port_info->phy_pipe;

  cnt = 0;
  rc = lld_read_register(dev_id,
                         offsetof(Tofino,
                                  pipes[p]
                                      .pmarb.ebp18_reg.egrNx_reg[pg]
                                      .epb_disp_port_regs.egr_bypass_count[ch]
                                      .egr_bypass_count_0_2),
                         &val);
  if (rc) {
    LOG_ERROR("%s: LLD read register failed for dev%d, rc = %d",
              __func__,
              dev_id,
              rc);
    return PIPE_LLD_FAILED;
  }

  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  rc = lld_read_register(dev_id,
                         offsetof(Tofino,
                                  pipes[p]
                                      .pmarb.ebp18_reg.egrNx_reg[pg]
                                      .epb_disp_port_regs.egr_bypass_count[ch]
                                      .egr_bypass_count_1_2),
                         &val);
  if (rc) {
    LOG_ERROR("%s: LLD read register failed for dev%d, rc = %d",
              __func__,
              dev_id,
              rc);
    return PIPE_LLD_FAILED;
  }

  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  *value = cnt;

  return (rc);
}

pipe_status_t pipe_mgr_ebuf_tof_get_port_100g_credits(rmt_dev_info_t *dev_info,
                                                      bf_dev_port_t port_id,
                                                      uint64_t *value) {
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  bf_dev_id_t dev_id = dev_info->dev_id;
  rmt_port_info_t *port_info = NULL;
  int p, rc, pg;
  uint32_t reg_val = 0;

  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);

  pg = port / TOF_NUM_CHN_PER_PORT;  // Port group has 4 ports/channels

  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port);
    return PIPE_INVALID_ARG;
  }
  p = port_info->phy_pipe;

  rc = lld_read_register(
      dev_id,
      offsetof(
          Tofino,
          pipes[p].pmarb.ebp18_reg.egrNx_reg[pg].epb_disp_port_regs.cred_ctrl),
      &reg_val);
  if (rc) {
    LOG_ERROR("%s: LLD read register failed for dev%d, rc = %d",
              __func__,
              dev_id,
              rc);
    return PIPE_LLD_FAILED;
  }

  *value = (reg_val & 0x1F);

  return (rc);
}
