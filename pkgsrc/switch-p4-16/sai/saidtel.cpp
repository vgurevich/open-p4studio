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

static sai_api_t api_id = SAI_API_DTEL;
static switch_object_id_t device_handle = {0};

sai_status_t sai_create_dtel(_Out_ sai_object_id_t *dtel_id,
                             _In_ sai_object_id_t switch_id,
                             _In_ uint32_t attr_count,
                             _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  if (!dtel_id || (!attr_list && attr_count > 0)) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("DTel object create failed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *dtel_id = SAI_NULL_OBJECT_ID;

  switch_object_id_t object_id = {};
  std::set<smi::attr_w> sw_dtel_attrs;

  for (uint32_t index = 0; index < attr_count; index++) {
    switch (attr_list[index].id) {
      case SAI_DTEL_ATTR_INT_TRANSIT_ENABLE:  // Unsupported
        status = SAI_STATUS_NOT_SUPPORTED;
        break;
      case SAI_DTEL_ATTR_INT_ENDPOINT_ENABLE: {
        if (bf_switch_is_feature_enabled(SWITCH_FEATURE_DTEL_IFA_EDGE)) {
          sw_dtel_attrs.insert(
              smi::attr_w(SWITCH_DTEL_ATTR_IFA_REPORT,
                          static_cast<bool>(attr_list[index].value.booldata)));
        } else {
          status = SAI_STATUS_NOT_SUPPORTED;
        }
        break;
      }
      case SAI_DTEL_ATTR_INT_L4_DSCP: {
        if (bf_switch_is_feature_enabled(SWITCH_FEATURE_DTEL_IFA_EDGE)) {
          sw_dtel_attrs.insert(smi::attr_w(
              SWITCH_DTEL_ATTR_IFA_DSCP,
              (uint8_t)(attr_list[index].value.aclfield.data.u8 & 0x3F)));
          sw_dtel_attrs.insert(smi::attr_w(
              SWITCH_DTEL_ATTR_IFA_DSCP_MASK,
              (uint8_t)(attr_list[index].value.aclfield.mask.u8 & 0x3F)));
        }
        // else (unsupported case), ignore and return silently rather than
        // explicitly return NOT_SUPPORTED, in order to avoid SONiC crash
        break;
      }
      case SAI_DTEL_ATTR_FLOW_STATE_CLEAR_CYCLE: {
        uint16_t flow_state_clear_cycle = attr_list[index].value.u16;
        attr_w switch_attr(SWITCH_DEVICE_ATTR_FLOW_STATE_CLEAR_CYCLE,
                           flow_state_clear_cycle);
        switch_status = bf_switch_attribute_set(device_handle, switch_attr);
        break;
      }
      case SAI_DTEL_ATTR_SWITCH_ID: {
        uint32_t dtel_switch_id = attr_list[index].value.u32;
        attr_w switch_attr(SWITCH_DEVICE_ATTR_SWITCH_ID, dtel_switch_id);
        switch_status = bf_switch_attribute_set(device_handle, switch_attr);
        break;
      }
      case SAI_DTEL_ATTR_LATENCY_SENSITIVITY: {
        uint8_t value = attr_list[index].value.u8;
        attr_w switch_attr(SWITCH_DEVICE_ATTR_LATENCY_SENSITIVITY, value);
        switch_status = bf_switch_attribute_set(device_handle, switch_attr);
        break;
      }
      case SAI_DTEL_ATTR_SINK_PORT_LIST: {
        attr_w switch_attr(SWITCH_PORT_ATTR_DTEL_INT_EDGE, true);
        for (uint32_t i = 0; i < attr_list[index].value.objlist.count; i++) {
          switch_object_id_t port_oid = {
              .data = attr_list[index].value.objlist.list[i]};
          switch_status = bf_switch_attribute_set(port_oid, switch_attr);
          status = status_switch_to_sai(switch_status);
          if (status != SAI_STATUS_SUCCESS) {
            break;
          }
        }
        break;
      }
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_DTEL, &attr_list[index], sw_dtel_attrs);
        break;
    }
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR(
          "failed to create dtel: %s : sai_to_switch attr map failed, ",
          sai_metadata_get_status_name(status));
      return status;
    }
  }

  sai_insert_device_attribute(0, SWITCH_DTEL_ATTR_DEVICE, sw_dtel_attrs);
  switch_status = bf_switch_object_create(
      SWITCH_OBJECT_TYPE_DTEL, sw_dtel_attrs, object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS &&
      status != SAI_STATUS_ITEM_ALREADY_EXISTS) {
    SAI_LOG_ERROR("failed to create dtel: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *dtel_id = object_id.data;
  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

sai_status_t sai_remove_dtel(_In_ sai_object_id_t dtel_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  if (sai_object_type_query(dtel_id) != SAI_OBJECT_TYPE_DTEL) {
    SAI_LOG_ERROR("Invalid object 0x%" PRIx64, dtel_id);
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  const switch_object_id_t sw_object_id = {.data = dtel_id};

  switch_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to remove dtel handle %" PRIx64 ": %s",
                  dtel_id,
                  sai_metadata_get_status_name(status));
    return status;
  }

  // Clear all SWITCH_PORT_ATTR_DTEL_INT_EDGE
  uint32_t count = 0;
  switch_object_id_t first_handle = {0};
  std::vector<switch_object_id_t> handles;
  switch_status =
      bf_switch_get_first_handle(SWITCH_OBJECT_TYPE_PORT, first_handle);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR(
        "failed to retrieve first port handle to clear dtel_int_edge: %s",
        sai_metadata_get_status_name(status));
    return status;
  }
  switch_status = bf_switch_get_next_handles(
      first_handle, SWITCH_MAX_PORTS, handles, count);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to retrieve port handles to clear dtel_int_edge: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }
  handles.push_back(first_handle);
  for (auto it = handles.begin(); it != handles.end(); it++) {
    switch_object_id_t port_oid = *it;
    bool prev_edge_port;
    attr_w switch_attr(SWITCH_PORT_ATTR_DTEL_INT_EDGE);
    switch_status = bf_switch_attribute_get(
        port_oid, SWITCH_PORT_ATTR_DTEL_INT_EDGE, switch_attr);
    status = status_switch_to_sai(switch_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("failed to retrieve dtel_int_edge on port handle %" PRIx64
                    ": %s",
                    port_oid.data,
                    sai_metadata_get_status_name(status));
      return status;
    }
    switch_attr.v_get(prev_edge_port);
    if (prev_edge_port) {
      attr_w switch_attr_2(SWITCH_PORT_ATTR_DTEL_INT_EDGE, false);
      switch_status = bf_switch_attribute_set(port_oid, switch_attr_2);
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("failed to clear dtel_int_edge on port handle %" PRIx64
                      ": %s",
                      port_oid.data,
                      sai_metadata_get_status_name(status));
        return status;
      }
    }
  }

  SAI_LOG_EXIT();
  return (sai_status_t)status;
}

