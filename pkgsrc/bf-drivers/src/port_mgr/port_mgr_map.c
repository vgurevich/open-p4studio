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


#include <dvm/bf_drv_intf.h>
#include <port_mgr/port_mgr_intf.h>
#include <port_mgr/port_mgr.h>
#include <port_mgr/port_mgr_dev.h>
#include <port_mgr/port_mgr_log.h>
#include "port_mgr_tof1/port_mgr_tof1_map.h"
#include "port_mgr_tof2/port_mgr_tof2_map.h"
#include "port_mgr_tof3/port_mgr_tof3_map.h"

/*************************************************
 * port_mgr_map_dev_port_to_all
 *
 * Map dev_id and dev_port_t to the all corresponding
 * non-NULL values.
 *************************************************/
port_mgr_err_t port_mgr_map_dev_port_to_all(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            bf_dev_pipe_t *pipe_id,
                                            bf_dev_port_t *port_id,
                                            int *mac_block,
                                            int *ch,
                                            int *is_cpu_port) {
  if (port_mgr_dev_is_tof1(dev_id)) {
    return port_mgr_tof1_map_dev_port_to_all(
        dev_id, dev_port, pipe_id, port_id, mac_block, ch, is_cpu_port);
  } else if (port_mgr_dev_is_tof2(dev_id)) {
    port_mgr_err_t rc;
    bool __is_cpu_port = false;
    rc = port_mgr_tof2_map_dev_port_to_all(dev_id,
                                           dev_port,
                                           (uint32_t *)pipe_id,
                                           (uint32_t *)port_id,
                                           (uint32_t *)mac_block,
                                           (uint32_t *)ch,
                                           &__is_cpu_port);
    if (rc == 0) {
      if (is_cpu_port) {  // if response requested
        if (__is_cpu_port) {
          *is_cpu_port = true;
        } else {
          *is_cpu_port = false;
        }
      }
    } else {
      // rc will be non-zero if the port is a special port without a MAC.  Make
      // sure is_cpu_port is set in this case also.
      if (is_cpu_port) *is_cpu_port = false;
    }
    return rc;
  } else if (port_mgr_dev_is_tof3(dev_id)) {
    port_mgr_err_t rc;
    bool __is_cpu_port = false;
    rc = port_mgr_tof3_map_dev_port_to_all(dev_id,
                                           dev_port,
                                           (uint32_t *)pipe_id,
                                           (uint32_t *)port_id,
                                           (uint32_t *)mac_block,
                                           (uint32_t *)ch,
                                           &__is_cpu_port);
    if (rc == 0) {
      if (is_cpu_port) {  // if response requested
        if (__is_cpu_port) {
          *is_cpu_port = true;
        } else {
          *is_cpu_port = false;
        }
      }
    } else {
      // rc will be non-zero if the port is a special port without a MAC.  Make
      // sure is_cpu_port is set in this case also.
      if (is_cpu_port) *is_cpu_port = false;
    }
    return rc;
  }
  return PORT_MGR_ERR_BAD_PARM;
}

/*************************************************
 * port_mgr_map_dev_id_to_dev_p
 *
 * Return port_mgr_dev_t associated with dev_id.
 * From that, you can get either logical device
 * (ldev) or physical device (pdev) structures
 *
 * Requires device to have been added.
 *************************************************/
port_mgr_dev_t *port_mgr_map_dev_id_to_dev_p(bf_dev_id_t dev_id) {
  if (dev_id < 0) return NULL;
  if (dev_id >= BF_MAX_DEV_COUNT) return NULL;
  if (!port_mgr_ctx->dev[dev_id].ldev.assigned) return NULL;

  return (&port_mgr_ctx->dev[dev_id]);
}

/*************************************************
 * port_mgr_map_dev_id_to_dev_p_allow_unassigned
 *
 * Return port_mgr_dev_t associated with dev_id.
 * From that, you can get either logical device
 * (ldev) or physical device (pdev) structures
 *
 * Does not require device to have been added.
 *************************************************/
port_mgr_dev_t *port_mgr_map_dev_id_to_dev_p_allow_unassigned(
    bf_dev_id_t dev_id) {
  if (dev_id < 0) return NULL;
  if (dev_id >= BF_MAX_DEV_COUNT) return NULL;

  return (&port_mgr_ctx->dev[dev_id]);
}

/*************************************************
 * port_mgr_map_dev_id_to_ldev_p
 *
 * Return port_mgr_logical_dev_t associated with dev_id.
 * Requires device to have been added.
 *************************************************/
