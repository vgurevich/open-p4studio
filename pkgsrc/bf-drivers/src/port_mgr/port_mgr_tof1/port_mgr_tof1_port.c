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

#include <target-sys/bf_sal/bf_sys_intf.h>
#include <dvm/bf_drv_intf.h>
#include <dvm/dvm_intf.h>
#include <port_mgr/port_mgr_intf.h>
#include <tofino_regs/tofino.h>
#include <lld/tofino_defs.h>
#include <lld/bf_dma_if.h>
#include <lld/lld_dr_if.h>
#include <lld/lld_err.h>
#include <lld/lld_sku.h>
#include <port_mgr/bf_port_if.h>
#include <port_mgr/bf_serdes_if.h>
#include <port_mgr/port_mgr_ha.h>
#include <port_mgr/port_mgr.h>
#include <port_mgr/port_mgr_dev.h>
#include <port_mgr/port_mgr_map.h>
#include <port_mgr/port_mgr_log.h>
#include "../port_mgr_mac_stats.h"
#include "port_mgr_tof1_map.h"
#include "port_mgr_mac.h"
#include "port_mgr_tof1_port.h"
#include "port_mgr_av_sd.h"
#include "port_mgr_serdes.h"

// for aim_printf
#include <target-utils/uCli/ucli.h>
#include <target-utils/uCli/ucli_argparse.h>
#include <target-utils/uCli/ucli_handler_macros.h>

// The following basic port state-machine is assumed:
//
//                     state
//                     -----

//            +-----> removed
//            |          |  (port_mgr_port_add)
//            |          v
//            +-----> disabled
//            |          |  (port_mgr_port_enable)
//            |          v
//            |       enabled
//            |          |  (port_mgr_port_disable or port_mgr_port_remove)
//            |          v
//            +----------+
//
// The use of port "add" and "remove" semantics facilitates port "breakout"
// and "reverse breakout", by providing explicit allocation of UMAC
// channels and serdes lanes (resources). In order to be successfully "added" in
// a specific mode all of the necessary resources must be available (i.e.
// not already allocated to other ports defined on the same UMAC).
//
//
// Port State          Description
// ----------          -----------
// removed             The initial state of all ports. In this
//                     state the only allowed API is port_mgr_port_add()
//
// disabled            After being added to the system the port may
//                     be configured using any "pre-enable" API. In
//                     this state the serdes output is disabled.
//
// enabled             After being enabled the serdes output is enabled
//                     and a link-partner would "see" a serdes signal.
//                     In this state the port may be configured using
//                     any "post-enable" API.

int port_mgr_ch_reqd_by_speed(bf_dev_id_t dev_id,
                              bf_dev_port_t dev_port,
                              bf_port_speed_t speed);
static int port_mgr_ch_available(bf_dev_id_t dev_id,
                                 bf_dev_port_t dev_port,
                                 bf_port_speed_t speed);
void port_mgr_port_pgm_speed_fec(bf_dev_id_t dev_id,
                                 bf_dev_port_t dev_port,
                                 bf_port_speed_t speed,
                                 bf_fec_type_t fec);
bf_status_t port_mgr_port_set_speed(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port,
                                    bf_port_speed_t speed);