sai_status_t sai_get_dtel_attribute(_In_ sai_object_id_t dtel_id,
                                    _In_ uint32_t attr_count,
                                    _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(dtel_id) != SAI_OBJECT_TYPE_DTEL) {
    SAI_LOG_ERROR("Invalid object 0x%" PRIx64, dtel_id);
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  const switch_object_id_t sw_object_id = {.data = dtel_id};

  for (uint32_t i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      case SAI_DTEL_ATTR_INT_TRANSIT_ENABLE:  // Unsupported
        status = SAI_STATUS_NOT_SUPPORTED;
        break;
      case SAI_DTEL_ATTR_INT_ENDPOINT_ENABLE: {
        if (bf_switch_is_feature_enabled(SWITCH_FEATURE_DTEL_IFA_EDGE)) {
          bool ifa_report = false;
          attr_w switch_attr(SWITCH_DTEL_ATTR_IFA_REPORT);
          switch_status = bf_switch_attribute_get(
              sw_object_id, SWITCH_DTEL_ATTR_IFA_REPORT, switch_attr);
          switch_attr.v_get(ifa_report);
          attr_list[i].value.booldata = ifa_report;
          status = status_switch_to_sai(switch_status);
        } else {
          status = SAI_STATUS_NOT_SUPPORTED;
        }
        break;
      }
      case SAI_DTEL_ATTR_INT_L4_DSCP: {
        if (bf_switch_is_feature_enabled(SWITCH_FEATURE_DTEL_IFA_EDGE)) {
          uint8_t dscp = 0;
          attr_w switch_attr(SWITCH_DTEL_ATTR_IFA_DSCP);
          switch_status = bf_switch_attribute_get(
              sw_object_id, SWITCH_DTEL_ATTR_IFA_DSCP, switch_attr);
          switch_attr.v_get(dscp);
          attr_list[i].value.aclfield.data.u8 = dscp;
          status = status_switch_to_sai(switch_status);
          if (status != SAI_STATUS_SUCCESS) {
            break;
          }
          uint8_t dscp_mask = 0;
          attr_w switch_attr_2(SWITCH_DTEL_ATTR_IFA_DSCP_MASK);
          switch_status = bf_switch_attribute_get(
              sw_object_id, SWITCH_DTEL_ATTR_IFA_DSCP_MASK, switch_attr);
          switch_attr.v_get(dscp_mask);
          attr_list[i].value.aclfield.mask.u8 = dscp_mask;
          status = status_switch_to_sai(switch_status);
        } else {
          status = SAI_STATUS_NOT_SUPPORTED;
        }
      } break;
      case SAI_DTEL_ATTR_FLOW_STATE_CLEAR_CYCLE: {
        uint16_t flow_state_clear_cycle = 0;
        attr_w switch_attr(SWITCH_DEVICE_ATTR_FLOW_STATE_CLEAR_CYCLE);
        switch_status =
            bf_switch_attribute_get(device_handle,
                                    SWITCH_DEVICE_ATTR_FLOW_STATE_CLEAR_CYCLE,
                                    switch_attr);
        switch_attr.v_get(flow_state_clear_cycle);
        attr_list[i].value.u16 = flow_state_clear_cycle;
        status = status_switch_to_sai(switch_status);
      } break;
      case SAI_DTEL_ATTR_SWITCH_ID: {
        uint32_t switch_id = 0;
        attr_w switch_attr(SWITCH_DEVICE_ATTR_SWITCH_ID);
        switch_status = bf_switch_attribute_get(
            device_handle, SWITCH_DEVICE_ATTR_SWITCH_ID, switch_attr);
        switch_attr.v_get(switch_id);
        attr_list[i].value.u32 = switch_id;
        status = status_switch_to_sai(switch_status);
        break;
      }
      case SAI_DTEL_ATTR_LATENCY_SENSITIVITY: {
        uint32_t value = 0;
        attr_w switch_attr(SWITCH_DEVICE_ATTR_LATENCY_SENSITIVITY);
        switch_status = bf_switch_attribute_get(
            device_handle, SWITCH_DEVICE_ATTR_LATENCY_SENSITIVITY, switch_attr);
        switch_attr.v_get(value);
        attr_list[i].value.u8 = value;
        status = status_switch_to_sai(switch_status);
      } break;
      case SAI_DTEL_ATTR_SINK_PORT_LIST: {
        sai_object_list_t *objlist;
        uint32_t count = 0;
        uint32_t num_ports = 0;
        switch_object_id_t first_handle = {0};
        std::vector<switch_object_id_t> handles_list;
        attr_w attr(0);
        objlist = &attr_list[i].value.objlist;
        switch_status =
            bf_switch_get_first_handle(SWITCH_OBJECT_TYPE_PORT, first_handle);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "failed to retrieve first port handle to get sink port list: %s",
              sai_metadata_get_status_name(status));
          break;
        }
        switch_status = bf_switch_get_next_handles(
            first_handle, SWITCH_MAX_PORTS, handles_list, num_ports);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "failed to retrieve port handles to get sink port list: %s",
              sai_metadata_get_status_name(status));
          break;
        }
        handles_list.push_back(first_handle);
        for (const auto &handle : handles_list) {
          bool dtel_int_edge = false;
          attr_w switch_attr(SWITCH_PORT_ATTR_DTEL_INT_EDGE);
          switch_status = bf_switch_attribute_get(
              handle, SWITCH_PORT_ATTR_DTEL_INT_EDGE, switch_attr);
          status = status_switch_to_sai(switch_status);
          if (status != SAI_STATUS_SUCCESS) {
            SAI_LOG_ERROR(
                "failed to retrieve dtel_int_edge on port handle %" PRIx64
                ": %s",
                handle.data,
                sai_metadata_get_status_name(status));
            return status;
          }
          switch_attr.v_get(dtel_int_edge);
          if (!dtel_int_edge) continue;
          if (count == objlist->count) {
            objlist->count = ++count;
            return SAI_STATUS_BUFFER_OVERFLOW;
          }
          objlist->list[count++] = handle.data;
        }
        objlist->count = count;
        break;
      }
      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_DTEL, sw_object_id, &attr_list[i]);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to get dtel attribute object_id: 0x%" PRIx64
              "attribute %s "
              "error: %s",
              dtel_id,
              sai_attribute_name(SAI_OBJECT_TYPE_DTEL, attr_list[i].id),
              sai_metadata_get_status_name(status));
          return status;
        }
        break;
    }
  }

  SAI_LOG_EXIT();
  return status;
}

