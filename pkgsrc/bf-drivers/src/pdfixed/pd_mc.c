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


#include <string.h>
#include <tofino/pdfixed/pd_mc.h>
#include <mc_mgr/mc_mgr_intf.h>

p4_pd_status_t p4_pd_mc_init() { return bf_mc_init(); }

p4_pd_status_t p4_pd_mc_create_session(p4_pd_sess_hdl_t *sess_hdl) {
  return bf_mc_create_session(sess_hdl);
}

p4_pd_status_t p4_pd_mc_destroy_session(p4_pd_sess_hdl_t sess_hdl) {
  return bf_mc_destroy_session(sess_hdl);
}

p4_pd_status_t p4_pd_mc_complete_operations(p4_pd_sess_hdl_t sess_hdl) {
  return bf_mc_complete_operations(sess_hdl);
}

p4_pd_status_t p4_pd_mc_begin_batch(p4_pd_sess_hdl_t shdl) {
  return bf_mc_begin_batch(shdl);
}

p4_pd_status_t p4_pd_mc_flush_batch(p4_pd_sess_hdl_t shdl) {
  return bf_mc_flush_batch(shdl);
}

p4_pd_status_t p4_pd_mc_end_batch(p4_pd_sess_hdl_t shdl, bool hwSynchronous) {
  return bf_mc_end_batch(shdl, hwSynchronous);
}

p4_pd_status_t p4_pd_mc_mgrp_create(p4_pd_sess_hdl_t sess_hdl,
                                    bf_dev_id_t dev,
                                    uint16_t grp,
                                    p4_pd_entry_hdl_t *grp_hdl) {
  return bf_mc_mgrp_create(sess_hdl, dev, grp, grp_hdl);
}

p4_pd_status_t p4_pd_mc_mgrp_get_attr(p4_pd_sess_hdl_t shdl,
                                      bf_dev_id_t dev,
                                      p4_pd_entry_hdl_t mgrp_hdl,
                                      uint16_t *grp) {
  return bf_mc_mgrp_get_attr(shdl, dev, mgrp_hdl, grp);
}

p4_pd_status_t p4_pd_mc_mgrp_destroy(p4_pd_sess_hdl_t sess_hdl,
                                     bf_dev_id_t dev,
                                     p4_pd_entry_hdl_t grp_hdl) {
  return bf_mc_mgrp_destroy(sess_hdl, dev, grp_hdl);
}

p4_pd_status_t p4_pd_mc_mgrp_get_first(p4_pd_sess_hdl_t shdl,
                                       bf_dev_id_t dev,
                                       p4_pd_entry_hdl_t *mgrp_hdl) {
  return bf_mc_mgrp_get_first(shdl, dev, mgrp_hdl);
}

p4_pd_status_t p4_pd_mc_mgrp_get_count(p4_pd_sess_hdl_t shdl,
                                       bf_dev_id_t dev,
                                       uint32_t *count) {
  return bf_mc_mgrp_get_count(shdl, dev, count);
}

p4_pd_status_t p4_pd_mc_mgrp_get_next_i(p4_pd_sess_hdl_t shdl,
                                        bf_dev_id_t dev,
                                        p4_pd_entry_hdl_t mgrp_hdl,
                                        uint32_t i,
                                        p4_pd_entry_hdl_t *next_mgrp_hdls) {
  return bf_mc_mgrp_get_next_i(shdl, dev, mgrp_hdl, i, next_mgrp_hdls);
}

p4_pd_status_t p4_pd_mc_mgrp_get_first_node_mbr(p4_pd_sess_hdl_t shdl,
                                                bf_dev_id_t dev,
                                                p4_pd_entry_hdl_t mgrp_hdl,
                                                p4_pd_entry_hdl_t *node_hdl,
                                                bool *xid_valid,
                                                uint16_t *xid) {
  return bf_mc_mgrp_get_first_node_mbr(
      shdl, dev, mgrp_hdl, node_hdl, xid_valid, xid);
}

