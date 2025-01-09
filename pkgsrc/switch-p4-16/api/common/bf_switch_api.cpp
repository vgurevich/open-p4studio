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


extern "C" {
// Special include to version
#include <saiversion.h>
}

#include <string>
#include <fstream>
#include <set>
#include <vector>

#include "bf_switch/bf_switch.h"
#include "s3/switch_packet.h"
#include "s3/record.h"
#include "../../s3/log.h"
#include "common/utils.h"
#include "common/hostif.h"
#include "switch_tna/utils.h"

extern "C" {
#include "bf_types/bf_types.h"
#include "lld/bf_lld_if.h"
// Special include to version
#include "../../version/bf_switch_ver.h"
}

using ::smi::logging::switch_log;

namespace smi {
using ::smi::attr_util::parse_mac;

/* Device add routine
 * Create a device object for passed in dev_id
 * Create a CPU port and assign it to the device
 * Create a default RMAC group and an rmac
 * Create default vlan
 * Create default vrf
 */
switch_status_t bf_switch_device_add(uint16_t device,
                                     const char *const veth_port,
                                     switch_object_id_t *device_handle) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  uint32_t cpu_port = SWITCH_CPU_PORT_ETH_DEFAULT;
  bool use_pcie = false;
  switch_object_id_t cpu_port_handle, default_vlan_handle, default_vrf_handle,
      cpu_rif_handle, nhop_drop_handle, nhop_glean_handle, mirror_handle,
      default_stp_handle, device_hdl;
  switch_mac_addr_t default_mac = {0x00, 0xBA, 0x7E, 0xF0, 0x00, 0x0};

  if (!veth_port) {
    use_pcie = true;
    cpu_port = SWITCH_CPU_PORT_PCIE_DEFAULT;
  }

