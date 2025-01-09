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
#include <unordered_map>
#include <set>

#include "s3/switch_store.h"

static sai_api_t api_id = SAI_API_BRIDGE;
static switch_object_id_t device_handle = {0};

/* This handle is only used to refer to 1Q bridge.  */
static switch_object_id_t DEFAULT_BRIDGE_1Q = {0};
std::vector<switch_object_id_t> dot1q_bridgeport;

// This is necessary so that we don't keep going down to SMI to fetch this data
// and also because this is not stored directly as an attribute in the SMI
// schema
std::unordered_map<uint64_t, sai_object_id_t> port_to_bridge_port_map;
std::unordered_map<uint64_t, sai_object_id_t> tunnel_to_bridge_port_map;

sai_status_t sai_get_port_to_bridge_port(const switch_object_id_t port_handle,
                                         sai_object_id_t &bridge_port_id) {
  const auto itr = port_to_bridge_port_map.find(port_handle.data);
  if (itr != port_to_bridge_port_map.end()) {
    bridge_port_id = itr->second;
    return SAI_STATUS_SUCCESS;
  }
  return SAI_STATUS_INVALID_OBJECT_ID;
}

sai_status_t sai_get_tunnel_to_bridge_port(
    const switch_object_id_t tunnel_handle, sai_object_id_t &bridge_port_id) {
  const auto itr = tunnel_to_bridge_port_map.find(tunnel_handle.data);
  if (itr != tunnel_to_bridge_port_map.end()) {
    bridge_port_id = itr->second;
    return SAI_STATUS_SUCCESS;
  }
  return SAI_STATUS_INVALID_OBJECT_ID;
}

switch_object_id_t sai_bridge_get_default1q_bridge() {
  return DEFAULT_BRIDGE_1Q;
}

#define SAI_VLAN_ID_MAX 4095
/**
 * @brief Create bridge port
 *
 * @param[out] bridge_port_id Bridge port ID
 * @param[in] switch_id Switch object id
 * @param[in] attr_count number of attributes
 * @param[in] attr_list array of attributes
 *
 * @return #SAI_STATUS_SUCCESS on success Failure status code on error
 */
