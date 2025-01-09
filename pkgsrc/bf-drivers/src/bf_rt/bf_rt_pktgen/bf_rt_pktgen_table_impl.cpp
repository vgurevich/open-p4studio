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


#include <bf_rt_common/bf_rt_init_impl.hpp>

#include "bf_rt_pktgen_table_impl.hpp"
#include "bf_rt_pktgen_table_data_impl.hpp"
#include "bf_rt_pktgen_table_key_impl.hpp"
#include <bf_rt_common/bf_rt_pipe_mgr_intf.hpp>

namespace bfrt {
namespace {
// Intergers in enum here should be keep strictly the same with the field_ids in
// bf_rt_pktgen.json file.
enum PktgenPortDataFieldId {
  RECIR_EN = 1,
  PKTGEN_EN = 2,
  PATTEN_MAT_EN = 3,
  CLEAR_PORT_DOWN_EN = 4
};

// pktgen pkt buffer table only has one data field, field id is 1
constexpr int PKTGEN_BUFFER_DATA_FIELD_ID = 1;
constexpr int PORT_DOWN_MASK_SZ = 9;

static std::string portDataFieldIdName(bf_rt_id_t id) {
  switch (id) {
    case RECIR_EN:
      return "Recir Enable";
    case PKTGEN_EN:
      return "Pktgen Enable";
    case PATTEN_MAT_EN:
      return "Recir Pattern Matching Enable";
    case CLEAR_PORT_DOWN_EN:
      return "Port Down Event Cleared State";
    default:
      return "Unknown";
  }
}

static const std::set<bf_rt_id_t> getAllPktgenPortDataFieldId() {
  return {RECIR_EN, PKTGEN_EN, PATTEN_MAT_EN, CLEAR_PORT_DOWN_EN};
}

}  // anonymous namespace

bf_status_t BfRtPktgenPortTable::tableEntryMod(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    const BfRtTableData &data) const {
  const BfRtPktgenPortTableKey &pktgen_key =
      static_cast<const BfRtPktgenPortTableKey &>(key);
  const BfRtPktgenPortTableData &pktgen_data =
      static_cast<const BfRtPktgenPortTableData &>(data);
  const uint32_t &dev_port = pktgen_key.getId();
  const std::map<bf_rt_id_t, bool> &boolField =
      pktgen_data.getBoolFieldDataMap();
  bf_status_t status = BF_SUCCESS;
  for (const auto &id : getAllPktgenPortDataFieldId()) {
    if (boolField.find(id) != boolField.end()) {
      std::unique_ptr<PgenDataId> dataId(PgenDataIdFactory::create(id));
      status = dataId->set(session,
                           dev_tgt.dev_id,
                           static_cast<bf_dev_port_t>(dev_port),
                           boolField.at(id));
      if (status != BF_SUCCESS) {
        LOG_TRACE("%s:%d %s ERROR in setting %s %s, err %d",
                  __func__,
                  __LINE__,
                  portDataFieldIdName(dataId->id()).c_str(),
                  boolField.at(id) ? "enable" : "disable",
                  table_name_get().c_str(),
                  status);
        return status;
      }
    }
  }

  return BF_SUCCESS;
}

bf_status_t BfRtPktgenPortTable::tableEntryGet(const BfRtSession &session,
                                               const bf_rt_target_t &dev_tgt,
                                               const uint64_t &flags,
                                               const BfRtTableKey &key,
                                               BfRtTableData *data) const {
  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    LOG_TRACE("%s:%d %s ERROR : Read from hardware not supported",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }
  bf_status_t status = BF_SUCCESS;
  const BfRtPktgenPortTableKey &pktgen_key =
      static_cast<const BfRtPktgenPortTableKey &>(key);
  const uint32_t &dev_port = pktgen_key.getId();
  BfRtPktgenPortTableData *pktgen_data =
      static_cast<BfRtPktgenPortTableData *>(data);
  bool is_all = pktgen_data->getActiveFields().empty();
  const std::set<bf_rt_id_t> &activeDataFields =
      (is_all) ? getAllPktgenPortDataFieldId() : pktgen_data->getActiveFields();
  std::vector<std::unique_ptr<PgenDataId>> dataIdList;
  for (const auto &id : activeDataFields) {
    std::unique_ptr<PgenDataId> dataId(PgenDataIdFactory::create(id));
    if (dataId) dataIdList.push_back(std::move(dataId));
  }

  bool ret = false;
  for (const auto &i : dataIdList) {
    if (i) {
      status = i->get(
          session, dev_tgt.dev_id, static_cast<bf_dev_port_t>(dev_port), &ret);
      if (BF_SUCCESS != status) {
        LOG_TRACE("%s:%d %s: Error in getting %s of dev_port %d",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  portDataFieldIdName(i->id()).c_str(),
                  dev_port);
        return status;
      }
      status = pktgen_data->setValue(i->id(), ret);
      if (BF_SUCCESS != status) {
        LOG_TRACE("%s:%d %s: Error in setting %s of dev_port %d",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  portDataFieldIdName(i->id()).c_str(),
                  dev_port);
        return status;
      }
    }
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPktgenPortTable::tableClear(const BfRtSession &session,
                                            const bf_rt_target_t &dev_tgt,
                                            const uint64_t & /*flags*/) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  bf_dev_port_t port = pipeMgr->pipeMgrPktgenPortGet(dev_tgt.dev_id);
  while (status != BF_OBJECT_NOT_FOUND) {
    for (const auto &id : getAllPktgenPortDataFieldId()) {
      std::unique_ptr<PgenDataId> dataId(PgenDataIdFactory::create(id));
      status = dataId->clear(session, dev_tgt.dev_id, port);
      if (status != BF_SUCCESS) {
        LOG_TRACE("%s:%d %s Error in Clearing %s of dev_port %d",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  portDataFieldIdName(id).c_str(),
                  port);
        return status;
      }
    }
    status = pipeMgr->pipeMgrPktgenPortGetNext(dev_tgt.dev_id, &port);
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPktgenPortTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtPktgenPortTableKey(this));
  return (*key_ret == nullptr) ? BF_NO_SYS_RESOURCES : BF_SUCCESS;
}

bf_status_t BfRtPktgenPortTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  std::vector<bf_rt_id_t> fields;
  *data_ret =
      std::unique_ptr<BfRtTableData>(new BfRtPktgenPortTableData(this, fields));
  return (*data_ret == nullptr) ? BF_NO_SYS_RESOURCES : BF_SUCCESS;
}

bf_status_t BfRtPktgenPortTable::dataAllocate(
    const std::vector<bf_rt_id_t> &fields,
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret =
      std::unique_ptr<BfRtTableData>(new BfRtPktgenPortTableData(this, fields));
  return (*data_ret == nullptr) ? BF_NO_SYS_RESOURCES : BF_SUCCESS;
}

// pktgen app cfg table
namespace {
// Intergers in enum here should be keep strictly the same with the field_ids in
// bf_rt_pktgen.json file.
enum PktgenAppDataFieldId {
  TIMER_NS = 1,
  PORT_MASK_SEL = 3,
  PATTERN_VALUE = 4,
  PATTERN_MASK = 5,
  PFC_HDR = 8,
  TIMER_ENABLE = 9,
  TIMER = 10,
  MAX_PFC_EVENTS = 11,
  APP_ENB = 12,
  PKT_LEN = 13,
  PKT_BUF_OFFSET = 14,
  SRC_PORT = 15,
  SRC_PORT_INC = 16,
  BATCH_CNT_CFG = 17,
  PKT_PER_BATCH = 18,
  IBG = 19,
  IBG_JITTER = 20,
  IPG = 21,
  IPG_JITTER = 22,
  BATCH_CNT = 23,
  PKT_CNT = 24,
  TRIG_CNT = 25,
  OFFSET_LEN_FLAG = 26,
  SRC_PORT_MAX = 27,
  CHNL_ID = 28,
  ID_MAX_INVALID = 29
};
// Intergers in enum here should be keep strictly the same with the action_ids
// in bf_rt_pktgen.json file.
enum PktgenAppActionId {
  TIMER_ONE_SHOT = 1,
  TIMER_PERIODIC = 2,
  PORT_DOWN = 3,
  RECIRC_PATTERN = 4,
  DPRSR = 5,
  PFC = 6
};

PktgenAppActionId triggerToActionId(const bf_pktgen_trigger_type &t) {
  switch (t) {
    case (BF_PKTGEN_TRIGGER_TIMER_ONE_SHOT):
      return TIMER_ONE_SHOT;
    case BF_PKTGEN_TRIGGER_TIMER_PERIODIC:
      return TIMER_PERIODIC;
    case BF_PKTGEN_TRIGGER_PORT_DOWN:
      return PORT_DOWN;
    case BF_PKTGEN_TRIGGER_RECIRC_PATTERN:
      return RECIRC_PATTERN;
    case BF_PKTGEN_TRIGGER_DPRSR:
      return DPRSR;
    case BF_PKTGEN_TRIGGER_PFC:
      return PFC;
    default:
      LOG_TRACE("%s:%d : Invalid trigger type %d", __func__, __LINE__, t);
      return static_cast<PktgenAppActionId>(0);
  }
}
}  // anonymous namespace

bf_status_t BfRtPktgenAppTable::tableEntryMod(const BfRtSession &session,
                                              const bf_rt_target_t &dev_tgt,
                                              const uint64_t & /*flags*/,
                                              const BfRtTableKey &key,
                                              const BfRtTableData &data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtPktgenAppTableKey &pktgen_key =
      static_cast<const BfRtPktgenAppTableKey &>(key);
  const BfRtPktgenAppTableData &pktgen_data =
      static_cast<const BfRtPktgenAppTableData &>(data);
  const int &app_id = static_cast<const int &>(pktgen_key.getId());
  const std::map<bf_rt_id_t, bool> &boolField =
      pktgen_data.getBoolFieldDataMap();
  const std::map<bf_rt_id_t, uint64_t> &u64Field =
      pktgen_data.getU64FieldDataMap();
  const std::map<bf_rt_id_t, std::array<uint8_t, 16>> &arrayField =
      pktgen_data.getArrayFieldDataMap();
  bf_rt_id_t act_id;
  pktgen_data.actionIdGet(&act_id);
  bf_status_t status;
  bf_pktgen_app_cfg_t cfg;
  memset(&cfg, 0, sizeof cfg);
  dev_target_t pipe_dev_tgt = {dev_tgt.dev_id, dev_tgt.pipe_id};
  // Only when cfg_app is true, we would call the pktgen application
  // configuration pipe_mgr function.
  bool cfg_app = false;
  if (act_id != 0) {
    // check required args are provided
    switch (act_id) {
      case TIMER_ONE_SHOT:
      case TIMER_PERIODIC: {
        if (u64Field.find(TIMER_NS) != u64Field.end()) {
          cfg.u.timer_nanosec =
              static_cast<uint32_t>(u64Field.find(TIMER_NS)->second);
          cfg_app = true;
        }
        break;
      }
      case PORT_DOWN: {
        if (u64Field.find(PORT_MASK_SEL) != u64Field.end()) {
          cfg.u.port_mask_sel_tof2 =
              static_cast<uint32_t>(u64Field.find(PORT_MASK_SEL)->second);
          cfg_app = true;
        }
        break;
      }
      case RECIRC_PATTERN:
      case DPRSR: {
        bf_dev_family_t dev_family =
            bf_dev_type_to_family(bf_drv_get_dev_type(dev_tgt.dev_id));
        cfg_app = false;
        if (dev_family == BF_DEV_FAMILY_TOFINO) {
          auto msk = u64Field.find(PATTERN_MASK);
          auto val = u64Field.find(PATTERN_VALUE);
          if ((val != u64Field.end()) && (msk != u64Field.end())) {
            cfg.u.pattern.value = val->second;
            cfg.u.pattern.mask = msk->second;
            cfg_app = true;
          }
        } else {  // BF_DEV_FAMILY_TOFINO2+
          auto msk = arrayField.find(PATTERN_MASK);
          auto val = arrayField.find(PATTERN_VALUE);
          if ((val != arrayField.end()) && (msk != arrayField.end())) {
            std::memcpy(
                cfg.u.pattern_tof2.value, &val->second, pattern_size_tof2 / 8);
            std::memcpy(
                cfg.u.pattern_tof2.mask, &msk->second, pattern_size_tof2 / 8);
            cfg_app = true;
          }
        }
        if (!cfg_app) {
          LOG_ERROR(
              "ERROR: %s:%d %s: Configuring Pattern requires both "
              "pattern_value and pattern_mask",
              __func__,
              __LINE__,
              table_name_get().c_str());
        }
        break;
      }
      case PFC: {
        if ((u64Field.find(TIMER) != u64Field.end()) &&
            (u64Field.find(MAX_PFC_EVENTS) != u64Field.end()) &&
            (boolField.find(TIMER_ENABLE) != boolField.end()) &&
            (arrayField.find(PFC_HDR) != arrayField.end())) {
          cfg.u.pfc_cfg.cfg_timer_en = (boolField.find(TIMER_ENABLE)->second);
          cfg.u.pfc_cfg.cfg_timer =
              static_cast<uint16_t>(u64Field.find(TIMER)->second);
          cfg.u.pfc_cfg.pfc_max_msgs =
              static_cast<uint16_t>(u64Field.find(MAX_PFC_EVENTS)->second);
          std::memcpy(
              cfg.u.pfc_cfg.pfc_hdr, &(arrayField.find(PFC_HDR)->second), 16);
          cfg_app = true;
        } else {
          LOG_ERROR(
              "ERROR: %s:%d %s: Configuring PFC trigger pktgen requires timer, "
              "max_pfc_events, timer_enable, pfc_hdr",
              __func__,
              __LINE__,
              table_name_get().c_str());
        }
        break;
      }
      default:
        LOG_TRACE("ERROR: %s:%d %s: Invalid pktgen trigger type %d",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  (act_id - 1));
        return BF_INVALID_ARG;
    }
    // action_id starts from 1, while bf_pktgen_trigger_type_e start from 0
    cfg.trigger_type = static_cast<bf_pktgen_trigger_type_e>(act_id - 1);
    if (u64Field.find(BATCH_CNT_CFG) != u64Field.end()) {
      cfg.batch_count =
          static_cast<uint16_t>(u64Field.find(BATCH_CNT_CFG)->second);
    }
    if (u64Field.find(PKT_PER_BATCH) != u64Field.end()) {
      cfg.packets_per_batch =
          static_cast<uint16_t>(u64Field.find(PKT_PER_BATCH)->second);
    }
    if (u64Field.find(IBG) != u64Field.end()) {
      cfg.ibg = static_cast<uint32_t>(u64Field.find(IBG)->second);
      cfg_app = true;
    }
    if (u64Field.find(IBG_JITTER) != u64Field.end()) {
      cfg.ibg_jitter = static_cast<uint32_t>(u64Field.find(IBG_JITTER)->second);
      cfg_app = true;
    }
    if (u64Field.find(IPG) != u64Field.end()) {
      cfg.ipg = static_cast<uint32_t>(u64Field.find(IPG)->second);
      cfg_app = true;
    }
    if (u64Field.find(IPG_JITTER) != u64Field.end()) {
      cfg.ipg_jitter = static_cast<uint32_t>(u64Field.find(IPG_JITTER)->second);
      cfg_app = true;
    }
    if (u64Field.find(SRC_PORT) != u64Field.end()) {
      cfg.pipe_local_source_port =
          static_cast<bf_dev_port_t>(u64Field.find(SRC_PORT)->second);
      cfg_app = true;
    }
    if (boolField.find(SRC_PORT_INC) != boolField.end()) {
      cfg.increment_source_port = boolField.find(SRC_PORT_INC)->second;
      cfg_app = true;
    }
    if (u64Field.find(PKT_BUF_OFFSET) != u64Field.end()) {
      cfg.pkt_buffer_offset =
          static_cast<uint16_t>(u64Field.find(PKT_BUF_OFFSET)->second);
      cfg_app = true;
    }
    if (u64Field.find(PKT_LEN) != u64Field.end()) {
      cfg.length = static_cast<uint16_t>(u64Field.find(PKT_LEN)->second);
      cfg_app = true;
    }
    if (boolField.find(OFFSET_LEN_FLAG) != boolField.end()) {
      cfg.tof2.offset_len_from_recir_pkt =
          boolField.find(OFFSET_LEN_FLAG)->second;
      cfg_app = true;
    }
    if (u64Field.find(SRC_PORT_MAX) != u64Field.end()) {
      cfg.tof2.source_port_wrap_max =
          static_cast<uint8_t>(u64Field.find(SRC_PORT_MAX)->second);
      cfg_app = true;
    }
    if (u64Field.find(CHNL_ID) != u64Field.end()) {
      cfg.tof2.assigned_chnl_id =
          static_cast<uint8_t>(u64Field.find(CHNL_ID)->second);
      cfg_app = true;
    }
    if (cfg_app) {
      status = pipeMgr->pipeMgrPktgenAppSet(
          session.sessHandleGet(), pipe_dev_tgt, app_id, &cfg);
      if (status != BF_SUCCESS) {
        LOG_TRACE(
            "%s:%d %s: Error in setting Pktgen Application with app_id %d",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            app_id);
        return status;
      }
    }
  }
  // not configure app itself
  if (u64Field.find(BATCH_CNT) != u64Field.end()) {
    status =
        pipeMgr->pipeMgrPktgenBatchCntSet(session.sessHandleGet(),
                                          pipe_dev_tgt,
                                          app_id,
                                          (u64Field.find(BATCH_CNT)->second));
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s: Error in setting Pktgen Batch counter of app_id %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                app_id);
      return status;
    }
  }
  if (u64Field.find(PKT_CNT) != u64Field.end()) {
    status = pipeMgr->pipeMgrPktgenPktCntSet(session.sessHandleGet(),
                                             pipe_dev_tgt,
                                             app_id,
                                             (u64Field.find(PKT_CNT)->second));
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s: Error in setting Pktgen Packet counter of app_id %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                app_id);
      return status;
    }
  }
  if (u64Field.find(TRIG_CNT) != u64Field.end()) {
    status =
        pipeMgr->pipeMgrPktgenTriggerCntSet(session.sessHandleGet(),
                                            pipe_dev_tgt,
                                            app_id,
                                            (u64Field.find(TRIG_CNT)->second));
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s: Error in setting Pktgen Trigger counter of app_id %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          app_id);
      return status;
    }
  }
  if (boolField.find(APP_ENB) != boolField.end()) {
    status =
        pipeMgr->pipeMgrPktgenAppEnableSet(session.sessHandleGet(),
                                           pipe_dev_tgt,
                                           app_id,
                                           (boolField.find(APP_ENB)->second));
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s: Error in %s Pktgen application of app_id %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                (boolField.find(APP_ENB)->second) ? "enable" : "disable",
                app_id);
      return status;
    }
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPktgenAppTable::tableClear(const BfRtSession &session,
                                           const bf_rt_target_t &dev_tgt,
                                           const uint64_t & /*flags*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  bf_status_t status;
  bf_pktgen_app_cfg_t cfg = {};
  cfg.trigger_type =
      static_cast<bf_pktgen_trigger_type_e>(BF_PKTGEN_TRIGGER_TIMER_ONE_SHOT);
  cfg.length = 60;
  dev_target_t pipe_dev_tgt = {dev_tgt.dev_id, dev_tgt.pipe_id};

  const int app_count = pipeMgr->pipeMgrPktgenAppCountGet(dev_tgt.dev_id);
  for (int app_id = 0; app_id < app_count; ++app_id) {
    status = pipeMgr->pipeMgrPktgenAppSet(
        session.sessHandleGet(), pipe_dev_tgt, app_id, &cfg);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s: Error in setting Pktgen Application with app_id %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                app_id);
      return status;
    }

    status = pipeMgr->pipeMgrPktgenBatchCntSet(
        session.sessHandleGet(), pipe_dev_tgt, app_id, 0);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s: Error in clearing Pktgen Batch counter of app_id %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                app_id);
      return status;
    }
    status = pipeMgr->pipeMgrPktgenPktCntSet(
        session.sessHandleGet(), pipe_dev_tgt, app_id, 0);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s: Error in clearing Pktgen Packet counter of app_id %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          app_id);
      return status;
    }
    status = pipeMgr->pipeMgrPktgenTriggerCntSet(
        session.sessHandleGet(), pipe_dev_tgt, app_id, 0);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s: Error in clearing Pktgen Trigger counter of app_id %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          app_id);
      return status;
    }
    status = pipeMgr->pipeMgrPktgenAppEnableSet(
        session.sessHandleGet(), pipe_dev_tgt, app_id, false);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s: Error in disable Pktgen application of app_id %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                app_id);
      return status;
    }
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPktgenAppTable::tableEntryGet_internal(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const uint32_t &app_id,
    BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  BfRtPktgenAppTableData *pktgen_data =
      static_cast<BfRtPktgenAppTableData *>(data);
  std::vector<bf_rt_id_t> activeDataFields;
  bf_pktgen_app_cfg_t cfg = {};
  dev_target_t pipe_dev_tgt = {dev_tgt.dev_id, dev_tgt.pipe_id};

  // get configuration from pipe mgr
  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    LOG_WARN(
        "%s:%d %s ERROR : Read pktgen application configuration from "
        "hardware not supported",
        __func__,
        __LINE__,
        table_name_get().c_str());
  }
  status = pipeMgr->pipeMgrPktgenAppGet(
      session.sessHandleGet(), pipe_dev_tgt, app_id, &cfg);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s: Error in getting Pktgen Application Config of app_id "
        "%d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        app_id);
    return status;
  }
  // set action and action data
  PktgenAppActionId action = triggerToActionId(cfg.trigger_type);
  pktgen_data->actionIdSet(action);
  status = pktgen_data->setActiveFields(std::vector<bf_rt_id_t>());
  if (status) return status;
  status = this->dataFieldIdListGet(action, &activeDataFields);
  if (status) return status;

  if (pktgen_data->allFieldsSet()) {
    uint64_t cnt;
    for (const auto &id : activeDataFields) {
      switch (id) {
        case (BATCH_CNT):
          if (!BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
            LOG_TRACE(
                "%s:%d %s ERROR : Read batch counter from software not "
                "supported",
                __func__,
                __LINE__,
                table_name_get().c_str());
          }
          status = pipeMgr->pipeMgrPktgenBatchCntGet(
              session.sessHandleGet(), pipe_dev_tgt, app_id, &cnt);
          if (status != BF_SUCCESS) {
            LOG_TRACE(
                "%s:%d %s: Error in getting Pktgen Batch Counter of app_id %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                app_id);
            return status;
          }
          status = pktgen_data->setValue(BATCH_CNT, cnt);
          break;
        case (PKT_CNT):
          if (!BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
            LOG_TRACE(
                "%s:%d %s ERROR : Read packet counter from software not "
                "supported",
                __func__,
                __LINE__,
                table_name_get().c_str());
          }
          status = pipeMgr->pipeMgrPktgenPktCntGet(
              session.sessHandleGet(), pipe_dev_tgt, app_id, &cnt);
          if (status != BF_SUCCESS) {
            LOG_TRACE(
                "%s:%d %s: Error in getting Pktgen Pkt Counter of app_id %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                app_id);
            return status;
          }
          status = pktgen_data->setValue(PKT_CNT, cnt);
          break;
        case (TRIG_CNT):
          if (!BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
            LOG_TRACE(
                "%s:%d %s ERROR : Read trigger counter from software not "
                "supported",
                __func__,
                __LINE__,
                table_name_get().c_str());
          }
          status = pipeMgr->pipeMgrPktgenTriggerCntGet(
              session.sessHandleGet(), pipe_dev_tgt, app_id, &cnt);
          if (status != BF_SUCCESS) {
            LOG_TRACE(
                "%s:%d %s: Error in getting Pktgen Trigger Counter of app_id "
                "%d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                app_id);
            return status;
          }
          status = pktgen_data->setValue(TRIG_CNT, cnt);
          break;
        case (APP_ENB):
          bool enable;
          if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
            LOG_WARN(
                "%s:%d %s ERROR : Read pktgen application enable from hardware "
                "not supported",
                __func__,
                __LINE__,
                table_name_get().c_str());
          }
          status = pipeMgr->pipeMgrPktgenAppEnableGet(
              session.sessHandleGet(), pipe_dev_tgt, app_id, &enable);
          if (status != BF_SUCCESS) {
            LOG_TRACE(
                "%s:%d %s: Error in getting Pktgen Enable of app_id "
                "%d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                app_id);
            return status;
          }
          status = pktgen_data->setValue(APP_ENB, enable);
          break;
        case TIMER_NS:
          status = pktgen_data->setValue(
              TIMER_NS, static_cast<uint64_t>(cfg.u.timer_nanosec));
          break;
        case PORT_MASK_SEL:
          status = pktgen_data->setValue(
              PORT_MASK_SEL, static_cast<uint64_t>(cfg.u.port_mask_sel_tof2));
          break;
        case PATTERN_VALUE:
        case PATTERN_MASK:
          size_t field_size;
          status = dataFieldSizeGet(PATTERN_VALUE, RECIRC_PATTERN, &field_size);
          if (BF_SUCCESS != status) {
            LOG_TRACE(
                "%s:%d %s: Error in getting Pattern Value field size of app_id "
                "%d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                app_id);
            return status;
          }
          if (field_size == pattern_size_tof) {
            status = pktgen_data->setValue(
                PATTERN_VALUE, static_cast<uint64_t>(cfg.u.pattern.value));
          } else if (field_size == pattern_size_tof2) {
            status = pktgen_data->setValue(
                PATTERN_VALUE, cfg.u.pattern_tof2.value, 16);
          }
          if (status) return status;
          status = dataFieldSizeGet(PATTERN_MASK, RECIRC_PATTERN, &field_size);
          if (BF_SUCCESS != status) {
            LOG_TRACE(
                "%s:%d %s: Error in getting Pattern Mask field size of app_id "
                "%d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                app_id);
            return status;
          }
          if (field_size == pattern_size_tof) {
            status = pktgen_data->setValue(
                PATTERN_MASK, static_cast<uint64_t>(cfg.u.pattern.mask));
          } else if (field_size == pattern_size_tof2) {
            status = pktgen_data->setValue(
                PATTERN_MASK, cfg.u.pattern_tof2.mask, 16);
          }
          break;
        case PFC_HDR:
          status = pktgen_data->setValue(PFC_HDR, cfg.u.pfc_cfg.pfc_hdr, 16);
          break;
        case TIMER_ENABLE:
          status =
              pktgen_data->setValue(TIMER_ENABLE, cfg.u.pfc_cfg.cfg_timer_en);
          break;
        case TIMER:
          status = pktgen_data->setValue(
              TIMER, static_cast<uint64_t>(cfg.u.pfc_cfg.cfg_timer));
          break;
        case MAX_PFC_EVENTS:
          status = pktgen_data->setValue(
              MAX_PFC_EVENTS,
              static_cast<uint64_t>(cfg.u.pfc_cfg.pfc_max_msgs));
          break;
        // set common data
        case PKT_LEN:
          status =
              pktgen_data->setValue(PKT_LEN, static_cast<uint64_t>(cfg.length));
          break;
        case PKT_BUF_OFFSET:
          status = pktgen_data->setValue(
              PKT_BUF_OFFSET, static_cast<uint64_t>(cfg.pkt_buffer_offset));
          break;
        case SRC_PORT:
          status = pktgen_data->setValue(
              SRC_PORT, static_cast<uint64_t>(cfg.pipe_local_source_port));
          break;
        case SRC_PORT_INC:
          status =
              pktgen_data->setValue(SRC_PORT_INC, cfg.increment_source_port);
          break;
        case BATCH_CNT_CFG:
          status = pktgen_data->setValue(
              BATCH_CNT_CFG, static_cast<uint64_t>(cfg.batch_count));
          break;
        case PKT_PER_BATCH:
          status = pktgen_data->setValue(
              PKT_PER_BATCH, static_cast<uint64_t>(cfg.packets_per_batch));
          break;
        case IBG:
          status = pktgen_data->setValue(IBG, static_cast<uint64_t>(cfg.ibg));
          break;
        case IBG_JITTER:
          status = pktgen_data->setValue(IBG_JITTER,
                                         static_cast<uint64_t>(cfg.ibg_jitter));
          break;
        case IPG:
          status = pktgen_data->setValue(IPG, static_cast<uint64_t>(cfg.ipg));
          break;
        case IPG_JITTER:
          status = pktgen_data->setValue(IPG_JITTER,
                                         static_cast<uint64_t>(cfg.ipg_jitter));
          break;
        case OFFSET_LEN_FLAG:
          status = pktgen_data->setValue(OFFSET_LEN_FLAG,
                                         cfg.tof2.offset_len_from_recir_pkt);
          break;
        case SRC_PORT_MAX:
          status = pktgen_data->setValue(
              SRC_PORT_MAX,
              static_cast<uint64_t>(cfg.tof2.source_port_wrap_max));
          break;
        case CHNL_ID:
          status = pktgen_data->setValue(
              CHNL_ID, static_cast<uint64_t>(cfg.tof2.assigned_chnl_id));
          break;
        default:
          LOG_TRACE("%s:%d %s: Invalid field id %d ",
                    __func__,
                    __LINE__,
                    table_name_get().c_str(),
                    id);
          return BF_INVALID_ARG;
      }
      if (status) return status;
    }
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPktgenAppTable::tableEntryGet(const BfRtSession &session,
                                              const bf_rt_target_t &dev_tgt,
                                              const uint64_t &flags,
                                              const BfRtTableKey &key,
                                              BfRtTableData *data) const {
  const BfRtPktgenAppTableKey &pktgen_key =
      static_cast<const BfRtPktgenAppTableKey &>(key);
  const uint32_t &app_id = pktgen_key.getId();
  return tableEntryGet_internal(session, dev_tgt, flags, app_id, data);
}

bf_status_t BfRtPktgenAppTable::tableEntryGetFirst(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    BfRtTableKey *key,
    BfRtTableData *data) const {
  BfRtPktgenAppTableKey &pktgen_key =
      *static_cast<BfRtPktgenAppTableKey *>(key);
  pktgen_key.setId(0);
  return tableEntryGet_internal(
      session, dev_tgt, flags, pktgen_key.getId(), data);
}

bf_status_t BfRtPktgenAppTable::tableEntryGetNext_n(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    const uint32_t &n,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  bf_status_t status = BF_SUCCESS;
  auto &pktgen_key = static_cast<const BfRtPktgenAppTableKey &>(key);
  uint32_t pktgen_id = pktgen_key.getId();
  uint32_t entry_num = 0;
  uint32_t i = 0;
  status = tableUsageGet(session, dev_tgt, flags, &entry_num);
  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "%s: %d: ERROR: Failed to get pktgen app count", __func__, __LINE__);
    return status;
  }
  if (n < entry_num) {
    entry_num = n;
  }
  for (; i < entry_num - 1; ++i) {
    int next_idx = pktgen_id + i + 1;
    auto &p = (*key_data_pairs)[i];
    status =
        tableEntryGet_internal(session, dev_tgt, flags, next_idx, p.second);
    if (status != BF_SUCCESS) break;
    auto this_key = static_cast<BfRtPktgenAppTableKey *>(p.first);
    this_key->setId(next_idx);
  }
  if (num_returned) *num_returned = i;
  return status;
}

bf_status_t BfRtPktgenAppTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtPktgenAppTableKey(this));
  return (*key_ret == nullptr) ? BF_NO_SYS_RESOURCES : BF_SUCCESS;
}

bf_status_t BfRtPktgenAppTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  std::vector<bf_rt_id_t> fields;
  *data_ret = std::unique_ptr<BfRtTableData>(
      new BfRtPktgenAppTableData(this, 0, fields));
  return (*data_ret == nullptr) ? BF_NO_SYS_RESOURCES : BF_SUCCESS;
}

bf_status_t BfRtPktgenAppTable::dataAllocate(
    const bf_rt_id_t &action_id,
    std::unique_ptr<BfRtTableData> *data_ret) const {
  std::vector<bf_rt_id_t> fields;
  *data_ret = std::unique_ptr<BfRtTableData>(
      new BfRtPktgenAppTableData(this, action_id, fields));
  return (*data_ret == nullptr) ? BF_NO_SYS_RESOURCES : BF_SUCCESS;
}

bf_status_t BfRtPktgenAppTable::dataAllocate(
    const std::vector<bf_rt_id_t> &fields,
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret = std::unique_ptr<BfRtTableData>(
      new BfRtPktgenAppTableData(this, 0, fields));
  return (*data_ret == nullptr) ? BF_NO_SYS_RESOURCES : BF_SUCCESS;
}

bf_status_t BfRtPktgenAppTable::dataAllocate(
    const std::vector<bf_rt_id_t> &fields,
    const bf_rt_id_t &action_id,
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret = std::unique_ptr<BfRtTableData>(
      new BfRtPktgenAppTableData(this, action_id, fields));
  return (*data_ret == nullptr) ? BF_NO_SYS_RESOURCES : BF_SUCCESS;
}

bf_status_t BfRtPktgenAppTable::tableUsageGet(const BfRtSession &session,
                                              const bf_rt_target_t &dev_tgt,
                                              const uint64_t & /*flags*/,
                                              uint32_t *count) const {
  if (!count) return BF_INVALID_ARG;
  *count = PipeMgrIntf::getInstance(session)->pipeMgrPktgenAppCountGet(
      dev_tgt.dev_id);
  return (*count != 0) ? BF_SUCCESS : BF_INVALID_ARG;
}