  // Create device
  std::set<attr_w> device_attrs;
  device_attrs.insert(attr_w(SWITCH_DEVICE_ATTR_DEV_ID, device));
  device_attrs.insert(attr_w(SWITCH_DEVICE_ATTR_USE_PCIE, use_pcie));
  device_attrs.insert(attr_w(SWITCH_DEVICE_ATTR_SRC_MAC, default_mac));
  status = switch_store::object_create(
      SWITCH_OBJECT_TYPE_DEVICE, device_attrs, device_hdl);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}:{}: failed status={} for device {}",
               __func__,
               __LINE__,
               status,
               device);
    return status;
  }

  // Create the CPU port handle
  std::set<attr_w> port_attrs;
  switch_enum_t port_type = {.enumdata = SWITCH_PORT_ATTR_TYPE_CPU};
  std::vector<uint32_t> cpu_lane_list{
      cpu_port, cpu_port + 1, cpu_port + 2, cpu_port + 3};
  uint32_t speed = 100000;
  port_attrs.insert(attr_w(SWITCH_PORT_ATTR_DEVICE, device_hdl));
  port_attrs.insert(attr_w(SWITCH_PORT_ATTR_TYPE, port_type));
  port_attrs.insert(attr_w(SWITCH_PORT_ATTR_LANE_LIST, cpu_lane_list));
  port_attrs.insert(attr_w(SWITCH_PORT_ATTR_SPEED, speed));
  status = switch_store::object_create(
      SWITCH_OBJECT_TYPE_PORT, port_attrs, cpu_port_handle);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_PORT,
               "{}:{}: object_create failed status={} for port {}",
               __func__,
               __LINE__,
               status,
               cpu_port);
    return status;
  }

  // Set the device's CPU port attribute
  attr_w cpu_port_attr(SWITCH_DEVICE_ATTR_CPU_PORT, cpu_port_handle);
  status = switch_store::attribute_set(device_hdl, cpu_port_attr);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: attribute_set failed status={} for device {}",
               __func__,
               __LINE__,
               status,
               device);
    return status;
  }

  // Create egress CPU mirror session
  std::set<attr_w> mirror_attrs;
  switch_enum_t mirror_type = {.enumdata = SWITCH_MIRROR_ATTR_MIRROR_TYPE_PORT};
  switch_enum_t type = {.enumdata = SWITCH_MIRROR_ATTR_TYPE_LOCAL};
  switch_enum_t session_type = {.enumdata =
                                    SWITCH_MIRROR_ATTR_SESSION_TYPE_SIMPLE};
  switch_enum_t direction = {.enumdata = SWITCH_MIRROR_ATTR_DIRECTION_EGRESS};
  mirror_attrs.insert(attr_w(SWITCH_MIRROR_ATTR_DEVICE, device_hdl));
  mirror_attrs.insert(
      attr_w(SWITCH_MIRROR_ATTR_EGRESS_PORT_HANDLE, cpu_port_handle));
  mirror_attrs.insert(attr_w(SWITCH_MIRROR_ATTR_MIRROR_TYPE, mirror_type));
  mirror_attrs.insert(attr_w(SWITCH_MIRROR_ATTR_TYPE, type));
  mirror_attrs.insert(attr_w(SWITCH_MIRROR_ATTR_SESSION_TYPE, session_type));
  mirror_attrs.insert(attr_w(SWITCH_MIRROR_ATTR_DIRECTION, direction));
  mirror_attrs.insert(attr_w(SWITCH_MIRROR_ATTR_INTERNAL_OBJECT, true));
  status = switch_store::object_create(
      SWITCH_OBJECT_TYPE_MIRROR, mirror_attrs, mirror_handle);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_MIRROR,
               "{}:{}: object_create failed status={} for cpu mirror {}",
               __func__,
               __LINE__,
               status,
               cpu_port);
    return status;
  }

  // Set the device's CPU egress mirror attribute
  attr_w eg_cpu_mirror_attr(SWITCH_DEVICE_ATTR_EGRESS_CPU_MIRROR_HANDLE,
                            mirror_handle);
  status = switch_store::attribute_set(device_hdl, eg_cpu_mirror_attr);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: attribute_set failed status={} for device {}",
               __func__,
               __LINE__,
               status,
               device);
    return status;
  }

  attr_w recirc_list_attr(SWITCH_DEVICE_ATTR_RECIRC_PORT_LIST);
  status = switch_store::attribute_get(
      device_hdl, SWITCH_DEVICE_ATTR_RECIRC_PORT_LIST, recirc_list_attr);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: recirc port list get failed for device {}:"
               "device add failed: {}",
               __func__,
               __LINE__,
               device,
               switch_error_to_string(status));
    return SWITCH_STATUS_FAILURE;
  }
  std::vector<uint16_t> recirc_port_list;
  recirc_list_attr.v_get(recirc_port_list);
  for (uint16_t recirc_port : recirc_port_list) {
    switch_object_id_t recirc_port_handle = {};
    std::set<attr_w> recirc_port_attrs;
    switch_enum_t recirc_port_type = {.enumdata = SWITCH_PORT_ATTR_TYPE_RECIRC};
    std::vector<uint32_t> recirc_lane_list{recirc_port};
    uint32_t recirc_speed = 100000;
    recirc_port_attrs.insert(attr_w(SWITCH_PORT_ATTR_DEVICE, device_hdl));
    recirc_port_attrs.insert(attr_w(SWITCH_PORT_ATTR_TYPE, recirc_port_type));
    recirc_port_attrs.insert(
        attr_w(SWITCH_PORT_ATTR_LANE_LIST, recirc_lane_list));
    recirc_port_attrs.insert(attr_w(SWITCH_PORT_ATTR_SPEED, recirc_speed));
    recirc_port_attrs.insert(attr_w(SWITCH_PORT_ATTR_INTERNAL_OBJECT, true));
    status = switch_store::object_create(
        SWITCH_OBJECT_TYPE_PORT, recirc_port_attrs, recirc_port_handle);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_PORT,
                 "{}:{}: object_create failed status={} for recirc port {}",
                 __func__,
                 __LINE__,
                 status,
                 recirc_port);
      return status;
    }
  }

  // Create default vrf
  // VRF created before vlan so it get's first BD
  std::set<attr_w> vrf_attrs;
  uint32_t vrf_id = SWITCH_DEFAULT_VRF;
  vrf_attrs.insert(attr_w(SWITCH_VRF_ATTR_DEVICE, device_hdl));
  vrf_attrs.insert(attr_w(SWITCH_VRF_ATTR_ID, vrf_id));
  vrf_attrs.insert(attr_w(SWITCH_VRF_ATTR_SRC_MAC, default_mac));
  status = switch_store::object_create(
      SWITCH_OBJECT_TYPE_VRF, vrf_attrs, default_vrf_handle);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_VRF,
               "{}.{}: bf_switch_object_create failed status={} for vrf {}",
               __func__,
               __LINE__,
               status,
               SWITCH_DEFAULT_VRF);
    return status;
  }

  // Set the device's default VRF attribute
  attr_w vrf_attr(SWITCH_DEVICE_ATTR_DEFAULT_VRF, default_vrf_handle);
  status = switch_store::attribute_set(device_hdl, vrf_attr);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: attribute_set default "
               "vrf failed status={} for device {}",
               __func__,
               __LINE__,
               status,
               device);
    return status;
  }

  // Create default stp
  std::set<attr_w> stp_attrs;
  stp_attrs.insert(attr_w(SWITCH_STP_ATTR_DEVICE, device_hdl));
  status = switch_store::object_create(
      SWITCH_OBJECT_TYPE_STP, stp_attrs, default_stp_handle);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_STP,
               "{}.{}: object_create failed status={} for default stp",
               __func__,
               __LINE__,
               status);
    return status;
  }

  // Set the device's default STP attribute
  attr_w stp_attr(SWITCH_DEVICE_ATTR_DEFAULT_STP, default_stp_handle);
  status = switch_store::attribute_set(device_hdl, stp_attr);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: attribute_set default "
               "stp failed status={} for device {}",
               __func__,
               __LINE__,
               status,
               device);
    return status;
  }

  // Create default vlan
  std::set<attr_w> vlan_attrs;
  uint16_t vlan_id = SWITCH_DEFAULT_VLAN;
  vlan_attrs.insert(attr_w(SWITCH_VLAN_ATTR_DEVICE, device_hdl));
  vlan_attrs.insert(attr_w(SWITCH_VLAN_ATTR_VLAN_ID, vlan_id));
  vlan_attrs.insert(attr_w(SWITCH_VLAN_ATTR_STP_HANDLE, default_stp_handle));
  status = switch_store::object_create(
      SWITCH_OBJECT_TYPE_VLAN, vlan_attrs, default_vlan_handle);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_VLAN,
               "{}.{}: object_create failed status={} for vlan {}",
               __func__,
               __LINE__,
               status,
               SWITCH_DEFAULT_VLAN);
    return status;
  }

  // Set the device's default vlan attribute
  attr_w vlan_attr(SWITCH_DEVICE_ATTR_DEFAULT_VLAN, default_vlan_handle);
  status = switch_store::attribute_set(device_hdl, vlan_attr);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: attribute_set default "
               "vlan failed status={} for device {}",
               __func__,
               __LINE__,
               status,
               device);
    return status;
  }

  // Create CPU rif
  // needed for creation glean and drop nhop handle
  std::set<attr_w> cpu_rif_attrs;
  switch_enum_t rif_type = {.enumdata = SWITCH_RIF_ATTR_TYPE_PORT};
  cpu_rif_attrs.insert(attr_w(SWITCH_RIF_ATTR_DEVICE, device_hdl));
  cpu_rif_attrs.insert(attr_w(SWITCH_RIF_ATTR_VRF_HANDLE, default_vrf_handle));
  cpu_rif_attrs.insert(attr_w(SWITCH_RIF_ATTR_TYPE, rif_type));
  cpu_rif_attrs.insert(attr_w(SWITCH_RIF_ATTR_PORT_HANDLE, cpu_port_handle));
  cpu_rif_attrs.insert(attr_w(SWITCH_RIF_ATTR_SRC_MAC, default_mac));
  cpu_rif_attrs.insert(attr_w(SWITCH_RIF_ATTR_INTERNAL_OBJECT, true));
  status = switch_store::object_create(
      SWITCH_OBJECT_TYPE_RIF, cpu_rif_attrs, cpu_rif_handle);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_RIF,
               "{}:{}: object_create failed status={} for rif_handle",
               __func__,
               __LINE__,
               status);
    return status;
  }

  // Create glean nhop handle
  std::set<attr_w> glean_attrs;
  switch_enum_t glean_type = {.enumdata = SWITCH_NEXTHOP_ATTR_TYPE_GLEAN};
  glean_attrs.insert(attr_w(SWITCH_NEXTHOP_ATTR_DEVICE, device_hdl));
  glean_attrs.insert(attr_w(SWITCH_NEXTHOP_ATTR_TYPE, glean_type));
  glean_attrs.insert(attr_w(SWITCH_NEXTHOP_ATTR_HANDLE, cpu_rif_handle));
  status = switch_store::object_create(
      SWITCH_OBJECT_TYPE_NEXTHOP, glean_attrs, nhop_glean_handle);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NEXTHOP,
               "{}:{}: object_create failed status={} for glean nhop",
               __func__,
               __LINE__,
               status);
    return status;
  }

  // Set the device's glean nexthop handle attribute
  attr_w glean_attr(SWITCH_DEVICE_ATTR_GLEAN_NEXTHOP_HANDLE, nhop_glean_handle);
  status = switch_store::attribute_set(device_hdl, glean_attr);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: attribute_set failed status={} for device {}",
               __func__,
               __LINE__,
               status,
               device);
    return status;
  }

  // Create nhop drop handle
  std::set<attr_w> drop_attrs;
  switch_enum_t drop_type = {.enumdata = SWITCH_NEXTHOP_ATTR_TYPE_DROP};
  drop_attrs.insert(attr_w(SWITCH_NEXTHOP_ATTR_DEVICE, device_hdl));
  drop_attrs.insert(attr_w(SWITCH_NEXTHOP_ATTR_TYPE, drop_type));
  drop_attrs.insert(attr_w(SWITCH_NEXTHOP_ATTR_HANDLE, cpu_rif_handle));
  status = switch_store::object_create(
      SWITCH_OBJECT_TYPE_NEXTHOP, drop_attrs, nhop_drop_handle);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NEXTHOP,
               "{}:{}: object_create failed status={} for drop nhop",
               __func__,
               __LINE__,
               status);
    return status;
  }

  // Set the device's drop nexthop handle attribute
  attr_w drop_attr(SWITCH_DEVICE_ATTR_DROP_NEXTHOP_HANDLE, nhop_drop_handle);
  status = switch_store::attribute_set(device_hdl, drop_attr);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: attribute_set failed status={} for device {}",
               __func__,
               __LINE__,
               status,
               device);
    return status;
  }

  if (switch_pktdriver_mode_is_kernel()) {
    status = switch_pktdriver_knet_device_add(device);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_DEVICE,
                 "{}.{}: device add failed for device {}:"
                 "knet device add failed: {}",
                 __func__,
                 __LINE__,
                 device,
                 switch_error_to_string(status));

      return SWITCH_STATUS_FAILURE;
    }
  }

  *device_handle = device_hdl;
  return status;
}

