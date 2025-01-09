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
 * @file pipe_mgr_tof_parb.c
 * @date
 *
 * Implementation of Parser Arbiter related initializations
 */

#include <dvm/bf_drv_intf.h>
#include <tofino_regs/tofino.h>
#include <lld/bf_dma_if.h>

#include "pipe_mgr_int.h"
#include "pipe_mgr_drv.h"

static void parb_prep_port_ingress_chnl_control(rmt_port_info_t *port_info,
                                                uint32_t *val) {
  // Program parb_reg.i_chnl_ctrl
  uint32_t credit;

  *val = 0;

  switch (port_info->speed) {
    case BF_SPEED_40G:
    case BF_SPEED_50G:
      credit = 10;
      break;
    case BF_SPEED_100G:
      credit = 20;
      break;
    case BF_SPEED_1G:
    case BF_SPEED_10G:
      credit = 2;
      break;
    case BF_SPEED_25G:
      credit = 5;
      break;
    default:
      credit = 2;
      break;
  }

  setp_parb_regs_i_chnl_ctrl_prtchl_norm_cred(val, credit);
  setp_parb_regs_i_chnl_ctrl_prtchl_cong_cred(val, credit);
  setp_parb_regs_i_chnl_ctrl_prtchl_norm_dist(
      val, 2);  // keep power on default value
  setp_parb_regs_i_chnl_ctrl_prtchl_cong_dist(
      val, 2);  // keep power on default value
  setp_parb_regs_i_chnl_ctrl_prtchl_ena(val, 1);
}

pipe_status_t pipe_mgr_tof_parb_set_port_ingress_chnl_control(
    rmt_dev_info_t *dev_info, bf_dev_port_t port_id) {
  // Program parb_reg.i_chnl_ctrl
  bf_dev_pipe_t pipe = 0;
  pipe_status_t sts = PIPE_SUCCESS;
  rmt_port_info_t *port_info = NULL;
  uint32_t val = 0;
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);
  bf_dev_id_t dev_id = dev_info->dev_id;

  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port_id);
    return PIPE_INVALID_ARG;
  }
  pipe = port_info->phy_pipe;

  parb_prep_port_ingress_chnl_control(port_info, &val);

  pipe_mgr_write_register(
      dev_id,
      0,
      offsetof(Tofino, pipes[pipe].pmarb.parb_reg.parb_group.i_chnl_ctrl[port]),
      val);
  return (sts);
}

static pipe_status_t parb_prep_port_egress_chnl_control(
    rmt_port_info_t *port_info, uint32_t *val) {
  // Program parb_reg.e_chnl_ctrl
  pipe_status_t sts = PIPE_SUCCESS;
  uint32_t credit;

  *val = 0;

  switch (port_info->speed) {
    case BF_SPEED_40G:
    case BF_SPEED_50G:
      credit = 10;
      break;
    case BF_SPEED_100G:
      credit = 20;
      break;
    case BF_SPEED_1G:
    case BF_SPEED_10G:
      credit = 2;
      break;
    case BF_SPEED_25G:
      credit = 5;
      break;
    default:
      credit = 2;
      break;
  }

  setp_parb_regs_e_chnl_ctrl_prtchl_norm_cred(val, credit);
  setp_parb_regs_e_chnl_ctrl_prtchl_ena(val, 1);

  return (sts);
}

pipe_status_t pipe_mgr_tof_parb_set_port_egress_chnl_control(
    rmt_dev_info_t *dev_info, bf_dev_port_t port_id) {
  bf_dev_pipe_t pipe = 0;
  rmt_port_info_t *port_info = NULL;
  pipe_status_t sts = PIPE_SUCCESS;
  uint32_t val = 0;
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  bf_dev_id_t dev_id = dev_info->dev_id;

  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);

  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port_id);
    return PIPE_INVALID_ARG;
  }
  pipe = port_info->phy_pipe;

  sts = parb_prep_port_egress_chnl_control(port_info, &val);
  pipe_mgr_write_register(
      dev_id,
      0,
      offsetof(Tofino, pipes[pipe].pmarb.parb_reg.parb_group.e_chnl_ctrl[port]),
      val);
  return (sts);
}

pipe_status_t pipe_mgr_tof_parb_enable_port_arb_priority_high(
    rmt_dev_info_t *dev_info, bf_dev_port_t port_id) {
  bf_dev_pipe_t pipe = 0;
  pipe_status_t sts = PIPE_SUCCESS;
  rmt_port_info_t *port_info = NULL;
  uint32_t val = 0;
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  bf_dev_id_t dev_id = dev_info->dev_id;

  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);

  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port_id);
    return PIPE_INVALID_ARG;
  }
  pipe = port_info->phy_pipe;

  parb_prep_port_ingress_chnl_control(port_info, &val);
  setp_parb_regs_e_chnl_ctrl_prtchl_hipri(&val, 1);
  pipe_mgr_write_register(
      dev_id,
      0,
      offsetof(Tofino, pipes[pipe].pmarb.parb_reg.parb_group.i_chnl_ctrl[port]),
      val);
  val = 0;
  sts = parb_prep_port_egress_chnl_control(port_info, &val);
  setp_parb_regs_e_chnl_ctrl_prtchl_hipri(&val, 1);
  pipe_mgr_write_register(
      dev_id,
      0,
      offsetof(Tofino, pipes[pipe].pmarb.parb_reg.parb_group.e_chnl_ctrl[port]),
      val);
  port_info->high_pri = true;

  return (sts);
}

