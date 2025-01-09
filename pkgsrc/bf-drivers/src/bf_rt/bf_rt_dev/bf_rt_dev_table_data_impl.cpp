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


#include <arpa/inet.h>
#include <inttypes.h>
#include <regex>

#include <bf_rt_common/bf_rt_table_field_utils.hpp>
#include <bf_rt_common/bf_rt_utils.hpp>
#include <fstream>
#include "bf_rt_dev_table_data_impl.hpp"

#include <bf_rt/bf_rt_init.hpp>

namespace bfrt {

bf_status_t BfRtDevTableData::reset(const std::vector<bf_rt_id_t> &fields) {
  u32FieldData.clear();
  boolFieldData.clear();
  strFieldData.clear();
  return this->setActiveFields(fields);
}

bf_status_t BfRtDevTableData::reset() {
  std::vector<bf_rt_id_t> emptyfield;
  return this->reset(emptyfield);
}

bf_status_t BfRtDevTableData::setValue(const bf_rt_id_t &field_id,
                                       const bool &value) {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;

  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_TRACE("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  // Next, check if this setter can be used
  if (tableDataField->getDataType() != DataType::BOOL) {
    LOG_TRACE(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a bool",
        __func__,
        __LINE__,
        field_id,
        this->table_->table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }

  if (!this->checkFieldActive(field_id)) {
    LOG_TRACE("ERROR: %s:%d Set inactive field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  boolFieldData[field_id] = value;

  return BF_SUCCESS;
}

bf_status_t BfRtDevTableData::setValueInternal(const bf_rt_id_t &field_id,
                                               const uint64_t &value,
                                               const uint8_t *value_ptr,
                                               const size_t &s) {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;

  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_TRACE("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  // Do bounds checking using the utility functions
  auto sts = utils::BfRtTableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, &value, value_ptr, s);
  if (sts != BF_SUCCESS) {
    LOG_TRACE(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        tableDataField->getId());
    return sts;
  }

  sts = utils::BfRtTableFieldUtils::boundsCheck(
      *this->table_, *tableDataField, value, value_ptr, s);
  if (sts != BF_SUCCESS) {
    LOG_TRACE(
        "ERROR: %s:%d %s : Input Param bounds check failed for field id %d ",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        tableDataField->getId());
    return sts;
  }

  if (!this->checkFieldActive(field_id)) {
    LOG_TRACE("ERROR: %s:%d Set inactive field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  if (s > sizeof(uint32_t)) {
    LOG_TRACE(
        "ERROR: %s:%d Setting field id %d for table %s of size > 4B is not "
        "supported",
        __func__,
        __LINE__,
        field_id,
        this->table_->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  uint64_t val = 0;
  if (value_ptr) {
    utils::BfRtTableFieldUtils::toHostOrderData(
        *tableDataField, value_ptr, &val);
  } else {
    val = value;
  }

  u32FieldData[field_id] = static_cast<uint32_t>(val);

  return BF_SUCCESS;
}

bf_status_t BfRtDevTableData::setValue(const bf_rt_id_t &field_id,
                                       const std::vector<uint32_t> &arr) {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;

  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return status;
  }

  // Next, check if this setter can be used
  if (!tableDataField->isIntArr()) {
    LOG_TRACE(
        "%s:%d %s ERROR : This setter cannot be used for field id %d, since "
        "the field is not an integer array",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        field_id);
    return BF_NOT_SUPPORTED;
  }

  this->setListFieldData(tableDataField->getId(), arr);

  return BF_SUCCESS;
}

bf_status_t BfRtDevTableData::getValue(const bf_rt_id_t &field_id,
                                       std::vector<bf_rt_id_t> *arr) const {
  const BfRtTableDataField *tableDataField = nullptr;

  // Get the data_field from the table
  auto status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return status;
  }

  // Next, check if this getter can be used
  if (!tableDataField->isIntArr()) {
    LOG_TRACE(
        "%s:%d %s ERROR : This getter cannot be used for field id %d, since "
        "the field is not an integer array",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        field_id);
    return BF_NOT_SUPPORTED;
  }

  *arr = this->getListFieldData(tableDataField->getId());

  return BF_SUCCESS;
}

bf_status_t BfRtDevTableData::setValue(const bf_rt_id_t &field_id,
                                       const uint64_t &value) {
  return this->setValueInternal(field_id, value, nullptr, 0);
}

bf_status_t BfRtDevTableData::setValue(const bf_rt_id_t &field_id,
                                       const uint8_t *value,
                                       const size_t &size) {
  return this->setValueInternal(field_id, 0, value, size);
}

bf_status_t BfRtDevTableData::setValue(const bf_rt_id_t &field_id,
                                       const std::string &str) {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;

  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_TRACE("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  // Next, check if this setter can be used
  if (tableDataField->getDataType() != DataType::STRING) {
    LOG_TRACE(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a string or enum",
        __func__,
        __LINE__,
        field_id,
        this->table_->table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }

  if (!this->checkFieldActive(field_id)) {
    LOG_TRACE("ERROR: %s:%d Set inactive field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  strFieldData[field_id] = str;

  return BF_SUCCESS;
}

bf_status_t BfRtDevTableData::getValue(const bf_rt_id_t &field_id,
                                       bool *value) const {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;

  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_TRACE("ERROR %s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }

  // Next, check if this getter can be used
  if (tableDataField->getDataType() != DataType::BOOL) {
    LOG_TRACE(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a bool",
        __func__,
        __LINE__,
        field_id,
        this->table_->table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }

  auto elem = boolFieldData.find(field_id);
  if (elem == boolFieldData.end()) {
    LOG_TRACE("ERROR: %s:%d %s : field id %d has no valid data",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }

  *value = elem->second;

  return BF_SUCCESS;
}

bf_status_t BfRtDevTableData::getValue(const bf_rt_id_t &field_id,
                                       const size_t &size,
                                       uint8_t *value_ptr) const {
  return this->getValueInternal(field_id, nullptr, value_ptr, size);
}

bf_status_t BfRtDevTableData::getValueInternal(const bf_rt_id_t &field_id,
                                               uint64_t *value,
                                               uint8_t *value_ptr,
                                               const size_t &s) const {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;

  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_TRACE("ERROR %s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }

  status = utils::BfRtTableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, value, value_ptr, s);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        tableDataField->getId());
    return status;
  }
  auto elem = u32FieldData.find(field_id);

  if (elem == u32FieldData.end()) {
    LOG_TRACE("ERROR: %s:%d %s : field id %d has no valid data",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }

  if (value_ptr) {
    utils::BfRtTableFieldUtils::toNetworkOrderData(
        *tableDataField, elem->second, value_ptr);
  } else if (value) {
    *value = elem->second;
  }

  return status;
}

bf_status_t BfRtDevTableData::getValue(const bf_rt_id_t &field_id,
                                       uint64_t *value) const {
  return this->getValueInternal(field_id, value, nullptr, 0);
}

bf_status_t BfRtDevTableData::getValue(const bf_rt_id_t &field_id,
                                       std::string *str) const {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;

  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_TRACE("ERROR %s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }

  // Next, check if this getter can be used
  if (tableDataField->getDataType() != DataType::STRING) {
    LOG_TRACE(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a string nor enum",
        __func__,
        __LINE__,
        field_id,
        this->table_->table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }

  auto elem = strFieldData.find(field_id);
  if (elem == strFieldData.end()) {
    LOG_TRACE("ERROR: %s:%d %s : field id %d has no valid data",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }

  *str = elem->second;

  return BF_SUCCESS;
}

bf_status_t BfRtDevWarmInitTableData::setValue(const bf_rt_id_t &field_id,
                                               const bool &value) {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;
  status = this->table_->getDataField(
      field_id, this->actionIdGet_(), &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  // Next, check if this setter can be used
  if (tableDataField->getDataType() != DataType::BOOL) {
    LOG_ERROR(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a bool",
        __func__,
        __LINE__,
        field_id,
        this->table_->table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }

  if (field_id == ERROR)
    error_state = value;
  else if (field_id == UPGRADE_AGENTS)
    upgrade_agents = value;
  else {
    LOG_ERROR(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a bool",
        __func__,
        __LINE__,
        field_id,
        this->table_->table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtDevWarmInitTableData::getValue(const bf_rt_id_t &field_id,
                                               bool *value) const {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;
  status = this->table_->getDataField(
      field_id, this->actionIdGet_(), &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR %s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }
  // Next, check if this getter can be used
  if (tableDataField->getDataType() != DataType::BOOL) {
    LOG_ERROR(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a bool",
        __func__,
        __LINE__,
        field_id,
        this->table_->table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }

  if (field_id == ERROR)
    *value = error_state;
  else if (field_id == UPGRADE_AGENTS)
    *value = upgrade_agents;
  else {
    LOG_ERROR(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a bool",
        __func__,
        __LINE__,
        field_id,
        this->table_->table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtDevWarmInitTableData::setValue(const bf_rt_id_t &field_id,
                                               const std::string &str) {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;
  status = this->table_->getDataField(
      field_id, this->actionIdGet_(), &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  // Next, check if this setter can be used
  if (tableDataField->getDataType() != DataType::STRING) {
    LOG_ERROR(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a string or enum",
        __func__,
        __LINE__,
        field_id,
        this->table_->table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }
  /*auto tmp = string_to_portdown_reply_mode_map.find(str);
  if (tmp == string_to_portdown_reply_mode_map.end()) {
    LOG_ERROR("%s:%d Invalid port down replay mode %s",
              __func__,
              __LINE__,
              str.c_str());
    return BF_NOT_SUPPORTED;
  }
  bf_mode_ = tmp->second;*/
  init_mode = str;
  return BF_SUCCESS;
}

bf_status_t BfRtDevWarmInitTableData::getValue(const bf_rt_id_t &field_id,
                                               std::string *str) const {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;

  status = this->table_->getDataField(
      field_id, this->actionIdGet_(), &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR %s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }
  // Next, check if this getter can be used
  if (tableDataField->getDataType() != DataType::STRING) {
    LOG_ERROR(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a string nor enum",
        __func__,
        __LINE__,
        field_id,
        this->table_->table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }

  *str = init_mode;
  return BF_SUCCESS;
}

static std::string createPathBfrt(std::string base_path,
                                  std::string p4_name_to_use) {
  return base_path + p4_name_to_use + "/bf-rt.json";
}

static std::string createPathContext(std::string base_path,
                                     std::string p4_name_to_use,
                                     std::string profile_name) {
  return base_path + p4_name_to_use + "/" + profile_name + "/context.json";
}

static std::string createPathTofino(std::string base_path,
                                    std::string p4_name_to_use,
                                    std::string profile_name) {
  // Get tofino version folder name e.g. tofino, tofino2, tofino3 etc.
  const std::regex reg("(tofino[[:digit:]])|(tofino)");
  std::smatch version_match;
  std::regex_search(base_path, version_match, reg);
  const std::string tof = version_match.ready() ? version_match[0].str() : "";
  return base_path + p4_name_to_use + "/" + profile_name + "/" + tof + ".bin";
}

static bf_status_t getBasePath(const bf_dev_id_t &dev_id,
                               std::string &base_path) {
  std::vector<std::reference_wrapper<const std::string>> p4_names;
  auto &dev_mgr = BfRtDevMgr::getInstance();
  bf_status_t status = dev_mgr.bfRtInfoP4NamesGet(dev_id, p4_names);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d bfRtInfoP4NamesGet() failed", __func__, __LINE__);
    return status;
  }

  const std::string &prog_name = p4_names.back().get();
  const BfRtInfo *bfrt_info;
  status = dev_mgr.bfRtInfoGet(dev_id, prog_name, &bfrt_info);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d bfRtInfoGet() failed", __func__, __LINE__);
    return status;
  }

  // Get the bf-rt json path
  std::vector<std::reference_wrapper<const std::string>> bfrt_info_path_vec;
  status = bfrt_info->bfRtInfoFilePathGet(&bfrt_info_path_vec);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d bfRtInfoFilePathGet() failed", __func__, __LINE__);
    return status;
  }

  const std::string &path = bfrt_info_path_vec.back().get();
  const size_t found = path.find(prog_name);
  if (found == std::string::npos) return BF_UNEXPECTED;
  base_path = path.substr(0, found);

  return status;
}

bf_status_t BfRtDevWarmInitTableData::containerToProfile(
    bf_dev_id_t dev_id, bf_device_profile_t *out) const {
  int p4_cnt = 0;
  bf_status_t status = BF_SUCCESS;

  auto file_exists = [](const std::string &path) -> bool {
    return std::ifstream(path).good();
  };

  std::string base_path;
  status = getBasePath(dev_id, base_path);
  if (status != BF_SUCCESS) return status;

  for (auto &l : this->p4ProgramsContainerData) {
    auto leaf = static_cast<BfRtDevP4ProgramsTableData *>(l.get());
    const auto &p4_name = leaf->getP4Name();

    auto bfrt_path = createPathBfrt(base_path, p4_name);
    if (!file_exists(bfrt_path)) {
      LOG_ERROR("ERROR: %s:%d File not found: %s",
                __func__,
                __LINE__,
                bfrt_path.c_str());
      return BF_INVALID_ARG;
    }
    leaf->setBfrtPath(bfrt_path);

    for (auto &k : leaf->getP4PipelinesCtrData()) {
      auto leaf_pipe = static_cast<BfRtDevP4PipelinesTableData *>(k.get());
      const auto &pipeline_name = leaf_pipe->getPipelineName();

      auto context_path = createPathContext(base_path, p4_name, pipeline_name);
      if (!file_exists(context_path)) {
        LOG_ERROR("ERROR: %s:%d File not found: %s",
                  __func__,
                  __LINE__,
                  context_path.c_str());
        return BF_INVALID_ARG;
      }
      leaf_pipe->setContextPath(context_path);

      auto cfg_file_path = createPathTofino(base_path, p4_name, pipeline_name);
      if (!file_exists(cfg_file_path)) {
        LOG_ERROR("ERROR: %s:%d File not found: %s",
                  __func__,
                  __LINE__,
                  cfg_file_path.c_str());
        return BF_INVALID_ARG;
      }
      leaf_pipe->setCfgFilePath(cfg_file_path);
    }

    out->p4_programs[p4_cnt] = leaf->to_bf_p4_program();
    ++p4_cnt;
  }

  out->num_p4_programs = p4_cnt;

  return status;
}

bf_status_t BfRtDevWarmInitTableData::setValue(
    const bf_rt_id_t &field_id,
    std::vector<std::unique_ptr<BfRtTableData>> container_v) {
  if (container_v.size() > MAX_PROGRAMS_PER_DEVICE) {
    LOG_ERROR(
        "ERROR: %s:%d field id %d for table has to many programs per device",
        __func__,
        __LINE__,
        field_id);
    return BF_INVALID_ARG;
  }
  if (field_id != P4_PROGRAMS) {
    LOG_ERROR("ERROR: %s:%d Wrong field id %d", __func__, __LINE__, field_id);
    return BF_INVALID_ARG;
  }

  for (auto &l : container_v)
    this->p4ProgramsContainerData.push_back(std::move(l));

  return BF_SUCCESS;
}

bf_status_t BfRtDevWarmInitTableData::getValue(
    const bf_rt_id_t &field_id,
    std::vector<BfRtTableData *> *container_v) const {
  if (field_id != P4_PROGRAMS) {
    LOG_ERROR("ERROR: %s:%d Wrong field id %d", __func__, __LINE__, field_id);
    return BF_INVALID_ARG;
  }

  for (auto &l : this->p4ProgramsContainerData) container_v->push_back(l.get());

  return BF_SUCCESS;
}

bf_status_t BfRtDevP4ProgramsTableData::setValue(const bf_rt_id_t &field_id,
                                                 const std::string &str) {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;
  status = this->table_->getDataField(
      field_id, this->actionIdGet_(), &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  // Next, check if this setter can be used
  if (tableDataField->getDataType() != DataType::STRING) {
    LOG_ERROR(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a string or enum",
        __func__,
        __LINE__,
        field_id,
        this->table_->table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }
  /*auto tmp = string_to_portdown_reply_mode_map.find(str);
  if (tmp == string_to_portdown_reply_mode_map.end()) {
    LOG_ERROR("%s:%d Invalid port down replay mode %s",
              __func__,
              __LINE__,
              str.c_str());
    return BF_NOT_SUPPORTED;
  }
  bf_mode_ = tmp->second;*/
  if (field_id == P4_NAME)
    this->p4_name = str;
  else if (field_id == BFRT_JSON_FILE)
    this->bfrt_json_file = str;
  else {
    LOG_ERROR("ERROR: %s:%d Wrong field id %d", __func__, __LINE__, field_id);
    return BF_INVALID_ARG;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtDevP4ProgramsTableData::getValue(const bf_rt_id_t &field_id,
                                                 std::string *str) const {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;

  status = this->table_->getDataField(
      field_id, this->actionIdGet_(), &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR %s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }
  // Next, check if this getter can be used
  if (tableDataField->getDataType() != DataType::STRING) {
    LOG_ERROR(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a string nor enum",
        __func__,
        __LINE__,
        field_id,
        this->table_->table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }

  if (field_id == P4_NAME)
    *str = this->p4_name;
  else if (field_id == BFRT_JSON_FILE)
    *str = this->bfrt_json_file;
  else {
    LOG_ERROR("ERROR: %s:%d Wrong field id %d", __func__, __LINE__, field_id);
    return BF_INVALID_ARG;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtDevP4ProgramsTableData::setValue(
    const bf_rt_id_t &field_id,
    std::vector<std::unique_ptr<BfRtTableData>> container_v) {
  if (field_id != P4_PIPELINES) {
    LOG_ERROR("ERROR: %s:%d Wrong field id %d", __func__, __LINE__, field_id);
    return BF_INVALID_ARG;
  }
  if (container_v.size() > MAX_P4_PIPELINES) {
    LOG_ERROR("ERROR: %s:%d Wrong field id %d", __func__, __LINE__, field_id);
    return BF_INVALID_ARG;
  }

  for (auto &l : container_v)
    this->p4PipelinesContainerData.push_back(std::move(l));

  return BF_SUCCESS;
}

bf_status_t BfRtDevP4ProgramsTableData::getValue(
    const bf_rt_id_t &field_id,
    std::vector<BfRtTableData *> *container_v) const {
  if (field_id != P4_PIPELINES) {
    LOG_ERROR("ERROR: %s:%d Wrong field id %d", __func__, __LINE__, field_id);
    return BF_INVALID_ARG;
  }

  for (auto &l : this->p4PipelinesContainerData)
    container_v->push_back(l.get());

  return BF_SUCCESS;
}

bf_p4_program BfRtDevP4ProgramsTableData::to_bf_p4_program() const {
  bf_p4_program program = {};
  std::snprintf(program.prog_name, PROG_NAME_LEN, "%s", p4_name.c_str());
  program.bfrt_json_file = const_cast<char *>(bf_rt_path.c_str());
  const auto &pipe_data = p4PipelinesContainerData;
  program.num_p4_pipelines = pipe_data.size();
  auto f = [](const std::unique_ptr<BfRtTableData> &v) {
    auto *data = static_cast<BfRtDevP4PipelinesTableData *>(v.get());
    return data->to_bf_p4_pipeline();
  };
  std::transform(pipe_data.begin(), pipe_data.end(), program.p4_pipelines, f);
  return program;
}

bf_status_t BfRtDevP4PipelinesTableData::setValue(const bf_rt_id_t &field_id,
                                                  const std::string &str) {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;
  status = this->table_->getDataField(
      field_id, this->actionIdGet_(), &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  // Next, check if this setter can be used
  if (tableDataField->getDataType() != DataType::STRING) {
    LOG_ERROR(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a string or enum",
        __func__,
        __LINE__,
        field_id,
        this->table_->table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }

  if (field_id == RUNTIME_CTX_FILE)
    runtime_context_file = str;
  else if (field_id == BINARY_PATH)
    binary_path = str;
  else if (field_id == PIPELINE_NAME)
    pipeline_name = str;
  else {
    LOG_ERROR("ERROR: %s:%d Wrong field id %d", __func__, __LINE__, field_id);
    return BF_INVALID_ARG;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtDevP4PipelinesTableData::getValue(const bf_rt_id_t &field_id,
                                                  std::string *str) const {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;

  status = this->table_->getDataField(
      field_id, this->actionIdGet_(), &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR %s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }
  // Next, check if this getter can be used
  if (tableDataField->getDataType() != DataType::STRING) {
    LOG_ERROR(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a string nor enum",
        __func__,
        __LINE__,
        field_id,
        this->table_->table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }

  if (field_id == RUNTIME_CTX_FILE)
    *str = runtime_context_file;
  else if (field_id == BINARY_PATH)
    *str = binary_path;
  else if (field_id == PIPELINE_NAME)
    *str = pipeline_name;
  else {
    LOG_ERROR("ERROR: %s:%d Wrong field id %d", __func__, __LINE__, field_id);
    return BF_INVALID_ARG;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtDevP4PipelinesTableData::setValue(
    const bf_rt_id_t &field_id, const std::vector<uint32_t> &arr) {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;

  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return status;
  }

  // Next, check if this setter can be used
  if (!tableDataField->isIntArr()) {
    LOG_TRACE(
        "%s:%d %s ERROR : This setter cannot be used for field id %d, since "
        "the field is not an integer array",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        field_id);
    return BF_NOT_SUPPORTED;
  }

  if (arr.size() > MAX_P4_PIPELINES) {
    LOG_ERROR("ERROR: %s:%d Field id %d size is greater than %d",
              __func__,
              __LINE__,
              field_id,
              MAX_P4_PIPELINES);
    return BF_INVALID_ARG;
  }

  if (field_id != PIPE_SCOPE) {
    LOG_ERROR("ERROR: %s:%d Wrong field id %d", __func__, __LINE__, field_id);
    return BF_INVALID_ARG;
  }

  pipe_scope = arr;

  return BF_SUCCESS;
}

bf_status_t BfRtDevP4PipelinesTableData::getValue(
    const bf_rt_id_t &field_id, std::vector<bf_rt_id_t> *arr) const {
  const BfRtTableDataField *tableDataField = nullptr;

  // Get the data_field from the table
  auto status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return status;
  }

  // Next, check if this getter can be used
  if (!tableDataField->isIntArr()) {
    LOG_TRACE(
        "%s:%d %s ERROR : This getter cannot be used for field id %d, since "
        "the field is not an integer array",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        field_id);
    return BF_NOT_SUPPORTED;
  }

  if (field_id != PIPE_SCOPE) {
    LOG_ERROR("ERROR: %s:%d Wrong field id %d", __func__, __LINE__, field_id);
    return BF_INVALID_ARG;
  }

  *arr = pipe_scope;

  return BF_SUCCESS;
}

bf_p4_pipeline_t BfRtDevP4PipelinesTableData::to_bf_p4_pipeline() const {
  bf_p4_pipeline_t pipeline = {};
  const char *name = pipeline_name.c_str();
  std::snprintf(pipeline.p4_pipeline_name, PROG_NAME_LEN, "%s", name);
  pipeline.cfg_file = const_cast<char *>(cfg_file_path.c_str());
  pipeline.runtime_context_file = const_cast<char *>(context_path.c_str());
  pipeline.pi_config_file = nullptr;
  pipeline.num_pipes_in_scope = pipe_scope.size();
  for (int i = 0; i < pipeline.num_pipes_in_scope; ++i)
    pipeline.pipe_scope[i] = pipe_scope[i];
  return pipeline;
}
}  // namespace bfrt
