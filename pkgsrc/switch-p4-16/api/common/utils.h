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


#ifndef __COMMON_UTILS_H__
#define __COMMON_UTILS_H__

#include <mutex>
#include <bitset>

// top-level includes
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

#include "switch_tna/utils.h"

// switch specific includes
#include "model.h"

#include <bf_types/bf_types.h>

namespace smi {

#define DEFAULT_ACTION_MEMBER_ID 0xFFFFFFF

/*
 * SAI's valid range for weight is 1-100 and hardware supports
 * 0-1023 value.
 */
#define SWITCH_PD_WEIGHT(weight) (weight * 10)
#define SWITCH_PD_QUEUE_DEFAULT_DWRR_WEIGHT 1

#define MAX_MAC_MTU 10240
#define FOLDED_PIPELINE_INTERNAL_PIPE_PORT_IFG 0
#define FOLDED_PIPELINE_INTERNAL_PIPE_PORT_PREAMBLE_LEN 4

#define UNKNOWN_FEATURE "Unknown Feature"

#define SWITCH_MAX_BD 8192

const char eth_1s[ETH_LEN] = {-1, -1, -1, -1, -1, -1};
const char eth_0s[ETH_LEN] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
const char eth_mc[ETH_LEN] = {0x1, 0x0, 0x0, 0x0, 0x0, 0x0};
const char reserved_mac[ETH_LEN] = {
    1, -128, -62, 0, 0, 0};  // 01-80-C2-00-00-00
const char reserved_mac_mask[ETH_LEN] = {
    -1, -1, -1, -1, -1, -16};  // FF-FF-FF-FF-FF-F0
const char ipv6_0s[IPV6_LEN] = {0x0,
                                0x0,
                                0x0,
                                0x0,
                                0x0,
                                0x0,
                                0x0,
                                0x0,
                                0x0,
                                0x0,
                                0x0,
                                0x0,
                                0x0,
                                0x0,
                                0x0,
                                0x0};
const char ipv6_1s[IPV6_LEN] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
const char ipv6_mc[IPV6_LEN] = {-1,
                                0x0,
                                0x0,
                                0x0,
                                0x0,
                                0x0,
                                0x0,
                                0x0,
                                0x0,
                                0x0,
                                0x0,
                                0x0,
                                0x0,
                                0x0,
                                0x0,
                                0x0};

#define SWITCH_MAC_VALID(_mac)                                  \
  !(_mac.mac[0] == 0 && _mac.mac[1] == 0 && _mac.mac[2] == 0 && \
    _mac.mac[3] == 0 && _mac.mac[4] == 0 && _mac.mac[5] == 0)

#define SWITCH_MAC_COMPARE(mac1, mac2)                         \
  (mac1.mac[0] == mac2.mac[0] && mac1.mac[1] == mac2.mac[1] && \
   mac1.mac[2] == mac2.mac[2] && mac1.mac[3] == mac2.mac[3] && \
   mac1.mac[4] == mac2.mac[4] && mac1.mac[5] == mac2.mac[5])

// In the port rate buffer the rate values are arranged in a  fixed
// order that IN_OCTETS, IN_PKTS, OUT_OCTETS, OUT_PKTS.
typedef enum {
  BF_PORT_IN_OCTETS_RATE,
  BF_PORT_IN_PKTS_RATE,
  BF_PORT_OUT_OCTETS_RATE,
  BF_PORT_OUT_PKTS_RATE,
  BF_PORT_RATE_MAX_COUNTERS
} switch_port_rate_t;

struct hostif_info {
  int fd;
  uint64_t knet_hostif_handle;
};

class switchContext {
 private:
  switchContext() { l2_session = ::bfrt::BfRtSession::sessionCreate(); }

  typedef std::lock_guard<std::mutex> LOCK_GUARD;
  std::bitset<256> yid_bitmap;
  std::mutex yid_mtx;

  std::unordered_map<uint16_t, switch_object_id_t> dev_port_to_port_handle;
  std::mutex dp2p_mtx;

  std::mutex fp2dp_mtx;
  std::map<uint64_t, uint16_t> fp_idx_to_dev_port;
  std::map<uint16_t, uint64_t> dev_port_to_fp_idx;

  std::unordered_map<uint64_t, hostif_info> hostif_fd_map;
  std::mutex fd_mtx;
  bool pkt_drv_inited = false;

  idAllocator mirror_session_manager;
  std::mutex mirror_mtx;

  // Will Store the data following order
  // IN_OCTATE, IN_PKT, OUT_OCTATE, OUT_PKT
  using port_stat_list_t = std::vector<uint64_t>;

  // Port -> counter/rate values list
  using port_to_stat_t = std::vector<port_stat_list_t>;

  // dev_id -> port_to_rate_map_t
  using dev_to_port_list_t = std::vector<port_to_stat_t>;