// pktgen port mask table
bf_status_t BfRtPktgenPortMaskTable::tableEntryMod(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    const BfRtTableData &data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtPktgenPortMaskTableKey &pktgen_key =
      static_cast<const BfRtPktgenPortMaskTableKey &>(key);
  const BfRtPktgenPortMaskTableData &pktgen_data =
      static_cast<const BfRtPktgenPortMaskTableData &>(data);
  const uint32_t &port_mask_sel = pktgen_key.getId();
  const uint8_t *port_down_mask = pktgen_data.getMask();
  struct bf_tof2_port_down_sel mask;
  // port down mask is 72 bits, 9 bytes.
  std::memcpy(mask.port_mask, port_down_mask, PORT_DOWN_MASK_SZ);
  dev_target_t pipe_dev_tgt = {dev_tgt.dev_id, dev_tgt.pipe_id};
  return pipeMgr->pipeMgrPktgenPortDownMaskSet(
      session.sessHandleGet(), pipe_dev_tgt, port_mask_sel, &mask);
}

bf_status_t BfRtPktgenPortMaskTable::tableClear(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const uint32_t port_mask_sel = 0;
  const uint8_t port_down_mask[9] = {0};
  struct bf_tof2_port_down_sel mask;
  // port down mask is 72 bits, 9 bytes.
  std::memcpy(mask.port_mask, &port_down_mask, PORT_DOWN_MASK_SZ);
  dev_target_t pipe_dev_tgt = {dev_tgt.dev_id, dev_tgt.pipe_id};
  return pipeMgr->pipeMgrPktgenPortDownMaskSet(
      session.sessHandleGet(), pipe_dev_tgt, port_mask_sel, &mask);
}

