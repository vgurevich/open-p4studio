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


#include <math.h>

#include <utility>
#include <set>

#include "switch_tna/utils.h"
#include "switch_tna/p4_16_types.h"

namespace smi {
using namespace smi::bf_rt;  // NOLINT(build/namespaces)
using ::smi::logging::switch_log;

class etrap_ipv4_acl : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_ETRAP_IPV4_ACL;
  static const switch_attr_id_t status_attr_id =
      SWITCH_ETRAP_IPV4_ACL_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_ETRAP_IPV4_ACL_ATTR_PARENT_HANDLE;

  switch_enum_t type = {.enumdata = SWITCH_ETRAP_ACL_ENTRY_ATTR_TYPE_NONE};
  switch_ip_address_t src_ip = {};
  switch_ip_address_t src_ip_mask = {};
  switch_ip_address_t dst_ip = {};
  switch_ip_address_t dst_ip_mask = {};
  switch_object_id_t meter_handle = {0};
  uint8_t tc = 0;

 public:
  etrap_ipv4_acl(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_ETRAP_IPV4_ACL,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    status |=
        switch_store::v_get(parent, SWITCH_ETRAP_ACL_ENTRY_ATTR_TYPE, type);
    if (type.enumdata != SWITCH_ETRAP_ACL_ENTRY_ATTR_TYPE_IPV4) {
      return;
    }

    status |=
        switch_store::v_get(parent, SWITCH_ETRAP_ACL_ENTRY_ATTR_SRC_IP, src_ip);
    status |= switch_store::v_get(
        parent, SWITCH_ETRAP_ACL_ENTRY_ATTR_SRC_IP_MASK, src_ip_mask);
    status |=
        switch_store::v_get(parent, SWITCH_ETRAP_ACL_ENTRY_ATTR_DST_IP, dst_ip);
    status |= switch_store::v_get(
        parent, SWITCH_ETRAP_ACL_ENTRY_ATTR_DST_IP_MASK, dst_ip_mask);
    status |= switch_store::v_get(
        parent, SWITCH_ETRAP_ACL_ENTRY_ATTR_METER_HANDLE, meter_handle);
    status |= switch_store::v_get(parent, SWITCH_ETRAP_ACL_ENTRY_ATTR_TC, tc);

    status |=
        match_key.set_ternary(smi_id::F_ETRAP_IPV4_ACL_LOCAL_MD_LKP_IP_SRC_ADDR,
                              src_ip.ip4,
                              src_ip_mask.ip4);
    status |=
        match_key.set_ternary(smi_id::F_ETRAP_IPV4_ACL_LOCAL_MD_LKP_IP_DST_ADDR,
                              dst_ip.ip4,
                              dst_ip_mask.ip4);
    action_entry.init_action_data(smi_id::A_ETRAP_IPV4_ACL_SET_METER_AND_TC);
    if (meter_handle.data != 0) {
      status |= action_entry.set_arg(
          smi_id::P_ETRAP_IPV4_ACL_SET_METER_AND_TC_INDEX, meter_handle);
    }
    status |=
        action_entry.set_arg(smi_id::P_ETRAP_IPV4_ACL_SET_METER_AND_TC_TC, tc);
  }
};