  // Store most recent read counter values
  dev_to_port_list_t port_counter_val;

  // Store the computed rate for the port
  dev_to_port_list_t port_rate;

  std::mutex prs_mtx;

  // NAPT allocators
  idAllocator snapt_index_manager;
  idAllocator dnapt_index_manager;

  // Tunnel allocator
  idAllocator tunnel_index_manager;

  // Tunnel mapper allocator
  idAllocator tunnel_mapper_index_manager;

  // RID allocator
  idAllocator rid_manager;

  // pre.node index allocator
  idAllocator pre_node_manager;

  // Etype label allocator
  idAllocator etype_index_manager;

  // bfrt session for L2 operations (learning/aging)
  std::shared_ptr<::bfrt::BfRtSession> l2_session;

  // List of all "fold" tables in different pipes. Applicable only to AFP
  std::set<bf_rt_id_t> fold_table_ids;

  // List of all pipes which do not hold SwitchIngress. Applicable only to AFP
  std::set<bf_dev_pipe_t> switch_non_ingress_pipes;

  // List of all pipes containing ing_port_mapping. For AFP, this also means
  // the list of all pipes running SwitchIngress
  std::set<bf_dev_pipe_t> switch_ingress_pipes;

  // List of all pipes containing egress system ACL. For AFP, this also means
  // the list of all pipes running SwitchEgress
  std::set<bf_dev_pipe_t> switch_egress_pipes;

  // For certain cases BMAI manages port create/update/delete of ports on non
  // Switch pipelines. This flag specifies whether BMAI should or should not
  // manage these ports
  bool internal_pipe_ports_bmai_managed = false;

  // This flag indicates where the packet DMA processing happens in kernel or
  // not
  bool use_kpkt = false;

 public:
  std::unordered_map<uint64_t, switch_lpm_trie_t *> ipv4_tries;
  std::unordered_map<uint64_t, switch_lpm_trie_t *> ipv6_tries;
  static switchContext &context() {
    static switchContext instance;
    return instance;
  }

  switchContext(switchContext const &) = delete;
  void operator=(switchContext const &) = delete;

  ::bfrt::BfRtSession &get_l2_session() { return *l2_session; }
  std::shared_ptr<::bfrt::BfRtSession> get_l2_session_ptr() {
    return l2_session;
  }
  void update_yid(uint16_t yid, switch_object_type_t ot);
  uint16_t reserve_yid(switch_object_type_t ot);
  uint16_t alloc_yid(switch_object_id_t handle);
  void release_yid(switch_object_id_t handle);
  void clear_yid();

  void port_rate_create(const uint16_t dev_id, const uint32_t max_ports);
  void port_rate_reset(const uint16_t dev_id, const uint32_t port_id);
  void port_rate_get(uint16_t dev_id,
                     uint32_t port_id,
                     std::vector<uint64_t> &rate);
  void port_rate_update(uint16_t dev_id,
                        uint32_t port_id,
                        uint64_t counters_val[BF_PORT_RATE_MAX_COUNTERS]);

  void add_dev_port_to_port_handle(uint16_t dev_port,
                                   const switch_object_id_t object_id) {
    LOCK_GUARD guard(dp2p_mtx);
    dev_port_to_port_handle[dev_port] = object_id;
  }
  void clear_dev_port_to_port_handle(uint16_t dev_port) {
    LOCK_GUARD guard(dp2p_mtx);
    dev_port_to_port_handle.erase(dev_port);
  }
  switch_object_id_t find_dev_port_to_port_handle(uint16_t dev_port) {
    switch_object_id_t empty_handle = {0};
    LOCK_GUARD guard(dp2p_mtx);
    auto map_it = dev_port_to_port_handle.find(dev_port);
    if (map_it != dev_port_to_port_handle.end()) {
      return dev_port_to_port_handle[dev_port];
    }
    return empty_handle;
  }

  void add_fp_idx_to_dev_port(uint64_t fp_idx, uint16_t dev_port) {
    LOCK_GUARD guard(fp2dp_mtx);
    fp_idx_to_dev_port[fp_idx] = dev_port;
    dev_port_to_fp_idx[dev_port] = fp_idx;
  }

  switch_status_t get_dev_port_for_fp_idx(uint64_t fp_idx, uint16_t &dev_port) {
    LOCK_GUARD guard(fp2dp_mtx);
    auto map_it = fp_idx_to_dev_port.find(fp_idx);
    if (map_it != fp_idx_to_dev_port.end()) {
      dev_port = map_it->second;
      // dev_port = fp_idx_to_dev_port[fp_idx];
      return SWITCH_STATUS_SUCCESS;
    }
    return SWITCH_STATUS_ITEM_NOT_FOUND;
  }

