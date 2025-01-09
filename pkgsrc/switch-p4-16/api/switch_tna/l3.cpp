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


#include <arpa/inet.h>

#include <memory>
#include <utility>
#include <set>
#include <unordered_map>
#include <functional>
#include <vector>

#include "switch_tna/utils.h"
#include "switch_tna/minimal_match_action.h"

namespace smi {
using namespace smi::bf_rt;  // NOLINT(build/namespaces)
using ::smi::logging::switch_log;

// helper macro to define the various fib bfrt objects
#define FIB_TABLE(x)                             \
  std::unique_ptr<_Table> x##_table;             \
  std::unique_ptr<_MatchKey> x##_match_key;      \
  std::unique_ptr<_ActionEntry> x##_action_myip; \
  std::unique_ptr<_ActionEntry> x##_action_hit;  \
  std::unique_ptr<_ActionEntry> x##_action_drop;

// helper macro to initialize the various fib bfrt objects
#define FIB_TABLE_INIT(x, y)                                                   \
  x##_table =                                                                  \
      std::unique_ptr<_Table>(new _Table(get_dev_tgt(), get_bf_rt_info(), y)); \
  x##_match_key = std::unique_ptr<_MatchKey>(new _MatchKey(y));                \
  x##_action_myip = std::unique_ptr<_ActionEntry>(new _ActionEntry(y));        \
  x##_action_myip->init_action_data(smi_id::A_FIB_MYIP);                       \
  x##_action_hit = std::unique_ptr<_ActionEntry>(new _ActionEntry(y));         \
  x##_action_hit->init_action_data(smi_id::A_FIB_HIT);                         \
  x##_action_drop = std::unique_ptr<_ActionEntry>(new _ActionEntry(y));        \
  x##_action_drop->init_action_data(smi_id::A_FIB_DROP);

// helper macro to pick action based on myip_type (y)
#define FIB_CREATE_UPDATE(x, y, z)                           \
  switch (y.enumdata) {                                      \
    case SWITCH_DEVICE_ATTR_MYIP_TYPE_HOST:                  \
    case SWITCH_DEVICE_ATTR_MYIP_TYPE_SUBNET:                \
      status = minimal_match_action::create_update(          \
          *x##_table, *x##_match_key, *x##_action_myip);     \
      break;                                                 \
    default: {                                               \
      switch (z.enumdata) {                                  \
        case SWITCH_ROUTE_ATTR_PACKET_ACTION_DROP:           \
        case SWITCH_ROUTE_ATTR_PACKET_ACTION_DENY:           \
          status = minimal_match_action::create_update(      \
              *x##_table, *x##_match_key, *x##_action_drop); \
          break;                                             \
        case SWITCH_ROUTE_ATTR_PACKET_ACTION_TRAP:           \
        case SWITCH_ROUTE_ATTR_PACKET_ACTION_FORWARD:        \
        case SWITCH_ROUTE_ATTR_PACKET_ACTION_TRANSIT:        \
          status = minimal_match_action::create_update(      \
              *x##_table, *x##_match_key, *x##_action_hit);  \
          break;                                             \
        default:                                             \
          break;                                             \
      }                                                      \
    }                                                        \
  }

FIB_TABLE(v4_host);
FIB_TABLE(v4_local_host);
FIB_TABLE(v4_lpm);
FIB_TABLE(v6_host);
FIB_TABLE(v6_host64);
FIB_TABLE(v6_lpm);
FIB_TABLE(v6_lpm64);
FIB_TABLE(v6_lpm_tcam);
FIB_TABLE(shared_lpm64);

std::unordered_map<uint64_t, bfrtCacheObject> &p4_match_action_cache =
    bfrtCache::cache().p4_match_action_cache();

/******************************************************************************
 * minimal_match_action
 *****************************************************************************/
minimal_match_action::minimal_match_action(
    const switch_attr_id_t status_attr_id,
    const switch_object_type_t auto_ot,
    const switch_attr_id_t parent_attr_id,
    const switch_object_id_t parent)
    : auto_obj(auto_ot, parent_attr_id, parent, status_attr_id, true),
      table_dev_tgt(get_dev_tgt()),
      _status_attr_id(status_attr_id) {}

switch_status_t minimal_match_action::create_update(
    _Table &table, _MatchKey &match_key, _ActionEntry &action_entry) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  bool is_update = false;

  if (auto_obj.get_auto_oid() != 0) is_update = true;

  // status attribute is reset during warm init in store.cpp:db_load
  // Set it here
  if (switch_store::smiContext::context().in_warm_init()) {
    status |= switch_store::v_set(
        auto_obj.get_auto_oid(), _status_attr_id, is_update);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OT_NONE,
                 "{}:{}: failed to set bf_rt_status status {}",
                 __func__,
                 __LINE__,
                 status);
      return status;
    }
  }
  status = auto_obj.create_update_minimal();
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OT_NONE,
               "{}: auto_obj.create_update_minimal failure status {}",
               __func__,
               status);
    return status;
  }

  status = pi_create_update(is_update, table, match_key, action_entry);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OT_NONE,
               "{}:{}: pi_create_update failure status {}",
               __func__,
               __LINE__,
               status);
    if (auto_obj.del() != SWITCH_STATUS_SUCCESS) {
      switch_log(
          SWITCH_API_LEVEL_ERROR,
          SWITCH_OT_NONE,
          "{}:{}: failed auto_obj cleanup during create_update status {}",
          __func__,
          __LINE__,
          status);
    }
  }

  return status;
}

