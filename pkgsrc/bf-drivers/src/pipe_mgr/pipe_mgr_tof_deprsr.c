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
 * @file pipe_mgr_tof_deprsr.c
 * @date
 *
 * Configuration of Tofino Deparser based on port speed.
 */

#include "pipe_mgr_int.h"
#include <tofino_regs/tofino.h>
#include "pipe_mgr_drv_intf.h"

static int get_edf_min_threshold(bf_dev_id_t dev_id, int out_egr) {
#define BF_TOFINO_EDF_MIN_THRESHOLD (380)
#define BF_TOFINO_B0_OUT_INGR_EDF_MIN_THRESHOLD (240)
#define BF_TOFINO_B0_OUT_EGR_EDF_MIN_THRESHOLD (160)
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s : Failed to get device info, dev %d ", __func__, dev_id);
    return (BF_TOFINO_EDF_MIN_THRESHOLD);
  }
  if (dev_info->part_rev == BF_SKU_CHIP_PART_REV_A0) {
    return (BF_TOFINO_EDF_MIN_THRESHOLD);
  }
  if (out_egr) {
    return (BF_TOFINO_B0_OUT_EGR_EDF_MIN_THRESHOLD);
  } else {
    return (BF_TOFINO_B0_OUT_INGR_EDF_MIN_THRESHOLD);
  }
}

static uint32_t get_ctm_fcu_cfg(rmt_dev_info_t *dev_info) {
  (void)dev_info;

  /* return 0 for all part revisions */
  return 0;
}

static void get_ch_rate_edf_inc(bf_dev_id_t dev_id,
                                rmt_port_info_t *port_info,
                                uint32_t *ch_rate,
                                uint32_t *edf_inc,
                                uint32_t *num_ch) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    *ch_rate = 0;
    *edf_inc = 0;
    *num_ch = 0;
    LOG_ERROR("%s : Failed to get device info, dev %d ", __func__, dev_id);
    return;
  }
  if (dev_info->part_rev == BF_SKU_CHIP_PART_REV_A0) {
    switch (port_info->speed) {
      case BF_SPEED_40G:
        *ch_rate = 1;
        *edf_inc = 8;
        *num_ch = 2;
        break;
      case BF_SPEED_50G:
        *ch_rate = 1;
        *edf_inc = 11;
        *num_ch = 2;
        break;
      case BF_SPEED_100G:
        *ch_rate = 1;
        *edf_inc = 23;
        *num_ch = 4;
        break;
      case BF_SPEED_25G:
        *ch_rate = 0;
        *edf_inc = 5;
        *num_ch = 1;
        break;
      case BF_SPEED_1G:
      case BF_SPEED_10G:
        *ch_rate = 0;
        *edf_inc = 2;
        *num_ch = 1;
        break;
      default:
        *ch_rate = 0;
        *edf_inc = 2;
        *num_ch = 1;
        break;
    }
  } else {
    switch (port_info->speed) {
      case BF_SPEED_40G:
        *ch_rate = 1;
        *edf_inc = 8;
        *num_ch = 2;
        break;
      case BF_SPEED_50G:
        *ch_rate = 1;
        *edf_inc = 10;
        *num_ch = 2;
        break;
      case BF_SPEED_100G:
        *ch_rate = 1;
        *edf_inc = 20;
        *num_ch = 4;
        break;
      case BF_SPEED_25G:
        *ch_rate = 0;
        *edf_inc = 5;
        *num_ch = 1;
        break;
      case BF_SPEED_1G:
      case BF_SPEED_10G:
        *ch_rate = 0;
        *edf_inc = 2;
        *num_ch = 1;
        break;
      default:
        *ch_rate = 0;
        *edf_inc = 2;
        *num_ch = 1;
        break;
    }
  }
}

