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


extern "C" {
#include <tofino/bf_pal/bf_pal_port_intf.h>
#include <traffic_mgr/traffic_mgr_counters.h>
#include <tofino/pdfixed/pd_common.h>
#include <tofino/pdfixed/pd_mirror.h>
#include <tofino/pdfixed/pd_mc.h>
#include <tofino/bf_pal/pltfm_intf.h>
#include <pipe_mgr/bf_packetpath_counter.h>
#include <lld/bf_lld_if.h>
}

#include <cassert>
#include <vector>
#include <set>
#include <memory>
#include <utility>
#include <string>
#include <algorithm>

#include "common/multicast.h"
#include "switch_tna/p4_16_types.h"
#include "switch_tna/utils.h"
#include "common/bfrt_tm.h"

namespace smi {
using namespace smi::bf_rt;  // NOLINT(build/namespaces)
using ::smi::logging::switch_log;

void switch_port_counter_to_pd_port_counters_list(
    switch_port_counter_id counter_id,
    std::vector<bf_rmon_counter_t> &rmon_cntrs) {
  switch (counter_id) {
    case SWITCH_PORT_COUNTER_ID_OCTETS:
      rmon_cntrs.push_back(bf_mac_stat_OctetsReceived);
      rmon_cntrs.push_back(bf_mac_stat_OctetsTransmittedTotal);
      break;
    case SWITCH_PORT_COUNTER_ID_PKTS:
      rmon_cntrs.push_back(bf_mac_stat_FramesReceivedAll);
      rmon_cntrs.push_back(bf_mac_stat_FramesTransmittedAll);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_NON_UCAST_PKTS:
      rmon_cntrs.push_back(bf_mac_stat_FramesReceivedwithMulticastAddresses);
      rmon_cntrs.push_back(bf_mac_stat_FramesReceivedwithBroadcastAddresses);
      break;
    case SWITCH_PORT_COUNTER_ID_OUT_NON_UCAST_PKTS:
      rmon_cntrs.push_back(bf_mac_stat_FramesTransmittedMulticast);
      rmon_cntrs.push_back(bf_mac_stat_FramesTransmittedBroadcast);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_OVER_SIZED_PKTS:
      rmon_cntrs.push_back(bf_mac_stat_FramesReceivedOversized);
      rmon_cntrs.push_back(bf_mac_stat_JabberReceived);
      break;
    case SWITCH_PORT_COUNTER_ID_OUT_OVER_SIZED_PKTS:
      rmon_cntrs.push_back(bf_mac_stat_FramesTransmittedLength_1519_2047);
      rmon_cntrs.push_back(bf_mac_stat_FramesTransmittedLength_2048_4095);
      rmon_cntrs.push_back(bf_mac_stat_FramesTransmittedLength_4096_8191);
      rmon_cntrs.push_back(bf_mac_stat_FramesTransmittedLength_8192_9215);
      rmon_cntrs.push_back(bf_mac_stat_FramesTransmittedLength_9216);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_GOOD_OCTETS:
      rmon_cntrs.push_back(bf_mac_stat_OctetsReceivedinGoodFrames);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_ALL_OCTETS:
      rmon_cntrs.push_back(bf_mac_stat_OctetsReceived);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_GOOD_PKTS:
      rmon_cntrs.push_back(bf_mac_stat_FramesReceivedOK);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_ALL_PKTS:
      rmon_cntrs.push_back(bf_mac_stat_FramesReceivedAll);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_VLAN_PKTS:
      rmon_cntrs.push_back(bf_mac_stat_RxVLANFramesGood);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_UCAST_PKTS:
      rmon_cntrs.push_back(bf_mac_stat_FramesReceivedwithUnicastAddresses);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_MCAST_PKTS:
      rmon_cntrs.push_back(bf_mac_stat_FramesReceivedwithMulticastAddresses);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_BCAST_PKTS:
      rmon_cntrs.push_back(bf_mac_stat_FramesReceivedwithBroadcastAddresses);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_FCS_ERRORS:
      rmon_cntrs.push_back(bf_mac_stat_FramesReceivedwithFCSError);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_ERROR_PKTS:
      rmon_cntrs.push_back(bf_mac_stat_FrameswithanyError);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_CRC_ERRORS:
      rmon_cntrs.push_back(bf_mac_stat_CRCErrorStomped);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_BUFFER_FULL:
      rmon_cntrs.push_back(bf_mac_stat_FramesDroppedBufferFull);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_FRAGMENTS:
      rmon_cntrs.push_back(bf_mac_stat_FragmentsReceived);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_JABBERS:
      rmon_cntrs.push_back(bf_mac_stat_JabberReceived);
      break;
    case SWITCH_PORT_COUNTER_ID_OUT_GOOD_OCTETS:
      rmon_cntrs.push_back(bf_mac_stat_OctetsTransmittedwithouterror);
      break;
    case SWITCH_PORT_COUNTER_ID_OUT_ALL_OCTETS:
      rmon_cntrs.push_back(bf_mac_stat_OctetsTransmittedTotal);
      break;
    case SWITCH_PORT_COUNTER_ID_OUT_GOOD_PKTS:
      rmon_cntrs.push_back(bf_mac_stat_FramesTransmittedOK);
      break;
    case SWITCH_PORT_COUNTER_ID_OUT_ALL_PKTS:
      rmon_cntrs.push_back(bf_mac_stat_FramesTransmittedAll);
      break;
    case SWITCH_PORT_COUNTER_ID_OUT_VLAN_PKTS:
      rmon_cntrs.push_back(bf_mac_stat_FramesTransmittedVLAN);
      break;
    case SWITCH_PORT_COUNTER_ID_OUT_UCAST_PKTS:
      rmon_cntrs.push_back(bf_mac_stat_FramesTransmittedUnicast);
      break;
    case SWITCH_PORT_COUNTER_ID_OUT_MCAST_PKTS:
      rmon_cntrs.push_back(bf_mac_stat_FramesTransmittedMulticast);
      break;
    case SWITCH_PORT_COUNTER_ID_OUT_BCAST_PKTS:
      rmon_cntrs.push_back(bf_mac_stat_FramesTransmittedBroadcast);
      break;
    case SWITCH_PORT_COUNTER_ID_OUT_ERROR_PKTS:
      rmon_cntrs.push_back(bf_mac_stat_FramesTransmittedwithError);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_PKTS_LT_64:
      rmon_cntrs.push_back(bf_mac_stat_FramesReceivedLength_lt_64);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_PKTS_EQ_64:
      rmon_cntrs.push_back(bf_mac_stat_FramesReceivedLength_eq_64);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_PKTS_65_TO_127:
      rmon_cntrs.push_back(bf_mac_stat_FramesReceivedLength_65_127);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_PKTS_128_TO_255:
      rmon_cntrs.push_back(bf_mac_stat_FramesReceivedLength_128_255);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_PKTS_256_TO_511:
      rmon_cntrs.push_back(bf_mac_stat_FramesReceivedLength_256_511);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_PKTS_512_TO_1023:
      rmon_cntrs.push_back(bf_mac_stat_FramesReceivedLength_512_1023);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_PKTS_1024_TO_1518:
      rmon_cntrs.push_back(bf_mac_stat_FramesReceivedLength_1024_1518);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_PKTS_1519_TO_2047:
      rmon_cntrs.push_back(bf_mac_stat_FramesReceivedLength_1519_2047);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_PKTS_2048_TO_4095:
      rmon_cntrs.push_back(bf_mac_stat_FramesReceivedLength_2048_4095);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_PKTS_4096_TO_8191:
      rmon_cntrs.push_back(bf_mac_stat_FramesReceivedLength_4096_8191);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_PKTS_8192_TO_9215:
      rmon_cntrs.push_back(bf_mac_stat_FramesReceivedLength_8192_9215);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_PKTS_9216:
      rmon_cntrs.push_back(bf_mac_stat_FramesReceivedLength_9216);
      break;
    case SWITCH_PORT_COUNTER_ID_OUT_PKTS_LT_64:
      rmon_cntrs.push_back(bf_mac_stat_FramesTransmittedLength_lt_64);
      break;
    case SWITCH_PORT_COUNTER_ID_OUT_PKTS_EQ_64:
      rmon_cntrs.push_back(bf_mac_stat_FramesTransmittedLength_eq_64);
      break;
    case SWITCH_PORT_COUNTER_ID_OUT_PKTS_65_TO_127:
      rmon_cntrs.push_back(bf_mac_stat_FramesTransmittedLength_65_127);
      break;
    case SWITCH_PORT_COUNTER_ID_OUT_PKTS_128_TO_255:
      rmon_cntrs.push_back(bf_mac_stat_FramesTransmittedLength_128_255);
      break;
    case SWITCH_PORT_COUNTER_ID_OUT_PKTS_256_TO_511:
      rmon_cntrs.push_back(bf_mac_stat_FramesTransmittedLength_256_511);
      break;
    case SWITCH_PORT_COUNTER_ID_OUT_PKTS_512_TO_1023:
      rmon_cntrs.push_back(bf_mac_stat_FramesTransmittedLength_512_1023);
      break;
    case SWITCH_PORT_COUNTER_ID_OUT_PKTS_1024_TO_1518:
      rmon_cntrs.push_back(bf_mac_stat_FramesTransmittedLength_1024_1518);
      break;
    case SWITCH_PORT_COUNTER_ID_OUT_PKTS_1519_TO_2047:
      rmon_cntrs.push_back(bf_mac_stat_FramesTransmittedLength_1519_2047);
      break;
    case SWITCH_PORT_COUNTER_ID_OUT_PKTS_2048_TO_4095:
      rmon_cntrs.push_back(bf_mac_stat_FramesTransmittedLength_2048_4095);
      break;
    case SWITCH_PORT_COUNTER_ID_OUT_PKTS_4096_TO_8191:
      rmon_cntrs.push_back(bf_mac_stat_FramesTransmittedLength_4096_8191);
      break;
    case SWITCH_PORT_COUNTER_ID_OUT_PKTS_8192_TO_9215:
      rmon_cntrs.push_back(bf_mac_stat_FramesTransmittedLength_8192_9215);
      break;
    case SWITCH_PORT_COUNTER_ID_OUT_PKTS_9216:
      rmon_cntrs.push_back(bf_mac_stat_FramesTransmittedLength_9216);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_PAUSE_PKTS:
      rmon_cntrs.push_back(bf_mac_stat_FramesReceivedoftypePAUSE);
      break;
    case SWITCH_PORT_COUNTER_ID_OUT_PAUSE_PKTS:
      rmon_cntrs.push_back(bf_mac_stat_FramesTransmittedPause);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_PFC_0_PKTS:
      rmon_cntrs.push_back(bf_mac_stat_Pri0FramesReceived);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_PFC_1_PKTS:
      rmon_cntrs.push_back(bf_mac_stat_Pri1FramesReceived);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_PFC_2_PKTS:
      rmon_cntrs.push_back(bf_mac_stat_Pri2FramesReceived);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_PFC_3_PKTS:
      rmon_cntrs.push_back(bf_mac_stat_Pri3FramesReceived);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_PFC_4_PKTS:
      rmon_cntrs.push_back(bf_mac_stat_Pri4FramesReceived);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_PFC_5_PKTS:
      rmon_cntrs.push_back(bf_mac_stat_Pri5FramesReceived);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_PFC_6_PKTS:
      rmon_cntrs.push_back(bf_mac_stat_Pri6FramesReceived);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_PFC_7_PKTS:
      rmon_cntrs.push_back(bf_mac_stat_Pri7FramesReceived);
      break;
    case SWITCH_PORT_COUNTER_ID_OUT_PFC_0_PKTS:
      rmon_cntrs.push_back(bf_mac_stat_Pri0FramesTransmitted);
      break;
    case SWITCH_PORT_COUNTER_ID_OUT_PFC_1_PKTS:
      rmon_cntrs.push_back(bf_mac_stat_Pri1FramesTransmitted);
      break;
    case SWITCH_PORT_COUNTER_ID_OUT_PFC_2_PKTS:
      rmon_cntrs.push_back(bf_mac_stat_Pri2FramesTransmitted);
      break;
    case SWITCH_PORT_COUNTER_ID_OUT_PFC_3_PKTS:
      rmon_cntrs.push_back(bf_mac_stat_Pri3FramesTransmitted);
      break;
    case SWITCH_PORT_COUNTER_ID_OUT_PFC_4_PKTS:
      rmon_cntrs.push_back(bf_mac_stat_Pri4FramesTransmitted);
      break;
    case SWITCH_PORT_COUNTER_ID_OUT_PFC_5_PKTS:
      rmon_cntrs.push_back(bf_mac_stat_Pri5FramesTransmitted);
      break;
    case SWITCH_PORT_COUNTER_ID_OUT_PFC_6_PKTS:
      rmon_cntrs.push_back(bf_mac_stat_Pri6FramesTransmitted);
      break;
    case SWITCH_PORT_COUNTER_ID_OUT_PFC_7_PKTS:
      rmon_cntrs.push_back(bf_mac_stat_Pri7FramesTransmitted);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_UNDER_SIZED_PKTS:
      rmon_cntrs.push_back(bf_mac_stat_FramesReceivedUndersized);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_FRAMES_TOO_LONG:
      rmon_cntrs.push_back(bf_mac_stat_FrameTooLong);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_PFC_0_RX_PAUSE_DURATION:
      rmon_cntrs.push_back(bf_mac_stat_ReceivePri0Pause1USCount);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_PFC_1_RX_PAUSE_DURATION:
      rmon_cntrs.push_back(bf_mac_stat_ReceivePri1Pause1USCount);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_PFC_2_RX_PAUSE_DURATION:
      rmon_cntrs.push_back(bf_mac_stat_ReceivePri2Pause1USCount);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_PFC_3_RX_PAUSE_DURATION:
      rmon_cntrs.push_back(bf_mac_stat_ReceivePri3Pause1USCount);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_PFC_4_RX_PAUSE_DURATION:
      rmon_cntrs.push_back(bf_mac_stat_ReceivePri4Pause1USCount);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_PFC_5_RX_PAUSE_DURATION:
      rmon_cntrs.push_back(bf_mac_stat_ReceivePri5Pause1USCount);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_PFC_6_RX_PAUSE_DURATION:
      rmon_cntrs.push_back(bf_mac_stat_ReceivePri6Pause1USCount);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_PFC_7_RX_PAUSE_DURATION:
      rmon_cntrs.push_back(bf_mac_stat_ReceivePri7Pause1USCount);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_PFC_0_TX_PAUSE_DURATION:
      rmon_cntrs.push_back(bf_mac_stat_TransmitPri0Pause1USCount);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_PFC_1_TX_PAUSE_DURATION:
      rmon_cntrs.push_back(bf_mac_stat_TransmitPri1Pause1USCount);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_PFC_2_TX_PAUSE_DURATION:
      rmon_cntrs.push_back(bf_mac_stat_TransmitPri2Pause1USCount);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_PFC_3_TX_PAUSE_DURATION:
      rmon_cntrs.push_back(bf_mac_stat_TransmitPri3Pause1USCount);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_PFC_4_TX_PAUSE_DURATION:
      rmon_cntrs.push_back(bf_mac_stat_TransmitPri4Pause1USCount);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_PFC_5_TX_PAUSE_DURATION:
      rmon_cntrs.push_back(bf_mac_stat_TransmitPri5Pause1USCount);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_PFC_6_TX_PAUSE_DURATION:
      rmon_cntrs.push_back(bf_mac_stat_TransmitPri6Pause1USCount);
      break;
    case SWITCH_PORT_COUNTER_ID_IN_PFC_7_TX_PAUSE_DURATION:
      rmon_cntrs.push_back(bf_mac_stat_TransmitPri7Pause1USCount);
      break;
    default:
      break;
  }
}

class port_metadata : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_PORT_METADATA;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_PORT_METADATA_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_PORT_METADATA_ATTR_STATUS;

 public:
  port_metadata(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_PORT_METADATA,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_object_id_t lag_handle = {0}, ingress_acl_handle = {0},
                       device_handle = {0};
    uint16_t port_lag_index = 0;
    switch_ig_port_lag_label_t port_lag_label = 0;
    switch_enum_t e = {0};
    uint16_t external_dev_port{}, port{};
    bf_status_t rc = BF_SUCCESS;

    CHECK_PORT_IN_SWITCH_PIPE_AND_RETURN(parent);
    status |=
        switch_store::v_get(parent, SWITCH_PORT_ATTR_DEVICE, device_handle);
    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_TYPE, e);
    status |= switch_store::v_get(
        parent, SWITCH_PORT_ATTR_DEV_PORT, external_dev_port);
    auto pipe = DEV_PORT_TO_PIPE(external_dev_port);
    auto &switch_ingress_pipes = SWITCH_CONTEXT.get_switch_ingress_pipe_list();
    // If port is CPU and it is not in Switch Ingress Pipeline, then find
    // corresponding recirc port in Switch Ingress Pipeline and use the
    // translation recirc port -> cpu port in switch ingress pipeline. (CPU
    // Packets incoming on non Switch Ingress Pipeline are snaked to the Switch
    // Ingress Pipeline via the recirc port)
    if (e.enumdata == SWITCH_PORT_ATTR_TYPE_CPU) {
      if (std::find(switch_ingress_pipes.begin(),
                    switch_ingress_pipes.end(),
                    pipe) == switch_ingress_pipes.end()) {
        status = get_recirc_port_in_pipe(
            device_handle, INGRESS_DEV_PORT_TO_PIPE(external_dev_port), port);
      } else {
        port = external_dev_port;
      }
    } else if (e.enumdata == SWITCH_PORT_ATTR_TYPE_RECIRC) {
      // If port is Recirc then skip programming port metadata table if
      // 1. Recirc port belongs to Non Switch Ingress Pipeline (port metadata
      // table does not exist on this pipe)
      // 2. AFP is being run. For AFP recirc port -> CPU port mapping is used in
      // Switch Ingress Pipeline
      if (std::find(switch_ingress_pipes.begin(),
                    switch_ingress_pipes.end(),
                    pipe) == switch_ingress_pipes.end() ||
          feature::is_feature_set(SWITCH_FEATURE_ASYMMETRIC_FOLDED_PIPELINE)) {
        return;
      } else {
        port = external_dev_port;
      }
    } else {
      port = translate_ingress_port(external_dev_port);
    }

    status |=
        switch_store::v_get(parent, SWITCH_PORT_ATTR_LAG_HANDLE, lag_handle);
    port_lag_index =
        compute_port_lag_index(lag_handle.data ? lag_handle : parent);

    if (lag_handle.data == 0) {
      status |= switch_store::v_get(
          parent, SWITCH_PORT_ATTR_INGRESS_ACL_HANDLE, ingress_acl_handle);
      if (ingress_acl_handle.data == 0) {
        status |= switch_store::v_get(
            parent, SWITCH_PORT_ATTR_INGRESS_PORT_LAG_LABEL, port_lag_label);
      } else {
        port_lag_label = compute_bind_label(parent, ingress_acl_handle);
      }
    } else {
      std::set<switch_object_id_t> lag_members;
      status |= switch_store::referencing_set_get(
          parent, SWITCH_OBJECT_TYPE_LAG_MEMBER, lag_members);
      if (lag_members.size() != 1) {
        status = SWITCH_STATUS_FAILURE;
        return;
      } else {
        auto lag_member = *lag_members.begin();
        bool ingress_disable = false;
        status |= switch_store::v_get(lag_member,
                                      SWITCH_LAG_MEMBER_ATTR_INGRESS_DISABLE,
                                      ingress_disable);
        // If ingress disable is true for the lag member, then we do not
        // consider the port as part of the lag
        if (ingress_disable) port_lag_index = 0;
      }
      status |= switch_store::v_get(
          lag_handle, SWITCH_LAG_ATTR_INGRESS_ACL_HANDLE, ingress_acl_handle);
      if (ingress_acl_handle.data == 0) {
        status |= switch_store::v_get(
            lag_handle, SWITCH_LAG_ATTR_INGRESS_PORT_LAG_LABEL, port_lag_label);
      } else {
        port_lag_label = compute_bind_label(lag_handle, ingress_acl_handle);
      }
    }
    uint32_t port_exclusion_bit_map{};
    status |= switch_store::v_get(parent,
                                  SWITCH_PORT_ATTR_PORT_EXCLUSION_BIT_MAP,
                                  port_exclusion_bit_map);
    port_lag_label |= port_exclusion_bit_map;

    const BfRtTable *table = NULL;
    const BfRtInfo *bfrtinfo = get_bf_rt_info();
    rc = bfrtinfo->bfrtTableFromIdGet(smi_id::T_PORT_METADATA, &table);
    if (rc != BF_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_PORT,
                 "{}.{}: Failed to get table from id {}",
                 "port_metadata",
                 __LINE__,
                 smi_id::T_PORT_METADATA);
    }

    status |= match_key.set_exact(smi_id::F_PORT_METADATA_PORT, port);
    action_entry.init_indirect_data();

    status |= action_entry.set_arg(smi_id::P_PORT_METADATA_PORT_LAG_INDEX,
                                   port_lag_index);
    bool use_ingress_port_group = false;
    status |=
        switch_store::v_get(device_handle,
                            SWITCH_DEVICE_ATTR_INGRESS_ACL_PORT_GROUP_ENABLE,
                            use_ingress_port_group);
    if (use_ingress_port_group == false) {
      status |= action_entry.set_arg(smi_id::P_PORT_METADATA_PORT_LAG_LABEL,
                                     port_lag_label);
    } else {
      uint32_t port_group_index = 0;
      if (lag_handle.data == 0) {
        status |= switch_store::v_get(parent,
                                      SWITCH_PORT_ATTR_INGRESS_PORT_GROUP_INDEX,
                                      port_group_index);
        switch_log(SWITCH_API_LEVEL_DEBUG,
                   SWITCH_OBJECT_TYPE_PORT,
                   "Port {} Program port in hardware with Port group index {}",
                   parent,
                   port_group_index);
      } else {
        status |= switch_store::v_get(lag_handle,
                                      SWITCH_LAG_ATTR_INGRESS_PORT_GROUP_INDEX,
                                      port_group_index);
        switch_log(SWITCH_API_LEVEL_DEBUG,
                   SWITCH_OBJECT_TYPE_LAG,
                   "LAG {} Program port in hardware with Port group index {}",
                   lag_handle,
                   port_group_index);
      }
      status |= action_entry.set_arg(smi_id::P_PORT_METADATA_PORT_LAG_LABEL,
                                     port_group_index);
    }
    if (feature::is_feature_set(SWITCH_FEATURE_ASYMMETRIC_FOLDED_PIPELINE)) {
      status |= action_entry.set_arg(smi_id::P_PORT_METADATA_EXT_INGRESS_PORT,
                                     parent,
                                     SWITCH_PORT_ATTR_DEV_PORT);
    }
  }
};

/* PORT_MAPPING p4 object */
class ingress_port_mapping : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_INGRESS_PORT_MAPPING;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INGRESS_PORT_MAPPING_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_INGRESS_PORT_MAPPING_ATTR_STATUS;

 public:
  ingress_port_mapping(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_INGRESS_PORT_MAPPING,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_object_id_t lag_handle = {0}, ingress_acl_handle = {0};
    switch_ig_port_lag_label_t port_lag_label = 0;
    uint16_t yid = 0;
    switch_enum_t e = {0};
    bool learning = false;
    bool port_qos_config_precedence = false;
    uint8_t color = 0, tc = 0;
    switch_enum_t _color = {};

    CHECK_PORT_IN_SWITCH_PIPE_AND_RETURN(parent);

    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_TYPE, e);
    status |= switch_store::v_get(parent,
                                  SWITCH_PORT_ATTR_QOS_CONFIG_PRECEDENCE,
                                  port_qos_config_precedence);
    status |=
        switch_store::v_get(parent, SWITCH_PORT_ATTR_LAG_HANDLE, lag_handle);

    const auto port_lag_index =
        compute_port_lag_index(lag_handle.data ? lag_handle : parent);

    if ((lag_handle.data == 0) || port_qos_config_precedence) {
      status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_COLOR, _color);
      color = static_cast<uint8_t>(_color.enumdata);
      if (color == SWITCH_PORT_ATTR_COLOR_RED) color = SWITCH_PKT_COLOR_RED;
      status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_TC, tc);
    }

    if (lag_handle.data == 0) {
      status |= switch_store::v_get(
          parent, SWITCH_PORT_ATTR_INGRESS_ACL_HANDLE, ingress_acl_handle);
      if (ingress_acl_handle.data == 0) {
        status |= switch_store::v_get(
            parent, SWITCH_PORT_ATTR_INGRESS_PORT_LAG_LABEL, port_lag_label);
      } else {
        port_lag_label = compute_bind_label(parent, ingress_acl_handle);
      }
      status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_YID, yid);
      status |=
          switch_store::v_get(parent, SWITCH_PORT_ATTR_LEARNING, learning);
    } else {
      status |= switch_store::v_get(
          lag_handle, SWITCH_LAG_ATTR_INGRESS_ACL_HANDLE, ingress_acl_handle);
      if (ingress_acl_handle.data == 0) {
        status |= switch_store::v_get(
            lag_handle, SWITCH_LAG_ATTR_INGRESS_PORT_LAG_LABEL, port_lag_label);
      } else {
        port_lag_label = compute_bind_label(lag_handle, ingress_acl_handle);
      }
      status |= switch_store::v_get(lag_handle, SWITCH_LAG_ATTR_YID, yid);
      status |=
          switch_store::v_get(lag_handle, SWITCH_LAG_ATTR_LEARNING, learning);
      if (!port_qos_config_precedence) {
        status |=
            switch_store::v_get(lag_handle, SWITCH_LAG_ATTR_COLOR, _color);
        color = static_cast<uint8_t>(_color.enumdata);
        if (color == SWITCH_LAG_ATTR_COLOR_RED) color = SWITCH_PKT_COLOR_RED;
        status |= switch_store::v_get(lag_handle, SWITCH_LAG_ATTR_TC, tc);
      }
    }

    status |= match_key.set_exact(
        smi_id::F_INGRESS_PORT_MAPPING_LOCAL_MD_INGRESS_PORT,
        parent,
        SWITCH_PORT_ATTR_DEV_PORT);

    if (e.enumdata == SWITCH_PORT_ATTR_TYPE_CPU) {
      if (feature::is_feature_set(SWITCH_FEATURE_CPU_BD_MAP)) {
        status |= match_key.set_exact(
            smi_id::F_INGRESS_PORT_MAPPING_HDR_CPU_VALID, true);
      }
      // status |= match_key.set_exact(
      //     smi_id::F_INGRESS_PORT_MAPPING_HDR_CPU_INGRESS_PORT,
      //     parent,
      //     SWITCH_PORT_ATTR_DEV_PORT);
      action_entry.init_action_data(smi_id::A_SET_CPU_PORT_PROPERTIES);
      status |= action_entry.set_arg(
          smi_id::P_SET_CPU_PORT_PROPERTIES_PORT_LAG_INDEX, port_lag_index);
      status |= action_entry.set_arg(
          smi_id::P_SET_CPU_PORT_PROPERTIES_EXCLUSION_ID, yid);
      status |= action_entry.set_arg(
          smi_id::P_SET_CPU_PORT_PROPERTIES_PORT_LAG_LABEL, port_lag_label);
      status |=
          action_entry.set_arg(smi_id::P_SET_CPU_PORT_PROPERTIES_COLOR, color);
      status |= action_entry.set_arg(smi_id::P_SET_CPU_PORT_PROPERTIES_TC, tc);
    } else {
      action_entry.init_action_data(smi_id::A_SET_PORT_PROPERTIES);
      status |=
          action_entry.set_arg(smi_id::P_SET_PORT_PROPERTIES_EXCLUSION_ID, yid);

      status |= action_entry.set_arg(
          smi_id::P_SET_PORT_PROPERTIES_LEARNING_MODE, learning);
      status |=
          action_entry.set_arg(smi_id::P_SET_PORT_PROPERTIES_COLOR, color);
      status |= action_entry.set_arg(smi_id::P_SET_PORT_PROPERTIES_TC, tc);
      status |=
          action_entry.set_arg(smi_id::P_SET_PORT_PROPERTIES_SFLOW_SESSION_ID,
                               compute_sflow_id(parent));

      if (feature::is_feature_set(SWITCH_FEATURE_IN_PORTS_IN_DATA) ||
          feature::is_feature_set(SWITCH_FEATURE_IN_PORTS_IN_MIRROR)) {
        uint32_t in_ports_group_label = 0;
        uint8_t ipv4_label = 0, ipv6_label = 0, mirror_label = 0;

        status |= switch_store::v_get(parent,
                                      SWITCH_PORT_ATTR_IN_PORTS_GROUP_LABEL,
                                      in_ports_group_label);
        ipv4_label = in_ports_group_label & 0xFF;
        ipv6_label = (in_ports_group_label >> 8) & 0xFF;
        mirror_label = (in_ports_group_label >> 16) & 0xFF;

        status |= action_entry.set_arg(
            smi_id::P_SET_PORT_PROPERTIES_IN_PORTS_GROUP_LABEL_IPV4,
            ipv4_label);
        status |= action_entry.set_arg(
            smi_id::P_SET_PORT_PROPERTIES_IN_PORTS_GROUP_LABEL_IPV6,
            ipv6_label);
        status |= action_entry.set_arg(
            smi_id::P_SET_PORT_PROPERTIES_IN_PORTS_GROUP_LABEL_MIRROR,
            mirror_label);

        switch_log(
            SWITCH_API_LEVEL_DEBUG,
            SWITCH_OBJECT_TYPE_PORT,
            "{}:{} Program port:{} in hardware with in_ports_group_label: {}",
            __func__,
            __LINE__,
            parent,
            in_ports_group_label);
      }
    }
  }
};

class egress_port_mapping : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_EGRESS_PORT_MAPPING;
  static const switch_attr_id_t status_attr_id =
      SWITCH_EGRESS_PORT_MAPPING_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_EGRESS_PORT_MAPPING_ATTR_PARENT_HANDLE;

 public:
  egress_port_mapping(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_EGRESS_PORT_MAPPING,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_enum_t enum_v = {};
    uint8_t action_port_type = 0;
    switch_eg_port_lag_label_t port_lag_label = 0;
    switch_object_id_t lag_handle = {}, egress_acl_handle = {};

    CHECK_PORT_IN_SWITCH_PIPE_AND_RETURN(parent);

    status |=
        switch_store::v_get(parent, SWITCH_PORT_ATTR_LAG_HANDLE, lag_handle);
    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_TYPE, enum_v);
    action_port_type = enum_v.enumdata;

    status |= match_key.set_exact(
        smi_id::F_EGRESS_PORT_MAPPING_PORT, parent, SWITCH_PORT_ATTR_DEV_PORT);
    if (action_port_type == SWITCH_PORT_ATTR_TYPE_NORMAL) {
      action_entry.init_action_data(smi_id::A_PORT_NORMAL);

      bool port_qos_config_precedence = false;
      status |= switch_store::v_get(parent,
                                    SWITCH_PORT_ATTR_QOS_CONFIG_PRECEDENCE,
                                    port_qos_config_precedence);
      if (lag_handle.data != 0) {
        status |= switch_store::v_get(
            lag_handle, SWITCH_LAG_ATTR_EGRESS_ACL_HANDLE, egress_acl_handle);
        status |= action_entry.set_arg(smi_id::P_PORT_NORMAL_PORT_LAG_INDEX,
                                       compute_port_lag_index(lag_handle));
        if (egress_acl_handle.data != 0) {
          port_lag_label = compute_bind_label(lag_handle, egress_acl_handle);
          status |= action_entry.set_arg(smi_id::P_PORT_NORMAL_PORT_LAG_LABEL,
                                         port_lag_label);
        } else {
          status |= action_entry.set_arg(smi_id::P_PORT_NORMAL_PORT_LAG_LABEL,
                                         lag_handle,
                                         SWITCH_LAG_ATTR_EGRESS_PORT_LAG_LABEL);
        }
      } else {
        status |= switch_store::v_get(
            parent, SWITCH_PORT_ATTR_EGRESS_ACL_HANDLE, egress_acl_handle);
        status |= action_entry.set_arg(smi_id::P_PORT_NORMAL_PORT_LAG_INDEX,
                                       compute_port_lag_index(parent));
        if (egress_acl_handle.data != 0) {
          port_lag_label = compute_bind_label(parent, egress_acl_handle);
          status |= action_entry.set_arg(smi_id::P_PORT_NORMAL_PORT_LAG_LABEL,
                                         port_lag_label);
        } else {
          status |=
              action_entry.set_arg(smi_id::P_PORT_NORMAL_PORT_LAG_LABEL,
                                   parent,
                                   SWITCH_PORT_ATTR_EGRESS_PORT_LAG_LABEL);
        }
      }

      if (feature::is_feature_set(SWITCH_FEATURE_OUT_PORTS)) {
        uint16_t out_ports_group_label = 0;
        uint8_t ipv4_label = 0, ipv6_label = 0;

        status |= switch_store::v_get(parent,
                                      SWITCH_PORT_ATTR_OUT_PORTS_GROUP_LABEL,
                                      out_ports_group_label);
        ipv4_label = out_ports_group_label & 0xFF;
        ipv6_label = (out_ports_group_label >> 8) & 0xFF;

        status |= action_entry.set_arg(
            smi_id::P_PORT_NORMAL_OUT_PORTS_GROUP_LABEL_IPV4, ipv4_label);
        status |= action_entry.set_arg(
            smi_id::P_PORT_NORMAL_OUT_PORTS_GROUP_LABEL_IPV6, ipv6_label);

        switch_log(
            SWITCH_API_LEVEL_DEBUG,
            SWITCH_OBJECT_TYPE_PORT,
            "{}:{} Program port:{} in hardware with out_ports_group_label: {}",
            __func__,
            __LINE__,
            parent,
            out_ports_group_label);
      }
    } else if (action_port_type == SWITCH_PORT_ATTR_TYPE_CPU) {
      if (smi_id::A_PORT_CPU) {
        action_entry.init_action_data(smi_id::A_PORT_CPU);
      }
    }
  }
};

