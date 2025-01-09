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


#include <utility>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <set>

extern "C" {
#include "target-sys/bf_sal/bf_sys_timer.h"
#include "pipe_mgr/pipe_mgr_intf.h"
#include "tofino/bf_pal/pltfm_intf.h"
#include "tofino/bf_pal/bf_pal_port_intf.h"
#include "tofino/pdfixed/pd_tm.h"
#include "lld/bf_lld_if.h"
#include "dvm/bf_drv_intf.h"
}

#include "switch_tna/utils.h"
#include "switch_tna/p4_16_types.h"
#include "common/hostif.h"
#include "common/bfrt_tm.h"
#define UNUSED(x) (void)x;
namespace smi {
using namespace smi::bf_rt;  // NOLINT(build/namespaces)
using ::smi::logging::switch_log;

using ::bfrt::BfRtSession;

static unsigned bloom_filter_index = 0;
static uint32_t port_autoneg_interval = 5 * 1000;

void device_stats_timer_cb(bf_sys_timer_t *timer, void *data) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  bool sw_model = false;
  bool is_warm_init = false;
  uint16_t dev_id = 0;

  if (!data) return;
  (void)timer;

  dev_id = *(static_cast<uint16_t *>(data));

  status = bf_pal_pltfm_type_get(dev_id, &sw_model);
  if (sw_model) return;

  bf_device_warm_init_in_progress(dev_id, &is_warm_init);
  if (is_warm_init) return;

  // Disable stats sync during WR/FR
  if (switch_store::smiContext::context().in_warm_init()) return;

  if (switch_store::smiContext::context().is_stats_timer_off()) {
    switch_log(SWITCH_API_LEVEL_DEBUG,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: MAU stats polling is off until warm reboot counter "
               "restore process is complete",
               __func__,
               __LINE__);
    return;
  }

  std::shared_ptr<BfRtSession> stats_session = BfRtSession::sessionCreate();
  std::vector<bf_rt_table_id_t> sync_sym_tables = {
      smi_id::T_PRE_INGRESS_ACL,
      smi_id::T_INGRESS_MAC_ACL,
      smi_id::T_INGRESS_IP_ACL,
      smi_id::T_INGRESS_IPV4_ACL,
      smi_id::T_INGRESS_IPV6_ACL,
      smi_id::T_INGRESS_IP_DTEL_ACL,
      smi_id::T_INGRESS_IP_MIRROR_ACL,
      smi_id::T_EGRESS_MAC_ACL,
      smi_id::T_EGRESS_IPV4_ACL,
      smi_id::T_EGRESS_IPV6_ACL,
      smi_id::T_EGRESS_IPV4_MIRROR_ACL,
      smi_id::T_EGRESS_IPV6_MIRROR_ACL,
      smi_id::T_INGRESS_BD_STATS,
      smi_id::T_EGRESS_BD_STATS,
      smi_id::T_COPP,
      smi_id::T_EGRESS_COPP,
      smi_id::T_INGRESS_MIRROR_METER_ACTION,
      smi_id::T_EGRESS_MIRROR_METER_ACTION,
      smi_id::T_INGRESS_PORT_METER_ACTION,
      smi_id::T_EGRESS_PORT_METER_ACTION,
      smi_id::T_INGRESS_ACL_METER_ACTION,
      smi_id::T_INGRESS_IP_QOS_ACL_METER_ACTION,
      smi_id::T_INGRESS_IP_MIRROR_ACL_METER_ACTION,
      smi_id::T_EGRESS_ACL_METER_ACTION,
      smi_id::T_EGRESS_IP_QOS_ACL_METER_ACTION,
      smi_id::T_EGRESS_IP_MIRROR_ACL_METER_ACTION,
      smi_id::T_IPV4_MULTICAST_ROUTE_S_G,
      // smi_id::T_IPV4_MULTICAST_ROUTE_X_G,
      smi_id::T_INGRESS_NAT_DEST_NAPT,
      smi_id::T_INGRESS_NAT_DEST_NAT,
      smi_id::T_INGRESS_NAT_DEST_NAT_POOL,
      smi_id::T_INGRESS_NAT_FLOW_NAPT,
      smi_id::T_INGRESS_NAT_FLOW_NAT,
      smi_id::T_INGRESS_NAT_SNAT,
      smi_id::T_INGRESS_NAT_SNAPT,
      smi_id::T_MPLS_FIB,
      smi_id::T_MY_SID,
      smi_id::T_SID_REWRITE,
      smi_id::T_INGRESS_TOS_MIRROR_ACL,
      smi_id::T_EGRESS_TOS_MIRROR_ACL};
  for (auto table_id : sync_sym_tables) {
    _Table table(get_dev_tgt(), get_bf_rt_info(), table_id, stats_session);
    status = table.do_hw_stats_sync();
    if (status != SWITCH_STATUS_SUCCESS) {
      std::string table_name;
      table.name_get(table_name);
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_NONE,
                 "{}.{}: status:{} failed hw sync for sym table {}",
                 __func__,
                 __LINE__,
                 status,
                 table_name);
    }
  }

  std::vector<bf_rt_table_id_t> sync_ingress_asym_tables = {
      smi_id::T_PPG,
      smi_id::T_INGRESS_DROP_STATS,
      smi_id::T_STORM_CONTROL_STATS,
      smi_id::T_INGRESS_PFC_WD_ACL,
      smi_id::T_INGRESS_PORT_IP_STATS};
  for (auto table_id : sync_ingress_asym_tables) {
    _Table table(get_dev_tgt(), get_bf_rt_info(), table_id, stats_session);
    status = table.do_hw_stats_sync();
    if (status != SWITCH_STATUS_SUCCESS) {
      std::string table_name;
      table.name_get(table_name);
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_NONE,
                 "{}.{}: status:{} failed hw sync for ingress asym table {}",
                 __func__,
                 __LINE__,
                 status,
                 table_name);
    }
  }

  std::vector<bf_rt_table_id_t> sync_egress_asym_tables = {
      smi_id::T_QUEUE,
      smi_id::T_EGRESS_WRED_STATS,
      smi_id::T_EGRESS_DROP_STATS,
      smi_id::T_EGRESS_PFC_WD_ACL,
      smi_id::T_EGRESS_PORT_IP_STATS};
  if (feature::is_feature_set(SWITCH_FEATURE_EGRESS_SYSTEM_ACL_STATS)) {
    sync_egress_asym_tables.push_back(smi_id::T_EGRESS_SYSTEM_ACL);
  }
  for (auto table_id : sync_egress_asym_tables) {
    _Table table(get_dev_tgt(), get_bf_rt_info(), table_id, stats_session);
    status = table.do_hw_stats_sync();
    if (status != SWITCH_STATUS_SUCCESS) {
      std::string table_name;
      table.name_get(table_name);
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_NONE,
                 "{}.{}: status:{} failed hw sync for egress asym table {}",
                 __func__,
                 __LINE__,
                 status,
                 table_name);
    }
  }
  return;
}

void device_port_rate_timer_cb(bf_sys_timer_t *timer, void *data) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  bool sw_model = false;
  uint16_t dev_id = 0;
  switch_object_id_t device_handle = {0};

  if (!data) return;
  (void)timer;

  dev_id = *(static_cast<uint16_t *>(data));
  status = bf_pal_pltfm_type_get(dev_id, &sw_model);
  if (sw_model) return;

  // Disable stats sync during WR/FR
  if (switch_store::smiContext::context().in_warm_init()) return;

  if (switch_store::smiContext::context().is_stats_timer_off()) {
    switch_log(
        SWITCH_API_LEVEL_DEBUG,
        SWITCH_OBJECT_TYPE_NONE,
        "{}.{}: Port rate stats polling is off until warm reboot counter "
        "restore process is complete",
        __func__,
        __LINE__);
    return;
  }

  // Get the device handle
  std::set<attr_w> attrs_dev;
  attrs_dev.insert(attr_w(SWITCH_DEVICE_ATTR_DEV_ID, dev_id));
  status = switch_store::object_id_get_wkey(
      SWITCH_OBJECT_TYPE_DEVICE, attrs_dev, device_handle);

  // Get the configured ports for the given device
  std::set<switch_object_id_t> port_handles;
  status |= switch_store::referencing_set_get(
      device_handle, SWITCH_OBJECT_TYPE_PORT, port_handles);

  uint32_t max_ports = 0;
  status |= switch_store::v_get(
      device_handle, SWITCH_DEVICE_ATTR_MAX_PORTS, max_ports);

  // Query the RMON counter directly from HW
  uint16_t dev_port = 0;
  uint64_t port_id = 0;
  switch_enum_t port_type = {0};
  uint64_t rmon_counters[BF_PORT_RATE_MAX_COUNTERS] = {0};

  bf_rmon_counter_t direct_rmon_counter[BF_PORT_RATE_MAX_COUNTERS] = {
      bf_mac_stat_OctetsReceived,
      bf_mac_stat_FramesReceivedAll,
      bf_mac_stat_OctetsTransmittedTotal,
      bf_mac_stat_FramesTransmittedAll};

  for (auto const &port_handle : port_handles) {
    port_type = {0};
    // Skipping internal ports
    status |=
        switch_store::v_get(port_handle, SWITCH_PORT_ATTR_TYPE, port_type);
    if ((status == SWITCH_STATUS_SUCCESS) &&
        (port_type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL)) {
      continue;
    }

    dev_port = 0;
    port_id = 0;
    status |=
        switch_store::v_get(port_handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
    status |=
        switch_store::v_get(port_handle, SWITCH_PORT_ATTR_PORT_ID, port_id);
    if (status != SWITCH_STATUS_SUCCESS) {
      continue;
    }

    memset(rmon_counters, 0, sizeof(rmon_counters));
    if (!sw_model) {
      bool is_added = false;
      status = bf_pal_is_port_added(dev_id, dev_port, &is_added);
      if (status != SWITCH_STATUS_SUCCESS || !is_added) {
        continue;
      }

      status = bf_pal_port_stat_direct_get(dev_id,
                                           dev_port,
                                           direct_rmon_counter,
                                           rmon_counters,
                                           BF_PORT_RATE_MAX_COUNTERS);

      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_PORT,
                   "{}.{}: status:{} failed to get port rate stats from "
                   "hw  for dev: {}, port-id {}",
                   __func__,
                   __LINE__,
                   status,
                   dev_id,
                   port_id);
        continue;
      }
    }

    if (port_id < max_ports) {
      SWITCH_CONTEXT.port_rate_update(dev_id, port_id, rmon_counters);
    }
  }
  return;
}

void device_port_autoneg_timer_cb(bf_sys_timer_t *timer, void *data) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  bool sw_model = false;
  uint16_t dev_id = 0;
  switch_object_id_t device_handle = {0};

  if (!data) return;
  (void)timer;

  dev_id = *(static_cast<uint16_t *>(data));
  status = bf_pal_pltfm_type_get(dev_id, &sw_model);
  if (sw_model) return;

  // Disable port autoneg processing during WR/FR
  if (switch_store::smiContext::context().in_warm_init()) return;

  // Get the device handle
  std::set<attr_w> attrs_dev;
  attrs_dev.insert(attr_w(SWITCH_DEVICE_ATTR_DEV_ID, dev_id));
  status = switch_store::object_id_get_wkey(
      SWITCH_OBJECT_TYPE_DEVICE, attrs_dev, device_handle);

  // Get the configured ports for the given device
  std::set<switch_object_id_t> port_handles;
  status |= switch_store::referencing_set_get(
      device_handle, SWITCH_OBJECT_TYPE_PORT, port_handles);

  uint16_t dev_port = 0;
  uint64_t port_id = 0;
  switch_enum_t port_type = {0};

  for (auto const &port_handle : port_handles) {
    port_type = {0};
    // Skipping internal ports
    status |=
        switch_store::v_get(port_handle, SWITCH_PORT_ATTR_TYPE, port_type);
    if ((status == SWITCH_STATUS_SUCCESS) &&
        (port_type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL)) {
      continue;
    }

    dev_port = 0;
    port_id = 0;
    status |=
        switch_store::v_get(port_handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
    status |=
        switch_store::v_get(port_handle, SWITCH_PORT_ATTR_PORT_ID, port_id);
    if (status != SWITCH_STATUS_SUCCESS) {
      continue;
    }
    // Skip ports in admin down state
    bool admin_state = false;

    status = switch_store::v_get(
        port_handle, SWITCH_PORT_ATTR_ADMIN_STATE, admin_state);
    if (status != SWITCH_STATUS_SUCCESS) continue;
    if (!admin_state) continue;

    switch_enum_t _an = {0};
    switch_port_attr_autoneg autoneg;
    status = switch_store::v_get(port_handle, SWITCH_PORT_ATTR_AUTONEG, _an);
    if (status != SWITCH_STATUS_SUCCESS) continue;
    autoneg = static_cast<switch_port_attr_autoneg>(_an.enumdata);
    if (autoneg != SWITCH_PORT_ATTR_AUTONEG_ENABLED) continue;

    // if (switch_store::object_try_lock(port_handle) == 0) continue;
    switch_store::switch_store_lock();

    // Make sure AN is still enabled
    status = switch_store::v_get(port_handle, SWITCH_PORT_ATTR_AUTONEG, _an);
    if (status == SWITCH_STATUS_SUCCESS) {
      autoneg = static_cast<switch_port_attr_autoneg>(_an.enumdata);
      if (autoneg == SWITCH_PORT_ATTR_AUTONEG_ENABLED) {
        update_port_an_state(port_handle, port_id, dev_id, dev_port);
      }
    }

    // switch_store::object_unlock(port_handle);
    switch_store::switch_store_unlock();
  }
}

void report_state_clear_timer_cb(bf_sys_timer_t *timer, void *data) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  size_t table_size = 0;
  bf_rt_table_id_t table_id = smi_id::T_EGRESS_DROP_REPORT_BLOOM_FILTER_1;

  if (!data || !timer) return;
  (void)data;

  std::set<bf_rt_table_id_t> bloom_filters;

  if (smi_id::T_EGRESS_DROP_REPORT_BLOOM_FILTER_1) {
    bloom_filters.insert(smi_id::T_EGRESS_DROP_REPORT_BLOOM_FILTER_1);
  }
  if (smi_id::T_EGRESS_FLOW_REPORT_BLOOM_FILTER_1) {
    bloom_filters.insert(smi_id::T_EGRESS_FLOW_REPORT_BLOOM_FILTER_1);
  }
  if (smi_id::T_EGRESS_DROP_REPORT_BLOOM_FILTER_2) {
    bloom_filters.insert(smi_id::T_EGRESS_DROP_REPORT_BLOOM_FILTER_2);
  }
  if (smi_id::T_EGRESS_FLOW_REPORT_BLOOM_FILTER_2) {
    bloom_filters.insert(smi_id::T_EGRESS_FLOW_REPORT_BLOOM_FILTER_2);
  }

  if (bloom_filters.empty()) return;
  if (bloom_filter_index >= bloom_filters.size()) bloom_filter_index = 0;

  table_id = *std::next(bloom_filters.begin(), bloom_filter_index);

  auto bloom_filter_clear_session = bfrt::BfRtSession::sessionCreate();
  _Table table(
      get_dev_tgt(), get_bf_rt_info(), table_id, bloom_filter_clear_session);
  status |= table.clear_entries();
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "{}:{}: failed to clear {} bloom filter status = {} ",
               __func__,
               __LINE__,
               table_id,
               status);
    return;
  }

  bloom_filter_clear_session->sessionDestroy();
  switch_log(SWITCH_API_LEVEL_DEBUG,
             SWITCH_OBJECT_TYPE_NONE,
             "{}:{}: bloom filter entries({}) cleared for table:{}",
             __func__,
             __LINE__,
             table_size,
             table_id);
  bloom_filter_index++;
  return;
}

// Helper class to hold timer context
class device_timer {
 private:
  bf_sys_timer_t timer = {};
  uint16_t _dev_id;
  uint32_t _refresh_interval;
  bf_sys_timeout_cb _cb_fn;

 public:
  device_timer(uint32_t refresh_interval,
               uint16_t dev_id,
               bf_sys_timeout_cb cb_fn)
      : _dev_id(dev_id), _refresh_interval(refresh_interval), _cb_fn(cb_fn) {}
  switch_status_t timer_create() {
    return bf_sys_timer_create(
        &timer, 0x0, _refresh_interval, _cb_fn, static_cast<void *>(&_dev_id));
  }
  switch_status_t timer_start() { return bf_sys_timer_start(&timer); }
  switch_status_t timer_stop() { return bf_sys_timer_stop(&timer); }
  switch_status_t timer_delete() { return bf_sys_timer_del(&timer); }
  bool timer_valid() { return (_refresh_interval ? true : false); }
  void timer_set(uint32_t val) { _refresh_interval = val; }
};
std::unique_ptr<device_timer> counter_timer = nullptr;
std::unique_ptr<device_timer> port_rate_timer = nullptr;
std::unique_ptr<device_timer> report_state_clear_timer = nullptr;
std::unique_ptr<device_timer> port_autoneg_timer = nullptr;

