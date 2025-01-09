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


#include <target-sys/bf_sal/bf_sys_intf.h>
#include "bf_types/bf_types.h"
#include "lld/lld_reg_if.h"
#include "dvm/bf_drv_intf.h"
#include "dvm/dvm_intf.h"
#include "port_mgr/port_mgr_intf.h"
#include "tof2_regs/tof2_reg_drv.h"
#include "port_mgr/port_mgr.h"
#include "port_mgr/port_mgr_map.h"
#include "port_mgr_tof2_map.h"
#include "port_mgr/port_mgr_log.h"
#include "port_mgr_tof2_port.h"
#include "port_mgr_tof2_umac.h"
#include "port_mgr_tof2_serdes.h"
#include "eth400g_mac_rspec_access.h"
#include "eth100g_reg_rspec_access.h"

static bf_status_t port_mgr_tof2_ha_enable_traffic_iter_fn(
    bf_dev_id_t dev_id, bf_dev_port_t dev_port, void *unused_ud);
static void port_mgr_tof2_compute_delta_changes_this_port(
    bf_dev_id_t dev_id, bf_dev_port_t dev_port);

bf_status_t port_mgr_tof2_ha_disable_traffic(bf_dev_id_t dev_id) {
  uint32_t num_pipes = 0;
  uint32_t mac_addr_step =
      offsetof(tof2_reg, eth400g_p2.eth400g_mac.txff_ctrl[0]) -
      offsetof(tof2_reg, eth400g_p1.eth400g_mac.txff_ctrl[0]);
  uint32_t chan_addr_step =
      offsetof(tof2_reg, eth400g_p1.eth400g_mac.txff_ctrl[1]) -
      offsetof(tof2_reg, eth400g_p1.eth400g_mac.txff_ctrl[0]);
  uint32_t addr_base = offsetof(tof2_reg, eth400g_p1.eth400g_mac.txff_ctrl[0]);

  lld_sku_get_num_active_pipes(dev_id, &num_pipes);
  for (bf_dev_pipe_t log_pipe = 0; log_pipe < num_pipes; ++log_pipe) {
    lld_err_t rc;
    bf_dev_pipe_t phy_pipe;
    rc = lld_sku_map_pipe_id_to_phy_pipe_id(dev_id, log_pipe, &phy_pipe);
    if (rc != LLD_OK) {
      return BF_UNEXPECTED;
    }
    for (int mac = 0; mac < 8; ++mac) {
      for (int ch = 0; ch < 8;) {
        int mac_num = phy_pipe * 8 + mac;
        uint32_t addr =
            addr_base + mac_addr_step * mac_num + chan_addr_step * ch;
        uint32_t txff = 0;
        lld_read_register(dev_id, addr, &txff);
        /* If the channel is not enabled, skip it and move onto the next. */
        uint32_t ena = getp_tof2_eth400g_mac_rspec_txff_ctrl_chnl_ena(&txff);
        if (!ena) {
          ++ch;
          continue;
        }
        /* Since the channel is enabled, check the chnl_mode so we know the next
         * channel to check. */
        uint32_t mode = getp_tof2_eth400g_mac_rspec_txff_ctrl_chnl_mode(&txff);
        if (mode == 0) { /* 400g 8 channel */
          ch += 8;
        } else if (mode == 1) { /* 200g 4 channel */
          ch += 4;
        } else if (mode == 2) { /* 100g 2 channel */
          ch += 2;
        } else { /* Single channel */
          ++ch;
        }

        /* If this is a pipe loopback case leave the channel enabled but place
         * it in flush mode to discard any packets.  We do not want to disable
         * the channel (chnl_ena = 0) as that will internally clear the pipe
         * loopback and if we flip chnl_ena back to 1 the loopback may be
         * restored mid packet if traffic is being forwarded to this port and
         * IPB can see EOP w/o SOP and other errors. */
        if (!getp_tof2_eth400g_mac_rspec_txff_ctrl_txrx_lpbk(&txff)) {
          /* Not pipe loopback, clear channel enable. */
          setp_tof2_eth400g_mac_rspec_txff_ctrl_chnl_ena(&txff, 0);
        }
        setp_tof2_eth400g_mac_rspec_txff_ctrl_tx_flush(&txff, 1);
        lld_write_register(dev_id, addr, txff);
      }
    }
  }

  /* And do it all once more for the CPU port MAC... */
  uint32_t umac;
  bf_dev_port_t dev_port = lld_get_min_cpu_port(dev_id);
  port_mgr_err_t sts = port_mgr_tof2_map_dev_port_to_all(
      dev_id, dev_port, NULL, NULL, &umac, NULL, NULL);
  if (sts != PORT_MGR_OK) {
    return BF_UNEXPECTED;
  }
  for (int ch = 0; ch < 4;) {
    uint32_t txff = 0;
    uint32_t chnl_ena = 0;
    eth100g_reg_rspec_txff_ctrl_chnl_ena_get(
        dev_id, umac, ch, &txff, &chnl_ena, true);
    /* If the channel is not enabled, skip it and move onto the next. */
    if (!chnl_ena) {
      ++ch;
      continue;
    }
    /* Since the channel is enabled, check the chnl_mode so we know the next
     * channel to check. */
    uint32_t mode = 0;
    eth100g_reg_rspec_txff_ctrl_chnl_mode_get(
        dev_id, umac, ch, &txff, &mode, false);

    uint32_t pipe_lpbk = 0;
    eth100g_reg_rspec_txff_ctrl_txrx_lpbk_get(
        dev_id, umac, ch, &txff, &pipe_lpbk, false);
    if (!pipe_lpbk) {
      /* Not pipe loopback, clear channel enable. */
      eth100g_reg_rspec_txff_ctrl_chnl_ena_set(
          dev_id, umac, ch, &txff, 0, false);
    }
    eth100g_reg_rspec_txff_ctrl_tx_flush_set(dev_id, umac, ch, &txff, 1, true);

    // wait for txff_status counts to go to 0
    uint32_t cnt, cnt1, cnt2, max_wait, reg32;
    cnt = cnt1 = cnt2 = -1;
    max_wait = 100;
    reg32 = 0;

    do {
      eth100g_reg_rspec_txff_status_txff_count_get(
          dev_id, umac, ch, &reg32, &cnt, true);
      cnt1 = cnt;
      if (cnt == 0) {
        eth100g_reg_rspec_txff_status_txappfifo_count_get(
            dev_id, umac, ch, &reg32, &cnt, true);
        cnt2 = cnt;
      }
    } while ((cnt != 0) && (--max_wait != 0));

    if (cnt != 0) {
      port_mgr_log(
          "UMAC: %d:%2d:%d : Warning: TxFifo not drained after 100 checks: "
          "txff_cnt=%08x : txappfifo_cnt=%08x",
          dev_id,
          umac,
          ch,
          cnt1,
          cnt2);
    }

    if (mode == 0) { /* 100g 4 channel */
      ch += 4;
    } else if (mode == 1) { /* 50g 2 channel */
      ch += 2;
    } else { /* Single channel */
      ++ch;
    }
  }
  return BF_SUCCESS;
}

