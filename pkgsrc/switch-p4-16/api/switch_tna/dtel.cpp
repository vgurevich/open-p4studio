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
#include <tofino/pdfixed/pd_common.h>
#include <tofino/pdfixed/pd_mirror.h>
#include <tofino/pdfixed/pd_conn_mgr.h>
#include <traffic_mgr/traffic_mgr_pipe_intf.h>
#include <lld/bf_lld_if.h>
#include <lld/lld_sku.h>
}

#include <utility>
#include <memory>
#include <vector>
#include <set>

#include "./utils.h"
#include "./p4_16_types.h"

namespace smi {
using ::smi::logging::switch_log;
using namespace smi::bf_rt;  // NOLINT(build/namespaces)

/* QUINDEX calculated below will be used in asymmetric queue report threshold
 * table which size is 2K entries.
 * So the entry index should have only 11 bits of size.
 * As we support up to 64 ports per pipe and up to 32 queues per port we need
 * only lower 6 bits from device port and 5 bits of qid to form the entry
 * index.
 */
#define QINDEX(dev_port, qid) \
  static_cast<uint16_t>((((dev_port)&0x3F) << 5) | ((qid)&0x1F))

/* Action Prof ID for dtel session selector table uses mirror session id. We
 * reserve
 * one session id for default session selector entry (no action)
 */
#define SWITCH_DTEL_SESSION_DEFAULT_ACT_PROF_ID 0x100

#define SWITCH_DTEL_CONFIG_BASE_PRIORITY 0x200
#define SWITCH_DTEL_CONFIG_CATCH_ALL_PRIORITY 0xFF0
#define SWITCH_DTEL_CONFIG_IFA_EDGE_PRIORITY 0x100
#define SWITCH_DTEL_CONFIG_IFA_CLONE_PRIORITY 0x300
#define SWITCH_DTEL_MOD_CONFIG_DEFAULT_ETRAP_PRIORITY 0x0
#define SWITCH_DTEL_MOD_CONFIG_DEFAULT_DROP_REASON_UNKNOWN_PRIORITY 0x1
#define SWITCH_DTEL_MOD_CONFIG_DEFAULT_L2_PRIORITY 0xFC0
#define SWITCH_DTEL_MOD_CONFIG_CATCH_ALL_PRIORITY 0xFFF
#define SWITCH_DTEL_MOD_CONFIG_USER_PRIO_BASE \
  SWITCH_DTEL_MOD_CONFIG_DEFAULT_DROP_REASON_UNKNOWN_PRIORITY + 10
#define SWITCH_DTEL_CONFIG_USER_PRIO_BASE SWITCH_DTEL_CONFIG_BASE_PRIORITY + 100

class dtel_config : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_DTEL_CONFIG;
  static const switch_attr_id_t status_attr_id = SWITCH_DTEL_CONFIG_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DTEL_CONFIG_ATTR_PARENT_HANDLE;
  bool drop_report = false, queue_report = false, flow_report = false,
       tail_drop_report = false, ifa_report = false;
  uint32_t report_type = SWITCH_DTEL_REPORT_TYPE_NONE;
  uint8_t pkt_src_mask = 0xFF;
  switch_object_id_t device_handle = {};
  uint32_t priority = SWITCH_DTEL_CONFIG_BASE_PRIORITY;

  typedef std::vector<std::pair<_MatchKey, _ActionEntry>> MatchAction;
  std::vector<std::pair<bf_dev_pipe_t, MatchAction>> pipe_entries;
  // Code hack for c++ = 11 does not support auto as func arguments.
  typedef __gnu_cxx::__normal_iterator<
      std::pair<bf_dev_pipe_t,
                std::vector<std::pair<smi::bf_rt::_MatchKey,
                                      smi::bf_rt::_ActionEntry>>> *,
      std::vector<std::pair<bf_dev_pipe_t,
                            std::vector<std::pair<smi::bf_rt::_MatchKey,
                                                  smi::bf_rt::_ActionEntry>>>>>
      pipe_itr;
  typedef __gnu_cxx::__normal_iterator<
      std::pair<smi::bf_rt::_MatchKey, smi::bf_rt::_ActionEntry> *,
      std::vector<std::pair<smi::bf_rt::_MatchKey, smi::bf_rt::_ActionEntry>>>
      ma_itr;

  void set_cloned_ingress(ma_itr &it,
                          pipe_itr &pipe_it,
                          bf_dev_pipe_t pipe,
                          switch_status_t &status);
  void set_cloned_egress(ma_itr &it,
                         pipe_itr &pipe_it,
                         bf_dev_pipe_t pipe,
                         switch_status_t &status);
  void set_cloned_egress_continued(ma_itr &it,
                                   pipe_itr &pipe_it,
                                   bf_dev_pipe_t pipe,
                                   switch_status_t &status);
  void set_deflected(ma_itr &it,
                     pipe_itr &pipe_it,
                     bf_dev_pipe_t pipe,
                     switch_status_t &status);
  void set_deflected_continued(ma_itr &it,
                               pipe_itr &pipe_it,
                               bf_dev_pipe_t pipe,
                               switch_status_t &status);
  void set_bridged(ma_itr &it, pipe_itr &pipe_it, switch_status_t &status);

 public:
  dtel_config(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status |=
        switch_store::v_get(parent, SWITCH_DTEL_ATTR_DROP_REPORT, drop_report);
    status |= switch_store::v_get(
        parent, SWITCH_DTEL_ATTR_QUEUE_REPORT, queue_report);
    status |=
        switch_store::v_get(parent, SWITCH_DTEL_ATTR_FLOW_REPORT, flow_report);
    status |=
        switch_store::v_get(parent, SWITCH_DTEL_ATTR_DEVICE, device_handle);
    status |= switch_store::v_get(
        parent, SWITCH_DTEL_ATTR_TAIL_DROP_REPORT, tail_drop_report);
    status |=
        switch_store::v_get(parent, SWITCH_DTEL_ATTR_IFA_REPORT, ifa_report);

    auto pipe_it = pipe_entries.begin();
    for (bf_dev_pipe_t pipe :
         _Table(smi_id::T_DTEL_CONFIG).get_active_pipes()) {
      priority = SWITCH_DTEL_CONFIG_BASE_PRIORITY;
      MatchAction match_action_list;
      pipe_it =
          pipe_entries.insert(pipe_it, std::make_pair(pipe, match_action_list));

      auto it = pipe_it->second.begin();

      set_deflected(static_cast<ma_itr &>(it),
                    static_cast<pipe_itr &>(pipe_it),
                    pipe,
                    status);
      set_cloned_ingress(static_cast<ma_itr &>(it),
                         static_cast<pipe_itr &>(pipe_it),
                         pipe,
                         status);
      set_cloned_egress(static_cast<ma_itr &>(it),
                        static_cast<pipe_itr &>(pipe_it),
                        pipe,
                        status);
      set_bridged(
          static_cast<ma_itr &>(it), static_cast<pipe_itr &>(pipe_it), status);
    }
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    bool bf_rt_status = false;
    bool add = (get_auto_oid() == 0 ||
                switch_store::smiContext::context().in_warm_init());

    for (auto const &pipe_entry : pipe_entries) {
      bf_rt_target_t dev_pipe_tgt = {.dev_id = 0, .pipe_id = pipe_entry.first};
      _Table mt(dev_pipe_tgt, get_bf_rt_info(), smi_id::T_DTEL_CONFIG);
      for (auto const &entry : pipe_entry.second) {
        if (add) {
          status = mt.entry_add(entry.first, entry.second, bf_rt_status);
          if (status != SWITCH_STATUS_SUCCESS) {
            switch_log(SWITCH_API_LEVEL_ERROR,
                       SWITCH_OBJECT_TYPE_DTEL,
                       "{}.{}:{}: status:{} failed entry_add",
                       "dtel_config",
                       __func__,
                       __LINE__,
                       status);
            return status;
          }
        } else {
          status = mt.entry_modify(entry.first, entry.second);
          if (status != SWITCH_STATUS_SUCCESS) {
            switch_log(SWITCH_API_LEVEL_ERROR,
                       SWITCH_OBJECT_TYPE_DTEL,
                       "{}.{}:{}: status:{} failed entry_modify",
                       "dtel_config",
                       __func__,
                       __LINE__,
                       status);
            return status;
          }
        }
      }
    }

    status = auto_object::create_update();
    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    for (auto const &pipe_entry : pipe_entries) {
      bf_rt_target_t dev_pipe_tgt = {.dev_id = 0, .pipe_id = pipe_entry.first};
      for (auto const &entry : pipe_entry.second) {
        _Table mt(dev_pipe_tgt, get_bf_rt_info(), smi_id::T_DTEL_CONFIG);
        status = mt.entry_delete(entry.first);
        if (status != SWITCH_STATUS_SUCCESS) {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_DTEL,
                     "{}.{}:{}: status:{} failed entry_delete",
                     "dtel_config",
                     __func__,
                     __LINE__,
                     status);
          return status;
        }
      }
    }
    status = auto_object::del();
    return status;
  }
};

void dtel_config::set_deflected(ma_itr &it,
                                pipe_itr &pipe_it,
                                bf_dev_pipe_t pipe,
                                switch_status_t &status) {
  /***********************************************************************
   * Deflected (tail drop)
   ***********************************************************************/
  // #D-1 if drop, flow are enabled, suppression is ignore, qalert is true,
  //      update
  report_type = SWITCH_DTEL_REPORT_TYPE_DROP | SWITCH_DTEL_REPORT_TYPE_FLOW;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_PKT_SRC, SWITCH_PKT_SRC_DEFLECTED, pkt_src_mask);
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_REPORT_TYPE,
                                  SWITCH_DTEL_SUPPRESS_REPORT | report_type,
                                  SWITCH_DTEL_SUPPRESS_REPORT | report_type);
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_QUEUE_REPORT_FLAG,
                                  static_cast<uint8_t>(1),
                                  static_cast<uint8_t>(1));
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  if (tail_drop_report && (drop_report || queue_report)) {
    if (drop_report && flow_report && queue_report) {
      report_type = SWITCH_DTEL_REPORT_TYPE_DROP |
                    SWITCH_DTEL_REPORT_TYPE_FLOW |
                    SWITCH_DTEL_REPORT_TYPE_QUEUE;
    } else if (drop_report && !flow_report && queue_report) {
      report_type =
          SWITCH_DTEL_REPORT_TYPE_DROP | SWITCH_DTEL_REPORT_TYPE_QUEUE;
    } else if (drop_report && !flow_report && !queue_report) {
      report_type = SWITCH_DTEL_REPORT_TYPE_DROP;
    } else if (!drop_report && flow_report && queue_report) {
      report_type =
          SWITCH_DTEL_REPORT_TYPE_FLOW | SWITCH_DTEL_REPORT_TYPE_QUEUE;
    } else if (!drop_report && !flow_report && queue_report) {
      report_type = SWITCH_DTEL_REPORT_TYPE_QUEUE;
    }
    if (bf_lld_dev_is_tof1(0)) {
      it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_SWITCH_ID,
                         device_handle,
                         SWITCH_DEVICE_ATTR_SWITCH_ID);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_HW_ID,
                         static_cast<uint8_t>(pipe));
      it->second.set_arg(
          smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_NEXT_PROTO,
          static_cast<uint8_t>(1));
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_MD_LENGTH,
                         SWITCH_DTEL_MD_LENGTH_DROP_DEFAULT);
      it->second.set_arg(
          smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_REP_MD_BITS,
          SWITCH_DTEL_REP_MD_BITS_DROP_DEFAULT);
      it->second.set_arg(
          smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_REPORT_TYPE,
          report_type);
    } else {
      it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SWITCH_ID,
                         device_handle,
                         SWITCH_DEVICE_ATTR_SWITCH_ID);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_HW_ID,
                         static_cast<uint8_t>(pipe));
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_NEXT_PROTO,
                         static_cast<uint8_t>(1));
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MD_LENGTH,
                         SWITCH_DTEL_MD_LENGTH_DROP_DEFAULT);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REP_MD_BITS,
                         SWITCH_DTEL_REP_MD_BITS_DROP_DEFAULT);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REPORT_TYPE, report_type);
    }
  } else {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_DROP);
  }
  priority++;

  // #D-2 if drop, flow are enabled, drop flag is true, qalert is true,
  //      update
  report_type = SWITCH_DTEL_REPORT_TYPE_DROP | SWITCH_DTEL_REPORT_TYPE_FLOW;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_PKT_SRC, SWITCH_PKT_SRC_DEFLECTED, pkt_src_mask);
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_REPORT_FLAG,
                                  static_cast<uint8_t>(0x3),
                                  static_cast<uint8_t>(0x3));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_QUEUE_REPORT_FLAG,
                                  static_cast<uint8_t>(1),
                                  static_cast<uint8_t>(1));
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  if (tail_drop_report && queue_report) {
    if (drop_report && flow_report) {
      report_type = SWITCH_DTEL_REPORT_TYPE_DROP |
                    SWITCH_DTEL_REPORT_TYPE_FLOW |
                    SWITCH_DTEL_REPORT_TYPE_QUEUE;
    } else if (drop_report && !flow_report) {
      report_type =
          SWITCH_DTEL_REPORT_TYPE_DROP | SWITCH_DTEL_REPORT_TYPE_QUEUE;
    } else if (!drop_report && flow_report) {
      report_type =
          SWITCH_DTEL_REPORT_TYPE_FLOW | SWITCH_DTEL_REPORT_TYPE_QUEUE;
    } else if (!drop_report && !flow_report) {
      report_type = SWITCH_DTEL_REPORT_TYPE_QUEUE;
    }
    if (bf_lld_dev_is_tof1(0)) {
      it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_SWITCH_ID,
                         device_handle,
                         SWITCH_DEVICE_ATTR_SWITCH_ID);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_HW_ID,
                         static_cast<uint8_t>(pipe));
      it->second.set_arg(
          smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_NEXT_PROTO,
          static_cast<uint8_t>(1));
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_MD_LENGTH,
                         SWITCH_DTEL_MD_LENGTH_DROP_DEFAULT);
      it->second.set_arg(
          smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_REP_MD_BITS,
          SWITCH_DTEL_REP_MD_BITS_DROP_DEFAULT);
      it->second.set_arg(
          smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_REPORT_TYPE,
          report_type);
    } else {
      it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SWITCH_ID,
                         device_handle,
                         SWITCH_DEVICE_ATTR_SWITCH_ID);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_HW_ID,
                         static_cast<uint8_t>(pipe));
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_NEXT_PROTO,
                         static_cast<uint8_t>(1));
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MD_LENGTH,
                         SWITCH_DTEL_MD_LENGTH_DROP_DEFAULT);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REP_MD_BITS,
                         SWITCH_DTEL_REP_MD_BITS_DROP_DEFAULT);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REPORT_TYPE, report_type);
    }
  } else {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_DROP);
  }
  priority++;

  // #D-3 if drop, flow are enabled, drop flag is false, qalert is true,
  //      update
  report_type = SWITCH_DTEL_REPORT_TYPE_DROP | SWITCH_DTEL_REPORT_TYPE_FLOW;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_PKT_SRC, SWITCH_PKT_SRC_DEFLECTED, pkt_src_mask);
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_QUEUE_REPORT_FLAG,
                                  static_cast<uint8_t>(1),
                                  static_cast<uint8_t>(1));
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  if (tail_drop_report && (drop_report || queue_report)) {
    if (drop_report && flow_report && queue_report) {
      report_type = SWITCH_DTEL_REPORT_TYPE_DROP |
                    SWITCH_DTEL_REPORT_TYPE_FLOW |
                    SWITCH_DTEL_REPORT_TYPE_QUEUE;
    } else if (drop_report && !flow_report && queue_report) {
      report_type =
          SWITCH_DTEL_REPORT_TYPE_DROP | SWITCH_DTEL_REPORT_TYPE_QUEUE;
    } else if (drop_report && !flow_report && !queue_report) {
      report_type = SWITCH_DTEL_REPORT_TYPE_DROP;
    } else if (!drop_report && flow_report && queue_report) {
      report_type =
          SWITCH_DTEL_REPORT_TYPE_FLOW | SWITCH_DTEL_REPORT_TYPE_QUEUE;
    } else if (!drop_report && !flow_report && queue_report) {
      report_type = SWITCH_DTEL_REPORT_TYPE_QUEUE;
    }
    if (bf_lld_dev_is_tof1(0)) {
      it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_SWITCH_ID,
                         device_handle,
                         SWITCH_DEVICE_ATTR_SWITCH_ID);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_HW_ID,
                         static_cast<uint8_t>(pipe));
      it->second.set_arg(
          smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_NEXT_PROTO,
          static_cast<uint8_t>(1));
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_MD_LENGTH,
                         SWITCH_DTEL_MD_LENGTH_DROP_DEFAULT);
      it->second.set_arg(
          smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_REP_MD_BITS,
          SWITCH_DTEL_REP_MD_BITS_DROP_DEFAULT);
      it->second.set_arg(
          smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_REPORT_TYPE,
          report_type);
    } else {
      it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SWITCH_ID,
                         device_handle,
                         SWITCH_DEVICE_ATTR_SWITCH_ID);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_HW_ID,
                         static_cast<uint8_t>(pipe));
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_NEXT_PROTO,
                         static_cast<uint8_t>(1));
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MD_LENGTH,
                         SWITCH_DTEL_MD_LENGTH_DROP_DEFAULT);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REP_MD_BITS,
                         SWITCH_DTEL_REP_MD_BITS_DROP_DEFAULT);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REPORT_TYPE, report_type);
    }
  } else {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_DROP);
  }
  priority++;

  // #D-4 if drop, flow are enabled and suppression is ignore, update
  report_type = SWITCH_DTEL_REPORT_TYPE_DROP | SWITCH_DTEL_REPORT_TYPE_FLOW;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_PKT_SRC, SWITCH_PKT_SRC_DEFLECTED, pkt_src_mask);
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_REPORT_TYPE,
                                  SWITCH_DTEL_SUPPRESS_REPORT | report_type,
                                  SWITCH_DTEL_SUPPRESS_REPORT | report_type);
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  if (tail_drop_report && drop_report) {
    if (!flow_report) {
      report_type = SWITCH_DTEL_REPORT_TYPE_DROP;
    }
    if (bf_lld_dev_is_tof1(0)) {
      it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_SWITCH_ID,
                         device_handle,
                         SWITCH_DEVICE_ATTR_SWITCH_ID);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_HW_ID,
                         static_cast<uint8_t>(pipe));
      it->second.set_arg(
          smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_NEXT_PROTO,
          static_cast<uint8_t>(1));
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_MD_LENGTH,
                         SWITCH_DTEL_MD_LENGTH_DROP_DEFAULT);
      it->second.set_arg(
          smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_REP_MD_BITS,
          SWITCH_DTEL_REP_MD_BITS_DROP_DEFAULT);
      it->second.set_arg(
          smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_REPORT_TYPE,
          report_type);
    } else {
      it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SWITCH_ID,
                         device_handle,
                         SWITCH_DEVICE_ATTR_SWITCH_ID);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_HW_ID,
                         static_cast<uint8_t>(pipe));
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_NEXT_PROTO,
                         static_cast<uint8_t>(1));
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MD_LENGTH,
                         SWITCH_DTEL_MD_LENGTH_DROP_DEFAULT);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REP_MD_BITS,
                         SWITCH_DTEL_REP_MD_BITS_DROP_DEFAULT);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REPORT_TYPE, report_type);
    }
  } else {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_DROP);
  }
  priority++;

  // #D-5 if drop, flow are enabled and drop flag is true, drop
  report_type = SWITCH_DTEL_REPORT_TYPE_DROP | SWITCH_DTEL_REPORT_TYPE_FLOW;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_PKT_SRC, SWITCH_PKT_SRC_DEFLECTED, pkt_src_mask);
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_REPORT_FLAG,
                                  static_cast<uint8_t>(0x3),
                                  static_cast<uint8_t>(0x3));
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  it->second.init_action_data(smi_id::A_DTEL_CONFIG_DROP);
  priority++;

  // #D-6 if drop, flow are enabled and drop flag is false, update
  report_type = SWITCH_DTEL_REPORT_TYPE_DROP | SWITCH_DTEL_REPORT_TYPE_FLOW;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_PKT_SRC, SWITCH_PKT_SRC_DEFLECTED, pkt_src_mask);
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  if (tail_drop_report && drop_report) {
    if (!flow_report) {
      report_type = SWITCH_DTEL_REPORT_TYPE_DROP;
    }
    if (bf_lld_dev_is_tof1(0)) {
      it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_SWITCH_ID,
                         device_handle,
                         SWITCH_DEVICE_ATTR_SWITCH_ID);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_HW_ID,
                         static_cast<uint8_t>(pipe));
      it->second.set_arg(
          smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_NEXT_PROTO,
          static_cast<uint8_t>(1));
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_MD_LENGTH,
                         SWITCH_DTEL_MD_LENGTH_DROP_DEFAULT);
      it->second.set_arg(
          smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_REP_MD_BITS,
          SWITCH_DTEL_REP_MD_BITS_DROP_DEFAULT);
      it->second.set_arg(
          smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_REPORT_TYPE,
          report_type);
    } else {
      it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SWITCH_ID,
                         device_handle,
                         SWITCH_DEVICE_ATTR_SWITCH_ID);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_HW_ID,
                         static_cast<uint8_t>(pipe));
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_NEXT_PROTO,
                         static_cast<uint8_t>(1));
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MD_LENGTH,
                         SWITCH_DTEL_MD_LENGTH_DROP_DEFAULT);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REP_MD_BITS,
                         SWITCH_DTEL_REP_MD_BITS_DROP_DEFAULT);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REPORT_TYPE, report_type);
    }
  } else {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_DROP);
  }
  priority++;

  set_deflected_continued(it, pipe_it, pipe, status);
}