p4_pd_status_t p4_pd_mc_mgrp_get_node_mbr_count(p4_pd_sess_hdl_t shdl,
                                                bf_dev_id_t dev,
                                                p4_pd_entry_hdl_t mgrp_hdl,
                                                uint32_t *count) {
  return bf_mc_mgrp_get_node_mbr_count(shdl, dev, mgrp_hdl, count);
}

p4_pd_status_t p4_pd_mc_mgrp_get_next_i_node_mbr(
    p4_pd_sess_hdl_t shdl,
    bf_dev_id_t dev,
    p4_pd_entry_hdl_t mgrp_hdl,
    p4_pd_entry_hdl_t node_hdl,
    uint32_t i,
    p4_pd_entry_hdl_t *next_node_hdls,
    bool *next_node_xids_valid,
    uint16_t *next_node_xids) {
  return bf_mc_mgrp_get_next_i_node_mbr(shdl,
                                        dev,
                                        mgrp_hdl,
                                        node_hdl,
                                        i,
                                        next_node_hdls,
                                        next_node_xids_valid,
                                        next_node_xids);
}

p4_pd_status_t p4_pd_mc_mgrp_get_first_ecmp_mbr(p4_pd_sess_hdl_t shdl,
                                                bf_dev_id_t dev,
                                                p4_pd_entry_hdl_t mgrp_hdl,
                                                p4_pd_entry_hdl_t *ecmp_hdl,
                                                bool *xid_valid,
                                                uint16_t *ecmp_xid) {
  return bf_mc_mgrp_get_first_ecmp_mbr(
      shdl, dev, mgrp_hdl, ecmp_hdl, xid_valid, ecmp_xid);
}

p4_pd_status_t p4_pd_mc_mgrp_get_ecmp_mbr_count(p4_pd_sess_hdl_t shdl,
                                                bf_dev_id_t dev,
                                                p4_pd_entry_hdl_t mgrp_hdl,
                                                uint32_t *count) {
  return bf_mc_mgrp_get_ecmp_mbr_count(shdl, dev, mgrp_hdl, count);
}

p4_pd_status_t p4_pd_mc_mgrp_get_next_i_ecmp_mbr(
    p4_pd_sess_hdl_t shdl,
    bf_dev_id_t dev,
    p4_pd_entry_hdl_t mgrp_hdl,
    p4_pd_entry_hdl_t ecmp_hdl,
    uint32_t i,
    p4_pd_entry_hdl_t *next_ecmp_hdls,
    bool *next_ecmp_xids_valid,
    uint16_t *next_ecmp_xids) {
  return bf_mc_mgrp_get_next_i_ecmp_mbr(shdl,
                                        dev,
                                        mgrp_hdl,
                                        ecmp_hdl,
                                        i,
                                        next_ecmp_hdls,
                                        next_ecmp_xids_valid,
                                        next_ecmp_xids);
}

p4_pd_status_t p4_pd_mc_ecmp_create(p4_pd_sess_hdl_t sess_hdl,
                                    bf_dev_id_t dev,
                                    p4_pd_entry_hdl_t *ecmp_hdl) {
  return bf_mc_ecmp_create(sess_hdl, dev, ecmp_hdl);
}

p4_pd_status_t p4_pd_mc_ecmp_destroy(p4_pd_sess_hdl_t sess_hdl,
                                     bf_dev_id_t dev,
                                     p4_pd_entry_hdl_t ecmp_hdl) {
  return bf_mc_ecmp_destroy(sess_hdl, dev, ecmp_hdl);
}

p4_pd_status_t p4_pd_mc_ecmp_get_first(p4_pd_sess_hdl_t shdl,
                                       bf_dev_id_t dev,
                                       p4_pd_entry_hdl_t *ecmp_hdl) {
  return bf_mc_ecmp_get_first(shdl, dev, ecmp_hdl);
}

p4_pd_status_t p4_pd_mc_ecmp_get_count(p4_pd_sess_hdl_t shdl,
                                       bf_dev_id_t dev,
                                       uint32_t *count) {
  return bf_mc_ecmp_get_count(shdl, dev, count);
}