pipe_status_t pipe_mgr_tof_deprsr_set_port_speed_based_cfg(
    rmt_dev_info_t *dev_info, bf_dev_port_t port_id) {
  bf_dev_pipe_t pipe = 0;
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  uint32_t offset, ch_rate, edf_inc, num_ch, i;
  rmt_port_info_t *port_info = NULL;
  bf_dev_id_t dev_id = dev_info->dev_id;
  union pipe_mgr_deprsr_ctx *ctx = pipe_mgr_deprsr_ctx(dev_id);

  if (!ctx) {
    LOG_ERROR("%s: context for dev_id(%d) not found", __func__, dev_id);
    return PIPE_INVALID_ARG;
  }

  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port);
    return PIPE_INVALID_ARG;
  }
  pipe = port_info->phy_pipe;

  get_ch_rate_edf_inc(dev_id, port_info, &ch_rate, &edf_inc, &num_ch);
  if (num_ch == 0) return (PIPE_INVALID_ARG);
  if ((port + num_ch - 1) >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);

  if (port < 32) {
    offset = offsetof(
        Tofino, pipes[pipe].deparser.out_ingr.regs.ctm_ch_rate.ctm_ch_rate_0_3);
    ch_rate <<= port;
    ch_rate |=
        (ctx->tof.ch_rate_0_3[pipe] &
         ~(1ul
           << port));  // Clear bit in ch_rate_0_3 before OR ing in new ch_rate
    ctx->tof.ch_rate_0_3[pipe] = ch_rate;
  } else if (port < 64) {
    ch_rate <<= (port - 32);
    offset = offsetof(
        Tofino, pipes[pipe].deparser.out_ingr.regs.ctm_ch_rate.ctm_ch_rate_1_3);
    ch_rate |= (ctx->tof.ch_rate_1_3[pipe] &
                ~(1ul << (port - 32)));  // Clear bit in ch_rate_1_3
                                         // before OR ing in new
                                         // ch_rate
    ctx->tof.ch_rate_1_3[pipe] = ch_rate;
  } else {
    ch_rate <<= (port - 64);
    offset = offsetof(
        Tofino, pipes[pipe].deparser.out_ingr.regs.ctm_ch_rate.ctm_ch_rate_2_3);
    ch_rate |= (ctx->tof.ch_rate_2_3[pipe] &
                ~(1ul << (port - 64)));  // Clear bit in ch_rate_2_3
                                         // before OR ing in new
                                         // ch_rate
    ctx->tof.ch_rate_2_3[pipe] = ch_rate;
  }
  pipe_mgr_write_register(dev_id, 0, offset, ch_rate);
  for (i = 0; i < num_ch; i++) {
    pipe_mgr_write_register(
        dev_id,
        0,
        offsetof(Tofino, pipes[pipe].deparser.out_ingr.regs.ctm_fifo_rst),
        port + i);
    pipe_mgr_write_register(
        dev_id,
        0,
        offsetof(Tofino,
                 pipes[pipe].deparser.hdr.hem.he_edf_cfg.edf_inc[port + i]),
        edf_inc);
    pipe_mgr_write_register(
        dev_id,
        0,
        offsetof(Tofino,
                 pipes[pipe].deparser.hdr.him.hi_edf_cfg.edf_inc[port + i]),
        edf_inc);
    pipe_mgr_write_register(
        dev_id,
        0,
        offsetof(Tofino,
                 pipes[pipe].deparser.out_egr.mem.edf_cfg.edf_inc[port + i]),
        edf_inc);
    pipe_mgr_write_register(
        dev_id,
        0,
        offsetof(Tofino,
                 pipes[pipe].deparser.out_ingr.mem.edf_cfg.edf_inc[port + i]),
        edf_inc);
  }

  LOG_TRACE(
      "%s: port %d Deparser channel rate = 0x%x", __func__, port, ch_rate);
  return (PIPE_SUCCESS);
}

