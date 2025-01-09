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

#include <set>
#include <vector>
#include <list>

static sai_api_t api_id = SAI_API_SCHEDULER_GROUP;

/*
 *  Routine Description:
 *    Create Scheduler group
 *
 *  Arguments:
 *    [out] scheduler_group_id - scheudler group id
 *    [in] switch_id - switch Object id
 *    [in] attr_count - number of attributes
 *    [in] attr_list - array of attributes
 *
 *  Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_create_scheduler_group(
    _Out_ sai_object_id_t *scheduler_group_id,
    _In_ sai_object_id_t switch_id,
    _In_ uint32_t attr_count,
    _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  uint32_t index = 0;

  if (!attr_list || !scheduler_group_id) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *scheduler_group_id = SAI_NULL_OBJECT_ID;

  switch_object_id_t scheduler_group_object_id = {};
  std::set<smi::attr_w> sw_attrs;
  for (index = 0; index < attr_count; index++) {
    const sai_attribute_t *attribute = &attr_list[index];
    switch (attribute->id) {
      case SAI_SCHEDULER_GROUP_ATTR_LEVEL: {
        switch_enum_t type = {};
        type.enumdata = (attribute->value.u8 == 0)
                            ? SWITCH_SCHEDULER_GROUP_ATTR_TYPE_PORT
                            : SWITCH_SCHEDULER_GROUP_ATTR_TYPE_QUEUE;
        sw_attrs.insert(smi::attr_w(SWITCH_SCHEDULER_GROUP_ATTR_TYPE, type));
        break;
      }
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_SCHEDULER_GROUP, attribute, sw_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to create scheduler group: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
    }
  }

  sai_insert_device_attribute(0, SWITCH_SCHEDULER_GROUP_ATTR_DEVICE, sw_attrs);

  switch_status = bf_switch_object_create(
      SWITCH_OBJECT_TYPE_SCHEDULER_GROUP, sw_attrs, scheduler_group_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to create scheduler group: %s",
                  sai_metadata_get_status_name(status));
  }
  *scheduler_group_id = scheduler_group_object_id.data;

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 *  Routine Description:
 *    Remove Scheduler group
 *
 *  Arguments:
 *    [in] scheduler_group_id - scheduler group id
 *
 *  Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_remove_scheduler_group(
    _In_ sai_object_id_t scheduler_group_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  if (sai_object_type_query(scheduler_group_id) !=
      SAI_OBJECT_TYPE_SCHEDULER_GROUP) {
    SAI_LOG_ERROR(
        "Scheduler group remove failed: invalid scheduler group handle "
        "0x%" PRIx64 "\n",
        scheduler_group_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = scheduler_group_id};
  switch_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to remove scheduler 0x%" PRIx64 ": %s",
                  scheduler_group_id,
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 * Routine Description:
 *   Set Scheduler Group Attribute
 *
 * Arguments:
 *   [in] scheduler_group_id - scheduler group id
 *   [in] attr - attribute
 *
 * Return Values:
 *   SAI_STATUS_SUCCESS on success
 *   Failure status code on error
 */
