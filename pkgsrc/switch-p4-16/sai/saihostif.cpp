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

static sai_api_t api_id = SAI_API_HOSTIF;
static switch_object_id_t device_handle = {0};
static switch_object_id_t default_hostif_trap_group_id = {};
static switch_object_id_t default_hostif_trap_group_policer = {};
static switch_object_id_t cpu_port_handle = {0};
static std::vector<sai_hostif_user_defined_trap_type_t>
    supported_hostif_user_defined_trap_types{
        SAI_HOSTIF_USER_DEFINED_TRAP_TYPE_NEIGHBOR};

sai_status_t sai_get_hostif_user_defined_trap_enum_capabilities(
    sai_attr_id_t attr_id, sai_s32_list_t *enum_values_capability) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  uint32_t i = 0;

  if (!enum_values_capability) return SAI_STATUS_INVALID_PARAMETER;

  if (attr_id == SAI_HOSTIF_USER_DEFINED_TRAP_ATTR_TYPE) {
    if (enum_values_capability->count >=
        supported_hostif_user_defined_trap_types.size()) {
      for (const auto &type : supported_hostif_user_defined_trap_types) {
        enum_values_capability->list[i] = type;
        i++;
      }
    } else {
      enum_values_capability->count =
          supported_hostif_user_defined_trap_types.size();
      return SAI_STATUS_BUFFER_OVERFLOW;
    }
  } else {
    status = SAI_STATUS_NOT_SUPPORTED;
    return status;
  }
  enum_values_capability->count = i;
  return status;
}

sai_object_id_t sai_hostif_get_default() {
  return (sai_object_id_t)(default_hostif_trap_group_id.data);
}

#define SAI_HOSTIF_TRAP_OBJECT(_reason_code)                           \
  ((_reason_code & 0xFFFF) | (SWITCH_HANDLE_TYPE_HOSTIF_TRAP << 26)) & \
      0xFFFFFFFF

#define SAI_HOSTIF_TRAP_TYPE_ARP_SUPPRESS (SAI_HOSTIF_TRAP_TYPE_END)
#define SAI_HOSTIF_TRAP_TYPE_ICMP (SAI_HOSTIF_TRAP_TYPE_END + 1)
#define SAI_HOSTIF_TRAP_TYPE_ICMPV6 (SAI_HOSTIF_TRAP_TYPE_END + 2)
#define SAI_HOSTIF_TRAP_TYPE_ICCP (SAI_HOSTIF_TRAP_TYPE_END + 3)
#define SAI_HOSTIF_TRAP_TYPE_ND_SUPPRESS (SAI_HOSTIF_TRAP_TYPE_END + 4)
#define SAI_HOSTIF_TRAP_TYPE_ARP_SUPPRESSION \
  (SAI_HOSTIF_TRAP_TYPE_ROUTER_CUSTOM_RANGE_BASE + 1)
#define SAI_HOSTIF_TRAP_TYPE_ND_SUPPRESSION \
  (SAI_HOSTIF_TRAP_TYPE_ROUTER_CUSTOM_RANGE_BASE + 2)

/*
 * Routine Description:
 *    Create host interface
 *
 * Arguments:
 *    [out] hif_id - host interface id
 *    [in] switch_id Switch object id
 *    [in] attr_count - number of attributes
 *    [in] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_create_hostif(_Out_ sai_object_id_t *hif_id,
                               _In_ sai_object_id_t switch_id,
                               _In_ uint32_t attr_count,
                               _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  const switch_object_type_t ot = SWITCH_OBJECT_TYPE_HOSTIF;
  uint32_t index = 0;

  if (!hif_id || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameters: %s", sai_metadata_get_status_name(status));
    return status;
  }

  *hif_id = SAI_NULL_OBJECT_ID;

  switch_object_id_t switch_hostif_object_id = {};
  std::set<smi::attr_w> sw_attrs;

  for (index = 0; index < attr_count; index++) {
    const sai_attribute_t *attribute = &attr_list[index];
    switch (attribute->id) {
      case SAI_HOSTIF_ATTR_TYPE:
        if (attribute->value.s32 == SAI_HOSTIF_TYPE_FD) {
          SAI_LOG_ERROR(
              "Failed to create hostif - SAI_HOSTIF_TYPE_FD is not "
              "supported\n");
          return SAI_STATUS_NOT_SUPPORTED;
        }
      /* fall through */
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_HOSTIF, attribute, sw_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          return status;
        }
        break;
    }
  }

  /* set hostif default source mac address*/
  switch_mac_addr_t hostif_mac;
  attr_w mac_attr(SWITCH_DEVICE_ATTR_SRC_MAC);

  switch_status = bf_switch_attribute_get(
      device_handle, SWITCH_DEVICE_ATTR_SRC_MAC, mac_attr);
  mac_attr.v_get(hostif_mac);
  sw_attrs.insert(smi::attr_w(SWITCH_HOSTIF_ATTR_MAC, hostif_mac));

  sai_insert_device_attribute(0, SWITCH_HOSTIF_ATTR_DEVICE, sw_attrs);

  switch_status =
      bf_switch_object_create(ot, sw_attrs, switch_hostif_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to create hostif: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *hif_id = switch_hostif_object_id.data;
  SAI_LOG_EXIT();
  return (sai_status_t)status;
}