void dtel_config::set_deflected_continued(ma_itr &it,
                                          pipe_itr &pipe_it,
                                          bf_dev_pipe_t pipe,
                                          switch_status_t &status) {
  // #D-7 if drop report is enabled, suppression is ignore, qalert is true,
  //      update
  report_type = SWITCH_DTEL_REPORT_TYPE_DROP;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_PKT_SRC, SWITCH_PKT_SRC_DEFLECTED, pkt_src_mask);
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_REPORT_TYPE,
                                  SWITCH_DTEL_SUPPRESS_REPORT | report_type,
                                  SWITCH_DTEL_SUPPRESS_REPORT | report_type);
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_QUEUE_REPORT_FLAG,
                                  static_cast<uint8_t>(1),
                                  static_cast<uint8_t>(1));
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  if (tail_drop_report && (drop_report || queue_report)) {
    if (drop_report && queue_report) {
      report_type =
          SWITCH_DTEL_REPORT_TYPE_DROP | SWITCH_DTEL_REPORT_TYPE_QUEUE;
    } else if (!drop_report && queue_report) {
      report_type = SWITCH_DTEL_REPORT_TYPE_QUEUE;
    }
    if (bf_lld_dev_is_tof1(0)) {
      it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_SWITCH_ID,
                         device_handle,
                         SWITCH_DEVICE_ATTR_SWITCH_ID);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_HW_ID,
                         static_cast<uint8_t>(pipe));
      it->second.set_arg(
          smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_NEXT_PROTO,
          static_cast<uint8_t>(1));
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_MD_LENGTH,
                         SWITCH_DTEL_MD_LENGTH_DROP_DEFAULT);
      it->second.set_arg(
          smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_REP_MD_BITS,
          SWITCH_DTEL_REP_MD_BITS_DROP_DEFAULT);
      it->second.set_arg(
          smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_REPORT_TYPE,
          report_type);
    } else {
      it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SWITCH_ID,
                         device_handle,
                         SWITCH_DEVICE_ATTR_SWITCH_ID);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_HW_ID,
                         static_cast<uint8_t>(pipe));
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_NEXT_PROTO,
                         static_cast<uint8_t>(1));
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MD_LENGTH,
                         SWITCH_DTEL_MD_LENGTH_DROP_DEFAULT);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REP_MD_BITS,
                         SWITCH_DTEL_REP_MD_BITS_DROP_DEFAULT);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REPORT_TYPE, report_type);
    }
  } else {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_DROP);
  }
  priority++;

  // #D-8 if drop report is enabled, drop flag is true, qalert is true,
  //      update
  report_type = SWITCH_DTEL_REPORT_TYPE_DROP;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_PKT_SRC, SWITCH_PKT_SRC_DEFLECTED, pkt_src_mask);
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_REPORT_FLAG,
                                  static_cast<uint8_t>(0x3),
                                  static_cast<uint8_t>(0x3));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_QUEUE_REPORT_FLAG,
                                  static_cast<uint8_t>(1),
                                  static_cast<uint8_t>(1));
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  if (tail_drop_report && queue_report) {
    if (drop_report) {
      report_type =
          SWITCH_DTEL_REPORT_TYPE_DROP | SWITCH_DTEL_REPORT_TYPE_QUEUE;
    } else {
      report_type = SWITCH_DTEL_REPORT_TYPE_QUEUE;
    }
    if (bf_lld_dev_is_tof1(0)) {
      it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_SWITCH_ID,
                         device_handle,
                         SWITCH_DEVICE_ATTR_SWITCH_ID);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_HW_ID,
                         static_cast<uint8_t>(pipe));
      it->second.set_arg(
          smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_NEXT_PROTO,
          static_cast<uint8_t>(1));
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_MD_LENGTH,
                         SWITCH_DTEL_MD_LENGTH_DROP_DEFAULT);
      it->second.set_arg(
          smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_REP_MD_BITS,
          SWITCH_DTEL_REP_MD_BITS_DROP_DEFAULT);
      it->second.set_arg(
          smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_REPORT_TYPE,
          report_type);
    } else {
      it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SWITCH_ID,
                         device_handle,
                         SWITCH_DEVICE_ATTR_SWITCH_ID);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_HW_ID,
                         static_cast<uint8_t>(pipe));
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_NEXT_PROTO,
                         static_cast<uint8_t>(1));
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MD_LENGTH,
                         SWITCH_DTEL_MD_LENGTH_DROP_DEFAULT);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REP_MD_BITS,
                         SWITCH_DTEL_REP_MD_BITS_DROP_DEFAULT);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REPORT_TYPE, report_type);
    }
  } else {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_DROP);
  }
  priority++;

  // #D-9 if drop report is enabled, drop flag is false, qalert is true,
  //      update
  report_type = SWITCH_DTEL_REPORT_TYPE_DROP;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_PKT_SRC, SWITCH_PKT_SRC_DEFLECTED, pkt_src_mask);
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_QUEUE_REPORT_FLAG,
                                  static_cast<uint8_t>(1),
                                  static_cast<uint8_t>(1));
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  if (tail_drop_report && (drop_report || queue_report)) {
    if (drop_report && queue_report) {
      report_type =
          SWITCH_DTEL_REPORT_TYPE_DROP | SWITCH_DTEL_REPORT_TYPE_QUEUE;
    } else if (!drop_report && queue_report) {
      report_type = SWITCH_DTEL_REPORT_TYPE_QUEUE;
    }
    if (bf_lld_dev_is_tof1(0)) {
      it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_SWITCH_ID,
                         device_handle,
                         SWITCH_DEVICE_ATTR_SWITCH_ID);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_HW_ID,
                         static_cast<uint8_t>(pipe));
      it->second.set_arg(
          smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_NEXT_PROTO,
          static_cast<uint8_t>(1));
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_MD_LENGTH,
                         SWITCH_DTEL_MD_LENGTH_DROP_DEFAULT);
      it->second.set_arg(
          smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_REP_MD_BITS,
          SWITCH_DTEL_REP_MD_BITS_DROP_DEFAULT);
      it->second.set_arg(
          smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_REPORT_TYPE,
          report_type);
    } else {
      it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SWITCH_ID,
                         device_handle,
                         SWITCH_DEVICE_ATTR_SWITCH_ID);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_HW_ID,
                         static_cast<uint8_t>(pipe));
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_NEXT_PROTO,
                         static_cast<uint8_t>(1));
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MD_LENGTH,
                         SWITCH_DTEL_MD_LENGTH_DROP_DEFAULT);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REP_MD_BITS,
                         SWITCH_DTEL_REP_MD_BITS_DROP_DEFAULT);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REPORT_TYPE, report_type);
    }
  } else {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_DROP);
  }
  priority++;

  // #D-10 if drop_report is enabled and suppression is ignore, update
  report_type = SWITCH_DTEL_REPORT_TYPE_DROP;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_PKT_SRC, SWITCH_PKT_SRC_DEFLECTED, pkt_src_mask);
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_REPORT_TYPE,
                                  SWITCH_DTEL_SUPPRESS_REPORT | report_type,
                                  SWITCH_DTEL_SUPPRESS_REPORT | report_type);
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  if (tail_drop_report && drop_report) {
    if (bf_lld_dev_is_tof1(0)) {
      it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_SWITCH_ID,
                         device_handle,
                         SWITCH_DEVICE_ATTR_SWITCH_ID);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_HW_ID,
                         static_cast<uint8_t>(pipe));
      it->second.set_arg(
          smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_NEXT_PROTO,
          static_cast<uint8_t>(1));
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_MD_LENGTH,
                         SWITCH_DTEL_MD_LENGTH_DROP_DEFAULT);
      it->second.set_arg(
          smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_REP_MD_BITS,
          SWITCH_DTEL_REP_MD_BITS_DROP_DEFAULT);
      it->second.set_arg(
          smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_REPORT_TYPE,
          report_type);
    } else {
      it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SWITCH_ID,
                         device_handle,
                         SWITCH_DEVICE_ATTR_SWITCH_ID);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_HW_ID,
                         static_cast<uint8_t>(pipe));
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_NEXT_PROTO,
                         static_cast<uint8_t>(1));
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MD_LENGTH,
                         SWITCH_DTEL_MD_LENGTH_DROP_DEFAULT);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REP_MD_BITS,
                         SWITCH_DTEL_REP_MD_BITS_DROP_DEFAULT);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REPORT_TYPE, report_type);
    }
  } else {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_DROP);
  }
  priority++;

  // #D-11 if drop report is enabled and drop flag is true, drop
  report_type = SWITCH_DTEL_REPORT_TYPE_DROP;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_PKT_SRC, SWITCH_PKT_SRC_DEFLECTED, pkt_src_mask);
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_REPORT_FLAG,
                                  static_cast<uint8_t>(0x3),
                                  static_cast<uint8_t>(0x3));
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  it->second.init_action_data(smi_id::A_DTEL_CONFIG_DROP);
  priority++;

  // #D-12 if drop report is enabled and drop flag is false, update
  report_type = SWITCH_DTEL_REPORT_TYPE_DROP;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_PKT_SRC, SWITCH_PKT_SRC_DEFLECTED, pkt_src_mask);
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  if (tail_drop_report && drop_report) {
    if (bf_lld_dev_is_tof1(0)) {
      it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_SWITCH_ID,
                         device_handle,
                         SWITCH_DEVICE_ATTR_SWITCH_ID);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_HW_ID,
                         static_cast<uint8_t>(pipe));
      it->second.set_arg(
          smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_NEXT_PROTO,
          static_cast<uint8_t>(1));
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_MD_LENGTH,
                         SWITCH_DTEL_MD_LENGTH_DROP_DEFAULT);
      it->second.set_arg(
          smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_REP_MD_BITS,
          SWITCH_DTEL_REP_MD_BITS_DROP_DEFAULT);
      it->second.set_arg(
          smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_REPORT_TYPE,
          report_type);
    } else {
      it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SWITCH_ID,
                         device_handle,
                         SWITCH_DEVICE_ATTR_SWITCH_ID);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_HW_ID,
                         static_cast<uint8_t>(pipe));
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_NEXT_PROTO,
                         static_cast<uint8_t>(1));
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MD_LENGTH,
                         SWITCH_DTEL_MD_LENGTH_DROP_DEFAULT);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REP_MD_BITS,
                         SWITCH_DTEL_REP_MD_BITS_DROP_DEFAULT);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REPORT_TYPE, report_type);
    }
  } else {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_DROP);
  }
  priority++;

  // #D-13 if queue report is enabled and qalert is false, drop
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_PKT_SRC, SWITCH_PKT_SRC_DEFLECTED, pkt_src_mask);
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_QUEUE_REPORT_FLAG,
                                  static_cast<uint8_t>(0),
                                  static_cast<uint8_t>(1));
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  it->second.init_action_data(smi_id::A_DTEL_CONFIG_DROP);
  priority++;

  // #D-14 if queue report is enabled and qalert is true, update
  report_type = SWITCH_DTEL_REPORT_TYPE_QUEUE;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_PKT_SRC, SWITCH_PKT_SRC_DEFLECTED, pkt_src_mask);
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_QUEUE_REPORT_FLAG,
                                  static_cast<uint8_t>(1),
                                  static_cast<uint8_t>(1));
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  if (tail_drop_report && queue_report) {
    if (bf_lld_dev_is_tof1(0)) {
      it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_SWITCH_ID,
                         device_handle,
                         SWITCH_DEVICE_ATTR_SWITCH_ID);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_HW_ID,
                         static_cast<uint8_t>(pipe));
      it->second.set_arg(
          smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_NEXT_PROTO,
          static_cast<uint8_t>(1));
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_MD_LENGTH,
                         SWITCH_DTEL_MD_LENGTH_DROP_DEFAULT);
      it->second.set_arg(
          smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_REP_MD_BITS,
          SWITCH_DTEL_REP_MD_BITS_DROP_DEFAULT);
      it->second.set_arg(
          smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_REPORT_TYPE,
          report_type);
    } else {
      it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SWITCH_ID,
                         device_handle,
                         SWITCH_DEVICE_ATTR_SWITCH_ID);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_HW_ID,
                         static_cast<uint8_t>(pipe));
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_NEXT_PROTO,
                         static_cast<uint8_t>(1));
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MD_LENGTH,
                         SWITCH_DTEL_MD_LENGTH_DROP_DEFAULT);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REP_MD_BITS,
                         SWITCH_DTEL_REP_MD_BITS_DROP_DEFAULT);
      it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REPORT_TYPE, report_type);
    }
  } else {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_DROP);
  }
  priority++;
}

