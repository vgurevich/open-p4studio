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
#include <vector>
#include <set>
#include <map>

#include "s3/switch_store.h"

static sai_api_t api_id = SAI_API_QOS_MAP;

#define SAI_SWITCH_MAX_PCP_TC_COUNT 8

bool global_pcp_tc_qos_map_enabled;

namespace std {
template <>
struct hash<sai_qos_map_type_t> {
  inline size_t operator()(sai_qos_map_type_t const &type) const {
    return std::hash<uint64_t>{}(type);
  }
};
}  // namespace std

static const std::unordered_map<sai_qos_map_type_t, std::array<uint64_t, 2>>
    sai_qos_map_type_to_switch_mapping = {
        {SAI_QOS_MAP_TYPE_DOT1P_TO_TC,
         {SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_PCP_TO_TC,
          SWITCH_OBJECT_TYPE_QOS_MAP_INGRESS}},
        {SAI_QOS_MAP_TYPE_DOT1P_TO_COLOR,
         {SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_PCP_TO_COLOR,
          SWITCH_OBJECT_TYPE_QOS_MAP_INGRESS}},
        {SAI_QOS_MAP_TYPE_DSCP_TO_TC,
         {SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_DSCP_TO_TC,
          SWITCH_OBJECT_TYPE_QOS_MAP_INGRESS}},
        {SAI_QOS_MAP_TYPE_DSCP_TO_COLOR,
         {SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_DSCP_TO_COLOR,
          SWITCH_OBJECT_TYPE_QOS_MAP_INGRESS}},
        {SAI_QOS_MAP_TYPE_TC_TO_QUEUE,
         {SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_TC_TO_QUEUE,
          SWITCH_OBJECT_TYPE_QOS_MAP_INGRESS}},
        {SAI_QOS_MAP_TYPE_TC_AND_COLOR_TO_DSCP,
         {SWITCH_QOS_MAP_EGRESS_ATTR_TYPE_TC_AND_COLOR_TO_DSCP,
          SWITCH_OBJECT_TYPE_QOS_MAP_EGRESS}},
        {SAI_QOS_MAP_TYPE_TC_AND_COLOR_TO_DOT1P,
         {SWITCH_QOS_MAP_EGRESS_ATTR_TYPE_TC_AND_COLOR_TO_PCP,
          SWITCH_OBJECT_TYPE_QOS_MAP_EGRESS}},
        {SAI_QOS_MAP_TYPE_PFC_PRIORITY_TO_PRIORITY_GROUP,
         {SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_PFC_PRIORITY_TO_PPG,
          SWITCH_OBJECT_TYPE_QOS_MAP_INGRESS}},
        {SAI_QOS_MAP_TYPE_PFC_PRIORITY_TO_QUEUE,
         {SWITCH_QOS_MAP_EGRESS_ATTR_TYPE_PFC_PRIORITY_TO_QUEUE,
          SWITCH_OBJECT_TYPE_QOS_MAP_EGRESS}},
        {SAI_QOS_MAP_TYPE_TC_TO_PRIORITY_GROUP,
         {SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_TC_TO_ICOS,
          SWITCH_OBJECT_TYPE_QOS_MAP_INGRESS}}};

static const std::unordered_map<uint64_t, uint64_t>
    switch_qos_map_ingress_type_to_sai_mapping = {
        {SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_PCP_TO_TC,
         SAI_QOS_MAP_TYPE_DOT1P_TO_TC},
        {SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_PCP_TO_COLOR,
         SAI_QOS_MAP_TYPE_DOT1P_TO_COLOR},
        {SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_DSCP_TO_TC,
         SAI_QOS_MAP_TYPE_DSCP_TO_TC},
        {SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_DSCP_TO_COLOR,
         SAI_QOS_MAP_TYPE_DSCP_TO_COLOR},
        {SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_TC_TO_QUEUE,
         SAI_QOS_MAP_TYPE_TC_TO_QUEUE},
        {SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_PFC_PRIORITY_TO_PPG,
         SAI_QOS_MAP_TYPE_PFC_PRIORITY_TO_PRIORITY_GROUP},
        {SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_TC_TO_ICOS,
         SAI_QOS_MAP_TYPE_TC_TO_PRIORITY_GROUP}};

static const std::unordered_map<uint64_t, uint64_t>
    switch_qos_map_egress_type_to_sai_mapping = {
        {SWITCH_QOS_MAP_EGRESS_ATTR_TYPE_TC_AND_COLOR_TO_DSCP,
         SAI_QOS_MAP_TYPE_TC_AND_COLOR_TO_DSCP},
        {SWITCH_QOS_MAP_EGRESS_ATTR_TYPE_TC_AND_COLOR_TO_PCP,
         SAI_QOS_MAP_TYPE_TC_AND_COLOR_TO_DOT1P},
        {SWITCH_QOS_MAP_EGRESS_ATTR_TYPE_PFC_PRIORITY_TO_QUEUE,
         SAI_QOS_MAP_TYPE_PFC_PRIORITY_TO_QUEUE},
};

sai_status_t switch_qos_map_type_to_sai_qos_map_type(
    switch_object_type_t ot,
    switch_enum_t sw_qos_type,
    sai_qos_map_type_t *sai_qos_type) {
  if (ot == SWITCH_OBJECT_TYPE_QOS_MAP_INGRESS) {
    auto it =
        switch_qos_map_ingress_type_to_sai_mapping.find(sw_qos_type.enumdata);
    if (it != switch_qos_map_ingress_type_to_sai_mapping.end()) {
      *sai_qos_type = static_cast<sai_qos_map_type_t>(it->second);
      return SAI_STATUS_SUCCESS;
    }
  } else if (ot == SWITCH_OBJECT_TYPE_QOS_MAP_EGRESS) {
    auto it =
        switch_qos_map_egress_type_to_sai_mapping.find(sw_qos_type.enumdata);
    if (it != switch_qos_map_egress_type_to_sai_mapping.end()) {
      *sai_qos_type = static_cast<sai_qos_map_type_t>(it->second);
      return SAI_STATUS_SUCCESS;
    }
  }
  return SAI_STATUS_NOT_SUPPORTED;
}

