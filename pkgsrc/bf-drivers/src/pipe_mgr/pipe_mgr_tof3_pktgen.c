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
#include "pipe_mgr_tof3_pktgen.h"
#include <tof3_regs/tof3_mem_drv.h>
#include <tof3_regs/tof3_reg_drv.h>
#include <port_mgr/bf_port_if.h>

/* Context-independent recirc state */
uint8_t tof3_tbc_eth_en[PIPE_MGR_NUM_DEVICES][PIPE_MGR_NUM_SUBDEVICES] = {0};

/* Pointer to global pipe_mgr context */
extern pipe_mgr_ctx_t *pipe_mgr_ctx;

static bf_status_t pg_tof3_read_one_pipe_reg(int dev,
                                             int logical_pipe,
                                             uint32_t addr,
                                             uint32_t *data) {
  *data = 0;
  bf_dev_pipe_t physical_pipe = logical_pipe;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return BF_INVALID_ARG;
  }
  if (logical_pipe == BF_DEV_PIPE_ALL) {
    PIPE_MGR_DBGCHK(logical_pipe != BF_DEV_PIPE_ALL);
    return BF_INVALID_ARG;
  }
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, logical_pipe, &physical_pipe);
  bf_subdev_id_t subdev = physical_pipe / BF_SUBDEV_PIPE_COUNT;
  physical_pipe = physical_pipe % BF_SUBDEV_PIPE_COUNT;
  uint32_t base =
      offsetof(tof3_reg, pipes[physical_pipe]) - offsetof(tof3_reg, pipes[0]);

  int x = lld_subdev_read_register(dev, subdev, base + addr, data);
  if (x) return BF_HW_COMM_FAIL;
  return BF_SUCCESS;
}

/* Write pkt buffer shadow memory to asic */
static bf_status_t pipe_mgr_pkt_buffer_tof3_addr_get(uint64_t *addr,
                                                     uint32_t *elem_size,
                                                     uint32_t *count) {
  *addr = tof3_mem_pipes_parde_pgr_mem_rspec_buffer_mem_word_address / 16;
  /* Write the entire 16K pkt buffer */
  *elem_size =
      tof3_mem_pipes_parde_pgr_mem_rspec_buffer_mem_word_array_element_size;
  *count = tof3_mem_pipes_parde_pgr_mem_rspec_buffer_mem_word_array_count;
  return BF_SUCCESS;
}

static struct pipe_mgr_tof3_pg_dev_ctx *pipe_mgr_tof3_get_pgr_ctx(
    bf_dev_id_t dev) {
  return pipe_mgr_pktgen_ctx(dev)->u.tof3_ctx;
}

static void pipe_mgr_tof3_pgr_tbc_port_recir_en(bf_session_hdl_t shdl,
                                                bf_dev_id_t dev,
                                                int pipe,
                                                bool recir_enable,
                                                bool use_shadow) {
  uint32_t data, addr;
  addr = offsetof(tof3_reg,
                  pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.tbc_port_ctrl);
  data = 0;
  if (use_shadow) {
    int die_id = pipe / BF_SUBDEV_PIPE_COUNT;
    recir_enable = (tof3_tbc_eth_en[dev][die_id] & 1) == 0;
  }
  setp_tof3_pgr_tbc_port_ctrl_port_en(&data, recir_enable ? 0 : 1);
  pg_write_one_pipe_reg(shdl, dev, 1u << pipe, addr, data);
}
static void pipe_mgr_tof3_pgr_init_eth_port_ctrl(bf_session_hdl_t shdl,
                                                 bf_dev_id_t dev,
                                                 uint8_t port_en,
                                                 uint8_t chnl_en,
                                                 uint8_t chnl_mode,
                                                 uint8_t chnl_seq,
                                                 bool use_shadow) {
  uint32_t addr, data, addr_scratch;
  uint32_t num_subdev = pipe_mgr_get_num_active_subdevices(dev);
  uint8_t pm = 0;
  int i = 0;
  addr = offsetof(
      tof3_reg, pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.eth_cpu_port_ctrl);
  addr_scratch =
      offsetof(tof3_reg, pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.scratch);
  if (use_shadow) {
    data = pipe_mgr_tof3_get_pgr_ctx(dev)->port_ctrl;
  } else {
    data = 0;
    setp_tof3_pgr_eth_cpu_ctrl_port_en(&data, port_en);
    setp_tof3_pgr_eth_cpu_ctrl_channel_en(&data, chnl_en);
    setp_tof3_pgr_eth_cpu_ctrl_channel_mode(&data, chnl_mode);
    setp_tof3_pgr_eth_cpu_ctrl_channel_seq(&data, chnl_seq);
    pipe_mgr_tof3_get_pgr_ctx(dev)->port_ctrl = data;
  }

  for (i = 0; i < (int)num_subdev; i++) {
    pm |= 1u << (i * BF_SUBDEV_PIPE_COUNT);
  }

  /* The port_en bit can only be set after the mode is set.  First write the
   * value without the port_en bit, write a scratch register, then write the
   * desired value. */
  if (port_en) {
    uint32_t t = data;
    setp_tof3_pgr_eth_cpu_ctrl_port_en(&t, 0);
    pg_write_one_pipe_reg(shdl, dev, pm, addr, t);
    pg_write_one_pipe_reg(shdl, dev, pm, addr_scratch, 0);
  }
  /* The channel mode must always be four; TF2LAB-41. */
  uint32_t wr_data = data;
  setp_tof3_pgr_eth_cpu_ctrl_channel_mode(&wr_data, 4);
  pg_write_one_pipe_reg(shdl, dev, pm, addr, wr_data);
}

static void pipe_mgr_tof3_free_dev_ctx(struct pipe_mgr_pg_dev_ctx *c,
                                       bf_dev_id_t dev) {
  uint32_t dev_pipes = pipe_mgr_get_num_active_pipes(dev);
  uint32_t i;
  if (c->u.tof3_ctx) {
    if (c->u.tof3_ctx->app.a) {
      for (i = 0; i < dev_pipes; i++) {
        if (c->u.tof3_ctx->app.a[i]) {
          PIPE_MGR_FREE(c->u.tof3_ctx->app.a[i]);
        }
      }
      PIPE_MGR_FREE(c->u.tof3_ctx->app.a);
    }
    if (c->u.tof3_ctx->app.b) {
      for (i = 0; i < dev_pipes; i++) {
        if (c->u.tof3_ctx->app.b[i]) {
          PIPE_MGR_FREE(c->u.tof3_ctx->app.b[i]);
        }
      }
      PIPE_MGR_FREE(c->u.tof3_ctx->app.b);
    }
    if (c->u.tof3_ctx->ipb_chnl_sp) {
      for (i = 0; i < dev_pipes; i++) {
        if (c->u.tof3_ctx->ipb_chnl_sp[i])
          PIPE_MGR_FREE(c->u.tof3_ctx->ipb_chnl_sp[i]);
      }
      PIPE_MGR_FREE(c->u.tof3_ctx->ipb_chnl_sp);
    }
    if (c->u.tof3_ctx->ebuf_port_ctrl) {
      for (i = 0; i < dev_pipes; i++) {
        if (c->u.tof3_ctx->ebuf_port_ctrl[i])
          PIPE_MGR_FREE(c->u.tof3_ctx->ebuf_port_ctrl[i]);
      }
      PIPE_MGR_FREE(c->u.tof3_ctx->ebuf_port_ctrl);
    }
    if (c->u.tof3_ctx->pkt_buffer_shadow)
      PIPE_MGR_FREE(c->u.tof3_ctx->pkt_buffer_shadow);
    if (c->u.tof3_ctx->ipb_ctrl) PIPE_MGR_FREE(c->u.tof3_ctx->ipb_ctrl);
    if (c->u.tof3_ctx->pfc) PIPE_MGR_FREE(c->u.tof3_ctx->pfc);
    if (c->u.tof3_ctx->ebuf_chnl_en) PIPE_MGR_FREE(c->u.tof3_ctx->ebuf_chnl_en);
    if (c->u.tof3_ctx->app_recirc_src)
      PIPE_MGR_FREE(c->u.tof3_ctx->app_recirc_src);
    if (c->u.tof3_ctx->port_down_mode)
      PIPE_MGR_FREE(c->u.tof3_ctx->port_down_mode);
    if (c->u.tof3_ctx->port_down_mask) {
      for (i = 0; i < dev_pipes; i++)
        PIPE_MGR_FREE(c->u.tof3_ctx->port_down_mask[i]);
      PIPE_MGR_FREE(c->u.tof3_ctx->port_down_mask);
    }
    PIPE_MGR_FREE(c->u.tof3_ctx);
  }
  PIPE_MGR_FREE(c);
}

bf_status_t pipe_mgr_tof3_pktgen_dev_init(bf_session_hdl_t shdl,
                                          bf_dev_id_t dev,
                                          bool use_shadow) {
  bf_status_t sts = BF_SUCCESS;
  uint32_t dev_pipes = pipe_mgr_get_num_active_pipes(dev);
  uint32_t num_subdev = pipe_mgr_get_num_active_subdevices(dev);
  uint32_t i, j;
  uint32_t data, addr;

  bf_dev_port_t eth_cpu_dev_port = bf_eth_cpu_port_get(dev);
  bf_dev_pipe_t cpu_pipe = DEV_PORT_TO_PIPE(eth_cpu_dev_port);
  // set all ts
  // recirc_ts
  for (i = 0; i < 4; i++) {
    addr = offsetof(tof3_reg,
                    pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.recirc_ts[i]);
    data = 65;
    for (j = 0; j < dev_pipes; j++) {
      pg_write_one_pipe_reg(shdl, dev, 1u << j, addr, data);
    }
  }
  // set csr_ts_offset
  addr = offsetof(tof3_reg,
                  pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.csr_ts_offset);
  data = 80 << 12;
  for (j = 0; j < dev_pipes; j++) {
    pg_write_one_pipe_reg(shdl, dev, 1u << j, addr, data);
  }
  // cfg_pgen_chnl_ts
  addr = offsetof(
      tof3_reg, pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.cfg_pgen_chnl_ts);
  data = 65;
  for (j = 0; j < dev_pipes; j++) {
    pg_write_one_pipe_reg(shdl, dev, 1u << j, addr, data);
  }

  // cfg_pgen_tdm_ts
  addr = offsetof(tof3_reg,
                  pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.cfg_pgen_tdm_ts);
  data = 65;
  for (j = 0; j < dev_pipes; j++) {
    pg_write_one_pipe_reg(shdl, dev, 1u << j, addr, data);
  }

  // set pgn_ctrl-pipe_id,swap_en
  for (i = 0; i < dev_pipes; i++) {
    data = 0;
    addr = offsetof(
        tof3_reg,
        pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.pgen_ctrl.pgen_ctrl_0_2);
    setp_tof3_pgr_pgen_ctrl_pgen_ctrl_0_2_pipe_id(&data, i);
    setp_tof3_pgr_pgen_ctrl_pgen_ctrl_0_2_swap_en(&data, 1);
    pg_write_one_pipe_reg(shdl, dev, 1u << i, addr, data);
    addr = offsetof(
        tof3_reg,
        pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.pgen_ctrl.pgen_ctrl_1_2);
    pg_write_one_pipe_reg(shdl, dev, 1u << i, addr, 0);

    /* For the pipe with the Ethernet CPU port enable pgen port-down detection,
     * otherwise disable it for the non-MAC ports. */
    data = (i == cpu_pipe) ? 0xF0 : 0;
    addr = offsetof(
        tof3_reg,
        pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.pgen_port_down_ctrl);
    sts = pg_write_one_pipe_reg(shdl, dev, 1u << i, addr, data);
    if (sts != BF_SUCCESS) return sts;
  }

  if (!use_shadow) {
    // set tbc port ctrl default to tbc
    for (i = 0; i < num_subdev; i++) {
      tof3_tbc_eth_en[dev][i] |= 1;
    }
  }
  for (i = 0; i < num_subdev; i++) {
    pipe_mgr_tof3_pgr_tbc_port_recir_en(
        shdl, dev, i * BF_SUBDEV_PIPE_COUNT, false, use_shadow);
  }

  if (!use_shadow) {
    // set default to eth cpu on port 2-5
    for (i = 0; i < num_subdev; i++) {
      tof3_tbc_eth_en[dev][i] |= (0xf << 2);
    }
  }
  pipe_mgr_tof3_pgr_init_eth_port_ctrl(shdl, dev, 1, 0xf, 4, 0xd8, use_shadow);

  /* Initialize packet buffer memory. */
  for (i = 0; i < dev_pipes; i++) {
    sts = pipe_mgr_tof3_pkt_buffer_write_from_shadow(shdl, dev, i);  // FIXME
    if (BF_SUCCESS != sts) {
      LOG_ERROR("Failed to init pkt-gen payload buffer. dev %d pipe %d sts %s",
                dev,
                i,
                bf_err_str(sts));
      return sts;
    }
  }

  return BF_SUCCESS;
}

/* Tofino3
 * add pgr dev
 */
bf_status_t pipe_mgr_tof3_pktgen_add_dev(bf_session_hdl_t shdl,
                                         bf_dev_id_t dev) {
  bf_status_t sts = BF_SUCCESS;

  /* Initialize context for this device. */
  struct pipe_mgr_pg_dev_ctx *c = PIPE_MGR_CALLOC(1, sizeof *c);
  if (!c) return BF_NO_SYS_RESOURCES;
  c->u.tof3_ctx = PIPE_MGR_CALLOC(1, sizeof(*c->u.tof3_ctx));
  if (!c->u.tof3_ctx) goto cleanup;
  uint32_t dev_pipes = pipe_mgr_get_num_active_pipes(dev);
  uint32_t i, j;

  c->u.tof3_ctx->ipb_chnl_sp =
      PIPE_MGR_CALLOC(dev_pipes, sizeof(*c->u.tof3_ctx->ipb_chnl_sp));
  c->u.tof3_ctx->ipb_ctrl =
      PIPE_MGR_CALLOC(dev_pipes, sizeof(*c->u.tof3_ctx->ipb_ctrl));
  c->u.tof3_ctx->ebuf_chnl_en =
      PIPE_MGR_CALLOC(dev_pipes, sizeof(*c->u.tof3_ctx->ebuf_chnl_en));
  c->u.tof3_ctx->ebuf_port_ctrl =
      PIPE_MGR_CALLOC(dev_pipes, sizeof(*c->u.tof3_ctx->ebuf_port_ctrl));
  c->u.tof3_ctx->app_recirc_src =
      PIPE_MGR_CALLOC(dev_pipes, sizeof(*c->u.tof3_ctx->app_recirc_src));
  c->u.tof3_ctx->pkt_buffer_shadow =
      PIPE_MGR_CALLOC(dev_pipes, sizeof(*c->u.tof3_ctx->pkt_buffer_shadow));
  c->u.tof3_ctx->pfc = PIPE_MGR_CALLOC(dev_pipes, sizeof(*c->u.tof3_ctx->pfc));
  if (!c->u.tof3_ctx->ipb_chnl_sp || !c->u.tof3_ctx->ipb_ctrl ||
      !c->u.tof3_ctx->ebuf_chnl_en || !c->u.tof3_ctx->ebuf_port_ctrl ||
      !c->u.tof3_ctx->pkt_buffer_shadow || !c->u.tof3_ctx->pfc) {
    goto cleanup;
  }
  c->u.tof3_ctx->app.a =
      PIPE_MGR_CALLOC(dev_pipes, sizeof(*c->u.tof3_ctx->app.a));
  c->u.tof3_ctx->app.b =
      PIPE_MGR_CALLOC(dev_pipes, sizeof(*c->u.tof3_ctx->app.b));
  if (!c->u.tof3_ctx->app.a || !c->u.tof3_ctx->app.b) goto cleanup;
  for (i = 0; i < dev_pipes; i++) {
    c->u.tof3_ctx->app.a[i] = PIPE_MGR_CALLOC(PIPE_MGR_TOF3_PKTGEN_APP_CNT,
                                              sizeof(*c->u.tof3_ctx->app.a[i]));
    c->u.tof3_ctx->app.b[i] = PIPE_MGR_CALLOC(PIPE_MGR_TOF3_PKTGEN_APP_CNT,
                                              sizeof(*c->u.tof3_ctx->app.b[i]));
    c->u.tof3_ctx->ipb_chnl_sp[i] =
        PIPE_MGR_CALLOC(8, sizeof(*c->u.tof3_ctx->ipb_chnl_sp[i]));
    c->u.tof3_ctx->ebuf_port_ctrl[i] =
        PIPE_MGR_CALLOC(4, sizeof(*c->u.tof3_ctx->ebuf_port_ctrl[i]));
    if (!c->u.tof3_ctx->app.a[i] || !c->u.tof3_ctx->app.b[i] ||
        !c->u.tof3_ctx->ipb_chnl_sp[i])
      goto cleanup;
  }
  c->u.tof3_ctx->port_down_mode =
      PIPE_MGR_CALLOC(dev_pipes, sizeof *c->u.tof3_ctx->port_down_mode);
  if (!c->u.tof3_ctx->port_down_mode) goto cleanup;
  for (i = 0; i < dev_pipes; ++i)
    c->u.tof3_ctx->port_down_mode[i] = BF_PKTGEN_PORT_DOWN_REPLAY_NONE;

  c->u.tof3_ctx->port_down_mask =
      PIPE_MGR_CALLOC(dev_pipes, sizeof *c->u.tof3_ctx->port_down_mask);
  if (!c->u.tof3_ctx->port_down_mask) goto cleanup;
  for (i = 0; i < dev_pipes; ++i) {
    c->u.tof3_ctx->port_down_mask[i] =
        PIPE_MGR_CALLOC(3, sizeof *c->u.tof3_ctx->port_down_mask[i]);
    if (!c->u.tof3_ctx->port_down_mask[i]) goto cleanup;
    for (size_t s = 0; s < sizeof c->u.tof3_ctx->port_down_mask[0][0].port_mask;
         ++s) {
      c->u.tof3_ctx->port_down_mask[i][0].port_mask[s] = 0xFF;
      c->u.tof3_ctx->port_down_mask[i][1].port_mask[s] = 0xFF;
      /* Index 2 is initialized to zero from the calloc. */
    }
  }
  if (!c->u.tof3_ctx->port_down_mask) goto cleanup;

  pipe_mgr_pktgen_ctx_set(dev, c);
  // initial values
  c->u.tof3_ctx->port_ctrl = 0;
  for (i = 0; i < dev_pipes; i++) {
    for (j = 0; j < 8; j++) {
      c->u.tof3_ctx->ipb_chnl_sp[i][j] = 0;
    }
    c->u.tof3_ctx->ipb_ctrl[i] = 0;
    c->u.tof3_ctx->app_recirc_src[i] = 0;
    c->u.tof3_ctx->ebuf_chnl_en[i] = 0;
    for (j = 0; j < 4; j++) {
      c->u.tof3_ctx->ebuf_port_ctrl[i][j] = 0;
    }
  }
  sts = pipe_mgr_tof3_pktgen_dev_init(shdl, dev, false);
  if (BF_SUCCESS != sts) return sts;

  return BF_SUCCESS;

cleanup:
  pipe_mgr_tof3_free_dev_ctx(c, dev);
  return BF_NO_SYS_RESOURCES;
}
/* Tofino3
 * remove pgr dev
 */
