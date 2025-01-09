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
#include <lld/lld_reg_if.h>
#include "pipe_mgr_int.h"
#include "pipe_mgr_drv.h"
#include "pipe_mgr_drv_intf.h"
#include "pipe_mgr_pktgen_comm.h"
#include "pipe_mgr_tof_pktgen.h"
#include <tofino_regs/pipe_top_level.h>
#include <tofino_regs/tofino.h>

/* Context-independent recirc state */
bool recirc_enable[PIPE_MGR_NUM_DEVICES][PIPE_MGR_MAX_PIPES] = {{0}};

/* Pointer to global pipe_mgr context */
extern pipe_mgr_ctx_t *pipe_mgr_ctx;

static bf_status_t pg_tof_read_one_pipe_reg(bf_dev_target_t dev_tgt,
                                            uint32_t addr,
                                            uint32_t *data) {
  *data = 0;
  bf_dev_pipe_t physical_pipe = dev_tgt.dev_pipe_id;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_tgt.device_id);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return BF_INVALID_ARG;
  }
  pipe_mgr_map_pipe_id_log_to_phy(
      dev_info, dev_tgt.dev_pipe_id, &physical_pipe);
  uint32_t base =
      offsetof(Tofino, pipes[physical_pipe]) - offsetof(Tofino, pipes[0]);

  int res = lld_read_register(dev_tgt.device_id, base + addr, data);
  return res ? BF_HW_COMM_FAIL : BF_SUCCESS;
}

/* Write pkt buffer shadow memory to asic */
static bf_status_t pipe_mgr_pkt_buffer_tof_addr_get(uint64_t *addr,
                                                    uint32_t *elem_size,
                                                    uint32_t *count) {
  *addr = pipe_top_level_pipes_party_pgr_buffer_mem_word_address / 16;
  /* Write the entire 16K pkt buffer */
  *elem_size =
      pipe_top_level_pipes_party_pgr_buffer_mem_word_array_element_size;
  *count = pipe_top_level_pipes_party_pgr_buffer_mem_word_array_count;
  return BF_SUCCESS;
}

static struct pipe_mgr_tof_pg_dev_ctx *pipe_mgr_tof_get_pgr_ctx(
    bf_dev_id_t dev) {
  struct pipe_mgr_pg_dev_ctx *ctx = pipe_mgr_pktgen_ctx(dev);
  return ctx ? ctx->u.tof_ctx : NULL;
}
static bf_status_t pipe_mgr_tof_pktgen_reg_pgr_com_port17_ctrl1(
    pipe_sess_hdl_t sid,
    bf_dev_target_t dev_tgt,
    bool recir_en,
    bool pgen_en,
    bool ecc_dis,
    bool swap_en,
    uint8_t crc_dis,
    uint8_t chan_en,
    uint8_t chan_mode,
    uint8_t chan_seq,
    bool use_shadow) {
  uint32_t data = 0;
  struct pipe_mgr_tof_pg_dev_ctx *ctx =
      pipe_mgr_tof_get_pgr_ctx(dev_tgt.device_id);
  if (!ctx) {
    LOG_ERROR("%s : Failed to get device context, dev %d",
              __func__,
              dev_tgt.device_id);
    return BF_UNEXPECTED;
  }
  if (use_shadow) {
    data = ctx->port17_ctrl1[dev_tgt.dev_pipe_id];
  } else {
    setp_pgr_port17_ctrl1_channel_seq(&data, chan_seq);
    setp_pgr_port17_ctrl1_channel_mode(&data, chan_mode);
    setp_pgr_port17_ctrl1_channel_en(&data, chan_en);
    setp_pgr_port17_ctrl1_crc_dis(&data, crc_dis);
    setp_pgr_port17_ctrl1_swap_en(&data, swap_en);
    setp_pgr_port17_ctrl1_ecc_dis(&data, ecc_dis);
    setp_pgr_port17_ctrl1_pgen_en(&data, pgen_en);
    setp_pgr_port17_ctrl1_recir_en(&data, recir_en);

    /* Update the shadow of this register. */
    ctx->port17_ctrl1[dev_tgt.dev_pipe_id] = data;
  }

  uint32_t addr =
      offsetof(Tofino, pipes[0].pmarb.pgr_reg.pgr_common.port17_ctrl1);
  return pg_write_one_pipe_reg(
      sid, dev_tgt.device_id, 1 << dev_tgt.dev_pipe_id, addr, data);
}

static bf_status_t pipe_mgr_tof_pktgen_reg_pgr_com_port17_ctrl2(
    pipe_sess_hdl_t sid,
    bf_dev_target_t dev_tgt,
    uint8_t recirc_chan_en,
    uint8_t pgen_chan_en,
    uint8_t pgen_pipe,
    bool use_shadow) {
  uint32_t data = 0;
  struct pipe_mgr_tof_pg_dev_ctx *ctx =
      pipe_mgr_tof_get_pgr_ctx(dev_tgt.device_id);
  if (!ctx) {
    LOG_ERROR("%s : Failed to get device context, dev %d",
              __func__,
              dev_tgt.device_id);
    return BF_UNEXPECTED;
  }
  if (use_shadow) {
    data = ctx->port17_ctrl2[dev_tgt.dev_pipe_id];
  } else {
    setp_pgr_port17_ctrl2_pgen_channel_en(&data, pgen_chan_en);
    setp_pgr_port17_ctrl2_recir_channel_en(&data, recirc_chan_en);
    setp_pgr_port17_ctrl2_pgen_pipe_id(&data, pgen_pipe);
    ctx->port17_ctrl2[dev_tgt.dev_pipe_id] = data;
  }
  uint32_t addr =
      offsetof(Tofino, pipes[0].pmarb.pgr_reg.pgr_common.port17_ctrl2);
  return pg_write_one_pipe_reg(
      sid, dev_tgt.device_id, 1 << dev_tgt.dev_pipe_id, addr, data);
}

static bf_status_t pipe_mgr_tof_pktgen_reg_pgr_com_port16_set_recirc_cpu(
    pipe_sess_hdl_t sid, bf_dev_target_t dev_tgt, bool recir, bool mxbar) {
  uint32_t recir_en = recir ? 1 : 0;
  uint32_t mxbar_en = mxbar ? 1 : 0;
  struct pipe_mgr_tof_pg_dev_ctx *ctx =
      pipe_mgr_tof_get_pgr_ctx(dev_tgt.device_id);
  if (!ctx) {
    LOG_ERROR("%s : Failed to get device context, dev %d",
              __func__,
              dev_tgt.device_id);
    return BF_UNEXPECTED;
  }
  uint32_t *data = &ctx->port16_ctrl[dev_tgt.dev_pipe_id];
  setp_pgr_port16_ctrl_mxbar_en(data, mxbar_en);
  setp_pgr_port16_ctrl_recir_en(data, recir_en);
  recirc_enable[dev_tgt.device_id][dev_tgt.dev_pipe_id] = recir_en;
  uint32_t addr =
      offsetof(Tofino, pipes[0].pmarb.pgr_reg.pgr_common.port16_ctrl);
  return pg_write_one_pipe_reg(
      sid, dev_tgt.device_id, 1 << dev_tgt.dev_pipe_id, addr, *data);
}

static void free_dev_ctx(struct pipe_mgr_pg_dev_ctx *c, bf_dev_id_t dev) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    LOG_ERROR("%s : Failed to get device info, dev %d ", __func__, dev);
    return;
  }
  int dev_pipes = dev_info->num_active_pipes;
  int p;
  if (!c) return;
  if (!c->u.tof_ctx) {
    PIPE_MGR_FREE(c);
    return;
  }
  struct pipe_mgr_tof_pg_dev_ctx *ctx = c->u.tof_ctx;
  if (ctx->port16_chan) {
    for (p = 0; p < dev_pipes; ++p) {
      if (ctx->port16_chan[p]) PIPE_MGR_FREE(ctx->port16_chan[p]);
    }
    PIPE_MGR_FREE(ctx->port16_chan);
  }
  if (ctx->port17_chan) {
    for (p = 0; p < dev_pipes; ++p) {
      if (ctx->port17_chan[p]) PIPE_MGR_FREE(ctx->port17_chan[p]);
    }
    PIPE_MGR_FREE(ctx->port17_chan);
  }
  if (ctx->app.a) {
    for (p = 0; p < dev_pipes; ++p) {
      if (ctx->app.a[p]) PIPE_MGR_FREE(ctx->app.a[p]);
    }
    PIPE_MGR_FREE(ctx->app.a);
  }
  if (ctx->app.b) {
    for (p = 0; p < dev_pipes; ++p) {
      if (ctx->app.b[p]) PIPE_MGR_FREE(ctx->app.b[p]);
    }
    PIPE_MGR_FREE(ctx->app.b);
  }
  if (ctx->port16_ctrl) PIPE_MGR_FREE(ctx->port16_ctrl);
  if (ctx->port17_ctrl1) PIPE_MGR_FREE(ctx->port17_ctrl1);
  if (ctx->port17_ctrl2) PIPE_MGR_FREE(ctx->port17_ctrl2);
  if (ctx->pkt_buffer_shadow) PIPE_MGR_FREE(ctx->pkt_buffer_shadow);
  PIPE_MGR_FREE(ctx);
  PIPE_MGR_FREE(c);
}

static uint32_t get_port_down_dis_addr(int local_port_bit_idx) {
  if (local_port_bit_idx < 32) {
    return offsetof(
        Tofino,
        pipes[0].pmarb.pgr_reg.pgr_common.port_down_dis.port_down_dis_0_3);
  } else if (local_port_bit_idx < 64) {
    return offsetof(
        Tofino,
        pipes[0].pmarb.pgr_reg.pgr_common.port_down_dis.port_down_dis_1_3);
  } else {
    return offsetof(
        Tofino,
        pipes[0].pmarb.pgr_reg.pgr_common.port_down_dis.port_down_dis_2_3);
  }
}

bf_status_t pipe_mgr_tof_pktgen_dev_init(bf_session_hdl_t shdl,
                                         bf_dev_id_t dev,
                                         bool use_shadow) {
  bf_status_t sts = BF_SUCCESS;
  /* Enable pkt-gen and recirc on the pipes.  This is a global enable, it is
   * still disabled by default on individual channels. */
  bf_dev_target_t dev_tgt = {dev, BF_INVALID_PIPE};
  uint32_t pipe_cnt = pipe_mgr_get_num_active_pipes(dev);
  for (uint32_t i = 0; i < pipe_cnt; ++i) {
    dev_tgt.dev_pipe_id = i;
    /* Enable CPU connectivity by default on port group 16 ports for pipes
     * connecting to the CPU.  For two pipe chips both connect to the CPU
     * but on four pipe chips only zero and three connect to the CPU. */
    bool cpu_en = 2 == pipe_cnt || !i || 2 == i;
    bool recirc_en = !cpu_en;
    sts = pipe_mgr_tof_pktgen_reg_pgr_com_port16_set_recirc_cpu(
        shdl, dev_tgt, recirc_en, cpu_en);
    if (BF_SUCCESS != sts) return sts;
    sts =
        pipe_mgr_tof_pktgen_reg_pgr_com_port17_ctrl1(shdl,
                                                     dev_tgt,
                                                     true,
                                                     true,  /* pgen and recir */
                                                     false, /* ecc-disable */
                                                     true,  /* swap-enable */
                                                     0xF,   /* CRC-disable */
                                                     0,
                                                     0,
                                                     0 /* Chan en/mode/seq */,
                                                     use_shadow);
    if (BF_SUCCESS != sts) return sts;
    sts = pipe_mgr_tof_pktgen_reg_pgr_com_port17_ctrl2(
        shdl,
        dev_tgt,
        0,
        0, /* pgen and recir chan enable */
        i /* pgen_pipe */,
        use_shadow);
    if (BF_SUCCESS != sts) return sts;
  }

  return PIPE_SUCCESS;
}