bf_status_t port_mgr_port_disable(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
uint32_t port_mgr_construct_txff_ctrl(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port);
void port_mgr_set_default_port_state(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port);
static bool port_stats_persistent[BF_MAX_DEV_COUNT];

bf_status_t port_mgr_stats_persistent_set(bf_dev_id_t dev_id, bool enable) {
  if (dev_id < 0 || dev_id >= BF_MAX_DEV_COUNT) return BF_INVALID_ARG;
  port_stats_persistent[dev_id] = enable;
  return BF_SUCCESS;
}

bf_status_t port_mgr_stats_persistent_get(bf_dev_id_t dev_id, bool *enable) {
  if (dev_id < 0 || dev_id >= BF_MAX_DEV_COUNT) return BF_INVALID_ARG;
  *enable = port_stats_persistent[dev_id];
  return BF_SUCCESS;
}

char *spd_to_str(bf_port_speed_t speed) {
  switch (speed) {
    case BF_SPEED_100G:
      return "100g";
    case BF_SPEED_50G:
      return "50g";
    case BF_SPEED_40G:
      return "40g";
    case BF_SPEED_25G:
      return "25g";
    case BF_SPEED_10G:
      return "10g";
    case BF_SPEED_1G:
      return "1g";
    default:
      return "invalid";
  }
  return "impossible";
}

char *fec_to_str(bf_fec_type_t fec) {
  switch (fec) {
    case BF_FEC_TYP_REED_SOLOMON:
      return "rs-fec";
    case BF_FEC_TYP_FIRECODE:
      return "fc-fec";
    case BF_FEC_TYP_NONE:
      return "none";
    default:
      return "invalid";
  }
  return "impossible";
}

/**************************************************************************
 * port_mgr_valid_speed_and_ch
 *
 * Return true if {speed, ch} represent a valid configuration.
 **************************************************************************/
static bool port_mgr_valid_speed_and_ch(bf_port_speeds_t speed, int ch) {
  // verify valid speed for channel
  if (((speed == BF_SPEED_100G) || (speed == BF_SPEED_40G)) && (ch != 0)) {
    return false;
  }
  if (speed == BF_SPEED_50G && ((ch != 0) && (ch != 2))) {
    return false;
  }
  return true;
}

/**************************************************************************
 * port_mgr_valid_speed_and_fec
 *
 * Return true if {speed, fec} represent a valid configuration.
 **************************************************************************/
static bool port_mgr_valid_speed_and_fec(bf_port_speeds_t speed,
                                         bf_fec_type_t fec) {
  // validate fec type for speed
  switch (speed) {
    case BF_SPEED_100G:
      // sanity checks for 100g and 40g MLG mode
      if ((fec != BF_FEC_TYP_NONE) && (fec != BF_FEC_TYP_REED_SOLOMON)) {
        return false;
      }
      break;
    case BF_SPEED_40G:
      // sanity checks for 40g mode
      if ((fec != BF_FEC_TYP_NONE) && (fec != BF_FEC_TYP_FIRECODE)) {
        return false;
      }
      break;
    case BF_SPEED_10G:
      if ((fec != BF_FEC_TYP_NONE) && (fec != BF_FEC_TYP_FIRECODE)) {
        return false;
      }
      break;
    case BF_SPEED_1G:
    case BF_SPEED_25G:
    case BF_SPEED_50G:
      break;  // all fec types supported
    case BF_SPEED_400G:
    case BF_SPEED_200G:
    case BF_SPEED_NONE:
      port_mgr_log_error("port_mgr_valid_speed_and_fec: Unsupported speed");
      return false;
      break;
    default:
      return false;  // invalid speed or fec mode
  }
  return true;
}

static int num_mac_chn_consumed(int ch_reqd_mask) {
  int num_chnl = 0;
  while (ch_reqd_mask != 0) {
    if (ch_reqd_mask & 0x01) {
      num_chnl++;
    }
    ch_reqd_mask = ch_reqd_mask >> 1;
  }
  return num_chnl;
}

static void port_mgr_port_attrib_get(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port,
                                     bf_port_attributes_t *port_attrib) {
  port_mgr_port_t *port_p;
  port_p = port_mgr_map_dev_port_to_port_allow_unassigned(dev_id, dev_port);
  if (port_p == NULL) return;

  port_mgr_dev_t *dev_p = port_mgr_map_dev_id_to_dev_p_allow_unassigned(dev_id);
  if (dev_p == NULL) return;

  port_attrib->autoneg_enable = port_p->an_enabled;
  port_attrib->port_speeds = port_p->sw.speed;
  port_attrib->port_fec_types = port_p->sw.fec;
  port_attrib->n_lanes = port_p->sw.n_lanes;
}

static void port_dump_all_config(port_cfg_settings_t *sw,
                                 port_cfg_settings_t *hw) {
  port_mgr_log(
      "assigned                        %d : %d", sw->assigned, hw->assigned);
  port_mgr_log(
      "enabled                         %d : %d", sw->enabled, hw->enabled);
  port_mgr_log("oper_state                      %d : %d",
               sw->oper_state,
               hw->oper_state);
  port_mgr_log("speed                       %5d : %d", sw->speed, hw->speed);
  port_mgr_log("fec                             %d : %d", sw->fec, hw->fec);
  port_mgr_log("fc_corr_en                      %d : %d",
               sw->fc_corr_en,
               hw->fc_corr_en);
  port_mgr_log(
      "fc_ind_en                       %d : %d", sw->fc_ind_en, hw->fc_ind_en);
  port_mgr_log(
      "tx_mtu                      %5d : %-5d", sw->tx_mtu, hw->tx_mtu);
  port_mgr_log(
      "rx_mtu                      %5d : %-5d", sw->rx_mtu, hw->rx_mtu);
  port_mgr_log("ifg                         %5d : %d", sw->ifg, hw->ifg);
  port_mgr_log("preamble_length                %2d : %-2d",
               sw->preamble_length,
               hw->preamble_length);
  port_mgr_log("promiscuous_mode                %d : %d",
               sw->promiscuous_mode,
               hw->promiscuous_mode);
  port_mgr_log("link_pause_tx                   %d : %d",
               sw->link_pause_tx,
               hw->link_pause_tx);
  port_mgr_log("link_pause_rx                   %d : %d",
               sw->link_pause_rx,
               hw->link_pause_rx);
  port_mgr_log("pfc_pause_tx                    %d : %d",
               sw->pfc_pause_tx,
               hw->pfc_pause_tx);
  port_mgr_log("pfc_pause_rx                    %d : %d",
               sw->pfc_pause_rx,
               hw->pfc_pause_rx);
  port_mgr_log("loopback_enabled                %d : %d",
               sw->loopback_enabled,
               hw->loopback_enabled);
  port_mgr_log(
      "dfe_type                        %d : %d", sw->dfe_type, hw->dfe_type);
  port_mgr_log("xoff_pause_time             %5d : %-5d",
               sw->xoff_pause_time,
               hw->xoff_pause_time);
  port_mgr_log("xon_pause_time              %5d : %-5d",
               sw->xon_pause_time,
               hw->xon_pause_time);
  port_mgr_log("txff_trunc_ctrl_size            %d : %d",
               sw->txff_trunc_ctrl_size,
               hw->txff_trunc_ctrl_size);
  port_mgr_log("txff_trunc_ctrl_en              %d : %d",
               sw->txff_trunc_ctrl_en,
               hw->txff_trunc_ctrl_en);
  port_mgr_log("txff_ctrl_crc_check_disable     %d : %d",
               sw->txff_ctrl_crc_check_disable,
               hw->txff_ctrl_crc_check_disable);
  port_mgr_log("txff_ctrl_crc_removal_disable   %d : %d",
               sw->txff_ctrl_crc_removal_disable,
               hw->txff_ctrl_crc_removal_disable);
  port_mgr_log("txff_ctrl_fcs_insert_disable    %d : %d",
               sw->txff_ctrl_fcs_insert_disable,
               hw->txff_ctrl_fcs_insert_disable);
  port_mgr_log("txff_ctrl_pad_disable           %d : %d",
               sw->txff_ctrl_pad_disable,
               hw->txff_ctrl_pad_disable);
  port_mgr_log(
      "mac_addr        %02x:%02x:%02x:%02x:%02x:%02x :  "
      "%02x:%02x:%02x:%02x:%02x:%02x\n",
      sw->mac_addr[0],
      sw->mac_addr[1],
      sw->mac_addr[2],
      sw->mac_addr[3],
      sw->mac_addr[4],
      sw->mac_addr[5],
      hw->mac_addr[0],
      hw->mac_addr[1],
      hw->mac_addr[2],
      hw->mac_addr[3],
      hw->mac_addr[4],
      hw->mac_addr[5]);
  port_mgr_log(
      "fc_dst_mac_addr %02x:%02x:%02x:%02x:%02x:%02x :  "
      "%02x:%02x:%02x:%02x:%02x:%02x\n",
      sw->fc_dst_mac_addr[0],
      sw->fc_dst_mac_addr[1],
      sw->fc_dst_mac_addr[2],
      sw->fc_dst_mac_addr[3],
      sw->fc_dst_mac_addr[4],
      sw->fc_dst_mac_addr[5],
      hw->fc_dst_mac_addr[0],
      hw->fc_dst_mac_addr[1],
      hw->fc_dst_mac_addr[2],
      hw->fc_dst_mac_addr[3],
      hw->fc_dst_mac_addr[4],
      hw->fc_dst_mac_addr[5]);
  port_mgr_log(
      "fc_src_mac_addr %02x:%02x:%02x:%02x:%02x:%02x :  "
      "%02x:%02x:%02x:%02x:%02x:%02x\n",
      sw->fc_src_mac_addr[0],
      sw->fc_src_mac_addr[1],
      sw->fc_src_mac_addr[2],
      sw->fc_src_mac_addr[3],
      sw->fc_src_mac_addr[4],
      sw->fc_src_mac_addr[5],
      hw->fc_src_mac_addr[0],
      hw->fc_src_mac_addr[1],
      hw->fc_src_mac_addr[2],
      hw->fc_src_mac_addr[3],
      hw->fc_src_mac_addr[4],
      hw->fc_src_mac_addr[5]);
}

static bool port_misc_params_change_detected(port_cfg_settings_t *hw,
                                             port_cfg_settings_t *sw) {
  uint32_t i = 0;
  if (sw->fc_corr_en != hw->fc_corr_en) {
    port_mgr_log("%s:%d: Change detected in fc_corr_en : %d(SW) : %d(HW)",
                 __func__,
                 __LINE__,
                 sw->fc_corr_en,
                 hw->fc_corr_en);
    return true;
  }
  if (sw->fc_ind_en != hw->fc_ind_en) {
    port_mgr_log("%s:%d: Change detected in fc_ind_en : %d(SW) : %d(HW)",
                 __func__,
                 __LINE__,
                 sw->fc_ind_en,
                 hw->fc_ind_en);
    return true;
  }
  if (sw->tx_mtu != hw->tx_mtu) {
    port_mgr_log("%s:%d: Change detected in tx_mtu : %d(SW) : %d(HW)",
                 __func__,
                 __LINE__,
                 sw->tx_mtu,
                 hw->tx_mtu);
    return true;
  }
  if (sw->rx_mtu != hw->rx_mtu) {
    port_mgr_log("%s:%d: Change detected in rx_mtu : %d(SW) : %d(HW)",
                 __func__,
                 __LINE__,
                 sw->rx_mtu,
                 hw->rx_mtu);
    return true;
  }
  if (sw->ifg != hw->ifg) {
    port_mgr_log("%s:%d: Change detected in ifg : %d(SW) : %d(HW)",
                 __func__,
                 __LINE__,
                 sw->ifg,
                 hw->ifg);
    return true;
  }
  if (sw->ipg != hw->ipg) {
    port_mgr_log("%s:%d: Change detected in ipg : %d(SW) : %d(HW)",
                 __func__,
                 __LINE__,
                 sw->ipg,
                 hw->ipg);
    return true;
  }
  if (memcmp(sw->preamble, hw->preamble, sizeof(sw->preamble)) != 0) {
    for (i = 0; i < sizeof(sw->preamble); i++) {
      port_mgr_log("%s:%d: Change detected in preamble : %d(SW) : %d(HW)",
                   __func__,
                   __LINE__,
                   sw->preamble[i],
                   hw->preamble[i]);
    }
    return true;
  }
  if (sw->preamble_length != hw->preamble_length) {
    port_mgr_log("%s:%d: Change detected in preamble_length : %d(SW) : %d(HW)",
                 __func__,
                 __LINE__,
                 sw->preamble_length,
                 hw->preamble_length);
    return true;
  }
  if (sw->promiscuous_mode != hw->promiscuous_mode) {
    port_mgr_log("%s:%d: Change detected in promiscuous_mode : %d(SW) : %d(HW)",
                 __func__,
                 __LINE__,
                 sw->promiscuous_mode,
                 hw->promiscuous_mode);
    return true;
  }
  if (memcmp(sw->mac_addr, hw->mac_addr, sizeof(sw->mac_addr)) != 0) {
    for (i = 0; i < sizeof(sw->mac_addr); i++) {
      port_mgr_log("%s:%d: Change detected in mac_addr : %d(SW) : %d(HW)",
                   __func__,
                   __LINE__,
                   sw->mac_addr[i],
                   hw->mac_addr[i]);
    }
    return true;
  }
  if (sw->loopback_enabled != hw->loopback_enabled) {
    port_mgr_log("%s:%d: Change detected in loopback_enabled : %d(SW) : %d(HW)",
                 __func__,
                 __LINE__,
                 sw->loopback_enabled,
                 hw->loopback_enabled);
    return true;
  }
  if (sw->dfe_type != hw->dfe_type) {
    port_mgr_log("%s:%d: Change detected in dfe_type : %d(SW) : %d(HW)",
                 __func__,
                 __LINE__,
                 sw->dfe_type,
                 hw->dfe_type);
    return true;
  }

  if (sw->link_pause_tx != hw->link_pause_tx) {
    port_mgr_log("%s:%d: Change detected in link_pause_tx : %d(SW) : %d(HW)",
                 __func__,
                 __LINE__,
                 sw->link_pause_tx,
                 hw->link_pause_tx);
    return true;
  }

  /*
   * For PFC TX enable, all priorities get enabled in HW by default
   * as per Comira. So, instead of checking for whole PFC bitmap
   * between HW & SW config, just check if it's enabled or disabled.
   */
  if ((sw->pfc_pause_tx && !hw->pfc_pause_tx) ||
      (!sw->pfc_pause_tx && hw->pfc_pause_tx)) {
    port_mgr_log("%s:%d: Change detected in pfc_pause_tx", __func__, __LINE__);
    port_mgr_log("%s:%d: pfc_pause_tx : %d(SW) : %d(HW)",
                 __func__,
                 __LINE__,
                 sw->pfc_pause_tx,
                 hw->pfc_pause_tx);
    return true;
  }

  /*
   * For PFC RX and link pause RX config, same register is used in HW and
   * also there is no support for per priority PFC RX config. So, if RX flow
   * control is enabled in HW, it's fine if either of PFC RX & link pause RX
   * is enabled in SW. If it's disabled in HW, both PFC RX & link pause RX
   * should be disabled in SW.
   */
  if ((hw->pfc_pause_rx || hw->link_pause_rx) &&
      (!sw->pfc_pause_rx && !sw->link_pause_rx)) {
    port_mgr_log(
        "%s:%d: Change detected : Atleast one of PFC and LINK pause_rx enabled "
        "in hw but neither enabled in sw",
        __func__,
        __LINE__);
    port_mgr_log("%s:%d: pfc_pause_rx : %d(SW) : %d(HW)",
                 __func__,
                 __LINE__,
                 sw->pfc_pause_rx,
                 hw->pfc_pause_rx);
    port_mgr_log("%s:%d: link_pause_rx : %d(SW) : %d(HW)",
                 __func__,
                 __LINE__,
                 sw->link_pause_rx,
                 hw->link_pause_rx);
    return true;
  }
  if ((!hw->pfc_pause_rx && !hw->link_pause_rx) &&
      (sw->pfc_pause_rx || sw->link_pause_rx)) {
    port_mgr_log(
        "%s:%d: Change detected : PFC and LINK pause_rx both disabled in hw "
        "but atleast one enabled in sw",
        __func__,
        __LINE__);
    port_mgr_log("%s:%d: pfc_pause_rx : %d(SW) : %d(HW)",
                 __func__,
                 __LINE__,
                 sw->pfc_pause_rx,
                 hw->pfc_pause_rx);
    port_mgr_log("%s:%d: link_pause_rx : %d(SW) : %d(HW)",
                 __func__,
                 __LINE__,
                 sw->link_pause_rx,
                 hw->link_pause_rx);
    return true;
  }

  if (sw->xoff_pause_time != hw->xoff_pause_time) {
    port_mgr_log("%s:%d: Change detected in xoff_pause_time : %d(SW) : %d(HW)",
                 __func__,
                 __LINE__,
                 sw->xoff_pause_time,
                 hw->xoff_pause_time);
    return true;
  }
  if (sw->xon_pause_time != hw->xon_pause_time) {
    port_mgr_log("%s:%d: Change detected in xon_pause_time : %d(SW) : %d(HW)",
                 __func__,
                 __LINE__,
                 sw->xon_pause_time,
                 hw->xon_pause_time);
    return true;
  }
  if (memcmp(sw->fc_src_mac_addr,
             hw->fc_src_mac_addr,
             sizeof(sw->fc_src_mac_addr)) != 0) {
    for (i = 0; i < sizeof(sw->fc_src_mac_addr); i++) {
      port_mgr_log(
          "%s:%d: Change detected in fc_src_mac_addr : %d(SW) : %d(HW)",
          __func__,
          __LINE__,
          sw->fc_src_mac_addr[i],
          hw->fc_src_mac_addr[i]);
    }
    return true;
  }
  if (memcmp(sw->fc_dst_mac_addr,
             hw->fc_dst_mac_addr,
             sizeof(sw->fc_dst_mac_addr)) != 0) {
    for (i = 0; i < sizeof(sw->fc_dst_mac_addr); i++) {
      port_mgr_log(
          "%s:%d: Change detected in fc_dst_mac_addr : %d(SW) : %d(HW)",
          __func__,
          __LINE__,
          sw->fc_dst_mac_addr[i],
          hw->fc_dst_mac_addr[i]);
    }
    return true;
  }
  if (sw->txff_trunc_ctrl_size != hw->txff_trunc_ctrl_size) {
    port_mgr_log(
        "%s:%d: Change detected in txff_trunc_ctrl_size : %d(SW) : %d(HW)",
        __func__,
        __LINE__,
        sw->txff_trunc_ctrl_size,
        hw->txff_trunc_ctrl_size);
    return true;
  }
  if (sw->txff_trunc_ctrl_en != hw->txff_trunc_ctrl_en) {
    port_mgr_log(
        "%s:%d: Change detected in txff_trunc_ctrl_en : %d(SW) : %d(HW)",
        __func__,
        __LINE__,
        sw->txff_trunc_ctrl_en,
        hw->txff_trunc_ctrl_en);
    return true;
  }
  if (sw->txff_ctrl_crc_check_disable != hw->txff_ctrl_crc_check_disable) {
    port_mgr_log(
        "%s:%d: Change detected in txff_ctrl_crc_check_disable : %d(SW) : "
        "%d(HW)",
        __func__,
        __LINE__,
        sw->txff_ctrl_crc_check_disable,
        hw->txff_ctrl_crc_check_disable);
    return true;
  }
  if (sw->txff_ctrl_crc_removal_disable != hw->txff_ctrl_crc_removal_disable) {
    port_mgr_log(
        "%s:%d: Change detected in txff_ctrl_crc_removal_disable : %d(SW) : "
        "%d(HW)",
        __func__,
        __LINE__,
        sw->txff_ctrl_crc_removal_disable,
        hw->txff_ctrl_crc_removal_disable);
    return true;
  }
  if (sw->txff_ctrl_fcs_insert_disable != hw->txff_ctrl_fcs_insert_disable) {
    port_mgr_log(
        "%s:%d: Change detected in txff_ctrl_fcs_insert_disable : %d(SW) : "
        "%d(HW)",
        __func__,
        __LINE__,
        sw->txff_ctrl_fcs_insert_disable,
        hw->txff_ctrl_fcs_insert_disable);
    return true;
  }
  if (sw->txff_ctrl_pad_disable != hw->txff_ctrl_pad_disable) {
    port_mgr_log(
        "%s:%d: Change detected in txff_ctrl_pad_disable : %d(SW) : %d(HW)",
        __func__,
        __LINE__,
        sw->txff_ctrl_pad_disable,
        hw->txff_ctrl_pad_disable);
    return true;
  }

  port_mgr_log(
      "%s:%d: No changes detected in misc port params", __func__, __LINE__);
  return false;
}

bf_status_t port_mgr_tof1_port_delta_compute(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    bf_ha_port_reconcile_info_t *recon_info) {
  port_mgr_port_t *port_p;
  bool existing_port_deleted = false;
  bool new_port_detected = false;
  bool same_port_with_diff_cfg_detected = false;
  bool existing_port_admin_state_change_detected = false;
  bool existing_port_misc_params_change_detected = false;
  bool port_bringup_required = false;
  bool port_fsm_link_monitoring_required = false;

  port_mgr_log("%s:%d Entering Computing MAC delta changes for dev %d port %d",
               __func__,
               __LINE__,
               dev_id,
               dev_port);
  port_p = port_mgr_map_dev_port_to_port_allow_unassigned(dev_id, dev_port);
  if (port_p == NULL) return BF_INVALID_ARG;

  port_mgr_dev_t *dev_p = port_mgr_map_dev_id_to_dev_p_allow_unassigned(dev_id);
  if (dev_p == NULL) {
    return BF_INVALID_ARG;
  }

  port_mgr_log("Dumping Config for Dev : %d : Port : %d : ( SW VAL : HW VAL )",
               dev_id,
               dev_port);
  port_dump_all_config(&port_p->sw, &port_p->hw);

  existing_port_deleted =
      ((port_p->hw.assigned == true) && (port_p->sw.assigned == false));
  new_port_detected =
      ((port_p->hw.assigned == false) && (port_p->sw.assigned == true));
  same_port_with_diff_cfg_detected =
      ((port_p->hw.assigned == true) && (port_p->sw.assigned == true) &&
       ((port_p->hw.speed != port_p->sw.speed) ||
        (port_p->hw.fec != port_p->sw.fec)));
  existing_port_admin_state_change_detected =
      ((port_p->hw.assigned == true) && (port_p->sw.assigned == true) &&
       (port_p->hw.enabled != port_p->sw.enabled));
  /* We care about changes in misc params only if the port was added in hw
   * before cfg replay and was added and enabled during cfg replay. We need
   * to care about the port being enabled during cfg replay because of the
   * following scenario
   * Before cfg replay, if we only added and set the src mac address but
   * never enabled the port, the src address will never be flushed to the
   * hardware. Then during cfg replay, we would again add the port and replay
   * the src mac address. Thus while computing the delta for the port, we
   * would think that there is a mismatch between the replayed software src
   * mac addr and the already programmed hardware src mac addr. And thus
   * port mgr would flap the port. The correct thing to do here would be
   * to do nothing
   */
  existing_port_misc_params_change_detected =
      ((port_p->hw.assigned == true) && (port_p->sw.assigned == true) &&
       (port_p->sw.enabled == true) &&
       (port_misc_params_change_detected(&port_p->hw, &port_p->sw)));
  /* oper_state is ignored if port is in MAC_NEAR loopback because for that
   * mode the hw oper_state is forced to 1 by sw.
   */
  port_bringup_required =
      ((port_p->hw.assigned == true) && (port_p->sw.assigned == true) &&
       (port_p->sw.enabled == true) && (port_p->hw.enabled == true) &&
       (port_p->hw.lpbk_mode != BF_LPBK_MAC_NEAR) && (!port_p->hw.oper_state));
  port_fsm_link_monitoring_required =
      ((port_p->hw.assigned == true) && (port_p->sw.assigned == true) &&
       (port_p->sw.enabled == true) && (port_p->hw.enabled == true) &&
       ((port_p->hw.oper_state) || (port_p->hw.lpbk_mode == BF_LPBK_MAC_NEAR)));

  /* Indicates that an existing port has been deleted */
  if (existing_port_deleted) {
    /* Set the corrective action as DEL for the port */
    recon_info->ca = BF_HA_CA_PORT_DELETE;
    port_mgr_log("%s:%d:%d:%d Existing port deleted",
                 __func__,
                 __LINE__,
                 dev_id,
                 dev_port);
    return BF_SUCCESS;
  }
  /* Indicates that a new port is being added */
  else if (new_port_detected) {
    /* Get the port attributes of the port which will be used when the port is
     * added */
    port_mgr_port_attrib_get(dev_id, dev_port, &recon_info->port_attrib);
    /* Set the corrective action as ADD for the port under consideration */
    recon_info->ca = BF_HA_CA_PORT_ADD;
    if (port_p->sw.enabled) {
      recon_info->ca = BF_HA_CA_PORT_ADD_THEN_ENABLE;
    }
    port_mgr_log(
        "%s:%d:%d:%d New port detected", __func__, __LINE__, dev_id, dev_port);
    return BF_SUCCESS;
  }
  /* Indicates that the same port is being added with a diff config*/
  else if (same_port_with_diff_cfg_detected) {
    /* Get the port attributes of the port which will be used when the port is
     * added */
    port_mgr_port_attrib_get(dev_id, dev_port, &recon_info->port_attrib);
    /* Set the corrective action as DEL_THEN_ADD for the port under
     * consideration */
    recon_info->ca = BF_HA_CA_PORT_DELETE_THEN_ADD;
    if (port_p->sw.enabled) {
      recon_info->ca = BF_HA_CA_PORT_DELETE_THEN_ADD_THEN_ENABLE;
    }
    port_mgr_log("%s:%d:%d:%d Same port with different config detected",
                 __func__,
                 __LINE__,
                 dev_id,
                 dev_port);
    return BF_SUCCESS;
  }
  /* Indicates that a change in the admin state of the port is detected */
  else if (existing_port_admin_state_change_detected) {
    recon_info->ca =
        port_p->sw.enabled ? BF_HA_CA_PORT_ENABLE : BF_HA_CA_PORT_DISABLE;
    port_mgr_log("%s:%d:%d:%d Port admin state change detected",
                 __func__,
                 __LINE__,
                 dev_id,
                 dev_port);
    return BF_SUCCESS;
  } else if (existing_port_misc_params_change_detected) {
    recon_info->ca = BF_HA_CA_PORT_FLAP;
    port_mgr_log("%s:%d:%d:%d Existing port misc params change detected",
                 __func__,
                 __LINE__,
                 dev_id,
                 dev_port);
    return BF_SUCCESS;
  }
  /* Indicates that the port was and is enabled after warm boot but is
   * is not UP. Hence FLAP the port in order to bring it up */
  else if (port_bringup_required) {
    recon_info->ca = BF_HA_CA_PORT_FLAP;
    port_mgr_log("%s:%d:%d:%d Port needs to be brought up",
                 __func__,
                 __LINE__,
                 dev_id,
                 dev_port);
    return BF_SUCCESS;
  }
  /* Indicates that the port was replayed with the exact same config */
  else if (port_fsm_link_monitoring_required) {
    /* This indicates that the port was added and enabled and up and was
     * replayed
     * with the exact same configuration. As a result we want initialize the
     * fsm in link monitoring state.
     */
    recon_info->ca = BF_HA_CA_PORT_FSM_LINK_MONITORING;
    port_mgr_log(
        "%s:%d:%d:%d Port FSM needs to be initialized in WAIT_FOR_DOWN_EVENT "
        "state",
        __func__,
        __LINE__,
        dev_id,
        dev_port);
    return BF_SUCCESS;
  }

  port_mgr_log("%s:%d:%d:%d No MAC delta detected for port",
               __func__,
               __LINE__,
               dev_id,
               dev_port);
  return BF_SUCCESS;
}

/* This is helper routine which does a few necessary preliminary checks on the
 * params of the port being added, and then fills up the sw datastructure of
 * the port. This is called during cfg replay and the usual non HA mode for
 * port add
 **/
static bf_status_t port_mgr_port_add_basic_setup(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    port_mgr_port_t *port_p,
    port_mgr_mac_block_t *mac_block_p,
    bf_port_attributes_t *port_attrib,
    int ch) {
  bool enabled;

  /* Indicates that this API request is during warm init. Hence just log
       this request in the data structure port_p->sw and return. DO NOT
       touch the hardware just yet */
  /* verify validity of the speed and fec type for the port as we can just
     throw away invalid cfg bring replayed as it would be thrown eventually
     anyway during push delta phase. Hence it's better to throw it away
     right now */
  if (port_p->sw.assigned) return BF_INVALID_ARG;
  if (!port_mgr_valid_speed_and_ch(port_attrib->port_speeds, ch)) {
    return BF_INVALID_ARG;
  }
  if (!port_mgr_valid_speed_and_fec(port_attrib->port_speeds,
                                    port_attrib->port_fec_types)) {
    return BF_INVALID_ARG;
  }
  // verify required resources are uncommitted
  if (!port_mgr_ch_available(dev_id, dev_port, port_attrib->port_speeds)) {
    return BF_INVALID_ARG;
  }

  // init all sw config to defaults
  port_mgr_set_default_port_state(dev_id, dev_port);

  // configuration is valid
  port_p->sw.assigned = 1;

  // determine MAC channels required by new speed mode
  int ch_reqd =
      port_mgr_ch_reqd_by_speed(dev_id, dev_port, port_attrib->port_speeds);

  // update committed channels in the mac_block (quad)
  mac_block_p->ch_in_use |= ch_reqd;

  // speed must be set in port for the txff_ctrl calculation (below)
  port_p->sw.speed = port_attrib->port_speeds;
  port_p->sw.fec = port_attrib->port_fec_types;

  mac_block_p->txff_ctrl = port_mgr_construct_txff_ctrl(dev_id, dev_port);

  // clear historical counter stats if persistent port stats is disabled
  port_mgr_stats_persistent_get(dev_id, &enabled);
  if (!enabled) {
    port_mgr_mac_stats_clear_sw_ctrs(&port_p->mac_stat_historical);
  }

  return BF_SUCCESS;
}

/*
 * This function returns the Aful channel mode for a MAC channel (port)
 * based on chip part revision number and port speed.
 */
static void port_mgr_mac_get_aful_chnl_mode(bf_dev_id_t dev_id,
                                            port_mgr_port_t *port_p,
                                            uint32_t *aful_1ch_mode,
                                            uint32_t *aful_2ch_mode,
                                            uint32_t *aful_4ch_mode) {
  /* Set default values for all Aful channel modes */
  *aful_1ch_mode = 19;
  *aful_2ch_mode = 9;
  *aful_4ch_mode = 4;

  if (port_p == NULL) {
    /* Should never hit this case */
    port_mgr_log_error("port_mgr_mac_get_aful_chnl_mode: invalid arg");
    /* return with default values */
    return;
  }

  /* For A0, use the default values for all Aful chnll modes & port speeds */
  if (port_mgr_dev_part_rev_get(dev_id) == BF_SKU_CHIP_PART_REV_A0) {
    return;
  }

  /* For B0 and later parts, set aful_1ch_mode to non-default value for
   * 40G port speed.
   */
  if (port_p->sw.speed == BF_SPEED_40G) {
    *aful_1ch_mode = 9;
  }

  return;
}

/*
 * This function returns the TX FIFO threshold for a MAC channel (port)
 * based on chip part revision number and port speed.
 */
static void port_mgr_mac_get_chnl_txff_thres(bf_dev_id_t dev_id,
                                             port_mgr_port_t *port_p,
                                             uint32_t *txff_thres) {
  if (port_p == NULL) {
    /* Should never hit this case */
    port_mgr_log_error("port_mgr_mac_get_chnl_txff_thres: invalid arg");
    /* set default value and return */
    *txff_thres = 4;
    return;
  }

  /* For B0 and later parts, set txfifo threshold to 4 for all port speeds */
  if (port_mgr_dev_part_rev_get(dev_id) != BF_SKU_CHIP_PART_REV_A0) {
    *txff_thres = 4;
    return;
  }

  /* For A0, set TX fifo thres according to port speed:
   *
   *   100G: txfifo threshold = 15
   *   25G: txfifo threshold = 4
   *   50G: txfifo threshold = 8
   *   10G: same as 25
   *   40G: same as 50
   *   40G MLG: same as 50
   */
  switch (port_p->sw.speed) {
    case BF_SPEED_100G:
      if (port_p->sw.fec == BF_FEC_TYP_NONE) {
        *txff_thres = 0xD;
      } else {
        *txff_thres = 0xC;
      }
      break;
    case BF_SPEED_50G:
    case BF_SPEED_40G:
      *txff_thres = 8;
      break;
    case BF_SPEED_25G:
    case BF_SPEED_10G:
    case BF_SPEED_1G:
      *txff_thres = 4;
      break;
    default:
      *txff_thres = 4;
      break;
  }

  return;
}

/** \brief port_mgr_port_eth_cpu_port_reset
 *         Disable/enable CPU eth port MAC channel enable bit
 *         Note: This function is called only during fast reconfig after
 *               core reset & config replay but before enabling traffic. This
 *               is needed to reinitialize credits to EBUF (through PGR) as
 *               core is reset. Otherwise, packets destined to CPU eth port
 *               will be stuck in TM due to lack of credits.
 *
 *
 * \param dev_id: int              : system-assigned identifier
 *(0..BF_MAX_DEV_COUNT-1)
 * \param eth_cpu_port: int              : dev port # for eth cpu port
 *
 */
bf_status_t port_mgr_port_eth_cpu_port_reset(bf_dev_id_t dev_id,
                                             bf_dev_port_t eth_cpu_port) {
  int mac_block, ch, is_cpu_port;
  int port_id;
  port_mgr_err_t err;
  port_mgr_port_t *port_p;
  bf_status_t sts;

  /* Check if the port is added during config replay */
  port_p = port_mgr_map_dev_port_to_port(dev_id, eth_cpu_port);
  if (port_p == NULL) {
    /* Do nothing, return */
    return BF_OBJECT_NOT_FOUND;
  }

  err = port_mgr_map_dev_port_to_all(
      dev_id, eth_cpu_port, NULL, &port_id, &mac_block, &ch, &is_cpu_port);
  if (err) {
    /* Do nothing, return */
    return BF_SUCCESS;
  }

  sts = port_mgr_mac_reset_txff_ctrl_chnl_enable(dev_id, mac_block);

  return sts;
}

/** \brief port_mgr_port_add
 *         Add a Tofino port to the system
 *         note: dvm will call this API twice, first for ingress, then for
 *egress.
 *
 * [ PRE_ENABLE ]
 *
 * \param dev_id: int              : system-assigned identifier
 *(0..BF_MAX_DEV_COUNT-1)
 * \param port: int              : physical port # on dev_id
 *(0..LLD_MAX_PORTS-1)
 * \param speed:port_mgr_port_speed_e : port speed/mode
 * \param fec:  port_mgr_fec_type_t   : fec mode
 *
 * \return: BF_SUCCESS     : port added successfully
 * \return: BF_INVALID_ARG : dev_id never added or dev_id >
 *BF_MAX_DEV_COUNT-1
 * \return: BF_INVALID_ARG : port > LLD_MAX_PORTS-1
 * \return: BF_INVALID_ARG : resources in-use
 *
 */
bf_status_t port_mgr_tof1_port_add(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   bf_port_attributes_t *port_attrib,
                                   bf_port_cb_direction_t direction) {
  int mac_block, ch, is_cpu_port;
  int port_id;
  port_mgr_err_t err;
  port_mgr_port_t *port_p;
  port_mgr_mac_block_t *mac_block_p;
  uint32_t txff_thres, aful_1ch_mode, aful_2ch_mode, aful_4ch_mode;
  bf_status_t sts;

  if (port_attrib == NULL) return BF_INVALID_ARG;
  port_mgr_log("%s:%d:%d:%d:%d(speed):%d(fec)",
               __func__,
               __LINE__,
               dev_id,
               dev_port,
               port_attrib->port_speeds,
               port_attrib->port_fec_types);

  if ((direction != BF_PORT_CB_DIRECTION_INGRESS) &&
      (direction != BF_PORT_CB_DIRECTION_EGRESS))
    return BF_INVALID_ARG;

  // Check for ports port_mgr really handles. This callback gets called for
  // any port_add, including recirc, etc. Just ignore those.
  err = port_mgr_tof1_map_dev_port_to_all(
      dev_id, dev_port, NULL, &port_id, &mac_block, &ch, &is_cpu_port);
  if (err) {
    return BF_SUCCESS;
  }
  if ((port_id >= 64) && (!is_cpu_port)) {
    // ignore non-MAC ports
    return BF_SUCCESS;
  }

  port_mgr_dev_t *dev_p = port_mgr_map_dev_id_to_dev_p_allow_unassigned(dev_id);
  if (dev_p == NULL) return BF_INVALID_ARG;

  port_p = port_mgr_map_dev_port_to_port_allow_unassigned(dev_id, dev_port);
  if (port_p == NULL) return BF_INVALID_ARG;

  mac_block_p = port_mgr_tof1_map_idx_to_mac_block(dev_id, mac_block);
  if (!mac_block_p) return BF_INVALID_ARG;

  switch (port_mgr_dev_ha_stage_get(dev_id)) {
    case PORT_MGR_HA_CFG_REPLAY:
      if (direction == BF_PORT_CB_DIRECTION_INGRESS) {
        /* During warm init, fill up the port_p->sw during EGRESS call only*/
        return BF_SUCCESS;
      }
      sts = port_mgr_port_add_basic_setup(
          dev_id, dev_port, port_p, mac_block_p, port_attrib, ch);
      if (sts != BF_SUCCESS) {
        port_mgr_log(
            "Error : Unable to do port add basic setup during cfg rply");
        return BF_INVALID_ARG;
      }
      return BF_SUCCESS;
    case PORT_MGR_HA_DELTA_PUSH:
      /* dvm will call this API twice, first for egress, then for ingress.
       * The port will be added on the first call (egress) but the
       * force_pfc_flush
       * bits will be left on until the second call, for ingress */
      if (direction == BF_PORT_CB_DIRECTION_INGRESS) {
        /* undo force clear pause state */
        port_mgr_mac_pgm_force_pfc_flush(dev_id, mac_block, ch);
        return BF_SUCCESS;
      }
      /* verify required resources are uncommitted only when we are not in delta
         push phase because this would have already been populated during cfg
         replay. Hence if we check again during push delta, then this check will
         surely fail. We can assume that right things already exist in
         mac_block_p->ch_in_use
         and simply proceed*/
      break;
    case PORT_MGR_HA_NONE:
      /* dvm will call this API twice, first for egress, then for ingress.
       * The port will be added on the first call (egress) but the
       * force_pfc_flush
       * bits will be left on until the second call, for ingress */
      if (direction == BF_PORT_CB_DIRECTION_INGRESS) {
        /* undo force clear pause state */
        port_mgr_mac_pgm_force_pfc_flush(dev_id, mac_block, ch);
        return BF_SUCCESS;
      }
      sts = port_mgr_port_add_basic_setup(
          dev_id, dev_port, port_p, mac_block_p, port_attrib, ch);
      if (sts != BF_SUCCESS) {
        port_mgr_log(
            "Error : Unable to do port add basic setup during non HA port add");
        return BF_INVALID_ARG;
      }
      /* disable persistent interrupt sources */
      port_mgr_mac_basic_interrupt_setup(dev_id, dev_port);
      break;
    case PORT_MGR_HA_DELTA_COMPUTE:
    case PORT_MGR_HA_MAX:
    default:
      port_mgr_log("Error : Invalid HA stage %d while trying to add a port",
                   port_mgr_dev_ha_stage_get(dev_id));
      return BF_INVALID_ARG;
  }

  // set in disabled state
  port_mgr_port_disable(dev_id, dev_port);

  /* pgm MAC but leave channel disabled */
  port_mgr_port_pgm_speed_fec(
      dev_id, dev_port, port_p->sw.speed, port_p->sw.fec);

  /* set rxsigok debounce */
  port_mgr_mac_pgm_rxsigok_ctrl(dev_id, mac_block);

  /* pgm txff truncation settings */
  port_mgr_mac_pgm_txff_trunc_ctrl(dev_id,
                                   mac_block,
                                   ch,
                                   port_p->sw.txff_trunc_ctrl_size,
                                   port_p->sw.txff_trunc_ctrl_en);

  /* enable ch in txff, this resets credits between txff and epb
   * note: txff still in force flush mode, until enabled */
  port_mgr_mac_pgm_txff_ctrl(dev_id, mac_block);

  port_mgr_mac_get_chnl_txff_thres(dev_id, port_p, &txff_thres);

  port_mgr_mac_cfg_txfifo_ctrl(dev_id, dev_port, txff_thres);

  port_mgr_mac_get_aful_chnl_mode(
      dev_id, port_p, &aful_1ch_mode, &aful_2ch_mode, &aful_4ch_mode);

  port_mgr_mac_cfg_fifo_ctrl1(
      dev_id, dev_port, aful_1ch_mode, aful_2ch_mode, aful_4ch_mode);

  {
    bool am_en = ((port_p->sw.fec == BF_FEC_TYP_REED_SOLOMON) &&
                  (port_p->sw.speed == BF_SPEED_25G))
                     ? true
                     : false;

    port_mgr_mac_rs_fec_25g_am_fix(dev_id, mac_block, ch, am_en);
  }
  return BF_SUCCESS;
}

static void port_mgr_port_remove_basic_setup(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    port_mgr_port_t *port_p,
    port_mgr_mac_block_t *mac_block_p) {
  int ch_reqd;
  // determine freed resources (mac/serdes)
  ch_reqd = port_mgr_ch_reqd_by_speed(dev_id, dev_port, port_p->sw.speed);
  // update committed resources
  mac_block_p->ch_in_use &= ~ch_reqd;

  // reset port speed and fec mode
  port_p->sw.speed = BF_SPEED_NONE;
  port_p->sw.fec = BF_FEC_TYP_NONE;

  // update txff_ctrl, clear chnl_ena
  mac_block_p->txff_ctrl = port_mgr_construct_txff_ctrl(dev_id, dev_port);

  port_p->sw.assigned = 0;
}

/** \brief port_mgr_port_remove
 *         Remove a Tofino port from the system
 *         note: dvm will call this API twice, first for ingress, then for
 *egress.
 *
 * \param dev_id: int : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param port: int : physical port # on dev_id (0..LLD_MAX_PORTS-1)
 *
 * \return: BF_SUCCESS : port removed successfully
 *
 */
bf_status_t port_mgr_tof1_port_remove(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      bf_port_cb_direction_t direction) {
  port_mgr_log("%s:%d:%d:%d:%d(direction)",
               __func__,
               __LINE__,
               dev_id,
               dev_port,
               direction);
  int mac_block, ch, is_cpu_port, port_id, ch_reqd;
  port_mgr_mac_block_t *mac_block_p;
  port_mgr_port_t *port_p;

  port_mgr_dev_t *dev_p = port_mgr_map_dev_id_to_dev_p_allow_unassigned(dev_id);
  if (dev_p == NULL) return BF_INVALID_ARG;

  port_mgr_err_t err = port_mgr_map_dev_port_to_all(
      dev_id, dev_port, NULL, &port_id, &mac_block, &ch, &is_cpu_port);
  if (err) {
    // ignore non-MAC ports
    return BF_SUCCESS;
  }

  if (port_mgr_dev_ha_stage_get(dev_id) == PORT_MGR_HA_DELTA_PUSH) {
    port_p = port_mgr_map_dev_port_to_port_allow_unassigned(dev_id, dev_port);
  } else {
    port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  }
  if (!port_p) return BF_INVALID_ARG;

  mac_block_p = port_mgr_tof1_map_idx_to_mac_block(dev_id, mac_block);

  if (mac_block_p == NULL) {
    port_mgr_log("Error : Invalid device id %d", dev_id);
    return BF_INVALID_ARG;
  }

  // make sure its a real mac
  if ((port_id >= 64) && (!is_cpu_port)) {
    // ignore non-MAC ports
    return BF_SUCCESS;
  }

  switch (port_mgr_dev_ha_stage_get(dev_id)) {
    case PORT_MGR_HA_CFG_REPLAY:
      if (direction == BF_PORT_CB_DIRECTION_INGRESS) {
        /* During warm init, do nothing during the INGRESS call*/
        return BF_SUCCESS;
      }
      port_mgr_port_remove_basic_setup(dev_id, dev_port, port_p, mac_block_p);
      return BF_SUCCESS;
    case PORT_MGR_HA_DELTA_PUSH:
      /* consider the case when a port is being deleted because it was already
         in the hw but was never replayed back. In this case the port_p->sw
         data structure would be invalid. Hence we need to go by the port_p->hw
         data strcuture */
      if (!port_p->hw.assigned) {
        /* Indicates that the port was never added in the past as well. Not sure
           if this condition will ever occur */
        return BF_SUCCESS;
      }
      if (direction == BF_PORT_CB_DIRECTION_INGRESS) {
        /* If the port was enabled in hw, then disable it before removing */
        if (port_p->hw.enabled) { /* we check hw instead of sw*/
          port_mgr_port_disable(dev_id, dev_port);
        }
        // update txff_ctrl, set force flush mode
        // mac_block_p->txff_ctrl = port_mgr_construct_txff_ctrl(dev_id,
        // dev_port);
        port_mgr_mac_pgm_txff_ctrl(dev_id, mac_block);
        return BF_SUCCESS;
      } else {
        // determine freed resources (mac/serdes)
        ch_reqd = port_mgr_ch_reqd_by_speed(dev_id, dev_port, port_p->hw.speed);
      }
      break;
    case PORT_MGR_HA_NONE:
      // PORT-SEQ If direction == BF_PORT_CB_DIRECTION_INGRESS port-disable,
      // enable flush in MAC
      // PORT-SEQ If direction == BF_PORT_CB_DIRECTION_EGRESS disable MAC
      // channel , Disable Flush
      if (direction == BF_PORT_CB_DIRECTION_INGRESS) {
        // if enabled, disable before removing
        if (port_p->sw.enabled) {
          port_mgr_port_disable(dev_id, dev_port);
        }
        // update txff_ctrl, set force flush mode
        mac_block_p->txff_ctrl = port_mgr_construct_txff_ctrl(dev_id, dev_port);
        port_mgr_mac_pgm_txff_ctrl(dev_id, mac_block);
        return BF_SUCCESS;
      } else {
        // determine freed resources (mac/serdes)
        ch_reqd = port_mgr_ch_reqd_by_speed(dev_id, dev_port, port_p->sw.speed);
        port_mgr_port_remove_basic_setup(dev_id, dev_port, port_p, mac_block_p);
      }
      break;
    case PORT_MGR_HA_DELTA_COMPUTE:
    case PORT_MGR_HA_MAX:
    default:
      port_mgr_log("Error : Invalid HA stage %d while trying to remove a port",
                   port_mgr_dev_ha_stage_get(dev_id));
      return BF_INVALID_ARG;
  }

  for (ch = 0; ch < 4; ch++) {
    if (ch_reqd & (1 << ch)) {
      port_mgr_mac_delete_ch(dev_id, mac_block, ch);
    }
  }
  port_mgr_mac_pgm_txff_ctrl(dev_id, mac_block);
  return BF_SUCCESS;
}

/** \brief port_mgr_port_enable
 *         Enable a Tofino port. This kicks off the port
 *         bring-up sequence.
 *
 * [ PRE_ENABLE ]
 *
 *
 * \param dev_id: int         : system-assigned identifier
 *(0..BF_MAX_DEV_COUNT-1)
 * \param port: int         : physical port # on dev_id (0..LLD_MAX_PORTS-1)
 *
 * \return: BF_SUCCESS     : port enabled successfully
 * \return: BF_INVALID_ARG : dev_id never added or dev_id > BF_MAX_DEV_COUNT-1
 * \return: BF_INVALID_ARG : port > LLD_MAX_PORTS-1
 *
 */
bf_status_t port_mgr_tof1_port_enable(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      bool enable) {
  port_mgr_log(
      "%s:%d:%d:%d:%d(enb)", __func__, __LINE__, dev_id, dev_port, enable);
  bf_status_t sts = BF_SUCCESS;
  // port_mgr_dev_t *dev_p =
  // port_mgr_map_dev_id_to_dev_p_allow_unassigned(dev_id);
  port_mgr_tof1_pdev_t *dev_p = port_mgr_dev_physical_dev_get(dev_id);

  if (!dev_p) return BF_INVALID_ARG;

  port_mgr_port_t *port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  if (!port_p) return BF_INVALID_ARG;
  int mac_block, ch, is_cpu_port, port_id;

  port_mgr_map_dev_port_to_all(
      dev_id, dev_port, NULL, &port_id, &mac_block, &ch, &is_cpu_port);

  // make sure its a real mac
  if ((port_id >= 64) && (!is_cpu_port)) {
    // ignore non-MAC ports
    return BF_SUCCESS;
  }

  port_mgr_log(
      "PRT :%d:%3d:-: %s", dev_id, dev_port, enable ? "Enable" : "Disable");

  if (!enable) {
    return port_mgr_port_disable(dev_id, dev_port);
  }

  switch (port_mgr_dev_ha_stage_get(dev_id)) {
    case PORT_MGR_HA_CFG_REPLAY:
      port_p->sw.enabled = 1;
      return BF_SUCCESS;
    case PORT_MGR_HA_DELTA_PUSH:
      sts = port_mgr_serdes_delta_settings_apply(dev_id, dev_port);
      if (sts != BF_SUCCESS) {
        port_mgr_log(
            "Error : Unable to apply serdes delta settings for port %d "
            "during port enable",
            dev_port);
        return sts;
      }
      break;
    case PORT_MGR_HA_NONE:
      port_p->sw.enabled = 1;
      break;
    case PORT_MGR_HA_DELTA_COMPUTE:
    case PORT_MGR_HA_MAX:
    default:
      port_mgr_log("Error : Invalid HA stage %d while trying to enable a port",
                   port_mgr_dev_ha_stage_get(dev_id));
      return BF_INVALID_ARG;
  }

  // Check if the port had been marked for serdes upgrade
  if (port_p->serdes_upgrade_required) {
    sts = port_mgr_ha_port_serdes_upgrade(
        dev_id, dev_port, dev_p->new_serdes_fw_ver, dev_p->new_serdes_fw);
    if (sts != BF_SUCCESS) {
      port_mgr_log(
          "Error %s (%d) : Unable to upgrade the serdes firmware for dev %d "
          "port %d",
          bf_err_str(sts),
          sts,
          dev_id,
          dev_port);
    }
  }
  port_mgr_mac_basic_setup(dev_id, mac_block, ch);

  // checkpoint
  return BF_SUCCESS;
}

static void port_mgr_port_disable_basic_setup(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              port_mgr_port_t *port_p) {
  if (port_p->sw.oper_state) {
    port_mgr_link_dn_actions(dev_id, dev_port);
  }
  port_p->sw.enabled = 0;
  port_p->sw.oper_state = 0;

  (void)dev_port;
  (void)dev_id;
}

static bf_status_t port_mgr_disable_mac(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port) {
  bool is_sw_model = false;

  bf_drv_device_type_get(dev_id, &is_sw_model);
  if (is_sw_model) {
    /* this API depends on real data returned from the MAC
     * which is not simlated in the sw_model
     */
    return BF_SUCCESS;
  }

  port_mgr_log("PRT :%d:%3d:-: MACs serdesmux.sig_ok=LOW", dev_id, dev_port);
  bf_port_force_sig_ok_low_set(dev_id, dev_port);

  port_mgr_mac_force_pcs_near_loopback(dev_id, dev_port, false);

  // guarantee at least 10us between SIG_OK=0 and RX_EN=0
  bf_sys_usleep(10);

  // disable the MAC Rx side
  port_mgr_log("PRT :%d:%3d:-: Disable MAC Rx", dev_id, dev_port);
  port_mgr_mac_set_rx_enable(dev_id, dev_port, false);
  return BF_SUCCESS;
}

static bf_status_t port_mgr_disable_serdes(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port) {
  int num_lanes, ln;

  num_lanes = port_mgr_tof1_get_num_lanes(dev_id, dev_port);
  // shut off the serdes
  for (ln = 0; ln < num_lanes; ln++) {
    // turn off the PLLs and output driver
    bf_serdes_set_rx_tx_and_tx_output_en(dev_id,
                                         dev_port,
                                         ln,
                                         false /*rx_en*/,
                                         false /*tx_en*/,
                                         false /*tx_output_en*/);
    if (ln == 0) {
      // disable AN just in case
      bf_serdes_autoneg_stop(dev_id, dev_port);
    }
    bf_serdes_link_training_set(dev_id, dev_port, ln, false);
  }
  for (ln = 0; ln < num_lanes; ln++) {
    // stop any DFE
    bf_serdes_stop_dfe_adaptive(dev_id, dev_port, ln);
    bf_serdes_stop_dfe(dev_id, dev_port, ln);
  }
  return BF_SUCCESS;
}

/** \brief port_mgr_port_disable
 *         Disable a Tofino port. This brings the port down
 *         and disables the transmitter, bringing the link-
 *         partner down.
 *
 * \param dev_id: int         : system-assigned identifier
 *(0..BF_MAX_DEV_COUNT-1)
 * \param port: int         : physical port # on dev_id (0..LLD_MAX_PORTS-1)
 *
 * \return: BF_SUCCESS     : port disabled successfully
 * \return: BF_INVALID_ARG : dev_id never added or dev_id > BF_MAX_DEV_COUNT-1
 * \return: BF_INVALID_ARG : port > LLD_MAX_PORTS-1
 *
 */
bf_status_t port_mgr_port_disable(bf_dev_id_t dev_id, bf_dev_port_t dev_port) {
  int ch_reqd;
  int mac_block, ch, is_cpu_port, port_id;
  int num_lanes, ln;

  port_mgr_dev_t *dev_p = port_mgr_map_dev_id_to_dev_p_allow_unassigned(dev_id);
  if (!dev_p) return BF_INVALID_ARG;

  port_mgr_port_t *port_p;
  if (port_mgr_dev_ha_stage_get(dev_id) == PORT_MGR_HA_DELTA_PUSH) {
    port_p = port_mgr_map_dev_port_to_port_allow_unassigned(dev_id, dev_port);
  } else {
    port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  }
  if (!port_p) return BF_INVALID_ARG;

  port_mgr_map_dev_port_to_all(
      dev_id, dev_port, NULL, &port_id, &mac_block, &ch, &is_cpu_port);

  // make sure its a real mac
  if ((port_id >= 64) && (!is_cpu_port)) {
    // ignore non-MAC ports
    return BF_SUCCESS;
  }

  switch (port_mgr_dev_ha_stage_get(dev_id)) {
    case PORT_MGR_HA_CFG_REPLAY:
      port_mgr_port_disable_basic_setup(dev_id, dev_port, port_p);
      return BF_SUCCESS;
    case PORT_MGR_HA_DELTA_PUSH:
      ch_reqd = port_mgr_ch_reqd_by_speed(dev_id, dev_port, port_p->hw.speed);
      num_lanes = num_mac_chn_consumed(ch_reqd);

      if (port_p->hw.enabled) { /* Here we check the hw value*/
        // first shut off the serdes
        for (ln = 0; ln < num_lanes; ln++) {
          // turn off the PLLs and output driver
          bf_serdes_set_rx_tx_and_tx_output_en_allow_unassigned(
              dev_id,
              dev_port,
              ln,
              false /*rx_en*/,
              false /*tx_en*/,
              false /*tx_output_en*/);
          if (ln == 0) {
            // disable AN just in case
            bf_serdes_autoneg_stop(dev_id, dev_port);
          }
          bf_serdes_link_training_set(dev_id, dev_port, ln, false);
        }
      }
      break;
    case PORT_MGR_HA_NONE: {
      bf_rmon_counter_array_t hw_ctrs;
      bf_status_t bf_status;

      num_lanes = port_mgr_tof1_get_num_lanes(dev_id, dev_port);
      port_mgr_disable_mac(dev_id, dev_port);

      // update historical counters before MAC ch disable (which clears the HW
      // counters)
      bf_status =
          bf_port_mac_stats_hw_only_sync_get(dev_id, dev_port, &hw_ctrs);
      if (bf_status != BF_SUCCESS) {
        port_mgr_log("%d:%3d: Error : %d : updating historical ctrs",
                     dev_id,
                     dev_port,
                     bf_status);
      } else {
        port_mgr_mac_stats_copy(&hw_ctrs,
                                &port_p->mac_stat_historical,
                                &port_p->mac_stat_historical);
      }
      // disable serdes after some time (using above stats collection for the
      // delay)
      // Dont disable serdes for internal ports
      bool is_internal = false;
      lld_err_t err =
          lld_sku_is_dev_port_internal(dev_id, dev_port, &is_internal);
      if ((err != LLD_OK) || !is_internal) {
        port_mgr_disable_serdes(dev_id, dev_port);
      }
      port_mgr_port_disable_basic_setup(dev_id, dev_port, port_p);
      break;
    }
    case PORT_MGR_HA_DELTA_COMPUTE:
    case PORT_MGR_HA_MAX:
    default:
      port_mgr_log("Error : Invalid HA stage %d while trying to add a port",
                   port_mgr_dev_ha_stage_get(dev_id));
      return BF_INVALID_ARG;
  }

  port_mgr_mac_disable_ch(dev_id, mac_block, ch);
  port_mgr_mac_basic_interrupt_teardown(dev_id, dev_port);

  // checkpoint
  return BF_SUCCESS;
}

/** \brief port_mgr_port_disable_all
 *         Set all ports to the disabled state (usually at start-up).
 *
 * \param dev_id : int   : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 *
 * \return: BF_SUCCESS : port speed set successfully
 */
bf_status_t port_mgr_port_disable_all(bf_dev_id_t dev_id) {
  bf_dev_pipe_t pipe;
  int port_id;
  bf_dev_port_t dev_port;
  uint32_t num_pipes = 0;

  lld_sku_get_num_active_pipes(dev_id, &num_pipes);
  for (pipe = 0; pipe < num_pipes; pipe++) {
    for (port_id = lld_get_min_fp_port(dev_id);
         port_id <= lld_get_max_fp_port(dev_id);
         port_id++) {
      dev_port = MAKE_DEV_PORT(pipe, port_id);

      /* note: No "port_t" will exist since no ports have been
       *       "added", so need to go directly to mac layer. */
      port_mgr_mac_basic_interrupt_teardown(dev_id, dev_port);
    }
  }
  // ..and then the CPU MAC channels
  for (port_id = lld_get_min_cpu_port(dev_id);
       port_id <= lld_get_max_cpu_port(dev_id);
       port_id++) {
    dev_port = MAKE_DEV_PORT(0, port_id);
    port_mgr_mac_basic_interrupt_teardown(dev_id, dev_port);
  }
  return BF_SUCCESS;
}

/** \brief port_mgr_port_pgm_speed_fec
 *         Program configured speed in umac
 *
 * \param dev_id   : bf_dev_id_t     : system-assigned identifier
 *(0..BF_MAX_DEV_COUNT-1)
 * \param dev_port : bf_dev_port_t   : encoded port
 * \param speed    : bf_port_speed_t : speed enum
 * \param fec      : bf_fec_type_t   : fec enum
 *
 * \return: nothing
 */
void port_mgr_port_pgm_speed_fec(bf_dev_id_t dev_id,
                                 bf_dev_port_t dev_port,
                                 bf_port_speed_t speed,
                                 bf_fec_type_t fec) {
  int mac_block, ch, is_cpu_port;
  port_mgr_port_t *port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  if (port_p == NULL) return;

  port_mgr_map_dev_port_to_all(
      dev_id, dev_port, NULL, NULL, &mac_block, &ch, &is_cpu_port);

  // set port speed and fec type
  switch (speed) {
    case BF_SPEED_100G:
      port_mgr_mac_set_speed_100g(dev_id,
                                  mac_block,
                                  ch,
                                  CMRA_PMA_BASE_R4,
                                  fec,
                                  port_p->sw.loopback_enabled);
      break;
    case BF_SPEED_50G:
      port_mgr_mac_set_speed_50g(dev_id,
                                 mac_block,
                                 ch,
                                 CMRA_PMA_BASE_R2,
                                 fec,
                                 port_p->sw.loopback_enabled);
      break;
    case BF_SPEED_40G:
      port_mgr_mac_set_speed_40g(dev_id,
                                 mac_block,
                                 ch,
                                 CMRA_PMA_BASE_R4,
                                 fec,
                                 port_p->sw.loopback_enabled);
      break;
    case BF_SPEED_25G:
      port_mgr_mac_set_speed_25g(dev_id,
                                 mac_block,
                                 ch,
                                 CMRA_PMA_BASE_R1,
                                 fec,
                                 port_p->sw.loopback_enabled);
      break;
    case BF_SPEED_10G:
      port_mgr_mac_set_speed_10g(dev_id,
                                 mac_block,
                                 ch,
                                 CMRA_PMA_BASE_R1,
                                 fec,
                                 port_p->sw.loopback_enabled);
      break;
    case BF_SPEED_1G:
      port_mgr_mac_set_speed_1g(dev_id,
                                mac_block,
                                ch,
                                CMRA_PMA_BASE_R1,
                                fec,
                                port_p->sw.loopback_enabled);
      break;
    default:
      bf_sys_assert(0);
  }
}

/********************************************************************
 * port_mgr_ch_reqd_by_speed
 *
 * Internal function to return the number of UMAC channels used
 * for a given speed mode.
 ********************************************************************/
int port_mgr_ch_reqd_by_speed(bf_dev_id_t dev_id,
                              bf_dev_port_t dev_port,
                              bf_port_speed_t speed) {
  int ch_reqd = 0, ch_mask = 0, ch = 0;

  port_mgr_map_dev_port_to_all(dev_id, dev_port, NULL, NULL, NULL, &ch, NULL);

  switch (speed) {
    case BF_SPEED_100G:
      ch_mask = 0xf;
      break;
    case BF_SPEED_50G:
      ch_mask = 0x3;
      break;
    case BF_SPEED_40G:
      ch_mask = 0xf;
      break;
    case BF_SPEED_25G:
      ch_mask = 0x1;
      break;
    case BF_SPEED_10G:
      ch_mask = 0x1;
      break;
    case BF_SPEED_1G:
      ch_mask = 0x1;
      break;
    case BF_SPEED_NONE:
      ch_mask = 0x0;
      break;
    default:
      bf_sys_assert(0);
  }
  return ch_reqd = ch_mask << ch;
}

/********************************************************************
 * port_mgr_ch_available
 *
 * Internal function to determine if the given port can be placed in
 * the given speed mode, which depends on the current configuration of
 * the other ports in the UMAC.
 *
 * Each UMAC channel that is actively being used by a port is saved
 * in the mac_block "ch_in_use" member.
 *
 * note: this function assumes the port is unassigned, which means
 *       no channels are currently being used by this port.
 *******************************************************************/
static int port_mgr_ch_available(bf_dev_id_t dev_id,
                                 bf_dev_port_t dev_port,
                                 bf_port_speed_t speed) {
  int mac_block, ch, ch_reqd, ch_in_use;
  port_mgr_mac_block_t *mac_block_p;

  port_mgr_map_dev_port_to_all(
      dev_id, dev_port, NULL, NULL, &mac_block, &ch, NULL);

  mac_block_p = port_mgr_tof1_map_idx_to_mac_block(dev_id, mac_block);
  if (mac_block_p == NULL) {
    port_mgr_log("device id %d is invalid, no mac block", dev_id);
    return 0;
  }

  ch_reqd = port_mgr_ch_reqd_by_speed(dev_id, dev_port, speed);
  ch_in_use = mac_block_p->ch_in_use;

  // now see if the new speed works
  if (ch_in_use & ch_reqd) {
    return 0;  // trying to add port when channel is already being used
  }
  return 1;
}

/********************************************************************
 * port_mgr_ch_in_use
 *
 * Internal function to return a bit mask of in-use channels on the
 * mac_block associated with the specified dev_port.
 *******************************************************************/
uint32_t port_mgr_ch_in_use(bf_dev_id_t dev_id, bf_dev_port_t dev_port) {
  port_mgr_mac_block_t *mac_block_p;
  int mac_block, ch;

  port_mgr_map_dev_port_to_all(
      dev_id, dev_port, NULL, NULL, &mac_block, &ch, NULL);
  mac_block_p = port_mgr_tof1_map_idx_to_mac_block(dev_id, mac_block);
  if (mac_block_p == NULL) {
    port_mgr_log("device id %d is invalid, no mac block", dev_id);
    return 0;
  }

  return (mac_block_p->ch_in_use);
}

/********************************************************************
 * port_mgr_construct_txff_ctrl
 ********************************************************************/
uint32_t port_mgr_construct_txff_ctrl(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port) {
  uint32_t chnl_seq, chnl_ena, chnl_mode0, chnl_mode1, ins_wait = 0;
  bf_dev_pipe_t log_pipe, pipe_id;
  int log_port_id, mac_block, port_id, ch, is_cpu_port;
  port_mgr_port_t *port_p;
  port_mgr_mac_block_t *mac_block_p;
  int spd[4];

  port_mgr_map_dev_port_to_all(
      dev_id, dev_port, &pipe_id, &port_id, &mac_block, NULL, &is_cpu_port);

  mac_block_p = port_mgr_tof1_map_idx_to_mac_block(dev_id, mac_block);
  if (mac_block_p == NULL) {
    port_mgr_log("device id %d is invalid, no mac block", dev_id);
    return 0;
  }

  chnl_ena = 0;

  // find the (up to) 4 ports involved
  log_pipe = DEV_PORT_TO_PIPE(dev_port);
  log_port_id = DEV_PORT_TO_LOCAL_PORT(dev_port);
  log_port_id = log_port_id & (~0x3);  // mask off channel

  for (ch = 0; ch < 4; ch++) {
    bf_dev_port_t dev_port_ch = MAKE_DEV_PORT(log_pipe, (log_port_id + ch));

    port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port_ch);
    if (port_p == NULL)
      spd[ch] = 0;
    else if (port_p->sw.speed == BF_SPEED_100G)
      spd[ch] = 100;
    else if (port_p->sw.speed == BF_SPEED_40G)
      spd[ch] = 40;
    else if (port_p->sw.speed == BF_SPEED_50G)
      spd[ch] = 50;
    else if (port_p->sw.speed == BF_SPEED_25G)
      spd[ch] = 25;
    else if (port_p->sw.speed == BF_SPEED_10G)
      spd[ch] = 10;
    else if (port_p->sw.speed == BF_SPEED_1G)
      spd[ch] = 1;
    else
      spd[ch] = 0;  // unused

    if (spd[ch] != 0) {
      chnl_ena |= (1 << ch);
    }
  }

  chnl_mode0 = 2;
  chnl_mode1 = 2;

  if ((spd[0] == 100) || (spd[0] == 40)) {
    chnl_seq = 0x00;  // 1-ch mode
    chnl_mode0 = 0;   // 100G
  } else {
    if ((spd[0] == 50) || (spd[0] == 42)) {
      chnl_mode0 = 1;  // 50G

      if ((spd[2] == 50) || (spd[2] == 42)) {
        chnl_mode1 = 1;   // 50G
        chnl_seq = 0x88;  // 2-ch mode
      } else {
        chnl_mode1 = 2;   // 25G or 10G
        chnl_seq = 0xC8;  // 3-ch mode Left
      }
    } else if ((spd[2] == 50) || (spd[2] == 42)) {
      chnl_mode1 = 1;   // 50G
      chnl_seq = 0x98;  // 3-ch mode Right
    } else {
      chnl_mode1 = 2;   // 25 or 10G
      chnl_seq = 0xD8;  // 4-ch mode
    }
  }

  // 4x10g 40G needs ins_wait set to 1
  if (spd[0] == 40) {
    ins_wait = 1;
  }

  mac_block_p->txff_ctrl = 0;

  // chnl_ena = mac_block_p->ch_in_use;
  setp_eth_regs_txff_ctrl_chnl_ena(&mac_block_p->txff_ctrl, chnl_ena);
  setp_eth_regs_txff_ctrl_chnl_seq(&mac_block_p->txff_ctrl, chnl_seq);
  setp_eth_regs_txff_ctrl_chnl_mode0(&mac_block_p->txff_ctrl, chnl_mode0);
  setp_eth_regs_txff_ctrl_chnl_mode1(&mac_block_p->txff_ctrl, chnl_mode1);

  port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  if (port_p == NULL) return 0;

  // fill out some defaults
  setp_eth_regs_txff_ctrl_txdispad(&mac_block_p->txff_ctrl,
                                   port_p->sw.txff_ctrl_pad_disable ? 1 : 0);
  setp_eth_regs_txff_ctrl_txdisfcs(
      &mac_block_p->txff_ctrl, port_p->sw.txff_ctrl_fcs_insert_disable ? 1 : 0);
  setp_eth_regs_txff_ctrl_crcrmv_dis(
      &mac_block_p->txff_ctrl,
      port_p->sw.txff_ctrl_crc_removal_disable ? 1 : 0);
  setp_eth_regs_txff_ctrl_crcchk_dis(
      &mac_block_p->txff_ctrl,
      port_p->sw.txff_ctrl_crc_check_disable ? 0xF : 0);
  setp_eth_regs_txff_ctrl_ins_ws(&mac_block_p->txff_ctrl, ins_wait);

  return mac_block_p->txff_ctrl;
}

