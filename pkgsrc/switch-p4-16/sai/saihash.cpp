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

static sai_api_t api_id = SAI_API_HASH;
static switch_object_id_t device_handle = {0};

/**
 * @brief Convert SAI attribute to switch attribute
 *
 * @param[in] sai_hash_attr SAI hash attribute value
 *
 * @return Switch hash attribute
 */
uint32_t sai_field_to_switch_hash_field(int32_t sai_hash_attr) {
  uint32_t switch_hash_attr = 0;
  switch (sai_hash_attr) {
    case SAI_NATIVE_HASH_FIELD_SRC_IP:
      switch_hash_attr = SWITCH_HASH_ATTR_FIELD_SRC_ADDR;
      break;
    case SAI_NATIVE_HASH_FIELD_DST_IP:
      switch_hash_attr = SWITCH_HASH_ATTR_FIELD_DST_ADDR;
      break;
    case SAI_NATIVE_HASH_FIELD_IP_PROTOCOL:
      switch_hash_attr = SWITCH_HASH_ATTR_FIELD_IP_PROTO;
      break;
    case SAI_NATIVE_HASH_FIELD_L4_SRC_PORT:
      switch_hash_attr = SWITCH_HASH_ATTR_FIELD_SRC_PORT;
      break;
    case SAI_NATIVE_HASH_FIELD_L4_DST_PORT:
      switch_hash_attr = SWITCH_HASH_ATTR_FIELD_DST_PORT;
      break;
    case SAI_NATIVE_HASH_FIELD_SRC_MAC:
      switch_hash_attr = SWITCH_HASH_ATTR_FIELD_SRC_MAC;
      break;
    case SAI_NATIVE_HASH_FIELD_DST_MAC:
      switch_hash_attr = SWITCH_HASH_ATTR_FIELD_DST_MAC;
      break;
    case SAI_NATIVE_HASH_FIELD_ETHERTYPE:
      switch_hash_attr = SWITCH_HASH_ATTR_FIELD_MAC_TYPE;
      break;
    case SAI_NATIVE_HASH_FIELD_IN_PORT:
      switch_hash_attr = SWITCH_HASH_ATTR_FIELD_INGRESS_PORT;
      break;
    case SAI_NATIVE_HASH_FIELD_IPV6_FLOW_LABEL:
      switch_hash_attr = SWITCH_HASH_ATTR_FIELD_IPV6_FLOW_LABEL;
      break;
    default:
      break;
  }
  return switch_hash_attr;
}

/**
 * @brief Convert Switch HASH Atrribite to SAI HASH attribute
 *
 * @param[in] sw_hash_attr switch hash attribute value
 *
 * @return SAI hash attribute value
 */
int32_t switch_hash_field_to_sai_field(uint32_t sw_hash_attr) {
  int32_t sai_hash_attr = 0;
  switch (sw_hash_attr) {
    case SWITCH_HASH_ATTR_FIELD_SRC_ADDR:
      sai_hash_attr = SAI_NATIVE_HASH_FIELD_SRC_IP;
      break;
    case SWITCH_HASH_ATTR_FIELD_DST_ADDR:
      sai_hash_attr = SAI_NATIVE_HASH_FIELD_DST_IP;
      break;
    case SWITCH_HASH_ATTR_FIELD_IP_PROTO:
      sai_hash_attr = SAI_NATIVE_HASH_FIELD_IP_PROTOCOL;
      break;
    case SWITCH_HASH_ATTR_FIELD_SRC_PORT:
      sai_hash_attr = SAI_NATIVE_HASH_FIELD_L4_SRC_PORT;
      break;
    case SWITCH_HASH_ATTR_FIELD_DST_PORT:
      sai_hash_attr = SAI_NATIVE_HASH_FIELD_L4_DST_PORT;
      break;
    case SWITCH_HASH_ATTR_FIELD_SRC_MAC:
      sai_hash_attr = SAI_NATIVE_HASH_FIELD_SRC_MAC;
      break;
    case SWITCH_HASH_ATTR_FIELD_DST_MAC:
      sai_hash_attr = SAI_NATIVE_HASH_FIELD_DST_MAC;
      break;
    case SWITCH_HASH_ATTR_FIELD_MAC_TYPE:
      sai_hash_attr = SAI_NATIVE_HASH_FIELD_ETHERTYPE;
      break;
    case SWITCH_HASH_ATTR_FIELD_INGRESS_PORT:
      sai_hash_attr = SAI_NATIVE_HASH_FIELD_IN_PORT;
      break;
    case SWITCH_HASH_ATTR_FIELD_IPV6_FLOW_LABEL:
      sai_hash_attr = SAI_NATIVE_HASH_FIELD_IPV6_FLOW_LABEL;
      break;
    default:
      break;
  }
  return sai_hash_attr;
}

