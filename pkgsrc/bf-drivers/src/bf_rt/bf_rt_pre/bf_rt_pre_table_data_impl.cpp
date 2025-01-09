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


#ifdef __cplusplus
extern "C" {
#endif
#include <arpa/inet.h>
#include <inttypes.h>
#ifdef __cplusplus
}
#endif

#include <bf_rt/bf_rt_table_data.hpp>
#include "bf_rt_pre_table_impl.hpp"
#include "bf_rt_pre_table_data_impl.hpp"

namespace bfrt {

bf_status_t BfRtPREMGIDTableData::setValue(const bf_rt_id_t &field_id,
                                           const std::vector<bf_rt_id_t> &arr) {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;

  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return status;
  }

  // Next, check if this setter can be used
  if (!tableDataField->isIntArr()) {
    LOG_ERROR(
        "%s:%d %s ERROR : This setter cannot be used for field id %d, since "
        "the field is not an integer array",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        field_id);
    return BF_NOT_SUPPORTED;
  }

  // For PRE data fields, there should be only one field type
  auto fieldTypes = tableDataField->getTypes();
  if (fieldTypes.size() != 1) {
    LOG_ERROR("%s:%d %s ERROR : More than one fieldType found for field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_UNEXPECTED;
  }

  // The size of node and ecmp L1 XIDs is 16bits (uint16_t). So, bounds check
  // needs to be done to make sure all XIDs are valid.
  for (const auto &fieldType : fieldTypes) {
    switch (fieldType) {
      case DataFieldType::MULTICAST_NODE_L1_XID:
      case DataFieldType::MULTICAST_ECMP_L1_XID: {
        for (auto l1_xid : arr) {
          status = utils::BfRtTableFieldUtils::boundsCheck(
              *this->table_,
              *tableDataField,
              l1_xid,
              NULL,
              tableDataField->getSize());
          if (status != BF_SUCCESS) {
            LOG_ERROR(
                "ERROR: %s:%d %s : Input Param bounds check failed for "
                "data field id %d",
                __func__,
                __LINE__,
                this->table_->table_name_get().c_str(),
                field_id);
            return status;
          }
        }
      } break;

      default:
        break;
    }
  }

  for (const auto &fieldType : fieldTypes) {
    switch (fieldType) {
      case DataFieldType::MULTICAST_NODE_ID:
        this->setMulticastNodeIds(arr);
        break;
      case DataFieldType::MULTICAST_NODE_L1_XID:
        this->setMulticastNodeL1XIds(arr);
        break;
      case DataFieldType::MULTICAST_ECMP_ID:
        this->setMulticastECMPIds(arr);
        break;
      case DataFieldType::MULTICAST_ECMP_L1_XID:
        this->setMulticastECMPL1XIds(arr);
        break;
      default:
        LOG_ERROR("%s:%d %s ERROR : Invalid Data Field type for field id %d",
                  __func__,
                  __LINE__,
                  this->table_->table_name_get().c_str(),
                  field_id);
        return BF_UNEXPECTED;
    }
  }

  return BF_SUCCESS;
}

bf_status_t BfRtPREMGIDTableData::setValue(const bf_rt_id_t &field_id,
                                           const std::vector<bool> &arr) {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;

  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return status;
  }

  // Next, check if this setter can be used
  if (!tableDataField->isBoolArr()) {
    LOG_ERROR(
        "%s:%d %s ERROR : This setter cannot be used for field id %d, since "
        "the field is not a bool array",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        field_id);

    return BF_NOT_SUPPORTED;
  }

  // For PRE data fields, there should be only one field type
  auto fieldTypes = tableDataField->getTypes();
  if (fieldTypes.size() != 1) {
    LOG_ERROR("%s:%d %s ERROR : More than one fieldType found for field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_UNEXPECTED;
  }

  for (const auto &fieldType : fieldTypes) {
    switch (fieldType) {
      case DataFieldType::MULTICAST_NODE_L1_XID_VALID:
        this->setMulticastNodeL1XIdsValid(arr);
        break;
      case DataFieldType::MULTICAST_ECMP_L1_XID_VALID:
        this->setMulticastECMPL1XIdsValid(arr);
        break;
      default:
        LOG_ERROR("%s:%d %s ERROR : Invalid Data Field type for field id %d",
                  __func__,
                  __LINE__,
                  this->table_->table_name_get().c_str(),
                  field_id);
        return BF_INVALID_ARG;
    }
  }

  return BF_SUCCESS;
}

bf_status_t BfRtPREMGIDTableData::getValue(const bf_rt_id_t &field_id,
                                           std::vector<bf_rt_id_t> *arr) const {
  const BfRtTableDataField *tableDataField = nullptr;

  // Get the data_field from the table
  auto status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return status;
  }