/*****************************************************************************
 * port_mgr_tof1_get_num_lanes
 *
 * Internal function to return the number of "logical" lanes on a port.
 * Intention is to be called to determine "for loop" limits.
 *
 * The only valid num_lanes values are,
 *     1, for single lane (e.g. 1g, 10g, 25g)
 *     2, for two lane (e.g. 40g-mlg, 50g)
 *     4, for four lane (e.g. 40g, 100g)
 *****************************************************************************/
int port_mgr_tof1_get_num_lanes(bf_dev_id_t dev_id, bf_dev_port_t dev_port) {
  port_mgr_port_t *port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);

  if (!port_p) {
    return 0;
  }
  if (port_p->sw.speed == BF_SPEED_100G)
    return 4;
  else if (port_p->sw.speed == BF_SPEED_40G)
    return 4;
  else if (port_p->sw.speed == BF_SPEED_50G)
    return 2;
  else if (port_p->sw.speed == BF_SPEED_25G)
    return 1;
  else if (port_p->sw.speed == BF_SPEED_10G)
    return 1;
  else if (port_p->sw.speed == BF_SPEED_1G)
    return 1;
  else
    return 0;
}

/*****************************************************************************
 * port_mgr_dump_this_port_config
 *****************************************************************************/
int port_mgr_dump_this_port_config(ucli_context_t *uc,
                                   bf_dev_id_t dev_id,
                                   bf_dev_pipe_t pipe,
                                   int port) {
  port_mgr_port_t *port_p;
  bf_dev_port_t dev_port;
  bf_dev_pipe_t phy_pipe;
  int mac_block, ch, rc;
  port_mgr_serdes_t *serdes_p;
  // port_mgr_dev_t *dev_p = port_mgr_map_dev_id_to_dev_p(dev_id);
  port_mgr_ldev_t *dev_p = port_mgr_dev_logical_dev_get(dev_id);
  uint32_t lport_index;

  if (!dev_p) return port;
  if (!dev_p->assigned) return port;

  dev_port = MAKE_DEV_PORT(pipe, port);
  rc = port_mgr_map_dev_port_to_all(
      dev_id, dev_port, &phy_pipe, NULL, &mac_block, &ch, NULL);
  if (rc != 0) return port;  // invalid port for sku

  // generate logical port index from dev_port
  lport_index = port_mgr_tof1_map_dev_port_to_port_index(dev_id, dev_port);
  if (lport_index == (uint32_t)-1) return port;

  port_p = &dev_p->lport[lport_index];
  if ((port % 4) == 0) {
    if (lport_index >= sizeof(dev_p->lport) / sizeof(dev_p->lport[0]) - 3)
      return port;
    if (!dev_p->lport[lport_index + 0].sw.assigned &&
        !dev_p->lport[lport_index + 1].sw.assigned &&
        !dev_p->lport[lport_index + 2].sw.assigned &&
        !dev_p->lport[lport_index + 3].sw.assigned) {
      return port += 3;  // skip next 3 then
    }
  }

  if ((port % 4) == 0) {
    aim_printf(&uc->pvs,
               "-+---+---+---+---+-+---+---+---+---+----+-----+-----+-----+----"
               "---+---+"
               "---+"
               "---+----+\n");
  }

  serdes_p = port_mgr_tof1_map_port_lane_to_serdes_allow_unassigned(
      dev_id, dev_port, 0);
  if (serdes_p == NULL) {
    port_mgr_log("%s:%d Map dev %d port %d to serdes failed",
                 __func__,
                 __LINE__,
                 dev_id,
                 dev_port);
    return port;
  }

  aim_printf(
      &uc->pvs,
      "%d| %d | %d |%3d|%3d|%d| %d |%3d|%3d| %s | %s |%4s |%s |%5d| %d | %d "
      "|%2x "
      "|%2x | %d | %d  "
      "|\n",
      dev_id,
      phy_pipe,
      pipe,  // logical pipe
      port,
      mac_block,
      ch,
      serdes_p->ring,
      serdes_p->tx_sd,
      serdes_p->rx_sd,
      port_p->sw.enabled ? "E" : "-",
      port_p->sw.assigned == 0 ? "--" : port_p->sw.oper_state ? "Up" : "Dn",
      port_p->sw.assigned == 0
          ? "--"
          : port_p->sw.speed == BF_SPEED_100G
                ? "100g"
                : port_p->sw.speed == BF_SPEED_50G
                      ? " 50g"
                      : port_p->sw.speed == BF_SPEED_40G
                            ? " 40g"
                            : port_p->sw.speed == BF_SPEED_25G
                                  ? " 25g"
                                  : port_p->sw.speed == BF_SPEED_10G
                                        ? " 10g"
                                        : port_p->sw.speed == BF_SPEED_1G
                                              ? "  1g"
                                              : "????",
      port_p->sw.assigned == 0
          ? "----"
          : port_p->sw.fec == BF_FEC_TYP_NONE
                ? "none"
                : port_p->sw.fec == BF_FEC_TYP_FIRECODE
                      ? "fire"
                      : port_p->sw.fec == BF_FEC_TYP_REED_SOLOMON ? "reed"
                                                                  : "????",
      port_p->sw.tx_mtu,
      port_p->sw.link_pause_tx,
      port_p->sw.link_pause_rx,
      port_p->sw.pfc_pause_tx,
      port_p->sw.pfc_pause_rx,
      port_p->an_enabled,
      port_p->sw.loopback_enabled);
  return port;
}

