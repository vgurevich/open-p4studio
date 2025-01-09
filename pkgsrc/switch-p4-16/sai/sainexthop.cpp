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

#include "s3/switch_store.h"

static sai_api_t api_id = SAI_API_NEXT_HOP;

static sai_status_t sai_get_nh_rw_type(switch_object_id_t tunnel_handle,
                                       bool tunnel_vni,
                                       switch_enum_t &rw_type) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  switch_enum_t tunnel_type = {.enumdata = SWITCH_TUNNEL_ATTR_TYPE_NONE};
  smi::attr_w type_attr(SWITCH_TUNNEL_ATTR_TYPE);

  // Get the tunnel type
  switch_status = bf_switch_attribute_get(
      tunnel_handle, SWITCH_TUNNEL_ATTR_TYPE, type_attr);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("Failed to get tunnel type: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  type_attr.v_get(tunnel_type);
  if (tunnel_type.enumdata == SWITCH_TUNNEL_ATTR_TYPE_IPIP) {
    rw_type.enumdata = SWITCH_NEXTHOP_ATTR_RW_TYPE_L3;
  } else if (tunnel_type.enumdata == SWITCH_TUNNEL_ATTR_TYPE_VXLAN) {
    if (tunnel_vni) {
      rw_type.enumdata = SWITCH_NEXTHOP_ATTR_RW_TYPE_L3_VNI;
    } else {
      rw_type.enumdata = SWITCH_NEXTHOP_ATTR_RW_TYPE_L3;
    }
  }

  return status;
}

/*
 * Routine Description:
 *   Converts SAI outseg ttl_mode to switch ttl_mode.
 *
 * Arguments:
 *  [in] sai_ttl_mode - SAI ttl_mode
 *  [out] sw_ttl_mode - switch ttl_mode
 *
 * Return Values:
 *    Void
 */
static void sai_outseg_ttl_mode_to_switch(
    _In_ sai_outseg_ttl_mode_t sai_ttl_mode,
    _Out_ switch_nexthop_attr_mpls_encap_ttl_mode &sw_ttl_mode) {
  switch (sai_ttl_mode) {
    case SAI_OUTSEG_TTL_MODE_UNIFORM:
      sw_ttl_mode = SWITCH_NEXTHOP_ATTR_MPLS_ENCAP_TTL_MODE_UNIFORM_MODEL;
      break;
    case SAI_OUTSEG_TTL_MODE_PIPE:
      sw_ttl_mode = SWITCH_NEXTHOP_ATTR_MPLS_ENCAP_TTL_MODE_PIPE_MODEL;
      break;
    default:
      SAI_LOG_ERROR("Invalid sai ttl_mode");
      sw_ttl_mode = SWITCH_NEXTHOP_ATTR_MPLS_ENCAP_TTL_MODE_UNIFORM_MODEL;
      break;
  }
}

/*
 * Routine Description:
 *   Converts switch mpls encap ttl_mode to SAI outseg ttl_mode.
 *
 * Arguments:
 *  [in] sw_ttl_mode - switch ttl_mode
 *  [out] sai_ttl_mode - SAI ttl_mode
 *
 * Return Values:
 *    Void
 */
static void sai_switch_mpls_encap_ttl_mode_to_sai(
    _In_ switch_nexthop_attr_mpls_encap_ttl_mode sw_ttl_mode,
    _Out_ sai_outseg_ttl_mode_t &sai_ttl_mode) {
  switch (sw_ttl_mode) {
    case SWITCH_NEXTHOP_ATTR_MPLS_ENCAP_TTL_MODE_UNIFORM_MODEL:
      sai_ttl_mode = SAI_OUTSEG_TTL_MODE_UNIFORM;
      break;
    case SWITCH_NEXTHOP_ATTR_MPLS_ENCAP_TTL_MODE_PIPE_MODEL:
      sai_ttl_mode = SAI_OUTSEG_TTL_MODE_PIPE;
      break;
    default:
      SAI_LOG_ERROR("Invalid switch ttl_mode");
      sai_ttl_mode = SAI_OUTSEG_TTL_MODE_UNIFORM;
      break;
  }
}