p4_pd_status_t p4_pd_mc_ecmp_get_next_i(p4_pd_sess_hdl_t shdl,
                                        bf_dev_id_t dev,
                                        p4_pd_entry_hdl_t ecmp_hdl,
                                        uint32_t i,
                                        p4_pd_entry_hdl_t *next_ecmp_hdls) {
  return bf_mc_ecmp_get_next_i(shdl, dev, ecmp_hdl, i, next_ecmp_hdls);
}

p4_pd_status_t p4_pd_mc_ecmp_mbr_add(p4_pd_sess_hdl_t sess_hdl,
                                     bf_dev_id_t dev,
                                     p4_pd_entry_hdl_t ecmp_hdl,
                                     p4_pd_entry_hdl_t l1_hdl) {
  return bf_mc_ecmp_mbr_add(sess_hdl, dev, ecmp_hdl, l1_hdl);
}

p4_pd_status_t p4_pd_mc_ecmp_mbr_rem(p4_pd_sess_hdl_t sess_hdl,
                                     bf_dev_id_t dev,
                                     p4_pd_entry_hdl_t ecmp_hdl,
                                     p4_pd_entry_hdl_t l1_hdl) {
  return bf_mc_ecmp_mbr_rem(sess_hdl, dev, ecmp_hdl, l1_hdl);
}

p4_pd_status_t p4_pd_mc_ecmp_get_first_mbr(p4_pd_sess_hdl_t shdl,
                                           bf_dev_id_t dev,
                                           p4_pd_entry_hdl_t ecmp_hdl,
                                           p4_pd_entry_hdl_t *node_hdl) {
  return bf_mc_ecmp_get_first_mbr(shdl, dev, ecmp_hdl, node_hdl);
}

p4_pd_status_t p4_pd_mc_ecmp_get_mbr_count(p4_pd_sess_hdl_t shdl,
                                           bf_dev_id_t dev,
                                           p4_pd_entry_hdl_t ecmp_hdl,
                                           uint32_t *count) {
  return bf_mc_ecmp_get_mbr_count(shdl, dev, ecmp_hdl, count);
}

p4_pd_status_t p4_pd_mc_ecmp_get_next_i_mbr(p4_pd_sess_hdl_t shdl,
                                            bf_dev_id_t dev,
                                            p4_pd_entry_hdl_t ecmp_hdl,
                                            p4_pd_entry_hdl_t node_hdl,
                                            uint32_t i,
                                            p4_pd_entry_hdl_t *next_node_hdls) {
  return bf_mc_ecmp_get_next_i_mbr(
      shdl, dev, ecmp_hdl, node_hdl, i, next_node_hdls);
}

p4_pd_status_t p4_pd_mc_ecmp_get_mbr_from_hash(p4_pd_sess_hdl_t shdl,
                                               bf_dev_id_t dev,
                                               p4_pd_entry_hdl_t mgrp_hdl,
                                               p4_pd_entry_hdl_t ecmp_hdl,
                                               uint16_t level1_mcast_hash,
                                               uint16_t pkt_xid,
                                               p4_pd_entry_hdl_t *node_hdl,
                                               bool *is_pruned) {
  return bf_mc_ecmp_get_mbr_from_hash(shdl,
                                      dev,
                                      mgrp_hdl,
                                      ecmp_hdl,
                                      level1_mcast_hash,
                                      pkt_xid,
                                      node_hdl,
                                      is_pruned);
}

p4_pd_status_t p4_pd_mc_ecmp_get_first_assoc(p4_pd_sess_hdl_t shdl,
                                             bf_dev_id_t dev,
                                             p4_pd_entry_hdl_t ecmp_hdl,
                                             p4_pd_entry_hdl_t *mgrp_hdl) {
  return bf_mc_ecmp_get_first_assoc(shdl, dev, ecmp_hdl, mgrp_hdl);
}

p4_pd_status_t p4_pd_mc_ecmp_get_assoc_count(p4_pd_sess_hdl_t shdl,
                                             bf_dev_id_t dev,
                                             p4_pd_entry_hdl_t ecmp_hdl,
                                             uint32_t *count) {
  return bf_mc_ecmp_get_assoc_count(shdl, dev, ecmp_hdl, count);
}

