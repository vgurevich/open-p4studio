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
#include <stdbool.h>

#include <bf_types/bf_types.h>
#include <dvm/bf_dma_types.h>
#include <dvm/bf_drv_intf.h>
#include <port_mgr/port_mgr_intf.h>
#include <port_mgr/bf_port_if.h>
#include <port_mgr/port_mgr.h>
#include <port_mgr/port_mgr_map.h>
#include <port_mgr/port_mgr_log.h>
#include <port_mgr/bf_fsm_if.h>
#include <port_mgr/bf_fsm_hdlrs.h>
#include <port_mgr/bf_serdes_if.h>

/************************************************************************
 * Port Initialization FSMs
 *
 * This module defines generic Finite State Machine (FSM). FSMs are
 * used by the Port Manager Example code to bring up ports once they
 * have been enabled.
 *
 * The FSMs are useful for amortizing delays, required by certain
 * programming steps, across multiple ports, effectively "sharing" the
 * delays. For example, initializing each serdes slice can take up to
 * 40ms, waiting for the PLLs to stabilize. Executed serially over 256x
 * 25G ports would require a minimum of 256*40ms, or over 10 seconds.
 * However, If all ports are enabled at roughly the same time (for
 * instance on a cold boot) all slices can be programmed then share a
 * single 40ms wait for the PLLs.
 *
 * The states are defined in port_mgr.h and may be extended as necessary.
 * Each state descriptor describes a single state in the FSM.
 *
 * State descriptor variables include the,
 *    state identifier
 *    state handler fn
 *    next state identifier to execute if the handler returns BF_SUCCESS
 *    time before either retrying the current state or transitioning to
 *    the next state
 *    a "transition fn" to be called on transition to next state.
 *
 * Each state handler function has the same prototype,
 *   bf_status_t <handler_fn>( bf_dev_id_t, bf_dev_port_t )
 *
 * The state machine is controlled by the return value from the handlers.
 * If the handler returns,
 *
 *    BF_SUCCES,    transition to next state
 *    BF_NOT_READY, stay in current state
 *    otherwise,    terminate the state machine (some fatal error)
 *
 * There are two FSMs, one for ports requiring clause 73/72 autoneg and
 * link training (bf_fsm_an_default) and one for those that don't
 * (bf_fsm_default).
 ************************************************************************/

// forward declarations
bf_status_t bf_fsm_transition_from_up_state(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            bf_fsm_st *next_state,
                                            uint32_t *next_state_wait_ms);