bf_status_t pipe_mgr_tof_pktgen_add_dev(bf_session_hdl_t shdl,
                                        bf_dev_id_t dev) {
  bf_status_t sts = BF_SUCCESS;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) return BF_INVALID_ARG;

  /* Initialize context for this device. */
  struct pipe_mgr_pg_dev_ctx *c = PIPE_MGR_CALLOC(1, sizeof *c);
  if (!c) return BF_NO_SYS_RESOURCES;
  c->u.tof_ctx = PIPE_MGR_CALLOC(1, sizeof(*c->u.tof_ctx));
  if (!c->u.tof_ctx) {
    PIPE_MGR_FREE(c);
    return BF_NO_SYS_RESOURCES;
  }
  struct pipe_mgr_tof_pg_dev_ctx *ctx = c->u.tof_ctx;
  int dev_pipes = dev_info->num_active_pipes;
  int dev_chans = dev_info->dev_cfg.num_chan_per_port;
  int p;
  ctx->port16_chan = PIPE_MGR_CALLOC(dev_pipes, sizeof(*ctx->port16_chan));
  ctx->port17_chan = PIPE_MGR_CALLOC(dev_pipes, sizeof(*ctx->port17_chan));
  ctx->app.a = PIPE_MGR_CALLOC(dev_pipes, sizeof(*ctx->app.a));
  ctx->app.b = PIPE_MGR_CALLOC(dev_pipes, sizeof(*ctx->app.b));
  if (!ctx->port16_chan || !ctx->port17_chan || !ctx->app.a || !ctx->app.b)
    goto cleanup;
  for (p = 0; p < dev_pipes; ++p) {
    ctx->port16_chan[p] =
        PIPE_MGR_CALLOC(dev_chans, sizeof(*ctx->port16_chan[p]));
    ctx->port17_chan[p] =
        PIPE_MGR_CALLOC(dev_chans, sizeof(*ctx->port17_chan[p]));
    ctx->app.a[p] =
        PIPE_MGR_CALLOC(PIPE_MGR_TOF_PKTGEN_APP_CNT, sizeof(*ctx->app.a[p]));
    ctx->app.b[p] =
        PIPE_MGR_CALLOC(PIPE_MGR_TOF_PKTGEN_APP_CNT, sizeof(*ctx->app.b[p]));
    if (!ctx->port16_chan[p] || !ctx->port17_chan[p] || !ctx->app.a[p] ||
        !ctx->app.b[p])
      goto cleanup;
  }
  ctx->port16_ctrl = PIPE_MGR_CALLOC(dev_pipes, sizeof(*ctx->port16_ctrl));
  ctx->port17_ctrl1 = PIPE_MGR_CALLOC(dev_pipes, sizeof(*ctx->port17_ctrl1));
  ctx->port17_ctrl2 = PIPE_MGR_CALLOC(dev_pipes, sizeof(*ctx->port17_ctrl2));
  ctx->pkt_buffer_shadow =
      PIPE_MGR_CALLOC(dev_pipes, sizeof(*ctx->pkt_buffer_shadow));
  if (!ctx->port16_ctrl || !ctx->port17_ctrl1 || !ctx->port17_ctrl2 ||
      !ctx->pkt_buffer_shadow) {
    goto cleanup;
  }

  pipe_mgr_pktgen_ctx_set(dev, c);

  sts = pipe_mgr_tof_pktgen_dev_init(shdl, dev, false);
  if (BF_SUCCESS != sts) return sts;

  return BF_SUCCESS;
cleanup:
  free_dev_ctx(c, dev);
  return BF_NO_SYS_RESOURCES;
}

bf_status_t pipe_mgr_tof_pktgen_rmv_dev(bf_dev_id_t dev) {
  struct pipe_mgr_pg_dev_ctx *c = pipe_mgr_pktgen_ctx(dev);
  pipe_mgr_pktgen_ctx_set(dev, NULL);
  free_dev_ctx(c, dev);
  return BF_SUCCESS;
}

static uint8_t get_chan_seq(int *chan) {
  uint8_t chan_seq = 0;
  if (100 == chan[0]) {  // 100
    PIPE_MGR_DBGCHK(100 == chan[1]);
    PIPE_MGR_DBGCHK(100 == chan[2]);
    PIPE_MGR_DBGCHK(100 == chan[3]);
    chan_seq = 0x00;
  } else if (50 == chan[0]) {  // 50-50 or 50-25-25
    PIPE_MGR_DBGCHK(50 == chan[1]);
    if (50 == chan[2]) {
      PIPE_MGR_DBGCHK(50 == chan[3]);
      chan_seq = 0x88;
    } else {
      chan_seq = 0xC8;
    }
  } else {  // 25-25-50 or 25-25-25-25
    if (50 == chan[2]) {
      chan_seq = 0x98;
    } else {
      chan_seq = 0xD8;
    }
  }

  return chan_seq;
}

static uint8_t get_chan_en(int *chan) {
  uint8_t chan_en = 0;
  // Bit 0: 100, 50, or 25
  if (chan[0]) chan_en |= 1;
  // Bit 1: 25
  if (25 == chan[1]) chan_en |= 2;
  // Bit 2: 50 or 25
  if (50 == chan[2] || 25 == chan[2]) chan_en |= 4;
  // Bit 3: 25
  if (25 == chan[3]) chan_en |= 8;

  return chan_en;
}
static uint8_t get_chan_mode(int *chan) {
  if (100 == chan[0]) return 0;
  if (50 == chan[0] && 50 == chan[2]) return 1;
  if (50 == chan[0] && 50 != chan[2]) return 2;
  if (50 != chan[0] && 50 == chan[2]) return 3;
  return 4;
}
static bf_status_t pipe_mgr_tof_pktgen_reg_pgr_com_port16_ctrl(
    pipe_sess_hdl_t sid,
    bf_dev_target_t dev_tgt,
    uint8_t crc_dis,
    uint8_t chan_en,
    uint8_t chan_mode,
    uint8_t chan_seq) {
  struct pipe_mgr_tof_pg_dev_ctx *ctx =
      pipe_mgr_tof_get_pgr_ctx(dev_tgt.device_id);
  if (!ctx) {
    LOG_ERROR("%s : Failed to get device context, dev %d",
              __func__,
              dev_tgt.device_id);
    return BF_UNEXPECTED;
  }
  uint32_t *data = &ctx->port16_ctrl[dev_tgt.dev_pipe_id];
  setp_pgr_port16_ctrl_channel_seq(data, chan_seq);
  setp_pgr_port16_ctrl_channel_mode(data, chan_mode);
  setp_pgr_port16_ctrl_channel_en(data, chan_en);
  setp_pgr_port16_ctrl_crc_dis(data, crc_dis);
  // setp_pgr_port16_ctrl_mxbar_en(data, mxbar_en);
  // setp_pgr_port16_ctrl_recir_en(data, recir_en);
  uint32_t addr =
      offsetof(Tofino, pipes[0].pmarb.pgr_reg.pgr_common.port16_ctrl);
  return pg_write_one_pipe_reg(
      sid, dev_tgt.device_id, 1 << dev_tgt.dev_pipe_id, addr, *data);
}

static bf_status_t pipe_mgr_tof_pktgen_reg_pgr_com_port17_ctrl1_chan(
    pipe_sess_hdl_t sid,
    bf_dev_target_t dev_tgt,
    uint8_t chan_en,
    uint8_t chan_mode,
    uint8_t chan_seq) {
  uint32_t addr =
      offsetof(Tofino, pipes[0].pmarb.pgr_reg.pgr_common.port17_ctrl1);
  struct pipe_mgr_tof_pg_dev_ctx *ctx =
      pipe_mgr_tof_get_pgr_ctx(dev_tgt.device_id);
  if (!ctx) {
    LOG_ERROR("%s : Failed to get device context, dev %d",
              __func__,
              dev_tgt.device_id);
    return BF_UNEXPECTED;
  }
  setp_pgr_port17_ctrl1_channel_seq(&ctx->port17_ctrl1[dev_tgt.dev_pipe_id],
                                    chan_seq);
  setp_pgr_port17_ctrl1_channel_mode(&ctx->port17_ctrl1[dev_tgt.dev_pipe_id],
                                     chan_mode);
  setp_pgr_port17_ctrl1_channel_en(&ctx->port17_ctrl1[dev_tgt.dev_pipe_id],
                                   chan_en);
  setp_pgr_port17_ctrl1_crc_dis(&ctx->port17_ctrl1[dev_tgt.dev_pipe_id], 0xF);
  return pg_write_one_pipe_reg(sid,
                               dev_tgt.device_id,
                               1 << dev_tgt.dev_pipe_id,
                               addr,
                               ctx->port17_ctrl1[dev_tgt.dev_pipe_id]);
}

bf_status_t pipe_mgr_tof_pktgen_port_add(rmt_dev_info_t *dev_info,
                                         bf_dev_port_t port_id,
                                         bf_port_speeds_t speed) {
  bf_dev_id_t dev = dev_info->dev_id;
  bf_dev_pipe_t pipe = dev_info->dev_cfg.dev_port_to_pipe(port_id);
  bf_dev_target_t dev_tgt = {dev, pipe};
  int port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  LOG_TRACE(
      "%s dev %d pipe %d port %d speed 0x%x", __func__, dev, pipe, port, speed);
  bf_status_t sts = BF_SUCCESS;
  bool isPort16 = 64 <= port && 67 >= port;
  bool isPort17 = 68 <= port && 71 >= port;
  struct pipe_mgr_tof_pg_dev_ctx *ctx = pipe_mgr_tof_get_pgr_ctx(dev);
  if (!ctx) {
    LOG_ERROR("%s : Failed to get device context, dev %d", __func__, dev);
    return BF_UNEXPECTED;
  }
  int *chanp = isPort16 ? &ctx->port16_chan[pipe][0]
                        : (isPort17 ? &ctx->port17_chan[pipe][0] : NULL);
  if (!chanp) return BF_SUCCESS;

  int chan = port & 3;
  LOG_TRACE("    chan %d, port %d", chan, isPort16 ? 16 : 17);

  /* Update shadow of port speed config. */
  if (BF_SPEED_100G == speed) {
    chanp[0] = chanp[1] = chanp[2] = chanp[3] = 100;
  } else if (BF_SPEED_40G == speed || BF_SPEED_50G == speed) {
    chanp[chan] = chanp[chan + 1] = 50;
  } else if (BF_SPEED_25G == speed || BF_SPEED_10G == speed ||
             BF_SPEED_1G == speed) {
    chanp[chan] = 25;
  }
  LOG_TRACE("    %d %d %d %d", chanp[0], chanp[1], chanp[2], chanp[3]);

  /* Compute the channel enable bit mask to program. */
  uint8_t chan_en = get_chan_en(chanp);
  /* Compute the channel sequence to program. */
  uint8_t chan_seq = get_chan_seq(chanp);
  /* Compute the channel mode to program. */
  uint8_t chan_mode = get_chan_mode(chanp);
  /* Use the default session to set this up. */
  pipe_sess_hdl_t sid = pipe_mgr_ctx->int_ses_hndl;

  LOG_TRACE("    En 0x%x Seq 0x%02x, Mode 0x%x", chan_en, chan_seq, chan_mode);
  /* Program port control register. */
  if (isPort16) {
    sts =
        pipe_mgr_tof_pktgen_reg_pgr_com_port16_ctrl(sid,
                                                    dev_tgt,
                                                    0xf,  // CRC check disabled
                                                    chan_en,
                                                    chan_mode,
                                                    chan_seq);
  } else {
    sts = pipe_mgr_tof_pktgen_reg_pgr_com_port17_ctrl1_chan(
        sid, dev_tgt, chan_en, chan_mode, chan_seq);
  }
  return sts;
}

bf_status_t pipe_mgr_tof_pktgen_port_rem(rmt_dev_info_t *dev_info,
                                         bf_dev_port_t port_id) {
  bf_dev_id_t dev = dev_info->dev_id;
  bf_dev_pipe_t pipe = dev_info->dev_cfg.dev_port_to_pipe(port_id);
  dev_target_t dev_tgt = {dev, pipe};
  int port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  bf_status_t sts = BF_SUCCESS;
  bool isPort16 = 64 <= port && 67 >= port;
  bool isPort17 = 68 <= port && 71 >= port;
  struct pipe_mgr_tof_pg_dev_ctx *ctx = pipe_mgr_tof_get_pgr_ctx(dev);
  if (!ctx) {
    LOG_ERROR("%s : Failed to get device context, dev %d", __func__, dev);
    return BF_UNEXPECTED;
  }
  int *chanp = isPort16 ? &ctx->port16_chan[pipe][0]
                        : (isPort17 ? &ctx->port17_chan[pipe][0] : NULL);
  if (!chanp) return BF_SUCCESS;

  int chan = port & 3;

  /* Update shadow of port speed config. */
  if (100 == chanp[chan]) {
    PIPE_MGR_DBGCHK(!chan);
    chanp[0] = chanp[1] = chanp[2] = chanp[3] = 0;
  } else if (50 == chanp[chan]) {
    PIPE_MGR_DBGCHK(!(chan & 1));
    chanp[chan] = chanp[chan + 1] = 0;
  } else if (25 == chanp[chan]) {
    chanp[chan] = 0;
  }

  /* Compute the channel enable bit mask to program. */
  uint8_t chan_en = get_chan_en(chanp);
  /* Compute the channel sequence to program. */
  uint8_t chan_seq = get_chan_seq(chanp);
  /* Compute the channel mode to program. */
  uint8_t chan_mode = get_chan_mode(chanp);
  /* Use the default session to set this up. */
  pipe_sess_hdl_t sid = pipe_mgr_ctx->int_ses_hndl;

  /* Program port control register. */
  if (isPort16) {
    sts =
        pipe_mgr_tof_pktgen_reg_pgr_com_port16_ctrl(sid,
                                                    dev_tgt,
                                                    0xf,  // CRC check disabled
                                                    chan_en,
                                                    chan_mode,
                                                    chan_seq);
  } else {
    sts = pipe_mgr_tof_pktgen_reg_pgr_com_port17_ctrl1_chan(
        sid, dev_tgt, chan_en, chan_mode, chan_seq);
  }
  return sts;
}

