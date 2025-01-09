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

#include <sched.h>
#include <linux/tcp.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>  //strlen
#include <stddef.h>
#include <inttypes.h>  //strlen
#include <sys/socket.h>
#include <arpa/inet.h>  //inet_addr
#include <unistd.h>     //write
#include <errno.h>
#include <target-sys/bf_sal/bf_sys_intf.h>
#include <pkt_mgr/pkt_mgr_intf.h>
#include "diag_common.h"
#include "diag_util.h"
#include "diag_pd.h"
#include "diag_pkt.h"
#include "diag_create_pkt.h"
#include "diag_pkt_eth_cpu.h"
#ifdef THRIFT_ENABLED
extern int start_diag_rpc_server(void **server_cookie);
extern int stop_diag_rpc_server();
static void *diag_rpc_server_cookie;
#endif

/* global */
diag_info_t diag_info;
uint32_t diag_sessions_current_max = DIAG_SESSIONS_MAX_DEFAULT;
static bool g_diag_lib_init_done = false;
extern void diag_register_ucli_node();
extern void diag_unregister_ucli_node();

/* Init device */
bf_status_t diag_device_init(bf_dev_id_t dev_id,
                             const char *eth_cpu_port_name) {
  bf_status_t status = BF_SUCCESS;

  if (DIAG_DEV_INFO(dev_id)) {
    return BF_SUCCESS;
  }

  /* Allocate for device 0 */
  DIAG_DEV_INFO(dev_id) = DIAG_MALLOC(sizeof(diag_dev_info_t));
  if (!DIAG_DEV_INFO(dev_id)) {
    DIAG_PRINT("Unable to allocate memory for dev %d \n", dev_id);
    return 0;
  }
  memset(DIAG_DEV_INFO(dev_id), 0, sizeof(diag_dev_info_t));

  DIAG_DEV_INFO(dev_id)->dev_id = dev_id;
  DIAG_DEV_INFO(dev_id)->kernel_pkt = !bf_pkt_is_inited(dev_id);
  bf_drv_device_type_get(dev_id, &(DIAG_DEV_INFO(dev_id)->is_sw_model));

  DIAG_DEV_INFO(dev_id)->chip_family =
      bf_dev_type_to_family(lld_sku_get_dev_type(dev_id));

  diag_get_num_active_pipes_and_cpu_port(
      dev_id,
      &(DIAG_DEV_INFO(dev_id)->num_active_pipes),
      &(DIAG_DEV_INFO(dev_id)->cpu_port),
      &(DIAG_DEV_INFO(dev_id)->cpu_port2),
      &(DIAG_DEV_INFO(dev_id)->eth_cpu_info.eth_cpu_port));
  /* Get part revision */
  diag_get_part_revision(dev_id, &(DIAG_DEV_INFO(dev_id)->part_rev));
  bf_map_init(&DIAG_SESSION_INFO(dev_id).sess_map);

  /* If using ETH_CPU port add the port */
  diag_eth_cpu_port_init(dev_id, eth_cpu_port_name);

  /* Setup default entries in tables */
  diag_pd_add_default_entries(dev_id);
  diag_pd_power_populate_entries_in_tables(dev_id);
  diag_pd_phv_power_populate_entries_in_tables(dev_id);
  diag_pd_mau_bus_stress_populate_entries_in_tables(dev_id);
  if (DIAG_DEV_INFO(dev_id)->kernel_pkt) {
    status = diag_kernel_setup_pkt_intf(dev_id);
  } else if (!DIAG_DEV_INFO(dev_id)->eth_cpu_info.use_eth_cpu_port) {
    /* Register pkt rx once */
    diag_register_pkt_rx(dev_id);
  }

  /* Add the pkt-gen port */
  diag_pgen_port_add(dev_id);

  return status;
}

bf_status_t diag_device_deinit(bf_dev_id_t dev_id) {
  if (!DIAG_DEV_INFO(dev_id)) {
    return BF_SUCCESS;
  }
  /* Delete all existing sessions */
  diag_session_info_del_all(dev_id, true);

  /* Stop the Eth CPU thread */
  diag_eth_cpu_port_deinit(dev_id);

  /* Free the memory allocated for device 0 */
  DIAG_FREE(DIAG_DEV_INFO(dev_id));

  DIAG_DEV_INFO(dev_id) = NULL;

  return BF_SUCCESS;
}

/* Initialize and start diags */
bf_status_t bf_diag_init(bf_dev_id_t dev_id, const char *eth_cpu_port_name) {
  if (dev_id >= BF_MAX_DEV_COUNT) {
    return BF_INVALID_ARG;
  }

  DIAG_PRINT("bf_diag: Entered Diag lib init for dev %d\n", dev_id);
  if (!g_diag_lib_init_done) {
    memset(&diag_info, 0, sizeof(diag_info));
    /* Register ucli node */
    diag_register_ucli_node();

#ifdef THRIFT_ENABLED
    /* Start the rpc server */
    start_diag_rpc_server(&diag_rpc_server_cookie);
#endif

    g_diag_lib_init_done = true;
  }
  DIAG_PKT_ID_REPLICATION_FACTOR = PKT_ID_REPLICATION_FACTOR_DEF;
  DIAG_MIN_PKT_SIZE_ENABLED = false;
#if defined(DIAG_CREATE_IPV4_PKT)
  DIAG_MIN_PKT_SIZE_ENABLED = true;
#endif

  diag_device_init(dev_id, eth_cpu_port_name);

  return 0;
}

bf_status_t bf_diag_deinit(bf_dev_id_t dev_id) {
  if (dev_id >= BF_MAX_DEV_COUNT) {
    return BF_INVALID_ARG;
  }

  diag_device_deinit(dev_id);

  if (g_diag_lib_init_done && (!diag_any_device_exists())) {
    /* Unregister ucli node */
    diag_unregister_ucli_node();

#ifdef THRIFT_ENABLED
    /* Stop the rpc server */
    stop_diag_rpc_server();
#endif

    memset(&diag_info, 0, sizeof(diag_info));
    diag_sessions_current_max = DIAG_SESSIONS_MAX_DEFAULT;
    g_diag_lib_init_done = false;
  }

  return 0;
}