/*
 * Routine Description:
 *    Remove host interface
 *
 * Arguments:
 *    [in] hif_id - host interface id
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_remove_hostif(_In_ sai_object_id_t hif_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  SAI_ASSERT(sai_object_type_query(hif_id) == SAI_OBJECT_TYPE_HOSTIF);

  const switch_object_id_t sw_object_id = {.data = hif_id};

  switch_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(switch_status);

  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to remove hostif %" PRIx64 ": %s",
                  hif_id,
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();
  return status;
}

/*
 * Routine Description:
 *    Set host interface attribute
 *
 * Arguments:
 *    [in] hif_id - host interface id
 *    [in] attr - attribute
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_set_hostif_attribute(_In_ sai_object_id_t hif_id,
                                      _In_ const sai_attribute_t *sai_attr) {
  SAI_LOG_ENTER();
  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!sai_attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  SAI_ASSERT(sai_object_type_query(hif_id) == SAI_OBJECT_TYPE_HOSTIF);

  const switch_object_id_t sw_object_id = {.data = hif_id};

  switch (sai_attr->id) {
    case SAI_HOSTIF_ATTR_OPER_STATUS:
    case SAI_HOSTIF_ATTR_QUEUE:
    case SAI_HOSTIF_ATTR_VLAN_TAG:
      status = sai_to_switch_attribute_set(
          SAI_OBJECT_TYPE_HOSTIF, sai_attr, sw_object_id);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to set attribute %s error: %s\n",
                      sai_attribute_name(SAI_OBJECT_TYPE_HOSTIF, sai_attr->id),
                      sai_metadata_get_status_name(status));
        return status;
      }
      break;
    default:
      break;
  }

  SAI_LOG_EXIT();

  return SAI_STATUS_SUCCESS;
}

/*
 * Routine Description:
 *    Get host interface attribute
 *
 * Arguments:
 *    [in] hif_id - host interface id
 *    [in] attr_count - number of attributes
 *    [inout] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_get_hostif_attribute(_In_ sai_object_id_t hif_id,
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

  SAI_ASSERT(sai_object_type_query(hif_id) == SAI_OBJECT_TYPE_HOSTIF);

  const switch_object_id_t sw_object_id = {.data = hif_id};

  for (unsigned int i = 0; i < attr_count; i++) {
    sai_attribute_t *sai_attr = &attr_list[i];
    switch (sai_attr->id) {
      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_HOSTIF, sw_object_id, sai_attr);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to get attribute %s error: %s\n",
              sai_attribute_name(SAI_OBJECT_TYPE_HOSTIF, sai_attr->id),
              sai_metadata_get_status_name(status));
        }
        break;
    }
  }

  SAI_LOG_EXIT();

  return SAI_STATUS_SUCCESS;
}

sai_status_t sai_get_switch_queue_handle(uint32_t qid,
                                         switch_object_id_t &queue_handle) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  attr_w queue_list(SWITCH_PORT_ATTR_QUEUE_HANDLES);
  std::vector<switch_object_id_t> queue_handles;
  sai_status_t sai_status = SAI_STATUS_SUCCESS;
  uint8_t temp_qid;

  status = bf_switch_attribute_get(
      cpu_port_handle, SWITCH_PORT_ATTR_QUEUE_HANDLES, queue_list);
  if (status != SWITCH_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to retrieve queue handles");
    return SAI_STATUS_FAILURE;
  }

  queue_list.v_get(queue_handles);
  if (qid >= queue_handles.size()) {
    return SAI_STATUS_INVALID_PARAMETER;
  }

  sai_status = SAI_STATUS_ITEM_NOT_FOUND;
  for (auto &q_handle : queue_handles) {
    smi::attr_w qid_attr(SWITCH_QUEUE_ATTR_QUEUE_ID);
    status =
        bf_switch_attribute_get(q_handle, SWITCH_QUEUE_ATTR_QUEUE_ID, qid_attr);
    qid_attr.v_get(temp_qid);
    if (temp_qid == qid) {
      queue_handle = q_handle;
      sai_status = SAI_STATUS_SUCCESS;
    }
  }

  return sai_status;
}

/*
 * Routine Description:
 *    Create host interface trap group
 *
 * Arguments:
 *  [out] hostif_trap_group_id  - host interface trap group id
 *  [in] switch_id Switch object id
 *  [in] attr_count - number of attributes
 *  [in] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_create_hostif_trap_group(
    _Out_ sai_object_id_t *hostif_trap_group_id,
    _In_ sai_object_id_t switch_id,
    _In_ uint32_t attr_count,
    _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  const sai_attribute_t *attribute;
  const switch_object_type_t ot = SWITCH_OBJECT_TYPE_HOSTIF_TRAP_GROUP;
  uint32_t index = 0;
  uint32_t qid = 0;
  switch_object_id_t queue_handle = {};

  if (!hostif_trap_group_id || (attr_count && !attr_list)) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter: %s", sai_metadata_get_status_name(status));
    return status;
  }

  *hostif_trap_group_id = SAI_NULL_OBJECT_ID;

  switch_object_id_t hostif_group_object_id = {};
  std::set<smi::attr_w> sw_attrs;
  bool policer_attr_exists = false;

  for (index = 0; index < attr_count; index++) {
    attribute = &attr_list[index];
    switch (attribute->id) {
      case SAI_HOSTIF_TRAP_GROUP_ATTR_QUEUE: {
        qid = attribute->value.u32;
        break;
      }
      case SAI_HOSTIF_TRAP_GROUP_ATTR_POLICER:
        policer_attr_exists = true;
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_HOSTIF_TRAP_GROUP, attribute, sw_attrs);
        break;
      case SAI_HOSTIF_TRAP_GROUP_ATTR_ADMIN_STATE:  // Unsupported
      default:
        break;
    }
  }

  status = sai_get_switch_queue_handle(qid, queue_handle);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get queue %u handle, error: %s\n",
                  qid,
                  sai_metadata_get_status_name(status));
    return status;
  }

  attr_w q_attr(SWITCH_HOSTIF_TRAP_GROUP_ATTR_QUEUE_HANDLE, queue_handle);
  sw_attrs.insert(q_attr);

  if (!policer_attr_exists) {
    sw_attrs.insert(attr_w(SWITCH_HOSTIF_TRAP_GROUP_ATTR_POLICER_HANDLE,
                           default_hostif_trap_group_policer));
  }

  sai_insert_device_attribute(
      0, SWITCH_HOSTIF_TRAP_GROUP_ATTR_DEVICE, sw_attrs);
  switch_status = bf_switch_object_create(ot, sw_attrs, hostif_group_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to create hostif trap group: %s",
                  sai_metadata_get_status_name(status));
  }
  *hostif_trap_group_id = hostif_group_object_id.data;
  SAI_LOG_EXIT();

  return SAI_STATUS_SUCCESS;
}

/*
 * Routine Description:
 *    Remove host interface trap group
 *
 * Arguments:
 *  [in] hostif_trap_group_id - host interface trap group id
 *
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_remove_hostif_trap_group(
    _In_ sai_object_id_t hostif_trap_group_id) {
  SAI_LOG_ENTER();

  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  SAI_ASSERT(sai_object_type_query(hostif_trap_group_id) ==
             SAI_OBJECT_TYPE_HOSTIF_TRAP_GROUP);

  const switch_object_id_t sw_object_id = {.data = hostif_trap_group_id};

  switch_status = bf_switch_object_delete(sw_object_id);

  SAI_LOG_EXIT();
  return status_switch_to_sai(switch_status);
}

/*
 * Routine Description:
 *   Set host interface trap group attribute value.
 *
 * Arguments:
 *    [in] hostif_trap_group_id - host interface trap group id
 *    [in] attr - attribute
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_set_hostif_trap_group_attribute(
    _In_ sai_object_id_t hostif_trap_group_id,
    _In_ const sai_attribute_t *sai_attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  if (!sai_attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute: %s", sai_metadata_get_status_name(status));
    return status;
  }
  SAI_ASSERT(sai_object_type_query(hostif_trap_group_id) ==
             SAI_OBJECT_TYPE_HOSTIF_TRAP_GROUP);

  const switch_object_id_t sw_object_id = {.data = hostif_trap_group_id};

  switch (sai_attr->id) {
    case SAI_HOSTIF_TRAP_GROUP_ATTR_POLICER:
      status = sai_to_switch_attribute_set(
          SAI_OBJECT_TYPE_HOSTIF_TRAP_GROUP, sai_attr, sw_object_id);
      break;
    case SAI_HOSTIF_TRAP_GROUP_ATTR_QUEUE: {
      switch_object_id_t queue_handle = {};
      status = sai_get_switch_queue_handle(sai_attr->value.u32, queue_handle);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to get queue %u handle, error: %s\n",
                      sai_attr->value.u32,
                      sai_metadata_get_status_name(status));
        return status;
      }

      attr_w sw_attr(SWITCH_HOSTIF_TRAP_GROUP_ATTR_QUEUE_HANDLE);
      sw_attr.v_set(queue_handle);
      switch_status = bf_switch_attribute_set(sw_object_id, sw_attr);
      status = status_switch_to_sai(switch_status);
      break;
    }
    default:
      break;
  }
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR(
        "Failed to set attribute %s error: %s\n",
        sai_attribute_name(SAI_OBJECT_TYPE_HOSTIF_TRAP_GROUP, sai_attr->id),
        sai_metadata_get_status_name(status));
    return status;
  }
  SAI_LOG_EXIT();

  return SAI_STATUS_SUCCESS;
}

/*
 * Routine Description:
 *   get host interface trap group attribute value.
 *
 * Arguments:
 *    [in] hostif_trap_group_id - host interface trap group id
 *    [in] attr_count - number of attributes
 *    [in,out] attr_list - array of attributes
 *
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_get_hostif_trap_group_attribute(
    _In_ sai_object_id_t hostif_trap_group_id,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute: %s", sai_metadata_get_status_name(status));
    return status;
  }

  SAI_ASSERT(sai_object_type_query(hostif_trap_group_id) ==
             SAI_OBJECT_TYPE_HOSTIF_TRAP_GROUP);

  const switch_object_id_t sw_object_id = {.data = hostif_trap_group_id};

  for (unsigned int i = 0; i < attr_count; i++) {
    sai_attribute_t *sai_attr = &attr_list[i];
    switch (sai_attr->id) {
      /* added supported list here. */
      case SAI_HOSTIF_TRAP_GROUP_ATTR_QUEUE: {
        smi::attr_w sw_attr(SWITCH_HOSTIF_TRAP_GROUP_ATTR_QUEUE_HANDLE);
        switch_status = bf_switch_attribute_get(
            sw_object_id, SWITCH_HOSTIF_TRAP_GROUP_ATTR_QUEUE_HANDLE, sw_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to fetch switch attribute for SAI: %s, error: %s",
              sai_attribute_name(SAI_OBJECT_TYPE_HOSTIF_TRAP_GROUP,
                                 sai_attr->id),
              sai_metadata_get_status_name(status));
          return status;
        }

        switch_object_id_t q_handle = {};
        sw_attr.v_get(q_handle);

        uint8_t qid;
        smi::attr_w qid_attr(SWITCH_QUEUE_ATTR_QUEUE_ID);
        switch_status = bf_switch_attribute_get(
            q_handle, SWITCH_QUEUE_ATTR_QUEUE_ID, qid_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Switch Queue handle to Id failed, "
              "Failed to fetch switch attribute for SAI: %s, error: %s",
              sai_attribute_name(SAI_OBJECT_TYPE_HOSTIF_TRAP_GROUP,
                                 sai_attr->id),
              sai_metadata_get_status_name(status));
          return status;
        }
        qid_attr.v_get(qid);
        sai_attr->value.u32 = static_cast<uint32_t>(qid);
        break;
      }

      case SAI_HOSTIF_TRAP_GROUP_ATTR_POLICER:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_HOSTIF_TRAP_GROUP, sw_object_id, sai_attr);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to get attribute %s error: %s\n",
                        sai_attribute_name(SAI_OBJECT_TYPE_HOSTIF_TRAP_GROUP,
                                           sai_attr->id),
                        sai_metadata_get_status_name(status));
        }
        break;
      default:
        break;
    }
  }

  SAI_LOG_EXIT();

  return SAI_STATUS_SUCCESS;
}