bf_status_t pipe_mgr_tof3_pktgen_rmv_dev(bf_dev_id_t dev) {
  struct pipe_mgr_pg_dev_ctx *c = pipe_mgr_pktgen_ctx(dev);
  pipe_mgr_pktgen_ctx_set(dev, NULL);
  pipe_mgr_tof3_free_dev_ctx(c, dev);
  return BF_SUCCESS;
}

static bool pipe_mgr_tof3_ipb_conflict_check(int dev, int pipe, int port) {
  /* true: pass conflict check*/
  uint32_t *ipb_ch_sp = pipe_mgr_tof3_get_pgr_ctx(dev)->ipb_chnl_sp[pipe];
  if (ipb_ch_sp[port] == 1) return true;
  if (ipb_ch_sp[port] != 0) return true;
  // if == 0, find the former non-zero
  for (int i = (port - 1); i >= 0; i--) {
    if (ipb_ch_sp[i] == 0) continue;
    if (ipb_ch_sp[i] > (uint32_t)(port - i))
      return false;
    else
      break;
  }
  return true;
}

static uint8_t pipe_mgr_tof3_get_chnls(bf_port_speeds_t speed, bool is_eth) {
  // Arbiter's left side channels number
  switch (speed) {
    case BF_SPEED_1G:
    case BF_SPEED_10G:
    case BF_SPEED_25G:
      return 1;
    case BF_SPEED_40G:
      return 4;
    case BF_SPEED_200G:
    case BF_SPEED_400G:
      return 0;
    case BF_SPEED_50G:
      if (is_eth)
        return 2;
      else
        return 1;
    case BF_SPEED_100G:
      if (is_eth)
        return 4;
      else
        return 2;
    default:
      return 0;
  }
}

static uint32_t pipe_mgr_tof3_get_speed(bf_port_speeds_t speed) {
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
    case BF_SPEED_50G:
      return 50;
    case BF_SPEED_100G:
      return 100;
    default:
      return 0;
  }
}

