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

#ifndef _TDI_MC_MGR_INTERFACE_MOCK_HPP
#define _TDI_MC_MGR_INTERFACE_MOCK_HPP

#ifdef __cplusplus
extern "C" {
#endif
#include <tdi/tdi_common.h>
#include <pipe_mgr/pipe_mgr_intf.h>
#include <mc_mgr/mc_mgr_intf.h>
#ifdef __cplusplus
}
#endif

#include "gmock/gmock.h"

#include <map>

/* tdi_includes */
#include <tdi/tdi_info.hpp>
#include <tdi_common/tdi_pipe_mgr_intf.hpp>
#include <tdi_pre/tdi_mc_mgr_intf.hpp>
#include <tdi_common/tdi_table_data_impl.hpp>
#include <tdi_common/tdi_table_impl.hpp>

#include <mutex>

namespace tdi {
namespace tdi_test {

using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;

class MockIMcMgrIntf : public IMcMgrIntf {
 public:
  static MockIMcMgrIntf *getInstance(const TdiSession &session) {
    return MockIMcMgrIntf::getInstance();
  }
  static MockIMcMgrIntf *getInstance() {
    if (instance.get() == nullptr) {
      mc_mgr_intf_mtx.lock();
      if (instance.get() == nullptr) {
        instance.reset(new NiceMock<MockIMcMgrIntf>());
      }
      mc_mgr_intf_mtx.unlock();
    }
    return static_cast<MockIMcMgrIntf *>(IMcMgrIntf::instance.get());
  }
  // We need the following function only in case of MockIMcMgrIntf and not
  // in McMgrIntf. This is because "instance" variable in IMcMgrIntf is
  // a static global. Thus it will go out of scope only when the program
  // terminates, which means that the MockIMcMgrIntf or McMgrIntf object
  // which it owns will be destroyed only at the end of the program. This is
  // exactly what we want for McMgrIntf but not for MockIMcMgrIntf as while
  // running the tests, we want the mock object to be created and destroyed
  // for every test in the test suite in all the test fixtures. This is
  // required as the gtest framework automatically verifies all the
  // expectations on the mock object when the mock object is destroyed and
  // generates failure reports if the expectations have not been met. Thus
  // to enable the testing framework to actually cause the mock object to be
  // destroyed, we need to pass it a reference to the mock object
  static std::unique_ptr<IMcMgrIntf> &getInstanceToUniquePtr() {
    mc_mgr_intf_mtx.lock();
    if (instance.get() == nullptr) {
      instance.reset(new NiceMock<MockIMcMgrIntf>());
    }
    mc_mgr_intf_mtx.unlock();
    return instance;
  }
  ~MockIMcMgrIntf() { int i = 0; }

