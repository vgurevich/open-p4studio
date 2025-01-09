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


#ifndef _TDI_CONTEXT_INFO_HPP
#define _TDI_CONTEXT_INFO_HPP

#ifdef __cplusplus
extern "C" {
#endif
#include <pipe_mgr/pipe_mgr_intf.h>
#ifdef __cplusplus
}
#endif

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <tdi/common/tdi_defs.h>
#include <tdi/common/tdi_info.hpp>
#include <tdi/common/tdi_learn.hpp>
#include <tdi/common/tdi_table.hpp>
#include <tdi/common/tdi_table_data.hpp>
#include <tdi/common/tdi_table_key.hpp>
#include <tdi/common/tdi_utils.hpp>

#include <tdi_tofino/tdi_tofino_defs.h>

#include <tdi/common/tdi_json_parser/tdi_cjson.hpp>

namespace tdi {
namespace tna {
namespace tofino {

class PipeMgrIntf;

class TofinoLearnContextInfo;
class TofinoTableContextInfo;
class TofinoKeyFieldContextInfo;
class TofinoActionContextInfo;
class TofinoDataFieldContextInfo;

typedef struct key_size_ {
  size_t bytes;
  size_t bits;
} key_size_t;

enum DataFieldType {
  INVALID,
  ACTION_PARAM,
  ACTION_PARAM_OPTIMIZED_OUT,
  COUNTER_INDEX,
  METER_INDEX,
  REGISTER_INDEX,
  LPF_INDEX,
  WRED_INDEX,
  COUNTER_SPEC_BYTES,
  COUNTER_SPEC_PACKETS,
  METER_SPEC_CIR_PPS,
  METER_SPEC_PIR_PPS,
  METER_SPEC_CBS_PKTS,
  METER_SPEC_PBS_PKTS,
  METER_SPEC_CIR_KBPS,
  METER_SPEC_PIR_KBPS,
  METER_SPEC_CBS_KBITS,
  METER_SPEC_PBS_KBITS,
  ACTION_MEMBER_ID,
  SELECTOR_GROUP_ID,
  SELECTOR_MEMBERS,
  ACTION_MEMBER_STATUS,
  MAX_GROUP_SIZE,
  TTL,
  ENTRY_HIT_STATE,
  LPF_SPEC_GAIN_TIME_CONSTANT,
  LPF_SPEC_DECAY_TIME_CONSTANT,
  LPF_SPEC_OUTPUT_SCALE_DOWN_FACTOR,
  LPF_SPEC_TYPE,
  WRED_SPEC_TIME_CONSTANT,
  WRED_SPEC_MIN_THRESHOLD,
  WRED_SPEC_MAX_THRESHOLD,
  WRED_SPEC_MAX_PROBABILITY,
  REGISTER_SPEC_HI,
  REGISTER_SPEC_LO,
  REGISTER_SPEC,
  SNAPSHOT_ENABLE,
  SNAPSHOT_TIMER_ENABLE,
  SNAPSHOT_TIMER_VALUE_USECS,
  SNAPSHOT_STAGE_ID,
  SNAPSHOT_PREV_STAGE_TRIGGER,
  SNAPSHOT_TIMER_TRIGGER,
  SNAPSHOT_LOCAL_STAGE_TRIGGER,
  SNAPSHOT_NEXT_TABLE_NAME,
  SNAPSHOT_ENABLED_NEXT_TABLES,
  SNAPSHOT_TABLE_ID,
  SNAPSHOT_TABLE_NAME,
  SNAPSHOT_MATCH_HIT_ADDRESS,
  SNAPSHOT_MATCH_HIT_HANDLE,
  SNAPSHOT_TABLE_HIT,
  SNAPSHOT_TABLE_INHIBITED,
  SNAPSHOT_TABLE_EXECUTED,
  SNAPSHOT_FIELD_INFO,
  SNAPSHOT_CONTROL_INFO,
  SNAPSHOT_METER_ALU_INFO,
  SNAPSHOT_METER_ALU_OPERATION_TYPE,
  SNAPSHOT_TABLE_INFO,
  SNAPSHOT_LIVENESS_VALID_STAGES,
  SNAPSHOT_GBL_EXECUTE_TABLES,
  SNAPSHOT_ENABLED_GBL_EXECUTE_TABLES,
  SNAPSHOT_LONG_BRANCH_TABLES,
  SNAPSHOT_ENABLED_LONG_BRANCH_TABLES,
  MULTICAST_NODE_ID,
  MULTICAST_NODE_L1_XID_VALID,
  MULTICAST_NODE_L1_XID,
  MULTICAST_ECMP_ID,
  MULTICAST_ECMP_L1_XID_VALID,
  MULTICAST_ECMP_L1_XID,
  MULTICAST_RID,
  MULTICAST_LAG_ID,
  MULTICAST_LAG_REMOTE_MSB_COUNT,
  MULTICAST_LAG_REMOTE_LSB_COUNT,
  DEV_PORT,
  DYN_HASH_CFG_START_BIT,
  DYN_HASH_CFG_LENGTH,
  DYN_HASH_CFG_ORDER,
};
enum class fieldDestination {
  ACTION_SPEC,
  DIRECT_COUNTER,
  DIRECT_METER,
  DIRECT_LPF,
  DIRECT_WRED,
  DIRECT_REGISTER,
  TTL,
  ENTRY_HIT_STATE,
  INVALID
};

/* Struct to keep info regarding a reference to a table */
typedef struct tdi_table_ref_info_ {
  std::string name;
  tdi_id_t id;
  pipe_tbl_hdl_t tbl_hdl;
  // A flag to indicate if the reference is indirect. TRUE when it is, FALSE
  // when the refernece is direct
  bool indirect_ref;
} tdi_table_ref_info_t;

// Map of reference_type -> vector of ref_info structs
using table_ref_map_t =
    std::map<std::string, std::vector<tdi_table_ref_info_t>>;

class ContextInfoParser {
 public:
  ContextInfoParser(const TdiInfo *tdi_info,
                    const ProgramConfig &program_config,
                    const tdi_dev_id_t &dev_id);
  std::unique_ptr<TofinoLearnContextInfo> parseLearnContextInfo(
      Cjson &learn_context,
      const LearnInfo *tdi_learn_info,
      const std::string &prog_name);
  std::unique_ptr<TofinoTableContextInfo> parseTableContext(
      Cjson &table_context,
      const TableInfo *tdi_table_Info,
      const tdi_dev_id_t &dev_id,
      const std::string &prog_name);
  std::unique_ptr<TofinoKeyFieldContextInfo> parseKeyFieldContext(
      const KeyFieldInfo *tdi_key_field_info,
      const size_t &field_offset,
      const size_t &start_bit,
      const size_t &parent_field_byte_size,
      const bool &is_atcam_partition_index);

