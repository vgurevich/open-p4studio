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


#ifndef BF_FSM_H_INCLUDED
#define BF_FSM_H_INCLUDED

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

// FSM extended return codes
#define BF_ALT2_NEXT_ST 0x20000
#define BF_ALT3_NEXT_ST 0x30000

// PCS lock sample threshold: number of pcs-lock consecutive reads needed to
// move to the following state.
#define BF_FSM_PCS_LOCK_THRSHLD 2
// PCS lock sample threshold: for optical modules.
#define BF_FSM_PCS_LOCK_THRSHLD_OPT 8
// PCS lock sample threshold: for non FEC Ethernet modes
#define BF_FSM_PCS_LOCK_THRSHLD_NO_FEC 4
// Link UP sample threshold: number of state=UP consecutive reads needed to
// move to the following state.
#define BF_FSM_LINK_UP_THRSHLD 2
// Qualifying eye threshold for non-FEC modes
#define BF_FSM_QUALIF_EYE_HT_THRESH_NO_FEC 6
// Qualifying eye threshold for FEC modes
#define BF_FSM_QUALIF_EYE_HT_THRESH_FEC 0

typedef enum {

  // non-AN fsm states
  BF_FSM_ST_IDLE = 0,
  BF_FSM_ST_WAIT_PLL,
  BF_FSM_ST_CFG_SERDES,
  BF_FSM_ST_WAIT_SIGNAL_OK,
  BF_FSM_ST_START_DFE,
  BF_FSM_ST_WAIT_DFE_DONE,
  BF_FSM_ST_ENA_MAC,
  BF_FSM_ST_WAIT_PCS_UP,
  BF_FSM_ST_REMOTE_FAULT,
  BF_FSM_ST_WAIT_PCS_DN,
  BF_FSM_ST_WAIT_DWN_EVNT,
  BF_FSM_ST_END,
  BF_FSM_ST_RE_INIT_SERDES_RX,
  BF_FSM_ST_WAIT_RX_PLL,
  BF_FSM_ST_RE_CFG_SERDES_RX,

  // AN fsm states
  BF_AN_FSM_ST_IDLE,
  BF_AN_FSM_ST_WAIT_PLL1,
  BF_AN_FSM_ST_CFG_SERDES1,
  BF_AN_FSM_ST_START_AN,
  BF_AN_FSM_ST_WAIT_BASE_PG,
  BF_AN_FSM_ST_WAIT_NEXT_PG,
  BF_AN_FSM_ST_WAIT_AN_GOOD,
  BF_AN_FSM_ST_PGM_HCD,
  BF_AN_FSM_ST_WAIT_PLL2,
  BF_AN_FSM_ST_CFG_SERDES2,
  BF_AN_FSM_ST_START_LT,
  BF_AN_FSM_ST_WAIT_LT_DONE,
  BF_AN_FSM_ST_WAIT_AN_CMPLT,
  BF_AN_FSM_ST_WAIT_PCS_UP,
  BF_AN_FSM_ST_WAIT_PCS_DN,
  BF_AN_FSM_ST_WAIT_PCAL_DONE,
  BF_AN_FSM_ST_WAIT_DWN_EVNT,
  BF_AN_FSM_ST_END,

  // Special "clean-up" state (optional
  BF_FSM_ST_ABORT,

  // Non-Serdes FSM states
  BF_FSM_ST_ENABLE_MAC_TX_RX,
  BF_FSM_ST_ASSERT_RS_FEC,
  BF_FSM_ST_DEASSERT_RS_FEC,
  BF_FSM_ST_WAIT_LPBK_UP
} bf_fsm_st;

/* Signature for a state handler function, executed
 *  by the FSM on each call */
typedef bf_status_t (*bf_fsm_fn)(bf_dev_id_t dev_id, bf_dev_port_t dev_port);

/* Signature for a state-transition function, executed
 *  (optionally) on transition from the current state. This
 *  may be specified as NULL if no specific function is
 *  required to execute (once) when entering a new state.
 *  If specified, this function is passed pointers to the
 *  (expected) next state and wait times. These may be modified
 *  by the function (for example, to provide a multi-way branch
 *  from one state to one of several others).
 */
