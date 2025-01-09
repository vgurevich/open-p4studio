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


#include "test_api_perf.h"

#ifdef __CPU_PROFILER__
#include "gperftools/profiler.h"
#endif

namespace smi {
using namespace smi::bf_rt;
using ::smi::logging::switch_log;

PerfResult ITableBMAIMacEntry::run(uint32_t entries, bool batch) {
  PerfResult perf_test(_test_name);
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_enum_t entry_type = {.enumdata = SWITCH_MAC_ENTRY_ATTR_TYPE_STATIC};
  switch_object_id_t mac_entry_handle = {};
  uint32_t ports = _port_handle_list.size();
  uint32_t vlans = _vlan_handle_list.size();
  int32_t entries_added(0);

  mac_addr_list_gen(_mac_addr_list, std::min(entries, max_entries));

  perf_test.start_clock();
#ifdef __CPU_PROFILER__
  ProfilerStart(get_prof_file_name(_test_name, entries, batch).c_str());
  ProfilerFlush();
#endif
  if (batch) {
    smi::bf_rt::start_batch();
  }

  for (auto &addr : _mac_addr_list) {
    std::set<attr_w> attrs;
    attrs.insert(attr_w(SWITCH_MAC_ENTRY_ATTR_DEVICE, _device));
    attrs.insert(attr_w(SWITCH_MAC_ENTRY_ATTR_DESTINATION_HANDLE,
                        _port_handle_list.at(entries_added % ports)));
    attrs.insert(attr_w(SWITCH_MAC_ENTRY_ATTR_VLAN_HANDLE,
                        _vlan_handle_list.at(entries_added % vlans)));
    attrs.insert(attr_w(SWITCH_MAC_ENTRY_ATTR_MAC_ADDRESS, addr));
    attrs.insert(attr_w(SWITCH_MAC_ENTRY_ATTR_TYPE, entry_type));

    status = bf_switch_object_create(
        SWITCH_OBJECT_TYPE_MAC_ENTRY, attrs, mac_entry_handle);
    if (status == SWITCH_STATUS_SUCCESS) {
      _mac_entries_handles.push_back(mac_entry_handle);
    } else {
      std::cerr << _test_name << ": error adding " << entries_added + 1
                << " entry" << std::endl;
      break;
    }
    entries_added++;
  }

  if (batch) {
    smi::bf_rt::end_batch();
  }
#ifdef __CPU_PROFILER__
  ProfilerStop();
#endif
  perf_test.end_clock();
  perf_test.compute_rate(entries_added);
  std::cout << _test_name << ": entries_added: " << entries_added << std::endl;
  perf_test.set_status(status);
  return perf_test;
}

PerfResult ITableBFRTMacEntry::run(uint32_t entries, bool batch) {
  PerfResult perf_test(_test_name);
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t bd_handle = {};
  uint32_t ports = _port_handle_list.size();
  uint32_t vlans = _vlan_handle_list.size();
  int32_t entries_added(0);

  mac_addr_list_gen(_mac_addr_list, std::min(entries, max_entries));

  perf_test.start_clock();
#ifdef __CPU_PROFILER__
  ProfilerStart(get_prof_file_name(_test_name, entries, batch).c_str());
  ProfilerFlush();
#endif
  if (batch) {
    smi::bf_rt::start_batch();
  }

  bool ret(false);
  for (auto &addr : _mac_addr_list) {
    _Table table_dmac(get_dev_tgt(), get_bf_rt_info(), smi_id::T_DMAC);
    _Table table_smac(get_dev_tgt(), get_bf_rt_info(), smi_id::T_SMAC);
    _MatchKey match_key_dmac(smi_id::T_DMAC);
    _MatchKey match_key_smac(smi_id::T_SMAC);
    _ActionEntry action_entry_dmac(smi_id::T_DMAC);
    _ActionEntry action_entry_smac(smi_id::T_SMAC);

    find_auto_oid(_vlan_handle_list.at(entries_added % vlans),
                  SWITCH_OBJECT_TYPE_BD,
                  bd_handle);

    match_key_dmac.set_exact(smi_id::F_DMAC_LOCAL_MD_BD, bd_handle);
    match_key_smac.set_exact(smi_id::F_SMAC_LOCAL_MD_BD, bd_handle);
    match_key_dmac.set_exact(smi_id::F_DMAC_DST_ADDR, addr);
    match_key_smac.set_exact(smi_id::F_SMAC_SRC_ADDR, addr);
    action_entry_dmac.init_action_data(smi_id::A_DMAC_HIT);
    action_entry_smac.init_action_data(smi_id::A_SMAC_HIT);
    action_entry_dmac.set_arg(smi_id::P_DMAC_HIT_PORT_LAG_INDEX,
                              _port_handle_list.at(entries_added % ports));
    action_entry_smac.set_arg(smi_id::P_SMAC_HIT_PORT_LAG_INDEX,
                              _port_handle_list.at(entries_added % ports));
    ret = false;
    status = table_dmac.entry_add(match_key_dmac, action_entry_dmac, ret);
    if (status != SWITCH_STATUS_SUCCESS) {
      std::cerr << _test_name << ": error adding " << entries_added + 1
                << " entry" << std::endl;
      break;
    }
    ret = false;
    status = table_smac.entry_add(match_key_smac, action_entry_smac, ret);
    if (status != SWITCH_STATUS_SUCCESS) {
      std::cerr << _test_name << ": error adding " << entries_added + 1
                << " entry" << std::endl;
      break;
    }
    entries_added++;
  }

  if (batch) {
    smi::bf_rt::end_batch();
  }
#ifdef __CPU_PROFILER__
  ProfilerStop();
#endif
  perf_test.end_clock();
  perf_test.compute_rate(entries_added);
  std::cout << _test_name << ": entries_added: " << entries_added << std::endl;

  perf_test.set_status(status);
  return perf_test;
}

PerfResult ITableBMAIAclIpv4::run(uint32_t entries, bool batch) {
  PerfResult perf_test(_test_name);
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_enum_t table_type = {.enumdata = SWITCH_ACL_TABLE_ATTR_TYPE_IPV4};
  switch_enum_t table_bind_point = {
      .enumdata = SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE_PORT};
  switch_enum_t table_direction = {.enumdata =
                                       SWITCH_ACL_TABLE_ATTR_DIRECTION_INGRESS};
  switch_enum_t packet_action = {.enumdata =
                                     SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION_DROP};
  switch_object_id_t entry_handle = {};
  switch_ip_address_t mask = {};
  int32_t entries_added(0);

  mask.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
  mask.ip4 = 0xFFFFFFFF;

  ip4_addr_list_gen(_ip4_addr_list, std::min(entries, max_entries));

  perf_test.start_clock();
#ifdef __CPU_PROFILER__
  ProfilerStart(get_prof_file_name(_test_name, entries, batch).c_str());
  ProfilerFlush();
#endif
  if (batch) {
    smi::bf_rt::start_batch();
  }

  std::set<attr_w> t_attrs;
  t_attrs.insert(attr_w(SWITCH_ACL_TABLE_ATTR_DEVICE, _device));
  t_attrs.insert(attr_w(SWITCH_ACL_TABLE_ATTR_TYPE, table_type));
  std::vector<switch_enum_t> bp_list;
  bp_list.push_back(table_bind_point);
  t_attrs.insert(attr_w(SWITCH_ACL_TABLE_ATTR_BIND_POINT_TYPE, bp_list));
  t_attrs.insert(attr_w(SWITCH_ACL_TABLE_ATTR_DIRECTION, table_direction));

  status = bf_switch_object_create(
      SWITCH_OBJECT_TYPE_ACL_TABLE, t_attrs, _acl_table_handle);
  if (status == SWITCH_STATUS_SUCCESS) {
    for (auto &addr : _ip4_addr_list) {
      std::set<attr_w> e_attrs;
      e_attrs.insert(attr_w(SWITCH_ACL_ENTRY_ATTR_DEVICE, _device));
      e_attrs.insert(
          attr_w(SWITCH_ACL_ENTRY_ATTR_TABLE_HANDLE, _acl_table_handle));
      e_attrs.insert(attr_w(SWITCH_ACL_ENTRY_ATTR_DST_IP, addr));
      e_attrs.insert(attr_w(SWITCH_ACL_ENTRY_ATTR_DST_IP_MASK, mask));
      e_attrs.insert(
          attr_w(SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION, packet_action));
      status = bf_switch_object_create(
          SWITCH_OBJECT_TYPE_ACL_ENTRY, e_attrs, entry_handle);
      if (status == SWITCH_STATUS_SUCCESS) {
        _acl_entry_handles.push_back(entry_handle);
      } else {
        std::cout << _test_name << ": error adding " << entries_added + 1
                  << " entry" << std::endl;
        break;
      }
      entries_added++;
    }
  } else {
    std::cout << _test_name << ": error adding table" << std::endl;
  }

  if (batch) {
    smi::bf_rt::end_batch();
  }
#ifdef __CPU_PROFILER__
  ProfilerStop();
#endif
  perf_test.end_clock();
  perf_test.compute_rate(entries_added);
  std::cout << _test_name << ": entries_added: " << entries_added << std::endl;

  perf_test.set_status(status);
  return perf_test;
}

PerfResult ITableBFRTAclIpv4::run(uint32_t entries, bool batch) {
  PerfResult perf_test(_test_name);
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_ip_address_t mask = {};
  int32_t entries_added(0);

  mask.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
  mask.ip4 = 0xFFFFFFFF;

  ip4_addr_list_gen(_ip4_addr_list, std::min(entries, max_entries));

  perf_test.start_clock();
#ifdef __CPU_PROFILER__
  ProfilerStart(get_prof_file_name(_test_name, entries, batch).c_str());
  ProfilerFlush();
#endif
  if (batch) {
    smi::bf_rt::start_batch();
  }

  for (auto &addr : _ip4_addr_list) {
    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_INGRESS_IPV4_ACL);
    _MatchKey match_key(smi_id::T_INGRESS_IPV4_ACL);
    _ActionEntry action_entry(smi_id::T_INGRESS_IPV4_ACL);

    match_key.set_ternary(
        smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_SRC_ADDR, addr, mask);
    action_entry.init_action_data(smi_id::A_INGRESS_IPV4_ACL_DENY);

    bool ret = false;
    status = table.entry_add(match_key, action_entry, ret);
    if (status != SWITCH_STATUS_SUCCESS) {
      std::cerr << _test_name << ": error adding " << entries_added + 1
                << " entry" << std::endl;
      break;
    }
    entries_added++;
  }

