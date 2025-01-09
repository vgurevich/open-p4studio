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


#ifndef _TDI_MC_MGR_INTERFACE_HPP
#define _TDI_MC_MGR_INTERFACE_HPP

#ifdef __cplusplus
extern "C" {
#endif
#include <bf_types/bf_types.h>
#include <target-sys/bf_sal/bf_sys_mem.h>
#include <mc_mgr/mc_mgr_intf.h>
#ifdef __cplusplus
}
#endif

/* tdi_includes */
#include <tdi/common/tdi_session.hpp>
#include <tdi/common/tdi_utils.hpp>

// TDI Tofino includes
#include <tdi_tofino/tdi_tofino_defs.h>

// local
#include "../tdi_common/tdi_tofino_session.hpp"

namespace tdi {
namespace tna {
namespace tofino {

class IMcMgrIntf {
 public:
  // Library init API
  virtual ~IMcMgrIntf() = default;
  virtual tdi_status_t mcMgrInit(void) = 0;

  virtual tdi_status_t mcMgrCreateSession(bf_mc_session_hdl_t *shdl) = 0;
  virtual tdi_status_t mcMgrDestroySession(bf_mc_session_hdl_t hdl) = 0;
  virtual tdi_status_t mcMgrBeginBatch(bf_mc_session_hdl_t shdl) = 0;
  virtual tdi_status_t mcMgrFlushBatch(bf_mc_session_hdl_t shdl) = 0;
  virtual tdi_status_t mcMgrEndBatch(bf_mc_session_hdl_t shdl,
                                     bool hwSynchronous) = 0;
  virtual tdi_status_t mcMgrCompleteOperations(bf_mc_session_hdl_t shdl) = 0;