  // Next, check if this getter can be used
  if (!tableDataField->isIntArr()) {
    LOG_ERROR(
        "%s:%d %s ERROR : This getter cannot be used for field id %d, since "
        "the field is not an integer array",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        field_id);
    return BF_NOT_SUPPORTED;
  }

  // For PRE data fields, there should be only one field type
  auto fieldTypes = tableDataField->getTypes();
  if (fieldTypes.size() != 1) {
    LOG_ERROR("%s:%d %s ERROR : More than one fieldType found for field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_UNEXPECTED;
  }

  for (const auto &fieldType : fieldTypes) {
    switch (fieldType) {
      case DataFieldType::MULTICAST_NODE_ID:
        *arr = this->getMulticastNodeIds();
        break;
      case DataFieldType::MULTICAST_NODE_L1_XID:
        *arr = this->getMulticastNodeL1XIds();
        break;
      case DataFieldType::MULTICAST_ECMP_ID:
        *arr = this->getMulticastECMPIds();
        break;
      case DataFieldType::MULTICAST_ECMP_L1_XID:
        *arr = this->getMulticastECMPL1XIds();
        break;
      default:
        LOG_ERROR("%s:%d %s ERROR : Invalid Data Field type for field id %d",
                  __func__,
                  __LINE__,
                  this->table_->table_name_get().c_str(),
                  field_id);
        return BF_INVALID_ARG;
    }
  }

  return BF_SUCCESS;
}

bf_status_t BfRtPREMGIDTableData::getValue(const bf_rt_id_t &field_id,
                                           std::vector<bool> *arr) const {
  const BfRtTableDataField *tableDataField = nullptr;

  // Get the data_field from the table
  auto status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return status;
  }

  // Next, check if this getter can be used
  if (!tableDataField->isBoolArr()) {
    LOG_ERROR(
        "%s:%d %s ERROR : This getter cannot be used for field id %d since "
        "the field is not a bool array",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        field_id);
    return BF_NOT_SUPPORTED;
  }

  // For PRE data fields, there should be only one field type
  auto fieldTypes = tableDataField->getTypes();
  if (fieldTypes.size() != 1) {
    LOG_ERROR("%s:%d %s ERROR : More than one fieldType found for field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_UNEXPECTED;
  }

  for (const auto &fieldType : fieldTypes) {
    switch (fieldType) {
      case DataFieldType::MULTICAST_NODE_L1_XID_VALID:
        *arr = this->getMulticastNodeL1XIdsValid();
        break;
      case DataFieldType::MULTICAST_ECMP_L1_XID_VALID:
        *arr = this->getMulticastECMPL1XIdsValid();
        break;
      default:
        LOG_ERROR("%s:%d %s ERROR :  Invalid Data Field type for field id %d",
                  __func__,
                  __LINE__,
                  this->table_->table_name_get().c_str(),
                  field_id);
        return BF_INVALID_ARG;
    }
  }

  return BF_SUCCESS;
}

bf_status_t BfRtPREMGIDTableData::reset() {
  multicast_node_ids_.clear();
  node_l1_xids_valid_.clear();
  node_l1_xids_.clear();
  multicast_ecmp_ids_.clear();
  ecmp_l1_xids_valid_.clear();
  ecmp_l1_xids_.clear();

  return BF_SUCCESS;
}

bf_status_t BfRtPREMulticastNodeTableData::setValue(const bf_rt_id_t &field_id,
                                                    const uint64_t &value) {
  const BfRtTableDataField *tableDataField = nullptr;

  // Get the data_field from the table
  auto status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return status;
  }

  // For PRE data fields, there should be only one field type
  auto fieldTypes = tableDataField->getTypes();
  if (fieldTypes.size() != 1) {
    LOG_ERROR("%s:%d %s ERROR : More than one fieldType found for field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_NOT_SUPPORTED;
  }

  for (const auto &fieldType : fieldTypes) {
    if (fieldType != DataFieldType::MULTICAST_RID) {
      LOG_ERROR("%s:%d %s ERROR : Invalid Field type for field id %d",
                __func__,
                __LINE__,
                this->table_->table_name_get().c_str(),
                field_id);
      return BF_INVALID_ARG;
    }
  }

  // Perform bounds check as RID field is 16bits (uint16_t) and
  // value should be within limit
  status = utils::BfRtTableFieldUtils::boundsCheck(
      *this->table_, *tableDataField, value, NULL, tableDataField->getSize());
  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input Param bounds check failed for "
        "data field id %d",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        field_id);
    return status;
  }

  this->setRId(static_cast<bf_mc_rid_t>(value));

  return BF_SUCCESS;
}