switch_status_t minimal_match_action::del(_Table &table, _MatchKey &match_key) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  bool bf_rt_status = false;

  if (auto_obj.get_auto_oid() == 0) return SWITCH_STATUS_SUCCESS;

  status |= switch_store::v_get(
      auto_obj.get_auto_oid(), _status_attr_id, bf_rt_status);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OT_NONE,
               "{}:{}: failed to get bf_rt_status status {}",
               __func__,
               __LINE__,
               status);
    return status;
  }

  status = pi_del(bf_rt_status, table, match_key);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OT_NONE,
               "{}:{}: pi_del failure status {}",
               __func__,
               __LINE__,
               status);
    return status;
  }

  status |= auto_obj.del();
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OT_NONE,
               "{}:{}: failed auto_obj.del status {}",
               __func__,
               __LINE__,
               status);
    return status;
  }

  return status;
}

switch_status_t minimal_match_action::pi_create_update(
    bool &bf_rt_status,
    _Table &table,
    _MatchKey &match_key,
    _ActionEntry &action_entry) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (bf_rt_status) {
    status = table.entry_modify(match_key, action_entry, false);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OT_NONE,
                 "{}:{}: failed table.entry_modify status {}",
                 __func__,
                 __LINE__,
                 status);
      return status;
    }
  } else {
    status = table.entry_add(match_key, action_entry, bf_rt_status, false);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OT_NONE,
                 "{}:{}: failed table.entry_add status {}",
                 __func__,
                 __LINE__,
                 status);
      return status;
    }
  }

  status = add_to_cache(table, match_key, action_entry);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OT_NONE,
               "{}:{}: failed add_to_cache status {}",
               __func__,
               __LINE__,
               status);
    return status;
  }

  return status;
}

switch_status_t minimal_match_action::pi_del(bool &bf_rt_status,
                                             _Table &table,
                                             _MatchKey &match_key) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (bf_rt_status) {
    status = table.entry_delete(match_key);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OT_NONE,
                 "{}:{}: failed table.entry_delete status {}",
                 __func__,
                 __LINE__,
                 status);
      return status;
    }
  }

  return status;
}

switch_status_t minimal_match_action::add_to_cache(_Table &table,
                                                   _MatchKey &match_key,
                                                   _ActionEntry &action_entry) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (!switch_store::smiContext::context().in_warm_init()) return status;

  uint64_t oid = get_auto_oid().data;
  if (oid == 0) return status;

  if (p4_match_action_cache.find(oid) != p4_match_action_cache.end()) {
    p4_match_action_cache.erase(oid);
  }
  p4_match_action_cache[oid] =
      table.create_cache_object(table_dev_tgt, match_key, action_entry);

  return SWITCH_STATUS_SUCCESS;
}

/*
 * default ipv4/ipv6 table selection:
 *    IPV4_HOST/IPV6_HOST = local/remote host
 *    IPV4_LPM/IPV6_LPM - prefix.
 *
 * with ipv4_local_host feature eenabled,
 *     IPV4_LOCAL_HOST - ARP sourced local HOST
 *     IPV4_HOST - Remote Host/MY_IP
 *     IPV4_LPM - prefix.
 */
