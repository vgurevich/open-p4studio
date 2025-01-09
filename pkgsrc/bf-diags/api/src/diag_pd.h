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
 * @file diag_pd.h
 * @date
 *
 * Contains definitions of diag PD
 *
 */
#ifndef _DIAG_PD_H
#define _DIAG_PD_H

/* Module header includes */
#include "stdbool.h"
#include <bf_types/bf_types.h>
#include <tofino/pdfixed/pd_conn_mgr.h>
#ifdef CMAKE_BUILD
#ifdef TOFINO3
#include <diag/tofino3/pd/pd.h>
#elif TOFINO2M
#include <diag/tofino2m/pd/pd.h>
#elif TOFINO2H
#include <diag/tofino2h/pd/pd.h>
#elif TOFINO2
#include <diag/tofino2/pd/pd.h>
#else
#include <diag/tofino/pd/pd.h>
#endif
#else  // CMAKE_BUILD
#ifdef TOFINO3
#include <tofino3pd/diag/pd/pd.h>
#elif TOFINO2
#include <tofino2pd/diag/pd/pd.h>
#else
#include <tofinopd/diag/pd/pd.h>
#endif
#endif  // CMAKE_BUILD
#include <mc_mgr/mc_mgr_types.h>
#include <mc_mgr/mc_mgr_intf.h>

bf_status_t diag_pd_read_counter(bf_dev_id_t dev_id);
bf_status_t diag_pd_reset_shift_counter(bf_dev_id_t dev_id);
bf_status_t diag_pd_add_default_entries(bf_dev_id_t dev_id);
bf_status_t diag_pd_power_populate_entries_in_tables(bf_dev_id_t dev_id);
bf_status_t diag_pd_phv_power_populate_entries_in_tables(bf_dev_id_t dev_id);
bf_status_t diag_pd_mau_bus_stress_populate_entries_in_tables(
    bf_dev_id_t dev_id);
bf_status_t diag_pd_add_ing_port_prop(bf_dev_id_t dev_id);
bf_status_t diag_pd_add_yid(bf_dev_id_t dev_id);
bf_status_t diag_pd_add_default_vlan(bf_dev_id_t dev_id,
                                     int vlan_id,
                                     bf_mc_rid_t rid,
                                     bf_dev_port_t dev_port,
                                     p4_pd_entry_hdl_t *entry_hdl);
bf_status_t diag_pd_del_default_vlan(bf_dev_id_t dev_id,
                                     int vlan_id,
                                     bf_dev_port_t dev_port,
                                     p4_pd_entry_hdl_t entry_hdl);
bf_status_t diag_pd_add_port_vlan_mapping(bf_dev_id_t dev_id,
                                          int vlan_id,
                                          bf_mc_rid_t rid,
                                          bf_dev_port_t dev_port,
                                          p4_pd_entry_hdl_t *entry_hdl);
bf_status_t diag_pd_del_port_vlan_mapping(bf_dev_id_t dev_id,
                                          int vlan_id,
                                          bf_dev_port_t dev_port,
                                          p4_pd_entry_hdl_t entry_hdl);
bf_status_t diag_pd_add_dmac(bf_dev_id_t dev_id,
                             int vlan_id,
                             bf_dev_port_t dev_port,
                             uint8_t *dmac,
                             int ttl,
                             p4_pd_entry_hdl_t *entry_hdl);
bf_status_t diag_pd_del_dmac(bf_dev_id_t dev_id, p4_pd_entry_hdl_t entry_hdl);
bf_status_t diag_pd_idle_tmo_en_dmac(bf_dev_id_t dev_id);
bf_status_t diag_pd_add_smac(bf_dev_id_t dev_id,
                             int vlan_id,
                             bf_dev_port_t dev_port,
                             uint8_t *smac,
                             int ttl,
                             p4_pd_entry_hdl_t *entry_hdl);
bf_status_t diag_pd_del_smac(bf_dev_id_t dev_id, p4_pd_entry_hdl_t entry_hdl);
bf_status_t diag_pd_idle_tmo_en_smac(bf_dev_id_t dev_id);
bf_status_t diag_pd_add_bd_flood(bf_dev_id_t dev_id,
                                 int vlan_id,
                                 int mc_index,
                                 p4_pd_entry_hdl_t *entry_hdl);
bf_status_t diag_pd_del_bd_flood(bf_dev_id_t dev_id,
                                 int vlan_id,
                                 int mc_index,
                                 p4_pd_entry_hdl_t entry_hdl);
