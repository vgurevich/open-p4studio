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
#include <string.h>

#include <dvm/bf_drv_intf.h>
#include <dvm/dvm_intf.h>
#include <port_mgr/port_mgr_intf.h>
#include <lld/bf_dev_if.h>
#include <lld/bf_dma_dr_id.h>
#include <lld/bf_dma_if.h>
#include <lld/lld_dr_if.h>
#include <lld/lld_err.h>
#include <lld/lld_sku.h>
#include <lld/lld_interrupt_if.h>
#include <bf_types/bf_types.h>
#include <port_mgr/bf_port_if.h>
#include <port_mgr/bf_serdes_if.h>
#include <port_mgr/port_mgr_ha.h>
#include <port_mgr/port_mgr.h>
#include <port_mgr/port_mgr_dev.h>
#include "port_mgr_tof1_port.h"
#include <port_mgr/port_mgr_map.h>
#include <port_mgr/port_mgr_log.h>
#include "port_mgr_tof1_map.h"
#include "port_mgr_mac.h"
#include "port_mgr_serdes.h"

// fwd refs
void port_mgr_tof1_ha_warm_init(bf_dev_id_t dev_id);
bf_status_t port_mgr_dma_create(bf_dev_id_t dev_id);
bf_status_t port_mgr_hardware_read(bf_dev_id_t dev_id);
bf_status_t port_mgr_compute_delta_changes(bf_dev_id_t dev_id);
bf_status_t port_mgr_push_delta_changes(bf_dev_id_t dev_id);
bf_status_t port_mgr_register_port_corr_action(
    bf_dev_id_t dev_id, bf_ha_port_reconcile_info_per_device_t *recon_info);
bf_status_t port_mgr_port_delta_push_done(bf_dev_id_t dev_id);

/*
 * Note:
 *    This function is called only during fast reconfig after
 *    core reset & config replay but before enabling traffic. This is
 *    needed to reinitialize credits to EBUF (through PGR) as core
 *    is reset. Otherwise, packets destined to CPU eth port will
 *    be stuck in TM due to lack of credits.
 */
static bf_status_t port_mgr_tof1_ha_enable_eth_cpu_port(bf_dev_id_t dev_id) {
  port_mgr_log("%s:%d:%d", __func__, __LINE__, dev_id);
  bf_dev_port_t eth_cpu_port;
  bf_status_t sts;

  if (dev_id >= BF_MAX_DEV_COUNT) {
    return BF_INVALID_ARG;
  }

  if (port_mgr_dev_is_locked(dev_id)) {
    /* Do nothing, return */
    return BF_SUCCESS;
  }

  bf_dev_port_t min_cpu_port = lld_get_min_cpu_port(dev_id);
  bf_dev_port_t max_cpu_port = lld_get_max_cpu_port(dev_id);
  port_mgr_log("%s:%d: Iterating on cpu_ports: %d : %d",
               __func__,
               __LINE__,
               min_cpu_port,
               max_cpu_port);
  for (eth_cpu_port = min_cpu_port; eth_cpu_port <= max_cpu_port;
       eth_cpu_port++) {
    sts = port_mgr_port_eth_cpu_port_reset(dev_id, eth_cpu_port);
    port_mgr_log("%s:%d: Trying to reset cpu port: %d - STATUS:%s",
                 __func__,
                 __LINE__,
                 eth_cpu_port,
                 sts == 0 ? "BF_SUCCESS" : "BF_OBJ_NOT_FOUND");
    if (sts == BF_OBJECT_NOT_FOUND) {
      sts = BF_SUCCESS;  // if we have no cpu ports added (eth_cpu_port is
      // last), return success
      port_mgr_log(
          "%s:%d: cpu_port was not found continue...", __func__, __LINE__);
      continue;
    }
    if (sts == BF_SUCCESS) {
      // break because we will hit this place if at least one cpu port was
      // reset. All cpu eth ports connected to the single macblock (64), so
      // in this place macblock 64 was reset already
      port_mgr_log(
          "%s:%d: cpu_port was found and resetted", __func__, __LINE__);
      break;
    }
  }

  return sts;
}

/** \brief Enable RX flow control for the port if it's enabled in config.
 *
 * \param dev_id    : bf_dev_id_t           : system-assigned identifier
 *(0..BF_MAX_DEV_COUNT-1)
 *
 * param dev_port    : bf_dev_port_t           : port identifier
 *
 * \return: nothing
 */