switch_status_t set_vlan_aging_cb(switch_object_id_t vlan_handle,
                                  switch_attribute_t attr) {
  return switch_store::v_set(
      vlan_handle, SWITCH_VLAN_ATTR_AGING_INTERVAL, attr.value.u32);
}

switch_status_t set_mac_aging_cb(switch_object_id_t mac_handle,
                                 switch_attribute_t attr) {
  (void)attr;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  // if the MAC object invalid, the lock API would return 0
  if (switch_store::object_try_lock(mac_handle) == 0) return status;
  std::unique_ptr<object> smac_obj(factory::get_instance().create(
      SWITCH_OBJECT_TYPE_SMAC, mac_handle, status));
  if (smac_obj != nullptr) {
    status = smac_obj->create_update();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_MAC_ENTRY,
                 "{}: smac update failed status={} for mac {}",
                 __func__,
                 status,
                 mac_handle);
    }
  }
  switch_store::object_unlock(mac_handle);
  return status;
}

class device_aging : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_DEVICE_AGING;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DEVICE_AGING_ATTR_PARENT_HANDLE;

 public:
  device_aging(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    uint32_t aging_interval = 0;
    switch_attribute_t attr = {};
    status |= switch_store::v_get(
        parent, SWITCH_DEVICE_ATTR_DEFAULT_AGING_INTERVAL, aging_interval);

    // Update the vlan aging interval attribute for all vlans
    attr.value.u32 = aging_interval;
    status |=
        execute_cb_for_all(SWITCH_OBJECT_TYPE_VLAN, set_vlan_aging_cb, attr);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(
          SWITCH_API_LEVEL_ERROR,
          SWITCH_OBJECT_TYPE_DEVICE,
          "{}: aging interval update failed status={} for object_type {}",
          __func__,
          status,
          SWITCH_OBJECT_TYPE_VLAN);
    }

    // Now re-evaluate all SMAC objects so that the TTL is updated
    status |= execute_cb_for_all(
        SWITCH_OBJECT_TYPE_MAC_ENTRY, set_mac_aging_cb, attr);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(
          SWITCH_API_LEVEL_ERROR,
          SWITCH_OBJECT_TYPE_DEVICE,
          "{}: aging interval update failed status={} for object_type {}",
          __func__,
          status,
          SWITCH_OBJECT_TYPE_MAC_ENTRY);
    }
  }
};

class device_refresh_interval : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DEVICE_REFRESH_INTERVAL;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DEVICE_REFRESH_INTERVAL_ATTR_PARENT_HANDLE;

 public:
  device_refresh_interval(const switch_object_id_t parent,
                          switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    uint32_t refresh_interval = 0;
    bool sw_model = false;
    uint16_t dev_id = 0;

    status |= switch_store::v_get(parent, SWITCH_DEVICE_ATTR_DEV_ID, dev_id);
    status |= switch_store::v_get(
        parent, SWITCH_DEVICE_ATTR_REFRESH_INTERVAL, refresh_interval);

    status = bf_pal_pltfm_type_get(dev_id, &sw_model);
    if (!sw_model) {
      if (counter_timer->timer_valid()) {
        status = counter_timer->timer_stop();
        if (status != SWITCH_STATUS_SUCCESS) {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_DEVICE,
                     "stop timer fail {}",
                     status);
        }
        status = counter_timer->timer_delete();
        if (status != SWITCH_STATUS_SUCCESS) {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_DEVICE,
                     "del timer fail {}",
                     status);
        }
      }

      if (refresh_interval) {
        // if invoked during warm_init, just reset the flag to false
        if (switch_store::smiContext::context().in_warm_init()) return;

        counter_timer->timer_set(refresh_interval * 1000);
        status = counter_timer->timer_create();
        if (status != SWITCH_STATUS_SUCCESS) {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_DEVICE,
                     "create timer fail {}",
                     status);
        } else {
          counter_timer->timer_start();
        }
      }
    }
  }
};

class device_warm_reboot : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DEVICE_WARM_REBOOT;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DEVICE_WARM_REBOOT_ATTR_PARENT_HANDLE;

 public:
  device_warm_reboot(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    bool warm_shut = false;
    uint16_t dev_id = 0;
    uint16_t flow_state_clear_cycle = 0;
    uint32_t flow_state_clear_cycle_ms = 0;
    uint32_t refresh_interval = 0;
    uint32_t port_rate_interval = 0;
    status |=
        switch_store::v_get(parent, SWITCH_DEVICE_ATTR_WARM_SHUT, warm_shut);
    status |= switch_store::v_get(parent,
                                  SWITCH_DEVICE_ATTR_FLOW_STATE_CLEAR_CYCLE,
                                  flow_state_clear_cycle);
    flow_state_clear_cycle_ms = flow_state_clear_cycle * 500;
    status |= switch_store::v_get(
        parent, SWITCH_DEVICE_ATTR_REFRESH_INTERVAL, refresh_interval);
    status |= switch_store::v_get(
        parent, SWITCH_DEVICE_ATTR_PORT_RATE_INTERVAL, port_rate_interval);

    // if invoked during warm_init, return
    if (switch_store::smiContext::context().in_warm_init()) return;

    bool sw_model = false;
    status = bf_pal_pltfm_type_get(dev_id, &sw_model);
    if (sw_model) return;

    if (warm_shut) {
      /*
      if (counter_timer->timer_valid()) {
        counter_timer->timer_set(0);
        status = counter_timer->timer_stop();
        if (status != SWITCH_STATUS_SUCCESS) {
          switch_log(
              SWITCH_API_LEVEL_ERROR, "counter stop timer fail {}", status);
        } else {
          status = counter_timer->timer_delete();
          if (status != SWITCH_STATUS_SUCCESS) {
            switch_log(
                SWITCH_API_LEVEL_ERROR, "counter del timer fail {}", status);
          }
        }
      }
      if (report_state_clear_timer->timer_valid()) {
        report_state_clear_timer->timer_set(0);
        status = report_state_clear_timer->timer_stop();
        if (status != SWITCH_STATUS_SUCCESS) {
          switch_log(
              SWITCH_API_LEVEL_ERROR, "report stop timer fail {}", status);
        } else {
          status = report_state_clear_timer->timer_delete();
          if (status != SWITCH_STATUS_SUCCESS) {
            switch_log(
                SWITCH_API_LEVEL_ERROR, "report del timer fail {}", status);
          }
        }
      }
      */
    } else {
      if (refresh_interval) {
        counter_timer->timer_set(refresh_interval * 1000);
        status = counter_timer->timer_create();
        if (status != SWITCH_STATUS_SUCCESS) {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_DEVICE,
                     "{}.{}: create timer fail {}",
                     __func__,
                     __LINE__,
                     status);
        } else {
          counter_timer->timer_start();
          switch_log(SWITCH_API_LEVEL_WARN,
                     SWITCH_OBJECT_TYPE_DEVICE,
                     "{}.{}: Turn on MAU stats polling - warm reboot counter "
                     "restore process is complete",
                     __func__,
                     __LINE__);
        }
      }
      if (flow_state_clear_cycle_ms >= 1000) {
        report_state_clear_timer->timer_set(flow_state_clear_cycle_ms);
        status = report_state_clear_timer->timer_create();
        if (status != SWITCH_STATUS_SUCCESS) {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_DEVICE,
                     "create timer fail {}",
                     status);
        } else {
          report_state_clear_timer->timer_start();
        }
      } else {
        switch_log(
            SWITCH_API_LEVEL_INFO,
            SWITCH_OBJECT_TYPE_DEVICE,
            "{}: dtel bloom filter timer start skipped, current "
            "supported minimum timer precision is 2sec, User value: {}sec",
            __func__,
            flow_state_clear_cycle);
      }
      if (port_rate_interval && bf_lld_dev_is_tof1(0)) {
        port_rate_timer->timer_set(port_rate_interval);
        status = port_rate_timer->timer_create();
        if (status != SWITCH_STATUS_SUCCESS) {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_DEVICE,
                     "port rate timer create fail {}",
                     status);
        } else {
          port_rate_timer->timer_start();
          switch_log(SWITCH_API_LEVEL_WARN,
                     SWITCH_OBJECT_TYPE_DEVICE,
                     "{}.{}: Turn on port rate timer - warm reboot counter "
                     "restore process is complete",
                     __func__,
                     __LINE__);
        }
      }
      if (port_autoneg_interval) {
        port_autoneg_timer->timer_set(port_autoneg_interval);
        status = port_autoneg_timer->timer_create();
        if (status != SWITCH_STATUS_SUCCESS) {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_DEVICE,
                     "port autoneg timer create fail {}",
                     status);
        } else {
          port_autoneg_timer->timer_start();
          switch_log(SWITCH_API_LEVEL_WARN,
                     SWITCH_OBJECT_TYPE_DEVICE,
                     "{}.{}: Turn on port autoneg timer - warm reboot counter "
                     "restore process is complete",
                     __func__,
                     __LINE__);
        }
      }
    }
  }
};

class device_dtel_bloom_filters_clear_cycle : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DEVICE_DTEL_BLOOM_FILTERS_CLEAR_CYCLE;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DEVICE_DTEL_BLOOM_FILTERS_CLEAR_CYCLE_ATTR_PARENT_HANDLE;

 public:
  device_dtel_bloom_filters_clear_cycle(const switch_object_id_t parent,
                                        switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    uint16_t flow_state_clear_cycle = 0;
    uint32_t flow_state_clear_cycle_ms = 0;
    bool sw_model = false;
    uint16_t dev_id = 0;

    status |= switch_store::v_get(parent, SWITCH_DEVICE_ATTR_DEV_ID, dev_id);
    status |= switch_store::v_get(parent,
                                  SWITCH_DEVICE_ATTR_FLOW_STATE_CLEAR_CYCLE,
                                  flow_state_clear_cycle);

    flow_state_clear_cycle_ms = flow_state_clear_cycle * 500;

    if (report_state_clear_timer == nullptr) return;

    status = bf_pal_pltfm_type_get(dev_id, &sw_model);
    if (!sw_model) {
      if (report_state_clear_timer->timer_valid()) {
        status = report_state_clear_timer->timer_stop();
        if (status != SWITCH_STATUS_SUCCESS) {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_DEVICE,
                     "stop timer fail {}",
                     status);
        }
        status = report_state_clear_timer->timer_delete();
        if (status != SWITCH_STATUS_SUCCESS) {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_DEVICE,
                     "del timer fail {}",
                     status);
        }
      }

      // if invoked during warm_init, just reset the flag to false
      if (switch_store::smiContext::context().in_warm_init()) return;

      report_state_clear_timer->timer_set(flow_state_clear_cycle_ms);
      if (flow_state_clear_cycle_ms >= 1000) {
        status = report_state_clear_timer->timer_create();
        if (status != SWITCH_STATUS_SUCCESS) {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_DEVICE,
                     "create timer fail {}",
                     status);
        } else {
          report_state_clear_timer->timer_start();
        }
      } else {
        switch_log(
            SWITCH_API_LEVEL_INFO,
            SWITCH_OBJECT_TYPE_DEVICE,
            "{}: dtel bloom filter timer start skipped, current "
            "supported minimum timer precision is 2sec, User value: {}sec",
            __func__,
            flow_state_clear_cycle);
      }
    }
  }
};

class device_learn_notif_timeout : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DEVICE_LEARN_NOTIF_TIMEOUT;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DEVICE_LEARN_NOTIF_TIMEOUT_ATTR_PARENT_HANDLE;

 public:
  device_learn_notif_timeout(const switch_object_id_t parent,
                             switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    uint32_t learn_timeout = 0;
    bool sw_model = false;
    uint16_t dev_id = 0;
    pipe_status_t p_status = PIPE_SUCCESS;

    status |= switch_store::v_get(parent, SWITCH_DEVICE_ATTR_DEV_ID, dev_id);
    status |= switch_store::v_get(
        parent, SWITCH_DEVICE_ATTR_LEARN_NOTIF_TIMEOUT, learn_timeout);

    status = bf_pal_pltfm_type_get(dev_id, &sw_model);
    if (sw_model) return;

    if (status == SWITCH_STATUS_SUCCESS) {
      p_status = pipe_mgr_flow_lrn_set_timeout(
          get_bf_rt_session_ptr()->sessHandleGet(),
          dev_id,
          learn_timeout * 1000);
      if (p_status != PIPE_SUCCESS) {
        switch_log(
            SWITCH_API_LEVEL_ERROR,
            SWITCH_OBJECT_TYPE_DEVICE,
            "{}: set learn notif timeout {} failed status={} for device: {}",
            __func__,
            learn_timeout,
            status,
            dev_id);
      }
    }
  }
};

switch_status_t set_port_cut_thru_cb(switch_object_id_t port_handle,
                                     switch_attribute_t attr) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_enum_t port_type = {0};
  switch_object_id_t device_handle = {0};
  uint16_t device = 0, dev_port = 0;

  status = switch_store::v_get(port_handle, SWITCH_PORT_ATTR_TYPE, port_type);
  status =
      switch_store::v_get(port_handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
  status =
      switch_store::v_get(port_handle, SWITCH_PORT_ATTR_DEVICE, device_handle);
  status =
      switch_store::v_get(device_handle, SWITCH_DEVICE_ATTR_DEV_ID, device);

  if (port_type.enumdata == SWITCH_PORT_ATTR_TYPE_NORMAL) {
    if (attr.value.booldata) {
      status = bf_pal_port_cut_through_enable(device, dev_port);
    } else {
      status = bf_pal_port_cut_through_disable(device, dev_port);
    }
  }
  return status;
}

class device_cut_through : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DEVICE_CUT_THROUGH;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DEVICE_CUT_THROUGH_ATTR_PARENT_HANDLE;

 public:
  device_cut_through(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    bool cut_thru = false;
    switch_attribute_t attr = {};
    status |=
        switch_store::v_get(parent, SWITCH_DEVICE_ATTR_CUT_THROUGH, cut_thru);

    // Now re-evaluate all port objects and setup cut thru
    attr.value.booldata = cut_thru;
    status |=
        execute_cb_for_all(SWITCH_OBJECT_TYPE_PORT, set_port_cut_thru_cb, attr);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_DEVICE,
                 "{}: cut thru update failed status={} for object_type {}",
                 __func__,
                 status,
                 SWITCH_OBJECT_TYPE_PORT);
    }
  }
};

class device_fdb_unicast_packet_action : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DEVICE_FDB_UNICAST_PACKET_ACTION;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DEVICE_FDB_UNICAST_PACKET_ACTION_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_DEVICE_FDB_UNICAST_PACKET_ACTION_ATTR_STATUS;

 public:
  device_fdb_unicast_packet_action(const switch_object_id_t parent,
                                   switch_status_t &status)
      : p4_object_match_action(smi_id::T_INGRESS_SYSTEM_ACL,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    uint8_t pkt_type = SWITCH_PACKET_TYPE_UNICAST, pkt_type_mask = 0x3;
    uint8_t miss = 0, hit = 1;
    uint8_t action = 0;
    uint32_t priority = 0xFFFFFF;
    switch_mac_entry_attr_action fdb_action =
        SWITCH_MAC_ENTRY_ATTR_ACTION_FORWARD;

    status = switch_store::v_get(
        parent, SWITCH_DEVICE_ATTR_UNICAST_MISS_PACKET_ACTION, action);
    fdb_action = (switch_mac_entry_attr_action)action;

    status |= match_key.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_PKT_TYPE, pkt_type, pkt_type_mask);
    status |= match_key.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_RMAC_HIT, miss, hit);
    status |= match_key.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_DMAC_MISS, hit, hit);
    status |= match_key.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, priority);

    if (fdb_action == SWITCH_MAC_ENTRY_ATTR_ACTION_FORWARD) {
      action_entry.init_action_data(smi_id::A_SYSTEM_ACL_PERMIT);
    } else if (fdb_action == SWITCH_MAC_ENTRY_ATTR_ACTION_DROP) {
      action_entry.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
      action_entry.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                           SWITCH_DROP_REASON_L2_MISS_UNICAST);
    } else if (fdb_action == SWITCH_MAC_ENTRY_ATTR_ACTION_COPY) {
      action_entry.init_action_data(smi_id::A_SYSTEM_ACL_COPY_TO_CPU);
      action_entry.set_arg(
          smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_REASON_CODE,
          static_cast<uint16_t>(
              SWITCH_HOSTIF_TRAP_ATTR_TYPE_MAX +
              SWITCH_HOSTIF_TRAP_ATTR_INTERNAL_TRAP_L2_MISS_UNICAST));
    } else if (fdb_action == SWITCH_MAC_ENTRY_ATTR_ACTION_REDIRECT) {
      action_entry.init_action_data(smi_id::A_SYSTEM_ACL_REDIRECT_TO_CPU);
      action_entry.set_arg(
          smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_REASON_CODE,
          static_cast<uint16_t>(
              SWITCH_HOSTIF_TRAP_ATTR_TYPE_MAX +
              SWITCH_HOSTIF_TRAP_ATTR_INTERNAL_TRAP_L2_MISS_UNICAST));
    }
  }
};

