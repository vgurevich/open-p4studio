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


#ifndef _BF_RT_TM_MGR_INTERFACE_MOCK_HPP
#define _BF_RT_TM_MGR_INTERFACE_MOCK_HPP

#include <bf_rt/bf_rt_info.hpp>
#include <bf_rt_tm/bf_rt_tm_intf.hpp>
#include "gmock/gmock.h"

namespace bfrt {
namespace bfrt_test {

using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;

class MockITrafficMgrIntf : public ITrafficMgrIntf {
 public:
  static MockITrafficMgrIntf *getInstance() {
    if (instance.get() == nullptr) {
      tm_mgr_intf_mtx.lock();
      if (instance.get() == nullptr) {
        instance.reset(new NiceMock<MockITrafficMgrIntf>());
      }
      tm_mgr_intf_mtx.unlock();
    }
    return static_cast<MockITrafficMgrIntf *>(ITrafficMgrIntf::instance.get());
  }

  // We need the following function only in case of MockITrafficMgrIntf and not
  // in TrafficMgrIntf. This is because "instance" variable in ITrafficMgrIntf
  // is
  // a static global. Thus it will go out of scope only when the program
  // terminates, which means that the MockITrafficMgrIntf or TrafficMgrIntf
  // object
  // which it owns will be destroyed only at the end of the program. This is
  // exactly what we want for TrafficMgrIntf but not for MockITrafficMgrIntf as
  // while
  // running the tests, we want the mock object to be created and destroyed
  // for every test in the test suite in all the test fixtures. This is
  // required as the gtest framework automatically verifies all the
  // expectations on the mock object when the mock object is destroyed and
  // generates failure reports if the expectations have not been met. Thus
  // to enable the testing framework to actually cause the mock object to be
  // destroyed, we need to pass it a reference to the mock object
  static std::unique_ptr<ITrafficMgrIntf> &getInstanceToUniquePtr() {
    if (instance.get() == nullptr) {
      instance.reset(new NiceMock<MockITrafficMgrIntf>());
    }
    return instance;
  }

  /*** TM Cofiguration read API. ***/
  MOCK_METHOD2(bfTMDevCfgGet,
               bf_status_t(bf_dev_id_t dev, bf_tm_dev_cfg_t *cfg));

  /*** TM Control APIs. ***/
  MOCK_METHOD1(bfTMCompleteOperations, bf_status_t(bf_dev_id_t dev));