class etrap_ipv6_acl : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_ETRAP_IPV6_ACL;
  static const switch_attr_id_t status_attr_id =
      SWITCH_ETRAP_IPV6_ACL_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_ETRAP_IPV6_ACL_ATTR_PARENT_HANDLE;

  switch_enum_t type = {.enumdata = SWITCH_ETRAP_ACL_ENTRY_ATTR_TYPE_NONE};
  switch_ip_address_t src_ip = {};
  switch_ip_address_t src_ip_mask = {};
  switch_ip_address_t dst_ip = {};
  switch_ip_address_t dst_ip_mask = {};
  switch_object_id_t meter_handle = {0};
  uint8_t tc = 0;

 public:
  etrap_ipv6_acl(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_ETRAP_IPV6_ACL,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    status |=
        switch_store::v_get(parent, SWITCH_ETRAP_ACL_ENTRY_ATTR_TYPE, type);
    if (type.enumdata != SWITCH_ETRAP_ACL_ENTRY_ATTR_TYPE_IPV6) {
      return;
    }

    status |=
        switch_store::v_get(parent, SWITCH_ETRAP_ACL_ENTRY_ATTR_SRC_IP, src_ip);
    status |= switch_store::v_get(
        parent, SWITCH_ETRAP_ACL_ENTRY_ATTR_SRC_IP_MASK, src_ip_mask);
    status |=
        switch_store::v_get(parent, SWITCH_ETRAP_ACL_ENTRY_ATTR_DST_IP, dst_ip);
    status |= switch_store::v_get(
        parent, SWITCH_ETRAP_ACL_ENTRY_ATTR_DST_IP_MASK, dst_ip_mask);
    status |= switch_store::v_get(
        parent, SWITCH_ETRAP_ACL_ENTRY_ATTR_METER_HANDLE, meter_handle);
    status |= switch_store::v_get(parent, SWITCH_ETRAP_ACL_ENTRY_ATTR_TC, tc);

    // Take only lower 64 bits of IPv6
    const char *src_ip_ptr = reinterpret_cast<char *>(&src_ip.ip6[8]);
    const char *src_ip_mask_ptr = reinterpret_cast<char *>(&src_ip_mask.ip6[8]);
    const char *dst_ip_ptr = reinterpret_cast<char *>(&dst_ip.ip6[8]);
    const char *dst_ip_mask_ptr = reinterpret_cast<char *>(&dst_ip_mask.ip6[8]);

    status |=
        match_key.set_ternary(smi_id::F_ETRAP_IPV6_ACL_LOCAL_MD_LKP_IP_SRC_ADDR,
                              src_ip_ptr,
                              src_ip_mask_ptr,
                              8);
    status |=
        match_key.set_ternary(smi_id::F_ETRAP_IPV6_ACL_LOCAL_MD_LKP_IP_DST_ADDR,
                              dst_ip_ptr,
                              dst_ip_mask_ptr,
                              8);
    action_entry.init_action_data(smi_id::A_ETRAP_IPV6_ACL_SET_METER_AND_TC);
    if (meter_handle.data != 0) {
      status |= action_entry.set_arg(
          smi_id::P_ETRAP_IPV6_ACL_SET_METER_AND_TC_INDEX, meter_handle);
    }
    status |=
        action_entry.set_arg(smi_id::P_ETRAP_IPV6_ACL_SET_METER_AND_TC_TC, tc);
  }
};

static uint64_t ip6_addr_to_uint64(switch_ip6_t ip, bool lower_bytes = true) {
  uint64_t converted_ip = 0;
  uint8_t start_index = (lower_bytes) ? (IPV6_LEN / 2) : 0;
  uint8_t end_index = (lower_bytes) ? (IPV6_LEN - 1) : (IPV6_LEN / 2 - 1);

  for (uint8_t i = start_index; i <= end_index; i++) {
    converted_ip |= ip[i];
    if (i != end_index) {
      converted_ip <<= 8;
    }
  }

  return converted_ip;
}

static void uint64_to_ip6_addr(uint64_t ipu64,
                               switch_ip6_t ip6,
                               bool lower_bytes = true) {
  uint8_t start_index = (lower_bytes) ? (IPV6_LEN - 1) : (IPV6_LEN / 2 - 1);
  uint8_t end_index = (lower_bytes) ? (IPV6_LEN / 2) : 0;

  for (uint8_t i = start_index; i >= end_index; i--) {
    ip6[i] = (uint8_t)ipu64;
    ipu64 >>= 8;
  }
}