uint32_t sai_hostif_to_switch_trap_type(sai_hostif_trap_type_t sai_trap_type) {
  uint32_t trap_type;
  switch (static_cast<int32_t>(sai_trap_type)) {
    case SAI_HOSTIF_TRAP_TYPE_STP:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_STP;
      break;
    case SAI_HOSTIF_TRAP_TYPE_LACP:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_LACP;
      break;
    case SAI_HOSTIF_TRAP_TYPE_LLDP:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_LLDP;
      break;
    case SAI_HOSTIF_TRAP_TYPE_IGMP_TYPE_QUERY:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_IGMP_TYPE_QUERY;
      break;
    case SAI_HOSTIF_TRAP_TYPE_IGMP_TYPE_LEAVE:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_IGMP_TYPE_LEAVE;
      break;
    case SAI_HOSTIF_TRAP_TYPE_IGMP_TYPE_V1_REPORT:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_IGMP_TYPE_V1_REPORT;
      break;
    case SAI_HOSTIF_TRAP_TYPE_IGMP_TYPE_V2_REPORT:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_IGMP_TYPE_V2_REPORT;
      break;
    case SAI_HOSTIF_TRAP_TYPE_IGMP_TYPE_V3_REPORT:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_IGMP_TYPE_V3_REPORT;
      break;
    case SAI_HOSTIF_TRAP_TYPE_IPV6_MLD_V1_V2:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_IPV6_MLD_V1_V2;
      break;
    case SAI_HOSTIF_TRAP_TYPE_IPV6_MLD_V1_REPORT:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_IPV6_MLD_V1_REPORT;
      break;
    case SAI_HOSTIF_TRAP_TYPE_IPV6_MLD_V1_DONE:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_IPV6_MLD_V1_DONE;
      break;
    case SAI_HOSTIF_TRAP_TYPE_MLD_V2_REPORT:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_MLD_V2_REPORT;
      break;
    case SAI_HOSTIF_TRAP_TYPE_SAMPLEPACKET:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_SAMPLEPACKET;
      break;
    case SAI_HOSTIF_TRAP_TYPE_ARP_REQUEST:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_ARP_REQUEST;
      break;
    case SAI_HOSTIF_TRAP_TYPE_ARP_RESPONSE:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_ARP_RESPONSE;
      break;
    case SAI_HOSTIF_TRAP_TYPE_DHCP:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_DHCP;
      break;
    case SAI_HOSTIF_TRAP_TYPE_DHCP_L2:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_DHCP_L2;
      break;
    case SAI_HOSTIF_TRAP_TYPE_OSPF:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_OSPF;
      break;
    case SAI_HOSTIF_TRAP_TYPE_PIM:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_PIM;
      break;
    case SAI_HOSTIF_TRAP_TYPE_VRRP:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_VRRP;
      break;
    case SAI_HOSTIF_TRAP_TYPE_VRRPV6:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_VRRPV6;
      break;
    case SAI_HOSTIF_TRAP_TYPE_DHCPV6:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_DHCPV6;
      break;
    case SAI_HOSTIF_TRAP_TYPE_DHCPV6_L2:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_DHCPV6_L2;
      break;
    case SAI_HOSTIF_TRAP_TYPE_OSPFV6:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_OSPFV6;
      break;
    case SAI_HOSTIF_TRAP_TYPE_IPV6_NEIGHBOR_DISCOVERY:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_IPV6_NEIGHBOR_DISCOVERY;
      break;
    case SAI_HOSTIF_TRAP_TYPE_IP2ME:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_MYIP;
      break;
#ifdef SAI_TRAP_TYPE_IP2ME_SUBNET  // customer patch
    case SAI_HOSTIF_TRAP_TYPE_IP2ME_SUBNET:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_MYIP_SUBNET;
      break;
#endif
    case SAI_HOSTIF_TRAP_TYPE_SSH:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_SSH;
      break;
    case SAI_HOSTIF_TRAP_TYPE_SNMP:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_SNMP;
      break;
    case SAI_HOSTIF_TRAP_TYPE_BGP:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_BGP;
      break;
    case SAI_HOSTIF_TRAP_TYPE_BGPV6:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_BGPV6;
      break;
    case SAI_HOSTIF_TRAP_TYPE_L3_MTU_ERROR:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_L3_MTU_ERROR;
      break;
    case SAI_HOSTIF_TRAP_TYPE_TTL_ERROR:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_TTL_ERROR;
      break;
    case SAI_HOSTIF_TRAP_TYPE_UDLD:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_UDLD;
      break;
    case SAI_HOSTIF_TRAP_TYPE_DNAT_MISS:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_DNAT_MISS;
      break;
    case SAI_HOSTIF_TRAP_TYPE_SNAT_MISS:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_SNAT_MISS;
      break;
    case SAI_HOSTIF_TRAP_TYPE_PTP:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_PTP;
      break;
    case SAI_HOSTIF_TRAP_TYPE_BFD:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_BFD;
      break;
    case SAI_HOSTIF_TRAP_TYPE_BFDV6:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_BFDV6;
      break;
    case SAI_HOSTIF_TRAP_TYPE_MPLS_ROUTER_ALERT_LABEL:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_MPLS_ROUTER_ALERT;
      break;
    case SAI_HOSTIF_TRAP_TYPE_MPLS_TTL_ERROR:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_MPLS_TTL_ERROR;
      break;
    case SAI_HOSTIF_TRAP_TYPE_ISIS:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_ISIS;
      break;
    case SAI_HOSTIF_TRAP_TYPE_EAPOL:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_EAPOL;
      break;
    case SAI_HOSTIF_TRAP_TYPE_PVRST:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_PVRST;
      break;
    case SAI_HOSTIF_TRAP_TYPE_ICMP:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_ICMP;
      break;
    case SAI_HOSTIF_TRAP_TYPE_ARP_SUPPRESS:
    case SAI_HOSTIF_TRAP_TYPE_ARP_SUPPRESSION:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_ARP_SUPPRESS;
      break;
    case SAI_HOSTIF_TRAP_TYPE_ND_SUPPRESS:
    case SAI_HOSTIF_TRAP_TYPE_ND_SUPPRESSION:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_ND_SUPPRESS;
      break;
    case SAI_HOSTIF_TRAP_TYPE_ICMPV6:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_ICMPV6;
      break;
    case SAI_HOSTIF_TRAP_TYPE_ICCP:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_ICCP;
      break;
    case SAI_HOSTIF_TRAP_TYPE_GNMI:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_GNMI;
      break;
    case SAI_HOSTIF_TRAP_TYPE_P4RT:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_P4RT;
      break;
    case SAI_HOSTIF_TRAP_TYPE_NTPCLIENT:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_NTPCLIENT;
      break;
    case SAI_HOSTIF_TRAP_TYPE_NTPSERVER:
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_NTPSERVER;
      break;
    default:
      SAI_LOG_ERROR("Unknown trap type %d\n", sai_trap_type);
      trap_type = SWITCH_HOSTIF_TRAP_ATTR_TYPE_NONE;
  }
  return trap_type;
}

