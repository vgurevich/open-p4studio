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


#ifndef INCLUDE_BF_SWITCH_BF_SWITCH_H__
#define INCLUDE_BF_SWITCH_BF_SWITCH_H__

#include "bf_switch_types.h"

/** @defgroup bf_switch BF Switch object API
 *  API functions, enums and structures to create/delete/set/get objects
 *  @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief List of features
 */
typedef enum _switch_feature_id_t {
  // clang-format off
  SWITCH_FEATURE_MIN                    = 0,
  SWITCH_FEATURE_COPP                   = SWITCH_FEATURE_MIN,
  SWITCH_FEATURE_STORM_CONTROL          = 1,
  SWITCH_FEATURE_STP                    = 2,
  SWITCH_FEATURE_MSTP                   = 3,
  SWITCH_FEATURE_INGRESS_RACL           = 4,
  SWITCH_FEATURE_ETRAP                  = 5,
  SWITCH_FEATURE_ETYPE_IN_ACL           = 6,
  SWITCH_FEATURE_IPV4_LOCAL_HOST        = 7,
  SWITCH_FEATURE_MULTICAST              = 8,
  SWITCH_FEATURE_MAC_PKT_CLASSIFICATION = 10,
  SWITCH_FEATURE_TCP_FLAGS_LOU          = 11,
  SWITCH_FEATURE_INGRESS_L4_PORT_RANGE  = 12,
  SWITCH_FEATURE_ACL_REDIRECT_NEXTHOP   = 13,
  SWITCH_FEATURE_INGRESS_TOS_MIRROR_ACL = 14,
  SWITCH_FEATURE_EGRESS_TOS_MIRROR_ACL  = 15,
  SWITCH_FEATURE_INGRESS_MAC_ACL        = 16,
  SWITCH_FEATURE_EGRESS_MAC_ACL         = 17,
  SWITCH_FEATURE_EGRESS_IP_ACL          = 18,
  SWITCH_FEATURE_INGRESS_PORT_MIRROR    = 19,
  SWITCH_FEATURE_EGRESS_PORT_MIRROR     = 20,
  SWITCH_FEATURE_INGRESS_MIRROR_ACL     = 21,
  SWITCH_FEATURE_EGRESS_MIRROR_ACL      = 22,
  SWITCH_FEATURE_RSPAN                  = 23,
  SWITCH_FEATURE_ERSPAN_TYPE2           = 24,
  SWITCH_FEATURE_ERSPAN_PLATFORM_INFO   = 25,
  SWITCH_FEATURE_INGRESS_QOS_MAP        = 26,
  SWITCH_FEATURE_INGRESS_IP_QOS_ACL     = 27,  // unused
  SWITCH_FEATURE_EGRESS_IP_QOS_ACL      = 28,
  SWITCH_FEATURE_INGRESS_MAC_QOS_ACL    = 29,
  SWITCH_FEATURE_EGRESS_MAC_QOS_ACL     = 30,
  SWITCH_FEATURE_INGRESS_ACL_METER      = 31,
  SWITCH_FEATURE_EGRESS_ACL_METER       = 32,
  SWITCH_FEATURE_INGRESS_PORT_METER     = 33,
  SWITCH_FEATURE_EGRESS_PORT_METER      = 34,
  SWITCH_FEATURE_ECN_ACL                = 35,
  SWITCH_FEATURE_WRED                   = 36,
  SWITCH_FEATURE_IPV4_TUNNEL            = 37,
  SWITCH_FEATURE_IPV6_TUNNEL            = 38,
  SWITCH_FEATURE_VXLAN                  = 39,
  SWITCH_FEATURE_IPINIP                 = 40,
  SWITCH_FEATURE_QUEUE_REPORT           = 41,
  SWITCH_FEATURE_DROP_REPORT            = 42,
  SWITCH_FEATURE_FLOW_REPORT            = 43,
  SWITCH_FEATURE_REPORT_SUPPRESSION     = 44,
  SWITCH_FEATURE_MLAG                   = 45,  // unused
  SWITCH_FEATURE_PTP                    = 46,
  SWITCH_FEATURE_QINQ                   = 47,
  SWITCH_FEATURE_QINQ_RIF               = 48,
  SWITCH_FEATURE_ACL_USER_META          = 49,
  SWITCH_FEATURE_PPG_STATS              = 50,
  SWITCH_FEATURE_PFC_WD                 = 51,
  SWITCH_FEATURE_ACL_REDIRECT_PORT      = 52,
  SWITCH_FEATURE_IPV6_LPM64             = 53,
  SWITCH_FEATURE_L3_UNICAST_SELF_FWD_CHECK = 54,
  SWITCH_FEATURE_MIRROR_METER           = 55,
  SWITCH_FEATURE_SFLOW                  = 56,
  SWITCH_FEATURE_SAME_MAC_CHECK         = 57,
  SWITCH_FEATURE_INGRESS_IPV4_ACL       = 58,
  SWITCH_FEATURE_INGRESS_IPV6_ACL       = 59,
  SWITCH_FEATURE_DTEL_IFA_CLONE         = 60,
  SWITCH_FEATURE_DTEL_IFA_EDGE          = 61,
  SWITCH_FEATURE_IPV6_FIB_LPM_TCAM      = 62,
  SWITCH_FEATURE_ACL_PORT_GROUP         = 63,
  SWITCH_FEATURE_NAT                    = 64,
  SWITCH_FEATURE_BASIC_NAT              = 65,
  SWITCH_FEATURE_NAPT                   = 66,
  SWITCH_FEATURE_FLOW_NAT               = 67,
  SWITCH_FEATURE_TUNNEL_ENCAP           = 68,
  SWITCH_FEATURE_CPU_BD_MAP             = 69,
  SWITCH_FEATURE_INT_V2                 = 70,
  SWITCH_FEATURE_TUNNEL_DECAP           = 71,
  SWITCH_FEATURE_NAT_NAPT_INDEX_INDIRECTION = 72,
  SWITCH_FEATURE_BD_LABEL_IN_EGRESS_ACL = 73,
  SWITCH_FEATURE_SHARED_ALPM            = 74,
  SWITCH_FEATURE_MPLS                   = 75,
  SWITCH_FEATURE_SRV6                   = 76,
  SWITCH_FEATURE_INNER_L2               = 77,
  SWITCH_FEATURE_PRE_INGRESS_ACL        = 78,
  SWITCH_FEATURE_SHARED_INGRESS_IP_ACL  = 79,
  SWITCH_FEATURE_TUNNEL_TTL_MODE        = 80,
  SWITCH_FEATURE_TUNNEL_QOS_MODE        = 81,
  SWITCH_FEATURE_INNER_HASH             = 82,
  SWITCH_FEATURE_PORT_ISOLATION         = 83,
  SWITCH_FEATURE_ASYMMETRIC_FOLDED_PIPELINE = 84,
  SWITCH_FEATURE_EGRESS_COPP            = 85,
  SWITCH_FEATURE_BD_LABEL_IN_INGRESS_ACL = 86,
  SWITCH_FEATURE_EGRESS_L4_PORT_RANGE   = 87,
  SWITCH_FEATURE_SFC                    = 88,
  SWITCH_FEATURE_L2_VXLAN               = 89,
  SWITCH_FEATURE_IPV6_HOST64            = 90,
  SWITCH_FEATURE_FIB_ACL_LABEL          = 91,
  SWITCH_FEATURE_IN_PORTS_IN_DATA       = 92,
  SWITCH_FEATURE_IN_PORTS_IN_MIRROR     = 93,
  SWITCH_FEATURE_OUT_PORTS              = 94,
  SWITCH_FEATURE_INGRESS_MIRROR_ACL_MIRROR_IN_OUT = 95,
  SWITCH_FEATURE_EGRESS_MIRROR_ACL_MIRROR_IN_OUT  = 96,
  SWITCH_FEATURE_IP_STATS               = 97,
  SWITCH_FEATURE_IPGRE                  = 98,
  SWITCH_FEATURE_ECMP_DEFAULT_HASH_OFFSET = 99,
  SWITCH_FEATURE_IPV6_ACL_UPPER64       = 100,
  SWITCH_FEATURE_INNER_DTEL             = 101,
  SWITCH_FEATURE_MULTIPLE_RIFS_PER_PORT = 102,
  SWITCH_FEATURE_FOLDED_SWITCH_PIPELINE = 103,
  SWITCH_FEATURE_TUNNEL_ECN_RFC_6040    = 104,
  SWITCH_FEATURE_BD_IN_INGRESS_ACL      = 105,
  SWITCH_FEATURE_BD_IN_EGRESS_ACL       = 106,
  SWITCH_FEATURE_UDT_TYPE_NEIGHBOR      = 107,
  SWITCH_FEATURE_INGRESS_MAC_IP_ACL_DENY_ACTION = 108,
  SWITCH_FEATURE_INGRESS_MAC_IP_ACL_TRANSIT_ACTION = 109,
  SWITCH_FEATURE_IPMC_DMAC_VALIDATION   = 110,
  SWITCH_FEATURE_EGRESS_SYSTEM_ACL_STATS = 111,
  SWITCH_FEATURE_INGRESS_ACL            = 112,
  SWITCH_FEATURE_EGRESS_ACL             = 113,
  SWITCH_FEATURE_INGRESS_IPV4_RACL      = 114,  // unused
  SWITCH_FEATURE_INGRESS_IPV6_RACL      = 115,  // unused
  SWITCH_FEATURE_PEER_LINK_TUNNEL_ISOLATION = 116,
  SWITCH_FEATURE_BFD_OFFLOAD            = 117,
  SWITCH_FEATURE_PKTGEN                 = 118,
  SWITCH_FEATURE_MAX
  // clang-format on
} switch_feature_id_t;

