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
 * @file pipe_mgr_tof_prsr.c
 * @date
 *
 * Implementation/Configuration of Ingress parser, parser-merge and Egress
 *parser based on port speed.
 */

#include <dvm/bf_drv_intf.h>
#include "dvm/dvm.h"
#include <tofino_regs/tofino.h>
#include <lld/bf_dma_if.h>

#include "pipe_mgr_int.h"
#include "pipe_mgr_drv.h"

pipe_status_t pipe_mgr_tof_iprsr_port_speed_based_cfg(rmt_dev_info_t *dev_info,
                                                      bf_dev_port_t port_id) {
  uint32_t parse_front_end_mode = 0;
  bf_dev_port_t first_port_in_pg;
  uint32_t parse_front_end_enable = 0;
  uint32_t val;
  rmt_port_info_t *port_info = NULL, *all_port_info = NULL;
  bf_dev_pipe_t pipe = 0;
  int pg;
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  uint8_t port_speed[TOF_NUM_CHN_PER_PORT], i;
  bf_dev_id_t dev_id = dev_info->dev_id;

  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);
  pg = port / TOF_NUM_CHN_PER_PORT;  // Port group has 4 ports/channels

  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port_id);
    return PIPE_INVALID_ARG;
  }
  pipe = port_info->phy_pipe;

  if ((port_info->speed != BF_SPEED_100G) &&
      (port_info->speed != BF_SPEED_40G)) {
    first_port_in_pg = port_id & 0xFFFFFFFC;
    for (i = 0; i < TOF_NUM_CHN_PER_PORT; i++) {
      all_port_info = pipe_mgr_get_port_info(dev_id, first_port_in_pg + i);
      if (!all_port_info) {
        port_speed[i] = BF_SPEED_10G;  // Assume unpopulated port as 10G
                                       // In this case, it shouldnt matter
        // if this port is later added or 40G/50G..
      } else {
        port_speed[i] = all_port_info->speed;
      }
    }
    // All 50G mode
    if (port_speed[0] == BF_SPEED_50G && port_speed[2] == BF_SPEED_50G) {
      parse_front_end_enable = 0xf;
      parse_front_end_mode = 2;
    }
    // Mix mode
    if (port_speed[0] == BF_SPEED_50G &&
        (port_speed[2] == BF_SPEED_25G || port_speed[2] == BF_SPEED_1G ||
         port_speed[2] == BF_SPEED_10G)) {
      parse_front_end_enable = 0xf;
      parse_front_end_mode = 3;
    }
    // Mix mode
    if (port_speed[2] == BF_SPEED_50G &&
        (port_speed[0] == BF_SPEED_25G || port_speed[0] == BF_SPEED_1G ||
         port_speed[0] == BF_SPEED_10G)) {
      parse_front_end_enable = 0xf;
      parse_front_end_mode = 7;
    }
    // All 25G/10G mode
    if ((port_speed[2] == BF_SPEED_25G || port_speed[2] == BF_SPEED_1G ||
         port_speed[2] == BF_SPEED_10G) &&
        (port_speed[0] == BF_SPEED_25G || port_speed[0] == BF_SPEED_1G ||
         port_speed[0] == BF_SPEED_10G)) {
      parse_front_end_enable = 0xf;
      parse_front_end_mode = 4;
    }
  } else {
    parse_front_end_mode = 1;
    parse_front_end_enable = 0xf;

    val = 0;
    setp_prsr_reg_main_rspec_mode_mode(&val, 4);
    // Set mode to 4 before setting correct mode. Setting to 4
    // (as though all channels were in 25G) will reset data_seq.
    // This is needed because when port transitions from 100G-->100G
    // or 40G-->40G, parser sequence number needs to be reset. Reset
    // happens when port-mode is set to zero.
    pipe_mgr_write_register(
        dev_id,
        0,
        offsetof(Tofino, pipes[pipe].pmarb.ibp18_reg.ibp_reg[pg].prsr_reg.mode),
        val);
    val = 0;
    setp_prsr_reg_merge_rspec_mode_mode(&val, 4);
    pipe_mgr_write_register(
        dev_id, 0, offsetof(Tofino, pipes[pipe].pmarb.prsr_reg.mode[pg]), val);
  }

  val = 0;
  setp_prsr_reg_main_rspec_mode_mode(&val, parse_front_end_mode);
  pipe_mgr_write_register(
      dev_id,
      0,
      offsetof(Tofino, pipes[pipe].pmarb.ibp18_reg.ibp_reg[pg].prsr_reg.mode),
      val);

  val = 0;
  for (i = 0; i < TOF_NUM_CHN_PER_PORT; i++) {
    setp_prsr_reg_main_rspec_enable_enable(
        &val, i, (parse_front_end_enable & (1 << i)) ? 1 : 0);
  }
  pipe_mgr_write_register(
      dev_id,
      0,
      offsetof(Tofino, pipes[pipe].pmarb.ibp18_reg.ibp_reg[pg].prsr_reg.enable),
      val);

  val = 0;
  setp_prsr_reg_merge_rspec_mode_mode(&val, parse_front_end_mode);
  pipe_mgr_write_register(
      dev_id, 0, offsetof(Tofino, pipes[pipe].pmarb.prsr_reg.mode[pg]), val);

  LOG_TRACE(
      "%s: port %d Configured ingress parser mode = 0x%x , enable = 0x%x, "
      "parse-merge mode = 0x%x",
      __func__,
      port,
      parse_front_end_mode,
      parse_front_end_enable,
      parse_front_end_mode);
  return (PIPE_SUCCESS);
}

