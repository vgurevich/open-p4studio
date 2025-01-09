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


#include "common/utils.h"

extern "C" {
#include <tofino/bf_pal/bf_pal_port_intf.h>
#include <tofino/pdfixed/pd_common.h>
#include <tofino/pdfixed/pd_mirror.h>
#include <tofino/pdfixed/pd_mc.h>
}

#include <algorithm>
#include <string>
#include <bitset>
#include <vector>
#include <set>
#include <unordered_map>
#include <mutex>  // NOLINT(build/c++11)

#include "common/multicast.h"
#include "common/bfrt_tm.h"

namespace smi {
using namespace smi::bf_rt;  // NOLINT(build/namespaces)
using ::smi::logging::switch_log;

bf_dev_pipe_t SHIFT_PIPE_IF_FOLDED(bf_dev_pipe_t pipe,
                                   const std::set<bf_dev_pipe_t> &table_pipes) {
  if (table_pipes.find(pipe) == table_pipes.end())
    return pipe ^ 0x1;
  else
    return pipe;
}

bf_dev_pipe_t translate_ingress_pipe(bf_dev_pipe_t external_pipe) {
  auto &temp_list = SWITCH_CONTEXT.get_switch_ingress_pipe_list();
  std::vector<bf_dev_pipe_t> ingress_pipe_list(temp_list.begin(),
                                               temp_list.end());
  uint8_t pipe_id = 0;
  for (auto &pipe : SWITCH_CONTEXT.get_switch_non_ingress_pipe_list()) {
    if (external_pipe == pipe) break;
    pipe_id++;
  }
  return ingress_pipe_list[(pipe_id) % (ingress_pipe_list.size())];
}

bf_dev_pipe_t translate_egress_pipe(bf_dev_pipe_t internal_pipe) {
  auto temp_list = SWITCH_CONTEXT.get_switch_egress_pipe_list();
  std::vector<bf_dev_pipe_t> egress_pipe_list(temp_list.begin(),
                                              temp_list.end());
  uint8_t pipe_id = 0;
  std::set<bf_dev_pipe_t> non_switch_egress_pipes;
  auto active_pipes = get_active_pipes();
  // non switch egress is all pipes - egress pipes
  std::set_difference(
      active_pipes.begin(),
      active_pipes.end(),
      temp_list.begin(),
      temp_list.end(),
      std::inserter(non_switch_egress_pipes, non_switch_egress_pipes.end()));
  for (auto &pipe : non_switch_egress_pipes) {
    if (internal_pipe == pipe) break;
    pipe_id++;
  }
  return egress_pipe_list[(pipe_id) % (egress_pipe_list.size())];
}

uint16_t translate_ingress_port(uint16_t external_dev_port) {
  return (feature::is_feature_set(SWITCH_FEATURE_ASYMMETRIC_FOLDED_PIPELINE))
             ? ((external_dev_port & ~(3U << 7)) |
                (translate_ingress_pipe(DEV_PORT_TO_PIPE(external_dev_port))
                 << 7))
             : external_dev_port;
}

uint16_t translate_egress_port(uint16_t internal_dev_port) {
  return feature::is_feature_set(SWITCH_FEATURE_ASYMMETRIC_FOLDED_PIPELINE)
             ? ((internal_dev_port & ~(3U << 7)) |
                (translate_egress_pipe(DEV_PORT_TO_PIPE(internal_dev_port))
                 << 7))
             : internal_dev_port;
}

void switchContext::clear_yid() {
  LOCK_GUARD guard(yid_mtx);
  for (uint16_t i = 0; i < yid_bitmap.size(); ++i) yid_bitmap.set(i, false);
  return;
}

void switchContext::update_yid(uint16_t yid, switch_object_type_t ot) {
  if (yid == 0) {
    return;
  }
  LOCK_GUARD guard(yid_mtx);
  yid_bitmap.set(yid - 1, true);
  switch_log(SWITCH_API_LEVEL_DEBUG, ot, "Updated yid {}", yid);
}

uint16_t switchContext::reserve_yid(switch_object_type_t ot) {
  LOCK_GUARD guard(yid_mtx);
  for (uint16_t i = 0; i < yid_bitmap.size(); ++i) {
    if (yid_bitmap.test(i)) continue;
    yid_bitmap.set(i, true);
    switch_log(SWITCH_API_LEVEL_DEBUG, ot, "Reserving yid {}", i + 1);
    return (i + 1);
  }
  return 0;
}

uint16_t switchContext::alloc_yid(switch_object_id_t handle) {
  const switch_object_type_t object_type =
      switch_store::object_type_query(handle);

  LOCK_GUARD guard(yid_mtx);

  if (switch_store::smiContext::context().in_warm_init()) {
    uint16_t yid = 0;
    if (object_type == SWITCH_OBJECT_TYPE_PORT) {
      switch_store::v_get(handle, SWITCH_PORT_ATTR_YID, yid);
    } else if (object_type == SWITCH_OBJECT_TYPE_LAG) {
      switch_store::v_get(handle, SWITCH_LAG_ATTR_YID, yid);
    }
    // this yid has not been reserved yet
    yid_bitmap.set(yid - 1, true);
    switch_log(
        SWITCH_API_LEVEL_DEBUG, object_type, "Warm init reserve yid {}", yid);
    return (yid);
  } else {
    if (yid_bitmap.all()) return 0;
    for (uint16_t i = 0; i < yid_bitmap.size(); ++i) {
      if (yid_bitmap.test(i)) continue;
      yid_bitmap.set(i, true);
      if (object_type == SWITCH_OBJECT_TYPE_PORT) {
        switch_store::v_set(
            handle, SWITCH_PORT_ATTR_YID, static_cast<uint16_t>(i + 1));
      } else if (object_type == SWITCH_OBJECT_TYPE_LAG) {
        switch_store::v_set(
            handle, SWITCH_LAG_ATTR_YID, static_cast<uint16_t>(i + 1));
      }
      switch_log(
          SWITCH_API_LEVEL_DEBUG, object_type, "Allocating yid {}", i + 1);
      return (i + 1);
    }
  }
  return 0;
}

void switchContext::release_yid(switch_object_id_t handle) {
  uint16_t yid = 0;
  switch_mc_port_map_t port_map = {0};
  uint32_t pd_status = 0;
  uint32_t mc_sess_hdl = get_mc_sess_hdl();

  const switch_object_type_t object_type =
      switch_store::object_type_query(handle);

  LOCK_GUARD guard(yid_mtx);

  if (object_type == SWITCH_OBJECT_TYPE_PORT) {
    switch_store::v_get(handle, SWITCH_PORT_ATTR_YID, yid);
    if (yid == 0) return;
    yid_bitmap.set(yid - 1, false);
    switch_store::v_set(handle, SWITCH_PORT_ATTR_YID, static_cast<uint16_t>(0));
  } else if (object_type == SWITCH_OBJECT_TYPE_LAG) {
    switch_store::v_get(handle, SWITCH_LAG_ATTR_YID, yid);
    if (yid == 0) return;
    yid_bitmap.set(yid - 1, false);
    switch_store::v_set(handle, SWITCH_LAG_ATTR_YID, static_cast<uint16_t>(0));
  }

  // clear the prune map for this yid
  memset(port_map, 0x0, sizeof(switch_mc_port_map_t));
  pd_status = p4_pd_mc_update_port_prune_table(mc_sess_hdl, 0, yid, port_map);
  p4_pd_mc_complete_operations(mc_sess_hdl);
  if (pd_status != 0) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               object_type,
               "Error clearing prune map for yid {}",
               yid);
  }
  switch_log(SWITCH_API_LEVEL_DEBUG, object_type, "Releasing yid {}", yid);

  return;
}