p4_pd_status_t p4_pd_mc_ecmp_get_next_i_assoc(
    p4_pd_sess_hdl_t shdl,
    bf_dev_id_t dev,
    p4_pd_entry_hdl_t ecmp_hdl,
    p4_pd_entry_hdl_t mgrp_hdl,
    uint32_t i,
    p4_pd_entry_hdl_t *next_mgrp_hdls) {
  return bf_mc_ecmp_get_next_i_assoc(
      shdl, dev, ecmp_hdl, mgrp_hdl, i, next_mgrp_hdls);
}

p4_pd_status_t p4_pd_mc_associate_ecmp(p4_pd_sess_hdl_t sess_hdl,
                                       bf_dev_id_t dev,
                                       p4_pd_entry_hdl_t grp_hdl,
                                       p4_pd_entry_hdl_t ecmp_hdl,
                                       uint16_t xid,
                                       bool xid_valid) {
  return bf_mc_associate_ecmp(sess_hdl, dev, grp_hdl, ecmp_hdl, xid_valid, xid);
}

p4_pd_status_t p4_pd_mc_ecmp_get_assoc_attr(p4_pd_sess_hdl_t shdl,
                                            bf_dev_id_t dev,
                                            p4_pd_entry_hdl_t mgrp_hdl,
                                            p4_pd_entry_hdl_t ecmp_hdl,
                                            bool *xid_valid,
                                            uint16_t *xid) {
  return bf_mc_ecmp_get_assoc_attr(
      shdl, dev, mgrp_hdl, ecmp_hdl, xid_valid, xid);
}

p4_pd_status_t p4_pd_mc_dissociate_ecmp(p4_pd_sess_hdl_t sess_hdl,
                                        bf_dev_id_t dev,
                                        p4_pd_entry_hdl_t grp_hdl,
                                        p4_pd_entry_hdl_t ecmp_hdl) {
  return bf_mc_dissociate_ecmp(sess_hdl, dev, grp_hdl, ecmp_hdl);
}

p4_pd_status_t p4_pd_mc_node_create(p4_pd_sess_hdl_t sess_hdl,
                                    bf_dev_id_t dev,
                                    uint16_t rid,
                                    uint8_t *port_map,
                                    uint8_t *lag_map,
                                    p4_pd_entry_hdl_t *node_hdl) {
  return bf_mc_node_create(sess_hdl, dev, rid, port_map, lag_map, node_hdl);
}

p4_pd_status_t p4_pd_mc_node_get_attr(p4_pd_sess_hdl_t shdl,
                                      bf_dev_id_t dev,
                                      p4_pd_entry_hdl_t node_hdl,
                                      uint16_t *rid,
                                      uint8_t *port_map,
                                      uint8_t *lag_map) {
  return bf_mc_node_get_attr(shdl, dev, node_hdl, rid, port_map, lag_map);
}

p4_pd_status_t p4_pd_mc_node_update(p4_pd_sess_hdl_t sess_hdl,
                                    bf_dev_id_t dev,
                                    p4_pd_entry_hdl_t node_hdl,
                                    uint8_t *port_map,
                                    uint8_t *lag_map) {
  return bf_mc_node_update(sess_hdl, dev, node_hdl, port_map, lag_map);
}

p4_pd_status_t p4_pd_mc_node_destroy(p4_pd_sess_hdl_t sess_hdl,
                                     bf_dev_id_t dev,
                                     p4_pd_entry_hdl_t node_hdl) {
  return bf_mc_node_destroy(sess_hdl, dev, node_hdl);
}

p4_pd_status_t p4_pd_mc_node_get_first(p4_pd_sess_hdl_t shdl,
                                       bf_dev_id_t dev,
                                       p4_pd_entry_hdl_t *node_hdl) {
  return bf_mc_node_get_first(shdl, dev, node_hdl);
}

p4_pd_status_t p4_pd_mc_node_get_count(p4_pd_sess_hdl_t shdl,
                                       bf_dev_id_t dev,
                                       uint32_t *count) {
  return bf_mc_node_get_count(shdl, dev, count);
}