static switch_qos_map_attr_color sai_color_to_switch_color(
    sai_packet_color_t color) {
  switch (color) {
    case SAI_PACKET_COLOR_GREEN:
      return SWITCH_QOS_MAP_ATTR_COLOR_GREEN;
    case SAI_PACKET_COLOR_YELLOW:
      return SWITCH_QOS_MAP_ATTR_COLOR_YELLOW;
    case SAI_PACKET_COLOR_RED:
      return SWITCH_QOS_MAP_ATTR_COLOR_RED;
    default:
      return SWITCH_QOS_MAP_ATTR_COLOR_GREEN;
  }
}

static sai_packet_color_t switch_color_to_sai_color(
    switch_qos_map_attr_color switch_color) {
  switch (switch_color) {
    case SWITCH_QOS_MAP_ATTR_COLOR_GREEN:
      return SAI_PACKET_COLOR_GREEN;
    case SWITCH_QOS_MAP_ATTR_COLOR_YELLOW:
      return SAI_PACKET_COLOR_YELLOW;
    case SWITCH_QOS_MAP_ATTR_COLOR_RED:
      return SAI_PACKET_COLOR_RED;
    default:
      return SAI_PACKET_COLOR_GREEN;
  }
}

static void sai_qos_map_to_switch_attr(sai_qos_map_type_t qos_map_type,
                                       const sai_qos_map_t &qos_map,
                                       std::set<smi::attr_w> &attrs) {
  switch_enum_t color = {.enumdata = SWITCH_QOS_MAP_ATTR_COLOR_GREEN};
  uint16_t tc = 0;

  switch (qos_map_type) {
    case SAI_QOS_MAP_TYPE_DOT1P_TO_TC:
      attrs.insert(attr_w(SWITCH_QOS_MAP_ATTR_PCP, qos_map.key.dot1p));
      tc = qos_map.value.tc;
      attrs.insert(attr_w(SWITCH_QOS_MAP_ATTR_TC, tc));
      break;
    case SAI_QOS_MAP_TYPE_DOT1P_TO_COLOR:
      attrs.insert(attr_w(SWITCH_QOS_MAP_ATTR_PCP, qos_map.key.dot1p));
      color.enumdata = sai_color_to_switch_color(qos_map.value.color);
      attrs.insert(attr_w(SWITCH_QOS_MAP_ATTR_COLOR, color));
      break;
    case SAI_QOS_MAP_TYPE_DSCP_TO_TC:
      attrs.insert(attr_w(SWITCH_QOS_MAP_ATTR_DSCP, qos_map.key.dscp));
      tc = qos_map.value.tc;
      attrs.insert(attr_w(SWITCH_QOS_MAP_ATTR_TC, tc));
      break;
    case SAI_QOS_MAP_TYPE_DSCP_TO_COLOR:
      attrs.insert(attr_w(SWITCH_QOS_MAP_ATTR_DSCP, qos_map.key.dscp));
      color.enumdata = sai_color_to_switch_color(qos_map.value.color);
      attrs.insert(attr_w(SWITCH_QOS_MAP_ATTR_COLOR, color));
      break;
    case SAI_QOS_MAP_TYPE_TC_TO_QUEUE:
      tc = qos_map.key.tc;
      attrs.insert(attr_w(SWITCH_QOS_MAP_ATTR_TC, tc));
      attrs.insert(attr_w(SWITCH_QOS_MAP_ATTR_QID, qos_map.value.queue_index));
      break;
    case SAI_QOS_MAP_TYPE_TC_AND_COLOR_TO_DSCP:
      tc = qos_map.key.tc;
      attrs.insert(attr_w(SWITCH_QOS_MAP_ATTR_TC, tc));
      color.enumdata = sai_color_to_switch_color(qos_map.key.color);
      attrs.insert(attr_w(SWITCH_QOS_MAP_ATTR_COLOR, color));
      attrs.insert(attr_w(SWITCH_QOS_MAP_ATTR_DSCP, qos_map.value.dscp));
      break;
    case SAI_QOS_MAP_TYPE_TC_AND_COLOR_TO_DOT1P:
      tc = qos_map.key.tc;
      attrs.insert(attr_w(SWITCH_QOS_MAP_ATTR_TC, tc));
      color.enumdata = sai_color_to_switch_color(qos_map.key.color);
      attrs.insert(attr_w(SWITCH_QOS_MAP_ATTR_COLOR, color));
      attrs.insert(attr_w(SWITCH_QOS_MAP_ATTR_PCP, qos_map.value.dot1p));
      break;
    case SAI_QOS_MAP_TYPE_TC_TO_PRIORITY_GROUP:
      tc = qos_map.key.tc;
      attrs.insert(attr_w(SWITCH_QOS_MAP_ATTR_TC, tc));
      attrs.insert(attr_w(SWITCH_QOS_MAP_ATTR_ICOS, qos_map.value.pg));
      break;
    case SAI_QOS_MAP_TYPE_PFC_PRIORITY_TO_PRIORITY_GROUP:
      attrs.insert(attr_w(SWITCH_QOS_MAP_ATTR_ICOS, qos_map.key.prio));
      attrs.insert(attr_w(SWITCH_QOS_MAP_ATTR_PPG, qos_map.value.pg));
      break;
    case SAI_QOS_MAP_TYPE_PFC_PRIORITY_TO_QUEUE:
      attrs.insert(attr_w(SWITCH_QOS_MAP_ATTR_PFC_PRIORITY, qos_map.key.prio));
      attrs.insert(attr_w(SWITCH_QOS_MAP_ATTR_QID, qos_map.value.queue_index));
      break;
    default:
      break;
  }
}