sai_status_t sai_create_bridge_port(_Out_ sai_object_id_t *bridge_port_id,
                                    _In_ sai_object_id_t switch_id,
                                    _In_ uint32_t attr_count,
                                    _In_ const sai_attribute_t *attr_list) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  const switch_object_type_t ot = SWITCH_OBJECT_TYPE_BRIDGE_PORT;
  sai_bridge_port_type_t sai_bridge_port_type = SAI_BRIDGE_PORT_TYPE_PORT;
  sai_bridge_port_fdb_learning_mode_t fdb_learning_mode =
      SAI_BRIDGE_PORT_FDB_LEARNING_MODE_HW;
  bool admin_state = false;
  switch_object_id_t bridge_handle = {0}, port_handle = {0};
  switch_object_id_t tunnel_handle = {0};
  (void)fdb_learning_mode;

  bridge_handle = sai_bridge_get_default1q_bridge();
  SAI_LOG_ENTER();

  if (!bridge_port_id || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *bridge_port_id = SAI_NULL_OBJECT_ID;

  switch_object_id_t sw_object_id = {};
  std::set<attr_w> sw_attrs;

  for (unsigned index = 0; index < attr_count; index++) {
    switch (attr_list[index].id) {
      case SAI_BRIDGE_PORT_ATTR_TYPE: {
        sai_bridge_port_type =
            (sai_bridge_port_type_t)attr_list[index].value.s32;
        break;
      }
      case SAI_BRIDGE_PORT_ATTR_PORT_ID:
        port_handle.data = attr_list[index].value.oid;
        break;
      case SAI_BRIDGE_PORT_ATTR_TUNNEL_ID:
        tunnel_handle.data = attr_list[index].value.oid;
        break;
      case SAI_BRIDGE_PORT_ATTR_RIF_ID:  // Unsupported
        status = SAI_STATUS_NOT_SUPPORTED;
        SAI_LOG_ERROR("Unsupported bridge port attribute: %s, skipped",
                      sai_attribute_name(SAI_OBJECT_TYPE_BRIDGE_PORT,
                                         attr_list[index].id));
        break;
      case SAI_BRIDGE_PORT_ATTR_BRIDGE_ID:
        bridge_handle.data = attr_list[index].value.oid;
        break;
      case SAI_BRIDGE_PORT_ATTR_FDB_LEARNING_MODE:
        fdb_learning_mode =
            (sai_bridge_port_fdb_learning_mode_t)attr_list[index].value.s32;
        break;
      case SAI_BRIDGE_PORT_ATTR_ADMIN_STATE:
        admin_state = attr_list[index].value.booldata;
        break;
      case SAI_BRIDGE_PORT_ATTR_CUSTOM_0:
        sw_attrs.insert(attr_w(SWITCH_BRIDGE_PORT_ATTR_IS_PEER_LINK,
                               attr_list[index].value.booldata));
        break;
      case SAI_BRIDGE_PORT_ATTR_MAX_LEARNED_ADDRESSES:  // Unsupported
      case SAI_BRIDGE_PORT_ATTR_FDB_LEARNING_LIMIT_VIOLATION_PACKET_ACTION:  // Unsupported
        status = SAI_STATUS_NOT_SUPPORTED;
        SAI_LOG_ERROR("Unsupported bridge port attribute: %s, skipped",
                      sai_attribute_name(SAI_OBJECT_TYPE_BRIDGE_PORT,
                                         attr_list[index].id));
        break;
      default:
        status = sai_to_switch_attribute(
            SAI_OBJECT_TYPE_BRIDGE_PORT, &attr_list[index], sw_attrs);
        break;
    }
  }

  if (bridge_handle.data == 0) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("unexpected attribute : %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  sw_attrs.insert(attr_w(SWITCH_BRIDGE_PORT_ATTR_DEVICE, device_handle));

  switch (sai_bridge_port_type) {
    case SAI_BRIDGE_PORT_TYPE_PORT: {
      /* Must be 1Q bridge */
      if (bridge_handle.data != DEFAULT_BRIDGE_1Q.data) {
        status = SAI_STATUS_INVALID_PARAMETER;
        SAI_LOG_ERROR("Unexpected bridge type; %s",
                      sai_metadata_get_status_name(status));
        return status;
      }
      switch_enum_t e = {.enumdata = SWITCH_BRIDGE_PORT_ATTR_TYPE_PORT};
      sw_attrs.insert(attr_w(SWITCH_BRIDGE_PORT_ATTR_TYPE, e));
      sw_attrs.insert(
          attr_w(SWITCH_BRIDGE_PORT_ATTR_PORT_LAG_HANDLE, port_handle));
      sw_attrs.insert(
          attr_w(SWITCH_BRIDGE_PORT_ATTR_BRIDGE_HANDLE, bridge_handle));
      sw_attrs.insert(attr_w(SWITCH_BRIDGE_PORT_ATTR_ADMIN_STATE, admin_state));
      switch_object_type_t port_lag_ot =
          switch_store::object_type_query(port_handle);

      bool learning;
      if (admin_state == true) {
        learning = (fdb_learning_mode == SAI_BRIDGE_PORT_FDB_LEARNING_MODE_HW)
                       ? true
                       : false;
      } else {
        learning = false;
      }

      if (port_lag_ot == SWITCH_OBJECT_TYPE_PORT) {
        attr_w lr_attr(SWITCH_PORT_ATTR_LEARNING, learning);
        switch_status = bf_switch_attribute_set(port_handle, lr_attr);
      } else if (port_lag_ot == SWITCH_OBJECT_TYPE_LAG) {
        attr_w lr_attr(SWITCH_LAG_ATTR_LEARNING, learning);
        switch_status = bf_switch_attribute_set(port_handle, lr_attr);
      }
    } break;

    case SAI_BRIDGE_PORT_TYPE_TUNNEL: {
      /* Must be 1Q bridge */
      if (bridge_handle.data != DEFAULT_BRIDGE_1Q.data) {
        status = SAI_STATUS_INVALID_PARAMETER;
        SAI_LOG_ERROR("Unexpected bridge type; %s",
                      sai_metadata_get_status_name(status));
        return status;
      }
      if (fdb_learning_mode != SAI_BRIDGE_PORT_FDB_LEARNING_MODE_DISABLE) {
        status = SAI_STATUS_INVALID_PARAMETER;
        SAI_LOG_ERROR("Unexpected learning mode on tunnel bridge port; %s",
                      sai_metadata_get_status_name(status));
        return status;
      }
      switch_enum_t e = {.enumdata = SWITCH_BRIDGE_PORT_ATTR_TYPE_TUNNEL};
      sw_attrs.insert(attr_w(SWITCH_BRIDGE_PORT_ATTR_TYPE, e));
      sw_attrs.insert(
          attr_w(SWITCH_BRIDGE_PORT_ATTR_PORT_LAG_HANDLE, port_handle));
      sw_attrs.insert(
          attr_w(SWITCH_BRIDGE_PORT_ATTR_TUNNEL_HANDLE, tunnel_handle));
      sw_attrs.insert(
          attr_w(SWITCH_BRIDGE_PORT_ATTR_BRIDGE_HANDLE, bridge_handle));
      sw_attrs.insert(attr_w(SWITCH_BRIDGE_PORT_ATTR_ADMIN_STATE, admin_state));
    } break;

    case SAI_BRIDGE_PORT_TYPE_SUB_PORT:   // Unsupported
    case SAI_BRIDGE_PORT_TYPE_1Q_ROUTER:  // Unsupported
    case SAI_BRIDGE_PORT_TYPE_1D_ROUTER:  // Unsupported
    default:
      SAI_LOG_ERROR("Unsupported bridge port type: %d", sai_bridge_port_type);

      return SAI_STATUS_NOT_SUPPORTED;
      break;
  }

  switch_status = bf_switch_object_create(ot, sw_attrs, sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) return status;

  *bridge_port_id = sw_object_id.data;

  if (bridge_handle.data == DEFAULT_BRIDGE_1Q.data) {
    dot1q_bridgeport.push_back(sw_object_id);
  }

  if (sai_bridge_port_type == SAI_BRIDGE_PORT_TYPE_PORT) {
    port_to_bridge_port_map[port_handle.data] = sw_object_id.data;
  } else if (sai_bridge_port_type == SAI_BRIDGE_PORT_TYPE_TUNNEL) {
    tunnel_to_bridge_port_map[tunnel_handle.data] = sw_object_id.data;
  }

  SAI_LOG_EXIT();

  return status;
}

sai_status_t sai_bridge_port_to_handle_and_type(
    switch_object_id_t sw_object_id,
    switch_object_id_t &port_lag_handle,
    switch_object_id_t &tunnel_handle,
    sai_bridge_port_type_t &sai_bridge_port_type) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  switch_enum_t intf_type = {};
  attr_w type_attr(SWITCH_BRIDGE_PORT_ATTR_TYPE);
  attr_w port_attr(SWITCH_BRIDGE_PORT_ATTR_PORT_LAG_HANDLE);
  attr_w tunnel_attr(SWITCH_BRIDGE_PORT_ATTR_TUNNEL_HANDLE);

  switch_status = bf_switch_attribute_get(
      sw_object_id, SWITCH_BRIDGE_PORT_ATTR_TYPE, type_attr);
  type_attr.v_get(intf_type);

  if (intf_type.enumdata == SWITCH_BRIDGE_PORT_ATTR_TYPE_PORT) {
    sai_bridge_port_type = SAI_BRIDGE_PORT_TYPE_PORT;
    switch_status = bf_switch_attribute_get(
        sw_object_id, SWITCH_BRIDGE_PORT_ATTR_PORT_LAG_HANDLE, port_attr);
    port_attr.v_get(port_lag_handle);
  } else if (intf_type.enumdata == SWITCH_BRIDGE_PORT_ATTR_TYPE_TUNNEL) {
    sai_bridge_port_type = SAI_BRIDGE_PORT_TYPE_TUNNEL;
    switch_status = bf_switch_attribute_get(
        sw_object_id, SWITCH_BRIDGE_PORT_ATTR_TUNNEL_HANDLE, tunnel_attr);
    tunnel_attr.v_get(tunnel_handle);
  } else {
    switch_status = SWITCH_STATUS_NOT_SUPPORTED;
  }

  status = status_switch_to_sai(switch_status);
  return status;
}