bf_status_t BfRtPktgenPortMaskTable::tableEntryGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    BfRtTableData *data) const {
  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    LOG_TRACE("%s:%d %s ERROR : Read from hardware not supported",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtPktgenPortMaskTableKey &pktgen_key =
      static_cast<const BfRtPktgenPortMaskTableKey &>(key);
  BfRtPktgenPortMaskTableData *pktgen_data =
      static_cast<BfRtPktgenPortMaskTableData *>(data);
  uint32_t sel = pktgen_key.getId();
  dev_target_t pipe_dev_tgt = {dev_tgt.dev_id, dev_tgt.pipe_id};
  struct bf_tof2_port_down_sel mask;
  status = pipeMgr->pipeMgrPktgenPortDownMaskGet(pipe_dev_tgt, sel, &mask);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR in reading pktgen port mask, port_mask_sel %d, err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        sel,
        status);
    return status;
  }
  // port down mask array size is 72 bits, 9 bytes, fixed, defined in chip.
  pktgen_data->setMask(static_cast<uint8_t *>(mask.port_mask),
                       PORT_DOWN_MASK_SZ);
  return BF_SUCCESS;
}

bf_status_t BfRtPktgenPortMaskTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret =
      std::unique_ptr<BfRtTableKey>(new BfRtPktgenPortMaskTableKey(this));
  return (*key_ret == nullptr) ? BF_NO_SYS_RESOURCES : BF_SUCCESS;
}

bf_status_t BfRtPktgenPortMaskTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret =
      std::unique_ptr<BfRtTableData>(new BfRtPktgenPortMaskTableData(this));
  return (*data_ret == nullptr) ? BF_NO_SYS_RESOURCES : BF_SUCCESS;
}

// pktgen pkt buffer table
bf_status_t BfRtPktgenPktBufferTable::tableEntryMod(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    const BfRtTableData &data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtPktgenPktBufferTableKey &pktgen_key =
      static_cast<const BfRtPktgenPktBufferTableKey &>(key);
  const BfRtPktgenPktBufferTableData &pktgen_data =
      static_cast<const BfRtPktgenPktBufferTableData &>(data);
  const uint32_t &byte_buf_offset = pktgen_key.getId();
  const uint32_t &byte_buf_size = pktgen_key.getBuffSize();
  const std::vector<uint8_t> &buf_vec = pktgen_data.getData();
  const uint32_t size = buf_vec.size();
  if (size != byte_buf_size) {
    LOG_TRACE(
        "%s:%d %s ERROR : packet buffer data size %d does not match the data "
        "size %d passed in key",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        size,
        byte_buf_size);
    return BF_INVALID_ARG;
  }
  dev_target_t pipe_dev_tgt = {dev_tgt.dev_id, dev_tgt.pipe_id};
  return pipeMgr->pipeMgrPktgenWritePktBuffer(session.sessHandleGet(),
                                              pipe_dev_tgt,
                                              byte_buf_offset,
                                              size,
                                              buf_vec.data());
}

