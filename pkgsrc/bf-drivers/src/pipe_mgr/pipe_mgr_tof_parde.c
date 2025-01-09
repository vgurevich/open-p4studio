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
 * @file pipe_mgr_tof_parde.c
 * @date
 *
 * Parser and Deparser programming
 */

#include "pipe_mgr_int.h"
#include "pipe_mgr_db.h"
#include "pipe_mgr_drv.h"
#include <tofino_regs/tofino.h>

pipe_status_t pipe_mgr_parde_tof_port_add_egr(pipe_sess_hdl_t shdl,
                                              rmt_dev_info_t *dev_info,
                                              bf_dev_port_t port_id) {
  (void)shdl;
  pipe_status_t sts = PIPE_SUCCESS;
  sts = pipe_mgr_tof_deprsr_set_port_speed_based_cfg(dev_info, port_id);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("%s: failed to initialize Deparser config as per port speed",
              __func__);
    return sts;
  }
  sts = pipe_mgr_tof_parb_set_port_egress_chnl_control(dev_info, port_id);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("%s: failed to initialize parser arbiter ingress port controls",
              __func__);
    return sts;
  }
  sts = pipe_mgr_tof_eprsr_port_speed_based_cfg(dev_info, port_id);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("%s: failed to initialize egress parser related to port speed",
              __func__);
    return sts;
  }
  sts = pipe_mgr_ebuf_tof_set_port_chnl_ctrl(dev_info, port_id);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("%s: failed to initialize egress port controls", __func__);
    return sts;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_parde_tof_complete_port_mode_transition_wa(
    pipe_sess_hdl_t shdl, rmt_dev_info_t *dev_info, bf_dev_port_t port_id) {
  pipe_status_t sts = PIPE_SUCCESS;

  sts = pipe_mgr_tof_eprsr_complete_port_mode_transition_wa(dev_info, port_id);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s: failed to initialize egress parser related to port speed for port "
        "%d",
        __func__,
        port_id);
    return sts;
  }

  pipe_mgr_drv_ilist_push(&shdl, NULL, NULL);
  pipe_mgr_complete_operations(shdl);

  sts = pipe_mgr_ebuf_tof_complete_port_mode_transition_wa(dev_info, port_id);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s: failed to initialize ebuf configs for port %d", __func__, port_id);
    return sts;
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_parde_tof_port_add_ing(pipe_sess_hdl_t shdl,
                                              rmt_dev_info_t *dev_info,
                                              bf_dev_port_t port_id) {
  (void)shdl;
  pipe_status_t sts = PIPE_SUCCESS;
  bf_dev_id_t dev_id = dev_info->dev_id;
  sts = pipe_mgr_tof_parb_set_port_ingress_chnl_control(dev_info, port_id);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("%s: failed to initialize parser arbiter ingress port controls",
              __func__);
    return sts;
  }
  sts = pipe_mgr_tof_iprsr_port_speed_based_cfg(dev_info, port_id);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("%s: failed to initialize ingress parser related to port speed",
              __func__);
    return sts;
  }
  sts = pipe_mgr_ibuf_tof_set_port_speed_based_cfg(dev_info, port_id);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("%s: failed to initialize ibuf config as per port-speed",
              __func__);
    return sts;
  }

  /*
   * Skip enabling ibuf channel for traffic if the fast reconfig is in
   * progress. In this case, ibuf channel will be enabled in traffic resume
   * stage. For all other scenarios (including HITLESS warm init), enable
   * ibuf channel for traffic.
   */
  if (!pipe_mgr_fast_recfg_warm_init_in_progress(dev_id)) {
    sts = pipe_mgr_ibuf_tof_enable_channel(dev_info, port_id);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR("%s: failed to enable ibuf channel", __func__);
      return sts;
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_parde_tof_port_rmv_egr(pipe_sess_hdl_t shdl,
                                              rmt_dev_info_t *dev_info,
                                              bf_dev_port_t port_id) {
  (void)shdl;
  pipe_status_t sts = PIPE_SUCCESS;
  sts = pipe_mgr_ebuf_tof_disable_port_chnl(dev_info, port_id);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("%s: failed to remove port", __func__);
  }
  return sts;
}