  virtual tdi_status_t mcMgrMgrpCreate(bf_mc_session_hdl_t shdl,
                                       tdi_dev_id_t dev,
                                       bf_mc_grp_id_t grp,
                                       bf_mc_mgrp_hdl_t *hdl) = 0;
  virtual tdi_status_t mcMgrMgrpGetAttr(bf_mc_session_hdl_t shdl,
                                        tdi_dev_id_t dev,
                                        bf_mc_mgrp_hdl_t mgrp_hdl,
                                        bf_mc_grp_id_t *grp) = 0;
  virtual tdi_status_t mcMgrMgrpDestroy(bf_mc_session_hdl_t shdl,
                                        tdi_dev_id_t dev,
                                        bf_mc_mgrp_hdl_t mgrp_hdl) = 0;
  virtual tdi_status_t mcMgrMgrpGetFirstNodeMbr(
      bf_mc_session_hdl_t shdl,
      tdi_dev_id_t dev,
      bf_mc_mgrp_hdl_t mgrp_hdl,
      bf_mc_node_hdl_t *node_hdl,
      bool *node_l1_xid_valid,
      bf_mc_l1_xid_t *node_l1_xid) = 0;
  virtual tdi_status_t mcMgrMgrpGetNodeMbrCount(bf_mc_session_hdl_t shdl,
                                                tdi_dev_id_t dev,
                                                bf_mc_mgrp_hdl_t mgrp_hdl,
                                                uint32_t *count) = 0;
  virtual tdi_status_t mcMgrMgrpGetNextINodeMbr(
      bf_mc_session_hdl_t shdl,
      tdi_dev_id_t dev,
      bf_mc_mgrp_hdl_t mgrp_hdl,
      bf_mc_node_hdl_t node_hdl,
      uint32_t i,
      bf_mc_node_hdl_t *next_node_hdls,
      bool *next_node_l1_xids_valid,
      bf_mc_l1_xid_t *next_node_l1_xids) = 0;
  virtual tdi_status_t mcMgrMgrpGetFirstEcmpMbr(
      bf_mc_session_hdl_t shdl,
      tdi_dev_id_t dev,
      bf_mc_mgrp_hdl_t mgrp_hdl,
      bf_mc_ecmp_hdl_t *ecmp_hdl,
      bool *ecmp_l1_xid_valid,
      bf_mc_l1_xid_t *ecmp_l1_xid) = 0;
  virtual tdi_status_t mcMgrMgrpGetEcmpMbrCount(bf_mc_session_hdl_t shdl,
                                                tdi_dev_id_t dev,
                                                bf_mc_mgrp_hdl_t mgrp_hdl,
                                                uint32_t *count) = 0;
  virtual tdi_status_t mcMgrMgrpGetNextIEcmpMbr(
      bf_mc_session_hdl_t shdl,
      tdi_dev_id_t dev,
      bf_mc_mgrp_hdl_t mgrp_hdl,
      bf_mc_ecmp_hdl_t ecmp_hdl,
      uint32_t i,
      bf_mc_ecmp_hdl_t *next_ecmp_hdls,
      bool *next_ecmp_l1_xids_valid,
      bf_mc_l1_xid_t *next_ecmp_l1_xids) = 0;
  virtual tdi_status_t mcMgrMgrpGetCount(bf_mc_session_hdl_t shdl,
                                         tdi_dev_id_t dev,
                                         uint32_t *count) = 0;
  virtual tdi_status_t mcMgrNodeCreate(bf_mc_session_hdl_t shdl,
                                       tdi_dev_id_t dev,
                                       bf_mc_rid_t rid,
                                       bf_mc_port_map_t port_map,
                                       bf_mc_lag_map_t lag_map,
                                       bf_mc_node_hdl_t *node_hdl) = 0;
  virtual tdi_status_t mcMgrNodeGetAttr(bf_mc_session_hdl_t shdl,
                                        tdi_dev_id_t dev,
                                        bf_mc_node_hdl_t node_hdl,
                                        bf_mc_rid_t *rid,
                                        bf_mc_port_map_t port_map,
                                        bf_mc_lag_map_t lag_map) = 0;
  virtual tdi_status_t mcMgrNodeDestroy(bf_mc_session_hdl_t shdl,
                                        tdi_dev_id_t dev,
                                        bf_mc_node_hdl_t node_hdl) = 0;
  virtual tdi_status_t mcMgrNodeGetFirst(bf_mc_session_hdl_t shdl,
                                         tdi_dev_id_t dev,
                                         bf_mc_node_hdl_t *node_hdl) = 0;
  virtual tdi_status_t mcMgrNodeGetCount(bf_mc_session_hdl_t shdl,
                                         tdi_dev_id_t dev,
                                         uint32_t *count) = 0;
  virtual tdi_status_t mcMgrNodeGetNextI(bf_mc_session_hdl_t shdl,
                                         tdi_dev_id_t dev,
                                         bf_mc_node_hdl_t node_hdl,
                                         uint32_t i,
                                         bf_mc_node_hdl_t *next_node_hdls) = 0;
  virtual tdi_status_t mcMgrNodeGetAssociation(
      bf_mc_session_hdl_t shdl,
      tdi_dev_id_t dev,
      bf_mc_node_hdl_t node_hdl,
      bool *is_associated,
      bf_mc_mgrp_hdl_t *mgrp_hdl,
      bool *level1_exclusion_id_valid,
      bf_mc_l1_xid_t *level1_exclusion_id) = 0;
  virtual tdi_status_t mcMgrNodeUpdate(bf_mc_session_hdl_t shdl,
                                       tdi_dev_id_t dev,
                                       bf_mc_node_hdl_t node_hdl,
                                       bf_mc_port_map_t port_map,
                                       bf_mc_lag_map_t lag_map) = 0;
  virtual tdi_status_t mcMgrEcmpCreate(bf_mc_session_hdl_t shld,
                                       tdi_dev_id_t dev,
                                       bf_mc_ecmp_hdl_t *ecmp_hdl) = 0;
  virtual tdi_status_t mcMgrEcmpDestroy(bf_mc_session_hdl_t shdl,
                                        tdi_dev_id_t dev,
                                        bf_mc_ecmp_hdl_t ecmp_hdl) = 0;
  virtual tdi_status_t mcMgrEcmpGetFirst(bf_mc_session_hdl_t shdl,
                                         tdi_dev_id_t dev,
                                         bf_mc_ecmp_hdl_t *ecmp_hdl) = 0;
  virtual tdi_status_t mcMgrEcmpGetCount(bf_mc_session_hdl_t shdl,
                                         tdi_dev_id_t dev,
                                         uint32_t *count) = 0;
  virtual tdi_status_t mcMgrEcmpGetNextI(bf_mc_session_hdl_t shdl,
                                         tdi_dev_id_t dev,
                                         bf_mc_ecmp_hdl_t ecmp_hdl,
                                         uint32_t i,
                                         bf_mc_ecmp_hdl_t *next_ecmp_hdls) = 0;
  virtual tdi_status_t mcMgrEcmpMbrAdd(bf_mc_session_hdl_t shdl,
                                       tdi_dev_id_t dev,
                                       bf_mc_ecmp_hdl_t ecmp_hdl,
                                       bf_mc_node_hdl_t node_hdl) = 0;
  virtual tdi_status_t mcMgrEcmpMbrRem(bf_mc_session_hdl_t shdl,
                                       tdi_dev_id_t dev,
                                       bf_mc_ecmp_hdl_t ecmp_hdl,
                                       bf_mc_node_hdl_t node_hdl) = 0;
  virtual tdi_status_t mcMgrEcmpMbrMod(bf_mc_session_hdl_t shdl,
                                       tdi_dev_id_t dev,
                                       bf_mc_ecmp_hdl_t ecmp_hdl,
                                       bf_mc_node_hdl_t *node_hdls,
                                       uint32_t size) = 0;
  virtual tdi_status_t mcMgrEcmpGetFirstMbr(bf_mc_session_hdl_t shdl,
                                            tdi_dev_id_t dev,
                                            bf_mc_ecmp_hdl_t ecmp_hdl,
                                            bf_mc_node_hdl_t *node_hdl) = 0;
  virtual tdi_status_t mcMgrEcmpGetMbrCount(bf_mc_session_hdl_t shdl,
                                            tdi_dev_id_t dev,
                                            bf_mc_ecmp_hdl_t ecmp_hdl,
                                            uint32_t *count) = 0;
  virtual tdi_status_t mcMgrEcmpGetNextIMbr(
      bf_mc_session_hdl_t shdl,
      tdi_dev_id_t dev,
      bf_mc_ecmp_hdl_t ecmp_hdl,
      bf_mc_node_hdl_t node_hdl,
      uint32_t i,
      bf_mc_node_hdl_t *next_node_hdls) = 0;
  virtual tdi_status_t mcMgrAssociateNode(
      bf_mc_session_hdl_t shdl,
      tdi_dev_id_t dev,
      bf_mc_mgrp_hdl_t mgrp_hdl,
      bf_mc_node_hdl_t node_hdl,
      bool level1_exclusion_id_valid,
      bf_mc_l1_xid_t level1_exclusion_id) = 0;
  virtual tdi_status_t mcMgrDissociateNode(bf_mc_session_hdl_t shdl,
                                           tdi_dev_id_t dev,
                                           bf_mc_mgrp_hdl_t mgrp_hdl,
                                           bf_mc_node_hdl_t node_hdl) = 0;
  virtual tdi_status_t mcMgrAssociateEcmp(
      bf_mc_session_hdl_t shdl,
      tdi_dev_id_t dev,
      bf_mc_mgrp_hdl_t mgrp_hdl,
      bf_mc_ecmp_hdl_t ecmp_hdl,
      bool level1_exclusion_id_valid,
      bf_mc_l1_xid_t level1_exclusion_id) = 0;
  virtual tdi_status_t mcMgrEcmpGetAssocAttr(
      bf_mc_session_hdl_t shdl,
      tdi_dev_id_t dev,
      bf_mc_mgrp_hdl_t mgrp_hdl,
      bf_mc_ecmp_hdl_t ecmp_hdl,
      bool *level1_exclusion_id_valid,
      bf_mc_l1_xid_t *level1_exclusion_id) = 0;
  virtual tdi_status_t mcMgrDissociateEcmp(bf_mc_session_hdl_t shdl,
                                           tdi_dev_id_t dev,
                                           bf_mc_mgrp_hdl_t mgrp_hdl,
                                           bf_mc_ecmp_hdl_t ecmp_hdl) = 0;
  virtual tdi_status_t mcMgrSetPortPruneTable(
      bf_mc_session_hdl_t shdl,
      tdi_dev_id_t dev,
      bf_mc_l2_xid_t l2_exclusion_id,
      bf_mc_port_map_t pruned_ports) = 0;
  virtual tdi_status_t mcMgrGetPortPruneTable(bf_mc_session_hdl_t shdl,
                                              tdi_dev_id_t dev,
                                              bf_mc_l2_xid_t l2_exclusion_id,
                                              bf_mc_port_map_t *pruned_ports,
                                              bool from_hw) = 0;
  virtual tdi_status_t mcMgrSetLagMembership(bf_mc_session_hdl_t shdl,
                                             tdi_dev_id_t dev,
                                             bf_mc_lag_id_t lag_id,
                                             bf_mc_port_map_t port_map) = 0;
  virtual tdi_status_t mcMgrGetLagMembership(bf_mc_session_hdl_t shdl,
                                             tdi_dev_id_t dev,
                                             bf_mc_lag_id_t lag_id,
                                             bf_mc_port_map_t port_map,
                                             bool from_hw) = 0;
  virtual tdi_status_t mcMgrSetGlobalRid(bf_mc_session_hdl_t shdl,
                                         tdi_dev_id_t dev,
                                         bf_mc_rid_t rid) = 0;
  virtual tdi_status_t mcMgrEnablePortProtection(bf_mc_session_hdl_t shdl,
                                                 tdi_dev_id_t dev) = 0;
  virtual tdi_status_t mcMgrDisablePortProtection(bf_mc_session_hdl_t shdl,
                                                  tdi_dev_id_t dev) = 0;
  virtual tdi_status_t mcMgrSetPortMcFwdState(bf_mc_session_hdl_t shdl,
                                              tdi_dev_id_t dev,
                                              bf_dev_port_t port_id,
                                              bool is_active) = 0;
  virtual tdi_status_t mcMgrEnablePortFastFailover(bf_mc_session_hdl_t shdl,
                                                   tdi_dev_id_t dev) = 0;
  virtual tdi_status_t mcMgrDisablePortFastFailover(bf_mc_session_hdl_t shdl,
                                                    tdi_dev_id_t dev) = 0;
  virtual tdi_status_t mcMgrClearFastFailoverState(bf_mc_session_hdl_t shdl,
                                                   tdi_dev_id_t dev,
                                                   bf_dev_port_t port_id) = 0;
  virtual tdi_status_t mcMgrSetPortProtection(bf_mc_session_hdl_t shdl,
                                              tdi_dev_id_t dev,
                                              bf_dev_port_t protected_port,
                                              bf_dev_port_t backup_port) = 0;
  virtual tdi_status_t mcMgrClearPortProtection(bf_mc_session_hdl_t shdl,
                                                tdi_dev_id_t dev,
                                                bf_dev_port_t port) = 0;