// AN fsm for platforms with serdes (eval board or real hw)
bf_fsm_state_desc_t bf_fsm_an_w_serdes[] = {

    /* Configure serdes lane 0 to AN speed, tx_drv_en=0
     * Configure other lanes to expected HCD speed, tx_drv_en=0
     *
     */
    {.state = BF_FSM_ST_IDLE,
     .handler = bf_fsm_an_init_serdes,
     .wait_ms = 0,
     .tmout_cycles = BF_FSM_NO_TIMEOUT,

     .next_state = BF_AN_FSM_ST_WAIT_PLL1,
     .next_state_wait_ms = 10,

     .alt_next_state = BF_FSM_ST_ABORT,
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = NULL},

    /* Wait for PLLs to lock
     * Once locked,
     * Set expected link-training mode
     * Configure serdes polarity swaps etc
     * start AN state-machine
     * tx_drv_en=1
     */
    {.state = BF_AN_FSM_ST_WAIT_PLL1,
     .handler = bf_fsm_an_wait_pll1,
     .wait_ms = 10,
     .tmout_cycles = 10,  // supposed to lock in 40ms, 100ms should be enuf

     .next_state = BF_AN_FSM_ST_WAIT_BASE_PG,
     .next_state_wait_ms = 10,

     .alt_next_state = BF_FSM_ST_ABORT,
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = NULL},

    /* If necessary, exchange next pages
     * If not, just got o next state
     */
    {.state = BF_AN_FSM_ST_WAIT_BASE_PG,
     .handler = bf_fsm_an_wait_base_pg,
     .wait_ms = 10,
     .tmout_cycles = (100 * 60),  // 1min until restart AN

     .next_state = BF_AN_FSM_ST_WAIT_NEXT_PG,
     .next_state_wait_ms = 10,

     .alt_next_state = BF_FSM_ST_ABORT,
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = NULL},

    /* If necessary, exchange next pages
     * If not, just got o next state
     */
    {.state = BF_AN_FSM_ST_WAIT_NEXT_PG,
     .handler = bf_fsm_an_wait_next_pg,
     .wait_ms = 10,
     .tmout_cycles = 100,  // 1 sec NEXT PG wait

     .next_state = BF_AN_FSM_ST_WAIT_AN_GOOD,
     .next_state_wait_ms = 0,

     .alt_next_state = BF_FSM_ST_ABORT,
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = NULL},

    /* Wait for AN_GOOD, determines HCD
     * Once HCD is determined,
     * Re-program lane 0 for HCD speed, tx_drv_en=0
     */
    {.state = BF_AN_FSM_ST_WAIT_AN_GOOD,
     .handler = bf_fsm_an_wait_an_good,
     .wait_ms = 10,
     .tmout_cycles = 50,  //=500ms, most it could reasonably take

     .next_state = BF_AN_FSM_ST_WAIT_PLL2,
     .next_state_wait_ms = 10,

     .alt_next_state = BF_FSM_ST_ABORT,
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = NULL},

    /* Wait for AN lane PLLs to lock
     * Once locked,
     * Start link-training on all lanes
     * tx_drv_en=1
     */
    {.state = BF_AN_FSM_ST_WAIT_PLL2,
     .handler = bf_fsm_an_wait_pll2,
     .wait_ms = 10,
     .tmout_cycles = 10,

     .next_state = BF_AN_FSM_ST_WAIT_AN_CMPLT,
     .next_state_wait_ms = 50,

     .alt_next_state = BF_FSM_ST_ABORT,
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = NULL},

    /* Wait for AN_COMPLETE and link-training
     * to finish. If link-training fails restart
     * AN.
     */
    {.state = BF_AN_FSM_ST_WAIT_AN_CMPLT,
     .handler = bf_fsm_an_wait_an_cmplt,
     .wait_ms = 20,
     .tmout_cycles = 50,  // for some reason needs to be larger than 510ms

     .next_state = BF_AN_FSM_ST_WAIT_PCS_UP,
     .next_state_wait_ms = 1000,

     .alt_next_state = BF_FSM_ST_ABORT,
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = NULL},

    {.state = BF_AN_FSM_ST_WAIT_PCS_UP,
     .handler = bf_fsm_an_wait_pcs_up,
     .wait_ms = 1000,
     .tmout_cycles = 10,  // 10 sec max

     .next_state = BF_AN_FSM_ST_WAIT_DWN_EVNT,
     .next_state_wait_ms = 0,

     .alt_next_state = BF_FSM_ST_ABORT,
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = NULL},

    {.state = BF_AN_FSM_ST_WAIT_DWN_EVNT,
     .handler = bf_fsm_an_wait_for_port_dwn_event,
     .wait_ms = 100,
     .tmout_cycles = BF_FSM_NO_TIMEOUT,

     .next_state = BF_FSM_ST_ABORT,
     .next_state_wait_ms = 0,

     .alt_next_state = BF_FSM_ST_ABORT,
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = NULL},

    {.state = BF_FSM_ST_ABORT,
     .handler = bf_fsm_an_abort,
     .wait_ms = 0,
     .tmout_cycles = BF_FSM_NO_TIMEOUT,

     .next_state = BF_FSM_ST_IDLE,  // restart AN
     .next_state_wait_ms = 1000,

     .alt_next_state = BF_FSM_ST_IDLE,  // restart AN
     .alt_next_state_wait_ms = 1000,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = NULL},

    {.state = BF_FSM_ST_END,
     .handler = NULL,
     .wait_ms = 0,
     .tmout_cycles = BF_FSM_NO_TIMEOUT,

     .next_state = BF_FSM_ST_END,
     .next_state_wait_ms = 0,

     .alt_next_state = BF_FSM_ST_END,
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = NULL},
};