switch_status_t get_fib_table_type(bool is_nbr_sourced,
                                   const switch_ip_prefix_t &prefix,
                                   switch_prefix_type_t &fib_tbl_type) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (prefix.addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4) {
    if (prefix.len == (sizeof(prefix.addr.ip4) * 8)) {
      if (is_nbr_sourced &&
          (feature::is_feature_set(SWITCH_FEATURE_IPV4_LOCAL_HOST))) {
        fib_tbl_type = IPV4_LOCAL_HOST;
      } else {
        fib_tbl_type = IPV4_HOST;
      }
    } else {
      if (feature::is_feature_set(SWITCH_FEATURE_SHARED_ALPM)) {
        fib_tbl_type = IP_LPM64;
      } else {
        fib_tbl_type = IPV4_LPM;
      }
    }
  } else if (prefix.addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6) {
    if (prefix.len == (sizeof(prefix.addr.ip6) * 8)) {
      fib_tbl_type = IPV6_HOST;
    } else if ((feature::is_feature_set(SWITCH_FEATURE_IPV6_HOST64)) &&
               (prefix.len == 64)) {
      fib_tbl_type = IPV6_HOST64;
    } else if ((feature::is_feature_set(SWITCH_FEATURE_SHARED_ALPM)) &&
               (prefix.len <= 64)) {
      fib_tbl_type = IP_LPM64;
    } else if ((feature::is_feature_set(SWITCH_FEATURE_IPV6_LPM64)) &&
               (prefix.len <= 64)) {
      fib_tbl_type = IPV6_LPM64;
    } else if (feature::is_feature_set(SWITCH_FEATURE_IPV6_FIB_LPM_TCAM)) {
      fib_tbl_type = IPV6_LPM_TCAM;
    } else {
      fib_tbl_type = IPV6_LPM;
    }
  } else {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ROUTE,
               "{}:{}: invalid IP address family {}",
               __func__,
               __LINE__,
               prefix.addr.addr_family);
    return SWITCH_STATUS_INVALID_PARAMETER;
  }
  return status;
}

static inline bool is_myip_trap(switch_myip_type_t &is_host_myip) {
  return ((is_host_myip & SWITCH_DEVICE_ATTR_MYIP_TYPE_MASK) ==
          SWITCH_DEVICE_ATTR_MYIP_TYPE_HOST);
}

static inline bool is_myip_subnet_trap(switch_myip_type_t &is_host_myip) {
  return ((is_host_myip & SWITCH_DEVICE_ATTR_MYIP_TYPE_MASK) ==
          SWITCH_DEVICE_ATTR_MYIP_TYPE_SUBNET);
}

/*
 * this API used for the case route installed with nexthop handle as RIF.
 * typical use case are proxy arp scenarios
 * SONIC uses this approach.  only one nexthop will be attached to connected
 * RIF.
 * If a nexthop is found, use it, else return glean nhop
 */
static switch_status_t search_nhop_handle_over_rif(
    const switch_ip_prefix_t route_subnet_ip,
    const switch_object_id_t route_vrf_handle,
    const switch_object_id_t hostif_nhop_handle,
    switch_object_id_t &nhop_handle) {
  switch_object_id_t rif_vrf_handle = {};
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t rif_handle = nhop_handle;
  std::set<switch_object_id_t> ref_oids;

  // By default we'll use glean NH
  nhop_handle = hostif_nhop_handle;

  switch_status = switch_store::referencing_set_get(
      rif_handle, SWITCH_OBJECT_TYPE_NEXTHOP, ref_oids);
  if (switch_status != SWITCH_STATUS_SUCCESS) {
    switch_log(
        SWITCH_API_LEVEL_ERROR,
        SWITCH_OBJECT_TYPE_ROUTE,
        "{}:{}: Failed to get nhop handle from rif_handle: {}, status {}",
        __func__,
        __LINE__,
        rif_handle.data,
        switch_error_to_string(switch_status));
    return switch_status;
  }

  if (!ref_oids.size()) {
    return SWITCH_STATUS_SUCCESS;
  }

  switch_status = switch_store::v_get(
      rif_handle, SWITCH_RIF_ATTR_VRF_HANDLE, rif_vrf_handle);
  if (switch_status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_ROUTE,
               "{}:{}: Failed to get vrf handle from rif_handle: {}, status {}",
               __func__,
               __LINE__,
               rif_handle.data,
               switch_error_to_string(switch_status));
    return switch_status;
  }

  for (auto nh : ref_oids) {
    switch_ip_address_t nh_ip = {};
    switch_enum_t nh_type = {};

    switch_status = switch_store::v_get(nh, SWITCH_NEXTHOP_ATTR_TYPE, nh_type);
    switch_status |=
        switch_store::v_get(nh, SWITCH_NEXTHOP_ATTR_DEST_IP, nh_ip);

    if (nh_type.enumdata != SWITCH_NEXTHOP_ATTR_TYPE_IP) {
      continue;
    }

    if (switch_status == SWITCH_STATUS_SUCCESS) {
      // If rif vrf is not the same as route vrf then pick non
      // dir_bcast and non glean nexthop. This is needed to pass SONiC vxlan CTs
      // which uses inter-vrf forwarding and expect that the packet forwarded
      // will be sent using the learned neighbor NH data instead of directed
      // broadcast NH.
      if (rif_vrf_handle.data != route_vrf_handle.data) {
        if (!attr_util::is_dir_bcast_addr(route_subnet_ip.len, nh_ip)) {
          nhop_handle = nh;
          return SWITCH_STATUS_SUCCESS;
        }
      } else if (switch_ip_prefix_is_host_ip(route_subnet_ip)) {
        switch_ip_address_t subnet_ip;
        switch_ip_prefix_to_ip_addr(route_subnet_ip, subnet_ip);

        // If this is a host IP then check if we have NH for it
        // otherwise just use glean
        if (switch_ip_addr_cmp(subnet_ip, nh_ip) == 0) {
          nhop_handle = nh;
          return SWITCH_STATUS_SUCCESS;
        }
      }
    }
  }

  return switch_status;
}

