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

#include <algorithm>
#include <set>

sai_status_t sai_get_device_acl_entry_priority(const uint16_t dev_id,
                                               bool min,
                                               uint32_t &acl_prio) {
  switch_object_id_t device_handle = sai_get_device_id(dev_id);
  sai_status_t status;
  switch_status_t switch_status;

  if (min) {
    attr_w acl_min_prio_attr(SWITCH_DEVICE_ATTR_ACL_ENTRY_MIN_PRIORITY);
    switch_status =
        bf_switch_attribute_get(device_handle,
                                SWITCH_DEVICE_ATTR_ACL_ENTRY_MIN_PRIORITY,
                                acl_min_prio_attr);
    acl_min_prio_attr.v_get(acl_prio);
    if ((status = status_switch_to_sai(switch_status)) != SAI_STATUS_SUCCESS) {
      SAI_GENERIC_ERROR("failed to get default min acl_priority: %s",
                        sai_metadata_get_status_name(status));
      return status;
    }
  } else {
    attr_w acl_min_prio_attr(SWITCH_DEVICE_ATTR_ACL_ENTRY_MAX_PRIORITY);
    switch_status =
        bf_switch_attribute_get(device_handle,
                                SWITCH_DEVICE_ATTR_ACL_ENTRY_MAX_PRIORITY,
                                acl_min_prio_attr);
    acl_min_prio_attr.v_get(acl_prio);
    if ((status = status_switch_to_sai(switch_status)) != SAI_STATUS_SUCCESS) {
      SAI_GENERIC_ERROR("failed to get default max acl_priority: %s",
                        sai_metadata_get_status_name(status));
      return status;
    }
  }

  return SAI_STATUS_SUCCESS;
}

uint32_t sai_acl_priority_to_switch_priority(sai_uint32_t sai_acl_priority) {
  uint32_t sw_min_priority;
  uint32_t sw_max_priority;

  sai_get_device_acl_entry_priority(0, true, sw_min_priority);
  sai_get_device_acl_entry_priority(0, false, sw_max_priority);

  if (sai_acl_priority < sw_min_priority ||
      sai_acl_priority > sw_max_priority) {
    SAI_GENERIC_DEBUG("ACL entry priority is invalid %d", sai_acl_priority);
  }
  // In driver, lower the priority_value, higher the priority.
  // So, convert the SAI priority value.
  if (sai_acl_priority <= sw_min_priority) {
    SAI_GENERIC_DEBUG(
        "ACL entry priority is less than min priority, return max value");
    return (sw_max_priority + (sw_min_priority - sai_acl_priority));
  }
  if (sai_acl_priority >= sw_max_priority) {
    SAI_GENERIC_DEBUG("ACL entry priority is maximum, return min value");
    return sw_min_priority;
  }
  return ((sw_max_priority - sai_acl_priority) + sw_min_priority);
}

sai_uint32_t switch_priority_to_sai_acl_priority(uint32_t switch_priority) {
  const bool is_min = true;
  const uint16_t dev_id = 0;

  uint32_t switch_min_priority;
  uint32_t switch_max_priority;

  sai_get_device_acl_entry_priority(dev_id, is_min, switch_min_priority);
  sai_get_device_acl_entry_priority(dev_id, !is_min, switch_max_priority);

  // to avoid overflow, limit switch_priority
  // between [switch_min_priority ; switch_max_priority]
  switch_priority = std::max(switch_priority, switch_min_priority);
  switch_priority = std::min(switch_priority, switch_max_priority);

  return static_cast<sai_uint32_t>(switch_max_priority - switch_priority +
                                   switch_min_priority);
}

#define SWITCH_HOSTIF_ACL_MIN 0
#define SWITCH_HOSTIF_ACL_MAX 0x7FFFFF  // 23-bit hostif trap max

uint32_t sai_hostif_priority_to_switch_hostif_priority(
    sai_uint32_t sai_hostif_priority) {
  uint32_t sw_min_priority = SWITCH_HOSTIF_ACL_MIN;
  uint32_t sw_max_priority = SWITCH_HOSTIF_ACL_MAX;

  if (sai_hostif_priority < sw_min_priority ||
      sai_hostif_priority > sw_max_priority) {
    SAI_GENERIC_DEBUG("SAI hotif priority is invalid %d", sai_hostif_priority);
  }
  // In driver, lower the priority_value, higher the priority.
  // So, convert the SAI priority value.
  if (sai_hostif_priority <= sw_min_priority) {
    return sw_max_priority;
  }
  if (sai_hostif_priority >= sw_max_priority) {
    return sw_min_priority;
  }
  return (sw_max_priority - sai_hostif_priority);
}