class device_fdb_broadcast_packet_action : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DEVICE_FDB_BROADCAST_PACKET_ACTION;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DEVICE_FDB_BROADCAST_PACKET_ACTION_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_DEVICE_FDB_BROADCAST_PACKET_ACTION_ATTR_STATUS;

 public:
  device_fdb_broadcast_packet_action(const switch_object_id_t parent,
                                     switch_status_t &status)
      : p4_object_match_action(smi_id::T_INGRESS_SYSTEM_ACL,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    uint8_t pkt_type = SWITCH_PACKET_TYPE_BROADCAST, pkt_type_mask = 0x3;
    uint8_t miss = 0, hit = 1;
    uint8_t action = 0;
    uint32_t priority = 0xFFFFFF;
    switch_mac_entry_attr_action fdb_action =
        SWITCH_MAC_ENTRY_ATTR_ACTION_FORWARD;
    switch_mac_addr_t bmac = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    status = switch_store::v_get(
        parent, SWITCH_DEVICE_ATTR_BROADCAST_MISS_PACKET_ACTION, action);
    fdb_action = (switch_mac_entry_attr_action)action;

    status |= match_key.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_PKT_TYPE, pkt_type, pkt_type_mask);
    status |= match_key.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_RMAC_HIT, miss, hit);
    // status |= match_key.set_ternary(
    //     smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_DMAC_MISS, hit, hit);
    status |= match_key.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_MAC_DST_ADDR, bmac, bmac);
    status |= match_key.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, priority);

    if (fdb_action == SWITCH_MAC_ENTRY_ATTR_ACTION_FORWARD) {
      action_entry.init_action_data(smi_id::A_SYSTEM_ACL_PERMIT);
    } else if (fdb_action == SWITCH_MAC_ENTRY_ATTR_ACTION_DROP) {
      action_entry.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
      action_entry.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                           SWITCH_DROP_REASON_L2_MISS_BROADCAST);
    } else if (fdb_action == SWITCH_MAC_ENTRY_ATTR_ACTION_COPY) {
      action_entry.init_action_data(smi_id::A_SYSTEM_ACL_COPY_TO_CPU);
      action_entry.set_arg(
          smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_REASON_CODE,
          static_cast<uint16_t>(
              SWITCH_HOSTIF_TRAP_ATTR_TYPE_MAX +
              SWITCH_HOSTIF_TRAP_ATTR_INTERNAL_TRAP_L2_MISS_BROADCAST));
    } else if (fdb_action == SWITCH_MAC_ENTRY_ATTR_ACTION_REDIRECT) {
      action_entry.init_action_data(smi_id::A_SYSTEM_ACL_REDIRECT_TO_CPU);
      action_entry.set_arg(
          smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_REASON_CODE,
          static_cast<uint16_t>(
              SWITCH_HOSTIF_TRAP_ATTR_TYPE_MAX +
              SWITCH_HOSTIF_TRAP_ATTR_INTERNAL_TRAP_L2_MISS_BROADCAST));
    }
  }
};

class device_fdb_multicast_packet_action : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DEVICE_FDB_MULTICAST_PACKET_ACTION;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DEVICE_FDB_MULTICAST_PACKET_ACTION_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_DEVICE_FDB_MULTICAST_PACKET_ACTION_ATTR_STATUS;

 public:
  device_fdb_multicast_packet_action(const switch_object_id_t parent,
                                     switch_status_t &status)
      : p4_object_match_action(smi_id::T_INGRESS_SYSTEM_ACL,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    uint8_t pkt_type = SWITCH_PACKET_TYPE_MULTICAST, pkt_type_mask = 0x3;
    uint8_t miss = 0, hit = 1;
    uint8_t action = 0;
    uint32_t priority = 0xFFFFFF;
    switch_mac_entry_attr_action fdb_action =
        SWITCH_MAC_ENTRY_ATTR_ACTION_FORWARD;
    switch_mac_addr_t mcast_mac = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00};

    status = switch_store::v_get(
        parent, SWITCH_DEVICE_ATTR_MULTICAST_MISS_PACKET_ACTION, action);
    fdb_action = (switch_mac_entry_attr_action)action;

    status |= match_key.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_PKT_TYPE, pkt_type, pkt_type_mask);
    status |= match_key.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_RMAC_HIT, miss, hit);
    status |= match_key.set_ternary(
        smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_ROUTED, miss, hit);
    // status |= match_key.set_ternary(
    //     smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_DMAC_MISS, hit, hit);
    status |= match_key.set_ternary(
        smi_id::F_SYSTEM_ACL_LKP_MAC_DST_ADDR, mcast_mac, mcast_mac);
    status |= match_key.set_exact(smi_id::F_SYSTEM_ACL_PRIORITY, priority);

    if (fdb_action == SWITCH_MAC_ENTRY_ATTR_ACTION_FORWARD) {
      action_entry.init_action_data(smi_id::A_SYSTEM_ACL_PERMIT);
    } else if (fdb_action == SWITCH_MAC_ENTRY_ATTR_ACTION_DROP) {
      action_entry.init_action_data(smi_id::A_SYSTEM_ACL_DROP);
      action_entry.set_arg(smi_id::P_SYSTEM_ACL_DROP_DROP_REASON,
                           SWITCH_DROP_REASON_L2_MISS_MULTICAST);
    } else if (fdb_action == SWITCH_MAC_ENTRY_ATTR_ACTION_COPY) {
      action_entry.init_action_data(smi_id::A_SYSTEM_ACL_COPY_TO_CPU);
      action_entry.set_arg(
          smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_REASON_CODE,
          static_cast<uint16_t>(
              SWITCH_HOSTIF_TRAP_ATTR_TYPE_MAX +
              SWITCH_HOSTIF_TRAP_ATTR_INTERNAL_TRAP_L2_MISS_MULTICAST));
    } else if (fdb_action == SWITCH_MAC_ENTRY_ATTR_ACTION_REDIRECT) {
      action_entry.init_action_data(smi_id::A_SYSTEM_ACL_REDIRECT_TO_CPU);
      action_entry.set_arg(
          smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_REASON_CODE,
          static_cast<uint16_t>(
              SWITCH_HOSTIF_TRAP_ATTR_TYPE_MAX +
              SWITCH_HOSTIF_TRAP_ATTR_INTERNAL_TRAP_L2_MISS_MULTICAST));
    }
  }
};

class device_hostif : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_DEVICE_HOSTIF;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DEVICE_HOSTIF_ATTR_PARENT_HANDLE;
  switch_object_id_t cpu_port_handle = {0};
  uint16_t cpu_dev_port = {0};

 public:
  device_hostif(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status |= switch_store::v_get(
        parent, SWITCH_DEVICE_ATTR_CPU_PORT, cpu_port_handle);
    if (!cpu_port_handle.data) return;

    status |= switch_store::v_get(
        cpu_port_handle, SWITCH_PORT_ATTR_DEV_PORT, cpu_dev_port);
  }
  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    status = auto_object::create_update();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_DEVICE,
                 "{}.{}: auto_obj.create_update failure status {}",
                 __func__,
                 __LINE__,
                 status);
      return status;
    }

    if (!cpu_port_handle.data) return status;
    // Set ingress parser value set.
    bool bf_rt_status = false;
    uint16_t port_mask = 0x1FF;
    uint16_t ether_type_mask = 0xFFFF;
    bf_rt_target_t dev_tgt = {.dev_id = 0,
                              .pipe_id = BF_DEV_PIPE_ALL,
                              .direction = BF_DEV_DIR_ALL,
                              .prsr_id = 0xFF};

    _Table table(dev_tgt, get_bf_rt_info(), smi_id::T_INGRESS_CPU_PORT);
    _MatchKey match_key(smi_id::T_INGRESS_CPU_PORT);
    _ActionEntry action_entry(smi_id::T_INGRESS_CPU_PORT);

    status |= match_key.set_ternary(
        smi_id::F_INGRESS_CPU_PORT_ETHER_TYPE, ether_type_bfn, ether_type_mask);
    status |= match_key.set_ternary(
        smi_id::F_INGRESS_CPU_PORT_PORT, cpu_dev_port, port_mask);
    status |= action_entry.init_indirect_data();
    status |= table.entry_add(match_key, action_entry, bf_rt_status);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_NONE,
                 "{}.{}: failure to set egress pvs for cpu port handle {} "
                 "status {}",
                 __func__,
                 __LINE__,
                 cpu_port_handle,
                 status);
      return status;
    }

    // Set egress parser value set.
    _Table eg_table(dev_tgt, get_bf_rt_info(), smi_id::T_EGRESS_CPU_PORT);
    _MatchKey eg_match_key(smi_id::T_EGRESS_CPU_PORT);
    _ActionEntry eg_action_entry(smi_id::T_EGRESS_CPU_PORT);
    status |= eg_match_key.set_ternary(
        smi_id::F_EGRESS_CPU_PORT_PORT, cpu_dev_port, port_mask);
    status |= eg_action_entry.init_indirect_data();
    bf_rt_status = false;
    status |= eg_table.entry_add(eg_match_key, eg_action_entry, bf_rt_status);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_NONE,
                 "{}.{}: failure to set egress pvs for cpu port handle {} "
                 "status {}",
                 __func__,
                 __LINE__,
                 cpu_port_handle,
                 status);
      return status;
    }
    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (!cpu_port_handle.data) return status;
    // Set ingress parser value set.
    uint16_t port_mask = 0x1FF;
    uint16_t ether_type_mask = 0xFFFF;
    bf_rt_target_t dev_tgt = {.dev_id = 0,
                              .pipe_id = BF_DEV_PIPE_ALL,
                              .direction = BF_DEV_DIR_ALL,
                              .prsr_id = 0xFF};

    _Table table(dev_tgt, get_bf_rt_info(), smi_id::T_INGRESS_CPU_PORT);
    _MatchKey match_key(smi_id::T_INGRESS_CPU_PORT);

    status |= match_key.set_ternary(
        smi_id::F_EGRESS_CPU_PORT_ETHER_TYPE, ether_type_bfn, ether_type_mask);
    status |= match_key.set_ternary(
        smi_id::F_INGRESS_CPU_PORT_PORT, cpu_dev_port, port_mask);
    status |= table.entry_delete(match_key);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_NONE,
                 "{}.{}: failure to delete egress pvs for cpu port handle {} "
                 "status {}",
                 __func__,
                 __LINE__,
                 cpu_port_handle,
                 status);
      return status;
    }

    // Set egress parser value set.
    _Table eg_table(dev_tgt, get_bf_rt_info(), smi_id::T_EGRESS_CPU_PORT);
    _MatchKey eg_match_key(smi_id::T_EGRESS_CPU_PORT);
    status |= eg_match_key.set_ternary(
        smi_id::F_EGRESS_CPU_PORT_PORT, cpu_dev_port, port_mask);
    status |= eg_table.entry_delete(eg_match_key);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_NONE,
                 "{}.{}: failure to delete egress pvs for cpu port handle {} "
                 "status {}",
                 __func__,
                 __LINE__,
                 cpu_port_handle,
                 status);
      return status;
    }
    status |= auto_object::del();
    return status;
  }
};

class pktgen_pvs : public auto_object {
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_PKTGEN_PVS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_PKTGEN_PVS_ATTR_PARENT_HANDLE;
  /* bf_switch uses batch-id/ethertype range 0-0x01FF for pktgen
   * (overlaps with IANA "experimental" range)
   * match 7 upper ethertype bits to be zero. */
  static const uint16_t ethertype = 0x0;
  static const uint16_t ethertype_mask = 0xFE00;
  static const uint16_t dev_port_mask = 0x01ff;

  switch_enum_t port_type{};
  bf_rt_target_t dev_tgt{};
  uint16_t dev_port{};
  bf_dev_pipe_t pipe{};

 public:
  pktgen_pvs(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    switch_object_id_t device_handle{};
    uint16_t dev_id{};

    status |=
        switch_store::v_get(parent, SWITCH_PORT_ATTR_DEVICE, device_handle);
    status |=
        switch_store::v_get(device_handle, SWITCH_DEVICE_ATTR_DEV_ID, dev_id);
    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_TYPE, port_type);
    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
    pipe = (bf_dev_pipe_t)DEV_PORT_TO_PIPE(dev_port);
    dev_tgt = bf_rt_target_t{.dev_id = dev_id,
                             .pipe_id = pipe,
                             .direction = BF_DEV_DIR_INGRESS,
                             .prsr_id = 0xFF};
  }

  switch_status_t create_update() {
    bool bf_rt_status = false;
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (port_type.enumdata != SWITCH_PORT_ATTR_TYPE_RECIRC) {
      return SWITCH_STATUS_SUCCESS;
    }

    /* TODO could be relevant for asymmetric pipeline??? */
    auto pipes = _Table(smi_id::T_INGRESS_PKTGEN_PORT).get_active_pipes();
    if (pipes.find(pipe) == pipes.end()) {
      return SWITCH_STATUS_SUCCESS;
    }

    status = auto_object::create_update();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_DEVICE,
                 "{}.{}: auto_obj.create_update failure status {}",
                 __func__,
                 __LINE__,
                 status);
      return status;
    }

    _Table table(dev_tgt, get_bf_rt_info(), smi_id::T_INGRESS_PKTGEN_PORT);
    _MatchKey match_key(smi_id::T_INGRESS_PKTGEN_PORT);
    _ActionEntry action_entry(smi_id::T_INGRESS_PKTGEN_PORT);
    status |= match_key.set_ternary(
        smi_id::F_INGRESS_PKTGEN_PORT_ETHER_TYPE, ethertype, ethertype_mask);
    status |= match_key.set_ternary(
        smi_id::F_INGRESS_PKTGEN_PORT_PORT, dev_port, dev_port_mask);

    status |= action_entry.init_indirect_data();
    status |= table.entry_add(match_key, action_entry, bf_rt_status);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_NONE,
                 "{}.{}: failure to set ingress pvs for pktgen port {} "
                 "status {}",
                 __func__,
                 __LINE__,
                 dev_port,
                 status);
    }

    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    _Table table(dev_tgt, get_bf_rt_info(), smi_id::T_INGRESS_PKTGEN_PORT);
    _MatchKey match_key(smi_id::T_INGRESS_PKTGEN_PORT);
    status |= match_key.set_ternary(
        smi_id::F_INGRESS_PKTGEN_PORT_ETHER_TYPE, ethertype, ethertype_mask);
    status |= match_key.set_ternary(
        smi_id::F_INGRESS_PKTGEN_PORT_PORT, dev_port, dev_port_mask);

    status |= table.entry_delete(match_key);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(
          SWITCH_API_LEVEL_ERROR,
          SWITCH_OBJECT_TYPE_NONE,
          "{}.{}: failure to delete ingress pvs for pktgen port handle {} "
          "status {}",
          __func__,
          __LINE__,
          dev_port,
          status);
      return status;
    }

    return auto_object::del();
  }
};

