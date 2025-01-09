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


#ifndef __TEST_API_PERF_H__
#define __TEST_API_PERF_H__

#include <iostream>
#include <stdio.h>
#include <dirent.h>
#include "arpa/inet.h"

#include "bf_switcht_api_rpc.h"

#include "bf_switch/bf_switch.h"
#include "../../s3/log.h"
#include "../switch_tna/bf_rt_ids.h"
#include "model.h"

namespace smi {

using namespace bf_switcht_api;
using namespace bf_switch;
using namespace smi::bf_rt;

/**
 * This is class which is used to measure and calulate rate of the tests
 * and store its status.
 */
class PerfResult {
 private:
  struct timespec _start = {0}, _end = {0};
  std::string _test = "";
  double _ms = 0;
  uint32_t _rate = 0;
  switch_status_t _status = SWITCH_STATUS_FAILURE;

 public:
  PerfResult() {}
  PerfResult(std::string test) : _test(test) {}
  PerfResult(double ms, uint32_t rate) : _ms(ms), _rate(rate) {}
  void start_clock() { clock_gettime(CLOCK_MONOTONIC, &_start); }
  void end_clock() { clock_gettime(CLOCK_MONOTONIC, &_end); }
  void compute_rate(uint64_t it) {
    struct timespec diff = {0};
    double microseconds = 0;
    double ops_per_microsecond = 0;
    diff.tv_sec = _end.tv_sec - _start.tv_sec;
    diff.tv_nsec = _end.tv_nsec - _start.tv_nsec;

    if (diff.tv_nsec < 0) {
      diff.tv_sec -= 1;
      diff.tv_nsec += 1000000000;
    }
    microseconds = diff.tv_sec * 1000000 + diff.tv_nsec / 1000;
    ops_per_microsecond = (double)it / microseconds;
    _rate = ops_per_microsecond * 1000000;
    std::cout << _test << ": rate: " << _rate
              << " seconds: " << microseconds / 1000000 << std::endl;
  }
  uint32_t get_rate() { return _rate; }
  switch_status_t get_status() { return _status; }
  void set_status(switch_status_t status) { _status = status; }
};

/**
 * This is helper class which contains some common methods used by
 * all Strategy interface classes.
 */
class ITableHelper {
 public:
  const uint32_t max_ports = 512;
  const uint32_t max_vlans = 4096;
  const uint32_t max_vrfs = 10;
  const uint32_t max_nhops = 10;
  const uint32_t max_rifs = 10;

  static void vrf_gen(std::vector<switch_object_id_t> &list,
                      switch_object_id_t device,
                      uint32_t count) {
    std::set<attr_w> vrf_attrs;
    vrf_attrs.insert(attr_w(SWITCH_VRF_ATTR_DEVICE, device));
    for (uint32_t i = 0; i < count; i++) {
      switch_object_id_t vrf_handle;
      bf_switch_object_create(SWITCH_OBJECT_TYPE_VRF, vrf_attrs, vrf_handle);
      list.push_back(vrf_handle);
    }
  }

  static void object_handle_list_get(
      std::vector<switch_object_id_t> &object_handle_list,
      const switch_object_type_t object_type,
      const uint32_t max_obj) {
    uint32_t num_obj(0);
    switch_object_id_t first_handle = {0};
    bf_switch_get_first_handle(object_type, first_handle);
    bf_switch_get_next_handles(
        first_handle, max_obj, object_handle_list, num_obj);
  }

  static void port_handle_list_get(std::vector<switch_object_id_t> &list,
                                   const uint32_t max_obj) {
    std::vector<switch_object_id_t> handle_list;
    object_handle_list_get(handle_list, SWITCH_OBJECT_TYPE_PORT, max_obj);
    for (const auto &handle : handle_list) {
      switch_enum_t type = {};
      attr_w port_type(SWITCH_PORT_ATTR_TYPE);
      bf_switch_attribute_get(handle, SWITCH_PORT_ATTR_TYPE, port_type);
      port_type.v_get(type);
      if (type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL) continue;
      list.push_back(handle);
    }
  }

