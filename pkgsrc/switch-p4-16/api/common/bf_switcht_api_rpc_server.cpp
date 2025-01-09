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


#include <iostream>
#include <stdio.h>
#include <mutex>  // NOLINT(build/c++11

#include "bf_switcht_api_rpc.h"

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/processor/TMultiplexedProcessor.h>

#include "arpa/inet.h"

#include <time.h>
#include <cassert>
#include "bf_switch/bf_switch.h"
#include "bf_switch/bf_event.h"
#include "s3/attribute_util.h"
#include "s3/bf_rt_backend.h"
#include "test/test_api_perf.h"
#include "model.h"
#include <dlfcn.h>

#include <target-sys/bf_sal/bf_sys_intf.h>

//#define __CPU_PROFILER__
#ifdef __CPU_PROFILER__
#include "gperftools/profiler.h"
#endif

#define SWITCH_API_RPC_SERVER_PORT (9091)

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using namespace ::bf_switcht_api;

using namespace ::smi;
using namespace ::bf_switch;
using namespace ::smi::bf_rt;

std::shared_ptr<TMultiplexedProcessor> processor(new TMultiplexedProcessor());
typedef void *pvoid_dl_t __attribute__((__may_alias__));

bool bf_switcht_api::switcht_attribute_t::operator<(
    const switcht_attribute_t &rhs) const {
  return this->id < rhs.id;
}

void parse_string(const std::string &str, switch_string_t &text) {
  memcpy(text.text, str.c_str(), sizeof(text.text));
}