static pipe_status_t deprsr_set_pipe_mapping(pipe_sess_hdl_t sess_hdl,
                                             rmt_dev_info_t *dev_info) {
  LOG_TRACE("%s: Deparser set ipipe_id and epipe_id", __func__);
  uint32_t i, pipe_map_val = 0;

  pipe_bitmap_t pipe_bit_map = {{0}};
  PIPE_BITMAP_INIT(&pipe_bit_map, PIPE_BMP_SIZE);
  uint32_t num_active_pipes = dev_info->num_active_pipes;
  for (i = 0; i < num_active_pipes; ++i) {
    bf_dev_pipe_t phy_pipe = 0;
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, i, &phy_pipe);
    pipe_map_val |= (phy_pipe << (i * 2));
    PIPE_BITMAP_SET(&pipe_bit_map, i);
  }
  uint32_t stage;
  bf_dev_id_t dev_id = dev_info->dev_id;
  lld_err_t lld_err = lld_sku_get_dprsr_stage(dev_id, &stage);
  if (LLD_OK != lld_err) {
    LOG_ERROR("Failed to get deparser id at %s", __func__);
    PIPE_MGR_DBGCHK(LLD_OK == lld_err);
    return PIPE_INIT_ERROR;
  }

  /* Do not program ipipe_id to remap logical pipe to physical pipe.
   * ipipe_id has to be programmed to map logical to physical only
   * if ingress_port metadata fed into Deparser is from PHV container.
   */
  /* For complete discussion see JIRA DRV-557 */
  pipe_instr_write_reg_t instr;
  uint32_t addr = offsetof(Tofino, pipes[0].deparser.out_ingr.regs.epipe_id);
  construct_instr_reg_write(dev_id, &instr, addr, pipe_map_val);
  pipe_status_t sts = pipe_mgr_drv_ilist_add(&sess_hdl,
                                             dev_info,
                                             &pipe_bit_map,
                                             stage,
                                             (uint8_t *)&instr,
                                             sizeof instr);
  if (PIPE_SUCCESS != sts) {
    LOG_ERROR("Failed to add deparser pipe map to instruction list (%s)",
              pipe_str_err(sts));
    PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
    return sts;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tof_deprsr_cfg_init(pipe_sess_hdl_t sess_hdl,
                                           rmt_dev_info_t *dev_info) {
  uint32_t i;
  pipe_status_t sts;

  LOG_TRACE("%s: Deparser config init", __func__);
  bf_dev_id_t dev_id = dev_info->dev_id;

  union pipe_mgr_deprsr_ctx *ctx = pipe_mgr_deprsr_ctx(dev_id);
  int dev_pipes = dev_info->dev_cfg.num_pipelines;

  if (!ctx) {
    LOG_ERROR("%s: context for dev_id(%d) not found", __func__, dev_id);
    return PIPE_INVALID_ARG;
  }

  if (!pipe_mgr_init_mode_fast_recfg_quick(dev_id)) {
    ctx->tof.ch_rate_0_3 =
        PIPE_MGR_CALLOC(dev_pipes, sizeof *ctx->tof.ch_rate_0_3);
    ctx->tof.ch_rate_1_3 =
        PIPE_MGR_CALLOC(dev_pipes, sizeof *ctx->tof.ch_rate_1_3);
    ctx->tof.ch_rate_2_3 =
        PIPE_MGR_CALLOC(dev_pipes, sizeof *ctx->tof.ch_rate_2_3);
  }
  if (!ctx->tof.ch_rate_0_3 || !ctx->tof.ch_rate_1_3 || !ctx->tof.ch_rate_2_3) {
    sts = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }

  if (pipe_mgr_hitless_warm_init_in_progress(dev_id)) {
    // Read the ch_rate_x_3 from the HW for hitless warm_init case
    // This is needed to make the software cache in sync with HW
    uint32_t num_active_pipes = dev_info->num_active_pipes;
    for (i = 0; i < num_active_pipes; ++i) {
      bf_dev_pipe_t phy_pipe = 0;
      pipe_mgr_map_pipe_id_log_to_phy(dev_info, i, &phy_pipe);

      for (uint32_t j = 0; j < 3; j++) {
        uint32_t reg_addr = 0, reg_val = 0;

        if (j == 0) {
          reg_addr =
              offsetof(Tofino,
                       pipes[phy_pipe]
                           .deparser.out_ingr.regs.ctm_ch_rate.ctm_ch_rate_0_3);
        } else if (j == 1) {
          reg_addr =
              offsetof(Tofino,
                       pipes[phy_pipe]
                           .deparser.out_ingr.regs.ctm_ch_rate.ctm_ch_rate_1_3);
        } else {
          reg_addr =
              offsetof(Tofino,
                       pipes[phy_pipe]
                           .deparser.out_ingr.regs.ctm_ch_rate.ctm_ch_rate_2_3);
        }

        sts = pipe_mgr_drv_reg_rd(&sess_hdl, dev_id, reg_addr, &reg_val);
        if (sts != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Error in reading ctm_ch_rate_0_3 from hw dev %d phy_pipe "
              "%d sts 0x%x(%s)",
              __func__,
              __LINE__,
              dev_id,
              phy_pipe,
              sts,
              pipe_str_err(sts));
          goto cleanup;
        }
        // ch_rate_0_3 [31 : 0]
        // ch_rate_1_3 [63 : 32]
        // ch_rate_2_3 [71 : 64]
        if (j == 0) {
          ctx->tof.ch_rate_0_3[phy_pipe] = reg_val;
        } else if (j == 1) {
          ctx->tof.ch_rate_1_3[phy_pipe] = reg_val;
        } else {
          ctx->tof.ch_rate_2_3[phy_pipe] = reg_val;
        }
      }
      // cleanup bits [31:8] since these are unused ( > 72 ports)
      ctx->tof.ch_rate_2_3[phy_pipe] &= (0xfful);
    }
  } else {
    // No need to do these API in hitless WARM_INIT Mode
    sts = deprsr_set_pipe_mapping(sess_hdl, dev_info);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Failed to add deparser pipe map to instruction list (%s)",
                pipe_str_err(sts));
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
      goto cleanup;
    }

    /* Build bit map of pipes to program. */
    pipe_bitmap_t pipe_bit_map = {{0}};
    PIPE_BITMAP_INIT(&pipe_bit_map, PIPE_BMP_SIZE);
    uint32_t num_active_pipes = dev_info->num_active_pipes;
    for (i = 0; i < num_active_pipes; ++i) {
      PIPE_BITMAP_SET(&pipe_bit_map, i);
    }

    /* Get the stage id of the deparser. */
    uint32_t stage;
    lld_err_t lld_err = lld_sku_get_dprsr_stage(dev_id, &stage);
    if (LLD_OK != lld_err) {
      LOG_ERROR("Failed to get deparser id at %s", __func__);
      PIPE_MGR_DBGCHK(LLD_OK == lld_err);
      sts = PIPE_INIT_ERROR;
      goto cleanup;
    }

    /* Setup instructions to program deparser registers. */
    for (i = 0; i < 2; ++i) {
      pipe_instr_write_reg_t instr;
      uint32_t addr =
          i ? offsetof(Tofino, pipes[0].deparser.out_egr.regs.edf_min_thresh)
            : offsetof(Tofino, pipes[0].deparser.out_ingr.regs.edf_min_thresh);
      construct_instr_reg_write(
          dev_id, &instr, addr, get_edf_min_threshold(dev_id, i));
      sts = pipe_mgr_drv_ilist_add(&sess_hdl,
                                   dev_info,
                                   &pipe_bit_map,
                                   stage,
                                   (uint8_t *)&instr,
                                   sizeof instr);
      if (PIPE_SUCCESS != sts) {
        LOG_ERROR("Failed to add deparser cfg to instruction list (%s)",
                  pipe_str_err(sts));
        PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
        goto cleanup;
      }
    }

    /* Setup instructions to program deparser ctm_fcu_cfg registers */
    uint32_t ctm_fcu_cfg = get_ctm_fcu_cfg(dev_info);
    for (i = 0; i < 2; ++i) {
      pipe_instr_write_reg_t instr;
      uint32_t addr =
          i ? offsetof(Tofino, pipes[0].deparser.out_egr.regs.ctm_fcu_cfg)
            : offsetof(Tofino, pipes[0].deparser.out_ingr.regs.ctm_fcu_cfg);
      construct_instr_reg_write(dev_id, &instr, addr, ctm_fcu_cfg);
      sts = pipe_mgr_drv_ilist_add(&sess_hdl,
                                   dev_info,
                                   &pipe_bit_map,
                                   stage,
                                   (uint8_t *)&instr,
                                   sizeof instr);
      if (PIPE_SUCCESS != sts) {
        LOG_ERROR("Failed to add deparser fcu cfg to instruction list (%s)",
                  pipe_str_err(sts));
        PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
        goto cleanup;
      }
    }

    // For B0 and later parts
    if (dev_info->part_rev != BF_SKU_CHIP_PART_REV_A0) {
      pipe_instr_write_reg_t instr2;
      uint32_t addr2 =
          offsetof(Tofino, pipes[0].deparser.out_egr.regs.edf_ctr_limit);
      construct_instr_reg_write(dev_id, &instr2, addr2, 0x3f0);
      sts = pipe_mgr_drv_ilist_add(&sess_hdl,
                                   dev_info,
                                   &pipe_bit_map,
                                   stage,
                                   (uint8_t *)&instr2,
                                   sizeof instr2);
      if (PIPE_SUCCESS != sts) {
        LOG_ERROR(
            "Failed to add deparser edf ctr limit to instruction list (%s)",
            pipe_str_err(sts));
        PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
        goto cleanup;
      }
      for (i = 0; i < 2; ++i) {
        pipe_instr_write_reg_t instr;
        uint32_t addr =
            i ? offsetof(Tofino, pipes[0].deparser.out_egr.regs.edf_cfg)
              : offsetof(Tofino, pipes[0].deparser.out_ingr.regs.edf_cfg);
        uint32_t val = i ? 1 : 0;
        construct_instr_reg_write(dev_id, &instr, addr, val);
        sts = pipe_mgr_drv_ilist_add(&sess_hdl,
                                     dev_info,
                                     &pipe_bit_map,
                                     stage,
                                     (uint8_t *)&instr,
                                     sizeof instr);
        if (PIPE_SUCCESS != sts) {
          LOG_ERROR(
              "Failed to add deparser edf cfg reg to instruction list (%s)",
              pipe_str_err(sts));
          PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
          goto cleanup;
        }
      }

      /* Setup instructions to program deparser edf_hi_thresh registers */
      uint32_t edf_hi_thresh =
          0x1e4;  // default value for rev B and later parts
      for (i = 0; i < 2; ++i) {
        pipe_instr_write_reg_t instr;
        uint32_t addr =
            i ? offsetof(Tofino, pipes[0].deparser.out_egr.regs.edf_hi_thresh)
              : offsetof(Tofino, pipes[0].deparser.out_ingr.regs.edf_hi_thresh);
        construct_instr_reg_write(dev_id, &instr, addr, edf_hi_thresh);
        sts = pipe_mgr_drv_ilist_add(&sess_hdl,
                                     dev_info,
                                     &pipe_bit_map,
                                     stage,
                                     (uint8_t *)&instr,
                                     sizeof instr);
        if (PIPE_SUCCESS != sts) {
          LOG_ERROR(
              "Failed to add deparser edf_hi_thresh to instruction "
              "list (%s)",
              pipe_str_err(sts));
          PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
          goto cleanup;
        }
      }

      /* Setup instructions to program deparser load_interval registers */
      uint32_t load_interval = 2;  // default value for rev B and later parts
      for (i = 0; i < 2; ++i) {
        pipe_instr_write_reg_t instr;
        uint32_t addr =
            i ? offsetof(Tofino, pipes[0].deparser.out_egr.regs.load_interval)
              : offsetof(Tofino, pipes[0].deparser.out_ingr.regs.load_interval);
        construct_instr_reg_write(dev_id, &instr, addr, load_interval);
        sts = pipe_mgr_drv_ilist_add(&sess_hdl,
                                     dev_info,
                                     &pipe_bit_map,
                                     stage,
                                     (uint8_t *)&instr,
                                     sizeof instr);
        if (PIPE_SUCCESS != sts) {
          LOG_ERROR(
              "Failed to add deparser load_interval to instruction "
              "list (%s)",
              pipe_str_err(sts));
          PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
          goto cleanup;
        }
      }
    }
  }
  return PIPE_SUCCESS;
