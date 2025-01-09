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


#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <dvm/bf_drv_intf.h>
#include <lld/lld_err.h>
#include <lld/lld_sku.h>
#include <tofino_regs/tofino.h>
#include <port_mgr/port_mgr_intf.h>
#include <port_mgr/bf_port_if.h>
#include <port_mgr/port_mgr_serdes_sbus_map.h>
#include <port_mgr/port_mgr_ha.h>
#include <port_mgr/port_mgr.h>
#include <port_mgr/port_mgr_map.h>

/*************************************************
 * port_mgr_map_dev_port_to_all
 *
 * Map chip # and bf_dev_port_t to the all corresponding
 * non-NULL values.
 *************************************************/
port_mgr_err_t port_mgr_tof1_map_dev_port_to_all(bf_dev_id_t dev_id,
                                                 bf_dev_port_t dev_port,
                                                 bf_dev_pipe_t *pipe_id,
                                                 bf_dev_port_t *port_id,
                                                 int *mac_block,
                                                 int *ch,
                                                 int *is_cpu_port) {
  uint32_t mac_blk;
  uint32_t channel;
  bf_dev_pipe_t pipe = 0;
  bf_dev_pipe_t logical_pipe_id = DEV_PORT_TO_PIPE(dev_port);
  int port = DEV_PORT_TO_LOCAL_PORT(dev_port);
  lld_err_t err;

  if (!DEV_PORT_VALIDATE(dev_port)) return PORT_MGR_ERR_BAD_PARM;

  err = lld_sku_map_dev_port_id_to_mac_ch(dev_id, dev_port, &mac_blk, &channel);
  if (err != LLD_OK) {
    return PORT_MGR_ERR_BAD_PARM;
  }
  err = lld_sku_map_pipe_id_to_phy_pipe_id(dev_id, logical_pipe_id, &pipe);
  if (err != LLD_OK) {
    return PORT_MGR_ERR_BAD_PARM;
  }

  if (pipe_id) *pipe_id = pipe;
  if (port_id) *port_id = port;
  if (mac_block) *mac_block = mac_blk;
  if (ch) *ch = channel;
  if (is_cpu_port) {
    if (mac_blk == 64 && logical_pipe_id == 0) {
      *is_cpu_port = 1;
    } else {
      *is_cpu_port = 0;
    }
  }
  return PORT_MGR_OK;
}

/*************************************************
 * port_mgr_tof1_map_idx_to_mac_block
 *
 *************************************************/
port_mgr_mac_block_t *port_mgr_tof1_map_idx_to_mac_block(bf_dev_id_t dev_id,
                                                         uint32_t idx) {
  // port_mgr_dev_t *dev_p = port_mgr_map_dev_id_to_dev_p(dev_id);
  port_mgr_tof1_pdev_t *dev_p = port_mgr_dev_physical_dev_get(dev_id);

  if (dev_p == NULL) return NULL;

  return (&dev_p->mac_block[idx]);
}

/*************************************************
 * port_mgr_tof1_map_idx_to_mac_block_allow_unassigned
 *
 *************************************************/
port_mgr_mac_block_t *port_mgr_tof1_map_idx_to_mac_block_allow_unassigned(
    bf_dev_id_t dev_id, uint32_t idx) {
  // port_mgr_dev_t *dev_p =
  // port_mgr_map_dev_id_to_dev_p_allow_unassigned(dev_id);
  port_mgr_tof1_pdev_t *dev_p = port_mgr_dev_physical_dev_get(dev_id);

  if (dev_p == NULL) return NULL;

  return (&dev_p->mac_block[idx]);
}

/*************************************************
 * port_mgr_tof1_map_port_lane_to_serdes
 *
 *************************************************/