static bf_status_t pipe_mgr_tof3_get_ipb_seq(int dev,
                                             int pipe,
                                             uint32_t *seq_return) {
  uint32_t seq = 0;
  uint32_t *ipb_ch_sp = pipe_mgr_tof3_get_pgr_ctx(dev)->ipb_chnl_sp[pipe];
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
static bf_status_t pipe_mgr_tof3_pgr_ipb_ctrl_set(pipe_sess_hdl_t shdl,
                                                  int dev,
                                                  int pipe,
                                                  int port,
                                                  int port_end,
                                                  bool enable,
                                                  bf_port_speeds_t speed) {
  uint32_t *data, addr, data1, seq = 0;
  struct pipe_mgr_tof3_pg_dev_ctx *ctx = pipe_mgr_tof3_get_pgr_ctx(dev);
  bf_status_t sts;
  addr = offsetof(
      tof3_reg,
      pipes[0]
          .pardereg.pgstnreg.pgrreg.pgr_common.ipb_port_ctrl.ipb_port_ctrl_0_2);
  data = &(ctx->ipb_ctrl[pipe]);
  // chnl en
  uint32_t chnl_en =
      getp_tof3_pgr_ipb_port_ctrl_ipb_port_ctrl_0_2_recir_channel_en(data);
  for (int i = port; i <= port_end; i++) {
    if (enable) {
      chnl_en |= (1u << i);
    } else {
      chnl_en &= ~(1u << i);
    }
    ctx->ipb_chnl_sp[pipe][i] = 0;
  }
  if (enable) {
    uint32_t speed_numb = pipe_mgr_tof3_get_speed(speed);
    ctx->ipb_chnl_sp[pipe][port] =
        ((speed_numb / 50 < 1) ? 1 : (speed_numb / 50));
  } else {
    (void)speed;
  }
  setp_tof3_pgr_ipb_port_ctrl_ipb_port_ctrl_0_2_recir_channel_en(data, chnl_en);
  // seq
  sts = pipe_mgr_tof3_get_ipb_seq(dev, pipe, &seq);
  if (sts != BF_SUCCESS) {
    LOG_ERROR("%s:%d Get ipb sequence error, dev %d, pipe %d",
              __func__,
              __LINE__,
              dev,
              pipe);
    return sts;
  }
  setp_tof3_pgr_ipb_port_ctrl_ipb_port_ctrl_0_2_channel_seq_7_0(data,
                                                                seq & 0xff);
  pg_write_one_pipe_reg(shdl, dev, 1 << pipe, addr, *data);
  addr = offsetof(
      tof3_reg,
      pipes[0]
          .pardereg.pgstnreg.pgrreg.pgr_common.ipb_port_ctrl.ipb_port_ctrl_1_2);
  data1 = 0;
  setp_tof3_pgr_ipb_port_ctrl_ipb_port_ctrl_1_2_channel_seq_23_8(&data1,
                                                                 seq >> 8);
  pg_write_one_pipe_reg(shdl, dev, 1 << pipe, addr, data1);
  return BF_SUCCESS;
}

static bf_status_t pipe_mgr_tof3_pgr_ebuf_port_ctrl_set(pipe_sess_hdl_t shdl,
                                                        int dev,
                                                        int pipe,
                                                        int port,
                                                        uint8_t chnl_numb,
                                                        bool add) {
  uint32_t addr, data1;
  int pair_port;
  bool two_steps_op = false;
  struct pipe_mgr_tof3_pg_dev_ctx *ctx = pipe_mgr_tof3_get_pgr_ctx(dev);
  uint32_t chnl_en = (ctx->ebuf_chnl_en[pipe]);
  // ebuf_port_ctrl: chnl pipe
  addr = offsetof(
      tof3_reg,
      pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.ebuf_port_ctrl[port / 2]);

  switch (chnl_numb) {
    case 2:
      // 1channel mode
      if ((port != 0) && (port != 2) && (port != 4) && (port != 6)) {
        LOG_ERROR("%s:%d Invalid port, dev %d, port 0x%x, channel number %d",
                  __func__,
                  __LINE__,
                  dev,
                  port,
                  chnl_numb);
        return BF_INVALID_ARG;
      }
      if (add && (((chnl_en & (1 << port)) != 0) ||
                  ((chnl_en & (1 << (port + 1))) != 0))) {
        LOG_ERROR("%s:%d Invalid port, dev %d, port 0x%x, channel number %d",
                  __func__,
                  __LINE__,
                  dev,
                  port,
                  chnl_numb);
        return BF_INVALID_ARG;
      }
      if (!add && (((chnl_en & (1 << port)) == 0) ||
                   ((chnl_en & (1 << (port + 1))) == 0))) {
        LOG_ERROR("%s:%d Invalid port, dev %d, port 0x%x, channel number %d",
                  __func__,
                  __LINE__,
                  dev,
                  port,
                  chnl_numb);
        return BF_INVALID_ARG;
      }
      data1 = 0;
      setp_tof3_pgr_ebuf_port_ctrl_port_en(&data1, (add ? 1 : 0));
      setp_tof3_pgr_ebuf_port_ctrl_channel_en(&data1, (add ? 0x3 : 0));
      setp_tof3_pgr_ebuf_port_ctrl_channel_mode(&data1, 0);
      ctx->ebuf_port_ctrl[pipe][port / 2] = data1;
      pg_write_one_pipe_reg(shdl, dev, 1 << pipe, addr, data1);
      if (add) {
        ctx->ebuf_chnl_en[pipe] |= (1u << port);
        ctx->ebuf_chnl_en[pipe] |= (1u << (port + 1));
      } else {
        ctx->ebuf_chnl_en[pipe] &= ~(1u << port);
        ctx->ebuf_chnl_en[pipe] &= ~(1u << (port + 1));
      }
      break;
    case 1:
      // 2channel mode
      pair_port = (port % 2 == 0) ? (port + 1) : (port - 1);
      data1 = 0;
      // check whether pair_port is enabled or not
      if (chnl_en & (1 << pair_port)) {
        setp_tof3_pgr_ebuf_port_ctrl_port_en(&data1, 1);
        setp_tof3_pgr_ebuf_port_ctrl_channel_mode(&data1, 1);
        // enabled
        if (add) {
          setp_tof3_pgr_ebuf_port_ctrl_channel_en(&data1, 0x3);
        } else {
          setp_tof3_pgr_ebuf_port_ctrl_channel_en(
              &data1, (pair_port > port) ? 0x2 : 0x1);
        }
      } else {
        if (add) {
          setp_tof3_pgr_ebuf_port_ctrl_port_en(&data1,
                                               0);  // set channel mode first
          setp_tof3_pgr_ebuf_port_ctrl_channel_mode(&data1, 1);
          setp_tof3_pgr_ebuf_port_ctrl_channel_en(&data1,
                                                  (pair_port > port) ? 1 : 2);
          two_steps_op = true;
        } else {
          setp_tof3_pgr_ebuf_port_ctrl_channel_en(&data1, 0);
          setp_tof3_pgr_ebuf_port_ctrl_port_en(&data1, 0);
        }
      }
      if (add) {
        ctx->ebuf_chnl_en[pipe] |= (1u << port);
      } else {
        ctx->ebuf_chnl_en[pipe] &= ~(1u << port);
      }
      if (two_steps_op) {
        pg_write_one_pipe_reg(shdl, dev, 1 << pipe, addr, data1);
        setp_tof3_pgr_ebuf_port_ctrl_port_en(&data1, 1);  // set port en second
      }
      ctx->ebuf_port_ctrl[pipe][port / 2] = data1;
      pg_write_one_pipe_reg(shdl, dev, 1 << pipe, addr, data1);
      break;
    default:
      LOG_ERROR("%s:%d Invalid port, dev %d, port 0x%x, channel number %d",
                __func__,
                __LINE__,
                dev,
                port,
                chnl_numb);
      return BF_INVALID_ARG;
  }

  return BF_SUCCESS;
}

/* add recir port */
static bf_status_t pipe_mgr_tof3_pgr_recir_port_add(pipe_sess_hdl_t shdl,
                                                    int dev,
                                                    int pipe,
                                                    int port,
                                                    uint8_t chnl_numb,
                                                    bf_port_speeds_t speed,
                                                    bool add) {
  uint32_t port_end = (port + chnl_numb - 1);
  bf_status_t sts = BF_SUCCESS;
  // check
  if (pipe_mgr_tof3_get_speed(speed) > 100) {
    LOG_ERROR("%s:%d Invalid speed, not support > 100G recir port, dev %d",
              __func__,
              __LINE__,
              dev);
    return BF_INVALID_ARG;
  }
  // ebuf port ctrl
  sts = pipe_mgr_tof3_pgr_ebuf_port_ctrl_set(
      shdl, dev, pipe, port, chnl_numb, add);
  if (sts != BF_SUCCESS) return sts;
  // ipb port ctrl
  sts = pipe_mgr_tof3_pgr_ipb_ctrl_set(
      shdl, dev, pipe, port, port_end, add, speed);
  return sts;
}
static bf_status_t pipe_mgr_tof3_pgr_get_eth_mode_seq_rm(uint32_t *data_eth,
                                                         int port,
                                                         uint8_t chnl_numb,
                                                         uint32_t *mod_r,
                                                         uint32_t *seq_r) {
  uint32_t mode = 0, seq = 0;
  uint32_t old_mode = getp_tof3_pgr_eth_cpu_ctrl_channel_mode(data_eth);
  uint32_t old_seq = getp_tof3_pgr_eth_cpu_ctrl_channel_seq(data_eth);

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
static bf_status_t pipe_mgr_tof3_pgr_get_eth_mode_seq(uint32_t *data_eth,
                                                      int port,
                                                      uint8_t chnl_numb,
                                                      uint32_t *mod_r,
                                                      uint32_t *seq_r) {
  uint32_t mode = 0, seq = 0, old_seq, recir_dis;
  old_seq = getp_tof3_pgr_eth_cpu_ctrl_channel_mode(data_eth);
  recir_dis = getp_tof3_pgr_eth_cpu_ctrl_channel_en(data_eth);

  // get mode and seq
  switch (recir_dis) {
    case 0xf:
      if (chnl_numb == 4) {
        mode = 0;
        seq = 0;
      } else if (chnl_numb == 2) {
        if (((old_seq == 1) || (old_seq == 3)) && (port == 2)) {
          mode = 1;
          seq = 0x88;
        } else if (((old_seq == 1) || (old_seq == 2)) && (port == 4)) {
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
static bf_status_t pipe_mgr_tof3_pgr_eth_port_add(pipe_sess_hdl_t shdl,
                                                  int dev,
                                                  int pipe,
                                                  int port,
                                                  uint8_t chnl_numb,
                                                  bf_port_speeds_t speed,
                                                  bool add) {
  uint32_t addr, *data_eth;
  uint32_t chnl_en;
  uint32_t i, mode, seq;
  uint32_t port_end = port + chnl_numb - 1;
  bf_status_t sts;
  // chec
  int die_id = pipe / BF_SUBDEV_PIPE_COUNT;
  if (pipe_mgr_tof3_get_speed(speed) > 100) {
    LOG_ERROR(
        "Dev %d unsupported speed-enum %d for PGR eth %s port %d chnl-num %d",
        dev,
        speed,
        add ? "add" : "rmv",
        port,
        chnl_numb);
    return BF_INVALID_ARG;
  }
  if ((tof3_tbc_eth_en[dev][die_id] >> 2) != 0xf) {
    LOG_ERROR("Dev %d PGR eth en 0x%x illegal for %s port %d chnl-num %d",
              dev,
              tof3_tbc_eth_en[dev][die_id] >> 2,
              add ? "add" : "rmv",
              port,
              chnl_numb);
    return BF_INVALID_ARG;
  }
  // eth_cpu_port_ctrl: chnl speed
  addr = offsetof(
      tof3_reg, pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.eth_cpu_port_ctrl);
  data_eth = &(pipe_mgr_tof3_get_pgr_ctx(dev)->port_ctrl);
  chnl_en = getp_tof3_pgr_eth_cpu_ctrl_channel_en(data_eth);
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
  setp_tof3_pgr_eth_cpu_ctrl_channel_en(data_eth, chnl_en);
  setp_tof3_pgr_eth_cpu_ctrl_port_en(data_eth, (chnl_en == 0 ? 0 : 1));
  if (add) {
    sts = pipe_mgr_tof3_pgr_get_eth_mode_seq(
        data_eth, port, chnl_numb, &mode, &seq);
  } else {
    sts = pipe_mgr_tof3_pgr_get_eth_mode_seq_rm(
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
  setp_tof3_pgr_eth_cpu_ctrl_channel_mode(data_eth, mode);
  setp_tof3_pgr_eth_cpu_ctrl_channel_seq(data_eth, seq);
  /* The channel mode must always be four; TF2LAB-41. */
  uint32_t wr_data = *data_eth;
  setp_tof3_pgr_eth_cpu_ctrl_channel_mode(&wr_data, 4);
  pg_write_one_pipe_reg(shdl, dev, 1 << pipe, addr, wr_data);

  // ebuf port ctrl
  if ((chnl_numb == 1) || (chnl_numb == 2)) {
    sts = pipe_mgr_tof3_pgr_ebuf_port_ctrl_set(
        shdl, dev, pipe, port, chnl_numb, add);
  } else if ((chnl_numb == 4) && (port == 2)) {
    sts = pipe_mgr_tof3_pgr_ebuf_port_ctrl_set(shdl, dev, pipe, port, 2, add);
    sts |=
        pipe_mgr_tof3_pgr_ebuf_port_ctrl_set(shdl, dev, pipe, port + 2, 2, add);
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
  sts = pipe_mgr_tof3_pgr_ipb_ctrl_set(
      shdl, dev, pipe, port, port_end, add, speed);
  return sts;
}

/* add tbc port */
static bf_status_t pipe_mgr_tof3_pgr_tbc_port_add(bf_session_hdl_t shdl,
                                                  bf_dev_id_t dev,
                                                  bf_dev_port_t port_id,
                                                  bf_port_speeds_t speed,
                                                  bool add) {
  bf_status_t sts;
  // check speed
  if (pipe_mgr_tof3_get_speed(speed) > 25) {
    LOG_ERROR("%s:%d Invalid speed, not support > 25G tbc port, dev %d",
              __func__,
              __LINE__,
              dev);
    return BF_INVALID_ARG;
  }
  int pipe = DEV_PORT_TO_PIPE(port_id);
  int die_id = pipe / BF_SUBDEV_PIPE_COUNT;
  int local_port = DEV_PORT_TO_LOCAL_PORT(port_id);
  // ipb port ctrl: chnl speed
  if (add) {
    pipe_mgr_tof3_pgr_tbc_port_recir_en(
        shdl, dev, pipe, (tof3_tbc_eth_en[dev][die_id] & 1) == 0, false);
  } else {
    pipe_mgr_tof3_pgr_tbc_port_recir_en(shdl, dev, pipe, true, false);
  }
  sts =
      pipe_mgr_tof3_pgr_ebuf_port_ctrl_set(shdl, dev, pipe, local_port, 1, add);
  if (sts != BF_SUCCESS) return sts;
  sts = pipe_mgr_tof3_pgr_ipb_ctrl_set(
      shdl, dev, pipe, local_port, local_port, add, BF_SPEED_25G);
  return sts;
}

static bool is_port_pcie(bf_dev_id_t dev, bf_dev_port_t port) {
  const bf_dev_port_t pcie_cpu = bf_pcie_cpu_port_get(dev);
  const bf_dev_port_t pcie_cpu2 = bf_pcie_cpu_port2_get(dev);
  return ((port == pcie_cpu) || (port == pcie_cpu2));
}

/* Tofino3
 * add pgr(recirc, tbc, eth cpu) port
 */
bf_status_t pipe_mgr_tof3_pktgen_port_add(rmt_dev_info_t *dev_info,
                                          bf_dev_port_t port_id,
                                          bf_port_speeds_t speed) {
  int dev = dev_info->dev_id;
  int pipe = dev_info->dev_cfg.dev_port_to_pipe(port_id);
  int port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  char *speed_str = NULL;

#if defined(EMU_SKIP_BLOCKS_OPT)
  if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO3) {
    return PIPE_SUCCESS;
  }
#endif

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
  uint8_t chnl_numb = 0;
  /* Use the default session to set this up. */
  pipe_sess_hdl_t sid = pipe_mgr_ctx->int_ses_hndl;

  /* Local port numbers 8-71 do not require any special PGR programming, they
   * only need to be tracked in the port down mask. */
  if (port > 7) {
  }

  /* Ports 2-5 require special handling depending if they are connected to the
   * Ethernet CPU MAC or are in recirculation mode. */
  else if ((port <= 5) && (port >= 2) && (pipe == 0 || pipe == 4)) {
    if (pipe_mgr_tof3_pgr_recir_get(dev, port_id)) {
      /* Port is in recirculation mode, add use 50g channels. */
      chnl_numb = pipe_mgr_tof3_get_chnls(speed, false);
      if ((chnl_numb == 0) || (chnl_numb > 2)) {
        LOG_ERROR(
            "%s:%d Invalid speed %s; recirculation port %d on device %d cannot "
            "use speeds greater than 100g (i.e. two channels at the most).",
            __func__,
            __LINE__,
            speed_str,
            port_id,
            dev);
        return BF_INVALID_ARG;
      }
      sts = pipe_mgr_tof3_pgr_recir_port_add(
          sid, dev, pipe, port, chnl_numb, speed, true);
    } else {
      /* Port is connected to the Ethernet MAC, add it using 25g channels. */
      chnl_numb = pipe_mgr_tof3_get_chnls(speed, true);
      if ((chnl_numb == 0) || (chnl_numb > 4)) {
        LOG_ERROR(
            "%s:%d Invalid speed %s; CPU port %d on device %d cannot "
            "use speeds greater than 100g (i.e. four 25g channels at the "
            "most).",
            __func__,
            __LINE__,
            speed_str,
            port_id,
            dev);
        return BF_INVALID_ARG;
      }
      sts = pipe_mgr_tof3_pgr_eth_port_add(
          sid, dev, pipe, port, chnl_numb, speed, true);
    }
  }

  /* Port 0 requires special handling depending if it is connected to the PCIe
   * (TBus) path or the recirculation path. */
  else if (is_port_pcie(dev, port_id)) {
    if (!pipe_mgr_tof3_pgr_recir_get(dev, port_id)) {
      /* Recirculation is not enabled therefore port 0 is connected to TBUS for
       * packet tx and rx over PCIe. */
      chnl_numb = pipe_mgr_tof3_get_chnls(speed, true);
      if (chnl_numb != 1) {
        LOG_ERROR(
            "%s:%d Invalid speed %s; port 0 on device %d cannot use speeds "
            "greater than 25g when recirculation is disabled.",
            __func__,
            __LINE__,
            speed_str,
            dev);
        return BF_INVALID_ARG;
      }
      sts = pipe_mgr_tof3_pgr_tbc_port_add(sid, dev, port_id, speed, true);
    } else {
      /* Recirculation is enabled therefore port 0 is not connected to TBUS and
       * can use upto two 50g channels. */
      chnl_numb = pipe_mgr_tof3_get_chnls(speed, false);
      if ((chnl_numb == 0) || (chnl_numb > 2)) {
        LOG_ERROR(
            "%s:%d Invalid speed %s; port 0 on device %d cannot use speeds "
            "greater than 100g.",
            __func__,
            __LINE__,
            speed_str,
            dev);
        return BF_INVALID_ARG;
      }
      sts = pipe_mgr_tof3_pgr_recir_port_add(
          sid, dev, pipe, port, chnl_numb, speed, true);
    }
  }

  /* The remaining ports (1, 6, 7 and 0 (but only 0 in pipes 1-3) are always
   * recirculation ports and use 50g channels. */
  else if (port_id & 1) { /* Odd port numbers. */
    chnl_numb = pipe_mgr_tof3_get_chnls(speed, false);
    if (chnl_numb != 1) {
      LOG_ERROR(
          "%s:%d Invalid speed %s; port %d on device %d cannot use speeds "
          "greater than 50g (i.e. single channel only).",
          __func__,
          __LINE__,
          speed_str,
          port_id,
          dev);
      return BF_INVALID_ARG;
    }
    sts = pipe_mgr_tof3_pgr_recir_port_add(
        sid, dev, pipe, port, chnl_numb, speed, true);
  } else { /* Even ports. */
    chnl_numb = pipe_mgr_tof3_get_chnls(speed, false);
    if ((chnl_numb == 0) || (chnl_numb > 2)) {
      LOG_ERROR(
          "%s:%d Invalid speed %s; port %d on device %d cannot use speeds "
          "greater than 100g (i.e. two channels at the most).",
          __func__,
          __LINE__,
          speed_str,
          port_id,
          dev);
      return BF_INVALID_ARG;
    }
    sts = pipe_mgr_tof3_pgr_recir_port_add(
        sid, dev, pipe, port, chnl_numb, speed, true);
  }

  if (BF_SUCCESS == sts) {
    /* Update our mask of created ports. */
    uint8_t *m_mask =
        pipe_mgr_pktgen_ctx(dev)->u.tof3_ctx->port_down_mask[pipe][2].port_mask;
    /* Special case ports 2-5 (ethernet CPU) since it maps to bits 4-7 instead
     * of bits 2-5.  */
    if (pipe == 0 && port >= 2 && port <= 5)
      m_mask[0] |= 1 << (4 + port - 2);
    else
      m_mask[port / 8] |= 1 << (port % 8);
  }
  return sts;
}

bf_status_t pipe_mgr_tof3_pktgen_port_rem(rmt_dev_info_t *dev_info,
                                          bf_dev_port_t port_id) {
  int dev = dev_info->dev_id;
  rmt_port_info_t *port_info = pipe_mgr_get_port_info(dev, port_id);
  if (!port_info) return BF_SUCCESS;
  bf_port_speeds_t speed = port_info->speed;
  uint8_t chnl_numb = 0;
  int pipe = dev_info->dev_cfg.dev_port_to_pipe(port_id);
  int port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  pipe_sess_hdl_t sid = pipe_mgr_ctx->int_ses_hndl;

#if defined(EMU_SKIP_BLOCKS_OPT)
  if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO3) {
    return PIPE_SUCCESS;
  }
#endif
  /* Update our mask of created ports. */
  uint8_t *m_mask =
      pipe_mgr_pktgen_ctx(dev)->u.tof3_ctx->port_down_mask[pipe][2].port_mask;
  /* Special case port 2, ethernet CPU since it maps to bit 4 instead of bit 2.
   */
  if ((pipe == 0 || pipe == 4) && port_id >= 2 && port_id <= 5)
    m_mask[0] &= ~(1 << (4 + port - 2));
  else
    m_mask[port / 8] &= ~(1 << (port % 8));

  bf_status_t sts = BF_SUCCESS;
  if (port > 7) return sts;

  // have to clear ipb_chnl_sp ipb_ctrl
  if (((port <= 5) && (port >= 2) && (pipe == 0 || pipe == 4)) &&
      (!pipe_mgr_tof3_pgr_recir_get(dev, port_id))) {
    // remove eth port
    chnl_numb = pipe_mgr_tof3_get_chnls(speed, true);
    // eth port chnl disable
    sts = pipe_mgr_tof3_pgr_eth_port_add(
        sid, dev, pipe, port, chnl_numb, speed, false);
  } else if ((port == 0) && (pipe == 0 || pipe == 4) &&
             (!pipe_mgr_tof3_pgr_recir_get(dev, port_id))) {
    // remove tbc port
    sts = pipe_mgr_tof3_pgr_tbc_port_add(sid, dev, port_id, speed, false);
  } else {
    // remove recirc port
    chnl_numb = pipe_mgr_tof3_get_chnls(speed, false);
    // ebuf chnl disable
    sts = pipe_mgr_tof3_pgr_recir_port_add(
        sid, dev, pipe, port, chnl_numb, speed, false);
  }
  return sts;
}

bf_status_t pipe_mgr_tof3_recir_en(bf_session_hdl_t shdl,
                                   bf_dev_id_t dev,
                                   bf_dev_port_t port_id,
                                   bool en) {
  (void)shdl;
  int pipe = DEV_PORT_TO_PIPE(port_id);
  int die_id = pipe / BF_SUBDEV_PIPE_COUNT;
  bf_dev_port_t port = DEV_PORT_TO_LOCAL_PORT(port_id);
  if (port_id == 0) {
    if (pipe_mgr_get_port_info(dev, port_id)) {
      LOG_ERROR(
          "%s: Port 0 is in use cannot %sable recirculation, dev %d port %d",
          __func__,
          en ? "en" : "dis",
          dev,
          port);
      return BF_IN_USE;
    }
    if (en) {
      tof3_tbc_eth_en[dev][die_id] &= ~(1u);
    } else {
      tof3_tbc_eth_en[dev][die_id] |= 1;
    }
    return BF_SUCCESS;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) return PIPE_UNEXPECTED;
  if (port_id < 2 || port_id > 5) return BF_SUCCESS;
  uint8_t in_use = 0;
  for (int i = 2; i <= 5; i++) {
    if ((i % 2) != 0) continue;
    bf_dev_port_t dev_port = dev_info->dev_cfg.make_dev_port(pipe, i);
    if (pipe_mgr_get_port_info(dev, dev_port)) {
      /* The port is in use but only log it if it has a different recirc mode
       * than what is being requested. */
      if (((~tof3_tbc_eth_en[dev][die_id] >> i) & 1) != en) in_use |= (1 << i);
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
    tof3_tbc_eth_en[dev][die_id] &= ~(1u << port);
  } else {
    tof3_tbc_eth_en[dev][die_id] |= (1u << port);
  }

  /* Get the current value of eth_cpu_port_ctrl. */
  uint32_t *data = &pipe_mgr_tof3_get_pgr_ctx(dev)->port_ctrl;
  uint32_t addr = offsetof(
      tof3_reg, pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.eth_cpu_port_ctrl);
  bf_dev_port_t tbc_port = bf_pcie_cpu_port_get(dev);
  bf_dev_pipe_t cpu_pipe = DEV_PORT_TO_PIPE(tbc_port);

  /* Get the current recirc mode from it. */
  bool current = 0 == (*data & 1);

  /* Update it if the modes are different. */
  if (current != en) {
    setp_tof3_pgr_eth_cpu_ctrl_port_en(data, en ? 0 : 1);
    pg_write_one_pipe_reg(shdl, dev, 1u << cpu_pipe, addr, *data);
  }

  return BF_SUCCESS;
}
bool pipe_mgr_tof3_pgr_recir_get(bf_dev_id_t dev, bf_dev_port_t port) {
  bf_dev_port_t eth_port_min = bf_eth_cpu_port_get(dev);
  bf_dev_port_t eth_port_max = eth_port_min + 3;
  bf_dev_port_t local_port = DEV_PORT_TO_LOCAL_PORT(port);
  int die_id = DEV_PORT_TO_PIPE(port) / BF_SUBDEV_PIPE_COUNT;
  bf_dev_port_t tbc_port = 0;
  if (die_id == 0) {
    tbc_port = bf_pcie_cpu_port_get(dev);
  } else {
    tbc_port = bf_pcie_cpu_port2_get(dev);
  }

  if (port == tbc_port ||
      (local_port >= eth_port_min && local_port <= eth_port_max)) {
    return ((tof3_tbc_eth_en[dev][die_id] >> local_port) & 1) == 0;
  }
  return true;
}
bf_status_t pipe_mgr_tof3_pktgen_en(bf_session_hdl_t shdl,
                                    bf_dev_id_t dev,
                                    bf_dev_port_t port,
                                    bool en) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) return PIPE_UNEXPECTED;
  bf_dev_port_t tbc_port = 0;

  int pipe_id = dev_info->dev_cfg.dev_port_to_pipe(port);
  int port_id = dev_info->dev_cfg.dev_port_to_local_port(port);
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
  int die_id = pipe_id / BF_SUBDEV_PIPE_COUNT;
  if (die_id == 0) {
    tbc_port = bf_pcie_cpu_port_get(dev);
  } else {
    tbc_port = bf_pcie_cpu_port2_get(dev);
  }
  if (tbc_port == -1) return BF_INVALID_ARG;

  if (en && port == tbc_port && (tof3_tbc_eth_en[dev][die_id] & 1)) {
    return BF_INVALID_ARG;
  }
  if (port_id <= 5 && port_id >= 2 && (pipe_id == 0 || pipe_id == 4) &&
      ((tof3_tbc_eth_en[dev][die_id] >> port) & 1)) {
    /* This port exists as an eth-cpu port so recirculation is disabled and
     * cannot be enabled. */
    if (en) {
      LOG_ERROR("%s:%d Invalid parameter, dev %d, port 0x%x, %s",
                __func__,
                __LINE__,
                dev,
                port,
                (en ? "enable" : "disable"));
      return BF_INVALID_ARG;
    } else {
      return BF_SUCCESS;
    }
  }

  // Cannot enable/disable one channel in a port
  if (!pipe_mgr_tof3_ipb_conflict_check(dev, pipe_id, port_id)) {
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
      tof3_reg,
      pipes[0]
          .pardereg.pgstnreg.pgrreg.pgr_common.ipb_port_ctrl.ipb_port_ctrl_0_2);
  data = &(pipe_mgr_tof3_get_pgr_ctx(dev)->ipb_ctrl[pipe_id]);
  pgen_chnl_en =
      getp_tof3_pgr_ipb_port_ctrl_ipb_port_ctrl_0_2_pgen_channel_en(data);
  if (en) {
    pgen_chnl_en |= 1u << port_id;
  } else {
    pgen_chnl_en &= ~(1u << port_id);
  }
  setp_tof3_pgr_ipb_port_ctrl_ipb_port_ctrl_0_2_pgen_channel_en(data,
                                                                pgen_chnl_en);
  // ipb_ctrl only shadows bit0:31 of the wide register
  // recalculate here to get the same value
  sts = pipe_mgr_tof3_get_ipb_seq(dev, pipe_id, &seq);
  if (sts != BF_SUCCESS) return sts;
  setp_tof3_pgr_ipb_port_ctrl_ipb_port_ctrl_0_2_channel_seq_7_0(data,
                                                                seq & 0xff);
  pg_write_one_pipe_reg(shdl, dev, 1 << pipe_id, addr, *data);
  addr = offsetof(
      tof3_reg,
      pipes[0]
          .pardereg.pgstnreg.pgrreg.pgr_common.ipb_port_ctrl.ipb_port_ctrl_1_2);
  data1 = 0;
  setp_tof3_pgr_ipb_port_ctrl_ipb_port_ctrl_1_2_channel_seq_23_8(&data1,
                                                                 seq >> 8);
  pg_write_one_pipe_reg(shdl, dev, 1 << pipe_id, addr, data1);
  return BF_SUCCESS;
}

bf_status_t pipe_mgr_tof3_pktgen_get_port_en(rmt_dev_info_t *dev_info,
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
      (pipe_mgr_tof3_get_pgr_ctx(dev_info->dev_id)->ipb_ctrl[pipe_id]);
  pgen_chnl_en =
      getp_tof3_pgr_ipb_port_ctrl_ipb_port_ctrl_0_2_pgen_channel_en(&data);
  if ((pgen_chnl_en & (1u << port_id)) != 0)
    *is_enabled = true;
  else
    *is_enabled = false;
  return BF_SUCCESS;
}

bf_status_t pipe_mgr_tof3_pktgen_pgr_com_port_down_clr_get(
    pipe_sess_hdl_t sid,
    int dev,
    int logical_pipe,
    int local_port_bit_idx,
    bool *is_cleared) {
  uint32_t addr = 0, data = 0;
  if (local_port_bit_idx < 32) {
    addr = offsetof(tof3_reg,
                    pipes[0]
                        .pardereg.pgstnreg.pgrreg.pgr_common.port_down_dis
                        .port_down_dis_0_3);
  } else if (local_port_bit_idx < 64) {
    addr = offsetof(tof3_reg,
                    pipes[0]
                        .pardereg.pgstnreg.pgrreg.pgr_common.port_down_dis
                        .port_down_dis_1_3);
  } else {
    addr = offsetof(tof3_reg,
                    pipes[0]
                        .pardereg.pgstnreg.pgrreg.pgr_common.port_down_dis
                        .port_down_dis_2_3);
  }
  if ((logical_pipe == 0) &&
      (local_port_bit_idx >= 2 && local_port_bit_idx <= 5)) {
    /* Special case ports 2-5 (ethernet CPU) since it maps to bits 4-7 instead
     * of bits 2-5.  */
    local_port_bit_idx += 2;
  }
  bf_status_t sts = pg_tof3_read_one_pipe_reg(dev, logical_pipe, addr, &data);
  if (BF_SUCCESS != sts) return BF_HW_COMM_FAIL;
  *is_cleared =
      ((data >> (local_port_bit_idx % 32)) & 0x1) == 0x1 ? false : true;
  (void)sid;
  return BF_SUCCESS;
}

bf_status_t pipe_mgr_tof3_pktgen_pgr_com_port_down_clr(pipe_sess_hdl_t sid,
                                                       int dev,
                                                       int logical_pipe,
                                                       int local_port_bit_idx) {
  if ((logical_pipe == 0) &&
      (local_port_bit_idx >= 2 && local_port_bit_idx <= 5)) {
    /* Special case ports 2-5 (ethernet CPU) since it maps to bits 4-7 instead
     * of bits 2-5.  */
    local_port_bit_idx += 2;
  }
  uint32_t data = 1u << (local_port_bit_idx % 32);
  uint32_t addr = 0;
  if (local_port_bit_idx < 32) {
    addr = offsetof(tof3_reg,
                    pipes[0]
                        .pardereg.pgstnreg.pgrreg.pgr_common.port_down_dis
                        .port_down_dis_0_3);
  } else if (local_port_bit_idx < 64) {
    addr = offsetof(tof3_reg,
                    pipes[0]
                        .pardereg.pgstnreg.pgrreg.pgr_common.port_down_dis
                        .port_down_dis_1_3);
  } else {
    addr = offsetof(tof3_reg,
                    pipes[0]
                        .pardereg.pgstnreg.pgrreg.pgr_common.port_down_dis
                        .port_down_dis_2_3);
  }
  return pg_write_one_pipe_reg(sid, dev, 1 << logical_pipe, addr, data);
}

void pipe_mgr_tof3_pktgen_txn_commit(int dev) {
  struct pipe_mgr_tof3_pg_dev_ctx *ctx = pipe_mgr_tof3_get_pgr_ctx(dev);
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info || !ctx) {
    PIPE_MGR_ASSERT(0);
    return;
  }
  ctx->app.b_valid = false;
  for (uint32_t i = 0; i < dev_info->num_active_pipes; ++i) {
    ctx->pkt_buffer_shadow[i].txn_data_valid = false;
  }
  return;
}

void pipe_mgr_tof3_pktgen_txn_abort(int dev, int max_app_id, int active_pipes) {
  int p, i;
  const uint32_t buf_sz = PIPE_MGR_PKT_BUFFER_SIZE;
  struct pkt_buffer_shadow_t *pkt_buf_shadow = NULL;
  struct pipe_mgr_tof3_pg_dev_ctx *ctx = pipe_mgr_tof3_get_pgr_ctx(dev);
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!ctx || !dev_info) {
    PIPE_MGR_ASSERT(0);
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

void pipe_mgr_tof3_pkt_buffer_shadow_mem_update(bf_dev_id_t dev,
                                                bf_dev_pipe_t pipe,
                                                uint32_t offset,
                                                const uint8_t *buf,
                                                uint32_t size,
                                                bool txn) {
  const uint32_t buf_sz = PIPE_MGR_PKT_BUFFER_SIZE;
  PIPE_MGR_ASSERT(offset < PIPE_MGR_PKT_BUFFER_SIZE);
  PIPE_MGR_ASSERT(size <= PIPE_MGR_PKT_BUFFER_SIZE - offset);
  struct pkt_buffer_shadow_t *pkt_buf_shadow = NULL;
  bf_dev_target_t dev_tgt = {dev, pipe};
  int pipe_mask = pg_log_pipe_mask(dev_tgt);
  for (int i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1 << i))) continue;
    pkt_buf_shadow = &pipe_mgr_tof3_get_pgr_ctx(dev)->pkt_buffer_shadow[i];
    if (txn && !pkt_buf_shadow->txn_data_valid) {
      pkt_buf_shadow->txn_data_valid = true;
      PIPE_MGR_MEMCPY(pkt_buf_shadow->txn_data, pkt_buf_shadow->data, buf_sz);
    }
    PIPE_MGR_MEMCPY(&(pkt_buf_shadow->data[offset]), buf, size);
  }
}

bf_status_t pipe_mgr_tof3_pkt_buffer_shadow_mem_get(bf_dev_id_t dev,
                                                    bf_dev_pipe_t pipe,
                                                    uint32_t offset,
                                                    uint8_t *buf,
                                                    uint32_t size) {
  bf_dev_pipe_t i = (pipe == BF_DEV_PIPE_ALL) ? 0 : pipe;
  struct pipe_mgr_tof3_pg_dev_ctx *ctx = pipe_mgr_tof3_get_pgr_ctx(dev);
  PIPE_MGR_MEMCPY(buf, &(ctx->pkt_buffer_shadow[i].data[offset]), size);
  return BF_SUCCESS;
}

bf_status_t pipe_mgr_tof3_pkt_buffer_write_from_shadow(bf_session_hdl_t shdl,
                                                       bf_dev_id_t dev,
                                                       bf_dev_pipe_t log_pipe) {
  uint64_t addr = 0;
  uint32_t buf_sz = 0, size = 0, elem_size = 0, count = 0;
  bf_status_t sts = BF_SUCCESS;
  pipe_mgr_drv_ses_state_t *st;
  st = pipe_mgr_drv_get_ses_state(&shdl, __func__, __LINE__);
  if (!st) {
    return PIPE_INVALID_ARG;
  }
  /* Form the address using the physical pipe rather than the logical pipe. */
  bf_dev_target_t dev_tgt;
  dev_tgt.device_id = dev;
  dev_tgt.dev_pipe_id = log_pipe;
  int log_pipe_msk = pg_log_pipe_mask(dev_tgt);
  sts = pipe_mgr_pkt_buffer_tof3_addr_get(&addr, &elem_size, &count);
  /* Write the entire 16K pkt buffer */
  size = elem_size * count;
  if (sts != BF_SUCCESS) {
    LOG_ERROR("%s:%d Get memory info error, dev %d, elem_size 0x%x, count 0x%x",
              __func__,
              __LINE__,
              dev,
              elem_size,
              count);
    return sts;
  }
  buf_sz = pipe_mgr_drv_buf_size(dev, PIPE_MGR_DRV_BUF_BWR);
  if ((size != PIPE_MGR_PKT_BUFFER_SIZE) || (size > buf_sz)) {
    LOG_ERROR("%s:%d Get memory info error, dev %d, elem_size 0x%x, count 0x%x",
              __func__,
              __LINE__,
              dev,
              elem_size,
              count);
    return PIPE_INVALID_ARG;
  }
  /* Since each pipe might have different buffer contents write them
   * individually. */
  int i;
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(log_pipe_msk & (1 << i))) continue;
    pipe_mgr_drv_buf_t *b = pipe_mgr_drv_buf_alloc(
        st->sid, dev, buf_sz, PIPE_MGR_DRV_BUF_BWR, true);
    if (!b) {
      LOG_ERROR("%s:%d Error allocating buffer for dev %d",
                __func__,
                __LINE__,
                dev_tgt.device_id);
      return BF_NO_SYS_RESOURCES;
    }
    PIPE_MGR_MEMCPY(
        b->addr,
        &(pipe_mgr_tof3_get_pgr_ctx(dev)->pkt_buffer_shadow[log_pipe].data),
        size);

    pipe_status_t s = pipe_mgr_drv_blk_wr(&shdl,
                                          PIPE_MGR_PKT_BUFFER_WIDTH,
                                          PIPE_MGR_PKT_BUFFER_MEM_ROWS,
                                          1,
                                          addr,
                                          1 << log_pipe,
                                          b);
    if (PIPE_SUCCESS != s) {
      LOG_ERROR("Packet Generator buffer udpate fails (%s) dev %d log_pipe %#x",
                pipe_str_err(s),
                dev,
                log_pipe);
      return BF_HW_COMM_FAIL;
    }
  }
  return BF_SUCCESS;
}

static bf_status_t pipe_mgr_tof3_pktgen_reg_write_mem_with_ilist(
    bf_session_hdl_t shdl,
    bf_dev_id_t dev,
    bf_dev_pipe_t pipe,
    int row,
    int cnt,
    uint64_t base,
    uint64_t step) {
  pipe_bitmap_t pbm = {{0}};
  rmt_dev_info_t *dev_info = NULL;

  PIPE_BITMAP_INIT(&pbm, PIPE_BMP_SIZE);
  uint32_t pipe_cnt = pipe_mgr_get_num_active_pipes(dev);
  uint32_t p;
  for (p = 0; p < pipe_cnt; ++p) {
    if (BF_DEV_PIPE_ALL == pipe) {
      PIPE_BITMAP_SET(&pbm, p);
    } else if (pipe == p) {
      PIPE_BITMAP_SET(&pbm, p);
    }
  }
  dev_info = pipe_mgr_get_dev_info(dev);
  if (dev_info == NULL) {
    LOG_ERROR("%s:%d Device info for device id %d not found",
              __func__,
              __LINE__,
              dev);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Reset pipe so that it can be used to index the shadow. */
  if (BF_DEV_PIPE_ALL == pipe) pipe = 0;

  uint32_t stage;
  lld_err_t lld_err = lld_sku_get_prsr_stage(dev, &stage);
  if (LLD_OK != lld_err) {
    LOG_ERROR("Cannot get pgen stage, error %d, from %s", lld_err, __func__);
    PIPE_MGR_DBGCHK(LLD_OK == lld_err);
    return PIPE_UNEXPECTED;
  }

  int i;
  uint64_t addr = base + step * row;
  pipe_instr_common_wd0_t instr = {0};
  instr.pipe_ring_addr_type = 2;
  instr.data_width = 3;
  instr.specific = (uint32_t)addr & 0x1FFFFF;
  for (i = 0; i < cnt; ++i) {
    pipe_status_t sts =
        pipe_mgr_drv_ilist_add_2(&shdl,
                                 dev_info,
                                 &pbm,
                                 stage,
                                 (uint8_t *)&instr,
                                 sizeof instr,
                                 &pipe_mgr_tof3_get_pgr_ctx(dev)
                                      ->pkt_buffer_shadow[pipe]
                                      .data[row * PIPE_MGR_PKT_BUFFER_WIDTH],
                                 PIPE_MGR_PKT_BUFFER_WIDTH);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR(
          "Failed to post instruction to update pkt-gen buffer on dev %d, pipe "
          "%x, err %s",
          dev,
          pipe,
          pipe_str_err(sts));
      return BF_HW_COMM_FAIL;
    }
    ++row;
    instr.specific += step;
  }
  return BF_SUCCESS;
}

bf_status_t pipe_mgr_tof3_pktgen_write_pkt_buffer(bf_session_hdl_t shdl,
                                                  bf_dev_target_t dev_tgt,
                                                  int row,
                                                  int num_rows) {
  bf_status_t sts = BF_SUCCESS;
  uint32_t elem_size = 0, count = 0;
  uint64_t addr = 0;
  sts = pipe_mgr_pkt_buffer_tof3_addr_get(&addr, &elem_size, &count);
  /* Write the data to asic from shadow mem */
  sts |= pipe_mgr_tof3_pktgen_reg_write_mem_with_ilist(shdl,
                                                       dev_tgt.device_id,
                                                       dev_tgt.dev_pipe_id,
                                                       row,
                                                       num_rows,
                                                       addr,
                                                       elem_size / 16);

  return sts;
}

bf_status_t pipe_mgr_tof3_pktgen_reg_app_batch_ctr(int dev,
                                                   int logical_pipe,
                                                   int aid,
                                                   uint64_t *val) {
  bf_status_t sts = BF_SUCCESS;
  uint32_t hi = 0, lo = 0;

  uint32_t addrLo = offsetof(tof3_reg,
                             pipes[0]
                                 .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                                 .ctr48_batch.ctr48_batch_0_2);
  uint32_t addrHi = offsetof(tof3_reg,
                             pipes[0]
                                 .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                                 .ctr48_batch.ctr48_batch_1_2);
  sts = pg_tof3_read_one_pipe_reg(dev, logical_pipe, addrLo, &lo);
  if (BF_SUCCESS != sts) return BF_HW_COMM_FAIL;
  sts = pg_tof3_read_one_pipe_reg(dev, logical_pipe, addrHi, &hi);
  if (BF_SUCCESS != sts) return BF_HW_COMM_FAIL;

  uint64_t x = hi;
  uint64_t y = lo;
  *val = (x << 32) | (y & UINT64_C(0xFFFFFFFF));

  return BF_SUCCESS;
}

bf_status_t pipe_mgr_tof3_pktgen_reg_app_pkt_ctr(int dev,
                                                 int logical_pipe,
                                                 int aid,
                                                 uint64_t *val) {
  bf_status_t sts = BF_SUCCESS;
  uint32_t hi = 0, lo = 0;
  uint32_t addrLo = offsetof(tof3_reg,
                             pipes[0]
                                 .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                                 .ctr48_packet.ctr48_packet_0_2);
  uint32_t addrHi = offsetof(tof3_reg,
                             pipes[0]
                                 .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                                 .ctr48_packet.ctr48_packet_1_2);
  sts = pg_tof3_read_one_pipe_reg(dev, logical_pipe, addrLo, &lo);
  if (BF_SUCCESS != sts) return BF_HW_COMM_FAIL;
  sts = pg_tof3_read_one_pipe_reg(dev, logical_pipe, addrHi, &hi);
  if (BF_SUCCESS != sts) return BF_HW_COMM_FAIL;

  uint64_t x = hi;
  uint64_t y = lo;
  *val = (x << 32) | (y & UINT64_C(0xFFFFFFFF));
  return BF_SUCCESS;
}

bf_status_t pipe_mgr_tof3_pktgen_reg_app_trig_ctr(int dev,
                                                  int logical_pipe,
                                                  int aid,
                                                  uint64_t *val) {
  bf_status_t sts = BF_SUCCESS;
  uint32_t hi = 0, lo = 0;

  uint32_t addrLo = offsetof(tof3_reg,
                             pipes[0]
                                 .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                                 .ctr48_trigger.ctr48_trigger_0_2);
  uint32_t addrHi = offsetof(tof3_reg,
                             pipes[0]
                                 .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                                 .ctr48_trigger.ctr48_trigger_1_2);
  sts = pg_tof3_read_one_pipe_reg(dev, logical_pipe, addrLo, &lo);
  if (BF_SUCCESS != sts) return BF_HW_COMM_FAIL;
  sts = pg_tof3_read_one_pipe_reg(dev, logical_pipe, addrHi, &hi);
  if (BF_SUCCESS != sts) return BF_HW_COMM_FAIL;

  uint64_t x = hi;
  uint64_t y = lo;
  *val = (x << 32) | (y & UINT64_C(0xFFFFFFFF));
  return BF_SUCCESS;
}

bf_status_t pipe_mgr_tof3_pktgen_reg_app_batch_ctr_set(
    bf_session_hdl_t shdl, int dev, int logical_pipe, int aid, uint64_t val) {
  bf_status_t sts = BF_SUCCESS;
  uint32_t hi = val >> 32;
  uint32_t lo = val & UINT64_C(0xFFFFFFFF);
  bf_dev_target_t dev_tgt;
  dev_tgt.device_id = dev;
  dev_tgt.dev_pipe_id = logical_pipe;
  int pipe_mask = pg_log_pipe_mask(dev_tgt);

  uint32_t addrLo = offsetof(tof3_reg,
                             pipes[0]
                                 .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                                 .ctr48_batch.ctr48_batch_0_2);
  uint32_t addrHi = offsetof(tof3_reg,
                             pipes[0]
                                 .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                                 .ctr48_batch.ctr48_batch_1_2);
  sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addrLo, lo);
  if (BF_SUCCESS != sts) return BF_HW_COMM_FAIL;
  sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addrHi, hi);
  if (BF_SUCCESS != sts) return BF_HW_COMM_FAIL;
  return BF_SUCCESS;
}