p4_pd_status_t p4_pd_mc_node_get_next_i(p4_pd_sess_hdl_t shdl,
                                        bf_dev_id_t dev,
                                        p4_pd_entry_hdl_t node_hdl,
                                        uint32_t i,
                                        p4_pd_entry_hdl_t *next_node_hdls) {
  return bf_mc_node_get_next_i(shdl, dev, node_hdl, i, next_node_hdls);
}

p4_pd_status_t p4_pd_mc_node_get_association(p4_pd_sess_hdl_t shdl,
                                             bf_dev_id_t dev,
                                             p4_pd_entry_hdl_t node_hdl,
                                             bool *is_associated,
                                             p4_pd_entry_hdl_t *mgrp_hdl,
                                             bool *xid_valid,
                                             uint16_t *xid) {
  return bf_mc_node_get_association(
      shdl, dev, node_hdl, is_associated, mgrp_hdl, xid_valid, xid);
}

p4_pd_status_t p4_pd_mc_node_is_mbr(p4_pd_sess_hdl_t shdl,
                                    bf_dev_id_t dev,
                                    p4_pd_entry_hdl_t node_hdl,
                                    bool *is_ecmp_mbr,
                                    p4_pd_entry_hdl_t *ecmp_hdl) {
  return bf_mc_node_is_mbr(shdl, dev, node_hdl, is_ecmp_mbr, ecmp_hdl);
}

p4_pd_status_t p4_pd_mc_associate_node(p4_pd_sess_hdl_t sess_hdl,
                                       bf_dev_id_t dev,
                                       p4_pd_entry_hdl_t grp_hdl,
                                       p4_pd_entry_hdl_t l1_hdl,
                                       uint16_t xid,
                                       bool xid_valid) {
  return bf_mc_associate_node(sess_hdl, dev, grp_hdl, l1_hdl, xid_valid, xid);
}

p4_pd_status_t p4_pd_mc_dissociate_node(p4_pd_sess_hdl_t sess_hdl,
                                        bf_dev_id_t dev,
                                        p4_pd_entry_hdl_t grp_hdl,
                                        p4_pd_entry_hdl_t node_hdl) {
  return bf_mc_dissociate_node(sess_hdl, dev, grp_hdl, node_hdl);
}

/* Multicast misc APIs. */
p4_pd_status_t p4_pd_mc_set_lag_membership(p4_pd_sess_hdl_t sess_hdl,
                                           bf_dev_id_t dev,
                                           uint8_t lag,
                                           uint8_t *port_map) {
  return bf_mc_set_lag_membership(sess_hdl, dev, lag, port_map);
}

p4_pd_status_t p4_pd_mc_get_lag_membership(p4_pd_sess_hdl_t sess_hdl,
                                           bf_dev_id_t dev,
                                           uint8_t lag,
                                           uint8_t *port_map) {
  return bf_mc_get_lag_membership(sess_hdl, dev, lag, port_map, false);
}

p4_pd_status_t p4_pd_mc_get_lag_member_from_hash(p4_pd_sess_hdl_t sess_hdl,
                                                 bf_dev_id_t dev,
                                                 p4_pd_entry_hdl_t node_hdl,
                                                 uint8_t lag,
                                                 uint16_t level2_mcast_hash,
                                                 uint16_t pkt_xid,
                                                 uint16_t pkt_yid,
                                                 uint16_t pkt_rid,
                                                 int *port,
                                                 bool *is_pruned) {
  return bf_mc_get_lag_member_from_hash(sess_hdl,
                                        dev,
                                        node_hdl,
                                        lag,
                                        level2_mcast_hash,
                                        pkt_xid,
                                        pkt_yid,
                                        pkt_rid,
                                        port,
                                        is_pruned);
}

p4_pd_status_t p4_pd_mc_set_remote_lag_member_count(p4_pd_sess_hdl_t sess_hdl,
                                                    bf_dev_id_t dev,
                                                    uint8_t lag,
                                                    int left,
                                                    int right) {
  return bf_mc_set_remote_lag_member_count(sess_hdl, dev, lag, left, right);
}

p4_pd_status_t p4_pd_mc_update_port_prune_table(p4_pd_sess_hdl_t sess_hdl,
                                                bf_dev_id_t dev,
                                                uint16_t yid,
                                                uint8_t *port_map) {
  return bf_mc_set_port_prune_table(sess_hdl, dev, yid, port_map);
}

