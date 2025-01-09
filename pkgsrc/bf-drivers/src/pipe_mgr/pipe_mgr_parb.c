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
 * @file pipe_mgr_parb.c
 * @date
 *
 * Implementation of Parser Arbiter related initializations
 */

#include "pipe_mgr_int.h"
#include "pipe_mgr_parb.h"
#include "pipe_mgr_tof_parb.h"
#include "pipe_mgr_drv_intf.h"

extern pipe_mgr_ctx_t *pipe_mgr_ctx;

pipe_status_t bf_pipe_enable_port_arb_priority_high(bf_dev_id_t dev_id,
                                                    bf_dev_port_t port_id) {
  pipe_status_t sts;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return PIPE_INVALID_ARG;

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  sts = pipe_mgr_api_enter(shdl);
  if (sts != PIPE_SUCCESS) return sts;

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      sts = pipe_mgr_tof_parb_enable_port_arb_priority_high(dev_info, port_id);
      break;
    default:
      sts = PIPE_INVALID_ARG;
      break;
  }

  pipe_mgr_api_exit(shdl);
  return sts;
}

pipe_status_t bf_pipe_enable_port_arb_priority_normal(bf_dev_id_t dev_id,
                                                      bf_dev_port_t port_id)

{
  pipe_status_t sts;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return PIPE_INVALID_ARG;

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  sts = pipe_mgr_api_enter(shdl);
  if (sts != PIPE_SUCCESS) return sts;

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      sts =
          pipe_mgr_tof_parb_enable_port_arb_priority_normal(dev_info, port_id);
      break;
    default:
      sts = PIPE_INVALID_ARG;
      break;
  }

  pipe_mgr_api_exit(shdl);
  return sts;
}