bf_status_t BfRtPktgenPktBufferTable::tableClear(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  dev_target_t pipe_dev_tgt = {dev_tgt.dev_id, dev_tgt.pipe_id};
  return pipeMgr->pipeMgrPktgenClearPktBuffer(session.sessHandleGet(),
                                              pipe_dev_tgt);
}

bf_status_t BfRtPktgenPktBufferTable::tableEntryGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    BfRtTableData *data) const {
  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    LOG_TRACE("%s:%d %s ERROR : Read from hardware not supported",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtPktgenPktBufferTableKey &pktgen_key =
      static_cast<const BfRtPktgenPktBufferTableKey &>(key);
  BfRtPktgenPktBufferTableData *pktgen_data =
      static_cast<BfRtPktgenPktBufferTableData *>(data);
  uint32_t offset = pktgen_key.getId();
  dev_target_t pipe_dev_tgt = {dev_tgt.dev_id, dev_tgt.pipe_id};
  const uint32_t size = pktgen_key.getBuffSize();
  std::vector<uint8_t> buf(size, 0);
  bf_status_t status =
      pipeMgr->pipeMgrPktgenPktBufferGet(session.sessHandleGet(),
                                         pipe_dev_tgt,
                                         offset,
                                         size,
                                         static_cast<uint8_t *>(buf.data()));
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in reading pktgen pkt buffer, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  uint8_t *b = static_cast<uint8_t *>(buf.data());
  status = pktgen_data->setValue(PKTGEN_BUFFER_DATA_FIELD_ID, b, size);
  return status;
}

bf_status_t BfRtPktgenPktBufferTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret =
      std::unique_ptr<BfRtTableKey>(new BfRtPktgenPktBufferTableKey(this));
  return (*key_ret == nullptr) ? BF_NO_SYS_RESOURCES : BF_SUCCESS;
}

bf_status_t BfRtPktgenPktBufferTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret =
      std::unique_ptr<BfRtTableData>(new BfRtPktgenPktBufferTableData(this));
  return (*data_ret == nullptr) ? BF_NO_SYS_RESOURCES : BF_SUCCESS;
}

// pktgen port down reply cfg table
bf_status_t BfRtPktgenPortDownReplayCfgTable::tableEntryMod(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey & /*key*/,
    const BfRtTableData &data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const BfRtPktgenPortDownReplayCfgTableData &match_data =
      static_cast<const BfRtPktgenPortDownReplayCfgTableData &>(data);
  bf_pktgen_port_down_mode_t mode = match_data.get_mode();
  dev_target_t pipe_dev_tgt = {dev_tgt.dev_id, dev_tgt.pipe_id};
  return pipeMgr->pipeMgrPktgenPortDownReplayModeSet(
      session.sessHandleGet(), pipe_dev_tgt, mode);
}

bf_status_t BfRtPktgenPortDownReplayCfgTable::tableClear(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  bf_pktgen_port_down_mode_t mode = BF_PKTGEN_PORT_DOWN_REPLAY_NONE;
  dev_target_t pipe_dev_tgt = {dev_tgt.dev_id, dev_tgt.pipe_id};
  return pipeMgr->pipeMgrPktgenPortDownReplayModeSet(
      session.sessHandleGet(), pipe_dev_tgt, mode);
}

