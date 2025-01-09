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


/** @file tdi_tofino_attributes.hpp
 *
 *  @brief Contains TDI Attributes related public headers for tofino C++
 */

#ifndef _TDI_TOFINO_ATTRIBUTES_HPP
#define _TDI_TOFINO_ATTRIBUTES_HPP

#include <bitset>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <tdi/common/tdi_defs.h>

#include <tdi/common/tdi_table_key.hpp>
#include <tdi/common/tdi_session.hpp>

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief IdleTable Mode
 */
enum tdi_tofino_attributes_idle_table_mode_e {
  /** Idle poll mode. When set, entry_hit_state on MAT entries can be
     queried to check idle time */
  TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_POLL_MODE = 0,
  /** Idle notify mode. Can be used to set CB for idletimeout on a MAT */
  TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_NOTIFY_MODE = 1,
  TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_INVALID_MODE = 2
};

enum tdi_tofino_attributes_idle_table_field_type_e {
  TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_FIELD_TYPE_MODE = TDI_ATTRIBUTES_FIELD_BEGIN,
  TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_FIELD_TYPE_ENABLE,
  TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_FIELD_TYPE_CALLBACK_CPP,
  TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_FIELD_TYPE_CALLBACK_C,
  TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_FIELD_TYPE_TTL_QUERY_INTVL,
  TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_FIELD_TYPE_MAX_TTL,
  TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_FIELD_TYPE_MIN_TTL,
  TDI_TOFINO_ATTRIBUTES_IDLE_TABLE_FIELD_TYPE_COOKIE,
};

/**
 * @brief Pipe Entry scope
 */
enum tdi_tofino_attributes_entry_scope_e {
  /** Set scope to all pipelines of current profile for this table. Turns
     table to symmetric. Default mode of tables */
  TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_ALL_PIPELINES = 0,
  /** Set scope to a single logical pipe in this profile for this table.
      Turns table to assymmetric */
  TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_SINGLE_PIPELINE = 1,
  /** Set scope to user defined scope in this profile for this table.
      Turns table to assymmetric but can be used to group some pipes
      together and hence can be used differently from single scope */
  TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_USER_DEFINED = 2
};

/**
 * @brief Gress Scope. Similar to pipe scope but for gress
 */
enum tdi_tofino_attributes_gress_scope_e {
  /** Both ingress and egress in scope */
  TDI_TOFINO_ATTRIBUTES_GRESS_SCOPE_ALL_GRESS,
  /** Either Ingress or Egress in scope */
  TDI_TOFINO_ATTRIBUTES_GRESS_SCOPE_SINGLE_GRESS
};

/**
 * @brief Parser Scope. Similar to pipe_scope bit for parser
 */
enum tdi_tofino_attributes_parser_scope_e {
  /** All parsers in scope*/
  TDI_TOFINO_ATTRIBUTES_PARSER_SCOPE_ALL_PARSERS_IN_PIPE,
  /** Single parser scope*/
  TDI_TOFINO_ATTRIBUTES_PARSER_SCOPE_SINGLE_PARSER
};

/**
 * @brief Gress Target. Similar to Pipe ID but for gress
 */
enum tdi_tofino_attributes_gress_target_e {
  /** Ingress */
  TDI_TOFINO_ATTRIBUTES_GRESS_TARGET_INGRESS,
  /** Egress */
  TDI_TOFINO_ATTRIBUTES_GRESS_TARGET_EGRESS,
  /** All gress */
  TDI_TOFINO_ATTRIBUTES_GRESS_TARGET_ALL = 0xFF
};

enum tdi_tofino_attributes_entry_scope_field_type_e {
  TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_FIELD_TYPE_ENTRY_SCOPE_TYPE =
      TDI_ATTRIBUTES_FIELD_BEGIN,
  TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_FIELD_TYPE_ENTRY_SCOPE_ARGS,
  TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_FIELD_TYPE_GRESS_SCOPE_TYPE,
  TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_FIELD_TYPE_GRESS_TARGET,
  TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_FIELD_TYPE_PARSER_SCOPE_TYPE,
};

enum tdi_tofino_attributes_meter_byte_count_adjust_field_type_e {
  TDI_TOFINO_ATTRIBUTES_METER_BYTE_COUNT_ADJUST_FIELD_TYPE_BYTE_COUNT =
      TDI_ATTRIBUTES_FIELD_BEGIN,
  TDI_TOFINO_ATTRIBUTES_METER_BYTE_COUNT_ADJUST_FIELD_TYPE_IS_POSITIVE,
};

// For Dynamic Key Mask, the field IDs are the same as the Key field IDs

enum tdi_tofino_attributes_port_status_change_field_type_e {
  TDI_TOFINO_ATTRIBUTES_PORT_STATUS_CHANGE_FIELD_TYPE_ENABLE =
      TDI_ATTRIBUTES_FIELD_BEGIN,
  TDI_TOFINO_ATTRIBUTES_PORT_STATUS_CHANGE_FIELD_TYPE_CALLBACK_CPP,
  TDI_TOFINO_ATTRIBUTES_PORT_STATUS_CHANGE_FIELD_TYPE_CALLBACK_C,
  TDI_TOFINO_ATTRIBUTES_PORT_STATUS_CHANGE_FIELD_TYPE_COOKIE,
};

enum tdi_tofino_attributes_port_stat_poll_intvl_ms_field_type_e {
  TDI_TOFINO_ATTRIBUTES_PORT_STAT_POLL_INTVL_MS_FIELD_TYPE_VALUE =
      TDI_ATTRIBUTES_FIELD_BEGIN,
};

#ifdef __cplusplus
}
#endif

namespace tdi {
namespace tna {
namespace tofino {

/**
 * @brief IdleTimeout Callback
 * @param[in] dev_tgt Device target
 * @param[in] key Table Key
 * @param[in] cookie User provided cookie during cb registration
 */
typedef void (*IdleTmoExpiryCb)(const Target &dev_tgt,
                                const TableKey *key,
                                void *cookie);

/**
 * @brief PortStatusChange Callback
 * @param[in] dev_id Device ID
 * @param[in] key Port Table Key
 * @param[in] port_up If port is up
 * @param[in] cookie User provided cookie during cb registration
 */
typedef void (*PortStatusNotifCb)(const tdi_dev_id_t &dev_id,
                                  const TableKey *key,
                                  const bool &port_up,
                                  void *cookie);

/**
 * @brief Selector Table Update Callback. This can be used to get notification
 * of data plane triggered Sel table update
 *
 * @param[in] session shared_ptr to session
 * @param[in] dev_tgt Device target
 * @param[in] cookie User provided cookie during cb registration
 * @param[in] sel_grp_id Selector-grp ID which was updated
 * @param[in] act_mbr_id action-mbr ID which was updated
 * @param[in] logical_entry_index Table logical entry index
 * @param[in] is_add If the operation was add or del
 */
typedef std::function<void(const std::shared_ptr<Session> session,
                           const Target &dev_tgt,
                           const void *cookie,
                           const tdi_id_t &sel_grp_id,
                           const tdi_id_t &act_mbr_id,
                           const int &logical_entry_index,
                           const bool &is_add)>
    SelUpdateCb;

}  // namespace tofino
}  // namespace tna
}  // namespace tdi

#endif  // __TDI_ATTRIBUTES_HPP
