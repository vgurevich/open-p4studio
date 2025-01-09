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

/* clang-format off */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/*********************************************************************
* bf_ll_serdes_tx_0x7a_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x7a_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x7a_lock_ph_flip_get
* 
* TRF output polarity. Set to 1b. 
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x7a_lock_ph_flip_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x7a_lock_ph_flip_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x7a_lock_ph_flip_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x7a_spd_sel2_get
* 
* TRF speed selection MSB:
*     Set to 01b if RX baud rate is 25.78G and TX baud rate is 26.56G.
*     Otherwise, set to 00b.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x7a_spd_sel2_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x7a_spd_sel2_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x7a_spd_sel2_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x7a_spd_sel1_get
* 
* TRF speed selection LSB:
*     Set to 01b if RX baud rate is 26.56G and TX baud rate is 25.78G.
*     Otherwise, set to 00b.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x7a_spd_sel1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x7a_spd_sel1_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x7a_spd_sel1_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x7a_freq_mup_get
* 
* TRF control bit. Always set to 010b.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x7a_freq_mup_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x7a_freq_mup_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x7a_freq_mup_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x7a_freq_multiplier_get
* 
* TRF control bit:
*     Set to 01b if RX baud rate is 25.78G, and TX baud rate is 26.56G.
*     Set to 10b if RX baud rate is 26.56G and TX baud rate is 25.78G.
*     Otherwise, set to 00b.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x7a_freq_multiplier_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x7a_freq_multiplier_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x7a_freq_multiplier_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x7a_freq_shift_sel_get
* 
* TRF control bit. Always set to 100b.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x7a_freq_shift_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x7a_freq_shift_sel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x7a_freq_shift_sel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x0_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x0_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x0_tx_l_0_sel_get
* 
* Which physical lane is used for logical lane 0 for TX
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x0_tx_l_0_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x0_tx_l_0_sel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x0_tx_l_0_sel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x0_rsvd1_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x0_rsvd1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x0_tx_l_1_sel_get
* 
* Which physical lane is used for logical lane 1 for TX
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x0_tx_l_1_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x0_tx_l_1_sel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x0_tx_l_1_sel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x0_rsvd2_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x0_rsvd2_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x0_tx_l_2_sel_get
* 
* Which physical lane is used for logical lane 2 for TX
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x0_tx_l_2_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x0_tx_l_2_sel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x0_tx_l_2_sel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x0_rsvd3_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x0_rsvd3_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x0_tx_l_3_sel_get
* 
* Which physical lane is used for logical lane 3 for TX
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x0_tx_l_3_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x0_tx_l_3_sel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x0_tx_l_3_sel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x1_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x1_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x1_tx_l_4_sel_get
* 
* Which physical lane is used for logical lane 4 for TX
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x1_tx_l_4_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x1_tx_l_4_sel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x1_tx_l_4_sel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x1_rsvd4_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x1_rsvd4_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x1_tx_l_5_sel_get
* 
* Which physical lane is used for logical lane 5 for TX
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x1_tx_l_5_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x1_tx_l_5_sel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x1_tx_l_5_sel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x1_rsvd5_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x1_rsvd5_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x1_tx_l_6_sel_get
* 
* Which physical lane is used for logical lane 6 for TX
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x1_tx_l_6_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x1_tx_l_6_sel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x1_tx_l_6_sel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x1_rsvd6_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x1_rsvd6_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x1_tx_l_7_sel_get
* 
* Which physical lane is used for logical lane 7 for TX
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x1_tx_l_7_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x1_tx_l_7_sel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x1_tx_l_7_sel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x2_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x2_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x2_rx_l_0_sel_get
* 
* Which physical lane is used for logical lane 0 for Rx
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x2_rx_l_0_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x2_rx_l_0_sel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x2_rx_l_0_sel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x2_rsvd7_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x2_rsvd7_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x2_rx_l_1_sel_get
* 
* Which physical lane is used for logical lane 1 for Rx
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x2_rx_l_1_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x2_rx_l_1_sel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x2_rx_l_1_sel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x2_rsvd8_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x2_rsvd8_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x2_rx_l_2_sel_get
* 
* Which physical lane is used for logical lane 2 for Rx
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x2_rx_l_2_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x2_rx_l_2_sel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x2_rx_l_2_sel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x2_rsvd9_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x2_rsvd9_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x2_rx_l_3_sel_get
* 
* Which physical lane is used for logical lane 3 for Rx
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x2_rx_l_3_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x2_rx_l_3_sel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x2_rx_l_3_sel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x3_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x3_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x3_rx_l_4_sel_get
* 
* Which physical lane is used for logical lane 4 for Rx
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x3_rx_l_4_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x3_rx_l_4_sel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x3_rx_l_4_sel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x3_rsvd10_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x3_rsvd10_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x3_rx_l_5_sel_get
* 
* Which physical lane is used for logical lane 5 for Rx
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x3_rx_l_5_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x3_rx_l_5_sel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x3_rx_l_5_sel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x3_rsvd11_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x3_rsvd11_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x3_rx_l_6_sel_get
* 
* Which physical lane is used for logical lane 6 for Rx
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x3_rx_l_6_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x3_rx_l_6_sel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x3_rx_l_6_sel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x3_rsvd12_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x3_rsvd12_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x3_rx_l_7_sel_get
* 
* Which physical lane is used for logical lane 7 for Rx
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x3_rx_l_7_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x3_rx_l_7_sel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x3_rx_l_7_sel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0xa_tx_l_reset_slice_get
* 
* TX logic reset for single logical Lane SerDes.
* For eight-lane groups, bits 15:8 control lanes 0 through 7, where bit 8 is lane 0 and bit 15 is lane 7.
* For four-lane groups, bits 11:8 control lanes 0 through 3, whre bit 8 is lane and bit 11 is lane 3.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0xa_tx_l_reset_slice_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xa_tx_l_reset_slice_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xa_tx_l_reset_slice_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0xa_rx_l_reset_slice_get
* 
* RX logic reset for single logical Lane SerDes.
* For eight-lane groups, bits 7:0 control lanes 0 through 7, where bit is lane 0 and bit 7 is lane 7.
* For four-lane groups, bits 0 to control lanes 0 through 3, where bit 0 is lane 0 and bit 3 is lane 3.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0xa_rx_l_reset_slice_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xa_rx_l_reset_slice_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xa_rx_l_reset_slice_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0xb_l_reset_slice_get
* 
* Single physical lane ANLT register reset in each group.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0xb_l_reset_slice_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xb_l_reset_slice_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xb_l_reset_slice_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0xb_l_reset_get
* 
* Single lane register reset in each group.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0xb_l_reset_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xb_l_reset_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xb_l_reset_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0xc_tx_mode0_get
* 
* TX mode of logical lane 0
*     00b:    NRZ
*     01b:    PAM-4
*     10b:    1G
*     11b:    10G
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0xc_tx_mode0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xc_tx_mode0_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xc_tx_mode0_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0xc_tx_mode1_get
* 
* TX mode of logical lane 0
*     00b:    NRZ
*     01b:    PAM-4
*     10b:    1G
*     11b:    10G
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0xc_tx_mode1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xc_tx_mode1_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xc_tx_mode1_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0xc_tx_mode2_get
* 
* TX mode of logical lane 0
*     00b:    NRZ
*     01b:    PAM-4
*     10b:    1G
*     11b:    10G
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0xc_tx_mode2_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xc_tx_mode2_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xc_tx_mode2_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0xc_tx_mode3_get
* 
* TX mode of logical lane 0
*     00b:    NRZ
*     01b:    PAM-4
*     10b:    1G
*     11b:    10G
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0xc_tx_mode3_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xc_tx_mode3_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xc_tx_mode3_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0xc_tx_mode4_get
* 
* TX mode of logical lane 0
*     00b:    NRZ
*     01b:    PAM-4
*     10b:    1G
*     11b:    10G
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0xc_tx_mode4_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xc_tx_mode4_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xc_tx_mode4_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0xc_tx_mode5_get
* 
* TX mode of logical lane 0
*     00b:    NRZ
*     01b:    PAM-4
*     10b:    1G
*     11b:    10G
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0xc_tx_mode5_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xc_tx_mode5_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xc_tx_mode5_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0xc_tx_mode6_get
* 
* TX mode of logical lane 0
*     00b:    NRZ
*     01b:    PAM-4
*     10b:    1G
*     11b:    10G
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0xc_tx_mode6_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xc_tx_mode6_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xc_tx_mode6_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0xc_tx_mode7_get
* 
* TX mode of logical lane 0
*     00b:    NRZ
*     01b:    PAM-4
*     10b:    1G
*     11b:    10G
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0xc_tx_mode7_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xc_tx_mode7_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xc_tx_mode7_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0xd_rx_mode0_get
* 
* Rx mode of logical lane 0:
*     00b:    NRZ
*     01b:    PAM-4
*     10b:    1G
*     11b:    10G
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0xd_rx_mode0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xd_rx_mode0_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xd_rx_mode0_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0xd_rx_mode1_get
* 
* Rx mode of logical lane 0:
*     00b:    NRZ
*     01b:    PAM-4
*     10b:    1G
*     11b:    10G
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0xd_rx_mode1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xd_rx_mode1_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xd_rx_mode1_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0xd_rx_mode2_get
* 
* Rx mode of logical lane:
*     00b:    NRZ
*     01b:    PAM4
*     10b:    1G
*     11b:    10G
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0xd_rx_mode2_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xd_rx_mode2_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xd_rx_mode2_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0xd_rx_mode3_get
* 
* Rx mode of logical lane 0:
*     00b:    NRZ
*     01b:    PAM-4
*     10b:    1G
*     11b:    10G
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0xd_rx_mode3_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xd_rx_mode3_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xd_rx_mode3_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0xd_rx_mode4_get
* 
* Rx mode of logical lane 0:
*     00b:    NRZ
*     01b:    PAM-4
*     10b:    1G
*     11b:    10G
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0xd_rx_mode4_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xd_rx_mode4_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xd_rx_mode4_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0xd_rx_mode5_get
* 
* Rx mode of logical lane 0:
*     00b:    NRZ
*     01b:    PAM-4
*     10b:    1G
*     11b:    10G
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0xd_rx_mode5_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xd_rx_mode5_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xd_rx_mode5_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0xd_rx_mode6_get
* 
* Rx mode of logical lane 0:
*     00b:    NRZ
*     01b:    PAM-4
*     10b:    1G
*     11b:    10G
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0xd_rx_mode6_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xd_rx_mode6_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xd_rx_mode6_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0xd_rx_mode7_get
* 
* Rx mode of logical lane 0:
*     00b:    NRZ
*     01b:    PAM-4
*     10b:    1G
*     11b:    10G
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0xd_rx_mode7_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xd_rx_mode7_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xd_rx_mode7_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x13_cpuc_div_get
* 
* 2'b00=reg_clk is used as CPU's clock
* 2'b01=reg_clk divided by 2 is used as CPU's clock
* 2'b10=reg_clk divided by 4 is used as CPU's clock
* 2'b11=reg_clk divided by 8 is used as CPU's clock
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x13_cpuc_div_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x13_cpuc_div_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x13_cpuc_div_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x13_regc_div_get
* 
* 2'b00=original reg_clk is used as reg_clk,
* 2'b01=original reg_clk is divided by 2 as reg_clk
* 2'b10=original reg_clk is divided by 4 as reg_clk
* 2'b11=original reg_clk is divided by 8 as reg_clk
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x13_regc_div_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x13_regc_div_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x13_regc_div_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x13_sw_rstb_magic_get
* 
* 12'h777=logic reset (that is, FIFO, PHY)
* 12'h888=all reset (that is, FIFO, PHY, register map, CPU)
* 12'h999=register map reset
* 12'haaa=cpu reset
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x13_sw_rstb_magic_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x13_sw_rstb_magic_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x13_sw_rstb_magic_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x2f_sram_bist_en_get
* 
* SRAM BIST select: 1'b1 select SRAM BIST mode
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x2f_sram_bist_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x2f_sram_bist_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x2f_sram_bist_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x2f_sram_test_en_get
* 
* MCU SRAM BIST enable: 1'b1 STAM BIST start
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x2f_sram_test_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x2f_sram_test_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x2f_sram_test_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x2f_rom_bist_en_get
* 
* ROM BIST select: 1'b1 select ROM BIST mode
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x2f_rom_bist_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x2f_rom_bist_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x2f_rom_bist_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x2f_rom_test_en_get
* 
* MCU ROM BIST enable: 1'b1 ROM BIST start
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x2f_rom_test_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x2f_rom_test_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x2f_rom_test_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x2f_rsvd13_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x2f_rsvd13_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x30_sram_bist_done_get
* 
* MCU SRAM BIST done status: 1'b1 SRAM BIST finish
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x30_sram_bist_done_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x30_sram_bist_pass_get
* 
* MCU SRAM BIST pass status: 1'b1 SRAM BIST success, 1'b0 SRAM BIST fail
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x30_sram_bist_pass_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x30_rom_test_done_get
* 
* MCU ROM BIST done status: 1'b1 ROM BIST finish
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x30_rom_test_done_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x30_rsvd14_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x30_rsvd14_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x31_rom_test_result_31_16__get
* 
* ROM BIST check sum value.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x31_rom_test_result_31_16__get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x32_rom_test_result_15_0__get
* 
* ROM BIST check sum value.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x32_rom_test_result_15_0__get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x40_feedback_read_0_get
* 
* feedback_read_0[15:0]
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x40_feedback_read_0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x41_feedback_read_1_get
* 
* feedback_read_1[15:0]
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x41_feedback_read_1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x42_feedback_read_2_get
* 
* feedback_read_2[15:0]
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x42_feedback_read_2_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x43_feedback_read_3_get
* 
* feedback_read_3[15:0]
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x43_feedback_read_3_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x44_feedback_read_4_get
* 
* feedback_read_4[15:0]
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x44_feedback_read_4_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x45_feedback_read_5_get
* 
* feedback_read_5[15:0]
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x45_feedback_read_5_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x46_feedback_read_6_get
* 
* feedback_read_6[15:0]
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x46_feedback_read_6_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x47_feedback_read_7_get
* 
* feedback_read_7[15:0]
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x47_feedback_read_7_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x48_feedback_read_8_get
* 
* feedback_read_8[15:0]
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x48_feedback_read_8_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x49_feedback_read_9_get
* 
* feedback_read_9[15:0]
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x49_feedback_read_9_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x4a_feedback_read_10_get
* 
* feedback_read_10[15:0]
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x4a_feedback_read_10_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x4b_feedback_read_11_get
* 
* feedback_read_11[15:0]
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x4b_feedback_read_11_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x4c_feedback_read_12_get
* 
* feedback_read_12[15:0]
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x4c_feedback_read_12_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x4d_feedback_read_13_get
* 
* feedback_read_13[15:0]
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x4d_feedback_read_13_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x4e_feedback_read_14_get
* 
* feedback_read_14[15:0]
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x4e_feedback_read_14_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x4f_feedback_read_15_get
* 
* feedback_read_15[15:0]
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x4f_feedback_read_15_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x50_int_0_re_en_get
* 
* Interrupt enable for rise edge of the signal set 0.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x50_int_0_re_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x50_int_0_re_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x50_int_0_re_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x51_int_1_re_en_get
* 
* Interrupt enable for rise edge of the signal set 1.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x51_int_1_re_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x51_int_1_re_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x51_int_1_re_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x52_int_2_re_en_get
* 
* Interrupt enable for rise edge of the signal set 2.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x52_int_2_re_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x52_int_2_re_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x52_int_2_re_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x53_int_3_re_en_get
* 
* Interrupt enable for rise edge of the signal set 3.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x53_int_3_re_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x53_int_3_re_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x53_int_3_re_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x54_int_4_re_en_get
* 
* Interrupt enable for rise edge of the signal set 4.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x54_int_4_re_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x54_int_4_re_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x54_int_4_re_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x55_int_5_re_en_get
* 
* Interrupt enable for rise edge of the signal set 5.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x55_int_5_re_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x55_int_5_re_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x55_int_5_re_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x56_int_6_re_en_get
* 
* Interrupt enable for rise edge of the signal set 6.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x56_int_6_re_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x56_int_6_re_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x56_int_6_re_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x57_int_7_re_en_get
* 
* Interrupt enable for rise edge of the signal set 7.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x57_int_7_re_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x57_int_7_re_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x57_int_7_re_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x58_int_0_fe_en_get
* 
* Interrupt enable for fall edge of the signal set 0.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x58_int_0_fe_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x58_int_0_fe_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x58_int_0_fe_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x59_int_1_fe_en_get
* 
* Interrupt enable for fall edge of the signal set 1.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x59_int_1_fe_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x59_int_1_fe_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x59_int_1_fe_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x5a_int_2_fe_en_get
* 
* Interrupt enable for fall edge of the signal set 2.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x5a_int_2_fe_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x5a_int_2_fe_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x5a_int_2_fe_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x5b_int_3_fe_en_get
* 
* Interrupt enable for fall edge of the signal set 3
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x5b_int_3_fe_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x5b_int_3_fe_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x5b_int_3_fe_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x5c_int_4_fe_en_get
* 
* Interrupt enable for fall edge of the signal set 4
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x5c_int_4_fe_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x5c_int_4_fe_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x5c_int_4_fe_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x5d_int_5_fe_en_get
* 
* Interrupt enable for fall edge of the signal set 5
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x5d_int_5_fe_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x5d_int_5_fe_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x5d_int_5_fe_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x5e_int_6_fe_en_get
* 
* Interrupt enable for fall edge of the signal set 6
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x5e_int_6_fe_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x5e_int_6_fe_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x5e_int_6_fe_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x5f_int_7_fe_en_get
* 
* Interrupt enable for fall edge of the signal set 7
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x5f_int_7_fe_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x5f_int_7_fe_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x5f_int_7_fe_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x60_status_0_get
* 
* Raw status bits set 0.
*     Bit 7:0: L_SIG_DETEST[7:0]
*     Bit 15:8: L_R_PHY_READY[7:0]
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x60_status_0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x61_status_1_get
* 
* Raw status bits set 1.
*     Bit 7:0: aa_hcd_resolved[7:0]
*     Bit 15:8: link_training_done[7:0]
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x61_status_1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x62_status_2_get
* 
* Raw status bits set 2.
*     Bit 0: ecc_err
*     Bit 15:1: Reserved
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x62_status_2_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x63_status_3_get
* 
* Raw status bits set 3. Reserved.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x63_status_3_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x64_status_4_get
* 
* Raw status bits set 4. Reserved.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x64_status_4_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x65_status_5_get
* 
* Raw status bits set 5. Reserved.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x65_status_5_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x66_status_6_get
* 
* Software Interrupt status from register 72[15:0]
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x66_status_6_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x67_status_7_get
* 
* REG_OUT[15:0]
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x67_status_7_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x68_int_status_0_get
* 
* Interrupt Event set 0: enabled rise/fall edge event of raw status
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x68_int_status_0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x69_int_status_1_get
* 
* Interrupt Event set 1: enabled rise/fall edge event of raw status
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x69_int_status_1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x6a_int_status_2_get
* 
* Interrupt Event set 2: enabled rise/fall edge event of raw status
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x6a_int_status_2_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x6b_int_status_3_get
* 
* Interrupt Event set 3: enabled rise/fall edge event of raw status
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x6b_int_status_3_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x6c_int_status_4_get
* 
* Interrupt Event set 4: enabled rise/fall edge event of raw status
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x6c_int_status_4_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x6d_int_status_5_get
* 
* Interrupt Event set 5: enabled rise/fall edge event of raw status
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x6d_int_status_5_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x6e_int_status_6_get
* 
* Interrupt Event set 6: enabled rise/fall edge event of raw status
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x6e_int_status_6_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x6f_int_status_7_get
* 
* Interrupt Event set 7: enabled rise/fall edge event of raw status
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x6f_int_status_7_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x70_int_summary_get
* 
* Interrupt Summary
* [7:0]: which lane or lanes have an interrupt event or events.
* [15:8]: which status set or sets have an interrupt event or events
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x70_int_summary_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x71_int_ctrl_get
* 
* [15:4]: Reserved
* [3]: interrupt polarity, 0=active high, 1=active low
* [2]: interrupt status internal reset
* [1]: force interrupt low
* [0]: force interrupt high, has higher priority over bit [1]
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x71_int_ctrl_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x71_int_ctrl_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x71_int_ctrl_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x72_sw_int_get
* 
* Software Interrupts[15:0]
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x72_sw_int_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x72_sw_int_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x72_sw_int_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x73_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x73_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x73_interrupt_get
* 
* Final Interrupt Status
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x73_interrupt_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x8b_rx_addrpair_0_en_get
* 
* SERDES RX register range 0 enable 
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x8b_rx_addrpair_0_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x8b_rx_addrpair_0_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x8b_rx_addrpair_0_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x8b_rx_addrpair_1_en_get
* 
* SERDES RX register range 1 enable 
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x8b_rx_addrpair_1_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x8b_rx_addrpair_1_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x8b_rx_addrpair_1_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x8b_rx_addrpair_2_en_get
* 
* SERDES RX register range 2 enable 
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x8b_rx_addrpair_2_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x8b_rx_addrpair_2_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x8b_rx_addrpair_2_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x8b_rx_addrpair_3_en_get
* 
* SERDES RX register range 3 enable 
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x8b_rx_addrpair_3_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x8b_rx_addrpair_3_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x8b_rx_addrpair_3_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0x8b_rsvd15_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x8b_rsvd15_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_group8_0x8b_rx_addr_start0_get
* 
* SERDES Rx register range 0 start
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0x8b_rx_addr_start0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x8b_rx_addr_start0_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0x8b_rx_addr_start0_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0xc0_firmware_0_get
* 
* Firmware general access register
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0xc0_firmware_0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xc0_firmware_0_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xc0_firmware_0_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0xc1_firmware_1_get
* 
* Firmware general access register
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0xc1_firmware_1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xc1_firmware_1_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xc1_firmware_1_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0xc2_firmware_2_get
* 
* Firmware general access register
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0xc2_firmware_2_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xc2_firmware_2_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xc2_firmware_2_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0xc3_firmware_3_get
* 
* Firmware general access register
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0xc3_firmware_3_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xc3_firmware_3_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xc3_firmware_3_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0xc4_firmware_4_get
* 
* Firmware general access register
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0xc4_firmware_4_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xc4_firmware_4_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xc4_firmware_4_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0xc5_firmware_5_get
* 
* Firmware general access register
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0xc5_firmware_5_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xc5_firmware_5_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xc5_firmware_5_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0xc8_firmware_8_get
* 
* Firmware general access register
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0xc8_firmware_8_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xc8_firmware_8_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xc8_firmware_8_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0xc9_firmware_9_get
* 
* Firmware general access register
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0xc9_firmware_9_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xc9_firmware_9_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xc9_firmware_9_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0xca_firmware_10_get
* 
* Firmware general access register
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0xca_firmware_10_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xca_firmware_10_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xca_firmware_10_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0xcb_firmware_11_get
* 
* Firmware general access register
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0xcb_firmware_11_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xcb_firmware_11_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xcb_firmware_11_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0xcc_firmware_12_get
* 
* Firmware general access register
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0xcc_firmware_12_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xcc_firmware_12_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xcc_firmware_12_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0xcd_firmware_13_get
* 
* Firmware general access register
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0xcd_firmware_13_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xcd_firmware_13_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xcd_firmware_13_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0xce_firmware_14_get
* 
* Firmware general access register
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0xce_firmware_14_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xce_firmware_14_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xce_firmware_14_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0xcf_firmware_15_get
* 
* Firmware general access register
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0xcf_firmware_15_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xcf_firmware_15_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xcf_firmware_15_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_group8_0xd0_cpu_out_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_group8_0xd0_cpu_out_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xd0_cpu_out_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_group8_0xd0_cpu_out_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_dfx_odd_0x0_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_odd_0x0_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_dfx_odd_0x0_prbs_ethx_rd_result_33_32__get
* 
* PRBS checker result. Connection failed if bit[x] = 1'b1.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_odd_0x0_prbs_ethx_rd_result_33_32__get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_dfx_odd_0x1_prbs_ethx_rd_result_31_16_get
* 
* PRBS checker result. Connection failed if bit[x] = 1'b1
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_odd_0x1_prbs_ethx_rd_result_31_16_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_dfx_odd_0x2_prbs_ethx_rd_result_15_0__get
* 
* PRBS checker result. Connection failed if bit[x] = 1'b1
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_odd_0x2_prbs_ethx_rd_result_15_0__get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_dfx_odd_0x3_ethx_err_cycle_31_16_get
* 
* Cycle number of last error. For debug use only.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_odd_0x3_ethx_err_cycle_31_16_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_dfx_odd_0x4_ethx_err_cycle_15_0_get
* 
* Cycle number of last error. For debug use only.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_odd_0x4_ethx_err_cycle_15_0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_dfx_odd_0x5_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_odd_0x5_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_dfx_odd_0x5_tx_prbs_ethx_state_get
* 
* STM state in PRBS generator. For debug use only 
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_odd_0x5_tx_prbs_ethx_state_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_dfx_odd_0x5_rx_prbs_ethx_state_get
* 
* STM state in PRBS checker. Debug use only 
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_odd_0x5_rx_prbs_ethx_state_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_dfx_odd_0x6_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_odd_0x6_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_dfx_odd_0x6_tx_to_rx_lpbk_get
* 
* Control of TX-to-RX data loopback.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_odd_0x6_tx_to_rx_lpbk_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_odd_0x6_tx_to_rx_lpbk_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_odd_0x6_tx_to_rx_lpbk_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_dfx_even_0x0_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_even_0x0_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_dfx_even_0x0_glob_tx_to_rx_lpbk_get
* 
* Global control of TX-to-RX data loopback.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_even_0x0_glob_tx_to_rx_lpbk_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0x0_glob_tx_to_rx_lpbk_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0x0_glob_tx_to_rx_lpbk_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_dfx_even_0x0_rsvd16_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_even_0x0_rsvd16_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_dfx_even_0x1_loc_repairable_status_clr_get
* 
* Local repairable test status clear.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_even_0x1_loc_repairable_status_clr_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0x1_loc_repairable_status_clr_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0x1_loc_repairable_status_clr_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_dfx_even_0x1_rsvd17_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_even_0x1_rsvd17_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_dfx_even_0x2_repairable_test_done_get
* 
* 34 bit PRBS test done signal.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_even_0x2_repairable_test_done_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_dfx_even_0x2_rsvd18_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_even_0x2_rsvd18_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_dfx_even_0x2_upper_repairable_status_get
* 
* Bit [33:17] repairable status bit.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_even_0x2_upper_repairable_status_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_dfx_even_0x2_upper_repair_code_get
* 
* Bit [33:17] repair bits encode status.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_even_0x2_upper_repair_code_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_dfx_even_0x2_lower_repairable_status_get
* 
* Bit [16: 0] repairable status bit.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_even_0x2_lower_repairable_status_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_dfx_even_0x2_lower_repair_code_get
* 
* Bit [16:0] repair bits encode status.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_even_0x2_lower_repair_code_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_dfx_even_0x6_ethx_rxclk_ph_mask_get
* 
* The eth(x)_txclk phase mask.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_even_0x6_ethx_rxclk_ph_mask_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0x6_ethx_rxclk_ph_mask_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0x6_ethx_rxclk_ph_mask_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_dfx_even_0x6_rsvd19_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_even_0x6_rsvd19_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_dfx_even_0x6_ethx_rxclk_ph_get
* 
* The eth(x)_rxclk phase shift configuration.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_even_0x6_ethx_rxclk_ph_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0x6_ethx_rxclk_ph_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0x6_ethx_rxclk_ph_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_dfx_even_0x7_prbs_ethx_mode_get
* 
* PRBS mode select:
*     1:    PRBS 32-bits mode
*     0:    PRBS 34-bits mode
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_even_0x7_prbs_ethx_mode_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0x7_prbs_ethx_mode_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0x7_prbs_ethx_mode_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_dfx_even_0x7_rsvd20_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_even_0x7_rsvd20_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_dfx_even_0x7_prbs_ethx_force0_get
* 
* Force all 0 on tx_data when this field is 1'b1.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_even_0x7_prbs_ethx_force0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0x7_prbs_ethx_force0_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0x7_prbs_ethx_force0_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_dfx_even_0x7_prbs_ethx_gen_en_get
* 
* PRBS generator clock gate. Disable when this field is 1'b0
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_even_0x7_prbs_ethx_gen_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0x7_prbs_ethx_gen_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0x7_prbs_ethx_gen_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_dfx_even_0x7_prbs_ethx_en_get
* 
* PRBS generator turn on when this field is 1'b1.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_even_0x7_prbs_ethx_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0x7_prbs_ethx_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0x7_prbs_ethx_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_dfx_even_0x7_prbs_ethx_gen_mode_get
* 
* PRBS generator mode select:
*     00:    prbs9
*     01:    prbs15
*     10:    prbs23
*     11:    prbs31 
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_even_0x7_prbs_ethx_gen_mode_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0x7_prbs_ethx_gen_mode_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0x7_prbs_ethx_gen_mode_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_dfx_even_0x7_rsvd21_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_even_0x7_rsvd21_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_dfx_even_0x7_prbs_ethx_err_gen_33_32_get
* 
* PRBS error generator. When bit(x) = 1'b1, this will generate one error in tx_data[x]. For debug use only.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_even_0x7_prbs_ethx_err_gen_33_32_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0x7_prbs_ethx_err_gen_33_32_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0x7_prbs_ethx_err_gen_33_32_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_dfx_even_0x8_prbs_ethx_err_gen_31_16_get
* 
* PRBS error generator. When bit(x) = 1'b1, this will generate one error in tx_data[x]. For debug use only.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_even_0x8_prbs_ethx_err_gen_31_16_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0x8_prbs_ethx_err_gen_31_16_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0x8_prbs_ethx_err_gen_31_16_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_dfx_even_0x9_prbs_ethx_err_gen_15_0_get
* 
* PRBS error generator. When bit(x) = 1'b1, this will generate one error in tx_data[x]. For debug use only.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_even_0x9_prbs_ethx_err_gen_15_0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0x9_prbs_ethx_err_gen_15_0_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0x9_prbs_ethx_err_gen_15_0_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_dfx_even_0xa_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_even_0xa_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_dfx_even_0xa_ethx_gen_seed_30_16_get
* 
* PRBS generator seed, use bit[30, 31-x] for PRBSx.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_even_0xa_ethx_gen_seed_30_16_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0xa_ethx_gen_seed_30_16_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0xa_ethx_gen_seed_30_16_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_dfx_even_0xb_ethx_gen_seed_15_0_get
* 
* PRBS generator seed, use bit[30, 31-x] for PRBSx.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_even_0xb_ethx_gen_seed_15_0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0xb_ethx_gen_seed_15_0_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0xb_ethx_gen_seed_15_0_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_dfx_even_0xc_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_even_0xc_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_dfx_even_0xc_prbs_ethx_chk_en_get
* 
* PRBS checker clock gate. Disabled when this field is 1'b0.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_even_0xc_prbs_ethx_chk_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0xc_prbs_ethx_chk_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0xc_prbs_ethx_chk_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_dfx_even_0xc_prbs_ethx_chk_mode_get
* 
* PRBS checker mode select:
*     00b:    prbs9
*     01b:    prbs15
*     10b:    prbs23
*     11b:    prbs31 
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_even_0xc_prbs_ethx_chk_mode_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0xc_prbs_ethx_chk_mode_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0xc_prbs_ethx_chk_mode_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_dfx_even_0xc_prbs_ethx_cntr_reset_get
* 
* PRBS checker counter reset signal: 
*     1:    Keep resetting counter to 0
*     0:    Normal accumulation
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_even_0xc_prbs_ethx_cntr_reset_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0xc_prbs_ethx_cntr_reset_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0xc_prbs_ethx_cntr_reset_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_dfx_even_0xc_rx_prbs_ethx_en_get
* 
* PRBS checker is turned on when this field is 1'b1.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_even_0xc_rx_prbs_ethx_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0xc_rx_prbs_ethx_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0xc_rx_prbs_ethx_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_dfx_even_0xc_rsvd22_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_even_0xc_rsvd22_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_dfx_even_0xc_ethx_err_th_get
* 
* Threshold for PRBS checker error detection.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_even_0xc_ethx_err_th_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0xc_ethx_err_th_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0xc_ethx_err_th_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_dfx_even_0xd_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_even_0xd_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_dfx_even_0xd_ethx_chk_seed_30_16_get
* 
* PRBS checker seed. Use bit[30, 31-x] for PRBSx.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_even_0xd_ethx_chk_seed_30_16_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0xd_ethx_chk_seed_30_16_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0xd_ethx_chk_seed_30_16_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_dfx_even_0xe_ethx_chk_seed_15_0_get
* 
* PRBS checker seed. Use bit[30, 31-x] for PRBSx.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_even_0xe_ethx_chk_seed_15_0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0xe_ethx_chk_seed_15_0_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0xe_ethx_chk_seed_15_0_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_dfx_even_0xf_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_even_0xf_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_dfx_even_0xf_prbs_ethx_start_th_get
* 
* Threshold for PRBS checker start signal detection.
*********************************************************************/
bf_status_t bf_ll_serdes_dfx_even_0xf_prbs_ethx_start_th_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0xf_prbs_ethx_start_th_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_dfx_even_0xf_prbs_ethx_start_th_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_top_pll_0x4_rd_wait_get
* 
* For the MAC register interface, the number of wait cycles before DUT responds to read transaction on MDO.
*********************************************************************/
bf_status_t bf_ll_serdes_top_pll_0x4_rd_wait_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_top_pll_0x4_rd_wait_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_top_pll_0x4_rd_wait_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_top_pll_0x4_wr_wait_get
* 
* For the MAC register interface, the number of wait cycles before DUT responds to write transaction on MDO.
*********************************************************************/
bf_status_t bf_ll_serdes_top_pll_0x4_wr_wait_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_top_pll_0x4_wr_wait_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_top_pll_0x4_wr_wait_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_top_pll_0x5_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_top_pll_0x5_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_top_pll_0x5_rst_cnt_get
* 
* For the MAC register interface, the number of wait cycles before DUT responds to reset transaction on MDO.
*********************************************************************/
bf_status_t bf_ll_serdes_top_pll_0x5_rst_cnt_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_top_pll_0x5_rst_cnt_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_top_pll_0x5_rst_cnt_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_top_pll_0x13_cpuc_div_get
* 
* 00b: reg_clk is used as CPU's clock
* 01b: reg_clk divided by 2 is used as CPU's clock
* 10b: reg_clk divided by 4 is used as CPU's clock
* 11b: reg_clk divided by 8 is used as CPU's clock
*********************************************************************/
bf_status_t bf_ll_serdes_top_pll_0x13_cpuc_div_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_top_pll_0x13_cpuc_div_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_top_pll_0x13_cpuc_div_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_top_pll_0x13_regc_div_get
* 
* 00b: Original reg_clk is used as reg_clk
* 01b: Original reg_clk is divided by 2 as reg_clk
* 10b: Original reg_clk is divided by 4 as reg_clk
* 11b: Original reg_clk is divided by 8 as reg_clk
*********************************************************************/
bf_status_t bf_ll_serdes_top_pll_0x13_regc_div_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_top_pll_0x13_regc_div_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_top_pll_0x13_regc_div_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_top_pll_0x13_sw_rstb_magic_get
* 
* 777h: Logic reset (that is, FIFO, PHY)
* 888h: All reset (that is, FIFO, PHY, register map, CPU)
* 999h: Register map reset
* AAAh: CPU reset
*********************************************************************/
bf_status_t bf_ll_serdes_top_pll_0x13_sw_rstb_magic_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_top_pll_0x13_sw_rstb_magic_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_top_pll_0x13_sw_rstb_magic_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_top_pll_0x15_repairable_status_s_get
* 
* 1: Indicate all macro bump lanes are repairable.
* 0: At least one lane's macro bump is not repairable
*********************************************************************/
bf_status_t bf_ll_serdes_top_pll_0x15_repairable_status_s_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_top_pll_0x15_repairable_test_done_s_get
* 
* 1: Indicate all macro bump lanes tests are done.
* 0: At least one lane's macro bump test is not done.
*********************************************************************/
bf_status_t bf_ll_serdes_top_pll_0x15_repairable_test_done_s_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_top_pll_0x15_rsvd23_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_top_pll_0x15_rsvd23_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_top_pll_0x16_repairable_status_clr_s_get
* 
* Clear repairable status (repairable_status_s, repairable_test_done_s)
*********************************************************************/
bf_status_t bf_ll_serdes_top_pll_0x16_repairable_status_clr_s_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_top_pll_0x16_repairable_status_clr_s_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_top_pll_0x16_repairable_status_clr_s_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_top_pll_0x16_rsvd24_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_top_pll_0x16_rsvd24_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_top_pll_0x50_sensor_0_get
* 
* Voltage sensor 0 result:
*     15:13    Always 000b
*     12    1 = Voltage sensor ready
*         0 = Voltage sensor not ready
*     11:0     Voltage sensor result
*********************************************************************/
bf_status_t bf_ll_serdes_top_pll_0x50_sensor_0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_top_pll_0x51_sensor_1_get
* 
* Voltage sensor 1 result:
*     15:13    Always 000b
*     12    1 = Voltage sensor ready        0 = Voltage sensor not ready
*     11:0    Voltage sensor result
*********************************************************************/
bf_status_t bf_ll_serdes_top_pll_0x51_sensor_1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_top_pll_0x52_sensor_2_get
* 
* Voltage sensor 2 result:
*     15:13    Always 3'b000,
*     12    1 = Voltage sensor ready
*         0 = Voltage sensor not ready
* 11:0         Voltage sensor result
*********************************************************************/
bf_status_t bf_ll_serdes_top_pll_0x52_sensor_2_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_top_pll_0x53_sensor_3_get
* 
* Voltage sensor 3 result:
*     15:13    Always 000b
*     12    1 = Voltage sensor ready        0 = Voltage sensor not ready
*     11:0    Voltage sensor result
*********************************************************************/
bf_status_t bf_ll_serdes_top_pll_0x53_sensor_3_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_top_pll_0x54_sensor_4_get
* 
* Voltage sensor 4 result:
*     15:13    Always 000b
*     12    1 = Voltage sensor ready        0 = Voltage sensor not ready
*     11:0    Voltage sensor result
*********************************************************************/
bf_status_t bf_ll_serdes_top_pll_0x54_sensor_4_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_top_pll_0x55_sensor_5_get
* 
* Voltage sensor 5 result:
*     15:13    Always 000b
*     12    1 = Voltage sensor ready        0 = Voltage sensor not ready
*     11:0    Voltage sensor result
*********************************************************************/
bf_status_t bf_ll_serdes_top_pll_0x55_sensor_5_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_top_pll_0x56_sensor_6_get
* 
* Voltage sensor 6 result:
*     15:13    Always 000b
*     12    1 = Voltage sensor ready        0 = Voltage sensor not ready
*     11:0    Voltage sensor result
*********************************************************************/
bf_status_t bf_ll_serdes_top_pll_0x56_sensor_6_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_top_pll_0x57_sensor_7_get
* 
* Voltage sensor 7 result:
*     15:13    Always 000b
*     12    1 = Voltage sensor ready        0 = Voltage sensor not ready
*     11:0    Voltage sensor result
*********************************************************************/
bf_status_t bf_ll_serdes_top_pll_0x57_sensor_7_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_top_pll_0x58_sensor_8_get
* 
* Voltage sensor 8 result:
*     15:13    Always 000b
*     12    1 = Voltage sensor ready        0 = Voltage sensor not ready
*     11:0    Voltage sensor result
*********************************************************************/
bf_status_t bf_ll_serdes_top_pll_0x58_sensor_8_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_top_pll_0x59_sensor_9_get
* 
* Temperature sensor result:
*     15:13    Always 000b
*     12    1 = Sensor ready        0 = Sensor not ready
*     11:0    Sensor result
*********************************************************************/
bf_status_t bf_ll_serdes_top_pll_0x59_sensor_9_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_sensor_0x37_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_sensor_0x37_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_sensor_0x37_tsensor_run_get
* 
* Enable temperature sensor.
*********************************************************************/
bf_status_t bf_ll_serdes_sensor_0x37_tsensor_run_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_sensor_0x37_tsensor_run_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_sensor_0x37_tsensor_run_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_sensor_0x37_tsensor_rstn_get
* 
* Temperature sensor reset. 0h = Reset.
*********************************************************************/
bf_status_t bf_ll_serdes_sensor_0x37_tsensor_rstn_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_sensor_0x37_tsensor_rstn_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_sensor_0x37_tsensor_rstn_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_sensor_0x37_rsvd25_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_sensor_0x37_rsvd25_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_sensor_0x38_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_sensor_0x38_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_sensor_0x38_tsensor_ready_get
* 
* Temperature sensor data is ready.
*********************************************************************/
bf_status_t bf_ll_serdes_sensor_0x38_tsensor_ready_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_sensor_0x38_tsensor_digo_get
* 
* Multiplexed digital test output.
*********************************************************************/
bf_status_t bf_ll_serdes_sensor_0x38_tsensor_digo_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_sensor_0x38_rsvd26_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_sensor_0x38_rsvd26_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_sensor_0x39_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_sensor_0x39_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_sensor_0x39_tsensor_dout1_get
* 
* Temperature sensor data.
*********************************************************************/
bf_status_t bf_ll_serdes_sensor_0x39_tsensor_dout1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_sensor_0x3a_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_sensor_0x3a_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_sensor_0x3a_auto_ts_get
* 
* Enable temperature sensor auto-read.
*********************************************************************/
bf_status_t bf_ll_serdes_sensor_0x3a_auto_ts_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_sensor_0x3a_auto_ts_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_sensor_0x3a_auto_ts_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_sensor_0x3a_auto_vs_get
* 
* Enable voltage sensor auto read.
*********************************************************************/
bf_status_t bf_ll_serdes_sensor_0x3a_auto_vs_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_sensor_0x3a_auto_vs_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_sensor_0x3a_auto_vs_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_sensor_0x3a_auto_vs1_get
* 
* Enable voltage sensor1 auto read.
*********************************************************************/
bf_status_t bf_ll_serdes_sensor_0x3a_auto_vs1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_sensor_0x3a_auto_vs1_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_sensor_0x3a_auto_vs1_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_sensor_0x3a_ts_rstb_get
* 
* Reset temperature sensor
*********************************************************************/
bf_status_t bf_ll_serdes_sensor_0x3a_ts_rstb_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_sensor_0x3a_ts_rstb_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_sensor_0x3a_ts_rstb_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_sensor_0x3a_vs_rtsb_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_sensor_0x3a_vs_rtsb_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_sensor_0x3a_vs_rtsb_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_sensor_0x3a_vs_rtsb_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_sensor_0x3a_vs1_rstb_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_sensor_0x3a_vs1_rstb_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_sensor_0x3a_vs1_rstb_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_sensor_0x3a_vs1_rstb_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_sensor_0x3e_tsensor_clk_sel_get
* 
* Source selection for the temperature sensor clock.
*********************************************************************/
bf_status_t bf_ll_serdes_sensor_0x3e_tsensor_clk_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_sensor_0x3e_tsensor_clk_sel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_sensor_0x3e_tsensor_clk_sel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_sensor_0x3e_tsensor_clk_cntr_get
* 
* Temperature sensor counter.
*********************************************************************/
bf_status_t bf_ll_serdes_sensor_0x3e_tsensor_clk_cntr_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_sensor_0x3e_tsensor_clk_cntr_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_sensor_0x3e_tsensor_clk_cntr_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_sensor_0x3f_vsensor_clk_sel_get
* 
* Source selection for the voltage sensor clock.
*********************************************************************/
bf_status_t bf_ll_serdes_sensor_0x3f_vsensor_clk_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_sensor_0x3f_vsensor_clk_sel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_sensor_0x3f_vsensor_clk_sel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_sensor_0x3f_vsensor_clk_cntr_get
* 
* Voltage sensor counter.
*********************************************************************/
bf_status_t bf_ll_serdes_sensor_0x3f_vsensor_clk_cntr_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_sensor_0x3f_vsensor_clk_cntr_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_sensor_0x3f_vsensor_clk_cntr_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x20_gpio0_def2_get
* 
* GPIO input/output mode setting
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x20_gpio0_def2_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x20_gpio0_def2_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x20_gpio0_def2_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x20_gpio0_def1_get
* 
* GPIO input/output mode setting
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x20_gpio0_def1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x20_gpio0_def1_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x20_gpio0_def1_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x20_gpio0_def0_get
* 
* GPIO input/output mode setting
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x20_gpio0_def0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x20_gpio0_def0_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x20_gpio0_def0_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x20_gpio1_def2_get
* 
* GPIO input/output mode setting
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x20_gpio1_def2_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x20_gpio1_def2_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x20_gpio1_def2_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x20_gpio1_def1_get
* 
* GPIO input/output mode setting
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x20_gpio1_def1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x20_gpio1_def1_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x20_gpio1_def1_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x20_gpio1_def0_get
* 
* GPIO input/output mode setting
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x20_gpio1_def0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x20_gpio1_def0_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x20_gpio1_def0_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x20_gpio2_def2_get
* 
* GPIO input/output mode setting
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x20_gpio2_def2_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x20_gpio2_def2_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x20_gpio2_def2_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x20_gpio2_def1_get
* 
* GPIO input/output mode setting
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x20_gpio2_def1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x20_gpio2_def1_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x20_gpio2_def1_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x20_gpio2_def0_get
* 
* GPIO input/output mode setting
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x20_gpio2_def0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x20_gpio2_def0_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x20_gpio2_def0_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x20_gpio3_def2_get
* 
* GPIO input/output mode setting
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x20_gpio3_def2_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x20_gpio3_def2_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x20_gpio3_def2_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x20_gpio3_def1_get
* 
* GPIO input/output mode setting
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x20_gpio3_def1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x20_gpio3_def1_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x20_gpio3_def1_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x20_gpio3_def0_get
* 
* GPIO input/output mode setting
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x20_gpio3_def0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x20_gpio3_def0_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x20_gpio3_def0_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x20_rsvd27_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x20_rsvd27_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rsvd_0x24_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x24_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rsvd_0x24_pu3_gpio_get
* 
* GPIO pull up.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x24_pu3_gpio_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x24_pu3_gpio_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x24_pu3_gpio_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x24_pu2_gpio_get
* 
* GPIO pull up.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x24_pu2_gpio_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x24_pu2_gpio_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x24_pu2_gpio_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x24_pu1_gpio_get
* 
* GPIO pull up.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x24_pu1_gpio_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x24_pu1_gpio_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x24_pu1_gpio_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x24_pu0_gpio_get
* 
* GPIO pull up.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x24_pu0_gpio_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x24_pu0_gpio_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x24_pu0_gpio_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x25_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x25_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rsvd_0x25_pd3_gpio_get
* 
* GPIO pull down.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x25_pd3_gpio_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x25_pd3_gpio_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x25_pd3_gpio_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x25_pd2_gpio_get
* 
* GPIO pull down.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x25_pd2_gpio_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x25_pd2_gpio_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x25_pd2_gpio_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x25_pd1_gpio_get
* 
* GPIO pull down.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x25_pd1_gpio_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x25_pd1_gpio_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x25_pd1_gpio_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x25_pd0_gpio_get
* 
* GPIO pull down.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x25_pd0_gpio_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x25_pd0_gpio_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x25_pd0_gpio_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x26_prg_alarm_1_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x26_prg_alarm_1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x26_prg_alarm_1_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x26_prg_alarm_1_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x26_rsvd28_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x26_rsvd28_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rsvd_0x26_gpio_out3_get
* 
* GPIO output.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x26_gpio_out3_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x26_gpio_out3_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x26_gpio_out3_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x26_gpio_out2_get
* 
* GPIO output.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x26_gpio_out2_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x26_gpio_out2_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x26_gpio_out2_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x26_gpio_out1_get
* 
* GPIO output.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x26_gpio_out1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x26_gpio_out1_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x26_gpio_out1_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x26_gpio_out0_get
* 
* GPIO output.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x26_gpio_out0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x26_gpio_out0_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x26_gpio_out0_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x27_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x27_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rsvd_0x27_gpio_sts3_get
* 
* GPIO input.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x27_gpio_sts3_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rsvd_0x27_gpio_sts2_get
* 
* GPIO input.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x27_gpio_sts2_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rsvd_0x27_gpio_sts1_get
* 
* GPIO input.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x27_gpio_sts1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rsvd_0x27_gpio_sts0_get
* 
* GPIO input.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x27_gpio_sts0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rsvd_0x0_pd_cal_fcal_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x0_pd_cal_fcal_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x0_pd_cal_fcal_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x0_pd_cal_fcal_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x0_pu_refclk_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x0_pu_refclk_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x0_pu_refclk_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x0_pu_refclk_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x0_chpmpclk_sel_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x0_chpmpclk_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x0_chpmpclk_sel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x0_chpmpclk_sel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x0_pu_clkdsc_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x0_pu_clkdsc_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x0_pu_clkdsc_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x0_pu_clkdsc_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x0_refclk_src_sel_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x0_refclk_src_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x0_refclk_src_sel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x0_refclk_src_sel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x0_vref1p3clkdsc_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x0_vref1p3clkdsc_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x0_vref1p3clkdsc_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x0_vref1p3clkdsc_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x0_bypass_1p3reg_clkbuf_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x0_bypass_1p3reg_clkbuf_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x0_bypass_1p3reg_clkbuf_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x0_bypass_1p3reg_clkbuf_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x0_vtstgroup_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x0_vtstgroup_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x0_vtstgroup_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x0_vtstgroup_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x0_refclk_out_en_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x0_refclk_out_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x0_refclk_out_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x0_refclk_out_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x1_vref1p3clkbuf_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x1_vref1p3clkbuf_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x1_vref1p3clkbuf_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x1_vref1p3clkbuf_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x1_pu_rvdd_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x1_pu_rvdd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x1_pu_rvdd_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x1_pu_rvdd_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x1_pu_rvddloop_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x1_pu_rvddloop_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x1_pu_rvddloop_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x1_pu_rvddloop_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x1_rvddvco_en_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x1_rvddvco_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x1_rvddvco_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x1_rvddvco_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x1_vrefclkbuf_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x1_vrefclkbuf_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x1_vrefclkbuf_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x1_vrefclkbuf_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x1_testmode_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x1_testmode_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x1_testmode_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x1_testmode_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x1_vbg_c_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x1_vbg_c_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x1_vbg_c_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x1_vbg_c_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x1_toprefclk_r1_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x1_toprefclk_r1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x1_toprefclk_r1_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x1_toprefclk_r1_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x2_vrefvddr2clkdsc_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x2_vrefvddr2clkdsc_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x2_vrefvddr2clkdsc_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x2_vrefvddr2clkdsc_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x2_refclk_div4_en_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x2_refclk_div4_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x2_refclk_div4_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x2_refclk_div4_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x2_vrefclkdsc_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x2_vrefclkdsc_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x2_vrefclkdsc_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x2_vrefclkdsc_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x2_vrvdd_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x2_vrvdd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x2_vrvdd_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x2_vrvdd_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x2_pu_bg_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x2_pu_bg_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x2_pu_bg_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x2_pu_bg_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x2_bypass_1p3reg_clkdsc_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x2_bypass_1p3reg_clkdsc_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x2_bypass_1p3reg_clkdsc_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x2_bypass_1p3reg_clkdsc_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x2_test_en_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x2_test_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x2_test_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x2_test_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x2_toprefclk_r2_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x2_toprefclk_r2_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x2_toprefclk_r2_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x2_toprefclk_r2_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x3_resv_refclk_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x3_resv_refclk_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x3_resv_refclk_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x3_resv_refclk_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fw_0x0_firmware_data_0_get
* 
* Firmware Download
* DOWNLOAD_FRAME_0[15:0] = Firmware binary frame word 0.
* Eye Diagram Data
* EYE_DIAGRAM_DATA_0[15:0] = Eye diagram data frame word 0. In this mode, the register is read-only access.
*********************************************************************/
bf_status_t bf_ll_serdes_fw_0x0_firmware_data_0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fw_0x0_firmware_data_0_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fw_0x0_firmware_data_0_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fw_0x1_firmware_data_1_get
* 
* Firmware Download
* DOWNLOAD_FRAME_1[15:0] = Firmware binary frame word 1.
* Eye Diagram Data
* EYE_DIAGRAM_DATA_1[15:0] = Eye diagram data frame word 1. In this mode, the register is read-only access.
*********************************************************************/
bf_status_t bf_ll_serdes_fw_0x1_firmware_data_1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fw_0x1_firmware_data_1_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fw_0x1_firmware_data_1_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fw_0x2_firmware_data_2_get
* 
* Firmware Download
* DOWNLOAD_FRAME_2[15:0] = Firmware binary frame word 2.
* Eye Diagram Data
* EYE_DIAGRAM_DATA_2[15:0] = Eye diagram data frame word 2. In this mode, the register is read-only access.
*********************************************************************/
bf_status_t bf_ll_serdes_fw_0x2_firmware_data_2_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fw_0x2_firmware_data_2_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fw_0x2_firmware_data_2_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fw_0x3_firmware_data_3_get
* 
* Firmware Download
* DOWNLOAD_FRAME_3[15:0] = Firmware binary frame word 3.
* Eye Diagram Data
* EYE_DIAGRAM_DATA_3[15:0] = Eye diagram data frame word 3. In this mode, the register is read-only access.
*********************************************************************/
bf_status_t bf_ll_serdes_fw_0x3_firmware_data_3_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fw_0x3_firmware_data_3_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fw_0x3_firmware_data_3_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fw_0x4_firmware_data_4_get
* 
* Firmware Download
* DOWNLOAD_FRAME_4[15:0] = Firmware binary frame word 4.
* Eye Diagram Data
* EYE_DIAGRAM_DATA_4[15:0] = Eye diagram data frame word 4. In this mode, the register is read-only access.
*********************************************************************/
bf_status_t bf_ll_serdes_fw_0x4_firmware_data_4_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fw_0x4_firmware_data_4_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fw_0x4_firmware_data_4_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fw_0x5_firmware_data_5_get
* 
* Firmware Download
* DOWNLOAD_FRAME_5[15:0] = Firmware binary frame word 5.
* Eye Diagram Data
* EYE_DIAGRAM_DATA_5[15:0] = Eye diagram data frame word 5. In this mode, the register is read-only access.
*********************************************************************/
bf_status_t bf_ll_serdes_fw_0x5_firmware_data_5_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fw_0x5_firmware_data_5_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fw_0x5_firmware_data_5_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fw_0x6_firmware_data_6_get
* 
* Firmware Download
* DOWNLOAD_FRAME_6[15:0] = Firmware binary frame word 6.
* Eye Diagram Data
* EYE_DIAGRAM_DATA_6[15:0] = Eye diagram data frame word 6. In this mode, the register is read-only access.
*********************************************************************/
bf_status_t bf_ll_serdes_fw_0x6_firmware_data_6_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fw_0x6_firmware_data_6_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fw_0x6_firmware_data_6_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fw_0x7_firmware_data_7_get
* 
* Firmware Download
* DOWNLOAD_FRAME_7[15:0] = Firmware binary frame word 7.
* Eye Diagram Data
* EYE_DIAGRAM_DATA_7[15:0] = Eye diagram data frame word 7. In this mode, the register is read-only access.
*********************************************************************/
bf_status_t bf_ll_serdes_fw_0x7_firmware_data_7_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fw_0x7_firmware_data_7_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fw_0x7_firmware_data_7_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fw_0x8_firmware_data_8_get
* 
* Firmware Download
* DOWNLOAD_FRAME_8[15:0] = Firmware binary frame word 8.
* Eye Diagram Data
* EYE_DIAGRAM_DATA_8[15:0] = Eye diagram data frame word 8. In this mode, the register is read-only access.
*********************************************************************/
bf_status_t bf_ll_serdes_fw_0x8_firmware_data_8_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fw_0x8_firmware_data_8_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fw_0x8_firmware_data_8_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fw_0x9_firmware_data_9_get
* 
* Firmware Download
* DOWNLOAD_FRAME_9[15:0] = Firmware binary frame word 9.
* Eye Diagram Data
* EYE_DIAGRAM_DATA_9[15:0] = Eye diagram data frame word 9. In this mode, the register is read-only access.
*********************************************************************/
bf_status_t bf_ll_serdes_fw_0x9_firmware_data_9_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fw_0x9_firmware_data_9_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fw_0x9_firmware_data_9_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fw_0xa_firmware_data_a_get
* 
* Firmware Download
* DOWNLOAD_FRAME_A[15:0] = Firmware binary frame word A.
* Eye Diagram Data
* EYE_DIAGRAM_DATA_A[15:0] = Eye diagram data frame word A. In this mode, the register is read-only access.
*********************************************************************/
bf_status_t bf_ll_serdes_fw_0xa_firmware_data_a_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fw_0xa_firmware_data_a_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fw_0xa_firmware_data_a_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fw_0xb_firmware_data_b_get
* 
* Firmware Download
* DOWNLOAD_FRAME_B[15:0] = Firmware binary frame word B.
* Eye Diagram Data
* EYE_DIAGRAM_DATA_B[15:0] = Eye diagram data frame word B. In this mode the register is read-only access.
*********************************************************************/
bf_status_t bf_ll_serdes_fw_0xb_firmware_data_b_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fw_0xb_firmware_data_b_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fw_0xb_firmware_data_b_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fw_0xc_firmware_data_c_get
* 
* Firmware Download
* RAM_ADDRESS[31:16] = During firmware download, defines the starting RAM address of where the current frame will be loaded to. After firmware download is complete, defines the starting RAM address for MCU execution. The mode is controlled by settings in the control word.
* Eye Diagram Data
* EYE_DIAGRAM_DATA_12[15:0] = Eye diagram data frame word 12. In this mode, the register is read-only access.
*********************************************************************/
bf_status_t bf_ll_serdes_fw_0xc_firmware_data_c_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fw_0xc_firmware_data_c_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fw_0xc_firmware_data_c_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fw_0xd_firmware_data_d_get
* 
* Firmware Download
* RAM_ADDRESS[15:0] = During firmware download, defines the starting RAM address of where the current frame will be loaded to. After firmware download is complete, defines the starting RAM address for MCU execution. The mode is controlled by settings in the control word.
* Eye Diagram Data
* EYE_DIAGRAM_DATA_13[15:0] = Eye diagram data frame word 13. In this mode, the register is read-only access.
*********************************************************************/
bf_status_t bf_ll_serdes_fw_0xd_firmware_data_d_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fw_0xd_firmware_data_d_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fw_0xd_firmware_data_d_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fw_0xe_firmware_data_e_get
* 
* Firmware Download
* FRAME_CHECKSUM[15:0] = Checksum for the entire 16-word frame.
* Eye Diagram Data
* EYE_DIAGRAM_DATA_14[15:0] = Eye diagram data frame word 14. In this mode, the register is read-only access.
*********************************************************************/
bf_status_t bf_ll_serdes_fw_0xe_firmware_data_e_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fw_0xe_firmware_data_e_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fw_0xe_firmware_data_e_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fw_0xf_firmware_data_f_get
* 
* Firmware Download
* WRITE_FRAME[15] = AHB sets this bit to '1' to initiate a frame write. This bit will be cleared by the MCU after the write is complete.
* START_EXECUTION[14] = AHB sets this bit to '1' to tell MCU to start execution.
* CHECK_SUM_ERR[13] = MCU sets this bit to '1' to indicate a checksum error has occurred. Value is read-only.
* RSVD[12:4] = Reserved for future use. Do not change the default value of 000h.
* FRAME_SIZE[3:0] = AHB sets the number of valid firmware words in the current frame. Max value is 0Ch.
* Eye Diagram Data
* EYE_DIAGRAM_DATA_15[15:0] = Eye diagram data frame word 15. In this mode, the register is read-only access.
*********************************************************************/
bf_status_t bf_ll_serdes_fw_0xf_firmware_data_f_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fw_0xf_firmware_data_f_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fw_0xf_firmware_data_f_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_ecc_0x0_sram_ecc_en_get
* 
* When set, enables SRAM ECC generation and checking
*********************************************************************/
bf_status_t bf_ll_serdes_ecc_0x0_sram_ecc_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_ecc_0x0_sram_ecc_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_ecc_0x0_sram_ecc_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_ecc_0x0_sram_ecc_force_err_get
* 
* For debug, complement data bits to force errors in ECC generation:
*     00: No error
*     01: Complement bit 0 of SRAM input data (after ECC)
*     10: Complement bit 1 of SRAM input data (after ECC)
*     11: Complement bits 0 and 1 of SRAM input data (after ECC)
*********************************************************************/
bf_status_t bf_ll_serdes_ecc_0x0_sram_ecc_force_err_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_ecc_0x0_sram_ecc_force_err_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_ecc_0x0_sram_ecc_force_err_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_ecc_0x0_rsvd29_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_ecc_0x0_rsvd29_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x0_pam4_sm_reset_get
* 
* State machine reset level triggered: 1=reset
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x0_pam4_sm_reset_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x0_pam4_sm_reset_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x0_pam4_sm_reset_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x0_data_thr_init_get
* 
* Data threshold init value in S7.6 format
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x0_data_thr_init_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x0_data_thr_init_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x0_data_thr_init_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x0_dfe_mode_get
* 
* Select if the state machine is for 3 comp or 4 comp
*     1    3 comp mode
*     0    4 comp mode
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x0_dfe_mode_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x0_dfe_mode_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x0_dfe_mode_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x0_rsvd30_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x0_rsvd30_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x1_cntr_target_init_get
* 
* CNTR_TARGET_INIT for initial convergence error rate target.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x1_cntr_target_init_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x1_cntr_target_init_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x1_cntr_target_init_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x2_cntr_target_final_get
* 
* Cntr_target_final for final adaptation error rate target.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x2_cntr_target_final_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x2_cntr_target_final_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x2_cntr_target_final_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x3_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x3_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x3_rx_pam4_sd_thr_get
* 
* Signal detect threshold level. This is an unsigned number and each bit is equivalent to 0.4mV.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x3_rx_pam4_sd_thr_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x3_rx_pam4_sd_thr_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x3_rx_pam4_sd_thr_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x4_sd_period_get
* 
* 4b'{0000 0001 0010... 1111} maps to {0 2^0 2^1... 2^14} SM CLK cycles.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x4_sd_period_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x4_sd_period_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x4_sd_period_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x4_dfe_init_val_get
* 
* Initial DFE value. 
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x4_dfe_init_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x4_dfe_init_val_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x4_dfe_init_val_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x4_rsvd31_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x4_rsvd31_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x5_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x5_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x5_kp_s4_get
* 
* 3b'{000 001 010 011 100 101 110 111} maps to {0 2^-12 2^-11 2^-10 2^-9 2^-8 2^-7 2^-6}
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x5_kp_s4_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x5_kp_s4_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x5_kp_s4_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x5_rsvd32_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x5_rsvd32_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x6_iteration_count_s4_get
* 
* 4b'{0000 0001 0010... 1111} maps to {0 2^0 2^1... 2^14} SM CLK cycles
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x6_iteration_count_s4_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x6_iteration_count_s4_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x6_iteration_count_s4_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x6_rsvd33_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x6_rsvd33_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x6_kp_s5_get
* 
* 3b'{000 001 010 011 100 101 110 111} maps to {0 2^-12 2^-11 2^-10 2^-9 2^-8 2^-7 2^-6}
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x6_kp_s5_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x6_kp_s5_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x6_kp_s5_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x6_rsvd34_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x6_rsvd34_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x6_kp_s6_get
* 
* 3b'{000 001 010 011 100 101 110 111} maps to {0 2^-12 2^-11 2^-10 2^-9 2^-8 2^-7 2^-6}
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x6_kp_s6_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x6_kp_s6_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x6_kp_s6_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x7_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x7_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x7_timer_meas_s5_get
* 
* 5b'{00000 00001 00010... 11111} maps to {0 2^0 2^1... 2^30} SM CLK cycles
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x7_timer_meas_s5_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x7_timer_meas_s5_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x7_timer_meas_s5_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x7_rsvd35_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x7_rsvd35_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x7_timer_meas_s6_get
* 
* 4b'{0000 0001 0010... 1111} maps to {0 2^0 2^2... 2^28} SM CLK cycles
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x7_timer_meas_s6_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x7_timer_meas_s6_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x7_timer_meas_s6_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x7_rsvd36_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x7_rsvd36_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x8_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x8_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x8_freq_init_get
* 
* FREQ_INIT controls the initial frequency offset used for locking timing.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x8_freq_init_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x8_freq_init_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x8_freq_init_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x8_rsvd37_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x8_rsvd37_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x9_iteration_count_s5_get
* 
* 4b'{0000 0001 0010... 1111} maps to {0 2^0 2^1... 2^14} SM clk cycles
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x9_iteration_count_s5_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x9_iteration_count_s5_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x9_iteration_count_s5_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x9_dac_settling_period_get
* 
* 4b'{0000 0001 0010... 1111} maps to {0 2^0 2^1... 2^14} SM clk cycles
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x9_dac_settling_period_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x9_dac_settling_period_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x9_dac_settling_period_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x9_rsvd38_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x9_rsvd38_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0xa_mu_margin_s6_get
* 
* 2b'{00 01 10 11} maps to {0 2^-11 2^-9 2^-7}
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xa_mu_margin_s6_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa_mu_margin_s6_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa_mu_margin_s6_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xa_timer_meas_s7_get
* 
* 4b'{0000 0001 0010... 1111} maps to {0 2^0 2^1... 2^14} SM clk cycles
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xa_timer_meas_s7_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa_timer_meas_s7_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa_timer_meas_s7_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xa_overflow_period_get
* 
* 5b'{00000 00001 00010... 11111} maps to {0 2^0 2^1... 2^30} SM clk cycles
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xa_overflow_period_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa_overflow_period_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa_overflow_period_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xa_hf_period_get
* 
* 5b'{00000 00001 00010... 11111} maps to {0 2^0 2^1... 2^30} SM clk cycles
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xa_hf_period_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa_hf_period_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa_hf_period_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xb_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xb_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0xb_overflow_thr_get
* 
* Overflow threshold
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xb_overflow_thr_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xb_overflow_thr_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xb_overflow_thr_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xb_rsvd39_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xb_rsvd39_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0xb_hf_thr_get
* 
* High frequency pattern test threshold
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xb_hf_thr_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xb_hf_thr_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xb_hf_thr_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xc_overflow_cntr_upper_limit_get
* 
* Overflow counter upper limit of format U16.0
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xc_overflow_cntr_upper_limit_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xc_overflow_cntr_upper_limit_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xc_overflow_cntr_upper_limit_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xd_overflow_cntr_lower_limit_get
* 
* Overflow counter lower limit of format U16.0
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xd_overflow_cntr_lower_limit_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xd_overflow_cntr_lower_limit_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xd_overflow_cntr_lower_limit_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xe_hf_cntr_target_get
* 
* High frequency counter target of format U16.0
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xe_hf_cntr_target_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xe_hf_cntr_target_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xe_hf_cntr_target_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xf_dfe_freeze_en_get
* 
* Controls eye monitor function
*     0h:     Eye Monitor disabled
*     1h:     Eye Monitor enabled
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xf_dfe_freeze_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xf_dfe_freeze_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xf_dfe_freeze_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xf_em_cntr_reset_get
* 
*     0h:     Eye Monitor counter normal
*     1h:     Eye Monitor counter reset and hold to 0
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xf_em_cntr_reset_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xf_em_cntr_reset_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xf_em_cntr_reset_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xf_rsvd40_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xf_rsvd40_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x11_pam4_bp1_en_get
* 
* Enable signal for Breakpoint 1
*     1b: Breakpoint function enabled
*     0b: Breakpoint function disabled
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x11_pam4_bp1_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x11_pam4_bp1_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x11_pam4_bp1_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x11_pam4_bp2_en_get
* 
* Enable signal for Breakpoint 2
*     1b: Breakpoint function enabled
*     0b: Breakpoint function disabled
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x11_pam4_bp2_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x11_pam4_bp2_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x11_pam4_bp2_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x11_pam4_sm_cont_get
* 
* Resume the state machine flow from the trapped state: rising edge trigger
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x11_pam4_sm_cont_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x11_pam4_sm_cont_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x11_pam4_sm_cont_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x11_pam4_bp1_state_get
* 
* Breakpoint 1 for the state machine
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x11_pam4_bp1_state_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x11_pam4_bp1_state_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x11_pam4_bp1_state_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x11_rsvd41_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x11_rsvd41_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x11_pam4_bp2_state_get
* 
* Breakpoint 2 for the state machine
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x11_pam4_bp2_state_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x11_pam4_bp2_state_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x11_pam4_bp2_state_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x12_ths_owen_get
* 
* Overwrite enable signal for THS_OW
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x12_ths_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x12_ths_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x12_ths_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x12_cth_owen_get
* 
* Overwrite enable signal for CTH_OW
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x12_cth_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x12_cth_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x12_cth_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x12_delta_owen_get
* 
* Overwrite enable signal for DELTA_OW
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x12_delta_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x12_delta_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x12_delta_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x12_delta_ow_get
* 
* Overwrite value for DELTA. 
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x12_delta_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x12_delta_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x12_delta_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x12_force_sm_val_en_get
* 
* Enable signal for the force value of the state machine.
*     1b: Force enable
*     0b: Force disable
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x12_force_sm_val_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x12_force_sm_val_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x12_force_sm_val_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x12_force_sm_val_get
* 
* Force value for the state machine
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x12_force_sm_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x12_force_sm_val_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x12_force_sm_val_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x13_ths0_ow_get
* 
* Overwrite value for THS element 0.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x13_ths0_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x13_ths0_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x13_ths0_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x13_ths1_msb_ow_get
* 
* Overwrite value for THS element 1, 4 MSBs. 
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x13_ths1_msb_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x13_ths1_msb_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x13_ths1_msb_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x14_ths1_lsb_ow_get
* 
* Overwrite value for THS element 1, 8 LSBs.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x14_ths1_lsb_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x14_ths1_lsb_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x14_ths1_lsb_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x14_ths2_msb_ow_get
* 
* Overwrite value for THS element 2, 8 MSBs.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x14_ths2_msb_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x14_ths2_msb_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x14_ths2_msb_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x15_ths2_ow_get
* 
* Overwrite value for THS element 2. 4 LSBs.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x15_ths2_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x15_ths2_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x15_ths2_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x15_cth_ow_get
* 
* Overwrite value for CTH. 
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x15_cth_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x15_cth_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x15_cth_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x16_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x16_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x16_hf_cntr_owen_get
* 
* Overwrite enable signal for OW_HF_CNTR
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x16_hf_cntr_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x16_hf_cntr_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x16_hf_cntr_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x16_rsvd42_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x16_rsvd42_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x20_mu_margin_owen_get
* 
* Overwrite enable signal for OW_MU_MARGIN
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x20_mu_margin_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x20_mu_margin_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x20_mu_margin_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x20_mu_margin_ow_get
* 
* Overwrite value for MU_MARGIN. Format U2.0
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x20_mu_margin_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x20_mu_margin_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x20_mu_margin_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x20_kf_owen_get
* 
* Overwrite enable signal for OW_KF
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x20_kf_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x20_kf_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x20_kf_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x20_kf_ow_get
* 
* Overwrite value for KF. Format U2.0
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x20_kf_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x20_kf_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x20_kf_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x20_kp_owen_get
* 
* Overwrite enable signal for OW_KP
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x20_kp_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x20_kp_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x20_kp_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x20_kp_ow_get
* 
* Overwrite value for KP. Format U3.0
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x20_kp_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x20_kp_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x20_kp_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x20_ktheta2_owen_get
* 
* Overwrite enable signal for OW_KTHETA2
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x20_ktheta2_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x20_ktheta2_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x20_ktheta2_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x20_ktheta2_ow_get
* 
* Overwrite value for KTHETA2. Format U2.0
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x20_ktheta2_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x20_ktheta2_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x20_ktheta2_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x20_ktheta3_owen_get
* 
* Overwrite enable signal for OW_KTHETA3
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x20_ktheta3_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x20_ktheta3_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x20_ktheta3_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x20_ktheta3_ow_get
* 
* Overwrite value for KTHETA3. Format U2.0
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x20_ktheta3_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x20_ktheta3_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x20_ktheta3_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x21_ktheta4_owen_get
* 
* Overwrite enable signal for OW_KTHETA4
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x21_ktheta4_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x21_ktheta4_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x21_ktheta4_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x21_ktheta4_ow_get
* 
* Overwrite value for KTHETA4. Format U2.0
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x21_ktheta4_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x21_ktheta4_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x21_ktheta4_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x21_dac_owen_sel_get
* 
* Overwrite enable signal for OW_DAC_SEL
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x21_dac_owen_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x21_dac_owen_sel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x21_dac_owen_sel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x21_dac_ow_sel_get
* 
* Overwrite value for DAC_SEL. Format U4.0
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x21_dac_ow_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x21_dac_ow_sel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x21_dac_ow_sel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x21_rx_pam4_ctle_over_en_get
* 
* When enabled, the CTLE setting is programmed with the override values and no adaptation occurs. For use in testing only.
*     0h:     Override disabled
*     1h:     Override enabled
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x21_rx_pam4_ctle_over_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x21_rx_pam4_ctle_over_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x21_rx_pam4_ctle_over_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x21_rx_pam4_ctle_over_val_get
* 
* CTLE value written when CTLE override is enabled.
*     0h:         22.9 dB
*     1h:         20.4 dB
*     2h:         17.4 dB
*     3h:         15.1 dB
*     4h:         13.0 dB
*     5h:         10.6 dB
*     6h:         7.5 dB
*     7h:         2.5 dB
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x21_rx_pam4_ctle_over_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x21_rx_pam4_ctle_over_val_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x21_rx_pam4_ctle_over_val_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x21_rsvd43_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x21_rsvd43_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x24_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x24_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x24_pam4_sig_det_owen_get
* 
* Enable overwrite of signal detect output.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x24_pam4_sig_det_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x24_pam4_sig_det_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x24_pam4_sig_det_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x24_pam4_sig_det_ow_get
* 
* Overwrite signal detect level.
*     0:    Drive low
*     1:     Drive high
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x24_pam4_sig_det_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x24_pam4_sig_det_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x24_pam4_sig_det_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x28_pam4_bp1_reached_get
* 
* Readout of breakpoint 1 reached indicator signal: 
*     1h:     breakpoint reached
*     0h:     breakpoint not reached
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x28_pam4_bp1_reached_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x28_pam4_bp2_reached_get
* 
* Readout of breakpoint 2 reached indicator signal: 
*     1h:     breakpoint reached
*     0h:     breakpoint not reached
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x28_pam4_bp2_reached_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x28_pam4_current_state_get
* 
* Read back of current state number of the state machine. Note: due to crossing clock domains, this field should be firmware debounced.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x28_pam4_current_state_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x28_read_dac_sel_get
* 
* Readout of the DAC_SEL of format U4.0
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x28_read_dac_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x28_rsvd44_get
* 
* Reserved. Do not change the default value. 
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x28_rsvd44_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x38_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x38_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x38_read_em_get
* 
* Readout of the eye margin. 
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x38_read_em_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x41_pam4_en_get
* 
* Enable SerDes PAM-4 mode: 0=disable, 1=enable
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x41_pam4_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x41_pam4_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x41_pam4_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x41_rsvd45_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x41_rsvd45_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x41_theta2_update_mode_get
* 
* Select the UPDN mode for theta2 loop: 0=clock comp, 1=data
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x41_theta2_update_mode_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x41_theta2_update_mode_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x41_theta2_update_mode_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x41_theta3_update_mode_get
* 
* Select the UPDN mode for theta3 loop: 0=clock comp, 1=data
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x41_theta3_update_mode_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x41_theta3_update_mode_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x41_theta3_update_mode_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x41_theta4_update_mode_get
* 
* Select the UPDN mode for theta4 loop: 0=clock comp, 1=data
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x41_theta4_update_mode_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x41_theta4_update_mode_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x41_theta4_update_mode_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x41_rsvd46_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x41_rsvd46_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x41_theta2_update_flip_get
* 
* Flip theta2 loop polarity: 1=flip, 0=no flip
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x41_theta2_update_flip_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x41_theta2_update_flip_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x41_theta2_update_flip_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x41_theta3_update_flip_get
* 
* Flip theta3 loop polarity: 1=flip, 0=no flip
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x41_theta3_update_flip_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x41_theta3_update_flip_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x41_theta3_update_flip_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x41_theta4_update_flip_get
* 
* Flip theta4 loop polarity: 1=flip, 0=no flip
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x41_theta4_update_flip_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x41_theta4_update_flip_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x41_theta4_update_flip_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x41_rsvd47_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x41_rsvd47_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x42_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x42_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x42_cntr1_en_get
* 
* Counter on phase1
*     0b:    Disable
*     1b:    Enable
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x42_cntr1_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x42_cntr1_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x42_cntr1_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x42_cntr2_en_get
* 
* Counter on phase2
*     0b:    Disable
*     1b:    Enable
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x42_cntr2_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x42_cntr2_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x42_cntr2_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x42_cntr3_en_get
* 
* Counter on phase3
*     0b:    Disable
*     1b:    Enable
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x42_cntr3_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x42_cntr3_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x42_cntr3_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x42_cntr4_en_get
* 
* Counter on phase4
*     0b:    Disable
*     1b:    Enable
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x42_cntr4_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x42_cntr4_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x42_cntr4_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x42_cntr5_en_get
* 
* Counter on phase5
*     0b:    Disable
*     1b:    Enable
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x42_cntr5_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x42_cntr5_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x42_cntr5_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x42_cntr6_en_get
* 
* Counter on phase6
*     0b:    Disable
*     1b:    Enable
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x42_cntr6_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x42_cntr6_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x42_cntr6_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x42_cntr7_en_get
* 
* Counter on phase7
*     0b:    Disable
*     1b:    Enable
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x42_cntr7_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x42_cntr7_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x42_cntr7_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x42_cntr8_en_get
* 
* Counter on phase8
*     0b:    Disable
*     1b:    Enable
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x42_cntr8_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x42_cntr8_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x42_cntr8_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x42_rx_precode_en_get
* 
* RX Precode Enable bit
*     0b:    Disable
*     1b:    Enable
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x42_rx_precode_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x42_rx_precode_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x42_rx_precode_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x42_rx_graycode_en_get
* 
* RX Gray code Enable bit
*     0b:    Disable
*     1b:    Enable
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x42_rx_graycode_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x42_rx_graycode_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x42_rx_graycode_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x43_rx_swap_msb_lsb_get
* 
* Select if the MSB and LSB of the PAM-4 symbol is swapped in the RX:
*     0b:    Not swapped
*     1b:    Swapped
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x43_rx_swap_msb_lsb_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x43_rx_swap_msb_lsb_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x43_rx_swap_msb_lsb_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x43_rsvd48_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x43_rsvd48_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x43_prbs_check_cntr_reset_get
* 
* PRBS checker counter reset signal: 
*     0b:    Normal accumulation
*     1b:    Keep resetting counter to 0
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x43_prbs_check_cntr_reset_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x43_prbs_check_cntr_reset_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x43_prbs_check_cntr_reset_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x43_rx_data_fw_flip_get
* 
* XOR with RX_DATA_FLIP to generate overall rx data flip: 
*     0b:    No flip
*     1b:    Flip polarity
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x43_rx_data_fw_flip_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x43_rx_data_fw_flip_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x43_rx_data_fw_flip_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x43_rx_data_flip_get
* 
* XOR with rx_data_firmware_flip to generate overall rx data flip: 
*     0b:    No flip
*     1b:    Flip polarity
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x43_rx_data_flip_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x43_rx_data_flip_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x43_rx_data_flip_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x43_prbs_mode_sel_get
* 
* PRBS checker and mode sel: 
*     00b:    PRBS9
*     01b:    PRBS13
*     10b:    PRBS15
*     11b:    PRBS31
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x43_prbs_mode_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x43_prbs_mode_sel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x43_prbs_mode_sel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x43_pu_prbs_chkr_get
* 
* PRBS checker power up: 
*     0b:    Power down
*     1b:    Power up
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x43_pu_prbs_chkr_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x43_pu_prbs_chkr_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x43_pu_prbs_chkr_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x43_pu_prbs_sync_chkr_get
* 
* PRBS sync checker power up: 
*     0b:    Power down
*     1b:    Power up
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x43_pu_prbs_sync_chkr_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x43_pu_prbs_sync_chkr_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x43_pu_prbs_sync_chkr_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x43_rx_prbs_force_reload_get
* 
* Forces to reload the PRBS sync checker's shift registers: edge triggered
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x43_rx_prbs_force_reload_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x43_rx_prbs_force_reload_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x43_rx_prbs_force_reload_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x43_rx_prbs_auto_sync_en_get
* 
* Enables the auto re-sync function: 
*     0b:    Disable
*     1b:    Enable
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x43_rx_prbs_auto_sync_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x43_rx_prbs_auto_sync_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x43_rx_prbs_auto_sync_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x43_prbs_sync_cntr_reset_get
* 
* PRBS sync error cntr reset signal: 
*     0b:    Normal accumulation
*     1b:    Keep resetting counter to 0
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x43_prbs_sync_cntr_reset_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x43_prbs_sync_cntr_reset_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x43_prbs_sync_cntr_reset_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x44_prbs_mismatch_thr_get
* 
* PRBS mismatch threshold for the sync checker of format U8.0
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x44_prbs_mismatch_thr_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x44_prbs_mismatch_thr_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x44_prbs_mismatch_thr_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x44_prbs_sync_loss_thr_get
* 
* PRBS sync loss threshold for the sync checker of format U6.0
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x44_prbs_sync_loss_thr_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x44_prbs_sync_loss_thr_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x44_prbs_sync_loss_thr_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x44_rsvd49_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x44_rsvd49_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x45_prbs_sync_thr_get
* 
* PRBS sync threshold for the sync checker of format U5.0
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x45_prbs_sync_thr_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x45_prbs_sync_thr_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x45_prbs_sync_thr_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x45_prbs_pat_cntr_reset_get
* 
* PRBS pattern cntr reset signal: 
*     1 = Keep resetting counter to 0
*     0 = Normal accumulation
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x45_prbs_pat_cntr_reset_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x45_prbs_pat_cntr_reset_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x45_prbs_pat_cntr_reset_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x45_rsvd50_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x45_rsvd50_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x45_rx_asic_flip_get
* 
* Digital Inversion of RX data.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x45_rx_asic_flip_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x45_rx_asic_flip_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x45_rx_asic_flip_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x46_ph1_acc_owen_get
* 
* Overwrite enable signal for OW_PH1_ACC. 1=enable
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x46_ph1_acc_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x46_ph1_acc_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x46_ph1_acc_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x46_ph1_acc_ow_get
* 
* Overwrite value for PH1_ACC 7 MSBs
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x46_ph1_acc_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x46_ph1_acc_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x46_ph1_acc_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x46_theta2_acc_owen_get
* 
* Overwrite enable signal for OW_THETA2_ACC. 1=enable
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x46_theta2_acc_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x46_theta2_acc_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x46_theta2_acc_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x46_theta2_acc_ow_get
* 
* Overwrite value for THETA2_ACC 7 MSBs
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x46_theta2_acc_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x46_theta2_acc_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x46_theta2_acc_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x47_theta3_acc_owen_get
* 
* Overwrite enable signal for OW_THETA3_ACC. 1=enable
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x47_theta3_acc_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x47_theta3_acc_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x47_theta3_acc_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x47_theta3_acc_ow_get
* 
* Overwrite value for THETA3_ACC 7 MSBs
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x47_theta3_acc_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x47_theta3_acc_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x47_theta3_acc_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x47_theta4_acc_owen_get
* 
* Overwrite enable signal for OW_THETA4_ACC. 1=enable
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x47_theta4_acc_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x47_theta4_acc_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x47_theta4_acc_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x47_theta4_acc_ow_get
* 
* Overwrite value for THETA4_ACC 7 MSBs
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x47_theta4_acc_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x47_theta4_acc_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x47_theta4_acc_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x48_agc_setting0_get
* 
* The AGC setting for the highest peaking.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x48_agc_setting0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x48_agc_setting0_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x48_agc_setting0_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x48_agc_setting1_get
* 
* The AGC setting for the second highest peaking.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x48_agc_setting1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x48_agc_setting1_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x48_agc_setting1_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x48_agc_setting2_msb_get
* 
* 4 MSBs of the AGC setting for the third highest peaking.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x48_agc_setting2_msb_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x48_agc_setting2_msb_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x48_agc_setting2_msb_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x49_agc_setting2_lsb_get
* 
* 2 LSBs of the AGC setting for the third highest peaking.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x49_agc_setting2_lsb_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x49_agc_setting2_lsb_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x49_agc_setting2_lsb_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x49_agc_setting3_get
* 
* The AGC setting for the fourth highest peaking.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x49_agc_setting3_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x49_agc_setting3_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x49_agc_setting3_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x49_agc_setting4_get
* 
* The AGC setting for the fifth highest peaking.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x49_agc_setting4_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x49_agc_setting4_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x49_agc_setting4_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x49_agc_setting5_msb_get
* 
* 2 MSBs of the AGC setting for the sixth highest peaking.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x49_agc_setting5_msb_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x49_agc_setting5_msb_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x49_agc_setting5_msb_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x4a_agc_setting5_lsb_get
* 
* 4 LSBs of the AGC setting for the sixth highest peaking
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x4a_agc_setting5_lsb_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x4a_agc_setting5_lsb_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x4a_agc_setting5_lsb_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x4a_agc_setting6_get
* 
* The AGC setting for the seventh highest peaking
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x4a_agc_setting6_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x4a_agc_setting6_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x4a_agc_setting6_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x4a_agc_setting7_get
* 
* The AGC setting for the lowest peaking
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x4a_agc_setting7_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x4a_agc_setting7_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x4a_agc_setting7_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x4b_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x4b_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x4b_dtl_lost_lock_mode_get
* 
* Select which frequency sets alarm mode: 
*     0h    Wander detect
*     1h    Local frequency accumulator
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x4b_dtl_lost_lock_mode_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x4b_dtl_lost_lock_mode_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x4b_dtl_lost_lock_mode_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x4b_top_rotr_en_get
* 
* Enables the frequency accumulator to top/PLL rotator
*     0h    Disables PLL interpolator
*     1h    Enables PLL interpolator (Default)
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x4b_top_rotr_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x4b_top_rotr_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x4b_top_rotr_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x4b_rx_freq_en_get
* 
* Enables RX DTL interpolator
*     0h    Disables the DTL interpolator (Default)
*     1h    Enables the DTL interpolator
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x4b_rx_freq_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x4b_rx_freq_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x4b_rx_freq_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x4b_sd_cntr_limit_msb_get
* 
* SD_CNTR_LIMIT for signal detection counter limit. 11 MSB of 20 bits.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x4b_sd_cntr_limit_msb_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x4b_sd_cntr_limit_msb_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x4b_sd_cntr_limit_msb_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x4c_sd_cntr_limit_lsb_get
* 
* SD_CNTR_LIMIT for signal detection counter limit. 9 LSB of 20 bits. 
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x4c_sd_cntr_limit_lsb_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x4c_sd_cntr_limit_lsb_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x4c_sd_cntr_limit_lsb_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x4c_rsvd51_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x4c_rsvd51_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x4e_rx_pam4_prbs_err_chkr_msb_get
* 
* Read only, variable value of the upper word (16 MSB) of the RX PRBS 32-bit error checker count.
* Note: due to crossing clock domains, this field should be firmware debounced.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x4e_rx_pam4_prbs_err_chkr_msb_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x4f_rx_pam4_prbs_err_chkr_lsb_get
* 
* Read only, variable value of the lower word of the RX PRBS 32-bit error checker count.
* Note: due to crossing clock domains, this field should be firmware debounced.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x4f_rx_pam4_prbs_err_chkr_lsb_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x50_prbs_read_sync_err_cntr_msb_get
* 
* Readout of the PRBS bit error counter (16 MSBs)
* Note: due to crossing clock domains, this field should be firmware debounced.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x50_prbs_read_sync_err_cntr_msb_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x51_prbs_read_sync_err_cntr_lsb_get
* 
* Readout of the PRBS bit error counter (16 LSBs)
* Note: due to crossing clock domains, this field should be firmware debounced.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x51_prbs_read_sync_err_cntr_lsb_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x6a_rx_read_phy_ready_get
* 
* Readout of the PHY_READY signal from state machine
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x6a_rx_read_phy_ready_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x6a_rsvd52_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x6a_rsvd52_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x6a_read_sig_det_get
* 
* Readout of the signal detect indicator from SD_PAT counter block
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x6a_read_sig_det_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x6a_rsvd53_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x6a_rsvd53_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x6c_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x6c_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x6c_read_ph1_get
* 
* Readout of DTL's phase1
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x6c_read_ph1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x6c_rsvd54_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x6c_rsvd54_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x6c_read_theta2_get
* 
* Readout of DTL's theta2
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x6c_read_theta2_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x6d_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x6d_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x6d_read_theta3_get
* 
* Readout of DTL's theta3.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x6d_read_theta3_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x6d_rsvd55_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x6d_rsvd55_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x6d_read_theta4_get
* 
* Readout of DTL's theta4.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x6d_read_theta4_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x6e_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x6e_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x6e_readout_sync_en_get
* 
* Enable bit for atomic readout of PRBS errors.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x6e_readout_sync_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x6e_readout_sync_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x6e_readout_sync_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x6e_readout_capture_get
* 
* Latch the PRBS errors to read out register rising edge trigger.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x6e_readout_capture_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x6e_readout_capture_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x6e_readout_capture_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x6e_rsvd56_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x6e_rsvd56_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x73_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x73_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x73_read_freq_acc_get
* 
* Readout of frequency accumulator 16 MSBs from DTL.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x73_read_freq_acc_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x74_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x74_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x74_read_ph_wander_get
* 
* Readout of the absolute value of phase wander in U15.14 format
* Note: due to crossing clock domains, this field should be firmware debounced.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x74_read_ph_wander_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x79_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x79_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x79_lock_ph_flip_get
* 
* TRF output polarity. Set to 1b. 
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x79_lock_ph_flip_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x79_lock_ph_flip_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x79_lock_ph_flip_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x79_spd_sel2_get
* 
* TRF speed selection MSB:
*     Set to 01b if RX baud rate is 25.78G and TX baud rate is 26.56G.
*     Otherwise, set to 00b.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x79_spd_sel2_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x79_spd_sel2_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x79_spd_sel2_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x79_spd_sel1_get
* 
* TRF speed selection LSB:
*     Set to 01b if RX baud rate is 26.56G and TX baud rate is 25.78G.
*     Otherwise, set to 00b.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x79_spd_sel1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x79_spd_sel1_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x79_spd_sel1_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x79_freq_mup_get
* 
* TRF control bit. Always set to 010b.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x79_freq_mup_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x79_freq_mup_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x79_freq_mup_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x79_freq_multiplier_get
* 
* TRF control bit:
*     Set to 01b if RX baud rate is 25.78G, and TX baud rate is 26.56G.
*     Set to 10b if RX baud rate is 26.56G and TX baud rate is 25.78G.
*     Otherwise, set to 00b.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x79_freq_multiplier_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x79_freq_multiplier_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x79_freq_multiplier_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x79_freq_shift_sel_get
* 
* TRF control bit. Always set to 100b.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x79_freq_shift_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x79_freq_shift_sel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x79_freq_shift_sel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x80_bp5_en_get
* 
* Enable signal for breakpoint 5:
*     1h:     breakpoint function enabled
*     0h:     function disabled
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x80_bp5_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x80_bp5_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x80_bp5_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x80_rsvd57_get
* 
* Reserved. Do not change the default value. 
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x80_rsvd57_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x80_bp5_state_get
* 
* Breakpoint 5 for the state machine 
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x80_bp5_state_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x80_bp5_state_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x80_bp5_state_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x80_bp6_en_get
* 
* Enable signal for breakpoint 6:
*     1h:     breakpoint function enabled
*     0h:     function disabled
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x80_bp6_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x80_bp6_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x80_bp6_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x80_rsvd58_get
* 
* Reserved. Do not change the default value. 
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x80_rsvd58_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x80_bp6_state_get
* 
* Breakpoint 6 for the state machine 
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x80_bp6_state_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x80_bp6_state_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x80_bp6_state_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x87_comparator_offsets_accu_data_ow_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x87_comparator_offsets_accu_data_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x87_comparator_offsets_accu_data_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x87_comparator_offsets_accu_data_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x87_mu_offset_owen_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x87_mu_offset_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x87_mu_offset_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x87_mu_offset_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x87_mu_offset_ow_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x87_mu_offset_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x87_mu_offset_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x87_mu_offset_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x87_rsvd59_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x87_rsvd59_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x88_plus_margin_sel_get
* 
* Plus margin readout address select.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x88_plus_margin_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x88_plus_margin_sel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x88_plus_margin_sel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x88_minus_margin_sel_get
* 
* Minus margin readout address select.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x88_minus_margin_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x88_minus_margin_sel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x88_minus_margin_sel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x88_ths_sel_get
* 
* Threshold 0-11 readout select.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x88_ths_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x88_ths_sel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x88_ths_sel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x88_rsvd60_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x88_rsvd60_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x8f_pam4_bp3_en_get
* 
* Enable signal for breakpoint 3:
*     1h:     breakpoint function enabled
*     0h:     function disabled
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x8f_pam4_bp3_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x8f_pam4_bp3_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x8f_pam4_bp3_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x8f_rsvd61_get
* 
* Reserved. Do not change the default value. 
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x8f_rsvd61_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x8f_pam4_bp3_state_get
* 
* Breakpoint 3 for the state machine 
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x8f_pam4_bp3_state_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x8f_pam4_bp3_state_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x8f_pam4_bp3_state_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x8f_pam4_bp4_en_get
* 
* Enable signal for breakpoint 4:
*     1h:     breakpoint function enabled
*     0h:     function disabled
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x8f_pam4_bp4_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x8f_pam4_bp4_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x8f_pam4_bp4_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x8f_rsvd62_get
* 
* Reserved. Do not change the default value. 
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x8f_rsvd62_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x8f_pam4_bp4_state_get
* 
* Breakpoint 4 for the state machine 
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x8f_pam4_bp4_state_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x8f_pam4_bp4_state_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x8f_pam4_bp4_state_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x90_xcorr_c_qua_pol_get
* 
* Polarity for decision symbol in qualify U1.0 
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x90_xcorr_c_qua_pol_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x90_xcorr_c_qua_pol_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x90_xcorr_c_qua_pol_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x90_xcorr_corr_shift_tap_get
* 
* Chunk index U3.0
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x90_xcorr_corr_shift_tap_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x90_xcorr_corr_shift_tap_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x90_xcorr_corr_shift_tap_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x90_xcorr_timer_set_tap_get
* 
* Counter timer U4.0
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x90_xcorr_timer_set_tap_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x90_xcorr_timer_set_tap_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x90_xcorr_timer_set_tap_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x90_xcorr_fixed_pat_xcorr_get
* 
* Qualify pattern [b0, bm1] U4.0
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x90_xcorr_fixed_pat_xcorr_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x90_xcorr_fixed_pat_xcorr_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x90_xcorr_fixed_pat_xcorr_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x90_xcorr_cntr_read_mux_get
* 
* Read XCORR counter mux U4.0
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x90_xcorr_cntr_read_mux_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x90_xcorr_cntr_read_mux_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x90_xcorr_cntr_read_mux_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x91_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x91_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x91_xcorr_gate_get
* 
* Disable bit for XCORR U1.0
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x91_xcorr_gate_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x91_xcorr_gate_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x91_xcorr_gate_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x91_rsvd63_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x91_rsvd63_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x91_xcorr_cntr_freeze_dc_get
* 
* Counter freeze for Error counter U1.0
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x91_xcorr_cntr_freeze_dc_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x91_xcorr_cntr_freeze_dc_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x91_xcorr_cntr_freeze_dc_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x91_xcorr_cntr_freeze_tap_get
* 
* Counter freeze for tap counter U1.0
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x91_xcorr_cntr_freeze_tap_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x91_xcorr_cntr_freeze_tap_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x91_xcorr_cntr_freeze_tap_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x91_xcorr_result_clr_dc_get
* 
* Counter clear for Error counter U1.0
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x91_xcorr_result_clr_dc_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x91_xcorr_result_clr_dc_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x91_xcorr_result_clr_dc_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x91_xcorr_result_clr_tap_get
* 
* Counter clear for tap counter U1.0
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x91_xcorr_result_clr_tap_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x91_xcorr_result_clr_tap_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x91_xcorr_result_clr_tap_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x91_xcorr_timer_start_clr_tap_get
* 
* Timer start/clear control U1.0
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x91_xcorr_timer_start_clr_tap_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x91_xcorr_timer_start_clr_tap_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x91_xcorr_timer_start_clr_tap_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x92_xcorr_readout_get
* 
* XCORR tap counter readout S16.0
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x92_xcorr_readout_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x93_xcorr_err_readout_get
* 
* Error counter readout S16.0
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x93_xcorr_err_readout_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x94_mm_dtl_en_get
* 
* DTL mode. 
*     0b:     BB
*     1b:     MM
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x94_mm_dtl_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x94_mm_dtl_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x94_mm_dtl_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x94_mm_quali_mode_get
* 
* MM UPDN Qualification mode.
*     1b: MSB transition
*     0b: no qualifier
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x94_mm_quali_mode_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x94_mm_quali_mode_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x94_mm_quali_mode_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x94_mm_f0_fgt_get
* 
* F0 target value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x94_mm_f0_fgt_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x94_mm_f0_fgt_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x94_mm_f0_fgt_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x94_mm_pol_get
* 
* MM UPDN polarity flip.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x94_mm_pol_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x94_mm_pol_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x94_mm_pol_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x94_mm_lev_update_get
* 
* Update all the settings, rising edge sensitive.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x94_mm_lev_update_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x94_mm_lev_update_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x94_mm_lev_update_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x94_mm_f1_fgt_get
* 
* F1 target value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x94_mm_f1_fgt_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x94_mm_f1_fgt_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x94_mm_f1_fgt_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x95_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x95_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x95_mm_quant_lev0_get
* 
* Quanti level 0. S6.7
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x95_mm_quant_lev0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x95_mm_quant_lev0_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x95_mm_quant_lev0_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x95_rsvd64_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x95_rsvd64_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x95_mm_quant_lev1_get
* 
* Quanti level 1. S6.7
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x95_mm_quant_lev1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x95_mm_quant_lev1_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x95_mm_quant_lev1_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x96_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x96_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x96_mm_quant_lev2_get
* 
* Quanti level 2. S6.7
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x96_mm_quant_lev2_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x96_mm_quant_lev2_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x96_mm_quant_lev2_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x96_rsvd65_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x96_rsvd65_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x96_mm_quant_lev3_get
* 
* Quanti level 3. S6.7
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x96_mm_quant_lev3_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x96_mm_quant_lev3_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x96_mm_quant_lev3_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x97_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x97_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x97_mm_quant_lev4_get
* 
* Quanti level 4. S6.7
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x97_mm_quant_lev4_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x97_mm_quant_lev4_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x97_mm_quant_lev4_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x97_rsvd66_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x97_rsvd66_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x97_mm_quant_lev5_get
* 
* Quanti level 5. S6.7
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x97_mm_quant_lev5_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x97_mm_quant_lev5_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x97_mm_quant_lev5_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x98_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x98_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x98_mm_quant_lev6_get
* 
* Quanti level 6. S6.7
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x98_mm_quant_lev6_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x98_mm_quant_lev6_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x98_mm_quant_lev6_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x98_rsvd67_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x98_rsvd67_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x98_mm_quant_lev7_get
* 
* Quanti level 7. S6.7
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x98_mm_quant_lev7_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x98_mm_quant_lev7_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x98_mm_quant_lev7_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x99_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x99_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x99_mm_quant_lev8_get
* 
* Quanti level 8. S6.7
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x99_mm_quant_lev8_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x99_mm_quant_lev8_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x99_mm_quant_lev8_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x99_rsvd68_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x99_rsvd68_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x99_mm_quant_lev9_get
* 
* Quanti level 9. S6.7
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x99_mm_quant_lev9_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x99_mm_quant_lev9_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x99_mm_quant_lev9_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x9a_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x9a_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x9a_mm_quant_lev10_get
* 
* Quanti level 10. S6.7
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x9a_mm_quant_lev10_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x9a_mm_quant_lev10_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x9a_mm_quant_lev10_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x9a_rsvd69_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x9a_rsvd69_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x9a_mm_quant_lev11_get
* 
* Quanti level 11. S6.7
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x9a_mm_quant_lev11_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x9a_mm_quant_lev11_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x9a_mm_quant_lev11_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x9b_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x9b_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0x9b_mm_quant_lev12_get
* 
* Quanti level 12. S6.7
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x9b_mm_quant_lev12_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x9b_mm_quant_lev12_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0x9b_mm_quant_lev12_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0x9b_rsvd70_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0x9b_rsvd70_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0xa0_tx_test_data_src_get
* 
* Mux control bit selecting the TX test pattern when operating in test mode.
*     0h:     Test pattern memory, JP03B, or TxLinearity
*     1h:     PRBS generator
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xa0_tx_test_data_src_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa0_tx_test_data_src_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa0_tx_test_data_src_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xa0_tx_prbs_clk_en_get
* 
* Enables the clock source to the PRBS generator.
*     0h:     Disabled
*     1h:     Enabled
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xa0_tx_prbs_clk_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa0_tx_prbs_clk_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa0_tx_prbs_clk_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xa0_tx_test_en_get
* 
* Mux control bit to select whether the TX sends out user data or test data.
*     0h:     User data
*     1h:     Test data
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xa0_tx_test_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa0_tx_test_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa0_tx_test_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xa0_rsvd71_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xa0_rsvd71_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0xa0_tx_prbs_gen_en_get
* 
* Enables the TX PRBS generator logic.
*     0h:     Disabled
*     1h:     Enabled
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xa0_tx_prbs_gen_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa0_tx_prbs_gen_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa0_tx_prbs_gen_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xa0_tx_prbs_gen_err_get
* 
* Setting this bit will generate a single bit flip in the PRBS generated output. To send additional error(s), the bit must be cleared first.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xa0_tx_prbs_gen_err_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa0_tx_prbs_gen_err_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa0_tx_prbs_gen_err_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xa0_tx_prbs_mode_get
* 
* Mode select for the TX PRBS generator.
*     00b:     NRZ PRBS9 or PAM-4 PRBS9
*     01b:     NRZ PRBS15 or PAM-4 PRBS13
*     10b:     NRZ PRBS23 or PAM-4 PRBS15
*     11b:     NRZ PRBS31 or PAM-4 PRBS31
* * The PRBS13Q or PRBS31Q test pattern is a sequence formed by gray coding pairs of bits from two repetitions of the PRBS13 or PRBS31 pattern into PAM-4 symbols. After setting register 0n00AF[9]=1'b1 to enable gray code, the normal PRBS generator will generate PRBS13Q and PRBS31Q patterns.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xa0_tx_prbs_mode_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa0_tx_prbs_mode_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa0_tx_prbs_mode_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xa0_tx_data_pol_flip_get
* 
* Polarity of the outgoing TX data.
*     0h:     Normal
*     1h:     Inverted
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xa0_tx_data_pol_flip_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa0_tx_data_pol_flip_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa0_tx_data_pol_flip_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xa0_rsvd72_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xa0_rsvd72_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0xa0_tx_ana_out_flip_get
* 
* TX analog output flip control
*     0h:     Inverted
*     1h:     Normal
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xa0_tx_ana_out_flip_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa0_tx_ana_out_flip_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa0_tx_ana_out_flip_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xa0_rsvd73_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xa0_rsvd73_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0xa0_pam4_test_pat_mode_get
* 
* Use these bits to select the pattern mode.
*     00b:     Pattern is generated by pattern memory
*     01b:     Pattern is JP03B
*     10b:     Pattern is TxLinearity
* Notes:
* *To use the JP03B pattern, set 0n00A0[13] to 1'b1, set 0n00A0[15] to 1'b0, and set 00A0[3:2] to 2'b01.
* * The JP03A pattern is a repeating {0,3} sequence that can be generated in pattern memory registers 00A1, 00A2, 00A3, and 00A4. Set 0n00A0[3:2] to 2'b00.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xa0_pam4_test_pat_mode_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa0_pam4_test_pat_mode_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa0_pam4_test_pat_mode_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xa0_half_rate_speed_mode_get
* 
* Use these bits to select the operation speed when TX half-rate is enabled in 0n00B0[0] (set to 1'b1).
*     00b:     Full half-rate
*     01b:     Half half-rate
*     10b:     Quarter half-rate
*     11b:     Eighth half-rate
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xa0_half_rate_speed_mode_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa0_half_rate_speed_mode_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa0_half_rate_speed_mode_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xa1_tx_test_pat_3_get
* 
* Upper word of 64-bit TX test pattern memory, bits 63:48. (16 MSB)
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xa1_tx_test_pat_3_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa1_tx_test_pat_3_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa1_tx_test_pat_3_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xa2_tx_test_pat_2_get
* 
* Next word of 64-bit TX test pattern memory, bits 47:32. (Next 16 MSB)
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xa2_tx_test_pat_2_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa2_tx_test_pat_2_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa2_tx_test_pat_2_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xa3_tx_test_pat_1_get
* 
* Next word of 64-bit TX test pattern memory, bits 31:16. (Next 16 MSB)
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xa3_tx_test_pat_1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa3_tx_test_pat_1_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa3_tx_test_pat_1_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xa4_tx_test_pat_0_get
* 
* Lowest word of 64-bit TX test pattern memory, bits 15:0. (16 LSB)
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xa4_tx_test_pat_0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa4_tx_test_pat_0_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa4_tx_test_pat_0_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xa5_tx_pre_2_get
* 
* TX coefficient value for pre-cursor 2 of +1 level.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xa5_tx_pre_2_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa5_tx_pre_2_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa5_tx_pre_2_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xa5_tx_pre_2_comp_get
* 
* TX coefficient value for pre-cursor 2 of -1 level.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xa5_tx_pre_2_comp_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa5_tx_pre_2_comp_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa5_tx_pre_2_comp_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xa6_tx_pre_2_3x_get
* 
* TX coefficient value for pre-cursor 2 of +3 level.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xa6_tx_pre_2_3x_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa6_tx_pre_2_3x_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa6_tx_pre_2_3x_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xa6_tx_pre_2_3x_comp_get
* 
* TX coefficient value for pre-cursor 2 of -3 level.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xa6_tx_pre_2_3x_comp_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa6_tx_pre_2_3x_comp_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa6_tx_pre_2_3x_comp_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xa7_tx_pre_1_get
* 
* TX coefficient value for pre-cursor 1 of +1 level
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xa7_tx_pre_1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa7_tx_pre_1_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa7_tx_pre_1_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xa7_tx_pre_1_comp_get
* 
* TX coefficient value for pre-cursor 1 of -1 level.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xa7_tx_pre_1_comp_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa7_tx_pre_1_comp_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa7_tx_pre_1_comp_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xa8_tx_pre_1_3x_get
* 
* TX coefficient value for pre-cursor 1 of +3 level.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xa8_tx_pre_1_3x_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa8_tx_pre_1_3x_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa8_tx_pre_1_3x_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xa8_tx_pre_1_3x_comp_get
* 
* TX coefficient value for pre-cursor 1 of -3 level.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xa8_tx_pre_1_3x_comp_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa8_tx_pre_1_3x_comp_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa8_tx_pre_1_3x_comp_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xa9_tx_main_get
* 
* TX coefficient value for the main-cursor of +1 level.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xa9_tx_main_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa9_tx_main_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa9_tx_main_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xa9_tx_main_comp_get
* 
* TX coefficient value for the main cursor of -1 level. 
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xa9_tx_main_comp_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa9_tx_main_comp_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xa9_tx_main_comp_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xaa_tx_main_3x_get
* 
* TX coefficient value for the main-cursor of +3 level
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xaa_tx_main_3x_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xaa_tx_main_3x_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xaa_tx_main_3x_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xaa_tx_main_3x_comp_get
* 
* TX coefficient value for the main cursor -3 level. 
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xaa_tx_main_3x_comp_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xaa_tx_main_3x_comp_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xaa_tx_main_3x_comp_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xab_tx_post_1_get
* 
* TX coefficient value for post-cursor 1 of level +1
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xab_tx_post_1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xab_tx_post_1_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xab_tx_post_1_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xab_tx_post_1_comp_get
* 
* TX coefficient value for post-cursor 1 of level -1
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xab_tx_post_1_comp_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xab_tx_post_1_comp_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xab_tx_post_1_comp_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xac_tx_post_1_3x_get
* 
* TX coefficient for post-cursor 1 of level +3.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xac_tx_post_1_3x_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xac_tx_post_1_3x_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xac_tx_post_1_3x_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xac_tx_post_1_3x_comp_get
* 
* TX coefficient value for post-cursor 1 of level -3.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xac_tx_post_1_3x_comp_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xac_tx_post_1_3x_comp_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xac_tx_post_1_3x_comp_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xad_tx_post_2_get
* 
* TX coefficient value for post-cursor 2 of level +1.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xad_tx_post_2_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xad_tx_post_2_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xad_tx_post_2_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xad_tx_post_2_comp_get
* 
* TX coefficient for post-cursor 2 of level -1.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xad_tx_post_2_comp_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xad_tx_post_2_comp_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xad_tx_post_2_comp_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xae_tx_post_2_3x_get
* 
* TX coefficient value for post-cursor 2 of level +3.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xae_tx_post_2_3x_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xae_tx_post_2_3x_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xae_tx_post_2_3x_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xae_tx_post_2_3x_comp_get
* 
* TX coefficient value for post-cursor 2 of level -3.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xae_tx_post_2_3x_comp_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xae_tx_post_2_3x_comp_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xae_tx_post_2_3x_comp_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xaf_tx_post2_auto_sel_get
* 
* Enable bit for automatic select coefficient post-cursor 2
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xaf_tx_post2_auto_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xaf_tx_post2_auto_sel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xaf_tx_post2_auto_sel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xaf_tx_post1_auto_sel_get
* 
* Enable bit for automatic select coefficient post-cursor 1
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xaf_tx_post1_auto_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xaf_tx_post1_auto_sel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xaf_tx_post1_auto_sel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xaf_tx_main_auto_sel_get
* 
* Enable bit for automatic select coefficient main cursor
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xaf_tx_main_auto_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xaf_tx_main_auto_sel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xaf_tx_main_auto_sel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xaf_tx_pre1_auto_sel_get
* 
* Enable bit for automatic select coefficient pre-cursor 1
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xaf_tx_pre1_auto_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xaf_tx_pre1_auto_sel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xaf_tx_pre1_auto_sel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xaf_tx_pre2_auto_sel_get
* 
* Enable bit for automatic select coefficient pre-cursor 2
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xaf_tx_pre2_auto_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xaf_tx_pre2_auto_sel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xaf_tx_pre2_auto_sel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xaf_tx_swap_msb_lsb_get
* 
* Swap the MSB and LSB.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xaf_tx_swap_msb_lsb_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xaf_tx_swap_msb_lsb_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xaf_tx_swap_msb_lsb_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xaf_tx_graycode_en_get
* 
* Set this bit to 1b to enable gray code. When gray code is enabled, you can generate PAM-4 PRBS13Q or PRBS31Q test patterns. See also TX_PRBS_MODE in register 0n00A0.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xaf_tx_graycode_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xaf_tx_graycode_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xaf_tx_graycode_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xaf_tx_precode_en_get
* 
* TX Precode enable bit: 1=enable, 0=disable
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xaf_tx_precode_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xaf_tx_precode_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xaf_tx_precode_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xaf_rsvd74_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xaf_rsvd74_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0xaf_tx_post2_scale_get
* 
* TX post-cursor 2 scale
*     0h: Half scale
*     1h: Full scale
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xaf_tx_post2_scale_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xaf_tx_post2_scale_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xaf_tx_post2_scale_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xaf_tx_post1_scale_get
* 
* TX post-cursor 1 scale
*     0h: Half scale
*     1h: Full scale
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xaf_tx_post1_scale_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xaf_tx_post1_scale_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xaf_tx_post1_scale_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xaf_tx_main_scale_get
* 
* TX main cursor scale
*     0h: Half scale
*     1h: Full scale
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xaf_tx_main_scale_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xaf_tx_main_scale_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xaf_tx_main_scale_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xaf_tx_pre1_scale_get
* 
* TX pre-cursor 1 scale
*     0h: Half scale
*     1h: Full scale
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xaf_tx_pre1_scale_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xaf_tx_pre1_scale_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xaf_tx_pre1_scale_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xaf_tx_pre2_scale_get
* 
* TX pre-cursor 2 scale
*     0h: Half scale
*     1h: Full scale
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xaf_tx_pre2_scale_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xaf_tx_pre2_scale_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xaf_tx_pre2_scale_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xaf_rsvd75_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xaf_rsvd75_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0xb0_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xb0_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0xb0_tx_nrz_prbs_clk_en_get
* 
* For NRZ mode, enables the clock source to the PRBS generator.
*     0h:     PAM-4 mode
*     1h:     NRZ mode
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xb0_tx_nrz_prbs_clk_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xb0_tx_nrz_prbs_clk_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xb0_tx_nrz_prbs_clk_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xb0_rsvd76_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xb0_rsvd76_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0xb0_tx_nrz_prbs_gen_en_get
* 
* For NRZ mode, enables the TX PRBS generator logic.
*     0h:     Disabled
*     1h:     Enabled
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xb0_tx_nrz_prbs_gen_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xb0_tx_nrz_prbs_gen_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xb0_tx_nrz_prbs_gen_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xb0_tx_nrz_prbs_gerr_en_get
* 
* Setting this bit generates a single bit flip in the PRBS generated output. To send additional error(s), the bit must be cleared first.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xb0_tx_nrz_prbs_gerr_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xb0_tx_nrz_prbs_gerr_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xb0_tx_nrz_prbs_gerr_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xb0_rsvd77_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xb0_rsvd77_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_tx_0xb0_tx_nrz_mode_get
* 
* Mux control bit that selects the TX operating mode (PAM-4 or NRZ).
*     0h:     PAM-4 mode (Default)
*     1h:     NRZ mode
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xb0_tx_nrz_mode_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xb0_tx_nrz_mode_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xb0_tx_nrz_mode_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_tx_0xb0_tx_half_rate_en_get
* 
* For NRZ 10G mode, set this field to 1b.
* For NRZ 25G mode, leave this field at 0b. (Default)
*********************************************************************/
bf_status_t bf_ll_serdes_tx_0xb0_tx_half_rate_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xb0_tx_half_rate_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_tx_0xb0_tx_half_rate_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0xa_subrevision_get
* 
* Subrevision
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0xa_subrevision_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rsvd_0xb_revision_get
* 
* Revision
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0xb_revision_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rsvd_0xc_version_get
* 
* Version
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0xc_version_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rsvd_0xe_pu_acjtag_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0xe_pu_acjtag_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0xe_pu_acjtag_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0xe_pu_acjtag_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0xe_idac_acjtag_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0xe_idac_acjtag_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0xe_idac_acjtag_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0xe_idac_acjtag_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0xe_rsvd78_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0xe_rsvd78_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rsvd_0x1b_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x1b_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rsvd_0x1b_tx_pll_vco_range_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x1b_tx_pll_vco_range_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x1b_tx_pll_vco_range_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x1b_tx_pll_vco_range_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x1b_rsvd79_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x1b_rsvd79_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rsvd_0x29_tx_testmode_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x29_tx_testmode_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x29_tx_testmode_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x29_tx_testmode_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x29_tx_vtstgroup_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x29_tx_vtstgroup_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x29_tx_vtstgroup_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x29_tx_vtstgroup_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x29_rx_test_en_get
* 
* Enable TX block DC voltage test mode:
* 1h = Enable test mode
* 0h = Disable test mode
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x29_rx_test_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x29_rx_test_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x29_rx_test_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x29_rsvd80_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x29_rsvd80_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rsvd_0x2a_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x2a_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rsvd_0x2a_v1p3regvdrvbuf_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x2a_v1p3regvdrvbuf_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x2a_v1p3regvdrvbuf_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x2a_v1p3regvdrvbuf_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x2a_rsvd81_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x2a_rsvd81_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rsvd_0x2a_pu_himode_vddr_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x2a_pu_himode_vddr_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x2a_pu_himode_vddr_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x2a_pu_himode_vddr_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x2a_rsvd82_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x2a_rsvd82_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rsvd_0x2b_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x2b_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rsvd_0x2b_enextsel_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x2b_enextsel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x2b_enextsel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x2b_enextsel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x2b_rsvd83_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x2b_rsvd83_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rsvd_0x2d_edge1_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x2d_edge1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x2d_edge1_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x2d_edge1_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x2d_edge2_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x2d_edge2_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x2d_edge2_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x2d_edge2_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x2d_edge3_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x2d_edge3_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x2d_edge3_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x2d_edge3_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x2d_edge4_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x2d_edge4_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x2d_edge4_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x2d_edge4_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x2e_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x2e_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rsvd_0x2e_bypass1p3reg_adc_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x2e_bypass1p3reg_adc_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x2e_bypass1p3reg_adc_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x2e_bypass1p3reg_adc_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x2e_rsvd84_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x2e_rsvd84_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rsvd_0x30_eye_en_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x30_eye_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x30_eye_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x30_eye_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x30_rsvd85_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x30_rsvd85_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rsvd_0x32_idacx_eye_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x32_idacx_eye_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x32_idacx_eye_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x32_idacx_eye_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x32_idacy_eye_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x32_idacy_eye_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x32_idacy_eye_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x32_idacy_eye_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x32_idacz_eye_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x32_idacz_eye_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x32_idacz_eye_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x32_idacz_eye_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x33_ph_rotr_owen_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x33_ph_rotr_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x33_ph_rotr_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x33_ph_rotr_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x33_ph_rotr_ow_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x33_ph_rotr_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x33_ph_rotr_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x33_ph_rotr_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x33_masterless_mode_dis_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x33_masterless_mode_dis_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x33_masterless_mode_dis_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x33_masterless_mode_dis_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x33_ph_rotr_flip_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x33_ph_rotr_flip_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x33_ph_rotr_flip_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x33_ph_rotr_flip_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x33_rotr_dis_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x33_rotr_dis_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x33_rotr_dis_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x33_rotr_dis_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x33_refclk_mux_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x33_refclk_mux_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x33_refclk_mux_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x33_refclk_mux_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x33_tx_ph_rotr_flip_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x33_tx_ph_rotr_flip_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x33_tx_ph_rotr_flip_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x33_tx_ph_rotr_flip_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x33_rsvd86_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x33_rsvd86_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rsvd_0x34_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x34_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rsvd_0x34_freq_acc_top_owen_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x34_freq_acc_top_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x34_freq_acc_top_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x34_freq_acc_top_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x34_freq_acc_top_ow_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x34_freq_acc_top_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x34_freq_acc_top_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x34_freq_acc_top_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x34_tx_pi_en_get
* 
* Enable TX PLL using RX recovered clock as its reference clock.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x34_tx_pi_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x34_tx_pi_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x34_tx_pi_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x3a_pu_vdrv_ma_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x3a_pu_vdrv_ma_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x3a_pu_vdrv_ma_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x3a_pu_vdrv_ma_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x3a_rsvd87_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x3a_rsvd87_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rsvd_0x3a_tx_pll_testmode_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x3a_tx_pll_testmode_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x3a_tx_pll_testmode_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x3a_tx_pll_testmode_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x3a_tx_pll_vtstgroup_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x3a_tx_pll_vtstgroup_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x3a_tx_pll_vtstgroup_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x3a_tx_pll_vtstgroup_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x3a_tx_pll_est_en_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x3a_tx_pll_est_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x3a_tx_pll_est_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x3a_tx_pll_est_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x3a_rsvd88_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x3a_rsvd88_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rsvd_0x3d_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x3d_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rsvd_0x3d_tx_bypass_pllpmp_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x3d_tx_bypass_pllpmp_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x3d_tx_bypass_pllpmp_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x3d_tx_bypass_pllpmp_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x3d_tx_pll_bypass1p7reg_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x3d_tx_pll_bypass1p7reg_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x3d_tx_pll_bypass1p7reg_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x3d_tx_pll_bypass1p7reg_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x3d_rsvd89_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x3d_rsvd89_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rsvd_0x3e_tx_pll_n_get
* 
* PLL clock multiplier where:
*     PLL frequency = PLL reference clock * 2 * PLL_N. 
* The SerDes user NRZ rate is 2 * PLL frequency.
* The PAM-4 rate is 4 * PLL frequency.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x3e_tx_pll_n_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x3e_tx_pll_n_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x3e_tx_pll_n_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x3e_tx_charge_pump_cur_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x3e_tx_charge_pump_cur_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x3e_tx_charge_pump_cur_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x3e_tx_charge_pump_cur_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x3e_tx_vco_current_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x3e_tx_vco_current_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x3e_tx_vco_current_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x3e_tx_vco_current_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x3e_pu_tx_pll_get
* 
* Power up PLL when this field is set to 1.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x3e_pu_tx_pll_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x3e_pu_tx_pll_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x3e_pu_tx_pll_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x3f_tx_vbg_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x3f_tx_vbg_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x3f_tx_vbg_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x3f_tx_vbg_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x3f_pu_tx_bg_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x3f_pu_tx_bg_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x3f_pu_tx_bg_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x3f_pu_tx_bg_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rsvd_0x3f_rsvd90_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x3f_rsvd90_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rsvd_0x3f_tx_refclk_div4_en_get
* 
* Mux control to select the LVDS reference clock directly to the PLLs when this field is set to 0 or to a/4 version when this field is set to 1.
*********************************************************************/
bf_status_t bf_ll_serdes_rsvd_0x3f_tx_refclk_div4_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x3f_tx_refclk_div4_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rsvd_0x3f_tx_refclk_div4_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x0_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x0_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x0_dfe_init_1_val_get
* 
* DFE init 1 value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x0_dfe_init_1_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x0_dfe_init_1_val_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x0_dfe_init_1_val_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x1_delta_adapt_en_get
* 
* DFE parameter. For 25G, set this field to 01b. For 10G, set this field to 00b.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x1_delta_adapt_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x1_delta_adapt_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x1_delta_adapt_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x1_delta_min_get
* 
* Controls the low limit for delta accumulator.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x1_delta_min_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x1_delta_min_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x1_delta_min_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x1_tloop_const_get
* 
* Timing loop constant.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x1_tloop_const_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x1_tloop_const_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x1_tloop_const_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x2_target_ctrl_get
* 
* DFE adaptation target.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x2_target_ctrl_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x2_target_ctrl_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x2_target_ctrl_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x2_agc_period_get
* 
* AGC settling time.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x2_agc_period_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x2_agc_period_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x2_agc_period_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x3_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x3_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x3_rx_nrz_sd_thr_get
* 
* Signal detect threshold level of format U11.11. This is an unsigned number and each bit is equivalent to 0.4mV.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x3_rx_nrz_sd_thr_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x3_rx_nrz_sd_thr_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x3_rx_nrz_sd_thr_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x4_rx_sd_clk_cycles_get
* 
* Number of clock cycles that defines the signal detection window. The period is defined as 2^(N+6) UI, where N is a 4-bit value. Zero is not valid for N.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x4_rx_sd_clk_cycles_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x4_rx_sd_clk_cycles_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x4_rx_sd_clk_cycles_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x4_rx_sd_crossover_count_get
* 
* The count of the number of times the incoming signal must cross the signal detect threshold level within the signal detection window.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x4_rx_sd_crossover_count_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x4_rx_sd_crossover_count_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x4_rx_sd_crossover_count_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x5_timer4_get
* 
* Controls the link-up state machine timer. Leave this value as is. Do not change without consulting the Credo Application Team.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x5_timer4_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x5_timer4_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x5_timer4_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x6_itr_cnt4_get
* 
* Iteration count 4.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x6_itr_cnt4_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x6_itr_cnt4_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x6_itr_cnt4_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x6_rsvd91_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x6_rsvd91_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x7_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x7_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x7_dfe_init_2_val_get
* 
* DFE init 2 value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x7_dfe_init_2_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x7_dfe_init_2_val_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x7_dfe_init_2_val_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x8_itr_cnt5_get
* 
* Iteration count 5.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x8_itr_cnt5_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x8_itr_cnt5_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x8_itr_cnt5_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x8_meas_timer5_get
* 
* State machine timer 5.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x8_meas_timer5_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x8_meas_timer5_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x8_meas_timer5_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x8_meas_timer6_get
* 
* State machine timer 6. Controls eye margin sample size.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x8_meas_timer6_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x8_meas_timer6_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x8_meas_timer6_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x8_meas_timer7_get
* 
* State machine timer 7.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x8_meas_timer7_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x8_meas_timer7_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x8_meas_timer7_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x9_em_mode_get
* 
* Setting this bit freezes all adapted loops. Use during eye diagram measurement.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x9_em_mode_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x9_em_mode_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x9_em_mode_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x9_rsvd92_get
* 
* Reserved. Do not change the value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x9_rsvd92_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0xb_nrz_bp1_en_get
* 
* State machine breakpoint 1 enabled when set to 1. Set this bit to 0 and DOMAIN_RESET in register 980D to 0xAAA to stop firmware execution.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0xb_nrz_bp1_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0xb_nrz_bp1_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0xb_nrz_bp1_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0xb_rsvd93_get
* 
* Reserved. Do not change the value. The firmware sets the value of this field.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0xb_rsvd93_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0xb_nrz_bp1_state_get
* 
* Breakpoint 1 state.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0xb_nrz_bp1_state_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0xb_nrz_bp1_state_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0xb_nrz_bp1_state_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0xb_nrz_bp2_en_get
* 
* State machine breakpoint 2 enabled when set to 1.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0xb_nrz_bp2_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0xb_nrz_bp2_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0xb_nrz_bp2_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0xb_rx_dfe_over_en_get
* 
* When enabled, the DFE settings are programmed with the override values and no adaptation occurs. For use in testing only.
*     0h:     Override disabled
*     1h:     Override enabled
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0xb_rx_dfe_over_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0xb_rx_dfe_over_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0xb_rx_dfe_over_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0xb_rsvd94_get
* 
* Reserved. Do not change the value. The firmware sets the value of this field.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0xb_rsvd94_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0xb_nrz_bp2_state_get
* 
* Breakpoint 2 state.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0xb_nrz_bp2_state_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0xb_nrz_bp2_state_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0xb_nrz_bp2_state_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0xc_nrz_sm_cont_get
* 
* Resume state machine. Set this bit to 1h if the state machine is stopped using a breakpoint in register 010B.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0xc_nrz_sm_cont_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0xc_nrz_sm_cont_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0xc_nrz_sm_cont_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0xc_rsvd95_get
* 
* Reserved. Do not change the value. The firmware sets the value of this field.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0xc_rsvd95_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0xc_nrz_phy_ready_owen_get
* 
* Overwrite enable signal for PHY_READY:
*     0h:     Disable
*     1h:     Enable
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0xc_nrz_phy_ready_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0xc_nrz_phy_ready_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0xc_nrz_phy_ready_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0xc_nrz_phy_ready_ow_get
* 
* Overwrite value for PHY_READY.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0xc_nrz_phy_ready_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0xc_nrz_phy_ready_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0xc_nrz_phy_ready_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0xc_rsvd96_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0xc_rsvd96_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0xd_nrz_bp1_reached_get
* 
* Readout of breakpoint 1 reached indicator signal: 
*     1h:     breakpoint reached
*     0h:     breakpoint not reached
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0xd_nrz_bp1_reached_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0xd_nrz_bp2_reached_get
* 
* Readout of breakpoint 2 reached indicator signal: 
*     1h:     breakpoint reached
*     0h:     breakpoint not reached
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0xd_nrz_bp2_reached_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0xd_rsvd97_get
* 
* Reserved.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0xd_rsvd97_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0xd_nrz_current_state_get
* 
* Read back of current state number of the state machine. Note: due to crossing clock domains, this field should be firmware debounced.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0xd_nrz_current_state_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0xd_rsvd98_get
* 
* Reserved.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0xd_rsvd98_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0xd_delta_val_get
* 
* Read only, variable delta value.Note: due to crossing clock domains, this field should be firmware debounced.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0xd_delta_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x2a_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x2a_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x2a_rx_read_em_get
* 
* Read only, variable value of the read eye margin of the format S15.11.
* This is a signed number and each bit is equivalent to 0.4mV.
* Note: Due to crossing clock domains, this field should be firmware debounced.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x2a_rx_read_em_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x2b_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x2b_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x2b_rsvd99_get
* 
* Reserved. 
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x2b_rsvd99_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x2b_rsvd100_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x2b_rsvd100_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x2b_rx_dfe_tap_1_curr_val_get
* 
* Read only, variable value of the adapted DFE tap 1 value of the format S7.6.
* Each step size is ~13mV. 
* Note: due to crossing clock domains, this field should be firmware debounced.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x2b_rx_dfe_tap_1_curr_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x2c_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x2c_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x2c_rx_dfe_tap_2_curr_val_get
* 
* Read only, variable value of the adapted DFE tap 2 value of the format S7.6.
* Each step size is ~13mV.
* Note: due to crossing clock domains, this field should be firmware debounced.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x2c_rx_dfe_tap_2_curr_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x2c_rsvd101_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x2c_rsvd101_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x2c_rx_dfe_tap_3_curr_val_get
* 
* Read only, variable value of the adapted DFE tap 3 value of the format S7.6.
* Each step size is ~13mV.
* Note: due to crossing clock domains, this field should be firmware debounced.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x2c_rx_dfe_tap_3_curr_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x2e_rsvd_get
* 
* Reserved. 
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x2e_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x2e_rx_cdr_bw_curr_val_get
* 
* Read back value of the CDR bandwidth.
*     111b:         15.0Mhz
*     110b:         7.5Mhz
*     101b:         3.75Mhz
*        ...          ...       (Continue dividing by 2, round to 2 digits all the way to 000.)
*     000b:         0.0Mhz
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x2e_rx_cdr_bw_curr_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x2e_rsvd102_get
* 
* Reserved. 
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x2e_rsvd102_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x2e_rx_cdr_freq_curr_val_get
* 
* Read back value of the CDR second-order loop update rate.
*     11b:     Fastest
*     01b:     Slowest
*     00b:     Not updating
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x2e_rx_cdr_freq_curr_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x2e_rsvd103_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x2e_rsvd103_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x2e_rsvd104_get
* 
* Reserved. 
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x2e_rsvd104_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x2e_rx_sig_det_get
* 
* Status of the Signal Detect logic.
*     0h:     Signal not detected
*     1h:     Signal detected
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x2e_rx_sig_det_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x2e_rx_nrz_phy_ready_get
* 
* Status of the PHY.
*     0h:     PHY not ready
*     1h:     PHY ready
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x2e_rx_nrz_phy_ready_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x2e_rsvd105_get
* 
* Reserved. 
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x2e_rsvd105_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x3b_rx_cdr_over_bw_get
* 
* Enables override of automatic CDR bandwidth update.
*     0b:     Disable
*     1b:     Enable
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x3b_rx_cdr_over_bw_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x3b_rx_cdr_over_bw_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x3b_rx_cdr_over_bw_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x3b_rx_cdr_bw_over_val_get
* 
* CDR bandwidth override value.
*     111b:         15.00Mhz
*     110b:         7.50Mhz
*     101b:         3.75Mhz
*        ...          ...       (Continue dividing by 2, round to 2 digits all the way to 000.)
*     000b:         0.0Mhz
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x3b_rx_cdr_bw_over_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x3b_rx_cdr_bw_over_val_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x3b_rx_cdr_bw_over_val_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x3b_rsvd106_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x3b_rsvd106_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x3b_rx_cdr_over_ph_k_get
* 
* Enables timing loop phase accumulator constant override.
*     0b:     Disable
*     1b:     Enable
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x3b_rx_cdr_over_ph_k_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x3b_rx_cdr_over_ph_k_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x3b_rx_cdr_over_ph_k_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x3b_rx_cdr_over_ph_k_val_get
* 
* Timing loop phase accumulator constant override value.
*     00b:     Freeze phase accumulator update
*     01b:     Lowest constant applied to PH1/PH2/PH3 phase accumulators
*     ...
*     11b:     Highest constant applied to PH1/PH2/PH3 phase accumulators
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x3b_rx_cdr_over_ph_k_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x3b_rx_cdr_over_ph_k_val_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x3b_rx_cdr_over_ph_k_val_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x3b_rx_cdr_over_freq_get
* 
* Enables override of automatic CDR bandwidth updating.
*     0b:     Disable
*     1b:     Enable
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x3b_rx_cdr_over_freq_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x3b_rx_cdr_over_freq_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x3b_rx_cdr_over_freq_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x3b_rx_cdr_freq_over_val_get
* 
* CDR second-order loop update rate override value.
*     11b:     Fastest
*     01b:     Slowest
*     00b:     Not updating
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x3b_rx_cdr_freq_over_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x3b_rx_cdr_freq_over_val_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x3b_rx_cdr_freq_over_val_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x3b_rsvd107_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x3b_rsvd107_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x4d_rx_nrz_ctle_over_en_get
* 
* When enabled, the CTLE setting is programmed with the override values and no adaptation occurs. For use in testing only.
*     0h:     Override disabled
*     1h:     Override enabled
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x4d_rx_nrz_ctle_over_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x4d_rx_nrz_ctle_over_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x4d_rx_nrz_ctle_over_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x4d_rsvd108_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x4d_rsvd108_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x4e_rsvd_get
* 
* Reserved. 
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x4e_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x4e_rx_nrz_ctle_over_val_get
* 
* CTLE value written when CTLE override is enabled.
*     0h:         22.9 dB
*     1h:         20.4 dB
*     2h:         17.4 dB
*     3h:         15.1 dB
*     4h:         13.0 dB
*     5h:         10.6 dB
*     6h:         7.5 dB
*     7h:         2.5 dB
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x4e_rx_nrz_ctle_over_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x4e_rx_nrz_ctle_over_val_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x4e_rx_nrz_ctle_over_val_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x4e_rsvd109_get
* 
* Reserved. 
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x4e_rsvd109_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x4f_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x4f_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x4f_em_search_param_get
* 
* Eye margin search parameter.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x4f_em_search_param_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x4f_em_search_param_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x4f_em_search_param_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x50_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x50_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x50_em_thr_get
* 
* Eye margin failure threshold.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x50_em_thr_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x50_em_thr_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x50_em_thr_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x5d_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x5d_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x5d_rx_dfe_tap_1_val_get
* 
* DFE tap 1 value of format S7.6 written when DFE override is enabled.
* Each step size is ~13mV.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x5d_rx_dfe_tap_1_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x5d_rx_dfe_tap_1_val_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x5d_rx_dfe_tap_1_val_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x5d_rsvd110_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x5d_rsvd110_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x5d_rx_dfe_tap_2_val_get
* 
* DFE tap 2 value of format S7.6 written when DFE override is enabled.
* Each step size is ~13mV.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x5d_rx_dfe_tap_2_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x5d_rx_dfe_tap_2_val_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x5d_rx_dfe_tap_2_val_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x5e_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x5e_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x5e_rx_dfe_tap_3_val_get
* 
* DFE tap 3 value of format S7.6 written when DFE override is enabled.
* Each step size is ~13mV.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x5e_rx_dfe_tap_3_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x5e_rx_dfe_tap_3_val_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x5e_rx_dfe_tap_3_val_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x5e_rsvd111_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x5e_rsvd111_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x61_rx_prbs_count_reset_get
* 
* Resets the RX PRBS error count. User needs to set, then clear this bit to resume accumulation.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x61_rx_prbs_count_reset_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x61_rx_prbs_count_reset_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x61_rx_prbs_count_reset_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x61_rx_pol_flip_get
* 
* Polarity of the incoming RX data.
*     0h:     Normal
*     1h:     Inverted
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x61_rx_pol_flip_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x61_rx_pol_flip_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x61_rx_pol_flip_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x61_rx_prbs_mode_get
* 
* PRBS mode select of the RX PRBS checker.
*     0h:     PRBS9
*     1h:     PRBS15
*     2h:     PRBS23
*     3h:     PRBS31
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x61_rx_prbs_mode_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x61_rx_prbs_mode_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x61_rx_prbs_mode_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x61_rsvd112_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x61_rsvd112_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x61_rx_prbs_check_en_get
* 
* Enables the RX PRBS checker logic.
*     0h:     Disabled
*     1h:     Enabled
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x61_rx_prbs_check_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x61_rx_prbs_check_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x61_rx_prbs_check_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x61_rsvd113_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x61_rsvd113_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x61_ph1_over_en_get
* 
* Phase 1 overwrite enable with value in PH1_VAL.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x61_ph1_over_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x61_ph1_over_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x61_ph1_over_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x61_ph1_val_get
* 
* Phase 1 interpolator value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x61_ph1_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x61_ph1_val_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x61_ph1_val_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x62_ph2_over_en_get
* 
* Enables Phase 2 overwrite with value in PH2_VAL.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x62_ph2_over_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x62_ph2_over_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x62_ph2_over_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x62_ph2_val_get
* 
* Overwrite Phase 2 value in two's complement format (-64 to +63).
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x62_ph2_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x62_ph2_val_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x62_ph2_val_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x62_ph3_over_en_get
* 
* Enables Phase 3 overwrite with value in PH3_VAL.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x62_ph3_over_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x62_ph3_over_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x62_ph3_over_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x62_ph3_val_get
* 
* Overwrite Phase 3 value in two's complement format (-64 to +63).
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x62_ph3_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x62_ph3_val_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x62_ph3_val_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x63_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x63_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x63_ph1_pol_flip_get
* 
* Phase 1 interpolator flip.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x63_ph1_pol_flip_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x63_ph1_pol_flip_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x63_ph1_pol_flip_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x63_ph1_gray_val_get
* 
* Overwrite Phase 1 values in gray code format (0 to 127).
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x63_ph1_gray_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x63_ph1_gray_val_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x63_ph1_gray_val_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x64_ph2_pol_flip_get
* 
* Phase 2 interpolator flip.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x64_ph2_pol_flip_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x64_ph2_pol_flip_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x64_ph2_pol_flip_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x64_ph2_gray_val_get
* 
* Overwrite Phase 2 values in gray code format (0 to 127).
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x64_ph2_gray_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x64_ph2_gray_val_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x64_ph2_gray_val_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x64_ph3_pol_flip_get
* 
* Phase 3 interpolator flip.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x64_ph3_pol_flip_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x64_ph3_pol_flip_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x64_ph3_pol_flip_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x64_ph3_gray_val_get
* 
* Overwrite Phase 3 values in gray code format (0 to 127).
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x64_ph3_gray_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x64_ph3_gray_val_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x64_ph3_gray_val_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x65_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x65_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x65_ph1_tloop_mode_get
* 
* Controls phase 1 timing update mode.
* For short reach, set this to 0 (recommended).
* For long reach, set this to 1 (recommended).
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x65_ph1_tloop_mode_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x65_ph1_tloop_mode_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x65_ph1_tloop_mode_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x65_ph2_tloop_mode_get
* 
* Controls phase 2 timing update mode.
* For short reach, set this to 0 (recommended).
* For long reach, set this to 1 (recommended).
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x65_ph2_tloop_mode_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x65_ph2_tloop_mode_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x65_ph2_tloop_mode_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x65_ph3_tloop_mode_get
* 
* Controls phase 3 timing update mode.
* For short reach, set this to 0 (recommended).
* For long reach, set this to 1 (recommended).
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x65_ph3_tloop_mode_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x65_ph3_tloop_mode_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x65_ph3_tloop_mode_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x65_rsvd114_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x65_rsvd114_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x66_rx_nrz_prbs_read_err_high_get
* 
* Read only, variable value of the upper word of the RX PRBS 32-bit error count.
* Note: due to crossing clock domains, this field should be firmware debounced.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x66_rx_nrz_prbs_read_err_high_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x67_rx_nrz_prbs_read_err_low_get
* 
* Read only, variable value of the lower word of the RX PRBS 32-bit error count.
* Note: due to crossing clock domains, this field should be firmware debounced.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x67_rx_nrz_prbs_read_err_low_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x6e_rsvd_get
* 
* Reserved.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x6e_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x6e_read_ph1_gray_val_get
* 
* Read only, variable value for phase 1 in gray code format (0 to 127).
* Note: due to crossing clock domains, this field should be firmware debounced.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x6e_read_ph1_gray_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x6f_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x6f_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x6f_read_ph2_gray_val_get
* 
* Read only, variable value for phase 2 in gray code format (0 to 127).
* Note: due to crossing clock domains, this field should be firmware debounced.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x6f_read_ph2_gray_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x6f_rsvd115_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x6f_rsvd115_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x6f_read_ph3_gray_val_get
* 
* Read only, variable value for phase 3 in gray code format (0 to 127).
* Note: due to crossing clock domains, this field should be firmware debounced.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x6f_read_ph3_gray_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x71_rsvd_get
* 
* Reserved.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x71_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x71_read_ph1_val_get
* 
* Read only, variable Phase 1 interpolator value in two's complement format (-64 to +63).Note: due to crossing clock domains, this field should be firmware debounced.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x71_read_ph1_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x71_rsvd116_get
* 
* Reserved. 
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x71_rsvd116_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x71_read_ph2_val_get
* 
* Read only, variable Phase 2 interpolator value in two's complement format (-64 to +63).
* Note: due to crossing clock domains, this field should be firmware debounced.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x71_read_ph2_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x72_rsvd_get
* 
* Reserved.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x72_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x72_read_ph3_val_get
* 
* Read only, variable Phase 3 interpolator value in two's complement format (-64 to +63).
* Note: due to crossing clock domains, this field should be firmware debounced.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x72_read_ph3_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x72_rsvd117_get
* 
* Reserved.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x72_rsvd117_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x73_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x73_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x73_rx_read_freq_err_get
* 
* Read value of the RX frequency accumulator of format S11.11. Each bit represents ~0.5ppm offset error between the incoming traffic and the PLL.
* Note: due to crossing clock domains, this field should be firmware debounced.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x73_rx_read_freq_err_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x75_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x75_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x75_tloop_ctrl1_get
* 
* Timing control parameters, group 1.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x75_tloop_ctrl1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x75_tloop_ctrl1_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x75_tloop_ctrl1_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x75_tloop_ctrl2_get
* 
* Timing control parameters, group 2.
* For 25G, set this field to 1111b.
* For 10G, set this field to 1010b.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x75_tloop_ctrl2_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x75_tloop_ctrl2_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x75_tloop_ctrl2_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x75_tloop_ctrl3_get
* 
* Timing control parameters, group 3.
* For 25G, set this field to 1111b.
* For 10G, set this field to 1010b.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x75_tloop_ctrl3_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x75_tloop_ctrl3_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x75_tloop_ctrl3_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x76_ctle_map_table_3_get
* 
* CTLE control table 3. Consult the Credo Application Team for recommended values. 
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x76_ctle_map_table_3_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x76_ctle_map_table_3_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x76_ctle_map_table_3_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x77_ctle_map_table_2_get
* 
* CTLE control table 2. Consult the Credo Application Team for recommended values. 
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x77_ctle_map_table_2_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x77_ctle_map_table_2_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x77_ctle_map_table_2_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x78_ctle_map_table_1_get
* 
* CTLE control table 1. Consult the Credo Application Team for recommended values. 
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x78_ctle_map_table_1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x78_ctle_map_table_1_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x78_ctle_map_table_1_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x79_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x79_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x79_rx_sub_rate_mode_get
* 
* RX NRZ sub-rate mode when RX_HALF_RATE_EN is enabled. When RX_HALF_RATE_EN is disabled the values do not have any effect.
*     0h:     Half-rate (default)
*     1h:     Quarter-rate
*     2h:     Eighth-rate
*     3h:     Sixteenth-rate
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x79_rx_sub_rate_mode_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x79_rx_sub_rate_mode_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x79_rx_sub_rate_mode_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x79_rx_25g_en_get
* 
* Selects the 25G timing mode:
*     0h:     Standard 25G timing mode (default)
*     1h:     BB 25G timing mode 
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x79_rx_25g_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x79_rx_25g_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x79_rx_25g_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x79_rx_nrz_mode_en_get
* 
* Enables RX NRZ mode when set to 1.
*     0b    Shuts down all the RX NRZ clocks (default)
*     1b    Enables NRZ mode. 
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x79_rx_nrz_mode_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x79_rx_nrz_mode_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x79_rx_nrz_mode_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x79_rsvd118_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x79_rsvd118_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x79_rx_half_rate_en_get
* 
* Enables RX NRZ half-rate mode.
*     0h:     Full-rate (default)
*     1h:     Half-rate
* For 10G, set this field to 1b.
* For 25G, leave this field at 0b. 
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x79_rx_half_rate_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x79_rx_half_rate_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x79_rx_half_rate_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x7d_nrz_bp3_en_get
* 
* Enable signal for breakpoint 3
*     1h:     breakpoint function enabled
*     0h:     function disabled
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x7d_nrz_bp3_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x7d_nrz_bp3_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x7d_nrz_bp3_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x7d_rsvd119_get
* 
* Reserved. Do not change the default value. Reserved. Do not change the default value. 
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x7d_rsvd119_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x7d_nrz_bp3_state_get
* 
* Breakpoint 3 for the state machine 
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x7d_nrz_bp3_state_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x7d_nrz_bp3_state_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x7d_nrz_bp3_state_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x7d_nrz_bp4_en_get
* 
* Enable signal for breakpoint 4
*     1h:     breakpoint function enabled
*     0h:     function disabled
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x7d_nrz_bp4_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x7d_nrz_bp4_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x7d_nrz_bp4_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x7d_rsvd120_get
* 
* Reserved. Do not change the default value. 
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x7d_rsvd120_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x7d_nrz_bp4_state_get
* 
* Breakpoint 4 for the state machine 
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x7d_nrz_bp4_state_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x7d_nrz_bp4_state_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x7d_nrz_bp4_state_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x7e_nrz_bp5_en_get
* 
* Enable signal for breakpoint 5
*     1h:     breakpoint function enabled
*     0h:     function disabled
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x7e_nrz_bp5_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x7e_nrz_bp5_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x7e_nrz_bp5_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x7e_rsvd121_get
* 
* Reserved. Do not change the default value. 
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x7e_rsvd121_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x7e_nrz_bp5_state_get
* 
* Breakpoint 5 for the state machine 
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x7e_nrz_bp5_state_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x7e_nrz_bp5_state_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x7e_nrz_bp5_state_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x7e_nrz_bp6_en_get
* 
* Enable signal for breakpoint 6
*     1h:     breakpoint function enabled
*     0h:     function disabled
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x7e_nrz_bp6_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x7e_nrz_bp6_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x7e_nrz_bp6_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x7e_rsvd122_get
* 
* Reserved. Do not change the default value. 
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x7e_rsvd122_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x7e_nrz_bp6_state_get
* 
* Breakpoint 6 for the state machine 
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x7e_nrz_bp6_state_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x7e_nrz_bp6_state_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x7e_nrz_bp6_state_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x7f_dac_sel_get
* 
* RX DAC value corresponding to input amplitude. Read-only.
*     1111b:             ~850mV
*        ...          ...       (Step size ~50mV)
*     0000b:         ~    100mV
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x7f_dac_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x7f_ctle_read_curr_val_get
* 
* Valid only when register 0n014D[15] is set to 0.Read only value of the current setting of the CTLE.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x7f_ctle_read_curr_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x7f_rsvd123_get
* 
* Reserved.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x7f_rsvd123_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x80_delta_max_get
* 
* Delta Max for new CTLE-DFE.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x80_delta_max_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x80_delta_max_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x80_delta_max_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x80_rsvd124_get
* 
* Reserved. Do not change
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x80_rsvd124_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x81_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x81_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x81_nrz_sm_reset_get
* 
* To reset the state machine and start the optimization and adaptation process over, program to 1 and then clear.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x81_nrz_sm_reset_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x81_nrz_sm_reset_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x81_nrz_sm_reset_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x81_nrz_freq_init3_get
* 
* Initial timing 3 value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x81_nrz_freq_init3_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x81_nrz_freq_init3_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x81_nrz_freq_init3_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x82_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x82_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x82_nrz_freq_init4_get
* 
* Initial timing 4 value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x82_nrz_freq_init4_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x82_nrz_freq_init4_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x82_nrz_freq_init4_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x83_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x83_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x83_dfe_init_3_val_get
* 
* DFE init 3 value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x83_dfe_init_3_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x83_dfe_init_3_val_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x83_dfe_init_3_val_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x84_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x84_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x84_dfe_init_4_val_get
* 
* DFE init 4 value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x84_dfe_init_4_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x84_dfe_init_4_val_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x84_dfe_init_4_val_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x85_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x85_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x85_dfe_init_5_val_get
* 
* DFE init 5 value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x85_dfe_init_5_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x85_dfe_init_5_val_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x85_dfe_init_5_val_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x86_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x86_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x86_dfe_init_6_val_get
* 
* DFE init 6 value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x86_dfe_init_6_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x86_dfe_init_6_val_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x86_dfe_init_6_val_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x87_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x87_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x87_dfe_init_7_val_get
* 
* DFE init 7 value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x87_dfe_init_7_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x87_dfe_init_7_val_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x87_dfe_init_7_val_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_rx_0x88_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x88_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_rx_0x88_dfe_init_8_val_get
* 
* DFE init 8 value.
*********************************************************************/
bf_status_t bf_ll_serdes_rx_0x88_dfe_init_8_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x88_dfe_init_8_val_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_rx_0x88_dfe_init_8_val_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x0_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x0_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x0_sym_size_get
* 
* FEC symbol size
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x0_sym_size_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x0_sym_size_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x0_sym_size_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x1_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x1_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x1_cnt_clr_get
* 
* Clear all the counters
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x1_cnt_clr_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x1_cnt_clr_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x1_cnt_clr_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x1_cnt_freeze_get
* 
* Freeze all the counters
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x1_cnt_freeze_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x1_cnt_freeze_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x1_cnt_freeze_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x1_fec_clk_en_get
* 
* Top clock enable, active high
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x1_fec_clk_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x1_fec_clk_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x1_fec_clk_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x1_fec_ana_en_get
* 
* FEC analyzer enable, active high
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x1_fec_ana_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x1_fec_ana_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x1_fec_ana_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x2_mem1_raddr_get
* 
* Read address of memory
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x2_mem1_raddr_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x2_mem1_raddr_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x2_mem1_raddr_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x4_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x4_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x4_fram_size_get
* 
* N: FEC frame size (number of bits)
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x4_fram_size_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x4_fram_size_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x4_fram_size_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x5_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x5_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x5_reset_his_get
* 
* Reset the histogram counters
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x5_reset_his_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x5_reset_his_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x5_reset_his_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x5_corr_size_get
* 
* T:FEC error correction capability
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x5_corr_size_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x5_corr_size_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x5_corr_size_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x5_th_size_get
* 
* Memory trigger threshold
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x5_th_size_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x5_th_size_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x5_th_size_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x5_n_his_get
* 
* Group selection i(0-6) of error histogram
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x5_n_his_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x5_n_his_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x5_n_his_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x6_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x6_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x6_fec_clk_sel_get
* 
* Top clock source selection
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x6_fec_clk_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x6_fec_clk_sel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x6_fec_clk_sel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x6_clear_trigger_get
* 
* Clear the memory trigger status
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x6_clear_trigger_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x6_clear_trigger_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x6_clear_trigger_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x6_mem2_wr_en_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x6_mem2_wr_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x6_mem2_wr_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x6_mem2_wr_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x6_mem1_wr_en_get
* 
* Write enable of memory
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x6_mem1_wr_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x6_mem1_wr_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x6_mem1_wr_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x6_set_mem_wren_get
* 
* Enable of manually write of memory
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x6_set_mem_wren_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x6_set_mem_wren_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x6_set_mem_wren_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x7_read_data_get
* 
* Readout from register
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x7_read_data_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x8_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x8_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x8_mem1_sel_get
* 
* selection of data source of memory, 1:debug signal, 0: golden data
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x8_mem1_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x8_mem1_sel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x8_mem1_sel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x8_debug_data_sel_get
* 
* Select data type for Error debug mode
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x8_debug_data_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x8_debug_data_sel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x8_debug_data_sel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x8_prbs_mode_get
* 
* PRBS mode selection
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x8_prbs_mode_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x8_prbs_mode_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x8_prbs_mode_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x8_opr_mode_get
* 
* For symbol error counter, 00:M consecutive bits, 01:M even bits, 10:M odd bits 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x8_opr_mode_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x8_opr_mode_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x8_opr_mode_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x8_mem2_data_sel_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x8_mem2_data_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x8_mem2_data_sel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x8_mem2_data_sel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x9_prbs_chk_cntr_reset_get
* 
* PRBS sync error counter reset signal.
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x9_prbs_chk_cntr_reset_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x9_prbs_chk_cntr_reset_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x9_prbs_chk_cntr_reset_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x9_rx_prbs_force_reload_get
* 
* Force reload of the PRBS sync checker's shift registers: edge triggered
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x9_rx_prbs_force_reload_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x9_rx_prbs_force_reload_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x9_rx_prbs_force_reload_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x9_rx_prbs_auto_sync_en_get
* 
* Enable the auto sync function
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x9_rx_prbs_auto_sync_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x9_rx_prbs_auto_sync_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x9_rx_prbs_auto_sync_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x9_prbs_mismatch_thr_get
* 
* PRBS mismatch threshold
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x9_prbs_mismatch_thr_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x9_prbs_mismatch_thr_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x9_prbs_mismatch_thr_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x9_prbs_sync_thr_get
* 
* PRBS sync threshold
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x9_prbs_sync_thr_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x9_prbs_sync_thr_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x9_prbs_sync_thr_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0xa_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0xa_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0xa_prbs_sync_loss_thr_get
* 
* PRBS sync loss threshold
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0xa_prbs_sync_loss_thr_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0xa_prbs_sync_loss_thr_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0xa_prbs_sync_loss_thr_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0xb_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0xb_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0xb_ctrl_tei_get
* 
* TEI mode selection:
*     00b: TEI add 1
*     01b: TEI add SEI
*     10b: TEI add BEI
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0xb_ctrl_tei_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0xb_ctrl_tei_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0xb_ctrl_tei_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0xc_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0xc_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0xc_ctrl_teo_get
* 
* TEO mode selection:
*     00b: TEO add 1
*     01b: TEO add SEO
*     10b: TEO add BEI
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0xc_ctrl_teo_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0xc_ctrl_teo_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0xc_ctrl_teo_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0xd_read_sel_get
* 
* The data source selection for read. For more information, see the multiplexing section of the device specification.
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0xd_read_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0xd_read_sel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0xd_read_sel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x1d_degendl9_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x1d_degendl9_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x1d_degendl9_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x1d_degendl9_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x1d_rsvd125_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x1d_rsvd125_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x1d_skef_en_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x1d_skef_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x1d_skef_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x1d_skef_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x1d_skef_val_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x1d_skef_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x1d_skef_val_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x1d_skef_val_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x1d_skef_addcap_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x1d_skef_addcap_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x1d_skef_addcap_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x1d_skef_addcap_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x1d_pu_intp_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x1d_pu_intp_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x1d_pu_intp_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x1d_pu_intp_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x1d_rsvd126_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x1d_rsvd126_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x1e_degendl5_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x1e_degendl5_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x1e_degendl5_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x1e_degendl5_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x1e_degendl6_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x1e_degendl6_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x1e_degendl6_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x1e_degendl6_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x1e_degendl7_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x1e_degendl7_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x1e_degendl7_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x1e_degendl7_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x1e_degendl8_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x1e_degendl8_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x1e_degendl8_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x1e_degendl8_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x1f_degendl1_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x1f_degendl1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x1f_degendl1_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x1f_degendl1_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x1f_degendl2_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x1f_degendl2_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x1f_degendl2_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x1f_degendl2_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x1f_degendl3_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x1f_degendl3_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x1f_degendl3_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x1f_degendl3_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x1f_degendl4_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x1f_degendl4_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x1f_degendl4_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x1f_degendl4_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x20_pu_degenmain_sum1_msb_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x20_pu_degenmain_sum1_msb_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x20_pu_degenmain_sum1_msb_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x20_pu_degenmain_sum1_msb_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x20_pu_degenmain_sum1_lsb_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x20_pu_degenmain_sum1_lsb_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x20_pu_degenmain_sum1_lsb_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x20_pu_degenmain_sum1_lsb_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x20_pu_degenmain_sum2_msb_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x20_pu_degenmain_sum2_msb_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x20_pu_degenmain_sum2_msb_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x20_pu_degenmain_sum2_msb_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x20_pu_degenmain_sum2_lsb_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x20_pu_degenmain_sum2_lsb_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x20_pu_degenmain_sum2_lsb_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x20_pu_degenmain_sum2_lsb_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x20_pu_degenmain_sum3_msb_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x20_pu_degenmain_sum3_msb_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x20_pu_degenmain_sum3_msb_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x20_pu_degenmain_sum3_msb_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x20_pu_degenmain_sum3_lsb_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x20_pu_degenmain_sum3_lsb_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x20_pu_degenmain_sum3_lsb_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x20_pu_degenmain_sum3_lsb_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x20_pol_mux1_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x20_pol_mux1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x20_pol_mux1_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x20_pol_mux1_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x20_pol_mux2_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x20_pol_mux2_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x20_pol_mux2_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x20_pol_mux2_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x20_pol_mux3_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x20_pol_mux3_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x20_pol_mux3_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x20_pol_mux3_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x20_pol_mux4_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x20_pol_mux4_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x20_pol_mux4_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x20_pol_mux4_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x20_rsvd127_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x20_rsvd127_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x21_degenmain_sum4_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x21_degenmain_sum4_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x21_degenmain_sum4_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x21_degenmain_sum4_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x21_rsvd128_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x21_rsvd128_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x21_degensum_sum4_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x21_degensum_sum4_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x21_degensum_sum4_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x21_degensum_sum4_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x21_rsvd129_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x21_rsvd129_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x22_degenmain1_sum3_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x22_degenmain1_sum3_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x22_degenmain1_sum3_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x22_degenmain1_sum3_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x22_degenmain0_sum3_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x22_degenmain0_sum3_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x22_degenmain0_sum3_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x22_degenmain0_sum3_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x22_degensum1_sum3_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x22_degensum1_sum3_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x22_degensum1_sum3_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x22_degensum1_sum3_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x22_degensum0_sum3_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x22_degensum0_sum3_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x22_degensum0_sum3_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x22_degensum0_sum3_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x23_degenmain1_sum2_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x23_degenmain1_sum2_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x23_degenmain1_sum2_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x23_degenmain1_sum2_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x23_degenmain0_sum2_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x23_degenmain0_sum2_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x23_degenmain0_sum2_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x23_degenmain0_sum2_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x23_degensum1_sum2_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x23_degensum1_sum2_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x23_degensum1_sum2_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x23_degensum1_sum2_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x23_degensum0_sum3_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x23_degensum0_sum3_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x23_degensum0_sum3_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x23_degensum0_sum3_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x24_degenmain1_sum1_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x24_degenmain1_sum1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x24_degenmain1_sum1_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x24_degenmain1_sum1_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x24_degenmain0_sum1_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x24_degenmain0_sum1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x24_degenmain0_sum1_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x24_degenmain0_sum1_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x24_degensum1_sum1_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x24_degensum1_sum1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x24_degensum1_sum1_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x24_degensum1_sum1_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x24_degensum0_sum1_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x24_degensum0_sum1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x24_degensum0_sum1_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x24_degensum0_sum1_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x27_pu_rx_agc_ln_get
* 
* Power up RX AGC for this lane.
*     0h:     Power down
*     1h:     Power up
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x27_pu_rx_agc_ln_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x27_pu_rx_agc_ln_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x27_pu_rx_agc_ln_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x27_pu_agcdl_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x27_pu_agcdl_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x27_pu_agcdl_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x27_pu_agcdl_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x27_rsvd130_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x27_rsvd130_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x27_rx_ac_couple_en_get
* 
* Enables RX AC coupling.
*     0h:     DC coupling
*     1h:     AC coupling
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x27_rx_ac_couple_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x27_rx_ac_couple_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x27_rx_ac_couple_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x27_rx_ctle_cm1_get
* 
* CTLE Common Mode voltage 1
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x27_rx_ctle_cm1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x27_rx_ctle_cm1_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x27_rx_ctle_cm1_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x27_rx_ctle_cm2_get
* 
* CTLE Common Mode voltage 2
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x27_rx_ctle_cm2_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x27_rx_ctle_cm2_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x27_rx_ctle_cm2_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x27_rsvd131_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x27_rsvd131_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x2b_rx_acal_bm_vco_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x2b_rx_acal_bm_vco_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x2b_rx_acal_done_c_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x2b_rx_acal_done_c_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x2b_rx_acal_up_dn_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x2b_rx_acal_up_dn_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x2b_rsvd132_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x2b_rsvd132_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x2c_rx_acal_rg_adc_reg4_s_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x2c_rx_acal_rg_adc_reg4_s_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x2c_rx_acal_rg_adc_reg4_s_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x2c_rx_acal_rg_adc_reg4_s_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x2d_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x2d_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x2d_rx_acal_rg_adc_wait_s_sel_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x2d_rx_acal_rg_adc_wait_s_sel_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x2d_rx_acal_rg_adc_wait_s_sel_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x2d_rx_acal_rg_adc_wait_s_sel_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x2d_rx_acal_over_data_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x2d_rx_acal_over_data_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x2d_rx_acal_over_data_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x2d_rx_acal_over_data_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x2d_rx_acal_bm_vco_owen_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x2d_rx_acal_bm_vco_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x2d_rx_acal_bm_vco_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x2d_rx_acal_bm_vco_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x2d_rsvd133_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x2d_rsvd133_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x33_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x33_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x33_pu_rx_pll_intp_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x33_pu_rx_pll_intp_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x33_pu_rx_pll_intp_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x33_pu_rx_pll_intp_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x33_rsvd134_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x33_rsvd134_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x34_rx_refclk_div2_en_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x34_rx_refclk_div2_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x34_rx_refclk_div2_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x34_rx_refclk_div2_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x34_rsvd135_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x34_rsvd135_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x35_rx_pll_vco_range_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x35_rx_pll_vco_range_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x35_rx_pll_vco_range_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x35_rx_pll_vco_range_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x35_rsvd136_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x35_rsvd136_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x37_rx_pll_testmode_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x37_rx_pll_testmode_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x37_rx_pll_testmode_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x37_rx_pll_testmode_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x37_rx_pll_vtstgroup_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x37_rx_pll_vtstgroup_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x37_rx_pll_vtstgroup_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x37_rx_pll_vtstgroup_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x37_rx_pll_test_en_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x37_rx_pll_test_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x37_rx_pll_test_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x37_rx_pll_test_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x37_rsvd137_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x37_rsvd137_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x38_pu_adc_master_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x38_pu_adc_master_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x38_pu_adc_master_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x38_pu_adc_master_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x38_rsvd138_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x38_rsvd138_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x3c_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x3c_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x3c_rx_enclkloss_master_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x3c_rx_enclkloss_master_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x3c_rx_enclkloss_master_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x3c_rx_enclkloss_master_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x3c_rsvd139_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x3c_rsvd139_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x3d_rx_pll_n_get
* 
* PLL clock multiplier where:
*     PLL frequency = PLL reference clock * 2 * PLL_N.
* The SerDes user NRZ rate is 2 * PLL frequency.
* The PAM4 rate is 4 * PLL frequency.
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x3d_rx_pll_n_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x3d_rx_pll_n_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x3d_rx_pll_n_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x3d_rsvd140_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x3d_rsvd140_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x3d_rx_vco_current_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x3d_rx_vco_current_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x3d_rx_vco_current_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x3d_rx_vco_current_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x3d_pu_rx_pll_get
* 
* Power up PLL when this field is set to 1.
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x3d_pu_rx_pll_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x3d_pu_rx_pll_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x3d_pu_rx_pll_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x3e_pu_agc_1_master_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x3e_pu_agc_1_master_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x3e_pu_agc_1_master_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x3e_pu_agc_1_master_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x3e_pu_agcdl_master_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x3e_pu_agcdl_master_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x3e_pu_agcdl_master_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x3e_pu_agcdl_master_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x3e_rsvd141_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x3e_rsvd141_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x3f_rx_vbg_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x3f_rx_vbg_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x3f_rx_vbg_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x3f_rx_vbg_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x3f_pu_rx_bg_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x3f_pu_rx_bg_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x3f_pu_rx_bg_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x3f_pu_rx_bg_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x3f_pu_rx_rvdd_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x3f_pu_rx_rvdd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x3f_pu_rx_rvdd_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x3f_pu_rx_rvdd_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x3f_rx_rvddvco_en_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x3f_rx_rvddvco_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x3f_rx_rvddvco_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x3f_rx_rvddvco_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x3f_rsvd142_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x3f_rsvd142_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x3f_rx_refclk_div4_en_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x3f_rx_refclk_div4_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x3f_rx_refclk_div4_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_fec_analyzer_0x3f_rx_refclk_div4_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_fec_analyzer_0x3f_rsvd143_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_fec_analyzer_0x3f_rsvd143_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_lane_slice_0x0_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_lane_slice_0x0_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_lane_slice_0x0_rx_to_tx_lpbk_get
* 
* SerDes RX data to TX.
*********************************************************************/
bf_status_t bf_ll_serdes_lane_slice_0x0_rx_to_tx_lpbk_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_lane_slice_0x0_rx_to_tx_lpbk_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_lane_slice_0x0_rx_to_tx_lpbk_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_lane_slice_0x0_tx_to_rx_lpbk_get
* 
* TX data to RX directly, do not pass PHOENIX3
*********************************************************************/
bf_status_t bf_ll_serdes_lane_slice_0x0_tx_to_rx_lpbk_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_lane_slice_0x0_tx_to_rx_lpbk_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_lane_slice_0x0_tx_to_rx_lpbk_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_lane_slice_0x0_rsvd144_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_lane_slice_0x0_rsvd144_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_lane_slice_0x2_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_lane_slice_0x2_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_lane_slice_0x2_ubump_tx_to_rx_loop_get
* 
* TX clock to RX clock directly for clk loopback
*********************************************************************/
bf_status_t bf_ll_serdes_lane_slice_0x2_ubump_tx_to_rx_loop_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_lane_slice_0x2_ubump_tx_to_rx_loop_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_lane_slice_0x2_ubump_tx_to_rx_loop_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_lane_slice_0x2_rx_prbs_clr_cntr_s_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_lane_slice_0x2_rx_prbs_clr_cntr_s_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_lane_slice_0x2_rx_prbs_clr_cntr_s_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_lane_slice_0x2_rx_prbs_clr_cntr_s_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_lane_slice_0x2_rx_prbs_s_en_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_lane_slice_0x2_rx_prbs_s_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_lane_slice_0x2_rx_prbs_s_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_lane_slice_0x2_rx_prbs_s_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_lane_slice_0x2_tx_prbs_s_en_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_lane_slice_0x2_tx_prbs_s_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_lane_slice_0x2_tx_prbs_s_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_lane_slice_0x2_tx_prbs_s_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_lane_slice_0x3_prbs_err_cntr_15_0_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_lane_slice_0x3_prbs_err_cntr_15_0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_lane_slice_0x4_prbs_err_cntr_31_16_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_lane_slice_0x4_prbs_err_cntr_31_16_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_lane_slice_0x5_prbs_err_cntr_47_32_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_lane_slice_0x5_prbs_err_cntr_47_32_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_lane_slice_0x6_lane_num_get
* 
* Group number and lane number for this lane.
*********************************************************************/
bf_status_t bf_ll_serdes_lane_slice_0x6_lane_num_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_lane_slice_0x7_fifo_mode_a0_get
* 
* AR0_FIFO output data control register.
*     3'b0XX: Do not override AR0_FIFO's output data,
*     3'b100: AR0_FIFO output 32'd0
*     3'b101: AR0_FIFO output 32'hFFFF
*     3'b11X: AR0_FIFO output 32'b010101...
*********************************************************************/
bf_status_t bf_ll_serdes_lane_slice_0x7_fifo_mode_a0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_lane_slice_0x7_fifo_mode_a0_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_lane_slice_0x7_fifo_mode_a0_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_lane_slice_0x7_rsvd145_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_lane_slice_0x7_rsvd145_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_lane_slice_0x7_int_wr_count_val_a0_get
* 
* Slice 0 AR0_FIFO initial write address after AR0_PHY_READY toggle from inactive to active
*********************************************************************/
bf_status_t bf_ll_serdes_lane_slice_0x7_int_wr_count_val_a0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_lane_slice_0x7_int_wr_count_val_a0_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_lane_slice_0x7_int_wr_count_val_a0_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_lane_slice_0x7_rsvd146_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_lane_slice_0x7_rsvd146_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_lane_slice_0x8_rsvd_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_lane_slice_0x8_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_lane_slice_0x8_ar0_wren_0_get
* 
* Slice 0 AR0_FIFO enable
*********************************************************************/
bf_status_t bf_ll_serdes_lane_slice_0x8_ar0_wren_0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_lane_slice_0x8_rsvd147_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_lane_slice_0x8_rsvd147_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_lane_slice_0xa_prbs_dat_cntr_15_0_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_lane_slice_0xa_prbs_dat_cntr_15_0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_lane_slice_0xb_prbs_dat_cntr_31_16_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_lane_slice_0xb_prbs_dat_cntr_31_16_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_lane_slice_0xc_prbs_dat_cntr_47_32_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_lane_slice_0xc_prbs_dat_cntr_47_32_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x0_an_reset_get
* 
* 1 = AN reset
* 0 = AN normal operation
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x0_an_reset_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0x0_an_reset_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0x0_an_reset_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_lt_0x0_rsvd_get
* 
* Value always 0 writes ignored
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x0_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x0_extended_next_page_control_get
* 
* 1 = Extended Next Pages are enabled 
* 0 = Extended Next Pages are disabled
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x0_extended_next_page_control_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x0_auto_negotiation_en_get
* 
* 1 = enable Auto-Negotiation process 
* 0 = disable Auto-Negotiation process
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x0_auto_negotiation_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0x0_auto_negotiation_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0x0_auto_negotiation_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_lt_0x0_rsvd148_get
* 
* Value always 0 writes ignored
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x0_rsvd148_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x0_restart_auto_negotiation_get
* 
* 1 = Restart Auto-Negotiation process
* 0 = Auto-Negotiation in process disabled or not supported
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x0_restart_auto_negotiation_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0x0_restart_auto_negotiation_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0x0_restart_auto_negotiation_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_lt_0x0_rsvd149_get
* 
* Value always 0 writes ignored
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x0_rsvd149_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x1_rsvd_get
* 
* Value always 0
* Writes ignored
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x1_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x1_parallel_dectection_fault_get
* 
* 1 = A fault has been detected via the parallel detection function.
* 0 = A fault has not been detected via the parallel detection function.
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x1_parallel_dectection_fault_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0x1_parallel_dectection_fault_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0x1_parallel_dectection_fault_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_lt_0x1_rsvd150_get
* 
* Value always 0
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x1_rsvd150_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x1_extended_next_page_status_get
* 
* 1 = Extended Next Page format is used 
* 0 = Extended Next Page is not allowed
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x1_extended_next_page_status_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x1_page_received_get
* 
* 1 = A page has been received
* 0 = A page has not been received
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x1_page_received_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0x1_page_received_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0x1_page_received_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_lt_0x1_auto_negotiation_complete_get
* 
* 1 = Auto-Negotiation process completed
* 0 = Auto-Negotiation process not completed
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x1_auto_negotiation_complete_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x1_remote_fault_get
* 
* 1 = remote fault condition detected
* 0 = no remote fault condition detected
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x1_remote_fault_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0x1_remote_fault_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0x1_remote_fault_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_lt_0x1_auto_negotiation_ability_get
* 
* 1 = PHY is able to perform Auto-Negotiation
* 0 = PHY is not able to perform Auto-Negotiation
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x1_auto_negotiation_ability_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x1_link_status_get
* 
* 1 = Link is up
* 0 = Link is down
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x1_link_status_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0x1_link_status_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0x1_link_status_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_lt_0x1_rsvd151_get
* 
* Value always 0
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x1_rsvd151_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x1_link_partner_auto_negotiation_ability_get
* 
* 1 = LP is able to perform Auto-Negotiation
* 0 = LP is not able to perform Auto-Negotiation
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x1_link_partner_auto_negotiation_ability_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x2_oui_part_1_get
* 
* OUI[2:17]
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x2_oui_part_1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x3_oui_part_2_get
* 
* OUI[18:23]
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x3_oui_part_2_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x3_model_num_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x3_model_num_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x3_revision_num_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x3_revision_num_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x5_rsvd_get
* 
* Ignore on read
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x5_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x5_separated_pma_4_present_get
* 
* 1 = Separated PMAc (4) present in package 
* 0 = Separated PMA (4) not present in package
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x5_separated_pma_4_present_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x5_separated_pma_3_present_get
* 
* 1 = Separated PMA (3) present in package
* 0 = Separated PMA (3) not present in package
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x5_separated_pma_3_present_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x5_separated_pma__2_present_get
* 
* 1 = Separated PMA (2) present in package 
* 0 = Separated PMA (2) not present in package
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x5_separated_pma__2_present_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x5_separated_pma_1_present_get
* 
* 1 = Separated PMA (1) present in package
* 0 = Separated PMA (1) not present in package
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x5_separated_pma_1_present_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x5_auto_negotiation_present_get
* 
* 1 = Auto-Negotiation present in package
* 0 = Auto-Negotiation not present in package
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x5_auto_negotiation_present_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x5_tc_present_get
* 
* 1 = TC present in package
* 0 = TC not present in package
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x5_tc_present_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x5_dte_xs_present_get
* 
* 1 = DTE XS present in package
* 0 = DTE XS not present in package
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x5_dte_xs_present_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x5_phy_xs_present_get
* 
* 1 = PHY XS present in package
* 0 = PHY XS not present in package
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x5_phy_xs_present_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x5_pcs_present_get
* 
* 1 = PCS present in package
* 0 = PCS not present in package
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x5_pcs_present_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x5_wis_present_get
* 
* 1 = WIS present in package
* 0 = WIS not present in package
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x5_wis_present_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x5_pmd_pma_present_get
* 
* 1 = PMA/PMD present in package
* 0 = PMA/PMD not present in package
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x5_pmd_pma_present_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x5_clause_22_register_pres_ent_get
* 
* 1 = Clause 22 registers present in package
* 0 = Clause 22 registers not present in package
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x5_clause_22_register_pres_ent_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x6_vendor_specific_device_2_present_get
* 
* 1 = Vendor-specific device 2 present in package
* 0 = Vendor-specific device 2 not present in package
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x6_vendor_specific_device_2_present_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x6_vendor_specific_device_1_present_get
* 
* 1 = Vendor-specific device 1 present in package
* 0 = Vendor-specific device 1 not present in package
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x6_vendor_specific_device_1_present_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x6_clause_22_extension_present_get
* 
* 1 = Clause 22 extension present in package
* 0 = Clause 22 extension not present in package
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x6_clause_22_extension_present_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x6_rsvd152_get
* 
* Ignore on read
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x6_rsvd152_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x10_next_page_get
* 
* See 28.2.1.2 and 73.6.9
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x10_next_page_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0x10_next_page_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0x10_next_page_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_lt_0x10_acknowledge_get
* 
* Value always 0
* Writes ignored
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x10_acknowledge_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x10_remote_fault_get
* 
* See 28.2.1.2 and 73.6.7
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x10_remote_fault_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0x10_remote_fault_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0x10_remote_fault_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_lt_0x10_d12_d5_get
* 
* See 28.2.1.2 and 73.6
* D12: C2, reserved
* D11:D10: PAUSE
* D9:D5: Echoed Nonce Field
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x10_d12_d5_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0x10_d12_d5_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0x10_d12_d5_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_lt_0x10_selector_field_get
* 
* See Annex 28A
* Fixed 5'b0_0001
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x10_selector_field_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0x10_selector_field_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0x10_selector_field_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_lt_0x11_d31_d16_get
* 
* See 73.6
* bit 4:0=D20:D16: Tx Nonce Field
* bit 15:5=D31:D21: A10 to A0
* Technology Ability Fields
* bit 5: A0: 1000BASE-KX
* bit 6: A1: 10G-KX4
* bit 7: A2: 10G-KR
* bit 8: A3: 40G-KR4
* bit 9: A4: 40G-CR4
* bit 10: A5: 100G-CR10
* (following are from 802.3bj)
* bit 11: A6: 100G-KP4
* bit 12: A7: 100G-KR4
* bit 13: A8: 100G-CR4
* (following are from 802.3by)
* bit 14: A9: 25G-KR/CR-S
* bit 15: A10: 25G-KR/CR
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x11_d31_d16_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0x11_d31_d16_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0x11_d31_d16_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_lt_0x12_d47_d32_get
* 
* See 73.6
* D47: FEC request
* D46: FEC capability 
* D45:D32: A24 to A11
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x12_d47_d32_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0x12_d47_d32_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0x12_d47_d32_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_lt_0x13_d15_d0_get
* 
* See 28.2.1.2 and 73.6
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x13_d15_d0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x14_d31_d16_get
* 
* See 73.6
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x14_d31_d16_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x15_d47_d32_get
* 
* See 73.6
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x15_d47_d32_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x16_next_page_get
* 
* See 28.2.3.4 and 73.7.7.1
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x16_next_page_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0x16_next_page_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0x16_next_page_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_lt_0x16_rsvd153_get
* 
* Value always 0 writes ignored
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x16_rsvd153_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x16_msg_page_get
* 
* See 28.2.3.4 and 73.7.7.1
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x16_msg_page_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0x16_msg_page_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0x16_msg_page_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_lt_0x16_acknowledge_2_get
* 
* See 28.2.3.4 and 73.7.7.1
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x16_acknowledge_2_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0x16_acknowledge_2_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0x16_acknowledge_2_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_lt_0x16_toggle_get
* 
* See 28.2.3.4 and 73.7.7.1
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x16_toggle_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x16_msg_unformatted_code_field_get
* 
* See 28.2.3.4 and 73.7.7.1
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x16_msg_unformatted_code_field_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0x16_msg_unformatted_code_field_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0x16_msg_unformatted_code_field_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_lt_0x17_unformatted_code_field_1_get
* 
* See 28.2.3.4 and 73.7.7.1
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x17_unformatted_code_field_1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0x17_unformatted_code_field_1_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0x17_unformatted_code_field_1_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_lt_0x18_unformatted_code_field_2_get
* 
* See 28.2.3.4 and 73.7.7.1
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x18_unformatted_code_field_2_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0x18_unformatted_code_field_2_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0x18_unformatted_code_field_2_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_lt_0x19_next_page_get
* 
* See 28.2.3.4 and 73.7.7.1
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x19_next_page_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x19_acknowledge_get
* 
* See 28.2.3.4 and 73.7.7.1
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x19_acknowledge_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x19_msg_pane_get
* 
* See 28.2.3.4 and 73.7.7.1
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x19_msg_pane_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x19_acknowledge_2_get
* 
* See 28.2.3.4 and 73.7.7.1
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x19_acknowledge_2_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x19_toggle_get
* 
* See 28.2.3.4 and 73.7.7.1
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x19_toggle_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x19_msg_unformatted_code_field_get
* 
* See 28.2.3.4 and 73.7.7.1
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x19_msg_unformatted_code_field_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x1a_unformatted_code_field_1_get
* 
* See 28.2.3.4 and 73.7.7.1
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x1a_unformatted_code_field_1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x1b_unformatted_code_field_2_get
* 
* See 28.2.3.4 and 73.7.7.1
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x1b_unformatted_code_field_2_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x30_5gbase_kr_a12_get
* 
* 1 = PMA/PMD is negotiated to perform 5GBASE-KR
* 0 = PMA/PMD is not negotiated to perform 5GBASE-KR
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x30_5gbase_kr_a12_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x30_2_5gbase_kx_a11_get
* 
* 1 = PMA/PMD is negotiated to perform 2.5GBASE-KX
* 0 = PMA/PMD is not negotiated to perform 2.5GBASE-KX
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x30_2_5gbase_kx_a11_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x30_2_5gbase_kx_or_25gbase_cr_a10_get
* 
* 1 = PMA/PMD is negotiated to perform 25GBASE-KR or 25GBASE-CR
* 0 = PMA/PMD is not negotiated to perform 25GBASE-KR or 25GBASE-CR
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x30_2_5gbase_kx_or_25gbase_cr_a10_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x30_2_5gbase_kr_s_or_25gbase_cr_s_a9_get
* 
* 1 = PMA/PMD is negotiated to perform 25GBASE-KR-S or 25GBASE-CR-S
* 0 = PMA/PMD is not negotiated to perform 25GBASE-KR-S or 25GBASE-CR-S
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x30_2_5gbase_kr_s_or_25gbase_cr_s_a9_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x30_100gbase_cr4_a8_get
* 
* 1 = PMA/PMD is negotiated to perform 100GBASE-CR4
* 0 = PMA/PMD is not negotiated to perform 100GBASE-CR4
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x30_100gbase_cr4_a8_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x30_100gbase_kr4_a7_get
* 
* 1 = PMA/PMD is negotiated to perform 100GBASE-KR4
* 0 = PMA/PMD is not negotiated to perform 100GBASE-KR4
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x30_100gbase_kr4_a7_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x30_100gbase_kp4_a6_get
* 
* 1 = PMA/PMD is negotiated to perform 100GBASE-KP4
* 0 = PMA/PMD is not negotiated to perform 100GBASE-KP4
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x30_100gbase_kp4_a6_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x30_100gbase_cr10_a5_get
* 
* 1 = PMA/PMD is negotiated to perform 100GBASE-CR10
* 0 = PMA/PMD is not negotiated to perform 100GBASE-CR10
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x30_100gbase_cr10_a5_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x30_rsvd154_get
* 
* Ignore on read
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x30_rsvd154_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x30_40gbase_cr4_a4_get
* 
* 1 = PMA/PMD is negotiated to perform 40GBASE-CR4
* 0 = PMA/PMD is not negotiated to perform 40GBASE-CR4
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x30_40gbase_cr4_a4_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x30_40gbase_kr4_a3_get
* 
* 1 = PMA/PMD is negotiated to perform 40GBASE-KR4
* 0 = PMA/PMD is not negotiated to perform 40GBASE-KR4
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x30_40gbase_kr4_a3_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x30_base_r_fec_negotiated_get
* 
* 1 = PMA/PMD is negotiated to perform BASE-R FEC
* 0 = PMA/PMD is not negotiated to perform BASE-R FEC
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x30_base_r_fec_negotiated_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x30_10gbase_kr_a2_get
* 
* 1 = PMA/PMD is negotiated to perform 10GBASE-KR
* 0 = PMA/PMD is not negotiated to perform 10GBASE-KR
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x30_10gbase_kr_a2_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x30_10gbase_kx4_a1_get
* 
* 1 = PMA/PMD is negotiated to perform 10GBASE-KX4 or CX4 
* 0 = PMA/PMD is not negotiated to perform 10GBASE-KX4/CX4
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x30_10gbase_kx4_a1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x30_1000base_kx_a0_get
* 
* 1 = PMA/PMD is negotiated to perform 1000BASE-KX
* 0 = PMA/PMD is not negotiated to perform 1000BASE-KX
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x30_1000base_kx_a0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0x30_bp_an_ability_get
* 
* If a Backplane BASE-R copper PHY type is implemented, this bit is set to 1.
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0x30_bp_an_ability_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_lt_0xf0_25g_50g_consortium_autoneg_next_page_msg_page_bit_15_0_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0xf0_25g_50g_consortium_autoneg_next_page_msg_page_bit_15_0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0xf0_25g_50g_consortium_autoneg_next_page_msg_page_bit_15_0_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0xf0_25g_50g_consortium_autoneg_next_page_msg_page_bit_15_0_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_lt_0xf1_25g_50g_consortium_autoneg_next_page_msg_page_bit_31_16_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0xf1_25g_50g_consortium_autoneg_next_page_msg_page_bit_31_16_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0xf1_25g_50g_consortium_autoneg_next_page_msg_page_bit_31_16_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0xf1_25g_50g_consortium_autoneg_next_page_msg_page_bit_31_16_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_lt_0xf2_25g_50g_consortium_autoneg_next_page_msg_page_bit_47_32_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0xf2_25g_50g_consortium_autoneg_next_page_msg_page_bit_47_32_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0xf2_25g_50g_consortium_autoneg_next_page_msg_page_bit_47_32_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0xf2_25g_50g_consortium_autoneg_next_page_msg_page_bit_47_32_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_lt_0xf3_25g_50g_consortium_autoneg_next_page_msg_page_bit_15_0_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0xf3_25g_50g_consortium_autoneg_next_page_msg_page_bit_15_0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0xf3_25g_50g_consortium_autoneg_next_page_msg_page_bit_15_0_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0xf3_25g_50g_consortium_autoneg_next_page_msg_page_bit_15_0_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_lt_0xf4_25g_50g_consortium_autoneg_next_page_msg_page_bit_31_16_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0xf4_25g_50g_consortium_autoneg_next_page_msg_page_bit_31_16_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0xf4_25g_50g_consortium_autoneg_next_page_msg_page_bit_31_16_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0xf4_25g_50g_consortium_autoneg_next_page_msg_page_bit_31_16_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_lt_0xf5_25g_50g_consortium_autoneg_next_page_msg_page_bit_47_32_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0xf5_25g_50g_consortium_autoneg_next_page_msg_page_bit_47_32_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0xf5_25g_50g_consortium_autoneg_next_page_msg_page_bit_47_32_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0xf5_25g_50g_consortium_autoneg_next_page_msg_page_bit_47_32_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_lt_0xf6_25g_50g_consortium_oui_mp5_valid_code_for_comparing_with_received_oui_15_0_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0xf6_25g_50g_consortium_oui_mp5_valid_code_for_comparing_with_received_oui_15_0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0xf6_25g_50g_consortium_oui_mp5_valid_code_for_comparing_with_received_oui_15_0_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0xf6_25g_50g_consortium_oui_mp5_valid_code_for_comparing_with_received_oui_15_0_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_lt_0xf7_25g_50g_consortium_oui_mp5_valid_code_for_comparing_with_received_oui_23_16_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0xf7_25g_50g_consortium_oui_mp5_valid_code_for_comparing_with_received_oui_23_16_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0xf7_25g_50g_consortium_oui_mp5_valid_code_for_comparing_with_received_oui_23_16_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0xf7_25g_50g_consortium_oui_mp5_valid_code_for_comparing_with_received_oui_23_16_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_lt_0xf8_25g_50g_consortium_oui_mp5_valid_code_for_comparing_with_received_oui_15_0_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0xf8_25g_50g_consortium_oui_mp5_valid_code_for_comparing_with_received_oui_15_0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0xf8_25g_50g_consortium_oui_mp5_valid_code_for_comparing_with_received_oui_15_0_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0xf8_25g_50g_consortium_oui_mp5_valid_code_for_comparing_with_received_oui_15_0_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_lt_0xf9_tx_arg_null_mp_s_15_0_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0xf9_tx_arg_null_mp_s_15_0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0xf9_tx_arg_null_mp_s_15_0_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0xf9_tx_arg_null_mp_s_15_0_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_lt_0xfa_tx_arg_null_mp_s_31_16_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0xfa_tx_arg_null_mp_s_31_16_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0xfa_tx_arg_null_mp_s_31_16_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0xfa_tx_arg_null_mp_s_31_16_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_lt_0xfb_tx_arg_null_mp_s_47_32_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0xfb_tx_arg_null_mp_s_47_32_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0xfb_tx_arg_null_mp_s_47_32_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_lt_0xfb_tx_arg_null_mp_s_47_32_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_lt_0xfc_aa_link_control_report_s_15_0_get
* 
* 
*********************************************************************/
bf_status_t bf_ll_serdes_an_lt_0xfc_aa_link_control_report_s_15_0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x0_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x0_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x0_training_en_get
* 
* 0h = Disable the training start-up protocol
* 1h = Enable the training start-up protocol
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x0_training_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x0_training_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x0_training_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x0_training_restart_get
* 
* 0h = Normal operation
* 1h = Restart the training start-up protocol
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x0_training_restart_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x0_training_restart_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x0_training_restart_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x4_readout_state_get
* 
* Readout for frame lock state.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x4_readout_state_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x4_frame_lock_get
* 
* Frame lock readout.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x4_frame_lock_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x4_rx_trained_get
* 
* Local training finish flag.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x4_rx_trained_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x4_readout_training_state_get
* 
* Training state readout.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x4_readout_training_state_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x4_training_failure_get
* 
* Training failure.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x4_training_failure_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x4_tx_training_data_en_get
* 
* TX control to send training pattern.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x4_tx_training_data_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x4_sig_det_get
* 
* Signal detect for PCS.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x4_sig_det_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x4_readout_txstate_get
* 
* State machine readout for training arbiter
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x4_readout_txstate_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x6_max_wait_timer_31_16_get
* 
* Training max wait timer.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x6_max_wait_timer_31_16_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x6_max_wait_timer_31_16_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x6_max_wait_timer_31_16_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x7_max_wait_timer_15_0_get
* 
* Training max wait timer.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x7_max_wait_timer_15_0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x7_max_wait_timer_15_0_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x7_max_wait_timer_15_0_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x8_wait_timer_31_16_get
* 
* Training wait timer.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x8_wait_timer_31_16_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x8_wait_timer_31_16_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x8_wait_timer_31_16_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x9_wait_timer_15_0_get
* 
* Training wait timer.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x9_wait_timer_15_0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x9_wait_timer_15_0_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x9_wait_timer_15_0_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0xa_training_state_owen_get
* 
* Training state overwrite enable bit.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0xa_training_state_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0xa_training_state_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0xa_training_state_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0xa_training_state_ow_get
* 
* Training state overwrite number.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0xa_training_state_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0xa_training_state_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0xa_training_state_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0xa_rsvd155_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0xa_rsvd155_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0xa_total_power_max_get
* 
* Total tap value maximum number. 
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0xa_total_power_max_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0xa_total_power_max_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0xa_total_power_max_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0xa_rsvd156_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0xa_rsvd156_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0xa_rsvd157_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0xa_rsvd157_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0xb_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0xb_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0xb_tapm1_max_val_get
* 
* Maximum coefficient value for tap -1.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0xb_tapm1_max_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0xb_tapm1_max_val_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0xb_tapm1_max_val_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0xb_rsvd158_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0xb_rsvd158_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0xb_tapm1_min_val_get
* 
* Minimum coefficient value for tap -1.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0xb_tapm1_min_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0xb_tapm1_min_val_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0xb_tapm1_min_val_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0xc_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0xc_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0xc_tapm1_preset_val_get
* 
* Preset coefficient value for tap -1.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0xc_tapm1_preset_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0xc_tapm1_preset_val_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0xc_tapm1_preset_val_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0xc_rsvd159_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0xc_rsvd159_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0xc_tapm1_ini_val_get
* 
* Initialize coefficient value for tap -1.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0xc_tapm1_ini_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0xc_tapm1_ini_val_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0xc_tapm1_ini_val_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0xd_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0xd_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0xd_tap0_max_val_get
* 
* Maximum coefficient value for tap 0
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0xd_tap0_max_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0xd_tap0_max_val_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0xd_tap0_max_val_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0xd_rsvd160_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0xd_rsvd160_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0xd_tap0_min_val_get
* 
* Minimum coefficient value for tap 0
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0xd_tap0_min_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0xd_tap0_min_val_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0xd_tap0_min_val_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0xe_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0xe_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0xe_tap0_preset_val_get
* 
* Preset coefficient value for tap 0
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0xe_tap0_preset_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0xe_tap0_preset_val_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0xe_tap0_preset_val_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0xe_rsvd161_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0xe_rsvd161_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0xe_tap0_ini_val_get
* 
* Initialize coefficient value for tap 0
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0xe_tap0_ini_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0xe_tap0_ini_val_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0xe_tap0_ini_val_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0xf_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0xf_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0xf_tap1_max_val_get
* 
* Maximum coefficient value for tap 1
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0xf_tap1_max_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0xf_tap1_max_val_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0xf_tap1_max_val_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0xf_rsvd162_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0xf_rsvd162_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0xf_tap1_min_val_get
* 
* Minimum coefficient value for tap 1
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0xf_tap1_min_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0xf_tap1_min_val_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0xf_tap1_min_val_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x10_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x10_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x10_tap1_preset_val_get
* 
* Preset coefficient value for tap 0
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x10_tap1_preset_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x10_tap1_preset_val_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x10_tap1_preset_val_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x10_rsvd163_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x10_rsvd163_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x10_tap1_ini_val_get
* 
* Initialize coefficient value for tap 0
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x10_tap1_ini_val_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x10_tap1_ini_val_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x10_tap1_ini_val_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x11_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x11_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x11_lp_preset_get
* 
* Link partner preset control
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x11_lp_preset_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x11_lp_init_get
* 
* Link partner initialize control
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x11_lp_init_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x11_rsvd164_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x11_rsvd164_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x11_lp_coeff_1update_get
* 
* Link partner coefficient (+1) update 
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x11_lp_coeff_1update_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x11_lp_coeff_0update_get
* 
* Link partner coefficient (0) update 
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x11_lp_coeff_0update_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x11_lp_coeff_m1update_get
* 
* Link partner coefficient (-1) update 
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x11_lp_coeff_m1update_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x12_lp_preset_owen_get
* 
* Link partner preset control overwrite enable
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x12_lp_preset_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x12_lp_preset_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x12_lp_preset_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x12_lp_init_owen_get
* 
* Link partner initialize control overwrite enable
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x12_lp_init_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x12_lp_init_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x12_lp_init_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x12_lp_preset_ow_get
* 
* Link partner preset control overwrite
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x12_lp_preset_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x12_lp_preset_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x12_lp_preset_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x12_lp_init_ow_get
* 
* Link partner initialize control overwrite
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x12_lp_init_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x12_lp_init_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x12_lp_init_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x12_rsvd165_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x12_rsvd165_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x12_lp_coeff_1update_owen_get
* 
* Link partner coefficient (+1) update overwrite enable
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x12_lp_coeff_1update_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x12_lp_coeff_1update_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x12_lp_coeff_1update_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x12_lp_coeff_0update_owen_get
* 
* Link partner coefficient (0) update overwrite enable
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x12_lp_coeff_0update_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x12_lp_coeff_0update_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x12_lp_coeff_0update_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x12_lp_coeff_m1update_owen_get
* 
* Link partner coefficient (-1) update overwrite enable
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x12_lp_coeff_m1update_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x12_lp_coeff_m1update_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x12_lp_coeff_m1update_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x12_lp_coeff_1update_ow_get
* 
* Link partner coefficient (+1) update overwrite
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x12_lp_coeff_1update_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x12_lp_coeff_1update_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x12_lp_coeff_1update_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x12_lp_coeff_0update_ow_get
* 
* Link partner coefficient (0) update overwrite
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x12_lp_coeff_0update_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x12_lp_coeff_0update_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x12_lp_coeff_0update_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x12_lp_coeff_m1update_ow_get
* 
* Link partner coefficient (-1) update overwrite
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x12_lp_coeff_m1update_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x12_lp_coeff_m1update_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x12_lp_coeff_m1update_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x13_lp_receiver_ready_get
* 
* Link partner training finish
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x13_lp_receiver_ready_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x13_rsvd166_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x13_rsvd166_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x13_lp_coeff_1status_get
* 
* Link partner coefficient (+1) status.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x13_lp_coeff_1status_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x13_lp_coeff_0status_get
* 
* Link partner coefficient (0) status.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x13_lp_coeff_0status_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x13_lp_coeff_m1status_get
* 
* Link partner coefficient (-1) status.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x13_lp_coeff_m1status_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x14_lp_receiver_ready_owen_get
* 
* Link partner training finish overwrite enable.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x14_lp_receiver_ready_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x14_lp_receiver_ready_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x14_lp_receiver_ready_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x14_lp_receiver_ready_ow_get
* 
* Link partner training finish overwrite.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x14_lp_receiver_ready_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x14_lp_receiver_ready_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x14_lp_receiver_ready_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x14_rsvd167_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x14_rsvd167_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x14_lp_coeff_1status_owen_get
* 
* Link partner coefficient (+1) status overwrite enable.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x14_lp_coeff_1status_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x14_lp_coeff_1status_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x14_lp_coeff_1status_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x14_lp_coeff_0status_owen_get
* 
* Link partner coefficient (0) status overwrite enable.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x14_lp_coeff_0status_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x14_lp_coeff_0status_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x14_lp_coeff_0status_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x14_lp_coeff_m1status_owen_get
* 
* Link partner coefficient (-1) status overwrite enable.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x14_lp_coeff_m1status_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x14_lp_coeff_m1status_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x14_lp_coeff_m1status_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x14_lp_coeff_1status_ow_get
* 
* Link partner coefficient (+1) status overwrite.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x14_lp_coeff_1status_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x14_lp_coeff_1status_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x14_lp_coeff_1status_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x14_lp_coeff_0status_ow_get
* 
* Link partner coefficient (0) status overwrite.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x14_lp_coeff_0status_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x14_lp_coeff_0status_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x14_lp_coeff_0status_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x14_lp_coeff_m1status_ow_get
* 
* Link partner coefficient (-1) status overwrite.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x14_lp_coeff_m1status_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x14_lp_coeff_m1status_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x14_lp_coeff_m1status_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x15_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x15_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x15_ld_preset_get
* 
* Local device preset control.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x15_ld_preset_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x15_ld_init_get
* 
* Local device initialize control.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x15_ld_init_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x15_rsvd168_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x15_rsvd168_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x15_ld_coeff_1update_get
* 
* Local device coefficient (+1) update. 
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x15_ld_coeff_1update_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x15_ld_coeff_0update_get
* 
* Local device coefficient (0) update. 
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x15_ld_coeff_0update_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x15_ld_coeff_m1update_get
* 
* Local device coefficient (-1) update. 
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x15_ld_coeff_m1update_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x16_ld_preset_owen_get
* 
* Local device preset control overwrite enable.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x16_ld_preset_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x16_ld_preset_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x16_ld_preset_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x16_ld_init_owen_get
* 
* Local device initialize control overwrite enable.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x16_ld_init_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x16_ld_init_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x16_ld_init_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x16_ld_preset_ow_get
* 
* Local device preset control overwrite.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x16_ld_preset_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x16_ld_preset_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x16_ld_preset_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x16_ld_init_ow_get
* 
* Local device initialize control overwrite.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x16_ld_init_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x16_ld_init_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x16_ld_init_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x16_rsvd169_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x16_rsvd169_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x16_ld_coeff_1update_owen_get
* 
* Local device coefficient (+1) update overwrite enable.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x16_ld_coeff_1update_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x16_ld_coeff_1update_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x16_ld_coeff_1update_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x16_ld_coeff_0update_owen_get
* 
* Local device coefficient (0) update overwrite enable.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x16_ld_coeff_0update_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x16_ld_coeff_0update_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x16_ld_coeff_0update_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x16_ld_coeff_m1update_owen_get
* 
* Local device coefficient (-1) update overwrite enable.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x16_ld_coeff_m1update_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x16_ld_coeff_m1update_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x16_ld_coeff_m1update_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x16_ld_coeff_1update_ow_get
* 
* Local device coefficient (+1) update overwrite.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x16_ld_coeff_1update_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x16_ld_coeff_1update_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x16_ld_coeff_1update_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x16_ld_coeff_0update_ow_get
* 
* Local device coefficient (0) update overwrite.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x16_ld_coeff_0update_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x16_ld_coeff_0update_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x16_ld_coeff_0update_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x16_ld_coeff_m1update_ow_get
* 
* Local device coefficient (-1) update overwrite.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x16_ld_coeff_m1update_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x16_ld_coeff_m1update_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x16_ld_coeff_m1update_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x17_ld_receiver_ready_get
* 
* Local device training finished.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x17_ld_receiver_ready_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x17_rsvd170_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x17_rsvd170_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x17_ld_coeff_1status_get
* 
* Local device coefficient (+1) status.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x17_ld_coeff_1status_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x17_ld_coeff_0status_get
* 
* Local device coefficient (0) status.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x17_ld_coeff_0status_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x17_ld_coeff_m1status_get
* 
* Local device coefficient (-1) status.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x17_ld_coeff_m1status_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x18_ld_receiver_ready_owen_get
* 
* Local device training finished overwrite enable.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x18_ld_receiver_ready_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x18_ld_receiver_ready_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x18_ld_receiver_ready_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x18_ld_receiver_ready_ow_get
* 
* Local device training finished overwrite.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x18_ld_receiver_ready_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x18_ld_receiver_ready_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x18_ld_receiver_ready_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x18_rsvd171_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x18_rsvd171_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x18_ld_coeff_1status_owen_get
* 
* Local device coefficient (+1) status overwrite enable.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x18_ld_coeff_1status_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x18_ld_coeff_1status_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x18_ld_coeff_1status_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x18_ld_coeff_0status_owen_get
* 
* Local device coefficient (0) status overwrite enable.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x18_ld_coeff_0status_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x18_ld_coeff_0status_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x18_ld_coeff_0status_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x18_ld_coeff_m1status_owen_get
* 
* Local device coefficient (-1) status overwrite enable.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x18_ld_coeff_m1status_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x18_ld_coeff_m1status_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x18_ld_coeff_m1status_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x18_ld_coeff_1status_ow_get
* 
* Local device coefficient (+1) status overwrite.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x18_ld_coeff_1status_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x18_ld_coeff_1status_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x18_ld_coeff_1status_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x18_ld_coeff_0status_ow_get
* 
* Local device coefficient (0) status overwrite.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x18_ld_coeff_0status_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x18_ld_coeff_0status_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x18_ld_coeff_0status_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x18_ld_coeff_m1status_ow_get
* 
* Local device coefficient (-1) status overwrite.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x18_ld_coeff_m1status_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x18_ld_coeff_m1status_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x18_ld_coeff_m1status_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x19_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x19_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x19_dis_an_start_get
* 
* Disable auto-negotiation finish start
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x19_dis_an_start_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x19_dis_an_start_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x19_dis_an_start_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x19_hold_frame_get
* 
* Send hold frame
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x19_hold_frame_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x19_hold_frame_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x19_hold_frame_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x19_preset_frame_get
* 
* Send preset frame
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x19_preset_frame_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x19_preset_frame_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x19_preset_frame_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x19_ini_frame_get
* 
* Send initialize frame
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x19_ini_frame_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x19_ini_frame_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x19_ini_frame_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x19_coeff0_frame_inc_get
* 
* Send coefficient 0 increment frame
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x19_coeff0_frame_inc_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x19_coeff0_frame_inc_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x19_coeff0_frame_inc_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x19_coeff0_frame_dec_get
* 
* Send coefficient 0 decrement frame
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x19_coeff0_frame_dec_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x19_coeff0_frame_dec_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x19_coeff0_frame_dec_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x19_coeffm1_frame_inc_get
* 
* Send coefficient -1 increment frame
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x19_coeffm1_frame_inc_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x19_coeffm1_frame_inc_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x19_coeffm1_frame_inc_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x19_coeffm1_frame_dec_get
* 
* Send coefficient -1 decrement frame
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x19_coeffm1_frame_dec_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x19_coeffm1_frame_dec_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x19_coeffm1_frame_dec_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x19_coeff1_frame_inc_get
* 
* Send coefficient 1 increment frame
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x19_coeff1_frame_inc_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x19_coeff1_frame_inc_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x19_coeff1_frame_inc_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x19_coeff1_frame_dec_get
* 
* Send coefficient 1 decrement frame
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x19_coeff1_frame_dec_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x19_coeff1_frame_dec_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x19_coeff1_frame_dec_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x19_reg_auto_finish_get
* 
* Use timer counter to generated trained signal
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x19_reg_auto_finish_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x19_reg_auto_finish_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x19_reg_auto_finish_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x19_reg_rx_trained_get
* 
* Local device training finish
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x19_reg_rx_trained_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x19_reg_rx_trained_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x19_reg_rx_trained_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x19_reg_send_clear_get
* 
* Reset the TX state machine to UPDATE WAIT
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x19_reg_send_clear_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x19_reg_send_clear_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x19_reg_send_clear_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x1a_finish_cntr_msb_get
* 
* Auto finish training counter bits 31:16
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x1a_finish_cntr_msb_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x1a_finish_cntr_msb_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x1a_finish_cntr_msb_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x1b_finish_cntr_lsb_get
* 
* Auto finish training counter bits 15:0
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x1b_finish_cntr_lsb_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x1b_finish_cntr_lsb_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x1b_finish_cntr_lsb_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x1c_train_inst_0_get
* 
* Training instruction command, bits 15:0. Four instructions.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x1c_train_inst_0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x1c_train_inst_0_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x1c_train_inst_0_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x1d_train_inst_1_get
* 
* Training instruction command, bits 31:16. Four instructions.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x1d_train_inst_1_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x1d_train_inst_1_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x1d_train_inst_1_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x1e_train_inst_2_get
* 
* Training instruction command, bits 47:32. Four instructions.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x1e_train_inst_2_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x1e_train_inst_2_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x1e_train_inst_2_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x1f_train_inst_3_get
* 
* Training instruction command, bits 63:48. Four instructions.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x1f_train_inst_3_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x1f_train_inst_3_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x1f_train_inst_3_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x20_train_inst_4_get
* 
* Training instruction command, bits 79:64. Four instructions.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x20_train_inst_4_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x20_train_inst_4_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x20_train_inst_4_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x21_train_inst_5_get
* 
* Training instruction command, bits 95:80. Four instructions.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x21_train_inst_5_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x21_train_inst_5_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x21_train_inst_5_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x22_train_inst_6_get
* 
* Training instruction command, bits 111:96. Four instructions.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x22_train_inst_6_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x22_train_inst_6_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x22_train_inst_6_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x23_train_inst_7_get
* 
* Training instruction command, bits 127:112. Four instructions.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x23_train_inst_7_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x23_train_inst_7_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x23_train_inst_7_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x24_train_mode_get
* 
* Link training mode selection:
*      00b or 01b: Clause 136/137 PAM4 50G link training
*      11b: Clause 72, Clause 84/85 NRZ 10G link training
*      10b: Clause 92/93, Clause 110/111 NRZ 25G link training
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x24_train_mode_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x24_train_mode_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x24_train_mode_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x24_rsvd172_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x24_rsvd172_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x25_holdoff_timer_31_16_get
* 
* Training timeout wait timer.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x25_holdoff_timer_31_16_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x25_holdoff_timer_31_16_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x25_holdoff_timer_31_16_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x26_holdoff_timer_15_0_get
* 
* Training timeout wait timer.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x26_holdoff_timer_15_0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x26_holdoff_timer_15_0_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x26_holdoff_timer_15_0_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x29_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x29_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x29_total_power_max_50g_get
* 
* Maximum total power.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x29_total_power_max_50g_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x29_total_power_max_50g_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x29_total_power_max_50g_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x29_rsvd173_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x29_rsvd173_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x2a_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x2a_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x2a_tap0_max_val_50g_get
* 
* Maximum coefficient value for tap 0.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x2a_tap0_max_val_50g_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x2a_tap0_max_val_50g_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x2a_tap0_max_val_50g_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x2a_rsvd174_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x2a_rsvd174_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x2a_tap0_min_val_50g_get
* 
* Minimum coefficient value for tap 0.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x2a_tap0_min_val_50g_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x2a_tap0_min_val_50g_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x2a_tap0_min_val_50g_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x2b_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x2b_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x2b_tap0_preset1_val_50g_get
* 
* Preset1 coefficient value for tap 0.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x2b_tap0_preset1_val_50g_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x2b_tap0_preset1_val_50g_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x2b_tap0_preset1_val_50g_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x2b_rsvd175_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x2b_rsvd175_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x2b_tap0_preset2_val_50g_get
* 
* Preset2 coefficient value for tap 0.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x2b_tap0_preset2_val_50g_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x2b_tap0_preset2_val_50g_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x2b_tap0_preset2_val_50g_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x2c_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x2c_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x2c_tap0_preset3_val_50g_get
* 
* Preset3 coefficient value for tap 0.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x2c_tap0_preset3_val_50g_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x2c_tap0_preset3_val_50g_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x2c_tap0_preset3_val_50g_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x2c_rsvd176_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x2c_rsvd176_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x2d_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x2d_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x2d_tap1_max_val_50g_get
* 
* Maximum coefficient value for tap 1.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x2d_tap1_max_val_50g_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x2d_tap1_max_val_50g_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x2d_tap1_max_val_50g_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x2d_rsvd177_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x2d_rsvd177_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x2d_tap1_min_val_50g_get
* 
* Minimum coefficient value for tap 1.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x2d_tap1_min_val_50g_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x2d_tap1_min_val_50g_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x2d_tap1_min_val_50g_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x2e_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x2e_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x2e_tap1_preset1_val_50g_get
* 
* Preset1 coefficient value for tap 1.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x2e_tap1_preset1_val_50g_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x2e_tap1_preset1_val_50g_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x2e_tap1_preset1_val_50g_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x2e_rsvd178_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x2e_rsvd178_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x2e_tap1_preset2_val_50g_get
* 
* Preset2 coefficient value for tap 1.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x2e_tap1_preset2_val_50g_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x2e_tap1_preset2_val_50g_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x2e_tap1_preset2_val_50g_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x2f_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x2f_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x2f_tap1_preset3_val_50g_get
* 
* Preset3 coefficient value for tap 1.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x2f_tap1_preset3_val_50g_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x2f_tap1_preset3_val_50g_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x2f_tap1_preset3_val_50g_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x2f_rsvd179_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x2f_rsvd179_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x30_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x30_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x30_tapm1_max_val_50g_get
* 
* Maximum coefficient value for tap -1.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x30_tapm1_max_val_50g_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x30_tapm1_max_val_50g_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x30_tapm1_max_val_50g_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x30_rsvd180_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x30_rsvd180_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x30_tapm1_min_val_50g_get
* 
* Minimum coefficient value for tap -1.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x30_tapm1_min_val_50g_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x30_tapm1_min_val_50g_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x30_tapm1_min_val_50g_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x31_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x31_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x31_tapm1_preset1_val_50g_get
* 
* Preset1 coefficient value for tap -1.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x31_tapm1_preset1_val_50g_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x31_tapm1_preset1_val_50g_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x31_tapm1_preset1_val_50g_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x31_rsvd181_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x31_rsvd181_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x31_tapm1_preset2_val_50g_get
* 
* Preset2 coefficient value for tap -1.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x31_tapm1_preset2_val_50g_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x31_tapm1_preset2_val_50g_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x31_tapm1_preset2_val_50g_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x32_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x32_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x32_tapm1_preset3_val_50g_get
* 
* Preset3 coefficient value for tap -1.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x32_tapm1_preset3_val_50g_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x32_tapm1_preset3_val_50g_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x32_tapm1_preset3_val_50g_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x32_rsvd182_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x32_rsvd182_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x33_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x33_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x33_tapm2_max_val_50g_get
* 
* Maximum coefficient value for tap -2.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x33_tapm2_max_val_50g_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x33_tapm2_max_val_50g_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x33_tapm2_max_val_50g_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x33_rsvd183_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x33_rsvd183_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x33_tapm2_min_val_50g_get
* 
* Minimum coefficient value for tap -2.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x33_tapm2_min_val_50g_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x33_tapm2_min_val_50g_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x33_tapm2_min_val_50g_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x34_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x34_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x34_tapm2_preset1_val_50g_get
* 
* Preset1 coefficient value for tap -2.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x34_tapm2_preset1_val_50g_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x34_tapm2_preset1_val_50g_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x34_tapm2_preset1_val_50g_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x34_rsvd184_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x34_rsvd184_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x34_tapm2_preset2_val_50g_get
* 
* Preset2 coefficient value for tap -2.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x34_tapm2_preset2_val_50g_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x34_tapm2_preset2_val_50g_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x34_tapm2_preset2_val_50g_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x35_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x35_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x35_tapm2_preset3_val_50g_get
* 
* Preset3 coefficient value for tap -2.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x35_tapm2_preset3_val_50g_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x35_tapm2_preset3_val_50g_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x35_tapm2_preset3_val_50g_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x35_rsvd185_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x35_rsvd185_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x36_finish_cntr_50g_31_16_get
* 
* Auto finish training counter.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x36_finish_cntr_50g_31_16_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x36_finish_cntr_50g_31_16_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x36_finish_cntr_50g_31_16_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x37_finish_cntr_50g_15_0_get
* 
* Auto finish training counter.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x37_finish_cntr_50g_15_0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x37_finish_cntr_50g_15_0_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x37_finish_cntr_50g_15_0_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x38_train_inst_50g_15_0_get
* 
* Training instruction command, bits 15:0.
*     TRAIN_INST_50G[159:0] holds 32 PAM-4 training frame requests. Each request is represented by 5 bits, with the first request in TRAIN_INST_50G[4:0] and the last in TRAIN_INST_50G[159:155].
* The 5 bit training frame requests are encoded as follows:
*     5'd0 = TRAINED_FRAME, last training frame was sent, ready to finish link training.
*     5'd1 = PRESET1_FRAME, training frame with Preset1 initial condition request 
*     5'd2 = PRESET2_FRAME, training frame with Preset2 initial condition request 
*     5'd3 = PRESET3_FRAME, training frame with Preset3 initial condition request 
*     5'd4 = C0_INC, training frame with coefficient select of c(0) and coefficient request of increment
*     5'd5 = C0_DEC, training frame with coefficient select of c(0) and coefficient request of decrement
*     5'd6 = C0_NO_EQ, training frame with coefficient select of c(0) and coefficient request of no equalization
*     5'd7 = CM2_INC, training frame with coefficient select of c(-2) and coefficient request of increment
*     5'd8 = CM2_DEC, training frame with coefficient select of c(-2) and coefficient request of decrement
*     5'd9 = CM2_NO_EQ, training frame with coefficient select of c(-2) and coefficient request of no equalization
*     5'd10 = CM1_INC, training frame with coefficient select of c(-1) and coefficient request of increment
*     5'd11 = CM1_DEC, training frame with coefficient select of c(-1) and coefficient request of decrement
*     5'd12 = CM1_NO_EQ, training frame with coefficient select of c(-1) and coefficient request of no equalization
*     5'd13 = C1_INC, training frame with coefficient select of c(1) and coefficient request of increment
*     5'd14 = C1_DEC, training frame with coefficient select of c(1) and coefficient request of increment
*     5'd15 = C1_NO_EQ, training frame with coefficient select of c(1) and coefficient request of no equalization
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x38_train_inst_50g_15_0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x38_train_inst_50g_15_0_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x38_train_inst_50g_15_0_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x39_train_inst_50g_31_16_get
* 
* Training instruction command, bits 31:16.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x39_train_inst_50g_31_16_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x39_train_inst_50g_31_16_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x39_train_inst_50g_31_16_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x3a_train_inst_50g_47_32_get
* 
* Training instruction command, bits 47:32.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x3a_train_inst_50g_47_32_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x3a_train_inst_50g_47_32_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x3a_train_inst_50g_47_32_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x3b_train_inst_50g_63_48_get
* 
* Training instruction command, bits 63:48.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x3b_train_inst_50g_63_48_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x3b_train_inst_50g_63_48_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x3b_train_inst_50g_63_48_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x3c_train_inst_50g_79_64_get
* 
* Training instruction command, bits 79:64.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x3c_train_inst_50g_79_64_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x3c_train_inst_50g_79_64_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x3c_train_inst_50g_79_64_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x3d_train_inst_50g_95_80_get
* 
* Training instruction command, bits 95:80.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x3d_train_inst_50g_95_80_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x3d_train_inst_50g_95_80_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x3d_train_inst_50g_95_80_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x3e_train_inst_50g_111_96_get
* 
* Training instruction command, bits 111:96.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x3e_train_inst_50g_111_96_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x3e_train_inst_50g_111_96_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x3e_train_inst_50g_111_96_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x3f_train_inst_50g_127_112_get
* 
* Training instruction command, bits 127:112.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x3f_train_inst_50g_127_112_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x3f_train_inst_50g_127_112_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x3f_train_inst_50g_127_112_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x40_train_inst_50g_143_128_get
* 
* Training instruction command, bits 143:128.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x40_train_inst_50g_143_128_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x40_train_inst_50g_143_128_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x40_train_inst_50g_143_128_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x41_train_inst_50g_159_144_get
* 
* Training instruction command, bits 159:144.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x41_train_inst_50g_159_144_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x41_train_inst_50g_159_144_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x41_train_inst_50g_159_144_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x42_lp_ic_req_owen_get
* 
* Link partner initial condition request overwrite enable (50G mode).
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x42_lp_ic_req_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x42_lp_ic_req_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x42_lp_ic_req_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x42_lp_ic_req_ow_get
* 
* Link partner initial condition request overwrite (50G mode).
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x42_lp_ic_req_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x42_lp_ic_req_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x42_lp_ic_req_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x42_lp_mod_prec_req_owen_get
* 
* Link partner modulation and precoding request overwrite enable (50G mode).
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x42_lp_mod_prec_req_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x42_lp_mod_prec_req_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x42_lp_mod_prec_req_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x42_lp_mod_prec_req_ow_get
* 
* Link partner modulation and precoding request overwrite (50G mode).
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x42_lp_mod_prec_req_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x42_lp_mod_prec_req_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x42_lp_mod_prec_req_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x42_lp_coeff_sel_owen_get
* 
* Link partner coefficient select overwrite enable (50G mode). 
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x42_lp_coeff_sel_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x42_lp_coeff_sel_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x42_lp_coeff_sel_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x42_lp_coeff_sel_ow_get
* 
* Link partner coefficient select overwrite (50G mode)
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x42_lp_coeff_sel_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x42_lp_coeff_sel_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x42_lp_coeff_sel_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x42_lp_coeff_req_owen_get
* 
* Link partner coefficient request overwrite enable (50G mode).
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x42_lp_coeff_req_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x42_lp_coeff_req_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x42_lp_coeff_req_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x42_lp_coeff_req_ow_get
* 
* Link partner coefficient request overwrite (50G mode).
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x42_lp_coeff_req_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x42_lp_coeff_req_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x42_lp_coeff_req_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x42_rsvd186_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x42_rsvd186_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x43_lp_mod_prec_sts_owen_get
* 
* Link partner modulation and precoding status overwrite enable (50G mode).
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x43_lp_mod_prec_sts_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x43_lp_mod_prec_sts_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x43_lp_mod_prec_sts_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x43_lp_mod_prec_sts_ow_get
* 
* Link partner modulation and precoding status overwrite (50G mode).
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x43_lp_mod_prec_sts_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x43_lp_mod_prec_sts_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x43_lp_mod_prec_sts_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x43_lp_ic_sts_owen_get
* 
* Link partner initial condition status overwrite enable (50G mode).
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x43_lp_ic_sts_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x43_lp_ic_sts_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x43_lp_ic_sts_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x43_lp_ic_sts_ow_get
* 
* Link partner initial condition status overwrite (50G mode).
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x43_lp_ic_sts_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x43_lp_ic_sts_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x43_lp_ic_sts_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x43_lp_coeff_sel_sts_owen_get
* 
* Link partner coefficient select echo overwrite enable (50G mode).
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x43_lp_coeff_sel_sts_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x43_lp_coeff_sel_sts_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x43_lp_coeff_sel_sts_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x43_lp_coeff_sel_sts_ow_get
* 
* Link partner coefficient select echo overwrite (50G mode).
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x43_lp_coeff_sel_sts_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x43_lp_coeff_sel_sts_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x43_lp_coeff_sel_sts_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x43_lp_coeff_sts_owen_get
* 
* Link partner coefficient status overwrite enable (50G mode).
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x43_lp_coeff_sts_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x43_lp_coeff_sts_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x43_lp_coeff_sts_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x43_lp_coeff_sts_ow_get
* 
* Link partner coefficient status overwrite (50G mode).
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x43_lp_coeff_sts_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x43_lp_coeff_sts_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x43_lp_coeff_sts_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x43_lp_rcv_tf_lock_sts_owen_get
* 
* Link partner receiver frame lock overwrite enable (50G mode).
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x43_lp_rcv_tf_lock_sts_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x43_lp_rcv_tf_lock_sts_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x43_lp_rcv_tf_lock_sts_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x43_lp_rcv_tf_lock_sts_ow_get
* 
* Link partner receiver frame lock overwrite (50G mode).
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x43_lp_rcv_tf_lock_sts_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x43_lp_rcv_tf_lock_sts_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x43_lp_rcv_tf_lock_sts_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x43_rsvd187_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x43_rsvd187_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x44_ld_ic_req_owen_get
* 
* Local device initial condition request overwrite enable (50G mode).
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x44_ld_ic_req_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x44_ld_ic_req_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x44_ld_ic_req_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x44_ld_ic_req_ow_get
* 
* Local device initial condition request overwrite (50G mode).
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x44_ld_ic_req_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x44_ld_ic_req_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x44_ld_ic_req_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x44_ld_mod_prec_req_owen_get
* 
* Local device modulation and precoding request overwrite enable (50G mode).
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x44_ld_mod_prec_req_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x44_ld_mod_prec_req_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x44_ld_mod_prec_req_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x44_ld_mod_prec_req_ow_get
* 
* Local device modulation and precoding request overwrite (50G mode).
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x44_ld_mod_prec_req_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x44_ld_mod_prec_req_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x44_ld_mod_prec_req_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x44_ld_coeff_sel_owen_get
* 
* Local device coefficient select overwrite enable (50G mode).
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x44_ld_coeff_sel_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x44_ld_coeff_sel_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x44_ld_coeff_sel_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x44_ld_coeff_sel_ow_get
* 
* Local device coefficient select overwrite (50G mode).
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x44_ld_coeff_sel_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x44_ld_coeff_sel_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x44_ld_coeff_sel_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x44_ld_coeff_req_owen_get
* 
* Local device coefficient request overwrite enable (50G mode).
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x44_ld_coeff_req_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x44_ld_coeff_req_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x44_ld_coeff_req_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x44_ld_coeff_req_ow_get
* 
* Local device coefficient request overwrite (50G mode).
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x44_ld_coeff_req_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x44_ld_coeff_req_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x44_ld_coeff_req_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x44_ld_rcv_tf_lock_sts_owen_get
* 
* Local device receiver frame lock overwrite enable (50G mode).
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x44_ld_rcv_tf_lock_sts_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x44_ld_rcv_tf_lock_sts_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x44_ld_rcv_tf_lock_sts_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x44_ld_rcv_tf_lock_sts_ow_get
* 
* Local device receiver frame lock overwrite (50G mode).
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x44_ld_rcv_tf_lock_sts_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x44_ld_rcv_tf_lock_sts_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x44_ld_rcv_tf_lock_sts_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x44_rsvd188_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x44_rsvd188_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x45_ld_mod_prec_sts_owen_get
* 
* Local device modulation and precoding status overwrite enable (50G mode).
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x45_ld_mod_prec_sts_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x45_ld_mod_prec_sts_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x45_ld_mod_prec_sts_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x45_ld_mod_prec_sts_ow_get
* 
* Local device modulation and precoding status overwrite (50G mode).
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x45_ld_mod_prec_sts_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x45_ld_mod_prec_sts_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x45_ld_mod_prec_sts_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x45_ld_ic_sts_owen_get
* 
* Local device initial condition status overwrite enable (50G mode).
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x45_ld_ic_sts_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x45_ld_ic_sts_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x45_ld_ic_sts_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x45_ld_ic_sts_ow_get
* 
* Local device initial condition status overwrite (50G mode).
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x45_ld_ic_sts_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x45_ld_ic_sts_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x45_ld_ic_sts_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x45_ld_coeff_sel_sts_owen_get
* 
* Local device coefficient select echo overwrite enable (50G mode).
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x45_ld_coeff_sel_sts_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x45_ld_coeff_sel_sts_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x45_ld_coeff_sel_sts_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x45_ld_coeff_sel_sts_ow_get
* 
* Local device coefficient select echo overwrite (50G mode).
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x45_ld_coeff_sel_sts_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x45_ld_coeff_sel_sts_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x45_ld_coeff_sel_sts_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x45_ld_coeff_sts_owen_get
* 
* Local device coefficient status overwrite enable (50G mode).
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x45_ld_coeff_sts_owen_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x45_ld_coeff_sts_owen_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x45_ld_coeff_sts_owen_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x45_ld_coeff_sts_ow_get
* 
* Local device coefficient status overwrite (50G mode).
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x45_ld_coeff_sts_ow_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x45_ld_coeff_sts_ow_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x45_ld_coeff_sts_ow_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x45_rsvd189_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x45_rsvd189_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x4e_seed_12_11_get
* 
* CL136/137 PRBS 2 MSB seed value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x4e_seed_12_11_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x4e_seed_12_11_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x4e_seed_12_11_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x4e_rsvd190_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x4e_rsvd190_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x4e_polynomial_identifier_get
* 
* Used to select PRBS polynomial.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x4e_polynomial_identifier_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x4e_polynomial_identifier_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x4e_polynomial_identifier_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x4e_seed_10_0_get
* 
* Seed value for all PRBS.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x4e_seed_10_0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x4e_seed_10_0_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x4e_seed_10_0_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x4f_record_25g_en_get
* 
* Training frame record enable for 25G mode.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x4f_record_25g_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x4f_record_25g_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x4f_record_25g_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x4f_record_clear_25g_get
* 
* Training frame record clear for 25G mode.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x4f_record_clear_25g_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x4f_record_clear_25g_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x4f_record_clear_25g_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x4f_cntr_25g_start_get
* 
* Training frame record start address for 25G mode.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x4f_cntr_25g_start_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x4f_cntr_25g_start_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x4f_cntr_25g_start_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x50_record_frame_25g_119_104_get
* 
* Training frame record field for 25G mode, bits 119:104.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x50_record_frame_25g_119_104_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x51_record_frame_25g_103_88_get
* 
* Training frame record field for 25G mode, bits 103:88.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x51_record_frame_25g_103_88_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x52_record_frame_25g_87_72_get
* 
* Training frame record field for 25G mode, bits 87:72.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x52_record_frame_25g_87_72_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x53_record_frame_25g_71_56_get
* 
* Training frame record field for 25G mode, bits 71:56.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x53_record_frame_25g_71_56_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x54_record_frame_25g_55_40_get
* 
* Training frame record field for 25G mode, bits 55:40.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x54_record_frame_25g_55_40_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x55_record_frame_25g_39_24_get
* 
* Training frame record field for 25G mode, bits 39:24.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x55_record_frame_25g_39_24_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x56_record_frame_25g_23_8_get
* 
* Training frame record field for 25G mode, bits 23:8.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x56_record_frame_25g_23_8_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x57_record_frame_25g_7_0_get
* 
* Training frame record field for 25G mode, bits 7:0.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x57_record_frame_25g_7_0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x57_rsvd191_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x57_rsvd191_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x58_record_50g_en_get
* 
* Training frame record enable for 25G mode.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x58_record_50g_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x58_record_50g_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x58_record_50g_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x58_record_clear_50g_get
* 
* Training frame record clear for 25G mode.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x58_record_clear_50g_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x58_record_clear_50g_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x58_record_clear_50g_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x58_cntr_50g_start_get
* 
* Training frame record start frame counter for 25G mode.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x58_cntr_50g_start_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x58_cntr_50g_start_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_link_trng_0x58_cntr_50g_start_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_link_trng_0x59_record_frame_50g_191_176_get
* 
* Training frame record field for 50G mode, bits 191:176.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x59_record_frame_50g_191_176_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x5a_record_frame_50g_175_160_get
* 
* Training frame record field for 50G mode, bits 175:160.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x5a_record_frame_50g_175_160_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x5b_record_frame_50g_159_144_get
* 
* Training frame record field for 50G mode, bits 159:144.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x5b_record_frame_50g_159_144_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x5c_record_frame_50g_143_128_get
* 
* Training frame record field for 50G mode, bits 143:128.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x5c_record_frame_50g_143_128_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x5d_record_frame_50g_127_112_get
* 
* Training frame record field for 50G mode, bits 127:112.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x5d_record_frame_50g_127_112_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x5e_record_frame_50g_111_96_get
* 
* Training frame record field for 50G mode, bits 111:96.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x5e_record_frame_50g_111_96_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x5f_record_frame_50g_95_80_get
* 
* Training frame record field for 50G mode, bits 95:80.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x5f_record_frame_50g_95_80_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x60_record_frame_50g_79_64_get
* 
* Training frame record field for 50G mode, bits 79:64.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x60_record_frame_50g_79_64_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x61_record_frame_50g_63_48_get
* 
* Training frame record field for 50G mode, bits 63:48.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x61_record_frame_50g_63_48_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x62_record_frame_50g_47_32_get
* 
* Training frame record field for 50G mode, bits 47:32.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x62_record_frame_50g_47_32_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x63_record_frame_50g_31_16_get
* 
* Training frame record field for 50G mode, bits 31:16.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x63_record_frame_50g_31_16_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_link_trng_0x64_record_frame_50g_15_0_get
* 
* Training frame record field for 50G mode, bits 15:0.
*********************************************************************/
bf_status_t bf_ll_serdes_link_trng_0x64_record_frame_50g_15_0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_ieee_0x1_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_an_ieee_0x1_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_ieee_0x1_link_training_done_get
* 
* Train done signal
*********************************************************************/
bf_status_t bf_ll_serdes_an_ieee_0x1_link_training_done_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_ieee_0x1_an_finished_get
* 
* Auto-negotiation complete
*********************************************************************/
bf_status_t bf_ll_serdes_an_ieee_0x1_an_finished_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_ieee_0x1_aa_hcd_resolved_get
* 
* Auto-negotiation done signal
*********************************************************************/
bf_status_t bf_ll_serdes_an_ieee_0x1_aa_hcd_resolved_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_ieee_0x2_company_oui_15_0_get
* 
* OUI[16:1] for all active autoneg's OUI register. 
* 8 auto-negotiation on side A side.
* 8 auto-negotiation on side B side.
*********************************************************************/
bf_status_t bf_ll_serdes_an_ieee_0x2_company_oui_15_0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_ieee_0x2_company_oui_15_0_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_ieee_0x2_company_oui_15_0_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_ieee_0x3_company_oui_23_16_get
* 
* Credo's OUI[24:17] for all active auto-negotiation.
* 8 auto-negotiation on side A side.
* 8 auto-negotiation on side B side.
*********************************************************************/
bf_status_t bf_ll_serdes_an_ieee_0x3_company_oui_23_16_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_ieee_0x3_company_oui_23_16_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_ieee_0x3_company_oui_23_16_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_ieee_0x3_rsvd192_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_an_ieee_0x3_rsvd192_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_ieee_0x4_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_an_ieee_0x4_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_ieee_0x4_model_num_get
* 
* 6-bits model number for all active auto-negotiation.
* 8 autoneg on A side, 8 autoneg on B side)
*********************************************************************/
bf_status_t bf_ll_serdes_an_ieee_0x4_model_num_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_ieee_0x4_model_num_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_ieee_0x4_model_num_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_ieee_0x4_revision_num_get
* 
* 4-bits revision number for all active auto-negotiation (8 autoneg on A side, 8 autoneg on B side)
*********************************************************************/
bf_status_t bf_ll_serdes_an_ieee_0x4_revision_num_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_ieee_0x4_revision_num_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_ieee_0x4_revision_num_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_ieee_0x5_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_an_ieee_0x5_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_ieee_0x5_pcs_advertise_consortium_50g_s_get
* 
* Default value of side A, lane 0 auto-negotiation for 25G Ethernet Consortium NP extended technology ability bits 50GBase-CR2/50Gbase-KR2.
*********************************************************************/
bf_status_t bf_ll_serdes_an_ieee_0x5_pcs_advertise_consortium_50g_s_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_ieee_0x5_pcs_advertise_consortium_50g_s_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_ieee_0x5_pcs_advertise_consortium_50g_s_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_ieee_0x5_pcs_advertise_consortium_25g_s_get
* 
* Default value of  side A side, lane 0 auto-negotiation for 25G Ethernet Consortium NP extended technology ability bits 25GBase-CR1/25Gbase-KR1.
*********************************************************************/
bf_status_t bf_ll_serdes_an_ieee_0x5_pcs_advertise_consortium_25g_s_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_ieee_0x5_pcs_advertise_consortium_25g_s_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_ieee_0x5_pcs_advertise_consortium_25g_s_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_ieee_0x5_rsvd193_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_an_ieee_0x5_rsvd193_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_ieee_0x5_pcs_advertise_ability_s_22_16_get
* 
* Default value of side A side, lane 0 auto-negotiation BP technology ability bits A[22:16] 
*********************************************************************/
bf_status_t bf_ll_serdes_an_ieee_0x5_pcs_advertise_ability_s_22_16_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_ieee_0x5_pcs_advertise_ability_s_22_16_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_ieee_0x5_pcs_advertise_ability_s_22_16_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_ieee_0x6_pcs_advertise_ability_s_15_0_get
* 
* Default value of side A side, lane 0 auto-negotiation BP technology ability bits A[15:0] 
*********************************************************************/
bf_status_t bf_ll_serdes_an_ieee_0x6_pcs_advertise_ability_s_15_0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_ieee_0x6_pcs_advertise_ability_s_15_0_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_ieee_0x6_pcs_advertise_ability_s_15_0_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_ieee_0x7_pcs_advertise_consortium_fec_s_get
* 
* Default value of side A side, lane 0 auto-negotiation  for 25G Ethernet Consortium NP extended technology ability bits F4/F3/F2/F1
*********************************************************************/
bf_status_t bf_ll_serdes_an_ieee_0x7_pcs_advertise_consortium_fec_s_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_ieee_0x7_pcs_advertise_consortium_fec_s_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_ieee_0x7_pcs_advertise_consortium_fec_s_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_ieee_0x7_pcs_advertise_fec_s_get
* 
* Default value of side A side, lane 0 auto-negotiation BP FEC capability bits F1/F0/F3/F2
*********************************************************************/
bf_status_t bf_ll_serdes_an_ieee_0x7_pcs_advertise_fec_s_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_ieee_0x7_pcs_advertise_fec_s_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_ieee_0x7_pcs_advertise_fec_s_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_ieee_0x7_rsvd194_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_an_ieee_0x7_rsvd194_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_ieee_0x8_rsvd_get
* 
* Reserved. Do not change the default value.
*********************************************************************/
bf_status_t bf_ll_serdes_an_ieee_0x8_rsvd_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