bf_status_t BfRtPktgenPortDownReplayCfgTable::tableEntryGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey & /*key*/,
    BfRtTableData *data) const {
  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    LOG_TRACE("%s:%d %s ERROR : Read from hardware not supported",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  BfRtPktgenPortDownReplayCfgTableData *pktgen_data =
      static_cast<BfRtPktgenPortDownReplayCfgTableData *>(data);
  bf_pktgen_port_down_mode_t mode;
  dev_target_t pipe_dev_tgt = {dev_tgt.dev_id, dev_tgt.pipe_id};
  bf_status_t status = pipeMgr->pipeMgrPktgenPortDownReplayModeGet(
      session.sessHandleGet(), pipe_dev_tgt, &mode);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in reading port down replay mode, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  pktgen_data->set_mode(mode);
  return BF_SUCCESS;
}

bf_status_t BfRtPktgenPortDownReplayCfgTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret = std::unique_ptr<BfRtTableData>(
      new BfRtPktgenPortDownReplayCfgTableData(this));
  return (*data_ret == nullptr) ? BF_NO_SYS_RESOURCES : BF_SUCCESS;
}

// pktgen Data Id classes

bf_status_t RecircEnableId::get(const BfRtSession &session,
                                bf_dev_id_t dev,
                                bf_dev_port_t port,
                                bool *enable) const {
  auto *pipe_mgr = PipeMgrIntf::getInstance(session);
  return pipe_mgr->pipeMgrRecirEnableGet(dev, port, enable);
}

