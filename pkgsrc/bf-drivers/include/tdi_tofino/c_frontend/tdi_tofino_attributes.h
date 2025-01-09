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


/** @file tdi_tofino_attributes.h
 *
 *  @brief Contains TDI Attributes related public headers for tofino C
 */
#ifndef _TDI_TOFINO_ATTRIBUTES_H
#define _TDI_TOFINO_ATTRIBUTES_H

#include <tdi/common/tdi_defs.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IdleTimeout Callback. Users do not need to free the key. It will
 * be freed by TDI after the callback is done.
 * @param[in] dev_tgt Device target
 * @param[in] key Table Key
 * @param[in] cookie User provided cookie during cb registration
 */
typedef void (*tdi_idle_tmo_expiry_cb)(tdi_target_hdl *dev_tgt,
                                       const tdi_table_key_hdl *key,
                                       void *cookie);

/**
 * @brief PortStatusChange Callback. Users do not need to free the key. It
 * will be freed by TDI after callback is done
 * @param[in] dev_id Device ID
 * @param[in] key Port Table Key hdl
 * @param[in] port_up If port is up
 * @param[in] cookie User provided cookie during cb registration
 */
typedef void (*tdi_port_status_chg_cb)(tdi_dev_id_t dev_id,
                                       const tdi_table_key_hdl *key,
                                       bool port_up,
                                       void *cookie);

/**
 * @brief Selector Table Update Callback. This can be used to get notification
 * of data plane triggered Sel table update
 *
 * @param[in] session Session
 * @param[in] dev_tgt Device target
 * @param[in] cookie User provided cookie during cb registration
 * @param[in] sel_grp_id Selector-grp ID which was updated
 * @param[in] act_mbr_id Action-mbr ID which was updated
 * @param[in] logical_entry_index Table logical entry index
 * @param[in] is_add If the operation was add or del
 */
typedef void (*tdi_selector_table_update_cb)(const tdi_session_hdl *session,
                                             const tdi_target_hdl *dev_tgt,
                                             const void *cookie,
                                             const tdi_id_t sel_grp_id,
                                             const tdi_id_t act_mbr_id,
                                             const int logical_table_index,
                                             const bool is_add);
#if 0

/**
 * @brief Set Table entry scope attribute
 *
 * @param[in] tbl_attr          Table attribute object handle
 * @param[in] symmetric_mode    True to enable symmetric mode
 *
 * @return Status of the API call
 */
bf_status_t tdi_attributes_entry_scope_symmetric_mode_set(
    tdi_table_attributes_hdl *tbl_attr, const bool symmetric_mode);

/**
 * @brief Get Table entry scope attribute
 *
 * @param[in] tbl_attr            Table attribute object handle
 * @param[out] is_symmetric_mode  Currently configured symmetric mode
 *
 * @return Status of the API call
 */
bf_status_t tdi_attributes_entry_scope_symmetric_mode_get(
    tdi_table_attributes_hdl *tbl_attr, bool *is_symmetric_mode);

/**
 * @brief Set IdleTable Poll Mode options in the Attributes Object
 *
 * @param[in] tbl_attr          Table attribute object handle
 * @param[in] enable            Flag to enable IdleTable
 *
 * @return Status of the API call
 */
bf_status_t tdi_attributes_idle_table_poll_mode_set(
    tdi_table_attributes_hdl *tbl_attr, const bool enable);

/**
 * @brief Set IdleTable Notify Mode options in the Attributes Object
 *
 * @param[in] tbl_attr            Table attribute object handle
 * @param[in] enable              Enable IdleTimeout table
 * @param[in] callback            Will be called when entry expires
 * @param[in] ttl_query_interval  Inverval for querying entry TTL
 * @param[in] max_ttl             Max. allowed entry TTL value - not used
 * @param[in] min_ttl             Min. allowed entry TTL value - not used
 * @param[in] cookie              Used with callbacks
 *
 * @return Status of the API call
 */
bf_status_t tdi_attributes_idle_table_notify_mode_set(
    tdi_table_attributes_hdl *tbl_attr,
    const bool enable,
    const tdi_idle_tmo_expiry_cb callback,
    const uint32_t ttl_query_interval,
    const uint32_t max_ttl,
    const uint32_t min_ttl,
    const void *cookie);

