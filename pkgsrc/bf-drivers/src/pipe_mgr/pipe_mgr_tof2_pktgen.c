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
#include <arpa/inet.h>
#include <dvm/bf_drv_intf.h>
#include <lld/lld_reg_if.h>
#include "pipe_mgr_int.h"
#include "pipe_mgr_drv.h"
#include "pipe_mgr_drv_intf.h"
#include "pipe_mgr_pktgen_comm.h"
#include "pipe_mgr_tof2_pktgen.h"
#include <tof2_regs/tof2_mem_drv.h>
#include <tof2_regs/tof2_reg_drv.h>
#include <port_mgr/bf_port_if.h>

/* Context-independent recirc state */
uint8_t tbc_eth_en[PIPE_MGR_NUM_DEVICES] = {0};

/* Pointer to global pipe_mgr context */
extern pipe_mgr_ctx_t *pipe_mgr_ctx;

static bf_status_t pg_tof2_read_one_pipe_reg(bf_dev_target_t dev_tgt,
                                             uint32_t addr,
                                             uint32_t *data) {
  *data = 0;
  bf_dev_pipe_t physical_pipe = dev_tgt.dev_pipe_id;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_tgt.device_id);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return BF_INVALID_ARG;
  }
  if (dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL) {
    PIPE_MGR_DBGCHK(false);
    return BF_INVALID_ARG;
  }
  pipe_mgr_map_pipe_id_log_to_phy(
      dev_info, dev_tgt.dev_pipe_id, &physical_pipe);
  uint32_t base =
      offsetof(tof2_reg, pipes[physical_pipe]) - offsetof(tof2_reg, pipes[0]);

  int x = lld_read_register(dev_tgt.device_id, base + addr, data);
  if (x) return BF_HW_COMM_FAIL;
  return BF_SUCCESS;
}

/* Write pkt buffer shadow memory to asic */
static bf_status_t pipe_mgr_pkt_buffer_tof2_addr_get(uint64_t *addr,
                                                     uint32_t *elem_size,
                                                     uint32_t *count) {
  *addr = tof2_mem_pipes_parde_pgr_mem_rspec_buffer_mem_word_address / 16;
  /* Write the entire 16K pkt buffer */
  *elem_size =
      tof2_mem_pipes_parde_pgr_mem_rspec_buffer_mem_word_array_element_size;
  *count = tof2_mem_pipes_parde_pgr_mem_rspec_buffer_mem_word_array_count;
  return BF_SUCCESS;
}

static pipe_mgr_tof2_pg_dev_ctx *pipe_mgr_tof2_get_pgr_ctx(bf_dev_id_t dev) {
  struct pipe_mgr_pg_dev_ctx *ctx = pipe_mgr_pktgen_ctx(dev);
  return ctx ? ctx->u.tof2_ctx : NULL;
}

static void pipe_mgr_tof2_pgr_tbc_port_recir_en(bf_session_hdl_t shdl,
                                                bf_dev_target_t dev_tgt,
                                                bool recir_enable,
                                                bool use_shadow) {
  uint32_t data = 0;
  uint32_t addr = offsetof(
      tof2_reg, pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.tbc_port_ctrl);
  if (use_shadow) {
    recir_enable = (tbc_eth_en[dev_tgt.device_id] & 1) == 0;
  }
  setp_tof2_pgr_tbc_port_ctrl_port_en(&data, recir_enable ? 0 : 1);
  pg_write_one_pipe_reg(
      shdl, dev_tgt.device_id, 1u << dev_tgt.dev_pipe_id, addr, data);
}
static void pipe_mgr_tof2_pgr_init_eth_port_ctrl(bf_session_hdl_t shdl,
                                                 bf_dev_target_t dev_tgt,
                                                 uint8_t port_en,
                                                 uint8_t chnl_en,
                                                 uint8_t chnl_mode,
                                                 uint8_t chnl_seq,
                                                 bool use_shadow) {
  uint32_t addr;
  uint32_t data = 0;
  uint32_t addr_scratch;
  addr = offsetof(
      tof2_reg, pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.eth_cpu_port_ctrl);
  addr_scratch =
      offsetof(tof2_reg, pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.scratch);
  if (use_shadow) {
    data = pipe_mgr_tof2_get_pgr_ctx(dev_tgt.device_id)->port_ctrl;
  } else {
    setp_tof2_pgr_eth_cpu_ctrl_port_en(&data, port_en);
    setp_tof2_pgr_eth_cpu_ctrl_channel_en(&data, chnl_en);
    setp_tof2_pgr_eth_cpu_ctrl_channel_mode(&data, chnl_mode);
    setp_tof2_pgr_eth_cpu_ctrl_channel_seq(&data, chnl_seq);
    pipe_mgr_tof2_get_pgr_ctx(dev_tgt.device_id)->port_ctrl = data;
  }
  /* The port_en bit can only be set after the mode is set.  First write the
   * value without the port_en bit, write a scratch register, then write the
   * desired value. */
  if (port_en) {
    uint32_t t = data;
    setp_tof2_pgr_eth_cpu_ctrl_port_en(&t, 0);
    pg_write_one_pipe_reg(
        shdl, dev_tgt.device_id, 1u << dev_tgt.dev_pipe_id, addr, t);
    pg_write_one_pipe_reg(
        shdl, dev_tgt.device_id, 1u << dev_tgt.dev_pipe_id, addr_scratch, 0);
  }
  /* The channel mode must always be four; TF2LAB-41. */
  uint32_t wr_data = data;
  setp_tof2_pgr_eth_cpu_ctrl_channel_en(&wr_data, 0xF);
  setp_tof2_pgr_eth_cpu_ctrl_channel_mode(&wr_data, 4);
  pg_write_one_pipe_reg(
      shdl, dev_tgt.device_id, 1u << dev_tgt.dev_pipe_id, addr, wr_data);
}

static void pipe_mgr_tof2_free_dev_ctx(struct pipe_mgr_pg_dev_ctx *c,
                                       bf_dev_id_t dev) {
  uint32_t dev_pipes = pipe_mgr_get_num_active_pipes(dev);
  uint32_t i;
  if (!c) return;
  if (!c->u.tof_ctx) {
    PIPE_MGR_FREE(c);
    return;
  }
  if (c->u.tof2_ctx->app.a) {
    for (i = 0; i < dev_pipes; i++) {
      if (c->u.tof2_ctx->app.a[i]) PIPE_MGR_FREE(c->u.tof2_ctx->app.a[i]);
    }
    PIPE_MGR_FREE(c->u.tof2_ctx->app.a);
  }
  if (c->u.tof2_ctx->app.b) {
    for (i = 0; i < dev_pipes; i++) {
      if (c->u.tof2_ctx->app.b[i]) PIPE_MGR_FREE(c->u.tof2_ctx->app.b[i]);
    }
    PIPE_MGR_FREE(c->u.tof2_ctx->app.b);
  }
  if (c->u.tof2_ctx->ipb_chnl_sp) {
    for (i = 0; i < dev_pipes; i++) {
      if (c->u.tof2_ctx->ipb_chnl_sp[i])
        PIPE_MGR_FREE(c->u.tof2_ctx->ipb_chnl_sp[i]);
    }
    PIPE_MGR_FREE(c->u.tof2_ctx->ipb_chnl_sp);
  }
  if (c->u.tof2_ctx->ebuf_port_ctrl) {
    for (i = 0; i < dev_pipes; i++) {
      if (c->u.tof2_ctx->ebuf_port_ctrl[i])
        PIPE_MGR_FREE(c->u.tof2_ctx->ebuf_port_ctrl[i]);
    }
    PIPE_MGR_FREE(c->u.tof2_ctx->ebuf_port_ctrl);
  }
  if (c->u.tof2_ctx->pkt_buffer_shadow)
    PIPE_MGR_FREE(c->u.tof2_ctx->pkt_buffer_shadow);
  if (c->u.tof2_ctx->ipb_ctrl) PIPE_MGR_FREE(c->u.tof2_ctx->ipb_ctrl);
  if (c->u.tof2_ctx->pfc) PIPE_MGR_FREE(c->u.tof2_ctx->pfc);
  if (c->u.tof2_ctx->ebuf_chnl_en) PIPE_MGR_FREE(c->u.tof2_ctx->ebuf_chnl_en);
  if (c->u.tof2_ctx->app_recirc_src)
    PIPE_MGR_FREE(c->u.tof2_ctx->app_recirc_src);
  if (c->u.tof2_ctx->port_down_mode)
    PIPE_MGR_FREE(c->u.tof2_ctx->port_down_mode);
  if (c->u.tof2_ctx->port_down_mask) {
    for (i = 0; i < dev_pipes; i++)
      PIPE_MGR_FREE(c->u.tof2_ctx->port_down_mask[i]);
    PIPE_MGR_FREE(c->u.tof2_ctx->port_down_mask);
  }
  PIPE_MGR_FREE(c->u.tof2_ctx);
  PIPE_MGR_FREE(c);
}

static uint32_t get_port_down_dis_addr(int local_port_bit_idx) {
  if (local_port_bit_idx < 32) {
    return offsetof(tof2_reg,
                    pipes[0]
                        .pardereg.pgstnreg.pgrreg.pgr_common.port_down_dis
                        .port_down_dis_0_3);
  } else if (local_port_bit_idx < 64) {
    return offsetof(tof2_reg,
                    pipes[0]
                        .pardereg.pgstnreg.pgrreg.pgr_common.port_down_dis
                        .port_down_dis_1_3);
  } else {
    return offsetof(tof2_reg,
                    pipes[0]
                        .pardereg.pgstnreg.pgrreg.pgr_common.port_down_dis
                        .port_down_dis_2_3);
  }
}

bf_status_t pipe_mgr_tof2_pktgen_dev_init(bf_session_hdl_t shdl,
                                          bf_dev_id_t dev,
                                          bool use_shadow) {
  bf_status_t sts = BF_SUCCESS;
  int dev_pipes = pipe_mgr_get_num_active_pipes(dev);
  uint32_t data, addr;
  int pbm = 0;

  for (int i = 0; i < dev_pipes; ++i) {
    pbm |= 1u << i;
  }

  bf_dev_port_t eth_cpu_dev_port = bf_eth_cpu_port_get(dev);
  bf_dev_pipe_t cpu_pipe = DEV_PORT_TO_PIPE(eth_cpu_dev_port);
  bf_dev_target_t dev_tgt = {dev, 0};

  /* Setup timestamping offsets in several places. */
  for (int i = 0; i < 4; i++) {
    addr = offsetof(tof2_reg,
                    pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.recirc_ts[i]);
    data = 65;
    sts = pg_write_one_pipe_reg(shdl, dev, pbm, addr, data);
    if (sts != BF_SUCCESS) return sts;
  }

  addr = offsetof(tof2_reg,
                  pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.csr_ts_offset);
  data = 80 << 12;
  sts = pg_write_one_pipe_reg(shdl, dev, pbm, addr, data);
  if (sts != BF_SUCCESS) return sts;

  addr = offsetof(
      tof2_reg, pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.cfg_pgen_chnl_ts);
  data = 65;
  sts = pg_write_one_pipe_reg(shdl, dev, pbm, addr, data);
  if (sts != BF_SUCCESS) return sts;

  addr = offsetof(tof2_reg,
                  pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.cfg_pgen_tdm_ts);
  data = 65;
  sts = pg_write_one_pipe_reg(shdl, dev, pbm, addr, data);
  if (sts != BF_SUCCESS) return sts;

  /* Configure a few register per-pipe... */
  for (int i = 0; i < dev_pipes; i++) {
    /* Configure the logical pipe id used by the pgen apps and set the "swap"
     * flag which affects the data read out of the pgen payload buffer. */
    data = 0;
    addr = offsetof(
        tof2_reg,
        pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.pgen_ctrl.pgen_ctrl_0_2);
    setp_tof2_pgr_pgen_ctrl_pgen_ctrl_0_2_pipe_id(&data, i);
    setp_tof2_pgr_pgen_ctrl_pgen_ctrl_0_2_swap_en(&data, 1);
    sts = pg_write_one_pipe_reg(shdl, dev, 1u << i, addr, data);
    if (sts != BF_SUCCESS) return sts;
    addr = offsetof(
        tof2_reg,
        pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.pgen_ctrl.pgen_ctrl_1_2);
    sts = pg_write_one_pipe_reg(shdl, dev, 1u << i, addr, 0);
    if (sts != BF_SUCCESS) return sts;

    /* For the pipe with the Ethernet CPU port enable pgen port-down detection,
     * otherwise disable it for the non-MAC ports. */
    data = i == (int)cpu_pipe ? 0xF0 : 0;
    addr = offsetof(
        tof2_reg,
        pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.pgen_port_down_ctrl);
    sts = pg_write_one_pipe_reg(shdl, dev, 1u << i, addr, data);
    if (sts != BF_SUCCESS) return sts;
  }

  /* Default to TBC path rather than recirc for the PCIe CPU port. */
  if (!use_shadow) {
    tbc_eth_en[dev] |= 1;
  }
  dev_tgt.dev_pipe_id = cpu_pipe;
  pipe_mgr_tof2_pgr_tbc_port_recir_en(shdl, dev_tgt, false, use_shadow);

  /* Default to Ethernet CPU MAC rather than recirc for the Eth CPU port. */
  if (!use_shadow) {
    tbc_eth_en[dev] |= (0xf << 2);
  }
  pipe_mgr_tof2_pgr_init_eth_port_ctrl(
      shdl, dev_tgt, 1, 0xf, 4, 0xd8, use_shadow);

  return BF_SUCCESS;
}

/* Tofino2
 * add pgr dev
 */
bf_status_t pipe_mgr_tof2_pktgen_add_dev(bf_session_hdl_t shdl,
                                         bf_dev_id_t dev) {
  bf_status_t sts = BF_SUCCESS;

  /* Initialize context for this device. */
  struct pipe_mgr_pg_dev_ctx *c = PIPE_MGR_CALLOC(1, sizeof *c);
  if (!c) return BF_NO_SYS_RESOURCES;
  c->u.tof2_ctx = PIPE_MGR_CALLOC(1, sizeof(*c->u.tof2_ctx));
  if (!c->u.tof2_ctx) goto cleanup;
  pipe_mgr_tof2_pg_dev_ctx *ctx = c->u.tof2_ctx;
  uint32_t dev_pipes = pipe_mgr_get_num_active_pipes(dev);
  uint32_t i;

  ctx->ipb_chnl_sp = PIPE_MGR_CALLOC(dev_pipes, sizeof(*ctx->ipb_chnl_sp));
  ctx->ipb_ctrl = PIPE_MGR_CALLOC(dev_pipes, sizeof(*ctx->ipb_ctrl));
  ctx->ebuf_chnl_en = PIPE_MGR_CALLOC(dev_pipes, sizeof(*ctx->ebuf_chnl_en));
  ctx->ebuf_port_ctrl =
      PIPE_MGR_CALLOC(dev_pipes, sizeof(*ctx->ebuf_port_ctrl));
  ctx->app_recirc_src =
      PIPE_MGR_CALLOC(dev_pipes, sizeof(*ctx->app_recirc_src));
  ctx->pkt_buffer_shadow =
      PIPE_MGR_CALLOC(dev_pipes, sizeof(*ctx->pkt_buffer_shadow));
  ctx->pfc = PIPE_MGR_CALLOC(dev_pipes, sizeof(*ctx->pfc));
  if (!ctx->ipb_chnl_sp || !ctx->ipb_ctrl || !ctx->ebuf_chnl_en ||
      !ctx->ebuf_port_ctrl || !ctx->pkt_buffer_shadow || !ctx->pfc) {
    goto cleanup;
  }
  ctx->app.a = PIPE_MGR_CALLOC(dev_pipes, sizeof(*ctx->app.a));
  ctx->app.b = PIPE_MGR_CALLOC(dev_pipes, sizeof(*ctx->app.b));
  if (!ctx->app.a || !ctx->app.b) goto cleanup;
  for (i = 0; i < dev_pipes; i++) {
    ctx->app.a[i] =
        PIPE_MGR_CALLOC(PIPE_MGR_TOF2_PKTGEN_APP_CNT, sizeof(*ctx->app.a[i]));
    ctx->app.b[i] =
        PIPE_MGR_CALLOC(PIPE_MGR_TOF2_PKTGEN_APP_CNT, sizeof(*ctx->app.b[i]));
    ctx->ipb_chnl_sp[i] = PIPE_MGR_CALLOC(8, sizeof(*ctx->ipb_chnl_sp[i]));
    ctx->ebuf_port_ctrl[i] =
        PIPE_MGR_CALLOC(4, sizeof(*ctx->ebuf_port_ctrl[i]));
    if (!ctx->app.a[i] || !ctx->app.b[i] || !ctx->ipb_chnl_sp[i]) goto cleanup;
  }
  ctx->port_down_mode = PIPE_MGR_CALLOC(dev_pipes, sizeof *ctx->port_down_mode);
  if (!ctx->port_down_mode) goto cleanup;
  PIPE_MGR_MEMSET(
      ctx->port_down_mode, BF_PKTGEN_PORT_DOWN_REPLAY_NONE, dev_pipes);

  ctx->port_down_mask = PIPE_MGR_CALLOC(dev_pipes, sizeof *ctx->port_down_mask);
  if (!ctx->port_down_mask) goto cleanup;
  for (i = 0; i < dev_pipes; ++i) {
    ctx->port_down_mask[i] = PIPE_MGR_CALLOC(3, sizeof *ctx->port_down_mask[i]);
    if (!ctx->port_down_mask[i]) goto cleanup;
    const int sz = sizeof ctx->port_down_mask[0][0].port_mask;
    PIPE_MGR_MEMSET(ctx->port_down_mask[i][0].port_mask, 0xFF, sz);
    PIPE_MGR_MEMSET(ctx->port_down_mask[i][1].port_mask, 0xFF, sz);
    /* Index 2 is initialized to zero from the calloc. */
  }
  if (!ctx->port_down_mask) goto cleanup;

  pipe_mgr_pktgen_ctx_set(dev, c);
  // initial values
  ctx->port_ctrl = 0;
  for (i = 0; i < dev_pipes; i++) {
    PIPE_MGR_MEMSET(ctx->ipb_chnl_sp[i], 0, 8);
    ctx->ipb_ctrl[i] = 0;
    ctx->app_recirc_src[i] = 0;
    ctx->ebuf_chnl_en[i] = 0;
    PIPE_MGR_MEMSET(ctx->ebuf_port_ctrl[i], 0, 4);
  }
  sts = pipe_mgr_tof2_pktgen_dev_init(shdl, dev, false);
  if (BF_SUCCESS != sts) return sts;

  return BF_SUCCESS;

cleanup:
  pipe_mgr_tof2_free_dev_ctx(c, dev);
  return BF_NO_SYS_RESOURCES;
}
/* Tofino2
 * remove pgr dev
 */