static sai_status_t switch_qos_map_attr_get(
    switch_object_id_t sw_oid,
    switch_attr_id_t sw_attr_id,
    sai_qos_map_params_t &sai_qos_attr) {
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_enum_t color = {.enumdata = SWITCH_QOS_MAP_ATTR_COLOR_GREEN};
  uint16_t tc = 0;
  attr_w attr(sw_attr_id);

  switch_status = bf_switch_attribute_get(sw_oid, sw_attr_id, attr);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get QoS map attribute %" PRIu16 " : %s",
                  sw_attr_id,
                  sai_metadata_get_status_name(status));
    return status;
  }

  switch (sw_attr_id) {
    case SWITCH_QOS_MAP_ATTR_DSCP:
      attr.v_get(sai_qos_attr.dscp);
      break;
    case SWITCH_QOS_MAP_ATTR_PCP:
      attr.v_get(sai_qos_attr.dot1p);
      break;
    case SWITCH_QOS_MAP_ATTR_TC:
      attr.v_get(tc);
      sai_qos_attr.tc = static_cast<sai_cos_t>(tc);
      break;
    case SWITCH_QOS_MAP_ATTR_COLOR:
      attr.v_get(color);
      sai_qos_attr.color = switch_color_to_sai_color(
          static_cast<switch_qos_map_attr_color>(color.enumdata));
      break;
    case SWITCH_QOS_MAP_ATTR_ICOS:
      // SAI_QOS_MAP_TYPE_TC_TO_PRIORITY_GROUP            : value.pg
      // SAI_QOS_MAP_TYPE_PFC_PRIORITY_TO_PRIORITY_GROUP  : key.prio
      attr.v_get(sai_qos_attr.pg);
      break;
    case SWITCH_QOS_MAP_ATTR_QID:
      attr.v_get(sai_qos_attr.queue_index);
      break;
    case SWITCH_QOS_MAP_ATTR_PFC_PRIORITY:
      attr.v_get(sai_qos_attr.prio);
      break;
    case SWITCH_QOS_MAP_ATTR_PPG:
      attr.v_get(sai_qos_attr.pg);
      break;
    default:
      break;
  }
  return status;
}

static sai_status_t switch_qos_map_get(switch_object_id_t oid,
                                       sai_qos_map_type_t qos_type,
                                       sai_qos_map_t &qos_map) {
  sai_status_t status = SAI_STATUS_SUCCESS;

  switch (qos_type) {
    case SAI_QOS_MAP_TYPE_DOT1P_TO_TC:
      switch_qos_map_attr_get(oid, SWITCH_QOS_MAP_ATTR_PCP, qos_map.key);
      switch_qos_map_attr_get(oid, SWITCH_QOS_MAP_ATTR_TC, qos_map.value);
      break;
    case SAI_QOS_MAP_TYPE_DOT1P_TO_COLOR:
      switch_qos_map_attr_get(oid, SWITCH_QOS_MAP_ATTR_PCP, qos_map.key);
      switch_qos_map_attr_get(oid, SWITCH_QOS_MAP_ATTR_COLOR, qos_map.value);
      break;
    case SAI_QOS_MAP_TYPE_DSCP_TO_TC:
      switch_qos_map_attr_get(oid, SWITCH_QOS_MAP_ATTR_DSCP, qos_map.key);
      switch_qos_map_attr_get(oid, SWITCH_QOS_MAP_ATTR_TC, qos_map.value);
      break;
    case SAI_QOS_MAP_TYPE_DSCP_TO_COLOR:
      switch_qos_map_attr_get(oid, SWITCH_QOS_MAP_ATTR_DSCP, qos_map.key);
      switch_qos_map_attr_get(oid, SWITCH_QOS_MAP_ATTR_COLOR, qos_map.value);
      break;
    case SAI_QOS_MAP_TYPE_TC_TO_QUEUE:
      switch_qos_map_attr_get(oid, SWITCH_QOS_MAP_ATTR_TC, qos_map.key);
      switch_qos_map_attr_get(oid, SWITCH_QOS_MAP_ATTR_QID, qos_map.value);
      break;
    case SAI_QOS_MAP_TYPE_TC_AND_COLOR_TO_DSCP:
      switch_qos_map_attr_get(oid, SWITCH_QOS_MAP_ATTR_TC, qos_map.key);
      switch_qos_map_attr_get(oid, SWITCH_QOS_MAP_ATTR_COLOR, qos_map.key);
      switch_qos_map_attr_get(oid, SWITCH_QOS_MAP_ATTR_DSCP, qos_map.value);
      break;
    case SAI_QOS_MAP_TYPE_TC_AND_COLOR_TO_DOT1P:
      switch_qos_map_attr_get(oid, SWITCH_QOS_MAP_ATTR_TC, qos_map.key);
      switch_qos_map_attr_get(oid, SWITCH_QOS_MAP_ATTR_COLOR, qos_map.key);
      switch_qos_map_attr_get(oid, SWITCH_QOS_MAP_ATTR_PCP, qos_map.value);
      break;
    case SAI_QOS_MAP_TYPE_PFC_PRIORITY_TO_PRIORITY_GROUP:
      switch_qos_map_attr_get(oid, SWITCH_QOS_MAP_ATTR_ICOS, qos_map.key);
      qos_map.key.prio = qos_map.key.pg;
      switch_qos_map_attr_get(oid, SWITCH_QOS_MAP_ATTR_PPG, qos_map.value);
      break;
    case SAI_QOS_MAP_TYPE_PFC_PRIORITY_TO_QUEUE:
      switch_qos_map_attr_get(
          oid, SWITCH_QOS_MAP_ATTR_PFC_PRIORITY, qos_map.key);
      switch_qos_map_attr_get(oid, SWITCH_QOS_MAP_ATTR_QID, qos_map.value);
      break;
    case SAI_QOS_MAP_TYPE_TC_TO_PRIORITY_GROUP:
      switch_qos_map_attr_get(oid, SWITCH_QOS_MAP_ATTR_TC, qos_map.key);
      switch_qos_map_attr_get(oid, SWITCH_QOS_MAP_ATTR_ICOS, qos_map.value);
      break;
    default:
      status = SAI_STATUS_NOT_SUPPORTED;
  }
  return status;
}