pipe_status_t pipe_mgr_parde_tof_port_rmv_ing(pipe_sess_hdl_t shdl,
                                              rmt_dev_info_t *dev_info,
                                              bf_dev_port_t port_id) {
  (void)shdl;
  pipe_status_t sts = PIPE_SUCCESS;
  sts = pipe_mgr_ibuf_tof_disable_chnl(dev_info, port_id);
  sts |= pipe_mgr_tof_parb_disable_chnl_control(dev_info, port_id);
  return sts;
}

pipe_status_t pipe_mgr_parde_tof_device_add(pipe_sess_hdl_t shdl,
                                            rmt_dev_info_t *dev_info) {
  pipe_status_t sts = PIPE_SUCCESS;
  bf_dev_id_t dev_id = dev_info->dev_id;

  if (!pipe_mgr_init_mode_fast_recfg_quick(dev_id)) {
    sts = pipe_mgr_ebuf_tof_dev_add(dev_info);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR("%s: failed to initialize ebuf state. Device %u, status %s",
                __func__,
                dev_id,
                pipe_str_err(sts));
      return sts;
    }
  }

  if (!pipe_mgr_hitless_warm_init_in_progress(dev_id)) {
    // No need to do these operations in hitless warm_init since HW is already
    // configured.
    sts = pipe_mgr_ibuf_tof_set_logical_port(shdl, dev_info);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR("%s: failed to initialize ingress buffer. Device %u, status %s",
                __func__,
                dev_id,
                pipe_str_err(sts));
      return sts;
    }

    sts = pipe_mgr_ebuf_tof_epb_set_100g_credits(shdl, dev_info);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s: failed to initialize epb credit control. Device %u, status %s",
          __func__,
          dev_id,
          pipe_str_err(sts));
      return sts;
    }

    sts = pipe_mgr_ibuf_tof_set_1588_timestamp_offset(shdl, dev_info);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR("%s: failed to set timestamp offset. Device %u, status %s",
                __func__,
                dev_id,
                pipe_str_err(sts));
      return sts;
    }

    sts = pipe_mgr_ebuf_tof_set_1588_timestamp_offset(shdl, dev_info);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR("%s: failed to set timestamp offset. Device %u, status %s",
                __func__,
                dev_id,
                pipe_str_err(sts));
      return sts;
    }
    sts = pipe_mgr_tof_parb_init(shdl, dev_info);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR("%s: failed to initialize port arbiter. Device %u, status %s",
                __func__,
                dev_id,
                pipe_str_err(sts));
      return sts;
    }
  }

  sts = pipe_mgr_tof_deprsr_cfg_init(shdl, dev_info);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("%s: failed to initialize deparser block. Device %u,  status %s",
              __func__,
              dev_id,
              pipe_str_err(sts));
    return sts;
  }
  return sts;
}

void pipe_mgr_parde_tof_device_rmv(rmt_dev_info_t *dev_info) {
  /* Clean up the deparser context. */
  pipe_mgr_tof_deprsr_cfg_deinit(dev_info->dev_id);

  /* Clean up the ebuf context. */
  pipe_mgr_ebuf_tof_dev_rmv(dev_info);
}

int pipe_mgr_parde_tof_speed_to_chan_cnt(bf_port_speeds_t speed) {
  switch (speed) {
    case BF_SPEED_NONE:
    case BF_SPEED_1G:
    case BF_SPEED_10G:
    case BF_SPEED_25G:
      return 1;
    case BF_SPEED_40G:
    case BF_SPEED_50G:
      return 2;
    case BF_SPEED_100G:
      return 4;
    default:
      PIPE_MGR_DBGCHK(0);
      return 0;
  }
}