bf_status_t pipe_mgr_tof_set_recirc_16_en(bf_session_hdl_t shdl,
                                          bf_dev_id_t dev,
                                          bf_dev_port_t port,
                                          bool en) {
  if (!pipe_mgr_valid_session(&shdl, __func__, __LINE__)) return BF_INVALID_ARG;
  if (pipe_mgr_sess_in_txn(shdl)) return BF_TXN_NOT_SUPPORTED;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return BF_INVALID_ARG;
  }
  if (64 != dev_info->dev_cfg.dev_port_to_local_port(port))
    return BF_INVALID_ARG;

  /* We do not allow the recirc enable or cpu enable to be changed while the
   * port exists. */
  int logical_pipe = dev_info->dev_cfg.dev_port_to_pipe(port);
  for (int local_port = 64; local_port < 68; ++local_port) {
    bf_dev_port_t dev_port =
        dev_info->dev_cfg.make_dev_port(logical_pipe, local_port);
    if (pipe_mgr_get_port_info(dev, dev_port)) {
      return BF_IN_USE;
    }
  }

  bf_dev_pipe_t pipe = dev_info->dev_cfg.dev_port_to_pipe(port);

  /* If recirculation is being enabled then disable CPU connectivity.  If
   * recirculation is being disabled then only enable CPU connectivity on
   * legal pipes.  Legal pipes are 0 and 3 in four pipeline chips and 0 and 1
   * in two pipeline chips. */
  uint32_t pipe_cnt = pipe_mgr_get_num_active_pipes(dev);
  bool cpu_en = !en && (2 == pipe_cnt || !pipe || 2 == pipe);
  bf_dev_target_t dev_tgt = {dev, pipe};
  bf_status_t sts = pipe_mgr_tof_pktgen_reg_pgr_com_port16_set_recirc_cpu(
      shdl, dev_tgt, en, cpu_en);
  return sts;
}

bool pipe_mgr_tof_pktgen_reg_pgr_com_port16_get_recirc(
    bf_dev_target_t dev_tgt) {
  return recirc_enable[dev_tgt.device_id][dev_tgt.dev_pipe_id];
}

static bool port_is_pktgenable(bf_dev_id_t dev, bf_dev_port_t p) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) return false;
  bf_dev_pipe_t pipe = dev_info->dev_cfg.dev_port_to_pipe(p);
  if (pipe >= dev_info->num_active_pipes) return false;
  bf_dev_port_t lo = dev_info->dev_cfg.make_dev_port(pipe, 68);
  bf_dev_port_t hi = dev_info->dev_cfg.make_dev_port(pipe, 71);
  return (lo <= p) && (p <= hi);
}

static int pktgen_port_to_chan(bf_dev_id_t dev, bf_dev_port_t p) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return 0;
  }
  PIPE_MGR_DBGCHK(port_is_pktgenable(dev, p));
  return dev_info->dev_cfg.dev_port_to_local_port(p) - 68;
}

static bf_status_t pipe_mgr_tof_pktgen_reg_pgr_com_port17_ctrl2_chan_en(
    pipe_sess_hdl_t sid, bf_dev_target_t dev_tgt, uint8_t chan, bool en) {
  if (dev_tgt.device_id >= PIPE_MGR_NUM_DEVICES || dev_tgt.device_id < 0) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  if (chan >= 4) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  struct pipe_mgr_tof_pg_dev_ctx *ctx =
      pipe_mgr_tof_get_pgr_ctx(dev_tgt.device_id);
  if (!ctx) {
    LOG_ERROR("%s : Failed to get device context, dev %d",
              __func__,
              dev_tgt.device_id);
    return BF_UNEXPECTED;
  }
  uint32_t addr =
      offsetof(Tofino, pipes[0].pmarb.pgr_reg.pgr_common.port17_ctrl2);
  uint32_t x = getp_pgr_port17_ctrl2_pgen_channel_en(
      &ctx->port17_ctrl2[dev_tgt.dev_pipe_id]);
  x &= ~(1u << chan);
  if (en) x |= 1u << chan;
  setp_pgr_port17_ctrl2_pgen_channel_en(&ctx->port17_ctrl2[dev_tgt.dev_pipe_id],
                                        x);
  return pg_write_one_pipe_reg(sid,
                               dev_tgt.device_id,
                               1 << dev_tgt.dev_pipe_id,
                               addr,
                               ctx->port17_ctrl2[dev_tgt.dev_pipe_id]);
}

bf_status_t pipe_mgr_tof_pktgen_set_port_en(bf_session_hdl_t shdl,
                                            bf_dev_id_t dev,
                                            bf_dev_port_t port,
                                            bool en) {
  if (!pipe_mgr_valid_session(&shdl, __func__, __LINE__)) return BF_INVALID_ARG;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return BF_INVALID_ARG;
  }
  if (!port_is_pktgenable(dev, port)) return BF_INVALID_ARG;
  if (pipe_mgr_sess_in_txn(shdl)) return BF_IN_USE;

  /* Convert the port to a pipe and port, then from the port to the channel. */
  bf_dev_target_t dev_tgt = {dev, dev_info->dev_cfg.dev_port_to_pipe(port)};
  int chan = pktgen_port_to_chan(dev, port);

  bf_status_t sts = BF_SUCCESS;
  sts = pipe_mgr_tof_pktgen_reg_pgr_com_port17_ctrl2_chan_en(
      shdl, dev_tgt, chan, en);
  if (sts != BF_SUCCESS) return sts;

  return BF_SUCCESS;
}
bf_status_t pipe_mgr_tof_pktgen_get_port_en(rmt_dev_info_t *dev_info,
                                            bf_dev_port_t port,
                                            bool *en) {
  if (!port_is_pktgenable(dev_info->dev_id, port)) {
    *en = false;
    return BF_SUCCESS;
  }
  struct pipe_mgr_tof_pg_dev_ctx *ctx =
      pipe_mgr_tof_get_pgr_ctx(dev_info->dev_id);
  if (!ctx) {
    LOG_ERROR("%s : Failed to get device context, dev %d",
              __func__,
              dev_info->dev_id);
    return BF_UNEXPECTED;
  }
  bf_dev_pipe_t pipe = dev_info->dev_cfg.dev_port_to_pipe(port);
  int chan = pktgen_port_to_chan(dev_info->dev_id, port);
  uint32_t x = getp_pgr_port17_ctrl2_pgen_channel_en(&ctx->port17_ctrl2[pipe]);
  *en = (x & (1u << chan)) != 0;
  return BF_SUCCESS;
}

static bf_status_t pipe_mgr_tof_pktgen_reg_pgr_com_port17_ctrl2_snoop_en(
    pipe_sess_hdl_t sid, bf_dev_target_t dev_tgt, uint8_t chan, bool en) {
  if (dev_tgt.device_id >= PIPE_MGR_NUM_DEVICES || dev_tgt.device_id < 0) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  if (chan >= 4) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  struct pipe_mgr_tof_pg_dev_ctx *ctx =
      pipe_mgr_tof_get_pgr_ctx(dev_tgt.device_id);
  if (!ctx) {
    LOG_ERROR("%s : Failed to get device context, dev %d",
              __func__,
              dev_tgt.device_id);
    return BF_UNEXPECTED;
  }
  uint32_t addr =
      offsetof(Tofino, pipes[0].pmarb.pgr_reg.pgr_common.port17_ctrl2);
  uint32_t x = getp_pgr_port17_ctrl2_recir_channel_en(
      &ctx->port17_ctrl2[dev_tgt.dev_pipe_id]);
  x &= ~(1u << chan);
  if (en) x |= 1u << chan;
  setp_pgr_port17_ctrl2_recir_channel_en(
      &ctx->port17_ctrl2[dev_tgt.dev_pipe_id], x);
  return pg_write_one_pipe_reg(sid,
                               dev_tgt.device_id,
                               1 << dev_tgt.dev_pipe_id,
                               addr,
                               ctx->port17_ctrl2[dev_tgt.dev_pipe_id]);
}

bf_status_t pipe_mgr_tof_pktgen_set_rpm_en(bf_session_hdl_t shdl,
                                           bf_dev_id_t dev,
                                           bf_dev_port_t port,
                                           bool en) {
  if (!pipe_mgr_valid_session(&shdl, __func__, __LINE__)) return BF_INVALID_ARG;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return BF_INVALID_ARG;
  }
  if (!port_is_pktgenable(dev, port)) return BF_INVALID_ARG;
  if (pipe_mgr_sess_in_txn(shdl)) return BF_IN_USE;

  /* Convert the port to a pipe and port, then from the port to the channel. */
  bf_dev_target_t dev_tgt = {dev, dev_info->dev_cfg.dev_port_to_pipe(port)};
  int chan = pktgen_port_to_chan(dev, port);

  bf_status_t sts = BF_SUCCESS;
  sts = pipe_mgr_tof_pktgen_reg_pgr_com_port17_ctrl2_snoop_en(
      shdl, dev_tgt, chan, en);
  if (sts != BF_SUCCESS) return sts;

  return BF_SUCCESS;
}

bf_status_t pipe_mgr_tof_pktgen_get_rpm_en(rmt_dev_info_t *dev_info,
                                           bf_dev_port_t port,
                                           bool *en) {
  if (!port_is_pktgenable(dev_info->dev_id, port)) {
    *en = false;
    return BF_SUCCESS;
  }
  bf_dev_pipe_t pipe = dev_info->dev_cfg.dev_port_to_pipe(port);
  int chan = pktgen_port_to_chan(dev_info->dev_id, port);
  if (dev_info->dev_id >= PIPE_MGR_NUM_DEVICES || chan >= 4)
    return BF_INVALID_ARG;
  struct pipe_mgr_tof_pg_dev_ctx *ctx =
      pipe_mgr_tof_get_pgr_ctx(dev_info->dev_id);
  if (!ctx) {
    LOG_ERROR("%s : Failed to get device context, dev %d",
              __func__,
              dev_info->dev_id);
    return BF_UNEXPECTED;
  }
  uint32_t x = getp_pgr_port17_ctrl2_recir_channel_en(&ctx->port17_ctrl2[pipe]);
  *en = (x & (1u << chan)) != 0;
  return BF_SUCCESS;
}

bf_status_t pipe_mgr_tof_pktgen_pgr_com_port_down_clr_get(
    bf_dev_target_t dev_tgt, int local_port_bit_idx, bool *is_cleared) {
  uint32_t addr = get_port_down_dis_addr(local_port_bit_idx);
  uint32_t data = 0;

  bf_status_t sts = pg_tof_read_one_pipe_reg(dev_tgt, addr, &data);
  if (BF_SUCCESS != sts) return BF_HW_COMM_FAIL;
  *is_cleared = ((data >> (local_port_bit_idx % 32)) & 0x1) != 0x1;
  return BF_SUCCESS;
}

bf_status_t pipe_mgr_tof_pktgen_pgr_com_port_down_clr(pipe_sess_hdl_t sid,
                                                      bf_dev_target_t dev_tgt,
                                                      int local_port_bit_idx) {
  uint32_t data = 1u << (local_port_bit_idx % 32);
  uint32_t addr = get_port_down_dis_addr(local_port_bit_idx);
  return pg_write_one_pipe_reg(
      sid, dev_tgt.device_id, 1 << dev_tgt.dev_pipe_id, addr, data);
}

static void pipe_mgr_tof_pg_txn_bkup_app_cfg(pipe_sess_hdl_t shdl,
                                             bf_dev_id_t dev) {
  bool txn = pipe_mgr_sess_in_txn(shdl);
  struct pipe_mgr_tof_pg_dev_ctx *ctx = pipe_mgr_tof_get_pgr_ctx(dev);
  if (!ctx) {
    LOG_ERROR("%s : Failed to get device context, dev %d", __func__, dev);
    return;
  }
  if (!txn || ctx->app.b_valid) return;
  uint32_t p, i;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    LOG_ERROR("%s : Failed to get device info, dev %d ", __func__, dev);
    return;
  }
  ctx->app.b_valid = true;
  for (p = 0; p < dev_info->num_active_pipes; ++p) {
    for (i = 0; i < PIPE_MGR_TOF_PKTGEN_APP_CNT; ++i) {
      PIPE_MGR_MEMCPY(
          &ctx->app.b[p][i], &ctx->app.a[p][i], sizeof ctx->app.b[p][i]);
    }
  }
}

bf_status_t pipe_mgr_tof_pktgen_set_app_ctrl_en(pipe_sess_hdl_t sid,
                                                bf_dev_target_t dev_tgt,
                                                int aid,
                                                bool en) {
  int i, pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t vals[BF_PIPE_COUNT] = {0}, val = 0;
  int val_cnt = 0;
  struct pipe_mgr_tof_pg_dev_ctx *ctx =
      pipe_mgr_tof_get_pgr_ctx(dev_tgt.device_id);
  if (!ctx) {
    LOG_ERROR("%s : Failed to get device context, dev %d",
              __func__,
              dev_tgt.device_id);
    return BF_UNEXPECTED;
  }
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1 << i))) continue;
    vals[val_cnt] = ctx->app.a[i][aid].app_ctrl;
    setp_pgr_app_ctrl_app_en(&vals[val_cnt], en);
    val = vals[val_cnt++];
  }
  for (i = 0; i < val_cnt; ++i) {
    if (val != vals[i]) {
      LOG_ERROR(
          "Error, cannot program asymmetric data to multiple pipes. "
          "%#x != %#x dev %d pipe %#x at %s:%d",
          val,
          vals[i],
          dev_tgt.device_id,
          dev_tgt.dev_pipe_id,
          __func__,
          __LINE__);
      return BF_INVALID_ARG;
    }
  }
  pipe_mgr_tof_pg_txn_bkup_app_cfg(sid, dev_tgt.device_id);
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1 << i))) continue;
    ctx->app.a[i][aid].app_ctrl = val;
  }
  uint32_t addr = offsetof(Tofino, pipes[0].pmarb.pgr_reg.pgr_app[aid].ctrl);
  return pg_write_one_pipe_reg(sid, dev_tgt.device_id, pipe_mask, addr, val);
}