/**
 * @brief Create Qos Map
 *
 * @param[out] qos_map_id Qos Map Id
 * @param[in] switch_id Switch id
 * @param[in] attr_count number of attributes
 * @param[in] attr_list array of attributes
 *
 * @return  SAI_STATUS_SUCCESS on success
 *          Failure status code on error
 */
sai_status_t sai_create_qos_map(_Out_ sai_object_id_t *qos_map_id,
                                _In_ sai_object_id_t switch_id,
                                _In_ uint32_t attr_count,
                                _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  const sai_attribute_t *attribute;
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  sai_qos_map_type_t qos_map_type = SAI_QOS_MAP_TYPE_DOT1P_TO_TC;
  switch_object_type_t ot = SWITCH_OBJECT_TYPE_QOS_MAP_INGRESS;
  switch_attr_id_t type_attr_id = 0;
  switch_attr_id_t list_attr_id = 0;
  switch_attr_id_t dev_attr_id = 0;
  std::set<smi::attr_w> sw_attrs;

  if (!qos_map_id || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  attribute =
      sai_get_attr_from_list(SAI_QOS_MAP_ATTR_TYPE, attr_list, attr_count);
  if (attribute == NULL) {
    SAI_LOG_ERROR(
        "QoS map attribute parse failed. SAI_QOS_MAP_ATTR_TYPE attribute is "
        "missing");
    return SAI_STATUS_MANDATORY_ATTRIBUTE_MISSING;
  }

  qos_map_type = static_cast<sai_qos_map_type_t>(attribute->value.s32);
  switch_enum_t sw_qos_map_type = {0};

  std::unordered_map<sai_qos_map_type_t, std::array<uint64_t, 2>> map;

  auto it = sai_qos_map_type_to_switch_mapping.find(qos_map_type);
  if (it != sai_qos_map_type_to_switch_mapping.end()) {
    sw_qos_map_type.enumdata = it->second[0];
    ot = it->second[1];
  } else {
    status = SAI_STATUS_NOT_SUPPORTED;
    SAI_LOG_ERROR("QoS map attribute parse failed %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (ot == SWITCH_OBJECT_TYPE_QOS_MAP_INGRESS) {
    dev_attr_id = SWITCH_QOS_MAP_INGRESS_ATTR_DEVICE;
    type_attr_id = SWITCH_QOS_MAP_INGRESS_ATTR_TYPE;
    list_attr_id = SWITCH_QOS_MAP_INGRESS_ATTR_QOS_MAP_LIST;
  } else {
    dev_attr_id = SWITCH_QOS_MAP_EGRESS_ATTR_DEVICE;
    type_attr_id = SWITCH_QOS_MAP_EGRESS_ATTR_TYPE;
    list_attr_id = SWITCH_QOS_MAP_EGRESS_ATTR_QOS_MAP_LIST;
  }

  // device
  sai_insert_device_attribute(0, dev_attr_id, sw_attrs);

  // type
  sw_attrs.insert(attr_w(type_attr_id, sw_qos_map_type));

  attribute = sai_get_attr_from_list(
      SAI_QOS_MAP_ATTR_MAP_TO_VALUE_LIST, attr_list, attr_count);
  if (attribute == NULL) {
    SAI_LOG_ERROR(
        "QoS map attribute parse failed. SAI_QOS_MAP_ATTR_MAP_TO_VALUE_LIST "
        "attribute is missing");
    return SAI_STATUS_MANDATORY_ATTRIBUTE_MISSING;
  }

  std::vector<switch_object_id_t> qos_map_list;

  for (uint32_t j = 0; j < attribute->value.qosmap.count; j++) {
    std::set<smi::attr_w> qos_map_attr;
    switch_object_id_t oid = {};
    sai_qos_map_to_switch_attr(
        qos_map_type, attribute->value.qosmap.list[j], qos_map_attr);
    switch_status =
        bf_switch_object_create(SWITCH_OBJECT_TYPE_QOS_MAP, qos_map_attr, oid);
    status = status_switch_to_sai(switch_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to create QoS map : %s",
                    sai_metadata_get_status_name(status));
      return status;
    }
    qos_map_list.push_back(oid);
  }

  // qos_map_list
  smi::attr_w qos_map_list_attr(list_attr_id);
  qos_map_list_attr.v_set(qos_map_list);
  sw_attrs.insert(qos_map_list_attr);

  switch_object_id_t qos_map_object_id = {};
  switch_status = bf_switch_object_create(ot, sw_attrs, qos_map_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to create QoS map : %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *qos_map_id = qos_map_object_id.data;
  SAI_LOG_EXIT();

  return status;
}

/**
 * @brief Remove Qos Map
 *
 *  @param[in] qos_map_id Qos Map id to be removed.
 *
 *  @return  SAI_STATUS_SUCCESS on success
 *           Failure status code on error
 */
sai_status_t sai_remove_qos_map(_In_ sai_object_id_t qos_map_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t sw_object_id = {.data = qos_map_id};
  switch_object_type_t ot = switch_store::object_type_query(sw_object_id);
  switch_attr_id_t list_attr_id = 0;

  if (ot == SWITCH_OBJECT_TYPE_QOS_MAP_INGRESS) {
    list_attr_id = SWITCH_QOS_MAP_INGRESS_ATTR_QOS_MAP_LIST;
  } else if (ot == SWITCH_OBJECT_TYPE_QOS_MAP_EGRESS) {
    list_attr_id = SWITCH_QOS_MAP_EGRESS_ATTR_QOS_MAP_LIST;
  } else {
    SAI_LOG_ERROR("Invalid object 0x%" PRIx64, qos_map_id);
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  attr_w attr(list_attr_id);
  switch_status = bf_switch_attribute_get(sw_object_id, list_attr_id, attr);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get QoS map list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }
  if (attr.type_get() != SWITCH_TYPE_LIST) {
    SAI_LOG_ERROR("Invalid SMI attribute type %u", attr.type_get());
    return SAI_STATUS_FAILURE;
  }
  switch_attr_type_t attr_type = attr.list_type_get();
  if (attr_type != SWITCH_TYPE_OBJECT_ID) {
    SAI_LOG_ERROR("Invalid SMI list type %u", attr_type);
    return SAI_STATUS_FAILURE;
  }

  switch_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to delete QoS map: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  std::vector<switch_object_id_t> qos_map_handles;
  attr.v_get(qos_map_handles);
  for (auto sw_oid : qos_map_handles) {
    switch_status = bf_switch_object_delete(sw_oid);
    status = status_switch_to_sai(switch_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_WARN("Failed to delete QoS map entry: %s",
                   sai_metadata_get_status_name(status));
    }
  }
  SAI_LOG_EXIT();

  return SAI_STATUS_SUCCESS;
}

static sai_status_t switch_qos_map_update(
    const switch_object_id_t qos_map_id,
    sai_qos_map_type_t qos_map_type,
    std::vector<switch_object_id_t> &qos_map_handles,
    const sai_attribute_t *attr) {
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  sai_status_t status = SAI_STATUS_SUCCESS;
  sai_qos_map_t current_entry;
  std::vector<switch_object_id_t> updated_qos_map_handles;
  switch_enum_t color = {.enumdata = SWITCH_QOS_MAP_ATTR_COLOR_GREEN};
  uint16_t tc = 0;

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Failed to update QoS map: Null pointer argument: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  // Loop through the update list and update/add new entries in SMI
  for (uint32_t i = 0; i < attr->value.qosmap.count; i++) {
    auto update_entry = attr->value.qosmap.list[i];
    bool update = false;

    for (auto sw_oid : qos_map_handles) {
      if (update) {
        // Once the entry was found and updated no need to iterate more
        break;
      }

      status = switch_qos_map_get(sw_oid, qos_map_type, current_entry);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to get QoS map for type %u: %s",
                      qos_map_type,
                      sai_metadata_get_status_name(status));
        return status;
      }

      switch (qos_map_type) {
        case SAI_QOS_MAP_TYPE_DOT1P_TO_TC: {
          if (current_entry.key.dot1p == update_entry.key.dot1p) {
            tc = update_entry.value.tc;
            switch_status = bf_switch_attribute_set(
                sw_oid, attr_w(SWITCH_QOS_MAP_ATTR_TC, tc));
            status = status_switch_to_sai(switch_status);
            if (status != SAI_STATUS_SUCCESS) {
              SAI_LOG_ERROR("Failed to update QoS map entry 0x%" PRIx64 ": %s",
                            sw_oid.data,
                            sai_metadata_get_status_name(status));
              return status;
            }
            update = true;
            updated_qos_map_handles.push_back(sw_oid);
          }
        } break;
        case SAI_QOS_MAP_TYPE_DOT1P_TO_COLOR: {
          if (current_entry.key.dot1p == update_entry.key.dot1p) {
            color.enumdata =
                sai_color_to_switch_color(update_entry.value.color);
            switch_status = bf_switch_attribute_set(
                sw_oid, attr_w(SWITCH_QOS_MAP_ATTR_COLOR, color));
            status = status_switch_to_sai(switch_status);
            if (status != SAI_STATUS_SUCCESS) {
              SAI_LOG_ERROR("Failed to update QoS map entry 0x%" PRIx64 ": %s",
                            sw_oid.data,
                            sai_metadata_get_status_name(status));
              return status;
            }
            update = true;
            updated_qos_map_handles.push_back(sw_oid);
          }
        } break;
        case SAI_QOS_MAP_TYPE_DSCP_TO_TC: {
          if (current_entry.key.dscp == update_entry.key.dscp) {
            tc = update_entry.value.tc;
            switch_status = bf_switch_attribute_set(
                sw_oid, attr_w(SWITCH_QOS_MAP_ATTR_TC, tc));
            status = status_switch_to_sai(switch_status);
            if (status != SAI_STATUS_SUCCESS) {
              SAI_LOG_ERROR("Failed to update QoS map entry 0x%" PRIx64 ": %s",
                            sw_oid.data,
                            sai_metadata_get_status_name(status));
              return status;
            }
            update = true;
            updated_qos_map_handles.push_back(sw_oid);
          }
        } break;
        case SAI_QOS_MAP_TYPE_DSCP_TO_COLOR: {
          if (current_entry.key.dscp == update_entry.key.dscp) {
            color.enumdata =
                sai_color_to_switch_color(update_entry.value.color);
            switch_status = bf_switch_attribute_set(
                sw_oid, attr_w(SWITCH_QOS_MAP_ATTR_COLOR, color));
            status = status_switch_to_sai(switch_status);
            if (status != SAI_STATUS_SUCCESS) {
              SAI_LOG_ERROR("Failed to update QoS map entry 0x%" PRIx64 ": %s",
                            sw_oid.data,
                            sai_metadata_get_status_name(status));
              return status;
            }
            update = true;
            updated_qos_map_handles.push_back(sw_oid);
          }
        } break;
        case SAI_QOS_MAP_TYPE_TC_TO_QUEUE: {
          if (current_entry.key.tc == update_entry.key.tc) {
            switch_status =
                bf_switch_attribute_set(sw_oid,
                                        attr_w(SWITCH_QOS_MAP_ATTR_QID,
                                               update_entry.value.queue_index));
            status = status_switch_to_sai(switch_status);
            if (status != SAI_STATUS_SUCCESS) {
              SAI_LOG_ERROR("Failed to update QoS map entry 0x%" PRIx64 ": %s",
                            sw_oid.data,
                            sai_metadata_get_status_name(status));
              return status;
            }
            update = true;
            updated_qos_map_handles.push_back(sw_oid);
          }
        } break;
        case SAI_QOS_MAP_TYPE_TC_AND_COLOR_TO_DSCP: {
          if ((current_entry.key.tc == update_entry.key.tc) &&
              (current_entry.key.color == update_entry.key.color)) {
            switch_status = bf_switch_attribute_set(
                sw_oid,
                attr_w(SWITCH_QOS_MAP_ATTR_DSCP, update_entry.value.dscp));
            status = status_switch_to_sai(switch_status);
            if (status != SAI_STATUS_SUCCESS) {
              SAI_LOG_ERROR("Failed to update QoS map entry 0x%" PRIx64 ": %s",
                            sw_oid.data,
                            sai_metadata_get_status_name(status));
              return status;
            }
            update = true;
            updated_qos_map_handles.push_back(sw_oid);
          }
        } break;
        case SAI_QOS_MAP_TYPE_TC_AND_COLOR_TO_DOT1P: {
          if ((current_entry.key.tc == update_entry.key.tc) &&
              (current_entry.key.color == update_entry.key.color)) {
            switch_status = bf_switch_attribute_set(
                sw_oid,
                attr_w(SWITCH_QOS_MAP_ATTR_PCP, update_entry.value.dot1p));
            status = status_switch_to_sai(switch_status);
            if (status != SAI_STATUS_SUCCESS) {
              SAI_LOG_ERROR("Failed to update QoS map entry 0x%" PRIx64 ": %s",
                            sw_oid.data,
                            sai_metadata_get_status_name(status));
              return status;
            }
            update = true;
            updated_qos_map_handles.push_back(sw_oid);
          }
        } break;
        case SAI_QOS_MAP_TYPE_TC_TO_PRIORITY_GROUP: {
          if (current_entry.key.tc == update_entry.key.tc) {
            switch_status = bf_switch_attribute_set(
                sw_oid,
                attr_w(SWITCH_QOS_MAP_ATTR_ICOS, update_entry.value.pg));
            status = status_switch_to_sai(switch_status);
            if (status != SAI_STATUS_SUCCESS) {
              SAI_LOG_ERROR("Failed to update QoS map entry 0x%" PRIx64 ": %s",
                            sw_oid.data,
                            sai_metadata_get_status_name(status));
              return status;
            }
            update = true;
            updated_qos_map_handles.push_back(sw_oid);
          }
        } break;
        case SAI_QOS_MAP_TYPE_PFC_PRIORITY_TO_PRIORITY_GROUP: {
          if (current_entry.key.prio == update_entry.key.prio) {
            switch_status = bf_switch_attribute_set(
                sw_oid, attr_w(SWITCH_QOS_MAP_ATTR_PPG, update_entry.value.pg));
            status = status_switch_to_sai(switch_status);
            if (status != SAI_STATUS_SUCCESS) {
              SAI_LOG_ERROR("Failed to update QoS map entry 0x%" PRIx64 ": %s",
                            sw_oid.data,
                            sai_metadata_get_status_name(status));
              return status;
            }
            update = true;
            updated_qos_map_handles.push_back(sw_oid);
          }
        } break;
        case SAI_QOS_MAP_TYPE_PFC_PRIORITY_TO_QUEUE: {
          if (current_entry.key.prio == update_entry.key.prio) {
            switch_status =
                bf_switch_attribute_set(sw_oid,
                                        attr_w(SWITCH_QOS_MAP_ATTR_QID,
                                               update_entry.value.queue_index));
            status = status_switch_to_sai(switch_status);
            if (status != SAI_STATUS_SUCCESS) {
              SAI_LOG_ERROR("Failed to update QoS map entry 0x%" PRIx64 ": %s",
                            sw_oid.data,
                            sai_metadata_get_status_name(status));
              return status;
            }
            update = true;
            updated_qos_map_handles.push_back(sw_oid);
          }
        } break;
        default:
          return SAI_STATUS_ITEM_NOT_FOUND;
      }
    }

    if (!update) {
      // Add a new map entry
      std::set<smi::attr_w> qos_map_attr;
      switch_object_id_t new_qos_map = {0};
      sai_qos_map_to_switch_attr(qos_map_type, update_entry, qos_map_attr);
      switch_status = bf_switch_object_create(
          SWITCH_OBJECT_TYPE_QOS_MAP, qos_map_attr, new_qos_map);
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to create a new QoS map entry : %s",
                      sai_metadata_get_status_name(status));
        return status;
      }
      // New Qosmap entry has been added. Update the list of handles.
      updated_qos_map_handles.push_back(new_qos_map);
    }
  }

  // Set the updated list of qos map entries to qos_map
  switch_object_type_t ot = switch_store::object_type_query(qos_map_id);
  switch_attr_id_t list_attr_id = 0;

  if (ot == SWITCH_OBJECT_TYPE_QOS_MAP_INGRESS) {
    list_attr_id = SWITCH_QOS_MAP_INGRESS_ATTR_QOS_MAP_LIST;
  } else if (ot == SWITCH_OBJECT_TYPE_QOS_MAP_EGRESS) {
    list_attr_id = SWITCH_QOS_MAP_EGRESS_ATTR_QOS_MAP_LIST;
  } else {
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  smi::attr_w qos_map_list_attr(list_attr_id);
  qos_map_list_attr.v_set(updated_qos_map_handles);

  switch_status = bf_switch_attribute_set(qos_map_id, qos_map_list_attr);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to add new entry to the qosmap list 0x%" PRIx64 ":%s",
                  qos_map_id.data,
                  sai_metadata_get_status_name(status));
    return status;
  }

  // Now find the entries which have to be removed and remove them.
  for (auto old_qos_map : qos_map_handles) {
    bool found = false;
    for (auto updated_qos_map : updated_qos_map_handles) {
      if (old_qos_map.data == updated_qos_map.data) {
        found = true;
        break;
      }
    }

    if (!found) {
      // The entry form the old list is not present in the updated one and hence
      // has to be removed
      switch_status = bf_switch_object_delete(old_qos_map);
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to remove qos map 0x%" PRIx64 ": %s",
                      old_qos_map.data,
                      sai_metadata_get_status_name(status));
        return status;
      }
    }
  }

  return status;
}