/*
 * Routine Description:
 *   Converts SAI outseg exp_mode to switch qos_mode.
 *
 * Arguments:
 *  [in] sai_exp_mode - SAI exp_mode
 *  [out] sw_qos_mode - switch qos_mode
 *
 * Return Values:
 *    Void
 */
static void sai_outseg_exp_mode_to_switch(
    _In_ sai_outseg_exp_mode_t sai_exp_mode,
    _Out_ switch_nexthop_attr_mpls_encap_qos_mode &sw_qos_mode) {
  switch (sai_exp_mode) {
    case SAI_OUTSEG_EXP_MODE_UNIFORM:
      sw_qos_mode = SWITCH_NEXTHOP_ATTR_MPLS_ENCAP_QOS_MODE_UNIFORM_MODEL;
      break;
    case SAI_OUTSEG_EXP_MODE_PIPE:
      sw_qos_mode = SWITCH_NEXTHOP_ATTR_MPLS_ENCAP_QOS_MODE_PIPE_MODEL;
      break;
    default:
      SAI_LOG_ERROR("Invalid sai exp_mode");
      sw_qos_mode = SWITCH_NEXTHOP_ATTR_MPLS_ENCAP_QOS_MODE_UNIFORM_MODEL;
      break;
  }
}

/*
 * Routine Description:
 *   Converts switch mpls encap qos_mode to SAI outseg exp_mode.
 *
 * Arguments:
 *  [in] sw_qos_mode - switch qos_mode
 *  [out] sai_exp_mode - SAI exp_mode
 *
 * Return Values:
 *    Void
 */
static void sai_switch_mpls_encap_qos_mode_to_sai(
    _In_ switch_nexthop_attr_mpls_encap_qos_mode sw_qos_mode,
    _Out_ sai_outseg_exp_mode_t &sai_exp_mode) {
  switch (sw_qos_mode) {
    case SWITCH_NEXTHOP_ATTR_MPLS_ENCAP_QOS_MODE_UNIFORM_MODEL:
      sai_exp_mode = SAI_OUTSEG_EXP_MODE_UNIFORM;
      break;
    case SWITCH_NEXTHOP_ATTR_MPLS_ENCAP_QOS_MODE_PIPE_MODEL:
      sai_exp_mode = SAI_OUTSEG_EXP_MODE_PIPE;
      break;
    default:
      SAI_LOG_ERROR("Invalid switch exp_mode");
      sai_exp_mode = SAI_OUTSEG_EXP_MODE_UNIFORM;
      break;
  }
}

/*
 * Routine Description:
 *    Create next hop
 *
 * Arguments:
 *    [out] next_hop_id - next hop id
 *    [in] attr_count - number of attributes
 *    [in] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 *
 * Note: IP address expected in Network Byte Order.
 */