/*
 * Routine Description:
 *    Create host interface trap
 *
 * Arguments:
 *  [in] hostif_trap_id - host interface trap id
 *  [in] switch_id Switch object id
 *  [in] attr_count - number of attributes
 *  [in] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_create_hostif_trap(_In_ sai_object_id_t *hostif_trapid,
                                    _In_ sai_object_id_t switch_id,
                                    _In_ uint32_t attr_count,
                                    _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  const sai_attribute_t *attribute = NULL;
  const switch_object_type_t ot = SWITCH_OBJECT_TYPE_HOSTIF_TRAP;
  uint32_t index = 0;

  if (!hostif_trapid || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter: %s", sai_metadata_get_status_name(status));
    return status;
  }

  *hostif_trapid = SAI_NULL_OBJECT_ID;

  switch_object_id_t switch_hostif_trap_object_id = {};
  std::set<smi::attr_w> sw_attrs;
  bool is_trap_priority_set = false;
  bool is_trap_group_set = false;

  for (index = 0; index < attr_count; index++) {
    attribute = &attr_list[index];
    switch (attribute->id) {
      case SAI_HOSTIF_TRAP_ATTR_TRAP_PRIORITY: {
        const uint32_t switch_api_trap_priority =
            sai_hostif_priority_to_switch_hostif_priority(attribute->value.u32);
        sw_attrs.insert(
            attr_w(SWITCH_HOSTIF_TRAP_ATTR_PRIORITY, switch_api_trap_priority));
        is_trap_priority_set = true;
        break;
      }
      case SAI_HOSTIF_TRAP_ATTR_TRAP_TYPE: {
        const uint32_t trap_type = sai_hostif_to_switch_trap_type(
            (sai_hostif_trap_type_t)attribute->value.s32);
        switch_enum_t e = {.enumdata = trap_type};
        sw_attrs.insert(attr_w(SWITCH_HOSTIF_TRAP_ATTR_TYPE, e));
        break;
      }
      case SAI_HOSTIF_TRAP_ATTR_TRAP_GROUP:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_HOSTIF_TRAP, attribute, sw_attrs);
        is_trap_group_set = true;
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to set attribute %s error: %s\n",
              sai_attribute_name(SAI_OBJECT_TYPE_HOSTIF_TRAP, attribute->id),
              sai_metadata_get_status_name(status));
          is_trap_group_set = false;
        }
        break;
      case SAI_HOSTIF_TRAP_ATTR_PACKET_ACTION:
      case SAI_HOSTIF_TRAP_ATTR_EXCLUDE_PORT_LIST:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_HOSTIF_TRAP, attribute, sw_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to set attribute %s error: %s\n",
              sai_attribute_name(SAI_OBJECT_TYPE_HOSTIF_TRAP, attribute->id),
              sai_metadata_get_status_name(status));
          return status;
        }
        break;
      default:
        break;
    }
  }

  if (!is_trap_priority_set) {
    const uint32_t switch_api_trap_priority =
        sai_hostif_priority_to_switch_hostif_priority(0);
    sw_attrs.insert(
        attr_w(SWITCH_HOSTIF_TRAP_ATTR_PRIORITY, switch_api_trap_priority));
    is_trap_priority_set = true;
  }

  if (!is_trap_group_set) {
    const switch_object_id_t hostif_trap_group_handle =
        default_hostif_trap_group_id;

    sw_attrs.insert(attr_w(SWITCH_HOSTIF_TRAP_ATTR_HOSTIF_TRAP_GROUP_HANDLE,
                           hostif_trap_group_handle));
    is_trap_group_set = true;
  }

  sai_insert_device_attribute(0, SWITCH_HOSTIF_TRAP_ATTR_DEVICE, sw_attrs);
  switch_status =
      bf_switch_object_create(ot, sw_attrs, switch_hostif_trap_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS &&
      status != SAI_STATUS_ITEM_ALREADY_EXISTS) {
    SAI_LOG_ERROR("failed to create hostif trap: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *hostif_trapid = switch_hostif_trap_object_id.data;
  SAI_LOG_EXIT();

  return SAI_STATUS_SUCCESS;
}

/*
 * Routine Description:
 *    Remove host interface trap
 *
 * Arguments:
 *  [in] hostif_trap_id - host interface trap id
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_remove_hostif_trap(_In_ sai_object_id_t hostif_trapid) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  const switch_object_id_t sw_object_id = {.data = hostif_trapid};

  switch_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to remove hostif trap %" PRIx64 ": %s",
                  hostif_trapid,
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return status;
}

/*
 * Routine Description:
 *   Set trap attribute value.
 *
 * Arguments:
 *    [in] hostif_trap_id - host interface trap id
 *    [in] attr - attribute
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_set_hostif_trap_attribute(
    _In_ sai_object_id_t hostif_trapid, _In_ const sai_attribute_t *sai_attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  if (!sai_attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter: %s", sai_metadata_get_status_name(status));
    return status;
  }

  SAI_ASSERT(sai_object_type_query(hostif_trapid) ==
             SAI_OBJECT_TYPE_HOSTIF_TRAP);

  const switch_object_id_t sw_object_id = {.data = hostif_trapid};

  switch (sai_attr->id) {
    case SAI_HOSTIF_TRAP_ATTR_TRAP_PRIORITY: {
      attr_w sw_attr(SWITCH_HOSTIF_TRAP_ATTR_PRIORITY);
      sw_attr.v_set(
          sai_hostif_priority_to_switch_hostif_priority(sai_attr->value.u32));
      switch_status = bf_switch_attribute_set(sw_object_id, sw_attr);
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("failed to update hostif trap %" PRIx64 ": %s",
                      hostif_trapid,
                      sai_metadata_get_status_name(status));
        return status;
      }
    } break;
    case SAI_HOSTIF_TRAP_ATTR_PACKET_ACTION:
    case SAI_HOSTIF_TRAP_ATTR_TRAP_GROUP:
      status = sai_to_switch_attribute_set(
          SAI_OBJECT_TYPE_HOSTIF_TRAP, sai_attr, sw_object_id);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR(
            "Failed to set attribute %s error: %s\n",
            sai_attribute_name(SAI_OBJECT_TYPE_HOSTIF_TRAP, sai_attr->id),
            sai_metadata_get_status_name(status));
        return status;
      }
      break;
    default:
      break;
  }

  SAI_LOG_EXIT();

  return status;
}

/*
 * Routine Description:
 *   Get trap attribute value.
 *
 * Arguments:
 *    [in] hostif_trap_id - host interface trap id
 *    [in] attr_count - number of attributes
 *    [in,out] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_get_hostif_trap_attribute(_In_ sai_object_id_t hostif_trapid,
                                           _In_ uint32_t attr_count,
                                           _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute list: %s",
                  sai_metadata_get_status_name(status));
    SAI_LOG_EXIT();
    return status;
  }

  const switch_object_id_t sw_object_id = {.data = hostif_trapid};
  for (size_t i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      case SAI_HOSTIF_TRAP_ATTR_TRAP_PRIORITY: {
        smi::attr_w sw_attr(SWITCH_HOSTIF_TRAP_ATTR_PRIORITY);
        switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
        switch_status = bf_switch_attribute_get(
            sw_object_id, SWITCH_HOSTIF_TRAP_ATTR_PRIORITY, sw_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to fetch switch attribute for SAI: %s, error: %s",
              sai_attribute_name(SAI_OBJECT_TYPE_HOSTIF_TRAP,
                                 SAI_HOSTIF_TRAP_ATTR_TRAP_PRIORITY),
              sai_metadata_get_status_name(status));
          SAI_LOG_EXIT();
          return status;
        }
        uint32_t priority;
        sw_attr.v_get(priority);
        attr_list[i].value.u32 =
            switch_hostif_priority_to_sai_hostif_priority(priority);
      } break;
      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_HOSTIF_TRAP, sw_object_id, &attr_list[i]);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to exec sai_to_switch_attribute_get"
              ", hostif_trapid: 0x%" PRIx64
              ", attr_id: %d"
              ", sai_status: %d",
              hostif_trapid,
              attr_list[i].id,
              status);

          SAI_LOG_EXIT();
          return status;
        }
        break;
    }
  }

  SAI_LOG_EXIT();
  return SAI_STATUS_SUCCESS;
}

/*
 * Routine Description:
 *   Set user defined trap attribute value.
 *
 * Arguments:
 *    [in] hostif_user_defined_trap_id - host interface user defined trap id
 *    [in] attr - attribute
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_set_hostif_user_defined_trap_attribute(
    _In_ sai_object_id_t hostif_user_defined_trap_id,
    _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  const sai_object_type_t sai_ot = SAI_OBJECT_TYPE_HOSTIF_USER_DEFINED_TRAP;
  switch_object_id_t sw_trap_id = {.data = hostif_user_defined_trap_id};

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute: %s", sai_metadata_get_status_name(status));
    return status;
  }

  switch (attr->id) {
    default:
      status = sai_to_switch_attribute_set(sai_ot, attr, sw_trap_id);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("failed to set hostif user defined trap %" PRIx64
                      " attribute '%s': %s",
                      hostif_user_defined_trap_id,
                      sai_attribute_name(sai_ot, attr->id),
                      sai_metadata_get_status_name(status));
        return status;
      }
      break;
  }

  SAI_LOG_EXIT();

  return SAI_STATUS_SUCCESS;
}

/*
 * Routine Description:
 *   Get user defined trap attribute value.
 *
 * Arguments:
 *    [in] hostif_user_defined_trap_id - host interface user defined trap id
 *    [in] attr_count - number of attributes
 *    [in,out] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_get_hostif_user_defined_trap_attribute(
    _In_ sai_object_id_t hostif_user_defined_trap_id,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  const sai_object_type_t sai_ot = SAI_OBJECT_TYPE_HOSTIF_USER_DEFINED_TRAP;
  switch_object_id_t sw_trap_id = {.data = hostif_user_defined_trap_id};

  if (attr_count && !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  for (uint32_t i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      default:
        status = sai_to_switch_attribute_get(sai_ot, sw_trap_id, &attr_list[i]);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to set hostif user defined trap %" PRIx64
                        " attribute '%s': %s",
                        hostif_user_defined_trap_id,
                        sai_attribute_name(sai_ot, attr_list[i].id),
                        sai_metadata_get_status_name(status));
          return status;
        }
        break;
    }
  }

  SAI_LOG_EXIT();

  return SAI_STATUS_SUCCESS;
}

/**
 * Routine Description:
 *   Create host interface user defined trap
 *
 * [out] hostif_user_defined_trap_id Host interface user defined trap id
 * [in] switch_id Switch object id
 * [in] attr_count Number of attributes
 * [in] attr_list Array of attributes
 *
 * Return Values:
 *   SAI_STATUS_SUCCESS on success
 *   Failure status code on error
 */