static void update_nhop_handle(const switch_object_id_t parent,
                               const switch_object_id_t device_handle,
                               const switch_object_id_t vrf_handle,
                               switch_ip_prefix_t &prefix,
                               switch_enum_t packet_action,
                               switch_object_id_t &nexthop_handle) {
  switch_object_id_t nhop_glean_handle = {0};
  switch_object_id_t nhop_drop_handle = {0};

  switch (packet_action.enumdata) {
    case SWITCH_ROUTE_ATTR_PACKET_ACTION_DROP:
    case SWITCH_ROUTE_ATTR_PACKET_ACTION_DENY: {
      switch_store::v_get(device_handle,
                          SWITCH_DEVICE_ATTR_DROP_NEXTHOP_HANDLE,
                          nhop_drop_handle);
      nexthop_handle = nhop_drop_handle;
      return;
    }
    case SWITCH_ROUTE_ATTR_PACKET_ACTION_TRAP: {
      switch_store::v_get(device_handle,
                          SWITCH_DEVICE_ATTR_GLEAN_NEXTHOP_HANDLE,
                          nhop_glean_handle);
      nexthop_handle = nhop_glean_handle;
      return;
    }
    case SWITCH_ROUTE_ATTR_PACKET_ACTION_FORWARD:
    case SWITCH_ROUTE_ATTR_PACKET_ACTION_TRANSIT:
      break;
    default:
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_ROUTE,
                 "{}:{}: invalid packet action: {}",
                 __func__,
                 __LINE__,
                 packet_action);
      return;
  }

  switch_object_id_t rif_handle = {};
  // if nexthop is a rif, search for nhop over this RIF
  if (switch_store::object_type_query(nexthop_handle) ==
      SWITCH_OBJECT_TYPE_RIF) {
    rif_handle = nexthop_handle;

    switch_store::v_get(device_handle,
                        SWITCH_DEVICE_ATTR_GLEAN_NEXTHOP_HANDLE,
                        nhop_glean_handle);
    // search and update NH, by default we'll use glean NH
    search_nhop_handle_over_rif(
        prefix, vrf_handle, nhop_glean_handle, nexthop_handle);
    switch_store::v_set(
        parent, SWITCH_ROUTE_ATTR_NEXTHOP_HANDLE, nexthop_handle);
    switch_store::v_set(parent, SWITCH_ROUTE_ATTR_RIF_HANDLE, rif_handle);
  } else {
    switch_store::v_set(parent, SWITCH_ROUTE_ATTR_RIF_HANDLE, rif_handle);
  }
}

class fib_factory : public minimal_match_action {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_FIB_FACTORY;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_FIB_FACTORY_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id = SWITCH_FIB_FACTORY_ATTR_STATUS;
  switch_prefix_type_t fib_tbl_type = IPV4_HOST;
  switch_enum_t myip_type = {};
  switch_enum_t packet_action = {};
  switch_object_id_t vrf_handle = {};
  bool is_nbr_sourced = false;
  switch_ip_prefix_t prefix = {};