bf_status_t port_mgr_tof2_ha_enable_traffic(bf_dev_id_t dev_id) {
  port_mgr_log("HA: port_mgr_tof2_ha_enable_traffic: start ..");
  port_mgr_tof2_port_iter(
      dev_id, port_mgr_tof2_ha_enable_traffic_iter_fn, NULL);
  port_mgr_log("HA: port_mgr_tof2_ha_enable_traffic: end");
  return BF_SUCCESS;
}

bf_status_t port_mgr_tof2_ha_compute_delta_changes(bf_dev_id_t dev_id) {
  if (bf_drv_is_device_virtual(dev_id)) return BF_SUCCESS;
  port_mgr_ldev_t *dev_p = port_mgr_dev_logical_dev_get(dev_id);
  if (dev_p == NULL) return BF_SUCCESS;

  int pipe_id, port_id;
  uint32_t num_pipes = 0;
  bf_dev_port_t dev_port;
  lld_sku_get_num_active_pipes(dev_id, &num_pipes);

  // for each (possible) port_p
  for (pipe_id = 0; pipe_id < (int)num_pipes; pipe_id++) {
    for (port_id = lld_get_min_fp_port(dev_id);
         port_id <= lld_get_max_fp_port(dev_id);
         port_id++) {
      dev_port = MAKE_DEV_PORT(pipe_id, port_id);
      port_mgr_tof2_compute_delta_changes_this_port(dev_id, dev_port);
    }
  }

  // one last time for the CPU port
  for (port_id = lld_get_min_cpu_port(dev_id);
       port_id <= lld_get_max_cpu_port(dev_id);
       port_id++) {
    dev_port = MAKE_DEV_PORT(0, port_id);  // CPU port
    port_mgr_tof2_compute_delta_changes_this_port(dev_id, dev_port);
  }

  /* Once we have computed the delta, set the ha stage to
     PORT_MGR_HA_DELTA_PUSH */
  dev_p->ha_stage = PORT_MGR_HA_DELTA_PUSH;

  return BF_SUCCESS;
}