static void port_mgr_tof1_ha_enable_rx_flow_control_this_port(
    bf_dev_id_t dev_id, bf_dev_port_t dev_port) {
  port_mgr_port_t *port_p;

  port_p = port_mgr_map_dev_port_to_port_allow_unassigned(dev_id, dev_port);
  if (port_p == NULL) return;

  // Enable RX flow control if it's enabled in SW config
  if (port_p->sw.link_pause_rx || port_p->sw.pfc_pause_rx) {
    port_mgr_log("%s: HA, Enable RX flow control for dev %d, dev_port %d",
                 __func__,
                 dev_id,
                 dev_port);
    port_mgr_mac_enable_rx_flow_control(dev_id, dev_port);
  }
}

/** \brief Enable RX flow control for enabled ports based on config replay.
 *
 * \param dev_id    : bf_dev_id_t           : system-assigned identifier
 *(0..BF_MAX_DEV_COUNT-1)
 *
 * \return: nothing
 */
static void port_mgr_tof1_ha_enable_rx_flow_control(bf_dev_id_t dev_id) {
  bf_dev_pipe_t pipe_id;
  int port_id;
  uint32_t num_pipes = 0;
  bf_dev_port_t dev_port;
  bool has_mac;

  lld_sku_get_num_active_pipes(dev_id, &num_pipes);

  // for each (possible) port_p
  for (pipe_id = 0; pipe_id < num_pipes; pipe_id++) {
    for (port_id = lld_get_min_fp_port(dev_id);
         port_id <= lld_get_max_fp_port(dev_id);
         port_id++) {
      dev_port = MAKE_DEV_PORT(pipe_id, port_id);
      has_mac = 0;
      if (bf_port_has_mac(dev_id, dev_port, &has_mac) != BF_SUCCESS) {
        has_mac = 0;
      }

      if (has_mac) {
        port_mgr_tof1_ha_enable_rx_flow_control_this_port(dev_id, dev_port);
      }
    }
  }

  // CPU ethernet port
  for (port_id = lld_get_min_cpu_port(dev_id);
       port_id <= lld_get_max_cpu_port(dev_id);
       port_id++) {
    dev_port = MAKE_DEV_PORT(0, port_id);  // CPU eth port on 1st logical pipe
    port_mgr_tof1_ha_enable_rx_flow_control_this_port(dev_id, dev_port);
  }
}

/** \brief Enable some of the port configs to accept and transmit packets
 *         after core reset during fast reconfig
 *
 * \param dev_id    : bf_dev_id_t           : system-assigned identifier
 *(0..BF_MAX_DEV_COUNT-1)
 *
 * \return: bf_status_t    : 0 on success, error codes on failure
 */
bf_status_t port_mgr_tof1_ha_enable_input_packets(bf_dev_id_t dev_id) {
  bf_status_t sts;

  if (dev_id >= BF_MAX_DEV_COUNT) {
    return BF_INVALID_ARG;
  }

  if (port_mgr_dev_is_locked(dev_id)) {
    /* Do nothing, return */
    return BF_SUCCESS;
  }

  // Reset CPU ethernet port to reset EBUF credits for traffic
  sts = port_mgr_tof1_ha_enable_eth_cpu_port(dev_id);

  // Enable RX flow control for ports that have enabled in SW config replay
  // Error checking is not needed
  port_mgr_tof1_ha_enable_rx_flow_control(dev_id);

  return sts;
}

/** \brief Build port configuration by reading directly from hw.
 *
 * \param dev_id    : bf_dev_id_t           : system-assigned identifier
 *(0..BF_MAX_DEV_COUNT-1)
 *
 * \return: nothing
 */
static void port_mgr_tof1_ha_warm_init_this_port(bf_dev_id_t dev_id,
                                                 bf_dev_port_t dev_port) {
  port_mgr_port_t *port_p;
  int mac_block, ch, is_cpu_port;

  port_p = port_mgr_map_dev_port_to_port_allow_unassigned(dev_id, dev_port);
  if (port_p == NULL) return;  // FIXME: needed?

  port_mgr_map_dev_port_to_all(
      dev_id, dev_port, NULL, NULL, &mac_block, &ch, &is_cpu_port);
  // set default software state in port_p->sw
  // port_mgr_set_default_port_state(dev_id, dev_port);

  // rebuild port_p->hw
  port_mgr_mac_hw_cfg_get(dev_id, mac_block, ch);

  // at this point, portP->hw has been updated with what is actually
  // programmed in the MAC registers. This data is used to update
  // a few port_p->hw fields that are software-defined and cannot be
  // read directly, such as "ready".
}