sai_status_t sai_set_dtel_attribute(_In_ sai_object_id_t dtel_id,
                                    _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute: %s", sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(dtel_id) != SAI_OBJECT_TYPE_DTEL) {
    SAI_LOG_ERROR("Invalid object 0x%" PRIx64, dtel_id);
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  const switch_object_id_t sw_object_id = {.data = dtel_id};

  switch (attr->id) {
    case SAI_DTEL_ATTR_INT_TRANSIT_ENABLE:  // Unsupported
      status = SAI_STATUS_NOT_SUPPORTED;
      break;
    case SAI_DTEL_ATTR_INT_ENDPOINT_ENABLE: {
      if (bf_switch_is_feature_enabled(SWITCH_FEATURE_DTEL_IFA_EDGE)) {
        bool ifa_report = attr->value.booldata;
        attr_w switch_attr(SWITCH_DTEL_ATTR_IFA_REPORT, ifa_report);
        switch_status = bf_switch_attribute_set(sw_object_id, switch_attr);
        status = status_switch_to_sai(switch_status);
      } else {
        status = SAI_STATUS_NOT_SUPPORTED;
      }
      break;
    }
    case SAI_DTEL_ATTR_FLOW_STATE_CLEAR_CYCLE: {
      uint16_t flow_state_clear_cycle = attr->value.u16;
      attr_w switch_attr(SWITCH_DEVICE_ATTR_FLOW_STATE_CLEAR_CYCLE,
                         flow_state_clear_cycle);
      switch_status = bf_switch_attribute_set(device_handle, switch_attr);
      status = status_switch_to_sai(switch_status);
      break;
    }
    case SAI_DTEL_ATTR_INT_L4_DSCP: {
      if (bf_switch_is_feature_enabled(SWITCH_FEATURE_DTEL_IFA_EDGE)) {
        uint8_t value = attr->value.aclfield.data.u8;
        attr_w switch_attr(SWITCH_DTEL_ATTR_IFA_DSCP, value);
        switch_status = bf_switch_attribute_set(sw_object_id, switch_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          break;
        }
        value = attr->value.aclfield.mask.u8;
        attr_w switch_attr_2(SWITCH_DTEL_ATTR_IFA_DSCP_MASK, value);
        switch_status = bf_switch_attribute_set(sw_object_id, switch_attr_2);
        status = status_switch_to_sai(switch_status);
      }
      // else (unsupported case), ignore and return silently rather than
      // explicitly return NOT_SUPPORTED, in order to avoid SONiC crash
      break;
    }
    case SAI_DTEL_ATTR_SWITCH_ID: {
      uint32_t switch_id = attr->value.u32;
      attr_w switch_attr(SWITCH_DEVICE_ATTR_SWITCH_ID, switch_id);
      switch_status = bf_switch_attribute_set(device_handle, switch_attr);
      status = status_switch_to_sai(switch_status);
      break;
    }
    case SAI_DTEL_ATTR_LATENCY_SENSITIVITY: {
      uint8_t value = attr->value.u8;
      attr_w switch_attr(SWITCH_DEVICE_ATTR_LATENCY_SENSITIVITY, value);
      switch_status = bf_switch_attribute_set(device_handle, switch_attr);
      status = status_switch_to_sai(switch_status);
      break;
    }
    case SAI_DTEL_ATTR_SINK_PORT_LIST: {
      bool edge_ports[SWITCH_MAX_PORTS] = {0};
      uint16_t dev_port = 0;
      uint32_t count = 0;
      switch_object_id_t first_handle = {0};
      std::vector<switch_object_id_t> handles;
      for (uint32_t i = 0; i < attr->value.objlist.count; i++) {
        switch_object_id_t port_oid = {.data = attr->value.objlist.list[i]};
        attr_w switch_attr(SWITCH_PORT_ATTR_DEV_PORT);
        switch_status = bf_switch_attribute_get(
            port_oid, SWITCH_PORT_ATTR_DEV_PORT, switch_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "failed to retrieve dev port while setting sink ports: %s",
              sai_metadata_get_status_name(status));
          break;
        }
        switch_attr.v_get(dev_port);
        edge_ports[dev_port] = true;
      }
      if (status != SAI_STATUS_SUCCESS) {
        break;
      }
      switch_status =
          bf_switch_get_first_handle(SWITCH_OBJECT_TYPE_PORT, first_handle);
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR(
            "failed to retrieve first port handle while setting sink ports: %s",
            sai_metadata_get_status_name(status));
        break;
      }
      switch_status = bf_switch_get_next_handles(
          first_handle, SWITCH_MAX_PORTS, handles, count);
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR(
            "failed to retrieve port handles while setting sink port list: %s",
            sai_metadata_get_status_name(status));
        break;
      }
      handles.push_back(first_handle);
      for (auto it = handles.begin(); it != handles.end(); it++) {
        switch_object_id_t port_oid = *it;
        bool prev_edge_port;
        attr_w switch_attr(SWITCH_PORT_ATTR_DEV_PORT);
        attr_w switch_attr_2(SWITCH_PORT_ATTR_DTEL_INT_EDGE);
        switch_status = bf_switch_attribute_get(
            port_oid, SWITCH_PORT_ATTR_DEV_PORT, switch_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "failed to retrieve dev port while setting sink port: %s",
              sai_metadata_get_status_name(status));
          break;
        }
        switch_attr.v_get(dev_port);
        switch_status = bf_switch_attribute_get(
            port_oid, SWITCH_PORT_ATTR_DTEL_INT_EDGE, switch_attr_2);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "failed to retrieve dtel_int_edge while setting sink port: %s",
              sai_metadata_get_status_name(status));
          break;
        }
        switch_attr_2.v_get(prev_edge_port);
        if (!prev_edge_port && edge_ports[dev_port]) {
          attr_w switch_attr_3(SWITCH_PORT_ATTR_DTEL_INT_EDGE, true);
          switch_status = bf_switch_attribute_set(port_oid, switch_attr_3);
          status = status_switch_to_sai(switch_status);
          if (status != SAI_STATUS_SUCCESS) {
            SAI_LOG_ERROR("failed to set sink port on port %" PRIx64 ": %s",
                          port_oid.data,
                          sai_metadata_get_status_name(status));
            break;
          }
        } else if (prev_edge_port && !edge_ports[dev_port]) {
          attr_w switch_attr_3(SWITCH_PORT_ATTR_DTEL_INT_EDGE, false);
          switch_status = bf_switch_attribute_set(port_oid, switch_attr_3);
          status = status_switch_to_sai(switch_status);
          if (status != SAI_STATUS_SUCCESS) {
            SAI_LOG_ERROR("failed to clear sink port on port %" PRIx64 ": %s",
                          port_oid.data,
                          sai_metadata_get_status_name(status));
            break;
          }
        }
      }
      break;
    }
    default:
      status =
          sai_to_switch_attribute_set(SAI_OBJECT_TYPE_DTEL, attr, sw_object_id);
      break;
  }

  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to set attribute %s error: %s",
                  sai_attribute_name(SAI_OBJECT_TYPE_DTEL, attr->id),
                  sai_metadata_get_status_name(status));
    return status;
  }

  SAI_LOG_EXIT();
  return status;
}