sai_status_t sai_create_hostif_user_defined_trap(
    _Out_ sai_object_id_t *hostif_user_defined_trap_id,
    _In_ sai_object_id_t switch_id,
    _In_ uint32_t attr_count,
    _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();
  sai_status_t status;
  std::set<smi::attr_w> sw_attrs;
  switch_object_id_t sw_trap_id;
  bool is_trap_group_set = false;

  if (!hostif_user_defined_trap_id) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null hostif_user_defined_trap_id out parameter: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (attr_count && !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  for (uint32_t i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_HOSTIF_USER_DEFINED_TRAP_ATTR_TRAP_GROUP: {
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_HOSTIF_USER_DEFINED_TRAP, &attr_list[i], sw_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "failed to create hostif_user_defined_trap, attribute "
              "translation "
              "failed: %s",
              sai_metadata_get_status_name(status));
          break;
        }
        is_trap_group_set = true;
      } break;
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_HOSTIF_USER_DEFINED_TRAP, &attr_list[i], sw_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "failed to create hostif_user_defined_trap, attribute "
              "translation "
              "failed: %s",
              sai_metadata_get_status_name(status));
          return status;
        }
        break;
    }
  }

  sai_insert_device_attribute(
      0, SWITCH_HOSTIF_USER_DEFINED_TRAP_ATTR_DEVICE, sw_attrs);

  if (!is_trap_group_set) {
    const switch_object_id_t hostif_trap_group_handle =
        default_hostif_trap_group_id;

    sw_attrs.insert(
        attr_w(SWITCH_HOSTIF_USER_DEFINED_TRAP_ATTR_HOSTIF_TRAP_GROUP_HANDLE,
               hostif_trap_group_handle));
  }

  status = status_switch_to_sai(bf_switch_object_create(
      SWITCH_OBJECT_TYPE_HOSTIF_USER_DEFINED_TRAP, sw_attrs, sw_trap_id));
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to create hostif_user_defined_trap: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *hostif_user_defined_trap_id = (sai_object_id_t)sw_trap_id.data;

  SAI_LOG_EXIT();

  return SAI_STATUS_SUCCESS;
}