class device_vxlan_port : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DEVICE_VXLAN_PORT;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DEVICE_VXLAN_PORT_ATTR_PARENT_HANDLE;

  uint16_t udp_port_vxlan = 4789, mask = 0xFFFF;
  bf_rt_target_t dev_tgt = {.dev_id = 0,
                            .pipe_id = BF_DEV_PIPE_ALL,
                            .direction = BF_DEV_DIR_INGRESS,
                            .prsr_id = 0xFF};

 public:
  device_vxlan_port(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    if (!feature::is_feature_set(SWITCH_FEATURE_VXLAN)) return;
    status = switch_store::v_get(
        parent, SWITCH_DEVICE_ATTR_UDP_PORT_VXLAN, udp_port_vxlan);
  }
  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (!feature::is_feature_set(SWITCH_FEATURE_VXLAN)) return status;

    status = auto_object::create_update();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_HASH,
                 "{}: auto_obj.create_update failure status {}",
                 __func__,
                 status);
      return status;
    }

    bool bf_rt_status = false;
    // set ingress parser
    _Table i_table(dev_tgt, get_bf_rt_info(), smi_id::T_ING_UDP_PORT_VXLAN);
    _MatchKey i_match_key(smi_id::T_ING_UDP_PORT_VXLAN);
    _ActionEntry i_action_entry(smi_id::T_ING_UDP_PORT_VXLAN);
    i_action_entry.init_indirect_data();
    // not a ternary key, just using for setValueandMask
    i_match_key.set_ternary(
        smi_id::F_ING_UDP_PORT_VXLAN_F1, udp_port_vxlan, mask);
    status |= i_table.entry_add(i_match_key, i_action_entry, bf_rt_status);

    // set egress parser
    bf_rt_status = false;
    dev_tgt.direction = BF_DEV_DIR_EGRESS;
    _Table e_table(dev_tgt, get_bf_rt_info(), smi_id::T_EG_UDP_PORT_VXLAN);
    _MatchKey e_match_key(smi_id::T_EG_UDP_PORT_VXLAN);
    _ActionEntry e_action_entry(smi_id::T_EG_UDP_PORT_VXLAN);
    e_action_entry.init_indirect_data();
    // not a ternary key, just using for setValueandMask
    e_match_key.set_ternary(
        smi_id::F_EG_UDP_PORT_VXLAN_F1, udp_port_vxlan, mask);
    status |= e_table.entry_add(e_match_key, e_action_entry, bf_rt_status);

    if (feature::is_feature_set(SWITCH_FEATURE_FOLDED_SWITCH_PIPELINE)) {
      dev_tgt.direction = BF_DEV_DIR_INGRESS;
      // set ingress parser
      _Table i1_table(dev_tgt, get_bf_rt_info(), smi_id::T_ING1_UDP_PORT_VXLAN);
      _MatchKey i1_match_key(smi_id::T_ING1_UDP_PORT_VXLAN);
      _ActionEntry i1_action_entry(smi_id::T_ING1_UDP_PORT_VXLAN);
      i1_action_entry.init_indirect_data();
      // not a ternary key, just using for setValueandMask
      i1_match_key.set_ternary(
          smi_id::F_ING1_UDP_PORT_VXLAN_F1, udp_port_vxlan, mask);
      status |= i1_table.entry_add(i1_match_key, i1_action_entry, bf_rt_status);

      // set egress parser
      bf_rt_status = false;
      dev_tgt.direction = BF_DEV_DIR_EGRESS;
      _Table e1_table(dev_tgt, get_bf_rt_info(), smi_id::T_EG1_UDP_PORT_VXLAN);
      _MatchKey e1_match_key(smi_id::T_EG1_UDP_PORT_VXLAN);
      _ActionEntry e1_action_entry(smi_id::T_EG1_UDP_PORT_VXLAN);
      e1_action_entry.init_indirect_data();
      // not a ternary key, just using for setValueandMask
      e1_match_key.set_ternary(
          smi_id::F_EG1_UDP_PORT_VXLAN_F1, udp_port_vxlan, mask);
      status |= e1_table.entry_add(e1_match_key, e1_action_entry, bf_rt_status);
    }
    return status;
  }
  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (!feature::is_feature_set(SWITCH_FEATURE_VXLAN)) return status;
    // set ingress parser
    _Table i_table(dev_tgt, get_bf_rt_info(), smi_id::T_ING_UDP_PORT_VXLAN);
    _MatchKey i_match_key(smi_id::T_ING_UDP_PORT_VXLAN);
    // not a ternary key, just using for setValueandMask
    i_match_key.set_ternary(
        smi_id::F_ING_UDP_PORT_VXLAN_F1, udp_port_vxlan, mask);
    status |= i_table.entry_delete(i_match_key);

    // set egress parser
    dev_tgt.direction = BF_DEV_DIR_EGRESS;
    _Table e_table(dev_tgt, get_bf_rt_info(), smi_id::T_EG_UDP_PORT_VXLAN);
    _MatchKey e_match_key(smi_id::T_EG_UDP_PORT_VXLAN);
    // not a ternary key, just using for setValueandMask
    e_match_key.set_ternary(
        smi_id::F_EG_UDP_PORT_VXLAN_F1, udp_port_vxlan, mask);
    status |= e_table.entry_delete(e_match_key);

    if (feature::is_feature_set(SWITCH_FEATURE_FOLDED_SWITCH_PIPELINE)) {
      dev_tgt.direction = BF_DEV_DIR_INGRESS;
      // set ingress parser
      _Table i1_table(dev_tgt, get_bf_rt_info(), smi_id::T_ING1_UDP_PORT_VXLAN);
      _MatchKey i1_match_key(smi_id::T_ING1_UDP_PORT_VXLAN);
      // not a ternary key, just using for setValueandMask
      i1_match_key.set_ternary(
          smi_id::F_ING1_UDP_PORT_VXLAN_F1, udp_port_vxlan, mask);
      status |= i1_table.entry_delete(i1_match_key);

      // set egress parser
      dev_tgt.direction = BF_DEV_DIR_EGRESS;
      _Table e1_table(dev_tgt, get_bf_rt_info(), smi_id::T_EG1_UDP_PORT_VXLAN);
      _MatchKey e1_match_key(smi_id::T_EG1_UDP_PORT_VXLAN);
      // not a ternary key, just using for setValueandMask
      e1_match_key.set_ternary(
          smi_id::F_EG1_UDP_PORT_VXLAN_F1, udp_port_vxlan, mask);
      status |= e1_table.entry_delete(e1_match_key);
    }
    status |= auto_object::del();
    return status;
  }
};

class default_tunnel_entries : public p4_object_match_action_list {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DEFAULT_TUNNEL_ENTRIES;
  static const switch_attr_id_t status_attr_id =
      SWITCH_DEFAULT_TUNNEL_ENTRIES_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DEFAULT_TUNNEL_ENTRIES_ATTR_PARENT_HANDLE;

 public:
  default_tunnel_entries(const switch_object_id_t parent,
                         switch_status_t &status)
      : p4_object_match_action_list(smi_id::T_TUNNEL_TABLE,
                                    status_attr_id,
                                    auto_ot,
                                    parent_attr_id,
                                    parent) {
    auto it = match_action_list.begin();
    if (feature::is_feature_set(SWITCH_FEATURE_SRV6)) return;

    // set ipv4 vxlan port
    it = match_action_list.insert(it,
                                  std::pair<_MatchKey, _ActionEntry>(
                                      _MatchKey(smi_id::T_TUNNEL_TABLE),
                                      _ActionEntry(smi_id::T_TUNNEL_TABLE)));
    status |= it->first.set_exact(smi_id::F_TUNNEL_TABLE_LOCAL_MD_TUNNEL_TYPE,
                                  static_cast<uint8_t>(SWITCH_IPV4_VXLAN));
    it->second.init_action_data(smi_id::A_TUNNEL_TABLE_ENCAP_IPV4_VXLAN);
    status |= it->second.set_arg(smi_id::F_TUNNEL_TABLE_ENCAP_IPV4_VXLAN_PORT,
                                 parent,
                                 SWITCH_DEVICE_ATTR_UDP_PORT_VXLAN);

    // set ipv6 vxlan port
    it = match_action_list.insert(it,
                                  std::pair<_MatchKey, _ActionEntry>(
                                      _MatchKey(smi_id::T_TUNNEL_TABLE),
                                      _ActionEntry(smi_id::T_TUNNEL_TABLE)));
    status |= it->first.set_exact(smi_id::F_TUNNEL_TABLE_LOCAL_MD_TUNNEL_TYPE,
                                  static_cast<uint8_t>(SWITCH_IPV6_VXLAN));
    it->second.init_action_data(smi_id::A_TUNNEL_TABLE_ENCAP_IPV6_VXLAN);
    status |= it->second.set_arg(smi_id::F_TUNNEL_TABLE_ENCAP_IPV6_VXLAN_PORT,
                                 parent,
                                 SWITCH_DEVICE_ATTR_UDP_PORT_VXLAN);

    // set ipv4 IPIP
    it = match_action_list.insert(it,
                                  std::pair<_MatchKey, _ActionEntry>(
                                      _MatchKey(smi_id::T_TUNNEL_TABLE),
                                      _ActionEntry(smi_id::T_TUNNEL_TABLE)));
    status |= it->first.set_exact(smi_id::F_TUNNEL_TABLE_LOCAL_MD_TUNNEL_TYPE,
                                  static_cast<uint8_t>(SWITCH_IPV4_IPIP));
    it->second.init_action_data(smi_id::A_TUNNEL_TABLE_ENCAP_IPV4_IP);

    // set ipv6 IPIP
    it = match_action_list.insert(it,
                                  std::pair<_MatchKey, _ActionEntry>(
                                      _MatchKey(smi_id::T_TUNNEL_TABLE),
                                      _ActionEntry(smi_id::T_TUNNEL_TABLE)));
    status |= it->first.set_exact(smi_id::F_TUNNEL_TABLE_LOCAL_MD_TUNNEL_TYPE,
                                  static_cast<uint8_t>(SWITCH_IPV6_IPIP));
    it->second.init_action_data(smi_id::A_TUNNEL_TABLE_ENCAP_IPV6_IP);

    // set ipv4 GRE
    it = match_action_list.insert(it,
                                  std::pair<_MatchKey, _ActionEntry>(
                                      _MatchKey(smi_id::T_TUNNEL_TABLE),
                                      _ActionEntry(smi_id::T_TUNNEL_TABLE)));
    status |= it->first.set_exact(smi_id::F_TUNNEL_TABLE_LOCAL_MD_TUNNEL_TYPE,
                                  static_cast<uint8_t>(SWITCH_IPV4_GRE));
    it->second.init_action_data(smi_id::A_TUNNEL_TABLE_ENCAP_IPV4_GRE);

    // set ipv6 GRE
    it = match_action_list.insert(it,
                                  std::pair<_MatchKey, _ActionEntry>(
                                      _MatchKey(smi_id::T_TUNNEL_TABLE),
                                      _ActionEntry(smi_id::T_TUNNEL_TABLE)));
    status |= it->first.set_exact(smi_id::F_TUNNEL_TABLE_LOCAL_MD_TUNNEL_TYPE,
                                  static_cast<uint8_t>(SWITCH_IPV6_GRE));
    it->second.init_action_data(smi_id::A_TUNNEL_TABLE_ENCAP_IPV6_GRE);
  }
};

class device_latency_sensitivity : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DEVICE_LATENCY_SENSITIVITY;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DEVICE_LATENCY_SENSITIVITY_ATTR_PARENT_HANDLE;

 public:
  device_latency_sensitivity(const switch_object_id_t parent,
                             switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    uint8_t latency = 0;
    status = switch_store::v_get(
        parent, SWITCH_DEVICE_ATTR_LATENCY_SENSITIVITY, latency);

    for (bf_dev_pipe_t pipe :
         _Table(smi_id::T_QUEUE_REPORT_ALERT).get_active_pipes()) {
      bf_rt_target_t dev_pipe_tgt = {.dev_id = 0, .pipe_id = pipe};
      _Table table(
          dev_pipe_tgt, get_bf_rt_info(), smi_id::T_QUEUE_REPORT_ALERT);
      _ActionEntry action_entry(smi_id::T_QUEUE_REPORT_ALERT);
      action_entry.init_action_data(smi_id::A_SET_QMASK);
      action_entry.set_arg(smi_id::D_SET_QMASK_QUANTIZATION_MASK,
                           static_cast<uint32_t>(~((1LL << latency) - 1)));
      table.default_entry_set(action_entry, false);
    }

    // find and update the quantization mask for all queue_report objects
    std::set<switch_object_id_t> queue_report_handles;
    status = switch_store::referencing_set_get(
        parent, SWITCH_OBJECT_TYPE_QUEUE_REPORT, queue_report_handles);
    for (const auto &queue_report : queue_report_handles) {
      std::unique_ptr<object> queue_alert_obj(factory::get_instance().create(
          SWITCH_OBJECT_TYPE_QUEUE_ALERT, queue_report, status));
      if (queue_alert_obj != nullptr) {
        status = queue_alert_obj->create_update();
      }
    }
  }
};

class device_ecmp_hash_algo : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DEVICE_ECMP_HASH_ALGO;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DEVICE_ECMP_HASH_ALGO_ATTR_PARENT_HANDLE;

 public:
  device_ecmp_hash_algo(const switch_object_id_t parent,
                        switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    switch_object_id_t hash_algo_obj = {};
    switch_enum_t algo_type = {};
    uint32_t algo_value = 0;
    std::string algo_name;
    uint32_t seed = 0;
    uint8_t rotate = 0;

    status = switch_store::v_get(
        parent, SWITCH_DEVICE_ATTR_ECMP_DEFAULT_HASH_SEED, seed);
    status = switch_store::v_get(
        parent, SWITCH_DEVICE_ATTR_ECMP_DEFAULT_HASH_OFFSET, rotate);

    status |= switch_store::v_get(
        parent, SWITCH_DEVICE_ATTR_ECMP_DEFAULT_HASH_ALGO, hash_algo_obj);
    if (hash_algo_obj.data) {
      status |= switch_store::v_get(
          hash_algo_obj, SWITCH_HASH_ALGORITHM_ATTR_TYPE, algo_type);
    } else {
      // In case user does not configure the algorithm, use the predefined
      // default algorithm
      algo_type.enumdata = SWITCH_HASH_ALGORITHM_ATTR_TYPE_PRE_DEFINED;
    }

    // ipv4 dynamic hash algo table
    _Table table(
        get_dev_tgt(), get_bf_rt_info(), smi_id::T_IPV4_DYN_HASH_ALGORITHM);
    // ipv6 dynamic hash algo table
    _Table table_v6(
        get_dev_tgt(), get_bf_rt_info(), smi_id::T_IPV6_DYN_HASH_ALGORITHM);
    // outer ipv4
    _Table outer_table_v4(get_dev_tgt(),
                          get_bf_rt_info(),
                          smi_id::T_OUTER_IPV4_DYN_HASH_ALGORITHM);
    // outer ipv6
    _Table outer_table_v6(get_dev_tgt(),
                          get_bf_rt_info(),
                          smi_id::T_OUTER_IPV6_DYN_HASH_ALGORITHM);
    if (algo_type.enumdata == SWITCH_HASH_ALGORITHM_ATTR_TYPE_PRE_DEFINED) {
      if (hash_algo_obj.data) {
        switch_enum_t algo{};
        status |= switch_store::v_get(
            hash_algo_obj, SWITCH_HASH_ALGORITHM_ATTR_ALGORITHM, algo);
        algo_value = algo.enumdata;
      } else {
        status |= switch_store::v_get(
            parent, SWITCH_DEVICE_ATTR_ECMP_HASH_ALGO_CACHE, algo_value);
      }
      status |= switch_hash_alg_type_to_str(algo_value, algo_name);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_DEVICE,
                   "{}.{}: failed to retrive default ecmp hash algo value {}",
                   __func__,
                   __LINE__,
                   status);
        return;
      }
      status |= table.dynamic_hash_algo_set(smi_id::A_IPV4_HASH_PREDEFINED,
                                            algo_name,
                                            seed,
                                            static_cast<uint32_t>(rotate));

      status |= table_v6.dynamic_hash_algo_set(smi_id::A_IPV6_HASH_PREDEFINED,
                                               algo_name,
                                               seed,
                                               static_cast<uint32_t>(rotate));

      status |= outer_table_v4.dynamic_hash_algo_set(
          smi_id::A_OUTER_IPV4_HASH_PREDEFINED,
          algo_name,
          seed,
          static_cast<uint32_t>(rotate));

      status |= outer_table_v6.dynamic_hash_algo_set(
          smi_id::A_OUTER_IPV6_HASH_PREDEFINED,
          algo_name,
          seed,
          static_cast<uint32_t>(rotate));
    } else if (algo_type.enumdata ==
               SWITCH_HASH_ALGORITHM_ATTR_TYPE_USER_DEFINED) {
      bool reverse = false;
      uint64_t polynomial{}, init{}, final_xor{}, hash_bit_width{};
      status |= switch_store::v_get(
          hash_algo_obj, SWITCH_HASH_ALGORITHM_ATTR_POLYNOMIAL, polynomial);
      status |= switch_store::v_get(
          hash_algo_obj, SWITCH_HASH_ALGORITHM_ATTR_INIT, init);
      status |= switch_store::v_get(
          hash_algo_obj, SWITCH_HASH_ALGORITHM_ATTR_FINAL_XOR, final_xor);
      status |= switch_store::v_get(hash_algo_obj,
                                    SWITCH_HASH_ALGORITHM_ATTR_HASH_BIT_WIDTH,
                                    hash_bit_width);
      status |= switch_store::v_get(
          hash_algo_obj, SWITCH_HASH_ALGORITHM_ATTR_REVERSE, reverse);

      status |= table.dynamic_hash_algo_set(smi_id::A_IPV4_HASH_USERDEFINED,
                                            reverse,
                                            polynomial,
                                            init,
                                            final_xor,
                                            hash_bit_width,
                                            seed,
                                            static_cast<uint32_t>(rotate));

      status |= table_v6.dynamic_hash_algo_set(smi_id::A_IPV6_HASH_USERDEFINED,
                                               reverse,
                                               polynomial,
                                               init,
                                               final_xor,
                                               hash_bit_width,
                                               seed,
                                               static_cast<uint32_t>(rotate));
    }
  }
};

