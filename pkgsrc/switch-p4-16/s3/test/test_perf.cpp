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

#include <cassert>
#include <iostream>
#include <chrono>
#include <list>
#include <set>
#include <thread>
#include <algorithm>
#include <functional>

#include <arpa/inet.h>
#include "time.h"
#include "gen-model/replay.h"
#include "bf_switch/bf_switch_types.h"
#include "../id_gen.h"
#include "s3/attribute.h"
#include "s3/attribute_util.h"
#include "s3/switch_store.h"
#include "s3/factory.h"
#include "../log.h"

//#define __CPU_PROFILER__
#ifdef __CPU_PROFILER__
#include "gperftools/profiler.h"
#endif

using namespace smi;

static ModelInfo *model_info = NULL;

class ipv4_fib : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_IPV4_FIB;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_IPV4_FIB_ATTR_PARENT_HANDLE;

 public:
  ipv4_fib(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    (void)status;
    switch_object_id_t device = {}, nexthop = {}, vrf = {};
    switch_ip_prefix_t ip_prefix = {};
    std::vector<std::reference_wrapper<const switch_attribute_t>> route_attrs;
    status = switch_store::attribute_get_all(parent, route_attrs);
    for (const auto &ref_attr : route_attrs) {
      const switch_attribute_t &attr = ref_attr.get();
      switch (attr.id) {
        case SWITCH_ROUTE_ATTR_DEVICE:
          device = attr.value.oid;
          break;
        case SWITCH_ROUTE_ATTR_IP_PREFIX:
          attr_util::v_get(attr.value, ip_prefix);
          break;
        case SWITCH_ROUTE_ATTR_VRF:
          vrf = attr.value.oid;
          break;
        case SWITCH_ROUTE_ATTR_NEXTHOP:
          nexthop = attr.value.oid;
          break;
        default:
          break;
      }
    }
    // switch_store::v_get(parent, SWITCH_ROUTE_ATTR_DEVICE, device);
    // switch_store::v_get(parent, SWITCH_ROUTE_ATTR_IP_PREFIX, ip_prefix);
    // switch_store::v_get(parent, SWITCH_ROUTE_ATTR_VRF, vrf);
    // switch_store::v_get(parent, SWITCH_ROUTE_ATTR_NEXTHOP, nexthop);
  }
  switch_status_t create_update() { return SWITCH_STATUS_SUCCESS; }
  switch_status_t del() { return SWITCH_STATUS_SUCCESS; }
};

class smac : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_SMAC;
  static const switch_attr_id_t parent_attr_id = SWITCH_SMAC_ATTR_PARENT_HANDLE;

 public:
  smac(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    (void)status;
    switch_object_id_t device = {}, port = {}, vrf = {};
    switch_mac_addr_t mac = {};
    std::vector<std::reference_wrapper<const switch_attribute_t>> mac_attrs;
    status = switch_store::attribute_get_all(parent, mac_attrs);
    for (const auto &ref_attr : mac_attrs) {
      const switch_attribute_t &attr = ref_attr.get();
      switch (attr.id) {
        case SWITCH_FDB_ATTR_DEVICE:
          device = attr.value.oid;
          break;
        case SWITCH_FDB_ATTR_MAC_ADDRESS:
          attr_util::v_get(attr.value, mac);
          break;
        case SWITCH_FDB_ATTR_VRF:
          vrf = attr.value.oid;
          break;
        case SWITCH_FDB_ATTR_PORT:
          port = attr.value.oid;
          break;
        default:
          break;
      }
    }
    // switch_store::v_get(parent, SWITCH_FDB_ATTR_DEVICE, device);
    // switch_store::v_get(parent, SWITCH_FDB_ATTR_MAC_ADDRESS, mac);
    // switch_store::v_get(parent, SWITCH_FDB_ATTR_VRF, vrf);
    // switch_store::v_get(parent, SWITCH_FDB_ATTR_PORT, port);
  }
  switch_status_t create_update() { return SWITCH_STATUS_SUCCESS; }
  switch_status_t del() { return SWITCH_STATUS_SUCCESS; }
};