/**
 * Routine Description:
 *   Remove host interface user defined trap
 *
 * [in] hostif_user_defined_trap_id Host interface user defined trap id
 *
 * Return Values:
 *   SAI_STATUS_SUCCESS on success
 *   Failure status code on error
 */
sai_status_t sai_remove_hostif_user_defined_trap(
    _Out_ sai_object_id_t hostif_user_defined_trap_id) {
  SAI_LOG_ENTER();
  switch_object_id_t sw_trap_id = {.data = hostif_user_defined_trap_id};

  sai_status_t status =
      status_switch_to_sai(bf_switch_object_delete(sw_trap_id));

  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to delete hostif_user_defined_trap %" PRIx64 ": %s",
                  hostif_user_defined_trap_id,
                  sai_metadata_get_status_name(status));
    return status;
  }

  SAI_LOG_EXIT();

  return SAI_STATUS_SUCCESS;
}

sai_status_t sai_create_hostif_table_entry(
    _In_ sai_object_id_t *hostif_table_entry_id,
    _In_ sai_object_id_t switch_id,
    _In_ uint32_t attr_count,
    _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  const switch_object_type_t ot = SWITCH_OBJECT_TYPE_HOSTIF_RX_FILTER;

  if (!hostif_table_entry_id || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter: %s", sai_metadata_get_status_name(status));
    return status;
  }

  *hostif_table_entry_id = SAI_NULL_OBJECT_ID;

  switch_object_id_t switch_hostif_rx_filter_object_id = {};
  std::set<smi::attr_w> sw_attrs;

  for (unsigned i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_HOSTIF_TABLE_ENTRY, &attr_list[i], sw_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "failed to create hostif_table entry, attribute get failed: %s",
              sai_metadata_get_status_name(status));
          return status;
        }
        break;
    }
  }

  sai_insert_device_attribute(0, SWITCH_HOSTIF_RX_FILTER_ATTR_DEVICE, sw_attrs);
  switch_status =
      bf_switch_object_create(ot, sw_attrs, switch_hostif_rx_filter_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to create hostif table_entry sai_status: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *hostif_table_entry_id = switch_hostif_rx_filter_object_id.data;
  SAI_LOG_EXIT();
  return status;
}

sai_status_t sai_remove_hostif_table_entry(
    _In_ sai_object_id_t hostif_table_entry_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  const switch_object_id_t sw_object_id = {.data = hostif_table_entry_id};
  switch_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to remove hostif trap %" PRIx64 ": %s",
                  hostif_table_entry_id,
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return status;
}