bf_status_t RecircEnableId::set(const BfRtSession &session,
                                bf_dev_id_t dev,
                                bf_dev_port_t port,
                                bool enable) const {
  auto *pipe_mgr = PipeMgrIntf::getInstance(session);
  return pipe_mgr->pipeMgrRecirEnableSet(
      session.sessHandleGet(), dev, port, enable);
}

bf_status_t RecircEnableId::clear(const BfRtSession &session,
                                  bf_dev_id_t dev,
                                  bf_dev_port_t port) const {
  bf_dev_type_t dev_type = bf_drv_get_dev_type(dev);
  bool tof1 = bf_is_dev_type_family_tofino(dev_type);
  // Recirculation is not allowed on ports 68-71 for TOF1
  if (tof1 && port >= 68) return BF_SUCCESS;
  return this->set(session, dev, port, false);
}

bf_rt_id_t RecircEnableId::id() const { return RECIR_EN; }

bf_status_t PktgenEnableId::get(const BfRtSession &session,
                                bf_dev_id_t dev,
                                bf_dev_port_t port,
                                bool *enable) const {
  auto *pipe_mgr = PipeMgrIntf::getInstance(session);
  return pipe_mgr->pipeMgrPktgenEnableGet(
      session.sessHandleGet(), dev, port, enable);
}

bf_status_t PktgenEnableId::set(const BfRtSession &session,
                                bf_dev_id_t dev,
                                bf_dev_port_t port,
                                bool enable) const {
  auto *pipe_mgr = PipeMgrIntf::getInstance(session);
  return pipe_mgr->pipeMgrPktgenEnableSet(
      session.sessHandleGet(), dev, port, enable);
}

bf_status_t PktgenEnableId::clear(const BfRtSession &session,
                                  bf_dev_id_t dev,
                                  bf_dev_port_t port) const {
  bool enabled = false;
  bf_status_t sts = this->get(session, dev, port, &enabled);
  if (sts != BF_SUCCESS) return sts;
  if (enabled) sts = this->set(session, dev, port, false);
  return sts;
}

bf_rt_id_t PktgenEnableId::id() const { return PKTGEN_EN; }

bf_status_t RecircPatternMatchingEnableId::get(const BfRtSession &session,
                                               bf_dev_id_t dev,
                                               bf_dev_port_t port,
                                               bool *enable) const {
  auto *pipe_mgr = PipeMgrIntf::getInstance(session);
  return pipe_mgr->pipeMgrPktgenRecirPatternMatchingEnableGet(
      session.sessHandleGet(), dev, port, enable);
}

bf_status_t RecircPatternMatchingEnableId::set(const BfRtSession &session,
                                               bf_dev_id_t dev,
                                               bf_dev_port_t port,
                                               bool enable) const {
  auto *pipe_mgr = PipeMgrIntf::getInstance(session);
  return pipe_mgr->pipeMgrPktgenRecirPatternMatchingEnableSet(
      session.sessHandleGet(), dev, port, enable);
}

bf_status_t RecircPatternMatchingEnableId::clear(const BfRtSession &session,
                                                 bf_dev_id_t dev,
                                                 bf_dev_port_t port) const {
  bool enabled = false;
  bf_status_t sts = this->get(session, dev, port, &enabled);
  if (sts != BF_SUCCESS) return sts;
  if (enabled) sts = this->set(session, dev, port, false);
  return sts;
}

bf_rt_id_t RecircPatternMatchingEnableId::id() const { return PATTEN_MAT_EN; }

bf_status_t ClearPortDownEnableId::get(const BfRtSession &session,
                                       bf_dev_id_t dev,
                                       bf_dev_port_t port,
                                       bool *cleared) const {
  auto *pipe_mgr = PipeMgrIntf::getInstance(session);
  return pipe_mgr->pipeMgrPktgenClearPortDownGet(
      session.sessHandleGet(), dev, port, cleared);
}

bf_status_t ClearPortDownEnableId::set(const BfRtSession &session,
                                       bf_dev_id_t dev,
                                       bf_dev_port_t port,
                                       bool cleared) const {
  bf_status_t sts = BF_INVALID_ARG;
  auto *pipe_mgr = PipeMgrIntf::getInstance(session);
  if (cleared)
    sts = pipe_mgr->pipeMgrPktgenClearPortDownSet(
        session.sessHandleGet(), dev, port);
  return sts;
}

bf_status_t ClearPortDownEnableId::clear(const BfRtSession &session,
                                         bf_dev_id_t dev,
                                         bf_dev_port_t port) const {
  auto *pipe_mgr = PipeMgrIntf::getInstance(session);
  auto s_hdl = session.sessHandleGet();
  return pipe_mgr->pipeMgrPktgenClearPortDownSet(s_hdl, dev, port);
}

bf_rt_id_t ClearPortDownEnableId::id() const { return CLEAR_PORT_DOWN_EN; }

PgenDataId *PgenDataIdFactory::create(bf_rt_id_t id) {
  switch (id) {
    case RECIR_EN:
      return new RecircEnableId();
    case PKTGEN_EN:
      return new PktgenEnableId();
    case PATTEN_MAT_EN:
      return new RecircPatternMatchingEnableId();
    case CLEAR_PORT_DOWN_EN:
      return new ClearPortDownEnableId();
    default:
      LOG_ERROR("%s:%d ERROR : incorrect id: %d ", __func__, __LINE__, id);
      return nullptr;
  }
}

}  // namespace bfrt