pipe_status_t pipe_mgr_tof_eprsr_port_speed_based_cfg(rmt_dev_info_t *dev_info,
                                                      bf_dev_port_t port_id) {
  uint32_t parse_front_end_mode = 0;
  bf_dev_port_t first_port_in_pg;
  uint32_t parse_front_end_enable = 0;
  uint32_t val;
  rmt_port_info_t *port_info = NULL, *all_port_info = NULL;
  bf_dev_pipe_t pipe = 0;
  int pg;
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  uint8_t port_speed[4], i;
  bf_dev_id_t dev_id = dev_info->dev_id;

  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);
  pg = port / TOF_NUM_CHN_PER_PORT;  // Port group has 4 ports/channels

  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port_id);
    return PIPE_INVALID_ARG;
  }
  pipe = port_info->phy_pipe;

  if ((port_info->speed != BF_SPEED_100G) &&
      (port_info->speed != BF_SPEED_40G)) {
    first_port_in_pg = port_id & 0xFFFFFFFC;
    for (i = 0; i < TOF_NUM_CHN_PER_PORT; i++) {
      all_port_info = pipe_mgr_get_port_info(dev_id, first_port_in_pg + i);
      if (!all_port_info) {
        port_speed[i] = BF_SPEED_10G;  // Assume unpopulated port as 10G
                                       // In this case, it shouldnt matter
        // if this port is later added or 40G/50G..
      } else {
        port_speed[i] = all_port_info->speed;
      }
    }
    // All 50G mode
    if (port_speed[0] == BF_SPEED_50G && port_speed[2] == BF_SPEED_50G) {
      parse_front_end_enable = 0xf;
      parse_front_end_mode = 2;
    }
    // Mix mode
    if (port_speed[0] == BF_SPEED_50G &&
        (port_speed[2] == BF_SPEED_25G || port_speed[2] == BF_SPEED_1G ||
         port_speed[2] == BF_SPEED_10G)) {
      parse_front_end_enable = 0xf;
      parse_front_end_mode = 3;
    }
    // Mix mode
    if (port_speed[2] == BF_SPEED_50G &&
        (port_speed[0] == BF_SPEED_25G || port_speed[0] == BF_SPEED_1G ||
         port_speed[0] == BF_SPEED_10G)) {
      parse_front_end_enable = 0xf;
      parse_front_end_mode = 7;
    }
    // All 25G/10G mode
    if ((port_speed[2] == BF_SPEED_25G || port_speed[2] == BF_SPEED_1G ||
         port_speed[2] == BF_SPEED_10G) &&
        (port_speed[0] == BF_SPEED_25G || port_speed[0] == BF_SPEED_1G ||
         port_speed[0] == BF_SPEED_10G)) {
      parse_front_end_enable = 0xf;
      parse_front_end_mode = 4;
    }
  } else {
    // Check if port mode transition workaround needs to be applied
    if (bf_drv_check_port_mode_transition_wa(
            dev_id, port_id, port_info->speed)) {
      parse_front_end_mode = 0;
    } else {
      parse_front_end_mode = 1;
    }
    parse_front_end_enable = 0xf;
    val = 0;
    setp_prsr_reg_main_rspec_mode_mode(&val, 4);
    // Set mode to 4 before setting correct mode. Setting to 4
    // (as though all channels were in 25G) will reset data_seq.
    // This is needed because when port transitions from 100G-->100G
    // or 40G-->40G, parser sequence number needs to be reset. Reset
    // happens when port-mode is set to zero.
    pipe_mgr_write_register(
        dev_id,
        0,
        offsetof(Tofino, pipes[pipe].pmarb.ebp18_reg.ebp_reg[pg].prsr_reg.mode),
        val);
  }

  val = 0;
  setp_prsr_reg_main_rspec_mode_mode(&val, parse_front_end_mode);
  pipe_mgr_write_register(
      dev_id,
      0,
      offsetof(Tofino, pipes[pipe].pmarb.ebp18_reg.ebp_reg[pg].prsr_reg.mode),
      val);
  val = 0;
  for (i = 0; i < TOF_NUM_CHN_PER_PORT; i++) {
    setp_prsr_reg_main_rspec_enable_enable(
        &val, i, (parse_front_end_enable & (1 << i)) ? 1 : 0);
  }
  pipe_mgr_write_register(
      dev_id,
      0,
      offsetof(Tofino, pipes[pipe].pmarb.ebp18_reg.ebp_reg[pg].prsr_reg.enable),
      val);
  LOG_TRACE("%s: Port %d, Configured egress parser mode = 0x%x , enable = 0x%x",
            __func__,
            port,
            parse_front_end_mode,
            parse_front_end_enable);
  return (PIPE_SUCCESS);
}

