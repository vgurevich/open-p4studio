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


#include <stdarg.h>

#include <bf_types/bf_types.h>
#include <dvm/bf_drv_intf.h>
#include <dvm/dvm_intf.h>
#include <tof2_regs/tof2_reg_drv.h>
#include <lld/lld_reg_if.h>
#include <port_mgr/port_mgr_intf.h>
#include <port_mgr/bf_port_if.h>
#include <port_mgr/port_mgr_log.h>
//#include "eth400g_mac_rspec_access.h"
//#include "eth400g_pcs_rspec_access.h"

/** \brief port_mgr_tof2_init:
 *
 * Initialize port_mgr module for Tofino2
 */
void port_mgr_tof2_init(void) {}

/***********************************************************************
 * autogen_log
 *
 * wrapper for logs in auto-generated code. This is mainly to control
 * the amount of trace logs generated.
 *
 * The typical usage would be to set autogen_log_en = true then run
 * some set of functions and set autogen_log_en = false, to disable
 * further tracing.
 *
 * Leaving tracing enabled will significantly slow down system operation
 * and will cause rapid overflof of the bf_drivers.log files.
 ************************************************************************
 */
bool autogen_log_en = false;

int autogen_log(const char *fmt, ...) {
  if (!autogen_log_en) {
    return 0;
  } else {
    char log_string[128];

    va_list args;
    va_start(args, fmt);
    vsnprintf(log_string, sizeof(log_string) - 1, fmt, args);
    va_end(args);
    port_mgr_log("%s", log_string);
  }
  return 0;
}