port_mgr_ldev_t *port_mgr_map_dev_id_to_ldev_p(bf_dev_id_t dev_id) {
  port_mgr_dev_t *dev_p = port_mgr_map_dev_id_to_dev_p(dev_id);

  if (dev_p == NULL) return NULL;
  return (&port_mgr_ctx->dev[dev_id].ldev);
}

/*************************************************
 * port_mgr_map_dev_port_to_port_allow_unassigned
 *
 * Map dev_id and dev_port to the corresponding
 * logical port structure (port_mgr_port_t)
 *
 * Does not require port to have been added.
 *************************************************/
port_mgr_port_t *port_mgr_map_dev_port_to_port_allow_unassigned(
    bf_dev_id_t dev_id, bf_dev_port_t dev_port) {
  uint32_t lport_index;
  port_mgr_dev_t *dev_p = port_mgr_map_dev_id_to_dev_p_allow_unassigned(dev_id);

  if (dev_p == NULL) return NULL;

  if (port_mgr_dev_is_tof1(dev_id)) {
    lport_index = port_mgr_tof1_map_dev_port_to_port_index(dev_id, dev_port);
  } else if (port_mgr_dev_is_tof2(dev_id)) {
    lport_index = port_mgr_tof2_map_dev_port_to_port_index(dev_id, dev_port);
  } else if (port_mgr_dev_is_tof3(dev_id)) {
    lport_index = port_mgr_tof3_map_dev_port_to_port_index(dev_id, dev_port);
  } else {
    return NULL;
  }
  if (lport_index == 0xffffffff) return NULL;

  return (&dev_p->ldev.lport[lport_index]);
}

/*************************************************
 * port_mgr_map_dev_port_to_port
 *
 * Map dev_id and dev_port to the corresponding
 * logical port structure (port_mgr_port_t)
 *
 * Requires port to have been added.
 *************************************************/
port_mgr_port_t *port_mgr_map_dev_port_to_port(bf_dev_id_t dev_id,
                                               bf_dev_port_t port) {
  port_mgr_port_t *port_p =
      port_mgr_map_dev_port_to_port_allow_unassigned(dev_id, port);

  if (port_p != NULL) {
    if (port_p->sw.assigned) {
      return (port_p);
    }
  }
  return NULL;
}

/*************************************************
 * port_mgr_map_idx_to_mac_block
 *
 * Map chip # and index to the corresponding
 * port_mgr_mac_block_t structure in the physical
 * device (pdev)
 *************************************************/
port_mgr_mac_block_t *port_mgr_map_idx_to_mac_block(bf_dev_id_t dev_id,
                                                    uint32_t idx) {
  if (port_mgr_dev_is_tof1(dev_id)) {
    return port_mgr_tof1_map_idx_to_mac_block(dev_id, idx);
  } else {
    //
  }
  return NULL;
}

/*************************************************
 * port_mgr_map_idx_to_mac_block_allow_unassigned
 *
 * Map chip # and index to the corresponding
 * port_mgr_mac_block_t structure in the physical
 * device (pdev)
 *************************************************/
port_mgr_mac_block_t *port_mgr_map_idx_to_mac_block_allow_unassigned(
    bf_dev_id_t dev_id, uint32_t idx) {
  if (port_mgr_dev_is_tof1(dev_id)) {
    return port_mgr_tof1_map_idx_to_mac_block_allow_unassigned(dev_id, idx);
  } else {
    //
  }
  return NULL;
}

/*************************************************
 * port_mgr_map_port_lane_to_serdes_int
 *
 * Map dev_id and dev_port to the corresponding
 * physical serdes structure.
 *************************************************/
port_mgr_serdes_t *port_mgr_map_port_lane_to_serdes_int(bf_dev_id_t dev_id,
                                                        bf_dev_port_t dev_port,
                                                        uint32_t lane,
                                                        int allow_unassigned) {
  if (port_mgr_dev_is_tof1(dev_id)) {
    return port_mgr_tof1_map_port_lane_to_serdes_int(
        dev_id, dev_port, lane, allow_unassigned);
  } else {
    //
  }
  return NULL;
}

/*************************************************
 * port_mgr_map_port_lane_to_serdes_allow_unassigned
 *
 * Map dev_id and dev_port to the corresponding
 * physical serdes structure.
 *************************************************/
port_mgr_serdes_t *port_mgr_map_port_lane_to_serdes_allow_unassigned(
    bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t lane) {
  return (port_mgr_map_port_lane_to_serdes_int(dev_id, dev_port, lane, 1));
}

/*************************************************
 * port_mgr_map_port_lane_to_serdes
 *
 * Map dev_id and dev_port to the corresponding
 * physical serdes structure.
 *************************************************/