  static void mac_addr_list_gen(std::vector<switch_mac_addr_t> &list,
                                const uint32_t entries) {
    switch_mac_addr_t addr = {0x10, 0x20, 0x30, 0x00, 0x00, 0x00};
    for (int64_t n = 1; n <= entries; n++) {
      addr.mac[5] = n % 256;
      addr.mac[4] = (n >> 8) % 256;
      list.push_back(addr);
    }
  }

  static void mac_addr_list_gen(std::vector<uint64_t> &list,
                                const uint32_t entries) {
    uint64_t addr = 0x102030000000;
    for (int64_t n = 1; n <= entries; n++) {
      list.push_back(++addr);
    }
  }

  static void ip4_addr_list_gen(std::vector<switch_ip_address_t> &list,
                                const uint32_t entries) {
    switch_ip_address_t addr = {};
    addr.ip4 = 0x0A0B0001;
    addr.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
    for (int64_t n = 1; n <= entries; n++) {
      list.push_back(addr);
      addr.ip4++;
    }
  }

  static void ip4_prefix_list_gen(std::vector<switch_ip_prefix_t> &list,
                                  const uint32_t entries) {
    switch_ip_prefix_t prefix = {};
    prefix.addr.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
    prefix.len = 32;
    prefix.addr.ip4 = 168427521;
    for (int64_t n = 1; n <= entries; n++) {
      list.push_back(prefix);
      prefix.addr.ip4++;
    }
  }

  static void ip6_prefix_list_gen(std::vector<switch_ip_prefix_t> &list,
                                  const uint32_t entries) {
    switch_ip_prefix_t prefix = {};
    prefix.addr.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6;
    prefix.len = 128;
    for (std::uint8_t i = 0; i < 16; i++) {
      prefix.addr.ip6[i] = 1;
    }
    for (int64_t n = 0; n < entries; n++) {
      prefix.addr.ip6[4] = n % 256;
      prefix.addr.ip6[5] = (n >> 8) % 256;
      list.push_back(prefix);
    }
  }

  /**
   * submodules folder present if using p4factory
   * pkgsrc folder present if using SDE package
   */
  static switch_status_t ip4_prefix_list_read(
      std::vector<switch_ip_prefix_t> &list, const uint32_t entries) {
    switch_status_t ret = SWITCH_STATUS_SUCCESS;
    FILE *pFile;
    std::string file_name = "empty";
    DIR *p4f = opendir("submodules");
    DIR *sde = opendir("pkgsrc");
    if (p4f && sde) {
      std::cout << "Found submodules/ and pkgsrc/ dirs which is not acceptable."
                << std::endl;
      closedir(p4f);
      closedir(sde);
      ret = SWITCH_STATUS_ITEM_NOT_FOUND;
      return ret;
    } else if (p4f) {
      file_name = "./submodules/bf-switch/ptf/api/ipv4_route_table.txt";
      closedir(p4f);
    } else if (sde) {
      file_name = "./pkgsrc/switch-p4-16/ptf/api/ipv4_route_table.txt";
      closedir(sde);
    } else {
      std::cout << "Unable to find submodules/ or pkgsrc/ dir." << std::endl;
      ret = SWITCH_STATUS_ITEM_NOT_FOUND;
      return ret;
    }
    char buffer[50];
    int64_t n(0);

    std::uint32_t octet_1{}, octet_2{}, octet_3{}, octet_4{};
    std::uint32_t mask{};
    pFile = fopen(file_name.c_str(), "r");
    if (pFile == NULL) {
      perror("Error opening file");
      ret = SWITCH_STATUS_INSUFFICIENT_RESOURCES;
    } else {
      while (!feof(pFile) && (n++ < entries)) {
        if (fgets(buffer, 50, pFile) == NULL) {
          break;
        }
        sscanf(buffer,
               "%u.%u.%u.%u %u",
               &octet_1,
               &octet_2,
               &octet_3,
               &octet_4,
               &mask);
        switch_ip_prefix_t prefix = {};
        prefix.len = mask;
        prefix.addr.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
        prefix.addr.ip4 =
            (octet_1 << 24) + (octet_2 << 16) + (octet_3 << 8) + octet_4;
        list.push_back(prefix);
      }
      fclose(pFile);
    }

    return ret;
  }