void convert_attr_thrift_to_api(const switcht_attribute_t &switcht_attr,
                                attr_w &api_attr) {
  switch_attr_id_t id = static_cast<switch_attr_id_t>(switcht_attr.id);
  api_attr.id_set(id);

  bool BOOL = switcht_attr.value.BOOL;
  uint8_t UINT8 = switcht_attr.value.UINT8;
  uint16_t UINT16 = switcht_attr.value.UINT16;
  uint32_t UINT32 = switcht_attr.value.UINT32;
  uint64_t UINT64 = switcht_attr.value.UINT64;
  int64_t INT64 = switcht_attr.value.INT64;
  switch_attr_type_t type = SWITCH_TYPE_NONE;
  switch_enum_t ENUM = {.enumdata =
                            static_cast<uint64_t>(switcht_attr.value.ENUM)};
  switch_object_id_t OBJECT_ID = {
      .data = static_cast<uint64_t>(switcht_attr.value.OBJECT_ID)};
  switch_range_t RANGE = {
      .min = static_cast<uint32_t>(switcht_attr.value.RANGE.min),
      .max = static_cast<uint32_t>(switcht_attr.value.RANGE.max)};

  /* thrift doesn't have unsigned types, do integral conversion */
  switch (switcht_attr.value.type) {
    case switcht_value_type::BOOL:
      api_attr.v_set(BOOL);
      break;
    case switcht_value_type::UINT8:
      api_attr.v_set(UINT8);
      break;
    case switcht_value_type::UINT16:
      api_attr.v_set(UINT16);
      break;
    case switcht_value_type::UINT32:
      api_attr.v_set(UINT32);
      break;
    case switcht_value_type::UINT64:
      api_attr.v_set(UINT64);
      break;
    case switcht_value_type::INT64:
      api_attr.v_set(INT64);
      break;
    case switcht_value_type::ENUM:
      api_attr.v_set(ENUM);
      break;
    case switcht_value_type::OBJECT_ID:
      api_attr.v_set(OBJECT_ID);
      break;
    case switcht_value_type::MAC: {
      switch_mac_addr_t MAC = {0};
      attr_util::parse_mac(switcht_attr.value.MAC, MAC);
      api_attr.v_set(MAC);
    } break;
    case switcht_value_type::STRING: {
      switch_string_t STRING = {0};
      parse_string(switcht_attr.value.STRING, STRING);
      api_attr.v_set(STRING);
    } break;
    case switcht_value_type::IP_ADDRESS: {
      switch_ip_address_t IP_ADDRESS = {};
      attr_util::parse_ip_address(switcht_attr.value.IP_ADDRESS, IP_ADDRESS);
      api_attr.v_set(IP_ADDRESS);
    } break;
    case switcht_value_type::IP_PREFIX: {
      switch_ip_prefix_t IP_PREFIX = {};
      attr_util::parse_ip_prefix(switcht_attr.value.IP_PREFIX, IP_PREFIX);
      api_attr.v_set(IP_PREFIX);
    } break;
    case switcht_value_type::LIST: {
      switch_ip_address_t IP_ADDRESS = {};
      switch_object_id_t oid = {};
      if (switcht_attr.value.LIST.empty()) {
        // this is a hack until the thrift file is fixed for lists
        std::vector<switch_object_id_t> oid_list;
        api_attr.v_set(oid_list);
        break;
      }
      // for (auto &val : switcht_attr.value.LIST) {
      std::vector<switch_object_id_t> oid_list;
      std::vector<switch_ip_address_t> ip_list;
      std::vector<switch_enum_t> enum_list;
      std::vector<uint32_t> u32_list;
      auto val = switcht_attr.value.LIST.begin();
      if (val->type == switcht_value_type::IP_ADDRESS) {
        type = SWITCH_TYPE_IP_ADDRESS;
      } else if (val->type == switcht_value_type::UINT32) {
        type = SWITCH_TYPE_UINT32;
      } else if (val->type == switcht_value_type::ENUM) {
        type = SWITCH_TYPE_ENUM;
      } else {
        type = SWITCH_TYPE_OBJECT_ID;
      }
      for (; val != switcht_attr.value.LIST.end(); ++val) {
        if (type == SWITCH_TYPE_IP_ADDRESS) {
          attr_util::parse_ip_address(val->ip_addr, IP_ADDRESS);
          ip_list.push_back(IP_ADDRESS);
        } else if (type == SWITCH_TYPE_UINT32) {
          u32_list.push_back(val->u32data);
        } else if (type == SWITCH_TYPE_ENUM) {
          switch_enum_t enumdata = {.enumdata =
                                        static_cast<uint64_t>(val->enumdata)};
          enum_list.push_back(enumdata);
        } else {
          oid.data = static_cast<uint64_t>(val->oid);
          oid_list.push_back(oid);
        }
      }
      if (type == SWITCH_TYPE_IP_ADDRESS) {
        api_attr.v_set(ip_list);
      } else if (type == SWITCH_TYPE_UINT32) {
        api_attr.v_set(u32_list);
      } else if (type == SWITCH_TYPE_ENUM) {
        api_attr.v_set(enum_list);
      } else {
        api_attr.v_set(oid_list);
      }
    } break;
    case switcht_value_type::RANGE:
      api_attr.v_set(RANGE);
      break;
    default:
      assert(0);
  }
}