  if (batch) {
    smi::bf_rt::end_batch();
  }
#ifdef __CPU_PROFILER__
  ProfilerStop();
#endif
  perf_test.end_clock();
  perf_test.compute_rate(entries_added);
  std::cout << _test_name << ": entries_added: " << entries_added << std::endl;

  perf_test.set_status(status);
  return perf_test;
}

PerfResult ITableBMAIHostRouteIpv4::run(uint32_t entries, bool batch) {
  PerfResult perf_test(_test_name);
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t route_handle = {};
  uint32_t vrfs = _vrf_handle_list.size();
  uint32_t nhops = _nhop_handle_list.size();
  int32_t entries_added(0);

  ip4_prefix_list_gen(_prefix_list, std::min(entries, max_entries));

  perf_test.start_clock();
#ifdef __CPU_PROFILER__
  ProfilerStart(get_prof_file_name(_test_name, entries, batch).c_str());
  ProfilerFlush();
#endif
  if (batch) {
    smi::bf_rt::start_batch();
  }

  for (auto &prefix : _prefix_list) {
    std::set<attr_w> attrs;
    attrs.insert(attr_w(SWITCH_ROUTE_ATTR_DEVICE, _device));
    attrs.insert(attr_w(SWITCH_ROUTE_ATTR_VRF_HANDLE,
                        _vrf_handle_list.at(entries_added % vrfs)));
    attrs.insert(attr_w(SWITCH_ROUTE_ATTR_NEXTHOP_HANDLE,
                        _nhop_handle_list.at(entries_added % nhops)));
    attrs.insert(attr_w(SWITCH_ROUTE_ATTR_IP_PREFIX, prefix));

    status =
        bf_switch_object_create(SWITCH_OBJECT_TYPE_ROUTE, attrs, route_handle);
    if (status == SWITCH_STATUS_SUCCESS) {
      _route_handles.push_back(route_handle);
    } else {
      std::cout << _test_name << ": error adding " << entries_added + 1
                << " entry" << std::endl;
      break;
    }
    entries_added++;
  }

  if (batch) {
    smi::bf_rt::end_batch();
  }
#ifdef __CPU_PROFILER__
  ProfilerStop();
#endif
  perf_test.end_clock();
  perf_test.compute_rate(entries_added);
  std::cout << _test_name << ": entries_added: " << entries_added << std::endl;

  perf_test.set_status(status);
  return perf_test;
}