  /**
   * submodules folder present if using p4factory
   * pkgsrc folder present if using SDE package
   */
  static switch_status_t ip6_prefix_list_read(
      std::vector<switch_ip_prefix_t> &list, const uint32_t entries) {
    switch_status_t ret = SWITCH_STATUS_SUCCESS;
    FILE *pFile;
    std::string file_name = "empty";
    DIR *p4f = opendir("submodules");
    DIR *sde = opendir("pkgsrc");
    if (p4f && sde) {
      std::cout << "Found submodules/ and pkgsrc/ dirs which is not acceptable."
                << std::endl;
      closedir(p4f);
      closedir(sde);
      ret = SWITCH_STATUS_ITEM_NOT_FOUND;
      return ret;
    } else if (p4f) {
      file_name = "./submodules/bf-switch/ptf/api/ipv6_lpm64_route_table.txt";
      closedir(p4f);
    } else if (sde) {
      file_name = "./pkgsrc/switch-p4-16/ptf/api/ipv6_lpm64_route_table.txt";
      closedir(sde);
    } else {
      std::cout << "Unable to find submodules/ or pkgsrc/ dir." << std::endl;
      ret = SWITCH_STATUS_ITEM_NOT_FOUND;
      return ret;
    }
    char buffer[100];
    int inet_ret_code{};
    int64_t n(0);

    pFile = fopen(file_name.c_str(), "r");
    if (pFile == NULL) {
      perror("Error opening file");
      ret = SWITCH_STATUS_INSUFFICIENT_RESOURCES;
    } else {
      while (!feof(pFile) && (n++ < entries)) {
        if (fgets(buffer, 100, pFile) == NULL) {
          break;
        }
        switch_ip_prefix_t prefix = {};
        std::string buffer_str{buffer};
        std::string address = buffer_str.substr(0, buffer_str.find("/"));
        /* TODO get rid of magic numbers */
        std::string mask = buffer_str.substr(buffer_str.find("/") + 1, 2);

        inet_ret_code = inet_pton(AF_INET6, address.c_str(), &prefix.addr.ip6);
        if (inet_ret_code != 1) {
          std::cout << "Inet return code: " << inet_ret_code << std::endl;
          ret = SWITCH_STATUS_INVALID_PARAMETER;
        }
        prefix.len = std::atoi(mask.c_str());
        prefix.addr.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6;
        list.push_back(prefix);
      }
      fclose(pFile);
    }

    return ret;
  }
};

/**
 * This is Strategy pattern interface declares run() operation defined in all
 * versions of Concrete classes that inherit from this class.
 *
 * The ApiPerf class uses this interface to call the algorithm defined by
 * Concrete Strategies (e.g. ITableBMAIMacEntry, ITableBMAIRoute).
 */
class ITable {
 protected:
  std::vector<switch_object_id_t> _port_handle_list;
  std::vector<switch_object_id_t> _vlan_handle_list;
  std::vector<switch_object_id_t> _vrf_handle_list;
  std::vector<switch_object_id_t> _nhop_handle_list;
  std::vector<switch_object_id_t> _rif_handle_list;

 public:
  virtual ~ITable() {}
  // run is an overloaded API. It is used both to measure insertion performance
  // while at the same time to add entries to the table
  virtual PerfResult run(uint32_t entries, bool batch) = 0;
  virtual void clean() = 0;