class egress_ingress_port_mapping : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_EGRESS_INGRESS_PORT_MAPPING;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_EGRESS_INGRESS_PORT_MAPPING_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_EGRESS_INGRESS_PORT_MAPPING_ATTR_STATUS;

 public:
  egress_ingress_port_mapping(const switch_object_id_t parent,
                              switch_status_t &status,
                              switch_object_id_t port_isolation_group,
                              switch_object_id_t bport_isolation_group)
      : p4_object_match_action(smi_id::T_EGRESS_EGRESS_INGRESS_PORT_MAPPING,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    status |= match_key.set_exact(
        smi_id::F_EGRESS_EGRESS_INGRESS_PORT_MAPPING_LOCAL_MD_INGRESS_PORT,
        parent,
        SWITCH_PORT_ATTR_DEV_PORT);

    action_entry.init_action_data(smi_id::A_SET_EGRESS_INGRESS_PORT_PROPERTIES);

    status |= action_entry.set_arg(
        smi_id::D_SET_INGRESS_PORT_PROPERTIES_PORT_ISOLATION_GROUP,
        port_isolation_group);
    status |= action_entry.set_arg(
        smi_id::D_SET_INGRESS_PORT_PROPERTIES_BRIDGE_PORT_ISOLATION_GROUP,
        bport_isolation_group);
  }
};

class ingress_port_state_eg_1 : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_INGRESS_PORT_STATE_EG_1;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INGRESS_PORT_STATE_EG_1_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_INGRESS_PORT_STATE_EG_1_ATTR_STATUS;

  switch_object_id_t lag_handle = {0}, ingress_acl_handle = {0},
                     mirror_handle = {0}, meter_handle = {0};
  switch_ig_port_lag_label_t port_lag_label = 0;
  uint16_t port_lag_index = 0, yid = 0, dev_port = 0;
  switch_enum_t e = {0};

 public:
  ingress_port_state_eg_1(const switch_object_id_t parent,
                          switch_status_t &status)
      : p4_object_match_action(smi_id::T_INGRESS_PORT_STATE_EG_1,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    if (feature::is_feature_set(SWITCH_FEATURE_FOLDED_SWITCH_PIPELINE)) {
      status |=
          switch_store::v_get(parent, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
      dev_port = dev_port ^ 0x0080;
      status |= match_key.set_exact(
          smi_id::F_INGRESS_PORT_STATE_EG_1_INGRESS_PORT, dev_port);

      if (switch_store::object_type_query(parent) != SWITCH_OBJECT_TYPE_PORT) {
        switch_log(
            SWITCH_API_LEVEL_ERROR,
            SWITCH_OBJECT_TYPE_RIF,
            "{}:{}: Cannot set port properties for {}. Invalid object type.",
            parent.data,
            __func__,
            __LINE__);
        status |= SWITCH_STATUS_INVALID_PARAMETER;
      }

      status |=
          switch_store::v_get(parent, SWITCH_PORT_ATTR_LAG_HANDLE, lag_handle);

      if (lag_handle.data == 0) {
        status |= switch_store::v_get(
            parent, SWITCH_PORT_ATTR_INGRESS_ACL_HANDLE, ingress_acl_handle);

        if (ingress_acl_handle.data == 0) {
          status |= switch_store::v_get(
              parent, SWITCH_PORT_ATTR_INGRESS_PORT_LAG_LABEL, port_lag_label);
        } else {
          port_lag_label = compute_bind_label(parent, ingress_acl_handle);
        }

        status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_YID, yid);
        port_lag_index = compute_port_lag_index(parent);
      } else {
        status |= switch_store::v_get(
            lag_handle, SWITCH_LAG_ATTR_INGRESS_ACL_HANDLE, ingress_acl_handle);

        if (ingress_acl_handle.data == 0) {
          status |= switch_store::v_get(lag_handle,
                                        SWITCH_LAG_ATTR_INGRESS_PORT_LAG_LABEL,
                                        port_lag_label);
        } else {
          port_lag_label = compute_bind_label(lag_handle, ingress_acl_handle);
        }

        status |= switch_store::v_get(lag_handle, SWITCH_LAG_ATTR_YID, yid);
        port_lag_index = compute_port_lag_index(lag_handle);
      }

      status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_TYPE, e);
      if (e.enumdata == SWITCH_PORT_ATTR_TYPE_RECIRC) {
        // In folded pipeline, cpu-tx packets arrive on the recirc port in the
        // internal pipe
        action_entry.init_action_data(
            smi_id::A_INGRESS_PORT_STATE_EG_1_SET_CPU_PORT_PROPERTIES);
      } else {
        action_entry.init_action_data(
            smi_id::A_INGRESS_PORT_STATE_EG_1_SET_PORT_PROPERTIES);
      }

      status |= action_entry.set_arg(
          smi_id::P_INGRESS_PORT_STATE_EG_1_SET_PORT_PROPERTIES_PORT_LAG_INDEX,
          port_lag_index);

      status |= action_entry.set_arg(
          smi_id::P_INGRESS_PORT_STATE_EG_1_SET_PORT_PROPERTIES_PORT_LAG_LABEL,
          port_lag_label);
    } else {
      clear_attrs();
      return;
    }
  }
};

class ingress_port_state_ig_1 : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_INGRESS_PORT_STATE_IG_1;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INGRESS_PORT_STATE_IG_1_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_INGRESS_PORT_STATE_IG_1_ATTR_STATUS;
  switch_object_id_t lag_handle = {0}, ingress_acl_handle = {0},
                     mirror_handle = {0}, meter_handle = {0};
  switch_ig_port_lag_label_t port_lag_label = 0;
  uint16_t port_lag_index = 0, yid = 0, dev_port = 0;
  switch_enum_t e = {0};

 public:
  ingress_port_state_ig_1(const switch_object_id_t parent,
                          switch_status_t &status)
      : p4_object_match_action(smi_id::T_INGRESS_PORT_STATE_IG_1,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    if (feature::is_feature_set(SWITCH_FEATURE_FOLDED_SWITCH_PIPELINE)) {
      status |=
          switch_store::v_get(parent, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
      dev_port = dev_port ^ 0x0080;
      status |= match_key.set_exact(
          smi_id::F_INGRESS_PORT_STATE_IG_1_INGRESS_PORT, dev_port);

      if (switch_store::object_type_query(parent) != SWITCH_OBJECT_TYPE_PORT) {
        switch_log(
            SWITCH_API_LEVEL_ERROR,
            SWITCH_OBJECT_TYPE_RIF,
            "{}:{}: Cannot set port properties for {}. Invalid object type.",
            parent.data,
            __func__,
            __LINE__);
        status |= SWITCH_STATUS_INVALID_PARAMETER;
      }

      status |=
          switch_store::v_get(parent, SWITCH_PORT_ATTR_LAG_HANDLE, lag_handle);

      if (lag_handle.data == 0) {
        status |= switch_store::v_get(
            parent, SWITCH_PORT_ATTR_INGRESS_ACL_HANDLE, ingress_acl_handle);

        if (ingress_acl_handle.data == 0) {
          status |= switch_store::v_get(
              parent, SWITCH_PORT_ATTR_INGRESS_PORT_LAG_LABEL, port_lag_label);
        } else {
          port_lag_label = compute_bind_label(parent, ingress_acl_handle);
        }

        status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_YID, yid);
        port_lag_index = compute_port_lag_index(parent);
      } else {
        status |= switch_store::v_get(
            lag_handle, SWITCH_LAG_ATTR_INGRESS_ACL_HANDLE, ingress_acl_handle);

        if (ingress_acl_handle.data == 0) {
          status |= switch_store::v_get(lag_handle,
                                        SWITCH_LAG_ATTR_INGRESS_PORT_LAG_LABEL,
                                        port_lag_label);
        } else {
          port_lag_label = compute_bind_label(lag_handle, ingress_acl_handle);
        }

        status |= switch_store::v_get(lag_handle, SWITCH_LAG_ATTR_YID, yid);
        port_lag_index = compute_port_lag_index(lag_handle);
      }

      status |= switch_store::v_get(
          parent, SWITCH_PORT_ATTR_INGRESS_MIRROR_HANDLE, mirror_handle);
      if (mirror_handle.data != 0) {
        status |= switch_store::v_get(
            mirror_handle, SWITCH_MIRROR_ATTR_METER_HANDLE, meter_handle);
      }

      status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_TYPE, e);
      if (e.enumdata == SWITCH_PORT_ATTR_TYPE_RECIRC) {
        // In folded pipeline, cpu-tx packets arrive on the recirc port in the
        // internal pipe
        action_entry.init_action_data(
            smi_id::A_INGRESS_PORT_STATE_IG_1_SET_CPU_PORT_PROPERTIES);
      } else {
        action_entry.init_action_data(
            smi_id::A_INGRESS_PORT_STATE_IG_1_SET_PORT_PROPERTIES);
      }

      status |= action_entry.set_arg(
          smi_id::P_INGRESS_PORT_STATE_IG_1_SET_PORT_PROPERTIES_PORT_LAG_INDEX,
          port_lag_index);

      status |= action_entry.set_arg(
          smi_id::P_INGRESS_PORT_STATE_IG_1_SET_PORT_PROPERTIES_PORT_LAG_LABEL,
          port_lag_label);

      status |= action_entry.set_arg(
          smi_id::P_INGRESS_PORT_STATE_IG_1_SET_PORT_PROPERTIES_EXCLUSION_ID,
          yid);

      status |= action_entry.set_arg(
          smi_id::
              P_INGRESS_PORT_STATE_IG_1_SET_PORT_PROPERTIES_SFLOW_SESSION_ID,
          compute_sflow_id(parent));

      status |= action_entry.set_arg(
          smi_id::P_INGRESS_PORT_STATE_IG_1_SET_PORT_PROPERTIES_METER_INDEX,
          meter_handle);
    } else {
      clear_attrs();
      return;
    }
  }
};

class isolation_group_helper : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_ISOLATION_GROUP_HELPER;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_ISOLATION_GROUP_HELPER_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_ISOLATION_GROUP_HELPER_ATTR_STATUS;

  switch_object_id_t bridge_port = {SWITCH_NULL_OBJECT_ID};
  switch_object_type_t parent_type = {SWITCH_OBJECT_TYPE_NONE};
  std::set<switch_object_id_t> ports;

  /**
   * Get isolation group for port/bridge_port
   *
   * @param[in] handle port/bridge_port
   *
   * @param[out] group isolation group handle
   */
  switch_status_t get_isolation_group(switch_object_id_t handle,
                                      switch_object_id_t &group) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_attr_id_t attr = {SWITCH_PORT_ATTR_ISOLATION_GROUP_HANDLE};

    if (handle.data == SWITCH_NULL_OBJECT_ID) {
      group.data = SWITCH_NULL_OBJECT_ID;
      return status;
    }

    if (switch_store::object_type_query(handle) ==
        SWITCH_OBJECT_TYPE_BRIDGE_PORT) {
      attr = {SWITCH_BRIDGE_PORT_ATTR_ISOLATION_GROUP_HANDLE};
    }

    status |= switch_store::v_get(handle, attr, group);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_LAG_MEMBER,
                 "{}:{}: failed to get isolation group handle from {} - {}, "
                 "status - {}",
                 __func__,
                 __LINE__,
                 switch_store::object_type_query(handle),
                 handle,
                 status);
      return status;
    }
    return status;
  }

  /**
   * Get bridge port for port/lag
   *
   * @param[in] port_lag port/lag
   *
   * @param[out] bridge_port bridge port
   */
  switch_status_t get_bridge_port(switch_object_id_t port_lag,
                                  switch_object_id_t &bport) {
    bport = {SWITCH_NULL_OBJECT_ID};
    std::set<switch_object_id_t> bridge_ports;

    if (port_lag.data == SWITCH_NULL_OBJECT_ID) {
      bport.data = SWITCH_NULL_OBJECT_ID;
      return SWITCH_STATUS_SUCCESS;
    }

    switch_store::referencing_set_get(
        port_lag, SWITCH_OBJECT_TYPE_BRIDGE_PORT, bridge_ports);
    if (bridge_ports.size() > 1) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 switch_store::object_type_query(port_lag),
                 "{}:{} More then one ({}) bridge port is created over this "
                 "port/lag - {}",
                 __func__,
                 __LINE__,
                 bridge_ports.size(),
                 port_lag);
      return SWITCH_STATUS_INVALID_PARAMETER;
    }

    if (bridge_ports.size() == 1) {
      bport = *bridge_ports.begin();
    }

    return SWITCH_STATUS_SUCCESS;
  }

  /**
   * Convert port to port_lag
   *
   * If port is a member of lag then return lag handle, else do nothing
   *
   * @param[inout] port_lag port handle
   *
   */
  switch_status_t port_to_port_lag(switch_object_id_t &port_lag) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (port_lag.data == SWITCH_NULL_OBJECT_ID) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 parent_type,
                 "{}:{} Invalid object id - {}",
                 __func__,
                 __LINE__,
                 port_lag);
      return SWITCH_STATUS_INVALID_PARAMETER;
    }

    switch_object_id_t lag = {SWITCH_NULL_OBJECT_ID};
    status |= switch_store::v_get(port_lag, SWITCH_PORT_ATTR_LAG_HANDLE, lag);
    if (lag.data != SWITCH_NULL_OBJECT_ID) {
      port_lag = lag;
    }

    return status;
  }

 public:
  isolation_group_helper(const switch_object_id_t parent,
                         switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    parent_type = switch_store::object_type_query(parent);
    if (parent_type == SWITCH_OBJECT_TYPE_PORT) {
      // Bridge port could be created over parent port object directly
      // or over lag object, which has this parent port as lag_member.
      // So the first try to find if this port is lag member.
      // Then if YES get the bridge port created over lag which has this port as
      // lag_member, if NO try to find the bridge port created over this port
      // object
      switch_object_id_t port_lag = parent;
      status |= port_to_port_lag(port_lag);
      status |= get_bridge_port(port_lag, bridge_port);
      ports.insert(parent);
      return;
    }

    if (parent_type == SWITCH_OBJECT_TYPE_BRIDGE_PORT) {
      switch_enum_t type = {.enumdata = SWITCH_BRIDGE_PORT_ATTR_TYPE_MAX};
      status |= switch_store::v_get(parent, SWITCH_BRIDGE_PORT_ATTR_TYPE, type);
      if (type.enumdata != SWITCH_BRIDGE_PORT_ATTR_TYPE_PORT) {
        return;
      }

      // get ports over which bridge port is created
      // in case of LAG it could be more then one port
      bridge_port = parent;
      switch_object_id_t port_lag = {SWITCH_NULL_OBJECT_ID};
      std::vector<switch_object_id_t> lag_members;

      status |= switch_store::v_get(
          parent, SWITCH_BRIDGE_PORT_ATTR_PORT_LAG_HANDLE, port_lag);

      if (switch_store::object_type_query(port_lag) == SWITCH_OBJECT_TYPE_LAG) {
        status |= switch_store::v_get(
            port_lag, SWITCH_LAG_ATTR_LAG_MEMBERS, lag_members);
        for (auto const mbr : lag_members) {
          status |= switch_store::v_get(
              mbr, SWITCH_LAG_MEMBER_ATTR_PORT_HANDLE, port_lag);
          ports.insert(port_lag);
        }
      } else {
        ports.insert(port_lag);
      }
      return;
    }

    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ISOLATION_GROUP_HELPER,
               "{}:{}: Unkown parent type {}",
               __func__,
               __LINE__,
               parent_type);
    status = SWITCH_STATUS_INVALID_PARAMETER;
  }

  switch_status_t create_update_port() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_status_t obj_status = SWITCH_STATUS_SUCCESS;
    switch_object_id_t port_igroup = {SWITCH_NULL_OBJECT_ID};
    switch_object_id_t bridge_port_igroup = {SWITCH_NULL_OBJECT_ID};

    status |= get_isolation_group(*ports.begin(), port_igroup);
    status |= get_isolation_group(bridge_port, bridge_port_igroup);

    std::unique_ptr<object> ei_port_mapping =
        std::unique_ptr<egress_ingress_port_mapping>(
            new egress_ingress_port_mapping(
                *ports.begin(), obj_status, port_igroup, bridge_port_igroup));

    status |= obj_status;
    status |= ei_port_mapping->create_update();
    return status;
  }

  switch_status_t create_update_bp_port() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_object_id_t port_igroup = {SWITCH_NULL_OBJECT_ID};
    switch_object_id_t bridge_port_igroup = {SWITCH_NULL_OBJECT_ID};

    status |= get_isolation_group(bridge_port, bridge_port_igroup);

    for (auto const port : ports) {
      switch_status_t obj_status = SWITCH_STATUS_SUCCESS;
      status |= get_isolation_group(port, port_igroup);
      std::unique_ptr<object> ei_port_mapping =
          std::unique_ptr<egress_ingress_port_mapping>(
              new egress_ingress_port_mapping(
                  port, obj_status, port_igroup, bridge_port_igroup));

      status |= obj_status;
      status |= ei_port_mapping->create_update();
    }

    return status;
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (parent_type == SWITCH_OBJECT_TYPE_PORT) {
      status |= create_update_port();
    } else {
      status |= create_update_bp_port();
    }

    status |= auto_object::create_update();
    return status;
  }

  switch_status_t del() {
    switch_status_t obj_status = SWITCH_STATUS_SUCCESS;
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (parent_type == SWITCH_OBJECT_TYPE_PORT) {
      std::unique_ptr<object> ei_port_mapping =
          std::unique_ptr<egress_ingress_port_mapping>(
              new egress_ingress_port_mapping(*ports.begin(),
                                              obj_status,
                                              {SWITCH_NULL_OBJECT_ID},
                                              {SWITCH_NULL_OBJECT_ID}));

      status |= obj_status;
      status |= ei_port_mapping->del();
    }

    status |= auto_object::del();
    return status;
  }
};

class egress_port_isolation : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_EGRESS_PORT_ISOLATION;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_EGRESS_PORT_ISOLATION_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_EGRESS_PORT_ISOLATION_ATTR_STATUS;

 public:
  egress_port_isolation(const switch_object_id_t parent,
                        switch_status_t &status)
      : p4_object_match_action(smi_id::T_EGRESS_EGRESS_PORT_ISOLATION,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_object_id_t port_handle = {};
    status |= switch_store::v_get(
        parent, SWITCH_ISOLATION_GROUP_MEMBER_ATTR_HANDLE, port_handle);

    if (switch_store::object_type_query(port_handle) !=
        SWITCH_OBJECT_TYPE_PORT) {
      return;
    }

    status |= match_key.set_exact(
        smi_id::F_EGRESS_EGRESS_PORT_ISOLATION_EG_INTR_MD_EGRESS_PORT,
        port_handle,
        SWITCH_PORT_ATTR_DEV_PORT);
    status |= match_key.set_exact(
        smi_id::F_EGRESS_EGRESS_PORT_ISOLATION_LOCAL_MD_PORT_ISOLATION_GROUP,
        parent,
        SWITCH_ISOLATION_GROUP_MEMBER_ATTR_ISOLATION_GROUP_HANDLE);

    action_entry.init_action_data(smi_id::A_ISOLATE_PACKET_PORT);
    status |= action_entry.set_arg(smi_id::D_ISOLATE_PACKET_PORT_DROP, true);
  }
};

class egress_bridge_port_isolation : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_EGRESS_BRIDGE_PORT_ISOLATION;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_EGRESS_BRIDGE_PORT_ISOLATION_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_EGRESS_BRIDGE_PORT_ISOLATION_ATTR_STATUS;

 public:
  egress_bridge_port_isolation(const switch_object_id_t parent,
                               switch_status_t &status)
      : p4_object_match_action_list(
            smi_id::T_EGRESS_EGRESS_BRIDGE_PORT_ISOLATION,
            status_attr_id,
            auto_ot,
            parent_attr_id,
            parent) {
    switch_object_id_t port_lag = {SWITCH_NULL_OBJECT_ID};
    switch_object_id_t port_bport = {SWITCH_NULL_OBJECT_ID};
    std::set<switch_object_id_t> ports;

    status |= switch_store::v_get(
        parent, SWITCH_ISOLATION_GROUP_MEMBER_ATTR_HANDLE, port_bport);

    if (switch_store::object_type_query(port_bport) !=
        SWITCH_OBJECT_TYPE_BRIDGE_PORT) {
      clear_attrs();
      return;
    }

    switch_enum_t type = {.enumdata = SWITCH_BRIDGE_PORT_ATTR_TYPE_MAX};
    status |=
        switch_store::v_get(port_bport, SWITCH_BRIDGE_PORT_ATTR_TYPE, type);
    if (type.enumdata != SWITCH_BRIDGE_PORT_ATTR_TYPE_PORT) {
      clear_attrs();
      return;
    }

    // get ports over which bridge port is created
    // in case of LAG it could be more then one port
    status |= switch_store::v_get(
        port_bport, SWITCH_BRIDGE_PORT_ATTR_PORT_LAG_HANDLE, port_lag);
    if (switch_store::object_type_query(port_lag) == SWITCH_OBJECT_TYPE_LAG) {
      std::vector<switch_object_id_t> lag_members;
      status |= switch_store::v_get(
          port_lag, SWITCH_LAG_ATTR_LAG_MEMBERS, lag_members);
      for (auto const mbr : lag_members) {
        status |= switch_store::v_get(
            mbr, SWITCH_LAG_MEMBER_ATTR_PORT_HANDLE, port_lag);
        ports.insert(port_lag);
      }
    } else {
      ports.insert(port_lag);
    }

    auto it = match_action_list.begin();
    for (auto const port : ports) {
      it = match_action_list.insert(
          it,
          std::pair<_MatchKey, _ActionEntry>(
              _MatchKey(smi_id::T_EGRESS_EGRESS_BRIDGE_PORT_ISOLATION),
              _ActionEntry(smi_id::T_EGRESS_EGRESS_BRIDGE_PORT_ISOLATION)));

      status |= it->first.set_exact(
          smi_id::F_EGRESS_EGRESS_BRIDGE_PORT_ISOLATION_EG_INTR_MD_EGRESS_PORT,
          port,
          SWITCH_PORT_ATTR_DEV_PORT);

      status |= it->first.set_exact(
          smi_id::F_EGRESS_EGRESS_BRIDGE_PORT_ISOLATION_LOCAL_MD_FLAGS_ROUTED,
          false);

      status |= it->first.set_exact(
          smi_id::
              F_EGRESS_EGRESS_BRIDGE_PORT_ISOLATION_LOCAL_MD_BRIDGE_PORT_ISOLATION_GROUP,
          parent,
          SWITCH_ISOLATION_GROUP_MEMBER_ATTR_ISOLATION_GROUP_HANDLE);

      it->second.init_action_data(smi_id::A_ISOLATE_PACKET_BPORT);
      status |= it->second.set_arg(smi_id::D_ISOLATE_PACKET_BPORT_DROP, true);
    }
  }
};

/* This class is not registered with the s3 factory.
 * Specifically, this means that this class is not created
 * whenever the parent isolation_group is created or deleted.
 * Instead, all creation / update / delete events
 * are triggered by peer_link_tunnel_isolation_helper.
 */
class peer_link_tunnel_isolation : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_PEER_LINK_TUNNEL_ISOLATION;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_PEER_LINK_TUNNEL_ISOLATION_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_PEER_LINK_TUNNEL_ISOLATION_ATTR_STATUS;

 public:
  peer_link_tunnel_isolation(const switch_object_id_t parent,
                             switch_status_t &status)
      : p4_object_match_action(smi_id::T_EGRESS_PEER_LINK_TUNNEL_ISOLATION,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    status |= match_key.set_exact(
        smi_id::
            F_EGRESS_PEER_LINK_TUNNEL_ISOLATION_LOCAL_MD_BPORT_ISOLATION_GROUP,
        parent);

    action_entry.init_action_data(smi_id::A_PEER_LINK_ISOLATE);
    status |= action_entry.set_arg(smi_id::D_PEER_LINK_ISOLATE_DROP, true);
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_object_id_t isolation_group = {SWITCH_NULL_OBJECT_ID};

    status |= switch_store::v_get(
        get_auto_oid(),
        SWITCH_PEER_LINK_TUNNEL_ISOLATION_ATTR_PARENT_HANDLE,
        isolation_group);

    std::set<switch_object_id_t> bridge_ports;
    status |= switch_store::referencing_set_get(
        isolation_group, SWITCH_OBJECT_TYPE_BRIDGE_PORT, bridge_ports);
    for (auto bp : bridge_ports) {
      bool is_peer_link = false;
      status |= switch_store::v_get(
          bp, SWITCH_BRIDGE_PORT_ATTR_IS_PEER_LINK, is_peer_link);
      if (!is_peer_link) {
        bridge_ports.erase(bp);
      }
    }

    if (bridge_ports.size() > 1) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_BRIDGE_PORT,
                 "{}:{} peer_link_tunnel_isolation del skip. More than one "
                 "bridge_port with is_peer_link==true reference the parent "
                 "isolation_group",
                 __func__,
                 __LINE__);
      return status;
    }

    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(
          SWITCH_API_LEVEL_ERROR,
          SWITCH_OBJECT_TYPE_BRIDGE_PORT,
          "{}:{} peer_link_tunnel_isolation construction failed, status {}",
          __func__,
          __LINE__,
          switch_error_to_string(status));
    }

    status |= p4_object_match_action::del();
    return status;
  }
};

class peer_link_tunnel_isolation_helper : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_PEER_LINK_TUNNEL_ISOLATION_HELPER;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_PEER_LINK_TUNNEL_ISOLATION_HELPER_ATTR_PARENT_HANDLE;
  switch_object_id_t isolation_group = {SWITCH_NULL_OBJECT_ID};
  switch_object_id_t peer_link_tunnel_isolation_handle = {
      SWITCH_NULL_OBJECT_ID};
  bool is_peer_link = false;

 public:
  peer_link_tunnel_isolation_helper(const switch_object_id_t parent,
                                    switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    if (!feature::is_feature_set(SWITCH_FEATURE_PEER_LINK_TUNNEL_ISOLATION)) {
      clear_attrs();
      return;
    }

    if (switch_store::object_type_query(parent) !=
        SWITCH_OBJECT_TYPE_BRIDGE_PORT) {
      clear_attrs();
      return;
    }

    switch_enum_t type = {.enumdata = SWITCH_BRIDGE_PORT_ATTR_TYPE_MAX};
    status |= switch_store::v_get(parent, SWITCH_BRIDGE_PORT_ATTR_TYPE, type);

    if (type.enumdata != SWITCH_BRIDGE_PORT_ATTR_TYPE_PORT) {
      clear_attrs();
      return;
    }

    status |= switch_store::v_get(
        parent, SWITCH_BRIDGE_PORT_ATTR_IS_PEER_LINK, is_peer_link);

    status |=
        switch_store::v_get(parent,
                            SWITCH_BRIDGE_PORT_ATTR_ISOLATION_GROUP_HANDLE,
                            isolation_group);

    if (get_auto_oid() != 0) {
      status |= switch_store::v_get(
          get_auto_oid(),
          SWITCH_PEER_LINK_TUNNEL_ISOLATION_HELPER_ATTR_PEER_LINK_TUNNEL_ISOLATION_HANDLE,
          peer_link_tunnel_isolation_handle);
    }
  }

  switch_status_t del_peer_link_tunnel_isolation(switch_object_id_t parent) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_object_id_t null_oid = {SWITCH_NULL_OBJECT_ID};

    auto obj = std::unique_ptr<peer_link_tunnel_isolation>(
        new peer_link_tunnel_isolation(parent, status));
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(
          SWITCH_API_LEVEL_ERROR,
          SWITCH_OBJECT_TYPE_BRIDGE_PORT,
          "{}:{} peer_link_tunnel_isolation construction failed, status {}",
          __func__,
          __LINE__,
          switch_error_to_string(status));
    }

    status |= switch_store::v_set(
        get_auto_oid(),
        SWITCH_PEER_LINK_TUNNEL_ISOLATION_HELPER_ATTR_PEER_LINK_TUNNEL_ISOLATION_HANDLE,
        null_oid);

    status |= obj->del();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_BRIDGE_PORT,
                 "{}:{} del for peer_link_tunnel_isolation failed, status {}",
                 __func__,
                 __LINE__,
                 switch_error_to_string(status));
    }
    return status;
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_status_t status_tmp = SWITCH_STATUS_SUCCESS;

    if (is_peer_link && isolation_group.data != 0) {
      if (peer_link_tunnel_isolation_handle.data != 0) {
        switch_object_id_t prev_isolation_group = {SWITCH_NULL_OBJECT_ID};
        status |= switch_store::v_get(
            peer_link_tunnel_isolation_handle,
            SWITCH_PEER_LINK_TUNNEL_ISOLATION_ATTR_PARENT_HANDLE,
            prev_isolation_group);

        if (prev_isolation_group.data == isolation_group.data) {
          return status;
        }

        del_peer_link_tunnel_isolation(prev_isolation_group);
      }

      auto obj = std::unique_ptr<peer_link_tunnel_isolation>(
          new peer_link_tunnel_isolation(isolation_group, status_tmp));

      status |= status_tmp;
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(
            SWITCH_API_LEVEL_ERROR,
            SWITCH_OBJECT_TYPE_BRIDGE_PORT,
            "{}:{} peer_link_tunnel_isolation construction failed, status {}",
            __func__,
            __LINE__,
            switch_error_to_string(status));
      }

      status |= obj->create_update();
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_BRIDGE_PORT,
                   "{}:{} create_update for peer_link_tunnel_isolation failed, "
                   "status {}",
                   __func__,
                   __LINE__,
                   switch_error_to_string(status));
      }

      attrs.insert(attr_w(
          SWITCH_PEER_LINK_TUNNEL_ISOLATION_HELPER_ATTR_PEER_LINK_TUNNEL_ISOLATION_HANDLE,
          obj->get_auto_oid()));

    } else if (get_auto_oid() != 0 &&
               peer_link_tunnel_isolation_handle.data != 0) {
      status |= switch_store::v_get(
          peer_link_tunnel_isolation_handle,
          SWITCH_PEER_LINK_TUNNEL_ISOLATION_ATTR_PARENT_HANDLE,
          isolation_group);

      del_peer_link_tunnel_isolation(isolation_group);
    }

    status |= auto_object::create_update();
    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (peer_link_tunnel_isolation_handle.data != 0) {
      switch_status_t status_tmp = SWITCH_STATUS_SUCCESS;
      status |= switch_store::v_get(
          peer_link_tunnel_isolation_handle,
          SWITCH_PEER_LINK_TUNNEL_ISOLATION_ATTR_PARENT_HANDLE,
          isolation_group);
      auto obj = std::unique_ptr<peer_link_tunnel_isolation>(
          new peer_link_tunnel_isolation(isolation_group, status_tmp));
      status |= status_tmp;
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(
            SWITCH_API_LEVEL_ERROR,
            SWITCH_OBJECT_TYPE_BRIDGE_PORT,
            "{}:{} peer_link_tunnel_isolation construction failed, status {}",
            __func__,
            __LINE__,
            switch_error_to_string(status));
      }

      status |= obj->del();
    }

    status |= auto_object::del();
    return status;
  }
};