  switch_status_t get_fp_idx_for_dev_port(uint16_t dev_port, uint64_t &fp_idx) {
    LOCK_GUARD guard(fp2dp_mtx);
    auto map_it = dev_port_to_fp_idx.find(dev_port);
    if (map_it != dev_port_to_fp_idx.end()) {
      fp_idx = map_it->second;
      // fp_idx = dev_port_to_fp_idx[dev_port];
      return SWITCH_STATUS_SUCCESS;
    }
    return SWITCH_STATUS_ITEM_NOT_FOUND;
  }

  uint32_t mirror_session_id_allocate() {
    LOCK_GUARD guard(mirror_mtx);
    return mirror_session_manager.allocate();
  }
  switch_status_t mirror_session_id_release(uint32_t sid) {
    LOCK_GUARD guard(mirror_mtx);
    return mirror_session_manager.release(sid);
  }
  void mirror_session_id_reserve(uint32_t sid) {
    LOCK_GUARD guard(mirror_mtx);
    mirror_session_manager.reserve(sid);
  }
  void add_hostif_fd_map(const switch_object_id_t hostif_handle,
                         int fd,
                         uint64_t knet_hostif_handle) {
    LOCK_GUARD guard(fd_mtx);
    hostif_fd_map[hostif_handle.data] = {fd, knet_hostif_handle};
  }
  void clear_hostif_fd_map(const switch_object_id_t hostif_handle) {
    LOCK_GUARD guard(fd_mtx);
    hostif_fd_map.erase(hostif_handle.data);
  }
  void find_hostif_fd_map(const switch_object_id_t hostif_handle,
                          int *fd,
                          uint64_t *knet_hostif_handle) {
    LOCK_GUARD guard(fd_mtx);
    *fd = -1;
    *knet_hostif_handle = 0;
    auto map_it = hostif_fd_map.find(hostif_handle.data);
    if (map_it != hostif_fd_map.end()) {
      *fd = map_it->second.fd;
      *knet_hostif_handle = map_it->second.knet_hostif_handle;
    }
    return;
  }
  void clean_hostif_fd_map() {
    LOCK_GUARD guard(fd_mtx);
    hostif_fd_map.clear();
  }
  const std::unordered_map<uint64_t, hostif_info> &get_hostif_fd_map() {
    return hostif_fd_map;
  }
  void set_pkt_drv_init(bool init) { pkt_drv_inited = init; }
  bool is_pkt_drv_init_done() { return pkt_drv_inited; }

  uint16_t snapt_index_allocate() { return snapt_index_manager.allocate(); }
  uint16_t snapt_index_reserve(uint16_t index) {
    return snapt_index_manager.reserve(index);
  }
  switch_status_t snapt_index_release(uint32_t index) {
    return snapt_index_manager.release(index);
  }
  uint16_t dnapt_index_allocate() { return dnapt_index_manager.allocate(); }
  uint16_t dnapt_index_reserve(uint16_t index) {
    return dnapt_index_manager.reserve(index);
  }
  switch_status_t dnapt_index_release(uint16_t index) {
    return dnapt_index_manager.release(index);
  }

  uint16_t tunnel_index_allocate() { return tunnel_index_manager.allocate(); }
  uint16_t tunnel_index_reserve(uint16_t index) {
    return tunnel_index_manager.reserve(index);
  }
  switch_status_t tunnel_index_release(uint16_t index) {
    return tunnel_index_manager.release(index);
  }

  uint16_t tunnel_mapper_index_allocate() {
    return tunnel_mapper_index_manager.allocate();
  }
  uint16_t tunnel_mapper_index_reserve(uint16_t index) {
    return tunnel_mapper_index_manager.reserve(index);
  }
  switch_status_t tunnel_mapper_index_release(uint16_t index) {
    return tunnel_mapper_index_manager.release(index);
  }

  // first 8k numbers are reserved for BDs
  uint16_t rid_allocate() { return rid_manager.allocate() + SWITCH_MAX_BD; }
  uint16_t rid_reserve(uint16_t index) {
    return rid_manager.reserve(index - SWITCH_MAX_BD);
  }
  switch_status_t rid_release(uint16_t index) {
    return rid_manager.release(index - SWITCH_MAX_BD);
  }

  uint32_t pre_node_allocate() { return pre_node_manager.allocate(); }
  uint32_t pre_node_reserve(uint32_t index) {
    return pre_node_manager.reserve(index);
  }
  switch_status_t pre_node_release(uint32_t index) {
    return pre_node_manager.release(index);
  }

  uint16_t etype_index_allocate() { return etype_index_manager.allocate(); }
  uint16_t etype_index_reserve(uint16_t index) {
    return etype_index_manager.reserve(index);
  }
  switch_status_t etype_index_release(uint16_t index) {
    return etype_index_manager.release(index);
  }