bf_status_t diag_pd_add_vlan_flood_ports(bf_dev_id_t dev_id,
                                         int vlan_id,
                                         int mc_index,
                                         bf_mc_rid_t rid,
                                         uint8_t *port_map,
                                         uint8_t *lag_map,
                                         p4_pd_entry_hdl_t *mc_grp_hdl,
                                         p4_pd_entry_hdl_t *mc_node_hdl);
bf_status_t diag_pd_upd_vlan_flood_ports(bf_dev_id_t dev_id,
                                         int vlan_id,
                                         int mc_index,
                                         uint8_t *port_map,
                                         uint8_t *lag_map,
                                         p4_pd_entry_hdl_t mc_node_hdl);
bf_status_t diag_pd_del_vlan_flood_ports(bf_dev_id_t dev_id,
                                         int vlan_id,
                                         int mc_index,
                                         p4_pd_entry_hdl_t mc_grp_hdl,
                                         p4_pd_entry_hdl_t mc_node_hdl);
bf_status_t diag_pd_add_learn_notify(bf_dev_id_t dev_id,
                                     p4_pd_entry_hdl_t *entry_hdl);
bf_status_t diag_pd_del_learn_notify(bf_dev_id_t dev_id,
                                     p4_pd_entry_hdl_t entry_hdl);

#if !defined(DIAG_SINGLE_STAGE) && !defined(DIAG_MAU_BUS_STRESS_ENABLE) && \
    !defined(DIAG_PHV_FLOP_TEST)
p4_pd_status_t diag_mac_learn_digest_notify_cb(
    p4_pd_sess_hdl_t sess_hdl,
    p4_pd_diag_SwitchIngressDeparser_digest_digest_msg_t *msg,
    void *callback_fn_cookie);
#endif

void diag_dmac_idle_tmo_expiry_cb(bf_dev_id_t dev_id,
                                  p4_pd_entry_hdl_t entry_hdl,
                                  p4_pd_idle_time_hit_state_e hs,
                                  void *cookie);
void diag_smac_idle_tmo_expiry_cb(bf_dev_id_t dev_id,
                                  p4_pd_entry_hdl_t entry_hdl,
                                  p4_pd_idle_time_hit_state_e hs,
                                  void *cookie);
bf_status_t diag_pd_add_dst_override(bf_dev_id_t dev_id,
                                     bf_dev_port_t ig_port,
                                     bf_dev_port_t dst_port,
                                     uint32_t tcp_dstPort_start,
                                     uint32_t tcp_dstPort_end,
                                     int priority,
                                     p4_pd_entry_hdl_t *entry_hdl);
bf_status_t diag_pd_mc_add_dst_override(bf_dev_id_t dev_id,
                                        bf_dev_port_t ig_port,
                                        bf_mc_grp_id_t mc_grp_id_a,
                                        bf_mc_grp_id_t mc_grp_id_b,
                                        uint32_t tcp_dstPort_start,
                                        uint32_t tcp_dstPort_end,
                                        int priority,
                                        p4_pd_entry_hdl_t *entry_hdl);
bf_status_t diag_pd_del_dst_override(bf_dev_id_t dev_id,
                                     p4_pd_entry_hdl_t entry_hdl);
bf_status_t diag_pd_del_dst_override_by_match_spec(bf_dev_id_t dev_id,
                                                   bf_dev_port_t ig_port,
                                                   uint32_t tcp_dstPort_start,
                                                   uint32_t tcp_dstPort_end,
                                                   int priority);
bf_status_t diag_pd_add_vlan_encap(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   int vlan_id,
                                   p4_pd_entry_hdl_t *entry_hdl);
bf_status_t diag_pd_del_vlan_encap(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   p4_pd_entry_hdl_t entry_hdl);
bf_status_t diag_pd_del_vlan_encap_by_match_spec(bf_dev_id_t dev_id,
                                                 bf_dev_port_t dev_port,
                                                 int vlan_id);
bf_status_t diag_pd_add_vlan_decap(bf_dev_id_t dev_id,
                                   p4_pd_entry_hdl_t *entry_hdl);
bf_status_t diag_pd_del_vlan_decap(bf_dev_id_t dev_id,
                                   p4_pd_entry_hdl_t entry_hdl);
bf_status_t diag_pd_learning_timeout_set(bf_dev_id_t dev_id,
                                         uint32_t timeout_usec);

bf_status_t diag_pd_mc_mgrp_create(bf_dev_id_t dev_id,
                                   bf_mc_grp_id_t grp_id,
                                   bf_mc_mgrp_hdl_t *mgrp_hdl);