pipe_status_t pipe_mgr_parde_tof_port_ena_one(pipe_sess_hdl_t shdl,
                                              rmt_dev_info_t *dev_info,
                                              uint8_t logical_pipe,
                                              int ipb_num,
                                              bool ing_0_egr_1) {
  bf_dev_pipe_t phy_pipe = 0;
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, logical_pipe, &phy_pipe);
  pipe_status_t sts = PIPE_SUCCESS;
  if (ing_0_egr_1 == 0) {
    for (int i = 0; i < TOF_NUM_CHN_PER_PORT;) {
      bf_dev_port_t p_local = ipb_num * TOF_NUM_CHN_PER_PORT + i;
      bf_dev_port_t p = dev_info->dev_cfg.make_dev_port(logical_pipe, p_local);
      rmt_port_info_t *port_info = pipe_mgr_get_port_info(dev_info->dev_id, p);
      if (port_info) {
        sts =
            ibuf_set_chnl_ctrl(dev_info, port_info->port_id, true, shdl, false);
        if (sts != PIPE_SUCCESS) {
          LOG_ERROR("Failed to enable channel for dev %d port %d sts %s",
                    dev_info->dev_id,
                    port_info->port_id,
                    pipe_str_err(sts));
          return sts;
        }
        i += pipe_mgr_parde_tof_speed_to_chan_cnt(port_info->speed);
      } else {
        ++i;
      }
    }
  } else if (ing_0_egr_1 == 1) {
    for (int i = 0; i < TOF_NUM_CHN_PER_PORT;) {
      bf_dev_port_t p_local = ipb_num * TOF_NUM_CHN_PER_PORT + i;
      bf_dev_port_t p = dev_info->dev_cfg.make_dev_port(logical_pipe, p_local);
      rmt_port_info_t *port_info = pipe_mgr_get_port_info(dev_info->dev_id, p);
      if (port_info) {
        sts = ebuf_set_epb_prsr_port_chnl_ctrl_en_reg(
            dev_info, logical_pipe, p_local, true);
        if (sts != PIPE_SUCCESS) {
          LOG_ERROR("Failed to enable channel for dev %d port %d sts %s",
                    dev_info->dev_id,
                    port_info->port_id,
                    pipe_str_err(sts));
          return sts;
        }
        i += pipe_mgr_parde_tof_speed_to_chan_cnt(port_info->speed);
      } else {
        ++i;
      }
    }
  } else {
    LOG_ERROR("Invalid gress %d, dev %d", ing_0_egr_1, dev_info->dev_id);
    return PIPE_INVALID_ARG;
  }
  return sts;
}

pipe_status_t pipe_mgr_parde_tof_port_dis_one(pipe_sess_hdl_t shdl,
                                              rmt_dev_info_t *dev_info,
                                              uint8_t logical_pipe,
                                              int ipb_num,
                                              bool ing_0_egr_1) {
  bf_dev_pipe_t phy_pipe = 0;
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, logical_pipe, &phy_pipe);
  pipe_status_t sts = PIPE_SUCCESS;
  if (ing_0_egr_1 == 0) {
    // ingress
    for (int i = 0; i < TOF_NUM_CHN_PER_PORT;) {
      bf_dev_port_t p_local = ipb_num * TOF_NUM_CHN_PER_PORT + i;
      bf_dev_port_t p = dev_info->dev_cfg.make_dev_port(logical_pipe, p_local);
      rmt_port_info_t *port_info = pipe_mgr_get_port_info(dev_info->dev_id, p);
      if (port_info) {
        sts = ibuf_set_chnl_ctrl(
            dev_info, port_info->port_id, false, shdl, false);
        if (sts != PIPE_SUCCESS) {
          LOG_ERROR("Failed to disable channel for dev %d port %d sts %s",
                    dev_info->dev_id,
                    port_info->port_id,
                    pipe_str_err(sts));
          return sts;
        }
        i += pipe_mgr_parde_tof_speed_to_chan_cnt(port_info->speed);
      } else {
        ++i;
      }
    }
  } else if (ing_0_egr_1 == 1) {
    for (int i = 0; i < TOF_NUM_CHN_PER_PORT;) {
      bf_dev_port_t p_local = ipb_num * TOF_NUM_CHN_PER_PORT + i;
      bf_dev_port_t p = dev_info->dev_cfg.make_dev_port(logical_pipe, p_local);
      rmt_port_info_t *port_info = pipe_mgr_get_port_info(dev_info->dev_id, p);
      if (port_info) {
        sts = ebuf_set_epb_prsr_port_chnl_ctrl_en_reg(
            dev_info, logical_pipe, p_local, false);
        if (sts != PIPE_SUCCESS) {
          LOG_ERROR("Failed to disable channel for dev %d port %d sts %s",
                    dev_info->dev_id,
                    port_info->port_id,
                    pipe_str_err(sts));
          return sts;
        }
        i += pipe_mgr_parde_tof_speed_to_chan_cnt(port_info->speed);
      } else {
        ++i;
      }
    }
  } else {
    LOG_ERROR("Invalid gress %d, dev %d", ing_0_egr_1, dev_info->dev_id);
    return PIPE_INVALID_ARG;
  }
  return sts;
}