  virtual tdi_status_t mcMgrSetMaxNodesBeforeYield(bf_mc_session_hdl_t shdl,
                                                   tdi_dev_id_t dev,
                                                   int count) = 0;

  virtual tdi_status_t mcMgrSetMaxNodeThreshold(bf_mc_session_hdl_t shdl,
                                                tdi_dev_id_t dev,
                                                int node_count,
                                                int node_port_lag_count) = 0;

  virtual tdi_status_t mcMgrSetPortForwardState(bf_mc_session_hdl_t shdl,
                                                tdi_dev_id_t dev,
                                                bf_dev_port_t port_id,
                                                bool is_active) = 0;

  virtual tdi_status_t mcMgrSetCopyToCPUPort(tdi_dev_id_t dev,
                                             bool enable,
                                             bf_dev_port_t port) = 0;

  virtual tdi_status_t mcMgrGetPortProtection(bf_mc_session_hdl_t shdl,
                                              tdi_dev_id_t dev,
                                              bf_dev_port_t protected_port,
                                              bf_dev_port_t *backup_port) = 0;

  virtual tdi_status_t mcMgrGetPortForwardState(bf_mc_session_hdl_t shdl,
                                                tdi_dev_id_t dev,
                                                bf_dev_port_t port_id,
                                                bool *is_active) = 0;