/*********************************************************************
* bf_ll_serdes_an_ieee_0x8_tx_nonce_seed_get
* 
* XOR groupnumber for TX seed
*********************************************************************/
bf_status_t bf_ll_serdes_an_ieee_0x8_tx_nonce_seed_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_ieee_0x8_tx_nonce_seed_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_ieee_0x8_tx_nonce_seed_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_ieee_0x8_pcs_restart_autoneg_s_get
* 
* Auto-negotiation reset function
*********************************************************************/
bf_status_t bf_ll_serdes_an_ieee_0x8_pcs_restart_autoneg_s_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_ieee_0x8_pcs_restart_autoneg_s_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_ieee_0x8_pcs_restart_autoneg_s_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_ieee_0x8_pcs_autoneg_s_en_get
* 
* Value of Auto-negotiation IEEE 0x00[13]
*********************************************************************/
bf_status_t bf_ll_serdes_an_ieee_0x8_pcs_autoneg_s_en_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_ieee_0x8_pcs_autoneg_s_en_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_ieee_0x8_pcs_autoneg_s_en_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_ieee_0x8_mode_25g_get
* 
* Select Serdes speed mode for Auto-negotiation:
*     1: Serdes runs at 25G;
*     0: Serdes runs at 10G;
*********************************************************************/
bf_status_t bf_ll_serdes_an_ieee_0x8_mode_25g_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_ieee_0x8_mode_25g_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_ieee_0x8_mode_25g_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_ieee_0x9_pcs_link_status_31_16_get
* 
* Used by A side lane 0, auto-negotiation as link_status_[*] variable. Each bit represent one type of link status.
* 
* These bits are defined for exact PCS yet, reserved for future use.
*********************************************************************/
bf_status_t bf_ll_serdes_an_ieee_0x9_pcs_link_status_31_16_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_ieee_0x9_pcs_link_status_31_16_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_ieee_0x9_pcs_link_status_31_16_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_ieee_0xa_pcs_link_status_15_0_get
* 
* Used by A side lane, 0 auto-negotiation as a link_status_[*] variable. Each bit represents one type of link status as follows:
* 
* [0 ]=LINK_STATUS_[1G_KX_BIT]
* [1 ]=LINK_STATUS_[10G_KX4_BIT]
* [2 ]=LINK_STATUS_[10G_KR_BIT]
* [3 ]=LINK_STATUS_[40G_KR4_BIT]
* [4 ]=LINK_STATUS_[40G_CR4_BIT]
* [5 ]=LINK_STATUS_[100G_CR10_BIT]
* [6 ]=LINK_STATUS_[100G_KP4_BIT]
* [7 ]=LINK_STATUS_[100G_KR4_BIT]
* [8 ]=LINK_STATUS_[100G_CR4_BIT]
* [9 ]=LINK_STATUS_[25G_KRS_CRS_BIT]
* [10]=LINK_STATUS_[25G_KR_CR_BIT]
* [11]=LINK_STATUS_[2P5G_KX_BIT]
* [12]=LINK_STATUS_[5G_KR_BIT]
* [13]=LINK_STATUS_[50G_KR_CR_BIT]
* [14]=LINK_STATUS_[100G_KR2_CR2_BIT]
* [15]=LINK_STATUS_[200G_KR4_CR4_BIT]
*********************************************************************/
bf_status_t bf_ll_serdes_an_ieee_0xa_pcs_link_status_15_0_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_ieee_0xa_pcs_link_status_15_0_set(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val,
     uint32_t hw);

bf_status_t bf_ll_serdes_an_ieee_0xa_pcs_link_status_15_0_rmw(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t fld_val);

/*********************************************************************
* bf_ll_serdes_an_ieee_0xb_lane_num_get
* 
* Current lane and group number. Bits 6:3 are the group number. Bits 2:0 are the lane number of this group
*********************************************************************/
bf_status_t bf_ll_serdes_an_ieee_0xb_lane_num_get(bf_dev_id_t dev_id,
     bf_dev_port_t dev_port,
     uint32_t ln,
     uint32_t *reg32,
     uint32_t *fld_val,
     uint32_t hw);

