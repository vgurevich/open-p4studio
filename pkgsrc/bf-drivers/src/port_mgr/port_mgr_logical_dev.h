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


#ifndef port_mgr_logical_dev_h_included
#define port_mgr_logical_dev_h_included

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

// Mapping of pipes and ports can be different for different chips
#define PORT_MGR_MAX_LOGICAL_FRONT_PORT (256 * 2)
#define PORT_MGR_MAX_LOGICAL_CPU_PORT (8)

/** \typedef port_mgr_ldev_t:
 *
 */
typedef struct port_mgr_ldev_t {
  bf_dev_family_t dev_family;
  bf_sku_chip_part_rev_t part_rev;       // Chip part revision number
  bool assigned;                         // true = in-use
  bool ready;                            // true = chip ready to use
  bool dev_locked;                       // true = HA in-progress
  bool interrupt_based_link_monitoring;  // Local/Remote Fault interrupts used
  bool no_auto_adaptive_tuning;          // dont start adaptive PCAL on port-up
  struct bf_dma_info_s dma_info;         // from device add

  // per-device callbacks
  port_mgr_port_callback_t port_cb;
  void *port_cb_userdata;  // context ffor port_cb
  bf_mac_stats_callback_t mac_stats_callback_fn;
  bf_port_intbh_wakeup_callback_t port_intbh_wakeup_cb;
  bf_sys_cmp_and_swp_t port_int_flag;  // port interrupt status presence
  bool port_intr_bhalf_valid;  // flag to determine whether bottom half for
                               // interrupt processing is enabled or not

  // storage for all possible (port_mgr supported) logical ports on a logical
  // device (incl CPU ports).
  // Mapping of { dev_id, dev_port } -> logical_port is via device-type
  // dependent mapping function as follows,
  //
  //   0-255, front-ports
  // 256-260, CPU ports
  //
  // not sure which way to go yet, so keep enuf space in lport for cpu ports as
  // well
  port_mgr_port_t
      lport[PORT_MGR_MAX_LOGICAL_FRONT_PORT + PORT_MGR_MAX_LOGICAL_CPU_PORT];
  //  port_mgr_port_t lcpu_port[PORT_MGR_MAX_LOGICAL_CPU_PORT];

  // HA
  port_mgr_ha_stages_t ha_stage;
  // Init mode of the device (cold boot/fast reconfig/hitless HA)
  bf_dev_init_mode_t init_mode;
  bf_ha_port_reconcile_info_per_device_t port_mgr_port_recon_info;
#ifdef PORT_MGR_HA_UNIT_TESTING
  bf_ha_port_reconcile_info_per_device_t port_mgr_mac_recon_info;
  bf_ha_port_reconcile_info_per_device_t port_mgr_serdes_recon_info;
#endif

} port_mgr_ldev_t;

port_mgr_ldev_t *port_mgr_dev_logical_dev_get(bf_dev_id_t dev_id);

#ifdef __cplusplus
}
#endif /* C++ */

#endif