bf_status_t pipe_mgr_tof3_pktgen_reg_app_pkt_ctr_set(
    bf_session_hdl_t shdl, int dev, int logical_pipe, int aid, uint64_t val) {
  bf_status_t sts = BF_SUCCESS;
  uint32_t hi = val >> 32;
  uint32_t lo = val & UINT64_C(0xFFFFFFFF);
  bf_dev_target_t dev_tgt;
  dev_tgt.device_id = dev;
  dev_tgt.dev_pipe_id = logical_pipe;
  int pipe_mask = pg_log_pipe_mask(dev_tgt);

  uint32_t addrLo = offsetof(tof3_reg,
                             pipes[0]
                                 .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                                 .ctr48_packet.ctr48_packet_0_2);
  uint32_t addrHi = offsetof(tof3_reg,
                             pipes[0]
                                 .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                                 .ctr48_packet.ctr48_packet_1_2);
  sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addrLo, lo);
  if (BF_SUCCESS != sts) return BF_HW_COMM_FAIL;
  sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addrHi, hi);
  if (BF_SUCCESS != sts) return BF_HW_COMM_FAIL;
  return BF_SUCCESS;
}

bf_status_t pipe_mgr_tof3_pktgen_reg_app_trig_ctr_set(
    bf_session_hdl_t shdl, int dev, int logical_pipe, int aid, uint64_t val) {
  bf_status_t sts = BF_SUCCESS;
  uint32_t hi = val >> 32;
  uint32_t lo = val & UINT64_C(0xFFFFFFFF);
  bf_dev_target_t dev_tgt;
  dev_tgt.device_id = dev;
  dev_tgt.dev_pipe_id = logical_pipe;
  int pipe_mask = pg_log_pipe_mask(dev_tgt);

  uint32_t addrLo = offsetof(tof3_reg,
                             pipes[0]
                                 .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                                 .ctr48_trigger.ctr48_trigger_0_2);
  uint32_t addrHi = offsetof(tof3_reg,
                             pipes[0]
                                 .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                                 .ctr48_trigger.ctr48_trigger_1_2);
  sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addrLo, lo);
  if (BF_SUCCESS != sts) return BF_HW_COMM_FAIL;
  sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addrHi, hi);
  if (BF_SUCCESS != sts) return BF_HW_COMM_FAIL;
  return BF_SUCCESS;
}