class lag_helper : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_LAG_HELPER;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_LAG_HELPER_ATTR_PARENT_HANDLE;
  std::vector<switch_object_id_t> lag_members;

 public:
  lag_helper(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status |=
        switch_store::v_get(parent, SWITCH_LAG_ATTR_LAG_MEMBERS, lag_members);
    for (auto const mbr : lag_members) {
      switch_object_id_t mbr_port_handle = {0};
      status |= switch_store::v_get(
          mbr, SWITCH_LAG_MEMBER_ATTR_PORT_HANDLE, mbr_port_handle);
      ingress_port_mapping ipm(mbr_port_handle, status);
      status |= ipm.create_update();
      egress_port_mapping epm(mbr_port_handle, status);
      status |= epm.create_update();
      port_metadata pm(mbr_port_handle, status);
      status |= pm.create_update();
      ingress_port_state_ig_1 ipsi_1(mbr_port_handle, status);
      status |= ipsi_1.create_update();
      ingress_port_state_eg_1 ipse_1(mbr_port_handle, status);
      status |= ipse_1.create_update();
    }
  }
  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    (void)handle;
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    for (uint64_t i = 0; i < SWITCH_LAG_COUNTER_ID_MAX; i++) {
      cntrs[i].counter_id = i;
      cntrs[i].count = 0;
    }

    for (auto const mbr : lag_members) {
      switch_object_id_t mbr_port_handle = {0};
      std::vector<switch_counter_t> port_cntrs;
      status |= switch_store::v_get(
          mbr, SWITCH_LAG_MEMBER_ATTR_PORT_HANDLE, mbr_port_handle);
      status |= switch_store::object_counters_get(mbr_port_handle, port_cntrs);
      cntrs[SWITCH_LAG_COUNTER_ID_IN_OCTETS].count +=
          port_cntrs[SWITCH_PORT_COUNTER_ID_IN_ALL_OCTETS].count;
      cntrs[SWITCH_LAG_COUNTER_ID_OUT_OCTETS].count +=
          port_cntrs[SWITCH_PORT_COUNTER_ID_OUT_ALL_OCTETS].count;
      cntrs[SWITCH_LAG_COUNTER_ID_IN_PACKETS].count +=
          port_cntrs[SWITCH_PORT_COUNTER_ID_IN_ALL_PKTS].count;
      cntrs[SWITCH_LAG_COUNTER_ID_OUT_PACKETS].count +=
          port_cntrs[SWITCH_PORT_COUNTER_ID_OUT_ALL_PKTS].count;
    }
    return status;
  }
};

/**
 * A BD object is modeled as below
 *
 *   -------    --------    ----------    -------
 *   | vrf |    | vlan |    | bridge |    | rif |
 *   -------    --------    ----------    -------
 *       \          \            /           /
 *                    --------
 *                    |  bd  |
 *                    --------
 *
 * Changes to BD object or it's parent affects
 * - bd_action_profile
 * - bd_flood
 * - egress_bd_mapping
 * - vlan_to_bd_mapping
 *
 * BD gets assigned for a vlan, vrf and bridge unconditionally
 * For a RIF, a new BD is assigned only if it is an L3 interface
 * For an SVI and bridge RIF, the BD is derived from the vlan
 * and bridge respectively
 */
class bd : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_BD;
  static const switch_attr_id_t parent_attr_id = SWITCH_BD_ATTR_PARENT_HANDLE;

 public:
  bd(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    switch_enum_t rif_type = {};

    const auto parent_type = switch_store::object_type_query(parent);
    bool is_virtual = false;

    if (parent_type == SWITCH_OBJECT_TYPE_RIF) {
      status |=
          switch_store::v_get(parent, SWITCH_RIF_ATTR_IS_VIRTUAL, is_virtual);
      status |= switch_store::v_get(parent, SWITCH_RIF_ATTR_TYPE, rif_type);
      if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_LOOPBACK ||
          rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_VLAN || is_virtual) {
        attrs.clear();
        return;
      }
    } else if (parent_type == SWITCH_OBJECT_TYPE_BRIDGE) {
      switch_enum_t bridge_type;
      status |=
          switch_store::v_get(parent, SWITCH_BRIDGE_ATTR_TYPE, bridge_type);
      if (bridge_type.enumdata == SWITCH_BRIDGE_ATTR_TYPE_DOT1Q) {
        attrs.clear();
        return;
      }
    }
  }
};

/**
 * A BD member object is modeled as below
 *
 *   -------    ---------------  --------
 *   | rif |    | vlan_member |  | intf |
 *   -------    ---------------  --------
 *       \             /            /
 *              ---------------
 *              |  bd_member  |
 *              ---------------
 *
 * Changes to BD member object or it's parent affects
 * - port_vlan_to_bd_mapping
 * - cpu_to_bd_mapping
 * - mc_node
 * - rid_table
 *
 * If RIF is L3 interface
 *   - get it's own BD
 * else if RIF is SVI
 *   - get BD from vlan
 * else if RIF is bridge sub port
 *   - get BD from bridge
 */
class bd_member : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_BD_MEMBER;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_BD_MEMBER_ATTR_PARENT_HANDLE;

 public:
  std::unique_ptr<bd_member> bd_member_override;
  bd_member(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    const switch_object_type_t parent_type =
        switch_store::object_type_query(parent);
    uint16_t vlan_id = 0;
    switch_object_id_t vlan_handle = {0}, bd_handle = {0}, handle = {0};
    switch_enum_t rif_type = {0};

    /* we need a bd_member for every vlan member */
    if (parent_type == SWITCH_OBJECT_TYPE_VLAN_MEMBER) {
      status |= switch_store::v_get(
          parent, SWITCH_VLAN_MEMBER_ATTR_MEMBER_HANDLE, handle);
      status |= switch_store::v_get(
          parent, SWITCH_VLAN_MEMBER_ATTR_VLAN_HANDLE, vlan_handle);
      status |=
          switch_store::v_get(vlan_handle, SWITCH_VLAN_ATTR_VLAN_ID, vlan_id);
      status |= find_auto_oid(vlan_handle, SWITCH_OBJECT_TYPE_BD, bd_handle);
    } else if (parent_type == SWITCH_OBJECT_TYPE_RIF) {
      status |= switch_store::v_get(parent, SWITCH_RIF_ATTR_TYPE, rif_type);
      status |=
          switch_store::v_get(parent, SWITCH_RIF_ATTR_PORT_HANDLE, handle);
      bool is_virtual = false;
      status |=
          switch_store::v_get(parent, SWITCH_RIF_ATTR_IS_VIRTUAL, is_virtual);
      // return for loopback rif
      if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_LOOPBACK) {
        attrs.clear();
        return;
      }

      if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_VLAN) {
        /* BD not required for SVI, vlan's BD is used */
        attrs.clear();
        return;
      }

      if (is_virtual) {
        attrs.clear();
        return;
      }

      if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_SUB_PORT) {
        status |=
            switch_store::v_get(parent, SWITCH_RIF_ATTR_OUTER_VLAN_ID, vlan_id);
      }

      // if RIF attached to port/lag, and BD exist, generate bd member
      status |= find_auto_oid(parent, SWITCH_OBJECT_TYPE_BD, bd_handle);
      if (bd_handle == 0) {
        /* The framework will give it a try again after BD becomes available
         */
        attrs.clear();
        status = SWITCH_STATUS_DEPENDENCY_FAILURE;
        return;
      }
    } else {
      return;
    }

    attrs.insert(attr_w(SWITCH_BD_MEMBER_ATTR_HANDLE, handle.data));
    attrs.insert(attr_w(SWITCH_BD_MEMBER_ATTR_VLAN_ID, vlan_id));
    attrs.insert(attr_w(SWITCH_BD_MEMBER_ATTR_PARENT_TYPE, parent_type));
  }
};

switch_status_t set_bd_state_properties_for_rif(
    switch_object_id_t bd_parent_rif, _ActionEntry &action_entry) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_enum_t vrf_ttl_action;
  switch_enum_t vrf_ip_options_action;
  switch_packet_action_t sw_packet_action = 0;
  bool sw_packet_action_is_valid = false;
  bool ipv4_unicast = true, ipv6_unicast = true;
  switch_bd_label_t bd_label = 0;
  switch_object_id_t vrf_handle = {0}, ingress_acl_handle = {0};
  bool vrf_ucast, rif_ucast;
  uint8_t nat_zone = 0;

  switch_object_type_t parent_ot =
      switch_store::object_type_query(bd_parent_rif);
  if (parent_ot != SWITCH_OBJECT_TYPE_RIF) {
    switch_log(
        SWITCH_API_LEVEL_ERROR,
        SWITCH_OBJECT_TYPE_RIF,
        "{}:{}: Cannot set bd properties for {}. Invalid object type {}.",
        __func__,
        __LINE__,
        bd_parent_rif.data,
        switch_store::object_name_get_from_type(parent_ot));
    return SWITCH_STATUS_INVALID_PARAMETER;
  }

  status |= switch_store::v_get(
      bd_parent_rif, SWITCH_RIF_ATTR_INGRESS_ACL_HANDLE, ingress_acl_handle);
  if (ingress_acl_handle.data != 0) {
    bd_label = compute_bind_label(bd_parent_rif, ingress_acl_handle);
  } else {
    status |= switch_store::v_get(
        bd_parent_rif, SWITCH_RIF_ATTR_INGRESS_VLAN_RIF_LABEL, bd_label);
  }
  status |= action_entry.set_arg(
      smi_id::P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_BD_LABEL, bd_label);

  status |= switch_store::v_get(
      bd_parent_rif, SWITCH_RIF_ATTR_VRF_HANDLE, vrf_handle);

  status |= action_entry.set_arg(
      smi_id::P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_VRF,
      compute_vrf(vrf_handle));
  status |=
      switch_store::v_get(vrf_handle, SWITCH_VRF_ATTR_IPV4_UNICAST, vrf_ucast);
  status |= switch_store::v_get(
      bd_parent_rif, SWITCH_RIF_ATTR_IPV4_UNICAST, rif_ucast);
  ipv4_unicast = vrf_ucast && rif_ucast;
  status |= action_entry.set_arg(
      smi_id::P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_IPV4_UNICAST_ENABLE,
      ipv4_unicast);

  status |=
      switch_store::v_get(vrf_handle, SWITCH_VRF_ATTR_IPV6_UNICAST, vrf_ucast);
  status |= switch_store::v_get(
      bd_parent_rif, SWITCH_RIF_ATTR_IPV6_UNICAST, rif_ucast);
  ipv6_unicast = vrf_ucast && rif_ucast;
  status |= action_entry.set_arg(
      smi_id::P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_IPV6_UNICAST_ENABLE,
      ipv6_unicast);

  status |= action_entry.set_arg(
      smi_id::P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_IPV4_MULTICAST_ENABLE,
      bd_parent_rif,
      SWITCH_RIF_ATTR_IPV4_MULTICAST);

  status |= action_entry.set_arg(
      smi_id::P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_IPV6_MULTICAST_ENABLE,
      bd_parent_rif,
      SWITCH_RIF_ATTR_IPV6_MULTICAST);

  status |= action_entry.set_arg(
      smi_id::P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_MPLS_ENABLE,
      bd_parent_rif,
      SWITCH_RIF_ATTR_MPLS_STATE);

  status |= switch_store::v_get(
      vrf_handle, SWITCH_VRF_ATTR_TTL_ACTION, vrf_ttl_action);
  status |= vrf_ttl_action_to_switch_packet_action(
      vrf_ttl_action, sw_packet_action, sw_packet_action_is_valid);
  if (sw_packet_action_is_valid) {
    status |= action_entry.set_arg(
        smi_id::P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_VRF_TTL_VIOLATION,
        sw_packet_action);
    status |= action_entry.set_arg(
        smi_id::
            P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_VRF_TTL_VIOLATION_VALID,
        true);
  }

  status |= switch_store::v_get(
      vrf_handle, SWITCH_VRF_ATTR_IP_OPTIONS_ACTION, vrf_ip_options_action);
  status |= vrf_ip_options_action_to_switch_packet_action(vrf_ip_options_action,
                                                          sw_packet_action);
  status |= action_entry.set_arg(
      smi_id::
          P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_VRF_IP_OPTIONS_VIOLATION,
      sw_packet_action);

  switch_store::v_get(bd_parent_rif, SWITCH_RIF_ATTR_NAT_ZONE, nat_zone);
  // SONiC supports only 2 zones: inside and outside
  if (nat_zone > SWITCH_RIF_OUTSIDE_NAT_ZONE_ID) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_RIF,
               "{}:{}: Incorrect NAT zone",
               __func__,
               __LINE__);
  }
  if (feature::is_feature_set(SWITCH_FEATURE_NAT)) {
    status |= action_entry.set_arg(
        smi_id::P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_NAT_ZONE, nat_zone);
  }

  return status;
}

switch_status_t set_bd_state_properties_for_vlan(
    switch_object_id_t bd_parent_vlan, _ActionEntry &action_entry) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  bool ipv4_unicast = false, ipv6_unicast = false;
  switch_bd_label_t bd_label = 0;
  switch_object_id_t ingress_acl_handle = {0};

  switch_object_type_t parent_ot =
      switch_store::object_type_query(bd_parent_vlan);
  if (parent_ot != SWITCH_OBJECT_TYPE_VLAN) {
    switch_log(
        SWITCH_API_LEVEL_ERROR,
        SWITCH_OBJECT_TYPE_VLAN,
        "{}:{}: Cannot set bd properties for {}. Invalid object type {}.",
        __func__,
        __LINE__,
        bd_parent_vlan.data,
        switch_store::object_name_get_from_type(parent_ot));
    return SWITCH_STATUS_INVALID_PARAMETER;
  }

  status |= action_entry.set_arg(
      smi_id::P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_IPV4_UNICAST_ENABLE,
      ipv4_unicast);

  status |= action_entry.set_arg(
      smi_id::P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_IPV6_UNICAST_ENABLE,
      ipv6_unicast);

  status |= switch_store::v_get(
      bd_parent_vlan, SWITCH_VLAN_ATTR_INGRESS_ACL_HANDLE, ingress_acl_handle);
  if (ingress_acl_handle.data != 0) {
    bd_label = compute_bind_label(bd_parent_vlan, ingress_acl_handle);
  } else {
    status |= switch_store::v_get(
        bd_parent_vlan, SWITCH_VLAN_ATTR_INGRESS_VLAN_RIF_LABEL, bd_label);
  }
  status |= action_entry.set_arg(
      smi_id::P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_BD_LABEL, bd_label);

  status |= action_entry.set_arg(
      smi_id::P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_IPV4_MULTICAST_ENABLE,
      bd_parent_vlan,
      SWITCH_VLAN_ATTR_IPV4_MULTICAST);

  status |= action_entry.set_arg(
      smi_id::P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_IPV6_MULTICAST_ENABLE,
      bd_parent_vlan,
      SWITCH_VLAN_ATTR_IPV6_MULTICAST);

  status |= action_entry.set_arg(
      smi_id::P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_IGMP_SNOOPING_ENABLE,
      bd_parent_vlan,
      SWITCH_VLAN_ATTR_IGMP_SNOOPING);

  status |= action_entry.set_arg(
      smi_id::P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_MLD_SNOOPING_ENABLE,
      bd_parent_vlan,
      SWITCH_VLAN_ATTR_MLD_SNOOPING);

  status |= action_entry.set_arg(
      smi_id::P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_VLAN_ARP_SUPPRESS,
      bd_parent_vlan,
      SWITCH_VLAN_ATTR_ARP_SUPPRESS_ENABLE);

  switch_object_id_t rif_handle = {0};
  std::vector<switch_object_id_t> rif_handles;
  status |= switch_store::v_get(
      bd_parent_vlan, SWITCH_VLAN_ATTR_RIF_HANDLES, rif_handles);
  if (rif_handles.size() > 0) {
    rif_handle = rif_handles[0];
    status |= set_bd_state_properties_for_rif(rif_handle, action_entry);
  }
  return status;
}

switch_status_t set_bd_state_properties_for_vrf(
    switch_object_id_t bd_parent_vrf,
    _ActionEntry &action_entry,
    switch_object_type_t caller_ot) {
  switch_enum_t vrf_ttl_action;
  switch_enum_t vrf_ip_options_action;
  switch_packet_action_t sw_packet_action = 0;
  bool sw_packet_action_is_valid = false;
  std::set<switch_object_id_t> vrf_rifs;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  status |= switch_store::v_get(
      bd_parent_vrf, SWITCH_VRF_ATTR_TTL_ACTION, vrf_ttl_action);
  status |= switch_store::v_get(
      bd_parent_vrf, SWITCH_VRF_ATTR_IP_OPTIONS_ACTION, vrf_ip_options_action);

  switch_object_type_t parent_ot =
      switch_store::object_type_query(bd_parent_vrf);
  if (parent_ot != SWITCH_OBJECT_TYPE_VRF) {
    switch_log(
        SWITCH_API_LEVEL_ERROR,
        SWITCH_OBJECT_TYPE_VRF,
        "{}:{}: Cannot set bd properties for {}. Invalid object type {}.",
        __func__,
        __LINE__,
        bd_parent_vrf.data,
        switch_store::object_name_get_from_type(parent_ot));
    return SWITCH_STATUS_INVALID_PARAMETER;
  }

  status |= switch_store::referencing_set_get(
      bd_parent_vrf, SWITCH_OBJECT_TYPE_RIF, vrf_rifs);
  for (auto rif_handle : vrf_rifs) {
    switch_object_id_t bd_handle = {};
    switch_status_t rif_status = get_bd_for_object(rif_handle, bd_handle);

    if (rif_status != SWITCH_STATUS_SUCCESS) {
      status |= rif_status;
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_VRF,
                 "{}:{} Error getting bd for rif.{}",
                 __func__,
                 __LINE__,
                 switch_store::handle_to_id(rif_handle));
      continue;
    }

    if (!bd_handle.data) {
      switch_log(SWITCH_API_LEVEL_WARN,
                 SWITCH_OBJECT_TYPE_VRF,
                 "{}:{} Error getting bd for rif.{}",
                 __func__,
                 __LINE__,
                 switch_store::handle_to_id(rif_handle));
      continue;
    }

    auto rif = factory::get_instance().create(caller_ot, bd_handle, status);

    if (rif) {
      rif->create_update();
    }
  }

  status |= action_entry.set_arg(
      smi_id::P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_VRF,
      compute_vrf(bd_parent_vrf));
  status |= vrf_ttl_action_to_switch_packet_action(
      vrf_ttl_action, sw_packet_action, sw_packet_action_is_valid);
  status |= action_entry.set_arg(
      smi_id::P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_IPV4_UNICAST_ENABLE,
      bd_parent_vrf,
      SWITCH_VRF_ATTR_IPV4_UNICAST);
  status |= action_entry.set_arg(
      smi_id::P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_IPV6_UNICAST_ENABLE,
      bd_parent_vrf,
      SWITCH_VRF_ATTR_IPV6_UNICAST);

  if (sw_packet_action_is_valid) {
    status |= action_entry.set_arg(
        smi_id::P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_VRF_TTL_VIOLATION,
        sw_packet_action);
    status |= action_entry.set_arg(
        smi_id::
            P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_VRF_TTL_VIOLATION_VALID,
        true);
  }

  status |= vrf_ip_options_action_to_switch_packet_action(vrf_ip_options_action,
                                                          sw_packet_action);
  status |= action_entry.set_arg(
      smi_id::
          P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_VRF_IP_OPTIONS_VIOLATION,
      sw_packet_action);

  return status;
}

/*
class ingress_bd_state_eg_1 : public p4_object_match_action {
private:
static const switch_object_type_t auto_ot =
    SWITCH_OBJECT_TYPE_INGRESS_BD_STATE_EG_1;
static const switch_attr_id_t parent_attr_id =
    SWITCH_INGRESS_BD_STATE_EG_1_ATTR_PARENT_HANDLE;
static const switch_attr_id_t status_attr_id =
    SWITCH_INGRESS_BD_STATE_EG_1_ATTR_STATUS;

public:
ingress_bd_state_eg_1(const switch_object_id_t parent,
                      switch_status_t &status)
    : p4_object_match_action(smi_id::T_INGRESS_BD_STATE_EG_1,
                             status_attr_id,
                             auto_ot,
                             parent_attr_id,
                             parent) {
  switch_object_id_t bd_parent = {0};
  switch_object_type_t bd_parent_ot = 0;

  if (!feature::is_feature_set(SWITCH_FEATURE_FOLDED_SWITCH_PIPELINE)) {
    clear_attrs();
    return;
  }

  status |= match_key.set_exact(smi_id::F_INGRESS_BD_STATE_EG_1_BD,
                                compute_bd(parent));
  status |=
      switch_store::v_get(parent, SWITCH_BD_ATTR_PARENT_HANDLE, bd_parent);
  bd_parent_ot = switch_store::object_type_query(bd_parent);

  action_entry.init_action_data(
      smi_id::A_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES);

  if (bd_parent_ot == SWITCH_OBJECT_TYPE_VLAN) {
    status |= set_bd_state_properties_for_vlan(bd_parent, action_entry);
  } else if (bd_parent_ot == SWITCH_OBJECT_TYPE_RIF) {
    status |= set_bd_state_properties_for_rif(bd_parent, action_entry);
  } else if (bd_parent_ot == SWITCH_OBJECT_TYPE_VRF) {
    status |=
        set_bd_state_properties_for_vrf(bd_parent, action_entry, auto_ot);
  }
}
};
*/
class ingress_bd_state_ig_1 : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_INGRESS_BD_STATE_IG_1;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INGRESS_BD_STATE_IG_1_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_INGRESS_BD_STATE_IG_1_ATTR_STATUS;

 public:
  ingress_bd_state_ig_1(const switch_object_id_t parent,
                        switch_status_t &status)
      : p4_object_match_action(smi_id::T_INGRESS_BD_STATE_IG_1,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_object_id_t bd_parent = {0};
    switch_object_type_t bd_parent_ot = 0;

    if (!feature::is_feature_set(SWITCH_FEATURE_FOLDED_SWITCH_PIPELINE)) {
      clear_attrs();
      return;
    }

    status |= match_key.set_exact(smi_id::F_INGRESS_BD_STATE_IG_1_BD,
                                  compute_bd(parent));
    status |=
        switch_store::v_get(parent, SWITCH_BD_ATTR_PARENT_HANDLE, bd_parent);
    bd_parent_ot = switch_store::object_type_query(bd_parent);

    action_entry.init_action_data(
        smi_id::A_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES);

    if (bd_parent_ot == SWITCH_OBJECT_TYPE_VLAN) {
      status |= set_bd_state_properties_for_vlan(bd_parent, action_entry);
    } else if (bd_parent_ot == SWITCH_OBJECT_TYPE_RIF) {
      status |= set_bd_state_properties_for_rif(bd_parent, action_entry);
    } else if (bd_parent_ot == SWITCH_OBJECT_TYPE_VRF) {
      status |=
          set_bd_state_properties_for_vrf(bd_parent, action_entry, auto_ot);
    }
  }
};

/*
 * BD_ACTION_PROFILE p4 object
 */
class bd_action_profile : public p4_object_action_selector {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_BD_ACTION_PROFILE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_BD_ACTION_PROFILE_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_BD_ACTION_PROFILE_ATTR_PARENT_HANDLE;

 public:
  bd_action_profile(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_action_selector(smi_id::AP_BD_ACTION_PROFILE,
                                  smi_id::F_BD_ACTION_PROFILE_ACTION_MEMBER_ID,
                                  status_attr_id,
                                  auto_ot,
                                  parent_attr_id,
                                  parent) {
    switch_object_id_t user_handle = {0};
    switch_object_type_t ot = 0;

    status |= match_key.set_exact(smi_id::F_BD_ACTION_PROFILE_ACTION_MEMBER_ID,
                                  parent);
    action_entry.init_action_data(smi_id::A_INGRESS_SET_BD_PROPERTIES);
    status |= action_entry.set_arg(smi_id::P_INGRESS_SET_BD_PROPERTIES_BD,
                                   compute_bd(parent));
    status |= action_entry.set_arg(
        smi_id::P_INGRESS_SET_BD_PROPERTIES_MRPF_GROUP, compute_bd(parent));

    status |=
        switch_store::v_get(parent, SWITCH_BD_ATTR_PARENT_HANDLE, user_handle);
    ot = switch_store::object_type_query(user_handle);
    if (ot == SWITCH_OBJECT_TYPE_VLAN) {
      status |= setup_bd_for_vlan(parent, user_handle);
    } else if (ot == SWITCH_OBJECT_TYPE_RIF) {
      status |= setup_bd_for_rif(parent, user_handle);
    } else if (ot == SWITCH_OBJECT_TYPE_VRF) {
      status |= setup_bd_for_vrf(parent, user_handle);
    }
  }

  switch_status_t setup_bd_for_vlan(switch_object_id_t bd,
                                    switch_object_id_t vlan_handle) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_object_id_t ingress_acl_handle = {0};
    switch_bd_label_t vlan_rif_label = 0;

    // compute the bd label
    status |= switch_store::v_get(
        vlan_handle, SWITCH_VLAN_ATTR_INGRESS_ACL_HANDLE, ingress_acl_handle);
    if (ingress_acl_handle.data != 0) {
      vlan_rif_label = compute_bind_label(vlan_handle, ingress_acl_handle);
    } else {
      status |= switch_store::v_get(
          vlan_handle, SWITCH_VLAN_ATTR_INGRESS_VLAN_RIF_LABEL, vlan_rif_label);
    }
    status |= action_entry.set_arg(smi_id::P_INGRESS_SET_BD_PROPERTIES_BD_LABEL,
                                   vlan_rif_label);

    status |=
        action_entry.set_arg(smi_id::P_INGRESS_SET_BD_PROPERTIES_STP_GROUP,
                             vlan_handle,
                             SWITCH_VLAN_ATTR_STP_HANDLE);
    status |=
        action_entry.set_arg(smi_id::P_INGRESS_SET_BD_PROPERTIES_LEARNING_MODE,
                             vlan_handle,
                             SWITCH_VLAN_ATTR_LEARNING);
    status |= action_entry.set_arg(
        smi_id::P_INGRESS_SET_BD_PROPERTIES_IGMP_SNOOPING_ENABLE,
        vlan_handle,
        SWITCH_VLAN_ATTR_IGMP_SNOOPING);
    status |= action_entry.set_arg(
        smi_id::P_INGRESS_SET_BD_PROPERTIES_MLD_SNOOPING_ENABLE,
        vlan_handle,
        SWITCH_VLAN_ATTR_MLD_SNOOPING);
    status |= action_entry.set_arg(
        smi_id::P_INGRESS_SET_BD_PROPERTIES_IPV4_MULTICAST_ENABLE,
        vlan_handle,
        SWITCH_VLAN_ATTR_IPV4_MULTICAST);
    status |= action_entry.set_arg(
        smi_id::P_INGRESS_SET_BD_PROPERTIES_IPV6_MULTICAST_ENABLE,
        vlan_handle,
        SWITCH_VLAN_ATTR_IPV6_MULTICAST);
    status |= action_entry.set_arg(
        smi_id::P_INGRESS_SET_BD_PROPERTIES_IPV4_UNICAST_ENABLE, false);
    status |= action_entry.set_arg(
        smi_id::P_INGRESS_SET_BD_PROPERTIES_IPV6_UNICAST_ENABLE, false);
    status |= action_entry.set_arg(
        smi_id::P_INGRESS_SET_BD_PROPERTIES_VLAN_ARP_SUPPRESS_ENABLE,
        vlan_handle,
        SWITCH_VLAN_ATTR_ARP_SUPPRESS_ENABLE);

    // Update the bd entry if this is an update for a RIF
    switch_object_id_t rif_handle = {0};
    std::vector<switch_object_id_t> rif_handles;
    status |= switch_store::v_get(
        vlan_handle, SWITCH_VLAN_ATTR_RIF_HANDLES, rif_handles);
    if (rif_handles.size() > 0) {
      rif_handle = rif_handles[0];
      status |= setup_bd_for_rif(bd, rif_handle);
    }
    return status;
  }

  switch_status_t setup_bd_for_rif(switch_object_id_t bd,
                                   switch_object_id_t rif_handle) {
    (void)bd;
    switch_enum_t vrf_ttl_action;
    switch_enum_t vrf_ip_options_action;
    switch_enum_t vrf_unknown_l3_mcast_action;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_object_id_t ingress_acl_handle = {0};
    switch_bd_label_t vlan_rif_label = 0;
    bool vrf_ipv4_unicast = false, vrf_ipv6_unicast = false;
    bool rif_ipv4_unicast = false, rif_ipv6_unicast = false;
    switch_packet_action_t sw_packet_action = 0;
    bool sw_packet_action_is_valid = false;

    if (rif_handle.data == 0) return status;

    // compute the bd label
    status |= switch_store::v_get(
        rif_handle, SWITCH_RIF_ATTR_INGRESS_ACL_HANDLE, ingress_acl_handle);
    if (ingress_acl_handle.data != 0) {
      vlan_rif_label = compute_bind_label(rif_handle, ingress_acl_handle);
    } else {
      status |= switch_store::v_get(
          rif_handle, SWITCH_RIF_ATTR_INGRESS_VLAN_RIF_LABEL, vlan_rif_label);
    }
    status |= action_entry.set_arg(smi_id::P_INGRESS_SET_BD_PROPERTIES_BD_LABEL,
                                   vlan_rif_label);

    switch_object_id_t vrf_handle = {};
    status |=
        switch_store::v_get(rif_handle, SWITCH_RIF_ATTR_VRF_HANDLE, vrf_handle);
    status |= action_entry.set_arg(smi_id::P_INGRESS_SET_BD_PROPERTIES_VRF,
                                   compute_vrf(vrf_handle));

    status |= switch_store::v_get(
        vrf_handle, SWITCH_VRF_ATTR_TTL_ACTION, vrf_ttl_action);
    status |= vrf_ttl_action_to_switch_packet_action(
        vrf_ttl_action, sw_packet_action, sw_packet_action_is_valid);
    if (sw_packet_action_is_valid) {
      status |= action_entry.set_arg(
          smi_id::P_INGRESS_SET_BD_PROPERTIES_VRF_TTL_VIOLATION,
          sw_packet_action);
      status |= action_entry.set_arg(
          smi_id::P_INGRESS_SET_BD_PROPERTIES_VRF_TTL_VIOLATION_VALID, true);
    }

    status |= switch_store::v_get(
        vrf_handle, SWITCH_VRF_ATTR_IP_OPTIONS_ACTION, vrf_ip_options_action);
    status |= vrf_ip_options_action_to_switch_packet_action(
        vrf_ip_options_action, sw_packet_action);
    status |= action_entry.set_arg(
        smi_id::P_INGRESS_SET_BD_PROPERTIES_VRF_IP_OPTIONS_VIOLATION,
        sw_packet_action);

    status |= switch_store::v_get(vrf_handle,
                                  SWITCH_VRF_ATTR_UNKNOWN_L3_MCAST_ACTION,
                                  vrf_unknown_l3_mcast_action);
    status |= vrf_unknown_l3_mcast_action_to_switch_packet_action(
        vrf_unknown_l3_mcast_action, sw_packet_action);
    status |= action_entry.set_arg(
        smi_id::P_INGRESS_SET_BD_PROPERTIES_VRF_UNKNOWN_L3_MULTICAST_TRAP,
        sw_packet_action == SWITCH_PACKET_ACTION_TRAP);

    status |= switch_store::v_get(
        vrf_handle, SWITCH_VRF_ATTR_IPV4_UNICAST, vrf_ipv4_unicast);
    status |= switch_store::v_get(
        vrf_handle, SWITCH_VRF_ATTR_IPV6_UNICAST, vrf_ipv6_unicast);
    status |= switch_store::v_get(
        rif_handle, SWITCH_RIF_ATTR_IPV4_UNICAST, rif_ipv4_unicast);
    status |= switch_store::v_get(
        rif_handle, SWITCH_RIF_ATTR_IPV6_UNICAST, rif_ipv6_unicast);
    status |= action_entry.set_arg(
        smi_id::P_INGRESS_SET_BD_PROPERTIES_IPV4_UNICAST_ENABLE,
        vrf_ipv4_unicast && rif_ipv4_unicast);
    status |= action_entry.set_arg(
        smi_id::P_INGRESS_SET_BD_PROPERTIES_IPV6_UNICAST_ENABLE,
        vrf_ipv6_unicast && rif_ipv6_unicast);
    status |= action_entry.set_arg(
        smi_id::P_INGRESS_SET_BD_PROPERTIES_IPV4_MULTICAST_ENABLE,
        rif_handle,
        SWITCH_RIF_ATTR_IPV4_MULTICAST);
    status |= action_entry.set_arg(
        smi_id::P_INGRESS_SET_BD_PROPERTIES_IPV6_MULTICAST_ENABLE,
        rif_handle,
        SWITCH_RIF_ATTR_IPV6_MULTICAST);
    status |=
        action_entry.set_arg(smi_id::P_INGRESS_SET_BD_PROPERTIES_MPLS_ENABLE,
                             rif_handle,
                             SWITCH_RIF_ATTR_MPLS_STATE);
    uint8_t nat_zone = 0;
    // Currently, Sonic supports only 2 zones. Inside=0 and Outside=1.
    // Packets from Inside zone are source translated and packets from
    // Outside zone are destination translated.
    switch_store::v_get(rif_handle, SWITCH_RIF_ATTR_NAT_ZONE, nat_zone);
    if (nat_zone > SWITCH_RIF_OUTSIDE_NAT_ZONE_ID) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_RIF,
                 "{}:{}: Zone ids not inside or outside, check the config",
                 __func__,
                 __LINE__);
    }
    if (feature::is_feature_set(SWITCH_FEATURE_NAT)) {
      status |= action_entry.set_arg(
          smi_id::P_INGRESS_SET_BD_PROPERTIES_NAT_ZONE, nat_zone);
    }
    return status;
  }

  switch_status_t setup_bd_for_vrf(switch_object_id_t bd,
                                   switch_object_id_t vrf_handle) {
    (void)bd;
    switch_enum_t vrf_ttl_action;
    switch_enum_t vrf_ip_options_action;
    switch_enum_t vrf_unknown_l3_mcast_action;
    switch_packet_action_t sw_packet_action = 0;
    bool sw_packet_action_is_valid = false;
    std::set<switch_object_id_t> vrf_rifs;
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    status |= switch_store::v_get(
        vrf_handle, SWITCH_VRF_ATTR_TTL_ACTION, vrf_ttl_action);
    status |= switch_store::v_get(
        vrf_handle, SWITCH_VRF_ATTR_IP_OPTIONS_ACTION, vrf_ip_options_action);
    status |= switch_store::v_get(vrf_handle,
                                  SWITCH_VRF_ATTR_UNKNOWN_L3_MCAST_ACTION,
                                  vrf_unknown_l3_mcast_action);

    status |= switch_store::referencing_set_get(
        vrf_handle, SWITCH_OBJECT_TYPE_RIF, vrf_rifs);
    for (auto rif_handle : vrf_rifs) {
      switch_object_id_t bd_handle = {};
      switch_status_t rif_status = get_bd_for_object(rif_handle, bd_handle);

      if (rif_status != SWITCH_STATUS_SUCCESS) {
        status |= rif_status;
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_VRF,
                   "{}:{} Error getting bd for rif.{}",
                   __func__,
                   __LINE__,
                   switch_store::handle_to_id(rif_handle));
        continue;
      }

      if (!bd_handle.data) {
        switch_log(SWITCH_API_LEVEL_WARN,
                   SWITCH_OBJECT_TYPE_VRF,
                   "{}:{} Error getting bd for rif.{}",
                   __func__,
                   __LINE__,
                   switch_store::handle_to_id(rif_handle));
        continue;
      }

      auto rif = factory::get_instance().create(
          SWITCH_OBJECT_TYPE_BD_ACTION_PROFILE, bd_handle, status);

      if (rif) {
        rif->create_update();
      }
    }

    status |= vrf_ttl_action_to_switch_packet_action(
        vrf_ttl_action, sw_packet_action, sw_packet_action_is_valid);
    status |= action_entry.set_arg(
        smi_id::P_INGRESS_SET_BD_PROPERTIES_IPV4_UNICAST_ENABLE,
        vrf_handle,
        SWITCH_VRF_ATTR_IPV4_UNICAST);
    status |= action_entry.set_arg(
        smi_id::P_INGRESS_SET_BD_PROPERTIES_IPV6_UNICAST_ENABLE,
        vrf_handle,
        SWITCH_VRF_ATTR_IPV6_UNICAST);
    if (sw_packet_action_is_valid) {
      status |= action_entry.set_arg(
          smi_id::P_INGRESS_SET_BD_PROPERTIES_VRF_TTL_VIOLATION,
          sw_packet_action);
      status |= action_entry.set_arg(
          smi_id::P_INGRESS_SET_BD_PROPERTIES_VRF_TTL_VIOLATION_VALID, true);
    }

    status |= vrf_ip_options_action_to_switch_packet_action(
        vrf_ip_options_action, sw_packet_action);
    status |= action_entry.set_arg(
        smi_id::P_INGRESS_SET_BD_PROPERTIES_VRF_IP_OPTIONS_VIOLATION,
        sw_packet_action);

    status |= vrf_unknown_l3_mcast_action_to_switch_packet_action(
        vrf_unknown_l3_mcast_action, sw_packet_action);
    status |= action_entry.set_arg(
        smi_id::P_INGRESS_SET_BD_PROPERTIES_VRF_UNKNOWN_L3_MULTICAST_TRAP,
        sw_packet_action == SWITCH_PACKET_ACTION_TRAP);

    return status;
  }
};