PerfResult ITableBFRTHostRouteIpv4::run(uint32_t entries, bool batch) {
  PerfResult perf_test(_test_name);
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  uint32_t vrfs = _vrf_handle_list.size();
  uint32_t nhops = _nhop_handle_list.size();
  int32_t entries_added(0);

  ip4_prefix_list_gen(_prefix_list, std::min(entries, max_entries));

  perf_test.start_clock();
#ifdef __CPU_PROFILER__
  ProfilerStart(get_prof_file_name(_test_name, entries, batch).c_str());
  ProfilerFlush();
#endif
  if (batch) {
    smi::bf_rt::start_batch();
  }

  for (auto &prefix : _prefix_list) {
    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_IPV4_FIB_HOST);
    _MatchKey match_key(smi_id::T_IPV4_FIB_HOST);
    _ActionEntry action_entry(smi_id::T_IPV4_FIB_HOST);

    match_key.set_exact(smi_id::F_IPV4_FIB_VRF,
                        _vrf_handle_list.at(entries_added % vrfs));
    match_key.set_exact(smi_id::F_IPV4_FIB_DST_ADDR, htonl(prefix.addr.ip4));
    action_entry.init_action_data(smi_id::A_FIB_HIT);
    action_entry.set_arg(smi_id::P_FIB_HIT_NEXTHOP_INDEX,
                         _nhop_handle_list.at(entries_added % nhops));
    bool ret(false);
    status = table.entry_add(match_key, action_entry, ret);
    if (status != SWITCH_STATUS_SUCCESS) {
      std::cerr << _test_name << ": error adding " << entries_added + 1
                << " entry" << std::endl;
      break;
    }
    entries_added++;
  }

  if (batch) {
    smi::bf_rt::end_batch();
  }