pipe_status_t pipe_mgr_parb_pps_limit_set(pipe_sess_hdl_t shdl,
                                          rmt_dev_info_t *dev_info,
                                          bf_dev_pipe_t log_pipe,
                                          uint64_t pps) {
  bf_dev_pipe_t start, end;
  if (log_pipe != BF_DEV_PIPE_ALL) {
    if (log_pipe >= dev_info->num_active_pipes) {
      LOG_ERROR("%s: Dev %d pipe %d not valid, only %d pipes present",
                __func__,
                dev_info->dev_id,
                log_pipe,
                dev_info->num_active_pipes);
      return PIPE_INVALID_ARG;
    }
    start = end = log_pipe;
  } else {
    start = 0;
    end = dev_info->num_active_pipes - 1;
  }

  /* Check requested pps against the per-profile max_pps specified by the P4
   * compiler.  Build a bit map of logical pipes the config will be sent to at
   * the same time. */
  pipe_bitmap_t pbm;
  PIPE_BITMAP_INIT(&pbm, dev_info->dev_cfg.num_pipelines);
  for (bf_dev_pipe_t p = start; p <= end; ++p) {
    PIPE_BITMAP_SET(&pbm, p);
    profile_id_t prof_id = 0;
    if (PIPE_SUCCESS !=
        pipe_mgr_pipe_to_profile(dev_info, p, &prof_id, NULL, 0)) {
      /* No profile assigned to this pipe, that is okay. */
      continue;
    }
    rmt_dev_profile_info_t *prof = dev_info->profile_info[prof_id];
    if (prof->pps_limit && prof->pps_limit < pps) {
      LOG_ERROR("Dev %d, logical pipe %d, cannot set pipeline PPS to %" PRIu64
                "; pipe-name %s program-name %s, has a max PPS of %" PRIu64,
                dev_info->dev_id,
                p,
                pps,
                prof->pipeline_name,
                prof->prog_name,
                prof->pps_limit);
      return PIPE_INVALID_ARG;
    }
  }

  /* Check requested PPS against the MAU clock (maximum PPS the chip can do
   * given one packet per clock). */
  uint64_t pps_max = dev_info->clock_speed;
  if (pps > dev_info->clock_speed) {
    LOG_ERROR("Dev %d cannot set PPS limit to %" PRIu64 ", maximum is %" PRIu64,
              dev_info->dev_id,
              pps,
              dev_info->clock_speed);
    return PIPE_INVALID_ARG;
  }

  uint32_t addr[2] = {0, 0}, data = 0;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      /* Implementation pending. */
      return PIPE_NOT_SUPPORTED;
      break;
    case BF_DEV_FAMILY_TOFINO2:
      addr[0] = offsetof(
          tof2_reg, pipes[0].pardereg.pgstnreg.parbreg.left.i_phv_rate_ctrl);
      addr[1] = offsetof(
          tof2_reg, pipes[0].pardereg.pgstnreg.parbreg.right.e_phv_rate_ctrl);
      if (pps != pps_max) {
        /* Use a fixed max credit of 1024 to allow small bursts.
         * Use a fixed interval of 255.
         * Scale the increment value based on percentage of max rate; an
         * increment of 255 would be the maximum rate, 128 would be 50% rate,
         * 192 would be 75% rate, etc. */
        double percentage = (double)pps / (double)pps_max;
        uint8_t inc = 255.0 * percentage + 0.5;
        setp_tof2_parb_left_reg_i_phv_rate_ctrl_max(&data, 0x400);
        setp_tof2_parb_left_reg_i_phv_rate_ctrl_interval(&data, 0xFF);
        setp_tof2_parb_left_reg_i_phv_rate_ctrl_inc(&data, inc);
      } else {
        /* Leave data as zero to disable the rate control. */
      }
      break;
    case BF_DEV_FAMILY_TOFINO3:
      addr[0] = offsetof(
          tof3_reg, pipes[0].pardereg.pgstnreg.parbreg.left.i_phv_rate_ctrl);
      addr[1] = offsetof(
          tof3_reg, pipes[0].pardereg.pgstnreg.parbreg.right.e_phv_rate_ctrl);
      if (pps != pps_max) {
        /* Use a fixed max credit of 1024 to allow small bursts.
         * Use a fixed interval of 255.
         * Scale the increment value based on percentage of max rate; an
         * increment of 255 would be the maximum rate, 128 would be 50% rate,
         * 192 would be 75% rate, etc. */
        double percentage = (double)pps / (double)pps_max;
        uint8_t inc = 255.0 * percentage + 0.5;
        setp_tof3_parb_left_reg_i_phv_rate_ctrl_max(&data, 0x400);
        setp_tof3_parb_left_reg_i_phv_rate_ctrl_interval(&data, 0xFF);
        setp_tof3_parb_left_reg_i_phv_rate_ctrl_inc(&data, inc);
      } else {
        /* Leave data as zero to disable the rate control. */
      }
      break;

    case BF_DEV_FAMILY_UNKNOWN:
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }

  for (size_t i = 0; i < sizeof addr / sizeof addr[0]; ++i) {
    uint32_t stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(addr[i]);
    pipe_instr_write_reg_t instr;
    construct_instr_reg_write(dev_info->dev_id, &instr, addr[i], data);
    pipe_status_t sts = pipe_mgr_drv_ilist_add(
        &shdl, dev_info, &pbm, stage, (uint8_t *)&instr, sizeof instr);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Dev %d Failed to add PPS limit instr (%s)",
                dev_info->dev_id,
                pipe_str_err(sts));
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
      return sts;
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_parb_pps_limit_get(rmt_dev_info_t *dev_info,
                                          bf_dev_pipe_t log_pipe,
                                          uint64_t *max_pps) {
  uint64_t pps = dev_info->clock_speed;
  *max_pps = 0;
  bf_dev_pipe_t start, end;
  if (log_pipe != BF_DEV_PIPE_ALL) {
    if (log_pipe >= dev_info->num_active_pipes) {
      LOG_ERROR("%s: Dev %d pipe %d not valid, only %d pipes present",
                __func__,
                dev_info->dev_id,
                log_pipe,
                dev_info->num_active_pipes);
      return PIPE_INVALID_ARG;
    }
    start = end = log_pipe;
  } else {
    start = 0;
    end = dev_info->num_active_pipes - 1;
  }
  for (bf_dev_pipe_t p = start; p <= end; ++p) {
    bf_dev_pipe_t phy_pipe;
    pipe_status_t s = pipe_mgr_map_pipe_id_log_to_phy(dev_info, p, &phy_pipe);
    if (s != PIPE_SUCCESS) return s;

    uint32_t addr, data;
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        /* Implementation pending. */
        return PIPE_NOT_SUPPORTED;
        break;
      case BF_DEV_FAMILY_TOFINO2: {
        addr = offsetof(
            tof2_reg,
            pipes[phy_pipe].pardereg.pgstnreg.parbreg.left.i_phv_rate_ctrl);
        lld_read_register(dev_info->dev_id, addr, &data);
        uint32_t max = getp_tof2_parb_left_reg_i_phv_rate_ctrl_max(&data);
        uint32_t inc = getp_tof2_parb_left_reg_i_phv_rate_ctrl_inc(&data);
        uint32_t itvl = getp_tof2_parb_left_reg_i_phv_rate_ctrl_interval(&data);
        uint64_t this_pps = 0;
        if (max) {
          if (itvl) {
            double percentage = (double)inc / (double)itvl;
            this_pps = percentage * pps;
          }
        } else {
          /* No rate limit for this pipe. */
          this_pps = pps;
        }
        if (this_pps > *max_pps) *max_pps = this_pps;
      } break;
      case BF_DEV_FAMILY_TOFINO3: {
        addr = offsetof(tof3_reg,
                        pipes[phy_pipe % BF_SUBDEV_PIPE_COUNT]
                            .pardereg.pgstnreg.parbreg.left.i_phv_rate_ctrl);
        lld_subdev_read_register(
            dev_info->dev_id, phy_pipe / BF_SUBDEV_PIPE_COUNT, addr, &data);
        uint32_t max = getp_tof3_parb_left_reg_i_phv_rate_ctrl_max(&data);
        uint32_t inc = getp_tof3_parb_left_reg_i_phv_rate_ctrl_inc(&data);
        uint32_t itvl = getp_tof3_parb_left_reg_i_phv_rate_ctrl_interval(&data);
        uint64_t this_pps = 0;
        if (max) {
          if (itvl) {
            double percentage = (double)inc / (double)itvl;
            this_pps = percentage * pps;
          }
        } else {
          /* No rate limit for this pipe. */
          this_pps = pps;
        }
        if (this_pps > *max_pps) *max_pps = this_pps;
      } break;

      case BF_DEV_FAMILY_UNKNOWN:
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_parb_pps_limit_max_get(rmt_dev_info_t *dev_info,
                                              bf_dev_pipe_t log_pipe,
                                              uint64_t *max_pps) {
  /* By default, the max PPS rate is one per clock. */
  *max_pps = dev_info->clock_speed;

  /* However, the compiler may enforce a lower limit to stay within the power
   * budget.  Check each profile for a lower limit. */
  bf_dev_pipe_t start, end;
  if (log_pipe != BF_DEV_PIPE_ALL) {
    if (log_pipe >= dev_info->num_active_pipes) {
      LOG_ERROR("%s: Dev %d pipe %d not valid, only %d pipes present",
                __func__,
                dev_info->dev_id,
                log_pipe,
                dev_info->num_active_pipes);
      return PIPE_INVALID_ARG;
    }
    start = end = log_pipe;
  } else {
    start = 0;
    end = dev_info->num_active_pipes - 1;
  }
  for (bf_dev_pipe_t p = start; p <= end; ++p) {
    profile_id_t prof_id = 0;
    if (PIPE_SUCCESS !=
        pipe_mgr_pipe_to_profile(dev_info, p, &prof_id, NULL, 0)) {
      /* No profile assigned to this pipe, that is okay. */
      continue;
    }
    rmt_dev_profile_info_t *prof = dev_info->profile_info[prof_id];
    if (prof->pps_limit && prof->pps_limit < *max_pps) {
      *max_pps = prof->pps_limit;
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_parb_pps_limit_reset(pipe_sess_hdl_t shdl,
                                            rmt_dev_info_t *dev_info,
                                            bf_dev_pipe_t log_pipe) {
  pipe_status_t sts;
  bf_dev_pipe_t start, end;
  if (log_pipe != BF_DEV_PIPE_ALL) {
    if (log_pipe >= dev_info->num_active_pipes) {
      LOG_ERROR("%s: Dev %d pipe %d not valid, only %d pipes present",
                __func__,
                dev_info->dev_id,
                log_pipe,
                dev_info->num_active_pipes);
      return PIPE_INVALID_ARG;
    }
    start = end = log_pipe;
  } else {
    start = 0;
    end = dev_info->num_active_pipes - 1;
  }
  for (bf_dev_pipe_t p = start; p <= end; ++p) {
    uint64_t pps = 0;
    sts = pipe_mgr_parb_pps_limit_max_get(dev_info, p, &pps);
    if (sts != PIPE_SUCCESS) return sts;
    sts = pipe_mgr_parb_pps_limit_set(shdl, dev_info, p, pps);
    if (sts != PIPE_SUCCESS) return sts;
  }
  return PIPE_SUCCESS;
}
