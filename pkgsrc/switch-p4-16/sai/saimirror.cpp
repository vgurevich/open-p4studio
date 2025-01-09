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

#include <unordered_map>
#include <set>

static sai_api_t api_id = SAI_API_MIRROR;

#define SAI_DEFAULT_VLAN_TPID 0x8100

namespace std {
template <>
struct hash<switch_mirror_attr_erspan_type> {
  inline size_t operator()(switch_mirror_attr_erspan_type const &type) const {
    return std::hash<uint64_t>{}(type);
  }
};
}  // namespace std

/**
 * Implemented GRE protocol types for mirror session 'ENHANCED REMOTE'
 */
static const std::unordered_map<sai_uint16_t, uint64_t> erspan_map = {
    {0x88BE, SWITCH_MIRROR_ATTR_ERSPAN_TYPE_ERSPAN_2},
    {0x22EB, SWITCH_MIRROR_ATTR_ERSPAN_TYPE_ERSPAN_3}};

static const std::unordered_map<uint64_t, sai_uint16_t> erspan_rmap = {
    {SWITCH_MIRROR_ATTR_ERSPAN_TYPE_ERSPAN_2, 0x88BE},
    {SWITCH_MIRROR_ATTR_ERSPAN_TYPE_ERSPAN_3, 0x22EB}};

/**
 * @brief Create mirror session.
 *
 * @param[out] session_id Port mirror session id
 * @param[in] switch_id Switch id
 * @param[in] attr_count Number of attributes
 * @param[in] attr_list Value of attributes
 * @return SAI_STATUS_SUCCESS if operation is successful otherwise a different
 *  error code is returned.
 */