class device_lag_hash_algo : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DEVICE_LAG_HASH_ALGO;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DEVICE_LAG_HASH_ALGO_ATTR_PARENT_HANDLE;

 public:
  device_lag_hash_algo(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    switch_object_id_t hash_algo_obj = {};
    switch_enum_t algo_type = {};
    uint32_t algo_value = 0;
    std::string algo_name;
    uint32_t seed = 0;
    uint8_t rotate = 0;

    status = switch_store::v_get(
        parent, SWITCH_DEVICE_ATTR_LAG_DEFAULT_HASH_SEED, seed);
    status = switch_store::v_get(
        parent, SWITCH_DEVICE_ATTR_LAG_DEFAULT_HASH_OFFSET, rotate);

    status |= switch_store::v_get(
        parent, SWITCH_DEVICE_ATTR_LAG_DEFAULT_HASH_ALGO, hash_algo_obj);
    if (hash_algo_obj.data) {
      status |= switch_store::v_get(
          hash_algo_obj, SWITCH_HASH_ALGORITHM_ATTR_TYPE, algo_type);
    } else {
      // In case user does not configure the algorithm, use the predefined
      // default algorithm
      algo_type.enumdata = SWITCH_HASH_ALGORITHM_ATTR_TYPE_PRE_DEFINED;
    }

    // updating lagv4 dynamic hash algo table for algo
    _Table table(
        get_dev_tgt(), get_bf_rt_info(), smi_id::T_LAGV4_DYN_HASH_ALGORITHM);
    // updating lagv6 dynamic hash algo table for algo
    _Table table_v6(
        get_dev_tgt(), get_bf_rt_info(), smi_id::T_LAGV6_DYN_HASH_ALGORITHM);
    // updaing the non-ip hash algo table for algo
    _Table table_nonip(
        get_dev_tgt(), get_bf_rt_info(), smi_id::T_NONIP_DYN_HASH_ALGORITHM);
    if (algo_type.enumdata == SWITCH_HASH_ALGORITHM_ATTR_TYPE_PRE_DEFINED) {
      if (hash_algo_obj.data) {
        switch_enum_t algo{};
        status |= switch_store::v_get(
            hash_algo_obj, SWITCH_HASH_ALGORITHM_ATTR_ALGORITHM, algo);
        algo_value = algo.enumdata;
      } else {
        status |= switch_store::v_get(
            parent, SWITCH_DEVICE_ATTR_LAG_HASH_ALGO_CACHE, algo_value);
      }
      status |= switch_hash_alg_type_to_str(algo_value, algo_name);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_DEVICE,
                   "{}.{}: failed to retrive default LAG hash algo value {}",
                   __func__,
                   __LINE__,
                   status);
        return;
      }

      status |= table.dynamic_hash_algo_set(smi_id::A_LAGV4_HASH_PREDEFINED,
                                            algo_name,
                                            seed,
                                            static_cast<uint32_t>(rotate));

      status |= table_v6.dynamic_hash_algo_set(smi_id::A_LAGV6_HASH_PREDEFINED,
                                               algo_name,
                                               seed,
                                               static_cast<uint32_t>(rotate));

      status |=
          table_nonip.dynamic_hash_algo_set(smi_id::A_NONIP_HASH_PREDEFINED,
                                            algo_name,
                                            seed,
                                            static_cast<uint32_t>(rotate));
    } else if (algo_type.enumdata ==
               SWITCH_HASH_ALGORITHM_ATTR_TYPE_USER_DEFINED) {
      bool reverse = false;
      uint64_t polynomial{}, init{}, final_xor{}, hash_bit_width{};

      status |= switch_store::v_get(
          hash_algo_obj, SWITCH_HASH_ALGORITHM_ATTR_POLYNOMIAL, polynomial);
      status |= switch_store::v_get(
          hash_algo_obj, SWITCH_HASH_ALGORITHM_ATTR_INIT, init);
      status |= switch_store::v_get(
          hash_algo_obj, SWITCH_HASH_ALGORITHM_ATTR_FINAL_XOR, final_xor);
      status |= switch_store::v_get(hash_algo_obj,
                                    SWITCH_HASH_ALGORITHM_ATTR_HASH_BIT_WIDTH,
                                    hash_bit_width);
      status |= switch_store::v_get(
          hash_algo_obj, SWITCH_HASH_ALGORITHM_ATTR_REVERSE, reverse);

      status |= table.dynamic_hash_algo_set(smi_id::A_LAGV4_HASH_USERDEFINED,
                                            reverse,
                                            polynomial,
                                            init,
                                            final_xor,
                                            hash_bit_width,
                                            seed,
                                            static_cast<uint32_t>(rotate));

      status |= table_v6.dynamic_hash_algo_set(smi_id::A_LAGV6_HASH_USERDEFINED,
                                               reverse,
                                               polynomial,
                                               init,
                                               final_xor,
                                               hash_bit_width,
                                               seed,
                                               static_cast<uint32_t>(rotate));

      status |=
          table_nonip.dynamic_hash_algo_set(smi_id::A_NONIP_HASH_USERDEFINED,
                                            reverse,
                                            polynomial,
                                            init,
                                            final_xor,
                                            hash_bit_width,
                                            seed,
                                            static_cast<uint32_t>(rotate));
    }
  }
};

class device_ecmp_ipv4_hash : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DEVICE_ECMP_IPV4_HASH;

  static const switch_attr_id_t parent_attr_id =
      SWITCH_DEVICE_ECMP_IPV4_HASH_ATTR_PARENT_HANDLE;

 public:
  device_ecmp_ipv4_hash(const switch_object_id_t parent,
                        switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    switch_object_id_t hash_oid;
    status = switch_store::v_get(
        parent, SWITCH_DEVICE_ATTR_ECMP_IPV4_HASH, hash_oid);
    if (!switch_store::object_handle_valid(hash_oid)) return;

    switch_enum_t e = {SWITCH_HASH_ATTR_TYPE_ECMP_IPV4_HASH};
    status |= switch_store::v_set(hash_oid, SWITCH_HASH_ATTR_TYPE, e);
  }
};

class device_ecmp_ipv6_hash : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DEVICE_ECMP_IPV6_HASH;

  static const switch_attr_id_t parent_attr_id =
      SWITCH_DEVICE_ECMP_IPV6_HASH_ATTR_PARENT_HANDLE;

 public:
  device_ecmp_ipv6_hash(const switch_object_id_t parent,
                        switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    switch_object_id_t hash_oid;
    status = switch_store::v_get(
        parent, SWITCH_DEVICE_ATTR_ECMP_IPV6_HASH, hash_oid);
    if (!switch_store::object_handle_valid(hash_oid)) return;

    switch_enum_t e = {SWITCH_HASH_ATTR_TYPE_ECMP_IPV6_HASH};
    status |= switch_store::v_set(hash_oid, SWITCH_HASH_ATTR_TYPE, e);
  }
};

class device_nonip_hash : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DEVICE_NONIP_HASH;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DEVICE_NONIP_HASH_ATTR_PARENT_HANDLE;

 public:
  device_nonip_hash(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    switch_object_id_t hash_oid;
    status =
        switch_store::v_get(parent, SWITCH_DEVICE_ATTR_NON_IP_HASH, hash_oid);
    if (!switch_store::object_handle_valid(hash_oid)) return;

    switch_enum_t e = {SWITCH_HASH_ATTR_TYPE_NON_IP_HASH};
    status |= switch_store::v_set(hash_oid, SWITCH_HASH_ATTR_TYPE, e);
  }
};

class device_lag_v4_hash : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DEVICE_LAG_V4_HASH;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DEVICE_LAG_V4_HASH_ATTR_PARENT_HANDLE;

 public:
  device_lag_v4_hash(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    switch_object_id_t hash_oid;
    status =
        switch_store::v_get(parent, SWITCH_DEVICE_ATTR_LAG_IPV4_HASH, hash_oid);
    if (!switch_store::object_handle_valid(hash_oid)) return;

    switch_enum_t e = {SWITCH_HASH_ATTR_TYPE_LAG_IPV4_HASH};
    status |= switch_store::v_set(hash_oid, SWITCH_HASH_ATTR_TYPE, e);
  }
};

class device_lag_v6_hash : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DEVICE_LAG_V6_HASH;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DEVICE_LAG_V6_HASH_ATTR_PARENT_HANDLE;

 public:
  device_lag_v6_hash(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    switch_object_id_t hash_oid;
    status =
        switch_store::v_get(parent, SWITCH_DEVICE_ATTR_LAG_IPV6_HASH, hash_oid);
    if (!switch_store::object_handle_valid(hash_oid)) return;

    switch_enum_t e = {SWITCH_HASH_ATTR_TYPE_LAG_IPV6_HASH};
    status |= switch_store::v_set(hash_oid, SWITCH_HASH_ATTR_TYPE, e);
  }
};

class device_port_rate_stat : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DEVICE_PORT_RATE_STAT;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DEVICE_PORT_RATE_STAT_ATTR_PARENT_HANDLE;

 public:
  device_port_rate_stat(const switch_object_id_t parent,
                        switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    uint16_t dev_id = 0;
    bool sw_model = false;
    uint32_t port_rate_interval = 0;

    status |= switch_store::v_get(parent, SWITCH_DEVICE_ATTR_DEV_ID, dev_id);
    status |= switch_store::v_get(
        parent, SWITCH_DEVICE_ATTR_PORT_RATE_INTERVAL, port_rate_interval);

    status = bf_pal_pltfm_type_get(dev_id, &sw_model);
    if (sw_model) return;

    if (port_rate_interval != 1000) {
      switch_log(SWITCH_API_LEVEL_INFO,
                 SWITCH_OBJECT_TYPE_DEVICE,
                 "{}: port rate time has current support for 1000ms only",
                 __func__);
      status = SWITCH_STATUS_INVALID_PARAMETER;
      return;
    }

    if (!bf_lld_dev_is_tof1(0)) return;

    if (port_rate_timer->timer_valid()) {
      status = port_rate_timer->timer_stop();
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_DEVICE,
                   "port rate timer stop fail {}",
                   status);
      }
      status = port_rate_timer->timer_delete();
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_DEVICE,
                   "port rate timer del fail {}",
                   status);
      }
    }

    if (switch_store::smiContext::context().in_warm_init()) return;

    if (port_rate_interval) {
      port_rate_timer->timer_set(port_rate_interval);
      status = port_rate_timer->timer_create();
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_DEVICE,
                   "port rate timer create fail {}",
                   status);
      } else {
        port_rate_timer->timer_start();
      }
    }
  }
};

class device_nvgre_st_key : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DEVICE_NVGRE_ST_KEY;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DEVICE_NVGRE_ST_KEY_ATTR_PARENT_HANDLE;

 public:
  device_nvgre_st_key(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    uint32_t nvgre_st_key = 0;
    bool bf_rt_status = false;
    uint32_t mask = 0xFFFFFFFF;

    // vsid = 100 [or config], flowid = 0 for nvgre-st
    status = switch_store::v_get(
        parent, SWITCH_DEVICE_ATTR_NVGRE_ST_KEY, nvgre_st_key);

    bf_rt_target_t dev_tgt = {.dev_id = 0,
                              .pipe_id = BF_DEV_PIPE_ALL,
                              .direction = BF_DEV_DIR_INGRESS,
                              .prsr_id = 0xFF};
    //  set ingress parser
    _Table i_table(dev_tgt, get_bf_rt_info(), smi_id::T_INGRESS_NVGRE_ST_KEY);
    _MatchKey i_match_key(smi_id::T_INGRESS_NVGRE_ST_KEY);
    _ActionEntry i_action_entry(smi_id::T_INGRESS_NVGRE_ST_KEY);
    i_action_entry.init_indirect_data();
    // not a ternary key, just using for setValue mask
    i_match_key.set_ternary(
        smi_id::F_INGRESS_NVGRE_ST_KEY_VSID_FLOWID, nvgre_st_key, mask);
    i_table.entry_add(i_match_key, i_action_entry, bf_rt_status);

    //  set egress parser
    bf_rt_status = false;
    dev_tgt.direction = BF_DEV_DIR_EGRESS;
    _Table e_table(dev_tgt, get_bf_rt_info(), smi_id::T_EGRESS_NVGRE_ST_KEY);
    _MatchKey e_match_key(smi_id::T_EGRESS_NVGRE_ST_KEY);
    _ActionEntry e_action_entry(smi_id::T_EGRESS_NVGRE_ST_KEY);
    e_action_entry.init_indirect_data();
    // not a ternary key, just using for setValue mask
    e_match_key.set_ternary(
        smi_id::F_EGRESS_NVGRE_ST_KEY_VSID_FLOWID, nvgre_st_key, mask);
    e_table.entry_add(e_match_key, e_action_entry, bf_rt_status);
  }
};