/**
 * @brief Get IdleTable configuration options in the Attributes Object
 * Only mode and enable params apply to poll mode.
 *
 * @param[in] tbl_attr             Table attribute object handle
 * @param[out] mode                IdleTable mode
 * @param[out] enable              IdleTimeout table enable
 * @param[out] callback            Will be called when entry expires
 * @param[out] ttl_query_interval  Inverval for querying entry TTL
 * @param[out] max_ttl             Max. allowed entry TTL value - not used
 * @param[out] min_ttl             Min. allowed entry TTL value - not used
 * @param[out] cookie              Used with callbacks
 *
 * @return Status of the API call
 */
bf_status_t tdi_attributes_idle_table_get(
    const tdi_table_attributes_hdl *tbl_attr,
    tdi_attributes_idle_table_mode_t *mode,
    bool *enable,
    tdi_idle_tmo_expiry_cb *callback,
    uint32_t *ttl_query_interval,
    uint32_t *max_ttl,
    uint32_t *min_ttl,
    void **cookie);

/**
 * @brief Set Port status notificaiton options in the Attributes Object
 *
 * @param[in] tbl_attr          Table attribute object handle
 * @param[in] enable            Port status notification enable
 * @param[in] callback          Will be called when port status chagnes
 * @param[in] cookie            Used with callbacks
 *
 * @return Status of the API call
 */
bf_status_t tdi_attributes_port_status_notify_set(
    tdi_table_attributes_hdl *tbl_attr,
    const bool enable,
    const tdi_port_status_chg_cb callback,
    const void *cookie);

/**
 * @brief Get Port status notificaiton options in the Attributes Object
 *
 * @param[in] tbl_attr          Table attribute object handle
 * @param[out] enable           Port status notification enable
 * @param[out] callback         Will be called when port status chagnes
 * @param[out] cookie           Used with callbacks
 *
 * @return Status of the API call
 */
bf_status_t tdi_attributes_port_status_notify_get(
    const tdi_table_attributes_hdl *tbl_attr,
    bool *enable,
    tdi_port_status_chg_cb *callback,
    void **cookie);

/**
 * @brief Set Port Stat Poll Interval in Millisecond in the Attributes Object
 *
 * @param[in] tbl_attr          Table attribute object handle
 * @param[in] poll_intvl_ms     Poll interval
 *
 * @return Status of the API call
 */
bf_status_t tdi_attributes_port_stats_poll_intv_set(
    tdi_table_attributes_hdl *tbl_attr, const uint32_t poll_intv_ms);

/**
 * @brief Get Port Stat Poll Interval in Millisecond in the Attributes Object
 *
 * @param[in] tbl_attr          Table attribute object handle
 * @param[out] poll_intvl_ms    Poll interval
 *
 * @return Status of the API call
 */
bf_status_t tdi_attributes_port_stats_poll_intv_get(
    tdi_table_attributes_hdl *tbl_attr, uint32_t *poll_intv_ms);

/**
 * @brief Set PRE Global RID in the PRE Attributes Object
 *
 * @param[in] tbl_attr          Table attribute object handle
 * @param[in] global_rid        Global rid value
 *
 * @return Status of the API call
 */
bf_status_t tdi_attributes_pre_global_rid_set(
    tdi_table_attributes_hdl *tbl_attr, uint32_t global_rid);

/**
 * @brief Get PRE Global RID in the PRE Attributes Object
 *
 * @param[in] tbl_attr          Table attribute object handle
 * @param[out] global_rid       Global rid value
 *
 * @return Status of the API call
 */
bf_status_t tdi_attributes_pre_global_rid_get(
    tdi_table_attributes_hdl *tbl_attr, uint32_t *global_rid);

/**
 * @brief Set PRE Port Protection Status in the PRE Attributes Object
 *
 * @param[in] tbl_attr          Table attribute object handle
 * @param[in] enable            Enable/Disable status value
 *
 * @return Status of the API call
 */
bf_status_t tdi_attributes_pre_port_protection_set(
    tdi_table_attributes_hdl *tbl_attr, bool enable);