sai_status_t sai_create_mirror_session(_Out_ sai_object_id_t *session_id,
                                       _In_ sai_object_id_t switch_id,
                                       _In_ uint32_t attr_count,
                                       _In_ const sai_attribute_t *attr_list) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t missor_object_id = {};
  const switch_object_type_t ot = SWITCH_OBJECT_TYPE_MIRROR;
  sai_mirror_session_type_t mirror_type = SAI_MIRROR_SESSION_TYPE_LOCAL;
  bool vlan_header_valid = false;
  std::set<smi::attr_w> sw_attrs;
  switch_enum_t sw_enum;
  sai_uint16_t vlan_id = 0;
  sai_uint16_t deflt_tpid = SAI_DEFAULT_VLAN_TPID;

  SAI_LOG_ENTER();

  if (!session_id || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *session_id = SAI_NULL_OBJECT_ID;

  for (uint32_t index = 0; index < attr_count; index++) {
    const sai_attribute_t *attribute = &attr_list[index];
    switch (attribute->id) {
      case SAI_MIRROR_SESSION_ATTR_TYPE:
        mirror_type = (sai_mirror_session_type_t)attribute->value.s32;
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_MIRROR_SESSION, attribute, sw_attrs);
        break;
      case SAI_MIRROR_SESSION_ATTR_VLAN_ID:
        vlan_id = attribute->value.u16;
        break;
      case SAI_MIRROR_SESSION_ATTR_VLAN_HEADER_VALID:
        vlan_header_valid = attribute->value.booldata;
        break;
      case SAI_MIRROR_SESSION_ATTR_GRE_PROTOCOL_TYPE:
        if (erspan_map.count(attribute->value.u16) <= 0) {
          status = SAI_STATUS_INVALID_PARAMETER;
          SAI_LOG_ERROR(
              "failed to create mirror session with attribute %s set to "
              "0x%04X: %s",
              sai_attribute_name(SAI_OBJECT_TYPE_MIRROR_SESSION, attribute->id),
              attribute->value.u16,
              sai_metadata_get_status_name(status));
          return status;
        }
        sw_enum.enumdata = erspan_map.at(attribute->value.u16);
        sw_attrs.insert(attr_w(SWITCH_MIRROR_ATTR_ERSPAN_TYPE, sw_enum));
        break;
      case SAI_MIRROR_SESSION_ATTR_VLAN_TPID:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_MIRROR_SESSION, attribute, sw_attrs);
        deflt_tpid = 0;
        break;
      case SAI_MIRROR_SESSION_ATTR_POLICER: {
        switch_object_id_t meter_handle = {.data = attribute->value.oid};
        sw_attrs.insert(
            smi::attr_w(SWITCH_MIRROR_ATTR_METER_HANDLE, meter_handle));
      } break;
      case SAI_MIRROR_SESSION_ATTR_TRUNCATE_SIZE:
      case SAI_MIRROR_SESSION_ATTR_SAMPLE_RATE:
      case SAI_MIRROR_SESSION_ATTR_VLAN_CFI:
      case SAI_MIRROR_SESSION_ATTR_ERSPAN_ENCAPSULATION_TYPE:
      case SAI_MIRROR_SESSION_ATTR_IPHDR_VERSION:
        break;
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_MIRROR_SESSION, attribute, sw_attrs);
    }
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR(
          "failed to create mirror session with attribute %s: %s",
          sai_attribute_name(SAI_OBJECT_TYPE_MIRROR_SESSION, attribute->id),
          sai_metadata_get_status_name(status));
      return status;
    }
  }

  sai_insert_device_attribute(0, SWITCH_MIRROR_ATTR_DEVICE, sw_attrs);
  sw_enum.enumdata = SWITCH_MIRROR_ATTR_SESSION_TYPE_SIMPLE;
  sw_attrs.insert(attr_w(SWITCH_MIRROR_ATTR_SESSION_TYPE, sw_enum));
  sw_enum.enumdata = SWITCH_MIRROR_ATTR_MIRROR_TYPE_PORT;
  sw_attrs.insert(attr_w(SWITCH_MIRROR_ATTR_MIRROR_TYPE, sw_enum));
  sw_enum.enumdata = SWITCH_MIRROR_ATTR_DIRECTION_BOTH;
  sw_attrs.insert(attr_w(SWITCH_MIRROR_ATTR_DIRECTION, sw_enum));
  sw_attrs.insert(attr_w(SWITCH_MIRROR_ATTR_PLATFORM_INFO, false));
  if (mirror_type == SAI_MIRROR_SESSION_TYPE_REMOTE || vlan_header_valid) {
    if (vlan_id == 0) {
      status = SAI_STATUS_MANDATORY_ATTRIBUTE_MISSING;
      SAI_LOG_ERROR("VLAN ID attribute is missing: %s",
                    sai_metadata_get_status_name(status));
      return status;
    }
    sai_object_id_t vlan_oid = SAI_NULL_OBJECT_ID;
    status = sai_get_vlan_oid_by_vlan_id(&vlan_oid, vlan_id);
    if (status == SAI_STATUS_SUCCESS) {
      switch_object_id_t vlan_handle = {.data = vlan_oid};
      sw_attrs.insert(attr_w(SWITCH_MIRROR_ATTR_VLAN_HANDLE, vlan_handle));
      sw_enum.enumdata = SWITCH_MIRROR_ATTR_RSPAN_TYPE_VLAN_HANDLE;
    } else {
      sw_attrs.insert(attr_w(SWITCH_MIRROR_ATTR_VLAN_ID, vlan_id));
      sw_enum.enumdata = SWITCH_MIRROR_ATTR_RSPAN_TYPE_VLAN_ID;
    }
    if (0 != deflt_tpid) {
      sw_attrs.insert(attr_w(SWITCH_MIRROR_ATTR_VLAN_TPID, deflt_tpid));
    }
  } else {
    sw_enum.enumdata = SWITCH_MIRROR_ATTR_RSPAN_TYPE_NONE;
  }
  sw_attrs.insert(attr_w(SWITCH_MIRROR_ATTR_RSPAN_TYPE, sw_enum));

  switch_status = bf_switch_object_create(ot, sw_attrs, missor_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to create mirror session: %s",
                  sai_metadata_get_status_name(status));
  }
  *session_id = missor_object_id.data;

  SAI_LOG_EXIT();

  return status;
}

/**
 * @brief Remove mirror session.
 *
 * @param[in] session_id Port mirror session id
 * @return SAI_STATUS_SUCCESS if operation is successful otherwise a different
 *  error code is returned.
 */
sai_status_t sai_remove_mirror_session(_In_ sai_object_id_t session_id) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  SAI_LOG_ENTER();

  if (sai_object_type_query(session_id) != SAI_OBJECT_TYPE_MIRROR_SESSION) {
    SAI_LOG_ERROR("Invalid object 0x%016" PRIx64 "", session_id);
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  const switch_object_id_t sw_object_id = {.data = session_id};
  switch_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to remove mirror session %" PRIx64 ": %s",
                  session_id,
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return status;
}

/**
 * @brief Set mirror session attributes.
 *
 * @param[in] session_id Port mirror session id
 * @param[in] attr Value of attribute
 * @return SAI_STATUS_SUCCESS if operation is successful otherwise a different
 *  error code is returned.
 */