void port_mgr_dump_port_config_banner(ucli_context_t *uc) {
  aim_printf(&uc->pvs, "\n");
  aim_printf(&uc->pvs,
             "-+---+---+---+---+-+---+---+---+---+----+-----+-----+-----+------"
             "-+---+--"
             "-+--"
             "-+----+\n");
  aim_printf(&uc->pvs,
             "c| p | p | i | m | |   serdes  |   |    |     |     |     | Link "
             " |      "
             " |  "
             " |    |\n");
  aim_printf(&uc->pvs,
             "h| i | i | b | a | +---+       |   |    |     |     |     | "
             "Pause |  PFC "
             " |  "
             " |    |\n");
  aim_printf(
      &uc->pvs,
      "i| p | p | u | c | | r |       |En | St |Speed| fec | mtu | Tx Rx | Tx "
      "Rx "
      "|AN |Lpbk|\n");
  aim_printf(&uc->pvs,
             "p| e | e | f | b | | i |       |   |    |     |     |     |      "
             " |      "
             " |  "
             " |    |\n");
  aim_printf(&uc->pvs,
             " |   |   |   | l |c| n +-------+   |    |     |     |     |      "
             " |      "
             " |  "
             " |    |\n");
  aim_printf(&uc->pvs,
             " |phy|log|   | k |h| g | tx| rx|   |    |     |     |     |      "
             " |      "
             " |  "
             " |    |\n");
}