static void pipe_mgr_tof3_pg_txn_bkup_app_cfg(pipe_sess_hdl_t shdl, int dev) {
  struct pipe_mgr_tof3_pg_dev_ctx *ctx = pipe_mgr_tof3_get_pgr_ctx(dev);
  bool txn = pipe_mgr_sess_in_txn(shdl);
  if (!txn || ctx->app.b_valid) return;
  uint32_t p, i;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    LOG_ERROR(
        "%s:%d Invalid device %d for shdl %d", __func__, __LINE__, dev, shdl);
    return;
  }
  // rmt_dev_cfg_t *cfg = &dev_info->dev_cfg;
  ctx->app.b_valid = true;
  for (p = 0; p < dev_info->num_active_pipes; ++p) {
    for (i = 0; i < PIPE_MGR_TOF3_PKTGEN_APP_CNT; ++i) {
      PIPE_MGR_MEMCPY(
          &ctx->app.b[p][i], &ctx->app.a[p][i], sizeof ctx->app.b[p][i]);
    }
  }
}

bf_status_t pipe_mgr_tof3_pktgen_get_app_ctrl_en(bf_dev_target_t dev_tgt,
                                                 int app_id,
                                                 bool *is_enabled) {
  int i, pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t val = 0;
  uint32_t en_tmp[BF_PIPE_COUNT] = {0};
  int val_cnt = 0;
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1 << i))) continue;
    val =
        pipe_mgr_tof3_get_pgr_ctx(dev_tgt.device_id)->app.a[i][app_id].app_ctrl;
    en_tmp[val_cnt++] = getp_tof3_pgr_app_ctrl_app_en(&val);
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

bf_status_t pipe_mgr_tof3_pktgen_reg_pgr_app_ctrl_en(pipe_sess_hdl_t sid,
                                                     int dev,
                                                     bf_dev_pipe_t logical_pipe,
                                                     int aid,
                                                     bool en) {
  bf_status_t sts = BF_SUCCESS;
  bf_dev_target_t dev_tgt = {dev, logical_pipe};
  struct pipe_mgr_tof3_pg_dev_ctx *ctx = pipe_mgr_tof3_get_pgr_ctx(dev);
  int i, pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t vals[BF_PIPE_COUNT] = {0}, val = 0, addr = 0;
  int val_cnt = 0;
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1 << i))) continue;
    vals[val_cnt] = ctx->app.a[i][aid].app_ctrl;
    setp_tof3_pgr_app_ctrl_app_en(&vals[val_cnt], en);
    /* Set the init_lfsr to a different value per app */
    setp_tof3_pgr_app_ctrl_init_lfsr(&vals[val_cnt], aid);
    val = vals[val_cnt++];
  }
  for (i = 0; i < val_cnt; ++i) {
    if (val != vals[i]) {
      LOG_ERROR(
          "Error, cannot program asymmetric data to multiple pipes. "
          "%#x != %#x dev %d pipe %#x at %s:%d",
          val,
          vals[i],
          dev,
          logical_pipe,
          __func__,
          __LINE__);
      return BF_INVALID_ARG;
    }
  }

  int log_pipe = logical_pipe == BF_DEV_PIPE_ALL ? 0 : logical_pipe;
  /* If the app is a port-down app and the port down replay mode is set then
   * toggle the enable in pgen_retrigger_port_down before enabling the app. */
  if (getp_tof3_pgr_app_ctrl_app_type(&vals[log_pipe]) ==
          BF_PKTGEN_TRIGGER_PORT_DOWN &&
      en) {
    uint32_t x = 0;
    bool symmertic = true;
    rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
    if (!dev_info) return BF_INVALID_ARG;
    uint32_t pipe_num = dev_info->num_active_pipes;
    if (pipe_num < 1) return BF_UNEXPECTED;
    bf_pktgen_port_down_mode_t mode[pipe_num];

    if (BF_DEV_PIPE_ALL == logical_pipe) {
      int num_active_pipes = dev_info->num_active_pipes;
      if (num_active_pipes < 1) return BF_UNEXPECTED;
      for (i = 0; i < num_active_pipes; ++i) {
        mode[i] = ctx->port_down_mode[0];
        if (mode[0] != ctx->port_down_mode[i]) {
          symmertic = false;
        }
      }
    } else {
      mode[0] = ctx->port_down_mode[logical_pipe];
      symmertic = true;
    }

    addr = offsetof(
        tof3_reg,
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
        sts = pg_write_one_pipe_reg(sid, dev, pipe_mask, addr, x & ~1u);
        if (sts != BF_SUCCESS) return sts;
        sts = pg_write_one_pipe_reg(sid, dev, pipe_mask, addr, x);
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
          sts = pg_write_one_pipe_reg(sid, dev, 1u << i, addr, x & ~1u);
          if (sts != BF_SUCCESS) return sts;
          sts = pg_write_one_pipe_reg(sid, dev, 1u << i, addr, x);
          if (sts != BF_SUCCESS) return sts;
        }
      }
    }
  }

  pipe_mgr_tof3_pg_txn_bkup_app_cfg(sid, dev);
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1 << i))) continue;
    ctx->app.a[i][aid].app_ctrl = val;
  }
  addr =
      offsetof(tof3_reg, pipes[0].pardereg.pgstnreg.pgrreg.pgr_app[aid].ctrl);
  return pg_write_one_pipe_reg(sid, dev, pipe_mask, addr, val);
}

static uint32_t pipe_mgr_tof3_pktgen_app_ctrl_type(int dev,
                                                   int logical_pipe,
                                                   int aid) {
  return getp_tof3_pgr_app_ctrl_app_type(
      &pipe_mgr_tof3_get_pgr_ctx(dev)->app.a[logical_pipe][aid].app_ctrl);
}
static uint32_t pipe_mgr_tof3_pktgen_app_ctrl_en(int dev,
                                                 int logical_pipe,
                                                 int aid) {
  return getp_tof3_pgr_app_ctrl_app_en(
      &pipe_mgr_tof3_get_pgr_ctx(dev)->app.a[logical_pipe][aid].app_ctrl);
}

bf_status_t pipe_mgr_tof3_pktgen_cfg_app_conf_check(rmt_dev_info_t *dev_info,
                                                    bf_dev_target_t dev_tgt,
                                                    int app_id,
                                                    bf_pktgen_app_cfg_t *cfg) {
  int dev = dev_tgt.device_id;
  int pipe = dev_tgt.dev_pipe_id;
  uint32_t a = pipe_mgr_get_num_active_pipes(dev);
  /* Cannot change trigger_type while the app is enabled. */
  if (BF_DEV_PIPE_ALL == dev_tgt.dev_pipe_id) {
    uint32_t i;
    for (i = 0; i < a; ++i) {
      uint32_t x = pipe_mgr_tof3_pktgen_app_ctrl_type(dev, i, app_id);
      uint32_t y = pipe_mgr_tof3_pktgen_app_ctrl_en(dev, i, app_id);
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
    uint32_t x = pipe_mgr_tof3_pktgen_app_ctrl_type(dev, pipe, app_id);
    uint32_t y = pipe_mgr_tof3_pktgen_app_ctrl_en(dev, pipe, app_id);
    if (y && x != cfg->trigger_type) {
      LOG_ERROR("%s:%d Application in use, dev %d, app_id %d",
                __func__,
                __LINE__,
                dev,
                app_id);
      return BF_IN_USE;
    }
  }
  if (app_id >= PIPE_MGR_TOF3_PKTGEN_APP_CNT) {
    LOG_ERROR("%s:%d Invalid application id, dev %d, app_id %d",
              __func__,
              __LINE__,
              dev,
              app_id);
    return BF_INVALID_ARG;
  }
  /* Cannot set illegal source ports. */
  if (!dev_info->dev_cfg.dev_port_validate(cfg->pipe_local_source_port)) {
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
  if (cfg->increment_source_port && cfg->tof2.source_port_wrap_max > 71) {
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
      cfg->packets_per_batch > (71 - cfg->pipe_local_source_port)) {
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

  /* Must generate at least a 64 byte packet.  We will add 6 bytes of pgen
   * header and 4 bytes of CRC though. */
  if ((cfg->length + 6 < 64) && !cfg->tof2.offset_len_from_recir_pkt) {
    LOG_ERROR("%s:%d Packet length, %d, is too small.  Must be at least %d",
              __func__,
              __LINE__,
              cfg->length,
              64 - 6);
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

static bf_status_t pipe_mgr_tof3_pktgen_reg_pgr_app_ctrl(pipe_sess_hdl_t sid,
                                                         int dev,
                                                         int logical_pipe,
                                                         int aid,
                                                         int chnl,
                                                         int mask_sel,
                                                         int type) {
  LOG_TRACE("PktGenAppCfg: dev %d pipe %d app %d shdl %d, %s",
            dev,
            logical_pipe,
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
  bf_dev_target_t dev_tgt = {dev, logical_pipe};
  struct pipe_mgr_tof3_pg_dev_ctx *ctx = pipe_mgr_tof3_get_pgr_ctx(dev);
  int i, pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t *data = NULL;
  uint32_t addr =
      offsetof(tof3_reg, pipes[0].pardereg.pgstnreg.pgrreg.pgr_app[aid].ctrl);
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1u << i))) continue;
    data = &ctx->app.a[i][aid].app_ctrl;
    if (getp_tof3_pgr_app_ctrl_app_en(data) == 1) {
      LOG_ERROR(
          "Error, cannot program to a pipe while an application has already "
          "been"
          "enabled"
          "dev %d pipe %#x error_pipe %d at %s:%d",
          dev,
          logical_pipe,
          i,
          __func__,
          __LINE__);
      return BF_INVALID_ARG;
    }
  }
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1u << i))) continue;
    data = &ctx->app.a[i][aid].app_ctrl;
    setp_tof3_pgr_app_ctrl_app_type(data, type);
    setp_tof3_pgr_app_ctrl_app_chnl(data, chnl);
    setp_tof3_pgr_app_ctrl_app_stop_at_pkt_bndry(
        data, 1);  // fix to stop at pkt boundry
    if (type == BF_PKTGEN_TRIGGER_PORT_DOWN) {
      setp_tof3_pgr_app_ctrl_app_port_down_mask_sel(data, mask_sel);
    } else {
      (void)chnl;
    }
    // timestamp, prio, nokey, at_pkt_boundry
    // pg_write_one_pipe_reg(sid, dev, 1<<i, addr, *data);
  }
  if (data == NULL) return BF_INVALID_ARG;
  pipe_mgr_tof3_pg_txn_bkup_app_cfg(sid, dev);
  return pg_write_one_pipe_reg(sid, dev, pipe_mask, addr, *data);
}