/**
 * @brief List of switch init types
 */
typedef enum _switch_start_type_t {
  SWITCH_START_TYPE_COLD_BOOT = 0,
  SWITCH_START_TYPE_WARM_BOOT = 1,
  SWITCH_START_TYPE_FAST_RECFG = 2,
  SWITCH_START_TYPE_UNKNOWN
} switch_start_type_t;

/*******************************************************************************
 * C frontend APIs
 ******************************************************************************/

/**
 * @brief Get object handle using key group lookup
 * \n C wrapper for \ref bf_switch_object_get
 *
 * @param[in] object_type object type
 * @param[in] attr_count number of attributes
 * @param[in] attr_list list of attributes used for lookup
 * @param[out] object_handle object handle obtained from lookup
 *
 * @retval SWITCH_STATUS_SUCCESS object found
 * @retval SWITCH_STATUS_ITEM_NOT_FOUND object not found
 * @retval SWITCH_STATUS_INVALID_KEY_GROUP invalid key group for lookup
 */
switch_status_t bf_switch_object_get_c(
    switch_object_type_t object_type,
    uint32_t attr_count,
    const switch_attribute_t *const attr_list,
    switch_object_id_t *const object_handle);

/**
 * @brief Create new object based on object_type and attributes
 * \n C wrapper for \ref bf_switch_object_create
 *
 * @param[in] object_type object type
 * @param[in] attr_count number of attributes
 * @param[in] attr_list list of attributes
 * @param[out] object_handle object handle after creation
 *
 * @retval SWITCH_STATUS_SUCCESS object created successfully
 * @retval SWITCH_STATUS_ITEM_ALREADY_EXISTS object exists and object_handle is
 *updated
 * @retval SWITCH_STATUS_INVALID_PARAMETER wrong list of attributes are passed
 * @retval SWITCH_STATUS_FAILURE failed to create object
 */
