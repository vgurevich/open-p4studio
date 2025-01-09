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
#include <traffic_mgr/traffic_mgr_read_apis.h>
#include <pipe_mgr/bf_packetpath_counter.h>
#include <lld/bf_lld_if.h>
}

#include <string>
#include <vector>

#include "switch_tna/p4_16_types.h"
#include "switch_tna/utils.h"

namespace smi {
using namespace smi::bf_rt;  // NOLINT(build/namespaces)
using ::smi::logging::switch_log;
using ::smi::logging::formatNumber;

inline static void process_counters_to_buffer(std::string &buffer,
                                              bf_packetpath_counter_t *cntrs,
                                              int count) {
  if (cntrs == NULL) return;
  for (int index = 0; index < count; index++) {
    if (cntrs[index].value == 0) continue;
    buffer += fmt::format("{:<50}: {}\n",
                          bf_lld_dev_is_tof1(0)
                              ? bf_packet_path_counter_description_get(
                                    cntrs[index].description_index)
                              : cntrs[index].description,
                          formatNumber(cntrs[index].value));
  }
  if (cntrs) bf_sys_free(cntrs);
  return;
}

std::string debug_pkt_path_cb(switch_object_id_t port_handle) {
  uint16_t dev_port = 0;
  switch_store::v_get(port_handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);

  // prepare header
  std::string ig_blks = "imac -> ibuf -> iprsr -> mau -> idprsr -> ";
  std::string tm_blks = bf_lld_dev_is_tof2(0) ? "s2p -> tm -> p2s" : "tm";
  std::string eg_blks = " -> epb -> eprsr -> mau -> edprsr -> ebuf -> emac";
  std::string pkt_path = ig_blks + tm_blks + eg_blks;
  std::string decorator = '\n' + std::string(pkt_path.length(), '=') + '\n';
  std::string header = decorator + pkt_path + decorator;

  // prepare counters
  std::string buffer =
      "Packet path counters for dev_port: " + formatNumber(dev_port) + '\n';

  // imac stats
  {
    uint64_t val = 0;
    bf_pal_port_this_stat_get(0, dev_port, bf_mac_stat_FramesReceivedAll, &val);
    buffer += "\nIngress MAC (imac)\n";
    buffer += "==================\n";
    buffer += fmt::format("{:<50}: {}\n",
                          "MAC: bf_mac_stat_FramesReceivedAll",
                          formatNumber(val));
  }

  // ibuf stats
  {
    buffer += "\nIngress buffer (ibuf)\n";
    buffer += "=====================\n";
    int count = 0;
    bf_packetpath_counter_t *cntrs =
        bf_packet_path_buffer_ingress_counter_get(0, dev_port, &count);
    process_counters_to_buffer(buffer, cntrs, count);
  }

  // iprsr stats
  {
    buffer += "\nIngress parser (iprsr)\n";
    buffer += "======================\n";
    int count = 0;
    bf_packetpath_counter_t *cntrs =
        bf_packet_path_parser_ingress_counter_get(0, dev_port, &count);
    process_counters_to_buffer(buffer, cntrs, count);
  }

  // ingress mau discards
  {
    buffer += "\nIngress MAU discards (imau)\n";
    buffer += "===========================\n";
    std::vector<switch_counter_t> cntrs;
    switch_store::object_counters_get(port_handle, cntrs);
    buffer += fmt::format(
        "{:<50}: {}\n",
        "IMAU: IF_IN_DISCARDS",
        formatNumber(cntrs[SWITCH_PORT_COUNTER_ID_IF_IN_DISCARDS].count));
  }

  // idprsr stats
  {
    buffer += "\nIngress deparser (idprsr)\n";
    buffer += "=========================\n";
    int count = 0;
    bf_packetpath_counter_t *cntrs =
        bf_packet_path_deparser_ingress_counter_get(
            0, DEV_PORT_TO_PIPE(dev_port), &count);
    process_counters_to_buffer(buffer, cntrs, count);
  }

  // tofino2 pmarb stats
  {
    if (bf_lld_dev_is_tof2(0)) {
      buffer += "\nIngress parser to mau (pmarb)\n";
      buffer += "=============================\n";
      int count = 0;
      bf_packetpath_counter_t *cntrs =
          bf_packet_path_parser2mau_counter_get(0, dev_port, &count);
      process_counters_to_buffer(buffer, cntrs, count);
    }
  }

  // tofino2 s2p stats
  {
    if (bf_lld_dev_is_tof2(0)) {
      buffer += "\nIngress traffic to TM (s2p)\n";
      buffer += "===========================\n";
      int count = 0;
      bf_packetpath_counter_t *cntrs =
          bf_packet_path_tm_ingress_counter_get(0, dev_port, &count);
      process_counters_to_buffer(buffer, cntrs, count);
    }
  }

  // Traffic manager
  bf_status_t st = BF_SUCCESS;
  buffer += "\nPort stats\n";
  buffer += "==========\n";
  bool shr_state = false, hdr_state = false, qac_drop_state = false;
  uint64_t wac_perport_cnt = 0, qac_perport_cnt = 0;
  uint64_t green = 0, red = 0, yellow = 0;
  if (bf_lld_dev_is_tof1(0)) {
    st = bf_tm_port_wac_drop_state_get(0, dev_port, &shr_state);
    if (st == BF_SUCCESS) {
      buffer += fmt::format("port_drop_st(wac)           : {}\n", shr_state);
    } else {
      buffer += fmt::format("port_drop_st(wac)           : Not supported\n");
    }
  } else {
    st = bf_tm_port_wac_drop_state_get_ext(0, dev_port, &shr_state, &hdr_state);
    if (st == BF_SUCCESS) {
      buffer += fmt::format("port_drop_st(wac), shared   : {}\n", shr_state);
      buffer += fmt::format("port_drop_st(wac), headroom : {}\n", hdr_state);
    } else {
      buffer += fmt::format("port_drop_st(wac)           : Not supported\n");
    }
  }
  bf_tm_port_qac_drop_state_get(0, dev_port, &qac_drop_state);
  buffer += fmt::format("port_drop_st(qac)           : {}\n", qac_drop_state);
  bf_tm_port_ingress_drop_get(
      0, DEV_PORT_TO_PIPE(dev_port), dev_port, &wac_perport_cnt);
  buffer += fmt::format("wac_perport                 : {}\n", wac_perport_cnt);
  bf_tm_port_egress_color_drop_get(0, dev_port, BF_TM_COLOR_GREEN, &green);
  bf_tm_port_egress_color_drop_get(0, dev_port, BF_TM_COLOR_RED, &red);
  bf_tm_port_egress_color_drop_get(0, dev_port, BF_TM_COLOR_YELLOW, &yellow);
  qac_perport_cnt = green + red + yellow;
  buffer += fmt::format("qac_perport (green)         : {}\n", green);
  buffer += fmt::format("qac_perport (red)           : {}\n", red);
  buffer += fmt::format("qac_perport (yellow)        : {}\n", yellow);
  buffer += fmt::format("qac_perport (total)         : {}\n", qac_perport_cnt);

  // PPGs
  std::vector<switch_object_id_t> ppg_handles;
  switch_store::v_get(
      port_handle, SWITCH_PORT_ATTR_PORT_PRIORITY_GROUPS, ppg_handles);
  buffer += "\nPPG stats\n";
  buffer += "=========\n";
  buffer +=
      fmt::format("{:<10}{:<15}{}\n", "index", "ppg_drop_st", "wac_perport");
  buffer += std::string(36, '=') + '\n';
  for (auto ppg_handle : ppg_handles) {
    uint8_t ppg_index = 0;
    switch_store::v_get(
        ppg_handle, SWITCH_PORT_PRIORITY_GROUP_ATTR_PPG_INDEX, ppg_index);
    uint32_t pd_hdl = 0;
    switch_store::v_get(
        ppg_handle, SWITCH_PORT_PRIORITY_GROUP_ATTR_PD_HDL, pd_hdl);
    bool ppg_drop_st = false;
    bf_tm_ppg_drop_state_get(0, pd_hdl, &ppg_drop_st);
    uint64_t wac_perppg_cnt = 0;
    bf_tm_ppg_drop_get(0, DEV_PORT_TO_PIPE(dev_port), pd_hdl, &wac_perppg_cnt);
    buffer +=
        fmt::format("{:<10}{:<15}{}\n", ppg_index, ppg_drop_st, wac_perppg_cnt);
  }

  // Queue stats
  std::vector<switch_object_id_t> queue_handles;
  switch_store::v_get(
      port_handle, SWITCH_PORT_ATTR_QUEUE_HANDLES, queue_handles);
  buffer += "\nQueue stats\n";
  buffer += "===========\n";
  buffer += fmt::format("{:<10}{}\n", "qid", "qac_q_drop");
  buffer += std::string(20, '=') + '\n';
  for (auto queue_handle : queue_handles) {
    uint8_t qid = 0;
    switch_store::v_get(queue_handle, SWITCH_QUEUE_ATTR_QUEUE_ID, qid);
    uint64_t q_drop_cnt = 0;
    bf_tm_q_drop_get(0, DEV_PORT_TO_PIPE(dev_port), dev_port, qid, &q_drop_cnt);
    buffer += fmt::format("{:<10}{}\n", qid, q_drop_cnt);
  }

  // tofino2 p2s stats
  {
    if (bf_lld_dev_is_tof2(0)) {
      buffer += "\nIngress traffic from TM (p2s)\n";
      buffer += "============================\n";
      int count = 0;
      bf_packetpath_counter_t *cntrs =
          bf_packet_path_tm_egress_counter_get(0, dev_port, &count);
      process_counters_to_buffer(buffer, cntrs, count);
    }
  }

  // ebuf/epb stats
  {
    buffer += "\nEgress buffer (ebuf/ebp)\n";
    buffer += "========================\n";
    int count = 0;
    bf_packetpath_counter_t *cntrs =
        bf_packet_path_buffer_egress_counter_get(0, dev_port, &count);
    process_counters_to_buffer(buffer, cntrs, count);
  }

  // eprsr stats
  {
    buffer += "\nEgress parser (eprsr)\n";
    buffer += "======================\n";
    int count = 0;
    bf_packetpath_counter_t *cntrs =
        bf_packet_path_parser_egress_counter_get(0, dev_port, &count);
    process_counters_to_buffer(buffer, cntrs, count);
  }

  // egress mau discards
  {
    buffer += "\nEgress MAU discards (emau)\n";
    buffer += "==========================\n";
    std::vector<switch_counter_t> cntrs;
    switch_store::object_counters_get(port_handle, cntrs);
    buffer += fmt::format(
        "{:<50}: {}\n",
        "EMAU: IF_OUT_DISCARDS",
        formatNumber(cntrs[SWITCH_PORT_COUNTER_ID_IF_OUT_DISCARDS].count));
  }

  // edprsr stats
  {
    buffer += "\nEgress deparser (edprsr)\n";
    buffer += "=========================\n";
    int count = 0;
    bf_packetpath_counter_t *cntrs = bf_packet_path_deparser_egress_counter_get(
        0, DEV_PORT_TO_PIPE(dev_port), &count);
    process_counters_to_buffer(buffer, cntrs, count);
  }

  // emac stats
  {
    uint64_t val = 0;
    bf_pal_port_this_stat_get(
        0, dev_port, bf_mac_stat_FramesTransmittedAll, &val);
    buffer += "\nEgress MAC (emac)\n";
    buffer += "=================\n";
    buffer += fmt::format("{:<50}: {}\n",
                          "MAC: bf_mac_stat_FramesTransmittedAll",
                          formatNumber(val));
  }

  header += buffer;
  return header;
}

switch_status_t packet_path_init() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  status |= switch_store::reg_debug_cli_callback(SWITCH_OBJECT_TYPE_PORT,
                                                 debug_pkt_path_cb);
  return status;
}

switch_status_t packet_path_clean() { return SWITCH_STATUS_SUCCESS; }

}  // namespace smi