/* Copy data from a list of source addresses into a single DMA buffer and issue
 * a block-write DMA to the target address.
 * The sources are passed as pointers in the data array, each source consumes
 * two array slots with a start pointer and limit pointer.  Data is copied from
 * the start pointer upto but not including the end pointer.
 * The num_segments indicate how many start+limit pairs are in the data array.
 */
pipe_status_t pipe_mgr_config_multi_mem_tof(pipe_sess_hdl_t sess_hdl,
                                            rmt_dev_info_t *dev_info,
                                            uint8_t **data,
                                            int num_segments,
                                            uint64_t addr,
                                            uint8_t log_pipe_mask) {
  pipe_mgr_drv_buf_t *b;
  pipe_mgr_drv_ses_state_t *st =
      pipe_mgr_drv_get_ses_state(&sess_hdl, __func__, __LINE__);
  if (!st) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }

  int bwr_size = pipe_mgr_drv_buf_size(dev_info->dev_id, PIPE_MGR_DRV_BUF_BWR);
  b = pipe_mgr_drv_buf_alloc(
      st->sid, dev_info->dev_id, bwr_size, PIPE_MGR_DRV_BUF_BWR, true);
  if (!b) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }

  size_t offset = 0;
  for (int i = 0; i < num_segments; ++i) {
    uint8_t *start = data[2 * i + 0];
    uint8_t *end = data[2 * i + 1];
    ptrdiff_t len = end - start;
    if (len <= 0) {
      pipe_mgr_drv_buf_free(b);
      return PIPE_INVALID_ARG;
    }
    PIPE_MGR_MEMCPY(b->addr + offset, start, len);
    offset += len;
  }

  pipe_status_t status = pipe_mgr_drv_blk_wr(
      &sess_hdl, 16, offset / 16, 1, addr, log_pipe_mask, b);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s : Write block push for prsr memory error %s",
              __func__,
              pipe_str_err(status));
    pipe_mgr_drv_buf_free(b);
    PIPE_MGR_DBGCHK(PIPE_SUCCESS == status);
    return PIPE_UNEXPECTED;
  }
  return status;
}

pipe_status_t pipe_mgr_config_one_mem_tof(pipe_sess_hdl_t sess_hdl,
                                          rmt_dev_info_t *dev_info,
                                          uint8_t *data,
                                          int depth,
                                          int width,
                                          uint64_t addr,
                                          uint8_t log_pipe_mask) {
  pipe_mgr_drv_buf_t *b;
  pipe_mgr_drv_ses_state_t *st =
      pipe_mgr_drv_get_ses_state(&sess_hdl, __func__, __LINE__);
  if (!st) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }

  int bwr_size = pipe_mgr_drv_buf_size(dev_info->dev_id, PIPE_MGR_DRV_BUF_BWR);
  b = pipe_mgr_drv_buf_alloc(
      st->sid, dev_info->dev_id, bwr_size, PIPE_MGR_DRV_BUF_BWR, true);
  if (!b) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }
  PIPE_MGR_MEMCPY((void *)b->addr, data, width * depth);
  pipe_status_t status = pipe_mgr_drv_blk_wr(
      &sess_hdl, 16, width * depth / 16, 1, addr, log_pipe_mask, b);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s : Write block push for prsr memory error %s",
              __func__,
              pipe_str_err(status));
    pipe_mgr_drv_buf_free(b);
    PIPE_MGR_DBGCHK(PIPE_SUCCESS == status);
    return PIPE_UNEXPECTED;
  }
  return status;
}