bf_status_t pipe_mgr_tof2_pktgen_rmv_dev(bf_dev_id_t dev) {
  struct pipe_mgr_pg_dev_ctx *c = pipe_mgr_pktgen_ctx(dev);
  pipe_mgr_pktgen_ctx_set(dev, NULL);
  pipe_mgr_tof2_free_dev_ctx(c, dev);
  return BF_SUCCESS;
}

static bool pipe_mgr_tof2_ipb_conflict_check(bf_dev_target_t dev_tgt,
                                             int port) {
  /* true: pass conflict check*/
  struct pipe_mgr_tof2_pg_dev_ctx *c =
      pipe_mgr_tof2_get_pgr_ctx(dev_tgt.device_id);
  if (c == NULL) {
    LOG_ERROR("%s:%d Unable to retrieve device context, dev %d",
              __func__,
              __LINE__,
              dev_tgt.device_id);
    return false;
  }
  uint32_t *ipb_ch_sp = c->ipb_chnl_sp[dev_tgt.dev_pipe_id];
  if (ipb_ch_sp[port] != 0) return true;
  // if == 0, find the former non-zero
  for (int i = (port - 1); i >= 0; i--) {
    if (ipb_ch_sp[i] == 0) continue;
    return !(ipb_ch_sp[i] > (uint32_t)(port - i));
  }
  return true;
}

static uint8_t pipe_mgr_tof2_get_chnls(bf_port_speeds_t speed, bool is_eth) {
  // Arbiter's left side channels number
  switch (speed) {
    case BF_SPEED_1G:
    case BF_SPEED_10G:
    case BF_SPEED_25G:
      return 1;
    case BF_SPEED_200G:
    case BF_SPEED_400G:
      return 0;
    case BF_SPEED_40G:
    case BF_SPEED_40G_R2:
    case BF_SPEED_50G:
    case BF_SPEED_50G_CONS:
      return (is_eth) ? 2 : 1;
    case BF_SPEED_100G:
      return (is_eth) ? 4 : 2;
    default:
      return 0;
  }
}

static uint32_t pipe_mgr_tof2_get_speed(bf_port_speeds_t speed) {
  // Arbiter's right side channel occupation for calculation ibp seq
  switch (speed) {
    case BF_SPEED_1G:
    case BF_SPEED_10G:
    case BF_SPEED_25G:
      return 25;
    case BF_SPEED_200G:
      return 200;
    case BF_SPEED_400G:
      return 400;
    case BF_SPEED_40G:
    case BF_SPEED_40G_R2:
    case BF_SPEED_50G:
    case BF_SPEED_50G_CONS:
      return 50;
    case BF_SPEED_100G:
      return 100;
    default:
      return 0;
  }
}

static bf_status_t pipe_mgr_tof2_get_ipb_seq(bf_dev_target_t dev_tgt,
                                             uint32_t *seq_return) {
  uint32_t seq = 0;
  uint32_t *ipb_ch_sp = pipe_mgr_tof2_get_pgr_ctx(dev_tgt.device_id)
                            ->ipb_chnl_sp[dev_tgt.dev_pipe_id];
  uint8_t spare[8] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  bool occupy[8] = {false, false, false, false, false, false, false, false};
  uint32_t i, j, spare_idx = 0, search_idx = 0;
  for (i = 0; i < 8; i++) {
    switch (ipb_ch_sp[i]) {
      /*      case 8:
              *seq_return = ((i << 21) & (i << 18) & (i << 15) & (i << 12) &
                             (i << 8) & (i << 6) & (i << 3) & i);
              return BF_SUCCESS;*/
      case 4:
        if ((occupy[0] == false) && (occupy[2] == false) &&
            (occupy[4] == false) && (occupy[6] == false)) {
          seq |= ((i << 18) & (i << 12) & (i << 6) & i);
          occupy[0] = occupy[2] = occupy[4] = occupy[6] = true;
        } else if ((occupy[1] == false) && (occupy[3] == false) &&
                   (occupy[5] == false) && (occupy[7] == false)) {
          seq |= ((i << 21) & (i << 15) & (i << 8) & (i << 3));
          occupy[1] = occupy[3] = occupy[5] = occupy[7] = true;
        } else {
          *seq_return = 0xfac688;
          return BF_INVALID_ARG;
        }
        break;
      case 2:
        j = 0;
        while ((j < 8) && (occupy[j] == true)) j++;
        if (j >= 8) {
          *seq_return = 0xfac688;
          return BF_INVALID_ARG;
        }
        seq |= (i << (3 * j));
        occupy[j] = true;
        j += 4;
        while ((j < 8) && (occupy[j] == true)) j++;
        if (j >= 8) {
          *seq_return = 0xfac688;
          return BF_INVALID_ARG;
        }
        seq |= (i << (3 * j));
        occupy[j] = true;
        break;
      case 1:
        j = 0;
        while ((j < 8) && (occupy[j] == true)) j++;
        if (j >= 8) {
          *seq_return = 0xfac688;
          return BF_INVALID_ARG;
        }
        seq |= (i << (3 * j));
        occupy[j] = true;
        if (i % 2 == 0) {
          j += 4;
          i++;  // pair chnl
          while ((j < 8) && (occupy[j] == true)) j++;
          if (j >= 8) {
            *seq_return = 0xfac688;
            return BF_INVALID_ARG;
          }
          seq |= (i << (3 * j));
          occupy[j] = true;
        }
        break;
      case 0:
        spare[spare_idx++] = i;
        break;
      default:
        *seq_return = 0xfac688;  // set to default value
        return BF_INVALID_ARG;
    }
  }
  for (j = 0; j < 8; j++) {
    if (occupy[j] == true) continue;
    if (spare_idx == 0) {
      *seq_return = 0xfac688;
      return BF_INVALID_ARG;
    }
    seq |= (spare[search_idx++] << (3 * j));
    if (search_idx >= spare_idx) search_idx = 0;
    occupy[j] = true;
  }
  *seq_return = seq;
  return BF_SUCCESS;
}
static bf_status_t pipe_mgr_tof2_pgr_ipb_ctrl_set(pipe_sess_hdl_t shdl,
                                                  bf_dev_target_t dev_tgt,
                                                  int port,
                                                  int port_end,
                                                  bool enable,
                                                  bf_port_speeds_t speed) {
  uint32_t addr, data1, seq = 0;
  uint32_t chnl_en;
  bf_status_t sts;
  addr = offsetof(
      tof2_reg,
      pipes[0]
          .pardereg.pgstnreg.pgrreg.pgr_common.ipb_port_ctrl.ipb_port_ctrl_0_2);
  pipe_mgr_tof2_pg_dev_ctx *ctx = pipe_mgr_tof2_get_pgr_ctx(dev_tgt.device_id);
  if (ctx == NULL) {
    LOG_ERROR("%s:%d Unable to retrieve device context, dev %d",
              __func__,
              __LINE__,
              dev_tgt.device_id);
    return BF_INVALID_ARG;
  }
  uint32_t *data = &ctx->ipb_ctrl[dev_tgt.dev_pipe_id];
  // chnl en
  chnl_en =
      getp_tof2_pgr_ipb_port_ctrl_ipb_port_ctrl_0_2_recir_channel_en(data);
  for (int i = port; i <= port_end; i++) {
    if (enable) {
      chnl_en |= (1u << i);
    } else {
      chnl_en &= ~(1u << i);
    }
    ctx->ipb_chnl_sp[dev_tgt.dev_pipe_id][i] = 0;
  }
  if (enable) {
    uint32_t speed_numb = pipe_mgr_tof2_get_speed(speed) / 50;
    ctx->ipb_chnl_sp[dev_tgt.dev_pipe_id][port] =
        (speed_numb < 1) ? 1 : speed_numb;
  }
  setp_tof2_pgr_ipb_port_ctrl_ipb_port_ctrl_0_2_recir_channel_en(data, chnl_en);
  // seq
  sts = pipe_mgr_tof2_get_ipb_seq(dev_tgt, &seq);
  if (sts != BF_SUCCESS) {
    LOG_ERROR("%s:%d Get ipb sequence error, dev %d, pipe %d",
              __func__,
              __LINE__,
              dev_tgt.device_id,
              dev_tgt.dev_pipe_id);
    return sts;
  }
  setp_tof2_pgr_ipb_port_ctrl_ipb_port_ctrl_0_2_channel_seq_7_0(data,
                                                                seq & 0xff);
  pg_write_one_pipe_reg(
      shdl, dev_tgt.device_id, 1 << dev_tgt.dev_pipe_id, addr, *data);
  addr = offsetof(
      tof2_reg,
      pipes[0]
          .pardereg.pgstnreg.pgrreg.pgr_common.ipb_port_ctrl.ipb_port_ctrl_1_2);
  data1 = 0;
  setp_tof2_pgr_ipb_port_ctrl_ipb_port_ctrl_1_2_channel_seq_23_8(&data1,
                                                                 seq >> 8);
  pg_write_one_pipe_reg(
      shdl, dev_tgt.device_id, 1 << dev_tgt.dev_pipe_id, addr, data1);
  return BF_SUCCESS;
}

static bf_status_t pipe_mgr_tof2_pgr_ebuf_port_ctrl_set(pipe_sess_hdl_t shdl,
                                                        bf_dev_target_t dev_tgt,
                                                        int port,
                                                        uint8_t chnl_numb,
                                                        bool add) {
  uint32_t addr, data1;
  uint32_t chnl_en;
  int pair_port;
  bool two_steps_op = false;
  pipe_mgr_tof2_pg_dev_ctx *ctx = pipe_mgr_tof2_get_pgr_ctx(dev_tgt.device_id);
  if (ctx == NULL) {
    LOG_ERROR("%s:%d Unable to retrieve device context, dev %d",
              __func__,
              __LINE__,
              dev_tgt.device_id);
    return BF_INVALID_ARG;
  }
  chnl_en = (ctx->ebuf_chnl_en[dev_tgt.dev_pipe_id]);
  // ebuf_port_ctrl: chnl pipe
  addr = offsetof(
      tof2_reg,
      pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.ebuf_port_ctrl[port / 2]);

  switch (chnl_numb) {
    case 2:
      // 1channel mode
      if ((port != 0) && (port != 2) && (port != 4) && (port != 6)) {
        LOG_ERROR("%s:%d Invalid port, dev %d, port 0x%x, channel number %d",
                  __func__,
                  __LINE__,
                  dev_tgt.device_id,
                  port,
                  chnl_numb);
        return BF_INVALID_ARG;
      }
      if (add && (((chnl_en & (1 << port)) != 0) ||
                  ((chnl_en & (1 << (port + 1))) != 0))) {
        LOG_ERROR(
            "%s:%d Invalid port, dev %d, pipe %d, port %d, channel number %d, "
            "existing enable mask 0x%x",
            __func__,
            __LINE__,
            dev_tgt.device_id,
            dev_tgt.dev_pipe_id,
            port,
            chnl_numb,
            chnl_en);
        return BF_INVALID_ARG;
      }
      if (!add && (((chnl_en & (1 << port)) == 0) ||
                   ((chnl_en & (1 << (port + 1))) == 0))) {
        LOG_ERROR(
            "%s:%d Invalid port, dev %d, pipe %d, port %d, channel number %d, "
            "existing enable mask 0x%x",
            __func__,
            __LINE__,
            dev_tgt.device_id,
            dev_tgt.dev_pipe_id,
            port,
            chnl_numb,
            chnl_en);
        return BF_INVALID_ARG;
      }
      data1 = 0;
      setp_tof2_pgr_ebuf_port_ctrl_port_en(&data1, (add ? 1 : 0));
      setp_tof2_pgr_ebuf_port_ctrl_channel_en(&data1, (add ? 0x3 : 0));
      setp_tof2_pgr_ebuf_port_ctrl_channel_mode(&data1, 0);
      ctx->ebuf_port_ctrl[dev_tgt.dev_pipe_id][port / 2] = data1;
      pg_write_one_pipe_reg(
          shdl, dev_tgt.device_id, 1 << dev_tgt.dev_pipe_id, addr, data1);
      if (add) {
        ctx->ebuf_chnl_en[dev_tgt.dev_pipe_id] |= (1u << port);
        ctx->ebuf_chnl_en[dev_tgt.dev_pipe_id] |= (1u << (port + 1));
      } else {
        ctx->ebuf_chnl_en[dev_tgt.dev_pipe_id] &= ~(1u << port);
        ctx->ebuf_chnl_en[dev_tgt.dev_pipe_id] &= ~(1u << (port + 1));
      }
      break;
    case 1:
      // 2channel mode
      pair_port = (port % 2 == 0) ? (port + 1) : (port - 1);
      data1 = 0;
      // check whether pair_port is enabled or not
      if (chnl_en & (1 << pair_port)) {
        setp_tof2_pgr_ebuf_port_ctrl_port_en(&data1, 1);
        setp_tof2_pgr_ebuf_port_ctrl_channel_mode(&data1, 1);
        // enabled
        uint32_t val = (add) ? 0x3 : ((pair_port > port) ? 0x2 : 0x1);
        setp_tof2_pgr_ebuf_port_ctrl_channel_en(&data1, val);
      } else {
        if (add) {
          setp_tof2_pgr_ebuf_port_ctrl_port_en(&data1,
                                               0);  // set channel mode first
          setp_tof2_pgr_ebuf_port_ctrl_channel_mode(&data1, 1);
          setp_tof2_pgr_ebuf_port_ctrl_channel_en(&data1,
                                                  (pair_port > port) ? 1 : 2);
          two_steps_op = true;
        } else {
          setp_tof2_pgr_ebuf_port_ctrl_channel_en(&data1, 0);
          setp_tof2_pgr_ebuf_port_ctrl_port_en(&data1, 0);
        }
      }
      if (add) {
        ctx->ebuf_chnl_en[dev_tgt.dev_pipe_id] |= (1u << port);
      } else {
        ctx->ebuf_chnl_en[dev_tgt.dev_pipe_id] &= ~(1u << port);
      }
      if (two_steps_op) {
        pg_write_one_pipe_reg(
            shdl, dev_tgt.device_id, 1 << dev_tgt.dev_pipe_id, addr, data1);
        setp_tof2_pgr_ebuf_port_ctrl_port_en(&data1, 1);  // set port en second
      }
      ctx->ebuf_port_ctrl[dev_tgt.dev_pipe_id][port / 2] = data1;
      pg_write_one_pipe_reg(
          shdl, dev_tgt.device_id, 1 << dev_tgt.dev_pipe_id, addr, data1);
      break;
    default:
      LOG_ERROR("%s:%d Invalid port, dev %d, port 0x%x, channel number %d",
                __func__,
                __LINE__,
                dev_tgt.device_id,
                port,
                chnl_numb);
      return BF_INVALID_ARG;
  }

  return BF_SUCCESS;
}