/**
 * @brief Remove bridge port
 *
 * @param[in] bridge_port_id Bridge port ID
 *
 * @return #SAI_STATUS_SUCCESS on success Failure status code on error
 */
sai_status_t sai_remove_bridge_port(_In_ sai_object_id_t bridge_port_id) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t bridge_id = {0};
  switch_object_id_t sw_object_id = {.data = bridge_port_id};
  sai_bridge_port_type_t sai_bridge_port_type = SAI_BRIDGE_PORT_TYPE_PORT;
  sai_attribute_t attr;

  SAI_LOG_ENTER();

  if (sai_object_type_query(bridge_port_id) != SAI_OBJECT_TYPE_BRIDGE_PORT) {
    SAI_LOG_ERROR("Invalid object 0x%" PRIx64, bridge_port_id);
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  attr.id = SAI_BRIDGE_PORT_ATTR_BRIDGE_ID;
  status = sai_to_switch_attribute_get(
      SAI_OBJECT_TYPE_BRIDGE_PORT, sw_object_id, &attr);
  if (status == SAI_STATUS_SUCCESS) {
    bridge_id.data = attr.value.oid;
  } else {
    bridge_id = sai_bridge_get_default1q_bridge();
  }

  if (bridge_id.data == DEFAULT_BRIDGE_1Q.data) {
    for (auto it = dot1q_bridgeport.begin(); it != dot1q_bridgeport.end();
         it++) {
      if (it->data == sw_object_id.data) {
        dot1q_bridgeport.erase(it);
        break;
      }
    }
  }

  switch_object_id_t port_lag_handle = {}, tunnel_handle = {};
  status = sai_bridge_port_to_handle_and_type(
      sw_object_id, port_lag_handle, tunnel_handle, sai_bridge_port_type);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to get bridge port type %" PRIx64 ": %s",
                  bridge_port_id,
                  sai_metadata_get_status_name(status));
    return status;
  }
  if (sai_bridge_port_type == SAI_BRIDGE_PORT_TYPE_PORT) {
    switch_object_type_t ot = switch_store::object_type_query(port_lag_handle);
    if (ot == SWITCH_OBJECT_TYPE_PORT) {
      attr_w lr_attr(SWITCH_PORT_ATTR_LEARNING, false);
      switch_status = bf_switch_attribute_set(port_lag_handle, lr_attr);
    } else if (ot == SWITCH_OBJECT_TYPE_LAG) {
      attr_w lr_attr(SWITCH_LAG_ATTR_LEARNING, false);
      switch_status = bf_switch_attribute_set(port_lag_handle, lr_attr);
    }
    port_to_bridge_port_map.erase(port_lag_handle.data);
  } else if (sai_bridge_port_type == SAI_BRIDGE_PORT_TYPE_TUNNEL) {
    tunnel_to_bridge_port_map.erase(tunnel_handle.data);
  }

  switch_status = bf_switch_object_delete(sw_object_id);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to remove bridge port handle %" PRIx64 ": %s",
                  bridge_port_id,
                  sai_metadata_get_status_name(status));
    return status;
  }

  SAI_LOG_EXIT();

  return status;
}

/**
 * @brief Set attribute for bridge port
 *
 * @param[in] bridge_port_id Bridge port ID
 * @param[in] attr attribute to set
 *
 * @return #SAI_STATUS_SUCCESS on success Failure status code on error
 */