switch_status_t bf_switch_object_create_c(
    switch_object_type_t object_type,
    uint32_t attr_count,
    const switch_attribute_t *const attr_list,
    switch_object_id_t *const object_handle);

/**
 * @brief Delete object based on object_handle
 * \n C wrapper for \ref bf_switch_object_delete
 *
 * @param[in] object_handle object handle
 *
 * @retval SWITCH_STATUS_SUCCESS object deleted successfully
 * @retval SWITCH_STATUS_RESOURCE_IN_USE object is an attribute for another
 *object
 * @retval SWITCH_STATUS_FAILURE failed to delete object
 */
switch_status_t bf_switch_object_delete_c(switch_object_id_t object_handle);

/**
 * @brief Get object type by object_handle
 * \n C wrapper for \ref bf_switch_object_type_get
 *
 * @param[in] object_handle object handle
 * @param[out] object_type object type value filled in
 *
 * @retval SWITCH_STATUS_SUCCESS object type retrieved successfully
 * @retval SWITCH_STATUS_FAILURE failed to retrieve object type
 */
switch_status_t bf_switch_object_type_get_c(
    switch_object_id_t object_handle, switch_object_type_t *const object_type);

/**
 * @brief Set one attribute of an object
 * \n C wrapper for \ref bf_switch_attribute_set
 *
 * @param[in] object_handle object handle
 * @param[in] attr single attribute to set
 *
 * @retval SWITCH_STATUS_SUCCESS object deleted successfully
 * @retval SWITCH_STATUS_INVALID_PARAMETER invalid attribute type or value
 * @retval SWITCH_STATUS_FAILURE failed to delete object
 */