  /*** TM PPG API. ***/
  MOCK_METHOD3(bfTMPPGAllocate,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_ppg_hdl *ppg));
  MOCK_METHOD2(bfTMPPGFree, bf_status_t(bf_dev_id_t dev, bf_tm_ppg_hdl ppg));
  MOCK_METHOD3(bfTMPPGDefaultPpgGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_ppg_hdl *ppg));
  MOCK_METHOD3(bfTMPPGIcosMappingSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_tm_ppg_hdl ppg,
                           uint8_t icos_bmap));
  MOCK_METHOD2(bfTMPPGLosslessTreatmentEnable,
               bf_status_t(bf_dev_id_t dev, bf_tm_ppg_hdl ppg));
  MOCK_METHOD2(bfTMPPGLosslessTreatmentDisable,
               bf_status_t(bf_dev_id_t dev, bf_tm_ppg_hdl ppg));
  MOCK_METHOD6(bfTMPPGAppPoolUsageSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_tm_ppg_hdl ppg,
                           bf_tm_app_pool_t pool,
                           uint32_t base_use_limit,
                           bf_tm_ppg_baf_t dynamic_baf,
                           uint32_t hysteresis));
  MOCK_METHOD3(bfTMPPGAppPoolUsageDisable,
               bf_status_t(bf_dev_id_t dev,
                           bf_tm_app_pool_t pool,
                           bf_tm_ppg_hdl ppg));
  MOCK_METHOD3(bfTMPPGGuaranteedMinLimitSet,
               bf_status_t(bf_dev_id_t dev, bf_tm_ppg_hdl ppg, uint32_t cells));
  MOCK_METHOD3(bfTMPPGSkidLimitSet,
               bf_status_t(bf_dev_id_t dev, bf_tm_ppg_hdl ppg, uint32_t cells));
  MOCK_METHOD3(bfTMPPGGuaranteedMinSkidHysteresisSet,
               bf_status_t(bf_dev_id_t dev, bf_tm_ppg_hdl ppg, uint32_t cells));
  MOCK_METHOD3(bfTMPPGAppPoolIdGet,
               bf_status_t(bf_dev_id_t dev, bf_tm_ppg_hdl ppg, uint32_t *pool));
  MOCK_METHOD6(bfTMPPGAppPoolUsageGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_tm_ppg_hdl ppg,
                           bf_tm_app_pool_t pool,
                           uint32_t *base_use_limit,
                           bf_tm_ppg_baf_t *dynamic_baf,
                           uint32_t *hysteresis));
  MOCK_METHOD3(bfTMPPGuaranteedMinLimitGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_tm_ppg_hdl ppg,
                           uint32_t *cells));
  MOCK_METHOD3(bfTMPPGLosslessTreatmentGet,
               bf_status_t(bf_dev_id_t dev, bf_tm_ppg_hdl ppg, bool *pfc_val));
  MOCK_METHOD3(bfTMPPGSkidLimitGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_tm_ppg_hdl ppg,
                           uint32_t *cells));
  MOCK_METHOD3(bfTMPPGGuaranteedMinSkidHysteresisGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_tm_ppg_hdl ppg,
                           uint32_t *cells));
  MOCK_METHOD3(bfTMPPGIcosMappingGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_tm_ppg_hdl ppg,
                           uint8_t *icos_bmap));

  MOCK_METHOD3(bfTMPPGTotalCntGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           uint32_t *total_cnt));

  MOCK_METHOD3(bfTMPPGUnusedCntGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           uint32_t *unused_cnt));

  MOCK_METHOD3(bfTMPPGPortGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_tm_ppg_hdl ppg_hdl,
                           bf_dev_port_t *port_id));

  MOCK_METHOD3(bfTMPPGNrGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_tm_ppg_hdl ppg_hdl,
                           bf_tm_ppg_id_t *ppg_nr));

  MOCK_METHOD3(bfTMPPGMirrorPortHandleGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           bf_tm_ppg_hdl *ppg_hdl));

  MOCK_METHOD4(bfTMPPGBufferGetDefault,
               bf_status_t(bf_dev_id_t dev,
                           bf_tm_ppg_hdl ppg_hdl,
                           uint32_t *min_limit_cells,
                           uint32_t *hysteresis_cells));

  MOCK_METHOD5(bfTMPPGAppPoolUsageGetDefault,
               bf_status_t(bf_dev_id_t dev,
                           bf_tm_ppg_hdl ppg_hdl,
                           bf_tm_app_pool_t *pool,
                           uint32_t *pool_max_cells,
                           bf_tm_ppg_baf_t *dynamic_baf));

  /****** TM Pipe API *****/
  MOCK_METHOD2(bfTMPipeIsValid,
               bf_status_t(bf_dev_id_t dev, bf_dev_pipe_t pipe));
  MOCK_METHOD2(bfTMPipeGetCount, bf_status_t(bf_dev_id_t dev, uint8_t *count));
  MOCK_METHOD2(bfTMPipeGetFirst,
               bf_status_t(bf_dev_id_t dev, bf_dev_pipe_t *pipe));
  MOCK_METHOD3(bfTMPipeGetNext,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           bf_dev_pipe_t *pipe_next));

  MOCK_METHOD3(bfTMPipeGetPortCount,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           uint16_t *count));
  MOCK_METHOD3(bfTMPipeGetPortFirst,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           bf_dev_port_t *dev_port));
  MOCK_METHOD3(bfTMPipeGetPortNext,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t dev_port,
                           bf_dev_port_t *dev_port_next));

  MOCK_METHOD4(bfTMPipeGetPortGroupBasePort,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           bf_tm_pg_t pg_id,
                           bf_dev_port_t *dev_port));

  MOCK_METHOD4(bfTMPipeMirrorOnDropDestGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           bf_dev_port_t *port,
                           bf_tm_queue_t *queue));
  MOCK_METHOD4(bfTMPipeMirrorOnDropDestDefaultGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           bf_dev_port_t *port,
                           bf_tm_queue_t *queue));
  MOCK_METHOD4(bfTMPipeMirrorOnDropDestSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue));
  MOCK_METHOD3(bfTMPipeSchedPktIfgCompDefaultGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           uint8_t *adjustment));
  MOCK_METHOD3(bfTMPipeSchedPktIfgCompGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           uint8_t *adjustment));
  MOCK_METHOD3(bfTMPipeSchedPktIfgCompSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           uint8_t adjustment));
  MOCK_METHOD3(bfTMPipeSchedAdvFcModeDefaultGet,
               bf_status_t(bf_dev_id_t dev, bf_dev_pipe_t pipe, bool *enable));
  MOCK_METHOD3(bfTMPipeSchedAdvFcModeGet,
               bf_status_t(bf_dev_id_t dev, bf_dev_pipe_t pipe, bool *enable));
  MOCK_METHOD3(bfTMPipeSchedAdvFcModeSet,
               bf_status_t(bf_dev_id_t dev, bf_dev_pipe_t pipe, bool enable));
  MOCK_METHOD3(bfTMPipeQstatReportDefaultGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           bool *enable_any));
  MOCK_METHOD3(bfTMPipeQstatReportGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           bool *enable_any));
  MOCK_METHOD3(bfTMPipeQstatReportSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           bool enable_any));
  MOCK_METHOD3(bfTMPipeEgHysteresisDefaultGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           uint32_t *cells));
  MOCK_METHOD3(bfTMPipeEgHysteresisGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           uint32_t *cells));
  MOCK_METHOD3(bfTMPipeEgHysteresisSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           uint32_t cells));
  MOCK_METHOD3(bfTMPipeEgLimitDefaultGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           uint32_t *cells));
  MOCK_METHOD3(bfTMPipeEgLimitGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           uint32_t *cells));
  MOCK_METHOD3(bfTMPipeEgLimitSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           uint32_t cells));
  MOCK_METHOD3(bfTMPipeMirrorDropEnableGet,
               bf_status_t(bf_dev_id_t dev, bf_dev_pipe_t pipe, bool *enable));
  MOCK_METHOD3(bfTMPipeMirrorDropEnableDefaultGet,
               bf_status_t(bf_dev_id_t dev, bf_dev_pipe_t pipe, bool *enable));
  MOCK_METHOD3(bfTMPipeMirrorDropEnableSet,
               bf_status_t(bf_dev_id_t dev, bf_dev_pipe_t pipe, bool enable));

  /****** TM Port API *****/
  MOCK_METHOD2(bfTMPortIsValid,
               bf_status_t(bf_dev_id_t dev, bf_dev_port_t port_id));

  MOCK_METHOD7(bfTMPortStatusGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port_id,
                           bool *is_offline,
                           bool *is_enabled,
                           bool *qac_rx_enable,
                           bool *recirc_enable,
                           bool *has_mac));

  MOCK_METHOD3(bfTMPortIcosCntGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port_id,
                           uint8_t *icos_count));

  MOCK_METHOD3(bfTMPortIcosMapDefaultsGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           uint8_t *icos_mask));

  MOCK_METHOD4(bfTMPortPPGMapGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           uint8_t *ppg_mask,
                           bf_tm_ppg_hdl *ppg_hdlrs));

  MOCK_METHOD5(bfTMPortBaseQueueGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port_id,
                           bf_tm_pg_t *pg_id,
                           uint8_t *pg_queue,
                           bool *is_mapped));

  MOCK_METHOD4(bfTMPortQMappingGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           uint8_t *q_count,
                           uint8_t *q_mapping));
  MOCK_METHOD4(bfTMPortQMappingSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           uint8_t q_count,
                           uint8_t *q_mapping));
  //---
  MOCK_METHOD3(bfTMPortCreditsGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           uint32_t *pkt_credits));
  //---
  MOCK_METHOD9(bfTMPortBufferDefaultsGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bool *ct_enable,
                           uint8_t *uc_ct_limit_cells,
                           uint32_t *ig_limit_cells,
                           uint32_t *ig_hysteresis_cells,
                           uint32_t *eg_limit_cells,
                           uint32_t *eg_hysteresis_cells,
                           uint32_t *skid_limit_cells));

  MOCK_METHOD3(bfTMPortCutThroughGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bool *ct_enablei));

  MOCK_METHOD3(bfTMPortCutThroughLimitGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           uint8_t *uc_ct_limit_cells));

  MOCK_METHOD3(bfTMPortBufIgLimitGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           uint32_t *ig_limit_cells));

  MOCK_METHOD3(bfTMPortBufIgHysteresisGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           uint32_t *ig_hysteresis_cells));

  MOCK_METHOD3(bfTMPortBufEgLimitGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           uint32_t *eg_limit_cells));

  MOCK_METHOD3(bfTMPortBufEgHysteresisGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           uint32_t *eg_hysteresis_cells));

  MOCK_METHOD3(bfTMPortBufSkidLimitGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           uint32_t *skid_limit_cells));

  MOCK_METHOD3(bfTMPortCutThroughLimitSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           uint8_t uc_ct_limit_cells));

  MOCK_METHOD3(bfTMPortBufIgLimitSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           uint32_t ig_limit_cells));

  MOCK_METHOD3(bfTMPortBufIgHysteresisSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           uint32_t ig_hysteresis_cells));

  MOCK_METHOD3(bfTMPortBufEgLimitSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           uint32_t eg_limit_cells));

  MOCK_METHOD3(bfTMPortBufEgHysteresisSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           uint32_t eg_hysteresis_cells));

  MOCK_METHOD3(bfTMPortBufSkidLimitSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           uint32_t skid_limit_cells));
  //---
  MOCK_METHOD5(bfTMPortFlowCtrlDefaultsGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_flow_ctrl_type_t *mode_tx,
                           bf_tm_flow_ctrl_type_t *mode_rx,
                           uint8_t *cos_map));

  MOCK_METHOD3(bfTMPortFlowCtrlTxGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_flow_ctrl_type_t *mode_tx));

  MOCK_METHOD3(bfTMPortFlowCtrlRxGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_flow_ctrl_type_t *mode_rx));

  MOCK_METHOD3(bfTMPortCosMappingGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           uint8_t *cos_map));

  MOCK_METHOD3(bfTMPortFlowCtrlTxSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_flow_ctrl_type_t mode_tx));

  MOCK_METHOD3(bfTMPortFlowCtrlRxSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_flow_ctrl_type_t mode_rx));

  MOCK_METHOD3(bfTMPortCosMappingSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           uint8_t *cos_map));
  //---- Port Scheduling
  MOCK_METHOD3(bfTMPortSchedEnable,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_port_speeds_t speed));
  MOCK_METHOD2(bfTMPortSchedDisable,
               bf_status_t(bf_dev_id_t dev, bf_dev_port_t port));
  //---
  MOCK_METHOD3(bfTMPortSchedSpeedGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_port_speeds_t *speed));
  MOCK_METHOD3(bfTMPortSchedSpeedResetGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_port_speeds_t *speed));
  //---
  MOCK_METHOD3(bfTMPortSchedShapingEnableDefaultGet,
               bf_status_t(bf_dev_id_t dev, bf_dev_port_t port, bool *enable));
  MOCK_METHOD3(bfTMPortSchedShapingEnableGet,
               bf_status_t(bf_dev_id_t dev, bf_dev_port_t port, bool *enable));
  MOCK_METHOD2(bfTMPortSchedShapingEnable,
               bf_status_t(bf_dev_id_t dev, bf_dev_port_t port));
  MOCK_METHOD2(bfTMPortSchedShapingDisable,
               bf_status_t(bf_dev_id_t dev, bf_dev_port_t port));
  //---
  MOCK_METHOD6(bfTMPortSchedMaxRateDefaultsGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bool *pps,
                           uint32_t *burst_size,
                           uint32_t *rate,
                           bf_tm_sched_shaper_prov_type_t *prov_type));
  MOCK_METHOD6(bfTMPortSchedMaxRateGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bool *pps,
                           uint32_t *burst_size,
                           uint32_t *rate,
                           bf_tm_sched_shaper_prov_type_t *prov_type));
  MOCK_METHOD6(bfTMPortSchedMaxRateSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bool pps,
                           uint32_t burst_size,
                           uint32_t rate,
                           bf_tm_sched_shaper_prov_type_t prov_type));
  /****** TM Port Group API ******/
  MOCK_METHOD4(bfTMPortGroupGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port_id,
                           bf_tm_pg_t *pg_id,
                           uint8_t *pg_port_nr));

  MOCK_METHOD6(bfTMPortGroupPortQueueGet,
               bf_status_t(const bf_rt_target_t &dev_tgt,
                           bf_tm_pg_t pg_id,
                           uint8_t pg_queue,
                           bf_dev_port_t *port,
                           bf_tm_queue_t *queue_nr,
                           bool *is_mapped));

  /****** TM Queue API *****/
  MOCK_METHOD4(bfTMQueuePfcCosGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           uint8_t *cos));
  MOCK_METHOD4(bfTMQueuePfcCosSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           uint8_t cos));

  MOCK_METHOD4(bfTMQueueColorDropGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           bool *is_enabled));
  MOCK_METHOD4(bfTMQueueColorDropSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           bool enable));

  MOCK_METHOD4(bfTMQueueVisibleGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           bool *is_visible));
  MOCK_METHOD4(bfTMQueueVisibleSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           bool is_visible));
  MOCK_METHOD2(bfTMQueueVisibleDefaultsGet,
               bf_status_t(bf_dev_id_t dev, bool *is_visible));

  MOCK_METHOD5(bfTMQueueColorDropLimitGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           bf_tm_color_t color,
                           bf_tm_queue_color_limit_t *limit));
  MOCK_METHOD5(bfTMQueueColorDropLimitSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           bf_tm_color_t color,
                           bf_tm_queue_color_limit_t limit));

  MOCK_METHOD5(bfTMQueueColorHysteresisGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           bf_tm_color_t color,
                           bf_tm_thres_t *hyst_cells));
  MOCK_METHOD5(bfTMQueueColorHysteresisSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           bf_tm_color_t color,
                           bf_tm_thres_t hyst_cells));

  MOCK_METHOD4(bfTMQueueGuaranteedCellsGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           uint32_t *cells));
  MOCK_METHOD4(bfTMQueueGuaranteedCellsSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           uint32_t cells));

  MOCK_METHOD4(bfTMQueueTailDropGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           bool *is_enabled));
  MOCK_METHOD4(bfTMQueueTailDropSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           bool enable));

  MOCK_METHOD4(bfTMQueueHysteresisGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           uint32_t *hyst_cells));
  MOCK_METHOD4(bfTMQueueHysteresisSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           uint32_t cells));

  MOCK_METHOD7(bfTMQueueAppPoolGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           bf_tm_app_pool_t *pool,
                           uint32_t *base_use_limit,
                           bf_tm_queue_baf_t *dynamic_baf,
                           uint32_t *hyst_cells));
  MOCK_METHOD7(bfTMQueueAppPoolSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           bf_tm_app_pool_t pool,
                           uint32_t base_use_limit,
                           bf_tm_queue_baf_t dynamic_baf,
                           uint32_t hyst_cells));

  MOCK_METHOD4(bfTMQueueAppPoolLimitGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           uint32_t *cells));

  MOCK_METHOD3(bfTMQueueAppPoolDisable,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue));

  MOCK_METHOD2(bfTMQueueCfgDefaultsGet,
               bf_status_t(bf_dev_id_t dev, uint8_t *pfc_cos));

  MOCK_METHOD6(bfTMQueueColorDefaultsGet,
               bf_status_t(bf_dev_id_t dev,
                           bool *drop_enable,
                           bf_tm_queue_color_limit_t *yellow_drop_limit,
                           bf_tm_thres_t *hyst_yellow,
                           bf_tm_queue_color_limit_t *red_drop_limit,
                           bf_tm_thres_t *hyst_red));

  MOCK_METHOD7(bfTMQueueBufferDefaultsGet,
               bf_status_t(bf_dev_id_t dev,
                           uint32_t *guaranteed_cells,
                           uint32_t *hysteresis_cells,
                           bool *tail_drop_enable,
                           bf_tm_app_pool_t *pool_id,
                           uint32_t *pool_max_cells,
                           bf_tm_queue_baf_t *dynamic_baf));
  //--- Queue Scheduling
  MOCK_METHOD4(bfTMQueueSchedEnableGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           bool *enable));
  MOCK_METHOD4(bfTMQueueSchedEnableDefaultGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           bool *enable));
  MOCK_METHOD3(bfTMQueueSchedEnable,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue));
  MOCK_METHOD3(bfTMQueueSchedDisable,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue));

  MOCK_METHOD4(bfTMQueueSchedSpeedGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           bf_port_speeds_t *speed));

  MOCK_METHOD4(bfTMQueueSchedGuaranteedEnableGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           bool *enable));
  MOCK_METHOD5(bfTMQueueSchedGuaranteedEnableDefaultsGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           bool *enable,
                           bf_tm_sched_prio_t *priority));
  MOCK_METHOD3(bfTMQueueSchedGuaranteedEnable,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue));
  MOCK_METHOD3(bfTMQueueSchedGuaranteedDisable,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue));
  MOCK_METHOD4(bfTMQueueSchedGuaranteedPriorityGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           bf_tm_sched_prio_t *priority));
  MOCK_METHOD4(bfTMQueueSchedGuaranteedPrioritySet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           bf_tm_sched_prio_t priority));
  MOCK_METHOD4(bfTMQueueSchedShapingEnableGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           bool *enable));
  MOCK_METHOD5(bfTMQueueSchedShapingEnableDefaultsGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           bool *enable,
                           bf_tm_sched_prio_t *priority));
  MOCK_METHOD3(bfTMQueueSchedShapingEnable,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue));
  MOCK_METHOD3(bfTMQueueSchedShapingDisable,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue));
  MOCK_METHOD4(bfTMQueueSchedRemainingBwPriorityGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           bf_tm_sched_prio_t *priority));
  MOCK_METHOD4(bfTMQueueSchedRemainingBwPrioritySet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           bf_tm_sched_prio_t priority));

  MOCK_METHOD4(bfTMQueueSchedDwrrWeightGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           uint16_t *weight));
  MOCK_METHOD4(bfTMQueueSchedDwrrWeightDefaultGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           uint16_t *weight));
  MOCK_METHOD4(bfTMQueueSchedDwrrWeightSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           uint16_t weight));

  MOCK_METHOD4(bfTMQueueSchedAdvFcModeGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           bf_tm_sched_adv_fc_mode_t *mode));
  MOCK_METHOD4(bfTMQueueSchedAdvFcModeDefaultGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           bf_tm_sched_adv_fc_mode_t *mode));
  MOCK_METHOD4(bfTMQueueSchedAdvFcModeSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           bf_tm_sched_adv_fc_mode_t mode));

  MOCK_METHOD7(bfTMQueueSchedMaxRateDefaultsGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           bool *pps,
                           uint32_t *burst_size,
                           uint32_t *rate,
                           bf_tm_sched_shaper_prov_type_t *prov_type));
  MOCK_METHOD7(bfTMQueueSchedMaxRateGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           bool *pps,
                           uint32_t *burst_size,
                           uint32_t *rate,
                           bf_tm_sched_shaper_prov_type_t *prov_type));
  MOCK_METHOD7(bfTMQueueSchedMaxRateSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           bool pps,
                           uint32_t burst_size,
                           uint32_t rate,
                           bf_tm_sched_shaper_prov_type_t prov_type));
  MOCK_METHOD7(bfTMQueueSchedMinRateDefaultsGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           bool *pps,
                           uint32_t *burst_size,
                           uint32_t *rate,
                           bf_tm_sched_shaper_prov_type_t *prov_type));
  MOCK_METHOD6(bfTMQueueSchedMinRateGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           bool *pps,
                           uint32_t *burst_size,
                           uint32_t *rate));
  MOCK_METHOD6(bfTMQueueSchedMinRateSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           bool pps,
                           uint32_t burst_size,
                           uint32_t rate));

  //---- TM L1 Node
  MOCK_METHOD7(bfTML1NodePortAssignmentGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           bf_tm_pg_t pg_id,
                           bf_tm_l1_node_t l1_node,
                           bool *in_use,
                           uint8_t *pg_port_nr,
                           bf_dev_port_t *port_id));

  MOCK_METHOD7(bfTML1NodePortAssignmentDefaultGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           bf_tm_pg_t pg_id,
                           bf_tm_l1_node_t l1_node,
                           bool *in_use,
                           uint8_t *pg_port_nr,
                           bf_dev_port_t *port_id));

  MOCK_METHOD6(bfTML1NodeQueueAssignmentGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           bf_tm_pg_t pg_id,
                           bf_tm_l1_node_t l1_node,
                           uint8_t *l1_queues_cnt,
                           bf_tm_queue_t *l1_queues));

  MOCK_METHOD5(bfTMPortL1NodeAssignmentGet,
               bf_status_t(bf_dev_id_t dev_id,
                           bf_dev_port_t port_id,
                           bool in_use_only,
                           uint8_t *l1_nodes_cnt,
                           bf_tm_l1_node_t *l1_nodes));

  MOCK_METHOD4(bfTMQueueL1NodeSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           bf_tm_l1_node_t l1_node));

  MOCK_METHOD4(bfTMQueueL1NodeGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           bf_tm_l1_node_t *l1_node));

  MOCK_METHOD4(bfTMQueueL1NodeDefaultGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           bf_tm_l1_node_t *l1_node));

  MOCK_METHOD3(bfTML1NodeFree,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_l1_node_t l1_node));

  MOCK_METHOD4(bfTML1NodeSchedEnableGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_l1_node_t l1_node,
                           bool *enable));

  MOCK_METHOD3(bfTML1NodeSchedEnable,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_l1_node_t l1_node));

  MOCK_METHOD3(bfTML1NodeSchedDisable,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_l1_node_t l1_node));
  //---
  MOCK_METHOD4(bfTML1NodeSchedMinRateEnableGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_l1_node_t l1_node,
                           bool *enable));

  MOCK_METHOD5(bfTML1NodeSchedMinRateEnableDefaultsGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_l1_node_t l1_node,
                           bool *enable,
                           bf_tm_sched_prio_t *priority));

  MOCK_METHOD3(bfTML1NodeSchedMinRateEnable,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_l1_node_t l1_node));

  MOCK_METHOD3(bfTML1NodeSchedMinRateDisable,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_l1_node_t l1_node));

  MOCK_METHOD4(bfTML1NodeSchedMinRatePriorityGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_l1_node_t l1_node,
                           bf_tm_sched_prio_t *priority));

  MOCK_METHOD4(bfTML1NodeSchedMinRatePrioritySet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_l1_node_t l1_node,
                           bf_tm_sched_prio_t priority));

  //---
  MOCK_METHOD4(bfTML1NodeSchedMaxRateEnableGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_l1_node_t l1_node,
                           bool *enable));

  MOCK_METHOD5(bfTML1NodeSchedMaxRateEnableDefaultsGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_l1_node_t l1_node,
                           bool *enable,
                           bf_tm_sched_prio_t *priority));

  MOCK_METHOD3(bfTML1NodeSchedMaxRateEnable,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_l1_node_t l1_node));

  MOCK_METHOD3(bfTML1NodeSchedMaxRateDisable,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_l1_node_t l1_node));

  MOCK_METHOD4(bfTML1NodeSchedMaxRatePriorityGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_l1_node_t l1_node,
                           bf_tm_sched_prio_t *priority));

  MOCK_METHOD4(bfTML1NodeSchedMaxRatePrioritySet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_l1_node_t l1_node,
                           bf_tm_sched_prio_t priority));

  MOCK_METHOD4(bfTML1NodeSchedDwrrWeightGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_l1_node_t l1_node,
                           uint16_t *weight));

  MOCK_METHOD4(bfTML1NodeSchedDwrrWeightDefaultGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_l1_node_t l1_node,
                           uint16_t *weight));

  MOCK_METHOD4(bfTML1NodeSchedDwrrWeightSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_l1_node_t l1_node,
                           uint16_t weight));
  //---
  MOCK_METHOD4(bfTML1NodeSchedPriorityPropagationGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_l1_node_t l1_node,
                           bool *is_enabled));

  MOCK_METHOD4(bfTML1NodeSchedPriorityPropagationDefaultGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_l1_node_t l1_node,
                           bool *is_enabled));

  MOCK_METHOD3(bfTML1NodeSchedPriorityPropagationEnable,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_l1_node_t l1_node));

  MOCK_METHOD3(bfTML1NodeSchedPriorityPropagationDisable,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_l1_node_t l1_node));

  //---
  MOCK_METHOD6(bfTML1NodeSchedMaxRateDefaultsGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_l1_node_t l1_node,
                           bool *pps,
                           uint32_t *burst_size,
                           uint32_t *rate));

  MOCK_METHOD6(bfTML1NodeSchedMaxRateGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_l1_node_t l1_node,
                           bool *pps,
                           uint32_t *burst_size,
                           uint32_t *rate));

  MOCK_METHOD6(bfTML1NodeSchedMaxRateSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_l1_node_t l1_node,
                           bool pps,
                           uint32_t burst_size,
                           uint32_t rate));

  //---
  MOCK_METHOD6(bfTML1NodeSchedMinRateDefaultsGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_l1_node_t l1_node,
                           bool *pps,
                           uint32_t *burst_size,
                           uint32_t *rate));

  MOCK_METHOD6(bfTML1NodeSchedMinRateGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_l1_node_t l1_node,
                           bool *pps,
                           uint32_t *burst_size,
                           uint32_t *rate));

  MOCK_METHOD6(bfTML1NodeSchedMinRateSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_l1_node_t l1_node,
                           bool pps,
                           uint32_t burst_size,
                           uint32_t rate));
  /* Pool Config APIs */
  MOCK_METHOD3(bfTmAppPoolSizeSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_tm_app_pool_t pool,
                           uint32_t cells));
  MOCK_METHOD3(bfTmAppPoolSizeGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_tm_app_pool_t pool,
                           uint32_t *cells));

  MOCK_METHOD2(bfTmPoolSkidSizeSet,
               bf_status_t(bf_dev_id_t dev, uint32_t cells));

  MOCK_METHOD2(bfTmPoolMirrorOnDropSizeSet,
               bf_status_t(bf_dev_id_t dev, uint32_t cells));

  MOCK_METHOD4(bfTmPreFifoLimitSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           uint8_t fifo,
                           uint32_t cells));

  MOCK_METHOD2(bfTmGlobalMinLimitSet,
               bf_status_t(bf_dev_id_t dev, uint32_t cells));

  MOCK_METHOD2(bfTmPoolUcCutThroughSizeSet,
               bf_status_t(bf_dev_id_t dev, uint32_t cells));

  MOCK_METHOD2(bfTmPoolMcCutThroughSizeSet,
               bf_status_t(bf_dev_id_t dev, uint32_t cells));

  MOCK_METHOD2(bfTmPoolSkidSizeGet,
               bf_status_t(bf_dev_id_t dev, uint32_t *cells));

  MOCK_METHOD2(bfTmPoolMirrorOnDropSizeGet,
               bf_status_t(bf_dev_id_t dev, uint32_t *cells));

  MOCK_METHOD4(bfTmPreFifoLimitGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           uint8_t fifo,
                           uint32_t *cells));

  MOCK_METHOD2(bfTmGlobalMinLimitGet,
               bf_status_t(bf_dev_id_t dev, uint32_t *cells));

  MOCK_METHOD2(bfTmPoolUcCutThroughSizeGet,
               bf_status_t(bf_dev_id_t dev, uint32_t *cells));

  MOCK_METHOD2(bfTmPoolMcCutThroughSizeGet,
               bf_status_t(bf_dev_id_t dev, uint32_t *cells));

  /* Pool Config APIs */
  MOCK_METHOD2(bfTmSkidPoolHysteresisSet,
               bf_status_t(bf_dev_id_t dev, uint32_t cells));
  MOCK_METHOD2(bfTmSkidPoolHysteresisGet,
               bf_status_t(bf_dev_id_t dev, uint32_t *cells));
  MOCK_METHOD2(bfTmSkidPoolHysteresisGetDefault,
               bf_status_t(bf_dev_id_t dev, uint32_t *cells));

  /* App Pool Config APIs */
  MOCK_METHOD2(bfTmPoolColorDropEnable,
               bf_status_t(bf_dev_id_t dev, bf_tm_app_pool_t pool));
  MOCK_METHOD2(bfTmPoolColorDropDisable,
               bf_status_t(bf_dev_id_t dev, bf_tm_app_pool_t pool));
  MOCK_METHOD4(bfTmAppPoolColorDropLimitSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_tm_app_pool_t pool,
                           bf_tm_color_t color,
                           uint32_t limit));
  MOCK_METHOD4(bfTmPoolColorDropLimitGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_tm_app_pool_t pool,
                           bf_tm_color_t color,
                           uint32_t *limit));
  MOCK_METHOD3(bfTmPoolColorDropStateGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_tm_app_pool_t pool,
                           bool *drop_state));

  /***** Pool color APIs *******/
  MOCK_METHOD3(bfTmPoolColorDropHysteresisSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_tm_color_t color,
                           uint32_t limit));

  MOCK_METHOD3(bfTmPoolColorDropHysteresisGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_tm_color_t color,
                           uint32_t *limit));

  MOCK_METHOD3(bfTmPoolColorDropHysteresisDefaultGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_tm_color_t color,
                           uint32_t *limit));

  /***** App Pool PFC APIs *******/
  MOCK_METHOD4(bfTmPoolPfcLimitSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_tm_app_pool_t pool,
                           bf_tm_icos_t icos,
                           uint32_t limit));

  MOCK_METHOD4(bfTmPoolPfcLimitGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_tm_app_pool_t pool,
                           bf_tm_icos_t icos,
                           uint32_t *limit));

  MOCK_METHOD4(bfTmPoolPfcLimitGetDefault,
               bf_status_t(bf_dev_id_t dev,
                           bf_tm_app_pool_t pool,
                           bf_tm_icos_t icos,
                           uint32_t *limit));

  MOCK_METHOD2(bfTmMaxPfcLevelsGet,
               bf_status_t(bf_dev_id_t dev, uint32_t *levels));

  /***** Port counter APIs *******/
  MOCK_METHOD5(bfTmPortDropGetCached,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           bf_dev_port_t port,
                           uint64_t *ig_count,
                           uint64_t *eg_count));

  MOCK_METHOD7(bfTmPortUsageGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           bf_dev_port_t port,
                           uint32_t *ig_count,
                           uint32_t *eg_count,
                           uint32_t *ig_wm,
                           uint32_t *eg_wm));

  MOCK_METHOD2(bfTmPortIngressWatermarkClear,
               bf_status_t(bf_dev_id_t dev, bf_dev_port_t port));

  MOCK_METHOD2(bfTmPortEgressWatermarkClear,
               bf_status_t(bf_dev_id_t dev, bf_dev_port_t port));

  MOCK_METHOD3(bfTmPortDropIngressCacheSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           uint64_t drop_count));

  MOCK_METHOD3(bfTmPortDropEgressCacheSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           uint64_t drop_count));

  /***** Queue counter APIs *******/
  MOCK_METHOD5(bfTmQDropGetCached,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           uint64_t *count));

  MOCK_METHOD5(bfTmQDropCacheSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           uint64_t count));

  MOCK_METHOD6(bfTmQUsageGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue,
                           uint32_t *count,
                           uint32_t *wm));

  MOCK_METHOD3(bfTmQWatermarkClear,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_tm_queue_t queue));

  /***** Pool counter APIs *******/
  MOCK_METHOD4(bfTmPoolUsageGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_tm_app_pool_t pool,
                           uint32_t *count,
                           uint32_t *wm));

  MOCK_METHOD2(bfTmPoolWatermarkClear,
               bf_status_t(bf_dev_id_t dev, bf_tm_app_pool_t pool));

  /***** Pipe counter APIs *******/
  MOCK_METHOD3(bfTmPipeBufferFullDropGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           uint64_t *count));

  MOCK_METHOD4(bfTmQDiscardUsageGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           uint32_t *count,
                           uint32_t *wm));

  MOCK_METHOD4(bfTmPipeCountersGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           uint64_t *cell_count,
                           uint64_t *pkt_count));

  MOCK_METHOD4(bfTmCutThroughCountersGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           uint64_t *uc_count,
                           uint64_t *mc_count));

  MOCK_METHOD3(bfTmBlklvlDropGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           bf_tm_blklvl_cntrs_t *blk_cntrs));

  MOCK_METHOD2(bfTmPreFifoDropGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_tm_pre_fifo_cntrs_t *fifo_cntrs));

  MOCK_METHOD2(bfTmPipeBufferFullDropClear,
               bf_status_t(bf_dev_id_t dev, bf_dev_pipe_t pipe));

  MOCK_METHOD2(bfTmQDiscardWatermarkClear,
               bf_status_t(bf_dev_id_t dev, bf_dev_pipe_t pipe));

  MOCK_METHOD2(bfTmPipeClearCellCounter,
               bf_status_t(bf_dev_id_t dev, bf_dev_pipe_t pipe));

  MOCK_METHOD2(bfTmPipeClearPacketCounter,
               bf_status_t(bf_dev_id_t dev, bf_dev_pipe_t pipe));

  MOCK_METHOD2(bfTmPipeClearUcCtPacketCounter,
               bf_status_t(bf_dev_id_t dev, bf_dev_pipe_t pipe));

  MOCK_METHOD2(bfTmPipeClearMcCtPacketCounter,
               bf_status_t(bf_dev_id_t dev, bf_dev_pipe_t pipe));

  MOCK_METHOD3(bfTmBlklvlDropClear,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           uint32_t clear_mask));

  MOCK_METHOD3(bfTmPreFifoDropClear,
               bf_status_t(bf_dev_id_t dev, bf_dev_pipe_t pipe, uint32_t fifo));

  /***** PPG Counter APIs *******/
  MOCK_METHOD4(bfTmPpgDropGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           bf_tm_ppg_hdl ppg,
                           uint64_t *count));

  MOCK_METHOD4(bfTmPpgDropGetCached,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           bf_tm_ppg_hdl ppg,
                           uint64_t *count));

  MOCK_METHOD4(bfTmPpgDropCacheSet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           bf_tm_ppg_hdl ppg,
                           uint64_t count));

  MOCK_METHOD7(bfTmPpgUsageGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           bf_tm_ppg_hdl ppg,
                           uint32_t *gmin_count,
                           uint32_t *shared_count,
                           uint32_t *skid_count,
                           uint32_t *wm));

  MOCK_METHOD2(bfTmPpgWatermarkClear,
               bf_status_t(bf_dev_id_t dev, bf_tm_ppg_hdl ppg));

  /***** TM Cfg APIs *******/
  MOCK_METHOD2(bfTmTimestampShiftSet,
               bf_status_t(bf_dev_id_t dev, uint8_t shift));

  MOCK_METHOD2(bfTmTimestampShiftGet,
               bf_status_t(bf_dev_id_t dev, uint8_t *shift));

  MOCK_METHOD2(bfTmTimestampShiftGetDefault,
               bf_status_t(bf_dev_id_t dev, uint8_t *shift));

  MOCK_METHOD2(bfTmCellSizeInBytesGet,
               bf_status_t(bf_dev_id_t dev, uint32_t *bytes));

  MOCK_METHOD2(bfTmTotalCellCountGet,
               bf_status_t(bf_dev_id_t dev, uint32_t *total_cells));

  MOCK_METHOD2(bfTmIngressBufferLimitSet,
               bf_status_t(bf_dev_id_t dev, uint32_t cells));

  MOCK_METHOD2(bfTmIngressBufferLimitGet,
               bf_status_t(bf_dev_id_t dev, uint32_t *cells));

  MOCK_METHOD2(bfTmIngressBufferLimitGetDefault,
               bf_status_t(bf_dev_id_t dev, uint32_t *cells));

  MOCK_METHOD1(bfTmIngressBufferLimitEnable, bf_status_t(bf_dev_id_t dev));

  MOCK_METHOD1(bfTmIngressBufferLimitDisable, bf_status_t(bf_dev_id_t dev));

  MOCK_METHOD2(bfTmIngressBufferLimitStateGet,
               bf_status_t(bf_dev_id_t dev, bool *state));

  MOCK_METHOD2(bfTmIngressBufferLimitStateGetDefault,
               bf_status_t(bf_dev_id_t dev, bool *state));

  /***** TM Pipe Multicast fifo APIs *******/
  MOCK_METHOD4(bfTmMcFifoIcosMappingSet,
               bf_status_t(bf_dev_id_t dev,
                           uint8_t pipe_bmap,
                           int fifo,
                           uint8_t icos_bmap));

  MOCK_METHOD4(bfTmMcFifoIcosMappingGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           int fifo,
                           uint8_t *icos_bmap));

  MOCK_METHOD4(bfTmMcFifoIcosMappingGetDefault,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           int fifo,
                           uint8_t *icos_bmap));

  MOCK_METHOD4(bfTmMcFifoArbModeSet,
               bf_status_t(bf_dev_id_t dev,
                           uint8_t pipe_bmap,
                           int fifo,
                           bool use_strict_pri));

  MOCK_METHOD4(bfTmMcFifoArbModeGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           int fifo,
                           bool *use_strict_pri));

  MOCK_METHOD4(bfTmMcFifoArbModeGetDefault,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           int fifo,
                           bool *use_strict_pri));

  MOCK_METHOD4(bfTmMcFifoWrrWeightSet,
               bf_status_t(bf_dev_id_t dev,
                           uint8_t pipe_bmap,
                           int fifo,
                           uint8_t weight));

  MOCK_METHOD4(bfTmMcFifoWrrWeightGet,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           int fifo,
                           uint8_t *weight));

  MOCK_METHOD4(bfTmMcFifoWrrWeightGetDefault,
               bf_status_t(bf_dev_id_t dev,
                           bf_dev_pipe_t pipe,
                           int fifo,
                           uint8_t *weight));

  MOCK_METHOD4(
      bfTmMcFifoDepthSet,
      bf_status_t(bf_dev_id_t dev, uint8_t pipe_bmap, int fifo, int size));

  MOCK_METHOD4(
      bfTmMcFifoDepthGet,
      bf_status_t(bf_dev_id_t dev, bf_dev_pipe_t pipe, int fifo, int *size));

  MOCK_METHOD4(
      bfTmMcFifoDepthGetDefault,
      bf_status_t(bf_dev_id_t dev, bf_dev_pipe_t pipe, int fifo, int *size));
};
}  // namespace bfrt_test
}  // namespace bfrt
#endif