void switchContext::port_rate_create(const uint16_t dev_id,
                                     const uint32_t max_ports) {
  if (dev_id >= SWITCH_MAX_DEVICE) return;

  // allocating the vector buffer to store the counter value
  if (port_counter_val.empty()) {
    dev_to_port_list_t port_cntr_list(SWITCH_MAX_DEVICE);
    port_counter_val = port_cntr_list;
  }
  port_to_stat_t port_counters_list(max_ports);
  port_counter_val[dev_id] = port_counters_list;

  // allocating the vector buffer to store port rate values
  if (port_rate.empty()) {
    dev_to_port_list_t port_rate_list(SWITCH_MAX_DEVICE);
    port_rate = port_rate_list;
  }
  port_to_stat_t port_rate_list(max_ports);
  port_rate[dev_id] = port_rate_list;
}

void switchContext::port_rate_reset(const uint16_t dev_id,
                                    const uint32_t port_id) {
  if (dev_id >= SWITCH_MAX_DEVICE) return;

  LOCK_GUARD guard(prs_mtx);
  if ((!port_counter_val.empty()) && (!port_counter_val[dev_id].empty()))
    port_counter_val[dev_id][port_id].clear();

  if ((!port_rate.empty()) && (!port_rate[dev_id].empty()))
    port_rate[dev_id][port_id].clear();
}