/**
 * @brief Convert SAI fine grained hash attribute to switch fine grained hash
 *attribute
 *
 * @param[in] sai_hash_attr SAI hash attribute value
 *
 * @return Switch hash attribute
 */
uint32_t sai_field_to_switch_fg_hash_field(int32_t sai_hash_attr) {
  uint32_t switch_hash_attr = 0;
  switch (sai_hash_attr) {
    case SAI_NATIVE_HASH_FIELD_SRC_IP:
      switch_hash_attr = SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_SRC_ADDR;
      break;
    case SAI_NATIVE_HASH_FIELD_DST_IP:
      switch_hash_attr = SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_DST_ADDR;
      break;
    case SAI_NATIVE_HASH_FIELD_IP_PROTOCOL:
      switch_hash_attr = SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_IP_PROTO;
      break;
    case SAI_NATIVE_HASH_FIELD_L4_SRC_PORT:
      switch_hash_attr = SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_SRC_PORT;
      break;
    case SAI_NATIVE_HASH_FIELD_L4_DST_PORT:
      switch_hash_attr = SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_DST_PORT;
      break;
    case SAI_NATIVE_HASH_FIELD_IN_PORT:
      switch_hash_attr = SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_INGRESS_PORT;
      break;
    case SAI_NATIVE_HASH_FIELD_SRC_MAC:
      switch_hash_attr = SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_SRC_MAC;
      break;
    case SAI_NATIVE_HASH_FIELD_DST_MAC:
      switch_hash_attr = SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_DST_MAC;
      break;
    case SAI_NATIVE_HASH_FIELD_ETHERTYPE:
      switch_hash_attr = SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_MAC_TYPE;
      break;
    case SAI_NATIVE_HASH_FIELD_IPV6_FLOW_LABEL:
      switch_hash_attr =
          SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_IPV6_FLOW_LABEL;
      break;
    default:
      break;
  }
  return switch_hash_attr;
}

/**
 * @brief Convert Switch FG HASH Atrribite to SAI FG HASH attribute
 *
 * @param[in] sw_hash_attr switch hash attribute value
 *
 * @return SAI hash attribute value
 */
uint32_t switch_fg_hash_field_to_sai_field(uint32_t sw_hash_attr) {
  int32_t sai_hash_attr = 0;
  switch (sw_hash_attr) {
    case SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_SRC_ADDR:
      sai_hash_attr = SAI_NATIVE_HASH_FIELD_SRC_IP;
      break;
    case SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_DST_ADDR:
      sai_hash_attr = SAI_NATIVE_HASH_FIELD_DST_IP;
      break;
    case SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_IP_PROTO:
      sai_hash_attr = SAI_NATIVE_HASH_FIELD_IP_PROTOCOL;
      break;
    case SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_SRC_PORT:
      sai_hash_attr = SAI_NATIVE_HASH_FIELD_L4_SRC_PORT;
      break;
    case SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_DST_PORT:
      sai_hash_attr = SAI_NATIVE_HASH_FIELD_L4_DST_PORT;
      break;
    case SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_SRC_MAC:
      sai_hash_attr = SAI_NATIVE_HASH_FIELD_SRC_MAC;
      break;
    case SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_DST_MAC:
      sai_hash_attr = SAI_NATIVE_HASH_FIELD_DST_MAC;
      break;
    case SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_MAC_TYPE:
      sai_hash_attr = SAI_NATIVE_HASH_FIELD_ETHERTYPE;
      break;
    case SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_INGRESS_PORT:
      sai_hash_attr = SAI_NATIVE_HASH_FIELD_IN_PORT;
      break;
    case SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME_IPV6_FLOW_LABEL:
      sai_hash_attr = SAI_NATIVE_HASH_FIELD_IPV6_FLOW_LABEL;
      break;
    default:
      break;
  }
  return sai_hash_attr;
}