  virtual tdi_status_t mcMgrGetCopyToCPUPort(tdi_dev_id_t dev,
                                             bf_dev_port_t *port,
                                             bool *enable) = 0;

  virtual tdi_status_t mcMgrGetFastFailoverState(bf_mc_session_hdl_t shdl,
                                                 tdi_dev_id_t dev,
                                                 bf_dev_port_t port_id,
                                                 bool *is_active) = 0;
  virtual tdi_status_t mcMgrSetLagRemoteCountConfig(bf_mc_session_hdl_t shdl,
                                                    tdi_dev_id_t dev,
                                                    bf_mc_lag_id_t lag_id,
                                                    int msb_count,
                                                    int lsb_count) = 0;
  virtual tdi_status_t mcMgrGetLagRemoteCountConfig(bf_mc_session_hdl_t shdl,
                                                    tdi_dev_id_t dev,
                                                    bf_mc_lag_id_t lag_id,
                                                    int *msb_count,
                                                    int *lsb_count) = 0;

 protected:
  static std::unique_ptr<IMcMgrIntf> instance;
  static std::mutex mc_mgr_intf_mtx;
};

class McMgrIntf : public IMcMgrIntf {
 public:
  virtual ~McMgrIntf() {
    if (instance) {
      instance.release();
    }
  };
  McMgrIntf() = default;