sai_status_t sai_set_scheduler_group_attribute(
    _In_ sai_object_id_t scheduler_group_id, _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute: %s", sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(scheduler_group_id) !=
      SAI_OBJECT_TYPE_SCHEDULER_GROUP) {
    SAI_LOG_ERROR(
        "Scheduler group set failed: invalid scheduler group handle 0x%" PRIx64
        "\n",
        scheduler_group_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = scheduler_group_id};
  switch (attr->id) {
    case SAI_SCHEDULER_GROUP_ATTR_LEVEL: {
      switch_enum_t type = {};
      type.enumdata = (attr->value.u8 == 1)
                          ? SWITCH_SCHEDULER_GROUP_ATTR_TYPE_PORT
                          : SWITCH_SCHEDULER_GROUP_ATTR_TYPE_QUEUE;
      sw_status = bf_switch_attribute_set(
          sw_object_id, smi::attr_w(SWITCH_SCHEDULER_GROUP_ATTR_TYPE, type));
      status = status_switch_to_sai(sw_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR(
            "Failed to set scheduler group 0x%" PRIx64
            " attribute %s error: %s",
            scheduler_group_id,
            sai_attribute_name(SAI_OBJECT_TYPE_SCHEDULER_GROUP, attr->id),
            sai_metadata_get_status_name(status));
        return status;
      }
      break;
    }
    default:
      status = sai_to_switch_attribute_set(
          SAI_OBJECT_TYPE_SCHEDULER_GROUP, attr, sw_object_id);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR(
            "Failed to set scheduler group 0x%" PRIx64
            " attribute %s error: %s",
            scheduler_group_id,
            sai_attribute_name(SAI_OBJECT_TYPE_SCHEDULER_GROUP, attr->id),
            sai_metadata_get_status_name(status));
        return status;
      }
      break;
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 * Routine Description:
 *   Get SAI Scheduler Group child cound or
 *   child count attribute
 *
 * Arguments:
 *   [in] sched_group - scheduler group id
 *   [out] attr - attribute
 *
 * Return Values:
 *   SAI_STATUS_SUCCESS on success
 *   Failure status code on error
 */
static sai_status_t sai_get_scheduler_group_child_list_count_attr(
    _In_ switch_object_id_t sched_group, _Out_ sai_attribute_t &attr) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;
  switch_enum_t type = {};

  // Get scheduler group type
  smi::attr_w type_attr(SWITCH_SCHEDULER_GROUP_ATTR_TYPE);
  sw_status = bf_switch_attribute_get(
      sched_group, SWITCH_SCHEDULER_GROUP_ATTR_TYPE, type_attr);
  status = status_switch_to_sai(sw_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get switch scheduler group 0x%" PRIx64
                  " type error: %s",
                  sched_group.data,
                  sai_metadata_get_status_name(status));
    return status;
  }

  type_attr.v_get(type);

  switch (type.enumdata) {
    case SWITCH_SCHEDULER_GROUP_ATTR_TYPE_PORT: {
      // Get port handle
      switch_object_id_t port = {.data = 0};
      smi::attr_w sw_attr(SWITCH_SCHEDULER_GROUP_ATTR_PORT_HANDLE);
      sw_status = bf_switch_attribute_get(
          sched_group, SWITCH_SCHEDULER_GROUP_ATTR_PORT_HANDLE, sw_attr);
      status = status_switch_to_sai(sw_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to get scheduler group 0x%" PRIx64
                      " port handle attribute error: %s",
                      sched_group.data,
                      sai_metadata_get_status_name(status));
        return status;
      }
      sw_attr.v_get(port);

      // Get scheduler group list attached to queues on this port
      attr_w port_q_sch_grp_handles(
          SWITCH_PORT_ATTR_PORT_QUEUE_SCHEDULER_GROUP_HANDLES);
      sw_status = bf_switch_attribute_get(
          port,
          SWITCH_PORT_ATTR_PORT_QUEUE_SCHEDULER_GROUP_HANDLES,
          port_q_sch_grp_handles);
      status = status_switch_to_sai(sw_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to get switch port 0x%" PRIx64
                      " port queue scheduler queue handles attribute error: %s",
                      port.data,
                      sai_metadata_get_status_name(status));
        return status;
      }

      std::vector<switch_object_id_t> scheduler_group_handles;
      port_q_sch_grp_handles.v_get(scheduler_group_handles);

      std::list<sai_object_id_t> oids;
      for (auto sch_grp : scheduler_group_handles) {
        attr_w sch_type(SWITCH_SCHEDULER_GROUP_ATTR_TYPE);
        sw_status = bf_switch_attribute_get(
            sch_grp, SWITCH_SCHEDULER_GROUP_ATTR_TYPE, sch_type);
        status = status_switch_to_sai(sw_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to get scheduler group 0x%" PRIx64
                        " port handle attribute error: %s",
                        sched_group.data,
                        sai_metadata_get_status_name(status));
          return status;
        }
        switch_enum_t sch_type_e = {};
        sch_type.v_get(sch_type_e);
        if (sch_type_e.enumdata == SWITCH_SCHEDULER_GROUP_ATTR_TYPE_QUEUE) {
          oids.push_back(sch_grp.data);
        }
      }
      if (attr.id == SAI_SCHEDULER_GROUP_ATTR_CHILD_COUNT) {
        attr.value.u32 = oids.size();
      } else {
        TRY_LIST_SET(attr.value.objlist, oids);
      }
    } break;
    case SWITCH_SCHEDULER_GROUP_ATTR_TYPE_QUEUE: {
      switch_object_id_t queue = {.data = 0};
      smi::attr_w sw_attr(SWITCH_SCHEDULER_GROUP_ATTR_QUEUE_HANDLE);

      sw_status = bf_switch_attribute_get(
          sched_group, SWITCH_SCHEDULER_GROUP_ATTR_QUEUE_HANDLE, sw_attr);
      status = status_switch_to_sai(sw_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to get switch scheduler group 0x%" PRIx64
                      " queue handle error: %s",
                      sched_group.data,
                      sai_metadata_get_status_name(status));
        return status;
      }

      sw_attr.v_get(queue);

      if (attr.id == SAI_SCHEDULER_GROUP_ATTR_CHILD_COUNT) {
        attr.value.u32 = 1;
      } else {
        TRY_LIST_SET(attr.value.objlist,
                     std::vector<sai_object_id_t>{queue.data});
      }
      break;
    }
    default:
      SAI_LOG_ERROR("Invalid scheduler group 0x%" PRIx64 " type",
                    sched_group.data);
      return SAI_STATUS_FAILURE;
  }

  return SAI_STATUS_SUCCESS;
}
/*
 * Routine Description:
 *   Get Scheduler Group attribute
 *
 * Arguments:
 *   [in] scheduler_group_id - scheduler group id
 *   [in] attr_count - number of attributes
 *   [out] attr_list - array of attributes
 *
 * Return Values:
 *   SAI_STATUS_SUCCESS on success
 *   Failure status code on error
 */