  std::unique_ptr<TofinoActionContextInfo> parseActionContext(
      Cjson &action_context,
      const ActionInfo *tdi_action_Info,
      const TableInfo *tdi_table_info,
      const TofinoTableContextInfo *table_context_info);
  std::unique_ptr<TofinoDataFieldContextInfo> parseDataFieldContext(
      Cjson action_indirect_res,
      const DataFieldInfo *tdi_data_field_info,
      const TableInfo *tdi_table_info,
      const TofinoTableContextInfo *table_context_info,
      size_t &field_offset,
      size_t &bitsize,
      pipe_act_fn_hdl_t action_handle);

  static tdi_status_t parseContextJson(const TdiInfo *tdi_info,
                                       const tdi_dev_id_t &dev_id,
                                       const ProgramConfig &program_config);

  tdi_status_t setGhostTableHandles(Cjson &table_context,
                                    const TdiInfo *tdi_info);

  pipe_tbl_hdl_t getPvsHdl(Cjson &parser_context_obj);

  void populateParserObj(const std::string &profile_name,
                         Cjson &parser_gress_ctxt);

  // This function applies a mask (obtained from pipr mgr) on all the
  // handles parsed from tdi/context jsons. We need to do this as the
  // for multiprogram and multiprofile case, we need to ensure that the
  // handles are unique across all the programs on the device
  template <typename T>
  void applyMaskOnContextJsonHandle(T *handle,
                                    const std::string &name,
                                    const bool is_learn = false);