/**
 * @brief Get PRE Port Protection Status in the PRE Attributes Object
 *
 * @param[in] tbl_attr          Table attribute object handle
 * @param[out] enable           Enable/Disable status value
 *
 * @return Status of the API call
 */
bf_status_t tdi_attributes_pre_port_protection_get(
    tdi_table_attributes_hdl *tbl_attr, bool *enable);

/**
 * @brief Set PRE Fast Failover Status in the PRE Attributes Object
 *
 * @param[in] tbl_attr          Table attribute object handle
 * @param[in] enable            Enable/Disable status value
 *
 * @return Status of the API call
 */
bf_status_t tdi_attributes_pre_fast_failover_set(
    tdi_table_attributes_hdl *tbl_attr, bool enable);

/**
 * @brief Get PRE Fast Failover Status in the PRE Attributes Object
 *
 * @param[in] tbl_attr          Table attribute object handle
 * @param[out] enable           Enable/Disable status value
 *
 * @return Status of the API call
 */
bf_status_t tdi_attributes_pre_fast_failover_get(
    tdi_table_attributes_hdl *tbl_attr, bool *enable);

/**
 * @brief Set PRE Max Nodes Before Yield in the PRE Attributes Object
 *
 * @param[in] tbl_attr          Table attribute object handle
 * @param[in] count             Max node before yield count value
 *
 * @return Status of the API call
 */
bf_status_t tdi_attributes_pre_max_nodes_before_yield_set(
    tdi_table_attributes_hdl *tbl_attr, uint32_t count);

/**
 * @brief Get PRE Max Nodes Before Yield in the PRE Attributes Object
 *
 * @param[in] tbl_attr          Table attribute object handle
 * @param[out] count            Max node before yield count value
 *
 * @return Status of the API call
 */
bf_status_t tdi_attributes_pre_max_nodes_before_yield_get(
    tdi_table_attributes_hdl *tbl_attr, uint32_t *count);

/**
 * @brief Set PRE Max Node Threshold in the PRE Attributes Object
 *
 * @param[in] tbl_attr             Table attribute object handle
 * @param[in] node_count           Node count value
 * @param[in] node_port_lag_count  Node port lag count value
 *
 * @return Status of the API call
 */
bf_status_t tdi_attributes_pre_max_nodes_threshold_set(
    tdi_table_attributes_hdl *tbl_attr,
    uint32_t node_count,
    uint32_t node_port_lag_count);

/**
 * @brief Get PRE Max Node Threshold in the PRE Attributes Object
 *
 * @param[in] tbl_attr             Table attribute object handle
 * @param[out] node_count          Node count value
 * @param[out] node_port_lag_count Node port lag count value
 *
 * @return Status of the API call
 */
bf_status_t tdi_attributes_pre_max_nodes_threshold_get(
    tdi_table_attributes_hdl *tbl_attr,
    uint32_t *node_count,
    uint32_t *node_port_lag_count);

/**
 * @brief Get Dynamic Key Mask supported fields number
 *
 * @param[in] tbl_attr          Table attribute object handle
 * @param[out] num_fields       Total number of supported fields
 *
 * @return Status of the API call
 */
bf_status_t tdi_attributes_dyn_key_mask_num_fields_get(
    tdi_table_attributes_hdl *tbl_attr, uint32_t *num_fields);

/**
 * @brief Get Dynamic Key Mask supported fields IDs
 *
 * @param[in] tbl_attr          Table attribute object handle
 * @param[out] field_ids       Total number of supported fields
 *
 * @return Status of the API call
 */
bf_status_t tdi_attributes_dyn_key_mask_fields_get(
    tdi_table_attributes_hdl *tbl_attr, uint32_t *field_ids);

/**
 * @brief Set Dynamic Key Mask bytes for a specific field
 *
 * @param[in] tbl_attr          Table attribute object handle
 * @param[in] field_id          Specifies key field
 * @param[in] bytes             Key field mask buffer
 * @param[in] num_bytes         Key field mask buffer size
 *
 * @return Status of the API call
 */
bf_status_t tdi_attributes_dyn_key_mask_set(
    tdi_table_attributes_hdl *tbl_attr,
    const tdi_id_t field_id,
    const uint8_t *bytes,
    const uint32_t num_bytes);