/* add recir port */
static bf_status_t pipe_mgr_tof2_pgr_recir_port_add(pipe_sess_hdl_t shdl,
                                                    bf_dev_target_t dev_tgt,
                                                    int port,
                                                    uint8_t chnl_numb,
                                                    bf_port_speeds_t speed,
                                                    bool add) {
  uint32_t port_end = (port + chnl_numb - 1);
  bf_status_t sts = BF_SUCCESS;
  // check
  if (pipe_mgr_tof2_get_speed(speed) > 100) {
    LOG_ERROR("%s:%d Invalid speed, not support > 100G recir port, dev %d",
              __func__,
              __LINE__,
              dev_tgt.device_id);
    return BF_INVALID_ARG;
  }
  // ebuf port ctrl
  sts =
      pipe_mgr_tof2_pgr_ebuf_port_ctrl_set(shdl, dev_tgt, port, chnl_numb, add);
  if (sts != BF_SUCCESS) return sts;
  // ipb port ctrl
  sts =
      pipe_mgr_tof2_pgr_ipb_ctrl_set(shdl, dev_tgt, port, port_end, add, speed);
  return sts;
}
static bf_status_t pipe_mgr_tof2_pgr_get_eth_mode_seq_rm(uint32_t *data_eth,
                                                         int port,
                                                         uint8_t chnl_numb,
                                                         uint32_t *mod_r,
                                                         uint32_t *seq_r) {
  uint32_t mode = 0, seq = 0;
  uint32_t old_mode = getp_tof2_pgr_eth_cpu_ctrl_channel_mode(data_eth);
  uint32_t old_seq = getp_tof2_pgr_eth_cpu_ctrl_channel_seq(data_eth);

  // get mode and seq
  switch (chnl_numb) {
    case 1:
      mode = old_mode;
      seq = old_seq;
      break;
    case 2:
      if (port == 2) {
        if (old_mode == 2) {
          mode = 4;
          seq = 0xd8;
        } else if (old_mode == 1) {
          mode = 3;
          seq = 0x98;
        } else {
          return BF_INVALID_ARG;
        }
      } else if (port == 4) {
        if (old_mode == 3) {
          mode = 4;
          seq = 0xd8;
        } else if (old_mode == 1) {
          mode = 2;
          seq = 0xc8;
        } else {
          return BF_INVALID_ARG;
        }
      } else {
        return BF_INVALID_ARG;
      }
      break;
    case 4:
      mode = 4;
      seq = 0xd8;
      break;
    default:
      return BF_INVALID_ARG;
  }
  *seq_r = seq;
  *mod_r = mode;
  return BF_SUCCESS;
}
static bf_status_t pipe_mgr_tof2_pgr_get_eth_mode_seq(uint32_t *data_eth,
                                                      int port,
                                                      uint8_t chnl_numb,
                                                      uint32_t *mod_r,
                                                      uint32_t *seq_r) {
  uint32_t mode = 0;
  uint32_t seq = 0;
  uint32_t old_seq = getp_tof2_pgr_eth_cpu_ctrl_channel_mode(data_eth);
  uint32_t recir_dis = getp_tof2_pgr_eth_cpu_ctrl_channel_en(data_eth);

  // get mode and seq
  switch (recir_dis) {
    case 0xf:
      if (chnl_numb == 2) {
        if (((old_seq >= 1) && (old_seq <= 3)) &&
            ((port == 2) || (port == 4))) {
          mode = 1;
          seq = 0x88;
        } else if (port == 2) {
          mode = 2;
          seq = 0xc8;
        } else if (port == 4) {
          mode = 3;
          seq = 0x98;
        } else {
          return BF_INVALID_ARG;
        }
      } else if (chnl_numb == 1) {
        if ((port == 2) || (port == 3)) {
          // mode 3 or 4
          if ((old_seq == 1) || (old_seq == 3)) {
            mode = 3;
            seq = 0x98;
          } else {
            mode = 4;
            seq = 0xd8;
          }
        } else if ((port == 4) || (port == 5)) {
          // mode 2 or 4
          if ((old_seq == 1) || (old_seq == 2)) {
            mode = 2;
            seq = 0xc8;
          } else {
            mode = 4;
            seq = 0xd8;
          }
        }
      }
      break;
    case 0x3:
    case 0xc:
      if (chnl_numb == 2) {
        mode = 1;
        seq = 0x88;
      } else if (chnl_numb == 1) {
        mode = 4;
        seq = 0xd8;
      } else {
        return BF_INVALID_ARG;
      }
      break;
    case 0x1:
    case 0x2:
    case 0x4:
    case 0x8:
    case 0x5:
    case 0xa:
    case 0x6:
    case 0x9:
      mode = 4;
      seq = 0xd8;
      break;
    case 0x7:
    case 0xb:
      // 0111/1011, can be 2 or 4
      if ((port == 2) || (port == 3)) {
        if ((port == 2) && (chnl_numb == 2)) {
          mode = 2;
          seq = 0xc8;
        } else {
          mode = 4;
          seq = 0xd8;
        }
      } else {
        if ((old_seq == 1) || (old_seq == 2)) {
          mode = 2;
          seq = 0xc8;
        } else {
          mode = 4;
          seq = 0xd8;
        }
      }
      break;
    case 0xd:
    case 0xe:
      // 1101/1110, can be 3 or 4
      if ((port == 4) || (port == 5)) {
        if ((port == 4) && (chnl_numb == 2)) {
          mode = 3;
          seq = 0x98;
        } else {
          mode = 4;
          seq = 0xd8;
        }
      } else {
        if ((old_seq == 1) || (old_seq == 3)) {
          mode = 3;
          seq = 0x98;
        } else {
          mode = 4;
          seq = 0xd8;
        }
      }
      break;
    default:
      return BF_INVALID_ARG;
  }
  *seq_r = seq;
  *mod_r = mode;
  return BF_SUCCESS;
}
/* add eth port */
static bf_status_t pipe_mgr_tof2_pgr_eth_port_add(pipe_sess_hdl_t shdl,
                                                  bf_dev_id_t dev,
                                                  bf_dev_port_t dev_port,
                                                  uint8_t chnl_numb,
                                                  bf_port_speeds_t speed,
                                                  bool add) {
  uint32_t addr, *data_eth;
  uint32_t chnl_en;
  uint32_t i, mode, seq;
  int port = DEV_PORT_TO_LOCAL_PORT(dev_port);
  int pipe = DEV_PORT_TO_PIPE(dev_port);
  uint32_t port_end = port + chnl_numb - 1;
  int pbm = 1 << pipe;
  bf_status_t sts;

  if (pipe_mgr_tof2_get_speed(speed) > 100) {
    LOG_ERROR(
        "Dev %d unsupported speed-enum %d for PGR eth %s port %d chnl-num %d",
        dev,
        speed,
        add ? "add" : "rmv",
        port,
        chnl_numb);
    return BF_INVALID_ARG;
  }
  if ((tbc_eth_en[dev] >> 2) != 0xf) {
    LOG_ERROR("Dev %d PGR eth en 0x%x illegal for %s port %d chnl-num %d",
              dev,
              tbc_eth_en[dev] >> 2,
              add ? "add" : "rmv",
              port,
              chnl_numb);
    return BF_INVALID_ARG;
  }
  // eth_cpu_port_ctrl: chnl speed
  addr = offsetof(
      tof2_reg, pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.eth_cpu_port_ctrl);
  data_eth = &(pipe_mgr_tof2_get_pgr_ctx(dev)->port_ctrl);
  chnl_en = getp_tof2_pgr_eth_cpu_ctrl_channel_en(data_eth);
  // get mode and seq
  if (add) {
    for (i = port; i <= port_end; i++) {
      chnl_en |= (1u << (i - 2));
    }
  } else {
    for (i = port; i <= port_end; i++) {
      chnl_en &= ~(1u << (i - 2));
    }
  }
  setp_tof2_pgr_eth_cpu_ctrl_channel_en(data_eth, chnl_en);
  if (add) {
    sts = pipe_mgr_tof2_pgr_get_eth_mode_seq(
        data_eth, port, chnl_numb, &mode, &seq);
  } else {
    sts = pipe_mgr_tof2_pgr_get_eth_mode_seq_rm(
        data_eth, port, chnl_numb, &mode, &seq);
  }
  if (sts != BF_SUCCESS) {
    LOG_ERROR(
        "%s:%d Get eth mode sequence error, dev %d, port 0x%x, channel number "
        "%d",
        __func__,
        __LINE__,
        dev,
        port,
        chnl_numb);
    return sts;
  }
  setp_tof2_pgr_eth_cpu_ctrl_channel_mode(data_eth, mode);
  setp_tof2_pgr_eth_cpu_ctrl_channel_seq(data_eth, seq);
  /* The channel mode must always be four; TF2LAB-41. */
  uint32_t wr_data = *data_eth;
  setp_tof2_pgr_eth_cpu_ctrl_channel_mode(&wr_data, 4);
  pg_write_one_pipe_reg(shdl, dev, pbm, addr, wr_data);

  // ebuf port ctrl
  bf_dev_target_t dev_tgt = {dev, 0};
  if ((chnl_numb == 1) || (chnl_numb == 2)) {
    sts = pipe_mgr_tof2_pgr_ebuf_port_ctrl_set(
        shdl, dev_tgt, port, chnl_numb, add);
  } else if ((chnl_numb == 4) && (port == 2)) {
    sts = pipe_mgr_tof2_pgr_ebuf_port_ctrl_set(shdl, dev_tgt, port, 2, add);
    sts |=
        pipe_mgr_tof2_pgr_ebuf_port_ctrl_set(shdl, dev_tgt, port + 2, 2, add);
  } else {
    LOG_ERROR("%s:%d Invalid port channel number %d, dev %d, port %d",
              __func__,
              __LINE__,
              chnl_numb,
              dev,
              port);
    return BF_INVALID_ARG;
  }
  if (sts != BF_SUCCESS) return sts;

  // ipb port ctrl: chnl speed
  sts =
      pipe_mgr_tof2_pgr_ipb_ctrl_set(shdl, dev_tgt, port, port_end, add, speed);
  return sts;
}

/* add tbc port */
static bf_status_t pipe_mgr_tof2_pgr_tbc_port_add(bf_session_hdl_t shdl,
                                                  bf_dev_id_t dev,
                                                  bf_port_speeds_t speed,
                                                  bool add) {
  // check speed
  if (pipe_mgr_tof2_get_speed(speed) > 25) {
    LOG_ERROR("%s:%d Invalid speed, not support > 25G tbc port, dev %d",
              __func__,
              __LINE__,
              dev);
    return BF_INVALID_ARG;
  }
  bf_dev_port_t pcie_port = bf_pcie_cpu_port_get(dev);
  bf_dev_target_t dev_tgt = {dev, DEV_PORT_TO_PIPE(pcie_port)};
  int local_port = DEV_PORT_TO_LOCAL_PORT(pcie_port);
  // ipb port ctrl: chnl speed
  bool recirc_enable = add ? (tbc_eth_en[dev] & 1) == 0 : true;
  pipe_mgr_tof2_pgr_tbc_port_recir_en(shdl, dev_tgt, recirc_enable, false);
  bf_status_t sts =
      pipe_mgr_tof2_pgr_ebuf_port_ctrl_set(shdl, dev_tgt, local_port, 1, add);
  if (sts != BF_SUCCESS) return sts;
  sts = pipe_mgr_tof2_pgr_ipb_ctrl_set(
      shdl, dev_tgt, local_port, local_port, add, BF_SPEED_25G);
  return sts;
}

static bool is_port_eth(bf_dev_id_t dev, bf_dev_port_t port) {
  const bf_dev_port_t eth_cpu = bf_eth_cpu_port_get(dev);
  return (port < (eth_cpu + 4)) && (port >= eth_cpu);
}

static bool is_port_pcie(bf_dev_id_t dev, bf_dev_port_t port) {
  const bf_dev_port_t pcie_cpu = bf_pcie_cpu_port_get(dev);
  return port == pcie_cpu;
}

/* Tofino2
 * add pgr(recirc, tbc, eth cpu) port
 */
bf_status_t pipe_mgr_tof2_pktgen_port_add(rmt_dev_info_t *dev_info,
                                          bf_dev_port_t port_id,
                                          bf_port_speeds_t speed) {
  bf_dev_id_t dev = dev_info->dev_id;
  bf_dev_pipe_t pipe = dev_info->dev_cfg.dev_port_to_pipe(port_id);
  bf_dev_target_t dev_tgt = {dev, pipe};
  int port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  char *speed_str = NULL;
  bf_status_t sts = bf_port_speed_to_str(speed, &speed_str);
  if (BF_SUCCESS != sts) {
    LOG_ERROR(
        "%s: Dev %d invalid speed %d port %d", __func__, dev, speed, port_id);
    return BF_INVALID_ARG;
  }

  LOG_TRACE("%s: dev %d pipe %d port %d speed %s",
            __func__,
            dev,
            pipe,
            port,
            speed_str);
  /* Use the default session to set this up. */
  pipe_sess_hdl_t sid = pipe_mgr_ctx->int_ses_hndl;

  /* Local port numbers 8-71 do not require any special PGR programming, they
   * only need to be tracked in the port down mask. */
  if (port <= 7) {
    const bool is_recirc = pipe_mgr_tof2_pgr_recir_get(dev, port_id);
    const uint8_t chnl_numb = pipe_mgr_tof2_get_chnls(speed, !is_recirc);
    /* Ports 2-5 require special handling depending if they are connected to the
     * Ethernet CPU MAC or are in recirculation mode. */
    if (is_port_eth(dev, port_id)) {
      const int max_chnl = is_recirc ? 2 : 4;
      if ((chnl_numb == 0) || (chnl_numb > max_chnl)) {
        LOG_ERROR(
            "%s:%d Invalid speed %s; %s port %d on device %d cannot "
            "use speeds greater than 100g (i.e. %d channels at the most).",
            __func__,
            __LINE__,
            speed_str,
            is_recirc ? "recirculation" : "CPU",
            port_id,
            dev,
            max_chnl);
        return BF_INVALID_ARG;
      }

      if (!is_recirc) {
        /* Port is connected to the Ethernet MAC, add it using 25g channels. */
        sts = pipe_mgr_tof2_pgr_eth_port_add(
            sid, dev, port_id, chnl_numb, speed, true);
      }
    }

    /* Port 0 requires special handling depending if it is connected to the PCIe
     * (TBus) path or the recirculation path. */
    else if (is_port_pcie(dev, port_id)) {
      const bool is_chnl_num_err =
          is_recirc ? (chnl_numb == 0) || (chnl_numb > 2) : chnl_numb != 1;
      if (is_chnl_num_err) {
        LOG_ERROR(
            "%s:%d Invalid speed %s; port %d on device %d cannot use speeds "
            "greater than %s.",
            __func__,
            __LINE__,
            speed_str,
            port_id,
            dev,
            is_recirc ? "100g" : "25g when recirculation is disabled");
        return BF_INVALID_ARG;
      }

      if (!is_recirc) {
        /* Recirculation is not enabled therefore port 0 is connected to TBUS
         * for
         * packet tx and rx over PCIe. */
        sts = pipe_mgr_tof2_pgr_tbc_port_add(sid, dev, speed, true);
      }
    }

    /* The remaining ports (1, 6, 7 and 0 (but only 0 in pipes 1-3) are always
     * recirculation ports and use 50g channels. */
    else {
      const bool is_odd = (port_id & 1); /* Odd port numbers. */
      const bool is_chnl_num_err =
          is_odd ? (chnl_numb != 1) : (chnl_numb == 0) || (chnl_numb > 2);
      if (is_chnl_num_err) {
        LOG_ERROR(
            "%s:%d Invalid speed %s; port %d on device %d cannot use speeds "
            "greater than %s.",
            __func__,
            __LINE__,
            speed_str,
            port_id,
            dev,
            is_odd ? "50g (i.e. single channel only)"
                   : "100g (i.e. two channels at the most)");
        return BF_INVALID_ARG;
      }
    }
    if (is_recirc)
      sts = pipe_mgr_tof2_pgr_recir_port_add(
          sid, dev_tgt, port, chnl_numb, speed, true);
  }

  if (BF_SUCCESS == sts) {
    /* Update our mask of created ports. */
    uint8_t *m_mask = pipe_mgr_tof2_get_pgr_ctx(dev)
                          ->port_down_mask[dev_tgt.dev_pipe_id][2]
                          .port_mask;
    /* Special case ports 2-5 (ethernet CPU) since it maps to bits 4-7 instead
     * of bits 2-5.  */
    if (dev_tgt.dev_pipe_id == 0 && port >= 2 && port <= 5)
      m_mask[0] |= 1 << (4 + port - 2);
    else
      m_mask[port / 8] |= 1 << (port % 8);
  }
  return sts;
}

bf_status_t pipe_mgr_tof2_pktgen_port_rem(rmt_dev_info_t *dev_info,
                                          bf_dev_port_t port_id) {
  bf_dev_id_t dev = dev_info->dev_id;
  rmt_port_info_t *port_info = pipe_mgr_get_port_info(dev, port_id);
  if (!port_info) return BF_SUCCESS;
  bf_port_speeds_t speed = port_info->speed;
  uint8_t chnl_numb = 0;
  bf_dev_pipe_t pipe = dev_info->dev_cfg.dev_port_to_pipe(port_id);
  int port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  pipe_sess_hdl_t sid = pipe_mgr_ctx->int_ses_hndl;

  /* Update our mask of created ports. */
  uint8_t *m_mask =
      pipe_mgr_tof2_get_pgr_ctx(dev)->port_down_mask[pipe][2].port_mask;
  /* Special case port 2, ethernet CPU since it maps to bit 4 instead of bit 2.
   */
  if (pipe == 0 && port == 2)
    m_mask[0] &= ~(1 << 4);
  else
    m_mask[port / 8] &= ~(1 << (port % 8));

  bf_status_t sts = BF_SUCCESS;
  if (port > 7) return sts;

  bool recirc = pipe_mgr_tof2_pgr_recir_get(dev, port_id);
  chnl_numb = pipe_mgr_tof2_get_chnls(speed, !recirc);
  // have to clear ipb_chnl_sp ipb_ctrl
  if (is_port_eth(dev, port_id) && !recirc) {
    // remove eth port
    // eth port chnl disable
    sts = pipe_mgr_tof2_pgr_eth_port_add(
        sid, dev, port_id, chnl_numb, speed, false);
  } else if (is_port_pcie(dev, port_id) && !recirc) {
    // remove tbc port
    sts = pipe_mgr_tof2_pgr_tbc_port_add(sid, dev, speed, false);
  } else {
    // remove recirc port
    // ebuf chnl disable
    dev_target_t dev_tgt = {dev, pipe};
    sts = pipe_mgr_tof2_pgr_recir_port_add(
        sid, dev_tgt, port, chnl_numb, speed, false);
  }
  return sts;
}