/**
 * @brief Create hash
 *
 * @param[out] hash_id Hash id
 * @param[in] switch_id Switch object id
 * @param[in] attr_count Number of attributes
 * @param[in] attr_list Array of attributes
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_create_hash(_Out_ sai_object_id_t *hash_id,
                             _In_ sai_object_id_t switch_id,
                             _In_ uint32_t attr_count,
                             _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();
  sai_status_t status = SAI_STATUS_SUCCESS;
  const sai_attribute_t *attribute = NULL;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  const switch_object_type_t ot = SWITCH_OBJECT_TYPE_HASH;

  if (!hash_id || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter: %s", sai_metadata_get_status_name(status));
    return status;
  }

  uint32_t hash_field = 0;
  *hash_id = SAI_NULL_OBJECT_ID;
  std::set<attr_w> sw_attrs;
  std::vector<uint32_t> hash_field_list;
  std::vector<switch_object_id_t> fg_hash_field_id_list;
  switch_object_id_t switch_hash_object_id = {};

  for (uint32_t index = 0; index < attr_count; index++) {
    attribute = &attr_list[index];
    switch (attribute->id) {
      case SAI_HASH_ATTR_NATIVE_HASH_FIELD_LIST: {
        for (uint32_t idx = 0; idx < attribute->value.s32list.count; idx++) {
          int32_t f = attribute->value.s32list.list[idx];
          hash_field = sai_field_to_switch_hash_field(f);
          if (!hash_field) {
            SAI_LOG_ERROR("Un supported hash attribute: %d",
                          attribute->value.s32list.list[idx]);
            return SAI_STATUS_NOT_SUPPORTED;
          }
          hash_field_list.push_back(hash_field);
        }
        sai_insert_device_attribute(0, SWITCH_HASH_ATTR_DEVICE, sw_attrs);
        sw_attrs.insert(attr_w(SWITCH_HASH_ATTR_FIELD_LIST, hash_field_list));
        break;
      }
      case SAI_HASH_ATTR_FINE_GRAINED_HASH_FIELD_LIST: {
        for (uint32_t idx = 0; idx < attribute->value.objlist.count; idx++) {
          switch_object_id_t obj_id = {.data =
                                           attribute->value.objlist.list[idx]};
          fg_hash_field_id_list.push_back(obj_id);
        }
        sai_insert_device_attribute(0, SWITCH_HASH_ATTR_DEVICE, sw_attrs);
        sw_attrs.insert(attr_w(SWITCH_HASH_ATTR_FINE_GRAINED_FIELD_LIST,
                               fg_hash_field_id_list));
        break;
      }
      default:
        break;
    }
  }

  switch_status = bf_switch_object_create(ot, sw_attrs, switch_hash_object_id);
  status = status_switch_to_sai(switch_status);

  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to create hash object: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *hash_id = switch_hash_object_id.data;
  SAI_LOG_EXIT();
  return SAI_STATUS_SUCCESS;
}

/**
 * @brief Create fine grained hash field
 *
 * @param[out] fine_grained_hash_field_id fine grain hash filed id
 * @param[in] switch_id Switch object id
 * @param[in] attr_count Number of attributes
 * @param[in] attr_list Array of attributes
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_create_fine_grained_hash_field(
    _Out_ sai_object_id_t *fine_grained_hash_field_id,
    _In_ sai_object_id_t switch_id,
    _In_ uint32_t attr_count,
    _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();
  sai_status_t status = SAI_STATUS_SUCCESS;
  const sai_attribute_t *attribute = NULL;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  const switch_object_type_t ot = SWITCH_OBJECT_TYPE_FINE_GRAINED_HASH;

  if (!fine_grained_hash_field_id || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter: %s", sai_metadata_get_status_name(status));
    return status;
  }

  std::set<attr_w> sw_attrs;
  switch_ip_address_t switch_ip_addr;
  switch_object_id_t switch_hash_object_id = {};

  for (uint32_t index = 0; index < attr_count; index++) {
    attribute = &attr_list[index];
    switch (attribute->id) {
      case SAI_FINE_GRAINED_HASH_FIELD_ATTR_NATIVE_HASH_FIELD: {
        auto fg_hash_field =
            sai_field_to_switch_fg_hash_field(attribute->value.s32);
        if (!fg_hash_field) {
          SAI_LOG_ERROR("Unsupported fine grained native hash field: %d",
                        attribute->value.s32);
          return SAI_STATUS_NOT_SUPPORTED;
        }
        switch_enum_t hash_field_e = {fg_hash_field};
        sw_attrs.insert(
            attr_w(SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME, hash_field_e));
        break;
      }
      case SAI_FINE_GRAINED_HASH_FIELD_ATTR_IPV4_MASK: {
        sai_ipv4_to_switch_ip_addr(attribute->value.ip4, switch_ip_addr);
        sw_attrs.insert(
            attr_w(SWITCH_FINE_GRAINED_HASH_ATTR_SRC_IP_MASK, switch_ip_addr));
        sw_attrs.insert(
            attr_w(SWITCH_FINE_GRAINED_HASH_ATTR_DST_IP_MASK, switch_ip_addr));
        break;
      }
      case SAI_FINE_GRAINED_HASH_FIELD_ATTR_IPV6_MASK: {
        sai_ipv6_to_switch_ip_addr(attribute->value.ip6, switch_ip_addr);
        sw_attrs.insert(
            attr_w(SWITCH_FINE_GRAINED_HASH_ATTR_SRC_IP_MASK, switch_ip_addr));
        sw_attrs.insert(
            attr_w(SWITCH_FINE_GRAINED_HASH_ATTR_DST_IP_MASK, switch_ip_addr));
        break;
      }
      case SAI_FINE_GRAINED_HASH_FIELD_ATTR_SEQUENCE_ID: {
        auto order = attribute->value.u32;
        sw_attrs.insert(attr_w(SWITCH_FINE_GRAINED_HASH_ATTR_SEQUENCE, order));
        break;
      }
      default:
        SAI_LOG_ERROR(
            "Unsupported fine-grained hash attribute: %s",
            (sai_attribute_name(SAI_OBJECT_TYPE_FINE_GRAINED_HASH_FIELD,
                                attribute->id)));
        break;
    }
  }

  switch_status = bf_switch_object_create(ot, sw_attrs, switch_hash_object_id);
  status = status_switch_to_sai(switch_status);

  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to create fg hash field object: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *fine_grained_hash_field_id = switch_hash_object_id.data;
  SAI_LOG_EXIT();
  return SAI_STATUS_SUCCESS;
}

/**
 * @brief Remove fine grained hash
 *
 * @param[in] fg_hash_id fine grained hash field id
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_remove_fine_grained_hash_field(
    _In_ sai_object_id_t fg_hash_id) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  SAI_ASSERT(sai_object_type_query(fg_hash_id) ==
             SWITCH_OBJECT_TYPE_FINE_GRAINED_HASH);
  const switch_object_id_t sw_object_id = {.data = fg_hash_id};
  switch_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to remove hash object : %ld", sw_object_id.data);
  }
  SAI_LOG_EXIT();
  return status;
}

/**
 * @brief Set hash attribute
 *
 * @param[in] hash_id Hash id
 * @param[in] attr Attribute
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_set_fine_grained_hash_field_attribute(
    _In_ sai_object_id_t fg_hash_id, _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();
  SAI_LOG_ERROR("Set not supported for fine-grained hash");
  return SAI_STATUS_NOT_SUPPORTED;
}

/**
 * @brief Get fine graind hash attribute value
 *
 * @param[in] fg_hash_id find grained field id
 * @param[in] attr_count Number of attributes
 * @param[inout] attr_list Array of attributes
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_get_fine_grained_hash_field_attribute(
    _In_ sai_object_id_t fg_hash_id,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  sai_attribute_t *attribute = NULL;

  if (!fg_hash_id || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter: %s", sai_metadata_get_status_name(status));
    return status;
  }

  SAI_ASSERT(sai_object_type_query(fg_hash_id) ==
             SWITCH_OBJECT_TYPE_FINE_GRAINED_HASH);
  switch_ip_address_t switch_ip_addr;

  const switch_object_id_t sw_object_id = {.data = fg_hash_id};

  for (uint32_t index = 0; index < attr_count; index++) {
    attribute = &attr_list[index];
    switch (attribute->id) {
      case SAI_FINE_GRAINED_HASH_FIELD_ATTR_NATIVE_HASH_FIELD: {
        attr_w hash_attr(SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME);
        switch_status = bf_switch_attribute_get(
            sw_object_id, SWITCH_FINE_GRAINED_HASH_ATTR_FIELD_NAME, hash_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to retrieve hash attribute: %s",
              (sai_attribute_name(SAI_OBJECT_TYPE_FINE_GRAINED_HASH_FIELD,
                                  attribute->id)));
          return status;
        }
        switch_enum_t hash_field_e = {0};
        hash_attr.v_get(hash_field_e);
        int32_t sai_hash_attr =
            switch_fg_hash_field_to_sai_field(hash_field_e.enumdata);
        attribute->value.s32 = sai_hash_attr;
        break;
      }
      case SAI_FINE_GRAINED_HASH_FIELD_ATTR_IPV4_MASK: {
        attr_w hash_attr(SWITCH_FINE_GRAINED_HASH_ATTR_SRC_IP_MASK);
        switch_status = bf_switch_attribute_get(
            sw_object_id, SWITCH_FINE_GRAINED_HASH_ATTR_SRC_IP_MASK, hash_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to retrieve hash attribute: %s",
              (sai_attribute_name(SAI_OBJECT_TYPE_FINE_GRAINED_HASH_FIELD,
                                  attribute->id)));
          return status;
        }
        hash_attr.v_get(switch_ip_addr);
        switch_ip_addr_to_sai_ipv4(attribute->value.ip4, switch_ip_addr);
        break;
      }
      case SAI_FINE_GRAINED_HASH_FIELD_ATTR_IPV6_MASK: {
        attr_w hash_attr(SWITCH_FINE_GRAINED_HASH_ATTR_SRC_IP_MASK);
        switch_status = bf_switch_attribute_get(
            sw_object_id, SWITCH_FINE_GRAINED_HASH_ATTR_SRC_IP_MASK, hash_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to retrieve hash attribute: %s",
              (sai_attribute_name(SAI_OBJECT_TYPE_FINE_GRAINED_HASH_FIELD,
                                  attribute->id)));
          return status;
        }
        hash_attr.v_get(switch_ip_addr);
        switch_ip_addr_to_sai_ipv6(attribute->value.ip6, switch_ip_addr);
        break;
      }
      case SAI_FINE_GRAINED_HASH_FIELD_ATTR_SEQUENCE_ID: {
        attr_w hash_attr(SWITCH_FINE_GRAINED_HASH_ATTR_SEQUENCE);
        switch_status = bf_switch_attribute_get(
            sw_object_id, SWITCH_FINE_GRAINED_HASH_ATTR_SEQUENCE, hash_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to retrieve hash attribute: %s",
              (sai_attribute_name(SAI_OBJECT_TYPE_FINE_GRAINED_HASH_FIELD,
                                  attribute->id)));
          return status;
        }
        uint32_t order = 0;
        hash_attr.v_get(order);
        attribute->value.u32 = order;
        break;
      }
      default:
        SAI_LOG_ERROR(
            "Unsupported fine-grained hash attribute: %s",
            (sai_attribute_name(SAI_OBJECT_TYPE_FINE_GRAINED_HASH_FIELD,
                                attribute->id)));
        break;
    }
  }
  SAI_LOG_EXIT();
  return status;
}

/**
 * @brief Remove hash
 *
 * @param[in] hash_id Hash id
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_remove_hash(_In_ sai_object_id_t hash_id) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  SAI_ASSERT(sai_object_type_query(hash_id) == SAI_OBJECT_TYPE_HASH);
  const switch_object_id_t sw_object_id = {.data = hash_id};
  switch_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to remove hash object");
  }
  SAI_LOG_EXIT();
  return status;
}

/**
 * @brief Set hash attribute
 *
 * @param[in] hash_id Hash id
 * @param[in] attr Attribute
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_set_hash_attribute(_In_ sai_object_id_t hash_id,
                                    _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  if (!hash_id || !attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter: %s", sai_metadata_get_status_name(status));
    return status;
  }

  SAI_ASSERT(sai_object_type_query(hash_id) == SAI_OBJECT_TYPE_HASH);
  std::vector<uint32_t> hash_field_list;
  std::vector<switch_object_id_t> fg_hash_field_id_list;

  const switch_object_id_t sw_object_id = {.data = hash_id};
  switch (attr->id) {
    case SAI_HASH_ATTR_NATIVE_HASH_FIELD_LIST: {
      uint32_t hash_field = 0;
      for (uint32_t idx = 0; idx < attr->value.s32list.count; idx++) {
        int32_t f = attr->value.s32list.list[idx];
        if (f == SAI_NATIVE_HASH_FIELD_IPV6_FLOW_LABEL) {
          continue;
        }
        hash_field = sai_field_to_switch_hash_field(f);
        if (!hash_field) {
          SAI_LOG_ERROR("Un supported hash attribute: %d",
                        attr->value.s32list.list[idx]);
          return SAI_STATUS_NOT_SUPPORTED;
        }
        hash_field_list.push_back(hash_field);
      }
      smi::attr_w sw_attr(SWITCH_HASH_ATTR_FIELD_LIST, hash_field_list);
      switch_status = bf_switch_attribute_set(sw_object_id, sw_attr);
    } break;

    case SAI_HASH_ATTR_FINE_GRAINED_HASH_FIELD_LIST: {
      switch_object_id_t obj_id;
      for (uint32_t idx = 0; idx < attr->value.objlist.count; idx++) {
        obj_id = {.data = attr->value.objlist.list[idx]};
        fg_hash_field_id_list.push_back(obj_id);
      }
      smi::attr_w sw_attr(SWITCH_HASH_ATTR_FINE_GRAINED_FIELD_LIST,
                          fg_hash_field_id_list);
      switch_status = bf_switch_attribute_set(sw_object_id, sw_attr);
    } break;

    default:
      SAI_LOG_ERROR("Un supported hash attribute: %d", attr->id);
      return SAI_STATUS_INVALID_PARAMETER;
  }

  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to update hash object: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  SAI_LOG_EXIT();
  return SAI_STATUS_SUCCESS;
}

/**
 * @brief Get hash attribute value
 *
 * @param[in] hash_id Hash id
 * @param[in] attr_count Number of attributes
 * @param[inout] attr_list Array of attributes
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_get_hash_attribute(_In_ sai_object_id_t hash_id,
                                    _In_ uint32_t attr_count,
                                    _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  sai_attribute_t *attribute = NULL;

  if (!hash_id || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter: %s", sai_metadata_get_status_name(status));
    return status;
  }

  attr_w hash_attr_list(SWITCH_HASH_ATTR_FIELD_LIST);
  const switch_object_id_t sw_object_id = {.data = hash_id};

  for (uint32_t index = 0; index < attr_count; index++) {
    attribute = &attr_list[index];
    switch (attribute->id) {
      case SAI_HASH_ATTR_NATIVE_HASH_FIELD_LIST: {
        switch_status = bf_switch_attribute_get(
            sw_object_id, SWITCH_HASH_ATTR_FIELD_LIST, hash_attr_list);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to retrieve hash attributes");
          return status;
        }
        std::vector<uint32_t> hash_field_list;
        hash_attr_list.v_get(hash_field_list);
        TRY_LIST_SET_WITH_CONVERTOR(attribute->value.s32list,
                                    hash_field_list,
                                    switch_hash_field_to_sai_field);
        break;
      }
      default:
        SAI_LOG_WARN("HASH attr %d is not implemented", attribute->id);
        return SAI_STATUS_ATTR_NOT_IMPLEMENTED_0 + index;
    }
  }
  SAI_LOG_EXIT();
  return status;
}

sai_hash_api_t hash_api = {
  create_hash : sai_create_hash,
  remove_hash : sai_remove_hash,
  set_hash_attribute : sai_set_hash_attribute,
  get_hash_attribute : sai_get_hash_attribute,
  create_fine_grained_hash_field : sai_create_fine_grained_hash_field,
  remove_fine_grained_hash_field : sai_remove_fine_grained_hash_field,
  set_fine_grained_hash_field_attribute :
      sai_set_fine_grained_hash_field_attribute,
  get_fine_grained_hash_field_attribute :
      sai_get_fine_grained_hash_field_attribute
};

sai_status_t sai_lag_hash_initialize(bool warm_init) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t switch_hash_object_id = {};

  SAI_LOG_DEBUG("Initializing Lag Hash object");

  // If warm reboot, no need create again
  if (warm_init) {
    return status;
  }

  device_handle = sai_get_device_id(0);
  std::vector<uint32_t> sw_hash_field_list = {
      SWITCH_HASH_ATTR_FIELD_INGRESS_PORT,
      SWITCH_HASH_ATTR_FIELD_MAC_TYPE,
      SWITCH_HASH_ATTR_FIELD_SRC_MAC,
      SWITCH_HASH_ATTR_FIELD_DST_MAC};

  // Creating a Hash Object
  std::set<attr_w> sw_hash_attrs;
  sai_insert_device_attribute(0, SWITCH_HASH_ATTR_DEVICE, sw_hash_attrs);
  sw_hash_attrs.insert(attr_w(SWITCH_HASH_ATTR_FIELD_LIST, sw_hash_field_list));
  switch_status = bf_switch_object_create(
      SWITCH_OBJECT_TYPE_HASH, sw_hash_attrs, switch_hash_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to create lag hash object");
    return status;
  }

  // Associating the created hash object to Lag Hash
  attr_w sattr(SWITCH_DEVICE_ATTR_NON_IP_HASH, switch_hash_object_id);
  switch_status = bf_switch_attribute_set(device_handle, sattr);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to set lag hash for switch object, error");
    return status;
  }
  return status;
}

sai_hash_api_t *sai_hash_api_get() { return &hash_api; }

sai_status_t sai_hash_initialize(bool warm_init) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  status = sai_lag_hash_initialize(warm_init);
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_HASH);
  bf_sai_add_object_type_to_supported_list(
      SAI_OBJECT_TYPE_FINE_GRAINED_HASH_FIELD);
  return status;
}