sai_status_t sai_create_dtel_queue_report(
    _Out_ sai_object_id_t *dtel_queue_report_id,
    _In_ sai_object_id_t switch_id,
    _In_ uint32_t attr_count,
    _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  if (!dtel_queue_report_id || (attr_count && !attr_list)) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter: %s", sai_metadata_get_status_name(status));
    return status;
  }

  *dtel_queue_report_id = SAI_NULL_OBJECT_ID;

  switch_object_id_t sw_queue_report_id = {};
  std::set<smi::attr_w> sw_attrs;

  sai_insert_device_attribute(0, SWITCH_QUEUE_REPORT_ATTR_DEVICE, sw_attrs);
  status = sai_to_switch_attribute_list(
      SAI_OBJECT_TYPE_DTEL_QUEUE_REPORT, attr_count, attr_list, sw_attrs);
  if (status != SWITCH_STATUS_SUCCESS) {
    return status;
  }

  switch_status = bf_switch_object_create(
      SWITCH_OBJECT_TYPE_QUEUE_REPORT, sw_attrs, sw_queue_report_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to create dtel queue report: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *dtel_queue_report_id = sw_queue_report_id.data;
  SAI_LOG_EXIT();
  return status;
}

sai_status_t sai_remove_dtel_queue_report(
    _In_ sai_object_id_t dtel_queue_report_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  if (sai_object_type_query(dtel_queue_report_id) !=
      SAI_OBJECT_TYPE_DTEL_QUEUE_REPORT) {
    SAI_LOG_ERROR("Invalid object 0x%" PRIx64, dtel_queue_report_id);
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  const switch_object_id_t sw_object_id = {.data = dtel_queue_report_id};

  switch_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to remove dtel queue report %" PRIx64 ": %s",
                  dtel_queue_report_id,
                  sai_metadata_get_status_name(status));
    return status;
  }

  SAI_LOG_EXIT();
  return status;
}