bf_status_t pipe_mgr_tof2_recir_en(bf_session_hdl_t shdl,
                                   bf_dev_id_t dev,
                                   bf_dev_port_t port,
                                   bool en) {
  bf_dev_port_t tbc_port = bf_pcie_cpu_port_get(dev);
  bf_dev_port_t eth_port_min = bf_eth_cpu_port_get(dev);
  if (eth_port_min == -1 || tbc_port == -1) return BF_NOT_READY;

  bf_dev_port_t eth_port_max = eth_port_min + 3;
  bf_dev_port_t local_port = DEV_PORT_TO_LOCAL_PORT(port);
  bf_dev_pipe_t cpu_pipe = DEV_PORT_TO_PIPE(tbc_port);

  /* Handle the TBC port specially. */
  if (port == tbc_port) {
    if (pipe_mgr_get_port_info(dev, port)) {
      LOG_ERROR(
          "%s: Port 0 is in use cannot %sable recirculation, dev %d port %d",
          __func__,
          en ? "en" : "dis",
          dev,
          port);
      return BF_IN_USE;
    }
    if (en) {
      tbc_eth_en[dev] &= ~(1u);
    } else {
      tbc_eth_en[dev] |= 1;
    }
    return BF_SUCCESS;
  }

  /* Ports other than CPU-Eth have nothing to do. */
  if (!(port >= eth_port_min && port <= eth_port_max)) return BF_SUCCESS;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    LOG_ERROR("%s: Failed to get device info, dev %d ", __func__, dev);
    PIPE_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  uint8_t in_use = 0;
  for (bf_dev_port_t i = eth_port_min; i <= eth_port_max; ++i) {
    if (pipe_mgr_get_port_info(dev, i)) {
      int ch = DEV_PORT_TO_LOCAL_PORT(i);
      /* The port is in use but only log it if it has a different recirc mode
       * than what is being requested. */
      if (((~tbc_eth_en[dev] >> ch) & 1) != en) in_use |= (1 << ch);
    }
  }
  if (in_use) {
    LOG_ERROR(
        "%s:%d Port(s) %s%s%s%s%s%s%s are in use and have recirculation %s.  "
        "Cannot %s recirculation on dev %d port %d.",
        __func__,
        __LINE__,
        in_use & (1 << 2) ? "2" : "",
        in_use & 0xF8 ? " " : "",
        in_use & (1 << 3) ? "3" : "",
        in_use & 0xF0 ? " " : "",
        in_use & (1 << 4) ? "4" : "",
        in_use & 0xE0 ? "  " : "",
        in_use & (1 << 5) ? "5" : "",
        en ? "disabled" : "enabled",
        en ? "enable" : "disable",
        dev,
        port);
    return BF_IN_USE;
  }
  LOG_TRACE("Dev %d port %d PGR recirc %d", dev, port, en);
  if (en) {
    tbc_eth_en[dev] &= ~(1u << local_port);
  } else {
    tbc_eth_en[dev] |= (1u << local_port);
  }

  /* Get the current value of eth_cpu_port_ctrl. */
  uint32_t *data = &pipe_mgr_tof2_get_pgr_ctx(dev)->port_ctrl;
  uint32_t addr = offsetof(
      tof2_reg, pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.eth_cpu_port_ctrl);

  /* Get the current recirc mode from it. */
  bool current = 0 == (*data & 1);

  /* Update it if the modes are different. */
  if (current != en) {
    setp_tof2_pgr_eth_cpu_ctrl_port_en(data, en ? 0 : 1);
    pg_write_one_pipe_reg(shdl, dev, 1u << cpu_pipe, addr, *data);
  }

  return BF_SUCCESS;
}
bool pipe_mgr_tof2_pgr_recir_get(bf_dev_id_t dev, bf_dev_port_t port) {
  bf_dev_port_t tbc_port = bf_pcie_cpu_port_get(dev);
  bf_dev_port_t eth_port_min = bf_eth_cpu_port_get(dev);
  bf_dev_port_t eth_port_max = eth_port_min + 3;
  bf_dev_port_t local_port = DEV_PORT_TO_LOCAL_PORT(port);
  if (port == tbc_port || (port >= eth_port_min && port <= eth_port_max)) {
    return ((tbc_eth_en[dev] >> local_port) & 1) == 0;
  }
  return true;
}
bf_status_t pipe_mgr_tof2_pktgen_en(bf_session_hdl_t shdl,
                                    bf_dev_id_t dev,
                                    bf_dev_port_t port,
                                    bool en) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    LOG_ERROR("%s : Failed to get device info, dev %d ", __func__, dev);
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  bf_dev_port_t tbc_port = bf_pcie_cpu_port_get(dev);
  bf_dev_port_t eth_port_min = bf_eth_cpu_port_get(dev);
  bf_dev_port_t eth_port_max = eth_port_min + 3;

  int pipe_id = dev_info->dev_cfg.dev_port_to_pipe(port);
  int port_id = dev_info->dev_cfg.dev_port_to_local_port(port);
  bf_dev_target_t dev_tgt = {dev, pipe_id};
  if (port_id < 0 || port_id > 7) {
    LOG_ERROR(
        "pktgen-enable/disable dev %d port %d; only ports 0-7 of each pipe can "
        "be enabled for packet-generation.",
        dev,
        port);
    return BF_INVALID_ARG;
  }
  if (pipe_id >= (int)dev_info->num_active_pipes) {
    LOG_ERROR("Dev %d port %d would be on pipe %d but dev only has %d pipes",
              dev,
              port,
              pipe_id,
              dev_info->num_active_pipes);
    return BF_INVALID_ARG;
  }
  if (en && port == tbc_port && (tbc_eth_en[dev] & 1)) {
    /* Already exists as the PCIe CPU port so pktgen cannot be enabled. */
    return BF_INVALID_ARG;
  }
  if (port >= eth_port_min && port <= eth_port_max &&
      ((tbc_eth_en[dev] >> port_id) & 1)) {
    /* Already exists as an Eth CPU port so pktgen cannot be enabled. */
    return (en) ? BF_INVALID_ARG : BF_SUCCESS;
  }

  // Cannot enable/disable one channel in a port
  if (!pipe_mgr_tof2_ipb_conflict_check(dev_tgt, port_id)) {
    LOG_ERROR("%s:%d Invalid parameter, dev %d, port 0x%x, %s",
              __func__,
              __LINE__,
              dev,
              port,
              (en ? "enable" : "disable"));
    return BF_INVALID_ARG;
  }
  uint32_t *data, addr, data1;
  uint32_t pgen_chnl_en, seq;
  // Configure registers. no change on ipb_seq.
  bf_status_t sts;
  addr = offsetof(
      tof2_reg,
      pipes[0]
          .pardereg.pgstnreg.pgrreg.pgr_common.ipb_port_ctrl.ipb_port_ctrl_0_2);
  data = &(pipe_mgr_tof2_get_pgr_ctx(dev)->ipb_ctrl[pipe_id]);
  pgen_chnl_en =
      getp_tof2_pgr_ipb_port_ctrl_ipb_port_ctrl_0_2_pgen_channel_en(data);
  if (en) {
    pgen_chnl_en |= 1u << port_id;
  } else {
    pgen_chnl_en &= ~(1u << port_id);
  }
  setp_tof2_pgr_ipb_port_ctrl_ipb_port_ctrl_0_2_pgen_channel_en(data,
                                                                pgen_chnl_en);
  // ipb_ctrl only shadows bit0:31 of the wide register
  // recalculate here to get the same value
  sts = pipe_mgr_tof2_get_ipb_seq(dev_tgt, &seq);
  if (sts != BF_SUCCESS) return sts;
  setp_tof2_pgr_ipb_port_ctrl_ipb_port_ctrl_0_2_channel_seq_7_0(data,
                                                                seq & 0xff);
  pg_write_one_pipe_reg(shdl, dev, 1 << pipe_id, addr, *data);
  addr = offsetof(
      tof2_reg,
      pipes[0]
          .pardereg.pgstnreg.pgrreg.pgr_common.ipb_port_ctrl.ipb_port_ctrl_1_2);
  data1 = 0;
  setp_tof2_pgr_ipb_port_ctrl_ipb_port_ctrl_1_2_channel_seq_23_8(&data1,
                                                                 seq >> 8);
  pg_write_one_pipe_reg(shdl, dev, 1 << pipe_id, addr, data1);
  return BF_SUCCESS;
}

bf_status_t pipe_mgr_tof2_pktgen_get_port_en(rmt_dev_info_t *dev_info,
                                             bf_dev_port_t port,
                                             bool *is_enabled) {
  int pipe_id = dev_info->dev_cfg.dev_port_to_pipe(port);
  int port_id = dev_info->dev_cfg.dev_port_to_local_port(port);
  uint32_t pgen_chnl_en;
  int pipe_cnt = pipe_mgr_get_num_active_pipes(dev_info->dev_id);
  if (pipe_id >= pipe_cnt) {
    LOG_ERROR("%s:%d Invalid port, dev %d, pipe %d, port %d",
              __func__,
              __LINE__,
              dev_info->dev_id,
              pipe_id,
              port_id);
    return BF_INVALID_ARG;
  }
  if (port_id < 0 || port_id > 7) {
    *is_enabled = false;
    return BF_SUCCESS;
  }
  uint32_t data =
      (pipe_mgr_tof2_get_pgr_ctx(dev_info->dev_id)->ipb_ctrl[pipe_id]);
  pgen_chnl_en =
      getp_tof2_pgr_ipb_port_ctrl_ipb_port_ctrl_0_2_pgen_channel_en(&data);
  *is_enabled = (pgen_chnl_en & (1u << port_id));

  return BF_SUCCESS;
}

bf_status_t pipe_mgr_tof2_pktgen_pgr_com_port_down_clr_get(
    bf_dev_target_t dev_tgt, int local_port_bit_idx, bool *is_cleared) {
  const uint32_t addr = get_port_down_dis_addr(local_port_bit_idx);
  uint32_t data = 0;
  if ((dev_tgt.dev_pipe_id == 0) &&
      (local_port_bit_idx >= 2 && local_port_bit_idx <= 5)) {
    /* Special case ports 2-5 (ethernet CPU) since it maps to bits 4-7 instead
     * of bits 2-5.  */
    local_port_bit_idx += 2;
  }
  bf_status_t sts = pg_tof2_read_one_pipe_reg(dev_tgt, addr, &data);
  if (BF_SUCCESS != sts) return BF_HW_COMM_FAIL;
  *is_cleared = ((data >> (local_port_bit_idx % 32)) & 0x1) != 0x1;
  return BF_SUCCESS;
}

bf_status_t pipe_mgr_tof2_pktgen_pgr_com_port_down_clr(pipe_sess_hdl_t sid,
                                                       bf_dev_target_t dev_tgt,
                                                       int local_port_bit_idx) {
  if ((dev_tgt.dev_pipe_id == 0) &&
      (local_port_bit_idx >= 2 && local_port_bit_idx <= 5)) {
    /* Special case ports 2-5 (ethernet CPU) since it maps to bits 4-7 instead
     * of bits 2-5.  */
    local_port_bit_idx += 2;
  }
  uint32_t data = 1u << (local_port_bit_idx % 32);
  const uint32_t addr = get_port_down_dis_addr(local_port_bit_idx);
  return pg_write_one_pipe_reg(
      sid, dev_tgt.device_id, 1 << dev_tgt.dev_pipe_id, addr, data);
}

void pipe_mgr_tof2_pktgen_txn_commit(bf_dev_id_t dev) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    LOG_ERROR("%s: Failed to get device info, dev %d ", __func__, dev);
    return;
  }
  pipe_mgr_tof2_pg_dev_ctx *ctx = pipe_mgr_tof2_get_pgr_ctx(dev);
  if (ctx == NULL) {
    LOG_ERROR("%s:%d Unable to retrieve device context, dev %d",
              __func__,
              __LINE__,
              dev);
    return;
  }

  ctx->app.b_valid = false;
  for (uint32_t i = 0; i < dev_info->num_active_pipes; ++i) {
    ctx->pkt_buffer_shadow[i].txn_data_valid = false;
  }
  return;
}