sai_status_t sai_set_bridge_port_attribute(_In_ sai_object_id_t bridge_port_id,
                                           _In_ const sai_attribute_t *attr) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t sw_object_id = {.data = bridge_port_id};
  sai_bridge_port_type_t sai_bridge_port_type = SAI_BRIDGE_PORT_TYPE_PORT;
  bool learning = false;

  SAI_LOG_ENTER();

  if (!attr) {
    SAI_LOG_ERROR("null attribute : %s", sai_metadata_get_status_name(status));
    return SAI_STATUS_INVALID_PARAMETER;
  }

  switch_object_id_t port_lag_handle = {}, tunnel_handle = {};
  status = sai_bridge_port_to_handle_and_type(
      sw_object_id, port_lag_handle, tunnel_handle, sai_bridge_port_type);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to get bridge port type %" PRIx64 ": %s",
                  bridge_port_id,
                  sai_metadata_get_status_name(status));
    return status;
  }
  switch_object_type_t ot = switch_store::object_type_query(port_lag_handle);

  switch (attr->id) {
    case SAI_BRIDGE_PORT_ATTR_FDB_LEARNING_MODE: {
      // applies only to physical interfaces
      if (sai_bridge_port_type != SAI_BRIDGE_PORT_TYPE_PORT) return status;

      sai_bridge_port_fdb_learning_mode_t fdb_learning_mode =
          (sai_bridge_port_fdb_learning_mode_t)attr->value.s32;
      learning = (fdb_learning_mode == SAI_BRIDGE_PORT_FDB_LEARNING_MODE_HW)
                     ? true
                     : false;
      if (ot == SWITCH_OBJECT_TYPE_PORT) {
        attr_w lr_attr(SWITCH_PORT_ATTR_LEARNING, learning);
        switch_status = bf_switch_attribute_set(port_lag_handle, lr_attr);
      } else if (ot == SWITCH_OBJECT_TYPE_LAG) {
        attr_w lr_attr(SWITCH_LAG_ATTR_LEARNING, learning);
        switch_status = bf_switch_attribute_set(port_lag_handle, lr_attr);
      }
      break;
    }
    case SAI_BRIDGE_PORT_ATTR_ADMIN_STATE: {
      if (attr->value.booldata) {
        // enable learning when admin state is true
        learning = true;
      } else {
        // flush all FDB of this bridge port as per SAI spec, if false
        std::set<switch_object_id_t> mac_oids;
        if (sai_bridge_port_type == SAI_BRIDGE_PORT_TYPE_PORT) {
          switch_status = switch_store::referencing_set_get(
              port_lag_handle, SWITCH_OBJECT_TYPE_MAC_ENTRY, mac_oids);
        } else if (sai_bridge_port_type == SAI_BRIDGE_PORT_TYPE_TUNNEL) {
          switch_status = switch_store::referencing_set_get(
              tunnel_handle, SWITCH_OBJECT_TYPE_MAC_ENTRY, mac_oids);
        } else {
          return status;
        }

        if (switch_status != SWITCH_STATUS_SUCCESS) break;

        for (const auto &mac_handle : mac_oids) {
          // delete only dynamic entries
          attr_w mac_type_attr(SWITCH_MAC_ENTRY_ATTR_TYPE);
          switch_enum_t mac_type = {0};
          switch_status = bf_switch_attribute_get(
              mac_handle, SWITCH_MAC_ENTRY_ATTR_TYPE, mac_type_attr);
          mac_type_attr.v_get(mac_type);
          if (mac_type.enumdata == SWITCH_MAC_ENTRY_ATTR_TYPE_DYNAMIC) {
            switch_status = bf_switch_object_delete(mac_handle);
          }
        }
        // disable learning when admin state is false
        learning = false;
        if (switch_status != SWITCH_STATUS_SUCCESS) break;
      }

      attr_w as_attr(SWITCH_BRIDGE_PORT_ATTR_ADMIN_STATE, attr->value.booldata);
      switch_status = bf_switch_attribute_set(sw_object_id, as_attr);
      if (switch_status != SWITCH_STATUS_SUCCESS) break;

      if (sai_bridge_port_type == SAI_BRIDGE_PORT_TYPE_PORT) {
        if (ot == SWITCH_OBJECT_TYPE_PORT) {
          attr_w lr_attr(SWITCH_PORT_ATTR_LEARNING, learning);
          switch_status = bf_switch_attribute_set(port_lag_handle, lr_attr);
        } else if (ot == SWITCH_OBJECT_TYPE_LAG) {
          attr_w lr_attr(SWITCH_LAG_ATTR_LEARNING, learning);
          switch_status = bf_switch_attribute_set(port_lag_handle, lr_attr);
        }
      }
      break;
    }
    case SAI_BRIDGE_PORT_ATTR_CUSTOM_0: {
      attr_w as_attr(SWITCH_BRIDGE_PORT_ATTR_IS_PEER_LINK,
                     attr->value.booldata);
      switch_status = bf_switch_attribute_set(sw_object_id, as_attr);
      break;
    }
    case SAI_BRIDGE_PORT_ATTR_PORT_ID:
    case SAI_BRIDGE_PORT_ATTR_TUNNEL_ID:
    case SAI_BRIDGE_PORT_ATTR_TYPE:
      SAI_LOG_ERROR("Cannot set create_only attr for bridge port %" PRIx64
                    ": %s",
                    bridge_port_id,
                    sai_metadata_get_status_name(SAI_STATUS_INVALID_PARAMETER));
      return SAI_STATUS_INVALID_PARAMETER;
    default:
      status = sai_to_switch_attribute_set(
          SAI_OBJECT_TYPE_BRIDGE_PORT, attr, sw_object_id);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("Failed to set attribute %s error: %s",
                      sai_attribute_name(SAI_OBJECT_TYPE_BRIDGE_PORT, attr->id),
                      sai_metadata_get_status_name(status));
        return status;
      }
      break;
  }

  status = status_switch_to_sai(switch_status);

  SAI_LOG_EXIT();

  return status;
}

/**
 * @brief Get attributes of bridge port
 *
 * @param[in] bridge_port_id Bridge port ID
 * @param[in] attr_count number of attributes
 * @param[inout] attr_list array of attributes
 *
 * @return #SAI_STATUS_SUCCESS on success Failure status code on error
 */