sai_status_t sai_create_next_hop_entry(_Out_ sai_object_id_t *next_hop_id,
                                       _In_ sai_object_id_t switch_id,
                                       _In_ uint32_t attr_count,
                                       _In_ const sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  const sai_attribute_t *attribute;
  sai_status_t status = SAI_STATUS_SUCCESS;
  uint32_t index = 0;
  const switch_object_type_t ot = SWITCH_OBJECT_TYPE_NEXTHOP;
  switch_status_t switch_status;
  bool tunnel_vni = false, sidlist = false;
  switch_object_id_t tunnel_handle = {}, tunnel_rif_handle = {};
  switch_object_id_t sidlist_handle = {};
  switch_enum_t label_op = {.enumdata = SWITCH_NEXTHOP_ATTR_LABELOP_NONE};
  std::vector<uint32_t> label_list;

  if (!next_hop_id || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *next_hop_id = SAI_NULL_OBJECT_ID;

  switch_object_id_t nexthop_object_id = {};
  std::set<smi::attr_w> sw_attrs;
  switch_enum_t sw_enum = {0};
  for (index = 0; index < attr_count; index++) {
    attribute = &attr_list[index];
    switch (attribute->id) {
      case SAI_NEXT_HOP_ATTR_TUNNEL_ID:
      case SAI_NEXT_HOP_ATTR_ROUTER_INTERFACE_ID:
        tunnel_handle.data = attribute->value.oid;
        sw_attrs.insert(smi::attr_w(SWITCH_NEXTHOP_ATTR_HANDLE, tunnel_handle));
        break;
      case SAI_NEXT_HOP_ATTR_TUNNEL_VNI:
        sw_attrs.insert(
            smi::attr_w(SWITCH_NEXTHOP_ATTR_TUNNEL_VNI, attribute->value.u32));
        tunnel_vni = true;
        break;
#if SAI_API_VERSION >= 10901
      case SAI_NEXT_HOP_ATTR_SRV6_SIDLIST_ID:
        sidlist_handle.data = attribute->value.oid;
        sw_attrs.insert(
            smi::attr_w(SWITCH_NEXTHOP_ATTR_SRV6_SIDLIST_ID, sidlist_handle));
        sidlist = true;
        break;
#endif
      case SAI_NEXT_HOP_ATTR_OUTSEG_TTL_MODE: {
        if (bf_switch_is_feature_enabled(SWITCH_FEATURE_TUNNEL_TTL_MODE)) {
          switch_nexthop_attr_mpls_encap_ttl_mode sw_ttl_mode;
          sai_outseg_ttl_mode_to_switch(
              (sai_outseg_ttl_mode_t)attribute->value.s32, sw_ttl_mode);
          sw_enum.enumdata = sw_ttl_mode;
          sw_attrs.insert(
              smi::attr_w(SWITCH_NEXTHOP_ATTR_MPLS_ENCAP_TTL_MODE, sw_enum));
        } else {
          if (attribute->value.s32 == SAI_OUTSEG_TTL_MODE_UNIFORM) {
            SAI_LOG_ERROR(
                "Uniform mode for %s is not supported, using default pipe mode",
                sai_attribute_name(SAI_OBJECT_TYPE_NEXT_HOP, attribute->id));
            status = SAI_STATUS_NOT_SUPPORTED;
          }
        }
      } break;
      case SAI_NEXT_HOP_ATTR_OUTSEG_EXP_MODE: {
        if (bf_switch_is_feature_enabled(SWITCH_FEATURE_TUNNEL_QOS_MODE)) {
          switch_nexthop_attr_mpls_encap_qos_mode sw_qos_mode;
          sai_outseg_exp_mode_to_switch(
              (sai_outseg_exp_mode_t)attribute->value.s32, sw_qos_mode);
          sw_enum.enumdata = sw_qos_mode;
          sw_attrs.insert(
              smi::attr_w(SWITCH_NEXTHOP_ATTR_MPLS_ENCAP_QOS_MODE, sw_enum));
        } else {
          if (attribute->value.s32 == SAI_OUTSEG_EXP_MODE_PIPE) {
            SAI_LOG_ERROR(
                "Pipe mode for %s is not supported, using default uniform mode",
                sai_attribute_name(SAI_OBJECT_TYPE_NEXT_HOP, attribute->id));
            status = SAI_STATUS_NOT_SUPPORTED;
          }
        }
      } break;
      case SAI_NEXT_HOP_ATTR_CUSTOM_0:  // customattr = SAI_NEXT_HOP_ATTR_END
        tunnel_rif_handle.data = attribute->value.oid;
        sw_attrs.insert(
            attr_w(SWITCH_NEXTHOP_ATTR_TUNNEL_RIF_HANDLE, tunnel_rif_handle));
        break;
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_NEXT_HOP, attribute, sw_attrs);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to create nexthop: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
    }
  }
  sai_insert_device_attribute(0, SWITCH_NEXTHOP_ATTR_DEVICE, sw_attrs);

  // Check wether this is a tunnel NH and set proper RW type for it
  // If tunnel is IPIP, then type is L3
  // If tunnel is VXLAN, then it is either L3 or L3_VNI
  // Note: SAI nexthop does not map to RW type L2.
  //       For future releases, will investigate whether SAI bridge_port of
  //       type tunnel should cause nexthop with RW type L2 to be created.
  auto it = sw_attrs.find(smi::attr_w(SWITCH_NEXTHOP_ATTR_TYPE));
  if (it != sw_attrs.end()) {
    switch_enum_t nh_type = {.enumdata = SWITCH_NEXTHOP_ATTR_TYPE_NONE};
    it->v_get(nh_type);
    if (nh_type.enumdata == SWITCH_NEXTHOP_ATTR_TYPE_TUNNEL) {
      switch_enum_t rw_type = {.enumdata = SWITCH_NEXTHOP_ATTR_RW_TYPE_NONE};
      status = sai_get_nh_rw_type(tunnel_handle, tunnel_vni, rw_type);
      if (status != SAI_STATUS_SUCCESS) {
        return status;
      }
      sw_attrs.insert(smi::attr_w(SWITCH_NEXTHOP_ATTR_RW_TYPE, rw_type));
      if (sidlist == false) {
        sw_attrs.insert(
            smi::attr_w(SWITCH_NEXTHOP_ATTR_SRV6_SIDLIST_ID, sidlist_handle));
      }
      sw_attrs.insert(smi::attr_w(SWITCH_NEXTHOP_ATTR_LABELOP, label_op));
      sw_attrs.insert(smi::attr_w(SWITCH_NEXTHOP_ATTR_LABELSTACK, label_list));
    } else if (nh_type.enumdata == SWITCH_NEXTHOP_ATTR_TYPE_IP) {
      switch_object_id_t nhop_handle = {};
      if (tunnel_vni == false) {
        sw_attrs.insert(smi::attr_w(SWITCH_NEXTHOP_ATTR_TUNNEL_VNI,
                                    static_cast<uint32_t>(0)));
      }
      if (sidlist == false) {
        sw_attrs.insert(
            smi::attr_w(SWITCH_NEXTHOP_ATTR_SRV6_SIDLIST_ID, sidlist_handle));
      }
      sw_attrs.insert(smi::attr_w(SWITCH_NEXTHOP_ATTR_LABELOP, label_op));
      sw_attrs.insert(smi::attr_w(SWITCH_NEXTHOP_ATTR_LABELSTACK, label_list));
      switch_status = bf_switch_object_get(
          SWITCH_OBJECT_TYPE_NEXTHOP, sw_attrs, nhop_handle);
      if (switch_status == SWITCH_STATUS_SUCCESS) {
        // Found an existing nexthop
        if (nhop_handle.data != 0) {
          *next_hop_id = nhop_handle.data;
          return SAI_STATUS_SUCCESS;
        }
      }
    }
  }

  switch_status = bf_switch_object_create(ot, sw_attrs, nexthop_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to create nexthop: %s",
                  sai_metadata_get_status_name(status));
  }
  *next_hop_id = nexthop_object_id.data;
  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 * Routine Description:
 *    Remove next hop
 *
 * Arguments:
 *    [in] next_hop_id - next hop id
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_remove_next_hop_entry(_In_ sai_object_id_t next_hop_id) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  if (sai_object_type_query(next_hop_id) != SAI_OBJECT_TYPE_NEXT_HOP) {
    SAI_LOG_ERROR("nexthop remove failed: invalid nexthop handle %" PRIx64 "\n",
                  next_hop_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = next_hop_id};
  sai_route_set_glean_internal(sw_object_id);

  bool del = false;
  switch_status = switch_store::object_ready_for_delete(sw_object_id, del);
  // nexthop may have been already deleted during neighbor removal <or>
  // under use by some object
  // Both cases return success
  if (switch_status == SWITCH_STATUS_ITEM_NOT_FOUND || del == false) {
    return (sai_status_t)status;
  }

  switch_status = bf_switch_object_delete(sw_object_id);
  // nexthop may have been removed during neighbor removal
  if (switch_status == SWITCH_STATUS_ITEM_NOT_FOUND) {
    switch_status = SWITCH_STATUS_SUCCESS;
  }
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to remove nexthop %" PRIx64 ": %s",
                  next_hop_id,
                  sai_metadata_get_status_name(status));
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 * Routine Description:
 *    Set Next Hop attribute
 *
 * Arguments:
 *    [in] next_hop_id - next hop id
 *    [in] attr - attribute
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_set_next_hop_entry_attribute(
    _In_ sai_object_id_t next_hop_id, _In_ const sai_attribute_t *attr) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  SAI_ASSERT(sai_object_type_query(next_hop_id) == SAI_OBJECT_TYPE_NEXT_HOP);

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute: %s", sai_metadata_get_status_name(status));
    return status;
  }

  const switch_object_id_t sw_object_id = {.data = next_hop_id};
  switch (attr->id) {
    case SAI_NEXT_HOP_ATTR_TUNNEL_VNI: {
      switch_enum_t nh_type = {.enumdata = SWITCH_NEXTHOP_ATTR_TYPE_NONE};
      smi::attr_w sw_attr(SWITCH_NEXTHOP_ATTR_TYPE);

      switch_status = bf_switch_attribute_get(
          sw_object_id, SWITCH_NEXTHOP_ATTR_TYPE, sw_attr);
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to get next hop type: %s",
                      sai_metadata_get_status_name(status));
        return status;
      }
      sw_attr.v_get(nh_type);

      if (nh_type.enumdata != SWITCH_NEXTHOP_ATTR_TYPE_TUNNEL) {
        SAI_LOG_ERROR("Failed to set tunnel VNI to non tunnel next hop");
        return SAI_STATUS_INVALID_PARAMETER;
      }

      smi::attr_w sw_vni_attr(SWITCH_NEXTHOP_ATTR_TUNNEL_VNI, attr->value.u32);
      switch_status = bf_switch_attribute_set(sw_object_id, sw_vni_attr);
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to set next hop tunnel vni: %s",
                      sai_metadata_get_status_name(status));
        return status;
      }

      switch_object_id_t tunnel_handle = {};
      smi::attr_w sw_handle_attr(SWITCH_NEXTHOP_ATTR_HANDLE);

      switch_status = bf_switch_attribute_get(
          sw_object_id, SWITCH_NEXTHOP_ATTR_HANDLE, sw_handle_attr);
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to get next hop tunnel handle: %s",
                      sai_metadata_get_status_name(status));
        return status;
      }
      sw_handle_attr.v_get(tunnel_handle);

      switch_enum_t rw_type = {.enumdata = SWITCH_NEXTHOP_ATTR_RW_TYPE_NONE};
      status = sai_get_nh_rw_type(tunnel_handle, true, rw_type);
      if (status != SAI_STATUS_SUCCESS) {
        return status;
      }

      smi::attr_w sw_rw_attr(SWITCH_NEXTHOP_ATTR_RW_TYPE, rw_type);
      switch_status = bf_switch_attribute_set(sw_object_id, sw_rw_attr);
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to set next hop tunnel rw type: %s",
                      sai_metadata_get_status_name(status));
        return status;
      }
      break;
    }
    case SAI_NEXT_HOP_ATTR_OUTSEG_TTL_MODE: {
      if (bf_switch_is_feature_enabled(SWITCH_FEATURE_TUNNEL_TTL_MODE)) {
        switch_nexthop_attr_mpls_encap_ttl_mode sw_ttl_mode;
        sai_outseg_ttl_mode_to_switch((sai_outseg_ttl_mode_t)attr->value.s32,
                                      sw_ttl_mode);
        switch_enum_t sw_enum = {.enumdata = sw_ttl_mode};
        smi::attr_w sw_attr(SWITCH_NEXTHOP_ATTR_MPLS_ENCAP_TTL_MODE, sw_enum);
        switch_status = bf_switch_attribute_set(sw_object_id, sw_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to set next hop tunnel mpls ttl_mode: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
      } else {
        if (attr->value.s32 == SAI_OUTSEG_TTL_MODE_UNIFORM) {
          SAI_LOG_ERROR(
              "Uniform mode for %s is not supported, using default pipe mode",
              sai_attribute_name(SAI_OBJECT_TYPE_NEXT_HOP, attr->id));
          status = SAI_STATUS_NOT_SUPPORTED;
        }
      }
    } break;
    case SAI_NEXT_HOP_ATTR_OUTSEG_EXP_MODE: {
      if (bf_switch_is_feature_enabled(SWITCH_FEATURE_TUNNEL_QOS_MODE)) {
        switch_nexthop_attr_mpls_encap_qos_mode sw_qos_mode;
        sai_outseg_exp_mode_to_switch((sai_outseg_exp_mode_t)attr->value.s32,
                                      sw_qos_mode);
        switch_enum_t sw_enum = {.enumdata = sw_qos_mode};
        smi::attr_w sw_attr(SWITCH_NEXTHOP_ATTR_MPLS_ENCAP_QOS_MODE, sw_enum);
        switch_status = bf_switch_attribute_set(sw_object_id, sw_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to set next hop tunnel mpls qos_mode: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
      } else {
        if (attr->value.s32 == SAI_OUTSEG_EXP_MODE_PIPE) {
          SAI_LOG_ERROR(
              "Pipe mode for %s is not supported, using default uniform mode",
              sai_attribute_name(SAI_OBJECT_TYPE_NEXT_HOP, attr->id));
          status = SAI_STATUS_NOT_SUPPORTED;
        }
      }
    } break;
    case SAI_NEXT_HOP_ATTR_CUSTOM_0: {  // customattr = SAI_NEXT_HOP_ATTR_END
      switch_object_id_t rif_handle = {.data = attr->value.oid};
      smi::attr_w sw_attr(SWITCH_NEXTHOP_ATTR_TUNNEL_RIF_HANDLE, rif_handle);
      switch_status = bf_switch_attribute_set(sw_object_id, sw_attr);
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to set next hop custom attribute 0: %s",
                      sai_metadata_get_status_name(status));
        return status;
      }
    } break;
    default:
      status = sai_to_switch_attribute_set(
          SAI_OBJECT_TYPE_NEXT_HOP, attr, sw_object_id);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to set attribute %s error: %s\n",
                      sai_attribute_name(SAI_OBJECT_TYPE_NEXT_HOP, attr->id),
                      sai_metadata_get_status_name(status));
        return status;
      }
  }

  SAI_LOG_EXIT();
  return (sai_status_t)status;
}