static bf_status_t port_mgr_tof2_ha_enable_traffic_iter_fn(
    bf_dev_id_t dev_id, bf_dev_port_t dev_port, void *unused_ud) {
  port_mgr_port_t *port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  uint32_t umac, ch;
  bool is_cpu_port;
  int sts;

  if (port_p == NULL) return BF_INVALID_ARG;
  if (port_p->sw.assigned == 0) return BF_SUCCESS;

  if (!port_p->sw.enabled) {
    /* Port is configured but not enabled, do not enable traffic on it. */
    port_mgr_log(
        "HA: %d:%3d: port_mgr_tof2_ha_enable_traffic_iter_fn: port down, "
        "skipping",
        dev_id,
        dev_port);
    return BF_SUCCESS;
  }

  sts = port_mgr_tof2_map_dev_port_to_all(
      dev_id, dev_port, NULL, NULL, &umac, &ch, &is_cpu_port);
  if (sts != PORT_MGR_OK) {
    return BF_SUCCESS;
  }
  port_mgr_log(
      "HA: %d:%3d: port_mgr_tof2_ha_enable_traffic_iter_fn: umac=%d ch=%d",
      dev_id,
      dev_port,
      umac,
      ch);

  if (is_cpu_port) {
#if defined(DEVICE_IS_EMULATOR)
    return BF_SUCCESS;
#endif
    uint32_t txff = 0;
    uint32_t lpbk = 0;
    eth100g_reg_rspec_txff_ctrl_txrx_lpbk_get(
        dev_id, umac, ch, &txff, &lpbk, true);
    if (lpbk) {
      /* Need to toggle the channel enable as well as clear the tx_flush on the
       * port.  The channel enable must toggle in order to send credits to the
       * EBUF. */
      eth100g_reg_rspec_txff_ctrl_chnl_ena_set(
          dev_id, umac, ch, &txff, 0, true);
      eth100g_reg_rspec_txff_ctrl_chnl_ena_set(
          dev_id, umac, ch, &txff, 1, true);
      eth100g_reg_rspec_txff_ctrl_tx_flush_set(
          dev_id, umac, ch, &txff, 0, true);
    } else {
      /* Re-enable the channel to allow traffic to resume.
       * set chnl_ena to 1 while tx_flush is 1 will make the CPU ETH port enter
       * some abnormal state that the future tx fifo flush can not empty the
       * appfifo and some packet truncation can happen.
       * Need to first disable tx_flush then enable the chnl_ena  */
      eth100g_reg_rspec_txff_ctrl_tx_flush_set(
          dev_id, umac, ch, &txff, 0, true);
      eth100g_reg_rspec_txff_ctrl_chnl_ena_set(
          dev_id, umac, ch, &txff, 1, true);
    }
  } else {
    uint32_t txff = 0;
    uint32_t lpbk = 0;
    eth400g_mac_rspec_txff_ctrl_txrx_lpbk_get(
        dev_id, umac, ch, &txff, &lpbk, true);
    if (lpbk) {
      /* In pipe loopback clear the tx_flush and toggle the channel enable.
       * The channel enable must be toggled in order to send credits to the
       * EBUF and tx_flush must be cleared since it was set as part of the
       * disable traffic step. */
      eth400g_mac_rspec_txff_ctrl_chnl_ena_set(
          dev_id, umac, ch, &txff, 0, true);
      eth400g_mac_rspec_txff_ctrl_chnl_ena_set(
          dev_id, umac, ch, &txff, 1, true);
      eth400g_mac_rspec_txff_ctrl_tx_flush_set(
          dev_id, umac, ch, &txff, 0, true);
    } else {
      /* Re-enable the channel to allow traffic to resume. */
      eth400g_mac_rspec_txff_ctrl_chnl_ena_set(
          dev_id, umac, ch, &txff, 1, true);
      eth400g_mac_rspec_txff_ctrl_tx_flush_set(
          dev_id, umac, ch, &txff, 0, true);
    }
  }
  (void)unused_ud;
  return BF_SUCCESS;
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

static void port_mgr_tof2_compute_delta_changes_this_port(
    bf_dev_id_t dev_id, bf_dev_port_t dev_port) {
  int dev_port_idx;
  port_mgr_ldev_t *dev_p = port_mgr_dev_logical_dev_get(dev_id);
  port_mgr_port_t *port_p =
      port_mgr_map_dev_port_to_port_allow_unassigned(dev_id, dev_port);

  if (port_p == NULL) return;
  if (dev_p == NULL) return;

  dev_port_idx = DEV_PORT_TO_BIT_IDX(dev_port);
  if (!BIT_IDX_VALIDATE(dev_port_idx)) return;

  bf_ha_port_reconcile_info_t mac_recon_info = {0};
  bf_ha_port_reconcile_info_t serdes_recon_info = {0};
  bf_ha_port_reconcile_info_per_device_t *final_recon_info =
      &dev_p->port_mgr_port_recon_info;

  mac_recon_info.ca = BF_HA_CA_PORT_NONE;
  if ((port_p->sw.loopback_enabled == true) ||
      (port_p->hw.loopback_enabled == true)) {
    if (port_mgr_tof2_loopbackport_delta_compute(
            dev_id, dev_port, &mac_recon_info)) {
      port_mgr_log(
          "Error : Unable to compute loopback port delta for dev %d dev_port "
          "%d",
          dev_id,
          dev_port);
    }

    final_recon_info->recon_info_per_port[dev_port_idx].dev_id = dev_id;
    final_recon_info->recon_info_per_port[dev_port_idx].dev_port = dev_port;
    final_recon_info->recon_info_per_port[dev_port_idx].ca = mac_recon_info.ca;
    port_mgr_log(
        "Dev : %d : Loopback Port %d : Final Corr Action : %s : ",
        dev_id,
        dev_port,
        bf_ha_ca_str(final_recon_info->recon_info_per_port[dev_port_idx].ca));
    memcpy(
        &final_recon_info->recon_info_per_port[dev_port_idx].port_attrib,
        &mac_recon_info.port_attrib,
        sizeof(
            final_recon_info->recon_info_per_port[dev_port_idx].port_attrib));
    return;
  } else {
    if (port_mgr_tof2_port_delta_compute(dev_id, dev_port, &mac_recon_info)) {
      port_mgr_log(
          "Error : Unable to compute port delta for dev %d dev_port %d",
          dev_id,
          dev_port);
    }
  }

  port_mgr_log("%s:%d Done computing MAC delta for dev %d port %d",
               __func__,
               __LINE__,
               dev_id,
               dev_port);

  serdes_recon_info.ca = BF_HA_CA_PORT_NONE;
  if (port_p->hw.assigned && port_p->sw.enabled &&
      (port_p->hw.lpbk_mode != BF_LPBK_MAC_NEAR) &&
      (port_p->hw.lpbk_mode != BF_LPBK_PIPE)) {
    if (port_mgr_tof2_serdes_delta_compute(
            dev_id, dev_port, &serdes_recon_info)) {
      port_mgr_log(
          "Error : Unable to compute serdes delta for dev %d dev_port %d",
          dev_id,
          dev_port);
    }
  }

  /* Cache the reconcile info in the per device array */
  final_recon_info->recon_info_per_port[dev_port_idx].dev_id = dev_id;
  final_recon_info->recon_info_per_port[dev_port_idx].dev_port = dev_port;
  final_recon_info->recon_info_per_port[dev_port_idx].ca =
      final_corrective_action_get(mac_recon_info.ca, serdes_recon_info.ca);
  port_mgr_log(
      "Dev : %d : Port %d : MAC Corr Action : %s : Serdes Corr Action : %s : "
      "Final Corr Action : %s",
      dev_id,
      dev_port,
      bf_ha_ca_str(mac_recon_info.ca),
      bf_ha_ca_str(serdes_recon_info.ca),
      bf_ha_ca_str(final_recon_info->recon_info_per_port[dev_port_idx].ca));
  memcpy(
      &final_recon_info->recon_info_per_port[dev_port_idx].port_attrib,
      &mac_recon_info.port_attrib,
      sizeof(final_recon_info->recon_info_per_port[dev_port_idx].port_attrib));
}

static void port_mgr_tof2_ha_warm_init_this_port(bf_dev_id_t dev_id,
                                                 bf_dev_port_t dev_port) {
  port_mgr_port_t *port_p;
  int mac_block, ch, is_cpu_port;

  port_p = port_mgr_map_dev_port_to_port_allow_unassigned(dev_id, dev_port);
  if (port_p == NULL) return;

  port_mgr_map_dev_port_to_all(
      dev_id, dev_port, NULL, NULL, &mac_block, &ch, &is_cpu_port);

  // rebuild port_p->hw
  port_mgr_tof2_umac_hw_cfg_get(dev_id, mac_block, ch);
}

static void port_mgr_tof2_ha_warm_init_serdes_this_port(
    bf_dev_id_t dev_id, bf_dev_port_t dev_port) {
  bf_mac_block_lane_map_t lane_map;
  bf_status_t sts;
  uint32_t umac, ln, ch;
  bool is_cpu_port;
  port_mgr_tof2_pdev_t *dev_p = port_mgr_dev_physical_dev_tof2_get(dev_id);
  uint32_t num_lanes;
  port_mgr_port_t *port_p;

  if (!dev_p) return;

  port_mgr_tof2_map_dev_port_to_all(
      dev_id, dev_port, NULL, NULL, &umac, &ch, &is_cpu_port);

  // for tofino2, the serdes lane start number equals the MAC channel number
  // the below code setting on hw_sd[ln + i].rx_inv/tx_inv is incorrect if ch
  // not 0.
  if (ch != 0) {
    port_mgr_log("dev_port %d is not with MAC channel 0", dev_port);
    return;
  }

  lane_map.dev_port = dev_port;
  sts = bf_port_lane_map_get(dev_id, umac, &lane_map);
  if (sts != BF_SUCCESS) {
    port_mgr_log(
        "Unable to get the lane remapper info for dev_port %d, log "
        "mac_block %d : %s (%d)",
        dev_port,
        umac,
        bf_err_str(sts),
        sts);
    return;
  }

  num_lanes = is_cpu_port ? 4 : 8;

  if (is_cpu_port) {
    for (ln = 0; ln < num_lanes; ln++) {
      dev_p->umac3[0].hw_phys_tx_ln[ln] = 3 - lane_map.tx_lane[ln];
      dev_p->umac3[0].hw_phys_rx_ln[ln] = 3 - lane_map.rx_lane[ln];
    }
  } else {
    for (ln = 0; ln < 8; ln++) {
      if (((umac <= 8) && (umac >= 1)) || ((umac <= 24) && (umac >= 17))) {
        dev_p->umac4[umac - 1].hw_phys_tx_ln[ln] = 7 - lane_map.tx_lane[ln];
        dev_p->umac4[umac - 1].hw_phys_rx_ln[ln] = 7 - lane_map.rx_lane[ln];
      } else {
        dev_p->umac4[umac - 1].hw_phys_tx_ln[ln] = lane_map.tx_lane[ln];
        dev_p->umac4[umac - 1].hw_phys_rx_ln[ln] = lane_map.rx_lane[ln];
      }
    }
  }

  bf_dev_port_t dport = dev_port;
  for (ln = 0; ln < num_lanes; ln++, dport++) {
    port_p = port_mgr_map_dev_port_to_port_allow_unassigned(dev_id, dport);
    if (port_p == NULL) {
      port_mgr_log("Error: No port_p for dev_port=%d\n", dport);
      return;
    }

    for (uint32_t i = 0; i < port_p->hw.n_lanes; i++) {
      if (!is_cpu_port) {
        port_mgr_tof2_serdes_rx_polarity_get(
            dev_id, dport, i, &dev_p->umac4[umac - 1].hw_sd[ln + i].rx_inv);
        port_mgr_tof2_serdes_tx_polarity_get(
            dev_id, dport, i, &dev_p->umac4[umac - 1].hw_sd[ln + i].tx_inv);
      } else {
        port_mgr_tof2_serdes_rx_polarity_get(
            dev_id, dport, i, &dev_p->umac3[0].hw_sd[ln + i].rx_inv);
        port_mgr_tof2_serdes_tx_polarity_get(
            dev_id, dport, i, &dev_p->umac3[0].hw_sd[ln + i].tx_inv);
      }
    }
  }
}

static void port_mgr_tof2_ha_warm_init_serdes(bf_dev_id_t dev_id) {
  bf_dev_pipe_t pipe_id;
  int port_id;
  uint32_t num_pipes = 0;
  bf_dev_port_t dev_port;
  bool has_mac;
  int chnl = lld_get_chnls_per_mac(dev_id);
  if (chnl == -1) return;
  lld_sku_get_num_active_pipes(dev_id, &num_pipes);

  for (pipe_id = 0; pipe_id < num_pipes; pipe_id++) {
    for (port_id = lld_get_min_fp_port(dev_id);
         port_id <= lld_get_max_fp_port(dev_id);
         port_id = port_id + chnl) {
      dev_port = MAKE_DEV_PORT(pipe_id, port_id);
      port_mgr_port_t *port_p =
          port_mgr_map_dev_port_to_port_allow_unassigned(dev_id, dev_port);
      if (!port_p) continue;

      /* Do not check on loopback mode. This port_id = port_id + chnl will
       * make the for loop get executed at lane 0 only. Consider the case
       * that port at lane 0 port is loopback but port on other lanes not,
       * we need to ensure those lanes also get fetched. if check on lane 0 and
       * bypass it if it is in loopback mode, then that will make the full mac
       * block get bypassed; those ports at other lanes not in loopback mode
       * will also will get bypassed and unable to fetch on the HW polarity.*/
      // if (port_p->hw.lpbk_mode == BF_LPBK_PIPE) continue;

      has_mac = 0;
      if (bf_port_has_mac(dev_id, dev_port, &has_mac) != BF_SUCCESS) {
        has_mac = 0;
      }

      if (has_mac) {
        port_mgr_tof2_ha_warm_init_serdes_this_port(dev_id, dev_port);
      }
    }
  }

  for (port_id = lld_get_min_cpu_port(dev_id);
       port_id <= lld_get_max_cpu_port(dev_id);
       port_id++) {
    dev_port = MAKE_DEV_PORT(0, port_id);  // CPU port
    port_mgr_tof2_ha_warm_init_serdes_this_port(dev_id, dev_port);
  }
}

static void port_mgr_tof2_ha_warm_init(bf_dev_id_t dev_id) {
  bf_dev_pipe_t pipe_id;
  int port_id;
  uint32_t num_pipes = 0;
  bf_dev_port_t dev_port;

  lld_sku_get_num_active_pipes(dev_id, &num_pipes);

  for (pipe_id = 0; pipe_id < num_pipes; pipe_id++) {
    for (port_id = lld_get_min_fp_port(dev_id);
         port_id <= lld_get_max_fp_port(dev_id);
         port_id++) {
      dev_port = MAKE_DEV_PORT(pipe_id, port_id);
      port_mgr_tof2_ha_warm_init_this_port(dev_id, dev_port);
    }
  }

  for (port_id = lld_get_min_cpu_port(dev_id);
       port_id <= lld_get_max_cpu_port(dev_id);
       port_id++) {
    // Don't use MAKE_DEV_PORT, On 3 pipe SKUs the CPU ports are on pipe 1
    dev_port = port_id;  // CPU port
    port_mgr_tof2_ha_warm_init_this_port(dev_id, dev_port);
  }
}

/** \brief port_mgr_hardware_read
 *         Read the hardare during warm reboot
 *
 * \param dev_id: system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 *
 * \return: BF_SUCCESS     : hardware read successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 *
 */
bf_status_t port_mgr_tof2_ha_hardware_read(bf_dev_id_t dev_id) {
  port_mgr_log("%s:%d:%d", __func__, __LINE__, dev_id);
  /* Simply return for virtual devices */
  if (bf_drv_is_device_virtual(dev_id)) return BF_SUCCESS;

  port_mgr_log(
      "%s:%d : Reading Port MACs for dev %d", __func__, __LINE__, dev_id);
  // Read MAC hardware;
  port_mgr_tof2_ha_warm_init(dev_id);
  port_mgr_log(
      "%s:%d : Reading Port MACs Done for dev %d", __func__, __LINE__, dev_id);

  // Read the serdes info programmed in the hardware
  port_mgr_log("Dev %d : Reading Port Serdes", dev_id);
  port_mgr_tof2_ha_warm_init_serdes(dev_id);
  port_mgr_log("Dev %d : Reading Port Serdes Done", dev_id);

  return BF_SUCCESS;
}
