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


///
//  mc_mgr_shared_dr.h
//
#ifndef __MC_MGR_SHARED_DR_H__
#define __MC_MGR_SHARED_DR_H__

#include <stdio.h>

#define BF_MC_DMA_MSG_ID \
  (7)  // Any value other than BF_TM_DMA_MSG_ID becasue
       // both TM and MC write list share same DR.
       // DR level syncronization is done in LLD. No need
       // for another level of locks on DR.
#define BF_TM_DMA_MSG_ID (3)
#define BF_DIAG_DMA_MSG_ID (0)

int mcmgr_tm_register_completion_cb(bf_dev_id_t chip,
                                    bf_dma_dr_id_t dr,
                                    dr_completion_callback_fn fn,
                                    int dr_user_id);

void mcmgr_tm_set_dr_state(bf_dev_id_t dev, bool enable);
bool mcmgr_tm_get_dr_state(bf_dev_id_t dev);
#endif