// non-AN fsm for platforms with serdes (eval board or real hw)
bf_fsm_state_desc_t bf_fsm_w_serdes[] = {
    {.state = BF_FSM_ST_IDLE,
     .handler = bf_fsm_init_serdes,
     .wait_ms = 0,
     .tmout_cycles = BF_FSM_NO_TIMEOUT,

     .next_state = BF_FSM_ST_WAIT_PLL,
     .next_state_wait_ms = 50,

     .alt_next_state = BF_FSM_ST_ABORT,
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = NULL},

    {.state = BF_FSM_ST_WAIT_PLL,
     .handler = bf_fsm_wait_pll,
     .wait_ms = 10,
     .tmout_cycles = 10,

     .next_state = BF_FSM_ST_CFG_SERDES,
     .next_state_wait_ms = 10,

     .alt_next_state =
         BF_FSM_ST_IDLE,  // drv-1648, need to re-init both tx and rx
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = NULL},

    {.state = BF_FSM_ST_CFG_SERDES,
     .handler = bf_fsm_config_serdes,
     .wait_ms = 0,
     .tmout_cycles = BF_FSM_NO_TIMEOUT,

     .next_state = BF_FSM_ST_WAIT_SIGNAL_OK,
     .next_state_wait_ms =
         400,  // up to 400ms for retimer to re-adapt after !LOS
     //.next_state_wait_ms = 0,

     .alt_next_state = BF_FSM_ST_ABORT,
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = NULL},

    {.state = BF_FSM_ST_WAIT_SIGNAL_OK,
     .handler = bf_fsm_wait_signal_ok,
     .wait_ms = 500,
     .tmout_cycles = BF_FSM_NO_TIMEOUT,

     .next_state = BF_FSM_ST_START_DFE,
     .next_state_wait_ms = 0,

     .alt_next_state = BF_FSM_ST_ABORT,
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = NULL},

    {.state = BF_FSM_ST_START_DFE,
     .handler = bf_fsm_dfe_quick,
     .wait_ms = 0,
     .tmout_cycles = BF_FSM_NO_TIMEOUT,

     .next_state = BF_FSM_ST_WAIT_DFE_DONE,
     .next_state_wait_ms = 500,

     .alt_next_state = BF_FSM_ST_ABORT,
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = NULL},

    {.state = BF_FSM_ST_WAIT_DFE_DONE,
     .handler = bf_fsm_wait_dfe_done,
     .wait_ms = 200,
     .tmout_cycles = 50,  // 10 seconds

     .next_state = BF_FSM_ST_WAIT_PCS_UP,
     .next_state_wait_ms = 100,

     .alt_next_state = BF_FSM_ST_ABORT,
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = NULL},

    /* This timeout needs to be long enough to cover the
     * case where optical links are used and the QSFP fsm
     * may be slow to de-assert TX_DISABLE. It has been
     * seen that QSFP processing can extend into this
     * state. If this timeout is too short it could cause
     * this FSM to give up even before the optical links
     * have been fully enabled.
     */
    {.state = BF_FSM_ST_WAIT_PCS_UP,
     .handler = bf_fsm_wait_pcs_up,
     .wait_ms = 100,
     .tmout_cycles = 20,

     .next_state = BF_FSM_ST_WAIT_DWN_EVNT,
     .next_state_wait_ms = 0,

     .alt_next_state = BF_FSM_ST_ABORT,
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_REMOTE_FAULT,
     .alt_next_state_2_wait_ms = 100,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = bf_fsm_transition_from_up_state},

    {.state = BF_FSM_ST_REMOTE_FAULT,
     .handler = bf_fsm_remote_fault,
     .wait_ms = 100,
     .tmout_cycles = BF_FSM_NO_TIMEOUT,

     .next_state = BF_FSM_ST_WAIT_DWN_EVNT,
     .next_state_wait_ms = 100,

     .alt_next_state = BF_FSM_ST_ABORT,
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_WAIT_PCS_UP,
     .alt_next_state_3_wait_ms = 100,
     .transition_fn = bf_fsm_transition_from_up_state},

    {.state = BF_FSM_ST_WAIT_DWN_EVNT,
     .handler = bf_fsm_wait_for_port_dwn_event,
     .wait_ms = 100,
     .tmout_cycles = BF_FSM_NO_TIMEOUT,

     .next_state = BF_FSM_ST_ABORT,
     .next_state_wait_ms = 0,

     .alt_next_state = BF_FSM_ST_ABORT,
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_WAIT_PCS_UP,
     .alt_next_state_2_wait_ms = 100,
     .alt_next_state_3 = BF_FSM_ST_REMOTE_FAULT,
     .alt_next_state_3_wait_ms = 100,
     .transition_fn = bf_fsm_transition_from_up_state},

    {.state = BF_FSM_ST_ABORT,
     .handler = bf_fsm_abort,
     .wait_ms = 0,
     .tmout_cycles = BF_FSM_NO_TIMEOUT,

     .next_state = BF_FSM_ST_RE_INIT_SERDES_RX,
     .next_state_wait_ms = 3000,

     .alt_next_state = BF_FSM_ST_END,
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = NULL},

    {.state = BF_FSM_ST_RE_INIT_SERDES_RX,
     .handler = bf_fsm_re_init_serdes_rx,
     .wait_ms = 0,
     .tmout_cycles = BF_FSM_NO_TIMEOUT,

     .next_state = BF_FSM_ST_WAIT_RX_PLL,
     .next_state_wait_ms = 50,

     .alt_next_state = BF_FSM_ST_END,
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = NULL},

    {.state = BF_FSM_ST_WAIT_RX_PLL,
     .handler = bf_fsm_wait_pll,
     .wait_ms = 10,
     .tmout_cycles = 10,

     .next_state = BF_FSM_ST_RE_CFG_SERDES_RX,
     .next_state_wait_ms = 0,

     .alt_next_state = BF_FSM_ST_ABORT,
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = NULL},

    {.state = BF_FSM_ST_RE_CFG_SERDES_RX,
     .handler = bf_fsm_re_config_serdes_rx,
     .wait_ms = 0,
     .tmout_cycles = BF_FSM_NO_TIMEOUT,

     .next_state = BF_FSM_ST_WAIT_SIGNAL_OK,
     .next_state_wait_ms = 50,

     .alt_next_state = BF_FSM_ST_ABORT,
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = NULL},

    {.state = BF_FSM_ST_END,
     .handler = NULL,
     .wait_ms = 0,
     .tmout_cycles = BF_FSM_NO_TIMEOUT,

     .next_state = BF_FSM_ST_END,
     .next_state_wait_ms = 0,

     .alt_next_state = BF_FSM_ST_END,
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = NULL},
};

