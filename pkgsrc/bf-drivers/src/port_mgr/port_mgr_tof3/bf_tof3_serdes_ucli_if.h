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

#ifndef UCLI_IF_INCLUDED
#define UCLI_IF_INCLUDED

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// static config for evb testing
typedef struct lane_cfg_t {
  uint32_t rx_term;
  uint32_t tx_rate;
  uint32_t rx_rate;
  uint32_t tx_width;
  uint32_t rx_width;
  uint32_t tx_pstate;
  uint32_t rx_pstate;
  uint32_t cm3;
  uint32_t cm2;
  uint32_t cm1;
  uint32_t c0;
  uint32_t c1;
  uint32_t prbs_mode;
  uint64_t user_data_pat;
  uint32_t prbs_gen_tx_en;
  uint32_t rx_prbs_chk_en;
  uint32_t tx_precoder_override;
  uint32_t tx_precoder_en_gc;
  uint32_t tx_precoder_en_pc;
  uint32_t rx_precoder_en_gc;
  uint32_t rx_precoder_en_pc;
  uint32_t tx_polarity;
  uint32_t rx_polarity;
  uint32_t loopback_mode;
  uint32_t rx_ctle_adapt_en;
  uint32_t rx_ctle_adapt_boost;
  uint32_t tx_disable;  // currently unused
  uint32_t rx_disable;  // currently unused
  uint32_t tx_reset;    // currently unused
  uint32_t rx_reset;    // currently unused

  // statuses (used by ucli)
  uint32_t init_status;
  uint32_t sig_det;
  uint32_t cdr_lock;
  uint32_t bist_lock;
  double ber;
  int32_t tx_ppm;
  int32_t rx_ppm;
} lane_cfg_t;

bf_status_t bf_tof3_serdes_status_get(uint32_t dev_id,
                                      uint32_t dev_port,
                                      uint32_t ln,
                                      lane_cfg_t *rtn_struct);

#ifdef __cplusplus
}
#endif /* C++ */

#endif  // UCLI_IF_INCLUDED
