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


#ifndef __UTILS_H__
#define __UTILS_H__

#include <vector>
#include <mutex>

#include "bf_switch/bf_switch.h"

// s3 specific includes
#include "s3/switch_store.h"
#include "s3/attribute_util.h"
#include "s3/bf_rt_backend.h"
#include "s3/event.h"
#include "s3/factory.h"
#include "../../s3/log.h"
#include "../../s3/id_gen.h"
#include "../../s3/switch_lpm_int.h"

// switch specific includes
#include "bf_rt_ids.h"
#include "model.h"
#include "p4_16_types.h"
#include "common/utils.h"

namespace smi {

#define CHECK_PORT_IN_SWITCH_PIPE_AND_RETURN(port_handle)                     \
  do {                                                                        \
    if (feature::is_feature_set(SWITCH_FEATURE_ASYMMETRIC_FOLDED_PIPELINE) || \
        feature::is_feature_set(SWITCH_FEATURE_FOLDED_SWITCH_PIPELINE)) {     \
      const auto _ot = switch_store::object_type_query(port_handle);          \
      if (_ot == SWITCH_OBJECT_TYPE_PORT) {                                   \
        uint16_t _dp{};                                                       \
        switch_enum_t _port_type{SWITCH_PORT_ATTR_TYPE_NORMAL};               \
        switch_status_t _status =                                             \
            switch_store::v_get(port_handle, SWITCH_PORT_ATTR_DEV_PORT, _dp); \
        status |= switch_store::v_get(                                        \
            port_handle, SWITCH_PORT_ATTR_TYPE, _port_type);                  \
        if (_status != SWITCH_STATUS_SUCCESS) {                               \
          return;                                                             \
        }                                                                     \
        if (_port_type.enumdata == SWITCH_PORT_ATTR_TYPE_NORMAL) {            \
          if (!check_port_in_switch_pipe(_dp)) {                              \
            clear_attrs();                                                    \
            return;                                                           \
          }                                                                   \
        }                                                                     \
      }                                                                       \
    }                                                                         \
  } while (0)

/*
 * HACK:
 * Need to allocate port lag index instead of shitfing.
 */
#define PORT_LAG_INDEX_WIDTH 9
#define PORT_LAG_INDEX_MASK 0x3FF

// MLAG peer link port_lag_index
#define MLAG_PEER_LINK_PORT_LAG_INDEX 0x200

#define TOFINO_MGID_WIDTH 15
#define TOFINO_PRE_NODE_WIDTH 31
#define TOFINO_RID_WIDTH 15

// Max port queues
#define TOFINO1_MAX_PORT_QUEUES 8
#define TOFINO2_MAX_PORT_QUEUES 16
switch_status_t reevaluate_tunnel_nexthops();

switch_status_t get_parent_of_bd(switch_object_id_t bd_handle,
                                 switch_object_id_t &vlan_handle,
                                 switch_object_id_t &bridge_handle,
                                 switch_object_id_t &rif_handle,
                                 switch_object_id_t &vrf_handle);
switch_status_t get_bd_from_bd_member(switch_object_id_t handle,
                                      switch_object_id_t &bd_handle);
switch_status_t get_bd_member_from_port_bd_to_vlan(
    switch_object_id_t port_bd_to_vlan_handle,
    switch_object_id_t &bd_member_handle);

switch_status_t get_bd_for_object(switch_object_id_t oid,
                                  switch_object_id_t &bd_handle);
std::vector<switch_object_id_t> get_untagged_vlan_member_ports(
    switch_object_id_t vlan_handle);

uint16_t compute_pre_mgid(switch_object_id_t oid);
uint16_t compute_rid(switch_object_id_t oid);

typedef enum switch_prefix_type_ {
  IPV4_HOST,
  IPV4_LOCAL_HOST,
  IPV4_LPM,
  IPV6_HOST,
  IPV6_HOST64,
  IPV6_LPM,
  IPV6_LPM64,
  IPV6_LPM_TCAM,
  IP_LPM64,
} switch_prefix_type_t;

typedef enum switch_nexthop_index_type_ {
  SWITCH_NEXTHOP_INDEX_TYPE_NEXTHOP = 0,
  SWITCH_NEXTHOP_INDEX_TYPE_GROUP = 1,
} switch_nexthop_index_type_t;

// dir ? ingress : egress
bf_rt_target_t compute_dev_target_for_table(uint16_t dev_port,
                                            bf_rt_id_t table_id,
                                            bool dir);
uint16_t compute_nexthop_index(const switch_object_id_t handle);
uint16_t compute_vrf(const switch_object_id_t vrf_handle);
switch_acl_label_t compute_bind_label(const switch_object_id_t handle,
                                      const switch_object_id_t acl_handle);
switch_status_t compute_acl_group_label(
    const switch_object_id_t acl_group_member, bool, bool);
switch_status_t compute_acl_table_label(const switch_object_id_t acl_table);
switch_status_t release_acl_table_label(const switch_object_id_t acl_table);
switch_status_t acl_entry_compute_port_group(
    const switch_object_id_t acl_entry);
switch_status_t release_acl_table_port_group(
    const switch_object_id_t acl_table);
switch_status_t update_acl_port_group_index(
    const switch_object_id_t acl_group_member, uint64_t acl_type, uint64_t dir);

switch_status_t compute_inout_ports_label(std::set<attr_w> &attrs);
switch_status_t release_inout_ports_label(
    const switch_object_id_t acl_entry_handle);
switch_status_t update_inout_ports_label(
    const switch_object_id_t acl_entry_handle);
switch_status_t set_bind_point_attach_flag(const switch_object_id_t acl_handle,
                                           bool attach_flag);
switch_status_t update_bind_point_flag(const switch_object_id_t acl_handle,
                                       const attr_w &attr);

switch_status_t compute_outer_nexthop(switch_object_id_t nexthop_handle,
                                      switch_object_id_t &outer_nexthop_handle);

switch_status_t mac_aging_callback_register(switch_object_id_t device_handle);
switch_status_t mac_learn_callback_register(switch_object_id_t device_handle);
switch_status_t nat_aging_callback_register(switch_object_id_t device_handle);

switch_status_t get_acl_table_type_dir(switch_object_id_t acl_entry,
                                       switch_enum_t &table_type,
                                       switch_enum_t &dir);

switch_status_t pfc_wd_on_acl_to_port_bound(
    const switch_object_id_t port_handle, const switch_object_id_t acl_handle);
switch_status_t pfc_wd_on_acl_to_port_unbound(
    const switch_object_id_t port_handle, uint8_t direction);
switch_status_t pfc_wd_create_for_acl_entry(
    const switch_object_id_t acl_entry_handle);
switch_status_t pfc_wd_remove_by_acl_entry(
    const switch_object_id_t acl_entry_handle);
switch_status_t pfc_wd_on_acl_group_member_create(
    const switch_object_id_t acl_group_member_handle);
switch_status_t pfc_wd_on_acl_group_member_remove(
    const switch_object_id_t acl_group_member_handle);

switch_status_t etraps_create(const switch_object_id_t acl_entry_handle);
switch_status_t etraps_remove(const switch_object_id_t acl_entry_handle);
switch_status_t etraps_update(const switch_object_id_t acl_entry_handle,
                              const attr_w &attr);
switch_status_t etrap_meters_create(const switch_object_id_t acl_entry_handle);
switch_status_t etrap_meters_remove(const switch_object_id_t acl_entry_handle);

uint64_t switch_meter_bytes_to_kbps(uint64_t bytes);

switch_status_t vrf_ip_options_action_to_switch_packet_action(
    switch_enum_t vrf_ip_options_action, switch_packet_action_t &packet_action);

switch_status_t vrf_ttl_action_to_switch_packet_action(
    switch_enum_t vrf_ttl_action,
    switch_packet_action_t &packet_action,
    bool &is_valid);

switch_status_t vrf_unknown_l3_mcast_action_to_switch_packet_action(
    switch_enum_t vrf_unknown_l3_mcast_action,
    switch_packet_action_t &packet_action);

uint32_t port_mirror_metadata_len_get(bool is_direction_egress);
uint8_t compute_sflow_id(switch_object_id_t port_handle);

switch_status_t device_initialize();
switch_status_t tables_init();
switch_status_t validation_init();
switch_status_t validation_fp_init();
switch_status_t port_init();
switch_status_t packet_path_init();
switch_status_t l2_init();
switch_status_t l3_init();
switch_status_t rmac_init();
switch_status_t nexthop_init();
switch_status_t rewrite_init();
switch_status_t acl_init();
switch_status_t afp_init();
switch_status_t fp_init();

switch_status_t hostif_trap_init();
switch_status_t pd_init();
switch_status_t triggers_init();
switch_status_t multicast_init();
switch_status_t mirror_init();
switch_status_t qos_init();
switch_status_t meter_init();
switch_status_t wred_init();
switch_status_t tunnel_init();
switch_status_t dtel_init();
switch_status_t sflow_init();
switch_status_t pfc_wd_init();
switch_status_t hash_initialize();
switch_status_t etrap_init();
switch_status_t nat_init();
switch_status_t mpls_init();
switch_status_t bfd_init();

switch_status_t triggers_clean();
switch_status_t hostif_trap_clean();
switch_status_t tunnel_clean();
switch_status_t wred_clean();
switch_status_t meter_clean();
switch_status_t qos_clean();
switch_status_t mirror_clean();
switch_status_t multicast_clean();
switch_status_t fp_clean();
switch_status_t afp_clean();
switch_status_t acl_clean();
switch_status_t rewrite_clean();
switch_status_t nexthop_clean();
switch_status_t rmac_clean();
switch_status_t l3_clean();
switch_status_t l2_clean();
switch_status_t packet_path_clean();
switch_status_t port_clean();
switch_status_t validation_fp_clean();
switch_status_t validation_clean();
switch_status_t tables_clean();
switch_status_t device_clean();
switch_status_t dtel_clean();
switch_status_t sflow_clean();
switch_status_t pfc_wd_clean();
switch_status_t hash_clean();
switch_status_t etrap_clean();
switch_status_t bfd_clean();
bool is_nexthop_resolution_feature_on(switch_object_id_t dev_hdl);
switch_status_t port_drop_reason_to_switch_counter(
    switch_drop_reason_t drop_reason, uint16_t *counter_id);
switch_status_t table_info_get(switch_device_attr_table table_id,
                               switch_table_info_t &table_info);

void sai_mode_set(bool set);
bool sai_mode();

}  // namespace smi

#endif  // __UTILS_H__