sai_status_t sai_get_bridge_port_attribute(_In_ sai_object_id_t bridge_port_id,
                                           _In_ uint32_t attr_count,
                                           _Inout_ sai_attribute_t *attr_list) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  sai_attribute_t *sai_attr;
  sai_bridge_port_type_t sai_bridge_port_type = SAI_BRIDGE_PORT_TYPE_PORT;
  uint32_t i = 0;

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  SAI_ASSERT(sai_object_type_query(bridge_port_id) ==
             SAI_OBJECT_TYPE_BRIDGE_PORT);
  const switch_object_id_t sw_object_id = {.data = bridge_port_id};

  switch_object_id_t port_lag_handle = {}, tunnel_handle = {};
  status = sai_bridge_port_to_handle_and_type(
      sw_object_id, port_lag_handle, tunnel_handle, sai_bridge_port_type);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to get bridge port type %" PRIx64 ": %s",
                  bridge_port_id,
                  sai_metadata_get_status_name(status));
    return status;
  }
  switch_object_type_t ot = switch_store::object_type_query(port_lag_handle);

  for (i = 0, sai_attr = attr_list; i < attr_count; i++, sai_attr++) {
    switch (sai_attr->id) {
      case SAI_BRIDGE_PORT_ATTR_FDB_LEARNING_MODE: {
        bool learning = false;
        if (sai_bridge_port_type == SAI_BRIDGE_PORT_TYPE_PORT) {
          if (ot == SWITCH_OBJECT_TYPE_PORT) {
            attr_w lr_attr(SWITCH_PORT_ATTR_LEARNING);
            switch_status = bf_switch_attribute_get(
                port_lag_handle, SWITCH_PORT_ATTR_LEARNING, lr_attr);
            lr_attr.v_get(learning);
          } else if (ot == SWITCH_OBJECT_TYPE_LAG) {
            attr_w lr_attr(SWITCH_LAG_ATTR_LEARNING);
            switch_status = bf_switch_attribute_get(
                port_lag_handle, SWITCH_LAG_ATTR_LEARNING, lr_attr);
            lr_attr.v_get(learning);
          }
        }
        sai_attr->value.s32 = learning
                                  ? SAI_BRIDGE_PORT_FDB_LEARNING_MODE_HW
                                  : SAI_BRIDGE_PORT_FDB_LEARNING_MODE_DISABLE;
        status = status_switch_to_sai(switch_status);
      } break;
      case SAI_BRIDGE_PORT_ATTR_CUSTOM_0: {
        attr_w is_peer_link_attr(SWITCH_BRIDGE_PORT_ATTR_IS_PEER_LINK);
        switch_status =
            bf_switch_attribute_get(sw_object_id,
                                    SWITCH_BRIDGE_PORT_ATTR_IS_PEER_LINK,
                                    is_peer_link_attr);
        status = status_switch_to_sai(switch_status);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR("failed to bridge port 0x%" PRIx64
                        "is_peer_link attr: %s",
                        bridge_port_id,
                        sai_metadata_get_status_name(status));
          return status;
        }
        bool is_peer_link = false;
        is_peer_link_attr.v_get(is_peer_link);

        attr_list[i].value.booldata = is_peer_link;
      } break;
      default:
        status = sai_to_switch_attribute_get(
            SAI_OBJECT_TYPE_BRIDGE_PORT, sw_object_id, sai_attr);
        if (status != SAI_STATUS_SUCCESS) {
          SAI_LOG_ERROR(
              "Failed to get bridge port attribute %s: %" PRIx64 "error: %s",
              sai_attribute_name(SAI_OBJECT_TYPE_BRIDGE_PORT, sai_attr->id),
              bridge_port_id,
              sai_metadata_get_status_name(status));
          return status;
        }
        break;
    }
  }
  SAI_LOG_EXIT();
  return status;
}

/**
 * @brief Create bridge
 *
 * @param[out] bridge_id Bridge ID
 * @param[in] switch_id Switch object id
 * @param[in] attr_count number of attributes
 * @param[in] attr_list array of attributes
 *
 * @return #SAI_STATUS_SUCCESS on success Failure status code on error
 */
sai_status_t sai_create_bridge(_Out_ sai_object_id_t *bridge_id,
                               _In_ sai_object_id_t switch_id,
                               _In_ uint32_t attr_count,
                               _In_ const sai_attribute_t *attr_list) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  const sai_attribute_t *attribute;
  sai_bridge_type_t sai_bridge_type;
  sai_uint32_t max_learned_addresses = 0;
  // bool learning_disable = false;

  SAI_LOG_ENTER();

  if (!bridge_id || !attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  *bridge_id = SAI_NULL_OBJECT_ID;

  attribute =
      sai_get_attr_from_list(SAI_BRIDGE_ATTR_TYPE, attr_list, attr_count);
  if (attribute == NULL) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("missing attribute %s", sai_metadata_get_status_name(status));
    return status;
  }
  sai_bridge_type = (sai_bridge_type_t)attribute->value.s32;

  for (unsigned index = 0; index < attr_count; index++) {
    switch (attr_list[index].id) {
      case SAI_BRIDGE_ATTR_TYPE:
        break;
      case SAI_BRIDGE_ATTR_MAX_LEARNED_ADDRESSES:
        max_learned_addresses = attr_list[index].value.u32;
        (void)max_learned_addresses;
        // ToDo: add support
        //        return SAI_STATUS_NOT_SUPPORTED;
        break;
      case SAI_BRIDGE_ATTR_LEARN_DISABLE:
        if (sai_bridge_type == SAI_BRIDGE_TYPE_1Q) {
          //          return SAI_STATUS_NOT_SUPPORTED;
        }
        // learning_disable = attr_list[index].value.booldata;
        break;
      default:
        //        return SAI_STATUS_NOT_SUPPORTED;
        break;
    }
  }

  if (sai_bridge_type == SAI_BRIDGE_TYPE_1Q) {
    *bridge_id = DEFAULT_BRIDGE_1Q.data;
  } else {
    status = SAI_STATUS_NOT_SUPPORTED;
  }
  SAI_LOG_EXIT();

  return status;
}

/**
 * @brief Remove bridge
 *
 * @param[in] bridge_id Bridge ID
 *
 * @return #SAI_STATUS_SUCCESS on success Failure status code on error
 */
sai_status_t sai_remove_bridge(_In_ sai_object_id_t bridge_id) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t bridge_handle = {.data = bridge_id};

  SAI_LOG_ENTER();

  if (bridge_id == DEFAULT_BRIDGE_1Q.data) {
    return SAI_STATUS_NOT_SUPPORTED;
  } else {
    switch_status = bf_switch_object_delete(bridge_handle);
    if ((status = status_switch_to_sai(switch_status)) != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("failed to delete bridge: %s",
                    sai_metadata_get_status_name(status));
      return status;
    }
  }
  SAI_LOG_EXIT();

  return status;
}

/**
 * @brief Set attribute for bridge
 *
 * @param[in] bridge_id Bridge ID
 * @param[in] attr attribute to set
 *
 * @return #SAI_STATUS_SUCCESS on success Failure status code on error
 */
