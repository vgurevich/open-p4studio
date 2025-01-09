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

#ifndef _TDI_TM_MGR_INTERFACE_MOCK_HPP
#define _TDI_TM_MGR_INTERFACE_MOCK_HPP

#include <tdi/tdi_info.hpp>
#include <tdi_tm/tdi_tm_intf.hpp>
#include "gmock/gmock.h"

namespace tdi {
namespace tdi_test {

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
               tdi_status_t(tdi_dev_id_t dev, bf_tm_dev_cfg_t *cfg));

  /*** TM PPG API. ***/
  MOCK_METHOD3(bfTMPPGAllocate,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_ppg_hdl *ppg));
  MOCK_METHOD2(bfTMPPGFree, tdi_status_t(tdi_dev_id_t dev, bf_tm_ppg_hdl ppg));
  MOCK_METHOD3(bfTMPPGDefaultPpgGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_ppg_hdl *ppg));
  MOCK_METHOD3(bfTMPPGIcosMappingSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_tm_ppg_hdl ppg,
                            uint8_t icos_bmap));
  MOCK_METHOD2(bfTMPPGLosslessTreatmentEnable,
               tdi_status_t(tdi_dev_id_t dev, bf_tm_ppg_hdl ppg));
  MOCK_METHOD2(bfTMPPGLosslessTreatmentDisable,
               tdi_status_t(tdi_dev_id_t dev, bf_tm_ppg_hdl ppg));
  MOCK_METHOD6(bfTMPPGAppPoolUsageSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_tm_ppg_hdl ppg,
                            bf_tm_app_pool_t pool,
                            uint32_t base_use_limit,
                            bf_tm_ppg_baf_t dynamic_baf,
                            uint32_t hysteresis));
  MOCK_METHOD3(bfTMPPGAppPoolUsageDisable,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_tm_app_pool_t pool,
                            bf_tm_ppg_hdl ppg));
  MOCK_METHOD3(bfTMPPGGuaranteedMinLimitSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_tm_ppg_hdl ppg,
                            uint32_t cells));
  MOCK_METHOD3(bfTMPPGSkidLimitSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_tm_ppg_hdl ppg,
                            uint32_t cells));
  MOCK_METHOD3(bfTMPPGGuaranteedMinSkidHysteresisSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_tm_ppg_hdl ppg,
                            uint32_t cells));
  MOCK_METHOD3(bfTMPPGAppPoolIdGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_tm_ppg_hdl ppg,
                            uint32_t *pool));
  MOCK_METHOD6(bfTMPPGAppPoolUsageGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_tm_ppg_hdl ppg,
                            bf_tm_app_pool_t pool,
                            uint32_t *base_use_limit,
                            bf_tm_ppg_baf_t *dynamic_baf,
                            uint32_t *hysteresis));
  MOCK_METHOD3(bfTMPPGuaranteedMinLimitGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_tm_ppg_hdl ppg,
                            uint32_t *cells));
  MOCK_METHOD3(bfTMPPGLosslessTreatmentGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_tm_ppg_hdl ppg,
                            bool *pfc_val));
  MOCK_METHOD3(bfTMPPGSkidLimitGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_tm_ppg_hdl ppg,
                            uint32_t *cells));
  MOCK_METHOD3(bfTMPPGGuaranteedMinSkidHysteresisGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_tm_ppg_hdl ppg,
                            uint32_t *cells));
  MOCK_METHOD3(bfTMPPGIcosMappingGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_tm_ppg_hdl ppg,
                            uint8_t *icos_bmap));

  MOCK_METHOD3(bfTMPPGTotalCntGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            uint32_t *total_cnt));

  MOCK_METHOD3(bfTMPPGUnusedCntGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            uint32_t *unused_cnt));

  MOCK_METHOD3(bfTMPPGPortGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_tm_ppg_hdl ppg_hdl,
                            bf_dev_port_t *port_id));

  MOCK_METHOD3(bfTMPPGNrGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_tm_ppg_hdl ppg_hdl,
                            bf_tm_ppg_id_t *ppg_nr));

  MOCK_METHOD3(bfTMPPGMirrorPortHandleGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            bf_tm_ppg_hdl *ppg_hdl));

  MOCK_METHOD4(bfTMPPGBufferGetDefault,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_tm_ppg_hdl ppg_hdl,
                            uint32_t *min_limit_cells,
                            uint32_t *hysteresis_cells));

  MOCK_METHOD5(bfTMPPGAppPoolUsageGetDefault,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_tm_ppg_hdl ppg_hdl,
                            bf_tm_app_pool_t *pool,
                            uint32_t *pool_max_cells,
                            bf_tm_ppg_baf_t *dynamic_baf));

  /****** TM Pipe API *****/
  MOCK_METHOD2(bfTMPipeIsValid,
               tdi_status_t(tdi_dev_id_t dev, bf_dev_pipe_t pipe));
  MOCK_METHOD2(bfTMPipeGetCount,
               tdi_status_t(tdi_dev_id_t dev, uint8_t *count));
  MOCK_METHOD2(bfTMPipeGetFirst,
               tdi_status_t(tdi_dev_id_t dev, bf_dev_pipe_t *pipe));
  MOCK_METHOD3(bfTMPipeGetNext,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            bf_dev_pipe_t *pipe_next));

  MOCK_METHOD3(bfTMPipeGetPortCount,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            uint16_t *count));
  MOCK_METHOD3(bfTMPipeGetPortFirst,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            bf_dev_port_t *dev_port));
  MOCK_METHOD3(bfTMPipeGetPortNext,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t dev_port,
                            bf_dev_port_t *dev_port_next));

  MOCK_METHOD4(bfTMPipeGetPortGroupBasePort,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            bf_tm_pg_t pg_id,
                            bf_dev_port_t *dev_port));

  MOCK_METHOD4(bfTMPipeMirrorOnDropDestGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            bf_dev_port_t *port,
                            bf_tm_queue_t *queue));
  MOCK_METHOD4(bfTMPipeMirrorOnDropDestDefaultGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            bf_dev_port_t *port,
                            bf_tm_queue_t *queue));
  MOCK_METHOD4(bfTMPipeMirrorOnDropDestSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue));
  MOCK_METHOD3(bfTMPipeSchedPktIfgCompDefaultGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            uint8_t *adjustment));
  MOCK_METHOD3(bfTMPipeSchedPktIfgCompGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            uint8_t *adjustment));
  MOCK_METHOD3(bfTMPipeSchedPktIfgCompSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            uint8_t adjustment));
  MOCK_METHOD3(bfTMPipeSchedAdvFcModeDefaultGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            bool *enable));
  MOCK_METHOD3(bfTMPipeSchedAdvFcModeGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            bool *enable));
  MOCK_METHOD3(bfTMPipeSchedAdvFcModeSet,
               tdi_status_t(tdi_dev_id_t dev, bf_dev_pipe_t pipe, bool enable));
  MOCK_METHOD3(bfTMPipeQstatReportDefaultGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            bool *enable_any));
  MOCK_METHOD3(bfTMPipeQstatReportGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            bool *enable_any));
  MOCK_METHOD3(bfTMPipeQstatReportSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            bool enable_any));
  MOCK_METHOD3(bfTMPipeEgHysteresisDefaultGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            uint32_t *cells));
  MOCK_METHOD3(bfTMPipeEgHysteresisGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            uint32_t *cells));
  MOCK_METHOD3(bfTMPipeEgHysteresisSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            uint32_t cells));
  MOCK_METHOD3(bfTMPipeEgLimitDefaultGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            uint32_t *cells));
  MOCK_METHOD3(bfTMPipeEgLimitGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            uint32_t *cells));
  MOCK_METHOD3(bfTMPipeEgLimitSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            uint32_t cells));

  /****** TM Port API *****/
  MOCK_METHOD2(bfTMPortIsValid,
               tdi_status_t(tdi_dev_id_t dev, bf_dev_port_t port_id));

  MOCK_METHOD7(bfTMPortStatusGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port_id,
                            bool *is_offline,
                            bool *is_enabled,
                            bool *qac_rx_enable,
                            bool *recirc_enable,
                            bool *has_mac));

  MOCK_METHOD3(bfTMPortIcosCntGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port_id,
                            uint8_t *icos_count));

  MOCK_METHOD3(bfTMPortIcosMapDefaultsGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            uint8_t *icos_mask));

  MOCK_METHOD4(bfTMPortPPGMapGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            uint8_t *ppg_mask,
                            bf_tm_ppg_hdl *ppg_hdlrs));

  MOCK_METHOD5(bfTMPortBaseQueueGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port_id,
                            bf_tm_pg_t *pg_id,
                            uint8_t *pg_queue,
                            bool *is_mapped));

  MOCK_METHOD4(bfTMPortQMappingGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            uint8_t *q_count,
                            uint8_t *q_mapping));
  MOCK_METHOD4(bfTMPortQMappingSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            uint8_t q_count,
                            uint8_t *q_mapping));
  //---
  MOCK_METHOD3(bfTMPortCreditsGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            uint32_t *pkt_credits));
  //---
  MOCK_METHOD7(bfTMPortBufferDefaultsGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bool *ct_enable,
                            uint8_t *uc_ct_limit_cells,
                            uint32_t *ig_limit_cells,
                            uint32_t *ig_hysteresis_cells,
                            uint32_t *skid_limit_cells));

  MOCK_METHOD3(bfTMPortCutThroughGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bool *ct_enablei));

  MOCK_METHOD3(bfTMPortCutThroughLimitGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            uint8_t *uc_ct_limit_cells));

  MOCK_METHOD3(bfTMPortBufIgLimitGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            uint32_t *ig_limit_cells));

  MOCK_METHOD3(bfTMPortBufIgHysteresisGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            uint32_t *ig_hysteresis_cells));

  MOCK_METHOD3(bfTMPortBufSkidLimitGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            uint32_t *skid_limit_cells));

  MOCK_METHOD3(bfTMPortCutThroughLimitSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            uint8_t uc_ct_limit_cells));

  MOCK_METHOD3(bfTMPortBufIgLimitSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            uint32_t ig_limit_cells));

  MOCK_METHOD3(bfTMPortBufIgHysteresisSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            uint32_t ig_hysteresis_cells));

  MOCK_METHOD3(bfTMPortBufSkidLimitSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            uint32_t skid_limit_cells));
  //---
  MOCK_METHOD5(bfTMPortFlowCtrlDefaultsGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_flow_ctrl_type_t *mode_tx,
                            bf_tm_flow_ctrl_type_t *mode_rx,
                            uint8_t *cos_map));

  MOCK_METHOD3(bfTMPortFlowCtrlTxGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_flow_ctrl_type_t *mode_tx));

  MOCK_METHOD3(bfTMPortFlowCtrlRxGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_flow_ctrl_type_t *mode_rx));

  MOCK_METHOD3(bfTMPortCosMappingGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            uint8_t *cos_map));

  MOCK_METHOD3(bfTMPortFlowCtrlTxSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_flow_ctrl_type_t mode_tx));

  MOCK_METHOD3(bfTMPortFlowCtrlRxSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_flow_ctrl_type_t mode_rx));

  MOCK_METHOD3(bfTMPortCosMappingSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            uint8_t *cos_map));
  //---- Port Scheduling
  MOCK_METHOD3(bfTMPortSchedEnable,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_port_speeds_t speed));
  MOCK_METHOD2(bfTMPortSchedDisable,
               tdi_status_t(tdi_dev_id_t dev, bf_dev_port_t port));
  //---
  MOCK_METHOD3(bfTMPortSchedSpeedGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_port_speeds_t *speed));
  MOCK_METHOD3(bfTMPortSchedSpeedResetGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_port_speeds_t *speed));
  //---
  MOCK_METHOD3(bfTMPortSchedShapingEnableDefaultGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bool *enable));
  MOCK_METHOD3(bfTMPortSchedShapingEnableGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bool *enable));
  MOCK_METHOD2(bfTMPortSchedShapingEnable,
               tdi_status_t(tdi_dev_id_t dev, bf_dev_port_t port));
  MOCK_METHOD2(bfTMPortSchedShapingDisable,
               tdi_status_t(tdi_dev_id_t dev, bf_dev_port_t port));
  //---
  MOCK_METHOD6(bfTMPortSchedMaxRateDefaultsGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bool *pps,
                            uint32_t *burst_size,
                            uint32_t *rate,
                            bf_tm_sched_shaper_prov_type_t *prov_type));
  MOCK_METHOD6(bfTMPortSchedMaxRateGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bool *pps,
                            uint32_t *burst_size,
                            uint32_t *rate,
                            bf_tm_sched_shaper_prov_type_t *prov_type));
  MOCK_METHOD6(bfTMPortSchedMaxRateSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bool pps,
                            uint32_t burst_size,
                            uint32_t rate,
                            bf_tm_sched_shaper_prov_type_t prov_type));
  /****** TM Port Group API ******/
  MOCK_METHOD4(bfTMPortGroupGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port_id,
                            bf_tm_pg_t *pg_id,
                            uint8_t *pg_port_nr));

  MOCK_METHOD6(bfTMPortGroupPortQueueGet,
               tdi_status_t(const tdi_target_t &dev_tgt,
                            bf_tm_pg_t pg_id,
                            uint8_t pg_queue,
                            bf_dev_port_t *port,
                            bf_tm_queue_t *queue_nr,
                            bool *is_mapped));

  /****** TM Queue API *****/
  MOCK_METHOD4(bfTMQueuePfcCosGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            uint8_t *cos));
  MOCK_METHOD4(bfTMQueuePfcCosSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            uint8_t cos));

  MOCK_METHOD4(bfTMQueueColorDropGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            bool *is_enabled));
  MOCK_METHOD4(bfTMQueueColorDropSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            bool enable));

  MOCK_METHOD4(bfTMQueueColorDropVisibleGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            bool *is_enabled));
  MOCK_METHOD4(bfTMQueueColorDropVisibleSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            bool enable));

  MOCK_METHOD5(bfTMQueueColorDropLimitGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            bf_tm_color_t color,
                            bf_tm_queue_color_limit_t *limit));
  MOCK_METHOD5(bfTMQueueColorDropLimitSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            bf_tm_color_t color,
                            bf_tm_queue_color_limit_t limit));

  MOCK_METHOD5(bfTMQueueColorHysteresisGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            bf_tm_color_t color,
                            bf_tm_thres_t *hyst_cells));
  MOCK_METHOD5(bfTMQueueColorHysteresisSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            bf_tm_color_t color,
                            bf_tm_thres_t hyst_cells));

  MOCK_METHOD4(bfTMQueueGuaranteedCellsGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            uint32_t *cells));
  MOCK_METHOD4(bfTMQueueGuaranteedCellsSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            uint32_t cells));

  MOCK_METHOD4(bfTMQueueTailDropGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            bool *is_enabled));
  MOCK_METHOD4(bfTMQueueTailDropSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            bool enable));

  MOCK_METHOD4(bfTMQueueHysteresisGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            uint32_t *hyst_cells));
  MOCK_METHOD4(bfTMQueueHysteresisSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            uint32_t cells));

  MOCK_METHOD7(bfTMQueueAppPoolGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            bf_tm_app_pool_t *pool,
                            uint32_t *base_use_limit,
                            bf_tm_queue_baf_t *dynamic_baf,
                            uint32_t *hyst_cells));
  MOCK_METHOD7(bfTMQueueAppPoolSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            bf_tm_app_pool_t pool,
                            uint32_t base_use_limit,
                            bf_tm_queue_baf_t dynamic_baf,
                            uint32_t hyst_cells));

  MOCK_METHOD4(bfTMQueueAppPoolLimitGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            uint32_t *cells));

  MOCK_METHOD3(bfTMQueueAppPoolDisable,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue));

  MOCK_METHOD2(bfTMQueueCfgDefaultsGet,
               tdi_status_t(tdi_dev_id_t dev, uint8_t *pfc_cos));

  MOCK_METHOD6(bfTMQueueColorDefaultsGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bool *drop_enable,
                            bf_tm_queue_color_limit_t *yellow_drop_limit,
                            bf_tm_thres_t *hyst_yellow,
                            bf_tm_queue_color_limit_t *red_drop_limit,
                            bf_tm_thres_t *hyst_red));

  MOCK_METHOD2(bfTMQueueColorDefaultsExtGet,
               tdi_status_t(tdi_dev_id_t dev, bool *drop_visible));

  MOCK_METHOD7(bfTMQueueBufferDefaultsGet,
               tdi_status_t(tdi_dev_id_t dev,
                            uint32_t *guaranteed_cells,
                            uint32_t *hysteresis_cells,
                            bool *tail_drop_enable,
                            bf_tm_app_pool_t *pool_id,
                            uint32_t *pool_max_cells,
                            bf_tm_queue_baf_t *dynamic_baf));
  //--- Queue Scheduling
  MOCK_METHOD4(bfTMQueueSchedEnableGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            bool *enable));
  MOCK_METHOD4(bfTMQueueSchedEnableDefaultGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            bool *enable));
  MOCK_METHOD3(bfTMQueueSchedEnable,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue));
  MOCK_METHOD3(bfTMQueueSchedDisable,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue));

  MOCK_METHOD4(bfTMQueueSchedGuaranteedEnableGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            bool *enable));
  MOCK_METHOD5(bfTMQueueSchedGuaranteedEnableDefaultsGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            bool *enable,
                            bf_tm_sched_prio_t *priority));
  MOCK_METHOD3(bfTMQueueSchedGuaranteedEnable,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue));
  MOCK_METHOD3(bfTMQueueSchedGuaranteedDisable,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue));
  MOCK_METHOD4(bfTMQueueSchedGuaranteedPriorityGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            bf_tm_sched_prio_t *priority));
  MOCK_METHOD4(bfTMQueueSchedGuaranteedPrioritySet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            bf_tm_sched_prio_t priority));
  MOCK_METHOD4(bfTMQueueSchedShapingEnableGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            bool *enable));
  MOCK_METHOD5(bfTMQueueSchedShapingEnableDefaultsGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            bool *enable,
                            bf_tm_sched_prio_t *priority));
  MOCK_METHOD3(bfTMQueueSchedShapingEnable,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue));
  MOCK_METHOD3(bfTMQueueSchedShapingDisable,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue));
  MOCK_METHOD4(bfTMQueueSchedRemainingBwPriorityGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            bf_tm_sched_prio_t *priority));
  MOCK_METHOD4(bfTMQueueSchedRemainingBwPrioritySet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            bf_tm_sched_prio_t priority));

  MOCK_METHOD4(bfTMQueueSchedDwrrWeightGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            uint16_t *weight));
  MOCK_METHOD4(bfTMQueueSchedDwrrWeightDefaultGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            uint16_t *weight));
  MOCK_METHOD4(bfTMQueueSchedDwrrWeightSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            uint16_t weight));

  MOCK_METHOD4(bfTMQueueSchedAdvFcModeGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            bf_tm_sched_adv_fc_mode_t *mode));
  MOCK_METHOD4(bfTMQueueSchedAdvFcModeDefaultGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            bf_tm_sched_adv_fc_mode_t *mode));
  MOCK_METHOD4(bfTMQueueSchedAdvFcModeSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            bf_tm_sched_adv_fc_mode_t mode));

  MOCK_METHOD7(bfTMQueueSchedMaxRateDefaultsGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            bool *pps,
                            uint32_t *burst_size,
                            uint32_t *rate,
                            bf_tm_sched_shaper_prov_type_t *prov_type));
  MOCK_METHOD7(bfTMQueueSchedMaxRateGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            bool *pps,
                            uint32_t *burst_size,
                            uint32_t *rate,
                            bf_tm_sched_shaper_prov_type_t *prov_type));
  MOCK_METHOD7(bfTMQueueSchedMaxRateSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            bool pps,
                            uint32_t burst_size,
                            uint32_t rate,
                            bf_tm_sched_shaper_prov_type_t prov_type));
  MOCK_METHOD7(bfTMQueueSchedMinRateDefaultsGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            bool *pps,
                            uint32_t *burst_size,
                            uint32_t *rate,
                            bf_tm_sched_shaper_prov_type_t *prov_type));
  MOCK_METHOD6(bfTMQueueSchedMinRateGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            bool *pps,
                            uint32_t *burst_size,
                            uint32_t *rate));
  MOCK_METHOD6(bfTMQueueSchedMinRateSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            bool pps,
                            uint32_t burst_size,
                            uint32_t rate));

  //---- TM L1 Node
  MOCK_METHOD7(bfTML1NodePortAssignmentGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            bf_tm_pg_t pg_id,
                            bf_tm_l1_node_t l1_node,
                            bool *in_use,
                            uint8_t *pg_port_nr,
                            bf_dev_port_t *port_id));

  MOCK_METHOD7(bfTML1NodePortAssignmentDefaultGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            bf_tm_pg_t pg_id,
                            bf_tm_l1_node_t l1_node,
                            bool *in_use,
                            uint8_t *pg_port_nr,
                            bf_dev_port_t *port_id));

  MOCK_METHOD6(bfTML1NodeQueueAssignmentGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            bf_tm_pg_t pg_id,
                            bf_tm_l1_node_t l1_node,
                            uint8_t *l1_queues_cnt,
                            bf_tm_queue_t *l1_queues));

  MOCK_METHOD5(bfTMPortL1NodeAssignmentGet,
               tdi_status_t(tdi_dev_id_t dev_id,
                            bf_dev_port_t port_id,
                            bool in_use_only,
                            uint8_t *l1_nodes_cnt,
                            bf_tm_l1_node_t *l1_nodes));

  MOCK_METHOD4(bfTMQueueL1NodeSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            bf_tm_l1_node_t l1_node));

  MOCK_METHOD4(bfTMQueueL1NodeGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            bf_tm_l1_node_t *l1_node));

  MOCK_METHOD4(bfTMQueueL1NodeDefaultGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            bf_tm_l1_node_t *l1_node));

  MOCK_METHOD3(bfTML1NodeFree,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_l1_node_t l1_node));

  MOCK_METHOD4(bfTML1NodeSchedEnableGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_l1_node_t l1_node,
                            bool *enable));

  MOCK_METHOD3(bfTML1NodeSchedEnable,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_l1_node_t l1_node));

  MOCK_METHOD3(bfTML1NodeSchedDisable,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_l1_node_t l1_node));
  //---
  MOCK_METHOD4(bfTML1NodeSchedMinRateEnableGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_l1_node_t l1_node,
                            bool *enable));

  MOCK_METHOD5(bfTML1NodeSchedMinRateEnableDefaultsGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_l1_node_t l1_node,
                            bool *enable,
                            bf_tm_sched_prio_t *priority));

  MOCK_METHOD3(bfTML1NodeSchedMinRateEnable,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_l1_node_t l1_node));

  MOCK_METHOD3(bfTML1NodeSchedMinRateDisable,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_l1_node_t l1_node));

  MOCK_METHOD4(bfTML1NodeSchedMinRatePriorityGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_l1_node_t l1_node,
                            bf_tm_sched_prio_t *priority));

  MOCK_METHOD4(bfTML1NodeSchedMinRatePrioritySet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_l1_node_t l1_node,
                            bf_tm_sched_prio_t priority));

  //---
  MOCK_METHOD4(bfTML1NodeSchedMaxRateEnableGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_l1_node_t l1_node,
                            bool *enable));

  MOCK_METHOD5(bfTML1NodeSchedMaxRateEnableDefaultsGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_l1_node_t l1_node,
                            bool *enable,
                            bf_tm_sched_prio_t *priority));

  MOCK_METHOD3(bfTML1NodeSchedMaxRateEnable,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_l1_node_t l1_node));

  MOCK_METHOD3(bfTML1NodeSchedMaxRateDisable,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_l1_node_t l1_node));

  MOCK_METHOD4(bfTML1NodeSchedMaxRatePriorityGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_l1_node_t l1_node,
                            bf_tm_sched_prio_t *priority));

  MOCK_METHOD4(bfTML1NodeSchedMaxRatePrioritySet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_l1_node_t l1_node,
                            bf_tm_sched_prio_t priority));

  MOCK_METHOD4(bfTML1NodeSchedDwrrWeightGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_l1_node_t l1_node,
                            uint16_t *weight));

  MOCK_METHOD4(bfTML1NodeSchedDwrrWeightDefaultGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_l1_node_t l1_node,
                            uint16_t *weight));

  MOCK_METHOD4(bfTML1NodeSchedDwrrWeightSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_l1_node_t l1_node,
                            uint16_t weight));
  //---
  MOCK_METHOD4(bfTML1NodeSchedPriorityPropagationGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_l1_node_t l1_node,
                            bool *is_enabled));

  MOCK_METHOD4(bfTML1NodeSchedPriorityPropagationDefaultGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_l1_node_t l1_node,
                            bool *is_enabled));

  MOCK_METHOD3(bfTML1NodeSchedPriorityPropagationEnable,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_l1_node_t l1_node));

  MOCK_METHOD3(bfTML1NodeSchedPriorityPropagationDisable,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_l1_node_t l1_node));

  //---
  MOCK_METHOD6(bfTML1NodeSchedMaxRateDefaultsGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_l1_node_t l1_node,
                            bool *pps,
                            uint32_t *burst_size,
                            uint32_t *rate));

  MOCK_METHOD6(bfTML1NodeSchedMaxRateGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_l1_node_t l1_node,
                            bool *pps,
                            uint32_t *burst_size,
                            uint32_t *rate));

  MOCK_METHOD6(bfTML1NodeSchedMaxRateSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_l1_node_t l1_node,
                            bool pps,
                            uint32_t burst_size,
                            uint32_t rate));

  //---
  MOCK_METHOD6(bfTML1NodeSchedMinRateDefaultsGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_l1_node_t l1_node,
                            bool *pps,
                            uint32_t *burst_size,
                            uint32_t *rate));

  MOCK_METHOD6(bfTML1NodeSchedMinRateGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_l1_node_t l1_node,
                            bool *pps,
                            uint32_t *burst_size,
                            uint32_t *rate));

  MOCK_METHOD6(bfTML1NodeSchedMinRateSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_l1_node_t l1_node,
                            bool pps,
                            uint32_t burst_size,
                            uint32_t rate));
  /* Pool Config APIs */
  MOCK_METHOD3(bfTmAppPoolSizeSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_tm_app_pool_t pool,
                            uint32_t cells));
  MOCK_METHOD3(bfTmAppPoolSizeGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_tm_app_pool_t pool,
                            uint32_t *cells));

  MOCK_METHOD2(bfTmPoolSkidSizeSet,
               tdi_status_t(tdi_dev_id_t dev, uint32_t cells));

  MOCK_METHOD2(bfTmPoolMirrorOnDropSizeSet,
               tdi_status_t(tdi_dev_id_t dev, uint32_t cells));

  MOCK_METHOD4(bfTmPreFifoLimitSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            uint8_t fifo,
                            uint32_t cells));

  MOCK_METHOD2(bfTmGlobalMinLimitSet,
               tdi_status_t(tdi_dev_id_t dev, uint32_t cells));

  MOCK_METHOD2(bfTmPoolUcCutThroughSizeSet,
               tdi_status_t(tdi_dev_id_t dev, uint32_t cells));

  MOCK_METHOD2(bfTmPoolMcCutThroughSizeSet,
               tdi_status_t(tdi_dev_id_t dev, uint32_t cells));

  MOCK_METHOD2(bfTmPoolSkidSizeGet,
               tdi_status_t(tdi_dev_id_t dev, uint32_t *cells));

  MOCK_METHOD2(bfTmPoolMirrorOnDropSizeGet,
               tdi_status_t(tdi_dev_id_t dev, uint32_t *cells));

  MOCK_METHOD4(bfTmPreFifoLimitGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            uint8_t fifo,
                            uint32_t *cells));

  MOCK_METHOD2(bfTmGlobalMinLimitGet,
               tdi_status_t(tdi_dev_id_t dev, uint32_t *cells));

  MOCK_METHOD2(bfTmPoolUcCutThroughSizeGet,
               tdi_status_t(tdi_dev_id_t dev, uint32_t *cells));

  MOCK_METHOD2(bfTmPoolMcCutThroughSizeGet,
               tdi_status_t(tdi_dev_id_t dev, uint32_t *cells));

  /* Pool Config APIs */
  MOCK_METHOD2(bfTmSkidPoolHysteresisSet,
               tdi_status_t(tdi_dev_id_t dev, uint32_t cells));
  MOCK_METHOD2(bfTmSkidPoolHysteresisGet,
               tdi_status_t(tdi_dev_id_t dev, uint32_t *cells));
  MOCK_METHOD2(bfTmSkidPoolHysteresisGetDefault,
               tdi_status_t(tdi_dev_id_t dev, uint32_t *cells));

  /* App Pool Config APIs */
  MOCK_METHOD2(bfTmPoolColorDropEnable,
               tdi_status_t(tdi_dev_id_t dev, bf_tm_app_pool_t pool));
  MOCK_METHOD2(bfTmPoolColorDropDisable,
               tdi_status_t(tdi_dev_id_t dev, bf_tm_app_pool_t pool));
  MOCK_METHOD4(bfTmAppPoolColorDropLimitSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_tm_app_pool_t pool,
                            bf_tm_color_t color,
                            uint32_t limit));
  MOCK_METHOD4(bfTmPoolColorDropLimitGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_tm_app_pool_t pool,
                            bf_tm_color_t color,
                            uint32_t *limit));
  MOCK_METHOD3(bfTmPoolColorDropStateGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_tm_app_pool_t pool,
                            bool *drop_state));

  /***** Pool color APIs *******/
  MOCK_METHOD3(bfTmPoolColorDropHysteresisSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_tm_color_t color,
                            uint32_t limit));

  MOCK_METHOD3(bfTmPoolColorDropHysteresisGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_tm_color_t color,
                            uint32_t *limit));

  MOCK_METHOD3(bfTmPoolColorDropHysteresisDefaultGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_tm_color_t color,
                            uint32_t *limit));

  /***** App Pool PFC APIs *******/
  MOCK_METHOD4(bfTmPoolPfcLimitSet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_tm_app_pool_t pool,
                            bf_tm_icos_t icos,
                            uint32_t limit));

  MOCK_METHOD4(bfTmPoolPfcLimitGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_tm_app_pool_t pool,
                            bf_tm_icos_t icos,
                            uint32_t *limit));

  MOCK_METHOD4(bfTmPoolPfcLimitGetDefault,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_tm_app_pool_t pool,
                            bf_tm_icos_t icos,
                            uint32_t *limit));

  MOCK_METHOD2(bfTmMaxPfcLevelsGet,
               tdi_status_t(tdi_dev_id_t dev, uint32_t *levels));

  /***** Port counter APIs *******/
  MOCK_METHOD5(bfTmPortDropGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            bf_dev_port_t port,
                            uint64_t *ig_count,
                            uint64_t *eg_count));

  MOCK_METHOD5(bfTmPortDropGetCached,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            bf_dev_port_t port,
                            uint64_t *ig_count,
                            uint64_t *eg_count));

  MOCK_METHOD7(bfTmPortUsageGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            bf_dev_port_t port,
                            uint32_t *ig_count,
                            uint32_t *eg_count,
                            uint32_t *ig_wm,
                            uint32_t *eg_wm));

  MOCK_METHOD2(bfTmPortIngressDropCountClear,
               tdi_status_t(tdi_dev_id_t dev, bf_dev_port_t port));

  MOCK_METHOD2(bfTmPortEgressDropCountClear,
               tdi_status_t(tdi_dev_id_t dev, bf_dev_port_t port));

  MOCK_METHOD2(bfTmPortIngressWatermarkClear,
               tdi_status_t(tdi_dev_id_t dev, bf_dev_port_t port));

  MOCK_METHOD2(bfTmPortEgressWatermarkClear,
               tdi_status_t(tdi_dev_id_t dev, bf_dev_port_t port));

  /***** Queue counter APIs *******/
  MOCK_METHOD5(bfTmQDropGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            uint64_t *count));

  MOCK_METHOD5(bfTmQDropGetCached,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            uint64_t *count));

  MOCK_METHOD6(bfTmQUsageGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue,
                            uint32_t *count,
                            uint32_t *wm));

  MOCK_METHOD3(bfTmQDropCountClear,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue));

  MOCK_METHOD3(bfTmQWatermarkClear,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_port_t port,
                            bf_tm_queue_t queue));

  /***** Pool counter APIs *******/
  MOCK_METHOD4(bfTmPoolUsageGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_tm_app_pool_t pool,
                            uint32_t *count,
                            uint32_t *wm));

  MOCK_METHOD2(bfTmPoolWatermarkClear,
               tdi_status_t(tdi_dev_id_t dev, bf_tm_app_pool_t pool));

  /***** Pipe counter APIs *******/
  MOCK_METHOD3(bfTmPipeBufferFullDropGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            uint64_t *count));

  MOCK_METHOD4(bfTmQDiscardUsageGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            uint32_t *count,
                            uint32_t *wm));

  MOCK_METHOD4(bfTmPipeCountersGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            uint64_t *cell_count,
                            uint64_t *pkt_count));

  MOCK_METHOD4(bfTmCutThroughCountersGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            uint64_t *uc_count,
                            uint64_t *mc_count));

  MOCK_METHOD3(bfTmBlklvlDropGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            bf_tm_blklvl_cntrs_t *blk_cntrs));

  MOCK_METHOD2(bfTmPreFifoDropGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_tm_pre_fifo_cntrs_t *fifo_cntrs));

  MOCK_METHOD2(bfTmPipeBufferFullDropClear,
               tdi_status_t(tdi_dev_id_t dev, bf_dev_pipe_t pipe));

  MOCK_METHOD2(bfTmQDiscardWatermarkClear,
               tdi_status_t(tdi_dev_id_t dev, bf_dev_pipe_t pipe));

  MOCK_METHOD2(bfTmPipeClearCellCounter,
               tdi_status_t(tdi_dev_id_t dev, bf_dev_pipe_t pipe));

  MOCK_METHOD2(bfTmPipeClearPacketCounter,
               tdi_status_t(tdi_dev_id_t dev, bf_dev_pipe_t pipe));

  MOCK_METHOD2(bfTmPipeClearUcCtPacketCounter,
               tdi_status_t(tdi_dev_id_t dev, bf_dev_pipe_t pipe));

  MOCK_METHOD2(bfTmPipeClearMcCtPacketCounter,
               tdi_status_t(tdi_dev_id_t dev, bf_dev_pipe_t pipe));

  MOCK_METHOD3(bfTmBlklvlDropClear,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            uint32_t clear_mask));

  MOCK_METHOD3(bfTmPreFifoDropClear,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            uint32_t fifo));

  /***** PPG Counter APIs *******/
  MOCK_METHOD4(bfTmPpgDropGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            bf_tm_ppg_hdl ppg,
                            uint64_t *count));

  MOCK_METHOD4(bfTmPpgDropGetCached,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            bf_tm_ppg_hdl ppg,
                            uint64_t *count));

  MOCK_METHOD2(bfTmPpgDropCountClear,
               tdi_status_t(tdi_dev_id_t dev, bf_tm_ppg_hdl ppg));

  MOCK_METHOD7(bfTmPpgUsageGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            bf_tm_ppg_hdl ppg,
                            uint32_t *gmin_count,
                            uint32_t *shared_count,
                            uint32_t *skid_count,
                            uint32_t *wm));

  MOCK_METHOD2(bfTmPpgWatermarkClear,
               tdi_status_t(tdi_dev_id_t dev, bf_tm_ppg_hdl ppg));

  /***** TM Cfg APIs *******/
  MOCK_METHOD2(bfTmTimestampShiftSet,
               tdi_status_t(tdi_dev_id_t dev, uint8_t shift));

  MOCK_METHOD2(bfTmTimestampShiftGet,
               tdi_status_t(tdi_dev_id_t dev, uint8_t *shift));

  MOCK_METHOD2(bfTmTimestampShiftGetDefault,
               tdi_status_t(tdi_dev_id_t dev, uint8_t *shift));

  MOCK_METHOD2(bfTmCellSizeInBytesGet,
               tdi_status_t(tdi_dev_id_t dev, uint32_t *bytes));

  MOCK_METHOD2(bfTmTotalCellCountGet,
               tdi_status_t(tdi_dev_id_t dev, uint32_t *total_cells));

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
               tdi_status_t(tdi_dev_id_t dev,
                            uint8_t pipe_bmap,
                            int fifo,
                            uint8_t icos_bmap));

  MOCK_METHOD4(bfTmMcFifoIcosMappingGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            int fifo,
                            uint8_t *icos_bmap));

  MOCK_METHOD4(bfTmMcFifoIcosMappingGetDefault,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            int fifo,
                            uint8_t *icos_bmap));

  MOCK_METHOD4(bfTmMcFifoArbModeSet,
               tdi_status_t(tdi_dev_id_t dev,
                            uint8_t pipe_bmap,
                            int fifo,
                            bool use_strict_pri));

  MOCK_METHOD4(bfTmMcFifoArbModeGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            int fifo,
                            bool *use_strict_pri));

  MOCK_METHOD4(bfTmMcFifoArbModeGetDefault,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            int fifo,
                            bool *use_strict_pri));

  MOCK_METHOD4(bfTmMcFifoWrrWeightSet,
               tdi_status_t(tdi_dev_id_t dev,
                            uint8_t pipe_bmap,
                            int fifo,
                            uint8_t weight));

  MOCK_METHOD4(bfTmMcFifoWrrWeightGet,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            int fifo,
                            uint8_t *weight));

  MOCK_METHOD4(bfTmMcFifoWrrWeightGetDefault,
               tdi_status_t(tdi_dev_id_t dev,
                            bf_dev_pipe_t pipe,
                            int fifo,
                            uint8_t *weight));

  MOCK_METHOD4(
      bfTmMcFifoDepthSet,
      tdi_status_t(tdi_dev_id_t dev, uint8_t pipe_bmap, int fifo, int size));

  MOCK_METHOD4(
      bfTmMcFifoDepthGet,
      tdi_status_t(tdi_dev_id_t dev, bf_dev_pipe_t pipe, int fifo, int *size));

  MOCK_METHOD4(
      bfTmMcFifoDepthGetDefault,
      tdi_status_t(tdi_dev_id_t dev, bf_dev_pipe_t pipe, int fifo, int *size));
};
}  // namespace tdi_test
}  // namespace tdi
#endif