void switchContext::port_rate_update(
    const uint16_t dev_id,
    const uint32_t port_id,
    uint64_t counters_val[BF_PORT_RATE_MAX_COUNTERS]) {
  if (dev_id >= SWITCH_MAX_DEVICE) return;

  port_stat_list_t port_rate_list(BF_PORT_RATE_MAX_COUNTERS);
  LOCK_GUARD guard(prs_mtx);

  // when the update happens for the first time, there wont
  // be any element in both port_counter_val, port_rate vectors.
  // So, initialize the vector elements with value zero
  if (port_counter_val[dev_id][port_id].empty()) {
    port_counter_val[dev_id][port_id].assign(BF_PORT_RATE_MAX_COUNTERS, 0);
  }

  if (port_rate[dev_id][port_id].empty()) {
    port_rate[dev_id][port_id].assign(BF_PORT_RATE_MAX_COUNTERS, 0);
  }

  for (int cnt = 0; cnt < BF_PORT_RATE_MAX_COUNTERS; cnt++) {
    if (counters_val[cnt] >= port_counter_val[dev_id][port_id][cnt]) {
      port_rate[dev_id][port_id][cnt] =
          counters_val[cnt] - port_counter_val[dev_id][port_id][cnt];
    }
    port_counter_val[dev_id][port_id][cnt] = counters_val[cnt];
  }

  return;
}

void switchContext::port_rate_get(uint16_t dev_id,
                                  uint32_t port_id,
                                  std::vector<uint64_t> &rate) {
  LOCK_GUARD guard(prs_mtx);
  if ((!port_rate.empty()) && (!port_rate[dev_id][port_id].empty()))
    rate = port_rate[dev_id][port_id];
}