  size_t getStartBit(const Cjson *context_json_key_field,
                     const KeyFieldInfo *key_field_info,
                     const TableInfo *tdi_table_info) const;
#if 0
      std::string getParentKeyName(const Cjson &key_field) const;
#endif

  bool isFieldSlice(const KeyFieldInfo *tdi_key_field_info,
                    const size_t &start_bit,
                    const size_t &key_width_byte_size,
                    const size_t &key_width_parent_byte_size) const;
  void parseKeyHelper(
      const Cjson *context_json_table_keys,
      const std::map<tdi_id_t, std::unique_ptr<KeyFieldInfo>> &table_key_map,
      const tdi_tofino_table_type_e &table_type,
      std::map<tdi_id_t, int> &key_id_to_byte_offset,
      std::map<tdi_id_t, int> &key_id_to_byte_sz,
      std::map<tdi_id_t, int> &key_id_to_start_bit,
      size_t *num_valid_match_bytes,
      size_t *num_valid_match_bits);

  tdi_status_t keyByteSizeAndOffsetGet(
      const std::string &table_name,
      const std::string &key_name,
      const std::map<std::string, size_t> &match_key_field_name_to_position_map,
      const std::map<std::string, size_t>
          &match_key_field_name_to_byte_size_map,
      const std::map<size_t, size_t> &match_key_field_position_to_offset_map,
      size_t *field_offset,
      size_t *field_byte_size);

  bool isParamActionParam(const TableInfo *tdi_table_info,
                          const TofinoTableContextInfo *table_context_info,
                          tdi_id_t action_handle,
                          std::string parameter_name,
                          bool use_p4_params_node = false);

  bool isActionParam_matchTbl(const TableInfo *tdi_table_info,
                              const TofinoTableContextInfo *table_context_info,
                              tdi_id_t action_handle,
                              std::string parameter_name);

  bool isActionParam_actProf(const TableInfo *tdi_table_info,
                             tdi_id_t action_handle,
                             std::string parameter_name,
                             bool use_p4_params_node = false);

  bool isActionParam(Cjson action_table_cjson,
                     tdi_id_t action_handle,
                     std::string action_name,
                     std::string parameter_name,
                     bool use_p4_params_node = false);

  void setDirectRegisterDataFieldType(const std::vector<tdi_id_t> &data_fields,
                                      const TableInfo *tdi_table_info);

  void setIndirectRegisterDataFieldType(const TableInfo *tdi_table_info);

  // TODO: To be made private members
  // tableInfo has mapping from id to object.
  // context json parsing need name to object mapping between table name and
  // TableInfo object, table_context json pair
  std::map<std::string, std::pair<const TableInfo *, std::shared_ptr<Cjson>>>
      tableInfo_contextJson_map;

  // Map from table handle to table ID
  std::map<pipe_tbl_hdl_t, tdi_id_t> handleToIdMap;

  // Map to store pipeline_name <-> context_json_handle_mask
  // which is the mask to be applied (ORed) on all handles
  // parsed from ctx_json
  std::map<std::string, tdi_id_t> context_json_handle_mask_map;

  // Map to store table_name <-> pipeline_name
  std::map<std::string, std::string> table_pipeline_name;
  // Map to store learn <-> pipeline_name
  std::map<std::string, std::string> learn_pipeline_name;

  std::vector<tdi::Cjson> context_json_files;

  // class_end
};

class TofinoLearnContextInfo : public LearnContextInfo {
  tdi_id_t learn_id_;
  pipe_fld_lst_hdl_t flow_lrn_fld_lst_hdl_;

  friend class ContextInfoParser;
};

class TofinoTableContextInfo : public TableContextInfo {
 public:
  std::string tableNameGet() const { return table_name_; };

