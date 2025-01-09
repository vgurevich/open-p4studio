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

#include <vector>
#include <set>

static sai_api_t api_id = SAI_API_MPLS;

/**
 * @brief Create In Segment entry
 *
 * @param[in] inseg_entry InSegment entry
 * @param[in] attr_count Number of attributes
 * @param[in] attr_list Array of attributes
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_create_inseg_entry(_In_ const sai_inseg_entry_t *inseg_entry,
                                    _In_ uint32_t attr_count,
                                    _In_ const sai_attribute_t *attr_list) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  const switch_object_type_t ot = SWITCH_OBJECT_TYPE_MPLS;
  switch_object_id_t switch_mpls_object_id = {};
  std::set<smi::attr_w> mpls_attrs;
  SAI_LOG_ENTER();

  if (!inseg_entry || (attr_count && !attr_list)) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("NULL MPLS parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  mpls_attrs.insert(smi::attr_w(SWITCH_MPLS_ATTR_LABEL, inseg_entry->label));

  for (unsigned i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      case SAI_INSEG_ENTRY_ATTR_POP_TTL_MODE:
        if (bf_switch_is_feature_enabled(SWITCH_FEATURE_TUNNEL_TTL_MODE)) {
          status = sai_to_switch_attribute(
              SAI_OBJECT_TYPE_INSEG_ENTRY, &attr_list[i], mpls_attrs);
        } else {
          if (attr_list[i].value.s32 == SAI_INSEG_ENTRY_POP_TTL_MODE_UNIFORM) {
            SAI_LOG_ERROR(
                "Uniform mode for %s is not supported, using default pipe mode",
                sai_attribute_name(SAI_OBJECT_TYPE_INSEG_ENTRY,
                                   attr_list[i].id));
            status = SAI_STATUS_NOT_SUPPORTED;
          }
        }
        break;
      case SAI_INSEG_ENTRY_ATTR_POP_QOS_MODE:
        if (bf_switch_is_feature_enabled(SWITCH_FEATURE_TUNNEL_QOS_MODE)) {
          status = sai_to_switch_attribute(
              SAI_OBJECT_TYPE_INSEG_ENTRY, &attr_list[i], mpls_attrs);
        } else {
          if (attr_list[i].value.s32 == SAI_INSEG_ENTRY_POP_QOS_MODE_PIPE) {
            SAI_LOG_ERROR(
                "pipe mode for %s is not supported, using default uniform mode",
                sai_attribute_name(SAI_OBJECT_TYPE_INSEG_ENTRY,
                                   attr_list[i].id));
            status = SAI_STATUS_NOT_SUPPORTED;
          }
        }
        break;
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_INSEG_ENTRY, &attr_list[i], mpls_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to create MPLS inseg entry: unsupported attrs %s",
              sai_metadata_get_status_name(status));
          return status;
        }
        break;
    }
  }

  sai_insert_device_attribute(0, SWITCH_MPLS_ATTR_DEVICE, mpls_attrs);
  switch_status =
      bf_switch_object_create(ot, mpls_attrs, switch_mpls_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to create MPLS object: %s",
                  sai_metadata_get_status_name(status));
  }
  SAI_LOG_EXIT();
  return status;
}

/**
 * @brief Remove In Segment entry
 *
 * @param[in] inseg_entry InSegment entry
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_remove_inseg_entry(_In_ const sai_inseg_entry_t *inseg_entry) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t switch_mpls_object_id = {};
  std::set<smi::attr_w> mpls_attrs;
  SAI_LOG_ENTER();

  if (!inseg_entry) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null MPLS entry: %s", sai_metadata_get_status_name(status));
    return status;
  }

  mpls_attrs.insert(smi::attr_w(SWITCH_MPLS_ATTR_LABEL, inseg_entry->label));
  sai_insert_device_attribute(0, SWITCH_MPLS_ATTR_DEVICE, mpls_attrs);

  switch_status = bf_switch_object_get(
      SWITCH_OBJECT_TYPE_MPLS, mpls_attrs, switch_mpls_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR(
        "Failed to remove MPLS entry: key_error, object_get failed: %s",
        sai_metadata_get_status_name(status));
    return status;
  }

  switch_status = bf_switch_object_delete(switch_mpls_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to remove MPLS entry, object_id: %" PRIx64 ": %s",
                  switch_mpls_object_id.data,
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();
  return status;
}

/**
 * @brief Set In Segment attribute value
 *
 * @param[in] inseg_entry InSegment entry
 * @param[in] attr Attribute
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_set_inseg_entry_attribute(
    _In_ const sai_inseg_entry_t *inseg_entry,
    _In_ const sai_attribute_t *attr) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t switch_mpls_object_id = {};
  std::set<smi::attr_w> mpls_attrs;
  SAI_LOG_ENTER();

  if (!inseg_entry) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null MPLS entry: %s", sai_metadata_get_status_name(status));
    return status;
  }

  mpls_attrs.insert(smi::attr_w(SWITCH_MPLS_ATTR_LABEL, inseg_entry->label));
  sai_insert_device_attribute(0, SWITCH_MPLS_ATTR_DEVICE, mpls_attrs);

  switch_status = bf_switch_object_get(
      SWITCH_OBJECT_TYPE_MPLS, mpls_attrs, switch_mpls_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR(
        "Failed to set MPLS entry attribute: key_error, object_get failed: %s",
        sai_metadata_get_status_name(status));
    return status;
  }

  switch (attr->id) {
    case SAI_INSEG_ENTRY_ATTR_POP_TTL_MODE:
      if (bf_switch_is_feature_enabled(SWITCH_FEATURE_TUNNEL_TTL_MODE)) {
        status = sai_to_switch_attribute_set(
            SAI_OBJECT_TYPE_INSEG_ENTRY, attr, switch_mpls_object_id);
      } else {
        if (attr->value.s32 == SAI_INSEG_ENTRY_POP_TTL_MODE_UNIFORM) {
          SAI_LOG_ERROR(
              "Setting %s to uniform is not supported",
              sai_attribute_name(SAI_OBJECT_TYPE_INSEG_ENTRY, attr->id));
          status = SAI_STATUS_NOT_SUPPORTED;
        }
      }
      break;
    case SAI_INSEG_ENTRY_ATTR_POP_QOS_MODE:
      if (bf_switch_is_feature_enabled(SWITCH_FEATURE_TUNNEL_QOS_MODE)) {
        status = sai_to_switch_attribute_set(
            SAI_OBJECT_TYPE_INSEG_ENTRY, attr, switch_mpls_object_id);
      } else {
        if (attr->value.s32 == SAI_INSEG_ENTRY_POP_QOS_MODE_PIPE) {
          SAI_LOG_ERROR(
              "Setting %s to pipe is not supported",
              sai_attribute_name(SAI_OBJECT_TYPE_INSEG_ENTRY, attr->id));
          status = SAI_STATUS_NOT_SUPPORTED;
        }
      }
      break;
    default:
      status = sai_to_switch_attribute_set(
          SAI_OBJECT_TYPE_INSEG_ENTRY, attr, switch_mpls_object_id);
      if (status != SAI_STATUS_SUCCESS && status != SAI_STATUS_NOT_SUPPORTED) {
        SAI_LOG_ERROR("Failed to set MPLS entry, sw_object_id: %" PRIx64 ": %s",
                      switch_mpls_object_id.data,
                      sai_metadata_get_status_name(status));
      }
      break;
  }

  SAI_LOG_EXIT();
  return status;
}

/**
 * @brief Get In Segment attribute value
 *
 * @param[in] inseg_entry InSegment entry
 * @param[in] attr_count Number of attributes
 * @param[inout] attr_list Array of attributes
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_get_inseg_entry_attribute(
    _In_ const sai_inseg_entry_t *inseg_entry,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t switch_mpls_object_id = {};
  std::set<smi::attr_w> mpls_attrs;

  SAI_LOG_ENTER();

  if (!inseg_entry) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null MPLS entry: %s", sai_metadata_get_status_name(status));
    return status;
  }

  mpls_attrs.insert(smi::attr_w(SWITCH_MPLS_ATTR_LABEL, inseg_entry->label));
  sai_insert_device_attribute(0, SWITCH_MPLS_ATTR_DEVICE, mpls_attrs);

  switch_status = bf_switch_object_get(
      SWITCH_OBJECT_TYPE_MPLS, mpls_attrs, switch_mpls_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR(
        "Failed to remove MPLS entry: key_error, object_get failed: %s",
        sai_metadata_get_status_name(status));
    return status;
  }

  for (uint32_t i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      case SAI_INSEG_ENTRY_ATTR_POP_TTL_MODE:
        if (bf_switch_is_feature_enabled(SWITCH_FEATURE_TUNNEL_TTL_MODE)) {
          status = sai_to_switch_attribute_get(SAI_OBJECT_TYPE_INSEG_ENTRY,
                                               switch_mpls_object_id,
                                               &attr_list[i]);
          if (status != SAI_STATUS_SUCCESS &&
              status != SAI_STATUS_NOT_SUPPORTED) {
            SAI_LOG_ERROR("Failed to get MPLS entry, sw_object_id: %" PRIx64
                          ": %s",
                          switch_mpls_object_id.data,
                          sai_metadata_get_status_name(status));
          }
        } else {
          attr_list[i].value.s32 = SAI_INSEG_ENTRY_POP_TTL_MODE_PIPE;
        }
        break;
      case SAI_INSEG_ENTRY_ATTR_POP_QOS_MODE:
        if (bf_switch_is_feature_enabled(SWITCH_FEATURE_TUNNEL_QOS_MODE)) {
          status = sai_to_switch_attribute_get(SAI_OBJECT_TYPE_INSEG_ENTRY,
                                               switch_mpls_object_id,
                                               &attr_list[i]);
          if (status != SAI_STATUS_SUCCESS &&
              status != SAI_STATUS_NOT_SUPPORTED) {
            SAI_LOG_ERROR("Failed to get MPLS entry, sw_object_id: %" PRIx64
                          ": %s",
                          switch_mpls_object_id.data,
                          sai_metadata_get_status_name(status));
          }
        } else {
          attr_list[i].value.s32 = SAI_INSEG_ENTRY_POP_QOS_MODE_UNIFORM;
        }
        break;
      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_INSEG_ENTRY, switch_mpls_object_id, &attr_list[i]);
        if (status != SAI_STATUS_SUCCESS &&
            status != SAI_STATUS_NOT_SUPPORTED) {
          SAI_LOG_ERROR("Failed to get MPLS entry, sw_object_id: %" PRIx64
                        ": %s",
                        switch_mpls_object_id.data,
                        sai_metadata_get_status_name(status));
        }
        break;
    }
  }

  SAI_LOG_EXIT();
  return status;
}

/**
 * MPLS methods table retrieved with sai_api_query()
 */
sai_mpls_api_t mpls_api = {
  create_inseg_entry : sai_create_inseg_entry,
  remove_inseg_entry : sai_remove_inseg_entry,
  set_inseg_entry_attribute : sai_set_inseg_entry_attribute,
  get_inseg_entry_attribute : sai_get_inseg_entry_attribute,
};

sai_mpls_api_t *sai_mpls_api_get() { return &mpls_api; }

sai_status_t sai_mpls_initialize(bool warm_init) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_INSEG_ENTRY);
  return status;
}