switch_status_t compute_scaled_weights(
    std::vector<bf_rt_id_t> &mbrs,
    std::vector<uint32_t> &weights,
    uint32_t max_grp_size,
    std::unordered_map<bf_rt_id_t, uint32_t> &scaled_weights) {
  uint32_t sum_of_weights = 0;

  if (mbrs.size() != weights.size() || weights.size() == 0) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "Mismatched member {} and weight list size {}",
               mbrs.size(),
               weights.size());
    return SWITCH_STATUS_INVALID_PARAMETER;
  }

  for (auto &n : weights) sum_of_weights += n;

  // ECMP case where weight is 1
  if (mbrs.size() == sum_of_weights) {
    for (uint32_t i = 0; i < mbrs.size(); i++)
      scaled_weights[mbrs.at(i)] = weights.at(i);
  } else if (sum_of_weights == 0) {
    return SWITCH_STATUS_FAILURE;
  } else {
    // scale the weights to _max_grp_size
    // weights: 10, 20, 30, 10
    // sum = 70
    // max_grp_size=256
    // scale_factor = 256/70 = 3.6571
    // clang-format off
    //       deficit from   Scaled   Scaled        Rounded  %error
    //       prev stage     weight   wgt+deficit   weight
    // 10    0.00           36.57    36.57         37       1.17%
    // 20    -0.43          73.14    72.71         73       -0.20%
    // 30    0.14           109.71   109.86        110      0.26%
    // 10    -0.29          36.57    36.29         36       -1.56%
    // clang-format on
    double scale_factor = static_cast<double>(max_grp_size / sum_of_weights);
    double deficit = 0;
    for (uint32_t i = 0; i < mbrs.size(); i++) {
      double scaled_weight = weights.at(i) * scale_factor;
      double scaled_weight_with_deficit = scaled_weight + deficit;
      double rounded_weight = std::round(scaled_weight_with_deficit);
      scaled_weights[mbrs.at(i)] = static_cast<uint32_t>(rounded_weight);
      deficit = scaled_weight - rounded_weight;
    }
  }
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t execute_cb_for_all(switch_object_type_t ot,
                                   object_cb cb,
                                   switch_attribute_t attr) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  bool more = false;
  switch_object_id_t first_handle = {0};

  status = switch_store::object_get_first_handle(ot, first_handle);
  if (status != SWITCH_STATUS_SUCCESS) {
    if (status == SWITCH_STATUS_ITEM_NOT_FOUND) {
      // No objects of this type, return
      return SWITCH_STATUS_SUCCESS;
    }
    switch_log(SWITCH_API_LEVEL_ERROR,
               ot,
               "{}: failed to get first handle for object_type={} status={}",
               __func__,
               ot,
               status);
    return status;
  }
  // execute callback for first handle
  status = cb(first_handle, attr);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               ot,
               "{}: callback failed object_type={} status={}",
               __func__,
               ot,
               status);
    return status;
  }
  do {
    more = false;
    std::vector<switch_object_id_t> handles;
    uint32_t count = 0;
    status = switch_store::object_get_next_handles(
        first_handle, 1024, handles, count);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 ot,
                 "{}: failed to get more handles object_type={} status={}",
                 __func__,
                 ot,
                 status);
      return status;
    }
    for (auto it = handles.begin(); it != handles.end(); it++) {
      switch_object_id_t local_handle = *it;
      status = cb(local_handle, attr);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   ot,
                   "{}: callback failed object_type={} status={}",
                   __func__,
                   ot,
                   status);
        return status;
      }
    }

    if (handles.size() == 1024) {
      more = true;
      first_handle = handles.back();
    }
  } while (more == true);

  return status;
}

switch_status_t queue_tail_drop_set(switch_object_id_t port_handle,
                                    switch_object_id_t pfc_queue_qos_map_handle,
                                    uint32_t pfc_map) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t device_handle = {0};
  const uint8_t PFC_MAX_PRIO = 8;
  uint16_t dev_id = 0;
  uint16_t dev_port = 0;
  std::vector<switch_object_id_t> pfc_queue_qos_map_list, queue_list;

  status =
      switch_store::v_get(port_handle, SWITCH_PORT_ATTR_DEVICE, device_handle);
  status |=
      switch_store::v_get(device_handle, SWITCH_DEVICE_ATTR_DEV_ID, dev_id);
  status |=
      switch_store::v_get(port_handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
  status |= switch_store::v_get(
      port_handle, SWITCH_PORT_ATTR_QUEUE_HANDLES, queue_list);
  std::set<uint8_t> port_qids;
  for (auto const queue : queue_list) {
    uint8_t qid = 0;
    status |= switch_store::v_get(queue, SWITCH_QUEUE_ATTR_QUEUE_ID, qid);
    port_qids.insert(qid);
  }

  if (pfc_queue_qos_map_handle.data) {
    status |= switch_store::v_get(pfc_queue_qos_map_handle,
                                  SWITCH_QOS_MAP_EGRESS_ATTR_QOS_MAP_LIST,
                                  pfc_queue_qos_map_list);
  }

  for (auto qos_map : pfc_queue_qos_map_list) {
    uint8_t priority = 0;

    // Get the PFC priority from the qos_map handle
    status |= switch_store::v_get(
        qos_map, SWITCH_QOS_MAP_ATTR_PFC_PRIORITY, priority);

    for (uint8_t pfc_prio = 0; pfc_prio < PFC_MAX_PRIO; pfc_prio++) {
      if (pfc_prio == priority) {
        p4_pd_status_t pd_status = 0;
        // Got the right priority to queue entry. Take the qid.
        uint8_t qid = 0;
        status |= switch_store::v_get(qos_map, SWITCH_QOS_MAP_ATTR_QID, qid);

        // Disable tail drops on all the queues where PFC is enabled and enable
        // on the others
        pd_status = ((pfc_map >> pfc_prio) & 1)
                        ? p4_pd_tm_disable_q_tail_drop(dev_id, dev_port, qid)
                        : p4_pd_tm_enable_q_tail_drop(dev_id, dev_port, qid);
        if (pd_status != 0) {
          status = SWITCH_STATUS_FAILURE;
          switch_log(SWITCH_API_LEVEL_ERROR,
                     SWITCH_OBJECT_TYPE_PORT,
                     "{}:{}: Failed to enable/disable tail drops for queue {} "
                     "port {} device {}",
                     __func__,
                     __LINE__,
                     qid,
                     dev_port,
                     dev_id);
          return status;
        }
        port_qids.erase(qid);
      }
    }
    // For queues that do not have a cos mapping enable tail drops
    for (auto qid : port_qids) {
      p4_pd_tm_enable_q_tail_drop(dev_id, dev_port, qid);
    }
  }

  return status;
}