void dtel_config::set_cloned_egress(ma_itr &it,
                                    pipe_itr &pipe_it,
                                    bf_dev_pipe_t pipe,
                                    switch_status_t &status) {
  /***********************************************************************
   * Cloned egress
   ***********************************************************************/
  // Switch local entries
  // #CE-1 if flow_report && queue_report enabled, update
  report_type = SWITCH_DTEL_REPORT_TYPE_FLOW | SWITCH_DTEL_REPORT_TYPE_QUEUE;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_PKT_SRC,
                                  SWITCH_PKT_SRC_CLONED_EGRESS,
                                  pkt_src_mask);
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
  status |=
      it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_HDR_VALID, false, true);
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  if (flow_report || ifa_report || queue_report) {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SWITCH_ID,
                       device_handle,
                       SWITCH_DEVICE_ATTR_SWITCH_ID);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_HW_ID,
                       static_cast<uint8_t>(pipe));
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_NEXT_PROTO,
                       static_cast<uint8_t>(2));
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MD_LENGTH,
                       SWITCH_DTEL_MD_LENGTH_SWITCH_LOCAL_DEFAULT);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REP_MD_BITS,
                       SWITCH_DTEL_REP_MD_BITS_SWITCH_LOCAL_DEFAULT);
    if ((flow_report || ifa_report) && !queue_report) {
      report_type = SWITCH_DTEL_REPORT_TYPE_FLOW;
    } else if (!flow_report && !ifa_report && queue_report) {
      report_type = SWITCH_DTEL_REPORT_TYPE_QUEUE;
    }
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REPORT_TYPE, report_type);
  } else {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_DROP);
  }
  priority++;

  // #CE-2 if queue_report is enabled, then update dtel header
  report_type = SWITCH_DTEL_REPORT_TYPE_QUEUE;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_PKT_SRC,
                                  SWITCH_PKT_SRC_CLONED_EGRESS,
                                  pkt_src_mask);
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
  status |=
      it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_HDR_VALID, false, true);
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  if (queue_report) {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SWITCH_ID,
                       device_handle,
                       SWITCH_DEVICE_ATTR_SWITCH_ID);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_HW_ID,
                       static_cast<uint8_t>(pipe));
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_NEXT_PROTO,
                       static_cast<uint8_t>(2));
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MD_LENGTH,
                       SWITCH_DTEL_MD_LENGTH_SWITCH_LOCAL_DEFAULT);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REP_MD_BITS,
                       SWITCH_DTEL_REP_MD_BITS_SWITCH_LOCAL_DEFAULT);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REPORT_TYPE, report_type);
  } else {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_DROP);
  }
  priority++;

  // #CE-3 if flow_report is enabled, then update dtel header
  report_type = SWITCH_DTEL_REPORT_TYPE_FLOW;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_PKT_SRC,
                                  SWITCH_PKT_SRC_CLONED_EGRESS,
                                  pkt_src_mask);
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
  status |=
      it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_HDR_VALID, false, true);
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  if (flow_report || ifa_report) {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SWITCH_ID,
                       device_handle,
                       SWITCH_DEVICE_ATTR_SWITCH_ID);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_HW_ID,
                       static_cast<uint8_t>(pipe));
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_NEXT_PROTO,
                       static_cast<uint8_t>(2));
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MD_LENGTH,
                       SWITCH_DTEL_MD_LENGTH_SWITCH_LOCAL_DEFAULT);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REP_MD_BITS,
                       SWITCH_DTEL_REP_MD_BITS_SWITCH_LOCAL_DEFAULT);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REPORT_TYPE, report_type);
  } else {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_DROP);
  }
  priority++;

  // Drop entries
  // #CE-4 if drop, flow, queue enabled and suppression is ignore, update
  report_type = SWITCH_DTEL_REPORT_TYPE_DROP | SWITCH_DTEL_REPORT_TYPE_FLOW |
                SWITCH_DTEL_REPORT_TYPE_QUEUE;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_PKT_SRC,
                                  SWITCH_PKT_SRC_CLONED_EGRESS,
                                  pkt_src_mask);
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_REPORT_TYPE,
                                  SWITCH_DTEL_SUPPRESS_REPORT | report_type,
                                  SWITCH_DTEL_SUPPRESS_REPORT | report_type);
  status |=
      it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_HDR_VALID, true, true);
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  if (drop_report || queue_report) {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SWITCH_ID,
                       device_handle,
                       SWITCH_DEVICE_ATTR_SWITCH_ID);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_HW_ID,
                       static_cast<uint8_t>(pipe));
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_NEXT_PROTO,
                       static_cast<uint8_t>(1));
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MD_LENGTH,
                       SWITCH_DTEL_MD_LENGTH_DROP_DEFAULT);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REP_MD_BITS,
                       SWITCH_DTEL_REP_MD_BITS_DROP_DEFAULT);
    if (drop_report && !flow_report && queue_report) {
      report_type =
          SWITCH_DTEL_REPORT_TYPE_DROP | SWITCH_DTEL_REPORT_TYPE_QUEUE;
    } else if (drop_report && flow_report && !queue_report) {
      report_type = SWITCH_DTEL_REPORT_TYPE_DROP | SWITCH_DTEL_REPORT_TYPE_FLOW;
    } else if (drop_report && !flow_report && !queue_report) {
      report_type = SWITCH_DTEL_REPORT_TYPE_DROP;
    } else if (!drop_report && flow_report && queue_report) {
      report_type =
          SWITCH_DTEL_REPORT_TYPE_FLOW | SWITCH_DTEL_REPORT_TYPE_QUEUE;
    } else if (!drop_report && !flow_report && queue_report) {
      report_type = SWITCH_DTEL_REPORT_TYPE_QUEUE;
    }
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REPORT_TYPE, report_type);
  } else {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_DROP);
  }
  priority++;

  // #CE-5 if drop, flow, queue are enabled and drop flag is true, update
  report_type = SWITCH_DTEL_REPORT_TYPE_DROP | SWITCH_DTEL_REPORT_TYPE_FLOW |
                SWITCH_DTEL_REPORT_TYPE_QUEUE;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_PKT_SRC,
                                  SWITCH_PKT_SRC_CLONED_EGRESS,
                                  pkt_src_mask);
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_REPORT_FLAG,
                                  static_cast<uint8_t>(0x3),
                                  static_cast<uint8_t>(0x3));
  status |=
      it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_HDR_VALID, true, true);
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  if (queue_report) {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SWITCH_ID,
                       device_handle,
                       SWITCH_DEVICE_ATTR_SWITCH_ID);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_HW_ID,
                       static_cast<uint8_t>(pipe));
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_NEXT_PROTO,
                       static_cast<uint8_t>(1));
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MD_LENGTH,
                       SWITCH_DTEL_MD_LENGTH_DROP_DEFAULT);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REP_MD_BITS,
                       SWITCH_DTEL_REP_MD_BITS_DROP_DEFAULT);
    if (drop_report && !flow_report) {
      report_type =
          SWITCH_DTEL_REPORT_TYPE_DROP | SWITCH_DTEL_REPORT_TYPE_QUEUE;
    } else if (!drop_report && flow_report) {
      report_type =
          SWITCH_DTEL_REPORT_TYPE_FLOW | SWITCH_DTEL_REPORT_TYPE_QUEUE;
    } else if (!drop_report && !flow_report) {
      report_type = SWITCH_DTEL_REPORT_TYPE_QUEUE;
    }
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REPORT_TYPE, report_type);
  } else {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_DROP);
  }
  priority++;

  // #CE-6 if drop, flow queue are enabled and drop flag is false, update
  report_type = SWITCH_DTEL_REPORT_TYPE_DROP | SWITCH_DTEL_REPORT_TYPE_FLOW |
                SWITCH_DTEL_REPORT_TYPE_QUEUE;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_PKT_SRC,
                                  SWITCH_PKT_SRC_CLONED_EGRESS,
                                  pkt_src_mask);
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
  status |=
      it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_HDR_VALID, true, true);
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  if (drop_report || queue_report) {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SWITCH_ID,
                       device_handle,
                       SWITCH_DEVICE_ATTR_SWITCH_ID);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_HW_ID,
                       static_cast<uint8_t>(pipe));
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_NEXT_PROTO,
                       static_cast<uint8_t>(1));
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MD_LENGTH,
                       SWITCH_DTEL_MD_LENGTH_DROP_DEFAULT);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REP_MD_BITS,
                       SWITCH_DTEL_REP_MD_BITS_DROP_DEFAULT);
    if (drop_report && !flow_report && queue_report) {
      report_type =
          SWITCH_DTEL_REPORT_TYPE_DROP | SWITCH_DTEL_REPORT_TYPE_QUEUE;
    } else if (drop_report && flow_report && !queue_report) {
      report_type = SWITCH_DTEL_REPORT_TYPE_DROP | SWITCH_DTEL_REPORT_TYPE_FLOW;
    } else if (drop_report && !flow_report && !queue_report) {
      report_type = SWITCH_DTEL_REPORT_TYPE_DROP;
    } else if (!drop_report && flow_report && queue_report) {
      report_type =
          SWITCH_DTEL_REPORT_TYPE_FLOW | SWITCH_DTEL_REPORT_TYPE_QUEUE;
    } else if (!drop_report && !flow_report && queue_report) {
      report_type = SWITCH_DTEL_REPORT_TYPE_QUEUE;
    }
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REPORT_TYPE, report_type);
  } else {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_DROP);
  }
  priority++;

  // #CE-7 if drop, flow are enabled and suppression is ignore, update
  report_type = SWITCH_DTEL_REPORT_TYPE_DROP | SWITCH_DTEL_REPORT_TYPE_FLOW;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_PKT_SRC,
                                  SWITCH_PKT_SRC_CLONED_EGRESS,
                                  pkt_src_mask);
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_REPORT_TYPE,
                                  SWITCH_DTEL_SUPPRESS_REPORT | report_type,
                                  SWITCH_DTEL_SUPPRESS_REPORT | report_type);
  status |=
      it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_HDR_VALID, true, true);
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  if (drop_report) {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SWITCH_ID,
                       device_handle,
                       SWITCH_DEVICE_ATTR_SWITCH_ID);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_HW_ID,
                       static_cast<uint8_t>(pipe));
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_NEXT_PROTO,
                       static_cast<uint8_t>(1));
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MD_LENGTH,
                       SWITCH_DTEL_MD_LENGTH_DROP_DEFAULT);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REP_MD_BITS,
                       SWITCH_DTEL_REP_MD_BITS_DROP_DEFAULT);
    if (!flow_report) {
      report_type = SWITCH_DTEL_REPORT_TYPE_DROP;
    }
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REPORT_TYPE, report_type);
  } else {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_DROP);
  }
  priority++;

  // #CE-8 if drop, flow are enabled and drop flag is true, drop
  report_type = SWITCH_DTEL_REPORT_TYPE_DROP | SWITCH_DTEL_REPORT_TYPE_FLOW;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_PKT_SRC,
                                  SWITCH_PKT_SRC_CLONED_EGRESS,
                                  pkt_src_mask);
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_REPORT_FLAG,
                                  static_cast<uint8_t>(0x3),
                                  static_cast<uint8_t>(0x3));
  status |=
      it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_HDR_VALID, true, true);
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  it->second.init_action_data(smi_id::A_DTEL_CONFIG_DROP);
  priority++;

  // #CE-9 if drop, flow are enabled and drop flag is false, update
  report_type = SWITCH_DTEL_REPORT_TYPE_DROP | SWITCH_DTEL_REPORT_TYPE_FLOW;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_PKT_SRC,
                                  SWITCH_PKT_SRC_CLONED_EGRESS,
                                  pkt_src_mask);
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
  status |=
      it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_HDR_VALID, true, true);
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  if (drop_report) {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SWITCH_ID,
                       device_handle,
                       SWITCH_DEVICE_ATTR_SWITCH_ID);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_HW_ID,
                       static_cast<uint8_t>(pipe));
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_NEXT_PROTO,
                       static_cast<uint8_t>(1));
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MD_LENGTH,
                       SWITCH_DTEL_MD_LENGTH_DROP_DEFAULT);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REP_MD_BITS,
                       SWITCH_DTEL_REP_MD_BITS_DROP_DEFAULT);
    if (!flow_report) {
      report_type = SWITCH_DTEL_REPORT_TYPE_DROP;
    }
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REPORT_TYPE, report_type);
  } else {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_DROP);
  }
  priority++;

  set_cloned_egress_continued(it, pipe_it, pipe, status);
}

void dtel_config::set_cloned_egress_continued(ma_itr &it,
                                              pipe_itr &pipe_it,
                                              bf_dev_pipe_t pipe,
                                              switch_status_t &status) {
  // #CE-10 if drop, queue are enabled and suppression is ignore, update
  report_type = SWITCH_DTEL_REPORT_TYPE_DROP | SWITCH_DTEL_REPORT_TYPE_QUEUE;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_PKT_SRC,
                                  SWITCH_PKT_SRC_CLONED_EGRESS,
                                  pkt_src_mask);
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_REPORT_TYPE,
                                  SWITCH_DTEL_SUPPRESS_REPORT | report_type,
                                  SWITCH_DTEL_SUPPRESS_REPORT | report_type);
  status |=
      it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_HDR_VALID, true, true);
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  if (drop_report || queue_report) {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SWITCH_ID,
                       device_handle,
                       SWITCH_DEVICE_ATTR_SWITCH_ID);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_HW_ID,
                       static_cast<uint8_t>(pipe));
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_NEXT_PROTO,
                       static_cast<uint8_t>(1));
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MD_LENGTH,
                       SWITCH_DTEL_MD_LENGTH_DROP_DEFAULT);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REP_MD_BITS,
                       SWITCH_DTEL_REP_MD_BITS_DROP_DEFAULT);
    if (drop_report && !queue_report) {
      report_type = SWITCH_DTEL_REPORT_TYPE_DROP;
    } else if (!drop_report && queue_report) {
      report_type = SWITCH_DTEL_REPORT_TYPE_QUEUE;
    }
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REPORT_TYPE, report_type);
  } else {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_DROP);
  }
  priority++;

  // #CE-11 if drop, queue are enabled and drop flag is true, update
  report_type = SWITCH_DTEL_REPORT_TYPE_DROP | SWITCH_DTEL_REPORT_TYPE_QUEUE;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_PKT_SRC,
                                  SWITCH_PKT_SRC_CLONED_EGRESS,
                                  pkt_src_mask);
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_REPORT_FLAG,
                                  static_cast<uint8_t>(0x3),
                                  static_cast<uint8_t>(0x3));
  status |=
      it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_HDR_VALID, true, true);
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  if (queue_report) {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SWITCH_ID,
                       device_handle,
                       SWITCH_DEVICE_ATTR_SWITCH_ID);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_HW_ID,
                       static_cast<uint8_t>(pipe));
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_NEXT_PROTO,
                       static_cast<uint8_t>(1));
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MD_LENGTH,
                       SWITCH_DTEL_MD_LENGTH_DROP_DEFAULT);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REP_MD_BITS,
                       SWITCH_DTEL_REP_MD_BITS_DROP_DEFAULT);
    if (!drop_report) {
      report_type = SWITCH_DTEL_REPORT_TYPE_QUEUE;
    }
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REPORT_TYPE, report_type);
  } else {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_DROP);
  }
  priority++;

  // #CE-12 if drop, queue are enabled and drop flag is false, update
  report_type = SWITCH_DTEL_REPORT_TYPE_DROP | SWITCH_DTEL_REPORT_TYPE_QUEUE;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_PKT_SRC,
                                  SWITCH_PKT_SRC_CLONED_EGRESS,
                                  pkt_src_mask);
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
  status |=
      it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_HDR_VALID, true, true);
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  if (drop_report || queue_report) {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SWITCH_ID,
                       device_handle,
                       SWITCH_DEVICE_ATTR_SWITCH_ID);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_HW_ID,
                       static_cast<uint8_t>(pipe));
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_NEXT_PROTO,
                       static_cast<uint8_t>(1));
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MD_LENGTH,
                       SWITCH_DTEL_MD_LENGTH_DROP_DEFAULT);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REP_MD_BITS,
                       SWITCH_DTEL_REP_MD_BITS_DROP_DEFAULT);
    if (drop_report && !queue_report) {
      report_type = SWITCH_DTEL_REPORT_TYPE_DROP;
    } else if (!drop_report && queue_report) {
      report_type = SWITCH_DTEL_REPORT_TYPE_QUEUE;
    }
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REPORT_TYPE, report_type);
  } else {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_DROP);
  }
  priority++;

  // #CE-13 if drop is enabled and suppression is ignore, update
  report_type = SWITCH_DTEL_REPORT_TYPE_DROP;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_PKT_SRC,
                                  SWITCH_PKT_SRC_CLONED_EGRESS,
                                  pkt_src_mask);
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_REPORT_TYPE,
                                  SWITCH_DTEL_SUPPRESS_REPORT | report_type,
                                  SWITCH_DTEL_SUPPRESS_REPORT | report_type);
  status |=
      it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_HDR_VALID, true, true);
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  if (drop_report) {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SWITCH_ID,
                       device_handle,
                       SWITCH_DEVICE_ATTR_SWITCH_ID);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_HW_ID,
                       static_cast<uint8_t>(pipe));
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_NEXT_PROTO,
                       static_cast<uint8_t>(1));
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MD_LENGTH,
                       SWITCH_DTEL_MD_LENGTH_DROP_DEFAULT);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REP_MD_BITS,
                       SWITCH_DTEL_REP_MD_BITS_DROP_DEFAULT);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REPORT_TYPE, report_type);
  } else {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_DROP);
  }
  priority++;

  // #CE-14 if drop is enabled and drop flag is true, drop
  report_type = SWITCH_DTEL_REPORT_TYPE_DROP;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_PKT_SRC,
                                  SWITCH_PKT_SRC_CLONED_EGRESS,
                                  pkt_src_mask);
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_REPORT_FLAG,
                                  static_cast<uint8_t>(0x3),
                                  static_cast<uint8_t>(0x3));
  status |=
      it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_HDR_VALID, true, true);
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  it->second.init_action_data(smi_id::A_DTEL_CONFIG_DROP);
  priority++;

  // #CE-15 if drop is enabled and drop flag is false, update
  report_type = SWITCH_DTEL_REPORT_TYPE_DROP;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_PKT_SRC,
                                  SWITCH_PKT_SRC_CLONED_EGRESS,
                                  pkt_src_mask);
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
  status |=
      it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_HDR_VALID, true, true);
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  if (drop_report) {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SWITCH_ID,
                       device_handle,
                       SWITCH_DEVICE_ATTR_SWITCH_ID);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_HW_ID,
                       static_cast<uint8_t>(pipe));
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_NEXT_PROTO,
                       static_cast<uint8_t>(1));
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MD_LENGTH,
                       SWITCH_DTEL_MD_LENGTH_DROP_DEFAULT);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REP_MD_BITS,
                       SWITCH_DTEL_REP_MD_BITS_DROP_DEFAULT);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REPORT_TYPE, report_type);
  } else {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_DROP);
  }
  priority++;
}