 public:
  fib_factory(const switch_object_id_t parent, switch_status_t &status)
      : minimal_match_action(status_attr_id, auto_ot, parent_attr_id, parent) {
    switch_object_id_t nexthop_handle = {}, device_handle = {};
    uint8_t is_host_myip = 0;
    uint32_t fib_label = 0;

    std::vector<std::reference_wrapper<const switch_attribute_t>> route_attrs;
    route_attrs.reserve(16);

    status = switch_store::attribute_get_all(parent, route_attrs);
    for (const auto &ref_attr : route_attrs) {
      const switch_attribute_t &route_attr = ref_attr.get();
      switch (route_attr.id) {
        case SWITCH_ROUTE_ATTR_DEVICE:
          device_handle = route_attr.value.oid;
          break;
        case SWITCH_ROUTE_ATTR_VRF_HANDLE:
          vrf_handle = route_attr.value.oid;
          break;
        case SWITCH_ROUTE_ATTR_IP_PREFIX:
          attr_util::v_get(route_attr.value, prefix);
          break;
        case SWITCH_ROUTE_ATTR_NEXTHOP_HANDLE:
          nexthop_handle = route_attr.value.oid;
          break;
        case SWITCH_ROUTE_ATTR_PACKET_ACTION:
          packet_action.enumdata = route_attr.value.enumdata.enumdata;
          break;
        case SWITCH_ROUTE_ATTR_IS_HOST_MYIP:
          is_host_myip = route_attr.value.u8;
          break;
        case SWITCH_ROUTE_ATTR_IS_NBR_SOURCED:
          is_nbr_sourced = route_attr.value.booldata;
          break;
        case SWITCH_ROUTE_ATTR_FIB_LABEL:
          fib_label = route_attr.value.u32;
          break;
        case SWITCH_ROUTE_ATTR_IS_ROUTE_SOURCED:
        case SWITCH_ROUTE_ATTR_INTERNAL_OBJECT:
        case SWITCH_ROUTE_ATTR_RIF_HANDLE:
        case SWITCH_ROUTE_ATTR_NEIGHBOR_HANDLE:
          break;
        default:
          switch_log(
              SWITCH_API_LEVEL_ERROR,
              SWITCH_OBJECT_TYPE_ROUTE,
              "{}:{}: attribute_get_all fail invalid attr id {} route.{}",
              __func__,
              __LINE__,
              route_attr.id,
              switch_store::handle_to_id(parent));
          status = SWITCH_STATUS_FAILURE;
          return;
          break;
      }
    }
    update_nhop_handle(parent,
                       device_handle,
                       vrf_handle,
                       prefix,
                       packet_action,
                       nexthop_handle);

    if (is_myip_trap(is_host_myip)) {
      myip_type.enumdata = SWITCH_DEVICE_ATTR_MYIP_TYPE_HOST;
    } else if (is_myip_subnet_trap(is_host_myip)) {
      myip_type.enumdata = SWITCH_DEVICE_ATTR_MYIP_TYPE_SUBNET;
    } else {
      myip_type.enumdata = SWITCH_DEVICE_ATTR_MYIP_TYPE_NONE;
    }

    status = get_fib_table_type(is_nbr_sourced, prefix, fib_tbl_type);
    if (status != SWITCH_STATUS_SUCCESS) return;

    status = prepare_match_keys();
    if (status != SWITCH_STATUS_SUCCESS) return;

    status = prepare_action_data(nexthop_handle, fib_label);
    if (status != SWITCH_STATUS_SUCCESS) return;
  }

  switch_status_t prepare_match_keys() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    uint16_t vrf = compute_vrf(vrf_handle);
    switch_ip4_t ip4 = 0;