void convert_attr_api_to_thrift(const attr_w &api_attr,
                                switcht_attribute_t &switcht_attr) {
  switch_attr_id_t id = api_attr.id_get();
  switcht_attr.id = id;

  switch_attr_type_t type = api_attr.type_get();

  switch (type) {
    case SWITCH_TYPE_BOOL: {
      bool BOOL;
      switcht_attr.value.type = switcht_value_type::BOOL;
      api_attr.v_get(BOOL);
      switcht_attr.value.BOOL = BOOL;
    } break;
    case SWITCH_TYPE_UINT8: {
      uint8_t UINT8;
      switcht_attr.value.type = switcht_value_type::UINT8;
      api_attr.v_get(UINT8);
      switcht_attr.value.UINT8 = UINT8;
    } break;
    case SWITCH_TYPE_UINT16: {
      uint16_t UINT16;
      switcht_attr.value.type = switcht_value_type::UINT16;
      api_attr.v_get(UINT16);
      switcht_attr.value.UINT16 = UINT16;
    } break;
    case SWITCH_TYPE_UINT32: {
      uint32_t UINT32;
      switcht_attr.value.type = switcht_value_type::UINT32;
      api_attr.v_get(UINT32);
      switcht_attr.value.UINT32 = UINT32;
    } break;
    case SWITCH_TYPE_UINT64: {
      uint64_t UINT64;
      switcht_attr.value.type = switcht_value_type::UINT64;
      api_attr.v_get(UINT64);
      switcht_attr.value.UINT64 = UINT64;
    } break;
    case SWITCH_TYPE_INT64: {
      int64_t INT64;
      switcht_attr.value.type = switcht_value_type::INT64;
      api_attr.v_get(INT64);
      // thrift has no signed types, cast to unsigned
      switcht_attr.value.INT64 = static_cast<uint64_t>(INT64);
    } break;
    case SWITCH_TYPE_ENUM: {
      switcht_attr.value.type = switcht_value_type::ENUM;
      switch_enum_t e;
      api_attr.v_get(e);
      switcht_attr.value.ENUM = static_cast<int64_t>(e.enumdata);
    } break;
    case SWITCH_TYPE_MAC: {
      switcht_attr.value.type = switcht_value_type::MAC;
      switch_mac_addr_t mac;
      api_attr.v_get(mac);
      switcht_attr.value.MAC = (char *)mac.mac;
    } break;
    case SWITCH_TYPE_STRING: {
      switcht_attr.value.type = switcht_value_type::STRING;
      switch_string_t text;
      api_attr.v_get(text);
      switcht_attr.value.STRING = (char *)text.text;
    } break;
    case SWITCH_TYPE_OBJECT_ID: {
      switcht_attr.value.type = switcht_value_type::OBJECT_ID;
      switch_object_id_t oid;
      api_attr.v_get(oid);
      switcht_attr.value.OBJECT_ID = static_cast<switcht_object_id_t>(oid.data);
    } break;
    case SWITCH_TYPE_IP_ADDRESS: {
      switcht_attr.value.type = switcht_value_type::IP_ADDRESS;
      switch_ip_address_t ip_addr;
      api_attr.v_get(ip_addr);
    } break;
    case SWITCH_TYPE_IP_PREFIX: {
      switcht_attr.value.type = switcht_value_type::IP_PREFIX;
      switch_ip_prefix_t ip_prefix;
      api_attr.v_get(ip_prefix);
    } break;
    case SWITCH_TYPE_LIST: {
      switcht_attr.value.type = switcht_value_type::LIST;
      type = api_attr.list_type_get();
      if (type == SWITCH_TYPE_IP_ADDRESS) {
        std::vector<switch_ip_address_t> ip_addrs;
        api_attr.v_get(ip_addrs);
        for (auto ip_addr : ip_addrs) {
          (void)ip_addr;
          // TODO: Construct ip string for ip address and add to list
          // switcht_attr.value.LIST.IP_LIST.push_back(ip_addr);
        }
      } else if (type == SWITCH_TYPE_OBJECT_ID) {
        std::vector<switch_object_id_t> oids;
        api_attr.v_get(oids);
        for (auto oid : oids) {
          switcht_list_val_t list_val = {};
          list_val.type = switcht_value_type::OBJECT_ID;
          list_val.oid = static_cast<uint64_t>(oid.data);
          switcht_attr.value.LIST.push_back(list_val);
        }
      } else if (type == SWITCH_TYPE_UINT8) {
        std::vector<uint8_t> data_list;
        api_attr.v_get(data_list);
        for (auto data : data_list) {
          switcht_list_val_t list_val = {};
          list_val.type = switcht_value_type::UINT8;
          list_val.u8data = data;
          switcht_attr.value.LIST.push_back(list_val);
        }
      }
    } break;
    case SWITCH_TYPE_RANGE: {
      switcht_attr.value.type = switcht_value_type::RANGE;
      switch_range_t RANGE;
      api_attr.v_get(RANGE);
      switcht_attr.value.RANGE.max = RANGE.max;
      switcht_attr.value.RANGE.min = RANGE.min;
    } break;
    default:
      return;
  }
}