sai_status_t sai_get_scheduler_group_attribute(
    _In_ sai_object_id_t scheduler_group_id,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  if (sai_object_type_query(scheduler_group_id) !=
      SAI_OBJECT_TYPE_SCHEDULER_GROUP) {
    SAI_LOG_ERROR(
        "Scheduler group get failed: invalid scheduler group handle 0x%" PRIx64
        "\n",
        scheduler_group_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = scheduler_group_id};

  for (uint32_t index = 0; index < attr_count; index++) {
    switch (attr_list[index].id) {
      case SAI_SCHEDULER_GROUP_ATTR_CHILD_LIST:
      case SAI_SCHEDULER_GROUP_ATTR_CHILD_COUNT:
        status = sai_get_scheduler_group_child_list_count_attr(
            sw_object_id, attr_list[index]);
        if (status != SAI_STATUS_SUCCESS) {
          return status;
        }
        break;
      case SAI_SCHEDULER_GROUP_ATTR_PARENT_NODE:  // Unsupported
        break;
      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_SCHEDULER_GROUP, sw_object_id, &attr_list[index]);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to get attribute %s error: %s\n",
                        sai_attribute_name(SAI_OBJECT_TYPE_SCHEDULER_GROUP,
                                           attr_list[index].id),
                        sai_metadata_get_status_name(status));
          return status;
        }
        break;
    }
  }

  SAI_LOG_EXIT();

  return status;
}

/*
 *  Scheduler Group methods table retrieved with sai_api_query()
 */
sai_scheduler_group_api_t scheduler_group_api = {
    .create_scheduler_group = sai_create_scheduler_group,
    .remove_scheduler_group = sai_remove_scheduler_group,
    .set_scheduler_group_attribute = sai_set_scheduler_group_attribute,
    .get_scheduler_group_attribute = sai_get_scheduler_group_attribute,
};

sai_scheduler_group_api_t *sai_scheduler_group_api_get() {
  return &scheduler_group_api;
}

sai_status_t sai_scheduler_group_initialize() {
  SAI_LOG_DEBUG("Initializing scheduler group");
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_SCHEDULER_GROUP);
  return SAI_STATUS_SUCCESS;
}
