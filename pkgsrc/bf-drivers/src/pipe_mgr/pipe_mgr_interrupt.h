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


/*!
 * @file pipe_mgr_interrupt.h
 * @date
 *
 * Contains definitions of pipe-mgr interrupt handling
 *
 */
#ifndef _PIPE_MGR_INTERRUPT_H
#define _PIPE_MGR_INTERRUPT_H

/* Module header includes */
#include <pipe_mgr/pipe_mgr_intf.h>
#include <target-sys/bf_sal/bf_sys_log.h>
#include <target-sys/bf_sal/bf_sys_timer.h>
#include "pipe_mgr_int.h"

/* ------ FUNCTIONS -------- */
/**
 * The function is used to set the error interrupt mode
 *
 * @param  dev                   ASIC device identifier
 * @param  enable                Enable/Disable interrupt mode
 * @return                       Status of the API call
 */
bf_status_t pipe_mgr_err_interrupt_mode_set(bf_dev_id_t dev, bool enable);

/**
 * The function is used to register interrupt notifications
 *
 * @param  dev                  ASIC device identifier
 * @return                       Status of the API call
 */
pipe_status_t pipe_mgr_register_interrupt_notifs(rmt_dev_info_t *dev_info);
pipe_status_t pipe_mgr_enable_interrupt_notifs(rmt_dev_info_t *dev_info);

pipe_status_t pipe_mgr_intr_init(rmt_dev_info_t *dev_info);
pipe_status_t pipe_mgr_intr_start_scrub_timer(bf_dev_id_t dev_id);
pipe_status_t pipe_mgr_intr_cleanup(bf_dev_id_t dev);

/**
 * The function is used to dump all error events
 *
 * @param  uc                    ucli handle
 * @param  dev                   ASIC device identifier
 * @param  n                     Display n most recent events, -1 for all
 * @return                       None
 */
void pipe_mgr_err_evt_log_dump(ucli_context_t *uc, bf_dev_id_t dev_id, int n);
#endif