void port_mgr_dump_port_config(ucli_context_t *uc) {
  bf_dev_id_t dev_id;
  uint32_t pipe, port, this_port;
  int ports_per_banner = 32, ports_dumped = 0;

  port_mgr_dump_port_config_banner(uc);

  for (dev_id = 0; dev_id < BF_MAX_DEV_COUNT; dev_id++) {
    if (!port_mgr_dev_is_ready(dev_id)) continue;

    for (pipe = 0; pipe < BF_PIPE_COUNT; pipe++) {
      for (port = (uint32_t)lld_get_min_fp_port(dev_id);
           port <= (uint32_t)lld_get_max_fp_port(dev_id);
           port++) {
        if (ports_dumped == ports_per_banner) {
          port_mgr_dump_port_config_banner(uc);
          ports_dumped = 0;
        }
        this_port = port;
        port = port_mgr_dump_this_port_config(uc, dev_id, pipe, port);
        if (port == this_port) {  // then it printed something
          ports_dumped++;
        }
      }
    }
    aim_printf(&uc->pvs,
               "-+---+---+---+---+-+---+---+---+---+----+-----+-----+-----+----"
               "---+---+--"
               "-+--"
               "-+----+\n");
    aim_printf(&uc->pvs, "Cpu port(s):\n");

    for (port = (uint32_t)lld_get_min_cpu_port(dev_id);
         port <= (uint32_t)lld_get_max_cpu_port(dev_id);
         port++) {
      port = port_mgr_dump_this_port_config(uc, dev_id, 0, port);
    }
    aim_printf(&uc->pvs,
               "-+---+---+---+---+-+---+---+---+---+----+-----+-----+-----+----"
               "---+---+--"
               "-+--"
               "-+----+\n");
  }
}

