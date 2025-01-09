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


#include <stdint.h>
#include <stdbool.h>

#include <bf_types/bf_types.h>
#include <dvm/bf_drv_intf.h>
#include <port_mgr/port_mgr_intf.h>
#include <port_mgr/port_mgr_ha.h>
#include <port_mgr/port_mgr.h>
#include <port_mgr/port_mgr_map.h>
#include <port_mgr/port_mgr_log.h>
#include "port_mgr_tof1_port.h"
#include "port_mgr_mac.h"
#include <port_mgr/bf_port_if.h>
#include <port_mgr/bf_serdes_if.h>
#include "port_mgr_serdes.h"
#include <port_mgr/bf_fsm_if.h>
#include "port_mgr/port_mgr_mac_stats.h"
// fwd ref
bf_status_t bf_fsm_ena_mac(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
bf_status_t bf_fsm_dfe_quick(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
bf_status_t bf_fsm_rs_up(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
bf_status_t bf_fsm_check_pcs_up(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
void bf_fsm_reset_rx_path(bf_dev_id_t dev_id,
                          bf_dev_port_t dev_port,
                          bool wait_for_unreset);
bf_status_t bf_fsm_start_adaptive_tuning(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port);
static bool bf_fsm_check_for_errored_blocks(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port);

/*****************************************************************************
 * bf_fsm_port_is_valid
 *
 * Internal function to determine whether port is still in-use (added).
 *****************************************************************************/
static bool bf_fsm_port_is_valid(bf_dev_id_t dev_id, bf_dev_port_t dev_port) {
  bf_status_t rc = bf_port_is_valid(dev_id, dev_port);

  return (rc == BF_SUCCESS);
}

/*****************************************************************************
 * bf_fsm_num_lanes_get
 *
 * Internal function to return number of logical lanes a port uses
 *****************************************************************************/
static int bf_fsm_num_lanes_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port) {
  int num_lanes;
  bf_status_t bf_status;

  bf_status = bf_port_num_lanes_get(dev_id, dev_port, &num_lanes);
  if (bf_status != BF_SUCCESS) {
    port_mgr_log("FSM :%d:%3d:-: Warn: bf_port_num_lanes_get rtnd err=%d",
                 dev_id,
                 dev_port,
                 bf_status);
    return 0;
  }
  return num_lanes;
}

/*****************************************************************************
 * bf_fsm_serdes_lane_map_get
 *
 * Internal function to return a 0-based bit map of logical serdes lanes
 * associated with a port based on its speed. Intention is to use this to
 * perform some operation on all lanes of a port.
 *
 * The only valid lane_maps are,
 *     0x1, for single lane (1g, 10g, 25g)
 *     0x3, for two lane (50g)
 *     0xF, for four lane (40g, 100g)
 *****************************************************************************/
static uint32_t bf_fsm_serdes_lane_map_get(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port) {
  int lane_map[5] = {0x0, 0x1, 0x3, 0x0, 0xF};
  int num_lanes = bf_fsm_num_lanes_get(dev_id, dev_port);

  return lane_map[num_lanes];
}

/*****************************************************************************
 * update_historical_mac_stats
 *
 * Reads HW mac stats and updates historical counters.
 *****************************************************************************/
static void update_historical_mac_stats(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port) {
  bf_rmon_counter_array_t hw_ctrs;
  port_mgr_port_t *port_p;
  bf_status_t bf_status;

  port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  if (port_p == NULL) {
    port_mgr_log("%d:%3d: Error : Invalid port updating historical ctrs",
                 dev_id,
                 dev_port);
    return;
  }

  bf_status = bf_port_mac_stats_hw_only_sync_get(dev_id, dev_port, &hw_ctrs);

  if (bf_status != BF_SUCCESS) {
    port_mgr_log("%d:%3d: Error : %d : updating historical ctrs",
                 dev_id,
                 dev_port,
                 bf_status);
  } else {
    port_mgr_mac_stats_copy(
        &hw_ctrs, &port_p->mac_stat_historical, &port_p->mac_stat_historical);
    port_mgr_log("%d:%3d: updated historical ctrs, wait for hw ctrs clear.",
                 dev_id,
                 dev_port);
  }
}

bf_status_t bf_fsm_pcs_rs_fec_block_error_validation(bf_dev_id_t dev_id,
                                                     bf_dev_port_t dev_port) {
  port_mgr_port_t *port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  if (!port_p) return BF_INVALID_ARG;

  if (port_p->sw.fec == BF_FEC_TYP_REED_SOLOMON &&
      (port_p->sw.speed == BF_SPEED_25G || port_p->sw.speed == BF_SPEED_50G ||
       port_p->sw.speed == BF_SPEED_100G)) {
    if (bf_fsm_check_for_errored_blocks(dev_id, dev_port)) {
      // There are errored blocks:
      // Keep the link down
      if (!port_p->fec_reset_inp) {
        port_p->fec_reset_inp = true;
        port_mgr_log("FSM :%d:%3d: erroredblockhi found. Reset the FEC in fsm",
                     dev_id,
                     dev_port);
        bf_fsm_reset_rx_path(dev_id, dev_port, true);
      }
      return BF_NOT_READY;
    } else if (port_p->fec_reset_inp) {
      port_mgr_log(
          "FSM :%d:%3d: erroredblockhi cleared in AN FSM.", dev_id, dev_port);
      port_p->fec_reset_inp = false;
    }
  }
  return BF_SUCCESS;
}

bf_status_t bf_fsm_pcs_no_fec_block_error_validation(bf_dev_id_t dev_id,
                                                     bf_dev_port_t dev_port) {
  uint32_t bip_errors_per_pcs_lane[20];
  uint32_t bip_err_summary = 0;
  uint32_t unknown_error_cnt;
  uint32_t invalid_error_cnt;
  uint32_t valid_error_cnt;
  uint32_t errored_blk_cnt;
  uint32_t block_lock_loss;
  port_mgr_port_t *port_p;
  bf_status_t bf_status;
  uint32_t hi_ber_cnt;
  uint32_t sync_loss;
  uint32_t ber_cnt;

  port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  if (!port_p) return BF_INVALID_ARG;

  if ((port_p->sw.speed == BF_SPEED_25G || port_p->sw.speed == BF_SPEED_10G) &&
      port_p->sw.fec == BF_FEC_TYP_NONE) {
    bf_status = bf_port_pcs_counters_get(dev_id,
                                         dev_port,
                                         &ber_cnt,
                                         &errored_blk_cnt,
                                         &sync_loss,
                                         &block_lock_loss,
                                         &hi_ber_cnt,
                                         &valid_error_cnt,
                                         &unknown_error_cnt,
                                         &invalid_error_cnt,
                                         bip_errors_per_pcs_lane);
    if (bf_status != BF_SUCCESS) return bf_status;

    port_mgr_log(
        "FSM :%d:%3d:-: PCS err-blk=%d : syn-lst=%d : bk-lk-lst=%d : "
        "hi_ber=%d : val-err=%d : unk-err=%d : inv-err=%d : bip=%d",
        dev_id,
        dev_port,
        errored_blk_cnt,
        sync_loss,
        block_lock_loss,
        hi_ber_cnt,
        valid_error_cnt,
        unknown_error_cnt,
        invalid_error_cnt,
        bip_err_summary);

    if (port_p->sw.single_lane_error_thresh != 0) {
      if (errored_blk_cnt >= port_p->sw.single_lane_error_thresh) {
        port_mgr_log(
            "FSM :%d:%3d:-: PCS : Errored block thresh exceeded "
            "[%d >= %d]",
            dev_id,
            dev_port,
            errored_blk_cnt,
            port_p->sw.single_lane_error_thresh);
        // too many errors, return an error code to force a FSM transition.
        return BF_INVALID_ARG;
      }
    } else {
      port_mgr_log("FSM :%d:%3d:-: PCS : Errored block validation disabled",
                   dev_id,
                   dev_port);
    }
  }
  return BF_SUCCESS;
}

bf_status_t bf_fsm_pcs_error_validation(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port) {
  port_mgr_port_t *port_p;
  bf_status_t bf_status;
  int los_ok;

  port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  if (!port_p) return BF_INVALID_ARG;

  // No additional validations for 1G
  if (port_p->sw.speed == BF_SPEED_1G) return BF_SUCCESS;

  // Check serdes LOS
  bf_status = bf_port_serdes_los_get(dev_id, dev_port, &los_ok);
  if (bf_status != BF_SUCCESS) return bf_status;
  if (!los_ok) {
    // LOS was detected.
    port_mgr_log("FSM :%d:%3d: detected LOS", dev_id, dev_port);
    return BF_INVALID_ARG;
  }

  // If FEC is enabled, check for block errors, and if any, reset RS-FEC
  if (port_p->sw.fec == BF_FEC_TYP_REED_SOLOMON) {
    bf_status = bf_fsm_pcs_rs_fec_block_error_validation(dev_id, dev_port);
  } else {
    // This exec path is for 10G & 25G without FEC: check number of bad blocks
    bf_status = bf_fsm_pcs_no_fec_block_error_validation(dev_id, dev_port);
  }

  return bf_status;
}

/**
 * @addtogroup lld-port-fsm
 * @{
 * This is a description of some APIs.
 */

/** \brief Initial AN/LT FSM state. Lane 0 serdes programmed for AN speed
 *         other lanes programmed for expected HCD with tx_output_en=0
 *
 * \param dev_id  : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param dev_port: encoded port identifier
 *
 * \return: BF_SUCCESS
 * \return: BF_INVALID_ARG: invalid dev_id
 * \return: BF_INVALID_ARG: invalid dev_port
 *
 */
bf_status_t bf_fsm_an_init_serdes(bf_dev_id_t dev_id, bf_dev_port_t dev_port) {
  bf_status_t rc;
  int ln;
  int num_lanes = bf_fsm_num_lanes_get(dev_id, dev_port);
  port_mgr_port_t *port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  if (port_p == NULL) return BF_INVALID_ARG;

  if (!bf_fsm_port_is_valid(dev_id, dev_port)) return BF_INVALID_ARG;

  // Initialize link fault status var.
  port_p->fsm_ext.link_fault_status = BF_PORT_LINK_FAULT_LOC_FAULT;
  port_p->fsm_ext.alt_link_fault_st = BF_PORT_LINK_FAULT_LOC_FAULT;
  port_p->fsm_ext.port_was_up = false;

  // Clear mac link up override, if any, for normal mode.
  bf_port_mac_set_tx_mode(dev_id, dev_port, false);

  /* Release MAC serdes sig_ok indication so MAC gets the real status */
  port_mgr_log(
      "FSM :%d:%3d:-: MACs serdesmux.sig_ok=PASS-THRU", dev_id, dev_port);
  bf_port_un_force_sig_ok_low_set(dev_id, dev_port);

  for (ln = 0; ln < num_lanes; ln++) {
    if (ln == 0) {
      rc = bf_serdes_init(dev_id,
                          dev_port,
                          ln,
                          1 /* reset*/,
                          1 /*init_mode*/,
                          bf_serdes_pll_divider_get(125),
                          bf_serdes_data_width_get(125),
                          0 /*FIXME: phase_cal*/,
                          0 /*1*/ /*output_en*/);
    } else {
      // pre-program the non-AN lanes for the expected speed,
      // but leave tx_output_en=0
      rc = bf_serdes_init(dev_id,
                          dev_port,
                          ln,
                          1 /* reset*/,
                          1 /*init_mode*/,
                          bf_serdes_pll_divider_get(port_p->sw.speed),
                          bf_serdes_data_width_get(port_p->sw.speed),
                          0 /*FIXME: phase_cal*/,
                          0 /*output_en*/);
    }
    if (rc != BF_SUCCESS) {
      port_mgr_log("FSM :%d:%3d:%d: Error: bf_fsm_an_init_serdes: rc=%d",
                   dev_id,
                   dev_port,
                   ln,
                   rc);
      return rc;
    }
  }
  bf_port_an_lt_stats_init(dev_id, dev_port, false);
  return BF_SUCCESS;
}

/** \brief Wait for serdes PLLs to lock to requested value
 *
 * \param dev_id  : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param dev_port: encoded port identifier
 *
 * \return: BF_SUCCESS
 * \return: BF_INVALID_ARG: invalid dev_id
 * \return: BF_INVALID_ARG: invalid dev_port
 *
 */
bf_status_t bf_fsm_an_wait_pll1(bf_dev_id_t dev_id, bf_dev_port_t dev_port) {
  bf_status_t rc;
  bf_link_training_mode_t mode;
  int ln;
  int num_lanes = bf_fsm_num_lanes_get(dev_id, dev_port);
  bool lt_disabled = false;
  bool rx_locked = false, tx_locked = false;
  port_mgr_port_t *port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  if (port_p == NULL) return BF_INVALID_ARG;

  if (!bf_fsm_port_is_valid(dev_id, dev_port)) return BF_INVALID_ARG;

  for (ln = 0; ln < num_lanes; ln++) {
    rc = bf_serdes_rx_and_tx_lock_get(
        dev_id, dev_port, ln, &rx_locked, &tx_locked);
    if (rc != BF_SUCCESS) return BF_INVALID_ARG;

    if (!rx_locked || !tx_locked) {
      port_mgr_log("FSM :%d:%3d:%d: PLL not ready: tx=%s : rx=%s",
                   dev_id,
                   dev_port,
                   ln,
                   tx_locked ? "locked" : "not ready",
                   rx_locked ? "locked" : "not ready");
      break;
    }
  }
  if (!rx_locked || !tx_locked) {
    return BF_NOT_READY;
  }

  rc = bf_port_lt_disable_get(dev_id, dev_port, &lt_disabled);
  if (rc != BF_SUCCESS) {
    port_mgr_log(
        "FSM :%d:%3d:-: Error: %d : getting link-training disable setting",
        dev_id,
        dev_port,
        rc);
    return BF_INVALID_ARG;
  }

  if (port_p->sw.speed == BF_SPEED_100G) {
    mode = BF_CLAUSE_92_LINK_TRAINING;
  } else {
    mode = BF_CLAUSE_72_LINK_TRAINING;
  }

  // pre-set expected training mode for non-AN lanes
  // AN lane (0) will be reset after AN so it can't be set here
  for (ln = 1; ln < num_lanes; ln++) {
    rc = bf_serdes_link_training_mode_set(dev_id, dev_port, ln, mode);
    if (rc != BF_SUCCESS) {
      port_mgr_log("FSM :%d:%3d:%d: Error: %d : setting link-training mode",
                   dev_id,
                   dev_port,
                   ln,
                   rc);
    }
  }
  // configure polarity swaps etc
  // for lane 0, set Tx Eq parms correctly to help AN
  // for other lanes, preset Tx Eq settings to 0' for link-training
  for (ln = 0; ln < num_lanes; ln++) {
    bool set_tx_eq = ((ln == 0) || lt_disabled) ? true : false;
    rc = bf_serdes_params_an_set(dev_id, dev_port, ln, set_tx_eq);
    if (rc != BF_SUCCESS) {
      return BF_INVALID_ARG;
    }
  }
  /* reset Next Page counters */
  port_p->cur_next_pg = 0;
  port_p->an_lp_num_next_pages = 0;
  port_p->an_lp_base_page = 0ull;

  /* initiate AN */
  rc = bf_port_autoneg_restart_set(
      dev_id,
      dev_port,
      true /*ignore nonce match, allow AN with lpbk modules*/,
      false /*defaut link-inhibit timer*/);
  if (rc != BF_SUCCESS) {
    return rc;
  }

  // now, turn on tx_output_en on lane 0
  rc = bf_serdes_tx_drv_en_set(dev_id, dev_port, 0, true);
  port_mgr_log(
      "FSM :%d:%3d:0: Set tx_output_en 1 <rc=%d>", dev_id, dev_port, rc);
  if (rc != BF_SUCCESS) {
    return BF_INVALID_ARG;
  }

  return BF_SUCCESS;
}

/** \brief Wait for LP Base Page to be received
 *
 * \param dev_id  : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param dev_port: encoded port identifier
 *
 * \return: BF_SUCCESS
 * \return: BF_INVALID_ARG: invalid dev_id
 * \return: BF_INVALID_ARG: invalid dev_port
 *
 */
bf_status_t bf_fsm_an_wait_base_pg(bf_dev_id_t dev_id, bf_dev_port_t dev_port) {
  bf_status_t rc;
  bool lp_base_pg_rdy;
  bool lp_next_pg_rdy;
  bool an_good;
  bool an_complete;
  bool an_failed;
  uint64_t lp_base_pg;
  port_mgr_port_t *port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  if (port_p == NULL) return BF_INVALID_ARG;

  if (!bf_fsm_port_is_valid(dev_id, dev_port)) return BF_INVALID_ARG;

  rc = bf_serdes_autoneg_all_state(dev_id,
                                   dev_port,
                                   0,
                                   &lp_base_pg_rdy,
                                   &lp_next_pg_rdy,
                                   &an_good,
                                   &an_complete,
                                   &an_failed);
  if (rc != BF_SUCCESS) return BF_INVALID_ARG;
  if (an_failed) {
    port_mgr_log("FSM :%d:%3d:0: AN Failed. Restart..", dev_id, dev_port);
    return BF_INVALID_ARG;  // went to "parallel detection fault" state
  }
  if (!lp_base_pg_rdy) {
    return BF_NOT_READY;
  }

  rc = bf_serdes_an_lp_base_page_get(dev_id, dev_port, &lp_base_pg);
  if (rc != BF_SUCCESS) return BF_INVALID_ARG;

  /* if AN_GOOD already, skip to next state */
  // if (an_good) return BF_SUCCESS;
  // if (an_complete) return BF_SUCCESS; // ??

  // if we have Next pages to send, load the first now
  if (port_p->an_num_next_pages > 0) {  // we have Next pages
    // load first Next Page
    rc = bf_serdes_autoneg_next_pg_set(
        dev_id, dev_port, 0, port_p->an_next_page[0]);

    port_mgr_log("FSM :%d:%3d:0: -- NEXT PG: %04x_%04x_%04x",
                 dev_id,
                 dev_port,
                 (uint32_t)((port_p->an_next_page[0] >> 32ull) & 0xFFFFull),
                 (uint32_t)((port_p->an_next_page[0] >> 16ull) & 0xFFFFull),
                 (uint32_t)(port_p->an_next_page[0] & 0xFFFFull));

    port_p->cur_next_pg = 1;
  }
  // if LP has Next pages to send, load NULL Next page for us
  else if (lp_base_pg & (1ull << 15)) {  // LP has next pages
    // set NULL Page
    rc = bf_serdes_autoneg_next_pg_set(dev_id, dev_port, 0, 0ull);
  }
  if (rc != BF_SUCCESS) return BF_INVALID_ARG;
  return BF_SUCCESS;
}

/** \brief Wait for link-partners Next Page to be received
 *
 * \param dev_id  : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param dev_port: encoded port identifier
 *
 * \return: BF_SUCCESS
 * \return: BF_INVALID_ARG: invalid dev_id
 * \return: BF_INVALID_ARG: invalid dev_port
 *
 */
bf_status_t bf_fsm_an_wait_next_pg(bf_dev_id_t dev_id, bf_dev_port_t dev_port) {
  bf_status_t rc;
  bool lp_base_pg_rdy;
  bool lp_next_pg_rdy;
  bool an_good;
  bool an_complete;
  bool an_failed;
  uint64_t lp_next_pg;
  port_mgr_port_t *port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  if (port_p == NULL) return BF_INVALID_ARG;

  if (!bf_fsm_port_is_valid(dev_id, dev_port)) return BF_INVALID_ARG;

  rc = bf_serdes_autoneg_all_state(dev_id,
                                   dev_port,
                                   0,
                                   &lp_base_pg_rdy,
                                   &lp_next_pg_rdy,
                                   &an_good,
                                   &an_complete,
                                   &an_failed);
  if (rc != BF_SUCCESS) return BF_INVALID_ARG;
  if (an_failed) {
    port_mgr_log("FSM :%d:%3d:0: AN Failed. Restart..", dev_id, dev_port);
    return BF_INVALID_ARG;
  }
  /* if AN_GOOD already, skip to next state. This would be the
   * case where neither us nor LP had NP's to send */
  if (an_good) {
    port_mgr_log("FSM :%d:%3d:0: AN GOOD (early) an_cmpl=%d",
                 dev_id,
                 dev_port,
                 an_complete);
    return BF_SUCCESS;
  }
  if (an_complete) return BF_SUCCESS;

  if (!lp_next_pg_rdy) return BF_NOT_READY;

  rc = bf_serdes_autoneg_lp_next_pg_get(dev_id, dev_port, 0, &lp_next_pg);
  if (rc != BF_SUCCESS) return BF_INVALID_ARG;

  port_p->an_lp_next_page[port_p->an_lp_num_next_pages] = lp_next_pg;
  port_p->an_lp_num_next_pages++;

  port_mgr_log("FSM :%d:%3d:0: LP NEXT PG: %04x_%04x_%04x",
               dev_id,
               dev_port,
               (uint32_t)((lp_next_pg >> 32ull) & 0xFFFFull),
               (uint32_t)((lp_next_pg >> 16ull) & 0xFFFFull),
               (uint32_t)(lp_next_pg & 0xFFFFull));

  /* if we have more Next Pages, send the next one.
   * Note: May need to set ACK here, not sure
   */
  if (port_p->an_num_next_pages > port_p->cur_next_pg) {
    // load next Next Page
    rc = bf_serdes_autoneg_next_pg_set(
        dev_id, dev_port, 0, port_p->an_next_page[port_p->cur_next_pg]);
    if (rc != BF_SUCCESS) return BF_INVALID_ARG;

    port_mgr_log(
        "FSM :%d:%3d:0: -- NEXT PG: %04x_%04x_%04x",
        dev_id,
        dev_port,
        (uint32_t)((port_p->an_next_page[port_p->cur_next_pg] >> 32ull) &
                   0xFFFFull),
        (uint32_t)((port_p->an_next_page[port_p->cur_next_pg] >> 16ull) &
                   0xFFFFull),
        (uint32_t)(port_p->an_next_page[port_p->cur_next_pg] & 0xFFFFull));

    port_p->cur_next_pg++;
    return BF_NOT_READY;
  }
  // if LP has more Next pages to send, load NULL Next page for us
  else if (lp_next_pg & (1ull << 15)) {  // LP has more next pages
    // set NULL Page
    rc = bf_serdes_autoneg_next_pg_set(dev_id, dev_port, 0, 0ull);
    if (rc != BF_SUCCESS) return BF_INVALID_ARG;
    return BF_NOT_READY;
  }

  return BF_SUCCESS;
}

/** \brief Wait for AN to determine HCD speed
 *
 * \param dev_id  : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param dev_port: encoded port identifier
 *
 * \return: BF_SUCCESS
 * \return: BF_INVALID_ARG: invalid dev_id
 * \return: BF_INVALID_ARG: invalid dev_port
 *
 */
bf_status_t bf_fsm_an_wait_an_good(bf_dev_id_t dev_id, bf_dev_port_t dev_port) {
  bf_status_t rc;
  bf_port_speed_t hcd = 0;
  bf_fec_type_t fec = 0;
  bool an_good;
  port_mgr_port_t *port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  if (port_p == NULL) return BF_INVALID_ARG;

  if (!bf_fsm_port_is_valid(dev_id, dev_port)) return BF_INVALID_ARG;

  rc = bf_serdes_an_good_get(dev_id, dev_port, &an_good);

  if (rc != BF_SUCCESS) {
    port_mgr_log("FSM :%d:%3d:-: Error: bf_serdes_an_good_get: rc=%d",
                 dev_id,
                 dev_port,
                 rc);
    return rc;
  }
  if (!an_good) {
    return BF_NOT_READY;
  }
  /* Need to do software-based resolution here in Consortium cases */
  rc = bf_port_autoneg_hcd_fec_get_v2(dev_id, dev_port, &hcd, NULL, &fec);
  if (rc != BF_SUCCESS) {
    port_mgr_log("FSM :%d:%3d:0: Error: bf_port_autoneg_hcd_fec_get_v2: rc=%d",
                 dev_id,
                 dev_port,
                 rc);
    return rc;
  }
  if (port_p->sw.speed != hcd) {
    port_mgr_log(
        "FSM :%d:%3d:-: Error: HCD(%d)/port speed(%s) mismatch. Must re-add "
        "port",
        dev_id,
        dev_port,
        hcd,
        spd_to_str(port_p->sw.speed));
    return BF_HW_COMM_FAIL;
  }
  port_p->sw.speed = hcd;

  rc = bf_serdes_init(dev_id,
                      dev_port,
                      0 /*lane*/,
                      0 /* reset*/,
                      1 /*init_mode*/,
                      bf_serdes_pll_divider_get(port_p->sw.speed),
                      bf_serdes_data_width_get(port_p->sw.speed),
                      0 /*FIXME: phase_cal*/,
                      0 /*output_en*/);
  if (rc != BF_SUCCESS) {
    port_mgr_log("FSM :%d:%3d:%d: Error: bf_serdes_init: rc=%d <ln=0>",
                 dev_id,
                 dev_port,
                 0,
                 rc);
    return rc;
  }

  bf_port_an_lt_start_time_set(dev_id, dev_port);
  bf_port_an_try_inc(dev_id, dev_port);

  return BF_SUCCESS;
}

/** \brief Wait for PLLs to lock to HCD speed
 *
 * \param dev_id  : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param dev_port: encoded port identifier
 *
 * \return: BF_SUCCESS
 * \return: BF_INVALID_ARG: invalid dev_id
 * \return: BF_INVALID_ARG: invalid dev_port
 *
 */
bf_status_t bf_fsm_an_wait_pll2(bf_dev_id_t dev_id, bf_dev_port_t dev_port) {
  bf_status_t rc;
  int ln = 0;
  port_mgr_port_t *port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  int num_lanes = bf_fsm_num_lanes_get(dev_id, dev_port);
  bool rx_locked, tx_locked;
  bool lt_disabled = false;
  bf_port_speed_t hcd = 0;  // for checking AN s/m hasnt restarted
  bf_fec_type_t fec = 0;    // for checking AN s/m hasnt restarted

  if (port_p == NULL) return BF_INVALID_ARG;

  if (!bf_fsm_port_is_valid(dev_id, dev_port)) return BF_INVALID_ARG;

  // only AN lane was re-programmed, so only wait for lane 0
  rc =
      bf_serdes_rx_and_tx_lock_get(dev_id, dev_port, 0, &rx_locked, &tx_locked);
  if (rc != BF_SUCCESS) {
    port_mgr_log("FSM :%d:%3d:-: Error: %d : getting PLL Lock status",
                 dev_id,
                 dev_port,
                 rc);
    return BF_INVALID_ARG;
  }
  if (!rx_locked || !tx_locked) {
    port_mgr_log("FSM :%d:%3d:%d: PLL not ready: tx=%s : rx=%s",
                 dev_id,
                 dev_port,
                 0,
                 tx_locked ? "locked" : "not ready",
                 rx_locked ? "locked" : "not ready");
    return BF_NOT_READY;
  }

  rc = bf_port_lt_disable_get(dev_id, dev_port, &lt_disabled);
  if (rc != BF_SUCCESS) {
    port_mgr_log(
        "FSM :%d:%3d:-: Error: %d : getting link-training disable setting",
        dev_id,
        dev_port,
        rc);
    return BF_INVALID_ARG;
  }

  // We have AN_GOOD and all lanes are now programmed with the hcd speed
  if (lt_disabled) {
    // AN lane needs to get p/n swaps etc applied again
    // set Tx Eq parms here since link-training is skipped
    rc = bf_serdes_params_an_set(dev_id, dev_port, 0, true);
    if (rc != BF_SUCCESS) {
      return BF_INVALID_ARG;
    }

    rc = bf_serdes_assert_hcd_link_status(dev_id, dev_port, 0, port_p->av_hcd);
    if (rc != BF_SUCCESS) {
      port_mgr_log("FSM :%d:%3d:%d: Warning: Assert HCD link-status failed",
                   dev_id,
                   dev_port,
                   0);
      return BF_INVALID_ARG;
    }
    port_mgr_log(
        "FSM :%d:%3d:%d: Note: Link-training disabled", dev_id, dev_port, 0);
  } else {
    bf_link_training_mode_t mode;

    // set link-training mode on AN lane
    if (port_p->sw.speed == BF_SPEED_100G) {
      mode = BF_CLAUSE_92_LINK_TRAINING;
    } else {
      mode = BF_CLAUSE_72_LINK_TRAINING;
    }
    rc = bf_serdes_link_training_mode_set(dev_id, dev_port, 0, mode);
    if (rc != BF_SUCCESS) {
      port_mgr_log("FSM :%d:%3d:%d: Error: %d : setting link-training mode",
                   dev_id,
                   dev_port,
                   0,
                   rc);
    }

    // AN lane needs to get p/n swaps etc applied again
    // dont set Tx Eq parms here. Link-training needs them reset
    rc = bf_serdes_params_an_set(dev_id, dev_port, 0, false);
    if (rc != BF_SUCCESS) {
      return BF_INVALID_ARG;
    }

    rc = bf_serdes_assert_hcd_link_status(dev_id, dev_port, 0, port_p->av_hcd);
    if (rc != BF_SUCCESS) {
      port_mgr_log("FSM :%d:%3d:%d: Warning: Assert HCD link-status failed",
                   dev_id,
                   dev_port,
                   0);
      return BF_INVALID_ARG;
    }

    // launch training (note: output driver still off)
    for (ln = 0; ln < num_lanes; ln++) {
      if (!lt_disabled) {
        rc = bf_serdes_link_training_set(dev_id, dev_port, ln, true);
        if (rc != BF_SUCCESS) {
          port_mgr_log("FSM :%d:%3d:%d: Warning: Training enable failed",
                       dev_id,
                       dev_port,
                       ln);
          return BF_INVALID_ARG;
        }
      }
    }
    // verify HCD is still valid (i.e. AN s/m has not reset
    rc = bf_port_autoneg_hcd_fec_get_v2(dev_id, dev_port, &hcd, NULL, &fec);
    if (rc != BF_SUCCESS) {
      port_mgr_log(
          "FSM :%d:%3d:0: Error: bf_port_autoneg_hcd_fec_get_v2(2): rc=%d",
          dev_id,
          dev_port,
          rc);
      return BF_INVALID_ARG;
    }
    if (port_p->sw.speed != hcd) {
      port_mgr_log(
          "FSM :%d:%3d:0: WARNING: HCD reset. Restart AN", dev_id, dev_port);
      return BF_INVALID_ARG;
    }
  }

  // Disable CH RX, Enable TX
  // note: this also takes care of the case where the FSM is switched
  //       from non-AN to this one and the Rx was disabled in the other
  //       FSM.
  port_mgr_log(
      "FSM :%d:%3d:-: Enable MAC CH Tx : Disable MAC Rx", dev_id, dev_port);
  port_mgr_mac_set_tx_enable(dev_id, dev_port, true);
  port_mgr_mac_set_rx_enable(dev_id, dev_port, false);
  port_mgr_mac_set_tx_enable(dev_id, dev_port, true);

  // toggle swreset
  bf_port_mac_ch_enable_set(dev_id, dev_port, false);
  bf_port_mac_ch_enable_set(dev_id, dev_port, true);

  // turn on the output driver
  for (ln = 0; ln < num_lanes; ln++) {
    rc = bf_serdes_tx_drv_en_set(dev_id, dev_port, ln, true);
    if (rc != BF_SUCCESS) {
      port_mgr_log("FSM :%d:%3d:%d: Warning: Tx output driver enable failed",
                   dev_id,
                   dev_port,
                   ln);
      return BF_INVALID_ARG;
    }
  }

  if (lt_disabled) {
    bf_fsm_dfe_quick(dev_id, dev_port);
  }
  return BF_SUCCESS;
}

/** \brief Wait for AN + link-training to complete
 *
 * \param dev_id  : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param dev_port: encoded port identifier
 *
 * \return: BF_SUCCESS
 * \return: BF_INVALID_ARG: invalid dev_id
 * \return: BF_INVALID_ARG: invalid dev_port
 *
 */
bf_status_t bf_fsm_an_wait_an_cmplt(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port) {
  bf_status_t rc;
  bool an_cmplt;
  int num_lanes;
  int ln, log_it = 0;
  bf_lt_state_e lt_st, lt[4] = {BF_LT_ST_NONE};
  port_mgr_port_t *port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  bool lt_disabled = false;

  if (port_p == NULL) return BF_INVALID_ARG;

  if (!bf_fsm_port_is_valid(dev_id, dev_port)) return BF_INVALID_ARG;

  rc = bf_port_lt_disable_get(dev_id, dev_port, &lt_disabled);
  if (rc != BF_SUCCESS) {
    port_mgr_log(
        "FSM :%d:%3d:-: Error: %d : getting link-training disable setting",
        dev_id,
        dev_port,
        rc);
    return BF_INVALID_ARG;
  }

  num_lanes = bf_fsm_num_lanes_get(dev_id, dev_port);
  if (!lt_disabled) {
    // verify link-training is complete
    for (ln = 0; ln < num_lanes; ln++) {
      rc = bf_serdes_link_training_st_get(dev_id, dev_port, ln, &lt_st);
      if (rc != BF_SUCCESS) {
        port_mgr_log(
            "FSM :%d:%3d:%d: Error: Failed to get link-training status: rc=%d",
            dev_id,
            dev_port,
            ln,
            rc);
      }
      lt[ln] = lt_st;
      if ((lt_st == BF_LT_ST_COMPLETE) || (lt_st == BF_LT_ST_FAILED))
        log_it = 1;
    }

    if (log_it) {
      port_mgr_log(
          "FSM :%d:%3d:-: LT: | %8s | %8s | %8s | %8s |",
          dev_id,
          dev_port,
          ((lt[0] == BF_LT_ST_COMPLETE)
               ? "complete"
               : (lt[0] == BF_LT_ST_RUNNING)
                     ? "running "
                     : (lt[0] == BF_LT_ST_FAILED)
                           ? "failed  "
                           : (lt[0] == BF_LT_ST_NONE) ? "none    " : "?"),
          ((lt[1] == BF_LT_ST_COMPLETE)
               ? "complete"
               : (lt[1] == BF_LT_ST_RUNNING)
                     ? "running "
                     : (lt[1] == BF_LT_ST_FAILED)
                           ? "failed  "
                           : (lt[1] == BF_LT_ST_NONE) ? "none    " : "?"),
          ((lt[2] == BF_LT_ST_COMPLETE)
               ? "complete"
               : (lt[2] == BF_LT_ST_RUNNING)
                     ? "running "
                     : (lt[2] == BF_LT_ST_FAILED)
                           ? "failed  "
                           : (lt[2] == BF_LT_ST_NONE) ? "none    " : "?"),
          ((lt[3] == BF_LT_ST_COMPLETE)
               ? "complete"
               : (lt[3] == BF_LT_ST_RUNNING)
                     ? "running "
                     : (lt[3] == BF_LT_ST_FAILED)
                           ? "failed  "
                           : (lt[3] == BF_LT_ST_NONE) ? "none    " : "?"));
    }
    for (ln = 0; ln < num_lanes; ln++) {
      if (lt[ln] == BF_LT_ST_RUNNING) {
        return BF_NOT_READY;
      }
      if (lt[ln] == BF_LT_ST_FAILED) {
        return BF_INVALID_ARG;
      }
      if (lt[ln] == BF_LT_ST_NONE) {
        return BF_INVALID_ARG;
      }
    }

    rc = bf_serdes_an_complete_get(dev_id, dev_port, &an_cmplt);
    if (rc != BF_SUCCESS) {
      return BF_INVALID_ARG;  // error
    }
    if (!an_cmplt) {
      return BF_NOT_READY;
    }
  } else {
    int lane_map = bf_fsm_serdes_lane_map_get(dev_id, dev_port);
    int dfe_done_map = 0;
    bool dfe_running = false;
    bf_port_speed_t hcd = 0;
    bf_fec_type_t fec = 0;

    /* Check HCD. If it has been cleared (=0xF) it indicates the AN block
     * has restarted AN, meaning link-training timed out */
    rc = bf_port_autoneg_hcd_fec_get_v2(dev_id, dev_port, &hcd, NULL, &fec);
    if (rc != BF_SUCCESS) {
      port_mgr_log(
          "FSM :%d:%3d:0: Error: bf_port_autoneg_hcd_fec_get_v2: rc=%d",
          dev_id,
          dev_port,
          rc);
      bf_port_an_lt_dur_set(dev_id, dev_port);
      return BF_INVALID_ARG;
    }
    if (port_p->sw.speed != hcd) {
      port_mgr_log(
          "FSM :%d:%3d:0: Error: Link-training timed out <hcd=%d>... restart "
          "FSM ...",
          dev_id,
          dev_port,
          hcd);
      bf_port_an_lt_dur_set(dev_id, dev_port);
      return BF_INVALID_ARG;
    }

    for (ln = 0; ln < num_lanes; ln++) {
      rc = bf_serdes_get_dfe_running(dev_id, dev_port, ln, &dfe_running);
      if (rc != BF_SUCCESS) {
        port_mgr_log("FSM :%d:%3d:%d: Error: bf_serdes_get_dfe_running: rc=%d",
                     dev_id,
                     dev_port,
                     ln,
                     rc);
        return rc;
      }
      if (dfe_running) {
        return BF_NOT_READY;
      } else {
        dfe_done_map |= (1 << ln);
      }
    }
    if (dfe_done_map != lane_map) {
      return BF_NOT_READY;
    }
  }
  // enable Rx path (now that training is done)
  port_mgr_log("FSM :%d:%3d:-: Enable MAC Rx", dev_id, dev_port);
  port_mgr_mac_set_rx_enable(dev_id, dev_port, true);

  for (ln = 0; ln < num_lanes; ln++) {
    bf_serdes_start_dfe_pcal(dev_id, dev_port, ln);
  }
  bf_port_an_lt_dur_set(dev_id, dev_port);

  return BF_SUCCESS;
}

/** \brief Initialize serdes for non-AN mode
 *
 * \param dev_id  : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param dev_port: encoded port identifier
 *
 * \return: BF_SUCCESS
 * \return: BF_INVALID_ARG: invalid dev_id
 * \return: BF_INVALID_ARG: invalid dev_port
 *
 * Initial Conditions:
 *
 * +-[ MAC ]---------------------------+
 * |   Tx En           : DISABLED      |
 * |   Rx En           : DISABLED      |
 * |   RS (LF/RF)      : Fault State   |
 * |   PCS/FEC (Rx)    : Not Syncd     |
 * +-[ Serdes ]------------------------+
 * |   Tx En           : DISABLED      |
 * |   Tx PLL          : Not Locked    |
 * |   Tx Output En    : DISABLED      |
 * |   Tx Data         : X             |
 * |   Rx En           : DISABLED      |
 * |   Rx PLL          : Not Locked    |
 * |   SigOkEnable     : X             |
 * |   SigOk           : DE-ASSERTED   |
 * |   Rx Data         : Not Equalized |
 * +-[ QSFP ]--------------------------+
 * |   resetL          : DE-ASSERTED   |
 * |   LPMode (hw)     : ASSERTED      |
 * |   LPMode (sw ovrd): ASSERT HiPwr  |
 * |   TX_DISABLE      : ASSERTED      |
 * |   Tx CDR          : Not Locked    |
 * |   Rx (optical) LOS: X             |
 * +-----------------------------------+
 *
 * State Actions:
 *   Serdes Initialization
 *     Tx En   = ENABLED
 *     Rx En   = ENABLED
 *     Tx Data = CORE DATA
 */
bf_status_t bf_fsm_init_serdes(bf_dev_id_t dev_id, bf_dev_port_t dev_port) {
  bf_status_t rc;
  int ln, is_emu = 0;
  int num_lanes = bf_fsm_num_lanes_get(dev_id, dev_port);
  port_mgr_port_t *port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  if (port_p == NULL) return BF_INVALID_ARG;

  if (!bf_fsm_port_is_valid(dev_id, dev_port)) return BF_INVALID_ARG;

  // Initialize link fault status var.
  port_p->fsm_ext.link_fault_status = BF_PORT_LINK_FAULT_LOC_FAULT;
  port_p->fsm_ext.alt_link_fault_st = BF_PORT_LINK_FAULT_LOC_FAULT;
  port_p->fsm_ext.port_was_up = false;

#if defined(DEVICE_IS_EMULATOR)
  is_emu = 1;
#endif

  if (is_emu) {
    /* force MAC serdes sig_ok indication to "down" until after DFE */
    port_mgr_log("FSM :%d:%3d:-: MACs serdesmux.sig_ok=HIGH", dev_id, dev_port);
    bf_port_force_sig_ok_high_set(dev_id, dev_port);
    return BF_SUCCESS;
  }
  /* force MAC serdes sig_ok indication to "down" until after DFE */
  port_mgr_log("FSM :%d:%3d:-: MACs serdesmux.sig_ok=LOW", dev_id, dev_port);
  bf_port_force_sig_ok_low_set(dev_id, dev_port);
  for (ln = 0; ln < num_lanes; ln++) {
    rc = bf_serdes_init(dev_id,
                        dev_port,
                        ln,
                        1 /*reset*/,
                        1 /*init_mode*/,
                        bf_serdes_pll_divider_get(port_p->sw.speed),
                        bf_serdes_data_width_get(port_p->sw.speed),
                        0 /*FIXME: phase_cal*/,
                        0 /*1*/ /*output_en*/);
    if (rc != BF_SUCCESS) {
      port_mgr_log("FSM :%d:%3d:%d: Error: bf_serdes_init: rc=%d",
                   dev_id,
                   dev_port,
                   ln,
                   rc);
      return rc;
    }
  }
  // restore flag indicating adaptive dfe enabled
  port_p->lstate.adative_dfe_enabled = false;

  return BF_SUCCESS;
}

/** \brief Re-enable serdes Rx
 *
 * \param dev_id  : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param dev_port: encoded port identifier
 *
 * \return: BF_SUCCESS
 * \return: BF_INVALID_ARG: invalid dev_id
 * \return: BF_INVALID_ARG: invalid dev_port
 *
 */
bf_status_t bf_fsm_re_init_serdes_rx(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port) {
  bf_status_t rc;
  int ln;
  int num_lanes = bf_fsm_num_lanes_get(dev_id, dev_port);

  if (!bf_fsm_port_is_valid(dev_id, dev_port)) return BF_INVALID_ARG;

  /* force MAC serdes sig_ok indication to "down" until after DFE */
  port_mgr_log("FSM :%d:%3d:-: MACs serdesmux.sig_ok=LOW", dev_id, dev_port);
  bf_port_force_sig_ok_low_set(dev_id, dev_port);

  port_mgr_log("FSM :%d:%3d:-: Serdes Rx=ON", dev_id, dev_port);

  for (ln = 0; ln < num_lanes; ln++) {
    rc = bf_serdes_rx_en_set(dev_id, dev_port, ln, true);
    if (rc != BF_SUCCESS) {
      port_mgr_log("FSM :%d:%3d:%d: Error: bf_serdes_re_init_rx: rc=%d",
                   dev_id,
                   dev_port,
                   ln,
                   rc);
      return rc;
    }
  }
  return BF_SUCCESS;
}

/** \brief Wait for serdes PLLs to lock to requested value
 *
 * \param dev_id  : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param dev_port: encoded port identifier
 *
 * \return: BF_SUCCESS
 * \return: BF_INVALID_ARG: invalid dev_id
 * \return: BF_INVALID_ARG: invalid dev_port
 *
 * Initial Conditions:
 *
 * +-[ MAC ]---------------------------+
 * |   Tx En           : DISABLED      |
 * |   Rx En           : DISABLED      |
 * |   RS (LF/RF)      : Fault State   |
 * |   PCS/FEC (Rx)    : Not Syncd     |
 * +-[ Serdes ]------------------------+
 * |   Tx En           : ENABLED       |
 * |   Tx PLL          : Not Locked    |
 * |   Tx Output En    : DISABLED      |
 * |   Tx Data         : CORE DATA     |
 * |   Rx En           : ENABLED       |
 * |   Rx PLL          : Not Locked    |
 * |   SigOkEnable     : X             |
 * |   SigOk           : DE-ASSERTED   |
 * |   Rx Data         : Not Equalized |
 * +-[ QSFP ]--------------------------+
 * |   resetL          : DE-ASSERTED   |
 * |   LPMode (hw)     : ASSERTED      |
 * |   LPMode (sw ovrd): ASSERT HiPwr  |
 * |   TX_DISABLE      : ASSERTED      |
 * |   Tx CDR          : Not Locked    |
 * |   Rx (optical) LOS: X             |
 * +-----------------------------------+
 *
 * State Actions:
 *   Serdes Initialization
 *     Wait for
 *       (Tx PLL = Locked) && (Rx PLL = Locked)
 */
bf_status_t bf_fsm_wait_pll(bf_dev_id_t dev_id, bf_dev_port_t dev_port) {
  int ln;
  bool rx_locked = false, tx_locked = false;
  int num_lanes = bf_fsm_num_lanes_get(dev_id, dev_port);
  port_mgr_port_t *port_p;

  if (!bf_fsm_port_is_valid(dev_id, dev_port)) return BF_INVALID_ARG;

  port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  if (port_p == NULL) return BF_INVALID_ARG;

  bool is_sw_model = false;
  // API requires data from a MAC register which is not
  // implemented on the model. just skip it ..
  bf_drv_device_type_get(dev_id, &is_sw_model);
  if (is_sw_model) {
    return BF_SUCCESS;
  }

#if defined(DEVICE_IS_EMULATOR)
  return BF_SUCCESS;
#endif

  for (ln = 0; ln < num_lanes; ln++) {
    bf_serdes_rx_and_tx_lock_get(dev_id, dev_port, ln, &rx_locked, &tx_locked);
    if (!rx_locked || !tx_locked) {
      port_mgr_log("FSM :%d:%3d:%d: PLL not ready: tx=%s : rx=%s",
                   dev_id,
                   dev_port,
                   ln,
                   tx_locked ? "locked" : "not ready",
                   rx_locked ? "locked" : "not ready");
      break;
    }
  }
  if (!rx_locked || !tx_locked) {
    return BF_NOT_READY;
  }
  return BF_SUCCESS;
}

/** \brief Program the basic serdes parameters
 *
 * \param dev_id  : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param dev_port: encoded port identifier
 *
 * \return: BF_SUCCESS
 * \return: BF_INVALID_ARG: invalid dev_id
 * \return: BF_INVALID_ARG: invalid dev_port
 *
 * Initial Conditions:
 *
 * +-[ MAC ]---------------------------+
 * |   Tx En           : DISABLED      |
 * |   Rx En           : DISABLED      |
 * |   RS (LF/RF)      : Fault State   |
 * |   PCS/FEC (Rx)    : Not Syncd     |
 * +-[ Serdes ]------------------------+
 * |   Tx En           : ENABLED       |
 * |   Tx PLL          : Locked        |
 * |   Tx Output En    : DISABLED      |
 * |   Tx Data         : CORE DATA     |
 * |   Rx En           : ENABLED       |
 * |   Rx PLL          : Locked        |
 * |   SigOkEnable     : X             |
 * |   SigOk           : DE-ASSERTED   |
 * |   Rx Data         : Not Equalized |
 * +-[ QSFP ]--------------------------+
 * |   resetL          : DE-ASSERTED   |
 * |   LPMode (hw)     : ASSERTED      |
 * |   LPMode (sw ovrd): ASSERT HiPwr  |
 * |   TX_DISABLE      : ASSERTED      |
 * |   Tx CDR          : Not Locked    |
 * |   Rx (optical) LOS: X             |
 * +-----------------------------------+
 *
 * State Actions:
 *   MAC:
 *     Force LF     = True (to hold peer down)
 *     Rx En        = DISABLED (duplicate)?
 *     Tx En        = ENABLED
 *   Serdes Initialization
 *     Tx Data      = CORE DATA (duplicate?)
 *     SigOkEnable  = ENABLED
 *     Tx Output En = ENABLED
 */
bf_status_t bf_fsm_config_serdes(bf_dev_id_t dev_id, bf_dev_port_t dev_port) {
  bf_status_t rc;
  int ln, is_emu = 0;
  int num_lanes = bf_fsm_num_lanes_get(dev_id, dev_port);

  if (!bf_fsm_port_is_valid(dev_id, dev_port)) return BF_INVALID_ARG;

#if defined(DEVICE_IS_EMULATOR)  // Emulator
  is_emu = 1;
#endif

  if (!is_emu) {
    for (ln = 0; ln < num_lanes; ln++) {
      rc = bf_serdes_params_set(dev_id, dev_port, ln);
      if (rc != BF_SUCCESS) {
        return BF_INVALID_ARG;
      }
    }
  }

  // Clear mac link up override, if any, for normal mode.
  bf_port_mac_set_tx_mode(dev_id, dev_port, false);

  // Toggle MAC soft reset.
  // Rx path is held down by serdesmux.serdessigovrrd
  // MAC tx_en/rx_en de-asserted so no pkts to/from app logic
  port_mgr_log(
      "FSM :%d:%3d:-: Disable MAC CH TX/RX, toggle swreset", dev_id, dev_port);
  port_mgr_mac_set_tx_enable(dev_id, dev_port, false);
  port_mgr_mac_set_rx_enable(dev_id, dev_port, false);
  port_mgr_mac_set_tx_auto_drain_on_disable(dev_id, dev_port, false);
  // toggle swreset
  bf_port_mac_ch_enable_set(dev_id, dev_port, false);
  bf_port_mac_ch_enable_set(dev_id, dev_port, true);

  if (!is_emu) {
    // now, turn on tx_output_en
    for (ln = 0; ln < num_lanes; ln++) {
      rc = bf_serdes_tx_drv_en_set(dev_id, dev_port, ln, true);
      if (rc != BF_SUCCESS) {
        return BF_INVALID_ARG;
      }
    }
    port_mgr_log("FSM :%d:%3d:-: Serdes TX driver ON", dev_id, dev_port);
  }

  return BF_SUCCESS;
}

/*
 * Enables TX, Disables RX for both mac and serdes.
 * Enable serdes-mux-loop
 */
bf_status_t bf_fsm_config_for_tx_mode(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port) {
  bf_status_t rc;
  int ln;
  int num_lanes = bf_fsm_num_lanes_get(dev_id, dev_port);

  if (!bf_fsm_port_is_valid(dev_id, dev_port)) return BF_INVALID_ARG;

  // Disable CH RX, but turn on TX
  port_mgr_log("FSM :%d:%3d:-: Disable MAC RX", dev_id, dev_port);

  // Disable RX-path and make sure Tx-path is enabled
  port_mgr_mac_set_rx_enable(dev_id, dev_port, false);
  port_mgr_mac_set_tx_enable(dev_id, dev_port, true);
  for (ln = 0; ln < num_lanes; ln++) {
    rc = bf_serdes_rx_en_set(dev_id, dev_port, ln, false);
    if (rc != BF_SUCCESS) {
      return BF_INVALID_ARG;
    }
  }

  bf_port_mac_ch_enable_set(dev_id, dev_port, true);

  for (ln = 0; ln < num_lanes; ln++) {
    rc = bf_serdes_params_set(dev_id, dev_port, ln);
    if (rc != BF_SUCCESS) {
      return BF_INVALID_ARG;
    }
  }

  port_mgr_log("FSM :%d:%3d:-: set MAC in tx-mode-oper", dev_id, dev_port);
  bf_port_mac_set_tx_mode(dev_id, dev_port, true);

  /* Release MAC serdes sig_ok indication so MAC gets the real status */
  port_mgr_log(
      "FSM :%d:%3d:-: MACs serdesmux.sig_ok=PASS-THRU", dev_id, dev_port);
  bf_port_un_force_sig_ok_low_set(dev_id, dev_port);

  for (ln = 0; ln < num_lanes; ln++) {
    // now, turn on tx_output_en
    rc = bf_serdes_tx_drv_en_set(dev_id, dev_port, ln, true);
    if (rc != BF_SUCCESS) {
      return BF_INVALID_ARG;
    }
  }
  port_mgr_log("FSM :%d:%3d:-: Serdes TX driver ON", dev_id, dev_port);
  return BF_SUCCESS;
}

/** \brief Re-Program the basic serdes Rx parameters
 *
 * \param dev_id  : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param dev_port: encoded port identifier
 *
 * \return: BF_SUCCESS
 * \return: BF_INVALID_ARG: invalid dev_id
 * \return: BF_INVALID_ARG: invalid dev_port
 *
 */
bf_status_t bf_fsm_re_config_serdes_rx(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port) {
  bf_status_t rc;
  int ln;
  int num_lanes = bf_fsm_num_lanes_get(dev_id, dev_port);

  if (!bf_fsm_port_is_valid(dev_id, dev_port)) return BF_INVALID_ARG;

  // Toggle MAC soft reset.
  // Rx path is held down by serdesmux.serdessigovrrd
  // MAC tx_en/rx_en de-asserted so no pkts to/from app logic
  port_mgr_log(
      "FSM :%d:%3d:-: Disable MAC CH TX/RX, toggle swreset", dev_id, dev_port);
  port_mgr_mac_set_tx_enable(dev_id, dev_port, false);
  port_mgr_mac_set_rx_enable(dev_id, dev_port, false);

  for (ln = 0; ln < num_lanes; ln++) {
    rc = bf_serdes_params_set(dev_id, dev_port, ln);
    if (rc != BF_SUCCESS) {
      return BF_INVALID_ARG;
    }
  }

  return BF_SUCCESS;
}

/** \brief Poll for signal_ok from serdes and start DFE if requested
 *
 * \param dev_id  : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param dev_port: encoded port identifier
 *
 * \return: BF_SUCCESS
 * \return: BF_INVALID_ARG: invalid dev_id
 * \return: BF_INVALID_ARG: invalid dev_port
 *
 * Initial Conditions:
 *
 * +-[ MAC ]---------------------------+
 * |   Tx En           : ENABLED       |
 * |   Rx En           : DISABLED      |
 * |   RS (LF/RF)      : Fault State   |
 * |   PCS/FEC (Rx)    : Not Syncd     |
 * +-[ Serdes ]------------------------+
 * |   Tx En           : ENABLED       |
 * |   Tx PLL          : Locked        |
 * |   Tx Output En    : ENABLED       |
 * |   Tx Data         : CORE DATA     |
 * |   Rx En           : ENABLED       |
 * |   Rx PLL          : Locked        |
 * |   SigOkEnable     : ENABLED       |
 * |   SigOk           : ?             |
 * |   Rx Data         : Not Equalized |
 * +-[ QSFP ]--------------------------+
 * |   resetL          : DE-ASSERTED   |
 * |   LPMode (hw)     : ASSERTED      |
 * |   LPMode (sw ovrd): ASSERT HiPwr  |
 * |   TX_DISABLE      : ASSERTED      |
 * |   Tx CDR          : Not Locked    |
 * |   Rx (optical) LOS: X             |
 * +-----------------------------------+
 *
 * State Actions:
 *   MAC:
 *   Serdes Initialization:
 *     Waits for serdes sgnal detected
 *     If A0, detection didn't work, so just assume its detected
 *   QSFP:
 *     if optical
 *       Waits for optical QSFP intialization, signaled by
 *         optical_xcvr_ready.
 *       Waits for optical LOS=0 (optical signal present)
 *
 */
bf_status_t bf_fsm_wait_signal_ok(bf_dev_id_t dev_id, bf_dev_port_t dev_port) {
  bf_status_t rc;
  int sig_ok_map = 0;
  bool sig_ok, is_optical = 0, optical_los = 0, optical_xcvr_ready = 0;
  int ln;
  int num_lanes = bf_fsm_num_lanes_get(dev_id, dev_port);
  int lane_map = bf_fsm_serdes_lane_map_get(dev_id, dev_port);
  port_mgr_port_t *port_p;

  port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  if (!port_p) return BF_INVALID_ARG;

#if defined(DEVICE_IS_EMULATOR)
  return BF_SUCCESS;
#endif
  // check optical LOS if optical link
  rc = bf_port_is_optical_xcvr_get(dev_id, dev_port, &is_optical);
  if (rc != BF_SUCCESS) {
    port_mgr_log("FSM :%d:%3d: Error: bf_port_is_optical_xcvr_get: rc=%d",
                 dev_id,
                 dev_port,
                 rc);
  }
  if (is_optical) {
    rc = bf_port_optical_xcvr_ready_get(dev_id, dev_port, &optical_xcvr_ready);
    if (rc != BF_SUCCESS) {
      port_mgr_log("FSM :%d:%3d: Error: bf_port_optical_xcvr_ready_get: rc=%d",
                   dev_id,
                   dev_port,
                   rc);
    }
    if (!optical_xcvr_ready) {
      return BF_NOT_READY;
    }

    rc = bf_port_optical_los_get(dev_id, dev_port, &optical_los);
    if (rc != BF_SUCCESS) {
      port_mgr_log("FSM :%d:%3d: Error: bf_port_optical_los_get: rc=%d",
                   dev_id,
                   dev_port,
                   rc);
    }
    if (optical_los) {
      return BF_NOT_READY;
    }
    /* set debounce for optical modules */
    port_p->fsm_ext.debounce_thr = BF_FSM_PCS_LOCK_THRSHLD_OPT;
  } else if (port_p->sw.fec == BF_FEC_TYP_NONE) {
    /* set debounce for non FEC Ethernet modes (excl optical modules) */
    port_p->fsm_ext.debounce_thr = BF_FSM_PCS_LOCK_THRSHLD_NO_FEC;
  } else {
    /* set default debounce threshold */
    port_p->fsm_ext.debounce_thr = BF_FSM_PCS_LOCK_THRSHLD;
  }

  port_mgr_log("FSM :%d:%3d: is_opt=%d : is_ready=%d : LOS=%d debounce_thr=%d",
               dev_id,
               dev_port,
               is_optical,
               optical_xcvr_ready,
               optical_los,
               port_p->fsm_ext.debounce_thr);

  for (ln = 0; ln < num_lanes; ln++) {
    lld_err_t err;
    bf_sku_chip_part_rev_t rev_no;

    /* A0 (rev_no==0) use sig_ok
     *  B0 (rev_no==1) use !los
     */
    err = lld_sku_get_chip_part_revision_number(dev_id, &rev_no);
    if ((err == LLD_OK) && rev_no != 0) {
      bool los = true;
      bool ei = true;
      bool flock = false;
      bool failed = false;

      /* Revision is B0 or later: log status of LOS, flock, EI and cal status */

      rc = bf_serdes_get_los(dev_id, dev_port, ln, &los);
      if (rc != BF_SUCCESS) {
        port_mgr_log("FSM :%d:%3d:%d: Error reading LOS", dev_id, dev_port, ln);
      }
      rc = bf_serdes_elec_idle_get(dev_id, dev_port, ln, &ei);
      if (rc != BF_SUCCESS) {
        port_mgr_log("FSM :%d:%3d:%d: Error reading EI", dev_id, dev_port, ln);
      }
      rc = bf_serdes_frequency_lock_get(dev_id, dev_port, ln, &flock);
      if (rc != BF_SUCCESS) {
        port_mgr_log("FSM :%d:%3d:%d: Error reading frequency lock",
                     dev_id,
                     dev_port,
                     ln);
      }
      rc = bf_serdes_calibration_status_get(dev_id, dev_port, ln, &failed);
      if (rc != BF_SUCCESS) {
        port_mgr_log("FSM :%d:%3d:%d: Error reading serdes calibration status",
                     dev_id,
                     dev_port,
                     ln);
      }

      port_mgr_log("FSM :%d:%3d:%d: LOS=%d EI=%d flock= %d calibration=%s",
                   dev_id,
                   dev_port,
                   ln,
                   los ? 1 : 0,
                   ei ? 1 : 0,
                   flock ? 1 : 0,
                   failed ? "failed" : "ok");
      // Assume sig_ok is good.
      sig_ok = 1;
    } else {
      rc = bf_serdes_get_signal_ok(dev_id, dev_port, ln, &sig_ok);
      port_mgr_log("FSM :%d:%3d:%d: A0 sig_ok=%d <err=%d>",
                   dev_id,
                   dev_port,
                   ln,
                   sig_ok,
                   rc);
      sig_ok = 1;  // For A0, pretty much just have to assume its good..
    }

    if (sig_ok) {
      sig_ok_map |= (1 << ln);
    }
  }
  if (sig_ok_map == lane_map) {
    return BF_SUCCESS;
  }
  return BF_NOT_READY;
}

/** \brief Start quick DFE
 *
 * \param dev_id  : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param dev_port: encoded port identifier
 *
 * \return: BF_SUCCESS
 * \return: BF_INVALID_ARG: invalid dev_id
 * \return: BF_INVALID_ARG: invalid dev_port
 *
 * Initial Conditions:
 *
 * +-[ MAC ]---------------------------+
 * |   Tx En           : ENABLED       |
 * |   Rx En           : DISABLED      |
 * |   RS (LF/RF)      : Fault State   |
 * |   PCS/FEC (Rx)    : Not Syncd     |
 * +-[ Serdes ]------------------------+
 * |   Tx En           : ENABLED       |
 * |   Tx PLL          : Locked        |
 * |   Tx Output En    : ENABLED       |
 * |   Tx Data         : CORE DATA     |
 * |   Rx En           : ENABLED       |
 * |   Rx PLL          : Locked        |
 * |   SigOkEnable     : ENABLED       |
 * |   SigOk           : OK            |
 * |   Rx Data         : Not Equalized |
 * +-[ QSFP ]--------------------------+
 * |   resetL          : DE-ASSERTED   |
 * |   LPMode (hw)     : ASSERTED      |
 * |   LPMode (sw ovrd): ASSERT HiPwr  |
 * |   TX_DISABLE      : DE-ASSERTED   |
 * |   Tx CDR          : Locked        |
 * |   Rx (optical) LOS: DE-ASSERTED   |
 * +-----------------------------------+
 *
 * State Actions:
 *   MAC:
 *     if 1G, enable Rx
 *   Serdes Initialization:
 *     Initiate DFE ICAL
 *
 */
bf_status_t bf_fsm_dfe_quick(bf_dev_id_t dev_id, bf_dev_port_t dev_port) {
  bf_status_t rc = BF_NOT_READY;
  int ln;
  int num_lanes = bf_fsm_num_lanes_get(dev_id, dev_port);
  port_mgr_port_t *port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  if (port_p == NULL) return BF_INVALID_ARG;

  if (!bf_fsm_port_is_valid(dev_id, dev_port)) return BF_INVALID_ARG;

  // no need for DFE on 1G links
  if (port_p->sw.speed == BF_SPEED_1G) {
    port_mgr_mac_set_rx_enable(dev_id, dev_port, true);
    return BF_SUCCESS;
  }
#if defined(DEVICE_IS_EMULATOR)
  return BF_SUCCESS;
#endif

  for (ln = 0; ln < num_lanes; ln++) {
    if (0) {
      /* check and reset LOS */
      bool rx_los_1, rx_los_2;
      bf_serdes_rx_afe_los_get(dev_id, dev_port, ln, &rx_los_1);
      bf_serdes_rx_afe_los_get(dev_id, dev_port, ln, &rx_los_2);
      port_mgr_log("FSM :%d:%3d:%d: Start ICAL : los1=%d : los2=%d",
                   dev_id,
                   dev_port,
                   ln,
                   rx_los_1,
                   rx_los_2);
    }

    rc = bf_serdes_start_dfe_ical(dev_id, dev_port, ln);
    if (rc != BF_SUCCESS) {
      port_mgr_log("FSM :%d:%3d:%d: Error: %d: from bf_serdes_start_dfe_ical",
                   dev_id,
                   dev_port,
                   ln,
                   rc);
      return rc;
    }
  }
  // restore flag indicating adaptive dfe enabled
  port_p->lstate.adative_dfe_enabled = false;
  // reset configured min eye hts (if any)
  bf_port_eye_quality_reset(dev_id, dev_port);
  return BF_SUCCESS;
}

/** \brief Poll for DFE done
 *
 * \param dev_id  : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param dev_port: encoded port identifier
 *
 * \return: BF_SUCCESS
 * \return: BF_INVALID_ARG: invalid dev_id
 * \return: BF_INVALID_ARG: invalid dev_port
 *
 * Initial Conditions:
 *
 * +-[ MAC ]---------------------------+
 * |   Tx En           : ENABLED       |
 * |   Rx En           : DISABLED      |
 * |   RS (LF/RF)      : Fault State   |
 * |   PCS/FEC (Rx)    : Not Syncd     |
 * +-[ Serdes ]------------------------+
 * |   Tx En           : ENABLED       |
 * |   Tx PLL          : Locked        |
 * |   Tx Output En    : ENABLED       |
 * |   Tx Data         : CORE DATA     |
 * |   Rx En           : ENABLED       |
 * |   Rx PLL          : Locked        |
 * |   SigOkEnable     : ENABLED       |
 * |   SigOk           : OK            |
 * |   Rx Data         : DFE BUSY      |
 * +-[ QSFP ]--------------------------+
 * |   resetL          : DE-ASSERTED   |
 * |   LPMode (hw)     : ASSERTED      |
 * |   LPMode (sw ovrd): ASSERT HiPwr  |
 * |   TX_DISABLE      : DE-ASSERTED   |
 * |   Tx CDR          : Locked        |
 * |   Rx (optical) LOS: DE-ASSERTED   |
 * +-----------------------------------+
 *
 * State Actions:
 *   MAC:
 *     Rx En    = True
 *     RS LF/RF = False
 *     Rx Path reset
 *     if 100G RS FEC, RS FEC s/m reset
 *   Serdes Initialization:
 *     Wait for DFE ICAL to complete
 *
 */
bf_status_t bf_fsm_wait_dfe_done(bf_dev_id_t dev_id, bf_dev_port_t dev_port) {
  int mac_block, ch, is_cpu_port;
  int ln, dfe_done_map = 0;
  int dfe_retry_map = 0;
  bool dfe_running = false;
  bf_status_t rc;
  int los_ok;
  int num_lanes = bf_fsm_num_lanes_get(dev_id, dev_port);
  int lane_map = bf_fsm_serdes_lane_map_get(dev_id, dev_port);
  port_mgr_port_t *port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  if (port_p == NULL) return BF_INVALID_ARG;

  if (!bf_fsm_port_is_valid(dev_id, dev_port)) return BF_INVALID_ARG;

  // no need for DFE on 1G links
  if (port_p->sw.speed == BF_SPEED_1G) {
    /* Release MAC serdes sig_ok indication so MAC gets the real status */
    port_mgr_log(
        "FSM :%d:%3d:-: MACs serdesmux.sig_ok=PASS-THRU", dev_id, dev_port);
    bf_port_un_force_sig_ok_low_set(dev_id, dev_port);

    port_mgr_log(
        "FSM :%d:%3d:-: Enable MAC TX and RX for app logic", dev_id, dev_port);
    port_mgr_mac_set_rx_enable(dev_id, dev_port, true);
    port_mgr_mac_set_tx_enable(dev_id, dev_port, true);
    return BF_SUCCESS;
  }

#if defined(DEVICE_IS_EMULATOR)
  port_mgr_log(
      "FSM :%d:%3d:-: Enable MAC TX and RX for app logic", dev_id, dev_port);
  port_mgr_mac_set_rx_enable(dev_id, dev_port, true);
  port_mgr_mac_set_tx_enable(dev_id, dev_port, true);
  return BF_SUCCESS;
#endif

  for (ln = 0; ln < num_lanes; ln++) {
    rc = bf_serdes_get_dfe_running(dev_id, dev_port, ln, &dfe_running);
    if (rc != BF_SUCCESS) {
      port_mgr_log("FSM :%d:%3d:%d: Error: bf_serdes_get_dfe_running: rc=%d",
                   dev_id,
                   dev_port,
                   ln,
                   rc);
      return rc;
    }
    if (!dfe_running) {
      dfe_done_map |= (1 << ln);
      // bool rx_los;
      // bf_serdes_rx_afe_los_get(dev_id, dev_port, ln, &rx_los);
      // if (rx_los) {
      //  port_mgr_log("FSM :%d:%3d:%d: ICAL done, but LOS occurred during",
      //               dev_id,
      //               dev_port,
      //               ln);
      //}
    }
  }
  if (dfe_done_map == lane_map) {
    /* Eye quality check
     *
     * This algorithm attempts to optimize the Rx eye while bounding
     * the number of retry attempts for a given lane.
     * The user can set a "goal" eye height for 3 BER's using the
     * API, bf_port_eye_quality_set()
     *
     * The goal height, if not set, is 0, meaning we will accept anything.
     *
     * Here we check the measured eye height against the goal height for
     * each BER. If the measured height is higher then continue on to the
     * PCS error check in the FSM.
     *
     * If the measured height is below the current goal value, then the goal
     * value is reduced (essentially, lowering our expectations). The amount
     * of downward adjustment is currently being tuned. Currently we use,
     *
     * for BER 1e06, half the distance between goal and measured, 10mV minimum
     * for BER 1e10, 2/3rds the distance between goal and measured, 10mV minimum
     * for BER 1e12, 3/4ths the distance between goal and measured, 10mV minimum
     *
     * The rationale for the difference is the decreasing accuracy of the
     *measured
     * values at higher BER.
     *
     * An additional heuristic is to track the best measured value reported for
     *each
     * lane at each BER. The goal height maintains a lower-bound that is 90%
     * of the best value reported since the the port was enabled.
     *
     */
    for (ln = 0; ln < num_lanes; ln++) {
      int eye_ht_1e06, qualifying_eye_ht_1e06, unused_var;
      int qualifying_eye_thrshld;

      (void)unused_var;

      // Determine the qualifying eye threshold, which depends on the FEC
      // mode.
      if ((port_p->sw.speed == BF_SPEED_10G ||
           port_p->sw.speed == BF_SPEED_25G) &&
          port_p->sw.fec == BF_FEC_TYP_NONE) {
        qualifying_eye_thrshld = BF_FSM_QUALIF_EYE_HT_THRESH_NO_FEC;
      } else {
        qualifying_eye_thrshld = BF_FSM_QUALIF_EYE_HT_THRESH_FEC;
      }

      // get (and adjust if necessary) qualifying eye values
      rc = bf_serdes_eye_quality_get(dev_id,
                                     dev_port,
                                     ln,
                                     &qualifying_eye_ht_1e06,
                                     &unused_var,
                                     &unused_var);
      if (rc != BF_SUCCESS) return BF_INVALID_ARG;

      rc = bf_serdes_quick_eye_get(dev_id,
                                   dev_port,
                                   ln,
                                   &eye_ht_1e06);  // really about 1e08

      if (rc != BF_SUCCESS) {
        port_mgr_log("FSM :%d:%3d:%d: port disabled?", dev_id, dev_port, ln);
        return BF_INVALID_ARG;  // restart FSM
      }

      port_mgr_log("FSM :%d:%3d:%d: 1e06=%3dmV/%3dmV",
                   dev_id,
                   dev_port,
                   ln,
                   eye_ht_1e06,
                   qualifying_eye_ht_1e06);

      // make sure the eye height "seems"valid
      if (eye_ht_1e06 >= 900) {
        port_mgr_log("FSM :%d:%3d:%d: Invalid eye height: %d",
                     dev_id,
                     dev_port,
                     ln,
                     eye_ht_1e06);
        return BF_INVALID_ARG;
      }

      // if no eye height requirements just continue
      if (qualifying_eye_ht_1e06 == 0) continue;

      if ((eye_ht_1e06 == 0) || (eye_ht_1e06 < qualifying_eye_ht_1e06)) {
        // adjust our expectations for this link
        int adjustment;

        // set flag to restart DFE on this lane
        dfe_retry_map |= (1 << ln);

        if (eye_ht_1e06 < qualifying_eye_ht_1e06) {
          int delta = qualifying_eye_ht_1e06 - eye_ht_1e06;

          adjustment = (delta / 2 + 1);
          if (adjustment < 10) {
            adjustment = 10;  // 10mV min to converge faster
          }
          if (qualifying_eye_ht_1e06 < adjustment) {
            adjustment = qualifying_eye_ht_1e06;  // got to 0
          }
          qualifying_eye_ht_1e06 -= adjustment;
          if (qualifying_eye_ht_1e06 < qualifying_eye_thrshld) {
            qualifying_eye_ht_1e06 = qualifying_eye_thrshld;
          }
        }
#if 1
        bf_serdes_eye_quality_set(
            dev_id, dev_port, ln, qualifying_eye_ht_1e06, 0, 0);
#endif
      }
    }

    if (dfe_retry_map) {
      for (ln = 0; ln < num_lanes; ln++) {
        // DRV-8367 restart dfe on all the lanes if one lane fails the check
        // one lane fail means the incoming signal on this port can be
        // unreliable
        /* if (dfe_retry_map & (1 << ln)) */ {
          // restart DFE on this lane.
          port_mgr_log("FSM :%d:%3d:%d: Re-Start ICAL", dev_id, dev_port, ln);
          rc = bf_serdes_start_dfe_ical(dev_id, dev_port, ln);
          if (rc != BF_SUCCESS) {
            port_mgr_log(
                "FSM :%d:%3d:%d: Error: %d: from bf_serdes_start_dfe_ical",
                dev_id,
                dev_port,
                ln,
                rc);
            return BF_INVALID_ARG;
          }
        }
      }
      return BF_NOT_READY;
    }

    /* Release MAC serdes sig_ok indication so MAC gets the real status */
    port_mgr_log(
        "FSM :%d:%3d:-: MACs serdesmux.sig_ok=PASS-THRU", dev_id, dev_port);
    bf_port_un_force_sig_ok_low_set(dev_id, dev_port);

    port_mgr_log(
        "FSM :%d:%3d:-: Enable MAC TX and RX for app logic", dev_id, dev_port);
    port_mgr_mac_set_rx_enable(dev_id, dev_port, true);
    if (port_p->sw.port_dir != BF_PORT_DIR_RX_ONLY) {
      /* Port direction is BF_PORT_DIR_DUPLEX (BF_PORT_DIR_TX_ONLY is
       * implemented by a different FSM.
       */
      port_mgr_mac_set_tx_enable(dev_id, dev_port, true);
    } else {
      /* port direction mode is RX-ONLY: keep TX-MAC disabled and enable
       * tx-drain-on-disable.
       */
      port_mgr_mac_set_tx_auto_drain_on_disable(dev_id, dev_port, true);
      port_mgr_mac_set_tx_enable(dev_id, dev_port, false);
    }

    for (ln = 0; ln < num_lanes; ln++) {
      /* call bf_port_serdes_los_get() to clear los flag, ignore its value */
      bf_port_serdes_los_get(dev_id, dev_port, &los_ok);
      bf_serdes_start_dfe_pcal(dev_id, dev_port, ln);
    }
    rc = port_mgr_map_dev_port_to_all(
        dev_id, dev_port, NULL, NULL, &mac_block, &ch, &is_cpu_port);
    if (rc) return rc;

    port_mgr_mac_int_fault_clr(dev_id, mac_block, ch, true);
    return BF_SUCCESS;
  }
  return BF_NOT_READY;
}

/** \brief Enable the MAC channel
 *
 * \param dev_id  : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param dev_port: encoded port identifier
 *
 * \return: BF_SUCCESS
 * \return: BF_INVALID_ARG: invalid dev_id
 * \return: BF_INVALID_ARG: invalid dev_port
 *
 */
bf_status_t bf_fsm_ena_mac(bf_dev_id_t dev_id, bf_dev_port_t dev_port) {
  bf_port_mac_ch_enable_set(dev_id, dev_port, true);
  return BF_SUCCESS;
}

/** \brief Abort port bring-up. Undo anything that might have been done
 *         before the error
 *
 * \param dev_id  : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param dev_port: encoded port identifier
 *
 * \return: BF_SUCCESS
 * \return: BF_INVALID_ARG: invalid dev_id
 * \return: BF_INVALID_ARG: invalid dev_port
 *
 */
bf_status_t bf_fsm_abort(bf_dev_id_t dev_id, bf_dev_port_t dev_port) {
  port_mgr_port_t *port_p = NULL;
  bool dfe_running = false;
  int num_lanes, ln;
  bf_status_t rc;
  if (!bf_fsm_port_is_valid(dev_id, dev_port)) return BF_INVALID_ARG;

  port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  if (port_p == NULL) return BF_INVALID_ARG;
  port_p->fsm_ext.link_fault_status = BF_PORT_LINK_FAULT_LOC_FAULT;
  port_p->fsm_ext.alt_link_fault_st = BF_PORT_LINK_FAULT_LOC_FAULT;

  num_lanes = bf_fsm_num_lanes_get(dev_id, dev_port);

  port_mgr_log("FSM :%d:%3d:-: MACs serdesmux.sig_ok=LOW", dev_id, dev_port);
  bf_port_force_sig_ok_low_set(dev_id, dev_port);

  // disable the MAC Rx side
  port_mgr_log("FSM :%d:%3d:-: Disable MAC Rx", dev_id, dev_port);
  port_mgr_mac_set_rx_enable(dev_id, dev_port, false);

  for (ln = 0; ln < num_lanes; ln++) {
    rc = bf_serdes_get_dfe_running(dev_id, dev_port, ln, &dfe_running);
    if (rc != BF_SUCCESS) {
      /* On error, log the error and assume dfe_running is true */
      port_mgr_log("FSM :%d:%3d:%d: Error: bf_serdes_get_dfe_running: rc=%d",
                   dev_id,
                   dev_port,
                   ln,
                   rc);
      dfe_running = true;
    }

    if (dfe_running) {
      /* stop iCal or pCal DFE, note that pCal can only be started if all
       * pending iCal were completed, so lstate.adative_dfe_enabled is a per
       * port flag: adaptive dfe is always enabled on all lanes simultaneously.
       */
      if (port_p->lstate.adative_dfe_enabled) {
        bf_serdes_stop_dfe_adaptive(dev_id, dev_port, ln);
      } else {
        bf_serdes_stop_dfe(dev_id, dev_port, ln);
      }
    }
  }

  port_p->lstate.adative_dfe_enabled = false;

  for (ln = 0; ln < num_lanes; ln++) {
    // turn off the serdes Rx
    bf_serdes_set_rx_tx_and_tx_output_en(dev_id,
                                         dev_port,
                                         ln,
                                         false,  // Rx OFF
                                         true,   // Tx ON
                                         true);  // Tx output_en ON
  }
  return BF_SUCCESS;
}

/** \brief Abort AN port bring-up. Bring undo anything that might have been done
 *         before the error. Disable AN and link-training
 *
 * \param dev_id  : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param dev_port: encoded port identifier
 *
 * \return: BF_SUCCESS
 * \return: BF_INVALID_ARG: invalid dev_id
 * \return: BF_INVALID_ARG: invalid dev_port
 *
 */
bf_status_t bf_fsm_an_abort(bf_dev_id_t dev_id, bf_dev_port_t dev_port) {
  port_mgr_port_t *port_p;
  int num_lanes, ln;
  if (!bf_fsm_port_is_valid(dev_id, dev_port)) return BF_INVALID_ARG;

  port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  if (port_p == NULL) return BF_INVALID_ARG;
  port_p->fsm_ext.link_fault_status = BF_PORT_LINK_FAULT_LOC_FAULT;
  port_p->fsm_ext.alt_link_fault_st = BF_PORT_LINK_FAULT_LOC_FAULT;

  // disable any running AN
  bf_serdes_autoneg_stop(dev_id, dev_port);

  num_lanes = bf_fsm_num_lanes_get(dev_id, dev_port);
  // disable any running LT
  for (ln = 0; ln < num_lanes; ln++) {
    // turn off the serdes
    bf_serdes_set_rx_tx_and_tx_output_en(
        dev_id, dev_port, ln, false, false, false);
    // turn off the serdes link-training on each lane
    bf_serdes_link_training_set(dev_id, dev_port, ln, false);
  }
  // swreset for clear stats
  bf_port_mac_ch_enable_set(dev_id, dev_port, false);
  port_mgr_log("%d:%3d: hw ctrs cleared.", dev_id, dev_port);
  return BF_SUCCESS;
}

/** \brief Abort port bring-up. Disable mac channel.
 *
 * \param dev_id  : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param dev_port: encoded port identifier
 *
 * \return: BF_SUCCESS
 * \return: BF_INVALID_ARG: invalid dev_id
 * \return: BF_INVALID_ARG: invalid dev_port
 *
 */
bf_status_t bf_fsm_default_abort(bf_dev_id_t dev_id, bf_dev_port_t dev_port) {
  // swreset for clear stats
  bf_port_mac_ch_enable_set(dev_id, dev_port, false);
  port_mgr_log("%d:%3d: hw ctrs cleared.", dev_id, dev_port);
  return BF_SUCCESS;
}

/*
 * Return true, if fec-alignment, pcs-status and erroredblockhi.
 *
 * Return false in all other cases.
 */
static bool bf_fsm_check_for_errored_blocks(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port) {
  bool pcs_status;
  uint32_t block_lock_per_pcs_lane;
  uint32_t alignment_marker_lock_per_pcs_lane;
  bool hi_ber;
  bool block_lock_all;
  bool alignment_marker_lock_all;

  bool hi_ser;
  bool fec_align_status;
  uint32_t fec_corr_cnt;
  uint32_t fec_uncorr_cnt;
  uint32_t fec_ser_lane_0;
  uint32_t fec_ser_lane_1;
  uint32_t fec_ser_lane_2;
  uint32_t fec_ser_lane_3;
  int mac_block;
  bf_status_t rc;
  bool fec_errors_found = false;

  port_mgr_map_dev_port_to_all(
      dev_id, dev_port, NULL, NULL, &mac_block, NULL, NULL);

  rc = bf_port_rs_fec_status_and_counters_get(dev_id,
                                              dev_port,
                                              &hi_ser,
                                              &fec_align_status,
                                              &fec_corr_cnt,
                                              &fec_uncorr_cnt,
                                              &fec_ser_lane_0,
                                              &fec_ser_lane_1,
                                              &fec_ser_lane_2,
                                              &fec_ser_lane_3,
                                              NULL,
                                              NULL,
                                              NULL,
                                              NULL);
  if (rc != BF_SUCCESS) return true;

  // If no fec-alignment, donot proceed
  if ((fec_align_status == 0) || (hi_ser) || (fec_uncorr_cnt > 0) ||
      (fec_corr_cnt > 0)) {
    port_mgr_log(
        "FSM :%d:%3d:-: FEC Errors: algn=%d : hi_ser=%d : uncorr=%d : corr=%d",
        dev_id,
        dev_port,
        fec_align_status,
        hi_ser,
        fec_uncorr_cnt,
        fec_corr_cnt);
    fec_errors_found = true;
    // fall thru to read and clear PCS errors
  }
  port_mgr_port_t *port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  if (port_p == NULL) return BF_INVALID_ARG;

  // check PCS status
  bf_port_pcs_status_get(dev_id,
                         dev_port,
                         &pcs_status,
                         &block_lock_per_pcs_lane,
                         &alignment_marker_lock_per_pcs_lane,
                         &hi_ber,
                         &block_lock_all,
                         &alignment_marker_lock_all);

  if (pcs_status) {
    // if PCS is up, check for bit errors
    uint32_t ber_cnt;
    uint32_t errored_blk_cnt;
    uint32_t sync_loss;
    uint32_t block_lock_loss;
    uint32_t hi_ber_cnt;
    uint32_t valid_error_cnt;
    uint32_t unknown_error_cnt;
    uint32_t invalid_error_cnt;
    uint32_t bip_errors_per_pcs_lane[20] = {0};

    bf_port_pcs_counters_get(dev_id,
                             dev_port,
                             &ber_cnt,
                             &errored_blk_cnt,
                             &sync_loss,
                             &block_lock_loss,
                             &hi_ber_cnt,
                             &valid_error_cnt,
                             &unknown_error_cnt,
                             &invalid_error_cnt,
                             bip_errors_per_pcs_lane);

    if (port_p->sw.speed == BF_SPEED_100G) {
      for (int ln = 0; ln < 20; ln++) {
        if (bip_errors_per_pcs_lane[ln]) {
          port_mgr_log(
              "FSM :%d:%3d:-: Bit Interleaved Parity errors PCS ln%d : %d",
              dev_id,
              dev_port,
              ln,
              bip_errors_per_pcs_lane[ln]);
        }
      }
    }

    /* Since this port already has some error correction (FEC) enabled
     * requires PCS error-free for the brief period where the link is
     * coming up.
     */
    if ((port_p->sw.single_lane_error_thresh != 0) && (errored_blk_cnt > 0)) {
      port_mgr_log(
          "FSM :%d:%3d:-: PCS Errored blocks detected  "
          "[%d >= %d] "
          "PCS sts=%x : blk-lk=%x : blk-lk-all=%d : hi-ber=%x : "
          "algn-lk-all=%d : algn-lk=%x",
          dev_id,
          dev_port,
          errored_blk_cnt,
          port_p->sw.single_lane_error_thresh,
          pcs_status,
          block_lock_per_pcs_lane,
          block_lock_all,
          hi_ber,
          alignment_marker_lock_all,
          alignment_marker_lock_per_pcs_lane);
      return true;
    } else {
      return false || fec_errors_found;
    }
  }

  (void)fec_corr_cnt;
  (void)fec_ser_lane_0;
  (void)fec_ser_lane_1;
  (void)fec_ser_lane_2;
  (void)fec_ser_lane_3;

  return fec_errors_found;
}

/** \brief Check if the oper state of the port and return success if the port
 *         goes down
 *
 * \param dev_id  : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param dev_port: encoded port identifier
 *
 * \return: BF_SUCCESS
 * \return: BF_INVALID_ARG: invalid dev_id
 * \return: BF_INVALID_ARG: invalid dev_port
 *
 * Initial Conditions:
 *
 * +-[ MAC ]---------------------------+
 * |   Tx En           : ENABLED       |
 * |   Rx En           : ENABLED       |
 * |   RS (LF/RF)      : Not Forced    |
 * |   PCS/FEC (Rx)    : Syncd         |
 * +-[ Serdes ]------------------------+
 * |   Tx En           : ENABLED       |
 * |   Tx PLL          : Locked        |
 * |   Tx Output En    : ENABLED       |
 * |   Tx Data         : CORE DATA     |
 * |   Rx En           : ENABLED       |
 * |   Rx PLL          : Locked        |
 * |   SigOkEnable     : ENABLED       |
 * |   SigOk           : OK            |
 * |   Rx Data         : Equalized     |
 * +-[ QSFP ]--------------------------+
 * |   resetL          : DE-ASSERTED   |
 * |   LPMode (hw)     : ASSERTED      |
 * |   LPMode (sw ovrd): ASSERT HiPwr  |
 * |   TX_DISABLE      : DE-ASSERTED   |
 * |   Tx CDR          : Locked        |
 * |   Rx (optical) LOS: DE-ASSERTED   |
 * +-----------------------------------+
 *
 * State Actions:
 *   MAC:
 *     Wait for glbl.livelilnkstate = 0
 *
 */
bf_status_t bf_fsm_wait_for_port_dwn_event(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port) {
  int mac_block, ch, is_cpu_port;
  port_mgr_port_t *port_p = NULL;
  port_mgr_dev_t *dev_p = NULL;
  bool remote_fault = false;
  bool local_fault = false;
  bf_status_t bf_status;
  bool pcs_rdy;
  int state = 1;

  if (!bf_fsm_port_is_valid(dev_id, dev_port)) return BF_INVALID_ARG;

  dev_p = port_mgr_map_dev_id_to_dev_p(dev_id);
  if (dev_p == NULL) return BF_INVALID_ARG;

  port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  if (port_p == NULL) return BF_INVALID_ARG;

  bf_status = port_mgr_map_dev_port_to_all(
      dev_id, dev_port, NULL, NULL, &mac_block, &ch, &is_cpu_port);
  if (bf_status) {
    return bf_status;
  }

  bf_status = bf_port_oper_state_get_extended(
      dev_id, dev_port, &state, &pcs_rdy, &local_fault, &remote_fault);

  if (bf_status) return bf_status;

  // If interrupts are enabled, override state using sw.oper_state, which is
  // set by the interrupt handler.
  if (dev_p->ldev.interrupt_based_link_monitoring) {
    state = port_p->sw.oper_state;
  }

  if (!pcs_rdy) {
    // Local Fault && PCS is not ready:
    //  transition to BF_FSM_ST_ABORT (original behavior)
    bf_status = BF_INVALID_ARG;
  } else if (local_fault) {
    // Local Fault event, but PCS is ready again:
    //  transition to BF_FSM_ST_WAIT_PCS_UP state
    bf_status = BF_ALT2_NEXT_ST;
  } else if (remote_fault || !state) {
    // Note: this condition covers the following situations:
    // 1. remote_fault=true;  state=1 : there was a short RF. Moving to
    //                                  REMOTE_FAUL state just for logging
    // 2. remote_fault=true;  state=0 : link currently in RF
    // 3. remote_fault=false; state=0 : link is assumed in RF because PCS is
    //                                  still ready (locked).
    bf_status = BF_ALT3_NEXT_ST;
  } else {
    // link remains UP: just return BF_NOT_READY
    bf_status = BF_NOT_READY;
  }

  if (bf_status != BF_NOT_READY) {
    // Link went from UP to DOWN
    port_mgr_mac_int_fault_clr(dev_id, mac_block, ch, false);
    port_mgr_log(
        "FSM :%d:%3d:-: (WDEV) state=%d PCS_status=%x local_fault=%d remote "
        "fault =%d",
        dev_id,
        dev_port,
        state,
        pcs_rdy,
        local_fault,
        remote_fault);

    if (!dev_p->ldev.interrupt_based_link_monitoring) {
      // Update operational link state and set flag to call callbacks.
      // Note that here state is forced to 0 (link down)
      // Do this only if interrupt based link monitoring is disabled (when link
      // interrupts are enabled, the operational state update and callback calls
      // are done by the interrupt handler)
      bf_port_oper_state_update(dev_id, dev_port, 0);
    }
  }

  // update link fault status
  switch (bf_status) {
    case BF_NOT_READY:
      port_p->fsm_ext.link_fault_status = BF_PORT_LINK_FAULT_OK;
      break;
    case BF_ALT3_NEXT_ST:
      port_p->fsm_ext.link_fault_status = BF_PORT_LINK_FAULT_REM_FAULT;
      break;
    default:
      port_p->fsm_ext.link_fault_status = BF_PORT_LINK_FAULT_LOC_FAULT;
  }

  return bf_status;
}

/** \brief Check if the oper state of the port and return success if the port
 *         goes down. AN state machine.
 *
 * \param dev_id  : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param dev_port: encoded port identifier
 *
 * \return: BF_SUCCESS
 * \return: BF_INVALID_ARG: invalid dev_id
 * \return: BF_INVALID_ARG: invalid dev_port
 *
 * Initial Conditions:
 *
 * +-[ MAC ]---------------------------+
 * |   Tx En           : ENABLED       |
 * |   Rx En           : ENABLED       |
 * |   RS (LF/RF)      : Not Forced    |
 * |   PCS/FEC (Rx)    : Syncd         |
 * +-[ Serdes ]------------------------+
 * |   Tx En           : ENABLED       |
 * |   Tx PLL          : Locked        |
 * |   Tx Output En    : ENABLED       |
 * |   Tx Data         : CORE DATA     |
 * |   Rx En           : ENABLED       |
 * |   Rx PLL          : Locked        |
 * |   SigOkEnable     : ENABLED       |
 * |   SigOk           : OK            |
 * |   Rx Data         : Equalized     |
 * +-[ QSFP ]--------------------------+
 * |   resetL          : DE-ASSERTED   |
 * |   LPMode (hw)     : ASSERTED      |
 * |   LPMode (sw ovrd): ASSERT HiPwr  |
 * |   TX_DISABLE      : DE-ASSERTED   |
 * |   Tx CDR          : Locked        |
 * |   Rx (optical) LOS: DE-ASSERTED   |
 * +-----------------------------------+
 *
 * State Actions:
 *   MAC:
 *     Wait for glbl.livelilnkstate = 0
 *
 */
bf_status_t bf_fsm_an_wait_for_port_dwn_event(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port) {
  bf_status_t bf_status;
  int state;
  port_mgr_dev_t *dev_p = NULL;
  port_mgr_port_t *port_p = NULL;

  if (!bf_fsm_port_is_valid(dev_id, dev_port)) return BF_INVALID_ARG;

  dev_p = port_mgr_map_dev_id_to_dev_p(dev_id);
  if (dev_p == NULL) return BF_INVALID_ARG;

  port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  if (port_p == NULL) return BF_INVALID_ARG;

  // If interrupt is enabled, link-down will be marked by intr handler -
  // Until then return current (UP) state.
  if (dev_p->ldev.interrupt_based_link_monitoring) {
    state = port_p->sw.oper_state;
  } else {
    bf_status =
        bf_port_oper_state_get_skip_intr_check(dev_id, dev_port, &state);
    if (bf_status != BF_SUCCESS) {
      return bf_status;
    }
  }

  if (!state) {
    // Indicates that the port went from up to down.
    // shadow mac stats
    update_historical_mac_stats(dev_id, dev_port);

    // swreset for clear stats
    bf_port_mac_ch_enable_set(dev_id, dev_port, false);
    bf_port_mac_ch_enable_set(dev_id, dev_port, true);

    port_p->fsm_ext.link_fault_status = BF_PORT_LINK_FAULT_LOC_FAULT;
    bf_port_an_lt_stats_init(dev_id, dev_port, true);
    return BF_SUCCESS;
  }

  port_p->fsm_ext.link_fault_status = BF_PORT_LINK_FAULT_OK;

  return BF_NOT_READY;
}

/** \brief Wait for PCS status = UP
 *
 * \param dev_id  : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param dev_port: encoded port identifier
 *
 * \return: BF_SUCCESS
 * \return: BF_INVALID_ARG: invalid dev_id
 * \return: BF_INVALID_ARG: invalid dev_port
 *
 * Initial Conditions:
 *
 * +-[ MAC ]---------------------------+
 * |   Tx En           : ENABLED       |
 * |   Rx En           : ENABLED       |
 * |   RS (LF/RF)      : Not Forced    |
 * |   PCS/FEC (Rx)    : ?             |
 * +-[ Serdes ]------------------------+
 * |   Tx En           : ENABLED       |
 * |   Tx PLL          : Locked        |
 * |   Tx Output En    : ENABLED       |
 * |   Tx Data         : CORE DATA     |
 * |   Rx En           : ENABLED       |
 * |   Rx PLL          : Locked        |
 * |   SigOkEnable     : ENABLED       |
 * |   SigOk           : OK            |
 * |   Rx Data         : Equalized     |
 * +-[ QSFP ]--------------------------+
 * |   resetL          : DE-ASSERTED   |
 * |   LPMode (hw)     : ASSERTED      |
 * |   LPMode (sw ovrd): ASSERT HiPwr  |
 * |   TX_DISABLE      : DE-ASSERTED   |
 * |   Tx CDR          : Locked        |
 * |   Rx (optical) LOS: DE-ASSERTED   |
 * +-----------------------------------+
 *
 * State Actions:
 *   MAC:
 *     if RS FEC
 *       if RS.aligned
 *         LF = False (duplicate?)
 *         if glbl.livelinkstate = 0, reset Rx path
 *     if 1G
 *       LF = False (duplicate?)
 *     Get and log PCS counters/errors
 *     LF = False (duplciate?)
 *
 *   Serdes Initialization:
 *
 */
bf_status_t bf_fsm_wait_pcs_up(bf_dev_id_t dev_id, bf_dev_port_t dev_port) {
  int mac_block, ch, is_cpu_port;
  port_mgr_port_t *port_p;
  bf_status_t bf_status;
  bool remote_fault;
  bool local_fault;
  bool pcs_rdy;
  int state = 0;

  if (!bf_fsm_port_is_valid(dev_id, dev_port)) {
    return BF_INVALID_ARG;
  }

  port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  if (!port_p) {
    return BF_INVALID_ARG;
  }

  bf_status = port_mgr_map_dev_port_to_all(
      dev_id, dev_port, NULL, NULL, &mac_block, &ch, &is_cpu_port);
  if (bf_status) {
    return bf_status;
  }

  // special-case 1G ports, which don't have the BASE-R PCS
  if (port_p->sw.speed == BF_SPEED_1G) {
    bf_status =
        bf_port_oper_state_get_skip_intr_check(dev_id, dev_port, &state);

    if (bf_status == BF_SUCCESS && state) {
      return BF_SUCCESS;
    }

    return BF_NOT_READY;
  }

  // Special case for RS-FEC
  // Note sure if this is still needed: PCS will remain unlocked if this
  //  condition is not satisfied.
  if (port_p->sw.fec == BF_FEC_TYP_REED_SOLOMON) {
    bf_status = bf_fsm_rs_up(dev_id, dev_port);
    /* Check LOS if FEC is not locked */
    if (bf_status != BF_SUCCESS) {
      int los_ok = 0;
      bf_port_serdes_los_get(dev_id, dev_port, &los_ok);
      if (!los_ok) {
        port_mgr_log("FSM :%d:%3d: detected LOS", dev_id, dev_port);
        return BF_INVALID_ARG;
      }
      return bf_status;
    }
  }

  // check extended link state
  bf_status = bf_port_oper_state_get_extended(
      dev_id, dev_port, &state, &pcs_rdy, &local_fault, &remote_fault);
  if (bf_status) {
    return bf_status;
  }

  port_mgr_log(
      "FSM :%d:%3d:-: state=%d PCS_status=%x local_fault=%d remote fault =%d",
      dev_id,
      dev_port,
      state,
      pcs_rdy,
      local_fault,
      remote_fault);

  if (!pcs_rdy || local_fault) {
    // PCS not ready or local_fault condition:
    //  clear interrupts and keep debounce counter cleared.
    port_mgr_mac_int_fault_clr(dev_id, mac_block, ch, true);
    port_p->fsm_ext.debounce_cnt = 0;
    return BF_NOT_READY;
  }

  // PCS is locked and aligned:
  // Perform some additional PCS/serdes validations
  bf_status = bf_fsm_pcs_error_validation(dev_id, dev_port);
  if (bf_status != BF_SUCCESS) {
    port_mgr_mac_int_fault_clr(dev_id, mac_block, ch, true);
    port_p->fsm_ext.debounce_cnt = 0;
    return bf_status;
  }

  // Start adaptive DFE.
  // Note that adative_dfe_enabled flag is set by bf_fsm_start_adaptive_tuning()
  if (!port_p->lstate.adative_dfe_enabled) {
    bf_fsm_start_adaptive_tuning(dev_id, dev_port);
  }

  // PCS and RS-FEC are OK:
  if (remote_fault) {
    if (++(port_p->fsm_ext.debounce_cnt) >= BF_FSM_PCS_LOCK_THRSHLD) {
      // clear interrupts and move to REMOTE_FAULT state.
      port_mgr_mac_int_fault_clr(dev_id, mac_block, ch, false);
      port_p->fsm_ext.link_fault_status = BF_PORT_LINK_FAULT_REM_FAULT;
      return BF_ALT2_NEXT_ST;
    }
  } else if (state) {
    if (++(port_p->fsm_ext.debounce_cnt) >= port_p->fsm_ext.debounce_thr) {
      // move to WAIT_DWN_EVENT  state (this is the default
      // next-state, so just returning BF_SUCCESS is enough)

      // Notify the link is going UP
      port_mgr_mac_int_fault_clr(dev_id, mac_block, ch, true);
      bf_port_oper_state_update(dev_id, dev_port, state);
      port_p->fsm_ext.link_fault_status = BF_PORT_LINK_FAULT_OK;
      port_p->fsm_ext.alt_link_fault_st = BF_PORT_LINK_FAULT_OK;
      port_p->fsm_ext.port_was_up = true;
      return BF_SUCCESS;
    }
  } else {
    // PCS is ready, but link is not UP: and hi_ber and, if OK, assume
    // REMOTE_FAULT condition
    if (!port_p->lstate.hi_ber) {
      if (++(port_p->fsm_ext.debounce_cnt) >= BF_FSM_PCS_LOCK_THRSHLD) {
        // Move to REMOTE_FAULT state.
        port_p->fsm_ext.link_fault_status = BF_PORT_LINK_FAULT_REM_FAULT;
        return BF_ALT2_NEXT_ST;
      }
    } else {
      // no block lock or hi_ber condition.
      port_p->fsm_ext.debounce_cnt = 0;
    }
  }

  return BF_NOT_READY;
}

/** \brief Remote Fault state
 *
 * \param dev_id  : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param dev_port: encoded port identifier
 *
 * \return: BF_SUCCESS
 * \return: BF_INVALID_ARG: invalid dev_id
 * \return: BF_INVALID_ARG: invalid dev_port
 *
 * Initial Conditions:
 *
 * +-[ MAC ]---------------------------+
 * |   Tx En           : ENABLED       |
 * |   Rx En           : ENABLED       |
 * |   RS (LF/RF)      : Not Forced    |
 * |   PCS/FEC (Rx)    : OK            |
 * +-[ Serdes ]------------------------+
 * |   Tx En           : ENABLED       |
 * | Tx PLL          : Locked          |
 * |   Tx Output En    : ENABLED       |
 * |   Tx Data : CORE DATA             |
 * |   Rx En           : ENABLED       |
 * | Rx PLL          : Locked          |
 * |   SigOkEnable     : ENABLED       |
 * |   SigOk : OK                      |
 * | Rx Data         : Equalized       |
 * +-[ QSFP ]--------------------------+
 * |   resetL : DE-ASSERTED            |
 * | LPMode (hw)     : ASSERTED        |
 * | LPMode (sw ovrd): ASSERT HiPwr    |
 * |   TX_DISABLE      : DE-ASSERTED   |
 * |   Tx CDR : Locked                 |
 * |   Rx (optical) LOS: DE-ASSERTED   |
 * +-----------------------------------+
 *
 * State Actions:
 *   MAC:
 *     if LF or !pcs_ready
 *       back to BF_FSM_ST_WAIT_PCS_LOCK state
 *     else if RF
 *       clear interrupts, restore debounce coutner
 *     else if link-up
 *       clear interrupts, move to BF_FSM_ST_WAIT_PCS_UP state
 *     else
 *       clear debounce counter
 *
 *   Serdes Initialization:
 *     ---
 */
bf_status_t bf_fsm_remote_fault(bf_dev_id_t dev_id, bf_dev_port_t dev_port) {
  int mac_block, ch, is_cpu_port;
  bf_status_t bf_status;
  bool remote_fault;
  bool local_fault;
  bool pcs_rdy;
  int state = 0;

  port_mgr_port_t *port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  if (!port_p) {
    return BF_INVALID_ARG;
  }

  bf_status = port_mgr_map_dev_port_to_all(
      dev_id, dev_port, NULL, NULL, &mac_block, &ch, &is_cpu_port);
  if (bf_status) {
    return bf_status;
  }

  // special-case 1G ports, which don't have the BASE-R PCS
  if (port_p->sw.speed == BF_SPEED_1G) {
    bf_status =
        bf_port_oper_state_get_skip_intr_check(dev_id, dev_port, &state);

    if (bf_status == BF_SUCCESS && state) return BF_SUCCESS;

    return BF_NOT_READY;
  }

  // check extended link state
  bf_status = bf_port_oper_state_get_extended(
      dev_id, dev_port, &state, &pcs_rdy, &local_fault, &remote_fault);
  if (bf_status) {
    return bf_status;
  }

  if (!pcs_rdy) {
    // PCS is not ready: move to BF_FSM_ST_ABORT (original behavior)
    bf_status = BF_INVALID_ARG;
  } else if (local_fault) {
    // Local Fault event, but PCS is ready again, move to
    // BF_FSM_ST_WAIT_PCS_UP state
    port_p->fsm_ext.link_fault_status = BF_PORT_LINK_FAULT_LOC_FAULT;
    bf_status = BF_ALT3_NEXT_ST;
  } else if (remote_fault) {
    // Remote Fault event: just clear interrupts and remain in current state
    port_p->fsm_ext.debounce_cnt = 0;
    port_p->fsm_ext.link_fault_status = BF_PORT_LINK_FAULT_REM_FAULT;
    bf_status = BF_NOT_READY;
  } else if (state) {
    // Link is UP: inc debounce counter and move to BF_FSM_ST_WAIT_DWN_EVNT
    // when it hits the threshold.
    if (++(port_p->fsm_ext.debounce_cnt) >= port_p->fsm_ext.debounce_thr) {
      // Update link oper_state, call callbacks to indicate the link is UP and
      // return BF_SUCCESS to transition to next_state.
      bf_status = bf_port_oper_state_update(dev_id, dev_port, state);
      if (bf_status == BF_SUCCESS) {
        port_p->fsm_ext.link_fault_status = BF_PORT_LINK_FAULT_OK;
        port_p->fsm_ext.alt_link_fault_st = BF_PORT_LINK_FAULT_OK;
        port_p->fsm_ext.port_was_up = true;
      }
    } else {
      bf_status = BF_NOT_READY;
    }

  } else {
    port_p->fsm_ext.debounce_cnt = 0;
    bf_status = BF_NOT_READY;
  }

  if (bf_status == BF_SUCCESS) {
    // Clear HW interrupt status when link going from Down -> UP
    port_mgr_mac_int_fault_clr(dev_id, mac_block, ch, true);
  } else {
    port_mgr_mac_int_fault_clr(dev_id, mac_block, ch, false);
  }

  if (bf_status != BF_NOT_READY) {
    port_mgr_log(
        "FSM :%d:%3d:-: state=%d PCS_status=%x local_fault=%d remote fault =%d",
        dev_id,
        dev_port,
        state,
        pcs_rdy,
        local_fault,
        remote_fault);
  }
  return bf_status;
}

/** \brief Check PCS status for AN ports
 *
 * \param dev_id  : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param dev_port: encoded port identifier
 *
 * \return: BF_SUCCESS
 * \return: BF_INVALID_ARG: invalid dev_id
 * \return: BF_INVALID_ARG: invalid dev_port
 *
 */
bf_status_t bf_fsm_an_wait_pcs_up(bf_dev_id_t dev_id, bf_dev_port_t dev_port) {
  int mac_block, ch, is_cpu_port;
  bool pcs_status;
  uint32_t block_lock_per_pcs_lane;
  uint32_t alignment_marker_lock_per_pcs_lane;
  bool hi_ber;
  bool block_lock_all;
  bool alignment_marker_lock_all;
  bf_status_t bf_status;
  int state;
  port_mgr_port_t *port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);

  port_mgr_map_dev_port_to_all(
      dev_id, dev_port, NULL, NULL, &mac_block, &ch, &is_cpu_port);

  if (port_p == NULL) return BF_INVALID_ARG;

  // special-case for RS-FEC, which replaces the PCS statuses
  if (port_p->sw.fec == BF_FEC_TYP_REED_SOLOMON) {
    bf_status = bf_fsm_rs_up(dev_id, dev_port);
    if (bf_status == BF_SUCCESS) {
      bf_status =
          bf_port_oper_state_get_no_side_effect(dev_id, dev_port, &state);
      if (((bf_status == BF_SUCCESS) && (state == 0))) {
        return BF_NOT_READY;
      }
      // Link is up, but seeing errored blocks, which means uncorrectable FEC
      // errors. Toggle serdes sig_ok indication to Rx path to reset FEC
      // note: May want to add 100G case here too
      if ((port_p->sw.speed == BF_SPEED_25G ||
           port_p->sw.speed == BF_SPEED_50G) &&
          bf_fsm_check_for_errored_blocks(dev_id, dev_port)) {
        // It may take sometime to see errorblocked count 0.
        // Declare port not ready until then.
        if (!port_p->fec_reset_inp) {
          port_p->fec_reset_inp = true;
          port_mgr_log(
              "FSM :%d:%3d: erroredblockhi found. Reset the FEC in AN fsm",
              dev_id,
              dev_port);
          bf_fsm_reset_rx_path(dev_id, dev_port, true);
          bf_port_oper_state_get_skip_intr_check(dev_id, dev_port, &state);
          state = 0;
        }
        return BF_NOT_READY;
      } else {
        if ((port_p->sw.speed == BF_SPEED_25G) && (port_p->fec_reset_inp)) {
          port_mgr_log("FSM :%d:%3d: erroredblockhi cleared in AN FSM.",
                       dev_id,
                       dev_port);
          port_p->fec_reset_inp = false;
        }
        // fire for effect (issue callbacks if exiting FSM)
        bf_status =
            bf_port_oper_state_get_skip_intr_check(dev_id, dev_port, &state);
        if (bf_status != BF_SUCCESS) {
          return BF_INVALID_ARG;
        }
        if (state == 0) {
          return BF_NOT_READY;
        }
        bf_fsm_start_adaptive_tuning(dev_id, dev_port);
        port_p->fsm_ext.link_fault_status = BF_PORT_LINK_FAULT_OK;
        port_p->fsm_ext.alt_link_fault_st = BF_PORT_LINK_FAULT_OK;
        port_mgr_mac_int_fault_clr(dev_id, mac_block, ch, true);
        return BF_SUCCESS;
      }
    }
    return BF_NOT_READY;
  }

  // check PCS status
  bf_port_pcs_status_get(dev_id,
                         dev_port,
                         &pcs_status,
                         &block_lock_per_pcs_lane,
                         &alignment_marker_lock_per_pcs_lane,
                         &hi_ber,
                         &block_lock_all,
                         &alignment_marker_lock_all);
  port_mgr_log(
      "FSM :%d:%3d:-: PCS sts=%x : blk-lk=%x : blk-lk-all=%d : hi-ber=%x : "
      "algn-lk-all=%d : algn-lk=%x",
      dev_id,
      dev_port,
      pcs_status,
      block_lock_per_pcs_lane,
      block_lock_all,
      hi_ber,
      alignment_marker_lock_all,
      alignment_marker_lock_per_pcs_lane);

  if (pcs_status && block_lock_all) {
    // if PCS is up, check for bit errors
    if (hi_ber == 0) {
      uint32_t ber_cnt;
      uint32_t errored_blk_cnt;
      uint32_t sync_loss;
      uint32_t block_lock_loss;
      uint32_t hi_ber_cnt;
      uint32_t valid_error_cnt;
      uint32_t unknown_error_cnt;
      uint32_t invalid_error_cnt;
      uint32_t bip_errors_per_pcs_lane[20];
      uint32_t bip_err_summary = 0;
      uint32_t ln;

      bf_port_pcs_counters_get(dev_id,
                               dev_port,
                               &ber_cnt,
                               &errored_blk_cnt,
                               &sync_loss,
                               &block_lock_loss,
                               &hi_ber_cnt,
                               &valid_error_cnt,
                               &unknown_error_cnt,
                               &invalid_error_cnt,
                               bip_errors_per_pcs_lane);

      if (port_p->sw.speed == BF_SPEED_100G) {
        for (ln = 0; ln < 20; ln++) {
          bip_err_summary += bip_errors_per_pcs_lane[ln];
        }
      }
      bf_status =
          bf_port_oper_state_get_no_side_effect(dev_id, dev_port, &state);

      port_mgr_log(
          "FSM :%d:%3d:-: Op=%s : PCS err-blk=%d : syn-lst=%d : bk-lk-lst=%d : "
          "hi_ber=%d : val-err=%d : unk-err=%d : inv-err=%d : bip=%d",
          dev_id,
          dev_port,
          state == 0 ? "Dn" : "Up",
          errored_blk_cnt,
          sync_loss,
          block_lock_loss,
          hi_ber_cnt,
          valid_error_cnt,
          unknown_error_cnt,
          invalid_error_cnt,
          bip_err_summary);

      /* extra check for single-lane ports that are prone to erroneous
       * PCS status when receiving random data
       */
      if ((port_p->sw.speed == BF_SPEED_25G) ||
          (port_p->sw.speed == BF_SPEED_10G)) {
        if (port_p->sw.fec == BF_FEC_TYP_NONE) {
          if (port_p->sw.single_lane_error_thresh != 0) {
            if (errored_blk_cnt >= port_p->sw.single_lane_error_thresh) {
              if (state != 0) {
                port_mgr_log(
                    "FSM :%d:%3d:-: [hold-dn] Errored block thresh exceeded "
                    "[%d >= %d]",
                    dev_id,
                    dev_port,
                    errored_blk_cnt,
                    port_p->sw.single_lane_error_thresh);
                state = 0;
              }
            }
          }
        }
      }

      if ((bf_status == BF_SUCCESS) && (state == 0)) {
        return BF_NOT_READY;
      }
      // fire for effect (issue callbacks if exiting FSM)
      bf_status =
          bf_port_oper_state_get_skip_intr_check(dev_id, dev_port, &state);
      if (bf_status != BF_SUCCESS) {
        return BF_INVALID_ARG;
      }
      if (state == 0) {
        return BF_NOT_READY;
      }

      bf_fsm_start_adaptive_tuning(dev_id, dev_port);
      port_p->fsm_ext.link_fault_status = BF_PORT_LINK_FAULT_OK;
      port_p->fsm_ext.alt_link_fault_st = BF_PORT_LINK_FAULT_OK;
      port_p->fsm_ext.port_was_up = true;
      port_mgr_mac_int_fault_clr(dev_id, mac_block, ch, true);
      return BF_SUCCESS;
    }
  }
  return BF_INVALID_ARG;
  // return BF_NOT_READY;
}

/** \brief Poll for DFE PCAL done
 *
 * \param dev_id  : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param dev_port: encoded port identifier
 *
 * \return: BF_SUCCESS
 * \return: BF_INVALID_ARG: invalid dev_id
 * \return: BF_INVALID_ARG: invalid dev_port
 *
 */
bf_status_t bf_fsm_an_wait_pcal_done(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port) {
  int ln, dfe_done_map = 0;
  bool dfe_running = false;
  bf_status_t rc;
  int num_lanes = bf_fsm_num_lanes_get(dev_id, dev_port);
  int lane_map = bf_fsm_serdes_lane_map_get(dev_id, dev_port);
  port_mgr_port_t *port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  if (port_p == NULL) return BF_INVALID_ARG;

  if (!bf_fsm_port_is_valid(dev_id, dev_port)) return BF_INVALID_ARG;

  for (ln = 0; ln < num_lanes; ln++) {
    rc = bf_serdes_get_dfe_running(dev_id, dev_port, ln, &dfe_running);
    if (rc != BF_SUCCESS) {
      port_mgr_log("FSM :%d:%3d:%d: Error: bf_serdes_get_dfe_running: rc=%d",
                   dev_id,
                   dev_port,
                   ln,
                   rc);
      return rc;
    }
    if (!dfe_running) {
      dfe_done_map |= (1 << ln);
    }
  }
  if (dfe_done_map == lane_map) {
    if ((port_p->sw.speed == BF_SPEED_10G) ||
        (port_p->sw.speed == BF_SPEED_40G) ||
        (port_p->sw.speed == BF_SPEED_1G)) {
    } else {
      // PI CAL results in significant bit errors for a few
      // seconds, requiring considerable special-handling at
      // the MAC/PCS level. Leave disabled for now.
      //
      // Only run phase-interpolator cal for 25G serdes speeds
      // port_mgr_log("FSM :%d:%3d:-: Run pi-cal..", dev_id, dev_port);
      // for (ln = 0; ln < num_lanes; ln++) {
      //  bf_serdes_start_dfe_pi_cal(dev_id, dev_port, ln);
      //}
    }
    return BF_SUCCESS;
  }
  return BF_NOT_READY;
}

bf_status_t bf_fsm_rs_up(bf_dev_id_t dev_id, bf_dev_port_t dev_port) {
  bool hi_ser;
  bool fec_align_status;
  uint32_t fec_corr_cnt = 0;
  uint32_t fec_uncorr_cnt = 0;
  uint32_t fec_ser_lane_0 = 0;
  uint32_t fec_ser_lane_1 = 0;
  uint32_t fec_ser_lane_2 = 0;
  uint32_t fec_ser_lane_3 = 0;
  int mac_block;
  bf_status_t rc;

  port_mgr_map_dev_port_to_all(
      dev_id, dev_port, NULL, NULL, &mac_block, NULL, NULL);

  rc = bf_port_rs_fec_status_and_counters_get(dev_id,
                                              dev_port,
                                              &hi_ser,
                                              &fec_align_status,
                                              &fec_corr_cnt,
                                              &fec_uncorr_cnt,
                                              &fec_ser_lane_0,
                                              &fec_ser_lane_1,
                                              &fec_ser_lane_2,
                                              &fec_ser_lane_3,
                                              NULL,
                                              NULL,
                                              NULL,
                                              NULL);

  if (rc != BF_SUCCESS) return BF_INVALID_ARG;

  port_mgr_log(
      "FSM :%d:%3d:-: RS FEC align=%x : hi_ser=%d : uncorr=%d : corr=%d",
      dev_id,
      dev_port,
      fec_align_status,
      hi_ser,
      fec_uncorr_cnt,
      fec_corr_cnt);

  if (fec_align_status && !hi_ser) return BF_SUCCESS;

  bf_fsm_reset_rx_path(dev_id, dev_port, true);

  return BF_NOT_READY;
}

void bf_fsm_reset_rx_path(bf_dev_id_t dev_id,
                          bf_dev_port_t dev_port,
                          bool wait_for_unreset) {
  // toggle force-signalOK to reset rx path
  bf_port_force_sig_ok_low_set(dev_id, dev_port);
  bf_sys_usleep(1);  // no delay seems to be too fast

  if (wait_for_unreset) {
    bf_sys_usleep(5000);  // close to 5ms, worst case alignment time
  }

  bf_port_un_force_sig_ok_low_set(dev_id, dev_port);
  port_mgr_log("FSM :%d:%3d:-: RS-FEC Rx reset", dev_id, dev_port);
}

/** \brief Assert RS-FEC Reset if the port is configured with RS-FEC
 *
 * \param dev_id  : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param dev_port: encoded port identifier
 *
 * \return: BF_SUCCESS
 * \return: BF_INVALID_ARG: invalid dev_id
 * \return: BF_INVALID_ARG: invalid dev_port
 *
 */
bf_status_t bf_fsm_assert_rs_fec(bf_dev_id_t dev_id, bf_dev_port_t dev_port) {
  port_mgr_port_t *port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  if (!port_p) return BF_INVALID_ARG;

  if (!bf_fsm_port_is_valid(dev_id, dev_port)) return BF_INVALID_ARG;

  if (port_p->sw.fec == BF_FEC_TYP_REED_SOLOMON) {
    bool is_sw_model = false;

    // API requires data from a MAC register which is not
    // implemented on the model. just skip it ..
    bf_drv_device_type_get(dev_id, &is_sw_model);
    if (is_sw_model) {
      return BF_SUCCESS;
    }
    // bf_port_rs_fec_reset_set(dev_id, dev_port, true);
    bf_port_force_sig_ok_low_set(dev_id, dev_port);
    port_mgr_log(
        "FSM :%d:%3d:-: RS-FEC SIG_LO asserted to reset Rx", dev_id, dev_port);
  }

  return BF_SUCCESS;
}

/** \brief De-Assert RS-FEC Reset if the port is configured with RS-FEC
 *
 * \param dev_id  : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param dev_port: encoded port identifier
 *
 * \return: BF_SUCCESS
 * \return: BF_INVALID_ARG: invalid dev_id
 * \return: BF_INVALID_ARG: invalid dev_port
 *
 */
bf_status_t bf_fsm_deassert_rs_fec(bf_dev_id_t dev_id, bf_dev_port_t dev_port) {
  bool is_sw_model = false;
  port_mgr_port_t *port_p;
  bf_status_t bf_status;
  int is_cpu_port;
  int mac_block;
  int ch;

  port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  if (!port_p) return BF_INVALID_ARG;

  // API requires data from a MAC register which is not
  // implemented on the model. just skip it ..
  bf_drv_device_type_get(dev_id, &is_sw_model);
  if (is_sw_model) {
    bf_port_oper_state_set_pending_callbacks(dev_id, dev_port, 1);
    return BF_SUCCESS;
  }

  bf_status = port_mgr_map_dev_port_to_all(
      dev_id, dev_port, NULL, NULL, &mac_block, &ch, &is_cpu_port);
  if (bf_status) return bf_status;

  bf_port_un_force_sig_ok_low_set(dev_id, dev_port);
  port_mgr_log("FSM :%d:%3d:-: RS-FEC SIG_LO  de-asserted to un-reset Rx",
               dev_id,
               dev_port);
  port_mgr_mac_int_fault_clr(dev_id, mac_block, ch, true);

  return BF_SUCCESS;
}

/** \brief Wait for UP state for a port with a PCS loopback
 *
 * \param dev_id  : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param dev_port: encoded port identifier
 *
 * \return: BF_SUCCESS
 * \return: BF_INVALID_ARG: invalid dev_id
 * \return: BF_INVALID_ARG: invalid dev_port
 *
 */
bf_status_t bf_fsm_wait_lpbk_port_up(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port) {
  bool remote_fault = false;
  bool local_fault = false;
  bool is_sw_model = false;
  port_mgr_port_t *port_p;
  bf_status_t bf_status;
  int is_cpu_port;
  int mac_block;
  bool pcs_rdy;
  int state = 0;
  int ch;

  port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  if (!port_p) return BF_INVALID_ARG;

  // API requires data from a MAC register which is not
  // implemented on the model. just skip it ..
  bf_drv_device_type_get(dev_id, &is_sw_model);
  if (is_sw_model) {
    bf_port_oper_state_set_pending_callbacks(dev_id, dev_port, 1);
    return BF_SUCCESS;
  }

  bf_status = port_mgr_map_dev_port_to_all(
      dev_id, dev_port, NULL, NULL, &mac_block, &ch, &is_cpu_port);
  if (bf_status) return bf_status;

  if (port_p->sw.fec == BF_FEC_TYP_REED_SOLOMON &&
      port_p->sw.lpbk_mode != BF_LPBK_MAC_NEAR) {
    bf_status = bf_fsm_rs_up(dev_id, dev_port);
    if (bf_status != BF_SUCCESS) return bf_status;
  }

  // check extended link state
  bf_status = bf_port_oper_state_get_extended(
      dev_id, dev_port, &state, &pcs_rdy, &local_fault, &remote_fault);
  if (bf_status) return bf_status;

  if (!state || !pcs_rdy || local_fault || remote_fault) {
    port_mgr_log("FSM :%d:%3d:-: state=%d, pcs_readyt=%d, lfault=%d, rfault=%d",
                 dev_id,
                 dev_port,
                 state,
                 pcs_rdy,
                 local_fault,
                 remote_fault);
    port_mgr_mac_int_fault_clr(dev_id, mac_block, ch, true);
    return BF_NOT_READY;
  }

  // set the port "Up" here if in mac or pcs loopback
  if ((port_p->sw.lpbk_mode == BF_LPBK_MAC_NEAR) ||
      (port_p->sw.lpbk_mode == BF_LPBK_PCS_NEAR) ||
      (port_p->sw.lpbk_mode == BF_LPBK_SERDES_NEAR)) {
    bf_port_oper_state_set_pending_callbacks(dev_id, dev_port, 1);
  }

  return BF_SUCCESS;
}

/** \brief Enable the MAC Tx and Rx
 *
 * \param dev_id  : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param dev_port: encoded port identifier
 *
 * \return: BF_SUCCESS
 * \return: BF_INVALID_ARG: invalid dev_id
 * \return: BF_INVALID_ARG: invalid dev_port
 *
 */
bf_status_t bf_fsm_enable_mac_tx_rx(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port) {
  port_mgr_port_t *port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  if (!port_p) return BF_INVALID_ARG;

  if (!bf_fsm_port_is_valid(dev_id, dev_port)) return BF_INVALID_ARG;

  bf_port_mac_tx_enable_set(dev_id, dev_port, true);
  bf_port_mac_rx_enable_set(dev_id, dev_port, true);

  return BF_SUCCESS;
}

/** \brief Start adaptive PCAL on a port
 *
 * \param dev_id  : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param dev_port: encoded port identifier
 *
 * \return: BF_SUCCESS
 * \return: BF_INVALID_ARG: invalid dev_id
 * \return: BF_INVALID_ARG: invalid dev_port
 *
 */
bf_status_t bf_fsm_start_adaptive_tuning(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port) {
  int ln;
  bf_status_t rc;
  int num_lanes = bf_fsm_num_lanes_get(dev_id, dev_port);
  bool no_auto = false;
  port_mgr_port_t *port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  if (!port_p) return BF_INVALID_ARG;

  rc = bf_port_no_auto_adaptive_tuning_get(dev_id, &no_auto);
  if (rc != BF_SUCCESS) return rc;
  if (no_auto) return BF_SUCCESS;

  port_p->lstate.adative_dfe_enabled = true;

  for (ln = 0; ln < num_lanes; ln++) {
    rc = bf_serdes_start_dfe_adaptive(dev_id, dev_port, ln);
    if (rc != BF_SUCCESS) {
      port_mgr_log(
          "FSM :%d:%3d:%d: Error <%d> starting adaptive tuning..ignore",
          dev_id,
          dev_port,
          ln,
          rc);
    }
  }
  return BF_SUCCESS;
}

/** \brief Check for link up
 *
 * \param dev_id  : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param dev_port: encoded port identifier
 *
 * \return: BF_SUCCESS
 * \return: BF_INVALID_ARG: invalid dev_id
 * \return: BF_INVALID_ARG: invalid dev_port
 *
 */
bf_status_t bf_fsm_wait_for_port_up_in_tx_mode(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port) {
  int mac_block, ch, is_cpu_port;
  bool pcs_status;
  int state = 0;
  uint32_t block_lock_per_pcs_lane;
  uint32_t alignment_marker_lock_per_pcs_lane;
  bool hi_ber;
  bool block_lock_all;
  bool alignment_marker_lock_all;
  bf_status_t bf_status;
  port_mgr_port_t *port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  if (port_p == NULL) return BF_INVALID_ARG;

  port_mgr_map_dev_port_to_all(
      dev_id, dev_port, NULL, NULL, &mac_block, &ch, &is_cpu_port);

  if (!bf_fsm_port_is_valid(dev_id, dev_port)) return BF_INVALID_ARG;

  // special-case 1G ports, which don't have the BASE-R PCS
  if (port_p->sw.speed == BF_SPEED_1G) {
    bf_status =
        bf_port_oper_state_get_skip_intr_check(dev_id, dev_port, &state);
    if ((bf_status == BF_SUCCESS) && (state != 0)) {
      return BF_SUCCESS;
    }
    return BF_NOT_READY;
  }

  // check PCS status
  bf_status = bf_port_pcs_status_get(dev_id,
                                     dev_port,
                                     &pcs_status,
                                     &block_lock_per_pcs_lane,
                                     &alignment_marker_lock_per_pcs_lane,
                                     &hi_ber,
                                     &block_lock_all,
                                     &alignment_marker_lock_all);

  if ((pcs_status) && (bf_status == BF_SUCCESS)) {
    // Get MAC status
    bf_status =
        bf_port_oper_state_get_skip_intr_check(dev_id, dev_port, &state);

    port_mgr_log("FSM :%d:%3d:-: PCS sts=%x state:%x",
                 dev_id,
                 dev_port,
                 pcs_status,
                 state);
    if ((bf_status == BF_SUCCESS) && (state != 0)) {
      return BF_SUCCESS;
    }
  }

  (void)block_lock_per_pcs_lane;
  (void)alignment_marker_lock_per_pcs_lane;
  (void)hi_ber;
  (void)block_lock_all;
  (void)alignment_marker_lock_all;

  return BF_NOT_READY;
}

/** \brief Check for link up
 *
 * \param dev_id  : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param dev_port: encoded port identifier
 * \param next_state: pointer to next state, it may be modified by this function
 *        to select a different state to transition to.
 * \param next_state_wait_ms: wait time to next bf_fsm_run execution, it may be
 *        overwritten by this function.
 *
 * \return: BF_SUCCESS
 * \return: BF_INVALID_ARG: invalid dev_id
 * \return: BF_INVALID_ARG: invalid dev_port
 * \return: BF_INVALID_ARG: if any of next_state or next_state_wait_ms is NULL.
 *
 */
bf_status_t bf_fsm_transition_from_up_state(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            bf_fsm_st *next_state,
                                            uint32_t *next_state_wait_ms) {
  port_mgr_port_t *port_p;

  port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  if (!port_p || !next_state || !next_state_wait_ms) {
    return BF_INVALID_ARG;
  }

  if (*next_state == BF_FSM_ST_ABORT && port_p->fsm_ext.port_was_up) {
    // shadow mac stats
    update_historical_mac_stats(dev_id, dev_port);

    // swreset for clear stats
    bf_port_mac_ch_enable_set(dev_id, dev_port, false);
    bf_port_mac_ch_enable_set(dev_id, dev_port, true);
    port_mgr_log("%d:%3d: hw ctrs cleared.", dev_id, dev_port);
    port_p->fsm_ext.port_was_up = false;
  }
  return BF_SUCCESS;
}

/**
 * @}
 */