bf_status_t pipe_mgr_tof_pktgen_get_app_ctrl_en(bf_dev_target_t dev_tgt,
                                                int app_id,
                                                bool *is_enabled) {
  int i = (dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL) ? 0 : dev_tgt.dev_pipe_id;
  struct pipe_mgr_tof_pg_dev_ctx *ctx =
      pipe_mgr_tof_get_pgr_ctx(dev_tgt.device_id);
  if (!ctx) {
    LOG_ERROR("%s : Failed to get device context, dev %d",
              __func__,
              dev_tgt.device_id);
    return BF_UNEXPECTED;
  }
  uint32_t val = ctx->app.a[i][app_id].app_ctrl;
  uint32_t en_tmp = getp_pgr_app_ctrl_app_en(&val);
  *is_enabled = en_tmp != 0;
  return BF_SUCCESS;
}

void pipe_mgr_tof_pktgen_txn_commit(bf_dev_id_t dev) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    LOG_ERROR("%s : Failed to get device info, dev %d ", __func__, dev);
    return;
  }
  struct pipe_mgr_tof_pg_dev_ctx *ctx = pipe_mgr_tof_get_pgr_ctx(dev);
  if (!ctx) {
    LOG_ERROR("%s : Failed to get device context, dev %d", __func__, dev);
    return;
  }
  ctx->app.b_valid = false;
  for (uint32_t i = 0; i < dev_info->num_active_pipes; ++i) {
    ctx->pkt_buffer_shadow[i].txn_data_valid = false;
  }
  return;
}

void pipe_mgr_tof_pktgen_txn_abort(bf_dev_id_t dev,
                                   int max_app_id,
                                   int active_pipes) {
  int p, i;
  const uint32_t buf_sz = PIPE_MGR_PKT_BUFFER_SIZE;
  struct pkt_buffer_shadow_t *pkt_buf_shadow = NULL;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    LOG_ERROR("%s : Failed to get device info, dev %d ", __func__, dev);
    return;
  }
  struct pipe_mgr_tof_pg_dev_ctx *ctx = pipe_mgr_tof_get_pgr_ctx(dev);
  if (!ctx) {
    LOG_ERROR("%s : Failed to get device context, dev %d", __func__, dev);
    return;
  }
  if (ctx->app.b_valid) {
    ctx->app.b_valid = false;
    for (p = 0; p < (int)dev_info->num_active_pipes; ++p) {
      for (i = 0; i < max_app_id; ++i) {
        PIPE_MGR_MEMCPY(
            &ctx->app.a[p][i], &ctx->app.b[p][i], sizeof ctx->app.b[p][i]);
      }
    }
  }

  for (i = 0; i < active_pipes; ++i) {
    pkt_buf_shadow = &ctx->pkt_buffer_shadow[i];
    if (!pkt_buf_shadow->txn_data_valid) continue;
    pkt_buf_shadow->txn_data_valid = false;
    PIPE_MGR_MEMCPY(pkt_buf_shadow->data, pkt_buf_shadow->txn_data, buf_sz);
  }
}

void pipe_mgr_tof_pkt_buffer_shadow_mem_update(bf_dev_target_t dev_tgt,
                                               uint32_t offset,
                                               const uint8_t *buf,
                                               uint32_t size,
                                               bool txn) {
  int pipe_mask = pg_log_pipe_mask(dev_tgt);
  const uint32_t buf_sz = PIPE_MGR_PKT_BUFFER_SIZE;
  struct pkt_buffer_shadow_t *pkt_buf_shadow = NULL;
  struct pipe_mgr_tof_pg_dev_ctx *ctx =
      pipe_mgr_tof_get_pgr_ctx(dev_tgt.device_id);
  if (!ctx) {
    LOG_ERROR("%s : Failed to get device context, dev %d",
              __func__,
              dev_tgt.device_id);
    return;
  }
  for (int i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1 << i))) continue;
    pkt_buf_shadow = &ctx->pkt_buffer_shadow[i];
    if (txn && !pkt_buf_shadow->txn_data_valid) {
      pkt_buf_shadow->txn_data_valid = true;
      PIPE_MGR_MEMCPY(pkt_buf_shadow->txn_data, pkt_buf_shadow->data, buf_sz);
    }
    PIPE_MGR_MEMCPY(&(pkt_buf_shadow->data[offset]), buf, size);
  }
}

bf_status_t pipe_mgr_tof_pkt_buffer_shadow_mem_get(bf_dev_target_t dev_tgt,
                                                   uint32_t offset,
                                                   uint8_t *buf,
                                                   uint32_t size) {
  int i = (dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL) ? 0 : dev_tgt.dev_pipe_id;
  struct pipe_mgr_tof_pg_dev_ctx *ctx =
      pipe_mgr_tof_get_pgr_ctx(dev_tgt.device_id);
  if (!ctx) {
    LOG_ERROR("%s : Failed to get device context, dev %d",
              __func__,
              dev_tgt.device_id);
    return BF_UNEXPECTED;
  }
  PIPE_MGR_MEMCPY(buf, &(ctx->pkt_buffer_shadow[i].data[offset]), size);
  return BF_SUCCESS;
}

bf_status_t pipe_mgr_tof_pkt_buffer_write_from_shadow(bf_session_hdl_t shdl,
                                                      bf_dev_target_t dev_tgt) {
  uint64_t addr = 0;
  uint32_t buf_sz = 0, size = 0, elem_size = 0, count = 0;
  bf_status_t sts = BF_SUCCESS;
  pipe_mgr_drv_ses_state_t *st;
  st = pipe_mgr_drv_get_ses_state(&shdl, __func__, __LINE__);
  if (!st) {
    return PIPE_INVALID_ARG;
  }
  struct pipe_mgr_tof_pg_dev_ctx *ctx =
      pipe_mgr_tof_get_pgr_ctx(dev_tgt.device_id);
  if (!ctx) {
    LOG_ERROR("%s : Failed to get device context, dev %d",
              __func__,
              dev_tgt.device_id);
    return BF_UNEXPECTED;
  }
  /* Form the address using the physical pipe rather than the logical pipe. */
  int log_pipe_msk = pg_log_pipe_mask(dev_tgt);
  sts = pipe_mgr_pkt_buffer_tof_addr_get(&addr, &elem_size, &count);
  /* Write the entire 16K pkt buffer */
  size = elem_size * count;
  if (sts != BF_SUCCESS) {
    return sts;
  }
  PIPE_MGR_DBGCHK(size == PIPE_MGR_PKT_BUFFER_SIZE);
  buf_sz = pipe_mgr_drv_buf_size(dev_tgt.device_id, PIPE_MGR_DRV_BUF_BWR);
  PIPE_MGR_DBGCHK(size <= buf_sz);
  /* Since each pipe might have different buffer contents write them
   * individually. */
  int i;
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(log_pipe_msk & (1 << i))) continue;
    pipe_mgr_drv_buf_t *b = pipe_mgr_drv_buf_alloc(
        st->sid, dev_tgt.device_id, buf_sz, PIPE_MGR_DRV_BUF_BWR, true);
    if (!b) {
      LOG_ERROR(
          "%s : Out of memory blocks, dev %d ", __func__, dev_tgt.device_id);
      return PIPE_NO_SYS_RESOURCES;
    }
    PIPE_MGR_MEMCPY(
        b->addr, &(ctx->pkt_buffer_shadow[dev_tgt.dev_pipe_id].data), size);

    pipe_status_t s = pipe_mgr_drv_blk_wr(&shdl,
                                          PIPE_MGR_PKT_BUFFER_WIDTH,
                                          PIPE_MGR_PKT_BUFFER_MEM_ROWS,
                                          1,
                                          addr,
                                          1 << i,
                                          b);
    if (PIPE_SUCCESS != s) {
      LOG_ERROR("Packet Generator buffer udpate fails (%s) dev %d log_pipe %#x",
                pipe_str_err(s),
                dev_tgt.device_id,
                dev_tgt.dev_pipe_id);
      return BF_HW_COMM_FAIL;
    }
  }
  return BF_SUCCESS;
}