std::mutex one_packet_lock;
switch_packet_event_data_t one_packet;
void thrift_packet_rx_cb(const switch_packet_event_data_t data) {
  one_packet_lock.lock();
  one_packet.pkt = (char *)malloc(data.pkt_size);
  if (one_packet.pkt) memcpy(one_packet.pkt, data.pkt, data.pkt_size);
  one_packet.pkt_size = data.pkt_size;
  one_packet.port_handle = data.port_handle;
  one_packet_lock.unlock();
}

class bf_switcht_api_rpcHandler : virtual public ::bf_switcht_api_rpcIf {
 public:
  bf_switcht_api_rpcHandler() {}

  void object_get(switcht_return_data_t &_return,
                  const switcht_object_type_t switcht_object_type,
                  const std::set<switcht_attribute_t> &switcht_attrs) {
    const switch_object_type_t object_type =
        static_cast<switch_object_type_t>(switcht_object_type);
    switch_object_id_t object_id = {0};
    std::set<attr_w> attrs;

    for (const auto &switcht_attr : switcht_attrs) {
      attr_w attr(0);
      convert_attr_thrift_to_api(switcht_attr, attr);
      attrs.insert(attr);
    }

    switch_status_t status =
        bf_switch_object_get(object_type, attrs, object_id);
    _return.status = status;
    _return.object_id = static_cast<switcht_object_id_t>(object_id.data);
  }
  void object_create(switcht_return_data_t &_return,
                     const switcht_object_type_t switcht_object_type,
                     const std::set<switcht_attribute_t> &switcht_attrs) {
    const switch_object_type_t object_type =
        static_cast<switch_object_type_t>(switcht_object_type);
    switch_object_id_t object_id = {0};
    std::set<attr_w> attrs;

    for (const auto &switcht_attr : switcht_attrs) {
      attr_w attr(0);
      convert_attr_thrift_to_api(switcht_attr, attr);
      attrs.insert(attr);
    }

    switch_status_t status =
        bf_switch_object_create(object_type, attrs, object_id);
    _return.status = status;
    _return.object_id = static_cast<switcht_object_id_t>(object_id.data);
  }
  void object_delete(switcht_return_data_t &_return,
                     const switcht_object_id_t switcht_object_id) {
    switch_object_id_t object_id = {
        .data = static_cast<uint64_t>(switcht_object_id)};

    switch_status_t status = bf_switch_object_delete(object_id);
    _return.status = status;
  }