/**
 * @brief Get Dynamic Key Mask length for a specific field
 *
 * @param[in] tbl_attr          Table attribute object handle
 * @param[in] field_id          Specifies key field
 * @param[out] num_bytes        Configured key field mask buffer size
 *
 * @return Status of the API call
 */
bf_status_t tdi_attributes_dyn_key_mask_num_bytes_get(
    tdi_table_attributes_hdl *tbl_attr,
    tdi_id_t field_id,
    uint32_t *num_bytes);

/**
 * @brief Get Dynamic Key Mask for a specific field
 *
 * @param[in] tbl_attr          Table attribute object handle
 * @param[in] field_id          Specifies key field
 * @param[out] bytes            Key field mask buffer
 *
 * @return Status of the API call
 */
bf_status_t tdi_attributes_dyn_key_mask_bytes_get(
    tdi_table_attributes_hdl *tbl_attr, tdi_id_t field_id, uint8_t *bytes);

/**
 * @brief Set Dynamic Hashing Algorithm and Seed in the Attributes Object
 *
 * @param[in] tbl_attr          Table attribute object handle
 * @param[in] alg_hdl           Algorithm handle for hashing
 * @param[in] seed              Seed for hashing
 *
 * @return Status of the API call
 */
bf_status_t tdi_attributes_dyn_hashing_set(
    tdi_table_attributes_hdl *tbl_attr,
    const uint32_t alg_hdl,
    const uint64_t seed);

/**
 * @brief Get Dynamic Hashing Algorithm and Seed in the Attributes Object
 *
 * @param[in] tbl_attr          Table attribute object handle
 * @param[out] alg_hdl          Algorithm handle for hashing
 * @param[out] seed             Seed for hashing
 *
 * @return Status of the API call
 */
bf_status_t tdi_attributes_dyn_hashing_get(
    tdi_table_attributes_hdl *tbl_attr, uint32_t *alg_hdl, uint64_t *seed);

/**
 * @brief Set Meter Byte Count Adjust in the Attributes Object
 *
 * @param[in] tbl_attr          Table attribute object handle
 * @param[in] byte_count_adj    Number of adjust bytes for meter tables
 *
 * @return Status of the API call
 */
bf_status_t tdi_attributes_meter_byte_count_adjust_set(
    tdi_table_attributes_hdl *tbl_attr, const int byte_count_adj);

/**
 * @brief Get Meter Byte Count Adjust in the Attributes Object
 *
 * @param[in] tbl_attr          Table attribute object handle
 * @param[out] byte_count_adj   Number of adjust bytes for meter tables
 *
 * @return Status of the API call
 */
bf_status_t tdi_attributes_meter_byte_count_adjust_get(
    tdi_table_attributes_hdl *tbl_attr, int *byte_count_adj);

/**
 * @brief Set Selector Update Notification Callback
 *
 * @param[in] tbl_attr          Table attribute object handle
 * @param[in] enable            Flag to enable selector update notifications
 * @param[in] session           Session handle
 * @param[in] callback_fn       Callback on Selector table update
 * @param[in] cookie            User cookie
 *
 * @return Status of the API call
 */
bf_status_t tdi_attributes_selector_table_update_cb_set(
    tdi_table_attributes_hdl *tbl_attr,
    const bool enable,
    const tdi_session_hdl *session,
    const tdi_selector_table_update_cb callback,
    const void *cookie);

/**
 * @brief Get Selector Update Notification Callback
 *
 * @param[in] tbl_attr          Table attribute object handle
 * @param[out] enable           Enable Flag
 * @param[out] session          Session handle
 * @param[out] callback_fn      Callback fn set for Selector table update
 * @param[out] cookie           User cookie
 *
 * @return Status of the API call
 */
bf_status_t tdi_attributes_selector_table_update_cb_get(
    const tdi_table_attributes_hdl *tbl_attr,
    bool *enable,
    tdi_session_hdl **session,
    tdi_selector_table_update_cb *callback,
    void **cookie);

#endif

#ifdef __cplusplus
}
#endif

#endif  // _TDI_TABLE_ATTRIBUTES_H