  static std::string get_prof_file_name(std::string test_name,
                                        uint32_t entries,
                                        bool batch) {
    std::string prof_file_name = {};

    prof_file_name.append("prof-");
    prof_file_name.append(test_name);
    prof_file_name.append("-");
    prof_file_name.append(std::to_string(entries));
    if (batch) {
      prof_file_name.append("-batch");
    }
    prof_file_name.append(".data");

    return prof_file_name;
  }
};

class ITableBMAIMacEntry : public ITable, public ITableHelper {
 private:
  const uint32_t max_entries = 15000;
  std::vector<switch_object_id_t> _mac_entries_handles;
  std::vector<switch_mac_addr_t> _mac_addr_list;
  switch_object_id_t _device;
  std::string _test_name;

 public:
  ITableBMAIMacEntry(switch_object_id_t device)
      : _device(device), _test_name("bmai-mac-entry") {
    port_handle_list_get(_port_handle_list, max_ports);
    object_handle_list_get(
        _vlan_handle_list, SWITCH_OBJECT_TYPE_VLAN, max_vlans);
  }
  ~ITableBMAIMacEntry() {}
  void clean() {
    for (auto &handle : _mac_entries_handles) {
      bf_switch_object_delete(handle);
    }
  }
  PerfResult run(uint32_t entries, bool batch);
};

class ITableBFRTMacEntry : public ITable, public ITableHelper {
 private:
  const uint32_t max_entries = 15000;
  switch_enum_t entry_type = {.enumdata = SWITCH_MAC_ENTRY_ATTR_TYPE_STATIC};
  std::vector<uint64_t> _mac_addr_list;
  switch_object_id_t _device;
  std::string _test_name;

 public:
  ITableBFRTMacEntry(switch_object_id_t device)
      : _device(device), _test_name("bfrt-mac-entry") {
    port_handle_list_get(_port_handle_list, max_ports);
    object_handle_list_get(
        _vlan_handle_list, SWITCH_OBJECT_TYPE_VLAN, max_vlans);
  }
  ~ITableBFRTMacEntry() {}
  void clean() {
    _Table table_dmac(get_dev_tgt(), get_bf_rt_info(), smi_id::T_DMAC);
    _Table table_smac(get_dev_tgt(), get_bf_rt_info(), smi_id::T_SMAC);
    table_dmac.clear_entries();
    table_smac.clear_entries();
  }
  PerfResult run(uint32_t entries, bool batch);
};

class ITableBMAIAclIpv4 : public ITable, public ITableHelper {
 private:
  const uint32_t max_entries = 2000;
  switch_object_id_t _acl_table_handle = {};
  std::vector<switch_object_id_t> _acl_entry_handles;
  std::vector<switch_ip_address_t> _ip4_addr_list;
  switch_object_id_t _device;
  std::string _test_name;

 public:
  ITableBMAIAclIpv4(switch_object_id_t device)
      : _device(device), _test_name("bmai-acl-ip4") {}
  ~ITableBMAIAclIpv4() {}
  void clean() {
    for (auto &handle : _acl_entry_handles) {
      bf_switch_object_delete(handle);
    }
    bf_switch_object_delete(_acl_table_handle);
  }
  PerfResult run(uint32_t entries, bool batch);
};

class ITableBFRTAclIpv4 : public ITable, public ITableHelper {
 private:
  const uint32_t max_entries = 2000;
  std::vector<switch_ip_address_t> _ip4_addr_list;
  switch_object_id_t _device;
  std::string _test_name;

 public:
  ITableBFRTAclIpv4(switch_object_id_t device)
      : _device(device), _test_name("bfrt-acl-ip4") {}
  ~ITableBFRTAclIpv4() {}
  void clean() {
    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_INGRESS_IPV4_ACL);
    table.clear_entries();
  }
  PerfResult run(uint32_t entries, bool batch);
};