bf_status_t bf_error_events(bf_error_sev_level_t severity,
                            bf_dev_id_t dev_id,
                            bf_dev_pipe_t pipe,
                            uint8_t stage,
                            uint64_t address,
                            bf_error_type_t type,
                            bf_error_block_t blk,
                            bf_error_block_location_t loc,
                            const char *obj_name,
                            const bf_dev_port_t *port_list,
                            int num_ports,
                            const char *string,
                            void *cookie) {
  UNUSED(pipe);
  UNUSED(stage);
  UNUSED(address);
  UNUSED(blk);
  UNUSED(loc);
  UNUSED(obj_name);
  UNUSED(port_list);
  UNUSED(num_ports);
  UNUSED(string);
  UNUSED(cookie);
  if (severity == BF_ERR_SEV_FATAL) {
    switch_device_event_data_t data;
    memset(&data, 0, sizeof(switch_device_event_data_t));
    data.device_status_event = SWITCH_DEVICE_OPER_STATUS_FAILED;
    switch (type) {
      case BF_ERR_TYPE_GENERIC: {
        data.error_type = SWITCH_DEVICE_ERROR_TYPE_GENERIC;
        break;
      }
      case BF_ERR_TYPE_SINGLE_BIT_ECC: {
        data.error_type = SWITCH_DEVICE_ERROR_TYPE_SINGLE_BIT_ECC;
        break;
      }
      case BF_ERR_TYPE_MULTI_BIT_ECC: {
        data.error_type = SWITCH_DEVICE_ERROR_TYPE_MULTI_BIT_ECC;
        break;
      }
      case BF_ERR_TYPE_PARITY: {
        data.error_type = SWITCH_DEVICE_ERROR_TYPE_PARITY;
        break;
      }
      case BF_ERR_TYPE_OVERFLOW: {
        data.error_type = SWITCH_DEVICE_ERROR_TYPE_OVERFLOW;
        break;
      }
      case BF_ERR_TYPE_UNDERFLOW: {
        data.error_type = SWITCH_DEVICE_ERROR_TYPE_UNDERFLOW;
        break;
      }
      case BF_ERR_TYPE_PKT_DROP: {
        data.error_type = SWITCH_DEVICE_ERROR_TYPE_PKT_DROP;
        break;
      }
      default:
        break;
    }
    data.device_handle = static_cast<uint32_t>(dev_id);
  }
  return BF_SUCCESS;
}
// Also, look at after_device_create() trigger for more init code
class device_init : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_DEVICE_INIT;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DEVICE_INIT_ATTR_PARENT_HANDLE;

 public:
  device_init(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status = SWITCH_STATUS_SUCCESS;
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    bf_status_t bf_status = BF_SUCCESS;
    uint32_t refresh_interval = 0;
    uint16_t flow_state_clear_cycle = 0;
    uint32_t flow_state_clear_cycle_ms = 0;
    bool sw_model = false;
    uint16_t dev_id = 0;
    uint32_t port_rate_interval = 0;
    uint32_t max_ports = 0;
    uint64_t total_buffer_size = 0;
    uint64_t total_buffer_size_kb = 0;
    int cookie = 1;
    status |=
        switch_store::v_get(get_parent(), SWITCH_DEVICE_ATTR_DEV_ID, dev_id);
    status |= switch_store::v_get(
        get_parent(), SWITCH_DEVICE_ATTR_REFRESH_INTERVAL, refresh_interval);
    status |= switch_store::v_get(get_parent(),
                                  SWITCH_DEVICE_ATTR_FLOW_STATE_CLEAR_CYCLE,
                                  (flow_state_clear_cycle));
    status |= switch_store::v_get(get_parent(),
                                  SWITCH_DEVICE_ATTR_PORT_RATE_INTERVAL,
                                  port_rate_interval);
    status |= switch_store::v_get(
        get_parent(), SWITCH_DEVICE_ATTR_MAX_PORTS, max_ports);

    // Query total TM buffer size
    status = bfrt_tm_cfg_total_buffer_size_get(dev_id, total_buffer_size);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_DEVICE_INIT,
                 "{}.{}: failed to get total buffer size for "
                 "device {} status {}",
                 __func__,
                 __LINE__,
                 dev_id,
                 switch_error_to_string(status));
      return status;
    }

    total_buffer_size_kb = total_buffer_size / 1024;
    status |= switch_store::v_set(get_parent(),
                                  SWITCH_DEVICE_ATTR_TOTAL_BUFFER_SIZE,
                                  total_buffer_size_kb);

    flow_state_clear_cycle_ms = flow_state_clear_cycle * 500;

    std::unique_ptr<device_timer> p =
        std::unique_ptr<device_timer>(new device_timer(
            refresh_interval * 1000, dev_id, device_stats_timer_cb));
    counter_timer = std::move(p);

    p = std::unique_ptr<device_timer>(new device_timer(
        port_rate_interval, dev_id, device_port_rate_timer_cb));
    port_rate_timer = std::move(p);

    p = std::unique_ptr<device_timer>(new device_timer(
        port_autoneg_interval, dev_id, device_port_autoneg_timer_cb));
    port_autoneg_timer = std::move(p);

    if (feature::is_feature_set(SWITCH_FEATURE_REPORT_SUPPRESSION)) {
      p = std::unique_ptr<device_timer>(new device_timer(
          flow_state_clear_cycle_ms, dev_id, report_state_clear_timer_cb));
      report_state_clear_timer = std::move(p);
    }

    bf_status = bf_pal_pltfm_type_get(dev_id, &sw_model);
    if (bf_status != BF_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_DEVICE,
                 "{}.{}: Device init for device {}, failed to query "
                 "platform type (hardware/model) {}",
                 __func__,
                 __LINE__,
                 get_parent(),
                 switch_error_to_string(pal_status_xlate(bf_status)));
      return pal_status_xlate(bf_status);
    }
    if (!sw_model) {
      const uint32_t SWITCH_PORT_STATS_POLL_TMR_PERIOD_MS = 50;

      bf_status = bf_pal_port_stats_poll_intvl_set(
          static_cast<bf_dev_id_t>(dev_id),
          SWITCH_PORT_STATS_POLL_TMR_PERIOD_MS);
      if (bf_status != BF_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_DEVICE,
                   "{}.{}: failed to set port stats poll interval {} ms for "
                   "device {} status {}",
                   __func__,
                   __LINE__,
                   SWITCH_PORT_STATS_POLL_TMR_PERIOD_MS,
                   dev_id,
                   pal_status_xlate(bf_status));
        return pal_status_xlate(bf_status);
      }

      status = counter_timer->timer_create();
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_DEVICE,
                   "{}: failed to start counter timer status={} for device {}",
                   __func__,
                   status,
                   get_parent());
        return status;
      }

      if (report_state_clear_timer) {
        status = report_state_clear_timer->timer_create();
        if (status != SWITCH_STATUS_SUCCESS) {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_DEVICE,
                     "{}: failed to start dtel report state clear timer "
                     "status={} for device {}",
                     __func__,
                     status,
                     get_parent());
          return status;
        }
        if (flow_state_clear_cycle_ms >= 1000) {
          report_state_clear_timer->timer_start();
        } else {
          switch_log(
              SWITCH_API_LEVEL_INFO,
              SWITCH_OBJECT_TYPE_DEVICE,
              "{}: dtel bloom filter timer start skipped, current "
              "supported minimum timer precision is 2sec, User value:{} sec",
              __func__,
              flow_state_clear_cycle);
        }
      }

      status = port_rate_timer->timer_create();
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_DEVICE,
                   "{}: Fail to allocate port rate timer status={} for dev {}",
                   __func__,
                   status,
                   get_parent());
        return status;
      }

      status = port_autoneg_timer->timer_create();
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(
            SWITCH_API_LEVEL_ERROR,
            SWITCH_OBJECT_TYPE_DEVICE,
            "{}: Fail to allocate port autoneg timer status={} for dev {}",
            __func__,
            status,
            get_parent());
        return status;
      }
    }

    SWITCH_CONTEXT.port_rate_create(dev_id, max_ports);

    status = mac_aging_callback_register(get_parent());
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_DEVICE,
                 "{}: aging callback register failed status={} for device {}",
                 __func__,
                 status,
                 get_parent());
      return status;
    }

    status = mac_learn_callback_register(get_parent());
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_DEVICE,
                 "{}: learn callback register failed status={} for device {}",
                 __func__,
                 status,
                 get_parent());
      return status;
    }

    if (feature::is_feature_set(SWITCH_FEATURE_NAT)) {
      status = nat_aging_callback_register(get_parent());
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(
            SWITCH_API_LEVEL_ERROR,
            SWITCH_OBJECT_TYPE_DEVICE,
            "{}: nat aging callback register failed status={} for device {}",
            __func__,
            status,
            get_parent());
        return status;
      }
    }
    // set max queues as 8 for Tofino1 and 16 for Tofino2 and beyond
    uint8_t max_queues = 0;
    if (bf_lld_dev_is_tof1(0)) {
      max_queues = TOFINO1_MAX_PORT_QUEUES;
    } else {
      max_queues = TOFINO2_MAX_PORT_QUEUES;
    }
    switch_store::v_set(
        get_parent(), SWITCH_DEVICE_ATTR_MAX_PORT_QUEUE, max_queues);

    bf_status = bf_register_error_events(dev_id, bf_error_events, &cookie);
    if (bf_status != BF_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_DEVICE,
                 "{}: bf register callback error events failed status={} for "
                 "device {}",
                 __func__,
                 pal_status_xlate(bf_status),
                 dev_id);
      return pal_status_xlate(bf_status);
    }
    std::vector<bf_rt_table_id_t> asym_tables = {
        smi_id::T_VLAN_MEMBERSHIP,
        smi_id::T_INGRESS_SFLOW_SESSION,
        smi_id::T_INGRESS_STP0_CHECK,
        smi_id::T_INGRESS_STP1_CHECK,
        smi_id::T_EGRESS_STP_CHECK,
        smi_id::T_INGRESS_DROP_STATS,
        smi_id::T_EGRESS_DROP_STATS,
        smi_id::T_PPG,
        smi_id::T_QUEUE,
        smi_id::T_WRED_INDEX,
        smi_id::T_EGRESS_WRED_STATS,
        smi_id::T_QUEUE_REPORT_ALERT,
        smi_id::T_QUEUE_REPORT_CHECK_QUOTA,
        smi_id::T_DTEL_CONFIG,
        smi_id::T_STORM_CONTROL,
        smi_id::T_STORM_CONTROL_STATS,
        smi_id::T_INGRESS_PFC_WD_ACL,
        smi_id::T_EGRESS_PFC_WD_ACL,
        smi_id::T_INGRESS_PORT_IP_STATS,
        smi_id::T_EGRESS_PORT_IP_STATS,
        smi_id::T_DSCP_TC_MAP,
        smi_id::T_PCP_TC_MAP,
        smi_id::T_L3_QOS_MAP,
        smi_id::T_L2_QOS_MAP};
    if (feature::is_feature_set(SWITCH_FEATURE_EGRESS_SYSTEM_ACL_STATS)) {
      asym_tables.push_back(smi_id::T_EGRESS_SYSTEM_ACL);
    }
    if (feature::is_feature_set(SWITCH_FEATURE_BFD_OFFLOAD)) {
      asym_tables.push_back(smi_id::T_BFD_RX_TIMER);
      asym_tables.push_back(smi_id::T_BFD_TX_SESSION);
      asym_tables.push_back(smi_id::T_BFD_PKT_ACTION);
    }
    for (auto table_id : asym_tables) {
      _Table table(get_dev_tgt(), get_bf_rt_info(), table_id);
      status = table.asymmetric_scope_set();
      if (status != SWITCH_STATUS_SUCCESS) {
        std::string table_name;
        table.name_get(table_name);
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_DEVICE,
                   "{}.{}: status:{} failed asymmetric set for table {}",
                   __func__,
                   __LINE__,
                   status,
                   table_name);
        return status;
      }
    }

    std::vector<bf_rt_table_id_t> pvs_tables = {smi_id::T_ING_UDP_PORT_VXLAN,
                                                smi_id::T_EG_UDP_PORT_VXLAN,
                                                smi_id::T_ING1_UDP_PORT_VXLAN,
                                                smi_id::T_EG1_UDP_PORT_VXLAN,
                                                smi_id::T_INGRESS_NVGRE_ST_KEY,
                                                smi_id::T_EGRESS_NVGRE_ST_KEY};
    for (auto table_id : pvs_tables) {
      _Table table(get_dev_tgt(), get_bf_rt_info(), table_id);
      status =
          table.pvs_scope_set(((table_id == smi_id::T_ING_UDP_PORT_VXLAN) ||
                               (table_id == smi_id::T_ING1_UDP_PORT_VXLAN) ||
                               (table_id == smi_id::T_INGRESS_NVGRE_ST_KEY))
                                  ? GressTarget::GRESS_TARGET_INGRESS
                                  : GressTarget::GRESS_TARGET_EGRESS);
      if (status != SWITCH_STATUS_SUCCESS) {
        std::string table_name;
        table.name_get(table_name);
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_DEVICE,
                   "{}.{}: status:{} failed pvs scope set for table {}",
                   __func__,
                   __LINE__,
                   status,
                   table_name);
        return status;
      }
    }

    if (feature::is_feature_set(SWITCH_FEATURE_PKTGEN)) {
      _Table table(
          get_dev_tgt(), get_bf_rt_info(), smi_id::T_INGRESS_PKTGEN_PORT);
      status |= table.pvs_scope_set(GressTarget::GRESS_TARGET_INGRESS, true);
      if (status != SWITCH_STATUS_SUCCESS) {
        std::string table_name;
        table.name_get(table_name);
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_DEVICE,
                   "{}.{}: status:{} failed pvs scope set for table {}",
                   __func__,
                   __LINE__,
                   status,
                   table_name);
        return status;
      }
    }

    if (smi_id::F_EGRESS_IPV4_ACL_LOCAL_MD_USER_METADATA) {
      switch_range_t range = {};
      size_t acl_user_meta_data_width = 0;
      // ACL user metadata size is common for ACL tables
      _Table eg_ipv4_acl_table(
          get_dev_tgt(), get_bf_rt_info(), smi_id::T_EGRESS_IPV4_ACL);
      eg_ipv4_acl_table.match_key_field_size_get(
          smi_id::F_EGRESS_IPV4_ACL_LOCAL_MD_USER_METADATA,
          &acl_user_meta_data_width);
      range.min = 0;
      range.max = (1 << acl_user_meta_data_width) - 1;
      status = switch_store::v_set(
          get_parent(), SWITCH_DEVICE_ATTR_ACL_USER_METADATA_RANGE, range);
    }
    {
      // Getting ECMP Dynamic Hash Algorithm and Seed
      std::string algo_str;
      uint32_t seed = 0, rotate = 0;

      _Table algo_table(
          get_dev_tgt(), get_bf_rt_info(), smi_id::T_IPV4_DYN_HASH_ALGORITHM);
      status = algo_table.dynamic_hash_algo_get(
          smi_id::A_IPV4_HASH_PREDEFINED, algo_str, seed, rotate);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_DEVICE,
                   "{}:{}: failed to get dynamic ecmp hash algo status {}",
                   __func__,
                   __LINE__,
                   status);
        return status;
      }

      if (!algo_str.empty()) {
        uint32_t algo = switch_hash_alg_str_to_type(algo_str);
        status = switch_store::v_set(
            get_parent(), SWITCH_DEVICE_ATTR_ECMP_HASH_ALGO_CACHE, algo);
        if (status != SWITCH_STATUS_SUCCESS) {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_DEVICE,
                     "{}:{}: failed to cache  ecmp default hash algorithm {}",
                     __func__,
                     __LINE__,
                     status);
        }
      }

      if (seed != 0) {
        status = switch_store::v_set(
            get_parent(), SWITCH_DEVICE_ATTR_ECMP_DEFAULT_HASH_SEED, seed);
      }
      if (rotate != 0) {
        status =
            switch_store::v_set(get_parent(),
                                SWITCH_DEVICE_ATTR_ECMP_DEFAULT_HASH_OFFSET,
                                static_cast<uint8_t>(rotate));
      }
    }
    {
      // Getting LAG Hash Algorithm and Seed.
      std::string algo_str;
      uint32_t seed = 0, rotate = 0;

      _Table algo_table(
          get_dev_tgt(), get_bf_rt_info(), smi_id::T_LAGV4_DYN_HASH_ALGORITHM);
      status = algo_table.dynamic_hash_algo_get(
          smi_id::A_LAGV4_HASH_PREDEFINED, algo_str, seed, rotate);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_DEVICE,
                   "{}:{}: failed to get dynamic LAG hash algo status {}",
                   __func__,
                   __LINE__,
                   status);
        return status;
      }

      if (!algo_str.empty()) {
        uint32_t algo = switch_hash_alg_str_to_type(algo_str);
        status = switch_store::v_set(
            get_parent(), SWITCH_DEVICE_ATTR_LAG_HASH_ALGO_CACHE, algo);
        if (status != SWITCH_STATUS_SUCCESS) {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_DEVICE,
                     "{}:{}: failed to cache lag default hash algorithm {}",
                     __func__,
                     __LINE__,
                     status);
        }
      }

      if (seed != 0) {
        status = switch_store::v_set(
            get_parent(), SWITCH_DEVICE_ATTR_LAG_DEFAULT_HASH_SEED, seed);
      }
      if (rotate != 0) {
        status = switch_store::v_set(get_parent(),
                                     SWITCH_DEVICE_ATTR_LAG_DEFAULT_HASH_OFFSET,
                                     static_cast<uint8_t>(seed));
      }
    }
    {
      bool bf_rt_status = false;
      _Table table(
          get_dev_tgt(), get_bf_rt_info(), smi_id::AP_BD_ACTION_PROFILE);
      _MatchKey match_key(smi_id::AP_BD_ACTION_PROFILE);
      _ActionEntry action_entry(smi_id::AP_BD_ACTION_PROFILE);
      match_key.set_exact(smi_id::F_BD_ACTION_PROFILE_ACTION_MEMBER_ID,
                          static_cast<uint32_t>(DEFAULT_ACTION_MEMBER_ID));
      action_entry.init_action_data(smi_id::A_NO_ACTION);
      status = table.entry_add(match_key, action_entry, bf_rt_status);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(
            SWITCH_API_LEVEL_ERROR,
            SWITCH_OBJECT_TYPE_DEVICE,
            "{}:{}: failed to create default bd action profile status {}",
            __func__,
            __LINE__,
            status);
        return status;
      }
    }

    {
      bool bf_rt_status = false;
      _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::AP_ECMP_SELECTOR);
      _MatchKey match_key(smi_id::AP_ECMP_SELECTOR);
      _ActionEntry action_entry(smi_id::AP_ECMP_SELECTOR);
      match_key.set_exact(smi_id::F_ECMP_SELECTOR_ACTION_MEMBER_ID,
                          static_cast<uint32_t>(DEFAULT_ACTION_MEMBER_ID));
      action_entry.init_action_data(smi_id::A_SET_ECMP_PROPERTIES_DROP);
      status = table.entry_add(match_key, action_entry, bf_rt_status);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(
            SWITCH_API_LEVEL_ERROR,
            SWITCH_OBJECT_TYPE_DEVICE,
            "{}:{}: failed to create default inner ecmp member status {}",
            __func__,
            __LINE__,
            status);
        return status;
      }
    }

    {
      bool bf_rt_status = false;
      _Table table(
          get_dev_tgt(), get_bf_rt_info(), smi_id::AP_OUTER_ECMP_SELECTOR);
      _MatchKey match_key(smi_id::AP_OUTER_ECMP_SELECTOR);
      _ActionEntry action_entry(smi_id::AP_OUTER_ECMP_SELECTOR);
      match_key.set_exact(smi_id::F_OUTER_ECMP_SELECTOR_ACTION_MEMBER_ID,
                          static_cast<uint32_t>(DEFAULT_ACTION_MEMBER_ID));
      action_entry.init_action_data(smi_id::A_NO_ACTION);
      status = table.entry_add(match_key, action_entry, bf_rt_status);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(
            SWITCH_API_LEVEL_ERROR,
            SWITCH_OBJECT_TYPE_DEVICE,
            "{}:{}: failed to create default outer ecmp member status {}",
            __func__,
            __LINE__,
            status);
        return status;
      }
    }

    {
      bool bf_rt_status = false;
      _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::AP_LAG_SELECTOR);
      _MatchKey match_key(smi_id::AP_LAG_SELECTOR);
      _ActionEntry action_entry(smi_id::AP_LAG_SELECTOR);
      match_key.set_exact(smi_id::F_LAG_SELECTOR_ACTION_MEMBER_ID,
                          static_cast<uint32_t>(DEFAULT_ACTION_MEMBER_ID));
      action_entry.init_action_data(smi_id::A_LAG_MISS);
      status = table.entry_add(match_key, action_entry, bf_rt_status);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(
            SWITCH_API_LEVEL_ERROR,
            SWITCH_OBJECT_TYPE_DEVICE,
            "{}:{}: failed to create default lag selector member status {}",
            __func__,
            __LINE__,
            status);
        return status;
      }
    }

    // Default 100G copp meter
    {
      uint64_t cir = 100000000000UL / (8 * 100);
      uint64_t pir = 100000000000UL / (8 * 100);
      uint64_t cbs = 1000, pbs = 1000;
      _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_COPP_METER);
      _MatchKey match_key(smi_id::T_COPP_METER);
      _ActionEntry action_entry(smi_id::T_COPP_METER);
      match_key.set_exact(smi_id::F_COPP_METER_METER_INDEX,
                          static_cast<uint32_t>(0));
      action_entry.init_indirect_data();
      action_entry.set_arg(smi_id::D_COPP_METER_METER_SPEC_CIR_PPS, cir);
      action_entry.set_arg(smi_id::D_COPP_METER_METER_SPEC_PIR_PPS, pir);
      action_entry.set_arg(smi_id::D_COPP_METER_METER_SPEC_CBS_PKTS, cbs);
      action_entry.set_arg(smi_id::D_COPP_METER_METER_SPEC_PBS_PKTS, pbs);
      status |= table.entry_modify(match_key, action_entry);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_DEVICE,
                   "{}:{}: failed to create default 100g meter status {}",
                   __func__,
                   __LINE__,
                   status);
        return status;
      }
    }

    // default  -  storm control meter
    {
      uint64_t cir = 100000000000UL / (8 * 100);
      uint64_t pir = 100000000000UL / (8 * 100);
      uint64_t cbs = 1000, pbs = 1000;
      for (bf_dev_pipe_t tpipe :
           _Table(smi_id::T_STORM_CONTROL_METER).get_active_pipes()) {
        bf_rt_target_t dev_tgt = {.dev_id = 0, .pipe_id = tpipe};
        _Table table(dev_tgt, get_bf_rt_info(), smi_id::T_STORM_CONTROL_METER);
        _MatchKey match_key(smi_id::T_STORM_CONTROL_METER);
        _ActionEntry action_entry(smi_id::T_STORM_CONTROL_METER);
        match_key.set_exact(smi_id::F_STORM_CONTROL_METER_METER_INDEX,
                            static_cast<uint32_t>(0));
        action_entry.init_indirect_data();
        action_entry.set_arg(smi_id::D_STORM_CONTROL_METER_METER_SPEC_CIR_KBPS,
                             cir);
        action_entry.set_arg(smi_id::D_STORM_CONTROL_METER_METER_SPEC_PIR_KBPS,
                             pir);
        action_entry.set_arg(smi_id::D_STORM_CONTROL_METER_METER_SPEC_CBS_KBITS,
                             cbs);
        action_entry.set_arg(smi_id::D_STORM_CONTROL_METER_METER_SPEC_PBS_KBITS,
                             pbs);
        status |= table.entry_modify(match_key, action_entry);
        if (status != SWITCH_STATUS_SUCCESS) {
          switch_log(
              SWITCH_API_LEVEL_ERROR,
              SWITCH_OBJECT_TYPE_DEVICE,
              "{}:{}: fail to program default storm control meter status {}",
              __func__,
              __LINE__,
              status);
          return status;
        }
      }
    }

    if (feature::is_feature_set(SWITCH_FEATURE_ACL_PORT_GROUP)) {
      switch_log(SWITCH_API_LEVEL_DEBUG,
                 SWITCH_OBJECT_TYPE_NONE,
                 "Port group support in ACL enabled");
      status |= switch_store::v_set(
          get_parent(), SWITCH_DEVICE_ATTR_INGRESS_ACL_PORT_GROUP_ENABLE, true);
    }

    // For AFP internal ports are always managed by BMAI
    // For FP internal ports are managed by BMAI on model . On hardware these
    // are manged by Driver
    if (feature::is_feature_set(SWITCH_FEATURE_ASYMMETRIC_FOLDED_PIPELINE) ||
        feature::is_feature_set(SWITCH_FEATURE_FOLDED_SWITCH_PIPELINE)) {
      SWITCH_CONTEXT.set_internal_pipe_ports_bmai_managed(true);
    }

    auto_object::create_update();

    return status;
  }
};