  const std::set<bf_rt_id_t> get_fold_table_ids() const {
    return fold_table_ids;
  }
  void append_fold_table_ids(bf_rt_id_t table_id) {
    fold_table_ids.insert(table_id);
  }
  void update_switch_non_ingress_pipe_list(bf_dev_pipe_t pipe_id) {
    switch_non_ingress_pipes.insert(pipe_id);
  }
  const std::set<bf_dev_pipe_t> get_switch_non_ingress_pipe_list() {
    return switch_non_ingress_pipes;
  }
  void update_switch_ingress_pipe_list(bf_dev_pipe_t pipe_id) {
    switch_ingress_pipes.insert(pipe_id);
  }
  const std::set<bf_dev_pipe_t> get_switch_ingress_pipe_list() {
    return switch_ingress_pipes;
  }
  void update_switch_egress_pipe_list(bf_dev_pipe_t pipe_id) {
    switch_egress_pipes.insert(pipe_id);
  }
  const std::set<bf_dev_pipe_t> get_switch_egress_pipe_list() {
    return switch_egress_pipes;
  }

  void set_internal_pipe_ports_bmai_managed(bool bmai_managed) {
    internal_pipe_ports_bmai_managed = bmai_managed;
  }
  bool is_internal_pipe_ports_bmai_managed() const {
    return internal_pipe_ports_bmai_managed;
  }

  void set_use_kpkt(bool enable) { use_kpkt = enable; }