  void attribute_set(switcht_return_data_t &_return,
                     const switcht_object_id_t switcht_object_id,
                     const switcht_attribute_t &switcht_attr) {
    attr_w attr(0);
    convert_attr_thrift_to_api(switcht_attr, attr);
    switch_object_id_t object_id = {
        .data = static_cast<uint64_t>(switcht_object_id)};
    switch_status_t status = bf_switch_attribute_set(object_id, attr);
    _return.status = status;
  }
  void attribute_get(switcht_return_data_t &_return,
                     const switcht_object_id_t switcht_object_id,
                     const switcht_attr_id_t attr_id) {
    switcht_attribute_t switcht_attr;
    attr_w attr(0);
    switch_object_id_t object_id = {
        .data = static_cast<uint64_t>(switcht_object_id)};
    switch_attr_id_t attr_api_id = static_cast<uint64_t>(attr_id);
    switch_status_t status =
        bf_switch_attribute_get(object_id, attr_api_id, attr);
    convert_attr_api_to_thrift(attr, _return.attr);
    _return.status = status;
  }
  void object_get_all_handles(std::vector<switcht_object_id_t> &_objects,
                              const switcht_object_type_t switcht_object_type) {
    const switch_object_type_t object_type =
        static_cast<switch_object_type_t>(switcht_object_type);
    std::vector<switch_object_id_t> object_handles;
    bf_switch_get_all_handles(object_type, object_handles);
    for (auto handle : object_handles) {
      _objects.push_back(static_cast<switcht_object_id_t>(handle.data));
    }
  }
  void object_flush_all(switcht_return_data_t &_return,
                        const switcht_object_type_t switcht_object_type) {
    const switch_object_type_t object_type =
        static_cast<switch_object_type_t>(switcht_object_type);
    switch_status_t status = bf_switch_object_flush_all(object_type);
    _return.status = status;
  }
  void object_counters_get(std::vector<switcht_counter_t> &_counters,
                           const switcht_object_id_t switcht_object_id) {
    switcht_counter_t _cntr;
    std::vector<switch_counter_t> cntrs;
    switch_object_id_t object_id = {
        .data = static_cast<uint64_t>(switcht_object_id)};
    bf_switch_counters_get(object_id, cntrs);
    for (const auto cntr : cntrs) {
      _cntr.counter_id = cntr.counter_id;
      _cntr.count = cntr.count;
      _counters.push_back(_cntr);
    }
  }

  void object_counters_clear(switcht_return_data_t &_return,
                             const switcht_object_id_t switcht_object_id,
                             const std::vector<int16_t> &cntr_ids) {
    switch_object_id_t object_id = {
        .data = static_cast<uint64_t>(switcht_object_id)};
    std::vector<uint16_t> _cntr_ids;
    for (auto id : cntr_ids) {
      _cntr_ids.push_back(static_cast<uint16_t>(id));
    }
    switch_status_t status = bf_switch_counters_clear(object_id, _cntr_ids);
    _return.status = status;
  }

  void object_counters_clear_all(switcht_return_data_t &_return,
                                 const switcht_object_id_t switcht_object_id) {
    switch_object_id_t object_id = {
        .data = static_cast<uint64_t>(switcht_object_id)};
    switch_status_t status = bf_switch_counters_clear_all(object_id);
    _return.status = status;
  }

  int32_t is_feature_enable(const switcht_feature_id_t switcht_feature) {
    switch_feature_id_t feature = (switch_feature_id_t)switcht_feature;
    if (bf_switch_is_feature_enabled(feature) == true) {
      return 1;
    }
    return 0;
  }

  void table_info_get(switcht_table_info_t &_info, int64_t table_id) {
    switch_table_info_t table_info;
    bf_switch_table_info_get(table_id, table_info);
    _info.size = static_cast<int32_t>(table_info.table_size);
    _info.usage = static_cast<int32_t>(table_info.table_usage);
    return;
  }

  void fill_table(switcht_return_data_t &_return,
                  const switcht_object_id_t device_id,
                  const switcht_perf_table::type table_type,
                  int32_t num_entries) {
    switch_object_id_t device = {.data = static_cast<uint64_t>(device_id)};

    ApiPerf api_perf(device, switcht_perf_api::BMAI, table_type);
    api_perf.run(num_entries, true);

    _return.status = SWITCH_STATUS_SUCCESS;
  }