void pipe_mgr_tof2_pktgen_txn_abort(bf_dev_id_t dev,
                                    int max_app_id,
                                    int active_pipes) {
  int p, i;
  const uint32_t buf_sz = PIPE_MGR_PKT_BUFFER_SIZE;
  struct pkt_buffer_shadow_t *pkt_buf_shadow = NULL;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    LOG_ERROR("%s: Failed to get device info, dev %d ", __func__, dev);
    return;
  }
  pipe_mgr_tof2_pg_dev_ctx *ctx = pipe_mgr_tof2_get_pgr_ctx(dev);
  if (ctx == NULL) {
    LOG_ERROR("%s:%d Unable to retrieve device context, dev %d",
              __func__,
              __LINE__,
              dev);
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

void pipe_mgr_tof2_pkt_buffer_shadow_mem_update(bf_dev_target_t dev_tgt,
                                                uint32_t offset,
                                                const uint8_t *buf,
                                                uint32_t size,
                                                bool txn) {
  const uint32_t buf_sz = PIPE_MGR_PKT_BUFFER_SIZE;
  PIPE_MGR_DBGCHK(offset < buf_sz);
  PIPE_MGR_DBGCHK(size <= buf_sz - offset);

  int pipe_mask = pg_log_pipe_mask(dev_tgt);
  struct pkt_buffer_shadow_t *pkt_buf_shadow = NULL;
  pipe_mgr_tof2_pg_dev_ctx *ctx = pipe_mgr_tof2_get_pgr_ctx(dev_tgt.device_id);
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

bf_status_t pipe_mgr_tof2_pkt_buffer_shadow_mem_get(bf_dev_target_t dev_tgt,
                                                    uint32_t offset,
                                                    uint8_t *buf,
                                                    uint32_t size) {
  bf_dev_pipe_t i =
      (dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL) ? 0 : dev_tgt.dev_pipe_id;
  struct pipe_mgr_tof2_pg_dev_ctx *c =
      pipe_mgr_tof2_get_pgr_ctx(dev_tgt.device_id);
  if (c == NULL) {
    LOG_ERROR("%s:%d Unable to retrieve device context, dev %d",
              __func__,
              __LINE__,
              dev_tgt.device_id);
    return BF_INVALID_ARG;
  }
  PIPE_MGR_MEMCPY(buf, &c->pkt_buffer_shadow[i].data[offset], size);
  return BF_SUCCESS;
}

bf_status_t pipe_mgr_tof2_pkt_buffer_write_from_shadow(
    bf_session_hdl_t shdl, bf_dev_target_t dev_tgt) {
  uint64_t addr = 0;
  uint32_t buf_sz = 0, size = 0, elem_size = 0, count = 0;
  bf_status_t sts = BF_SUCCESS;
  pipe_mgr_drv_ses_state_t *st;
  st = pipe_mgr_drv_get_ses_state(&shdl, __func__, __LINE__);
  if (!st) {
    return PIPE_INVALID_ARG;
  }
  /* Form the address using the physical pipe rather than the logical pipe. */
  int log_pipe_msk = pg_log_pipe_mask(dev_tgt);
  sts = pipe_mgr_pkt_buffer_tof2_addr_get(&addr, &elem_size, &count);
  /* Write the entire 16K pkt buffer */
  size = elem_size * count;
  if (sts != BF_SUCCESS) {
    LOG_ERROR("%s:%d Get memory info error, dev %d, elem_size 0x%x, count 0x%x",
              __func__,
              __LINE__,
              dev_tgt.device_id,
              elem_size,
              count);
    return sts;
  }
  buf_sz = pipe_mgr_drv_buf_size(dev_tgt.device_id, PIPE_MGR_DRV_BUF_BWR);
  if ((size != PIPE_MGR_PKT_BUFFER_SIZE) || (size > buf_sz)) {
    LOG_ERROR("%s:%d Get memory info error, dev %d, elem_size 0x%x, count 0x%x",
              __func__,
              __LINE__,
              dev_tgt.device_id,
              elem_size,
              count);
    return PIPE_INVALID_ARG;
  }
  /* Since each pipe might have different buffer contents write them
   * individually. */
  for (int i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(log_pipe_msk & (1 << i))) continue;
    pipe_mgr_drv_buf_t *b = pipe_mgr_drv_buf_alloc(
        st->sid, dev_tgt.device_id, buf_sz, PIPE_MGR_DRV_BUF_BWR, true);
    if (!b) {
      LOG_ERROR("%s:%d Error allocating buffer for dev %d",
                __func__,
                __LINE__,
                dev_tgt.device_id);
      return BF_NO_SYS_RESOURCES;
    }
    PIPE_MGR_MEMCPY(b->addr,
                    &(pipe_mgr_tof2_get_pgr_ctx(dev_tgt.device_id)
                          ->pkt_buffer_shadow[dev_tgt.dev_pipe_id]
                          .data),
                    size);

    pipe_status_t s = pipe_mgr_drv_blk_wr(&shdl,
                                          PIPE_MGR_PKT_BUFFER_WIDTH,
                                          PIPE_MGR_PKT_BUFFER_MEM_ROWS,
                                          1,
                                          addr,
                                          1 << dev_tgt.dev_pipe_id,
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

static bf_status_t pipe_mgr_tof2_pktgen_reg_write_mem_with_ilist(
    bf_session_hdl_t shdl,
    bf_dev_target_t dev_tgt,
    int row,
    int cnt,
    uint64_t base,
    uint64_t step) {
  pipe_bitmap_t pbm = {{0}};
  rmt_dev_info_t *dev_info = NULL;

  PIPE_BITMAP_INIT(&pbm, PIPE_BMP_SIZE);
  uint32_t pipe_cnt = pipe_mgr_get_num_active_pipes(dev_tgt.device_id);
  for (uint32_t p = 0; p < pipe_cnt; ++p) {
    if (BF_DEV_PIPE_ALL == dev_tgt.dev_pipe_id || dev_tgt.dev_pipe_id == p) {
      PIPE_BITMAP_SET(&pbm, p);
    }
  }
  dev_info = pipe_mgr_get_dev_info(dev_tgt.device_id);
  if (dev_info == NULL) {
    LOG_ERROR("%s:%d Device info for device id %d not found",
              __func__,
              __LINE__,
              dev_tgt.device_id);
    return BF_OBJECT_NOT_FOUND;
  }

  /* Reset pipe so that it can be used to index the shadow. */
  if (BF_DEV_PIPE_ALL == dev_tgt.dev_pipe_id) dev_tgt.dev_pipe_id = 0;

  uint32_t stage;
  lld_err_t lld_err = lld_sku_get_prsr_stage(dev_tgt.device_id, &stage);
  if (LLD_OK != lld_err) {
    LOG_ERROR("Cannot get pgen stage, error %d, from %s", lld_err, __func__);
    PIPE_MGR_DBGCHK(LLD_OK == lld_err);
    return BF_UNEXPECTED;
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
                                 &pipe_mgr_tof2_get_pgr_ctx(dev_tgt.device_id)
                                      ->pkt_buffer_shadow[dev_tgt.dev_pipe_id]
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

bf_status_t pipe_mgr_tof2_pktgen_write_pkt_buffer(bf_session_hdl_t shdl,
                                                  bf_dev_target_t dev_tgt,
                                                  int row,
                                                  int num_rows) {
  bf_status_t sts = BF_SUCCESS;
  uint32_t elem_size = 0, count = 0;
  uint64_t addr = 0;
  sts = pipe_mgr_pkt_buffer_tof2_addr_get(&addr, &elem_size, &count);
  /* Write the data to asic from shadow mem */
  sts |= pipe_mgr_tof2_pktgen_reg_write_mem_with_ilist(
      shdl, dev_tgt, row, num_rows, addr, elem_size / 16);

  return sts;
}

bf_status_t pipe_mgr_tof2_pktgen_reg_app_batch_ctr(bf_dev_target_t dev_tgt,
                                                   int aid,
                                                   uint64_t *val) {
  bf_status_t sts = BF_SUCCESS;
  uint32_t hi = 0, lo = 0;

  uint32_t addrLo = offsetof(tof2_reg,
                             pipes[0]
                                 .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                                 .ctr48_batch.ctr48_batch_0_2);
  uint32_t addrHi = offsetof(tof2_reg,
                             pipes[0]
                                 .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                                 .ctr48_batch.ctr48_batch_1_2);
  sts = pg_tof2_read_one_pipe_reg(dev_tgt, addrLo, &lo);
  if (BF_SUCCESS != sts) return BF_HW_COMM_FAIL;
  sts = pg_tof2_read_one_pipe_reg(dev_tgt, addrHi, &hi);
  if (BF_SUCCESS != sts) return BF_HW_COMM_FAIL;

  uint64_t x = hi;
  uint64_t y = lo;
  *val = (x << 32) | (y & UINT64_C(0xFFFFFFFF));

  return BF_SUCCESS;
}

bf_status_t pipe_mgr_tof2_pktgen_reg_app_pkt_ctr(bf_dev_target_t dev_tgt,
                                                 int aid,
                                                 uint64_t *val) {
  bf_status_t sts = BF_SUCCESS;
  uint32_t hi = 0, lo = 0;
  uint32_t addrLo = offsetof(tof2_reg,
                             pipes[0]
                                 .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                                 .ctr48_packet.ctr48_packet_0_2);
  uint32_t addrHi = offsetof(tof2_reg,
                             pipes[0]
                                 .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                                 .ctr48_packet.ctr48_packet_1_2);
  sts = pg_tof2_read_one_pipe_reg(dev_tgt, addrLo, &lo);
  if (BF_SUCCESS != sts) return BF_HW_COMM_FAIL;
  sts = pg_tof2_read_one_pipe_reg(dev_tgt, addrHi, &hi);
  if (BF_SUCCESS != sts) return BF_HW_COMM_FAIL;

  uint64_t x = hi;
  uint64_t y = lo;
  *val = (x << 32) | (y & UINT64_C(0xFFFFFFFF));
  return BF_SUCCESS;
}

bf_status_t pipe_mgr_tof2_pktgen_reg_app_trig_ctr(bf_dev_target_t dev_tgt,
                                                  int aid,
                                                  uint64_t *val) {
  bf_status_t sts = BF_SUCCESS;
  uint32_t hi = 0, lo = 0;

  uint32_t addrLo = offsetof(tof2_reg,
                             pipes[0]
                                 .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                                 .ctr48_trigger.ctr48_trigger_0_2);
  uint32_t addrHi = offsetof(tof2_reg,
                             pipes[0]
                                 .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                                 .ctr48_trigger.ctr48_trigger_1_2);
  sts = pg_tof2_read_one_pipe_reg(dev_tgt, addrLo, &lo);
  if (BF_SUCCESS != sts) return BF_HW_COMM_FAIL;
  sts = pg_tof2_read_one_pipe_reg(dev_tgt, addrHi, &hi);
  if (BF_SUCCESS != sts) return BF_HW_COMM_FAIL;

  uint64_t x = hi;
  uint64_t y = lo;
  *val = (x << 32) | (y & UINT64_C(0xFFFFFFFF));
  return BF_SUCCESS;
}

bf_status_t pipe_mgr_tof2_pktgen_reg_app_batch_ctr_set(bf_session_hdl_t shdl,
                                                       bf_dev_target_t dev_tgt,
                                                       int aid,
                                                       uint64_t val) {
  bf_status_t sts = BF_SUCCESS;
  uint32_t hi = val >> 32;
  uint32_t lo = val & UINT64_C(0xFFFFFFFF);
  int pipe_mask = pg_log_pipe_mask(dev_tgt);

  uint32_t addrLo = offsetof(tof2_reg,
                             pipes[0]
                                 .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                                 .ctr48_batch.ctr48_batch_0_2);
  uint32_t addrHi = offsetof(tof2_reg,
                             pipes[0]
                                 .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                                 .ctr48_batch.ctr48_batch_1_2);
  sts = pg_write_one_pipe_reg(shdl, dev_tgt.device_id, pipe_mask, addrLo, lo);
  if (BF_SUCCESS != sts) return BF_HW_COMM_FAIL;
  sts = pg_write_one_pipe_reg(shdl, dev_tgt.device_id, pipe_mask, addrHi, hi);
  if (BF_SUCCESS != sts) return BF_HW_COMM_FAIL;
  return BF_SUCCESS;
}

bf_status_t pipe_mgr_tof2_pktgen_reg_app_pkt_ctr_set(bf_session_hdl_t shdl,
                                                     bf_dev_target_t dev_tgt,
                                                     int aid,
                                                     uint64_t val) {
  bf_status_t sts = BF_SUCCESS;
  uint32_t hi = val >> 32;
  uint32_t lo = val & UINT64_C(0xFFFFFFFF);
  int pipe_mask = pg_log_pipe_mask(dev_tgt);

  uint32_t addrLo = offsetof(tof2_reg,
                             pipes[0]
                                 .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                                 .ctr48_packet.ctr48_packet_0_2);
  uint32_t addrHi = offsetof(tof2_reg,
                             pipes[0]
                                 .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                                 .ctr48_packet.ctr48_packet_1_2);
  sts = pg_write_one_pipe_reg(shdl, dev_tgt.device_id, pipe_mask, addrLo, lo);
  if (BF_SUCCESS != sts) return BF_HW_COMM_FAIL;
  sts = pg_write_one_pipe_reg(shdl, dev_tgt.device_id, pipe_mask, addrHi, hi);
  if (BF_SUCCESS != sts) return BF_HW_COMM_FAIL;
  return BF_SUCCESS;
}

bf_status_t pipe_mgr_tof2_pktgen_reg_app_trig_ctr_set(bf_session_hdl_t shdl,
                                                      bf_dev_target_t dev_tgt,
                                                      int aid,
                                                      uint64_t val) {
  bf_status_t sts = BF_SUCCESS;
  uint32_t hi = val >> 32;
  uint32_t lo = val & UINT64_C(0xFFFFFFFF);
  int pipe_mask = pg_log_pipe_mask(dev_tgt);

  uint32_t addrLo = offsetof(tof2_reg,
                             pipes[0]
                                 .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                                 .ctr48_trigger.ctr48_trigger_0_2);
  uint32_t addrHi = offsetof(tof2_reg,
                             pipes[0]
                                 .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                                 .ctr48_trigger.ctr48_trigger_1_2);
  sts = pg_write_one_pipe_reg(shdl, dev_tgt.device_id, pipe_mask, addrLo, lo);
  if (BF_SUCCESS != sts) return BF_HW_COMM_FAIL;
  sts = pg_write_one_pipe_reg(shdl, dev_tgt.device_id, pipe_mask, addrHi, hi);
  if (BF_SUCCESS != sts) return BF_HW_COMM_FAIL;
  return BF_SUCCESS;
}

static void pipe_mgr_tof2_pg_txn_bkup_app_cfg(pipe_sess_hdl_t shdl,
                                              bf_dev_id_t dev) {
  pipe_mgr_tof2_pg_dev_ctx *ctx = pipe_mgr_tof2_get_pgr_ctx(dev);
  if (ctx == NULL) {
    LOG_ERROR("%s:%d Unable to retrieve device context, dev %d",
              __func__,
              __LINE__,
              dev);
    return;
  }
  if (!pipe_mgr_sess_in_txn(shdl) || ctx->app.b_valid) return;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    LOG_ERROR("%s: Failed to get device info, dev %d ", __func__, dev);
    PIPE_MGR_DBGCHK(0);
    return;
  }
  // rmt_dev_cfg_t *cfg = &dev_info->dev_cfg;
  ctx->app.b_valid = true;
  for (uint32_t p = 0; p < dev_info->num_active_pipes; ++p) {
    for (uint32_t i = 0; i < PIPE_MGR_TOF2_PKTGEN_APP_CNT; ++i) {
      PIPE_MGR_MEMCPY(
          &ctx->app.b[p][i], &ctx->app.a[p][i], sizeof ctx->app.b[p][i]);
    }
  }
}

bf_status_t pipe_mgr_tof2_pktgen_get_app_ctrl_en(bf_dev_target_t dev_tgt,
                                                 int app_id,
                                                 bool *is_enabled) {
  int i, pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t val = 0;
  uint32_t en_tmp[BF_PIPE_COUNT] = {0};
  int val_cnt = 0;
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1 << i))) continue;
    val =
        pipe_mgr_tof2_get_pgr_ctx(dev_tgt.device_id)->app.a[i][app_id].app_ctrl;
    en_tmp[val_cnt++] = getp_tof2_pgr_app_ctrl_app_en(&val);
  }
  val = en_tmp[0];
  for (i = 0; i < val_cnt; ++i) {
    if (val != en_tmp[i]) {
      LOG_ERROR(
          "Error, cannot get asymmetric data to multiple pipes. "
          "%#x != %#x dev %d pipe %#x at %s:%d",
          val,
          en_tmp[i],
          dev_tgt.device_id,
          dev_tgt.dev_pipe_id,
          __func__,
          __LINE__);
      return BF_INVALID_ARG;
    }
  }
  *is_enabled = ((val == 0) ? false : true);
  return BF_SUCCESS;
}

bf_status_t pipe_mgr_tof2_pktgen_reg_pgr_app_ctrl_en(pipe_sess_hdl_t sid,
                                                     bf_dev_target_t dev_tgt,
                                                     int aid,
                                                     bool en) {
  bf_status_t sts = BF_SUCCESS;
  int i, pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t vals[BF_PIPE_COUNT] = {0}, val = 0, addr = 0;
  int val_cnt = 0;
  pipe_mgr_tof2_pg_dev_ctx *ctx = pipe_mgr_tof2_get_pgr_ctx(dev_tgt.device_id);
  if (ctx == NULL) {
    LOG_ERROR("%s:%d Unable to retrieve device context, dev %d",
              __func__,
              __LINE__,
              dev_tgt.device_id);
    return BF_INVALID_ARG;
  }
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1 << i))) continue;
    vals[val_cnt] = ctx->app.a[i][aid].app_ctrl;
    setp_tof2_pgr_app_ctrl_app_en(&vals[val_cnt], en);
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

  int log_pipe =
      dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL ? 0 : dev_tgt.dev_pipe_id;

  pipe_mgr_tof2_pg_txn_bkup_app_cfg(sid, dev_tgt.device_id);
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1 << i))) continue;
    ctx->app.a[i][aid].app_ctrl = val;
  }
  addr =
      offsetof(tof2_reg, pipes[0].pardereg.pgstnreg.pgrreg.pgr_app[aid].ctrl);
  sts = pg_write_one_pipe_reg(sid, dev_tgt.device_id, pipe_mask, addr, val);
  if (sts != BF_SUCCESS) return sts;

  /* If the app is a port-down app and the port down replay mode is set then
   * toggle the enable in pgen_retrigger_port_down after enabling the app. */
  if (getp_tof2_pgr_app_ctrl_app_type(&vals[log_pipe]) ==
          BF_PKTGEN_TRIGGER_PORT_DOWN &&
      en) {
    uint32_t x = 0;
    bool symmertic = true;
    rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_tgt.device_id);
    if (!dev_info) return BF_INVALID_ARG;
    const int pipe_num = dev_info->num_active_pipes;
    if (pipe_num < 1) return BF_UNEXPECTED;
    bf_pktgen_port_down_mode_t mode[pipe_num];

    if (BF_DEV_PIPE_ALL == dev_tgt.dev_pipe_id) {
      for (i = 0; i < (int)pipe_num; ++i) {
        mode[i] = ctx->port_down_mode[0];
        if (mode[0] != ctx->port_down_mode[i]) {
          symmertic = false;
        }
      }
    } else {
      mode[0] = ctx->port_down_mode[dev_tgt.dev_pipe_id];
      symmertic = true;
    }

    addr = offsetof(
        tof2_reg,
        pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.pgen_retrigger_port_down);

    if (symmertic) {
      x = 0;
      switch (mode[0]) {
        case BF_PKTGEN_PORT_DOWN_REPLAY_NONE:
          break;
        case BF_PKTGEN_PORT_DOWN_REPLAY_MISSED:
          x = 1;
          break;
        case BF_PKTGEN_PORT_DOWN_REPLAY_ALL:
          x = 3;
          break;
      }
      if (x) {
        sts = pg_write_one_pipe_reg(
            sid, dev_tgt.device_id, pipe_mask, addr, x & ~1u);
        if (sts != BF_SUCCESS) return sts;
        sts = pg_write_one_pipe_reg(sid, dev_tgt.device_id, pipe_mask, addr, x);
        if (sts != BF_SUCCESS) return sts;
      }
    } else {
      for (i = 0; i < (int)dev_info->num_active_pipes; ++i) {
        if (!(pipe_mask & (1 << i))) continue;
        x = 0;
        switch (mode[i]) {
          case BF_PKTGEN_PORT_DOWN_REPLAY_NONE:
            break;
          case BF_PKTGEN_PORT_DOWN_REPLAY_MISSED:
            x = 1;
            break;
          case BF_PKTGEN_PORT_DOWN_REPLAY_ALL:
            x = 3;
            break;
        }
        if (x) {
          sts = pg_write_one_pipe_reg(
              sid, dev_tgt.device_id, 1u << i, addr, x & ~1u);
          if (sts != BF_SUCCESS) return sts;
          sts = pg_write_one_pipe_reg(sid, dev_tgt.device_id, 1u << i, addr, x);
          if (sts != BF_SUCCESS) return sts;
        }
      }
    }
  }
  return sts;
}

static uint32_t pipe_mgr_tof2_pktgen_app_ctrl_type(bf_dev_target_t dev_tgt,
                                                   int aid) {
  pipe_mgr_tof2_pg_dev_ctx *c = pipe_mgr_tof2_get_pgr_ctx(dev_tgt.device_id);
  if (c == NULL) {
    LOG_ERROR("%s:%d Unable to retrieve device context, dev %d",
              __func__,
              __LINE__,
              dev_tgt.device_id);
    return 0;
  }
  return getp_tof2_pgr_app_ctrl_app_type(
      &c->app.a[dev_tgt.dev_pipe_id][aid].app_ctrl);
}

static uint32_t pipe_mgr_tof2_pktgen_app_ctrl_en(bf_dev_target_t dev_tgt,
                                                 int aid) {
  struct pipe_mgr_tof2_pg_dev_ctx *c =
      pipe_mgr_tof2_get_pgr_ctx(dev_tgt.device_id);
  if (c == NULL) {
    LOG_ERROR("%s:%d Unable to retrieve device context, dev %d",
              __func__,
              __LINE__,
              dev_tgt.device_id);
    return 0;
  }
  return getp_tof2_pgr_app_ctrl_app_en(
      &c->app.a[dev_tgt.dev_pipe_id][aid].app_ctrl);
}

bf_status_t pipe_mgr_tof2_pktgen_cfg_app_conf_check(bf_dev_target_t dev_tgt,
                                                    int app_id,
                                                    bf_pktgen_app_cfg_t *cfg) {
  bf_dev_id_t dev = dev_tgt.device_id;
  uint32_t a = pipe_mgr_get_num_active_pipes(dev);
  /* Cannot change trigger_type while the app is enabled. */
  if (BF_DEV_PIPE_ALL == dev_tgt.dev_pipe_id) {
    for (uint32_t i = 0; i < a; ++i) {
      bf_dev_target_t d_tgt = {dev, i};
      uint32_t x = pipe_mgr_tof2_pktgen_app_ctrl_type(d_tgt, app_id);
      uint32_t y = pipe_mgr_tof2_pktgen_app_ctrl_en(d_tgt, app_id);
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
    uint32_t x = pipe_mgr_tof2_pktgen_app_ctrl_type(dev_tgt, app_id);
    uint32_t y = pipe_mgr_tof2_pktgen_app_ctrl_en(dev_tgt, app_id);
    if (y && x != cfg->trigger_type) {
      LOG_ERROR("%s:%d Application in use, dev %d, app_id %d",
                __func__,
                __LINE__,
                dev,
                app_id);
      return BF_IN_USE;
    }
  }
  if (app_id >= PIPE_MGR_TOF2_PKTGEN_APP_CNT) {
    LOG_ERROR("%s:%d Invalid application id, dev %d, app_id %d",
              __func__,
              __LINE__,
              dev,
              app_id);
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
  if (cfg->tof2.assigned_chnl_id > 7) {
    LOG_ERROR(
        "%s:%d Invalid chnl id, cannot be > 7, dev %d, app_id %d, chnl_id %d",
        __func__,
        __LINE__,
        dev,
        app_id,
        cfg->tof2.assigned_chnl_id);
    return BF_INVALID_ARG;
  }
  if (cfg->increment_source_port &&
      cfg->tof2.source_port_wrap_max > PIPE_MGR_PKTGEN_SRC_PRT_MAX) {
    LOG_ERROR(
        "%s:%d Invalid increment port, dev %d, app_id %d, increment "
        "source port 0x%x, source port wrap 0x%x, source port 0x%x",
        __func__,
        __LINE__,
        dev,
        app_id,
        cfg->increment_source_port,
        cfg->tof2.source_port_wrap_max,
        cfg->pipe_local_source_port);
    return BF_INVALID_ARG;
  }
  if (cfg->increment_source_port &&
      cfg->packets_per_batch >
          (PIPE_MGR_PKTGEN_SRC_PRT_MAX - cfg->pipe_local_source_port)) {
    LOG_ERROR(
        "%s:%d Invalid increment port info, dev %d, app_id %d, increment "
        "source port 0x%x, pkt per batch 0x%x, source port 0x%x",
        __func__,
        __LINE__,
        dev,
        app_id,
        cfg->increment_source_port,
        cfg->packets_per_batch,
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
  if ((cfg->pkt_buffer_offset & 0xF) && !cfg->tof2.offset_len_from_recir_pkt) {
    LOG_ERROR("%s:%d Packet buffer offset (0x%x) must be 16B aligned",
              __func__,
              __LINE__,
              cfg->pkt_buffer_offset);
    return BF_INVALID_ARG;
  }

  /* Hardware will add 6 bytes of pgen header and 4 bytes of CRC to packet
   * length. */
  if ((cfg->length + 6 + 4 < PIPE_MGR_TOF2_PKTGEN_PKT_LEN_MIN) &&
      !cfg->tof2.offset_len_from_recir_pkt) {
    LOG_ERROR("%s:%d Packet length, %d, is too small.  Must be at least %d.",
              __func__,
              __LINE__,
              cfg->length,
              PIPE_MGR_TOF2_PKTGEN_PKT_LEN_MIN - 6 - 4);
    return BF_INVALID_ARG;
  }
  /* check for pfc event */
  if (cfg->trigger_type == BF_PKTGEN_TRIGGER_PFC) {
    if (cfg->u.pfc_cfg.cfg_timer_en && (0 == cfg->u.pfc_cfg.cfg_timer)) {
      LOG_ERROR(
          "%s:%d Invalid PFC timer, timer enable. Timer value must be non-zero",
          __func__,
          __LINE__);
      return BF_INVALID_ARG;
    }
    if (cfg->u.pfc_cfg.pfc_max_msgs == 0 ||
        cfg->u.pfc_cfg.pfc_max_msgs >= 1024) {
      LOG_ERROR(
          "%s:%d Invalid PFC max message count %d, expected range is 1-1023",
          __func__,
          __LINE__,
          cfg->u.pfc_cfg.pfc_max_msgs);
      return BF_INVALID_ARG;
    }
  }
  return BF_SUCCESS;
}

static bf_status_t pipe_mgr_tof2_pktgen_reg_pgr_app_ctrl(
    pipe_sess_hdl_t sid,
    bf_dev_target_t dev_tgt,
    int aid,
    int chnl,
    int mask_sel,
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
                                        ? "Dprsr"
                                        : type == BF_PKTGEN_TRIGGER_PFC
                                              ? "PFC"
                                              : "Unknown");
  int i, pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t *data = NULL;
  uint32_t addr =
      offsetof(tof2_reg, pipes[0].pardereg.pgstnreg.pgrreg.pgr_app[aid].ctrl);
  pipe_mgr_tof2_pg_dev_ctx *ctx = pipe_mgr_tof2_get_pgr_ctx(dev_tgt.device_id);
  if (ctx == NULL) {
    LOG_ERROR("%s:%d Unable to retrieve device context, dev %d",
              __func__,
              __LINE__,
              dev_tgt.device_id);
    return BF_INVALID_ARG;
  }
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1u << i))) continue;
    data = &ctx->app.a[i][aid].app_ctrl;
    if (getp_tof2_pgr_app_ctrl_app_en(data) == 1) {
      LOG_ERROR(
          "Error, cannot program to a pipe while an application has already "
          "been enabled dev %d pipe %x error_pipe %d at %s:%d",
          dev_tgt.device_id,
          dev_tgt.dev_pipe_id,
          i,
          __func__,
          __LINE__);
      return BF_INVALID_ARG;
    }
  }
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1u << i))) continue;
    data = &ctx->app.a[i][aid].app_ctrl;
    setp_tof2_pgr_app_ctrl_app_type(data, type);
    setp_tof2_pgr_app_ctrl_app_chnl(data, chnl);
    setp_tof2_pgr_app_ctrl_app_stop_at_pkt_bndry(
        data, 1);  // fix to stop at pkt boundry
    if (type == BF_PKTGEN_TRIGGER_PORT_DOWN) {
      setp_tof2_pgr_app_ctrl_app_port_down_mask_sel(data, mask_sel);
    }
  }
  if (data == NULL) return BF_INVALID_ARG;
  pipe_mgr_tof2_pg_txn_bkup_app_cfg(sid, dev_tgt.device_id);
  return pg_write_one_pipe_reg(sid, dev_tgt.device_id, pipe_mask, addr, *data);
}

