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
 * @file diag_util.h
 * @date
 *
 * Contains definitions of diag utilities
 *
 */
#ifndef _DIAG_UTIL_H
#define _DIAG_UTIL_H

/* Module header includes */
#include "diag_common.h"
#include <dvm/bf_dma_types.h>
#include <dvm/bf_drv_intf.h>
#include <port_mgr/bf_port_if.h>

#define DIAG_SESS_HANDLE_TO_DEV(_hdl) ((_hdl >> 8) & 0xff)
#define DIAG_SESS_HANDLE_TO_HDL(_hdl) (_hdl & 0xff)
#define DIAG_SESS_HANDLE_CREATE(_dev, _hdl) \
  (((_hdl)&0xff) | ((uint16_t)(_dev) << 8))
#define DIAG_SESS_HANDLE_MAX 0xffff
#define DIAG_SESS_VALID(_hdl)                                               \
  ((_hdl < DIAG_SESS_HANDLE_MAX) && (DIAG_SESS_HANDLE_TO_HDL(_hdl) >= 1) && \
   (DIAG_SESS_HANDLE_TO_HDL(_hdl) < DIAG_SESSIONS_MAX_LIMIT))

diag_port_t *diag_get_port_info(bf_dev_id_t dev_id, int port);
bf_status_t diag_get_num_active_pipes_and_cpu_port(bf_dev_id_t dev_id,
                                                   int *num_pipes,
                                                   int *cpu_port,
                                                   int *cpu_port2,
                                                   int *eth_cpu_port);
bf_status_t diag_get_part_revision(bf_dev_id_t dev_id,
                                   bf_sku_chip_part_rev_t *rev);
bf_status_t diag_vlan_show(bf_dev_id_t dev_id,
                           int input_vlan_id,
                           char *resp_str,
                           int max_str_len);
bool diag_is_pkt_size_expected(bf_diag_sess_hdl_t sess_hdl, uint32_t pkt_size);
bool diag_any_pkt_size_sent_valid(const diag_session_info_t *sess_info);
bool diag_is_pkt_expected_on_port(bf_diag_sess_hdl_t sess_hdl,
                                  bf_dev_port_t port,
                                  uint32_t tcp_dst_port);
bf_status_t diag_save_loopback_test_params(bf_dev_id_t dev_id,
                                           bf_diag_sess_hdl_t sess_hdl,
                                           bf_dev_port_t *port_arr,
                                           int num_ports,
                                           bf_diag_port_lpbk_mode_e loop_mode,
                                           diag_test_type_e test_type);
bf_status_t diag_save_runtime_loopback_test_params(bf_dev_id_t dev_id,
                                                   bf_diag_sess_hdl_t sess_hdl,
                                                   uint32_t pkt_size,
                                                   uint32_t num_packet,
                                                   uint32_t tcp_dstPort_start,
                                                   uint32_t tcp_dstPort_end,
                                                   bool bidir);
bf_loopback_mode_e diag_get_bf_loop_mode(
    bf_diag_port_lpbk_mode_e diag_loop_mode);
bf_diag_port_lpbk_mode_e diag_get_loop_mode(bf_loopback_mode_e loop_mode);
bf_status_t diag_vlan_show(bf_dev_id_t dev_id,
                           int input_vlan_id,
                           char *resp_str,
                           int max_str_len);
bf_status_t diag_update_vlan_flood_list(bf_dev_id_t dev_id, int vlan_id);
bf_status_t diag_test_cleanup(bf_diag_sess_hdl_t sess_hdl);
bool diag_test_type_loopback(const diag_session_info_t *sess_info);
bool diag_test_type_snake(const diag_session_info_t *sess_info);
bool diag_test_type_loopback_pair(const diag_session_info_t *sess_info);
bool diag_test_type_multicast_loopback(const diag_session_info_t *sess_info);
bool diag_test_type_stream(const diag_session_info_t *sess_info);

/* ---- Stats API ----- */
bf_status_t diag_cpu_port_stats_get(bf_dev_id_t dev_id,
                                    bf_dev_port_t port,
                                    bf_diag_sess_hdl_t sess_hdl,
                                    bf_diag_port_stats_t *stats);
bf_status_t diag_cpu_port_stats_all_sessions_get(bf_dev_id_t dev_id,
                                                 bf_dev_port_t port,
                                                 bf_diag_port_stats_t *stats);
bf_status_t diag_cpu_port_stats_clear(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      bf_diag_sess_hdl_t sess_hdl,
                                      bool all_ports);
bf_status_t diag_devport_is_valid(bf_dev_id_t dev_id, bf_dev_port_t dev_port);

