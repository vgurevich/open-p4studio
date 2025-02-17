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

include "res.thrift"

namespace py conn_mgr_pd_rpc
namespace cpp conn_mgr_pd_rpc
namespace c_glib conn_mgr_pd_rpc

enum PktGenTriggerType_t {
    TIMER_ONE_SHOT=0,
    TIMER_PERIODIC=1,
    PORT_DOWN=2,
    RECIRC_PATTERN=3,
    DPRSR=4,
    PFC=5
}
struct PktGenAppCfg_t {
  1: required PktGenTriggerType_t trigger_type;
  2: i32 batch_count = 0;
  3: i32 pkt_count = 0;
  4: i32 pattern_key = 0;
  5: i32 pattern_msk = 0;
  6: i32 timer = 0;
  7: i32 ibg = 0;
  8: i32 ibg_jitter = 0;
  9: i32 ipg = 0;
  10: i32 ipg_jitter = 0;
  11: i32 src_port = 0;
  12: i32 src_port_inc = 0;
  13: i32 buffer_offset = 0;
  14: required i32 length;
}

struct PktGenAppCfg_tof2_t {
  1: required PktGenTriggerType_t trigger_type;
  2: i32 batch_count = 0;
  3: i32 pkt_count = 0;
  4: binary pattern_key;
  5: binary pattern_msk;
  6: binary pfc_hdr;
  7: bool pfc_timer_en = 0;
  8: i16 pfc_timer = 0;
  9: i16 pfc_max_msgs = 0;
  10: i32 timer = 0;
  11: i32 ibg = 0;
  12: i32 ibg_jitter = 0;
  13: i32 ipg = 0;
  14: i32 ipg_jitter = 0;
  15: i32 src_port = 0;
  16: i32 src_port_inc = 0;
  17: i32 buffer_offset = 0;
  18: required i32 length;
  19: i16 port_mask_sel = 0;
  20: i16 source_port_wrap_max = 0;
  21: i16 assigned_chnl_id = 0;
  22: bool offset_len_from_recir_pkt = 0;
}

enum PktGenPortDownReplay_t {
  REPLAY_NONE = 0,
  REPLAY_ALL = 1,
  REPLAY_MISSED = 2
}

struct PortMask_t {
  1: binary mask
}

struct indirect_reg_data_t {
  1: required i64 hi;
  2: required i64 lo;
}

exception InvalidConnMgrOperation {
  1:i32 code
}

exception InvalidPktGenOperation {
  1:i32 code
}