p4_pd_status_t p4_pd_mc_get_port_prune_table(p4_pd_sess_hdl_t sess_hdl,
                                             bf_dev_id_t dev,
                                             uint16_t yid,
                                             uint8_t *port_map,
                                             bool from_hw) {
  if (!port_map) return BF_INVALID_ARG;
  bf_mc_port_map_t pm;
  bf_status_t rc = bf_mc_get_port_prune_table(sess_hdl, dev, yid, &pm, from_hw);
  if (rc == BF_SUCCESS) memcpy(port_map, &pm, sizeof pm);
  return rc;
}

p4_pd_status_t p4_pd_mc_set_global_rid(p4_pd_sess_hdl_t sess_hdl,
                                       bf_dev_id_t dev,
                                       uint16_t rid) {
  return bf_mc_set_global_exclusion_rid(sess_hdl, dev, rid);
}

p4_pd_status_t p4_pd_mc_set_port_mc_fwd_state(p4_pd_sess_hdl_t sess_hdl,
                                              bf_dev_id_t dev,
                                              uint16_t port,
                                              bool is_active) {
  return bf_mc_set_port_mc_fwd_state(sess_hdl, dev, port, is_active);
}

p4_pd_status_t p4_pd_mc_enable_port_ff(p4_pd_sess_hdl_t sess_hdl,
                                       bf_dev_id_t dev) {
  return bf_mc_enable_port_fast_failover(sess_hdl, dev);
}

p4_pd_status_t p4_pd_mc_disable_port_ff(p4_pd_sess_hdl_t sess_hdl,
                                        bf_dev_id_t dev) {
  return bf_mc_disable_port_fast_failover(sess_hdl, dev);
}

p4_pd_status_t p4_pd_mc_clr_port_ff_state(p4_pd_sess_hdl_t sess_hdl,
                                          bf_dev_id_t dev,
                                          int port) {
  return bf_mc_clear_fast_failover_state(sess_hdl, dev, port);
}

p4_pd_status_t p4_pd_mc_enable_port_protection(p4_pd_sess_hdl_t sess_hdl,
                                               bf_dev_id_t dev) {
  return bf_mc_enable_port_protection(sess_hdl, dev);
}

p4_pd_status_t p4_pd_mc_disable_port_protection(p4_pd_sess_hdl_t sess_hdl,
                                                bf_dev_id_t dev) {
  return bf_mc_disable_port_protection(sess_hdl, dev);
}

p4_pd_status_t p4_pd_mc_set_port_protection(p4_pd_sess_hdl_t sess_hdl,
                                            bf_dev_id_t dev,
                                            uint16_t pport,
                                            uint16_t bport) {
  return bf_mc_set_port_protection(sess_hdl, dev, pport, bport);
}

p4_pd_status_t p4_pd_mc_clear_port_protection(p4_pd_sess_hdl_t sess_hdl,
                                              bf_dev_id_t dev,
                                              uint16_t pport) {
  return bf_mc_clear_port_protection(sess_hdl, dev, pport);
}

p4_pd_status_t p4_pd_mc_set_max_nodes_before_yield(p4_pd_sess_hdl_t sess_hdl,
                                                   bf_dev_id_t dev,
                                                   int count) {
  return bf_mc_set_max_nodes_before_yield(sess_hdl, dev, count);
}

p4_pd_status_t p4_pd_mc_set_max_node_threshold(p4_pd_sess_hdl_t sess_hdl,
                                               bf_dev_id_t dev,
                                               int node_count,
                                               int port_lag_count) {
  return bf_mc_set_max_node_threshold(
      sess_hdl, dev, node_count, port_lag_count);
}

p4_pd_status_t p4_pd_mc_get_pipe_vector(p4_pd_sess_hdl_t sess_hdl,
                                        bf_dev_id_t dev,
                                        uint16_t grp,
                                        int *logical_pipe_vector) {
  return bf_mc_get_pipe_vector(sess_hdl, dev, grp, logical_pipe_vector);
}