// Initializes the lists of supported ingress and egress acl actions for device
switch_status_t device_acl_actions_init(const switch_object_id_t device_obj) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::vector<uint8_t> ingress_acl_actions;
  std::vector<uint8_t> egress_acl_actions;
  attr_w ingress_acl_actions_attr(SWITCH_DEVICE_ATTR_INGRESS_ACL_ACTIONS);
  attr_w egress_acl_actions_attr(SWITCH_DEVICE_ATTR_EGRESS_ACL_ACTIONS);

  ingress_acl_actions.push_back(SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION);
  ingress_acl_actions.push_back(SWITCH_ACL_ENTRY_ATTR_ACTION_COUNTER_HANDLE);
  ingress_acl_actions.push_back(SWITCH_ACL_ENTRY_ATTR_ACTION_SET_VRF_HANDLE);
  ingress_acl_actions.push_back(SWITCH_ACL_ENTRY_ATTR_ACTION_METER_HANDLE);
  ingress_acl_actions.push_back(SWITCH_ACL_ENTRY_ATTR_SET_USER_METADATA);
  if (smi_id::A_INGRESS_MAC_ACL_REDIRECT_NEXTHOP ||
      smi_id::A_INGRESS_IP_ACL_REDIRECT_NEXTHOP ||
      smi_id::A_INGRESS_IPV4_ACL_REDIRECT_NEXTHOP ||
      smi_id::A_INGRESS_IPV6_ACL_REDIRECT_NEXTHOP ||
      smi_id::A_INGRESS_MAC_ACL_REDIRECT_PORT ||
      smi_id::A_INGRESS_IP_ACL_REDIRECT_PORT ||
      smi_id::A_INGRESS_IPV4_ACL_REDIRECT_PORT ||
      smi_id::A_INGRESS_IPV6_ACL_REDIRECT_PORT) {
    ingress_acl_actions.push_back(SWITCH_ACL_ENTRY_ATTR_REDIRECT);
  }
  if (smi_id::A_INGRESS_MAC_ACL_MIRROR || smi_id::A_INGRESS_IP_ACL_MIRROR ||
      smi_id::A_INGRESS_IPV4_ACL_MIRROR || smi_id::A_INGRESS_IPV6_ACL_MIRROR ||
      smi_id::A_INGRESS_IP_MIRROR_ACL_MIRROR ||
      smi_id::A_INGRESS_IP_DTEL_ACL_MIRROR) {
    ingress_acl_actions.push_back(
        SWITCH_ACL_ENTRY_ATTR_ACTION_INGRESS_MIRROR_HANDLE);
  }
  if (smi_id::A_INGRESS_IP_MIRROR_ACL_MIRROR_OUT) {
    ingress_acl_actions.push_back(
        SWITCH_ACL_ENTRY_ATTR_ACTION_EGRESS_MIRROR_HANDLE);
  }
  if (smi_id::A_INGRESS_MAC_ACL_SET_TC || smi_id::A_INGRESS_IP_ACL_SET_TC ||
      smi_id::A_INGRESS_IPV4_ACL_SET_TC || smi_id::A_INGRESS_IPV6_ACL_SET_TC ||
      smi_id::A_INGRESS_IP_MIRROR_ACL_SET_TC ||
      smi_id::A_INGRESS_IP_DTEL_ACL_SET_TC) {
    ingress_acl_actions.push_back(SWITCH_ACL_ENTRY_ATTR_ACTION_SET_TC);
  }
  if (smi_id::A_INGRESS_MAC_ACL_SET_COLOR ||
      smi_id::A_INGRESS_IP_ACL_SET_COLOR ||
      smi_id::A_INGRESS_IPV4_ACL_SET_COLOR ||
      smi_id::A_INGRESS_IPV6_ACL_SET_COLOR ||
      smi_id::A_INGRESS_IP_MIRROR_ACL_SET_COLOR ||
      smi_id::A_INGRESS_IP_DTEL_ACL_SET_COLOR) {
    ingress_acl_actions.push_back(SWITCH_ACL_ENTRY_ATTR_ACTION_SET_COLOR);
  }
  if (smi_id::A_INGRESS_IPV4_ACL_NO_NAT) {
    ingress_acl_actions.push_back(SWITCH_ACL_ENTRY_ATTR_ACTION_DISABLE_NAT);
  }
  // hack since there is one attribute for dtel_action_type
  if (smi_id::A_INGRESS_IP_DTEL_ACL_SET_DTEL_REPORT_TYPE) {
    ingress_acl_actions.push_back(
        SWITCH_ACL_ENTRY_ATTR_ACTION_REPORT_ALL_PACKETS);
  }
  if (smi_id::A_INGRESS_IP_DTEL_ACL_IFA_CLONE_SAMPLE ||
      smi_id::P_INGRESS_IP_DTEL_ACL_IFA_CLONE_SESSION_ID ||
      smi_id::A_INGRESS_IP_DTEL_ACL_IFA_CLONE_AND_SET_REPORT_TYPE ||
      smi_id::P_INGRESS_IP_DTEL_ACL_IFA_CLONE_SESSION_ID_WITH_TYPE) {
    ingress_acl_actions.push_back(SWITCH_ACL_ENTRY_ATTR_DTEL_ACTION_TYPE);
  }

  egress_acl_actions.push_back(SWITCH_ACL_ENTRY_ATTR_PACKET_ACTION);
  egress_acl_actions.push_back(SWITCH_ACL_ENTRY_ATTR_ACTION_COUNTER_HANDLE);
  if (smi_id::A_EGRESS_MAC_ACL_MIRROR || smi_id::A_EGRESS_IPV4_ACL_MIRROR ||
      smi_id::A_EGRESS_IPV6_ACL_MIRROR ||
      smi_id::A_EGRESS_IPV4_MIRROR_ACL_MIRROR ||
      smi_id::A_EGRESS_IPV6_MIRROR_ACL_MIRROR) {
    egress_acl_actions.push_back(
        SWITCH_ACL_ENTRY_ATTR_ACTION_EGRESS_MIRROR_HANDLE);
  }
  if (smi_id::A_EGRESS_IPV4_ACL_MIRROR_IN ||
      smi_id::A_EGRESS_IPV6_ACL_MIRROR_IN ||
      smi_id::A_EGRESS_IPV4_MIRROR_ACL_MIRROR_IN ||
      smi_id::A_EGRESS_IPV6_MIRROR_ACL_MIRROR_IN) {
    egress_acl_actions.push_back(
        SWITCH_ACL_ENTRY_ATTR_ACTION_INGRESS_MIRROR_HANDLE);
  }

  ingress_acl_actions_attr.v_set(ingress_acl_actions);
  egress_acl_actions_attr.v_set(egress_acl_actions);
  status |= switch_store::attribute_set(device_obj, ingress_acl_actions_attr);
  status |= switch_store::attribute_set(device_obj, egress_acl_actions_attr);

  return status;
}

class device_acl_init : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DEVICE_ACL_INIT;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DEVICE_ACL_INIT_ATTR_PARENT_HANDLE;

 public:
  device_acl_init(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status = device_acl_actions_init(parent);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(
          SWITCH_API_LEVEL_ERROR,
          SWITCH_OBJECT_TYPE_DEVICE,
          "{}.{}: failed to initialize device acl actions list status={}",
          __func__,
          __LINE__,
          status);
    }
  }
};

// device object is parent
class device_rmac : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_DEVICE_RMAC;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DEVICE_RMAC_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id = SWITCH_DEVICE_RMAC_ATTR_STATUS;

 public:
  device_rmac(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_INGRESS_PV_RMAC,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_mac_addr_t mac = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    switch_mac_addr_t mac_mask = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    status = switch_store::v_get(parent, SWITCH_DEVICE_ATTR_SRC_MAC, mac);
    status |= match_key.set_ternary(
        smi_id::F_INGRESS_PV_RMAC_HDR_ETHERNET_DST_ADDR, mac, mac_mask);

    action_entry.init_action_data(smi_id::A_RMAC_HIT);
  }
};

// device object is parent
class device_vxlan_rmac : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_DEVICE_VXLAN_RMAC;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DEVICE_VXLAN_RMAC_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_DEVICE_VXLAN_RMAC_ATTR_STATUS;

 public:
  device_vxlan_rmac(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_TUNNEL_VXLAN_DEVICE_RMAC,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    status |= match_key.set_exact(
        smi_id::F_TUNNEL_VXLAN_DEVICE_RMAC_HDR_INNER_ETHERNET_DST_ADDR,
        parent,
        SWITCH_DEVICE_ATTR_SRC_MAC);

    action_entry.init_action_data(smi_id::A_TUNNEL_RMAC_HIT);
  }
};

class outer_tunnel_dmac : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_OUTER_TUNNEL_DMAC;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_OUTER_TUNNEL_DMAC_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_OUTER_TUNNEL_DMAC_ATTR_STATUS;

 public:
  outer_tunnel_dmac(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_INGRESS_PV_RMAC,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    switch_mac_addr_t mac = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    switch_mac_addr_t mac_mask = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    status = switch_store::v_get(parent, SWITCH_DEVICE_ATTR_TUNNEL_DMAC, mac);

    status |= match_key.set_ternary(
        smi_id::F_INGRESS_PV_RMAC_HDR_ETHERNET_DST_ADDR, mac, mac_mask);

    action_entry.init_action_data(smi_id::A_RMAC_HIT);
  }
};

class inner_tunnel_dmac : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_INNER_TUNNEL_DMAC;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INNER_TUNNEL_DMAC_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id =
      SWITCH_INNER_TUNNEL_DMAC_ATTR_STATUS;

 public:
  inner_tunnel_dmac(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_TUNNEL_VXLAN_DEVICE_RMAC,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    status |= match_key.set_exact(
        smi_id::F_TUNNEL_VXLAN_DEVICE_RMAC_HDR_INNER_ETHERNET_DST_ADDR,
        parent,
        SWITCH_DEVICE_ATTR_TUNNEL_DMAC);

    action_entry.init_action_data(smi_id::A_TUNNEL_RMAC_HIT);
  }
};

switch_status_t before_device_update(const switch_object_id_t handle,
                                     const attr_w &attr) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_mac_addr_t src_mac = {}, tunnel_dmac = {};
  switch_attr_id_t attr_id = attr.id_get();
  switch (attr_id) {
    case SWITCH_DEVICE_ATTR_INGRESS_ACL:
    case SWITCH_DEVICE_ATTR_PRE_INGRESS_ACL:
    case SWITCH_DEVICE_ATTR_EGRESS_ACL:
      return update_bind_point_flag(handle, attr);
    default:
      break;
  }

  // delete the entry here and add later
  if (attr.id_get() == SWITCH_DEVICE_ATTR_UDP_PORT_VXLAN) {
    uint16_t udp_port_vxlan = 4789, mask = 0xFFFF;

    status = switch_store::v_get(
        handle, SWITCH_DEVICE_ATTR_UDP_PORT_VXLAN, udp_port_vxlan);

    bf_rt_target_t dev_tgt = {.dev_id = 0,
                              .pipe_id = BF_DEV_PIPE_ALL,
                              .direction = BF_DEV_DIR_INGRESS,
                              .prsr_id = 0xFF};
    // set ingress parser
    _Table i_table(dev_tgt, get_bf_rt_info(), smi_id::T_ING_UDP_PORT_VXLAN);
    _MatchKey i_match_key(smi_id::T_ING_UDP_PORT_VXLAN);
    // not a ternary key, just using for setValueandMask
    i_match_key.set_ternary(
        smi_id::F_ING_UDP_PORT_VXLAN_F1, udp_port_vxlan, mask);
    i_table.entry_delete(i_match_key);

    // set egress parser
    dev_tgt.direction = BF_DEV_DIR_EGRESS;
    _Table e_table(dev_tgt, get_bf_rt_info(), smi_id::T_EG_UDP_PORT_VXLAN);
    _MatchKey e_match_key(smi_id::T_EG_UDP_PORT_VXLAN);
    // not a ternary key, just using for setValueandMask
    e_match_key.set_ternary(
        smi_id::F_EG_UDP_PORT_VXLAN_F1, udp_port_vxlan, mask);
    e_table.entry_delete(e_match_key);

    if (feature::is_feature_set(SWITCH_FEATURE_FOLDED_SWITCH_PIPELINE)) {
      dev_tgt.direction = BF_DEV_DIR_INGRESS;
      // set ingress parser
      _Table i1_table(dev_tgt, get_bf_rt_info(), smi_id::T_ING1_UDP_PORT_VXLAN);
      _MatchKey i1_match_key(smi_id::T_ING1_UDP_PORT_VXLAN);
      // not a ternary key, just using for setValueandMask
      i1_match_key.set_ternary(
          smi_id::F_ING1_UDP_PORT_VXLAN_F1, udp_port_vxlan, mask);
      i1_table.entry_delete(i1_match_key);

      // set egress parser
      dev_tgt.direction = BF_DEV_DIR_EGRESS;
      _Table e1_table(dev_tgt, get_bf_rt_info(), smi_id::T_EG1_UDP_PORT_VXLAN);
      _MatchKey e1_match_key(smi_id::T_EG1_UDP_PORT_VXLAN);
      // not a ternary key, just using for setValueandMask
      e1_match_key.set_ternary(
          smi_id::F_EG1_UDP_PORT_VXLAN_F1, udp_port_vxlan, mask);
      e1_table.entry_delete(e1_match_key);
    }
  }

  // remove entry first, then program it back with new value
  if (attr.id_get() == SWITCH_DEVICE_ATTR_NVGRE_ST_KEY) {
    uint32_t nvgre_st_key = 0;
    uint32_t mask = 0xFFFFFFFF;

    status = switch_store::v_get(
        handle, SWITCH_DEVICE_ATTR_NVGRE_ST_KEY, nvgre_st_key);

    bf_rt_target_t dev_tgt = {.dev_id = 0,
                              .pipe_id = BF_DEV_PIPE_ALL,
                              .direction = BF_DEV_DIR_INGRESS,
                              .prsr_id = 0xFF};
    // set ingress parser
    _Table i_table(dev_tgt, get_bf_rt_info(), smi_id::T_INGRESS_NVGRE_ST_KEY);
    _MatchKey i_match_key(smi_id::T_INGRESS_NVGRE_ST_KEY);
    // not a ternary key, just using for setValue, mask
    i_match_key.set_ternary(
        smi_id::F_INGRESS_NVGRE_ST_KEY_VSID_FLOWID, nvgre_st_key, mask);
    i_table.entry_delete(i_match_key);

    // set egress parser
    dev_tgt.direction = BF_DEV_DIR_EGRESS;
    _Table e_table(dev_tgt, get_bf_rt_info(), smi_id::T_EGRESS_NVGRE_ST_KEY);
    _MatchKey e_match_key(smi_id::T_EGRESS_NVGRE_ST_KEY);
    // not a ternary key, just using for setValue, mask
    e_match_key.set_ternary(
        smi_id::F_EGRESS_NVGRE_ST_KEY_VSID_FLOWID, nvgre_st_key, mask);
    e_table.entry_delete(e_match_key);
  }

  if (attr.id_get() == SWITCH_DEVICE_ATTR_SRC_MAC) {
    device_rmac dmac1(handle, status);
    status |= dmac1.del();
    device_vxlan_rmac dmac2(handle, status);
    status |= dmac2.del();

    // When tunnel_dmac == src_mac (which will soon have a different value),
    // restore tunnel_dmac entries
    status |= switch_store::v_get(handle, SWITCH_DEVICE_ATTR_SRC_MAC, src_mac);
    status |= switch_store::v_get(
        handle, SWITCH_DEVICE_ATTR_TUNNEL_DMAC, tunnel_dmac);
    if (tunnel_dmac == src_mac) {
      inner_tunnel_dmac dmac3(handle, status);
      status |= dmac3.create_update();
      outer_tunnel_dmac dmac4(handle, status);
      status |= dmac4.create_update();
    }
    return status;
  }

  if (attr.id_get() == SWITCH_DEVICE_ATTR_TUNNEL_DMAC) {
    inner_tunnel_dmac dmac1(handle, status);
    status = dmac1.del();
    outer_tunnel_dmac dmac2(handle, status);
    status = dmac2.del();
    return status;
  }
  if (attr.id_get() == SWITCH_DEVICE_ATTR_CPU_PORT) {
    switch_object_id_t cpu_port_handle{0}, old_cpu_port_handle{0};
    status = attr.v_get(cpu_port_handle);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_DEVICE,
                 "{}.{}: Device cpu port update failed for device {:#x}, cpu "
                 "port handle get failed: {}",
                 __func__,
                 __LINE__,
                 handle.data,
                 attr.id_get(),
                 status);
      return status;
    }
    status = switch_store::v_get(
        handle, SWITCH_DEVICE_ATTR_CPU_PORT, old_cpu_port_handle);
    if (old_cpu_port_handle.data && !cpu_port_handle.data) {
      std::unique_ptr<object> default_ingress_system_acl(
          factory::get_instance().create(
              SWITCH_OBJECT_TYPE_DEFAULT_INGRESS_SYSTEM_ACL, handle, status));
      if (default_ingress_system_acl != nullptr) {
        status = default_ingress_system_acl->del();
        if (status != SWITCH_STATUS_SUCCESS) {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_PPG_HELPER,
                     "{}.{}: Device cpu port update failed for device {:#x}, "
                     "cpu port handle {:#x}: {}",
                     __func__,
                     __LINE__,
                     handle,
                     cpu_port_handle,
                     status);
          return status;
        }
      }
    }
  }

  return status;
}