/* PORT_DOUBLE_TAG_TO_BD_MAPPING p4 object
 * This object is used for SAI QinQ RIF cases. The parent is rif user object
 */
class port_double_tag_to_bd_mapping : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_PORT_DOUBLE_TAG_TO_BD_MAPPING;
  static const switch_attr_id_t status_attr_id =
      SWITCH_PORT_DOUBLE_TAG_TO_BD_MAPPING_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_PORT_DOUBLE_TAG_TO_BD_MAPPING_ATTR_PARENT_HANDLE;

 public:
  std::unique_ptr<port_double_tag_to_bd_mapping> bd_member_override = nullptr;
  port_double_tag_to_bd_mapping(const switch_object_id_t parent,
                                switch_status_t &status)
      : p4_object_match_action(smi_id::T_PORT_DOUBLE_TAG_TO_BD_MAPPING,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_object_id_t bd_handle = {0}, bd_ap_handle = {0},
                       port_lag_handle = {0}, bd_member_parent_handle = {0};
    uint16_t outer_vlan_id = 0;
    uint16_t inner_vlan_id = 0;

    switch_enum_t rif_type = {};
    status |= switch_store::v_get(
        parent, SWITCH_BD_MEMBER_ATTR_PARENT_HANDLE, bd_member_parent_handle);
    if (switch_store::object_type_query(bd_member_parent_handle) ==
        SWITCH_OBJECT_TYPE_RIF) {
      status |= switch_store::v_get(
          bd_member_parent_handle, SWITCH_RIF_ATTR_TYPE, rif_type);
      if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_QINQ_PORT) {
        //        status |=
        //            switch_store::v_get(parent, SWITCH_PORT_ATTR_DEVICE,
        //            device_handle);
        status |= switch_store::v_get(bd_member_parent_handle,
                                      SWITCH_RIF_ATTR_OUTER_VLAN_ID,
                                      outer_vlan_id);
        status |= switch_store::v_get(bd_member_parent_handle,
                                      SWITCH_RIF_ATTR_INNER_VLAN_ID,
                                      inner_vlan_id);
        status |= switch_store::v_get(bd_member_parent_handle,
                                      SWITCH_RIF_ATTR_PORT_HANDLE,
                                      port_lag_handle);
        // Get bd first and then action profile handle from bd
        status |= get_bd_from_bd_member(parent, bd_handle);
        status |= find_auto_oid(
            bd_handle, SWITCH_OBJECT_TYPE_BD_ACTION_PROFILE, bd_ap_handle);
        if (bd_handle.data != 0) {
          status |= find_auto_oid(
              bd_handle, SWITCH_OBJECT_TYPE_BD_ACTION_PROFILE, bd_ap_handle);
        }
        if (bd_ap_handle == 0) {
          /* The framework will give it a try again after BD ap becomes
           * available */
          status = SWITCH_STATUS_DEPENDENCY_FAILURE;
          return;
        }

        status |= match_key.set_exact(
            smi_id::
                F_PORT_DOUBLE_TAG_TO_BD_MAPPING_LOCAL_MD_INGRESS_PORT_LAG_INDEX,
            compute_port_lag_index(port_lag_handle));
        status |= match_key.set_exact(
            smi_id::F_PORT_DOUBLE_TAG_TO_BD_MAPPING_VLAN_0_VID, outer_vlan_id);
        status |= match_key.set_exact(
            smi_id::F_PORT_DOUBLE_TAG_TO_BD_MAPPING_VLAN_0_VALID, true);
        status |= match_key.set_exact(
            smi_id::F_PORT_DOUBLE_TAG_TO_BD_MAPPING_VLAN_1_VID, inner_vlan_id);
        status |= match_key.set_exact(
            smi_id::F_PORT_DOUBLE_TAG_TO_BD_MAPPING_VLAN_1_VALID, true);

        status |= action_entry.init_indirect_data();
        status |= action_entry.set_arg(
            smi_id::D_PORT_DOUBLE_TAG_TO_BD_MAPPING_ACTION_MEMBER_ID,
            bd_ap_handle);
      }
    }
  }
  switch_status_t create_update() {
    if (bd_member_override != nullptr) {
      return bd_member_override->create_update();
    }
    return p4_object_match_action::create_update();
  }
  switch_status_t del() {
    // if the bd_member is being removed, we still want to keep this entry
    // in
    // hardware since the port is still present
    if (bd_member_override != nullptr) {
      return bd_member_override->create_update();
    }
    return p4_object_match_action::del();
  }
};

/* PORT_VLAN_TO_BD_MAPPING_FOR_PORT p4 object
 * The parent is port/lag/bd-member user object
 * This is common now for both sai/non-sai cases to program ingress entry
 * Usage: 2 entries per physical port or lag
 *        1 CPU port + 5 RIF ports = 10
 *        256 front ports          = 512
 * RIF updates above entries only if not supporting multiple RIFs per port
 * RIF simply updates the above 2 port entries
 * SUBPORT RIF adds 1 entry per each
 */
class port_vlan_to_bd_mapping_for_port : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_PORT_VLAN_TO_BD_MAPPING_FOR_PORT;
  static const switch_attr_id_t status_attr_id =
      SWITCH_PORT_VLAN_TO_BD_MAPPING_FOR_PORT_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_PORT_VLAN_TO_BD_MAPPING_FOR_PORT_ATTR_PARENT_HANDLE;

 public:
  std::unique_ptr<port_vlan_to_bd_mapping_for_port> rif_override = nullptr;
  port_vlan_to_bd_mapping_for_port(const switch_object_id_t parent,
                                   switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_PORT_VLAN_TO_BD_MAPPING,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    switch_object_id_t bd_handle = {0}, bd_ap_handle = {0}, device_handle = {0},
                       vlan_handle = {0}, rif_handle = {0},
                       port_lag_handle = {0};
    uint16_t vlan_id = 0;
    uint32_t action_member_id = DEFAULT_ACTION_MEMBER_ID;
    uint32_t prio = 0xFFFF;  // higher than all RIF related entries

    typedef struct entries_ {
      uint16_t vlan_id;
      uint16_t vlan_id_mask;
      uint8_t valid;
      uint8_t valid_mask;
    } entries;

    CHECK_PORT_IN_SWITCH_PIPE_AND_RETURN(parent);

    // this ingress entry req for bridge-port
    // {vlan_tag__0__vid, 0xFFF, vlan_tag__0__valid, 1}, // port, vlan,
    // valid

    std::vector<entries> rules = {
        {0, 0, 0, 1},     // port, *, 0
        {0, 0xFFF, 1, 1}  // port, 0, valid
    };
    switch_object_type_t ot = switch_store::object_type_query(parent);

    if (ot == SWITCH_OBJECT_TYPE_PORT) {
      status |=
          switch_store::v_get(parent, SWITCH_PORT_ATTR_DEVICE, device_handle);
      status |=
          switch_store::v_get(parent, SWITCH_PORT_ATTR_PORT_VLAN_ID, vlan_id);
      status |=
          switch_store::v_get(parent, SWITCH_PORT_ATTR_RIF_HANDLE, rif_handle);
      port_lag_handle = parent;
    } else if (ot == SWITCH_OBJECT_TYPE_LAG) {
      status |=
          switch_store::v_get(parent, SWITCH_LAG_ATTR_DEVICE, device_handle);
      status |=
          switch_store::v_get(parent, SWITCH_LAG_ATTR_PORT_VLAN_ID, vlan_id);
      status |=
          switch_store::v_get(parent, SWITCH_LAG_ATTR_RIF_HANDLE, rif_handle);
      port_lag_handle = parent;
    } else if (ot == SWITCH_OBJECT_TYPE_RIF) {
      // for multiple_rifs, use port_vlan_to_bd_mapping_for_rif
      if (feature::is_feature_set(SWITCH_FEATURE_MULTIPLE_RIFS_PER_PORT)) {
        clear_attrs();
        return;
      }
      bool is_virtual = false;
      status |=
          switch_store::v_get(parent, SWITCH_RIF_ATTR_IS_VIRTUAL, is_virtual);
      if (is_virtual) {
        clear_attrs();
        return;
      }
      switch_enum_t rif_type = {};
      status |= switch_store::v_get(parent, SWITCH_RIF_ATTR_TYPE, rif_type);
      if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_SUB_PORT) {
        status |=
            switch_store::v_get(parent, SWITCH_RIF_ATTR_OUTER_VLAN_ID, vlan_id);
        if (vlan_id == 0) {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_RIF,
                     "{}.{}: Invalid sub_port configuration, vlan_id {} for "
                     "rif_handle",
                     __func__,
                     __LINE__,
                     vlan_id);
          status = SWITCH_STATUS_INVALID_PARAMETER;
          return;
        }
        rules.clear();
        rules.push_back({vlan_id, 0xFFF, 1, 1});
        status |= switch_store::v_get(
            parent, SWITCH_RIF_ATTR_PORT_HANDLE, port_lag_handle);
        rif_handle = parent;
      } else if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_PORT) {
        status |= switch_store::v_get(
            parent, SWITCH_RIF_ATTR_PORT_HANDLE, port_lag_handle);
        // unlikely
        if (port_lag_handle.data == 0) return;
        std::unique_ptr<port_vlan_to_bd_mapping_for_port> p =
            std::unique_ptr<port_vlan_to_bd_mapping_for_port>(
                new port_vlan_to_bd_mapping_for_port(port_lag_handle, status));
        rif_override = std::move(p);
        return;
      } else {
        clear_attrs();
        return;
      }
    }

    if (rif_handle.data == 0) {
      // If parent is port/lag user object
      if (vlan_id != 0) {
        bool is_vlan_deletes = false;
        std::set<attr_w> lookup_attrs;
        lookup_attrs.insert(attr_w(SWITCH_VLAN_ATTR_DEVICE, device_handle));
        lookup_attrs.insert(attr_w(SWITCH_VLAN_ATTR_VLAN_ID, vlan_id));
        switch_store::object_id_get_wkey(
            SWITCH_OBJECT_TYPE_VLAN, lookup_attrs, vlan_handle);
        if (vlan_handle.data != 0) {
          status |=
              find_auto_oid(vlan_handle, SWITCH_OBJECT_TYPE_BD, bd_handle);
          status |= switch_store::v_get(
              bd_handle, SWITCH_BD_ATTR_IS_DELETING, is_vlan_deletes);
          if (is_vlan_deletes) {
            bd_handle.data = 0;
          }
        }
      }
    } else {
      // This is an L3 interface
      // If parent is bd_member user object
      status |= find_auto_oid(rif_handle, SWITCH_OBJECT_TYPE_BD, bd_handle);
    }

    if (bd_handle.data != 0) {
      status |= find_auto_oid(
          bd_handle, SWITCH_OBJECT_TYPE_BD_ACTION_PROFILE, bd_ap_handle);
    }

    // normal mode
    // no rif or vlan_member, use default
    // rif, for_bd_member is true, use rif's bd
    // vlan_member, use vlan's bd
    if (bd_ap_handle.data != 0) {
      if (rif_handle.data != 0 || vlan_handle.data != 0 || vlan_id != 0) {
        // If VLAN deletes in this default action_member_id set.
        action_member_id = static_cast<uint32_t>(bd_ap_handle.data);
      } else {
        return;
      }
    }

    auto it = match_action_list.begin();
    for (const auto entry : rules) {
      it = match_action_list.insert(
          it,
          std::pair<_MatchKey, _ActionEntry>(
              _MatchKey(smi_id::T_PORT_VLAN_TO_BD_MAPPING),
              _ActionEntry(smi_id::T_PORT_VLAN_TO_BD_MAPPING)));

      status |= it->first.set_ternary(
          smi_id::F_PORT_VLAN_TO_BD_MAPPING_LOCAL_MD_INGRESS_PORT_LAG_INDEX,
          compute_port_lag_index(port_lag_handle),
          static_cast<uint16_t>(0x3FF));

      status |=
          it->first.set_ternary(smi_id::F_PORT_VLAN_TO_BD_MAPPING_VLAN_0_VID,
                                entry.vlan_id,
                                entry.vlan_id_mask);
      status |= it->first.set_ternary(
          smi_id::F_PORT_VLAN_TO_BD_MAPPING_VLAN_0_VID_VALID,
          entry.valid,
          entry.valid_mask);
      status |=
          it->first.set_exact(smi_id::F_PORT_VLAN_TO_BD_MAPPING_PRIORITY, prio);
      status |= it->second.init_indirect_data();
      status |= it->second.set_arg(
          smi_id::D_PORT_VLAN_TO_BD_MAPPING_ACTION_MEMBER_ID, action_member_id);
    }
  }
  switch_status_t create_update() {
    if (rif_override != nullptr) {
      return rif_override->create_update();
    }
    return p4_object_match_action_list::create_update();
  }
  switch_status_t del() {
    // if the bd_member is being removed, we still want to keep this entry
    // in
    // hardware since the port is still present
    if (rif_override != nullptr) {
      return rif_override->create_update();
    }
    return p4_object_match_action_list::del();
  }
};

/* PORT_VLAN_TO_BD_MAPPING_FOR_RIF p4 object
 * The parent is rif user object
 */
class port_vlan_to_bd_mapping_for_rif : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_PORT_VLAN_TO_BD_MAPPING_FOR_RIF;
  static const switch_attr_id_t status_attr_id =
      SWITCH_PORT_VLAN_TO_BD_MAPPING_FOR_RIF_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_PORT_VLAN_TO_BD_MAPPING_FOR_RIF_ATTR_PARENT_HANDLE;

 public:
  port_vlan_to_bd_mapping_for_rif(const switch_object_id_t parent,
                                  switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_PORT_VLAN_TO_BD_MAPPING,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    switch_object_id_t bd_handle = {0}, bd_ap_handle = {0},
                       port_lag_handle = {0};
    uint16_t vlan_id = 0, vlan_id_mask = 0xFFF;
    uint8_t valid = 0, valid_mask = 1;
    switch_mac_addr_t rmac = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    switch_mac_addr_t rmac_mask = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    switch_enum_t rif_type = {};
    uint32_t prio = static_cast<uint32_t>(switch_store::handle_to_id(parent));

    // no need of these entries when not supporting multiple rifs per port
    if (!feature::is_feature_set(SWITCH_FEATURE_MULTIPLE_RIFS_PER_PORT)) {
      clear_attrs();
      return;
    }
    bool is_virtual = false;
    status |=
        switch_store::v_get(parent, SWITCH_RIF_ATTR_IS_VIRTUAL, is_virtual);
    if (is_virtual) {
      clear_attrs();
      return;
    }
    status |= switch_store::v_get(
        parent, SWITCH_RIF_ATTR_PORT_HANDLE, port_lag_handle);
    status |= switch_store::v_get(parent, SWITCH_RIF_ATTR_SRC_MAC, rmac);
    status |= switch_store::v_get(parent, SWITCH_RIF_ATTR_TYPE, rif_type);

    if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_PORT) {
      valid = 0;
    } else if (rif_type.enumdata == SWITCH_RIF_ATTR_TYPE_SUB_PORT) {
      status |=
          switch_store::v_get(parent, SWITCH_RIF_ATTR_OUTER_VLAN_ID, vlan_id);
      if (vlan_id == 0) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_RIF,
                   "{}.{}: Invalid sub_port configuration, vlan_id {} for "
                   "rif_handle",
                   __func__,
                   __LINE__,
                   vlan_id);
        status = SWITCH_STATUS_INVALID_PARAMETER;
        return;
      }
      valid = 1;
    } else {
      clear_attrs();
      return;
    }

    status |= find_auto_oid(parent, SWITCH_OBJECT_TYPE_BD, bd_handle);
    if (bd_handle.data != 0) {
      status |= find_auto_oid(
          bd_handle, SWITCH_OBJECT_TYPE_BD_ACTION_PROFILE, bd_ap_handle);
    }

    auto it = match_action_list.begin();
    for (auto set_src_mac : {true, false}) {
      it = match_action_list.insert(
          it,
          std::pair<_MatchKey, _ActionEntry>(
              _MatchKey(smi_id::T_PORT_VLAN_TO_BD_MAPPING),
              _ActionEntry(smi_id::T_PORT_VLAN_TO_BD_MAPPING)));

      status |= it->first.set_ternary(
          smi_id::F_PORT_VLAN_TO_BD_MAPPING_LOCAL_MD_INGRESS_PORT_LAG_INDEX,
          compute_port_lag_index(port_lag_handle),
          static_cast<uint16_t>(0x3FF));
      status |= it->first.set_ternary(
          smi_id::F_PORT_VLAN_TO_BD_MAPPING_VLAN_0_VID, vlan_id, vlan_id_mask);
      status |= it->first.set_ternary(
          smi_id::F_PORT_VLAN_TO_BD_MAPPING_VLAN_0_VID_VALID,
          valid,
          valid_mask);
      // allow packets to be routed even if rmac is a miss. my_mac will check
      // for rmac
      if (set_src_mac)
        status |= it->first.set_ternary(
            smi_id::F_PORT_VLAN_TO_BD_MAPPING_SRC_MAC_ADDRESS, rmac, rmac_mask);
      else
        // entry without rmac is catchall and is lower priority than all other
        // rmac entries of all other RIFs on this port
        status |= it->first.set_exact(
            smi_id::F_PORT_VLAN_TO_BD_MAPPING_PRIORITY, prio);

      status |= it->second.init_indirect_data();
      status |=
          it->second.set_arg(smi_id::D_PORT_VLAN_TO_BD_MAPPING_ACTION_MEMBER_ID,
                             static_cast<uint32_t>(bd_ap_handle.data));
    }
  }
};

/* VLAN_TO_BD_MAPPING p4 object */
class vlan_to_bd_mapping : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_VLAN_TO_BD_MAPPING;
  static const switch_attr_id_t status_attr_id =
      SWITCH_VLAN_TO_BD_MAPPING_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_VLAN_TO_BD_MAPPING_ATTR_PARENT_HANDLE;

 public:
  vlan_to_bd_mapping(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_VLAN_TO_BD_MAPPING,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_object_id_t vlan_handle = {0}, bd_ap_handle = {0};
    CHECK_PORT_IN_SWITCH_PIPE_AND_RETURN(parent);

    // Get vlan from bd
    status |=
        switch_store::v_get(parent, SWITCH_BD_ATTR_PARENT_HANDLE, vlan_handle);
    if (switch_store::object_type_query(vlan_handle) != SWITCH_OBJECT_TYPE_VLAN)
      return;

    // Get action profile handle from bd
    status |= find_auto_oid(
        parent, SWITCH_OBJECT_TYPE_BD_ACTION_PROFILE, bd_ap_handle);
    if (bd_ap_handle == 0) {
      /* The framework will give it a try again after BD ap becomes
       * available
       */
      status = SWITCH_STATUS_DEPENDENCY_FAILURE;
      return;
    }

    status |= action_entry.init_indirect_data();
    status |= action_entry.set_arg(
        smi_id::D_VLAN_TO_BD_MAPPING_ACTION_MEMBER_ID, bd_ap_handle);

    status |= match_key.set_exact(smi_id::F_VLAN_TO_BD_MAPPING_VLAN_0_VID,
                                  vlan_handle,
                                  SWITCH_VLAN_ATTR_VLAN_ID);
  }
};

/* CPU_TO_BD_MAPPING p4 object */
class cpu_to_bd_mapping : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_CPU_TO_BD_MAPPING;
  static const switch_attr_id_t status_attr_id =
      SWITCH_CPU_TO_BD_MAPPING_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_CPU_TO_BD_MAPPING_ATTR_PARENT_HANDLE;

 public:
  cpu_to_bd_mapping(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_CPU_TO_BD_MAPPING,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_object_id_t vlan_handle = {0}, bd_ap_handle = {0};

    if (!feature::is_feature_set(SWITCH_FEATURE_CPU_BD_MAP)) {
      return;
    }
    // Get vlan from bd
    status |=
        switch_store::v_get(parent, SWITCH_BD_ATTR_PARENT_HANDLE, vlan_handle);
    if (switch_store::object_type_query(vlan_handle) != SWITCH_OBJECT_TYPE_VLAN)
      return;

    // Get action profile handle from bd
    status |= find_auto_oid(
        parent, SWITCH_OBJECT_TYPE_BD_ACTION_PROFILE, bd_ap_handle);
    if (bd_ap_handle == 0) {
      /* The framework will give it a try again after BD ap becomes
       * available
       */
      status = SWITCH_STATUS_DEPENDENCY_FAILURE;
      return;
    }

    status |= action_entry.init_indirect_data();
    status |= action_entry.set_arg(smi_id::D_CPU_TO_BD_MAPPING_ACTION_MEMBER_ID,
                                   bd_ap_handle);

    status |=
        match_key.set_exact(smi_id::F_CPU_TO_BD_MAPPING_HDR_CPU_INGRESS_BD,
                            vlan_handle,
                            SWITCH_VLAN_ATTR_VLAN_ID);
  }
};

switch_status_t add_delete_pv_membership(uint16_t dev_port,
                                         uint16_t vlan_id,
                                         uint8_t valid) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  uint32_t register_index = 0;
  bf_rt_target_t dev_target =
      compute_dev_target_for_table(dev_port, smi_id::T_VLAN_MEMBERSHIP, true);

  _Table table(dev_target, get_bf_rt_info(), smi_id::T_VLAN_MEMBERSHIP);
  register_index = ((dev_port & 0x7f) << VLAN_ID_WIDTH) | vlan_id;
  {
    // entry for untagged/tagged vlan member
    _MatchKey register_key(smi_id::T_VLAN_MEMBERSHIP);
    _ActionEntry register_action(smi_id::T_VLAN_MEMBERSHIP);
    register_action.init_indirect_data();
    status |= register_key.set_exact(smi_id::F_VLAN_MEMBERSHIP_REGISTER_INDEX,
                                     register_index);
    status |=
        register_action.set_arg(smi_id::D_VLAN_MEMBERSHIP_REGISTER_DATA, valid);
    status = table.entry_modify(register_key, register_action);
  }

  {
    // entry for priority tagged packet
    register_index = ((dev_port & 0x7f) << VLAN_ID_WIDTH) | 0;
    _MatchKey register_key(smi_id::T_VLAN_MEMBERSHIP);
    _ActionEntry register_action(smi_id::T_VLAN_MEMBERSHIP);
    register_action.init_indirect_data();
    status |= register_key.set_exact(smi_id::F_VLAN_MEMBERSHIP_REGISTER_INDEX,
                                     register_index);
    status |=
        register_action.set_arg(smi_id::D_VLAN_MEMBERSHIP_REGISTER_DATA, valid);
    status = table.entry_modify(register_key, register_action);
  }

  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_PORT,
               "{}:{}: failed pv membership status {} dev_port {} vlan_id {} "
               "valid {}",
               __func__,
               __LINE__,
               status,
               dev_port,
               vlan_id,
               valid);
  }

  return status;
}

/* VLAN_MEMBERSHIP register p4 object */
class vlan_membership : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_VLAN_MEMBERSHIP;
  static const switch_attr_id_t status_attr_id =
      SWITCH_VLAN_MEMBERSHIP_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_VLAN_MEMBERSHIP_ATTR_PARENT_HANDLE;
  uint16_t vlan_id = 0;
  switch_object_id_t port_lag_handle = {0}, vlan_handle = {0},
                     vlan_member_handle = {0};
  std::vector<uint16_t> port_map;
  std::vector<uint8_t> lag_map;

 public:
  vlan_membership(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status |=
        switch_store::v_get(parent, SWITCH_BD_MEMBER_ATTR_VLAN_ID, vlan_id);

    status |= switch_store::v_get(
        parent, SWITCH_BD_MEMBER_ATTR_HANDLE, port_lag_handle.data);
    status |= switch_store::v_get(
        parent, SWITCH_BD_MEMBER_ATTR_PARENT_HANDLE, vlan_member_handle);
    if (switch_store::object_type_query(vlan_member_handle) ==
        SWITCH_OBJECT_TYPE_VLAN_MEMBER) {
      status |= switch_store::v_get(
          vlan_member_handle, SWITCH_VLAN_MEMBER_ATTR_VLAN_HANDLE, vlan_handle);
      if (feature::is_feature_set(SWITCH_FEATURE_L2_VXLAN)) {
        std::unique_ptr<object> mc_mgid_vlan_obj(factory::get_instance().create(
            SWITCH_OBJECT_TYPE_MC_MGID, vlan_handle, status));
        if (mc_mgid_vlan_obj != nullptr) {
          status |= mc_mgid_vlan_obj->create_update();
          if (status != SWITCH_STATUS_SUCCESS) {
            switch_log(
                SWITCH_API_LEVEL_ERROR,
                SWITCH_OBJECT_TYPE_MC_MGID,
                "{}: mc_mgid_vlan_obj update failed status={} for vlan {}",
                __func__,
                status,
                vlan_handle);
          }
        }
      }
      std::unique_ptr<object> mc_node_vlan_obj(factory::get_instance().create(
          SWITCH_OBJECT_TYPE_MC_NODE_VLAN, vlan_handle, status));
      if (mc_node_vlan_obj != nullptr) {
        status = mc_node_vlan_obj->create_update();
        if (status != SWITCH_STATUS_SUCCESS) {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_MC_NODE_VLAN,
                     "{}: mc_node_vlan_obj update failed status={} for vlan {}",
                     __func__,
                     status,
                     vlan_handle);
        }
      }
      if (feature::is_feature_set(SWITCH_FEATURE_MULTICAST)) {
        switch_object_id_t ipmc_node_parent = {};
        const auto &ipmc_nodes = switch_store::get_object_references(
            vlan_handle, SWITCH_OBJECT_TYPE_IPMC_NODE);

        for (auto ipmc_node : ipmc_nodes) {
          status |= switch_store::v_get(ipmc_node.oid,
                                        SWITCH_IPMC_NODE_ATTR_PARENT_HANDLE,
                                        ipmc_node_parent);

          std::unique_ptr<object> ipmc_node_obj(factory::get_instance().create(
              SWITCH_OBJECT_TYPE_IPMC_NODE, ipmc_node_parent, status));
          if (ipmc_node_obj != nullptr) {
            status |= ipmc_node_obj->create_update();
            if (status != SWITCH_STATUS_SUCCESS) {
              switch_log(
                  SWITCH_API_LEVEL_ERROR,
                  SWITCH_OBJECT_TYPE_IPMC_NODE,
                  "{}: ipmc_node_obj update failed status={} for vlan {}",
                  __func__,
                  status,
                  vlan_handle);
            }
          }
        }
      }
    }
  }

  // Loop through all the LAG ports (if lag) and update pv registers
  // If a port gets added later, lag_membership will ensure it gets added
  // to the pv table
  switch_status_t update_pv_membership(uint8_t valid) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    std::set<uint16_t> dev_port_list;
    switch_object_type_t ot = switch_store::object_type_query(port_lag_handle);

    if (ot == SWITCH_OBJECT_TYPE_PORT) {
      uint16_t dev_port = 0;
      status |= switch_store::v_get(
          port_lag_handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
      dev_port_list.insert(dev_port);
    } else if (ot == SWITCH_OBJECT_TYPE_LAG) {
      std::vector<switch_object_id_t> lag_members;
      uint16_t dev_port = 0;
      status |= switch_store::v_get(
          port_lag_handle, SWITCH_LAG_ATTR_LAG_MEMBERS, lag_members);
      for (auto const mbr : lag_members) {
        switch_object_id_t mbr_port_handle = {0};
        status |= switch_store::v_get(
            mbr, SWITCH_LAG_MEMBER_ATTR_PORT_HANDLE, mbr_port_handle);
        status |= switch_store::v_get(
            mbr_port_handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
        dev_port_list.insert(dev_port);
      }
    } else if (ot == SWITCH_OBJECT_TYPE_TUNNEL) {
      return status;
    } else {
      return SWITCH_STATUS_FAILURE;
    }

    for (const auto dev_port : dev_port_list) {
      status = add_delete_pv_membership(dev_port, vlan_id, valid);
    }
    return status;
  }
  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    update_pv_membership(1);
    return status;
  }
  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    update_pv_membership(0);
    return status;
  }
};

/** @brief LAG selector group
 * Subscribes to LAG user object. Sets the group ID from the object handle
 * and size of the group as data.
 */
class lag_selector_group : public p4_object_selector_group {
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_LAG_SELECTOR_GROUP;
  static const switch_attr_id_t status_attr_id =
      SWITCH_LAG_SELECTOR_GROUP_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_LAG_SELECTOR_GROUP_ATTR_PARENT_HANDLE;

 public:
  lag_selector_group(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_selector_group(
            smi_id::SG_LAG_SELECTOR_GROUP,
            status_attr_id,
            smi_id::P_LAG_SELECTOR_GROUP_MAX_GROUP_SIZE,
            smi_id::P_LAG_SELECTOR_GROUP_MAX_MEMBER_ARRAY,
            smi_id::P_LAG_SELECTOR_GROUP_MAX_MEMBER_STATUS_ARRAY,
            auto_ot,
            parent_attr_id,
            parent) {
    status |= match_key.set_exact(smi_id::F_LAG_SELECTOR_GROUP_ID, parent);
    status |= action_entry.init_indirect_data();
  }
};

/** @brief LAG selector handler
 * Subscribes to a port object.
 * Creates a member in the lag selector table. Use the object handle as the
 * key and dev_port as the action parameter.
 */
class lag_selector : public p4_object_action_selector {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_LAG_SELECTOR;
  static const switch_attr_id_t status_attr_id =
      SWITCH_LAG_SELECTOR_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_LAG_SELECTOR_ATTR_PARENT_HANDLE;