port_mgr_serdes_t *port_mgr_tof1_map_port_lane_to_serdes_int(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    uint32_t lane,
    bool allow_unassigned) {
  uint32_t ch, mac_block;
  lld_err_t err;
  port_mgr_port_t *port_p;
  // port_mgr_dev_t *dev_p = port_mgr_map_dev_id_to_dev_p(dev_id);
  port_mgr_tof1_pdev_t *dev_p = port_mgr_dev_physical_dev_get(dev_id);
  int port_id = DEV_PORT_TO_LOCAL_PORT(dev_port);

  if (dev_p == NULL) return NULL;
  if (!DEV_PORT_VALIDATE(dev_port)) return NULL;
  if (lane >= 4) return NULL;
  if (((uint32_t)port_id + lane) > (uint32_t)lld_get_max_cpu_port(dev_id))
    return NULL;

  if (allow_unassigned) {
    port_p = port_mgr_map_dev_port_to_port_allow_unassigned(dev_id, dev_port);
  } else {
    port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  }
  if (port_p == NULL) return NULL;

  err = lld_sku_map_dev_port_id_to_mac_ch(dev_id, dev_port, &mac_block, &ch);
  if (err != LLD_OK) return NULL;
  if ((ch + lane) >= 4) return NULL;

  return (&dev_p->mac_block[mac_block].serdes[ch + lane]);
}

/*************************************************
 * port_mgr_tof1_map_port_lane_to_serdes
 *
 *************************************************/
port_mgr_serdes_t *port_mgr_tof1_map_port_lane_to_serdes(bf_dev_id_t dev_id,
                                                         bf_dev_port_t dev_port,
                                                         uint32_t lane) {
  return port_mgr_tof1_map_port_lane_to_serdes_int(
      dev_id, dev_port, lane, false);
}

/*************************************************
 * port_mgr_tof1_map_port_lane_to_serdes_allow_unassigned
 *
 *************************************************/
port_mgr_serdes_t *port_mgr_tof1_map_port_lane_to_serdes_allow_unassigned(
    bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t lane) {
  return port_mgr_tof1_map_port_lane_to_serdes_int(
      dev_id, dev_port, lane, true);
}

/*************************************************
 * port_mgr_tof1_map_port_lane_to_hw_serdes
 *
 *************************************************/
port_mgr_serdes_t *port_mgr_tof1_map_port_lane_to_hw_serdes_int(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    uint32_t lane,
    bool allow_unassigned) {
  uint32_t ch, mac_block;
  lld_err_t err;
  port_mgr_port_t *port_p;
  // port_mgr_dev_t *dev_p = port_mgr_map_dev_id_to_dev_p(dev_id);
  port_mgr_tof1_pdev_t *dev_p = port_mgr_dev_physical_dev_get(dev_id);
  int port_id = DEV_PORT_TO_LOCAL_PORT(dev_port);

  if (dev_p == NULL) return NULL;
  if (!DEV_PORT_VALIDATE(dev_port)) return NULL;
  if (lane >= 4) return NULL;
  if (((uint32_t)port_id + lane) > (uint32_t)lld_get_max_cpu_port(dev_id))
    return NULL;

  if (allow_unassigned) {
    port_p = port_mgr_map_dev_port_to_port_allow_unassigned(dev_id, dev_port);
  } else {
    port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  }
  if (port_p == NULL) return NULL;

  err = lld_sku_map_dev_port_id_to_mac_ch(dev_id, dev_port, &mac_block, &ch);
  if (err != LLD_OK) return NULL;
  if ((ch + lane) >= 4) return NULL;

  return (&dev_p->mac_block[mac_block].hw_serdes[ch + lane]);
}

/*************************************************
 * port_mgr_tof1_map_port_lane_to_hw_serdes_allow_unassigned
 *
 *************************************************/
port_mgr_serdes_t *port_mgr_tof1_map_port_lane_to_hw_serdes_allow_unassigned(
    bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t lane) {
  return (port_mgr_tof1_map_port_lane_to_hw_serdes_int(
      dev_id, dev_port, lane, true));
}