sai_status_t sai_set_mirror_session_attribute(
    _In_ sai_object_id_t session_id, _In_ const sai_attribute_t *attr) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  SAI_LOG_ENTER();

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute: %s", sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(session_id) != SAI_OBJECT_TYPE_MIRROR_SESSION) {
    SAI_LOG_ERROR("Invalid object 0x%016" PRIx64 "", session_id);
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  const switch_object_id_t sw_object_id = {.data = session_id};
  switch (attr->id) {
    // CREATE_ONLY attributes
    case SAI_MIRROR_SESSION_ATTR_TYPE:
    case SAI_MIRROR_SESSION_ATTR_ERSPAN_ENCAPSULATION_TYPE:
      status = SAI_STATUS_FAILURE;
      break;
    case SAI_MIRROR_SESSION_ATTR_VLAN_HEADER_VALID: {
      sai_attribute_t attribute = {.id = SAI_MIRROR_SESSION_ATTR_TYPE};
      status = sai_to_switch_attribute_get(
          SAI_OBJECT_TYPE_MIRROR_SESSION, sw_object_id, &attribute);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to get mirror type for 0x%016" PRIx64 "",
                      session_id);
        return status;
      }
      if (attribute.value.s32 != SAI_MIRROR_SESSION_TYPE_ENHANCED_REMOTE) {
        SAI_LOG_ERROR(
            "Cannot set attribute: %s for mirror session type: %s",
            sai_attribute_name(SAI_OBJECT_TYPE_MIRROR_SESSION, attr->id),
            sai_metadata_get_mirror_session_type_name(
                (sai_mirror_session_type_t)attribute.value.s32));
        return SAI_STATUS_INVALID_PARAMETER;
      }
      switch_enum_t rspan_type = {};
      if (!attr->value.booldata) {
        rspan_type.enumdata = SWITCH_MIRROR_ATTR_RSPAN_TYPE_NONE;
      } else {
        rspan_type.enumdata = SWITCH_MIRROR_ATTR_RSPAN_TYPE_VLAN_ID;
      }
      switch_status = bf_switch_attribute_set(
          sw_object_id, attr_w(SWITCH_MIRROR_ATTR_RSPAN_TYPE, rspan_type));
      status = status_switch_to_sai(switch_status);
    } break;
    case SAI_MIRROR_SESSION_ATTR_VLAN_ID: {
      sai_object_id_t vlan_oid = SAI_NULL_OBJECT_ID;
      status = sai_get_vlan_oid_by_vlan_id(&vlan_oid, attr->value.u16);
      if (status == SAI_STATUS_SUCCESS) {
        switch_object_id_t vlan_handle = {.data = vlan_oid};
        attr_w mirror_attr(SWITCH_MIRROR_ATTR_VLAN_HANDLE, vlan_handle);
        switch_status = bf_switch_attribute_set(sw_object_id, mirror_attr);
      } else {
        attr_w mirror_attr(SWITCH_MIRROR_ATTR_VLAN_ID, attr->value.u16);
        switch_status = bf_switch_attribute_set(sw_object_id, mirror_attr);
      }
      status = status_switch_to_sai(switch_status);
      break;
    }
    case SAI_MIRROR_SESSION_ATTR_GRE_PROTOCOL_TYPE: {
      if (erspan_map.count(attr->value.u16) > 0) {
        switch_enum_t gre_type = {.enumdata = erspan_map.at(attr->value.u16)};

        attr_w mirror_attr(SWITCH_MIRROR_ATTR_ERSPAN_TYPE, gre_type);
        switch_status = bf_switch_attribute_set(sw_object_id, mirror_attr);
        status = status_switch_to_sai(switch_status);
      } else {
        status = SAI_STATUS_NOT_IMPLEMENTED;
      }
      break;
    }
    case SAI_MIRROR_SESSION_ATTR_TRUNCATE_SIZE:
    case SAI_MIRROR_SESSION_ATTR_SAMPLE_RATE:
    case SAI_MIRROR_SESSION_ATTR_VLAN_CFI:
    case SAI_MIRROR_SESSION_ATTR_IPHDR_VERSION:
      status = SAI_STATUS_NOT_IMPLEMENTED;
      break;
    default:
      status = sai_to_switch_attribute_set(
          SAI_OBJECT_TYPE_MIRROR_SESSION, attr, sw_object_id);
  }
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to set mirror session attribute %s: %s",
                  sai_attribute_name(SAI_OBJECT_TYPE_MIRROR_SESSION, attr->id),
                  sai_metadata_get_status_name(status));
    return status;
  }

  SAI_LOG_EXIT();

  return status;
}

/**
 * @brief Get mirror session attributes.
 *
 * @param[in] session_id Port mirror session id
 * @param[in] attr_count Number of attributes
 * @param[inout] attr_list Value of attribute
 * @return SAI_STATUS_SUCCESS if operation is successful otherwise a different
 *  error code is returned.
 */
