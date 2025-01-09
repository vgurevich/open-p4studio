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


#include <stdio.h>
#include <dvm/bf_drv_intf.h>
#include <tofino/pdfixed/pd_devport_mgr.h>

p4_pd_status_t p4_devport_mgr_add_port(uint32_t dev_id,
                                       uint32_t port_id,
                                       uint32_t port_speeds,
                                       uint32_t port_fec_types) {
  return bf_port_add(dev_id, port_id, port_speeds, port_fec_types);
}

p4_pd_status_t p4_devport_mgr_add_port_with_lanes(uint32_t dev_id,
                                                  uint32_t port_id,
                                                  uint32_t port_speeds,
                                                  uint32_t port_lanes,
                                                  uint32_t port_fec_types) {
  return bf_port_add_with_lane_numb(
      dev_id, port_id, port_speeds, port_lanes, port_fec_types);
}

p4_pd_status_t p4_devport_mgr_remove_port(uint32_t dev_id, uint32_t port_id) {
  return bf_port_remove(dev_id, port_id);
}

p4_pd_status_t p4_devport_mgr_enable_port(uint32_t dev_id,
                                          uint32_t port_id,
                                          bool enable) {
  return bf_port_enable(dev_id, port_id, enable);
}

int p4_devport_mgr_port_ca_get(const int32_t dev_id, const int32_t port_id) {
#ifdef PORT_MGR_HA_UNIT_TESTING
  bf_ha_corrective_action_t ca;
  if (bf_ha_port_mgr_port_corrective_action_get(dev_id, port_id, &ca) != 0) {
    return -1;
  }
  return ca;
#endif
  (void)dev_id;
  (void)port_id;
  return 0;
}

int p4_devport_mgr_serdes_ca_get(const int32_t dev_id, const int32_t port_id) {
#ifdef PORT_MGR_HA_UNIT_TESTING
  bf_ha_corrective_action_t ca;
  if (bf_ha_port_mgr_serdes_corrective_action_get(dev_id, port_id, &ca) != 0) {
    return -1;
  }
  return ca;
#endif
  (void)dev_id;
  (void)port_id;
  return 0;
}

/* HACK to execute runner functions over thrift.
 * Should move to runner's thrift service
 */
warm_init_begin_fn warm_init_begin;
warm_init_end_fn warm_init_end;

p4_pd_status_t p4_devport_mgr_register_test_handlers(warm_init_begin_fn f1,
                                                     warm_init_end_fn f2) {
  if (f1) {
    warm_init_begin = f1;
  }
  if (f2) {
    warm_init_end = f2;
  }
  return 0;
}

p4_pd_status_t p4_devport_mgr_warm_init_begin(
    uint32_t dev_id,
    p4_devport_mgr_warm_init_mode_e warm_init_mode,
    p4_devport_mgr_serdes_upgrade_mode_e serdes_upgrade_mode,
    bool upgrade_agents) {
  return warm_init_begin(dev_id,
                         (p4_devport_mgr_warm_init_mode_e)warm_init_mode,
                         serdes_upgrade_mode,
                         upgrade_agents);
}

p4_pd_status_t p4_devport_mgr_warm_init_end(uint32_t dev_id) {
  return warm_init_end(dev_id);
}

p4_pd_status_t p4_devport_mgr_set_copy_to_cpu(bf_dev_id_t dev,
                                              bool enable,
                                              uint16_t port) {
  return bf_set_copy_to_cpu(dev, enable, port);
}

int p4_devport_mgr_pcie_cpu_port_get(bf_dev_id_t dev) {
  return bf_pcie_cpu_port_get(dev);
}

int p4_devport_mgr_eth_cpu_port_get(bf_dev_id_t dev) {
  return bf_eth_cpu_port_get(dev);
}

p4_pd_status_t p4_devport_mgr_set_virtual_dev_slave_mode(uint32_t dev_id) {
  return bf_device_set_virtual_dev_slave_mode(dev_id);
}

p4_pd_status_t p4_devport_mgr_get_clock_speed(uint32_t dev_id,
                                              uint64_t *bps_clock_speed,
                                              uint64_t *pps_clock_speed) {
  return bf_drv_get_clock_speed(dev_id, bps_clock_speed, pps_clock_speed);
}

p4_pd_status_t p4_devport_mgr_lrt_dr_timeout_set(uint32_t dev_id,
                                                 uint32_t timeout_ms) {
  return bf_drv_lrt_dr_timeout_set(dev_id, timeout_ms);
}

p4_pd_status_t p4_devport_mgr_lrt_dr_timeout_get(uint32_t dev_id,
                                                 uint32_t *timeout_ms) {
  return bf_drv_lrt_dr_timeout_get(dev_id, timeout_ms);
}