class ITableBMAIHostRouteIpv4 : public ITable, public ITableHelper {
 private:
  const uint32_t max_entries = 65000;
  std::vector<switch_object_id_t> _route_handles;
  std::vector<switch_ip_prefix_t> _prefix_list;
  switch_object_id_t _device;
  std::string _test_name;

 public:
  ITableBMAIHostRouteIpv4(switch_object_id_t device)
      : _device(device), _test_name("bmai-route") {
    vrf_gen(_vrf_handle_list, device, max_vrfs);
    object_handle_list_get(
        _nhop_handle_list, SWITCH_OBJECT_TYPE_NEXTHOP, max_nhops);
  }
  ~ITableBMAIHostRouteIpv4() {}
  void clean() {
    for (auto &handle : _route_handles) {
      bf_switch_object_delete(handle);
    }
    for (auto &handle : _vrf_handle_list) {
      bf_switch_object_delete(handle);
    }
  }
  PerfResult run(uint32_t entries, bool batch);
};

class ITableBFRTHostRouteIpv4 : public ITable, public ITableHelper {
 private:
  const uint32_t max_entries = 65000;
  std::vector<switch_ip_prefix_t> _prefix_list;
  switch_object_id_t _device;
  std::string _test_name;

 public:
  ITableBFRTHostRouteIpv4(switch_object_id_t device)
      : _device(device), _test_name("bfrt-route") {
    vrf_gen(_vrf_handle_list, device, max_vrfs);
    object_handle_list_get(
        _nhop_handle_list, SWITCH_OBJECT_TYPE_NEXTHOP, max_nhops);
  }
  ~ITableBFRTHostRouteIpv4() {}
  void clean() {
    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_IPV4_FIB_HOST);
    table.clear_entries();
    for (auto &handle : _vrf_handle_list) {
      bf_switch_object_delete(handle);
    }
  }
  PerfResult run(uint32_t entries, bool batch);
};

class ITableBMAIHostRouteIpv6 : public ITable, public ITableHelper {
 private:
  const uint32_t max_entries = 32000;
  std::vector<switch_object_id_t> _route_handles;
  std::vector<switch_ip_prefix_t> _prefix_list;
  switch_object_id_t _device;
  std::string _test_name;

 public:
  ITableBMAIHostRouteIpv6(switch_object_id_t device)
      : _device(device), _test_name("bmai-route6") {
    vrf_gen(_vrf_handle_list, device, max_vrfs);
    object_handle_list_get(
        _nhop_handle_list, SWITCH_OBJECT_TYPE_NEXTHOP, max_nhops);
  }
  ~ITableBMAIHostRouteIpv6() {}
  void clean() {
    for (auto &handle : _route_handles) {
      bf_switch_object_delete(handle);
    }
    for (auto &handle : _vrf_handle_list) {
      bf_switch_object_delete(handle);
    }
  }
  PerfResult run(uint32_t entries, bool batch);
};

class ITableBFRTHostRouteIpv6 : public ITable, public ITableHelper {
 private:
  const uint32_t max_entries = 32000;
  std::vector<switch_ip_prefix_t> _prefix_list;
  switch_object_id_t _device;
  std::string _test_name;

 public:
  ITableBFRTHostRouteIpv6(switch_object_id_t device)
      : _device(device), _test_name("bfrt-route6") {
    vrf_gen(_vrf_handle_list, device, max_vrfs);
    object_handle_list_get(
        _nhop_handle_list, SWITCH_OBJECT_TYPE_NEXTHOP, max_nhops);
  }
  ~ITableBFRTHostRouteIpv6() {}
  void clean() {
    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_IPV6_FIB_HOST);
    table.clear_entries();
    for (auto &handle : _vrf_handle_list) {
      bf_switch_object_delete(handle);
    }
  }
  PerfResult run(uint32_t entries, bool batch);
};

class ITableBMAILPMRouteIpv4 : public ITable, public ITableHelper {
 private:
  const uint32_t max_entries = 120000;
  std::vector<switch_object_id_t> _route_handles;
  std::vector<switch_ip_prefix_t> _prefix_list;
  switch_object_id_t _device;
  std::string _test_name;

