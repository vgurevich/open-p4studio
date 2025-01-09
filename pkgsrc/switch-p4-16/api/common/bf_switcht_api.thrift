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


/*
        switcht API thrift file
*/

namespace py bf_switcht_api
namespace cpp bf_switcht_api

typedef byte switcht_device_t
typedef i32 switcht_status_t
typedef i64 switcht_object_type_t
typedef i64 switcht_object_id_t
typedef i64 switcht_attr_id_t
typedef i32 switcht_feature_id_t

struct switcht_range_t {
  1 : i32 min
  2 : i32 max
}

/*
	thrift unions are broken..
	__isset, is not set in the generated cpp code
	using struct instead
*/
enum switcht_value_type {
  BOOL,
  UINT8,
  UINT16,
  UINT32,
  UINT64,
  INT64,
  ENUM,
  OBJECT_ID,
  MAC,
  IP_ADDRESS,
  IP_PREFIX,
  STRING,
  LIST,
  RANGE
}

enum switcht_perf_api {
  BMAI,
  BFRT
}

enum switcht_perf_table {
  MAC_ENTRY,
  ACL_IPV4,
  HOST_ROUTE_IPV4,
  LPM_ROUTE_IPV4,
  HOST_ROUTE_IPV6,
  LPM_ROUTE_IPV6,
  NEXTHOP,
  VLAN,
  MCAST_MEMBER
}

struct switcht_list_val_t {
  1: switcht_value_type type
  2: switcht_object_id_t oid
  3: string ip_addr
  4: i32 u32data
  5: i64 enumdata
  6: byte u8data
}

struct switcht_value_t {
	1:  switcht_value_type type
	2:  bool BOOL
	3:  byte UINT8
	4:  i16 UINT16
	5:  i32 UINT32
	6:  i64 UINT64
	7:  i64 INT64
	8:  i64 ENUM
	9:  switcht_object_id_t OBJECT_ID
	10: string MAC
	11: string IP_ADDRESS
	12: string IP_PREFIX
	13: string STRING
	14: list<switcht_list_val_t> LIST
	15: switcht_range_t RANGE
}

struct switcht_attribute_t {
	1: required switcht_attr_id_t id
	2: required switcht_value_t value
}

struct switcht_counter_t {
  1: required i16 counter_id
  2: required i64 count
}

struct switcht_table_info_t {
  1: required i32 size
  2: required i32 usage
}

struct switcht_return_data_t {
	1: required switcht_status_t status
	2: required switcht_object_id_t object_id
	3: required switcht_attribute_t attr
}

struct switcht_packet_event_data_t {
	1: required string pkt
	2: required i64 pkt_size
	3: required switcht_object_id_t port_handle
	4: required bool valid
}

service bf_switcht_api_rpc {

  switcht_return_data_t object_get(1: switcht_object_type_t object_type,
                                   2: set<switcht_attribute_t> attrs);

  switcht_return_data_t object_create(1: switcht_object_type_t object_type,
                                      2: set<switcht_attribute_t> attrs);

	switcht_return_data_t object_delete(1: switcht_object_id_t object_id);

	switcht_return_data_t attribute_set(1: switcht_object_id_t object_id,
                                      2: switcht_attribute_t attr);

	switcht_return_data_t attribute_get(1: switcht_object_id_t object_id,
                                      2: switcht_attr_id_t attr_id);

  list<switcht_object_id_t> object_get_all_handles(1: switcht_object_type_t object_type);

  list<switcht_counter_t> object_counters_get(1: switcht_object_id_t object_id);

  switcht_return_data_t object_counters_clear(1: switcht_object_id_t object_id,
                                              2: list<i16> cntr_ids);

  switcht_return_data_t object_counters_clear_all(1: switcht_object_id_t object_id);

  switcht_return_data_t object_flush_all(1: switcht_object_type_t object_type);

  switcht_return_data_t packet_rx_cb_register(1: bool enable);
  switcht_packet_event_data_t get_last_packet();

  switcht_return_data_t switch_api_perf_test(1 : switcht_object_id_t device_id,
                                             2 : switcht_perf_api api,
                                             3 : switcht_perf_table table,
                                             4 : bool batch,
                                             5 : i32 entries)

  switcht_return_data_t nat_perf_batch_test(1 : switcht_object_id_t device_id,
                                        2 : i32 num_entries);

  switcht_return_data_t nat_perf_test(1 : switcht_object_id_t device_id,
                                        2 : i32 num_entries);

  i32 is_feature_enable(1: switcht_feature_id_t switcht_feature);

  switcht_table_info_t table_info_get(1: i64 table_id);
  switcht_return_data_t fill_table(1 : switcht_object_id_t device_id,
                                   2: switcht_perf_table table,
                                   3: i32 num_entries);

  list<switcht_object_id_t> port_list_get(1: switcht_object_id_t device_id);
  i32 thrift_ports_present();
  i32 add_thrift_ports();

  i32 bf_switcht_clean(1: bool warm_shut,
                       2: string warm_shut_file);

  void exit();
}
