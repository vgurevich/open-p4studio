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


#include <lld/lld_err.h>
#include <lld/lld_sku.h>
#include <dvm/bf_drv_intf.h>
#include <port_mgr/port_mgr_intf.h>
#include <port_mgr/bf_port_if.h>
#include <port_mgr/port_mgr_serdes_sbus_map.h>
#include <port_mgr/port_mgr_ha.h>
#include <port_mgr/port_mgr.h>
#include <port_mgr/port_mgr_map.h>
#include "lld/lld_efuse.h"
#include "port_mgr/port_mgr_tof2/port_mgr_tof2_serdes.h"

bool port_mgr_tof2_dev_port_is_cpu_port(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port) {
  return dev_port >= lld_get_min_cpu_port(dev_id) &&
         dev_port <= lld_get_max_cpu_port(dev_id);
}

/*************************************************
 * port_mgr_tof2_map_dev_port_to_all
 *
 * Map chip # and bf_dev_port_t to the all corresponding
 * non-NULL values.
 *
 * Tof2 dev port mapping is as follows:
 *
 * dev_port[8:7] dev_port[6:0]
 * Pipe, 0-3     Port, 0-71
 * -------------+-------------+
 *      0       |      0      | PCIe
 *              |      1      | Recir_port_0_chnl_1
 *              |      2-5    | Eth CPU (umac3) OR recirc_port_1 & 2
 *              |      6-7    | Recirc_port_3 (not supported by port_mgr)
 *              |      8-71   | umac1-8
 * -------------+-------------+
 *      1       |      0-7    | Recirc (not supported by port_mgr)
 *              |      8-71   | umac9-16
 * -------------+-------------+
 *      2       |      0-7    | Recirc (not supported by port_mgr)
 *              |      8-71   | umac17-24
 * -------------+-------------+
 *      3       |      0-7    | Recirc (not supported by port_mgr)
 *              |      8-71   | umac25-32
 * -------------+-------------+
 *
 *************************************************/
port_mgr_err_t port_mgr_tof2_map_dev_port_to_all(bf_dev_id_t dev_id,
                                                 bf_dev_port_t dev_port,
                                                 uint32_t *phy_pipe_id,
                                                 uint32_t *port_id,
                                                 uint32_t *umac,
                                                 uint32_t *ch,
                                                 bool *is_cpu_port) {
  lld_err_t rc;
  uint32_t channel, phy_mac_blk;
  uint32_t port = DEV_PORT_TO_LOCAL_PORT(dev_port);

  if (!DEV_PORT_VALIDATE(dev_port)) return PORT_MGR_ERR_BAD_PARM;
  rc = lld_sku_map_dev_port_id_to_mac_ch(
      dev_id, dev_port, &phy_mac_blk, &channel);
  if (rc != LLD_OK) return PORT_MGR_ERR_BAD_PARM;

  if (phy_pipe_id) {
    bf_dev_pipe_t phy_pipe;
    bf_dev_pipe_t log_pipe = DEV_PORT_TO_PIPE(dev_port);
    rc = lld_sku_map_pipe_id_to_phy_pipe_id(dev_id, log_pipe, &phy_pipe);
    if (rc != LLD_OK) return PORT_MGR_ERR;
    *phy_pipe_id = phy_pipe;
  }
  if (port_id) *port_id = port;
  if (umac) *umac = phy_mac_blk;
  if (ch) *ch = channel;
  if (is_cpu_port) {
    *is_cpu_port = port_mgr_tof2_dev_port_is_cpu_port(dev_id, dev_port);
  }

  return PORT_MGR_OK;
}

/*************************************************
 * port_mgr_tof2_map_dev_port_to_port_index
 *
 * Support for the linear mapping of port structures
 * in the logical device.
 *************************************************/
uint32_t port_mgr_tof2_map_dev_port_to_port_index(bf_dev_id_t dev_id,
                                                  bf_dev_port_t dev_port) {
  (void)dev_id;
  uint32_t pipe = DEV_PORT_TO_PIPE(dev_port);
  uint32_t port = DEV_PORT_TO_LOCAL_PORT(dev_port);
  if (!LOCAL_PORT_VALIDATE(port)) return -1;

  if (port_mgr_tof2_dev_port_is_cpu_port(dev_id, dev_port)) {
    return (256 + (port - 2));
  } else if (port < 8) {
    return -1;
  } else {
    return ((pipe * 64) + (port - 8));
  }
}

/*************************************************
 * port_mgr_tof2_map_dev_port_lane_to_serdes
 *
 * Return ptr to serdes struct
 *************************************************/
port_mgr_tof2_serdes_t *port_mgr_tof2_map_dev_port_lane_to_serdes(
    bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t ln) {
  uint32_t umac, ch;
  bool is_cpu_port;
  port_mgr_err_t rc;

  port_mgr_tof2_pdev_t *dev_p = port_mgr_dev_physical_dev_tof2_get(dev_id);

  bf_sys_assert(dev_p != NULL);
  rc = port_mgr_tof2_map_dev_port_to_all(
      dev_id, dev_port, NULL, NULL, &umac, &ch, &is_cpu_port);
  bf_sys_assert(rc == PORT_MGR_OK);
  if (!is_cpu_port) {
    return &dev_p->umac4[(umac - 1)].sd[(ch + ln)];
  } else {
    return &dev_p->umac3[0].sd[(ch + ln)];
  }
}

/*************************************************
 * port_mgr_tof2_map_dev_port_lane_to_umac4
 *
 * Return ptr to umac4 struct
 *************************************************/
port_mgr_umac4_t *port_mgr_tof2_map_dev_port_lane_to_umac4(
    bf_dev_id_t dev_id, bf_dev_port_t dev_port) {
  uint32_t umac, ch;
  bool is_cpu_port;
  port_mgr_err_t rc;

  port_mgr_tof2_pdev_t *dev_p = port_mgr_dev_physical_dev_tof2_get(dev_id);

  bf_sys_assert(dev_p != NULL);
  rc = port_mgr_tof2_map_dev_port_to_all(
      dev_id, dev_port, NULL, NULL, &umac, &ch, &is_cpu_port);
  bf_sys_assert(rc == PORT_MGR_OK);
  if (!is_cpu_port) {
    return &dev_p->umac4[(umac - 1)];
  } else {
    return NULL;  // error
  }
}

/*************************************************
 * port_mgr_tof2_map_dev_port_lane_to_umac3
 *
 * Return ptr to umac3 struct
 *************************************************/
port_mgr_umac3_t *port_mgr_tof2_map_dev_port_lane_to_umac3(
    bf_dev_id_t dev_id, bf_dev_port_t dev_port) {
  uint32_t umac, ch;
  bool is_cpu_port;
  port_mgr_err_t rc;

  port_mgr_tof2_pdev_t *dev_p = port_mgr_dev_physical_dev_tof2_get(dev_id);

  bf_sys_assert(dev_p != NULL);
  rc = port_mgr_tof2_map_dev_port_to_all(
      dev_id, dev_port, NULL, NULL, &umac, &ch, &is_cpu_port);
  bf_sys_assert(rc == PORT_MGR_OK);
  if (!is_cpu_port) {
    return NULL;  // error
  } else {
    return &dev_p->umac3[0];
  }
}