static bf_status_t pipe_mgr_tof_pktgen_reg_write_mem_with_ilist(
    bf_session_hdl_t shdl,
    bf_dev_target_t dev_tgt,
    int row,
    int cnt,
    uint64_t base,
    uint64_t step) {
  pipe_bitmap_t pbm = {{0}};
  PIPE_BITMAP_INIT(&pbm, PIPE_BMP_SIZE);
  uint32_t pipe_cnt = pipe_mgr_get_num_active_pipes(dev_tgt.device_id);
  for (uint32_t p = 0; p < pipe_cnt; ++p) {
    if (BF_DEV_PIPE_ALL == dev_tgt.dev_pipe_id || dev_tgt.dev_pipe_id == p) {
      PIPE_BITMAP_SET(&pbm, p);
    }
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_tgt.device_id);
  if (dev_info == NULL) {
    LOG_ERROR("%s:%d Device info for device id %d not found",
              __func__,
              __LINE__,
              dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  struct pipe_mgr_tof_pg_dev_ctx *ctx =
      pipe_mgr_tof_get_pgr_ctx(dev_tgt.device_id);
  if (!ctx) {
    LOG_ERROR("%s : Failed to get device context, dev %d",
              __func__,
              dev_tgt.device_id);
    return BF_UNEXPECTED;
  }
  /* Reset pipe so that it can be used to index the shadow. */
  if (BF_DEV_PIPE_ALL == dev_tgt.dev_pipe_id) dev_tgt.dev_pipe_id = 0;

  uint32_t stage;
  lld_err_t lld_err = lld_sku_get_prsr_stage(dev_tgt.device_id, &stage);
  if (LLD_OK != lld_err) {
    LOG_ERROR("Cannot get pgen stage, error %d, from %s", lld_err, __func__);
    PIPE_MGR_DBGCHK(LLD_OK == lld_err);
    return PIPE_UNEXPECTED;
  }

  uint64_t addr = base + step * row;
  pipe_instr_common_wd0_t instr = {0};
  instr.pipe_ring_addr_type = 2;
  instr.data_width = 3;
  instr.specific = (uint32_t)addr & 0x1FFFFF;
  for (int i = 0; i < cnt; ++i) {
    pipe_status_t sts =
        pipe_mgr_drv_ilist_add_2(&shdl,
                                 dev_info,
                                 &pbm,
                                 stage,
                                 (uint8_t *)&instr,
                                 sizeof instr,
                                 &ctx->pkt_buffer_shadow[dev_tgt.dev_pipe_id]
                                      .data[row * PIPE_MGR_PKT_BUFFER_WIDTH],
                                 PIPE_MGR_PKT_BUFFER_WIDTH);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR(
          "Failed to post instruction to update pkt-gen buffer on dev %d, pipe "
          "%x, err %s",
          dev_tgt.device_id,
          dev_tgt.dev_pipe_id,
          pipe_str_err(sts));
      return BF_HW_COMM_FAIL;
    }
    ++row;
    instr.specific += step;
  }
  return BF_SUCCESS;
}

bf_status_t pipe_mgr_tof_pktgen_write_pkt_buffer(bf_session_hdl_t shdl,
                                                 bf_dev_target_t dev_tgt,
                                                 int row,
                                                 int num_rows) {
  uint32_t elem_size = 0, count = 0;
  uint64_t addr = 0;
  bf_status_t sts = pipe_mgr_pkt_buffer_tof_addr_get(&addr, &elem_size, &count);
  /* Write the data to asic from shadow mem */
  sts |= pipe_mgr_tof_pktgen_reg_write_mem_with_ilist(
      shdl, dev_tgt, row, num_rows, addr, elem_size / 16);

  return sts;
}

bf_status_t pipe_mgr_tof_pktgen_reg_app_batch_ctr(bf_dev_target_t dev_tgt,
                                                  int aid,
                                                  uint64_t *val) {
  bf_status_t sts = BF_SUCCESS;
  uint32_t hi = 0, lo = 0;

  uint32_t addrLo = offsetof(
      Tofino, pipes[0].pmarb.pgr_reg.pgr_app[aid].ctr48_batch.ctr48_batch_0_2);
  uint32_t addrHi = offsetof(
      Tofino, pipes[0].pmarb.pgr_reg.pgr_app[aid].ctr48_batch.ctr48_batch_1_2);
  sts = pg_tof_read_one_pipe_reg(dev_tgt, addrLo, &lo);
  if (BF_SUCCESS != sts) return BF_HW_COMM_FAIL;
  sts = pg_tof_read_one_pipe_reg(dev_tgt, addrHi, &hi);
  if (BF_SUCCESS != sts) return BF_HW_COMM_FAIL;

  uint64_t x = hi;
  uint64_t y = lo;
  *val = (x << 32) | (y & UINT64_C(0xFFFFFFFF));

  return BF_SUCCESS;
}

bf_status_t pipe_mgr_tof_pktgen_reg_app_pkt_ctr(bf_dev_target_t dev_tgt,
                                                int aid,
                                                uint64_t *val) {
  bf_status_t sts = BF_SUCCESS;
  uint32_t hi = 0, lo = 0;
  uint32_t addrLo = offsetof(
      Tofino,
      pipes[0].pmarb.pgr_reg.pgr_app[aid].ctr48_packet.ctr48_packet_0_2);
  uint32_t addrHi = offsetof(
      Tofino,
      pipes[0].pmarb.pgr_reg.pgr_app[aid].ctr48_packet.ctr48_packet_1_2);
  sts = pg_tof_read_one_pipe_reg(dev_tgt, addrLo, &lo);
  if (BF_SUCCESS != sts) return BF_HW_COMM_FAIL;
  sts = pg_tof_read_one_pipe_reg(dev_tgt, addrHi, &hi);
  if (BF_SUCCESS != sts) return BF_HW_COMM_FAIL;

  uint64_t x = hi;
  uint64_t y = lo;
  *val = (x << 32) | (y & UINT64_C(0xFFFFFFFF));
  return BF_SUCCESS;
}

bf_status_t pipe_mgr_tof_pktgen_reg_app_trig_ctr(bf_dev_target_t dev_tgt,
                                                 int aid,
                                                 uint64_t *val) {
  bf_status_t sts = BF_SUCCESS;
  uint32_t hi = 0, lo = 0;

  uint32_t addrLo = offsetof(
      Tofino,
      pipes[0].pmarb.pgr_reg.pgr_app[aid].ctr48_trigger.ctr48_trigger_0_2);
  uint32_t addrHi = offsetof(
      Tofino,
      pipes[0].pmarb.pgr_reg.pgr_app[aid].ctr48_trigger.ctr48_trigger_1_2);
  sts = pg_tof_read_one_pipe_reg(dev_tgt, addrLo, &lo);
  if (BF_SUCCESS != sts) return BF_HW_COMM_FAIL;
  sts = pg_tof_read_one_pipe_reg(dev_tgt, addrHi, &hi);
  if (BF_SUCCESS != sts) return BF_HW_COMM_FAIL;

  uint64_t x = hi;
  uint64_t y = lo;
  *val = (x << 32) | (y & UINT64_C(0xFFFFFFFF));
  return BF_SUCCESS;
}

bf_status_t pipe_mgr_tof_pktgen_reg_app_batch_ctr_set(bf_session_hdl_t shdl,
                                                      bf_dev_target_t dev_tgt,
                                                      int aid,
                                                      uint64_t val) {
  bf_status_t sts = BF_SUCCESS;
  uint32_t hi = val >> 32;
  uint32_t lo = val & UINT64_C(0xFFFFFFFF);
  int pipe_mask = pg_log_pipe_mask(dev_tgt);

  uint32_t addrLo = offsetof(
      Tofino, pipes[0].pmarb.pgr_reg.pgr_app[aid].ctr48_batch.ctr48_batch_0_2);
  uint32_t addrHi = offsetof(
      Tofino, pipes[0].pmarb.pgr_reg.pgr_app[aid].ctr48_batch.ctr48_batch_1_2);
  sts = pg_write_one_pipe_reg(shdl, dev_tgt.device_id, pipe_mask, addrHi, hi);
  if (BF_SUCCESS != sts) return BF_HW_COMM_FAIL;
  sts = pg_write_one_pipe_reg(shdl, dev_tgt.device_id, pipe_mask, addrLo, lo);
  if (BF_SUCCESS != sts) return BF_HW_COMM_FAIL;
  return BF_SUCCESS;
}

bf_status_t pipe_mgr_tof_pktgen_reg_app_pkt_ctr_set(bf_session_hdl_t shdl,
                                                    bf_dev_target_t dev_tgt,
                                                    int aid,
                                                    uint64_t val) {
  bf_status_t sts = BF_SUCCESS;
  uint32_t hi = val >> 32;
  uint32_t lo = val & UINT64_C(0xFFFFFFFF);
  int pipe_mask = pg_log_pipe_mask(dev_tgt);

  uint32_t addrLo = offsetof(
      Tofino,
      pipes[0].pmarb.pgr_reg.pgr_app[aid].ctr48_packet.ctr48_packet_0_2);
  uint32_t addrHi = offsetof(
      Tofino,
      pipes[0].pmarb.pgr_reg.pgr_app[aid].ctr48_packet.ctr48_packet_1_2);
  sts = pg_write_one_pipe_reg(shdl, dev_tgt.device_id, pipe_mask, addrHi, hi);
  if (BF_SUCCESS != sts) return BF_HW_COMM_FAIL;
  sts = pg_write_one_pipe_reg(shdl, dev_tgt.device_id, pipe_mask, addrLo, lo);
  if (BF_SUCCESS != sts) return BF_HW_COMM_FAIL;
  return BF_SUCCESS;
}

bf_status_t pipe_mgr_tof_pktgen_reg_app_trig_ctr_set(bf_session_hdl_t shdl,
                                                     bf_dev_target_t dev_tgt,
                                                     int aid,
                                                     uint64_t val) {
  bf_status_t sts = BF_SUCCESS;
  uint32_t hi = val >> 32;
  uint32_t lo = val & UINT64_C(0xFFFFFFFF);
  int pipe_mask = pg_log_pipe_mask(dev_tgt);

  uint32_t addrLo = offsetof(
      Tofino,
      pipes[0].pmarb.pgr_reg.pgr_app[aid].ctr48_trigger.ctr48_trigger_0_2);
  uint32_t addrHi = offsetof(
      Tofino,
      pipes[0].pmarb.pgr_reg.pgr_app[aid].ctr48_trigger.ctr48_trigger_1_2);
  sts = pg_write_one_pipe_reg(shdl, dev_tgt.device_id, pipe_mask, addrHi, hi);
  if (BF_SUCCESS != sts) return BF_HW_COMM_FAIL;
  sts = pg_write_one_pipe_reg(shdl, dev_tgt.device_id, pipe_mask, addrLo, lo);
  if (BF_SUCCESS != sts) return BF_HW_COMM_FAIL;
  return BF_SUCCESS;
}

static uint32_t pipe_mgr_tof_pktgen_app_ctrl_type(bf_dev_target_t dev_tgt,
                                                  int aid) {
  struct pipe_mgr_tof_pg_dev_ctx *ctx =
      pipe_mgr_tof_get_pgr_ctx(dev_tgt.device_id);
  if (!ctx) {
    LOG_ERROR("%s : Failed to get device context, dev %d",
              __func__,
              dev_tgt.device_id);
    return 0;
  }
  return getp_pgr_app_ctrl_app_type(
      &ctx->app.a[dev_tgt.dev_pipe_id][aid].app_ctrl);
}
static uint32_t pipe_mgr_tof_pktgen_app_ctrl_en(bf_dev_target_t dev_tgt,
                                                int aid) {
  struct pipe_mgr_tof_pg_dev_ctx *ctx =
      pipe_mgr_tof_get_pgr_ctx(dev_tgt.device_id);
  if (!ctx) {
    LOG_ERROR("%s : Failed to get device context, dev %d",
              __func__,
              dev_tgt.device_id);
    return 0;
  }
  return getp_pgr_app_ctrl_app_en(
      &ctx->app.a[dev_tgt.dev_pipe_id][aid].app_ctrl);
}

bf_status_t pipe_mgr_tof_pktgen_cfg_app_conf_check(rmt_dev_info_t *dev_info,
                                                   bf_dev_target_t dev_tgt,
                                                   int app_id,
                                                   bf_pktgen_app_cfg_t *cfg) {
  bf_dev_id_t dev = dev_tgt.device_id;
  /* Cannot change trigger_type while the app is enabled. */
  if (BF_DEV_PIPE_ALL == dev_tgt.dev_pipe_id) {
    uint32_t i, a = pipe_mgr_get_num_active_pipes(dev);
    for (i = 0; i < a; ++i) {
      dev_tgt.dev_pipe_id = i;
      uint32_t x = pipe_mgr_tof_pktgen_app_ctrl_type(dev_tgt, app_id);
      uint32_t y = pipe_mgr_tof_pktgen_app_ctrl_en(dev_tgt, app_id);
      if (y && x != cfg->trigger_type) {
        LOG_ERROR("%s:%d Application in use, dev %d, app_id %d",
                  __func__,
                  __LINE__,
                  dev,
                  app_id);
        return BF_IN_USE;
      }
    }
  } else {
    uint32_t x = pipe_mgr_tof_pktgen_app_ctrl_type(dev_tgt, app_id);
    uint32_t y = pipe_mgr_tof_pktgen_app_ctrl_en(dev_tgt, app_id);
    if (y && x != cfg->trigger_type) {
      LOG_ERROR("%s:%d Application in use, dev %d, app_id %d",
                __func__,
                __LINE__,
                dev,
                app_id);
      return BF_IN_USE;
    }
  }
  if (app_id >= PIPE_MGR_TOF_PKTGEN_APP_CNT) {
    LOG_ERROR("%s:%d Invalid application id, dev %d, app_id %d",
              __func__,
              __LINE__,
              dev,
              app_id);
    return BF_INVALID_ARG;
  }
  /* Cannot include pipe id in source port, we will set it. */
  if (0 != dev_info->dev_cfg.dev_port_to_pipe(cfg->pipe_local_source_port)) {
    LOG_ERROR(
        "%s:%d Invalid source port, dev %d, app_id %d, source port 0x%x, must "
        "not include pipe id in source port",
        __func__,
        __LINE__,
        dev,
        app_id,
        cfg->pipe_local_source_port);
    return BF_INVALID_ARG;
  }
  /* Cannot set illegal source ports. */
  if (cfg->pipe_local_source_port > PIPE_MGR_PKTGEN_SRC_PRT_MAX) {
    LOG_ERROR("%s:%d Invalid source port, dev %d, app_id %d, source port 0x%x",
              __func__,
              __LINE__,
              dev,
              app_id,
              cfg->pipe_local_source_port);
    return BF_INVALID_ARG;
  }
  /* Cannot increment the source port to an illegal value. */
  if (cfg->increment_source_port &&
      cfg->packets_per_batch >
          (PIPE_MGR_PKTGEN_SRC_PRT_MAX - cfg->pipe_local_source_port)) {
    LOG_ERROR(
        "%s:%d Invalid increment port, dev %d, app_id %d, increment "
        "source port 0x%x, source port 0x%x",
        __func__,
        __LINE__,
        dev,
        app_id,
        cfg->increment_source_port,
        cfg->pipe_local_source_port);
    return BF_INVALID_ARG;
  }
  /* If the trigger type is port down, then only one batch is usable as
   * the batch id in the pkt-gen header carrys the port number. */
  if (cfg->batch_count != 0 &&
      cfg->trigger_type == BF_PKTGEN_TRIGGER_PORT_DOWN) {
    LOG_ERROR(
        "%s:%d Port-Down triggers must have a batch size of 0 (single batch)",
        __func__,
        __LINE__);
    return BF_INVALID_ARG;
  }

  /* Buffer offset must be 16 byte aligned. */
  if (cfg->pkt_buffer_offset & 0xF) {
    LOG_ERROR("%s:%d Packet buffer offset (0x%x) must be 16B aligned",
              __func__,
              __LINE__,
              cfg->pkt_buffer_offset);
    return BF_INVALID_ARG;
  }

  /* Hardware will add 6 bytes of pgen header and 4 bytes of CRC to packet
   * length. */
  if (cfg->length + 6 + 4 < PIPE_MGR_TOF_PKTGEN_PKT_LEN_MIN) {
    LOG_ERROR("%s:%d Packet length, %d, is too small.  Must be at least %d",
              __func__,
              __LINE__,
              cfg->length,
              PIPE_MGR_TOF_PKTGEN_PKT_LEN_MIN - 6 - 4);
    return BF_INVALID_ARG;
  }
  return BF_SUCCESS;
}

static bf_status_t pipe_mgr_tof_pktgen_reg_pgr_app_ctrl(pipe_sess_hdl_t sid,
                                                        bf_dev_target_t dev_tgt,
                                                        int aid,
                                                        int type) {
  LOG_TRACE("PktGenAppCfg: dev %d pipe %d app %d shdl %d, %s",
            dev_tgt.device_id,
            dev_tgt.dev_pipe_id,
            aid,
            sid,
            type == BF_PKTGEN_TRIGGER_TIMER_ONE_SHOT
                ? "One-Shot Timer"
                : type == BF_PKTGEN_TRIGGER_TIMER_PERIODIC
                      ? "Periodic Timer"
                      : type == BF_PKTGEN_TRIGGER_PORT_DOWN
                            ? "Port Down"
                            : type == BF_PKTGEN_TRIGGER_RECIRC_PATTERN
                                  ? "Recirc"
                                  : type == BF_PKTGEN_TRIGGER_DPRSR
                                        ? "dprsr"
                                        : type == BF_PKTGEN_TRIGGER_PFC
                                              ? "PFC"
                                              : "Unknown");
  int i, pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t vals[BF_PIPE_COUNT] = {0}, val = 0;
  int val_cnt = 0;
  struct pipe_mgr_tof_pg_dev_ctx *ctx =
      pipe_mgr_tof_get_pgr_ctx(dev_tgt.device_id);
  if (!ctx) {
    LOG_ERROR("%s : Failed to get device context, dev %d",
              __func__,
              dev_tgt.device_id);
    return BF_UNEXPECTED;
  }
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1 << i))) continue;
    vals[val_cnt] = ctx->app.a[i][aid].app_ctrl;
    setp_pgr_app_ctrl_app_type(&vals[val_cnt], type);
    val = vals[val_cnt++];
  }
  for (i = 0; i < val_cnt; ++i) {
    if (val != vals[i]) {
      LOG_ERROR(
          "Error, cannot program asymmetric data to multiple pipes. "
          "%#x != %#x dev %d pipe %#x at %s:%d",
          val,
          vals[i],
          dev_tgt.device_id,
          dev_tgt.dev_pipe_id,
          __func__,
          __LINE__);
      return BF_INVALID_ARG;
    }
  }
  pipe_mgr_tof_pg_txn_bkup_app_cfg(sid, dev_tgt.device_id);
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1 << i))) continue;
    ctx->app.a[i][aid].app_ctrl = val;
  }
  uint32_t addr = offsetof(Tofino, pipes[0].pmarb.pgr_reg.pgr_app[aid].ctrl);
  return pg_write_one_pipe_reg(sid, dev_tgt.device_id, pipe_mask, addr, val);
}