  bool is_kpkt_enabled() const { return use_kpkt; }
};
#define SWITCH_CONTEXT switchContext::context()

typedef std::function<switch_status_t(const switch_object_id_t oid,
                                      switch_attribute_t attr)>
    object_cb;
switch_status_t execute_cb_for_all(switch_object_type_t ot,
                                   object_cb cb,
                                   switch_attribute_t attr);

switch_status_t hostif_init();
switch_status_t pal_init();
switch_status_t qos_pdfixed_init();

switch_status_t qos_pdfixed_clean();
switch_status_t pal_clean();
switch_status_t hostif_clean();

void update_port_an_state(const switch_object_id_t &port_handle,
                          const uint64_t port_id,
                          const bf_dev_id_t bf_dev_id,
                          const bf_dev_port_t bf_dev_port);
switch_status_t queue_tail_drop_set(switch_object_id_t port_handle,
                                    switch_object_id_t pfc_queue_qos_map_handle,
                                    uint32_t pfc_map);
switch_status_t compute_pd_buffer_bytes_to_cells(uint16_t dev_id,
                                                 uint64_t bytes_threshold,
                                                 uint32_t *cell_threshold);
switch_status_t compute_pd_buffer_cells_to_bytes(uint16_t dev_id,
                                                 uint32_t num_cells,
                                                 uint64_t *num_bytes);
switch_status_t switch_hash_alg_type_to_str(uint32_t algo_value,
                                            std::string &algo_name);
uint32_t switch_hash_alg_str_to_type(std::string algo_name);
// These are implementation specific depending on the P4
uint16_t compute_port_lag_index(switch_object_id_t handle);
uint16_t compute_bd(const switch_object_id_t bd_handle);
void get_mask_info(uint32_t mask, uint32_t &start_bit, uint32_t &length);
void get_mac_mask_info(switch_mac_addr_t mac_mask,
                       uint32_t &start_bit,
                       uint32_t &length);
void get_ip_addr_mask_info(switch_ip_address_t ip_addr_mask,
                           uint32_t &start_bit,
                           uint32_t &length);

switch_status_t create_default_buffer_profile(
    const switch_object_id_t device_handle,
    switch_object_id_t *ingress_profile,
    switch_object_id_t *egress_profile);

int32_t switch_ip_addr_cmp(const switch_ip_address_t &ip_addr1,
                           const switch_ip_address_t &ip_addr2);
void switch_ip_prefix_to_ip_addr(const switch_ip_prefix_t &ip_prefix,
                                 switch_ip_address_t &ip_addr);
bool switch_ip_prefix_is_host_ip(const switch_ip_prefix_t &ip_prefix);

// The following list of APIs have to be implemented by all architectures
switch_status_t switch_init(const char *model_json,
                            const char *cpu_port,
                            bool use_pcie,
                            bool override_log_level,
                            bool warm_init,
                            const char *const warm_init_file);

switch_status_t switch_clean(bool warm_shut, const char *const warm_shut_file);

switch_status_t table_info_get(switch_device_attr_table table_id,
                               switch_table_info_t &table_info);

switch_status_t hostif_start_packet_driver();
switch_status_t hostif_stop_packet_driver();
void sai_mode_set(bool set);
bool sai_mode();

class feature {
 public:
  static void init_features();
  static std::bitset<SWITCH_FEATURE_MAX> feature_bmap;
  static void clear_features();
  static bool is_feature_set(switch_feature_id_t feature);
  static void feature_set(switch_feature_id_t feature);
};

/**
 * For folded pipe scenario running switch.p4, the front port could be different
 * from the pipes in which the table is present. If given pipe is within the
 * list of pipes the table is present in, return the same value, else return the
 * corresponding internal pipe
 *
 * @param[in] front panel pipe
 * @param[in] Set of pipes table is present
 * @param[out] shifted pipe
 */
bf_dev_pipe_t SHIFT_PIPE_IF_FOLDED(bf_dev_pipe_t pipe,
                                   const std::set<bf_dev_pipe_t> &table_pipes);

/**
 * @brief Translates or Maps a Non Switch Ingress Pipe to Switch Ingress Pipe
 *
 * nth non-switch Ingress pipe is mapped to 'n%(#SwitchIngress Pipes)'th switch
 *Ingress pipe
 * (Pipe that contains the switch p4 ingress pipeline)
 *
 * @param[in] non_switch_pipe
 *
 * @return Corresponding Internal Pipe for External Pipe
 */
bf_dev_pipe_t translate_ingress_pipe(bf_dev_pipe_t external_pipe);

bf_dev_pipe_t translate_egress_pipe(bf_dev_pipe_t internal_pipe);

/**
 * @brief Translate Ingress Port
 *
 * When the SwitchIngress Pipeline exists on an internal pipe, the external dev
 *port needs to be converted to
 * SwitchIngress Pipeline Specific devport. For example Port 0 on External Pipe
 *0 can be converted to Port 0 on
 * SwitchIngress Pipe Pipe 3. This function helps in translating the port
 *assuming the port-id remains the same across
 * the pipe.
 * Converts an external (front panel port facing pipe) to corresponding internal
 * pipe (Pipe that contains the switch p4 ingress pipeline) using @ref
 * translate_ingress_pipe
 *
 * @param[in] external_port External Port
 *
 * @return Corresponding Internal Port for External Port
 */
uint16_t translate_ingress_port(uint16_t external_dev_port);

/**
 * @brief Translate Egress Port
 *
 * When a port being added lies on an internal pipe, the port needs to be
 *translated to corresponding port in Switch
 * Egress pipe. This function helps convert this internal Pipe port to Switch
 *Egress Pipe Port.
 * Converts an internal Pipe Port to corresponding external
 * pipe (Pipe that contains the switch p4 egress pipeline) using @ref
 * translate_egress_pipe
 *
 * @param[in] external_port External Port
 *
 * @return Corresponding External Port for Internal Port
 */
uint16_t translate_egress_port(uint16_t external_dev_port);

/**
 * @brief Get Pipe for Dev Port
 *
 * For normal (non folded) pipeline case pipe is derived directly from the dev
 *port. Whereas in case of asymmetric folded pipeline the pipe is translated
 *from the pipe on which the dev port is to a different pipe using @ref
 *translate_ingresss_pipe. This function is not meant to be used for egress
 *asymmetric
 *tables, since the egress pipe is always the external pipe in asymmetric folded
 *pipeline.
 *
 * @param[in] dev_port Device port
 *
 * @return Pipe for in dev_port
 */
inline bf_dev_pipe_t INGRESS_DEV_PORT_TO_PIPE(uint16_t dev_port) {
  auto pipe = DEV_PORT_TO_PIPE(dev_port);
  if (feature::is_feature_set(SWITCH_FEATURE_ASYMMETRIC_FOLDED_PIPELINE)) {
    const auto &switch_ingress_pipes =
        SWITCH_CONTEXT.get_switch_ingress_pipe_list();
    // Translate Pipe if the port lies on Non SwitchIngress Pipe
    if (std::find(switch_ingress_pipes.begin(),
                  switch_ingress_pipes.end(),
                  pipe) == switch_ingress_pipes.end()) {
      return smi::translate_ingress_pipe(pipe);
    }
  }
  return pipe;
}

inline bf_dev_pipe_t EGRESS_DEV_PORT_TO_PIPE(uint16_t dev_port) {
  auto pipe = DEV_PORT_TO_PIPE(dev_port);
  if (feature::is_feature_set(SWITCH_FEATURE_ASYMMETRIC_FOLDED_PIPELINE)) {
    const auto &switch_egress_pipes =
        SWITCH_CONTEXT.get_switch_egress_pipe_list();
    // Translate Pipe if the port lies on an internal pipe
    if (unlikely(std::find(switch_egress_pipes.begin(),
                           switch_egress_pipes.end(),
                           pipe) == switch_egress_pipes.end())) {
      return smi::translate_egress_pipe(pipe);
    }
  }
  return pipe;
}

inline bool check_port_in_switch_pipe(uint16_t dev_port) {
  auto pipe = DEV_PORT_TO_PIPE(dev_port);
  const auto &switch_egress_pipes =
      SWITCH_CONTEXT.get_switch_egress_pipe_list();
  if (std::find(switch_egress_pipes.begin(), switch_egress_pipes.end(), pipe) ==
      switch_egress_pipes.end()) {
    return false;
  } else {
    return true;
  }
}

switch_status_t get_recirc_port_in_pipe(switch_object_id_t device_handle,
                                        bf_dev_pipe_t pipe,
                                        uint16_t &recirc_port);

switch_status_t pal_status_xlate(bf_status_t bf_status);
}  // namespace smi