switch_status_t bf_switch_attribute_set_c(switch_object_id_t object_handle,
                                          const switch_attribute_t *const attr);

/**
 * @brief Get one attribute of an object. If the attribute type is a list,
 * the caller is responsible for free'ing attr->value.list.list
 * \n C wrapper for \ref bf_switch_attribute_get
 *
 * @param[in] object_handle object handle
 * @param[in] attr_id attribute id to get
 * @param[out] attr atrribute with type and value filled in
 *
 * @retval SWITCH_STATUS_SUCCESS object deleted successfully
 * @retval SWITCH_STATUS_FAILURE failed to delete object
 */
switch_status_t bf_switch_attribute_get_c(switch_object_id_t object_handle,
                                          switch_attr_id_t attr_id,
                                          switch_attribute_t *const attr);

/**
 * @brief Get list of counters supported by the object
 * \n C wrapper for \ref bf_switch_counters_get
 *
 * @param[in] object_handle object handle
 * @param[out] num_counters Total supported counters
 * @param[out] counters List of counters of size num_counters. Memory has
 * to be free'd by caller
 *
 * @retval SWITCH_STATUS_SUCCESS counters retrieved successfully
 * @retval SWITCH_STATUS_FAILURE failed to get counters
 */
switch_status_t bf_switch_counters_get_c(switch_object_id_t object_handle,
                                         uint16_t *num_counters,
                                         switch_counter_t **counters);

/**
 * @brief Clear counters for the object. In most cases, this sets value
 * to 0, if clearing is supported
 * \n C wrapper for \ref bf_switch_counters_clear
 *
 * @param[in] object_handle object handle
 * @param[in] cntrs_ids list of counter ids to be cleared
 * @param[in] cntrs_num number of counter ids in the list
 *
 * @retval SWITCH_STATUS_SUCCESS counters retrieved successfully
 * @retval SWITCH_STATUS_FAILURE failed to get counters
 */
switch_status_t bf_switch_counters_clear_c(switch_object_id_t object_handle,
                                           uint16_t *cntr_ids,
                                           uint32_t cntrs_num);

/**
 * @brief Clear all counters for the object. In most cases, this sets value
 * to 0, if clearing is supported
 * \n C wrapper for \ref bf_switch_counters_clear_all
 *
 * @param[in] object_handle object handle
 *
 * @retval SWITCH_STATUS_SUCCESS counters retrieved successfully
 * @retval SWITCH_STATUS_FAILURE failed to get counters
 */
switch_status_t bf_switch_counters_clear_all_c(
    switch_object_id_t object_handle);

/**
 * @brief Iterator helper function. This call gives the first object of a given
 * type. If valid handle then application can proceed to call \ref
 * bf_switch_get_next_handles_c
 * \n C wrapper for \ref bf_switch_get_first_handle
 *
 * @param[in] object_type Object type to iterate on
 * @param[out] object_handle First object handle of object_type
 *
 * @return
 */
switch_status_t bf_switch_get_first_handle_c(
    switch_object_type_t object_type, switch_object_id_t *const object_handle);

/**
 * @brief Get next set of handles after getting the first handle via \ref
 *bf_switch_get_first_handle_c
 * \n C wrapper for \ref bf_switch_get_next_handles
 * Caller allocates memory of size in_num_handles
 * If value of out_num_handles > in_num_handles, this API needs to be invoked
 *again with the last handle returned from this call
 *
 * @param object_handle first handle or last handle from previous get_next call
 * @param in_num_handles number of handles of retrieve
 * @param next_handles List of handles filled in
 * @param out_num_handles Actual count of number of handles that can return
 *
 * @retval SWITCH_STATUS_SUCCESS
 */
switch_status_t bf_switch_get_next_handles_c(
    switch_object_id_t object_handle,
    uint32_t in_num_handles,
    switch_object_id_t *const next_handles,
    uint32_t *const out_num_handles);

/**
 * @brief Get all object handles of a given object type. If no object handles
 * are found for a given object type, this call returns success with an emtpy
 * list
 * Caller is responsible to free memory allocated to out_handles
 * \n C wrapper for \ref bf_switch_get_all_handles
 *
 * @param[in] object_type Object type to iterate on
 * @param[out] out_handles List of handles returned
 * @param out_num_handles Actual count of number of handles returned
 *
 * @retval SWITCH_STATUS_SUCCESS
 */