bf_status_t BfRtPREMulticastNodeTableData::setValue(const bf_rt_id_t &field_id,
                                                    const uint8_t *value,
                                                    const size_t &size) {
  const BfRtTableDataField *tableDataField = nullptr;

  // Get the data_field from the table
  auto status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return status;
  }

  // For PRE data fields, there should be only one field type
  auto fieldTypes = tableDataField->getTypes();
  if (fieldTypes.size() != 1) {
    LOG_ERROR("%s:%d %s ERROR : More than one fieldType found for field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_NOT_SUPPORTED;
  }

  status = utils::BfRtTableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, 0, value, size);
  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        tableDataField->getId());
    return status;
  }

  for (const auto &fieldType : fieldTypes) {
    switch (fieldType) {
      case DataFieldType::MULTICAST_RID: {
        // Size is already validated
        uint64_t rid_val = 0;
        utils::BfRtTableFieldUtils::toHostOrderData(
            *tableDataField, value, &rid_val);
        this->setRId(static_cast<uint16_t>(rid_val));
      } break;

      case DataFieldType::MULTICAST_LAG_ID: {
        // Size is already validated
        this->setLAGBitmap(value);
      } break;

      case DataFieldType::DEV_PORT: {
        // Size is already validated
        this->setPortBitmap(value);
      } break;

      default:
        LOG_ERROR("%s:%d %s ERROR : Invalid Field type for field id %d",
                  __func__,
                  __LINE__,
                  this->table_->table_name_get().c_str(),
                  field_id);
        return BF_INVALID_ARG;
    }
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPREMulticastNodeTableData::getValue(const bf_rt_id_t &field_id,
                                                    uint64_t *value) const {
  const BfRtTableDataField *tableDataField = nullptr;

  // Get the data_field from the table
  auto status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return status;
  }

  // For PRE data fields, there should be only one field type
  auto fieldTypes = tableDataField->getTypes();
  if (fieldTypes.size() != 1) {
    LOG_ERROR("%s:%d %s ERROR : More than one fieldType found for field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_NOT_SUPPORTED;
  }

  for (const auto &fieldType : fieldTypes) {
    if (fieldType != DataFieldType::MULTICAST_RID) {
      LOG_ERROR("%s:%d %s ERROR : Invalid Field type for field id %d",
                __func__,
                __LINE__,
                this->table_->table_name_get().c_str(),
                field_id);
      return BF_INVALID_ARG;
    }
  }

  *value = this->getRId();
  return BF_SUCCESS;
}

bf_status_t BfRtPREMulticastNodeTableData::getValue(const bf_rt_id_t &field_id,
                                                    const size_t &size,
                                                    uint8_t *value) const {
  const BfRtTableDataField *tableDataField = nullptr;

  // Get the data_field from the table
  auto status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return status;
  }

  // For PRE data fields, there should be only one field type
  auto fieldTypes = tableDataField->getTypes();
  if (fieldTypes.size() != 1) {
    LOG_ERROR("%s:%d %s ERROR : More than one fieldType found for field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_NOT_SUPPORTED;
  }

  status = utils::BfRtTableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, 0, value, size);
  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        tableDataField->getId());
    return status;
  }

  for (const auto &fieldType : fieldTypes) {
    switch (fieldType) {
      case DataFieldType::MULTICAST_RID: {
        // Size is already validated
        uint64_t rid_val = this->getRId();
        utils::BfRtTableFieldUtils::toNetworkOrderData(
            *tableDataField, rid_val, value);
      } break;
      case DataFieldType::MULTICAST_LAG_ID: {
        // Size is already validated
        const uint8_t *bitmap = this->getLAGBitmap();
        std::memcpy(value, bitmap, size);
      } break;
      case DataFieldType::DEV_PORT: {
        // Size is already validated
        const uint8_t *bitmap = this->getPortBitmap();
        std::memcpy(value, bitmap, size);
      } break;
      default:
        LOG_ERROR("%s:%d %s ERROR : Invalid Field type for field id %d",
                  __func__,
                  __LINE__,
                  this->table_->table_name_get().c_str(),
                  field_id);
        return BF_INVALID_ARG;
    }
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPREMulticastNodeTableData::setValue(
    const bf_rt_id_t &field_id, const std::vector<bf_rt_id_t> &arr) {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;

  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return status;
  }

  // Next, check if this setter can be used
  if (!tableDataField->isIntArr()) {
    LOG_ERROR(
        "%s:%d %s ERROR : This setter cannot be used for field id %d, since "
        "the field is not an integer array",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        field_id);
    return BF_NOT_SUPPORTED;
  }

  // For PRE data fields, there should be only one field type
  auto fieldTypes = tableDataField->getTypes();
  if (fieldTypes.size() != 1) {
    LOG_ERROR("%s:%d %s ERROR : More than one fieldType found for field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_UNEXPECTED;
  }

  for (const auto &fieldType : fieldTypes) {
    switch (fieldType) {
      case DataFieldType::MULTICAST_LAG_ID:
      case DataFieldType::DEV_PORT: {
        for (auto id : arr) {
          status = utils::BfRtTableFieldUtils::boundsCheck(
              *this->table_,
              *tableDataField,
              id,
              NULL,
              tableDataField->getSize());
          if (status != BF_SUCCESS) {
            LOG_ERROR(
                "ERROR: %s:%d %s : Input Param bounds check failed for "
                "data field id %d",
                __func__,
                __LINE__,
                this->table_->table_name_get().c_str(),
                field_id);
            return status;
          }
        }
      } break;

      default:
        break;
    }
  }

  for (const auto &fieldType : fieldTypes) {
    switch (fieldType) {
      case DataFieldType::MULTICAST_LAG_ID:
        this->setMulticastLAGIDs(arr);
        break;
      case DataFieldType::DEV_PORT:
        this->setMulticastDevPorts(arr);
        break;
      default:
        LOG_ERROR("%s:%d %s ERROR : Invalid Data Field type for field id %d",
                  __func__,
                  __LINE__,
                  this->table_->table_name_get().c_str(),
                  field_id);
        return BF_UNEXPECTED;
    }
  }

  return BF_SUCCESS;
}

bf_status_t BfRtPREMulticastNodeTableData::getValue(
    const bf_rt_id_t &field_id, std::vector<bf_rt_id_t> *arr) const {
  const BfRtTableDataField *tableDataField = nullptr;

  // Get the data_field from the table
  auto status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return status;
  }

  // Next, check if this getter can be used
  if (!tableDataField->isIntArr()) {
    LOG_ERROR(
        "%s:%d %s ERROR : This getter cannot be used for field id %d since "
        "the field is not a int array",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        field_id);
    return BF_NOT_SUPPORTED;
  }

  // For PRE data fields, there should be only one field type
  auto fieldTypes = tableDataField->getTypes();
  if (fieldTypes.size() != 1) {
    LOG_ERROR("%s:%d %s ERROR : More than one fieldType found for field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_UNEXPECTED;
  }

  for (const auto &fieldType : fieldTypes) {
    switch (fieldType) {
      case DataFieldType::MULTICAST_LAG_ID:
        *arr = this->getMulticastLAGIDs();
        break;
      case DataFieldType::DEV_PORT:
        *arr = this->getMulticastDevPorts();
        break;
      default:
        LOG_ERROR("%s:%d %s ERROR :  Invalid Data Field type for field id %d",
                  __func__,
                  __LINE__,
                  this->table_->table_name_get().c_str(),
                  field_id);
        return BF_INVALID_ARG;
    }
  }

  return BF_SUCCESS;
}

bf_status_t BfRtPREMulticastNodeTableData::reset() {
  rid_ = 0;
  std::memset(port_bitmap_, 0, sizeof(bf_mc_port_map_t));
  std::memset(lag_bitmap_, 0, sizeof(bf_mc_lag_map_t));
  dev_port_list_.clear();
  lag_id_list_.clear();
  return BF_SUCCESS;
}

bf_status_t BfRtPREECMPTableData::setValue(const bf_rt_id_t &field_id,
                                           const std::vector<bf_rt_id_t> &arr) {
  const BfRtTableDataField *tableDataField = nullptr;

  auto status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return status;
  }

  // Next, check if this setter can be used
  if (!tableDataField->isIntArr()) {
    LOG_ERROR(
        "%s:%d %s ERROR : This setter cannot be used for field id %d, since "
        "the field is not an integer array",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        field_id);
    return BF_NOT_SUPPORTED;
  }

  // For PRE data fields, there should be only one field type
  auto fieldTypes = tableDataField->getTypes();
  if (fieldTypes.size() != 1) {
    LOG_ERROR("%s:%d %s ERROR : More than one fieldType found for field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_UNEXPECTED;
  }

  for (const auto &fieldType : fieldTypes) {
    if (fieldType != DataFieldType::MULTICAST_NODE_ID) {
      LOG_ERROR("%s:%d %s ERROR : Invalid Data Field type for field id %d",
                __func__,
                __LINE__,
                this->table_->table_name_get().c_str(),
                field_id);
      return BF_INVALID_ARG;
    }
  }

  // No need for bound checks as full 32 bit ID is valid for node id
  this->setMulticastNodeIds(arr);

  return BF_SUCCESS;
}

bf_status_t BfRtPREECMPTableData::getValue(const bf_rt_id_t &field_id,
                                           std::vector<bf_rt_id_t> *arr) const {
  const BfRtTableDataField *tableDataField = nullptr;

  // Get the data_field from the table
  auto status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return status;
  }

  // Next, check if this getter can be used
  if (!tableDataField->isIntArr()) {
    LOG_ERROR(
        "%s:%d %s ERROR : This getter cannot be used for field id %d, since "
        "the field is not an integer array",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        field_id);
    return BF_NOT_SUPPORTED;
  }

  // For PRE data fields, there should be only one field type
  auto fieldTypes = tableDataField->getTypes();
  if (fieldTypes.size() != 1) {
    LOG_ERROR("%s:%d %s ERROR : More than one fieldType found for field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_UNEXPECTED;
  }

  for (const auto &fieldType : fieldTypes) {
    if (fieldType != DataFieldType::MULTICAST_NODE_ID) {
      LOG_ERROR("%s:%d %s ERROR : Invalid Data Field type for field id %d",
                __func__,
                __LINE__,
                this->table_->table_name_get().c_str(),
                field_id);
      return BF_INVALID_ARG;
    }
  }

  *arr = this->getMulticastNodeIds();

  return BF_SUCCESS;
}

bf_status_t BfRtPREECMPTableData::reset() {
  multicast_node_ids_.clear();

  return BF_SUCCESS;
}

bf_status_t BfRtPRELAGTableData::setValueInternal(const bf_rt_id_t &field_id,
                                                  const uint64_t &value,
                                                  const uint8_t *value_ptr,
                                                  const size_t &size) {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;

  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return status;
  }

  auto fieldTypes = tableDataField->getTypes();
  if (fieldTypes.size() != 1) {
    LOG_ERROR("%s:%d %s ERROR : More than one fieldType found for field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_UNEXPECTED;
  }
  //
  // Do type compatibility checks using the utility functions
  auto sts = utils::BfRtTableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, &value, value_ptr, size);
  if (sts != BF_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        tableDataField->getId());
    return sts;
  }

  // Do bounds checking using the utility functions
  sts = utils::BfRtTableFieldUtils::boundsCheck(
      *this->table_, *tableDataField, value, value_ptr, size);
  if (sts != BF_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input Param bounds check failed for field id %d ",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        tableDataField->getId());
    return sts;
  }

  if (!this->checkFieldActive(field_id)) {
    LOG_ERROR("ERROR: %s:%d Set inactive field id %d for table %s",
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
  for (const auto &fieldType : fieldTypes) {
    switch (fieldType) {
      case DataFieldType::MULTICAST_LAG_REMOTE_MSB_COUNT: {
        // Size is already validated
        this->lag_msb_count = val;
      } break;

      case DataFieldType::MULTICAST_LAG_REMOTE_LSB_COUNT: {
        // Size is already validated
        this->lag_lsb_count = val;
      } break;

      default:
        LOG_ERROR("%s:%d %s ERROR : Invalid Field type for field id %d",
                  __func__,
                  __LINE__,
                  this->table_->table_name_get().c_str(),
                  field_id);
        return BF_INVALID_ARG;
    }
  }
  return status;
}
bf_status_t BfRtPRELAGTableData::setValue(const bf_rt_id_t &field_id,
                                          const uint8_t *value,
                                          const size_t &size) {
  return setValueInternal(field_id, 0, value, size);
}
bf_status_t BfRtPRELAGTableData::setValue(const bf_rt_id_t &field_id,
                                          const uint64_t &value) {
  return setValueInternal(field_id, value, nullptr, 0);
}

bf_status_t BfRtPRELAGTableData::setValue(const bf_rt_id_t &field_id,
                                          const std::vector<bf_rt_id_t> &arr) {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;

  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return status;
  }

  // Next, check if this setter can be used
  if (!tableDataField->isIntArr()) {
    LOG_ERROR(
        "%s:%d %s ERROR : This setter cannot be used for field id %d, since "
        "the field is not an integer array",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        field_id);
    return BF_NOT_SUPPORTED;
  }

  // For PRE data fields, there should be only one field type
  auto fieldTypes = tableDataField->getTypes();
  if (fieldTypes.size() != 1) {
    LOG_ERROR("%s:%d %s ERROR : More than one fieldType found for field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_UNEXPECTED;
  }

  // The size of LAG_ID and DEV_PORT is uint8 and unit32 respectively,
  // needs to be done to make sure data is in range
  for (const auto &fieldType : fieldTypes) {
    switch (fieldType) {
      case DataFieldType::DEV_PORT: {
        for (auto id : arr) {
          status = utils::BfRtTableFieldUtils::boundsCheck(
              *this->table_,
              *tableDataField,
              id,
              NULL,
              tableDataField->getSize());
          if (status != BF_SUCCESS) {
            LOG_ERROR(
                "ERROR: %s:%d %s : Input Param bounds check failed for "
                "data field id %d",
                __func__,
                __LINE__,
                this->table_->table_name_get().c_str(),
                field_id);
            return status;
          }
        }
      } break;
      default:
        break;
    }
  }

  for (const auto &fieldType : fieldTypes) {
    switch (fieldType) {
      case DataFieldType::DEV_PORT:
        this->setMulticastDevPorts(arr);
        break;
      default:
        LOG_ERROR("%s:%d %s ERROR : Invalid Data Field type for field id %d",
                  __func__,
                  __LINE__,
                  this->table_->table_name_get().c_str(),
                  field_id);
        return BF_UNEXPECTED;
    }
  }

  return BF_SUCCESS;
}

bf_status_t BfRtPRELAGTableData::getValueInternal(const bf_rt_id_t &field_id,
                                                  uint64_t *value,
                                                  uint8_t *value_ptr,
                                                  const size_t &size) const {
  const BfRtTableDataField *tableDataField = nullptr;

  // Get the data_field from the table
  auto status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return status;
  }
  if (!value && !value_ptr) {
    LOG_ERROR(
        "ERROR: %s:%d %s : output param value cannot be NULL for"
        "data field %s",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        tableDataField->getName().c_str());
    return BF_INVALID_ARG;
  }
  auto fieldTypes = tableDataField->getTypes();
  if (fieldTypes.size() != 1) {
    LOG_ERROR("%s:%d %s ERROR : More than one fieldType found for field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_UNEXPECTED;
  }
  // Do type compatibility checks using the utility functions
  status = utils::BfRtTableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, value, value_ptr, size);
  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        tableDataField->getId());
    return status;
  }

  uint64_t val = 0;
  for (const auto &fieldType : fieldTypes) {
    switch (fieldType) {
      case DataFieldType::MULTICAST_LAG_REMOTE_MSB_COUNT: {
        val = this->lag_msb_count;
      } break;
      case DataFieldType::MULTICAST_LAG_REMOTE_LSB_COUNT: {
        val = this->lag_lsb_count;
      } break;
      default:
        LOG_ERROR("%s:%d %s ERROR : Invalid Field type for field id %d",
                  __func__,
                  __LINE__,
                  this->table_->table_name_get().c_str(),
                  field_id);
        return BF_INVALID_ARG;
    }
  }
  if (value_ptr) {
    utils::BfRtTableFieldUtils::toNetworkOrderData(
        *tableDataField, val, value_ptr);
  } else {
    *value = val;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPRELAGTableData::getValue(const bf_rt_id_t &field_id,
                                          const size_t &size,
                                          uint8_t *value) const {
  return getValueInternal(field_id, nullptr, value, size);
}

bf_status_t BfRtPRELAGTableData::getValue(const bf_rt_id_t &field_id,
                                          uint64_t *value) const {
  return getValueInternal(field_id, value, nullptr, 0);
}

bf_status_t BfRtPRELAGTableData::getValue(const bf_rt_id_t &field_id,
                                          std::vector<bf_rt_id_t> *arr) const {
  const BfRtTableDataField *tableDataField = nullptr;

  // Get the data_field from the table
  auto status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return status;
  }

  // Next, check if this getter can be used
  if (!tableDataField->isIntArr()) {
    LOG_ERROR(
        "%s:%d %s ERROR : This getter cannot be used for field id %d since "
        "the field is not a int array",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        field_id);
    return BF_NOT_SUPPORTED;
  }

  // For PRE data fields, there should be only one field type
  auto fieldTypes = tableDataField->getTypes();
  if (fieldTypes.size() != 1) {
    LOG_ERROR("%s:%d %s ERROR : More than one fieldType found for field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_UNEXPECTED;
  }

  for (const auto &fieldType : fieldTypes) {
    switch (fieldType) {
      case DataFieldType::DEV_PORT:
        *arr = this->getMulticastDevPorts();
        break;
      default:
        LOG_ERROR("%s:%d %s ERROR :  Invalid Data Field type for field id %d",
                  __func__,
                  __LINE__,
                  this->table_->table_name_get().c_str(),
                  field_id);
        return BF_INVALID_ARG;
    }
  }

  return BF_SUCCESS;
}

bf_status_t BfRtPRELAGTableData::reset() {
  std::memset(port_bitmap_, 0, sizeof(bf_mc_port_map_t));
  dev_port_list_.clear();
  return BF_SUCCESS;
}

bf_status_t BfRtPREMulticastPruneTableData::setValue(
    const bf_rt_id_t &field_id, const std::vector<bf_rt_id_t> &arr) {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;

  // Get the data_field from the table
  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return status;
  }

  // Next, check if this setter can be used
  if (!tableDataField->isIntArr()) {
    LOG_ERROR(
        "%s:%d %s ERROR : This setter cannot be used for field id %d, since "
        "the field is not an integer array",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        field_id);
    return BF_NOT_SUPPORTED;
  }

  // For PRE data fields, there should be only one field type
  auto fieldTypes = tableDataField->getTypes();
  if (fieldTypes.size() != 1) {
    LOG_ERROR("%s:%d %s ERROR : More than one fieldType found for field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_UNEXPECTED;
  }

  // The size of LAG_ID and DEV_PORT is uint8 and unit32 respectively,
  // needs to be done to make sure data is in range
  for (const auto &fieldType : fieldTypes) {
    switch (fieldType) {
      case DataFieldType::DEV_PORT: {
        for (auto id : arr) {
          status = utils::BfRtTableFieldUtils::boundsCheck(
              *this->table_,
              *tableDataField,
              id,
              NULL,
              tableDataField->getSize());
          if (status != BF_SUCCESS) {
            LOG_ERROR(
                "ERROR: %s:%d %s : Input Param bounds check failed for "
                "data field id %d",
                __func__,
                __LINE__,
                this->table_->table_name_get().c_str(),
                field_id);
            return status;
          }
        }
      } break;
      default:
        break;
    }
  }

  for (const auto &fieldType : fieldTypes) {
    switch (fieldType) {
      case DataFieldType::DEV_PORT:
        this->setMulticastDevPorts(arr);
        break;
      default:
        LOG_ERROR("%s:%d %s ERROR : Invalid Data Field type for field id %d",
                  __func__,
                  __LINE__,
                  this->table_->table_name_get().c_str(),
                  field_id);
        return BF_UNEXPECTED;
    }
  }

  return BF_SUCCESS;
}

bf_status_t BfRtPREMulticastPruneTableData::getValue(
    const bf_rt_id_t &field_id, std::vector<bf_rt_id_t> *arr) const {
  const BfRtTableDataField *tableDataField = nullptr;

  // Get the data_field from the table
  auto status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return status;
  }

  // Next, check if this getter can be used
  if (!tableDataField->isIntArr()) {
    LOG_ERROR(
        "%s:%d %s ERROR : This getter cannot be used for field id %d since "
        "the field is not a int array",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        field_id);
    return BF_NOT_SUPPORTED;
  }

  // For PRE data fields, there should be only one field type
  auto fieldTypes = tableDataField->getTypes();
  if (fieldTypes.size() != 1) {
    LOG_ERROR("%s:%d %s ERROR : More than one fieldType found for field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_UNEXPECTED;
  }

  for (const auto &fieldType : fieldTypes) {
    switch (fieldType) {
      case DataFieldType::DEV_PORT:
        *arr = this->getMulticastDevPorts();
        break;
      default:
        LOG_ERROR("%s:%d %s ERROR :  Invalid Data Field type for field id %d",
                  __func__,
                  __LINE__,
                  this->table_->table_name_get().c_str(),
                  field_id);
        return BF_INVALID_ARG;
    }
  }

  return BF_SUCCESS;
}

bf_status_t BfRtPREMulticastPruneTableData::reset() {
  std::memset(port_bitmap_, 0, sizeof(bf_mc_port_map_t));
  dev_port_list_.clear();
  return BF_SUCCESS;
}

bf_status_t BfRtPREMulticastPortTableData::setValue(const bf_rt_id_t &field_id,
                                                    const bool &value) {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;

  status = this->table_->getDataField(field_id, &tableDataField);
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

  if (!this->checkFieldActive(field_id)) {
    LOG_ERROR("ERROR: %s:%d Set inactive field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  boolFieldData[field_id] = value;

  return BF_SUCCESS;
}

bf_status_t BfRtPREMulticastPortTableData::setValueInternal(
    const bf_rt_id_t &field_id,
    const uint64_t &value,
    const uint8_t *value_ptr,
    const size_t &s) {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;

  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  // Do type compatibility checks using the utility functions
  auto sts = utils::BfRtTableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, &value, value_ptr, s);
  if (sts != BF_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        tableDataField->getId());
    return sts;
  }

  // Do bounds checking using the utility functions
  sts = utils::BfRtTableFieldUtils::boundsCheck(
      *this->table_, *tableDataField, value, value_ptr, s);
  if (sts != BF_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input Param bounds check failed for field id %d ",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        tableDataField->getId());
    return sts;
  }

  if (!this->checkFieldActive(field_id)) {
    LOG_ERROR("ERROR: %s:%d Set inactive field id %d for table %s",
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
  u64FieldData[field_id] = val;

  return BF_SUCCESS;
}

bf_status_t BfRtPREMulticastPortTableData::setValue(const bf_rt_id_t &field_id,
                                                    const uint64_t &value) {
  return setValueInternal(field_id, value, nullptr, 0);
}

bf_status_t BfRtPREMulticastPortTableData::setValue(const bf_rt_id_t &field_id,
                                                    const uint8_t *value,
                                                    const size_t &size) {
  return setValueInternal(field_id, 0, value, size);
}

bf_status_t BfRtPREMulticastPortTableData::getValue(const bf_rt_id_t &field_id,
                                                    bool *value) const {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;

  status = this->table_->getDataField(field_id, &tableDataField);
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

  auto elem = boolFieldData.find(field_id);
  if (elem == boolFieldData.end()) {
    LOG_ERROR("ERROR: %s:%d %s : field id %d has no valid data",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }

  *value = elem->second;

  return BF_SUCCESS;
}

bf_status_t BfRtPREMulticastPortTableData::getValueInternal(
    const bf_rt_id_t &field_id,
    uint64_t *value,
    uint8_t *value_ptr,
    const size_t &s) const {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;

  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR %s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }

  // Do type compatibility checks using the utility functions
  status = utils::BfRtTableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, value, value_ptr, s);
  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        tableDataField->getId());
    return status;
  }

  auto elem = u64FieldData.find(field_id);
  if (elem == u64FieldData.end()) {
    LOG_ERROR("ERROR: %s:%d %s : field id %d has no valid data",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  } else {
    uint64_t val = elem->second;
    if (value_ptr) {
      utils::BfRtTableFieldUtils::toNetworkOrderData(
          *tableDataField, val, value_ptr);
    } else {
      *value = val;
    }
  }

  return BF_SUCCESS;
}

bf_status_t BfRtPREMulticastPortTableData::getValue(const bf_rt_id_t &field_id,
                                                    uint64_t *value) const {
  return getValueInternal(field_id, value, nullptr, 0);
}

bf_status_t BfRtPREMulticastPortTableData::getValue(const bf_rt_id_t &field_id,
                                                    const size_t &size,
                                                    uint8_t *value) const {
  return getValueInternal(field_id, nullptr, value, size);
}

bf_status_t BfRtPREMulticastPortTableData::reset(
    const std::vector<bf_rt_id_t> &fields) {
  boolFieldData.clear();
  u64FieldData.clear();
  activeFields.clear();

  return this->set_active_fields(fields);
}

bf_status_t BfRtPREMulticastPortTableData::reset() {
  std::vector<bf_rt_id_t> fields;

  return this->reset(fields);
}

bf_status_t BfRtPREMulticastPortTableData::set_active_fields(
    const std::vector<bf_rt_id_t> &fields) {
  std::vector<bf_rt_id_t> dataFields;
  bf_status_t status = BF_SUCCESS;

  status = this->table_->dataFieldIdListGet(&dataFields);

  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d %s : cannot get data fields list",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  std::set<bf_rt_id_t> dataFieldsSet(dataFields.begin(), dataFields.end());

  if (fields.empty()) {
    activeFields = dataFieldsSet;
  } else {
    // check a field is valid for this table data then add into active field
    // list.
    for (const auto &field : fields) {
      if (dataFieldsSet.find(field) == dataFieldsSet.end()) {
        LOG_ERROR("ERROR %s:%d %s Invalid Field id %d ",
                  __func__,
                  __LINE__,
                  this->table_->table_name_get().c_str(),
                  field);
        continue;
      }
      auto elem = activeFields.find(field);
      if (elem == activeFields.end()) {
        activeFields.insert(field);
      } else {
        LOG_ERROR(
            "ERROR %s:%d %s Field id %d specified multiple times for data "
            "allocate",
            __func__,
            __LINE__,
            this->table_->table_name_get().c_str(),
            field);
      }
    }
  }

  return BF_SUCCESS;
}

bool BfRtPREMulticastPortTableData::checkFieldActive(
    const bf_rt_id_t &field_id) const {
  if (activeFields.find(field_id) != activeFields.end()) {
    return true;
  }

  LOG_ERROR("ERROR: %s:%d Inactive field id %d for table %s",
            __func__,
            __LINE__,
            field_id,
            this->table_->table_name_get().c_str());
  return false;
}

}  // namespace bfrt