pipe_status_t pipe_mgr_tof_parb_enable_port_arb_priority_normal(
    rmt_dev_info_t *dev_info, bf_dev_port_t port_id) {
  bf_dev_pipe_t pipe = 0;
  pipe_status_t sts = PIPE_SUCCESS;
  rmt_port_info_t *port_info = NULL;
  uint32_t val = 0;
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  bf_dev_id_t dev_id = dev_info->dev_id;

  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);

  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port_id);
    return PIPE_INVALID_ARG;
  }
  pipe = port_info->phy_pipe;

  parb_prep_port_ingress_chnl_control(port_info, &val);
  setp_parb_regs_e_chnl_ctrl_prtchl_hipri(&val, 0);
  pipe_mgr_write_register(
      dev_id,
      0,
      offsetof(Tofino, pipes[pipe].pmarb.parb_reg.parb_group.i_chnl_ctrl[port]),
      val);
  val = 0;
  sts = parb_prep_port_egress_chnl_control(port_info, &val);
  setp_parb_regs_e_chnl_ctrl_prtchl_hipri(&val, 0);
  pipe_mgr_write_register(
      dev_id,
      0,
      offsetof(Tofino, pipes[pipe].pmarb.parb_reg.parb_group.e_chnl_ctrl[port]),
      val);
  port_info->high_pri = false;
  return (sts);
}

pipe_status_t pipe_mgr_tof_parb_disable_chnl_control(rmt_dev_info_t *dev_info,
                                                     bf_dev_port_t port_id) {
  // Program parb_reg.i_chnl_ctrl
  pipe_status_t sts = PIPE_SUCCESS;
  rmt_port_info_t *port_info = NULL;
  bf_dev_pipe_t pipe = 0;
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  uint32_t val = 0;
  bf_dev_id_t dev_id = dev_info->dev_id;

  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);

  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port_id);
    return PIPE_INVALID_ARG;
  }
  pipe = port_info->phy_pipe;

  setp_parb_regs_i_chnl_ctrl_prtchl_ena(&val, 0);
  pipe_mgr_write_register(
      dev_id,
      0,
      offsetof(Tofino, pipes[pipe].pmarb.parb_reg.parb_group.i_chnl_ctrl[port]),
      val);

  return (sts);
}

pipe_status_t pipe_mgr_tof_parb_init(pipe_sess_hdl_t sess_hdl,
                                     rmt_dev_info_t *dev_info) {
  pipe_status_t sts = PIPE_SUCCESS;
  uint32_t val = 0;

  pipe_bitmap_t pipe_bit_map = {{0}};
  PIPE_BITMAP_INIT(&pipe_bit_map, PIPE_BMP_SIZE);
  uint32_t i;
  for (i = 0; i < dev_info->num_active_pipes; ++i) {
    PIPE_BITMAP_SET(&pipe_bit_map, i);
  }

  // set stall threshold to 0x3 (as recommended by HW team)
  // HW recommends 0x6.. (changing from 0x3 to 0x6)
  setp_parb_regs_mau_micro_update_stall_thr(&val, 0x6);
  pipe_instr_write_reg_t instr;
  construct_instr_reg_write(
      dev_info->dev_id,
      &instr,
      offsetof(Tofino, pipes[0].pmarb.parb_reg.parb_group.mau_micro_update),
      val);
  uint32_t stage;
  lld_err_t lld_err = lld_sku_get_prsr_stage(dev_info->dev_id, &stage);
  if (LLD_OK != lld_err) {
    LOG_ERROR("Failed to get parser id at %s", __func__);
    PIPE_MGR_DBGCHK(LLD_OK == lld_err);
    return PIPE_INIT_ERROR;
  }
  sts = pipe_mgr_drv_ilist_add(&sess_hdl,
                               dev_info,
                               &pipe_bit_map,
                               stage,
                               (uint8_t *)&instr,
                               sizeof instr);
  if (PIPE_SUCCESS != sts) {
    LOG_ERROR("Failed to add port-arb init to instruction list (%s)",
              pipe_str_err(sts));
    PIPE_MGR_DBGCHK(PIPE_SUCCESS == sts);
    return sts;
  }
  return (sts);
}