sai_status_t sai_get_dtel_queue_report_attribute(
    _In_ sai_object_id_t dtel_queue_report_id,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();
  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  SAI_ASSERT(sai_object_type_query(dtel_queue_report_id) ==
             SAI_OBJECT_TYPE_DTEL_QUEUE_REPORT);
  const switch_object_id_t object_id = {.data = dtel_queue_report_id};

  for (uint32_t i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_DTEL_QUEUE_REPORT, object_id, &attr_list[i]);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to get attribute %s for queue report id: %" PRIx64
              "error: %s",
              sai_attribute_name(SAI_OBJECT_TYPE_DTEL_QUEUE_REPORT,
                                 attr_list[i].id),
              dtel_queue_report_id,
              sai_metadata_get_status_name(status));
        }
        break;
    }
  }

  SAI_LOG_EXIT();
  return status;
}

sai_status_t sai_set_dtel_queue_report_attribute(
    _In_ sai_object_id_t dtel_queue_report_id,
    _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();
  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute: %s", sai_metadata_get_status_name(status));
    return status;
  }

  SAI_ASSERT(sai_object_type_query(dtel_queue_report_id) ==
             SAI_OBJECT_TYPE_DTEL_QUEUE_REPORT);

  const switch_object_id_t object_id = {.data = dtel_queue_report_id};
  status = sai_to_switch_attribute_set(
      SAI_OBJECT_TYPE_DTEL_QUEUE_REPORT, attr, object_id);
  if (status != SAI_STATUS_SUCCESS && status != SAI_STATUS_NOT_SUPPORTED) {
    SAI_LOG_ERROR(
        "Failed to set attribute %s error: %s",
        sai_attribute_name(SAI_OBJECT_TYPE_DTEL_QUEUE_REPORT, attr->id),
        sai_metadata_get_status_name(status));
    return status;
  }

  SAI_LOG_EXIT();
  return status;
}

