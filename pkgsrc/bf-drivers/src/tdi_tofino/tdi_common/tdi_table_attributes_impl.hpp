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

#ifndef _TDI_TABLE_ATTRIBUTES_IMPL_HPP
#define _TDI_TABLE_ATTRIBUTES_IMPL_HPP

#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <memory>
#include <unordered_map>
#include <functional>

#include <tdi/common/tdi_attributes.hpp>
#include <tdi/common/c_frontend/tdi_attributes.h>
#include <tdi/common/tdi_table.hpp>
#include <tdi/common/tdi_table_key.hpp>

#include <tdi_tofino/tdi_tofino_init.hpp>
#include <tdi_tofino/tdi_tofino_attributes.hpp>
#include <tdi_tofino/c_frontend/tdi_tofino_attributes.h>
#include "tdi_pipe_mgr_intf.hpp"

namespace tdi {
namespace tna {
namespace tofino {

class TableEntryScopeArguments {
 public:
  TableEntryScopeArguments(const std::bitset<32> &val);
  TableEntryScopeArguments(const std::array<std::bitset<8>, 4> &val_arr);
  tdi_status_t setValue(const std::bitset<32> &val);
  tdi_status_t setValue(const std::array<std::bitset<8>, 4> &val_arr);
  tdi_status_t getValue(std::bitset<32> *val) const;
  tdi_status_t getValue(std::array<std::bitset<8>, 4> *val_arr) const;

 private:
  // scope_value_ is a 32 bit unsigned int. The least significant byte (byte 0)
  // of which indicates the pipes present in scope 0, byte 1 indicates the
  // pipes in scope 1, and so on. Thus we can have a maximum of 4 scope. Thus
  // if the scope_value_ is set to 0xc03, pipes 0 and 1 are in scope 0 and
  // pipes 2 and 3 are in scope 1
  uint32_t scope_value_;
};

// This class is responsible for handling everything related to idle params
// and is called by the parent class (TableAttributesImpl)
class TableAttributesIdleTable {
 public:
  TableAttributesIdleTable();
  tdi_status_t idleTablePollModeSet(const bool &enable);
  tdi_status_t idleTableNotifyModeSet(const bool &enable,
                                      const IdleTmoExpiryCb &callback,
                                      const tdi_idle_tmo_expiry_cb &callback_c,
                                      const uint32_t &ttl_query_interval,
                                      const uint32_t &max_ttl,
                                      const uint32_t &min_ttl,
                                      const void *cookie);

  tdi_status_t idleTableGet(tdi_tofino_attributes_idle_table_mode_e *mode,
                            bool *enable,
                            IdleTmoExpiryCb *callback,
                            tdi_idle_tmo_expiry_cb *callback_c,
                            uint32_t *ttl_query_interval,
                            uint32_t *max_ttl,
                            uint32_t *min_ttl,
                            void **cookie) const;

  const pipe_idle_time_params_t &getTtlParamsInternal() const {
    return idle_time_param_;
  };
  const IdleTmoExpiryCb &getCallback() const { return callback_cpp_; };
  const tdi_idle_tmo_expiry_cb &getCallbackC() const { return callback_c_; };
  const bool &getEnabled() const { return enable_; };
  tdi_status_t idleTableModeSet(
      const tdi_tofino_attributes_idle_table_mode_e &table_type);
  tdi_tofino_attributes_idle_table_mode_e idleTableModeGet() const;
  void idleTableAllParamsClear();

  bool enable_{false};
  IdleTmoExpiryCb callback_cpp_{nullptr};
  tdi_idle_tmo_expiry_cb callback_c_{nullptr};
  pipe_idle_time_params_t idle_time_param_{};
};

// This class is responsible for handling everything related to entry scope
// and is called by the parent class (TableAttributesImpl)
class TableAttributesEntryScope {
 public:
  TableAttributesEntryScope();
  tdi_status_t entryScopeParamsSet(
      const tdi_tofino_attributes_entry_scope_e &entry_scope,
      const TableEntryScopeArguments &scope_args);
  tdi_status_t entryScopeParamsSet(
      const tdi_tofino_attributes_entry_scope_e &entry_scope);
  tdi_status_t entryScopeParamsGet(
      tdi_tofino_attributes_entry_scope_e *entry_scope,
      TableEntryScopeArguments *scope_args) const;

  tdi_status_t gressScopeParamsSet(
      const tdi_tofino_attributes_gress_scope_e &gress_scope) {
    gress_scope_ = gress_scope;
    return TDI_SUCCESS;
  };
  tdi_status_t gressScopeParamsGet(
      tdi_tofino_attributes_gress_scope_e *gress_scope) const {
    *gress_scope = gress_scope_;
    return TDI_SUCCESS;
  };