class dmac : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_DMAC;
  static const switch_attr_id_t parent_attr_id = SWITCH_DMAC_ATTR_PARENT_HANDLE;

 public:
  dmac(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    (void)status;
    switch_object_id_t device = {}, port = {}, vrf = {};
    switch_mac_addr_t mac = {};
    std::vector<std::reference_wrapper<const switch_attribute_t>> mac_attrs;
    status = switch_store::attribute_get_all(parent, mac_attrs);
    for (const auto &ref_attr : mac_attrs) {
      const switch_attribute_t &attr = ref_attr.get();
      switch (attr.id) {
        case SWITCH_FDB_ATTR_DEVICE:
          device = attr.value.oid;
          break;
        case SWITCH_FDB_ATTR_MAC_ADDRESS:
          attr_util::v_get(attr.value, mac);
          break;
        case SWITCH_FDB_ATTR_VRF:
          vrf = attr.value.oid;
          break;
        case SWITCH_FDB_ATTR_PORT:
          port = attr.value.oid;
          break;
        default:
          break;
      }
    }
    // switch_store::v_get(parent, SWITCH_FDB_ATTR_DEVICE, device);
    // switch_store::v_get(parent, SWITCH_FDB_ATTR_MAC_ADDRESS, mac);
    // switch_store::v_get(parent, SWITCH_FDB_ATTR_VRF, vrf);
    // switch_store::v_get(parent, SWITCH_FDB_ATTR_PORT, port);
  }
  switch_status_t create_update() { return SWITCH_STATUS_SUCCESS; }
  switch_status_t del() { return SWITCH_STATUS_SUCCESS; }
};

class rate {
 public:
  rate() {}
  rate(double _ms, uint32_t _rate) : ms_(_ms), rate_(_rate) {}
  double ms_ = 0;
  uint32_t rate_ = 0;
  rate operator+=(const rate rhs) {
    this->ms_ += rhs.ms_;
    this->rate_ += rhs.rate_;
    return *this;
  }
};
uint64_t iter = 200000;
rate compute_rate(struct timespec start,
                  struct timespec end,
                  uint64_t it,
                  std::string test) {
  struct timespec diff = {0};
  double microseconds = 0;
  double ops_per_microsecond = 0;
  uint32_t rate = 0;
  diff.tv_sec = end.tv_sec - start.tv_sec;
  diff.tv_nsec = end.tv_nsec - start.tv_nsec;
  if (diff.tv_nsec < 0) {
    diff.tv_sec -= 1;
    diff.tv_nsec += 1000000000;
  }
  microseconds = diff.tv_sec * 1000000 + diff.tv_nsec / 1000;
  ops_per_microsecond = (double)it / microseconds;
  rate = ops_per_microsecond * 1000000;
  std::cout << test << " Rate: " << rate
            << " Seconds: " << microseconds / 1000000 << std::endl;
  return {microseconds, rate};
}

static switch_object_id_t device = {}, port = {}, vrf = {}, nexthop = {};

static std::vector<std::set<attr_w>> mac_entries_attrs;
static std::vector<std::set<attr_w>> v4_routes_attrs;
static std::vector<std::set<attr_w>> v6_routes_attrs;
void generate_random_mac_entries_attrs() {
  switch_mac_addr_t mac = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  attr_w device_attr = attr_w(SWITCH_FDB_ATTR_DEVICE, device);
  attr_w vrf_attr = attr_w(SWITCH_FDB_ATTR_VRF, vrf);
  attr_w port_attr = attr_w(SWITCH_FDB_ATTR_PORT, port);

  std::set<attr_w> attrs{device_attr, vrf_attr, port_attr};
  srand(clock());
  std::set<std::pair<uint64_t, uint64_t>> mac_pairs;
  for (uint64_t i = 0; i < iter;) {
    mac.mac[0] = rand() % 255;
    mac.mac[1] = rand() % 255;
    mac.mac[2] = rand() % 255;
    mac.mac[3] = rand() % 255;
    mac.mac[4] = rand() % 255;
    mac.mac[5] = rand() % 255;
    uint32_t upper = 0;
    uint16_t lower = 0;
    std::memcpy(&upper, &mac.mac[0], sizeof(upper));
    std::memcpy(&lower, &mac.mac[4], sizeof(lower));
    auto result = mac_pairs.insert(std::make_pair(upper, lower));
    if (result.second == true) {
      attr_w mac_attr = attr_w(SWITCH_FDB_ATTR_MAC_ADDRESS, mac);
      attrs.insert(std::move(mac_attr));
      mac_entries_attrs.push_back(attrs);
      attrs.erase(mac_attr);
      i++;
    }
  }
}

void generate_random_ipv4_entries_attrs() {
  attr_w device_attr = attr_w(SWITCH_ROUTE_ATTR_DEVICE, device);
  attr_w vrf_attr = attr_w(SWITCH_ROUTE_ATTR_VRF, vrf);
  attr_w nexthop_attr = attr_w(SWITCH_ROUTE_ATTR_NEXTHOP, nexthop);
  switch_ip_prefix_t ip_prefix = {};
  ip_prefix.len = 32;
  ip_prefix.addr.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
  ip_prefix.addr.ip4 = 168427521;

  std::set<attr_w> attrs{device_attr, vrf_attr, nexthop_attr};
  for (uint64_t i = 0; i < iter; i++) {
    attr_w ip_attr = attr_w(SWITCH_ROUTE_ATTR_IP_PREFIX, ip_prefix);
    attrs.insert(ip_attr);
    v4_routes_attrs.push_back(attrs);
    attrs.erase(ip_attr);
    ip_prefix.addr.ip4++;
  }
}