  void port_list_get(std::vector<switcht_object_id_t> &_ports,
                     const switcht_object_id_t device_id) {
    switch_object_id_t device_handle = {.data =
                                            static_cast<uint64_t>(device_id)};
    // Get port handle list
    switch_object_id_t first_handle = {0};
    std::vector<switch_object_id_t> handles_list;
    attr_w num_ports_attr(0);
    uint32_t num_ports = 0;
    bf_switch_attribute_get(
        device_handle, SWITCH_DEVICE_ATTR_NUM_PORTS, num_ports_attr);
    num_ports_attr.v_get(num_ports);
    bf_switch_get_first_handle(SWITCH_OBJECT_TYPE_PORT, first_handle);
    bf_switch_get_next_handles(first_handle, 512, handles_list, num_ports);

    for (const auto &handle : handles_list) {
      switch_enum_t type = {};
      attr_w port_type(SWITCH_PORT_ATTR_TYPE);
      bf_switch_attribute_get(handle, SWITCH_PORT_ATTR_TYPE, port_type);
      port_type.v_get(type);
      if (type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL) continue;
      // For Asymmetric folded pipeline skip internal ports
      if (is_feature_enable(SWITCH_FEATURE_ASYMMETRIC_FOLDED_PIPELINE) ||
          is_feature_enable(SWITCH_FEATURE_FOLDED_SWITCH_PIPELINE)) {
        attr_w internal_port_list(SWITCH_PORT_ATTR_INTERNAL_PIPE_PORT_LIST);
        bf_switch_attribute_get(handle,
                                SWITCH_PORT_ATTR_INTERNAL_PIPE_PORT_LIST,
                                internal_port_list);
        std::vector<switch_object_id_t> internal_ports;
        internal_port_list.v_get(internal_ports);
        // Internal ports do not have their own internal ports
        if (internal_ports.empty()) continue;
      }
      _ports.push_back(static_cast<switcht_object_id_t>(handle.data));
    }
  }

  bool thrift_ports_added = false;
  int32_t thrift_ports_present() { return thrift_ports_added; }
  int32_t add_thrift_ports() {
    thrift_ports_added = true;
    return 0;
  }
  int32_t bf_switcht_clean(const bool warm_shut,
                           const std::string &warm_shut_file) {
    bf_switch_clean(0, warm_shut, warm_shut_file.c_str());
    return 0;
  }
  void get_last_packet(switcht_packet_event_data_t &_return) {
    one_packet_lock.lock();
    if (one_packet.pkt_size) {
      _return.valid = true;
      // _return.pkt = std::string(one_packet.pkt);
      _return.pkt_size = one_packet.pkt_size;
      _return.port_handle = one_packet.port_handle.data;
    }
    one_packet.pkt_size = 0;
    if (one_packet.pkt) {
      free(one_packet.pkt);
      one_packet.pkt = NULL;
    }
    one_packet_lock.unlock();
  }
  void packet_rx_cb_register(switcht_return_data_t &_return,
                             const bool enable) {
    if (enable) {
      bf_switch_event_register(SWITCH_PACKET_EVENT,
                               (void *)&thrift_packet_rx_cb);
    } else {
      bf_switch_event_deregister(SWITCH_PACKET_EVENT);
    }
    _return.status = 0;
  }
  void switch_api_perf_test(switcht_return_data_t &_return,
                            const switcht_object_id_t device_id,
                            const switcht_perf_api::type api_type,
                            const switcht_perf_table::type table_type,
                            const bool batch,
                            const int32_t entries) {
    switch_object_id_t device = {.data = static_cast<uint64_t>(device_id)};

    ApiPerf api_perf(device, api_type, table_type);
    PerfResult perf = api_perf.run(entries, batch);
    api_perf.clean();

    _return.object_id = static_cast<switcht_object_id_t>(perf.get_rate());
    _return.status = perf.get_status();
  }