sai_status_t sai_get_hostif_table_entry_attribute(
    _In_ sai_object_id_t hostif_table_entry_id,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  const sai_object_type_t sai_ot = SAI_OBJECT_TYPE_HOSTIF_TABLE_ENTRY;
  switch_object_id_t sw_hostif_table_id = {.data = hostif_table_entry_id};

  if (attr_count && !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  for (uint32_t i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      default:
        status = sai_to_switch_attribute_get(
            sai_ot, sw_hostif_table_id, &attr_list[i]);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to get hostif table entry %" PRIx64
                        " attribute '%s': %s",
                        hostif_table_entry_id,
                        sai_attribute_name(sai_ot, attr_list[i].id),
                        sai_metadata_get_status_name(status));
          return status;
        }
        break;
    }
  }

  SAI_LOG_EXIT();

  return status;
}

sai_status_t sai_set_hostif_table_entry_attribute(
    _In_ sai_object_id_t hostif_table_entry_id,
    _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  const sai_object_type_t sai_ot = SAI_OBJECT_TYPE_HOSTIF_TABLE_ENTRY;
  switch_object_id_t sw_hostif_table_id = {.data = hostif_table_entry_id};

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute: %s", sai_metadata_get_status_name(status));
    return status;
  }

  switch (attr->id) {
    default:
      status = sai_to_switch_attribute_set(sai_ot, attr, sw_hostif_table_id);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("failed to set hostif table entry %" PRIx64
                      " attribute '%s': %s",
                      hostif_table_entry_id,
                      sai_attribute_name(sai_ot, attr->id),
                      sai_metadata_get_status_name(status));
        return status;
      }
      break;
  }

  SAI_LOG_EXIT();

  return status;
}

/*
 * Routine Description:
 *   hostif receive function
 *
 * Arguments:
 *    [in]  hif_id  - host interface id
 *    [out] buffer - packet buffer
 *    [in,out] buffer_size - [in] allocated buffer size. [out] actual packet
 *size
 *in bytes
 *    [in,out] attr_count - [in] allocated list size. [out] number of
 *attributes
 *    [out] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    SAI_STATUS_BUFFER_OVERFLOW if buffer_size is insufficient,
 *    and buffer_size will be filled with required size. Or
 *    if attr_count is insufficient, and attr_count
 *    will be filled with required count.
 *    Failure status code on error
 */
sai_status_t sai_recv_hostif_packet(_In_ sai_object_id_t hif_id,
                                    _Inout_ sai_size_t *buffer_size,
                                    _Out_ void *buffer,
                                    _Inout_ uint32_t *attr_count,
                                    _Out_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  SAI_ASSERT(sai_object_type_query(hif_id) == SAI_OBJECT_TYPE_HOSTIF);

  SAI_LOG_EXIT();

  return SAI_STATUS_SUCCESS;
}

#ifndef SWITCH_SAI_SEND_HOSTIF_PACKET_ENABLE

#else
uint16_t switch_sai_tx_type_to_switch_api_tx_type(
    sai_hostif_tx_type_t tx_type) {
  switch (tx_type) {
    case SAI_HOSTIF_TX_TYPE_PIPELINE_BYPASS:
      return SWITCH_BYPASS_ALL;
    case SAI_HOSTIF_TX_TYPE_PIPELINE_LOOKUP:
      return SWITCH_BYPASS_NONE;
    default:
      return SWITCH_BYPASS_NONE;
  }
}
#endif

/*
 * Routine Description:
 *   hostif send function
 *
 * Arguments:
 *    [in] hif_id  - host interface id. only valid for send through FD channel.
 *Use SAI_NULL_OBJECT_ID for send through CB channel.
 *    [In] buffer - packet buffer
 *    [in] buffer size - packet size in bytes
 *    [in] attr_count - number of attributes
 *    [in] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_send_hostif_packet(_In_ sai_object_id_t hif_id,
                                    _In_ sai_size_t buffer_size,
                                    _In_ const void *buffer,
                                    _In_ uint32_t attr_count,
                                    _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  /*@fixme: add this when we need to support TX part. */

#ifndef SWITCH_SAI_SEND_HOSTIF_PACKET_ENABLE

#else
  switch_hostif_packet_t hostif_packet;
  sai_status_t status = SAI_STATUS_SUCCESS;
  const sai_attribute_t *attribute;
  void *pkt_buffer = NULL;
  uint32_t index = 0;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  if (!buffer) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return SAI_STATUS_INVALID_PARAMETER;
  }

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return SAI_STATUS_INVALID_PARAMETER;
  }

  pkt_buffer = calloc(1, buffer_size);
  if (!pkt_buffer) {
    status = SAI_STATUS_NO_MEMORY;
    SAI_LOG_ERROR("pkt buffer alloc failed: %s",
                  sai_metadata_get_status_name(status));
  }

  memcpy(pkt_buffer, buffer, buffer_size);
  memset(&hostif_packet, 0, sizeof(switch_hostif_packet_t));
  hostif_packet.pkt = pkt_buffer;
  hostif_packet.pkt_size = buffer_size;

  for (index = 0; index < attr_count; index++) {
    attribute = &attr_list[index];
    switch (attribute->id) {
      case SAI_HOSTIF_PACKET_ATTR_HOSTIF_TX_TYPE:
        hostif_packet.bypass_flags =
            switch_sai_tx_type_to_switch_api_tx_type(attribute->value.u32);
        break;
      case SAI_HOSTIF_PACKET_ATTR_EGRESS_PORT_OR_LAG:
        hostif_packet.handle = attribute->value.oid;
        // Set is_lag flag if oid is lag
        break;
      default:
        break;
    }
  }

  switch_status = switch_api_hostif_tx_packet(&hostif_packet);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to send hostif packet on %lx: %s",
                  hif_id,
                  sai_metadata_get_status_name(status));
  }

  if (pkt_buffer) free(pkt_buffer);

#endif

  SAI_LOG_EXIT();

  return status;
}

/*
 * Routine Description:
 *   hostif receive callback
 *
 * Arguments:
 *    [in] buffer - packet buffer
 *    [in] buffer_size - actual packet size in bytes
 *    [in] attr_count - number of attributes
 *    [in] attr_list - array of attributes
 *
 * Return Values:
 */
#ifndef SWITCH_SAI_HOSTIF_PKT_CB_ENABLE
void sai_recv_hostif_packet_cb() { return; }