static bf_status_t pipe_mgr_tof3_pktgen_reg_pgr_app_payload_ctrl(
    pipe_sess_hdl_t sid,
    int dev,
    int logical_pipe,
    int aid,
    uint16_t start,
    uint16_t size,
    bool set) {
  LOG_TRACE(
      "PktGenAppCfg: dev %d pipe %d app %d shdl %d, Payload offset/size %#x "
      "%#x, extract from recir %s",
      dev,
      logical_pipe,
      aid,
      sid,
      start,
      size,
      (set ? "yes" : "no"));
  bf_dev_target_t dev_tgt = {dev, logical_pipe};
  int i, pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t *data = NULL;
  uint32_t addr = offsetof(
      tof3_reg, pipes[0].pardereg.pgstnreg.pgrreg.pgr_app[aid].payload_ctrl);
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1u << i))) continue;
    data = &pipe_mgr_tof3_get_pgr_ctx(dev)->app.a[i][aid].payload_ctrl;
    if (set) {
      setp_tof3_pgr_app_payload_ctrl_app_recirc_extract(data, 1);
      setp_tof3_pgr_app_payload_ctrl_app_payload_size(data, 0);
      setp_tof3_pgr_app_payload_ctrl_app_payload_addr(data, 0);
    } else {
      setp_tof3_pgr_app_payload_ctrl_app_recirc_extract(data, 0);
      setp_tof3_pgr_app_payload_ctrl_app_payload_size(data, size);
      setp_tof3_pgr_app_payload_ctrl_app_payload_addr(data, start);
    }
  }
  if (data == NULL) return BF_INVALID_ARG;
  pipe_mgr_tof3_pg_txn_bkup_app_cfg(sid, dev);
  return pg_write_one_pipe_reg(sid, dev, pipe_mask, addr, *data);
}

static bf_status_t pipe_mgr_tof3_pktgen_reg_pgr_app_ing_port_ctrl(
    pipe_sess_hdl_t sid,
    int dev,
    int logical_pipe,
    int aid,
    uint16_t port,
    bool inc,
    uint8_t wrap) {
  LOG_TRACE(
      "PktGenAppCfg: dev %d pipe %d app %d shdl %d Port %d inc %d wrap_max %d",
      dev,
      logical_pipe,
      aid,
      sid,
      port,
      inc,
      wrap);
  bf_dev_target_t dev_tgt = {dev, logical_pipe};
  int i, pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t *data = NULL, addr;
  addr = offsetof(
      tof3_reg, pipes[0].pardereg.pgstnreg.pgrreg.pgr_app[aid].ingr_port_ctrl);
  pipe_mgr_tof3_pg_txn_bkup_app_cfg(sid, dev);
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1u << i))) continue;
    data = &pipe_mgr_tof3_get_pgr_ctx(dev)->app.a[i][aid].ing_port;
    setp_tof3_pgr_app_ingr_port_ctrl_app_ingr_port_pipe_id(data, i);
    setp_tof3_pgr_app_ingr_port_ctrl_app_ingr_port(data, port);
    if (inc) {
      setp_tof3_pgr_app_ingr_port_ctrl_app_ingr_port_wrap(data, wrap);
      setp_tof3_pgr_app_ingr_port_ctrl_app_ingr_port_inc(data, 1);
    } else {
      setp_tof3_pgr_app_ingr_port_ctrl_app_ingr_port_wrap(data, 0);
      setp_tof3_pgr_app_ingr_port_ctrl_app_ingr_port_inc(data, 0);
    }
    bf_status_t sts = pg_write_one_pipe_reg(sid, dev, 1 << i, addr, *data);
    if (BF_SUCCESS != sts) return sts;
  }
  return BF_SUCCESS;
}

static bf_status_t pipe_mgr_tof3_pktgen_reg_pgr_app_recir_match_value(
    pipe_sess_hdl_t sid,
    int dev,
    int logical_pipe,
    int aid,
    uint8_t *key,
    bool set) {
  LOG_TRACE(
      "PktGenAppCfg: dev %d pipe %d app %d shdl %d, Recirc Key %#x %#x %#x "
      "%#x %#x %#x %#x %#x %#x %#x %#x %#x %#x %#x %#x %#x ",
      dev,
      logical_pipe,
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
  bf_dev_target_t dev_tgt = {dev, logical_pipe};
  struct pipe_mgr_tof3_pg_dev_ctx *ctx = pipe_mgr_tof3_get_pgr_ctx(dev);
  int i, pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t *data = NULL, addr;
  bf_status_t sts = BF_SUCCESS;
  addr = offsetof(tof3_reg,
                  pipes[0]
                      .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                      .recir_match_value.recir_match_value_0_4);
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1u << i))) continue;
    data = &ctx->app.a[i][aid].recir_val[0];
    *data = 0;
    if (set)
      *data = (key[12] << 24) | (key[13] << 16) | (key[14] << 8) | key[15];
  }
  if (data == NULL) return BF_INVALID_ARG;
  sts |= pg_write_one_pipe_reg(sid, dev, pipe_mask, addr, *data);

  addr = offsetof(tof3_reg,
                  pipes[0]
                      .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                      .recir_match_value.recir_match_value_1_4);
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1u << i))) continue;
    data = &ctx->app.a[i][aid].recir_val[1];
    *data = 0;
    if (set) *data = (key[8] << 24) | (key[9] << 16) | (key[10] << 8) | key[11];
  }
  sts |= pg_write_one_pipe_reg(sid, dev, pipe_mask, addr, *data);

  addr = offsetof(tof3_reg,
                  pipes[0]
                      .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                      .recir_match_value.recir_match_value_2_4);
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1u << i))) continue;
    data = &ctx->app.a[i][aid].recir_val[2];
    *data = 0;
    if (set) *data = (key[4] << 24) | (key[5] << 16) | (key[6] << 8) | key[7];
  }
  sts |= pg_write_one_pipe_reg(sid, dev, pipe_mask, addr, *data);

  addr = offsetof(tof3_reg,
                  pipes[0]
                      .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                      .recir_match_value.recir_match_value_3_4);
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1u << i))) continue;
    data = &ctx->app.a[i][aid].recir_val[3];
    *data = 0;
    if (set) *data = (key[0] << 24) | (key[1] << 16) | (key[2] << 8) | key[3];
  }
  sts |= pg_write_one_pipe_reg(sid, dev, pipe_mask, addr, *data);

  pipe_mgr_tof3_pg_txn_bkup_app_cfg(sid, dev);
  return sts;
}

static bf_status_t pipe_mgr_tof3_pktgen_reg_pgr_app_recir_match_mask(
    pipe_sess_hdl_t sid,
    int dev,
    int logical_pipe,
    int aid,
    uint8_t *mask,
    bool set) {
  LOG_TRACE(
      "PktGenAppCfg: dev %d pipe %d app %d shdl %d, Recirc mask %#x %#x %#x "
      "%#x %#x %#x %#x %#x %#x %#x %#x %#x %#x %#x %#x %#x ",
      dev,
      logical_pipe,
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
  bf_dev_target_t dev_tgt = {dev, logical_pipe};
  struct pipe_mgr_tof3_pg_dev_ctx *ctx = pipe_mgr_tof3_get_pgr_ctx(dev);
  int i, pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t *data = NULL, addr;
  bf_status_t sts = BF_SUCCESS;
  addr = offsetof(tof3_reg,
                  pipes[0]
                      .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                      .recir_match_mask.recir_match_mask_0_4);
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1u << i))) continue;
    data = &ctx->app.a[i][aid].recir_msk[0];
    *data = 0;
    if (set)
      *data =
          ~((mask[12] << 24) | (mask[13] << 16) | (mask[14] << 8) | mask[15]);
  }
  if (data == NULL) return BF_INVALID_ARG;
  sts |= pg_write_one_pipe_reg(sid, dev, pipe_mask, addr, *data);

  addr = offsetof(tof3_reg,
                  pipes[0]
                      .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                      .recir_match_mask.recir_match_mask_1_4);
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1u << i))) continue;
    data = &ctx->app.a[i][aid].recir_msk[1];
    *data = 0;
    if (set)
      *data = ~((mask[8] << 24) | (mask[9] << 16) | (mask[10] << 8) | mask[11]);
  }
  sts |= pg_write_one_pipe_reg(sid, dev, pipe_mask, addr, *data);

  addr = offsetof(tof3_reg,
                  pipes[0]
                      .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                      .recir_match_mask.recir_match_mask_2_4);
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1u << i))) continue;
    data = &ctx->app.a[i][aid].recir_msk[2];
    *data = 0;
    if (set)
      *data = ~((mask[4] << 24) | (mask[5] << 16) | (mask[6] << 8) | mask[7]);
  }
  sts |= pg_write_one_pipe_reg(sid, dev, pipe_mask, addr, *data);

  addr = offsetof(tof3_reg,
                  pipes[0]
                      .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                      .recir_match_mask.recir_match_mask_3_4);
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1u << i))) continue;
    data = &ctx->app.a[i][aid].recir_msk[3];
    *data = 0;
    if (set)
      *data = ~((mask[0] << 24) | (mask[1] << 16) | (mask[2] << 8) | mask[3]);
  }
  sts |= pg_write_one_pipe_reg(sid, dev, pipe_mask, addr, *data);

  pipe_mgr_tof3_pg_txn_bkup_app_cfg(sid, dev);
  return sts;
}

static bf_status_t pipe_mgr_tof3_pktgen_reg_pgr_app_event_number(
    pipe_sess_hdl_t sid,
    int dev,
    int logical_pipe,
    int aid,
    bf_pktgen_trigger_type_e trigger_type,
    uint16_t pkt_num,
    uint16_t batch_num) {
  LOG_TRACE("PktGenAppCfg: dev %d pipe %d app %d shdl %d, Batch %d Pkt %d",
            dev,
            logical_pipe,
            aid,
            sid,
            batch_num,
            pkt_num);
  if (trigger_type == BF_PKTGEN_TRIGGER_PORT_DOWN) {
    if (batch_num) {
      LOG_ERROR(
          "PktGenAppCfg: dev %d app %d batch %d, must use batch_count of zero "
          "for port down triggers",
          dev,
          aid,
          batch_num);
      return BF_INVALID_ARG;
    }
  } else if (trigger_type == BF_PKTGEN_TRIGGER_PFC) {
    if (batch_num || pkt_num) {
      LOG_ERROR(
          "PktGenAppCfg: dev %d app %d batch %d packet %d, must use "
          "batch_count and packet_count of zero for PFC triggers",
          dev,
          aid,
          batch_num,
          pkt_num);
      return BF_INVALID_ARG;
    }
  }
  bf_dev_target_t dev_tgt = {dev, logical_pipe};
  int i, pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t *data = NULL, addr;
  addr = offsetof(tof3_reg,
                  pipes[0].pardereg.pgstnreg.pgrreg.pgr_app[aid].event_number);
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1u << i))) continue;
    data = &pipe_mgr_tof3_get_pgr_ctx(dev)->app.a[i][aid].event_num;
    setp_tof3_pgr_app_event_number_packet_num(data, pkt_num);
    setp_tof3_pgr_app_event_number_batch_num(data, batch_num);
  }
  if (data == NULL) return BF_INVALID_ARG;
  pipe_mgr_tof3_pg_txn_bkup_app_cfg(sid, dev);
  return pg_write_one_pipe_reg(sid, dev, pipe_mask, addr, *data);
}

static bf_status_t pipe_mgr_tof3_pktgen_reg_pgr_app_ibg_base(
    pipe_sess_hdl_t sid, int dev, int logical_pipe, int aid, uint32_t ibg) {
  LOG_TRACE("PktGenAppCfg: dev %d pipe %d app %d shdl %d, IBG %d",
            dev,
            logical_pipe,
            aid,
            sid,
            ibg);
  bf_dev_target_t dev_tgt = {dev, logical_pipe};
  int i, pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t *data = NULL, addr;
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1u << i))) continue;
    data = &pipe_mgr_tof3_get_pgr_ctx(dev)->app.a[i][aid].ibg;
    setp_tof3_pgr_app_event_base_jitter_value_value(data, ibg);
  }
  if (data == NULL) return BF_INVALID_ARG;
  pipe_mgr_tof3_pg_txn_bkup_app_cfg(sid, dev);
  addr = offsetof(tof3_reg,
                  pipes[0]
                      .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                      .event_ibg_jitter_base_value);
  return pg_write_one_pipe_reg(sid, dev, pipe_mask, addr, *data);
}

static void pipe_mgr_tof3_pktgen_jitter_cal(uint32_t jval,
                                            uint8_t *scale,
                                            uint8_t *max) {
  if (jval == 0) {
    *scale = 0;
    *max = 0;
    return;
  }
  int i, j;
  for (i = 31; i >= 0; i--) {
    if ((jval >> i) != 0) break;
  }
  if (i > 7) {
    *max = (jval >> (i - 7)) & 0xff;
    *scale = (i - 7);
  } else {
    *max = 0;
    *scale = 0;
    for (j = 0; j <= i; j++) {
      *max |= (((jval >> j) & 0x1) << j);
    }
  }
}

static bf_status_t pipe_mgr_tof3_pktgen_reg_pgr_app_ibg_jitter(
    pipe_sess_hdl_t sid, int dev, int logical_pipe, int aid, uint32_t jval) {
  LOG_TRACE("PktGenAppCfg: dev %d pipe %d app %d shdl %d, IBG-Jitter %#x",
            dev,
            logical_pipe,
            aid,
            sid,
            jval);
  bf_dev_target_t dev_tgt = {dev, logical_pipe};
  struct pipe_mgr_tof3_pg_dev_ctx *ctx = pipe_mgr_tof3_get_pgr_ctx(dev);
  int i, pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t *data = NULL, addr;
  uint8_t scale, max;
  bf_status_t sts = BF_SUCCESS;
  pipe_mgr_tof3_pktgen_jitter_cal(jval, &scale, &max);
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1u << i))) continue;
    data = &ctx->app.a[i][aid].ibg_jit_max;
    *data = max;
  }
  if (data == NULL) return BF_INVALID_ARG;
  addr = offsetof(
      tof3_reg,
      pipes[0].pardereg.pgstnreg.pgrreg.pgr_app[aid].event_max_ibg_jitter);
  sts |= pg_write_one_pipe_reg(sid, dev, pipe_mask, addr, *data);

  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1u << i))) continue;
    data = &ctx->app.a[i][aid].ibg_jit_scale;
    *data = scale;
  }
  addr = offsetof(
      tof3_reg,
      pipes[0].pardereg.pgstnreg.pgrreg.pgr_app[aid].event_ibg_jitter_scale);
  sts |= pg_write_one_pipe_reg(sid, dev, pipe_mask, addr, *data);

  pipe_mgr_tof3_pg_txn_bkup_app_cfg(sid, dev);
  return sts;
}

static bf_status_t pipe_mgr_tof3_pktgen_reg_pgr_app_ipg_base(
    pipe_sess_hdl_t sid, int dev, int logical_pipe, int aid, uint32_t ipg) {
  LOG_TRACE("PktGenAppCfg: dev %d pipe %d app %d shdl %d, IPG %d",
            dev,
            logical_pipe,
            aid,
            sid,
            ipg);
  bf_dev_target_t dev_tgt = {dev, logical_pipe};
  int i, pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t *data = NULL, addr;
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1u << i))) continue;
    data = &pipe_mgr_tof3_get_pgr_ctx(dev)->app.a[i][aid].ipg;
    setp_tof3_pgr_app_event_base_jitter_value_value(data, ipg);
  }
  if (data == NULL) return BF_INVALID_ARG;
  pipe_mgr_tof3_pg_txn_bkup_app_cfg(sid, dev);
  addr = offsetof(tof3_reg,
                  pipes[0]
                      .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                      .event_ipg_jitter_base_value);
  return pg_write_one_pipe_reg(sid, dev, pipe_mask, addr, *data);
}

static bf_status_t pipe_mgr_tof3_pktgen_reg_pgr_app_ipg_jitter(
    pipe_sess_hdl_t sid, int dev, int logical_pipe, int aid, uint32_t jval) {
  LOG_TRACE("PktGenAppCfg: dev %d pipe %d app %d shdl %d, IPG-Jitter %#x",
            dev,
            logical_pipe,
            aid,
            sid,
            jval);

  bf_dev_target_t dev_tgt = {dev, logical_pipe};
  struct pipe_mgr_tof3_pg_dev_ctx *ctx = pipe_mgr_tof3_get_pgr_ctx(dev);
  int i, pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t *data = NULL, addr;
  uint8_t scale, max;
  bf_status_t sts = BF_SUCCESS;
  pipe_mgr_tof3_pktgen_jitter_cal(jval, &scale, &max);
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1u << i))) continue;
    data = &ctx->app.a[i][aid].ipg_jit_max;
    *data = max;
  }
  if (data == NULL) return BF_INVALID_ARG;
  addr = offsetof(
      tof3_reg,
      pipes[0].pardereg.pgstnreg.pgrreg.pgr_app[aid].event_max_ipg_jitter);
  sts |= pg_write_one_pipe_reg(sid, dev, pipe_mask, addr, *data);

  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1u << i))) continue;
    data = &ctx->app.a[i][aid].ipg_jit_scale;
    *data = scale;
  }
  addr = offsetof(
      tof3_reg,
      pipes[0].pardereg.pgstnreg.pgrreg.pgr_app[aid].event_ipg_jitter_scale);
  sts |= pg_write_one_pipe_reg(sid, dev, pipe_mask, addr, *data);

  pipe_mgr_tof3_pg_txn_bkup_app_cfg(sid, dev);
  return sts;
}