  tdi_status_t prsrScopeParamsSet(
      const tdi_tofino_attributes_parser_scope_e &prsr_scope,
      const tdi_tofino_attributes_gress_target_e &prsr_gress);
  tdi_status_t prsrScopeParamsGet(
      tdi_tofino_attributes_parser_scope_e *prsr_scope,
      tdi_tofino_attributes_gress_target_e *prsr_gress) const;
  void entryScopeAllParamsClear();

  tdi_tofino_attributes_entry_scope_e entry_scope_{
      TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_ALL_PIPELINES};
  tdi_tofino_attributes_gress_scope_e gress_scope_{
      TDI_TOFINO_ATTRIBUTES_GRESS_SCOPE_ALL_GRESS};
  tdi_tofino_attributes_parser_scope_e prsr_scope_{
      TDI_TOFINO_ATTRIBUTES_PARSER_SCOPE_ALL_PARSERS_IN_PIPE};
  // Property arguments
  uint64_t property_args_value_{0};
  uint64_t prsr_args_value_{TNA_DIRECTION_ALL};
};

// This class is responsible for handling everything related to dynamic key mask
// and is called by the parent class (TableAttributesImpl)
class TableAttributesDynKeyMask {
 public:
  TableAttributesDynKeyMask();

  void dynKeyMaskAllParamsClear();
  tdi_status_t dynKeyMaskParamsSet(
      const std::unordered_map<tdi_id_t, std::vector<uint8_t>> &field_mask);
  tdi_status_t dynKeyMaskParamsGet(
      std::unordered_map<tdi_id_t, std::vector<uint8_t>> *field_mask) const;

  std::unordered_map<tdi_id_t, std::vector<uint8_t>> field_mask_;
};

// This class is responsible for handling everything related to meter byte count
// adjust,
// and is called by the parent class (TableAttributesImpl)
class TableAttributesMeterByteCountAdj {
 public:
  TableAttributesMeterByteCountAdj();

  void byteCountAdjClear();
  tdi_status_t byteCountAdjSet(const int &byte_count_adj);
  tdi_status_t byteCountAdjGet(int *byte_count_adj) const;

  int byte_count_adj_;
};

// This class is responsible for handling everything related to port status
// change callback function
// and is called by the parent class (TableAttributesImpl)
class TableAttributePortStatusChangeReg {
 public:
  TableAttributePortStatusChangeReg();

  tdi_status_t portStatusChgCbSet(const bool enable,
                                  const PortStatusNotifCb &cb,
                                  const tdi_port_status_chg_cb &cb_c,
                                  const void *cookie);
  tdi_status_t portStatusChgCbGet(bool *enable,
                                  PortStatusNotifCb *cb,
                                  tdi_port_status_chg_cb *cb_c,
                                  void **cookie) const;
  void portStatusChgParamsClear();

  bool enable_;
  PortStatusNotifCb callback_func_;
  tdi_port_status_chg_cb callback_func_c_;
  void *client_data_;
};

// This class is responsible for handling everything related to port stat poll
// intvl
// and is called by the parent class (TableAttributesImpl)
class TableAttributesPortStatPollIntvl {
 public:
  TableAttributesPortStatPollIntvl();

  tdi_status_t portStatPollIntvlParamSet(const uint32_t &poll_intvl);
  tdi_status_t portStatPollIntvlParamGet(uint32_t *poll_intvl) const;
  void portStatPollIntvlParamClear();

  uint32_t intvl_;
};

// This class is responsible for handling everything related to
// PRE global rid config and is called by the
// parent class (TableAttributesImpl)
class TableAttributesPREDeviceConfig {
 public:
  // Enums to support selctive PRE attributes set/get
  enum class PREAttributeType {
    PRE_GLOBAL_RID = 0,
    PRE_FAST_FAILOVER = 1,
    PRE_PORT_PROTECTION = 2,
    PRE_MAX_NODES_BEFORE_YIELD = 3,
    PRE_MAX_NODE_THRESHOLD = 4
  };

  TableAttributesPREDeviceConfig();

  tdi_status_t preGlobalRidParamSet(const uint32_t &global_rid);
  tdi_status_t prePortProtectionParamSet(const bool &enable);
  tdi_status_t preFastFailoverParamSet(const bool &enable);
  tdi_status_t preMaxNodesBeforeYieldParamSet(const uint32_t &count);
  tdi_status_t preMaxNodeThresholdParamSet(const uint32_t &node_count,
                                           const uint32_t &node_port_lag_count);

  void preDeviceConfigParamsClear();