#ifdef __CPU_PROFILER__
  ProfilerStop();
#endif
  perf_test.end_clock();
  perf_test.compute_rate(entries_added);
  std::cout << _test_name << ": entries_added: " << entries_added << std::endl;

  perf_test.set_status(status);
  return perf_test;
}

PerfResult ITableBMAIHostRouteIpv6::run(uint32_t entries, bool batch) {
  PerfResult perf_test(_test_name);
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t route_handle = {};
  uint32_t vrfs = _vrf_handle_list.size();
  uint32_t nhops = _nhop_handle_list.size();
  int32_t entries_added(0);

  ip6_prefix_list_gen(_prefix_list, std::min(entries, max_entries));

  perf_test.start_clock();
#ifdef __CPU_PROFILER__
  ProfilerStart(get_prof_file_name(_test_name, entries, batch).c_str());
  ProfilerFlush();
#endif
  if (batch) {
    smi::bf_rt::start_batch();
  }

  for (auto &prefix : _prefix_list) {
    std::set<attr_w> attrs;
    attrs.insert(attr_w(SWITCH_ROUTE_ATTR_DEVICE, _device));
    attrs.insert(attr_w(SWITCH_ROUTE_ATTR_VRF_HANDLE,
                        _vrf_handle_list.at(entries_added % vrfs)));
    attrs.insert(attr_w(SWITCH_ROUTE_ATTR_NEXTHOP_HANDLE,
                        _nhop_handle_list.at(entries_added % nhops)));
    attrs.insert(attr_w(SWITCH_ROUTE_ATTR_IP_PREFIX, prefix));

    status =
        bf_switch_object_create(SWITCH_OBJECT_TYPE_ROUTE, attrs, route_handle);
    if (status == SWITCH_STATUS_SUCCESS) {
      _route_handles.push_back(route_handle);
    } else {
      std::cout << _test_name << ": error adding " << entries_added + 1
                << " entry" << std::endl;
      break;
    }
    entries_added++;
  }

  if (batch) {
    smi::bf_rt::end_batch();
  }
#ifdef __CPU_PROFILER__
  ProfilerStop();
#endif
  perf_test.end_clock();
  perf_test.compute_rate(entries_added);
  std::cout << _test_name << ": entries_added: " << entries_added << std::endl;

  perf_test.set_status(status);
  return perf_test;
}