/** \brief Build port configuration by reading directly from hw.
 *
 * \param dev_id    : bf_dev_id_t           : system-assigned identifier
 *(0..BF_MAX_DEV_COUNT-1)
 *
 * \return: nothing
 */
void port_mgr_tof1_ha_warm_init(bf_dev_id_t dev_id) {
  bf_dev_pipe_t pipe_id;
  int port_id;
  uint32_t num_pipes = 0;
  bf_dev_port_t dev_port;

  lld_sku_get_num_active_pipes(dev_id, &num_pipes);

  // for each (possible) port_p
  for (pipe_id = 0; pipe_id < num_pipes; pipe_id++) {
    for (port_id = lld_get_min_fp_port(dev_id);
         port_id <= lld_get_max_fp_port(dev_id);
         port_id++) {
      dev_port = MAKE_DEV_PORT(pipe_id, port_id);
      port_mgr_tof1_ha_warm_init_this_port(dev_id, dev_port);
    }
  }

  for (port_id = lld_get_min_cpu_port(dev_id);
       port_id <= lld_get_max_cpu_port(dev_id);
       port_id++) {
    dev_port = MAKE_DEV_PORT(0, port_id);  // CPU port
    port_mgr_tof1_ha_warm_init_this_port(dev_id, dev_port);
  }
}

/** \brief Build serdes configuration by reading directly from hw.
 *
 * \param dev_id    : bf_dev_id_t           : system-assigned identifier
 *(0..BF_MAX_DEV_COUNT-1)
 *
 * \return: nothing
 */
static void port_mgr_tof1_ha_warm_init_serdes_this_port(
    bf_dev_id_t dev_id, bf_dev_port_t dev_port) {
  bf_mac_block_lane_map_t lane_map = {0};
  uint32_t phy_mac_block, channel, log_mac_block;
  port_mgr_mac_block_t *mac_block_p;
  lld_err_t err;
  bf_status_t sts;

  err = lld_sku_map_dev_port_id_to_mac_ch(
      dev_id, dev_port, &phy_mac_block, &channel);
  if (err) {
    port_mgr_log(
        "Error : Unable to get the phy mac block for dev %d port %d : %d",
        dev_id,
        dev_port,
        err);
    bf_sys_dbgchk(0);
  }
  err = lld_sku_map_phy2log_mac_block(dev_id, phy_mac_block, &log_mac_block);
  if (err) {
    port_mgr_log(
        "Error : Unable to get the log mac block for dev %d phy mac block %d : "
        "%d",
        dev_id,
        phy_mac_block,
        err);
    bf_sys_dbgchk(0);
  }
  lane_map.dev_port = dev_port;
  sts = bf_port_lane_map_get(dev_id, log_mac_block, &lane_map);
  if (sts != BF_SUCCESS) {
    port_mgr_log(
        "Unable to get the lane remapper info for dev_port %d, log "
        "mac_block %d : %s (%d)",
        dev_port,
        log_mac_block,
        bf_err_str(sts),
        sts);
    bf_sys_dbgchk(0);
  }

  mac_block_p = port_mgr_tof1_map_idx_to_mac_block(dev_id, phy_mac_block);
  if (!mac_block_p) bf_sys_assert(0);
  mac_block_p->hw_tx_lane_map[0] = lane_map.tx_lane[0];
  mac_block_p->hw_tx_lane_map[1] = lane_map.tx_lane[1];
  mac_block_p->hw_tx_lane_map[2] = lane_map.tx_lane[2];
  mac_block_p->hw_tx_lane_map[3] = lane_map.tx_lane[3];
  mac_block_p->hw_rx_lane_map[0] = lane_map.rx_lane[0];
  mac_block_p->hw_rx_lane_map[1] = lane_map.rx_lane[1];
  mac_block_p->hw_rx_lane_map[2] = lane_map.rx_lane[2];
  mac_block_p->hw_rx_lane_map[3] = lane_map.rx_lane[3];

  if (port_mgr_serdes_hw_cfg_get(dev_id, dev_port)) {
    port_mgr_log(
        "Error : Unable to get the serdes hw info for dev %d dev_port %d",
        dev_id,
        dev_port);
  }
}