    switch (fib_tbl_type) {
      case IPV4_HOST:
        v4_host_match_key->reset();
        status |= v4_host_match_key->set_exact(smi_id::F_IPV4_FIB_VRF, vrf);
        ip4 = htonl(prefix.addr.ip4);
        status |=
            v4_host_match_key->set_exact(smi_id::F_IPV4_FIB_DST_ADDR,
                                         reinterpret_cast<const char *>(&ip4),
                                         sizeof(prefix.addr.ip4));
        break;
      case IPV4_LOCAL_HOST:
        v4_local_host_match_key->reset();
        status |= v4_local_host_match_key->set_exact(
            smi_id::F_IPV4_FIB_LOCAL_HOST_VRF, vrf);
        ip4 = htonl(prefix.addr.ip4);
        status |= v4_local_host_match_key->set_exact(
            smi_id::F_IPV4_FIB_LOCAL_HOST_DST_ADDR,
            reinterpret_cast<const char *>(&ip4),
            sizeof(prefix.addr.ip4));
        break;
      case IPV4_LPM:
        v4_lpm_match_key->reset();
        status |= v4_lpm_match_key->set_exact(smi_id::F_IPV4_FIB_LPM_VRF, vrf);
        status |= v4_lpm_match_key->set_lpm(
            smi_id::F_IPV4_FIB_LPM_DST_ADDR, prefix.addr.ip4, prefix.len);
        break;
      case IPV6_HOST:
        v6_host_match_key->reset();
        status |= v6_host_match_key->set_exact(smi_id::F_IPV6_FIB_VRF, vrf);
        status |= v6_host_match_key->set_exact(
            smi_id::F_IPV6_FIB_DST_ADDR,
            reinterpret_cast<const char *>(&prefix.addr.ip6),
            sizeof(prefix.addr.ip6));
        break;
      case IPV6_HOST64:
        v6_host64_match_key->reset();
        status |=
            v6_host64_match_key->set_exact(smi_id::F_IPV6_FIB_HOST64_VRF, vrf);
        status |= v6_host64_match_key->set_exact(
            smi_id::F_IPV6_FIB_HOST64_DST_ADDR,
            reinterpret_cast<const char *>(&prefix.addr.ip6),
            sizeof(prefix.addr.ip6) / 2);
        break;
      case IPV6_LPM:
        v6_lpm_match_key->reset();
        status |= v6_lpm_match_key->set_exact(smi_id::F_IPV6_FIB_LPM_VRF, vrf);
        status |=
            v6_lpm_match_key->set_lpm(smi_id::F_IPV6_FIB_LPM_DST_ADDR,
                                      reinterpret_cast<char *>(prefix.addr.ip6),
                                      sizeof(prefix.addr.ip6),
                                      prefix.len);
        break;
      case IPV6_LPM64:
        v6_lpm64_match_key->reset();
        status |=
            v6_lpm64_match_key->set_exact(smi_id::F_IPV6_FIB_LPM64_VRF, vrf);
        status |= v6_lpm64_match_key->set_lpm(
            smi_id::F_IPV6_FIB_LPM64_DST_ADDR,
            reinterpret_cast<char *>(prefix.addr.ip6),
            sizeof(prefix.addr.ip6) / 2,
            prefix.len);
        break;
      case IPV6_LPM_TCAM:
        v6_lpm_tcam_match_key->reset();
        status |= v6_lpm_tcam_match_key->set_exact(
            smi_id::F_IPV6_FIB_LPM_TCAM_VRF, vrf);
        status |= v6_lpm_tcam_match_key->set_lpm(
            smi_id::F_IPV6_FIB_LPM_TCAM_DST_ADDR,
            reinterpret_cast<char *>(prefix.addr.ip6),
            sizeof(prefix.addr.ip6),
            prefix.len);
        break;
      case IP_LPM64:
        shared_lpm64_match_key->reset();
        status |=
            shared_lpm64_match_key->set_exact(smi_id::F_IP_FIB_LPM64_VRF, vrf);
        if (prefix.addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4) {
          status |=
              shared_lpm64_match_key->set_lpm(smi_id::F_IP_FIB_LPM64_DST_ADDR,
                                              prefix.addr.ip4,
                                              prefix.len + 32);
        } else {
          status |= shared_lpm64_match_key->set_lpm(
              smi_id::F_IP_FIB_LPM64_DST_ADDR,
              reinterpret_cast<char *>(prefix.addr.ip6),
              sizeof(prefix.addr.ip6) / 2,
              prefix.len);
        }
        break;
    }
    SWITCH_DEBUG_LOG(switch_log(SWITCH_API_LEVEL_DEBUG,
                                SWITCH_OBJECT_TYPE_ROUTE,
                                "prefix: {}",
                                prefix));
    return status;
  }

  switch_status_t prepare_action_data(const switch_object_id_t nexthop_handle,
                                      uint32_t fib_label) {
    uint16_t nhop_id = compute_nexthop_index(nexthop_handle);
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    switch (fib_tbl_type) {
      case IPV4_HOST:
        if (myip_type.enumdata == SWITCH_DEVICE_ATTR_MYIP_TYPE_NONE) {
          status |= v4_host_action_hit->set_arg(smi_id::P_FIB_HIT_FIB_LABEL,
                                                fib_label);
          status |= v4_host_action_hit->set_arg(smi_id::P_FIB_HIT_NEXTHOP_INDEX,
                                                nhop_id);
          return status;
        } else {
          return v4_host_action_myip->set_arg(smi_id::P_FIB_MYIP_MYIP,
                                              myip_type.enumdata);
        }
      case IPV4_LOCAL_HOST:
        if (myip_type.enumdata == SWITCH_DEVICE_ATTR_MYIP_TYPE_NONE) {
          status |= v4_local_host_action_hit->set_arg(
              smi_id::P_FIB_HIT_FIB_LABEL, fib_label);
          status |= v4_local_host_action_hit->set_arg(
              smi_id::P_FIB_HIT_NEXTHOP_INDEX, nhop_id);
          return status;
        } else {
          return v4_local_host_action_myip->set_arg(smi_id::P_FIB_MYIP_MYIP,
                                                    myip_type.enumdata);
        }
      case IPV4_LPM:
        if (myip_type.enumdata == SWITCH_DEVICE_ATTR_MYIP_TYPE_NONE) {
          status |= v4_lpm_action_hit->set_arg(smi_id::P_FIB_HIT_FIB_LABEL,
                                               fib_label);
          status |= v4_lpm_action_hit->set_arg(smi_id::P_FIB_HIT_NEXTHOP_INDEX,
                                               nhop_id);
          return status;
        } else {
          return v4_lpm_action_myip->set_arg(smi_id::P_FIB_MYIP_MYIP,
                                             myip_type.enumdata);
        }
      case IPV6_HOST:
        if (myip_type.enumdata == SWITCH_DEVICE_ATTR_MYIP_TYPE_NONE) {
          status |= v6_host_action_hit->set_arg(smi_id::P_FIB_HIT_FIB_LABEL,
                                                fib_label);
          status |= v6_host_action_hit->set_arg(smi_id::P_FIB_HIT_NEXTHOP_INDEX,
                                                nhop_id);
          return status;
        } else {
          return v6_host_action_myip->set_arg(smi_id::P_FIB_MYIP_MYIP,
                                              myip_type.enumdata);
        }
      case IPV6_HOST64:
        if (myip_type.enumdata == SWITCH_DEVICE_ATTR_MYIP_TYPE_NONE) {
          status |= v6_host64_action_hit->set_arg(smi_id::P_FIB_HIT_FIB_LABEL,
                                                  fib_label);
          status |= v6_host64_action_hit->set_arg(
              smi_id::P_FIB_HIT_NEXTHOP_INDEX, nhop_id);
          return status;
        } else {
          return v6_host64_action_myip->set_arg(smi_id::P_FIB_MYIP_MYIP,
                                                myip_type.enumdata);
        }
      case IPV6_LPM:
        if (myip_type.enumdata == SWITCH_DEVICE_ATTR_MYIP_TYPE_NONE) {
          status |= v6_lpm_action_hit->set_arg(smi_id::P_FIB_HIT_NEXTHOP_INDEX,
                                               nhop_id);
          status |= v6_lpm_action_hit->set_arg(smi_id::P_FIB_HIT_FIB_LABEL,
                                               fib_label);
          return status;
        } else {
          return v6_lpm_action_myip->set_arg(smi_id::P_FIB_MYIP_MYIP,
                                             myip_type.enumdata);
        }
      case IPV6_LPM64:
        if (myip_type.enumdata == SWITCH_DEVICE_ATTR_MYIP_TYPE_NONE) {
          status |= v6_lpm64_action_hit->set_arg(
              smi_id::P_FIB_HIT_NEXTHOP_INDEX, nhop_id);
          status |= v6_lpm64_action_hit->set_arg(smi_id::P_FIB_HIT_FIB_LABEL,
                                                 fib_label);
          return status;
        } else {
          return v6_lpm64_action_myip->set_arg(smi_id::P_FIB_MYIP_MYIP,
                                               myip_type.enumdata);
        }
      case IPV6_LPM_TCAM:
        if (myip_type.enumdata == SWITCH_DEVICE_ATTR_MYIP_TYPE_NONE) {
          status |= v6_lpm_tcam_action_hit->set_arg(
              smi_id::P_FIB_HIT_NEXTHOP_INDEX, nhop_id);
          status |= v6_lpm_tcam_action_hit->set_arg(smi_id::P_FIB_HIT_FIB_LABEL,
                                                    fib_label);
          return status;
        } else {
          return v6_lpm_tcam_action_myip->set_arg(smi_id::P_FIB_MYIP_MYIP,
                                                  myip_type.enumdata);
        }
      case IP_LPM64:
        if (myip_type.enumdata == SWITCH_DEVICE_ATTR_MYIP_TYPE_NONE) {
          status |= shared_lpm64_action_hit->set_arg(
              smi_id::P_FIB_HIT_NEXTHOP_INDEX, nhop_id);
          status |= shared_lpm64_action_hit->set_arg(
              smi_id::P_FIB_HIT_FIB_LABEL, fib_label);
          return status;
        } else {
          return shared_lpm64_action_myip->set_arg(smi_id::P_FIB_MYIP_MYIP,
                                                   myip_type.enumdata);
        }
      default:
        break;
    }
    return SWITCH_STATUS_SUCCESS;
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch (fib_tbl_type) {
      case IPV4_HOST: {
        FIB_CREATE_UPDATE(v4_host, myip_type, packet_action);
      } break;
      case IPV4_LOCAL_HOST: {
        FIB_CREATE_UPDATE(v4_local_host, myip_type, packet_action);
      } break;
      case IPV4_LPM: {
        FIB_CREATE_UPDATE(v4_lpm, myip_type, packet_action);
      } break;
      case IPV6_HOST: {
        FIB_CREATE_UPDATE(v6_host, myip_type, packet_action);
      } break;
      case IPV6_HOST64: {
        FIB_CREATE_UPDATE(v6_host64, myip_type, packet_action);
      } break;
      case IPV6_LPM: {
        FIB_CREATE_UPDATE(v6_lpm, myip_type, packet_action);
      } break;
      case IPV6_LPM64: {
        FIB_CREATE_UPDATE(v6_lpm64, myip_type, packet_action);
      } break;
      case IPV6_LPM_TCAM: {
        FIB_CREATE_UPDATE(v6_lpm_tcam, myip_type, packet_action);
      } break;
      case IP_LPM64: {
        FIB_CREATE_UPDATE(shared_lpm64, myip_type, packet_action);
      } break;
      default:
        break;
    }
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_ROUTE,
                 "{}:{}: Failed to add HW entry vrf.{} prefix {}",
                 __func__,
                 __LINE__,
                 switch_store::handle_to_id(vrf_handle),
                 prefix);
      return status;
    }

    switch_lpm_trie_t *trie = NULL;
    switch_ip4_t v4addr;
    uint8_t *prefix_ptr = NULL;
    if (prefix.addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4) {
      trie = SWITCH_CONTEXT.ipv4_tries[vrf_handle.data];
      v4addr = htonl(prefix.addr.ip4);
      prefix_ptr = reinterpret_cast<uint8_t *>(&v4addr);
    } else if (prefix.addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6) {
      trie = SWITCH_CONTEXT.ipv6_tries[vrf_handle.data];
      prefix_ptr = reinterpret_cast<uint8_t *>(prefix.addr.ip6);
    } else {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_ROUTE,
                 "{}:{}: Invalid IP address family {}",
                 __func__,
                 __LINE__,
                 prefix.addr.addr_family);
      return SWITCH_STATUS_INVALID_PARAMETER;
    }

    if (!is_nbr_sourced) {
      status = switch_lpm_trie_insert(
          trie, prefix_ptr, prefix.len, auto_obj.get_parent().data);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_ROUTE,
                   "{}:{}: Failed to insert in SW trie vrf.{} prefix {}",
                   __func__,
                   __LINE__,
                   switch_store::handle_to_id(vrf_handle),
                   prefix);
        return SWITCH_STATUS_FAILURE;
      }
    }

    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch (fib_tbl_type) {
      case IPV4_HOST:
        status = minimal_match_action::del(*v4_host_table, *v4_host_match_key);
        break;
      case IPV4_LOCAL_HOST:
        status = minimal_match_action::del(*v4_local_host_table,
                                           *v4_local_host_match_key);
        break;
      case IPV4_LPM:
        status = minimal_match_action::del(*v4_lpm_table, *v4_lpm_match_key);
        break;
      case IPV6_HOST64:
        status =
            minimal_match_action::del(*v6_host64_table, *v6_host64_match_key);
        break;
      case IPV6_HOST:
        status = minimal_match_action::del(*v6_host_table, *v6_host_match_key);
        break;
      case IPV6_LPM:
        status = minimal_match_action::del(*v6_lpm_table, *v6_lpm_match_key);
        break;
      case IPV6_LPM64:
        status =
            minimal_match_action::del(*v6_lpm64_table, *v6_lpm64_match_key);
        break;
      case IPV6_LPM_TCAM:
        status = minimal_match_action::del(*v6_lpm_tcam_table,
                                           *v6_lpm_tcam_match_key);
        break;
      case IP_LPM64:
        status = minimal_match_action::del(*shared_lpm64_table,
                                           *shared_lpm64_match_key);
        break;
      default:
        break;
    }
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_ROUTE,
                 "{}:{}: Failed to delete HW entry vrf.{} prefix {}",
                 __func__,
                 __LINE__,
                 switch_store::handle_to_id(vrf_handle),
                 prefix);
      return status;
    }

    return status;
  }
};

switch_status_t l3_init() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  REGISTER_OBJECT(fib_factory, SWITCH_OBJECT_TYPE_FIB_FACTORY);

  FIB_TABLE_INIT(v4_host, smi_id::T_IPV4_FIB_HOST);
  FIB_TABLE_INIT(v4_local_host, smi_id::T_IPV4_FIB_LOCAL_HOST);
  FIB_TABLE_INIT(v4_lpm, smi_id::T_IPV4_FIB_LPM);
  FIB_TABLE_INIT(v6_host, smi_id::T_IPV6_FIB_HOST);
  FIB_TABLE_INIT(v6_host64, smi_id::T_IPV6_FIB_HOST64);
  FIB_TABLE_INIT(v6_lpm, smi_id::T_IPV6_FIB_LPM);
  FIB_TABLE_INIT(v6_lpm64, smi_id::T_IPV6_FIB_LPM64);
  FIB_TABLE_INIT(v6_lpm_tcam, smi_id::T_IPV6_FIB_LPM_TCAM);
  FIB_TABLE_INIT(shared_lpm64, smi_id::T_IP_FIB_LPM64);

  return status;
}

switch_status_t l3_clean() { return SWITCH_STATUS_SUCCESS; }

} /* namespace smi */
