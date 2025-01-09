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
#include <string.h>
#include <knet_mgr/bf_knet_if.h>
#include <tofino/pdfixed/pd_knet_mgr.h>

p4_pd_status_t p4_knet_cpuif_ndev_add(const char *cpuif_netdev_name,
                                      char *cpuif_knetdev_name,
                                      p4_pd_knet_cpuif_t *knet_cpuif_id) {
  return bf_knet_cpuif_ndev_add(
      cpuif_netdev_name, cpuif_knetdev_name, knet_cpuif_id);
}
p4_pd_status_t p4_knet_cpuif_ndev_delete(
    const p4_pd_knet_cpuif_t knet_cpuif_id) {
  return bf_knet_cpuif_ndev_delete(knet_cpuif_id);
}

p4_pd_status_t p4_knet_hostif_kndev_add(const p4_pd_knet_cpuif_t knet_cpuif_id,
                                        const char *hostif_name,
                                        p4_pd_knet_hostif_t *knet_hostif_id) {
  bf_knet_hostif_knetdev_t hostif_knetdev;
  p4_pd_status_t status;
  strncpy(hostif_knetdev.name, hostif_name, IFNAMSIZ - 1);
  status = bf_knet_hostif_kndev_add(knet_cpuif_id, &hostif_knetdev);
  *knet_hostif_id = hostif_knetdev.knet_hostif_id;
  return status;
}

p4_pd_status_t p4_knet_hostif_kndev_delete(
    const p4_pd_knet_cpuif_t knet_cpuif_id,
    const p4_pd_knet_hostif_t knet_hostif_id) {
  return bf_knet_hostif_kndev_delete(knet_cpuif_id, knet_hostif_id);
}

p4_pd_status_t p4_knet_rx_filter_add(
    const p4_pd_knet_cpuif_t knet_cpuif_id,
    p4_pd_knet_rx_filter_t *const knet_rx_filter) {
  return bf_knet_rx_filter_add(knet_cpuif_id, knet_rx_filter);
}

p4_pd_status_t p4_knet_rx_filter_delete(const p4_pd_knet_cpuif_t knet_cpuif_id,
                                        const p4_pd_knet_filter_t filter_id) {
  return bf_knet_rx_filter_delete(knet_cpuif_id, filter_id);
}

p4_pd_status_t p4_knet_tx_action_add(const p4_pd_knet_cpuif_t knet_cpuif_id,
                                     const p4_pd_knet_hostif_t knet_hostif_id,
                                     p4_pd_knet_tx_action_t *const tx_action) {
  return bf_knet_tx_action_add(knet_cpuif_id, knet_hostif_id, tx_action);
}

p4_pd_status_t p4_knet_tx_action_delete(
    const p4_pd_knet_cpuif_t knet_cpuif_id,
    const p4_pd_knet_hostif_t knet_hostif_id) {
  return bf_knet_tx_action_delete(knet_cpuif_id, knet_hostif_id);
}

bool p4_knet_module_is_inited() { return bf_knet_module_is_inited(); }

p4_pd_status_t p4_knet_get_cpuif_cnt(p4_pd_knet_count_t *const obj_count) {
  return bf_knet_get_cpuif_cnt(obj_count);
}

p4_pd_status_t p4_knet_get_hostif_cnt(const p4_pd_knet_cpuif_t knet_cpuif_id,
                                      p4_pd_knet_count_t *const obj_count) {
  return bf_knet_get_hostif_cnt(knet_cpuif_id, obj_count);
}

p4_pd_status_t p4_knet_get_rx_filter_cnt(const p4_pd_knet_cpuif_t knet_cpuif_id,
                                         p4_pd_knet_count_t *const obj_count) {
  return bf_knet_get_rx_filter_cnt(knet_cpuif_id, obj_count);
}

p4_pd_status_t p4_knet_get_rx_mutation_cnt(
    const p4_pd_knet_cpuif_t knet_cpuif_id,
    const p4_pd_knet_filter_t filter_id,
    p4_pd_knet_count_t *const obj_count) {
  return bf_knet_get_rx_mutation_cnt(knet_cpuif_id, filter_id, obj_count);
}

p4_pd_status_t p4_knet_get_tx_mutation_cnt(
    const p4_pd_knet_cpuif_t knet_cpuif_id,
    const p4_pd_knet_hostif_t hostif_id,
    p4_pd_knet_count_t *const obj_count) {
  return bf_knet_get_tx_mutation_cnt(knet_cpuif_id, hostif_id, obj_count);
}

p4_pd_status_t p4_knet_rx_filter_get(const p4_pd_knet_cpuif_t knet_cpuif_id,
                                     const p4_pd_knet_filter_t filter_id,
                                     p4_pd_knet_rx_filter_t *rx_filter,
                                     p4_pd_knet_count_t rx_mutation_count) {
  return bf_knet_rx_filter_get(
      knet_cpuif_id, filter_id, rx_filter, rx_mutation_count);
}

p4_pd_status_t p4_knet_get_rx_filter_list(
    const p4_pd_knet_cpuif_t knet_cpuif_id,
    p4_pd_knet_filter_t *const filter_list,
    p4_pd_knet_count_t *const filter_count) {
  return bf_knet_rx_filter_list_get(knet_cpuif_id, filter_list, filter_count);
}

p4_pd_status_t p4_knet_get_cpuif_list(p4_pd_knet_cpuif_list_t *const cpuif_list,
                                      p4_pd_knet_count_t *const cpuif_count) {
  return bf_knet_cpuif_list_get(cpuif_list, cpuif_count);
}

p4_pd_status_t p4_knet_get_hostif_list(
    p4_pd_knet_cpuif_t knet_cpuif_id,
    p4_pd_knet_hostif_list_t *const hostif_list,
    p4_pd_knet_count_t *const hostif_count) {
  return bf_knet_hostif_list_get(knet_cpuif_id, hostif_list, hostif_count);
}

p4_pd_status_t p4_knet_tx_action_get(
    const p4_pd_knet_cpuif_t knet_cpuif_id,
    const p4_pd_knet_hostif_t knet_hostif_id,
    p4_pd_knet_tx_action_t *const tx_action,
    const p4_pd_knet_count_t tx_mutation_count) {
  return bf_knet_tx_action_get(
      knet_cpuif_id, knet_hostif_id, tx_action, tx_mutation_count);
}