PerfResult ITableBFRTHostRouteIpv6::run(uint32_t entries, bool batch) {
  PerfResult perf_test(_test_name);
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  uint32_t vrfs = _vrf_handle_list.size();
  uint32_t nhops = _nhop_handle_list.size();
  int32_t entries_added(0);

  ip6_prefix_list_gen(_prefix_list, std::min(entries, max_entries));

  perf_test.start_clock();
#ifdef __CPU_PROFILER__
  ProfilerStart(get_prof_file_name(_test_name, entries, batch).c_str());
  ProfilerFlush();
#endif
  if (batch) {
    smi::bf_rt::start_batch();
  }

  for (auto &prefix : _prefix_list) {
    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_IPV6_FIB_HOST);
    _MatchKey match_key(smi_id::T_IPV6_FIB_HOST);
    _ActionEntry action_entry(smi_id::T_IPV6_FIB_HOST);

    match_key.set_exact(smi_id::F_IPV6_FIB_VRF,
                        _vrf_handle_list.at(entries_added % vrfs));
    match_key.set_exact(smi_id::F_IPV6_FIB_DST_ADDR,
                        reinterpret_cast<char *>(prefix.addr.ip6),
                        sizeof(prefix.addr.ip6));
    action_entry.init_action_data(smi_id::A_FIB_HIT);
    action_entry.set_arg(smi_id::P_FIB_HIT_NEXTHOP_INDEX,
                         _nhop_handle_list.at(entries_added % nhops));
    bool ret(false);
    status = table.entry_add(match_key, action_entry, ret);
    if (status != SWITCH_STATUS_SUCCESS) {
      std::cerr << _test_name << ": error adding " << entries_added + 1
                << " entry" << std::endl;
      break;
    }
    entries_added++;
  }

  if (batch) {
    smi::bf_rt::end_batch();
  }
#ifdef __CPU_PROFILER__
  ProfilerStop();
#endif
  perf_test.end_clock();
  perf_test.compute_rate(entries_added);
  std::cout << _test_name << ": entries_added: " << entries_added << std::endl;

  perf_test.set_status(status);
  return perf_test;
}

PerfResult ITableBMAILPMRouteIpv4::run(uint32_t entries, bool batch) {
  PerfResult perf_test(_test_name);
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t route_handle = {};
  uint32_t vrfs = _vrf_handle_list.size();
  uint32_t nhops = _nhop_handle_list.size();
  int32_t entries_added(0);

  status = ip4_prefix_list_read(_prefix_list, std::min(entries, max_entries));
  if (status != SWITCH_STATUS_SUCCESS) {
    std::cerr << "ipv4 route table file not found!" << std::endl;
    perf_test.set_status(status);
    return perf_test;
  };

  perf_test.start_clock();
#ifdef __CPU_PROFILER__
  ProfilerStart(get_prof_file_name(_test_name, entries, batch).c_str());
  ProfilerFlush();
#endif
  if (batch) {
    smi::bf_rt::start_batch();
  }

  for (auto &prefix : _prefix_list) {
    std::set<attr_w> attrs;
    attrs.insert(attr_w(SWITCH_ROUTE_ATTR_DEVICE, _device));
    attrs.insert(attr_w(SWITCH_ROUTE_ATTR_VRF_HANDLE,
                        _vrf_handle_list.at(entries_added % vrfs)));
    attrs.insert(attr_w(SWITCH_ROUTE_ATTR_NEXTHOP_HANDLE,
                        _nhop_handle_list.at(entries_added % nhops)));
    attrs.insert(attr_w(SWITCH_ROUTE_ATTR_IP_PREFIX, prefix));

    status =
        bf_switch_object_create(SWITCH_OBJECT_TYPE_ROUTE, attrs, route_handle);
    if (status == SWITCH_STATUS_SUCCESS) {
      _route_handles.push_back(route_handle);
    } else {
      std::cerr << _test_name << ": error adding " << entries_added + 1
                << " entry" << std::endl;
      break;
    }
    entries_added++;
  }

  if (batch) {
    smi::bf_rt::end_batch();
  }
#ifdef __CPU_PROFILER__
  ProfilerStop();
#endif
  perf_test.end_clock();
  perf_test.compute_rate(entries_added);
  std::cout << _test_name << ": entries_added: " << entries_added << std::endl;

  perf_test.set_status(status);
  return perf_test;
}