switch_status_t bf_switch_get_all_handles_c(switch_object_type_t object_type,
                                            switch_object_id_t **out_handles,
                                            uint32_t *const out_num_handles);
/**
 * @brief Set log level per object. Log levels are defined in switch_verbosity_t
 * \n Passing object_type = 0, would set verbosity for all object types
 * \n C wrapper for \ref bf_switch_object_log_level_set
 *
 * @param[in] object_type object type
 * @param[in] verbosity verbosity
 *
 * @retval SWITCH_STATUS_SUCCESS
 */
switch_status_t bf_switch_object_log_level_set_c(
    switch_object_type_t object_type, switch_verbosity_t verbosity);

/**
 * @brief Set global logging level. This API overrides the settings of
 * \ref bf_switch_object_log_level_set_c per object
 * \n C wrapper for bf_switch_global_log_level_set
 *
 * @param[in] verbosity verbosity
 *
 * @retval SWITCH_STATUS_SUCCESS
 */
switch_status_t bf_switch_global_log_level_set_c(switch_verbosity_t verbosity);

/**
 * @brief Get table size and usage information
 * \n C wrapper for \ref bf_switch_table_info_get
 *
 * @param[in] table_id This is enum value from "device" object's "table"
 *attribute from the schema
 * @param[out] table_info switch_table_info_t object
 *
 * @retval SWITCH_STATUS_SUCCESS if valid table ID
 * @retval SWITCH_STATUS_INVALID_PARAMETER if invalid table ID is passed in
 */
switch_status_t bf_switch_table_info_get_c(uint64_t table_id,
                                           switch_table_info_t *table_info);

/**
 * @brief Device add routines
 * \n - Create a device object
 * \n - CPU port handle is created and attached to device cpu_port attribute
 *
 * @param[in] device Device id
 * @param[in] veth_port CPU port
 * @param[out] device_hdl device handle after creation
 *
 * @retval SWITCH_STATUS_SUCCESS device creation success
 * @retval SWITCH_STATUS_FAILURE failed to create device
 */
switch_status_t bf_switch_device_add(uint16_t device,
                                     const char *const veth_port,
                                     switch_object_id_t *device_hdl);

/**
 * @brief Entry point for bf_switch
 * \n - Calls internal SDK init functions specific to a P4 program
 * \n - Loads and parses the schema JSON
 * \n - Internally calls \ref bf_switch_device_add
 *
 * @param[in] device Device ID
 * @param[in] conf_file File name of conf file
 * @param[in] file_path_prefix Path prefix of any relative paths described in
 *                             the conf file
 * @param[in] schema Object model
 * @param[in] warm_init_mode flag to indicate cold/warm init mode
 * @param[in] warm_init_file file to recover SDK state from
 * @param[in] use_kpkt flag to indicate whether kpkt LKM is loaded
 * enabled. Default=false
 *
 * @retval SWITCH_STATUS_SUCCESS Initialization success
 * @retval SWITCH_STATUS_FAILURE failed to initialize SDK
 */
switch_status_t bf_switch_init(const uint16_t device,
                               const char *const conf_file,
                               const char *const file_path_prefix,
                               bool warm_init_mode,
                               const char *const warm_init_file,
                               bool use_kpkt = false);

/**
 * @brief Device remove routine
 * \n - Delete a device object
 * \n - All internally created default objects are deleted
 *
 * @param[in] device Device id
 *
 * @retval SWITCH_STATUS_SUCCESS device creation success
 * @retval SWITCH_STATUS_FAILURE failed to create device
 */
switch_status_t bf_switch_device_remove(uint16_t device);

/**
 * @brief Exit point for bf_switch
 * \n - Remove the object model and cleans all modules
 * \n - Internally calls \ref bf_switch_device_remove
 *
 * @param[in] device Device ID
 * @param[in] warm_shut_mode flag to indicate cold/warm shutdown
 * @param[in] warm_shut_file file to write SDK state to
 *
 * @retval SWITCH_STATUS_SUCCESS Cleanup success
 * @retval SWITCH_STATUS_FAILURE failed to clean SDK
 */
switch_status_t bf_switch_clean(const uint16_t device,
                                bool warm_shut_mode,
                                const char *const warm_shut_file);

/**
 * @brief Start the packet processing thread
 * \nThis API has to be invoked after \ref bf_switch_init
 *
 * @retval SWITCH_STATUS_SUCCESS
 */