static void port_mgr_set_default_serdes_state(port_mgr_serdes_t *serdes_p) {
  // serdes_p->ring = 0; /* FIXME : Should we reset the ring? */
  // serdes_p->tx_sd = 0;
  // serdes_p->rx_sd = 0;
  serdes_p->tx_pll_clk = 0;  // Tx PLL clock source
  serdes_p->pll_ovrclk = 0;

  // Tx parameters
  serdes_p->tx_inv = 0;
  serdes_p->tx_eq_pre = 0;
  serdes_p->tx_eq_post = 0;
  serdes_p->tx_eq_atten = 0;
  // Rx parameters
  serdes_p->rx_inv = 0;
  serdes_p->rx_sig_ok_thresh = 0;
  serdes_p->rx_term = 0;
  serdes_p->rx_horz_eye = 0;
  serdes_p->rx_vert_eye = 0;

  // minimum "qualifying" eye heights at BER 1e06/1e10/1e12
  serdes_p->qualifying_eye_ht_1e06 = 0;
  serdes_p->qualifying_eye_ht_1e10 = 0;
  serdes_p->qualifying_eye_ht_1e12 = 0;
}

static void port_mgr_set_default_mac_block_state(
    port_mgr_mac_block_t *mac_block_p) {
  int i = 0;
  mac_block_p->ch_in_use = 0;
  mac_block_p->txff_ctrl = 0;
  for (i = 0; i < 4; i++) {
    // mac_block_p->sds_node[i] = 0;
    mac_block_p->tx_lane_map[i] = 0;
    mac_block_p->rx_lane_map[i] = 0;
    mac_block_p->hw_tx_lane_map[i] = 0;
    mac_block_p->hw_rx_lane_map[i] = 0;
    port_mgr_set_default_serdes_state(&mac_block_p->serdes[i]);
    port_mgr_set_default_serdes_state(&mac_block_p->hw_serdes[i]);
  }
}