switch_status_t compute_pd_buffer_bytes_to_cells(uint16_t dev_id,
                                                 uint64_t bytes_threshold,
                                                 uint32_t *cell_threshold) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  uint32_t cell_size = 0;

  status = bfrt_tm_cfg_cell_size_bytes_get(dev_id, cell_size);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "{}.{}: Failed to get cell size in bytes, device {} "
               "status {}",
               __func__,
               __LINE__,
               dev_id,
               switch_error_to_string(status));
    return status;
  }

  if (bytes_threshold > UINT32_MAX) {
    switch_log(SWITCH_API_LEVEL_WARN,
               SWITCH_OBJECT_TYPE_NONE,
               "{}:{}: bytes threshold: {} exceeds UINT32_MAX and will be "
               "typecasted to uint32_t",
               __func__,
               __LINE__,
               bytes_threshold);
  }

  *cell_threshold =
      (static_cast<uint32_t>(bytes_threshold) + cell_size - 1) / cell_size;

  return status;
}

switch_status_t compute_pd_buffer_cells_to_bytes(uint16_t dev_id,
                                                 uint32_t num_cells,
                                                 uint64_t *num_bytes) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  p4_pd_status_t pd_status;
  uint32_t cell_size = 0;

  pd_status = p4_pd_tm_get_cell_size_in_bytes(dev_id, &cell_size);
  if (pd_status != SWITCH_STATUS_SUCCESS) {
    status = SWITCH_STATUS_FAILURE;
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "{}:{}: Failed to get cell size in bytes for device  {} , "
               "pd_status {}",
               __func__,
               __LINE__,
               dev_id,
               pd_status);
    return status;
  }
  *num_bytes = (uint64_t)num_cells * (uint64_t)cell_size;
  return status;
}

void get_hash_enum_metadata(std::vector<EnumMetadata> &enums) {
  ModelInfo *model_info = switch_store::switch_model_info_get();
  const ObjectInfo *object_info =
      model_info->get_object_info(SWITCH_OBJECT_TYPE_HASH_ALGORITHM);
  auto attr_md =
      object_info->get_attr_metadata(SWITCH_HASH_ALGORITHM_ATTR_ALGORITHM);
  const ValueMetadata *value_md = attr_md->get_value_metadata();
  enums = value_md->get_enum_metadata();
}

switch_status_t switch_hash_alg_type_to_str(uint32_t algo_value,
                                            std::string &algo_name) {
  std::vector<EnumMetadata> enums;
  get_hash_enum_metadata(enums);
  if (algo_value >= enums.size()) return SWITCH_STATUS_FAILURE;
  algo_name.assign(enums[algo_value].enum_name);
  return SWITCH_STATUS_SUCCESS;
}