 public:
  ITableBMAILPMRouteIpv4(switch_object_id_t device)
      : _device(device), _test_name("bmai-route-lpm") {
    vrf_gen(_vrf_handle_list, device, max_vrfs);
    object_handle_list_get(
        _nhop_handle_list, SWITCH_OBJECT_TYPE_NEXTHOP, max_nhops);
  }
  ~ITableBMAILPMRouteIpv4() {}
  void clean() {
    for (auto &handle : _route_handles) {
      bf_switch_object_delete(handle);
    }
    for (auto &handle : _vrf_handle_list) {
      bf_switch_object_delete(handle);
    }
  }
  PerfResult run(uint32_t entries, bool batch);
};

class ITableBFRTLPMRouteIpv4 : public ITable, public ITableHelper {
 private:
  const uint32_t max_entries = 120000;
  std::vector<switch_ip_prefix_t> _prefix_list;
  switch_object_id_t _device;
  std::string _test_name;

 public:
  ITableBFRTLPMRouteIpv4(switch_object_id_t device)
      : _device(device), _test_name("bfrt-route-lpm") {
    vrf_gen(_vrf_handle_list, device, max_vrfs);
    object_handle_list_get(
        _nhop_handle_list, SWITCH_OBJECT_TYPE_NEXTHOP, max_nhops);
  }
  ~ITableBFRTLPMRouteIpv4() {}
  void clean() {
    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_IPV4_FIB_LPM);
    table.clear_entries();
    for (auto &handle : _vrf_handle_list) {
      bf_switch_object_delete(handle);
    }
  }
  PerfResult run(uint32_t entries, bool batch);
};

class ITableBMAILPMRouteIpv6 : public ITable, public ITableHelper {
 private:
  const uint32_t max_entries = 200000;
  std::vector<switch_object_id_t> _route_handles;
  std::vector<switch_ip_prefix_t> _prefix_list;
  switch_object_id_t _device;
  std::string _test_name;

 public:
  ITableBMAILPMRouteIpv6(switch_object_id_t device)
      : _device(device), _test_name("bmai-route6-lpm") {
    vrf_gen(_vrf_handle_list, device, max_vrfs);
    object_handle_list_get(
        _nhop_handle_list, SWITCH_OBJECT_TYPE_NEXTHOP, max_nhops);
  }
  ~ITableBMAILPMRouteIpv6() {}
  void clean() {
    for (auto &handle : _route_handles) {
      bf_switch_object_delete(handle);
    }
    for (auto &handle : _vrf_handle_list) {
      bf_switch_object_delete(handle);
    }
  }
  PerfResult run(uint32_t entries, bool batch);
};

class ITableBFRTLPMRouteIpv6 : public ITable, public ITableHelper {
 private:
  const uint32_t max_entries = 200000;
  std::vector<switch_ip_prefix_t> _prefix_list;
  switch_object_id_t _device;
  std::string _test_name;

 public:
  ITableBFRTLPMRouteIpv6(switch_object_id_t device)
      : _device(device), _test_name("bfrt-route6-lpm") {
    vrf_gen(_vrf_handle_list, device, max_vrfs);
    object_handle_list_get(
        _nhop_handle_list, SWITCH_OBJECT_TYPE_NEXTHOP, max_nhops);
  }
  ~ITableBFRTLPMRouteIpv6() {}
  void clean() {
    _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_IPV6_FIB_LPM64);
    table.clear_entries();
    for (auto &handle : _vrf_handle_list) {
      bf_switch_object_delete(handle);
    }
  }
  PerfResult run(uint32_t entries, bool batch);
};

class ITableBMAINexthop : public ITable, public ITableHelper {
 private:
  const uint32_t max_entries = 24000;
  std::vector<switch_object_id_t> _nhop_handles;
  std::vector<switch_ip_address_t> _ip4_addr_list;
  switch_object_id_t _device;
  std::string _test_name;

