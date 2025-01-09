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


#ifndef _BF_RT_DEV_TABLE_DATA_IMPL_HPP
#define _BF_RT_DEV_TABLE_DATA_IMPL_HPP

#include <bf_rt_common/bf_rt_table_data_impl.hpp>
#include <bf_rt_common/bf_rt_table_impl.hpp>

extern "C" {
#include <tofino/bf_pal/dev_intf.h>
}

namespace bfrt {

enum DevCfgDataFieldId {
  SKU = 1,
  NUM_PIPES = 2,
  NUM_STAGES = 3,
  NUM_MAX_PORTS = 4,
  NUM_FRONT_PORTS = 5,
  PCIE_CPU_PORT = 6,
  ETH_CPU_PORT_LIST = 7,
  INTERNAL_PORT_LIST = 8,
  EXTERNAL_PORT_LIST = 9,
  RECIRC_PORT_LIST = 10,
  INTR_BASED_LINK_MONITORING = 11,
  FLOW_LEARN_INTR_MODE = 12,
  LRT_DR_TIMEOUT_USEC = 13,
  FLOW_LEARN_TIMEOUT_USEC = 14,
  INACTIVE_NODE_DELETE = 15,
  SELECTOR_MEMBER_ORDER = 16,
  ID_MAX_INVALID = 17
};

enum WarmInitDataFieldId {
  ERROR = 1,
  INIT_MODE = 2,
  UPGRADE_AGENTS = 3,
  P4_PROGRAMS = 4,
  P4_NAME = 5,
  P4_PIPELINES = 6,
  RUNTIME_CTX_FILE = 7,
  BINARY_PATH = 8,
  PIPELINE_NAME = 9,
  PIPE_SCOPE = 10,
  BFRT_JSON_FILE = 11
};

enum DevWarmInitActionFieldId {
  WARM_INIT_BEGIN = 1,
  WARM_INIT_COMPLETED = 2,
};

class BfRtDevTableData : public BfRtTableDataObj {
 public:
  BfRtDevTableData(const BfRtTableObj *tbl_obj,
                   const std::vector<bf_rt_id_t> &fields)
      : BfRtTableDataObj(tbl_obj) {
    this->setActiveFields(fields);
  }

  ~BfRtDevTableData() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const bool &value) override final;
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint64_t &value) override final;
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const std::string &str) override final;
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const std::vector<uint32_t> &arr) override final;
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t * /*value*/,
                       const size_t & /*size*/) override final;
  bf_status_t getValue(const bf_rt_id_t &field_id,
                       bool *value) const override final;
  bf_status_t getValue(const bf_rt_id_t &field_id,
                       uint64_t *value) const override final;
  bf_status_t getValue(const bf_rt_id_t &field_id,
                       std::string *str) const override final;
  bf_status_t getValue(const bf_rt_id_t &field_id,
                       std::vector<uint32_t> *arr) const override final;
  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value_ptr) const override final;

  bf_status_t reset() override final;
  bf_status_t reset(const std::vector<bf_rt_id_t> &fields) override final;

  const std::unordered_map<bf_rt_id_t, bool> &getBoolFieldDataMap() const {
    return boolFieldData;
  }
  const std::unordered_map<bf_rt_id_t, uint32_t> &getU32FieldDataMap() const {
    return u32FieldData;
  }
  const std::vector<uint32_t> &getListFieldData(bf_rt_id_t field_id) const {
    return listFieldData.at(field_id);
  }
  void setListFieldData(bf_rt_id_t field_id, const std::vector<uint32_t> &id) {
    listFieldData.insert(
        std::pair<bf_rt_id_t, std::vector<uint32_t>>(field_id, id));
  }

 private:
  bf_status_t setValueInternal(const bf_rt_id_t &field_id,
                               const uint64_t &value,
                               const uint8_t *value_ptr,
                               const size_t &s);
  bf_status_t getValueInternal(const bf_rt_id_t &field_id,
                               uint64_t *value,
                               uint8_t *value_ptr,
                               const size_t &s) const;
  std::unordered_map<bf_rt_id_t, bool> boolFieldData;
  std::unordered_map<bf_rt_id_t, uint32_t> u32FieldData;
  std::unordered_map<bf_rt_id_t, std::string> strFieldData;
  std::map<bf_rt_id_t, std::vector<uint32_t>> listFieldData;
};

class BfRtDevP4PipelinesTableData : public BfRtTableDataObj {
 public:
  BfRtDevP4PipelinesTableData(const BfRtTableObj *tbl_obj,
                              bf_rt_id_t act_id,
                              bf_rt_id_t container_id)
      : BfRtTableDataObj(tbl_obj, act_id, container_id){};