static switch_status_t etrap_meter_create(
    const switch_object_id_t meter_handle,
    switch_object_id_t &etrap_meter_handle) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t device_handle = {0};
  uint64_t cbs = 0;
  uint64_t pbs = 0;
  uint64_t cir = 0;
  uint64_t pir = 0;
  std::set<attr_w> etrap_meter_attrs;

  if (meter_handle.data == 0) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ETRAP_METER,
               "{}:{}: Etrap meter creation failed - invalid arguments passed",
               __func__,
               __LINE__);
    return SWITCH_STATUS_INVALID_PARAMETER;
  }

  status |= switch_store::v_get(
      meter_handle, SWITCH_METER_ATTR_DEVICE, device_handle);
  status |= switch_store::v_get(meter_handle, SWITCH_METER_ATTR_CBS, cbs);
  status |= switch_store::v_get(meter_handle, SWITCH_METER_ATTR_PBS, pbs);
  status |= switch_store::v_get(meter_handle, SWITCH_METER_ATTR_CIR, cir);
  status |= switch_store::v_get(meter_handle, SWITCH_METER_ATTR_PIR, pir);

  etrap_meter_attrs.insert(
      attr_w(SWITCH_ETRAP_METER_ATTR_DEVICE, device_handle));
  etrap_meter_attrs.insert(attr_w(SWITCH_ETRAP_METER_ATTR_CBS, cbs));
  etrap_meter_attrs.insert(attr_w(SWITCH_ETRAP_METER_ATTR_PBS, pbs));
  etrap_meter_attrs.insert(attr_w(SWITCH_ETRAP_METER_ATTR_CIR, cir));
  etrap_meter_attrs.insert(attr_w(SWITCH_ETRAP_METER_ATTR_PIR, pir));

  status |= switch_store::object_create(
      SWITCH_OBJECT_TYPE_ETRAP_METER, etrap_meter_attrs, etrap_meter_handle);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ETRAP_METER,
               "{}:{}: Etrap meter creation failed, error: {}",
               __func__,
               __LINE__,
               status);
  }
  status |= switch_store::v_set(
      etrap_meter_handle, SWITCH_ETRAP_METER_ATTR_INTERNAL_OBJECT, true);

  return status;
}