sai_status_t sai_create_dtel_int_session(
    _Out_ sai_object_id_t *dtel_int_session_id,
    _In_ sai_object_id_t switch_id,
    _In_ uint32_t attr_count,
    _In_ const sai_attribute_t *attr_list) {
  sai_status_t status = SAI_STATUS_SUCCESS;

  SAI_LOG_ENTER();

  if (!dtel_int_session_id || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter: %s", sai_metadata_get_status_name(status));
    return status;
  }

  *dtel_int_session_id = SAI_NULL_OBJECT_ID;
  SAI_LOG_EXIT();
  return status;
}

sai_status_t sai_remove_dtel_int_session(
    _In_ sai_object_id_t dtel_int_session_id) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  SAI_LOG_ENTER();

  SAI_LOG_EXIT();
  return status;
}

sai_status_t sai_get_dtel_int_session_attribute(
    _In_ sai_object_id_t dtel_int_session_id,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  SAI_LOG_ENTER();

  SAI_LOG_EXIT();
  return status;
}

sai_status_t sai_set_dtel_int_session_attribute(
    _In_ sai_object_id_t dtel_int_session_id,
    _In_ const sai_attribute_t *attr) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  SAI_LOG_ENTER();

  SAI_LOG_EXIT();
  return status;
}

sai_status_t sai_create_dtel_report_session(
    _Out_ sai_object_id_t *dtel_report_session_id,
    _In_ sai_object_id_t switch_id,
    _In_ uint32_t attr_count,
    _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  if (!dtel_report_session_id || (attr_count && !attr_list)) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null port list: %s", sai_metadata_get_status_name(status));
    return status;
  }

  *dtel_report_session_id = SAI_NULL_OBJECT_ID;

  switch_object_id_t sw_report_session_id = {};
  std::set<smi::attr_w> sw_attrs;

  sai_insert_device_attribute(0, SWITCH_REPORT_SESSION_ATTR_DEVICE, sw_attrs);
  status = sai_to_switch_attribute_list(
      SAI_OBJECT_TYPE_DTEL_REPORT_SESSION, attr_count, attr_list, sw_attrs);
  if (status != SWITCH_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to create dtel report session: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  switch_status = bf_switch_object_create(
      SWITCH_OBJECT_TYPE_REPORT_SESSION, sw_attrs, sw_report_session_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to create dtel report session: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *dtel_report_session_id = sw_report_session_id.data;
  SAI_LOG_EXIT();
  return status;
}

sai_status_t sai_remove_dtel_report_session(
    _In_ sai_object_id_t dtel_report_session_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  if (sai_object_type_query(dtel_report_session_id) !=
      SAI_OBJECT_TYPE_DTEL_REPORT_SESSION) {
    SAI_LOG_ERROR("Invalid object 0x%" PRIx64, dtel_report_session_id);
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  const switch_object_id_t sw_object_id = {.data = dtel_report_session_id};

  switch_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to remove dtel report session handle %" PRIx64 ": %s",
                  dtel_report_session_id,
                  sai_metadata_get_status_name(status));
    return status;
  }

  SAI_LOG_EXIT();
  return status;
}