/**
 * @brief Set attributes for qos map
 *
 * @param[in] qos_map_id Qos Map Id
 * @param[in] attr attribute to set
 *
 * @return  SAI_STATUS_SUCCESS on success
 *          Failure status code on error
 */

sai_status_t sai_set_qos_map_attribute(_In_ sai_object_id_t qos_map_id,
                                       _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t sw_object_id = {.data = qos_map_id};
  switch_object_type_t ot = switch_store::object_type_query(sw_object_id);
  switch_attr_id_t type_attr_id = 0;
  switch_attr_id_t list_attr_id = 0;
  switch_enum_t sw_qos_type = {0};
  sai_qos_map_type_t sai_qos_type;

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute : %s", sai_metadata_get_status_name(status));
    return status;
  }

  if (ot == SWITCH_OBJECT_TYPE_QOS_MAP_INGRESS) {
    type_attr_id = SWITCH_QOS_MAP_INGRESS_ATTR_TYPE;
    list_attr_id = SWITCH_QOS_MAP_INGRESS_ATTR_QOS_MAP_LIST;
  } else if (ot == SWITCH_OBJECT_TYPE_QOS_MAP_EGRESS) {
    type_attr_id = SWITCH_QOS_MAP_EGRESS_ATTR_TYPE;
    list_attr_id = SWITCH_QOS_MAP_EGRESS_ATTR_QOS_MAP_LIST;
  } else {
    SAI_LOG_ERROR("Invalid object 0x%" PRIx64, qos_map_id);
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  // Get QoS type
  attr_w type_attr(type_attr_id);
  switch_status =
      bf_switch_attribute_get(sw_object_id, type_attr_id, type_attr);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get QoS map type: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  type_attr.v_get(sw_qos_type);
  status =
      switch_qos_map_type_to_sai_qos_map_type(ot, sw_qos_type, &sai_qos_type);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR(
        "Failed to map switch qosmap type to sai type for handle 0x%" PRIx64
        " : %s",
        qos_map_id,
        sai_metadata_get_status_name(status));
    return status;
  }

  // Get QoS current map list
  attr_w list_attr(list_attr_id);
  switch_status =
      bf_switch_attribute_get(sw_object_id, list_attr_id, list_attr);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get QoS map list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }
  if (list_attr.type_get() != SWITCH_TYPE_LIST) {
    SAI_LOG_ERROR("Invalid SMI attribute type %u", list_attr.type_get());
    return SAI_STATUS_FAILURE;
  }

  std::vector<switch_object_id_t> qos_map_handles;
  list_attr.v_get(qos_map_handles);

  status =
      switch_qos_map_update(sw_object_id, sai_qos_type, qos_map_handles, attr);
  if (status != SAI_STATUS_ITEM_NOT_FOUND && status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to update QoS map 0x%" PRIx64 " for type %u: %s",
                  sw_object_id.data,
                  sai_qos_type,
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();
  return status;
}