sai_status_t sai_get_mirror_session_attribute(
    _In_ sai_object_id_t session_id,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  SAI_LOG_ENTER();

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(session_id) != SAI_OBJECT_TYPE_MIRROR_SESSION) {
    SAI_LOG_ERROR("Invalid object 0x%016" PRIx64 "", session_id);
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  const switch_object_id_t sw_object_id = {.data = session_id};
  for (uint32_t index = 0; index < attr_count; index++) {
    sai_attribute_t *attribute = &attr_list[index];
    switch (attribute->id) {
      case SAI_MIRROR_SESSION_ATTR_VLAN_ID: {
        switch_object_id_t mirror_handle = {.data = session_id};
        switch_object_id_t vlan_handle = {.data = SAI_NULL_OBJECT_ID};
        attr_w mirror_attr(SWITCH_MIRROR_ATTR_VLAN_HANDLE);
        switch_status = bf_switch_attribute_get(
            mirror_handle, SWITCH_MIRROR_ATTR_VLAN_HANDLE, mirror_attr);
        if (switch_status == SWITCH_STATUS_SUCCESS) {
          mirror_attr.v_get(vlan_handle);
        }
        if (vlan_handle.data != SAI_NULL_OBJECT_ID) {
          attr_w vlan_attr(SWITCH_VLAN_ATTR_VLAN_ID);
          switch_status = bf_switch_attribute_get(
              vlan_handle, SWITCH_VLAN_ATTR_VLAN_ID, vlan_attr);
          status = status_switch_to_sai(switch_status);
          if (status == SAI_STATUS_SUCCESS) {
            vlan_attr.v_get(attribute->value.u16);
          } else {
            SAI_LOG_ERROR("Failed to get vlan id by vlan handle error: %s\n",
                          sai_metadata_get_status_name(status));
          }
        } else {
          attr_w mirror_attr1(SWITCH_MIRROR_ATTR_VLAN_ID);
          switch_status = bf_switch_attribute_get(
              mirror_handle, SWITCH_MIRROR_ATTR_VLAN_ID, mirror_attr1);
          status = status_switch_to_sai(switch_status);
          if (status == SAI_STATUS_SUCCESS) {
            mirror_attr1.v_get(attribute->value.u16);
          }
        }
        break;
      }
      case SAI_MIRROR_SESSION_ATTR_TRUNCATE_SIZE:
        // no truncation
        attribute->value.u16 = 0;
        break;
      case SAI_MIRROR_SESSION_ATTR_SAMPLE_RATE:
        // every packet sampling (normal mirror)
        attribute->value.u32 = 1;
        break;
      case SAI_MIRROR_SESSION_ATTR_VLAN_CFI:
        attribute->value.u8 = 0;
        break;
      case SAI_MIRROR_SESSION_ATTR_ERSPAN_ENCAPSULATION_TYPE:
        attribute->value.s32 =
            SAI_ERSPAN_ENCAPSULATION_TYPE_MIRROR_L3_GRE_TUNNEL;
        break;
      case SAI_MIRROR_SESSION_ATTR_VLAN_HEADER_VALID:
      case SAI_MIRROR_SESSION_ATTR_IPHDR_VERSION:
        // ignore for now
        break;
      case SAI_MIRROR_SESSION_ATTR_GRE_PROTOCOL_TYPE: {
        attr_w mirror_attr(SWITCH_MIRROR_ATTR_ERSPAN_TYPE);
        switch_status = bf_switch_attribute_get(
            sw_object_id, SWITCH_MIRROR_ATTR_ERSPAN_TYPE, mirror_attr);
        status = status_switch_to_sai(switch_status);
        if (status == SAI_STATUS_SUCCESS) {
          switch_enum_t sw_enum = {};
          mirror_attr.v_get(sw_enum);
          if (erspan_rmap.count(sw_enum.enumdata) > 0) {
            attribute->value.u16 = erspan_rmap.at(sw_enum.enumdata);
          } else {
            status = SAI_STATUS_NOT_IMPLEMENTED;
          }
        }
        break;
      }
      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_MIRROR_SESSION, sw_object_id, attribute);
    }
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR(
          "Failed to get mirror attribute %s error: %s\n",
          sai_attribute_name(SAI_OBJECT_TYPE_MIRROR_SESSION, attribute->id),
          sai_metadata_get_status_name(status));
      return status;
    }
  }

  SAI_LOG_EXIT();

  return status;
}

/*
 *  Mirror API methods table retrieved with sai_api_query()
 */
sai_mirror_api_t mirror_api = {
    .create_mirror_session = sai_create_mirror_session,
    .remove_mirror_session = sai_remove_mirror_session,
    .set_mirror_session_attribute = sai_set_mirror_session_attribute,
    .get_mirror_session_attribute = sai_get_mirror_session_attribute,
};

sai_mirror_api_t *sai_mirror_api_get() { return &mirror_api; }

sai_status_t sai_mirror_initialize() {
  SAI_LOG_DEBUG("Initializing mirror");
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_MIRROR_SESSION);
  return SAI_STATUS_SUCCESS;
}