sai_status_t sai_get_dtel_report_session_attribute(
    _In_ sai_object_id_t dtel_report_session_id,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();
  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  SAI_ASSERT(sai_object_type_query(dtel_report_session_id) ==
             SAI_OBJECT_TYPE_DTEL_REPORT_SESSION);
  const switch_object_id_t object_id = {.data = dtel_report_session_id};

  for (uint32_t i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_DTEL_REPORT_SESSION, object_id, &attr_list[i]);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to get attribute %s for report session id: %" PRIx64
              "error: %s",
              sai_attribute_name(SAI_OBJECT_TYPE_DTEL_REPORT_SESSION,
                                 attr_list[i].id),
              dtel_report_session_id,
              sai_metadata_get_status_name(status));
        }
        break;
    }
  }

  SAI_LOG_EXIT();
  return status;
}

sai_status_t sai_set_dtel_report_session_attribute(
    _In_ sai_object_id_t dtel_report_session_id,
    _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();
  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute: %s", sai_metadata_get_status_name(status));
    return status;
  }

  SAI_ASSERT(sai_object_type_query(dtel_report_session_id) ==
             SAI_OBJECT_TYPE_DTEL_REPORT_SESSION);

  const switch_object_id_t object_id = {.data = dtel_report_session_id};
  status = sai_to_switch_attribute_set(
      SAI_OBJECT_TYPE_DTEL_REPORT_SESSION, attr, object_id);
  if (status != SAI_STATUS_SUCCESS && status != SAI_STATUS_NOT_SUPPORTED) {
    SAI_LOG_ERROR(
        "Failed to set attribute %s error: %s",
        sai_attribute_name(SAI_OBJECT_TYPE_DTEL_REPORT_SESSION, attr->id),
        sai_metadata_get_status_name(status));
    return status;
  }

  SAI_LOG_EXIT();
  return status;
}

//------------------------------------------------------------------------------
// SAI_DTEL_EVENT
//------------------------------------------------------------------------------

#if 0
static switch_dtel_event_type_t sai_dtel_event_to_switch(
    _In_ sai_dtel_event_type_t event_type) {
  switch (event_type) {
    case SAI_DTEL_EVENT_TYPE_FLOW_STATE:
      return SWITCH_DTEL_EVENT_TYPE_FLOW_STATE_CHANGE;
    case SAI_DTEL_EVENT_TYPE_FLOW_REPORT_ALL_PACKETS:
      return SWITCH_DTEL_EVENT_TYPE_FLOW_REPORT_ALL_PACKETS;
    case SAI_DTEL_EVENT_TYPE_FLOW_TCPFLAG:
      return SWITCH_DTEL_EVENT_TYPE_FLOW_TCPFLAG;
    case SAI_DTEL_EVENT_TYPE_QUEUE_REPORT_THRESHOLD_BREACH:
      return SWITCH_DTEL_EVENT_TYPE_Q_REPORT_THRESHOLD_BREACH;
    case SAI_DTEL_EVENT_TYPE_QUEUE_REPORT_TAIL_DROP:
      return SWITCH_DTEL_EVENT_TYPE_Q_REPORT_TAIL_DROP;
    case SAI_DTEL_EVENT_TYPE_DROP_REPORT:
      return SWITCH_DTEL_EVENT_TYPE_DROP_REPORT;
    default:
      return SWITCH_DTEL_EVENT_TYPE_MAX;
  }
}

typedef struct sai_dtel_events_info_ {
  switch_handle_t oid[SAI_DTEL_EVENT_TYPE_MAX];
  sai_uint8_t dscp[SAI_DTEL_EVENT_TYPE_MAX];
  switch_handle_t report_session;
} sai_dtel_events_info_t;