  static IMcMgrIntf *getInstance(const tdi::Session &session) {
    auto mc_mgr_intf = McMgrIntf::getInstance();

    const auto &sess = static_cast<const tdi::tna::tofino::Session &>(session);
    if (sess.isInBatch() && !sess.isInMcBatch()) {
      auto status = mc_mgr_intf->mcMgrBeginBatch(sess.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_TOFINO_MGR_TYPE_MC_MGR)));
      if (status != TDI_SUCCESS) {
        LOG_ERROR("%s:%d Failed to grab mc_mgr batch %s",
                  __func__,
                  __LINE__,
                  bf_err_str(status));
      } else {
        sess.setMcBatch(true);
      }
    }

    return mc_mgr_intf;
  }

  static IMcMgrIntf *getInstance() {
    if (instance.get() == nullptr) {
      mc_mgr_intf_mtx.lock();
      if (instance.get() == nullptr) {
        instance.reset(new McMgrIntf());
      }
      mc_mgr_intf_mtx.unlock();
    }
    return IMcMgrIntf::instance.get();
  }

  // Library init API
  tdi_status_t mcMgrInit(void) override;

  // MC mgr session related APIs
  tdi_status_t mcMgrCreateSession(bf_mc_session_hdl_t *shdl);
  tdi_status_t mcMgrDestroySession(bf_mc_session_hdl_t shdl);
  tdi_status_t mcMgrBeginBatch(bf_mc_session_hdl_t shdl);
  tdi_status_t mcMgrFlushBatch(bf_mc_session_hdl_t shdl);
  tdi_status_t mcMgrEndBatch(bf_mc_session_hdl_t shdl, bool hwSynchronous);
  tdi_status_t mcMgrCompleteOperations(bf_mc_session_hdl_t shdl);