sai_status_t sai_set_bridge_attribute(_In_ sai_object_id_t bridge_id,
                                      _In_ const sai_attribute_t *attr) {
  sai_status_t status = SAI_STATUS_SUCCESS;

  SAI_LOG_ENTER();

  if (!attr) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute : %s", sai_metadata_get_status_name(status));
    return status;
  }

  status = SAI_STATUS_NOT_SUPPORTED;
  SAI_LOG_EXIT();

  return status;
}

/**
 * @brief Get bridge statistics counters.
 *
 * @param[in] bridge_id Bridge id
 * @param[in] number_of_counters Number of counters in the array
 * @param[in] counter_ids Specifies the array of counter ids
 * @param[out] counters Array of resulting counter values.
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_get_bridge_stats(_In_ sai_object_id_t bridge_id,
                                  _In_ uint32_t number_of_counters,
                                  _In_ const sai_stat_id_t *counter_ids,
                                  _Out_ uint64_t *counters) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  std::vector<switch_counter_t> bridge_cntrs;

  if (sai_object_type_query(bridge_id) != SAI_OBJECT_TYPE_BRIDGE) {
    SAI_LOG_ERROR("Invalid object 0x%" PRIx64, bridge_id);
    return SAI_STATUS_INVALID_OBJECT_TYPE;
  }

  if (!counter_ids || !counters) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  const switch_object_id_t sw_bridge_object_id = {.data = bridge_id};
  switch_status = bf_switch_counters_get(sw_bridge_object_id, bridge_cntrs);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to get bridge stats bridge_id: %" PRIx64 ": %s",
                  bridge_id,
                  sai_metadata_get_status_name(status));
    return status;
  }

  for (uint32_t index = 0; index < number_of_counters; index++) {
    switch (counter_ids[index]) {
      case SAI_BRIDGE_STAT_IN_OCTETS:
        counters[index] =
            bridge_cntrs[SWITCH_BRIDGE_COUNTER_ID_IN_UCAST_BYTES].count +
            bridge_cntrs[SWITCH_BRIDGE_COUNTER_ID_IN_MCAST_BYTES].count +
            bridge_cntrs[SWITCH_BRIDGE_COUNTER_ID_IN_BCAST_BYTES].count;
        break;
      case SAI_BRIDGE_STAT_OUT_OCTETS:
        counters[index] =
            bridge_cntrs[SWITCH_BRIDGE_COUNTER_ID_OUT_UCAST_BYTES].count +
            bridge_cntrs[SWITCH_BRIDGE_COUNTER_ID_OUT_MCAST_BYTES].count +
            bridge_cntrs[SWITCH_BRIDGE_COUNTER_ID_OUT_BCAST_BYTES].count;
        break;
      case SAI_BRIDGE_STAT_IN_PACKETS:
        counters[index] =
            bridge_cntrs[SWITCH_BRIDGE_COUNTER_ID_IN_UCAST_PKTS].count +
            bridge_cntrs[SWITCH_BRIDGE_COUNTER_ID_IN_MCAST_PKTS].count +
            bridge_cntrs[SWITCH_BRIDGE_COUNTER_ID_IN_BCAST_PKTS].count;
        break;
      case SAI_BRIDGE_STAT_OUT_PACKETS:
        counters[index] =
            bridge_cntrs[SWITCH_BRIDGE_COUNTER_ID_OUT_UCAST_PKTS].count +
            bridge_cntrs[SWITCH_BRIDGE_COUNTER_ID_OUT_MCAST_PKTS].count +
            bridge_cntrs[SWITCH_BRIDGE_COUNTER_ID_OUT_BCAST_PKTS].count;
        break;
      default:
        counters[index] = 0;
        break;
    }
  }

  SAI_LOG_EXIT();

  return (sai_status_t)status;
}

/**
 * @brief Clear bridge statistics counters.
 *
 * @param[in] bridge_id Bridge id
 * @param[in] number_of_counters Number of counters in the array
 * @param[in] counter_ids Specifies the array of counter ids
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_clear_bridge_stats(_In_ sai_object_id_t bridge_id,
                                    _In_ uint32_t number_of_counters,
                                    _In_ const sai_stat_id_t *counter_ids) {
  SAI_LOG_ENTER();

  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  std::vector<uint16_t> cntr_ids;

  if (!counter_ids) {
    SAI_LOG_ERROR("Bridge stats clear failed: null pointer argument");
    return SAI_STATUS_INVALID_PARAMETER;
  }

  if (sai_object_type_query(bridge_id) != SAI_OBJECT_TYPE_BRIDGE) {
    SAI_LOG_ERROR("Bridge stats clear failed: invalid bridge handle 0x%" PRIx64,
                  bridge_id);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  const switch_object_id_t sw_object_id = {.data = bridge_id};

  for (uint32_t i = 0; i < number_of_counters; i++) {
    switch (counter_ids[i]) {
      case SAI_BRIDGE_STAT_IN_OCTETS:
        cntr_ids.push_back(SWITCH_BRIDGE_COUNTER_ID_IN_UCAST_BYTES);
        cntr_ids.push_back(SWITCH_BRIDGE_COUNTER_ID_IN_MCAST_BYTES);
        cntr_ids.push_back(SWITCH_BRIDGE_COUNTER_ID_IN_BCAST_BYTES);
        break;
      case SAI_BRIDGE_STAT_OUT_OCTETS:
        cntr_ids.push_back(SWITCH_BRIDGE_COUNTER_ID_OUT_UCAST_BYTES);
        cntr_ids.push_back(SWITCH_BRIDGE_COUNTER_ID_OUT_MCAST_BYTES);
        cntr_ids.push_back(SWITCH_BRIDGE_COUNTER_ID_OUT_BCAST_BYTES);
        break;
      case SAI_BRIDGE_STAT_IN_PACKETS:
        cntr_ids.push_back(SWITCH_BRIDGE_COUNTER_ID_IN_UCAST_PKTS);
        cntr_ids.push_back(SWITCH_BRIDGE_COUNTER_ID_IN_MCAST_PKTS);
        cntr_ids.push_back(SWITCH_BRIDGE_COUNTER_ID_IN_BCAST_PKTS);
        break;
      case SAI_BRIDGE_STAT_OUT_PACKETS:
        cntr_ids.push_back(SWITCH_BRIDGE_COUNTER_ID_OUT_UCAST_PKTS);
        cntr_ids.push_back(SWITCH_BRIDGE_COUNTER_ID_OUT_MCAST_PKTS);
        cntr_ids.push_back(SWITCH_BRIDGE_COUNTER_ID_OUT_BCAST_PKTS);
        break;
      default:
        break;
    }
  }

  if (cntr_ids.size()) {
    switch_status = bf_switch_counters_clear(sw_object_id, cntr_ids);
    status = status_switch_to_sai(switch_status);
    if (status != SAI_STATUS_SUCCESS) {
      SAI_LOG_ERROR("Failed to clear rif 0x%" PRIx64 " stats, status %s",
                    bridge_id,
                    sai_metadata_get_status_name(status));
    }
  }

  SAI_LOG_EXIT();

  return status;
}

/**
 * @brief Get attributes of bridge
 *
 * @param[in] bridge_id Bridge ID
 * @param[in] attr_count number of attributes
 * @param[inout] attr_list array of attributes
 *
 * @return #SAI_STATUS_SUCCESS on success Failure status code on error
 */