/*************************************************
 * port_mgr_tof1_map_ring_sd_to_hw_serdes
 *
 * Map chip # and port to the corresponding
 * port_mgr_port_t structure in the LLD context.
 * Returns NULL if "chip", "port", or "lane"
 * exceeds SDK configured max.
 *************************************************/
port_mgr_serdes_t *port_mgr_tof1_map_ring_sd_to_hw_serdes(bf_dev_id_t dev_id,
                                                          uint32_t ring,
                                                          uint32_t sd) {
  uint32_t ip_type;
  int inst, sub_inst;
  // port_mgr_dev_t *dev_p = port_mgr_map_dev_id_to_dev_p(dev_id);
  port_mgr_tof1_pdev_t *dev_p = port_mgr_dev_physical_dev_get(dev_id);

  if (dev_p == NULL) return NULL;

  port_mgr_find_mac_info_for(dev_id, ring, sd, &ip_type, &inst, &sub_inst);
  // make sure not to touch PCIe serdes
  if (ip_type != IP_TYPE_ETH_PMA) return NULL;

  return (&dev_p->mac_block[inst].hw_serdes[sub_inst]);
}

/*
 * Map a given mac_block/lane, return the ethgpio register, index
 * and bit controlling refclk.
 */
bf_status_t port_mgr_map_port_lane_to_gpio_refclk(bf_dev_id_t dev_id,
                                                  bf_dev_port_t dev_port,
                                                  uint32_t lane,
                                                  uint32_t *reg,
                                                  uint32_t *bit) {
  int mac_block, ch, index, base;
  int err;

  if (reg == NULL) return BF_INVALID_ARG;
  if (bit == NULL) return BF_INVALID_ARG;

  err = port_mgr_map_dev_port_to_all(
      dev_id, dev_port, NULL, NULL, &mac_block, &ch, NULL);

  if (err != PORT_MGR_OK) return BF_INVALID_ARG;
  if (mac_block == 64) return BF_INVALID_ARG;  // no control for CPU MAC
  if ((ch + lane) > 3) return BF_INVALID_ARG;

  if ((mac_block < 8) || (mac_block >= 40)) {
    if (mac_block < 8)
      index = 2;
    else if (mac_block < 48)
      index = 0;
    else if (mac_block < 56)
      index = 1;
    else
      index = 3;
    *reg = offsetof(Tofino, ethgpiotl.gpio_common_regs.refclk_select[index]);
  } else {
    if (mac_block < 16)
      index = 0;
    else if (mac_block < 24)
      index = 1;
    else if (mac_block < 32)
      index = 2;
    else
      index = 3;
    *reg = offsetof(Tofino, ethgpiobr.gpio_common_regs.refclk_select[index]);
  }

  // determine base mac_block of range
  if ((mac_block < 8) || (mac_block >= 40)) {
    if (mac_block < 8)
      base = 0;
    else if (mac_block < 48)
      base = 40;
    else if (mac_block < 56)
      base = 48;
    else
      base = 56;
  } else {
    if (mac_block < 16)
      base = 8;
    else if (mac_block < 24)
      base = 16;
    else if (mac_block < 32)
      base = 24;
    else
      base = 32;
  }
  *bit = ((mac_block - base) * 4) + ch + lane;

  return BF_SUCCESS;
}

/*************************************************
 * port_mgr_tof1_map_dev_port_to_port_index
 *
 * Support for the linear mapping of port structures
 * in the logical device.
 *************************************************/
uint32_t port_mgr_tof1_map_dev_port_to_port_index(bf_dev_id_t dev_id,
                                                  bf_dev_port_t dev_port) {
  uint32_t pipe = DEV_PORT_TO_PIPE(dev_port);
  uint32_t port = DEV_PORT_TO_LOCAL_PORT(dev_port);

  (void)dev_id;

  if (!LOCAL_PORT_VALIDATE(port)) return -1;

  if ((port >= 64) & (port <= 67)) {  // cpu port
    return (256 + (port - 64));
  } else {
    return ((pipe * 64) + port);
  }
}