  void nat_perf_batch_test(switcht_return_data_t &_return,
                           const switcht_object_id_t dev_id,
                           int32_t num_entries) {
    switch_ip_address_t dest_ip_key;
    switch_ip_address_t dest_ip_data;

    dest_ip_key.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
    dest_ip_key.ip4 = 167772161;

    dest_ip_data.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
    dest_ip_data.ip4 = 2684354561;

    switch_object_id_t device_handle = {.data = static_cast<uint64_t>(dev_id)};
    switch_object_id_t nat_handle = {};

    uint16_t dport_base = 100;
    uint16_t dport_key;
    uint8_t proto = 6;

    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t nat_type = {.enumdata = static_cast<int64_t>(
                                  SWITCH_NAT_ENTRY_ATTR_TYPE_DESTINATION_NAPT)};
    std::vector<switch_object_id_t> nat_handles;
#ifdef __CPU_PROFILER__
    ProfilerStart("./perf_data.txt");
    ProfilerFlush();
#endif
    time_t now;
    time(&now);
    printf("NAT entry batch bulk begin time: %s", ctime(&now));
    smi::bf_rt::start_batch();
    for (int32_t i = 0; i < num_entries; i++) {
      std::set<attr_w> nat_attrs;
      dport_key = dport_base + i;
      nat_attrs.insert(attr_w(SWITCH_NAT_ENTRY_ATTR_DEVICE, device_handle));
      nat_attrs.insert(attr_w(SWITCH_NAT_ENTRY_ATTR_DST_IP_KEY, dest_ip_key));
      nat_attrs.insert(attr_w(SWITCH_NAT_ENTRY_ATTR_IP_PROTO_KEY, proto));
      nat_attrs.insert(
          attr_w(SWITCH_NAT_ENTRY_ATTR_L4_DST_PORT_KEY, dport_key));
      nat_attrs.insert(attr_w(SWITCH_NAT_ENTRY_ATTR_NAT_DST_IP, dest_ip_data));
      nat_attrs.insert(
          attr_w(SWITCH_NAT_ENTRY_ATTR_NAT_L4_DST_PORT, dport_key));
      nat_attrs.insert(attr_w(SWITCH_NAT_ENTRY_ATTR_TYPE, nat_type));
      nat_attrs.insert(
          attr_w(SWITCH_NAT_ENTRY_ATTR_L4_SRC_PORT_KEY, dport_key));
      nat_attrs.insert(attr_w(SWITCH_NAT_ENTRY_ATTR_SRC_IP_KEY, dest_ip_key));
      status = bf_switch_object_create(
          SWITCH_OBJECT_TYPE_NAT_ENTRY, nat_attrs, nat_handle);
      nat_handles.push_back(nat_handle);
      _return.status = status;
    }
    time(&now);
    printf("NAT entry batch bulk end time: %s", ctime(&now));
    smi::bf_rt::end_batch();
#ifdef __CPU_PROFILER__
    ProfilerStop();
#endif
    for (auto handle : nat_handles) {
      bf_switch_object_delete(handle);
    }
  }

  void nat_perf_test(switcht_return_data_t &_return,
                     const switcht_object_id_t dev_id,
                     int32_t num_entries) {
    switch_ip_address_t dest_ip_key;
    switch_ip_address_t dest_ip_data;

    dest_ip_key.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
    dest_ip_key.ip4 = 167772161;

    dest_ip_data.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
    dest_ip_data.ip4 = 2684354561;

    switch_object_id_t device_handle = {.data = static_cast<uint64_t>(dev_id)};
    switch_object_id_t nat_handle = {};

    uint16_t dport_base = 100;
    uint16_t dport_key;
    uint8_t proto = 6;

    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_enum_t nat_type = {.enumdata = static_cast<int64_t>(
                                  SWITCH_NAT_ENTRY_ATTR_TYPE_DESTINATION_NAPT)};
    std::vector<switch_object_id_t> nat_handles;
#ifdef __CPU_PROFILER__
    ProfilerStart("./perf_data.txt");
    ProfilerFlush();
#endif
    time_t now;
    time(&now);
    printf("NAT entry bulk begin time: %s", ctime(&now));
    for (int32_t i = 0; i < num_entries; i++) {
      std::set<attr_w> nat_attrs;
      dport_key = dport_base + i;
      nat_attrs.insert(attr_w(SWITCH_NAT_ENTRY_ATTR_DEVICE, device_handle));
      nat_attrs.insert(attr_w(SWITCH_NAT_ENTRY_ATTR_DST_IP_KEY, dest_ip_key));
      nat_attrs.insert(attr_w(SWITCH_NAT_ENTRY_ATTR_IP_PROTO_KEY, proto));
      nat_attrs.insert(
          attr_w(SWITCH_NAT_ENTRY_ATTR_L4_DST_PORT_KEY, dport_key));
      nat_attrs.insert(attr_w(SWITCH_NAT_ENTRY_ATTR_NAT_DST_IP, dest_ip_data));
      nat_attrs.insert(
          attr_w(SWITCH_NAT_ENTRY_ATTR_NAT_L4_DST_PORT, dport_key));
      nat_attrs.insert(attr_w(SWITCH_NAT_ENTRY_ATTR_TYPE, nat_type));
      nat_attrs.insert(
          attr_w(SWITCH_NAT_ENTRY_ATTR_L4_SRC_PORT_KEY, dport_key));
      nat_attrs.insert(attr_w(SWITCH_NAT_ENTRY_ATTR_SRC_IP_KEY, dest_ip_key));
      status = bf_switch_object_create(
          SWITCH_OBJECT_TYPE_NAT_ENTRY, nat_attrs, nat_handle);
      nat_handles.push_back(nat_handle);
      _return.status = status;
    }
    time(&now);
    printf("NAT entry bulk end time: %s", ctime(&now));
#ifdef __CPU_PROFILER__
    ProfilerStop();
#endif
    for (auto handle : nat_handles) {
      bf_switch_object_delete(handle);
    }
  }