switch_status_t etraps_create(const switch_object_id_t acl_entry_handle) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t device_handle = {0};
  switch_object_id_t etrap_handle = {0};
  switch_object_id_t meter_handle = {0};
  switch_enum_t acl_dir = {SWITCH_ACL_TABLE_ATTR_DIRECTION_NONE};
  switch_enum_t acl_type = {SWITCH_ACL_TABLE_ATTR_TYPE_NONE};
  switch_enum_t etrap_type = {SWITCH_ETRAP_ACL_ENTRY_ATTR_TYPE_NONE};
  switch_ip_address_t src_ip;
  switch_ip_address_t src_ip_mask;
  switch_ip_address_t dst_ip;
  switch_ip_address_t dst_ip_mask;
  uint8_t tc = 0;

  if (acl_entry_handle.data == 0) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ETRAP_METER,
               "{}:{}: Etraps creation failed - invalid arguments passed",
               __func__,
               __LINE__);
    return SWITCH_STATUS_INVALID_PARAMETER;
  }

  if (switch_store::smiContext::context().in_warm_init()) {
    return status;
  }

  status = get_acl_table_type_dir(acl_entry_handle, acl_type, acl_dir);
  if (status != SWITCH_STATUS_SUCCESS) {
    return status;
  }

  if (acl_type.enumdata == SWITCH_ACL_TABLE_ATTR_TYPE_IPV4_ETRAP) {
    etrap_type.enumdata = SWITCH_ETRAP_ACL_ENTRY_ATTR_TYPE_IPV4;
  } else if (acl_type.enumdata == SWITCH_ACL_TABLE_ATTR_TYPE_IPV6_ETRAP) {
    etrap_type.enumdata = SWITCH_ETRAP_ACL_ENTRY_ATTR_TYPE_IPV6;
  } else {
    return status;
  }

  status |= switch_store::v_get(
      acl_entry_handle, SWITCH_ACL_ENTRY_ATTR_SRC_IP, src_ip);
  status |= switch_store::v_get(
      acl_entry_handle, SWITCH_ACL_ENTRY_ATTR_SRC_IP_MASK, src_ip_mask);
  status |= switch_store::v_get(
      acl_entry_handle, SWITCH_ACL_ENTRY_ATTR_DST_IP, dst_ip);
  status |= switch_store::v_get(
      acl_entry_handle, SWITCH_ACL_ENTRY_ATTR_DST_IP_MASK, dst_ip_mask);
  status |= switch_store::v_get(
      acl_entry_handle, SWITCH_ACL_ENTRY_ATTR_DEVICE, device_handle);
  status |= switch_store::v_get(acl_entry_handle,
                                SWITCH_ACL_ENTRY_ATTR_ACTION_METER_HANDLE,
                                meter_handle);
  status |= switch_store::v_get(
      acl_entry_handle, SWITCH_ACL_ENTRY_ATTR_ACTION_SET_TC, tc);

  if (etrap_type.enumdata == SWITCH_ETRAP_ACL_ENTRY_ATTR_TYPE_IPV4) {
    if ((src_ip_mask.ip4 == 0) && (dst_ip_mask.ip4 == 0)) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_ETRAP_METER,
                 "{}:{}: Etraps creation failed - both SRC and DST IP addr ANY "
                 "is not alowed",
                 __func__,
                 __LINE__);
      return SWITCH_STATUS_FAILURE;
    }

    uint32_t src_hosts_cnt = (src_ip_mask.ip4) ? (~src_ip_mask.ip4 + 1) : 1;
    uint32_t dst_hosts_cnt = (dst_ip_mask.ip4) ? (~dst_ip_mask.ip4 + 1) : 1;

    for (uint32_t i = 0; i < src_hosts_cnt; i++) {
      switch_ip_address_t etrap_src_ip = {.addr_family =
                                              SWITCH_IP_ADDR_FAMILY_IPV4};
      switch_ip_address_t etrap_src_ip_mask = {.addr_family =
                                                   SWITCH_IP_ADDR_FAMILY_IPV4};

      etrap_src_ip.ip4 = (src_ip.ip4 & src_ip_mask.ip4) + i;
      etrap_src_ip_mask.ip4 = (src_ip_mask.ip4) ? 0xFFFFFFFF : 0;

      for (uint32_t j = 0; j < dst_hosts_cnt; j++) {
        switch_ip_address_t etrap_dst_ip = {.addr_family =
                                                SWITCH_IP_ADDR_FAMILY_IPV4};
        switch_ip_address_t etrap_dst_ip_mask = {
            .addr_family = SWITCH_IP_ADDR_FAMILY_IPV4};

        std::set<attr_w> etrap_attrs;
        switch_object_id_t etrap_meter_handle = {0};

        etrap_dst_ip.ip4 = (dst_ip.ip4 & dst_ip_mask.ip4) + j;
        etrap_dst_ip_mask.ip4 = (dst_ip_mask.ip4) ? 0xFFFFFFFF : 0;

        if (meter_handle.data != 0) {
          status = etrap_meter_create(meter_handle, etrap_meter_handle);
          if (status != SWITCH_STATUS_SUCCESS) {
            return status;
          }
        }

        etrap_attrs.insert(
            attr_w(SWITCH_ETRAP_ACL_ENTRY_ATTR_DEVICE, device_handle));
        etrap_attrs.insert(
            attr_w(SWITCH_ETRAP_ACL_ENTRY_ATTR_TYPE, etrap_type));
        etrap_attrs.insert(
            attr_w(SWITCH_ETRAP_ACL_ENTRY_ATTR_SRC_IP, etrap_src_ip));
        etrap_attrs.insert(
            attr_w(SWITCH_ETRAP_ACL_ENTRY_ATTR_SRC_IP_MASK, etrap_src_ip_mask));
        etrap_attrs.insert(
            attr_w(SWITCH_ETRAP_ACL_ENTRY_ATTR_DST_IP, etrap_dst_ip));
        etrap_attrs.insert(
            attr_w(SWITCH_ETRAP_ACL_ENTRY_ATTR_DST_IP_MASK, etrap_dst_ip_mask));
        etrap_attrs.insert(attr_w(SWITCH_ETRAP_ACL_ENTRY_ATTR_METER_HANDLE,
                                  etrap_meter_handle));
        etrap_attrs.insert(attr_w(SWITCH_ETRAP_ACL_ENTRY_ATTR_ACL_ENTRY_HANDLE,
                                  acl_entry_handle));
        etrap_attrs.insert(attr_w(SWITCH_ETRAP_ACL_ENTRY_ATTR_TC, tc));

        status |= switch_store::object_create(
            SWITCH_OBJECT_TYPE_ETRAP_ACL_ENTRY, etrap_attrs, etrap_handle);
        if (status != SWITCH_STATUS_SUCCESS) {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_ETRAP_ACL_ENTRY,
                     "{}:{}: IPv4 etrap creation failed, error: {}",
                     __func__,
                     __LINE__,
                     status);
          return status;
        }
        status |= switch_store::v_set(
            etrap_handle, SWITCH_ETRAP_ACL_ENTRY_ATTR_INTERNAL_OBJECT, true);
      }
    }
  } else {
    uint64_t src_ip_u64 = ip6_addr_to_uint64(src_ip.ip6);
    uint64_t src_ip_mask_u64 = ip6_addr_to_uint64(src_ip_mask.ip6);
    uint64_t dst_ip_u64 = ip6_addr_to_uint64(dst_ip.ip6);
    uint64_t dst_ip_mask_u64 = ip6_addr_to_uint64(dst_ip_mask.ip6);
    uint64_t src_hosts_cnt = (src_ip_mask_u64) ? (~src_ip_mask_u64 + 1) : 1;
    uint64_t dst_hosts_cnt = (dst_ip_mask_u64) ? (~dst_ip_mask_u64 + 1) : 1;

    if ((src_ip_mask_u64 == 0) && (dst_ip_mask_u64 == 0)) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_ETRAP_ACL_ENTRY,
                 "{}:{}: Etraps v6 creation failed - both SRC and DST IP addr "
                 "ANY is not alowed",
                 __func__,
                 __LINE__);
      return SWITCH_STATUS_FAILURE;
    }

    src_ip_u64 &= src_ip_mask_u64;
    dst_ip_u64 &= dst_ip_mask_u64;

    for (uint32_t i = 0; i < src_hosts_cnt; i++) {
      switch_ip_address_t etrap_src_ip = src_ip;
      switch_ip_address_t etrap_src_ip_mask = {.addr_family =
                                                   SWITCH_IP_ADDR_FAMILY_IPV6};

      etrap_src_ip.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6;
      // Copy lower bytes of expanded source IP
      uint64_to_ip6_addr((src_ip_u64 + i), etrap_src_ip.ip6);
      // Set exact match mask if source IP is expanded otherwise 0
      memset(etrap_src_ip_mask.ip6, (src_ip_mask_u64) ? 0xff : 0, IPV6_LEN);

      for (uint32_t j = 0; j < dst_hosts_cnt; j++) {
        switch_ip_address_t etrap_dst_ip = dst_ip;
        switch_ip_address_t etrap_dst_ip_mask = {
            .addr_family = SWITCH_IP_ADDR_FAMILY_IPV6};
        std::set<attr_w> etrap_attrs;
        switch_object_id_t etrap_meter_handle = {0};

        etrap_dst_ip.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6;
        // Copy lower bytes of expanded destination IP
        uint64_to_ip6_addr((dst_ip_u64 + j), etrap_dst_ip.ip6);
        // Set exact match mask if destination IP is expanded otherwise 0
        memset(etrap_dst_ip_mask.ip6, (dst_ip_mask_u64) ? 0xff : 0, IPV6_LEN);

        if (meter_handle.data != 0) {
          status = etrap_meter_create(meter_handle, etrap_meter_handle);
          if (status != SWITCH_STATUS_SUCCESS) {
            return status;
          }
        }

        etrap_attrs.insert(
            attr_w(SWITCH_ETRAP_ACL_ENTRY_ATTR_DEVICE, device_handle));
        etrap_attrs.insert(
            attr_w(SWITCH_ETRAP_ACL_ENTRY_ATTR_TYPE, etrap_type));
        etrap_attrs.insert(
            attr_w(SWITCH_ETRAP_ACL_ENTRY_ATTR_SRC_IP, etrap_src_ip));
        etrap_attrs.insert(
            attr_w(SWITCH_ETRAP_ACL_ENTRY_ATTR_SRC_IP_MASK, etrap_src_ip_mask));
        etrap_attrs.insert(
            attr_w(SWITCH_ETRAP_ACL_ENTRY_ATTR_DST_IP, etrap_dst_ip));
        etrap_attrs.insert(
            attr_w(SWITCH_ETRAP_ACL_ENTRY_ATTR_DST_IP_MASK, etrap_dst_ip_mask));
        etrap_attrs.insert(attr_w(SWITCH_ETRAP_ACL_ENTRY_ATTR_METER_HANDLE,
                                  etrap_meter_handle));
        etrap_attrs.insert(attr_w(SWITCH_ETRAP_ACL_ENTRY_ATTR_ACL_ENTRY_HANDLE,
                                  acl_entry_handle));
        etrap_attrs.insert(attr_w(SWITCH_ETRAP_ACL_ENTRY_ATTR_TC, tc));

        status |= switch_store::object_create(
            SWITCH_OBJECT_TYPE_ETRAP_ACL_ENTRY, etrap_attrs, etrap_handle);
        if (status != SWITCH_STATUS_SUCCESS) {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_ETRAP_ACL_ENTRY,
                     "{}:{}: IPv6 etrap creation failed, error: {}",
                     __func__,
                     __LINE__,
                     status);
          return status;
        }
        status |= switch_store::v_set(
            etrap_handle, SWITCH_ETRAP_ACL_ENTRY_ATTR_INTERNAL_OBJECT, true);
      }
    }
  }

  return status;
}