PerfResult ITableBFRTLPMRouteIpv4::run(uint32_t entries, bool batch) {
  PerfResult perf_test(_test_name);
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  uint32_t vrfs = _vrf_handle_list.size();
  uint32_t nhops = _nhop_handle_list.size();
  int32_t entries_added(0);

  status = ip4_prefix_list_read(_prefix_list, std::min(entries, max_entries));
  if (status != SWITCH_STATUS_SUCCESS) {
    std::cerr << "ipv4 route table file not found!" << std::endl;
    perf_test.set_status(status);
    return perf_test;
  };

  perf_test.start_clock();
#ifdef __CPU_PROFILER__
  ProfilerStart(get_prof_file_name(_test_name, entries, batch).c_str());
  ProfilerFlush();
#endif
  if (batch) {
    smi::bf_rt::start_batch();
  }

  for (auto &prefix : _prefix_list) {
    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_IPV4_FIB_LPM);
    _MatchKey match_key(smi_id::T_IPV4_FIB_LPM);
    _ActionEntry action_entry(smi_id::T_IPV4_FIB_LPM);

    match_key.set_exact(smi_id::F_IPV4_FIB_LPM_VRF,
                        _vrf_handle_list.at(entries_added % vrfs));
    match_key.set_lpm(
        smi_id::F_IPV4_FIB_LPM_DST_ADDR, prefix.addr.ip4, prefix.len);
    action_entry.init_action_data(smi_id::A_FIB_HIT);
    action_entry.set_arg(smi_id::P_FIB_HIT_NEXTHOP_INDEX,
                         _nhop_handle_list.at(entries_added % nhops));
    bool ret(false);
    status = table.entry_add(match_key, action_entry, ret);
    if (status != SWITCH_STATUS_SUCCESS) {
      std::cerr << _test_name << ": error adding " << entries_added + 1
                << " entry" << std::endl;
      break;
    }
    entries_added++;
  }

  if (batch) {
    smi::bf_rt::end_batch();
  }
#ifdef __CPU_PROFILER__
  ProfilerStop();
#endif
  perf_test.end_clock();
  perf_test.compute_rate(entries_added);
  std::cout << _test_name << ": entries_added: " << entries_added << std::endl;

  perf_test.set_status(status);
  return perf_test;
}

PerfResult ITableBMAILPMRouteIpv6::run(uint32_t entries, bool batch) {
  PerfResult perf_test(_test_name);
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t route_handle = {};
  uint32_t vrfs = _vrf_handle_list.size();
  uint32_t nhops = _nhop_handle_list.size();
  int32_t entries_added(0);

  status = ip6_prefix_list_read(_prefix_list, std::min(entries, max_entries));
  if (status != SWITCH_STATUS_SUCCESS) {
    std::cerr << "ipv6 route table file not found!" << std::endl;
    perf_test.set_status(status);
    return perf_test;
  };

  perf_test.start_clock();
#ifdef __CPU_PROFILER__
  ProfilerStart(get_prof_file_name(_test_name, entries, batch).c_str());
  ProfilerFlush();
#endif
  if (batch) {
    smi::bf_rt::start_batch();
  }

  for (auto &prefix : _prefix_list) {
    std::set<attr_w> attrs;
    attrs.insert(attr_w(SWITCH_ROUTE_ATTR_DEVICE, _device));
    attrs.insert(attr_w(SWITCH_ROUTE_ATTR_VRF_HANDLE,
                        _vrf_handle_list.at(entries_added % vrfs)));
    attrs.insert(attr_w(SWITCH_ROUTE_ATTR_NEXTHOP_HANDLE,
                        _nhop_handle_list.at(entries_added % nhops)));
    attrs.insert(attr_w(SWITCH_ROUTE_ATTR_IP_PREFIX, prefix));

    status =
        bf_switch_object_create(SWITCH_OBJECT_TYPE_ROUTE, attrs, route_handle);
    if (status == SWITCH_STATUS_SUCCESS) {
      _route_handles.push_back(route_handle);
    } else {
      std::cerr << _test_name << ": error adding " << entries_added + 1
                << " entry" << std::endl;
      break;
    }
    entries_added++;
  }

  if (batch) {
    smi::bf_rt::end_batch();
  }
#ifdef __CPU_PROFILER__
  ProfilerStop();
#endif
  perf_test.end_clock();
  perf_test.compute_rate(entries_added);
  std::cout << _test_name << ": entries_added: " << entries_added << std::endl;

  perf_test.set_status(status);
  return perf_test;
}