void dtel_config::set_bridged(ma_itr &it,
                              pipe_itr &pipe_it,
                              switch_status_t &status) {
  /***********************************************************************
   * Bridged
   ***********************************************************************/
  // #B-1 if flow_report enabled, suppression is ignore, qalert true, mirror
  report_type = SWITCH_DTEL_REPORT_TYPE_FLOW;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_PKT_SRC, SWITCH_PKT_SRC_BRIDGED, pkt_src_mask);
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_REPORT_TYPE,
                                  SWITCH_DTEL_SUPPRESS_REPORT | report_type,
                                  SWITCH_DTEL_SUPPRESS_REPORT | report_type);
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_QUEUE_REPORT_FLAG,
                                  static_cast<uint8_t>(1),
                                  static_cast<uint8_t>(1));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_REASON,
                                  SWITCH_DROP_REASON_UNKNOWN,
                                  static_cast<uint32_t>(0xFF));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_MIRROR_TYPE,
                                  SWITCH_MIRROR_TYPE_INVALID,
                                  static_cast<uint8_t>(0xFF));
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  if (queue_report) {
    it->second.init_action_data(
        smi_id::A_DTEL_CONFIG_MIRROR_SWITCH_LOCAL_SET_Q);
  } else if (flow_report) {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_MIRROR_SWITCH_LOCAL);
  } else {
    it->second.init_action_data(smi_id::A_NO_ACTION);
  }
  priority++;

  // #B-2 if flow_report enabled, flow flag is 0, qalert true, mirror
  report_type = SWITCH_DTEL_REPORT_TYPE_FLOW;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_PKT_SRC, SWITCH_PKT_SRC_BRIDGED, pkt_src_mask);
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_FLOW_REPORT_FLAG,
                                  static_cast<uint8_t>(0),
                                  static_cast<uint8_t>(3));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_QUEUE_REPORT_FLAG,
                                  static_cast<uint8_t>(1),
                                  static_cast<uint8_t>(1));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_REASON,
                                  SWITCH_DROP_REASON_UNKNOWN,
                                  static_cast<uint32_t>(0xFF));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_MIRROR_TYPE,
                                  SWITCH_MIRROR_TYPE_INVALID,
                                  static_cast<uint8_t>(0xFF));
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  if (queue_report) {
    it->second.init_action_data(
        smi_id::A_DTEL_CONFIG_MIRROR_SWITCH_LOCAL_SET_Q);
  } else if (flow_report) {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_MIRROR_SWITCH_LOCAL);
  } else {
    it->second.init_action_data(smi_id::A_NO_ACTION);
  }
  priority++;

  // #B-3 if flow_report enabled, flow flag is 1*, qalert true, mirror
  report_type = SWITCH_DTEL_REPORT_TYPE_FLOW;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_PKT_SRC, SWITCH_PKT_SRC_BRIDGED, pkt_src_mask);
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_FLOW_REPORT_FLAG,
                                  static_cast<uint8_t>(2),
                                  static_cast<uint8_t>(2));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_QUEUE_REPORT_FLAG,
                                  static_cast<uint8_t>(1),
                                  static_cast<uint8_t>(1));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_REASON,
                                  SWITCH_DROP_REASON_UNKNOWN,
                                  static_cast<uint32_t>(0xFF));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_MIRROR_TYPE,
                                  SWITCH_MIRROR_TYPE_INVALID,
                                  static_cast<uint8_t>(0xFF));
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  if (queue_report) {
    it->second.init_action_data(
        smi_id::A_DTEL_CONFIG_MIRROR_SWITCH_LOCAL_SET_Q);
  } else if (flow_report) {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_MIRROR_SWITCH_LOCAL);
  } else {
    it->second.init_action_data(smi_id::A_NO_ACTION);
  }
  priority++;

  // #B-4 if qalert is true, mirror
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_PKT_SRC, SWITCH_PKT_SRC_BRIDGED, pkt_src_mask);
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_QUEUE_REPORT_FLAG,
                                  static_cast<uint8_t>(1),
                                  static_cast<uint8_t>(1));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_REASON,
                                  SWITCH_DROP_REASON_UNKNOWN,
                                  static_cast<uint32_t>(0xFF));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_MIRROR_TYPE,
                                  SWITCH_MIRROR_TYPE_INVALID,
                                  static_cast<uint8_t>(0xFF));
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  if (queue_report) {
    it->second.init_action_data(
        smi_id::A_DTEL_CONFIG_MIRROR_SWITCH_LOCAL_SET_Q);
  } else {
    it->second.init_action_data(smi_id::A_NO_ACTION);
  }
  priority++;

  // #B-5 if flow_report is enabled and suppression is ignore, mirror
  report_type = SWITCH_DTEL_REPORT_TYPE_FLOW;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_PKT_SRC, SWITCH_PKT_SRC_BRIDGED, pkt_src_mask);
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_REPORT_TYPE,
                                  SWITCH_DTEL_SUPPRESS_REPORT | report_type,
                                  SWITCH_DTEL_SUPPRESS_REPORT | report_type);
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_REASON,
                                  SWITCH_DROP_REASON_UNKNOWN,
                                  static_cast<uint32_t>(0xFF));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_MIRROR_TYPE,
                                  SWITCH_MIRROR_TYPE_INVALID,
                                  static_cast<uint8_t>(0xFF));
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  if (flow_report) {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_MIRROR_SWITCH_LOCAL);
  } else {
    it->second.init_action_data(smi_id::A_NO_ACTION);
  }
  priority++;

  // #B-6 if flow_report is enabled and flow flag is 0, mirror
  report_type = SWITCH_DTEL_REPORT_TYPE_FLOW;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_PKT_SRC, SWITCH_PKT_SRC_BRIDGED, pkt_src_mask);
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_FLOW_REPORT_FLAG,
                                  static_cast<uint8_t>(0),
                                  static_cast<uint8_t>(3));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_REASON,
                                  SWITCH_DROP_REASON_UNKNOWN,
                                  static_cast<uint32_t>(0xFF));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_MIRROR_TYPE,
                                  SWITCH_MIRROR_TYPE_INVALID,
                                  static_cast<uint8_t>(0xFF));
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  if (flow_report) {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_MIRROR_SWITCH_LOCAL);
  } else {
    it->second.init_action_data(smi_id::A_NO_ACTION);
  }
  priority++;

  // #B-7 if flow_report is enabled and flow flag is 1*, mirror
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_PKT_SRC, SWITCH_PKT_SRC_BRIDGED, pkt_src_mask);
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_FLOW_REPORT_FLAG,
                                  static_cast<uint8_t>(2),
                                  static_cast<uint8_t>(2));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_REASON,
                                  SWITCH_DROP_REASON_UNKNOWN,
                                  static_cast<uint32_t>(0xFF));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_MIRROR_TYPE,
                                  SWITCH_MIRROR_TYPE_INVALID,
                                  static_cast<uint8_t>(0xFF));
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  if (flow_report) {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_MIRROR_SWITCH_LOCAL);
  } else {
    it->second.init_action_data(smi_id::A_NO_ACTION);
  }
  priority++;

  if (feature::is_feature_set(SWITCH_FEATURE_FLOW_REPORT)) {
    // #B-8 if flow_report is enabled and TCP FIN, mirror
    it = pipe_it->second.insert(it,
                                std::pair<_MatchKey, _ActionEntry>(
                                    _MatchKey(smi_id::T_DTEL_CONFIG),
                                    _ActionEntry(smi_id::T_DTEL_CONFIG)));
    status |= it->first.set_ternary(
        smi_id::F_DTEL_CONFIG_PKT_SRC, SWITCH_PKT_SRC_BRIDGED, pkt_src_mask);
    status |= it->first.set_ternary(
        smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
    status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_TCP_FLAGS,
                                    static_cast<uint32_t>(1),
                                    static_cast<uint32_t>(1));
    status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_REASON,
                                    SWITCH_DROP_REASON_UNKNOWN,
                                    static_cast<uint32_t>(0xFF));
    status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_MIRROR_TYPE,
                                    SWITCH_MIRROR_TYPE_INVALID,
                                    static_cast<uint8_t>(0xFF));
    status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
    if (flow_report) {
      it->second.init_action_data(smi_id::A_DTEL_CONFIG_MIRROR_SWITCH_LOCAL);
    } else {
      it->second.init_action_data(smi_id::A_NO_ACTION);
    }
    priority++;

    // #B-9 if flow_report is enabled and TCP SYN, mirror
    it = pipe_it->second.insert(it,
                                std::pair<_MatchKey, _ActionEntry>(
                                    _MatchKey(smi_id::T_DTEL_CONFIG),
                                    _ActionEntry(smi_id::T_DTEL_CONFIG)));
    status |= it->first.set_ternary(
        smi_id::F_DTEL_CONFIG_PKT_SRC, SWITCH_PKT_SRC_BRIDGED, pkt_src_mask);
    status |= it->first.set_ternary(
        smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
    status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_TCP_FLAGS,
                                    static_cast<uint32_t>(2),
                                    static_cast<uint32_t>(2));
    status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_REASON,
                                    SWITCH_DROP_REASON_UNKNOWN,
                                    static_cast<uint32_t>(0xFF));
    status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_MIRROR_TYPE,
                                    SWITCH_MIRROR_TYPE_INVALID,
                                    static_cast<uint8_t>(0xFF));
    status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
    if (flow_report) {
      it->second.init_action_data(smi_id::A_DTEL_CONFIG_MIRROR_SWITCH_LOCAL);
    } else {
      it->second.init_action_data(smi_id::A_NO_ACTION);
    }
    priority++;

    // #B-10 if flow_report is enabled and TCP RST, mirror
    it = pipe_it->second.insert(it,
                                std::pair<_MatchKey, _ActionEntry>(
                                    _MatchKey(smi_id::T_DTEL_CONFIG),
                                    _ActionEntry(smi_id::T_DTEL_CONFIG)));
    status |= it->first.set_ternary(
        smi_id::F_DTEL_CONFIG_PKT_SRC, SWITCH_PKT_SRC_BRIDGED, pkt_src_mask);
    status |= it->first.set_ternary(
        smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
    status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_TCP_FLAGS,
                                    static_cast<uint32_t>(4),
                                    static_cast<uint32_t>(4));
    status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_REASON,
                                    SWITCH_DROP_REASON_UNKNOWN,
                                    static_cast<uint32_t>(0xFF));
    status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_MIRROR_TYPE,
                                    SWITCH_MIRROR_TYPE_INVALID,
                                    static_cast<uint8_t>(0xFF));
    status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
    if (flow_report) {
      it->second.init_action_data(smi_id::A_DTEL_CONFIG_MIRROR_SWITCH_LOCAL);
    } else {
      it->second.init_action_data(smi_id::A_NO_ACTION);
    }
    priority++;
  }

  // Egress pipeline drop report catch all entries
  priority = SWITCH_DTEL_CONFIG_CATCH_ALL_PRIORITY;

  // #B-11 if drop_reason == 0, NoAction
  report_type = SWITCH_DTEL_REPORT_TYPE_DROP;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_PKT_SRC, SWITCH_PKT_SRC_BRIDGED, pkt_src_mask);
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_REASON,
                                  SWITCH_DROP_REASON_UNKNOWN,
                                  static_cast<uint32_t>(0xFF));
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  it->second.init_action_data(smi_id::A_NO_ACTION);
  priority++;

  // #B-12 if drop_report enabled and qalert is true, mirror
  report_type = SWITCH_DTEL_REPORT_TYPE_DROP;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_PKT_SRC, SWITCH_PKT_SRC_BRIDGED, pkt_src_mask);
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_QUEUE_REPORT_FLAG,
                                  static_cast<uint8_t>(1),
                                  static_cast<uint8_t>(1));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_MIRROR_TYPE,
                                  SWITCH_MIRROR_TYPE_INVALID,
                                  static_cast<uint8_t>(0xFF));
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  if (drop_report && queue_report) {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_MIRROR_DROP_SET_Q);
  } else if (drop_report) {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_MIRROR_DROP);
  } else if (queue_report) {
    it->second.init_action_data(
        smi_id::A_DTEL_CONFIG_MIRROR_SWITCH_LOCAL_SET_Q);
  } else {
    it->second.init_action_data(smi_id::A_NO_ACTION);
  }
  priority++;

  // #B-13 if drop_report is enabled, mirror
  report_type = SWITCH_DTEL_REPORT_TYPE_DROP;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_PKT_SRC, SWITCH_PKT_SRC_BRIDGED, pkt_src_mask);
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_MIRROR_TYPE,
                                  SWITCH_MIRROR_TYPE_INVALID,
                                  static_cast<uint8_t>(0xFF));
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  if (drop_report) {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_MIRROR_DROP);
  } else {
    it->second.init_action_data(smi_id::A_NO_ACTION);
  }
  priority++;

  // #B-14 if qalert is true, mirror
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_PKT_SRC, SWITCH_PKT_SRC_BRIDGED, pkt_src_mask);
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_QUEUE_REPORT_FLAG,
                                  static_cast<uint8_t>(1),
                                  static_cast<uint8_t>(1));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_MIRROR_TYPE,
                                  SWITCH_MIRROR_TYPE_INVALID,
                                  static_cast<uint8_t>(0xFF));
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  if (queue_report) {
    it->second.init_action_data(
        smi_id::A_DTEL_CONFIG_MIRROR_SWITCH_LOCAL_SET_Q);
  } else {
    it->second.init_action_data(smi_id::A_NO_ACTION);
  }
  priority++;
}

void dtel_config::set_cloned_ingress(ma_itr &it,
                                     pipe_itr &pipe_it,
                                     bf_dev_pipe_t pipe,
                                     switch_status_t &status) {
  /***********************************************************************
   * Cloned ingress
   ***********************************************************************/
  if (feature::is_feature_set(SWITCH_FEATURE_ETRAP)) {
    // #CI-1 if etrap_change and etrap_hit, update_and_set_etrap
    report_type = SWITCH_DTEL_REPORT_TYPE_ETRAP_CHANGE |
                  SWITCH_DTEL_REPORT_TYPE_ETRAP_HIT;
    it = pipe_it->second.insert(it,
                                std::pair<_MatchKey, _ActionEntry>(
                                    _MatchKey(smi_id::T_DTEL_CONFIG),
                                    _ActionEntry(smi_id::T_DTEL_CONFIG)));
    status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_PKT_SRC,
                                    SWITCH_PKT_SRC_CLONED_INGRESS,
                                    pkt_src_mask);
    status |= it->first.set_ternary(
        smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
    status |=
        it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_HDR_VALID, true, true);
    status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);

    it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE_SET_ETRAP);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SET_ETRAP_SWITCH_ID,
                       device_handle,
                       SWITCH_DEVICE_ATTR_SWITCH_ID);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SET_ETRAP_HW_ID,
                       static_cast<uint8_t>(pipe));
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SET_ETRAP_NEXT_PROTO,
                       static_cast<uint8_t>(1));
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SET_ETRAP_MD_LENGTH,
                       SWITCH_DTEL_MD_LENGTH_DROP_DEFAULT);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SET_ETRAP_REP_MD_BITS,
                       SWITCH_DTEL_REP_MD_BITS_DROP_DEFAULT);
    report_type = SWITCH_DTEL_REPORT_TYPE_NONE;
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SET_ETRAP_REPORT_TYPE,
                       report_type);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SET_ETRAP_STATUS,
                       static_cast<uint8_t>(0x3));
    priority++;

    // #CI-2 if etrap_change, update_and_set_etrap
    report_type = SWITCH_DTEL_REPORT_TYPE_ETRAP_CHANGE;
    it = pipe_it->second.insert(it,
                                std::pair<_MatchKey, _ActionEntry>(
                                    _MatchKey(smi_id::T_DTEL_CONFIG),
                                    _ActionEntry(smi_id::T_DTEL_CONFIG)));
    status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_PKT_SRC,
                                    SWITCH_PKT_SRC_CLONED_INGRESS,
                                    pkt_src_mask);
    status |= it->first.set_ternary(
        smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
    status |=
        it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_HDR_VALID, true, true);
    status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);

    it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE_SET_ETRAP);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SET_ETRAP_SWITCH_ID,
                       device_handle,
                       SWITCH_DEVICE_ATTR_SWITCH_ID);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SET_ETRAP_HW_ID,
                       static_cast<uint8_t>(pipe));
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SET_ETRAP_NEXT_PROTO,
                       static_cast<uint8_t>(1));
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SET_ETRAP_MD_LENGTH,
                       SWITCH_DTEL_MD_LENGTH_DROP_DEFAULT);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SET_ETRAP_REP_MD_BITS,
                       SWITCH_DTEL_REP_MD_BITS_DROP_DEFAULT);
    report_type = SWITCH_DTEL_REPORT_TYPE_NONE;
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SET_ETRAP_REPORT_TYPE,
                       report_type);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SET_ETRAP_STATUS,
                       static_cast<uint8_t>(0x1));
    priority++;
  }

  // #CI-3 if drop, flow are enabled and suppression is ignore, update
  report_type = SWITCH_DTEL_REPORT_TYPE_DROP | SWITCH_DTEL_REPORT_TYPE_FLOW;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_PKT_SRC,
                                  SWITCH_PKT_SRC_CLONED_INGRESS,
                                  pkt_src_mask);
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_REPORT_TYPE,
                                  SWITCH_DTEL_SUPPRESS_REPORT | report_type,
                                  SWITCH_DTEL_SUPPRESS_REPORT | report_type);
  status |=
      it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_HDR_VALID, true, true);
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  if (drop_report) {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SWITCH_ID,
                       device_handle,
                       SWITCH_DEVICE_ATTR_SWITCH_ID);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_HW_ID,
                       static_cast<uint8_t>(pipe));
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_NEXT_PROTO,
                       static_cast<uint8_t>(1));
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MD_LENGTH,
                       SWITCH_DTEL_MD_LENGTH_DROP_DEFAULT);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REP_MD_BITS,
                       SWITCH_DTEL_REP_MD_BITS_DROP_DEFAULT);
    if (!flow_report) {
      report_type = SWITCH_DTEL_REPORT_TYPE_DROP;
    }
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REPORT_TYPE, report_type);
  } else {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_DROP);
  }
  priority++;

  // #CI-4 if drop, flow are enabled and drop flag is true, drop
  report_type = SWITCH_DTEL_REPORT_TYPE_DROP | SWITCH_DTEL_REPORT_TYPE_FLOW;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_PKT_SRC,
                                  SWITCH_PKT_SRC_CLONED_INGRESS,
                                  pkt_src_mask);
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_REPORT_FLAG,
                                  static_cast<uint8_t>(0x3),
                                  static_cast<uint8_t>(0x3));
  status |=
      it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_HDR_VALID, true, true);
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  it->second.init_action_data(smi_id::A_DTEL_CONFIG_DROP);
  priority++;

  // #CI-5 if drop, flow are enabled and drop flag is false, update
  report_type = SWITCH_DTEL_REPORT_TYPE_DROP | SWITCH_DTEL_REPORT_TYPE_FLOW;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_PKT_SRC,
                                  SWITCH_PKT_SRC_CLONED_INGRESS,
                                  pkt_src_mask);
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
  status |=
      it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_HDR_VALID, true, true);
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  if (drop_report) {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SWITCH_ID,
                       device_handle,
                       SWITCH_DEVICE_ATTR_SWITCH_ID);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_HW_ID,
                       static_cast<uint8_t>(pipe));
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_NEXT_PROTO,
                       static_cast<uint8_t>(1));
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MD_LENGTH,
                       SWITCH_DTEL_MD_LENGTH_DROP_DEFAULT);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REP_MD_BITS,
                       SWITCH_DTEL_REP_MD_BITS_DROP_DEFAULT);
    if (!flow_report) {
      report_type = SWITCH_DTEL_REPORT_TYPE_DROP;
    }
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REPORT_TYPE, report_type);
  } else {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_DROP);
  }
  priority++;

  // #CI-6 if drop_report is enabled and suppression is ignore, update
  report_type = SWITCH_DTEL_REPORT_TYPE_DROP;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_PKT_SRC,
                                  SWITCH_PKT_SRC_CLONED_INGRESS,
                                  pkt_src_mask);
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_REPORT_TYPE,
                                  SWITCH_DTEL_SUPPRESS_REPORT | report_type,
                                  SWITCH_DTEL_SUPPRESS_REPORT | report_type);
  status |=
      it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_HDR_VALID, true, true);
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  if (drop_report) {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SWITCH_ID,
                       device_handle,
                       SWITCH_DEVICE_ATTR_SWITCH_ID);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_HW_ID,
                       static_cast<uint8_t>(pipe));
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_NEXT_PROTO,
                       static_cast<uint8_t>(1));
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MD_LENGTH,
                       SWITCH_DTEL_MD_LENGTH_DROP_DEFAULT);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REP_MD_BITS,
                       SWITCH_DTEL_REP_MD_BITS_DROP_DEFAULT);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REPORT_TYPE, report_type);
  } else {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_DROP);
  }
  priority++;

  // #CI-7 if drop_report is enabled and drop flag is true, drop
  report_type = SWITCH_DTEL_REPORT_TYPE_DROP;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_PKT_SRC,
                                  SWITCH_PKT_SRC_CLONED_INGRESS,
                                  pkt_src_mask);
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_REPORT_FLAG,
                                  static_cast<uint8_t>(0x3),
                                  static_cast<uint8_t>(0x3));
  status |=
      it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_HDR_VALID, true, true);
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  it->second.init_action_data(smi_id::A_DTEL_CONFIG_DROP);
  priority++;

  // #CI-8 if drop_report is enabled and drop flag is false, update
  report_type = SWITCH_DTEL_REPORT_TYPE_DROP;
  it = pipe_it->second.insert(
      it,
      std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DTEL_CONFIG),
                                         _ActionEntry(smi_id::T_DTEL_CONFIG)));
  status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_PKT_SRC,
                                  SWITCH_PKT_SRC_CLONED_INGRESS,
                                  pkt_src_mask);
  status |= it->first.set_ternary(
      smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
  status |=
      it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_HDR_VALID, true, true);
  status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
  if (drop_report) {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_UPDATE);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_SWITCH_ID,
                       device_handle,
                       SWITCH_DEVICE_ATTR_SWITCH_ID);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_HW_ID,
                       static_cast<uint8_t>(pipe));
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_NEXT_PROTO,
                       static_cast<uint8_t>(1));
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_MD_LENGTH,
                       SWITCH_DTEL_MD_LENGTH_DROP_DEFAULT);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REP_MD_BITS,
                       SWITCH_DTEL_REP_MD_BITS_DROP_DEFAULT);
    it->second.set_arg(smi_id::D_DTEL_CONFIG_UPDATE_REPORT_TYPE, report_type);
  } else {
    it->second.init_action_data(smi_id::A_DTEL_CONFIG_DROP);
  }
  priority++;
}