bf_status_t diag_pd_mc_mgrp_destroy(bf_dev_id_t dev_id,
                                    bf_mc_mgrp_hdl_t mgrp_hdl);
bf_status_t diag_pd_mc_mgrp_ports_add(bf_dev_id_t dev_id,
                                      bf_mc_port_map_t port_map,
                                      bf_mc_lag_map_t lag_map,
                                      bf_mc_rid_t rid,
                                      bf_mc_mgrp_hdl_t mgrp_hdl,
                                      bf_mc_node_hdl_t *node_hdl);
bf_status_t diag_pd_mc_mgrp_ports_del(bf_dev_id_t dev_id,
                                      bf_mc_mgrp_hdl_t mgrp_hdl,
                                      bf_mc_node_hdl_t node_hdl);
bf_status_t diag_pd_mc_ecmp_create(bf_dev_id_t dev_id,
                                   bf_mc_ecmp_hdl_t *ecmp_hdl);
bf_status_t diag_pd_mc_ecmp_destory(bf_dev_id_t dev_id,
                                    bf_mc_ecmp_hdl_t ecmp_hdl);
bf_status_t diag_pd_mc_associate_ecmp(bf_dev_id_t dev_id,
                                      bf_mc_mgrp_hdl_t mgrp_hdl,
                                      bf_mc_ecmp_hdl_t ecmp_hdl);
bf_status_t diag_pd_mc_dissociate_ecmp(bf_dev_id_t dev_id,
                                       bf_mc_mgrp_hdl_t mgrp_hdl,
                                       bf_mc_ecmp_hdl_t ecmp_hdl);
bf_status_t diag_pd_mc_set_lag_membership(bf_dev_id_t dev_id,
                                          uint32_t lag_id,
                                          bf_mc_port_map_t port_map);
bf_status_t diag_pd_mc_ecmp_ports_add(bf_dev_id_t dev_id,
                                      bf_mc_port_map_t port_map,
                                      bf_mc_lag_map_t lag_map,
                                      bf_mc_rid_t rid,
                                      bf_mc_ecmp_hdl_t ecmp_hdl,
                                      bf_mc_node_hdl_t *node_hdl);
bf_status_t diag_pd_mc_ecmp_ports_del(bf_dev_id_t dev_id,
                                      bf_mc_ecmp_hdl_t ecmp_hdl,
                                      bf_mc_node_hdl_t node_hdl);
bf_status_t diag_pd_stream_setup(bf_dev_id_t dev_id,
                                 bf_dev_pipe_t pipe,
                                 uint32_t app_id,
                                 uint32_t pkt_size,
                                 uint32_t pkt_buf_offset,
                                 bf_dev_port_t pktgen_port,
                                 bf_dev_port_t src_port,
                                 uint32_t num_pkts,
                                 uint32_t timer_nsec);
bf_status_t diag_pd_stream_start(bf_dev_id_t dev_id,
                                 uint32_t app_id,
                                 bf_dev_pipe_t pipe,
                                 uint32_t pkt_size,
                                 uint8_t *pkt_buf,
                                 uint32_t pkt_buf_offset);
bf_status_t diag_pd_stream_adjust(bf_dev_id_t dev_id,
                                  bf_dev_pipe_t pipe,
                                  uint32_t app_id,
                                  uint32_t pkt_size,
                                  uint32_t pkt_buf_offset,
                                  bf_dev_port_t pktgen_port,
                                  bf_dev_port_t src_port,
                                  uint32_t num_pkts,
                                  uint32_t timer_nsec,
                                  uint8_t *pkt_buf,
                                  bool enabled);
bf_status_t diag_pd_stream_stop(bf_dev_id_t dev_id,
                                bf_dev_pipe_t pipe,
                                uint32_t app_id);
bf_status_t diag_pd_stream_counter_get(bf_dev_id_t dev_id,
                                       bf_dev_pipe_t pipe,
                                       uint32_t app_id,
                                       uint64_t *cntr_val);
bf_status_t diag_pd_stream_cleanup(bf_dev_id_t dev_id,
                                   bf_dev_port_t pktgen_port,
                                   bool disable_port);
bf_status_t diag_pd_gfm_pattern(bf_dev_id_t dev_id,
                                int num_patterns,
                                uint64_t *row_patterns,
                                uint64_t *bad_parity_rows);
bf_status_t diag_pd_gfm_col(bf_dev_id_t dev_id,
                            int column,
                            uint16_t col_data[64]);

#endif