// non-AN fsm with serdes enabled in tx-direction only, rx is Donot care.
bf_fsm_state_desc_t bf_fsm_w_serdes_for_tx_mode[] = {
    {.state = BF_FSM_ST_IDLE,
     .handler = bf_fsm_init_serdes,
     .wait_ms = 0,
     .tmout_cycles = BF_FSM_NO_TIMEOUT,

     .next_state = BF_FSM_ST_WAIT_PLL,
     .next_state_wait_ms = 50,

     .alt_next_state = BF_FSM_ST_ABORT,
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = NULL},

    {.state = BF_FSM_ST_WAIT_PLL,
     .handler = bf_fsm_wait_pll,
     .wait_ms = 10,
     .tmout_cycles = 10,

     .next_state = BF_FSM_ST_CFG_SERDES,
     .next_state_wait_ms = 0,

     .alt_next_state =
         BF_FSM_ST_IDLE,  // drv-1648, need to re-init both tx and rx
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = NULL},

    {.state = BF_FSM_ST_CFG_SERDES,
     .handler = bf_fsm_config_for_tx_mode,
     .wait_ms = 0,
     .tmout_cycles = BF_FSM_NO_TIMEOUT,

     .next_state = BF_FSM_ST_WAIT_PCS_UP,
     .next_state_wait_ms =
         400,  // up to 400ms for retimer to re-adapt after !LOS.
     //.next_state_wait_ms = 0,

     .alt_next_state = BF_FSM_ST_ABORT,
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = NULL},

    {.state = BF_FSM_ST_WAIT_PCS_UP,
     .handler = bf_fsm_wait_for_port_up_in_tx_mode,
     .wait_ms = 100,
     .tmout_cycles = BF_FSM_NO_TIMEOUT,

     .next_state = BF_FSM_ST_WAIT_DWN_EVNT, /* there is nothing todo */
     .next_state_wait_ms = 0,

     .alt_next_state = BF_FSM_ST_ABORT,
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = NULL},

    {.state = BF_FSM_ST_WAIT_DWN_EVNT,
     .handler = bf_fsm_wait_for_port_dwn_event,
     .wait_ms = 100,
     .tmout_cycles = BF_FSM_NO_TIMEOUT,

     .next_state = BF_FSM_ST_ABORT,
     .next_state_wait_ms = 0,

     .alt_next_state = BF_FSM_ST_ABORT,
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = NULL},

    {.state = BF_FSM_ST_ABORT,
     .handler = bf_fsm_default_abort,
     .wait_ms = 0,
     .tmout_cycles = BF_FSM_NO_TIMEOUT,

     .next_state = BF_FSM_ST_IDLE,
     .next_state_wait_ms = 0,

     .alt_next_state = BF_FSM_ST_END,
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = NULL},

    {.state = BF_FSM_ST_END,
     .handler = NULL,
     .wait_ms = 0,
     .tmout_cycles = BF_FSM_NO_TIMEOUT,

     .next_state = BF_FSM_ST_END,
     .next_state_wait_ms = 0,

     .alt_next_state = BF_FSM_ST_END,
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = NULL},
};