uint32_t switch_hash_alg_str_to_type(std::string algo_name) {
  std::vector<EnumMetadata> enums;
  get_hash_enum_metadata(enums);
  std::transform(
      algo_name.begin(), algo_name.end(), algo_name.begin(), ::toupper);
  for (auto obj : enums) {
    if ((obj.enum_name).compare(algo_name) == 0) {
      return obj.enum_value;
    }
  }
  return 0;
}

/*
  This utility function convert from mask value start and length value.
  @param start_bit =  value is the first position of the set bit.
  @param length = length of the continious bit from start_bit.

  Example: ipv4_mask = 255.255.255.0
  start_bit: 8
  length: 24

*/
void get_mask_info(uint32_t mask, uint32_t &start_bit, uint32_t &length) {
  bool set_in = false;
  while (mask) {
    if (mask & 1) {
      length += 1;
      if (!set_in) {
        set_in = true;
      }
    } else {
      if (set_in) {
        break;
      }
      start_bit += 1;
    }
    mask >>= 1;
  }
}

// This utility function convert from mac mask value start and length value.
void get_mac_mask_info(switch_mac_addr_t mac_mask,
                       uint32_t &p_start_bit,
                       uint32_t &p_length) {
  uint32_t multibyte_start_bit = 0;
  uint32_t multibyte_length = 0;
  uint32_t start_bit = 0;
  uint32_t length = 0;
  bool setbit_set = false;
  for (int8_t cnt = (ETH_LEN - 1); cnt >= 0; --cnt) {
    start_bit = 0;
    length = 0;
    get_mask_info(mac_mask.mac[cnt], start_bit, length);
    if (length > 0) {
      if (!setbit_set) {
        setbit_set = true;
        multibyte_start_bit += start_bit;
        multibyte_length = length;
        if (start_bit == 0 && length != CHAR_BIT) {
          break;
        } else {
          continue;
        }
      }
      if (start_bit == 0) {
        multibyte_length += length;
      }
      if (length != CHAR_BIT) {
        break;
      }
    } else {
      if (setbit_set) {
        break;
      }
      multibyte_start_bit += 8;
    }
  }
  p_start_bit = multibyte_start_bit;
  p_length = multibyte_length;
}

// This utility function convert from ip mask value start and length value.
void get_ip_addr_mask_info(switch_ip_address_t ip_addr_mask,
                           uint32_t &p_start_bit,
                           uint32_t &p_length) {
  // Calculating start and length for IPV4 address
  if (ip_addr_mask.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4) {
    get_mask_info(ip_addr_mask.ip4, p_start_bit, p_length);
    return;
  }

  // Calculating start and length for IPV6 address
  uint32_t multibyte_start_bit = 0;
  uint32_t multibyte_length = 0;
  uint32_t start_bit = 0;
  uint32_t length = 0;
  bool setbit_set = false;
  for (int8_t cnt = (IPV6_LEN - 1); cnt >= 0; --cnt) {
    start_bit = 0;
    length = 0;
    get_mask_info(ip_addr_mask.ip6[cnt], start_bit, length);
    if (length > 0) {
      if (!setbit_set) {
        setbit_set = true;
        multibyte_start_bit += start_bit;
        multibyte_length = length;
        if (start_bit == 0 && length != CHAR_BIT) {
          break;
        } else {
          continue;
        }
      }
      if (start_bit == 0) {
        multibyte_length += length;
      }
      if (length != CHAR_BIT) {
        break;
      }
    } else {
      if (setbit_set) {
        break;
      }
      multibyte_start_bit += 8;
    }
  }
  p_start_bit = multibyte_start_bit;
  p_length = multibyte_length;
}

switch_status_t get_recirc_port_in_pipe(switch_object_id_t device_handle,
                                        bf_dev_pipe_t pipe,
                                        uint16_t &recirc_port) {
  std::vector<uint16_t> recirc_dev_port_list;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  status = switch_store::v_get(device_handle,
                               SWITCH_DEVICE_ATTR_RECIRC_DEV_PORT_LIST,
                               recirc_dev_port_list);
  CHECK_RET(status != SWITCH_STATUS_SUCCESS, status);
  status = SWITCH_STATUS_ITEM_NOT_FOUND;
  for (auto dev_port : recirc_dev_port_list) {
    if (DEV_PORT_TO_PIPE(dev_port) == pipe) {
      recirc_port = dev_port;
      return SWITCH_STATUS_SUCCESS;
    }
  }
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "Failed to find recirc port in pipe {} status {}",
               pipe,
               status);
  }
  return status;
}