sai_uint32_t switch_hostif_priority_to_sai_hostif_priority(
    uint32_t switch_hostif_priority) {
  uint32_t sw_min_priority = SWITCH_HOSTIF_ACL_MIN;
  uint32_t sw_max_priority = SWITCH_HOSTIF_ACL_MAX;

  // to avoid overflow, limit switch_hostif_priority
  // between [sw_min_priority ; sw_max_priority]
  switch_hostif_priority = std::max(switch_hostif_priority, sw_min_priority);
  switch_hostif_priority = std::min(switch_hostif_priority, sw_max_priority);

  return static_cast<sai_uint32_t>(sw_max_priority - switch_hostif_priority);
}

sai_status_t sai_get_port_from_bridge_port(
    const sai_object_id_t bridge_port_id, switch_object_id_t &port_lag_handle) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  switch_object_id_t interface_handle = {.data = bridge_port_id};
  attr_w port_attr(SWITCH_BRIDGE_PORT_ATTR_PORT_LAG_HANDLE);
  switch_status = bf_switch_attribute_get(
      interface_handle, SWITCH_BRIDGE_PORT_ATTR_PORT_LAG_HANDLE, port_attr);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_GENERIC_ERROR("failed to get port_handle, bridge_port_id 0x%" PRIx64
                      ": %s",
                      bridge_port_id,
                      sai_metadata_get_status_name(status));
    return status;
  }
  port_attr.v_get(port_lag_handle);

  return status;
}

sai_status_t sai_get_tunnel_from_bridge_port(
    const sai_object_id_t bridge_port_id, switch_object_id_t &tunnel_handle) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  switch_object_id_t interface_handle = {.data = bridge_port_id};
  attr_w tunnel_attr(SWITCH_BRIDGE_PORT_ATTR_TUNNEL_HANDLE);
  switch_status = bf_switch_attribute_get(
      interface_handle, SWITCH_BRIDGE_PORT_ATTR_TUNNEL_HANDLE, tunnel_attr);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_GENERIC_ERROR("failed to get tunnel_handle, bridge_port_id 0x%" PRIx64
                      ": %s",
                      bridge_port_id,
                      sai_metadata_get_status_name(status));
    return status;
  }
  tunnel_attr.v_get(tunnel_handle);

  return status;
}

sai_status_t sai_get_bridge_port_interface_type(
    const sai_object_id_t bridge_port_id, switch_enum_t &intf_type) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  switch_object_id_t intf_handle = {.data = bridge_port_id};
  attr_w type_attr(SWITCH_BRIDGE_PORT_ATTR_TYPE);
  switch_status = bf_switch_attribute_get(
      intf_handle, SWITCH_BRIDGE_PORT_ATTR_TYPE, type_attr);
  status = status_switch_to_sai(switch_status);
  if (status != SAI_STATUS_SUCCESS) {
    SAI_GENERIC_ERROR("Failed to get interface type, bridge_port_id 0x%" PRIx64
                      ": %s",
                      bridge_port_id,
                      sai_metadata_get_status_name(status));
    return status;
  }

  type_attr.v_get(intf_type);

  return status;
}

void sai_ipv4_prefix_length(sai_ip4_t ip4, uint16_t *prefix_length) {
  int x = 0;
  *prefix_length = 0;
  while (ip4) {
    x = ip4 & 0x1;
    if (x) (*prefix_length)++;
    ip4 = ip4 >> 1;
  }
}

void sai_ipv6_prefix_length(const sai_ip6_t ip6, uint16_t *prefix_length) {
  int i = 0, x = 0;
  sai_ip6_t ip6_temp;
  memcpy(ip6_temp, ip6, 16);
  *prefix_length = 0;
  for (i = 0; i < 16; i++) {
    if (ip6_temp[i] == 0xFF) {
      *prefix_length += 8;
    } else {
      while (ip6_temp[i]) {
        x = ip6_temp[i] & 0x1;
        if (x) (*prefix_length)++;
        ip6_temp[i] = ip6_temp[i] >> 1;
      }
    }
  }
}