// FSMs for architectural model (which doesn't include serdes)

// AN fsm
bf_fsm_state_desc_t bf_fsm_an_default[] = {
    {.state = BF_FSM_ST_IDLE,
     .handler = bf_fsm_ena_mac,
     .wait_ms = 0,
     .tmout_cycles = BF_FSM_NO_TIMEOUT,

     .next_state = BF_FSM_ST_END,
     .next_state_wait_ms = 0,

     .alt_next_state = BF_FSM_ST_END,
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = NULL},
    {.state = BF_FSM_ST_END,
     .handler = NULL,
     .wait_ms = 0,
     .tmout_cycles = BF_FSM_NO_TIMEOUT,

     .next_state = BF_FSM_ST_END,
     .next_state_wait_ms = 0,

     .alt_next_state = BF_FSM_ST_END,
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = NULL},
};

// non-AN fsm
bf_fsm_state_desc_t bf_fsm_default[] = {
    {.state = BF_FSM_ST_IDLE,
     .handler = bf_fsm_ena_mac,
     .wait_ms = 0,
     .tmout_cycles = BF_FSM_NO_TIMEOUT,

     .next_state = BF_FSM_ST_ENABLE_MAC_TX_RX,
     .next_state_wait_ms = 0,

     .alt_next_state = BF_FSM_ST_END,
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = NULL},

    {.state = BF_FSM_ST_ENABLE_MAC_TX_RX,
     .handler = bf_fsm_enable_mac_tx_rx,
     .wait_ms = 0,

     .next_state = BF_FSM_ST_ASSERT_RS_FEC,
     .next_state_wait_ms = 0,

     .alt_next_state = BF_FSM_ST_END,
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = NULL},

    {.state = BF_FSM_ST_ASSERT_RS_FEC,
     .handler = bf_fsm_assert_rs_fec,
     .wait_ms = 0,

     .next_state = BF_FSM_ST_DEASSERT_RS_FEC,
     .next_state_wait_ms = 50,

     .alt_next_state = BF_FSM_ST_END,
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = NULL},

    {.state = BF_FSM_ST_DEASSERT_RS_FEC,
     .handler = bf_fsm_deassert_rs_fec,
     .wait_ms = 0,

     .next_state = BF_FSM_ST_WAIT_LPBK_UP,
     .next_state_wait_ms = 50,

     .alt_next_state = BF_FSM_ST_END,
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = NULL},

    {.state = BF_FSM_ST_WAIT_LPBK_UP,
     .handler = bf_fsm_wait_lpbk_port_up,
     .wait_ms = 50,
     .tmout_cycles = 5,

     .next_state = BF_FSM_ST_WAIT_DWN_EVNT,
     .next_state_wait_ms = 50,

     .alt_next_state = BF_FSM_ST_END,
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = NULL},

    {.state = BF_FSM_ST_WAIT_DWN_EVNT,
     .handler = bf_fsm_wait_for_port_dwn_event,
     .wait_ms = 100,
     .tmout_cycles = BF_FSM_NO_TIMEOUT,

     .next_state = BF_FSM_ST_ABORT,
     .next_state_wait_ms = 0,

     .alt_next_state = BF_FSM_ST_END,
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = NULL},

    {.state = BF_FSM_ST_ABORT,
     .handler = bf_fsm_default_abort,
     .wait_ms = 0,
     .tmout_cycles = BF_FSM_NO_TIMEOUT,

     .next_state = BF_FSM_ST_IDLE,
     .next_state_wait_ms = 0,

     .alt_next_state = BF_FSM_ST_IDLE,
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = NULL},

    {.state = BF_FSM_ST_END,
     .handler = NULL,
     .wait_ms = 0,
     .tmout_cycles = BF_FSM_NO_TIMEOUT,

     .next_state = BF_FSM_ST_END,
     .next_state_wait_ms = 0,

     .alt_next_state = BF_FSM_ST_END,
     .alt_next_state_wait_ms = 0,
     .alt_next_state_2 = BF_FSM_ST_END,
     .alt_next_state_2_wait_ms = 0,
     .alt_next_state_3 = BF_FSM_ST_END,
     .alt_next_state_3_wait_ms = 0,
     .transition_fn = NULL},
};