void port_mgr_tof1_set_default_all_ports_state(bf_dev_id_t dev_id) {
  bf_dev_pipe_t pipe;
  int port_id;
  bf_dev_port_t dev_port;
  uint32_t num_pipes = 0;
  port_mgr_port_t *port_p;
  int mac_block, ch;
  port_mgr_mac_block_t *mac_block_p;

  lld_sku_get_num_active_pipes(dev_id, &num_pipes);
  for (pipe = 0; pipe < num_pipes; pipe++) {
    for (port_id = lld_get_min_fp_port(dev_id);
         port_id <= lld_get_max_fp_port(dev_id);
         port_id++) {
      dev_port = MAKE_DEV_PORT(pipe, port_id);
      port_p = port_mgr_map_dev_port_to_port_allow_unassigned(dev_id, dev_port);
      if (port_p != NULL) {
        port_mgr_set_default_port_state(dev_id, dev_port);
        /* For every 4 dev ports, there is a MAC block */
        if ((dev_port % lld_get_chnls_per_mac(dev_id)) != 0) {
          continue;
        }
        // Mac block fields
        port_mgr_map_dev_port_to_all(
            dev_id, dev_port, NULL, NULL, &mac_block, &ch, NULL);
        mac_block_p = port_mgr_tof1_map_idx_to_mac_block_allow_unassigned(
            dev_id, mac_block);
        if (mac_block_p != NULL) {
          port_mgr_set_default_mac_block_state(mac_block_p);
        }
      }
    }
  }

  for (port_id = lld_get_min_cpu_port(dev_id);
       port_id <= lld_get_max_cpu_port(dev_id);
       port_id++) {
    // one last one for the CPU port (mac-blk 64)
    dev_port = MAKE_DEV_PORT(0, port_id);  // CPU port
    port_p = port_mgr_map_dev_port_to_port_allow_unassigned(dev_id, dev_port);
    if (port_p != NULL) {
      port_mgr_set_default_port_state(dev_id, dev_port);
      /* For every 4 dev ports, there is a MAC block */
      if ((dev_port % lld_get_chnls_dev_port(dev_id, port_id)) != 0) {
        continue;
      }
      // Mac block fields
      port_mgr_map_dev_port_to_all(
          dev_id, dev_port, NULL, NULL, &mac_block, &ch, NULL);
      mac_block_p = port_mgr_tof1_map_idx_to_mac_block_allow_unassigned(
          dev_id, mac_block);
      if (mac_block_p != NULL) {
        port_mgr_set_default_mac_block_state(mac_block_p);
      }
    }
  }
}