pipe_status_t pipe_mgr_parser_config_tof(
    pipe_sess_hdl_t sess_hdl,
    rmt_dev_info_t *dev_info,
    uint8_t gress,
    pipe_bitmap_t pipe_bmp,
    uint64_t prsr_map,
    struct pipe_mgr_tof_prsr_bin_config *cfg) {
  uint32_t prsr_max = TOF_NUM_PARSERS;
  struct pipe_mgr_tof_prsr_base_addr *addr_db =
      &PIPE_PRSR_ADDR(dev_info->dev_id, gress).tof;
  pipe_status_t sts;
  uint8_t log_pipe_mask = 0;
  uint32_t prsr_stage = 0;
  uint32_t i;

  if (LLD_OK != lld_sku_get_prsr_stage(dev_info->dev_id, &prsr_stage)) {
    LOG_ERROR("%s:%d Failed to get prsr stage for device %d",
              __func__,
              __LINE__,
              dev_info->dev_id);
    return PIPE_INVALID_ARG;
  }
  for (i = 0; i < dev_info->num_active_pipes; i++) {
    if (PIPE_BITMAP_GET(&pipe_bmp, i)) log_pipe_mask |= (1u << i);
  }
  for (uint32_t prsr_id = 0; prsr_id < prsr_max; prsr_id++) {
    if ((prsr_map & (1 << prsr_id)) == 0) continue;
    uint64_t prsr_offset = (prsr_id * addr_db->prsr_step);
    uint32_t prsr_reg_offset = (prsr_id * addr_db->prsr_reg_step);

    /* TCAMs are written per-pipe as asymmetric PVS entries may exist. */
    PIPE_BITMAP_ITER(&pipe_bmp, i) {
      bf_dev_pipe_t phy_pipe_id = 0;
      uint8_t tmp_pipe_mask = (1u << i);
      pipe_mgr_map_pipe_id_log_to_phy(dev_info, i, &phy_pipe_id);
      // word0_addr
      sts = pipe_mgr_config_one_mem_tof(
          sess_hdl,
          dev_info,
          &(PIPE_INTR_PRSR_TCAM_DATA(dev_info->dev_id, phy_pipe_id, prsr_id)
                .tof[gress]
                .word0_data[0][0]),
          TOF_PARSER_DEPTH,
          PIPE_MGR_TOF_TCAM_WORD_WIDTH,
          (addr_db->word0_addr + prsr_offset),
          tmp_pipe_mask);
      if (sts != PIPE_SUCCESS) return sts;
      // word1_addr
      sts = pipe_mgr_config_one_mem_tof(
          sess_hdl,
          dev_info,
          &(PIPE_INTR_PRSR_TCAM_DATA(dev_info->dev_id, phy_pipe_id, prsr_id)
                .tof[gress]
                .word1_data[0][0]),
          TOF_PARSER_DEPTH,
          PIPE_MGR_TOF_TCAM_WORD_WIDTH,
          (addr_db->word1_addr + prsr_offset),
          tmp_pipe_mask);
      if (sts != PIPE_SUCCESS) return sts;
    }

    /* 0 Write 0x21c80000200-0x21c800002FF: 256 EA Memory entries.
     * 1 Write 0x21c80000300-0x21c800003FF: 256 Dummy entries.
     * 2 Write 0x21c80000400-0x21c800005FF: 256 x2 PO Memory entries. */
    uint8_t *addrs[4 * 2];
    addrs[0] = &cfg->ea_row_data[0][0];
    addrs[1] = addrs[0] + TOF_PARSER_DEPTH * PIPE_MGR_TOF_TCAM_WORD_WIDTH;
    addrs[2] = addrs[0]; /* Filler data. */
    addrs[3] = addrs[2] + 256 * 16;
    addrs[4] = &cfg->po_action_data[0][0];
    addrs[5] = addrs[4] + TOF_PARSER_DEPTH * PIPE_MGR_TOF_PO_WORD_WIDTH;
    sts = pipe_mgr_config_multi_mem_tof(sess_hdl,
                                        dev_info,
                                        addrs,
                                        3,
                                        addr_db->ea_row_addr + prsr_offset,
                                        log_pipe_mask);
    if (sts != PIPE_SUCCESS) return sts;

    /* 0 Write 0x21c80000600-0x21c8000060F: 16 CounterInit entries.
     * 1 Write 0x21c80000610-0x21c8000061F: 16 Dummy entries.
     * 2 Write 0x21c80000620-0x21c8000063F: 32 Cksm0 entries.
     * 3 Write 0x21c80000640-0x21c8000065F: 32 Cksm1 entries.  */
    addrs[0] = &cfg->ctr_init_ram_data[0][0];
    addrs[1] =
        addrs[0] + TOF_PARSER_INIT_RAM_DEPTH * PIPE_MGR_TOF_TCAM_WORD_WIDTH;
    addrs[2] = addrs[0]; /* Filler data. */
    addrs[3] = addrs[2] + 16 * 16;
    addrs[4] = &cfg->po_csum_ctr0_data[0][0];
    addrs[5] = addrs[4] + TOF_PARSER_CSUM_DEPTH * PIPE_MGR_TOF_TCAM_WORD_WIDTH;
    addrs[6] = &cfg->po_csum_ctr1_data[0][0];
    addrs[7] = addrs[6] + TOF_PARSER_CSUM_DEPTH * PIPE_MGR_TOF_TCAM_WORD_WIDTH;
    sts =
        pipe_mgr_config_multi_mem_tof(sess_hdl,
                                      dev_info,
                                      addrs,
                                      4,
                                      addr_db->ctr_init_ram_addr + prsr_offset,
                                      log_pipe_mask);
    if (sts != PIPE_SUCCESS) return sts;
    // register
    for (i = 0; i < TOF_PRSR_REG_DEPTH; i++) {
      pipe_instr_write_reg_t instr;
      construct_instr_reg_write(dev_info->dev_id,
                                &instr,
                                (addr_db->prsr_reg_addr[i] + prsr_reg_offset),
                                cfg->prsr_reg_data[i]);
      sts = pipe_mgr_drv_ilist_add(&sess_hdl,
                                   dev_info,
                                   &pipe_bmp,
                                   prsr_stage,
                                   (uint8_t *)&instr,
                                   sizeof(pipe_instr_write_reg_t));
      if (sts != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s : Instruction list add failed for parser register "
            "configuration, error %s",
            __func__,
            pipe_str_err(sts));
        PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
        return sts;
      }
    }

    /* Now force the max_cycle register to a value of zero to disable it.
     * This is required for egress parsers as we should expect packet data to
     * potentially stay in the parser for an extended period of time if the MAC
     * receives pause frames.  */
    if (gress) {
      pipe_instr_write_reg_t instr;
      uint32_t max_cycle = offsetof(
          Tofino, pipes[0].pmarb.ebp18_reg.ebp_reg[prsr_id].prsr_reg.max_cycle);
      construct_instr_reg_write(dev_info->dev_id, &instr, max_cycle, 0);
      sts = pipe_mgr_drv_ilist_add(&sess_hdl,
                                   dev_info,
                                   &pipe_bmp,
                                   prsr_stage,
                                   (uint8_t *)&instr,
                                   sizeof instr);
      if (sts != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s : Instruction list add failed for parser max_cycle "
            "configuration, prsr %d, error %s",
            __func__,
            prsr_id,
            pipe_str_err(sts));
        PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
        return sts;
      }
    }
  }
  return PIPE_SUCCESS;
}