typedef bf_status_t (*bf_fsm_transition_fn)(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            bf_fsm_st *next_state,
                                            uint32_t *next_state_wait_ms);

// Abort indication if returned as "nxt_wait_ms"
#define BF_FSM_TERMINATE 0xffffffff

// No FSM state timeout
#define BF_FSM_NO_TIMEOUT 0xffffffff

/* Generic Finite State Machine (FSM) state descriptor.
 *  Each state in a state-machine is described by one
 *  bf_fsm_state_desc_t structure.
 *  The FSM defines a set of states, next states, and the
 *  amount of time that should elapse between calls to
 *  bf_fsm_run for a given state machine and state.
 *  The FSM provides a mechanism for two-way branching
 *  on transition from any state. This is envisioned
 *  to be used primarily as "good path"/'error path"
 *  branching, but can be used for any 2-way branching
 *  purpose.
 *  The FSM also provides a transition function to be
 *  specified to execute on transition FROM any state. This
 *  function is optional. If specified it can be used to
 *  set initial conditions upon entry to a state. It may
 *  also modify the next state and next state wait times,
 *  allowing m-way branching on transition.
 */
typedef struct bf_fsm_state_desc_t {
  bf_fsm_st state;        // current fsm state
  bf_fsm_fn handler;      // function executed when in cur_st
  uint32_t wait_ms;       // wait time to next bf_fsm_run invocation if
                          //   no transition (BF_NOT_READY)
  uint32_t tmout_cycles;  // max time in state (returning BF_NOT_READY)

  bf_fsm_st next_state;  // next state to transition to if cur_fn succeeds
  uint32_t next_state_wait_ms;  // wait time to next bf_fsm_run invocation if
                                //   transitioning (BF_SUCCESS)
  uint32_t next_state_custom_wait_ms;  // custom wait time to next bf_fsm_run
                                       // invocation if
                                       //   transitioning (BF_SUCCESS)

  bf_fsm_st alt_next_state;  // next state to transition to if cur_fn returns
                             //  an error code other than BF_SUCCESS,
                             //  BF_NOT_READY, BF_ALT2_NEXT_ST or
                             //  BF_ALT3_NEXT_ST
  uint32_t
      alt_next_state_wait_ms;  // wait time to next bf_fsm_run invocation if
                               //  anything other than BF_SUCCESS or
                               //  BF_NOT_READY, BF_ALT3_NEXT_ST or
                               //  BF_ALT3_NEXT_ST is returned from handler
  bf_fsm_st alt_next_state_2;  // next state to transition to if cur_fn
                               //  succeeds with error code BF_ALT2_NEXT_ST.
  uint32_t
      alt_next_state_2_wait_ms;  // wait time to next bf_fsm_run invocation if
                                 //   transitioning (BF_ALT2_NEXT_ST)
  bf_fsm_st alt_next_state_3;    // next state to transition to if cur_fn
                                 //  succeeds with error code BF_ALT3_NEXT_ST.
  uint32_t
      alt_next_state_3_wait_ms;  // wait time to next bf_fsm_run invocation if
                                 //   transitioning (BF_ALT3_NEXT_ST)
  bf_fsm_transition_fn
      transition_fn;  // function to execute if transitioning to
} bf_fsm_state_desc_t;

typedef bf_fsm_state_desc_t *bf_fsm_t;

bf_fsm_t bf_get_non_serdes_fsm();
bf_fsm_t bf_get_default_fsm(bf_dev_id_t dev_id, bool autoneg_enabled);
bf_status_t bf_fsm_bind_cb_to_state(bf_fsm_state_desc_t *fsm,
                                    bf_fsm_st st,
                                    bf_fsm_transition_fn cb);
bf_status_t bf_fsm_run(bf_dev_id_t dev_id,
                       bf_dev_port_t dev_port,
                       bf_fsm_t fsm,
                       bf_fsm_st fsm_st_current,
                       // returned values
                       bf_fsm_st *fsm_st_next,
                       uint32_t *wait_ms);
bf_fsm_t bf_get_fsm_for_tx_mode(void);

void bf_fsm_set_dfe_retry_time(uint32_t wait_ms);
uint32_t bf_fsm_get_dfe_retry_time(void);

#ifdef __cplusplus
}
#endif /* C++ */

#endif  // BF_FSM_H_INCLUDED