static bf_status_t pipe_mgr_tof_pktgen_reg_pgr_app_payload_ctrl(
    pipe_sess_hdl_t sid,
    bf_dev_target_t dev_tgt,
    int aid,
    uint16_t start,
    uint16_t size) {
  LOG_TRACE(
      "PktGenAppCfg: dev %d pipe %d app %d shdl %d, Payload offset/size %#x "
      "%#x",
      dev_tgt.device_id,
      dev_tgt.dev_pipe_id,
      aid,
      sid,
      start,
      size);
  int i, pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t vals[BF_PIPE_COUNT] = {0}, val = 0;
  int val_cnt = 0;
  struct pipe_mgr_tof_pg_dev_ctx *ctx =
      pipe_mgr_tof_get_pgr_ctx(dev_tgt.device_id);
  if (!ctx) {
    LOG_ERROR("%s : Failed to get device context, dev %d",
              __func__,
              dev_tgt.device_id);
    return BF_UNEXPECTED;
  }
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1 << i))) continue;
    vals[val_cnt] = ctx->app.a[i][aid].payload_ctrl;
    setp_pgr_app_payload_ctrl_app_payload_size(&vals[val_cnt], size);
    setp_pgr_app_payload_ctrl_app_payload_addr(&vals[val_cnt], start);
    val = vals[val_cnt++];
  }
  for (i = 0; i < val_cnt; ++i) {
    if (val != vals[i]) {
      LOG_ERROR(
          "Error, cannot program asymmetric data to multiple pipes. "
          "%#x != %#x dev %d pipe %#x at %s:%d",
          val,
          vals[i],
          dev_tgt.device_id,
          dev_tgt.dev_pipe_id,
          __func__,
          __LINE__);
      return BF_INVALID_ARG;
    }
  }
  pipe_mgr_tof_pg_txn_bkup_app_cfg(sid, dev_tgt.device_id);
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1 << i))) continue;
    ctx->app.a[i][aid].payload_ctrl = val;
  }
  uint32_t addr =
      offsetof(Tofino, pipes[0].pmarb.pgr_reg.pgr_app[aid].payload_ctrl);
  return pg_write_one_pipe_reg(sid, dev_tgt.device_id, pipe_mask, addr, val);
}

static bf_status_t pipe_mgr_tof_pktgen_reg_pgr_app_ing_port_ctrl(
    pipe_sess_hdl_t sid,
    bf_dev_target_t dev_tgt,
    int aid,
    uint16_t port,
    bool inc) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_tgt.device_id);
  if (!dev_info) return BF_INVALID_ARG;
  LOG_TRACE("PktGenAppCfg: dev %d pipe %d app %d shdl %d, Port %d inc %d",
            dev_info->dev_id,
            dev_tgt.dev_pipe_id,
            aid,
            sid,
            port,
            inc);
  int i, pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t vals[BF_PIPE_COUNT] = {0};
  int val_cnt = 0;
  struct pipe_mgr_tof_pg_dev_ctx *ctx =
      pipe_mgr_tof_get_pgr_ctx(dev_tgt.device_id);
  if (!ctx) {
    LOG_ERROR("%s : Failed to get device context, dev %d",
              __func__,
              dev_tgt.device_id);
    return BF_UNEXPECTED;
  }
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1 << i))) continue;
    vals[val_cnt] = ctx->app.a[i][aid].ing_port;
    setp_pgr_app_ingr_port_ctrl_app_ingr_port(
        &vals[val_cnt], dev_info->dev_cfg.make_dev_port(i, port));
    setp_pgr_app_ingr_port_ctrl_app_ingr_port_inc(&vals[val_cnt], inc);
    val_cnt++;
  }
  /* No check for symmetric data since the port numbers MUST be dfifernt for
   * each pipe. */
  pipe_mgr_tof_pg_txn_bkup_app_cfg(sid, dev_tgt.device_id);
  bf_status_t sts = BF_SUCCESS;
  val_cnt = 0;
  uint32_t addr =
      offsetof(Tofino, pipes[0].pmarb.pgr_reg.pgr_app[aid].ingr_port_ctrl);
  for (i = 0; i < BF_PIPE_COUNT && BF_SUCCESS == sts; ++i) {
    if (!(pipe_mask & (1 << i))) continue;
    sts = pg_write_one_pipe_reg(
        sid, dev_tgt.device_id, 1 << i, addr, vals[val_cnt++]);  // TODO
  }
  val_cnt = 0;
  for (i = 0; i < BF_PIPE_COUNT && BF_SUCCESS == sts; ++i) {
    if (!(pipe_mask & (1 << i))) continue;
    ctx->app.a[i][aid].ing_port = vals[val_cnt++];
  }
  return sts;
}

static bf_status_t pipe_mgr_tof_pktgen_reg_pgr_app_recir_match_value(
    pipe_sess_hdl_t sid, bf_dev_target_t dev_tgt, int aid, uint32_t key) {
  LOG_TRACE("PktGenAppCfg: dev %d pipe %d app %d shdl %d, Recirc Key %#x",
            dev_tgt.device_id,
            dev_tgt.dev_pipe_id,
            aid,
            sid,
            key);
  int i, pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t vals[BF_PIPE_COUNT] = {0}, val = 0;
  int val_cnt = 0;
  struct pipe_mgr_tof_pg_dev_ctx *ctx =
      pipe_mgr_tof_get_pgr_ctx(dev_tgt.device_id);
  if (!ctx) {
    LOG_ERROR("%s : Failed to get device context, dev %d",
              __func__,
              dev_tgt.device_id);
    return BF_UNEXPECTED;
  }
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1 << i))) continue;
    vals[val_cnt] = ctx->app.a[i][aid].recir_val;
    setp_pgr_app_recir_match_value_recir_match_value(&vals[val_cnt], key);
    val = vals[val_cnt++];
  }
  for (i = 0; i < val_cnt; ++i) {
    if (val != vals[i]) {
      LOG_ERROR(
          "Error, cannot program asymmetric data to multiple pipes. "
          "%#x != %#x dev %d pipe %#x at %s:%d",
          val,
          vals[i],
          dev_tgt.device_id,
          dev_tgt.dev_pipe_id,
          __func__,
          __LINE__);
      return BF_INVALID_ARG;
    }
  }
  pipe_mgr_tof_pg_txn_bkup_app_cfg(sid, dev_tgt.device_id);
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1 << i))) continue;
    ctx->app.a[i][aid].recir_val = val;
  }
  uint32_t addr =
      offsetof(Tofino, pipes[0].pmarb.pgr_reg.pgr_app[aid].recir_match_value);
  return pg_write_one_pipe_reg(sid, dev_tgt.device_id, pipe_mask, addr, val);
}

static bf_status_t pipe_mgr_tof_pktgen_reg_pgr_app_recir_match_mask(
    pipe_sess_hdl_t sid, bf_dev_target_t dev_tgt, int aid, uint32_t mask) {
  int i, pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t vals[BF_PIPE_COUNT] = {0}, val = 0;
  int val_cnt = 0;
  struct pipe_mgr_tof_pg_dev_ctx *ctx =
      pipe_mgr_tof_get_pgr_ctx(dev_tgt.device_id);
  if (!ctx) {
    LOG_ERROR("%s : Failed to get device context, dev %d",
              __func__,
              dev_tgt.device_id);
    return BF_UNEXPECTED;
  }
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1 << i))) continue;
    vals[val_cnt] = ctx->app.a[i][aid].recir_msk;
    setp_pgr_app_recir_match_mask_recir_match_mask(&vals[val_cnt], mask);
    val = vals[val_cnt++];
  }
  for (i = 0; i < val_cnt; ++i) {
    if (val != vals[i]) {
      LOG_ERROR(
          "Error, cannot program asymmetric data to multiple pipes. "
          "%#x != %#x dev %d pipe %#x at %s:%d",
          val,
          vals[i],
          dev_tgt.device_id,
          dev_tgt.dev_pipe_id,
          __func__,
          __LINE__);
      return BF_INVALID_ARG;
    }
  }
  pipe_mgr_tof_pg_txn_bkup_app_cfg(sid, dev_tgt.device_id);
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1 << i))) continue;
    ctx->app.a[i][aid].recir_msk = val;
  }
  uint32_t addr =
      offsetof(Tofino, pipes[0].pmarb.pgr_reg.pgr_app[aid].recir_match_mask);
  return pg_write_one_pipe_reg(sid, dev_tgt.device_id, pipe_mask, addr, val);
}

static bf_status_t pipe_mgr_tof_pktgen_reg_pgr_app_event_number(
    pipe_sess_hdl_t sid,
    bf_dev_target_t dev_tgt,
    int aid,
    uint16_t pkt_num,
    uint16_t batch_num) {
  LOG_TRACE("PktGenAppCfg: dev %d pipe %d app %d shdl %d, Batch %d Pkt %d",
            dev_tgt.device_id,
            dev_tgt.dev_pipe_id,
            aid,
            sid,
            batch_num,
            pkt_num);
  int i, pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t vals[BF_PIPE_COUNT] = {0}, val = 0;
  int val_cnt = 0;
  struct pipe_mgr_tof_pg_dev_ctx *ctx =
      pipe_mgr_tof_get_pgr_ctx(dev_tgt.device_id);
  if (!ctx) {
    LOG_ERROR("%s : Failed to get device context, dev %d",
              __func__,
              dev_tgt.device_id);
    return BF_UNEXPECTED;
  }
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1 << i))) continue;
    vals[val_cnt] = ctx->app.a[i][aid].event_num;
    setp_pgr_app_event_number_packet_num(&vals[val_cnt], pkt_num);
    setp_pgr_app_event_number_batch_num(&vals[val_cnt], batch_num);
    val = vals[val_cnt++];
  }
  for (i = 0; i < val_cnt; ++i) {
    if (val != vals[i]) {
      LOG_ERROR(
          "Error, cannot program asymmetric data to multiple pipes. "
          "%#x != %#x dev %d pipe %#x at %s:%d",
          val,
          vals[i],
          dev_tgt.device_id,
          dev_tgt.dev_pipe_id,
          __func__,
          __LINE__);
      return BF_INVALID_ARG;
    }
  }
  pipe_mgr_tof_pg_txn_bkup_app_cfg(sid, dev_tgt.device_id);
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1 << i))) continue;
    ctx->app.a[i][aid].event_num = val;
  }
  uint32_t addr =
      offsetof(Tofino, pipes[0].pmarb.pgr_reg.pgr_app[aid].event_number);
  return pg_write_one_pipe_reg(sid, dev_tgt.device_id, pipe_mask, addr, val);
}

static bf_status_t pipe_mgr_tof_pktgen_reg_pgr_app_event_ibg(
    pipe_sess_hdl_t sid, bf_dev_target_t dev_tgt, int aid, uint32_t ibg) {
  LOG_TRACE("PktGenAppCfg: dev %d pipe %d app %d shdl %d, IBG %d",
            dev_tgt.device_id,
            dev_tgt.dev_pipe_id,
            aid,
            sid,
            ibg);
  int i, pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t vals[BF_PIPE_COUNT] = {0}, val = 0;
  int val_cnt = 0;
  struct pipe_mgr_tof_pg_dev_ctx *ctx =
      pipe_mgr_tof_get_pgr_ctx(dev_tgt.device_id);
  if (!ctx) {
    LOG_ERROR("%s : Failed to get device context, dev %d",
              __func__,
              dev_tgt.device_id);
    return BF_UNEXPECTED;
  }
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1 << i))) continue;
    vals[val_cnt] = ctx->app.a[i][aid].ibg;
    setp_pgr_app_event_ibg_ibg_count(&vals[val_cnt], ibg);
    val = vals[val_cnt++];
  }
  for (i = 0; i < val_cnt; ++i) {
    if (val != vals[i]) {
      LOG_ERROR(
          "Error, cannot program asymmetric data to multiple pipes. "
          "%#x != %#x dev %d pipe %#x at %s:%d",
          val,
          vals[i],
          dev_tgt.device_id,
          dev_tgt.dev_pipe_id,
          __func__,
          __LINE__);
      return BF_INVALID_ARG;
    }
  }
  pipe_mgr_tof_pg_txn_bkup_app_cfg(sid, dev_tgt.device_id);
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1 << i))) continue;
    ctx->app.a[i][aid].ibg = val;
  }
  uint32_t addr =
      offsetof(Tofino, pipes[0].pmarb.pgr_reg.pgr_app[aid].event_ibg);
  return pg_write_one_pipe_reg(sid, dev_tgt.device_id, pipe_mask, addr, val);
}