  tdi_id_t tableIdGet() const { return table_id_; };

  tdi_tofino_table_type_e tableTypeGet() const { return table_type_; };

  pipe_tbl_hdl_t tableHdlGet() const { return table_hdl_; };

  const table_ref_map_t &tableRefMapGet() const { return table_ref_map_; };

  bool isConstTable() const { return is_const_table_; };

  key_size_t keySizeGet() const { return key_size_; };

  size_t maxDataSzGet() const { return maxDataSz_; };
  size_t maxDataSzBitsGet() const { return maxDataSzbits_; };

  static std::unique_ptr<TofinoTableContextInfo> makeTableContextInfo(
      tdi_tofino_table_type_e);

  virtual tdi_status_t ghostTableHandleSet(
      const pipe_tbl_hdl_t & /*hdl*/) const {
    LOG_ERROR("%s:%d API Not supported", __func__, __LINE__);
    return TDI_NOT_SUPPORTED;
  }

 protected:
  std::string table_name_;
  tdi_id_t table_id_;
  tdi_tofino_table_type_e table_type_;
  pipe_tbl_hdl_t table_hdl_;
  // Map of reference_type -> vector of ref_info structs
  table_ref_map_t table_ref_map_;
  bool is_const_table_;
  // hash_bit_width of hash object. Only required for the hash tables
  uint64_t hash_bit_width_ = 0;
  key_size_t key_size_;

  mutable std::set<tdi_tofino_operations_type_e> operations_type_set_;
  mutable std::set<tdi_tofino_attributes_type_e> attributes_type_set_;

  size_t maxDataSz_{0};
  size_t maxDataSzbits_{0};

  std::vector<tdi_table_ref_info_t> tableGetRefNameVec(std::string ref) const {
    if (table_ref_map_.find(ref) != table_ref_map_.end()) {
      return table_ref_map_.at(ref);
    }
    return {};
  }

  friend class ContextInfoParser;
};

class MatchActionTableContextInfo : public TofinoTableContextInfo {
 public:
  MatchActionTableContextInfo() : TofinoTableContextInfo(){};
  pipe_tbl_hdl_t resourceHdlGet(const DataFieldType &field_type) const {
    tdi_table_ref_info_t tbl_ref;
    bf_status_t status = resourceInternalGet(field_type, &tbl_ref);
    if (status != BF_SUCCESS) {
      return 0;
    }
    return tbl_ref.tbl_hdl;
  }

  pipe_tbl_hdl_t indirectResourceHdlGet(const DataFieldType &field_type) const {
    tdi_table_ref_info_t tbl_ref;
    bf_status_t status = resourceInternalGet(field_type, &tbl_ref);
    if (status != BF_SUCCESS) {
      return 0;
    }
    if (tbl_ref.indirect_ref) {
      return tbl_ref.tbl_hdl;
    }
    return 0;
  }

  void isTernaryTableSet(const tdi_dev_id_t &dev_id);
  const bool &isTernaryTableGet() const { return is_ternary_table_; };

  const std::map<pipe_act_fn_hdl_t, tdi_id_t> &actFnHdlToIdGet() const {
    return act_fn_hdl_to_id_;
  };

  void actionResourcesSet(const tdi_dev_id_t &dev_id);
  void actionResourcesGet(const tdi_id_t action_id,
                          bool *meter,
                          bool *reg,
                          bool *cntr) const {
    *cntr = *meter = *reg = false;
    // Will default to false if action_id does not exist.
    if (this->act_uses_dir_cntr_.find(action_id) !=
        this->act_uses_dir_cntr_.end()) {
      *cntr = this->act_uses_dir_cntr_.at(action_id);
    }
    if (this->act_uses_dir_reg_.find(action_id) !=
        this->act_uses_dir_reg_.end()) {
      *reg = this->act_uses_dir_reg_.at(action_id);
    }
    if (this->act_uses_dir_meter_.find(action_id) !=
        this->act_uses_dir_meter_.end()) {
      *meter = this->act_uses_dir_meter_.at(action_id);
    }
  }
  const tdi::Table *actProfGet() const { return actProfTbl_; };
  const tdi::Table *selectorGet() const { return selectorTbl_; };