/** \brief Build serdes configuration by reading directly from hw.
 *
 * \param dev_id    : bf_dev_id_t           : system-assigned identifier
 *(0..BF_MAX_DEV_COUNT-1)
 *
 * \return: nothing
 */
static void port_mgr_tof1_ha_warm_init_serdes(bf_dev_id_t dev_id) {
  bf_dev_pipe_t pipe_id;
  int port_id;
  uint32_t num_pipes = 0;
  bf_dev_port_t dev_port;
  bool has_mac;
  int chnl = lld_get_chnls_per_mac(dev_id);
  if (chnl == -1) return;
  lld_sku_get_num_active_pipes(dev_id, &num_pipes);

  /* Read the Quad lane based lane mapper config programmed in the hw */
  for (pipe_id = 0; pipe_id < num_pipes; pipe_id++) {
    for (port_id = lld_get_min_fp_port(dev_id);
         port_id <= lld_get_max_fp_port(dev_id);
         port_id = port_id + chnl) {
      dev_port = MAKE_DEV_PORT(pipe_id, port_id);
      has_mac = 0;
      if (bf_port_has_mac(dev_id, dev_port, &has_mac) != BF_SUCCESS) {
        has_mac = 0;
      }

      if (has_mac) {
        port_mgr_tof1_ha_warm_init_serdes_this_port(dev_id, dev_port);
      }
    }
  }

  for (port_id = lld_get_min_cpu_port(dev_id);
       port_id <= lld_get_max_cpu_port(dev_id);
       port_id = port_id + 4) {
    dev_port = MAKE_DEV_PORT(0, port_id);  // CPU port
    port_mgr_tof1_ha_warm_init_serdes_this_port(dev_id, dev_port);
  }
}

static bf_ha_corrective_action_t final_corrective_action_get(
    bf_ha_corrective_action_t port_ca, bf_ha_corrective_action_t serdes_ca) {
  if ((port_ca >= BF_HA_CA_PORT_MAX) || (serdes_ca >= BF_HA_CA_PORT_MAX)) {
    return BF_HA_CA_PORT_NONE;
  }

  if (port_ca == serdes_ca) return port_ca;

  switch (serdes_ca) { /* Switching on serdes corrective action */
    case BF_HA_CA_PORT_ADD:
    case BF_HA_CA_PORT_ENABLE:
    case BF_HA_CA_PORT_ADD_THEN_ENABLE:
    case BF_HA_CA_PORT_DISABLE:
    case BF_HA_CA_PORT_DELETE_THEN_ADD:
    case BF_HA_CA_PORT_DELETE_THEN_ADD_THEN_ENABLE:
    case BF_HA_CA_PORT_DELETE:
    case BF_HA_CA_PORT_FSM_LINK_MONITORING:
    case BF_HA_CA_PORT_MAX:
      return port_ca; /* Not sure if any of the above conditions ever
                         occur*/
    case BF_HA_CA_PORT_NONE:
      return port_ca;
    case BF_HA_CA_PORT_FLAP:
      switch (port_ca) { /* Switching on port cfg corrective action  */
        case BF_HA_CA_PORT_NONE:
        case BF_HA_CA_PORT_FSM_LINK_MONITORING:
          return serdes_ca;
        case BF_HA_CA_PORT_ADD:
          return port_ca;
        case BF_HA_CA_PORT_ENABLE:
          return serdes_ca;
        case BF_HA_CA_PORT_ADD_THEN_ENABLE:
          return port_ca;
        case BF_HA_CA_PORT_FLAP:
          return port_ca;
        case BF_HA_CA_PORT_DISABLE:
          return port_ca;
        case BF_HA_CA_PORT_DELETE_THEN_ADD:
          return port_ca;
        case BF_HA_CA_PORT_DELETE_THEN_ADD_THEN_ENABLE:
          return port_ca;
        case BF_HA_CA_PORT_DELETE:
          return port_ca;
        default:
          return BF_HA_CA_PORT_NONE;
      }
    default:
      return BF_HA_CA_PORT_NONE;
  }

  return BF_HA_CA_PORT_NONE;
}

/** \brief port_mgr_hardware_read
 *         Read the hardare during warm reboot
 *
 * \param dev_id: bf_dev_id_t: system-assigned identifier
 *(0..BF_MAX_DEV_COUNT-1)
 *
 * \return: BF_SUCCESS     : hardware read successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 *
 */