/* Device removal
 * Remove default vrf
 * Remove default vlan
 */
switch_status_t bf_switch_device_remove(uint16_t device) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t device_hdl, attr_hdl;
  switch_attribute_t attribute;
  switch_attribute_t attrs[3];

  memset(attrs, 0, sizeof(attrs));
  attrs[0].id = SWITCH_DEVICE_ATTR_DEV_ID;
  attrs[0].value.type = SWITCH_TYPE_UINT16;
  attrs[0].value.u16 = device;
  status =
      bf_switch_object_get_c(SWITCH_OBJECT_TYPE_DEVICE, 1, attrs, &device_hdl);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: device get failed for device {}:"
               "device remove failed: {}",
               __func__,
               __LINE__,
               device,
               switch_error_to_string(status));
    return SWITCH_STATUS_FAILURE;
  }

  // Get and remove default vlan
  memset(&attribute, 0, sizeof(attribute));
  status = bf_switch_attribute_get_c(
      device_hdl, SWITCH_DEVICE_ATTR_DEFAULT_VLAN, &attribute);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: default vlan get failed for device {}:"
               "device remove failed: {}",
               __func__,
               __LINE__,
               device,
               switch_error_to_string(status));
    return SWITCH_STATUS_FAILURE;
  }
  attr_hdl.data = attribute.value.oid.data;
  attribute.id = SWITCH_DEVICE_ATTR_DEFAULT_VLAN;
  attribute.value.type = SWITCH_TYPE_OBJECT_ID;
  attribute.value.oid.data = 0;
  status = bf_switch_attribute_set_c(device_hdl, &attribute);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: default vlan clear failed for device {}:"
               "device remove failed: {}",
               __func__,
               __LINE__,
               device,
               switch_error_to_string(status));
    return status;
  }
  status = bf_switch_object_delete_c(attr_hdl);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_VLAN,
               "{}.{}: default vlan delete failed for device {}:"
               "device remove failed: {}",
               __func__,
               __LINE__,
               device,
               switch_error_to_string(status));
    return SWITCH_STATUS_FAILURE;
  }

  // Get and remove Glean and NHOP
  memset(&attribute, 0, sizeof(attribute));
  status = bf_switch_attribute_get_c(
      device_hdl, SWITCH_DEVICE_ATTR_GLEAN_NEXTHOP_HANDLE, &attribute);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: glean nhop handle get failed for device {}:"
               "device remove failed: {}",
               __func__,
               __LINE__,
               device,
               switch_error_to_string(status));
    return SWITCH_STATUS_FAILURE;
  }
  attr_hdl.data = attribute.value.oid.data;
  attribute.id = SWITCH_DEVICE_ATTR_GLEAN_NEXTHOP_HANDLE;
  attribute.value.type = SWITCH_TYPE_OBJECT_ID;
  attribute.value.oid.data = 0;
  status = bf_switch_attribute_set_c(device_hdl, &attribute);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: glean nhop handle clear failed for device {}:"
               "device remove failed: {}",
               __func__,
               __LINE__,
               device,
               switch_error_to_string(status));
    return status;
  }
  status = bf_switch_object_delete_c(attr_hdl);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_VLAN,
               "{}.{}: glean nhop handle delete failed for device {}:"
               "device remove failed: {}",
               __func__,
               __LINE__,
               device,
               switch_error_to_string(status));
    return SWITCH_STATUS_FAILURE;
  }

  memset(&attribute, 0, sizeof(attribute));
  status = bf_switch_attribute_get_c(
      device_hdl, SWITCH_DEVICE_ATTR_DROP_NEXTHOP_HANDLE, &attribute);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: drop nhop handle get failed for device {}:"
               "device remove failed: {}",
               __func__,
               __LINE__,
               device,
               switch_error_to_string(status));
    return SWITCH_STATUS_FAILURE;
  }
  attr_hdl.data = attribute.value.oid.data;
  attribute.id = SWITCH_DEVICE_ATTR_DROP_NEXTHOP_HANDLE;
  attribute.value.type = SWITCH_TYPE_OBJECT_ID;
  attribute.value.oid.data = 0;
  status = bf_switch_attribute_set_c(device_hdl, &attribute);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: drop nhop handle clear failed for device {}:"
               "device remove failed: {}",
               __func__,
               __LINE__,
               device,
               switch_error_to_string(status));
    return status;
  }
  status = bf_switch_object_delete_c(attr_hdl);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_VLAN,
               "{}.{}: drop nhop handle delete failed for device {}:"
               "device remove failed: {}",
               __func__,
               __LINE__,
               device,
               switch_error_to_string(status));
    return SWITCH_STATUS_FAILURE;
  }

  // Get and remove CPU RIF
  switch_object_id_t cpu_port_handle;
  memset(&attribute, 0, sizeof(attribute));
  status = bf_switch_attribute_get_c(
      device_hdl, SWITCH_DEVICE_ATTR_CPU_PORT, &attribute);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: cpu port get failed for device {}:"
               "device get failed: {}",
               __func__,
               __LINE__,
               device,
               switch_error_to_string(status));

    return SWITCH_STATUS_FAILURE;
  }
  cpu_port_handle.data = attribute.value.oid.data;

  switch_object_id_t *rif_handles = NULL;
  uint32_t rif_count = 0;
  status = bf_switch_get_all_handles_c(
      SWITCH_OBJECT_TYPE_RIF, &rif_handles, &rif_count);
  if (status == SWITCH_STATUS_SUCCESS) {
    for (uint32_t i = 0; i < rif_count; i++) {
      memset(&attribute, 0, sizeof(attribute));
      status = bf_switch_attribute_get_c(
          rif_handles[i], SWITCH_RIF_ATTR_PORT_HANDLE, &attribute);
      if (status == SWITCH_STATUS_SUCCESS &&
          attribute.value.oid.data == cpu_port_handle.data) {
        status = bf_switch_object_delete_c(rif_handles[i]);
        if (status != SWITCH_STATUS_SUCCESS) {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_RIF,
                     "{}.{}: CPU RIF handle delete failed for device {}:"
                     "rif handle delete failed: {}",
                     __func__,
                     __LINE__,
                     device,
                     switch_error_to_string(status));
          free(rif_handles);
          return SWITCH_STATUS_FAILURE;
        }
        break;
      }
    }

    free(rif_handles);
  }

  // Get and remove default vrf
  memset(&attribute, 0, sizeof(attribute));
  status = bf_switch_attribute_get_c(
      device_hdl, SWITCH_DEVICE_ATTR_DEFAULT_VRF, &attribute);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: default vrf get failed for device {}:"
               "device remove failed: {}",
               __func__,
               __LINE__,
               device,
               switch_error_to_string(status));

    return SWITCH_STATUS_FAILURE;
  }
  attr_hdl.data = attribute.value.oid.data;
  attribute.id = SWITCH_DEVICE_ATTR_DEFAULT_VRF;
  attribute.value.type = SWITCH_TYPE_OBJECT_ID;
  attribute.value.oid.data = 0;
  status = bf_switch_attribute_set_c(device_hdl, &attribute);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: default vrf clear failed for device {}:"
               "device remove failed: {}",
               __func__,
               __LINE__,
               device,
               switch_error_to_string(status));
    return status;
  }
  status = bf_switch_object_delete_c(attr_hdl);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_VRF,
               "{}.{}: default vrf delete failed for device {}:"
               "device remove failed: {}",
               __func__,
               __LINE__,
               device,
               switch_error_to_string(status));
    return SWITCH_STATUS_FAILURE;
  }

  // Get and remove egress CPU mirror handle
  memset(&attribute, 0, sizeof(attribute));
  status = bf_switch_attribute_get_c(
      device_hdl, SWITCH_DEVICE_ATTR_EGRESS_CPU_MIRROR_HANDLE, &attribute);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: egress cpu mirror get failed for device {}:"
               "device get failed: {}",
               __func__,
               __LINE__,
               device,
               switch_error_to_string(status));

    return SWITCH_STATUS_FAILURE;
  }
  attr_hdl.data = attribute.value.oid.data;
  attribute.id = SWITCH_DEVICE_ATTR_EGRESS_CPU_MIRROR_HANDLE;
  attribute.value.type = SWITCH_TYPE_OBJECT_ID;
  attribute.value.oid.data = 0;
  status = bf_switch_attribute_set_c(device_hdl, &attribute);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: egress cpu mirror clear failed for device {}:"
               "device remove failed: {}",
               __func__,
               __LINE__,
               device,
               switch_error_to_string(status));
    return status;
  }
  status = bf_switch_object_delete_c(attr_hdl);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_PORT,
               "{}.{}: egress cpu mirror delete failed for device {}:"
               "device remove failed: {}",
               __func__,
               __LINE__,
               device,
               switch_error_to_string(status));
    return SWITCH_STATUS_FAILURE;
  }

  // Get and remove CPU port
  attribute.id = SWITCH_DEVICE_ATTR_CPU_PORT;
  attribute.value.type = SWITCH_TYPE_OBJECT_ID;
  attribute.value.oid.data = 0;
  status = bf_switch_attribute_set_c(device_hdl, &attribute);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: cpu port clear failed for device {}:"
               "device remove failed: {}",
               __func__,
               __LINE__,
               device,
               switch_error_to_string(status));
    return status;
  }
  status = bf_switch_object_delete_c(cpu_port_handle);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_PORT,
               "{}.{}: cpu port delete failed for device {}:"
               "device remove failed: {}",
               __func__,
               __LINE__,
               device,
               switch_error_to_string(status));
    return SWITCH_STATUS_FAILURE;
  }

  // Remove the recirc port handles
  switch_object_id_t *recirc_port_handles = NULL;
  uint32_t count = 0;
  status = bf_switch_get_all_handles_c(
      SWITCH_OBJECT_TYPE_PORT, &recirc_port_handles, &count);
  if (status == SWITCH_STATUS_SUCCESS) {
    for (uint32_t i = 0; i < count; i++) {
      status = bf_switch_object_delete_c(recirc_port_handles[i]);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_PORT,
                   "{}.{}: recirc port delete failed for device {}: port: {}"
                   "device remove failed: {}",
                   __func__,
                   __LINE__,
                   device,
                   recirc_port_handles[i].data,
                   switch_error_to_string(status));
        free(recirc_port_handles);
        return SWITCH_STATUS_FAILURE;
      }
    }

    free(recirc_port_handles);
  }

  // Get and remove Ingress and Egress buffer profiles
  memset(&attribute, 0, sizeof(attribute));
  status = bf_switch_attribute_get_c(
      device_hdl,
      SWITCH_DEVICE_ATTR_DEFAULT_INGRESS_BUFFER_PROFILE_HANDLE,
      &attribute);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(
        SWITCH_API_LEVEL_ERROR,
        SWITCH_OBJECT_TYPE_DEVICE,
        "{}.{}: default ingress buffer profile handle get failed for device {}:"
        "device remove failed: {}",
        __func__,
        __LINE__,
        device,
        switch_error_to_string(status));
    return SWITCH_STATUS_FAILURE;
  }
  attr_hdl.data = attribute.value.oid.data;
  attribute.id = SWITCH_DEVICE_ATTR_DEFAULT_INGRESS_BUFFER_PROFILE_HANDLE;
  attribute.value.type = SWITCH_TYPE_OBJECT_ID;
  attribute.value.oid.data = 0;
  status = bf_switch_attribute_set_c(device_hdl, &attribute);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: default ingress buffer profile handle clear failed for "
               "device {}:"
               "device remove failed: {}",
               __func__,
               __LINE__,
               device,
               switch_error_to_string(status));
    return status;
  }
  status = bf_switch_object_delete_c(attr_hdl);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_VLAN,
               "{}.{}: default ingress buffer profile handle delete failed for "
               "device {}:"
               "device remove failed: {}",
               __func__,
               __LINE__,
               device,
               switch_error_to_string(status));
    return SWITCH_STATUS_FAILURE;
  }

  memset(&attribute, 0, sizeof(attribute));
  status = bf_switch_attribute_get_c(
      device_hdl,
      SWITCH_DEVICE_ATTR_DEFAULT_EGRESS_BUFFER_PROFILE_HANDLE,
      &attribute);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(
        SWITCH_API_LEVEL_ERROR,
        SWITCH_OBJECT_TYPE_DEVICE,
        "{}.{}: default egress buffer profile handle get failed for device {}:"
        "device remove failed: {}",
        __func__,
        __LINE__,
        device,
        switch_error_to_string(status));
    return SWITCH_STATUS_FAILURE;
  }
  attr_hdl.data = attribute.value.oid.data;
  attribute.id = SWITCH_DEVICE_ATTR_DEFAULT_EGRESS_BUFFER_PROFILE_HANDLE;
  attribute.value.type = SWITCH_TYPE_OBJECT_ID;
  attribute.value.oid.data = 0;
  status = bf_switch_attribute_set_c(device_hdl, &attribute);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: default egress buffer profile handle handle clear "
               "failed for device {}:"
               "device remove failed: {}",
               __func__,
               __LINE__,
               device,
               switch_error_to_string(status));
    return status;
  }
  status = bf_switch_object_delete_c(attr_hdl);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_VLAN,
               "{}.{}: default egress buffer profile handle handle delete "
               "failed for device {}:"
               "device remove failed: {}",
               __func__,
               __LINE__,
               device,
               switch_error_to_string(status));
    return SWITCH_STATUS_FAILURE;
  }

  // Finally remove the device
  status = bf_switch_object_delete_c(device_hdl);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: device delete failed for device {}:"
               "device remove failed: {}",
               __func__,
               __LINE__,
               device,
               switch_error_to_string(status));
    return SWITCH_STATUS_FAILURE;
  }

  if (switch_pktdriver_mode_is_kernel()) {
    status = switch_pktdriver_knet_device_delete(device);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_DEVICE,
                 "{}.{}: knet device delete failed for device {}:"
                 "knet device remove failed: {}",
                 __func__,
                 __LINE__,
                 device,
                 switch_error_to_string(status));

      return SWITCH_STATUS_FAILURE;
    }
  }

  return SWITCH_STATUS_SUCCESS;
}

}  // namespace smi

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Entry point for SMI
 * This API is purely for calling from bf_switchd driver
 */