 public:
  lag_selector(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_action_selector(smi_id::AP_LAG_SELECTOR,
                                  smi_id::F_LAG_SELECTOR_ACTION_MEMBER_ID,
                                  status_attr_id,
                                  auto_ot,
                                  parent_attr_id,
                                  parent) {
    CHECK_PORT_IN_SWITCH_PIPE_AND_RETURN(parent);
    match_key.set_exact(smi_id::F_LAG_SELECTOR_ACTION_MEMBER_ID, parent);
    action_entry.init_action_data(smi_id::A_SET_LAG_PORT);
    status |= action_entry.set_arg(
        smi_id::P_SET_LAG_PORT_PORT, parent, SWITCH_PORT_ATTR_DEV_PORT);
  }
};

/** @brief LAG table
 * Auto object of above lag selector object
 * Adds an entry for each port. Use the port_lag_index as the key
 * and parent action member ID from above as the action parameter.
 * For port,
 * LAG_TABLE : port_index -> member_id
 * LAG_SELECTOR: member_id -> dev_port
 * For lag,
 * LAG_TABLE : lag_index -> grp_id
 * LAG_SEL_GRP: grp_id -> member_list -> LAG_SELECTOR::member_id
 */
class lag_table_member : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_LAG_TABLE_MEMBER;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_LAG_TABLE_MEMBER_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_LAG_TABLE_MEMBER_ATTR_STATUS;

 public:
  lag_table_member(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(
            smi_id::T_LAG, status_attr_id, auto_ot, parent_attr_id, parent) {
    CHECK_PORT_IN_SWITCH_PIPE_AND_RETURN(parent);
    const auto port_lag_index = compute_port_lag_index(parent);
    status |= match_key.set_exact(smi_id::F_LAG_PORT_LAG_INDEX, port_lag_index);
    status |= action_entry.init_indirect_data();
    status |= action_entry.set_arg(smi_id::D_LAG_ACTION_MEMBER_ID, parent);
  }
};

/**
 * Both lag_table_member above and this class are the same except that above
 * is used for port_handles and below for lag_handles
 */
class lag_table_selector : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_LAG_TABLE_SELECTOR;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_LAG_TABLE_SELECTOR_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_LAG_TABLE_SELECTOR_ATTR_STATUS;

 public:
  lag_table_selector(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(
            smi_id::T_LAG, status_attr_id, auto_ot, parent_attr_id, parent) {
    const auto port_lag_index = compute_port_lag_index(parent);
    status |= match_key.set_exact(smi_id::F_LAG_PORT_LAG_INDEX, port_lag_index);
    status |= action_entry.init_indirect_data();
  }
  switch_status_t create_update() {
    // by default always set the default action member id at creation time
    // lag_membership will update the group as members come and go
    return set_selector_group(auto_obj.get_parent(), false);
  }
  switch_status_t set_selector_group(switch_object_id_t lag_handle,
                                     bool enable) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (enable) {
      status |=
          action_entry.set_arg(smi_id::D_LAG_SELECTOR_GROUP_ID, lag_handle);
    } else {
      status |=
          action_entry.set_arg(smi_id::D_LAG_ACTION_MEMBER_ID,
                               static_cast<uint32_t>(DEFAULT_ACTION_MEMBER_ID));
    }
    status = p4_object_match_action::create_update();
    return status;
  }
};

/** @brief LAG membership handler
 * Manages adding and deleting to a LAG group. Subscribes to updates from a
 * LAG member.
 * BF-Runtime requires all members when adding or deleting members to the
 * grp.
 * LAG group holds the list of members
 * Fetch the member IDs from the lag_selector object below
 * LAG_SEL_GRP: grp_id -> member_list -> LAG_SELECTOR::member_id
 */
class lag_membership : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_LAG_MEMBERSHIP;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_LAG_MEMBERSHIP_ATTR_PARENT_HANDLE;
  switch_object_id_t lag_handle = {}, port_handle = {},
                     lag_selector_handle = {}, peer_link_handle = {};
  bool egress_disable = true;

 public:
  lag_membership(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status |= switch_store::v_get(
        parent, SWITCH_LAG_MEMBER_ATTR_PORT_HANDLE, port_handle);
    status |= find_auto_oid(
        port_handle, SWITCH_OBJECT_TYPE_LAG_SELECTOR, lag_selector_handle);
    status |= switch_store::v_get(
        parent, SWITCH_LAG_MEMBER_ATTR_LAG_HANDLE, lag_handle);
    status |= switch_store::v_get(
        parent, SWITCH_LAG_MEMBER_ATTR_EGRESS_DISABLE, egress_disable);
    status |= switch_store::v_get(
        lag_handle, SWITCH_LAG_ATTR_PEER_LINK_HANDLE, peer_link_handle);
  }

  // Update pv registers for all vlan's this lag is part of
  // For existing members, pv addition happened during vlan_member create
  switch_status_t update_lag_pv_membership(switch_object_id_t p_port_handle,
                                           switch_object_id_t p_lag_handle,
                                           uint8_t valid) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint16_t dev_port = 0;
    status |=
        switch_store::v_get(p_port_handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);

    // get all the vlan's this lag is part of
    std::set<switch_object_id_t> vlan_member_handles;
    status |= switch_store::referencing_set_get(
        p_lag_handle, SWITCH_OBJECT_TYPE_VLAN_MEMBER, vlan_member_handles);
    // now update the pv membership for each pv combination
    for (const auto vlan_member : vlan_member_handles) {
      switch_object_id_t vlan_handle = {0};
      uint16_t vlan_id = 0;
      status |= switch_store::v_get(
          vlan_member, SWITCH_VLAN_MEMBER_ATTR_VLAN_HANDLE, vlan_handle);
      status |=
          switch_store::v_get(vlan_handle, SWITCH_VLAN_ATTR_VLAN_ID, vlan_id);

      status = add_delete_pv_membership(dev_port, vlan_id, valid);
    }

    // get all the sub_port RIF this lag is part of
    std::set<switch_object_id_t> rif_handles;
    status |= switch_store::referencing_set_get(
        p_lag_handle, SWITCH_OBJECT_TYPE_RIF, rif_handles);
    // now update the pv membership for each pv combination
    for (const auto rif_handle : rif_handles) {
      uint16_t vlan_id = 0;
      switch_enum_t rif_type = {};
      status |= switch_store::v_get(rif_handle, SWITCH_RIF_ATTR_TYPE, rif_type);
      if (rif_type.enumdata != SWITCH_RIF_ATTR_TYPE_SUB_PORT) continue;
      status |= switch_store::v_get(
          rif_handle, SWITCH_RIF_ATTR_OUTER_VLAN_ID, vlan_id);
      status |= switch_store::v_get(
          p_port_handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);

      status = add_delete_pv_membership(dev_port, vlan_id, valid);
    }
    return status;
  }

  switch_status_t elect_designated_lag_member(bool remove_me) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t curr_designated_lag_member = {0}, new_designated_lag_member = {0};

    status |= switch_store::v_get(lag_handle,
                                  SWITCH_LAG_ATTR_DESIGNATED_LAG_MEMBER,
                                  curr_designated_lag_member);
    new_designated_lag_member = curr_designated_lag_member;
    if (!remove_me) {
      if (curr_designated_lag_member == 0) {
        new_designated_lag_member = get_parent().data;
      }
    } else {
      if (curr_designated_lag_member == get_parent().data) {
        new_designated_lag_member = {0};
        std::vector<switch_object_id_t> lag_members;
        status |= switch_store::v_get(
            lag_handle, SWITCH_LAG_ATTR_LAG_MEMBERS, lag_members);
        bool mbr_egress_disable = true;
        for (auto const mbr : lag_members) {
          if (mbr.data != get_parent().data) {
            status |= switch_store::v_get(
                mbr, SWITCH_LAG_MEMBER_ATTR_EGRESS_DISABLE, mbr_egress_disable);
            if (!mbr_egress_disable) {
              new_designated_lag_member = mbr.data;
              break;
            }
          }
        }
      }
    }
    if (curr_designated_lag_member != new_designated_lag_member) {
      status |= switch_store::v_set(lag_handle,
                                    SWITCH_LAG_ATTR_DESIGNATED_LAG_MEMBER,
                                    new_designated_lag_member);
      std::set<switch_object_id_t> mirror_sessions;
      status |= switch_store::referencing_set_get(
          lag_handle, SWITCH_OBJECT_TYPE_MIRROR, mirror_sessions);
      for (const auto mirror_session : mirror_sessions) {
        std::unique_ptr<object> mirror(factory::get_instance().create(
            SWITCH_OBJECT_TYPE_MIRROR_SESSION, mirror_session, status));
        if (mirror != nullptr) {
          status |= mirror->create_update();
        }
      }
    }
    return status;
  }

  // add to group
  // ACTION_MEMBER_ID is derived from port_handle of the lag member
  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_mc_port_map_t port_map = {0};
    uint16_t yid = 0, dev_port = 0;
    uint32_t mc_sess_hdl = get_mc_sess_hdl();
    uint32_t pd_status = 0;
    bool is_update = false;

    if (lag_handle == 0) return SWITCH_STATUS_INVALID_PARAMETER;

    // Unselect this lag member if it was the designated lag member
    if (egress_disable) {
      status |= elect_designated_lag_member(true);
    }

    memset(port_map, 0x0, sizeof(switch_mc_port_map_t));
    std::vector<switch_object_id_t> lag_members;
    std::vector<bf_rt_id_t> mbrs;
    std::vector<bool> mbr_status;
    status |= switch_store::v_get(
        lag_handle, SWITCH_LAG_ATTR_LAG_MEMBERS, lag_members);
    // We mark the lag as enabled if atleast one of the lag members has
    // attr egress_disable set to false (i.e. egress_enable = true)
    bool lag_enable = false;
    // Check if parent is already a member, else add to list
    for (auto const mbr : lag_members) {
      if (mbr.data != get_parent().data) {
        bool egress_disable_tmp = false;
        switch_object_id_t mbr_port_handle = {0};
        status |= switch_store::v_get(
            mbr, SWITCH_LAG_MEMBER_ATTR_PORT_HANDLE, mbr_port_handle);
        status |= switch_store::v_get(
            mbr, SWITCH_LAG_MEMBER_ATTR_EGRESS_DISABLE, egress_disable_tmp);
        lag_enable = lag_enable || !(egress_disable_tmp);
        mbrs.push_back(mbr_port_handle.data);
        mbr_status.push_back(!egress_disable_tmp);
        status = switch_store::v_get(
            mbr_port_handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
      } else {
        if (!switch_store::smiContext::context().in_warm_init())
          is_update = true;
      }
    }
    // add this member's port handle
    mbrs.push_back(port_handle.data);
    mbr_status.push_back(!egress_disable);
    lag_enable = lag_enable || !(egress_disable);
    status =
        switch_store::v_get(port_handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);

    status = update_lag_pv_membership(port_handle, lag_handle, 1);
    if (status) return SWITCH_STATUS_FAILURE;

    // add the lag table entry pointing to the selector group when first
    // active (egress enable) member is added
    if (lag_enable) {
      // this API may fail in the case of warm init since a mbr for which
      // port
      // is not evaluated yet may not have an action_member_id created. In
      // that case, do not update the lag table
      lag_selector_group lag_group_obj(lag_handle, status);
      status |= lag_group_obj.add_delete_member(mbrs, mbr_status);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_LAG,
                   "{}.{}: Lag Group Member Add/Delete failed for lag.{} "
                   "status {}",
                   __func__,
                   __LINE__,
                   switch_store::handle_to_id(lag_handle),
                   status);
      }
      if (status) lag_enable = false;
      lag_table_selector lag_table_obj(lag_handle, status);
      status |= lag_table_obj.set_selector_group(lag_handle, lag_enable);
      if (status) {
        switch_log(SWITCH_API_LEVEL_WARN,
                   SWITCH_OBJECT_TYPE_LAG,
                   "{}.{}: Lag table entry update failed for lag.{} "
                   "status {}",
                   __func__,
                   __LINE__,
                   switch_store::handle_to_id(lag_handle),
                   status);
        status = SWITCH_STATUS_SUCCESS;
      }
    } else {
      lag_table_selector lag_table_obj(lag_handle, status);
      status |= lag_table_obj.set_selector_group(lag_handle, lag_enable);
      lag_selector_group lag_group_obj(lag_handle, status);
      status |= lag_group_obj.add_delete_member(mbrs, mbr_status);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_LAG,
                   "{}.{}: Lag Group Member Add/Delete failed for lag.{} "
                   "status {}",
                   __func__,
                   __LINE__,
                   switch_store::handle_to_id(lag_handle),
                   status);
        status = SWITCH_STATUS_SUCCESS;
      }
    }

    // this is an update on the lag member. nothing to modify beyond here
    if (is_update) {
      // Re-elect lag member if no designated lag member exists
      if (!egress_disable) {
        status |= elect_designated_lag_member(false);
      }
    }

    if (!is_update) {
      SWITCH_CONTEXT.release_yid(port_handle);
      if (mbrs.size() == 1 ||
          (mbrs.size() > 0 &&
           switch_store::smiContext::context().in_warm_init())) {
        // allocate a yid if first member is added
        yid = SWITCH_CONTEXT.alloc_yid(lag_handle);
      }
    }

    status |= switch_store::v_get(lag_handle, SWITCH_LAG_ATTR_YID, yid);
    // Read the port_map for this yid
    pd_status =
        p4_pd_mc_get_port_prune_table(mc_sess_hdl, 0, yid, port_map, true);

    // update port_map only if member enabled
    if (egress_disable) {
      SWITCH_MC_PORT_MAP_CLEAR(port_map, dev_port);
    } else {
      SWITCH_MC_PORT_MAP_SET(port_map, dev_port);
    }

    // update the prune map for the newly added port in the lag map
    pd_status = p4_pd_mc_update_port_prune_table(mc_sess_hdl, 0, yid, port_map);
    p4_pd_mc_complete_operations(mc_sess_hdl);
    if (pd_status != 0) return SWITCH_STATUS_FAILURE;

    pd_status = p4_pd_mc_set_lag_membership(
        mc_sess_hdl, 0, switch_store::handle_to_id(lag_handle), port_map);
    p4_pd_mc_complete_operations(mc_sess_hdl);
    if (pd_status != 0) return SWITCH_STATUS_FAILURE;

    // if this lag is an MLAG, add the lag_member to the peer_links prune
    // mask
    if (peer_link_handle.data != 0) {
      switch_mc_port_map_t peer_link_port_map = {0};
      uint16_t peer_link_yid = 0;

      // Get the peer_link prune mask
      status |= switch_store::v_get(
          peer_link_handle, SWITCH_LAG_ATTR_YID, peer_link_yid);
      pd_status = p4_pd_mc_get_port_prune_table(
          mc_sess_hdl, 0, peer_link_yid, peer_link_port_map, true);
      // add the new lag member port and set the prune mask
      if (egress_disable) {
        SWITCH_MC_PORT_MAP_CLEAR(peer_link_port_map, dev_port);
      } else {
        SWITCH_MC_PORT_MAP_SET(peer_link_port_map, dev_port);
      }
      pd_status = p4_pd_mc_update_port_prune_table(
          mc_sess_hdl, 0, peer_link_yid, peer_link_port_map);
      p4_pd_mc_complete_operations(mc_sess_hdl);
      if (pd_status != 0) return SWITCH_STATUS_FAILURE;

      // set the port as mlag_member
      status |= switch_store::v_set(
          port_handle, SWITCH_PORT_ATTR_IS_MLAG_MEMBER, true);
    }

    if (!switch_store::smiContext::context().in_warm_init() && !is_update) {
      status |= switch_store::v_set(
          port_handle, SWITCH_PORT_ATTR_LAG_HANDLE, lag_handle);
      status |= switch_store::list_v_push(
          lag_handle, SWITCH_LAG_ATTR_LAG_MEMBERS, get_parent());
    }

    // Re-elect lag member if no designated lag member exists
    if (!egress_disable) {
      status |= elect_designated_lag_member(false);
    }

    status |= auto_object::create_update();
    return status;
  }

  /* remove from group . */
  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_mc_port_map_t port_map = {0};
    uint16_t yid = 0, dev_port = 0;
    uint32_t mc_sess_hdl = get_mc_sess_hdl();
    uint32_t pd_status = 0;

    if (lag_handle == 0) return SWITCH_STATUS_INVALID_PARAMETER;

    memset(port_map, 0x0, sizeof(switch_mc_port_map_t));
    std::vector<switch_object_id_t> lag_members;
    std::vector<bf_rt_id_t> mbrs;
    std::vector<bool> mbr_status;
    status |= switch_store::v_get(
        lag_handle, SWITCH_LAG_ATTR_LAG_MEMBERS, lag_members);
    bool lag_enable = false;
    // Exclude the parent from the member list
    for (auto const mbr : lag_members) {
      if (mbr.data != get_parent().data) {
        bool egress_disable_tmp = false;
        switch_object_id_t mbr_port_handle = {0};
        status |= switch_store::v_get(
            mbr, SWITCH_LAG_MEMBER_ATTR_PORT_HANDLE, mbr_port_handle);
        status |= switch_store::v_get(
            mbr, SWITCH_LAG_MEMBER_ATTR_EGRESS_DISABLE, egress_disable_tmp);
        lag_enable = lag_enable || !(egress_disable_tmp);
        mbrs.push_back(mbr_port_handle.data);
        mbr_status.push_back(!egress_disable_tmp);
      }
    }

    // get the dev_port from the parent to remove from prune map
    status =
        switch_store::v_get(port_handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);

    // update prune map excluding the member being deleted
    status |= switch_store::v_get(lag_handle, SWITCH_LAG_ATTR_YID, yid);
    // read the prune map for this yid
    pd_status =
        p4_pd_mc_get_port_prune_table(mc_sess_hdl, 0, yid, port_map, true);
    SWITCH_MC_PORT_MAP_CLEAR(port_map, dev_port);

    // delete the port from lag's prune map
    pd_status = p4_pd_mc_update_port_prune_table(mc_sess_hdl, 0, yid, port_map);
    p4_pd_mc_complete_operations(mc_sess_hdl);
    if (pd_status != 0) return SWITCH_STATUS_FAILURE;

    pd_status = p4_pd_mc_set_lag_membership(
        mc_sess_hdl, 0, switch_store::handle_to_id(lag_handle), port_map);
    p4_pd_mc_complete_operations(mc_sess_hdl);
    if (pd_status != 0) return SWITCH_STATUS_FAILURE;

    // if this lag is an MLAG, remove the lag_member from the peer_links
    // prune
    // mask
    if (peer_link_handle.data != 0) {
      switch_mc_port_map_t peer_link_port_map = {0};
      uint16_t peer_link_yid = 0;

      // Get the peer_link prune mask
      status |= switch_store::v_get(
          peer_link_handle, SWITCH_LAG_ATTR_YID, peer_link_yid);
      pd_status = p4_pd_mc_get_port_prune_table(
          mc_sess_hdl, 0, peer_link_yid, peer_link_port_map, true);
      // remove the lag member port and update the prune mask
      SWITCH_MC_PORT_MAP_CLEAR(peer_link_port_map, dev_port);
      pd_status = p4_pd_mc_update_port_prune_table(
          mc_sess_hdl, 0, peer_link_yid, peer_link_port_map);
      p4_pd_mc_complete_operations(mc_sess_hdl);
      if (pd_status != 0) return SWITCH_STATUS_FAILURE;

      // port is not mlag_member anymore
      status |= switch_store::v_set(
          port_handle, SWITCH_PORT_ATTR_IS_MLAG_MEMBER, false);
    }

    if (!lag_enable) {
      // delete the lag table entry to the selector group
      lag_table_selector lag_table_obj(lag_handle, status);
      lag_table_obj.set_selector_group(lag_handle, false);
    }

    if (mbrs.size() == 0) {
      // Release the yid if last member is removed
      SWITCH_CONTEXT.release_yid(lag_handle);
    }
    yid = SWITCH_CONTEXT.alloc_yid(port_handle);

    // update the prune map with the new yid for this port
    memset(port_map, 0x0, sizeof(switch_mc_port_map_t));
    status =
        switch_store::v_get(port_handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
    SWITCH_MC_PORT_MAP_SET(port_map, dev_port);
    pd_status = p4_pd_mc_update_port_prune_table(mc_sess_hdl, 0, yid, port_map);
    p4_pd_mc_complete_operations(mc_sess_hdl);
    if (pd_status != 0) return SWITCH_STATUS_FAILURE;

    switch_object_id_t empty_lag_handle = {0};
    lag_selector_group lag_group_obj(lag_handle, status);
    status |= lag_group_obj.add_delete_member(mbrs, mbr_status);
    if (status) return SWITCH_STATUS_FAILURE;

    status = update_lag_pv_membership(port_handle, lag_handle, 0);
    if (status) return SWITCH_STATUS_FAILURE;

    if (!switch_store::smiContext::context().in_warm_init()) {
      status |= switch_store::list_v_del(
          lag_handle, SWITCH_LAG_ATTR_LAG_MEMBERS, get_parent());
      status |= switch_store::v_set(
          port_handle, SWITCH_PORT_ATTR_LAG_HANDLE, empty_lag_handle);
    }

    // Re-elect designated lag member
    if (!egress_disable) {
      status |= elect_designated_lag_member(true);
    }

    status |= auto_object::del();
    return status;
  }
};

class ingress_drop_stats : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_INGRESS_DROP_STATS;
  static const switch_attr_id_t status_attr_id =
      SWITCH_INGRESS_DROP_STATS_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INGRESS_DROP_STATS_ATTR_PARENT_HANDLE;

 public:
  ingress_drop_stats(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_INGRESS_DROP_STATS,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    uint16_t dev_port = 0;
    switch_enum_t port_type = {};
    CHECK_PORT_IN_SWITCH_PIPE_AND_RETURN(parent);
    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
    device_tgt_set(compute_dev_target_for_table(
        dev_port, smi_id::T_INGRESS_DROP_STATS, true));

    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_TYPE, port_type);
    if (port_type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL) return;

    std::vector<bf_rt_id_t> cntrs{smi_id::D_INGRESS_DROP_STATS_PKTS};
    auto it = match_action_list.begin();
    for (auto drop_reason : drop_reasons) {
      it = match_action_list.insert(
          it,
          std::pair<_MatchKey, _ActionEntry>(
              _MatchKey(smi_id::T_INGRESS_DROP_STATS),
              _ActionEntry(smi_id::T_INGRESS_DROP_STATS)));
      status |=
          it->first.set_exact(smi_id::F_INGRESS_DROP_STATS_PORT, dev_port);
      status |= it->first.set_exact(smi_id::F_INGRESS_DROP_STATS_DROP_REASON,
                                    drop_reason);
      it->second.init_action_data(smi_id::A_INGRESS_DROP_STATS_COUNT, cntrs);
    }
  }
  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t port_type = {};

    status |= switch_store::v_get(handle, SWITCH_PORT_ATTR_TYPE, port_type);
    if (port_type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL) return status;

    p4_object_match_action_list::data_get();

    // SWITCH_PORT_COUNTER_ID_IF_IN_DISCARDS
    cntrs[SWITCH_PORT_COUNTER_ID_IF_IN_DISCARDS].counter_id =
        SWITCH_PORT_COUNTER_ID_IF_IN_DISCARDS;
    for (auto const &entry : match_action_list) {
      uint64_t pkts = 0, drop_reason = 0;
      uint16_t counter_id = 0;
      entry.second.get_arg(smi_id::D_INGRESS_DROP_STATS_PKTS,
                           smi_id::A_INGRESS_DROP_STATS_COUNT,
                           &pkts);

      if (pkts == 0) continue;

      entry.first.get_exact(smi_id::F_INGRESS_DROP_STATS_DROP_REASON,
                            &drop_reason);
      if (drop_reason == SWITCH_DROP_REASON_INGRESS_PFC_WD_DROP) {
        // PFC WD drop stats will be counted in PPG so no need to
        // add them here
        continue;
      }
      cntrs[SWITCH_PORT_COUNTER_ID_IF_IN_DISCARDS].count += pkts;

      // check if drop_reason falls into its own counter
      // If that is the case the drop is counted twice as its own dedicated
      // counter and
      // as summary total SWITCH_PORT_COUNTER_ID_IF_IN_DISCARDS counter.
      if (port_drop_reason_to_switch_counter(drop_reason, &counter_id) ==
          SWITCH_STATUS_SUCCESS) {
        cntrs[counter_id].counter_id = counter_id;
        cntrs[counter_id].count += pkts;
        if (counter_id == SWITCH_PORT_COUNTER_ID_IF_IN_L2_MISS_UNICAST ||
            counter_id == SWITCH_PORT_COUNTER_ID_IF_IN_L2_MISS_MULTICAST ||
            counter_id == SWITCH_PORT_COUNTER_ID_IF_IN_L2_MISS_BROADCAST ||
            counter_id ==
                SWITCH_PORT_COUNTER_ID_IF_IN_IP_BLACKHOLE_ROUTE_DISCARDS) {
          cntrs[SWITCH_PORT_COUNTER_ID_IF_IN_FDB_AND_BLACKHOLE_DISCARDS]
              .counter_id =
              SWITCH_PORT_COUNTER_ID_IF_IN_FDB_AND_BLACKHOLE_DISCARDS;
          cntrs[SWITCH_PORT_COUNTER_ID_IF_IN_FDB_AND_BLACKHOLE_DISCARDS]
              .count += pkts;
        }
      }
    }

    // Include PPG drops in IF_IN_DISCARDS. This is needed by SONiC which
    // checks
    // SAI_PORT_STAT_IF_IN_DISCARDS for ingress buffer drops.
    // Currently the drops are reflected in
    // SWITCH_PORT_COUNTER_ID_INGRESS_TM_DISCARDS and finally in
    // SAI_PORT_STAT_IN_DROPPED_PKTS but this counter is not used by SONiC
    // at
    // the moment.
    std::vector<switch_object_id_t> port_ppg_handles_list;
    status = switch_store::v_get(auto_obj.get_parent(),
                                 SWITCH_PORT_ATTR_PORT_PRIORITY_GROUPS,
                                 port_ppg_handles_list);
    for (auto ppg_handle : port_ppg_handles_list) {
      std::vector<switch_counter_t> ppg_cntrs(
          SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_MAX);
      switch_store::object_counters_get(ppg_handle, ppg_cntrs);
      cntrs[SWITCH_PORT_COUNTER_ID_IF_IN_DISCARDS].count +=
          ppg_cntrs[SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_DROPPED_PACKETS]
              .count;
    }

    return status;
  }
  switch_status_t counters_set(const switch_object_id_t handle) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t port_type = {};

    status |= switch_store::v_get(handle, SWITCH_PORT_ATTR_TYPE, port_type);
    if (port_type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL) return status;

    std::vector<switch_object_id_t> port_ppg_handles_list;
    status = switch_store::v_get(auto_obj.get_parent(),
                                 SWITCH_PORT_ATTR_PORT_PRIORITY_GROUPS,
                                 port_ppg_handles_list);
    for (auto ppg_handle : port_ppg_handles_list) {
      std::vector<uint16_t> ppg_cntr_ids;
      ppg_cntr_ids.push_back(
          SWITCH_PORT_PRIORITY_GROUP_COUNTER_ID_DROPPED_PACKETS);
      status |= switch_store::object_counters_clear(ppg_handle, ppg_cntr_ids);
    }

    for (auto &entry : match_action_list) {
      uint64_t value = 0;
      entry.second.set_arg(smi_id::D_INGRESS_DROP_STATS_PKTS, value);
    }
    return p4_object_match_action_list::data_set();
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t port_type = {};

    status |= switch_store::v_get(handle, SWITCH_PORT_ATTR_TYPE, port_type);
    if (port_type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL) return status;

    for (auto cntr_id : cntr_ids) {
      switch (cntr_id) {
        case SWITCH_PORT_COUNTER_ID_IF_IN_DISCARDS:
          return counters_set(handle);
        default:
          break;
      }
    }
    return SWITCH_STATUS_SUCCESS;
  }

  switch_status_t counters_save(const switch_object_id_t parent) {
    (void)parent;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t ctr_pkt = 0;

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    p4_object_match_action_list::data_get();
    for (auto const &entry : match_action_list) {
      ctr_pkt = 0;
      entry.second.get_arg(smi_id::D_INGRESS_DROP_STATS_PKTS,
                           smi_id::A_INGRESS_DROP_STATS_COUNT,
                           &ctr_pkt);
      ctr_list.push_back(static_cast<uint64_t>(ctr_pkt));
    }
    attr_w ctr_attr_list(SWITCH_INGRESS_DROP_STATS_ATTR_MAU_STATS_CACHE);
    ctr_attr_list.v_set(ctr_list);
    switch_store::attribute_set(get_auto_oid(), ctr_attr_list);

    return status;
  }

  switch_status_t counters_restore(const switch_object_id_t parent) {
    (void)parent;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    size_t list_i;

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    status |=
        switch_store::v_get(get_auto_oid(),
                            SWITCH_INGRESS_DROP_STATS_ATTR_MAU_STATS_CACHE,
                            ctr_list);
    if (ctr_list.empty()) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_PORT,
                 "{}.{}: No stat cache to restore mau stats, "
                 "ingress_drop_stats cache list empty, status {}",
                 __func__,
                 __LINE__,
                 status);
      return SWITCH_STATUS_SUCCESS;
    }

    list_i = 0;
    for (auto &entry : match_action_list) {
      entry.second.set_arg(smi_id::D_INGRESS_DROP_STATS_PKTS, ctr_list[list_i]);
      list_i++;
    }
    status = p4_object_match_action_list::data_set();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_PORT,
                 "{}.{}: Fail to restore mau stats cache,  "
                 "ingress_drop_stats status {} ",
                 __func__,
                 __LINE__,
                 status);
    }

    return status;
  }
};