/************************************************************************
 * bf_get_non_serdes_fsm
 *
 * Return the LLD non serdes FSM.
 ************************************************************************/
bf_fsm_t bf_get_fsm_for_tx_mode(void) { return bf_fsm_w_serdes_for_tx_mode; }

/************************************************************************
 * bf_get_non_serdes_fsm
 *
 * Return the LLD non serdes FSM.
 ************************************************************************/
bf_fsm_t bf_get_non_serdes_fsm() { return bf_fsm_default; }

/************************************************************************
 * bf_get_default_fsm
 *
 * Return the LLD "default" FSM, based o whether autoneg is required
 * or not.
 ************************************************************************/
bf_fsm_t bf_get_default_fsm(bf_dev_id_t dev_id, bool autoneg_enabled) {
  bool is_sw_model;
  bf_status_t sts;

  sts = bf_drv_device_type_get(dev_id, &is_sw_model);
  if (sts != BF_SUCCESS) {
    port_mgr_log_error(
        "device type get failed for device %d to get fsm, "
        "status %s (%d)",
        dev_id,
        bf_err_str(sts),
        sts);

    /* Set to asic in error case */
    is_sw_model = false;
  }

  if (is_sw_model) {
    /* For platforms that do not model serdes use default FSM */
    return bf_fsm_default;
  } else {
    if (autoneg_enabled) {
      return bf_fsm_an_w_serdes;
    }
    return bf_fsm_w_serdes;
  }
}

/************************************************************************
 * bf_fsm_st_to_str
 *
 * enum-to-str fn for display purposes
 ************************************************************************/
char *bf_fsm_st_to_str[] = {
    // non AN states
    "BF_FSM_ST_IDLE            ",
    "BF_FSM_ST_WAIT_PLL        ",
    "BF_FSM_ST_CFG_SERDES      ",
    "BF_FSM_ST_WAIT_SIGNAL_OK  ",
    "BF_FSM_ST_START_DFE       ",
    "BF_FSM_ST_WAIT_DFE_DONE   ",
    "BF_FSM_ST_ENA_MAC         ",
    "BF_FSM_ST_WAIT_PCS_UP     ",
    "BF_FSM_ST_REMOTE_FAULT    ",
    "BF_FSM_ST_WAIT_PCS_DN     ",
    "BF_FSM_ST_WAIT_DWN_EVNT   ",
    "BF_FSM_ST_END             ",
    "BF_FSM_ST_RE_INIT_SERDES_RX",
    "BF_FSM_ST_WAIT_RX_PLL     ",
    "BF_FSM_ST_RE_CFG_SERDES_RX",
    // AN states
    "BF_AN_FSM_ST_IDLE         ",
    "BF_AN_FSM_ST_WAIT_PLL1    ",
    "BF_AN_FSM_ST_CFG_SERDES1  ",
    "BF_AN_FSM_ST_START_AN     ",
    "BF_AN_FSM_ST_WAIT_BASE_PG ",
    "BF_AN_FSM_ST_WAIT_NEXT_PG ",
    "BF_AN_FSM_ST_WAIT_AN_GOOD ",
    "BF_AN_FSM_ST_PGM_HCD      ",
    "BF_AN_FSM_ST_WAIT_PLL2    ",
    "BF_AN_FSM_ST_CFG_SERDES2  ",
    "BF_AN_FSM_ST_START_LT     ",
    "BF_AN_FSM_ST_WAIT_LT_DONE ",
    "BF_AN_FSM_ST_WAIT_AN_CMPLT",
    "BF_AN_FSM_ST_WAIT_PCS_UP  ",
    "BF_AN_FSM_ST_WAIT_PCS_DN  ",
    "BF_AN_FSM_ST_WAIT_PCAL_DONE",
    "BF_AN_FSM_ST_WAIT_DWN_EVNT",
    "BF_AN_FSM_ST_END          ",
    // special "clean-up" state (optional)
    "BF_FSM_ST_ABORT           ",
    // Non-Serdes FSM states
    "BF_FSM_ST_ENABLE_MAC_TX_RX",
    "BF_FSM_ST_ASSERT_RS_FEC   ",
    "BF_FSM_ST_DEASSERT_RS_FEC ",
    "BF_FSM_ST_WAIT_LPBK_UP"};