static bf_status_t pipe_mgr_tof_pktgen_reg_pgr_app_ibg_jitter_val(
    pipe_sess_hdl_t sid, bf_dev_target_t dev_tgt, int aid, uint32_t jval) {
  LOG_TRACE("PktGenAppCfg: dev %d pipe %d app %d shdl %d, IBG-Jitter-Val %#x",
            dev_tgt.device_id,
            dev_tgt.dev_pipe_id,
            aid,
            sid,
            jval);
  int i, pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t vals[BF_PIPE_COUNT] = {0}, val = 0;
  int val_cnt = 0;
  struct pipe_mgr_tof_pg_dev_ctx *ctx =
      pipe_mgr_tof_get_pgr_ctx(dev_tgt.device_id);
  if (!ctx) {
    LOG_ERROR("%s : Failed to get device context, dev %d",
              __func__,
              dev_tgt.device_id);
    return BF_UNEXPECTED;
  }
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1 << i))) continue;
    vals[val_cnt] = ctx->app.a[i][aid].ibg_jit_val;
    setp_pgr_app_event_jitter_value_value(&vals[val_cnt], jval);
    val = vals[val_cnt++];
  }
  for (i = 0; i < val_cnt; ++i) {
    if (val != vals[i]) {
      LOG_ERROR(
          "Error, cannot program asymmetric data to multiple pipes. "
          "%#x != %#x dev %d pipe %#x at %s:%d",
          val,
          vals[i],
          dev_tgt.device_id,
          dev_tgt.dev_pipe_id,
          __func__,
          __LINE__);
      return BF_INVALID_ARG;
    }
  }
  pipe_mgr_tof_pg_txn_bkup_app_cfg(sid, dev_tgt.device_id);
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1 << i))) continue;
    ctx->app.a[i][aid].ibg_jit_val = val;
  }
  uint32_t addr = offsetof(
      Tofino, pipes[0].pmarb.pgr_reg.pgr_app[aid].event_ibg_jitter_value);
  return pg_write_one_pipe_reg(sid, dev_tgt.device_id, pipe_mask, addr, val);
}

static bf_status_t pipe_mgr_tof_pktgen_reg_pgr_app_ibg_jitter_mask(
    pipe_sess_hdl_t sid, bf_dev_target_t dev_tgt, int aid, uint32_t mask) {
  LOG_TRACE("PktGenAppCfg: dev %d pipe %d app %d shdl %d, IBG-Jitter-Mask %#x",
            dev_tgt.device_id,
            dev_tgt.dev_pipe_id,
            aid,
            sid,
            mask);
  int i, pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t vals[BF_PIPE_COUNT] = {0}, val = 0;
  int val_cnt = 0;
  struct pipe_mgr_tof_pg_dev_ctx *ctx =
      pipe_mgr_tof_get_pgr_ctx(dev_tgt.device_id);
  if (!ctx) {
    LOG_ERROR("%s : Failed to get device context, dev %d",
              __func__,
              dev_tgt.device_id);
    return BF_UNEXPECTED;
  }
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1 << i))) continue;
    vals[val_cnt] = ctx->app.a[i][aid].ibg_jit_msk;
    setp_pgr_app_event_jitter_mask_mask(&vals[val_cnt], mask);
    val = vals[val_cnt++];
  }
  for (i = 0; i < val_cnt; ++i) {
    if (val != vals[i]) {
      LOG_ERROR(
          "Error, cannot program asymmetric data to multiple pipes. "
          "%#x != %#x dev %d pipe %#x at %s:%d",
          val,
          vals[i],
          dev_tgt.device_id,
          dev_tgt.dev_pipe_id,
          __func__,
          __LINE__);
      return BF_INVALID_ARG;
    }
  }
  pipe_mgr_tof_pg_txn_bkup_app_cfg(sid, dev_tgt.device_id);
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1 << i))) continue;
    ctx->app.a[i][aid].ibg_jit_msk = val;
  }
  uint32_t addr = offsetof(
      Tofino, pipes[0].pmarb.pgr_reg.pgr_app[aid].event_ibg_jitter_mask);
  return pg_write_one_pipe_reg(sid, dev_tgt.device_id, pipe_mask, addr, val);
}

static bf_status_t pipe_mgr_tof_pktgen_reg_pgr_app_event_ipg(
    pipe_sess_hdl_t sid, bf_dev_target_t dev_tgt, int aid, uint32_t ipg) {
  LOG_TRACE("PktGenAppCfg: dev %d pipe %d app %d shdl %d, IPG %d",
            dev_tgt.device_id,
            dev_tgt.dev_pipe_id,
            aid,
            sid,
            ipg);
  int i, pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t vals[BF_PIPE_COUNT] = {0}, val = 0;
  int val_cnt = 0;
  struct pipe_mgr_tof_pg_dev_ctx *ctx =
      pipe_mgr_tof_get_pgr_ctx(dev_tgt.device_id);
  if (!ctx) {
    LOG_ERROR("%s : Failed to get device context, dev %d",
              __func__,
              dev_tgt.device_id);
    return BF_UNEXPECTED;
  }
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1 << i))) continue;
    vals[val_cnt] = ctx->app.a[i][aid].ipg;
    setp_pgr_app_event_ipg_ipg_count(&vals[val_cnt], ipg);
    val = vals[val_cnt++];
  }
  for (i = 0; i < val_cnt; ++i) {
    if (val != vals[i]) {
      LOG_ERROR(
          "Error, cannot program asymmetric data to multiple pipes. "
          "%#x != %#x dev %d pipe %#x at %s:%d",
          val,
          vals[i],
          dev_tgt.device_id,
          dev_tgt.dev_pipe_id,
          __func__,
          __LINE__);
      return BF_INVALID_ARG;
    }
  }
  pipe_mgr_tof_pg_txn_bkup_app_cfg(sid, dev_tgt.device_id);
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1 << i))) continue;
    ctx->app.a[i][aid].ipg = val;
  }
  uint32_t addr =
      offsetof(Tofino, pipes[0].pmarb.pgr_reg.pgr_app[aid].event_ipg);
  return pg_write_one_pipe_reg(sid, dev_tgt.device_id, pipe_mask, addr, val);
}

static bf_status_t pipe_mgr_tof_pktgen_reg_pgr_app_ipg_jitter_val(
    pipe_sess_hdl_t sid, bf_dev_target_t dev_tgt, int aid, uint32_t jval) {
  LOG_TRACE("PktGenAppCfg: dev %d pipe %d app %d shdl %d, IPG-Jitter-Val %#x",
            dev_tgt.device_id,
            dev_tgt.dev_pipe_id,
            aid,
            sid,
            jval);
  int i, pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t vals[BF_PIPE_COUNT] = {0}, val = 0;
  int val_cnt = 0;
  struct pipe_mgr_tof_pg_dev_ctx *ctx =
      pipe_mgr_tof_get_pgr_ctx(dev_tgt.device_id);
  if (!ctx) {
    LOG_ERROR("%s : Failed to get device context, dev %d",
              __func__,
              dev_tgt.device_id);
    return BF_UNEXPECTED;
  }
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1 << i))) continue;
    vals[val_cnt] = ctx->app.a[i][aid].ipg_jit_val;
    setp_pgr_app_event_jitter_value_value(&vals[val_cnt], jval);
    val = vals[val_cnt++];
  }
  for (i = 0; i < val_cnt; ++i) {
    if (val != vals[i]) {
      LOG_ERROR(
          "Error, cannot program asymmetric data to multiple pipes. "
          "%#x != %#x dev %d pipe %#x at %s:%d",
          val,
          vals[i],
          dev_tgt.device_id,
          dev_tgt.dev_pipe_id,
          __func__,
          __LINE__);
      return BF_INVALID_ARG;
    }
  }
  pipe_mgr_tof_pg_txn_bkup_app_cfg(sid, dev_tgt.device_id);
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1 << i))) continue;
    ctx->app.a[i][aid].ipg_jit_val = val;
  }
  uint32_t addr = offsetof(
      Tofino, pipes[0].pmarb.pgr_reg.pgr_app[aid].event_ipg_jitter_value);
  return pg_write_one_pipe_reg(sid, dev_tgt.device_id, pipe_mask, addr, val);
}

static bf_status_t pipe_mgr_tof_pktgen_reg_pgr_app_ipg_jitter_mask(
    pipe_sess_hdl_t sid, bf_dev_target_t dev_tgt, int aid, uint32_t mask) {
  LOG_TRACE("PktGenAppCfg: dev %d pipe %d app %d shdl %d, IPG-Jitter-Mask %#x",
            dev_tgt.device_id,
            dev_tgt.dev_pipe_id,
            aid,
            sid,
            mask);
  int i, pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t vals[BF_PIPE_COUNT] = {0}, val = 0;
  int val_cnt = 0;
  struct pipe_mgr_tof_pg_dev_ctx *ctx =
      pipe_mgr_tof_get_pgr_ctx(dev_tgt.device_id);
  if (!ctx) {
    LOG_ERROR("%s : Failed to get device context, dev %d",
              __func__,
              dev_tgt.device_id);
    return BF_UNEXPECTED;
  }
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1 << i))) continue;
    vals[val_cnt] = ctx->app.a[i][aid].ipg_jit_msk;
    setp_pgr_app_event_jitter_mask_mask(&vals[val_cnt], mask);
    val = vals[val_cnt++];
  }
  for (i = 0; i < val_cnt; ++i) {
    if (val != vals[i]) {
      LOG_ERROR(
          "Error, cannot program asymmetric data to multiple pipes. "
          "%#x != %#x dev %d pipe %#x at %s:%d",
          val,
          vals[i],
          dev_tgt.device_id,
          dev_tgt.dev_pipe_id,
          __func__,
          __LINE__);
      return BF_INVALID_ARG;
    }
  }
  pipe_mgr_tof_pg_txn_bkup_app_cfg(sid, dev_tgt.device_id);
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1 << i))) continue;
    ctx->app.a[i][aid].ipg_jit_msk = val;
  }
  uint32_t addr = offsetof(
      Tofino, pipes[0].pmarb.pgr_reg.pgr_app[aid].event_ipg_jitter_mask);
  return pg_write_one_pipe_reg(sid, dev_tgt.device_id, pipe_mask, addr, val);
}

static bf_status_t pipe_mgr_tof_pktgen_reg_pgr_app_event_timer(
    pipe_sess_hdl_t sid, bf_dev_target_t dev_tgt, int aid, uint32_t tval) {
  LOG_TRACE("PktGenAppCfg: dev %d pipe %d app %d shdl %d, Timer %d",
            dev_tgt.device_id,
            dev_tgt.dev_pipe_id,
            aid,
            sid,
            tval);
  int i, pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t vals[BF_PIPE_COUNT] = {0}, val = 0;
  int val_cnt = 0;
  struct pipe_mgr_tof_pg_dev_ctx *ctx =
      pipe_mgr_tof_get_pgr_ctx(dev_tgt.device_id);
  if (!ctx) {
    LOG_ERROR("%s : Failed to get device context, dev %d",
              __func__,
              dev_tgt.device_id);
    return BF_UNEXPECTED;
  }
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1 << i))) continue;
    vals[val_cnt] = ctx->app.a[i][aid].timer;
    setp_pgr_app_event_timer_timer_count(&vals[val_cnt], tval);
    val = vals[val_cnt++];
  }
  for (i = 0; i < val_cnt; ++i) {
    if (val != vals[i]) {
      LOG_ERROR(
          "Error, cannot program asymmetric data to multiple pipes. "
          "%#x != %#x dev %d pipe %#x at %s:%d",
          val,
          vals[i],
          dev_tgt.device_id,
          dev_tgt.dev_pipe_id,
          __func__,
          __LINE__);
      return BF_INVALID_ARG;
    }
  }
  pipe_mgr_tof_pg_txn_bkup_app_cfg(sid, dev_tgt.device_id);
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1 << i))) continue;
    ctx->app.a[i][aid].timer = val;
  }
  uint32_t addr =
      offsetof(Tofino, pipes[0].pmarb.pgr_reg.pgr_app[aid].event_timer);
  return pg_write_one_pipe_reg(sid, dev_tgt.device_id, pipe_mask, addr, val);
}

static uint32_t jitter_val_to_ns(bf_dev_id_t dev, uint32_t val, uint32_t mask) {
  if ((val == 0) || (mask == 0xFFFFFFFF)) {
    return 0;
  }
  int i;
  if (mask == 0)
    i = 31;
  else {
    for (i = 0; i < 31; i++) {
      if ((1u << (i + 1)) == ((~mask) + 1u)) break;
    }
  }
  uint64_t l = UINT64_C(1) << (i + 1);
  uint32_t clock = l / val;
  return pipe_mgr_clock_to_nsec(dev, clock);
}

static void jitter_ns_to_val(bf_dev_id_t dev,
                             uint32_t avg_jitter,
                             uint32_t *val,
                             uint32_t *mask) {
  /* Special case for zero jitter. */
  if (!avg_jitter) {
    *val = 0;
    *mask = 0xFFFFFFFF;
    return;
  }

  int i;
  uint64_t avg_clocks = pipe_mgr_nsec_to_clock(dev, avg_jitter);
  uint64_t l[32];
  /* Compute 2^x for values 1 <= x <= 32 */
  for (i = 0; i < 32; ++i) l[i] = UINT64_C(1) << (i + 1);

  /* Solve j = 2^x / k where j is the requested average jitter and 2^x is
   * the values computed above. */
  uint64_t k[32];
  for (i = 0; i < 32; ++i) k[i] = (avg_clocks ? l[i] / avg_clocks : 0);

  /* Compute the average jitter using the 32 pairs of l and k above. */
  uint64_t j[32];
  for (i = 0; i < 32; ++i) j[i] = k[i] ? l[i] / k[i] : 0;

  /* Check which of the 32 pairs of l and k give closest result to the
   * requested average jitter. */
  int x = 0, y = 0;
  for (y = 1; y < 32; ++y)
    if ((j[y] > avg_clocks ? j[y] - avg_clocks : avg_clocks - j[y]) <
        (j[x] > avg_clocks ? j[x] - avg_clocks : avg_clocks - j[x]))
      x = y;

  /* Unmask x+1 bits, and match the value of k[x]. */
  *val = k[x] & 0xFFFFFFFF;
  *mask = 31 == x ? 0 : ~((1u << (x + 1u)) - 1u);
  LOG_TRACE("Requested Jitter %u ns (%" PRIu64
            " clocks) --> Val 0x%08x Msk 0x%08x --> Actual %" PRIu64 " clocks",
            avg_jitter,
            avg_clocks,
            *val,
            *mask,
            j[x]);
}