void generate_random_ipv6_entries_attrs() {
  attr_w device_attr = attr_w(SWITCH_ROUTE_ATTR_DEVICE, device);
  attr_w vrf_attr = attr_w(SWITCH_ROUTE_ATTR_VRF, vrf);
  attr_w nexthop_attr = attr_w(SWITCH_ROUTE_ATTR_NEXTHOP, nexthop);
  std::string str = "4444::1/96";
  switch_ip_prefix_t ip_prefix = {};
  attr_util::parse_ip_prefix(str, ip_prefix);

  std::set<attr_w> attrs{device_attr, vrf_attr, nexthop_attr};
  srand(clock());
  std::set<std::pair<uint64_t, uint64_t>> ip_pairs;
  for (uint64_t i = 0; i < iter;) {
    ip_prefix.addr.ip6[iter % 13] = rand() % 255;
    ip_prefix.addr.ip6[iter % 11] = rand() % 255;
    ip_prefix.addr.ip6[iter % 7] = rand() % 255;
    ip_prefix.addr.ip6[iter % 5] = rand() % 255;
    ip_prefix.addr.ip6[iter % 3] = rand() % 255;
    ip_prefix.addr.ip6[iter % 2] = rand() % 255;
    uint64_t upper = 0;
    uint64_t lower = 0;
    std::memcpy(&upper, &ip_prefix.addr.ip6[0], sizeof(upper));
    std::memcpy(&lower, &ip_prefix.addr.ip6[8], sizeof(lower));
    auto result = ip_pairs.insert(std::make_pair(upper, lower));
    if (result.second == true) {
      attr_w ip_attr = attr_w(SWITCH_ROUTE_ATTR_IP_PREFIX, ip_prefix);
      attrs.insert(ip_attr);
      v6_routes_attrs.push_back(attrs);
      attrs.erase(ip_attr);
      i++;
    }
  }
}

rate test_mac_perf() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  struct timespec start = {0}, end = {0};
  clock_gettime(CLOCK_MONOTONIC, &start);
  for (auto &mac_entry_attrs : mac_entries_attrs) {
    switch_object_id_t oid = {};
    status = switch_store::object_create(
        SWITCH_OBJECT_TYPE_FDB, mac_entry_attrs, oid);
    assert(status == SWITCH_STATUS_SUCCESS);
  }
  clock_gettime(CLOCK_MONOTONIC, &end);
  return compute_rate(start, end, iter, "mac");
}

rate test_ipv4_perf() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  struct timespec start = {0}, end = {0};

  clock_gettime(CLOCK_MONOTONIC, &start);
  for (auto &v4_route_attrs : v4_routes_attrs) {
    switch_object_id_t oid = {};
    status = switch_store::object_create(
        SWITCH_OBJECT_TYPE_ROUTE, v4_route_attrs, oid);
    assert(status == SWITCH_STATUS_SUCCESS);
  }
  clock_gettime(CLOCK_MONOTONIC, &end);
  return compute_rate(start, end, iter, "ipv4");
}

rate test_ipv6_perf() {
  switch_status_t status;
  struct timespec start = {0}, end = {0};

  clock_gettime(CLOCK_MONOTONIC, &start);
  for (auto &route_attrs : v6_routes_attrs) {
    switch_object_id_t oid = {};
    status =
        switch_store::object_create(SWITCH_OBJECT_TYPE_ROUTE, route_attrs, oid);
    assert(status == SWITCH_STATUS_SUCCESS);
  }
  clock_gettime(CLOCK_MONOTONIC, &end);
  return compute_rate(start, end, iter, "ipv6");
}

rate test_ids_reserve_fetch() {
  std::vector<uint32_t> ids;
  struct timespec start = {0}, end = {0};
  clock_gettime(CLOCK_MONOTONIC, &start);
  idAllocator alloc;
  ids.clear();
  for (uint i = 0; i < 10000; i++) {
    ids.push_back(alloc.allocate());
  }
  for (auto id : ids) {
    assert(alloc.release(id) == SWITCH_STATUS_SUCCESS);
  }
  ids.clear();
  assert(alloc.reserve(1024) == SWITCH_STATUS_SUCCESS);
  assert(alloc.get_first() == 1024);
  std::vector<uint32_t> next_n;
  alloc.get_next_n(next_n, 1024, 5000);
  alloc.get_next_n(next_n, 1024, 8000);
  assert(next_n.size() == 0);

  clock_gettime(CLOCK_MONOTONIC, &end);
  return compute_rate(start, end, iter, "ID_allocate_fetch");
}