/**
 * @brief  Get attrbutes of qos map
 *
 * @param[in] qos_map_id  map id
 * @param[in] attr_count  number of attributes
 * @param[inout] attr_list  array of attributes
 *
 * @return SAI_STATUS_SUCCESS on success
 *        Failure status code on error
 */

sai_status_t sai_get_qos_map_attribute(_In_ sai_object_id_t qos_map_id,
                                       _In_ uint32_t attr_count,
                                       _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t sw_object_id = {.data = qos_map_id};
  switch_object_type_t ot = switch_store::object_type_query(sw_object_id);
  switch_attr_id_t type_attr_id = 0;
  switch_attr_id_t list_attr_id = 0;
  switch_enum_t sw_qos_type = {0};
  sai_qos_map_type_t sai_qos_type;

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (ot == SWITCH_OBJECT_TYPE_QOS_MAP_INGRESS) {
    type_attr_id = SWITCH_QOS_MAP_INGRESS_ATTR_TYPE;
    list_attr_id = SWITCH_QOS_MAP_INGRESS_ATTR_QOS_MAP_LIST;
  } else if (ot == SWITCH_OBJECT_TYPE_QOS_MAP_EGRESS) {
    type_attr_id = SWITCH_QOS_MAP_EGRESS_ATTR_TYPE;
    list_attr_id = SWITCH_QOS_MAP_EGRESS_ATTR_QOS_MAP_LIST;
  } else {
    SAI_LOG_ERROR("Invalid object 0x%" PRIx64, qos_map_id);
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  attr_w type_attr(type_attr_id);
  switch_status =
      bf_switch_attribute_get(sw_object_id, type_attr_id, type_attr);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get QoS map type: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  type_attr.v_get(sw_qos_type);

  status =
      switch_qos_map_type_to_sai_qos_map_type(ot, sw_qos_type, &sai_qos_type);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR(
        "Failed to map switch qosmap type to sai type for handle 0x%" PRIx64
        " : %s",
        qos_map_id,
        sai_metadata_get_status_name(status));
    return status;
  }

  sai_attribute_t *attribute;
  for (uint32_t i = 0; i < attr_count; i++) {
    attribute = &attr_list[i];
    switch (attribute->id) {
      case SAI_QOS_MAP_ATTR_TYPE: {
        attribute->value.s32 = sai_qos_type;
        break;
      }
      case SAI_QOS_MAP_ATTR_MAP_TO_VALUE_LIST: {
        attr_w attr(list_attr_id);
        switch_status =
            bf_switch_attribute_get(sw_object_id, list_attr_id, attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to get QoS map list: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
        if (attr.type_get() != SWITCH_TYPE_LIST) {
          SAI_LOG_ERROR("Invalid SMI attribute type %u", attr.type_get());
          return SAI_STATUS_FAILURE;
        }
        std::vector<switch_object_id_t> qos_map_handles;
        attr.v_get(qos_map_handles);
        if (attribute->value.qosmap.count < qos_map_handles.size()) {
          attribute->value.qosmap.count = qos_map_handles.size();
          return SAI_STATUS_BUFFER_OVERFLOW;
        }
        switch_attr_type_t attr_type = attr.list_type_get();
        if (attr_type != SWITCH_TYPE_OBJECT_ID) {
          SAI_LOG_ERROR("Invalid SMI list type %u", attr_type);
          return SAI_STATUS_FAILURE;
        }
        int j = 0;
        attribute->value.qosmap.count = qos_map_handles.size();
        for (auto sw_oid : qos_map_handles) {
          status = switch_qos_map_get(
              sw_oid, sai_qos_type, attribute->value.qosmap.list[j]);
          if (status != SAI_STATUS_SUCCESS) {
            SAI_LOG_ERROR("Failed to get QoS map for type %u: %s",
                          sai_qos_type,
                          sai_metadata_get_status_name(status));
            return status;
          }
          ++j;
        }
        break;
      }
      default:
        status = SAI_STATUS_NOT_SUPPORTED;
        SAI_LOG_WARN("The attribute %u is not supported: %s",
                     attribute->id,
                     sai_metadata_get_status_name(status));
        return status;
    }
  }
  SAI_LOG_EXIT();
  return status;
}

// Not in use - global PCP-TC can be config per port now
switch_status_t sai_init_global_pcp_tc_qosmap(uint16_t dev_id) {
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  std::vector<switch_object_id_t> qos_map_list;

  for (uint8_t j = 0; j < SAI_SWITCH_MAX_PCP_TC_COUNT; j++) {
    std::set<smi::attr_w> qos_map_attr;
    switch_object_id_t oid = {};

    qos_map_attr.insert(smi::attr_w(SWITCH_QOS_MAP_ATTR_PCP, j));
    qos_map_attr.insert(
        smi::attr_w(SWITCH_QOS_MAP_ATTR_TC, static_cast<uint16_t>(j)));
    switch_status =
        bf_switch_object_create(SWITCH_OBJECT_TYPE_QOS_MAP, qos_map_attr, oid);
    if (switch_status != SWITCH_STATUS_SUCCESS) {
      SAI_LOG_ERROR(
          "Failed to create QoS map : %s",
          sai_metadata_get_status_name(status_switch_to_sai(switch_status)));
      return switch_status;
    }
    qos_map_list.push_back(oid);
  }
  smi::attr_w qos_map_list_attr(SWITCH_QOS_MAP_INGRESS_ATTR_QOS_MAP_LIST);
  qos_map_list_attr.v_set(qos_map_list);

  std::set<smi::attr_w> ingress_pcp_qosmap_attrs;
  bool global_pcp_enable = true;
  switch_enum_t qos_map_type = {.enumdata =
                                    SWITCH_QOS_MAP_INGRESS_ATTR_TYPE_PCP_TO_TC};
  sai_insert_device_attribute(
      0, SWITCH_QOS_MAP_INGRESS_ATTR_DEVICE, ingress_pcp_qosmap_attrs);

  ingress_pcp_qosmap_attrs.insert(
      smi::attr_w(SWITCH_QOS_MAP_INGRESS_ATTR_TYPE, qos_map_type));
  ingress_pcp_qosmap_attrs.insert(qos_map_list_attr);
  ingress_pcp_qosmap_attrs.insert(smi::attr_w(
      SWITCH_QOS_MAP_INGRESS_ATTR_GLOBAL_ENABLE, global_pcp_enable));

  switch_object_id_t pcp_qos_map_handle = {};
  switch_status = bf_switch_object_create(SWITCH_OBJECT_TYPE_QOS_MAP_INGRESS,
                                          ingress_pcp_qosmap_attrs,
                                          pcp_qos_map_handle);
  if (switch_status != SWITCH_STATUS_SUCCESS) {
    SAI_LOG_ERROR(
        "Failed to create QoS map Ingress: %s",
        sai_metadata_get_status_name(status_switch_to_sai(switch_status)));
    return switch_status;
  }

  switch_object_id_t device_handle = sai_get_device_id(dev_id);
  attr_w global_pcp_tc_qos_map_handle_attr(
      SWITCH_DEVICE_ATTR_GLOBAL_PCP_TC_HANDLE, pcp_qos_map_handle);

  switch_status =
      bf_switch_attribute_set(device_handle, global_pcp_tc_qos_map_handle_attr);
  if (switch_status != SWITCH_STATUS_SUCCESS) {
    syslog(LOG_ERR, "ERROR: failed to set global_pcp_tc handle attribute\n");
    return switch_status;
  }

  global_pcp_tc_qos_map_enabled = true;

  return switch_status;
}

/*
 *  Qos maps methods table retrieved with sai_api_query()
 */
sai_qos_map_api_t qos_map_api = {
    .create_qos_map = sai_create_qos_map,
    .remove_qos_map = sai_remove_qos_map,
    .set_qos_map_attribute = sai_set_qos_map_attribute,
    .get_qos_map_attribute = sai_get_qos_map_attribute,
};

sai_qos_map_api_t *sai_qos_map_api_get() { return &qos_map_api; }

sai_status_t sai_qos_map_initialize() {
  sai_insert_sw_sai_object_type_mapping(SWITCH_OBJECT_TYPE_QOS_MAP_INGRESS,
                                        SAI_OBJECT_TYPE_QOS_MAP);
  sai_insert_sw_sai_object_type_mapping(SWITCH_OBJECT_TYPE_QOS_MAP_EGRESS,
                                        SAI_OBJECT_TYPE_QOS_MAP);
  SAI_LOG_DEBUG("Initializing qos map");
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_QOS_MAP);
  return SAI_STATUS_SUCCESS;
}