class egress_drop_stats : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_EGRESS_DROP_STATS;
  static const switch_attr_id_t status_attr_id =
      SWITCH_EGRESS_DROP_STATS_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_EGRESS_DROP_STATS_ATTR_PARENT_HANDLE;

 public:
  egress_drop_stats(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_EGRESS_DROP_STATS,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    uint16_t dev_port = 0;
    switch_enum_t port_type = {};
    CHECK_PORT_IN_SWITCH_PIPE_AND_RETURN(parent);
    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
    device_tgt_set(compute_dev_target_for_table(
        dev_port, smi_id::T_EGRESS_DROP_STATS, false));

    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_TYPE, port_type);
    if (port_type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL) return;

    std::vector<bf_rt_id_t> cntrs{smi_id::D_EGRESS_DROP_STATS_PKTS};
    auto it = match_action_list.begin();
    for (auto drop_reason : drop_reasons) {
      it = match_action_list.insert(
          it,
          std::pair<_MatchKey, _ActionEntry>(
              _MatchKey(smi_id::T_EGRESS_DROP_STATS),
              _ActionEntry(smi_id::T_EGRESS_DROP_STATS)));
      status |= it->first.set_exact(smi_id::F_EGRESS_DROP_STATS_PORT, dev_port);
      status |= it->first.set_exact(smi_id::F_EGRESS_DROP_STATS_DROP_REASON,
                                    drop_reason);
      it->second.init_action_data(smi_id::A_EGRESS_DROP_STATS_COUNT, cntrs);
    }
  }
  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t port_type = {};

    status |= switch_store::v_get(handle, SWITCH_PORT_ATTR_TYPE, port_type);
    if (port_type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL) return status;

    p4_object_match_action_list::data_get();
    cntrs[SWITCH_PORT_COUNTER_ID_IF_OUT_DISCARDS].counter_id =
        SWITCH_PORT_COUNTER_ID_IF_OUT_DISCARDS;
    for (auto const &entry : match_action_list) {
      uint64_t pkts = 0, drop_reason = 0;
      uint16_t counter_id;
      entry.second.get_arg(smi_id::D_EGRESS_DROP_STATS_PKTS,
                           smi_id::A_EGRESS_DROP_STATS_COUNT,
                           &pkts);
      cntrs[SWITCH_PORT_COUNTER_ID_IF_OUT_DISCARDS].count += pkts;
      entry.first.get_exact(smi_id::F_INGRESS_DROP_STATS_DROP_REASON,
                            &drop_reason);

      // check if drop_reason falls into another counter
      if (port_drop_reason_to_switch_counter(drop_reason, &counter_id) ==
          SWITCH_STATUS_SUCCESS) {
        cntrs[counter_id].counter_id = counter_id;
        cntrs[counter_id].count += pkts;
      }
    }
    std::vector<switch_object_id_t> queue_handles;
    status = switch_store::v_get(
        handle, SWITCH_PORT_ATTR_QUEUE_HANDLES, queue_handles);
    for (auto &queue_handle : queue_handles) {
      std::vector<switch_counter_t> queue_cntrs(SWITCH_QUEUE_COUNTER_ID_MAX);
      switch_store::object_counters_get(queue_handle, queue_cntrs);
      cntrs[SWITCH_PORT_COUNTER_ID_IF_OUT_DISCARDS].count +=
          queue_cntrs[SWITCH_QUEUE_COUNTER_ID_DROPPED_PACKETS].count;
    }
    return status;
  }
  switch_status_t counters_set(const switch_object_id_t handle) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t port_type = {};

    status |= switch_store::v_get(handle, SWITCH_PORT_ATTR_TYPE, port_type);
    if (port_type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL) return status;

    for (auto &entry : match_action_list) {
      uint64_t value = 0;
      entry.second.set_arg(smi_id::D_EGRESS_DROP_STATS_PKTS, value);
    }
    std::vector<switch_object_id_t> queue_handles;
    status = switch_store::v_get(
        handle, SWITCH_PORT_ATTR_QUEUE_HANDLES, queue_handles);
    for (auto &queue_handle : queue_handles) {
      std::vector<uint16_t> queue_cntr_ids;
      queue_cntr_ids.push_back(SWITCH_QUEUE_COUNTER_ID_DROPPED_PACKETS);
      status |=
          switch_store::object_counters_clear(queue_handle, queue_cntr_ids);
    }
    return p4_object_match_action_list::data_set();
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t port_type = {};

    status |= switch_store::v_get(handle, SWITCH_PORT_ATTR_TYPE, port_type);
    if (port_type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL) return status;

    for (auto cntr_id : cntr_ids) {
      switch (cntr_id) {
        case SWITCH_PORT_COUNTER_ID_IF_OUT_DISCARDS:
          return counters_set(handle);
        default:
          break;
      }
    }
    return SWITCH_STATUS_SUCCESS;
  }

  switch_status_t counters_save(const switch_object_id_t parent) {
    (void)parent;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t ctr_pkt = 0;

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    p4_object_match_action_list::data_get();
    for (auto const &entry : match_action_list) {
      ctr_pkt = 0;
      entry.second.get_arg(smi_id::D_EGRESS_DROP_STATS_PKTS,
                           smi_id::A_EGRESS_DROP_STATS_COUNT,
                           &ctr_pkt);
      ctr_list.push_back(static_cast<uint64_t>(ctr_pkt));
    }

    attr_w ctr_attr_list(SWITCH_EGRESS_DROP_STATS_ATTR_MAU_STATS_CACHE);
    ctr_attr_list.v_set(ctr_list);
    switch_store::attribute_set(get_auto_oid(), ctr_attr_list);

    return status;
  }

  switch_status_t counters_restore(const switch_object_id_t parent) {
    (void)parent;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    size_t list_i;

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    status |= switch_store::v_get(get_auto_oid(),
                                  SWITCH_EGRESS_DROP_STATS_ATTR_MAU_STATS_CACHE,
                                  ctr_list);
    if (ctr_list.empty()) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_PORT,
                 "{}.{}: No stat cache to restore mau stats, "
                 "ingress_drop_stats cache list empty, status {}",
                 __func__,
                 __LINE__,
                 status);
      return SWITCH_STATUS_SUCCESS;
    }

    list_i = 0;
    for (auto &entry : match_action_list) {
      entry.second.set_arg(smi_id::D_EGRESS_DROP_STATS_PKTS, ctr_list[list_i]);
      list_i++;
    }
    status = p4_object_match_action_list::data_set();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_PORT,
                 "{}.{}: Fail to restore mau stats cache,  "
                 "egress_drop_stats status {} ",
                 __func__,
                 __LINE__,
                 status);
    }

    return status;
  }
};

class port_stats : public auto_object {
 private:
  uint16_t device = 0, dev_port = 0;
  uint64_t port_id = 0;
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_PORT_STATS;
  static const switch_attr_id_t status_attr_id = SWITCH_PORT_STATS_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_PORT_STATS_ATTR_PARENT_HANDLE;

 public:
  port_stats(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    switch_object_id_t device_handle = {0};
    switch_enum_t port_type = {};

    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_TYPE, port_type);
    if (port_type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL) return;

    status |=
        switch_store::v_get(parent, SWITCH_PORT_ATTR_DEVICE, device_handle);
    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
    status |=
        switch_store::v_get(device_handle, SWITCH_DEVICE_ATTR_DEV_ID, device);
    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_PORT_ID, port_id);
  }

  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t port_type = {};

    status |= switch_store::v_get(handle, SWITCH_PORT_ATTR_TYPE, port_type);
    if (port_type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL) return status;

    uint64_t rmon_counters[BF_NUM_RMON_COUNTERS] = {0};
    bf_status_t bf_status = BF_SUCCESS;

    bf_status = bf_pal_port_all_stats_get(device, dev_port, rmon_counters);
    if (bf_status != BF_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_PORT,
                 "{}.{}: Failed to get port all stats, dev_port {} "
                 "status {}",
                 __func__,
                 __LINE__,
                 dev_port,
                 bf_err_str(bf_status));
      return SWITCH_STATUS_FAILURE;
    }

    uint64_t idrop_count = 0, edrop_count = 0;
    uint64_t in_cells = 0, out_cells = 0, in_wm_cells = 0, out_wm_cells = 0;
    uint32_t cell_size = 0;
    bool sw_model = false;

    bf_status = bf_pal_pltfm_type_get(device, &sw_model);
    if (!sw_model) {
      status = bfrt_tm_counter_ig_port_drop_packets_get(idrop_count);
      status |= bfrt_tm_counter_eg_port_drop_packets_get(edrop_count);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_PORT,
                   "{}.{}: Failed to get port tm drop stats, dev_port {} "
                   "status {}",
                   __func__,
                   __LINE__,
                   dev_port,
                   status);
        return SWITCH_STATUS_FAILURE;
      }

      status = bfrt_tm_counter_ig_port_usage_get(in_cells, in_wm_cells);
      status |= bfrt_tm_counter_eg_port_usage_get(out_cells, out_wm_cells);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_PORT,
                   "{}.{}: Failed to get port tm usage stats, dev_port {} "
                   "status {}",
                   __func__,
                   __LINE__,
                   dev_port,
                   status);
        return SWITCH_STATUS_FAILURE;
      }

      status = bfrt_tm_cfg_cell_size_bytes_get(device, cell_size);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_PORT,
                   "{}.{}: Failed to get cell size in bytes, dev_port {} "
                   "status {}",
                   __func__,
                   __LINE__,
                   dev_port,
                   switch_error_to_string(status));
        return status;
      }
    }

    // Reading the port rate stat from SW buffer
    std::vector<uint64_t> port_rate_stat(BF_PORT_RATE_MAX_COUNTERS);
    SWITCH_CONTEXT.port_rate_get(device, port_id, port_rate_stat);

    for (uint16_t i = 0; i < SWITCH_PORT_COUNTER_ID_MAX; i++) {
      switch_counter_t counter = {0};
      counter.counter_id = i;
      counter.count = cntrs[i].count;
      switch (i) {
        case SWITCH_PORT_COUNTER_ID_INGRESS_TM_DISCARDS:
          counter.count = idrop_count;
          break;
        case SWITCH_PORT_COUNTER_ID_EGRESS_TM_DISCARDS:
          counter.count = edrop_count;
          break;
        case SWITCH_PORT_COUNTER_ID_IN_CURR_OCCUPANCY_BYTES:
          counter.count = (in_cells * cell_size);
          break;
        case SWITCH_PORT_COUNTER_ID_OUT_CURR_OCCUPANCY_BYTES:
          counter.count = (out_cells * cell_size);
          break;
        case SWITCH_PORT_COUNTER_ID_IN_OCTETS_RATE:
          counter.count = port_rate_stat[BF_PORT_IN_OCTETS_RATE];
          break;
        case SWITCH_PORT_COUNTER_ID_IN_PKTS_RATE:
          counter.count = port_rate_stat[BF_PORT_IN_PKTS_RATE];
          break;
        case SWITCH_PORT_COUNTER_ID_OUT_OCTETS_RATE:
          counter.count = port_rate_stat[BF_PORT_OUT_OCTETS_RATE];
          break;
        case SWITCH_PORT_COUNTER_ID_OUT_PKTS_RATE:
          counter.count = port_rate_stat[BF_PORT_OUT_PKTS_RATE];
          break;
        default: {
          std::vector<bf_rmon_counter_t> rmon_cntrs_ids;
          switch_port_counter_to_pd_port_counters_list(
              static_cast<switch_port_counter_id>(i), rmon_cntrs_ids);
          for (auto rmon_id : rmon_cntrs_ids) {
            counter.count += rmon_counters[rmon_id];
          }
          break;
        }
      }
      cntrs[i] = counter;
    }

    cntrs[SWITCH_PORT_COUNTER_ID_IF_IN_DISCARDS].count +=
        cntrs[SWITCH_PORT_COUNTER_ID_INGRESS_TM_DISCARDS].count;

    cntrs[SWITCH_PORT_COUNTER_ID_IF_OUT_DISCARDS].count +=
        cntrs[SWITCH_PORT_COUNTER_ID_EGRESS_TM_DISCARDS].count;

    return status;
  }

  switch_status_t counters_set(const switch_object_id_t handle) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t port_type = {};

    status |= switch_store::v_get(handle, SWITCH_PORT_ATTR_TYPE, port_type);
    if (port_type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL) return status;

    uint32_t pd_status = 0;
    bool sw_model = false;
    bf_status_t bf_status = BF_SUCCESS;

    bf_status = bf_pal_port_all_stats_clear(device, dev_port);
    if (bf_status != BF_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_PORT,
                 "{}:{}: failed to clear stats for device {} dev_port {}, "
                 "status {}",
                 __func__,
                 __LINE__,
                 device,
                 dev_port,
                 bf_err_str(bf_status));
    }

    bf_status = bf_pal_pltfm_type_get(device, &sw_model);
    if (!sw_model) {
      status = bfrt_tm_counter_ig_port_drop_packets_clear();
      status |= bfrt_tm_counter_eg_port_drop_packets_clear();
      if (status != 0) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_PORT,
                   "{}.{}: Failed to clear port tm drop stats, dev_port {} "
                   "status {}",
                   __func__,
                   __LINE__,
                   dev_port,
                   status);
        return SWITCH_STATUS_FAILURE;
      }

      pd_status = p4_pd_tm_port_watermark_clear(device, dev_port);
      if (pd_status != 0) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_PORT,
                   "{}.{}: Failed to clear port tm usage stats, dev_port {} "
                   "status {}",
                   __func__,
                   __LINE__,
                   dev_port,
                   pd_status);
        return SWITCH_STATUS_FAILURE;
      }
    }
    return status;
  }

  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t port_type = {};

    status |= switch_store::v_get(handle, SWITCH_PORT_ATTR_TYPE, port_type);
    if (port_type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL) return status;

    uint32_t pd_status = 0;
    bool sw_model = false;
    bf_status_t bf_status = BF_SUCCESS;

    bf_status = bf_pal_pltfm_type_get(device, &sw_model);

    for (auto cntr_id : cntr_ids) {
      switch (cntr_id) {
        case SWITCH_PORT_COUNTER_ID_INGRESS_TM_DISCARDS:
        case SWITCH_PORT_COUNTER_ID_EGRESS_TM_DISCARDS:
          if (!sw_model) {
            pd_status = p4_pd_tm_port_drop_count_clear(device, dev_port);
            if (pd_status != 0) {
              switch_log(
                  SWITCH_API_LEVEL_ERROR,
                  SWITCH_OBJECT_TYPE_PORT,
                  "{}.{}: Failed to clear port tm drop stats, dev_port {} "
                  "status {}",
                  __func__,
                  __LINE__,
                  dev_port,
                  pd_status);
              return SWITCH_STATUS_FAILURE;
            }
          }
          break;
        case SWITCH_PORT_COUNTER_ID_IN_CURR_OCCUPANCY_BYTES:
        case SWITCH_PORT_COUNTER_ID_OUT_CURR_OCCUPANCY_BYTES:
          if (!sw_model) {
            pd_status = p4_pd_tm_port_watermark_clear(device, dev_port);
            if (pd_status != 0) {
              switch_log(
                  SWITCH_API_LEVEL_ERROR,
                  SWITCH_OBJECT_TYPE_PORT,
                  "{}.{}: Failed to clear port tm usage stats, dev_port {} "
                  "status {}",
                  __func__,
                  __LINE__,
                  dev_port,
                  pd_status);
              return SWITCH_STATUS_FAILURE;
            }
          }
          break;
        default: {
          std::vector<bf_rmon_counter_t> rmon_cntrs_ids;
          switch_port_counter_to_pd_port_counters_list(
              static_cast<switch_port_counter_id>(cntr_id), rmon_cntrs_ids);
          for (auto rmon_id : rmon_cntrs_ids) {
            bf_status = bf_pal_port_this_stat_clear(device, dev_port, rmon_id);
            if (bf_status != BF_SUCCESS) {
              switch_log(SWITCH_API_LEVEL_ERROR,
                         SWITCH_OBJECT_TYPE_PORT,
                         "{}:{}: failed to clear stats for device {} dev_port "
                         "{} counter_id {}, status {}",
                         __func__,
                         __LINE__,
                         device,
                         dev_port,
                         rmon_id,
                         bf_err_str(bf_status));
            }
          }
          break;
        }
      }
    }

    return status;
  }

  switch_status_t counters_save(const switch_object_id_t parent) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    bf_status_t bf_status = BF_SUCCESS;
    switch_enum_t port_type = {0};

    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_TYPE, port_type);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_PORT,
                 "{}.{}: Fail to get port_type for port {:#x} status {}",
                 __func__,
                 __LINE__,
                 parent.data,
                 status);
      return status;
    }
    if (port_type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL) {
      return status;
    }

    if (get_auto_oid() == 0) return status;

    // MAC rmon counters
    bf_rmon_counter_array_t rmon_ctr_array;
    memset(&rmon_ctr_array, 0x0, sizeof(rmon_ctr_array));
    bf_status =
        bf_pal_port_mac_stats_historical_get(device, dev_port, &rmon_ctr_array);
    if (bf_status != BF_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_PORT,
                 "{}.{}: Fail to get all mac stats for port, "
                 "dev_port {} status {}",
                 __func__,
                 __LINE__,
                 dev_port,
                 bf_err_str(bf_status));
      return SWITCH_STATUS_FAILURE;
    }

    std::vector<uint64_t> ctr_list;
    for (uint32_t i = 0; i < BF_NUM_RMON_COUNTERS; i++) {
      ctr_list.push_back(
          static_cast<uint64_t>(rmon_ctr_array.format.ctr_array[i]));
    }
    attr_w mac_ctr_attr_list(SWITCH_PORT_STATS_ATTR_MAC_STATS_CACHE);
    mac_ctr_attr_list.v_set(ctr_list);
    switch_store::attribute_set(get_auto_oid(), mac_ctr_attr_list);

    // TM counter
    bool sw_model = false;
    uint64_t ing_ctr_value = 0, eg_ctr_value = 0;

    bf_status = bf_pal_pltfm_type_get(device, &sw_model);
    if (!sw_model) {
      status = bf_tm_port_drop_cache_get(
          device,
          static_cast<bf_dev_pipe_t> DEV_PORT_TO_PIPE(dev_port),
          dev_port,
          &ing_ctr_value,
          &eg_ctr_value);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_PORT,
                   "{}.{}: Fail to get from tm port drop stats cache, "
                   "pd_status {}",
                   "port_stats",
                   __func__,
                   __LINE__,
                   status);
        return SWITCH_STATUS_FAILURE;
      }
    }  // (!sw_model)

    ctr_list.clear();
    ctr_list.push_back(static_cast<uint64_t>(ing_ctr_value));
    ctr_list.push_back(static_cast<uint64_t>(eg_ctr_value));
    attr_w tm_ctr_attr_list(SWITCH_PORT_STATS_ATTR_TM_STATS_CACHE);
    tm_ctr_attr_list.v_set(ctr_list);
    switch_store::attribute_set(get_auto_oid(), tm_ctr_attr_list);
    return status;
  }

  switch_status_t counters_restore(const switch_object_id_t parent) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    bf_status_t bf_status = BF_SUCCESS;
    switch_enum_t port_type = {0};

    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_TYPE, port_type);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_PORT,
                 "{}.{}: Fail to get port_type for port {:#x} status {}",
                 __func__,
                 __LINE__,
                 parent.data,
                 status);
      return status;
    }
    if (port_type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL) {
      return status;
    }

    if (get_auto_oid() == 0) return status;

    // MAC rmon counters
    bf_rmon_counter_array_t rmon_ctr_array;

    attr_w mac_ctr_attr_list(SWITCH_PORT_STATS_ATTR_MAC_STATS_CACHE);
    status |=
        switch_store::attribute_get(get_auto_oid(),
                                    SWITCH_PORT_STATS_ATTR_MAC_STATS_CACHE,
                                    mac_ctr_attr_list);
    std::vector<uint64_t> ctr_list;
    mac_ctr_attr_list.v_get(ctr_list);
    if (ctr_list.empty()) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_PORT,
                 "{}.{}: No stat cache to restore mac stats, "
                 "port_stats cache list empty, status {}",
                 __func__,
                 __LINE__,
                 status);
      return SWITCH_STATUS_SUCCESS;
    }

    memset(&rmon_ctr_array, 0x0, sizeof(rmon_ctr_array));
    for (size_t i = 0; i < ctr_list.size(); i++) {
      rmon_ctr_array.format.ctr_array[i] = ctr_list[i];
    }

    bf_status =
        bf_pal_port_mac_stats_historical_set(device, dev_port, &rmon_ctr_array);
    if (bf_status != BF_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_PORT,
                 "{}.{}: Fail to set port all mac stats, "
                 "dev_port {} status {}",
                 __func__,
                 __LINE__,
                 dev_port,
                 bf_err_str(bf_status));
      return SWITCH_STATUS_FAILURE;
    }

    // TM counter
    uint64_t ing_ctr_value = 0, eg_ctr_value = 0;
    bool sw_model = false;

    attr_w tm_ctr_attr_list(SWITCH_PORT_STATS_ATTR_TM_STATS_CACHE);
    status |= switch_store::attribute_get(get_auto_oid(),
                                          SWITCH_PORT_STATS_ATTR_TM_STATS_CACHE,
                                          tm_ctr_attr_list);
    ctr_list.clear();
    tm_ctr_attr_list.v_get(ctr_list);
    if (ctr_list.empty()) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_PORT,
                 "{}.{}: Fail to restore tm port drop stats cache, status {}",
                 "port_stats tm cache list count {} ",
                 __func__,
                 __LINE__,
                 status,
                 ctr_list.size());
      return status;
    }

    if (ctr_list.size() == 2) {
      ing_ctr_value = ctr_list[0];
      eg_ctr_value = ctr_list[1];
    }

    bf_status = bf_pal_pltfm_type_get(device, &sw_model);
    if (!sw_model) {
      bf_status = bf_tm_port_drop_cache_set(
          device,
          static_cast<bf_dev_pipe_t> DEV_PORT_TO_PIPE(dev_port),
          dev_port,
          ing_ctr_value,
          eg_ctr_value);
      if (bf_status != BF_SUCCESS) {
        switch_log(
            SWITCH_API_LEVEL_ERROR,
            SWITCH_OBJECT_TYPE_PORT,
            "{}.{}: Fail to restore tm port drop stats cache, pd_status {}",
            "port_stats",
            __func__,
            __LINE__,
            bf_err_str(bf_status));
        return SWITCH_STATUS_FAILURE;
      }
    }
    return status;
  }

  switch_status_t bfrt_tm_counter_ig_port_drop_packets_clear() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_TM_COUNTER_IG_PORT);
    _MatchKey match_key(smi_id::T_TM_COUNTER_IG_PORT);
    _ActionEntry entry(smi_id::T_TM_COUNTER_IG_PORT);
    entry.init_indirect_data();

    status =
        match_key.set_exact(smi_id::F_TM_COUNTER_IG_PORT_DEV_PORT, dev_port);
    status |= entry.set_arg(smi_id::D_TM_COUNTER_IG_PORT_DROP_COUNT,
                            static_cast<uint32_t>(0));
    status |= table.entry_modify(match_key, entry);

    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_PORT_PRIORITY_GROUP,
                 "{}.{}:{}: Failed to clear ingress port drop counter for ",
                 "dev_port {}, status {}",
                 "port_stats",
                 __func__,
                 __LINE__,
                 dev_port,
                 status);
    }
    return status;
  }

  switch_status_t bfrt_tm_counter_eg_port_drop_packets_clear() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_TM_COUNTER_EG_PORT);
    _MatchKey match_key(smi_id::T_TM_COUNTER_EG_PORT);
    _ActionEntry entry(smi_id::T_TM_COUNTER_EG_PORT);

    entry.init_indirect_data();
    status =
        match_key.set_exact(smi_id::F_TM_COUNTER_EG_PORT_DEV_PORT, dev_port);
    status |= entry.set_arg(smi_id::D_TM_COUNTER_EG_PORT_DROP_COUNT,
                            static_cast<uint32_t>(0));
    status |= table.entry_modify(match_key, entry);

    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_PORT_PRIORITY_GROUP,
                 "{}.{}:{}: Failed to clear egress port drop counter for ",
                 "dev_port {}, status {}",
                 "port_stats",
                 __func__,
                 __LINE__,
                 dev_port,
                 status);
    }
    return status;
  }

  switch_status_t bfrt_tm_counter_ig_port_drop_packets_get(uint64_t &ig_count) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_TM_COUNTER_IG_PORT);
    _MatchKey match_key(smi_id::T_TM_COUNTER_IG_PORT);
    _ActionEntry entry(smi_id::T_TM_COUNTER_IG_PORT);

    entry.init_indirect_data();
    status =
        match_key.set_exact(smi_id::F_TM_COUNTER_IG_PORT_DEV_PORT, dev_port);
    status |= table.entry_get(match_key, entry);
    status |= entry.get_arg(smi_id::D_TM_COUNTER_IG_PORT_DROP_COUNT, &ig_count);

    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_PORT_PRIORITY_GROUP,
                 "{}.{}:{}: Failed to get ingress port drop counter for ",
                 "dev_port {}, status {}",
                 "port_stats",
                 __func__,
                 __LINE__,
                 dev_port,
                 status);
    }
    return status;
  }

  switch_status_t bfrt_tm_counter_eg_port_drop_packets_get(uint64_t &eg_count) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_TM_COUNTER_EG_PORT);
    _MatchKey match_key(smi_id::T_TM_COUNTER_EG_PORT);
    _ActionEntry entry(smi_id::T_TM_COUNTER_EG_PORT);

    entry.init_indirect_data();
    status =
        match_key.set_exact(smi_id::F_TM_COUNTER_EG_PORT_DEV_PORT, dev_port);
    status |= table.entry_get(match_key, entry);
    status |= entry.get_arg(smi_id::D_TM_COUNTER_EG_PORT_DROP_COUNT, &eg_count);

    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_PORT_PRIORITY_GROUP,
                 "{}.{}:{}: Failed to get egress port drop counter for ",
                 "dev_port {}, status {}",
                 "port_stats",
                 __func__,
                 __LINE__,
                 dev_port,
                 status);
    }
    return status;
  }

  switch_status_t bfrt_tm_counter_ig_port_usage_get(uint64_t &ig_usage,
                                                    uint64_t &ig_wm) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_TM_COUNTER_IG_PORT);
    _MatchKey match_key(smi_id::T_TM_COUNTER_IG_PORT);
    _ActionEntry entry(smi_id::T_TM_COUNTER_IG_PORT);

    entry.init_indirect_data();
    status =
        match_key.set_exact(smi_id::F_TM_COUNTER_IG_PORT_DEV_PORT, dev_port);
    status |= table.entry_get(match_key, entry);
    status |=
        entry.get_arg(smi_id::D_TM_COUNTER_IG_PORT_USAGE_CELLS, &ig_usage);
    status |= entry.get_arg(smi_id::D_TM_COUNTER_IG_PORT_USAGE_CELLS, &ig_wm);

    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_PORT_PRIORITY_GROUP,
                 "{}.{}:{}: Failed to get ingress port usage counters for ",
                 "dev_port {}, status {}",
                 "port_stats",
                 __func__,
                 __LINE__,
                 dev_port,
                 status);
    }
    return status;
  }

  switch_status_t bfrt_tm_counter_eg_port_usage_get(uint64_t &eg_usage,
                                                    uint64_t &eg_wm) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_TM_COUNTER_EG_PORT);
    _MatchKey match_key(smi_id::T_TM_COUNTER_EG_PORT);
    _ActionEntry entry(smi_id::T_TM_COUNTER_EG_PORT);

    entry.init_indirect_data();
    status =
        match_key.set_exact(smi_id::F_TM_COUNTER_EG_PORT_DEV_PORT, dev_port);
    status |= table.entry_get(match_key, entry);
    status |=
        entry.get_arg(smi_id::D_TM_COUNTER_EG_PORT_USAGE_CELLS, &eg_usage);
    status |= entry.get_arg(smi_id::D_TM_COUNTER_EG_PORT_USAGE_CELLS, &eg_wm);

    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_PORT_PRIORITY_GROUP,
                 "{}.{}:{}: Failed to get egress port usage counters for ",
                 "dev_port {}, status {}",
                 "port_stats",
                 __func__,
                 __LINE__,
                 dev_port,
                 status);
    }
    return status;
  }

  switch_status_t create_update() { return auto_object::create_update(); }
  switch_status_t del() { return auto_object::del(); }
};

/* INGRESS_PORT_MIRROR p4 object */
class ingress_port_mirror : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_INGRESS_PORT_MIRROR;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INGRESS_PORT_MIRROR_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_INGRESS_PORT_MIRROR_ATTR_STATUS;

 public:
  ingress_port_mirror(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_INGRESS_PORT_MIRROR,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_object_id_t mirror_handle = {0};
    switch_object_id_t meter_handle = {0};
    CHECK_PORT_IN_SWITCH_PIPE_AND_RETURN(parent);
    status |= match_key.set_exact(
        smi_id::F_INGRESS_PORT_MIRROR_PORT, parent, SWITCH_PORT_ATTR_DEV_PORT);
    status |= switch_store::v_get(
        parent, SWITCH_PORT_ATTR_INGRESS_MIRROR_HANDLE, mirror_handle);
    if (mirror_handle.data == 0) {
      action_entry.init_action_data(smi_id::A_NO_ACTION);
      return;
    }
    status |= switch_store::v_get(
        mirror_handle, SWITCH_MIRROR_ATTR_METER_HANDLE, meter_handle);
    switch_enum_t direction = {0};
    status |= switch_store::v_get(
        mirror_handle, SWITCH_MIRROR_ATTR_DIRECTION, direction);
    if (direction.enumdata == SWITCH_MIRROR_ATTR_DIRECTION_EGRESS) {
      action_entry.init_action_data(smi_id::A_NO_ACTION);
      return;
    }

    switch_enum_t mirror_type = {};
    status |= switch_store::v_get(
        mirror_handle, SWITCH_MIRROR_ATTR_TYPE, mirror_type);
    uint8_t src = SWITCH_PKT_SRC_CLONED_INGRESS;
    if (mirror_type.enumdata == SWITCH_MIRROR_ATTR_TYPE_REMOTE)
      src = SWITCH_PKT_SRC_CLONED_INGRESS_RSPAN;

    uint16_t session_id;
    status |= switch_store::v_get(
        mirror_handle, SWITCH_MIRROR_ATTR_SESSION_ID, session_id);
    action_entry.init_action_data(smi_id::A_INGRESS_PORT_MIRROR_SET_MIRROR_ID);
    status |= action_entry.set_arg(
        smi_id::P_INGRESS_PORT_MIRROR_SET_MIRROR_ID_SESSION_ID, session_id);
    status |= action_entry.set_arg(
        smi_id::P_INGRESS_PORT_MIRROR_SET_MIRROR_ID_SRC, src);
    if (meter_handle.data != 0) {
      switch_enum_t target_type = {
          .enumdata = SWITCH_METER_ATTR_TARGET_TYPE_INGRESS_MIRROR};
      status |= switch_store::v_set(
          meter_handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
      status |= action_entry.set_arg(
          smi_id::P_INGRESS_PORT_MIRROR_SET_MIRROR_ID_METER_ID, meter_handle);
    }
  }
};

/* EGRESS_PORT_MIRROR p4 object */
class egress_port_mirror : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_EGRESS_PORT_MIRROR;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_EGRESS_PORT_MIRROR_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_EGRESS_PORT_MIRROR_ATTR_STATUS;

 public:
  egress_port_mirror(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_EGRESS_PORT_MIRROR,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    CHECK_PORT_IN_SWITCH_PIPE_AND_RETURN(parent);
    switch_object_id_t mirror_handle = {0};
    switch_object_id_t meter_handle = {0};
    status |= match_key.set_exact(
        smi_id::F_EGRESS_PORT_MIRROR_PORT, parent, SWITCH_PORT_ATTR_DEV_PORT);
    status |= switch_store::v_get(
        parent, SWITCH_PORT_ATTR_EGRESS_MIRROR_HANDLE, mirror_handle);
    if (mirror_handle.data == 0) {
      action_entry.init_action_data(smi_id::A_NO_ACTION);
      return;
    }
    status |= switch_store::v_get(
        mirror_handle, SWITCH_MIRROR_ATTR_METER_HANDLE, meter_handle);
    switch_enum_t e = {0};
    status |=
        switch_store::v_get(mirror_handle, SWITCH_MIRROR_ATTR_DIRECTION, e);
    if (e.enumdata == SWITCH_MIRROR_ATTR_DIRECTION_INGRESS) {
      action_entry.init_action_data(smi_id::A_NO_ACTION);
      return;
    }

    switch_enum_t mirror_type = {};
    status |= switch_store::v_get(
        mirror_handle, SWITCH_MIRROR_ATTR_TYPE, mirror_type);
    uint8_t src = SWITCH_PKT_SRC_CLONED_EGRESS;
    if (mirror_type.enumdata == SWITCH_MIRROR_ATTR_TYPE_REMOTE)
      src = SWITCH_PKT_SRC_CLONED_EGRESS_RSPAN;

    uint16_t session_id;
    status |= switch_store::v_get(
        mirror_handle, SWITCH_MIRROR_ATTR_SESSION_ID, session_id);
    action_entry.init_action_data(smi_id::A_EGRESS_PORT_MIRROR_SET_MIRROR_ID);
    status |= action_entry.set_arg(
        smi_id::P_EGRESS_PORT_MIRROR_SET_MIRROR_ID_SESSION_ID, session_id);
    status |= action_entry.set_arg(
        smi_id::P_EGRESS_PORT_MIRROR_SET_MIRROR_ID_SRC, src);
    if (meter_handle.data != 0) {
      switch_enum_t target_type = {
          .enumdata = SWITCH_METER_ATTR_TARGET_TYPE_EGRESS_MIRROR};
      status |= switch_store::v_set(
          meter_handle, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
      status |= action_entry.set_arg(
          smi_id::P_EGRESS_PORT_MIRROR_SET_MIRROR_ID_METER_ID, meter_handle);
    }
  }
};