void test_perf_single() {
  std::cout << "\n**** Tesing objects creation single thread ****" << std::endl;
  rate total = {};
#ifdef __CPU_PROFILER__
  ProfilerStart("./perf_single_thread_data.txt");
  ProfilerFlush();
#endif
  total += test_ipv4_perf();
  total += test_ipv6_perf();
  total += test_mac_perf();
#ifdef __CPU_PROFILER__
  ProfilerStop();
#endif
  std::cout << "total"
            << " Rate: " << (total.rate_ / 3)
            << " Seconds: " << total.ms_ / 1000000 << std::endl;
}

void test_perf_multi() {
  std::cout << "\n**** Testing objects creation multi thread ****" << std::endl;
  struct timespec start = {0}, end = {0};
  clock_gettime(CLOCK_MONOTONIC, &start);
#ifdef __CPU_PROFILER__
  ProfilerStart("./perf_multi_thread_data.txt");
  ProfilerFlush();
#endif
  std::thread tv4(test_ipv4_perf);
  std::thread tv6(test_ipv6_perf);
  std::thread tmac(test_mac_perf);
  tv4.join();
  tv6.join();
  tmac.join();
#ifdef __CPU_PROFILER__
  ProfilerStop();
#endif
  clock_gettime(CLOCK_MONOTONIC, &end);
  compute_rate(start, end, iter * 3, "total");
}

void test_mac_scale() {
  switch_status_t status;
  switch_mac_addr_t mac = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  attr_w device_attr = attr_w(SWITCH_FDB_ATTR_DEVICE, device);
  attr_w vrf_attr = attr_w(SWITCH_FDB_ATTR_VRF, vrf);
  attr_w port_attr = attr_w(SWITCH_FDB_ATTR_PORT, port);

  std::set<attr_w> attrs{device_attr, vrf_attr, port_attr};
  for (uint64_t i = 0; i < 255; i++) {
    for (uint64_t j = 0; j < 255; j++) {
      switch_object_id_t oid = {};
      mac.mac[4] = i;
      mac.mac[5] = j;
      attr_w mac_attr = attr_w(SWITCH_FDB_ATTR_MAC_ADDRESS, mac);
      attrs.insert(std::move(mac_attr));
      status = switch_store::object_create(SWITCH_OBJECT_TYPE_FDB, attrs, oid);
      assert(status == SWITCH_STATUS_SUCCESS);
      attrs.erase(mac_attr);
    }
  }
}
void test_ids_perf() {
  rate total = {};
  total = test_ids_reserve_fetch();
  std::cout << "\n**** Testing creation and fetch of Ids****" << std::endl;
  std::cout << "total"
            << " Rate: " << total.rate_ << " Seconds: " << total.ms_ / 1000000
            << std::endl;
}
void setup() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  const char *const test_model_name = TESTDATADIR "/test/replay.json";
  switch_store::object_info_init(test_model_name, false, NULL);
  assert(status == SWITCH_STATUS_SUCCESS);
  set_log_level(SWITCH_API_LEVEL_ERROR);
  set_log_level_all_objects(SWITCH_API_LEVEL_ERROR);

  model_info = switch_store::switch_model_info_get();
  assert(model_info != NULL);

  std::set<attr_w> attrs;
  status =
      switch_store::object_create(SWITCH_OBJECT_TYPE_DEVICE, attrs, device);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::object_create(SWITCH_OBJECT_TYPE_VRF, attrs, vrf);
  assert(status == SWITCH_STATUS_SUCCESS);
  status =
      switch_store::object_create(SWITCH_OBJECT_TYPE_NEXTHOP, attrs, nexthop);
  assert(status == SWITCH_STATUS_SUCCESS);
  std::set<attr_w> pattrs{attr_w(SWITCH_PORT_ATTR_PORT_DEVICE, device)};
  status = switch_store::object_create(SWITCH_OBJECT_TYPE_PORT, pattrs, port);
  assert(status == SWITCH_STATUS_SUCCESS);

  REGISTER_OBJECT(smac, SWITCH_OBJECT_TYPE_SMAC);
  REGISTER_OBJECT(dmac, SWITCH_OBJECT_TYPE_DMAC);
  REGISTER_OBJECT(ipv4_fib, SWITCH_OBJECT_TYPE_IPV4_FIB);
}

int main(void) {
  // Generate Random v4, v6 and mac entries for perf testing
  mac_entries_attrs.reserve(iter);
  v4_routes_attrs.reserve(iter);
  v6_routes_attrs.reserve(iter);
  generate_random_mac_entries_attrs();
  generate_random_ipv4_entries_attrs();
  generate_random_ipv6_entries_attrs();

  setup();

  test_perf_single();
  switch_store::object_info_clean();
  setup();
  test_perf_multi();
  test_ids_perf();

  test_mac_scale();
  printf("\n\nAll tests passed!\n");
  return 0;
}
