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


#include <stddef.h>

#include <dvm/bf_drv_intf.h>
#include "traffic_mgr/common/tm_error.h"
#include "traffic_mgr/common/tm_ctx.h"
#include "traffic_mgr/hw_intf/tm_tofino_hw_intf.h"

#include <tofino_regs/tofino.h>
#include <lld/bf_dma_if.h>
#include <lld/lld_dr_if.h>

bf_status_t bf_tm_tofinolite_start_init_seq_during_fast_recfg(bf_dev_id_t dev) {
  // This function is supposed to invoke init sequence for tofinolite
  dev &= dev;
  return (BF_SUCCESS);
}

/*
 *  This file implements default parameter values for TM.
 *  Defaults are set at the device init time (device-add)
 */
bf_status_t bf_tm_tofinolite_set_default(bf_dev_id_t dev) {
  dev &= dev;
  return (BF_SUCCESS);
}