switch_status_t after_port_update2(const switch_object_id_t object_id,
                                   const attr_w &attr) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  switch (attr.id_get()) {
    case SWITCH_PORT_ATTR_INGRESS_ACL_HANDLE: {
      switch_object_id_t ingress_acl_handle = {0};
      status = attr.v_get(ingress_acl_handle);
      if ((status != SWITCH_STATUS_SUCCESS) || (ingress_acl_handle.data == 0))
        return status;

      status = pfc_wd_on_acl_to_port_bound(object_id, ingress_acl_handle);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_PORT,
                   "{}.{}: PFC WD create failed for port handle {} and "
                   "ingress_acl_handle {} status {}",
                   __func__,
                   __LINE__,
                   object_id,
                   ingress_acl_handle,
                   status);
        return status;
      }
    } break;
    case SWITCH_PORT_ATTR_EGRESS_ACL_HANDLE: {
      switch_object_id_t egress_acl_handle = {0};
      status = attr.v_get(egress_acl_handle);
      if ((status != SWITCH_STATUS_SUCCESS) || (egress_acl_handle.data == 0))
        return status;

      status = pfc_wd_on_acl_to_port_bound(object_id, egress_acl_handle);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_PORT,
                   "{}.{}: PFC WD create failed for port handle {} and "
                   "egress_acl_handle {} status {}",
                   __func__,
                   __LINE__,
                   object_id,
                   egress_acl_handle,
                   status);
        return status;
      }
    } break;
    case SWITCH_PORT_ATTR_UNKNOWN_UNICAST_STORM_CONTROL:
    case SWITCH_PORT_ATTR_MULTICAST_STORM_CONTROL:
    case SWITCH_PORT_ATTR_BROADCAST_STORM_CONTROL: {
      switch_object_id_t meter_hdl{};
      status |= attr.v_get(meter_hdl);
      if (meter_hdl.data == 0) return status;
      const auto &port_attr_ref_map = switch_store::get_object_references(
          meter_hdl, SWITCH_OBJECT_TYPE_PORT);
      for (const auto &port_attr_ref : port_attr_ref_map) {
        auto port_handle = port_attr_ref.oid;
        auto ref_attr_id = port_attr_ref.attr_id;
        if (port_handle != object_id) {
          status = SWITCH_STATUS_RESOURCE_IN_USE;
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_PORT,
                     "{}.{}: Storm Control Meter set failed for port "
                     "handle:{:#x} attr:{}. Only 1:1 relationship is permitted "
                     "for port and storm control meter. Meter:{:#x} is already "
                     "referenced by port:{:#x} status {}",
                     __func__,
                     __LINE__,
                     object_id.data,
                     attr.id_get(),
                     meter_hdl.data,
                     port_handle.data,
                     status);
          return status;
        }
        if (ref_attr_id != attr.id_get()) {
          status = SWITCH_STATUS_RESOURCE_IN_USE;
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_PORT,
                     "{}.{}: Storm Control Meter set failed for port "
                     "handle:{:#x} attr:{}. Only 1:1 relationship is "
                     "permitted for port and storm control meter. Meter:{:#x} "
                     "is already referenced by port:{:#x} attr:{} status {}",
                     __func__,
                     __LINE__,
                     object_id.data,
                     attr.id_get(),
                     meter_hdl.data,
                     port_handle.data,
                     ref_attr_id,
                     status);
          return status;
        }
      }
      switch_enum_t target_type = {
          .enumdata = SWITCH_METER_ATTR_TARGET_TYPE_STORM_CONTROL};
      status |= switch_store::v_set(
          meter_hdl, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
      std::unique_ptr<object> storm_control_stats(
          factory::get_instance().create(
              SWITCH_OBJECT_TYPE_SC_STATS, meter_hdl, status));
      std::unique_ptr<object> storm_control(factory::get_instance().create(
          SWITCH_OBJECT_TYPE_STORM_CONTROL, meter_hdl, status));
      std::unique_ptr<object> storm_control_meter(
          factory::get_instance().create(
              SWITCH_OBJECT_TYPE_STORM_CONTROL_METER, meter_hdl, status));
      if (storm_control_meter != nullptr)
        status |= storm_control_meter->create_update();
      if (storm_control != nullptr) status |= storm_control->create_update();
      if (storm_control_stats != nullptr)
        status |= storm_control_stats->create_update();
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(
            SWITCH_API_LEVEL_ERROR,
            SWITCH_OBJECT_TYPE_PORT,
            "{}.{}: Storm Control Meter set failed for port handle:{:#x} "
            "attr {} meter_hdl:{:#x} status {}",
            __func__,
            __LINE__,
            object_id.data,
            attr.id_get(),
            meter_hdl.data,
            status);
        return status;
      }
    } break;
    case SWITCH_PORT_ATTR_INGRESS_QOS_DSCP_TOS_GROUP: {
      bool port_qos_config = false;
      status |= switch_store::v_get(
          object_id, SWITCH_PORT_ATTR_QOS_CONFIG_PRECEDENCE, port_qos_config);

      // currently we supp configuring this only on ports
      // [even if port is lag-mem]
      switch_object_id_t qos_dscp_tos_group = {0};
      status = switch_store::v_get(object_id,
                                   SWITCH_PORT_ATTR_INGRESS_QOS_DSCP_TOS_GROUP,
                                   qos_dscp_tos_group);
      if ((status != SWITCH_STATUS_SUCCESS) || (qos_dscp_tos_group.data == 0))
        return status;

      std::unique_ptr<object> qos_dscp_tos(factory::get_instance().create(
          SWITCH_OBJECT_TYPE_DSCP_TC_MAP, object_id, status));
      if (qos_dscp_tos != nullptr) qos_dscp_tos->create_update();
    } break;
    case SWITCH_PORT_ATTR_INGRESS_QOS_PCP_GROUP: {
      bool port_qos_config = false;
      status |= switch_store::v_get(
          object_id, SWITCH_PORT_ATTR_QOS_CONFIG_PRECEDENCE, port_qos_config);

      // currently we supp configuring this only on ports
      // [even if port is lag-mem]
      switch_object_id_t qos_pcp_group = {0};
      status = switch_store::v_get(
          object_id, SWITCH_PORT_ATTR_INGRESS_QOS_PCP_GROUP, qos_pcp_group);
      if ((status != SWITCH_STATUS_SUCCESS) || (qos_pcp_group.data == 0))
        return status;

      std::unique_ptr<object> qos_pcp(factory::get_instance().create(
          SWITCH_OBJECT_TYPE_PCP_TC_MAP, object_id, status));
      if (qos_pcp != nullptr) qos_pcp->create_update();
    } break;
    case SWITCH_PORT_ATTR_EGRESS_L3_QOS_GROUP: {
      bool port_qos_config = false;
      status |= switch_store::v_get(
          object_id, SWITCH_PORT_ATTR_QOS_CONFIG_PRECEDENCE, port_qos_config);

      switch_object_id_t egress_l3_qos_group = {0};
      status = switch_store::v_get(
          object_id, SWITCH_PORT_ATTR_EGRESS_L3_QOS_GROUP, egress_l3_qos_group);
      if ((status != SWITCH_STATUS_SUCCESS) || (egress_l3_qos_group.data == 0))
        return status;
      std::unique_ptr<object> l3_map_table(factory::get_instance().create(
          SWITCH_OBJECT_TYPE_L3_QOS_MAP_TABLE, object_id, status));
      if (l3_map_table != nullptr) l3_map_table->create_update();
    } break;
    case SWITCH_PORT_ATTR_EGRESS_L2_QOS_GROUP: {
      bool port_qos_config = false;
      status |= switch_store::v_get(
          object_id, SWITCH_PORT_ATTR_QOS_CONFIG_PRECEDENCE, port_qos_config);
      switch_object_id_t egress_l2_qos_group = {0};
      status = switch_store::v_get(
          object_id, SWITCH_PORT_ATTR_EGRESS_L2_QOS_GROUP, egress_l2_qos_group);
      if ((status != SWITCH_STATUS_SUCCESS) || (egress_l2_qos_group.data == 0))
        return status;
      std::unique_ptr<object> l2_map_table(factory::get_instance().create(
          SWITCH_OBJECT_TYPE_L2_QOS_MAP_TABLE, object_id, status));
      if (l2_map_table != nullptr) l2_map_table->create_update();
    } break;
    default:
      break;
  }
  return status;
}

class egress_cpu_port_rewrite : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_EGRESS_CPU_PORT_REWRITE;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_EGRESS_CPU_PORT_REWRITE_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_EGRESS_CPU_PORT_REWRITE_ATTR_STATUS;

 public:
  egress_cpu_port_rewrite(const switch_object_id_t parent,
                          switch_status_t &status)
      : p4_object_match_action(smi_id::T_EGRESS_CPU_PORT_REWRITE,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_enum_t port_type = {};
    uint16_t dp{};
    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_TYPE, port_type);
    if (port_type.enumdata != SWITCH_PORT_ATTR_TYPE_CPU) return;
    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_DEV_PORT, dp);
    bf_dev_pipe_t pipe = DEV_PORT_TO_PIPE(dp);
    auto &&switch_egress_pipes = SWITCH_CONTEXT.get_switch_egress_pipe_list();
    if (std::find(switch_egress_pipes.begin(),
                  switch_egress_pipes.end(),
                  pipe) != switch_egress_pipes.end()) {
      status |= match_key.set_exact(smi_id::F_EGRESS_CPU_PORT_REWRITE_PORT,
                                    parent,
                                    SWITCH_PORT_ATTR_DEV_PORT);
    } else {
      switch_object_id_t device{};
      status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_DEVICE, device);
      status |= match_key.set_exact(smi_id::F_EGRESS_CPU_PORT_REWRITE_PORT,
                                    device,
                                    SWITCH_DEVICE_ATTR_FOLDED_CPU_DEV_PORT);
    }

    action_entry.init_action_data(smi_id::A_CPU_REWRITE);
  }
};

class snake_table : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_SNAKE_TABLE;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_SNAKE_TABLE_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id = SWITCH_SNAKE_TABLE_ATTR_STATUS;

 public:
  snake_table(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(
            smi_id::T_SNAKE, status_attr_id, auto_ot, parent_attr_id, parent) {
    switch_object_id_t ingress_port_handle{}, egress_port_handle{};
    switch_enum_t ingress_port_type{.enumdata = SWITCH_PORT_ATTR_TYPE_CPU},
        egress_port_type{.enumdata = SWITCH_PORT_ATTR_TYPE_CPU};
    uint16_t in_dev_port = 0, eg_dev_port = 0;
    status |= switch_store::v_get(
        parent, SWITCH_SNAKE_ATTR_INGRESS_PORT_HANDLE, ingress_port_handle);
    status |= switch_store::v_get(
        parent, SWITCH_SNAKE_ATTR_EGRESS_PORT_HANDLE, egress_port_handle);
    status |= switch_store::v_get(
        ingress_port_handle, SWITCH_PORT_ATTR_TYPE, ingress_port_type);
    status |= switch_store::v_get(
        egress_port_handle, SWITCH_PORT_ATTR_TYPE, egress_port_type);
    if (ingress_port_type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL ||
        egress_port_type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL) {
      status = SWITCH_STATUS_INVALID_PARAMETER;
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_SNAKE,
                 "{}.{}: Snake table entry create failed. Ingress and Egress "
                 "port should both be of type NORMAL({}). Ingress port "
                 "handle:{:#x}, type:{}, Egress port handle:{:#x}, type:{}, "
                 "status {}",
                 __func__,
                 __LINE__,
                 SWITCH_PORT_ATTR_TYPE_NORMAL,
                 ingress_port_handle.data,
                 ingress_port_type,
                 egress_port_handle.data,
                 egress_port_type,
                 status);
      return;
    }
    status |= switch_store::v_get(
        ingress_port_handle, SWITCH_PORT_ATTR_DEV_PORT, in_dev_port);
    status |= switch_store::v_get(
        egress_port_handle, SWITCH_PORT_ATTR_DEV_PORT, eg_dev_port);
    status |= match_key.set_exact(smi_id::F_SNAKE_INGRESS_PORT,
                                  ingress_port_handle,
                                  SWITCH_PORT_ATTR_DEV_PORT);
    action_entry.init_action_data(smi_id::A_SNAKE_SET_EGRESS_PORT);
    status |= action_entry.set_arg(smi_id::P_SNAKE_SET_EGRESS_PORT_EGRESS_PORT,
                                   egress_port_handle,
                                   SWITCH_PORT_ATTR_DEV_PORT);
  }
};

switch_status_t before_port_update2(const switch_object_id_t port_handle,
                                    const attr_w &attr) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  switch (attr.id_get()) {
    case SWITCH_PORT_ATTR_INGRESS_ACL_HANDLE: {
      switch_object_id_t curr_ingress_acl_handle = {0};
      status = switch_store::v_get(port_handle,
                                   SWITCH_PORT_ATTR_INGRESS_ACL_HANDLE,
                                   curr_ingress_acl_handle);
      if ((status != SWITCH_STATUS_SUCCESS) ||
          (curr_ingress_acl_handle.data == 0))
        return status;

      // New acl handle is bound - remove all the pfc_wd objects associated
      // with
      // the old one
      status = pfc_wd_on_acl_to_port_unbound(
          port_handle, SWITCH_PFC_WD_ATTR_DIRECTION_INGRESS);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_PORT,
                   "{}.{}: PFC WD delete failed for port handle {} status {}",
                   __func__,
                   __LINE__,
                   port_handle,
                   status);
        return status;
      }
    } break;
    case SWITCH_PORT_ATTR_EGRESS_ACL_HANDLE: {
      switch_object_id_t curr_egress_acl_handle = {0};
      status = switch_store::v_get(port_handle,
                                   SWITCH_PORT_ATTR_EGRESS_ACL_HANDLE,
                                   curr_egress_acl_handle);
      if ((status != SWITCH_STATUS_SUCCESS) ||
          (curr_egress_acl_handle.data == 0))
        return status;

      // New acl handle is bound - remove all the pfc_wd objects associated
      // with
      // the old one
      status = pfc_wd_on_acl_to_port_unbound(
          port_handle, SWITCH_PFC_WD_ATTR_DIRECTION_EGRESS);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_PORT,
                   "{}.{}: PFC WD delete failed for port handle {} status {}",
                   __func__,
                   __LINE__,
                   port_handle,
                   status);
        return status;
      }
    } break;
    case SWITCH_PORT_ATTR_UNKNOWN_UNICAST_STORM_CONTROL:
    case SWITCH_PORT_ATTR_MULTICAST_STORM_CONTROL:
    case SWITCH_PORT_ATTR_BROADCAST_STORM_CONTROL: {
      switch_object_id_t meter_hdl{}, old_meter_hdl{};
      status |= attr.v_get(meter_hdl);
      status |= switch_store::v_get(port_handle, attr.id_get(), old_meter_hdl);
      if (status == SWITCH_STATUS_SUCCESS && old_meter_hdl.data != 0 &&
          old_meter_hdl.data != meter_hdl.data) {
        std::unique_ptr<object> storm_control_stats(
            factory::get_instance().create(
                SWITCH_OBJECT_TYPE_SC_STATS, old_meter_hdl, status));
        std::unique_ptr<object> storm_control(factory::get_instance().create(
            SWITCH_OBJECT_TYPE_STORM_CONTROL, old_meter_hdl, status));
        std::unique_ptr<object> storm_control_meter(
            factory::get_instance().create(
                SWITCH_OBJECT_TYPE_STORM_CONTROL_METER, old_meter_hdl, status));
        if (storm_control_stats != nullptr)
          status |= storm_control_stats->del();
        if (storm_control != nullptr) status |= storm_control->del();
        if (storm_control_meter != nullptr)
          status |= storm_control_meter->del();
        if (status != SWITCH_STATUS_SUCCESS) {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_PORT,
                     "{}.{}: Storm Control Meter set failed for port handle {} "
                     "attr {} status {}",
                     __func__,
                     __LINE__,
                     port_handle,
                     attr.id_get(),
                     status);
          return status;
        }
        // The old meter is no longer associated to any port object. So
        // remove
        // the meter type as Storm control and
        // reset it back to NONE. This will be reset to correct meter type
        // when
        // this meter gets associated with
        // another PORT object via UC/BC/MC SC port attr
        switch_enum_t target_type = {.enumdata =
                                         SWITCH_METER_ATTR_TARGET_TYPE_NONE};
        status |= switch_store::v_set(
            old_meter_hdl, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
      }
    } break;
    case SWITCH_PORT_ATTR_INGRESS_QOS_DSCP_TOS_GROUP: {
      bool port_qos_config = false;
      status |= switch_store::v_get(
          port_handle, SWITCH_PORT_ATTR_QOS_CONFIG_PRECEDENCE, port_qos_config);

      // currently we supp configuring this only on ports
      // [even if port is lag-mem]
      switch_object_id_t curr_qos_dscp_tos_group = {0};
      status = switch_store::v_get(port_handle,
                                   SWITCH_PORT_ATTR_INGRESS_QOS_DSCP_TOS_GROUP,
                                   curr_qos_dscp_tos_group);
      if ((status != SWITCH_STATUS_SUCCESS) ||
          (curr_qos_dscp_tos_group.data == 0))
        return status;

      std::unique_ptr<object> qos_dscp_tos(factory::get_instance().create(
          SWITCH_OBJECT_TYPE_DSCP_TC_MAP, port_handle, status));
      if (qos_dscp_tos != nullptr) qos_dscp_tos->del();
    } break;
    case SWITCH_PORT_ATTR_INGRESS_QOS_PCP_GROUP: {
      bool port_qos_config = false;
      status |= switch_store::v_get(
          port_handle, SWITCH_PORT_ATTR_QOS_CONFIG_PRECEDENCE, port_qos_config);

      // currently we supp configuring this only on ports
      // [even if port is lag-mem]
      switch_object_id_t curr_qos_pcp_group = {0};
      status = switch_store::v_get(port_handle,
                                   SWITCH_PORT_ATTR_INGRESS_QOS_PCP_GROUP,
                                   curr_qos_pcp_group);
      if ((status != SWITCH_STATUS_SUCCESS) || (curr_qos_pcp_group.data == 0))
        return status;

      std::unique_ptr<object> qos_pcp(factory::get_instance().create(
          SWITCH_OBJECT_TYPE_PCP_TC_MAP, port_handle, status));
      if (qos_pcp != nullptr) qos_pcp->del();
    } break;
    case SWITCH_PORT_ATTR_EGRESS_L3_QOS_GROUP: {
      bool port_qos_config = false;
      status |= switch_store::v_get(
          port_handle, SWITCH_PORT_ATTR_QOS_CONFIG_PRECEDENCE, port_qos_config);
      switch_object_id_t egress_l3_qos_group = {0};
      status = switch_store::v_get(port_handle,
                                   SWITCH_PORT_ATTR_EGRESS_L3_QOS_GROUP,
                                   egress_l3_qos_group);
      if ((status != SWITCH_STATUS_SUCCESS) || (egress_l3_qos_group.data == 0))
        return status;
      std::unique_ptr<object> l3_map_table(factory::get_instance().create(
          SWITCH_OBJECT_TYPE_L3_QOS_MAP_TABLE, port_handle, status));
      if (l3_map_table != nullptr) l3_map_table->del();
    } break;
    case SWITCH_PORT_ATTR_EGRESS_L2_QOS_GROUP: {
      bool port_qos_config = false;
      status |= switch_store::v_get(
          port_handle, SWITCH_PORT_ATTR_QOS_CONFIG_PRECEDENCE, port_qos_config);
      switch_object_id_t egress_l2_qos_group = {0};
      status = switch_store::v_get(port_handle,
                                   SWITCH_PORT_ATTR_EGRESS_L2_QOS_GROUP,
                                   egress_l2_qos_group);
      if ((status != SWITCH_STATUS_SUCCESS) || (egress_l2_qos_group.data == 0))
        return status;
      std::unique_ptr<object> l2_map_table(factory::get_instance().create(
          SWITCH_OBJECT_TYPE_L2_QOS_MAP_TABLE, port_handle, status));
      if (l2_map_table != nullptr) l2_map_table->del();
    } break;
    default:
      break;
  }
  return status;
}

switch_status_t after_port_create2(const switch_object_id_t object_id,
                                   const std::set<attr_w> &attrs) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  for (const auto &attr : attrs) {
    status = after_port_update2(object_id, attr);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_PORT,
                 "{}.{}: Failed to create port for attr {} status {}",
                 __func__,
                 __LINE__,
                 attr.id_get(),
                 status);
      return status;
    }
  }
  return status;
}

switch_status_t before_port_delete2(const switch_object_id_t object_id) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t ingress_acl_handle = {0}, egress_acl_handle = {0};

  status |= switch_store::v_get(
      object_id, SWITCH_PORT_ATTR_INGRESS_ACL_HANDLE, ingress_acl_handle);
  if (status != SWITCH_STATUS_SUCCESS) return status;

  if (ingress_acl_handle.data != 0) {
    status = pfc_wd_on_acl_to_port_unbound(
        object_id, SWITCH_PFC_WD_ATTR_DIRECTION_INGRESS);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_PORT,
                 "{}.{}: PFC WD delete failed for port handle {} status {}",
                 __func__,
                 __LINE__,
                 object_id,
                 status);
      return status;
    }
  }

  status |= switch_store::v_get(
      object_id, SWITCH_PORT_ATTR_EGRESS_ACL_HANDLE, egress_acl_handle);
  if (status != SWITCH_STATUS_SUCCESS) return status;

  if (egress_acl_handle.data != 0) {
    status = pfc_wd_on_acl_to_port_unbound(object_id,
                                           SWITCH_PFC_WD_ATTR_DIRECTION_EGRESS);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_PORT,
                 "{}.{}: PFC WD delete failed for port handle {} status {}",
                 __func__,
                 __LINE__,
                 object_id,
                 status);
      return status;
    }
  }

  std::vector<switch_object_id_t> storm_control_meters;
  switch_object_id_t meter_oid{};
  status |= switch_store::v_get(
      object_id, SWITCH_PORT_ATTR_UNKNOWN_UNICAST_STORM_CONTROL, meter_oid);
  if (status == SWITCH_STATUS_SUCCESS && meter_oid.data != 0) {
    storm_control_meters.push_back(meter_oid);
  }
  meter_oid.data = 0;
  status |= switch_store::v_get(
      object_id, SWITCH_PORT_ATTR_MULTICAST_STORM_CONTROL, meter_oid);
  if (status == SWITCH_STATUS_SUCCESS && meter_oid.data != 0) {
    storm_control_meters.push_back(meter_oid);
  }
  meter_oid.data = 0;
  status |= switch_store::v_get(
      object_id, SWITCH_PORT_ATTR_BROADCAST_STORM_CONTROL, meter_oid);
  if (status == SWITCH_STATUS_SUCCESS && meter_oid.data != 0) {
    storm_control_meters.push_back(meter_oid);
  }
  CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);
  for (auto &&meter : storm_control_meters) {
    std::unique_ptr<object> storm_control_stats(factory::get_instance().create(
        SWITCH_OBJECT_TYPE_SC_STATS, meter, status));
    std::unique_ptr<object> storm_control(factory::get_instance().create(
        SWITCH_OBJECT_TYPE_STORM_CONTROL, meter, status));
    std::unique_ptr<object> storm_control_meter(factory::get_instance().create(
        SWITCH_OBJECT_TYPE_STORM_CONTROL_METER, meter, status));
    if (storm_control_stats != nullptr) status |= storm_control_stats->del();
    if (storm_control != nullptr) status |= storm_control->del();
    if (storm_control_meter != nullptr) status |= storm_control_meter->del();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(
          SWITCH_API_LEVEL_ERROR,
          SWITCH_OBJECT_TYPE_PORT,
          "{}.{}: Storm Control Meter delete3 failed for port handle {:#x} "
          "status {}",
          __func__,
          __LINE__,
          object_id.data,
          status);
      return status;
    }
    // The old meter is no longer associated to any port object. So remove
    // the meter type as Storm control and
    // reset it back to NONE. This will be reset to correct meter type when
    // this meter gets associated with
    // another PORT object via UC/BC/MC SC port attr
    switch_enum_t target_type = {.enumdata =
                                     SWITCH_METER_ATTR_TARGET_TYPE_NONE};
    status |=
        switch_store::v_set(meter, SWITCH_METER_ATTR_TARGET_TYPE, target_type);
    CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);
  }

  return status;
}

class ingress_port_ip_stats : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_INGRESS_PORT_IP_STATS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INGRESS_PORT_IP_STATS_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_INGRESS_PORT_IP_STATS_ATTR_STATUS;
  struct rule_spec {
    uint64_t port;
    uint8_t ipv4_valid;
    uint8_t ipv4_valid_mask;
    uint8_t ipv6_valid;
    uint8_t ipv6_valid_mask;
    uint8_t drop;
    uint8_t drop_mask;
    uint8_t copy_to_cpu;
    uint8_t copy_to_cpu_mask;
    const char *dstAddr;
    const char *dstAddr_mask;
    uint64_t priority;
  };

 public:
  ingress_port_ip_stats(const switch_object_id_t parent,
                        switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_INGRESS_PORT_IP_STATS,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    uint16_t dev_port = 0;
    switch_enum_t port_type = {};
    CHECK_PORT_IN_SWITCH_PIPE_AND_RETURN(parent);
    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
    device_tgt_set(compute_dev_target_for_table(
        dev_port, smi_id::T_INGRESS_PORT_IP_STATS, true));

    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_TYPE, port_type);
    if (port_type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL) return;

    // {port, is_ipv4, is_ipv6, drop, copy_to_cpu, eth.dst_addr, priority}
    const std::vector<rule_spec> rules = {
        {dev_port, 1, 1, 0, 0, 1, 1, 0, 0, eth_0s, eth_0s, 0},  // ipv4 discards
        {dev_port, 1, 1, 0, 0, 0, 0, 0, 0, eth_1s, eth_mc, 1},  // ipv4 non_uc
        {dev_port, 1, 1, 0, 0, 0, 0, 0, 0, eth_0s, eth_mc, 2},  // ipv4 uc
        {dev_port, 0, 0, 1, 1, 1, 1, 0, 0, eth_0s, eth_0s, 3},  // ipv6 discards
        {dev_port, 0, 0, 1, 1, 0, 0, 0, 0, eth_1s, eth_1s, 4},  // ipv6 bc ??
        {dev_port, 0, 0, 1, 1, 0, 0, 0, 0, eth_mc, eth_mc, 5},  // ipv6
                                                                // multicast
        {dev_port, 0, 0, 1, 1, 0, 0, 0, 0, eth_0s, eth_mc, 6}   // ipv6 uc
    };

    std::vector<bf_rt_id_t> cntrs{
        smi_id::D_INGRESS_PORT_IP_STATS_COUNTER_SPEC_PKTS,
        smi_id::D_INGRESS_PORT_IP_STATS_COUNTER_SPEC_BYTES};
    uint64_t pkts = 0, bytes = 0;
    auto it = match_action_list.begin();
    uint32_t i = 0;
    for (i = 0; i < rules.size(); i++) {
      it = match_action_list.insert(
          it,
          std::pair<_MatchKey, _ActionEntry>(
              _MatchKey(smi_id::T_INGRESS_PORT_IP_STATS),
              _ActionEntry(smi_id::T_INGRESS_PORT_IP_STATS)));
      status |= it->first.set_exact(smi_id::F_INGRESS_PORT_IP_STATS_PORT,
                                    static_cast<uint64_t>(rules[i].port));
      status |=
          it->first.set_ternary(smi_id::F_INGRESS_PORT_IP_STATS_HDR_IPV4_VALID,
                                rules[i].ipv4_valid,
                                rules[i].ipv4_valid_mask);
      status |=
          it->first.set_ternary(smi_id::F_INGRESS_PORT_IP_STATS_HDR_IPV6_VALID,
                                rules[i].ipv6_valid,
                                rules[i].ipv6_valid_mask);
      status |= it->first.set_ternary(smi_id::F_INGRESS_PORT_IP_STATS_DROP,
                                      rules[i].drop,
                                      rules[i].drop_mask);
      status |=
          it->first.set_ternary(smi_id::F_INGRESS_PORT_IP_STATS_COPY_TO_CPU,
                                rules[i].copy_to_cpu,
                                rules[i].copy_to_cpu_mask);
      status |= it->first.set_ternary(
          smi_id::F_INGRESS_PORT_IP_STATS_HDR_ETHERNET_DST_ADDR,
          rules[i].dstAddr,
          rules[i].dstAddr_mask,
          ETH_LEN);
      status |= it->first.set_exact(smi_id::F_INGRESS_PORT_IP_STATS_PRIORITY,
                                    static_cast<uint64_t>(i));
      it->second.init_action_data(smi_id::A_INGRESS_PORT_IP_STATS_COUNT, cntrs);
      status |= it->second.set_arg(
          smi_id::D_INGRESS_PORT_IP_STATS_COUNTER_SPEC_PKTS, pkts);
      status |= it->second.set_arg(
          smi_id::D_INGRESS_PORT_IP_STATS_COUNTER_SPEC_BYTES, bytes);
    }
  }
  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t port_type = {};
    status |= switch_store::v_get(handle, SWITCH_PORT_ATTR_TYPE, port_type);
    if (port_type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL) return status;
    struct rule_spec ip_entry;
    p4_object_match_action_list::data_get();

    for (auto const &entry : match_action_list) {
      uint64_t pkts = 0, bytes = 0;
      entry.second.get_arg(smi_id::D_INGRESS_PORT_IP_STATS_COUNTER_SPEC_PKTS,
                           smi_id::A_INGRESS_PORT_IP_STATS_COUNT,
                           &pkts);

      if (pkts == 0) continue;
      entry.second.get_arg(smi_id::D_INGRESS_PORT_IP_STATS_COUNTER_SPEC_BYTES,
                           smi_id::A_INGRESS_PORT_IP_STATS_COUNT,
                           &bytes);
      if (bytes == 0) continue;

      entry.first.get_exact(smi_id::F_INGRESS_PORT_IP_STATS_PRIORITY,
                            &ip_entry.priority);

      if (0 == ip_entry.priority) {
        cntrs[SWITCH_PORT_COUNTER_ID_IP_IN_DISCARDS].counter_id =
            SWITCH_PORT_COUNTER_ID_IP_IN_DISCARDS;
        cntrs[SWITCH_PORT_COUNTER_ID_IP_IN_DISCARDS].count += pkts;
        cntrs[SWITCH_PORT_COUNTER_ID_IP_IN_RECEIVES].counter_id =
            SWITCH_PORT_COUNTER_ID_IP_IN_RECEIVES;
        cntrs[SWITCH_PORT_COUNTER_ID_IP_IN_RECEIVES].count += pkts;
        cntrs[SWITCH_PORT_COUNTER_ID_IP_IN_OCTETS].counter_id =
            SWITCH_PORT_COUNTER_ID_IP_IN_OCTETS;
        cntrs[SWITCH_PORT_COUNTER_ID_IP_IN_OCTETS].count += bytes;

      } else if (1 == ip_entry.priority) {
        cntrs[SWITCH_PORT_COUNTER_ID_IP_IN_NON_UCAST_PKTS].counter_id =
            SWITCH_PORT_COUNTER_ID_IP_IN_NON_UCAST_PKTS;
        cntrs[SWITCH_PORT_COUNTER_ID_IP_IN_NON_UCAST_PKTS].count += pkts;
        cntrs[SWITCH_PORT_COUNTER_ID_IP_IN_RECEIVES].counter_id =
            SWITCH_PORT_COUNTER_ID_IP_IN_RECEIVES;
        cntrs[SWITCH_PORT_COUNTER_ID_IP_IN_RECEIVES].count += pkts;
        cntrs[SWITCH_PORT_COUNTER_ID_IP_IN_OCTETS].counter_id =
            SWITCH_PORT_COUNTER_ID_IP_IN_OCTETS;
        cntrs[SWITCH_PORT_COUNTER_ID_IP_IN_OCTETS].count += bytes;
      } else if (2 == ip_entry.priority) {
        cntrs[SWITCH_PORT_COUNTER_ID_IP_IN_UCAST_PKTS].counter_id =
            SWITCH_PORT_COUNTER_ID_IP_IN_UCAST_PKTS;
        cntrs[SWITCH_PORT_COUNTER_ID_IP_IN_UCAST_PKTS].count += pkts;
        cntrs[SWITCH_PORT_COUNTER_ID_IP_IN_RECEIVES].counter_id =
            SWITCH_PORT_COUNTER_ID_IP_IN_RECEIVES;
        cntrs[SWITCH_PORT_COUNTER_ID_IP_IN_RECEIVES].count += pkts;
        cntrs[SWITCH_PORT_COUNTER_ID_IP_IN_OCTETS].counter_id =
            SWITCH_PORT_COUNTER_ID_IP_IN_OCTETS;
        cntrs[SWITCH_PORT_COUNTER_ID_IP_IN_OCTETS].count += bytes;
      } else if (3 == ip_entry.priority) {
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_IN_DISCARDS].counter_id =
            SWITCH_PORT_COUNTER_ID_IPV6_IN_DISCARDS;
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_IN_DISCARDS].count += pkts;
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_IN_RECEIVES].counter_id =
            SWITCH_PORT_COUNTER_ID_IPV6_IN_RECEIVES;
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_IN_RECEIVES].count += pkts;
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_IN_OCTETS].counter_id =
            SWITCH_PORT_COUNTER_ID_IPV6_IN_OCTETS;
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_IN_OCTETS].count += bytes;
      } else if (4 == ip_entry.priority) {
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_IN_NON_UCAST_PKTS].counter_id =
            SWITCH_PORT_COUNTER_ID_IPV6_IN_NON_UCAST_PKTS;
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_IN_NON_UCAST_PKTS].count += pkts;
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_IN_RECEIVES].counter_id =
            SWITCH_PORT_COUNTER_ID_IPV6_IN_RECEIVES;
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_IN_RECEIVES].count += pkts;
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_IN_OCTETS].counter_id =
            SWITCH_PORT_COUNTER_ID_IPV6_IN_OCTETS;
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_IN_OCTETS].count += bytes;
      } else if (5 == ip_entry.priority) {
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_IN_MCAST_PKTS].counter_id =
            SWITCH_PORT_COUNTER_ID_IPV6_IN_MCAST_PKTS;
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_IN_MCAST_PKTS].count += pkts;
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_IN_RECEIVES].counter_id =
            SWITCH_PORT_COUNTER_ID_IPV6_IN_RECEIVES;
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_IN_RECEIVES].count += pkts;
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_IN_OCTETS].counter_id =
            SWITCH_PORT_COUNTER_ID_IPV6_IN_OCTETS;
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_IN_OCTETS].count += bytes;
      } else if (6 == ip_entry.priority) {
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_IN_UCAST_PKTS].counter_id =
            SWITCH_PORT_COUNTER_ID_IPV6_IN_UCAST_PKTS;
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_IN_UCAST_PKTS].count += pkts;
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_IN_RECEIVES].counter_id =
            SWITCH_PORT_COUNTER_ID_IPV6_IN_RECEIVES;
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_IN_RECEIVES].count += pkts;
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_IN_OCTETS].counter_id =
            SWITCH_PORT_COUNTER_ID_IPV6_IN_OCTETS;
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_IN_OCTETS].count += bytes;
      }
    }
    return status;
  }

  switch_status_t counters_set(const switch_object_id_t handle) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t port_type = {};

    status |= switch_store::v_get(handle, SWITCH_PORT_ATTR_TYPE, port_type);
    if (port_type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL) return status;

    for (auto &entry : match_action_list) {
      uint64_t value = 0;
      entry.second.set_arg(smi_id::D_INGRESS_PORT_IP_STATS_COUNTER_SPEC_PKTS,
                           value);
      entry.second.set_arg(smi_id::D_INGRESS_PORT_IP_STATS_COUNTER_SPEC_BYTES,
                           value);
    }
    return p4_object_match_action_list::data_set();
  }
  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t port_type = {};

    status |= switch_store::v_get(handle, SWITCH_PORT_ATTR_TYPE, port_type);
    if (port_type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL) return status;

    for (auto cntr_id : cntr_ids) {
      switch (cntr_id) {
        case SWITCH_PORT_COUNTER_ID_IP_IN_DISCARDS:
        case SWITCH_PORT_COUNTER_ID_IP_IN_RECEIVES:
        case SWITCH_PORT_COUNTER_ID_IP_IN_OCTETS:
        case SWITCH_PORT_COUNTER_ID_IP_IN_UCAST_PKTS:
        case SWITCH_PORT_COUNTER_ID_IP_IN_NON_UCAST_PKTS:
        case SWITCH_PORT_COUNTER_ID_IPV6_IN_RECEIVES:
        case SWITCH_PORT_COUNTER_ID_IPV6_IN_OCTETS:
        case SWITCH_PORT_COUNTER_ID_IPV6_IN_UCAST_PKTS:
        case SWITCH_PORT_COUNTER_ID_IPV6_IN_NON_UCAST_PKTS:
        case SWITCH_PORT_COUNTER_ID_IPV6_IN_MCAST_PKTS:
        case SWITCH_PORT_COUNTER_ID_IPV6_IN_DISCARDS:
          return counters_set(handle);
        default:
          break;
      }
    }
    return SWITCH_STATUS_SUCCESS;
  }
  switch_status_t counters_save(const switch_object_id_t parent) {
    (void)parent;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t ctr_pkt = 0;
    uint64_t ctr_byte = 0;

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    p4_object_match_action_list::data_get();
    for (auto const &entry : match_action_list) {
      ctr_pkt = 0;
      ctr_byte = 0;
      entry.second.get_arg(smi_id::D_INGRESS_PORT_IP_STATS_COUNTER_SPEC_PKTS,
                           smi_id::A_INGRESS_PORT_IP_STATS_COUNT,
                           &ctr_pkt);
      entry.second.get_arg(smi_id::D_INGRESS_PORT_IP_STATS_COUNTER_SPEC_BYTES,
                           smi_id::A_INGRESS_PORT_IP_STATS_COUNT,
                           &ctr_byte);
      ctr_list.push_back(static_cast<uint64_t>(ctr_pkt));
      ctr_list.push_back(static_cast<uint64_t>(ctr_byte));
    }
    attr_w ctr_attr_list(SWITCH_INGRESS_PORT_IP_STATS_ATTR_MAU_STATS_CACHE);
    ctr_attr_list.v_set(ctr_list);
    switch_store::attribute_set(get_auto_oid(), ctr_attr_list);

    return status;
  }
  switch_status_t counters_restore(const switch_object_id_t parent) {
    (void)parent;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    size_t list_i;

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    status |=
        switch_store::v_get(get_auto_oid(),
                            SWITCH_INGRESS_PORT_IP_STATS_ATTR_MAU_STATS_CACHE,
                            ctr_list);
    if (ctr_list.empty()) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_PORT,
                 "{}.{}: No stat cache to restore mau stats, "
                 "ingress_drop_stats cache list empty, status {}",
                 __func__,
                 __LINE__,
                 status);
      return SWITCH_STATUS_SUCCESS;
    }

    list_i = 0;
    for (auto &entry : match_action_list) {
      entry.second.set_arg(smi_id::D_INGRESS_PORT_IP_STATS_COUNTER_SPEC_PKTS,
                           ctr_list[list_i]);
      list_i++;
      entry.second.set_arg(smi_id::D_INGRESS_PORT_IP_STATS_COUNTER_SPEC_BYTES,
                           ctr_list[list_i]);
      list_i++;
    }
    status = p4_object_match_action_list::data_set();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_PORT,
                 "{}.{}: Fail to restore mau stats cache,  "
                 "ingress_drop_stats status {} ",
                 __func__,
                 __LINE__,
                 status);
    }

    return status;
  }
};