  ~BfRtDevP4PipelinesTableData() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const std::string &str) override final;
  bf_status_t getValue(const bf_rt_id_t &field_id,
                       std::string *str) const override final;
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const std::vector<uint32_t> &arr) override final;
  bf_status_t getValue(const bf_rt_id_t &field_id,
                       std::vector<uint32_t> *arr) const override final;

  const std::string &getContextFile() const { return runtime_context_file; }
  const std::string &getBinaryPath() const { return binary_path; }
  const std::string &getPipelineName() const { return pipeline_name; }
  const std::vector<uint32_t> &getPipeScope() const { return pipe_scope; }

  // setters/getters for paths
  void setContextPath(std::string path) { context_path = path; }
  void setCfgFilePath(std::string path) { cfg_file_path = path; }
  const std::string &getContextPath() const { return context_path; }
  const std::string &getCfgFilePath() const { return cfg_file_path; }

  bf_p4_pipeline_t to_bf_p4_pipeline() const;

 private:
  std::string runtime_context_file;
  std::string binary_path;
  std::string pipeline_name;
  std::vector<uint32_t> pipe_scope;

  // paths, not data fields;
  std::string context_path;
  std::string cfg_file_path;
};

class BfRtDevP4ProgramsTableData : public BfRtTableDataObj {
 public:
  BfRtDevP4ProgramsTableData(const BfRtTableObj *tbl_obj,
                             bf_rt_id_t act_id,
                             bf_rt_id_t container_id)
      : BfRtTableDataObj(tbl_obj, act_id, container_id){};

  ~BfRtDevP4ProgramsTableData() = default;

  bf_status_t setValue(
      const bf_rt_id_t &field_id,
      std::vector<std::unique_ptr<BfRtTableData>> container_v) override final;
  bf_status_t getValue(
      const bf_rt_id_t &field_id,
      std::vector<BfRtTableData *> *container_v) const override final;
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const std::string &str) override final;
  bf_status_t getValue(const bf_rt_id_t &field_id,
                       std::string *str) const override final;

  const std::string &getP4Name() const { return p4_name; }
  const std::string &getBfrtJsonFile() const { return bfrt_json_file; }
  const std::vector<std::unique_ptr<BfRtTableData>> &getP4PipelinesCtrData()
      const {
    return p4PipelinesContainerData;
  }

  void setBfrtPath(const std::string &path) { bf_rt_path = path; }
  const std::string &getBfrtPath() const { return bf_rt_path; }

  bf_p4_program_t to_bf_p4_program() const;

 private:
  std::string p4_name;
  std::string bfrt_json_file;
  // points to the BfRtDevP4PipelinesTableData object
  std::vector<std::unique_ptr<BfRtTableData>> p4PipelinesContainerData;

  // paths, not data fields
  std::string bf_rt_path;
};

class BfRtDevWarmInitTableData : public BfRtTableDataObj {
 public:
  BfRtDevWarmInitTableData(const BfRtTableObj *tbl_obj,
                           const bf_rt_id_t &act_id,
                           const std::vector<bf_rt_id_t> &fields)
      : BfRtTableDataObj(tbl_obj, act_id) {
    this->setActiveFields(fields);
  }

  BfRtDevWarmInitTableData(const BfRtTableObj *tbl_obj,
                           const bf_rt_id_t &act_id,
                           bf_rt_id_t container_id)
      : BfRtTableDataObj(tbl_obj, act_id, container_id) {}

  ~BfRtDevWarmInitTableData() = default;

  bf_status_t setValue(
      const bf_rt_id_t &field_id,
      std::vector<std::unique_ptr<BfRtTableData>> container_v) override final;
  bf_status_t getValue(
      const bf_rt_id_t &field_id,
      std::vector<BfRtTableData *> *container_v) const override final;
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const bool &value) override final;
  bf_status_t getValue(const bf_rt_id_t &field_id,
                       bool *value) const override final;
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const std::string &str) override final;
  bf_status_t getValue(const bf_rt_id_t &field_id,
                       std::string *str) const override final;

  // unexposed API
  const bool &getUpgradeAgents() const { return upgrade_agents; }
  const std::string &getInitMode() const { return init_mode; }

  bf_status_t containerToProfile(bf_dev_id_t dev_id,
                                 bf_device_profile_t *out) const;

 private:
  bool error_state = false;
  bool upgrade_agents = false;
  std::string init_mode;
  // points to the BfRtDevP4ProgramsTableData object
  std::vector<std::unique_ptr<BfRtTableData>> p4ProgramsContainerData;
};

}  // namespace bfrt
#endif  // _BF_RT_DEV_TABLE_DATA_IMPL_HPP