 public:
  ITableBMAINexthop(switch_object_id_t device)
      : _device(device), _test_name("bmai-nexthop") {
    object_handle_list_get(_rif_handle_list, SWITCH_OBJECT_TYPE_RIF, max_rifs);
  }
  ~ITableBMAINexthop() {}
  void clean() {
    for (auto &handle : _nhop_handles) {
      bf_switch_object_delete(handle);
    }
  }
  PerfResult run(uint32_t entries, bool batch);
};

class ITableBMAIVlan : public ITable, public ITableHelper {
 private:
  const uint32_t max_entries = 4095;
  std::vector<switch_object_id_t> _vlan_handles;
  switch_object_id_t _device;
  std::string _test_name;

 public:
  ITableBMAIVlan(switch_object_id_t device)
      : _device(device), _test_name("bmai-vlan") {}
  ~ITableBMAIVlan() {}
  void clean() {
    for (auto &handle : _vlan_handles) {
      bf_switch_object_delete(handle);
    }
  }
  PerfResult run(uint32_t entries, bool batch);
};

/* TO DO - corresponding test to be fixed within multicast enabling works
class ITableBMAIMcastMember : public ITable, public ITableHelper {
 private:
  const uint32_t max_entries = 15000;
  std::vector<switch_object_id_t> _vlan_handles;
  std::vector<switch_object_id_t> _mcast_group_handles;
  std::vector<switch_object_id_t> _mcast_member_handles;
  switch_object_id_t _device;
  std::string _test_name;

 public:
  ITableBMAIMcastMember(switch_object_id_t device)
      : _device(device), _test_name("bmai-mcast-member") {}
  ~ITableBMAIMcastMember() {
    for (auto &handle : _mcast_member_handles) {
      bf_switch_object_delete(handle);
    }
    for (auto &handle : _mcast_group_handles) {
      bf_switch_object_delete(handle);
    }
    for (auto &handle : _vlan_handles) {
      bf_switch_object_delete(handle);
    }
  }
  PerfResult run(uint32_t entries, bool batch);
};
*/

/**
 * This is class defines the interface used by clients. It contains dispatcher
 * which is resposnisble for alloacting Concrete interface according to the
 * parameters passed in the constructor.
 */
