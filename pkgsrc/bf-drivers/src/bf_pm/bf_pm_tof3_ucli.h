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


#ifndef _BF_PM_TOF3_UCLI_H
#define _BF_PM_TOF3_UCLI_H

ucli_status_t bf_pm_ucli_ucli__tof3_command_start__(ucli_context_t *uc);
ucli_status_t bf_pm_ucli_ucli__tof3_port_pcs__(ucli_context_t *uc);
ucli_status_t bf_pm_ucli_ucli__tof3_port_fec__(ucli_context_t *uc);
ucli_status_t bf_pm_ucli_ucli__tof3_port_fec_mon__(ucli_context_t *uc);
ucli_status_t bf_pm_ucli_ucli__tof3_port_int__(ucli_context_t *uc);
ucli_status_t bf_pm_ucli_ucli__tof3_port_anlt__(ucli_context_t *uc);
ucli_status_t bf_pm_ucli_ucli__tof3_port_glue__(ucli_context_t *uc);
ucli_status_t bf_pm_ucli_ucli__tof3_port_serdes_debug__(ucli_context_t *uc);
void pm_ucli_tof3_pcs_status(ucli_context_t *uc,
                             bf_dev_id_t dev_id,
                             int aflag,
                             bf_pal_front_port_handle_t *port_hdl);

#endif
