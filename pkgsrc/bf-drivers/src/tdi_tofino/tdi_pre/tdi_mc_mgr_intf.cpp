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

#include "tdi_mc_mgr_intf.hpp"

namespace tdi {
namespace tna {
namespace tofino {

std::unique_ptr<IMcMgrIntf> IMcMgrIntf::instance = nullptr;
std::mutex IMcMgrIntf::mc_mgr_intf_mtx;

tdi_status_t McMgrIntf::mcMgrInit() { return bf_mc_init(); }

tdi_status_t McMgrIntf::mcMgrCreateSession(bf_mc_session_hdl_t *shdl) {
  return bf_mc_create_session(shdl);
}
tdi_status_t McMgrIntf::mcMgrDestroySession(bf_mc_session_hdl_t shdl) {
  return bf_mc_destroy_session(shdl);
}

tdi_status_t McMgrIntf::mcMgrBeginBatch(bf_mc_session_hdl_t shdl) {
  return bf_mc_begin_batch(shdl);
}

tdi_status_t McMgrIntf::mcMgrFlushBatch(bf_mc_session_hdl_t shdl) {
  return bf_mc_flush_batch(shdl);
}

tdi_status_t McMgrIntf::mcMgrEndBatch(bf_mc_session_hdl_t shdl,
                                      bool hwSynchronous) {
  return bf_mc_end_batch(shdl, hwSynchronous);
}

tdi_status_t McMgrIntf::mcMgrCompleteOperations(bf_mc_session_hdl_t shdl) {
  return bf_mc_complete_operations(shdl);
}

tdi_status_t McMgrIntf::mcMgrMgrpCreate(bf_mc_session_hdl_t shdl,
                                        tdi_dev_id_t dev,
                                        bf_mc_grp_id_t grp,
                                        bf_mc_mgrp_hdl_t *hdl) {
  return bf_mc_mgrp_create(shdl, dev, grp, hdl);
}

tdi_status_t McMgrIntf::mcMgrMgrpGetAttr(bf_mc_session_hdl_t shdl,
                                         tdi_dev_id_t dev,
                                         bf_mc_mgrp_hdl_t mgrp_hdl,
                                         bf_mc_grp_id_t *grp) {
  return bf_mc_mgrp_get_attr(shdl, dev, mgrp_hdl, grp);
}

tdi_status_t McMgrIntf::mcMgrMgrpDestroy(bf_mc_session_hdl_t shdl,
                                         tdi_dev_id_t dev,
                                         bf_mc_mgrp_hdl_t mgrp_hdl) {
  return bf_mc_mgrp_destroy(shdl, dev, mgrp_hdl);
}

tdi_status_t McMgrIntf::mcMgrMgrpGetFirstNodeMbr(bf_mc_session_hdl_t shdl,
                                                 tdi_dev_id_t dev,
                                                 bf_mc_mgrp_hdl_t mgrp_hdl,
                                                 bf_mc_node_hdl_t *node_hdl,
                                                 bool *node_l1_xid_valid,
                                                 bf_mc_l1_xid_t *node_l1_xid) {
  return bf_mc_mgrp_get_first_node_mbr(
      shdl, dev, mgrp_hdl, node_hdl, node_l1_xid_valid, node_l1_xid);
}

tdi_status_t McMgrIntf::mcMgrMgrpGetNodeMbrCount(bf_mc_session_hdl_t shdl,
                                                 tdi_dev_id_t dev,
                                                 bf_mc_mgrp_hdl_t mgrp_hdl,
                                                 uint32_t *count) {
  return bf_mc_mgrp_get_node_mbr_count(shdl, dev, mgrp_hdl, count);
}

tdi_status_t McMgrIntf::mcMgrMgrpGetNextINodeMbr(
    bf_mc_session_hdl_t shdl,
    tdi_dev_id_t dev,
    bf_mc_mgrp_hdl_t mgrp_hdl,
    bf_mc_node_hdl_t node_hdl,
    uint32_t i,
    bf_mc_node_hdl_t *next_node_hdls,
    bool *next_node_l1_xids_valid,
    bf_mc_l1_xid_t *next_node_l1_xids) {
  return bf_mc_mgrp_get_next_i_node_mbr(shdl,
                                        dev,
                                        mgrp_hdl,
                                        node_hdl,
                                        i,
                                        next_node_hdls,
                                        next_node_l1_xids_valid,
                                        next_node_l1_xids);
}

tdi_status_t McMgrIntf::mcMgrMgrpGetFirstEcmpMbr(bf_mc_session_hdl_t shdl,
                                                 tdi_dev_id_t dev,
                                                 bf_mc_mgrp_hdl_t mgrp_hdl,
                                                 bf_mc_ecmp_hdl_t *ecmp_hdl,
                                                 bool *ecmp_l1_xid_valid,
                                                 bf_mc_l1_xid_t *ecmp_l1_xid) {
  return bf_mc_mgrp_get_first_ecmp_mbr(
      shdl, dev, mgrp_hdl, ecmp_hdl, ecmp_l1_xid_valid, ecmp_l1_xid);
}

tdi_status_t McMgrIntf::mcMgrMgrpGetEcmpMbrCount(bf_mc_session_hdl_t shdl,
                                                 tdi_dev_id_t dev,
                                                 bf_mc_mgrp_hdl_t mgrp_hdl,
                                                 uint32_t *count) {
  return bf_mc_mgrp_get_ecmp_mbr_count(shdl, dev, mgrp_hdl, count);
}

tdi_status_t McMgrIntf::mcMgrMgrpGetNextIEcmpMbr(
    bf_mc_session_hdl_t shdl,
    tdi_dev_id_t dev,
    bf_mc_mgrp_hdl_t mgrp_hdl,
    bf_mc_ecmp_hdl_t ecmp_hdl,
    uint32_t i,
    bf_mc_ecmp_hdl_t *next_ecmp_hdls,
    bool *next_ecmp_l1_xids_valid,
    bf_mc_l1_xid_t *next_ecmp_l1_xids) {
  return bf_mc_mgrp_get_next_i_ecmp_mbr(shdl,
                                        dev,
                                        mgrp_hdl,
                                        ecmp_hdl,
                                        i,
                                        next_ecmp_hdls,
                                        next_ecmp_l1_xids_valid,
                                        next_ecmp_l1_xids);
}

tdi_status_t McMgrIntf::mcMgrMgrpGetCount(bf_mc_session_hdl_t shdl,
                                          tdi_dev_id_t dev,
                                          uint32_t *count) {
  return bf_mc_mgrp_get_count(shdl, dev, count);
}

tdi_status_t McMgrIntf::mcMgrNodeCreate(bf_mc_session_hdl_t shdl,
                                        tdi_dev_id_t dev,
                                        bf_mc_rid_t rid,
                                        bf_mc_port_map_t port_map,
                                        bf_mc_lag_map_t lag_map,
                                        bf_mc_node_hdl_t *node_hdl) {
  return bf_mc_node_create(shdl, dev, rid, port_map, lag_map, node_hdl);
}

tdi_status_t McMgrIntf::mcMgrNodeGetAttr(bf_mc_session_hdl_t shdl,
                                         tdi_dev_id_t dev,
                                         bf_mc_node_hdl_t node_hdl,
                                         bf_mc_rid_t *rid,
                                         bf_mc_port_map_t port_map,
                                         bf_mc_lag_map_t lag_map) {
  return bf_mc_node_get_attr(shdl, dev, node_hdl, rid, port_map, lag_map);
}

tdi_status_t McMgrIntf::mcMgrNodeDestroy(bf_mc_session_hdl_t shdl,
                                         tdi_dev_id_t dev,
                                         bf_mc_node_hdl_t node_hdl) {
  return bf_mc_node_destroy(shdl, dev, node_hdl);
}

tdi_status_t McMgrIntf::mcMgrNodeGetFirst(bf_mc_session_hdl_t shdl,
                                          tdi_dev_id_t dev,
                                          bf_mc_node_hdl_t *node_hdl) {
  return bf_mc_node_get_first(shdl, dev, node_hdl);
}

tdi_status_t McMgrIntf::mcMgrNodeGetCount(bf_mc_session_hdl_t shdl,
                                          tdi_dev_id_t dev,
                                          uint32_t *count) {
  return bf_mc_node_get_count(shdl, dev, count);
}

tdi_status_t McMgrIntf::mcMgrNodeGetNextI(bf_mc_session_hdl_t shdl,
                                          tdi_dev_id_t dev,
                                          bf_mc_node_hdl_t node_hdl,
                                          uint32_t i,
                                          bf_mc_node_hdl_t *next_node_hdls) {
  return bf_mc_node_get_next_i(shdl, dev, node_hdl, i, next_node_hdls);
}

tdi_status_t McMgrIntf::mcMgrNodeGetAssociation(
    bf_mc_session_hdl_t shdl,
    tdi_dev_id_t dev,
    bf_mc_node_hdl_t node_hdl,
    bool *is_associated,
    bf_mc_mgrp_hdl_t *mgrp_hdl,
    bool *level1_exclusion_id_valid,
    bf_mc_l1_xid_t *level1_exclusion_id) {
  return bf_mc_node_get_association(shdl,
                                    dev,
                                    node_hdl,
                                    is_associated,
                                    mgrp_hdl,
                                    level1_exclusion_id_valid,
                                    level1_exclusion_id);
}

tdi_status_t McMgrIntf::mcMgrNodeUpdate(bf_mc_session_hdl_t shdl,
                                        tdi_dev_id_t dev,
                                        bf_mc_node_hdl_t node_hdl,
                                        bf_mc_port_map_t port_map,
                                        bf_mc_lag_map_t lag_map) {
  return bf_mc_node_update(shdl, dev, node_hdl, port_map, lag_map);
}

tdi_status_t McMgrIntf::mcMgrEcmpCreate(bf_mc_session_hdl_t shld,
                                        tdi_dev_id_t dev,
                                        bf_mc_ecmp_hdl_t *ecmp_hdl) {
  return bf_mc_ecmp_create(shld, dev, ecmp_hdl);
}

tdi_status_t McMgrIntf::mcMgrEcmpDestroy(bf_mc_session_hdl_t shdl,
                                         tdi_dev_id_t dev,
                                         bf_mc_ecmp_hdl_t ecmp_hdl) {
  return bf_mc_ecmp_destroy_checked(shdl, dev, ecmp_hdl);
}

tdi_status_t McMgrIntf::mcMgrEcmpGetFirst(bf_mc_session_hdl_t shdl,
                                          tdi_dev_id_t dev,
                                          bf_mc_ecmp_hdl_t *ecmp_hdl) {
  return bf_mc_ecmp_get_first(shdl, dev, ecmp_hdl);
}

tdi_status_t McMgrIntf::mcMgrEcmpGetCount(bf_mc_session_hdl_t shdl,
                                          tdi_dev_id_t dev,
                                          uint32_t *count) {
  return bf_mc_ecmp_get_count(shdl, dev, count);
}

tdi_status_t McMgrIntf::mcMgrEcmpGetNextI(bf_mc_session_hdl_t shdl,
                                          tdi_dev_id_t dev,
                                          bf_mc_ecmp_hdl_t ecmp_hdl,
                                          uint32_t i,
                                          bf_mc_ecmp_hdl_t *next_ecmp_hdls) {
  return bf_mc_ecmp_get_next_i(shdl, dev, ecmp_hdl, i, next_ecmp_hdls);
}

tdi_status_t McMgrIntf::mcMgrEcmpMbrAdd(bf_mc_session_hdl_t shdl,
                                        tdi_dev_id_t dev,
                                        bf_mc_ecmp_hdl_t ecmp_hdl,
                                        bf_mc_node_hdl_t node_hdl) {
  return bf_mc_ecmp_mbr_add(shdl, dev, ecmp_hdl, node_hdl);
}

tdi_status_t McMgrIntf::mcMgrEcmpMbrMod(bf_mc_session_hdl_t shdl,
                                        tdi_dev_id_t dev,
                                        bf_mc_ecmp_hdl_t ecmp_hdl,
                                        bf_mc_node_hdl_t *node_hdls,
                                        uint32_t size) {
  return bf_mc_ecmp_mbr_mod(shdl, dev, ecmp_hdl, node_hdls, size);
}

tdi_status_t McMgrIntf::mcMgrEcmpMbrRem(bf_mc_session_hdl_t shdl,
                                        tdi_dev_id_t dev,
                                        bf_mc_ecmp_hdl_t ecmp_hdl,
                                        bf_mc_node_hdl_t node_hdl) {
  return bf_mc_ecmp_mbr_rem(shdl, dev, ecmp_hdl, node_hdl);
}

tdi_status_t McMgrIntf::mcMgrEcmpGetFirstMbr(bf_mc_session_hdl_t shdl,
                                             tdi_dev_id_t dev,
                                             bf_mc_ecmp_hdl_t ecmp_hdl,
                                             bf_mc_node_hdl_t *node_hdl) {
  return bf_mc_ecmp_get_first_mbr(shdl, dev, ecmp_hdl, node_hdl);
}

tdi_status_t McMgrIntf::mcMgrEcmpGetMbrCount(bf_mc_session_hdl_t shdl,
                                             tdi_dev_id_t dev,
                                             bf_mc_ecmp_hdl_t ecmp_hdl,
                                             uint32_t *count) {
  return bf_mc_ecmp_get_mbr_count(shdl, dev, ecmp_hdl, count);
}

tdi_status_t McMgrIntf::mcMgrEcmpGetNextIMbr(bf_mc_session_hdl_t shdl,
                                             tdi_dev_id_t dev,
                                             bf_mc_ecmp_hdl_t ecmp_hdl,
                                             bf_mc_node_hdl_t node_hdl,
                                             uint32_t i,
                                             bf_mc_node_hdl_t *next_node_hdls) {
  return bf_mc_ecmp_get_next_i_mbr(
      shdl, dev, ecmp_hdl, node_hdl, i, next_node_hdls);
}

tdi_status_t McMgrIntf::mcMgrAssociateNode(bf_mc_session_hdl_t shdl,
                                           tdi_dev_id_t dev,
                                           bf_mc_mgrp_hdl_t mgrp_hdl,
                                           bf_mc_node_hdl_t node_hdl,
                                           bool level1_exclusion_id_valid,
                                           bf_mc_l1_xid_t level1_exclusion_id) {
  return bf_mc_associate_node(shdl,
                              dev,
                              mgrp_hdl,
                              node_hdl,
                              level1_exclusion_id_valid,
                              level1_exclusion_id);
}

tdi_status_t McMgrIntf::mcMgrDissociateNode(bf_mc_session_hdl_t shdl,
                                            tdi_dev_id_t dev,
                                            bf_mc_mgrp_hdl_t mgrp_hdl,
                                            bf_mc_node_hdl_t node_hdl) {
  return bf_mc_dissociate_node(shdl, dev, mgrp_hdl, node_hdl);
}

tdi_status_t McMgrIntf::mcMgrAssociateEcmp(bf_mc_session_hdl_t shdl,
                                           tdi_dev_id_t dev,
                                           bf_mc_mgrp_hdl_t mgrp_hdl,
                                           bf_mc_ecmp_hdl_t ecmp_hdl,
                                           bool level1_exclusion_id_valid,
                                           bf_mc_l1_xid_t level1_exclusion_id) {
  return bf_mc_associate_ecmp(shdl,
                              dev,
                              mgrp_hdl,
                              ecmp_hdl,
                              level1_exclusion_id_valid,
                              level1_exclusion_id);
}

tdi_status_t McMgrIntf::mcMgrEcmpGetAssocAttr(
    bf_mc_session_hdl_t shdl,
    tdi_dev_id_t dev,
    bf_mc_mgrp_hdl_t mgrp_hdl,
    bf_mc_ecmp_hdl_t ecmp_hdl,
    bool *level1_exclusion_id_valid,
    bf_mc_l1_xid_t *level1_exclusion_id) {
  return bf_mc_ecmp_get_assoc_attr(shdl,
                                   dev,
                                   mgrp_hdl,
                                   ecmp_hdl,
                                   level1_exclusion_id_valid,
                                   level1_exclusion_id);
}

tdi_status_t McMgrIntf::mcMgrDissociateEcmp(bf_mc_session_hdl_t shdl,
                                            tdi_dev_id_t dev,
                                            bf_mc_mgrp_hdl_t mgrp_hdl,
                                            bf_mc_ecmp_hdl_t ecmp_hdl) {
  return bf_mc_dissociate_ecmp(shdl, dev, mgrp_hdl, ecmp_hdl);
}

tdi_status_t McMgrIntf::mcMgrSetPortPruneTable(bf_mc_session_hdl_t shdl,
                                               tdi_dev_id_t dev,
                                               bf_mc_l2_xid_t l2_exclusion_id,
                                               bf_mc_port_map_t pruned_ports) {
  return bf_mc_set_port_prune_table(shdl, dev, l2_exclusion_id, pruned_ports);
}

tdi_status_t McMgrIntf::mcMgrGetPortPruneTable(bf_mc_session_hdl_t shdl,
                                               tdi_dev_id_t dev,
                                               bf_mc_l2_xid_t l2_exclusion_id,
                                               bf_mc_port_map_t *pruned_ports,
                                               bool from_hw) {
  return bf_mc_get_port_prune_table(
      shdl, dev, l2_exclusion_id, pruned_ports, from_hw);
}

tdi_status_t McMgrIntf::mcMgrSetLagMembership(bf_mc_session_hdl_t shdl,
                                              tdi_dev_id_t dev,
                                              bf_mc_lag_id_t lag_id,
                                              bf_mc_port_map_t port_map) {
  return bf_mc_set_lag_membership(shdl, dev, lag_id, port_map);
}

tdi_status_t McMgrIntf::mcMgrGetLagMembership(bf_mc_session_hdl_t shdl,
                                              tdi_dev_id_t dev,
                                              bf_mc_lag_id_t lag_id,
                                              bf_mc_port_map_t port_map,
                                              bool from_hw) {
  return bf_mc_get_lag_membership(shdl, dev, lag_id, port_map, from_hw);
}

tdi_status_t McMgrIntf::mcMgrSetGlobalRid(bf_mc_session_hdl_t shdl,
                                          tdi_dev_id_t dev,
                                          bf_mc_rid_t rid) {
  return bf_mc_set_global_exclusion_rid(shdl, dev, rid);
}

tdi_status_t McMgrIntf::mcMgrEnablePortProtection(bf_mc_session_hdl_t shdl,
                                                  tdi_dev_id_t dev) {
  return bf_mc_enable_port_protection(shdl, dev);
}

tdi_status_t McMgrIntf::mcMgrDisablePortProtection(bf_mc_session_hdl_t shdl,
                                                   tdi_dev_id_t dev) {
  return bf_mc_disable_port_protection(shdl, dev);
}

tdi_status_t McMgrIntf::mcMgrSetPortMcFwdState(bf_mc_session_hdl_t shdl,
                                               tdi_dev_id_t dev,
                                               bf_dev_port_t port_id,
                                               bool is_active) {
  return bf_mc_set_port_mc_fwd_state(shdl, dev, port_id, is_active);
}

tdi_status_t McMgrIntf::mcMgrEnablePortFastFailover(bf_mc_session_hdl_t shdl,
                                                    tdi_dev_id_t dev) {
  return bf_mc_enable_port_fast_failover(shdl, dev);
}

tdi_status_t McMgrIntf::mcMgrDisablePortFastFailover(bf_mc_session_hdl_t shdl,
                                                     tdi_dev_id_t dev) {
  return bf_mc_disable_port_fast_failover(shdl, dev);
}

tdi_status_t McMgrIntf::mcMgrClearFastFailoverState(bf_mc_session_hdl_t shdl,
                                                    tdi_dev_id_t dev,
                                                    bf_dev_port_t port_id) {
  return bf_mc_clear_fast_failover_state(shdl, dev, port_id);
}

tdi_status_t McMgrIntf::mcMgrSetPortProtection(bf_mc_session_hdl_t shdl,
                                               tdi_dev_id_t dev,
                                               bf_dev_port_t protected_port,
                                               bf_dev_port_t backup_port) {
  return bf_mc_set_port_protection(shdl, dev, protected_port, backup_port);
}

tdi_status_t McMgrIntf::mcMgrClearPortProtection(bf_mc_session_hdl_t shdl,
                                                 tdi_dev_id_t dev,
                                                 bf_dev_port_t port) {
  return bf_mc_clear_port_protection(shdl, dev, port);
}

tdi_status_t McMgrIntf::mcMgrSetMaxNodesBeforeYield(bf_mc_session_hdl_t shdl,
                                                    tdi_dev_id_t dev,
                                                    int count) {
  return bf_mc_set_max_nodes_before_yield(shdl, dev, count);
}

tdi_status_t McMgrIntf::mcMgrSetMaxNodeThreshold(bf_mc_session_hdl_t shdl,
                                                 tdi_dev_id_t dev,
                                                 int node_count,
                                                 int node_port_lag_count) {
  return bf_mc_set_max_node_threshold(
      shdl, dev, node_count, node_port_lag_count);
}

tdi_status_t McMgrIntf::mcMgrSetPortForwardState(bf_mc_session_hdl_t shdl,
                                                 tdi_dev_id_t dev,
                                                 bf_dev_port_t port_id,
                                                 bool is_active) {
  return bf_mc_set_port_mc_fwd_state(shdl, dev, port_id, is_active);
}

tdi_status_t McMgrIntf::mcMgrSetCopyToCPUPort(tdi_dev_id_t dev,
                                              bool enable,
                                              bf_dev_port_t port) {
  return bf_mc_set_copy_to_cpu(dev, enable, port);
}

tdi_status_t McMgrIntf::mcMgrGetPortProtection(bf_mc_session_hdl_t shdl,
                                               tdi_dev_id_t dev,
                                               bf_dev_port_t protected_port,
                                               bf_dev_port_t *backup_port) {
  return bf_mc_get_port_protection(shdl, dev, protected_port, backup_port);
}

tdi_status_t McMgrIntf::mcMgrGetPortForwardState(bf_mc_session_hdl_t shdl,
                                                 tdi_dev_id_t dev,
                                                 bf_dev_port_t port_id,
                                                 bool *is_active) {
  return bf_mc_get_port_mc_fwd_state(shdl, dev, port_id, is_active);
}

tdi_status_t McMgrIntf::mcMgrGetFastFailoverState(bf_mc_session_hdl_t shdl,
                                                  tdi_dev_id_t dev,
                                                  bf_dev_port_t port_id,
                                                  bool *is_active) {
  return bf_mc_get_fast_failover_state(shdl, dev, port_id, is_active);
}

tdi_status_t McMgrIntf::mcMgrGetCopyToCPUPort(tdi_dev_id_t dev,
                                              bf_dev_port_t *port,
                                              bool *enable) {
  return bf_mc_get_copy_to_cpu(dev, port, enable);
}
tdi_status_t McMgrIntf::mcMgrSetLagRemoteCountConfig(bf_mc_session_hdl_t shdl,
                                                     tdi_dev_id_t dev,
                                                     bf_mc_lag_id_t lag_id,
                                                     int msb_count,
                                                     int lsb_count) {
  return bf_mc_set_remote_lag_member_count(
      shdl, dev, lag_id, msb_count, lsb_count);
}
tdi_status_t McMgrIntf::mcMgrGetLagRemoteCountConfig(bf_mc_session_hdl_t shdl,
                                                     tdi_dev_id_t dev,
                                                     bf_mc_lag_id_t lag_id,
                                                     int *msb_count,
                                                     int *lsb_count) {
  return bf_mc_get_remote_lag_member_count(
      shdl, dev, lag_id, msb_count, lsb_count);
}
}  // namespace tofino
}  // namespace tna
}  // namespace tdi