  // MC mgr APIs
  tdi_status_t mcMgrMgrpCreate(bf_mc_session_hdl_t shdl,
                               tdi_dev_id_t dev,
                               bf_mc_grp_id_t grp,
                               bf_mc_mgrp_hdl_t *hdl);
  tdi_status_t mcMgrMgrpGetAttr(bf_mc_session_hdl_t shdl,
                                tdi_dev_id_t dev,
                                bf_mc_mgrp_hdl_t mgrp_hdl,
                                bf_mc_grp_id_t *grp);
  tdi_status_t mcMgrMgrpDestroy(bf_mc_session_hdl_t shdl,
                                tdi_dev_id_t dev,
                                bf_mc_mgrp_hdl_t mgrp_hdl);
  tdi_status_t mcMgrMgrpGetFirstNodeMbr(bf_mc_session_hdl_t shdl,
                                        tdi_dev_id_t dev,
                                        bf_mc_mgrp_hdl_t mgrp_hdl,
                                        bf_mc_node_hdl_t *node_hdl,
                                        bool *node_l1_xid_valid,
                                        bf_mc_l1_xid_t *node_l1_xid);
  tdi_status_t mcMgrMgrpGetNodeMbrCount(bf_mc_session_hdl_t shdl,
                                        tdi_dev_id_t dev,
                                        bf_mc_mgrp_hdl_t mgrp_hdl,
                                        uint32_t *count);
  tdi_status_t mcMgrMgrpGetNextINodeMbr(bf_mc_session_hdl_t shdl,
                                        tdi_dev_id_t dev,
                                        bf_mc_mgrp_hdl_t mgrp_hdl,
                                        bf_mc_node_hdl_t node_hdl,
                                        uint32_t i,
                                        bf_mc_node_hdl_t *next_node_hdls,
                                        bool *next_node_l1_xids_valid,
                                        bf_mc_l1_xid_t *next_node_l1_xids);
  tdi_status_t mcMgrMgrpGetFirstEcmpMbr(bf_mc_session_hdl_t shdl,
                                        tdi_dev_id_t dev,
                                        bf_mc_mgrp_hdl_t mgrp_hdl,
                                        bf_mc_ecmp_hdl_t *ecmp_hdl,
                                        bool *ecmp_l1_xid_valid,
                                        bf_mc_l1_xid_t *ecmp_l1_xid);
  tdi_status_t mcMgrMgrpGetEcmpMbrCount(bf_mc_session_hdl_t shdl,
                                        tdi_dev_id_t dev,
                                        bf_mc_mgrp_hdl_t mgrp_hdl,
                                        uint32_t *count);
  tdi_status_t mcMgrMgrpGetNextIEcmpMbr(bf_mc_session_hdl_t shdl,
                                        tdi_dev_id_t dev,
                                        bf_mc_mgrp_hdl_t mgrp_hdl,
                                        bf_mc_ecmp_hdl_t ecmp_hdl,
                                        uint32_t i,
                                        bf_mc_ecmp_hdl_t *next_ecmp_hdls,
                                        bool *next_ecmp_l1_xids_valid,
                                        bf_mc_l1_xid_t *next_ecmp_l1_xids);
  tdi_status_t mcMgrMgrpGetCount(bf_mc_session_hdl_t shdl,
                                 tdi_dev_id_t dev,
                                 uint32_t *count);
  tdi_status_t mcMgrNodeCreate(bf_mc_session_hdl_t shdl,
                               tdi_dev_id_t dev,
                               bf_mc_rid_t rid,
                               bf_mc_port_map_t port_map,
                               bf_mc_lag_map_t lag_map,
                               bf_mc_node_hdl_t *node_hdl);
  tdi_status_t mcMgrNodeGetAttr(bf_mc_session_hdl_t shdl,
                                tdi_dev_id_t dev,
                                bf_mc_node_hdl_t node_hdl,
                                bf_mc_rid_t *rid,
                                bf_mc_port_map_t port_map,
                                bf_mc_lag_map_t lag_map);
  tdi_status_t mcMgrNodeDestroy(bf_mc_session_hdl_t shdl,
                                tdi_dev_id_t dev,
                                bf_mc_node_hdl_t node_hdl);
  tdi_status_t mcMgrNodeGetFirst(bf_mc_session_hdl_t shdl,
                                 tdi_dev_id_t dev,
                                 bf_mc_node_hdl_t *node_hdl);
  tdi_status_t mcMgrNodeGetCount(bf_mc_session_hdl_t shdl,
                                 tdi_dev_id_t dev,
                                 uint32_t *count);
  tdi_status_t mcMgrNodeGetNextI(bf_mc_session_hdl_t shdl,
                                 tdi_dev_id_t dev,
                                 bf_mc_node_hdl_t node_hdl,
                                 uint32_t i,
                                 bf_mc_node_hdl_t *next_node_hdls);
  tdi_status_t mcMgrNodeGetAssociation(bf_mc_session_hdl_t shdl,
                                       tdi_dev_id_t dev,
                                       bf_mc_node_hdl_t node_hdl,
                                       bool *is_associated,
                                       bf_mc_mgrp_hdl_t *mgrp_hdl,
                                       bool *level1_exclusion_id_valid,
                                       bf_mc_l1_xid_t *level1_exclusion_id);
  tdi_status_t mcMgrNodeUpdate(bf_mc_session_hdl_t shdl,
                               tdi_dev_id_t dev,
                               bf_mc_node_hdl_t node_hdl,
                               bf_mc_port_map_t port_map,
                               bf_mc_lag_map_t lag_map);
  tdi_status_t mcMgrEcmpCreate(bf_mc_session_hdl_t shld,
                               tdi_dev_id_t dev,
                               bf_mc_ecmp_hdl_t *ecmp_hdl);
  tdi_status_t mcMgrEcmpDestroy(bf_mc_session_hdl_t shdl,
                                tdi_dev_id_t dev,
                                bf_mc_ecmp_hdl_t ecmp_hdl);
  tdi_status_t mcMgrEcmpGetFirst(bf_mc_session_hdl_t shdl,
                                 tdi_dev_id_t dev,
                                 bf_mc_ecmp_hdl_t *ecmp_hdl);
  tdi_status_t mcMgrEcmpGetCount(bf_mc_session_hdl_t shdl,
                                 tdi_dev_id_t dev,
                                 uint32_t *count);
  tdi_status_t mcMgrEcmpGetNextI(bf_mc_session_hdl_t shdl,
                                 tdi_dev_id_t dev,
                                 bf_mc_ecmp_hdl_t ecmp_hdl,
                                 uint32_t i,
                                 bf_mc_ecmp_hdl_t *next_ecmp_hdls);
  tdi_status_t mcMgrEcmpMbrAdd(bf_mc_session_hdl_t shdl,
                               tdi_dev_id_t dev,
                               bf_mc_ecmp_hdl_t ecmp_hdl,
                               bf_mc_node_hdl_t node_hdl);
  tdi_status_t mcMgrEcmpMbrRem(bf_mc_session_hdl_t shdl,
                               tdi_dev_id_t dev,
                               bf_mc_ecmp_hdl_t ecmp_hdl,
                               bf_mc_node_hdl_t node_hdl);
  tdi_status_t mcMgrEcmpGetFirstMbr(bf_mc_session_hdl_t shdl,
                                    tdi_dev_id_t dev,
                                    bf_mc_ecmp_hdl_t ecmp_hdl,
                                    bf_mc_node_hdl_t *node_hdl);
  tdi_status_t mcMgrEcmpGetMbrCount(bf_mc_session_hdl_t shdl,
                                    tdi_dev_id_t dev,
                                    bf_mc_ecmp_hdl_t ecmp_hdl,
                                    uint32_t *count);
  tdi_status_t mcMgrEcmpGetNextIMbr(bf_mc_session_hdl_t shdl,
                                    tdi_dev_id_t dev,
                                    bf_mc_ecmp_hdl_t ecmp_hdl,
                                    bf_mc_node_hdl_t node_hdl,
                                    uint32_t i,
                                    bf_mc_node_hdl_t *next_node_hdls);
  tdi_status_t mcMgrAssociateNode(bf_mc_session_hdl_t shdl,
                                  tdi_dev_id_t dev,
                                  bf_mc_mgrp_hdl_t mgrp_hdl,
                                  bf_mc_node_hdl_t node_hdl,
                                  bool level1_exclusion_id_valid,
                                  bf_mc_l1_xid_t level1_exclusion_id);
  tdi_status_t mcMgrDissociateNode(bf_mc_session_hdl_t shdl,
                                   tdi_dev_id_t dev,
                                   bf_mc_mgrp_hdl_t mgrp_hdl,
                                   bf_mc_node_hdl_t node_hdl);
  tdi_status_t mcMgrAssociateEcmp(bf_mc_session_hdl_t shdl,
                                  tdi_dev_id_t dev,
                                  bf_mc_mgrp_hdl_t mgrp_hdl,
                                  bf_mc_ecmp_hdl_t ecmp_hdl,
                                  bool level1_exclusion_id_valid,
                                  bf_mc_l1_xid_t level1_exclusion_id);
  tdi_status_t mcMgrEcmpGetAssocAttr(bf_mc_session_hdl_t shdl,
                                     tdi_dev_id_t dev,
                                     bf_mc_mgrp_hdl_t mgrp_hdl,
                                     bf_mc_ecmp_hdl_t ecmp_hdl,
                                     bool *level1_exclusion_id_valid,
                                     bf_mc_l1_xid_t *level1_exclusion_id);
  tdi_status_t mcMgrDissociateEcmp(bf_mc_session_hdl_t shdl,
                                   tdi_dev_id_t dev,
                                   bf_mc_mgrp_hdl_t mgrp_hdl,
                                   bf_mc_ecmp_hdl_t ecmp_hdl);
  tdi_status_t mcMgrSetPortPruneTable(bf_mc_session_hdl_t shdl,
                                      tdi_dev_id_t dev,
                                      bf_mc_l2_xid_t l2_exclusion_id,
                                      bf_mc_port_map_t pruned_ports);
  tdi_status_t mcMgrGetPortPruneTable(bf_mc_session_hdl_t shdl,
                                      tdi_dev_id_t dev,
                                      bf_mc_l2_xid_t l2_exclusion_id,
                                      bf_mc_port_map_t *pruned_ports,
                                      bool from_hw);
  tdi_status_t mcMgrSetLagMembership(bf_mc_session_hdl_t shdl,
                                     tdi_dev_id_t dev,
                                     bf_mc_lag_id_t lag_id,
                                     bf_mc_port_map_t port_map);
  tdi_status_t mcMgrGetLagMembership(bf_mc_session_hdl_t shdl,
                                     tdi_dev_id_t dev,
                                     bf_mc_lag_id_t lag_id,
                                     bf_mc_port_map_t port_map,
                                     bool from_hw);
  tdi_status_t mcMgrSetGlobalRid(bf_mc_session_hdl_t shdl,
                                 tdi_dev_id_t dev,
                                 bf_mc_rid_t rid);
  tdi_status_t mcMgrEnablePortProtection(bf_mc_session_hdl_t shdl,
                                         tdi_dev_id_t dev);
  tdi_status_t mcMgrDisablePortProtection(bf_mc_session_hdl_t shdl,
                                          tdi_dev_id_t dev);
  tdi_status_t mcMgrSetPortMcFwdState(bf_mc_session_hdl_t shdl,
                                      tdi_dev_id_t dev,
                                      bf_dev_port_t port_id,
                                      bool is_active);
  tdi_status_t mcMgrEnablePortFastFailover(bf_mc_session_hdl_t shdl,
                                           tdi_dev_id_t dev);
  tdi_status_t mcMgrDisablePortFastFailover(bf_mc_session_hdl_t shdl,
                                            tdi_dev_id_t dev);
  tdi_status_t mcMgrClearFastFailoverState(bf_mc_session_hdl_t shdl,
                                           tdi_dev_id_t dev,
                                           bf_dev_port_t port_id);
  tdi_status_t mcMgrSetPortProtection(bf_mc_session_hdl_t shdl,
                                      tdi_dev_id_t dev,
                                      bf_dev_port_t protected_port,
                                      bf_dev_port_t backup_port);
  tdi_status_t mcMgrClearPortProtection(bf_mc_session_hdl_t shdl,
                                        tdi_dev_id_t dev,
                                        bf_dev_port_t port);
  tdi_status_t mcMgrSetMaxNodesBeforeYield(bf_mc_session_hdl_t shdl,
                                           tdi_dev_id_t dev,
                                           int count);
  tdi_status_t mcMgrSetMaxNodeThreshold(bf_mc_session_hdl_t shdl,
                                        tdi_dev_id_t dev,
                                        int node_count,
                                        int node_port_lag_count);

