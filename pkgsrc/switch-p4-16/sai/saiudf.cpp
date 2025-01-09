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


#include <saiinternal.h>

#include <map>
#include <vector>
#include <set>

#include "s3/switch_store.h"

static sai_api_t api_id = SAI_API_UDF;

/**
 * @brief Create UDF
 *
 * @param[out] udf_id UDF id
 * @param[in] switch_id Switch id
 * @param[in] attr_count Number of attributes
 * @param[in] attr_list Array of attributes
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
static sai_status_t sai_create_udf(_Out_ sai_object_id_t *udf_id,
                                   _In_ sai_object_id_t switch_id,
                                   _In_ uint32_t attr_count,
                                   _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();
  std::set<smi::attr_w> sw_attrs;
  switch_object_id_t object_id = {};
  sai_status_t status = status_switch_to_sai(
      bf_switch_object_create(SWITCH_OBJECT_TYPE_UDF, sw_attrs, object_id));
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to create UDF: %s",
                  sai_metadata_get_status_name(status));
  }
  *udf_id = object_id.data;
  SAI_LOG_EXIT();
  return status;
}

/**
 * @brief Remove UDF
 *
 * @param[in] udf_id UDF id
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
static sai_status_t sai_remove_udf(_In_ sai_object_id_t udf_id) {
  SAI_LOG_ENTER();
  const switch_object_id_t sw_object_id = {.data = udf_id};
  sai_status_t status =
      status_switch_to_sai(bf_switch_object_delete(sw_object_id));
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to remove UDF 0x%" PRIx64 ": %s",
                  udf_id,
                  sai_metadata_get_status_name(status));
  }
  SAI_LOG_EXIT();
  return status;
}

/**
 * @brief Set UDF attribute
 *
 * @param[in] udf_id UDF id
 * @param[in] attr Attribute
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
static sai_status_t sai_set_udf_attribute(_In_ sai_object_id_t udf_id,
                                          _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();
  SAI_LOG_EXIT();
  return SAI_STATUS_SUCCESS;
}

/**
 * @brief Get UDF attribute value
 *
 * @param[in] udf_id UDF id
 * @param[in] attr_count Number of attributes
 * @param[inout] attr_list Array of attributes
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
static sai_status_t sai_get_udf_attribute(_In_ sai_object_id_t udf_id,
                                          _In_ uint32_t attr_count,
                                          _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();
  SAI_LOG_EXIT();
  return SAI_STATUS_SUCCESS;
}

/**
 * @brief Create UDF match
 *
 * @param[out] udf_match_id UDF match id
 * @param[in] switch_id Switch id
 * @param[in] attr_count Number of attributes
 * @param[in] attr_list Array of attributes
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
static sai_status_t sai_create_udf_match(
    _Out_ sai_object_id_t *udf_match_id,
    _In_ sai_object_id_t switch_id,
    _In_ uint32_t attr_count,
    _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();
  std::set<smi::attr_w> sw_attrs;
  switch_object_id_t object_id = {};
  sai_status_t status = status_switch_to_sai(bf_switch_object_create(
      SWITCH_OBJECT_TYPE_UDF_MATCH, sw_attrs, object_id));
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to create UDF match: %s",
                  sai_metadata_get_status_name(status));
  }
  *udf_match_id = object_id.data;
  SAI_LOG_EXIT();
  return status;
}

/**
 * @brief Remove UDF match
 *
 * @param[in] udf_match_id UDF match id
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
static sai_status_t sai_remove_udf_match(_In_ sai_object_id_t udf_match_id) {
  SAI_LOG_ENTER();
  const switch_object_id_t sw_object_id = {.data = udf_match_id};
  sai_status_t status =
      status_switch_to_sai(bf_switch_object_delete(sw_object_id));
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to remove UDF match 0x%" PRIx64 ": %s",
                  udf_match_id,
                  sai_metadata_get_status_name(status));
  }
  SAI_LOG_EXIT();
  return status;
}

/**
 * @brief Set UDF match attribute
 *
 * @param[in] udf_match_id UDF match id
 * @param[in] attr Attribute
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
static sai_status_t sai_set_udf_match_attribute(
    _In_ sai_object_id_t udf_match_id, _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();
  SAI_LOG_EXIT();
  return SAI_STATUS_SUCCESS;
}

/**
 * @brief Get UDF match attribute value
 *
 * @param[in] udf_match_id UDF match id
 * @param[in] attr_count Number of attributes
 * @param[inout] attr_list List of attributes
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
static sai_status_t sai_get_udf_match_attribute(
    _In_ sai_object_id_t udf_match_id,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();
  SAI_LOG_EXIT();
  return SAI_STATUS_SUCCESS;
}

/**
 * @brief Create UDF group
 *
 * @param[out] udf_group_id UDF group id
 * @param[in] switch_id Switch id
 * @param[in] attr_count Number of attributes
 * @param[in] attr_list Array of attributes
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
static sai_status_t sai_create_udf_group(
    _Out_ sai_object_id_t *udf_group_id,
    _In_ sai_object_id_t switch_id,
    _In_ uint32_t attr_count,
    _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();
  std::set<smi::attr_w> sw_attrs;
  switch_object_id_t object_id = {};
  sai_status_t status = status_switch_to_sai(bf_switch_object_create(
      SWITCH_OBJECT_TYPE_UDF_GROUP, sw_attrs, object_id));
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to create UDF group: %s",
                  sai_metadata_get_status_name(status));
  }
  *udf_group_id = object_id.data;
  SAI_LOG_EXIT();
  return status;
}

/**
 * @brief Remove UDF group
 *
 * @param[in] udf_group_id UDF group id
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
static sai_status_t sai_remove_udf_group(_In_ sai_object_id_t udf_group_id) {
  SAI_LOG_ENTER();
  const switch_object_id_t sw_object_id = {.data = udf_group_id};
  sai_status_t status =
      status_switch_to_sai(bf_switch_object_delete(sw_object_id));
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to remove UDF group 0x%" PRIx64 ": %s",
                  udf_group_id,
                  sai_metadata_get_status_name(status));
  }
  SAI_LOG_EXIT();
  return status;
}

/**
 * @brief Set UDF group attribute
 *
 * @param[in] udf_group_id UDF group id
 * @param[in] attr Attribute
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
static sai_status_t sai_set_udf_group_attribute(
    _In_ sai_object_id_t udf_group_id, _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();
  SAI_LOG_EXIT();
  return SAI_STATUS_SUCCESS;
}

/**
 * @brief Get UDF group attribute value
 *
 * @param[in] udf_group_id UDF group id
 * @param[in] attr_count Number of attributes
 * @param[inout] attr_list Array of attributes
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
static sai_status_t sai_get_udf_group_attribute(
    _In_ sai_object_id_t udf_group_id,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();
  SAI_LOG_EXIT();
  return SAI_STATUS_SUCCESS;
}

static sai_udf_api_t udf_api = {
    .create_udf = sai_create_udf,
    .remove_udf = sai_remove_udf,
    .set_udf_attribute = sai_set_udf_attribute,
    .get_udf_attribute = sai_get_udf_attribute,
    .create_udf_match = sai_create_udf_match,
    .remove_udf_match = sai_remove_udf_match,
    .set_udf_match_attribute = sai_set_udf_match_attribute,
    .get_udf_match_attribute = sai_get_udf_match_attribute,
    .create_udf_group = sai_create_udf_group,
    .remove_udf_group = sai_remove_udf_group,
    .set_udf_group_attribute = sai_set_udf_group_attribute,
    .get_udf_group_attribute = sai_get_udf_group_attribute,
};

sai_udf_api_t *sai_udf_api_get() { return &udf_api; }

sai_status_t sai_udf_initialize() {
  SAI_LOG_DEBUG("Initializing udf");
  return SAI_STATUS_SUCCESS;
}