static bf_status_t pipe_mgr_tof3_pktgen_reg_pgr_app_event_timer(
    pipe_sess_hdl_t sid, int dev, int logical_pipe, int aid, uint32_t tval) {
  LOG_TRACE("PktGenAppCfg: dev %d pipe %d app %d shdl %d, Timer %d",
            dev,
            logical_pipe,
            aid,
            sid,
            tval);
  bf_dev_target_t dev_tgt = {dev, logical_pipe};
  int i, pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t *data = NULL, addr;
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1u << i))) continue;
    data = &pipe_mgr_tof3_get_pgr_ctx(dev)->app.a[i][aid].event_timer;
    setp_tof3_pgr_app_event_timer_timer_count(data, tval);
  }
  if (data == NULL) return BF_INVALID_ARG;
  pipe_mgr_tof3_pg_txn_bkup_app_cfg(sid, dev);
  addr = offsetof(tof3_reg,
                  pipes[0].pardereg.pgstnreg.pgrreg.pgr_app[aid].event_timer);
  return pg_write_one_pipe_reg(sid, dev, pipe_mask, addr, *data);
}

static bf_status_t pipe_mgr_tof3_pktgen_reg_pgr_app_recirc_src(
    pipe_sess_hdl_t sid, int dev, int logical_pipe, int aid, int src_port) {
  LOG_TRACE("PktGenAppCfg: dev %d pipe %d app %d shdl %d, recirc src chnl %d",
            dev,
            logical_pipe,
            aid,
            sid,
            src_port / 2);
  bf_dev_target_t dev_tgt = {dev, logical_pipe};
  int i, pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t *data, addr;
  addr =
      offsetof(tof3_reg,
               pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.cfg_app_recirc_src);
  for (i = 0; i < BF_PIPE_COUNT; ++i) {
    if (!(pipe_mask & (1u << i))) continue;
    data = &(pipe_mgr_tof3_get_pgr_ctx(dev)->app_recirc_src[i]);
    (*data) |= (0x3 << (aid * 2));
    (*data) &= (((src_port / 2) & 0x3) << (aid * 2));
    pg_write_one_pipe_reg(sid, dev, 1u << i, addr, *data);
  }
  return BF_SUCCESS;
}

static bf_status_t pipe_mgr_tof3_pktgen_reg_pfc(
    pipe_sess_hdl_t sid,
    int dev,
    int logical_pipe,
    int aid,
    struct bf_tof2_pktgen_pfc_trigger pfc_cfg) {
  LOG_TRACE(
      "PktGenAppCfg: dev %d pipe %d app %d shdl %d, pfc_hdr 0x%02x %02x %02x "
      "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x timer "
      "%s: %d, max_pfc_pkt_size %d x 8 bytes",
      dev,
      logical_pipe,
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

  bf_dev_target_t dev_tgt = {dev, logical_pipe};
  struct pipe_mgr_tof3_pg_dev_ctx *ctx = pipe_mgr_tof3_get_pgr_ctx(dev);
  int pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t addr, data;
  bf_status_t sts = BF_SUCCESS;
  uint32_t base_addr = offsetof(
      tof3_reg,
      pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.cfg_pfc_hdr.cfg_pfc_hdr_0_4);

  for (int a = 0; a < BF_PIPE_COUNT; ++a) {
    if (!(pipe_mask & (1u << a))) continue;

    for (int i = 0; i < 4; ++i) {
      addr = base_addr + i * 4;
      data = 0;
      for (int j = 0; j < 4; ++j)
        data = (data | pfc_cfg.pfc_hdr[4 * i + j]) << 8;
      ctx->pfc[a].pfc_hdr[i] = data;
      sts |= pg_write_one_pipe_reg(sid, dev, pipe_mask, addr, data);
    }

    addr = offsetof(tof3_reg,
                    pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.cfg_pfc_timer);
    data = 0;
    if (pfc_cfg.cfg_timer_en) {
      data = pfc_cfg.cfg_timer | (1u << 16);
    }
    ctx->pfc[a].pfc_timer = data;
    sts |= pg_write_one_pipe_reg(sid, dev, pipe_mask, addr, data);

    addr = offsetof(
        tof3_reg,
        pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.cfg_pfc_max_pkt_size);
    ctx->pfc[a].pfc_max_pkt_size = pfc_cfg.pfc_max_msgs & 0x7ff;
    sts |= pg_write_one_pipe_reg(
        sid, dev, pipe_mask, addr, pfc_cfg.pfc_max_msgs & 0x7ff);
  }
  return sts;
}

/* Tofino3
 * configure application
 */
bf_status_t pipe_mgr_tof3_pktgen_cfg_app(bf_session_hdl_t shdl,
                                         bf_dev_target_t dev_tgt,
                                         int app_id,
                                         bf_pktgen_app_cfg_t *cfg) {
  /* Everything is validated.  Go ahead and update the app. */
  bf_status_t sts;
  int dev = dev_tgt.device_id;
  int pipe = dev_tgt.dev_pipe_id;
  // ctrl
  sts = pipe_mgr_tof3_pktgen_reg_pgr_app_ctrl(shdl,
                                              dev,
                                              pipe,
                                              app_id,
                                              cfg->tof2.assigned_chnl_id,
                                              cfg->u.port_mask_sel_tof2,
                                              cfg->trigger_type);
  if (sts != BF_SUCCESS) return sts;
  // payload ctrl
  sts = pipe_mgr_tof3_pktgen_reg_pgr_app_payload_ctrl(
      shdl,
      dev,
      pipe,
      app_id,
      cfg->pkt_buffer_offset / 16,
      cfg->length,
      cfg->tof2.offset_len_from_recir_pkt);
  if (sts != BF_SUCCESS) return sts;
  // ing_port ctrl
  sts = pipe_mgr_tof3_pktgen_reg_pgr_app_ing_port_ctrl(
      shdl,
      dev,
      pipe,
      app_id,
      cfg->pipe_local_source_port,
      cfg->increment_source_port,
      cfg->tof2.source_port_wrap_max);
  if (sts != BF_SUCCESS) return sts;
  if ((BF_PKTGEN_TRIGGER_RECIRC_PATTERN == cfg->trigger_type) ||
      (BF_PKTGEN_TRIGGER_DPRSR == cfg->trigger_type)) {
    // key and mask
    sts = pipe_mgr_tof3_pktgen_reg_pgr_app_recir_match_value(
        shdl, dev, pipe, app_id, cfg->u.pattern_tof2.value, true);
    if (sts != BF_SUCCESS) return sts;
    sts = pipe_mgr_tof3_pktgen_reg_pgr_app_recir_match_mask(
        shdl, dev, pipe, app_id, cfg->u.pattern_tof2.mask, true);
    if (sts != BF_SUCCESS) return sts;
    // recirc src en
    sts = pipe_mgr_tof3_pktgen_reg_pgr_app_recirc_src(
        shdl, dev, pipe, app_id, cfg->tof2.assigned_chnl_id);
    if (sts != BF_SUCCESS) return sts;
  } else {
    sts = pipe_mgr_tof3_pktgen_reg_pgr_app_recir_match_value(
        shdl, dev, pipe, app_id, cfg->u.pattern_tof2.value, false);
    if (sts != BF_SUCCESS) return sts;
    sts = pipe_mgr_tof3_pktgen_reg_pgr_app_recir_match_mask(
        shdl, dev, pipe, app_id, cfg->u.pattern_tof2.mask, false);
    if (sts != BF_SUCCESS) return sts;
    sts =
        pipe_mgr_tof3_pktgen_reg_pgr_app_recirc_src(shdl, dev, pipe, app_id, 0);
    if (sts != BF_SUCCESS) return sts;
  }
  // event number
  sts = pipe_mgr_tof3_pktgen_reg_pgr_app_event_number(shdl,
                                                      dev,
                                                      pipe,
                                                      app_id,
                                                      cfg->trigger_type,
                                                      cfg->packets_per_batch,
                                                      cfg->batch_count);
  if (sts != BF_SUCCESS) return sts;
  // ibg
  sts = pipe_mgr_tof3_pktgen_reg_pgr_app_ibg_base(
      shdl, dev, pipe, app_id, pipe_mgr_nsec_to_clock(dev, cfg->ibg));
  if (sts != BF_SUCCESS) return sts;
  // ibg jitter
  sts = pipe_mgr_tof3_pktgen_reg_pgr_app_ibg_jitter(
      shdl, dev, pipe, app_id, pipe_mgr_nsec_to_clock(dev, cfg->ibg_jitter));
  if (sts != BF_SUCCESS) return sts;
  // ipg
  sts = pipe_mgr_tof3_pktgen_reg_pgr_app_ipg_base(
      shdl, dev, pipe, app_id, pipe_mgr_nsec_to_clock(dev, cfg->ipg));
  if (sts != BF_SUCCESS) return sts;
  // ipg jitter
  sts = pipe_mgr_tof3_pktgen_reg_pgr_app_ipg_jitter(
      shdl, dev, pipe, app_id, pipe_mgr_nsec_to_clock(dev, cfg->ipg_jitter));
  if (sts != BF_SUCCESS) return sts;
  // event timer
  if (BF_PKTGEN_TRIGGER_TIMER_ONE_SHOT == cfg->trigger_type ||
      BF_PKTGEN_TRIGGER_TIMER_PERIODIC == cfg->trigger_type) {
    sts = pipe_mgr_tof3_pktgen_reg_pgr_app_event_timer(
        shdl,
        dev,
        pipe,
        app_id,
        pipe_mgr_nsec_to_clock(dev, cfg->u.timer_nanosec));
    if (sts != BF_SUCCESS) return sts;
  } else {
    sts = pipe_mgr_tof3_pktgen_reg_pgr_app_event_timer(
        shdl, dev, pipe, app_id, 0);
    if (sts != BF_SUCCESS) return sts;
  }
  // pfc
  if (BF_PKTGEN_TRIGGER_PFC == cfg->trigger_type) {
    sts = pipe_mgr_tof3_pktgen_reg_pfc(shdl, dev, pipe, app_id, cfg->u.pfc_cfg);
    if (sts != BF_SUCCESS) return sts;
  }
  return BF_SUCCESS;
}

uint32_t pipe_mgr_tof3_pktgen_jitter_recal(uint8_t scale, uint8_t max) {
  if (scale == 0) {
    return max;
  } else {
    int i = scale + 7;
    return (max << (i - 7));
  }
}

bf_status_t pipe_mgr_tof3_pktgen_cfg_app_get(bf_dev_target_t dev_tgt,
                                             int app_id,
                                             bf_pktgen_app_cfg_t *cfg) {
  int dev = dev_tgt.device_id;
  struct pipe_mgr_tof3_pg_dev_ctx *ctx = pipe_mgr_tof3_get_pgr_ctx(dev);
  int pipe = dev_tgt.dev_pipe_id;
  int i = (pipe == BF_DEV_PIPE_ALL) ? 0 : pipe;
  uint32_t data, data_tmp;
  int j;
  // trigger type
  data = ctx->app.a[i][app_id].app_ctrl;
  cfg->trigger_type = getp_tof3_pgr_app_ctrl_app_type(&data);
  cfg->tof2.assigned_chnl_id = getp_tof3_pgr_app_ctrl_app_chnl(&data);
  if (cfg->trigger_type == BF_PKTGEN_TRIGGER_PORT_DOWN) {
    cfg->u.port_mask_sel_tof2 =
        getp_tof3_pgr_app_ctrl_app_port_down_mask_sel(&data);
  }
  // key and mask
  if ((BF_PKTGEN_TRIGGER_RECIRC_PATTERN == cfg->trigger_type) ||
      (BF_PKTGEN_TRIGGER_DPRSR == cfg->trigger_type)) {
    for (j = 0; j < 4; j++) {
      int tmp = 4 * j;
      data = ctx->app.a[i][app_id].recir_val[j];
      cfg->u.pattern_tof2.value[15 - tmp] = data & 0xff;
      cfg->u.pattern_tof2.value[14 - tmp] = (data >> 8) & 0xff;
      cfg->u.pattern_tof2.value[13 - tmp] = (data >> 16) & 0xff;
      cfg->u.pattern_tof2.value[12 - tmp] = (data >> 24) & 0xff;

      data = ctx->app.a[i][app_id].recir_msk[j];
      cfg->u.pattern_tof2.mask[15 - tmp] = data & 0xff;
      cfg->u.pattern_tof2.mask[14 - tmp] = (data >> 8) & 0xff;
      cfg->u.pattern_tof2.mask[13 - tmp] = (data >> 16) & 0xff;
      cfg->u.pattern_tof2.mask[12 - tmp] = (data >> 24) & 0xff;
    }
  } else if (BF_PKTGEN_TRIGGER_TIMER_ONE_SHOT == cfg->trigger_type ||
             BF_PKTGEN_TRIGGER_TIMER_PERIODIC == cfg->trigger_type) {
    data = ctx->app.a[i][app_id].event_timer;
    data_tmp = getp_tof3_pgr_app_event_timer_timer_count(&data);
    cfg->u.timer_nanosec = pipe_mgr_clock_to_nsec(dev, data_tmp);
  } else if (BF_PKTGEN_TRIGGER_PFC == cfg->trigger_type) {
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
  }

  // payload ctrl
  data = ctx->app.a[i][app_id].payload_ctrl;
  cfg->pkt_buffer_offset = 0;
  cfg->length = 0;
  cfg->tof2.offset_len_from_recir_pkt =
      getp_tof3_pgr_app_payload_ctrl_app_recirc_extract(&data);
  if (cfg->tof2.offset_len_from_recir_pkt) {
    cfg->pkt_buffer_offset =
        16 * (getp_tof3_pgr_app_payload_ctrl_app_payload_addr(&data));
    cfg->length = getp_tof3_pgr_app_payload_ctrl_app_payload_size(&data);
  }
  // ing_port ctrl
  data = ctx->app.a[i][app_id].ing_port;
  cfg->pipe_local_source_port =
      getp_tof3_pgr_app_ingr_port_ctrl_app_ingr_port(&data);
  cfg->increment_source_port =
      getp_tof3_pgr_app_ingr_port_ctrl_app_ingr_port_inc(&data);
  cfg->tof2.source_port_wrap_max =
      getp_tof3_pgr_app_ingr_port_ctrl_app_ingr_port_wrap(&data);
  // event number
  data = ctx->app.a[i][app_id].event_num;
  cfg->packets_per_batch = getp_tof3_pgr_app_event_number_packet_num(&data);
  cfg->batch_count = getp_tof3_pgr_app_event_number_batch_num(&data);
  // ibg
  data = ctx->app.a[i][app_id].ibg;
  data_tmp = getp_tof3_pgr_app_event_base_jitter_value_value(&data);
  cfg->ibg = pipe_mgr_clock_to_nsec(dev, data_tmp);
  // ibg jitter
  uint8_t max = ctx->app.a[i][app_id].ibg_jit_max;
  uint8_t scale = ctx->app.a[i][app_id].ibg_jit_scale;
  data_tmp = pipe_mgr_tof3_pktgen_jitter_recal(scale, max);
  cfg->ibg_jitter = pipe_mgr_clock_to_nsec(dev, data_tmp);
  // ipg
  data = ctx->app.a[i][app_id].ipg;
  data_tmp = getp_tof3_pgr_app_event_base_jitter_value_value(&data);
  cfg->ipg = pipe_mgr_clock_to_nsec(dev, data_tmp);
  // ipg jitter
  max = ctx->app.a[i][app_id].ipg_jit_max;
  scale = ctx->app.a[i][app_id].ipg_jit_scale;
  data_tmp = pipe_mgr_tof3_pktgen_jitter_recal(scale, max);
  cfg->ipg_jitter = pipe_mgr_clock_to_nsec(dev, data_tmp);
  return BF_SUCCESS;
}
/* Tofino3
 * configure port down mask
 */
static bf_status_t tof3_set_port_down_mask_hw(bf_session_hdl_t shdl,
                                              bf_dev_id_t dev_id,
                                              bf_dev_pipe_t log_pipe,
                                              int mask_sel,
                                              uint8_t *mask) {
  bf_status_t sts = BF_SUCCESS;
  struct pipe_mgr_tof3_pg_dev_ctx *ctx = pipe_mgr_tof3_get_pgr_ctx(dev_id);
  /* Grab a pointer to the port based mask. */
  uint8_t *m_mask = ctx->port_down_mask[log_pipe][2].port_mask;

  /* Mask the user provided mask with the port based mask before programming .*/
  uint32_t d0 = mask[3] & m_mask[3];
  d0 = (d0 << 8) | (mask[2] & m_mask[2]);
  d0 = (d0 << 8) | (mask[1] & m_mask[1]);
  if (log_pipe != 0) {
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
      tof3_reg,
      pipes[0]
          .pardereg.pgstnreg.pgrreg.pgr_common.pgen_port_down_mask[mask_sel]
          .pgen_port_down_mask_0_3);
  ctx->port_down_mask[log_pipe][mask_sel].val_0 = d0;
  ctx->port_down_mask[log_pipe][mask_sel].val_1 = d1;
  ctx->port_down_mask[log_pipe][mask_sel].val_2 = d2;

  sts = pg_write_one_pipe_reg(shdl, dev_id, 1 << log_pipe, addr, d0);
  if (BF_SUCCESS != sts) return sts;
  sts = pg_write_one_pipe_reg(shdl, dev_id, 1 << log_pipe, addr + 4, d1);
  if (BF_SUCCESS != sts) return sts;
  sts = pg_write_one_pipe_reg(shdl, dev_id, 1 << log_pipe, addr + 8, d2);
  return sts;
}
bf_status_t pipe_mgr_tof3_pktgen_cfg_port_down_mask(
    bf_session_hdl_t shdl,
    bf_dev_target_t dev_tgt,
    uint32_t port_mask_sel,
    struct bf_tof2_port_down_sel *msk) {
  if (!msk) return BF_INVALID_ARG;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_tgt.device_id);
  if (!dev_info) return BF_INVALID_ARG;

  uint8_t *mask = msk->port_mask;

  int pipe_mask = pg_log_pipe_mask(dev_tgt);
  bf_status_t sts = BF_SUCCESS;
  for (unsigned int i = 0; BF_SUCCESS == sts && i < dev_info->num_active_pipes;
       ++i) {
    if (pipe_mask & (1u << i))
      sts = tof3_set_port_down_mask_hw(
          shdl, dev_tgt.device_id, i, port_mask_sel, mask);
  }

  /* If the update was successful update our shadow. */
  if (BF_SUCCESS == sts) {
    if (dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL) {
      for (unsigned int i = 0; i < dev_info->num_active_pipes; ++i) {
        PIPE_MGR_MEMCPY(pipe_mgr_pktgen_ctx(dev_tgt.device_id)
                            ->u.tof3_ctx->port_down_mask[i][port_mask_sel]
                            .port_mask,
                        mask,
                        sizeof pipe_mgr_pktgen_ctx(dev_tgt.device_id)
                            ->u.tof3_ctx->port_down_mask[i][port_mask_sel]
                            .port_mask);
      }
    } else {
      pipe_mgr_pktgen_ctx(dev_tgt.device_id)
          ->u.tof3_ctx->port_down_mask[dev_tgt.dev_pipe_id][port_mask_sel] =
          *msk;
    }
  }

  return sts;
}