class dtel_config_ifa : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DTEL_CONFIG_IFA;
  static const switch_attr_id_t status_attr_id =
      SWITCH_DTEL_CONFIG_IFA_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DTEL_CONFIG_IFA_ATTR_PARENT_HANDLE;
  typedef std::vector<std::pair<_MatchKey, _ActionEntry>> MatchAction;
  std::vector<std::pair<bf_dev_pipe_t, MatchAction>> pipe_entries;

 public:
  dtel_config_ifa(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    bool drop_report = false, queue_report = false, flow_report = false,
         tail_drop_report = false, ifa_report = false;
    uint32_t report_type = SWITCH_DTEL_REPORT_TYPE_NONE;
    uint8_t pkt_src_mask = 0xFF;
    switch_object_id_t device_handle = {};
    uint32_t priority = SWITCH_DTEL_CONFIG_IFA_EDGE_PRIORITY;
    uint8_t ifa_dscp = 7;
    uint8_t ifa_dscp_mask = 63;

    status |=
        switch_store::v_get(parent, SWITCH_DTEL_ATTR_DROP_REPORT, drop_report);
    status |= switch_store::v_get(
        parent, SWITCH_DTEL_ATTR_QUEUE_REPORT, queue_report);
    status |=
        switch_store::v_get(parent, SWITCH_DTEL_ATTR_FLOW_REPORT, flow_report);
    status |=
        switch_store::v_get(parent, SWITCH_DTEL_ATTR_DEVICE, device_handle);
    status |= switch_store::v_get(
        parent, SWITCH_DTEL_ATTR_TAIL_DROP_REPORT, tail_drop_report);
    status |=
        switch_store::v_get(parent, SWITCH_DTEL_ATTR_IFA_REPORT, ifa_report);
    status |= switch_store::v_get(parent, SWITCH_DTEL_ATTR_IFA_DSCP, ifa_dscp);
    status |= switch_store::v_get(
        parent, SWITCH_DTEL_ATTR_IFA_DSCP_MASK, ifa_dscp_mask);

    auto pipe_it = pipe_entries.begin();
    for (bf_dev_pipe_t pipe :
         _Table(smi_id::T_DTEL_CONFIG).get_active_pipes()) {
      priority = SWITCH_DTEL_CONFIG_IFA_EDGE_PRIORITY;
      MatchAction match_action_list;
      pipe_it =
          pipe_entries.insert(pipe_it, std::make_pair(pipe, match_action_list));
      auto it = pipe_it->second.begin();

      /***********************************************************************
       * Bridged IFA edge entries
       ***********************************************************************/
      if ((feature::is_feature_set(SWITCH_FEATURE_DTEL_IFA_EDGE)) &&
          ifa_report) {
        // #BI-1 if ifa_dscp, ipv4 and qalert true, mirror
        report_type = SWITCH_DTEL_IFA_EDGE;
        it = pipe_it->second.insert(it,
                                    std::pair<_MatchKey, _ActionEntry>(
                                        _MatchKey(smi_id::T_DTEL_CONFIG),
                                        _ActionEntry(smi_id::T_DTEL_CONFIG)));
        status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_PKT_SRC,
                                        SWITCH_PKT_SRC_BRIDGED,
                                        pkt_src_mask);
        status |= it->first.set_ternary(
            smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
        status |= it->first.set_ternary(
            smi_id::F_DTEL_CONFIG_IPV4_HDR_VALID, true, true);
        status |= it->first.set_ternary(
            smi_id::F_DTEL_CONFIG_IPV4_DIFFSERV, ifa_dscp, ifa_dscp_mask);
        status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_QUEUE_REPORT_FLAG,
                                        static_cast<uint8_t>(1),
                                        static_cast<uint8_t>(1));
        status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_REASON,
                                        SWITCH_DROP_REASON_UNKNOWN,
                                        static_cast<uint32_t>(0xFF));
        status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
        if (queue_report) {
          it->second.init_action_data(
              smi_id::A_DTEL_CONFIG_MIRROR_SWITCH_LOCAL_SET_Q_F_AND_DROP);
        } else {
          it->second.init_action_data(
              smi_id::A_DTEL_CONFIG_MIRROR_SWITCH_LOCAL_SET_F_AND_DROP);
        }
        priority++;

        // #BI-2 if ifa_dscp, ipv6 and qalert true, mirror
        report_type = SWITCH_DTEL_IFA_EDGE;
        it = pipe_it->second.insert(it,
                                    std::pair<_MatchKey, _ActionEntry>(
                                        _MatchKey(smi_id::T_DTEL_CONFIG),
                                        _ActionEntry(smi_id::T_DTEL_CONFIG)));
        status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_PKT_SRC,
                                        SWITCH_PKT_SRC_BRIDGED,
                                        pkt_src_mask);
        status |= it->first.set_ternary(
            smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
        status |= it->first.set_ternary(
            smi_id::F_DTEL_CONFIG_IPV6_HDR_VALID, true, true);
        status |= it->first.set_ternary(
            smi_id::F_DTEL_CONFIG_IPV6_TRAFFIC_CLASS, ifa_dscp, ifa_dscp_mask);
        status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_QUEUE_REPORT_FLAG,
                                        static_cast<uint8_t>(1),
                                        static_cast<uint8_t>(1));
        status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_REASON,
                                        SWITCH_DROP_REASON_UNKNOWN,
                                        static_cast<uint32_t>(0xFF));
        status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
        if (queue_report) {
          it->second.init_action_data(
              smi_id::A_DTEL_CONFIG_MIRROR_SWITCH_LOCAL_SET_Q_F_AND_DROP);
        } else {
          it->second.init_action_data(
              smi_id::A_DTEL_CONFIG_MIRROR_SWITCH_LOCAL_SET_F_AND_DROP);
        }
        priority++;

        // #BI-3 if ifa_dscp and ipv4, mirror
        report_type = SWITCH_DTEL_IFA_EDGE;
        it = pipe_it->second.insert(it,
                                    std::pair<_MatchKey, _ActionEntry>(
                                        _MatchKey(smi_id::T_DTEL_CONFIG),
                                        _ActionEntry(smi_id::T_DTEL_CONFIG)));
        status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_PKT_SRC,
                                        SWITCH_PKT_SRC_BRIDGED,
                                        pkt_src_mask);
        status |= it->first.set_ternary(
            smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
        status |= it->first.set_ternary(
            smi_id::F_DTEL_CONFIG_IPV4_HDR_VALID, true, true);
        status |= it->first.set_ternary(
            smi_id::F_DTEL_CONFIG_IPV4_DIFFSERV, ifa_dscp, ifa_dscp_mask);
        status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_REASON,
                                        SWITCH_DROP_REASON_UNKNOWN,
                                        static_cast<uint32_t>(0xFF));
        status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
        it->second.init_action_data(
            smi_id::A_DTEL_CONFIG_MIRROR_SWITCH_LOCAL_SET_F_AND_DROP);
        priority++;

        // #BI-4 if ifa_dscp and ipv6, mirror
        report_type = SWITCH_DTEL_IFA_EDGE;
        it = pipe_it->second.insert(it,
                                    std::pair<_MatchKey, _ActionEntry>(
                                        _MatchKey(smi_id::T_DTEL_CONFIG),
                                        _ActionEntry(smi_id::T_DTEL_CONFIG)));
        status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_PKT_SRC,
                                        SWITCH_PKT_SRC_BRIDGED,
                                        pkt_src_mask);
        status |= it->first.set_ternary(
            smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
        status |= it->first.set_ternary(
            smi_id::F_DTEL_CONFIG_IPV6_HDR_VALID, true, true);
        status |= it->first.set_ternary(
            smi_id::F_DTEL_CONFIG_IPV6_TRAFFIC_CLASS, ifa_dscp, ifa_dscp_mask);
        status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_REASON,
                                        SWITCH_DROP_REASON_UNKNOWN,
                                        static_cast<uint32_t>(0xFF));
        status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
        it->second.init_action_data(
            smi_id::A_DTEL_CONFIG_MIRROR_SWITCH_LOCAL_SET_F_AND_DROP);
        priority++;
      }

      priority = SWITCH_DTEL_CONFIG_IFA_CLONE_PRIORITY;

      /***********************************************************************
       * Bridged IFA clone entries
       ***********************************************************************/
      if ((feature::is_feature_set(SWITCH_FEATURE_DTEL_IFA_CLONE)) &&
          ifa_report) {
        // #BI-5 if report_type ifa_clone is set and IPv4 is valid, mirror
        report_type = SWITCH_DTEL_REPORT_TYPE_IFA_CLONE;
        it = pipe_it->second.insert(it,
                                    std::pair<_MatchKey, _ActionEntry>(
                                        _MatchKey(smi_id::T_DTEL_CONFIG),
                                        _ActionEntry(smi_id::T_DTEL_CONFIG)));
        status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_PKT_SRC,
                                        SWITCH_PKT_SRC_BRIDGED,
                                        pkt_src_mask);
        status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_REPORT_TYPE,
                                        report_type,
                                        SWITCH_DTEL_IFA_EDGE | report_type);
        status |= it->first.set_ternary(
            smi_id::F_DTEL_CONFIG_IPV4_HDR_VALID, true, true);
        status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_REASON,
                                        SWITCH_DROP_REASON_UNKNOWN,
                                        static_cast<uint32_t>(0xFF));
        status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_MIRROR_TYPE,
                                        SWITCH_MIRROR_TYPE_INVALID,
                                        static_cast<uint8_t>(0xFF));
        status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
        it->second.init_action_data(smi_id::A_DTEL_CONFIG_MIRROR_CLONE);
        priority++;

        // #BI-6 if report_type ifa_clone is set and IPv6 is valid, mirror
        report_type = SWITCH_DTEL_REPORT_TYPE_IFA_CLONE;
        it = pipe_it->second.insert(it,
                                    std::pair<_MatchKey, _ActionEntry>(
                                        _MatchKey(smi_id::T_DTEL_CONFIG),
                                        _ActionEntry(smi_id::T_DTEL_CONFIG)));
        status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_PKT_SRC,
                                        SWITCH_PKT_SRC_BRIDGED,
                                        pkt_src_mask);
        status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_REPORT_TYPE,
                                        report_type,
                                        SWITCH_DTEL_IFA_EDGE | report_type);
        status |= it->first.set_ternary(
            smi_id::F_DTEL_CONFIG_IPV6_HDR_VALID, true, true);
        status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_REASON,
                                        SWITCH_DROP_REASON_UNKNOWN,
                                        static_cast<uint32_t>(0xFF));
        status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_MIRROR_TYPE,
                                        SWITCH_MIRROR_TYPE_INVALID,
                                        static_cast<uint8_t>(0xFF));
        status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
        it->second.init_action_data(smi_id::A_DTEL_CONFIG_MIRROR_CLONE);
        priority++;
      }

      /***********************************************************************
       * Cloned egress IFA entries to set IFA DSCP in cloned packets
       ***********************************************************************/
      if ((feature::is_feature_set(SWITCH_FEATURE_DTEL_IFA_CLONE)) &&
          ifa_report) {
        // #CEI-1 if cloned egress packet and IPv4 is valid, set IFA DSCP
        it = pipe_it->second.insert(it,
                                    std::pair<_MatchKey, _ActionEntry>(
                                        _MatchKey(smi_id::T_DTEL_CONFIG),
                                        _ActionEntry(smi_id::T_DTEL_CONFIG)));
        status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_PKT_SRC,
                                        SWITCH_PKT_SRC_CLONED_EGRESS,
                                        pkt_src_mask);
        status |=
            it->first.set_ternary(smi_id::F_DTEL_CONFIG_IFA_CLONED, true, true);
        status |= it->first.set_ternary(
            smi_id::F_DTEL_CONFIG_IPV4_HDR_VALID, true, true);
        status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
        if (ifa_dscp_mask == 0x3F) {
          it->second.init_action_data(smi_id::A_DTEL_CONFIG_SET_IPV4_DSCP_ALL);
          it->second.set_arg(smi_id::D_DTEL_CONFIG_IPV4_DSCP_ALL, ifa_dscp);
        } else if (ifa_dscp_mask == 0x1) {
          it->second.init_action_data(smi_id::A_DTEL_CONFIG_SET_IPV4_DSCP_2);
          it->second.set_arg(smi_id::D_DTEL_CONFIG_IPV4_DSCP_BIT_2,
                             (uint8_t)((ifa_dscp & ifa_dscp_mask) ? 1 : 0));
        } else if (ifa_dscp_mask == 0x2) {
          it->second.init_action_data(smi_id::A_DTEL_CONFIG_SET_IPV4_DSCP_3);
          it->second.set_arg(smi_id::D_DTEL_CONFIG_IPV4_DSCP_BIT_3,
                             (uint8_t)((ifa_dscp & ifa_dscp_mask) ? 1 : 0));
        } else if (ifa_dscp_mask == 0x4) {
          it->second.init_action_data(smi_id::A_DTEL_CONFIG_SET_IPV4_DSCP_4);
          it->second.set_arg(smi_id::D_DTEL_CONFIG_IPV4_DSCP_BIT_4,
                             (uint8_t)((ifa_dscp & ifa_dscp_mask) ? 1 : 0));
        } else if (ifa_dscp_mask == 0x8) {
          it->second.init_action_data(smi_id::A_DTEL_CONFIG_SET_IPV4_DSCP_5);
          it->second.set_arg(smi_id::D_DTEL_CONFIG_IPV4_DSCP_BIT_5,
                             (uint8_t)((ifa_dscp & ifa_dscp_mask) ? 1 : 0));
        } else if (ifa_dscp_mask == 0x10) {
          it->second.init_action_data(smi_id::A_DTEL_CONFIG_SET_IPV4_DSCP_6);
          it->second.set_arg(smi_id::D_DTEL_CONFIG_IPV4_DSCP_BIT_6,
                             (uint8_t)((ifa_dscp & ifa_dscp_mask) ? 1 : 0));
        } else if (ifa_dscp_mask == 0x20) {
          it->second.init_action_data(smi_id::A_DTEL_CONFIG_SET_IPV4_DSCP_7);
          it->second.set_arg(smi_id::D_DTEL_CONFIG_IPV4_DSCP_BIT_7,
                             (uint8_t)((ifa_dscp & ifa_dscp_mask) ? 1 : 0));
        } else {
          it->second.init_action_data(smi_id::A_DTEL_CONFIG_DROP);
        }
        priority++;

        // #CEI-2 if cloned egress packet and IPv6 is valid, set IFA DSCP
        it = pipe_it->second.insert(it,
                                    std::pair<_MatchKey, _ActionEntry>(
                                        _MatchKey(smi_id::T_DTEL_CONFIG),
                                        _ActionEntry(smi_id::T_DTEL_CONFIG)));
        status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_PKT_SRC,
                                        SWITCH_PKT_SRC_CLONED_EGRESS,
                                        pkt_src_mask);
        status |=
            it->first.set_ternary(smi_id::F_DTEL_CONFIG_IFA_CLONED, true, true);
        status |= it->first.set_ternary(
            smi_id::F_DTEL_CONFIG_IPV6_HDR_VALID, true, true);
        status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
        if (ifa_dscp_mask == 0x3F) {
          it->second.init_action_data(smi_id::A_DTEL_CONFIG_SET_IPV6_DSCP_ALL);
          it->second.set_arg(smi_id::D_DTEL_CONFIG_IPV6_DSCP_ALL, ifa_dscp);
        } else if (ifa_dscp_mask == 0x1) {
          it->second.init_action_data(smi_id::A_DTEL_CONFIG_SET_IPV6_DSCP_2);
          it->second.set_arg(smi_id::D_DTEL_CONFIG_IPV6_DSCP_BIT_2,
                             (uint8_t)((ifa_dscp & ifa_dscp_mask) ? 1 : 0));
        } else if (ifa_dscp_mask == 0x2) {
          it->second.init_action_data(smi_id::A_DTEL_CONFIG_SET_IPV6_DSCP_3);
          it->second.set_arg(smi_id::D_DTEL_CONFIG_IPV6_DSCP_BIT_3,
                             (uint8_t)((ifa_dscp & ifa_dscp_mask) ? 1 : 0));
        } else if (ifa_dscp_mask == 0x4) {
          it->second.init_action_data(smi_id::A_DTEL_CONFIG_SET_IPV6_DSCP_4);
          it->second.set_arg(smi_id::D_DTEL_CONFIG_IPV6_DSCP_BIT_4,
                             (uint8_t)((ifa_dscp & ifa_dscp_mask) ? 1 : 0));
        } else if (ifa_dscp_mask == 0x8) {
          it->second.init_action_data(smi_id::A_DTEL_CONFIG_SET_IPV6_DSCP_5);
          it->second.set_arg(smi_id::D_DTEL_CONFIG_IPV6_DSCP_BIT_5,
                             (uint8_t)((ifa_dscp & ifa_dscp_mask) ? 1 : 0));
        } else if (ifa_dscp_mask == 0x10) {
          it->second.init_action_data(smi_id::A_DTEL_CONFIG_SET_IPV6_DSCP_6);
          it->second.set_arg(smi_id::D_DTEL_CONFIG_IPV6_DSCP_BIT_6,
                             (uint8_t)((ifa_dscp & ifa_dscp_mask) ? 1 : 0));
        } else if (ifa_dscp_mask == 0x20) {
          it->second.init_action_data(smi_id::A_DTEL_CONFIG_SET_IPV6_DSCP_7);
          it->second.set_arg(smi_id::D_DTEL_CONFIG_IPV6_DSCP_BIT_7,
                             (uint8_t)((ifa_dscp & ifa_dscp_mask) ? 1 : 0));
        } else {
          it->second.init_action_data(smi_id::A_DTEL_CONFIG_DROP);
        }
        priority++;

        // #CEI-3 if cloned egress packet (and not IPv4 or IPv6), drop
        it = pipe_it->second.insert(it,
                                    std::pair<_MatchKey, _ActionEntry>(
                                        _MatchKey(smi_id::T_DTEL_CONFIG),
                                        _ActionEntry(smi_id::T_DTEL_CONFIG)));
        status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_PKT_SRC,
                                        SWITCH_PKT_SRC_CLONED_EGRESS,
                                        pkt_src_mask);
        status |=
            it->first.set_ternary(smi_id::F_DTEL_CONFIG_IFA_CLONED, true, true);
        status |= it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, priority);
        it->second.init_action_data(smi_id::A_DTEL_CONFIG_DROP);
        priority++;
      }
    }
  }
  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    bool bf_rt_status = false;
    bool add = (get_auto_oid() == 0 ||
                switch_store::smiContext::context().in_warm_init());

    for (auto const &pipe_entry : pipe_entries) {
      bf_rt_target_t dev_pipe_tgt = {.dev_id = 0, .pipe_id = pipe_entry.first};
      _Table mt(dev_pipe_tgt, get_bf_rt_info(), smi_id::T_DTEL_CONFIG);
      for (auto const &entry : pipe_entry.second) {
        if (add) {
          status = mt.entry_add(entry.first, entry.second, bf_rt_status);
          if (status != SWITCH_STATUS_SUCCESS) {
            switch_log(SWITCH_API_LEVEL_ERROR,
                       SWITCH_OBJECT_TYPE_DTEL,
                       "{}.{}:{}: status:{} failed entry_add",
                       "dtel_config",
                       __func__,
                       __LINE__,
                       status);
            return status;
          }
        } else {
          status = mt.entry_modify(entry.first, entry.second);
          if (status != SWITCH_STATUS_SUCCESS) {
            switch_log(SWITCH_API_LEVEL_ERROR,
                       SWITCH_OBJECT_TYPE_DTEL,
                       "{}.{}:{}: status:{} failed entry_modify",
                       "dtel_config",
                       __func__,
                       __LINE__,
                       status);
            return status;
          }
        }
      }
    }

    status = auto_object::create_update();
    return status;
  }
  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    for (auto const &pipe_entry : pipe_entries) {
      bf_rt_target_t dev_pipe_tgt = {.dev_id = 0, .pipe_id = pipe_entry.first};
      for (auto const &entry : pipe_entry.second) {
        _Table mt(dev_pipe_tgt, get_bf_rt_info(), smi_id::T_DTEL_CONFIG);
        status = mt.entry_delete(entry.first);
        if (status != SWITCH_STATUS_SUCCESS) {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_DTEL,
                     "{}.{}:{}: status:{} failed entry_delete",
                     "dtel_config",
                     __func__,
                     __LINE__,
                     status);
          return status;
        }
      }
    }
    status = auto_object::del();
    return status;
  }
};