 private:
  tdi_status_t resourceInternalGet(const DataFieldType &field_type,
                                   tdi_table_ref_info_t *tbl_ref) const;
  // Store information about direct resources applicable per action
  std::map<tdi_id_t, bool> act_uses_dir_meter_;
  std::map<tdi_id_t, bool> act_uses_dir_cntr_;
  std::map<tdi_id_t, bool> act_uses_dir_reg_;
  // if this table is a ternary table
  bool is_ternary_table_{false};

  // A map from action fn handle to action id
  std::map<pipe_act_fn_hdl_t, tdi_id_t> act_fn_hdl_to_id_;
  mutable tdi::Table *actProfTbl_;
  mutable tdi::Table *selectorTbl_;
  // Action profile table ID associated with this table. Applicable for
  // MatchAction_Indirect, MatchAction_Indirect_Selector and Selector table
  // types
  mutable tdi_id_t act_prof_id_;
  // Selector table ID associated with this table. Applicable for
  // MatchAction_Indirect_Selector table
  mutable tdi_id_t selector_tbl_id_;

  friend class ContextInfoParser;
};

class MatchActionDirectTableContextInfo : public MatchActionTableContextInfo {
 public:
  MatchActionDirectTableContextInfo() : MatchActionTableContextInfo(){};
};

class MatchActionIndirectTableContextInfo : public MatchActionTableContextInfo {
 public:
  MatchActionIndirectTableContextInfo() : MatchActionTableContextInfo(){};
};

class SelectorTableContextInfo : public TofinoTableContextInfo {
 public:
  SelectorTableContextInfo() : TofinoTableContextInfo(){};

 private:
  mutable tdi::Table *actProfTbl_;
  // Action profile table ID associated with this table. Applicable for
  // MatchAction_Indirect, MatchAction_Indirect_Selector and Selector table
  // types
  mutable tdi_id_t act_prof_id_;

  friend class ContextInfoParser;
  friend class Selector;
};

class RegisterIndirectTableContextInfo : public TofinoTableContextInfo {
 public:
  RegisterIndirectTableContextInfo()
      : TofinoTableContextInfo(), ghost_pipe_tbl_hdl_(){};

  tdi_status_t ghostTableHandleSet(
      const pipe_tbl_hdl_t &pipe_hdl) const override {
    (const_cast<RegisterIndirectTableContextInfo *>(this))
        ->ghost_pipe_tbl_hdl_ = pipe_hdl;
    return TDI_SUCCESS;
  }

 private:
  pipe_tbl_hdl_t ghost_pipe_tbl_hdl_;
};

class ActionProfileContextInfo : public TofinoTableContextInfo {
 public:
  ActionProfileContextInfo() : TofinoTableContextInfo(){};

  const std::map<pipe_act_fn_hdl_t, tdi_id_t> &actFnHdlToIdGet() const {
    return act_fn_hdl_to_id_;
  };
  pipe_tbl_hdl_t resourceHdlGet(const DataFieldType &field_type) const {
    tdi_table_ref_info_t tbl_ref;
    bf_status_t status = resourceInternalGet(field_type, &tbl_ref);
    if (status != BF_SUCCESS) {
      return 0;
    }
    return tbl_ref.tbl_hdl;
  }

  pipe_tbl_hdl_t indirectResourceHdlGet(const DataFieldType &field_type) const {
    tdi_table_ref_info_t tbl_ref;
    bf_status_t status = resourceInternalGet(field_type, &tbl_ref);
    if (status != BF_SUCCESS) {
      return 0;
    }
    if (tbl_ref.indirect_ref) {
      return tbl_ref.tbl_hdl;
    }
    return 0;
  }

 private:
  tdi_status_t resourceInternalGet(const DataFieldType &field_type,
                                   tdi_table_ref_info_t *tbl_ref) const;