static bf_status_t pipe_mgr_tof2_pktgen_reg_pgr_app_payload_ctrl(
    pipe_sess_hdl_t sid,
    bf_dev_target_t dev_tgt,
    int aid,
    uint16_t start,
    uint16_t size,
    bool set) {
  LOG_TRACE(
      "PktGenAppCfg: dev %d pipe %d app %d shdl %d, Payload offset/size %#x "
      "%#x, extract from recir %s",
      dev_tgt.device_id,
      dev_tgt.dev_pipe_id,
      aid,
      sid,
      start,
      size,
      (set ? "yes" : "no"));
  int pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t *data = NULL;
  uint32_t addr = offsetof(
      tof2_reg, pipes[0].pardereg.pgstnreg.pgrreg.pgr_app[aid].payload_ctrl);
  for (int i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1u << i))) continue;
    data = &pipe_mgr_tof2_get_pgr_ctx(dev_tgt.device_id)
                ->app.a[i][aid]
                .payload_ctrl;
    setp_tof2_pgr_app_payload_ctrl_app_recirc_extract(data, (set ? 1 : 0));
    setp_tof2_pgr_app_payload_ctrl_app_payload_size(data, (set ? 0 : size));
    setp_tof2_pgr_app_payload_ctrl_app_payload_addr(data, (set ? 0 : start));
  }
  if (data == NULL) return BF_INVALID_ARG;
  pipe_mgr_tof2_pg_txn_bkup_app_cfg(sid, dev_tgt.device_id);
  return pg_write_one_pipe_reg(sid, dev_tgt.device_id, pipe_mask, addr, *data);
}

static bf_status_t pipe_mgr_tof2_pktgen_reg_pgr_app_ing_port_ctrl(
    pipe_sess_hdl_t sid,
    bf_dev_target_t dev_tgt,
    int aid,
    uint16_t port,
    bool inc,
    uint8_t wrap) {
  LOG_TRACE(
      "PktGenAppCfg: dev %d pipe %d app %d shdl %d Port %d inc %d wrap_max %d",
      dev_tgt.device_id,
      dev_tgt.dev_pipe_id,
      aid,
      sid,
      port,
      inc,
      wrap);
  int i, pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t *data = NULL, addr;
  addr = offsetof(
      tof2_reg, pipes[0].pardereg.pgstnreg.pgrreg.pgr_app[aid].ingr_port_ctrl);
  pipe_mgr_tof2_pg_txn_bkup_app_cfg(sid, dev_tgt.device_id);
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1u << i))) continue;
    data =
        &pipe_mgr_tof2_get_pgr_ctx(dev_tgt.device_id)->app.a[i][aid].ing_port;
    setp_tof2_pgr_app_ingr_port_ctrl_app_ingr_port_pipe_id(data, i);
    setp_tof2_pgr_app_ingr_port_ctrl_app_ingr_port(data, port);
    setp_tof2_pgr_app_ingr_port_ctrl_app_ingr_port_wrap(data, (inc ? wrap : 0));
    setp_tof2_pgr_app_ingr_port_ctrl_app_ingr_port_inc(data, (inc ? 1 : 0));
    bf_status_t sts =
        pg_write_one_pipe_reg(sid, dev_tgt.device_id, 1 << i, addr, *data);
    if (BF_SUCCESS != sts) return sts;
  }
  return BF_SUCCESS;
}

static bf_status_t pipe_mgr_tof2_pktgen_reg_pgr_app_recir_match_value(
    pipe_sess_hdl_t sid,
    bf_dev_target_t dev_tgt,
    int aid,
    uint8_t *key,
    bool set) {
  LOG_TRACE(
      "PktGenAppCfg: dev %d pipe %d app %d shdl %d, Recirc Key %#x %#x %#x "
      "%#x %#x %#x %#x %#x %#x %#x %#x %#x %#x %#x %#x %#x ",
      dev_tgt.device_id,
      dev_tgt.dev_pipe_id,
      aid,
      sid,
      key[0],
      key[1],
      key[2],
      key[3],
      key[4],
      key[5],
      key[6],
      key[7],
      key[8],
      key[9],
      key[10],
      key[11],
      key[12],
      key[13],
      key[14],
      key[15]);
  int pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t *data = NULL;
  bf_status_t sts = BF_SUCCESS;
  pipe_mgr_tof2_pg_dev_ctx *ctx = pipe_mgr_tof2_get_pgr_ctx(dev_tgt.device_id);
  if (ctx == NULL) {
    LOG_ERROR("%s:%d Unable to retrieve device context, dev %d",
              __func__,
              __LINE__,
              dev_tgt.device_id);
    return BF_INVALID_ARG;
  }
  uint32_t addresses[4];
  addresses[0] = offsetof(tof2_reg,
                          pipes[0]
                              .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                              .recir_match_value.recir_match_value_0_4);
  addresses[1] = offsetof(tof2_reg,
                          pipes[0]
                              .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                              .recir_match_value.recir_match_value_1_4);
  addresses[2] = offsetof(tof2_reg,
                          pipes[0]
                              .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                              .recir_match_value.recir_match_value_2_4);
  addresses[3] = offsetof(tof2_reg,
                          pipes[0]
                              .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                              .recir_match_value.recir_match_value_3_4);

  for (int i = 0; i < 4; ++i) {
    for (int p = 0; p < BF_PIPE_COUNT; ++p) {
      if (!(pipe_mask & (1u << p))) continue;
      data = &ctx->app.a[p][aid].recir_val[i];
      *data = 0;
      if (set) {
        int tmp = i * 4;
        *data = (key[12 - tmp] << 24) | (key[13 - tmp] << 16) |
                (key[14 - tmp] << 8) | key[15 - tmp];
      }
    }
    if (data == NULL) return BF_INVALID_ARG;
    sts |= pg_write_one_pipe_reg(
        sid, dev_tgt.device_id, pipe_mask, addresses[i], *data);
  }

  pipe_mgr_tof2_pg_txn_bkup_app_cfg(sid, dev_tgt.device_id);
  return sts;
}

static bf_status_t pipe_mgr_tof2_pktgen_reg_pgr_app_recir_match_mask(
    pipe_sess_hdl_t sid,
    bf_dev_target_t dev_tgt,
    int aid,
    uint8_t *mask,
    bool set) {
  LOG_TRACE(
      "PktGenAppCfg: dev %d pipe %d app %d shdl %d, Recirc mask %#x %#x %#x "
      "%#x %#x %#x %#x %#x %#x %#x %#x %#x %#x %#x %#x %#x ",
      dev_tgt.device_id,
      dev_tgt.dev_pipe_id,
      aid,
      sid,
      ~mask[0] & 0xFF,
      ~mask[1] & 0xFF,
      ~mask[2] & 0xFF,
      ~mask[3] & 0xFF,
      ~mask[4] & 0xFF,
      ~mask[5] & 0xFF,
      ~mask[6] & 0xFF,
      ~mask[7] & 0xFF,
      ~mask[8] & 0xFF,
      ~mask[9] & 0xFF,
      ~mask[10] & 0xFF,
      ~mask[11] & 0xFF,
      ~mask[12] & 0xFF,
      ~mask[13] & 0xFF,
      ~mask[14] & 0xFF,
      ~mask[15] & 0xFF);
  int pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t *data = NULL;
  bf_status_t sts = BF_SUCCESS;
  pipe_mgr_tof2_pg_dev_ctx *ctx = pipe_mgr_tof2_get_pgr_ctx(dev_tgt.device_id);
  if (ctx == NULL) {
    LOG_ERROR("%s:%d Unable to retrieve device context, dev %d",
              __func__,
              __LINE__,
              dev_tgt.device_id);
    return BF_INVALID_ARG;
  }
  uint32_t addresses[4];
  addresses[0] = offsetof(tof2_reg,
                          pipes[0]
                              .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                              .recir_match_mask.recir_match_mask_0_4);
  addresses[1] = offsetof(tof2_reg,
                          pipes[0]
                              .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                              .recir_match_mask.recir_match_mask_1_4);
  addresses[2] = offsetof(tof2_reg,
                          pipes[0]
                              .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                              .recir_match_mask.recir_match_mask_2_4);
  addresses[3] = offsetof(tof2_reg,
                          pipes[0]
                              .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                              .recir_match_mask.recir_match_mask_3_4);

  for (int i = 0; i < 4; ++i) {
    for (int p = 0; p < BF_PIPE_COUNT; ++p) {
      if (!(pipe_mask & (1u << p))) continue;
      data = &ctx->app.a[p][aid].recir_msk[i];
      *data = 0;
      if (set) {
        int tmp = i * 4;
        *data = ~((mask[12 - tmp] << 24) | (mask[13 - tmp] << 16) |
                  (mask[14 - tmp] << 8) | mask[15 - tmp]);
      }
    }
    if (data == NULL) return BF_INVALID_ARG;
    sts |= pg_write_one_pipe_reg(
        sid, dev_tgt.device_id, pipe_mask, addresses[i], *data);
  }

  pipe_mgr_tof2_pg_txn_bkup_app_cfg(sid, dev_tgt.device_id);
  return sts;
}

static bf_status_t pipe_mgr_tof2_pktgen_reg_pgr_app_event_number(
    pipe_sess_hdl_t sid,
    bf_dev_target_t dev_tgt,
    int aid,
    bf_pktgen_trigger_type_e trigger_type,
    uint16_t pkt_num,
    uint16_t batch_num) {
  LOG_TRACE("PktGenAppCfg: dev %d pipe %d app %d shdl %d, Batch %d Pkt %d",
            dev_tgt.device_id,
            dev_tgt.dev_pipe_id,
            aid,
            sid,
            batch_num,
            pkt_num);
  if (trigger_type == BF_PKTGEN_TRIGGER_PORT_DOWN) {
    if (batch_num) {
      LOG_ERROR(
          "PktGenAppCfg: dev %d app %d batch %d, must use batch_count of zero "
          "for port down triggers",
          dev_tgt.device_id,
          aid,
          batch_num);
      return BF_INVALID_ARG;
    }
  } else if (trigger_type == BF_PKTGEN_TRIGGER_PFC) {
    if (batch_num || pkt_num) {
      LOG_ERROR(
          "PktGenAppCfg: dev %d app %d batch %d packet %d, must use "
          "batch_count and packet_count of zero for PFC triggers",
          dev_tgt.device_id,
          aid,
          batch_num,
          pkt_num);
      return BF_INVALID_ARG;
    }
  }
  int pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t *data = NULL, addr;
  pipe_mgr_tof2_pg_dev_ctx *ctx = pipe_mgr_tof2_get_pgr_ctx(dev_tgt.device_id);
  if (ctx == NULL) {
    LOG_ERROR("%s:%d Unable to retrieve device context, dev %d",
              __func__,
              __LINE__,
              dev_tgt.device_id);
    return BF_INVALID_ARG;
  }
  addr = offsetof(tof2_reg,
                  pipes[0].pardereg.pgstnreg.pgrreg.pgr_app[aid].event_number);
  for (int i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1u << i))) continue;
    data = &ctx->app.a[i][aid].event_num;
    setp_tof2_pgr_app_event_number_packet_num(data, pkt_num);
    setp_tof2_pgr_app_event_number_batch_num(data, batch_num);
  }
  if (data == NULL) return BF_INVALID_ARG;
  pipe_mgr_tof2_pg_txn_bkup_app_cfg(sid, dev_tgt.device_id);
  return pg_write_one_pipe_reg(sid, dev_tgt.device_id, pipe_mask, addr, *data);
}

static bf_status_t pipe_mgr_tof2_pktgen_reg_pgr_app_ibg_base(
    pipe_sess_hdl_t sid, bf_dev_target_t dev_tgt, int aid, uint32_t ibg) {
  LOG_TRACE("PktGenAppCfg: dev %d pipe %d app %d shdl %d, IBG %d",
            dev_tgt.device_id,
            dev_tgt.dev_pipe_id,
            aid,
            sid,
            ibg);
  int i, pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t *data = NULL, addr;
  pipe_mgr_tof2_pg_dev_ctx *ctx = pipe_mgr_tof2_get_pgr_ctx(dev_tgt.device_id);
  if (ctx == NULL) {
    LOG_ERROR("%s:%d Unable to retrieve device context, dev %d",
              __func__,
              __LINE__,
              dev_tgt.device_id);
    return BF_INVALID_ARG;
  }
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1u << i))) continue;
    data = &ctx->app.a[i][aid].ibg;
    setp_tof2_pgr_app_event_base_jitter_value_value(data, ibg);
  }
  if (data == NULL) return BF_INVALID_ARG;
  pipe_mgr_tof2_pg_txn_bkup_app_cfg(sid, dev_tgt.device_id);
  addr = offsetof(tof2_reg,
                  pipes[0]
                      .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                      .event_ibg_jitter_base_value);
  return pg_write_one_pipe_reg(sid, dev_tgt.device_id, pipe_mask, addr, *data);
}

static void pipe_mgr_tof2_pktgen_jitter_cal(uint32_t jval,
                                            uint8_t *scale,
                                            uint8_t *max) {
  if (!scale || !max) return;
  *scale = *max = 0;
  if (jval == 0) return;
  int i;
  for (i = 31; i >= 0; i--) {
    if ((jval >> i) != 0) break;
  }
  if (i > 7) {
    *max = (jval >> (i - 7)) & 0xff;
    *scale = (i - 7);
  } else {
    for (int j = 0; j <= i; j++) {
      *max |= (((jval >> j) & 0x1) << j);
    }
  }
}

static bf_status_t pipe_mgr_tof2_pktgen_reg_pgr_app_ibg_jitter(
    pipe_sess_hdl_t sid, bf_dev_target_t dev_tgt, int aid, uint32_t jval) {
  LOG_TRACE("PktGenAppCfg: dev %d pipe %d app %d shdl %d, IBG-Jitter %#x",
            dev_tgt.device_id,
            dev_tgt.dev_pipe_id,
            aid,
            sid,
            jval);
  int i, pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t *data = NULL, addr;
  uint8_t scale, max;
  bf_status_t sts = BF_SUCCESS;
  pipe_mgr_tof2_pg_dev_ctx *ctx = pipe_mgr_tof2_get_pgr_ctx(dev_tgt.device_id);
  if (ctx == NULL) {
    LOG_ERROR("%s:%d Unable to retrieve device context, dev %d",
              __func__,
              __LINE__,
              dev_tgt.device_id);
    return BF_INVALID_ARG;
  }
  pipe_mgr_tof2_pktgen_jitter_cal(jval, &scale, &max);
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1u << i))) continue;
    data = &ctx->app.a[i][aid].ibg_jit_max;
    *data = max;
  }
  if (data == NULL) return BF_INVALID_ARG;
  addr = offsetof(
      tof2_reg,
      pipes[0].pardereg.pgstnreg.pgrreg.pgr_app[aid].event_max_ibg_jitter);
  sts |= pg_write_one_pipe_reg(sid, dev_tgt.device_id, pipe_mask, addr, *data);

  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1u << i))) continue;
    data = &ctx->app.a[i][aid].ibg_jit_scale;
    *data = scale;
  }
  addr = offsetof(
      tof2_reg,
      pipes[0].pardereg.pgstnreg.pgrreg.pgr_app[aid].event_ibg_jitter_scale);
  sts |= pg_write_one_pipe_reg(sid, dev_tgt.device_id, pipe_mask, addr, *data);

  pipe_mgr_tof2_pg_txn_bkup_app_cfg(sid, dev_tgt.device_id);
  return sts;
}

static bf_status_t pipe_mgr_tof2_pktgen_reg_pgr_app_ipg_base(
    pipe_sess_hdl_t sid, bf_dev_target_t dev_tgt, int aid, uint32_t ipg) {
  LOG_TRACE("PktGenAppCfg: dev %d pipe %d app %d shdl %d, IPG %d",
            dev_tgt.device_id,
            dev_tgt.dev_pipe_id,
            aid,
            sid,
            ipg);
  int pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t *data = NULL, addr;
  pipe_mgr_tof2_pg_dev_ctx *ctx = pipe_mgr_tof2_get_pgr_ctx(dev_tgt.device_id);
  if (ctx == NULL) {
    LOG_ERROR("%s:%d Unable to retrieve device context, dev %d",
              __func__,
              __LINE__,
              dev_tgt.device_id);
    return BF_INVALID_ARG;
  }
  for (int i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1u << i))) continue;
    data = &ctx->app.a[i][aid].ipg;
    setp_tof2_pgr_app_event_base_jitter_value_value(data, ipg);
  }
  if (data == NULL) return BF_INVALID_ARG;
  pipe_mgr_tof2_pg_txn_bkup_app_cfg(sid, dev_tgt.device_id);
  addr = offsetof(tof2_reg,
                  pipes[0]
                      .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                      .event_ipg_jitter_base_value);
  return pg_write_one_pipe_reg(sid, dev_tgt.device_id, pipe_mask, addr, *data);
}