bf_status_t pipe_mgr_tof3_pktgen_cfg_port_down_mask_get(
    bf_dev_target_t dev_tgt,
    uint32_t port_mask_sel,
    struct bf_tof2_port_down_sel *msk) {
  if (!msk) return BF_INVALID_ARG;
  struct bf_tof2_port_down_sel *mask_tmp =
      &pipe_mgr_pktgen_ctx(dev_tgt.device_id)
           ->u.tof3_ctx
           ->port_down_mask[dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL
                                ? 0
                                : dev_tgt.dev_pipe_id][port_mask_sel];
  PIPE_MGR_MEMCPY(msk, mask_tmp, sizeof(struct bf_tof2_port_down_sel));
  return BF_SUCCESS;
}

bf_status_t pipe_mgr_tof3_pktgen_cfg_port_down_replay(
    bf_session_hdl_t shdl,
    bf_dev_target_t dev_tgt,
    bf_pktgen_port_down_mode_t mode) {
  int pipe_mask = pg_log_pipe_mask(dev_tgt);
  uint32_t addr = offsetof(
      tof3_reg,
      pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.pgen_retrigger_port_down);
  uint32_t data;
  switch (mode) {
    case BF_PKTGEN_PORT_DOWN_REPLAY_NONE:
      data = 0;
      break;
    case BF_PKTGEN_PORT_DOWN_REPLAY_ALL:
      data = 0;
      setp_tof3_pgr_pgen_retrigger_port_down_en(&data, 1);
      setp_tof3_pgr_pgen_retrigger_port_down_all_down_port(&data, 1);
      break;
    case BF_PKTGEN_PORT_DOWN_REPLAY_MISSED:
      data = 0;
      setp_tof3_pgr_pgen_retrigger_port_down_en(&data, 1);
      setp_tof3_pgr_pgen_retrigger_port_down_all_down_port(&data, 0);
      break;
    default:
      return BF_INVALID_ARG;
  }
  return pg_write_one_pipe_reg(shdl, dev_tgt.device_id, pipe_mask, addr, data);
}

bf_status_t pipe_mgr_tof3_pktgen_get_port_down_replay(
    bf_session_hdl_t shdl,
    rmt_dev_info_t *dev_info,
    bf_dev_target_t dev_tgt,
    bf_pktgen_port_down_mode_t *mode) {
  (void)shdl;
  bf_dev_id_t dev_id = dev_info->dev_id;
  uint32_t addr = offsetof(
      tof3_reg,
      pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.pgen_retrigger_port_down);
  uint32_t data = 0;

  if (dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL) {
    uint32_t pipe_data;
    for (unsigned int log_pipe = 0; log_pipe < dev_info->num_active_pipes;
         ++log_pipe) {
      pg_tof3_read_one_pipe_reg(dev_id, log_pipe, addr, &pipe_data);
      if (!log_pipe) {
        data = pipe_data;
      } else {
        if (pipe_data != data) {
          LOG_ERROR(
              "Dev %d: Pktgen port down replay mode not the same on all pipes, "
              "cannot query with PIPE-ALL",
              dev_id);
          return BF_INVALID_ARG;
        }
      }
    }
  } else {
    pg_tof3_read_one_pipe_reg(dev_id, dev_tgt.dev_pipe_id, addr, &data);
  }

  int en = getp_tof3_pgr_pgen_retrigger_port_down_en(&data);
  int all = getp_tof3_pgr_pgen_retrigger_port_down_all_down_port(&data);
  if (en == 0) {
    *mode = BF_PKTGEN_PORT_DOWN_REPLAY_NONE;
  } else if (all != 0) {
    *mode = BF_PKTGEN_PORT_DOWN_REPLAY_ALL;
  } else {
    *mode = BF_PKTGEN_PORT_DOWN_REPLAY_MISSED;
  }
  return BF_SUCCESS;
}

bf_status_t pipe_mgr_tof3_pktgen_app_cfg_download(bf_session_hdl_t shdl,
                                                  int dev) {
  uint32_t i = 0, aid = 0;
  uint32_t addr = 0, val = 0;
  uint32_t pipe_cnt = pipe_mgr_get_num_active_pipes(dev);
  int pipe_mask = 0;
  struct pipe_mgr_tof3_pg_dev_ctx *ctx = pipe_mgr_tof3_get_pgr_ctx(dev);
  bf_status_t sts = BF_SUCCESS;

  for (i = 0; i < pipe_cnt; ++i) {
    bf_dev_target_t dev_tgt;
    dev_tgt.device_id = dev;
    dev_tgt.dev_pipe_id = i;
    pipe_mask = pg_log_pipe_mask(dev_tgt);
    for (aid = 0; aid < PIPE_MGR_TOF3_PKTGEN_APP_CNT; aid++) {
      // payload ctrl
      addr =
          offsetof(tof3_reg,
                   pipes[0].pardereg.pgstnreg.pgrreg.pgr_app[aid].payload_ctrl);
      val = ctx->app.a[i][aid].payload_ctrl;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      // ing_port ctrl
      addr = offsetof(
          tof3_reg,
          pipes[0].pardereg.pgstnreg.pgrreg.pgr_app[aid].ingr_port_ctrl);
      val = ctx->app.a[i][aid].ing_port;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      // key
      addr = offsetof(tof3_reg,
                      pipes[0]
                          .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                          .recir_match_value.recir_match_value_0_4);
      val = ctx->app.a[i][aid].recir_val[0];
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;
      addr = offsetof(tof3_reg,
                      pipes[0]
                          .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                          .recir_match_value.recir_match_value_1_4);
      val = ctx->app.a[i][aid].recir_val[1];
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;
      addr = offsetof(tof3_reg,
                      pipes[0]
                          .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                          .recir_match_value.recir_match_value_2_4);
      val = ctx->app.a[i][aid].recir_val[2];
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;
      addr = offsetof(tof3_reg,
                      pipes[0]
                          .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                          .recir_match_value.recir_match_value_3_4);
      val = ctx->app.a[i][aid].recir_val[3];
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      // mask
      addr = offsetof(tof3_reg,
                      pipes[0]
                          .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                          .recir_match_mask.recir_match_mask_0_4);
      val = ctx->app.a[i][aid].recir_msk[0];
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;
      addr = offsetof(tof3_reg,
                      pipes[0]
                          .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                          .recir_match_mask.recir_match_mask_1_4);
      val = ctx->app.a[i][aid].recir_msk[1];
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;
      addr = offsetof(tof3_reg,
                      pipes[0]
                          .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                          .recir_match_mask.recir_match_mask_2_4);
      val = ctx->app.a[i][aid].recir_msk[2];
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;
      addr = offsetof(tof3_reg,
                      pipes[0]
                          .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                          .recir_match_mask.recir_match_mask_3_4);
      val = ctx->app.a[i][aid].recir_msk[3];
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      // recirc src en
      addr = offsetof(
          tof3_reg,
          pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.cfg_app_recirc_src);
      val = (ctx->app_recirc_src[i]);
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      // event number
      addr =
          offsetof(tof3_reg,
                   pipes[0].pardereg.pgstnreg.pgrreg.pgr_app[aid].event_number);
      val = ctx->app.a[i][aid].event_num;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      // ibg
      addr = offsetof(tof3_reg,
                      pipes[0]
                          .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                          .event_ibg_jitter_base_value);
      val = ctx->app.a[i][aid].ibg;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      // ibg jitter
      addr = offsetof(
          tof3_reg,
          pipes[0].pardereg.pgstnreg.pgrreg.pgr_app[aid].event_max_ibg_jitter);
      val = ctx->app.a[i][aid].ibg_jit_max;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      addr = offsetof(tof3_reg,
                      pipes[0]
                          .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                          .event_ibg_jitter_scale);
      val = ctx->app.a[i][aid].ibg_jit_scale;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      // ipg
      addr = offsetof(tof3_reg,
                      pipes[0]
                          .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                          .event_ipg_jitter_base_value);
      val = ctx->app.a[i][aid].ipg;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      // ipg jitter
      addr = offsetof(
          tof3_reg,
          pipes[0].pardereg.pgstnreg.pgrreg.pgr_app[aid].event_max_ipg_jitter);
      val = ctx->app.a[i][aid].ipg_jit_max;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;
      addr = offsetof(tof3_reg,
                      pipes[0]
                          .pardereg.pgstnreg.pgrreg.pgr_app[aid]
                          .event_ipg_jitter_scale);
      val = ctx->app.a[i][aid].ipg_jit_scale;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      // event timer
      addr = offsetof(
          tof3_reg, pipes[0].pardereg.pgstnreg.pgrreg.pgr_app[aid].event_timer);
      val = ctx->app.a[i][aid].event_timer;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      // pfc
      uint32_t base_addr = offsetof(
          tof3_reg,
          pipes[0]
              .pardereg.pgstnreg.pgrreg.pgr_common.cfg_pfc_hdr.cfg_pfc_hdr_0_4);
      for (int k = 0; k < 4; ++k) {
        addr = base_addr + k * 4;
        val = ctx->pfc[i].pfc_hdr[k];
        sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
        if (sts != BF_SUCCESS) return sts;
      }

      addr = offsetof(
          tof3_reg, pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.cfg_pfc_timer);
      val = ctx->pfc[i].pfc_timer;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      addr = offsetof(
          tof3_reg,
          pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.cfg_pfc_max_pkt_size);
      val = ctx->pfc[i].pfc_max_pkt_size;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;

      // ctrl
      addr = offsetof(tof3_reg,
                      pipes[0].pardereg.pgstnreg.pgrreg.pgr_app[aid].ctrl);
      val = ctx->app.a[i][aid].app_ctrl;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (sts != BF_SUCCESS) return sts;
    }
  }
  return BF_SUCCESS;
}

/* Warm init quick */
bf_status_t pipe_mgr_tof3_pktgen_warm_init_quick(bf_session_hdl_t shdl,
                                                 bf_dev_id_t dev) {
  bf_status_t sts = BF_SUCCESS;
  uint32_t i = 0, pipe_cnt = pipe_mgr_get_num_active_pipes(dev);
  int pipe_mask = 0;
  uint32_t addr = 0, val = 0;
  bf_dev_target_t dev_tgt;

  dev_tgt.device_id = dev;

  /* Clear state in our SW shadow related to ports.  This will be rebuild when
   * we internally replay all pipe_mgr ports. */
  struct pipe_mgr_tof3_pg_dev_ctx *c = pipe_mgr_tof3_get_pgr_ctx(dev);
  if (!c) return PIPE_UNEXPECTED;
  c->port_ctrl = 0;
  for (i = 0; c && i < pipe_cnt; ++i) {
    for (int j = 0; j < 8; ++j) c->ipb_chnl_sp[i][j] = 0;
    c->ipb_ctrl[i] = 0;
    c->ebuf_chnl_en[i] = 0;
    for (int j = 0; j < 4; ++j) c->ebuf_port_ctrl[i][j] = 0;
  }

  sts = pipe_mgr_tof3_pktgen_dev_init(shdl, dev, true);
  if (sts != BF_SUCCESS) return sts;

  for (i = 0; i < pipe_cnt; ++i) {
    dev_tgt.device_id = dev;
    dev_tgt.dev_pipe_id = i;
    pipe_mask = pg_log_pipe_mask(dev_tgt);

    // port_down_mode
    dev_tgt.dev_pipe_id = i;
    sts = pipe_mgr_tof3_pktgen_cfg_port_down_replay(
        shdl, dev_tgt, pipe_mgr_tof3_get_pgr_ctx(dev)->port_down_mode[i]);
    if (sts != BF_SUCCESS) return sts;

    // port_down_mask
    for (uint32_t mask_sel = 0; mask_sel < 2; mask_sel++) {
      addr = offsetof(
          tof3_reg,
          pipes[0]
              .pardereg.pgstnreg.pgrreg.pgr_common.pgen_port_down_mask[mask_sel]
              .pgen_port_down_mask_0_3);
      val = pipe_mgr_pktgen_ctx(dev)
                ->u.tof3_ctx->port_down_mask[i][mask_sel]
                .val_0;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (BF_SUCCESS != sts) return sts;

      addr = offsetof(
          tof3_reg,
          pipes[0]
              .pardereg.pgstnreg.pgrreg.pgr_common.pgen_port_down_mask[mask_sel]
              .pgen_port_down_mask_1_3);
      val = pipe_mgr_pktgen_ctx(dev)
                ->u.tof3_ctx->port_down_mask[i][mask_sel]
                .val_1;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (BF_SUCCESS != sts) return sts;

      addr = offsetof(
          tof3_reg,
          pipes[0]
              .pardereg.pgstnreg.pgrreg.pgr_common.pgen_port_down_mask[mask_sel]
              .pgen_port_down_mask_2_3);
      val = pipe_mgr_pktgen_ctx(dev)
                ->u.tof3_ctx->port_down_mask[i][mask_sel]
                .val_2;
      sts = pg_write_one_pipe_reg(shdl, dev, pipe_mask, addr, val);
      if (BF_SUCCESS != sts) return sts;
    }
  }

  sts = pipe_mgr_tof3_pktgen_app_cfg_download(shdl, dev);
  if (sts != BF_SUCCESS) return sts;

  return sts;
}