bf_status_t port_mgr_tof1_ha_hardware_read(bf_dev_id_t dev_id) {
  port_mgr_log("%s:%d:%d", __func__, __LINE__, dev_id);
  /* Simply return for virtual devices */
  if (bf_drv_is_device_virtual(dev_id)) return BF_SUCCESS;
  port_mgr_dev_t *dev_p = port_mgr_map_dev_id_to_dev_p_allow_unassigned(dev_id);

  if (dev_p == NULL) return BF_INVALID_ARG;

  port_mgr_log(
      "%s:%d : Reading Port MACs for dev %d", __func__, __LINE__, dev_id);
  // Read MAC hardware;
  port_mgr_tof1_ha_warm_init(dev_id);
  port_mgr_log(
      "%s:%d : Reading Port MACs Done for dev %d", __func__, __LINE__, dev_id);

  // Read the serdes info programmed in the hardware
  port_mgr_log("Dev %d : Reading Port Serdes", dev_id);
  port_mgr_tof1_ha_warm_init_serdes(dev_id);
  port_mgr_log("Dev %d : Reading Port Serdes Done", dev_id);

  return BF_SUCCESS;
}

/** \brief Disable RX flow control for the port if it's enabled in HW.
 *
 * \param dev_id    : bf_dev_id_t           : system-assigned identifier
 *(0..BF_MAX_DEV_COUNT-1)
 *
 * param dev_port    : bf_dev_port_t           : port identifier
 *
 * \return: nothing
 */
static void port_mgr_tof1_ha_force_disable_rx_flow_control_this_port(
    bf_dev_id_t dev_id, bf_dev_port_t dev_port) {
  port_mgr_port_t *port_p;

  port_p = port_mgr_map_dev_port_to_port_allow_unassigned(dev_id, dev_port);
  if (port_p == NULL) return;

  // Disable RX flow control if it's enabled in HW
  if (port_p->hw.link_pause_rx || port_p->hw.pfc_pause_rx) {
    port_mgr_log("%s: HA, Disable RX flow control for dev %d, dev_port %d",
                 __func__,
                 dev_id,
                 dev_port);
    port_mgr_mac_force_disable_rx_flow_control(dev_id, dev_port);
  }
}

/** \brief Disable RX flow control for enabled ports in HW.
 *
 * \param dev_id    : bf_dev_id_t           : system-assigned identifier
 *(0..BF_MAX_DEV_COUNT-1)
 *
 * \return: nothing
 */
void port_mgr_tof1_ha_force_disable_rx_flow_control(bf_dev_id_t dev_id) {
  bf_dev_pipe_t pipe_id;
  int port_id;
  uint32_t num_pipes = 0;
  bf_dev_port_t dev_port;
  bool has_mac;

  lld_sku_get_num_active_pipes(dev_id, &num_pipes);

  // for each (possible) port_p
  for (pipe_id = 0; pipe_id < num_pipes; pipe_id++) {
    for (port_id = lld_get_min_fp_port(dev_id);
         port_id <= lld_get_max_fp_port(dev_id);
         port_id++) {
      dev_port = MAKE_DEV_PORT(pipe_id, port_id);
      has_mac = 0;
      if (bf_port_has_mac(dev_id, dev_port, &has_mac) != BF_SUCCESS) {
        has_mac = 0;
      }

      if (has_mac) {
        port_mgr_tof1_ha_force_disable_rx_flow_control_this_port(dev_id,
                                                                 dev_port);
      }
    }
  }

  // CPU ethernet port
  for (port_id = lld_get_min_cpu_port(dev_id);
       port_id <= lld_get_max_cpu_port(dev_id);
       port_id++) {
    dev_port = MAKE_DEV_PORT(0, port_id);  // CPU eth port on 1st logical pipe
    port_mgr_tof1_ha_force_disable_rx_flow_control_this_port(dev_id, dev_port);
  }
}

