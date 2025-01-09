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


#ifndef _BF_RT_TM_TABLE_HELPER_HPP
#define _BF_RT_TM_TABLE_HELPER_HPP

namespace bfrt {

#define BFRT_TM_HLP_ACTION(action_name_)                                \
  action_name = #action_name_;                                          \
  status = w_table.actionIdGet(action_name, &(this->a_##action_name_)); \
  if (BF_SUCCESS != status) {                                           \
    break;                                                              \
  }

#define BFRT_TM_HLP_FIELD(field_name_)                              \
  field_name = #field_name_;                                        \
  status = w_table.popWorkField(                                    \
      field_name, do_action_id, wrk_fields, this->f_##field_name_); \
  if (BF_SUCCESS != status) {                                       \
    break;                                                          \
  }

// For data fields present only on some platforms or actions
#define BFRT_TM_HLP_FIELD_OPTIONAL(field_name_)                     \
  field_name = #field_name_;                                        \
  status = w_table.popWorkField(                                    \
      field_name, do_action_id, wrk_fields, this->f_##field_name_); \
  if (BF_OBJECT_NOT_FOUND == status) {                              \
    this->f_##field_name_ = 0;                                      \
    status = BF_SUCCESS;                                            \
  } else if (BF_SUCCESS != status) {                                \
    break;                                                          \
  }

#define BFRT_TM_HLP_GET_INT(field_name_, field_type_)            \
  if (this->f_##field_name_) {                                   \
    uint64_t field_tmp = 0;                                      \
    status = s_data.getValue(this->f_##field_name_, &field_tmp); \
    if (BF_SUCCESS != status) {                                  \
      err_data_id = this->f_##field_name_;                       \
      break;                                                     \
    }                                                            \
    this->field_name_ = static_cast<field_type_>(field_tmp);     \
  }

#define BFRT_TM_HLP_GET_VALUE(field_name_)                                 \
  if (this->f_##field_name_) {                                             \
    status = s_data.getValue(this->f_##field_name_, &(this->field_name_)); \
    if (BF_SUCCESS != status) {                                            \
      err_data_id = this->f_##field_name_;                                 \
      break;                                                               \
    }                                                                      \
  }

#define BFRT_TM_HLP_GET_ENCODED(field_name_, str_to_val_map_)    \
  if (this->f_##field_name_) {                                   \
    std::string field_str;                                       \
    status = s_data.getValue(this->f_##field_name_, &field_str); \
    if (BF_SUCCESS != status) {                                  \
      err_data_id = this->f_##field_name_;                       \
      break;                                                     \
    }                                                            \
    try {                                                        \
      this->field_name_ = this->str_to_val_map_.at(field_str);   \
    } catch (std::out_of_range &) {                              \
      status = BF_INVALID_ARG;                                   \
      err_data_id = this->f_##field_name_;                       \
      break;                                                     \
    }                                                            \
  }

#define BFRT_TM_HLP_SET_ENCODED(field_name_, val_to_str_map_)    \
  if (this->f_##field_name_) {                                   \
    std::string field_str;                                       \
    try {                                                        \
      field_str = this->val_to_str_map_.at(this->field_name_);   \
    } catch (std::out_of_range &) {                              \
      status = BF_UNEXPECTED;                                    \
      err_data_id = this->f_##field_name_;                       \
      break;                                                     \
    }                                                            \
    status = s_data->setValue(this->f_##field_name_, field_str); \
    if (BF_SUCCESS != status) {                                  \
      err_data_id = this->f_##field_name_;                       \
      break;                                                     \
    }                                                            \
  }

#define BFRT_TM_HLP_SET_INT(field_name_)                                 \
  if (this->f_##field_name_) {                                           \
    status = s_data->setValue(this->f_##field_name_,                     \
                              static_cast<uint64_t>(this->field_name_)); \
    if (BF_SUCCESS != status) {                                          \
      err_data_id = this->f_##field_name_;                               \
      break;                                                             \
    }                                                                    \
  }

#define BFRT_TM_HLP_SET_VALUE(field_name_)                               \
  if (this->f_##field_name_) {                                           \
    status = s_data->setValue(this->f_##field_name_, this->field_name_); \
    if (BF_SUCCESS != status) {                                          \
      err_data_id = this->f_##field_name_;                               \
      break;                                                             \
    }                                                                    \
  }

}  // namespace bfrt
#endif