static sai_dtel_events_info_t dtel_events;

#endif

sai_status_t sai_dtel_event_init() {
  sai_status_t status = SAI_STATUS_SUCCESS;

#if 0
  SAI_MEMSET(&dtel_events, 0, sizeof(sai_dtel_events_info_t));
  for (int type = 0; type < SAI_DTEL_EVENT_TYPE_MAX; type++) {
    dtel_events.oid[type] = sai_id_to_oid(SWITCH_HANDLE_TYPE_DTEL_EVENT, type);
  }
#endif

  return status;
}

sai_status_t sai_create_dtel_event(_Out_ sai_object_id_t *dtel_event_id,
                                   _In_ sai_object_id_t switch_id,
                                   _In_ uint32_t attr_count,
                                   _In_ const sai_attribute_t *attr_list) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  SAI_LOG_ENTER();
  *dtel_event_id = SAI_NULL_OBJECT_ID;
  SAI_LOG_EXIT();
  return status;
}

sai_status_t sai_remove_dtel_event(_In_ sai_object_id_t dtel_event_id) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  SAI_LOG_ENTER();
  SAI_LOG_EXIT();
  return status;
}

sai_status_t sai_get_dtel_event_attribute(_In_ sai_object_id_t dtel_event_id,
                                          _In_ uint32_t attr_count,
                                          _Inout_ sai_attribute_t *attr_list) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  SAI_LOG_ENTER();
  SAI_LOG_EXIT();
  return status;
}

sai_status_t sai_set_dtel_event_attribute(_In_ sai_object_id_t dtel_event_id,
                                          _In_ const sai_attribute_t *attr) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  SAI_LOG_ENTER();
  SAI_LOG_EXIT();
  return (sai_status_t)status;
}

/*
 *  DTel methods table retrieved with sai_api_query()
 */
sai_dtel_api_t dtel_api = {
    .create_dtel = sai_create_dtel,
    .remove_dtel = sai_remove_dtel,
    .set_dtel_attribute = sai_set_dtel_attribute,
    .get_dtel_attribute = sai_get_dtel_attribute,

    .create_dtel_queue_report = sai_create_dtel_queue_report,
    .remove_dtel_queue_report = sai_remove_dtel_queue_report,
    .set_dtel_queue_report_attribute = sai_set_dtel_queue_report_attribute,
    .get_dtel_queue_report_attribute = sai_get_dtel_queue_report_attribute,

    .create_dtel_int_session = sai_create_dtel_int_session,
    .remove_dtel_int_session = sai_remove_dtel_int_session,
    .set_dtel_int_session_attribute = sai_set_dtel_int_session_attribute,
    .get_dtel_int_session_attribute = sai_get_dtel_int_session_attribute,

    .create_dtel_report_session = sai_create_dtel_report_session,
    .remove_dtel_report_session = sai_remove_dtel_report_session,
    .set_dtel_report_session_attribute = sai_set_dtel_report_session_attribute,
    .get_dtel_report_session_attribute = sai_get_dtel_report_session_attribute,

    .create_dtel_event = sai_create_dtel_event,
    .remove_dtel_event = sai_remove_dtel_event,
    .set_dtel_event_attribute = sai_set_dtel_event_attribute,
    .get_dtel_event_attribute = sai_get_dtel_event_attribute};

sai_dtel_api_t *sai_dtel_api_get() { return &dtel_api; }

static sai_status_t sai_dtel_capability_initialize() {
  sai_object_capability_map &sai_object_capability =
      sai_get_object_capability_map();
  bool is_dtel_enable =
      bf_switch_is_feature_enabled(SWITCH_FEATURE_DTEL_IFA_EDGE);

  if (is_dtel_enable) {
    // Default capability
    sai_generate_attr_capability(SAI_OBJECT_TYPE_DTEL);

    // Unsupported attribute
    sai_object_capability[SAI_OBJECT_TYPE_DTEL]
                         [SAI_DTEL_ATTR_INT_TRANSIT_ENABLE] = {
                             false, false, false};
  }

  return SAI_STATUS_SUCCESS;
}

sai_status_t sai_dtel_initialize() {
  SAI_LOG_DEBUG("Initializing DTel");
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_DTEL);
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_DTEL_QUEUE_REPORT);
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_DTEL_REPORT_SESSION);
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_DTEL_EVENT);
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_DTEL_INT_SESSION);
  device_handle = sai_get_device_id(0);
  sai_dtel_capability_initialize();

  return SAI_STATUS_SUCCESS;
}