static void port_mgr_tof1_compute_delta_changes_this_port(
    bf_dev_id_t dev_id, bf_dev_port_t dev_port) {
  int dev_port_idx;
  port_mgr_port_t *port_p;
  port_mgr_ldev_t *dev_p = port_mgr_dev_logical_dev_get(dev_id);

  if (dev_p == NULL) return;
  bf_ha_port_reconcile_info_t mac_recon_info = {0};
  bf_ha_port_reconcile_info_t serdes_recon_info = {0};
  bf_ha_port_reconcile_info_per_device_t *final_recon_info =
      &dev_p->port_mgr_port_recon_info;
#ifdef PORT_MGR_HA_UNIT_TESTING
  bf_ha_port_reconcile_info_per_device_t *mac_recon =
      &dev_p->port_mgr_mac_recon_info;
  bf_ha_port_reconcile_info_per_device_t *serdes_recon =
      &dev_p->port_mgr_serdes_recon_info;
#endif

  dev_port_idx = DEV_PORT_TO_BIT_IDX(dev_port);
  if (!BIT_IDX_VALIDATE(dev_port_idx)) return;

  mac_recon_info.ca = BF_HA_CA_PORT_NONE;
  if (port_mgr_tof1_port_delta_compute(dev_id, dev_port, &mac_recon_info)) {
    port_mgr_log("Error : Unable to compute port delta for dev %d dev_port %d",
                 dev_id,
                 dev_port);
  }
  port_mgr_log("%s:%d Done computing MAC delta for dev %d port %d",
               __func__,
               __LINE__,
               dev_id,
               dev_port);
  /* Now compute the delta in the serdes. Note that here we only compute
     the delta for the ports that have actually existed before config replay
     and have been enabled after config replay */
  port_p = port_mgr_map_dev_port_to_port_allow_unassigned(dev_id, dev_port);
  if (port_p == NULL) {
    port_mgr_log("%s:%d Map dev %d port %d to port_p failed",
                 __func__,
                 __LINE__,
                 dev_id,
                 dev_port);
    return;
  }
  serdes_recon_info.ca = BF_HA_CA_PORT_NONE;
  if (port_p->hw.assigned && port_p->sw.enabled &&
      port_p->hw.lpbk_mode != BF_LPBK_MAC_NEAR) {
    if (port_mgr_serdes_delta_compute(dev_id, dev_port, &serdes_recon_info)) {
      port_mgr_log(
          "Error : Unable to compute serdes delta for dev %d dev_port %d",
          dev_id,
          dev_port);
    }
  }
  port_mgr_log("%s:%d Done computing Serdes delta for dev %d port %d",
               __func__,
               __LINE__,
               dev_id,
               dev_port);
#ifdef PORT_MGR_HA_UNIT_TESTING
  mac_recon->recon_info_per_port[dev_port_idx].ca = mac_recon_info.ca;
  serdes_recon->recon_info_per_port[dev_port_idx].ca = serdes_recon_info.ca;
#endif
  /* Cache the reconcile info in the per device array */
  final_recon_info->recon_info_per_port[dev_port_idx].dev_id = dev_id;
  final_recon_info->recon_info_per_port[dev_port_idx].dev_port = dev_port;
  final_recon_info->recon_info_per_port[dev_port_idx].ca =
      final_corrective_action_get(mac_recon_info.ca, serdes_recon_info.ca);
  port_mgr_log(
      "Dev : %d : Port : %d : MAC Corr Action : %d : Serdes Corr Action : %d : "
      "Final Corr Action : %d",
      dev_id,
      dev_port,
      mac_recon_info.ca,
      serdes_recon_info.ca,
      final_recon_info->recon_info_per_port[dev_port_idx].ca);
  memcpy(
      &final_recon_info->recon_info_per_port[dev_port_idx].port_attrib,
      &mac_recon_info.port_attrib,
      sizeof(final_recon_info->recon_info_per_port[dev_port_idx].port_attrib));
}