  tdi_status_t preGlobalRidParamGet(uint32_t *global_rid) const;
  tdi_status_t prePortProtectionParamGet(bool *enable) const;
  tdi_status_t preFastFailoverParamGet(bool *enable) const;
  tdi_status_t preMaxNodesBeforeYieldParamGet(uint32_t *count) const;
  tdi_status_t preMaxNodeThresholdParamGet(uint32_t *node_count,
                                           uint32_t *node_port_lag_count) const;

 private:
  std::set<PREAttributeType> active_pre_attributes_;
  uint32_t global_rid_;                         // Initialized by constructor
  bool fast_failover_enable_;                   // Initialized by constructor
  bool port_protection_enable_;                 // Initialized by constructor
  uint32_t max_nodes_before_yield_;             // Initialized by constructor
  uint32_t max_node_threshold_node_count_;      // Initialized by constructor
  uint32_t max_node_threshold_port_lag_count_;  // Initialized by constructor
};

class TableAttributesSelectorUpdateCallback {
 public:
  TableAttributesSelectorUpdateCallback();

  void paramSet(const bool &enable,
                const std::shared_ptr<tdi::Session> session,
                const SelUpdateCb &cpp_callback_fn,
                const tdi_selector_table_update_cb &c_callback_fn,
                const void *cookie);

  void paramGet(bool *enable,
                tdi::Session **session,
                SelUpdateCb *cpp_callback_fn,
                tdi_selector_table_update_cb *c_callback_fn,
                void **cookie) const;

  void paramsClear();

  void paramSetInternal(const std::tuple<bool,
                                         std::weak_ptr<tdi::Session>,
                                         SelUpdateCb,
                                         tdi_selector_table_update_cb,
                                         const void *> &t);

  std::tuple<bool,
             std::weak_ptr<tdi::Session>,
             SelUpdateCb,
             tdi_selector_table_update_cb,
             const void *>
  paramGetInternal() const;

 private:
  bool enable_{false};
  std::weak_ptr<tdi::Session> session_;
  SelUpdateCb cpp_callback_fn_{nullptr};
  tdi_selector_table_update_cb c_callback_fn_{nullptr};
  const void *cookie_{nullptr};
};

// This class is the implementation of the TableAttributes interface
// and is internally composed of multiple classes (components) with
// each sub class being responsible for one attribute type
class TableAttributes : public tdi::TableAttributes {
 public:
  TableAttributes(const Table *table, const tdi_attributes_type_e &attr_type)
      : tdi::TableAttributes(table, attr_type){};
  virtual tdi_status_t setValue(const tdi_attributes_field_type_e &type,
                                const uint64_t &value) override;
  virtual tdi_status_t getValue(const tdi_attributes_field_type_e &type,
                                uint64_t *value) const override;
  virtual tdi_status_t setValue(const tdi_attributes_field_type_e &type,
                                const uint8_t * /*value*/,
                                const size_t &size_bytes) override;
  virtual tdi_status_t getValue(const tdi_attributes_field_type_e &type,
                                const size_t &size_bytes,
                                uint8_t *value) const override;

  tdi_status_t idleTablePollModeSet(const bool &enable);
  tdi_status_t idleTableNotifyModeSet(const bool &enable,
                                      const IdleTmoExpiryCb &callback,
                                      const uint32_t &ttl_query_interval,
                                      const uint32_t &max_ttl,
                                      const uint32_t &min_ttl,
                                      const void *cookie);

  tdi_status_t idleTableGet(tdi_tofino_attributes_idle_table_mode_e *mode,
                            bool *enable,
                            IdleTmoExpiryCb *callback,
                            uint32_t *ttl_query_interval,
                            uint32_t *max_ttl,
                            uint32_t *min_ttl,
                            void **cookie) const;

  tdi_status_t dynKeyMaskSet(
      const std::unordered_map<tdi_id_t, std::vector<uint8_t>> &field_mask);
  tdi_status_t dynKeyMaskGet(
      std::unordered_map<tdi_id_t, std::vector<uint8_t>> *field_mask) const;

  tdi_status_t meterByteCountAdjSet(const int &byte_count);
  tdi_status_t meterByteCountAdjGet(int *byte_count) const;
  tdi_status_t portStatusChangeNotifSet(
      const bool &enable,
      const PortStatusNotifCb &callback,
      const tdi_port_status_chg_cb &callback_c,
      const void *cookie);

  tdi_status_t portStatusChangeNotifGet(bool *enable,
                                        PortStatusNotifCb *callback,
                                        tdi_port_status_chg_cb *callback_c,
                                        void **cookie) const;

  tdi_status_t portStatPollIntvlMsSet(const uint32_t &poll_intvl_ms);

  tdi_status_t portStatPollIntvlMsGet(uint32_t *poll_intvl_ms) const;

  tdi_status_t preGlobalRidSet(const uint32_t &global_rid);

  tdi_status_t preGlobalRidGet(uint32_t *global_rid) const;