switch_status_t etraps_remove(const switch_object_id_t acl_entry_handle) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::set<switch_object_id_t> etrap_handles;

  if (acl_entry_handle.data == 0) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ETRAP_ACL_ENTRY,
               "{}:{}: Etraps removal failed - invalid arguments passed",
               __func__,
               __LINE__);
    return SWITCH_STATUS_INVALID_PARAMETER;
  }

  status |= switch_store::referencing_set_get(
      acl_entry_handle, SWITCH_OBJECT_TYPE_ETRAP_ACL_ENTRY, etrap_handles);

  for (auto etrap : etrap_handles) {
    switch_object_id_t etrap_meter_handle = {0};
    status |= switch_store::v_get(
        etrap, SWITCH_ETRAP_ACL_ENTRY_ATTR_METER_HANDLE, etrap_meter_handle);

    status |= switch_store::object_delete(etrap);
    if (etrap_meter_handle.data != 0) {
      status |= switch_store::object_delete(etrap_meter_handle);
    }
  }

  return status;
}

switch_status_t etraps_update(const switch_object_id_t acl_entry_handle,
                              const attr_w &attr) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::set<switch_object_id_t> etrap_handles;
  uint8_t tc = 0;

  if (acl_entry_handle.data == 0) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ETRAP_ACL_ENTRY,
               "{}:{}: Etraps update failed - invalid arguments passed",
               __func__,
               __LINE__);
    return SWITCH_STATUS_INVALID_PARAMETER;
  }

  if (attr.id_get() != SWITCH_ACL_ENTRY_ATTR_ACTION_SET_TC) {
    return SWITCH_STATUS_SUCCESS;
  }

  attr.v_get(tc);

  status |= switch_store::referencing_set_get(
      acl_entry_handle, SWITCH_OBJECT_TYPE_ETRAP_ACL_ENTRY, etrap_handles);

  for (auto etrap : etrap_handles) {
    status |= switch_store::v_set(etrap, SWITCH_ETRAP_ACL_ENTRY_ATTR_TC, tc);
  }

  return status;
}