class mod_config : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_MOD_CONFIG;
  static const switch_attr_id_t status_attr_id = SWITCH_MOD_CONFIG_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_MOD_CONFIG_ATTR_PARENT_HANDLE;

 public:
  mod_config(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_MOD_CONFIG,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    bool drop_report = false;
    uint32_t drop_priority = 0;
    uint32_t report_type = SWITCH_DTEL_REPORT_TYPE_DROP;
    bf_rt_action_id_t action = smi_id::A_NO_ACTION;

    status |=
        switch_store::v_get(parent, SWITCH_DTEL_ATTR_DROP_REPORT, drop_report);

    auto it = match_action_list.begin();

    // Etrap entry
    // *, SWITCH_DTEL_REPORT_TYPE_ETRAP_CHANGE : mirror
    drop_priority = SWITCH_DTEL_MOD_CONFIG_DEFAULT_ETRAP_PRIORITY;
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_MOD_CONFIG),
                                           _ActionEntry(smi_id::T_MOD_CONFIG)));
    status |= it->first.set_ternary(
        smi_id::F_MOD_CONFIG_DTEL_MD_REPORT_TYPE,
        static_cast<uint32_t>(SWITCH_DTEL_REPORT_TYPE_ETRAP_CHANGE),
        static_cast<uint32_t>(SWITCH_DTEL_REPORT_TYPE_ETRAP_CHANGE));
    status |= it->first.set_exact(smi_id::F_MOD_CONFIG_PRIORITY, drop_priority);
    it->second.init_action_data(smi_id::A_MOD_CONFIG_MIRROR);

    // SWITCH_DROP_REASON_UNKNOWN, * : NoAction
    drop_priority = SWITCH_DTEL_MOD_CONFIG_DEFAULT_DROP_REASON_UNKNOWN_PRIORITY;
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_MOD_CONFIG),
                                           _ActionEntry(smi_id::T_MOD_CONFIG)));
    status |= it->first.set_ternary(smi_id::F_MOD_CONFIG_DROP_REASON,
                                    SWITCH_DROP_REASON_UNKNOWN,
                                    static_cast<uint32_t>(0xFF));
    status |= it->first.set_exact(smi_id::F_MOD_CONFIG_PRIORITY, drop_priority);
    it->second.init_action_data(smi_id::A_NO_ACTION);

    // L2 drop reason entries, report even if no watchlist hit
    drop_priority = SWITCH_DTEL_MOD_CONFIG_DEFAULT_L2_PRIORITY;
    if (drop_report) {
      action = smi_id::A_MOD_CONFIG_MIRROR_AND_SET_D_BIT;
    }
    std::vector<uint32_t> l2_drop_reasons;
    l2_drop_reasons.push_back(SWITCH_DROP_REASON_OUTER_SRC_MAC_ZERO);
    l2_drop_reasons.push_back(SWITCH_DROP_REASON_OUTER_SRC_MAC_MULTICAST);
    l2_drop_reasons.push_back(SWITCH_DROP_REASON_OUTER_DST_MAC_ZERO);
    for (uint32_t drop_reason : l2_drop_reasons) {
      it = match_action_list.insert(it,
                                    std::pair<_MatchKey, _ActionEntry>(
                                        _MatchKey(smi_id::T_MOD_CONFIG),
                                        _ActionEntry(smi_id::T_MOD_CONFIG)));
      status |= it->first.set_ternary(smi_id::F_MOD_CONFIG_DROP_REASON,
                                      drop_reason,
                                      static_cast<uint32_t>(0xFF));
      status |=
          it->first.set_exact(smi_id::F_MOD_CONFIG_PRIORITY, drop_priority);
      it->second.init_action_data(action);
      drop_priority++;
    }

    // Catch all entry
    // *, SWITCH_DTEL_REPORT_TYPE_DROP : mirror
    drop_priority = SWITCH_DTEL_MOD_CONFIG_CATCH_ALL_PRIORITY;
    if (drop_report) {
      action = smi_id::A_MOD_CONFIG_MIRROR;
    }
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_MOD_CONFIG),
                                           _ActionEntry(smi_id::T_MOD_CONFIG)));
    status |= it->first.set_ternary(
        smi_id::F_MOD_CONFIG_DTEL_MD_REPORT_TYPE, report_type, report_type);
    status |= it->first.set_exact(smi_id::F_MOD_CONFIG_PRIORITY, drop_priority);
    it->second.init_action_data(action);
  }
};

/** @brief Default session selector handle
 * Subscibes to default session selector auto object
 * This is an internal objected, created to add action member: no action to
 * session selector table
 * This entry is used by Match Action Table:mirror_session, when the dtel report
 * session is
 * deleted and the corresponding group and action members are deleted from
 * session selector table.
 * This entry gets created when device is created and removed when device is
 * deleted.
 */
class default_session_selector : public p4_object_action_selector {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DEFAULT_SESSION_SELECTOR;
  static const switch_attr_id_t status_attr_id =
      SWITCH_DEFAULT_SESSION_SELECTOR_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DEFAULT_SESSION_SELECTOR_ATTR_PARENT_HANDLE;

 public:
  default_session_selector(const switch_object_id_t parent,
                           switch_status_t &status)
      : p4_object_action_selector(smi_id::AP_SESSION_SELECTOR,
                                  smi_id::F_SESSION_SELECTOR_ACTION_MEMBER_ID,
                                  status_attr_id,
                                  auto_ot,
                                  parent_attr_id,
                                  parent) {
    status |= match_key.set_exact(
        smi_id::F_SESSION_SELECTOR_ACTION_MEMBER_ID,
        static_cast<uint32_t>(SWITCH_DTEL_SESSION_DEFAULT_ACT_PROF_ID));
    action_entry.init_action_data(smi_id::A_NO_ACTION);
  }
};

/** @brief session selector handle
 * Subscibes to dtel mirror session auto object
 * Creates action members, one for each dtel session dst ip, in session selector
 * table.
 * Uses mirror session ids as both key (action member id) and action parameter
 * (action data).
 * Since a report session provides a list of dst ips, we add all selector table
 * entries at once.
 */
class session_selector : public p4_object_action_selector_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_SESSION_SELECTOR;
  static const switch_attr_id_t status_attr_id =
      SWITCH_SESSION_SELECTOR_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_SESSION_SELECTOR_ATTR_PARENT_HANDLE;

 public:
  session_selector(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_action_selector_list(
            smi_id::AP_SESSION_SELECTOR,
            smi_id::F_SESSION_SELECTOR_ACTION_MEMBER_ID,
            status_attr_id,
            auto_ot,
            parent_attr_id,
            parent) {
    std::vector<uint16_t> session_ids;
    status |= switch_store::v_get(
        parent, SWITCH_REPORT_SESSION_ATTR_MIRROR_SESSION_ID, session_ids);

    auto it = match_action_list.begin();
    for (uint16_t session_id : session_ids) {
      it = match_action_list.insert(
          it,
          std::pair<_MatchKey, _ActionEntry>(
              _MatchKey(smi_id::AP_SESSION_SELECTOR),
              _ActionEntry(smi_id::AP_SESSION_SELECTOR)));
      status |= it->first.set_exact(smi_id::F_SESSION_SELECTOR_ACTION_MEMBER_ID,
                                    session_id);
      it->second.init_action_data(smi_id::A_SET_MIRROR_SESSION);
      status |= it->second.set_arg(smi_id::P_SET_MIRROR_SESSION_SESSION_ID,
                                   session_id);
      it++;
    }
  }
};

/** @brief session selector group handle
 * Subscibes to session selector auto object
 * Creates selector group for each session selector object and adds all action
 * member ids
 * of session selector to a single group. Action member id is assumed to map
 * one-one with mirror session ids.
 */
class session_selector_group : public p4_object_selector_group {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_SESSION_SELECTOR_GROUP;
  static const switch_attr_id_t status_attr_id =
      SWITCH_SESSION_SELECTOR_GROUP_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_SESSION_SELECTOR_GROUP_ATTR_PARENT_HANDLE;

 public:
  session_selector_group(const switch_object_id_t parent,
                         switch_status_t &status)
      : p4_object_selector_group(
            smi_id::SG_SESSION_SELECTOR_GROUP,
            status_attr_id,
            smi_id::P_SESSION_SELECTOR_GROUP_MAX_GROUP_SIZE,
            smi_id::P_SESSION_SELECTOR_GROUP_ACTION_MEMBER_ID_ARRAY,
            smi_id::P_SESSION_SELECTOR_GROUP_ACTION_MEMBER_STATUS_ARRAY,
            auto_ot,
            parent_attr_id,
            parent) {
    std::vector<uint16_t> session_ids;
    status |= switch_store::v_get(
        parent, SWITCH_REPORT_SESSION_ATTR_MIRROR_SESSION_ID, session_ids);

    std::vector<bf_rt_id_t> action_mbr_ids;
    std::vector<bool> action_mbrs_sts;
    // bf_rt_id_t action_mbr_id = {0};
    bool action_mbr_sts = true;

    if (!session_ids.size()) {
      /* We cannot have an empty group, so we create a group with default action
      member. This gets updated later on
      when monitor/report session ips are added */
      action_mbr_ids.push_back(
          static_cast<uint32_t>(SWITCH_DTEL_SESSION_DEFAULT_ACT_PROF_ID));
      action_mbrs_sts.push_back(action_mbr_sts);
    } else {
      // We reuse the mirror session ids allocated earlier as action member ids
      // and keep a one-one mapping between them
      for (uint16_t session_id : session_ids) {
        action_mbr_ids.push_back(static_cast<uint32_t>(session_id));
        action_mbrs_sts.push_back(action_mbr_sts);
      }
    }

    status |= match_key.set_exact(smi_id::F_SESSION_SELECTOR_GROUP_ID, parent);
    status |= action_entry.init_indirect_data();
    status |= action_entry.set_arg(
        smi_id::P_SESSION_SELECTOR_GROUP_MAX_GROUP_SIZE,
        p4_object_selector_group::SELECTOR_GROUP_MAX_GROUP_SIZE);
    status |= action_entry.set_arg(
        smi_id::P_SESSION_SELECTOR_GROUP_ACTION_MEMBER_ID_ARRAY,
        action_mbr_ids,
        true);
    status |= action_entry.set_arg(
        smi_id::P_SESSION_SELECTOR_GROUP_ACTION_MEMBER_STATUS_ARRAY,
        action_mbrs_sts,
        true);
  }
};

/** @brief dtel mirror session table handle
 * Subscibes to session selector auto object
 * Adds entry to keyless mirror session table. For a keyless table only default
 * entry add is supported.
 * Right now we support only one report session per switch, and thus this entry
 * sets the default entry to
 * point to the singular selector group created earlier.
 */
class dtel_mirror_session_table : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DTEL_MIRROR_SESSION_TABLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_DTEL_MIRROR_SESSION_TABLE_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DTEL_MIRROR_SESSION_TABLE_ATTR_PARENT_HANDLE;

 public:
  dtel_mirror_session_table(const switch_object_id_t parent,
                            switch_status_t &status)
      : p4_object_match_action(smi_id::T_DTEL_MIRROR_SESSION,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    bool value = true;
    status |= match_key.set_ternary(
        smi_id::F_DTEL_MIRROR_SESSION_HDR_ETHERNET_VALID, value, value);
    status |= action_entry.init_indirect_data();
    status |= action_entry.set_arg(
        smi_id::D_DTEL_MIRROR_SESSION_SELECTOR_GROUP_ID, parent);
  }
};

class recirc_rif : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_RECIRC_RIF;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_RECIRC_RIF_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id = SWITCH_RECIRC_RIF_ATTR_STATUS;
  std::vector<switch_object_id_t> recirc_port_handles;
  switch_object_id_t vrf_handle = {0};
  switch_object_id_t device_handle = {0};
  switch_mac_addr_t src_mac = {};

 public:
  recirc_rif(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status |= switch_store::v_get(
        parent, SWITCH_REPORT_SESSION_ATTR_VRF_HANDLE, vrf_handle);
    status |= switch_store::v_get(
        parent, SWITCH_REPORT_SESSION_ATTR_DEVICE, device_handle);
    status |=
        switch_store::v_get(device_handle, SWITCH_DEVICE_ATTR_SRC_MAC, src_mac);
    std::vector<uint16_t> port_list;
    status |= switch_store::v_get(
        device_handle, SWITCH_DEVICE_ATTR_RECIRC_PORT_LIST, port_list);
    /* Recirc port ids */
    if (!port_list.size()) {
      status = SWITCH_STATUS_ITEM_NOT_FOUND;
      switch_log(SWITCH_API_LEVEL_INFO,
                 SWITCH_OBJECT_TYPE_REPORT_SESSION,
                 "{}.{}: No recirc ports for device {} found."
                 "{} status {}",
                 __func__,
                 __LINE__,
                 device_handle,
                 status);
      return;
    }

    switch_object_id_t port_handle = {0};
    std::set<attr_w> port_attrs;
    port_attrs.insert(attr_w(SWITCH_PORT_ATTR_DEVICE, device_handle));
    for (uint16_t port_id : port_list) {
      port_attrs.insert(
          attr_w(SWITCH_PORT_ATTR_PORT_ID, static_cast<uint64_t>(port_id)));
      status |= switch_store::object_id_get_wkey(
          SWITCH_OBJECT_TYPE_PORT, port_attrs, port_handle);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_REPORT_SESSION,
                   "{}:{}: Failed to fetch port handle for recirc port id {} "
                   "error: {}",
                   __func__,
                   __LINE__,
                   port_id,
                   status);
        return;
      }
      port_attrs.erase(
          attr_w(SWITCH_PORT_ATTR_PORT_ID, static_cast<uint64_t>(port_id)));
      recirc_port_handles.push_back(port_handle);
    }
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    bool bf_rt_status = false;
    auto_object::create_update();
    switch_object_id_t oid = {0};
    oid = get_auto_oid();
    status |= switch_store::v_get(oid, status_attr_id, bf_rt_status);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_REPORT_SESSION,
                 "{}:{}: failed to get bf_rt_status status {}",
                 __func__,
                 __LINE__,
                 status);
      return status;
    }

    if (switch_store::smiContext::context().in_warm_init()) {
      bf_rt_status = true;
    } else if (bf_rt_status) {
      // TODO(bfn): update of recirc rifs
      bf_rt_status = true;
    } else {
      std::vector<switch_object_id_t> rif_handles;
      std::set<attr_w> rif_attrs;
      switch_enum_t e = {.enumdata = SWITCH_RIF_ATTR_TYPE_PORT};
      rif_attrs.insert(attr_w(SWITCH_RIF_ATTR_DEVICE, device_handle));
      rif_attrs.insert(attr_w(SWITCH_RIF_ATTR_VRF_HANDLE, vrf_handle));
      rif_attrs.insert(attr_w(SWITCH_RIF_ATTR_SRC_MAC, src_mac));
      rif_attrs.insert(attr_w(SWITCH_RIF_ATTR_TYPE, e));
      for (auto recirc_port : recirc_port_handles) {
        rif_attrs.insert(attr_w(SWITCH_RIF_ATTR_PORT_HANDLE, recirc_port));
        /* create rif object */
        switch_object_id_t rif_handle = {};
        status |= switch_store::object_create(
            SWITCH_OBJECT_TYPE_RIF, rif_attrs, rif_handle);
        if (status != SWITCH_STATUS_SUCCESS) {
          switch_log(
              SWITCH_API_LEVEL_ERROR,
              SWITCH_OBJECT_TYPE_REPORT_SESSION,
              "{}:{}: RIF creation failed for for recirc port id {} error: {}",
              __func__,
              __LINE__,
              recirc_port,
              status);
          return status;
        }
        status |= switch_store::v_set(
            rif_handle, SWITCH_RIF_ATTR_INTERNAL_OBJECT, true);
        rif_handles.push_back(rif_handle);
        rif_attrs.erase(attr_w(SWITCH_RIF_ATTR_PORT_HANDLE, recirc_port));
      }
      attr_w rif_handle_list(SWITCH_REPORT_SESSION_ATTR_RIF_HANDLES);
      rif_handle_list.v_set(rif_handles);
      status |= switch_store::attribute_set(get_parent(), rif_handle_list);
      if (status == SWITCH_STATUS_SUCCESS) {
        bf_rt_status = true;
      }
    }

    status |= switch_store::v_set(oid, status_attr_id, bf_rt_status);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_REPORT_SESSION,
                 "{}:{}: failed to set bf_rt_status status {}",
                 __func__,
                 __LINE__,
                 status);
      return status;
    }
    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    std::vector<switch_object_id_t> rif_handles;

    status |= switch_store::v_get(
        get_parent(), SWITCH_REPORT_SESSION_ATTR_RIF_HANDLES, rif_handles);
    status |= switch_store::list_clear(get_parent(),
                                       SWITCH_REPORT_SESSION_ATTR_RIF_HANDLES);

    for (const auto rif_handle : rif_handles) {
      status |= switch_store::object_delete(rif_handle);
    }
    status |= auto_object::del();
    return status;
  }
};

class queue_alert : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_QUEUE_ALERT;
  static const switch_attr_id_t status_attr_id = SWITCH_QUEUE_ALERT_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_QUEUE_ALERT_ATTR_PARENT_HANDLE;

 public:
  queue_alert(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_QUEUE_REPORT_ALERT,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    uint8_t qid = 0;
    uint16_t dev_port = 0;
    uint8_t quant_mask = 0;
    switch_object_id_t queue_handle = {}, port_handle = {}, device_handle = {};

    status |= switch_store::v_get(
        parent, SWITCH_QUEUE_REPORT_ATTR_DEVICE, device_handle);
    status |= switch_store::v_get(
        parent, SWITCH_QUEUE_REPORT_ATTR_QUEUE_HANDLE, queue_handle);
    status |=
        switch_store::v_get(queue_handle, SWITCH_QUEUE_ATTR_QUEUE_ID, qid);
    status |= switch_store::v_get(
        queue_handle, SWITCH_QUEUE_ATTR_PORT_HANDLE, port_handle);
    status |=
        switch_store::v_get(port_handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
    status |= switch_store::v_get(
        device_handle, SWITCH_DEVICE_ATTR_LATENCY_SENSITIVITY, quant_mask);

    bf_rt_target_t dev_target = {
        .dev_id = 0,
        .pipe_id = static_cast<bf_dev_pipe_t> DEV_PORT_TO_PIPE(dev_port)};
    device_tgt_set(dev_target);

    status |= match_key.set_exact(smi_id::F_QUEUE_REPORT_ALERT_QID, qid);
    status |= match_key.set_exact(smi_id::F_QUEUE_REPORT_ALERT_PORT, dev_port);
    action_entry.init_action_data(smi_id::A_SET_QALERT);
    status |=
        action_entry.set_arg(smi_id::D_SET_QALERT_INDEX, QINDEX(dev_port, qid));
    status |= action_entry.set_arg(smi_id::D_SET_QALERT_QUOTA,
                                   parent,
                                   SWITCH_QUEUE_REPORT_ATTR_BREACH_QUOTA);
    status |=
        action_entry.set_arg(smi_id::D_SET_QALERT_QUANTIZATION_MASK,
                             static_cast<uint32_t>(~((1LL << quant_mask) - 1)));
  }
};