switch_status_t bf_switch_init(uint16_t device,
                               const char *const conf_file,
                               const char *const file_path_prefix,
                               bool warm_init,
                               const char *const warm_init_file,
                               bool use_kpkt) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t device_handle = {0};
  bool use_pcie = false;

  // Parse conf file for:
  //   cpu_port
  //   model_json
  //   switchapi_port_add
  bf_switch::switchOptions options;
  if (options.load_from_conf(device, conf_file, file_path_prefix)) {
    return -1;
  }

  // TODO(AB): load from config
  // smi::record::record_file_init("bmai.rec");

  if (!options.use_eth_cpu_port_get()) use_pcie = true;
  if (use_kpkt) smi::SWITCH_CONTEXT.set_use_kpkt(true);

  /* Initialize infra, p4 objects, non-p4 objects */
  status = smi::switch_init(options.model_json_get().c_str(),
                            options.eth_cpu_port_get().c_str(),
                            use_pcie,
                            options.override_log_level_get(),
                            warm_init,
                            warm_init_file);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}: switch_init failed status={} for device {}",
               __func__,
               status,
               device);
    return status;
  }

  /* Create a device handle with the passed in device ID */
  if (!warm_init) {
    status = smi::bf_switch_device_add(
        device,
        use_pcie ? nullptr : options.eth_cpu_port_get().c_str(),
        &device_handle);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_DEVICE,
                 "{}: bf_switch_device_add failed status={} for device {}",
                 __func__,
                 status,
                 device);
      return status;
    }
  }

  return status;
}