/* Initialize the per port mutex for all the ports */
void port_mgr_init_port_mtx(bf_dev_id_t dev_id) {
  bf_dev_pipe_t pipe;
  int port_id;
  bf_dev_port_t dev_port;
  uint32_t num_pipes = 0;
  port_mgr_port_t *port_p;
  int mac_block, ch;
  port_mgr_mac_block_t *mac_block_p;

  lld_sku_get_num_active_pipes(dev_id, &num_pipes);
  for (pipe = 0; pipe < num_pipes; pipe++) {
    for (port_id = lld_get_min_fp_port(dev_id);
         port_id <= lld_get_max_fp_port(dev_id);
         port_id++) {
      dev_port = MAKE_DEV_PORT(pipe, port_id);
      port_p = port_mgr_map_dev_port_to_port_allow_unassigned(dev_id, dev_port);
      if (port_p != NULL) {
        bf_sys_mutex_init(&port_p->port_mtx);
      }

      if ((dev_port % lld_get_chnls_per_mac(dev_id)) != 0) {
        continue;
      }
      port_mgr_map_dev_port_to_all(
          dev_id, dev_port, NULL, NULL, &mac_block, &ch, NULL);
      mac_block_p = port_mgr_tof1_map_idx_to_mac_block_allow_unassigned(
          dev_id, mac_block);
      if (mac_block_p != NULL) {
        bf_sys_mutex_init(&mac_block_p->mac_stats_mtx);
      }
    }
  }

  for (port_id = lld_get_min_cpu_port(dev_id);
       port_id <= lld_get_max_cpu_port(dev_id);
       port_id++) {
    // one last one for the CPU port (mac-blk 64)
    dev_port = MAKE_DEV_PORT(0, port_id);  // CPU port
    port_p = port_mgr_map_dev_port_to_port_allow_unassigned(dev_id, dev_port);
    if (port_p != NULL) {
      bf_sys_mutex_init(&port_p->port_mtx);
    }

    if ((dev_port % lld_get_chnls_per_mac(dev_id)) != 0) {
      continue;
    }
    port_mgr_map_dev_port_to_all(
        dev_id, dev_port, NULL, NULL, &mac_block, &ch, NULL);
    mac_block_p =
        port_mgr_tof1_map_idx_to_mac_block_allow_unassigned(dev_id, mac_block);
    if (mac_block_p != NULL) {
      bf_sys_mutex_init(&mac_block_p->mac_stats_mtx);
    }
  }
}

/* De-Initialize the per port mutex for all the ports */
void port_mgr_deinit_port_mtx(bf_dev_id_t dev_id) {
  bf_dev_pipe_t pipe;
  int port_id;
  port_mgr_err_t err;
  bf_dev_port_t dev_port;
  uint32_t num_pipes = 0;
  port_mgr_port_t *port_p;
  int mac_block, ch;
  port_mgr_mac_block_t *mac_block_p;

  lld_sku_get_num_active_pipes(dev_id, &num_pipes);
  for (pipe = 0; pipe < num_pipes; pipe++) {
    for (port_id = lld_get_min_fp_port(dev_id);
         port_id <= lld_get_max_fp_port(dev_id);
         port_id++) {
      dev_port = MAKE_DEV_PORT(pipe, port_id);
      port_p = port_mgr_map_dev_port_to_port_allow_unassigned(dev_id, dev_port);
      if (port_p != NULL) {
        bf_sys_mutex_del(&port_p->port_mtx);
      }

      if ((dev_port % lld_get_chnls_per_mac(dev_id)) != 0) {
        continue;
      }
      err = port_mgr_map_dev_port_to_all(
          dev_id, dev_port, NULL, NULL, &mac_block, &ch, NULL);
      if (err != PORT_MGR_OK) {
        port_mgr_log_warn(
            "port_mgr_map_dev_port_to_all returned error code (%d)", err);
        continue;
      }
      mac_block_p = port_mgr_tof1_map_idx_to_mac_block_allow_unassigned(
          dev_id, mac_block);
      if (mac_block_p != NULL) {
        bf_sys_mutex_del(&mac_block_p->mac_stats_mtx);
      }
    }
  }

  for (port_id = lld_get_min_cpu_port(dev_id);
       port_id <= lld_get_max_cpu_port(dev_id);
       port_id++) {
    // one last one for the CPU port (mac-blk 64)
    dev_port = MAKE_DEV_PORT(0, port_id);  // CPU port
    port_p = port_mgr_map_dev_port_to_port_allow_unassigned(dev_id, dev_port);
    if (port_p != NULL) {
      bf_sys_mutex_del(&port_p->port_mtx);
    }

    if ((dev_port % lld_get_chnls_per_mac(dev_id)) != 0) {
      continue;
    }
    port_mgr_map_dev_port_to_all(
        dev_id, dev_port, NULL, NULL, &mac_block, &ch, NULL);
    mac_block_p =
        port_mgr_tof1_map_idx_to_mac_block_allow_unassigned(dev_id, mac_block);
    if (mac_block_p != NULL) {
      bf_sys_mutex_del(&mac_block_p->mac_stats_mtx);
    }
  }
}

bf_status_t port_mgr_port_tx_ignore_rx_set(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           bool en) {
  bf_status_t rc;

  rc = port_mgr_mac_tx_ignore_rx_set(dev_id, dev_port, en);
  return rc;
}