switch_status_t etrap_meters_remove(const switch_object_id_t acl_entry_handle) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::set<switch_object_id_t> etrap_handles;

  if (acl_entry_handle.data == 0) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ETRAP_METER,
               "{}:{}: Etraps removal failed - invalid arguments passed",
               __func__,
               __LINE__);
    return SWITCH_STATUS_INVALID_PARAMETER;
  }

  status |= switch_store::referencing_set_get(
      acl_entry_handle, SWITCH_OBJECT_TYPE_ETRAP_ACL_ENTRY, etrap_handles);

  for (auto etrap : etrap_handles) {
    switch_object_id_t etrap_meter_handle = {0};
    status |= switch_store::v_get(
        etrap, SWITCH_ETRAP_ACL_ENTRY_ATTR_METER_HANDLE, etrap_meter_handle);

    if (etrap_meter_handle.data != 0) {
      switch_object_id_t dummy_oid = {0};
      status |= switch_store::v_set(
          etrap, SWITCH_ETRAP_ACL_ENTRY_ATTR_METER_HANDLE, dummy_oid);
      status |= switch_store::object_delete(etrap_meter_handle);
    }
  }

  return status;
}

switch_status_t etrap_meters_create(const switch_object_id_t acl_entry_handle) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::set<switch_object_id_t> etrap_handles;
  switch_object_id_t meter_handle = {0};

  if (acl_entry_handle.data == 0) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ETRAP_METER,
               "{}:{}: Etraps removal failed - invalid arguments passed",
               __func__,
               __LINE__);
    return SWITCH_STATUS_INVALID_PARAMETER;
  }

  status |= switch_store::v_get(acl_entry_handle,
                                SWITCH_ACL_ENTRY_ATTR_ACTION_METER_HANDLE,
                                meter_handle);

  if (meter_handle.data == 0) {
    return SWITCH_STATUS_SUCCESS;
  }

  status |= switch_store::referencing_set_get(
      acl_entry_handle, SWITCH_OBJECT_TYPE_ETRAP_ACL_ENTRY, etrap_handles);

  for (auto etrap : etrap_handles) {
    switch_object_id_t etrap_meter_handle = {0};

    status |= etrap_meter_create(meter_handle, etrap_meter_handle);
    if (status != SWITCH_STATUS_SUCCESS) {
      return status;
    }

    status |= switch_store::v_set(
        etrap, SWITCH_ETRAP_ACL_ENTRY_ATTR_METER_HANDLE, etrap_meter_handle);
  }

  return status;
}