switch_status_t bf_switch_init_packet_driver();

#ifdef __cplusplus
}
#endif

/*******************************************************************************
 * C++ frontend APIs
 ******************************************************************************/

#ifdef __cplusplus
#include <set>
#include <vector>

namespace bf_switch {

/**
 * @brief Create new object by Id based on object_type and attributes
 *
 * @param[in] object_type object type
 * @param[in] attrs set of attributes
 * @param[in] id resource by id
 *
 * @retval SWITCH_STATUS_SUCCESS object created by id successfully
 * @retval SWITCH_STATUS_INVALID_PARAMETER wrong list of attributes are passed
 * @retval SWITCH_STATUS_FAILURE failed to create object
 */
switch_status_t bf_switch_object_create_by_id(
    const switch_object_type_t object_type,
    const std::set<smi::attr_w> &attrs,
    const uint64_t id);

/**
 * @brief Check if object by Id exists based on object_type and attributes
 *
 * @param[in] object_type object type
 * @param[in] id resource by id
 *
 * @retval true object by id exists
 * @retval false object by id not exists
 */
bool bf_switch_object_exists_by_id(const switch_object_type_t object_type,
                                   const uint64_t id);

/**
 * @brief delete object by Id based on object_type and attributes
 *
 * @param[in] object_type object type
 * @param[in] id resource by id
 *
 * @retval SWITCH_STATUS_SUCCESS object deleted by id successfully
 * @retval SWITCH_STATUS_INVALID_PARAMETER wrong list of attributes are passed
 * @retval SWITCH_STATUS_FAILURE failed to delete object
 */
switch_status_t bf_switch_object_delete_by_id(
    const switch_object_type_t object_type, const uint64_t id);

/**
 * @brief Get object handle using key group lookup
 *
 * @param[in] object_type object type
 * @param[in] attrs set of attributes used for lookup
 * @param[out] object_handle object handle obtained from lookup
 *
 * @retval SWITCH_STATUS_SUCCESS object found
 * @retval SWITCH_STATUS_ITEM_NOT_FOUND object not found
 * @retval SWITCH_STATUS_INVALID_KEY_GROUP invalid key group for lookup
 */
switch_status_t bf_switch_object_get(const switch_object_type_t object_type,
                                     const std::set<smi::attr_w> &attrs,
                                     switch_object_id_t &object_id);

/**
 * @brief Create new object based on object_type and attributes
 *
 * @param[in] object_type object type
 * @param[in] attrs set of attributes
 * @param[out] object_handle object handle after creation
 *
 * @retval SWITCH_STATUS_SUCCESS object created successfully
 * @retval SWITCH_STATUS_ITEM_ALREADY_EXISTS object exists and object_handle is
 *updated
 * @retval SWITCH_STATUS_INVALID_PARAMETER wrong list of attributes are passed
 * @retval SWITCH_STATUS_FAILURE failed to create object
 */
switch_status_t bf_switch_object_create(const switch_object_type_t object_type,
                                        const std::set<smi::attr_w> &attrs,
                                        switch_object_id_t &object_id);

/**
 * @brief Delete object based on object_handle
 *
 * @param[in] object_handle object handle
 *
 * @retval SWITCH_STATUS_SUCCESS object deleted successfully
 * @retval SWITCH_STATUS_RESOURCE_IN_USE object is an attribute for another
 *object
 * @retval SWITCH_STATUS_FAILURE failed to delete object
 */
switch_status_t bf_switch_object_delete(const switch_object_id_t object_id);

/**
 * @brief Get object type by object_handle
 *
 * @param[in] object_handle object handle
 * @param[out] object_type object type value filled in
 *
 * @retval SWITCH_STATUS_SUCCESS object type retrieved successfully
 * @retval SWITCH_STATUS_FAILURE failed to retrieve object type
 */
switch_status_t bf_switch_object_type_get(const switch_object_id_t object_id,
                                          switch_object_type_t &object_type);

/**
 * @brief Get object name by object_handle
 *
 * @param[in] object_id object handle
 *
 * @retval string type object name
 */
std::string bf_switch_object_name_get(const switch_object_id_t object_id);

/**
 * @brief Set one attribute of an object
 *
 * @param[in] object_handle object handle
 * @param[in] attr single attribute to set
 *
 * @retval SWITCH_STATUS_SUCCESS attribute updated successfully
 * @retval SWITCH_STATUS_INVALID_PARAMETER invalid attribute type or value
 * @retval SWITCH_STATUS_FAILURE failed to set attribute
 */
switch_status_t bf_switch_attribute_set(const switch_object_id_t object_id,
                                        const smi::attr_w &attr);

/**
 * @brief Get one attribute of an object
 *
 * @param[in] object_handle object handle
 * @param[in] attr_id attribute id to get
 * @param[out] attr atrribute with type and value filled in
 *
 * @retval SWITCH_STATUS_SUCCESS attribute get succeded
 * @retval SWITCH_STATUS_FAILURE failed to get attribute
 */
switch_status_t bf_switch_attribute_get(const switch_object_id_t object_id,
                                        const switch_attr_id_t attr_id,
                                        smi::attr_w &attr);

/**
 * @brief Get list of counters supported by the object
 *
 * @param[in] object_handle object handle
 * @param[out] counters Set of counters returned
 *
 * @retval SWITCH_STATUS_SUCCESS counters retrieved successfully
 * @retval SWITCH_STATUS_FAILURE failed to get counters
 */
switch_status_t bf_switch_counters_get(const switch_object_id_t object_handle,
                                       std::vector<switch_counter_t> &cntrs);

/**
 * @brief Clear counters for the object. In most cases, this sets value
 * to 0, if clearing is supported
 *
 * @param[in] object_handle object handle
 * @param[in] cntrs_ids list of counter ids to be cleared
 *
 * @retval SWITCH_STATUS_SUCCESS counters retrieved successfully
 * @retval SWITCH_STATUS_FAILURE failed to get counters
 */
switch_status_t bf_switch_counters_clear(
    const switch_object_id_t object_handle,
    const std::vector<uint16_t> &cntrs_ids);

/**
 * @brief Clear all counters for the object. In most cases, this sets value
 * to 0, if clearing is supported
 *
 * @param[in] object_handle object handle
 *
 * @retval SWITCH_STATUS_SUCCESS counters retrieved successfully
 * @retval SWITCH_STATUS_FAILURE failed to get counters
 */
switch_status_t bf_switch_counters_clear_all(
    const switch_object_id_t object_handle);

/**
 * @brief Delete all object of given type
 *
 * @param[in] object_type Switch object type
 *
 * @retval SWITCH_STATUS_SUCCESS if all objects deleted
 * @retval SWITCH_STATUS_FAILURE failed to delete objects
 */
switch_status_t bf_switch_object_flush_all(
    const switch_object_type_t object_type);

/**
 * @brief Iterator helper function. This call gives the first object of a given
 * type. If valid handle then application can proceed to call \ref
 * bf_switch_get_next_handles
 *
 * @param[in] object_type Object type to iterate on
 * @param[out] object_handle First object handle of object_type
 *
 * @return
 */
switch_status_t bf_switch_get_first_handle(switch_object_type_t object_type,
                                           switch_object_id_t &object_handle);

/**
 * @brief Get next set of handles after getting the first handle via \ref
 *bf_switch_get_first_handle
 *
 * @param object_handle first handle
 * @param in_num_handles number of handles of retrieve
 * @param next_handles List of handles returned
 * @param out_num_handles Actual count of number of handles returned
 *
 * @retval SWITCH_STATUS_SUCCESS
 */
switch_status_t bf_switch_get_next_handles(
    switch_object_id_t object_handle,
    uint32_t in_num_handles,
    std::vector<switch_object_id_t> &next_handles,
    uint32_t &out_num_handles);

/**
 * @brief Get all object handles of a given object type. If no object handles
 * are found for a given object type, this call returns success with an emtpy
 * list
 *
 * @param[in] object_type Object type to iterate on
 * @param[out] object_handles List of handles returned
 *
 * @retval SWITCH_STATUS_SUCCESS
 */
switch_status_t bf_switch_get_all_handles(
    switch_object_type_t object_type,
    std::vector<switch_object_id_t> &object_handles);

/**
 * @brief Set log level per object. Log levels are defined in switch_verbosity_t
 * \nPassing object_type = 0, would set verbosity for all object types
 * \nThe verbosity of this API should be <= the verbosity set using \ref
 *bf_switch_global_log_level_set
 *
 * @param[in] object_type object type
 * @param[in] verbosity verbosity
 *
 * @retval SWITCH_STATUS_SUCCESS
 */
switch_status_t bf_switch_object_log_level_set(
    const switch_object_type_t object_type, const switch_verbosity_t verbosity);

/**
 * @brief Set global logging level. This API only configures the logs to be
 *writable to STDOUT/FILE but does not affect the log level of objects. Each
 *object can then be configured individually using \ref
 *bf_witch_object_log_level_set
 *
 * @param[in] verbosity verbosity
 *
 * @retval SWITCH_STATUS_SUCCESS
 */
switch_status_t bf_switch_global_log_level_set(
    const switch_verbosity_t verbosity);

/**
 * @brief Get table size and usage information
 *
 * @param[in] table_id This is enum value from "device" object's "table"
 *attribute from the schema
 * @param[out table_info switch_table_info_t object
 *
 * @retval SWITCH_STATUS_SUCCESS if valid table ID
 * @retval SWITCH_STATUS_INVALID_PARAMETER if invalid table ID is passed in
 */
switch_status_t bf_switch_table_info_get(uint64_t table_id,
                                         switch_table_info_t &table_info);

/**
 * @brief This checks if feature is set in the bitmap
 *
 * param[in] feature - switch_feature_id_t
 *
 * @retval true, false
 */
bool bf_switch_is_feature_enabled(switch_feature_id_t feature);

/**
 * @brief Set the APIs to function in SAI mode. This means some optimizations
 *kick in when the APIs are invoked from the BFN SAI library. Nothing needs to
 *be done for non SAI applications.
 * \nThis API has to be invoked before \ref bf_switch_init
 *
 * @param[in] sai_mode boolean value indicating SAI mode
 *
 * @retval SWITCH_STATUS_SUCCESS
 */
switch_status_t bf_switch_sai_mode_set(bool sai_mode);

/**
 * @brief Set the APIs to function in SAL mode. This means some optimizations
 *kick in when the APIs are invoked from the BFN SAL library. Nothing needs to
 *be done for non SAL applications.
 * \nThis API has to be invoked before \ref bf_switch_init
 *
 * @param[in] sal_mode boolean value indicating SAL mode
 *
 * @retval SWITCH_STATUS_SUCCESS
 */
switch_status_t bf_switch_sal_mode_set(bool sal_mode);

/**
 * @brief Start batch session
 * \n - Internally calls \ref start_batch
 *
 * @retval SWITCH_STATUS_SUCCESS Success of start batch
 * @retval SWITCH_STATUS_FAILURE Fail of start batch
 */
switch_status_t bf_switch_start_batch();

/**
 * @brief End batch session
 * \n - Internally calls \ref end_batch
 *
 * @retval SWITCH_STATUS_SUCCESS Success of end batch
 * @retval SWITCH_STATUS_FAILURE Fail of end batch
 */
switch_status_t bf_switch_end_batch();

/**
 * @brief Get SDE and SAI versions
 *
 * @param[out] sde_version string SDE version in x.x.x format
 * @param[out] sai_version string SAI version in x.x.x format from saiversion.h
 *
 * @retval SWITCH_STATUS_SUCCESS Cleanup success
 */
switch_status_t bf_switch_version_get(std::string &sde_version,
                                      std::string &sai_version);

/**
 * @brief Comment recording BMAI operations. These records will be skipped
 * during playback.
 *
 * @param[in] boolean comment on/off
 */
void bf_switch_record_comment_mode_set(bool on);

class switchOptions {
 public:
  switchOptions()
      : tf_version(0),
        model_json(),
        eth_cpu_port(),
        use_eth_cpu_port(false),
        sai_skip_init(false),
        override_log_level(true),
        non_default_port_ppgs(0) {}
  int load_from_conf(int dev_id,
                     const char *conf_file,
                     const char *file_path_prefix);

  bool is_tf1() { return tf_version == 1; }
  bool is_tf2() { return tf_version == 2; }
  std::string model_json_get() { return model_json; }
  std::string eth_cpu_port_get() { return eth_cpu_port; }
  bool use_eth_cpu_port_get() { return use_eth_cpu_port; }
  bool sai_skip_init_get() { return sai_skip_init; }
  bool override_log_level_get() { return override_log_level; }
  uint32_t non_default_ppgs_get() { return non_default_port_ppgs; }

 private:
  int tf_version;
  std::string model_json;
  std::string eth_cpu_port;
  bool use_eth_cpu_port;
  bool sai_skip_init;
  bool override_log_level;
  uint32_t non_default_port_ppgs;
};

}  // namespace bf_switch
#endif  // __cplusplus

/** @} */

#endif  // INCLUDE_BF_SWITCH_BF_SWITCH_H__