  // A map from action fn handle to action id
  std::map<pipe_act_fn_hdl_t, tdi_id_t> act_fn_hdl_to_id_;

  friend class ContextInfoParser;
};

class PortMetadataTableContextInfo : public MatchActionTableContextInfo {
 public:
  PortMetadataTableContextInfo() : MatchActionTableContextInfo(){};
  std::unique_ptr<TofinoActionContextInfo> phase0_action_context_info_;
};

class TofinoKeyFieldContextInfo : public KeyFieldContextInfo {
 public:
  std::string nameGet() const { return name_; };

  size_t startBitGet() const { return start_bit_; };

  size_t fieldOffsetGet() const { return field_offset_; };

  size_t parentFieldFullByteSizeGet() const {
    return parent_field_full_byte_size_;
  };

  bool isFieldSlice() const { return is_field_slice_; };
  bool isPartition() const { return is_partition_; };
  bool isMatchPriority() const { return is_match_priority_; };

 private:
  std::string name_;
  size_t start_bit_{0};
  size_t field_offset_;
  // Flag to indicate if this is a field slice or not
  bool is_field_slice_{false};
  // This might vary from the 'size_bits' in case of field slices when the field
  // slice width does not equal the size of the entire key field
  size_t parent_field_full_byte_size_{0};
  // flag to indicate whether it is a priority index or not
  bool is_partition_{false};
  bool is_match_priority_{false};

  friend class ContextInfoParser;
};

class TofinoActionContextInfo : public ActionContextInfo {
 public:
  std::string nameGet() const { return name_; };

  // tdi_id_t actionIdGet() const { return action_id_; };

  pipe_act_fn_hdl_t actionFnHdlGet() const { return act_fn_hdl_; };

  size_t dataSzGet() const { return dataSz_; };

  size_t dataSzBitsGet() const { return dataSzbits_; };

 private:
  std::string name_;
  // tdi_id_t action_id_;
  pipe_act_fn_hdl_t act_fn_hdl_;
  size_t dataSz_;
  // Size of the action data in bits (not including byte padding)
  size_t dataSzbits_;

  friend class ContextInfoParser;
};

class TofinoDataFieldContextInfo : public DataFieldContextInfo {
 public:
  std::string nameGet() const { return name_; };
  const std::set<DataFieldType> &typesGet() const { return types_; }
  void typeAdd(DataFieldType type) const { types_.insert(type); }
  const size_t &offsetGet() const { return offset_; };
  static fieldDestination getDataFieldDestination(
      const std::set<DataFieldType> &fieldTypes);

 private:
  std::string name_;
  size_t offset_{0};
  mutable std::set<DataFieldType> types_;  // resource_set
  friend class ContextInfoParser;
};

const std::vector<std::string> indirect_refname_list = {
    "action_data_table_refs",
    "selection_table_refs",
    "meter_table_refs",
    "statistics_table_refs",
    "stateful_table_refs"};

void prependPipePrefixToKeyName(const std::string &&key,
                                const std::string &prefix,
                                Cjson *object);

void changeTableName(const std::string &pipe_name,
                     std::shared_ptr<Cjson> *context_table_cjson);

void changeIndirectResourceName(const std::string &pipe_name,
                                std::shared_ptr<Cjson> *context_table_cjson);

void changeActionIndirectResourceName(
    const std::string &pipe_name, std::shared_ptr<Cjson> *context_table_cjson);

void changeDynHashCalcName(const std::string &pipe_name,
                           Cjson *root_cjson_context);

void changeLearnName(const std::string &pipe_name, Cjson *root_cjson_context);

void changePvsName(const std::string &pipe_name, Cjson *root_pvs_context);

std::string getAlgoTableName(const std::string &cfg_table_name);

DataFieldType getDataFieldTypeFrmName(std::string data_name,
                                      tdi_tofino_table_type_e table_type);

DataFieldType getDataFieldTypeFrmRes(tdi_tofino_table_type_e table_type);

}  // namespace tofino
}  // namespace tna
}  // namespace tdi

#endif  // ifdef _TDI_CONTEXT_INFO_HPP