sai_status_t sai_get_bridge_attribute(_In_ sai_object_id_t bridge_id,
                                      _In_ uint32_t attr_count,
                                      _Inout_ sai_attribute_t *attr_list) {
  sai_status_t status = SAI_STATUS_SUCCESS;

  SAI_LOG_ENTER();

  if (!attr_list) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("null attribute list: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }
  for (unsigned int i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      case SAI_BRIDGE_ATTR_TYPE:
        break;
      case SAI_BRIDGE_ATTR_PORT_LIST:
        if (bridge_id == DEFAULT_BRIDGE_1Q.data) {
          TRY_LIST_SET(attr_list[i].value.objlist, dot1q_bridgeport);
        }
        break;
      case SAI_BRIDGE_ATTR_MAX_LEARNED_ADDRESSES:          // Unsupported
      case SAI_BRIDGE_ATTR_LEARN_DISABLE:                  // Unsupported
      case SAI_BRIDGE_ATTR_UNKNOWN_UNICAST_FLOOD_GROUP:    // Unsupported
      case SAI_BRIDGE_ATTR_UNKNOWN_MULTICAST_FLOOD_GROUP:  // Unsupported
      case SAI_BRIDGE_ATTR_BROADCAST_FLOOD_GROUP:          // Unsupported
        status = SAI_STATUS_NOT_SUPPORTED;
        break;
      default:
        SAI_LOG_ERROR(
            "Failed to get bridge attribute %s",
            sai_attribute_name(SAI_OBJECT_TYPE_BRIDGE, attr_list[i].id));
        status = SAI_STATUS_NOT_SUPPORTED;
        break;
    }
  }

  SAI_LOG_EXIT();
  return status;
}