/**
 * This API is purely for calling from bf_switchd driver
 */
switch_status_t bf_switch_clean(uint16_t device,
                                bool warm_shut,
                                const char *const warm_shut_file) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  smi::hostif_stop_packet_driver();

  if (!warm_shut) {
    // On cold reboot don't necessary clean all switch objects
    // enough only call device_clean() for stop system timers.
    // If timers continue to run on cold reboot,
    // we will have bf_timer crash.
    status = smi::device_clean();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_DEVICE,
                 "{}: device_clean failed status={} for device {}",
                 __func__,
                 status,
                 device);
    }
    return status;
  }

  /* Clear infra, p4 objects, non-p4 objects */
  status = smi::switch_clean(warm_shut, warm_shut_file);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}: switch_clean failed status={} for device {}",
               __func__,
               status,
               device);
    return status;
  }

  // Stop recording and save file
  // smi::record::record_file_clean();

  return status;
}

switch_status_t bf_switch_init_packet_driver() {
  smi::hostif_start_packet_driver();
  return SWITCH_STATUS_SUCCESS;
}

#ifdef __cplusplus
}
#endif

namespace bf_switch {

switch_status_t bf_switch_table_info_get(uint64_t table_id,
                                         switch_table_info_t &table_info) {
  return smi::table_info_get(static_cast<switch_device_attr_table>(table_id),
                             table_info);
}

bool bf_switch_is_feature_enabled(switch_feature_id_t feature) {
  bool retval = smi::feature::is_feature_set(feature);
  return retval;
}

switch_status_t bf_switch_sai_mode_set(bool sai_mode) {
  smi::sai_mode_set(sai_mode);
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t bf_switch_version_get(std::string &sde_version,
                                      std::string &sai_version) {
  sde_version = BF_SWITCH_REL_VER;
  sai_version = std::to_string(SAI_MAJOR) + "." + std::to_string(SAI_MINOR) +
                "." + std::to_string(SAI_REVISION);
  return SWITCH_STATUS_SUCCESS;
}

int switchOptions::load_from_conf(int dev_id,
                                  const char *conf_file,
                                  const char *file_path_prefix) {
  if (!conf_file) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}: NULL conf file dev_id {}",
               __func__,
               dev_id);
    return -1;
  }
  std::ostringstream tmp;
  std::ifstream file_stream(conf_file, std::ifstream::in);
  tmp << file_stream.rdbuf();

  cJSON *root = cJSON_Parse(tmp.str().c_str());
  if (!root) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}: SwitchOption json parsing failed for device {} file {}",
               __func__,
               dev_id,
               conf_file);
    return -1;
  }

  // Parse the chip_list to find the device family.
  bool found_chip = false;
  cJSON *chip_list = cJSON_GetObjectItem(root, "chip_list");
  if (!chip_list || chip_list->type != cJSON_Array) {
    switch_log(
        SWITCH_API_LEVEL_ERROR,
        SWITCH_OBJECT_TYPE_DEVICE,
        "{}: SwitchOption json parsing failed for device {} file {} option {}",
        __func__,
        dev_id,
        conf_file,
        "chip_list");
    return -1;
  }
  for (int i = 0, chip_list_sz = cJSON_GetArraySize(chip_list);
       i < chip_list_sz && !found_chip;
       ++i) {
    cJSON *chip = cJSON_GetArrayItem(chip_list, i);
    if (!chip || chip->type != cJSON_Object) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_DEVICE,
                 "{}: SwitchOption json parsing failed for device {} file {} "
                 "option {}",
                 __func__,
                 dev_id,
                 conf_file,
                 "chip_list");
      return -1;
    }
    cJSON *instance = cJSON_GetObjectItem(chip, "instance");
    if (!instance || instance->type != cJSON_Number) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_DEVICE,
                 "{}: SwitchOption json parsing failed for device {} file {} "
                 "option {}",
                 __func__,
                 dev_id,
                 conf_file,
                 "instance");
      return -1;
    }
    int this_dev = instance->valueint;
    if (this_dev != dev_id) continue;
    found_chip = true;

    cJSON *chip_fam = cJSON_GetObjectItem(chip, "chip_family");
    if (!chip_fam || chip_fam->type != cJSON_String) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_DEVICE,
                 "{}: SwitchOption json parsing failed for device {} file {} "
                 "option {}",
                 __func__,
                 dev_id,
                 conf_file,
                 "chip_family");
      return -1;
    }
    if (!strcasecmp(chip_fam->valuestring, "tofino")) {
      this->tf_version = 1;
    } else if (!strcasecmp(chip_fam->valuestring, "tofino2")) {
      this->tf_version = 2;
    } else {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_DEVICE,
                 "{}: SwitchOption json parsing failed for device {} file {} "
                 "option {}",
                 __func__,
                 dev_id,
                 conf_file,
                 "chip_family");
      return -1;
    }
  }

  // Seach the switch_options for the requested device and lookup the remaining
  // switch information.
  bool found_opts = false;
  cJSON *opts_list = cJSON_GetObjectItem(root, "switch_options");
  if (!opts_list || opts_list->type != cJSON_Array) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}: SwitchOption json parsing failed for device {} file {} "
               "option {}",
               __func__,
               dev_id,
               conf_file,
               "switch_options");
    return -1;
  }
  for (int i = 0, opts_list_sz = cJSON_GetArraySize(opts_list);
       i < opts_list_sz && !found_opts;
       ++i) {
    cJSON *opts = cJSON_GetArrayItem(opts_list, i);
    if (!opts || opts->type != cJSON_Object) {
      return -1;
    }
    // The "device-id" is a required field.
    cJSON *device_id = cJSON_GetObjectItem(opts, "device-id");
    if (!device_id || device_id->type != cJSON_Number) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_DEVICE,
                 "{}: SwitchOption json parsing failed for device {} file {} "
                 "option {}",
                 __func__,
                 dev_id,
                 conf_file,
                 "device-id");
      return -1;
    }
    int this_dev = device_id->valueint;
    if (this_dev != dev_id) continue;
    found_opts = true;

    // The "non_default_port_ppgs is an optional field defaulting to zero when
    // it is not present.
    cJSON *port_ppgs = cJSON_GetObjectItem(opts, "non_default_port_ppgs");
    if (!port_ppgs) {
      this->non_default_port_ppgs = 0;
    } else if (port_ppgs->type != cJSON_Number) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_DEVICE,
                 "{}: SwitchOption json parsing failed for device {} file {} "
                 "option {}",
                 __func__,
                 dev_id,
                 conf_file,
                 "non_default_port_ppgs");
      return -1;
    } else {
      this->non_default_port_ppgs = port_ppgs->valueint;
    }

    // The "model_json_path" is a required field.
    cJSON *model = cJSON_GetObjectItem(opts, "model_json_path");
    if (!model || model->type != cJSON_String) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_DEVICE,
                 "{}: SwitchOption json parsing failed for device {} file {} "
                 "option {}",
                 __func__,
                 dev_id,
                 conf_file,
                 "model_json_path");
      return -1;
    } else {
      this->model_json = std::string(model->valuestring);
      if (this->model_json.length() == 0) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_DEVICE,
                   "{}: SwitchOption json parsing failed for device {} file {} "
                   "model_json_path is empty",
                   __func__,
                   dev_id,
                   conf_file);
        return -1;
      }
      if (this->model_json.at(0) != '/') {
        // The file has a relative path in the conf file, prefix it with the
        // provided string.
        this->model_json.insert(0, "/");
        this->model_json.insert(0, file_path_prefix);
      }
    }

    // The "cpu_port" field is optional, when not present the use_eth_cpu_port
    // flag is set to false.
    cJSON *cpu_port = cJSON_GetObjectItem(opts, "cpu_port");
    if (!cpu_port) {
      this->use_eth_cpu_port = false;
      this->eth_cpu_port = std::string("");
    } else if (cpu_port->type != cJSON_String) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_DEVICE,
                 "{}: SwitchOption json parsing failed for device {} file {} "
                 "option {}",
                 __func__,
                 dev_id,
                 conf_file,
                 "cpu_port");
      return -1;
    } else {
      this->use_eth_cpu_port = true;
      this->eth_cpu_port = std::string(cpu_port->valuestring);
    }

    // The "sai_skip_init" field is optional, when not present it is assumed to
    // be false.
    cJSON *sai_init = cJSON_GetObjectItem(opts, "sai_skip_init");
    if (!sai_init) {
      this->sai_skip_init = false;
    } else if (sai_init->type != cJSON_True && sai_init->type != cJSON_False) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_DEVICE,
                 "{}: SwitchOption json parsing failed for device {} file {} "
                 "option {}",
                 __func__,
                 dev_id,
                 conf_file,
                 "sai_skip_init");
      return -1;
    } else {
      this->sai_skip_init = (sai_init->type == cJSON_True);
    }

    // The "override_log_level" field is optional, when not present it is
    // assumed to be true.
    cJSON *log_level = cJSON_GetObjectItem(opts, "override_log_level");
    if (!log_level) {
      this->override_log_level = true;
    } else if (log_level->type != cJSON_True &&
               log_level->type != cJSON_False) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_DEVICE,
                 "{}: SwitchOption json parsing failed for device {} file {} "
                 "option {}",
                 __func__,
                 dev_id,
                 conf_file,
                 "override_log_level");
      return -1;
    } else {
      this->override_log_level = (log_level->type == cJSON_True);
    }
  }

  if (!found_chip || !found_opts) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}: SwitchOption json parsing failed for device {} file {} "
               "chip_list and/or switch_options not found",
               __func__,
               dev_id,
               conf_file);
    return -1;
  }
  return 0;
}

}  // namespace bf_switch

#ifdef __cplusplus
extern "C" {
#endif

switch_status_t bf_switch_table_info_get_c(uint64_t table_id,
                                           switch_table_info_t *table_entry) {
  switch_table_info_t table_info = {};
  switch_status_t status = smi::table_info_get(
      static_cast<switch_device_attr_table>(table_id), table_info);
  if (table_entry) {
    table_entry->table_size = table_info.table_size;
    table_entry->table_usage = table_info.table_usage;
    table_entry->bf_rt_table_id = table_info.bf_rt_table_id;
  }
  return status;
}

#ifdef __cplusplus
}
#endif