  void exit() { std::exit(0); }
};

TSimpleServer *server_local = NULL;
static void *api_rpc_server_thread(void *args) {
  (void)args;
  int port = SWITCH_API_RPC_SERVER_PORT;
  std::shared_ptr<bf_switcht_api_rpcHandler> bf_switcht_api_rpc_handler(
      new bf_switcht_api_rpcHandler());
#ifdef SAL_API
  char *error;
  int (*sal_fn_register)(void);
  void *handle = NULL;
#endif

  processor->registerProcessor(
      "bf_switcht_api_rpc",
      std::shared_ptr<TProcessor>(
          new bf_switcht_api_rpcProcessor(bf_switcht_api_rpc_handler)));

#ifdef SAL_API
  handle = dlopen("libbfsal.so", RTLD_LAZY | RTLD_GLOBAL);
  if ((error = dlerror()) != NULL) {
    printf("ERROR:%s:%d: dlopen failed for %s, err=%s\n",
           __func__,
           __LINE__,
           "libbfsal",
           error);
    return NULL;
  }

  *(pvoid_dl_t *)(&sal_fn_register) = dlsym(handle, "add_sal_to_rpc_server");
  if (sal_fn_register) sal_fn_register();
#endif

  std::shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
  std::shared_ptr<TTransportFactory> transportFactory(
      new TBufferedTransportFactory());
  std::shared_ptr<TProtocolFactory> protocolFactory(
      new TBinaryProtocolFactory());

  server_local = new TSimpleServer(
      processor, serverTransport, transportFactory, protocolFactory);
  server_local->serve();
  return NULL;
}

static pthread_t api_rpc_thread;

extern "C" int start_bf_switcht_api_rpc_server(void);
int start_bf_switcht_api_rpc_server(void) {
  int retval = 0;
  std::cerr << "Starting API RPC server on port " << SWITCH_API_RPC_SERVER_PORT
            << std::endl;

  retval = pthread_create(&api_rpc_thread, NULL, api_rpc_server_thread, NULL);
  if (retval) return retval;
  pthread_setname_np(api_rpc_thread, "bf_switcht_api_thrift");

  bf_switch_init_packet_driver();

  return retval;
}

extern "C" int stop_switch_api_rpc_server(void);
int stop_switch_api_rpc_server(void) {
  server_local->stop();
  pthread_join(api_rpc_thread, NULL);
  std::cerr << "Stopping API RPC server on port " << SWITCH_API_RPC_SERVER_PORT
            << std::endl;
  return 0;
}