/**
 * @brief Get bridge port statistics counters.
 *
 * @param[in] bridge_port_id Bridge port id
 * @param[in] number_of_counters Number of counters in the array
 * @param[in] counter_ids Specifies the array of counter ids
 * @param[out] counters Array of resulting counter values.
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_get_bridge_port_stats(_In_ sai_object_id_t bridge_port_id,
                                       _In_ uint32_t number_of_counters,
                                       _In_ const sai_stat_id_t *counter_ids,
                                       _Out_ uint64_t *counters) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  switch_object_id_t sw_object_id = {.data = bridge_port_id};
  switch_object_id_t port_lag_handle = {0};
  attr_w attr(SWITCH_BRIDGE_PORT_ATTR_PORT_LAG_HANDLE);

  if (!counter_ids || !counters) {
    status = SAI_STATUS_INVALID_PARAMETER;
    SAI_LOG_ERROR("Null parameter passed: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  switch_status = bf_switch_attribute_get(
      sw_object_id, SWITCH_BRIDGE_PORT_ATTR_PORT_LAG_HANDLE, attr);
  attr.v_get(port_lag_handle);
  switch_object_type_t ot = switch_store::object_type_query(port_lag_handle);

  std::vector<switch_counter_t> cntrs;
  switch_status = bf_switch_counters_get(port_lag_handle, cntrs);
  status = status_switch_to_sai(switch_status);

  if (ot == SWITCH_OBJECT_TYPE_PORT) {
    for (uint32_t i = 0; i < number_of_counters; i++) {
      switch (counter_ids[i]) {
        case SAI_BRIDGE_PORT_STAT_IN_OCTETS:
          counters[i] = cntrs[SWITCH_PORT_COUNTER_ID_IN_ALL_OCTETS].count;
          break;
        case SAI_BRIDGE_PORT_STAT_IN_PACKETS:
          counters[i] = cntrs[SWITCH_PORT_COUNTER_ID_IN_ALL_PKTS].count;
          break;
        case SAI_BRIDGE_PORT_STAT_OUT_OCTETS:
          counters[i] = cntrs[SWITCH_PORT_COUNTER_ID_OUT_ALL_OCTETS].count;
          break;
        case SAI_BRIDGE_PORT_STAT_OUT_PACKETS:
          counters[i] = cntrs[SWITCH_PORT_COUNTER_ID_OUT_ALL_PKTS].count;
          break;
        default:
          break;
      }
    }
  } else if (ot == SWITCH_OBJECT_TYPE_LAG) {
    for (uint32_t i = 0; i < number_of_counters; i++) {
      switch (counter_ids[i]) {
        case SAI_BRIDGE_PORT_STAT_IN_OCTETS:
          counters[i] = cntrs[SWITCH_LAG_COUNTER_ID_IN_OCTETS].count;
          break;
        case SAI_BRIDGE_PORT_STAT_IN_PACKETS:
          counters[i] = cntrs[SWITCH_LAG_COUNTER_ID_IN_PACKETS].count;
          break;
        case SAI_BRIDGE_PORT_STAT_OUT_OCTETS:
          counters[i] = cntrs[SWITCH_LAG_COUNTER_ID_OUT_OCTETS].count;
          break;
        case SAI_BRIDGE_PORT_STAT_OUT_PACKETS:
          counters[i] = cntrs[SWITCH_LAG_COUNTER_ID_OUT_PACKETS].count;
          break;
        default:
          break;
      }
    }
  }

  return status;
}

/**
 * @brief Clear bridge port statistics counters.
 *
 * @param[in] bridge_port_id Bridge port id
 * @param[in] number_of_counters Number of counters in the array
 * @param[in] counter_ids Specifies the array of counter ids
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_clear_bridge_port_stats(
    _In_ sai_object_id_t bridge_port_id,
    _In_ uint32_t number_of_counters,
    _In_ const sai_stat_id_t *counter_ids) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  SAI_LOG_ENTER();
  SAI_LOG_EXIT();
  return (sai_status_t)status;
}

sai_bridge_api_t bridge_api = {
  create_bridge : sai_create_bridge,
  remove_bridge : sai_remove_bridge,
  set_bridge_attribute : sai_set_bridge_attribute,
  get_bridge_attribute : sai_get_bridge_attribute,
  get_bridge_stats : sai_get_bridge_stats,
  get_bridge_stats_ext : NULL,
  clear_bridge_stats : sai_clear_bridge_stats,
  create_bridge_port : sai_create_bridge_port,
  remove_bridge_port : sai_remove_bridge_port,
  set_bridge_port_attribute : sai_set_bridge_port_attribute,
  get_bridge_port_attribute : sai_get_bridge_port_attribute,
  get_bridge_port_stats : sai_get_bridge_port_stats,
  get_bridge_port_stats_ext : NULL,
  clear_bridge_port_stats : sai_clear_bridge_port_stats
};

sai_bridge_api_t *sai_bridge_api_get() { return &bridge_api; }

sai_status_t sai_bridge_initialize(bool warm_init) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  SAI_LOG_DEBUG("Initializing bridge");
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_BRIDGE);
  bf_sai_add_object_type_to_supported_list(SAI_OBJECT_TYPE_BRIDGE_PORT);
  device_handle = sai_get_device_id(0);

  if (warm_init) {
    // get default 1q bridge from device object
    attr_w dev_attr(SWITCH_DEVICE_ATTR_DEFAULT1Q_BRIDGE);
    switch_status = bf_switch_attribute_get(
        device_handle, SWITCH_DEVICE_ATTR_DEFAULT1Q_BRIDGE, dev_attr);
    dev_attr.v_get(DEFAULT_BRIDGE_1Q);

    uint32_t num_intf = 0;
    (void)num_intf;
    switch_object_id_t first_handle = {0};
    std::vector<switch_object_id_t> handles_list;
    switch_status = bf_switch_get_first_handle(SWITCH_OBJECT_TYPE_BRIDGE_PORT,
                                               first_handle);
    if (switch_status == SWITCH_STATUS_ITEM_NOT_FOUND) {
      return SAI_STATUS_SUCCESS;
    }
    switch_status =
        bf_switch_get_next_handles(first_handle, 512, handles_list, num_intf);
    handles_list.push_back(first_handle);
    for (const auto &handle : handles_list) {
      dot1q_bridgeport.push_back(handle);
      switch_object_id_t port_lag_handle = {};
      attr_w port_lag_attr(SWITCH_BRIDGE_PORT_ATTR_PORT_LAG_HANDLE);
      switch_status = bf_switch_attribute_get(
          handle, SWITCH_BRIDGE_PORT_ATTR_PORT_LAG_HANDLE, port_lag_attr);
      status = status_switch_to_sai(switch_status);
      if (status != SAI_STATUS_SUCCESS) {
        SAI_LOG_ERROR("failed to get port for bridge port : 0x%" PRIx64 " %s",
                      handle.data,
                      sai_metadata_get_status_name(status));
        return status;
      }
      port_lag_attr.v_get(port_lag_handle);
      // update the global port_to_bridge_port_map
      port_to_bridge_port_map[port_lag_handle.data] = handle.data;
    }
    return status;
  }

  // Create default 1Q bridge
  std::set<attr_w> bridge_attrs;
  switch_enum_t bridge_type = {.enumdata = SWITCH_BRIDGE_ATTR_TYPE_DOT1Q};
  bridge_attrs.insert(attr_w(SWITCH_BRIDGE_ATTR_DEVICE, device_handle));
  bridge_attrs.insert(attr_w(SWITCH_BRIDGE_ATTR_TYPE, bridge_type));
  switch_status = bf_switch_object_create(
      SWITCH_OBJECT_TYPE_BRIDGE, bridge_attrs, DEFAULT_BRIDGE_1Q);
  if ((status = status_switch_to_sai(switch_status)) != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to initialize bridge: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  attr_w dev_attr(SWITCH_DEVICE_ATTR_DEFAULT1Q_BRIDGE);
  dev_attr.v_set(DEFAULT_BRIDGE_1Q);
  switch_status = bf_switch_attribute_set(device_handle, dev_attr);
  if ((status = status_switch_to_sai(switch_status)) != SAI_STATUS_SUCCESS) {
    SAI_LOG_ERROR("failed to initialize bridge: %s",
                  sai_metadata_get_status_name(status));
    return status;
  }

  return SAI_STATUS_SUCCESS;
}