PerfResult ITableBFRTLPMRouteIpv6::run(uint32_t entries, bool batch) {
  PerfResult perf_test(_test_name);
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  uint32_t vrfs = _vrf_handle_list.size();
  uint32_t nhops = _nhop_handle_list.size();
  int32_t entries_added(0);

  status = ip6_prefix_list_read(_prefix_list, std::min(entries, max_entries));
  if (status != SWITCH_STATUS_SUCCESS) {
    std::cerr << "ipv6 route table file not found!" << std::endl;
    perf_test.set_status(status);
    return perf_test;
  };

  perf_test.start_clock();
#ifdef __CPU_PROFILER__
  ProfilerStart(get_prof_file_name(_test_name, entries, batch).c_str());
  ProfilerFlush();
#endif
  if (batch) {
    smi::bf_rt::start_batch();
  }

  for (auto &prefix : _prefix_list) {
    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_IPV6_FIB_LPM64);
    _MatchKey match_key(smi_id::T_IPV6_FIB_LPM64);
    _ActionEntry action_entry(smi_id::T_IPV6_FIB_LPM64);

    match_key.set_exact(smi_id::F_IPV6_FIB_LPM64_VRF,
                        _vrf_handle_list.at(entries_added % vrfs));
    match_key.set_lpm(smi_id::F_IPV6_FIB_LPM64_DST_ADDR,
                      reinterpret_cast<char *>(prefix.addr.ip6),
                      sizeof(prefix.addr.ip6) / 2,
                      prefix.len);
    action_entry.init_action_data(smi_id::A_FIB_HIT);
    action_entry.set_arg(smi_id::P_FIB_HIT_NEXTHOP_INDEX,
                         _nhop_handle_list.at(entries_added % nhops));
    bool ret(false);
    status = table.entry_add(match_key, action_entry, ret);
    if (status != SWITCH_STATUS_SUCCESS) {
      std::cerr << _test_name << ": error adding " << entries_added + 1
                << " entry" << std::endl;
      break;
    }
    entries_added++;
  }

  if (batch) {
    smi::bf_rt::end_batch();
  }
#ifdef __CPU_PROFILER__
  ProfilerStop();
#endif
  perf_test.end_clock();
  perf_test.compute_rate(entries_added);
  std::cout << _test_name << ": entries_added: " << entries_added << std::endl;

  perf_test.set_status(status);
  return perf_test;
}

PerfResult ITableBMAINexthop::run(uint32_t entries, bool batch) {
  PerfResult perf_test(_test_name);
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_enum_t nhop_type = {.enumdata = SWITCH_NEXTHOP_ATTR_TYPE_IP};
  switch_object_id_t nhop_handle = {};
  uint32_t rifs = _rif_handle_list.size();
  int32_t entries_added(0);

  ip4_addr_list_gen(_ip4_addr_list, std::min(entries, max_entries));

  perf_test.start_clock();
#ifdef __CPU_PROFILER__
  ProfilerStart(get_prof_file_name(_test_name, entries, batch).c_str());
  ProfilerFlush();
#endif
  if (batch) {
    smi::bf_rt::start_batch();
  }

  for (auto &addr : _ip4_addr_list) {
    std::set<attr_w> attrs;
    attrs.insert(attr_w(SWITCH_NEXTHOP_ATTR_DEVICE, _device));
    attrs.insert(attr_w(SWITCH_NEXTHOP_ATTR_HANDLE,
                        _rif_handle_list.at(entries_added % rifs)));
    attrs.insert(attr_w(SWITCH_NEXTHOP_ATTR_DEST_IP, addr));
    attrs.insert(attr_w(SWITCH_NEXTHOP_ATTR_TYPE, nhop_type));
    status =
        bf_switch_object_create(SWITCH_OBJECT_TYPE_NEXTHOP, attrs, nhop_handle);
    if (status == SWITCH_STATUS_SUCCESS) {
      _nhop_handles.push_back(nhop_handle);
    } else {
      std::cerr << _test_name << ": error adding " << entries_added + 1
                << " entry" << std::endl;
      break;
    }
    entries_added++;
  }

  if (batch) {
    smi::bf_rt::end_batch();
  }
#ifdef __CPU_PROFILER__
  ProfilerStop();
#endif
  perf_test.end_clock();
  perf_test.compute_rate(entries_added);
  std::cout << _test_name << ": entries_added: " << entries_added << std::endl;

  perf_test.set_status(status);
  return perf_test;
}

