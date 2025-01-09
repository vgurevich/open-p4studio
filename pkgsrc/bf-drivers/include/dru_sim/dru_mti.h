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


#ifndef DRU_MTI_INCLUDED
#define DRU_MTI_INCLUDED

#include <dru_sim/dru_sim.h>

typedef enum {
  MTI_TYP_RX_PKT_0 = 0,
  MTI_TYP_RX_PKT_1,
  MTI_TYP_RX_PKT_2,
  MTI_TYP_RX_PKT_3,
  MTI_TYP_RX_PKT_4,
  MTI_TYP_RX_PKT_5,
  MTI_TYP_RX_PKT_6,
  MTI_TYP_RX_PKT_7,
  MTI_TYP_LRT,
  MTI_TYP_IDLE,
  MTI_TYP_LEARN_PIPE0,
  MTI_TYP_LEARN_PIPE1,
  MTI_TYP_LEARN_PIPE2,
  MTI_TYP_LEARN_PIPE3,
  MTI_TYP_DIAG,
  MTI_TYP_NUM
} mti_typ_e;

void dru_mti_tx(dru_dev_id_t asic, mti_typ_e data_type, void *data, int len);
void dru_learn(dru_dev_id_t asic,
               uint8_t *learn_filter_data,
               int len,
               int pipe_nbr);
void dru_rx_pkt(dru_dev_id_t asic, uint8_t *pkt, int len, int cos);
void dru_lrt_update(dru_dev_id_t asic, uint8_t *lrt_stat_data, int len);
void dru_idle_update(dru_dev_id_t asic, uint8_t *idle_timeout_data, int len);
void dru_diag_event(dru_dev_id_t asic, uint8_t *diag_data, int len);

typedef void *(*dru_sim_dma2virt_dbg_callback_fn_mti)(dru_dev_id_t asic,
                                                      bf_dma_addr_t addr);
void dru_mti_register_dma2virt_cb(
    dru_sim_dma2virt_dbg_callback_fn_mti dma2virt_fn);

#endif  // DRU_MTI_INCLUDED