class check_quota : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_CHECK_QUOTA;
  static const switch_attr_id_t status_attr_id = SWITCH_CHECK_QUOTA_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_CHECK_QUOTA_ATTR_PARENT_HANDLE;

 public:
  check_quota(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_QUEUE_REPORT_CHECK_QUOTA,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    uint8_t qid = 0;
    uint16_t dev_port = 0;
    switch_object_id_t queue_handle = {}, port_handle = {};

    status |= switch_store::v_get(
        parent, SWITCH_QUEUE_REPORT_ATTR_QUEUE_HANDLE, queue_handle);
    status |=
        switch_store::v_get(queue_handle, SWITCH_QUEUE_ATTR_QUEUE_ID, qid);
    status |= switch_store::v_get(
        queue_handle, SWITCH_QUEUE_ATTR_PORT_HANDLE, port_handle);
    status |=
        switch_store::v_get(port_handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);

    bf_rt_target_t dev_target = {
        .dev_id = 0,
        .pipe_id = static_cast<bf_dev_pipe_t> DEV_PORT_TO_PIPE(dev_port)};
    device_tgt_set(dev_target);

    auto it = match_action_list.begin();

    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(
            _MatchKey(smi_id::T_QUEUE_REPORT_CHECK_QUOTA),
            _ActionEntry(smi_id::T_QUEUE_REPORT_CHECK_QUOTA)));
    status |= it->first.set_exact(smi_id::F_CHECK_QUOTA_PKT_SRC,
                                  SWITCH_PKT_SRC_BRIDGED);
    status |= it->first.set_exact(smi_id::F_CHECK_QUOTA_QALERT, true);
    status |= it->first.set_exact(smi_id::F_CHECK_QUOTA_QID, qid);
    status |= it->first.set_exact(smi_id::F_CHECK_QUOTA_PORT, dev_port);
    it->second.init_action_data(smi_id::A_CHECK_LATENCY_UPDATE_QUOTA);
    status |= it->second.set_arg(smi_id::D_CHECK_LATENCY_UPDATE_QUOTA_INDEX,
                                 QINDEX(dev_port, qid));

    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(
            _MatchKey(smi_id::T_QUEUE_REPORT_CHECK_QUOTA),
            _ActionEntry(smi_id::T_QUEUE_REPORT_CHECK_QUOTA)));
    status |= it->first.set_exact(smi_id::F_CHECK_QUOTA_PKT_SRC,
                                  SWITCH_PKT_SRC_BRIDGED);
    status |= it->first.set_exact(smi_id::F_CHECK_QUOTA_QALERT, false);
    status |= it->first.set_exact(smi_id::F_CHECK_QUOTA_QID, qid);
    status |= it->first.set_exact(smi_id::F_CHECK_QUOTA_PORT, dev_port);
    it->second.init_action_data(smi_id::A_RESET_QUOTA);
    status |=
        it->second.set_arg(smi_id::D_RESET_QUOTA_INDEX, QINDEX(dev_port, qid));

    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(
            _MatchKey(smi_id::T_QUEUE_REPORT_CHECK_QUOTA),
            _ActionEntry(smi_id::T_QUEUE_REPORT_CHECK_QUOTA)));
    status |= it->first.set_exact(smi_id::F_CHECK_QUOTA_PKT_SRC,
                                  SWITCH_PKT_SRC_DEFLECTED);
    status |= it->first.set_exact(smi_id::F_CHECK_QUOTA_QALERT, false);
    status |= it->first.set_exact(smi_id::F_CHECK_QUOTA_QID, qid);
    status |= it->first.set_exact(smi_id::F_CHECK_QUOTA_PORT, dev_port);
    it->second.init_action_data(smi_id::A_UPDATE_QUOTA);
    status |=
        it->second.set_arg(smi_id::D_UPDATE_QUOTA_INDEX, QINDEX(dev_port, qid));

    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(
            _MatchKey(smi_id::T_QUEUE_REPORT_CHECK_QUOTA),
            _ActionEntry(smi_id::T_QUEUE_REPORT_CHECK_QUOTA)));
    status |= it->first.set_exact(smi_id::F_CHECK_QUOTA_PKT_SRC,
                                  SWITCH_PKT_SRC_DEFLECTED);
    status |= it->first.set_exact(smi_id::F_CHECK_QUOTA_QALERT, true);
    status |= it->first.set_exact(smi_id::F_CHECK_QUOTA_QID, qid);
    status |= it->first.set_exact(smi_id::F_CHECK_QUOTA_PORT, dev_port);
    it->second.init_action_data(smi_id::A_UPDATE_QUOTA);
    status |=
        it->second.set_arg(smi_id::D_UPDATE_QUOTA_INDEX, QINDEX(dev_port, qid));
  }
};

switch_status_t update_thresholds(uint8_t pipe,
                                  uint32_t reg_index,
                                  uint32_t qdepth,
                                  uint32_t latency,
                                  uint32_t quota,
                                  bool valid) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  bf_rt_target_t dev_target = {.dev_id = 0, .pipe_id = pipe};

  _Table table(dev_target, get_bf_rt_info(), smi_id::T_QUEUE_REPORT_THRESHOLDS);
  _MatchKey register_key(smi_id::T_QUEUE_REPORT_THRESHOLDS);
  _ActionEntry register_action(smi_id::T_QUEUE_REPORT_THRESHOLDS);
  register_action.init_indirect_data();
  status |=
      register_key.set_exact(smi_id::F_THRESHOLDS_REGISTER_INDEX, reg_index);
  status |= register_action.set_arg(smi_id::D_THRESHOLDS_QDEPTH, qdepth);
  status |= register_action.set_arg(smi_id::D_THRESHOLDS_LATENCY, latency);
  status = table.entry_modify(register_key, register_action);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_QUEUE_REPORT,
               "{}:{}: failed qr thresholds status {} reg_index {} qdepth {} "
               "latency {} valid {}",
               __func__,
               __LINE__,
               status,
               reg_index,
               qdepth,
               latency,
               valid);
    return status;
  }

  _Table qtable(dev_target, get_bf_rt_info(), smi_id::T_QUEUE_REPORT_QUOTAS);
  _MatchKey qregister_key(smi_id::T_QUEUE_REPORT_QUOTAS);
  _ActionEntry qregister_action(smi_id::T_QUEUE_REPORT_QUOTAS);
  qregister_action.init_indirect_data();
  status |= qregister_key.set_exact(smi_id::F_QUOTAS_REGISTER_INDEX, reg_index);
  status |= qregister_action.set_arg(smi_id::D_QUOTAS_COUNTER, quota);
  status |= qregister_action.set_arg(smi_id::D_QUOTAS_LATENCY, latency);
  status = qtable.entry_modify(qregister_key, qregister_action);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_QUEUE_REPORT,
               "{}:{}: failed qr quota update status {} reg_index {} quota {} "
               "valid {}",
               __func__,
               __LINE__,
               status,
               reg_index,
               quota,
               valid);
    return status;
  }

  return status;
}

/* queue report thresholds register p4 object */
class queue_report_thresholds : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_QR_THRESHOLDS;
  static const switch_attr_id_t status_attr_id =
      SWITCH_QR_THRESHOLDS_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_QR_THRESHOLDS_ATTR_PARENT_HANDLE;
  uint8_t qid = 0;
  uint16_t dev_port = 0;
  uint32_t qdepth = 0, latency = 0, quota = 0;

 public:
  queue_report_thresholds(const switch_object_id_t parent,
                          switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    switch_object_id_t queue_handle = {}, port_handle = {};

    status |= switch_store::v_get(
        parent, SWITCH_QUEUE_REPORT_ATTR_QUEUE_HANDLE, queue_handle);
    status |=
        switch_store::v_get(queue_handle, SWITCH_QUEUE_ATTR_QUEUE_ID, qid);
    status |= switch_store::v_get(
        queue_handle, SWITCH_QUEUE_ATTR_PORT_HANDLE, port_handle);
    status |=
        switch_store::v_get(port_handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);

    status |= switch_store::v_get(
        parent, SWITCH_QUEUE_REPORT_ATTR_DEPTH_THRESHOLD, qdepth);
    status |= switch_store::v_get(
        parent, SWITCH_QUEUE_REPORT_ATTR_LATENCY_THRESHOLD, latency);
    status |= switch_store::v_get(
        parent, SWITCH_QUEUE_REPORT_ATTR_BREACH_QUOTA, quota);
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    status = auto_object::create_update();
    status |=
        update_thresholds(static_cast<uint8_t>(DEV_PORT_TO_PIPE(dev_port)),
                          static_cast<uint32_t>(QINDEX(dev_port, qid)),
                          qdepth,
                          latency,
                          quota,
                          true);
    return status;
  }
  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    status = update_thresholds(static_cast<uint8_t>(DEV_PORT_TO_PIPE(dev_port)),
                               static_cast<uint32_t>(QINDEX(dev_port, qid)),
                               0,
                               0,
                               0,
                               false);
    status |= auto_object::del();
    return status;
  }
};

/* DoD or Tail Drop can generate either of queue or drop reports.
This class is for the entries that enable DoD for drop reports, and
the override of DoD for queue reports when tail drops are disabled
in the global dtel object.
There is another class for the queue specific entries that enable DoD. */
class deflect_on_drop : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DEFLECT_ON_DROP;
  static const switch_attr_id_t status_attr_id =
      SWITCH_DEFLECT_ON_DROP_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DEFLECT_ON_DROP_ATTR_PARENT_HANDLE;

 public:
  deflect_on_drop(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_DOD_CONFIG,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    bool tail_drop_report = false;
    bool queue_report = false;
    uint32_t report_type = SWITCH_DTEL_REPORT_TYPE_DROP;
    uint32_t priority = 0x1;
    uint16_t mcast_grp_a = 0;
    uint16_t cpu_reason = 0;

    status |= switch_store::v_get(
        parent, SWITCH_DTEL_ATTR_TAIL_DROP_REPORT, tail_drop_report);
    status |= switch_store::v_get(
        parent, SWITCH_DTEL_ATTR_QUEUE_REPORT, queue_report);

    auto it = match_action_list.begin();

    // #1 Enable/disable dod drop reports based on global tail drop enable
    // flag
    it = match_action_list.insert(
        it,
        std::pair<_MatchKey, _ActionEntry>(_MatchKey(smi_id::T_DOD_CONFIG),
                                           _ActionEntry(smi_id::T_DOD_CONFIG)));
    status |=
        it->first.set_ternary(smi_id::F_DOD_CONFIG_LOCAL_MD_DTEL_REPORT_TYPE,
                              report_type,
                              report_type);
    status |= it->first.set_ternary(smi_id::F_DOD_CONFIG_LOCAL_MD_MULTICAST_ID,
                                    mcast_grp_a,
                                    static_cast<uint16_t>(0xFFFF));
    status |= it->first.set_ternary(smi_id::F_DOD_CONFIG_LOCAL_MD_CPU_REASON,
                                    cpu_reason,
                                    static_cast<uint16_t>(0xFFFF));
    status |= it->first.set_exact(smi_id::F_DOD_CONFIG_PRIORITY, priority);
    if (tail_drop_report) {
      it->second.init_action_data(smi_id::A_DOD_CONFIG_ENABLE_DOD);
    } else {
      it->second.init_action_data(smi_id::A_DOD_CONFIG_DISABLE_DOD);
    }
    priority++;
    it++;

    // #2 Disable dod queue reports based on global queue report flag
    report_type = SWITCH_DTEL_REPORT_TYPE_QUEUE;
    if (!queue_report || !tail_drop_report) {
      it = match_action_list.insert(it,
                                    std::pair<_MatchKey, _ActionEntry>(
                                        _MatchKey(smi_id::T_DOD_CONFIG),
                                        _ActionEntry(smi_id::T_DOD_CONFIG)));
      status |= it->first.set_exact(smi_id::F_DOD_CONFIG_PRIORITY, priority);
      it->second.init_action_data(smi_id::A_DOD_CONFIG_DISABLE_DOD);
    }
  }
};

/* DoD or Tail Drop can generate either of queue or drop reports.
This class is for the queue specific entries that enable DoD,
controlled via the queue_report object. */
class deflect_on_drop_queue : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DEFLECT_ON_DROP_QUEUE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_DEFLECT_ON_DROP_QUEUE_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DEFLECT_ON_DROP_QUEUE_ATTR_PARENT_HANDLE;

 public:
  deflect_on_drop_queue(const switch_object_id_t parent,
                        switch_status_t &status)
      : p4_object_match_action(smi_id::T_DOD_CONFIG,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_object_id_t queue_handle = {}, port_handle = {};
    uint8_t qid = 0;
    uint16_t dev_port = 0;
    uint16_t mcast_grp_a = 0;
    uint16_t cpu_reason = 0;
    bool tail_drop_report = false;
    // same priority works for all queue entries as other key fields will
    // necessarily be same
    uint32_t priority = 0x3;
    status |= switch_store::v_get(
        parent, SWITCH_QUEUE_REPORT_ATTR_QUEUE_HANDLE, queue_handle);
    status |=
        switch_store::v_get(queue_handle, SWITCH_QUEUE_ATTR_QUEUE_ID, qid);
    status |= switch_store::v_get(
        queue_handle, SWITCH_QUEUE_ATTR_PORT_HANDLE, port_handle);
    status |=
        switch_store::v_get(port_handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
    status |= switch_store::v_get(
        parent, SWITCH_QUEUE_REPORT_ATTR_TAIL_DROP, tail_drop_report);

    status |= match_key.set_ternary(
        smi_id::F_DOD_CONFIG_QID, qid, static_cast<uint8_t>(0x1F));
    status |= match_key.set_ternary(smi_id::F_DOD_CONFIG_EGRESS_PORT,
                                    dev_port,
                                    static_cast<uint16_t>(0x1FF));
    status |= match_key.set_ternary(smi_id::F_DOD_CONFIG_LOCAL_MD_MULTICAST_ID,
                                    mcast_grp_a,
                                    static_cast<uint16_t>(0xFFFF));
    status |= match_key.set_ternary(smi_id::F_DOD_CONFIG_LOCAL_MD_CPU_REASON,
                                    cpu_reason,
                                    static_cast<uint16_t>(0xFFFF));
    status |= match_key.set_exact(smi_id::F_DOD_CONFIG_PRIORITY, priority);
    if (tail_drop_report) {
      action_entry.init_action_data(smi_id::A_DOD_CONFIG_ENABLE_DOD);
    } else {
      action_entry.init_action_data(smi_id::A_DOD_CONFIG_DISABLE_DOD);
    }
  }
};

class tm_dod_config : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_TM_DOD_CONFIG;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_TM_DOD_CONFIG_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_TM_DOD_CONFIG_ATTR_STATUS;
  uint16_t dev_id = 0;
  std::vector<uint16_t> dev_port_list;

 public:
  tm_dod_config(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    switch_object_id_t device_handle = {0};
    status |=
        switch_store::v_get(parent, SWITCH_DTEL_ATTR_DEVICE, device_handle);
    status |=
        switch_store::v_get(device_handle, SWITCH_DEVICE_ATTR_DEV_ID, dev_id);
    status |= switch_store::v_get(
        device_handle, SWITCH_DEVICE_ATTR_RECIRC_DEV_PORT_LIST, dev_port_list);
    if (!dev_port_list.size()) {
      status = SWITCH_STATUS_ITEM_NOT_FOUND;
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_DTEL,
                 "{}.{}: Failed to configure deflect on drop destination. "
                 "Dest:RECIRC ports not configured {}",
                 __func__,
                 __LINE__,
                 status);
    }
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    p4_pd_status_t pd_status = 0;
    uint8_t pipe_id = 0;
    (void)pd_status;

    auto_object::create_update();
    bf_dev_family_t dev_family =
        bf_dev_type_to_family(lld_sku_get_dev_type(dev_id));

    for (uint16_t dev_port : dev_port_list) {
      // TODO(bfn): Check whether pipe translation is needed here
      pipe_id = DEV_PORT_TO_PIPE(dev_port);
      pd_status =
          p4_pd_tm_set_negative_mirror_dest(dev_id, pipe_id, dev_port, 0);
      CHECK_RET(pd_status != 0, SWITCH_STATUS_FAILURE);
      if (dev_family != BF_DEV_FAMILY_TOFINO) {
        pd_status = bf_tm_pipe_deflection_port_enable_set(dev_id, pipe_id, 1);
        CHECK_RET(pd_status != 0, SWITCH_STATUS_FAILURE);
      }
    }

    return status;
  }
};

#define TOF_MULTIPLIER 4
#define TOF2_MULTIPLIER 8
#define TOF3_MULTIPLIER 8
class ingress_port_conversion : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_INGRESS_PORT_CONVERSION;
  static const switch_attr_id_t status_attr_id =
      SWITCH_INGRESS_PORT_CONVERSION_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INGRESS_PORT_CONVERSION_ATTR_PARENT_HANDLE;

 public:
  ingress_port_conversion(const switch_object_id_t parent,
                          switch_status_t &status)
      : p4_object_match_action(smi_id::T_INGRESS_PORT_CONVERSION,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    uint32_t chnl_id = 0, conn_id = 0;
    uint16_t value = 0;
    switch_enum_t e = {};

    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_TYPE, e);
    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_CHANNEL_ID, chnl_id);
    status |=
        switch_store::v_get(parent, SWITCH_PORT_ATTR_CONNECTOR_ID, conn_id);

    status |= match_key.set_exact(smi_id::F_INGRESS_PORT_CONVERSION_PORT,
                                  parent,
                                  SWITCH_PORT_ATTR_DEV_PORT);
    status |= match_key.set_exact(
        smi_id::F_INGRESS_PORT_CONVERSION_REPORT_VALID, true);
    action_entry.init_action_data(smi_id::A_CONVERT_INGRESS_PORT);
    if (bf_lld_dev_is_tof3(0)) {
      value = (TOF3_MULTIPLIER * conn_id) + chnl_id;
    } else if (bf_lld_dev_is_tof2(0)) {
      value = (TOF2_MULTIPLIER * conn_id) + chnl_id;
    } else {
      value = (TOF_MULTIPLIER * conn_id) + chnl_id;
    }
    if (e.enumdata == SWITCH_PORT_ATTR_TYPE_CPU) value = 502;
    status |= action_entry.set_arg(smi_id::D_CONVERT_INGRESS_PORT_PORT, value);
  }
};