/* Tofino
 * configure application
 */
bf_status_t pipe_mgr_tof_pktgen_cfg_app(bf_session_hdl_t shdl,
                                        bf_dev_target_t dev_tgt,
                                        int app_id,
                                        bf_pktgen_app_cfg_t *cfg) {
  /* Everything is validated.  Go ahead and update the app. */
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_tgt.device_id);
  if (!dev_info) return BF_INVALID_ARG;
  // ctrl
  bf_status_t sts = pipe_mgr_tof_pktgen_reg_pgr_app_ctrl(
      shdl, dev_tgt, app_id, cfg->trigger_type);
  if (sts != BF_SUCCESS) return sts;
  // payload ctrl
  sts = pipe_mgr_tof_pktgen_reg_pgr_app_payload_ctrl(
      shdl, dev_tgt, app_id, cfg->pkt_buffer_offset / 16, cfg->length + 4);
  if (sts != BF_SUCCESS) return sts;
  // ing_port ctrl
  sts =
      pipe_mgr_tof_pktgen_reg_pgr_app_ing_port_ctrl(shdl,
                                                    dev_tgt,
                                                    app_id,
                                                    cfg->pipe_local_source_port,
                                                    cfg->increment_source_port);
  if (sts != BF_SUCCESS) return sts;
  // key and mask
  if (BF_PKTGEN_TRIGGER_RECIRC_PATTERN == cfg->trigger_type) {
    sts = pipe_mgr_tof_pktgen_reg_pgr_app_recir_match_value(
        shdl, dev_tgt, app_id, cfg->u.pattern.value);
    if (sts != BF_SUCCESS) return sts;
    sts = pipe_mgr_tof_pktgen_reg_pgr_app_recir_match_mask(
        shdl, dev_tgt, app_id, ~cfg->u.pattern.mask);
    if (sts != BF_SUCCESS) return sts;
  } else {
    sts = pipe_mgr_tof_pktgen_reg_pgr_app_recir_match_value(
        shdl, dev_tgt, app_id, 0);
    if (sts != BF_SUCCESS) return sts;
    sts = pipe_mgr_tof_pktgen_reg_pgr_app_recir_match_mask(
        shdl, dev_tgt, app_id, 0);
    if (sts != BF_SUCCESS) return sts;
  }
  // event number
  sts = pipe_mgr_tof_pktgen_reg_pgr_app_event_number(
      shdl, dev_tgt, app_id, cfg->packets_per_batch, cfg->batch_count);
  if (sts != BF_SUCCESS) return sts;
  // event ibg
  sts = pipe_mgr_tof_pktgen_reg_pgr_app_event_ibg(
      shdl,
      dev_tgt,
      app_id,
      pipe_mgr_nsec_to_clock(dev_tgt.device_id, cfg->ibg));
  if (sts != BF_SUCCESS) return sts;
  // ibg jitter
  uint32_t m, v;
  jitter_ns_to_val(dev_tgt.device_id, cfg->ibg_jitter, &v, &m);
  sts =
      pipe_mgr_tof_pktgen_reg_pgr_app_ibg_jitter_val(shdl, dev_tgt, app_id, v);
  if (sts != BF_SUCCESS) return sts;
  sts =
      pipe_mgr_tof_pktgen_reg_pgr_app_ibg_jitter_mask(shdl, dev_tgt, app_id, m);
  if (sts != BF_SUCCESS) return sts;
  // event ipg
  sts = pipe_mgr_tof_pktgen_reg_pgr_app_event_ipg(
      shdl,
      dev_tgt,
      app_id,
      pipe_mgr_nsec_to_clock(dev_tgt.device_id, cfg->ipg));
  if (sts != BF_SUCCESS) return sts;
  // ipg jitter
  jitter_ns_to_val(dev_tgt.device_id, cfg->ipg_jitter, &v, &m);
  sts =
      pipe_mgr_tof_pktgen_reg_pgr_app_ipg_jitter_val(shdl, dev_tgt, app_id, v);
  if (sts != BF_SUCCESS) return sts;
  sts =
      pipe_mgr_tof_pktgen_reg_pgr_app_ipg_jitter_mask(shdl, dev_tgt, app_id, m);
  if (sts != BF_SUCCESS) return sts;
  // event timer
  uint32_t tval = 0;
  if (BF_PKTGEN_TRIGGER_TIMER_ONE_SHOT == cfg->trigger_type ||
      BF_PKTGEN_TRIGGER_TIMER_PERIODIC == cfg->trigger_type) {
    tval = pipe_mgr_nsec_to_clock(dev_tgt.device_id, cfg->u.timer_nanosec);
  }

  sts =
      pipe_mgr_tof_pktgen_reg_pgr_app_event_timer(shdl, dev_tgt, app_id, tval);
  return sts;
}

bf_status_t pipe_mgr_tof_pktgen_cfg_app_get(bf_dev_target_t dev_tgt,
                                            int app_id,
                                            bf_pktgen_app_cfg_t *cfg) {
  bf_dev_id_t dev = dev_tgt.device_id;
  bf_dev_pipe_t pipe = dev_tgt.dev_pipe_id;
  int i = (pipe == BF_DEV_PIPE_ALL) ? 0 : pipe;
  uint32_t data, tmp;
  uint32_t m, v;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) return BF_INVALID_ARG;
  struct pipe_mgr_tof_pg_dev_ctx *ctx = pipe_mgr_tof_get_pgr_ctx(dev);
  if (!ctx) {
    LOG_ERROR("%s : Failed to get device context, dev %d", __func__, dev);
    return BF_UNEXPECTED;
  }
  struct pipe_mgr_pg_app_ctx app_ctx = ctx->app.a[i][app_id];
  // trigger type
  data = app_ctx.app_ctrl;
  cfg->trigger_type = getp_pgr_app_ctrl_app_type(&data);
  // key and mask
  switch (cfg->trigger_type) {
    case BF_PKTGEN_TRIGGER_RECIRC_PATTERN:
      data = app_ctx.recir_val;
      cfg->u.pattern.value =
          getp_pgr_app_recir_match_value_recir_match_value(&data);
      data = app_ctx.recir_msk;
      cfg->u.pattern.mask =
          ~getp_pgr_app_recir_match_mask_recir_match_mask(&data);
      break;
    case BF_PKTGEN_TRIGGER_TIMER_ONE_SHOT:
    case BF_PKTGEN_TRIGGER_TIMER_PERIODIC:
      data = app_ctx.timer;
      tmp = getp_pgr_app_event_timer_timer_count(&data);
      cfg->u.timer_nanosec = pipe_mgr_clock_to_nsec(dev, tmp);
      break;
    default:
      memset(&cfg->u, 0, sizeof(cfg->u));
      break;
  }
  // payload ctrl
  data = app_ctx.payload_ctrl;
  cfg->pkt_buffer_offset =
      16 * (getp_pgr_app_payload_ctrl_app_payload_addr(&data));
  cfg->length = (getp_pgr_app_payload_ctrl_app_payload_size(&data)) - 4;
  // ing_port ctrl
  data = app_ctx.ing_port;
  cfg->pipe_local_source_port = dev_info->dev_cfg.dev_port_to_local_port(
      getp_pgr_app_ingr_port_ctrl_app_ingr_port(&data));
  cfg->increment_source_port =
      getp_pgr_app_ingr_port_ctrl_app_ingr_port_inc(&data);
  // event number
  data = app_ctx.event_num;
  cfg->packets_per_batch = getp_pgr_app_event_number_packet_num(&data);
  cfg->batch_count = getp_pgr_app_event_number_batch_num(&data);
  // event ibg
  data = app_ctx.ibg;
  tmp = getp_pgr_app_event_ibg_ibg_count(&data);
  cfg->ibg = pipe_mgr_clock_to_nsec(dev, tmp);
  // ibg jitter
  data = app_ctx.ibg_jit_val;
  v = getp_pgr_app_event_jitter_value_value(&data);
  data = app_ctx.ibg_jit_msk;
  m = getp_pgr_app_event_jitter_mask_mask(&data);
  cfg->ibg_jitter = jitter_val_to_ns(dev, v, m);
  // event ipg
  data = app_ctx.ipg;
  tmp = getp_pgr_app_event_ipg_ipg_count(&data);
  cfg->ipg = pipe_mgr_clock_to_nsec(dev, tmp);
  // ipg jitter
  data = app_ctx.ipg_jit_val;
  v = getp_pgr_app_event_jitter_value_value(&data);
  data = app_ctx.ipg_jit_msk;
  m = getp_pgr_app_event_jitter_mask_mask(&data);
  cfg->ipg_jitter = jitter_val_to_ns(dev, v, m);
  return BF_SUCCESS;
}

/* Download the app config after warm init quick */
bf_status_t pipe_mgr_tof_pktgen_app_cfg_download(bf_session_hdl_t shdl,
                                                 bf_dev_id_t dev) {
  uint32_t addr = 0, val = 0;
  uint32_t pipe_cnt = pipe_mgr_get_num_active_pipes(dev);
  int pipe_mask = 0;
  bf_status_t sts = BF_SUCCESS;

  struct pipe_mgr_tof_pg_dev_ctx *ctx = pipe_mgr_tof_get_pgr_ctx(dev);
  if (!ctx) {
    LOG_ERROR("%s : Failed to get device context, dev %d", __func__, dev);
    return BF_UNEXPECTED;
  }
  for (uint32_t i = 0; i < pipe_cnt; ++i) {
    bf_dev_target_t dev_tgt = {dev, i};
    pipe_mask = pg_log_pipe_mask(dev_tgt);
    for (uint32_t aid = 0; aid < PIPE_MGR_TOF_PKTGEN_APP_CNT; aid++) {
      // payload ctrl
      addr = offsetof(Tofino, pipes[0].pmarb.pgr_reg.pgr_app[aid].payload_ctrl);
      val = ctx->app.a[i][aid].payload_ctrl = val;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      // ing_port ctrl
      addr =
          offsetof(Tofino, pipes[0].pmarb.pgr_reg.pgr_app[aid].ingr_port_ctrl);
      val = ctx->app.a[i][aid].ing_port;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      // recirc key
      addr = offsetof(Tofino,
                      pipes[0].pmarb.pgr_reg.pgr_app[aid].recir_match_value);
      val = ctx->app.a[i][aid].recir_val;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      // recirc mask
      addr = offsetof(Tofino,
                      pipes[0].pmarb.pgr_reg.pgr_app[aid].recir_match_mask);
      val = ctx->app.a[i][aid].recir_msk;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      // event number
      addr = offsetof(Tofino, pipes[0].pmarb.pgr_reg.pgr_app[aid].event_number);
      val = ctx->app.a[i][aid].event_num;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      // event ibg
      addr = offsetof(Tofino, pipes[0].pmarb.pgr_reg.pgr_app[aid].event_ibg);
      val = ctx->app.a[i][aid].ibg;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      // ibg jitter val
      addr = offsetof(
          Tofino, pipes[0].pmarb.pgr_reg.pgr_app[aid].event_ibg_jitter_value);
      val = ctx->app.a[i][aid].ibg_jit_val;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      // ibg jitter mask
      addr = offsetof(
          Tofino, pipes[0].pmarb.pgr_reg.pgr_app[aid].event_ibg_jitter_mask);
      val = ctx->app.a[i][aid].ibg_jit_msk;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      // event ipg
      addr = offsetof(Tofino, pipes[0].pmarb.pgr_reg.pgr_app[aid].event_ipg);
      val = ctx->app.a[i][aid].ipg;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      // ipg jitter val
      addr = offsetof(
          Tofino, pipes[0].pmarb.pgr_reg.pgr_app[aid].event_ipg_jitter_value);
      val = ctx->app.a[i][aid].ipg_jit_val;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      // ipg jitter mask
      addr = offsetof(
          Tofino, pipes[0].pmarb.pgr_reg.pgr_app[aid].event_ipg_jitter_mask);
      val = ctx->app.a[i][aid].ipg_jit_msk;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      // event timer
      addr = offsetof(Tofino, pipes[0].pmarb.pgr_reg.pgr_app[aid].event_timer);
      val = ctx->app.a[i][aid].timer;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      // ctrl
      addr = offsetof(Tofino, pipes[0].pmarb.pgr_reg.pgr_app[aid].ctrl);
      val = ctx->app.a[i][aid].app_ctrl;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;
    }  // app-id
  }    // pipe

  return sts;
}

/* Warm init quick */
bf_status_t pipe_mgr_tof_pktgen_warm_init_quick(bf_session_hdl_t shdl,
                                                bf_dev_id_t dev) {
  bf_status_t sts = pipe_mgr_tof_pktgen_dev_init(shdl, dev, true);
  if (sts != BF_SUCCESS) return sts;

  sts = pipe_mgr_tof_pktgen_app_cfg_download(shdl, dev);

  return sts;
}
