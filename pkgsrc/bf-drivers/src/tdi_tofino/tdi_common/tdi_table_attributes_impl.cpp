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

#include <array>
#include "tdi_table_attributes_impl.hpp"

namespace tdi {
namespace tna {
namespace tofino {

tdi_status_t TableAttributes::setValue(const tdi_attributes_field_type_e &type,
                                       const uint64_t &value) {
  auto tofino_attr_type =
      static_cast<tdi_tofino_attributes_type_e>(this->attributeTypeGet());
  switch (tofino_attr_type) {
    case TDI_TOFINO_ATTRIBUTES_TYPE_ENTRY_SCOPE: {
      auto entry_scope_field_type =
          static_cast<tdi_tofino_attributes_entry_scope_field_type_e>(type);
      switch (entry_scope_field_type) {
        case TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_FIELD_TYPE_ENTRY_SCOPE_TYPE:
          entry_scope_.entry_scope_ =
              static_cast<tdi_tofino_attributes_entry_scope_e>(value);
          break;
        case TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_FIELD_TYPE_ENTRY_SCOPE_ARGS:
          entry_scope_.property_args_value_ = value;
          break;
        case TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_FIELD_TYPE_GRESS_SCOPE_TYPE:
          entry_scope_.gress_scope_ =
              static_cast<tdi_tofino_attributes_gress_scope_e>(value);
          break;
        case TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_FIELD_TYPE_GRESS_TARGET:
          entry_scope_.prsr_args_value_ = value;
          break;
        case TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_FIELD_TYPE_PARSER_SCOPE_TYPE:
          entry_scope_.prsr_scope_ =
              static_cast<tdi_tofino_attributes_parser_scope_e>(value);
          break;
        default:
          LOG_ERROR("%s:%d Invalid field ID %d used",
                    __func__,
                    __LINE__,
                    entry_scope_field_type);
          return TDI_INVALID_ARG;
      }
      break;
    }
    case TDI_TOFINO_ATTRIBUTES_TYPE_DYNAMIC_KEY_MASK: {
    }
    case TDI_TOFINO_ATTRIBUTES_TYPE_METER_BYTE_COUNT_ADJ: {
      auto meter_field_type = static_cast<
          tdi_tofino_attributes_meter_byte_count_adjust_field_type_e>(type);
      switch (meter_field_type) {
        case TDI_TOFINO_ATTRIBUTES_METER_BYTE_COUNT_ADJUST_FIELD_TYPE_BYTE_COUNT:
          meter_bytecount_adj_.byte_count_adj_ = value;
          break;
        case TDI_TOFINO_ATTRIBUTES_METER_BYTE_COUNT_ADJUST_FIELD_TYPE_IS_POSITIVE:
          if ((value > 0 && meter_bytecount_adj_.byte_count_adj_ < 0) ||
              (value == 0 && meter_bytecount_adj_.byte_count_adj_ > 0))
            meter_bytecount_adj_.byte_count_adj_ =
                -meter_bytecount_adj_.byte_count_adj_;

          break;
        default:
          LOG_ERROR("%s:%d Invalid field ID %d used",
                    __func__,
                    __LINE__,
                    meter_field_type);
          return TDI_INVALID_ARG;
      }
      break;
    }
    case TDI_TOFINO_ATTRIBUTES_TYPE_IDLE_TABLE_RUNTIME: {
      auto idle_field_type =
          static_cast<tdi_tofino_attributes_idle_table_field_type_e>(type);
      switch (idle_field_type) {
        case TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_FIELD_TYPE_MODE:
          idle_table_.idleTableModeSet(
              static_cast<tdi_tofino_attributes_idle_table_mode_e>(value));
          break;
        case TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_FIELD_TYPE_ENABLE:
          idle_table_.enable_ = value;
          break;
        case TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_FIELD_TYPE_CALLBACK_CPP:
          idle_table_.callback_cpp_ = reinterpret_cast<IdleTmoExpiryCb>(value);
          break;
        case TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_FIELD_TYPE_CALLBACK_C:
          idle_table_.callback_c_ =
              reinterpret_cast<tdi_idle_tmo_expiry_cb>(value);
          break;
        case TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_FIELD_TYPE_TTL_QUERY_INTVL:
          idle_table_.idle_time_param_.u.notify.ttl_query_interval = value;
          break;
        case TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_FIELD_TYPE_MAX_TTL:
          idle_table_.idle_time_param_.u.notify.max_ttl = value;
          break;
        case TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_FIELD_TYPE_MIN_TTL:
          idle_table_.idle_time_param_.u.notify.min_ttl = value;
          break;
        case TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_FIELD_TYPE_COOKIE:
          idle_table_.idle_time_param_.u.notify.client_data =
              reinterpret_cast<void *>(value);
          break;
        default:
          LOG_ERROR("%s:%d Invalid field ID %d used",
                    __func__,
                    __LINE__,
                    idle_field_type);
          return TDI_INVALID_ARG;
      }
      break;
    }
    case TDI_TOFINO_ATTRIBUTES_TYPE_PORT_STATUS_NOTIF: {
      auto ps_field_type =
          static_cast<tdi_tofino_attributes_port_status_change_field_type_e>(
              type);
      switch (ps_field_type) {
        case TDI_TOFINO_ATTRIBUTES_PORT_STATUS_CHANGE_FIELD_TYPE_ENABLE:
          port_status_change_.enable_ = value;
          break;
        case TDI_TOFINO_ATTRIBUTES_PORT_STATUS_CHANGE_FIELD_TYPE_CALLBACK_CPP:
          port_status_change_.callback_func_ =
              reinterpret_cast<PortStatusNotifCb>(value);
          break;
        case TDI_TOFINO_ATTRIBUTES_PORT_STATUS_CHANGE_FIELD_TYPE_CALLBACK_C:
          port_status_change_.callback_func_c_ =
              reinterpret_cast<tdi_port_status_chg_cb>(value);
          break;
        case TDI_TOFINO_ATTRIBUTES_PORT_STATUS_CHANGE_FIELD_TYPE_COOKIE:
          port_status_change_.client_data_ = reinterpret_cast<void *>(value);
          break;
        default:
          LOG_ERROR("%s:%d Invalid field ID %d used",
                    __func__,
                    __LINE__,
                    ps_field_type);
          return TDI_INVALID_ARG;
      }
      break;
    }
    case TDI_TOFINO_ATTRIBUTES_TYPE_PORT_STAT_POLL_INTVL_MS: {
      auto ps_field_type = static_cast<
          tdi_tofino_attributes_port_stat_poll_intvl_ms_field_type_e>(type);
      switch (ps_field_type) {
        case TDI_TOFINO_ATTRIBUTES_PORT_STAT_POLL_INTVL_MS_FIELD_TYPE_VALUE:
          if (value > UINT32_MAX) {
            LOG_ERROR("%s:%d Value for this field cannot be > 32bits",
                      __func__,
                      __LINE__);
            return TDI_INVALID_ARG;
          }
          port_stat_poll_.intvl_ = (uint32_t)value;
          break;
        default:
          LOG_ERROR("%s:%d Invalid field ID %d used",
                    __func__,
                    __LINE__,
                    ps_field_type);
          return TDI_INVALID_ARG;
      }
      break;
    }
    default:
      LOG_TRACE(
          "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
          "set "
          "attributes",
          __func__,
          __LINE__,
          tableGet()->tableInfoGet()->nameGet().c_str(),
          static_cast<int>(tofino_attr_type));
      TDI_DBGCHK(0);
      return TDI_INVALID_ARG;
  }
  return TDI_SUCCESS;
}
tdi_status_t TableAttributes::getValue(const tdi_attributes_field_type_e &type,
                                       uint64_t *value) const {
  auto tofino_attr_type =
      static_cast<tdi_tofino_attributes_type_e>(this->attributeTypeGet());
  switch (tofino_attr_type) {
    case TDI_TOFINO_ATTRIBUTES_TYPE_ENTRY_SCOPE: {
      auto entry_scope_field_type =
          static_cast<tdi_tofino_attributes_entry_scope_field_type_e>(type);
      switch (entry_scope_field_type) {
        case TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_FIELD_TYPE_ENTRY_SCOPE_TYPE:
          *value = static_cast<uint64_t>(entry_scope_.entry_scope_);
          break;
        case TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_FIELD_TYPE_ENTRY_SCOPE_ARGS:
          *value = static_cast<uint64_t>(entry_scope_.property_args_value_);
          break;
        case TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_FIELD_TYPE_GRESS_SCOPE_TYPE:
          *value = static_cast<uint64_t>(entry_scope_.gress_scope_);
          break;
        case TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_FIELD_TYPE_GRESS_TARGET:
          *value = static_cast<uint64_t>(entry_scope_.prsr_args_value_);
          break;
        case TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_FIELD_TYPE_PARSER_SCOPE_TYPE:
          *value = static_cast<uint64_t>(entry_scope_.prsr_scope_);
          break;
        default:
          LOG_ERROR("%s:%d Invalid field ID %d used",
                    __func__,
                    __LINE__,
                    entry_scope_field_type);
          return TDI_INVALID_ARG;
      }
      break;
    }
    case TDI_TOFINO_ATTRIBUTES_TYPE_DYNAMIC_KEY_MASK: {
    }
    case TDI_TOFINO_ATTRIBUTES_TYPE_METER_BYTE_COUNT_ADJ: {
      auto meter_field_type = static_cast<
          tdi_tofino_attributes_meter_byte_count_adjust_field_type_e>(type);
      switch (meter_field_type) {
        case TDI_TOFINO_ATTRIBUTES_METER_BYTE_COUNT_ADJUST_FIELD_TYPE_BYTE_COUNT:
          *value = std::abs(meter_bytecount_adj_.byte_count_adj_);
          break;
        case TDI_TOFINO_ATTRIBUTES_METER_BYTE_COUNT_ADJUST_FIELD_TYPE_IS_POSITIVE:
          *value = 1 ? meter_bytecount_adj_.byte_count_adj_ >= 0 : 0;
          break;
        default:
          LOG_ERROR("%s:%d Invalid field ID %d used",
                    __func__,
                    __LINE__,
                    meter_field_type);
          return TDI_INVALID_ARG;
      }
      break;
    }
    case TDI_TOFINO_ATTRIBUTES_TYPE_IDLE_TABLE_RUNTIME: {
      auto idle_field_type =
          static_cast<tdi_tofino_attributes_idle_table_field_type_e>(type);
      switch (idle_field_type) {
        case TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_FIELD_TYPE_MODE:
          *value = static_cast<uint64_t>(idle_table_.idleTableModeGet());
          break;
        case TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_FIELD_TYPE_ENABLE:
          *value = static_cast<uint64_t>(idle_table_.enable_);
          break;
        case TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_FIELD_TYPE_CALLBACK_CPP:
          *value = reinterpret_cast<uint64_t>(idle_table_.callback_cpp_);
          break;
        case TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_FIELD_TYPE_CALLBACK_C:
          *value = reinterpret_cast<uint64_t>(idle_table_.callback_c_);
          break;
        case TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_FIELD_TYPE_TTL_QUERY_INTVL:
          *value = static_cast<uint64_t>(
              idle_table_.idle_time_param_.u.notify.ttl_query_interval);
          break;
        case TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_FIELD_TYPE_MAX_TTL:
          *value = static_cast<uint64_t>(
              idle_table_.idle_time_param_.u.notify.max_ttl);
          break;
        case TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_FIELD_TYPE_MIN_TTL:
          *value = static_cast<uint64_t>(
              idle_table_.idle_time_param_.u.notify.min_ttl);
          break;
        case TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_FIELD_TYPE_COOKIE:
          *value = reinterpret_cast<uint64_t>(
              idle_table_.idle_time_param_.u.notify.client_data);
          break;
        default:
          LOG_ERROR("%s:%d Invalid field ID %d used",
                    __func__,
                    __LINE__,
                    idle_field_type);
          return TDI_INVALID_ARG;
      }
      break;
    }
    case TDI_TOFINO_ATTRIBUTES_TYPE_PORT_STATUS_NOTIF: {
      auto ps_field_type =
          static_cast<tdi_tofino_attributes_port_status_change_field_type_e>(
              type);
      switch (ps_field_type) {
        case TDI_TOFINO_ATTRIBUTES_PORT_STATUS_CHANGE_FIELD_TYPE_ENABLE:
          *value = static_cast<uint64_t>(port_status_change_.enable_);
          break;
        case TDI_TOFINO_ATTRIBUTES_PORT_STATUS_CHANGE_FIELD_TYPE_CALLBACK_CPP:
          *value =
              reinterpret_cast<uint64_t>(port_status_change_.callback_func_);
          break;
        case TDI_TOFINO_ATTRIBUTES_PORT_STATUS_CHANGE_FIELD_TYPE_CALLBACK_C:
          *value =
              reinterpret_cast<uint64_t>(port_status_change_.callback_func_c_);
          break;
        case TDI_TOFINO_ATTRIBUTES_PORT_STATUS_CHANGE_FIELD_TYPE_COOKIE:
          *value = reinterpret_cast<uint64_t>(port_status_change_.client_data_);
          break;
        default:
          LOG_ERROR("%s:%d Invalid field ID %d used",
                    __func__,
                    __LINE__,
                    ps_field_type);
          return TDI_INVALID_ARG;
      }
      break;
    }
    case TDI_TOFINO_ATTRIBUTES_TYPE_PORT_STAT_POLL_INTVL_MS: {
      auto ps_field_type = static_cast<
          tdi_tofino_attributes_port_stat_poll_intvl_ms_field_type_e>(type);
      switch (ps_field_type) {
        case TDI_TOFINO_ATTRIBUTES_PORT_STAT_POLL_INTVL_MS_FIELD_TYPE_VALUE:
          *value = static_cast<uint64_t>(port_stat_poll_.intvl_);
          break;
        default:
          LOG_ERROR("%s:%d Invalid field ID %d used",
                    __func__,
                    __LINE__,
                    ps_field_type);
          return TDI_INVALID_ARG;
      }
      break;
    }
    default:
      LOG_TRACE(
          "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
          "set "
          "attributes",
          __func__,
          __LINE__,
          tableGet()->tableInfoGet()->nameGet().c_str(),
          static_cast<int>(tofino_attr_type));
      TDI_DBGCHK(0);
      return TDI_INVALID_ARG;
  }
  return TDI_SUCCESS;
}

tdi_status_t TableAttributes::setValue(const tdi_attributes_field_type_e &type,
                                       const uint8_t *value,
                                       const size_t &size_bytes) {
  auto tofino_attr_type =
      static_cast<tdi_tofino_attributes_type_e>(this->attributeTypeGet());
  switch (tofino_attr_type) {
    case TDI_TOFINO_ATTRIBUTES_TYPE_DYNAMIC_KEY_MASK: {
      std::vector<uint8_t> byte_vector(value, value + size_bytes);
      dyn_key_mask_.field_mask_[type] = byte_vector;
      break;
    }
    default:
      LOG_TRACE(
          "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
          "set "
          "attributes",
          __func__,
          __LINE__,
          tableGet()->tableInfoGet()->nameGet().c_str(),
          static_cast<int>(tofino_attr_type));
      TDI_DBGCHK(0);
      return TDI_INVALID_ARG;
  }
  return TDI_SUCCESS;
}

tdi_status_t TableAttributes::getValue(const tdi_attributes_field_type_e &type,
                                       const size_t &size_bytes,
                                       uint8_t *value) const {
  auto tofino_attr_type =
      static_cast<tdi_tofino_attributes_type_e>(this->attributeTypeGet());
  switch (tofino_attr_type) {
    case TDI_TOFINO_ATTRIBUTES_TYPE_DYNAMIC_KEY_MASK: {
      int i = 0;
      if (size_bytes != dyn_key_mask_.field_mask_.at(type).size()) {
        LOG_ERROR(
            "%s:%d %s Size of bytes sent in %zu doesn't match field size "
            "%zu",
            __func__,
            __LINE__,
            tableGet()->tableInfoGet()->nameGet().c_str(),
            size_bytes,
            dyn_key_mask_.field_mask_.at(type).size());
        return TDI_INVALID_ARG;
      }
      for (const auto &item : dyn_key_mask_.field_mask_.at(type)) {
        value[i++] = item;
      }
      break;
    }
    default:
      LOG_ERROR(
          "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
          "set "
          "attributes",
          __func__,
          __LINE__,
          tableGet()->tableInfoGet()->nameGet().c_str(),
          static_cast<int>(tofino_attr_type));
      return TDI_INVALID_ARG;
  }
  return TDI_SUCCESS;
}

TableEntryScopeArguments::TableEntryScopeArguments(const std::bitset<32> &val) {
  scope_value_ = val.to_ulong();
}

TableEntryScopeArguments::TableEntryScopeArguments(
    const std::array<std::bitset<8>, 4> &val_arr)
    : scope_value_() {
  int i = 0;
  for (const auto &ele : val_arr) {
    scope_value_ |= (ele.to_ulong() << (i * 8));
    i++;
  }
}

tdi_status_t TableEntryScopeArguments::setValue(const std::bitset<32> &val) {
  scope_value_ = val.to_ulong();
  return TDI_SUCCESS;
}

tdi_status_t TableEntryScopeArguments::setValue(
    const std::array<std::bitset<8>, 4> &val_arr) {
  int i = 0;
  for (const auto &ele : val_arr) {
    // Form scope_value_ from the elements of the array passed in with least
    // significant byte of scope_value_ (byte 0)  = val_arr[0], byte 1 =
    // val_arr[1] and so on.
    scope_value_ |= (ele.to_ulong() << (i * 8));
    i++;
  }
  return TDI_SUCCESS;
}

tdi_status_t TableEntryScopeArguments::getValue(std::bitset<32> *val) const {
  if (val == nullptr) {
    LOG_ERROR("%s:%d No memory assigned for out-param to get scope arguments",
              __func__,
              __LINE__);
    return TDI_INVALID_ARG;
  }
  *val = scope_value_;
  return TDI_SUCCESS;
}

tdi_status_t TableEntryScopeArguments::getValue(
    std::array<std::bitset<8>, 4> *val_arr) const {
  int i = 0;
  // Form a byte array of size 4 from the scope_value_ with val_arr[0] = least
  // signnificant byte of scope_value_ (byte 0), val_arr[1] = byte 1
  // and so on
  for (auto &ele : *val_arr) {
    ele = ((scope_value_ >> (i * 8)) & 0xff);
    i++;
  }
  return TDI_SUCCESS;
}

TableAttributesIdleTable::TableAttributesIdleTable() : enable_(false) {
  idle_time_param_.mode = POLL_MODE;
  // TODO confirm correctness
  idle_time_param_.u.notify.max_ttl = idle_time_param_.u.notify.min_ttl = 0;
}

void TableAttributesIdleTable::idleTableAllParamsClear() {
  enable_ = false;
  callback_cpp_ = nullptr;
  callback_c_ = nullptr;
  std::memset(&idle_time_param_, 0, sizeof(idle_time_param_));
}

tdi_status_t TableAttributesIdleTable::idleTableModeSet(
    const tdi_tofino_attributes_idle_table_mode_e &table_type) {
  switch (table_type) {
    case TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_NOTIFY_MODE:
      idle_time_param_.mode = NOTIFY_MODE;
      break;
    case TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_POLL_MODE:
      idle_time_param_.mode = POLL_MODE;
      break;
    case TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_INVALID_MODE:
      idle_time_param_.mode = INVALID_MODE;
      break;
    default:
      LOG_ERROR("%s:%d Invalid value for Idle Table Mode", __func__, __LINE__);
      return TDI_INVALID_ARG;
  }
  return TDI_SUCCESS;
}

tdi_tofino_attributes_idle_table_mode_e
TableAttributesIdleTable::idleTableModeGet() const {
  switch (idle_time_param_.mode) {
    case NOTIFY_MODE:
      return TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_NOTIFY_MODE;
    case POLL_MODE:
      return TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_POLL_MODE;
    case INVALID_MODE:
      return TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_INVALID_MODE;
    default:
      LOG_ERROR("%s:%d Invalid Idle Table Mode Found set internally",
                __func__,
                __LINE__);
      TDI_ASSERT(0);
  }
  return TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_POLL_MODE;
}

tdi_status_t TableAttributesIdleTable::idleTablePollModeSet(
    const bool &enable) {
  if (idle_time_param_.mode != POLL_MODE) {
    LOG_ERROR(
        "%s:%d Table Attribute Obj with Idle Table mode %d can't be used to "
        "set POLL_MODE",
        __func__,
        __LINE__,
        idle_time_param_.mode);
  }
  enable_ = enable;
  callback_cpp_ = nullptr;
  callback_c_ = nullptr;
  idle_time_param_.u.notify.ttl_query_interval = 0;
  idle_time_param_.u.notify.max_ttl = 0;
  idle_time_param_.u.notify.min_ttl = 0;
  idle_time_param_.u.notify.client_data = nullptr;
  return TDI_SUCCESS;
}

tdi_status_t TableAttributesIdleTable::idleTableNotifyModeSet(
    const bool &enable,
    const IdleTmoExpiryCb &callback_cpp,
    const tdi_idle_tmo_expiry_cb &callback_c,
    const uint32_t &ttl_query_interval,
    const uint32_t &max_ttl,
    const uint32_t &min_ttl,
    const void *cookie) {
  if (idle_time_param_.mode != NOTIFY_MODE) {
    LOG_ERROR(
        "%s:%d Table Attribute Obj with Idle Table mode %d can't be used to "
        "set NOTIFY_MODE",
        __func__,
        __LINE__,
        idle_time_param_.mode);
  }
  enable_ = enable;
  callback_cpp_ = callback_cpp;
  callback_c_ = callback_c;
  idle_time_param_.u.notify.ttl_query_interval = ttl_query_interval;
  idle_time_param_.u.notify.max_ttl = max_ttl;
  idle_time_param_.u.notify.min_ttl = min_ttl;
  idle_time_param_.u.notify.client_data = const_cast<void *>(cookie);
  return TDI_SUCCESS;
}

tdi_status_t TableAttributesIdleTable::idleTableGet(
    tdi_tofino_attributes_idle_table_mode_e *mode,
    bool *enable,
    IdleTmoExpiryCb *callback_cpp,
    tdi_idle_tmo_expiry_cb *callback_c,
    uint32_t *ttl_query_interval,
    uint32_t *max_ttl,
    uint32_t *min_ttl,
    void **cookie) const {
  if (mode) {
    switch (idle_time_param_.mode) {
      case POLL_MODE:
        *mode = TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_POLL_MODE;
        break;
      case NOTIFY_MODE:
        *mode = TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_NOTIFY_MODE;
        break;
      case INVALID_MODE:
        *mode = TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_INVALID_MODE;
        break;
      default:
        LOG_ERROR("%s:%d Invalid Idle Table Mode found to be set",
                  __func__,
                  __LINE__);
        TDI_ASSERT(0);
    }
  }
  if (enable) {
    *enable = enable_;
  }
  if (callback_cpp) {
    *callback_cpp = callback_cpp_;
  }
  if (callback_c) {
    *callback_c = callback_c_;
  }
  if (ttl_query_interval) {
    *ttl_query_interval = idle_time_param_.u.notify.ttl_query_interval;
  }
  if (max_ttl) {
    *max_ttl = idle_time_param_.u.notify.max_ttl;
  }
  if (min_ttl) {
    *min_ttl = idle_time_param_.u.notify.min_ttl;
  }
  if (cookie) {
    *cookie = idle_time_param_.u.notify.client_data;
  }
  return TDI_SUCCESS;
}

TableAttributesEntryScope::TableAttributesEntryScope()
    : entry_scope_(TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_ALL_PIPELINES) {
  property_args_value_ = 0;
}

tdi_status_t TableAttributesEntryScope::entryScopeParamsSet(
    const tdi_tofino_attributes_entry_scope_e &entry_scope,
    const TableEntryScopeArguments &scope_args) {
  switch (entry_scope) {
    case TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_ALL_PIPELINES:
    case TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_SINGLE_PIPELINE:
    case TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_USER_DEFINED:
      break;
    default:
      LOG_ERROR("%s:%d Unrecognized entry scope", __func__, __LINE__);
      return TDI_INVALID_ARG;
  }
  std::bitset<32> bitval;
  scope_args.getValue(&bitval);
  property_args_value_ = bitval.to_ulong();
  entry_scope_ = entry_scope;
  return TDI_SUCCESS;
}

tdi_status_t TableAttributesEntryScope::entryScopeParamsSet(
    const tdi_tofino_attributes_entry_scope_e &entry_scope) {
  switch (entry_scope) {
    case TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_ALL_PIPELINES:
    case TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_SINGLE_PIPELINE:
      break;
    case TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_USER_DEFINED:
    default:
      LOG_ERROR("%s:%d Unrecognized entry scope", __func__, __LINE__);
      return TDI_INVALID_ARG;
  }
  entry_scope_ = entry_scope;
  return TDI_SUCCESS;
}

tdi_status_t TableAttributesEntryScope::entryScopeParamsGet(
    tdi_tofino_attributes_entry_scope_e *entry_scope,
    TableEntryScopeArguments *scope_args) const {
  if (entry_scope == nullptr) {
    LOG_ERROR(
        "%s:%d No memory assigned for out-param to get "
        "tdi_tofino_attributes_entry_scope_e type",
        __func__,
        __LINE__);
    return TDI_INVALID_ARG;
  }
  std::bitset<32> bitval(property_args_value_);
  if (scope_args != nullptr) {
    scope_args->setValue(bitval);
  }
  *entry_scope = entry_scope_;
  return TDI_SUCCESS;
}

tdi_status_t TableAttributesEntryScope::prsrScopeParamsSet(
    const tdi_tofino_attributes_parser_scope_e &prsr_scope,
    const tdi_tofino_attributes_gress_target_e &prsr_gress) {
  switch (prsr_gress) {
    case TDI_TOFINO_ATTRIBUTES_GRESS_TARGET_INGRESS:
      prsr_args_value_ = static_cast<uint32_t>(TNA_DIRECTION_INGRESS);
      break;
    case TDI_TOFINO_ATTRIBUTES_GRESS_TARGET_EGRESS:
      prsr_args_value_ = static_cast<uint32_t>(TNA_DIRECTION_EGRESS);
      break;
    case TDI_TOFINO_ATTRIBUTES_GRESS_TARGET_ALL:
      prsr_args_value_ = static_cast<uint32_t>(TNA_DIRECTION_ALL);
      break;
    default:
      LOG_ERROR("%s:%d Unrecognized parser scope gress (%d)",
                __func__,
                __LINE__,
                static_cast<int>(prsr_gress));
      return TDI_INVALID_ARG;
  }
  prsr_scope_ = prsr_scope;
  return TDI_SUCCESS;
}

tdi_status_t TableAttributesEntryScope::prsrScopeParamsGet(
    tdi_tofino_attributes_parser_scope_e *prsr_scope,
    tdi_tofino_attributes_gress_target_e *prsr_gress) const {
  if ((prsr_scope == nullptr) || (prsr_gress == nullptr)) {
    LOG_ERROR(
        "%s:%d No memory assigned for out-param to get "
        "tdi_tofino_attributes_parser_scope_e type",
        __func__,
        __LINE__);
    return TDI_INVALID_ARG;
  }
  switch (prsr_args_value_) {
    case (static_cast<uint32_t>(TNA_DIRECTION_INGRESS)):
      *prsr_gress = TDI_TOFINO_ATTRIBUTES_GRESS_TARGET_INGRESS;
      break;
    case (static_cast<uint32_t>(TNA_DIRECTION_EGRESS)):
      *prsr_gress = TDI_TOFINO_ATTRIBUTES_GRESS_TARGET_EGRESS;
      break;
    case (static_cast<uint32_t>(TNA_DIRECTION_ALL)):
      *prsr_gress = TDI_TOFINO_ATTRIBUTES_GRESS_TARGET_ALL;
      break;
    default:
      LOG_ERROR("%s:%d Unrecognized parser scope args (%d)",
                __func__,
                __LINE__,
                static_cast<int>(prsr_args_value_));
      return TDI_INVALID_ARG;
  }
  *prsr_scope = prsr_scope_;
  return TDI_SUCCESS;
}

TableAttributesDynKeyMask::TableAttributesDynKeyMask() {
  dynKeyMaskAllParamsClear();
}

void TableAttributesDynKeyMask::dynKeyMaskAllParamsClear() {
  field_mask_.clear();
}

tdi_status_t TableAttributesDynKeyMask::dynKeyMaskParamsSet(
    const std::unordered_map<tdi_id_t, std::vector<uint8_t>> &field_mask) {
  field_mask_ = field_mask;
  return TDI_SUCCESS;
}

tdi_status_t TableAttributesDynKeyMask::dynKeyMaskParamsGet(
    std::unordered_map<tdi_id_t, std::vector<uint8_t>> *field_mask) const {
  if (field_mask == NULL) {
    LOG_ERROR(
        "%s:%d No memory assigned for out-param to get dynamic key mask "
        "attribute",
        __func__,
        __LINE__);
    return TDI_INVALID_ARG;
  }
  *field_mask = field_mask_;
  return TDI_SUCCESS;
}

TableAttributesMeterByteCountAdj::TableAttributesMeterByteCountAdj() {
  byteCountAdjClear();
}

void TableAttributesMeterByteCountAdj::byteCountAdjClear() {
  byte_count_adj_ = 0;
}

tdi_status_t TableAttributesMeterByteCountAdj::byteCountAdjSet(
    const int &byte_count_adj) {
  byte_count_adj_ = byte_count_adj;
  return TDI_SUCCESS;
}

tdi_status_t TableAttributesMeterByteCountAdj::byteCountAdjGet(
    int *byte_count_adj) const {
  if (byte_count_adj == NULL) {
    LOG_ERROR(
        "%s:%d No memory assigned for out-param to get meter byte count adjust "
        "attribute",
        __func__,
        __LINE__);
    return TDI_INVALID_ARG;
  }
  *byte_count_adj = byte_count_adj_;
  return TDI_SUCCESS;
}

TableAttributePortStatusChangeReg::TableAttributePortStatusChangeReg() {
  portStatusChgParamsClear();
}

tdi_status_t TableAttributePortStatusChangeReg::portStatusChgCbSet(
    const bool enable,
    const PortStatusNotifCb &cb,
    const tdi_port_status_chg_cb &cb_c,
    const void *cookie) {
  if (cb && cb_c) {
    LOG_ERROR(
        "%s:%d Not allow to set both c and c++ callback functions at the same "
        "time",
        __func__,
        __LINE__);
    return TDI_INVALID_ARG;
  }
  enable_ = enable;
  callback_func_ = cb;
  callback_func_c_ = cb_c;
  client_data_ = const_cast<void *>(cookie);
  return TDI_SUCCESS;
}

tdi_status_t TableAttributePortStatusChangeReg::portStatusChgCbGet(
    bool *enable,
    PortStatusNotifCb *cb,
    tdi_port_status_chg_cb *cb_c,
    void **cookie) const {
  if (cb) {
    *cb = callback_func_;
  }
  if (cb_c) {
    *cb_c = callback_func_c_;
  }
  *enable = enable_;
  if (cookie && *cookie) *cookie = client_data_;
  return TDI_SUCCESS;
}

void TableAttributePortStatusChangeReg::portStatusChgParamsClear() {
  enable_ = false;
  callback_func_ = nullptr;
  callback_func_c_ = nullptr;
  client_data_ = nullptr;
}

TableAttributesPortStatPollIntvl::TableAttributesPortStatPollIntvl() {
  portStatPollIntvlParamClear();
}

tdi_status_t TableAttributesPortStatPollIntvl::portStatPollIntvlParamSet(
    const uint32_t &poll_intvl) {
  intvl_ = poll_intvl;
  return TDI_SUCCESS;
}

tdi_status_t TableAttributesPortStatPollIntvl::portStatPollIntvlParamGet(
    uint32_t *poll_intvl) const {
  if (poll_intvl == NULL) {
    LOG_ERROR(
        "%s:%d No memory assigned for out-param to get port stat poll interval "
        "attribute",
        __func__,
        __LINE__);
    return TDI_INVALID_ARG;
  }
  *poll_intvl = intvl_;
  return TDI_SUCCESS;
}

void TableAttributesPortStatPollIntvl::portStatPollIntvlParamClear() {
  intvl_ = 0;
}

TableAttributesPREDeviceConfig::TableAttributesPREDeviceConfig() {
  this->preDeviceConfigParamsClear();
}

tdi_status_t TableAttributesPREDeviceConfig::preGlobalRidParamSet(
    const uint32_t &global_rid) {
  // First mark the PRE global rid attribute active by adding it to
  // the active attibutes set. No need to check if it is present already
  // or for any errors.
  this->active_pre_attributes_.insert(this->PREAttributeType::PRE_GLOBAL_RID);

  // Set the value
  global_rid_ = global_rid;

  return TDI_SUCCESS;
}

tdi_status_t TableAttributesPREDeviceConfig::preGlobalRidParamGet(
    uint32_t *global_rid) const {
  if (global_rid == NULL) {
    LOG_ERROR(
        "%s:%d No memory assigned for out-param to get PRE global rid "
        "attribute",
        __func__,
        __LINE__);
    return TDI_INVALID_ARG;
  }

  // Check whether global rid attribute is part of the active attributes set
  auto elem =
      this->active_pre_attributes_.find(this->PREAttributeType::PRE_GLOBAL_RID);
  if (elem == this->active_pre_attributes_.end()) {
    // No need to log error here as this could be valid case
    return TDI_OBJECT_NOT_FOUND;
  }

  *global_rid = global_rid_;

  return TDI_SUCCESS;
}

tdi_status_t TableAttributesPREDeviceConfig::prePortProtectionParamSet(
    const bool &enable) {
  // First mark the PRE port protection attribute active by adding it to
  // the active attibutes set. No need to check if it is present already
  // or for any errors.
  this->active_pre_attributes_.insert(
      this->PREAttributeType::PRE_PORT_PROTECTION);

  // Set the value
  port_protection_enable_ = enable;

  return TDI_SUCCESS;
}

tdi_status_t TableAttributesPREDeviceConfig::prePortProtectionParamGet(
    bool *enable) const {
  if (enable == NULL) {
    LOG_ERROR(
        "%s:%d No memory assigned for out-param to get PRE port protection "
        "attribute",
        __func__,
        __LINE__);
    return TDI_INVALID_ARG;
  }

  // Check whether port protection attribute is part of the active attributes
  // set
  auto elem = this->active_pre_attributes_.find(
      this->PREAttributeType::PRE_PORT_PROTECTION);
  if (elem == this->active_pre_attributes_.end()) {
    // No need to log error here as this could be valid case
    return TDI_OBJECT_NOT_FOUND;
  }

  *enable = port_protection_enable_;

  return TDI_SUCCESS;
}

tdi_status_t TableAttributesPREDeviceConfig::preFastFailoverParamSet(
    const bool &enable) {
  // First mark the PRE fast failover attribute active by adding it to
  // the active attibutes set. No need to check if it is present already
  // or for any errors.
  this->active_pre_attributes_.insert(
      this->PREAttributeType::PRE_FAST_FAILOVER);

  // Set the value
  fast_failover_enable_ = enable;

  return TDI_SUCCESS;
}

tdi_status_t TableAttributesPREDeviceConfig::preFastFailoverParamGet(
    bool *enable) const {
  if (enable == NULL) {
    LOG_ERROR(
        "%s:%d No memory assigned for out-param to get PRE fast failover "
        "attribute",
        __func__,
        __LINE__);
    return TDI_INVALID_ARG;
  }

  // Check whether fast failover attribute is part of the active attributes set
  auto elem = this->active_pre_attributes_.find(
      this->PREAttributeType::PRE_FAST_FAILOVER);
  if (elem == this->active_pre_attributes_.end()) {
    // No need to log error here as this could be valid case
    return TDI_OBJECT_NOT_FOUND;
  }

  *enable = fast_failover_enable_;

  return TDI_SUCCESS;
}

tdi_status_t TableAttributesPREDeviceConfig::preMaxNodesBeforeYieldParamSet(
    const uint32_t &count) {
  // First mark the PRE max nodes before yield attribute active by adding it to
  // the active attibutes set. No need to check if it is present already
  // or for any errors.
  this->active_pre_attributes_.insert(
      this->PREAttributeType::PRE_MAX_NODES_BEFORE_YIELD);

  // Set the value
  max_nodes_before_yield_ = count;

  return TDI_SUCCESS;
}

tdi_status_t TableAttributesPREDeviceConfig::preMaxNodesBeforeYieldParamGet(
    uint32_t *count) const {
  if (count == NULL) {
    LOG_ERROR(
        "%s:%d No memory assigned for out-param to get PRE max nodes before "
        "yield "
        "attribute",
        __func__,
        __LINE__);
    return TDI_INVALID_ARG;
  }

  // Check whether max nodes before yield attribute is part of the active
  // attributes set
  auto elem = this->active_pre_attributes_.find(
      this->PREAttributeType::PRE_MAX_NODES_BEFORE_YIELD);
  if (elem == this->active_pre_attributes_.end()) {
    // No need to log error here as this could be valid case
    return TDI_OBJECT_NOT_FOUND;
  }

  *count = max_nodes_before_yield_;

  return TDI_SUCCESS;
}

tdi_status_t TableAttributesPREDeviceConfig::preMaxNodeThresholdParamSet(
    const uint32_t &node_count, const uint32_t &node_port_lag_count) {
  // First mark the PRE max threshold attribute active by adding it to
  // the active attibutes set. No need to check if it is present already
  // or for any errors.
  this->active_pre_attributes_.insert(
      this->PREAttributeType::PRE_MAX_NODE_THRESHOLD);

  // Set the value
  max_node_threshold_node_count_ = node_count;
  max_node_threshold_port_lag_count_ = node_port_lag_count;

  return TDI_SUCCESS;
}

tdi_status_t TableAttributesPREDeviceConfig::preMaxNodeThresholdParamGet(
    uint32_t *node_count, uint32_t *node_port_lag_count) const {
  if (node_count == NULL || node_port_lag_count == NULL) {
    LOG_ERROR(
        "%s:%d No memory assigned for out-param(s) to get PRE max node "
        "threshold "
        "attribute",
        __func__,
        __LINE__);
    return TDI_INVALID_ARG;
  }

  // Check whether max node threshold attribute is part of the active attributes
  // set
  auto elem = this->active_pre_attributes_.find(
      this->PREAttributeType::PRE_MAX_NODE_THRESHOLD);
  if (elem == this->active_pre_attributes_.end()) {
    // No need to log error here as this could be valid case
    return TDI_OBJECT_NOT_FOUND;
  }

  *node_count = max_node_threshold_node_count_;
  *node_port_lag_count = max_node_threshold_port_lag_count_;

  return TDI_SUCCESS;
}

void TableAttributesPREDeviceConfig::preDeviceConfigParamsClear() {
  // Clear the active PRE attributes set
  this->active_pre_attributes_.clear();

  // Though the actual PRE attribute's value doesn't
  // matter if the PRE attributes set is all clear, better to initialize each
  // attribute filed to default values.
  global_rid_ = 0;
  port_protection_enable_ = false;
  fast_failover_enable_ = false;
  max_nodes_before_yield_ = 0;
  max_node_threshold_node_count_ = 0;
  max_node_threshold_port_lag_count_ = 0;
}

TableAttributesSelectorUpdateCallback::TableAttributesSelectorUpdateCallback() {
  this->paramsClear();
}

void TableAttributesSelectorUpdateCallback::paramsClear() {
  enable_ = false;
  session_.reset();
  cpp_callback_fn_ = nullptr;
  c_callback_fn_ = nullptr;
  cookie_ = nullptr;
}

void TableAttributesSelectorUpdateCallback::paramSet(
    const bool &enable,
    const std::shared_ptr<tdi::Session> session,
    const SelUpdateCb &cpp_callback_fn,
    const tdi_selector_table_update_cb &c_callback_fn,
    const void *cookie) {
  enable_ = enable;
  if (enable_) {
    session_ = session;
    cpp_callback_fn_ = cpp_callback_fn;
    c_callback_fn_ = c_callback_fn;
    cookie_ = cookie;
  }
  return;
}

void TableAttributesSelectorUpdateCallback::paramGet(
    bool *enable,
    tdi::Session **session,
    SelUpdateCb *cpp_callback_fn,
    tdi_selector_table_update_cb *c_callback_fn,
    void **cookie) const {
  *enable = enable_;
  if (enable_) {
    auto session_obj = session_.lock();
    if (session_obj != nullptr) {
      *session = session_obj.get();
    } else {
      LOG_ERROR(
          "%s:%d ERROR The session for which the selector update callback was "
          "set up no longer exists. Hence unavailable to retrieve the session",
          __func__,
          __LINE__);
      *session = nullptr;
    }
    if (cpp_callback_fn) {
      *cpp_callback_fn = cpp_callback_fn_;
    }
    if (c_callback_fn) {
      *c_callback_fn = c_callback_fn_;
    }
    *cookie = const_cast<void *>(cookie_);
  }
  return;
}

void TableAttributesSelectorUpdateCallback::paramSetInternal(
    const std::tuple<bool,
                     std::weak_ptr<tdi::Session>,
                     SelUpdateCb,
                     tdi_selector_table_update_cb,
                     const void *> &t) {
  enable_ = std::get<0>(t);
  session_ = std::get<1>(t);
  cpp_callback_fn_ = std::get<2>(t);
  c_callback_fn_ = std::get<3>(t);
  cookie_ = std::get<4>(t);
}
std::tuple<bool,
           std::weak_ptr<tdi::Session>,
           SelUpdateCb,
           tdi_selector_table_update_cb,
           const void *>
TableAttributesSelectorUpdateCallback::paramGetInternal() const {
  return std::make_tuple(
      enable_, session_, cpp_callback_fn_, c_callback_fn_, cookie_);
}

#if 0
tdi_status_t TableAttributes::resetAttributeType(
    const tdi_tofino_attributes_type_e &attr) {
  switch (attr) {
    case TDI_TOFINO_ATTRIBUTES_TYPE_IDLE_TABLE_RUNTIME:
      LOG_ERROR("%s:%d This API can't be used to set Attribute type as %d",
                __func__,
                __LINE__,
                static_cast<int>(attr));
      return TDI_INVALID_ARG;
    case TDI_TOFINO_ATTRIBUTES_TYPE_ENTRY_SCOPE:
      entry_scope_.entryScopeAllParamsClear();
      break;
    case TDI_TOFINO_ATTRIBUTES_TYPE_DYNAMIC_KEY_MASK:
      dyn_key_mask_.dynKeyMaskAllParamsClear();
      break;
    case TDI_TOFINO_ATTRIBUTES_TYPE_METER_BYTE_COUNT_ADJ:
      meter_bytecount_adj_.byteCountAdjClear();
      break;
    case TDI_TOFINO_ATTRIBUTES_TYPE_PORT_STATUS_NOTIF:
      port_status_change_.portStatusChgParamsClear();
      break;
    case TDI_TOFINO_ATTRIBUTES_TYPE_PORT_STAT_POLL_INTVL_MS:
      port_stat_poll_.portStatPollIntvlParamClear();
      break;
    case TDI_TOFINO_ATTRIBUTES_TYPE_PRE_DEVICE_CONFIG:
      pre_device_config_.preDeviceConfigParamsClear();
      break;
    case TDI_TOFINO_ATTRIBUTES_TYPE_SELECTOR_UPDATE_CALLBACK:
      selector_update_callback_.paramsClear();
      break;
    default:
      LOG_ERROR("%s:%d Invalid Attribute Type %d",
                __func__,
                __LINE__,
                static_cast<int>(attr));
      return TDI_INVALID_ARG;
  }
  attributeTypeGet() = attr;
  return TDI_SUCCESS;
}
#endif

tdi_status_t TableAttributes::idleTablePollModeSet(const bool &enable) {
  if (attributeTypeGet() !=
      static_cast<tdi_attributes_type_e>(
          TDI_TOFINO_ATTRIBUTES_TYPE_IDLE_TABLE_RUNTIME)) {
    LOG_ERROR("%s:%d Can't set Idle Table Attribute with Object of type %d",
              __func__,
              __LINE__,
              static_cast<int>(attributeTypeGet()));
    return TDI_INVALID_ARG;
  }
  const auto idle_mode = idleTableModeGet();
  if (idle_mode != TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_POLL_MODE) {
    LOG_ERROR(
        "%s:%d Can't set Idle Table Poll Mode with Attribute object of type %d",
        __func__,
        __LINE__,
        static_cast<int>(idle_mode));
    return TDI_INVALID_ARG;
  }
  return idle_table_.idleTablePollModeSet(enable);
}

tdi_status_t TableAttributes::idleTableNotifyModeSet(
    const bool &enable,
    const IdleTmoExpiryCb &callback,
    const uint32_t &ttl_query_interval,
    const uint32_t &max_ttl,
    const uint32_t &min_ttl,
    const void *cookie) {
  if (attributeTypeGet() !=
      static_cast<tdi_attributes_type_e>(
          TDI_TOFINO_ATTRIBUTES_TYPE_IDLE_TABLE_RUNTIME)) {
    LOG_ERROR("%s:%d Can't set Idle Table Attribute with Object of type %d",
              __func__,
              __LINE__,
              static_cast<int>(attributeTypeGet()));
    return TDI_INVALID_ARG;
  }
  const auto idle_mode = idleTableModeGet();
  if (idle_mode != TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_NOTIFY_MODE) {
    LOG_ERROR(
        "%s:%d Can't set Idle Table Notify Mode with Attribute object of type "
        "%d",
        __func__,
        __LINE__,
        static_cast<int>(idle_mode));
    return TDI_INVALID_ARG;
  }
  return idle_table_.idleTableNotifyModeSet(
      enable, callback, nullptr, ttl_query_interval, max_ttl, min_ttl, cookie);
}
tdi_status_t TableAttributes::idleTableGet(
    tdi_tofino_attributes_idle_table_mode_e *mode,
    bool *enable,
    IdleTmoExpiryCb *callback,
    uint32_t *ttl_query_interval,
    uint32_t *max_ttl,
    uint32_t *min_ttl,
    void **cookie) const {
  if (attributeTypeGet() !=
      static_cast<tdi_attributes_type_e>(
          TDI_TOFINO_ATTRIBUTES_TYPE_IDLE_TABLE_RUNTIME)) {
    LOG_ERROR("%s:%d Can't get Idle Table Attribute with Object of type %d",
              __func__,
              __LINE__,
              static_cast<int>(attributeTypeGet()));
    return TDI_INVALID_ARG;
  }
  return idle_table_.idleTableGet(mode,
                                  enable,
                                  callback,
                                  nullptr,
                                  ttl_query_interval,
                                  max_ttl,
                                  min_ttl,
                                  cookie);
}

tdi_status_t TableAttributes::dynKeyMaskSet(
    const std::unordered_map<tdi_id_t, std::vector<uint8_t>> &field_mask) {
  if (attributeTypeGet() != static_cast<tdi_attributes_type_e>(
                                TDI_TOFINO_ATTRIBUTES_TYPE_DYNAMIC_KEY_MASK)) {
    LOG_ERROR(
        "%s:%d Can't set Dynamic Key Mask Attribute with Object of type %d",
        __func__,
        __LINE__,
        static_cast<int>(attributeTypeGet()));
    return TDI_INVALID_ARG;
  }
  return dyn_key_mask_.dynKeyMaskParamsSet(field_mask);
}

tdi_status_t TableAttributes::dynKeyMaskGet(
    std::unordered_map<tdi_id_t, std::vector<uint8_t>> *field_mask) const {
  if (attributeTypeGet() != static_cast<tdi_attributes_type_e>(
                                TDI_TOFINO_ATTRIBUTES_TYPE_DYNAMIC_KEY_MASK)) {
    LOG_ERROR(
        "%s:%d Can't get Dynamic Key Mask Attribute with Object of type %d",
        __func__,
        __LINE__,
        static_cast<int>(attributeTypeGet()));
    return TDI_INVALID_ARG;
  }
  return dyn_key_mask_.dynKeyMaskParamsGet(field_mask);
}

tdi_status_t TableAttributes::meterByteCountAdjSet(const int &byte_count) {
  if (attributeTypeGet() !=
      static_cast<tdi_attributes_type_e>(
          TDI_TOFINO_ATTRIBUTES_TYPE_METER_BYTE_COUNT_ADJ)) {
    LOG_ERROR(
        "%s:%d Can't set Meter Byte Count Adjust Attribute with Object of type "
        "%d",
        __func__,
        __LINE__,
        static_cast<int>(attributeTypeGet()));
    return TDI_INVALID_ARG;
  }
  return meter_bytecount_adj_.byteCountAdjSet(byte_count);
}
tdi_status_t TableAttributes::meterByteCountAdjGet(int *byte_count) const {
  if (attributeTypeGet() !=
      static_cast<tdi_attributes_type_e>(
          TDI_TOFINO_ATTRIBUTES_TYPE_METER_BYTE_COUNT_ADJ)) {
    LOG_ERROR(
        "%s:%d Can't get Meter Byte Count Adjust Attribute with Object of type "
        "%d",
        __func__,
        __LINE__,
        static_cast<int>(attributeTypeGet()));
    return TDI_INVALID_ARG;
  }
  return meter_bytecount_adj_.byteCountAdjGet(byte_count);
}

tdi_status_t TableAttributes::idleTableNotifyModeSetCFrontend(
    const bool &enable,
    tdi_idle_tmo_expiry_cb callback_c,
    const uint32_t &ttl_query_interval,
    const uint32_t &max_ttl,
    const uint32_t &min_ttl,
    const void *cookie) {
  if (attributeTypeGet() !=
      static_cast<tdi_attributes_type_e>(
          TDI_TOFINO_ATTRIBUTES_TYPE_IDLE_TABLE_RUNTIME)) {
    LOG_ERROR("%s:%d Can't set Idle Table Attribute with Object of type %d",
              __func__,
              __LINE__,
              static_cast<int>(attributeTypeGet()));
    return TDI_INVALID_ARG;
  }
  const auto idle_mode = idleTableModeGet();
  if (idle_mode != TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_NOTIFY_MODE) {
    LOG_ERROR(
        "%s:%d Can't set Idle Table Notify Mode with Attribute object of type "
        "%d",
        __func__,
        __LINE__,
        static_cast<int>(idle_mode));
    return TDI_INVALID_ARG;
  }
  return idle_table_.idleTableNotifyModeSet(enable,
                                            nullptr,
                                            callback_c,
                                            ttl_query_interval,
                                            max_ttl,
                                            min_ttl,
                                            cookie);
}
tdi_status_t TableAttributes::idleTableGetCFrontend(
    tdi_tofino_attributes_idle_table_mode_e *mode,
    bool *enable,
    tdi_idle_tmo_expiry_cb *callback_c,
    uint32_t *ttl_query_interval,
    uint32_t *max_ttl,
    uint32_t *min_ttl,
    void **cookie) const {
  if (attributeTypeGet() !=
      static_cast<tdi_attributes_type_e>(
          TDI_TOFINO_ATTRIBUTES_TYPE_IDLE_TABLE_RUNTIME)) {
    LOG_ERROR("%s:%d Can't get Idle Table Attribute with Object of type %d",
              __func__,
              __LINE__,
              static_cast<int>(attributeTypeGet()));
    return TDI_INVALID_ARG;
  }
  return idle_table_.idleTableGet(mode,
                                  enable,
                                  nullptr,
                                  callback_c,
                                  ttl_query_interval,
                                  max_ttl,
                                  min_ttl,
                                  cookie);
}

tdi_status_t TableAttributes::portStatusChangeNotifSet(
    const bool &enable,
    const PortStatusNotifCb &callback,
    const tdi_port_status_chg_cb &callback_c,
    const void *cookie) {
  if (attributeTypeGet() != static_cast<tdi_attributes_type_e>(
                                TDI_TOFINO_ATTRIBUTES_TYPE_PORT_STATUS_NOTIF)) {
    LOG_ERROR("%s:%d Can't set Port Cfg Notif Attribute with Object of type %d",
              __func__,
              __LINE__,
              static_cast<int>(attributeTypeGet()));
    return TDI_INVALID_ARG;
  }
  return port_status_change_.portStatusChgCbSet(
      enable, callback, callback_c, cookie);
}

tdi_status_t TableAttributes::portStatusChangeNotifGet(
    bool *enable,
    PortStatusNotifCb *callback,
    tdi_port_status_chg_cb *callback_c,
    void **cookie) const {
  if (attributeTypeGet() != static_cast<tdi_attributes_type_e>(
                                TDI_TOFINO_ATTRIBUTES_TYPE_PORT_STATUS_NOTIF)) {
    LOG_ERROR("%s:%d Can't get Port Cfg Notif Attribute with Object of type %d",
              __func__,
              __LINE__,
              static_cast<int>(attributeTypeGet()));
    return TDI_INVALID_ARG;
  }
  return port_status_change_.portStatusChgCbGet(
      enable, callback, callback_c, cookie);
}

tdi_status_t TableAttributes::portStatusChangeNotifSetCFrontend(
    const bool &enable, tdi_port_status_chg_cb callback_c, const void *cookie) {
  if (attributeTypeGet() != static_cast<tdi_attributes_type_e>(
                                TDI_TOFINO_ATTRIBUTES_TYPE_PORT_STATUS_NOTIF)) {
    LOG_ERROR("%s:%d Can't set port cfg Table Attribute with Object of type %d",
              __func__,
              __LINE__,
              static_cast<int>(attributeTypeGet()));
    return TDI_INVALID_ARG;
  }
  return port_status_change_.portStatusChgCbSet(
      enable, nullptr, callback_c, cookie);
}

tdi_status_t TableAttributes::portStatusChangeNotifGetCFrontend(
    bool *enable, tdi_port_status_chg_cb *callback_c, void **cookie) const {
  if (attributeTypeGet() != static_cast<tdi_attributes_type_e>(
                                TDI_TOFINO_ATTRIBUTES_TYPE_PORT_STATUS_NOTIF)) {
    LOG_ERROR("%s:%d Can't get port cfg Table Attribute with Object of type %d",
              __func__,
              __LINE__,
              static_cast<int>(attributeTypeGet()));
    return TDI_INVALID_ARG;
  }
  return port_status_change_.portStatusChgCbGet(
      enable, nullptr, callback_c, cookie);
}

tdi_status_t TableAttributes::portStatPollIntvlMsSet(
    const uint32_t &poll_intvl_ms) {
  if (attributeTypeGet() !=
      static_cast<tdi_attributes_type_e>(
          TDI_TOFINO_ATTRIBUTES_TYPE_PORT_STAT_POLL_INTVL_MS)) {
    LOG_ERROR(
        "%s:%d Can't set Port Stat Poll Intvl Attribute with Object of type %d",
        __func__,
        __LINE__,
        static_cast<int>(attributeTypeGet()));
    return TDI_INVALID_ARG;
  }
  return port_stat_poll_.portStatPollIntvlParamSet(poll_intvl_ms);
}

tdi_status_t TableAttributes::portStatPollIntvlMsGet(
    uint32_t *poll_intvl_ms) const {
  if (attributeTypeGet() !=
      static_cast<tdi_attributes_type_e>(
          TDI_TOFINO_ATTRIBUTES_TYPE_PORT_STAT_POLL_INTVL_MS)) {
    LOG_ERROR(
        "%s:%d Can't get Port Stat Poll Intvl Attribute with Object of type %d",
        __func__,
        __LINE__,
        static_cast<int>(attributeTypeGet()));
    return TDI_INVALID_ARG;
  }
  return port_stat_poll_.portStatPollIntvlParamGet(poll_intvl_ms);
}

tdi_status_t TableAttributes::preGlobalRidSet(const uint32_t &global_rid) {
  if (attributeTypeGet() != static_cast<tdi_attributes_type_e>(
                                TDI_TOFINO_ATTRIBUTES_TYPE_PRE_DEVICE_CONFIG)) {
    LOG_ERROR("%s:%d Can't set PRE Global RID Attribute with Object of type %d",
              __func__,
              __LINE__,
              static_cast<int>(attributeTypeGet()));
    return TDI_INVALID_ARG;
  }

  // Actual global RID size in MC mgr is 16 bits. So, make sure
  // the value is valid.
  auto size_in_bits = sizeof(uint16_t) * 8;
  auto max_limit = (1U << size_in_bits) - 1;
  if (global_rid > max_limit) {
    LOG_ERROR(
        "%s:%d Can't set PRE Global RID Attribute, value %u greater "
        "than max limit %u",
        __func__,
        __LINE__,
        global_rid,
        max_limit);
    return TDI_INVALID_ARG;
  }

  return pre_device_config_.preGlobalRidParamSet(global_rid);
}

tdi_status_t TableAttributes::preGlobalRidGet(uint32_t *global_rid) const {
  if (attributeTypeGet() != static_cast<tdi_attributes_type_e>(
                                TDI_TOFINO_ATTRIBUTES_TYPE_PRE_DEVICE_CONFIG)) {
    LOG_ERROR("%s:%d Can't get PRE Global RID Attribute with Object of type %d",
              __func__,
              __LINE__,
              static_cast<int>(attributeTypeGet()));
    return TDI_INVALID_ARG;
  }
  return pre_device_config_.preGlobalRidParamGet(global_rid);
}

tdi_status_t TableAttributes::prePortProtectionSet(const bool &enable) {
  if (attributeTypeGet() != static_cast<tdi_attributes_type_e>(
                                TDI_TOFINO_ATTRIBUTES_TYPE_PRE_DEVICE_CONFIG)) {
    LOG_ERROR(
        "%s:%d Can't set PRE Port Protection Attribute with Object of type %d",
        __func__,
        __LINE__,
        static_cast<int>(attributeTypeGet()));
    return TDI_INVALID_ARG;
  }
  return pre_device_config_.prePortProtectionParamSet(enable);
}

tdi_status_t TableAttributes::prePortProtectionGet(bool *enable) const {
  if (attributeTypeGet() != static_cast<tdi_attributes_type_e>(
                                TDI_TOFINO_ATTRIBUTES_TYPE_PRE_DEVICE_CONFIG)) {
    LOG_ERROR(
        "%s:%d Can't get PRE Port Protection Attribute with Object of type %d",
        __func__,
        __LINE__,
        static_cast<int>(attributeTypeGet()));
    return TDI_INVALID_ARG;
  }
  return pre_device_config_.prePortProtectionParamGet(enable);
}

tdi_status_t TableAttributes::preFastFailoverSet(const bool &enable) {
  if (attributeTypeGet() != static_cast<tdi_attributes_type_e>(
                                TDI_TOFINO_ATTRIBUTES_TYPE_PRE_DEVICE_CONFIG)) {
    LOG_ERROR(
        "%s:%d Can't set PRE Fast Failover Attribute with Object of type %d",
        __func__,
        __LINE__,
        static_cast<int>(attributeTypeGet()));
    return TDI_INVALID_ARG;
  }
  return pre_device_config_.preFastFailoverParamSet(enable);
}

tdi_status_t TableAttributes::preFastFailoverGet(bool *enable) const {
  if (attributeTypeGet() != static_cast<tdi_attributes_type_e>(
                                TDI_TOFINO_ATTRIBUTES_TYPE_PRE_DEVICE_CONFIG)) {
    LOG_ERROR(
        "%s:%d Can't get PRE Fast Failover Attribute with Object of type %d",
        __func__,
        __LINE__,
        static_cast<int>(attributeTypeGet()));
    return TDI_INVALID_ARG;
  }
  return pre_device_config_.preFastFailoverParamGet(enable);
}

tdi_status_t TableAttributes::preMaxNodesBeforeYieldSet(const uint32_t &count) {
  if (attributeTypeGet() != static_cast<tdi_attributes_type_e>(
                                TDI_TOFINO_ATTRIBUTES_TYPE_PRE_DEVICE_CONFIG)) {
    LOG_ERROR(
        "%s:%d Can't set PRE Max Nodes Before Yield Attribute with Object of "
        "type %d",
        __func__,
        __LINE__,
        static_cast<int>(attributeTypeGet()));
    return TDI_INVALID_ARG;
  }
  return pre_device_config_.preMaxNodesBeforeYieldParamSet(count);
}

tdi_status_t TableAttributes::preMaxNodesBeforeYieldGet(uint32_t *count) const {
  if (attributeTypeGet() != static_cast<tdi_attributes_type_e>(
                                TDI_TOFINO_ATTRIBUTES_TYPE_PRE_DEVICE_CONFIG)) {
    LOG_ERROR(
        "%s:%d Can't get PRE Max Nodes Before Yield Attribute with Object of "
        "type %d",
        __func__,
        __LINE__,
        static_cast<int>(attributeTypeGet()));
    return TDI_INVALID_ARG;
  }
  return pre_device_config_.preMaxNodesBeforeYieldParamGet(count);
}

tdi_status_t TableAttributes::preMaxNodeThresholdSet(
    const uint32_t &node_count, const uint32_t &node_port_lag_count) {
  if (attributeTypeGet() != static_cast<tdi_attributes_type_e>(
                                TDI_TOFINO_ATTRIBUTES_TYPE_PRE_DEVICE_CONFIG)) {
    LOG_ERROR(
        "%s:%d Can't set PRE Max Node Threshold Attribute with Object of type "
        "%d",
        __func__,
        __LINE__,
        static_cast<int>(attributeTypeGet()));
    return TDI_INVALID_ARG;
  }
  return pre_device_config_.preMaxNodeThresholdParamSet(node_count,
                                                        node_port_lag_count);
}

tdi_status_t TableAttributes::preMaxNodeThresholdGet(
    uint32_t *node_count, uint32_t *node_port_lag_count) const {
  if (attributeTypeGet() != static_cast<tdi_attributes_type_e>(
                                TDI_TOFINO_ATTRIBUTES_TYPE_PRE_DEVICE_CONFIG)) {
    LOG_ERROR(
        "%s:%d Can't get PRE Max Node Threshold Attribute with Object of type "
        "%d",
        __func__,
        __LINE__,
        static_cast<int>(attributeTypeGet()));
    return TDI_INVALID_ARG;
  }
  return pre_device_config_.preMaxNodeThresholdParamGet(node_count,
                                                        node_port_lag_count);
}

tdi_status_t TableAttributes::selectorUpdateCbSet(
    const bool &enable,
    const std::shared_ptr<tdi::Session> session,
    const SelUpdateCb &callback_fn,
    const void *cookie) {
  if (attributeTypeGet() !=
      static_cast<tdi_attributes_type_e>(
          TDI_TOFINO_ATTRIBUTES_TYPE_SELECTOR_UPDATE_CALLBACK)) {
    LOG_ERROR(
        "%s:%d Can't set Selector update callback Attribute with Object of "
        "type "
        "%d",
        __func__,
        __LINE__,
        static_cast<int>(attributeTypeGet()));
    return TDI_INVALID_ARG;
  }
  selector_update_callback_.paramSet(
      enable, session, callback_fn, nullptr /* c_cb */, cookie);

  return TDI_SUCCESS;
}

tdi_status_t TableAttributes::selectorUpdateCbSetCFrontend(
    const bool &enable,
    const std::shared_ptr<tdi::Session> session,
    const tdi_selector_table_update_cb &callback_fn,
    const void *cookie) {
  if (attributeTypeGet() !=
      static_cast<tdi_attributes_type_e>(
          TDI_TOFINO_ATTRIBUTES_TYPE_SELECTOR_UPDATE_CALLBACK)) {
    LOG_ERROR(
        "%s:%d Can't set Selector update callback Attribute with Object of "
        "type "
        "%d",
        __func__,
        __LINE__,
        static_cast<int>(attributeTypeGet()));
    return TDI_INVALID_ARG;
  }
  selector_update_callback_.paramSet(
      enable, session, nullptr /* cpp_cb */, callback_fn, cookie);

  return TDI_SUCCESS;
}

tdi_status_t TableAttributes::selectorUpdateCbGet(bool *enable,
                                                  tdi::Session **session,
                                                  SelUpdateCb *callback_fn,
                                                  void **cookie) const {
  if (attributeTypeGet() !=
      static_cast<tdi_attributes_type_e>(
          TDI_TOFINO_ATTRIBUTES_TYPE_SELECTOR_UPDATE_CALLBACK)) {
    LOG_ERROR(
        "%s:%d Can't get Selector update callback Attribute with Object of "
        "type "
        "%d",
        __func__,
        __LINE__,
        static_cast<int>(attributeTypeGet()));
    return TDI_INVALID_ARG;
  }
  tdi_selector_table_update_cb unused_fn;
  selector_update_callback_.paramGet(
      enable, session, callback_fn, &unused_fn /* c_cb */, cookie);

  (void)unused_fn;
  return TDI_SUCCESS;
}

tdi_status_t TableAttributes::selectorUpdateCbGetCFrontend(
    bool *enable,
    tdi::Session **session,
    tdi_selector_table_update_cb *callback_fn,
    void **cookie) const {
  if (attributeTypeGet() !=
      static_cast<tdi_attributes_type_e>(
          TDI_TOFINO_ATTRIBUTES_TYPE_SELECTOR_UPDATE_CALLBACK)) {
    LOG_ERROR(
        "%s:%d Can't get Selector update callback Attribute with Object of "
        "type "
        "%d",
        __func__,
        __LINE__,
        static_cast<int>(attributeTypeGet()));
    return TDI_INVALID_ARG;
  }
  SelUpdateCb unused_fn;
  selector_update_callback_.paramGet(
      enable, session, &unused_fn /* cpp_cb*/, callback_fn, cookie);

  (void)unused_fn;
  return TDI_SUCCESS;
}

void TableAttributes::selectorUpdateCbInternalSet(
    const std::tuple<bool,
                     std::weak_ptr<tdi::Session>,
                     SelUpdateCb,
                     tdi_selector_table_update_cb,
                     const void *> &t) {
  return selector_update_callback_.paramSetInternal(t);
}

std::tuple<bool,
           std::weak_ptr<tdi::Session>,
           SelUpdateCb,
           tdi_selector_table_update_cb,
           const void *>
TableAttributes::selectorUpdateCbInternalGet() const {
  return selector_update_callback_.paramGetInternal();
}

}  // namespace tofino
}  // namespace tna
}  // namespace tdi