  tdi_status_t mcMgrSetPortForwardState(bf_mc_session_hdl_t shdl,
                                        tdi_dev_id_t dev,
                                        bf_dev_port_t port_id,
                                        bool is_active);

  tdi_status_t mcMgrSetCopyToCPUPort(tdi_dev_id_t dev,
                                     bool enable,
                                     bf_dev_port_t port);

  tdi_status_t mcMgrGetPortProtection(bf_mc_session_hdl_t shdl,
                                      tdi_dev_id_t dev,
                                      bf_dev_port_t protected_port,
                                      bf_dev_port_t *backup_port);

  tdi_status_t mcMgrGetPortForwardState(bf_mc_session_hdl_t shdl,
                                        tdi_dev_id_t dev,
                                        bf_dev_port_t port_id,
                                        bool *is_active);

  tdi_status_t mcMgrGetCopyToCPUPort(tdi_dev_id_t dev,
                                     bf_dev_port_t *port,
                                     bool *enable);

  tdi_status_t mcMgrGetFastFailoverState(bf_mc_session_hdl_t shdl,
                                         tdi_dev_id_t dev,
                                         bf_dev_port_t port_id,
                                         bool *is_active);

  tdi_status_t mcMgrSetLagRemoteCountConfig(bf_mc_session_hdl_t shdl,
                                            tdi_dev_id_t dev,
                                            bf_mc_lag_id_t lag_id,
                                            int msb_count,
                                            int lsb_count);
  tdi_status_t mcMgrGetLagRemoteCountConfig(bf_mc_session_hdl_t shdl,
                                            tdi_dev_id_t dev,
                                            bf_mc_lag_id_t lag_id,
                                            int *msb_count,
                                            int *lsb_count);

  tdi_status_t mcMgrEcmpMbrMod(bf_mc_session_hdl_t shdl,
                               tdi_dev_id_t dev,
                               bf_mc_ecmp_hdl_t ecmp_hdl,
                               bf_mc_node_hdl_t *node_hdls,
                               uint32_t size);

 private:
  McMgrIntf(const McMgrIntf &src) = delete;
  McMgrIntf &operator=(const McMgrIntf &rhs) = delete;
};

}  // namespace tofino
}  // namespace tna
}  // namespace tdi

#endif  // _TDI_MC_MGR_INTERFACE_HPP