#else
void sai_recv_hostif_packet_cb(switch_hostif_packet_t *hostif_packet) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  sai_object_id_t device = 0;
  if (!hostif_packet) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return;
  }

  int attr_count = 0;
  sai_attribute_t attr_list[3];
  sai_attribute_t *attribute;
  attribute = &attr_list[attr_count];
  attribute->id = SAI_HOSTIF_PACKET_ATTR_HOSTIF_TRAP_ID;
  attribute->value.u32 = hostif_packet->reason_code;
  attr_count++;
  attribute = &attr_list[attr_count];
  attribute->id = SAI_HOSTIF_PACKET_ATTR_INGRESS_PORT;
  attribute->value.oid = hostif_packet->handle;
  attr_count++;
  if (hostif_packet->lag_handle != SWITCH_API_INVALID_HANDLE) {
    attribute = &attr_list[attr_count];
    attribute->id = SAI_HOSTIF_PACKET_ATTR_INGRESS_LAG;
    attribute->value.oid = hostif_packet->lag_handle;
    attr_count++;
  }

  if (sai_switch_notifications.on_packet_event) {
    sai_switch_notifications.on_packet_event(device,
                                             hostif_packet->pkt_size,
                                             hostif_packet->pkt,
                                             attr_count,
                                             attr_list);
  }
  SAI_LOG_EXIT();
  return;
}

#endif

/*
 * hostif methods table retrieved with sai_api_query()
 */

sai_hostif_api_t hostif_api = {
  create_hostif : sai_create_hostif,
  remove_hostif : sai_remove_hostif,
  set_hostif_attribute : sai_set_hostif_attribute,
  get_hostif_attribute : sai_get_hostif_attribute,
  create_hostif_table_entry : sai_create_hostif_table_entry,
  remove_hostif_table_entry : sai_remove_hostif_table_entry,
  set_hostif_table_entry_attribute : sai_set_hostif_table_entry_attribute,
  get_hostif_table_entry_attribute : sai_get_hostif_table_entry_attribute,
  create_hostif_trap_group : sai_create_hostif_trap_group,
  remove_hostif_trap_group : sai_remove_hostif_trap_group,
  set_hostif_trap_group_attribute : sai_set_hostif_trap_group_attribute,
  get_hostif_trap_group_attribute : sai_get_hostif_trap_group_attribute,
  create_hostif_trap : sai_create_hostif_trap,
  remove_hostif_trap : sai_remove_hostif_trap,
  set_hostif_trap_attribute : sai_set_hostif_trap_attribute,
  get_hostif_trap_attribute : sai_get_hostif_trap_attribute,
  create_hostif_user_defined_trap : sai_create_hostif_user_defined_trap,
  remove_hostif_user_defined_trap : sai_remove_hostif_user_defined_trap,
  set_hostif_user_defined_trap_attribute :
      sai_set_hostif_user_defined_trap_attribute,
  get_hostif_user_defined_trap_attribute :
      sai_get_hostif_user_defined_trap_attribute,
  recv_hostif_packet : sai_recv_hostif_packet,
  send_hostif_packet : sai_send_hostif_packet,
};

sai_hostif_api_t *sai_hostif_api_get() { return &hostif_api; }

sai_status_t sai_hostif_initialize(bool warm_init) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  SAI_LOG_DEBUG("Initializing host interface");
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_HOSTIF);
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_HOSTIF_TRAP);
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_HOSTIF_TRAP_GROUP);
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_HOSTIF_TABLE_ENTRY);
  bf_sai_add_object_type_to_supported_list(
      SAI_OBJECT_TYPE_HOSTIF_USER_DEFINED_TRAP);
  device_handle = sai_get_device_id(0);

  /* get cpu_port_handle */
  smi::attr_w device_cpu_port_attr(SWITCH_DEVICE_ATTR_CPU_PORT);
  switch_status = bf_switch_attribute_get(
      device_handle, SWITCH_DEVICE_ATTR_CPU_PORT, device_cpu_port_attr);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to fetch cpu_port witch attribute, error: %s",
                  sai_metadata_get_status_name(status));
  }

  device_cpu_port_attr.v_get(cpu_port_handle);

  // During warm init there is no need to create default hostif trap group
  // as it was already created earlier before warm init. We simply fetch
  // the trap group from device object and cache it here for future use
  if (warm_init) {
    smi::attr_w device_hostif_trap_group_attr(
        SWITCH_DEVICE_ATTR_DEFAULT_HOSTIF_TRAP_GROUP);
    switch_status =
        bf_switch_attribute_get(device_handle,
                                SWITCH_DEVICE_ATTR_DEFAULT_HOSTIF_TRAP_GROUP,
                                device_hostif_trap_group_attr);
    status = status_switch_to_sai(switch_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR(
          "Failed to fetch switch default hosttif trap group, error: %s",
          sai_metadata_get_status_name(status));
    }
    device_hostif_trap_group_attr.v_get(default_hostif_trap_group_id);
    return status;
  }

/*@fixme: to do: rx_call back API support not added yet as no use case as of now
 * in switchapi */
#ifdef SMI_HOSTIF_RX_CB_ENABLE
  switch_api_hostif_rx_callback_register(
      device, SWITCH_SAI_APP_ID, &sai_recv_hostif_packet_cb, NULL);
#endif

  /*
   * create default hostif trap group.
   * this will be used as default hostif_trap_group if trap created without
   * hostif trap group.
   */

  std::set<smi::attr_w> trap_grup_attrs;
  switch_object_id_t cpu_queue0_handle;
  status = sai_get_switch_queue_handle(0, cpu_queue0_handle);

  /* set attributes to create default hostif trap group */
  trap_grup_attrs.insert(
      attr_w(SWITCH_HOSTIF_TRAP_GROUP_ATTR_QUEUE_HANDLE, cpu_queue0_handle));

  sai_insert_device_attribute(
      0, SWITCH_HOSTIF_TRAP_GROUP_ATTR_DEVICE, trap_grup_attrs);
  switch_status = bf_switch_object_create(SWITCH_OBJECT_TYPE_HOSTIF_TRAP_GROUP,
                                          trap_grup_attrs,
                                          default_hostif_trap_group_id);

  attr_w sattr(SWITCH_DEVICE_ATTR_DEFAULT_HOSTIF_TRAP_GROUP,
               default_hostif_trap_group_id);
  switch_status = bf_switch_attribute_set(device_handle, sattr);
  if (switch_status != SWITCH_STATUS_SUCCESS) {
    status = status_switch_to_sai(switch_status);
    SAI_LOG_ERROR("Failed to set hostif default trap group, error: %s",
                  sai_metadata_get_status_name(status));
  }

  // Populate supported hostif user defined types. Used for enum capability
  // query
  if (bf_switch_is_feature_enabled(SWITCH_FEATURE_INGRESS_ACL_METER)) {
    supported_hostif_user_defined_trap_types.push_back(
        SAI_HOSTIF_USER_DEFINED_TRAP_TYPE_ACL);
  }

  return SAI_STATUS_SUCCESS;
}
