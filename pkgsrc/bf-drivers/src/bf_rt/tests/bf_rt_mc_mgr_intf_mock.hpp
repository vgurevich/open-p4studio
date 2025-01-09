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


#ifndef _BF_RT_MC_MGR_INTERFACE_MOCK_HPP
#define _BF_RT_MC_MGR_INTERFACE_MOCK_HPP

#ifdef __cplusplus
extern "C" {
#endif
#include <bf_rt/bf_rt_common.h>
#include <pipe_mgr/pipe_mgr_intf.h>
#include <mc_mgr/mc_mgr_intf.h>
#ifdef __cplusplus
}
#endif

#include "gmock/gmock.h"

#include <map>

/* bf_rt_includes */
#include <bf_rt/bf_rt_info.hpp>
#include <bf_rt_common/bf_rt_pipe_mgr_intf.hpp>
#include <bf_rt_pre/bf_rt_mc_mgr_intf.hpp>
#include <bf_rt_common/bf_rt_table_data_impl.hpp>
#include <bf_rt_common/bf_rt_table_impl.hpp>

#include <mutex>

namespace bfrt {
namespace bfrt_test {

using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;

class MockIMcMgrIntf : public IMcMgrIntf {
 public:
  static MockIMcMgrIntf *getInstance(const BfRtSession &session) {
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

  MOCK_METHOD0(mcMgrInit, bf_status_t(void));
  MOCK_METHOD6(mcMgrAssociateEcmp,
               bf_status_t(bf_mc_session_hdl_t,
                           bf_dev_id_t,
                           bf_mc_mgrp_hdl_t,
                           bf_mc_ecmp_hdl_t,
                           bool,
                           bf_mc_l1_xid_t));
  MOCK_METHOD6(mcMgrAssociateNode,
               bf_status_t(bf_mc_session_hdl_t,
                           bf_dev_id_t,
                           bf_mc_mgrp_hdl_t,
                           bf_mc_node_hdl_t,
                           bool,
                           bf_mc_l1_xid_t));
  MOCK_METHOD1(mcMgrBeginBatch, bf_status_t(bf_mc_session_hdl_t));
  MOCK_METHOD3(mcMgrClearFastFailoverState,
               bf_status_t(bf_mc_session_hdl_t, bf_dev_id_t, bf_dev_port_t));
  MOCK_METHOD3(mcMgrClearPortProtection,
               bf_status_t(bf_mc_session_hdl_t, bf_dev_id_t, bf_dev_port_t));
  MOCK_METHOD1(mcMgrCompleteOperations, bf_status_t(bf_mc_session_hdl_t));
  MOCK_METHOD1(mcMgrCreateSession, bf_status_t(bf_mc_session_hdl_t *));
  MOCK_METHOD1(mcMgrDestroySession, bf_status_t(bf_mc_session_hdl_t));
  MOCK_METHOD2(mcMgrDisablePortFastFailover,
               bf_status_t(bf_mc_session_hdl_t, bf_dev_id_t));
  MOCK_METHOD2(mcMgrDisablePortProtection,
               bf_status_t(bf_mc_session_hdl_t, bf_dev_id_t));
  MOCK_METHOD4(mcMgrDissociateEcmp,
               bf_status_t(bf_mc_session_hdl_t,
                           bf_dev_id_t,
                           bf_mc_mgrp_hdl_t,
                           bf_mc_ecmp_hdl_t));
  MOCK_METHOD4(mcMgrDissociateNode,
               bf_status_t(bf_mc_session_hdl_t,
                           bf_dev_id_t,
                           bf_mc_mgrp_hdl_t,
                           bf_mc_node_hdl_t));
  MOCK_METHOD3(mcMgrEcmpCreate,
               bf_status_t(bf_mc_session_hdl_t shld,
                           bf_dev_id_t,
                           bf_mc_ecmp_hdl_t *));
  MOCK_METHOD3(mcMgrEcmpDestroy,
               bf_status_t(bf_mc_session_hdl_t, bf_dev_id_t, bf_mc_ecmp_hdl_t));
  MOCK_METHOD6(mcMgrEcmpGetAssocAttr,
               bf_status_t(bf_mc_session_hdl_t,
                           bf_dev_id_t,
                           bf_mc_mgrp_hdl_t,
                           bf_mc_ecmp_hdl_t,
                           bool *,
                           bf_mc_l1_xid_t *));
  MOCK_METHOD3(mcMgrEcmpGetCount,
               bf_status_t(bf_mc_session_hdl_t, bf_dev_id_t, uint32_t *));
  MOCK_METHOD3(mcMgrEcmpGetFirst,
               bf_status_t(bf_mc_session_hdl_t,
                           bf_dev_id_t,
                           bf_mc_ecmp_hdl_t *));
  MOCK_METHOD4(mcMgrEcmpGetFirstMbr,
               bf_status_t(bf_mc_session_hdl_t,
                           bf_dev_id_t,
                           bf_mc_ecmp_hdl_t,
                           bf_mc_node_hdl_t *));
  MOCK_METHOD4(mcMgrEcmpGetMbrCount,
               bf_status_t(bf_mc_session_hdl_t,
                           bf_dev_id_t,
                           bf_mc_ecmp_hdl_t,
                           uint32_t *));
  MOCK_METHOD5(mcMgrEcmpGetNextI,
               bf_status_t(bf_mc_session_hdl_t,
                           bf_dev_id_t,
                           bf_mc_ecmp_hdl_t,
                           uint32_t i,
                           bf_mc_ecmp_hdl_t *));
  MOCK_METHOD6(mcMgrEcmpGetNextIMbr,
               bf_status_t(bf_mc_session_hdl_t,
                           bf_dev_id_t,
                           bf_mc_ecmp_hdl_t,
                           bf_mc_node_hdl_t,
                           uint32_t,
                           bf_mc_node_hdl_t *));
  MOCK_METHOD4(mcMgrEcmpMbrAdd,
               bf_status_t(bf_mc_session_hdl_t,
                           bf_dev_id_t,
                           bf_mc_ecmp_hdl_t,
                           bf_mc_node_hdl_t));
  MOCK_METHOD4(mcMgrEcmpMbrRem,
               bf_status_t(bf_mc_session_hdl_t,
                           bf_dev_id_t,
                           bf_mc_ecmp_hdl_t,
                           bf_mc_node_hdl_t));
  MOCK_METHOD2(mcMgrEnablePortFastFailover,
               bf_status_t(bf_mc_session_hdl_t, bf_dev_id_t));
  MOCK_METHOD2(mcMgrEnablePortProtection,
               bf_status_t(bf_mc_session_hdl_t, bf_dev_id_t));
  MOCK_METHOD2(mcMgrEndBatch, bf_status_t(bf_mc_session_hdl_t, bool));
  MOCK_METHOD1(mcMgrFlushBatch, bf_status_t(bf_mc_session_hdl_t));
  MOCK_METHOD3(mcMgrGetCopyToCPUPort,
               bf_status_t(bf_dev_id_t, bf_dev_port_t *, bool *));
  MOCK_METHOD4(
      mcMgrGetFastFailoverState,
      bf_status_t(bf_mc_session_hdl_t, bf_dev_id_t, bf_dev_port_t, bool *));
  MOCK_METHOD5(mcMgrGetLagMembership,
               bf_status_t(bf_mc_session_hdl_t,
                           bf_dev_id_t,
                           bf_mc_lag_id_t lag_id,
                           bf_mc_port_map_t,
                           bool));
  MOCK_METHOD5(mcMgrGetLagRemoteCountConfig,
               bf_status_t(bf_mc_session_hdl_t,
                           bf_dev_id_t,
                           bf_mc_lag_id_t lag_id,
                           int *,
                           int *));
  MOCK_METHOD4(mcMgrGetPortForwardState,
               bf_status_t(bf_mc_session_hdl_t,
                           bf_dev_id_t,
                           bf_dev_port_t,
                           bool *is_active));
  MOCK_METHOD4(mcMgrGetPortProtection,
               bf_status_t(bf_mc_session_hdl_t,
                           bf_dev_id_t,
                           bf_dev_port_t protected_port,
                           bf_dev_port_t *backup_port));
  MOCK_METHOD5(mcMgrGetPortPruneTable,
               bf_status_t(bf_mc_session_hdl_t,
                           bf_dev_id_t,
                           bf_mc_l2_xid_t l2_exclusion_id,
                           bf_mc_port_map_t *pruned_ports,
                           bool));

  MOCK_METHOD3(mcMgrGetPortPruneTableSize,
               bf_status_t(bf_mc_session_hdl_t, bf_dev_id_t, uint32_t *));
  MOCK_METHOD4(mcMgrMgrpCreate,
               bf_status_t(bf_mc_session_hdl_t,
                           bf_dev_id_t,
                           bf_mc_grp_id_t,
                           bf_mc_mgrp_hdl_t *));
  MOCK_METHOD3(mcMgrMgrpDestroy,
               bf_status_t(bf_mc_session_hdl_t, bf_dev_id_t, bf_mc_mgrp_hdl_t));
  MOCK_METHOD4(mcMgrMgrpGetAttr,
               bf_status_t(bf_mc_session_hdl_t,
                           bf_dev_id_t,
                           bf_mc_mgrp_hdl_t,
                           bf_mc_grp_id_t *));
  MOCK_METHOD3(mcMgrMgrpGetCount,
               bf_status_t(bf_mc_session_hdl_t, bf_dev_id_t, uint32_t *));
  MOCK_METHOD4(mcMgrMgrpGetEcmpMbrCount,
               bf_status_t(bf_mc_session_hdl_t,
                           bf_dev_id_t,
                           bf_mc_mgrp_hdl_t,
                           uint32_t *));
  MOCK_METHOD6(mcMgrMgrpGetFirstEcmpMbr,
               bf_status_t(bf_mc_session_hdl_t,
                           bf_dev_id_t,
                           bf_mc_mgrp_hdl_t,
                           bf_mc_ecmp_hdl_t *,
                           bool *,
                           bf_mc_l1_xid_t *));
  MOCK_METHOD6(mcMgrMgrpGetFirstNodeMbr,
               bf_status_t(bf_mc_session_hdl_t,
                           bf_dev_id_t,
                           bf_mc_mgrp_hdl_t,
                           bf_mc_node_hdl_t *,
                           bool *node_l1_xid_valid,
                           bf_mc_l1_xid_t *node_l1_xid));
  MOCK_METHOD8(mcMgrMgrpGetNextIEcmpMbr,
               bf_status_t(bf_mc_session_hdl_t,
                           bf_dev_id_t,
                           bf_mc_mgrp_hdl_t,
                           bf_mc_ecmp_hdl_t,
                           uint32_t,
                           bf_mc_ecmp_hdl_t *,
                           bool *,
                           bf_mc_l1_xid_t *));
  MOCK_METHOD8(mcMgrMgrpGetNextINodeMbr,
               bf_status_t(bf_mc_session_hdl_t,
                           bf_dev_id_t,
                           bf_mc_mgrp_hdl_t,
                           bf_mc_node_hdl_t,
                           uint32_t,
                           bf_mc_node_hdl_t *,
                           bool *,
                           bf_mc_l1_xid_t *));
  MOCK_METHOD4(mcMgrMgrpGetNodeMbrCount,
               bf_status_t(bf_mc_session_hdl_t,
                           bf_dev_id_t,
                           bf_mc_mgrp_hdl_t,
                           uint32_t *));
  MOCK_METHOD6(mcMgrNodeCreate,
               bf_status_t(bf_mc_session_hdl_t,
                           bf_dev_id_t,
                           bf_mc_rid_t rid,
                           bf_mc_port_map_t,
                           bf_mc_lag_map_t,
                           bf_mc_node_hdl_t *));
  MOCK_METHOD3(mcMgrNodeDestroy,
               bf_status_t(bf_mc_session_hdl_t, bf_dev_id_t, bf_mc_node_hdl_t));
  MOCK_METHOD7(mcMgrNodeGetAssociation,
               bf_status_t(bf_mc_session_hdl_t,
                           bf_dev_id_t,
                           bf_mc_node_hdl_t,
                           bool *is_associated,
                           bf_mc_mgrp_hdl_t *mgrp_hdl,
                           bool *,
                           bf_mc_l1_xid_t *));
  MOCK_METHOD6(mcMgrNodeGetAttr,
               bf_status_t(bf_mc_session_hdl_t,
                           bf_dev_id_t,
                           bf_mc_node_hdl_t,
                           bf_mc_rid_t *rid,
                           bf_mc_port_map_t,
                           bf_mc_lag_map_t));
  MOCK_METHOD3(mcMgrNodeGetCount,
               bf_status_t(bf_mc_session_hdl_t, bf_dev_id_t, uint32_t *));
  MOCK_METHOD3(mcMgrNodeGetFirst,
               bf_status_t(bf_mc_session_hdl_t,
                           bf_dev_id_t,
                           bf_mc_node_hdl_t *));
  MOCK_METHOD5(mcMgrNodeGetNextI,
               bf_status_t(bf_mc_session_hdl_t,
                           bf_dev_id_t,
                           bf_mc_node_hdl_t,
                           uint32_t i,
                           bf_mc_node_hdl_t *));
  MOCK_METHOD5(mcMgrNodeUpdate,
               bf_status_t(bf_mc_session_hdl_t,
                           bf_dev_id_t,
                           bf_mc_node_hdl_t,
                           bf_mc_port_map_t,
                           bf_mc_lag_map_t));
  MOCK_METHOD3(mcMgrSetCopyToCPUPort,
               bf_status_t(bf_dev_id_t, bool, bf_dev_port_t));
  MOCK_METHOD3(mcMgrSetGlobalRid,
               bf_status_t(bf_mc_session_hdl_t, bf_dev_id_t, bf_mc_rid_t));
  MOCK_METHOD4(mcMgrSetLagMembership,
               bf_status_t(bf_mc_session_hdl_t,
                           bf_dev_id_t,
                           bf_mc_lag_id_t lag_id,
                           bf_mc_port_map_t));
  MOCK_METHOD5(
      mcMgrSetLagRemoteCountConfig,
      bf_status_t(bf_mc_session_hdl_t, bf_dev_id_t, bf_mc_lag_id_t, int, int));
  MOCK_METHOD4(mcMgrSetMaxNodeThreshold,
               bf_status_t(bf_mc_session_hdl_t, bf_dev_id_t, int, int));
  MOCK_METHOD3(mcMgrSetMaxNodesBeforeYield,
               bf_status_t(bf_mc_session_hdl_t, bf_dev_id_t, int));
  MOCK_METHOD4(
      mcMgrSetPortForwardState,
      bf_status_t(bf_mc_session_hdl_t, bf_dev_id_t, bf_dev_port_t, bool));
  MOCK_METHOD4(
      mcMgrSetPortMcFwdState,
      bf_status_t(bf_mc_session_hdl_t, bf_dev_id_t, bf_dev_port_t, bool));
  MOCK_METHOD4(mcMgrSetPortProtection,
               bf_status_t(bf_mc_session_hdl_t,
                           bf_dev_id_t,
                           bf_dev_port_t,
                           bf_dev_port_t));
  MOCK_METHOD4(mcMgrSetPortPruneTable,
               bf_status_t(bf_mc_session_hdl_t,
                           bf_dev_id_t,
                           bf_mc_l2_xid_t,
                           bf_mc_port_map_t));
  MOCK_METHOD5(mcMgrEcmpMbrMod,
               bf_status_t(bf_mc_session_hdl_t shdl,
                           bf_dev_id_t dev,
                           bf_mc_ecmp_hdl_t ecmp_hdl,
                           bf_mc_node_hdl_t *node_hdls,
                           uint32_t size));

};  // class MockIMcMgrIntf

}  // namespace bfrt_test
}  // namespace bfrt

#endif  // _BF_RT_MC_MGR_INTERFACE_MOCK_HPP