class egress_port_ip_stats : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_EGRESS_PORT_IP_STATS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_EGRESS_PORT_IP_STATS_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_EGRESS_PORT_IP_STATS_ATTR_STATUS;
  struct rule_spec {
    uint64_t port;
    uint8_t ipv4_valid;
    uint8_t ipv4_valid_mask;
    uint8_t ipv6_valid;
    uint8_t ipv6_valid_mask;
    uint8_t drop;
    uint8_t drop_mask;
    uint8_t copy_to_cpu;
    uint8_t copy_to_cpu_mask;
    const char *dstAddr;
    const char *dstAddr_mask;
    uint64_t priority;
  };

 public:
  egress_port_ip_stats(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_EGRESS_PORT_IP_STATS,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    uint16_t dev_port = 0;
    switch_enum_t port_type = {};
    CHECK_PORT_IN_SWITCH_PIPE_AND_RETURN(parent);
    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
    device_tgt_set(compute_dev_target_for_table(
        dev_port, smi_id::T_EGRESS_PORT_IP_STATS, false));

    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_TYPE, port_type);
    if (port_type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL) return;

    // {port, is_ipv4, is_ipv6, drop, copy_to_cpu, eth.dst_addr, priority}
    const std::vector<rule_spec> rules = {
        {dev_port, 1, 1, 0, 0, 1, 1, 0, 0, eth_0s, eth_0s, 0},  // ipv4 discards
        {dev_port, 1, 1, 0, 0, 0, 0, 0, 0, eth_1s, eth_mc, 1},  // ipv4 non_uc
        {dev_port, 1, 1, 0, 0, 0, 0, 0, 0, eth_0s, eth_mc, 2},  // ipv4 uc
        {dev_port, 0, 0, 1, 1, 1, 1, 0, 0, eth_0s, eth_0s, 3},  // ipv6 discards
        {dev_port, 0, 0, 1, 1, 0, 0, 0, 0, eth_1s, eth_1s, 4},  // ipv6 bc ??
        {dev_port, 0, 0, 1, 1, 0, 0, 0, 0, eth_mc, eth_mc, 5},  // ipv6
                                                                // multicast
        {dev_port, 0, 0, 1, 1, 0, 0, 0, 0, eth_0s, eth_mc, 6}   // ipv6 uc
    };

    std::vector<bf_rt_id_t> cntrs{
        smi_id::D_EGRESS_PORT_IP_STATS_COUNTER_SPEC_PKTS,
        smi_id::D_EGRESS_PORT_IP_STATS_COUNTER_SPEC_BYTES};
    uint64_t pkts = 0, bytes = 0;
    auto it = match_action_list.begin();
    uint32_t i = 0;
    for (i = 0; i < rules.size(); i++) {
      it = match_action_list.insert(
          it,
          std::pair<_MatchKey, _ActionEntry>(
              _MatchKey(smi_id::T_EGRESS_PORT_IP_STATS),
              _ActionEntry(smi_id::T_EGRESS_PORT_IP_STATS)));
      status |= it->first.set_exact(smi_id::F_EGRESS_PORT_IP_STATS_PORT,
                                    static_cast<uint64_t>(rules[i].port));
      status |=
          it->first.set_ternary(smi_id::F_EGRESS_PORT_IP_STATS_HDR_IPV4_VALID,
                                rules[i].ipv4_valid,
                                rules[i].ipv4_valid_mask);
      status |=
          it->first.set_ternary(smi_id::F_EGRESS_PORT_IP_STATS_HDR_IPV6_VALID,
                                rules[i].ipv6_valid,
                                rules[i].ipv6_valid_mask);
      status |= it->first.set_ternary(smi_id::F_EGRESS_PORT_IP_STATS_DROP,
                                      rules[i].drop,
                                      rules[i].drop_mask);
      status |=
          it->first.set_ternary(smi_id::F_EGRESS_PORT_IP_STATS_COPY_TO_CPU,
                                rules[i].copy_to_cpu,
                                rules[i].copy_to_cpu_mask);
      status |= it->first.set_ternary(
          smi_id::F_EGRESS_PORT_IP_STATS_HDR_ETHERNET_DST_ADDR,
          rules[i].dstAddr,
          rules[i].dstAddr_mask,
          ETH_LEN);
      status |= it->first.set_exact(smi_id::F_EGRESS_PORT_IP_STATS_PRIORITY,
                                    static_cast<uint64_t>(i));
      it->second.init_action_data(smi_id::A_EGRESS_PORT_IP_STATS_COUNT, cntrs);
      status |= it->second.set_arg(
          smi_id::D_EGRESS_PORT_IP_STATS_COUNTER_SPEC_PKTS, pkts);
      status |= it->second.set_arg(
          smi_id::D_EGRESS_PORT_IP_STATS_COUNTER_SPEC_BYTES, bytes);
    }
  }
  switch_status_t counters_get(const switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntrs) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t port_type = {};
    status |= switch_store::v_get(handle, SWITCH_PORT_ATTR_TYPE, port_type);
    if (port_type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL) return status;
    struct rule_spec ip_entry;
    p4_object_match_action_list::data_get();

    for (auto const &entry : match_action_list) {
      uint64_t pkts = 0, bytes = 0;
      entry.second.get_arg(smi_id::D_EGRESS_PORT_IP_STATS_COUNTER_SPEC_PKTS,
                           smi_id::A_EGRESS_PORT_IP_STATS_COUNT,
                           &pkts);

      if (pkts == 0) continue;
      entry.second.get_arg(smi_id::D_EGRESS_PORT_IP_STATS_COUNTER_SPEC_BYTES,
                           smi_id::A_EGRESS_PORT_IP_STATS_COUNT,
                           &bytes);
      if (bytes == 0) continue;
      entry.first.get_exact(smi_id::F_EGRESS_PORT_IP_STATS_PRIORITY,
                            &ip_entry.priority);

      if (0 == ip_entry.priority) {
        cntrs[SWITCH_PORT_COUNTER_ID_IP_OUT_DISCARDS].counter_id =
            SWITCH_PORT_COUNTER_ID_IP_OUT_DISCARDS;
        cntrs[SWITCH_PORT_COUNTER_ID_IP_OUT_DISCARDS].count += pkts;
        cntrs[SWITCH_PORT_COUNTER_ID_IP_OUT_OCTETS].counter_id =
            SWITCH_PORT_COUNTER_ID_IP_OUT_OCTETS;
        cntrs[SWITCH_PORT_COUNTER_ID_IP_OUT_OCTETS].count += bytes;

      } else if (1 == ip_entry.priority) {
        cntrs[SWITCH_PORT_COUNTER_ID_IP_OUT_NON_UCAST_PKTS].counter_id =
            SWITCH_PORT_COUNTER_ID_IP_OUT_NON_UCAST_PKTS;
        cntrs[SWITCH_PORT_COUNTER_ID_IP_OUT_NON_UCAST_PKTS].count += pkts;
        cntrs[SWITCH_PORT_COUNTER_ID_IP_OUT_OCTETS].counter_id =
            SWITCH_PORT_COUNTER_ID_IP_OUT_OCTETS;
        cntrs[SWITCH_PORT_COUNTER_ID_IP_OUT_OCTETS].count += bytes;
      } else if (2 == ip_entry.priority) {
        cntrs[SWITCH_PORT_COUNTER_ID_IP_OUT_UCAST_PKTS].counter_id =
            SWITCH_PORT_COUNTER_ID_IP_OUT_UCAST_PKTS;
        cntrs[SWITCH_PORT_COUNTER_ID_IP_OUT_UCAST_PKTS].count += pkts;
        cntrs[SWITCH_PORT_COUNTER_ID_IP_OUT_OCTETS].counter_id =
            SWITCH_PORT_COUNTER_ID_IP_OUT_OCTETS;
        cntrs[SWITCH_PORT_COUNTER_ID_IP_OUT_OCTETS].count += bytes;
      } else if (3 == ip_entry.priority) {
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_OUT_DISCARDS].counter_id =
            SWITCH_PORT_COUNTER_ID_IPV6_OUT_DISCARDS;
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_OUT_DISCARDS].count += pkts;
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_OUT_OCTETS].counter_id =
            SWITCH_PORT_COUNTER_ID_IPV6_OUT_OCTETS;
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_OUT_OCTETS].count += bytes;
      } else if (4 == ip_entry.priority) {
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_OUT_NON_UCAST_PKTS].counter_id =
            SWITCH_PORT_COUNTER_ID_IPV6_OUT_NON_UCAST_PKTS;
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_OUT_NON_UCAST_PKTS].count += pkts;
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_OUT_OCTETS].counter_id =
            SWITCH_PORT_COUNTER_ID_IPV6_OUT_OCTETS;
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_OUT_OCTETS].count += bytes;
      } else if (5 == ip_entry.priority) {
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_OUT_MCAST_PKTS].counter_id =
            SWITCH_PORT_COUNTER_ID_IPV6_OUT_MCAST_PKTS;
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_OUT_MCAST_PKTS].count += pkts;
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_OUT_OCTETS].counter_id =
            SWITCH_PORT_COUNTER_ID_IPV6_OUT_OCTETS;
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_OUT_OCTETS].count += bytes;
      } else if (6 == ip_entry.priority) {
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_OUT_UCAST_PKTS].counter_id =
            SWITCH_PORT_COUNTER_ID_IPV6_OUT_UCAST_PKTS;
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_OUT_UCAST_PKTS].count += pkts;
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_OUT_OCTETS].counter_id =
            SWITCH_PORT_COUNTER_ID_IPV6_OUT_OCTETS;
        cntrs[SWITCH_PORT_COUNTER_ID_IPV6_OUT_OCTETS].count += bytes;
      }
    }
    return status;
  }

  switch_status_t counters_set(const switch_object_id_t handle) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t port_type = {};

    status |= switch_store::v_get(handle, SWITCH_PORT_ATTR_TYPE, port_type);
    if (port_type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL) return status;

    for (auto &entry : match_action_list) {
      uint64_t value = 0;
      entry.second.set_arg(smi_id::D_EGRESS_PORT_IP_STATS_COUNTER_SPEC_PKTS,
                           value);
      entry.second.set_arg(smi_id::D_EGRESS_PORT_IP_STATS_COUNTER_SPEC_BYTES,
                           value);
    }
    return p4_object_match_action_list::data_set();
  }
  switch_status_t counters_set(const switch_object_id_t handle,
                               const std::vector<uint16_t> &cntr_ids) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t port_type = {};

    status |= switch_store::v_get(handle, SWITCH_PORT_ATTR_TYPE, port_type);
    if (port_type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL) return status;

    for (auto cntr_id : cntr_ids) {
      switch (cntr_id) {
        case SWITCH_PORT_COUNTER_ID_IP_OUT_DISCARDS:
        case SWITCH_PORT_COUNTER_ID_IP_OUT_OCTETS:
        case SWITCH_PORT_COUNTER_ID_IP_OUT_UCAST_PKTS:
        case SWITCH_PORT_COUNTER_ID_IP_OUT_NON_UCAST_PKTS:
        case SWITCH_PORT_COUNTER_ID_IPV6_OUT_OCTETS:
        case SWITCH_PORT_COUNTER_ID_IPV6_OUT_UCAST_PKTS:
        case SWITCH_PORT_COUNTER_ID_IPV6_OUT_NON_UCAST_PKTS:
        case SWITCH_PORT_COUNTER_ID_IPV6_OUT_MCAST_PKTS:
        case SWITCH_PORT_COUNTER_ID_IPV6_OUT_DISCARDS:
          return counters_set(handle);
        default:
          break;
      }
    }
    return SWITCH_STATUS_SUCCESS;
  }
  switch_status_t counters_save(const switch_object_id_t parent) {
    (void)parent;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint64_t ctr_pkt = 0;
    uint64_t ctr_byte = 0;

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    p4_object_match_action_list::data_get();
    for (auto const &entry : match_action_list) {
      ctr_pkt = 0;
      ctr_byte = 0;
      entry.second.get_arg(smi_id::D_EGRESS_PORT_IP_STATS_COUNTER_SPEC_PKTS,
                           smi_id::A_EGRESS_PORT_IP_STATS_COUNT,
                           &ctr_pkt);
      entry.second.get_arg(smi_id::D_EGRESS_PORT_IP_STATS_COUNTER_SPEC_BYTES,
                           smi_id::A_EGRESS_PORT_IP_STATS_COUNT,
                           &ctr_byte);
      ctr_list.push_back(static_cast<uint64_t>(ctr_pkt));
      ctr_list.push_back(static_cast<uint64_t>(ctr_byte));
    }
    attr_w ctr_attr_list(SWITCH_EGRESS_PORT_IP_STATS_ATTR_MAU_STATS_CACHE);
    ctr_attr_list.v_set(ctr_list);
    switch_store::attribute_set(get_auto_oid(), ctr_attr_list);

    return status;
  }
  switch_status_t counters_restore(const switch_object_id_t parent) {
    (void)parent;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    size_t list_i;

    if (get_auto_oid() == 0) return status;

    // MAU counter
    std::vector<uint64_t> ctr_list;
    status |=
        switch_store::v_get(get_auto_oid(),
                            SWITCH_EGRESS_PORT_IP_STATS_ATTR_MAU_STATS_CACHE,
                            ctr_list);
    if (ctr_list.empty()) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_PORT,
                 "{}.{}: No stat cache to restore mau stats, "
                 "egress_drop_stats cache list empty, status {}",
                 __func__,
                 __LINE__,
                 status);
      return SWITCH_STATUS_SUCCESS;
    }

    list_i = 0;
    for (auto &entry : match_action_list) {
      entry.second.set_arg(smi_id::D_EGRESS_PORT_IP_STATS_COUNTER_SPEC_PKTS,
                           ctr_list[list_i]);
      list_i++;
      entry.second.set_arg(smi_id::D_EGRESS_PORT_IP_STATS_COUNTER_SPEC_BYTES,
                           ctr_list[list_i]);
      list_i++;
    }
    status = p4_object_match_action_list::data_set();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_PORT,
                 "{}.{}: Fail to restore mau stats cache, "
                 "egress_drop_stats status {} ",
                 __func__,
                 __LINE__,
                 status);
    }

    return status;
  }
};

switch_status_t after_lag_member_update(const switch_object_id_t object_id,
                                        const attr_w &attr) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  switch (attr.id_get()) {
    case SWITCH_LAG_MEMBER_ATTR_INGRESS_DISABLE: {
      switch_object_id_t mbr_port_handle = {0};
      status |= switch_store::v_get(
          object_id, SWITCH_LAG_MEMBER_ATTR_PORT_HANDLE, mbr_port_handle);
      port_metadata port_md(mbr_port_handle, status);
      status |= port_md.create_update();
    } break;
    default:
      break;
  }
  return status;
}

switch_status_t before_rif_update3(const switch_object_id_t handle,
                                   const attr_w &attr) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_enum_t rif_type = {};

  status = switch_store::v_get(handle, SWITCH_RIF_ATTR_TYPE, rif_type);
  if (status != SWITCH_STATUS_SUCCESS) return status;

  if (attr.id_get() == SWITCH_RIF_ATTR_SRC_MAC) {
    port_vlan_to_bd_mapping_for_rif pv_bd_rif(handle, status);
    pv_bd_rif.del();
  }
  return status;
}

switch_status_t after_rif_update3(const switch_object_id_t handle,
                                  const attr_w &attr) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_enum_t rif_type = {};

  status = switch_store::v_get(handle, SWITCH_RIF_ATTR_TYPE, rif_type);
  if (status != SWITCH_STATUS_SUCCESS) return status;

  if (attr.id_get() == SWITCH_RIF_ATTR_SRC_MAC) {
    port_vlan_to_bd_mapping_for_rif pv_bd_rif(handle, status);
    pv_bd_rif.create_update();
  }
  return status;
}

switch_status_t before_vlan_update3(const switch_object_id_t vlan_handle,
                                    const attr_w &attr) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (attr.id_get() == SWITCH_VLAN_ATTR_RIF_HANDLES) {
    if (!switch_store::smiContext::context().in_warm_init()) {
      for (const auto &handle : get_untagged_vlan_member_ports(vlan_handle)) {
        std::unique_ptr<object> pv_rmac(factory::get_instance().create(
            SWITCH_OBJECT_TYPE_INGRESS_PV_RMAC_FOR_PORT, handle, status));
        pv_rmac->del();
      }
    }
  }
  return status;
}

switch_status_t after_vlan_update3(const switch_object_id_t vlan_handle,
                                   const attr_w &attr) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (attr.id_get() == SWITCH_VLAN_ATTR_RIF_HANDLES) {
    if (!switch_store::smiContext::context().in_warm_init()) {
      for (const auto &handle : get_untagged_vlan_member_ports(vlan_handle)) {
        std::unique_ptr<object> pv_rmac(factory::get_instance().create(
            SWITCH_OBJECT_TYPE_INGRESS_PV_RMAC_FOR_PORT, handle, status));
        pv_rmac->create_update();
      }
    }
  }
  return status;
}

/*
In the case when PVID was set before the VLAN object was created.
The port_vlan_to_bd_mapping_for_port P4 entry was programmed with default
action. Once appropriate VLAN obj is created the
port_vlan_to_bd_mapping_for_port entry should be updated
*/
switch_status_t after_vlan_create(const switch_object_id_t object_id,
                                  const std::set<attr_w> &attrs) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  attr_w attr_port_vlan_id(SWITCH_PORT_ATTR_PORT_VLAN_ID);
  attr_w attr_lag_vlan_id(SWITCH_LAG_ATTR_PORT_VLAN_ID);

  attr_w attr_vlan_id(SWITCH_VLAN_ATTR_VLAN_ID);
  uint16_t port_vlan_id = 0;
  uint16_t lag_vlan_id = 0;
  uint16_t vlan_id = 0;
  (void)object_id;
  (void)attrs;

  status |= switch_store::attribute_get(
      object_id, SWITCH_VLAN_ATTR_VLAN_ID, attr_vlan_id);
  attr_vlan_id.v_get(vlan_id);

  std::vector<switch_object_id_t> port_handle_list;
  status |= switch_store::object_get_all_handles(SWITCH_OBJECT_TYPE_PORT,
                                                 port_handle_list);

  for (auto port_handle : port_handle_list) {
    status |= switch_store::attribute_get(
        port_handle, SWITCH_PORT_ATTR_PORT_VLAN_ID, attr_port_vlan_id);
    attr_port_vlan_id.v_get(port_vlan_id);
    if (port_vlan_id == vlan_id) {
      // Setting the same PVID just to trigger port obj update
      status |= switch_store::attribute_set(port_handle, attr_port_vlan_id);
    }
  }
  std::vector<switch_object_id_t> lag_handle_list;
  status |= switch_store::object_get_all_handles(SWITCH_OBJECT_TYPE_LAG,
                                                 lag_handle_list);

  for (auto lag_handle : lag_handle_list) {
    status |= switch_store::attribute_get(
        lag_handle, SWITCH_LAG_ATTR_PORT_VLAN_ID, attr_lag_vlan_id);
    attr_lag_vlan_id.v_get(lag_vlan_id);
    if (lag_vlan_id == vlan_id) {
      status |= switch_store::attribute_set(lag_handle, attr_lag_vlan_id);
    }
  }
  return status;
}

/*
It is necessary when the VLAN is removed before PVID is set to the default
value. In this case, configures a new P4 entry with no default action.
*/
switch_status_t before_vlan_delete(const switch_object_id_t object_id) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  attr_w attr_port_vlan_id(SWITCH_PORT_ATTR_PORT_VLAN_ID);
  attr_w attr_vlan_id(SWITCH_VLAN_ATTR_VLAN_ID);
  attr_w attr_lag_vlan_id(SWITCH_LAG_ATTR_PORT_VLAN_ID);
  uint16_t port_vlan_id = 0;
  uint16_t lag_vlan_id = 0;
  uint16_t vlan_id = 0;
  switch_object_id_t bd_handle = {0};

  status |= switch_store::attribute_get(
      object_id, SWITCH_VLAN_ATTR_VLAN_ID, attr_vlan_id);
  attr_vlan_id.v_get(vlan_id);

  std::vector<switch_object_id_t> port_handle_list;
  status |= switch_store::object_get_all_handles(SWITCH_OBJECT_TYPE_PORT,
                                                 port_handle_list);

  status |= find_auto_oid(object_id, SWITCH_OBJECT_TYPE_BD, bd_handle);
  if (bd_handle.data != 0) {
    status |= switch_store::v_set(bd_handle, SWITCH_BD_ATTR_IS_DELETING, true);
  }
  for (auto port_handle : port_handle_list) {
    status |= switch_store::attribute_get(
        port_handle, SWITCH_PORT_ATTR_PORT_VLAN_ID, attr_port_vlan_id);
    attr_port_vlan_id.v_get(port_vlan_id);
    if (port_vlan_id == vlan_id) {
      status |= switch_store::attribute_set(port_handle, attr_port_vlan_id);
    }
  }

  std::vector<switch_object_id_t> lag_handle_list;
  status |= switch_store::object_get_all_handles(SWITCH_OBJECT_TYPE_LAG,
                                                 lag_handle_list);

  for (auto lag_handle : lag_handle_list) {
    status |= switch_store::attribute_get(
        lag_handle, SWITCH_LAG_ATTR_PORT_VLAN_ID, attr_lag_vlan_id);
    attr_lag_vlan_id.v_get(lag_vlan_id);
    if (lag_vlan_id == vlan_id) {
      status |= switch_store::attribute_set(lag_handle, attr_lag_vlan_id);
    }
  }
  return status;
}

switch_status_t port_init() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  status |= switch_store::reg_create_trigs_after(SWITCH_OBJECT_TYPE_PORT,
                                                 &after_port_create2);
  status |= switch_store::reg_delete_trigs_before(SWITCH_OBJECT_TYPE_PORT,
                                                  &before_port_delete2);
  status |= switch_store::reg_update_trigs_before(SWITCH_OBJECT_TYPE_PORT,
                                                  &before_port_update2);
  status |= switch_store::reg_update_trigs_after(SWITCH_OBJECT_TYPE_PORT,
                                                 &after_port_update2);
  status |= switch_store::reg_update_trigs_after(SWITCH_OBJECT_TYPE_LAG_MEMBER,
                                                 &after_lag_member_update);
  status |= switch_store::reg_update_trigs_before(SWITCH_OBJECT_TYPE_RIF,
                                                  &before_rif_update3);
  status |= switch_store::reg_update_trigs_after(SWITCH_OBJECT_TYPE_RIF,
                                                 &after_rif_update3);
  status |= switch_store::reg_update_trigs_before(SWITCH_OBJECT_TYPE_VLAN,
                                                  &before_vlan_update3);
  status |= switch_store::reg_update_trigs_after(SWITCH_OBJECT_TYPE_VLAN,
                                                 &after_vlan_update3);
  status |= switch_store::reg_create_trigs_after(SWITCH_OBJECT_TYPE_VLAN,
                                                 &after_vlan_create);
  status |= switch_store::reg_delete_trigs_before(SWITCH_OBJECT_TYPE_VLAN,
                                                  &before_vlan_delete);

  REGISTER_OBJECT(port_metadata, SWITCH_OBJECT_TYPE_PORT_METADATA);
  REGISTER_OBJECT(ingress_port_mapping,
                  SWITCH_OBJECT_TYPE_INGRESS_PORT_MAPPING);
  REGISTER_OBJECT(lag_helper, SWITCH_OBJECT_TYPE_LAG_HELPER);
  REGISTER_OBJECT(bd, SWITCH_OBJECT_TYPE_BD);
  REGISTER_OBJECT(bd_member, SWITCH_OBJECT_TYPE_BD_MEMBER);
  REGISTER_OBJECT(bd_action_profile, SWITCH_OBJECT_TYPE_BD_ACTION_PROFILE);
  REGISTER_OBJECT(port_vlan_to_bd_mapping_for_port,
                  SWITCH_OBJECT_TYPE_PORT_VLAN_TO_BD_MAPPING_FOR_PORT);
  REGISTER_OBJECT(port_vlan_to_bd_mapping_for_rif,
                  SWITCH_OBJECT_TYPE_PORT_VLAN_TO_BD_MAPPING_FOR_RIF);
  REGISTER_OBJECT(port_double_tag_to_bd_mapping,
                  SWITCH_OBJECT_TYPE_PORT_DOUBLE_TAG_TO_BD_MAPPING);
  REGISTER_OBJECT(vlan_to_bd_mapping, SWITCH_OBJECT_TYPE_VLAN_TO_BD_MAPPING);
  REGISTER_OBJECT(cpu_to_bd_mapping, SWITCH_OBJECT_TYPE_CPU_TO_BD_MAPPING);
  REGISTER_OBJECT(vlan_membership, SWITCH_OBJECT_TYPE_VLAN_MEMBERSHIP);
  REGISTER_OBJECT(lag_selector_group, SWITCH_OBJECT_TYPE_LAG_SELECTOR_GROUP);
  REGISTER_OBJECT(lag_membership, SWITCH_OBJECT_TYPE_LAG_MEMBERSHIP);
  REGISTER_OBJECT(lag_selector, SWITCH_OBJECT_TYPE_LAG_SELECTOR);
  REGISTER_OBJECT(lag_table_member, SWITCH_OBJECT_TYPE_LAG_TABLE_MEMBER);
  REGISTER_OBJECT(lag_table_selector, SWITCH_OBJECT_TYPE_LAG_TABLE_SELECTOR);
  REGISTER_OBJECT(egress_port_mapping, SWITCH_OBJECT_TYPE_EGRESS_PORT_MAPPING);
  REGISTER_OBJECT(peer_link_tunnel_isolation_helper,
                  SWITCH_OBJECT_TYPE_PEER_LINK_TUNNEL_ISOLATION_HELPER);
  REGISTER_OBJECT(isolation_group_helper,
                  SWITCH_OBJECT_TYPE_ISOLATION_GROUP_HELPER);
  REGISTER_OBJECT(egress_port_isolation,
                  SWITCH_OBJECT_TYPE_EGRESS_PORT_ISOLATION);
  REGISTER_OBJECT(egress_bridge_port_isolation,
                  SWITCH_OBJECT_TYPE_EGRESS_BRIDGE_PORT_ISOLATION);
  REGISTER_OBJECT(ingress_drop_stats, SWITCH_OBJECT_TYPE_INGRESS_DROP_STATS);
  REGISTER_OBJECT(egress_drop_stats, SWITCH_OBJECT_TYPE_EGRESS_DROP_STATS);
  REGISTER_OBJECT(port_stats, SWITCH_OBJECT_TYPE_PORT_STATS);
  REGISTER_OBJECT(ingress_port_mirror, SWITCH_OBJECT_TYPE_INGRESS_PORT_MIRROR);
  REGISTER_OBJECT(egress_port_mirror, SWITCH_OBJECT_TYPE_EGRESS_PORT_MIRROR);
  REGISTER_OBJECT(egress_cpu_port_rewrite,
                  SWITCH_OBJECT_TYPE_EGRESS_CPU_PORT_REWRITE);
  REGISTER_OBJECT(snake_table, SWITCH_OBJECT_TYPE_SNAKE_TABLE);
  REGISTER_OBJECT(ingress_port_ip_stats,
                  SWITCH_OBJECT_TYPE_INGRESS_PORT_IP_STATS);
  REGISTER_OBJECT(egress_port_ip_stats,
                  SWITCH_OBJECT_TYPE_EGRESS_PORT_IP_STATS);
  REGISTER_OBJECT(ingress_port_state_eg_1,
                  SWITCH_OBJECT_TYPE_INGRESS_PORT_STATE_EG_1);
  REGISTER_OBJECT(ingress_port_state_ig_1,
                  SWITCH_OBJECT_TYPE_INGRESS_PORT_STATE_IG_1);
  //  REGISTER_OBJECT(ingress_bd_state_eg_1,
  //                  SWITCH_OBJECT_TYPE_INGRESS_BD_STATE_EG_1);
  REGISTER_OBJECT(ingress_bd_state_ig_1,
                  SWITCH_OBJECT_TYPE_INGRESS_BD_STATE_IG_1);

  return status;
}

switch_status_t port_clean() {
  // this is strictly not required for hardware. In model though, the switch
  // library is not always unloaded, so this helps clean the SW state
  SWITCH_CONTEXT.clear_yid();
  return SWITCH_STATUS_SUCCESS;
}

}  // namespace smi