bf_status_t port_mgr_tof1_ha_compute_delta_changes(bf_dev_id_t dev_id) {
  port_mgr_log("%s:%d:%d", __func__, __LINE__, dev_id);
  /* Simply return for virtual devices */
  if (bf_drv_is_device_virtual(dev_id)) return BF_SUCCESS;
  port_mgr_ldev_t *dev_p = port_mgr_dev_logical_dev_get(dev_id);
  if (dev_p == NULL) return BF_SUCCESS;

  // Read the serdes info programmed in the hardware
  port_mgr_log(
      "%s:%d Dev %d : Reading Port Serdes", __func__, __LINE__, dev_id);
  port_mgr_tof1_ha_warm_init_serdes(dev_id);
  port_mgr_log(
      "%s:%d Dev %d : Reading Port Serdes Done", __func__, __LINE__, dev_id);

  bf_dev_pipe_t pipe_id;
  int port_id;
  uint32_t num_pipes = 0;
  bf_dev_port_t dev_port;
  lld_sku_get_num_active_pipes(dev_id, &num_pipes);

  // for each (possible) port_p
  for (pipe_id = 0; pipe_id < num_pipes; pipe_id++) {
    for (port_id = lld_get_min_fp_port(dev_id);
         port_id <= lld_get_max_fp_port(dev_id);
         port_id++) {
      dev_port = MAKE_DEV_PORT(pipe_id, port_id);
      port_mgr_tof1_compute_delta_changes_this_port(dev_id, dev_port);
    }
  }

  // one last time for the CPU port
  for (port_id = lld_get_min_cpu_port(dev_id);
       port_id <= lld_get_max_cpu_port(dev_id);
       port_id++) {
    dev_port = MAKE_DEV_PORT(0, port_id);  // CPU port
    port_mgr_tof1_compute_delta_changes_this_port(dev_id, dev_port);
  }

  /* Once we have computed the delta, set the ha stage to
     PORT_MGR_HA_DELTA_PUSH */
  dev_p->ha_stage = PORT_MGR_HA_DELTA_PUSH;

  /*
   * For fast reconfig, RX flow control needs to be disabled as core
   * is reset. Otherwise, EBUF<->MAC credits wouldn't be reinitialized
   * causing all TX packets to get stuck in TM.
   *
   * After core reset, RX flow control will be renabled before enabling
   * traffic.
   */
  if (port_mgr_dev_init_mode_get(dev_id) == BF_DEV_WARM_INIT_FAST_RECFG) {
    port_mgr_tof1_ha_force_disable_rx_flow_control(dev_id);
  }

  port_mgr_log("%s:%d Exiting compute delta changes for dev %d",
               __func__,
               __LINE__,
               dev_id);

  return BF_SUCCESS;
}

bf_status_t port_mgr_tof1_ha_push_delta_changes(bf_dev_id_t dev_id) {
  /* Do nothing in this callback since the delta push for the port manager
     would be co-ordinated by the DVM by making the relevant port add/del/
     enb/dis calls */
  (void)dev_id;
  return BF_SUCCESS;
}

/** \brief port_mgr_tof1_port_serdes_upgrade
 *         Mark a Tofino port for serdes upgrade. The new serdes firmware will
 *         be loaded the next time the port is enabled
 *
 * [ PRE_ENABLE ]
 *
 *
 * \param dev_id: int         : system-assigned identifier
 *(0..BF_MAX_DEV_COUNT-1)
 * \param dev_port: int         : physical port # on dev_id (0..LLD_MAX_PORTS-1)
 * \param fw_ver: uint32_t      : firmware version
 * \param fw_path: string       : path to file containing firmware
 *
 * \return: BF_SUCCESS     : port marked for serdes upgrade successfully
 * \return: BF_INVALID_ARG : dev_id never added or dev_id > BF_MAX_DEV_COUNT-1
 * \return: BF_INVALID_ARG : dev_port > LLD_MAX_PORTS-1
 *
 */
bf_status_t port_mgr_tof1_ha_port_serdes_upgrade(bf_dev_id_t dev_id,
                                                 bf_dev_port_t dev_port,
                                                 uint32_t fw_ver,
                                                 char *fw_path) {
  port_mgr_log("%s:%d:%d", __func__, __LINE__, dev_id);
  /* Simply return for virtual devices */
  if (bf_drv_is_device_virtual(dev_id)) return BF_SUCCESS;
  // port_mgr_dev_t *dev_p =
  // port_mgr_map_dev_id_to_dev_p_allow_unassigned(dev_id);
  port_mgr_tof1_pdev_t *dev_p = port_mgr_dev_physical_dev_get(dev_id);

  if (!dev_p) return BF_INVALID_ARG;

  port_mgr_port_t *port_p;
  port_p = port_mgr_map_dev_port_to_port_allow_unassigned(dev_id, dev_port);

  if (!port_p) return BF_INVALID_ARG;

  port_p->serdes_upgrade_required = true;
  dev_p->new_serdes_fw_ver = fw_ver;
  strncpy(dev_p->new_serdes_fw, fw_path, sizeof(dev_p->new_serdes_fw) - 1);

  port_mgr_log("%s:%d Exiting port serdes upgrade for dev %d",
               __func__,
               __LINE__,
               dev_id);
  return BF_SUCCESS;
}