void sai_ipv4_to_switch_ip_addr(const sai_ip4_t ip4,
                                switch_ip_address_t &switch_ip_addr) {
  switch_ip_addr.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
  switch_ip_addr.ip4 = ntohl(ip4);
}

void sai_ipv6_to_switch_ip_addr(const sai_ip6_t ip6,
                                switch_ip_address_t &switch_ip_addr) {
  switch_ip_addr.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6;
  memcpy(switch_ip_addr.ip6, ip6, IPV6_ADDR_LEN);
}

void switch_ip_addr_to_sai_ipv4(sai_ip4_t &ip4,
                                const switch_ip_address_t &switch_ip_addr) {
  ip4 = htonl(ip4);
}

void switch_ip_addr_to_sai_ipv6(sai_ip6_t &ip6,
                                const switch_ip_address_t &switch_ip_addr) {
  memcpy(ip6, switch_ip_addr.ip6, IPV6_ADDR_LEN);
}

void sai_ip_addr_to_switch_ip_addr(const sai_ip_address_t *sai_ip_addr,
                                   switch_ip_address_t &switch_ip_addr) {
  if (sai_ip_addr->addr_family == SAI_IP_ADDR_FAMILY_IPV4) {
    sai_ipv4_to_switch_ip_addr(sai_ip_addr->addr.ip4, switch_ip_addr);
  } else if (sai_ip_addr->addr_family == SAI_IP_ADDR_FAMILY_IPV6) {
    sai_ipv6_to_switch_ip_addr(sai_ip_addr->addr.ip6, switch_ip_addr);
  }
}

// Most common usage of link-local format is FE80::XXXX:XXFF:FEXX.XXXX
// But just checking for the entire link-local range i.e. FE80::/10
bool switch_ipv6_prefix_link_local_host_ip(
    const switch_ip_prefix_t &ip_prefix) {
  if ((ip_prefix.addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6) &&
      (ip_prefix.len == SWITCH_IPV6_MAX_PREFIX_LEN)) {
    if ((ip_prefix.addr.ip6[0] == 0xFE) &&
        ((ip_prefix.addr.ip6[1] & 0xC0) == 0x80)) {
      return true;
    }
  }
  return false;
}

void sai_ip_prefix_to_switch_ip_prefix(const sai_ip_prefix_t *sai_ip_prefix,
                                       switch_ip_prefix_t &switch_ip_prefix) {
  if (sai_ip_prefix->addr_family == SAI_IP_ADDR_FAMILY_IPV4) {
    switch_ip_prefix.addr.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
    switch_ip_prefix.addr.ip4 = ntohl(sai_ip_prefix->addr.ip4);
    sai_ipv4_prefix_length(ntohl(sai_ip_prefix->mask.ip4),
                           &switch_ip_prefix.len);
  } else if (sai_ip_prefix->addr_family == SAI_IP_ADDR_FAMILY_IPV6) {
    switch_ip_prefix.addr.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6;
    memcpy(switch_ip_prefix.addr.ip6, sai_ip_prefix->addr.ip6, IPV6_ADDR_LEN);
    sai_ipv6_prefix_length(sai_ip_prefix->mask.ip6, &switch_ip_prefix.len);
  }
}

sai_status_t sai_ipv4_to_string(_In_ sai_ip4_t ip4,
                                _In_ uint32_t max_length,
                                _Out_ char *entry_string,
                                _Out_ int *entry_length) {
  inet_ntop(AF_INET, &ip4, entry_string, max_length);
  *entry_length = static_cast<int>(strlen(entry_string));
  return SAI_STATUS_SUCCESS;
}

sai_status_t sai_ipv6_to_string(_In_ const sai_ip6_t &ip6,
                                _In_ uint32_t max_length,
                                _Out_ char *entry_string,
                                _Out_ int *entry_length) {
  inet_ntop(AF_INET6, &ip6, entry_string, max_length);
  *entry_length = static_cast<int>(strlen(entry_string));
  return SAI_STATUS_SUCCESS;
}