static bf_status_t pipe_mgr_tof2_pktgen_reg_pgr_app_ipg_jitter(
    pipe_sess_hdl_t sid, bf_dev_target_t dev_tgt, int aid, uint32_t jval) {
  LOG_TRACE("PktGenAppCfg: dev %d pipe %d app %d shdl %d, IPG-Jitter %#x",
            dev_tgt.device_id,
            dev_tgt.dev_pipe_id,
            aid,
            sid,
            jval);

  int i, pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t *data = NULL, addr;
  uint8_t scale, max;
  bf_status_t sts = BF_SUCCESS;
  pipe_mgr_tof2_pg_dev_ctx *ctx = pipe_mgr_tof2_get_pgr_ctx(dev_tgt.device_id);
  if (ctx == NULL) {
    LOG_ERROR("%s:%d Unable to retrieve device context, dev %d",
              __func__,
              __LINE__,
              dev_tgt.device_id);
    return BF_INVALID_ARG;
  }
  pipe_mgr_tof2_pktgen_jitter_cal(jval, &scale, &max);
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1u << i))) continue;
    data = &ctx->app.a[i][aid].ipg_jit_max;
    *data = max;
  }
  if (data == NULL) return BF_INVALID_ARG;
  addr = offsetof(
      tof2_reg,
      pipes[0].pardereg.pgstnreg.pgrreg.pgr_app[aid].event_max_ipg_jitter);
  sts |= pg_write_one_pipe_reg(sid, dev_tgt.device_id, pipe_mask, addr, *data);

  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1u << i))) continue;
    data = &ctx->app.a[i][aid].ipg_jit_scale;
    *data = scale;
  }
  addr = offsetof(
      tof2_reg,
      pipes[0].pardereg.pgstnreg.pgrreg.pgr_app[aid].event_ipg_jitter_scale);
  sts |= pg_write_one_pipe_reg(sid, dev_tgt.device_id, pipe_mask, addr, *data);

  pipe_mgr_tof2_pg_txn_bkup_app_cfg(sid, dev_tgt.device_id);
  return sts;
}

static bf_status_t pipe_mgr_tof2_pktgen_reg_pgr_app_event_timer(
    pipe_sess_hdl_t sid, bf_dev_target_t dev_tgt, int aid, uint32_t tval) {
  LOG_TRACE("PktGenAppCfg: dev %d pipe %d app %d shdl %d, Timer %d",
            dev_tgt.device_id,
            dev_tgt.dev_pipe_id,
            aid,
            sid,
            tval);
  int pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t *data = NULL, addr;
  pipe_mgr_tof2_pg_dev_ctx *ctx = pipe_mgr_tof2_get_pgr_ctx(dev_tgt.device_id);
  if (ctx == NULL) {
    LOG_ERROR("%s:%d Unable to retrieve device context, dev %d",
              __func__,
              __LINE__,
              dev_tgt.device_id);
    return BF_INVALID_ARG;
  }
  for (int i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1u << i))) continue;
    data = &ctx->app.a[i][aid].event_timer;
    setp_tof2_pgr_app_event_timer_timer_count(data, tval);
  }
  if (data == NULL) return BF_INVALID_ARG;
  pipe_mgr_tof2_pg_txn_bkup_app_cfg(sid, dev_tgt.device_id);
  addr = offsetof(tof2_reg,
                  pipes[0].pardereg.pgstnreg.pgrreg.pgr_app[aid].event_timer);
  return pg_write_one_pipe_reg(sid, dev_tgt.device_id, pipe_mask, addr, *data);
}

static bf_status_t pipe_mgr_tof2_pktgen_reg_pgr_app_recirc_src(
    pipe_sess_hdl_t sid, bf_dev_target_t dev_tgt, int aid, int src_port) {
  LOG_TRACE("PktGenAppCfg: dev %d pipe %d app %d shdl %d, recirc src chnl %d",
            dev_tgt.device_id,
            dev_tgt.dev_pipe_id,
            aid,
            sid,
            src_port / 2);
  int i, pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t *data, addr;
  pipe_mgr_tof2_pg_dev_ctx *ctx = pipe_mgr_tof2_get_pgr_ctx(dev_tgt.device_id);
  if (ctx == NULL) {
    LOG_ERROR("%s:%d Unable to retrieve device context, dev %d",
              __func__,
              __LINE__,
              dev_tgt.device_id);
    return BF_INVALID_ARG;
  }
  addr =
      offsetof(tof2_reg,
               pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.cfg_app_recirc_src);
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1u << i))) continue;
    data = &(ctx->app_recirc_src[i]);
    (*data) |= (0x3 << (aid * 2));
    (*data) &= (((src_port / 2) & 0x3) << (aid * 2));
    pg_write_one_pipe_reg(sid, dev_tgt.device_id, 1u << i, addr, *data);
  }
  return BF_SUCCESS;
}

static bf_status_t pipe_mgr_tof2_pktgen_reg_pfc(
    pipe_sess_hdl_t sid,
    bf_dev_target_t dev_tgt,
    int aid,
    struct bf_tof2_pktgen_pfc_trigger pfc_cfg) {
  LOG_TRACE(
      "PktGenAppCfg: dev %d pipe %d app %d shdl %d, pfc_hdr 0x%02x %02x %02x "
      "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x timer "
      "%s: %d, max_pfc_pkt_size %d x 8 bytes",
      dev_tgt.device_id,
      dev_tgt.dev_pipe_id,
      aid,
      sid,
      pfc_cfg.pfc_hdr[0],
      pfc_cfg.pfc_hdr[1],
      pfc_cfg.pfc_hdr[2],
      pfc_cfg.pfc_hdr[3],
      pfc_cfg.pfc_hdr[4],
      pfc_cfg.pfc_hdr[5],
      pfc_cfg.pfc_hdr[6],
      pfc_cfg.pfc_hdr[7],
      pfc_cfg.pfc_hdr[8],
      pfc_cfg.pfc_hdr[9],
      pfc_cfg.pfc_hdr[10],
      pfc_cfg.pfc_hdr[11],
      pfc_cfg.pfc_hdr[12],
      pfc_cfg.pfc_hdr[13],
      pfc_cfg.pfc_hdr[14],
      pfc_cfg.pfc_hdr[15],
      pfc_cfg.cfg_timer_en ? "true" : "flase",
      pfc_cfg.cfg_timer,
      pfc_cfg.pfc_max_msgs);

  int pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t addr, data;
  bf_status_t sts = BF_SUCCESS;
  uint32_t base_addr = offsetof(
      tof2_reg,
      pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.cfg_pfc_hdr.cfg_pfc_hdr_0_4);
  pipe_mgr_tof2_pg_dev_ctx *ctx = pipe_mgr_tof2_get_pgr_ctx(dev_tgt.device_id);
  if (ctx == NULL) {
    LOG_ERROR("%s:%d Unable to retrieve device context, dev %d",
              __func__,
              __LINE__,
              dev_tgt.device_id);
    return BF_INVALID_ARG;
  }

  for (int a = 0; a < BF_PIPE_COUNT; ++a) {
    if (!(pipe_mask & (1u << a))) continue;

    for (int i = 0, j = 3; i < 4; ++i, --j) {
      uint32_t pfc_hdr = pfc_cfg.pfc_hdr[4 * j];
      pfc_hdr = (pfc_hdr << 8) | pfc_cfg.pfc_hdr[4 * j + 1];
      pfc_hdr = (pfc_hdr << 8) | pfc_cfg.pfc_hdr[4 * j + 2];
      pfc_hdr = (pfc_hdr << 8) | pfc_cfg.pfc_hdr[4 * j + 3];
      addr = base_addr + i * 4;
      ctx->pfc[a].pfc_hdr[i] = pfc_hdr;
      sts |= pg_write_one_pipe_reg(
          sid, dev_tgt.device_id, pipe_mask, addr, pfc_hdr);
    }

    addr = offsetof(tof2_reg,
                    pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.cfg_pfc_timer);
    data = 0;
    if (pfc_cfg.cfg_timer_en) {
      data = pfc_cfg.cfg_timer | (1u << 16);
    }
    ctx->pfc[a].pfc_timer = data;
    sts |= pg_write_one_pipe_reg(sid, dev_tgt.device_id, pipe_mask, addr, data);

    addr = offsetof(
        tof2_reg,
        pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.cfg_pfc_max_pkt_size);
    ctx->pfc[a].pfc_max_pkt_size = pfc_cfg.pfc_max_msgs & 0x7ff;
    sts |= pg_write_one_pipe_reg(
        sid, dev_tgt.device_id, pipe_mask, addr, pfc_cfg.pfc_max_msgs & 0x7ff);
  }
  return sts;
}

/* Tofino2
 * configure application
 */
bf_status_t pipe_mgr_tof2_pktgen_cfg_app(bf_session_hdl_t shdl,
                                         bf_dev_target_t dev_tgt,
                                         int app_id,
                                         bf_pktgen_app_cfg_t *cfg) {
  /* Everything is validated.  Go ahead and update the app. */
  bf_status_t sts;
  bf_dev_id_t dev = dev_tgt.device_id;
  // ctrl
  sts = pipe_mgr_tof2_pktgen_reg_pgr_app_ctrl(shdl,
                                              dev_tgt,
                                              app_id,
                                              cfg->tof2.assigned_chnl_id,
                                              cfg->u.port_mask_sel_tof2,
                                              cfg->trigger_type);
  if (sts != BF_SUCCESS) return sts;
  // payload ctrl
  sts = pipe_mgr_tof2_pktgen_reg_pgr_app_payload_ctrl(
      shdl,
      dev_tgt,
      app_id,
      cfg->pkt_buffer_offset / 16,
      cfg->length,
      cfg->tof2.offset_len_from_recir_pkt);
  if (sts != BF_SUCCESS) return sts;
  // ing_port ctrl
  sts = pipe_mgr_tof2_pktgen_reg_pgr_app_ing_port_ctrl(
      shdl,
      dev_tgt,
      app_id,
      cfg->pipe_local_source_port,
      cfg->increment_source_port,
      cfg->tof2.source_port_wrap_max);
  if (sts != BF_SUCCESS) return sts;
  bool set = false;
  uint8_t chnl_id = 0;
  if ((BF_PKTGEN_TRIGGER_RECIRC_PATTERN == cfg->trigger_type) ||
      (BF_PKTGEN_TRIGGER_DPRSR == cfg->trigger_type)) {
    set = true;
    chnl_id = cfg->tof2.assigned_chnl_id;
  }
  // key and mask
  sts = pipe_mgr_tof2_pktgen_reg_pgr_app_recir_match_value(
      shdl, dev_tgt, app_id, cfg->u.pattern_tof2.value, set);
  if (sts != BF_SUCCESS) return sts;
  sts = pipe_mgr_tof2_pktgen_reg_pgr_app_recir_match_mask(
      shdl, dev_tgt, app_id, cfg->u.pattern_tof2.mask, set);
  if (sts != BF_SUCCESS) return sts;
  // recirc src en
  sts = pipe_mgr_tof2_pktgen_reg_pgr_app_recirc_src(
      shdl, dev_tgt, app_id, chnl_id);
  if (sts != BF_SUCCESS) return sts;
  // event number
  sts = pipe_mgr_tof2_pktgen_reg_pgr_app_event_number(shdl,
                                                      dev_tgt,
                                                      app_id,
                                                      cfg->trigger_type,
                                                      cfg->packets_per_batch,
                                                      cfg->batch_count);
  if (sts != BF_SUCCESS) return sts;
  // ibg
  sts = pipe_mgr_tof2_pktgen_reg_pgr_app_ibg_base(
      shdl, dev_tgt, app_id, pipe_mgr_nsec_to_clock(dev, cfg->ibg));
  if (sts != BF_SUCCESS) return sts;
  // ibg jitter
  sts = pipe_mgr_tof2_pktgen_reg_pgr_app_ibg_jitter(
      shdl, dev_tgt, app_id, pipe_mgr_nsec_to_clock(dev, cfg->ibg_jitter));
  if (sts != BF_SUCCESS) return sts;
  // ipg
  sts = pipe_mgr_tof2_pktgen_reg_pgr_app_ipg_base(
      shdl, dev_tgt, app_id, pipe_mgr_nsec_to_clock(dev, cfg->ipg));
  if (sts != BF_SUCCESS) return sts;
  // ipg jitter
  sts = pipe_mgr_tof2_pktgen_reg_pgr_app_ipg_jitter(
      shdl, dev_tgt, app_id, pipe_mgr_nsec_to_clock(dev, cfg->ipg_jitter));
  if (sts != BF_SUCCESS) return sts;
  // event timer
  if (BF_PKTGEN_TRIGGER_TIMER_ONE_SHOT == cfg->trigger_type ||
      BF_PKTGEN_TRIGGER_TIMER_PERIODIC == cfg->trigger_type) {
    sts = pipe_mgr_tof2_pktgen_reg_pgr_app_event_timer(
        shdl,
        dev_tgt,
        app_id,
        pipe_mgr_nsec_to_clock(dev, cfg->u.timer_nanosec));
    if (sts != BF_SUCCESS) return sts;
  } else {
    sts =
        pipe_mgr_tof2_pktgen_reg_pgr_app_event_timer(shdl, dev_tgt, app_id, 0);
    if (sts != BF_SUCCESS) return sts;
  }
  // pfc
  if (BF_PKTGEN_TRIGGER_PFC == cfg->trigger_type) {
    sts = pipe_mgr_tof2_pktgen_reg_pfc(shdl, dev_tgt, app_id, cfg->u.pfc_cfg);
    if (sts != BF_SUCCESS) return sts;
  }
  return BF_SUCCESS;
}

static inline uint32_t pipe_mgr_tof2_pktgen_jitter_recal(uint8_t scale,
                                                         uint8_t max) {
  return max << scale;
}

bf_status_t pipe_mgr_tof2_pktgen_cfg_app_get(bf_dev_target_t dev_tgt,
                                             int app_id,
                                             bf_pktgen_app_cfg_t *cfg) {
  bf_dev_id_t dev = dev_tgt.device_id;
  bf_dev_pipe_t pipe = dev_tgt.dev_pipe_id;
  int i = (pipe == BF_DEV_PIPE_ALL) ? 0 : pipe;
  uint32_t data, data_tmp;
  int j;
  pipe_mgr_tof2_pg_dev_ctx *ctx = pipe_mgr_tof2_get_pgr_ctx(dev_tgt.device_id);
  if (ctx == NULL) {
    LOG_ERROR("%s:%d Unable to retrieve device context, dev %d",
              __func__,
              __LINE__,
              dev_tgt.device_id);
    return BF_INVALID_ARG;
  }
  // trigger type
  data = ctx->app.a[i][app_id].app_ctrl;
  cfg->trigger_type = getp_tof2_pgr_app_ctrl_app_type(&data);
  cfg->tof2.assigned_chnl_id = getp_tof2_pgr_app_ctrl_app_chnl(&data);
  // key and mask
  switch (cfg->trigger_type) {
    case BF_PKTGEN_TRIGGER_PORT_DOWN:
      cfg->u.port_mask_sel_tof2 =
          getp_tof2_pgr_app_ctrl_app_port_down_mask_sel(&data);
      break;
    case BF_PKTGEN_TRIGGER_RECIRC_PATTERN:
    case BF_PKTGEN_TRIGGER_DPRSR:
      for (j = 0; j < 4; j++) {
        int tmp = 4 * j;
        data = ctx->app.a[i][app_id].recir_val[j];
        cfg->u.pattern_tof2.value[15 - tmp] = data & 0xff;
        cfg->u.pattern_tof2.value[14 - tmp] = (data >> 8) & 0xff;
        cfg->u.pattern_tof2.value[13 - tmp] = (data >> 16) & 0xff;
        cfg->u.pattern_tof2.value[12 - tmp] = (data >> 24) & 0xff;

        data = ctx->app.a[i][app_id].recir_msk[j];
        cfg->u.pattern_tof2.mask[15 - tmp] |= ~data & 0xff;
        cfg->u.pattern_tof2.mask[14 - tmp] |= (~data >> 8) & 0xff;
        cfg->u.pattern_tof2.mask[13 - tmp] |= (~data >> 16) & 0xff;
        cfg->u.pattern_tof2.mask[12 - tmp] |= (~data >> 24) & 0xff;
      }
      break;
    case BF_PKTGEN_TRIGGER_TIMER_ONE_SHOT:
    case BF_PKTGEN_TRIGGER_TIMER_PERIODIC:
      data = ctx->app.a[i][app_id].event_timer;
      data_tmp = getp_tof2_pgr_app_event_timer_timer_count(&data);
      cfg->u.timer_nanosec = pipe_mgr_clock_to_nsec(dev, data_tmp);
      break;
    case BF_PKTGEN_TRIGGER_PFC:
      for (j = 0; j < 4; j++) {
        int tmp = 4 * j;
        data = ctx->pfc[i].pfc_hdr[j];
        cfg->u.pfc_cfg.pfc_hdr[tmp + 3] = data & 0xff;
        cfg->u.pfc_cfg.pfc_hdr[tmp + 2] = (data >> 8) & 0xff;
        cfg->u.pfc_cfg.pfc_hdr[tmp + 1] = (data >> 16) & 0xff;
        cfg->u.pfc_cfg.pfc_hdr[tmp] = (data >> 24) & 0xff;
      }
      cfg->u.pfc_cfg.cfg_timer = ctx->pfc[i].pfc_timer & 0xffff;
      cfg->u.pfc_cfg.cfg_timer_en = ctx->pfc[i].pfc_timer & (1u << 16);
      cfg->u.pfc_cfg.pfc_max_msgs = ctx->pfc[i].pfc_max_pkt_size;
      break;
    default:
      PIPE_MGR_MEMSET(&cfg->u, 0, sizeof(cfg->u));
      break;
  }
  // payload ctrl
  data = ctx->app.a[i][app_id].payload_ctrl;
  cfg->pkt_buffer_offset = 0;
  cfg->length = 0;
  cfg->tof2.offset_len_from_recir_pkt =
      getp_tof2_pgr_app_payload_ctrl_app_recirc_extract(&data);
  if (!cfg->tof2.offset_len_from_recir_pkt) {
    cfg->pkt_buffer_offset =
        16 * (getp_tof2_pgr_app_payload_ctrl_app_payload_addr(&data));
    cfg->length = getp_tof2_pgr_app_payload_ctrl_app_payload_size(&data);
  }
  // ing_port ctrl
  data = ctx->app.a[i][app_id].ing_port;
  cfg->pipe_local_source_port =
      getp_tof2_pgr_app_ingr_port_ctrl_app_ingr_port(&data);
  cfg->increment_source_port =
      getp_tof2_pgr_app_ingr_port_ctrl_app_ingr_port_inc(&data);
  cfg->tof2.source_port_wrap_max =
      getp_tof2_pgr_app_ingr_port_ctrl_app_ingr_port_wrap(&data);
  // event number
  data = ctx->app.a[i][app_id].event_num;
  cfg->packets_per_batch = getp_tof2_pgr_app_event_number_packet_num(&data);
  cfg->batch_count = getp_tof2_pgr_app_event_number_batch_num(&data);
  // ibg
  data = ctx->app.a[i][app_id].ibg;
  data_tmp = getp_tof2_pgr_app_event_base_jitter_value_value(&data);
  cfg->ibg = pipe_mgr_clock_to_nsec(dev, data_tmp);
  // ibg jitter
  uint8_t max = ctx->app.a[i][app_id].ibg_jit_max;
  uint8_t scale = ctx->app.a[i][app_id].ibg_jit_scale;
  data_tmp = pipe_mgr_tof2_pktgen_jitter_recal(scale, max);
  cfg->ibg_jitter = pipe_mgr_clock_to_nsec(dev, data_tmp);
  // ipg
  data = ctx->app.a[i][app_id].ipg;
  data_tmp = getp_tof2_pgr_app_event_base_jitter_value_value(&data);
  cfg->ipg = pipe_mgr_clock_to_nsec(dev, data_tmp);
  // ipg jitter
  max = ctx->app.a[i][app_id].ipg_jit_max;
  scale = ctx->app.a[i][app_id].ipg_jit_scale;
  data_tmp = pipe_mgr_tof2_pktgen_jitter_recal(scale, max);
  cfg->ipg_jitter = pipe_mgr_clock_to_nsec(dev, data_tmp);
  return BF_SUCCESS;
}
/* Tofino2
 * configure port down mask
 */