class ApiPerf {
 private:
  const std::map<
      std::pair<switcht_perf_api::type, switcht_perf_table::type>,
      std::function<std::unique_ptr<smi::ITable>(switch_object_id_t device)> >
      table_map{
          {std::make_pair(switcht_perf_api::BMAI,
                          switcht_perf_table::MAC_ENTRY),
           [](switch_object_id_t device) {
             return std::unique_ptr<smi::ITable>(
                 new ITableBMAIMacEntry(device));
           }},

          {std::make_pair(switcht_perf_api::BMAI, switcht_perf_table::ACL_IPV4),
           [](switch_object_id_t device) {
             return std::unique_ptr<smi::ITable>(new ITableBMAIAclIpv4(device));
           }},

          {std::make_pair(switcht_perf_api::BMAI,
                          switcht_perf_table::HOST_ROUTE_IPV4),
           [](switch_object_id_t device) {
             return std::unique_ptr<smi::ITable>(
                 new ITableBMAIHostRouteIpv4(device));
           }},

          {std::make_pair(switcht_perf_api::BMAI,
                          switcht_perf_table::HOST_ROUTE_IPV6),
           [](switch_object_id_t device) {
             return std::unique_ptr<smi::ITable>(
                 new ITableBMAIHostRouteIpv6(device));
           }},

          {std::make_pair(switcht_perf_api::BMAI,
                          switcht_perf_table::LPM_ROUTE_IPV4),
           [](switch_object_id_t device) {
             return std::unique_ptr<smi::ITable>(
                 new ITableBMAILPMRouteIpv4(device));
           }},

          {std::make_pair(switcht_perf_api::BMAI,
                          switcht_perf_table::LPM_ROUTE_IPV6),
           [](switch_object_id_t device) {
             return std::unique_ptr<smi::ITable>(
                 new ITableBMAILPMRouteIpv6(device));
           }},

          {std::make_pair(switcht_perf_api::BMAI, switcht_perf_table::NEXTHOP),
           [](switch_object_id_t device) {
             return std::unique_ptr<smi::ITable>(new ITableBMAINexthop(device));
           }},

          {std::make_pair(switcht_perf_api::BMAI, switcht_perf_table::VLAN),
           [](switch_object_id_t device) {
             return std::unique_ptr<smi::ITable>(new ITableBMAIVlan(device));
           }},
          /* TO DO - corresponding test to be fixed within multicast enabling
             works {std::make_pair(switcht_perf_api::BMAI,
                                    switcht_perf_table::MCAST_MEMBER),
                     [](switch_object_id_t device) {
                       return new ITableBMAIMcastMember(device);
                     }},
          */
          {std::make_pair(switcht_perf_api::BFRT,
                          switcht_perf_table::MAC_ENTRY),
           [](switch_object_id_t device) {
             return std::unique_ptr<smi::ITable>(
                 new ITableBFRTMacEntry(device));
           }},

          {std::make_pair(switcht_perf_api::BFRT, switcht_perf_table::ACL_IPV4),
           [](switch_object_id_t device) {
             return std::unique_ptr<smi::ITable>(new ITableBFRTAclIpv4(device));
           }},

          {std::make_pair(switcht_perf_api::BFRT,
                          switcht_perf_table::HOST_ROUTE_IPV4),
           [](switch_object_id_t device) {
             return std::unique_ptr<smi::ITable>(
                 new ITableBFRTHostRouteIpv4(device));
           }},

          {std::make_pair(switcht_perf_api::BFRT,
                          switcht_perf_table::HOST_ROUTE_IPV6),
           [](switch_object_id_t device) {
             return std::unique_ptr<smi::ITable>(
                 new ITableBFRTHostRouteIpv6(device));
           }},

          {std::make_pair(switcht_perf_api::BFRT,
                          switcht_perf_table::LPM_ROUTE_IPV4),
           [](switch_object_id_t device) {
             return std::unique_ptr<smi::ITable>(
                 new ITableBFRTLPMRouteIpv4(device));
           }},

          {std::make_pair(switcht_perf_api::BFRT,
                          switcht_perf_table::LPM_ROUTE_IPV6),
           [](switch_object_id_t device) {
             return std::unique_ptr<smi::ITable>(
                 new ITableBFRTLPMRouteIpv6(device));
           }}};

  std::unique_ptr<smi::ITable> _table = nullptr;

 public:
  ApiPerf(const switch_object_id_t device,
          const switcht_perf_api::type api,
          const switcht_perf_table::type table) {
    std::cout << std::endl;
    std::cout << "ApiPerf: api:" << api << " table:" << table << std::endl;

    bf_sys_trace_level_set(BF_MOD_SWITCHAPI, BF_LOG_ERR);
    bf_sys_log_level_set(BF_MOD_SWITCHAPI, BF_LOG_DEST_FILE, BF_LOG_ERR);

    if (table_map.find(std::make_pair(api, table)) != table_map.end()) {
      this->_table = table_map.at(std::make_pair(api, table))(device);
    } else {
      std::cout << "ApiPerf: ERROR: Not supported" << std::endl;
    }
  }

  PerfResult run(uint32_t entries, bool batch) {
    PerfResult test_result = {};
    if (this->_table) {
      test_result = this->_table->run(entries, batch);
    } else {
      test_result.set_status(SWITCH_STATUS_NOT_IMPLEMENTED);
    }
    return test_result;
  }

  void clean() {
    if (this->_table) {
      this->_table->clean();
    }
    return;
  }
};

}  // namespace smi

#endif  // __TEST_API_PERF_H__