cleanup:
  if (ctx->tof.ch_rate_0_3) PIPE_MGR_FREE(ctx->tof.ch_rate_0_3);
  if (ctx->tof.ch_rate_1_3) PIPE_MGR_FREE(ctx->tof.ch_rate_1_3);
  if (ctx->tof.ch_rate_2_3) PIPE_MGR_FREE(ctx->tof.ch_rate_2_3);
  ctx->tof.ch_rate_0_3 = NULL;
  ctx->tof.ch_rate_1_3 = NULL;
  ctx->tof.ch_rate_2_3 = NULL;
  return sts;
}

pipe_status_t pipe_mgr_tof_deprsr_cfg_deinit(bf_dev_id_t dev_id) {
  union pipe_mgr_deprsr_ctx *ctx = pipe_mgr_deprsr_ctx(dev_id);

  if (!ctx) {
    LOG_ERROR("%s: context for dev_id(%d) not found", __func__, dev_id);
    return PIPE_INVALID_ARG;
  }

  if (ctx->tof.ch_rate_0_3) PIPE_MGR_FREE(ctx->tof.ch_rate_0_3);
  if (ctx->tof.ch_rate_1_3) PIPE_MGR_FREE(ctx->tof.ch_rate_1_3);
  if (ctx->tof.ch_rate_2_3) PIPE_MGR_FREE(ctx->tof.ch_rate_2_3);
  ctx->tof.ch_rate_0_3 = NULL;
  ctx->tof.ch_rate_1_3 = NULL;
  ctx->tof.ch_rate_2_3 = NULL;
  return PIPE_SUCCESS;
}