/************************************************************************
 * bf_fsm_find_state
 *
 * Look up the state descriptor corresponding to the passed "st" in the
 * passed "fsm".
 *
 * Each fsm is terminated by a null entry with cur_st=BF_FSM_ST_END.
 ************************************************************************/
bf_fsm_state_desc_t *bf_fsm_find_state(bf_fsm_state_desc_t *fsm, bf_fsm_st st) {
  bf_fsm_state_desc_t *desc_p = &fsm[0];

  while (desc_p->state != st) {
    desc_p++;
    // dont run off the end
    bf_sys_assert(desc_p->state != BF_FSM_ST_END);
  }
  return (desc_p);
}

/*
typedef bf_status_t (*bf_fsm_transition_fn)(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            bf_fsm_st *next_state,
                                            uint32_t *next_state_wait_ms);*/

bf_status_t bf_fsm_bind_cb_to_state(bf_fsm_state_desc_t *fsm,
                                    bf_fsm_st st,
                                    bf_fsm_transition_fn cb) {
  bf_fsm_state_desc_t *desc_p = bf_fsm_find_state(fsm, st);

  if (desc_p == NULL) return BF_INVALID_ARG;

  desc_p->transition_fn = cb;

  return BF_SUCCESS;
}

/*************************************************************************
 * bf_fsm_run
 *
 * Run the port thru its current state in the fsm. The state handling fn
 * returns one of,
 *
 * BF_SUCCESS, indicates the state completed successfully, transition to
 *             nxt_st
 * BF_NOT_READY, indicates the current state is incomplete and must be called
 *               again. No transition.
 * Anything else, indicates some error that will terminate fsm.
 *
 * This function returns the time to wait until the next state or the
 * next invocation of the current state (on BF_SUCCESS or BF_NOT_READY)
 * Otherwise, returns next wait time = BF_FSM_ABORT which will terminate
 * the tasklet.
 *************************************************************************/