/*
 * Routine Description:
 *    Get Next Hop attribute
 *
 * Arguments:
 *    [in] next_hop_id - next hop id
 *    [in] attr_count - number of attributes
 *    [inout] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
sai_status_t sai_get_next_hop_entry_attribute(
    _In_ sai_object_id_t next_hop_id,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  SAI_ASSERT(sai_object_type_query(next_hop_id) == SAI_OBJECT_TYPE_NEXT_HOP);

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  const switch_object_id_t sw_object_id = {.data = next_hop_id};
  uint32_t index;
  sai_attribute_t *attribute;
  for (index = 0; index < attr_count; index++) {
    attribute = &attr_list[index];
    switch (attribute->id) {
      case SAI_NEXT_HOP_ATTR_TUNNEL_VNI: {
        smi::attr_w sw_attr(SWITCH_NEXTHOP_ATTR_TUNNEL_VNI);

        switch_status = bf_switch_attribute_get(
            sw_object_id, SWITCH_NEXTHOP_ATTR_TUNNEL_VNI, sw_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to get next hop tunnel vni: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
        sw_attr.v_get(attribute->value.u32);
      } break;
      case SAI_NEXT_HOP_ATTR_OUTSEG_TTL_MODE: {
        if (bf_switch_is_feature_enabled(SWITCH_FEATURE_TUNNEL_TTL_MODE)) {
          smi::attr_w sw_attr(SWITCH_NEXTHOP_ATTR_MPLS_ENCAP_TTL_MODE);
          switch_status = bf_switch_attribute_get(
              sw_object_id, SWITCH_NEXTHOP_ATTR_MPLS_ENCAP_TTL_MODE, sw_attr);
          status = status_switch_to_sai(switch_status);
          if (status != SAI_STATUS_SUCCESS) {
            SAI_LOG_ERROR("Failed to get next hop tunnel mpls ttl_mode: %s",
                          sai_metadata_get_status_name(status));
            return status;
          }
          switch_enum_t sw_ttl_mode = {0};
          sai_outseg_ttl_mode_t sai_ttl_mode;
          sw_attr.v_get(sw_ttl_mode);
          sai_switch_mpls_encap_ttl_mode_to_sai(
              (switch_nexthop_attr_mpls_encap_ttl_mode)sw_ttl_mode.enumdata,
              sai_ttl_mode);
          attribute->value.s32 = sai_ttl_mode;
        } else {
          attribute->value.s32 = SAI_OUTSEG_TTL_MODE_PIPE;
        }
      } break;
      case SAI_NEXT_HOP_ATTR_OUTSEG_EXP_MODE: {
        if (bf_switch_is_feature_enabled(SWITCH_FEATURE_TUNNEL_QOS_MODE)) {
          smi::attr_w sw_attr(SWITCH_NEXTHOP_ATTR_MPLS_ENCAP_QOS_MODE);
          switch_status = bf_switch_attribute_get(
              sw_object_id, SWITCH_NEXTHOP_ATTR_MPLS_ENCAP_QOS_MODE, sw_attr);
          status = status_switch_to_sai(switch_status);
          if (status != SAI_STATUS_SUCCESS) {
            SAI_LOG_ERROR("Failed to get next hop tunnel mpls qos_mode: %s",
                          sai_metadata_get_status_name(status));
            return status;
          }
          switch_enum_t sw_qos_mode = {0};
          sai_outseg_exp_mode_t sai_exp_mode;
          sw_attr.v_get(sw_qos_mode);
          sai_switch_mpls_encap_qos_mode_to_sai(
              (switch_nexthop_attr_mpls_encap_qos_mode)sw_qos_mode.enumdata,
              sai_exp_mode);
          attribute->value.s32 = sai_exp_mode;
        } else {
          attribute->value.s32 = SAI_OUTSEG_EXP_MODE_UNIFORM;
        }
      } break;
      case SAI_NEXT_HOP_ATTR_CUSTOM_0: {  // customattr = SAI_NEXT_HOP_ATTR_END
        smi::attr_w sw_attr(SWITCH_NEXTHOP_ATTR_TUNNEL_RIF_HANDLE);
        switch_status = bf_switch_attribute_get(
            sw_object_id, SWITCH_NEXTHOP_ATTR_TUNNEL_RIF_HANDLE, sw_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("Failed to get next hop custom attribute 0: %s",
                        sai_metadata_get_status_name(status));
          return status;
        }
        switch_object_id_t tunnel_rif_handle = {};
        sw_attr.v_get(tunnel_rif_handle);
        attribute->value.oid = tunnel_rif_handle.data;
      } break;
      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_NEXT_HOP, sw_object_id, attribute);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to get attribute %s error: %s\n",
              sai_attribute_name(SAI_OBJECT_TYPE_NEXT_HOP, attribute->id),
              sai_metadata_get_status_name(status));
          return status;
        }
    }
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/*
 *  Next Hop methods table retrieved with sai_api_query()
 */
sai_next_hop_api_t nhop_api = {
    .create_next_hop = sai_create_next_hop_entry,
    .remove_next_hop = sai_remove_next_hop_entry,
    .set_next_hop_attribute = sai_set_next_hop_entry_attribute,
    .get_next_hop_attribute = sai_get_next_hop_entry_attribute};

sai_next_hop_api_t *sai_next_hop_api_get() { return &nhop_api; }

sai_status_t sai_next_hop_initialize() {
  SAI_LOG_DEBUG("Initializing nexthop");
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_NEXT_HOP);
  return SAI_STATUS_SUCCESS;
}