// endsWith()
//
// Returns whether a given string `text` ends with `suffix`.
inline bool endsWith(const std::string &text,
                     const std::string &suffix) noexcept {
  return suffix.empty() || (text.size() >= suffix.size() &&
                            memcmp(text.data() + (text.size() - suffix.size()),
                                   suffix.data(),
                                   suffix.size()) == 0);
}

// beginsWith()
//
// Returns whether a given string `text` begins with `prefix`.
inline bool beginsWith(const std::string &text,
                       const std::string &prefix) noexcept {
  return prefix.empty() ||
         (text.size() >= prefix.size() &&
          memcmp(text.data(), prefix.data(), prefix.size()) == 0);
}

inline const std::string bf_switch_cli_feature_name_str(
    switch_feature_id_t feature) {
  const char *type_str;
  switch (feature) {
    case SWITCH_FEATURE_ACL_PORT_GROUP:
      return "ACL_PORT_GROUP";
    case SWITCH_FEATURE_ACL_REDIRECT_NEXTHOP:
      return "ACL_REDIRECT_NEXTHOP";
    case SWITCH_FEATURE_ACL_REDIRECT_PORT:
      return "ACL_REDIRECT_PORT";
    case SWITCH_FEATURE_ACL_USER_META:
      return "ACL_USER_META";
    case SWITCH_FEATURE_ASYMMETRIC_FOLDED_PIPELINE:
      return "ASYMMETRIC_FOLDED_PIPELINE";
    case SWITCH_FEATURE_BASIC_NAT:
      return "BASIC_NAT";
    case SWITCH_FEATURE_BD_IN_EGRESS_ACL:
      return "BD_IN_EGRESS_ACL";
    case SWITCH_FEATURE_BD_IN_INGRESS_ACL:
      return "BD_IN_INGRESS_ACL";
    case SWITCH_FEATURE_BD_LABEL_IN_EGRESS_ACL:
      return "BD_LABEL_IN_EGRESS_ACL";
    case SWITCH_FEATURE_BD_LABEL_IN_INGRESS_ACL:
      return "BD_LABEL_IN_INGRESS_ACL";
    case SWITCH_FEATURE_COPP:
      return "COPP";
    case SWITCH_FEATURE_CPU_BD_MAP:
      return "CPU_BD_MAP";
    case SWITCH_FEATURE_DROP_REPORT:
      return "DROP_REPORT";
    case SWITCH_FEATURE_DTEL_IFA_CLONE:
      return "DTEL_IFA_CLONE";
    case SWITCH_FEATURE_DTEL_IFA_EDGE:
      return "DTEL_IFA_EDGE";
    case SWITCH_FEATURE_ECMP_DEFAULT_HASH_OFFSET:
      return "ECMP_DEFAULT_HASH_OFFSET";
    case SWITCH_FEATURE_ECN_ACL:
      return "ECN_ACL";
    case SWITCH_FEATURE_EGRESS_ACL_METER:
      return "EGRESS_ACL_METER";
    case SWITCH_FEATURE_EGRESS_COPP:
      return "EGRESS_COPP";
    case SWITCH_FEATURE_EGRESS_IP_ACL:
      return "EGRESS_IP_ACL";
    case SWITCH_FEATURE_EGRESS_IP_QOS_ACL:
      return "EGRESS_IP_QOS_ACL";
    case SWITCH_FEATURE_EGRESS_L4_PORT_RANGE:
      return "EGRESS_L4_PORT_RANGE";
    case SWITCH_FEATURE_EGRESS_MAC_ACL:
      return "EGRESS_MAC_ACL";
    case SWITCH_FEATURE_EGRESS_MAC_QOS_ACL:
      return "EGRESS_MAC_QOS_ACL";
    case SWITCH_FEATURE_EGRESS_MIRROR_ACL:
      return "EGRESS_MIRROR_ACL";
    case SWITCH_FEATURE_EGRESS_MIRROR_ACL_MIRROR_IN_OUT:
      return "EGRESS_MIRROR_ACL_MIRROR_IN_OUT";
    case SWITCH_FEATURE_EGRESS_PORT_METER:
      return "EGRESS_PORT_METER";
    case SWITCH_FEATURE_EGRESS_PORT_MIRROR:
      return "EGRESS_PORT_MIRROR";
    case SWITCH_FEATURE_EGRESS_TOS_MIRROR_ACL:
      return "EGRESS_TOS_MIRROR_ACL";
    case SWITCH_FEATURE_RSPAN:
      return "RSPAN";
    case SWITCH_FEATURE_ERSPAN_PLATFORM_INFO:
      return "ERSPAN_PLATFORM_INFO";
    case SWITCH_FEATURE_ERSPAN_TYPE2:
      return "ERSPAN_TYPE2";
    case SWITCH_FEATURE_ETRAP:
      return "ETRAP";
    case SWITCH_FEATURE_ETYPE_IN_ACL:
      return "ETYPE_IN_ACL";
    case SWITCH_FEATURE_FIB_ACL_LABEL:
      return "FIB_ACL_LABEL";
    case SWITCH_FEATURE_FLOW_NAT:
      return "FLOW_NAT";
    case SWITCH_FEATURE_FLOW_REPORT:
      return "FLOW_REPORT";
    case SWITCH_FEATURE_FOLDED_SWITCH_PIPELINE:
      return "FOLDED_SWITCH_PIPELINE";
    case SWITCH_FEATURE_INGRESS_ACL_METER:
      return "INGRESS_ACL_METER";
    case SWITCH_FEATURE_INGRESS_IP_QOS_ACL:
      return "INGRESS_IP_QOS_ACL";
    case SWITCH_FEATURE_INGRESS_IPV4_ACL:
      return "INGRESS_IPV4_ACL";
    case SWITCH_FEATURE_INGRESS_IPV4_RACL:
      return "INGRESS_IPV4_RACL";
    case SWITCH_FEATURE_INGRESS_IPV6_ACL:
      return "INGRESS_IPV6_ACL";
    case SWITCH_FEATURE_INGRESS_IPV6_RACL:
      return "INGRESS_IPV6_RACL";
    case SWITCH_FEATURE_INGRESS_L4_PORT_RANGE:
      return "INGRESS_L4_PORT_RANGE";
    case SWITCH_FEATURE_INGRESS_MAC_ACL:
      return "INGRESS_MAC_ACL";
    case SWITCH_FEATURE_INGRESS_MAC_IP_ACL_DENY_ACTION:
      return "INGRESS_MAC_IP_ACL_DENY_ACTION";
    case SWITCH_FEATURE_INGRESS_MAC_QOS_ACL:
      return "INGRESS_MAC_QOS_ACL";
    case SWITCH_FEATURE_INGRESS_MIRROR_ACL:
      return "INGRESS_MIRROR_ACL";
    case SWITCH_FEATURE_INGRESS_MIRROR_ACL_MIRROR_IN_OUT:
      return "INGRESS_MIRROR_ACL_MIRROR_IN_OUT";
    case SWITCH_FEATURE_INGRESS_PORT_METER:
      return "INGRESS_PORT_METER";
    case SWITCH_FEATURE_INGRESS_PORT_MIRROR:
      return "INGRESS_PORT_MIRROR";
    case SWITCH_FEATURE_INGRESS_QOS_MAP:
      return "INGRESS_QOS_MAP";
    case SWITCH_FEATURE_INGRESS_RACL:
      return "INGRESS_RACL";
    case SWITCH_FEATURE_INGRESS_TOS_MIRROR_ACL:
      return "INGRESS_TOS_MIRROR_ACL";
    case SWITCH_FEATURE_INNER_DTEL:
      return "INNER_DTEL";
    case SWITCH_FEATURE_INNER_HASH:
      return "INNER_HASH";
    case SWITCH_FEATURE_INNER_L2:
      return "INNER_L2";
    case SWITCH_FEATURE_IN_PORTS_IN_DATA:
      return "IN_PORTS_IN_DATA";
    case SWITCH_FEATURE_IN_PORTS_IN_MIRROR:
      return "IN_PORTS_IN_MIRROR";
    case SWITCH_FEATURE_OUT_PORTS:
      return "OUT_PORTS";
    case SWITCH_FEATURE_INT_V2:
      return "INT_V2";
    case SWITCH_FEATURE_IP_STATS:
      return "IP_STATS";
    case SWITCH_FEATURE_IPGRE:
      return "IPGRE";
    case SWITCH_FEATURE_IPINIP:
      return "IPINIP";
    case SWITCH_FEATURE_IPV4_LOCAL_HOST:
      return "IPV4_LOCAL_HOST";
    case SWITCH_FEATURE_IPV4_TUNNEL:
      return "IPV4_TUNNEL";
    case SWITCH_FEATURE_IPV6_ACL_UPPER64:
      return "IPV6_ACL_UPPER64";
    case SWITCH_FEATURE_IPV6_FIB_LPM_TCAM:
      return "IPV6_FIB_LPM_TCAM";
    case SWITCH_FEATURE_IPV6_HOST64:
      return "IPV6_HOST64";
    case SWITCH_FEATURE_IPV6_LPM64:
      return "IPV6_LPM64";
    case SWITCH_FEATURE_IPV6_TUNNEL:
      return "IPV6_TUNNEL";
    case SWITCH_FEATURE_L2_VXLAN:
      return "L2_VXLAN";
    case SWITCH_FEATURE_L3_UNICAST_SELF_FWD_CHECK:
      return "L3_UNICAST_SELF_FWD_CHECK";
    case SWITCH_FEATURE_MAC_PKT_CLASSIFICATION:
      return "MAC_PKT_CLASSIFICATION";
    case SWITCH_FEATURE_MIRROR_METER:
      return "MIRROR_METER";
    case SWITCH_FEATURE_MLAG:
      return "MLAG";
    case SWITCH_FEATURE_MPLS:
      return "MPLS";
    case SWITCH_FEATURE_MSTP:
      return "MSTP";
    case SWITCH_FEATURE_MULTICAST:
      return "MULTICAST";
    case SWITCH_FEATURE_MULTIPLE_RIFS_PER_PORT:
      return "MULTIPLE_RIFS_PER_PORT";
    case SWITCH_FEATURE_NAPT:
      return "NAPT";
    case SWITCH_FEATURE_NAT:
      return "NAT";
    case SWITCH_FEATURE_NAT_NAPT_INDEX_INDIRECTION:
      return "NAT_NAPT_INDEX_INDIRECTION";
    case SWITCH_FEATURE_PFC_WD:
      return "PFC_WD";
    case SWITCH_FEATURE_PORT_ISOLATION:
      return "PORT_ISOLATION";
    case SWITCH_FEATURE_PEER_LINK_TUNNEL_ISOLATION:
      return "PEER_LINK_TUNNEL_ISOLATION";
    case SWITCH_FEATURE_PPG_STATS:
      return "PPG_STATS";
    case SWITCH_FEATURE_PRE_INGRESS_ACL:
      return "PRE_INGRESS_ACL";
    case SWITCH_FEATURE_PTP:
      return "PTP";
    case SWITCH_FEATURE_QINQ:
      return "QINQ";
    case SWITCH_FEATURE_QINQ_RIF:
      return "QINQ_RIF";
    case SWITCH_FEATURE_QUEUE_REPORT:
      return "QUEUE_REPORT";
    case SWITCH_FEATURE_REPORT_SUPPRESSION:
      return "REPORT_SUPPRESSION";
    case SWITCH_FEATURE_SAME_MAC_CHECK:
      return "SAME_MAC_CHECK";
    case SWITCH_FEATURE_SFC:
      return "SFC";
    case SWITCH_FEATURE_SFLOW:
      return "SFLOW";
    case SWITCH_FEATURE_SHARED_ALPM:
      return "SHARED_ALPM";
    case SWITCH_FEATURE_SHARED_INGRESS_IP_ACL:
      return "SHARED_INGRESS_IP_ACL";
    case SWITCH_FEATURE_SRV6:
      return "SRV6";
    case SWITCH_FEATURE_STORM_CONTROL:
      return "STORM_CONTROL";
    case SWITCH_FEATURE_STP:
      return "STP";
    case SWITCH_FEATURE_TCP_FLAGS_LOU:
      return "TCP_FLAGS_LOU";
    case SWITCH_FEATURE_TUNNEL_DECAP:
      return "TUNNEL_DECAP";
    case SWITCH_FEATURE_TUNNEL_ECN_RFC_6040:
      return "TUNNEL_ECN_RFC_6040";
    case SWITCH_FEATURE_TUNNEL_ENCAP:
      return "TUNNEL_ENCAP";
    case SWITCH_FEATURE_TUNNEL_QOS_MODE:
      return "TUNNEL_QOS_MODE";
    case SWITCH_FEATURE_TUNNEL_TTL_MODE:
      return "TUNNEL_TTL_MODE";
    case SWITCH_FEATURE_UDT_TYPE_NEIGHBOR:
      return "UDT_TYPE_NEIGHBOR";
    case SWITCH_FEATURE_VXLAN:
      return "VXLAN";
    case SWITCH_FEATURE_WRED:
      return "WRED";
    case SWITCH_FEATURE_INGRESS_MAC_IP_ACL_TRANSIT_ACTION:
      return "INGRESS_MAC_IP_ACL_TRANSIT_ACTION";
    case SWITCH_FEATURE_IPMC_DMAC_VALIDATION:
      return "IPMC_DMAC_VALIDATION";
    case SWITCH_FEATURE_BFD_OFFLOAD:
      return "BFD_OFFLOAD";
    case SWITCH_FEATURE_PKTGEN:
      return "PKTGEN";
    default:
      type_str = UNKNOWN_FEATURE;
      break;
  }

  return type_str;
}

#endif  // __COMMON_UTILS_H__