sai_status_t sai_ipaddress_to_string(_In_ sai_ip_address_t ip_addr,
                                     _In_ uint32_t max_length,
                                     _Out_ char *entry_string,
                                     _Out_ int *entry_length) {
  if (ip_addr.addr_family == SAI_IP_ADDR_FAMILY_IPV4) {
    sai_ipv4_to_string(
        ip_addr.addr.ip4, max_length, entry_string, entry_length);
  } else if (ip_addr.addr_family == SAI_IP_ADDR_FAMILY_IPV6) {
    sai_ipv6_to_string(
        ip_addr.addr.ip6, max_length, entry_string, entry_length);
  } else {
    snprintf(entry_string,
             max_length,
             "Invalid addr family %d",
             ip_addr.addr_family);
    return SAI_STATUS_INVALID_PARAMETER;
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t sai_ipprefix_to_string(_In_ sai_ip_prefix_t ip_prefix,
                                    _In_ uint32_t max_length,
                                    _Out_ char *entry_string,
                                    _Out_ int *entry_length) {
  int len = 0;
  uint32_t pos = 0;

  if (ip_prefix.addr_family == SAI_IP_ADDR_FAMILY_IPV4) {
    sai_ipv4_to_string(ip_prefix.addr.ip4, max_length, entry_string, &len);
    pos += len;
    if (pos > max_length) {
      *entry_length = max_length;
      return SAI_STATUS_SUCCESS;
    }
    pos += snprintf(entry_string + pos, max_length - pos, "/");
    if (pos > max_length) {
      *entry_length = max_length;
      return SAI_STATUS_SUCCESS;
    }
    sai_ipv4_to_string(
        ip_prefix.mask.ip4, max_length - pos, entry_string + pos, &len);
    pos += len;
    if (pos > max_length) {
      *entry_length = max_length;
      return SAI_STATUS_SUCCESS;
    }
  } else if (ip_prefix.addr_family == SAI_IP_ADDR_FAMILY_IPV6) {
    sai_ipv6_to_string(ip_prefix.addr.ip6, max_length, entry_string, &len);
    pos += len;
    if (pos > max_length) {
      *entry_length = max_length;
      return SAI_STATUS_SUCCESS;
    }
    pos += snprintf(entry_string + pos, max_length - pos, "/");
    if (pos > max_length) {
      *entry_length = max_length;
      return SAI_STATUS_SUCCESS;
    }
    sai_ipv6_to_string(
        ip_prefix.mask.ip6, max_length - pos, entry_string + pos, &len);
    pos += len;
    if (pos > max_length) {
      *entry_length = max_length;
      return SAI_STATUS_SUCCESS;
    }
  } else {
    snprintf(entry_string,
             max_length,
             "Invalid addr family %d",
             ip_prefix.addr_family);
    return SAI_STATUS_INVALID_PARAMETER;
  }

  *entry_length = pos;
  return SAI_STATUS_SUCCESS;
}

const char *sai_attribute_name(sai_object_type_t sai_ot,
                               const sai_attr_id_t attr_id) {
  const sai_attr_metadata_t *attr_md = NULL;
  attr_md = sai_metadata_get_attr_metadata(sai_ot, attr_id);
  if (attr_md) return attr_md->attridname;
  return "";
}

switch_object_id_t sai_get_device_id(const uint16_t dev_id) {
  switch_object_id_t device_handle = {0};
  std::set<attr_w> dev_attrs;
  dev_attrs.insert(attr_w(SWITCH_DEVICE_ATTR_DEV_ID, dev_id));
  bf_switch_object_get(SWITCH_OBJECT_TYPE_DEVICE, dev_attrs, device_handle);
  return device_handle;
}

void sai_insert_device_attribute(const uint16_t dev_id,
                                 const switch_attr_id_t sw_dev_attr_id,
                                 std::set<smi::attr_w> &sw_attr_list) {
  switch_object_id_t device_handle = sai_get_device_id(dev_id);

  std::set<attr_w> dev_attrs;
  sw_attr_list.insert(attr_w(sw_dev_attr_id, device_handle));
}

char *sai_strncpy(char *dest, const char *src, size_t n) {
  size_t i;
  for (i = 0; i < n - 1 && src[i] != '\0'; i++) dest[i] = src[i];
  dest[i] = '\0';

  if (strlen(src) >= n) {
    SAI_GENERIC_ERROR(
        "Source string \"%s\" is bigger than maximum size to copy %zu", src, n);
  }

  return dest;
}