class etrap_meter_table : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_ETRAP_METER_TABLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_ETRAP_METER_TABLE_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_ETRAP_METER_TABLE_ATTR_PARENT_HANDLE;
  uint64_t _cir_bps = 0;
  uint64_t _pir_bps = 0;
  uint64_t _cburst_bytes = 0;
  uint64_t _pburst_bytes = 0;

 public:
  switch_status_t program_etrap_meter(switch_object_id_t meter,
                                      uint64_t cir_bps,
                                      uint64_t pir_bps,
                                      uint64_t cburst_bytes,
                                      uint64_t pburst_bytes) {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_ETRAP_METER);
    _MatchKey meter_key(smi_id::T_ETRAP_METER);
    _ActionEntry meter_action(smi_id::T_ETRAP_METER);

    status |= meter_key.set_exact(smi_id::F_ETRAP_METER_METER_INDEX, meter);
    status |= meter_action.init_indirect_data();
    status |= meter_action.set_arg(smi_id::D_ETRAP_METER_METER_SPEC_CIR_KBPS,
                                   switch_meter_bytes_to_kbps(cir_bps));
    status |= meter_action.set_arg(smi_id::D_ETRAP_METER_METER_SPEC_PIR_KBPS,
                                   switch_meter_bytes_to_kbps(pir_bps));
    status |= meter_action.set_arg(smi_id::D_ETRAP_METER_METER_SPEC_CBS_KBITS,
                                   switch_meter_bytes_to_kbps(cburst_bytes));
    status |= meter_action.set_arg(smi_id::D_ETRAP_METER_METER_SPEC_PBS_KBITS,
                                   switch_meter_bytes_to_kbps(pburst_bytes));
    status |= table.entry_modify(meter_key, meter_action);

    return status;
  }

  etrap_meter_table(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status |=
        switch_store::v_get(parent, SWITCH_ETRAP_METER_ATTR_CIR, _cir_bps);
    status |=
        switch_store::v_get(parent, SWITCH_ETRAP_METER_ATTR_PIR, _pir_bps);
    status |=
        switch_store::v_get(parent, SWITCH_ETRAP_METER_ATTR_CBS, _cburst_bytes);
    status |=
        switch_store::v_get(parent, SWITCH_ETRAP_METER_ATTR_PBS, _pburst_bytes);
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    status |= program_etrap_meter(
        get_parent(), _cir_bps, _pir_bps, _cburst_bytes, _pburst_bytes);
    status |= auto_object::create_update();

    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    status |= program_etrap_meter(get_parent(), 0, 0, 0, 0);

    // Clear etrap state register.
    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_ETRAP_STATE_REG);
    _MatchKey register_key(smi_id::T_ETRAP_STATE_REG);
    _ActionEntry register_action(smi_id::T_ETRAP_STATE_REG);

    register_action.init_indirect_data();
    status |= register_action.set_arg(smi_id::D_ETRAP_STATE_REG_RESULT_VALUE,
                                      static_cast<uint8_t>(0));
    status |= register_key.set_exact(smi_id::F_ETRAP_STATE_REG_REGISTER_INDEX,
                                     get_parent());
    status |= table.entry_modify(register_key, register_action);

    status |= auto_object::del();

    return status;
  }
};