bf_status_t bf_fsm_run(bf_dev_id_t dev_id,
                       bf_dev_port_t dev_port,
                       bf_fsm_t fsm,
                       bf_fsm_st fsm_st_current,
                       // returned values
                       bf_fsm_st *fsm_st_next,
                       uint32_t *wait_ms) {
  bf_status_t st_rc;
  bf_fsm_state_desc_t *desc_p = NULL;
  port_mgr_port_t *port_p = port_mgr_map_dev_port_to_port(dev_id, dev_port);
  bf_fsm_st expected_next_state = BF_FSM_ST_END;
  uint32_t expected_wait_ms = 0;
  char *alt_str = "";

  if (port_p == NULL) return BF_INVALID_ARG;
  if (!port_p->sw.assigned) return BF_INVALID_ARG;
  if (!port_p->sw.enabled) return BF_INVALID_ARG;
  if (fsm == NULL) return BF_INVALID_ARG;
  if (wait_ms == NULL) return BF_INVALID_ARG;

  // find the fsm_st state descriptor from the fsm
  desc_p = bf_fsm_find_state(fsm, fsm_st_current);
  if (desc_p == NULL) {
    port_mgr_log("FSM :%d:%3d:-: state=%d : state not found?!",
                 dev_id,
                 dev_port,
                 fsm_st_current);
    return BF_INVALID_ARG;
  }
  // make sure we dont jump off to 0 if user forgets to add a func ptr
  if (desc_p->handler != NULL) {
    // default value for opt_next_state is BF_FSM_ST_IDLE for both, non-AN and
    // AN state machines. The handler may override that value. If it is not
    // overwritten (it remains equal to BF_FSM_ST_IDLE), optional next state
    // is ignored.
    st_rc = desc_p->handler(dev_id, dev_port);
  } else {
    st_rc = BF_INVALID_ARG;
  }

  if (st_rc == BF_NOT_READY) {
    uint32_t time_in_state;

    if (desc_p->tmout_cycles != BF_FSM_NO_TIMEOUT) {
      // check for timeout
      bf_port_time_in_state_get(dev_id, dev_port, &time_in_state);
      time_in_state++;
      bf_port_time_in_state_set(dev_id, dev_port, time_in_state);
      if (0) {
        port_mgr_log("FSM :%d:%3d:-: %s <%d of %d>",
                     dev_id,
                     dev_port,
                     bf_fsm_st_to_str[desc_p->state],
                     time_in_state,
                     desc_p->tmout_cycles);
      }
      if (time_in_state >= desc_p->tmout_cycles) {
        st_rc = BF_INVALID_ARG;  // cause abort to be next state
        port_mgr_log("FSM :%d:%3d:-: %s <timeout>",
                     dev_id,
                     dev_port,
                     bf_fsm_st_to_str[desc_p->state]);
      }
    }
    if (st_rc == BF_NOT_READY) {
      // no transition. stay in current state
      *fsm_st_next = fsm_st_current;
      *wait_ms = desc_p->wait_ms;
      return BF_SUCCESS;
    }
  }
  if (st_rc == BF_SUCCESS) {
    expected_next_state = desc_p->next_state;
    if (desc_p->next_state_custom_wait_ms)
      expected_wait_ms = desc_p->next_state_custom_wait_ms;
    else
      expected_wait_ms = desc_p->next_state_wait_ms;
  } else if (st_rc == BF_ALT2_NEXT_ST) {
    expected_next_state = desc_p->alt_next_state_2;
    expected_wait_ms = desc_p->alt_next_state_2_wait_ms;
    alt_str = "(alt 2)";
  } else if (st_rc == BF_ALT3_NEXT_ST) {
    expected_next_state = desc_p->alt_next_state_3;
    expected_wait_ms = desc_p->alt_next_state_3_wait_ms;
    alt_str = "(alt 3)";
  } else {
    // st_rc is anything else different from previous error codes.
    expected_next_state = desc_p->alt_next_state;
    expected_wait_ms = desc_p->alt_next_state_wait_ms;
    alt_str = "(alt)";
  }

  port_mgr_log("FSM :%d:%3d:-: %s --> %s %s",
               dev_id,
               dev_port,
               bf_fsm_st_to_str[desc_p->state],
               bf_fsm_st_to_str[expected_next_state],
               alt_str);

  // transition to next state
  port_p->fsm_ext.debounce_cnt = 0;

  if (desc_p->transition_fn) {
    desc_p->transition_fn(
        dev_id, dev_port, &expected_next_state, &expected_wait_ms);
  }
  *fsm_st_next = expected_next_state;
  if (expected_next_state == BF_FSM_ST_END) {
    *wait_ms = BF_FSM_TERMINATE;  // normal termination of fsm
  } else {
    *wait_ms = expected_wait_ms;
  }
  bf_port_time_in_state_set(dev_id, dev_port, 0);
  return BF_SUCCESS;
}

void bf_fsm_set_dfe_retry_time(uint32_t wait_ms) {
  bf_fsm_state_desc_t *desc_p =
      bf_fsm_find_state(bf_fsm_w_serdes, BF_FSM_ST_ABORT);
  desc_p->next_state_custom_wait_ms = wait_ms;
}

uint32_t bf_fsm_get_dfe_retry_time(void) {
  bf_fsm_state_desc_t *desc_p =
      bf_fsm_find_state(bf_fsm_w_serdes, BF_FSM_ST_ABORT);
  /* To get the default value next_state_custom_wait_ms should be 0 */
  if (desc_p->next_state_custom_wait_ms)
    return desc_p->next_state_custom_wait_ms;
  else
    return desc_p->next_state_wait_ms;
}