int32_t switch_ip_addr_cmp(const switch_ip_address_t &ip_addr1,
                           const switch_ip_address_t &ip_addr2) {
  if (ip_addr1.addr_family < ip_addr2.addr_family) {
    return -1;
  } else if (ip_addr1.addr_family > ip_addr2.addr_family) {
    return 1;
  }

  if (ip_addr1.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4) {
    return memcmp(&ip_addr1.ip4, &ip_addr2.ip4, IPV4_LEN);
  }

  return memcmp(ip_addr1.ip6, ip_addr2.ip6, IPV6_LEN);
}

void switch_ip_prefix_to_ip_addr(const switch_ip_prefix_t &ip_prefix,
                                 switch_ip_address_t &ip_addr) {
  ip_addr.addr_family = ip_prefix.addr.addr_family;

  if (ip_prefix.addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4) {
    ip_addr.ip4 = ip_prefix.addr.ip4;

    if (ip_prefix.len >= SWITCH_IPV4_MAX_PREFIX_LEN) {
      return;
    }
    // IPv4 address is stored in host order so zero the hosts bits starting from
    // the least significant bytes
    ip_addr.ip4 &= ~((1 << (SWITCH_IPV4_MAX_PREFIX_LEN - ip_prefix.len)) - 1);

  } else if (ip_prefix.addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6) {
    memcpy(ip_addr.ip6, ip_prefix.addr.ip6, IPV6_LEN);

    if (ip_prefix.len >= SWITCH_IPV6_MAX_PREFIX_LEN) {
      return;
    }
    // Get number of bytes and bits holding prefix
    uint16_t bytes_num = ip_prefix.len / 8;
    uint16_t bits_num = ip_prefix.len % 8;

    ip_addr.ip6[bytes_num] &= (uint8_t) ~((1 << (8 - bits_num)) - 1);

    for (uint32_t i = bytes_num + 1; i < IPV6_LEN; i++) {
      ip_addr.ip6[i] = 0x00;
    }
  }
}

bool switch_ip_prefix_is_host_ip(const switch_ip_prefix_t &ip_prefix) {
  if (((ip_prefix.addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4) &&
       (ip_prefix.len == SWITCH_IPV4_MAX_PREFIX_LEN)) ||
      ((ip_prefix.addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6) &&
       (ip_prefix.len == SWITCH_IPV6_MAX_PREFIX_LEN))) {
    return true;
  }

  return false;
}

switch_status_t pal_status_xlate(bf_status_t bf_status) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  switch (bf_status) {
    case BF_SUCCESS:
      status = SWITCH_STATUS_SUCCESS;
      break;

    case BF_INVALID_ARG:
      status = SWITCH_STATUS_INVALID_PARAMETER;
      break;

    case BF_NO_SYS_RESOURCES:
      status = SWITCH_STATUS_INSUFFICIENT_RESOURCES;
      break;

    case BF_ALREADY_EXISTS:
      status = SWITCH_STATUS_ITEM_ALREADY_EXISTS;
      break;

    case BF_IN_USE:
      status = SWITCH_STATUS_RESOURCE_IN_USE;
      break;

    case BF_HW_COMM_FAIL:
      status = SWITCH_STATUS_HW_FAILURE;
      break;

    case BF_OBJECT_NOT_FOUND:
      status = SWITCH_STATUS_ITEM_NOT_FOUND;
      break;

    case BF_NOT_IMPLEMENTED:
      status = SWITCH_STATUS_NOT_IMPLEMENTED;
      break;

    default:
      status = SWITCH_STATUS_PD_FAILURE;
      break;
  }
  return status;
}

}  // namespace smi