  tdi_status_t prePortProtectionSet(const bool &enable);

  tdi_status_t prePortProtectionGet(bool *enable) const;

  tdi_status_t preFastFailoverSet(const bool &enable);

  tdi_status_t preFastFailoverGet(bool *enable) const;

  tdi_status_t preMaxNodesBeforeYieldSet(const uint32_t &count);

  tdi_status_t preMaxNodesBeforeYieldGet(uint32_t *count) const;

  tdi_status_t preMaxNodeThresholdSet(const uint32_t &node_count,
                                      const uint32_t &node_port_lag_count);

  tdi_status_t preMaxNodeThresholdGet(uint32_t *node_count,
                                      uint32_t *node_port_lag_count) const;

  tdi_status_t selectorUpdateCbSet(const bool &enable,
                                   const std::shared_ptr<tdi::Session> session,
                                   const SelUpdateCb &callback_fn,
                                   const void *cookie);

  tdi_status_t selectorUpdateCbGet(bool *enable,
                                   tdi::Session **session,
                                   SelUpdateCb *callback_fn,
                                   void **cookie) const;

  // Hidden functions
  tdi_status_t resetAttributeType(const tdi_attributes_type_e &attr);

  const pipe_idle_time_params_t &getIdleTableTtlParamsInternal() const {
    return idle_table_.getTtlParamsInternal();
  };
  const IdleTmoExpiryCb &getIdleTableCallback() const {
    return idle_table_.getCallback();
  };
  const tdi_idle_tmo_expiry_cb &getIdleTableCallbackC() const {
    return idle_table_.getCallbackC();
  };
  bool getIdleTableEnabled() const { return idle_table_.getEnabled(); };
  tdi_status_t idleTableModeSet(
      const tdi_tofino_attributes_idle_table_mode_e &table_type) {
    return idle_table_.idleTableModeSet(table_type);
  }
  tdi_tofino_attributes_idle_table_mode_e idleTableModeGet() const {
    return idle_table_.idleTableModeGet();
  }
  tdi_status_t idleTableNotifyModeSetCFrontend(
      const bool &enable,
      tdi_idle_tmo_expiry_cb callback_c,
      const uint32_t &ttl_query_interval,
      const uint32_t &max_ttl,
      const uint32_t &min_ttl,
      const void *cookie);
  tdi_status_t idleTableGetCFrontend(
      tdi_tofino_attributes_idle_table_mode_e *mode,
      bool *enable,
      tdi_idle_tmo_expiry_cb *callback_c,
      uint32_t *ttl_query_interval,
      uint32_t *max_ttl,
      uint32_t *min_ttl,
      void **cookie) const;
  tdi_status_t portStatusChangeNotifSetCFrontend(
      const bool &enable,
      tdi_port_status_chg_cb callback_c,
      const void *cookie);

  tdi_status_t portStatusChangeNotifGetCFrontend(
      bool *enable, tdi_port_status_chg_cb *callback_c, void **cookie) const;

  void selectorUpdateCbInternalSet(
      const std::tuple<bool,
                       std::weak_ptr<tdi::Session>,
                       SelUpdateCb,
                       tdi_selector_table_update_cb,
                       const void *> &t);

  std::tuple<bool,
             std::weak_ptr<tdi::Session>,
             SelUpdateCb,
             tdi_selector_table_update_cb,
             const void *>
  selectorUpdateCbInternalGet() const;

  tdi_status_t selectorUpdateCbSetCFrontend(
      const bool &enable,
      const std::shared_ptr<tdi::Session> session,
      const tdi_selector_table_update_cb &callback_fn,
      const void *cookie);

  tdi_status_t selectorUpdateCbGetCFrontend(
      bool *enable,
      tdi::Session **session,
      tdi_selector_table_update_cb *callback_fn,
      void **cookie) const;

  // Idle Params Attribute class object
  TableAttributesIdleTable idle_table_;
  // Entry Scope Attribute class object
  TableAttributesEntryScope entry_scope_;
  // Dyn Key Mask class object
  TableAttributesDynKeyMask dyn_key_mask_;
  // Meter Byte Count Adjust Attribute class object
  TableAttributesMeterByteCountAdj meter_bytecount_adj_;
  // Port Cfg class object
  TableAttributePortStatusChangeReg port_status_change_;
  // Port Stat class object
  TableAttributesPortStatPollIntvl port_stat_poll_;
  // PRE Decive Config  class object
  TableAttributesPREDeviceConfig pre_device_config_;
  // Selector Update callback class object
  TableAttributesSelectorUpdateCallback selector_update_callback_;
  // Other Attribute class objects will go here
};

}  // namespace tofino
}  // namespace tna
}  // namespace tdi

#endif  // _TDI_TABLE_ATTRIBUTES_IMPL_HPP