  MOCK_METHOD0(mcMgrInit, tdi_status_t(void));
  MOCK_METHOD6(mcMgrAssociateEcmp,
               tdi_status_t(bf_mc_session_hdl_t,
                            tdi_dev_id_t,
                            bf_mc_mgrp_hdl_t,
                            bf_mc_ecmp_hdl_t,
                            bool,
                            bf_mc_l1_xid_t));
  MOCK_METHOD6(mcMgrAssociateNode,
               tdi_status_t(bf_mc_session_hdl_t,
                            tdi_dev_id_t,
                            bf_mc_mgrp_hdl_t,
                            bf_mc_node_hdl_t,
                            bool,
                            bf_mc_l1_xid_t));
  MOCK_METHOD1(mcMgrBeginBatch, tdi_status_t(bf_mc_session_hdl_t));
  MOCK_METHOD3(mcMgrClearFastFailoverState,
               tdi_status_t(bf_mc_session_hdl_t, tdi_dev_id_t, bf_dev_port_t));
  MOCK_METHOD3(mcMgrClearPortProtection,
               tdi_status_t(bf_mc_session_hdl_t, tdi_dev_id_t, bf_dev_port_t));
  MOCK_METHOD1(mcMgrCompleteOperations, tdi_status_t(bf_mc_session_hdl_t));
  MOCK_METHOD1(mcMgrCreateSession, tdi_status_t(bf_mc_session_hdl_t *));
  MOCK_METHOD1(mcMgrDestroySession, tdi_status_t(bf_mc_session_hdl_t));
  MOCK_METHOD2(mcMgrDisablePortFastFailover,
               tdi_status_t(bf_mc_session_hdl_t, tdi_dev_id_t));
  MOCK_METHOD2(mcMgrDisablePortProtection,
               tdi_status_t(bf_mc_session_hdl_t, tdi_dev_id_t));
  MOCK_METHOD4(mcMgrDissociateEcmp,
               tdi_status_t(bf_mc_session_hdl_t,
                            tdi_dev_id_t,
                            bf_mc_mgrp_hdl_t,
                            bf_mc_ecmp_hdl_t));
  MOCK_METHOD4(mcMgrDissociateNode,
               tdi_status_t(bf_mc_session_hdl_t,
                            tdi_dev_id_t,
                            bf_mc_mgrp_hdl_t,
                            bf_mc_node_hdl_t));
  MOCK_METHOD3(mcMgrEcmpCreate,
               tdi_status_t(bf_mc_session_hdl_t shld,
                            tdi_dev_id_t,
                            bf_mc_ecmp_hdl_t *));
  MOCK_METHOD3(mcMgrEcmpDestroy,
               tdi_status_t(bf_mc_session_hdl_t,
                            tdi_dev_id_t,
                            bf_mc_ecmp_hdl_t));
  MOCK_METHOD6(mcMgrEcmpGetAssocAttr,
               tdi_status_t(bf_mc_session_hdl_t,
                            tdi_dev_id_t,
                            bf_mc_mgrp_hdl_t,
                            bf_mc_ecmp_hdl_t,
                            bool *,
                            bf_mc_l1_xid_t *));
  MOCK_METHOD3(mcMgrEcmpGetCount,
               tdi_status_t(bf_mc_session_hdl_t, tdi_dev_id_t, uint32_t *));
  MOCK_METHOD3(mcMgrEcmpGetFirst,
               tdi_status_t(bf_mc_session_hdl_t,
                            tdi_dev_id_t,
                            bf_mc_ecmp_hdl_t *));
  MOCK_METHOD4(mcMgrEcmpGetFirstMbr,
               tdi_status_t(bf_mc_session_hdl_t,
                            tdi_dev_id_t,
                            bf_mc_ecmp_hdl_t,
                            bf_mc_node_hdl_t *));
  MOCK_METHOD4(mcMgrEcmpGetMbrCount,
               tdi_status_t(bf_mc_session_hdl_t,
                            tdi_dev_id_t,
                            bf_mc_ecmp_hdl_t,
                            uint32_t *));
  MOCK_METHOD5(mcMgrEcmpGetNextI,
               tdi_status_t(bf_mc_session_hdl_t,
                            tdi_dev_id_t,
                            bf_mc_ecmp_hdl_t,
                            uint32_t i,
                            bf_mc_ecmp_hdl_t *));
  MOCK_METHOD6(mcMgrEcmpGetNextIMbr,
               tdi_status_t(bf_mc_session_hdl_t,
                            tdi_dev_id_t,
                            bf_mc_ecmp_hdl_t,
                            bf_mc_node_hdl_t,
                            uint32_t,
                            bf_mc_node_hdl_t *));
  MOCK_METHOD4(mcMgrEcmpMbrAdd,
               tdi_status_t(bf_mc_session_hdl_t,
                            tdi_dev_id_t,
                            bf_mc_ecmp_hdl_t,
                            bf_mc_node_hdl_t));
  MOCK_METHOD4(mcMgrEcmpMbrRem,
               tdi_status_t(bf_mc_session_hdl_t,
                            tdi_dev_id_t,
                            bf_mc_ecmp_hdl_t,
                            bf_mc_node_hdl_t));
  MOCK_METHOD2(mcMgrEnablePortFastFailover,
               tdi_status_t(bf_mc_session_hdl_t, tdi_dev_id_t));
  MOCK_METHOD2(mcMgrEnablePortProtection,
               tdi_status_t(bf_mc_session_hdl_t, tdi_dev_id_t));
  MOCK_METHOD2(mcMgrEndBatch, tdi_status_t(bf_mc_session_hdl_t, bool));
  MOCK_METHOD1(mcMgrFlushBatch, tdi_status_t(bf_mc_session_hdl_t));
  MOCK_METHOD3(mcMgrGetCopyToCPUPort,
               tdi_status_t(tdi_dev_id_t, bf_dev_port_t *, bool *));
  MOCK_METHOD4(
      mcMgrGetFastFailoverState,
      tdi_status_t(bf_mc_session_hdl_t, tdi_dev_id_t, bf_dev_port_t, bool *));
  MOCK_METHOD5(mcMgrGetLagMembership,
               tdi_status_t(bf_mc_session_hdl_t,
                            tdi_dev_id_t,
                            bf_mc_lag_id_t lag_id,
                            bf_mc_port_map_t,
                            bool));
  MOCK_METHOD5(mcMgrGetLagRemoteCountConfig,
               tdi_status_t(bf_mc_session_hdl_t,
                            tdi_dev_id_t,
                            bf_mc_lag_id_t lag_id,
                            int *,
                            int *));
  MOCK_METHOD4(mcMgrGetPortForwardState,
               tdi_status_t(bf_mc_session_hdl_t,
                            tdi_dev_id_t,
                            bf_dev_port_t,
                            bool *is_active));
  MOCK_METHOD4(mcMgrGetPortProtection,
               tdi_status_t(bf_mc_session_hdl_t,
                            tdi_dev_id_t,
                            bf_dev_port_t protected_port,
                            bf_dev_port_t *backup_port));
  MOCK_METHOD5(mcMgrGetPortPruneTable,
               tdi_status_t(bf_mc_session_hdl_t,
                            tdi_dev_id_t,
                            bf_mc_l2_xid_t l2_exclusion_id,
                            bf_mc_port_map_t *pruned_ports,
                            bool));
  MOCK_METHOD4(mcMgrMgrpCreate,
               tdi_status_t(bf_mc_session_hdl_t,
                            tdi_dev_id_t,
                            bf_mc_grp_id_t,
                            bf_mc_mgrp_hdl_t *));
  MOCK_METHOD3(mcMgrMgrpDestroy,
               tdi_status_t(bf_mc_session_hdl_t,
                            tdi_dev_id_t,
                            bf_mc_mgrp_hdl_t));
  MOCK_METHOD4(mcMgrMgrpGetAttr,
               tdi_status_t(bf_mc_session_hdl_t,
                            tdi_dev_id_t,
                            bf_mc_mgrp_hdl_t,
                            bf_mc_grp_id_t *));
  MOCK_METHOD3(mcMgrMgrpGetCount,
               tdi_status_t(bf_mc_session_hdl_t, tdi_dev_id_t, uint32_t *));
  MOCK_METHOD4(mcMgrMgrpGetEcmpMbrCount,
               tdi_status_t(bf_mc_session_hdl_t,
                            tdi_dev_id_t,
                            bf_mc_mgrp_hdl_t,
                            uint32_t *));
  MOCK_METHOD6(mcMgrMgrpGetFirstEcmpMbr,
               tdi_status_t(bf_mc_session_hdl_t,
                            tdi_dev_id_t,
                            bf_mc_mgrp_hdl_t,
                            bf_mc_ecmp_hdl_t *,
                            bool *,
                            bf_mc_l1_xid_t *));
  MOCK_METHOD6(mcMgrMgrpGetFirstNodeMbr,
               tdi_status_t(bf_mc_session_hdl_t,
                            tdi_dev_id_t,
                            bf_mc_mgrp_hdl_t,
                            bf_mc_node_hdl_t *,
                            bool *node_l1_xid_valid,
                            bf_mc_l1_xid_t *node_l1_xid));
  MOCK_METHOD8(mcMgrMgrpGetNextIEcmpMbr,
               tdi_status_t(bf_mc_session_hdl_t,
                            tdi_dev_id_t,
                            bf_mc_mgrp_hdl_t,
                            bf_mc_ecmp_hdl_t,
                            uint32_t,
                            bf_mc_ecmp_hdl_t *,
                            bool *,
                            bf_mc_l1_xid_t *));
  MOCK_METHOD8(mcMgrMgrpGetNextINodeMbr,
               tdi_status_t(bf_mc_session_hdl_t,
                            tdi_dev_id_t,
                            bf_mc_mgrp_hdl_t,
                            bf_mc_node_hdl_t,
                            uint32_t,
                            bf_mc_node_hdl_t *,
                            bool *,
                            bf_mc_l1_xid_t *));
  MOCK_METHOD4(mcMgrMgrpGetNodeMbrCount,
               tdi_status_t(bf_mc_session_hdl_t,
                            tdi_dev_id_t,
                            bf_mc_mgrp_hdl_t,
                            uint32_t *));
  MOCK_METHOD6(mcMgrNodeCreate,
               tdi_status_t(bf_mc_session_hdl_t,
                            tdi_dev_id_t,
                            bf_mc_rid_t rid,
                            bf_mc_port_map_t,
                            bf_mc_lag_map_t,
                            bf_mc_node_hdl_t *));
  MOCK_METHOD3(mcMgrNodeDestroy,
               tdi_status_t(bf_mc_session_hdl_t,
                            tdi_dev_id_t,
                            bf_mc_node_hdl_t));
  MOCK_METHOD7(mcMgrNodeGetAssociation,
               tdi_status_t(bf_mc_session_hdl_t,
                            tdi_dev_id_t,
                            bf_mc_node_hdl_t,
                            bool *is_associated,
                            bf_mc_mgrp_hdl_t *mgrp_hdl,
                            bool *,
                            bf_mc_l1_xid_t *));
  MOCK_METHOD6(mcMgrNodeGetAttr,
               tdi_status_t(bf_mc_session_hdl_t,
                            tdi_dev_id_t,
                            bf_mc_node_hdl_t,
                            bf_mc_rid_t *rid,
                            bf_mc_port_map_t,
                            bf_mc_lag_map_t));
  MOCK_METHOD3(mcMgrNodeGetCount,
               tdi_status_t(bf_mc_session_hdl_t, tdi_dev_id_t, uint32_t *));
  MOCK_METHOD3(mcMgrNodeGetFirst,
               tdi_status_t(bf_mc_session_hdl_t,
                            tdi_dev_id_t,
                            bf_mc_node_hdl_t *));
  MOCK_METHOD5(mcMgrNodeGetNextI,
               tdi_status_t(bf_mc_session_hdl_t,
                            tdi_dev_id_t,
                            bf_mc_node_hdl_t,
                            uint32_t i,
                            bf_mc_node_hdl_t *));
  MOCK_METHOD5(mcMgrNodeUpdate,
               tdi_status_t(bf_mc_session_hdl_t,
                            tdi_dev_id_t,
                            bf_mc_node_hdl_t,
                            bf_mc_port_map_t,
                            bf_mc_lag_map_t));
  MOCK_METHOD3(mcMgrSetCopyToCPUPort,
               tdi_status_t(tdi_dev_id_t, bool, bf_dev_port_t));
  MOCK_METHOD3(mcMgrSetGlobalRid,
               tdi_status_t(bf_mc_session_hdl_t, tdi_dev_id_t, bf_mc_rid_t));
  MOCK_METHOD4(mcMgrSetLagMembership,
               tdi_status_t(bf_mc_session_hdl_t,
                            tdi_dev_id_t,
                            bf_mc_lag_id_t lag_id,
                            bf_mc_port_map_t));
  MOCK_METHOD5(mcMgrSetLagRemoteCountConfig,
               tdi_status_t(bf_mc_session_hdl_t,
                            tdi_dev_id_t,
                            bf_mc_lag_id_t,
                            int,
                            int));
  MOCK_METHOD4(mcMgrSetMaxNodeThreshold,
               tdi_status_t(bf_mc_session_hdl_t, tdi_dev_id_t, int, int));
  MOCK_METHOD3(mcMgrSetMaxNodesBeforeYield,
               tdi_status_t(bf_mc_session_hdl_t, tdi_dev_id_t, int));
  MOCK_METHOD4(
      mcMgrSetPortForwardState,
      tdi_status_t(bf_mc_session_hdl_t, tdi_dev_id_t, bf_dev_port_t, bool));
  MOCK_METHOD4(
      mcMgrSetPortMcFwdState,
      tdi_status_t(bf_mc_session_hdl_t, tdi_dev_id_t, bf_dev_port_t, bool));
  MOCK_METHOD4(mcMgrSetPortProtection,
               tdi_status_t(bf_mc_session_hdl_t,
                            tdi_dev_id_t,
                            bf_dev_port_t,
                            bf_dev_port_t));
  MOCK_METHOD4(mcMgrSetPortPruneTable,
               tdi_status_t(bf_mc_session_hdl_t,
                            tdi_dev_id_t,
                            bf_mc_l2_xid_t,
                            bf_mc_port_map_t));
  MOCK_METHOD5(mcMgrEcmpMbrMod,
               tdi_status_t(bf_mc_session_hdl_t shdl,
                            tdi_dev_id_t dev,
                            bf_mc_ecmp_hdl_t ecmp_hdl,
                            bf_mc_node_hdl_t *node_hdls,
                            uint32_t size));

};  // class MockIMcMgrIntf

}  // namespace tdi_test
}  // namespace tdi

#endif  // _TDI_MC_MGR_INTERFACE_MOCK_HPP