port_mgr_serdes_t *port_mgr_map_port_lane_to_serdes(bf_dev_id_t dev_id,
                                                    bf_dev_port_t dev_port,
                                                    uint32_t lane) {
  int allow_unassigned = 0;

  if (port_mgr_dev_ha_stage_get(dev_id) != PORT_MGR_HA_NONE) {
    /* Called during warm init. Hence get the unassigned value */
    allow_unassigned = 1;
  }
  return (port_mgr_map_port_lane_to_serdes_int(
      dev_id, dev_port, lane, allow_unassigned));
}

/*************************************************
 * port_mgr_map_port_lane_to_hw_serdes_int
 *
 * Map dev_id and dev_port to the corresponding
 * physical serdes structure used during HA events
 * to recover state
 *************************************************/
port_mgr_serdes_t *port_mgr_map_port_lane_to_hw_serdes_int(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    uint32_t lane,
    int allow_unassigned) {
  if (port_mgr_dev_is_tof1(dev_id)) {
    port_mgr_tof1_map_port_lane_to_hw_serdes_int(
        dev_id, dev_port, lane, allow_unassigned);
  } else {
    //
  }
  return NULL;
}

/*************************************************
 * port_mgr_map_port_lane_to_hw_serdes
 *
 * Map dev_id and dev_port to the corresponding
 * physical serdes structure used during HA events
 * to recover state
 *************************************************/
port_mgr_serdes_t *port_mgr_map_port_lane_to_hw_serdes(bf_dev_id_t dev_id,
                                                       bf_dev_port_t dev_port,
                                                       uint32_t lane) {
  return (port_mgr_map_port_lane_to_hw_serdes_int(dev_id, dev_port, lane, 0));
}

/*************************************************
 * port_mgr_map_port_lane_to_hw_serdes_allow_unassigned
 *
 * Map dev_id and dev_port to the corresponding
 * physical serdes structure used during HA events
 * to recover state
 *************************************************/
port_mgr_serdes_t *port_mgr_map_port_lane_to_hw_serdes_allow_unassigned(
    bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t lane) {
  return (port_mgr_map_port_lane_to_hw_serdes_int(dev_id, dev_port, lane, 1));
}

/** \map dev_port and ch to lane, and adjust dev_port from the lane's dev_port
 *to port's
 *
 */

bf_status_t port_mgr_map_dev_port_to_lane(bf_dev_id_t dev_id,
                                          bf_dev_port_t *dev_port,
                                          int ch,
                                          uint32_t *lane) {
  port_mgr_port_t *port_p;
  bf_port_speed_t speed = BF_SPEED_NONE;
  bf_dev_port_t dev_port_0 = 0;
  int total_lane = 0;

  if (!dev_port) return BF_INVALID_ARG;
  if (!lane) return BF_INVALID_ARG;

  dev_port_0 = *dev_port - ch;
  for (int i = 0; i < 4; i = i + total_lane) {
    bf_dev_port_t curr_dev_port = dev_port_0 + i;
    total_lane = 1;
    port_p =
        port_mgr_map_dev_port_to_port_allow_unassigned(dev_id, curr_dev_port);
    if (!port_p) continue;
    if (port_p->sw.speed != BF_SPEED_NONE) {
      speed = port_p->sw.speed;
      switch (speed) {
        case BF_SPEED_1G:
        case BF_SPEED_10G:
        case BF_SPEED_25G:
          /* Starting from curr_dev_port, 1 dev port belongs to same front panel
           * port So if *dev_port is less than curr_dev_port + total_lane then
           * return with lane number after deriving it from channel value */
          total_lane = 1;
          if (curr_dev_port + total_lane > *dev_port) {
            *lane = 0;
            return BF_SUCCESS;
          }
          break;
        case BF_SPEED_40G:
        case BF_SPEED_100G:
          total_lane = 4;
          *lane = ch;
          *dev_port -= ch;
          return BF_SUCCESS;
        case BF_SPEED_50G:
          total_lane = 2;
          /* Starting from curr_dev_port, 2 dev port belongs to same front panel
           * port So if *dev_port is less than curr_dev_port + total_lane then
           * return with lane number after deriving it from channel value */
          if (curr_dev_port + total_lane > *dev_port) {
            if (ch == 0 || ch == 2) {
              *lane = 0;
              return BF_SUCCESS;
            } else if (ch == 1 || ch == 3) {
              *lane = 1;
              *dev_port -= 1;
              return BF_SUCCESS;
            } else {
              return BF_INVALID_ARG;
            }
          }
          break;
        default:
          return BF_NOT_SUPPORTED;
      }
      speed = BF_SPEED_NONE;
    }
  }
  return BF_NOT_SUPPORTED;
}