class etrap_meter_index : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_ETRAP_METER_INDEX;
  static const switch_attr_id_t status_attr_id =
      SWITCH_ETRAP_METER_INDEX_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_ETRAP_METER_INDEX_ATTR_PARENT_HANDLE;

 public:
  etrap_meter_index(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_ETRAP_METER_INDEX,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    status |= match_key.set_exact(
        smi_id::F_ETRAP_METER_INDEX_LOCAL_MD_QOS_ETRAP_INDEX, parent);
    action_entry.init_action_data(smi_id::A_ETRAP_METER_INDEX_ACTION);
    status |=
        action_entry.set_arg(smi_id::P_ETRAP_METER_INDEX_ACTION_INDEX, parent);
  }
};

class etrap_meter_state : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_ETRAP_METER_STATE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_ETRAP_METER_STATE_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_ETRAP_METER_STATE_ATTR_PARENT_HANDLE;

 public:
  etrap_meter_state(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_ETRAP_METER_STATE,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    status |= match_key.set_exact(
        smi_id::F_ETRAP_METER_STATE_LOCAL_MD_QOS_ETRAP_INDEX, parent);
    action_entry.init_action_data(smi_id::A_ETRAP_METER_STATE_ACTION);
  }
};

// Unused class. The entries are programmed as static entries in P4
class etrap_state : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_ETRAP_STATE;
  static const switch_attr_id_t status_attr_id = SWITCH_ETRAP_STATE_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_ETRAP_STATE_ATTR_PARENT_HANDLE;

 public:
  etrap_state(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_ETRAP_STATE,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    if (feature::is_feature_set(SWITCH_FEATURE_ETRAP)) {
      auto it = match_action_list.begin();

      // SWITCH_METER_COLOR_GREEN : report_etrap_state
      it = match_action_list.insert(it,
                                    std::pair<_MatchKey, _ActionEntry>(
                                        _MatchKey(smi_id::T_ETRAP_STATE),
                                        _ActionEntry(smi_id::T_ETRAP_STATE)));
      status |=
          it->first.set_exact(smi_id::F_ETRAP_STATE_LOCAL_MD_QOS_ETRAP_COLOR,
                              static_cast<uint8_t>(0x0));
      it->second.init_action_data(smi_id::A_ETRAP_STATE_ETRAP_GREEN_STATE);

      // SWITCH_METER_COLOR_RED : set_etrap_state
      it = match_action_list.insert(it,
                                    std::pair<_MatchKey, _ActionEntry>(
                                        _MatchKey(smi_id::T_ETRAP_STATE),
                                        _ActionEntry(smi_id::T_ETRAP_STATE)));
      status |=
          it->first.set_exact(smi_id::F_ETRAP_STATE_LOCAL_MD_QOS_ETRAP_COLOR,
                              static_cast<uint8_t>(0x3));
      it->second.init_action_data(smi_id::A_ETRAP_STATE_ETRAP_RED_STATE);
    }
  }
};

switch_status_t etrap_init() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  REGISTER_OBJECT(etrap_ipv4_acl, SWITCH_OBJECT_TYPE_ETRAP_IPV4_ACL);
  REGISTER_OBJECT(etrap_ipv6_acl, SWITCH_OBJECT_TYPE_ETRAP_IPV6_ACL);
  REGISTER_OBJECT(etrap_meter_table, SWITCH_OBJECT_TYPE_ETRAP_METER_TABLE);
  REGISTER_OBJECT(etrap_meter_index, SWITCH_OBJECT_TYPE_ETRAP_METER_INDEX);
  REGISTER_OBJECT(etrap_meter_state, SWITCH_OBJECT_TYPE_ETRAP_METER_STATE);
  // REGISTER_OBJECT(etrap_state, SWITCH_OBJECT_TYPE_ETRAP_STATE);

  return status;
}

switch_status_t etrap_clean() { return SWITCH_STATUS_SUCCESS; }

}  // namespace smi