/* ---- Session APIs ----- */
diag_session_info_t *diag_session_info_get(bf_diag_sess_hdl_t sess_hdl);
bf_status_t diag_session_info_add(bf_diag_sess_hdl_t sess_hdl,
                                  diag_session_info_t *sess_info);
bf_status_t diag_session_info_del(bf_diag_sess_hdl_t sess_hdl);
bf_status_t diag_session_info_del_all(bf_dev_id_t dev_id, bool dev_del);
bf_status_t diag_session_hdl_alloc(bf_dev_id_t dev_id,
                                   bf_diag_sess_hdl_t *sess_hdl);
bf_status_t diag_session_hdl_free(bf_diag_sess_hdl_t sess_hdl);
bf_status_t diag_session_config_overlap_check(
    bf_dev_id_t dev_id,
    bf_dev_port_t *port_list,
    int num_ports,
    bf_diag_port_lpbk_mode_e diag_loop_mode,
    bf_diag_sess_hdl_t *overlap_hdl);
bool diag_port_in_multiple_sessions(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
bf_status_t diag_find_default_session(diag_test_type_e test_type,
                                      bf_diag_sess_hdl_t *ret_sess_hdl);
bf_status_t diag_sessions_max_set_helper(bf_dev_id_t dev_id,
                                         uint32_t max_sessions,
                                         ucli_context_t *uc);
bf_status_t bf_diag_session_del_all(bf_dev_id_t dev_id);
bool is_diag_cpu_port_any_channel(bf_dev_id_t dev_id, bf_dev_port_t port);
bf_status_t diag_data_pattern_set_helper(bf_diag_sess_hdl_t sess_hdl,
                                         bf_diag_data_pattern_t mode,
                                         uint8_t start_pat,
                                         uint32_t start_pat_len,
                                         uint8_t pat_a,
                                         uint8_t pat_b,
                                         uint32_t pattern_len,
                                         ucli_context_t *uc);
bf_status_t diag_packet_payload_set_helper(ucli_context_t *uc,
                                           bf_diag_sess_hdl_t sess_hdl,
                                           bf_diag_packet_payload_t mode,
                                           const char *payload_str,
                                           const char *payload_file_path);
bf_status_t diag_packet_full_set_helper(ucli_context_t *uc,
                                        bf_diag_sess_hdl_t sess_hdl,
                                        bf_diag_packet_full_t mode,
                                        const char *pkt_str,
                                        const char *pkt_file_path);
void diag_get_all_ports_list(bf_dev_id_t dev_id,
                             bf_dev_port_t *port_arr,
                             int *num_ports,
                             bool allow_internal);
void diag_get_all_mesh_ports_list(bf_dev_id_t dev_id,
                                  bf_dev_port_t *port_arr,
                                  int *num_ports,
                                  bool allow_internal);
void diag_add_cpu_port_to_list(bf_dev_id_t dev_id,
                               bf_dev_port_t *port_arr,
                               int *num_ports,
                               bool twice);
bool diag_is_chip_family_tofino1(bf_dev_id_t dev_id);
bool diag_is_chip_family_tofino2(bf_dev_id_t dev_id);
bool diag_is_chip_family_tofino3(bf_dev_id_t dev_id);
int diag_cpu_port_get(bf_dev_id_t dev_id, bf_dev_port_t port);
bool diag_any_device_exists();

bf_dev_port_t diag_get_pktgen_port(bf_dev_id_t dev_id,
                                   bf_dev_pipe_t pipe,
                                   uint32_t app_id);
uint32_t diag_pgen_max_apps_on_chip(bf_dev_id_t dev_id);
bool diag_pgen_is_any_app_used(bf_dev_id_t dev_id, bf_dev_pipe_t pipe);
void diag_pgen_adjust_global_pkt_buf_offset(bf_dev_id_t dev_id,
                                            diag_session_info_t *sess_info);
bf_status_t diag_pgen_find_free_app(bf_dev_id_t dev_id,
                                    bf_dev_pipe_t pipe,
                                    uint32_t *app_id);
void diag_pgen_app_reserve(bf_dev_id_t dev_id,
                           bf_dev_pipe_t pipe,
                           uint32_t app_id,
                           bool free);
uint32_t diag_pgen_get_aligned_pkt_size(uint32_t pkt_size);
uint32_t diag_get_pktgen_def_trigger_time_nsecs();
bf_status_t diag_pgen_port_add(bf_dev_id_t dev_id);
bf_status_t diag_validate_loopback_mode(bf_dev_id_t dev_id,
                                        bf_diag_port_lpbk_mode_e loop_mode);

#endif