PerfResult ITableBMAIVlan::run(uint32_t entries, bool batch) {
  PerfResult perf_test(_test_name);
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  int32_t entries_added(0);

  perf_test.start_clock();
#ifdef __CPU_PROFILER__
  ProfilerStart(get_prof_file_name(_test_name, entries, batch).c_str());
  ProfilerFlush();
#endif
  if (batch) {
    smi::bf_rt::start_batch();
  }

  for (uint16_t i = 2; i < entries; i++) {
    switch_object_id_t vlan_handle = {};
    std::set<attr_w> attrs;
    attrs.insert(attr_w(SWITCH_VLAN_ATTR_DEVICE, _device));
    attrs.insert(attr_w(SWITCH_VLAN_ATTR_VLAN_ID, i));
    status =
        bf_switch_object_create(SWITCH_OBJECT_TYPE_VLAN, attrs, vlan_handle);
    if (status == SWITCH_STATUS_SUCCESS) {
      _vlan_handles.push_back(vlan_handle);
    } else {
      std::cerr << _test_name << ": error adding " << entries_added + 1
                << " entry" << std::endl;
      break;
    }
    entries_added++;
  }

  if (batch) {
    smi::bf_rt::end_batch();
  }
#ifdef __CPU_PROFILER__
  ProfilerStop();
#endif
  perf_test.end_clock();
  perf_test.compute_rate(entries_added);
  std::cout << _test_name << ": entries_added: " << entries_added << std::endl;

  perf_test.set_status(status);
  return perf_test;
}

/* TO DO - test to be fixed within multicast enabling works
PerfResult ITableBMAIMcastMember::run(uint32_t entries, bool batch) {
  PerfResult perf_test(_test_name);
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  int32_t entries_added(0);

  uint16_t count = static_cast<uint16_t>(entries / 100);
  for (uint16_t i = 2; i <= 101; i++) {
    switch_object_id_t vlan_handle = {};
    std::set<attr_w> attrs;
    attrs.insert(attr_w(SWITCH_VLAN_ATTR_DEVICE, _device));
    attrs.insert(attr_w(SWITCH_VLAN_ATTR_VLAN_ID, i));
    status =
        bf_switch_object_create(SWITCH_OBJECT_TYPE_VLAN, attrs, vlan_handle);
    if (status == SWITCH_STATUS_SUCCESS) {
      _vlan_handles.push_back(vlan_handle);
    }
  }

  for (uint16_t i = 0; i < count; i++) {
    switch_object_id_t mcast_group_handle = {};
    std::set<attr_w> attrs;
    attrs.insert(attr_w(SWITCH_MULTICAST_GROUP_ATTR_DEVICE, _device));
    status = bf_switch_object_create(
        SWITCH_OBJECT_TYPE_MULTICAST_GROUP, attrs, mcast_group_handle);
    if (status == SWITCH_STATUS_SUCCESS) {
      _mcast_group_handles.push_back(mcast_group_handle);
    }
  }

  switch_object_id_t cpu_port_handle = {};
  attr_w cpu_port_attr(SWITCH_DEVICE_ATTR_CPU_PORT);
  bf_switch_attribute_get(_device, SWITCH_DEVICE_ATTR_CPU_PORT, cpu_port_attr);

  perf_test.start_clock();
#ifdef __CPU_PROFILER__
  ProfilerStart(get_prof_file_name(_test_name, entries, batch).c_str());
  ProfilerFlush();
#endif
  if (batch) {
    smi::bf_rt::start_batch();
  }

  cpu_port_attr.v_get(cpu_port_handle);
  for (uint16_t k = 0; k < 100; k++) {
    for (uint16_t i = 0; i < count; i++) {
      switch_object_id_t member_handle = {};
      std::set<attr_w> attrs;
      attrs.insert(attr_w(SWITCH_MULTICAST_MEMBER_ATTR_DEVICE, _device));
      attrs.insert(attr_w(SWITCH_MULTICAST_MEMBER_ATTR_MULTICAST_GROUP_HANDLE,
                          _mcast_group_handles[i]));
      attrs.insert(
          attr_w(SWITCH_MULTICAST_MEMBER_ATTR_VLAN_HANDLE, _vlan_handles[k]));
      attrs.insert(
          attr_w(SWITCH_MULTICAST_MEMBER_ATTR_HANDLE, cpu_port_handle));
      status = bf_switch_object_create(
          SWITCH_OBJECT_TYPE_MULTICAST_MEMBER, attrs, member_handle);
      if (status == SWITCH_STATUS_SUCCESS) {
        _mcast_member_handles.push_back(member_handle);
      } else {
        std::cout << _test_name << ": error adding " << entries_added + 1
                  << " entry" << std::endl;
        break;
      }
      entries_added++;
    }
  }

  if (batch) {
    smi::bf_rt::end_batch();
  }
#ifdef __CPU_PROFILER__
  ProfilerStop();
#endif
  perf_test.end_clock();
  perf_test.compute_rate(entries_added);
  std::cout << _test_name << ": entries_added: " << entries_added << std::endl;
  perf_test.set_status(status);
  return perf_test;
}
*/

}  // namespace smi