service conn_mgr {
    # Test echo interface
    void echo(1:string s);

    void init() throws (1:InvalidConnMgrOperation ouch);

    void cleanup() throws (1:InvalidConnMgrOperation ouch);

    res.SessionHandle_t client_init() throws (1:InvalidConnMgrOperation ouch);

    void client_cleanup(1:res.SessionHandle_t sess_hdl) throws (1:InvalidConnMgrOperation ouch);

    void begin_txn(1:res.SessionHandle_t sess_hdl, 2:bool isAtomic) throws (1:InvalidConnMgrOperation ouch);

    void verify_txn(1:res.SessionHandle_t sess_hdl) throws (1:InvalidConnMgrOperation ouch);

    void abort_txn(1:res.SessionHandle_t sess_hdl) throws (1:InvalidConnMgrOperation ouch);

    void commit_txn(1:res.SessionHandle_t sess_hdl, 2:bool hwSynchronous) throws (1:InvalidConnMgrOperation ouch);

    void complete_operations(1:res.SessionHandle_t sess_hdl) throws (1:InvalidConnMgrOperation ouch);

    void begin_batch(1:res.SessionHandle_t sess_hdl) throws (1:InvalidConnMgrOperation ouch);

    void flush_batch(1:res.SessionHandle_t sess_hdl) throws (1:InvalidConnMgrOperation ouch);

    void end_batch(1:res.SessionHandle_t sess_hdl, 2:bool hwSynchronous) throws (1:InvalidConnMgrOperation ouch);

    void log_state(1:i32 device_id, 2:binary filepath) throws (1:InvalidConnMgrOperation ouch);

    void restore_state(1:i32 device_id, 2:binary filepath) throws (1:InvalidConnMgrOperation ouch);

    void advance_model_time(1:res.SessionHandle_t sess_hdl, 2:i32 device_id, 3:i64 tick_time) throws (1:InvalidConnMgrOperation ouch);

    void recirculation_enable(1:res.SessionHandle_t sess_hdl, 2:i32 dev, 3:i32 port) throws (1:InvalidConnMgrOperation ouch);
    void recirculation_disable(1:res.SessionHandle_t sess_hdl, 2:i32 dev, 3:i32 port) throws (1:InvalidConnMgrOperation ouch);

    void pktgen_enable(1:res.SessionHandle_t sess_hdl, 2:i32 dev, 3:i32 port) throws (1:InvalidPktGenOperation ouch);
    void pktgen_disable(1:res.SessionHandle_t sess_hdl, 2:i32 dev, 3:i32 port) throws (1:InvalidPktGenOperation ouch);
    bool pktgen_enable_state_get(1:res.SessionHandle_t sess_hdl, 2:i32 dev, 3:i32 port) throws (1:InvalidPktGenOperation ouch);
    void pktgen_enable_recirc_pattern_matching(1:res.SessionHandle_t sess_hdl, 2:i32 dev, 3:i32 port) throws (1:InvalidPktGenOperation ouch);
    void pktgen_disable_recirc_pattern_matching(1:res.SessionHandle_t sess_hdl, 2:i32 dev, 3:i32 port) throws (1:InvalidPktGenOperation ouch);
    bool pktgen_recirc_pattern_matching_state_get(1:res.SessionHandle_t sess_hdl, 2:i32 dev, 3:i32 port) throws (1:InvalidPktGenOperation ouch);
    void pktgen_clear_port_down(1:res.SessionHandle_t sess_hdl, 2:i32 dev, 3:i32 port) throws (1:InvalidPktGenOperation ouch);
    bool pktgen_port_down_state_get(1:res.SessionHandle_t sess_hdl, 2:i32 dev, 3:i32 port) throws (1:InvalidPktGenOperation ouch);
    void pktgen_cfg_app(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:i32 app_id, 4:PktGenAppCfg_t cfg) throws (1:InvalidPktGenOperation ouch);
    void pktgen_cfg_app_tof2(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:i32 app_id, 4:PktGenAppCfg_tof2_t cfg) throws (1:InvalidPktGenOperation ouch);
    void pktgen_cfg_port_mask_tof2(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:i32 sel, 4:PortMask_t mask_in) throws (1:InvalidPktGenOperation ouch);
    PortMask_t pktgen_cfg_port_mask_tof2_get(1:res.DevTarget_t dev_tgt, 2:i32 sel) throws (1:InvalidPktGenOperation ouch);
    PktGenAppCfg_t pktgen_cfg_app_get(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:i32 app_id) throws (1:InvalidPktGenOperation ouch);
    PktGenAppCfg_tof2_t pktgen_cfg_app_tof2_get(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:i32 app_id) throws (1:InvalidPktGenOperation ouch);
    void pktgen_app_enable(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:i32 app_id) throws (1:InvalidPktGenOperation ouch);
    void pktgen_app_disable(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:i32 app_id) throws (1:InvalidPktGenOperation ouch);
    bool pktgen_app_enable_state_get(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:i32 app_id) throws (1:InvalidPktGenOperation ouch);
    void pktgen_write_pkt_buffer(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:i32 offset, 4:i32 size, 5:binary buf) throws (1:InvalidPktGenOperation ouch);
    binary pktgen_read_pkt_buffer(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:i32 offset, 4:i32 size) throws (1:InvalidPktGenOperation ouch);
    i64 pktgen_get_batch_counter(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:i32 app_id) throws (1:InvalidPktGenOperation ouch);
    i64 pktgen_get_pkt_counter(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:i32 app_id) throws (1:InvalidPktGenOperation ouch);
    i64 pktgen_get_trigger_counter(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:i32 app_id) throws (1:InvalidPktGenOperation ouch);
    void pktgen_set_batch_counter(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:i32 app_id, 4:i64 count) throws (1:InvalidPktGenOperation ouch);
    void pktgen_set_pkt_counter(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:i32 app_id, 4:i64 count) throws (1:InvalidPktGenOperation ouch);
    void pktgen_set_trigger_counter(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:i32 app_id, 4:i64 count) throws (1:InvalidPktGenOperation ouch);
    void pktgen_port_down_replay_mode_set(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:PktGenPortDownReplay_t mode) throws (1:InvalidPktGenOperation ouch);
    PktGenPortDownReplay_t pktgen_port_down_replay_mode_get(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt) throws (1:InvalidPktGenOperation ouch);

    void reg_wr(1:i32 dev, 2:i32 addr, 3:i32 data) throws (1:InvalidConnMgrOperation ouch)
    i32 reg_rd(1:i32 dev, 2:i32 addr) throws (1:InvalidConnMgrOperation ouch)
    void ind_reg_wr(1:i32 dev, 2:i64 addr, 3:indirect_reg_data_t data) throws (1:InvalidConnMgrOperation ouch)
    indirect_reg_data_t ind_reg_rd(1:i32 dev, 2:i64 addr) throws (1:InvalidConnMgrOperation ouch)
    void tcam_scrub_timer_set(1: i32 dev, 2: i32 msec_timer) throws (1:InvalidConnMgrOperation ouch);
    i32 tcam_scrub_timer_get(1: i32 dev) throws (1:InvalidConnMgrOperation ouch);
    void flow_lrn_set_intr_mode(1:res.SessionHandle_t sess_hdl, 2: i32 dev, 3: bool en) throws (1:InvalidConnMgrOperation ouch);
    bool flow_lrn_get_intr_mode(1:res.SessionHandle_t sess_hdl, 2: i32 dev) throws (1:InvalidConnMgrOperation ouch);
}