switch_status_t after_device_update(const switch_object_id_t handle,
                                    const attr_w &attr) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t vrf_handle = {};
  switch_mac_addr_t src_mac = {}, tunnel_dmac = {};
  auto attr_id = attr.id_get();

  if (attr_id == SWITCH_DEVICE_ATTR_SRC_MAC) {
    // When tunnel_dmac == src_mac, delete tunnel_dmac entries
    status |= switch_store::v_get(handle, SWITCH_DEVICE_ATTR_SRC_MAC, src_mac);
    status |= switch_store::v_get(
        handle, SWITCH_DEVICE_ATTR_TUNNEL_DMAC, tunnel_dmac);
    if (tunnel_dmac == src_mac) {
      inner_tunnel_dmac dmac1(handle, status);
      status |= dmac1.del();
      outer_tunnel_dmac dmac2(handle, status);
      status |= dmac2.del();
    }

    device_rmac dmac3(handle, status);
    status |= dmac3.create_update();
    device_vxlan_rmac dmac4(handle, status);
    status |= dmac4.create_update();

    // update the default VRF with new src mac
    status |=
        switch_store::v_get(handle, SWITCH_DEVICE_ATTR_DEFAULT_VRF, vrf_handle);
    status |= switch_store::v_set(vrf_handle, SWITCH_VRF_ATTR_SRC_MAC, src_mac);
    return status;
  }

  if (attr_id == SWITCH_DEVICE_ATTR_TUNNEL_DMAC) {
    // When tunnel_dmac == src_mac, do not program tunnel_dmac entries
    status |= switch_store::v_get(handle, SWITCH_DEVICE_ATTR_SRC_MAC, src_mac);
    status |= switch_store::v_get(
        handle, SWITCH_DEVICE_ATTR_TUNNEL_DMAC, tunnel_dmac);
    if (tunnel_dmac == src_mac) return status;

    inner_tunnel_dmac dmac1(handle, status);
    status |= dmac1.create_update();
    outer_tunnel_dmac dmac2(handle, status);
    status |= dmac2.create_update();
    return status;
  }

  // Let all PPGs/Queues know about the change in device level Default Buffer
  // Profile
  // PPGs/Queues are configured based on their own buffer profiles if one is
  // present
  // If ppg level buffer profile is absent, device level ingress buffer profile
  // is used
  // If queue level buffer profile is absent, device level egress buffer profile
  // is used
  std::set<switch_object_id_t> port_handles;
  status = switch_store::referencing_set_get(
      handle, SWITCH_OBJECT_TYPE_PORT, port_handles);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: Port get failed for device {}: {}",
               __func__,
               __LINE__,
               handle,
               status);
    return status;
  }

  if (attr_id == SWITCH_DEVICE_ATTR_DEFAULT_INGRESS_BUFFER_PROFILE_HANDLE) {
    for (auto const &port_handle : port_handles) {
      std::vector<switch_object_id_t> ppg_handles;
      status = switch_store::v_get(
          port_handle, SWITCH_PORT_ATTR_PORT_PRIORITY_GROUPS, ppg_handles);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_DEVICE,
                   "{}.{}: Port priority group list get failed for device {} "
                   "port {}: {}",
                   __func__,
                   __LINE__,
                   handle,
                   port_handle,
                   status);
        return status;
      }
      for (auto const &ppg_handle : ppg_handles) {
        std::unique_ptr<object> ppg_helper_obj(factory::get_instance().create(
            SWITCH_OBJECT_TYPE_PPG_HELPER, ppg_handle, status));
        status = ppg_helper_obj->create_update();
        if (status != SWITCH_STATUS_SUCCESS) {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_PPG_HELPER,
                     "{}.{}: Buffer profile update failed for device {} port "
                     "{} ppg {}: {}",
                     __func__,
                     __LINE__,
                     handle,
                     port_handle,
                     ppg_handle,
                     status);
          return status;
        }
      }
      ppg_handles.clear();
    }
  } else if (attr_id ==
             SWITCH_DEVICE_ATTR_DEFAULT_EGRESS_BUFFER_PROFILE_HANDLE) {
    for (auto const &port_handle : port_handles) {
      std::vector<switch_object_id_t> queue_handles;
      status = switch_store::v_get(
          port_handle, SWITCH_PORT_ATTR_QUEUE_HANDLES, queue_handles);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_DEVICE,
                   "{}.{}: Queue handles list get failed for device {} "
                   "port {}: {}",
                   __func__,
                   __LINE__,
                   handle,
                   port_handle,
                   status);
        return status;
      }
      for (auto const &queue_handle : queue_handles) {
        std::unique_ptr<object> queue_buffer_config(
            factory::get_instance().create(
                SWITCH_OBJECT_TYPE_QUEUE_BUFFER_CONFIG, queue_handle, status));
        status = queue_buffer_config->create_update();
        if (status != SWITCH_STATUS_SUCCESS) {
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_DEVICE,
                     "{}.{}: Buffer profile update failed for device {} port "
                     "{} queue {}: {}",
                     __func__,
                     __LINE__,
                     handle,
                     port_handle,
                     queue_handle,
                     status);
          return status;
        }
      }
      queue_handles.clear();
    }
  }

  // Set max vrf
  uint16_t max_size_of_vrf = (1 << smi_id::F_IPV4_FIB_VRF_SIZE);
  status |=
      switch_store::v_set(handle, SWITCH_DEVICE_ATTR_MAX_VRF, max_size_of_vrf);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_PORT,
               "{}: max vrfs get failed status={} for device {}",
               __func__,
               status,
               handle);
    return SWITCH_STATUS_FAILURE;
  }

  return status;
}

switch_status_t device_initialize() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  status |= switch_store::reg_update_trigs_before(SWITCH_OBJECT_TYPE_DEVICE,
                                                  &before_device_update);
  status |= switch_store::reg_update_trigs_after(SWITCH_OBJECT_TYPE_DEVICE,
                                                 &after_device_update);
  REGISTER_OBJECT(device_init, SWITCH_OBJECT_TYPE_DEVICE_INIT);
  REGISTER_OBJECT(device_aging, SWITCH_OBJECT_TYPE_DEVICE_AGING);
  REGISTER_OBJECT(device_refresh_interval,
                  SWITCH_OBJECT_TYPE_DEVICE_REFRESH_INTERVAL);
  REGISTER_OBJECT(device_dtel_bloom_filters_clear_cycle,
                  SWITCH_OBJECT_TYPE_DEVICE_DTEL_BLOOM_FILTERS_CLEAR_CYCLE);
  REGISTER_OBJECT(device_warm_reboot, SWITCH_OBJECT_TYPE_DEVICE_WARM_REBOOT);
  REGISTER_OBJECT(device_learn_notif_timeout,
                  SWITCH_OBJECT_TYPE_DEVICE_LEARN_NOTIF_TIMEOUT);
  REGISTER_OBJECT(device_cut_through, SWITCH_OBJECT_TYPE_DEVICE_CUT_THROUGH);
  REGISTER_OBJECT(device_hostif, SWITCH_OBJECT_TYPE_DEVICE_HOSTIF);
  REGISTER_OBJECT(device_vxlan_port, SWITCH_OBJECT_TYPE_DEVICE_VXLAN_PORT);
  REGISTER_OBJECT(default_tunnel_entries,
                  SWITCH_OBJECT_TYPE_DEFAULT_TUNNEL_ENTRIES);
  REGISTER_OBJECT(device_fdb_unicast_packet_action,
                  SWITCH_OBJECT_TYPE_DEVICE_FDB_UNICAST_PACKET_ACTION);
  REGISTER_OBJECT(device_fdb_broadcast_packet_action,
                  SWITCH_OBJECT_TYPE_DEVICE_FDB_BROADCAST_PACKET_ACTION);
  REGISTER_OBJECT(device_fdb_multicast_packet_action,
                  SWITCH_OBJECT_TYPE_DEVICE_FDB_MULTICAST_PACKET_ACTION);
  REGISTER_OBJECT(device_latency_sensitivity,
                  SWITCH_OBJECT_TYPE_DEVICE_LATENCY_SENSITIVITY);
  REGISTER_OBJECT(device_ecmp_hash_algo,
                  SWITCH_OBJECT_TYPE_DEVICE_ECMP_HASH_ALGO);
  REGISTER_OBJECT(device_lag_hash_algo,
                  SWITCH_OBJECT_TYPE_DEVICE_LAG_HASH_ALGO);
  REGISTER_OBJECT(device_ecmp_ipv4_hash,
                  SWITCH_OBJECT_TYPE_DEVICE_ECMP_IPV4_HASH);
  REGISTER_OBJECT(device_ecmp_ipv6_hash,
                  SWITCH_OBJECT_TYPE_DEVICE_ECMP_IPV6_HASH);
  REGISTER_OBJECT(device_nonip_hash, SWITCH_OBJECT_TYPE_DEVICE_NONIP_HASH);
  REGISTER_OBJECT(device_lag_v4_hash, SWITCH_OBJECT_TYPE_DEVICE_LAG_V4_HASH);
  REGISTER_OBJECT(device_lag_v6_hash, SWITCH_OBJECT_TYPE_DEVICE_LAG_V6_HASH);
  REGISTER_OBJECT(device_acl_init, SWITCH_OBJECT_TYPE_DEVICE_ACL_INIT);
  REGISTER_OBJECT(device_nvgre_st_key, SWITCH_OBJECT_TYPE_DEVICE_NVGRE_ST_KEY);
  REGISTER_OBJECT(device_rmac, SWITCH_OBJECT_TYPE_DEVICE_RMAC);
  REGISTER_OBJECT(device_vxlan_rmac, SWITCH_OBJECT_TYPE_DEVICE_VXLAN_RMAC);
  REGISTER_OBJECT(inner_tunnel_dmac, SWITCH_OBJECT_TYPE_INNER_TUNNEL_DMAC);
  REGISTER_OBJECT(outer_tunnel_dmac, SWITCH_OBJECT_TYPE_OUTER_TUNNEL_DMAC);
  if (feature::is_feature_set(SWITCH_FEATURE_PKTGEN)) {
    REGISTER_OBJECT(pktgen_pvs, SWITCH_OBJECT_TYPE_PKTGEN_PVS);
  }

  for (auto p : _Table(smi_id::T_INGRESS_PORT_MAPPING).get_active_pipes()) {
    SWITCH_CONTEXT.update_switch_ingress_pipe_list(p);
  }

  for (auto p : _Table(smi_id::T_EGRESS_SYSTEM_ACL).get_active_pipes()) {
    SWITCH_CONTEXT.update_switch_egress_pipe_list(p);
  }

  return status;
}

switch_status_t device_clean() {
  uint16_t dev_id = 0;
  bool sw_model = false;
  bf_status_t bf_status = BF_SUCCESS;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  // In some corner scenario where the poll stats timer invokes
  // the polling function even before the timer is stopped, but
  // gets processed after the counter save process is complete
  // So will use this flag to block the polling logic in such cases
  switch_store::smiContext::context().stats_timer_turn_off();

  status = bf_pal_pltfm_type_get(dev_id, &sw_model);
  if (sw_model) {
    return status;
  }

  // stop MAC counter poll timer
  bf_status = bf_pal_port_stats_poll_stop(static_cast<bf_dev_id_t>(dev_id));
  if (bf_status != BF_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: Fail to stop port stats poll timer for "
               "dev {} status {}",
               __func__,
               __LINE__,
               dev_id,
               pal_status_xlate(bf_status));
    return pal_status_xlate(bf_status);
  }
  switch_log(SWITCH_API_LEVEL_WARN,
             SWITCH_OBJECT_TYPE_DEVICE,
             "{}.{}: Stop port stats poll timer for dev {}",
             __func__,
             __LINE__,
             dev_id);

  // stop MAU counter poll timer
  if (counter_timer->timer_valid()) {
    status = counter_timer->timer_stop();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_DEVICE,
                 "{}.{}: Fail to stop MAU stats timer for "
                 "dev {} status {}",
                 __func__,
                 __LINE__,
                 dev_id,
                 status);
      return status;
    }
  }
  switch_log(SWITCH_API_LEVEL_WARN,
             SWITCH_OBJECT_TYPE_DEVICE,
             "{}.{}: Stop MAU stats poll timer",
             __func__,
             __LINE__);

  // Stop port autoneg poll timer
  if (port_autoneg_timer->timer_valid()) {
    status = port_autoneg_timer->timer_stop();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_DEVICE,
                 "{}.{}: Fail to stop port autoneg timer {}",
                 __func__,
                 __LINE__,
                 status);
      return status;
    }
  }
  switch_log(SWITCH_API_LEVEL_WARN,
             SWITCH_OBJECT_TYPE_DEVICE,
             "{}.{}: Stop port autoneg poll timer",
             __func__,
             __LINE__);

  // Stop port rate poll timer
  if (port_rate_timer->timer_valid()) {
    status = port_rate_timer->timer_stop();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_DEVICE,
                 "{}.{}: Fail to stop port rate timer {}",
                 __func__,
                 __LINE__,
                 status);
      return status;
    }
  }
  switch_log(SWITCH_API_LEVEL_WARN,
             SWITCH_OBJECT_TYPE_DEVICE,
             "{}.{}: Stop port rate poll timer",
             __func__,
             __LINE__);

  // stop TM counter poll timer
  p4_pd_tm_stop_cache_counters_timer(dev_id);
  switch_log(SWITCH_API_LEVEL_WARN,
             SWITCH_OBJECT_TYPE_DEVICE,
             "{}.{}: Stop TM stats poll timer for dev {}",
             __func__,
             __LINE__,
             dev_id);

  return SWITCH_STATUS_SUCCESS;
}
}  // namespace smi