// Port mode transition issue workaround
pipe_status_t pipe_mgr_tof_eprsr_complete_port_mode_transition_wa(
    rmt_dev_info_t *dev_info, bf_dev_port_t port_id) {
  uint32_t parse_front_end_mode = 0;
  uint32_t val;
  rmt_port_info_t *port_info = NULL;
  bf_dev_pipe_t pipe = 0;
  int pg;
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_id);
  bf_dev_id_t dev_id = dev_info->dev_id;

  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);
  pg = port / TOF_NUM_CHN_PER_PORT;  // Port group has 4 ports/channels

  port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port_id);
    return PIPE_INVALID_ARG;
  }
  pipe = port_info->phy_pipe;

  if (!(port_info->speed == BF_SPEED_100G ||
        port_info->speed == BF_SPEED_40G)) {
    LOG_ERROR(
        "%s: port (%d) speed %d is invalid", __func__, port, port_info->speed);
    return PIPE_INVALID_ARG;
  }

  parse_front_end_mode = 1;
  val = 0;

  setp_prsr_reg_main_rspec_mode_mode(&val, parse_front_end_mode);

  pipe_mgr_write_register(
      dev_id,
      0,
      offsetof(Tofino, pipes[pipe].pmarb.ebp18_reg.ebp_reg[pg].prsr_reg.mode),
      val);

  LOG_TRACE("%s: Port %d, Configured egress parser mode = 0x%x",
            __func__,
            port,
            parse_front_end_mode);

  return (PIPE_SUCCESS);
}

pipe_status_t pipe_mgr_tof_iprsr_set_pri_thresh(rmt_dev_info_t *dev_info,
                                                rmt_port_info_t *port_info,
                                                uint32_t val) {
  bf_dev_pipe_t pipe = port_info->phy_pipe;
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_info->port_id);
  bf_dev_id_t dev_id = dev_info->dev_id;

  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);
  int pg = port / TOF_NUM_CHN_PER_PORT;  // Port group has 4 ports/channels

  uint32_t csr = 0;
  // Check port states to reconstruct register value. This allows to use
  // unified approach regardless of warm_init state, because there is no
  // need to read registers. Default to 0.
  for (rmt_port_info_t *p_info = dev_info->port_list; p_info;
       p_info = p_info->next) {
    if (p_info->phy_pipe != pipe) continue;
    if (p_info->port_id == port_info->port_id) continue;
    uint8_t l_port = dev_info->dev_cfg.dev_port_to_local_port(p_info->port_id);
    int p_grp = l_port / TOF_NUM_CHN_PER_PORT;
    if (p_grp != pg) continue;

    setp_prsr_reg_main_rspec_pri_thresh_pri(
        &csr, l_port % 4, p_info->iprsr_pri_thresh);
  }

  setp_prsr_reg_main_rspec_pri_thresh_pri(&csr, port % 4, val);

  pipe_status_t sts = pipe_mgr_write_register(
      dev_id,
      0,
      offsetof(Tofino,
               pipes[pipe].pmarb.ibp18_reg.ibp_reg[pg].prsr_reg.pri_thresh),
      csr);
  if (sts) {
    LOG_ERROR("%s: Unable to read register.", __func__);
    return sts;
  }

  LOG_TRACE("%s: Port %d, Configured ingress parser pririoty threshold = 0x%x",
            __func__,
            port,
            val);

  return (PIPE_SUCCESS);
}

pipe_status_t pipe_mgr_tof_iprsr_get_pri_thresh(rmt_dev_info_t *dev_info,
                                                rmt_port_info_t *port_info,
                                                uint32_t *val) {
  bf_dev_pipe_t pipe = 0;
  int pg;
  uint8_t port = dev_info->dev_cfg.dev_port_to_local_port(port_info->port_id);
  bf_dev_id_t dev_id = dev_info->dev_id;

  if (port >= TOF_NUM_PORTS_PER_PIPE) return (PIPE_INVALID_ARG);
  pg = port / TOF_NUM_CHN_PER_PORT;  // Port group has 4 ports/channels

  pipe = port_info->phy_pipe;

  uint32_t csr = 0;
  pipe_status_t sts = lld_read_register(
      dev_id,
      offsetof(Tofino,
               pipes[pipe].pmarb.ibp18_reg.ibp_reg[pg].prsr_reg.pri_thresh),
      &csr);
  if (sts) {
    LOG_ERROR("%s: Unable to read register.", __func__);
    return sts;
  }

  *val = getp_prsr_reg_main_rspec_pri_thresh_pri(&csr, port % 4);

  return (PIPE_SUCCESS);
}