class egress_port_conversion : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_EGRESS_PORT_CONVERSION;
  static const switch_attr_id_t status_attr_id =
      SWITCH_EGRESS_PORT_CONVERSION_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_EGRESS_PORT_CONVERSION_ATTR_PARENT_HANDLE;

 public:
  egress_port_conversion(const switch_object_id_t parent,
                         switch_status_t &status)
      : p4_object_match_action(smi_id::T_EGRESS_PORT_CONVERSION,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    uint32_t chnl_id = 0, conn_id = 0;
    uint16_t value = 0;
    switch_enum_t e = {};

    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_TYPE, e);
    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_CHANNEL_ID, chnl_id);
    status |=
        switch_store::v_get(parent, SWITCH_PORT_ATTR_CONNECTOR_ID, conn_id);

    status |= match_key.set_exact(smi_id::F_EGRESS_PORT_CONVERSION_PORT,
                                  parent,
                                  SWITCH_PORT_ATTR_DEV_PORT);
    status |= match_key.set_exact(smi_id::F_EGRESS_PORT_CONVERSION_REPORT_VALID,
                                  true);
    action_entry.init_action_data(smi_id::A_CONVERT_EGRESS_PORT);
    if (bf_lld_dev_is_tof3(0)) {
      value = (TOF3_MULTIPLIER * conn_id) + chnl_id;
    } else if (bf_lld_dev_is_tof2(0)) {
      value = (TOF2_MULTIPLIER * conn_id) + chnl_id;
    } else {
      value = (TOF_MULTIPLIER * conn_id) + chnl_id;
    }
    if (e.enumdata == SWITCH_PORT_ATTR_TYPE_CPU) value = 502;
    status |= action_entry.set_arg(smi_id::D_CONVERT_EGRESS_PORT_PORT, value);
  }
};

class int_edge : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_INT_EDGE;
  static const switch_attr_id_t status_attr_id = SWITCH_INT_EDGE_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INT_EDGE_ATTR_PARENT_HANDLE;

 public:
  int_edge(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_INT_EDGE_PORT_LOOKUP,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    uint16_t clone_session_id = 0;
    bool int_edge_port;

    status |= switch_store::v_get(
        parent, SWITCH_PORT_ATTR_CLONE_MIRROR_SESSION_ID, clone_session_id);
    status |= switch_store::v_get(
        parent, SWITCH_PORT_ATTR_DTEL_INT_EDGE, int_edge_port);

    status |= match_key.set_exact(
        smi_id::F_INT_EDGE_PORT_LOOKUP_PORT, parent, SWITCH_PORT_ATTR_DEV_PORT);
    if (int_edge_port) {
      action_entry.init_action_data(smi_id::A_INT_EDGE_SET_IFA_EDGE);
    } else {
      action_entry.init_action_data(
          smi_id::A_INT_EDGE_SET_CLONE_MIRROR_SESSION_ID);
      status |= action_entry.set_arg(smi_id::D_INT_EDGE_CLONE_MIRROR_SESSION_ID,
                                     clone_session_id);
    }
  }
};

class mod_config_drop_reason : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_MOD_CONFIG_DROP_REASON;
  static const switch_attr_id_t status_attr_id =
      SWITCH_MOD_CONFIG_DROP_REASON_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_MOD_CONFIG_DROP_REASON_ATTR_PARENT_HANDLE;

 public:
  mod_config_drop_reason(const switch_object_id_t parent,
                         switch_status_t &status)
      : p4_object_match_action(smi_id::T_MOD_CONFIG,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_object_id_t dtel_handle = {};
    uint32_t report_type = SWITCH_DTEL_REPORT_TYPE_DROP;
    uint32_t drop_priority = 0;
    uint32_t drop_reason = 0;
    uint32_t drop_reason_mask = 0xFF;
    bool enable = false;
    bool ingress = false;
    bool drop_report = false;

    status |= switch_store::v_get(
        parent, SWITCH_DTEL_DROP_CONTROL_ATTR_DROP_REASON, drop_reason);

    status |= switch_store::v_get(
        parent, SWITCH_DTEL_DROP_CONTROL_ATTR_ENABLE, enable);

    status |= switch_store::v_get(
        parent, SWITCH_DTEL_DROP_CONTROL_ATTR_DTEL_HANDLE, dtel_handle);

    if (dtel_handle == 0) return;

    status |= switch_store::v_get(
        dtel_handle, SWITCH_DTEL_ATTR_DROP_REPORT, drop_report);

    for (auto reason : ingress_drop_reasons) {
      if (drop_reason == reason) ingress = true;
    }

    if (!ingress) return;

    drop_priority = SWITCH_DTEL_MOD_CONFIG_USER_PRIO_BASE;

    status |= match_key.set_ternary(
        smi_id::F_MOD_CONFIG_DROP_REASON, drop_reason, drop_reason_mask);

    status |= match_key.set_ternary(
        smi_id::F_MOD_CONFIG_DTEL_MD_REPORT_TYPE, report_type, report_type);
    status |= match_key.set_exact(smi_id::F_MOD_CONFIG_PRIORITY, drop_priority);

    if (!enable || !drop_report) {
      action_entry.init_action_data(smi_id::A_NO_ACTION);
    } else if (drop_reason == SWITCH_DROP_REASON_OUTER_SRC_MAC_ZERO ||
               drop_reason == SWITCH_DROP_REASON_OUTER_SRC_MAC_MULTICAST ||
               drop_reason == SWITCH_DROP_REASON_OUTER_SRC_MAC_MULTICAST) {
      action_entry.init_action_data(smi_id::A_MOD_CONFIG_MIRROR_AND_SET_D_BIT);
    } else {
      action_entry.init_action_data(smi_id::A_MOD_CONFIG_MIRROR);
    }
  }
};

class dtel_config_drop_reason : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DTEL_CONFIG_DROP_REASON;
  static const switch_attr_id_t status_attr_id =
      SWITCH_DTEL_CONFIG_DROP_REASON_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DTEL_CONFIG_DROP_REASON_ATTR_PARENT_HANDLE;

  typedef std::vector<std::pair<_MatchKey, _ActionEntry>> MatchAction;
  std::vector<std::pair<bf_dev_pipe_t, MatchAction>> pipe_entries;

 public:
  dtel_config_drop_reason(const switch_object_id_t parent,
                          switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    switch_object_id_t dtel_handle = {};
    bool drop_report = false, queue_report = false;
    uint32_t drop_priority = 0;
    uint32_t report_type = SWITCH_DTEL_REPORT_TYPE_NONE;
    uint8_t pkt_src_mask = 0xFF;

    uint32_t drop_reason = 0;
    uint32_t drop_reason_mask = 0xFF;
    bool egress = false;
    bool enable = false;

    status |= switch_store::v_get(
        parent, SWITCH_DTEL_DROP_CONTROL_ATTR_DROP_REASON, drop_reason);
    status |= switch_store::v_get(
        parent, SWITCH_DTEL_DROP_CONTROL_ATTR_DTEL_HANDLE, dtel_handle);

    status |= switch_store::v_get(
        parent, SWITCH_DTEL_DROP_CONTROL_ATTR_ENABLE, enable);

    if (dtel_handle == 0) return;

    status |= switch_store::v_get(
        dtel_handle, SWITCH_DTEL_ATTR_DROP_REPORT, drop_report);
    status |= switch_store::v_get(
        dtel_handle, SWITCH_DTEL_ATTR_QUEUE_REPORT, queue_report);

    for (auto reason : egress_drop_reasons) {
      if (drop_reason == reason) egress = true;
    }

    if (!egress) return;

    auto pipe_it = pipe_entries.begin();
    for (bf_dev_pipe_t pipe :
         _Table(smi_id::T_DTEL_CONFIG).get_active_pipes()) {
      drop_priority = SWITCH_DTEL_CONFIG_USER_PRIO_BASE;
      MatchAction match_action_list;
      pipe_it =
          pipe_entries.insert(pipe_it, std::make_pair(pipe, match_action_list));
      auto it = pipe_it->second.begin();

      it = pipe_it->second.insert(it,
                                  std::pair<_MatchKey, _ActionEntry>(
                                      _MatchKey(smi_id::T_DTEL_CONFIG),
                                      _ActionEntry(smi_id::T_DTEL_CONFIG)));

      // egress drop_reason values with report_type DROP and qalert true.
      report_type = SWITCH_DTEL_REPORT_TYPE_DROP;
      status |= it->first.set_ternary(
          smi_id::F_DTEL_CONFIG_PKT_SRC, SWITCH_PKT_SRC_BRIDGED, pkt_src_mask);
      status |= it->first.set_ternary(
          smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
      status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_QUEUE_REPORT_FLAG,
                                      static_cast<uint8_t>(1),
                                      static_cast<uint8_t>(1));
      status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_MIRROR_TYPE,
                                      SWITCH_MIRROR_TYPE_INVALID,
                                      static_cast<uint8_t>(0xFF));

      status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_REASON,
                                      drop_reason,
                                      static_cast<uint32_t>(drop_reason_mask));

      status |=
          it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, drop_priority);

      if (enable && drop_report && queue_report) {
        it->second.init_action_data(smi_id::A_DTEL_CONFIG_MIRROR_DROP_SET_Q);
      } else if (enable && drop_report) {
        it->second.init_action_data(smi_id::A_DTEL_CONFIG_MIRROR_DROP);
      } else if (queue_report) {
        it->second.init_action_data(
            smi_id::A_DTEL_CONFIG_MIRROR_SWITCH_LOCAL_SET_Q);
      } else {
        it->second.init_action_data(smi_id::A_NO_ACTION);
      }

      drop_priority++;
      it = pipe_it->second.insert(it,
                                  std::pair<_MatchKey, _ActionEntry>(
                                      _MatchKey(smi_id::T_DTEL_CONFIG),
                                      _ActionEntry(smi_id::T_DTEL_CONFIG)));

      // egress drop_reason values with report_type DROP
      report_type = SWITCH_DTEL_REPORT_TYPE_DROP;
      status |= it->first.set_ternary(
          smi_id::F_DTEL_CONFIG_PKT_SRC, SWITCH_PKT_SRC_BRIDGED, pkt_src_mask);
      status |= it->first.set_ternary(
          smi_id::F_DTEL_CONFIG_REPORT_TYPE, report_type, report_type);
      status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_MIRROR_TYPE,
                                      SWITCH_MIRROR_TYPE_INVALID,
                                      static_cast<uint8_t>(0xFF));
      status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_REASON,
                                      drop_reason,
                                      static_cast<uint32_t>(drop_reason_mask));
      status |=
          it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, drop_priority);
      if (enable && drop_report) {
        it->second.init_action_data(smi_id::A_DTEL_CONFIG_MIRROR_DROP);
      } else {
        it->second.init_action_data(smi_id::A_NO_ACTION);
      }

      drop_priority++;
      it = pipe_it->second.insert(it,
                                  std::pair<_MatchKey, _ActionEntry>(
                                      _MatchKey(smi_id::T_DTEL_CONFIG),
                                      _ActionEntry(smi_id::T_DTEL_CONFIG)));

      // egress drop_reason values with qalert true
      status |= it->first.set_ternary(
          smi_id::F_DTEL_CONFIG_PKT_SRC, SWITCH_PKT_SRC_BRIDGED, pkt_src_mask);
      status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_MIRROR_TYPE,
                                      SWITCH_MIRROR_TYPE_INVALID,
                                      static_cast<uint8_t>(0xFF));
      status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_DROP_REASON,
                                      drop_reason,
                                      static_cast<uint32_t>(drop_reason_mask));
      status |= it->first.set_ternary(smi_id::F_DTEL_CONFIG_QUEUE_REPORT_FLAG,
                                      static_cast<uint8_t>(1),
                                      static_cast<uint8_t>(1));

      status |=
          it->first.set_exact(smi_id::F_DTEL_CONFIG_PRIORITY, drop_priority);

      if (queue_report) {
        it->second.init_action_data(
            smi_id::A_DTEL_CONFIG_MIRROR_SWITCH_LOCAL_SET_Q);
      } else {
        it->second.init_action_data(smi_id::A_NO_ACTION);
      }
    }
  }
  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    bool bf_rt_status = false;
    bool add = (get_auto_oid() == 0 ||
                switch_store::smiContext::context().in_warm_init());

    for (auto const &pipe_entry : pipe_entries) {
      bf_rt_target_t dev_pipe_tgt = {.dev_id = 0, .pipe_id = pipe_entry.first};
      _Table mt(dev_pipe_tgt, get_bf_rt_info(), smi_id::T_DTEL_CONFIG);
      for (auto const &entry : pipe_entry.second) {
        if (add) {
          status = mt.entry_add(entry.first, entry.second, bf_rt_status);
          if (status != SWITCH_STATUS_SUCCESS) {
            switch_log(SWITCH_API_LEVEL_ERROR,
                       SWITCH_OBJECT_TYPE_DTEL,
                       "{}.{}:{}: status:{} failed entry_add",
                       "dtel_config",
                       __func__,
                       __LINE__,
                       status);
            return status;
          }
        } else {
          status = mt.entry_modify(entry.first, entry.second);
          if (status != SWITCH_STATUS_SUCCESS) {
            switch_log(SWITCH_API_LEVEL_ERROR,
                       SWITCH_OBJECT_TYPE_DTEL,
                       "{}.{}:{}: status:{} failed entry_modify",
                       "dtel_config",
                       __func__,
                       __LINE__,
                       status);
            return status;
          }
        }
      }
    }

    status = auto_object::create_update();
    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    for (auto const &pipe_entry : pipe_entries) {
      bf_rt_target_t dev_pipe_tgt = {.dev_id = 0, .pipe_id = pipe_entry.first};
      for (auto const &entry : pipe_entry.second) {
        _Table mt(dev_pipe_tgt, get_bf_rt_info(), smi_id::T_DTEL_CONFIG);
        status = mt.entry_delete(entry.first);
        if (status != SWITCH_STATUS_SUCCESS) {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_DTEL,
                     "{}.{}:{}: status:{} failed entry_delete",
                     "dtel_config",
                     __func__,
                     __LINE__,
                     status);
          return status;
        }
      }
    }
    status = auto_object::del();
    return status;
  }
};

switch_status_t before_dtel_update(const switch_object_id_t handle,
                                   const attr_w &attr) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::unique_ptr<object> mobject;
  auto id = attr.id_get();

  if (id == SWITCH_DTEL_ATTR_TAIL_DROP_REPORT ||
      id == SWITCH_DTEL_ATTR_QUEUE_REPORT) {
    mobject =
        std::unique_ptr<deflect_on_drop>(new deflect_on_drop(handle, status));
    if (mobject != NULL) {
      mobject->del();
    }
  }
  if (status != SWITCH_STATUS_SUCCESS) return status;

  if (id == SWITCH_DTEL_ATTR_IFA_REPORT || id == SWITCH_DTEL_ATTR_IFA_DSCP ||
      id == SWITCH_DTEL_ATTR_IFA_DSCP_MASK) {
    mobject =
        std::unique_ptr<dtel_config_ifa>(new dtel_config_ifa(handle, status));
    if (mobject != NULL) {
      mobject->del();
    }
  }

  return status;
}

switch_status_t after_dtel_update(const switch_object_id_t handle,
                                  const attr_w &attr) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::unique_ptr<object> mobject;

  auto id = attr.id_get();

  if (id == SWITCH_DTEL_ATTR_TAIL_DROP_REPORT ||
      id == SWITCH_DTEL_ATTR_QUEUE_REPORT) {
    mobject =
        std::unique_ptr<deflect_on_drop>(new deflect_on_drop(handle, status));
    if (mobject != NULL) {
      mobject->create_update();
    }
  }

  if (id == SWITCH_DTEL_ATTR_IFA_REPORT || id == SWITCH_DTEL_ATTR_IFA_DSCP ||
      id == SWITCH_DTEL_ATTR_IFA_DSCP_MASK) {
    mobject =
        std::unique_ptr<dtel_config_ifa>(new dtel_config_ifa(handle, status));
    if (mobject != NULL) {
      mobject->create_update();
    }
  }

  return status;
}

switch_status_t dtel_init() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  REGISTER_OBJECT(dtel_config, SWITCH_OBJECT_TYPE_DTEL_CONFIG);
  REGISTER_OBJECT(dtel_config_ifa, SWITCH_OBJECT_TYPE_DTEL_CONFIG_IFA);
  REGISTER_OBJECT(default_session_selector,
                  SWITCH_OBJECT_TYPE_DEFAULT_SESSION_SELECTOR);
  REGISTER_OBJECT(mod_config, SWITCH_OBJECT_TYPE_MOD_CONFIG);
  REGISTER_OBJECT(mod_config_drop_reason,
                  SWITCH_OBJECT_TYPE_MOD_CONFIG_DROP_REASON);
  REGISTER_OBJECT(dtel_config_drop_reason,
                  SWITCH_OBJECT_TYPE_DTEL_CONFIG_DROP_REASON);
  REGISTER_OBJECT(session_selector_group,
                  SWITCH_OBJECT_TYPE_SESSION_SELECTOR_GROUP);
  REGISTER_OBJECT(session_selector, SWITCH_OBJECT_TYPE_SESSION_SELECTOR);
  REGISTER_OBJECT(dtel_mirror_session_table,
                  SWITCH_OBJECT_TYPE_DTEL_MIRROR_SESSION_TABLE);
  REGISTER_OBJECT(recirc_rif, SWITCH_OBJECT_TYPE_RECIRC_RIF);
  REGISTER_OBJECT(queue_alert, SWITCH_OBJECT_TYPE_QUEUE_ALERT);
  REGISTER_OBJECT(queue_report_thresholds, SWITCH_OBJECT_TYPE_QR_THRESHOLDS);
  REGISTER_OBJECT(check_quota, SWITCH_OBJECT_TYPE_CHECK_QUOTA);
  REGISTER_OBJECT(deflect_on_drop, SWITCH_OBJECT_TYPE_DEFLECT_ON_DROP);
  REGISTER_OBJECT(deflect_on_drop_queue,
                  SWITCH_OBJECT_TYPE_DEFLECT_ON_DROP_QUEUE);
  REGISTER_OBJECT(tm_dod_config, SWITCH_OBJECT_TYPE_TM_DOD_CONFIG);
  REGISTER_OBJECT(ingress_port_conversion,
                  SWITCH_OBJECT_TYPE_INGRESS_PORT_CONVERSION);
  REGISTER_OBJECT(egress_port_conversion,
                  SWITCH_OBJECT_TYPE_EGRESS_PORT_CONVERSION);
  REGISTER_OBJECT(int_edge, SWITCH_OBJECT_TYPE_INT_EDGE);
  status |= switch_store::reg_update_trigs_before(SWITCH_OBJECT_TYPE_DTEL,
                                                  &before_dtel_update);
  status |= switch_store::reg_update_trigs_after(SWITCH_OBJECT_TYPE_DTEL,
                                                 &after_dtel_update);

  return status;
}

switch_status_t dtel_clean() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  return status;
}

}  // namespace smi
