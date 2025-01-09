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


#ifndef __MC_MGR_PIPE_INTF_H__
#define __MC_MGR_PIPE_INTF_H__

#include <mc_mgr/mc_mgr_intf.h>

bf_status_t mc_mgr_ecc_correct_pvt(bf_dev_id_t dev,
                                   bf_dev_pipe_t pipe,
                                   uint16_t row,
                                   bool batch);

bf_status_t mc_mgr_ecc_correct_mit(bf_dev_id_t dev,
                                   bf_dev_pipe_t pipe,
                                   uint32_t address);
bf_status_t mc_mgr_ecc_correct_lit_bm(bf_dev_id_t dev,
                                      int ver,
                                      uint32_t address);

bf_status_t mc_mgr_ecc_correct_lit_np(bf_dev_id_t dev,
                                      int ver,
                                      uint32_t address);

bf_status_t mc_mgr_ecc_correct_pmt(bf_dev_id_t dev, int ver, uint32_t address);

bf_status_t mc_mgr_ecc_correct_rdm(bf_dev_id_t dev, uint32_t address);

#endif