static bf_status_t tof2_set_port_down_mask_hw(bf_session_hdl_t shdl,
                                              bf_dev_target_t dev_tgt,
                                              int mask_sel,
                                              uint8_t *mask) {
  bf_status_t sts = BF_SUCCESS;
  /* Grab a pointer to the port based mask. */
  struct pipe_mgr_tof2_pg_dev_ctx *ctx =
      pipe_mgr_tof2_get_pgr_ctx(dev_tgt.device_id);
  if (ctx == NULL) {
    LOG_ERROR("%s:%d Unable to retrieve device context, dev %d",
              __func__,
              __LINE__,
              dev_tgt.device_id);
    return BF_INVALID_ARG;
  }
  uint8_t *m_mask = ctx->port_down_mask[dev_tgt.dev_pipe_id][2].port_mask;

  /* Mask the user provided mask with the port based mask before programming .*/
  uint32_t d0 = mask[3] & m_mask[3];
  d0 = (d0 << 8) | (mask[2] & m_mask[2]);
  d0 = (d0 << 8) | (mask[1] & m_mask[1]);
  if (dev_tgt.dev_pipe_id != 0) {
    d0 = (d0 << 8) | (mask[0] & m_mask[0]);
  } else {
    /* Special case ports 2-5 (ethernet CPU) since it maps to bits 4-7 instead
     * of bits 2-5.  */
    d0 = (d0 << 8) | (((mask[0] & 1) | ((mask[0] & 0x3C) << 2)) & m_mask[0]);
  }

  uint32_t d1 = mask[7] & m_mask[7];
  d1 = (d1 << 8) | (mask[6] & m_mask[6]);
  d1 = (d1 << 8) | (mask[5] & m_mask[5]);
  d1 = (d1 << 8) | (mask[4] & m_mask[4]);
  uint32_t d2 = mask[8] & m_mask[8];
  uint32_t addr = offsetof(
      tof2_reg,
      pipes[0]
          .pardereg.pgstnreg.pgrreg.pgr_common.pgen_port_down_mask[mask_sel]
          .pgen_port_down_mask_0_3);
  ctx->port_down_mask[dev_tgt.dev_pipe_id][mask_sel].val_0 = d0;
  ctx->port_down_mask[dev_tgt.dev_pipe_id][mask_sel].val_1 = d1;
  ctx->port_down_mask[dev_tgt.dev_pipe_id][mask_sel].val_2 = d2;

  uint8_t pm = 1 << dev_tgt.dev_pipe_id;
  sts = pg_write_one_pipe_reg(shdl, dev_tgt.device_id, pm, addr, d0);
  if (BF_SUCCESS != sts) return sts;
  sts = pg_write_one_pipe_reg(shdl, dev_tgt.device_id, pm, addr + 4, d1);
  if (BF_SUCCESS != sts) return sts;
  sts = pg_write_one_pipe_reg(shdl, dev_tgt.device_id, pm, addr + 8, d2);
  return sts;
}
bf_status_t pipe_mgr_tof2_pktgen_cfg_port_down_mask(
    bf_session_hdl_t shdl,
    bf_dev_target_t dev_tgt,
    uint32_t port_mask_sel,
    struct bf_tof2_port_down_sel *msk) {
  if (!msk) return BF_INVALID_ARG;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_tgt.device_id);
  if (!dev_info) return BF_INVALID_ARG;
  pipe_mgr_tof2_pg_dev_ctx *ctx = pipe_mgr_tof2_get_pgr_ctx(dev_tgt.device_id);
  if (ctx == NULL) {
    LOG_ERROR("%s:%d Unable to retrieve device context, dev %d",
              __func__,
              __LINE__,
              dev_tgt.device_id);
    return BF_INVALID_ARG;
  }

  uint8_t *mask = msk->port_mask;

  int pipe_mask = pg_log_pipe_mask(dev_tgt);
  bf_status_t sts = BF_SUCCESS;
  for (bf_dev_pipe_t i = 0; BF_SUCCESS == sts && i < dev_info->num_active_pipes;
       ++i) {
    if (pipe_mask & (1u << i)) {
      bf_dev_target_t d_tgt = {dev_tgt.device_id, i};
      sts = tof2_set_port_down_mask_hw(shdl, d_tgt, port_mask_sel, mask);
    }
  }

  /* If the update was successful update our shadow. */
  if (BF_SUCCESS == sts) {
    if (dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL) {
      for (unsigned int i = 0; i < dev_info->num_active_pipes; ++i) {
        PIPE_MGR_MEMCPY(ctx->port_down_mask[i][port_mask_sel].port_mask,
                        mask,
                        sizeof ctx->port_down_mask[i][port_mask_sel].port_mask);
      }
    } else {
      ctx->port_down_mask[dev_tgt.dev_pipe_id][port_mask_sel] = *msk;
    }
  }

  return sts;
}

bf_status_t pipe_mgr_tof2_pktgen_cfg_port_down_mask_get(
    bf_dev_target_t dev_tgt,
    uint32_t port_mask_sel,
    struct bf_tof2_port_down_sel *msk) {
  if (!msk) return BF_INVALID_ARG;
  struct bf_tof2_port_down_sel *mask_tmp =
      &pipe_mgr_tof2_get_pgr_ctx(dev_tgt.device_id)
           ->port_down_mask[dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL
                                ? 0
                                : dev_tgt.dev_pipe_id][port_mask_sel];
  PIPE_MGR_MEMCPY(msk, mask_tmp, sizeof(struct bf_tof2_port_down_sel));
  return BF_SUCCESS;
}

bf_status_t pipe_mgr_tof2_pktgen_cfg_port_down_replay(
    bf_session_hdl_t shdl,
    bf_dev_target_t dev_tgt,
    bf_pktgen_port_down_mode_t mode) {
  int pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t addr = offsetof(
      tof2_reg,
      pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.pgen_retrigger_port_down);
  uint32_t data = 0;
  switch (mode) {
    case BF_PKTGEN_PORT_DOWN_REPLAY_NONE:
      break;
    case BF_PKTGEN_PORT_DOWN_REPLAY_ALL:
      setp_tof2_pgr_pgen_retrigger_port_down_en(&data, 1);
      setp_tof2_pgr_pgen_retrigger_port_down_all_down_port(&data, 1);
      break;
    case BF_PKTGEN_PORT_DOWN_REPLAY_MISSED:
      setp_tof2_pgr_pgen_retrigger_port_down_en(&data, 1);
      setp_tof2_pgr_pgen_retrigger_port_down_all_down_port(&data, 0);
      break;
    default:
      return BF_INVALID_ARG;
  }
  return pg_write_one_pipe_reg(shdl, dev_tgt.device_id, pipe_mask, addr, data);
}

bf_status_t pipe_mgr_tof2_pktgen_get_port_down_replay(
    rmt_dev_info_t *dev_info,
    bf_dev_target_t dev_tgt,
    bf_pktgen_port_down_mode_t *mode) {
  bf_dev_id_t dev_id = dev_info->dev_id;
  uint32_t addr = offsetof(
      tof2_reg,
      pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.pgen_retrigger_port_down);
  uint32_t data = 0;

  if (dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL) {
    uint32_t pipe_data;
    for (unsigned int log_pipe = 0; log_pipe < dev_info->num_active_pipes;
         ++log_pipe) {
      bf_dev_target_t d_tgt = {dev_tgt.device_id, log_pipe};
      pg_tof2_read_one_pipe_reg(d_tgt, addr, &pipe_data);
      if (!log_pipe) {
        data = pipe_data;
      } else if (pipe_data != data) {
        LOG_ERROR(
            "Dev %d: Pktgen port down replay mode not the same on all pipes, "
            "cannot query with PIPE-ALL",
            dev_id);
        return BF_INVALID_ARG;
      }
    }
  } else {
    pg_tof2_read_one_pipe_reg(dev_tgt, addr, &data);
  }

  if (getp_tof2_pgr_pgen_retrigger_port_down_en(&data) == 0) {
    *mode = BF_PKTGEN_PORT_DOWN_REPLAY_NONE;
  } else if (getp_tof2_pgr_pgen_retrigger_port_down_all_down_port(&data) != 0) {
    *mode = BF_PKTGEN_PORT_DOWN_REPLAY_ALL;
  } else {
    *mode = BF_PKTGEN_PORT_DOWN_REPLAY_MISSED;
  }
  return BF_SUCCESS;
}

bf_status_t pipe_mgr_tof2_pktgen_app_cfg_download(bf_session_hdl_t shdl,
                                                  bf_dev_id_t dev) {
  uint32_t i = 0, aid = 0;
  uint32_t addr = 0, val = 0;
  uint32_t pipe_cnt = pipe_mgr_get_num_active_pipes(dev);
  int pipe_mask = 0;
  bf_status_t sts = BF_SUCCESS;
  bf_dev_target_t dev_tgt = {dev, BF_INVALID_PIPE};
  pipe_mgr_tof2_pg_dev_ctx *ctx = pipe_mgr_tof2_get_pgr_ctx(dev_tgt.device_id);
  if (ctx == NULL) {
    LOG_ERROR("%s:%d Unable to retrieve device context, dev %d",
              __func__,
              __LINE__,
              dev_tgt.device_id);
    return BF_INVALID_ARG;
  }

  for (i = 0; i < pipe_cnt; ++i) {
    dev_tgt.dev_pipe_id = i;
    pipe_mask = pg_log_pipe_mask(dev_tgt);
    for (aid = 0; aid < PIPE_MGR_TOF2_PKTGEN_APP_CNT; aid++) {
      // payload ctrl
      addr =
          offsetof(tof2_reg,
                   pipes[0].pardereg.pgstnreg.pgrreg.pgr_app[aid].payload_ctrl);
      val = ctx->app.a[i][aid].payload_ctrl;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      // ing_port ctrl
      addr = offsetof(
          tof2_reg,
          pipes[0].pardereg.pgstnreg.pgrreg.pgr_app[aid].ingr_port_ctrl);
      val = ctx->app.a[i][aid].ing_port;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      // key
      addr = offsetof(tof2_reg,
                      pipes[0]
                          .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                          .recir_match_value.recir_match_value_0_4);
      val = ctx->app.a[i][aid].recir_val[0];
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;
      addr = offsetof(tof2_reg,
                      pipes[0]
                          .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                          .recir_match_value.recir_match_value_1_4);
      val = ctx->app.a[i][aid].recir_val[1];
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;
      addr = offsetof(tof2_reg,
                      pipes[0]
                          .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                          .recir_match_value.recir_match_value_2_4);
      val = ctx->app.a[i][aid].recir_val[2];
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;
      addr = offsetof(tof2_reg,
                      pipes[0]
                          .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                          .recir_match_value.recir_match_value_3_4);
      val = ctx->app.a[i][aid].recir_val[3];
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      // mask
      addr = offsetof(tof2_reg,
                      pipes[0]
                          .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                          .recir_match_mask.recir_match_mask_0_4);
      val = ctx->app.a[i][aid].recir_msk[0];
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;
      addr = offsetof(tof2_reg,
                      pipes[0]
                          .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                          .recir_match_mask.recir_match_mask_1_4);
      val = ctx->app.a[i][aid].recir_msk[1];
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;
      addr = offsetof(tof2_reg,
                      pipes[0]
                          .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                          .recir_match_mask.recir_match_mask_2_4);
      val = ctx->app.a[i][aid].recir_msk[2];
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;
      addr = offsetof(tof2_reg,
                      pipes[0]
                          .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                          .recir_match_mask.recir_match_mask_3_4);
      val = ctx->app.a[i][aid].recir_msk[3];
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      // recirc src en
      addr = offsetof(
          tof2_reg,
          pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.cfg_app_recirc_src);
      val = (ctx->app_recirc_src[i]);
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      // event number
      addr =
          offsetof(tof2_reg,
                   pipes[0].pardereg.pgstnreg.pgrreg.pgr_app[aid].event_number);
      val = ctx->app.a[i][aid].event_num;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      // ibg
      addr = offsetof(tof2_reg,
                      pipes[0]
                          .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                          .event_ibg_jitter_base_value);
      val = ctx->app.a[i][aid].ibg;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      // ibg jitter
      addr = offsetof(
          tof2_reg,
          pipes[0].pardereg.pgstnreg.pgrreg.pgr_app[aid].event_max_ibg_jitter);
      val = ctx->app.a[i][aid].ibg_jit_max;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      addr = offsetof(tof2_reg,
                      pipes[0]
                          .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                          .event_ibg_jitter_scale);
      val = ctx->app.a[i][aid].ibg_jit_scale;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      // ipg
      addr = offsetof(tof2_reg,
                      pipes[0]
                          .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                          .event_ipg_jitter_base_value);
      val = ctx->app.a[i][aid].ipg;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      // ipg jitter
      addr = offsetof(
          tof2_reg,
          pipes[0].pardereg.pgstnreg.pgrreg.pgr_app[aid].event_max_ipg_jitter);
      val = ctx->app.a[i][aid].ipg_jit_max;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;
      addr = offsetof(tof2_reg,
                      pipes[0]
                          .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                          .event_ipg_jitter_scale);
      val = ctx->app.a[i][aid].ipg_jit_scale;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      // event timer
      addr = offsetof(
          tof2_reg, pipes[0].pardereg.pgstnreg.pgrreg.pgr_app[aid].event_timer);
      val = ctx->app.a[i][aid].event_timer;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      // pfc
      uint32_t base_addr = offsetof(
          tof2_reg,
          pipes[0]
              .pardereg.pgstnreg.pgrreg.pgr_common.cfg_pfc_hdr.cfg_pfc_hdr_0_4);
      for (int k = 0; k < 4; ++k) {
        addr = base_addr + k * 4;
        val = ctx->pfc[i].pfc_hdr[k];
        sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
        if (sts != BF_SUCCESS) return sts;
      }

      addr = offsetof(
          tof2_reg, pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.cfg_pfc_timer);
      val = ctx->pfc[i].pfc_timer;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      addr = offsetof(
          tof2_reg,
          pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.cfg_pfc_max_pkt_size);
      val = ctx->pfc[i].pfc_max_pkt_size;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      // ctrl
      addr = offsetof(tof2_reg,
                      pipes[0].pardereg.pgstnreg.pgrreg.pgr_app[aid].ctrl);
      val = ctx->app.a[i][aid].app_ctrl;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;
    }
  }
  return BF_SUCCESS;
}

/* Warm init quick */
bf_status_t pipe_mgr_tof2_pktgen_warm_init_quick(bf_session_hdl_t shdl,
                                                 bf_dev_id_t dev) {
  bf_status_t sts = BF_SUCCESS;
  uint32_t i = 0, pipe_cnt = pipe_mgr_get_num_active_pipes(dev);
  int pipe_mask = 0;
  uint32_t addr = 0, val = 0;
  bf_dev_target_t dev_tgt;

  dev_tgt.device_id = dev;

  /* Clear state in our SW shadow related to ports.  This will be rebuild when
   * we internally replay all pipe_mgr ports. */
  pipe_mgr_tof2_pg_dev_ctx *ctx = pipe_mgr_tof2_get_pgr_ctx(dev_tgt.device_id);
  if (ctx == NULL) {
    LOG_ERROR("%s:%d Unable to retrieve device context, dev %d",
              __func__,
              __LINE__,
              dev_tgt.device_id);
    return BF_INVALID_ARG;
  }
  ctx->port_ctrl = 0;
  for (i = 0; i < pipe_cnt; ++i) {
    PIPE_MGR_MEMSET(ctx->ipb_chnl_sp[i], 0, 8);
    ctx->ipb_ctrl[i] = 0;
    ctx->ebuf_chnl_en[i] = 0;
    PIPE_MGR_MEMSET(ctx->ebuf_port_ctrl[i], 0, 4);
  }

  sts = pipe_mgr_tof2_pktgen_dev_init(shdl, dev, true);
  if (sts != BF_SUCCESS) return sts;

  for (i = 0; i < pipe_cnt; ++i) {
    dev_tgt.dev_pipe_id = i;
    pipe_mask = pg_log_pipe_mask(dev_tgt);

    // port_down_mode
    sts = pipe_mgr_tof2_pktgen_cfg_port_down_replay(
        shdl, dev_tgt, ctx->port_down_mode[i]);
    if (sts != BF_SUCCESS) return sts;
    // port_down_mask
    for (uint32_t mask_sel = 0; mask_sel < 2; mask_sel++) {
      addr = offsetof(
          tof2_reg,
          pipes[0]
              .pardereg.pgstnreg.pgrreg.pgr_common.pgen_port_down_mask[mask_sel]
              .pgen_port_down_mask_0_3);
      val = ctx->port_down_mask[i][mask_sel].val_0;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (BF_SUCCESS != sts) return sts;

      addr = offsetof(
          tof2_reg,
          pipes[0]
              .pardereg.pgstnreg.pgrreg.pgr_common.pgen_port_down_mask[mask_sel]
              .pgen_port_down_mask_1_3);
      val = ctx->port_down_mask[i][mask_sel].val_1;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (BF_SUCCESS != sts) return sts;

      addr = offsetof(
          tof2_reg,
          pipes[0]
              .pardereg.pgstnreg.pgrreg.pgr_common.pgen_port_down_mask[mask_sel]
              .pgen_port_down_mask_2_3);
      val = ctx->port_down_mask[i][mask_sel].val_2;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (BF_SUCCESS != sts) return sts;
    }
  }

  sts = pipe_mgr_tof2_pktgen_app_cfg_download(shdl, dev);
  if (sts != BF_SUCCESS) return sts;

  return sts;
}
