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
 * @file pipe_mgr_adt_mgr_dump.h
 * @date
 *
 * Action data table manager dump definitions to dump debug information.
 */

/* Standard header includes */

/* Module header includes */

/* Local header includes */

#include <pipe_mgr/pipe_mgr_intf.h>
#include "pipe_mgr_act_tbl.h"

#include <target-utils/uCli/ucli.h>
#include <target-utils/uCli/ucli_argparse.h>
#include <target-utils/uCli/ucli_handler_macros.h>

void pipe_mgr_adt_dump_tbl_info(ucli_context_t *uc,
                                uint8_t device_id,
                                pipe_adt_tbl_hdl_t adt_tbl_hdl);

void pipe_mgr_adt_dump_entry_info(ucli_context_t *uc,
                                  uint8_t device_id,
                                  pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                  pipe_adt_ent_hdl_t adt_ent_hdl);
