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


#include "switch_tna/bf_rt_ids.h"

#include <unordered_map>
#include <string>
#include <vector>

#include "switch_tna/utils.h"

namespace smi {
namespace bf_rt {
using ::smi::logging::switch_log;
using ::bfrt::BfRtTable;

namespace {
std::string tableNameGetInternal(const BfRtTable *table) {
  std::string name;
  auto status = table->tableNameGet(&name);
  if (status != BF_SUCCESS) {
    return "";
  }
  return name;
}
}  // namespace

/* This is just for logging sake and because bfrt does not have id to name */
std::unordered_map<bf_rt_action_id_t, std::string> action_id_to_name;
std::unordered_map<bf_rt_table_id_t, std::string> table_id_to_name;

static inline bf_rt_table_id_t table_id_from_fqn(const std::string &table_fqn) {
  const BfRtTable *table = NULL;
  bf_rt_table_id_t table_id = 0;
  smi_id::rt_info->bfrtTableFromNameGet(table_fqn, &table);
  if (table == NULL) {
    switch_log(SWITCH_API_LEVEL_INFO,
               SWITCH_OBJECT_TYPE_NONE,
               "{}: {} not present in this switch profile",
               __func__,
               table_fqn);
    return 0;
  }
  table->tableIdGet(&table_id);

  if (table_id == 0) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "{}: Table ID get fail for {}",
               __func__,
               table_fqn);
    return 0;
  }
  table_id_to_name[table_id] = table_fqn;
  return table_id;
}

bf_rt_table_id_t table_id_from_name(const std::string &name) {
  return table_id_from_fqn(name);
}

bf_rt_field_id_t key_id_from_name(bf_rt_table_id_t table_id,
                                  const std::string &name) {
  bf_status_t status = BF_SUCCESS;
  if (table_id == 0) return 0;

  const BfRtTable *table = NULL;
  status = smi_id::rt_info->bfrtTableFromIdGet(table_id, &table);
  if (table == NULL) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "{}: Table not found for table_id {} for key {}",
               __func__,
               table_id,
               name);
    return 0;
  }
  bf_rt_field_id_t field = 0;
  status = table->keyFieldIdGet(name, &field);
  if (status != BF_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_WARN,
               SWITCH_OBJECT_TYPE_NONE,
               "{}: Key ID not found\n"
               "    table  \"{}\"\n"
               "    key    \"{}\"",
               __func__,
               tableNameGetInternal(table),
               name);
    return 0;
  }
  return field;
}

size_t key_size_from_id(bf_rt_table_id_t table_id, bf_rt_field_id_t field_id) {
  bf_status_t status = BF_SUCCESS;
  if (table_id == 0) return 0;
  if (field_id == 0) return 0;

  const BfRtTable *table = NULL;
  status = smi_id::rt_info->bfrtTableFromIdGet(table_id, &table);
  if (table == NULL) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "{}: Table not found for table_id {}",
               __func__,
               table_id);
    return 0;
  }
  size_t field_size = 0;
  status = table->keyFieldSizeGet(field_id, &field_size);
  if (status != BF_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_WARN,
               SWITCH_OBJECT_TYPE_NONE,
               "{}: Key field size get failed table {} field_id {}",
               __func__,
               tableNameGetInternal(table),
               field_id);
    return 0;
  }
  return field_size;
}

bf_rt_action_id_t action_id_from_name(bf_rt_table_id_t table_id,
                                      const std::string &name) {
  bf_status_t status = BF_SUCCESS;
  if (table_id == 0) return 0;

  const BfRtTable *table = NULL;
  status = smi_id::rt_info->bfrtTableFromIdGet(table_id, &table);
  if (table == NULL) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "{}: Table not found for table_id {} for action {}",
               __func__,
               table_id,
               name);
    return 0;
  }
  bf_rt_action_id_t action_id = 0;
  status = table->actionIdGet(name, &action_id);
  if (status != BF_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_WARN,
               SWITCH_OBJECT_TYPE_NONE,
               "{}: Action ID not found\n"
               "    table  \"{}\"\n"
               "    action \"{}\"",
               __func__,
               tableNameGetInternal(table),
               name);
    return 0;
  }
  action_id_to_name[action_id] = name;
  return action_id;
}

bf_rt_field_id_t data_id_from_name(bf_rt_table_id_t table_id,
                                   bf_rt_action_id_t action_id,
                                   const std::string &name) {
  bf_status_t status = BF_SUCCESS;
  if (table_id == 0 || action_id == 0) return 0;

  const BfRtTable *table = NULL;
  status = smi_id::rt_info->bfrtTableFromIdGet(table_id, &table);
  if (table == NULL) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "{}: Table not found for table_id {}, action_id {}, name {}",
               __func__,
               table_id,
               action_id,
               name);
    return 0;
  }
  bf_rt_field_id_t field = 0;
  status = table->dataFieldIdGet(name, action_id, &field);
  if (status != BF_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_WARN,
               SWITCH_OBJECT_TYPE_NONE,
               "{}: Data field not found\n"
               "    table  \"{}\"\n"
               "    action \"{}\"\n"
               "    data   \"{}\"",
               __func__,
               tableNameGetInternal(table),
               action_id_to_name[action_id],
               name);
    return 0;
  }
  return field;
}

bf_rt_field_id_t data_id_from_name_container(bf_rt_table_id_t table_id,
                                             bf_rt_field_id_t field_id,
                                             std::string name) {
  bf_status_t status = BF_SUCCESS;
  if (table_id == 0) return 0;

  const BfRtTable *table = NULL;
  status = smi_id::rt_info->bfrtTableFromIdGet(table_id, &table);
  if (table == NULL) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "{}: Table not found for table_id {}, name {}",
               __func__,
               table_id,
               name);
    return 0;
  }
  std::vector<bf_rt_id_t> data_id_list;
  status = table->containerDataFieldIdListGet(field_id, &data_id_list);
  if (status != BF_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "{}: Data field not found\n"
               "    table  \"{}\"\n"
               "    container id   \"{}\"",
               __func__,
               tableNameGetInternal(table),
               field_id);
    return 0;
  }

  for (auto data_id : data_id_list) {
    std::string data_name;
    status = table->dataFieldNameGet(data_id, &data_name);
    if (status != BF_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_NONE,
                 "{}: Data field name not found\n"
                 "    table  \"{}\"\n"
                 "    data_id  \"{}\"",
                 __func__,
                 tableNameGetInternal(table),
                 data_id);
      return 0;
    }

    if (0 == name.compare(data_name)) {
      return data_id;
    }
  }
  return 0;
}

bf_rt_field_id_t data_id_from_name_noaction(bf_rt_table_id_t table_id,
                                            const std::string &name) {
  bf_status_t status = BF_SUCCESS;
  if (table_id == 0) return 0;

  const BfRtTable *table = NULL;
  status = smi_id::rt_info->bfrtTableFromIdGet(table_id, &table);
  if (table == NULL) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "{}: Table not found for table_id {}, name {}",
               __func__,
               table_id,
               name);
    return 0;
  }
  bf_rt_field_id_t field = 0;
  status = table->dataFieldIdGet(name, &field);
  if (status != BF_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_WARN,
               SWITCH_OBJECT_TYPE_NONE,
               "{}: Data field not found\n"
               "    table  \"{}\"\n"
               "    data   \"{}\"",
               __func__,
               tableNameGetInternal(table),
               name);
    return 0;
  }
  return field;
}

void smi_id::init_bf_rt_ids() {
  smi_id::rt_info = get_bf_rt_info();

  bf_sys_log_level_set(BF_MOD_BFRT, BF_LOG_DEST_STDOUT, BF_LOG_CRIT);
  bf_sys_log_level_set(BF_MOD_SWITCHAPI, BF_LOG_DEST_STDOUT, BF_LOG_CRIT);
  bf_sys_syslog_level_set(BF_LOG_CRIT);

  ENTER();

  switch_log(SWITCH_API_LEVEL_INFO,
             SWITCH_OBJECT_TYPE_NONE,
             "\n#################### Start ID retrieval from BF-Runtime "
             "####################\n");

  // pd fixed
  T_PRE_MGID = table_id_from_fqn("$pre.mgid");
  F_PRE_MGID_MGID = key_id_from_name(T_PRE_MGID, "$MGID");
  D_PRE_MGID_MULTICAST_NODE_ID =
      data_id_from_name_noaction(T_PRE_MGID, "$MULTICAST_NODE_ID");
  D_PRE_MGID_MULTICAST_NODE_L1_XID =
      data_id_from_name_noaction(T_PRE_MGID, "$MULTICAST_NODE_L1_XID");
  D_PRE_MGID_MULTICAST_NODE_L1_XID_VALID =
      data_id_from_name_noaction(T_PRE_MGID, "$MULTICAST_NODE_L1_XID_VALID");

  T_PRE_NODE = table_id_from_fqn("$pre.node");
  F_PRE_NODE_MULTICAST_NODE_ID =
      key_id_from_name(T_PRE_NODE, "$MULTICAST_NODE_ID");
  D_PRE_NODE_MULTICAST_RID =
      data_id_from_name_noaction(T_PRE_NODE, "$MULTICAST_RID");
  D_PRE_NODE_MULTICAST_LAG_ID =
      data_id_from_name_noaction(T_PRE_NODE, "$MULTICAST_LAG_ID");
  D_PRE_NODE_DEV_PORT = data_id_from_name_noaction(T_PRE_NODE, "$DEV_PORT");

  T_PRE_LAG = table_id_from_fqn("$pre.lag");
  F_PRE_LAG_LAG_ID = key_id_from_name(T_PRE_LAG, "$MULTICAST_LAG_ID");
  D_PRE_LAG_DEV_PORT = data_id_from_name_noaction(T_PRE_LAG, "$DEV_PORT");

  T_PRE_PRUNE = table_id_from_fqn("$pre.prune");
  F_PRE_PRUNE_YID = key_id_from_name(T_PRE_PRUNE, "$MULTICAST_L2_XID");
  D_PRE_PRUNE_DEV_PORT = data_id_from_name_noaction(T_PRE_PRUNE, "$DEV_PORT");

  // bfrt
  T_TM_CFG = table_id_from_fqn("tm.cfg");
  D_TM_CFG_CELL_SIZE = data_id_from_name_noaction(T_TM_CFG, "cell_size_bytes");
  D_TM_CFG_TOTAL_CELLS = data_id_from_name_noaction(T_TM_CFG, "total_cells");

  T_TM_PPG_CFG = table_id_from_fqn("tm.ppg.cfg");
  D_TM_PPG_CFG_GUARANTEED_CELLS =
      data_id_from_name_noaction(T_TM_PPG_CFG, "guaranteed_cells");

  T_TM_POOL_CFG = table_id_from_fqn("tm.pool.cfg");
  F_TM_POOL_CFG_POOL = key_id_from_name(T_TM_POOL_CFG, "pool");
  D_TM_POOL_CFG_SIZE_CELLS =
      data_id_from_name_noaction(T_TM_POOL_CFG, "size_cells");

  // tm.counter.ig.port
  T_TM_COUNTER_IG_PORT = table_id_from_fqn("tm.counter.ig.port");
  F_TM_COUNTER_IG_PORT_DEV_PORT =
      key_id_from_name(T_TM_COUNTER_IG_PORT, "dev_port");
  D_TM_COUNTER_IG_PORT_DROP_COUNT =
      data_id_from_name_noaction(T_TM_COUNTER_IG_PORT, "drop_count_packets");
  D_TM_COUNTER_IG_PORT_USAGE_CELLS =
      data_id_from_name_noaction(T_TM_COUNTER_IG_PORT, "usage_cells");
  D_TM_COUNTER_IG_PORT_WATERMARK_CELLS =
      data_id_from_name_noaction(T_TM_COUNTER_IG_PORT, "watermark_cells");

  // tm.counter.eg.port
  T_TM_COUNTER_EG_PORT = table_id_from_fqn("tm.counter.eg.port");
  F_TM_COUNTER_EG_PORT_DEV_PORT =
      key_id_from_name(T_TM_COUNTER_EG_PORT, "dev_port");
  D_TM_COUNTER_EG_PORT_DROP_COUNT =
      data_id_from_name_noaction(T_TM_COUNTER_EG_PORT, "drop_count_packets");
  D_TM_COUNTER_EG_PORT_USAGE_CELLS =
      data_id_from_name_noaction(T_TM_COUNTER_EG_PORT, "usage_cells");
  D_TM_COUNTER_EG_PORT_WATERMARK_CELLS =
      data_id_from_name_noaction(T_TM_COUNTER_EG_PORT, "watermark_cells");

  T_TM_POOL_APP_PFC = table_id_from_fqn("tm.pool.app_pfc");
  F_TM_POOL_APP_PFC_POOL = key_id_from_name(T_TM_POOL_APP_PFC, "pool");
  F_TM_POOL_APP_PFC_COS = key_id_from_name(T_TM_POOL_APP_PFC, "cos");
  D_TM_POOL_APP_PFC_LIMIT_CELLS =
      data_id_from_name_noaction(T_TM_POOL_APP_PFC, "pfc_limit_cells");

  T_TM_COUNTER_POOL = table_id_from_fqn("tm.counter.pool");
  F_TM_COUNTER_POOL = key_id_from_name(T_TM_COUNTER_POOL, "pool");
  D_TM_COUNTER_POOL_WATERMARK_CELLS =
      data_id_from_name_noaction(T_TM_COUNTER_POOL, "watermark_cells");
  D_TM_COUNTER_POOL_USAGE_CELLS =
      data_id_from_name_noaction(T_TM_COUNTER_POOL, "usage_cells");

  // TM port buffer
  T_TM_PORT_BUFFER = table_id_from_fqn("tm.port.buffer");
  F_TM_PORT_BUFFER_DEV_PORT = key_id_from_name(T_TM_PORT_BUFFER, "dev_port");
  D_TM_PORT_BUFFER_SKID_LIMIT_CELLS =
      data_id_from_name_noaction(T_TM_PORT_BUFFER, "skid_limit_cells");

  // tm.port.sched_shaping
  T_TM_PORT_SCHED_SHAPING = table_id_from_fqn("tm.port.sched_shaping");
  F_TM_PORT_SCHED_SHAPING_DEV_PORT =
      key_id_from_name(T_TM_PORT_SCHED_SHAPING, "dev_port");
  D_TM_PORT_SCHED_SHAPING_UNIT =
      data_id_from_name_noaction(T_TM_PORT_SCHED_SHAPING, "unit");
  D_TM_PORT_SCHED_SHAPING_PROVISIONING =
      data_id_from_name_noaction(T_TM_PORT_SCHED_SHAPING, "provisioning");
  D_TM_PORT_SCHED_SHAPING_MAX_RATE =
      data_id_from_name_noaction(T_TM_PORT_SCHED_SHAPING, "max_rate");
  D_TM_PORT_SCHED_SHAPING_MAX_BURST_SIZE =
      data_id_from_name_noaction(T_TM_PORT_SCHED_SHAPING, "max_burst_size");

  // tm.port.flowcontrol
  T_TM_PORT_FLOWCONTROL = table_id_from_fqn("tm.port.flowcontrol");
  F_TM_PORT_FLOWCONTROL_DEV_PORT =
      key_id_from_name(T_TM_PORT_FLOWCONTROL, "dev_port");
  D_TM_PORT_FLOWCONTROL_MODE_TX =
      data_id_from_name_noaction(T_TM_PORT_FLOWCONTROL, "mode_tx");
  D_TM_PORT_FLOWCONTROL_MODE_RX =
      data_id_from_name_noaction(T_TM_PORT_FLOWCONTROL, "mode_rx");
  D_TM_PORT_FLOWCONTROL_COS_TO_ICOS =
      data_id_from_name_noaction(T_TM_PORT_FLOWCONTROL, "cos_to_icos");

  // tm.port.sched_cfg
  T_TM_PORT_SCHED_CFG = table_id_from_fqn("tm.port.sched_cfg");
  F_TM_PORT_SCHED_CFG_DEV_PORT =
      key_id_from_name(T_TM_PORT_SCHED_CFG, "dev_port");
  D_TM_PORT_SCHED_CFG_MAX_RATE_ENABLE =
      data_id_from_name_noaction(T_TM_PORT_SCHED_CFG, "max_rate_enable");

  /* pvs */
  T_ING_UDP_PORT_VXLAN = table_id_from_name("ingress_udp_port_vxlan");
  F_ING_UDP_PORT_VXLAN_F1 = key_id_from_name(T_ING_UDP_PORT_VXLAN, "f1");
  T_EG_UDP_PORT_VXLAN = table_id_from_name("egress_udp_port_vxlan");
  F_EG_UDP_PORT_VXLAN_F1 = key_id_from_name(T_EG_UDP_PORT_VXLAN, "f1");

  // for folded pipeline only
  T_ING1_UDP_PORT_VXLAN = table_id_from_name("ingress1_udp_port_vxlan");
  F_ING1_UDP_PORT_VXLAN_F1 = key_id_from_name(T_ING1_UDP_PORT_VXLAN, "f1");
  T_EG1_UDP_PORT_VXLAN = table_id_from_name("egress1_udp_port_vxlan");
  F_EG1_UDP_PORT_VXLAN_F1 = key_id_from_name(T_EG1_UDP_PORT_VXLAN, "f1");

  T_INGRESS_CPU_PORT = table_id_from_name("ingress_cpu_port");
  F_INGRESS_CPU_PORT_ETHER_TYPE =
      key_id_from_name(T_INGRESS_CPU_PORT, "ether_type");
  F_INGRESS_CPU_PORT_PORT = key_id_from_name(T_INGRESS_CPU_PORT, "port");

  T_EGRESS_CPU_PORT = table_id_from_name("egress_cpu_port");
  F_EGRESS_CPU_PORT_ETHER_TYPE =
      key_id_from_name(T_EGRESS_CPU_PORT, "ether_type");
  F_EGRESS_CPU_PORT_PORT = key_id_from_name(T_EGRESS_CPU_PORT, "port");

  T_INTERNAL_PIPE_CPU_PORT = table_id_from_name("internal_pipe_cpu_port");
  F_INTERNAL_PIPE_CPU_PORT_ETHER_TYPE =
      key_id_from_name(T_INTERNAL_PIPE_CPU_PORT, "ether_type");
  F_INTERNAL_PIPE_CPU_PORT_PORT =
      key_id_from_name(T_INTERNAL_PIPE_CPU_PORT, "port");

  T_INGRESS_PIPE_CPU_PORT = table_id_from_name("ingress_pipe_cpu_port");
  F_INGRESS_PIPE_CPU_PORT_ETHER_TYPE =
      key_id_from_name(T_INGRESS_PIPE_CPU_PORT, "ether_type");
  F_INGRESS_PIPE_CPU_PORT_PORT =
      key_id_from_name(T_INGRESS_PIPE_CPU_PORT, "port");

  T_INGRESS_NVGRE_ST_KEY = table_id_from_name("ingress_nvgre_st_key");
  F_INGRESS_NVGRE_ST_KEY_VSID_FLOWID =
      key_id_from_name(T_INGRESS_NVGRE_ST_KEY, "vsid_flowid");

  T_EGRESS_NVGRE_ST_KEY = table_id_from_name("egress_nvgre_st_key");
  F_EGRESS_NVGRE_ST_KEY_VSID_FLOWID =
      key_id_from_name(T_EGRESS_NVGRE_ST_KEY, "vsid_flowid");

  /* ECMP IPV4 Dynamic Hash Algorithm Table */
  T_IPV4_DYN_HASH_ALGORITHM = table_id_from_name("ipv4_hash.algorithm");
  A_IPV4_HASH_PREDEFINED =
      action_id_from_name(T_IPV4_DYN_HASH_ALGORITHM, "pre_defined");
  A_IPV4_HASH_USERDEFINED =
      action_id_from_name(T_IPV4_DYN_HASH_ALGORITHM, "user_defined");
  P_IPV4_HASH_SEED = data_id_from_name(
      T_IPV4_DYN_HASH_ALGORITHM, A_IPV4_HASH_PREDEFINED, "seed");
  P_IPV4_HASH_ALG = data_id_from_name(
      T_IPV4_DYN_HASH_ALGORITHM, A_IPV4_HASH_PREDEFINED, "algorithm_name");
  P_IPV4_HASH_REV = data_id_from_name(
      T_IPV4_DYN_HASH_ALGORITHM, A_IPV4_HASH_USERDEFINED, "reverse");
  P_IPV4_HASH_POLY = data_id_from_name(
      T_IPV4_DYN_HASH_ALGORITHM, A_IPV4_HASH_USERDEFINED, "polynomial");
  P_IPV4_HASH_INIT = data_id_from_name(
      T_IPV4_DYN_HASH_ALGORITHM, A_IPV4_HASH_USERDEFINED, "init");
  P_IPV4_HASH_FXOR = data_id_from_name(
      T_IPV4_DYN_HASH_ALGORITHM, A_IPV4_HASH_USERDEFINED, "final_xor");
  P_IPV4_HASH_HBW = data_id_from_name(
      T_IPV4_DYN_HASH_ALGORITHM, A_IPV4_HASH_USERDEFINED, "hash_bit_width");

  /* ECMP IPV6 Dynamic Hash Algorithm Table */
  T_IPV6_DYN_HASH_ALGORITHM = table_id_from_name("ipv6_hash.algorithm");
  A_IPV6_HASH_PREDEFINED =
      action_id_from_name(T_IPV6_DYN_HASH_ALGORITHM, "pre_defined");
  A_IPV6_HASH_USERDEFINED =
      action_id_from_name(T_IPV6_DYN_HASH_ALGORITHM, "user_defined");
  P_IPV6_HASH_SEED = data_id_from_name(
      T_IPV6_DYN_HASH_ALGORITHM, A_IPV6_HASH_PREDEFINED, "seed");
  P_IPV6_HASH_ALG = data_id_from_name(
      T_IPV6_DYN_HASH_ALGORITHM, A_IPV6_HASH_PREDEFINED, "algorithm_name");
  P_IPV6_HASH_REV = data_id_from_name(
      T_IPV6_DYN_HASH_ALGORITHM, A_IPV6_HASH_USERDEFINED, "reverse");
  P_IPV6_HASH_POLY = data_id_from_name(
      T_IPV6_DYN_HASH_ALGORITHM, A_IPV6_HASH_USERDEFINED, "polynomial");
  P_IPV6_HASH_INIT = data_id_from_name(
      T_IPV6_DYN_HASH_ALGORITHM, A_IPV6_HASH_USERDEFINED, "init");
  P_IPV6_HASH_FXOR = data_id_from_name(
      T_IPV6_DYN_HASH_ALGORITHM, A_IPV6_HASH_USERDEFINED, "final_xor");
  P_IPV6_HASH_HBW = data_id_from_name(
      T_IPV6_DYN_HASH_ALGORITHM, A_IPV6_HASH_USERDEFINED, "hash_bit_width");

  // Outer ecmp IPv4
  T_OUTER_IPV4_DYN_HASH_ALGORITHM =
      table_id_from_name("outer_ipv4_hash.algorithm");
  A_OUTER_IPV4_HASH_PREDEFINED =
      action_id_from_name(T_OUTER_IPV4_DYN_HASH_ALGORITHM, "pre_defined");

  // Outer ecmp IPv6
  T_OUTER_IPV6_DYN_HASH_ALGORITHM =
      table_id_from_name("outer_ipv6_hash.algorithm");
  A_OUTER_IPV6_HASH_PREDEFINED =
      action_id_from_name(T_OUTER_IPV6_DYN_HASH_ALGORITHM, "pre_defined");

  /* NON-IP Dynamic Hash Algorithm Table */
  T_NONIP_DYN_HASH_ALGORITHM = table_id_from_name("non_ip_hash.algorithm");
  A_NONIP_HASH_PREDEFINED =
      action_id_from_name(T_NONIP_DYN_HASH_ALGORITHM, "pre_defined");
  A_NONIP_HASH_USERDEFINED =
      action_id_from_name(T_NONIP_DYN_HASH_ALGORITHM, "user_defined");
  P_NONIP_HASH_SEED = data_id_from_name(
      T_NONIP_DYN_HASH_ALGORITHM, A_NONIP_HASH_PREDEFINED, "seed");
  P_NONIP_HASH_ALG = data_id_from_name(
      T_NONIP_DYN_HASH_ALGORITHM, A_NONIP_HASH_PREDEFINED, "algorithm_name");
  P_NONIP_HASH_REV = data_id_from_name(
      T_NONIP_DYN_HASH_ALGORITHM, A_NONIP_HASH_USERDEFINED, "reverse");
  P_NONIP_HASH_POLY = data_id_from_name(
      T_NONIP_DYN_HASH_ALGORITHM, A_NONIP_HASH_USERDEFINED, "polynomial");
  P_NONIP_HASH_INIT = data_id_from_name(
      T_NONIP_DYN_HASH_ALGORITHM, A_NONIP_HASH_USERDEFINED, "init");
  P_NONIP_HASH_FXOR = data_id_from_name(
      T_NONIP_DYN_HASH_ALGORITHM, A_NONIP_HASH_USERDEFINED, "final_xor");
  P_NONIP_HASH_HBW = data_id_from_name(
      T_NONIP_DYN_HASH_ALGORITHM, A_NONIP_HASH_USERDEFINED, "hash_bit_width");

  /* LAG V4 Dynamic Hash Algorithm Table */
  T_LAGV4_DYN_HASH_ALGORITHM = table_id_from_name("lag_v4_hash.algorithm");
  A_LAGV4_HASH_PREDEFINED =
      action_id_from_name(T_LAGV4_DYN_HASH_ALGORITHM, "pre_defined");
  A_LAGV4_HASH_USERDEFINED =
      action_id_from_name(T_LAGV4_DYN_HASH_ALGORITHM, "user_defined");
  P_LAGV4_HASH_SEED = data_id_from_name(
      T_LAGV4_DYN_HASH_ALGORITHM, A_LAGV4_HASH_PREDEFINED, "seed");
  P_LAGV4_HASH_ALG = data_id_from_name(
      T_LAGV4_DYN_HASH_ALGORITHM, A_LAGV4_HASH_PREDEFINED, "algorithm_name");
  P_LAGV4_HASH_REV = data_id_from_name(
      T_LAGV4_DYN_HASH_ALGORITHM, A_LAGV4_HASH_USERDEFINED, "reverse");
  P_LAGV4_HASH_POLY = data_id_from_name(
      T_LAGV4_DYN_HASH_ALGORITHM, A_LAGV4_HASH_USERDEFINED, "polynomial");
  P_LAGV4_HASH_INIT = data_id_from_name(
      T_LAGV4_DYN_HASH_ALGORITHM, A_LAGV4_HASH_USERDEFINED, "init");
  P_LAGV4_HASH_FXOR = data_id_from_name(
      T_LAGV4_DYN_HASH_ALGORITHM, A_LAGV4_HASH_USERDEFINED, "final_xor");
  P_LAGV4_HASH_HBW = data_id_from_name(
      T_LAGV4_DYN_HASH_ALGORITHM, A_LAGV4_HASH_USERDEFINED, "hash_bit_width");

  /* LAG V6 Dynamic Hash Algorithm Table */
  T_LAGV6_DYN_HASH_ALGORITHM = table_id_from_name("lag_v6_hash.algorithm");
  A_LAGV6_HASH_PREDEFINED =
      action_id_from_name(T_LAGV6_DYN_HASH_ALGORITHM, "pre_defined");
  A_LAGV6_HASH_USERDEFINED =
      action_id_from_name(T_LAGV6_DYN_HASH_ALGORITHM, "user_defined");
  P_LAGV6_HASH_SEED = data_id_from_name(
      T_LAGV6_DYN_HASH_ALGORITHM, A_LAGV6_HASH_PREDEFINED, "seed");
  P_LAGV6_HASH_ALG = data_id_from_name(
      T_LAGV6_DYN_HASH_ALGORITHM, A_LAGV6_HASH_PREDEFINED, "algorithm_name");
  P_LAGV6_HASH_REV = data_id_from_name(
      T_LAGV6_DYN_HASH_ALGORITHM, A_LAGV6_HASH_USERDEFINED, "reverse");
  P_LAGV6_HASH_POLY = data_id_from_name(
      T_LAGV6_DYN_HASH_ALGORITHM, A_LAGV6_HASH_USERDEFINED, "polynomial");
  P_LAGV6_HASH_INIT = data_id_from_name(
      T_LAGV6_DYN_HASH_ALGORITHM, A_LAGV6_HASH_USERDEFINED, "init");
  P_LAGV6_HASH_FXOR = data_id_from_name(
      T_LAGV6_DYN_HASH_ALGORITHM, A_LAGV6_HASH_USERDEFINED, "final_xor");
  P_LAGV6_HASH_HBW = data_id_from_name(
      T_LAGV6_DYN_HASH_ALGORITHM, A_LAGV6_HASH_USERDEFINED, "hash_bit_width");

  // ipv4_hash
  T_IPV4_HASH = table_id_from_name("ipv4_hash.configure");
  P_IPV4_HASH_SRC_ADDR = data_id_from_name_noaction(T_IPV4_HASH, "ip_src_addr");
  P_IPV4_HASH_DST_ADDR = data_id_from_name_noaction(T_IPV4_HASH, "ip_dst_addr");
  P_IPV4_HASH_IP_PROTO = data_id_from_name_noaction(T_IPV4_HASH, "ip_proto");
  P_IPV4_HASH_SRC_PORT = data_id_from_name_noaction(T_IPV4_HASH, "l4_src_port");
  P_IPV4_HASH_DST_PORT = data_id_from_name_noaction(T_IPV4_HASH, "l4_dst_port");

  // Container inner fields will have same ids across all the hash fields
  P_HASH_CONTAINER_START_BIT = data_id_from_name_container(
      T_IPV4_HASH, P_IPV4_HASH_SRC_ADDR, "start_bit");
  P_HASH_CONTAINER_LENGTH =
      data_id_from_name_container(T_IPV4_HASH, P_IPV4_HASH_SRC_ADDR, "length");
  P_HASH_CONTAINER_ORDER =
      data_id_from_name_container(T_IPV4_HASH, P_IPV4_HASH_SRC_ADDR, "order");

  // ipv6_hash
  T_IPV6_HASH = table_id_from_name("ipv6_hash.configure");
  P_IPV6_HASH_SRC_ADDR = data_id_from_name_noaction(T_IPV6_HASH, "ip_src_addr");
  P_IPV6_HASH_DST_ADDR = data_id_from_name_noaction(T_IPV6_HASH, "ip_dst_addr");
  P_IPV6_HASH_IP_PROTO = data_id_from_name_noaction(T_IPV6_HASH, "ip_proto");
  P_IPV6_HASH_SRC_PORT = data_id_from_name_noaction(T_IPV6_HASH, "l4_src_port");
  P_IPV6_HASH_DST_PORT = data_id_from_name_noaction(T_IPV6_HASH, "l4_dst_port");
  P_IPV6_HASH_IPV6_FLOW_LABEL =
      data_id_from_name_noaction(T_IPV6_HASH, "ipv6_flow_label");

  // non_ip_hash
  T_NON_IP_HASH = table_id_from_name("non_ip_hash.configure");
  P_NONIP_HASH_ING_PORT = data_id_from_name_noaction(T_NON_IP_HASH, "port");
  P_NONIP_HASH_MAC_TYPE = data_id_from_name_noaction(T_NON_IP_HASH, "mac_type");
  P_NONIP_HASH_SRC_MAC =
      data_id_from_name_noaction(T_NON_IP_HASH, "mac_src_addr");
  P_NONIP_HASH_DST_MAC =
      data_id_from_name_noaction(T_NON_IP_HASH, "mac_dst_addr");

  // lag_v4_hash
  T_LAG_V4_HASH = table_id_from_name("lag_v4_hash.configure");
  P_LAG_V4_HASH_SRC_ADDR =
      data_id_from_name_noaction(T_LAG_V4_HASH, "ip_src_addr");
  P_LAG_V4_HASH_DST_ADDR =
      data_id_from_name_noaction(T_LAG_V4_HASH, "ip_dst_addr");
  P_LAG_V4_HASH_IP_PROTO =
      data_id_from_name_noaction(T_LAG_V4_HASH, "ip_proto");
  P_LAG_V4_HASH_DST_PORT =
      data_id_from_name_noaction(T_LAG_V4_HASH, "l4_dst_port");
  P_LAG_V4_HASH_SRC_PORT =
      data_id_from_name_noaction(T_LAG_V4_HASH, "l4_src_port");

  // lag_v6_hash
  T_LAG_V6_HASH = table_id_from_name("lag_v6_hash.configure");
  P_LAG_V6_HASH_SRC_ADDR =
      data_id_from_name_noaction(T_LAG_V6_HASH, "ip_src_addr");
  P_LAG_V6_HASH_DST_ADDR =
      data_id_from_name_noaction(T_LAG_V6_HASH, "ip_dst_addr");
  P_LAG_V6_HASH_IP_PROTO =
      data_id_from_name_noaction(T_LAG_V6_HASH, "ip_proto");
  P_LAG_V6_HASH_DST_PORT =
      data_id_from_name_noaction(T_LAG_V6_HASH, "l4_dst_port");
  P_LAG_V6_HASH_SRC_PORT =
      data_id_from_name_noaction(T_LAG_V6_HASH, "l4_src_port");
  P_LAG_V6_HASH_FLOW_LABEL =
      data_id_from_name_noaction(T_LAG_V6_HASH, "ipv6_flow_label");

  // inner_dtel_v4_hash
  T_INNER_DTEL_V4_HASH = table_id_from_name("inner_dtelv4_hash.configure");
  P_INNER_DTEL_V4_HASH_SRC_ADDR =
      data_id_from_name_noaction(T_INNER_DTEL_V4_HASH, "ip_src_addr");
  P_INNER_DTEL_V4_HASH_DST_ADDR =
      data_id_from_name_noaction(T_INNER_DTEL_V4_HASH, "ip_dst_addr");
  P_INNER_DTEL_V4_HASH_IP_PROTO =
      data_id_from_name_noaction(T_INNER_DTEL_V4_HASH, "ip_proto");
  P_INNER_DTEL_V4_HASH_DST_PORT =
      data_id_from_name_noaction(T_INNER_DTEL_V4_HASH, "l4_dst_port");
  P_INNER_DTEL_V4_HASH_SRC_PORT =
      data_id_from_name_noaction(T_INNER_DTEL_V4_HASH, "l4_src_port");

  // inner_dtel_v6_hash
  T_INNER_DTEL_V6_HASH = table_id_from_name("inner_dtelv6_hash.configure");
  P_INNER_DTEL_V6_HASH_SRC_ADDR =
      data_id_from_name_noaction(T_INNER_DTEL_V6_HASH, "ip_src_addr");
  P_INNER_DTEL_V6_HASH_DST_ADDR =
      data_id_from_name_noaction(T_INNER_DTEL_V6_HASH, "ip_dst_addr");
  P_INNER_DTEL_V6_HASH_IP_PROTO =
      data_id_from_name_noaction(T_INNER_DTEL_V6_HASH, "ip_proto");
  P_INNER_DTEL_V6_HASH_DST_PORT =
      data_id_from_name_noaction(T_INNER_DTEL_V6_HASH, "l4_dst_port");
  P_INNER_DTEL_V6_HASH_SRC_PORT =
      data_id_from_name_noaction(T_INNER_DTEL_V6_HASH, "l4_src_port");
  P_INNER_DTEL_V6_HASH_FLOW_LABEL =
      data_id_from_name_noaction(T_INNER_DTEL_V6_HASH, "ipv6_flow_label");

  // rmac.p4
  T_INGRESS_PV_RMAC = table_id_from_name("pv_rmac");
  F_INGRESS_PV_RMAC_LOCAL_MD_INGRESS_PORT_LAG_INDEX =
      key_id_from_name(T_INGRESS_PV_RMAC, "local_md.ingress_port_lag_index");
  F_INGRESS_PV_RMAC_HDR_VLAN_TAG0_VALID =
      key_id_from_name(T_INGRESS_PV_RMAC, "hdr.vlan_tag$0.$valid");
  F_INGRESS_PV_RMAC_HDR_VLAN_TAG0_VID =
      key_id_from_name(T_INGRESS_PV_RMAC, "hdr.vlan_tag$0.vid");
  F_INGRESS_PV_RMAC_HDR_ETHERNET_DST_ADDR =
      key_id_from_name(T_INGRESS_PV_RMAC, "hdr.ethernet.dst_addr");
  F_INGRESS_PV_RMAC_MATCH_PRIORITY =
      key_id_from_name(T_INGRESS_PV_RMAC, "$MATCH_PRIORITY");

  A_RMAC_MISS = action_id_from_name(T_INGRESS_PV_RMAC, "rmac_miss");
  A_RMAC_HIT = action_id_from_name(T_INGRESS_PV_RMAC, "rmac_hit");

  T_INGRESS_VLAN_RMAC = table_id_from_name("vlan_rmac");
  F_INGRESS_VLAN_RMAC_HDR_VLAN_TAG0_VID =
      key_id_from_name(T_INGRESS_VLAN_RMAC, "hdr.vlan_tag$0.vid");
  F_INGRESS_VLAN_RMAC_HDR_ETHERNET_DST_ADDR =
      key_id_from_name(T_INGRESS_VLAN_RMAC, "hdr.ethernet.dst_addr");

  /* validation.p4 */
  T_VALIDATE_ETHERNET = table_id_from_name("validate_ethernet");
  F_VALIDATE_ETHERNET_ETHERNET_SRC_ADDR =
      key_id_from_name(T_VALIDATE_ETHERNET, "hdr.ethernet.src_addr");
  F_VALIDATE_ETHERNET_ETHERNET_DST_ADDR =
      key_id_from_name(T_VALIDATE_ETHERNET, "hdr.ethernet.dst_addr");
  F_VALIDATE_ETHERNET_VLAN_0_VALID =
      key_id_from_name(T_VALIDATE_ETHERNET, "hdr.vlan_tag$0.$valid");
  F_VALIDATE_ETHERNET_VLAN_1_VALID =
      key_id_from_name(T_VALIDATE_ETHERNET, "hdr.vlan_tag$1.$valid");
  F_VALIDATE_ETHERNET_PRIORITY =
      key_id_from_name(T_VALIDATE_ETHERNET, "$MATCH_PRIORITY");
  A_MALFORMED_ETH_PKT =
      action_id_from_name(T_VALIDATE_ETHERNET, "malformed_eth_pkt");
  P_MALFORMED_ETH_PKT_REASON =
      data_id_from_name(T_VALIDATE_ETHERNET, A_MALFORMED_ETH_PKT, "reason");
  A_VALID_PKT_UNTAGGED =
      action_id_from_name(T_VALIDATE_ETHERNET, "valid_pkt_untagged");
  P_VALID_PKT_UNTAGGED_PKT_TYPE =
      data_id_from_name(T_VALIDATE_ETHERNET, A_VALID_PKT_UNTAGGED, "pkt_type");
  A_VALID_PKT_TAGGED =
      action_id_from_name(T_VALIDATE_ETHERNET, "valid_pkt_tagged");
  P_VALID_PKT_TAGGED_PKT_TYPE =
      data_id_from_name(T_VALIDATE_ETHERNET, A_VALID_PKT_TAGGED, "pkt_type");
  A_VALID_PKT_DOUBLE_TAGGED =
      action_id_from_name(T_VALIDATE_ETHERNET, "valid_pkt_double_tagged");
  P_VALID_PKT_DOUBLE_TAGGED_PKT_TYPE = data_id_from_name(
      T_VALIDATE_ETHERNET, A_VALID_PKT_DOUBLE_TAGGED, "pkt_type");

  T_VALIDATE_IP = table_id_from_name("validate_ip");
  F_VALIDATE_IP_ARP_VALID = key_id_from_name(T_VALIDATE_IP, "hdr.arp.$valid");
  F_VALIDATE_IP_IPV4_VALID = key_id_from_name(T_VALIDATE_IP, "hdr.ipv4.$valid");
  F_VALIDATE_IP_IPV4_CHKSUM_ERR =
      key_id_from_name(T_VALIDATE_IP, "local_md.flags.ipv4_checksum_err");
  F_VALIDATE_IP_IPV4_VERSION =
      key_id_from_name(T_VALIDATE_IP, "hdr.ipv4.version");
  F_VALIDATE_IP_IPV4_IHL = key_id_from_name(T_VALIDATE_IP, "hdr.ipv4.ihl");
  F_VALIDATE_IP_IPV4_FLAGS = key_id_from_name(T_VALIDATE_IP, "hdr.ipv4.flags");
  F_VALIDATE_IP_IPV4_FRAG_OFFSET =
      key_id_from_name(T_VALIDATE_IP, "hdr.ipv4.frag_offset");
  F_VALIDATE_IP_IPV4_TTL = key_id_from_name(T_VALIDATE_IP, "hdr.ipv4.ttl");
  F_VALIDATE_IP_IPV4_SRC_ADDR =
      key_id_from_name(T_VALIDATE_IP, "hdr.ipv4.src_addr[31:0]");
  F_VALIDATE_IP_IPV6_VALID = key_id_from_name(T_VALIDATE_IP, "hdr.ipv6.$valid");
  F_VALIDATE_IP_IPV6_VERSION =
      key_id_from_name(T_VALIDATE_IP, "hdr.ipv6.version");
  F_VALIDATE_IP_IPV6_HOP_LIMIT =
      key_id_from_name(T_VALIDATE_IP, "hdr.ipv6.hop_limit");
  F_VALIDATE_IP_IPV6_SRC_ADDR =
      key_id_from_name(T_VALIDATE_IP, "hdr.ipv6.src_addr[127:0]");
  F_VALIDATE_IP_MPLS_0_VALID =
      key_id_from_name(T_VALIDATE_IP, "hdr.mpls$0.$valid");
  F_VALIDATE_IP_MPLS_0_LABEL =
      key_id_from_name(T_VALIDATE_IP, "hdr.mpls$0.label");
  F_VALIDATE_IP_MPLS_1_VALID =
      key_id_from_name(T_VALIDATE_IP, "hdr.mpls$1.$valid");
  F_VALIDATE_IP_INNER_IPV4_VALID =
      key_id_from_name(T_VALIDATE_IP, "hdr.inner_ipv4.$valid");
  F_VALIDATE_IP_INNER_IPV6_VALID =
      key_id_from_name(T_VALIDATE_IP, "hdr.inner_ipv6.$valid");
  F_VALIDATE_IP_PRIORITY = key_id_from_name(T_VALIDATE_IP, "$MATCH_PRIORITY");
  A_MALFORMED_IPV4_PKT =
      action_id_from_name(T_VALIDATE_IP, "malformed_ipv4_pkt");
  P_MALFORMED_IPV4_PKT_REASON =
      data_id_from_name(T_VALIDATE_IP, A_MALFORMED_IPV4_PKT, "reason");
  A_MALFORMED_IPV6_PKT =
      action_id_from_name(T_VALIDATE_IP, "malformed_ipv6_pkt");
  P_MALFORMED_IPV6_PKT_REASON =
      data_id_from_name(T_VALIDATE_IP, A_MALFORMED_IPV6_PKT, "reason");
  A_VALID_ARP_PKT = action_id_from_name(T_VALIDATE_IP, "valid_arp_pkt");
  A_VALID_IPV4_PKT = action_id_from_name(T_VALIDATE_IP, "valid_ipv4_pkt");
  P_VALID_IPV4_PKT_IP_FRAG =
      data_id_from_name(T_VALIDATE_IP, A_VALID_IPV4_PKT, "ip_frag");
  P_VALID_IPV4_PKT_IS_LINK_LOCAL =
      data_id_from_name(T_VALIDATE_IP, A_VALID_IPV4_PKT, "is_link_local");
  A_VALID_IPV6_PKT = action_id_from_name(T_VALIDATE_IP, "valid_ipv6_pkt");
  A_VALID_MPLS_PKT = action_id_from_name(T_VALIDATE_IP, "valid_mpls_pkt");
  A_VALID_MPLS_NULL_PKT =
      action_id_from_name(T_VALIDATE_IP, "valid_mpls_null_pkt");
  A_VALID_MPLS_NULL_IPV4_PKT =
      action_id_from_name(T_VALIDATE_IP, "valid_mpls_null_ipv4_pkt");
  A_VALID_MPLS_NULL_IPV6_PKT =
      action_id_from_name(T_VALIDATE_IP, "valid_mpls_null_ipv6_pkt");
  A_VALID_MPLS_ROUTER_ALERT_LABEL =
      action_id_from_name(T_VALIDATE_IP, "valid_mpls_router_alert_label");

  T_INNER_VALIDATE_ETHERNET = table_id_from_name("validate_inner_ethernet");
  F_INNER_VALIDATE_ETHERNET_ETHERNET_DST_ADDR = key_id_from_name(
      T_INNER_VALIDATE_ETHERNET, "hdr.inner_ethernet.dst_addr");
  F_INNER_VALIDATE_ETHERNET_ETHERNET_VALID =
      key_id_from_name(T_INNER_VALIDATE_ETHERNET, "hdr.inner_ethernet.$valid");
  F_INNER_VALIDATE_ETHERNET_IPV4_VALID =
      key_id_from_name(T_INNER_VALIDATE_ETHERNET, "hdr.inner_ipv4.$valid");
  F_INNER_VALIDATE_ETHERNET_IPV4_CHKSUM_ERR = key_id_from_name(
      T_INNER_VALIDATE_ETHERNET, "local_md.flags.inner_ipv4_checksum_err");
  F_INNER_VALIDATE_ETHERNET_IPV4_VERSION =
      key_id_from_name(T_INNER_VALIDATE_ETHERNET, "hdr.inner_ipv4.version");
  F_INNER_VALIDATE_ETHERNET_IPV4_TTL =
      key_id_from_name(T_INNER_VALIDATE_ETHERNET, "hdr.inner_ipv4.ttl");
  F_INNER_VALIDATE_ETHERNET_IPV4_IHL =
      key_id_from_name(T_INNER_VALIDATE_ETHERNET, "hdr.inner_ipv4.ihl");
  F_INNER_VALIDATE_ETHERNET_IPV6_VALID =
      key_id_from_name(T_INNER_VALIDATE_ETHERNET, "hdr.inner_ipv6.$valid");
  F_INNER_VALIDATE_ETHERNET_IPV6_VERSION =
      key_id_from_name(T_INNER_VALIDATE_ETHERNET, "hdr.inner_ipv6.version");
  F_INNER_VALIDATE_ETHERNET_IPV6_HOP_LIMIT =
      key_id_from_name(T_INNER_VALIDATE_ETHERNET, "hdr.inner_ipv6.hop_limit");
  F_INNER_VALIDATE_ETHERNET_TCP_VALID =
      key_id_from_name(T_INNER_VALIDATE_ETHERNET, "hdr.inner_tcp.$valid");
  F_INNER_VALIDATE_ETHERNET_UDP_VALID =
      key_id_from_name(T_INNER_VALIDATE_ETHERNET, "hdr.inner_udp.$valid");
  F_INNER_VALIDATE_ETHERNET_PRIORITY =
      key_id_from_name(T_INNER_VALIDATE_ETHERNET, "$MATCH_PRIORITY");
  A_INNER_L2_MALFORMED_PKT =
      action_id_from_name(T_INNER_VALIDATE_ETHERNET, "malformed_l2_inner_pkt");
  P_INNER_L2_MALFORMED_PKT_REASON = data_id_from_name(
      T_INNER_VALIDATE_ETHERNET, A_INNER_L2_MALFORMED_PKT, "reason");
  A_INNER_L3_MALFORMED_PKT =
      action_id_from_name(T_INNER_VALIDATE_ETHERNET, "malformed_l3_inner_pkt");
  P_INNER_L3_MALFORMED_PKT_REASON = data_id_from_name(
      T_INNER_VALIDATE_ETHERNET, A_INNER_L3_MALFORMED_PKT, "reason");
  A_INNER_VALID_ETHERNET_PKT = action_id_from_name(T_INNER_VALIDATE_ETHERNET,
                                                   "valid_inner_ethernet_pkt");
  P_INNER_VALID_ETHERNET_PKT_TYPE = data_id_from_name(
      T_INNER_VALIDATE_ETHERNET, A_INNER_VALID_ETHERNET_PKT, "pkt_type");
  A_INNER_VALID_IPV4_PKT =
      action_id_from_name(T_INNER_VALIDATE_ETHERNET, "valid_inner_ipv4_pkt");
  P_INNER_VALID_IPV4_PKT_TYPE = data_id_from_name(
      T_INNER_VALIDATE_ETHERNET, A_INNER_VALID_IPV4_PKT, "pkt_type");
  A_INNER_VALID_IPV6_PKT =
      action_id_from_name(T_INNER_VALIDATE_ETHERNET, "valid_inner_ipv6_pkt");
  P_INNER_VALID_IPV6_PKT_TYPE = data_id_from_name(
      T_INNER_VALIDATE_ETHERNET, A_INNER_VALID_IPV6_PKT, "pkt_type");
  A_INNER_VALID_IPV4_TCP_PKT = action_id_from_name(T_INNER_VALIDATE_ETHERNET,
                                                   "valid_inner_ipv4_tcp_pkt");
  P_INNER_VALID_IPV4_TCP_PKT_TYPE = data_id_from_name(
      T_INNER_VALIDATE_ETHERNET, A_INNER_VALID_IPV4_TCP_PKT, "pkt_type");
  A_INNER_VALID_IPV4_UDP_PKT = action_id_from_name(T_INNER_VALIDATE_ETHERNET,
                                                   "valid_inner_ipv4_udp_pkt");
  P_INNER_VALID_IPV4_UDP_PKT_TYPE = data_id_from_name(
      T_INNER_VALIDATE_ETHERNET, A_INNER_VALID_IPV4_UDP_PKT, "pkt_type");
  A_INNER_VALID_IPV6_TCP_PKT = action_id_from_name(T_INNER_VALIDATE_ETHERNET,
                                                   "valid_inner_ipv6_tcp_pkt");
  P_INNER_VALID_IPV6_TCP_PKT_TYPE = data_id_from_name(
      T_INNER_VALIDATE_ETHERNET, A_INNER_VALID_IPV6_TCP_PKT, "pkt_type");
  A_INNER_VALID_IPV6_UDP_PKT = action_id_from_name(T_INNER_VALIDATE_ETHERNET,
                                                   "valid_inner_ipv6_udp_pkt");
  P_INNER_VALID_IPV6_UDP_PKT_TYPE = data_id_from_name(
      T_INNER_VALIDATE_ETHERNET, A_INNER_VALID_IPV6_UDP_PKT, "pkt_type");

  T_INGRESS_SAME_MAC_CHECK = table_id_from_name("same_mac_check");
  F_INGRESS_SAME_MAC_CHECK_LOCAL_MD_SAME_MAC =
      key_id_from_name(T_INGRESS_SAME_MAC_CHECK, "local_md.same_mac");
  A_COMPUTE_SAME_MAC_CHECK =
      action_id_from_name(T_INGRESS_SAME_MAC_CHECK, "compute_same_mac_check");

  /* validation_fp.p4 */
  T_FP_VALIDATE_ETHERNET = table_id_from_name("fp_validate_ethernet");
  F_FP_VALIDATE_ETHERNET_ETHERNET_SRC_ADDR =
      key_id_from_name(T_FP_VALIDATE_ETHERNET, "hdr.ethernet.src_addr");
  F_FP_VALIDATE_ETHERNET_ETHERNET_DST_ADDR =
      key_id_from_name(T_FP_VALIDATE_ETHERNET, "hdr.ethernet.dst_addr");
  F_FP_VALIDATE_ETHERNET_VLAN_0_VALID =
      key_id_from_name(T_FP_VALIDATE_ETHERNET, "hdr.vlan_tag$0.$valid");
  F_FP_VALIDATE_ETHERNET_VLAN_1_VALID =
      key_id_from_name(T_FP_VALIDATE_ETHERNET, "hdr.vlan_tag$1.$valid");
  F_FP_VALIDATE_ETHERNET_PRIORITY =
      key_id_from_name(T_FP_VALIDATE_ETHERNET, "$MATCH_PRIORITY");
  A_FP_MALFORMED_ETH_PKT =
      action_id_from_name(T_FP_VALIDATE_ETHERNET, "fp_malformed_eth_pkt");
  P_FP_MALFORMED_ETH_PKT_REASON = data_id_from_name(
      T_FP_VALIDATE_ETHERNET, A_FP_MALFORMED_ETH_PKT, "reason");
  A_FP_VALID_PKT_UNTAGGED =
      action_id_from_name(T_FP_VALIDATE_ETHERNET, "fp_valid_pkt_untagged");
  P_FP_VALID_PKT_UNTAGGED_PKT_TYPE = data_id_from_name(
      T_FP_VALIDATE_ETHERNET, A_FP_VALID_PKT_UNTAGGED, "pkt_type");
  A_FP_VALID_PKT_TAGGED =
      action_id_from_name(T_FP_VALIDATE_ETHERNET, "fp_valid_pkt_tagged");
  P_FP_VALID_PKT_TAGGED_PKT_TYPE = data_id_from_name(
      T_FP_VALIDATE_ETHERNET, A_FP_VALID_PKT_TAGGED, "pkt_type");
  A_FP_VALID_PKT_DOUBLE_TAGGED =
      action_id_from_name(T_FP_VALIDATE_ETHERNET, "fp_valid_pkt_double_tagged");
  P_FP_VALID_PKT_DOUBLE_TAGGED_PKT_TYPE = data_id_from_name(
      T_FP_VALIDATE_ETHERNET, A_FP_VALID_PKT_DOUBLE_TAGGED, "pkt_type");

  T_FP_VALIDATE_IP = table_id_from_name("fp_validate_ip");
  F_FP_VALIDATE_IP_ARP_VALID =
      key_id_from_name(T_FP_VALIDATE_IP, "hdr.arp.$valid");
  F_FP_VALIDATE_IP_IPV4_VALID =
      key_id_from_name(T_FP_VALIDATE_IP, "hdr.ipv4.$valid");
  F_FP_VALIDATE_IP_IPV4_CHKSUM_ERR =
      key_id_from_name(T_FP_VALIDATE_IP, "local_md.flags.ipv4_checksum_err");
  F_FP_VALIDATE_IP_IPV4_VERSION =
      key_id_from_name(T_FP_VALIDATE_IP, "hdr.ipv4.version");
  F_FP_VALIDATE_IP_IPV4_IHL =
      key_id_from_name(T_FP_VALIDATE_IP, "hdr.ipv4.ihl");
  F_FP_VALIDATE_IP_IPV4_FLAGS =
      key_id_from_name(T_FP_VALIDATE_IP, "hdr.ipv4.flags");
  F_FP_VALIDATE_IP_IPV4_FRAG_OFFSET =
      key_id_from_name(T_FP_VALIDATE_IP, "hdr.ipv4.frag_offset");
  F_FP_VALIDATE_IP_IPV4_TTL =
      key_id_from_name(T_FP_VALIDATE_IP, "hdr.ipv4.ttl");
  F_FP_VALIDATE_IP_IPV4_SRC_ADDR =
      key_id_from_name(T_FP_VALIDATE_IP, "hdr.ipv4.src_addr[31:0]");
  F_FP_VALIDATE_IP_IPV6_VALID =
      key_id_from_name(T_FP_VALIDATE_IP, "hdr.ipv6.$valid");
  F_FP_VALIDATE_IP_IPV6_VERSION =
      key_id_from_name(T_FP_VALIDATE_IP, "hdr.ipv6.version");
  F_FP_VALIDATE_IP_IPV6_HOP_LIMIT =
      key_id_from_name(T_FP_VALIDATE_IP, "hdr.ipv6.hop_limit");
  F_FP_VALIDATE_IP_IPV6_SRC_ADDR =
      key_id_from_name(T_FP_VALIDATE_IP, "hdr.ipv6.src_addr[127:0]");
  F_FP_VALIDATE_IP_MPLS_0_VALID =
      key_id_from_name(T_FP_VALIDATE_IP, "hdr.mpls$0.$valid");
  F_FP_VALIDATE_IP_MPLS_0_LABEL =
      key_id_from_name(T_FP_VALIDATE_IP, "hdr.mpls$0.label");
  F_FP_VALIDATE_IP_MPLS_1_VALID =
      key_id_from_name(T_FP_VALIDATE_IP, "hdr.mpls$1.$valid");
  F_FP_VALIDATE_IP_INNER_IPV4_VALID =
      key_id_from_name(T_FP_VALIDATE_IP, "hdr.inner_ipv4.$valid");
  F_FP_VALIDATE_IP_INNER_IPV6_VALID =
      key_id_from_name(T_FP_VALIDATE_IP, "hdr.inner_ipv6.$valid");
  F_FP_VALIDATE_IP_PRIORITY =
      key_id_from_name(T_FP_VALIDATE_IP, "$MATCH_PRIORITY");
  A_FP_MALFORMED_IPV4_PKT =
      action_id_from_name(T_FP_VALIDATE_IP, "fp_malformed_ipv4_pkt");
  P_FP_MALFORMED_IPV4_PKT_REASON =
      data_id_from_name(T_FP_VALIDATE_IP, A_FP_MALFORMED_IPV4_PKT, "reason");
  A_FP_MALFORMED_IPV6_PKT =
      action_id_from_name(T_FP_VALIDATE_IP, "fp_malformed_ipv6_pkt");
  P_FP_MALFORMED_IPV6_PKT_REASON =
      data_id_from_name(T_FP_VALIDATE_IP, A_FP_MALFORMED_IPV6_PKT, "reason");
  A_FP_VALID_ARP_PKT =
      action_id_from_name(T_FP_VALIDATE_IP, "fp_valid_arp_pkt");
  A_FP_VALID_IPV4_PKT =
      action_id_from_name(T_FP_VALIDATE_IP, "fp_valid_ipv4_pkt");
  P_FP_VALID_IPV4_PKT_IP_FRAG =
      data_id_from_name(T_FP_VALIDATE_IP, A_FP_VALID_IPV4_PKT, "ip_frag");
  P_FP_VALID_IPV4_PKT_IS_LINK_LOCAL =
      data_id_from_name(T_FP_VALIDATE_IP, A_FP_VALID_IPV4_PKT, "is_link_local");
  A_FP_VALID_IPV6_PKT =
      action_id_from_name(T_FP_VALIDATE_IP, "fp_valid_ipv6_pkt");
  A_FP_VALID_MPLS_PKT = action_id_from_name(T_FP_VALIDATE_IP, "valid_mpls_pkt");
  A_FP_VALID_MPLS_NULL_PKT =
      action_id_from_name(T_FP_VALIDATE_IP, "fp_valid_mpls_null_pkt");
  A_FP_VALID_MPLS_NULL_IPV4_PKT =
      action_id_from_name(T_FP_VALIDATE_IP, "fp_valid_mpls_null_ipv4_pkt");
  A_FP_VALID_MPLS_NULL_IPV6_PKT =
      action_id_from_name(T_FP_VALIDATE_IP, "fp_valid_mpls_null_ipv6_pkt");
  A_FP_VALID_MPLS_ROUTER_ALERT_LABEL =
      action_id_from_name(T_FP_VALIDATE_IP, "fp_valid_mpls_router_alert_label");

  T_FP_INGRESS_SAME_MAC_CHECK = table_id_from_name("fp_same_mac_check");
  F_FP_INGRESS_SAME_MAC_CHECK_LOCAL_MD_SAME_MAC =
      key_id_from_name(T_FP_INGRESS_SAME_MAC_CHECK, "local_md.same_mac");
  A_FP_COMPUTE_SAME_MAC_CHECK = action_id_from_name(
      T_FP_INGRESS_SAME_MAC_CHECK, "fp_compute_same_mac_check");

  /* NOTE: IDs from section below are not in use
     as long as constant entries of validate_ethernet table in control
     PktValidation1 in p4 code are sufficient */
  T_FP_VALIDATE_ETHERNET_1 = table_id_from_name("fp_validate_ethernet_1");

  F_FP_VALIDATE_ETHERNET_1_PRIORITY =
      key_id_from_name(T_FP_VALIDATE_ETHERNET_1, "$MATCH_PRIORITY");
  F_FP_VALIDATE_ETHERNET_1_IPV6_VALID =
      key_id_from_name(T_FP_VALIDATE_ETHERNET_1, "hdr.ipv6.$valid");
  F_FP_VALIDATE_ETHERNET_1_IPV6_VERSION =
      key_id_from_name(T_FP_VALIDATE_ETHERNET_1, "hdr.ipv6.version");
  F_FP_VALIDATE_ETHERNET_1_IPV6_HOP_LIMIT =
      key_id_from_name(T_FP_VALIDATE_ETHERNET_1, "hdr.ipv6.hop_limit");
  F_FP_VALIDATE_ETHERNET_1_IPV4_VALID =
      key_id_from_name(T_FP_VALIDATE_ETHERNET_1, "hdr.ipv4.$valid");
  F_FP_VALIDATE_ETHERNET_1_IPV4_CHKSUM_ERR = key_id_from_name(
      T_FP_VALIDATE_ETHERNET_1, "local_md.flags.ipv4_checksum_err");
  F_FP_VALIDATE_ETHERNET_1_IPV4_VERSION =
      key_id_from_name(T_FP_VALIDATE_ETHERNET_1, "hdr.ipv4.version");
  F_FP_VALIDATE_ETHERNET_1_IPV4_IHL =
      key_id_from_name(T_FP_VALIDATE_ETHERNET_1, "hdr.ipv4.ihl");
  F_FP_VALIDATE_ETHERNET_1_IPV4_TTL =
      key_id_from_name(T_FP_VALIDATE_ETHERNET_1, "hdr.ipv4.ttl");
  F_FP_VALIDATE_ETHERNET_1_TCP_VALID =
      key_id_from_name(T_FP_VALIDATE_ETHERNET_1, "hdr.tcp.$valid");
  F_FP_VALIDATE_ETHERNET_1_UDP_VALID =
      key_id_from_name(T_FP_VALIDATE_ETHERNET_1, "hdr.udp.$valid");

  A_FP_VALID_ETHERNET_PACKET_1 =
      action_id_from_name(T_FP_VALIDATE_ETHERNET_1, "fp_valid_ethernet_pkt_1");
  A_FP_VALID_IPV4_PKT_1 =
      action_id_from_name(T_FP_VALIDATE_ETHERNET_1, "fp_valid_ipv4_pkt_1");
  A_FP_VALID_IPV6_PKT_1 =
      action_id_from_name(T_FP_VALIDATE_ETHERNET_1, "fp_valid_ipv6_pkt_1");
  A_FP_VALID_IPV4_TCP_PKT_1 =
      action_id_from_name(T_FP_VALIDATE_ETHERNET_1, "fp_valid_ipv4_tcp_pkt_1");
  P_FP_VALID_IPV4_TCP_PKT_1_PKT_TYPE = data_id_from_name(
      T_FP_VALIDATE_ETHERNET_1, A_FP_VALID_IPV4_TCP_PKT_1, "pkt_type");
  A_FP_VALID_IPV4_UDP_PKT_1 =
      action_id_from_name(T_FP_VALIDATE_ETHERNET_1, "fp_valid_ipv4_udp_pkt_1");
  P_FP_VALID_IPV4_UDP_PKT_1_PKT_TYPE = data_id_from_name(
      T_FP_VALIDATE_ETHERNET_1, A_FP_VALID_IPV4_UDP_PKT_1, "pkt_type");
  A_FP_VALID_IPV6_TCP_PKT_1 =
      action_id_from_name(T_FP_VALIDATE_ETHERNET_1, "fp_valid_ipv6_tcp_pkt_1");
  P_FP_VALID_IPV6_TCP_PKT_1_PKT_TYPE = data_id_from_name(
      T_FP_VALIDATE_ETHERNET_1, A_FP_VALID_IPV6_TCP_PKT_1, "pkt_type");
  A_FP_VALID_IPV6_UDP_PKT_1 =
      action_id_from_name(T_FP_VALIDATE_ETHERNET_1, "fp_valid_ipv6_udp_pkt_1");
  P_FP_VALID_IPV6_UDP_PKT_1_PKT_TYPE = data_id_from_name(
      T_FP_VALIDATE_ETHERNET_1, A_FP_VALID_IPV6_UDP_PKT_1, "pkt_type");
  A_FP_MALFORMED_PKT_1 =
      action_id_from_name(T_FP_VALIDATE_ETHERNET_1, "fp_malformed_pkt_1");
  P_FP_MALFORMED_PKT_1_REASON = data_id_from_name(
      T_FP_VALIDATE_ETHERNET_1, A_FP_MALFORMED_PKT_1, "reason");

  T_EGRESS_PKT_VALIDATION = table_id_from_name("egress_pkt_validation");

  /* port.p4 */
  T_PORT_METADATA = table_id_from_name("SwitchIngressParser.$PORT_METADATA");
  // temporary wokraround
  if (T_PORT_METADATA == 0) {
    T_PORT_METADATA =
        table_id_from_name("SwitchIngressParser_0.$PORT_METADATA");
  }
  F_PORT_METADATA_PORT =
      key_id_from_name(T_PORT_METADATA, "ig_intr_md.ingress_port");
  P_PORT_METADATA_PORT_LAG_INDEX =
      data_id_from_name_noaction(T_PORT_METADATA, "port_lag_index");
  P_PORT_METADATA_PORT_LAG_LABEL =
      data_id_from_name_noaction(T_PORT_METADATA, "port_lag_label");
  P_PORT_METADATA_EXT_INGRESS_PORT =
      data_id_from_name_noaction(T_PORT_METADATA, "ext_ingress_port");

  T_INGRESS_PORT_MIRROR = table_id_from_name("ingress_port_mirror");
  F_INGRESS_PORT_MIRROR_PORT = key_id_from_name(T_INGRESS_PORT_MIRROR, "port");
  A_INGRESS_PORT_MIRROR_SET_MIRROR_ID =
      action_id_from_name(T_INGRESS_PORT_MIRROR, "set_ingress_mirror_id");
  P_INGRESS_PORT_MIRROR_SET_MIRROR_ID_SESSION_ID = data_id_from_name(
      T_INGRESS_PORT_MIRROR, A_INGRESS_PORT_MIRROR_SET_MIRROR_ID, "session_id");
  P_INGRESS_PORT_MIRROR_SET_MIRROR_ID_METER_ID =
      data_id_from_name(T_INGRESS_PORT_MIRROR,
                        A_INGRESS_PORT_MIRROR_SET_MIRROR_ID,
                        "meter_index");
  P_INGRESS_PORT_MIRROR_SET_MIRROR_ID_SRC = data_id_from_name(
      T_INGRESS_PORT_MIRROR, A_INGRESS_PORT_MIRROR_SET_MIRROR_ID, "src");

  T_EGRESS_PORT_MIRROR = table_id_from_name("egress_port_mirror");
  F_EGRESS_PORT_MIRROR_PORT = key_id_from_name(T_EGRESS_PORT_MIRROR, "port");
  A_EGRESS_PORT_MIRROR_SET_MIRROR_ID =
      action_id_from_name(T_EGRESS_PORT_MIRROR, "set_egress_mirror_id");
  P_EGRESS_PORT_MIRROR_SET_MIRROR_ID_SESSION_ID = data_id_from_name(
      T_EGRESS_PORT_MIRROR, A_EGRESS_PORT_MIRROR_SET_MIRROR_ID, "session_id");
  P_EGRESS_PORT_MIRROR_SET_MIRROR_ID_METER_ID = data_id_from_name(
      T_EGRESS_PORT_MIRROR, A_EGRESS_PORT_MIRROR_SET_MIRROR_ID, "meter_index");
  P_EGRESS_PORT_MIRROR_SET_MIRROR_ID_SRC = data_id_from_name(
      T_EGRESS_PORT_MIRROR, A_EGRESS_PORT_MIRROR_SET_MIRROR_ID, "src");

  T_INGRESS_PORT_MAPPING = table_id_from_name("ingress_port_mapping");
  F_INGRESS_PORT_MAPPING_LOCAL_MD_INGRESS_PORT =
      key_id_from_name(T_INGRESS_PORT_MAPPING, "local_md.ingress_port");
  F_INGRESS_PORT_MAPPING_HDR_CPU_VALID =
      key_id_from_name(T_INGRESS_PORT_MAPPING, "hdr.cpu.$valid");
  F_INGRESS_PORT_MAPPING_HDR_CPU_INGRESS_PORT =
      key_id_from_name(T_INGRESS_PORT_MAPPING, "hdr.cpu.ingress_port");
  A_SET_PORT_PROPERTIES =
      action_id_from_name(T_INGRESS_PORT_MAPPING, "set_port_properties");
  P_SET_PORT_PROPERTIES_EXCLUSION_ID = data_id_from_name(
      T_INGRESS_PORT_MAPPING, A_SET_PORT_PROPERTIES, "exclusion_id");
  P_SET_PORT_PROPERTIES_LEARNING_MODE = data_id_from_name(
      T_INGRESS_PORT_MAPPING, A_SET_PORT_PROPERTIES, "learning_mode");
  P_SET_PORT_PROPERTIES_COLOR =
      data_id_from_name(T_INGRESS_PORT_MAPPING, A_SET_PORT_PROPERTIES, "color");
  P_SET_PORT_PROPERTIES_TC =
      data_id_from_name(T_INGRESS_PORT_MAPPING, A_SET_PORT_PROPERTIES, "tc");
  P_SET_PORT_PROPERTIES_SFLOW_SESSION_ID = data_id_from_name(
      T_INGRESS_PORT_MAPPING, A_SET_PORT_PROPERTIES, "sflow_session_id");
  P_SET_PORT_PROPERTIES_IN_PORTS_GROUP_LABEL_IPV4 =
      data_id_from_name(T_INGRESS_PORT_MAPPING,
                        A_SET_PORT_PROPERTIES,
                        "in_ports_group_label_ipv4");
  P_SET_PORT_PROPERTIES_IN_PORTS_GROUP_LABEL_IPV6 =
      data_id_from_name(T_INGRESS_PORT_MAPPING,
                        A_SET_PORT_PROPERTIES,
                        "in_ports_group_label_ipv6");
  P_SET_PORT_PROPERTIES_IN_PORTS_GROUP_LABEL_MIRROR =
      data_id_from_name(T_INGRESS_PORT_MAPPING,
                        A_SET_PORT_PROPERTIES,
                        "in_ports_group_label_mirror");
  A_SET_CPU_PORT_PROPERTIES =
      action_id_from_name(T_INGRESS_PORT_MAPPING, "set_cpu_port_properties");
  P_SET_CPU_PORT_PROPERTIES_PORT_LAG_INDEX = data_id_from_name(
      T_INGRESS_PORT_MAPPING, A_SET_CPU_PORT_PROPERTIES, "port_lag_index");
  P_SET_CPU_PORT_PROPERTIES_PORT_LAG_LABEL = data_id_from_name(
      T_INGRESS_PORT_MAPPING, A_SET_CPU_PORT_PROPERTIES, "port_lag_label");
  P_SET_CPU_PORT_PROPERTIES_EXCLUSION_ID = data_id_from_name(
      T_INGRESS_PORT_MAPPING, A_SET_CPU_PORT_PROPERTIES, "exclusion_id");
  P_SET_CPU_PORT_PROPERTIES_COLOR = data_id_from_name(
      T_INGRESS_PORT_MAPPING, A_SET_CPU_PORT_PROPERTIES, "color");
  P_SET_CPU_PORT_PROPERTIES_TC = data_id_from_name(
      T_INGRESS_PORT_MAPPING, A_SET_CPU_PORT_PROPERTIES, "tc");

  T_PORT_VLAN_TO_BD_MAPPING = table_id_from_name("port_vlan_to_bd_mapping");
  F_PORT_VLAN_TO_BD_MAPPING_LOCAL_MD_INGRESS_PORT_LAG_INDEX = key_id_from_name(
      T_PORT_VLAN_TO_BD_MAPPING, "local_md.ingress_port_lag_index");
  F_PORT_VLAN_TO_BD_MAPPING_VLAN_0_VID =
      key_id_from_name(T_PORT_VLAN_TO_BD_MAPPING, "hdr.vlan_tag$0.vid");
  F_PORT_VLAN_TO_BD_MAPPING_VLAN_0_VID_VALID =
      key_id_from_name(T_PORT_VLAN_TO_BD_MAPPING, "hdr.vlan_tag$0.$valid");
  F_PORT_VLAN_TO_BD_MAPPING_SRC_MAC_ADDRESS =
      key_id_from_name(T_PORT_VLAN_TO_BD_MAPPING, "hdr.ethernet.dst_addr");
  F_PORT_VLAN_TO_BD_MAPPING_PRIORITY =
      key_id_from_name(T_PORT_VLAN_TO_BD_MAPPING, "$MATCH_PRIORITY");
  D_PORT_VLAN_TO_BD_MAPPING_ACTION_MEMBER_ID = data_id_from_name_noaction(
      T_PORT_VLAN_TO_BD_MAPPING, "$ACTION_MEMBER_ID");

  T_PORT_DOUBLE_TAG_TO_BD_MAPPING =
      table_id_from_name("port_double_tag_to_bd_mapping");
  F_PORT_DOUBLE_TAG_TO_BD_MAPPING_LOCAL_MD_INGRESS_PORT_LAG_INDEX =
      key_id_from_name(T_PORT_DOUBLE_TAG_TO_BD_MAPPING,
                       "local_md.ingress_port_lag_index");
  F_PORT_DOUBLE_TAG_TO_BD_MAPPING_VLAN_0_VID =
      key_id_from_name(T_PORT_DOUBLE_TAG_TO_BD_MAPPING, "hdr.vlan_tag$0.vid");
  F_PORT_DOUBLE_TAG_TO_BD_MAPPING_VLAN_0_VALID = key_id_from_name(
      T_PORT_DOUBLE_TAG_TO_BD_MAPPING, "hdr.vlan_tag$0.$valid");
  F_PORT_DOUBLE_TAG_TO_BD_MAPPING_VLAN_1_VID =
      key_id_from_name(T_PORT_DOUBLE_TAG_TO_BD_MAPPING, "hdr.vlan_tag$1.vid");
  F_PORT_DOUBLE_TAG_TO_BD_MAPPING_VLAN_1_VALID = key_id_from_name(
      T_PORT_DOUBLE_TAG_TO_BD_MAPPING, "hdr.vlan_tag$1.$valid");
  D_PORT_DOUBLE_TAG_TO_BD_MAPPING_ACTION_MEMBER_ID = data_id_from_name_noaction(
      T_PORT_DOUBLE_TAG_TO_BD_MAPPING, "$ACTION_MEMBER_ID");

  T_VLAN_TO_BD_MAPPING = table_id_from_name("vlan_to_bd_mapping");
  F_VLAN_TO_BD_MAPPING_VLAN_0_VID =
      key_id_from_name(T_VLAN_TO_BD_MAPPING, "hdr.vlan_tag$0.vid");
  D_VLAN_TO_BD_MAPPING_ACTION_MEMBER_ID =
      data_id_from_name_noaction(T_VLAN_TO_BD_MAPPING, "$ACTION_MEMBER_ID");

  T_VLAN_MEMBERSHIP = table_id_from_name("vlan_membership");
  F_VLAN_MEMBERSHIP_REGISTER_INDEX =
      key_id_from_name(T_VLAN_MEMBERSHIP, "$REGISTER_INDEX");
  D_VLAN_MEMBERSHIP_REGISTER_DATA =
      data_id_from_name_noaction(T_VLAN_MEMBERSHIP, "vlan_membership.f1");

  T_CPU_TO_BD_MAPPING = table_id_from_name("cpu_to_bd_mapping");
  F_CPU_TO_BD_MAPPING_HDR_CPU_INGRESS_BD =
      key_id_from_name(T_CPU_TO_BD_MAPPING, "hdr.cpu.ingress_bd");
  D_CPU_TO_BD_MAPPING_ACTION_MEMBER_ID =
      data_id_from_name_noaction(T_CPU_TO_BD_MAPPING, "$ACTION_MEMBER_ID");

  AP_BD_ACTION_PROFILE = table_id_from_name("bd_action_profile");
  F_BD_ACTION_PROFILE_ACTION_MEMBER_ID =
      key_id_from_name(AP_BD_ACTION_PROFILE, "$ACTION_MEMBER_ID");
  A_PORT_VLAN_MISS =
      action_id_from_name(AP_BD_ACTION_PROFILE, "port_vlan_miss");
  A_INGRESS_SET_BD_PROPERTIES =
      action_id_from_name(AP_BD_ACTION_PROFILE, "set_bd_properties");
  P_INGRESS_SET_BD_PROPERTIES_BD = data_id_from_name(
      AP_BD_ACTION_PROFILE, A_INGRESS_SET_BD_PROPERTIES, "bd");
  P_INGRESS_SET_BD_PROPERTIES_VRF_TTL_VIOLATION = data_id_from_name(
      AP_BD_ACTION_PROFILE, A_INGRESS_SET_BD_PROPERTIES, "vrf_ttl_violation");
  P_INGRESS_SET_BD_PROPERTIES_VRF_TTL_VIOLATION_VALID =
      data_id_from_name(AP_BD_ACTION_PROFILE,
                        A_INGRESS_SET_BD_PROPERTIES,
                        "vrf_ttl_violation_valid");
  P_INGRESS_SET_BD_PROPERTIES_VRF_IP_OPTIONS_VIOLATION =
      data_id_from_name(AP_BD_ACTION_PROFILE,
                        A_INGRESS_SET_BD_PROPERTIES,
                        "vrf_ip_options_violation");
  P_INGRESS_SET_BD_PROPERTIES_VRF_UNKNOWN_L3_MULTICAST_TRAP =
      data_id_from_name(AP_BD_ACTION_PROFILE,
                        A_INGRESS_SET_BD_PROPERTIES,
                        "vrf_unknown_l3_multicast_trap");
  P_INGRESS_SET_BD_PROPERTIES_VRF = data_id_from_name(
      AP_BD_ACTION_PROFILE, A_INGRESS_SET_BD_PROPERTIES, "vrf");
  P_INGRESS_SET_BD_PROPERTIES_BD_LABEL = data_id_from_name(
      AP_BD_ACTION_PROFILE, A_INGRESS_SET_BD_PROPERTIES, "bd_label");
  P_INGRESS_SET_BD_PROPERTIES_STP_GROUP = data_id_from_name(
      AP_BD_ACTION_PROFILE, A_INGRESS_SET_BD_PROPERTIES, "stp_group");
  P_INGRESS_SET_BD_PROPERTIES_LEARNING_MODE = data_id_from_name(
      AP_BD_ACTION_PROFILE, A_INGRESS_SET_BD_PROPERTIES, "learning_mode");
  P_INGRESS_SET_BD_PROPERTIES_IPV4_UNICAST_ENABLE = data_id_from_name(
      AP_BD_ACTION_PROFILE, A_INGRESS_SET_BD_PROPERTIES, "ipv4_unicast_enable");
  P_INGRESS_SET_BD_PROPERTIES_IPV6_UNICAST_ENABLE = data_id_from_name(
      AP_BD_ACTION_PROFILE, A_INGRESS_SET_BD_PROPERTIES, "ipv6_unicast_enable");
  P_INGRESS_SET_BD_PROPERTIES_VLAN_ARP_SUPPRESS_ENABLE = data_id_from_name(
      AP_BD_ACTION_PROFILE, A_INGRESS_SET_BD_PROPERTIES, "vlan_arp_suppress");
  P_INGRESS_SET_BD_PROPERTIES_IPV4_MULTICAST_ENABLE =
      data_id_from_name(AP_BD_ACTION_PROFILE,
                        A_INGRESS_SET_BD_PROPERTIES,
                        "ipv4_multicast_enable");
  P_INGRESS_SET_BD_PROPERTIES_IPV6_MULTICAST_ENABLE =
      data_id_from_name(AP_BD_ACTION_PROFILE,
                        A_INGRESS_SET_BD_PROPERTIES,
                        "ipv6_multicast_enable");
  P_INGRESS_SET_BD_PROPERTIES_IGMP_SNOOPING_ENABLE =
      data_id_from_name(AP_BD_ACTION_PROFILE,
                        A_INGRESS_SET_BD_PROPERTIES,
                        "igmp_snooping_enable");
  P_INGRESS_SET_BD_PROPERTIES_MPLS_ENABLE = data_id_from_name(
      AP_BD_ACTION_PROFILE, A_INGRESS_SET_BD_PROPERTIES, "mpls_enable");
  P_INGRESS_SET_BD_PROPERTIES_MLD_SNOOPING_ENABLE = data_id_from_name(
      AP_BD_ACTION_PROFILE, A_INGRESS_SET_BD_PROPERTIES, "mld_snooping_enable");
  P_INGRESS_SET_BD_PROPERTIES_MRPF_GROUP = data_id_from_name(
      AP_BD_ACTION_PROFILE, A_INGRESS_SET_BD_PROPERTIES, "mrpf_group");
  P_INGRESS_SET_BD_PROPERTIES_NAT_ZONE = data_id_from_name(
      AP_BD_ACTION_PROFILE, A_INGRESS_SET_BD_PROPERTIES, "zone");
  // Only one NO_ACTION is published in json
  A_NO_ACTION = action_id_from_name(AP_BD_ACTION_PROFILE, "NoAction");

  T_EGRESS_PORT_MAPPING = table_id_from_name("egress_port_mapping");
  F_EGRESS_PORT_MAPPING_PORT = key_id_from_name(T_EGRESS_PORT_MAPPING, "port");
  A_PORT_NORMAL = action_id_from_name(T_EGRESS_PORT_MAPPING, "set_port_normal");
  P_PORT_NORMAL_PORT_LAG_INDEX =
      data_id_from_name(T_EGRESS_PORT_MAPPING, A_PORT_NORMAL, "port_lag_index");
  P_PORT_NORMAL_PORT_LAG_LABEL =
      data_id_from_name(T_EGRESS_PORT_MAPPING, A_PORT_NORMAL, "port_lag_label");
  P_PORT_NORMAL_OUT_PORTS_GROUP_LABEL_IPV4 = data_id_from_name(
      T_EGRESS_PORT_MAPPING, A_PORT_NORMAL, "out_ports_group_label_ipv4");
  P_PORT_NORMAL_OUT_PORTS_GROUP_LABEL_IPV6 = data_id_from_name(
      T_EGRESS_PORT_MAPPING, A_PORT_NORMAL, "out_ports_group_label_ipv6");
  A_PORT_CPU = action_id_from_name(T_EGRESS_PORT_MAPPING, "set_port_cpu");

  T_SNAKE = table_id_from_name("snake");
  F_SNAKE_INGRESS_PORT = key_id_from_name(T_SNAKE, "local_md.ingress_port");
  A_SNAKE_SET_EGRESS_PORT = action_id_from_name(T_SNAKE, "set_egress_port");
  P_SNAKE_SET_EGRESS_PORT_EGRESS_PORT =
      data_id_from_name(T_SNAKE, A_SNAKE_SET_EGRESS_PORT, "port");

  T_EGRESS_EGRESS_INGRESS_PORT_MAPPING =
      table_id_from_name("egress_ingress_port_mapping");
  F_EGRESS_EGRESS_INGRESS_PORT_MAPPING_LOCAL_MD_INGRESS_PORT = key_id_from_name(
      T_EGRESS_EGRESS_INGRESS_PORT_MAPPING, "local_md.ingress_port");
  A_SET_EGRESS_INGRESS_PORT_PROPERTIES =
      action_id_from_name(T_EGRESS_EGRESS_INGRESS_PORT_MAPPING,
                          "set_egress_ingress_port_properties");
  D_SET_INGRESS_PORT_PROPERTIES_PORT_ISOLATION_GROUP =
      data_id_from_name(T_EGRESS_EGRESS_INGRESS_PORT_MAPPING,
                        A_SET_EGRESS_INGRESS_PORT_PROPERTIES,
                        "port_isolation_group");
  D_SET_INGRESS_PORT_PROPERTIES_BRIDGE_PORT_ISOLATION_GROUP =
      data_id_from_name(T_EGRESS_EGRESS_INGRESS_PORT_MAPPING,
                        A_SET_EGRESS_INGRESS_PORT_PROPERTIES,
                        "bport_isolation_group");

  T_EGRESS_EGRESS_PORT_ISOLATION = table_id_from_name("egress_port_isolation");
  F_EGRESS_EGRESS_PORT_ISOLATION_EG_INTR_MD_EGRESS_PORT = key_id_from_name(
      T_EGRESS_EGRESS_PORT_ISOLATION, "eg_intr_md.egress_port");
  F_EGRESS_EGRESS_PORT_ISOLATION_LOCAL_MD_PORT_ISOLATION_GROUP =
      key_id_from_name(T_EGRESS_EGRESS_PORT_ISOLATION,
                       "local_md.port_isolation_group");
  A_ISOLATE_PACKET_PORT = action_id_from_name(T_EGRESS_EGRESS_PORT_ISOLATION,
                                              "isolate_packet_port");
  D_ISOLATE_PACKET_PORT_DROP = data_id_from_name(
      T_EGRESS_EGRESS_PORT_ISOLATION, A_ISOLATE_PACKET_PORT, "drop");

  T_EGRESS_EGRESS_BRIDGE_PORT_ISOLATION =
      table_id_from_name("egress_bport_isolation");
  F_EGRESS_EGRESS_BRIDGE_PORT_ISOLATION_EG_INTR_MD_EGRESS_PORT =
      key_id_from_name(T_EGRESS_EGRESS_BRIDGE_PORT_ISOLATION,
                       "eg_intr_md.egress_port");
  F_EGRESS_EGRESS_BRIDGE_PORT_ISOLATION_LOCAL_MD_FLAGS_ROUTED =
      key_id_from_name(T_EGRESS_EGRESS_BRIDGE_PORT_ISOLATION,
                       "local_md.flags.routed");
  F_EGRESS_EGRESS_BRIDGE_PORT_ISOLATION_LOCAL_MD_BRIDGE_PORT_ISOLATION_GROUP =
      key_id_from_name(T_EGRESS_EGRESS_BRIDGE_PORT_ISOLATION,
                       "local_md.bport_isolation_group");
  A_ISOLATE_PACKET_BPORT = action_id_from_name(
      T_EGRESS_EGRESS_BRIDGE_PORT_ISOLATION, "isolate_packet_bport");
  D_ISOLATE_PACKET_BPORT_DROP = data_id_from_name(
      T_EGRESS_EGRESS_BRIDGE_PORT_ISOLATION, A_ISOLATE_PACKET_BPORT, "drop");

  T_EGRESS_PEER_LINK_TUNNEL_ISOLATION =
      table_id_from_name("peer_link_tunnel_isolation");
  F_EGRESS_PEER_LINK_TUNNEL_ISOLATION_LOCAL_MD_BPORT_ISOLATION_GROUP =
      key_id_from_name(T_EGRESS_PEER_LINK_TUNNEL_ISOLATION,
                       "local_md.bport_isolation_group");
  A_PEER_LINK_ISOLATE = action_id_from_name(T_EGRESS_PEER_LINK_TUNNEL_ISOLATION,
                                            "peer_link_isolate");
  D_PEER_LINK_ISOLATE_DROP = data_id_from_name(
      T_EGRESS_PEER_LINK_TUNNEL_ISOLATION, A_PEER_LINK_ISOLATE, "drop");

  T_LAG = table_id_from_name("lag_table");
  F_LAG_PORT_LAG_INDEX = key_id_from_name(T_LAG, "port_lag_index");
  F_LAG_PRIORITY = key_id_from_name(T_LAG, "$MATCH_PRIORITY");
  D_LAG_ACTION_MEMBER_ID =
      data_id_from_name_noaction(T_LAG, "$ACTION_MEMBER_ID");
  D_LAG_SELECTOR_GROUP_ID =
      data_id_from_name_noaction(T_LAG, "$SELECTOR_GROUP_ID");

  AP_LAG_SELECTOR = table_id_from_name("lag_action_profile");
  F_LAG_SELECTOR_ACTION_MEMBER_ID =
      key_id_from_name(AP_LAG_SELECTOR, "$ACTION_MEMBER_ID");
  A_LAG_MISS = action_id_from_name(AP_LAG_SELECTOR, "lag_miss");
  A_SET_LAG_PORT = action_id_from_name(AP_LAG_SELECTOR, "set_lag_port");
  P_SET_LAG_PORT_PORT =
      data_id_from_name(AP_LAG_SELECTOR, A_SET_LAG_PORT, "port");

  SG_LAG_SELECTOR_GROUP = table_id_from_name("lag_selector");
  F_LAG_SELECTOR_GROUP_ID =
      key_id_from_name(SG_LAG_SELECTOR_GROUP, "$SELECTOR_GROUP_ID");
  P_LAG_SELECTOR_GROUP_MAX_MEMBER_ARRAY =
      data_id_from_name_noaction(SG_LAG_SELECTOR_GROUP, "$ACTION_MEMBER_ID");
  P_LAG_SELECTOR_GROUP_MAX_MEMBER_STATUS_ARRAY = data_id_from_name_noaction(
      SG_LAG_SELECTOR_GROUP, "$ACTION_MEMBER_STATUS");
  P_LAG_SELECTOR_GROUP_MAX_GROUP_SIZE =
      data_id_from_name_noaction(SG_LAG_SELECTOR_GROUP, "$MAX_GROUP_SIZE");

  T_EGRESS_CPU_PORT_REWRITE = table_id_from_name("cpu_port_rewrite");
  F_EGRESS_CPU_PORT_REWRITE_PORT =
      key_id_from_name(T_EGRESS_CPU_PORT_REWRITE, "port");
  A_CPU_REWRITE = action_id_from_name(T_EGRESS_CPU_PORT_REWRITE, "cpu_rewrite");

  /* Port State tables for internal pipes */
  T_INGRESS_PORT_STATE_EG_1 =
      table_id_from_name("port_bd_state_eg_1.port_state");
  F_INGRESS_PORT_STATE_EG_1_INGRESS_PORT =
      key_id_from_name(T_INGRESS_PORT_STATE_EG_1, "local_md.ingress_port");
  T_INGRESS_PORT_STATE_IG_1 =
      table_id_from_name("port_bd_state_ig_1.port_state");
  F_INGRESS_PORT_STATE_IG_1_INGRESS_PORT =
      key_id_from_name(T_INGRESS_PORT_STATE_IG_1, "local_md.ingress_port");

  A_INGRESS_PORT_STATE_IG_1_SET_PORT_PROPERTIES =
      action_id_from_name(T_INGRESS_PORT_STATE_IG_1, "set_port_state");
  P_INGRESS_PORT_STATE_IG_1_SET_PORT_PROPERTIES_PORT_LAG_INDEX =
      data_id_from_name(T_INGRESS_PORT_STATE_IG_1,
                        A_INGRESS_PORT_STATE_IG_1_SET_PORT_PROPERTIES,
                        "port_lag_index");
  P_INGRESS_PORT_STATE_IG_1_SET_PORT_PROPERTIES_PORT_LAG_LABEL =
      data_id_from_name(T_INGRESS_PORT_STATE_IG_1,
                        A_INGRESS_PORT_STATE_IG_1_SET_PORT_PROPERTIES,
                        "port_lag_label");
  P_INGRESS_PORT_STATE_IG_1_SET_PORT_PROPERTIES_EXCLUSION_ID =
      data_id_from_name(T_INGRESS_PORT_STATE_IG_1,
                        A_INGRESS_PORT_STATE_IG_1_SET_PORT_PROPERTIES,
                        "exclusion_id");
  P_INGRESS_PORT_STATE_IG_1_SET_PORT_PROPERTIES_METER_INDEX =
      data_id_from_name(T_INGRESS_PORT_STATE_IG_1,
                        A_INGRESS_PORT_STATE_IG_1_SET_PORT_PROPERTIES,
                        "meter_index");
  P_INGRESS_PORT_STATE_IG_1_SET_PORT_PROPERTIES_SFLOW_SESSION_ID =
      data_id_from_name(T_INGRESS_PORT_STATE_IG_1,
                        A_INGRESS_PORT_STATE_IG_1_SET_PORT_PROPERTIES,
                        "sflow_session_id");
  A_INGRESS_PORT_STATE_IG_1_SET_CPU_PORT_PROPERTIES =
      action_id_from_name(T_INGRESS_PORT_STATE_IG_1, "set_cpu_port_state");
  P_INGRESS_PORT_STATE_IG_1_SET_CPU_PORT_PROPERTIES_PORT_LAG_INDEX =
      data_id_from_name(T_INGRESS_PORT_STATE_IG_1,
                        A_INGRESS_PORT_STATE_IG_1_SET_CPU_PORT_PROPERTIES,
                        "port_lag_index");
  P_INGRESS_PORT_STATE_IG_1_SET_CPU_PORT_PROPERTIES_PORT_LAG_LABEL =
      data_id_from_name(T_INGRESS_PORT_STATE_IG_1,
                        A_INGRESS_PORT_STATE_IG_1_SET_CPU_PORT_PROPERTIES,
                        "port_lag_label");
  P_INGRESS_PORT_STATE_IG_1_SET_CPU_PORT_PROPERTIES_EXCLUSION_ID =
      data_id_from_name(T_INGRESS_PORT_STATE_IG_1,
                        A_INGRESS_PORT_STATE_IG_1_SET_CPU_PORT_PROPERTIES,
                        "exclusion_id");
  P_INGRESS_PORT_STATE_IG_1_SET_CPU_PORT_PROPERTIES_METER_INDEX =
      data_id_from_name(T_INGRESS_PORT_STATE_IG_1,
                        A_INGRESS_PORT_STATE_IG_1_SET_CPU_PORT_PROPERTIES,
                        "meter_index");
  P_INGRESS_PORT_STATE_IG_1_SET_CPU_PORT_PROPERTIES_SFLOW_SESSION_ID =
      data_id_from_name(T_INGRESS_PORT_STATE_IG_1,
                        A_INGRESS_PORT_STATE_IG_1_SET_CPU_PORT_PROPERTIES,
                        "sflow_session_id");

  A_INGRESS_PORT_STATE_EG_1_SET_PORT_PROPERTIES =
      action_id_from_name(T_INGRESS_PORT_STATE_EG_1, "set_port_state");
  P_INGRESS_PORT_STATE_EG_1_SET_PORT_PROPERTIES_PORT_LAG_INDEX =
      data_id_from_name(T_INGRESS_PORT_STATE_EG_1,
                        A_INGRESS_PORT_STATE_EG_1_SET_PORT_PROPERTIES,
                        "port_lag_index");
  P_INGRESS_PORT_STATE_EG_1_SET_PORT_PROPERTIES_PORT_LAG_LABEL =
      data_id_from_name(T_INGRESS_PORT_STATE_EG_1,
                        A_INGRESS_PORT_STATE_EG_1_SET_PORT_PROPERTIES,
                        "port_lag_label");
  //  P_INGRESS_PORT_STATE_EG_1_SET_PORT_PROPERTIES_EXCLUSION_ID =
  //      data_id_from_name(T_INGRESS_PORT_STATE_EG_1,
  //                        A_INGRESS_PORT_STATE_EG_1_SET_PORT_PROPERTIES,
  //                        "exclusion_id");
  //  P_INGRESS_PORT_STATE_EG_1_SET_PORT_PROPERTIES_METER_INDEX =
  //      data_id_from_name(T_INGRESS_PORT_STATE_EG_1,
  //                        A_INGRESS_PORT_STATE_EG_1_SET_PORT_PROPERTIES,
  //                        "meter_index");
  //  P_INGRESS_PORT_STATE_EG_1_SET_PORT_PROPERTIES_SFLOW_SESSION_ID =
  //      data_id_from_name(T_INGRESS_PORT_STATE_EG_1,
  //                        A_INGRESS_PORT_STATE_EG_1_SET_PORT_PROPERTIES,
  //                        "sflow_session_id");
  A_INGRESS_PORT_STATE_EG_1_SET_CPU_PORT_PROPERTIES =
      action_id_from_name(T_INGRESS_PORT_STATE_EG_1, "set_cpu_port_state");
  P_INGRESS_PORT_STATE_EG_1_SET_CPU_PORT_PROPERTIES_PORT_LAG_INDEX =
      data_id_from_name(T_INGRESS_PORT_STATE_EG_1,
                        A_INGRESS_PORT_STATE_EG_1_SET_CPU_PORT_PROPERTIES,
                        "port_lag_index");
  P_INGRESS_PORT_STATE_EG_1_SET_CPU_PORT_PROPERTIES_PORT_LAG_LABEL =
      data_id_from_name(T_INGRESS_PORT_STATE_EG_1,
                        A_INGRESS_PORT_STATE_EG_1_SET_CPU_PORT_PROPERTIES,
                        "port_lag_label");
  //  P_INGRESS_PORT_STATE_EG_1_SET_CPU_PORT_PROPERTIES_EXCLUSION_ID =
  //      data_id_from_name(T_INGRESS_PORT_STATE_EG_1,
  //                        A_INGRESS_PORT_STATE_EG_1_SET_CPU_PORT_PROPERTIES,
  //                        "exclusion_id");
  //  P_INGRESS_PORT_STATE_EG_1_SET_CPU_PORT_PROPERTIES_METER_INDEX =
  //      data_id_from_name(T_INGRESS_PORT_STATE_EG_1,
  //                        A_INGRESS_PORT_STATE_EG_1_SET_CPU_PORT_PROPERTIES,
  //                        "meter_index");
  //  P_INGRESS_PORT_STATE_EG_1_SET_CPU_PORT_PROPERTIES_SFLOW_SESSION_ID =
  //      data_id_from_name(T_INGRESS_PORT_STATE_EG_1,
  //                        A_INGRESS_PORT_STATE_EG_1_SET_CPU_PORT_PROPERTIES,
  //                        "sflow_session_id");

  /* BD State tables for internal pipes */
  //  T_INGRESS_BD_STATE_EG_1 =
  //  table_id_from_name("port_bd_state_eg_1.bd_state");
  //  F_INGRESS_BD_STATE_EG_1_BD =
  //      key_id_from_name(T_INGRESS_BD_STATE_EG_1, "local_md.bd[12:0]");
  T_INGRESS_BD_STATE_IG_1 = table_id_from_name("port_bd_state_ig_1.bd_state");
  F_INGRESS_BD_STATE_IG_1_BD =
      key_id_from_name(T_INGRESS_BD_STATE_IG_1, "local_md.bd[12:0]");

  A_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES =
      action_id_from_name(T_INGRESS_BD_STATE_IG_1, "set_bd_properties");
  P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_BD_LABEL =
      data_id_from_name(T_INGRESS_BD_STATE_IG_1,
                        A_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES,
                        "bd_label");
  P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_IPV4_UNICAST_ENABLE =
      data_id_from_name(T_INGRESS_BD_STATE_IG_1,
                        A_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES,
                        "ipv4_unicast_enable");
  P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_IPV4_MULTICAST_ENABLE =
      data_id_from_name(T_INGRESS_BD_STATE_IG_1,
                        A_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES,
                        "ipv4_multicast_enable");
  P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_IGMP_SNOOPING_ENABLE =
      data_id_from_name(T_INGRESS_BD_STATE_IG_1,
                        A_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES,
                        "igmp_snooping_enable");
  P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_IPV6_UNICAST_ENABLE =
      data_id_from_name(T_INGRESS_BD_STATE_IG_1,
                        A_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES,
                        "ipv6_unicast_enable");
  P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_IPV6_MULTICAST_ENABLE =
      data_id_from_name(T_INGRESS_BD_STATE_IG_1,
                        A_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES,
                        "ipv6_multicast_enable");
  P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_MLD_SNOOPING_ENABLE =
      data_id_from_name(T_INGRESS_BD_STATE_IG_1,
                        A_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES,
                        "mld_snooping_enable");
  P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_MPLS_ENABLE =
      data_id_from_name(T_INGRESS_BD_STATE_IG_1,
                        A_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES,
                        "mpls_enable");
  P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_VLAN_ARP_SUPPRESS =
      data_id_from_name(T_INGRESS_BD_STATE_IG_1,
                        A_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES,
                        "vlan_arp_suppress");
  P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_VRF =
      data_id_from_name(T_INGRESS_BD_STATE_IG_1,
                        A_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES,
                        "vrf");
  P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_VRF_TTL_VIOLATION =
      data_id_from_name(T_INGRESS_BD_STATE_IG_1,
                        A_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES,
                        "vrf_ttl_violation");
  P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_VRF_TTL_VIOLATION_VALID =
      data_id_from_name(T_INGRESS_BD_STATE_IG_1,
                        A_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES,
                        "vrf_ttl_violation_valid");
  P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_VRF_IP_OPTIONS_VIOLATION =
      data_id_from_name(T_INGRESS_BD_STATE_IG_1,
                        A_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES,
                        "vrf_ip_options_violation");
  P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_NAT_ZONE =
      data_id_from_name(T_INGRESS_BD_STATE_IG_1,
                        A_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES,
                        "zone");
  /*
  A_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES =
      action_id_from_name(T_INGRESS_BD_STATE_EG_1, "set_bd_properties");
  P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_BD_LABEL =
      data_id_from_name(T_INGRESS_BD_STATE_EG_1,
                        A_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES,
                        "bd_label");
  P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_IPV4_UNICAST_ENABLE =
      data_id_from_name(T_INGRESS_BD_STATE_EG_1,
                        A_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES,
                        "ipv4_unicast_enable");
  P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_IPV4_MULTICAST_ENABLE =
      data_id_from_name(T_INGRESS_BD_STATE_EG_1,
                        A_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES,
                        "ipv4_multicast_enable");
  P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_IGMP_SNOOPING_ENABLE =
      data_id_from_name(T_INGRESS_BD_STATE_EG_1,
                        A_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES,
                        "igmp_snooping_enable");
  P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_IPV6_UNICAST_ENABLE =
      data_id_from_name(T_INGRESS_BD_STATE_EG_1,
                        A_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES,
                        "ipv6_unicast_enable");
  P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_IPV6_MULTICAST_ENABLE =
      data_id_from_name(T_INGRESS_BD_STATE_EG_1,
                        A_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES,
                        "ipv6_multicast_enable");
  P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_MLD_SNOOPING_ENABLE =
      data_id_from_name(T_INGRESS_BD_STATE_EG_1,
                        A_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES,
                        "mld_snooping_enable");
  P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_MPLS_ENABLE =
      data_id_from_name(T_INGRESS_BD_STATE_EG_1,
                        A_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES,
                        "mpls_enable");
  P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_VLAN_ARP_SUPPRESS =
      data_id_from_name(T_INGRESS_BD_STATE_EG_1,
                        A_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES,
                        "vlan_arp_suppress");
  P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_VRF =
      data_id_from_name(T_INGRESS_BD_STATE_EG_1,
                        A_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES,
                        "vrf");
  P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_VRF_TTL_VIOLATION =
      data_id_from_name(T_INGRESS_BD_STATE_EG_1,
                        A_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES,
                        "vrf_ttl_violation");
  P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_VRF_TTL_VIOLATION_VALID =
      data_id_from_name(T_INGRESS_BD_STATE_EG_1,
                        A_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES,
                        "vrf_ttl_violation_valid");
  P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_VRF_IP_OPTIONS_VIOLATION =
      data_id_from_name(T_INGRESS_BD_STATE_EG_1,
                        A_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES,
                        "vrf_ip_options_violation");
  P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_NAT_ZONE =
      data_id_from_name(T_INGRESS_BD_STATE_EG_1,
                        A_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES,
                          "zone");
  */

  /* l2.p4 */
  T_INGRESS_STP_GROUP = table_id_from_name("ingress_stp.mstp");
  F_INGRESS_STP_GROUP_PORT_LAG_INDEX =
      key_id_from_name(T_INGRESS_STP_GROUP, "local_md.ingress_port");
  F_INGRESS_STP_GROUP = key_id_from_name(T_INGRESS_STP_GROUP, "stp_md.group");
  A_INGRESS_STP_SET_STP_STATE =
      action_id_from_name(T_INGRESS_STP_GROUP, "ingress_stp.set_stp_state");
  P_INGRESS_STP_SET_STP_STATE_STP_STATE = data_id_from_name(
      T_INGRESS_STP_GROUP, A_INGRESS_STP_SET_STP_STATE, "stp_state");

  T_EGRESS_STP_GROUP = table_id_from_name("egress_stp.mstp");
  F_EGRESS_STP_GROUP_PORT_LAG_INDEX =
      key_id_from_name(T_EGRESS_STP_GROUP, "eg_intr_md.egress_port");
  F_EGRESS_STP_GROUP = key_id_from_name(T_EGRESS_STP_GROUP, "stp_md.group");
  A_EGRESS_STP_SET_STP_STATE =
      action_id_from_name(T_EGRESS_STP_GROUP, "egress_stp.set_stp_state");
  P_EGRESS_STP_SET_STP_STATE_STP_STATE = data_id_from_name(
      T_EGRESS_STP_GROUP, A_EGRESS_STP_SET_STP_STATE, "stp_state");

  T_INGRESS_STP0_CHECK = table_id_from_name("ingress_stp.stp0");
  F_INGRESS_STP0_CHECK_REGISTER_INDEX =
      key_id_from_name(T_INGRESS_STP0_CHECK, "$REGISTER_INDEX");
  D_INGRESS_STP0_CHECK_REGISTER_DATA =
      data_id_from_name_noaction(T_INGRESS_STP0_CHECK, "ingress_stp.stp0.f1");

  T_INGRESS_STP1_CHECK = table_id_from_name("ingress_stp.stp1");
  F_INGRESS_STP1_CHECK_REGISTER_INDEX =
      key_id_from_name(T_INGRESS_STP1_CHECK, "$REGISTER_INDEX");
  D_INGRESS_STP1_CHECK_REGISTER_DATA =
      data_id_from_name_noaction(T_INGRESS_STP1_CHECK, "ingress_stp.stp1.f1");

  T_EGRESS_STP_CHECK = table_id_from_name("egress_stp.stp");
  F_EGRESS_STP_CHECK_REGISTER_INDEX =
      key_id_from_name(T_EGRESS_STP_CHECK, "$REGISTER_INDEX");
  D_EGRESS_STP_CHECK_REGISTER_DATA =
      data_id_from_name_noaction(T_EGRESS_STP_CHECK, "egress_stp.stp.f1");

  T_SMAC = table_id_from_name("smac");
  F_SMAC_LOCAL_MD_BD = key_id_from_name(T_SMAC, "local_md.bd");
  F_SMAC_SRC_ADDR = key_id_from_name(T_SMAC, "src_addr");
  A_SMAC_HIT = action_id_from_name(T_SMAC, "smac_hit");
  P_SMAC_HIT_PORT_LAG_INDEX =
      data_id_from_name(T_SMAC, A_SMAC_HIT, "port_lag_index");
  D_SMAC_TTL = data_id_from_name(T_SMAC, A_SMAC_HIT, "$ENTRY_TTL");

  T_DMAC = table_id_from_name("dmac");
  F_DMAC_LOCAL_MD_BD = key_id_from_name(T_DMAC, "local_md.bd");
  F_DMAC_DST_ADDR = key_id_from_name(T_DMAC, "dst_addr");
  A_DMAC_MISS = action_id_from_name(T_DMAC, "dmac_miss");
  A_DMAC_HIT = action_id_from_name(T_DMAC, "dmac_hit");
  P_DMAC_HIT_PORT_LAG_INDEX =
      data_id_from_name(T_DMAC, A_DMAC_HIT, "port_lag_index");
  A_DMAC_MULTICAST = action_id_from_name(T_DMAC, "dmac_multicast");
  P_DMAC_MULTICAST_INDEX = data_id_from_name(T_DMAC, A_DMAC_MULTICAST, "index");
  A_DMAC_REDIRECT = action_id_from_name(T_DMAC, "dmac_redirect");
  P_DMAC_REDIRECT_NEXTHOP_INDEX =
      data_id_from_name(T_DMAC, A_DMAC_REDIRECT, "nexthop_index");

  T_INGRESS_BD_STATS = table_id_from_name("ingress_bd_stats");
  F_INGRESS_BD_STATS_BD = key_id_from_name(T_INGRESS_BD_STATS, "bd");
  F_INGRESS_BD_STATS_PKT_TYPE =
      key_id_from_name(T_INGRESS_BD_STATS, "pkt_type");
  A_INGRESS_BD_STATS_COUNT =
      action_id_from_name(T_INGRESS_BD_STATS, "ingress_bd_stats_count");
  P_INGRESS_BD_STATS_BYTES = data_id_from_name(
      T_INGRESS_BD_STATS, A_INGRESS_BD_STATS_COUNT, "$COUNTER_SPEC_BYTES");
  P_INGRESS_BD_STATS_PKTS = data_id_from_name(
      T_INGRESS_BD_STATS, A_INGRESS_BD_STATS_COUNT, "$COUNTER_SPEC_PKTS");

  T_EGRESS_BD_STATS = table_id_from_name("egress_bd_stats");
  F_EGRESS_BD_STATS_LOCAL_MD_BD =
      key_id_from_name(T_EGRESS_BD_STATS, "local_md.bd[12:0]");
  F_EGRESS_BD_STATS_LOCAL_MD_PKT_TYPE =
      key_id_from_name(T_EGRESS_BD_STATS, "local_md.lkp.pkt_type");
  A_EGRESS_BD_STATS_COUNT =
      action_id_from_name(T_EGRESS_BD_STATS, "egress_bd_stats_count");
  P_EGRESS_BD_STATS_BYTES = data_id_from_name(
      T_EGRESS_BD_STATS, A_EGRESS_BD_STATS_COUNT, "$COUNTER_SPEC_BYTES");
  P_EGRESS_BD_STATS_PKTS = data_id_from_name(
      T_EGRESS_BD_STATS, A_EGRESS_BD_STATS_COUNT, "$COUNTER_SPEC_PKTS");

  T_EGRESS_BD_MAPPING = table_id_from_name("egress_bd_mapping");
  F_EGRESS_BD_MAPPING_LOCAL_MD_BD =
      key_id_from_name(T_EGRESS_BD_MAPPING, "local_md.bd[12:0]");
  A_EGRESS_SET_BD_PROPERTIES =
      action_id_from_name(T_EGRESS_BD_MAPPING, "set_egress_bd_mapping");
  P_EGRESS_SET_BD_PROPERTIES_SMAC = data_id_from_name(
      T_EGRESS_BD_MAPPING, A_EGRESS_SET_BD_PROPERTIES, "smac");
  P_EGRESS_SET_BD_PROPERTIES_MTU =
      data_id_from_name(T_EGRESS_BD_MAPPING, A_EGRESS_SET_BD_PROPERTIES, "mtu");
  P_EGRESS_SET_BD_PROPERTIES_BD_LABEL = data_id_from_name(
      T_EGRESS_BD_MAPPING, A_EGRESS_SET_BD_PROPERTIES, "bd_label");

  T_VLAN_DECAP = table_id_from_name("vlan_decap");
  F_VLAN_DECAP_PORT = key_id_from_name(T_VLAN_DECAP, "port");
  F_VLAN_DECAP_VLAN_0_VALID =
      key_id_from_name(T_VLAN_DECAP, "hdr.vlan_tag$0.$valid");
  F_VLAN_DECAP_PRIORITY = key_id_from_name(T_VLAN_DECAP, "$MATCH_PRIORITY");
  A_REMOVE_VLAN_TAG = action_id_from_name(T_VLAN_DECAP, "remove_vlan_tag");
  A_REMOVE_DOUBLE_TAG = action_id_from_name(T_VLAN_DECAP, "remove_double_tag");
  F_VLAN_DECAP_VLAN_1_VALID =
      key_id_from_name(T_VLAN_DECAP, "hdr.vlan_tag$1.$valid");

  T_PORT_BD_TO_VLAN_MAPPING = table_id_from_name("port_bd_to_vlan_mapping");
  F_PORT_BD_TO_VLAN_MAPPING_PORT_LAG_INDEX =
      key_id_from_name(T_PORT_BD_TO_VLAN_MAPPING, "port_lag_index");
  F_PORT_BD_TO_VLAN_MAPPING_BD =
      key_id_from_name(T_PORT_BD_TO_VLAN_MAPPING, "bd");
  A_PORT_BD_TO_VLAN_MAPPING_SET_VLAN_UNTAGGED =
      action_id_from_name(T_PORT_BD_TO_VLAN_MAPPING, "set_vlan_untagged");
  A_PORT_BD_TO_VLAN_MAPPING_SET_VLAN_TAGGED =
      action_id_from_name(T_PORT_BD_TO_VLAN_MAPPING, "set_vlan_tagged");
  P_PORT_BD_TO_VLAN_MAPPING_SET_VLAN_TAGGED_VID =
      data_id_from_name(T_PORT_BD_TO_VLAN_MAPPING,
                        A_PORT_BD_TO_VLAN_MAPPING_SET_VLAN_TAGGED,
                        "vid");
  A_PORT_BD_TO_VLAN_MAPPING_SET_DOUBLE_TAGGED =
      action_id_from_name(T_PORT_BD_TO_VLAN_MAPPING, "set_double_tagged");
  P_PORT_BD_TO_VLAN_MAPPING_SET_DOUBLE_TAGGED_VID0 =
      data_id_from_name(T_PORT_BD_TO_VLAN_MAPPING,
                        A_PORT_BD_TO_VLAN_MAPPING_SET_DOUBLE_TAGGED,
                        "vid0");
  P_PORT_BD_TO_VLAN_MAPPING_SET_DOUBLE_TAGGED_VID1 =
      data_id_from_name(T_PORT_BD_TO_VLAN_MAPPING,
                        A_PORT_BD_TO_VLAN_MAPPING_SET_DOUBLE_TAGGED,
                        "vid1");

  T_BD_TO_VLAN_MAPPING = table_id_from_name("bd_to_vlan_mapping");
  F_BD_TO_VLAN_MAPPING_BD = key_id_from_name(T_BD_TO_VLAN_MAPPING, "bd");
  A_BD_TO_VLAN_MAPPING_SET_VLAN_UNTAGGED =
      action_id_from_name(T_BD_TO_VLAN_MAPPING, "set_vlan_untagged");
  A_BD_TO_VLAN_MAPPING_SET_VLAN_TAGGED =
      action_id_from_name(T_BD_TO_VLAN_MAPPING, "set_vlan_tagged");
  P_BD_TO_VLAN_MAPPING_SET_VLAN_TAGGED_VID = data_id_from_name(
      T_BD_TO_VLAN_MAPPING, A_BD_TO_VLAN_MAPPING_SET_VLAN_TAGGED, "vid");

  /* l3.p4 */
  T_IPV4_FIB_HOST = table_id_from_name("ipv4_host");
  F_IPV4_FIB_VRF = key_id_from_name(T_IPV4_FIB_HOST, "vrf");
  F_IPV4_FIB_VRF_SIZE = key_size_from_id(T_IPV4_FIB_HOST, F_IPV4_FIB_VRF);
  F_IPV4_FIB_DST_ADDR = key_id_from_name(T_IPV4_FIB_HOST, "ip_dst_addr");

  /* ipv4 local host table */
  T_IPV4_FIB_LOCAL_HOST = table_id_from_name("ipv4_local_host");
  F_IPV4_FIB_LOCAL_HOST_VRF = key_id_from_name(T_IPV4_FIB_LOCAL_HOST, "vrf");
  F_IPV4_FIB_LOCAL_HOST_DST_ADDR =
      key_id_from_name(T_IPV4_FIB_LOCAL_HOST, "ip_dst_addr");

  T_IPV4_FIB_LPM = table_id_from_name("ipv4_lpm");
  F_IPV4_FIB_LPM_VRF = key_id_from_name(T_IPV4_FIB_LPM, "vrf");
  F_IPV4_FIB_LPM_DST_ADDR = key_id_from_name(T_IPV4_FIB_LPM, "ip_dst_addr");

  T_IPV6_FIB_HOST = table_id_from_name("ipv6_host");
  F_IPV6_FIB_VRF = key_id_from_name(T_IPV6_FIB_HOST, "vrf");
  F_IPV6_FIB_DST_ADDR = key_id_from_name(T_IPV6_FIB_HOST, "ip_dst_addr");

  T_IPV6_FIB_HOST64 = table_id_from_name("ipv6_host64");
  F_IPV6_FIB_HOST64_VRF = key_id_from_name(T_IPV6_FIB_HOST64, "vrf");
  F_IPV6_FIB_HOST64_DST_ADDR =
      key_id_from_name(T_IPV6_FIB_HOST64, "ip_dst_addr");

  T_IPV6_FIB_LPM = table_id_from_name("ipv6_lpm128");
  F_IPV6_FIB_LPM_VRF = key_id_from_name(T_IPV6_FIB_LPM, "vrf");
  F_IPV6_FIB_LPM_DST_ADDR = key_id_from_name(T_IPV6_FIB_LPM, "ip_dst_addr");

  T_IPV6_FIB_LPM_TCAM = table_id_from_name("ipv6_lpm_tcam");
  F_IPV6_FIB_LPM_TCAM_VRF = key_id_from_name(T_IPV6_FIB_LPM_TCAM, "vrf");
  F_IPV6_FIB_LPM_TCAM_DST_ADDR =
      key_id_from_name(T_IPV6_FIB_LPM_TCAM, "ip_dst_addr");

  T_IPV6_FIB_LPM64 = table_id_from_name("ipv6_lpm64");
  F_IPV6_FIB_LPM64_VRF = key_id_from_name(T_IPV6_FIB_LPM64, "vrf");
  F_IPV6_FIB_LPM64_DST_ADDR = key_id_from_name(T_IPV6_FIB_LPM64, "ip_dst_addr");

  T_IP_FIB_LPM64 = table_id_from_name("ip_lpm64");
  F_IP_FIB_LPM64_VRF = key_id_from_name(T_IP_FIB_LPM64, "vrf");
  F_IP_FIB_LPM64_DST_ADDR = key_id_from_name(T_IP_FIB_LPM64, "ip_dst_addr");

  // common fib actions
  A_FIB_HIT = action_id_from_name(T_IPV4_FIB_HOST, "fib_hit");
  P_FIB_HIT_NEXTHOP_INDEX =
      data_id_from_name(T_IPV4_FIB_HOST, A_FIB_HIT, "nexthop_index");
  P_FIB_HIT_FIB_LABEL =
      data_id_from_name(T_IPV4_FIB_HOST, A_FIB_HIT, "fib_label");
  A_FIB_DROP = action_id_from_name(T_IPV4_FIB_HOST, "fib_drop");
  A_FIB_MYIP = action_id_from_name(T_IPV4_FIB_HOST, "fib_myip");
  P_FIB_MYIP_MYIP = data_id_from_name(T_IPV4_FIB_HOST, A_FIB_MYIP, "myip");

  /* nexthop.p4 */
  T_ECMP = table_id_from_name("ecmp_table");
  F_ECMP_LOCAL_MD_NEXTHOP = key_id_from_name(T_ECMP, "local_md.nexthop");
  D_ECMP_ACTION_MEMBER_ID =
      data_id_from_name_noaction(T_ECMP, "$ACTION_MEMBER_ID");
  D_ECMP_SELECTOR_GROUP_ID =
      data_id_from_name_noaction(T_ECMP, "$SELECTOR_GROUP_ID");

  T_NEXTHOP = table_id_from_name("nexthop");
  F_NEXTHOP_LOCAL_MD_NEXTHOP = key_id_from_name(T_NEXTHOP, "local_md.nexthop");
  F_NEXTHOP_INDEX_WIDTH =
      key_size_from_id(T_NEXTHOP, F_NEXTHOP_LOCAL_MD_NEXTHOP);
  A_SET_NEXTHOP_PROPERTIES =
      action_id_from_name(T_NEXTHOP, "nexthop_set_nexthop_properties");
  P_SET_NEXTHOP_PROPERTIES_PORT_LAG_INDEX =
      data_id_from_name(T_NEXTHOP, A_SET_NEXTHOP_PROPERTIES, "port_lag_index");
  P_SET_NEXTHOP_PROPERTIES_BD =
      data_id_from_name(T_NEXTHOP, A_SET_NEXTHOP_PROPERTIES, "bd");
  A_SET_NEXTHOP_PROPERTIES_PR_FLOOD = action_id_from_name(
      T_NEXTHOP, "set_nexthop_properties_post_routed_flood");
  P_SET_NEXTHOP_PROPERTIES_PR_FLOOD_BD =
      data_id_from_name(T_NEXTHOP, A_SET_NEXTHOP_PROPERTIES_PR_FLOOD, "bd");
  P_SET_NEXTHOP_PROPERTIES_PR_FLOOD_MGID =
      data_id_from_name(T_NEXTHOP, A_SET_NEXTHOP_PROPERTIES_PR_FLOOD, "mgid");
  P_SET_NEXTHOP_PROPERTIES_PR_FLOOD_NAT_ZONE =
      data_id_from_name(T_NEXTHOP, A_SET_NEXTHOP_PROPERTIES_PR_FLOOD, "zone");
  A_SET_NEXTHOP_PROPERTIES_GLEAN =
      action_id_from_name(T_NEXTHOP, "set_nexthop_properties_glean");
  P_SET_NEXTHOP_PROPERTIES_GLEAN_TRAP_ID =
      data_id_from_name(T_NEXTHOP, A_SET_NEXTHOP_PROPERTIES_GLEAN, "trap_id");
  A_SET_NEXTHOP_PROPERTIES_DROP =
      action_id_from_name(T_NEXTHOP, "set_nexthop_properties_drop");
  P_SET_NEXTHOP_PROPERTIES_DROP_DROP_REASON = data_id_from_name(
      T_NEXTHOP, A_SET_NEXTHOP_PROPERTIES_DROP, "drop_reason");
  P_SET_NEXTHOP_PROPERTIES_NAT_ZONE =
      data_id_from_name(T_NEXTHOP, A_SET_NEXTHOP_PROPERTIES, "zone");
  A_SET_NEXTHOP_PROPERTIES_TUNNEL =
      action_id_from_name(T_NEXTHOP, "set_nexthop_properties_tunnel");
  P_SET_NEXTHOP_PROPERTIES_TUNNEL_DIP_INDEX = data_id_from_name(
      T_NEXTHOP, A_SET_NEXTHOP_PROPERTIES_TUNNEL, "dip_index");

  T_OUTER_FIB = table_id_from_name("outer_fib");
  F_OUTER_FIB_LOCAL_MD_TUNNEL_DIP_INDEX =
      key_id_from_name(T_OUTER_FIB, "local_md.tunnel.dip_index");
  D_OUTER_FIB_ACTION_MEMBER_ID =
      data_id_from_name_noaction(T_OUTER_FIB, "$ACTION_MEMBER_ID");
  D_OUTER_FIB_SELECTOR_GROUP_ID =
      data_id_from_name_noaction(T_OUTER_FIB, "$SELECTOR_GROUP_ID");

  AP_OUTER_ECMP_SELECTOR = table_id_from_name("outer_fib_ecmp_action_profile");
  F_OUTER_ECMP_SELECTOR_ACTION_MEMBER_ID =
      key_id_from_name(AP_OUTER_ECMP_SELECTOR, "$ACTION_MEMBER_ID");
  A_SET_OUTER_NEXTHOP_PROPERTIES = action_id_from_name(
      AP_OUTER_ECMP_SELECTOR, "outer_fib_set_nexthop_properties");
  P_SET_OUTER_NEXTHOP_PROPERTIES_PORT_LAG_INDEX = data_id_from_name(
      AP_OUTER_ECMP_SELECTOR, A_SET_OUTER_NEXTHOP_PROPERTIES, "port_lag_index");
  P_SET_OUTER_NEXTHOP_PROPERTIES_NEXTHOP_INDEX = data_id_from_name(
      AP_OUTER_ECMP_SELECTOR, A_SET_OUTER_NEXTHOP_PROPERTIES, "nexthop_index");

  SG_OUTER_ECMP_SELECTOR_GROUP = table_id_from_name("outer_fib_ecmp_selector");
  F_OUTER_ECMP_SELECTOR_GROUP_ID =
      key_id_from_name(SG_OUTER_ECMP_SELECTOR_GROUP, "$SELECTOR_GROUP_ID");
  P_OUTER_ECMP_SELECTOR_GROUP_MAX_MEMBER_ARRAY = data_id_from_name_noaction(
      SG_OUTER_ECMP_SELECTOR_GROUP, "$ACTION_MEMBER_ID");
  P_OUTER_ECMP_SELECTOR_GROUP_MAX_MEMBER_STATUS_ARRAY =
      data_id_from_name_noaction(SG_OUTER_ECMP_SELECTOR_GROUP,
                                 "$ACTION_MEMBER_STATUS");
  P_OUTER_ECMP_SELECTOR_GROUP_MAX_GROUP_SIZE = data_id_from_name_noaction(
      SG_OUTER_ECMP_SELECTOR_GROUP, "$MAX_GROUP_SIZE");

  AP_ECMP_SELECTOR = table_id_from_name("nexthop_ecmp_action_profile");
  F_ECMP_SELECTOR_ACTION_MEMBER_ID =
      key_id_from_name(AP_ECMP_SELECTOR, "$ACTION_MEMBER_ID");
  A_SET_ECMP_PROPERTIES =
      action_id_from_name(AP_ECMP_SELECTOR, "set_ecmp_properties");
  P_SET_ECMP_PROPERTIES_PORT_LAG_INDEX = data_id_from_name(
      AP_ECMP_SELECTOR, A_SET_ECMP_PROPERTIES, "port_lag_index");
  P_SET_ECMP_PROPERTIES_BD =
      data_id_from_name(AP_ECMP_SELECTOR, A_SET_ECMP_PROPERTIES, "bd");
  P_SET_ECMP_PROPERTIES_NEXTHOP_INDEX = data_id_from_name(
      AP_ECMP_SELECTOR, A_SET_ECMP_PROPERTIES, "nexthop_index");
  A_SET_ECMP_PROPERTIES_DROP =
      action_id_from_name(AP_ECMP_SELECTOR, "set_ecmp_properties_drop");
  A_SET_ECMP_PROPERTIES_TUNNEL =
      action_id_from_name(AP_ECMP_SELECTOR, "set_ecmp_properties_tunnel");
  P_SET_ECMP_PROPERTIES_TUNNEL_DIP_INDEX = data_id_from_name(
      AP_ECMP_SELECTOR, A_SET_ECMP_PROPERTIES_TUNNEL, "dip_index");
  P_SET_ECMP_PROPERTIES_TUNNEL_NEXTHOP_INDEX = data_id_from_name(
      AP_ECMP_SELECTOR, A_SET_ECMP_PROPERTIES_TUNNEL, "nexthop_index");
  A_SET_ECMP_PROPERTIES_GLEAN =
      action_id_from_name(AP_ECMP_SELECTOR, "set_ecmp_properties_glean");
  P_SET_ECMP_PROPERTIES_GLEAN_TRAP_ID = data_id_from_name(
      AP_ECMP_SELECTOR, A_SET_ECMP_PROPERTIES_GLEAN, "trap_id");

  SG_ECMP_SELECTOR_GROUP = table_id_from_name("nexthop_ecmp_selector");
  F_ECMP_SELECTOR_GROUP_ID =
      key_id_from_name(SG_ECMP_SELECTOR_GROUP, "$SELECTOR_GROUP_ID");
  P_ECMP_SELECTOR_GROUP_MAX_MEMBER_ARRAY =
      data_id_from_name_noaction(SG_ECMP_SELECTOR_GROUP, "$ACTION_MEMBER_ID");
  P_ECMP_SELECTOR_GROUP_MAX_MEMBER_STATUS_ARRAY = data_id_from_name_noaction(
      SG_ECMP_SELECTOR_GROUP, "$ACTION_MEMBER_STATUS");
  P_ECMP_SELECTOR_GROUP_MAX_GROUP_SIZE =
      data_id_from_name_noaction(SG_ECMP_SELECTOR_GROUP, "$MAX_GROUP_SIZE");

  T_NEIGHBOR = table_id_from_name("neighbor");
  F_NEIGHBOR_LOCAL_MD_NEXTHOP =
      key_id_from_name(T_NEIGHBOR, "local_md.nexthop");
  A_NEIGHBOR_REWRITE_L2 =
      action_id_from_name(T_NEIGHBOR, "neighbor_rewrite_l2");
  P_NEIGHBOR_REWRITE_L2_DMAC =
      data_id_from_name(T_NEIGHBOR, A_NEIGHBOR_REWRITE_L2, "dmac");

  T_OUTER_NEXTHOP = table_id_from_name("outer_nexthop");
  F_OUTER_NEXTHOP_LOCAL_MD_NEXTHOP =
      key_id_from_name(T_OUTER_NEXTHOP, "local_md.nexthop");
  A_OUTER_NEXTHOP_REWRITE_L2 =
      action_id_from_name(T_OUTER_NEXTHOP, "outer_nexthop_rewrite_l2");
  P_OUTER_NEXTHOP_REWRITE_L2_BD =
      data_id_from_name(T_OUTER_NEXTHOP, A_OUTER_NEXTHOP_REWRITE_L2, "bd");

  /* acl.p4 */
  T_INGRESS_MAC_ACL = table_id_from_name("ingress_mac_acl.acl");
  F_INGRESS_MAC_ACL_LOCAL_MD_LKP_MAC_SRC_ADDR =
      key_id_from_name(T_INGRESS_MAC_ACL, "local_md.lkp.mac_src_addr");
  F_INGRESS_MAC_ACL_HDR_MAC_SRC_ADDR =
      key_id_from_name(T_INGRESS_MAC_ACL, "hdr.ethernet.src_addr");
  F_INGRESS_MAC_ACL_LOCAL_MD_LKP_MAC_DST_ADDR =
      key_id_from_name(T_INGRESS_MAC_ACL, "local_md.lkp.mac_dst_addr");
  F_INGRESS_MAC_ACL_HDR_MAC_DST_ADDR =
      key_id_from_name(T_INGRESS_MAC_ACL, "hdr.ethernet.dst_addr");
  F_INGRESS_MAC_ACL_LOCAL_MD_LKP_MAC_TYPE =
      key_id_from_name(T_INGRESS_MAC_ACL, "local_md.lkp.mac_type");
  F_INGRESS_MAC_ACL_LOCAL_MD_LKP_PCP =
      key_id_from_name(T_INGRESS_MAC_ACL, "local_md.lkp.pcp");
  F_INGRESS_MAC_ACL_LOCAL_MD_LKP_DEI =
      key_id_from_name(T_INGRESS_MAC_ACL, "local_md.lkp.dei");
  F_INGRESS_MAC_ACL_HDR_VLAN_TAG0_VALID =
      key_id_from_name(T_INGRESS_MAC_ACL, "hdr.vlan_tag$0.$valid");
  F_INGRESS_MAC_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL =
      key_id_from_name(T_INGRESS_MAC_ACL, "local_md.ingress_port_lag_label");
  F_INGRESS_MAC_ACL_LOCAL_MD_INGRESS_PORT_LAG_INDEX =
      key_id_from_name(T_INGRESS_MAC_ACL, "local_md.ingress_port_lag_index");
  F_INGRESS_MAC_ACL_LOCAL_MD_BD_LABEL =
      key_id_from_name(T_INGRESS_MAC_ACL, "local_md.bd_label");
  F_INGRESS_MAC_ACL_LOCAL_MD_BD =
      key_id_from_name(T_INGRESS_MAC_ACL, "local_md.bd");
  F_INGRESS_MAC_ACL_LOCAL_MD_FLAGS_RMAC_HIT =
      key_id_from_name(T_INGRESS_MAC_ACL, "local_md.flags.rmac_hit");
  F_INGRESS_MAC_ACL_MATCH_PRIORITY =
      key_id_from_name(T_INGRESS_MAC_ACL, "$MATCH_PRIORITY");
  A_INGRESS_MAC_ACL_NO_ACTION =
      action_id_from_name(T_INGRESS_MAC_ACL, "ingress_mac_acl.no_action");
  A_INGRESS_MAC_ACL_DROP =
      action_id_from_name(T_INGRESS_MAC_ACL, "ingress_mac_acl.drop");
  A_INGRESS_MAC_ACL_PERMIT =
      action_id_from_name(T_INGRESS_MAC_ACL, "ingress_mac_acl.permit");
  P_INGRESS_MAC_ACL_PERMIT_USER_METADATA = data_id_from_name(
      T_INGRESS_MAC_ACL, A_INGRESS_MAC_ACL_PERMIT, "user_metadata");
  P_INGRESS_MAC_ACL_PERMIT_METER_INDEX = data_id_from_name(
      T_INGRESS_MAC_ACL, A_INGRESS_MAC_ACL_PERMIT, "meter_index");
  P_INGRESS_MAC_ACL_PERMIT_TRAP_ID =
      data_id_from_name(T_INGRESS_MAC_ACL, A_INGRESS_MAC_ACL_PERMIT, "trap_id");
  A_INGRESS_MAC_ACL_REDIRECT_NEXTHOP = action_id_from_name(
      T_INGRESS_MAC_ACL, "ingress_mac_acl.redirect_nexthop");
  P_INGRESS_MAC_ACL_REDIRECT_NEXTHOP_INDEX = data_id_from_name(
      T_INGRESS_MAC_ACL, A_INGRESS_MAC_ACL_REDIRECT_NEXTHOP, "nexthop_index");
  P_INGRESS_MAC_ACL_REDIRECT_NEXTHOP_USER_METADATA = data_id_from_name(
      T_INGRESS_MAC_ACL, A_INGRESS_MAC_ACL_REDIRECT_NEXTHOP, "user_metadata");
  A_INGRESS_MAC_ACL_REDIRECT_PORT =
      action_id_from_name(T_INGRESS_MAC_ACL, "ingress_mac_acl.redirect_port");
  P_INGRESS_MAC_ACL_REDIRECT_PORT_LAG_INDEX =
      data_id_from_name(T_INGRESS_MAC_ACL,
                        A_INGRESS_MAC_ACL_REDIRECT_PORT,
                        "egress_port_lag_index");
  P_INGRESS_MAC_ACL_REDIRECT_PORT_USER_METADATA = data_id_from_name(
      T_INGRESS_MAC_ACL, A_INGRESS_MAC_ACL_REDIRECT_PORT, "user_metadata");
  A_INGRESS_MAC_ACL_MIRROR =
      action_id_from_name(T_INGRESS_MAC_ACL, "ingress_mac_acl.mirror_in");
  P_INGRESS_MAC_ACL_MIRROR_METER_INDEX = data_id_from_name(
      T_INGRESS_MAC_ACL, A_INGRESS_MAC_ACL_MIRROR, "meter_index");
  P_INGRESS_MAC_ACL_MIRROR_SESSION_ID = data_id_from_name(
      T_INGRESS_MAC_ACL, A_INGRESS_MAC_ACL_MIRROR, "session_id");
  A_INGRESS_MAC_ACL_SET_TC =
      action_id_from_name(T_INGRESS_MAC_ACL, "ingress_mac_acl.set_tc");
  P_INGRESS_MAC_ACL_SET_TC_TC =
      data_id_from_name(T_INGRESS_MAC_ACL, A_INGRESS_MAC_ACL_SET_TC, "tc");
  A_INGRESS_MAC_ACL_SET_COLOR =
      action_id_from_name(T_INGRESS_MAC_ACL, "ingress_mac_acl.set_color");
  P_INGRESS_MAC_ACL_SET_COLOR_COLOR = data_id_from_name(
      T_INGRESS_MAC_ACL, A_INGRESS_MAC_ACL_SET_COLOR, "color");
  A_INGRESS_MAC_ACL_TRAP =
      action_id_from_name(T_INGRESS_MAC_ACL, "ingress_mac_acl.trap");
  P_INGRESS_MAC_ACL_TRAP_TRAP_ID =
      data_id_from_name(T_INGRESS_MAC_ACL, A_INGRESS_MAC_ACL_TRAP, "trap_id");
  P_INGRESS_MAC_ACL_TRAP_METER_INDEX = data_id_from_name(
      T_INGRESS_MAC_ACL, A_INGRESS_MAC_ACL_TRAP, "meter_index");
  A_INGRESS_MAC_ACL_COPY =
      action_id_from_name(T_INGRESS_MAC_ACL, "ingress_mac_acl.copy_to_cpu");
  P_INGRESS_MAC_ACL_COPY_TRAP_ID =
      data_id_from_name(T_INGRESS_MAC_ACL, A_INGRESS_MAC_ACL_COPY, "trap_id");
  P_INGRESS_MAC_ACL_COPY_METER_INDEX = data_id_from_name(
      T_INGRESS_MAC_ACL, A_INGRESS_MAC_ACL_COPY, "meter_index");
  A_INGRESS_MAC_ACL_SET_DTEL_REPORT_TYPE = action_id_from_name(
      T_INGRESS_MAC_ACL, "ingress_mac_acl.set_dtel_report_type");
  P_INGRESS_MAC_ACL_DTEL_REPORT_TYPE = data_id_from_name(
      T_INGRESS_MAC_ACL, A_INGRESS_MAC_ACL_SET_DTEL_REPORT_TYPE, "type");
  A_INGRESS_MAC_ACL_TRANSIT =
      action_id_from_name(T_INGRESS_MAC_ACL, "ingress_mac_acl.transit");
  A_INGRESS_MAC_ACL_DENY =
      action_id_from_name(T_INGRESS_MAC_ACL, "ingress_mac_acl.deny");
  A_INGRESS_MAC_ACL_NO_NAT =
      action_id_from_name(T_INGRESS_MAC_ACL, "ingress_mac_acl.disable_nat");
  P_INGRESS_MAC_ACL_NO_NAT_DISABLE_NAT =
      data_id_from_name(T_INGRESS_MAC_ACL, A_INGRESS_MAC_ACL_NO_NAT, "nat_dis");
  D_INGRESS_MAC_ACL_COUNTER_SPEC_BYTES = data_id_from_name(
      T_INGRESS_MAC_ACL, A_INGRESS_MAC_ACL_NO_ACTION, "$COUNTER_SPEC_BYTES");
  D_INGRESS_MAC_ACL_COUNTER_SPEC_PKTS = data_id_from_name(
      T_INGRESS_MAC_ACL, A_INGRESS_MAC_ACL_NO_ACTION, "$COUNTER_SPEC_PKTS");

  T_PRE_INGRESS_ACL = table_id_from_name("pre_ingress_acl");
  F_PRE_INGRESS_ACL_LOCAL_MD_LKP_MAC_SRC_ADDR =
      key_id_from_name(T_PRE_INGRESS_ACL, "hdr.ethernet.src_addr");
  F_PRE_INGRESS_ACL_LOCAL_MD_LKP_MAC_DST_ADDR =
      key_id_from_name(T_PRE_INGRESS_ACL, "hdr.ethernet.dst_addr");
  F_PRE_INGRESS_ACL_LOCAL_MD_LKP_MAC_TYPE =
      key_id_from_name(T_PRE_INGRESS_ACL, "local_md.lkp.mac_type");
  F_PRE_INGRESS_ACL_LOCAL_MD_LKP_IP_SRC_ADDR =
      key_id_from_name(T_PRE_INGRESS_ACL, "local_md.lkp.ip_src_addr");
  F_PRE_INGRESS_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD3 =
      key_id_from_name(T_PRE_INGRESS_ACL, "local_md.lkp.ip_src_addr[127:96]");
  F_PRE_INGRESS_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD2 =
      key_id_from_name(T_PRE_INGRESS_ACL, "local_md.lkp.ip_src_addr[95:64]");
  F_PRE_INGRESS_ACL_LOCAL_MD_LKP_IP_DST_ADDR =
      key_id_from_name(T_PRE_INGRESS_ACL, "local_md.lkp.ip_dst_addr");
  F_PRE_INGRESS_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD3 =
      key_id_from_name(T_PRE_INGRESS_ACL, "local_md.lkp.ip_dst_addr[127:96]");
  F_PRE_INGRESS_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD2 =
      key_id_from_name(T_PRE_INGRESS_ACL, "local_md.lkp.ip_dst_addr[95:64]");
  F_PRE_INGRESS_ACL_LOCAL_MD_LKP_IP_TOS =
      key_id_from_name(T_PRE_INGRESS_ACL, "local_md.lkp.ip_tos");
  F_PRE_INGRESS_ACL_LOCAL_MD_INGRESS_PORT =
      key_id_from_name(T_PRE_INGRESS_ACL, "local_md.ingress_port");
  F_PRE_INGRESS_ACL_MATCH_PRIORITY =
      key_id_from_name(T_PRE_INGRESS_ACL, "$MATCH_PRIORITY");
  A_PRE_INGRESS_ACL_SET_VRF =
      action_id_from_name(T_PRE_INGRESS_ACL, "pre_ingress_acl_set_vrf");
  D_PRE_INGRESS_ACL_SET_VRF_VRF =
      data_id_from_name(T_PRE_INGRESS_ACL, A_PRE_INGRESS_ACL_SET_VRF, "vrf");
  A_PRE_INGRESS_ACL_NO_ACTION =
      action_id_from_name(T_PRE_INGRESS_ACL, "pre_ingress_acl_no_action");
  A_PRE_INGRESS_ACL_DROP =
      action_id_from_name(T_PRE_INGRESS_ACL, "pre_ingress_acl_drop");
  D_PRE_INGRESS_ACL_COUNTER_SPEC_BYTES =
      data_id_from_name_noaction(T_PRE_INGRESS_ACL, "$COUNTER_SPEC_BYTES");
  D_PRE_INGRESS_ACL_COUNTER_SPEC_PKTS =
      data_id_from_name_noaction(T_PRE_INGRESS_ACL, "$COUNTER_SPEC_PKTS");

  T_PRE_INGRESS_DEIVCE_TO_ACL = table_id_from_name("device_to_acl");
  A_PRE_INGRESS_SET_ACL_STATUS =
      action_id_from_name(T_PRE_INGRESS_DEIVCE_TO_ACL, "set_acl_status");
  D_PRE_INGRESS_SET_ACL_STATUS_ENABLED = data_id_from_name(
      T_PRE_INGRESS_DEIVCE_TO_ACL, A_PRE_INGRESS_SET_ACL_STATUS, "enabled");

  T_INGRESS_ACL_ETYPE1 = table_id_from_name("acl_etype1.etype");
  F_INGRESS_ACL_ETYPE1_LOCAL_MD_LKP_MAC_TYPE =
      key_id_from_name(T_INGRESS_ACL_ETYPE1, "local_md.lkp.mac_type");
  A_INGRESS_ACL_ETYPE1_SET_ETYPE_LABEL =
      action_id_from_name(T_INGRESS_ACL_ETYPE1, "acl_etype1.set_etype_label");
  D_INGRESS_ACL_ETYPE1_SET_ETYPE_LABEL = data_id_from_name(
      T_INGRESS_ACL_ETYPE1, A_INGRESS_ACL_ETYPE1_SET_ETYPE_LABEL, "label");

  T_INGRESS_ACL_ETYPE2 = table_id_from_name("acl_etype2.etype");
  F_INGRESS_ACL_ETYPE2_LOCAL_MD_LKP_MAC_TYPE =
      key_id_from_name(T_INGRESS_ACL_ETYPE2, "local_md.lkp.mac_type");
  A_INGRESS_ACL_ETYPE2_SET_ETYPE_LABEL =
      action_id_from_name(T_INGRESS_ACL_ETYPE2, "acl_etype2.set_etype_label");
  D_INGRESS_ACL_ETYPE2_SET_ETYPE_LABEL = data_id_from_name(
      T_INGRESS_ACL_ETYPE2, A_INGRESS_ACL_ETYPE2_SET_ETYPE_LABEL, "label");

  T_INGRESS_ACL_QOS_MACADDR = table_id_from_name("qos_acl_mac.mac");
  F_INGRESS_ACL_QOS_MACADDR_PORT_LAG_INDEX =
      key_id_from_name(T_INGRESS_ACL_QOS_MACADDR, "port_lag_index");
  F_INGRESS_ACL_QOS_MACADDR_SMAC_ADDR =
      key_id_from_name(T_INGRESS_ACL_QOS_MACADDR, "smac_addr");
  F_INGRESS_ACL_QOS_MACADDR_DMAC_ADDR =
      key_id_from_name(T_INGRESS_ACL_QOS_MACADDR, "dmac_addr");
  A_INGRESS_ACL_QOS_MACADDR_SET_MACADDR_LABEL =
      action_id_from_name(T_INGRESS_ACL_QOS_MACADDR, "set_mac_addr_label");
  D_INGRESS_ACL_QOS_MACADDR_SET_MACADDR_LABEL =
      data_id_from_name(T_INGRESS_ACL_QOS_MACADDR,
                        A_INGRESS_ACL_QOS_MACADDR_SET_MACADDR_LABEL,
                        "label");
  F_INGRESS_ACL_QOS_MACADDR_MATCH_PRIORITY =
      key_id_from_name(T_INGRESS_ACL_QOS_MACADDR, "$MATCH_PRIORITY");

  T_INGRESS_ACL_PBR_MACADDR = table_id_from_name("pbr_acl_mac.mac");
  F_INGRESS_ACL_PBR_MACADDR_PORT_LAG_INDEX =
      key_id_from_name(T_INGRESS_ACL_PBR_MACADDR, "port_lag_index");
  F_INGRESS_ACL_PBR_MACADDR_SMAC_ADDR =
      key_id_from_name(T_INGRESS_ACL_PBR_MACADDR, "smac_addr");
  F_INGRESS_ACL_PBR_MACADDR_DMAC_ADDR =
      key_id_from_name(T_INGRESS_ACL_PBR_MACADDR, "dmac_addr");
  A_INGRESS_ACL_PBR_MACADDR_SET_MACADDR_LABEL =
      action_id_from_name(T_INGRESS_ACL_PBR_MACADDR, "set_mac_addr_label");
  D_INGRESS_ACL_PBR_MACADDR_SET_MACADDR_LABEL =
      data_id_from_name(T_INGRESS_ACL_PBR_MACADDR,
                        A_INGRESS_ACL_PBR_MACADDR_SET_MACADDR_LABEL,
                        "label");
  F_INGRESS_ACL_PBR_MACADDR_MATCH_PRIORITY =
      key_id_from_name(T_INGRESS_ACL_PBR_MACADDR, "$MATCH_PRIORITY");

  T_INGRESS_ACL_MIRROR_MACADDR = table_id_from_name("mirror_acl_mac.mac");
  F_INGRESS_ACL_MIRROR_MACADDR_PORT_LAG_INDEX =
      key_id_from_name(T_INGRESS_ACL_MIRROR_MACADDR, "port_lag_index");
  F_INGRESS_ACL_MIRROR_MACADDR_SMAC_ADDR =
      key_id_from_name(T_INGRESS_ACL_MIRROR_MACADDR, "smac_addr");
  F_INGRESS_ACL_MIRROR_MACADDR_DMAC_ADDR =
      key_id_from_name(T_INGRESS_ACL_MIRROR_MACADDR, "dmac_addr");
  A_INGRESS_ACL_MIRROR_MACADDR_SET_MACADDR_LABEL =
      action_id_from_name(T_INGRESS_ACL_MIRROR_MACADDR, "set_mac_addr_label");
  D_INGRESS_ACL_MIRROR_MACADDR_SET_MACADDR_LABEL =
      data_id_from_name(T_INGRESS_ACL_MIRROR_MACADDR,
                        A_INGRESS_ACL_MIRROR_MACADDR_SET_MACADDR_LABEL,
                        "label");
  F_INGRESS_ACL_MIRROR_MACADDR_MATCH_PRIORITY =
      key_id_from_name(T_INGRESS_ACL_MIRROR_MACADDR, "$MATCH_PRIORITY");

  T_INGRESS_IP_ACL = table_id_from_name("ingress_ip_acl.acl");
  F_INGRESS_IP_ACL_LOCAL_MD_ETYPE_LABEL =
      key_id_from_name(T_INGRESS_IP_ACL, "local_md.etype_label");
  F_INGRESS_IP_ACL_LOCAL_MD_PBR_MAC_LABEL =
      key_id_from_name(T_INGRESS_IP_ACL, "local_md.pbr_mac_label");
  F_INGRESS_IP_ACL_LOCAL_MD_LKP_MAC_TYPE =
      key_id_from_name(T_INGRESS_IP_ACL, "local_md.lkp.mac_type");
  F_INGRESS_IP_ACL_LOCAL_MD_INGRESS_PORT =
      key_id_from_name(T_INGRESS_IP_ACL, "local_md.ingress_port");
  F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_SRC_ADDR =
      key_id_from_name(T_INGRESS_IP_ACL, "local_md.lkp.ip_src_addr");
  F_INGRESS_IP_ACL_LOCAL_MD_LKP_PCP =
      key_id_from_name(T_INGRESS_IP_ACL, "local_md.lkp.pcp");
  F_INGRESS_IP_ACL_LOCAL_MD_LKP_DEI =
      key_id_from_name(T_INGRESS_IP_ACL, "local_md.lkp.dei");
  F_INGRESS_IP_ACL_HDR_VLAN_TAG0_VALID =
      key_id_from_name(T_INGRESS_IP_ACL, "hdr.vlan_tag$0.$valid");
  F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD3 =
      key_id_from_name(T_INGRESS_IP_ACL, "local_md.lkp.ip_src_addr[127:96]");
  F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD2 =
      key_id_from_name(T_INGRESS_IP_ACL, "local_md.lkp.ip_src_addr[95:64]");
  F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_DST_ADDR =
      key_id_from_name(T_INGRESS_IP_ACL, "local_md.lkp.ip_dst_addr");
  F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD3 =
      key_id_from_name(T_INGRESS_IP_ACL, "local_md.lkp.ip_dst_addr[127:96]");
  F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD2 =
      key_id_from_name(T_INGRESS_IP_ACL, "local_md.lkp.ip_dst_addr[95:64]");
  F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_PROTO =
      key_id_from_name(T_INGRESS_IP_ACL, "local_md.lkp.ip_proto");
  F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_TOS =
      key_id_from_name(T_INGRESS_IP_ACL, "local_md.lkp.ip_tos");
  F_INGRESS_IP_ACL_LOCAL_MD_LKP_L4_SRC_PORT =
      key_id_from_name(T_INGRESS_IP_ACL, "local_md.lkp.l4_src_port");
  F_INGRESS_IP_ACL_LOCAL_MD_LKP_L4_DST_PORT =
      key_id_from_name(T_INGRESS_IP_ACL, "local_md.lkp.l4_dst_port");
  F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_TTL =
      key_id_from_name(T_INGRESS_IP_ACL, "local_md.lkp.ip_ttl");
  F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_FRAG =
      key_id_from_name(T_INGRESS_IP_ACL, "local_md.lkp.ip_frag");
  F_INGRESS_IP_ACL_LOCAL_MD_LKP_TCP_FLAGS =
      key_id_from_name(T_INGRESS_IP_ACL, "local_md.lkp.tcp_flags");
  F_INGRESS_IP_ACL_LOCAL_MD_FIB_LABEL =
      key_id_from_name(T_INGRESS_IP_ACL, "local_md.fib_label");
  F_INGRESS_IP_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL =
      key_id_from_name(T_INGRESS_IP_ACL, "local_md.ingress_port_lag_label");
  F_INGRESS_IP_ACL_LOCAL_MD_INGRESS_PORT_LAG_INDEX =
      key_id_from_name(T_INGRESS_IP_ACL, "local_md.ingress_port_lag_index");
  F_INGRESS_IP_ACL_LOCAL_MD_BD_LABEL =
      key_id_from_name(T_INGRESS_IP_ACL, "local_md.bd_label");
  F_INGRESS_IP_ACL_LOCAL_MD_BD =
      key_id_from_name(T_INGRESS_IP_ACL, "local_md.bd");
  F_INGRESS_IP_ACL_LOCAL_MD_L4_SRC_PORT_LABEL =
      key_id_from_name(T_INGRESS_IP_ACL, "local_md.l4_src_port_label");
  F_INGRESS_IP_ACL_LOCAL_MD_L4_DST_PORT_LABEL =
      key_id_from_name(T_INGRESS_IP_ACL, "local_md.l4_dst_port_label");
  F_INGRESS_IP_ACL_LOCAL_MD_LKP_MAC_DST_ADDR =
      key_id_from_name(T_INGRESS_IP_ACL, "hdr.ethernet.dst_addr");
  F_INGRESS_IP_ACL_LOCAL_MD_LKP_MAC_SRC_ADDR =
      key_id_from_name(T_INGRESS_IP_ACL, "hdr.ethernet.src_addr");
  F_INGRESS_IP_ACL_LOCAL_MD_FLAGS_RMAC_HIT =
      key_id_from_name(T_INGRESS_IP_ACL, "local_md.flags.rmac_hit");
  F_INGRESS_IP_ACL_MATCH_PRIORITY =
      key_id_from_name(T_INGRESS_IP_ACL, "$MATCH_PRIORITY");
  A_INGRESS_IP_ACL_NO_ACTION =
      action_id_from_name(T_INGRESS_IP_ACL, "ingress_ip_acl.no_action");
  A_INGRESS_IP_ACL_DROP =
      action_id_from_name(T_INGRESS_IP_ACL, "ingress_ip_acl.drop");
  A_INGRESS_IP_ACL_PERMIT =
      action_id_from_name(T_INGRESS_IP_ACL, "ingress_ip_acl.permit");
  P_INGRESS_IP_ACL_PERMIT_USER_METADATA = data_id_from_name(
      T_INGRESS_IP_ACL, A_INGRESS_IP_ACL_PERMIT, "user_metadata");
  P_INGRESS_IP_ACL_PERMIT_METER_INDEX = data_id_from_name(
      T_INGRESS_IP_ACL, A_INGRESS_IP_ACL_PERMIT, "meter_index");
  P_INGRESS_IP_ACL_PERMIT_TRAP_ID =
      data_id_from_name(T_INGRESS_IP_ACL, A_INGRESS_IP_ACL_PERMIT, "trap_id");
  A_INGRESS_IP_ACL_REDIRECT_NEXTHOP =
      action_id_from_name(T_INGRESS_IP_ACL, "ingress_ip_acl.redirect_nexthop");
  P_INGRESS_IP_ACL_REDIRECT_NEXTHOP_INDEX = data_id_from_name(
      T_INGRESS_IP_ACL, A_INGRESS_IP_ACL_REDIRECT_NEXTHOP, "nexthop_index");
  P_INGRESS_IP_ACL_REDIRECT_NEXTHOP_USER_METADATA = data_id_from_name(
      T_INGRESS_IP_ACL, A_INGRESS_IP_ACL_REDIRECT_NEXTHOP, "user_metadata");
  A_INGRESS_IP_ACL_REDIRECT_PORT =
      action_id_from_name(T_INGRESS_IP_ACL, "ingress_ip_acl.redirect_port");
  P_INGRESS_IP_ACL_REDIRECT_PORT_LAG_INDEX =
      data_id_from_name(T_INGRESS_IP_ACL,
                        A_INGRESS_IP_ACL_REDIRECT_PORT,
                        "egress_port_lag_index");
  P_INGRESS_IP_ACL_REDIRECT_PORT_USER_METADATA = data_id_from_name(
      T_INGRESS_IP_ACL, A_INGRESS_IP_ACL_REDIRECT_PORT, "user_metadata");
  A_INGRESS_IP_ACL_MIRROR =
      action_id_from_name(T_INGRESS_IP_ACL, "ingress_ip_acl.mirror_in");
  P_INGRESS_IP_ACL_MIRROR_METER_INDEX = data_id_from_name(
      T_INGRESS_IP_ACL, A_INGRESS_IP_ACL_MIRROR, "meter_index");
  P_INGRESS_IP_ACL_MIRROR_SESSION_ID = data_id_from_name(
      T_INGRESS_IP_ACL, A_INGRESS_IP_ACL_MIRROR, "session_id");
  A_INGRESS_IP_ACL_SET_TC =
      action_id_from_name(T_INGRESS_IP_ACL, "ingress_ip_acl.set_tc");
  P_INGRESS_IP_ACL_SET_TC_TC =
      data_id_from_name(T_INGRESS_IP_ACL, A_INGRESS_IP_ACL_SET_TC, "tc");
  A_INGRESS_IP_ACL_SET_COLOR =
      action_id_from_name(T_INGRESS_IP_ACL, "ingress_ip_acl.set_color");
  P_INGRESS_IP_ACL_SET_COLOR_COLOR =
      data_id_from_name(T_INGRESS_IP_ACL, A_INGRESS_IP_ACL_SET_COLOR, "color");
  A_INGRESS_IP_ACL_TRAP =
      action_id_from_name(T_INGRESS_IP_ACL, "ingress_ip_acl.trap");
  P_INGRESS_IP_ACL_TRAP_TRAP_ID =
      data_id_from_name(T_INGRESS_IP_ACL, A_INGRESS_IP_ACL_TRAP, "trap_id");
  P_INGRESS_IP_ACL_TRAP_METER_INDEX =
      data_id_from_name(T_INGRESS_IP_ACL, A_INGRESS_IP_ACL_TRAP, "meter_index");
  A_INGRESS_IP_ACL_COPY =
      action_id_from_name(T_INGRESS_IP_ACL, "ingress_ip_acl.copy_to_cpu");
  P_INGRESS_IP_ACL_COPY_TRAP_ID =
      data_id_from_name(T_INGRESS_IP_ACL, A_INGRESS_IP_ACL_COPY, "trap_id");
  P_INGRESS_IP_ACL_COPY_METER_INDEX =
      data_id_from_name(T_INGRESS_IP_ACL, A_INGRESS_IP_ACL_COPY, "meter_index");
  D_INGRESS_IP_ACL_COUNTER_SPEC_BYTES = data_id_from_name(
      T_INGRESS_IP_ACL, A_INGRESS_IP_ACL_NO_ACTION, "$COUNTER_SPEC_BYTES");
  D_INGRESS_IP_ACL_COUNTER_SPEC_PKTS = data_id_from_name(
      T_INGRESS_IP_ACL, A_INGRESS_IP_ACL_NO_ACTION, "$COUNTER_SPEC_PKTS");

  T_INGRESS_IPV4_ACL = table_id_from_name("ingress_ipv4_acl.acl");
  F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_MAC_TYPE =
      key_id_from_name(T_INGRESS_IPV4_ACL, "local_md.lkp.mac_type");
  F_INGRESS_IPV4_ACL_LOCAL_MD_INGRESS_PORT =
      key_id_from_name(T_INGRESS_IPV4_ACL, "local_md.ingress_port");
  F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_SRC_ADDR =
      key_id_from_name(T_INGRESS_IPV4_ACL, "local_md.lkp.ip_src_addr[95:64]");
  F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_DST_ADDR =
      key_id_from_name(T_INGRESS_IPV4_ACL, "local_md.lkp.ip_dst_addr[95:64]");
  F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_PROTO =
      key_id_from_name(T_INGRESS_IPV4_ACL, "local_md.lkp.ip_proto");
  F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_TOS =
      key_id_from_name(T_INGRESS_IPV4_ACL, "local_md.lkp.ip_tos");
  F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_L4_SRC_PORT =
      key_id_from_name(T_INGRESS_IPV4_ACL, "local_md.lkp.l4_src_port");
  F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_L4_DST_PORT =
      key_id_from_name(T_INGRESS_IPV4_ACL, "local_md.lkp.l4_dst_port");
  F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_TTL =
      key_id_from_name(T_INGRESS_IPV4_ACL, "local_md.lkp.ip_ttl");
  F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_FRAG =
      key_id_from_name(T_INGRESS_IPV4_ACL, "local_md.lkp.ip_frag");
  F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_TCP_FLAGS =
      key_id_from_name(T_INGRESS_IPV4_ACL, "local_md.lkp.tcp_flags");
  F_INGRESS_IPV4_ACL_LOCAL_MD_FIB_LABEL =
      key_id_from_name(T_INGRESS_IPV4_ACL, "local_md.fib_label");
  F_INGRESS_IPV4_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL =
      key_id_from_name(T_INGRESS_IPV4_ACL, "local_md.ingress_port_lag_label");
  F_INGRESS_IPV4_ACL_LOCAL_MD_INGRESS_PORT_LAG_INDEX =
      key_id_from_name(T_INGRESS_IPV4_ACL, "local_md.ingress_port_lag_index");
  F_INGRESS_IPV4_ACL_LOCAL_MD_BD_LABEL =
      key_id_from_name(T_INGRESS_IPV4_ACL, "local_md.bd_label");
  F_INGRESS_IPV4_ACL_LOCAL_MD_BD =
      key_id_from_name(T_INGRESS_IPV4_ACL, "local_md.bd");
  F_INGRESS_IPV4_ACL_LOCAL_MD_IN_PORTS_GROUP_LABEL = key_id_from_name(
      T_INGRESS_IPV4_ACL, "local_md.in_ports_group_label_ipv4");
  F_INGRESS_IPV4_ACL_LOCAL_MD_L4_SRC_PORT_LABEL =
      key_id_from_name(T_INGRESS_IPV4_ACL, "local_md.l4_src_port_label");
  F_INGRESS_IPV4_ACL_LOCAL_MD_L4_DST_PORT_LABEL =
      key_id_from_name(T_INGRESS_IPV4_ACL, "local_md.l4_dst_port_label");
  F_INGRESS_IPV4_ACL_MATCH_PRIORITY =
      key_id_from_name(T_INGRESS_IPV4_ACL, "$MATCH_PRIORITY");
  F_INGRESS_IPV4_ACL_LOCAL_MD_FLAGS_RMAC_HIT =
      key_id_from_name(T_INGRESS_IPV4_ACL, "local_md.flags.rmac_hit");
  A_INGRESS_IPV4_ACL_NO_ACTION =
      action_id_from_name(T_INGRESS_IPV4_ACL, "ingress_ipv4_acl.no_action");
  A_INGRESS_IPV4_ACL_DROP =
      action_id_from_name(T_INGRESS_IPV4_ACL, "ingress_ipv4_acl.drop");
  A_INGRESS_IPV4_ACL_PERMIT =
      action_id_from_name(T_INGRESS_IPV4_ACL, "ingress_ipv4_acl.permit");
  P_INGRESS_IPV4_ACL_PERMIT_USER_METADATA = data_id_from_name(
      T_INGRESS_IPV4_ACL, A_INGRESS_IPV4_ACL_PERMIT, "user_metadata");
  P_INGRESS_IPV4_ACL_PERMIT_METER_INDEX = data_id_from_name(
      T_INGRESS_IPV4_ACL, A_INGRESS_IPV4_ACL_PERMIT, "meter_index");
  P_INGRESS_IPV4_ACL_PERMIT_TRAP_ID = data_id_from_name(
      T_INGRESS_IPV4_ACL, A_INGRESS_IPV4_ACL_PERMIT, "trap_id");
  A_INGRESS_IPV4_ACL_REDIRECT_NEXTHOP = action_id_from_name(
      T_INGRESS_IPV4_ACL, "ingress_ipv4_acl.redirect_nexthop");
  P_INGRESS_IPV4_ACL_REDIRECT_NEXTHOP_INDEX = data_id_from_name(
      T_INGRESS_IPV4_ACL, A_INGRESS_IPV4_ACL_REDIRECT_NEXTHOP, "nexthop_index");
  P_INGRESS_IPV4_ACL_REDIRECT_NEXTHOP_USER_METADATA = data_id_from_name(
      T_INGRESS_IPV4_ACL, A_INGRESS_IPV4_ACL_REDIRECT_NEXTHOP, "user_metadata");
  A_INGRESS_IPV4_ACL_REDIRECT_PORT =
      action_id_from_name(T_INGRESS_IPV4_ACL, "ingress_ipv4_acl.redirect_port");
  P_INGRESS_IPV4_ACL_REDIRECT_PORT_LAG_INDEX =
      data_id_from_name(T_INGRESS_IPV4_ACL,
                        A_INGRESS_IPV4_ACL_REDIRECT_PORT,
                        "egress_port_lag_index");
  P_INGRESS_IPV4_ACL_REDIRECT_PORT_USER_METADATA = data_id_from_name(
      T_INGRESS_IPV4_ACL, A_INGRESS_IPV4_ACL_REDIRECT_PORT, "user_metadata");
  A_INGRESS_IPV4_ACL_MIRROR =
      action_id_from_name(T_INGRESS_IPV4_ACL, "ingress_ipv4_acl.mirror_in");
  P_INGRESS_IPV4_ACL_MIRROR_METER_INDEX = data_id_from_name(
      T_INGRESS_IPV4_ACL, A_INGRESS_IPV4_ACL_MIRROR, "meter_index");
  P_INGRESS_IPV4_ACL_MIRROR_SESSION_ID = data_id_from_name(
      T_INGRESS_IPV4_ACL, A_INGRESS_IPV4_ACL_MIRROR, "session_id");
  A_INGRESS_IPV4_ACL_SET_TC =
      action_id_from_name(T_INGRESS_IPV4_ACL, "ingress_ipv4_acl.set_tc");
  P_INGRESS_IPV4_ACL_SET_TC_TC =
      data_id_from_name(T_INGRESS_IPV4_ACL, A_INGRESS_IPV4_ACL_SET_TC, "tc");
  A_INGRESS_IPV4_ACL_SET_COLOR =
      action_id_from_name(T_INGRESS_IPV4_ACL, "ingress_ipv4_acl.set_color");
  P_INGRESS_IPV4_ACL_SET_COLOR_COLOR = data_id_from_name(
      T_INGRESS_IPV4_ACL, A_INGRESS_IPV4_ACL_SET_COLOR, "color");
  A_INGRESS_IPV4_ACL_SET_DTEL_REPORT_TYPE = action_id_from_name(
      T_INGRESS_IPV4_ACL, "ingress_ipv4_acl.set_dtel_report_type");
  P_INGRESS_IPV4_ACL_DTEL_REPORT_TYPE = data_id_from_name(
      T_INGRESS_IPV4_ACL, A_INGRESS_IPV4_ACL_SET_DTEL_REPORT_TYPE, "type");
  A_INGRESS_IPV4_ACL_TRAP =
      action_id_from_name(T_INGRESS_IPV4_ACL, "ingress_ipv4_acl.trap");
  P_INGRESS_IPV4_ACL_TRAP_TRAP_ID =
      data_id_from_name(T_INGRESS_IPV4_ACL, A_INGRESS_IPV4_ACL_TRAP, "trap_id");
  P_INGRESS_IPV4_ACL_TRAP_METER_INDEX = data_id_from_name(
      T_INGRESS_IPV4_ACL, A_INGRESS_IPV4_ACL_TRAP, "meter_index");
  A_INGRESS_IPV4_ACL_COPY =
      action_id_from_name(T_INGRESS_IPV4_ACL, "ingress_ipv4_acl.copy_to_cpu");
  P_INGRESS_IPV4_ACL_COPY_TRAP_ID =
      data_id_from_name(T_INGRESS_IPV4_ACL, A_INGRESS_IPV4_ACL_COPY, "trap_id");
  P_INGRESS_IPV4_ACL_COPY_METER_INDEX = data_id_from_name(
      T_INGRESS_IPV4_ACL, A_INGRESS_IPV4_ACL_COPY, "meter_index");
  D_INGRESS_IPV4_ACL_COUNTER_SPEC_BYTES = data_id_from_name(
      T_INGRESS_IPV4_ACL, A_INGRESS_IPV4_ACL_NO_ACTION, "$COUNTER_SPEC_BYTES");
  D_INGRESS_IPV4_ACL_COUNTER_SPEC_PKTS = data_id_from_name(
      T_INGRESS_IPV4_ACL, A_INGRESS_IPV4_ACL_NO_ACTION, "$COUNTER_SPEC_PKTS");
  F_IPV4_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL_7_0 = key_id_from_name(
      T_INGRESS_IPV4_ACL, "local_md.ingress_port_lag_label[7:0]");
  F_IPV4_ACL_LOCAL_MD_INGRESS_PORT_LAG_INDEX =
      key_id_from_name(T_INGRESS_IPV4_ACL, "local_md.ingress_port_lag_index");
  A_INGRESS_IPV4_ACL_TRANSIT =
      action_id_from_name(T_INGRESS_IPV4_ACL, "ingress_ipv4_acl.transit");
  A_INGRESS_IPV4_ACL_DENY =
      action_id_from_name(T_INGRESS_IPV4_ACL, "ingress_ipv4_acl.deny");
  A_INGRESS_IPV4_ACL_NO_NAT =
      action_id_from_name(T_INGRESS_IPV4_ACL, "ingress_ipv4_acl.disable_nat");
  P_INGRESS_IPV4_ACL_NO_NAT_DISABLE_NAT = data_id_from_name(
      T_INGRESS_IPV4_ACL, A_INGRESS_IPV4_ACL_NO_NAT, "nat_dis");

  T_INGRESS_INNER_DTEL_IPV4_ACL =
      table_id_from_name("ingress_inner_ipv4_dtel_acl.acl");
  F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_IPV4_SRC_ADDR = key_id_from_name(
      T_INGRESS_INNER_DTEL_IPV4_ACL, "hdr.inner_ipv4.src_addr");
  F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_IPV4_DST_ADDR = key_id_from_name(
      T_INGRESS_INNER_DTEL_IPV4_ACL, "hdr.inner_ipv4.dst_addr");
  F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_IPV4_PROTOCOL = key_id_from_name(
      T_INGRESS_INNER_DTEL_IPV4_ACL, "hdr.inner_ipv4.protocol");
  F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_IPV4_DIFFSERV = key_id_from_name(
      T_INGRESS_INNER_DTEL_IPV4_ACL, "hdr.inner_ipv4.diffserv");
  F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_TCP_SRC_PORT =
      key_id_from_name(T_INGRESS_INNER_DTEL_IPV4_ACL, "hdr.inner_tcp.src_port");
  F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_TCP_DST_PORT =
      key_id_from_name(T_INGRESS_INNER_DTEL_IPV4_ACL, "hdr.inner_tcp.dst_port");
  F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_UDP_SRC_PORT =
      key_id_from_name(T_INGRESS_INNER_DTEL_IPV4_ACL, "hdr.inner_udp.src_port");
  F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_UDP_DST_PORT =
      key_id_from_name(T_INGRESS_INNER_DTEL_IPV4_ACL, "hdr.inner_udp.dst_port");
  F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_IPV4_TTL =
      key_id_from_name(T_INGRESS_INNER_DTEL_IPV4_ACL, "hdr.inner_ipv4.ttl");
  F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_TCP_FLAGS =
      key_id_from_name(T_INGRESS_INNER_DTEL_IPV4_ACL, "hdr.inner_tcp.flags");
  F_INGRESS_INNER_DTEL_IPV4_ACL_IG_MD_PORT_LAG_LABEL =
      key_id_from_name(T_INGRESS_INNER_DTEL_IPV4_ACL, "ig_md.port_lag_label");
  F_INGRESS_INNER_DTEL_IPV4_ACL_LOCAL_MD_TUNNEL_VNI =
      key_id_from_name(T_INGRESS_INNER_DTEL_IPV4_ACL, "local_md.tunnel.vni");
  A_INGRESS_INNER_DTEL_IPV4_ACL_SET_DTEL_REPORT_TYPE =
      action_id_from_name(T_INGRESS_INNER_DTEL_IPV4_ACL,
                          "ingress_inner_ipv4_dtel_acl.set_dtel_report_type");
  P_INGRESS_INNER_DTEL_IPV4_ACL_DTEL_REPORT_TYPE =
      data_id_from_name(T_INGRESS_INNER_DTEL_IPV4_ACL,
                        A_INGRESS_INNER_DTEL_IPV4_ACL_SET_DTEL_REPORT_TYPE,
                        "type");
  D_INGRESS_INNER_DTEL_IPV4_ACL_COUNTER_SPEC_BYTES =
      data_id_from_name(T_INGRESS_INNER_DTEL_IPV4_ACL,
                        A_INGRESS_INNER_DTEL_IPV4_ACL_SET_DTEL_REPORT_TYPE,
                        "$COUNTER_SPEC_BYTES");
  D_INGRESS_INNER_DTEL_IPV4_ACL_COUNTER_SPEC_PKTS =
      data_id_from_name(T_INGRESS_INNER_DTEL_IPV4_ACL,
                        A_INGRESS_INNER_DTEL_IPV4_ACL_SET_DTEL_REPORT_TYPE,
                        "$COUNTER_SPEC_PKTS");

  T_INGRESS_IPV6_ACL = table_id_from_name("ingress_ipv6_acl.acl");
  F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_MAC_TYPE =
      key_id_from_name(T_INGRESS_IPV6_ACL, "local_md.lkp.mac_type");
  F_INGRESS_IPV6_ACL_LOCAL_MD_INGRESS_PORT =
      key_id_from_name(T_INGRESS_IPV6_ACL, "local_md.ingress_port");
  F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_SRC_ADDR =
      key_id_from_name(T_INGRESS_IPV6_ACL, "local_md.lkp.ip_src_addr");
  F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD3 =
      key_id_from_name(T_INGRESS_IP_ACL, "local_md.lkp.ip_src_addr[127:96]");
  F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD2 =
      key_id_from_name(T_INGRESS_IP_ACL, "local_md.lkp.ip_src_addr[95:64]");
  F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_DST_ADDR =
      key_id_from_name(T_INGRESS_IPV6_ACL, "local_md.lkp.ip_dst_addr");
  F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD3 =
      key_id_from_name(T_INGRESS_IP_ACL, "local_md.lkp.ip_dst_addr[127:96]");
  F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD2 =
      key_id_from_name(T_INGRESS_IP_ACL, "local_md.lkp.ip_dst_addr[95:64]");
  F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_PROTO =
      key_id_from_name(T_INGRESS_IPV6_ACL, "local_md.lkp.ip_proto");
  F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_TOS =
      key_id_from_name(T_INGRESS_IPV6_ACL, "local_md.lkp.ip_tos");
  F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_L4_SRC_PORT =
      key_id_from_name(T_INGRESS_IPV6_ACL, "local_md.lkp.l4_src_port");
  F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_L4_DST_PORT =
      key_id_from_name(T_INGRESS_IPV6_ACL, "local_md.lkp.l4_dst_port");
  F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_TTL =
      key_id_from_name(T_INGRESS_IPV6_ACL, "local_md.lkp.ip_ttl");
  F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_FRAG =
      key_id_from_name(T_INGRESS_IPV6_ACL, "local_md.lkp.ip_frag");
  F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_TCP_FLAGS =
      key_id_from_name(T_INGRESS_IPV6_ACL, "local_md.lkp.tcp_flags");
  F_INGRESS_IPV6_ACL_LOCAL_MD_FIB_LABEL =
      key_id_from_name(T_INGRESS_IPV6_ACL, "local_md.fib_label");
  F_INGRESS_IPV6_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL =
      key_id_from_name(T_INGRESS_IPV6_ACL, "local_md.ingress_port_lag_label");
  F_INGRESS_IPV6_ACL_LOCAL_MD_INGRESS_PORT_LAG_INDEX =
      key_id_from_name(T_INGRESS_IPV6_ACL, "local_md.ingress_port_lag_index");
  F_INGRESS_IPV6_ACL_LOCAL_MD_BD_LABEL =
      key_id_from_name(T_INGRESS_IPV6_ACL, "local_md.bd_label");
  F_INGRESS_IPV6_ACL_LOCAL_MD_BD =
      key_id_from_name(T_INGRESS_IPV6_ACL, "local_md.bd");
  F_INGRESS_IPV6_ACL_LOCAL_MD_L4_SRC_PORT_LABEL =
      key_id_from_name(T_INGRESS_IPV6_ACL, "local_md.l4_src_port_label");
  F_INGRESS_IPV6_ACL_LOCAL_MD_L4_DST_PORT_LABEL =
      key_id_from_name(T_INGRESS_IPV6_ACL, "local_md.l4_dst_port_label");
  F_INGRESS_IPV6_ACL_LOCAL_MD_IN_PORTS_GROUP_LABEL = key_id_from_name(
      T_INGRESS_IPV6_ACL, "local_md.in_ports_group_label_ipv6");
  F_INGRESS_IPV6_ACL_LOCAL_MD_FLAGS_RMAC_HIT =
      key_id_from_name(T_INGRESS_IPV6_ACL, "local_md.flags.rmac_hit");
  F_INGRESS_IPV6_ACL_MATCH_PRIORITY =
      key_id_from_name(T_INGRESS_IPV6_ACL, "$MATCH_PRIORITY");
  A_INGRESS_IPV6_ACL_NO_ACTION =
      action_id_from_name(T_INGRESS_IPV6_ACL, "ingress_ipv6_acl.no_action");
  A_INGRESS_IPV6_ACL_DROP =
      action_id_from_name(T_INGRESS_IPV6_ACL, "ingress_ipv6_acl.drop");
  A_INGRESS_IPV6_ACL_PERMIT =
      action_id_from_name(T_INGRESS_IPV6_ACL, "ingress_ipv6_acl.permit");
  P_INGRESS_IPV6_ACL_PERMIT_USER_METADATA = data_id_from_name(
      T_INGRESS_IPV6_ACL, A_INGRESS_IPV6_ACL_PERMIT, "user_metadata");
  P_INGRESS_IPV6_ACL_PERMIT_METER_INDEX = data_id_from_name(
      T_INGRESS_IPV6_ACL, A_INGRESS_IPV6_ACL_PERMIT, "meter_index");
  P_INGRESS_IPV6_ACL_PERMIT_TRAP_ID = data_id_from_name(
      T_INGRESS_IPV6_ACL, A_INGRESS_IPV6_ACL_PERMIT, "trap_id");
  A_INGRESS_IPV6_ACL_REDIRECT_NEXTHOP = action_id_from_name(
      T_INGRESS_IPV6_ACL, "ingress_ipv6_acl.redirect_nexthop");
  P_INGRESS_IPV6_ACL_REDIRECT_NEXTHOP_INDEX = data_id_from_name(
      T_INGRESS_IPV6_ACL, A_INGRESS_IPV6_ACL_REDIRECT_NEXTHOP, "nexthop_index");
  P_INGRESS_IPV6_ACL_REDIRECT_NEXTHOP_USER_METADATA = data_id_from_name(
      T_INGRESS_IPV6_ACL, A_INGRESS_IPV6_ACL_REDIRECT_NEXTHOP, "user_metadata");
  A_INGRESS_IPV6_ACL_REDIRECT_PORT =
      action_id_from_name(T_INGRESS_IPV6_ACL, "ingress_ipv6_acl.redirect_port");
  P_INGRESS_IPV6_ACL_REDIRECT_PORT_LAG_INDEX =
      data_id_from_name(T_INGRESS_IPV6_ACL,
                        A_INGRESS_IPV6_ACL_REDIRECT_PORT,
                        "egress_port_lag_index");
  P_INGRESS_IPV6_ACL_REDIRECT_PORT_USER_METADATA = data_id_from_name(
      T_INGRESS_IPV6_ACL, A_INGRESS_IPV6_ACL_REDIRECT_PORT, "user_metadata");
  A_INGRESS_IPV6_ACL_MIRROR =
      action_id_from_name(T_INGRESS_IPV6_ACL, "ingress_ipv6_acl.mirror_in");
  P_INGRESS_IPV6_ACL_MIRROR_METER_INDEX = data_id_from_name(
      T_INGRESS_IPV6_ACL, A_INGRESS_IPV6_ACL_MIRROR, "meter_index");
  P_INGRESS_IPV6_ACL_MIRROR_SESSION_ID = data_id_from_name(
      T_INGRESS_IPV6_ACL, A_INGRESS_IPV6_ACL_MIRROR, "session_id");
  A_INGRESS_IPV6_ACL_SET_TC =
      action_id_from_name(T_INGRESS_IPV6_ACL, "ingress_ipv6_acl.set_tc");
  P_INGRESS_IPV6_ACL_SET_TC_TC =
      data_id_from_name(T_INGRESS_IPV6_ACL, A_INGRESS_IPV6_ACL_SET_TC, "tc");
  A_INGRESS_IPV6_ACL_SET_COLOR =
      action_id_from_name(T_INGRESS_IPV6_ACL, "ingress_ipv6_acl.set_color");
  P_INGRESS_IPV6_ACL_SET_COLOR_COLOR = data_id_from_name(
      T_INGRESS_IPV6_ACL, A_INGRESS_IPV6_ACL_SET_COLOR, "color");
  A_INGRESS_IPV6_ACL_SET_DTEL_REPORT_TYPE = action_id_from_name(
      T_INGRESS_IPV6_ACL, "ingress_ipv6_acl.set_dtel_report_type");
  P_INGRESS_IPV6_ACL_DTEL_REPORT_TYPE = data_id_from_name(
      T_INGRESS_IPV6_ACL, A_INGRESS_IPV6_ACL_SET_DTEL_REPORT_TYPE, "type");
  A_INGRESS_IPV6_ACL_TRAP =
      action_id_from_name(T_INGRESS_IPV6_ACL, "ingress_ipv6_acl.trap");
  P_INGRESS_IPV6_ACL_TRAP_TRAP_ID =
      data_id_from_name(T_INGRESS_IPV6_ACL, A_INGRESS_IPV6_ACL_TRAP, "trap_id");
  P_INGRESS_IPV6_ACL_TRAP_METER_INDEX = data_id_from_name(
      T_INGRESS_IPV6_ACL, A_INGRESS_IPV6_ACL_TRAP, "meter_index");
  A_INGRESS_IPV6_ACL_COPY =
      action_id_from_name(T_INGRESS_IPV6_ACL, "ingress_ipv6_acl.copy_to_cpu");
  P_INGRESS_IPV6_ACL_COPY_TRAP_ID =
      data_id_from_name(T_INGRESS_IPV6_ACL, A_INGRESS_IPV6_ACL_COPY, "trap_id");
  P_INGRESS_IPV6_ACL_COPY_METER_INDEX = data_id_from_name(
      T_INGRESS_IPV6_ACL, A_INGRESS_IPV6_ACL_COPY, "meter_index");
  D_INGRESS_IPV6_ACL_COUNTER_SPEC_BYTES = data_id_from_name(
      T_INGRESS_IPV6_ACL, A_INGRESS_IPV6_ACL_NO_ACTION, "$COUNTER_SPEC_BYTES");
  D_INGRESS_IPV6_ACL_COUNTER_SPEC_PKTS = data_id_from_name(
      T_INGRESS_IPV6_ACL, A_INGRESS_IPV6_ACL_NO_ACTION, "$COUNTER_SPEC_PKTS");
  F_IPV6_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL_15_8 = key_id_from_name(
      T_INGRESS_IPV6_ACL, "local_md.ingress_port_lag_label[15:8]");
  F_IPV6_ACL_LOCAL_MD_INGRESS_PORT_LAG_INDEX =
      key_id_from_name(T_INGRESS_IPV6_ACL, "local_md.ingress_port_lag_index");
  A_INGRESS_IPV6_ACL_TRANSIT =
      action_id_from_name(T_INGRESS_IPV6_ACL, "ingress_ipv6_acl.transit");
  A_INGRESS_IPV6_ACL_DENY =
      action_id_from_name(T_INGRESS_IPV6_ACL, "ingress_ipv6_acl.deny");
  A_INGRESS_IPV6_ACL_NO_NAT =
      action_id_from_name(T_INGRESS_IPV6_ACL, "ingress_ipv6_acl.disable_nat");
  P_INGRESS_IPV6_ACL_NO_NAT_DISABLE_NAT = data_id_from_name(
      T_INGRESS_IPV6_ACL, A_INGRESS_IPV6_ACL_NO_NAT, "nat_dis");

  T_INGRESS_INNER_DTEL_IPV6_ACL =
      table_id_from_name("ingress_inner_ipv6_dtel_acl.acl");
  F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_IPV6_SRC_ADDR = key_id_from_name(
      T_INGRESS_INNER_DTEL_IPV6_ACL, "hdr.inner_ipv6.src_addr");
  F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_IPV6_DST_ADDR = key_id_from_name(
      T_INGRESS_INNER_DTEL_IPV6_ACL, "hdr.inner_ipv6.dst_addr");
  F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_IPV6_NEXT_HDR = key_id_from_name(
      T_INGRESS_INNER_DTEL_IPV6_ACL, "hdr.inner_ipv6.next_hdr");
  F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_IPV6_TRAFFIC_CLASS = key_id_from_name(
      T_INGRESS_INNER_DTEL_IPV6_ACL, "hdr.inner_ipv6.traffic_class");
  F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_TCP_SRC_PORT =
      key_id_from_name(T_INGRESS_INNER_DTEL_IPV6_ACL, "hdr.inner_tcp.src_port");
  F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_TCP_DST_PORT =
      key_id_from_name(T_INGRESS_INNER_DTEL_IPV6_ACL, "hdr.inner_tcp.dst_port");
  F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_UDP_SRC_PORT =
      key_id_from_name(T_INGRESS_INNER_DTEL_IPV6_ACL, "hdr.inner_udp.src_port");
  F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_UDP_DST_PORT =
      key_id_from_name(T_INGRESS_INNER_DTEL_IPV6_ACL, "hdr.inner_udp.dst_port");
  F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_IPV6_HOP_LIMIT = key_id_from_name(
      T_INGRESS_INNER_DTEL_IPV6_ACL, "hdr.inner_ipv6.hop_limit");
  F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_TCP_FLAGS =
      key_id_from_name(T_INGRESS_INNER_DTEL_IPV6_ACL, "hdr.inner_tcp.flags");
  F_INGRESS_INNER_DTEL_IPV6_ACL_IG_MD_PORT_LAG_LABEL =
      key_id_from_name(T_INGRESS_INNER_DTEL_IPV6_ACL, "ig_md.port_lag_label");
  F_INGRESS_INNER_DTEL_IPV6_ACL_LOCAL_MD_TUNNEL_VNI =
      key_id_from_name(T_INGRESS_INNER_DTEL_IPV6_ACL, "local_md.tunnel.vni");
  A_INGRESS_INNER_DTEL_IPV6_ACL_SET_DTEL_REPORT_TYPE =
      action_id_from_name(T_INGRESS_INNER_DTEL_IPV6_ACL,
                          "ingress_inner_ipv6_dtel_acl.set_dtel_report_type");
  P_INGRESS_INNER_DTEL_IPV6_ACL_DTEL_REPORT_TYPE =
      data_id_from_name(T_INGRESS_INNER_DTEL_IPV6_ACL,
                        A_INGRESS_INNER_DTEL_IPV6_ACL_SET_DTEL_REPORT_TYPE,
                        "type");
  D_INGRESS_INNER_DTEL_IPV6_ACL_COUNTER_SPEC_BYTES =
      data_id_from_name(T_INGRESS_INNER_DTEL_IPV6_ACL,
                        A_INGRESS_INNER_DTEL_IPV6_ACL_SET_DTEL_REPORT_TYPE,
                        "$COUNTER_SPEC_BYTES");
  D_INGRESS_INNER_DTEL_IPV6_ACL_COUNTER_SPEC_PKTS =
      data_id_from_name(T_INGRESS_INNER_DTEL_IPV6_ACL,
                        A_INGRESS_INNER_DTEL_IPV6_ACL_SET_DTEL_REPORT_TYPE,
                        "$COUNTER_SPEC_PKTS");

  T_INGRESS_IP_MIRROR_ACL = table_id_from_name("ingress_ip_mirror_acl.acl");
  F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_ETYPE_LABEL =
      key_id_from_name(T_INGRESS_IP_MIRROR_ACL, "local_md.etype_label");
  F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_MIRROR_MAC_LABEL =
      key_id_from_name(T_INGRESS_IP_MIRROR_ACL, "local_md.mirror_mac_label");
  F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_MAC_TYPE =
      key_id_from_name(T_INGRESS_IP_MIRROR_ACL, "local_md.lkp.mac_type");
  F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_SRC_ADDR =
      key_id_from_name(T_INGRESS_IP_MIRROR_ACL, "local_md.lkp.ip_src_addr");
  F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_DST_ADDR =
      key_id_from_name(T_INGRESS_IP_MIRROR_ACL, "local_md.lkp.ip_dst_addr");
  F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_PROTO =
      key_id_from_name(T_INGRESS_IP_MIRROR_ACL, "local_md.lkp.ip_proto");
  F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_TOS =
      key_id_from_name(T_INGRESS_IP_MIRROR_ACL, "local_md.lkp.ip_tos");
  F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_L4_SRC_PORT =
      key_id_from_name(T_INGRESS_IP_MIRROR_ACL, "local_md.lkp.l4_src_port");
  F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_L4_DST_PORT =
      key_id_from_name(T_INGRESS_IP_MIRROR_ACL, "local_md.lkp.l4_dst_port");
  F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_TTL =
      key_id_from_name(T_INGRESS_IP_MIRROR_ACL, "local_md.lkp.ip_ttl");
  F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_FRAG =
      key_id_from_name(T_INGRESS_IP_MIRROR_ACL, "local_md.lkp.ip_frag");
  F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_TCP_FLAGS =
      key_id_from_name(T_INGRESS_IP_MIRROR_ACL, "local_md.lkp.tcp_flags");
  F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_PCP =
      key_id_from_name(T_INGRESS_IP_MIRROR_ACL, "local_md.lkp.pcp");
  F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_DEI =
      key_id_from_name(T_INGRESS_IP_MIRROR_ACL, "local_md.lkp.dei");
  F_INGRESS_IP_MIRROR_ACL_HDR_VLAN_TAG0_VALID =
      key_id_from_name(T_INGRESS_IP_MIRROR_ACL, "hdr.vlan_tag$0.$valid");
  F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL = key_id_from_name(
      T_INGRESS_IP_MIRROR_ACL, "local_md.ingress_port_lag_label");
  F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_BD_LABEL =
      key_id_from_name(T_INGRESS_IP_MIRROR_ACL, "local_md.bd_label");
  F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_BD =
      key_id_from_name(T_INGRESS_IP_MIRROR_ACL, "local_md.bd");
  F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_L4_SRC_PORT_LABEL =
      key_id_from_name(T_INGRESS_IP_MIRROR_ACL, "local_md.l4_src_port_label");
  F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_L4_DST_PORT_LABEL =
      key_id_from_name(T_INGRESS_IP_MIRROR_ACL, "local_md.l4_dst_port_label");
  F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_IN_PORTS_GROUP_LABEL = key_id_from_name(
      T_INGRESS_IP_MIRROR_ACL, "local_md.in_ports_group_label_mirror");
  F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_FLAGS_RMAC_HIT =
      key_id_from_name(T_INGRESS_IP_MIRROR_ACL, "local_md.flags.rmac_hit");
  F_INGRESS_IP_MIRROR_ACL_MATCH_PRIORITY =
      key_id_from_name(T_INGRESS_IP_MIRROR_ACL, "$MATCH_PRIORITY");
  A_INGRESS_IP_MIRROR_ACL_NO_ACTION = action_id_from_name(
      T_INGRESS_IP_MIRROR_ACL, "ingress_ip_mirror_acl.no_action");
  P_INGRESS_IP_MIRROR_ACL_NO_ACTION_METER_INDEX =
      data_id_from_name(T_INGRESS_IP_MIRROR_ACL,
                        A_INGRESS_IP_MIRROR_ACL_NO_ACTION,
                        "meter_index");
  A_INGRESS_IP_MIRROR_ACL_DROP = action_id_from_name(
      T_INGRESS_IP_MIRROR_ACL, "ingress_ip_mirror_acl.drop");
  A_INGRESS_IP_MIRROR_ACL_PERMIT = action_id_from_name(
      T_INGRESS_IP_MIRROR_ACL, "ingress_ip_mirror_acl.permit");
  A_INGRESS_IP_MIRROR_ACL_REDIRECT_NEXTHOP = action_id_from_name(
      T_INGRESS_IP_MIRROR_ACL, "ingress_ip_mirror_acl.redirect_nexthop");
  P_INGRESS_IP_MIRROR_ACL_REDIRECT_NEXTHOP_INDEX =
      data_id_from_name(T_INGRESS_IP_MIRROR_ACL,
                        A_INGRESS_IP_MIRROR_ACL_REDIRECT_NEXTHOP,
                        "nexthop_index");
  A_INGRESS_IP_MIRROR_ACL_MIRROR = action_id_from_name(
      T_INGRESS_IP_MIRROR_ACL, "ingress_ip_mirror_acl.mirror_in");
  P_INGRESS_IP_MIRROR_ACL_MIRROR_METER_INDEX = data_id_from_name(
      T_INGRESS_IP_MIRROR_ACL, A_INGRESS_IP_MIRROR_ACL_MIRROR, "meter_index");
  P_INGRESS_IP_MIRROR_ACL_MIRROR_SESSION_ID = data_id_from_name(
      T_INGRESS_IP_MIRROR_ACL, A_INGRESS_IP_MIRROR_ACL_MIRROR, "session_id");
  A_INGRESS_IP_MIRROR_ACL_MIRROR_OUT = action_id_from_name(
      T_INGRESS_IP_MIRROR_ACL, "ingress_ip_mirror_acl.mirror_out");
  P_INGRESS_IP_MIRROR_ACL_MIRROR_OUT_METER_INDEX =
      data_id_from_name(T_INGRESS_IP_MIRROR_ACL,
                        A_INGRESS_IP_MIRROR_ACL_MIRROR_OUT,
                        "meter_index");
  P_INGRESS_IP_MIRROR_ACL_MIRROR_OUT_SESSION_ID =
      data_id_from_name(T_INGRESS_IP_MIRROR_ACL,
                        A_INGRESS_IP_MIRROR_ACL_MIRROR_OUT,
                        "session_id");
  A_INGRESS_IP_MIRROR_ACL_SET_TC = action_id_from_name(
      T_INGRESS_IP_MIRROR_ACL, "ingress_ip_mirror_acl.set_tc");
  P_INGRESS_IP_MIRROR_ACL_SET_TC_TC = data_id_from_name(
      T_INGRESS_IP_MIRROR_ACL, A_INGRESS_IP_MIRROR_ACL_SET_TC, "tc");
  A_INGRESS_IP_MIRROR_ACL_SET_COLOR = action_id_from_name(
      T_INGRESS_IP_MIRROR_ACL, "ingress_ip_mirror_acl.set_color");
  P_INGRESS_IP_MIRROR_ACL_SET_COLOR_COLOR = data_id_from_name(
      T_INGRESS_IP_MIRROR_ACL, A_INGRESS_IP_MIRROR_ACL_SET_COLOR, "color");
  A_INGRESS_IP_MIRROR_ACL_TRAP = action_id_from_name(
      T_INGRESS_IP_MIRROR_ACL, "ingress_ip_mirror_acl.trap");
  P_INGRESS_IP_MIRROR_ACL_TRAP_TRAP_ID = data_id_from_name(
      T_INGRESS_IP_MIRROR_ACL, A_INGRESS_IP_MIRROR_ACL_TRAP, "trap_id");
  P_INGRESS_IP_MIRROR_ACL_TRAP_METER_INDEX = data_id_from_name(
      T_INGRESS_IP_MIRROR_ACL, A_INGRESS_IP_MIRROR_ACL_TRAP, "meter_index");
  A_INGRESS_IP_MIRROR_ACL_COPY = action_id_from_name(
      T_INGRESS_IP_MIRROR_ACL, "ingress_ip_mirror_acl.copy_to_cpu");
  P_INGRESS_IP_MIRROR_ACL_COPY_TRAP_ID = data_id_from_name(
      T_INGRESS_IP_MIRROR_ACL, A_INGRESS_IP_MIRROR_ACL_COPY, "trap_id");
  P_INGRESS_IP_MIRROR_ACL_COPY_METER_INDEX = data_id_from_name(
      T_INGRESS_IP_MIRROR_ACL, A_INGRESS_IP_MIRROR_ACL_COPY, "meter_index");
  A_INGRESS_IP_MIRROR_ACL_SET_DTEL_REPORT_TYPE = action_id_from_name(
      T_INGRESS_IP_MIRROR_ACL, "ingress_ip_mirror_acl.set_dtel_report_type");
  P_INGRESS_IP_MIRROR_ACL_DTEL_REPORT_TYPE =
      data_id_from_name(T_INGRESS_IP_MIRROR_ACL,
                        A_INGRESS_IP_MIRROR_ACL_SET_DTEL_REPORT_TYPE,
                        "type");
  D_INGRESS_IP_MIRROR_ACL_COUNTER_SPEC_BYTES =
      data_id_from_name(T_INGRESS_IP_MIRROR_ACL,
                        A_INGRESS_IP_MIRROR_ACL_NO_ACTION,
                        "$COUNTER_SPEC_BYTES");
  D_INGRESS_IP_MIRROR_ACL_COUNTER_SPEC_PKTS =
      data_id_from_name(T_INGRESS_IP_MIRROR_ACL,
                        A_INGRESS_IP_MIRROR_ACL_NO_ACTION,
                        "$COUNTER_SPEC_PKTS");
  F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL_23_16 =
      key_id_from_name(T_INGRESS_IP_MIRROR_ACL,
                       "local_md.ingress_port_lag_label[23:16]");
  F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_INGRESS_PORT_LAG_INDEX = key_id_from_name(
      T_INGRESS_IP_MIRROR_ACL, "local_md.ingress_port_lag_index");

  T_INGRESS_IP_MIRROR_ACL_METER_ACTION =
      table_id_from_name("ingress_ip_mirror_acl.meter_action");
  F_INGRESS_IP_MIRROR_ACL_METER_ACTION_COLOR = key_id_from_name(
      T_INGRESS_IP_MIRROR_ACL_METER_ACTION, "local_md.mirror.meter_color");
  F_INGRESS_IP_MIRROR_ACL_METER_ACTION_INDEX = key_id_from_name(
      T_INGRESS_IP_MIRROR_ACL_METER_ACTION, "local_md.mirror.meter_index");
  A_INGRESS_IP_MIRROR_ACL_METER_ACTION_COUNT = action_id_from_name(
      T_INGRESS_IP_MIRROR_ACL_METER_ACTION, "ingress_ip_mirror_acl.count");
  A_INGRESS_IP_MIRROR_ACL_METER_ACTION_DROP_AND_COUNT =
      action_id_from_name(T_INGRESS_IP_MIRROR_ACL_METER_ACTION,
                          "ingress_ip_mirror_acl.drop_and_count");
  D_INGRESS_IP_MIRROR_ACL_METER_ACTION_COUNTER_SPEC_BYTES =
      data_id_from_name(T_INGRESS_IP_MIRROR_ACL_METER_ACTION,
                        A_INGRESS_IP_MIRROR_ACL_METER_ACTION_COUNT,
                        "$COUNTER_SPEC_BYTES");
  D_INGRESS_IP_MIRROR_ACL_METER_SPEC_CIR_KBPS = data_id_from_name_noaction(
      T_INGRESS_IP_MIRROR_ACL, "$METER_SPEC_CIR_KBPS");
  D_INGRESS_IP_MIRROR_ACL_METER_SPEC_PIR_KBPS = data_id_from_name_noaction(
      T_INGRESS_IP_MIRROR_ACL, "$METER_SPEC_PIR_KBPS");
  D_INGRESS_IP_MIRROR_ACL_METER_SPEC_CBS_KBITS = data_id_from_name_noaction(
      T_INGRESS_IP_MIRROR_ACL, "$METER_SPEC_CBS_KBITS");
  D_INGRESS_IP_MIRROR_ACL_METER_SPEC_PBS_KBITS = data_id_from_name_noaction(
      T_INGRESS_IP_MIRROR_ACL, "$METER_SPEC_PBS_KBITS");

  T_INGRESS_IP_QOS_ACL = table_id_from_name("ingress_ip_qos_acl.acl");
  F_INGRESS_IP_QOS_ACL_LOCAL_MD_ETYPE_LABEL =
      key_id_from_name(T_INGRESS_IP_QOS_ACL, "local_md.etype_label");
  F_INGRESS_IP_QOS_ACL_LOCAL_MD_QOS_MAC_LABEL =
      key_id_from_name(T_INGRESS_IP_QOS_ACL, "local_md.qos_mac_label");
  F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_MAC_TYPE =
      key_id_from_name(T_INGRESS_IP_QOS_ACL, "local_md.lkp.mac_type");
  F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_IP_SRC_ADDR =
      key_id_from_name(T_INGRESS_IP_QOS_ACL, "local_md.lkp.ip_src_addr");
  F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_IP_DST_ADDR =
      key_id_from_name(T_INGRESS_IP_QOS_ACL, "local_md.lkp.ip_dst_addr");
  F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_IP_PROTO =
      key_id_from_name(T_INGRESS_IP_QOS_ACL, "local_md.lkp.ip_proto");
  F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_IP_TOS =
      key_id_from_name(T_INGRESS_IP_QOS_ACL, "local_md.lkp.ip_tos");
  F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_L4_SRC_PORT =
      key_id_from_name(T_INGRESS_IP_QOS_ACL, "local_md.lkp.l4_src_port");
  F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_L4_DST_PORT =
      key_id_from_name(T_INGRESS_IP_QOS_ACL, "local_md.lkp.l4_dst_port");
  F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_TCP_FLAGS =
      key_id_from_name(T_INGRESS_IP_QOS_ACL, "local_md.lkp.tcp_flags");
  F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_PCP =
      key_id_from_name(T_INGRESS_IP_QOS_ACL, "local_md.lkp.pcp");
  F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_DEI =
      key_id_from_name(T_INGRESS_IP_QOS_ACL, "local_md.lkp.dei");
  F_INGRESS_IP_QOS_ACL_HDR_VLAN_TAG0_VALID =
      key_id_from_name(T_INGRESS_IP_QOS_ACL, "hdr.vlan_tag$0.$valid");
  F_INGRESS_IP_QOS_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL =
      key_id_from_name(T_INGRESS_IP_QOS_ACL, "local_md.ingress_port_lag_label");
  F_INGRESS_IP_QOS_ACL_LOCAL_MD_BD =
      key_id_from_name(T_INGRESS_IP_QOS_ACL, "local_md.bd");
  F_INGRESS_IP_QOS_ACL_LOCAL_MD_L4_SRC_PORT_LABEL =
      key_id_from_name(T_INGRESS_IP_QOS_ACL, "local_md.l4_src_port_label");
  F_INGRESS_IP_QOS_ACL_LOCAL_MD_L4_DST_PORT_LABEL =
      key_id_from_name(T_INGRESS_IP_QOS_ACL, "local_md.l4_dst_port_label");
  F_INGRESS_IP_QOS_ACL_LOCAL_MD_INGRESS_PORT_LAG_INDEX =
      key_id_from_name(T_INGRESS_IP_QOS_ACL, "local_md.ingress_port_lag_index");
  F_INGRESS_IP_QOS_ACL_MATCH_PRIORITY =
      key_id_from_name(T_INGRESS_IP_QOS_ACL, "$MATCH_PRIORITY");
  F_INGRESS_IP_QOS_ACL_LOCAL_MD_FLAGS_RMAC_HIT =
      key_id_from_name(T_INGRESS_IP_QOS_ACL, "local_md.flags.rmac_hit");
  A_INGRESS_IP_QOS_ACL_SET_TC =
      action_id_from_name(T_INGRESS_IP_QOS_ACL, "ingress_ip_qos_acl.set_tc");
  P_INGRESS_IP_QOS_ACL_SET_TC_TC = data_id_from_name(
      T_INGRESS_IP_QOS_ACL, A_INGRESS_IP_QOS_ACL_SET_TC, "tc");
  P_INGRESS_IP_QOS_ACL_SET_TC_METER_INDEX = data_id_from_name(
      T_INGRESS_IP_QOS_ACL, A_INGRESS_IP_QOS_ACL_SET_TC, "meter_index");
  A_INGRESS_IP_QOS_ACL_SET_COLOR =
      action_id_from_name(T_INGRESS_IP_QOS_ACL, "ingress_ip_qos_acl.set_color");
  P_INGRESS_IP_QOS_ACL_SET_COLOR_COLOR = data_id_from_name(
      T_INGRESS_IP_QOS_ACL, A_INGRESS_IP_QOS_ACL_SET_COLOR, "color");
  P_INGRESS_IP_QOS_ACL_SET_COLOR_METER_INDEX = data_id_from_name(
      T_INGRESS_IP_QOS_ACL, A_INGRESS_IP_QOS_ACL_SET_COLOR, "meter_index");
  A_INGRESS_IP_QOS_ACL_SET_METER =
      action_id_from_name(T_INGRESS_IP_QOS_ACL, "ingress_ip_qos_acl.set_meter");
  P_INGRESS_IP_QOS_ACL_SET_METER_INDEX = data_id_from_name(
      T_INGRESS_IP_QOS_ACL, A_INGRESS_IP_QOS_ACL_SET_METER, "meter_index");
  A_INGRESS_IP_QOS_ACL_SET_QOS_PARAMS = action_id_from_name(
      T_INGRESS_IP_QOS_ACL, "ingress_ip_qos_acl.set_qos_params");
  P_INGRESS_IP_QOS_ACL_SET_QOS_PARAMS_TC = data_id_from_name(
      T_INGRESS_IP_QOS_ACL, A_INGRESS_IP_QOS_ACL_SET_QOS_PARAMS, "tc");
  P_INGRESS_IP_QOS_ACL_SET_QOS_PARAMS_COLOR = data_id_from_name(
      T_INGRESS_IP_QOS_ACL, A_INGRESS_IP_QOS_ACL_SET_QOS_PARAMS, "color");
  P_INGRESS_IP_QOS_ACL_SET_QOS_PARAMS_METER_INDEX = data_id_from_name(
      T_INGRESS_IP_QOS_ACL, A_INGRESS_IP_QOS_ACL_SET_QOS_PARAMS, "meter_index");
  A_INGRESS_IP_QOS_ACL_NO_ACTION =
      action_id_from_name(T_INGRESS_IP_QOS_ACL, "ingress_ip_qos_acl.no_action");
  P_INGRESS_IP_QOS_ACL_NO_ACTION_METER_INDEX = data_id_from_name(
      T_INGRESS_IP_QOS_ACL, A_INGRESS_IP_QOS_ACL_NO_ACTION, "meter_index");
  D_INGRESS_IP_QOS_ACL_COUNTER_SPEC_BYTES =
      data_id_from_name(T_INGRESS_IP_QOS_ACL,
                        A_INGRESS_IP_QOS_ACL_NO_ACTION,
                        "$COUNTER_SPEC_BYTES");
  D_INGRESS_IP_QOS_ACL_COUNTER_SPEC_PKTS =
      data_id_from_name(T_INGRESS_IP_QOS_ACL,
                        A_INGRESS_IP_QOS_ACL_NO_ACTION,
                        "$COUNTER_SPEC_PKTS");
  D_INGRESS_IP_QOS_ACL_METER_SPEC_CIR_KBPS =
      data_id_from_name_noaction(T_INGRESS_IP_QOS_ACL, "$METER_SPEC_CIR_KBPS");
  D_INGRESS_IP_QOS_ACL_METER_SPEC_PIR_KBPS =
      data_id_from_name_noaction(T_INGRESS_IP_QOS_ACL, "$METER_SPEC_PIR_KBPS");
  D_INGRESS_IP_QOS_ACL_METER_SPEC_CBS_KBITS =
      data_id_from_name_noaction(T_INGRESS_IP_QOS_ACL, "$METER_SPEC_CBS_KBITS");
  D_INGRESS_IP_QOS_ACL_METER_SPEC_PBS_KBITS =
      data_id_from_name_noaction(T_INGRESS_IP_QOS_ACL, "$METER_SPEC_PBS_KBITS");

  T_INGRESS_IP_QOS_ACL_METER_ACTION =
      table_id_from_name("ingress_ip_qos_acl.meter_action");
  F_INGRESS_IP_QOS_ACL_METER_ACTION_COLOR = key_id_from_name(
      T_INGRESS_IP_QOS_ACL_METER_ACTION, "local_md.qos.acl_meter_color");
  F_INGRESS_IP_QOS_ACL_METER_ACTION_INDEX = key_id_from_name(
      T_INGRESS_IP_QOS_ACL_METER_ACTION, "local_md.qos.acl_meter_index");
  A_INGRESS_IP_QOS_ACL_METER_ACTION_COUNT = action_id_from_name(
      T_INGRESS_IP_QOS_ACL_METER_ACTION, "ingress_ip_qos_acl.count");
  A_INGRESS_IP_QOS_ACL_METER_ACTION_DROP_AND_COUNT = action_id_from_name(
      T_INGRESS_IP_QOS_ACL_METER_ACTION, "ingress_ip_qos_acl.drop_and_count");
  D_INGRESS_IP_QOS_ACL_METER_ACTION_COUNTER_SPEC_BYTES =
      data_id_from_name(T_INGRESS_IP_QOS_ACL_METER_ACTION,
                        A_INGRESS_IP_QOS_ACL_METER_ACTION_COUNT,
                        "$COUNTER_SPEC_BYTES");

  T_INGRESS_IP_DTEL_ACL = table_id_from_name("ingress_ip_dtel_acl.acl");
  F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_MAC_TYPE =
      key_id_from_name(T_INGRESS_IP_DTEL_ACL, "local_md.lkp.mac_type");
  F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_SRC_ADDR =
      key_id_from_name(T_INGRESS_IP_DTEL_ACL, "local_md.lkp.ip_src_addr");
  F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD3 = key_id_from_name(
      T_INGRESS_IP_DTEL_ACL, "local_md.lkp.ip_src_addr[127:96]");
  F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD2 = key_id_from_name(
      T_INGRESS_IP_DTEL_ACL, "local_md.lkp.ip_src_addr[95:64]");
  F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_DST_ADDR =
      key_id_from_name(T_INGRESS_IP_DTEL_ACL, "local_md.lkp.ip_dst_addr");
  F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD3 = key_id_from_name(
      T_INGRESS_IP_DTEL_ACL, "local_md.lkp.ip_dst_addr[127:96]");
  F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD2 = key_id_from_name(
      T_INGRESS_IP_DTEL_ACL, "local_md.lkp.ip_dst_addr[95:64]");
  F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_PROTO =
      key_id_from_name(T_INGRESS_IP_DTEL_ACL, "local_md.lkp.ip_proto");
  F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_TOS =
      key_id_from_name(T_INGRESS_IP_DTEL_ACL, "local_md.lkp.ip_tos");
  F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_L4_SRC_PORT =
      key_id_from_name(T_INGRESS_IP_DTEL_ACL, "local_md.lkp.l4_src_port");
  F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_L4_DST_PORT =
      key_id_from_name(T_INGRESS_IP_DTEL_ACL, "local_md.lkp.l4_dst_port");
  F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_TTL =
      key_id_from_name(T_INGRESS_IP_DTEL_ACL, "local_md.lkp.ip_ttl");
  F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_FRAG =
      key_id_from_name(T_INGRESS_IP_DTEL_ACL, "local_md.lkp.ip_frag");
  F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_TCP_FLAGS =
      key_id_from_name(T_INGRESS_IP_DTEL_ACL, "local_md.lkp.tcp_flags");
  F_INGRESS_IP_DTEL_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL = key_id_from_name(
      T_INGRESS_IP_DTEL_ACL, "local_md.ingress_port_lag_label");
  F_INGRESS_IP_DTEL_ACL_LOCAL_MD_BD_LABEL =
      key_id_from_name(T_INGRESS_IP_DTEL_ACL, "local_md.bd_label");
  F_INGRESS_IP_DTEL_ACL_LOCAL_MD_L4_SRC_PORT_LABEL =
      key_id_from_name(T_INGRESS_IP_DTEL_ACL, "local_md.l4_src_port_label");
  F_INGRESS_IP_DTEL_ACL_LOCAL_MD_L4_DST_PORT_LABEL =
      key_id_from_name(T_INGRESS_IP_DTEL_ACL, "local_md.l4_dst_port_label");
  F_INGRESS_IP_DTEL_ACL_MATCH_PRIORITY =
      key_id_from_name(T_INGRESS_IP_DTEL_ACL, "$MATCH_PRIORITY");
  A_INGRESS_IP_DTEL_ACL_NO_ACTION = action_id_from_name(
      T_INGRESS_IP_DTEL_ACL, "ingress_ip_dtel_acl.no_action");
  A_INGRESS_IP_DTEL_ACL_DROP =
      action_id_from_name(T_INGRESS_IP_DTEL_ACL, "ingress_ip_dtel_acl.drop");
  A_INGRESS_IP_DTEL_ACL_PERMIT =
      action_id_from_name(T_INGRESS_IP_DTEL_ACL, "ingress_ip_dtel_acl.permit");
  A_INGRESS_IP_DTEL_ACL_REDIRECT_NEXTHOP = action_id_from_name(
      T_INGRESS_IP_DTEL_ACL, "ingress_ip_dtel_acl.redirect_nexthop");
  P_INGRESS_IP_DTEL_ACL_REDIRECT_NEXTHOP_INDEX =
      data_id_from_name(T_INGRESS_IP_DTEL_ACL,
                        A_INGRESS_IP_DTEL_ACL_REDIRECT_NEXTHOP,
                        "nexthop_index");
  A_INGRESS_IP_DTEL_ACL_MIRROR = action_id_from_name(
      T_INGRESS_IP_DTEL_ACL, "ingress_ip_dtel_acl.mirror_in");
  P_INGRESS_IP_DTEL_ACL_MIRROR_METER_INDEX = data_id_from_name(
      T_INGRESS_IP_DTEL_ACL, A_INGRESS_IP_DTEL_ACL_MIRROR, "meter_index");
  P_INGRESS_IP_DTEL_ACL_MIRROR_SESSION_ID = data_id_from_name(
      T_INGRESS_IP_DTEL_ACL, A_INGRESS_IP_DTEL_ACL_MIRROR, "session_id");
  A_INGRESS_IP_DTEL_ACL_SET_TC =
      action_id_from_name(T_INGRESS_IP_DTEL_ACL, "ingress_ip_dtel_acl.set_tc");
  P_INGRESS_IP_DTEL_ACL_SET_TC_TC = data_id_from_name(
      T_INGRESS_IP_DTEL_ACL, A_INGRESS_IP_DTEL_ACL_SET_TC, "tc");
  A_INGRESS_IP_DTEL_ACL_SET_COLOR = action_id_from_name(
      T_INGRESS_IP_DTEL_ACL, "ingress_ip_dtel_acl.set_color");
  P_INGRESS_IP_DTEL_ACL_SET_COLOR_COLOR = data_id_from_name(
      T_INGRESS_IP_DTEL_ACL, A_INGRESS_IP_DTEL_ACL_SET_COLOR, "color");
  A_INGRESS_IP_DTEL_ACL_SET_DTEL_REPORT_TYPE = action_id_from_name(
      T_INGRESS_IP_DTEL_ACL, "ingress_ip_dtel_acl.set_dtel_report_type");
  P_INGRESS_IP_DTEL_ACL_DTEL_REPORT_TYPE =
      data_id_from_name(T_INGRESS_IP_DTEL_ACL,
                        A_INGRESS_IP_DTEL_ACL_SET_DTEL_REPORT_TYPE,
                        "type");
  A_INGRESS_IP_DTEL_ACL_IFA_CLONE_SAMPLE = action_id_from_name(
      T_INGRESS_IP_DTEL_ACL, "ingress_ip_dtel_acl.ifa_clone_sample");
  P_INGRESS_IP_DTEL_ACL_IFA_CLONE_SESSION_ID =
      data_id_from_name(T_INGRESS_IP_DTEL_ACL,
                        A_INGRESS_IP_DTEL_ACL_IFA_CLONE_SAMPLE,
                        "ifa_sample_session");
  A_INGRESS_IP_DTEL_ACL_IFA_CLONE_AND_SET_REPORT_TYPE =
      action_id_from_name(T_INGRESS_IP_DTEL_ACL,
                          "ingress_ip_dtel_acl.ifa_clone_sample_"
                          "and_set_dtel_report_type");
  P_INGRESS_IP_DTEL_ACL_IFA_CLONE_SESSION_ID_WITH_TYPE =
      data_id_from_name(T_INGRESS_IP_DTEL_ACL,
                        A_INGRESS_IP_DTEL_ACL_IFA_CLONE_AND_SET_REPORT_TYPE,
                        "ifa_sample_session");
  P_INGRESS_IP_DTEL_ACL_DTEL_REPORT_TYPE_WITH_CLONE =
      data_id_from_name(T_INGRESS_IP_DTEL_ACL,
                        A_INGRESS_IP_DTEL_ACL_IFA_CLONE_AND_SET_REPORT_TYPE,
                        "type");
  D_INGRESS_IP_DTEL_ACL_COUNTER_SPEC_BYTES =
      data_id_from_name(T_INGRESS_IP_DTEL_ACL,
                        A_INGRESS_IP_DTEL_ACL_SET_DTEL_REPORT_TYPE,
                        "$COUNTER_SPEC_BYTES");
  D_INGRESS_IP_DTEL_ACL_COUNTER_SPEC_PKTS =
      data_id_from_name(T_INGRESS_IP_DTEL_ACL,
                        A_INGRESS_IP_DTEL_ACL_SET_DTEL_REPORT_TYPE,
                        "$COUNTER_SPEC_PKTS");

  T_INGRESS_L4_SRC_PORT = table_id_from_name("ingress_l4_src_port");
  F_INGRESS_L4_SRC_PORT_LKP_L4_SRC_PORT =
      key_id_from_name(T_INGRESS_L4_SRC_PORT, "local_md.lkp.l4_src_port");
  F_INGRESS_L4_SRC_PORT_PRIORITY =
      key_id_from_name(T_INGRESS_L4_SRC_PORT, "$MATCH_PRIORITY");
  A_INGRESS_SET_SRC_PORT_LABEL =
      action_id_from_name(T_INGRESS_L4_SRC_PORT, "set_ingress_src_port_label");
  P_INGRESS_SET_SRC_PORT_LABEL_LABEL = data_id_from_name(
      T_INGRESS_L4_SRC_PORT, A_INGRESS_SET_SRC_PORT_LABEL, "label");

  // acl2 lou
  T_INGRESS_QOS_ACL_L4_SRC_PORT = table_id_from_name("qos_acl_lou.l4_src_port");
  F_INGRESS_QOS_ACL_L4_SRC_PORT_LKP_L4_SRC_PORT = key_id_from_name(
      T_INGRESS_QOS_ACL_L4_SRC_PORT, "local_md.lkp.l4_src_port");
  F_INGRESS_QOS_ACL_L4_SRC_PORT_PRIORITY =
      key_id_from_name(T_INGRESS_QOS_ACL_L4_SRC_PORT, "$MATCH_PRIORITY");
  A_INGRESS_QOS_ACL_SET_SRC_PORT_LABEL = action_id_from_name(
      T_INGRESS_QOS_ACL_L4_SRC_PORT, "set_ingress_src_port_label");
  P_INGRESS_QOS_ACL_SET_SRC_PORT_LABEL_LABEL =
      data_id_from_name(T_INGRESS_QOS_ACL_L4_SRC_PORT,
                        A_INGRESS_QOS_ACL_SET_SRC_PORT_LABEL,
                        "label");

  T_INGRESS_IP_ACL_L4_SRC_PORT = table_id_from_name("ip_acl_lou.l4_src_port");
  F_INGRESS_IP_ACL_L4_SRC_PORT_LKP_L4_SRC_PORT = key_id_from_name(
      T_INGRESS_IP_ACL_L4_SRC_PORT, "local_md.lkp.l4_src_port");
  F_INGRESS_IP_ACL_L4_SRC_PORT_PRIORITY =
      key_id_from_name(T_INGRESS_IP_ACL_L4_SRC_PORT, "$MATCH_PRIORITY");
  A_INGRESS_IP_ACL_SET_SRC_PORT_LABEL = action_id_from_name(
      T_INGRESS_IP_ACL_L4_SRC_PORT, "set_ingress_src_port_label");
  P_INGRESS_IP_ACL_SET_SRC_PORT_LABEL_LABEL =
      data_id_from_name(T_INGRESS_IP_ACL_L4_SRC_PORT,
                        A_INGRESS_IP_ACL_SET_SRC_PORT_LABEL,
                        "label");

  T_INGRESS_TOS_MIRROR_ACL = table_id_from_name("ingress_tos_mirror_acl.acl");
  F_INGRESS_TOS_MIRROR_ACL_LOCAL_MD_LKP_IP_TOS =
      key_id_from_name(T_INGRESS_TOS_MIRROR_ACL, "local_md.lkp.ip_tos");
  F_INGRESS_TOS_MIRROR_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL = key_id_from_name(
      T_INGRESS_TOS_MIRROR_ACL, "local_md.ingress_port_lag_label");
  F_INGRESS_TOS_MIRROR_ACL_MATCH_PRIORITY =
      key_id_from_name(T_INGRESS_TOS_MIRROR_ACL, "$MATCH_PRIORITY");
  A_INGRESS_TOS_MIRROR_ACL_MIRROR = action_id_from_name(
      T_INGRESS_TOS_MIRROR_ACL, "ingress_tos_mirror_acl.mirror_in");
  P_INGRESS_TOS_MIRROR_ACL_MIRROR_METER_INDEX = data_id_from_name(
      T_INGRESS_TOS_MIRROR_ACL, A_INGRESS_TOS_MIRROR_ACL_MIRROR, "meter_index");
  P_INGRESS_TOS_MIRROR_ACL_MIRROR_SESSION_ID = data_id_from_name(
      T_INGRESS_TOS_MIRROR_ACL, A_INGRESS_TOS_MIRROR_ACL_MIRROR, "session_id");
  A_INGRESS_TOS_MIRROR_ACL_MIRROR_OUT = action_id_from_name(
      T_INGRESS_TOS_MIRROR_ACL, "ingress_tos_mirror_acl.mirror_out");
  P_INGRESS_TOS_MIRROR_ACL_MIRROR_OUT_METER_INDEX =
      data_id_from_name(T_INGRESS_TOS_MIRROR_ACL,
                        A_INGRESS_TOS_MIRROR_ACL_MIRROR_OUT,
                        "meter_index");
  P_INGRESS_TOS_MIRROR_ACL_MIRROR_OUT_SESSION_ID =
      data_id_from_name(T_INGRESS_TOS_MIRROR_ACL,
                        A_INGRESS_TOS_MIRROR_ACL_MIRROR_OUT,
                        "session_id");
  D_INGRESS_TOS_MIRROR_ACL_COUNTER_SPEC_BYTES =
      data_id_from_name(T_INGRESS_TOS_MIRROR_ACL,
                        A_INGRESS_TOS_MIRROR_ACL_MIRROR,
                        "$COUNTER_SPEC_BYTES");
  D_INGRESS_TOS_MIRROR_ACL_COUNTER_SPEC_PKTS =
      data_id_from_name(T_INGRESS_TOS_MIRROR_ACL,
                        A_INGRESS_TOS_MIRROR_ACL_MIRROR,
                        "$COUNTER_SPEC_PKTS");

  T_INGRESS_L4_DST_PORT = table_id_from_name("ingress_l4_dst_port");
  F_INGRESS_L4_DST_PORT_LKP_L4_DST_PORT =
      key_id_from_name(T_INGRESS_L4_DST_PORT, "local_md.lkp.l4_dst_port");
  F_INGRESS_L4_DST_PORT_PRIORITY =
      key_id_from_name(T_INGRESS_L4_DST_PORT, "$MATCH_PRIORITY");
  A_INGRESS_SET_DST_PORT_LABEL =
      action_id_from_name(T_INGRESS_L4_DST_PORT, "set_ingress_dst_port_label");
  P_INGRESS_SET_DST_PORT_LABEL_LABEL = data_id_from_name(
      T_INGRESS_L4_DST_PORT, A_INGRESS_SET_DST_PORT_LABEL, "label");

  // acl2 lou
  T_INGRESS_QOS_ACL_L4_DST_PORT = table_id_from_name("qos_acl_lou.l4_dst_port");
  F_INGRESS_QOS_ACL_L4_DST_PORT_LKP_L4_DST_PORT = key_id_from_name(
      T_INGRESS_QOS_ACL_L4_DST_PORT, "local_md.lkp.l4_dst_port");
  F_INGRESS_QOS_ACL_L4_DST_PORT_PRIORITY =
      key_id_from_name(T_INGRESS_QOS_ACL_L4_DST_PORT, "$MATCH_PRIORITY");
  A_INGRESS_QOS_ACL_SET_DST_PORT_LABEL = action_id_from_name(
      T_INGRESS_QOS_ACL_L4_DST_PORT, "set_ingress_dst_port_label");
  P_INGRESS_QOS_ACL_SET_DST_PORT_LABEL_LABEL =
      data_id_from_name(T_INGRESS_QOS_ACL_L4_DST_PORT,
                        A_INGRESS_QOS_ACL_SET_DST_PORT_LABEL,
                        "label");

  T_INGRESS_IP_ACL_L4_DST_PORT = table_id_from_name("ip_acl_lou.l4_dst_port");
  F_INGRESS_IP_ACL_L4_DST_PORT_LKP_L4_DST_PORT = key_id_from_name(
      T_INGRESS_IP_ACL_L4_DST_PORT, "local_md.lkp.l4_dst_port");
  F_INGRESS_IP_ACL_L4_DST_PORT_PRIORITY =
      key_id_from_name(T_INGRESS_IP_ACL_L4_DST_PORT, "$MATCH_PRIORITY");
  A_INGRESS_IP_ACL_SET_DST_PORT_LABEL = action_id_from_name(
      T_INGRESS_IP_ACL_L4_DST_PORT, "set_ingress_dst_port_label");
  P_INGRESS_IP_ACL_SET_DST_PORT_LABEL_LABEL =
      data_id_from_name(T_INGRESS_IP_ACL_L4_DST_PORT,
                        A_INGRESS_IP_ACL_SET_DST_PORT_LABEL,
                        "label");

  T_INGRESS_DROP_STATS = table_id_from_name("ingress_drop_stats");
  F_INGRESS_DROP_STATS_PORT = key_id_from_name(T_INGRESS_DROP_STATS, "port");
  F_INGRESS_DROP_STATS_DROP_REASON =
      key_id_from_name(T_INGRESS_DROP_STATS, "drop_reason");
  A_INGRESS_DROP_STATS_COUNT =
      action_id_from_name(T_INGRESS_DROP_STATS, "ingress_drop_stats_count");
  D_INGRESS_DROP_STATS_PKTS = data_id_from_name(
      T_INGRESS_DROP_STATS, A_INGRESS_DROP_STATS_COUNT, "$COUNTER_SPEC_PKTS");

  T_INGRESS_SYSTEM_ACL = table_id_from_name("ingress_system_acl");
  F_SYSTEM_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.ingress_port_lag_label");
  F_SYSTEM_ACL_LOCAL_MD_BD =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.bd");
  F_SYSTEM_ACL_LOCAL_MD_BD_LABEL =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.bd_label");
  F_SYSTEM_ACL_LOCAL_MD_INGRESS_PORT_LAG_INDEX =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.ingress_port_lag_index");
  F_SYSTEM_ACL_LKP_PKT_TYPE =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.lkp.pkt_type");
  F_SYSTEM_ACL_LKP_MAC_TYPE =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.lkp.mac_type");
  F_SYSTEM_ACL_LKP_MAC_DST_ADDR =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.lkp.mac_dst_addr");
  F_SYSTEM_ACL_LKP_IP_DST_ADDR =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.lkp.ip_dst_addr");
  F_SYSTEM_ACL_LKP_IP_TYPE =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.lkp.ip_type");
  F_SYSTEM_ACL_LKP_IP_TTL =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.lkp.ip_ttl");
  F_SYSTEM_ACL_LKP_IP_PROTO =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.lkp.ip_proto");
  F_SYSTEM_ACL_LKP_L4_SRC_PORT =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.lkp.l4_src_port");
  F_SYSTEM_ACL_LKP_L4_DST_PORT =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.lkp.l4_dst_port");
  F_SYSTEM_ACL_LKP_ARP_OPCODE =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.lkp.arp_opcode");
  F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.flags.port_vlan_miss");
  F_SYSTEM_ACL_LOCAL_MD_FLAGS_ACL_DENY =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.flags.acl_deny");
  F_SYSTEM_ACL_LOCAL_MD_FLAGS_RACL_DENY =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.flags.racl_deny");
  F_SYSTEM_ACL_LOCAL_MD_FLAGS_ACL_COPY_CANCEL =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.flags.copy_cancel");
  F_SYSTEM_ACL_LOCAL_MD_FLAGS_RMAC_HIT =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.flags.rmac_hit");
  F_SYSTEM_ACL_LOCAL_MD_FLAGS_DMAC_MISS =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.flags.dmac_miss");
  F_SYSTEM_ACL_LOCAL_MD_FLAGS_MYIP =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.flags.myip");
  F_SYSTEM_ACL_LOCAL_MD_FLAGS_GLEAN =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.flags.glean");
  F_SYSTEM_ACL_LOCAL_MD_FLAGS_ROUTED =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.flags.routed");
  F_SYSTEM_ACL_LOCAL_MD_FLAGS_ARP_SUPPRESS = key_id_from_name(
      T_INGRESS_SYSTEM_ACL, "local_md.flags.vlan_arp_suppress");
  F_SYSTEM_ACL_LOCAL_MD_FLAGS_FIB_LPM_MISS =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.flags.fib_lpm_miss");
  F_SYSTEM_ACL_LOCAL_MD_FLAGS_FIB_DROP =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.flags.fib_drop");
  F_SYSTEM_ACL_LOCAL_MD_FLAGS_SAMPLE_PACKET =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.sflow.sample_packet");
  F_SYSTEM_ACL_LOCAL_MD_FLAGS_PFC_WD_DROP =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.flags.pfc_wd_drop");
  F_SYSTEM_ACL_LOCAL_MD_FLAGS_STORM_CONTROL_COLOR = key_id_from_name(
      T_INGRESS_SYSTEM_ACL, "local_md.qos.storm_control_color");
  F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_METER_DROP =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.flags.port_meter_drop");
  F_SYSTEM_ACL_LOCAL_MD_FLAGS_METER_PACKET_ACTION = key_id_from_name(
      T_INGRESS_SYSTEM_ACL, "local_md.flags.meter_packet_action");
  F_SYSTEM_ACL_LOCAL_MD_FLAGS_LINK_LOCAL =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.flags.link_local");
  F_SYSTEM_ACL_LOCAL_MD_CHECKS_SAME_BD =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.checks.same_bd");
  F_SYSTEM_ACL_LOCAL_MD_CHECKS_SAME_IF =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.checks.same_if");
  F_SYSTEM_ACL_LOCAL_MD_CHECKS_MRPF =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.checks.mrpf");
  F_SYSTEM_ACL_LOCAL_MD_STP_STATE =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.stp.state_");
  F_SYSTEM_ACL_LOCAL_MD_IPV4_MD_UNICAST_ENABLE =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.ipv4.unicast_enable");
  F_SYSTEM_ACL_LOCAL_MD_IPV4_MD_MULTICAST_ENABLE =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.ipv4.multicast_enable");
  F_SYSTEM_ACL_LOCAL_MD_IPV4_MD_MULTICAST_SNOOPING = key_id_from_name(
      T_INGRESS_SYSTEM_ACL, "local_md.ipv4.multicast_snooping");
  F_SYSTEM_ACL_LOCAL_MD_IPV6_MD_UNICAST_ENABLE =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.ipv6.unicast_enable");
  F_SYSTEM_ACL_LOCAL_MD_IPV6_MD_MULTICAST_ENABLE =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.ipv6.multicast_enable");
  F_SYSTEM_ACL_LOCAL_MD_IPV6_MD_MULTICAST_SNOOPING = key_id_from_name(
      T_INGRESS_SYSTEM_ACL, "local_md.ipv6.multicast_snooping");
  F_SYSTEM_ACL_LOCAL_MD_MPLS_ENABLE =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.mpls_enable");
  F_SYSTEM_ACL_LOCAL_MD_DROP_REASON =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.drop_reason");
  F_SYSTEM_ACL_LOCAL_MD_L2_DROP_REASON =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.l2_drop_reason");
  F_SYSTEM_ACL_LOCAL_MD_MPLS_TRAP =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.flags.mpls_trap");
  F_SYSTEM_ACL_LOCAL_MD_HOSTIF_TRAP_ID =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.hostif_trap_id");
  F_SYSTEM_ACL_LOCAL_MD_MPLS_ROUTER_ALERT = key_id_from_name(
      T_INGRESS_SYSTEM_ACL, "local_md.lkp.mpls_router_alert_label");
  F_SYSTEM_ACL_HDR_MPLS_0_TTL =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "hdr.mpls$0.ttl");
  F_SYSTEM_ACL_HDR_MPLS_0_VALID =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "hdr.mpls$0.$valid");
  F_SYSTEM_ACL_LOCAL_MD_SRV6_TRAP =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.flags.srv6_trap");
  F_SYSTEM_ACL_LOCAL_MD_BFD_TO_CPU =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.flags.bfd_to_cpu");
  F_SYSTEM_ACL_LOCAL_MD_VRF_TTL_VIOLATION = key_id_from_name(
      T_INGRESS_SYSTEM_ACL, "local_md.flags.vrf_ttl_violation");
  F_SYSTEM_ACL_LOCAL_MD_VRF_TTL_VIOLATION_VALID = key_id_from_name(
      T_INGRESS_SYSTEM_ACL, "local_md.flags.vrf_ttl_violation_valid");
  F_SYSTEM_ACL_LOCAL_MD_VRF_IP_OPTIONS_VIOLATION = key_id_from_name(
      T_INGRESS_SYSTEM_ACL, "local_md.flags.vrf_ip_options_violation");
  F_SYSTEM_ACL_LOCAL_MD_VRF_UNKNOWN_L3_MULTICAST_TRAP = key_id_from_name(
      T_INGRESS_SYSTEM_ACL, "local_md.flags.vrf_unknown_l3_multicast_trap");
  F_SYSTEM_ACL_HDR_IP_OPTIONS_VALID =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "hdr.ipv4_option.$valid");
  F_SYSTEM_ACL_PRIORITY =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "$MATCH_PRIORITY");
  A_SYSTEM_ACL_PERMIT =
      action_id_from_name(T_INGRESS_SYSTEM_ACL, "ingress_system_acl_permit");
  A_SYSTEM_ACL_DROP =
      action_id_from_name(T_INGRESS_SYSTEM_ACL, "ingress_system_acl_drop");
  P_SYSTEM_ACL_DROP_DROP_REASON =
      data_id_from_name(T_INGRESS_SYSTEM_ACL, A_SYSTEM_ACL_DROP, "drop_reason");
  P_SYSTEM_ACL_DROP_DISABLE_LEARNING = data_id_from_name(
      T_INGRESS_SYSTEM_ACL, A_SYSTEM_ACL_DROP, "disable_learning");
  A_SYSTEM_ACL_DENY =
      action_id_from_name(T_INGRESS_SYSTEM_ACL, "ingress_system_acl_deny");
  P_SYSTEM_ACL_DENY_DROP_REASON =
      data_id_from_name(T_INGRESS_SYSTEM_ACL, A_SYSTEM_ACL_DENY, "drop_reason");
  P_SYSTEM_ACL_DENY_DISABLE_LEARNING = data_id_from_name(
      T_INGRESS_SYSTEM_ACL, A_SYSTEM_ACL_DENY, "disable_learning");
  A_SYSTEM_ACL_COPY_TO_CPU_CANCEL = action_id_from_name(
      T_INGRESS_SYSTEM_ACL, "ingress_system_acl_copy_to_cpu_cancel");
  A_SYSTEM_ACL_COPY_TO_CPU = action_id_from_name(
      T_INGRESS_SYSTEM_ACL, "ingress_system_acl_copy_to_cpu");
  P_SYSTEM_ACL_COPY_TO_CPU_REASON_CODE = data_id_from_name(
      T_INGRESS_SYSTEM_ACL, A_SYSTEM_ACL_COPY_TO_CPU, "reason_code");
  P_SYSTEM_ACL_COPY_TO_CPU_QID =
      data_id_from_name(T_INGRESS_SYSTEM_ACL, A_SYSTEM_ACL_COPY_TO_CPU, "qid");
  P_SYSTEM_ACL_COPY_TO_CPU_METER_ID = data_id_from_name(
      T_INGRESS_SYSTEM_ACL, A_SYSTEM_ACL_COPY_TO_CPU, "meter_id");
  P_SYSTEM_ACL_COPY_TO_CPU_DISABLE_LEARNING = data_id_from_name(
      T_INGRESS_SYSTEM_ACL, A_SYSTEM_ACL_COPY_TO_CPU, "disable_learning");
  P_SYSTEM_ACL_COPY_TO_CPU_OVERWRITE_QID = data_id_from_name(
      T_INGRESS_SYSTEM_ACL, A_SYSTEM_ACL_COPY_TO_CPU, "overwrite_qid");
  A_SYSTEM_ACL_REDIRECT_TO_CPU = action_id_from_name(
      T_INGRESS_SYSTEM_ACL, "ingress_system_acl_redirect_to_cpu");
  P_SYSTEM_ACL_REDIRECT_TO_CPU_REASON_CODE = data_id_from_name(
      T_INGRESS_SYSTEM_ACL, A_SYSTEM_ACL_REDIRECT_TO_CPU, "reason_code");
  P_SYSTEM_ACL_REDIRECT_TO_CPU_QID = data_id_from_name(
      T_INGRESS_SYSTEM_ACL, A_SYSTEM_ACL_REDIRECT_TO_CPU, "qid");
  P_SYSTEM_ACL_REDIRECT_TO_CPU_METER_ID = data_id_from_name(
      T_INGRESS_SYSTEM_ACL, A_SYSTEM_ACL_REDIRECT_TO_CPU, "meter_id");
  P_SYSTEM_ACL_REDIRECT_TO_CPU_DISABLE_LEARNING = data_id_from_name(
      T_INGRESS_SYSTEM_ACL, A_SYSTEM_ACL_REDIRECT_TO_CPU, "disable_learning");
  P_SYSTEM_ACL_REDIRECT_TO_CPU_OVERWRITE_QID = data_id_from_name(
      T_INGRESS_SYSTEM_ACL, A_SYSTEM_ACL_REDIRECT_TO_CPU, "overwrite_qid");
  A_SYSTEM_ACL_COPY_SFLOW_TO_CPU = action_id_from_name(
      T_INGRESS_SYSTEM_ACL, "ingress_system_acl_copy_sflow_to_cpu");
  P_SYSTEM_ACL_COPY_SFLOW_TO_CPU_REASON_CODE = data_id_from_name(
      T_INGRESS_SYSTEM_ACL, A_SYSTEM_ACL_COPY_SFLOW_TO_CPU, "reason_code");
  P_SYSTEM_ACL_COPY_SFLOW_TO_CPU_QID = data_id_from_name(
      T_INGRESS_SYSTEM_ACL, A_SYSTEM_ACL_COPY_SFLOW_TO_CPU, "qid");
  P_SYSTEM_ACL_COPY_SFLOW_TO_CPU_METER_ID = data_id_from_name(
      T_INGRESS_SYSTEM_ACL, A_SYSTEM_ACL_COPY_SFLOW_TO_CPU, "meter_id");
  P_SYSTEM_ACL_COPY_SFLOW_TO_CPU_DISABLE_LEARNING = data_id_from_name(
      T_INGRESS_SYSTEM_ACL, A_SYSTEM_ACL_COPY_SFLOW_TO_CPU, "disable_learning");
  P_SYSTEM_ACL_COPY_SFLOW_TO_CPU_OVERWRITE_QID = data_id_from_name(
      T_INGRESS_SYSTEM_ACL, A_SYSTEM_ACL_COPY_SFLOW_TO_CPU, "overwrite_qid");
  A_SYSTEM_ACL_REDIRECT_SFLOW_TO_CPU = action_id_from_name(
      T_INGRESS_SYSTEM_ACL, "ingress_system_acl_redirect_sflow_to_cpu");
  P_SYSTEM_ACL_REDIRECT_SFLOW_TO_CPU_REASON_CODE = data_id_from_name(
      T_INGRESS_SYSTEM_ACL, A_SYSTEM_ACL_REDIRECT_SFLOW_TO_CPU, "reason_code");
  P_SYSTEM_ACL_REDIRECT_SFLOW_TO_CPU_QID = data_id_from_name(
      T_INGRESS_SYSTEM_ACL, A_SYSTEM_ACL_REDIRECT_SFLOW_TO_CPU, "qid");
  P_SYSTEM_ACL_REDIRECT_SFLOW_TO_CPU_METER_ID = data_id_from_name(
      T_INGRESS_SYSTEM_ACL, A_SYSTEM_ACL_REDIRECT_SFLOW_TO_CPU, "meter_id");
  P_SYSTEM_ACL_REDIRECT_SFLOW_TO_CPU_DISABLE_LEARNING =
      data_id_from_name(T_INGRESS_SYSTEM_ACL,
                        A_SYSTEM_ACL_REDIRECT_SFLOW_TO_CPU,
                        "disable_learning");
  P_SYSTEM_ACL_REDIRECT_SFLOW_TO_CPU_OVERWRITE_QID =
      data_id_from_name(T_INGRESS_SYSTEM_ACL,
                        A_SYSTEM_ACL_REDIRECT_SFLOW_TO_CPU,
                        "overwrite_qid");
  A_SYSTEM_ACL_REDIRECT_BFD_TO_CPU = action_id_from_name(
      T_INGRESS_SYSTEM_ACL, "ingress_system_acl_redirect_bfd_to_cpu");
  P_SYSTEM_ACL_REDIRECT_BFD_TO_CPU_REASON_CODE = data_id_from_name(
      T_INGRESS_SYSTEM_ACL, A_SYSTEM_ACL_REDIRECT_BFD_TO_CPU, "reason_code");
  P_SYSTEM_ACL_REDIRECT_BFD_TO_CPU_QID = data_id_from_name(
      T_INGRESS_SYSTEM_ACL, A_SYSTEM_ACL_REDIRECT_BFD_TO_CPU, "qid");
  P_SYSTEM_ACL_REDIRECT_BFD_TO_CPU_METER_ID = data_id_from_name(
      T_INGRESS_SYSTEM_ACL, A_SYSTEM_ACL_REDIRECT_BFD_TO_CPU, "meter_id");
  P_SYSTEM_ACL_REDIRECT_BFD_TO_CPU_DISABLE_LEARNING =
      data_id_from_name(T_INGRESS_SYSTEM_ACL,
                        A_SYSTEM_ACL_REDIRECT_BFD_TO_CPU,
                        "disable_learning");
  F_SYSTEM_ACL_LOCAL_MD_NAT_HIT =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.nat.hit");
  F_SYSTEM_ACL_LOCAL_MD_NAT_SAME_ZONE =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.checks.same_zone_check");
  F_SYSTEM_ACL_LOCAL_MD_TUNNEL_TERMINATE =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.tunnel.terminate");
  F_SYSTEM_ACL_LOCAL_MD_MULTICAST_HIT =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "local_md.multicast.hit");

  T_COPP = table_id_from_name("ingress_copp");
  F_COPP_PACKET_COLOR =
      key_id_from_name(T_COPP, "ig_intr_md_for_tm.packet_color");
  F_COPP_COPP_METER_ID = key_id_from_name(T_COPP, "copp_meter_id");
  F_COPP_PRIORITY = key_id_from_name(T_COPP, "$MATCH_PRIORITY");
  A_COPP_DROP = action_id_from_name(T_COPP, "ingress_copp_drop");
  A_COPP_PERMIT = action_id_from_name(T_COPP, "ingress_copp_permit");
  D_COPP_STATS_COUNTER_SPEC_PKTS =
      data_id_from_name(T_COPP, A_COPP_DROP, "$COUNTER_SPEC_PKTS");

  T_COPP_METER = table_id_from_name("ingress_copp_meter");
  F_COPP_METER_METER_INDEX = key_id_from_name(T_COPP_METER, "$METER_INDEX");
  D_COPP_METER_METER_SPEC_CIR_PPS =
      data_id_from_name_noaction(T_COPP_METER, "$METER_SPEC_CIR_PPS");
  D_COPP_METER_METER_SPEC_PIR_PPS =
      data_id_from_name_noaction(T_COPP_METER, "$METER_SPEC_PIR_PPS");
  D_COPP_METER_METER_SPEC_CBS_PKTS =
      data_id_from_name_noaction(T_COPP_METER, "$METER_SPEC_CBS_PKTS");
  D_COPP_METER_METER_SPEC_PBS_PKTS =
      data_id_from_name_noaction(T_COPP_METER, "$METER_SPEC_PBS_PKTS");

  T_EGRESS_COPP = table_id_from_name("egress_copp");
  F_EGRESS_COPP_PACKET_COLOR = key_id_from_name(T_EGRESS_COPP, "copp_color");
  F_EGRESS_COPP_COPP_METER_ID =
      key_id_from_name(T_EGRESS_COPP, "copp_meter_id");
  F_EGRESS_COPP_PRIORITY = key_id_from_name(T_EGRESS_COPP, "$MATCH_PRIORITY");
  A_EGRESS_COPP_DROP = action_id_from_name(T_EGRESS_COPP, "egress_copp_drop");
  A_EGRESS_COPP_PERMIT =
      action_id_from_name(T_EGRESS_COPP, "egress_copp_permit");
  D_EGRESS_COPP_STATS_COUNTER_SPEC_PKTS = data_id_from_name(
      T_EGRESS_COPP, A_EGRESS_COPP_DROP, "$COUNTER_SPEC_PKTS");

  T_EGRESS_COPP_METER = table_id_from_name("egress_copp_meter");
  F_EGRESS_COPP_METER_METER_INDEX =
      key_id_from_name(T_EGRESS_COPP_METER, "$METER_INDEX");
  D_EGRESS_COPP_METER_METER_SPEC_CIR_PPS =
      data_id_from_name_noaction(T_EGRESS_COPP_METER, "$METER_SPEC_CIR_PPS");
  D_EGRESS_COPP_METER_METER_SPEC_PIR_PPS =
      data_id_from_name_noaction(T_EGRESS_COPP_METER, "$METER_SPEC_PIR_PPS");
  D_EGRESS_COPP_METER_METER_SPEC_CBS_PKTS =
      data_id_from_name_noaction(T_EGRESS_COPP_METER, "$METER_SPEC_CBS_PKTS");
  D_EGRESS_COPP_METER_METER_SPEC_PBS_PKTS =
      data_id_from_name_noaction(T_EGRESS_COPP_METER, "$METER_SPEC_PBS_PKTS");

  T_EGRESS_MAC_ACL = table_id_from_name("egress_mac_acl.acl");
  F_EGRESS_MAC_ACL_LOCAL_MD_EGRESS_PORT_LAG_LABEL =
      key_id_from_name(T_EGRESS_MAC_ACL, "local_md.egress_port_lag_label");
  F_EGRESS_MAC_ACL_LOCAL_MD_BD_LABEL =
      key_id_from_name(T_EGRESS_MAC_ACL, "local_md.bd_label");
  F_EGRESS_MAC_ACL_LOCAL_MD_BD =
      key_id_from_name(T_EGRESS_MAC_ACL, "local_md.bd");
  F_EGRESS_MAC_ACL_SRC_ADDR =
      key_id_from_name(T_EGRESS_MAC_ACL, "hdr.ethernet.src_addr");
  F_EGRESS_MAC_ACL_DST_ADDR =
      key_id_from_name(T_EGRESS_MAC_ACL, "hdr.ethernet.dst_addr");
  F_EGRESS_MAC_ACL_ETHER_TYPE =
      key_id_from_name(T_EGRESS_MAC_ACL, "hdr.ethernet.ether_type");
  F_EGRESS_MAC_ACL_LOCAL_MD_USER_METADATA =
      key_id_from_name(T_EGRESS_MAC_ACL, "local_md.user_metadata");
  F_EGRESS_MAC_ACL_MATCH_PRIORITY =
      key_id_from_name(T_EGRESS_MAC_ACL, "$MATCH_PRIORITY");
  F_EGRESS_MAC_ACL_LOCAL_MD_EGRESS_PORT_LAG_INDEX =
      key_id_from_name(T_EGRESS_MAC_ACL, "local_md.egress_port_lag_index");
  F_EGRESS_MAC_ACL_HDR_VLAN_TAG0_PCP =
      key_id_from_name(T_EGRESS_MAC_ACL, "hdr.vlan_tag$0.pcp");
  F_EGRESS_MAC_ACL_HDR_VLAN_TAG0_DEI =
      key_id_from_name(T_EGRESS_MAC_ACL, "hdr.vlan_tag$0.dei");
  F_EGRESS_MAC_ACL_HDR_VLAN_TAG0_VALID =
      key_id_from_name(T_EGRESS_MAC_ACL, "hdr.vlan_tag$0.$valid");
  F_EGRESS_MAC_ACL_LOCAL_MD_FLAGS_ROUTED =
      key_id_from_name(T_EGRESS_MAC_ACL, "local_md.flags.routed");
  A_EGRESS_MAC_ACL_NO_ACTION =
      action_id_from_name(T_EGRESS_MAC_ACL, "egress_mac_acl.no_action");
  A_EGRESS_MAC_ACL_DROP =
      action_id_from_name(T_EGRESS_MAC_ACL, "egress_mac_acl.drop");
  A_EGRESS_MAC_ACL_PERMIT =
      action_id_from_name(T_EGRESS_MAC_ACL, "egress_mac_acl.permit");
  P_EGRESS_MAC_ACL_PERMIT_METER_INDEX = data_id_from_name(
      T_EGRESS_MAC_ACL, A_EGRESS_MAC_ACL_PERMIT, "meter_index");
  A_EGRESS_MAC_ACL_MIRROR =
      action_id_from_name(T_EGRESS_MAC_ACL, "egress_mac_acl.mirror_out");
  P_EGRESS_MAC_ACL_MIRROR_METER_INDEX = data_id_from_name(
      T_EGRESS_MAC_ACL, A_EGRESS_MAC_ACL_MIRROR, "meter_index");
  P_EGRESS_MAC_ACL_MIRROR_SESSION_ID = data_id_from_name(
      T_EGRESS_MAC_ACL, A_EGRESS_MAC_ACL_MIRROR, "session_id");
  D_EGRESS_MAC_ACL_COUNTER_SPEC_BYTES = data_id_from_name(
      T_EGRESS_MAC_ACL, A_EGRESS_MAC_ACL_NO_ACTION, "$COUNTER_SPEC_BYTES");
  D_EGRESS_MAC_ACL_COUNTER_SPEC_PKTS = data_id_from_name(
      T_EGRESS_MAC_ACL, A_EGRESS_MAC_ACL_NO_ACTION, "$COUNTER_SPEC_PKTS");

  T_EGRESS_L4_SRC_PORT = table_id_from_name("egress_l4_src_port");
  F_EGRESS_L4_SRC_PORT_LKP_L4_SRC_PORT =
      key_id_from_name(T_EGRESS_L4_SRC_PORT, "local_md.lkp.l4_src_port");
  F_EGRESS_L4_SRC_PORT_PRIORITY =
      key_id_from_name(T_EGRESS_L4_SRC_PORT, "$MATCH_PRIORITY");
  A_EGRESS_SET_SRC_PORT_LABEL =
      action_id_from_name(T_EGRESS_L4_SRC_PORT, "set_egress_src_port_label");
  P_EGRESS_SET_SRC_PORT_LABEL_LABEL = data_id_from_name(
      T_EGRESS_L4_SRC_PORT, A_EGRESS_SET_SRC_PORT_LABEL, "label");

  T_EGRESS_L4_DST_PORT = table_id_from_name("egress_l4_dst_port");
  F_EGRESS_L4_DST_PORT_LKP_L4_DST_PORT =
      key_id_from_name(T_EGRESS_L4_DST_PORT, "local_md.lkp.l4_dst_port");
  F_EGRESS_L4_DST_PORT_PRIORITY =
      key_id_from_name(T_EGRESS_L4_DST_PORT, "$MATCH_PRIORITY");
  A_EGRESS_SET_DST_PORT_LABEL =
      action_id_from_name(T_EGRESS_L4_DST_PORT, "set_egress_dst_port_label");
  P_EGRESS_SET_DST_PORT_LABEL_LABEL = data_id_from_name(
      T_EGRESS_L4_DST_PORT, A_EGRESS_SET_DST_PORT_LABEL, "label");

  T_EGRESS_IPV4_ACL = table_id_from_name("egress_ipv4_acl.acl");
  F_EGRESS_IPV4_ACL_LOCAL_MD_EGRESS_PORT_LAG_LABEL =
      key_id_from_name(T_EGRESS_IPV4_ACL, "local_md.egress_port_lag_label");
  F_EGRESS_IPV4_ACL_LOCAL_MD_BD_LABEL =
      key_id_from_name(T_EGRESS_IPV4_ACL, "local_md.bd_label");
  F_EGRESS_IPV4_ACL_LOCAL_MD_BD =
      key_id_from_name(T_EGRESS_IPV4_ACL, "local_md.bd");
  F_EGRESS_IPV4_ACL_HDR_IPV4_SRC_ADDR =
      key_id_from_name(T_EGRESS_IPV4_ACL, "hdr.ipv4.src_addr");
  F_EGRESS_IPV4_ACL_HDR_IPV4_DST_ADDR =
      key_id_from_name(T_EGRESS_IPV4_ACL, "hdr.ipv4.dst_addr");
  F_EGRESS_IPV4_ACL_HDR_IPV4_PROTOCOL =
      key_id_from_name(T_EGRESS_IPV4_ACL, "hdr.ipv4.protocol");
  F_EGRESS_IPV4_ACL_HDR_IPV4_DIFFSERV =
      key_id_from_name(T_EGRESS_IPV4_ACL, "hdr.ipv4.diffserv");
  F_EGRESS_IPV4_ACL_LOCAL_MD_LKP_TCP_FLAGS =
      key_id_from_name(T_EGRESS_IPV4_ACL, "local_md.lkp.tcp_flags");
  F_EGRESS_IPV4_ACL_LOCAL_MD_LKP_L4_SRC_PORT =
      key_id_from_name(T_EGRESS_IPV4_ACL, "local_md.lkp.l4_src_port");
  F_EGRESS_IPV4_ACL_LOCAL_MD_LKP_L4_DST_PORT =
      key_id_from_name(T_EGRESS_IPV4_ACL, "local_md.lkp.l4_dst_port");
  F_EGRESS_IPV4_ACL_LOCAL_MD_L4_SRC_PORT_LABEL =
      key_id_from_name(T_EGRESS_IPV4_ACL, "local_md.l4_src_port_label");
  F_EGRESS_IPV4_ACL_LOCAL_MD_L4_DST_PORT_LABEL =
      key_id_from_name(T_EGRESS_IPV4_ACL, "local_md.l4_dst_port_label");
  F_EGRESS_IPV4_ACL_LOCAL_MD_USER_METADATA =
      key_id_from_name(T_EGRESS_IPV4_ACL, "local_md.user_metadata");
  F_EGRESS_IPV4_ACL_LOCAL_MD_OUT_PORTS_GROUP_LABEL = key_id_from_name(
      T_EGRESS_IPV4_ACL, "local_md.out_ports_group_label_ipv4");
  F_EGRESS_IPV4_ACL_ETHER_TYPE =
      key_id_from_name(T_EGRESS_IPV4_ACL, "hdr.ethernet.ether_type");
  F_EGRESS_IPV4_ACL_MATCH_PRIORITY =
      key_id_from_name(T_EGRESS_IPV4_ACL, "$MATCH_PRIORITY");
  F_EGRESS_IPV4_ACL_LOCAL_MD_EGRESS_PORT_LAG_INDEX =
      key_id_from_name(T_EGRESS_IPV4_ACL, "local_md.egress_port_lag_index");
  F_EGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_SRC_ADDR =
      key_id_from_name(T_EGRESS_IPV4_ACL, "local_md.lkp.ip_src_addr_95_64");
  F_EGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_DST_ADDR =
      key_id_from_name(T_EGRESS_IPV4_ACL, "local_md.lkp.ip_dst_addr_95_64");
  F_EGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_PROTO =
      key_id_from_name(T_EGRESS_IPV4_ACL, "local_md.lkp.ip_proto");
  F_EGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_TOS =
      key_id_from_name(T_EGRESS_IPV4_ACL, "local_md.lkp.ip_tos");
  F_EGRESS_IPV4_ACL_LOCAL_MD_FLAGS_ROUTED =
      key_id_from_name(T_EGRESS_IPV4_ACL, "local_md.flags.routed");
  A_EGRESS_IPV4_ACL_NO_ACTION =
      action_id_from_name(T_EGRESS_IPV4_ACL, "egress_ipv4_acl.no_action");
  A_EGRESS_IPV4_ACL_DROP =
      action_id_from_name(T_EGRESS_IPV4_ACL, "egress_ipv4_acl.drop");
  A_EGRESS_IPV4_ACL_PERMIT =
      action_id_from_name(T_EGRESS_IPV4_ACL, "egress_ipv4_acl.permit");
  P_EGRESS_IPV4_ACL_PERMIT_METER_INDEX = data_id_from_name(
      T_EGRESS_IPV4_ACL, A_EGRESS_IPV4_ACL_PERMIT, "meter_index");
  A_EGRESS_IPV4_ACL_MIRROR =
      action_id_from_name(T_EGRESS_IPV4_ACL, "egress_ipv4_acl.mirror_out");
  P_EGRESS_IPV4_ACL_MIRROR_METER_INDEX = data_id_from_name(
      T_EGRESS_IPV4_ACL, A_EGRESS_IPV4_ACL_MIRROR, "meter_index");
  P_EGRESS_IPV4_ACL_MIRROR_SESSION_ID = data_id_from_name(
      T_EGRESS_IPV4_ACL, A_EGRESS_IPV4_ACL_MIRROR, "session_id");
  A_EGRESS_IPV4_ACL_MIRROR_IN =
      action_id_from_name(T_EGRESS_IPV4_ACL, "egress_ipv4_acl.mirror_in");
  P_EGRESS_IPV4_ACL_MIRROR_IN_METER_INDEX = data_id_from_name(
      T_EGRESS_IPV4_ACL, A_EGRESS_IPV4_ACL_MIRROR_IN, "meter_index");
  P_EGRESS_IPV4_ACL_MIRROR_IN_SESSION_ID = data_id_from_name(
      T_EGRESS_IPV4_ACL, A_EGRESS_IPV4_ACL_MIRROR_IN, "session_id");
  D_EGRESS_IPV4_ACL_COUNTER_SPEC_BYTES = data_id_from_name(
      T_EGRESS_IPV4_ACL, A_EGRESS_IPV4_ACL_NO_ACTION, "$COUNTER_SPEC_BYTES");
  D_EGRESS_IPV4_ACL_COUNTER_SPEC_PKTS = data_id_from_name(
      T_EGRESS_IPV4_ACL, A_EGRESS_IPV4_ACL_NO_ACTION, "$COUNTER_SPEC_PKTS");

  T_EGRESS_IPV6_ACL = table_id_from_name("egress_ipv6_acl.acl");
  F_EGRESS_IPV6_ACL_LOCAL_MD_EGRESS_PORT_LAG_LABEL =
      key_id_from_name(T_EGRESS_IPV6_ACL, "local_md.egress_port_lag_label");
  F_EGRESS_IPV6_ACL_LOCAL_MD_BD_LABEL =
      key_id_from_name(T_EGRESS_IPV6_ACL, "local_md.bd_label");
  F_EGRESS_IPV6_ACL_LOCAL_MD_BD =
      key_id_from_name(T_EGRESS_IPV6_ACL, "local_md.bd");
  F_EGRESS_IPV6_ACL_HDR_IPV6_SRC_ADDR =
      key_id_from_name(T_EGRESS_IPV6_ACL, "hdr.ipv6.src_addr");
  F_EGRESS_IPV6_ACL_HDR_IPV6_SRC_ADDR_WORD3 =
      key_id_from_name(T_EGRESS_IPV6_ACL, "hdr.ipv6.src_addr[127:96]");
  F_EGRESS_IPV6_ACL_HDR_IPV6_SRC_ADDR_WORD2 =
      key_id_from_name(T_EGRESS_IPV6_ACL, "hdr.ipv6.src_addr[95:64]");
  F_EGRESS_IPV6_ACL_HDR_IPV6_DST_ADDR =
      key_id_from_name(T_EGRESS_IPV6_ACL, "hdr.ipv6.dst_addr");
  F_EGRESS_IPV6_ACL_HDR_IPV6_DST_ADDR_WORD3 =
      key_id_from_name(T_EGRESS_IPV6_ACL, "hdr.ipv6.dst_addr[127:96]");
  F_EGRESS_IPV6_ACL_HDR_IPV6_DST_ADDR_WORD2 =
      key_id_from_name(T_EGRESS_IPV6_ACL, "hdr.ipv6.dst_addr[95:64]");
  F_EGRESS_IPV6_ACL_HDR_IPV6_NEXT_HDR =
      key_id_from_name(T_EGRESS_IPV6_ACL, "hdr.ipv6.next_hdr");
  F_EGRESS_IPV6_ACL_HDR_IPV6_TRAFFIC_CLASS =
      key_id_from_name(T_EGRESS_IPV6_ACL, "hdr.ipv6.traffic_class");
  F_EGRESS_IPV6_ACL_LOCAL_MD_LKP_TCP_FLAGS =
      key_id_from_name(T_EGRESS_IPV6_ACL, "local_md.lkp.tcp_flags");
  F_EGRESS_IPV6_ACL_LOCAL_MD_LKP_L4_SRC_PORT =
      key_id_from_name(T_EGRESS_IPV6_ACL, "local_md.lkp.l4_src_port");
  F_EGRESS_IPV6_ACL_LOCAL_MD_LKP_L4_DST_PORT =
      key_id_from_name(T_EGRESS_IPV6_ACL, "local_md.lkp.l4_dst_port");
  F_EGRESS_IPV6_ACL_LOCAL_MD_L4_SRC_PORT_LABEL =
      key_id_from_name(T_EGRESS_IPV6_ACL, "local_md.l4_src_port_label");
  F_EGRESS_IPV6_ACL_LOCAL_MD_L4_DST_PORT_LABEL =
      key_id_from_name(T_EGRESS_IPV6_ACL, "local_md.l4_dst_port_label");
  F_EGRESS_IPV6_ACL_LOCAL_MD_USER_METADATA =
      key_id_from_name(T_EGRESS_IPV6_ACL, "local_md.user_metadata");
  F_EGRESS_IPV6_ACL_LOCAL_MD_OUT_PORTS_GROUP_LABEL = key_id_from_name(
      T_EGRESS_IPV6_ACL, "local_md.out_ports_group_label_ipv6");
  F_EGRESS_IPV6_ACL_ETHER_TYPE =
      key_id_from_name(T_EGRESS_IPV6_ACL, "hdr.ethernet.ether_type");
  F_EGRESS_IPV6_ACL_MATCH_PRIORITY =
      key_id_from_name(T_EGRESS_IPV6_ACL, "$MATCH_PRIORITY");
  F_EGRESS_IPV6_ACL_LOCAL_MD_EGRESS_PORT_LAG_INDEX =
      key_id_from_name(T_EGRESS_IPV6_ACL, "local_md.egress_port_lag_index");
  F_EGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_SRC_ADDR =
      key_id_from_name(T_EGRESS_IPV6_ACL, "local_md.lkp.ip_src_addr");
  F_EGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_DST_ADDR =
      key_id_from_name(T_EGRESS_IPV6_ACL, "local_md.lkp.ip_dst_addr");
  F_EGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD2 =
      key_id_from_name(T_EGRESS_IPV6_ACL, "local_md.lkp.ip_src_addr_95_64");
  F_EGRESS_IPV6_ACL_HDR_IPV6_SRC_ADDR_WORD10 =
      key_id_from_name(T_EGRESS_IPV6_ACL, "hdr.ipv6.src_addr[63:0]");
  F_EGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD2 =
      key_id_from_name(T_EGRESS_IPV6_ACL, "local_md.lkp.ip_dst_addr_95_64");
  F_EGRESS_IPV6_ACL_HDR_IPV6_DST_ADDR_WORD10 =
      key_id_from_name(T_EGRESS_IPV6_ACL, "hdr.ipv6.dst_addr[63:0]");
  F_EGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_PROTO =
      key_id_from_name(T_EGRESS_IPV6_ACL, "local_md.lkp.ip_proto");
  F_EGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_TOS =
      key_id_from_name(T_EGRESS_IPV6_ACL, "local_md.lkp.ip_tos");
  F_EGRESS_IPV6_ACL_LOCAL_MD_FLAGS_ROUTED =
      key_id_from_name(T_EGRESS_IPV6_ACL, "local_md.flags.routed");
  A_EGRESS_IPV6_ACL_NO_ACTION =
      action_id_from_name(T_EGRESS_IPV6_ACL, "egress_ipv6_acl.no_action");
  A_EGRESS_IPV6_ACL_DROP =
      action_id_from_name(T_EGRESS_IPV6_ACL, "egress_ipv6_acl.drop");
  A_EGRESS_IPV6_ACL_PERMIT =
      action_id_from_name(T_EGRESS_IPV6_ACL, "egress_ipv6_acl.permit");
  P_EGRESS_IPV6_ACL_PERMIT_METER_INDEX = data_id_from_name(
      T_EGRESS_IPV6_ACL, A_EGRESS_IPV6_ACL_PERMIT, "meter_index");
  A_EGRESS_IPV6_ACL_MIRROR =
      action_id_from_name(T_EGRESS_IPV6_ACL, "egress_ipv6_acl.mirror_out");
  P_EGRESS_IPV6_ACL_MIRROR_SESSION_ID = data_id_from_name(
      T_EGRESS_IPV6_ACL, A_EGRESS_IPV6_ACL_MIRROR, "session_id");
  P_EGRESS_IPV6_ACL_MIRROR_METER_INDEX = data_id_from_name(
      T_EGRESS_IPV6_ACL, A_EGRESS_IPV6_ACL_MIRROR, "meter_index");
  A_EGRESS_IPV6_ACL_MIRROR_IN =
      action_id_from_name(T_EGRESS_IPV6_ACL, "egress_ipv6_acl.mirror_in");
  P_EGRESS_IPV6_ACL_MIRROR_IN_SESSION_ID = data_id_from_name(
      T_EGRESS_IPV6_ACL, A_EGRESS_IPV6_ACL_MIRROR_IN, "session_id");
  P_EGRESS_IPV6_ACL_MIRROR_IN_METER_INDEX = data_id_from_name(
      T_EGRESS_IPV6_ACL, A_EGRESS_IPV6_ACL_MIRROR_IN, "meter_index");
  D_EGRESS_IPV6_ACL_COUNTER_SPEC_BYTES = data_id_from_name(
      T_EGRESS_IPV6_ACL, A_EGRESS_IPV6_ACL_NO_ACTION, "$COUNTER_SPEC_BYTES");
  D_EGRESS_IPV6_ACL_COUNTER_SPEC_PKTS = data_id_from_name(
      T_EGRESS_IPV6_ACL, A_EGRESS_IPV6_ACL_NO_ACTION, "$COUNTER_SPEC_PKTS");

  T_EGRESS_TOS_MIRROR_ACL = table_id_from_name("egress_tos_mirror_acl.acl");
  F_EGRESS_TOS_MIRROR_ACL_LOCAL_MD_EGRESS_PORT_LAG_LABEL = key_id_from_name(
      T_EGRESS_TOS_MIRROR_ACL, "local_md.egress_port_lag_label");
  F_EGRESS_TOS_MIRROR_ACL_HDR_IPV6_VALID =
      key_id_from_name(T_EGRESS_TOS_MIRROR_ACL, "hdr.ipv6.$valid");
  F_EGRESS_TOS_MIRROR_ACL_HDR_IPV4_VALID =
      key_id_from_name(T_EGRESS_TOS_MIRROR_ACL, "hdr.ipv4.$valid");
  F_EGRESS_TOS_MIRROR_ACL_HDR_IPV6_TRAFFIC_CLASS =
      key_id_from_name(T_EGRESS_TOS_MIRROR_ACL, "hdr.ipv6.traffic_class");
  F_EGRESS_TOS_MIRROR_ACL_HDR_IPV4_DIFFSERV =
      key_id_from_name(T_EGRESS_TOS_MIRROR_ACL, "hdr.ipv4.diffserv");
  F_EGRESS_TOS_MIRROR_ACL_MATCH_PRIORITY =
      key_id_from_name(T_EGRESS_TOS_MIRROR_ACL, "$MATCH_PRIORITY");
  A_EGRESS_TOS_MIRROR_ACL_MIRROR_IN = action_id_from_name(
      T_EGRESS_TOS_MIRROR_ACL, "egress_tos_mirror_acl.mirror_in");
  P_EGRESS_TOS_MIRROR_ACL_MIRROR_IN_SESSION_ID = data_id_from_name(
      T_EGRESS_TOS_MIRROR_ACL, A_EGRESS_TOS_MIRROR_ACL_MIRROR_IN, "session_id");
  P_EGRESS_TOS_MIRROR_ACL_MIRROR_IN_METER_INDEX =
      data_id_from_name(T_EGRESS_TOS_MIRROR_ACL,
                        A_EGRESS_TOS_MIRROR_ACL_MIRROR_IN,
                        "meter_index");
  A_EGRESS_TOS_MIRROR_ACL_MIRROR = action_id_from_name(
      T_EGRESS_TOS_MIRROR_ACL, "egress_tos_mirror_acl.mirror_out");
  P_EGRESS_TOS_MIRROR_ACL_MIRROR_SESSION_ID = data_id_from_name(
      T_EGRESS_TOS_MIRROR_ACL, A_EGRESS_TOS_MIRROR_ACL_MIRROR, "session_id");
  P_EGRESS_TOS_MIRROR_ACL_MIRROR_METER_INDEX = data_id_from_name(
      T_EGRESS_TOS_MIRROR_ACL, A_EGRESS_TOS_MIRROR_ACL_MIRROR, "meter_index");
  D_EGRESS_TOS_MIRROR_ACL_COUNTER_SPEC_BYTES =
      data_id_from_name(T_EGRESS_TOS_MIRROR_ACL,
                        A_EGRESS_TOS_MIRROR_ACL_MIRROR,
                        "$COUNTER_SPEC_BYTES");
  D_EGRESS_TOS_MIRROR_ACL_COUNTER_SPEC_PKTS =
      data_id_from_name(T_EGRESS_TOS_MIRROR_ACL,
                        A_EGRESS_TOS_MIRROR_ACL_MIRROR,
                        "$COUNTER_SPEC_PKTS");

  T_EGRESS_DROP_STATS = table_id_from_name("egress_drop_stats");
  F_EGRESS_DROP_STATS_PORT = key_id_from_name(T_EGRESS_DROP_STATS, "port");
  F_EGRESS_DROP_STATS_DROP_REASON =
      key_id_from_name(T_EGRESS_DROP_STATS, "drop_reason");
  A_EGRESS_DROP_STATS_COUNT =
      action_id_from_name(T_EGRESS_DROP_STATS, "egress_drop_stats_count");
  D_EGRESS_DROP_STATS_PKTS = data_id_from_name(
      T_EGRESS_DROP_STATS, A_EGRESS_DROP_STATS_COUNT, "$COUNTER_SPEC_PKTS");

  T_EGRESS_SYSTEM_ACL = table_id_from_name("egress_system_acl");
  F_EGRESS_SYSTEM_ACL_EG_INTR_MD_EGRESS_PORT =
      key_id_from_name(T_EGRESS_SYSTEM_ACL, "eg_intr_md.egress_port");
  F_EGRESS_SYSTEM_ACL_LOCAL_MD_FLAGS_ACL_DENY =
      key_id_from_name(T_EGRESS_SYSTEM_ACL, "local_md.flags.acl_deny");
  F_EGRESS_SYSTEM_ACL_LOCAL_MD_FLAGS_WRED_DROP =
      key_id_from_name(T_EGRESS_SYSTEM_ACL, "local_md.flags.wred_drop");
  F_EGRESS_SYSTEM_ACL_LOCAL_MD_FLAGS_PFC_WD_DROP =
      key_id_from_name(T_EGRESS_SYSTEM_ACL, "local_md.flags.pfc_wd_drop");
  F_EGRESS_SYSTEM_ACL_LOCAL_MD_FLAGS_METER_PACKET_ACTION = key_id_from_name(
      T_EGRESS_SYSTEM_ACL, "local_md.flags.meter_packet_action");
  F_EGRESS_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_METER_DROP =
      key_id_from_name(T_EGRESS_SYSTEM_ACL, "local_md.flags.port_meter_drop");
  F_EGRESS_SYSTEM_ACL_LOCAL_MD_QOS_ACL_METER_COLOR =
      key_id_from_name(T_EGRESS_SYSTEM_ACL, "local_md.qos.acl_meter_color");
  F_EGRESS_SYSTEM_ACL_LOCAL_MD_MIRROR_ACL_METER_COLOR =
      key_id_from_name(T_EGRESS_SYSTEM_ACL, "local_md.mirror.meter_color");
  F_EGRESS_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_ISOLATION_PACKET_DROP =
      key_id_from_name(T_EGRESS_SYSTEM_ACL,
                       "local_md.flags.port_isolation_packet_drop");
  F_EGRESS_SYSTEM_ACL_LOCAL_MD_FLAGS_BPORT_ISOLATION_PACKET_DROP =
      key_id_from_name(T_EGRESS_SYSTEM_ACL,
                       "local_md.flags.bport_isolation_packet_drop");
  F_EGRESS_SYSTEM_ACL_LOCAL_MD_CHECKS_MTU =
      key_id_from_name(T_EGRESS_SYSTEM_ACL, "local_md.checks.mtu");
  F_EGRESS_SYSTEM_ACL_PRIORITY =
      key_id_from_name(T_EGRESS_SYSTEM_ACL, "$MATCH_PRIORITY");
  F_EGRESS_SYSTEM_ACL_LOCAL_MD_CHECKS_STP =
      key_id_from_name(T_EGRESS_SYSTEM_ACL, "local_md.checks.stp");
  A_EGRESS_SYSTEM_ACL_DROP =
      action_id_from_name(T_EGRESS_SYSTEM_ACL, "egress_system_acl_drop");
  P_EGRESS_SYSTEM_ACL_DROP_REASON_CODE = data_id_from_name(
      T_EGRESS_SYSTEM_ACL, A_EGRESS_SYSTEM_ACL_DROP, "reason_code");
  A_EGRESS_SYSTEM_ACL_MIRROR_METER_DROP = action_id_from_name(
      T_EGRESS_SYSTEM_ACL, "egress_system_acl_mirror_meter_drop");
  P_EGRESS_SYSTEM_ACL_MIRROR_METER_DROP_REASON_CODE =
      data_id_from_name(T_EGRESS_SYSTEM_ACL,
                        A_EGRESS_SYSTEM_ACL_MIRROR_METER_DROP,
                        "reason_code");
  A_EGRESS_SYSTEM_ACL_COPY_TO_CPU =
      action_id_from_name(T_EGRESS_SYSTEM_ACL, "egress_system_acl_copy_to_cpu");
  P_EGRESS_SYSTEM_ACL_COPY_TO_CPU_REASON_CODE = data_id_from_name(
      T_EGRESS_SYSTEM_ACL, A_EGRESS_SYSTEM_ACL_COPY_TO_CPU, "reason_code");
  P_EGRESS_SYSTEM_ACL_COPY_TO_CPU_METER_ID = data_id_from_name(
      T_EGRESS_SYSTEM_ACL, A_EGRESS_SYSTEM_ACL_COPY_TO_CPU, "meter_id");
  A_EGRESS_SYSTEM_ACL_REDIRECT_TO_CPU = action_id_from_name(
      T_EGRESS_SYSTEM_ACL, "egress_system_acl_redirect_to_cpu");
  P_EGRESS_SYSTEM_ACL_REDIRECT_TO_CPU_REASON_CODE = data_id_from_name(
      T_EGRESS_SYSTEM_ACL, A_EGRESS_SYSTEM_ACL_REDIRECT_TO_CPU, "reason_code");
  P_EGRESS_SYSTEM_ACL_REDIRECT_TO_CPU_METER_ID = data_id_from_name(
      T_EGRESS_SYSTEM_ACL, A_EGRESS_SYSTEM_ACL_REDIRECT_TO_CPU, "meter_id");
  D_EGRESS_SYSTEM_ACL_DROP_STATS_PKTS = data_id_from_name(
      T_EGRESS_SYSTEM_ACL, A_EGRESS_SYSTEM_ACL_DROP, "$COUNTER_SPEC_PKTS");

  T_EGRESS_IPV4_MIRROR_ACL = table_id_from_name("egress_ipv4_mirror_acl.acl");
  F_EGRESS_IPV4_MIRROR_ACL_LOCAL_MD_EGRESS_PORT_LAG_LABEL = key_id_from_name(
      T_EGRESS_IPV4_MIRROR_ACL, "local_md.egress_port_lag_label");
  F_EGRESS_IPV4_MIRROR_ACL_LOCAL_MD_BD_LABEL =
      key_id_from_name(T_EGRESS_IPV4_MIRROR_ACL, "local_md.bd_label");
  F_EGRESS_IPV4_MIRROR_ACL_LOCAL_MD_BD =
      key_id_from_name(T_EGRESS_IPV4_MIRROR_ACL, "local_md.bd");
  F_EGRESS_IPV4_MIRROR_ACL_HDR_IPV4_SRC_ADDR =
      key_id_from_name(T_EGRESS_IPV4_MIRROR_ACL, "hdr.ipv4.src_addr");
  F_EGRESS_IPV4_MIRROR_ACL_HDR_IPV4_DST_ADDR =
      key_id_from_name(T_EGRESS_IPV4_MIRROR_ACL, "hdr.ipv4.dst_addr");
  F_EGRESS_IPV4_MIRROR_ACL_HDR_IPV4_PROTOCOL =
      key_id_from_name(T_EGRESS_IPV4_MIRROR_ACL, "hdr.ipv4.protocol");
  F_EGRESS_IPV4_MIRROR_ACL_HDR_IPV4_DIFFSERV =
      key_id_from_name(T_EGRESS_IPV4_MIRROR_ACL, "hdr.ipv4.diffserv");
  F_EGRESS_IPV4_MIRROR_ACL_ETHER_TYPE =
      key_id_from_name(T_EGRESS_IPV4_MIRROR_ACL, "hdr.ethernet.ether_type");
  F_EGRESS_IPV4_MIRROR_ACL_LOCAL_MD_LKP_TCP_FLAGS =
      key_id_from_name(T_EGRESS_IPV4_MIRROR_ACL, "local_md.lkp.tcp_flags");
  F_EGRESS_IPV4_MIRROR_ACL_LOCAL_MD_LKP_L4_SRC_PORT =
      key_id_from_name(T_EGRESS_IPV4_MIRROR_ACL, "local_md.lkp.l4_src_port");
  F_EGRESS_IPV4_MIRROR_ACL_LOCAL_MD_LKP_L4_DST_PORT =
      key_id_from_name(T_EGRESS_IPV4_MIRROR_ACL, "local_md.lkp.l4_dst_port");
  F_EGRESS_IPV4_MIRROR_ACL_LOCAL_MD_L4_SRC_PORT_LABEL =
      key_id_from_name(T_EGRESS_IPV4_MIRROR_ACL, "local_md.l4_src_port_label");
  F_EGRESS_IPV4_MIRROR_ACL_LOCAL_MD_L4_DST_PORT_LABEL =
      key_id_from_name(T_EGRESS_IPV4_MIRROR_ACL, "local_md.l4_dst_port_label");
  F_EGRESS_IPV4_MIRROR_ACL_LOCAL_MD_OUT_PORTS_GROUP_LABEL = key_id_from_name(
      T_EGRESS_IPV4_MIRROR_ACL, "local_md.out_ports_group_label_ipv4");
  F_EGRESS_IPV4_MIRROR_ACL_MATCH_PRIORITY =
      key_id_from_name(T_EGRESS_IPV4_MIRROR_ACL, "$MATCH_PRIORITY");
  A_EGRESS_IPV4_MIRROR_ACL_NO_ACTION = action_id_from_name(
      T_EGRESS_IPV4_MIRROR_ACL, "egress_ipv4_mirror_acl.no_action");
  A_EGRESS_IPV4_MIRROR_ACL_DROP = action_id_from_name(
      T_EGRESS_IPV4_MIRROR_ACL, "egress_ipv4_mirror_acl.drop");
  A_EGRESS_IPV4_MIRROR_ACL_PERMIT = action_id_from_name(
      T_EGRESS_IPV4_MIRROR_ACL, "egress_ipv4_mirror_acl.permit");
  P_EGRESS_IPV4_MIRROR_ACL_PERMIT_METER_INDEX = data_id_from_name(
      T_EGRESS_IPV4_MIRROR_ACL, A_EGRESS_IPV4_MIRROR_ACL_PERMIT, "meter_index");
  A_EGRESS_IPV4_MIRROR_ACL_MIRROR = action_id_from_name(
      T_EGRESS_IPV4_MIRROR_ACL, "egress_ipv4_mirror_acl.mirror_out");
  P_EGRESS_IPV4_MIRROR_ACL_MIRROR_METER_INDEX = data_id_from_name(
      T_EGRESS_IPV4_MIRROR_ACL, A_EGRESS_IPV4_MIRROR_ACL_MIRROR, "meter_index");
  P_EGRESS_IPV4_MIRROR_ACL_MIRROR_SESSION_ID = data_id_from_name(
      T_EGRESS_IPV4_MIRROR_ACL, A_EGRESS_IPV4_MIRROR_ACL_MIRROR, "session_id");
  A_EGRESS_IPV4_MIRROR_ACL_MIRROR_IN = action_id_from_name(
      T_EGRESS_IPV4_MIRROR_ACL, "egress_ipv4_mirror_acl.mirror_in");
  P_EGRESS_IPV4_MIRROR_ACL_MIRROR_IN_METER_INDEX =
      data_id_from_name(T_EGRESS_IPV4_MIRROR_ACL,
                        A_EGRESS_IPV4_MIRROR_ACL_MIRROR_IN,
                        "meter_index");
  P_EGRESS_IPV4_MIRROR_ACL_MIRROR_IN_SESSION_ID =
      data_id_from_name(T_EGRESS_IPV4_MIRROR_ACL,
                        A_EGRESS_IPV4_MIRROR_ACL_MIRROR_IN,
                        "session_id");
  D_EGRESS_IPV4_MIRROR_ACL_COUNTER_SPEC_BYTES =
      data_id_from_name(T_EGRESS_IPV4_MIRROR_ACL,
                        A_EGRESS_IPV4_MIRROR_ACL_PERMIT,
                        "$COUNTER_SPEC_BYTES");
  D_EGRESS_IPV4_MIRROR_ACL_COUNTER_SPEC_PKTS =
      data_id_from_name(T_EGRESS_IPV4_MIRROR_ACL,
                        A_EGRESS_IPV4_MIRROR_ACL_PERMIT,
                        "$COUNTER_SPEC_PKTS");

  T_EGRESS_IPV6_MIRROR_ACL = table_id_from_name("egress_ipv6_mirror_acl.acl");
  F_EGRESS_IPV6_MIRROR_ACL_LOCAL_MD_EGRESS_PORT_LAG_LABEL = key_id_from_name(
      T_EGRESS_IPV6_MIRROR_ACL, "local_md.egress_port_lag_label");
  F_EGRESS_IPV6_MIRROR_ACL_LOCAL_MD_BD_LABEL =
      key_id_from_name(T_EGRESS_IPV6_MIRROR_ACL, "local_md.bd_label");
  F_EGRESS_IPV6_MIRROR_ACL_LOCAL_MD_BD =
      key_id_from_name(T_EGRESS_IPV6_MIRROR_ACL, "local_md.bd");
  F_EGRESS_IPV6_MIRROR_ACL_HDR_IPV6_SRC_ADDR =
      key_id_from_name(T_EGRESS_IPV6_MIRROR_ACL, "hdr.ipv6.src_addr");
  F_EGRESS_IPV6_MIRROR_ACL_HDR_IPV6_DST_ADDR =
      key_id_from_name(T_EGRESS_IPV6_MIRROR_ACL, "hdr.ipv6.dst_addr");
  F_EGRESS_IPV6_MIRROR_ACL_HDR_IPV6_NEXT_HDR =
      key_id_from_name(T_EGRESS_IPV6_MIRROR_ACL, "hdr.ipv6.next_hdr");
  F_EGRESS_IPV6_MIRROR_ACL_HDR_IPV6_TRAFFIC_CLASS =
      key_id_from_name(T_EGRESS_IPV6_MIRROR_ACL, "hdr.ipv6.traffic_class");
  F_EGRESS_IPV6_MIRROR_ACL_ETHER_TYPE =
      key_id_from_name(T_EGRESS_IPV6_MIRROR_ACL, "hdr.ethernet.ether_type");
  F_EGRESS_IPV6_MIRROR_ACL_LOCAL_MD_LKP_TCP_FLAGS =
      key_id_from_name(T_EGRESS_IPV6_MIRROR_ACL, "local_md.lkp.tcp_flags");
  F_EGRESS_IPV6_MIRROR_ACL_LOCAL_MD_LKP_L4_SRC_PORT =
      key_id_from_name(T_EGRESS_IPV6_MIRROR_ACL, "local_md.lkp.l4_src_port");
  F_EGRESS_IPV6_MIRROR_ACL_LOCAL_MD_LKP_L4_DST_PORT =
      key_id_from_name(T_EGRESS_IPV6_MIRROR_ACL, "local_md.lkp.l4_dst_port");
  F_EGRESS_IPV6_MIRROR_ACL_LOCAL_MD_L4_SRC_PORT_LABEL =
      key_id_from_name(T_EGRESS_IPV6_MIRROR_ACL, "local_md.l4_src_port_label");
  F_EGRESS_IPV6_MIRROR_ACL_LOCAL_MD_L4_DST_PORT_LABEL =
      key_id_from_name(T_EGRESS_IPV6_MIRROR_ACL, "local_md.l4_dst_port_label");
  F_EGRESS_IPV6_MIRROR_ACL_LOCAL_MD_OUT_PORTS_GROUP_LABEL = key_id_from_name(
      T_EGRESS_IPV6_MIRROR_ACL, "local_md.out_ports_group_label_ipv6");
  F_EGRESS_IPV6_MIRROR_ACL_MATCH_PRIORITY =
      key_id_from_name(T_EGRESS_IPV6_MIRROR_ACL, "$MATCH_PRIORITY");
  A_EGRESS_IPV6_MIRROR_ACL_NO_ACTION = action_id_from_name(
      T_EGRESS_IPV6_MIRROR_ACL, "egress_ipv6_mirror_acl.no_action");
  A_EGRESS_IPV6_MIRROR_ACL_DROP = action_id_from_name(
      T_EGRESS_IPV6_MIRROR_ACL, "egress_ipv6_mirror_acl.drop");
  A_EGRESS_IPV6_MIRROR_ACL_PERMIT = action_id_from_name(
      T_EGRESS_IPV6_MIRROR_ACL, "egress_ipv6_mirror_acl.permit");
  P_EGRESS_IPV6_MIRROR_ACL_PERMIT_METER_INDEX = data_id_from_name(
      T_EGRESS_IPV6_MIRROR_ACL, A_EGRESS_IPV6_MIRROR_ACL_PERMIT, "meter_index");
  A_EGRESS_IPV6_MIRROR_ACL_MIRROR = action_id_from_name(
      T_EGRESS_IPV6_MIRROR_ACL, "egress_ipv6_mirror_acl.mirror_out");
  P_EGRESS_IPV6_MIRROR_ACL_MIRROR_SESSION_ID = data_id_from_name(
      T_EGRESS_IPV6_MIRROR_ACL, A_EGRESS_IPV6_MIRROR_ACL_MIRROR, "session_id");
  P_EGRESS_IPV6_MIRROR_ACL_MIRROR_METER_INDEX = data_id_from_name(
      T_EGRESS_IPV6_MIRROR_ACL, A_EGRESS_IPV6_MIRROR_ACL_MIRROR, "meter_index");
  A_EGRESS_IPV6_MIRROR_ACL_MIRROR_IN = action_id_from_name(
      T_EGRESS_IPV6_MIRROR_ACL, "egress_ipv6_mirror_acl.mirror_in");
  P_EGRESS_IPV6_MIRROR_ACL_MIRROR_IN_SESSION_ID =
      data_id_from_name(T_EGRESS_IPV6_MIRROR_ACL,
                        A_EGRESS_IPV6_MIRROR_ACL_MIRROR_IN,
                        "session_id");
  P_EGRESS_IPV6_MIRROR_ACL_MIRROR_IN_METER_INDEX =
      data_id_from_name(T_EGRESS_IPV6_MIRROR_ACL,
                        A_EGRESS_IPV6_MIRROR_ACL_MIRROR_IN,
                        "meter_index");
  D_EGRESS_IPV6_MIRROR_ACL_COUNTER_SPEC_BYTES =
      data_id_from_name(T_EGRESS_IPV6_MIRROR_ACL,
                        A_EGRESS_IPV6_MIRROR_ACL_PERMIT,
                        "$COUNTER_SPEC_BYTES");
  D_EGRESS_IPV6_MIRROR_ACL_COUNTER_SPEC_PKTS =
      data_id_from_name(T_EGRESS_IPV6_MIRROR_ACL,
                        A_EGRESS_IPV6_MIRROR_ACL_PERMIT,
                        "$COUNTER_SPEC_PKTS");

  //  start acl2.p4 egress
  T_EGRESS_IP_MIRROR_ACL = table_id_from_name("egress_ip_mirror_acl.acl");
  F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_BD =
      key_id_from_name(T_EGRESS_IP_MIRROR_ACL, "local_md.bd");
  F_EGRESS_IP_MIRROR_ACL_HDR_IP_SRC_ADDR_WORD3 =
      key_id_from_name(T_EGRESS_IP_MIRROR_ACL, "hdr.ipv6.src_addr[127:96]");
  F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD2 = key_id_from_name(
      T_EGRESS_IP_MIRROR_ACL, "local_md.lkp.ip_src_addr_95_64");
  F_EGRESS_IP_MIRROR_ACL_HDR_IPV6_SRC_ADDR_WORD10 =
      key_id_from_name(T_EGRESS_IP_MIRROR_ACL, "hdr.ipv6.src_addr[63:0]");
  F_EGRESS_IP_MIRROR_ACL_HDR_IP_DST_ADDR_WORD3 =
      key_id_from_name(T_EGRESS_IP_MIRROR_ACL, "hdr.ipv6.dst_addr[127:96]");
  F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD2 = key_id_from_name(
      T_EGRESS_IP_MIRROR_ACL, "local_md.lkp.ip_dst_addr_95_64");
  F_EGRESS_IP_MIRROR_ACL_HDR_IPV6_DST_ADDR_WORD10 =
      key_id_from_name(T_EGRESS_IP_MIRROR_ACL, "hdr.ipv6.dst_addr[63:0]");
  F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_PROTOCOL =
      key_id_from_name(T_EGRESS_IP_MIRROR_ACL, "local_md.lkp.ip_proto");
  F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_TOS =
      key_id_from_name(T_EGRESS_IP_MIRROR_ACL, "local_md.lkp.ip_tos");
  F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_TCP_FLAGS =
      key_id_from_name(T_EGRESS_IP_MIRROR_ACL, "local_md.lkp.tcp_flags");
  F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_L4_SRC_PORT =
      key_id_from_name(T_EGRESS_IP_MIRROR_ACL, "local_md.lkp.l4_src_port");
  F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_L4_DST_PORT =
      key_id_from_name(T_EGRESS_IP_MIRROR_ACL, "local_md.lkp.l4_dst_port");
  F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_EGRESS_PORT_LAG_INDEX = key_id_from_name(
      T_EGRESS_IP_MIRROR_ACL, "local_md.egress_port_lag_index");
  F_EGRESS_IP_MIRROR_ACL_MATCH_PRIORITY =
      key_id_from_name(T_EGRESS_IP_MIRROR_ACL, "$MATCH_PRIORITY");
  F_EGRESS_IP_MIRROR_ACL_HDR_VLAN_TAG0_PCP =
      key_id_from_name(T_EGRESS_IP_MIRROR_ACL, "hdr.vlan_tag$0.pcp");
  F_EGRESS_IP_MIRROR_ACL_HDR_VLAN_TAG0_DEI =
      key_id_from_name(T_EGRESS_IP_MIRROR_ACL, "hdr.vlan_tag$0.dei");
  F_EGRESS_IP_MIRROR_ACL_HDR_VLAN_TAG0_VALID =
      key_id_from_name(T_EGRESS_IP_MIRROR_ACL, "hdr.vlan_tag$0.$valid");
  F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_FLAGS_ROUTED =
      key_id_from_name(T_EGRESS_IP_MIRROR_ACL, "local_md.flags.routed");
  A_EGRESS_IP_MIRROR_ACL_NO_ACTION = action_id_from_name(
      T_EGRESS_IP_MIRROR_ACL, "egress_ip_mirror_acl.no_action");
  P_EGRESS_IP_MIRROR_ACL_NO_ACTION_METER_INDEX = data_id_from_name(
      T_EGRESS_IP_MIRROR_ACL, A_EGRESS_IP_MIRROR_ACL_NO_ACTION, "meter_index");
  A_EGRESS_IP_MIRROR_ACL_MIRROR = action_id_from_name(
      T_EGRESS_IP_MIRROR_ACL, "egress_ip_mirror_acl.mirror_out");
  P_EGRESS_IP_MIRROR_ACL_MIRROR_METER_INDEX = data_id_from_name(
      T_EGRESS_IP_MIRROR_ACL, A_EGRESS_IP_MIRROR_ACL_MIRROR, "meter_index");
  P_EGRESS_IP_MIRROR_ACL_MIRROR_SESSION_ID = data_id_from_name(
      T_EGRESS_IP_MIRROR_ACL, A_EGRESS_IP_MIRROR_ACL_MIRROR, "session_id");
  D_EGRESS_IP_MIRROR_ACL_COUNTER_SPEC_BYTES =
      data_id_from_name(T_EGRESS_IP_MIRROR_ACL,
                        A_EGRESS_IP_MIRROR_ACL_NO_ACTION,
                        "$COUNTER_SPEC_BYTES");
  D_EGRESS_IP_MIRROR_ACL_COUNTER_SPEC_PKTS =
      data_id_from_name(T_EGRESS_IP_MIRROR_ACL,
                        A_EGRESS_IP_MIRROR_ACL_NO_ACTION,
                        "$COUNTER_SPEC_PKTS");
  D_EGRESS_IP_MIRROR_ACL_METER_SPEC_CIR_KBPS = data_id_from_name_noaction(
      T_EGRESS_IP_MIRROR_ACL, "$METER_SPEC_CIR_KBPS");
  D_EGRESS_IP_MIRROR_ACL_METER_SPEC_PIR_KBPS = data_id_from_name_noaction(
      T_EGRESS_IP_MIRROR_ACL, "$METER_SPEC_PIR_KBPS");
  D_EGRESS_IP_MIRROR_ACL_METER_SPEC_CBS_KBITS = data_id_from_name_noaction(
      T_EGRESS_IP_MIRROR_ACL, "$METER_SPEC_CBS_KBITS");
  D_EGRESS_IP_MIRROR_ACL_METER_SPEC_PBS_KBITS = data_id_from_name_noaction(
      T_EGRESS_IP_MIRROR_ACL, "$METER_SPEC_PBS_KBITS");

  T_EGRESS_IP_MIRROR_ACL_METER_ACTION =
      table_id_from_name("egress_ip_mirror_acl.meter_action");
  F_EGRESS_IP_MIRROR_ACL_METER_ACTION_COLOR = key_id_from_name(
      T_EGRESS_IP_MIRROR_ACL_METER_ACTION, "local_md.mirror.meter_color");
  F_EGRESS_IP_MIRROR_ACL_METER_ACTION_INDEX = key_id_from_name(
      T_EGRESS_IP_MIRROR_ACL_METER_ACTION, "local_md.mirror.meter_index");
  A_EGRESS_IP_MIRROR_ACL_METER_ACTION_COUNT = action_id_from_name(
      T_EGRESS_IP_MIRROR_ACL_METER_ACTION, "egress_ip_mirror_acl.count");
  D_EGRESS_IP_MIRROR_ACL_METER_ACTION_COUNTER_SPEC_BYTES =
      data_id_from_name(T_EGRESS_IP_MIRROR_ACL_METER_ACTION,
                        A_EGRESS_IP_MIRROR_ACL_METER_ACTION_COUNT,
                        "$COUNTER_SPEC_BYTES");

  T_EGRESS_IP_QOS_ACL = table_id_from_name("egress_ip_qos_acl.acl");
  F_EGRESS_IP_QOS_ACL_LOCAL_MD_BD =
      key_id_from_name(T_EGRESS_IP_QOS_ACL, "local_md.bd");
  F_EGRESS_IP_QOS_ACL_HDR_IP_SRC_ADDR_WORD3 =
      key_id_from_name(T_EGRESS_IP_QOS_ACL, "hdr.ipv6.src_addr[127:96]");
  F_EGRESS_IP_QOS_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD2 =
      key_id_from_name(T_EGRESS_IP_QOS_ACL, "local_md.lkp.ip_src_addr_95_64");
  F_EGRESS_IP_QOS_ACL_HDR_IPV6_SRC_ADDR_WORD10 =
      key_id_from_name(T_EGRESS_IP_QOS_ACL, "hdr.ipv6.src_addr[63:0]");
  F_EGRESS_IP_QOS_ACL_HDR_IP_DST_ADDR_WORD3 =
      key_id_from_name(T_EGRESS_IP_QOS_ACL, "hdr.ipv6.dst_addr[127:96]");
  F_EGRESS_IP_QOS_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD2 =
      key_id_from_name(T_EGRESS_IP_QOS_ACL, "local_md.lkp.ip_dst_addr_95_64");
  F_EGRESS_IP_QOS_ACL_HDR_IPV6_DST_ADDR_WORD10 =
      key_id_from_name(T_EGRESS_IP_QOS_ACL, "hdr.ipv6.dst_addr[63:0]");
  F_EGRESS_IP_QOS_ACL_LOCAL_MD_LKP_IP_PROTOCOL =
      key_id_from_name(T_EGRESS_IP_QOS_ACL, "local_md.lkp.ip_proto");
  F_EGRESS_IP_QOS_ACL_LOCAL_MD_LKP_IP_TOS =
      key_id_from_name(T_EGRESS_IP_QOS_ACL, "local_md.lkp.ip_tos");
  F_EGRESS_IP_QOS_ACL_LOCAL_MD_LKP_TCP_FLAGS =
      key_id_from_name(T_EGRESS_IP_QOS_ACL, "local_md.lkp.tcp_flags");
  F_EGRESS_IP_QOS_ACL_LOCAL_MD_LKP_L4_SRC_PORT =
      key_id_from_name(T_EGRESS_IP_QOS_ACL, "local_md.lkp.l4_src_port");
  F_EGRESS_IP_QOS_ACL_LOCAL_MD_LKP_L4_DST_PORT =
      key_id_from_name(T_EGRESS_IP_QOS_ACL, "local_md.lkp.l4_dst_port");
  F_EGRESS_IP_QOS_ACL_LOCAL_MD_EGRESS_PORT_LAG_INDEX =
      key_id_from_name(T_EGRESS_IP_QOS_ACL, "local_md.egress_port_lag_index");
  F_EGRESS_IP_QOS_ACL_MATCH_PRIORITY =
      key_id_from_name(T_EGRESS_IP_QOS_ACL, "$MATCH_PRIORITY");
  F_EGRESS_IP_QOS_ACL_HDR_VLAN_TAG0_PCP =
      key_id_from_name(T_EGRESS_IP_QOS_ACL, "hdr.vlan_tag$0.pcp");
  F_EGRESS_IP_QOS_ACL_HDR_VLAN_TAG0_DEI =
      key_id_from_name(T_EGRESS_IP_QOS_ACL, "hdr.vlan_tag$0.dei");
  F_EGRESS_IP_QOS_ACL_HDR_VLAN_TAG0_VALID =
      key_id_from_name(T_EGRESS_IP_QOS_ACL, "hdr.vlan_tag$0.$valid");
  F_EGRESS_IP_QOS_ACL_LOCAL_MD_FLAGS_ROUTED =
      key_id_from_name(T_EGRESS_IP_QOS_ACL, "local_md.flags.routed");
  A_EGRESS_IP_QOS_ACL_NO_ACTION =
      action_id_from_name(T_EGRESS_IP_QOS_ACL, "egress_ip_qos_acl.no_action");
  P_EGRESS_IP_QOS_ACL_NO_ACTION_METER_INDEX = data_id_from_name(
      T_EGRESS_IP_QOS_ACL, A_EGRESS_IP_QOS_ACL_NO_ACTION, "meter_index");
  A_EGRESS_IP_QOS_ACL_SET_PCP =
      action_id_from_name(T_EGRESS_IP_QOS_ACL, "egress_ip_qos_acl.set_pcp");
  P_EGRESS_IP_QOS_ACL_SET_PCP_PCP = data_id_from_name(
      T_EGRESS_IP_QOS_ACL, A_EGRESS_IP_QOS_ACL_SET_PCP, "pcp");
  P_EGRESS_IP_QOS_ACL_SET_PCP_METER_INDEX = data_id_from_name(
      T_EGRESS_IP_QOS_ACL, A_EGRESS_IP_QOS_ACL_SET_PCP, "meter_index");
  A_EGRESS_IP_QOS_ACL_SET_IPV4_TOS = action_id_from_name(
      T_EGRESS_IP_QOS_ACL, "egress_ip_qos_acl.set_ipv4_tos");
  P_EGRESS_IP_QOS_ACL_SET_IPV4_TOS_TOS = data_id_from_name(
      T_EGRESS_IP_QOS_ACL, A_EGRESS_IP_QOS_ACL_SET_IPV4_TOS, "tos");
  P_EGRESS_IP_QOS_ACL_SET_IPV4_TOS_METER_INDEX = data_id_from_name(
      T_EGRESS_IP_QOS_ACL, A_EGRESS_IP_QOS_ACL_SET_IPV4_TOS, "meter_index");
  A_EGRESS_IP_QOS_ACL_SET_IPV6_TOS = action_id_from_name(
      T_EGRESS_IP_QOS_ACL, "egress_ip_qos_acl.set_ipv6_tos");
  P_EGRESS_IP_QOS_ACL_SET_IPV6_TOS_TOS = data_id_from_name(
      T_EGRESS_IP_QOS_ACL, A_EGRESS_IP_QOS_ACL_SET_IPV6_TOS, "tos");
  P_EGRESS_IP_QOS_ACL_SET_IPV6_TOS_METER_INDEX = data_id_from_name(
      T_EGRESS_IP_QOS_ACL, A_EGRESS_IP_QOS_ACL_SET_IPV6_TOS, "meter_index");
  A_EGRESS_IP_QOS_ACL_SET_PCP_IPV4_TOS = action_id_from_name(
      T_EGRESS_IP_QOS_ACL, "egress_ip_qos_acl.set_pcp_and_ipv4_tos");
  P_EGRESS_IP_QOS_ACL_SET_PCP_IPV4_TOS_TOS = data_id_from_name(
      T_EGRESS_IP_QOS_ACL, A_EGRESS_IP_QOS_ACL_SET_PCP_IPV4_TOS, "tos");
  P_EGRESS_IP_QOS_ACL_SET_PCP_IPV4_TOS_PCP = data_id_from_name(
      T_EGRESS_IP_QOS_ACL, A_EGRESS_IP_QOS_ACL_SET_PCP_IPV4_TOS, "pcp");
  P_EGRESS_IP_QOS_ACL_SET_PCP_IPV4_TOS_METER_INDEX = data_id_from_name(
      T_EGRESS_IP_QOS_ACL, A_EGRESS_IP_QOS_ACL_SET_PCP_IPV4_TOS, "meter_index");
  A_EGRESS_IP_QOS_ACL_SET_PCP_IPV6_TOS = action_id_from_name(
      T_EGRESS_IP_QOS_ACL, "egress_ip_qos_acl.set_pcp_and_ipv6_tos");
  P_EGRESS_IP_QOS_ACL_SET_PCP_IPV6_TOS_TOS = data_id_from_name(
      T_EGRESS_IP_QOS_ACL, A_EGRESS_IP_QOS_ACL_SET_PCP_IPV6_TOS, "tos");
  P_EGRESS_IP_QOS_ACL_SET_PCP_IPV6_TOS_PCP = data_id_from_name(
      T_EGRESS_IP_QOS_ACL, A_EGRESS_IP_QOS_ACL_SET_PCP_IPV6_TOS, "pcp");
  P_EGRESS_IP_QOS_ACL_SET_PCP_IPV6_TOS_METER_INDEX = data_id_from_name(
      T_EGRESS_IP_QOS_ACL, A_EGRESS_IP_QOS_ACL_SET_PCP_IPV6_TOS, "meter_index");
  A_EGRESS_IP_QOS_ACL_SET_METER =
      action_id_from_name(T_EGRESS_IP_QOS_ACL, "egress_ip_qos_acl.set_meter");
  P_EGRESS_IP_QOS_ACL_SET_METER_INDEX = data_id_from_name(
      T_EGRESS_IP_QOS_ACL, A_EGRESS_IP_QOS_ACL_SET_METER, "meter_index");
  A_EGRESS_IP_QOS_ACL_SET_QOS_PARAMS = action_id_from_name(
      T_EGRESS_IP_QOS_ACL, "egress_ip_qos_acl.set_qos_params");
  P_EGRESS_IP_QOS_ACL_SET_QOS_PARAMS_PCP = data_id_from_name(
      T_EGRESS_IP_QOS_ACL, A_EGRESS_IP_QOS_ACL_SET_QOS_PARAMS, "pcp");
  P_EGRESS_IP_QOS_ACL_SET_QOS_PARAMS_TOS = data_id_from_name(
      T_EGRESS_IP_QOS_ACL, A_EGRESS_IP_QOS_ACL_SET_QOS_PARAMS, "tos");
  P_EGRESS_IP_QOS_ACL_SET_QOS_PARAMS_METER_INDEX = data_id_from_name(
      T_EGRESS_IP_QOS_ACL, A_EGRESS_IP_QOS_ACL_SET_QOS_PARAMS, "meter_index");
  D_EGRESS_IP_QOS_ACL_COUNTER_SPEC_BYTES =
      data_id_from_name(T_EGRESS_IP_QOS_ACL,
                        A_EGRESS_IP_QOS_ACL_NO_ACTION,
                        "$COUNTER_SPEC_BYTES");
  D_EGRESS_IP_QOS_ACL_COUNTER_SPEC_PKTS = data_id_from_name(
      T_EGRESS_IP_QOS_ACL, A_EGRESS_IP_QOS_ACL_NO_ACTION, "$COUNTER_SPEC_PKTS");
  D_EGRESS_IP_QOS_ACL_METER_SPEC_CIR_KBPS =
      data_id_from_name_noaction(T_EGRESS_IP_QOS_ACL, "$METER_SPEC_CIR_KBPS");
  D_EGRESS_IP_QOS_ACL_METER_SPEC_PIR_KBPS =
      data_id_from_name_noaction(T_EGRESS_IP_QOS_ACL, "$METER_SPEC_PIR_KBPS");
  D_EGRESS_IP_QOS_ACL_METER_SPEC_CBS_KBITS =
      data_id_from_name_noaction(T_EGRESS_IP_QOS_ACL, "$METER_SPEC_CBS_KBITS");
  D_EGRESS_IP_QOS_ACL_METER_SPEC_PBS_KBITS =
      data_id_from_name_noaction(T_EGRESS_IP_QOS_ACL, "$METER_SPEC_PBS_KBITS");

  T_EGRESS_IP_QOS_ACL_METER_ACTION =
      table_id_from_name("egress_ip_qos_acl.meter_action");
  F_EGRESS_IP_QOS_ACL_METER_ACTION_COLOR = key_id_from_name(
      T_EGRESS_IP_QOS_ACL_METER_ACTION, "local_md.qos.acl_meter_color");
  F_EGRESS_IP_QOS_ACL_METER_ACTION_INDEX = key_id_from_name(
      T_EGRESS_IP_QOS_ACL_METER_ACTION, "local_md.qos.acl_meter_index");
  A_EGRESS_IP_QOS_ACL_METER_ACTION_COUNT = action_id_from_name(
      T_EGRESS_IP_QOS_ACL_METER_ACTION, "egress_ip_qos_acl.count");
  D_EGRESS_IP_QOS_ACL_METER_ACTION_COUNTER_SPEC_BYTES =
      data_id_from_name(T_EGRESS_IP_QOS_ACL_METER_ACTION,
                        A_EGRESS_IP_QOS_ACL_METER_ACTION_COUNT,
                        "$COUNTER_SPEC_BYTES");

  T_EGRESS_ACL_QOS_MACADDR = table_id_from_name("egress_qos_acl_mac.mac");
  F_EGRESS_ACL_QOS_MACADDR_PORT_LAG_INDEX =
      key_id_from_name(T_EGRESS_ACL_QOS_MACADDR, "port_lag_index");
  F_EGRESS_ACL_QOS_MACADDR_SMAC_ADDR =
      key_id_from_name(T_EGRESS_ACL_QOS_MACADDR, "smac_addr");
  F_EGRESS_ACL_QOS_MACADDR_DMAC_ADDR =
      key_id_from_name(T_EGRESS_ACL_QOS_MACADDR, "dmac_addr");
  A_EGRESS_ACL_QOS_MACADDR_SET_MACADDR_LABEL =
      action_id_from_name(T_EGRESS_ACL_QOS_MACADDR, "set_mac_addr_label");
  D_EGRESS_ACL_QOS_MACADDR_SET_MACADDR_LABEL =
      data_id_from_name(T_EGRESS_ACL_QOS_MACADDR,
                        A_EGRESS_ACL_QOS_MACADDR_SET_MACADDR_LABEL,
                        "label");
  F_EGRESS_ACL_QOS_MACADDR_MATCH_PRIORITY =
      key_id_from_name(T_EGRESS_ACL_QOS_MACADDR, "$MATCH_PRIORITY");

  T_EGRESS_ACL_MIRROR_MACADDR = table_id_from_name("egress_mirror_acl_mac.mac");
  F_EGRESS_ACL_MIRROR_MACADDR_PORT_LAG_INDEX =
      key_id_from_name(T_EGRESS_ACL_MIRROR_MACADDR, "port_lag_index");
  F_EGRESS_ACL_MIRROR_MACADDR_SMAC_ADDR =
      key_id_from_name(T_EGRESS_ACL_MIRROR_MACADDR, "smac_addr");
  F_EGRESS_ACL_MIRROR_MACADDR_DMAC_ADDR =
      key_id_from_name(T_EGRESS_ACL_MIRROR_MACADDR, "dmac_addr");
  A_EGRESS_ACL_MIRROR_MACADDR_SET_MACADDR_LABEL =
      action_id_from_name(T_EGRESS_ACL_MIRROR_MACADDR, "set_mac_addr_label");
  D_EGRESS_ACL_MIRROR_MACADDR_SET_MACADDR_LABEL =
      data_id_from_name(T_EGRESS_ACL_MIRROR_MACADDR,
                        A_EGRESS_ACL_MIRROR_MACADDR_SET_MACADDR_LABEL,
                        "label");
  F_EGRESS_ACL_MIRROR_MACADDR_MATCH_PRIORITY =
      key_id_from_name(T_EGRESS_ACL_MIRROR_MACADDR, "$MATCH_PRIORITY");

  //  acl2.p4 egress complete

  /* multicast.p4 */
  T_IPV4_MULTICAST_BRIDGE_S_G = table_id_from_name("multicast_bridge_ipv4_s_g");
  F_IPV4_MULTICAST_BRIDGE_S_G_BD =
      key_id_from_name(T_IPV4_MULTICAST_BRIDGE_S_G, "bd");
  F_IPV4_MULTICAST_BRIDGE_S_G_SRC_ADDR =
      key_id_from_name(T_IPV4_MULTICAST_BRIDGE_S_G, "src_addr");
  F_IPV4_MULTICAST_BRIDGE_S_G_GRP_ADDR =
      key_id_from_name(T_IPV4_MULTICAST_BRIDGE_S_G, "grp_addr");
  A_IPV4_MULTICAST_BRIDGE_S_G_HIT = action_id_from_name(
      T_IPV4_MULTICAST_BRIDGE_S_G, "multicast_bridge_s_g_hit");
  P_IPV4_MULTICAST_BRIDGE_S_G_HIT_MGID = data_id_from_name(
      T_IPV4_MULTICAST_BRIDGE_S_G, A_IPV4_MULTICAST_BRIDGE_S_G_HIT, "mgid");

  T_IPV4_MULTICAST_BRIDGE_X_G =
      table_id_from_name("multicast_bridge_ipv4_star_g");
  F_IPV4_MULTICAST_BRIDGE_X_G_BD =
      key_id_from_name(T_IPV4_MULTICAST_BRIDGE_X_G, "bd");
  F_IPV4_MULTICAST_BRIDGE_X_G_GRP_ADDR =
      key_id_from_name(T_IPV4_MULTICAST_BRIDGE_X_G, "grp_addr");
  A_IPV4_MULTICAST_BRIDGE_X_G_HIT = action_id_from_name(
      T_IPV4_MULTICAST_BRIDGE_X_G, "multicast_bridge_star_g_hit");
  P_IPV4_MULTICAST_BRIDGE_X_G_HIT_MGID = data_id_from_name(
      T_IPV4_MULTICAST_BRIDGE_X_G, A_IPV4_MULTICAST_BRIDGE_X_G_HIT, "mgid");

  T_IPV4_MULTICAST_ROUTE_S_G = table_id_from_name("multicast_route_ipv4_s_g");
  F_IPV4_MULTICAST_ROUTE_S_G_VRF =
      key_id_from_name(T_IPV4_MULTICAST_ROUTE_S_G, "vrf");
  F_IPV4_MULTICAST_ROUTE_S_G_SRC_ADDR =
      key_id_from_name(T_IPV4_MULTICAST_ROUTE_S_G, "src_addr");
  F_IPV4_MULTICAST_ROUTE_S_G_GRP_ADDR =
      key_id_from_name(T_IPV4_MULTICAST_ROUTE_S_G, "grp_addr");
  A_IPV4_MULTICAST_ROUTE_S_G_HIT = action_id_from_name(
      T_IPV4_MULTICAST_ROUTE_S_G, "multicast_route_s_g_hit");
  P_IPV4_MULTICAST_ROUTE_S_G_HIT_MGID = data_id_from_name(
      T_IPV4_MULTICAST_ROUTE_S_G, A_IPV4_MULTICAST_ROUTE_S_G_HIT, "mgid");
  P_IPV4_MULTICAST_ROUTE_S_G_HIT_RPF_GROUP = data_id_from_name(
      T_IPV4_MULTICAST_ROUTE_S_G, A_IPV4_MULTICAST_ROUTE_S_G_HIT, "rpf_group");
  P_IPV4_MULTICAST_ROUTE_S_G_HIT_PKTS =
      data_id_from_name(T_IPV4_MULTICAST_ROUTE_S_G,
                        A_IPV4_MULTICAST_ROUTE_S_G_HIT,
                        "$COUNTER_SPEC_PKTS");

  T_IPV4_MULTICAST_ROUTE_X_G =
      table_id_from_name("multicast_route_ipv4_star_g");
  F_IPV4_MULTICAST_ROUTE_X_G_VRF =
      key_id_from_name(T_IPV4_MULTICAST_ROUTE_X_G, "vrf");
  F_IPV4_MULTICAST_ROUTE_X_G_GRP_ADDR =
      key_id_from_name(T_IPV4_MULTICAST_ROUTE_X_G, "grp_addr");
  A_IPV4_MULTICAST_ROUTE_X_G_HIT_SM = action_id_from_name(
      T_IPV4_MULTICAST_ROUTE_X_G, "multicast_route_star_g_hit_sm");
  P_IPV4_MULTICAST_ROUTE_X_G_HIT_SM_MGID = data_id_from_name(
      T_IPV4_MULTICAST_ROUTE_X_G, A_IPV4_MULTICAST_ROUTE_X_G_HIT_SM, "mgid");
  P_IPV4_MULTICAST_ROUTE_X_G_HIT_SM_RPF_GROUP =
      data_id_from_name(T_IPV4_MULTICAST_ROUTE_X_G,
                        A_IPV4_MULTICAST_ROUTE_X_G_HIT_SM,
                        "rpf_group");
  A_IPV4_MULTICAST_ROUTE_X_G_HIT_BIDIR = action_id_from_name(
      T_IPV4_MULTICAST_ROUTE_X_G, "multicast_route_star_g_hit_bidir");
  P_IPV4_MULTICAST_ROUTE_X_G_HIT_BIDIR_MGID = data_id_from_name(
      T_IPV4_MULTICAST_ROUTE_X_G, A_IPV4_MULTICAST_ROUTE_X_G_HIT_BIDIR, "mgid");
  P_IPV4_MULTICAST_ROUTE_X_G_HIT_BIDIR_RPF_GROUP =
      data_id_from_name(T_IPV4_MULTICAST_ROUTE_X_G,
                        A_IPV4_MULTICAST_ROUTE_X_G_HIT_BIDIR,
                        "rpf_group");
  P_IPV4_MULTICAST_ROUTE_X_G_HIT_SM_PKTS =
      data_id_from_name(T_IPV4_MULTICAST_ROUTE_X_G,
                        A_IPV4_MULTICAST_ROUTE_X_G_HIT_SM,
                        "$COUNTER_SPEC_PKTS");

  T_IPV6_MULTICAST_BRIDGE_S_G = table_id_from_name("multicast_bridge_ipv6_s_g");
  F_IPV6_MULTICAST_BRIDGE_S_G_BD =
      key_id_from_name(T_IPV6_MULTICAST_BRIDGE_S_G, "bd");
  F_IPV6_MULTICAST_BRIDGE_S_G_GRP_ADDR =
      key_id_from_name(T_IPV6_MULTICAST_BRIDGE_S_G, "grp_addr");
  F_IPV6_MULTICAST_BRIDGE_S_G_SRC_ADDR =
      key_id_from_name(T_IPV6_MULTICAST_BRIDGE_S_G, "src_addr");
  A_IPV6_MULTICAST_BRIDGE_S_G_HIT = action_id_from_name(
      T_IPV6_MULTICAST_BRIDGE_S_G, "multicast_bridge_ipv6_s_g_hit");
  P_IPV6_MULTICAST_BRIDGE_S_G_HIT_MGID = data_id_from_name(
      T_IPV6_MULTICAST_BRIDGE_S_G, A_IPV6_MULTICAST_BRIDGE_S_G_HIT, "mgid");

  T_IPV6_MULTICAST_BRIDGE_X_G =
      table_id_from_name("multicast_bridge_ipv6_star_g");
  F_IPV6_MULTICAST_BRIDGE_X_G_BD =
      key_id_from_name(T_IPV6_MULTICAST_BRIDGE_X_G, "bd");
  F_IPV6_MULTICAST_BRIDGE_X_G_GRP_ADDR =
      key_id_from_name(T_IPV6_MULTICAST_BRIDGE_X_G, "grp_addr");
  A_IPV6_MULTICAST_BRIDGE_X_G_HIT = action_id_from_name(
      T_IPV6_MULTICAST_BRIDGE_X_G, "multicast_bridge_ipv6_star_g_hit");
  P_IPV6_MULTICAST_BRIDGE_X_G_HIT_MGID = data_id_from_name(
      T_IPV6_MULTICAST_BRIDGE_X_G, A_IPV6_MULTICAST_BRIDGE_X_G_HIT, "mgid");

  T_IPV6_MULTICAST_ROUTE_S_G = table_id_from_name("multicast_route_ipv6_s_g");
  F_IPV6_MULTICAST_ROUTE_S_G_VRF =
      key_id_from_name(T_IPV6_MULTICAST_ROUTE_S_G, "vrf");
  F_IPV6_MULTICAST_ROUTE_S_G_GRP_ADDR =
      key_id_from_name(T_IPV6_MULTICAST_ROUTE_S_G, "grp_addr");
  F_IPV6_MULTICAST_ROUTE_S_G_SRC_ADDR =
      key_id_from_name(T_IPV6_MULTICAST_ROUTE_S_G, "src_addr");
  A_IPV6_MULTICAST_ROUTE_S_G_HIT = action_id_from_name(
      T_IPV6_MULTICAST_ROUTE_S_G, "multicast_route_ipv6_s_g_hit");
  P_IPV6_MULTICAST_ROUTE_S_G_HIT_MGID = data_id_from_name(
      T_IPV6_MULTICAST_ROUTE_S_G, A_IPV6_MULTICAST_ROUTE_S_G_HIT, "mgid");
  P_IPV6_MULTICAST_ROUTE_S_G_HIT_RPF_GROUP = data_id_from_name(
      T_IPV6_MULTICAST_ROUTE_S_G, A_IPV6_MULTICAST_ROUTE_S_G_HIT, "rpf_group");
  P_IPV6_MULTICAST_ROUTE_S_G_HIT_PKTS =
      data_id_from_name(T_IPV6_MULTICAST_ROUTE_S_G,
                        A_IPV6_MULTICAST_ROUTE_S_G_HIT,
                        "$COUNTER_SPEC_PKTS");

  T_IPV6_MULTICAST_ROUTE_X_G =
      table_id_from_name("multicast_route_ipv6_star_g");
  F_IPV6_MULTICAST_ROUTE_X_G_VRF =
      key_id_from_name(T_IPV6_MULTICAST_ROUTE_X_G, "vrf");
  F_IPV6_MULTICAST_ROUTE_X_G_GRP_ADDR =
      key_id_from_name(T_IPV6_MULTICAST_ROUTE_X_G, "grp_addr");
  A_IPV6_MULTICAST_ROUTE_X_G_HIT_BIDIR = action_id_from_name(
      T_IPV6_MULTICAST_ROUTE_X_G, "multicast_route_ipv6_star_g_hit_bidir");
  P_IPV6_MULTICAST_ROUTE_X_G_HIT_BIDIR_MGID = data_id_from_name(
      T_IPV6_MULTICAST_ROUTE_X_G, A_IPV6_MULTICAST_ROUTE_X_G_HIT_BIDIR, "mgid");
  P_IPV6_MULTICAST_ROUTE_X_G_HIT_BIDIR_RPF_GROUP =
      data_id_from_name(T_IPV6_MULTICAST_ROUTE_X_G,
                        A_IPV6_MULTICAST_ROUTE_X_G_HIT_BIDIR,
                        "rpf_group");
  P_IPV6_MULTICAST_ROUTE_X_G_HIT_BIDIR_PKTS =
      data_id_from_name(T_IPV6_MULTICAST_ROUTE_X_G,
                        A_IPV6_MULTICAST_ROUTE_X_G_HIT_BIDIR,
                        "$COUNTER_SPEC_PKTS");
  A_IPV6_MULTICAST_ROUTE_X_G_HIT_SM = action_id_from_name(
      T_IPV6_MULTICAST_ROUTE_X_G, "multicast_route_ipv6_star_g_hit_sm");
  P_IPV6_MULTICAST_ROUTE_X_G_HIT_SM_MGID = data_id_from_name(
      T_IPV6_MULTICAST_ROUTE_X_G, A_IPV6_MULTICAST_ROUTE_X_G_HIT_SM, "mgid");
  P_IPV6_MULTICAST_ROUTE_X_G_HIT_SM_RPF_GROUP =
      data_id_from_name(T_IPV6_MULTICAST_ROUTE_X_G,
                        A_IPV6_MULTICAST_ROUTE_X_G_HIT_SM,
                        "rpf_group");
  P_IPV6_MULTICAST_ROUTE_X_G_HIT_SM_PKTS =
      data_id_from_name(T_IPV6_MULTICAST_ROUTE_X_G,
                        A_IPV6_MULTICAST_ROUTE_X_G_HIT_SM,
                        "$COUNTER_SPEC_PKTS");

  T_MCAST_FWD_RESULT = table_id_from_name("multicast_fwd_result");
  F_MCAST_FWD_RESULT_MULTICAST_HIT =
      key_id_from_name(T_MCAST_FWD_RESULT, "local_md.multicast.hit");
  F_MCAST_FWD_RESULT_LKP_IP_TYPE =
      key_id_from_name(T_MCAST_FWD_RESULT, "lkp.ip_type");
  F_MCAST_FWD_RESULT_IPV4_MCAST_SNOOPING =
      key_id_from_name(T_MCAST_FWD_RESULT, "local_md.ipv4.multicast_snooping");
  F_MCAST_FWD_RESULT_IPV6_MCAST_SNOOPING =
      key_id_from_name(T_MCAST_FWD_RESULT, "local_md.ipv6.multicast_snooping");
  F_MCAST_FWD_RESULT_LOCAL_MD_MULTICAST_MODE =
      key_id_from_name(T_MCAST_FWD_RESULT, "local_md.multicast.mode");
  F_MCAST_FWD_RESULT_RPF_CHECK =
      key_id_from_name(T_MCAST_FWD_RESULT, "rpf_check");
  F_MCAST_FWD_RESULT_PRIORITY =
      key_id_from_name(T_MCAST_FWD_RESULT, "$MATCH_PRIORITY");
  A_MCAST_FWD_RESULT_SET_MULTICAST_BRIDGE =
      action_id_from_name(T_MCAST_FWD_RESULT, "set_multicast_bridge");
  P_MCAST_FWD_RESULT_SET_MULTICAST_BRIDGE_MRPF = data_id_from_name(
      T_MCAST_FWD_RESULT, A_MCAST_FWD_RESULT_SET_MULTICAST_BRIDGE, "mrpf");
  A_MCAST_FWD_RESULT_SET_MULTICAST_ROUTE =
      action_id_from_name(T_MCAST_FWD_RESULT, "set_multicast_route");
  A_MCAST_FWD_RESULT_SET_MULTICAST_FLOOD =
      action_id_from_name(T_MCAST_FWD_RESULT, "set_multicast_flood");
  P_MCAST_FWD_RESULT_SET_MULTICAST_FLOOD_MRPF = data_id_from_name(
      T_MCAST_FWD_RESULT, A_MCAST_FWD_RESULT_SET_MULTICAST_FLOOD, "mrpf");
  P_MCAST_FWD_RESULT_SET_MULTICAST_FLOOD_FLOOD = data_id_from_name(
      T_MCAST_FWD_RESULT, A_MCAST_FWD_RESULT_SET_MULTICAST_FLOOD, "flood");

  T_BD_FLOOD = table_id_from_name("bd_flood");
  F_BD_FLOOD_BD = key_id_from_name(T_BD_FLOOD, "bd");
  F_BD_FLOOD_PKT_TYPE = key_id_from_name(T_BD_FLOOD, "pkt_type");
  F_BD_FLOOD_FLOOD_TO_MROUTERS =
      key_id_from_name(T_BD_FLOOD, "flood_to_multicast_routers");
  A_FLOOD = action_id_from_name(T_BD_FLOOD, "mcast_flood");
  P_FLOOD_MGID = data_id_from_name(T_BD_FLOOD, A_FLOOD, "mgid");

  T_RID = table_id_from_name("rid");
  F_RID_REPLICATION_ID = key_id_from_name(T_RID, "eg_intr_md.egress_rid");
  A_RID_HIT = action_id_from_name(T_RID, "mc_rid_hit");
  P_RID_HIT_BD = data_id_from_name(T_RID, A_RID_HIT, "bd");
  A_RID_TUNNEL_RID_HIT = action_id_from_name(T_RID, "tunnel_rid_hit");
  P_RID_TUNNEL_RID_HIT_NEXTHOP =
      data_id_from_name(T_RID, A_RID_TUNNEL_RID_HIT, "nexthop");
  P_RID_TUNNEL_RID_HIT_TUNNEL_NEXTHOP =
      data_id_from_name(T_RID, A_RID_TUNNEL_RID_HIT, "tunnel_nexthop");
  A_RID_TUNNEL_MC_RID_HIT = action_id_from_name(T_RID, "tunnel_mc_rid_hit");
  P_RID_TUNNEL_MC_RID_HIT_BD =
      data_id_from_name(T_RID, A_RID_TUNNEL_MC_RID_HIT, "bd");
  P_RID_TUNNEL_MC_RID_HIT_NEXTHOP =
      data_id_from_name(T_RID, A_RID_TUNNEL_MC_RID_HIT, "nexthop");
  P_RID_TUNNEL_MC_RID_HIT_TUNNEL_NEXTHOP =
      data_id_from_name(T_RID, A_RID_TUNNEL_MC_RID_HIT, "tunnel_nexthop");

  T_DECAP_ECN = table_id_from_name("SwitchEgress.tunnel_decap.decap_ipv4_ecn");
  F_MODE_ECN = key_id_from_name(T_DECAP_ECN, "local_md.tunnel.ecn_mode");

  /* rewrite.p4 */
  T_MIRROR_REWRITE = table_id_from_name("mirror_rewrite");
  F_MIRROR_REWRITE_MIRROR_MD_SESSION_ID =
      key_id_from_name(T_MIRROR_REWRITE, "local_md.mirror.session_id");
  A_REWRITE_ERSPAN_TYPE2 =
      action_id_from_name(T_MIRROR_REWRITE, "rewrite_erspan_type2");
  P_REWRITE_ERSPAN_TYPE2_QUEUE_ID =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE2, "qid");
  P_REWRITE_ERSPAN_TYPE2_SMAC =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE2, "smac");
  P_REWRITE_ERSPAN_TYPE2_DMAC =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE2, "dmac");
  P_REWRITE_ERSPAN_TYPE2_SIP =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE2, "sip");
  P_REWRITE_ERSPAN_TYPE2_DIP =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE2, "dip");
  P_REWRITE_ERSPAN_TYPE2_TOS =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE2, "tos");
  P_REWRITE_ERSPAN_TYPE2_TTL =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE2, "ttl");
  A_REWRITE_ERSPAN_TYPE2_VLAN =
      action_id_from_name(T_MIRROR_REWRITE, "rewrite_erspan_type2_with_vlan");
  P_REWRITE_ERSPAN_TYPE2_VLAN_ETHER_TYPE = data_id_from_name(
      T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE2_VLAN, "ether_type");
  P_REWRITE_ERSPAN_TYPE2_VLAN_PCP =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE2_VLAN, "pcp");
  P_REWRITE_ERSPAN_TYPE2_VLAN_VID =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE2_VLAN, "vid");
  P_REWRITE_ERSPAN_TYPE2_VLAN_QUEUE_ID =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE2_VLAN, "qid");
  P_REWRITE_ERSPAN_TYPE2_VLAN_SMAC =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE2_VLAN, "smac");
  P_REWRITE_ERSPAN_TYPE2_VLAN_DMAC =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE2_VLAN, "dmac");
  P_REWRITE_ERSPAN_TYPE2_VLAN_SIP =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE2_VLAN, "sip");
  P_REWRITE_ERSPAN_TYPE2_VLAN_DIP =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE2_VLAN, "dip");
  P_REWRITE_ERSPAN_TYPE2_VLAN_TOS =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE2_VLAN, "tos");
  P_REWRITE_ERSPAN_TYPE2_VLAN_TTL =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE2_VLAN, "ttl");
  A_REWRITE_ERSPAN_TYPE3 =
      action_id_from_name(T_MIRROR_REWRITE, "rewrite_erspan_type3");
  P_REWRITE_ERSPAN_TYPE3_QUEUE_ID =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE3, "qid");
  P_REWRITE_ERSPAN_TYPE3_SMAC =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE3, "smac");
  P_REWRITE_ERSPAN_TYPE3_DMAC =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE3, "dmac");
  P_REWRITE_ERSPAN_TYPE3_SIP =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE3, "sip");
  P_REWRITE_ERSPAN_TYPE3_DIP =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE3, "dip");
  P_REWRITE_ERSPAN_TYPE3_TOS =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE3, "tos");
  P_REWRITE_ERSPAN_TYPE3_TTL =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE3, "ttl");
  A_REWRITE_ERSPAN_TYPE3_VLAN =
      action_id_from_name(T_MIRROR_REWRITE, "rewrite_erspan_type3_with_vlan");
  P_REWRITE_ERSPAN_TYPE3_VLAN_ETHER_TYPE = data_id_from_name(
      T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE3_VLAN, "ether_type");
  P_REWRITE_ERSPAN_TYPE3_VLAN_PCP =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE3_VLAN, "pcp");
  P_REWRITE_ERSPAN_TYPE3_VLAN_VID =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE3_VLAN, "vid");
  P_REWRITE_ERSPAN_TYPE3_VLAN_QUEUE_ID =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE3_VLAN, "qid");
  P_REWRITE_ERSPAN_TYPE3_VLAN_SMAC =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE3_VLAN, "smac");
  P_REWRITE_ERSPAN_TYPE3_VLAN_DMAC =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE3_VLAN, "dmac");
  P_REWRITE_ERSPAN_TYPE3_VLAN_SIP =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE3_VLAN, "sip");
  P_REWRITE_ERSPAN_TYPE3_VLAN_DIP =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE3_VLAN, "dip");
  P_REWRITE_ERSPAN_TYPE3_VLAN_TOS =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE3_VLAN, "tos");
  P_REWRITE_ERSPAN_TYPE3_VLAN_TTL =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE3_VLAN, "ttl");
  A_REWRITE_ERSPAN_TYPE3_PLAT = action_id_from_name(
      T_MIRROR_REWRITE, "rewrite_erspan_type3_platform_specific");
  P_REWRITE_ERSPAN_TYPE3_PLAT_QUEUE_ID =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE3_PLAT, "qid");
  P_REWRITE_ERSPAN_TYPE3_PLAT_SMAC =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE3_PLAT, "smac");
  P_REWRITE_ERSPAN_TYPE3_PLAT_DMAC =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE3_PLAT, "dmac");
  P_REWRITE_ERSPAN_TYPE3_PLAT_SIP =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE3_PLAT, "sip");
  P_REWRITE_ERSPAN_TYPE3_PLAT_DIP =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE3_PLAT, "dip");
  P_REWRITE_ERSPAN_TYPE3_PLAT_TOS =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE3_PLAT, "tos");
  P_REWRITE_ERSPAN_TYPE3_PLAT_TTL =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE3_PLAT, "ttl");
  A_REWRITE_ERSPAN_TYPE3_PLAT_VLAN = action_id_from_name(
      T_MIRROR_REWRITE, "rewrite_erspan_type3_platform_specific_with_vlan");
  P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_ETHER_TYPE = data_id_from_name(
      T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE3_PLAT_VLAN, "ether_type");
  P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_PCP = data_id_from_name(
      T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE3_PLAT_VLAN, "pcp");
  P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_VID = data_id_from_name(
      T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE3_PLAT_VLAN, "vid");
  P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_QUEUE_ID = data_id_from_name(
      T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE3_PLAT_VLAN, "qid");
  P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_SMAC = data_id_from_name(
      T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE3_PLAT_VLAN, "smac");
  P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_DMAC = data_id_from_name(
      T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE3_PLAT_VLAN, "dmac");
  P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_SIP = data_id_from_name(
      T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE3_PLAT_VLAN, "sip");
  P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_DIP = data_id_from_name(
      T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE3_PLAT_VLAN, "dip");
  P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_TOS = data_id_from_name(
      T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE3_PLAT_VLAN, "tos");
  P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_TTL = data_id_from_name(
      T_MIRROR_REWRITE, A_REWRITE_ERSPAN_TYPE3_PLAT_VLAN, "ttl");
  /* mirror rspan rewrite. */
  A_REWRITE_RSPAN = action_id_from_name(T_MIRROR_REWRITE, "rewrite_rspan");
  P_REWRITE_RSPAN_QUEUE_ID =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_RSPAN, "qid");
  P_REWRITE_RSPAN_VID =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_RSPAN, "vid");
  /* mirror DUMMY rewrite. */
  A_REWRITE_DUMMY = action_id_from_name(T_MIRROR_REWRITE, "rewrite_");
  P_REWRITE_DUMMY_QUEUE_ID =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_DUMMY, "qid");

  /* mirror dtel rewrite */
  A_REWRITE_DTEL_REPORT = action_id_from_name(
      T_MIRROR_REWRITE, "rewrite_dtel_report_without_entropy");
  P_REWRITE_DTEL_REPORT_SMAC =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_DTEL_REPORT, "smac");
  P_REWRITE_DTEL_REPORT_DMAC =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_DTEL_REPORT, "dmac");
  P_REWRITE_DTEL_REPORT_SIP =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_DTEL_REPORT, "sip");
  P_REWRITE_DTEL_REPORT_DIP =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_DTEL_REPORT, "dip");
  P_REWRITE_DTEL_REPORT_UDP_DST_PORT = data_id_from_name(
      T_MIRROR_REWRITE, A_REWRITE_DTEL_REPORT, "udp_dst_port");
  P_REWRITE_DTEL_REPORT_UDP_SRC_PORT = data_id_from_name(
      T_MIRROR_REWRITE, A_REWRITE_DTEL_REPORT, "udp_src_port");
  P_REWRITE_DTEL_REPORT_TTL =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_DTEL_REPORT, "ttl");
  P_REWRITE_DTEL_REPORT_TOS =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_DTEL_REPORT, "tos");
  P_REWRITE_DTEL_REPORT_SESSION_ID =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_DTEL_REPORT, "session_id");
  P_REWRITE_DTEL_REPORT_MAX_PKT_LEN =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_DTEL_REPORT, "max_pkt_len");

  A_REWRITE_DTEL_REPORT_ENTROPY =
      action_id_from_name(T_MIRROR_REWRITE, "rewrite_dtel_report_with_entropy");
  P_REWRITE_DTEL_REPORT_ENTROPY_SMAC = data_id_from_name(
      T_MIRROR_REWRITE, A_REWRITE_DTEL_REPORT_ENTROPY, "smac");
  P_REWRITE_DTEL_REPORT_ENTROPY_DMAC = data_id_from_name(
      T_MIRROR_REWRITE, A_REWRITE_DTEL_REPORT_ENTROPY, "dmac");
  P_REWRITE_DTEL_REPORT_ENTROPY_SIP =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_DTEL_REPORT_ENTROPY, "sip");
  P_REWRITE_DTEL_REPORT_ENTROPY_DIP =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_DTEL_REPORT_ENTROPY, "dip");
  P_REWRITE_DTEL_REPORT_ENTROPY_UDP_DST_PORT = data_id_from_name(
      T_MIRROR_REWRITE, A_REWRITE_DTEL_REPORT_ENTROPY, "udp_dst_port");
  P_REWRITE_DTEL_REPORT_ENTROPY_TTL =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_DTEL_REPORT_ENTROPY, "ttl");
  P_REWRITE_DTEL_REPORT_ENTROPY_TOS =
      data_id_from_name(T_MIRROR_REWRITE, A_REWRITE_DTEL_REPORT_ENTROPY, "tos");
  P_REWRITE_DTEL_REPORT_ENTROPY_SESSION_ID = data_id_from_name(
      T_MIRROR_REWRITE, A_REWRITE_DTEL_REPORT_ENTROPY, "session_id");
  P_REWRITE_DTEL_REPORT_ENTROPY_MAX_PKT_LEN = data_id_from_name(
      T_MIRROR_REWRITE, A_REWRITE_DTEL_REPORT_ENTROPY, "max_pkt_len");

  A_REWRITE_IP_UDP_LENGTHS =
      action_id_from_name(T_MIRROR_REWRITE, "rewrite_ip_udp_lengths");
  A_REWRITE_DTEL_IFA_CLONE =
      action_id_from_name(T_MIRROR_REWRITE, "rewrite_dtel_ifa_clone");

  /* tunnel.p4 */
  T_TUNNEL_NEXTHOP = table_id_from_name("tunnel_nexthop");
  F_TUNNEL_NEXTHOP_LOCAL_MD_TUNNEL_NEXTHOP =
      key_id_from_name(T_TUNNEL_NEXTHOP, "local_md.tunnel_nexthop");
  A_TUNNEL_NEXTHOP_L2_TUNNEL_ENCAP =
      action_id_from_name(T_TUNNEL_NEXTHOP, "l2_tunnel_encap");
  P_TUNNEL_NEXTHOP_L2_TUNNEL_ENCAP_TYPE = data_id_from_name(
      T_TUNNEL_NEXTHOP, A_TUNNEL_NEXTHOP_L2_TUNNEL_ENCAP, "type");
  P_TUNNEL_NEXTHOP_L2_TUNNEL_ENCAP_DIP_INDEX = data_id_from_name(
      T_TUNNEL_NEXTHOP, A_TUNNEL_NEXTHOP_L2_TUNNEL_ENCAP, "dip_index");
  P_TUNNEL_NEXTHOP_L2_TUNNEL_ENCAP_TUNNEL_INDEX = data_id_from_name(
      T_TUNNEL_NEXTHOP, A_TUNNEL_NEXTHOP_L2_TUNNEL_ENCAP, "tunnel_index");
  P_TUNNEL_NEXTHOP_L2_TUNNEL_ENCAP_TUNNEL_MAPPER_INDEX =
      data_id_from_name(T_TUNNEL_NEXTHOP,
                        A_TUNNEL_NEXTHOP_L2_TUNNEL_ENCAP,
                        "tunnel_mapper_index");
  A_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP =
      action_id_from_name(T_TUNNEL_NEXTHOP, "l3_tunnel_encap");
  P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_DMAC = data_id_from_name(
      T_TUNNEL_NEXTHOP, A_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP, "dmac");
  P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_TYPE = data_id_from_name(
      T_TUNNEL_NEXTHOP, A_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP, "type");
  P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_DIP_INDEX = data_id_from_name(
      T_TUNNEL_NEXTHOP, A_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP, "dip_index");
  P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_TUNNEL_INDEX = data_id_from_name(
      T_TUNNEL_NEXTHOP, A_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP, "tunnel_index");
  A_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_WITH_VNI =
      action_id_from_name(T_TUNNEL_NEXTHOP, "l3_tunnel_encap_with_vni");
  P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_WITH_VNI_DMAC = data_id_from_name(
      T_TUNNEL_NEXTHOP, A_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_WITH_VNI, "dmac");
  P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_WITH_VNI_TYPE = data_id_from_name(
      T_TUNNEL_NEXTHOP, A_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_WITH_VNI, "type");
  P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_WITH_VNI_VNI = data_id_from_name(
      T_TUNNEL_NEXTHOP, A_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_WITH_VNI, "vni");
  P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_WITH_VNI_DIP_INDEX = data_id_from_name(
      T_TUNNEL_NEXTHOP, A_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_WITH_VNI, "dip_index");
  A_TUNNEL_NEXTHOP_MPLS_PUSH =
      action_id_from_name(T_TUNNEL_NEXTHOP, "mpls_push");
  P_MPLS_PUSH_LABEL_COUNT = data_id_from_name(
      T_TUNNEL_NEXTHOP, A_TUNNEL_NEXTHOP_MPLS_PUSH, "label_count");
  P_MPLS_PUSH_ENCAP_TTL_MODE = data_id_from_name(
      T_TUNNEL_NEXTHOP, A_TUNNEL_NEXTHOP_MPLS_PUSH, "ttl_mode");
  P_MPLS_PUSH_ENCAP_TTL = data_id_from_name(
      T_TUNNEL_NEXTHOP, A_TUNNEL_NEXTHOP_MPLS_PUSH, "encap_ttl");
  P_MPLS_PUSH_ENCAP_QOS_MODE = data_id_from_name(
      T_TUNNEL_NEXTHOP, A_TUNNEL_NEXTHOP_MPLS_PUSH, "qos_mode");
  P_MPLS_PUSH_ENCAP_EXP = data_id_from_name(
      T_TUNNEL_NEXTHOP, A_TUNNEL_NEXTHOP_MPLS_PUSH, "encap_exp");
  P_MPLS_PUSH_SWAP =
      data_id_from_name(T_TUNNEL_NEXTHOP, A_TUNNEL_NEXTHOP_MPLS_PUSH, "swap");

  P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_WITH_VNI_TUNNEL_INDEX =
      data_id_from_name(T_TUNNEL_NEXTHOP,
                        A_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_WITH_VNI,
                        "tunnel_index");
  A_TUNNEL_NEXTHOP_SRV6_ENCAP =
      action_id_from_name(T_TUNNEL_NEXTHOP, "srv6_encap");
  P_SRV6_ENCAP_SEG_LEN = data_id_from_name(
      T_TUNNEL_NEXTHOP, A_TUNNEL_NEXTHOP_SRV6_ENCAP, "seg_len");
  P_SRV6_ENCAP_TUNNEL_INDEX = data_id_from_name(
      T_TUNNEL_NEXTHOP, A_TUNNEL_NEXTHOP_SRV6_ENCAP, "tunnel_index");
  A_TUNNEL_NEXTHOP_SRV6_INSERT =
      action_id_from_name(T_TUNNEL_NEXTHOP, "srv6_insert");
  P_SRV6_INSERT_SEG_LEN = data_id_from_name(
      T_TUNNEL_NEXTHOP, A_TUNNEL_NEXTHOP_SRV6_INSERT, "seg_len");

  T_IPV4_SRC_VTEP = table_id_from_name("SwitchIngress.tunnel.src_vtep");
  F_IPV4_SRC_VTEP_SRC_ADDR = key_id_from_name(T_IPV4_SRC_VTEP, "src_addr");
  F_IPV4_SRC_VTEP_LOCAL_MD_VRF =
      key_id_from_name(T_IPV4_SRC_VTEP, "local_md.vrf");
  F_IPV4_SRC_VTEP_LOCAL_MD_TUNNEL_TYPE =
      key_id_from_name(T_IPV4_SRC_VTEP, "local_md.tunnel.type");
  A_IPV4_SRC_VTEP_MISS = action_id_from_name(
      T_IPV4_SRC_VTEP, "SwitchIngress.tunnel.src_vtep_miss");
  A_IPV4_SRC_VTEP_HIT =
      action_id_from_name(T_IPV4_SRC_VTEP, "SwitchIngress.tunnel.src_vtep_hit");
  P_IPV4_SRC_VTEP_HIT_IFINDEX =
      data_id_from_name(T_IPV4_SRC_VTEP, A_IPV4_SRC_VTEP_HIT, "ifindex");

  T_IPV4_DST_VTEP = table_id_from_name("dst_vtep");
  F_IPV4_DST_VTEP_DST_ADDR = key_id_from_name(T_IPV4_DST_VTEP, "dst_addr");
  F_IPV4_DST_VTEP_SRC_ADDR = key_id_from_name(T_IPV4_DST_VTEP, "src_addr");
  F_IPV4_DST_VTEP_LOCAL_MD_VRF =
      key_id_from_name(T_IPV4_DST_VTEP, "local_md.vrf");
  F_IPV4_DST_VTEP_LOCAL_MD_TUNNEL_TYPE =
      key_id_from_name(T_IPV4_DST_VTEP, "local_md.tunnel.type");
  A_IPV4_DST_VTEP_HIT = action_id_from_name(T_IPV4_DST_VTEP, "dst_vtep_hit");
  P_IPV4_DST_VTEP_HIT_TTL_MODE =
      data_id_from_name(T_IPV4_DST_VTEP, A_IPV4_DST_VTEP_HIT, "ttl_mode");
  P_IPV4_DST_VTEP_HIT_QOS_MODE =
      data_id_from_name(T_IPV4_DST_VTEP, A_IPV4_DST_VTEP_HIT, "qos_mode");
  A_IPV4_DST_VTEP_SET_INNER_BD_PROPERTIES =
      action_id_from_name(T_IPV4_DST_VTEP, "set_inner_bd_properties");
  P_IPV4_DST_VTEP_SET_PROPERTIES_BD = data_id_from_name(
      T_IPV4_DST_VTEP, A_IPV4_DST_VTEP_SET_INNER_BD_PROPERTIES, "bd");
  P_IPV4_DST_VTEP_SET_PROPERTIES_VRF = data_id_from_name(
      T_IPV4_DST_VTEP, A_IPV4_DST_VTEP_SET_INNER_BD_PROPERTIES, "vrf");
  P_IPV4_DST_VTEP_SET_PROPERTIES_VRF_TTL_VIOLATION =
      data_id_from_name(T_IPV4_DST_VTEP,
                        A_IPV4_DST_VTEP_SET_INNER_BD_PROPERTIES,
                        "vrf_ttl_violation");
  P_IPV4_DST_VTEP_SET_PROPERTIES_VRF_TTL_VIOLATION_VALID =
      data_id_from_name(T_IPV4_DST_VTEP,
                        A_IPV4_DST_VTEP_SET_INNER_BD_PROPERTIES,
                        "vrf_ttl_violation_valid");
  P_IPV4_DST_VTEP_SET_PROPERTIES_VRF_IP_OPTIONS_VIOLATION =
      data_id_from_name(T_IPV4_DST_VTEP,
                        A_IPV4_DST_VTEP_SET_INNER_BD_PROPERTIES,
                        "vrf_ip_options_violation");
  P_IPV4_DST_VTEP_SET_PROPERTIES_BD_LABEL = data_id_from_name(
      T_IPV4_DST_VTEP, A_IPV4_DST_VTEP_SET_INNER_BD_PROPERTIES, "bd_label");
  //  P_IPV4_DST_VTEP_SET_PROPERTIES_RID = data_id_from_name(
  //      T_IPV4_DST_VTEP, A_IPV4_DST_VTEP_SET_INNER_BD_PROPERTIES, "rid");
  P_IPV4_DST_VTEP_SET_PROPERTIES_LEARNING_MODE =
      data_id_from_name(T_IPV4_DST_VTEP,
                        A_IPV4_DST_VTEP_SET_INNER_BD_PROPERTIES,
                        "learning_mode");
  P_IPV4_DST_VTEP_SET_PROPERTIES_IPV4_UNICAST_ENABLE =
      data_id_from_name(T_IPV4_DST_VTEP,
                        A_IPV4_DST_VTEP_SET_INNER_BD_PROPERTIES,
                        "ipv4_unicast_enable");
  P_IPV4_DST_VTEP_SET_PROPERTIES_IPV6_UNICAST_ENABLE =
      data_id_from_name(T_IPV4_DST_VTEP,
                        A_IPV4_DST_VTEP_SET_INNER_BD_PROPERTIES,
                        "ipv6_unicast_enable");
  P_IPV4_DST_VTEP_SET_PROPERTIES_TTL_MODE = data_id_from_name(
      T_IPV4_DST_VTEP, A_IPV4_DST_VTEP_SET_INNER_BD_PROPERTIES, "ttl_mode");
  P_IPV4_DST_VTEP_SET_PROPERTIES_QOS_MODE = data_id_from_name(
      T_IPV4_DST_VTEP, A_IPV4_DST_VTEP_SET_INNER_BD_PROPERTIES, "qos_mode");
  P_IPV4_DST_VTEP_SET_PROPERTIES_ECN_MODE = data_id_from_name(
      T_IPV4_DST_VTEP, A_IPV4_DST_VTEP_SET_INNER_BD_PROPERTIES, "ecn_mode");

  T_IPV6_SRC_VTEP =
      table_id_from_name("SwitchIngress.tunnel.ipv6_vtep.src_vtep");
  F_IPV6_SRC_VTEP_SRC_ADDR = key_id_from_name(T_IPV6_SRC_VTEP, "src_addr");
  F_IPV6_SRC_VTEP_LOCAL_MD_VRF =
      key_id_from_name(T_IPV6_SRC_VTEP, "local_md.vrf");
  F_IPV6_SRC_VTEP_LOCAL_MD_TUNNEL_TYPE =
      key_id_from_name(T_IPV6_SRC_VTEP, "local_md.tunnel.type");
  A_IPV6_SRC_VTEP_MISS = action_id_from_name(
      T_IPV6_SRC_VTEP, "SwitchIngress.tunnel.ipv6_vtep.src_vtep_miss");
  A_IPV6_SRC_VTEP_HIT = action_id_from_name(
      T_IPV6_SRC_VTEP, "SwitchIngress.tunnel.ipv6_vtep.src_vtep_hit");
  P_IPV6_SRC_VTEP_HIT_IFINDEX =
      data_id_from_name(T_IPV6_SRC_VTEP, A_IPV6_SRC_VTEP_HIT, "ifindex");

  T_IPV6_DST_VTEP = table_id_from_name("dst_vtepv6");
  F_IPV6_DST_VTEP_SRC_ADDR = key_id_from_name(T_IPV6_DST_VTEP, "src_addr");
  F_IPV6_DST_VTEP_DST_ADDR = key_id_from_name(T_IPV6_DST_VTEP, "dst_addr");
  F_IPV6_DST_VTEP_LOCAL_MD_VRF =
      key_id_from_name(T_IPV6_DST_VTEP, "local_md.vrf");
  F_IPV6_DST_VTEP_LOCAL_MD_TUNNEL_TYPE =
      key_id_from_name(T_IPV6_DST_VTEP, "local_md.tunnel.type");
  A_IPV6_DST_VTEP_HIT = action_id_from_name(T_IPV6_DST_VTEP, "dst_vtep_hit");
  P_IPV6_DST_VTEP_HIT_TTL_MODE =
      data_id_from_name(T_IPV6_DST_VTEP, A_IPV6_DST_VTEP_HIT, "ttl_mode");
  P_IPV6_DST_VTEP_HIT_QOS_MODE =
      data_id_from_name(T_IPV6_DST_VTEP, A_IPV6_DST_VTEP_HIT, "qos_mode");
  A_IPV6_DST_VTEP_SET_INNER_BD_PROPERTIES =
      action_id_from_name(T_IPV6_DST_VTEP, "set_inner_bd_properties");
  P_IPV6_DST_VTEP_SET_PROPERTIES_BD = data_id_from_name(
      T_IPV6_DST_VTEP, A_IPV6_DST_VTEP_SET_INNER_BD_PROPERTIES, "bd");
  P_IPV6_DST_VTEP_SET_PROPERTIES_VRF = data_id_from_name(
      T_IPV6_DST_VTEP, A_IPV6_DST_VTEP_SET_INNER_BD_PROPERTIES, "vrf");
  P_IPV6_DST_VTEP_SET_PROPERTIES_BD_LABEL = data_id_from_name(
      T_IPV6_DST_VTEP, A_IPV6_DST_VTEP_SET_INNER_BD_PROPERTIES, "bd_label");
  //  P_IPV6_DST_VTEP_SET_PROPERTIES_RID = data_id_from_name(
  //      T_IPV6_DST_VTEP, A_IPV6_DST_VTEP_SET_INNER_BD_PROPERTIES, "rid");
  P_IPV6_DST_VTEP_SET_PROPERTIES_LEARNING_MODE =
      data_id_from_name(T_IPV6_DST_VTEP,
                        A_IPV6_DST_VTEP_SET_INNER_BD_PROPERTIES,
                        "learning_mode");
  P_IPV6_DST_VTEP_SET_PROPERTIES_IPV4_UNICAST_ENABLE =
      data_id_from_name(T_IPV6_DST_VTEP,
                        A_IPV6_DST_VTEP_SET_INNER_BD_PROPERTIES,
                        "ipv4_unicast_enable");
  P_IPV6_DST_VTEP_SET_PROPERTIES_IPV6_UNICAST_ENABLE =
      data_id_from_name(T_IPV6_DST_VTEP,
                        A_IPV6_DST_VTEP_SET_INNER_BD_PROPERTIES,
                        "ipv6_unicast_enable");
  P_IPV6_DST_VTEP_SET_PROPERTIES_TTL_MODE = data_id_from_name(
      T_IPV6_DST_VTEP, A_IPV6_DST_VTEP_SET_INNER_BD_PROPERTIES, "ttl_mode");
  P_IPV6_DST_VTEP_SET_PROPERTIES_QOS_MODE = data_id_from_name(
      T_IPV6_DST_VTEP, A_IPV6_DST_VTEP_SET_INNER_BD_PROPERTIES, "qos_mode");
  P_IPV6_DST_VTEP_SET_PROPERTIES_ECN_MODE = data_id_from_name(
      T_IPV6_DST_VTEP, A_IPV6_DST_VTEP_SET_INNER_BD_PROPERTIES, "ecn_mode");

  T_TUNNEL_RMAC = table_id_from_name("vxlan_rmac");
  F_TUNNEL_RMAC_LOCAL_MD_TUNNEL_VNI =
      key_id_from_name(T_TUNNEL_RMAC, "local_md.tunnel.vni");
  F_TUNNEL_RMAC_HDR_INNER_ETHERNET_DST_ADDR =
      key_id_from_name(T_TUNNEL_RMAC, "hdr.inner_ethernet.dst_addr");
  A_TUNNEL_RMAC_MISS = action_id_from_name(T_TUNNEL_RMAC, "tunnel_rmac_miss");
  A_TUNNEL_RMAC_HIT = action_id_from_name(T_TUNNEL_RMAC, "tunnel_rmac_hit");

  T_TUNNEL_VXLAN_DEVICE_RMAC = table_id_from_name("vxlan_device_rmac");
  F_TUNNEL_VXLAN_DEVICE_RMAC_HDR_INNER_ETHERNET_DST_ADDR = key_id_from_name(
      T_TUNNEL_VXLAN_DEVICE_RMAC, "hdr.inner_ethernet.dst_addr");

  T_VNI_TO_BD_MAPPING = table_id_from_name("vni_to_bd_mapping");
  F_VNI_TO_BD_MAPPING_LOCAL_MD_TUNNEL_VNI =
      key_id_from_name(T_VNI_TO_BD_MAPPING, "local_md.tunnel.vni");
  A_VNI_TO_BD_MAPPING_SET_INNER_BD_PROPERTIES =
      action_id_from_name(T_VNI_TO_BD_MAPPING, "set_inner_bd_properties_base");
  P_VNI_SET_PROPERTIES_BD = data_id_from_name(
      T_VNI_TO_BD_MAPPING, A_VNI_TO_BD_MAPPING_SET_INNER_BD_PROPERTIES, "bd");
  P_VNI_SET_PROPERTIES_VRF = data_id_from_name(
      T_VNI_TO_BD_MAPPING, A_VNI_TO_BD_MAPPING_SET_INNER_BD_PROPERTIES, "vrf");

  P_VNI_SET_PROPERTIES_VRF_TTL_VIOLATION =
      data_id_from_name(T_VNI_TO_BD_MAPPING,
                        A_VNI_TO_BD_MAPPING_SET_INNER_BD_PROPERTIES,
                        "vrf_ttl_violation");
  P_VNI_SET_PROPERTIES_VRF_TTL_VIOLATION_VALID =
      data_id_from_name(T_VNI_TO_BD_MAPPING,
                        A_VNI_TO_BD_MAPPING_SET_INNER_BD_PROPERTIES,
                        "vrf_ttl_violation_valid");
  P_VNI_SET_PROPERTIES_VRF_IP_OPTIONS_VIOLATION =
      data_id_from_name(T_VNI_TO_BD_MAPPING,
                        A_VNI_TO_BD_MAPPING_SET_INNER_BD_PROPERTIES,
                        "vrf_ip_options_violation");

  P_VNI_SET_PROPERTIES_BD_LABEL =
      data_id_from_name(T_VNI_TO_BD_MAPPING,
                        A_VNI_TO_BD_MAPPING_SET_INNER_BD_PROPERTIES,
                        "bd_label");
  //  P_VNI_SET_PROPERTIES_RID = data_id_from_name(
  //      T_VNI_TO_BD_MAPPING, A_VNI_TO_BD_MAPPING_SET_INNER_BD_PROPERTIES,
  //      "rid");
  P_VNI_SET_PROPERTIES_LEARNING_MODE =
      data_id_from_name(T_VNI_TO_BD_MAPPING,
                        A_VNI_TO_BD_MAPPING_SET_INNER_BD_PROPERTIES,
                        "learning_mode");
  P_VNI_SET_PROPERTIES_IPV4_UNICAST_ENABLE =
      data_id_from_name(T_VNI_TO_BD_MAPPING,
                        A_VNI_TO_BD_MAPPING_SET_INNER_BD_PROPERTIES,
                        "ipv4_unicast_enable");
  P_VNI_SET_PROPERTIES_IPV6_UNICAST_ENABLE =
      data_id_from_name(T_VNI_TO_BD_MAPPING,
                        A_VNI_TO_BD_MAPPING_SET_INNER_BD_PROPERTIES,
                        "ipv6_unicast_enable");

  T_TUNNEL_SRC_ADDR_REWRITE = table_id_from_name("src_addr_rewrite");
  F_TUNNEL_SRC_ADDR_REWRITE_TUNNEL_INDEX =
      key_id_from_name(T_TUNNEL_SRC_ADDR_REWRITE, "local_md.tunnel.index");
  A_TUNNEL_SRC_ADDR_REWRITE_IPV4_SIP_REWRITE =
      action_id_from_name(T_TUNNEL_SRC_ADDR_REWRITE, "ipv4_sip_rewrite");
  P_TUNNEL_SRC_ADDR_REWRITE_IPV4_SIP_REWRITE_SRC_ADDR =
      data_id_from_name(T_TUNNEL_SRC_ADDR_REWRITE,
                        A_TUNNEL_SRC_ADDR_REWRITE_IPV4_SIP_REWRITE,
                        "src_addr");
  P_TUNNEL_SRC_ADDR_REWRITE_IPV4_SIP_REWRITE_TTL_VAL =
      data_id_from_name(T_TUNNEL_SRC_ADDR_REWRITE,
                        A_TUNNEL_SRC_ADDR_REWRITE_IPV4_SIP_REWRITE,
                        "ttl_val");
  P_TUNNEL_SRC_ADDR_REWRITE_IPV4_SIP_REWRITE_DSCP_VAL =
      data_id_from_name(T_TUNNEL_SRC_ADDR_REWRITE,
                        A_TUNNEL_SRC_ADDR_REWRITE_IPV4_SIP_REWRITE,
                        "dscp_val");
  A_TUNNEL_SRC_ADDR_REWRITE_IPV6_SIP_REWRITE =
      action_id_from_name(T_TUNNEL_SRC_ADDR_REWRITE, "ipv6_sip_rewrite");
  P_TUNNEL_SRC_ADDR_REWRITE_IPV6_SIP_REWRITE_SRC_ADDR =
      data_id_from_name(T_TUNNEL_SRC_ADDR_REWRITE,
                        A_TUNNEL_SRC_ADDR_REWRITE_IPV6_SIP_REWRITE,
                        "src_addr");
  P_TUNNEL_SRC_ADDR_REWRITE_IPV6_SIP_REWRITE_TTL_VAL =
      data_id_from_name(T_TUNNEL_SRC_ADDR_REWRITE,
                        A_TUNNEL_SRC_ADDR_REWRITE_IPV6_SIP_REWRITE,
                        "ttl_val");
  P_TUNNEL_SRC_ADDR_REWRITE_IPV6_SIP_REWRITE_DSCP_VAL =
      data_id_from_name(T_TUNNEL_SRC_ADDR_REWRITE,
                        A_TUNNEL_SRC_ADDR_REWRITE_IPV6_SIP_REWRITE,
                        "dscp_val");

  T_TUNNEL_DST_ADDR_REWRITE = table_id_from_name("dst_addr_rewrite");
  F_TUNNEL_DST_ADDR_REWRITE_LOCAL_MD_TUNNEL_DIP_INDEX =
      key_id_from_name(T_TUNNEL_DST_ADDR_REWRITE, "local_md.tunnel.dip_index");
  A_TUNNEL_DST_ADDR_REWRITE_IPV4_DIP_REWRITE =
      action_id_from_name(T_TUNNEL_DST_ADDR_REWRITE, "ipv4_dip_rewrite");
  P_TUNNEL_DST_ADDR_REWRITE_IPV4_DIP_REWRITE_DST_ADDR =
      data_id_from_name(T_TUNNEL_DST_ADDR_REWRITE,
                        A_TUNNEL_DST_ADDR_REWRITE_IPV4_DIP_REWRITE,
                        "dst_addr");
  A_TUNNEL_DST_ADDR_REWRITE_IPV6_DIP_REWRITE =
      action_id_from_name(T_TUNNEL_DST_ADDR_REWRITE, "ipv6_dip_rewrite");
  P_TUNNEL_DST_ADDR_REWRITE_IPV6_DIP_REWRITE_DST_ADDR =
      data_id_from_name(T_TUNNEL_DST_ADDR_REWRITE,
                        A_TUNNEL_DST_ADDR_REWRITE_IPV6_DIP_REWRITE,
                        "dst_addr");

  T_TUNNEL_REWRITE_ENCAP_TTL = table_id_from_name("tunnel_rewrite_encap_ttl");
  F_TUNNEL_REWRITE_ENCAP_TTL_LOCAL_MD_TUNNEL_INDEX =
      key_id_from_name(T_TUNNEL_REWRITE_ENCAP_TTL, "local_md.tunnel.index");
  F_TUNNEL_REWRITE_ENCAP_TTL_IPV4_VALID =
      key_id_from_name(T_TUNNEL_REWRITE_ENCAP_TTL, "hdr.ipv4.$valid");
  F_TUNNEL_REWRITE_ENCAP_TTL_IPV6_VALID =
      key_id_from_name(T_TUNNEL_REWRITE_ENCAP_TTL, "hdr.ipv6.$valid");
  F_TUNNEL_REWRITE_ENCAP_TTL_INNER_IPV4_VALID =
      key_id_from_name(T_TUNNEL_REWRITE_ENCAP_TTL, "hdr.inner_ipv4.$valid");
  F_TUNNEL_REWRITE_ENCAP_TTL_INNER_IPV6_VALID =
      key_id_from_name(T_TUNNEL_REWRITE_ENCAP_TTL, "hdr.inner_ipv6.$valid");
  A_TUNNEL_REWRITE_ENCAP_TTL_V4_IN_V4_PIPE = action_id_from_name(
      T_TUNNEL_REWRITE_ENCAP_TTL, "encap_ttl_v4_in_v4_pipe");
  P_TUNNEL_REWRITE_ENCAP_TTL_V4_IN_V4_PIPE_TTL_VAL =
      data_id_from_name(T_TUNNEL_REWRITE_ENCAP_TTL,
                        A_TUNNEL_REWRITE_ENCAP_TTL_V4_IN_V4_PIPE,
                        "ttl_val");
  A_TUNNEL_REWRITE_ENCAP_TTL_V4_IN_V6_PIPE = action_id_from_name(
      T_TUNNEL_REWRITE_ENCAP_TTL, "encap_ttl_v4_in_v6_pipe");
  P_TUNNEL_REWRITE_ENCAP_TTL_V4_IN_V6_PIPE_TTL_VAL =
      data_id_from_name(T_TUNNEL_REWRITE_ENCAP_TTL,
                        A_TUNNEL_REWRITE_ENCAP_TTL_V4_IN_V6_PIPE,
                        "ttl_val");
  A_TUNNEL_REWRITE_ENCAP_TTL_V6_IN_V4_PIPE = action_id_from_name(
      T_TUNNEL_REWRITE_ENCAP_TTL, "encap_ttl_v6_in_v4_pipe");
  P_TUNNEL_REWRITE_ENCAP_TTL_V6_IN_V4_PIPE_TTL_VAL =
      data_id_from_name(T_TUNNEL_REWRITE_ENCAP_TTL,
                        A_TUNNEL_REWRITE_ENCAP_TTL_V6_IN_V4_PIPE,
                        "ttl_val");
  A_TUNNEL_REWRITE_ENCAP_TTL_V6_IN_V6_PIPE = action_id_from_name(
      T_TUNNEL_REWRITE_ENCAP_TTL, "encap_ttl_v6_in_v6_pipe");
  P_TUNNEL_REWRITE_ENCAP_TTL_V6_IN_V6_PIPE_TTL_VAL =
      data_id_from_name(T_TUNNEL_REWRITE_ENCAP_TTL,
                        A_TUNNEL_REWRITE_ENCAP_TTL_V6_IN_V6_PIPE,
                        "ttl_val");
  A_TUNNEL_REWRITE_ENCAP_TTL_V4_IN_V6_UNIFORM = action_id_from_name(
      T_TUNNEL_REWRITE_ENCAP_TTL, "encap_ttl_v4_in_v6_uniform");
  A_TUNNEL_REWRITE_ENCAP_TTL_V6_IN_V4_UNIFORM = action_id_from_name(
      T_TUNNEL_REWRITE_ENCAP_TTL, "encap_ttl_v6_in_v4_uniform");

  T_TUNNEL_REWRITE_ENCAP_DSCP = table_id_from_name("tunnel_rewrite_encap_dscp");
  F_TUNNEL_REWRITE_ENCAP_DSCP_LOCAL_MD_TUNNEL_INDEX =
      key_id_from_name(T_TUNNEL_REWRITE_ENCAP_DSCP, "local_md.tunnel.index");
  F_TUNNEL_REWRITE_ENCAP_DSCP_IPV4_VALID =
      key_id_from_name(T_TUNNEL_REWRITE_ENCAP_DSCP, "hdr.ipv4.$valid");
  F_TUNNEL_REWRITE_ENCAP_DSCP_IPV6_VALID =
      key_id_from_name(T_TUNNEL_REWRITE_ENCAP_DSCP, "hdr.ipv6.$valid");
  F_TUNNEL_REWRITE_ENCAP_DSCP_INNER_IPV4_VALID =
      key_id_from_name(T_TUNNEL_REWRITE_ENCAP_DSCP, "hdr.inner_ipv4.$valid");
  F_TUNNEL_REWRITE_ENCAP_DSCP_INNER_IPV6_VALID =
      key_id_from_name(T_TUNNEL_REWRITE_ENCAP_DSCP, "hdr.inner_ipv6.$valid");
  A_TUNNEL_REWRITE_ENCAP_DSCP_V4_IN_V4_ECN = action_id_from_name(
      T_TUNNEL_REWRITE_ENCAP_DSCP, "encap_dscp_v4_in_v4_ecn");
  A_TUNNEL_REWRITE_ENCAP_DSCP_V4_IN_V6_ECN = action_id_from_name(
      T_TUNNEL_REWRITE_ENCAP_DSCP, "encap_dscp_v4_in_v6_ecn");
  A_TUNNEL_REWRITE_ENCAP_DSCP_V6_IN_V4_ECN = action_id_from_name(
      T_TUNNEL_REWRITE_ENCAP_DSCP, "encap_dscp_v6_in_v4_ecn");
  A_TUNNEL_REWRITE_ENCAP_DSCP_V6_IN_V6_ECN = action_id_from_name(
      T_TUNNEL_REWRITE_ENCAP_DSCP, "encap_dscp_v6_in_v6_ecn");
  A_TUNNEL_REWRITE_ENCAP_DSCP_V4_IN_V4_UNIFORM = action_id_from_name(
      T_TUNNEL_REWRITE_ENCAP_DSCP, "encap_dscp_v4_in_v4_uniform");
  A_TUNNEL_REWRITE_ENCAP_DSCP_V4_IN_V6_UNIFORM = action_id_from_name(
      T_TUNNEL_REWRITE_ENCAP_DSCP, "encap_dscp_v4_in_v6_uniform");
  A_TUNNEL_REWRITE_ENCAP_DSCP_V6_IN_V4_UNIFORM = action_id_from_name(
      T_TUNNEL_REWRITE_ENCAP_DSCP, "encap_dscp_v6_in_v4_uniform");
  A_TUNNEL_REWRITE_ENCAP_DSCP_V6_IN_V6_UNIFORM = action_id_from_name(
      T_TUNNEL_REWRITE_ENCAP_DSCP, "encap_dscp_v6_in_v6_uniform");
  A_TUNNEL_REWRITE_ENCAP_DSCP_V4_PIPE = action_id_from_name(
      T_TUNNEL_REWRITE_ENCAP_DSCP, "encap_dscp_v4_pipe_mode");
  P_TUNNEL_REWRITE_ENCAP_DSCP_V4_PIPE_DSCP_VAL =
      data_id_from_name(T_TUNNEL_REWRITE_ENCAP_DSCP,
                        A_TUNNEL_REWRITE_ENCAP_DSCP_V4_PIPE,
                        "dscp_val");
  A_TUNNEL_REWRITE_ENCAP_DSCP_V6_PIPE = action_id_from_name(
      T_TUNNEL_REWRITE_ENCAP_DSCP, "encap_dscp_v6_pipe_mode");
  P_TUNNEL_REWRITE_ENCAP_DSCP_V6_PIPE_DSCP_VAL =
      data_id_from_name(T_TUNNEL_REWRITE_ENCAP_DSCP,
                        A_TUNNEL_REWRITE_ENCAP_DSCP_V6_PIPE,
                        "dscp_val");

  T_EGRESS_VRF_MAPPING = table_id_from_name("vrf_mapping");
  F_EGRESS_VRF_MAPPING_LOCAL_MD_VRF =
      key_id_from_name(T_EGRESS_VRF_MAPPING, "vrf");
  A_EGRESS_VRF_MAPPING_SET_VRF_PROPERTIES =
      action_id_from_name(T_EGRESS_VRF_MAPPING, "set_vrf_properties");
  P_EGRESS_VRF_MAPPING_SET_VRF_PROPERTIES_SMAC = data_id_from_name(
      T_EGRESS_VRF_MAPPING, A_EGRESS_VRF_MAPPING_SET_VRF_PROPERTIES, "smac");

  T_BD_TO_VNI_MAPPING = table_id_from_name("bd_to_vni_mapping");
  F_BD_TO_VNI_MAPPING_LOCAL_MD_BD =
      key_id_from_name(T_BD_TO_VNI_MAPPING, "local_md.bd[11:0]");
  F_BD_TO_VNI_MAPPING_LOCAL_MD_TUNNEL_MAPPER_INDEX =
      key_id_from_name(T_BD_TO_VNI_MAPPING, "local_md.tunnel.mapper_index");
  A_BD_TO_VNI_MAPPING_SET_VNI =
      action_id_from_name(T_BD_TO_VNI_MAPPING, "set_vni");
  P_BD_SET_PROPERTIES_VNI = data_id_from_name(
      T_BD_TO_VNI_MAPPING, A_BD_TO_VNI_MAPPING_SET_VNI, "vni");

  T_VRF_TO_VNI_MAPPING = table_id_from_name("vrf_to_vni_mapping");
  F_VRF_TO_VNI_MAPPING_LOCAL_MD_VRF =
      key_id_from_name(T_VRF_TO_VNI_MAPPING, "local_md.vrf");
  A_VRF_TO_VNI_MAPPING_SET_VNI =
      action_id_from_name(T_VRF_TO_VNI_MAPPING, "set_vni");
  P_VRF_SET_PROPERTIES_VNI = data_id_from_name(
      T_VRF_TO_VNI_MAPPING, A_VRF_TO_VNI_MAPPING_SET_VNI, "vni");

  T_TUNNEL_TABLE = table_id_from_name("tunnel_encap_1");
  F_TUNNEL_TABLE_LOCAL_MD_TUNNEL_TYPE =
      key_id_from_name(T_TUNNEL_TABLE, "local_md.tunnel.type");
  A_TUNNEL_TABLE_ENCAP_IPV4_VXLAN =
      action_id_from_name(T_TUNNEL_TABLE, "encap_ipv4_vxlan");
  F_TUNNEL_TABLE_ENCAP_IPV4_VXLAN_PORT = data_id_from_name(
      T_TUNNEL_TABLE, A_TUNNEL_TABLE_ENCAP_IPV4_VXLAN, "vxlan_port");
  A_TUNNEL_TABLE_ENCAP_IPV6_VXLAN =
      action_id_from_name(T_TUNNEL_TABLE, "encap_ipv6_vxlan");
  F_TUNNEL_TABLE_ENCAP_IPV6_VXLAN_PORT = data_id_from_name(
      T_TUNNEL_TABLE, A_TUNNEL_TABLE_ENCAP_IPV6_VXLAN, "vxlan_port");
  A_TUNNEL_TABLE_ENCAP_IPV4_IP =
      action_id_from_name(T_TUNNEL_TABLE, "encap_ipv4_ip");
  A_TUNNEL_TABLE_ENCAP_IPV6_IP =
      action_id_from_name(T_TUNNEL_TABLE, "encap_ipv6_ip");
  A_TUNNEL_TABLE_ENCAP_IPV4_GRE =
      action_id_from_name(T_TUNNEL_TABLE, "encap_ipv4_gre");
  A_TUNNEL_TABLE_ENCAP_IPV6_GRE =
      action_id_from_name(T_TUNNEL_TABLE, "encap_ipv6_gre");

  /* qos.p4 */
  T_DSCP_TC_MAP = table_id_from_name("ingress_qos_map.dscp_tc_map");
  F_DSCP_TC_MAP_QOS_INGRESS_PORT =
      key_id_from_name(T_DSCP_TC_MAP, "local_md.ingress_port");
  F_DSCP_TC_MAP_LKP_IP_DSCP =
      key_id_from_name(T_DSCP_TC_MAP, "local_md.lkp.ip_tos[7:2]");
  A_DSCP_TC_MAP_SET_INGRESS_TC =
      action_id_from_name(T_DSCP_TC_MAP, "ingress_qos_map.set_ingress_tc");
  F_DSCP_TC_MAP_SET_INGRESS_TC_TC =
      data_id_from_name(T_DSCP_TC_MAP, A_DSCP_TC_MAP_SET_INGRESS_TC, "tc");
  A_DSCP_TC_MAP_SET_INGRESS_COLOR =
      action_id_from_name(T_DSCP_TC_MAP, "ingress_qos_map.set_ingress_color");
  F_DSCP_TC_MAP_SET_INGRESS_COLOR_COLOR = data_id_from_name(
      T_DSCP_TC_MAP, A_DSCP_TC_MAP_SET_INGRESS_COLOR, "color");
  A_DSCP_TC_MAP_SET_INGRESS_TC_AND_COLOR = action_id_from_name(
      T_DSCP_TC_MAP, "ingress_qos_map.set_ingress_tc_and_color");
  F_DSCP_TC_MAP_SET_INGRESS_TC_AND_COLOR_TC = data_id_from_name(
      T_DSCP_TC_MAP, A_DSCP_TC_MAP_SET_INGRESS_TC_AND_COLOR, "tc");
  F_DSCP_TC_MAP_SET_INGRESS_TC_AND_COLOR_COLOR = data_id_from_name(
      T_DSCP_TC_MAP, A_DSCP_TC_MAP_SET_INGRESS_TC_AND_COLOR, "color");
  //  A_DSCP_TC_MAP_SET_INGRESS_TC_COLOR_AND_METER = action_id_from_name(
  //      T_DSCP_TC_MAP,
  //      "SwitchIngress.qos_map.set_ingress_tc_color_and_meter");
  //  F_DSCP_TC_MAP_SET_INGRESS_TC_COLOR_AND_METER_TC = data_id_from_name(
  //      T_DSCP_TC_MAP, A_DSCP_TC_MAP_SET_INGRESS_TC_COLOR_AND_METER, "tc");
  //  F_DSCP_TC_MAP_SET_INGRESS_TC_COLOR_AND_METER_COLOR = data_id_from_name(
  //      T_DSCP_TC_MAP, A_DSCP_TC_MAP_SET_INGRESS_TC_COLOR_AND_METER, "color");
  //  F_DSCP_TC_MAP_SET_INGRESS_TC_COLOR_AND_METER_METER = data_id_from_name(
  //      T_DSCP_TC_MAP, A_DSCP_TC_MAP_SET_INGRESS_TC_COLOR_AND_METER, "index");

  T_PCP_TC_MAP = table_id_from_name("ingress_qos_map.pcp_tc_map");
  F_PCP_TC_MAP_QOS_INGRESS_PORT =
      key_id_from_name(T_PCP_TC_MAP, "local_md.ingress_port");
  F_PCP_TC_MAP_LKP_PCP = key_id_from_name(T_PCP_TC_MAP, "local_md.lkp.pcp");
  A_PCP_TC_MAP_SET_INGRESS_TC =
      action_id_from_name(T_PCP_TC_MAP, "ingress_qos_map.set_ingress_tc");
  F_PCP_TC_MAP_SET_INGRESS_TC_TC =
      data_id_from_name(T_PCP_TC_MAP, A_PCP_TC_MAP_SET_INGRESS_TC, "tc");
  A_PCP_TC_MAP_SET_INGRESS_COLOR =
      action_id_from_name(T_PCP_TC_MAP, "ingress_qos_map.set_ingress_color");
  F_PCP_TC_MAP_SET_INGRESS_COLOR_COLOR =
      data_id_from_name(T_PCP_TC_MAP, A_PCP_TC_MAP_SET_INGRESS_COLOR, "color");
  A_PCP_TC_MAP_SET_INGRESS_TC_AND_COLOR = action_id_from_name(
      T_PCP_TC_MAP, "ingress_qos_map.set_ingress_tc_and_color");
  F_PCP_TC_MAP_SET_INGRESS_TC_AND_COLOR_TC = data_id_from_name(
      T_PCP_TC_MAP, A_PCP_TC_MAP_SET_INGRESS_TC_AND_COLOR, "tc");
  F_PCP_TC_MAP_SET_INGRESS_TC_AND_COLOR_COLOR = data_id_from_name(
      T_PCP_TC_MAP, A_PCP_TC_MAP_SET_INGRESS_TC_AND_COLOR, "color");
  //  A_PCP_TC_MAP_SET_INGRESS_TC_COLOR_AND_METER = action_id_from_name(
  //      T_PCP_TC_MAP, "SwitchIngress.qos_map.set_ingress_tc_color_and_meter");
  //  F_PCP_TC_MAP_SET_INGRESS_TC_COLOR_AND_METER_TC = data_id_from_name(
  //      T_PCP_TC_MAP, A_PCP_TC_MAP_SET_INGRESS_TC_COLOR_AND_METER, "tc");
  //  F_PCP_TC_MAP_SET_INGRESS_TC_COLOR_AND_METER_COLOR = data_id_from_name(
  //      T_PCP_TC_MAP, A_PCP_TC_MAP_SET_INGRESS_TC_COLOR_AND_METER, "color");
  //  F_PCP_TC_MAP_SET_INGRESS_TC_COLOR_AND_METER_METER = data_id_from_name(
  //      T_PCP_TC_MAP, A_PCP_TC_MAP_SET_INGRESS_TC_COLOR_AND_METER, "index");

  T_TRAFFIC_CLASS = table_id_from_name("ingress_tc.traffic_class");
  F_TRAFFIC_CLASS_LOCAL_MD_INGRESS_PORT =
      key_id_from_name(T_TRAFFIC_CLASS, "port");
  F_TRAFFIC_CLASS_QOS_MD_COLOR = key_id_from_name(T_TRAFFIC_CLASS, "color");
  F_TRAFFIC_CLASS_QOS_MD_TC = key_id_from_name(T_TRAFFIC_CLASS, "tc");
  A_TRAFFIC_CLASS_SET_ICOS =
      action_id_from_name(T_TRAFFIC_CLASS, "ingress_tc.set_icos");
  F_TRAFFIC_CLASS_SET_ICOS_ICOS =
      data_id_from_name(T_TRAFFIC_CLASS, A_TRAFFIC_CLASS_SET_ICOS, "icos");
  A_TRAFFIC_CLASS_SET_QID =
      action_id_from_name(T_TRAFFIC_CLASS, "ingress_tc.set_queue");
  F_TRAFFIC_CLASS_SET_QID_QID =
      data_id_from_name(T_TRAFFIC_CLASS, A_TRAFFIC_CLASS_SET_QID, "qid");
  A_TRAFFIC_CLASS_SET_ICOS_AND_QUEUE =
      action_id_from_name(T_TRAFFIC_CLASS, "ingress_tc.set_icos_and_queue");
  F_TRAFFIC_CLASS_SET_ICOS_AND_QUEUE_ICOS = data_id_from_name(
      T_TRAFFIC_CLASS, A_TRAFFIC_CLASS_SET_ICOS_AND_QUEUE, "icos");
  F_TRAFFIC_CLASS_SET_ICOS_AND_QUEUE_QID = data_id_from_name(
      T_TRAFFIC_CLASS, A_TRAFFIC_CLASS_SET_ICOS_AND_QUEUE, "qid");

  T_PPG = table_id_from_name("ppg_stats.ppg");
  F_PPG_LOCAL_MD_INGRESS_PORT = key_id_from_name(T_PPG, "port");
  F_PPG_IG_INTR_MD_FOR_TM_INGRESS_COS = key_id_from_name(T_PPG, "icos");
  A_PPG_COUNT = action_id_from_name(T_PPG, "ppg_stats.count");
  P_PPG_STATS_BYTES =
      data_id_from_name(T_PPG, A_PPG_COUNT, "$COUNTER_SPEC_BYTES");
  P_PPG_STATS_PKTS =
      data_id_from_name(T_PPG, A_PPG_COUNT, "$COUNTER_SPEC_PKTS");

  T_QUEUE = table_id_from_name("egress_queue.queue");
  F_QUEUE_EG_INTR_MD_PORT = key_id_from_name(T_QUEUE, "port");
  F_QUEUE_LOCAL_MD_QOS_QID = key_id_from_name(T_QUEUE, "qid");
  A_QUEUE_COUNT = action_id_from_name(T_QUEUE, "egress_queue.count");
  P_QUEUE_STATS_BYTES =
      data_id_from_name(T_QUEUE, A_QUEUE_COUNT, "$COUNTER_SPEC_BYTES");
  P_QUEUE_STATS_PKTS =
      data_id_from_name(T_QUEUE, A_QUEUE_COUNT, "$COUNTER_SPEC_PKTS");

  T_L3_QOS_MAP = table_id_from_name("egress_qos.l3_qos_map");
  F_L3_QOS_MAP_PORT = key_id_from_name(T_L3_QOS_MAP, "port");
  F_L3_QOS_MAP_MD_TC = key_id_from_name(T_L3_QOS_MAP, "tc");
  F_L3_QOS_MAP_MD_COLOR = key_id_from_name(T_L3_QOS_MAP, "color");
  F_L3_QOS_MAP_HDR_IPV4_VALID =
      key_id_from_name(T_L3_QOS_MAP, "hdr.ipv4.$valid");
  F_L3_QOS_MAP_HDR_IPV6_VALID =
      key_id_from_name(T_L3_QOS_MAP, "hdr.ipv6.$valid");
  A_L3_QOS_MAP_SET_IPV4_DSCP =
      action_id_from_name(T_L3_QOS_MAP, "egress_qos.set_ipv4_dscp");
  F_L3_QOS_MAP_SET_IPV4_DSCP_DSCP =
      data_id_from_name(T_L3_QOS_MAP, A_L3_QOS_MAP_SET_IPV4_DSCP, "dscp");
  A_L3_QOS_MAP_SET_IPV6_DSCP =
      action_id_from_name(T_L3_QOS_MAP, "egress_qos.set_ipv6_dscp");
  F_L3_QOS_MAP_SET_IPV6_DSCP_DSCP =
      data_id_from_name(T_L3_QOS_MAP, A_L3_QOS_MAP_SET_IPV6_DSCP, "dscp");

  T_L2_QOS_MAP = table_id_from_name("egress_qos.l2_qos_map");
  F_L2_QOS_MAP_PORT = key_id_from_name(T_L2_QOS_MAP, "port");
  F_L2_QOS_MAP_MD_TC = key_id_from_name(T_L2_QOS_MAP, "tc");
  F_L2_QOS_MAP_MD_COLOR = key_id_from_name(T_L2_QOS_MAP, "color");
  A_L2_QOS_MAP_SET_VLAN_PCP =
      action_id_from_name(T_L2_QOS_MAP, "egress_qos.set_vlan_pcp");
  F_L2_QOS_MAP_SET_VLAN_PCP_PCP =
      data_id_from_name(T_L2_QOS_MAP, A_L2_QOS_MAP_SET_VLAN_PCP, "pcp");

  T_INGRESS_PFC_WD_ACL = table_id_from_name("ingress_pfcwd.acl");
  F_INGRESS_PFC_WD_ACL_QID = key_id_from_name(T_INGRESS_PFC_WD_ACL, "qid");
  F_INGRESS_PFC_WD_ACL_PORT = key_id_from_name(T_INGRESS_PFC_WD_ACL, "port");
  A_INGRESS_PFC_WD_ACL_DENY =
      action_id_from_name(T_INGRESS_PFC_WD_ACL, "ingress_pfcwd.acl_deny");
  P_INGRESS_PFC_WD_ACL_STATS_COUNTER_SPEC_PKTS = data_id_from_name(
      T_INGRESS_PFC_WD_ACL, A_INGRESS_PFC_WD_ACL_DENY, "$COUNTER_SPEC_PKTS");
  P_INGRESS_PFC_WD_ACL_STATS_COUNTER_SPEC_BYTES = data_id_from_name(
      T_INGRESS_PFC_WD_ACL, A_INGRESS_PFC_WD_ACL_DENY, "$COUNTER_SPEC_BYTES");

  T_EGRESS_PFC_WD_ACL = table_id_from_name("egress_pfcwd.acl");
  F_EGRESS_PFC_WD_ACL_QID = key_id_from_name(T_EGRESS_PFC_WD_ACL, "qid");
  F_EGRESS_PFC_WD_ACL_PORT = key_id_from_name(T_EGRESS_PFC_WD_ACL, "port");
  A_EGRESS_PFC_WD_ACL_DENY =
      action_id_from_name(T_EGRESS_PFC_WD_ACL, "egress_pfcwd.acl_deny");
  P_EGRESS_PFC_WD_ACL_STATS_COUNTER_SPEC_PKTS = data_id_from_name(
      T_EGRESS_PFC_WD_ACL, A_EGRESS_PFC_WD_ACL_DENY, "$COUNTER_SPEC_PKTS");
  P_EGRESS_PFC_WD_ACL_STATS_COUNTER_SPEC_BYTES = data_id_from_name(
      T_EGRESS_PFC_WD_ACL, A_EGRESS_PFC_WD_ACL_DENY, "$COUNTER_SPEC_BYTES");

  T_ETRAP_IPV4_ACL = table_id_from_name("pipe.etrap_ipv4_flow");
  F_ETRAP_IPV4_ACL_LOCAL_MD_LKP_IP_SRC_ADDR =
      key_id_from_name(T_ETRAP_IPV4_ACL, "src_addr");
  F_ETRAP_IPV4_ACL_LOCAL_MD_LKP_IP_DST_ADDR =
      key_id_from_name(T_ETRAP_IPV4_ACL, "dst_addr");
  A_ETRAP_IPV4_ACL_SET_METER_AND_TC =
      action_id_from_name(T_ETRAP_IPV4_ACL, "set_meter_and_tc");
  P_ETRAP_IPV4_ACL_SET_METER_AND_TC_INDEX = data_id_from_name(
      T_ETRAP_IPV4_ACL, A_ETRAP_IPV4_ACL_SET_METER_AND_TC, "index");
  P_ETRAP_IPV4_ACL_SET_METER_AND_TC_TC = data_id_from_name(
      T_ETRAP_IPV4_ACL, A_ETRAP_IPV4_ACL_SET_METER_AND_TC, "tc");

  T_ETRAP_IPV6_ACL = table_id_from_name("pipe.etrap_ipv6_flow");
  F_ETRAP_IPV6_ACL_LOCAL_MD_LKP_IP_SRC_ADDR =
      key_id_from_name(T_ETRAP_IPV6_ACL, "src_addr");
  F_ETRAP_IPV6_ACL_LOCAL_MD_LKP_IP_DST_ADDR =
      key_id_from_name(T_ETRAP_IPV6_ACL, "dst_addr");
  A_ETRAP_IPV6_ACL_SET_METER_AND_TC =
      action_id_from_name(T_ETRAP_IPV6_ACL, "set_meter_and_tc");
  P_ETRAP_IPV6_ACL_SET_METER_AND_TC_INDEX = data_id_from_name(
      T_ETRAP_IPV6_ACL, A_ETRAP_IPV6_ACL_SET_METER_AND_TC, "index");
  P_ETRAP_IPV6_ACL_SET_METER_AND_TC_TC = data_id_from_name(
      T_ETRAP_IPV6_ACL, A_ETRAP_IPV6_ACL_SET_METER_AND_TC, "tc");

  T_ETRAP_METER = table_id_from_name("SwitchIngress.etrap.meter");
  F_ETRAP_METER_METER_INDEX = key_id_from_name(T_ETRAP_METER, "$METER_INDEX");
  D_ETRAP_METER_METER_SPEC_CIR_KBPS =
      data_id_from_name_noaction(T_ETRAP_METER, "$METER_SPEC_CIR_KBPS");
  D_ETRAP_METER_METER_SPEC_PIR_KBPS =
      data_id_from_name_noaction(T_ETRAP_METER, "$METER_SPEC_PIR_KBPS");
  D_ETRAP_METER_METER_SPEC_CBS_KBITS =
      data_id_from_name_noaction(T_ETRAP_METER, "$METER_SPEC_CBS_KBITS");
  D_ETRAP_METER_METER_SPEC_PBS_KBITS =
      data_id_from_name_noaction(T_ETRAP_METER, "$METER_SPEC_PBS_KBITS");

  T_ETRAP_METER_INDEX = table_id_from_name("pipe.etrap.meter_index");
  F_ETRAP_METER_INDEX_LOCAL_MD_QOS_ETRAP_INDEX =
      key_id_from_name(T_ETRAP_METER_INDEX, "etrap_index");
  A_ETRAP_METER_INDEX_ACTION =
      action_id_from_name(T_ETRAP_METER_INDEX, "meter_action");
  P_ETRAP_METER_INDEX_ACTION_INDEX = data_id_from_name(
      T_ETRAP_METER_INDEX, A_ETRAP_METER_INDEX_ACTION, "index");

  T_ETRAP_METER_STATE = table_id_from_name("SwitchIngress.etrap.meter_state");
  F_ETRAP_METER_STATE_LOCAL_MD_QOS_ETRAP_INDEX =
      key_id_from_name(T_ETRAP_METER_STATE, "etrap_index");
  A_ETRAP_METER_STATE_ACTION = action_id_from_name(
      T_ETRAP_METER_STATE, "SwitchIngress.etrap.meter_state_action");

  T_ETRAP_STATE = table_id_from_name("pipe.etrap_state");
  F_ETRAP_STATE_LOCAL_MD_QOS_ETRAP_COLOR =
      key_id_from_name(T_ETRAP_STATE, "etrap_color");
  A_ETRAP_STATE_ETRAP_RED_STATE =
      action_id_from_name(T_ETRAP_STATE, "etrap_red_state");
  A_ETRAP_STATE_ETRAP_GREEN_STATE =
      action_id_from_name(T_ETRAP_STATE, "etrap_green_state");

  T_ETRAP_STATE_REG =
      table_id_from_name("SwitchIngress.etrap_state.etrap_state_reg");
  F_ETRAP_STATE_REG_REGISTER_INDEX =
      key_id_from_name(T_ETRAP_STATE_REG, "$REGISTER_INDEX");
  D_ETRAP_STATE_REG_RESULT_VALUE = data_id_from_name_noaction(
      T_ETRAP_STATE_REG, "SwitchIngress.etrap_state.etrap_state_reg.f1");

  /* meter.p4 */
  T_STORM_CONTROL_METER = table_id_from_name("storm_control.meter");
  F_STORM_CONTROL_METER_METER_INDEX =
      key_id_from_name(T_STORM_CONTROL_METER, "$METER_INDEX");
  D_STORM_CONTROL_METER_METER_SPEC_CIR_KBPS =
      data_id_from_name_noaction(T_STORM_CONTROL_METER, "$METER_SPEC_CIR_KBPS");
  D_STORM_CONTROL_METER_METER_SPEC_PIR_KBPS =
      data_id_from_name_noaction(T_STORM_CONTROL_METER, "$METER_SPEC_PIR_KBPS");
  D_STORM_CONTROL_METER_METER_SPEC_CBS_KBITS = data_id_from_name_noaction(
      T_STORM_CONTROL_METER, "$METER_SPEC_CBS_KBITS");
  D_STORM_CONTROL_METER_METER_SPEC_PBS_KBITS = data_id_from_name_noaction(
      T_STORM_CONTROL_METER, "$METER_SPEC_PBS_KBITS");

  T_STORM_CONTROL_STATS = table_id_from_name("storm_control.stats");
  F_STORM_CONTROL_STATS_COLOR = key_id_from_name(
      T_STORM_CONTROL_STATS, "local_md.qos.storm_control_color");
  F_STORM_CONTROL_STATS_PKT_TYPE =
      key_id_from_name(T_STORM_CONTROL_STATS, "pkt_type");
  F_STORM_CONTROL_STATS_LOCAL_MD_INGRESS_PORT =
      key_id_from_name(T_STORM_CONTROL_STATS, "local_md.ingress_port");
  F_STORM_CONTROL_STATS_DMAC_MISS =
      key_id_from_name(T_STORM_CONTROL_STATS, "local_md.flags.dmac_miss");
  F_STORM_CONTROL_STATS_MULTICAST_HIT =
      key_id_from_name(T_STORM_CONTROL_STATS, "local_md.multicast.hit");
  A_STORM_CONTROL_STATS_COUNT =
      action_id_from_name(T_STORM_CONTROL_STATS, "storm_control.count");
  A_STORM_CONTROL_STATS_DROP_AND_COUNT = action_id_from_name(
      T_STORM_CONTROL_STATS, "storm_control.drop_and_count");
  D_STORM_CONTROL_STATS_COUNTER_SPEC_PKTS = data_id_from_name(
      T_STORM_CONTROL_STATS, A_STORM_CONTROL_STATS_COUNT, "$COUNTER_SPEC_PKTS");
  D_STORM_CONTROL_STATS_COUNTER_SPEC_BYTES =
      data_id_from_name(T_STORM_CONTROL_STATS,
                        A_STORM_CONTROL_STATS_COUNT,
                        "$COUNTER_SPEC_BYTES");

  T_STORM_CONTROL = table_id_from_name("storm_control.storm_control");
  F_STORM_CONTROL_LOCAL_MD_INGRESS_PORT =
      key_id_from_name(T_STORM_CONTROL, "local_md.ingress_port");
  F_STORM_CONTROL_PKT_TYPE = key_id_from_name(T_STORM_CONTROL, "pkt_type");
  F_STORM_CONTROL_DMAC_MISS =
      key_id_from_name(T_STORM_CONTROL, "local_md.flags.dmac_miss");
  F_STORM_CONTROL_MULTICAST_HIT =
      key_id_from_name(T_STORM_CONTROL, "local_md.multicast.hit");
  A_STORM_CONTROL_SET_METER =
      action_id_from_name(T_STORM_CONTROL, "storm_control.set_meter");
  P_STORM_CONTROL_SET_METER_INDEX =
      data_id_from_name(T_STORM_CONTROL, A_STORM_CONTROL_SET_METER, "index");

  T_INGRESS_MIRROR_METER_INDEX =
      table_id_from_name("ingress_mirror_meter.meter_index");
  F_INGRESS_MIRROR_METER_INDEX_LOCAL_MD_MIRROR_METER_INDEX = key_id_from_name(
      T_INGRESS_MIRROR_METER_INDEX, "local_md.mirror.meter_index");
  A_SET_INGRESS_MIRROR_METER = action_id_from_name(
      T_INGRESS_MIRROR_METER_INDEX, "ingress_mirror_meter.set_meter");
  D_SET_INGRESS_MIRROR_METER_INDEX = data_id_from_name(
      T_INGRESS_MIRROR_METER_INDEX, A_SET_INGRESS_MIRROR_METER, "index");

  T_INGRESS_MIRROR_METER_ACTION =
      table_id_from_name("ingress_mirror_meter.meter_action");
  F_INGRESS_MIRROR_METER_ACTION_COLOR =
      key_id_from_name(T_INGRESS_MIRROR_METER_ACTION, "color");
  F_INGRESS_MIRROR_METER_ACTION_LOCAL_MD_MIRROR_METER_INDEX = key_id_from_name(
      T_INGRESS_MIRROR_METER_ACTION, "local_md.mirror.meter_index");
  A_INGRESS_MIRROR_METER_MIRROR_AND_COUNT = action_id_from_name(
      T_INGRESS_MIRROR_METER_ACTION, "ingress_mirror_meter.mirror_and_count");
  A_INGRESS_MIRROR_METER_NO_MIRROR_AND_COUNT =
      action_id_from_name(T_INGRESS_MIRROR_METER_ACTION,
                          "ingress_mirror_meter.no_mirror_and_count");
  D_INGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_PKTS =
      data_id_from_name(T_INGRESS_MIRROR_METER_ACTION,
                        A_INGRESS_MIRROR_METER_MIRROR_AND_COUNT,
                        "$COUNTER_SPEC_PKTS");
  D_INGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_BYTES =
      data_id_from_name(T_INGRESS_MIRROR_METER_ACTION,
                        A_INGRESS_MIRROR_METER_MIRROR_AND_COUNT,
                        "$COUNTER_SPEC_BYTES");

  T_INGRESS_MIRROR_METER = table_id_from_name("ingress_mirror_meter.meter");
  F_INGRESS_MIRROR_METER_METER_INDEX =
      key_id_from_name(T_INGRESS_MIRROR_METER, "$METER_INDEX");
  D_INGRESS_MIRROR_METER_METER_SPEC_CIR_PPS =
      data_id_from_name_noaction(T_INGRESS_MIRROR_METER, "$METER_SPEC_CIR_PPS");
  D_INGRESS_MIRROR_METER_METER_SPEC_PIR_PPS =
      data_id_from_name_noaction(T_INGRESS_MIRROR_METER, "$METER_SPEC_PIR_PPS");
  D_INGRESS_MIRROR_METER_METER_SPEC_CBS_PKTS = data_id_from_name_noaction(
      T_INGRESS_MIRROR_METER, "$METER_SPEC_CBS_PKTS");
  D_INGRESS_MIRROR_METER_METER_SPEC_PBS_PKTS = data_id_from_name_noaction(
      T_INGRESS_MIRROR_METER, "$METER_SPEC_PBS_PKTS");

  T_EGRESS_MIRROR_METER_INDEX =
      table_id_from_name("egress_mirror_meter.meter_index");
  F_EGRESS_MIRROR_METER_INDEX_LOCAL_MD_MIRROR_METER_INDEX = key_id_from_name(
      T_EGRESS_MIRROR_METER_INDEX, "local_md.mirror.meter_index");
  A_SET_EGRESS_MIRROR_METER = action_id_from_name(
      T_EGRESS_MIRROR_METER_INDEX, "egress_mirror_meter.set_meter");
  D_SET_EGRESS_MIRROR_METER_INDEX = data_id_from_name(
      T_EGRESS_MIRROR_METER_INDEX, A_SET_EGRESS_MIRROR_METER, "index");

  T_EGRESS_MIRROR_METER_ACTION =
      table_id_from_name("egress_mirror_meter.meter_action");
  F_EGRESS_MIRROR_METER_ACTION_COLOR =
      key_id_from_name(T_EGRESS_MIRROR_METER_ACTION, "color");
  F_EGRESS_MIRROR_METER_ACTION_LOCAL_MD_MIRROR_METER_INDEX = key_id_from_name(
      T_EGRESS_MIRROR_METER_ACTION, "local_md.mirror.meter_index");
  A_EGRESS_MIRROR_METER_MIRROR_AND_COUNT = action_id_from_name(
      T_EGRESS_MIRROR_METER_ACTION, "egress_mirror_meter.mirror_and_count");
  A_EGRESS_MIRROR_METER_NO_MIRROR_AND_COUNT = action_id_from_name(
      T_EGRESS_MIRROR_METER_ACTION, "egress_mirror_meter.no_mirror_and_count");
  D_EGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_PKTS =
      data_id_from_name(T_EGRESS_MIRROR_METER_ACTION,
                        A_EGRESS_MIRROR_METER_MIRROR_AND_COUNT,
                        "$COUNTER_SPEC_PKTS");
  D_EGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_BYTES =
      data_id_from_name(T_EGRESS_MIRROR_METER_ACTION,
                        A_EGRESS_MIRROR_METER_MIRROR_AND_COUNT,
                        "$COUNTER_SPEC_BYTES");

  T_EGRESS_MIRROR_METER = table_id_from_name("egress_mirror_meter.meter");
  F_EGRESS_MIRROR_METER_METER_INDEX =
      key_id_from_name(T_EGRESS_MIRROR_METER, "$METER_INDEX");
  D_EGRESS_MIRROR_METER_METER_SPEC_CIR_PPS =
      data_id_from_name_noaction(T_EGRESS_MIRROR_METER, "$METER_SPEC_CIR_PPS");
  D_EGRESS_MIRROR_METER_METER_SPEC_PIR_PPS =
      data_id_from_name_noaction(T_EGRESS_MIRROR_METER, "$METER_SPEC_PIR_PPS");
  D_EGRESS_MIRROR_METER_METER_SPEC_CBS_PKTS =
      data_id_from_name_noaction(T_EGRESS_MIRROR_METER, "$METER_SPEC_CBS_PKTS");
  D_EGRESS_MIRROR_METER_METER_SPEC_PBS_PKTS =
      data_id_from_name_noaction(T_EGRESS_MIRROR_METER, "$METER_SPEC_PBS_PKTS");

  T_INGRESS_ACL_METER_INDEX =
      table_id_from_name("ingress_acl_meter.meter_index");
  F_INGRESS_ACL_METER_INDEX_LOCAL_MD_QOS_ACL_METER_INDEX = key_id_from_name(
      T_INGRESS_ACL_METER_INDEX, "local_md.qos.acl_meter_index");
  A_SET_INGRESS_ACL_METER = action_id_from_name(T_INGRESS_ACL_METER_INDEX,
                                                "ingress_acl_meter.set_meter");
  D_SET_INGRESS_ACL_METER_INDEX = data_id_from_name(
      T_INGRESS_ACL_METER_INDEX, A_SET_INGRESS_ACL_METER, "index");

  T_INGRESS_ACL_METER_ACTION =
      table_id_from_name("ingress_acl_meter.meter_action");
  F_INGRESS_ACL_METER_ACTION_COLOR =
      key_id_from_name(T_INGRESS_ACL_METER_ACTION, "color");
  F_INGRESS_ACL_METER_ACTION_LOCAL_MD_QOS_ACL_METER_INDEX = key_id_from_name(
      T_INGRESS_ACL_METER_ACTION, "local_md.qos.acl_meter_index");
  A_INGRESS_ACL_METER_COUNT = action_id_from_name(T_INGRESS_ACL_METER_ACTION,
                                                  "ingress_acl_meter.count");
  D_INGRESS_ACL_METER_COUNT_PACKET_ACTION = data_id_from_name(
      T_INGRESS_ACL_METER_ACTION, A_INGRESS_ACL_METER_COUNT, "packet_action");
  D_INGRESS_ACL_METER_ACTION_COUNTER_SPEC_PKTS =
      data_id_from_name(T_INGRESS_ACL_METER_ACTION,
                        A_INGRESS_ACL_METER_COUNT,
                        "$COUNTER_SPEC_PKTS");
  D_INGRESS_ACL_METER_ACTION_COUNTER_SPEC_BYTES =
      data_id_from_name(T_INGRESS_ACL_METER_ACTION,
                        A_INGRESS_ACL_METER_COUNT,
                        "$COUNTER_SPEC_BYTES");

  T_INGRESS_ACL_METER = table_id_from_name("ingress_acl_meter.meter");
  F_INGRESS_ACL_METER_METER_INDEX =
      key_id_from_name(T_INGRESS_ACL_METER, "$METER_INDEX");
  D_INGRESS_ACL_METER_METER_SPEC_CIR_KBPS =
      data_id_from_name_noaction(T_INGRESS_ACL_METER, "$METER_SPEC_CIR_KBPS");
  D_INGRESS_ACL_METER_METER_SPEC_PIR_KBPS =
      data_id_from_name_noaction(T_INGRESS_ACL_METER, "$METER_SPEC_PIR_KBPS");
  D_INGRESS_ACL_METER_METER_SPEC_CBS_KBITS =
      data_id_from_name_noaction(T_INGRESS_ACL_METER, "$METER_SPEC_CBS_KBITS");
  D_INGRESS_ACL_METER_METER_SPEC_PBS_KBITS =
      data_id_from_name_noaction(T_INGRESS_ACL_METER, "$METER_SPEC_PBS_KBITS");

  T_EGRESS_ACL_METER_INDEX = table_id_from_name("egress_acl_meter.meter_index");
  F_EGRESS_ACL_METER_INDEX_LOCAL_MD_QOS_ACL_METER_INDEX = key_id_from_name(
      T_EGRESS_ACL_METER_INDEX, "local_md.qos.acl_meter_index");
  A_SET_EGRESS_ACL_METER = action_id_from_name(T_EGRESS_ACL_METER_INDEX,
                                               "egress_acl_meter.set_meter");
  D_SET_EGRESS_ACL_METER_INDEX = data_id_from_name(
      T_EGRESS_ACL_METER_INDEX, A_SET_EGRESS_ACL_METER, "index");

  T_EGRESS_ACL_METER_ACTION =
      table_id_from_name("egress_acl_meter.meter_action");
  F_EGRESS_ACL_METER_ACTION_COLOR =
      key_id_from_name(T_EGRESS_ACL_METER_ACTION, "color");
  F_EGRESS_ACL_METER_ACTION_LOCAL_MD_QOS_ACL_METER_INDEX = key_id_from_name(
      T_EGRESS_ACL_METER_ACTION, "local_md.qos.acl_meter_index");
  A_EGRESS_ACL_METER_COUNT =
      action_id_from_name(T_EGRESS_ACL_METER_ACTION, "egress_acl_meter.count");
  D_EGRESS_ACL_METER_COUNT_PACKET_ACTION = data_id_from_name(
      T_EGRESS_ACL_METER_ACTION, A_EGRESS_ACL_METER_COUNT, "packet_action");
  D_EGRESS_ACL_METER_ACTION_COUNTER_SPEC_PKTS =
      data_id_from_name(T_EGRESS_ACL_METER_ACTION,
                        A_EGRESS_ACL_METER_COUNT,
                        "$COUNTER_SPEC_PKTS");
  D_EGRESS_ACL_METER_ACTION_COUNTER_SPEC_BYTES =
      data_id_from_name(T_EGRESS_ACL_METER_ACTION,
                        A_EGRESS_ACL_METER_COUNT,
                        "$COUNTER_SPEC_BYTES");

  T_EGRESS_ACL_METER = table_id_from_name("egress_acl_meter.meter");
  F_EGRESS_ACL_METER_METER_INDEX =
      key_id_from_name(T_EGRESS_ACL_METER, "$METER_INDEX");
  D_EGRESS_ACL_METER_METER_SPEC_CIR_KBPS =
      data_id_from_name_noaction(T_EGRESS_ACL_METER, "$METER_SPEC_CIR_KBPS");
  D_EGRESS_ACL_METER_METER_SPEC_PIR_KBPS =
      data_id_from_name_noaction(T_EGRESS_ACL_METER, "$METER_SPEC_PIR_KBPS");
  D_EGRESS_ACL_METER_METER_SPEC_CBS_KBITS =
      data_id_from_name_noaction(T_EGRESS_ACL_METER, "$METER_SPEC_CBS_KBITS");
  D_EGRESS_ACL_METER_METER_SPEC_PBS_KBITS =
      data_id_from_name_noaction(T_EGRESS_ACL_METER, "$METER_SPEC_PBS_KBITS");

  T_INGRESS_PORT_METER_INDEX =
      table_id_from_name("ingress_port_meter.meter_index");
  F_INGRESS_PORT_METER_INDEX_LOCAL_MD_QOS_PORT_METER_INDEX = key_id_from_name(
      T_INGRESS_PORT_METER_INDEX, "local_md.qos.port_meter_index");
  A_SET_INGRESS_PORT_METER = action_id_from_name(
      T_INGRESS_PORT_METER_INDEX, "ingress_port_meter.set_meter");
  D_SET_INGRESS_PORT_METER_INDEX = data_id_from_name(
      T_INGRESS_PORT_METER_INDEX, A_SET_INGRESS_PORT_METER, "index");

  T_INGRESS_PORT_METER_ACTION =
      table_id_from_name("ingress_port_meter.meter_action");
  F_INGRESS_PORT_METER_ACTION_COLOR =
      key_id_from_name(T_INGRESS_PORT_METER_ACTION, "color");
  F_INGRESS_PORT_METER_ACTION_LOCAL_MD_QOS_PORT_METER_INDEX = key_id_from_name(
      T_INGRESS_PORT_METER_ACTION, "local_md.qos.port_meter_index");
  A_INGRESS_PORT_METER_COUNT = action_id_from_name(T_INGRESS_PORT_METER_ACTION,
                                                   "ingress_port_meter.count");
  A_INGRESS_PORT_METER_DROP_AND_COUNT = action_id_from_name(
      T_INGRESS_PORT_METER_ACTION, "ingress_port_meter.drop_and_count");
  D_INGRESS_PORT_METER_ACTION_COUNTER_SPEC_PKTS =
      data_id_from_name(T_INGRESS_PORT_METER_ACTION,
                        A_INGRESS_PORT_METER_COUNT,
                        "$COUNTER_SPEC_PKTS");
  D_INGRESS_PORT_METER_ACTION_COUNTER_SPEC_BYTES =
      data_id_from_name(T_INGRESS_PORT_METER_ACTION,
                        A_INGRESS_PORT_METER_COUNT,
                        "$COUNTER_SPEC_BYTES");

  T_INGRESS_PORT_METER = table_id_from_name("ingress_port_meter.meter");
  F_INGRESS_PORT_METER_METER_INDEX =
      key_id_from_name(T_INGRESS_PORT_METER, "$METER_INDEX");
  D_INGRESS_PORT_METER_METER_SPEC_CIR_KBPS =
      data_id_from_name_noaction(T_INGRESS_PORT_METER, "$METER_SPEC_CIR_KBPS");
  D_INGRESS_PORT_METER_METER_SPEC_PIR_KBPS =
      data_id_from_name_noaction(T_INGRESS_PORT_METER, "$METER_SPEC_PIR_KBPS");
  D_INGRESS_PORT_METER_METER_SPEC_CBS_KBITS =
      data_id_from_name_noaction(T_INGRESS_PORT_METER, "$METER_SPEC_CBS_KBITS");
  D_INGRESS_PORT_METER_METER_SPEC_PBS_KBITS =
      data_id_from_name_noaction(T_INGRESS_PORT_METER, "$METER_SPEC_PBS_KBITS");

  T_EGRESS_PORT_METER_INDEX =
      table_id_from_name("egress_port_meter.meter_index");
  F_EGRESS_PORT_METER_INDEX_LOCAL_MD_QOS_PORT_METER_INDEX = key_id_from_name(
      T_EGRESS_PORT_METER_INDEX, "local_md.qos.port_meter_index");
  A_SET_EGRESS_PORT_METER = action_id_from_name(T_EGRESS_PORT_METER_INDEX,
                                                "egress_port_meter.set_meter");
  D_SET_EGRESS_PORT_METER_INDEX = data_id_from_name(
      T_EGRESS_PORT_METER_INDEX, A_SET_EGRESS_PORT_METER, "index");

  T_EGRESS_PORT_METER_ACTION =
      table_id_from_name("egress_port_meter.meter_action");
  F_EGRESS_PORT_METER_ACTION_COLOR =
      key_id_from_name(T_EGRESS_PORT_METER_ACTION, "color");
  F_EGRESS_PORT_METER_ACTION_LOCAL_MD_QOS_PORT_METER_INDEX = key_id_from_name(
      T_EGRESS_PORT_METER_ACTION, "local_md.qos.port_meter_index");
  A_EGRESS_PORT_METER_COUNT = action_id_from_name(T_EGRESS_PORT_METER_ACTION,
                                                  "egress_port_meter.count");
  A_EGRESS_PORT_METER_DROP_AND_COUNT = action_id_from_name(
      T_EGRESS_PORT_METER_ACTION, "egress_port_meter.drop_and_count");
  D_EGRESS_PORT_METER_ACTION_COUNTER_SPEC_PKTS =
      data_id_from_name(T_EGRESS_PORT_METER_ACTION,
                        A_EGRESS_PORT_METER_COUNT,
                        "$COUNTER_SPEC_PKTS");
  D_EGRESS_PORT_METER_ACTION_COUNTER_SPEC_BYTES =
      data_id_from_name(T_EGRESS_PORT_METER_ACTION,
                        A_EGRESS_PORT_METER_DROP_AND_COUNT,
                        "$COUNTER_SPEC_BYTES");

  T_EGRESS_PORT_METER = table_id_from_name("egress_port_meter.meter");
  F_EGRESS_PORT_METER_METER_INDEX =
      key_id_from_name(T_EGRESS_PORT_METER, "$METER_INDEX");
  D_EGRESS_PORT_METER_METER_SPEC_CIR_KBPS =
      data_id_from_name_noaction(T_EGRESS_PORT_METER, "$METER_SPEC_CIR_KBPS");
  D_EGRESS_PORT_METER_METER_SPEC_PIR_KBPS =
      data_id_from_name_noaction(T_EGRESS_PORT_METER, "$METER_SPEC_PIR_KBPS");
  D_EGRESS_PORT_METER_METER_SPEC_CBS_KBITS =
      data_id_from_name_noaction(T_EGRESS_PORT_METER, "$METER_SPEC_CBS_KBITS");
  D_EGRESS_PORT_METER_METER_SPEC_PBS_KBITS =
      data_id_from_name_noaction(T_EGRESS_PORT_METER, "$METER_SPEC_PBS_KBITS");

  /* wred.p4 */
  T_ECN = table_id_from_name("ecn_acl.acl");
  F_ECN_LOCAL_MD_INGRESS_PORT_LAG_LABEL =
      key_id_from_name(T_ECN, "local_md.ingress_port_lag_label");
  F_ECN_LKP_IP_TOS = key_id_from_name(T_ECN, "lkp.ip_tos");
  F_ECN_LKP_TCP_FLAGS = key_id_from_name(T_ECN, "lkp.tcp_flags");
  A_ECN_SET_INGRESS_COLOR =
      action_id_from_name(T_ECN, "ecn_acl.set_ingress_color");
  D_ECN_SET_INGRESS_COLOR_COLOR =
      data_id_from_name(T_ECN, A_ECN_SET_INGRESS_COLOR, "color");

  T_WRED_SESSION = table_id_from_name("egress_wred.wred");
  F_WRED_SESSION_INDEX = key_id_from_name(T_WRED_SESSION, "$WRED_INDEX");
  D_WRED_SESSION_TIME_CONSTANT =
      data_id_from_name_noaction(T_WRED_SESSION, "$WRED_SPEC_TIME_CONSTANT_NS");
  D_WRED_SESSION_MIN_THRES =
      data_id_from_name_noaction(T_WRED_SESSION, "$WRED_SPEC_MIN_THRESH_CELLS");
  D_WRED_SESSION_MAX_THRES =
      data_id_from_name_noaction(T_WRED_SESSION, "$WRED_SPEC_MAX_THRESH_CELLS");
  D_WRED_SESSION_MAX_PROB =
      data_id_from_name_noaction(T_WRED_SESSION, "$WRED_SPEC_MAX_PROBABILITY");

  T_V4_WRED_ACTION = table_id_from_name("egress_wred.v4_wred_action");
  F_V4_WRED_ACTION_INDEX = key_id_from_name(T_V4_WRED_ACTION, "index");
  F_V4_WRED_ACTION_HDR_IPV4_ECN =
      key_id_from_name(T_V4_WRED_ACTION, "hdr.ipv4.diffserv[1:0]");
  A_WRED_ACTION_DROP =
      action_id_from_name(T_V4_WRED_ACTION, "egress_wred.drop");
  A_WRED_ACTION_SET_IPV4_ECN =
      action_id_from_name(T_V4_WRED_ACTION, "egress_wred.set_ipv4_ecn");

  T_V6_WRED_ACTION = table_id_from_name("egress_wred.v6_wred_action");
  F_V6_WRED_ACTION_INDEX = key_id_from_name(T_V6_WRED_ACTION, "index");
  F_V6_WRED_ACTION_HDR_IPV6_ECN =
      key_id_from_name(T_V6_WRED_ACTION, "hdr.ipv6.traffic_class[1:0]");
  // A_WRED_ACTION_DROP =
  //    action_id_from_name(T_V6_WRED_ACTION, "SwitchEgress.wred.drop");
  A_WRED_ACTION_SET_IPV6_ECN =
      action_id_from_name(T_V6_WRED_ACTION, "egress_wred.set_ipv6_ecn");

  T_WRED_INDEX = table_id_from_name("egress_wred.wred_index");
  F_WRED_INDEX_EG_INTR_MD_EGRESS_PORT = key_id_from_name(T_WRED_INDEX, "port");
  F_WRED_INDEX_QOS_MD_QID = key_id_from_name(T_WRED_INDEX, "qid");
  F_WRED_INDEX_QOS_MD_COLOR = key_id_from_name(T_WRED_INDEX, "color");
  A_WRED_INDEX_SET_WRED_INDEX =
      action_id_from_name(T_WRED_INDEX, "egress_wred.set_wred_index");
  D_SET_WRED_INDEX_WRED_INDEX = data_id_from_name(
      T_WRED_INDEX, A_WRED_INDEX_SET_WRED_INDEX, "wred_index");

  T_EGRESS_WRED_STATS = table_id_from_name("egress_wred.wred_stats");
  F_EGRESS_WRED_STATS_PORT = key_id_from_name(T_EGRESS_WRED_STATS, "port");
  F_EGRESS_WRED_STATS_QID = key_id_from_name(T_EGRESS_WRED_STATS, "qid");
  F_EGRESS_WRED_STATS_COLOR = key_id_from_name(T_EGRESS_WRED_STATS, "color");
  F_EGRESS_WRED_STATS_WRED_DROP =
      key_id_from_name(T_EGRESS_WRED_STATS, "wred_drop");
  A_EGRESS_WRED_STATS_COUNT =
      action_id_from_name(T_EGRESS_WRED_STATS, "egress_wred.count");
  D_EGRESS_WRED_STATS_COUNTER_SPEC_BYTES = data_id_from_name(
      T_EGRESS_WRED_STATS, A_EGRESS_WRED_STATS_COUNT, "$COUNTER_SPEC_BYTES");
  D_EGRESS_WRED_STATS_COUNTER_SPEC_PKTS = data_id_from_name(
      T_EGRESS_WRED_STATS, A_EGRESS_WRED_STATS_COUNT, "$COUNTER_SPEC_PKTS");

  /* dtel.p4 */
  T_QUEUE_REPORT_QUOTAS = table_id_from_name("queue_report.quotas");
  F_QUOTAS_REGISTER_INDEX =
      key_id_from_name(T_QUEUE_REPORT_QUOTAS, "$REGISTER_INDEX");
  D_QUOTAS_COUNTER = data_id_from_name_noaction(T_QUEUE_REPORT_QUOTAS,
                                                "queue_report.quotas.counter");
  D_QUOTAS_LATENCY = data_id_from_name_noaction(T_QUEUE_REPORT_QUOTAS,
                                                "queue_report.quotas.latency");

  T_QUEUE_REPORT_THRESHOLDS = table_id_from_name("queue_report.thresholds");
  F_THRESHOLDS_REGISTER_INDEX =
      key_id_from_name(T_QUEUE_REPORT_THRESHOLDS, "$REGISTER_INDEX");
  D_THRESHOLDS_QDEPTH = data_id_from_name_noaction(
      T_QUEUE_REPORT_THRESHOLDS, "queue_report.thresholds.qdepth");
  D_THRESHOLDS_LATENCY = data_id_from_name_noaction(
      T_QUEUE_REPORT_THRESHOLDS, "queue_report.thresholds.latency");

  T_QUEUE_REPORT_ALERT = table_id_from_name("queue_report.queue_alert");
  F_QUEUE_REPORT_ALERT_QID = key_id_from_name(T_QUEUE_REPORT_ALERT, "qid");
  F_QUEUE_REPORT_ALERT_PORT = key_id_from_name(T_QUEUE_REPORT_ALERT, "port");
  A_SET_QALERT =
      action_id_from_name(T_QUEUE_REPORT_ALERT, "dtel.queue_report.set_qalert");
  D_SET_QALERT_INDEX =
      data_id_from_name(T_QUEUE_REPORT_ALERT, A_SET_QALERT, "index");
  D_SET_QALERT_QUOTA =
      data_id_from_name(T_QUEUE_REPORT_ALERT, A_SET_QALERT, "quota");
  D_SET_QALERT_QUANTIZATION_MASK = data_id_from_name(
      T_QUEUE_REPORT_ALERT, A_SET_QALERT, "quantization_mask");
  A_SET_QMASK =
      action_id_from_name(T_QUEUE_REPORT_ALERT, "dtel.queue_report.set_qmask");
  D_SET_QMASK_QUANTIZATION_MASK =
      data_id_from_name(T_QUEUE_REPORT_ALERT, A_SET_QMASK, "quantization_mask");

  T_QUEUE_REPORT_CHECK_QUOTA = table_id_from_name("queue_report.check_quota");
  F_CHECK_QUOTA_PKT_SRC =
      key_id_from_name(T_QUEUE_REPORT_CHECK_QUOTA, "local_md.pkt_src");
  F_CHECK_QUOTA_QALERT = key_id_from_name(T_QUEUE_REPORT_CHECK_QUOTA, "qalert");
  F_CHECK_QUOTA_QID = key_id_from_name(T_QUEUE_REPORT_CHECK_QUOTA, "qid");
  F_CHECK_QUOTA_PORT = key_id_from_name(T_QUEUE_REPORT_CHECK_QUOTA, "port");
  A_RESET_QUOTA = action_id_from_name(T_QUEUE_REPORT_CHECK_QUOTA,
                                      "dtel.queue_report.reset_quota_");
  D_RESET_QUOTA_INDEX =
      data_id_from_name(T_QUEUE_REPORT_CHECK_QUOTA, A_RESET_QUOTA, "index");
  A_UPDATE_QUOTA = action_id_from_name(T_QUEUE_REPORT_CHECK_QUOTA,
                                       "dtel.queue_report.update_quota_");
  D_UPDATE_QUOTA_INDEX =
      data_id_from_name(T_QUEUE_REPORT_CHECK_QUOTA, A_UPDATE_QUOTA, "index");
  A_CHECK_LATENCY_UPDATE_QUOTA =
      action_id_from_name(T_QUEUE_REPORT_CHECK_QUOTA,
                          "dtel.queue_report.check_latency_and_update_quota_");
  D_CHECK_LATENCY_UPDATE_QUOTA_INDEX = data_id_from_name(
      T_QUEUE_REPORT_CHECK_QUOTA, A_CHECK_LATENCY_UPDATE_QUOTA, "index");

  T_MOD_CONFIG = table_id_from_name("mirror_on_drop.config");
  F_MOD_CONFIG_DROP_REASON = key_id_from_name(T_MOD_CONFIG, "drop_reason");
  F_MOD_CONFIG_DTEL_MD_REPORT_TYPE =
      key_id_from_name(T_MOD_CONFIG, "dtel_md.report_type");
  F_MOD_CONFIG_PRIORITY = key_id_from_name(T_MOD_CONFIG, "$MATCH_PRIORITY");
  A_MOD_CONFIG_MIRROR = action_id_from_name(T_MOD_CONFIG, "dtel.mod.mirror");
  A_MOD_CONFIG_MIRROR_AND_SET_D_BIT =
      action_id_from_name(T_MOD_CONFIG, "dtel.mod.mirror_and_set_d_bit");

  T_DOD_CONFIG = table_id_from_name("deflect_on_drop.config");
  F_DOD_CONFIG_LOCAL_MD_DTEL_REPORT_TYPE =
      key_id_from_name(T_DOD_CONFIG, "local_md.dtel.report_type");
  F_DOD_CONFIG_EGRESS_PORT = key_id_from_name(T_DOD_CONFIG, "egress_port");
  F_DOD_CONFIG_QID = key_id_from_name(T_DOD_CONFIG, "qid");
  F_DOD_CONFIG_LOCAL_MD_MULTICAST_ID =
      key_id_from_name(T_DOD_CONFIG, "local_md.multicast.id");
  F_DOD_CONFIG_LOCAL_MD_CPU_REASON =
      key_id_from_name(T_DOD_CONFIG, "local_md.cpu_reason");
  F_DOD_CONFIG_PRIORITY = key_id_from_name(T_DOD_CONFIG, "$MATCH_PRIORITY");
  A_DOD_CONFIG_ENABLE_DOD =
      action_id_from_name(T_DOD_CONFIG, "dtel.dod.enable_dod");
  A_DOD_CONFIG_DISABLE_DOD =
      action_id_from_name(T_DOD_CONFIG, "dtel.dod.disable_dod");

  T_DTEL_MIRROR_SESSION = table_id_from_name("dtel.mirror_session");
  F_DTEL_MIRROR_SESSION_HDR_ETHERNET_VALID =
      key_id_from_name(T_DTEL_MIRROR_SESSION, "hdr.ethernet.$valid");
  D_DTEL_MIRROR_SESSION_ACTION_MEMBER_ID =
      data_id_from_name_noaction(T_DTEL_MIRROR_SESSION, "$ACTION_MEMBER_ID");
  D_DTEL_MIRROR_SESSION_SELECTOR_GROUP_ID =
      data_id_from_name_noaction(T_DTEL_MIRROR_SESSION, "$SELECTOR_GROUP_ID");

  SG_SESSION_SELECTOR_GROUP = table_id_from_name("dtel.session_selector");
  F_SESSION_SELECTOR_GROUP_ID =
      key_id_from_name(SG_SESSION_SELECTOR_GROUP, "$SELECTOR_GROUP_ID");
  P_SESSION_SELECTOR_GROUP_MAX_GROUP_SIZE =
      data_id_from_name_noaction(SG_SESSION_SELECTOR_GROUP, "$MAX_GROUP_SIZE");
  P_SESSION_SELECTOR_GROUP_ACTION_MEMBER_ID_ARRAY = data_id_from_name_noaction(
      SG_SESSION_SELECTOR_GROUP, "$ACTION_MEMBER_ID");
  P_SESSION_SELECTOR_GROUP_ACTION_MEMBER_STATUS_ARRAY =
      data_id_from_name_noaction(SG_SESSION_SELECTOR_GROUP,
                                 "$ACTION_MEMBER_STATUS");

  AP_SESSION_SELECTOR = table_id_from_name("dtel.dtel_action_profile");
  F_SESSION_SELECTOR_ACTION_MEMBER_ID =
      key_id_from_name(AP_SESSION_SELECTOR, "$ACTION_MEMBER_ID");
  A_SET_MIRROR_SESSION =
      action_id_from_name(AP_SESSION_SELECTOR, "dtel.set_mirror_session");
  P_SET_MIRROR_SESSION_SESSION_ID = data_id_from_name(
      AP_SESSION_SELECTOR, A_SET_MIRROR_SESSION, "session_id");

  /* Egress */
  T_DTEL_CONFIG = table_id_from_name("dtel_config.config");
  F_DTEL_CONFIG_PKT_SRC = key_id_from_name(T_DTEL_CONFIG, "local_md.pkt_src");
  F_DTEL_CONFIG_REPORT_TYPE =
      key_id_from_name(T_DTEL_CONFIG, "local_md.dtel.report_type");
  F_DTEL_CONFIG_DROP_REPORT_FLAG =
      key_id_from_name(T_DTEL_CONFIG, "local_md.dtel.drop_report_flag");
  F_DTEL_CONFIG_FLOW_REPORT_FLAG =
      key_id_from_name(T_DTEL_CONFIG, "local_md.dtel.flow_report_flag");
  F_DTEL_CONFIG_QUEUE_REPORT_FLAG =
      key_id_from_name(T_DTEL_CONFIG, "local_md.dtel.queue_report_flag");
  F_DTEL_CONFIG_DROP_REASON =
      key_id_from_name(T_DTEL_CONFIG, "local_md.drop_reason");
  F_DTEL_CONFIG_MIRROR_TYPE =
      key_id_from_name(T_DTEL_CONFIG, "local_md.mirror.type");
  F_DTEL_CONFIG_DROP_HDR_VALID =
      key_id_from_name(T_DTEL_CONFIG, "hdr.dtel_drop_report.$valid");
  F_DTEL_CONFIG_TCP_FLAGS =
      key_id_from_name(T_DTEL_CONFIG, "local_md.lkp.tcp_flags[2:0]");
  F_DTEL_CONFIG_IPV4_HDR_VALID =
      key_id_from_name(T_DTEL_CONFIG, "hdr.ipv4.$valid");
  F_DTEL_CONFIG_IPV6_HDR_VALID =
      key_id_from_name(T_DTEL_CONFIG, "hdr.ipv6.$valid");
  F_DTEL_CONFIG_IPV4_DIFFSERV =
      key_id_from_name(T_DTEL_CONFIG, "hdr.ipv4.diffserv[7:2]");
  F_DTEL_CONFIG_IPV6_TRAFFIC_CLASS =
      key_id_from_name(T_DTEL_CONFIG, "hdr.ipv6.traffic_class[7:2]");
  F_DTEL_CONFIG_IFA_CLONED =
      key_id_from_name(T_DTEL_CONFIG, "local_md.dtel.ifa_cloned");
  F_DTEL_CONFIG_PRIORITY = key_id_from_name(T_DTEL_CONFIG, "$MATCH_PRIORITY");
  A_DTEL_CONFIG_MIRROR_SWITCH_LOCAL =
      action_id_from_name(T_DTEL_CONFIG, "dtel_config.mirror_switch_local");
  A_DTEL_CONFIG_MIRROR_SWITCH_LOCAL_SET_Q = action_id_from_name(
      T_DTEL_CONFIG, "dtel_config.mirror_switch_local_and_set_q_bit");
  A_DTEL_CONFIG_MIRROR_SWITCH_LOCAL_SET_F_AND_DROP = action_id_from_name(
      T_DTEL_CONFIG, "dtel_config.mirror_switch_local_and_set_f_bit_and_drop");
  A_DTEL_CONFIG_MIRROR_SWITCH_LOCAL_SET_Q_F_AND_DROP = action_id_from_name(
      T_DTEL_CONFIG,
      "dtel_config.mirror_switch_local_and_set_q_f_bits_and_drop");
  A_DTEL_CONFIG_MIRROR_DROP =
      action_id_from_name(T_DTEL_CONFIG, "dtel_config.mirror_drop");
  A_DTEL_CONFIG_MIRROR_DROP_SET_Q = action_id_from_name(
      T_DTEL_CONFIG, "dtel_config.mirror_drop_and_set_q_bit");
  A_DTEL_CONFIG_MIRROR_CLONE =
      action_id_from_name(T_DTEL_CONFIG, "dtel_config.mirror_clone");
  A_DTEL_CONFIG_DROP = action_id_from_name(T_DTEL_CONFIG, "dtel_config.drop");
  A_DTEL_CONFIG_UPDATE =
      action_id_from_name(T_DTEL_CONFIG, "dtel_config.update");
  D_DTEL_CONFIG_UPDATE_SWITCH_ID =
      data_id_from_name(T_DTEL_CONFIG, A_DTEL_CONFIG_UPDATE, "switch_id");
  D_DTEL_CONFIG_UPDATE_HW_ID =
      data_id_from_name(T_DTEL_CONFIG, A_DTEL_CONFIG_UPDATE, "hw_id");
  D_DTEL_CONFIG_UPDATE_NEXT_PROTO =
      data_id_from_name(T_DTEL_CONFIG, A_DTEL_CONFIG_UPDATE, "next_proto");
  D_DTEL_CONFIG_UPDATE_MD_LENGTH =
      data_id_from_name(T_DTEL_CONFIG, A_DTEL_CONFIG_UPDATE, "md_length");
  D_DTEL_CONFIG_UPDATE_REP_MD_BITS =
      data_id_from_name(T_DTEL_CONFIG, A_DTEL_CONFIG_UPDATE, "rep_md_bits");
  D_DTEL_CONFIG_UPDATE_REPORT_TYPE =
      data_id_from_name(T_DTEL_CONFIG, A_DTEL_CONFIG_UPDATE, "report_type");
  A_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE = action_id_from_name(
      T_DTEL_CONFIG, "dtel_config.update_and_mirror_truncate");
  D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_SWITCH_ID = data_id_from_name(
      T_DTEL_CONFIG, A_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE, "switch_id");
  D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_HW_ID = data_id_from_name(
      T_DTEL_CONFIG, A_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE, "hw_id");
  D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_NEXT_PROTO = data_id_from_name(
      T_DTEL_CONFIG, A_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE, "next_proto");
  D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_MD_LENGTH = data_id_from_name(
      T_DTEL_CONFIG, A_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE, "md_length");
  D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_REP_MD_BITS = data_id_from_name(
      T_DTEL_CONFIG, A_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE, "rep_md_bits");
  D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_REPORT_TYPE = data_id_from_name(
      T_DTEL_CONFIG, A_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE, "report_type");
  A_DTEL_CONFIG_UPDATE_SET_ETRAP =
      action_id_from_name(T_DTEL_CONFIG, "dtel_config.update_and_set_etrap");
  D_DTEL_CONFIG_UPDATE_SET_ETRAP_SWITCH_ID = data_id_from_name(
      T_DTEL_CONFIG, A_DTEL_CONFIG_UPDATE_SET_ETRAP, "switch_id");
  D_DTEL_CONFIG_UPDATE_SET_ETRAP_HW_ID =
      data_id_from_name(T_DTEL_CONFIG, A_DTEL_CONFIG_UPDATE_SET_ETRAP, "hw_id");
  D_DTEL_CONFIG_UPDATE_SET_ETRAP_NEXT_PROTO = data_id_from_name(
      T_DTEL_CONFIG, A_DTEL_CONFIG_UPDATE_SET_ETRAP, "next_proto");
  D_DTEL_CONFIG_UPDATE_SET_ETRAP_MD_LENGTH = data_id_from_name(
      T_DTEL_CONFIG, A_DTEL_CONFIG_UPDATE_SET_ETRAP, "md_length");
  D_DTEL_CONFIG_UPDATE_SET_ETRAP_REP_MD_BITS = data_id_from_name(
      T_DTEL_CONFIG, A_DTEL_CONFIG_UPDATE_SET_ETRAP, "rep_md_bits");
  D_DTEL_CONFIG_UPDATE_SET_ETRAP_REPORT_TYPE = data_id_from_name(
      T_DTEL_CONFIG, A_DTEL_CONFIG_UPDATE_SET_ETRAP, "report_type");
  D_DTEL_CONFIG_UPDATE_SET_ETRAP_STATUS = data_id_from_name(
      T_DTEL_CONFIG, A_DTEL_CONFIG_UPDATE_SET_ETRAP, "etrap_status");
  A_DTEL_CONFIG_SET_IPV4_DSCP_ALL =
      action_id_from_name(T_DTEL_CONFIG, "dtel_config.set_ipv4_dscp_all");
  D_DTEL_CONFIG_IPV4_DSCP_ALL =
      data_id_from_name(T_DTEL_CONFIG, A_DTEL_CONFIG_SET_IPV4_DSCP_ALL, "dscp");
  A_DTEL_CONFIG_SET_IPV6_DSCP_ALL =
      action_id_from_name(T_DTEL_CONFIG, "dtel_config.set_ipv6_dscp_all");
  D_DTEL_CONFIG_IPV6_DSCP_ALL =
      data_id_from_name(T_DTEL_CONFIG, A_DTEL_CONFIG_SET_IPV6_DSCP_ALL, "dscp");
  A_DTEL_CONFIG_SET_IPV4_DSCP_2 =
      action_id_from_name(T_DTEL_CONFIG, "dtel_config.set_ipv4_dscp_2");
  D_DTEL_CONFIG_IPV4_DSCP_BIT_2 = data_id_from_name(
      T_DTEL_CONFIG, A_DTEL_CONFIG_SET_IPV4_DSCP_2, "dscp_bit_value");
  A_DTEL_CONFIG_SET_IPV6_DSCP_2 =
      action_id_from_name(T_DTEL_CONFIG, "dtel_config.set_ipv6_dscp_2");
  D_DTEL_CONFIG_IPV6_DSCP_BIT_2 = data_id_from_name(
      T_DTEL_CONFIG, A_DTEL_CONFIG_SET_IPV6_DSCP_2, "dscp_bit_value");
  A_DTEL_CONFIG_SET_IPV4_DSCP_3 =
      action_id_from_name(T_DTEL_CONFIG, "dtel_config.set_ipv4_dscp_3");
  D_DTEL_CONFIG_IPV4_DSCP_BIT_3 = data_id_from_name(
      T_DTEL_CONFIG, A_DTEL_CONFIG_SET_IPV4_DSCP_3, "dscp_bit_value");
  A_DTEL_CONFIG_SET_IPV6_DSCP_3 =
      action_id_from_name(T_DTEL_CONFIG, "dtel_config.set_ipv6_dscp_3");
  D_DTEL_CONFIG_IPV6_DSCP_BIT_3 = data_id_from_name(
      T_DTEL_CONFIG, A_DTEL_CONFIG_SET_IPV6_DSCP_3, "dscp_bit_value");
  A_DTEL_CONFIG_SET_IPV4_DSCP_4 =
      action_id_from_name(T_DTEL_CONFIG, "dtel_config.set_ipv4_dscp_4");
  D_DTEL_CONFIG_IPV4_DSCP_BIT_4 = data_id_from_name(
      T_DTEL_CONFIG, A_DTEL_CONFIG_SET_IPV4_DSCP_4, "dscp_bit_value");
  A_DTEL_CONFIG_SET_IPV6_DSCP_4 =
      action_id_from_name(T_DTEL_CONFIG, "dtel_config.set_ipv6_dscp_4");
  D_DTEL_CONFIG_IPV6_DSCP_BIT_4 = data_id_from_name(
      T_DTEL_CONFIG, A_DTEL_CONFIG_SET_IPV6_DSCP_4, "dscp_bit_value");
  A_DTEL_CONFIG_SET_IPV4_DSCP_5 =
      action_id_from_name(T_DTEL_CONFIG, "dtel_config.set_ipv4_dscp_5");
  D_DTEL_CONFIG_IPV4_DSCP_BIT_5 = data_id_from_name(
      T_DTEL_CONFIG, A_DTEL_CONFIG_SET_IPV4_DSCP_5, "dscp_bit_value");
  A_DTEL_CONFIG_SET_IPV6_DSCP_5 =
      action_id_from_name(T_DTEL_CONFIG, "dtel_config.set_ipv6_dscp_5");
  D_DTEL_CONFIG_IPV6_DSCP_BIT_5 = data_id_from_name(
      T_DTEL_CONFIG, A_DTEL_CONFIG_SET_IPV6_DSCP_5, "dscp_bit_value");
  A_DTEL_CONFIG_SET_IPV4_DSCP_6 =
      action_id_from_name(T_DTEL_CONFIG, "dtel_config.set_ipv4_dscp_6");
  D_DTEL_CONFIG_IPV4_DSCP_BIT_6 = data_id_from_name(
      T_DTEL_CONFIG, A_DTEL_CONFIG_SET_IPV4_DSCP_6, "dscp_bit_value");
  A_DTEL_CONFIG_SET_IPV6_DSCP_6 =
      action_id_from_name(T_DTEL_CONFIG, "dtel_config.set_ipv6_dscp_6");
  D_DTEL_CONFIG_IPV6_DSCP_BIT_6 = data_id_from_name(
      T_DTEL_CONFIG, A_DTEL_CONFIG_SET_IPV6_DSCP_6, "dscp_bit_value");
  A_DTEL_CONFIG_SET_IPV4_DSCP_7 =
      action_id_from_name(T_DTEL_CONFIG, "dtel_config.set_ipv4_dscp_7");
  D_DTEL_CONFIG_IPV4_DSCP_BIT_7 = data_id_from_name(
      T_DTEL_CONFIG, A_DTEL_CONFIG_SET_IPV4_DSCP_7, "dscp_bit_value");
  A_DTEL_CONFIG_SET_IPV6_DSCP_7 =
      action_id_from_name(T_DTEL_CONFIG, "dtel_config.set_ipv6_dscp_7");
  D_DTEL_CONFIG_IPV6_DSCP_BIT_7 = data_id_from_name(
      T_DTEL_CONFIG, A_DTEL_CONFIG_SET_IPV6_DSCP_7, "dscp_bit_value");

  T_INGRESS_PORT_CONVERSION =
      table_id_from_name("dtel.ingress_port_conversion");
  F_INGRESS_PORT_CONVERSION_PORT =
      key_id_from_name(T_INGRESS_PORT_CONVERSION, "port");
  F_INGRESS_PORT_CONVERSION_REPORT_VALID =
      key_id_from_name(T_INGRESS_PORT_CONVERSION, "dtel_report_valid");
  A_CONVERT_INGRESS_PORT = action_id_from_name(T_INGRESS_PORT_CONVERSION,
                                               "dtel.convert_ingress_port");
  D_CONVERT_INGRESS_PORT_PORT = data_id_from_name(
      T_INGRESS_PORT_CONVERSION, A_CONVERT_INGRESS_PORT, "port");

  T_EGRESS_PORT_CONVERSION = table_id_from_name("dtel.egress_port_conversion");
  F_EGRESS_PORT_CONVERSION_PORT =
      key_id_from_name(T_EGRESS_PORT_CONVERSION, "port");
  F_EGRESS_PORT_CONVERSION_REPORT_VALID =
      key_id_from_name(T_EGRESS_PORT_CONVERSION, "dtel_report_valid");
  A_CONVERT_EGRESS_PORT =
      action_id_from_name(T_EGRESS_PORT_CONVERSION, "dtel.convert_egress_port");
  D_CONVERT_EGRESS_PORT_PORT = data_id_from_name(
      T_EGRESS_PORT_CONVERSION, A_CONVERT_EGRESS_PORT, "port");

  T_EGRESS_DROP_REPORT_BLOOM_FILTER_1 =
      table_id_from_name("drop_report.array1");
  F_EGRESS_DROP_REPORT_BLOOM_FILTER_1_REGISTER_INDEX =
      key_id_from_name(T_EGRESS_DROP_REPORT_BLOOM_FILTER_1, "$REGISTER_INDEX");
  D_EGRESS_DROP_REPORT_BLOOM_FILTER_1_RESULT_VALUE = data_id_from_name_noaction(
      T_EGRESS_DROP_REPORT_BLOOM_FILTER_1, "drop_report.array1.f1");

  T_EGRESS_DROP_REPORT_BLOOM_FILTER_2 =
      table_id_from_name("drop_report.array2");
  F_EGRESS_DROP_REPORT_BLOOM_FILTER_2_REGISTER_INDEX =
      key_id_from_name(T_EGRESS_DROP_REPORT_BLOOM_FILTER_2, "$REGISTER_INDEX");
  D_EGRESS_DROP_REPORT_BLOOM_FILTER_2_RESULT_VALUE = data_id_from_name_noaction(
      T_EGRESS_DROP_REPORT_BLOOM_FILTER_2, "drop_report.array2.f1");

  T_EGRESS_FLOW_REPORT_BLOOM_FILTER_1 =
      table_id_from_name("flow_report.array1");
  F_EGRESS_FLOW_REPORT_BLOOM_FILTER_1_REGISTER_INDEX =
      key_id_from_name(T_EGRESS_FLOW_REPORT_BLOOM_FILTER_1, "$REGISTER_INDEX");
  D_EGRESS_FLOW_REPORT_BLOOM_FILTER_1_RESULT_VALUE = data_id_from_name_noaction(
      T_EGRESS_FLOW_REPORT_BLOOM_FILTER_1, "flow_report.array1.f1");

  T_EGRESS_FLOW_REPORT_BLOOM_FILTER_2 =
      table_id_from_name("flow_report.array2");
  F_EGRESS_FLOW_REPORT_BLOOM_FILTER_2_REGISTER_INDEX =
      key_id_from_name(T_EGRESS_FLOW_REPORT_BLOOM_FILTER_2, "$REGISTER_INDEX");
  D_EGRESS_FLOW_REPORT_BLOOM_FILTER_2_RESULT_VALUE = data_id_from_name_noaction(
      T_EGRESS_FLOW_REPORT_BLOOM_FILTER_2, "flow_report.array2.f1");

  T_INT_EDGE_PORT_LOOKUP = table_id_from_name("dtel.int_edge.port_lookup");
  F_INT_EDGE_PORT_LOOKUP_PORT =
      key_id_from_name(T_INT_EDGE_PORT_LOOKUP, "local_md.egress_port");
  A_INT_EDGE_SET_IFA_EDGE =
      action_id_from_name(T_INT_EDGE_PORT_LOOKUP, "dtel.int_edge.set_ifa_edge");
  A_INT_EDGE_SET_CLONE_MIRROR_SESSION_ID = action_id_from_name(
      T_INT_EDGE_PORT_LOOKUP, "dtel.int_edge.set_clone_mirror_session_id");
  D_INT_EDGE_CLONE_MIRROR_SESSION_ID =
      data_id_from_name(T_INT_EDGE_PORT_LOOKUP,
                        A_INT_EDGE_SET_CLONE_MIRROR_SESSION_ID,
                        "session_id");

  T_INGRESS_IP_DTEL_ACL_SAMPLE_SESSION =
      table_id_from_name("ingress_ip_dtel_acl.samplers");
  F_INGRESS_IP_DTEL_ACL_SAMPLE_SESSION_REGISTER_INDEX =
      key_id_from_name(T_INGRESS_IP_DTEL_ACL_SAMPLE_SESSION, "$REGISTER_INDEX");
  D_INGRESS_IP_DTEL_ACL_SAMPLE_SESSION_REG_CURRENT =
      data_id_from_name_noaction(T_INGRESS_IP_DTEL_ACL_SAMPLE_SESSION,
                                 "ingress_ip_dtel_acl.samplers.current");
  D_INGRESS_IP_DTEL_ACL_SAMPLE_SESSION_REG_RATE =
      data_id_from_name_noaction(T_INGRESS_IP_DTEL_ACL_SAMPLE_SESSION,
                                 "ingress_ip_dtel_acl.samplers.rate");

  // sflow
  T_INGRESS_SFLOW_SESSION = table_id_from_name("ingress_sflow_samplers");
  F_INGRESS_SFLOW_SESSION_REGISTER_INDEX =
      key_id_from_name(T_INGRESS_SFLOW_SESSION, "$REGISTER_INDEX");
  D_INGRESS_SFLOW_SESSION_REG_CURRENT = data_id_from_name_noaction(
      T_INGRESS_SFLOW_SESSION, "ingress_sflow_samplers.current");
  D_INGRESS_SFLOW_SESSION_REG_RATE = data_id_from_name_noaction(
      T_INGRESS_SFLOW_SESSION, "ingress_sflow_samplers.rate");

  // NAT
  T_INGRESS_NAT_DNAPT_INDEX = table_id_from_name("dest_napt_index");
  F_INGRESS_NAT_DNAPT_INDEX_PROTOCOL =
      key_id_from_name(T_INGRESS_NAT_DNAPT_INDEX, "local_md.lkp.ip_proto");
  F_INGRESS_NAT_DNAPT_INDEX_DIP = key_id_from_name(
      T_INGRESS_NAT_DNAPT_INDEX, "local_md.lkp.ip_dst_addr[95:64]");
  F_INGRESS_NAT_DNAPT_INDEX_DPORT =
      key_id_from_name(T_INGRESS_NAT_DNAPT_INDEX, "local_md.lkp.l4_dst_port");
  A_INGRESS_NAT_DNAPT_SET_INDEX =
      action_id_from_name(T_INGRESS_NAT_DNAPT_INDEX, "set_dnapt_index");
  P_INGRESS_NAT_DNAPT_SET_INDEX = data_id_from_name(
      T_INGRESS_NAT_DNAPT_INDEX, A_INGRESS_NAT_DNAPT_SET_INDEX, "index");
  D_INGRESS_NAT_DNAPT_INDEX_TTL = data_id_from_name(
      T_INGRESS_NAT_DNAPT_INDEX, A_INGRESS_NAT_DNAPT_SET_INDEX, "$ENTRY_TTL");

  T_INGRESS_NAT_DEST_NAPT = table_id_from_name("dest_napt");
  F_INGRESS_NAT_DEST_NAPT_PROTOCOL =
      key_id_from_name(T_INGRESS_NAT_DEST_NAPT, "local_md.lkp.ip_proto");
  F_INGRESS_NAT_DEST_NAPT_DIP = key_id_from_name(
      T_INGRESS_NAT_DEST_NAPT, "local_md.lkp.ip_dst_addr[95:64]");
  F_INGRESS_NAT_DEST_NAPT_DPORT =
      key_id_from_name(T_INGRESS_NAT_DEST_NAPT, "local_md.lkp.l4_dst_port");
  F_INGRESS_NAT_DNAPT_INDEX =
      key_id_from_name(T_INGRESS_NAT_DEST_NAPT, "local_md.nat.dnapt_index");
  A_INGRESS_NAT_DEST_NAPT_REWRITE =
      action_id_from_name(T_INGRESS_NAT_DEST_NAPT, "set_dnapt_rewrite");
  A_INGRESS_NAT_DEST_NAPT_MISS =
      action_id_from_name(T_INGRESS_NAT_DEST_NAPT, "dnapt_miss");
  P_DEST_NAPT_REWRITE_DIP = data_id_from_name(
      T_INGRESS_NAT_DEST_NAPT, A_INGRESS_NAT_DEST_NAPT_REWRITE, "dip");
  P_DEST_NAPT_REWRITE_DPORT = data_id_from_name(
      T_INGRESS_NAT_DEST_NAPT, A_INGRESS_NAT_DEST_NAPT_REWRITE, "dport");
  D_INGRESS_NAT_DEST_NAPT_COUNTER_SPEC_PKTS =
      data_id_from_name(T_INGRESS_NAT_DEST_NAPT,
                        A_INGRESS_NAT_DEST_NAPT_REWRITE,
                        "$COUNTER_SPEC_PKTS");
  D_INGRESS_NAT_DEST_NAPT_COUNTER_SPEC_BYTES =
      data_id_from_name(T_INGRESS_NAT_DEST_NAPT,
                        A_INGRESS_NAT_DEST_NAPT_REWRITE,
                        "$COUNTER_SPEC_BYTES");
  D_INGRESS_NAT_DEST_NAPT_TTL = data_id_from_name(
      T_INGRESS_NAT_DEST_NAPT, A_INGRESS_NAT_DEST_NAPT_REWRITE, "$ENTRY_TTL");

  T_INGRESS_NAT_DEST_NAT = table_id_from_name("dest_nat");
  F_INGRESS_NAT_DEST_NAT_DIP = key_id_from_name(
      T_INGRESS_NAT_DEST_NAT, "local_md.lkp.ip_dst_addr[95:64]");
  A_INGRESS_NAT_DEST_NAT_REWRITE =
      action_id_from_name(T_INGRESS_NAT_DEST_NAT, "set_dnat_rewrite");
  A_INGRESS_NAT_DEST_NAT_MISS =
      action_id_from_name(T_INGRESS_NAT_DEST_NAT, "dnat_miss");
  P_DEST_NAT_REWRITE_DIP = data_id_from_name(
      T_INGRESS_NAT_DEST_NAT, A_INGRESS_NAT_DEST_NAT_REWRITE, "dip");
  D_INGRESS_NAT_DEST_NAT_COUNTER_SPEC_PKTS =
      data_id_from_name(T_INGRESS_NAT_DEST_NAT,
                        A_INGRESS_NAT_DEST_NAT_REWRITE,
                        "$COUNTER_SPEC_PKTS");
  D_INGRESS_NAT_DEST_NAT_COUNTER_SPEC_BYTES =
      data_id_from_name(T_INGRESS_NAT_DEST_NAT,
                        A_INGRESS_NAT_DEST_NAT_REWRITE,
                        "$COUNTER_SPEC_BYTES");
  D_INGRESS_NAT_DEST_NAT_TTL = data_id_from_name(
      T_INGRESS_NAT_DEST_NAT, A_INGRESS_NAT_DEST_NAT_REWRITE, "$ENTRY_TTL");

  T_INGRESS_NAT_DEST_NAT_POOL = table_id_from_name("dnat_pool");
  F_INGRESS_NAT_DEST_NAT_POOL_DIP = key_id_from_name(
      T_INGRESS_NAT_DEST_NAT_POOL, "local_md.lkp.ip_dst_addr[95:64]");
  A_INGRESS_NAT_DEST_NAT_POOL_HIT =
      action_id_from_name(T_INGRESS_NAT_DEST_NAT_POOL, "set_dnat_pool_hit");
  A_INGRESS_NAT_DEST_NAT_POOL_MISS =
      action_id_from_name(T_INGRESS_NAT_DEST_NAT_POOL, "set_dnat_pool_miss");
  D_INGRESS_NAT_DEST_NAT_POOL_COUNTER_SPEC_PKTS =
      data_id_from_name(T_INGRESS_NAT_DEST_NAT_POOL,
                        A_INGRESS_NAT_DEST_NAT_POOL_HIT,
                        "$COUNTER_SPEC_PKTS");
  D_INGRESS_NAT_DEST_NAT_POOL_COUNTER_SPEC_BYTES =
      data_id_from_name(T_INGRESS_NAT_DEST_NAT_POOL,
                        A_INGRESS_NAT_DEST_NAT_POOL_HIT,
                        "$COUNTER_SPEC_BYTES");

  T_INGRESS_NAT_FLOW_NAPT = table_id_from_name("flow_napt");
  F_INGRESS_NAT_FLOW_NAPT_PROTOCOL =
      key_id_from_name(T_INGRESS_NAT_FLOW_NAPT, "local_md.lkp.ip_proto");
  F_INGRESS_NAT_FLOW_NAPT_DIP = key_id_from_name(
      T_INGRESS_NAT_FLOW_NAPT, "local_md.lkp.ip_dst_addr[95:64]");
  F_INGRESS_NAT_FLOW_NAPT_DPORT =
      key_id_from_name(T_INGRESS_NAT_FLOW_NAPT, "local_md.lkp.l4_dst_port");
  F_INGRESS_NAT_FLOW_NAPT_SIP = key_id_from_name(
      T_INGRESS_NAT_FLOW_NAPT, "local_md.lkp.ip_src_addr[95:64]");
  F_INGRESS_NAT_FLOW_NAPT_SPORT =
      key_id_from_name(T_INGRESS_NAT_FLOW_NAPT, "local_md.lkp.l4_src_port");
  A_INGRESS_NAT_FLOW_NAPT_REWRITE =
      action_id_from_name(T_INGRESS_NAT_FLOW_NAPT, "set_flow_napt_rewrite");
  A_INGRESS_NAT_FLOW_NAPT_MISS =
      action_id_from_name(T_INGRESS_NAT_FLOW_NAPT, "flow_napt_miss");
  P_FLOW_NAPT_REWRITE_DIP = data_id_from_name(
      T_INGRESS_NAT_FLOW_NAPT, A_INGRESS_NAT_FLOW_NAPT_REWRITE, "dip");
  P_FLOW_NAPT_REWRITE_SIP = data_id_from_name(
      T_INGRESS_NAT_FLOW_NAPT, A_INGRESS_NAT_FLOW_NAPT_REWRITE, "sip");
  P_FLOW_NAPT_REWRITE_DPORT = data_id_from_name(
      T_INGRESS_NAT_FLOW_NAPT, A_INGRESS_NAT_FLOW_NAPT_REWRITE, "dport");
  P_FLOW_NAPT_REWRITE_SPORT = data_id_from_name(
      T_INGRESS_NAT_FLOW_NAPT, A_INGRESS_NAT_FLOW_NAPT_REWRITE, "sport");
  D_INGRESS_NAT_FLOW_NAPT_COUNTER_SPEC_PKTS =
      data_id_from_name(T_INGRESS_NAT_FLOW_NAPT,
                        A_INGRESS_NAT_FLOW_NAPT_REWRITE,
                        "$COUNTER_SPEC_PKTS");
  D_INGRESS_NAT_FLOW_NAPT_COUNTER_SPEC_BYTES =
      data_id_from_name(T_INGRESS_NAT_FLOW_NAPT,
                        A_INGRESS_NAT_FLOW_NAPT_REWRITE,
                        "$COUNTER_SPEC_BYTES");

  T_INGRESS_NAT_FLOW_NAT = table_id_from_name("flow_nat");
  F_INGRESS_NAT_FLOW_NAT_DIP = key_id_from_name(
      T_INGRESS_NAT_FLOW_NAT, "local_md.lkp.ip_dst_addr[95:64]");
  F_INGRESS_NAT_FLOW_NAT_SIP = key_id_from_name(
      T_INGRESS_NAT_FLOW_NAT, "local_md.lkp.ip_src_addr[95:64]");
  A_INGRESS_NAT_FLOW_NAT_REWRITE =
      action_id_from_name(T_INGRESS_NAT_FLOW_NAT, "set_flow_nat_rewrite");
  A_INGRESS_NAT_FLOW_NAT_MISS =
      action_id_from_name(T_INGRESS_NAT_FLOW_NAT, "flow_nat_miss");
  P_FLOW_NAT_REWRITE_DIP = data_id_from_name(
      T_INGRESS_NAT_FLOW_NAT, A_INGRESS_NAT_FLOW_NAT_REWRITE, "dip");
  P_FLOW_NAT_REWRITE_SIP = data_id_from_name(
      T_INGRESS_NAT_FLOW_NAT, A_INGRESS_NAT_FLOW_NAT_REWRITE, "sip");
  D_INGRESS_NAT_FLOW_NAT_COUNTER_SPEC_PKTS =
      data_id_from_name(T_INGRESS_NAT_FLOW_NAT,
                        A_INGRESS_NAT_FLOW_NAT_REWRITE,
                        "$COUNTER_SPEC_PKTS");
  D_INGRESS_NAT_FLOW_NAT_COUNTER_SPEC_BYTES =
      data_id_from_name(T_INGRESS_NAT_FLOW_NAT,
                        A_INGRESS_NAT_FLOW_NAT_REWRITE,
                        "$COUNTER_SPEC_BYTES");

  T_INGRESS_NAT_REWRITE = table_id_from_name("ingress_nat_rewrite");
  F_INGRESS_NAT_REWRITE_NAT_HIT =
      key_id_from_name(T_INGRESS_NAT_REWRITE, "local_md.nat.hit");
  F_INGRESS_NAT_REWRITE_IP_PROTO =
      key_id_from_name(T_INGRESS_NAT_REWRITE, "local_md.lkp.ip_proto");
  F_INGRESS_NAT_REWRITE_NAT_SAME_ZONE_CHECK = key_id_from_name(
      T_INGRESS_NAT_REWRITE, "local_md.checks.same_zone_check");
  A_INGRESS_NAT_REWRITE_TCP_FLOW =
      action_id_from_name(T_INGRESS_NAT_REWRITE, "rewrite_tcp_flow");
  A_INGRESS_NAT_REWRITE_UDP_FLOW =
      action_id_from_name(T_INGRESS_NAT_REWRITE, "rewrite_udp_flow");
  A_INGRESS_NAT_REWRITE_IPSA_IPDA =
      action_id_from_name(T_INGRESS_NAT_REWRITE, "rewrite_ipsa_ipda");
  A_INGRESS_NAT_REWRITE_TCP_DPORT_IPDA =
      action_id_from_name(T_INGRESS_NAT_REWRITE, "rewrite_tcp_ipda");
  A_INGRESS_NAT_REWRITE_UDP_DPORT_IPDA =
      action_id_from_name(T_INGRESS_NAT_REWRITE, "rewrite_udp_ipda");
  A_INGRESS_NAT_REWRITE_IPDA =
      action_id_from_name(T_INGRESS_NAT_REWRITE, "rewrite_ipda");

  A_INGRESS_NAT_REWRITE_TCP_SPORT_IPSA =
      action_id_from_name(T_INGRESS_NAT_REWRITE, "rewrite_tcp_ipsa");
  A_INGRESS_NAT_REWRITE_UDP_SPORT_IPSA =
      action_id_from_name(T_INGRESS_NAT_REWRITE, "rewrite_udp_ipsa");
  A_INGRESS_NAT_REWRITE_IPSA =
      action_id_from_name(T_INGRESS_NAT_REWRITE, "rewrite_ipsa");

  T_INGRESS_NAT_SNAT = table_id_from_name("source_nat");
  F_INGRESS_NAT_SNAT_IPSA =
      key_id_from_name(T_INGRESS_NAT_SNAT, "local_md.lkp.ip_src_addr[95:64]");
  A_INGRESS_NAT_SNAT_REWRITE =
      action_id_from_name(T_INGRESS_NAT_SNAT, "set_snat_rewrite");
  A_INGRESS_NAT_SNAT_MISS =
      action_id_from_name(T_INGRESS_NAT_SNAT, "source_nat_miss");
  P_SOURCE_NAT_REWRITE_SIP =
      data_id_from_name(T_INGRESS_NAT_SNAT, A_INGRESS_NAT_SNAT_REWRITE, "sip");
  D_INGRESS_NAT_SNAT_COUNTER_SPEC_PKTS = data_id_from_name(
      T_INGRESS_NAT_SNAT, A_INGRESS_NAT_SNAT_REWRITE, "$COUNTER_SPEC_PKTS");
  D_INGRESS_NAT_SNAT_COUNTER_SPEC_BYTES = data_id_from_name(
      T_INGRESS_NAT_SNAT, A_INGRESS_NAT_SNAT_REWRITE, "$COUNTER_SPEC_BYTES");
  D_INGRESS_NAT_SNAT_TTL = data_id_from_name(
      T_INGRESS_NAT_SNAT, A_INGRESS_NAT_SNAT_REWRITE, "$ENTRY_TTL");

  T_INGRESS_NAT_SNAPT_INDEX = table_id_from_name("src_napt_index");
  F_INGRESS_NAT_SNAPT_INDEX_IPSA = key_id_from_name(
      T_INGRESS_NAT_SNAPT_INDEX, "local_md.lkp.ip_src_addr[95:64]");
  F_INGRESS_NAT_SNAPT_INDEX_IP_PROTO =
      key_id_from_name(T_INGRESS_NAT_SNAPT_INDEX, "local_md.lkp.ip_proto");
  F_INGRESS_NAT_SNAPT_INDEX_IP_L4_SRC_PORT =
      key_id_from_name(T_INGRESS_NAT_SNAPT_INDEX, "local_md.lkp.l4_src_port");
  A_INGRESS_NAT_SNAPT_SET_INDEX =
      action_id_from_name(T_INGRESS_NAT_SNAPT_INDEX, "set_snapt_index");
  P_INGRESS_NAT_SNAPT_SET_INDEX = data_id_from_name(
      T_INGRESS_NAT_SNAPT_INDEX, A_INGRESS_NAT_SNAPT_SET_INDEX, "index");
  D_INGRESS_NAT_SNAPT_INDEX_TTL = data_id_from_name(
      T_INGRESS_NAT_SNAPT_INDEX, A_INGRESS_NAT_SNAPT_SET_INDEX, "$ENTRY_TTL");

  T_INGRESS_NAT_SNAPT = table_id_from_name("source_napt");
  F_INGRESS_NAT_SNAPT_IPSA =
      key_id_from_name(T_INGRESS_NAT_SNAPT, "local_md.lkp.ip_src_addr[95:64]");
  F_INGRESS_NAT_SNAPT_IP_PROTO =
      key_id_from_name(T_INGRESS_NAT_SNAPT, "local_md.lkp.ip_proto");
  F_INGRESS_NAT_SNAPT_IP_L4_SRC_PORT =
      key_id_from_name(T_INGRESS_NAT_SNAPT, "local_md.lkp.l4_src_port");
  F_INGRESS_NAT_SNAPT_INDEX =
      key_id_from_name(T_INGRESS_NAT_SNAPT, "local_md.nat.snapt_index");
  A_INGRESS_NAT_SNAPT_REWRITE =
      action_id_from_name(T_INGRESS_NAT_SNAPT, "set_snapt_rewrite");
  A_INGRESS_NAT_SNAPT_MISS =
      action_id_from_name(T_INGRESS_NAT_SNAPT, "source_napt_miss");
  P_SOURCE_NAPT_REWRITE_SIP = data_id_from_name(
      T_INGRESS_NAT_SNAPT, A_INGRESS_NAT_SNAPT_REWRITE, "sip");
  P_SOURCE_NAPT_REWRITE_SPORT = data_id_from_name(
      T_INGRESS_NAT_SNAPT, A_INGRESS_NAT_SNAPT_REWRITE, "sport");
  D_INGRESS_NAT_SNAPT_COUNTER_SPEC_PKTS = data_id_from_name(
      T_INGRESS_NAT_SNAPT, A_INGRESS_NAT_SNAPT_REWRITE, "$COUNTER_SPEC_PKTS");
  D_INGRESS_NAT_SNAPT_COUNTER_SPEC_BYTES = data_id_from_name(
      T_INGRESS_NAT_SNAPT, A_INGRESS_NAT_SNAPT_REWRITE, "$COUNTER_SPEC_BYTES");
  D_INGRESS_NAT_SNAPT_TTL = data_id_from_name(
      T_INGRESS_NAT_SNAPT, A_INGRESS_NAT_SNAPT_REWRITE, "$ENTRY_TTL");

  // SRv6

  T_MY_SID = table_id_from_name("my_sid");
  F_MY_SID_IPV6_DST_ADDR = key_id_from_name(T_MY_SID, "hdr.ipv6.dst_addr");
  F_MY_SID_VRF = key_id_from_name(T_MY_SID, "local_md.vrf");
  F_MY_SID_SRH_HDR_VALID = key_id_from_name(T_MY_SID, "hdr.srh_base.$valid");
  F_MY_SID_MATCH_PRIORITY = key_id_from_name(T_MY_SID, "$MATCH_PRIORITY");
  F_MY_SID_SRH_SEG_LEFT = key_id_from_name(T_MY_SID, "hdr.srh_base.seg_left");

  A_ENDPOINT_ACTION_END = action_id_from_name(T_MY_SID, "end");
  A_ENDPOINT_ACTION_END_WITH_PSP =
      action_id_from_name(T_MY_SID, "end_with_psp");
  A_ENDPOINT_ACTION_END_WITH_USD =
      action_id_from_name(T_MY_SID, "end_with_usd");
  A_ENDPOINT_ACTION_END_UN = action_id_from_name(T_MY_SID, "end_uN");
  A_ENDPOINT_ACTION_END_X = action_id_from_name(T_MY_SID, "end_x");
  D_ENDPOINT_ACTION_END_X_NEXTHOP =
      data_id_from_name(T_MY_SID, A_ENDPOINT_ACTION_END_X, "nexthop");
  A_ENDPOINT_ACTION_END_X_WITH_PSP =
      action_id_from_name(T_MY_SID, "end_x_with_psp");
  D_ENDPOINT_ACTION_END_X_WITH_PSP_NEXTHOP =
      data_id_from_name(T_MY_SID, A_ENDPOINT_ACTION_END_X_WITH_PSP, "nexthop");
  A_ENDPOINT_ACTION_END_X_WITH_USD =
      action_id_from_name(T_MY_SID, "end_x_with_usd");
  D_ENDPOINT_ACTION_END_X_WITH_USD_NEXTHOP =
      data_id_from_name(T_MY_SID, A_ENDPOINT_ACTION_END_X_WITH_USD, "nexthop");
  A_ENDPOINT_ACTION_END_UA = action_id_from_name(T_MY_SID, "end_uA");
  D_ENDPOINT_ACTION_END_UA_NEXTHOP =
      data_id_from_name(T_MY_SID, A_ENDPOINT_ACTION_END_UA, "nexthop");
  A_ENDPOINT_ACTION_END_T = action_id_from_name(T_MY_SID, "end_t");
  D_ENDPOINT_ACTION_END_T_VRF =
      data_id_from_name(T_MY_SID, A_ENDPOINT_ACTION_END_T, "vrf");
  A_ENDPOINT_ACTION_END_DT4 = action_id_from_name(T_MY_SID, "end_dt4");
  D_ENDPOINT_ACTION_END_DT4_VRF =
      data_id_from_name(T_MY_SID, A_ENDPOINT_ACTION_END_DT4, "vrf");
  A_ENDPOINT_ACTION_END_DT6 = action_id_from_name(T_MY_SID, "end_dt6");
  D_ENDPOINT_ACTION_END_DT6_VRF =
      data_id_from_name(T_MY_SID, A_ENDPOINT_ACTION_END_DT6, "vrf");
  A_ENDPOINT_ACTION_END_DT46 = action_id_from_name(T_MY_SID, "end_dt46");
  D_ENDPOINT_ACTION_END_DT46_VRF =
      data_id_from_name(T_MY_SID, A_ENDPOINT_ACTION_END_DT46, "vrf");
  A_ENDPOINT_ACTION_END_DX4 = action_id_from_name(T_MY_SID, "end_dx4");
  D_ENDPOINT_ACTION_END_DX4_NEXTHOP =
      data_id_from_name(T_MY_SID, A_ENDPOINT_ACTION_END_DX4, "nexthop");
  A_ENDPOINT_ACTION_END_DX6 = action_id_from_name(T_MY_SID, "end_dx6");
  D_ENDPOINT_ACTION_END_DX6_NEXTHOP =
      data_id_from_name(T_MY_SID, A_ENDPOINT_ACTION_END_DX6, "nexthop");
  A_ENDPOINT_ACTION_END_B6_ENCAPS_RED =
      action_id_from_name(T_MY_SID, "end_b6_encaps_red");
  D_ENDPOINT_ACTION_END_B6_ENCAPS_RED_NEXTHOP = data_id_from_name(
      T_MY_SID, A_ENDPOINT_ACTION_END_B6_ENCAPS_RED, "nexthop");
  A_ENDPOINT_ACTION_END_B6_INSERT_RED =
      action_id_from_name(T_MY_SID, "end_b6_insert_red");
  D_ENDPOINT_ACTION_END_B6_INSERT_RED_NEXTHOP = data_id_from_name(
      T_MY_SID, A_ENDPOINT_ACTION_END_B6_INSERT_RED, "nexthop");
  A_ENDPOINT_ACTION_TRAP = action_id_from_name(T_MY_SID, "srv6_trap");
  A_ENDPOINT_ACTION_DROP = action_id_from_name(T_MY_SID, "srv6_drop");
  D_MY_SID_COUNTER_SPEC_PKTS =
      data_id_from_name_noaction(T_MY_SID, "$COUNTER_SPEC_PKTS");
  D_MY_SID_COUNTER_SPEC_BYTES =
      data_id_from_name_noaction(T_MY_SID, "$COUNTER_SPEC_BYTES");

  T_SID_REWRITE = table_id_from_name("sid_rewrite");
  F_SID_REWRITE_LOCAL_MD_TUNNEL_NEXTHOP =
      key_id_from_name(T_SID_REWRITE, "local_md.tunnel_nexthop");
  A_SRV6_ENCAPS_SID_REWRITE_0 =
      action_id_from_name(T_SID_REWRITE, "srv6_encaps_sid_rewrite_0");
  D_SRV6_ENCAPS_SID_REWRITE_0_S0 =
      data_id_from_name(T_SID_REWRITE, A_SRV6_ENCAPS_SID_REWRITE_0, "s0");
  A_SRV6_ENCAPS_SID_REWRITE_1 =
      action_id_from_name(T_SID_REWRITE, "srv6_encaps_sid_rewrite_1");
  D_SRV6_ENCAPS_SID_REWRITE_1_S0 =
      data_id_from_name(T_SID_REWRITE, A_SRV6_ENCAPS_SID_REWRITE_1, "s0");
  D_SRV6_ENCAPS_SID_REWRITE_1_S1 =
      data_id_from_name(T_SID_REWRITE, A_SRV6_ENCAPS_SID_REWRITE_1, "s1");
  A_SRV6_ENCAPS_SID_REWRITE_2 =
      action_id_from_name(T_SID_REWRITE, "srv6_encaps_sid_rewrite_2");
  D_SRV6_ENCAPS_SID_REWRITE_2_S0 =
      data_id_from_name(T_SID_REWRITE, A_SRV6_ENCAPS_SID_REWRITE_2, "s0");
  D_SRV6_ENCAPS_SID_REWRITE_2_S1 =
      data_id_from_name(T_SID_REWRITE, A_SRV6_ENCAPS_SID_REWRITE_2, "s1");
  D_SRV6_ENCAPS_SID_REWRITE_2_S2 =
      data_id_from_name(T_SID_REWRITE, A_SRV6_ENCAPS_SID_REWRITE_2, "s2");
  A_SRV6_INSERT_SID_REWRITE_0 =
      action_id_from_name(T_SID_REWRITE, "srv6_insert_sid_rewrite_0");
  D_SRV6_INSERT_SID_REWRITE_0_S0 =
      data_id_from_name(T_SID_REWRITE, A_SRV6_INSERT_SID_REWRITE_0, "s0");
  A_SRV6_INSERT_SID_REWRITE_1 =
      action_id_from_name(T_SID_REWRITE, "srv6_insert_sid_rewrite_1");
  D_SRV6_INSERT_SID_REWRITE_1_S0 =
      data_id_from_name(T_SID_REWRITE, A_SRV6_INSERT_SID_REWRITE_1, "s0");
  D_SRV6_INSERT_SID_REWRITE_1_S1 =
      data_id_from_name(T_SID_REWRITE, A_SRV6_INSERT_SID_REWRITE_1, "s1");
  D_SRV6_SID_REWRITE_COUNTER_SPEC_PKTS =
      data_id_from_name_noaction(T_SID_REWRITE, "$COUNTER_SPEC_PKTS");
  D_SRV6_SID_REWRITE_COUNTER_SPEC_BYTES =
      data_id_from_name_noaction(T_SID_REWRITE, "$COUNTER_SPEC_BYTES");

  T_MPLS_LABEL = table_id_from_name("mpls_label");
  F_MPLS_LABEL_LOCAL_MD_TUNNEL_NEXTHOP =
      key_id_from_name(T_MPLS_LABEL, "local_md.tunnel_nexthop");
  A_MPLS_PUSH_1_LABEL = action_id_from_name(T_MPLS_LABEL, "mpls_push_1_label");
  P_MPLS_PUSH_1_LABEL0 =
      data_id_from_name(T_MPLS_LABEL, A_MPLS_PUSH_1_LABEL, "label0");
  A_MPLS_PUSH_2_LABEL = action_id_from_name(T_MPLS_LABEL, "mpls_push_2_label");
  P_MPLS_PUSH_2_LABEL0 =
      data_id_from_name(T_MPLS_LABEL, A_MPLS_PUSH_2_LABEL, "label0");
  P_MPLS_PUSH_2_LABEL1 =
      data_id_from_name(T_MPLS_LABEL, A_MPLS_PUSH_2_LABEL, "label1");
  A_MPLS_PUSH_3_LABEL = action_id_from_name(T_MPLS_LABEL, "mpls_push_3_label");
  P_MPLS_PUSH_3_LABEL0 =
      data_id_from_name(T_MPLS_LABEL, A_MPLS_PUSH_3_LABEL, "label0");
  P_MPLS_PUSH_3_LABEL1 =
      data_id_from_name(T_MPLS_LABEL, A_MPLS_PUSH_3_LABEL, "label1");
  P_MPLS_PUSH_3_LABEL2 =
      data_id_from_name(T_MPLS_LABEL, A_MPLS_PUSH_3_LABEL, "label2");
  A_MPLS_SWAP_LABEL = action_id_from_name(T_MPLS_LABEL, "mpls_swap_label");
  P_MPLS_SWAP_LABEL0 =
      data_id_from_name(T_MPLS_LABEL, A_MPLS_SWAP_LABEL, "label0");

  T_MPLS_ENCAP = table_id_from_name("mpls_encap");
  F_MPLS_PUSH_COUNT =
      key_id_from_name(T_MPLS_ENCAP, "local_md.tunnel.mpls_push_count");
  A_MPLS_ENCAP_NOACTION = action_id_from_name(T_MPLS_ENCAP, "NoAction");
  A_MPLS_ENCAP_1 = action_id_from_name(T_MPLS_ENCAP, "mpls_encap_1");
  A_MPLS_ENCAP_2 = action_id_from_name(T_MPLS_ENCAP, "mpls_encap_2");
  A_MPLS_ENCAP_3 = action_id_from_name(T_MPLS_ENCAP, "mpls_encap_3");

  T_MPLS_TTL_REWRITE = table_id_from_name("mpls_ttl_rewrite");
  F_MPLS_TTL_REWRITE_PUSH_COUNT =
      key_id_from_name(T_MPLS_TTL_REWRITE, "local_md.tunnel.mpls_push_count");
  F_MPLS_TTL_REWRITE_SWAP =
      key_id_from_name(T_MPLS_TTL_REWRITE, "local_md.tunnel.mpls_swap");
  A_MPLS_REWRITE_TTL_1_PIPE =
      action_id_from_name(T_MPLS_TTL_REWRITE, "mpls_ttl_1_pipe");
  A_MPLS_REWRITE_TTL_2_PIPE =
      action_id_from_name(T_MPLS_TTL_REWRITE, "mpls_ttl_2_pipe");
  A_MPLS_REWRITE_TTL_3_PIPE =
      action_id_from_name(T_MPLS_TTL_REWRITE, "mpls_ttl_3_pipe");
  A_MPLS_REWRITE_TTL_DECREMENT =
      action_id_from_name(T_MPLS_TTL_REWRITE, "mpls_ttl_decrement");

  T_MPLS_EXP_REWRITE = table_id_from_name("mpls_exp_rewrite");
  F_MPLS_EXP_REWRITE_PUSH_COUNT =
      key_id_from_name(T_MPLS_EXP_REWRITE, "local_md.tunnel.mpls_push_count");
  A_MPLS_EXP_REWRITE_LABEL1 = action_id_from_name(
      T_MPLS_EXP_REWRITE, "SwitchEgress.tunnel_rewrite.mpls_exp_label1");
  A_MPLS_EXP_REWRITE_LABEL2 = action_id_from_name(
      T_MPLS_EXP_REWRITE, "SwitchEgress.tunnel_rewrite.mpls_exp_label2");
  A_MPLS_EXP_REWRITE_LABEL3 = action_id_from_name(
      T_MPLS_EXP_REWRITE, "SwitchEgress.tunnel_rewrite.mpls_exp_label3");

  T_MPLS_FIB = table_id_from_name("mpls_fib");
  F_MPLS_FIB_LOOKUP_LABEL =
      key_id_from_name(T_MPLS_FIB, "lkp.mpls_lookup_label");
  A_MPLS_FIB_MPLS_TERM = action_id_from_name(T_MPLS_FIB, "mpls_term");
  A_MPLS_FIB_MPLS_SWAP = action_id_from_name(T_MPLS_FIB, "mpls_swap");
  A_MPLS_FIB_MPLS_PHP = action_id_from_name(T_MPLS_FIB, "mpls_php");
  A_MPLS_FIB_MPLS_DROP = action_id_from_name(T_MPLS_FIB, "mpls_drop");
  A_MPLS_FIB_MPLS_TRAP = action_id_from_name(T_MPLS_FIB, "mpls_trap");
  P_MPLS_FIB_MPLS_PHP_NEXTHOP =
      data_id_from_name(T_MPLS_FIB, A_MPLS_FIB_MPLS_PHP, "nexthop_index");
  P_MPLS_FIB_MPLS_PHP_POP_COUNT =
      data_id_from_name(T_MPLS_FIB, A_MPLS_FIB_MPLS_PHP, "pop_count");
  P_MPLS_FIB_MPLS_PHP_TTL_MODE =
      data_id_from_name(T_MPLS_FIB, A_MPLS_FIB_MPLS_PHP, "ttl_mode");
  P_MPLS_FIB_MPLS_PHP_QOS_MODE =
      data_id_from_name(T_MPLS_FIB, A_MPLS_FIB_MPLS_PHP, "qos_mode");
  P_MPLS_FIB_MPLS_SWAP_NEXTHOP =
      data_id_from_name(T_MPLS_FIB, A_MPLS_FIB_MPLS_SWAP, "nexthop_index");
  P_MPLS_FIB_MPLS_SWAP_POP_COUNT =
      data_id_from_name(T_MPLS_FIB, A_MPLS_FIB_MPLS_SWAP, "pop_count");

  P_MPLS_FIB_MPLS_TERM_BD =
      data_id_from_name(T_MPLS_FIB, A_MPLS_FIB_MPLS_TERM, "bd");
  P_MPLS_FIB_MPLS_TERM_VRF =
      data_id_from_name(T_MPLS_FIB, A_MPLS_FIB_MPLS_TERM, "vrf");
  P_MPLS_FIB_MPLS_TERM_VRF_TTL_VIOLATION =
      data_id_from_name(T_MPLS_FIB, A_MPLS_FIB_MPLS_TERM, "vrf_ttl_violation");
  P_MPLS_FIB_MPLS_TERM_VRF_TTL_VIOLATION_VALID = data_id_from_name(
      T_MPLS_FIB, A_MPLS_FIB_MPLS_TERM, "vrf_ttl_violation_valid");
  P_MPLS_FIB_MPLS_TERM_VRF_IP_OPTIONS_VIOLATION = data_id_from_name(
      T_MPLS_FIB, A_MPLS_FIB_MPLS_TERM, "vrf_ip_options_violation");
  P_MPLS_FIB_MPLS_TERM_BD_LABEL =
      data_id_from_name(T_MPLS_FIB, A_MPLS_FIB_MPLS_TERM, "bd_label");
  //  P_MPLS_FIB_MPLS_TERM_RID =
  //      data_id_from_name(T_MPLS_FIB, A_MPLS_FIB_MPLS_TERM, "rid");
  P_MPLS_FIB_MPLS_TERM_LEARN_MODE =
      data_id_from_name(T_MPLS_FIB, A_MPLS_FIB_MPLS_TERM, "learning_mode");
  P_MPLS_FIB_MPLS_TERM_V4_UNICAST_EN = data_id_from_name(
      T_MPLS_FIB, A_MPLS_FIB_MPLS_TERM, "ipv4_unicast_enable");
  P_MPLS_FIB_MPLS_TERM_V6_UNICAST_EN = data_id_from_name(
      T_MPLS_FIB, A_MPLS_FIB_MPLS_TERM, "ipv6_unicast_enable");
  P_MPLS_FIB_MPLS_TERM_POP_COUNT =
      data_id_from_name(T_MPLS_FIB, A_MPLS_FIB_MPLS_TERM, "pop_count");
  P_MPLS_FIB_MPLS_TERM_TTL_MODE =
      data_id_from_name(T_MPLS_FIB, A_MPLS_FIB_MPLS_TERM, "ttl_mode");
  P_MPLS_FIB_MPLS_TERM_QOS_MODE =
      data_id_from_name(T_MPLS_FIB, A_MPLS_FIB_MPLS_TERM, "qos_mode");

  D_MPLS_FIB_MPLS_COUNTER_SPEC_PKTS =
      data_id_from_name_noaction(T_MPLS_FIB, "$COUNTER_SPEC_PKTS");
  D_MPLS_FIB_MPLS_COUNTER_SPEC_BYTES =
      data_id_from_name_noaction(T_MPLS_FIB, "$COUNTER_SPEC_BYTES");

  T_MPLS_POP = table_id_from_name("pop_mpls");
  F_MPLS_POP_POP_COUNT =
      key_id_from_name(T_MPLS_POP, "local_md.tunnel.mpls_pop_count");
  A_MPLS_POP_POP1 = action_id_from_name(T_MPLS_POP, "mpls_pop1");
  A_MPLS_POP_POP2 = action_id_from_name(T_MPLS_POP, "mpls_pop2");
  A_MPLS_POP_POP3 = action_id_from_name(T_MPLS_POP, "mpls_pop3");
  switch_log(SWITCH_API_LEVEL_INFO,
             SWITCH_OBJECT_TYPE_NONE,
             "\n##################### End ID retrieval from BF-Runtime "
             "#####################\n");

  /* SFC */
  T_SFC_FILTER_EPOCH_REG = table_id_from_name(
      "SwitchIngress.sfc_epoch_init.suppression_epoch_duration");
  P_SFC_FILTER_EPOCH_REG_DURATION =
      data_id_from_name_noaction(T_SFC_FILTER_EPOCH_REG, "value");

  /* Port Stats */
  T_INGRESS_PORT_IP_STATS = table_id_from_name("ingress_ip_port_stats");
  F_INGRESS_PORT_IP_STATS_PORT =
      key_id_from_name(T_INGRESS_PORT_IP_STATS, "port");
  F_INGRESS_PORT_IP_STATS_HDR_IPV4_VALID =
      key_id_from_name(T_INGRESS_PORT_IP_STATS, "hdr.ipv4.$valid");
  F_INGRESS_PORT_IP_STATS_HDR_IPV6_VALID =
      key_id_from_name(T_INGRESS_PORT_IP_STATS, "hdr.ipv6.$valid");
  F_INGRESS_PORT_IP_STATS_DROP =
      key_id_from_name(T_INGRESS_PORT_IP_STATS, "drop");
  F_INGRESS_PORT_IP_STATS_COPY_TO_CPU =
      key_id_from_name(T_INGRESS_PORT_IP_STATS, "copy_to_cpu");
  F_INGRESS_PORT_IP_STATS_HDR_ETHERNET_DST_ADDR =
      key_id_from_name(T_INGRESS_PORT_IP_STATS, "hdr.ethernet.dst_addr");
  F_INGRESS_PORT_IP_STATS_PRIORITY =
      key_id_from_name(T_INGRESS_PORT_IP_STATS, "$MATCH_PRIORITY");
  A_INGRESS_PORT_IP_STATS_COUNT =
      action_id_from_name(T_INGRESS_PORT_IP_STATS, "ingress_ip_stats_count");
  D_INGRESS_PORT_IP_STATS_COUNTER_SPEC_PKTS =
      data_id_from_name(T_INGRESS_PORT_IP_STATS,
                        A_INGRESS_PORT_IP_STATS_COUNT,
                        "$COUNTER_SPEC_PKTS");
  D_INGRESS_PORT_IP_STATS_COUNTER_SPEC_BYTES =
      data_id_from_name(T_INGRESS_PORT_IP_STATS,
                        A_INGRESS_PORT_IP_STATS_COUNT,
                        "$COUNTER_SPEC_BYTES");

  T_EGRESS_PORT_IP_STATS = table_id_from_name("egress_ip_port_stats");
  F_EGRESS_PORT_IP_STATS_PORT =
      key_id_from_name(T_EGRESS_PORT_IP_STATS, "port");
  F_EGRESS_PORT_IP_STATS_HDR_IPV4_VALID =
      key_id_from_name(T_EGRESS_PORT_IP_STATS, "hdr.ipv4.$valid");
  F_EGRESS_PORT_IP_STATS_HDR_IPV6_VALID =
      key_id_from_name(T_EGRESS_PORT_IP_STATS, "hdr.ipv6.$valid");
  F_EGRESS_PORT_IP_STATS_DROP =
      key_id_from_name(T_EGRESS_PORT_IP_STATS, "drop");
  F_EGRESS_PORT_IP_STATS_COPY_TO_CPU =
      key_id_from_name(T_EGRESS_PORT_IP_STATS, "copy_to_cpu");
  F_EGRESS_PORT_IP_STATS_HDR_ETHERNET_DST_ADDR =
      key_id_from_name(T_EGRESS_PORT_IP_STATS, "hdr.ethernet.dst_addr");
  F_EGRESS_PORT_IP_STATS_PRIORITY =
      key_id_from_name(T_EGRESS_PORT_IP_STATS, "$MATCH_PRIORITY");
  A_EGRESS_PORT_IP_STATS_COUNT =
      action_id_from_name(T_EGRESS_PORT_IP_STATS, "egress_ip_stats_count");
  D_EGRESS_PORT_IP_STATS_COUNTER_SPEC_PKTS =
      data_id_from_name(T_EGRESS_PORT_IP_STATS,
                        A_EGRESS_PORT_IP_STATS_COUNT,
                        "$COUNTER_SPEC_PKTS");
  D_EGRESS_PORT_IP_STATS_COUNTER_SPEC_BYTES =
      data_id_from_name(T_EGRESS_PORT_IP_STATS,
                        A_EGRESS_PORT_IP_STATS_COUNT,
                        "$COUNTER_SPEC_BYTES");
  /* Rotate Hash */
  T_ROTATE_HASH = table_id_from_name("rotate_hash");
  A_ROTATE_BY_0 = action_id_from_name(T_ROTATE_HASH, "rotate_by_0");
  A_ROTATE_BY_1 = action_id_from_name(T_ROTATE_HASH, "rotate_by_1");
  A_ROTATE_BY_2 = action_id_from_name(T_ROTATE_HASH, "rotate_by_2");
  A_ROTATE_BY_3 = action_id_from_name(T_ROTATE_HASH, "rotate_by_3");
  A_ROTATE_BY_4 = action_id_from_name(T_ROTATE_HASH, "rotate_by_4");
  A_ROTATE_BY_5 = action_id_from_name(T_ROTATE_HASH, "rotate_by_5");
  A_ROTATE_BY_6 = action_id_from_name(T_ROTATE_HASH, "rotate_by_6");
  A_ROTATE_BY_7 = action_id_from_name(T_ROTATE_HASH, "rotate_by_7");
  A_ROTATE_BY_8 = action_id_from_name(T_ROTATE_HASH, "rotate_by_8");
  A_ROTATE_BY_9 = action_id_from_name(T_ROTATE_HASH, "rotate_by_9");
  A_ROTATE_BY_10 = action_id_from_name(T_ROTATE_HASH, "rotate_by_10");
  A_ROTATE_BY_11 = action_id_from_name(T_ROTATE_HASH, "rotate_by_11");
  A_ROTATE_BY_12 = action_id_from_name(T_ROTATE_HASH, "rotate_by_12");
  A_ROTATE_BY_13 = action_id_from_name(T_ROTATE_HASH, "rotate_by_13");
  A_ROTATE_BY_14 = action_id_from_name(T_ROTATE_HASH, "rotate_by_14");
  A_ROTATE_BY_15 = action_id_from_name(T_ROTATE_HASH, "rotate_by_15");

  T_INGRESS_FP_FOLD = table_id_from_name("pipe_0.SwitchIngress_0.fold");
  F_INGRESS_FP_IG_INTR_MD_INGRESS_PORT =
      key_id_from_name(T_INGRESS_FP_FOLD, "ig_intr_md.ingress_port");
  A_INGRESS_FP_FOLD_SET_EGRESS_PORT =
      action_id_from_name(T_INGRESS_FP_FOLD, "SwitchIngress_0.set_egress_port");
  P_INGRESS_FP_FOLD_SET_EGRESS_PORT_DEV_PORT = data_id_from_name(
      T_INGRESS_FP_FOLD, A_INGRESS_FP_FOLD_SET_EGRESS_PORT, "dev_port");

  T_BFD_TX_SESSION = table_id_from_name("bfd_tx_session");
  F_BFD_TX_SESSION_ID = key_id_from_name(T_BFD_TX_SESSION, "session_id");
  A_BFD_TX_SESSION_V4 =
      action_id_from_name(T_BFD_TX_SESSION, "bfd_tx_session_v4");
  P_BFD_TX_SESSION_V4_TX_MULT =
      data_id_from_name(T_BFD_TX_SESSION, A_BFD_TX_SESSION_V4, "tx_mult");
  P_BFD_TX_SESSION_V4_SESSION_ID =
      data_id_from_name(T_BFD_TX_SESSION, A_BFD_TX_SESSION_V4, "session_id");
  P_BFD_TX_SESSION_V4_VRF =
      data_id_from_name(T_BFD_TX_SESSION, A_BFD_TX_SESSION_V4, "vrf");
  P_BFD_TX_SESSION_V4_SIP =
      data_id_from_name(T_BFD_TX_SESSION, A_BFD_TX_SESSION_V4, "sip");
  P_BFD_TX_SESSION_V4_DIP =
      data_id_from_name(T_BFD_TX_SESSION, A_BFD_TX_SESSION_V4, "dip");
  P_BFD_TX_SESSION_V4_SPORT =
      data_id_from_name(T_BFD_TX_SESSION, A_BFD_TX_SESSION_V4, "sport");
  P_BFD_TX_SESSION_V4_DPORT =
      data_id_from_name(T_BFD_TX_SESSION, A_BFD_TX_SESSION_V4, "dport");
  A_BFD_TX_SESSION_V6 =
      action_id_from_name(T_BFD_TX_SESSION, "bfd_tx_session_v6");
  P_BFD_TX_SESSION_V6_TX_MULT =
      data_id_from_name(T_BFD_TX_SESSION, A_BFD_TX_SESSION_V6, "tx_mult");
  P_BFD_TX_SESSION_V6_SESSION_ID =
      data_id_from_name(T_BFD_TX_SESSION, A_BFD_TX_SESSION_V6, "session_id");
  P_BFD_TX_SESSION_V6_VRF =
      data_id_from_name(T_BFD_TX_SESSION, A_BFD_TX_SESSION_V6, "vrf");
  P_BFD_TX_SESSION_V6_SIP =
      data_id_from_name(T_BFD_TX_SESSION, A_BFD_TX_SESSION_V6, "sip");
  P_BFD_TX_SESSION_V6_DIP =
      data_id_from_name(T_BFD_TX_SESSION, A_BFD_TX_SESSION_V6, "dip");
  P_BFD_TX_SESSION_V6_SPORT =
      data_id_from_name(T_BFD_TX_SESSION, A_BFD_TX_SESSION_V6, "sport");
  P_BFD_TX_SESSION_V6_DPORT =
      data_id_from_name(T_BFD_TX_SESSION, A_BFD_TX_SESSION_V6, "dport");
  A_BFD_TX_SESSION_BFD_TX_DROP_PKT =
      action_id_from_name(T_BFD_TX_SESSION, "bfd_tx_drop_pkt");
  T_BFD_RX_SESSION = table_id_from_name("bfd_rx_session");
  F_BFD_RX_SESSION_HDR_BFD_MY_DISCRIMINATOR =
      key_id_from_name(T_BFD_RX_SESSION, "hdr.bfd.my_discriminator");
  F_BFD_RX_SESSION_HDR_BFD_YOUR_DISCRIMINATOR =
      key_id_from_name(T_BFD_RX_SESSION, "hdr.bfd.your_discriminator");
  F_BFD_RX_SESSION_HDR_BFD_VERSION =
      key_id_from_name(T_BFD_RX_SESSION, "hdr.bfd.version");
  F_BFD_RX_SESSION_HDR_BFD_FLAGS =
      key_id_from_name(T_BFD_RX_SESSION, "hdr.bfd.flags");
  F_BFD_RX_SESSION_HDR_BFD_DESIRED_MIN_TX_INTERVAL =
      key_id_from_name(T_BFD_RX_SESSION, "hdr.bfd.desired_min_tx_interval");
  F_BFD_RX_SESSION_HDR_BFD_REQ_MIN_RX_INTERVAL =
      key_id_from_name(T_BFD_RX_SESSION, "hdr.bfd.req_min_rx_interval");
  A_BFD_RX_SESSION_INFO =
      action_id_from_name(T_BFD_RX_SESSION, "bfd_rx_session_info");
  P_BFD_RX_SESSION_INFO_RX_MULT =
      data_id_from_name(T_BFD_RX_SESSION, A_BFD_RX_SESSION_INFO, "rx_mult");
  P_BFD_RX_SESSION_INFO_SESSION_ID =
      data_id_from_name(T_BFD_RX_SESSION, A_BFD_RX_SESSION_INFO, "session_id");
  P_BFD_RX_SESSION_INFO_PKTGEN_PIPE =
      data_id_from_name(T_BFD_RX_SESSION, A_BFD_RX_SESSION_INFO, "pktgen_pipe");
  P_BFD_RX_SESSION_INFO_RECIRC_PORT =
      data_id_from_name(T_BFD_RX_SESSION, A_BFD_RX_SESSION_INFO, "recirc_port");
  A_BFD_RX_SESSION_MISS =
      action_id_from_name(T_BFD_RX_SESSION, "bfd_rx_session_miss");
  T_BFD_RX_TIMER = table_id_from_name("bfd_rx_timer");
  F_BFD_RX_TIMER_LOCAL_MD_BFD_PKT_TX =
      key_id_from_name(T_BFD_RX_TIMER, "local_md.bfd.pkt_tx");
  F_BFD_RX_TIMER_LOCAL_MD_BFD_SESSION_ID =
      key_id_from_name(T_BFD_RX_TIMER, "local_md.bfd.session_id");
  A_BFD_RX_TIMER_RESET =
      action_id_from_name(T_BFD_RX_TIMER, "bfd_rx_timer_reset");
  P_BFD_RX_TIMER_RESET_SESSION_ID =
      data_id_from_name(T_BFD_RX_TIMER, A_BFD_RX_TIMER_RESET, "session_id");
  A_BFD_RX_TIMER_CHECK =
      action_id_from_name(T_BFD_RX_TIMER, "bfd_rx_timer_check");
  P_BFD_RX_TIMER_CHECK_SESSION_ID =
      data_id_from_name(T_BFD_RX_TIMER, A_BFD_RX_TIMER_CHECK, "session_id");
  T_BFD_RX_TIMER_REG =
      table_id_from_name("SwitchIngress.bfd_rx_timer.timer_reg");
  F_BFD_RX_TIMER_REG_INDEX =
      key_id_from_name(T_BFD_RX_TIMER_REG, "$REGISTER_INDEX");
  D_BFD_RX_TIMER_REG_DATA = data_id_from_name_noaction(
      T_BFD_RX_TIMER_REG, "SwitchIngress.bfd_rx_timer.timer_reg.f1");
  T_BFD_PKT_ACTION = table_id_from_name("bfd_pkt_action");
  F_BFD_PKT_ACTION_LOCAL_MD_BFD_PKT_TX =
      key_id_from_name(T_BFD_PKT_ACTION, "local_md.bfd.pkt_tx");
  F_BFD_PKT_ACTION_LOCAL_MD_BFD_PKT_ACTION =
      key_id_from_name(T_BFD_PKT_ACTION, "local_md.bfd.pkt_action");
  F_BFD_PKT_ACTION_LOCAL_MD_BFD_PKTGEN_PIPE =
      key_id_from_name(T_BFD_PKT_ACTION, "local_md.bfd.pktgen_pipe");
  F_BFD_PKT_ACTION_PRIORITY =
      key_id_from_name(T_INGRESS_SYSTEM_ACL, "$MATCH_PRIORITY");
  A_BFD_PKT_ACTION_BFD_PKT_TO_CPU =
      action_id_from_name(T_BFD_PKT_ACTION, "bfd_pkt_to_cpu");
  A_BFD_PKT_ACTION_BFD_RECIRC_TO_PKTGEN_PIPE =
      action_id_from_name(T_BFD_PKT_ACTION, "bfd_recirc_to_pktgen_pipe");
  A_BFD_PKT_ACTION_NOACTION = action_id_from_name(T_BFD_PKT_ACTION, "NoAction");
  A_BFD_PKT_ACTION_BFD_TX_PKT =
      action_id_from_name(T_BFD_PKT_ACTION, "bfd_tx_pkt");
  A_BFD_PKT_ACTION_BFD_DROP_PKT =
      action_id_from_name(T_BFD_PKT_ACTION, "bfd_drop_pkt");
  T_BFD_TX_TIMER = table_id_from_name("bfd_tx_timer");
  F_BFD_TX_TIMER_LOCAL_MD_BFD_SESSION_ID =
      key_id_from_name(T_BFD_TX_TIMER, "session_id");
  A_BFD_TX_TIMER_CHECK =
      action_id_from_name(T_BFD_TX_TIMER, "bfd_tx_timer_check");
  P_BFD_TX_TIMER_CHECK_SESSION_ID =
      data_id_from_name(T_BFD_TX_TIMER, A_BFD_TX_TIMER_CHECK, "session_id");
  P_BFD_TX_TIMER_CHECK_DETECT_MULTI =
      data_id_from_name(T_BFD_TX_TIMER, A_BFD_TX_TIMER_CHECK, "detect_multi");
  P_BFD_TX_TIMER_CHECK_MY_DISCRIMINATOR = data_id_from_name(
      T_BFD_TX_TIMER, A_BFD_TX_TIMER_CHECK, "my_discriminator");
  P_BFD_TX_TIMER_CHECK_YOUR_DISCRIMINATOR = data_id_from_name(
      T_BFD_TX_TIMER, A_BFD_TX_TIMER_CHECK, "your_discriminator");
  P_BFD_TX_TIMER_CHECK_DESIRED_MIN_TX_INTERVAL = data_id_from_name(
      T_BFD_TX_TIMER, A_BFD_TX_TIMER_CHECK, "desired_min_tx_interval");
  P_BFD_TX_TIMER_CHECK_REQ_MIN_RX_INTERVAL = data_id_from_name(
      T_BFD_TX_TIMER, A_BFD_TX_TIMER_CHECK, "req_min_rx_interval");
  A_BFD_TX_TIMER_BFD_DROP_PKT =
      action_id_from_name(T_BFD_TX_TIMER, "bfd_drop_pkt");

  T_INGRESS_PKTGEN_PORT = table_id_from_name("ingress_pktgen_port");
  F_INGRESS_PKTGEN_PORT_ETHER_TYPE =
      key_id_from_name(T_INGRESS_PKTGEN_PORT, "ether_type");
  F_INGRESS_PKTGEN_PORT_PORT = key_id_from_name(T_INGRESS_PKTGEN_PORT, "port");

  switch_log(
      SWITCH_API_LEVEL_WARN,
      SWITCH_OBJECT_TYPE_NONE,
      "\nThe above errors can be ignored. The messages are printed "
      "when looking \nfor tables and actions not available in the current "
      "profile or program\n");
  EXIT();

  bf_sys_log_level_set(BF_MOD_BFRT, BF_LOG_DEST_STDOUT, BF_LOG_ERR);
  bf_sys_log_level_set(BF_MOD_SWITCHAPI, BF_LOG_DEST_STDOUT, BF_LOG_INFO);
  bf_sys_syslog_level_set(BF_LOG_ERR);
}  // NOLINT(readability/fn_size)

const BfRtInfo *smi_id::rt_info = NULL;
bf_rt_table_id_t smi_id::A_NO_ACTION;

// pd fixed
bf_rt_table_id_t smi_id::T_PRE_MGID;
bf_rt_field_id_t smi_id::F_PRE_MGID_MGID;
bf_rt_field_id_t smi_id::D_PRE_MGID_MULTICAST_NODE_ID;
bf_rt_field_id_t smi_id::D_PRE_MGID_MULTICAST_NODE_L1_XID;
bf_rt_field_id_t smi_id::D_PRE_MGID_MULTICAST_NODE_L1_XID_VALID;

bf_rt_table_id_t smi_id::T_PRE_NODE;
bf_rt_field_id_t smi_id::F_PRE_NODE_MULTICAST_NODE_ID;
bf_rt_field_id_t smi_id::D_PRE_NODE_MULTICAST_RID;
bf_rt_field_id_t smi_id::D_PRE_NODE_MULTICAST_LAG_ID;
bf_rt_field_id_t smi_id::D_PRE_NODE_DEV_PORT;

bf_rt_table_id_t smi_id::T_PRE_LAG;
bf_rt_field_id_t smi_id::F_PRE_LAG_LAG_ID;
bf_rt_field_id_t smi_id::D_PRE_LAG_DEV_PORT;

bf_rt_table_id_t smi_id::T_PRE_PRUNE;
bf_rt_field_id_t smi_id::F_PRE_PRUNE_YID;
bf_rt_field_id_t smi_id::D_PRE_PRUNE_DEV_PORT;

// bfrt
bf_rt_table_id_t smi_id::T_TM_CFG;
bf_rt_field_id_t smi_id::D_TM_CFG_CELL_SIZE;
bf_rt_field_id_t smi_id::D_TM_CFG_TOTAL_CELLS;

bf_rt_table_id_t smi_id::T_TM_PPG_CFG;
bf_rt_field_id_t smi_id::D_TM_PPG_CFG_GUARANTEED_CELLS;

bf_rt_table_id_t smi_id::T_TM_POOL_CFG;
bf_rt_field_id_t smi_id::F_TM_POOL_CFG_POOL;
bf_rt_field_id_t smi_id::D_TM_POOL_CFG_SIZE_CELLS;

bf_rt_table_id_t smi_id::T_TM_PORT_BUFFER;
bf_rt_field_id_t smi_id::F_TM_PORT_BUFFER_DEV_PORT;
bf_rt_field_id_t smi_id::D_TM_PORT_BUFFER_SKID_LIMIT_CELLS;

bf_rt_table_id_t smi_id::T_TM_POOL_APP_PFC;
bf_rt_field_id_t smi_id::F_TM_POOL_APP_PFC_POOL;
bf_rt_field_id_t smi_id::F_TM_POOL_APP_PFC_COS;
bf_rt_field_id_t smi_id::D_TM_POOL_APP_PFC_LIMIT_CELLS;

// tm.counter.ig.port
bf_rt_table_id_t smi_id::T_TM_COUNTER_IG_PORT;
bf_rt_field_id_t smi_id::F_TM_COUNTER_IG_PORT_DEV_PORT;
bf_rt_field_id_t smi_id::D_TM_COUNTER_IG_PORT_DROP_COUNT;
bf_rt_field_id_t smi_id::D_TM_COUNTER_IG_PORT_USAGE_CELLS;
bf_rt_field_id_t smi_id::D_TM_COUNTER_IG_PORT_WATERMARK_CELLS;

// tm.counter.eg.port
bf_rt_table_id_t smi_id::T_TM_COUNTER_EG_PORT;
bf_rt_field_id_t smi_id::F_TM_COUNTER_EG_PORT_DEV_PORT;
bf_rt_field_id_t smi_id::D_TM_COUNTER_EG_PORT_DROP_COUNT;
bf_rt_field_id_t smi_id::D_TM_COUNTER_EG_PORT_USAGE_CELLS;
bf_rt_field_id_t smi_id::D_TM_COUNTER_EG_PORT_WATERMARK_CELLS;
// tm.port.sched_shaping
bf_rt_table_id_t smi_id::T_TM_PORT_SCHED_SHAPING;
bf_rt_field_id_t smi_id::F_TM_PORT_SCHED_SHAPING_DEV_PORT;
bf_rt_field_id_t smi_id::D_TM_PORT_SCHED_SHAPING_UNIT;
bf_rt_field_id_t smi_id::D_TM_PORT_SCHED_SHAPING_PROVISIONING;
bf_rt_field_id_t smi_id::D_TM_PORT_SCHED_SHAPING_MAX_RATE;
bf_rt_field_id_t smi_id::D_TM_PORT_SCHED_SHAPING_MAX_BURST_SIZE;

// tm.port.flowcontrol
bf_rt_table_id_t smi_id::T_TM_PORT_FLOWCONTROL;
bf_rt_field_id_t smi_id::F_TM_PORT_FLOWCONTROL_DEV_PORT;
bf_rt_field_id_t smi_id::D_TM_PORT_FLOWCONTROL_MODE_TX;
bf_rt_field_id_t smi_id::D_TM_PORT_FLOWCONTROL_MODE_RX;
bf_rt_field_id_t smi_id::D_TM_PORT_FLOWCONTROL_COS_TO_ICOS;

// tm.port.sched_cfg
bf_rt_table_id_t smi_id::T_TM_PORT_SCHED_CFG;
bf_rt_field_id_t smi_id::F_TM_PORT_SCHED_CFG_DEV_PORT;
bf_rt_field_id_t smi_id::D_TM_PORT_SCHED_CFG_MAX_RATE_ENABLE;

bf_rt_table_id_t smi_id::T_TM_COUNTER_POOL;
bf_rt_field_id_t smi_id::F_TM_COUNTER_POOL;
bf_rt_field_id_t smi_id::D_TM_COUNTER_POOL_WATERMARK_CELLS;
bf_rt_field_id_t smi_id::D_TM_COUNTER_POOL_USAGE_CELLS;

/* pvs */
bf_rt_table_id_t smi_id::T_ING_UDP_PORT_VXLAN;
bf_rt_field_id_t smi_id::F_ING_UDP_PORT_VXLAN_F1;
bf_rt_table_id_t smi_id::T_EG_UDP_PORT_VXLAN;
bf_rt_field_id_t smi_id::F_EG_UDP_PORT_VXLAN_F1;

// for folded pipeline only
bf_rt_table_id_t smi_id::T_ING1_UDP_PORT_VXLAN;
bf_rt_field_id_t smi_id::F_ING1_UDP_PORT_VXLAN_F1;
bf_rt_table_id_t smi_id::T_EG1_UDP_PORT_VXLAN;
bf_rt_field_id_t smi_id::F_EG1_UDP_PORT_VXLAN_F1;

bf_rt_table_id_t smi_id::T_INGRESS_CPU_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_CPU_PORT_ETHER_TYPE;
bf_rt_field_id_t smi_id::F_INGRESS_CPU_PORT_PORT;
bf_rt_table_id_t smi_id::T_EGRESS_CPU_PORT;
bf_rt_field_id_t smi_id::F_EGRESS_CPU_PORT_ETHER_TYPE;
bf_rt_field_id_t smi_id::F_EGRESS_CPU_PORT_PORT;
bf_rt_table_id_t smi_id::T_INTERNAL_PIPE_CPU_PORT;
bf_rt_field_id_t smi_id::F_INTERNAL_PIPE_CPU_PORT_ETHER_TYPE;
bf_rt_field_id_t smi_id::F_INTERNAL_PIPE_CPU_PORT_PORT;
bf_rt_table_id_t smi_id::T_INGRESS_PIPE_CPU_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_PIPE_CPU_PORT_ETHER_TYPE;
bf_rt_field_id_t smi_id::F_INGRESS_PIPE_CPU_PORT_PORT;

//  nvgre-st key
bf_rt_table_id_t smi_id::T_INGRESS_NVGRE_ST_KEY;
bf_rt_table_id_t smi_id::F_INGRESS_NVGRE_ST_KEY_VSID_FLOWID;
bf_rt_table_id_t smi_id::T_EGRESS_NVGRE_ST_KEY;
bf_rt_table_id_t smi_id::F_EGRESS_NVGRE_ST_KEY_VSID_FLOWID;

/*ipv4 dynamic hash algorithm */
bf_rt_table_id_t smi_id::T_IPV4_DYN_HASH_ALGORITHM;
bf_rt_table_id_t smi_id::A_IPV4_HASH_PREDEFINED;
bf_rt_table_id_t smi_id::A_IPV4_HASH_USERDEFINED;
bf_rt_table_id_t smi_id::P_IPV4_HASH_SEED;
bf_rt_table_id_t smi_id::P_IPV4_HASH_ALG;
bf_rt_table_id_t smi_id::P_IPV4_HASH_REV;
bf_rt_table_id_t smi_id::P_IPV4_HASH_POLY;
bf_rt_table_id_t smi_id::P_IPV4_HASH_INIT;
bf_rt_table_id_t smi_id::P_IPV4_HASH_FXOR;
bf_rt_table_id_t smi_id::P_IPV4_HASH_HBW;

/*ipv6 dynamic hash algorithm */
bf_rt_table_id_t smi_id::T_IPV6_DYN_HASH_ALGORITHM;
bf_rt_table_id_t smi_id::A_IPV6_HASH_PREDEFINED;
bf_rt_table_id_t smi_id::A_IPV6_HASH_USERDEFINED;
bf_rt_table_id_t smi_id::P_IPV6_HASH_SEED;
bf_rt_table_id_t smi_id::P_IPV6_HASH_ALG;
bf_rt_table_id_t smi_id::P_IPV6_HASH_REV;
bf_rt_table_id_t smi_id::P_IPV6_HASH_POLY;
bf_rt_table_id_t smi_id::P_IPV6_HASH_INIT;
bf_rt_table_id_t smi_id::P_IPV6_HASH_FXOR;
bf_rt_table_id_t smi_id::P_IPV6_HASH_HBW;

// outer ecmp
bf_rt_table_id_t smi_id::T_OUTER_IPV4_DYN_HASH_ALGORITHM;
bf_rt_table_id_t smi_id::A_OUTER_IPV4_HASH_PREDEFINED;
bf_rt_table_id_t smi_id::T_OUTER_IPV6_DYN_HASH_ALGORITHM;
bf_rt_table_id_t smi_id::A_OUTER_IPV6_HASH_PREDEFINED;

/*nonip dynamic hash algorithm */
bf_rt_table_id_t smi_id::T_NONIP_DYN_HASH_ALGORITHM;
bf_rt_table_id_t smi_id::A_NONIP_HASH_PREDEFINED;
bf_rt_table_id_t smi_id::A_NONIP_HASH_USERDEFINED;
bf_rt_table_id_t smi_id::P_NONIP_HASH_SEED;
bf_rt_table_id_t smi_id::P_NONIP_HASH_ALG;
bf_rt_table_id_t smi_id::P_NONIP_HASH_REV;
bf_rt_table_id_t smi_id::P_NONIP_HASH_POLY;
bf_rt_table_id_t smi_id::P_NONIP_HASH_INIT;
bf_rt_table_id_t smi_id::P_NONIP_HASH_FXOR;
bf_rt_table_id_t smi_id::P_NONIP_HASH_HBW;

/*LAG V4 dynamic hash algorithm */
bf_rt_table_id_t smi_id::T_LAGV4_DYN_HASH_ALGORITHM;
bf_rt_table_id_t smi_id::A_LAGV4_HASH_PREDEFINED;
bf_rt_table_id_t smi_id::A_LAGV4_HASH_USERDEFINED;
bf_rt_table_id_t smi_id::P_LAGV4_HASH_SEED;
bf_rt_table_id_t smi_id::P_LAGV4_HASH_ALG;
bf_rt_table_id_t smi_id::P_LAGV4_HASH_REV;
bf_rt_table_id_t smi_id::P_LAGV4_HASH_POLY;
bf_rt_table_id_t smi_id::P_LAGV4_HASH_INIT;
bf_rt_table_id_t smi_id::P_LAGV4_HASH_FXOR;
bf_rt_table_id_t smi_id::P_LAGV4_HASH_HBW;

/* LAG V6 dynamic hash algorithm */
bf_rt_table_id_t smi_id::T_LAGV6_DYN_HASH_ALGORITHM;
bf_rt_table_id_t smi_id::A_LAGV6_HASH_PREDEFINED;
bf_rt_table_id_t smi_id::A_LAGV6_HASH_USERDEFINED;
bf_rt_table_id_t smi_id::P_LAGV6_HASH_SEED;
bf_rt_table_id_t smi_id::P_LAGV6_HASH_ALG;
bf_rt_table_id_t smi_id::P_LAGV6_HASH_REV;
bf_rt_table_id_t smi_id::P_LAGV6_HASH_POLY;
bf_rt_table_id_t smi_id::P_LAGV6_HASH_INIT;
bf_rt_table_id_t smi_id::P_LAGV6_HASH_FXOR;
bf_rt_table_id_t smi_id::P_LAGV6_HASH_HBW;

/*ipv4_hash */
bf_rt_table_id_t smi_id::T_IPV4_HASH;
bf_rt_field_id_t smi_id::P_IPV4_HASH_SRC_ADDR;
bf_rt_field_id_t smi_id::P_IPV4_HASH_DST_ADDR;
bf_rt_field_id_t smi_id::P_IPV4_HASH_IP_PROTO;
bf_rt_field_id_t smi_id::P_IPV4_HASH_SRC_PORT;
bf_rt_field_id_t smi_id::P_IPV4_HASH_DST_PORT;

/*ipv6_hash */
bf_rt_table_id_t smi_id::T_IPV6_HASH;
bf_rt_field_id_t smi_id::P_IPV6_HASH_SRC_ADDR;
bf_rt_field_id_t smi_id::P_IPV6_HASH_DST_ADDR;
bf_rt_field_id_t smi_id::P_IPV6_HASH_IP_PROTO;
bf_rt_field_id_t smi_id::P_IPV6_HASH_SRC_PORT;
bf_rt_field_id_t smi_id::P_IPV6_HASH_DST_PORT;
bf_rt_field_id_t smi_id::P_IPV6_HASH_IPV6_FLOW_LABEL;

/* non-ip hash */
bf_rt_table_id_t smi_id::T_NON_IP_HASH;
bf_rt_field_id_t smi_id::P_NONIP_HASH_ING_PORT;
bf_rt_field_id_t smi_id::P_NONIP_HASH_MAC_TYPE;
bf_rt_field_id_t smi_id::P_NONIP_HASH_SRC_MAC;
bf_rt_field_id_t smi_id::P_NONIP_HASH_DST_MAC;

/* lag v4 hash */
bf_rt_table_id_t smi_id::T_LAG_V4_HASH;
bf_rt_field_id_t smi_id::P_LAG_V4_HASH_SRC_ADDR;
bf_rt_field_id_t smi_id::P_LAG_V4_HASH_DST_ADDR;
bf_rt_field_id_t smi_id::P_LAG_V4_HASH_IP_PROTO;
bf_rt_field_id_t smi_id::P_LAG_V4_HASH_DST_PORT;
bf_rt_field_id_t smi_id::P_LAG_V4_HASH_SRC_PORT;

/* lag v6 hash */
bf_rt_table_id_t smi_id::T_LAG_V6_HASH;
bf_rt_field_id_t smi_id::P_LAG_V6_HASH_SRC_ADDR;
bf_rt_field_id_t smi_id::P_LAG_V6_HASH_DST_ADDR;
bf_rt_field_id_t smi_id::P_LAG_V6_HASH_IP_PROTO;
bf_rt_field_id_t smi_id::P_LAG_V6_HASH_DST_PORT;
bf_rt_field_id_t smi_id::P_LAG_V6_HASH_SRC_PORT;
bf_rt_field_id_t smi_id::P_LAG_V6_HASH_FLOW_LABEL;

/* inner dtel v4 hash */
bf_rt_table_id_t smi_id::T_INNER_DTEL_V4_HASH;
bf_rt_field_id_t smi_id::P_INNER_DTEL_V4_HASH_SRC_ADDR;
bf_rt_field_id_t smi_id::P_INNER_DTEL_V4_HASH_DST_ADDR;
bf_rt_field_id_t smi_id::P_INNER_DTEL_V4_HASH_IP_PROTO;
bf_rt_field_id_t smi_id::P_INNER_DTEL_V4_HASH_DST_PORT;
bf_rt_field_id_t smi_id::P_INNER_DTEL_V4_HASH_SRC_PORT;

/* inner dtel v6 hash */
bf_rt_table_id_t smi_id::T_INNER_DTEL_V6_HASH;
bf_rt_field_id_t smi_id::P_INNER_DTEL_V6_HASH_SRC_ADDR;
bf_rt_field_id_t smi_id::P_INNER_DTEL_V6_HASH_DST_ADDR;
bf_rt_field_id_t smi_id::P_INNER_DTEL_V6_HASH_IP_PROTO;
bf_rt_field_id_t smi_id::P_INNER_DTEL_V6_HASH_DST_PORT;
bf_rt_field_id_t smi_id::P_INNER_DTEL_V6_HASH_SRC_PORT;
bf_rt_field_id_t smi_id::P_INNER_DTEL_V6_HASH_FLOW_LABEL;

/* Hash container inner fields ids */
bf_rt_field_id_t smi_id::P_HASH_CONTAINER_START_BIT;
bf_rt_field_id_t smi_id::P_HASH_CONTAINER_LENGTH;
bf_rt_field_id_t smi_id::P_HASH_CONTAINER_ORDER;

/* rmac.p4 */
bf_rt_action_id_t smi_id::A_RMAC_MISS;
bf_rt_action_id_t smi_id::A_RMAC_HIT;

bf_rt_table_id_t smi_id::T_INGRESS_PV_RMAC;
bf_rt_field_id_t smi_id::F_INGRESS_PV_RMAC_LOCAL_MD_INGRESS_PORT_LAG_INDEX;
bf_rt_field_id_t smi_id::F_INGRESS_PV_RMAC_HDR_VLAN_TAG0_VALID;
bf_rt_field_id_t smi_id::F_INGRESS_PV_RMAC_HDR_VLAN_TAG0_VID;
bf_rt_field_id_t smi_id::F_INGRESS_PV_RMAC_HDR_ETHERNET_DST_ADDR;
bf_rt_field_id_t smi_id::F_INGRESS_PV_RMAC_MATCH_PRIORITY;

bf_rt_table_id_t smi_id::T_INGRESS_VLAN_RMAC;
bf_rt_field_id_t smi_id::F_INGRESS_VLAN_RMAC_HDR_VLAN_TAG0_VID;
bf_rt_field_id_t smi_id::F_INGRESS_VLAN_RMAC_HDR_ETHERNET_DST_ADDR;

/* validation.p4 */
bf_rt_table_id_t smi_id::T_VALIDATE_ETHERNET;
bf_rt_field_id_t smi_id::F_VALIDATE_ETHERNET_ETHERNET_SRC_ADDR;
bf_rt_field_id_t smi_id::F_VALIDATE_ETHERNET_ETHERNET_DST_ADDR;
bf_rt_field_id_t smi_id::F_VALIDATE_ETHERNET_VLAN_0_VALID;
bf_rt_field_id_t smi_id::F_VALIDATE_ETHERNET_VLAN_1_VALID;
bf_rt_field_id_t smi_id::F_VALIDATE_ETHERNET_PRIORITY;
bf_rt_action_id_t smi_id::A_MALFORMED_ETH_PKT;
bf_rt_field_id_t smi_id::P_MALFORMED_ETH_PKT_REASON;
bf_rt_action_id_t smi_id::A_VALID_PKT_UNTAGGED;
bf_rt_field_id_t smi_id::P_VALID_PKT_UNTAGGED_PKT_TYPE;
bf_rt_action_id_t smi_id::A_VALID_PKT_TAGGED;
bf_rt_field_id_t smi_id::P_VALID_PKT_TAGGED_PKT_TYPE;
bf_rt_action_id_t smi_id::A_VALID_PKT_DOUBLE_TAGGED;
bf_rt_field_id_t smi_id::P_VALID_PKT_DOUBLE_TAGGED_PKT_TYPE;

bf_rt_table_id_t smi_id::T_VALIDATE_IP;
bf_rt_field_id_t smi_id::F_VALIDATE_IP_ARP_VALID;
bf_rt_field_id_t smi_id::F_VALIDATE_IP_IPV4_VALID;
bf_rt_field_id_t smi_id::F_VALIDATE_IP_IPV4_CHKSUM_ERR;
bf_rt_field_id_t smi_id::F_VALIDATE_IP_IPV4_VERSION;
bf_rt_field_id_t smi_id::F_VALIDATE_IP_IPV4_TTL;
bf_rt_field_id_t smi_id::F_VALIDATE_IP_IPV4_IHL;
bf_rt_field_id_t smi_id::F_VALIDATE_IP_IPV4_FLAGS;
bf_rt_field_id_t smi_id::F_VALIDATE_IP_IPV4_FRAG_OFFSET;
bf_rt_field_id_t smi_id::F_VALIDATE_IP_IPV4_SRC_ADDR;
bf_rt_field_id_t smi_id::F_VALIDATE_IP_IPV6_VALID;
bf_rt_field_id_t smi_id::F_VALIDATE_IP_IPV6_VERSION;
bf_rt_field_id_t smi_id::F_VALIDATE_IP_IPV6_HOP_LIMIT;
bf_rt_field_id_t smi_id::F_VALIDATE_IP_IPV6_SRC_ADDR;
bf_rt_field_id_t smi_id::F_VALIDATE_IP_MPLS_0_VALID;
bf_rt_field_id_t smi_id::F_VALIDATE_IP_MPLS_0_LABEL;
bf_rt_field_id_t smi_id::F_VALIDATE_IP_MPLS_1_VALID;
bf_rt_field_id_t smi_id::F_VALIDATE_IP_INNER_IPV4_VALID;
bf_rt_field_id_t smi_id::F_VALIDATE_IP_INNER_IPV6_VALID;
bf_rt_field_id_t smi_id::F_VALIDATE_IP_PRIORITY;
bf_rt_action_id_t smi_id::A_MALFORMED_IPV4_PKT;
bf_rt_field_id_t smi_id::P_MALFORMED_IPV4_PKT_REASON;
bf_rt_action_id_t smi_id::A_MALFORMED_IPV6_PKT;
bf_rt_field_id_t smi_id::P_MALFORMED_IPV6_PKT_REASON;
bf_rt_action_id_t smi_id::A_VALID_ARP_PKT;
bf_rt_action_id_t smi_id::A_VALID_IPV4_PKT;
bf_rt_field_id_t smi_id::P_VALID_IPV4_PKT_IP_FRAG;
bf_rt_field_id_t smi_id::P_VALID_IPV4_PKT_IS_LINK_LOCAL;
bf_rt_action_id_t smi_id::A_VALID_IPV6_PKT;
bf_rt_action_id_t smi_id::A_VALID_MPLS_PKT;
bf_rt_action_id_t smi_id::A_VALID_MPLS_NULL_PKT;
bf_rt_action_id_t smi_id::A_VALID_MPLS_NULL_IPV4_PKT;
bf_rt_action_id_t smi_id::A_VALID_MPLS_NULL_IPV6_PKT;
bf_rt_action_id_t smi_id::A_VALID_MPLS_ROUTER_ALERT_LABEL;

bf_rt_table_id_t smi_id::T_INNER_VALIDATE_ETHERNET;
bf_rt_field_id_t smi_id::F_INNER_VALIDATE_ETHERNET_ETHERNET_DST_ADDR;
bf_rt_field_id_t smi_id::F_INNER_VALIDATE_ETHERNET_ETHERNET_VALID;
bf_rt_field_id_t smi_id::F_INNER_VALIDATE_ETHERNET_PRIORITY;
bf_rt_field_id_t smi_id::F_INNER_VALIDATE_ETHERNET_IPV4_VALID;
bf_rt_field_id_t smi_id::F_INNER_VALIDATE_ETHERNET_IPV4_CHKSUM_ERR;
bf_rt_field_id_t smi_id::F_INNER_VALIDATE_ETHERNET_IPV4_VERSION;
bf_rt_field_id_t smi_id::F_INNER_VALIDATE_ETHERNET_IPV4_TTL;
bf_rt_field_id_t smi_id::F_INNER_VALIDATE_ETHERNET_IPV4_IHL;
bf_rt_field_id_t smi_id::F_INNER_VALIDATE_ETHERNET_IPV6_VALID;
bf_rt_field_id_t smi_id::F_INNER_VALIDATE_ETHERNET_IPV6_VERSION;
bf_rt_field_id_t smi_id::F_INNER_VALIDATE_ETHERNET_IPV6_HOP_LIMIT;
bf_rt_field_id_t smi_id::F_INNER_VALIDATE_ETHERNET_TCP_VALID;
bf_rt_field_id_t smi_id::F_INNER_VALIDATE_ETHERNET_UDP_VALID;
bf_rt_action_id_t smi_id::A_INNER_L2_MALFORMED_PKT;
bf_rt_field_id_t smi_id::P_INNER_L2_MALFORMED_PKT_REASON;
bf_rt_action_id_t smi_id::A_INNER_L3_MALFORMED_PKT;
bf_rt_field_id_t smi_id::P_INNER_L3_MALFORMED_PKT_REASON;
bf_rt_action_id_t smi_id::A_INNER_VALID_ETHERNET_PKT;
bf_rt_field_id_t smi_id::P_INNER_VALID_ETHERNET_PKT_TYPE;
bf_rt_action_id_t smi_id::A_INNER_VALID_IPV4_PKT;
bf_rt_field_id_t smi_id::P_INNER_VALID_IPV4_PKT_TYPE;
bf_rt_action_id_t smi_id::A_INNER_VALID_IPV6_PKT;
bf_rt_field_id_t smi_id::P_INNER_VALID_IPV6_PKT_TYPE;
bf_rt_action_id_t smi_id::A_INNER_VALID_IPV4_TCP_PKT;
bf_rt_field_id_t smi_id::P_INNER_VALID_IPV4_TCP_PKT_TYPE;
bf_rt_action_id_t smi_id::A_INNER_VALID_IPV4_UDP_PKT;
bf_rt_field_id_t smi_id::P_INNER_VALID_IPV4_UDP_PKT_TYPE;
bf_rt_action_id_t smi_id::A_INNER_VALID_IPV6_TCP_PKT;
bf_rt_field_id_t smi_id::P_INNER_VALID_IPV6_TCP_PKT_TYPE;
bf_rt_action_id_t smi_id::A_INNER_VALID_IPV6_UDP_PKT;
bf_rt_field_id_t smi_id::P_INNER_VALID_IPV6_UDP_PKT_TYPE;

bf_rt_table_id_t smi_id::T_INGRESS_SAME_MAC_CHECK;
bf_rt_field_id_t smi_id::F_INGRESS_SAME_MAC_CHECK_LOCAL_MD_SAME_MAC;
bf_rt_action_id_t smi_id::A_COMPUTE_SAME_MAC_CHECK;

/* validation_fp.p4 */
bf_rt_table_id_t smi_id::T_FP_VALIDATE_ETHERNET;
bf_rt_field_id_t smi_id::F_FP_VALIDATE_ETHERNET_ETHERNET_SRC_ADDR;
bf_rt_field_id_t smi_id::F_FP_VALIDATE_ETHERNET_ETHERNET_DST_ADDR;
bf_rt_field_id_t smi_id::F_FP_VALIDATE_ETHERNET_VLAN_0_VALID;
bf_rt_field_id_t smi_id::F_FP_VALIDATE_ETHERNET_VLAN_1_VALID;
bf_rt_field_id_t smi_id::F_FP_VALIDATE_ETHERNET_PRIORITY;
bf_rt_action_id_t smi_id::A_FP_MALFORMED_ETH_PKT;
bf_rt_field_id_t smi_id::P_FP_MALFORMED_ETH_PKT_REASON;
bf_rt_action_id_t smi_id::A_FP_VALID_PKT_UNTAGGED;
bf_rt_field_id_t smi_id::P_FP_VALID_PKT_UNTAGGED_PKT_TYPE;
bf_rt_action_id_t smi_id::A_FP_VALID_PKT_TAGGED;
bf_rt_field_id_t smi_id::P_FP_VALID_PKT_TAGGED_PKT_TYPE;
bf_rt_action_id_t smi_id::A_FP_VALID_PKT_DOUBLE_TAGGED;
bf_rt_field_id_t smi_id::P_FP_VALID_PKT_DOUBLE_TAGGED_PKT_TYPE;

bf_rt_table_id_t smi_id::T_FP_VALIDATE_IP;
bf_rt_field_id_t smi_id::F_FP_VALIDATE_IP_ARP_VALID;
bf_rt_field_id_t smi_id::F_FP_VALIDATE_IP_IPV4_VALID;
bf_rt_field_id_t smi_id::F_FP_VALIDATE_IP_IPV4_CHKSUM_ERR;
bf_rt_field_id_t smi_id::F_FP_VALIDATE_IP_IPV4_VERSION;
bf_rt_field_id_t smi_id::F_FP_VALIDATE_IP_IPV4_TTL;
bf_rt_field_id_t smi_id::F_FP_VALIDATE_IP_IPV4_IHL;
bf_rt_field_id_t smi_id::F_FP_VALIDATE_IP_IPV4_FLAGS;
bf_rt_field_id_t smi_id::F_FP_VALIDATE_IP_IPV4_FRAG_OFFSET;
bf_rt_field_id_t smi_id::F_FP_VALIDATE_IP_IPV4_SRC_ADDR;
bf_rt_field_id_t smi_id::F_FP_VALIDATE_IP_IPV6_VALID;
bf_rt_field_id_t smi_id::F_FP_VALIDATE_IP_IPV6_VERSION;
bf_rt_field_id_t smi_id::F_FP_VALIDATE_IP_IPV6_HOP_LIMIT;
bf_rt_field_id_t smi_id::F_FP_VALIDATE_IP_IPV6_SRC_ADDR;
bf_rt_field_id_t smi_id::F_FP_VALIDATE_IP_MPLS_0_VALID;
bf_rt_field_id_t smi_id::F_FP_VALIDATE_IP_MPLS_0_LABEL;
bf_rt_field_id_t smi_id::F_FP_VALIDATE_IP_MPLS_1_VALID;
bf_rt_field_id_t smi_id::F_FP_VALIDATE_IP_INNER_IPV4_VALID;
bf_rt_field_id_t smi_id::F_FP_VALIDATE_IP_INNER_IPV6_VALID;
bf_rt_field_id_t smi_id::F_FP_VALIDATE_IP_PRIORITY;
bf_rt_action_id_t smi_id::A_FP_MALFORMED_IPV4_PKT;
bf_rt_field_id_t smi_id::P_FP_MALFORMED_IPV4_PKT_REASON;
bf_rt_action_id_t smi_id::A_FP_MALFORMED_IPV6_PKT;
bf_rt_field_id_t smi_id::P_FP_MALFORMED_IPV6_PKT_REASON;
bf_rt_action_id_t smi_id::A_FP_VALID_ARP_PKT;
bf_rt_action_id_t smi_id::A_FP_VALID_IPV4_PKT;
bf_rt_field_id_t smi_id::P_FP_VALID_IPV4_PKT_IP_FRAG;
bf_rt_field_id_t smi_id::P_FP_VALID_IPV4_PKT_IS_LINK_LOCAL;
bf_rt_action_id_t smi_id::A_FP_VALID_IPV6_PKT;
bf_rt_action_id_t smi_id::A_FP_VALID_MPLS_PKT;
bf_rt_action_id_t smi_id::A_FP_VALID_MPLS_NULL_PKT;
bf_rt_action_id_t smi_id::A_FP_VALID_MPLS_NULL_IPV4_PKT;
bf_rt_action_id_t smi_id::A_FP_VALID_MPLS_NULL_IPV6_PKT;
bf_rt_action_id_t smi_id::A_FP_VALID_MPLS_ROUTER_ALERT_LABEL;

bf_rt_table_id_t smi_id::T_FP_INGRESS_SAME_MAC_CHECK;
bf_rt_field_id_t smi_id::F_FP_INGRESS_SAME_MAC_CHECK_LOCAL_MD_SAME_MAC;
bf_rt_action_id_t smi_id::A_FP_COMPUTE_SAME_MAC_CHECK;

/* NOTE: IDs from section below are not in use
   as long as constant entries of validate_ethernet table in control
   PktValidation1 in p4 code are sufficient */
bf_rt_table_id_t smi_id::T_FP_VALIDATE_ETHERNET_1;
bf_rt_field_id_t smi_id::F_FP_VALIDATE_ETHERNET_1_PRIORITY;
bf_rt_field_id_t smi_id::F_FP_VALIDATE_ETHERNET_1_IPV6_VALID;
bf_rt_field_id_t smi_id::F_FP_VALIDATE_ETHERNET_1_IPV6_VERSION;
bf_rt_field_id_t smi_id::F_FP_VALIDATE_ETHERNET_1_IPV6_HOP_LIMIT;
bf_rt_field_id_t smi_id::F_FP_VALIDATE_ETHERNET_1_IPV4_VALID;
bf_rt_field_id_t smi_id::F_FP_VALIDATE_ETHERNET_1_IPV4_CHKSUM_ERR;
bf_rt_field_id_t smi_id::F_FP_VALIDATE_ETHERNET_1_IPV4_VERSION;
bf_rt_field_id_t smi_id::F_FP_VALIDATE_ETHERNET_1_IPV4_IHL;
bf_rt_field_id_t smi_id::F_FP_VALIDATE_ETHERNET_1_IPV4_TTL;
bf_rt_field_id_t smi_id::F_FP_VALIDATE_ETHERNET_1_TCP_VALID;
bf_rt_field_id_t smi_id::F_FP_VALIDATE_ETHERNET_1_UDP_VALID;
bf_rt_action_id_t smi_id::A_FP_VALID_ETHERNET_PACKET_1;
bf_rt_field_id_t smi_id::P_FP_VALID_IPV4_ETHERNET_1_PKT_TYPE;
bf_rt_action_id_t smi_id::A_FP_VALID_IPV4_PKT_1;
bf_rt_field_id_t smi_id::P_FP_VALID_IPV4_PKT_1_PKT_TYPE;
bf_rt_action_id_t smi_id::A_FP_VALID_IPV6_PKT_1;
bf_rt_field_id_t smi_id::P_FP_VALID_IPV6_PKT_1_PKT_TYPE;
bf_rt_action_id_t smi_id::A_FP_VALID_IPV4_TCP_PKT_1;
bf_rt_field_id_t smi_id::P_FP_VALID_IPV4_TCP_PKT_1_PKT_TYPE;
bf_rt_action_id_t smi_id::A_FP_VALID_IPV4_UDP_PKT_1;
bf_rt_field_id_t smi_id::P_FP_VALID_IPV4_UDP_PKT_1_PKT_TYPE;
bf_rt_action_id_t smi_id::A_FP_VALID_IPV6_TCP_PKT_1;
bf_rt_field_id_t smi_id::P_FP_VALID_IPV6_TCP_PKT_1_PKT_TYPE;
bf_rt_action_id_t smi_id::A_FP_VALID_IPV6_UDP_PKT_1;
bf_rt_field_id_t smi_id::P_FP_VALID_IPV6_UDP_PKT_1_PKT_TYPE;
bf_rt_action_id_t smi_id::A_FP_MALFORMED_PKT_1;
bf_rt_field_id_t smi_id::P_FP_MALFORMED_PKT_1_REASON;

bf_rt_table_id_t smi_id::T_EGRESS_PKT_VALIDATION;

/* port.p4 */
bf_rt_table_id_t smi_id::T_PORT_METADATA;
bf_rt_field_id_t smi_id::F_PORT_METADATA_PORT;
bf_rt_field_id_t smi_id::P_PORT_METADATA_PORT_LAG_INDEX;
bf_rt_field_id_t smi_id::P_PORT_METADATA_PORT_LAG_LABEL;
bf_rt_field_id_t smi_id::P_PORT_METADATA_EXT_INGRESS_PORT;

bf_rt_table_id_t smi_id::T_INGRESS_PORT_MIRROR;
bf_rt_field_id_t smi_id::F_INGRESS_PORT_MIRROR_PORT;
bf_rt_action_id_t smi_id::A_INGRESS_PORT_MIRROR_SET_MIRROR_ID;
bf_rt_field_id_t smi_id::P_INGRESS_PORT_MIRROR_SET_MIRROR_ID_SESSION_ID;
bf_rt_field_id_t smi_id::P_INGRESS_PORT_MIRROR_SET_MIRROR_ID_METER_ID;
bf_rt_field_id_t smi_id::P_INGRESS_PORT_MIRROR_SET_MIRROR_ID_SRC;

bf_rt_table_id_t smi_id::T_EGRESS_PORT_MIRROR;
bf_rt_field_id_t smi_id::F_EGRESS_PORT_MIRROR_PORT;
bf_rt_action_id_t smi_id::A_EGRESS_PORT_MIRROR_SET_MIRROR_ID;
bf_rt_field_id_t smi_id::P_EGRESS_PORT_MIRROR_SET_MIRROR_ID_SESSION_ID;
bf_rt_field_id_t smi_id::P_EGRESS_PORT_MIRROR_SET_MIRROR_ID_METER_ID;
bf_rt_field_id_t smi_id::P_EGRESS_PORT_MIRROR_SET_MIRROR_ID_SRC;

bf_rt_table_id_t smi_id::T_INGRESS_PORT_MAPPING;
bf_rt_field_id_t smi_id::F_INGRESS_PORT_MAPPING_LOCAL_MD_INGRESS_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_PORT_MAPPING_HDR_CPU_VALID;
bf_rt_field_id_t smi_id::F_INGRESS_PORT_MAPPING_HDR_CPU_INGRESS_PORT;
bf_rt_action_id_t smi_id::A_SET_PORT_PROPERTIES;
bf_rt_field_id_t smi_id::P_SET_PORT_PROPERTIES_EXCLUSION_ID;
bf_rt_field_id_t smi_id::P_SET_PORT_PROPERTIES_LEARNING_MODE;
bf_rt_field_id_t smi_id::P_SET_PORT_PROPERTIES_COLOR;
bf_rt_field_id_t smi_id::P_SET_PORT_PROPERTIES_TC;
bf_rt_field_id_t smi_id::P_SET_PORT_PROPERTIES_SFLOW_SESSION_ID;
bf_rt_field_id_t smi_id::P_SET_PORT_PROPERTIES_IN_PORTS_GROUP_LABEL_IPV4;
bf_rt_field_id_t smi_id::P_SET_PORT_PROPERTIES_IN_PORTS_GROUP_LABEL_IPV6;
bf_rt_field_id_t smi_id::P_SET_PORT_PROPERTIES_IN_PORTS_GROUP_LABEL_MIRROR;
bf_rt_action_id_t smi_id::A_SET_CPU_PORT_PROPERTIES;
bf_rt_field_id_t smi_id::P_SET_CPU_PORT_PROPERTIES_PORT_LAG_INDEX;
bf_rt_field_id_t smi_id::P_SET_CPU_PORT_PROPERTIES_PORT_LAG_LABEL;
bf_rt_field_id_t smi_id::P_SET_CPU_PORT_PROPERTIES_EXCLUSION_ID;
bf_rt_field_id_t smi_id::P_SET_CPU_PORT_PROPERTIES_COLOR;
bf_rt_field_id_t smi_id::P_SET_CPU_PORT_PROPERTIES_TC;

bf_rt_table_id_t smi_id::T_PORT_VLAN_TO_BD_MAPPING;
bf_rt_field_id_t
    smi_id::F_PORT_VLAN_TO_BD_MAPPING_LOCAL_MD_INGRESS_PORT_LAG_INDEX;
bf_rt_field_id_t smi_id::F_PORT_VLAN_TO_BD_MAPPING_VLAN_0_VID;
bf_rt_field_id_t smi_id::F_PORT_VLAN_TO_BD_MAPPING_VLAN_0_VID_VALID;
bf_rt_field_id_t smi_id::F_PORT_VLAN_TO_BD_MAPPING_SRC_MAC_ADDRESS;
bf_rt_field_id_t smi_id::F_PORT_VLAN_TO_BD_MAPPING_PRIORITY;
bf_rt_field_id_t smi_id::D_PORT_VLAN_TO_BD_MAPPING_ACTION_MEMBER_ID;

bf_rt_table_id_t smi_id::T_PORT_DOUBLE_TAG_TO_BD_MAPPING;
bf_rt_field_id_t
    smi_id::F_PORT_DOUBLE_TAG_TO_BD_MAPPING_LOCAL_MD_INGRESS_PORT_LAG_INDEX;
bf_rt_field_id_t smi_id::F_PORT_DOUBLE_TAG_TO_BD_MAPPING_VLAN_0_VID;
bf_rt_field_id_t smi_id::F_PORT_DOUBLE_TAG_TO_BD_MAPPING_VLAN_0_VALID;
bf_rt_field_id_t smi_id::F_PORT_DOUBLE_TAG_TO_BD_MAPPING_VLAN_1_VID;
bf_rt_field_id_t smi_id::F_PORT_DOUBLE_TAG_TO_BD_MAPPING_VLAN_1_VALID;
bf_rt_field_id_t smi_id::D_PORT_DOUBLE_TAG_TO_BD_MAPPING_ACTION_MEMBER_ID;

bf_rt_table_id_t smi_id::T_VLAN_TO_BD_MAPPING;
bf_rt_field_id_t smi_id::F_VLAN_TO_BD_MAPPING_VLAN_0_VID;
bf_rt_field_id_t smi_id::D_VLAN_TO_BD_MAPPING_ACTION_MEMBER_ID;

bf_rt_table_id_t smi_id::T_VLAN_MEMBERSHIP;
bf_rt_field_id_t smi_id::F_VLAN_MEMBERSHIP_REGISTER_INDEX;
bf_rt_field_id_t smi_id::D_VLAN_MEMBERSHIP_REGISTER_DATA;

bf_rt_table_id_t smi_id::T_CPU_TO_BD_MAPPING;
bf_rt_field_id_t smi_id::F_CPU_TO_BD_MAPPING_HDR_CPU_INGRESS_BD;
bf_rt_field_id_t smi_id::D_CPU_TO_BD_MAPPING_ACTION_MEMBER_ID;

bf_rt_table_id_t smi_id::AP_BD_ACTION_PROFILE;
bf_rt_field_id_t smi_id::F_BD_ACTION_PROFILE_ACTION_MEMBER_ID;
bf_rt_action_id_t smi_id::A_PORT_VLAN_MISS;
bf_rt_action_id_t smi_id::A_INGRESS_SET_BD_PROPERTIES;
bf_rt_field_id_t smi_id::P_INGRESS_SET_BD_PROPERTIES_BD;
bf_rt_field_id_t smi_id::P_INGRESS_SET_BD_PROPERTIES_VRF_TTL_VIOLATION;
bf_rt_field_id_t smi_id::P_INGRESS_SET_BD_PROPERTIES_VRF_TTL_VIOLATION_VALID;
bf_rt_field_id_t smi_id::P_INGRESS_SET_BD_PROPERTIES_VRF_IP_OPTIONS_VIOLATION;
bf_rt_field_id_t
    smi_id::P_INGRESS_SET_BD_PROPERTIES_VRF_UNKNOWN_L3_MULTICAST_TRAP;
bf_rt_field_id_t smi_id::P_INGRESS_SET_BD_PROPERTIES_VRF;
bf_rt_field_id_t smi_id::P_INGRESS_SET_BD_PROPERTIES_BD_LABEL;
bf_rt_field_id_t smi_id::P_INGRESS_SET_BD_PROPERTIES_STP_GROUP;
bf_rt_field_id_t smi_id::P_INGRESS_SET_BD_PROPERTIES_LEARNING_MODE;
bf_rt_field_id_t smi_id::P_INGRESS_SET_BD_PROPERTIES_IPV4_UNICAST_ENABLE;
bf_rt_field_id_t smi_id::P_INGRESS_SET_BD_PROPERTIES_IPV6_UNICAST_ENABLE;
bf_rt_field_id_t smi_id::P_INGRESS_SET_BD_PROPERTIES_VLAN_ARP_SUPPRESS_ENABLE;
bf_rt_field_id_t smi_id::P_INGRESS_SET_BD_PROPERTIES_IPV4_MULTICAST_ENABLE;
bf_rt_field_id_t smi_id::P_INGRESS_SET_BD_PROPERTIES_IPV6_MULTICAST_ENABLE;
bf_rt_field_id_t smi_id::P_INGRESS_SET_BD_PROPERTIES_IGMP_SNOOPING_ENABLE;
bf_rt_field_id_t smi_id::P_INGRESS_SET_BD_PROPERTIES_MLD_SNOOPING_ENABLE;
bf_rt_field_id_t smi_id::P_INGRESS_SET_BD_PROPERTIES_MPLS_ENABLE;
bf_rt_field_id_t smi_id::P_INGRESS_SET_BD_PROPERTIES_MRPF_GROUP;
bf_rt_field_id_t smi_id::P_INGRESS_SET_BD_PROPERTIES_NAT_ZONE;

bf_rt_table_id_t smi_id::T_LAG;
bf_rt_field_id_t smi_id::F_LAG_PORT_LAG_INDEX;
bf_rt_field_id_t smi_id::F_LAG_PRIORITY;
bf_rt_field_id_t smi_id::D_LAG_ACTION_MEMBER_ID;
bf_rt_field_id_t smi_id::D_LAG_SELECTOR_GROUP_ID;

bf_rt_table_id_t smi_id::T_EGRESS_PORT_MAPPING;
bf_rt_field_id_t smi_id::F_EGRESS_PORT_MAPPING_PORT;
bf_rt_action_id_t smi_id::A_PORT_NORMAL;
bf_rt_field_id_t smi_id::P_PORT_NORMAL_PORT_LAG_INDEX;
bf_rt_field_id_t smi_id::P_PORT_NORMAL_PORT_LAG_LABEL;
bf_rt_field_id_t smi_id::P_PORT_NORMAL_OUT_PORTS_GROUP_LABEL_IPV4;
bf_rt_field_id_t smi_id::P_PORT_NORMAL_OUT_PORTS_GROUP_LABEL_IPV6;
bf_rt_action_id_t smi_id::A_PORT_CPU;

bf_rt_table_id_t smi_id::T_EGRESS_EGRESS_INGRESS_PORT_MAPPING;
bf_rt_field_id_t
    smi_id::F_EGRESS_EGRESS_INGRESS_PORT_MAPPING_LOCAL_MD_INGRESS_PORT;
bf_rt_action_id_t smi_id::A_SET_EGRESS_INGRESS_PORT_PROPERTIES;
bf_rt_field_id_t smi_id::D_SET_INGRESS_PORT_PROPERTIES_PORT_ISOLATION_GROUP;
bf_rt_field_id_t
    smi_id::D_SET_INGRESS_PORT_PROPERTIES_BRIDGE_PORT_ISOLATION_GROUP;

bf_rt_table_id_t smi_id::T_EGRESS_EGRESS_PORT_ISOLATION;
bf_rt_field_id_t smi_id::F_EGRESS_EGRESS_PORT_ISOLATION_EG_INTR_MD_EGRESS_PORT;
bf_rt_field_id_t
    smi_id::F_EGRESS_EGRESS_PORT_ISOLATION_LOCAL_MD_PORT_ISOLATION_GROUP;
bf_rt_action_id_t smi_id::A_ISOLATE_PACKET_PORT;
bf_rt_field_id_t smi_id::D_ISOLATE_PACKET_PORT_DROP;

bf_rt_table_id_t smi_id::T_EGRESS_EGRESS_BRIDGE_PORT_ISOLATION;
bf_rt_field_id_t
    smi_id::F_EGRESS_EGRESS_BRIDGE_PORT_ISOLATION_EG_INTR_MD_EGRESS_PORT;
bf_rt_field_id_t
    smi_id::F_EGRESS_EGRESS_BRIDGE_PORT_ISOLATION_LOCAL_MD_FLAGS_ROUTED;
bf_rt_field_id_t smi_id::
    F_EGRESS_EGRESS_BRIDGE_PORT_ISOLATION_LOCAL_MD_BRIDGE_PORT_ISOLATION_GROUP;
bf_rt_action_id_t smi_id::A_ISOLATE_PACKET_BPORT;
bf_rt_field_id_t smi_id::D_ISOLATE_PACKET_BPORT_DROP;

bf_rt_table_id_t smi_id::T_EGRESS_PEER_LINK_TUNNEL_ISOLATION;
bf_rt_field_id_t
    smi_id::F_EGRESS_PEER_LINK_TUNNEL_ISOLATION_LOCAL_MD_BPORT_ISOLATION_GROUP;
bf_rt_action_id_t smi_id::A_PEER_LINK_ISOLATE;
bf_rt_field_id_t smi_id::D_PEER_LINK_ISOLATE_DROP;

bf_rt_table_id_t smi_id::T_SNAKE;
bf_rt_field_id_t smi_id::F_SNAKE_INGRESS_PORT;
bf_rt_action_id_t smi_id::A_SNAKE_SET_EGRESS_PORT;
bf_rt_field_id_t smi_id::P_SNAKE_SET_EGRESS_PORT_EGRESS_PORT;

bf_rt_table_id_t smi_id::AP_LAG_SELECTOR;
bf_rt_field_id_t smi_id::F_LAG_SELECTOR_ACTION_MEMBER_ID;
bf_rt_action_id_t smi_id::A_LAG_MISS;
bf_rt_action_id_t smi_id::A_SET_LAG_PORT;
bf_rt_field_id_t smi_id::P_SET_LAG_PORT_PORT;

bf_rt_table_id_t smi_id::SG_LAG_SELECTOR_GROUP;
bf_rt_field_id_t smi_id::F_LAG_SELECTOR_GROUP_ID;
bf_rt_field_id_t smi_id::P_LAG_SELECTOR_GROUP_MAX_GROUP_SIZE;
bf_rt_field_id_t smi_id::P_LAG_SELECTOR_GROUP_MAX_MEMBER_ARRAY;
bf_rt_field_id_t smi_id::P_LAG_SELECTOR_GROUP_MAX_MEMBER_STATUS_ARRAY;

bf_rt_table_id_t smi_id::T_EGRESS_CPU_PORT_REWRITE;
bf_rt_field_id_t smi_id::F_EGRESS_CPU_PORT_REWRITE_PORT;
bf_rt_action_id_t smi_id::A_CPU_REWRITE;

bf_rt_table_id_t smi_id::T_INGRESS_PORT_STATE_EG_1;
bf_rt_field_id_t smi_id::F_INGRESS_PORT_STATE_EG_1_INGRESS_PORT;
bf_rt_table_id_t smi_id::T_INGRESS_PORT_STATE_IG_1;
bf_rt_field_id_t smi_id::F_INGRESS_PORT_STATE_IG_1_INGRESS_PORT;

bf_rt_action_id_t smi_id::A_INGRESS_PORT_STATE_IG_1_SET_PORT_PROPERTIES;
bf_rt_field_id_t
    smi_id::P_INGRESS_PORT_STATE_IG_1_SET_PORT_PROPERTIES_PORT_LAG_INDEX;
bf_rt_field_id_t
    smi_id::P_INGRESS_PORT_STATE_IG_1_SET_PORT_PROPERTIES_PORT_LAG_LABEL;
bf_rt_field_id_t
    smi_id::P_INGRESS_PORT_STATE_IG_1_SET_PORT_PROPERTIES_EXCLUSION_ID;
bf_rt_field_id_t
    smi_id::P_INGRESS_PORT_STATE_IG_1_SET_PORT_PROPERTIES_METER_INDEX;
bf_rt_field_id_t
    smi_id::P_INGRESS_PORT_STATE_IG_1_SET_PORT_PROPERTIES_SFLOW_SESSION_ID;

bf_rt_action_id_t smi_id::A_INGRESS_PORT_STATE_IG_1_SET_CPU_PORT_PROPERTIES;
bf_rt_field_id_t
    smi_id::P_INGRESS_PORT_STATE_IG_1_SET_CPU_PORT_PROPERTIES_PORT_LAG_INDEX;
bf_rt_field_id_t
    smi_id::P_INGRESS_PORT_STATE_IG_1_SET_CPU_PORT_PROPERTIES_PORT_LAG_LABEL;
bf_rt_field_id_t
    smi_id::P_INGRESS_PORT_STATE_IG_1_SET_CPU_PORT_PROPERTIES_EXCLUSION_ID;
bf_rt_field_id_t
    smi_id::P_INGRESS_PORT_STATE_IG_1_SET_CPU_PORT_PROPERTIES_METER_INDEX;
bf_rt_field_id_t
    smi_id::P_INGRESS_PORT_STATE_IG_1_SET_CPU_PORT_PROPERTIES_SFLOW_SESSION_ID;

bf_rt_action_id_t smi_id::A_INGRESS_PORT_STATE_EG_1_SET_PORT_PROPERTIES;
bf_rt_field_id_t
    smi_id::P_INGRESS_PORT_STATE_EG_1_SET_PORT_PROPERTIES_PORT_LAG_INDEX;
bf_rt_field_id_t
    smi_id::P_INGRESS_PORT_STATE_EG_1_SET_PORT_PROPERTIES_PORT_LAG_LABEL;
// bf_rt_field_id_t
// smi_id::P_INGRESS_PORT_STATE_EG_1_SET_PORT_PROPERTIES_EXCLUSION_ID;
// bf_rt_field_id_t
// smi_id::P_INGRESS_PORT_STATE_EG_1_SET_PORT_PROPERTIES_METER_INDEX;
// bf_rt_field_id_t
//    smi_id::P_INGRESS_PORT_STATE_EG_1_SET_PORT_PROPERTIES_SFLOW_SESSION_ID;

bf_rt_action_id_t smi_id::A_INGRESS_PORT_STATE_EG_1_SET_CPU_PORT_PROPERTIES;
bf_rt_field_id_t
    smi_id::P_INGRESS_PORT_STATE_EG_1_SET_CPU_PORT_PROPERTIES_PORT_LAG_INDEX;
bf_rt_field_id_t
    smi_id::P_INGRESS_PORT_STATE_EG_1_SET_CPU_PORT_PROPERTIES_PORT_LAG_LABEL;
// bf_rt_field_id_t
//    smi_id::P_INGRESS_PORT_STATE_EG_1_SET_CPU_PORT_PROPERTIES_EXCLUSION_ID;
// bf_rt_field_id_t
//    smi_id::P_INGRESS_PORT_STATE_EG_1_SET_CPU_PORT_PROPERTIES_METER_INDEX;
// bf_rt_field_id_t
//    smi_id::P_INGRESS_PORT_STATE_EG_1_SET_CPU_PORT_PROPERTIES_SFLOW_SESSION_ID;

// bf_rt_table_id_t smi_id::T_INGRESS_BD_STATE_EG_1;
// bf_rt_field_id_t smi_id::F_INGRESS_BD_STATE_EG_1_BD;
bf_rt_table_id_t smi_id::T_INGRESS_BD_STATE_IG_1;
bf_rt_field_id_t smi_id::F_INGRESS_BD_STATE_IG_1_BD;

bf_rt_action_id_t smi_id::A_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES;
bf_rt_field_id_t smi_id::P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_BD_LABEL;
bf_rt_field_id_t
    smi_id::P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_IPV4_UNICAST_ENABLE;
bf_rt_field_id_t
    smi_id::P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_IPV4_MULTICAST_ENABLE;
bf_rt_field_id_t
    smi_id::P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_IGMP_SNOOPING_ENABLE;
bf_rt_field_id_t
    smi_id::P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_IPV6_UNICAST_ENABLE;
bf_rt_field_id_t
    smi_id::P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_IPV6_MULTICAST_ENABLE;
bf_rt_field_id_t
    smi_id::P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_MLD_SNOOPING_ENABLE;
bf_rt_field_id_t smi_id::P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_MPLS_ENABLE;
bf_rt_field_id_t
    smi_id::P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_VLAN_ARP_SUPPRESS;
bf_rt_field_id_t smi_id::P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_VRF;
bf_rt_field_id_t
    smi_id::P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_VRF_TTL_VIOLATION;
bf_rt_field_id_t
    smi_id::P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_VRF_TTL_VIOLATION_VALID;
bf_rt_field_id_t
    smi_id::P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_VRF_IP_OPTIONS_VIOLATION;
bf_rt_field_id_t smi_id::P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_NAT_ZONE;

/*
bf_rt_action_id_t smi_id::A_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES;
bf_rt_field_id_t smi_id::P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_BD_LABEL;
bf_rt_field_id_t
  smi_id::P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_IPV4_UNICAST_ENABLE;
bf_rt_field_id_t
  smi_id::P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_IPV4_MULTICAST_ENABLE;
bf_rt_field_id_t
  smi_id::P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_IGMP_SNOOPING_ENABLE;
bf_rt_field_id_t
  smi_id::P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_IPV6_UNICAST_ENABLE;
bf_rt_field_id_t
  smi_id::P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_IPV6_MULTICAST_ENABLE;
bf_rt_field_id_t
  smi_id::P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_MLD_SNOOPING_ENABLE;
bf_rt_field_id_t smi_id::P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_MPLS_ENABLE;
bf_rt_field_id_t
  smi_id::P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_VLAN_ARP_SUPPRESS;
bf_rt_field_id_t smi_id::P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_VRF;
bf_rt_field_id_t
  smi_id::P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_VRF_TTL_VIOLATION;
bf_rt_field_id_t
  smi_id::P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_VRF_TTL_VIOLATION_VALID;
bf_rt_field_id_t
  smi_id::P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_VRF_IP_OPTIONS_VIOLATION;
bf_rt_field_id_t smi_id::P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_NAT_ZONE;
*/
/* l2.p4 */
bf_rt_table_id_t smi_id::T_INGRESS_STP_GROUP;
bf_rt_table_id_t smi_id::T_EGRESS_STP_GROUP;
bf_rt_table_id_t smi_id::T_SMAC;
bf_rt_table_id_t smi_id::T_DMAC;
bf_rt_table_id_t smi_id::T_INGRESS_BD_STATS;
bf_rt_table_id_t smi_id::T_EGRESS_BD_STATS;
bf_rt_table_id_t smi_id::T_EGRESS_BD_MAPPING;
bf_rt_table_id_t smi_id::T_VLAN_DECAP;
bf_rt_table_id_t smi_id::T_PORT_BD_TO_VLAN_MAPPING;
bf_rt_table_id_t smi_id::T_BD_TO_VLAN_MAPPING;
bf_rt_table_id_t smi_id::T_INGRESS_STP0_CHECK;
bf_rt_table_id_t smi_id::T_INGRESS_STP1_CHECK;
bf_rt_table_id_t smi_id::T_EGRESS_STP_CHECK;

bf_rt_field_id_t smi_id::F_INGRESS_STP_GROUP_PORT_LAG_INDEX;
bf_rt_field_id_t smi_id::F_INGRESS_STP_GROUP;
bf_rt_field_id_t smi_id::F_EGRESS_STP_GROUP_PORT_LAG_INDEX;
bf_rt_field_id_t smi_id::F_EGRESS_STP_GROUP;
bf_rt_field_id_t smi_id::F_INGRESS_STP0_CHECK_REGISTER_INDEX;
bf_rt_field_id_t smi_id::D_INGRESS_STP0_CHECK_REGISTER_DATA;
bf_rt_field_id_t smi_id::F_INGRESS_STP1_CHECK_REGISTER_INDEX;
bf_rt_field_id_t smi_id::D_INGRESS_STP1_CHECK_REGISTER_DATA;
bf_rt_field_id_t smi_id::F_EGRESS_STP_CHECK_REGISTER_INDEX;
bf_rt_field_id_t smi_id::D_EGRESS_STP_CHECK_REGISTER_DATA;
bf_rt_field_id_t smi_id::F_SMAC_LOCAL_MD_BD;
bf_rt_field_id_t smi_id::F_SMAC_SRC_ADDR;
bf_rt_field_id_t smi_id::F_DMAC_LOCAL_MD_BD;
bf_rt_field_id_t smi_id::F_DMAC_DST_ADDR;
bf_rt_field_id_t smi_id::F_INGRESS_BD_STATS_BD;
bf_rt_field_id_t smi_id::F_INGRESS_BD_STATS_PKT_TYPE;
bf_rt_field_id_t smi_id::F_EGRESS_BD_STATS_LOCAL_MD_BD;
bf_rt_field_id_t smi_id::F_EGRESS_BD_STATS_LOCAL_MD_PKT_TYPE;
bf_rt_field_id_t smi_id::F_EGRESS_BD_MAPPING_LOCAL_MD_BD;
bf_rt_field_id_t smi_id::F_VLAN_DECAP_PORT;
bf_rt_field_id_t smi_id::F_VLAN_DECAP_VLAN_0_VALID;
bf_rt_field_id_t smi_id::F_VLAN_DECAP_VLAN_1_VALID;
bf_rt_field_id_t smi_id::F_VLAN_DECAP_PRIORITY;
bf_rt_field_id_t smi_id::F_PORT_BD_TO_VLAN_MAPPING_PORT_LAG_INDEX;
bf_rt_field_id_t smi_id::F_PORT_BD_TO_VLAN_MAPPING_BD;
bf_rt_field_id_t smi_id::F_BD_TO_VLAN_MAPPING_BD;

bf_rt_action_id_t smi_id::A_INGRESS_STP_SET_STP_STATE;
bf_rt_field_id_t smi_id::P_INGRESS_STP_SET_STP_STATE_STP_STATE;
bf_rt_action_id_t smi_id::A_EGRESS_STP_SET_STP_STATE;
bf_rt_field_id_t smi_id::P_EGRESS_STP_SET_STP_STATE_STP_STATE;
bf_rt_action_id_t smi_id::A_SMAC_HIT;
bf_rt_field_id_t smi_id::P_SMAC_HIT_PORT_LAG_INDEX;
bf_rt_field_id_t smi_id::D_SMAC_TTL;
bf_rt_action_id_t smi_id::A_DMAC_MISS;
bf_rt_action_id_t smi_id::A_DMAC_HIT;
bf_rt_field_id_t smi_id::P_DMAC_HIT_PORT_LAG_INDEX;
bf_rt_action_id_t smi_id::A_DMAC_MULTICAST;
bf_rt_field_id_t smi_id::P_DMAC_MULTICAST_INDEX;
bf_rt_action_id_t smi_id::A_DMAC_REDIRECT;
bf_rt_field_id_t smi_id::P_DMAC_REDIRECT_NEXTHOP_INDEX;
bf_rt_action_id_t smi_id::A_INGRESS_BD_STATS_COUNT;
bf_rt_field_id_t smi_id::P_INGRESS_BD_STATS_BYTES;
bf_rt_field_id_t smi_id::P_INGRESS_BD_STATS_PKTS;
bf_rt_action_id_t smi_id::A_EGRESS_BD_STATS_COUNT;
bf_rt_field_id_t smi_id::P_EGRESS_BD_STATS_BYTES;
bf_rt_field_id_t smi_id::P_EGRESS_BD_STATS_PKTS;
bf_rt_action_id_t smi_id::A_EGRESS_SET_BD_PROPERTIES;
bf_rt_field_id_t smi_id::P_EGRESS_SET_BD_PROPERTIES_SMAC;
bf_rt_field_id_t smi_id::P_EGRESS_SET_BD_PROPERTIES_MTU;
bf_rt_field_id_t smi_id::P_EGRESS_SET_BD_PROPERTIES_BD_LABEL;
bf_rt_action_id_t smi_id::A_REMOVE_VLAN_TAG;
bf_rt_action_id_t smi_id::A_REMOVE_DOUBLE_TAG;
bf_rt_action_id_t smi_id::A_PORT_BD_TO_VLAN_MAPPING_SET_VLAN_UNTAGGED;
bf_rt_action_id_t smi_id::A_PORT_BD_TO_VLAN_MAPPING_SET_VLAN_TAGGED;
bf_rt_field_id_t smi_id::P_PORT_BD_TO_VLAN_MAPPING_SET_VLAN_TAGGED_VID;
bf_rt_action_id_t smi_id::A_PORT_BD_TO_VLAN_MAPPING_SET_DOUBLE_TAGGED;
bf_rt_field_id_t smi_id::P_PORT_BD_TO_VLAN_MAPPING_SET_DOUBLE_TAGGED_VID0;
bf_rt_field_id_t smi_id::P_PORT_BD_TO_VLAN_MAPPING_SET_DOUBLE_TAGGED_VID1;
bf_rt_action_id_t smi_id::A_BD_TO_VLAN_MAPPING_SET_VLAN_UNTAGGED;
bf_rt_action_id_t smi_id::A_BD_TO_VLAN_MAPPING_SET_VLAN_TAGGED;
bf_rt_field_id_t smi_id::P_BD_TO_VLAN_MAPPING_SET_VLAN_TAGGED_VID;

/* l3.p4 */
bf_rt_table_id_t smi_id::T_IPV4_FIB_HOST;
bf_rt_table_id_t smi_id::T_IPV4_FIB_LOCAL_HOST;
bf_rt_table_id_t smi_id::T_IPV4_FIB_LPM;
bf_rt_table_id_t smi_id::T_IPV6_FIB_HOST;
bf_rt_table_id_t smi_id::T_IPV6_FIB_HOST64;
bf_rt_table_id_t smi_id::T_IPV6_FIB_LPM;
bf_rt_table_id_t smi_id::T_IPV6_FIB_LPM64;
bf_rt_table_id_t smi_id::T_IPV6_FIB_LPM_TCAM;
bf_rt_table_id_t smi_id::T_IP_FIB_LPM64;

bf_rt_field_id_t smi_id::F_IPV4_FIB_VRF;
bf_rt_field_id_t smi_id::F_IPV4_FIB_VRF_SIZE;
bf_rt_field_id_t smi_id::F_IPV4_FIB_DST_ADDR;
bf_rt_field_id_t smi_id::F_IPV4_FIB_LOCAL_HOST_VRF;
bf_rt_field_id_t smi_id::F_IPV4_FIB_LOCAL_HOST_DST_ADDR;
bf_rt_field_id_t smi_id::F_IPV4_FIB_LPM_VRF;
bf_rt_field_id_t smi_id::F_IPV4_FIB_LPM_DST_ADDR;
bf_rt_field_id_t smi_id::F_IPV6_FIB_VRF;
bf_rt_field_id_t smi_id::F_IPV6_FIB_DST_ADDR;
bf_rt_field_id_t smi_id::F_IPV6_FIB_HOST64_VRF;
bf_rt_field_id_t smi_id::F_IPV6_FIB_HOST64_DST_ADDR;
bf_rt_field_id_t smi_id::F_IPV6_FIB_LPM_VRF;
bf_rt_field_id_t smi_id::F_IPV6_FIB_LPM_DST_ADDR;
bf_rt_field_id_t smi_id::F_IPV6_FIB_LPM64_VRF;
bf_rt_field_id_t smi_id::F_IPV6_FIB_LPM64_DST_ADDR;
bf_rt_field_id_t smi_id::F_IPV6_FIB_LPM_TCAM_VRF;
bf_rt_field_id_t smi_id::F_IPV6_FIB_LPM_TCAM_DST_ADDR;
bf_rt_field_id_t smi_id::F_IP_FIB_LPM64_VRF;
bf_rt_field_id_t smi_id::F_IP_FIB_LPM64_DST_ADDR;

bf_rt_action_id_t smi_id::A_FIB_HIT;
bf_rt_field_id_t smi_id::P_FIB_HIT_NEXTHOP_INDEX;
bf_rt_field_id_t smi_id::P_FIB_HIT_FIB_LABEL;
bf_rt_action_id_t smi_id::A_FIB_DROP;
bf_rt_action_id_t smi_id::A_FIB_MYIP;
bf_rt_field_id_t smi_id::P_FIB_MYIP_MYIP;

/* nexthop.p4 */
bf_rt_table_id_t smi_id::T_NEXTHOP;
bf_rt_field_id_t smi_id::F_NEXTHOP_LOCAL_MD_NEXTHOP;
size_t smi_id::F_NEXTHOP_INDEX_WIDTH;
bf_rt_action_id_t smi_id::A_SET_NEXTHOP_PROPERTIES;
bf_rt_field_id_t smi_id::P_SET_NEXTHOP_PROPERTIES_PORT_LAG_INDEX;
bf_rt_field_id_t smi_id::P_SET_NEXTHOP_PROPERTIES_BD;
bf_rt_action_id_t smi_id::A_SET_NEXTHOP_PROPERTIES_PR_FLOOD;
bf_rt_field_id_t smi_id::P_SET_NEXTHOP_PROPERTIES_PR_FLOOD_BD;
bf_rt_field_id_t smi_id::P_SET_NEXTHOP_PROPERTIES_PR_FLOOD_MGID;
bf_rt_field_id_t smi_id::P_SET_NEXTHOP_PROPERTIES_PR_FLOOD_NAT_ZONE;
bf_rt_action_id_t smi_id::A_SET_NEXTHOP_PROPERTIES_GLEAN;
bf_rt_field_id_t smi_id::P_SET_NEXTHOP_PROPERTIES_GLEAN_TRAP_ID;
bf_rt_action_id_t smi_id::A_SET_NEXTHOP_PROPERTIES_DROP;
bf_rt_action_id_t smi_id::P_SET_NEXTHOP_PROPERTIES_DROP_DROP_REASON;
bf_rt_field_id_t smi_id::P_SET_NEXTHOP_PROPERTIES_NAT_ZONE;
bf_rt_action_id_t smi_id::A_SET_NEXTHOP_PROPERTIES_TUNNEL;
bf_rt_field_id_t smi_id::P_SET_NEXTHOP_PROPERTIES_TUNNEL_DIP_INDEX;

bf_rt_table_id_t smi_id::T_OUTER_FIB;
bf_rt_field_id_t smi_id::F_OUTER_FIB_LOCAL_MD_TUNNEL_DIP_INDEX;
bf_rt_field_id_t smi_id::D_OUTER_FIB_ACTION_MEMBER_ID;
bf_rt_field_id_t smi_id::D_OUTER_FIB_SELECTOR_GROUP_ID;
bf_rt_action_id_t smi_id::A_SET_OUTER_NEXTHOP_PROPERTIES;
bf_rt_field_id_t smi_id::P_SET_OUTER_NEXTHOP_PROPERTIES_PORT_LAG_INDEX;
bf_rt_field_id_t smi_id::P_SET_OUTER_NEXTHOP_PROPERTIES_NEXTHOP_INDEX;

bf_rt_table_id_t smi_id::AP_OUTER_ECMP_SELECTOR;
bf_rt_field_id_t smi_id::F_OUTER_ECMP_SELECTOR_ACTION_MEMBER_ID;

bf_rt_table_id_t smi_id::SG_OUTER_ECMP_SELECTOR_GROUP;
bf_rt_field_id_t smi_id::F_OUTER_ECMP_SELECTOR_GROUP_ID;
bf_rt_field_id_t smi_id::P_OUTER_ECMP_SELECTOR_GROUP_MAX_GROUP_SIZE;
bf_rt_field_id_t smi_id::P_OUTER_ECMP_SELECTOR_GROUP_MAX_MEMBER_ARRAY;
bf_rt_field_id_t smi_id::P_OUTER_ECMP_SELECTOR_GROUP_MAX_MEMBER_STATUS_ARRAY;

bf_rt_table_id_t smi_id::T_ECMP;
bf_rt_field_id_t smi_id::F_ECMP_LOCAL_MD_NEXTHOP;
bf_rt_field_id_t smi_id::D_ECMP_ACTION_MEMBER_ID;
bf_rt_field_id_t smi_id::D_ECMP_SELECTOR_GROUP_ID;

bf_rt_table_id_t smi_id::AP_ECMP_SELECTOR;
bf_rt_field_id_t smi_id::F_ECMP_SELECTOR_ACTION_MEMBER_ID;
bf_rt_action_id_t smi_id::A_SET_ECMP_PROPERTIES;
bf_rt_field_id_t smi_id::P_SET_ECMP_PROPERTIES_PORT_LAG_INDEX;
bf_rt_field_id_t smi_id::P_SET_ECMP_PROPERTIES_BD;
bf_rt_field_id_t smi_id::P_SET_ECMP_PROPERTIES_NEXTHOP_INDEX;
bf_rt_action_id_t smi_id::A_SET_ECMP_PROPERTIES_DROP;
bf_rt_action_id_t smi_id::A_SET_ECMP_PROPERTIES_TUNNEL;
bf_rt_field_id_t smi_id::P_SET_ECMP_PROPERTIES_TUNNEL_NEXTHOP_INDEX;
bf_rt_field_id_t smi_id::P_SET_ECMP_PROPERTIES_TUNNEL_DIP_INDEX;
bf_rt_action_id_t smi_id::A_SET_ECMP_PROPERTIES_GLEAN;
bf_rt_field_id_t smi_id::P_SET_ECMP_PROPERTIES_GLEAN_TRAP_ID;

bf_rt_table_id_t smi_id::SG_ECMP_SELECTOR_GROUP;
bf_rt_field_id_t smi_id::F_ECMP_SELECTOR_GROUP_ID;
bf_rt_field_id_t smi_id::P_ECMP_SELECTOR_GROUP_MAX_GROUP_SIZE;
bf_rt_field_id_t smi_id::P_ECMP_SELECTOR_GROUP_MAX_MEMBER_ARRAY;
bf_rt_field_id_t smi_id::P_ECMP_SELECTOR_GROUP_MAX_MEMBER_STATUS_ARRAY;

bf_rt_table_id_t smi_id::T_NEIGHBOR;
bf_rt_field_id_t smi_id::F_NEIGHBOR_LOCAL_MD_NEXTHOP;
bf_rt_action_id_t smi_id::A_NEIGHBOR_REWRITE_L2;
bf_rt_field_id_t smi_id::P_NEIGHBOR_REWRITE_L2_DMAC;

bf_rt_table_id_t smi_id::T_OUTER_NEXTHOP;
bf_rt_field_id_t smi_id::F_OUTER_NEXTHOP_LOCAL_MD_NEXTHOP;
bf_rt_action_id_t smi_id::A_OUTER_NEXTHOP_REWRITE_L2;
bf_rt_field_id_t smi_id::P_OUTER_NEXTHOP_REWRITE_L2_BD;

/* acl.p4 */
bf_rt_table_id_t smi_id::T_INGRESS_MAC_ACL;
bf_rt_field_id_t smi_id::F_INGRESS_MAC_ACL_LOCAL_MD_LKP_MAC_SRC_ADDR;
bf_rt_field_id_t smi_id::F_INGRESS_MAC_ACL_LOCAL_MD_LKP_MAC_DST_ADDR;
bf_rt_field_id_t smi_id::F_INGRESS_MAC_ACL_HDR_MAC_SRC_ADDR;
bf_rt_field_id_t smi_id::F_INGRESS_MAC_ACL_HDR_MAC_DST_ADDR;
bf_rt_field_id_t smi_id::F_INGRESS_MAC_ACL_LOCAL_MD_LKP_MAC_TYPE;
bf_rt_field_id_t smi_id::F_INGRESS_MAC_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_MAC_ACL_LOCAL_MD_INGRESS_PORT_LAG_INDEX;
bf_rt_field_id_t smi_id::F_INGRESS_MAC_ACL_LOCAL_MD_BD_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_MAC_ACL_LOCAL_MD_BD;
bf_rt_field_id_t smi_id::F_INGRESS_MAC_ACL_LOCAL_MD_LKP_PCP;
bf_rt_field_id_t smi_id::F_INGRESS_MAC_ACL_LOCAL_MD_LKP_DEI;
bf_rt_field_id_t smi_id::F_INGRESS_MAC_ACL_HDR_VLAN_TAG0_VALID;
bf_rt_field_id_t smi_id::F_INGRESS_MAC_ACL_MATCH_PRIORITY;
bf_rt_field_id_t smi_id::F_INGRESS_MAC_ACL_LOCAL_MD_FLAGS_RMAC_HIT;
bf_rt_action_id_t smi_id::A_INGRESS_MAC_ACL_NO_ACTION;
bf_rt_action_id_t smi_id::A_INGRESS_MAC_ACL_DROP;
bf_rt_action_id_t smi_id::A_INGRESS_MAC_ACL_PERMIT;
bf_rt_field_id_t smi_id::P_INGRESS_MAC_ACL_PERMIT_USER_METADATA;
bf_rt_field_id_t smi_id::P_INGRESS_MAC_ACL_PERMIT_METER_INDEX;
bf_rt_field_id_t smi_id::P_INGRESS_MAC_ACL_PERMIT_TRAP_ID;
bf_rt_action_id_t smi_id::A_INGRESS_MAC_ACL_REDIRECT_NEXTHOP;
bf_rt_field_id_t smi_id::P_INGRESS_MAC_ACL_REDIRECT_NEXTHOP_INDEX;
bf_rt_field_id_t smi_id::P_INGRESS_MAC_ACL_REDIRECT_NEXTHOP_USER_METADATA;
bf_rt_action_id_t smi_id::A_INGRESS_MAC_ACL_REDIRECT_PORT;
bf_rt_field_id_t smi_id::P_INGRESS_MAC_ACL_REDIRECT_PORT_LAG_INDEX;
bf_rt_field_id_t smi_id::P_INGRESS_MAC_ACL_REDIRECT_PORT_USER_METADATA;
bf_rt_action_id_t smi_id::A_INGRESS_MAC_ACL_MIRROR;
bf_rt_field_id_t smi_id::P_INGRESS_MAC_ACL_MIRROR_METER_INDEX;
bf_rt_field_id_t smi_id::P_INGRESS_MAC_ACL_MIRROR_SESSION_ID;
bf_rt_action_id_t smi_id::A_INGRESS_MAC_ACL_SET_TC;
bf_rt_field_id_t smi_id::P_INGRESS_MAC_ACL_SET_TC_TC;
bf_rt_action_id_t smi_id::A_INGRESS_MAC_ACL_SET_COLOR;
bf_rt_field_id_t smi_id::P_INGRESS_MAC_ACL_SET_COLOR_COLOR;
bf_rt_action_id_t smi_id::A_INGRESS_MAC_ACL_TRAP;
bf_rt_field_id_t smi_id::P_INGRESS_MAC_ACL_TRAP_TRAP_ID;
bf_rt_field_id_t smi_id::P_INGRESS_MAC_ACL_TRAP_METER_INDEX;
bf_rt_action_id_t smi_id::A_INGRESS_MAC_ACL_COPY;
bf_rt_field_id_t smi_id::P_INGRESS_MAC_ACL_COPY_TRAP_ID;
bf_rt_field_id_t smi_id::P_INGRESS_MAC_ACL_COPY_METER_INDEX;
bf_rt_action_id_t smi_id::A_INGRESS_MAC_ACL_SET_DTEL_REPORT_TYPE;
bf_rt_field_id_t smi_id::P_INGRESS_MAC_ACL_DTEL_REPORT_TYPE;
bf_rt_field_id_t smi_id::D_INGRESS_MAC_ACL_COUNTER_SPEC_BYTES;
bf_rt_field_id_t smi_id::D_INGRESS_MAC_ACL_COUNTER_SPEC_PKTS;
bf_rt_action_id_t smi_id::A_INGRESS_MAC_ACL_TRANSIT;
bf_rt_action_id_t smi_id::A_INGRESS_MAC_ACL_DENY;
bf_rt_action_id_t smi_id::A_INGRESS_MAC_ACL_NO_NAT;
bf_rt_field_id_t smi_id::P_INGRESS_MAC_ACL_NO_NAT_DISABLE_NAT;

bf_rt_table_id_t smi_id::T_PRE_INGRESS_ACL;
bf_rt_field_id_t smi_id::F_PRE_INGRESS_ACL_LOCAL_MD_LKP_MAC_SRC_ADDR;
bf_rt_field_id_t smi_id::F_PRE_INGRESS_ACL_LOCAL_MD_LKP_MAC_DST_ADDR;
bf_rt_field_id_t smi_id::F_PRE_INGRESS_ACL_LOCAL_MD_LKP_MAC_TYPE;
bf_rt_field_id_t smi_id::F_PRE_INGRESS_ACL_LOCAL_MD_LKP_IP_SRC_ADDR;
bf_rt_field_id_t smi_id::F_PRE_INGRESS_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD3;
bf_rt_field_id_t smi_id::F_PRE_INGRESS_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD2;
bf_rt_field_id_t smi_id::F_PRE_INGRESS_ACL_LOCAL_MD_LKP_IP_DST_ADDR;
bf_rt_field_id_t smi_id::F_PRE_INGRESS_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD3;
bf_rt_field_id_t smi_id::F_PRE_INGRESS_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD2;
bf_rt_field_id_t smi_id::F_PRE_INGRESS_ACL_LOCAL_MD_LKP_IP_TOS;
bf_rt_field_id_t smi_id::F_PRE_INGRESS_ACL_LOCAL_MD_INGRESS_PORT;
bf_rt_field_id_t smi_id::F_PRE_INGRESS_ACL_MATCH_PRIORITY;
bf_rt_action_id_t smi_id::A_PRE_INGRESS_ACL_SET_VRF;
bf_rt_field_id_t smi_id::D_PRE_INGRESS_ACL_SET_VRF_VRF;
bf_rt_action_id_t smi_id::A_PRE_INGRESS_ACL_NO_ACTION;
bf_rt_action_id_t smi_id::A_PRE_INGRESS_ACL_DROP;
bf_rt_field_id_t smi_id::D_PRE_INGRESS_ACL_COUNTER_SPEC_BYTES;
bf_rt_field_id_t smi_id::D_PRE_INGRESS_ACL_COUNTER_SPEC_PKTS;
bf_rt_field_id_t smi_id::T_PRE_INGRESS_DEIVCE_TO_ACL;
bf_rt_field_id_t smi_id::A_PRE_INGRESS_SET_ACL_STATUS;
bf_rt_field_id_t smi_id::D_PRE_INGRESS_SET_ACL_STATUS_ENABLED;

bf_rt_table_id_t smi_id::T_INGRESS_ACL_ETYPE1;
bf_rt_field_id_t smi_id::F_INGRESS_ACL_ETYPE1_LOCAL_MD_LKP_MAC_TYPE;
bf_rt_action_id_t smi_id::A_INGRESS_ACL_ETYPE1_SET_ETYPE_LABEL;
bf_rt_field_id_t smi_id::D_INGRESS_ACL_ETYPE1_SET_ETYPE_LABEL;
bf_rt_table_id_t smi_id::T_INGRESS_ACL_ETYPE2;
bf_rt_field_id_t smi_id::F_INGRESS_ACL_ETYPE2_LOCAL_MD_LKP_MAC_TYPE;
bf_rt_action_id_t smi_id::A_INGRESS_ACL_ETYPE2_SET_ETYPE_LABEL;
bf_rt_field_id_t smi_id::D_INGRESS_ACL_ETYPE2_SET_ETYPE_LABEL;

bf_rt_field_id_t smi_id::T_INGRESS_ACL_QOS_MACADDR;
bf_rt_field_id_t smi_id::F_INGRESS_ACL_QOS_MACADDR_PORT_LAG_INDEX;
bf_rt_field_id_t smi_id::F_INGRESS_ACL_QOS_MACADDR_SMAC_ADDR;
bf_rt_field_id_t smi_id::F_INGRESS_ACL_QOS_MACADDR_DMAC_ADDR;
bf_rt_action_id_t smi_id::A_INGRESS_ACL_QOS_MACADDR_SET_MACADDR_LABEL;
bf_rt_field_id_t smi_id::D_INGRESS_ACL_QOS_MACADDR_SET_MACADDR_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_ACL_QOS_MACADDR_MATCH_PRIORITY;

bf_rt_field_id_t smi_id::T_INGRESS_ACL_PBR_MACADDR;
bf_rt_field_id_t smi_id::F_INGRESS_ACL_PBR_MACADDR_PORT_LAG_INDEX;
bf_rt_field_id_t smi_id::F_INGRESS_ACL_PBR_MACADDR_SMAC_ADDR;
bf_rt_field_id_t smi_id::F_INGRESS_ACL_PBR_MACADDR_DMAC_ADDR;
bf_rt_action_id_t smi_id::A_INGRESS_ACL_PBR_MACADDR_SET_MACADDR_LABEL;
bf_rt_field_id_t smi_id::D_INGRESS_ACL_PBR_MACADDR_SET_MACADDR_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_ACL_PBR_MACADDR_MATCH_PRIORITY;

bf_rt_field_id_t smi_id::T_INGRESS_ACL_MIRROR_MACADDR;
bf_rt_field_id_t smi_id::F_INGRESS_ACL_MIRROR_MACADDR_PORT_LAG_INDEX;
bf_rt_field_id_t smi_id::F_INGRESS_ACL_MIRROR_MACADDR_SMAC_ADDR;
bf_rt_field_id_t smi_id::F_INGRESS_ACL_MIRROR_MACADDR_DMAC_ADDR;
bf_rt_action_id_t smi_id::A_INGRESS_ACL_MIRROR_MACADDR_SET_MACADDR_LABEL;
bf_rt_field_id_t smi_id::D_INGRESS_ACL_MIRROR_MACADDR_SET_MACADDR_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_ACL_MIRROR_MACADDR_MATCH_PRIORITY;

bf_rt_table_id_t smi_id::T_INGRESS_IP_ACL;
bf_rt_field_id_t smi_id::F_INGRESS_IP_ACL_LOCAL_MD_ETYPE_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_IP_ACL_LOCAL_MD_PBR_MAC_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_IP_ACL_LOCAL_MD_LKP_MAC_TYPE;
bf_rt_field_id_t smi_id::F_INGRESS_IP_ACL_LOCAL_MD_INGRESS_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_SRC_ADDR;
bf_rt_field_id_t smi_id::F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD3;
bf_rt_field_id_t smi_id::F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD2;
bf_rt_field_id_t smi_id::F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_DST_ADDR;
bf_rt_field_id_t smi_id::F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD3;
bf_rt_field_id_t smi_id::F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD2;
bf_rt_field_id_t smi_id::F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_PROTO;
bf_rt_field_id_t smi_id::F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_TOS;
bf_rt_field_id_t smi_id::F_INGRESS_IP_ACL_LOCAL_MD_LKP_L4_SRC_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_IP_ACL_LOCAL_MD_LKP_L4_DST_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_TTL;
bf_rt_field_id_t smi_id::F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_FRAG;
bf_rt_field_id_t smi_id::F_INGRESS_IP_ACL_LOCAL_MD_LKP_TCP_FLAGS;
bf_rt_field_id_t smi_id::F_INGRESS_IP_ACL_LOCAL_MD_FIB_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_IP_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_IP_ACL_LOCAL_MD_INGRESS_PORT_LAG_INDEX;
bf_rt_field_id_t smi_id::F_INGRESS_IP_ACL_LOCAL_MD_BD_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_IP_ACL_LOCAL_MD_BD;
bf_rt_field_id_t smi_id::F_INGRESS_IP_ACL_LOCAL_MD_L4_SRC_PORT_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_IP_ACL_LOCAL_MD_L4_DST_PORT_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_IP_ACL_LOCAL_MD_LKP_MAC_DST_ADDR;
bf_rt_field_id_t smi_id::F_INGRESS_IP_ACL_LOCAL_MD_LKP_MAC_SRC_ADDR;
bf_rt_field_id_t smi_id::F_INGRESS_IP_ACL_LOCAL_MD_LKP_PCP;
bf_rt_field_id_t smi_id::F_INGRESS_IP_ACL_LOCAL_MD_LKP_DEI;
bf_rt_field_id_t smi_id::F_INGRESS_IP_ACL_HDR_VLAN_TAG0_VALID;
bf_rt_field_id_t smi_id::F_INGRESS_IP_ACL_MATCH_PRIORITY;
bf_rt_field_id_t smi_id::F_INGRESS_IP_ACL_LOCAL_MD_FLAGS_RMAC_HIT;
bf_rt_action_id_t smi_id::A_INGRESS_IP_ACL_NO_ACTION;
bf_rt_action_id_t smi_id::A_INGRESS_IP_ACL_DROP;
bf_rt_action_id_t smi_id::A_INGRESS_IP_ACL_PERMIT;
bf_rt_field_id_t smi_id::P_INGRESS_IP_ACL_PERMIT_USER_METADATA;
bf_rt_field_id_t smi_id::P_INGRESS_IP_ACL_PERMIT_METER_INDEX;
bf_rt_field_id_t smi_id::P_INGRESS_IP_ACL_PERMIT_TRAP_ID;
bf_rt_action_id_t smi_id::A_INGRESS_IP_ACL_REDIRECT_NEXTHOP;
bf_rt_field_id_t smi_id::P_INGRESS_IP_ACL_REDIRECT_NEXTHOP_INDEX;
bf_rt_field_id_t smi_id::P_INGRESS_IP_ACL_REDIRECT_NEXTHOP_USER_METADATA;
bf_rt_action_id_t smi_id::A_INGRESS_IP_ACL_REDIRECT_PORT;
bf_rt_field_id_t smi_id::P_INGRESS_IP_ACL_REDIRECT_PORT_LAG_INDEX;
bf_rt_field_id_t smi_id::P_INGRESS_IP_ACL_REDIRECT_PORT_USER_METADATA;
bf_rt_action_id_t smi_id::A_INGRESS_IP_ACL_MIRROR;
bf_rt_field_id_t smi_id::P_INGRESS_IP_ACL_MIRROR_METER_INDEX;
bf_rt_field_id_t smi_id::P_INGRESS_IP_ACL_MIRROR_SESSION_ID;
bf_rt_action_id_t smi_id::A_INGRESS_IP_ACL_SET_TC;
bf_rt_field_id_t smi_id::P_INGRESS_IP_ACL_SET_TC_TC;
bf_rt_action_id_t smi_id::A_INGRESS_IP_ACL_SET_COLOR;
bf_rt_field_id_t smi_id::P_INGRESS_IP_ACL_SET_COLOR_COLOR;
bf_rt_action_id_t smi_id::A_INGRESS_IP_ACL_TRAP;
bf_rt_field_id_t smi_id::P_INGRESS_IP_ACL_TRAP_TRAP_ID;
bf_rt_field_id_t smi_id::P_INGRESS_IP_ACL_TRAP_METER_INDEX;
bf_rt_action_id_t smi_id::A_INGRESS_IP_ACL_COPY;
bf_rt_field_id_t smi_id::P_INGRESS_IP_ACL_COPY_TRAP_ID;
bf_rt_field_id_t smi_id::P_INGRESS_IP_ACL_COPY_METER_INDEX;
bf_rt_field_id_t smi_id::D_INGRESS_IP_ACL_COUNTER_SPEC_BYTES;
bf_rt_field_id_t smi_id::D_INGRESS_IP_ACL_COUNTER_SPEC_PKTS;

bf_rt_table_id_t smi_id::T_INGRESS_IPV4_ACL;
bf_rt_field_id_t smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_MAC_TYPE;
bf_rt_field_id_t smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_INGRESS_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_SRC_ADDR;
bf_rt_field_id_t smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_DST_ADDR;
bf_rt_field_id_t smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_PROTO;
bf_rt_field_id_t smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_TOS;
bf_rt_field_id_t smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_L4_SRC_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_L4_DST_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_TTL;
bf_rt_field_id_t smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_FRAG;
bf_rt_field_id_t smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_TCP_FLAGS;
bf_rt_field_id_t smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_FIB_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_INGRESS_PORT_LAG_INDEX;
bf_rt_field_id_t smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_BD_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_BD;
bf_rt_field_id_t smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_L4_SRC_PORT_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_L4_DST_PORT_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_IN_PORTS_GROUP_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_IPV4_ACL_MATCH_PRIORITY;
bf_rt_field_id_t smi_id::F_INGRESS_IPV4_ACL_LOCAL_MD_FLAGS_RMAC_HIT;
bf_rt_action_id_t smi_id::A_INGRESS_IPV4_ACL_NO_ACTION;
bf_rt_action_id_t smi_id::A_INGRESS_IPV4_ACL_DROP;
bf_rt_action_id_t smi_id::A_INGRESS_IPV4_ACL_PERMIT;
bf_rt_field_id_t smi_id::P_INGRESS_IPV4_ACL_PERMIT_USER_METADATA;
bf_rt_field_id_t smi_id::P_INGRESS_IPV4_ACL_PERMIT_METER_INDEX;
bf_rt_field_id_t smi_id::P_INGRESS_IPV4_ACL_PERMIT_TRAP_ID;
bf_rt_action_id_t smi_id::A_INGRESS_IPV4_ACL_REDIRECT_NEXTHOP;
bf_rt_field_id_t smi_id::P_INGRESS_IPV4_ACL_REDIRECT_NEXTHOP_INDEX;
bf_rt_field_id_t smi_id::P_INGRESS_IPV4_ACL_REDIRECT_NEXTHOP_USER_METADATA;
bf_rt_action_id_t smi_id::A_INGRESS_IPV4_ACL_REDIRECT_PORT;
bf_rt_field_id_t smi_id::P_INGRESS_IPV4_ACL_REDIRECT_PORT_LAG_INDEX;
bf_rt_field_id_t smi_id::P_INGRESS_IPV4_ACL_REDIRECT_PORT_USER_METADATA;
bf_rt_action_id_t smi_id::A_INGRESS_IPV4_ACL_MIRROR;
bf_rt_field_id_t smi_id::P_INGRESS_IPV4_ACL_MIRROR_METER_INDEX;
bf_rt_field_id_t smi_id::P_INGRESS_IPV4_ACL_MIRROR_SESSION_ID;
bf_rt_action_id_t smi_id::A_INGRESS_IPV4_ACL_SET_TC;
bf_rt_field_id_t smi_id::P_INGRESS_IPV4_ACL_SET_TC_TC;
bf_rt_action_id_t smi_id::A_INGRESS_IPV4_ACL_SET_COLOR;
bf_rt_field_id_t smi_id::P_INGRESS_IPV4_ACL_SET_COLOR_COLOR;
bf_rt_action_id_t smi_id::A_INGRESS_IPV4_ACL_SET_DTEL_REPORT_TYPE;
bf_rt_field_id_t smi_id::P_INGRESS_IPV4_ACL_DTEL_REPORT_TYPE;
bf_rt_action_id_t smi_id::A_INGRESS_IPV4_ACL_TRAP;
bf_rt_field_id_t smi_id::P_INGRESS_IPV4_ACL_TRAP_TRAP_ID;
bf_rt_field_id_t smi_id::P_INGRESS_IPV4_ACL_TRAP_METER_INDEX;
bf_rt_action_id_t smi_id::A_INGRESS_IPV4_ACL_COPY;
bf_rt_field_id_t smi_id::P_INGRESS_IPV4_ACL_COPY_TRAP_ID;
bf_rt_field_id_t smi_id::P_INGRESS_IPV4_ACL_COPY_METER_INDEX;
bf_rt_field_id_t smi_id::D_INGRESS_IPV4_ACL_COUNTER_SPEC_BYTES;
bf_rt_field_id_t smi_id::D_INGRESS_IPV4_ACL_COUNTER_SPEC_PKTS;
bf_rt_field_id_t smi_id::F_IPV4_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL_7_0;
bf_rt_field_id_t smi_id::F_IPV4_ACL_LOCAL_MD_INGRESS_PORT_LAG_INDEX;
bf_rt_action_id_t smi_id::A_INGRESS_IPV4_ACL_TRANSIT;
bf_rt_action_id_t smi_id::A_INGRESS_IPV4_ACL_DENY;
bf_rt_action_id_t smi_id::A_INGRESS_IPV4_ACL_NO_NAT;
bf_rt_field_id_t smi_id::P_INGRESS_IPV4_ACL_NO_NAT_DISABLE_NAT;

bf_rt_table_id_t smi_id::T_INGRESS_INNER_DTEL_IPV4_ACL;
bf_rt_field_id_t smi_id::F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_IPV4_SRC_ADDR;
bf_rt_field_id_t smi_id::F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_IPV4_DST_ADDR;
bf_rt_field_id_t smi_id::F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_IPV4_PROTOCOL;
bf_rt_field_id_t smi_id::F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_IPV4_DIFFSERV;
bf_rt_field_id_t smi_id::F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_TCP_SRC_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_TCP_DST_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_UDP_SRC_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_UDP_DST_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_IPV4_TTL;
bf_rt_field_id_t smi_id::F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_TCP_FLAGS;
bf_rt_field_id_t smi_id::F_INGRESS_INNER_DTEL_IPV4_ACL_IG_MD_PORT_LAG_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_INNER_DTEL_IPV4_ACL_LOCAL_MD_TUNNEL_VNI;
bf_rt_field_id_t smi_id::A_INGRESS_INNER_DTEL_IPV4_ACL_SET_DTEL_REPORT_TYPE;
bf_rt_field_id_t smi_id::P_INGRESS_INNER_DTEL_IPV4_ACL_DTEL_REPORT_TYPE;
bf_rt_field_id_t smi_id::D_INGRESS_INNER_DTEL_IPV4_ACL_COUNTER_SPEC_BYTES;
bf_rt_field_id_t smi_id::D_INGRESS_INNER_DTEL_IPV4_ACL_COUNTER_SPEC_PKTS;

bf_rt_table_id_t smi_id::T_INGRESS_IPV6_ACL;
bf_rt_field_id_t smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_MAC_TYPE;
bf_rt_field_id_t smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_INGRESS_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_SRC_ADDR;
bf_rt_field_id_t smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD3;
bf_rt_field_id_t smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD2;
bf_rt_field_id_t smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_DST_ADDR;
bf_rt_field_id_t smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD3;
bf_rt_field_id_t smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD2;
bf_rt_field_id_t smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_PROTO;
bf_rt_field_id_t smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_TOS;
bf_rt_field_id_t smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_L4_SRC_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_L4_DST_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_TTL;
bf_rt_field_id_t smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_FRAG;
bf_rt_field_id_t smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_TCP_FLAGS;
bf_rt_field_id_t smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_FIB_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_INGRESS_PORT_LAG_INDEX;
bf_rt_field_id_t smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_BD_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_BD;
bf_rt_field_id_t smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_L4_SRC_PORT_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_L4_DST_PORT_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_IN_PORTS_GROUP_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_IPV6_ACL_MATCH_PRIORITY;
bf_rt_field_id_t smi_id::F_INGRESS_IPV6_ACL_LOCAL_MD_FLAGS_RMAC_HIT;
bf_rt_action_id_t smi_id::A_INGRESS_IPV6_ACL_NO_ACTION;
bf_rt_action_id_t smi_id::A_INGRESS_IPV6_ACL_DROP;
bf_rt_action_id_t smi_id::A_INGRESS_IPV6_ACL_PERMIT;
bf_rt_field_id_t smi_id::P_INGRESS_IPV6_ACL_PERMIT_USER_METADATA;
bf_rt_field_id_t smi_id::P_INGRESS_IPV6_ACL_PERMIT_METER_INDEX;
bf_rt_field_id_t smi_id::P_INGRESS_IPV6_ACL_PERMIT_TRAP_ID;
bf_rt_action_id_t smi_id::A_INGRESS_IPV6_ACL_REDIRECT_NEXTHOP;
bf_rt_field_id_t smi_id::P_INGRESS_IPV6_ACL_REDIRECT_NEXTHOP_INDEX;
bf_rt_field_id_t smi_id::P_INGRESS_IPV6_ACL_REDIRECT_NEXTHOP_USER_METADATA;
bf_rt_action_id_t smi_id::A_INGRESS_IPV6_ACL_REDIRECT_PORT;
bf_rt_field_id_t smi_id::P_INGRESS_IPV6_ACL_REDIRECT_PORT_LAG_INDEX;
bf_rt_field_id_t smi_id::P_INGRESS_IPV6_ACL_REDIRECT_PORT_USER_METADATA;
bf_rt_action_id_t smi_id::A_INGRESS_IPV6_ACL_MIRROR;
bf_rt_field_id_t smi_id::P_INGRESS_IPV6_ACL_MIRROR_METER_INDEX;
bf_rt_field_id_t smi_id::P_INGRESS_IPV6_ACL_MIRROR_SESSION_ID;
bf_rt_action_id_t smi_id::A_INGRESS_IPV6_ACL_SET_TC;
bf_rt_field_id_t smi_id::P_INGRESS_IPV6_ACL_SET_TC_TC;
bf_rt_action_id_t smi_id::A_INGRESS_IPV6_ACL_SET_COLOR;
bf_rt_field_id_t smi_id::P_INGRESS_IPV6_ACL_SET_COLOR_COLOR;
bf_rt_action_id_t smi_id::A_INGRESS_IPV6_ACL_SET_DTEL_REPORT_TYPE;
bf_rt_field_id_t smi_id::P_INGRESS_IPV6_ACL_DTEL_REPORT_TYPE;
bf_rt_action_id_t smi_id::A_INGRESS_IPV6_ACL_TRAP;
bf_rt_field_id_t smi_id::P_INGRESS_IPV6_ACL_TRAP_TRAP_ID;
bf_rt_field_id_t smi_id::P_INGRESS_IPV6_ACL_TRAP_METER_INDEX;
bf_rt_action_id_t smi_id::A_INGRESS_IPV6_ACL_COPY;
bf_rt_field_id_t smi_id::P_INGRESS_IPV6_ACL_COPY_TRAP_ID;
bf_rt_field_id_t smi_id::P_INGRESS_IPV6_ACL_COPY_METER_INDEX;
bf_rt_field_id_t smi_id::D_INGRESS_IPV6_ACL_COUNTER_SPEC_BYTES;
bf_rt_field_id_t smi_id::D_INGRESS_IPV6_ACL_COUNTER_SPEC_PKTS;
bf_rt_action_id_t smi_id::F_IPV6_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL_15_8;
bf_rt_field_id_t smi_id::F_IPV6_ACL_LOCAL_MD_INGRESS_PORT_LAG_INDEX;
bf_rt_action_id_t smi_id::A_INGRESS_IPV6_ACL_TRANSIT;
bf_rt_action_id_t smi_id::A_INGRESS_IPV6_ACL_DENY;
bf_rt_action_id_t smi_id::A_INGRESS_IPV6_ACL_NO_NAT;
bf_rt_field_id_t smi_id::P_INGRESS_IPV6_ACL_NO_NAT_DISABLE_NAT;

bf_rt_table_id_t smi_id::T_INGRESS_INNER_DTEL_IPV6_ACL;
bf_rt_field_id_t smi_id::F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_IPV6_SRC_ADDR;
bf_rt_field_id_t smi_id::F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_IPV6_DST_ADDR;
bf_rt_field_id_t smi_id::F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_IPV6_NEXT_HDR;
bf_rt_field_id_t
    smi_id::F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_IPV6_TRAFFIC_CLASS;
bf_rt_field_id_t smi_id::F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_TCP_SRC_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_TCP_DST_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_UDP_SRC_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_UDP_DST_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_IPV6_HOP_LIMIT;
bf_rt_field_id_t smi_id::F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_TCP_FLAGS;
bf_rt_field_id_t smi_id::F_INGRESS_INNER_DTEL_IPV6_ACL_IG_MD_PORT_LAG_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_INNER_DTEL_IPV6_ACL_LOCAL_MD_TUNNEL_VNI;
bf_rt_field_id_t smi_id::A_INGRESS_INNER_DTEL_IPV6_ACL_SET_DTEL_REPORT_TYPE;
bf_rt_field_id_t smi_id::P_INGRESS_INNER_DTEL_IPV6_ACL_DTEL_REPORT_TYPE;
bf_rt_field_id_t smi_id::D_INGRESS_INNER_DTEL_IPV6_ACL_COUNTER_SPEC_BYTES;
bf_rt_field_id_t smi_id::D_INGRESS_INNER_DTEL_IPV6_ACL_COUNTER_SPEC_PKTS;

bf_rt_table_id_t smi_id::T_INGRESS_IP_MIRROR_ACL;
bf_rt_field_id_t smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_ETYPE_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_MIRROR_MAC_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_MAC_TYPE;
bf_rt_field_id_t smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_SRC_ADDR;
bf_rt_field_id_t smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_DST_ADDR;
bf_rt_field_id_t smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_PROTO;
bf_rt_field_id_t smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_TOS;
bf_rt_field_id_t smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_L4_SRC_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_L4_DST_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_TTL;
bf_rt_field_id_t smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_FRAG;
bf_rt_field_id_t smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_TCP_FLAGS;
bf_rt_field_id_t smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_IN_PORTS_GROUP_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_FLAGS_RMAC_HIT;
bf_rt_field_id_t
    smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_BD_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_BD;
bf_rt_field_id_t smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_L4_SRC_PORT_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_L4_DST_PORT_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_PCP;
bf_rt_field_id_t smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_DEI;
bf_rt_field_id_t smi_id::F_INGRESS_IP_MIRROR_ACL_HDR_VLAN_TAG0_VALID;
bf_rt_field_id_t smi_id::F_INGRESS_IP_MIRROR_ACL_MATCH_PRIORITY;
bf_rt_action_id_t smi_id::A_INGRESS_IP_MIRROR_ACL_NO_ACTION;
bf_rt_field_id_t smi_id::P_INGRESS_IP_MIRROR_ACL_NO_ACTION_METER_INDEX;
bf_rt_action_id_t smi_id::A_INGRESS_IP_MIRROR_ACL_DROP;
bf_rt_action_id_t smi_id::A_INGRESS_IP_MIRROR_ACL_PERMIT;
bf_rt_action_id_t smi_id::A_INGRESS_IP_MIRROR_ACL_REDIRECT_NEXTHOP;
bf_rt_field_id_t smi_id::P_INGRESS_IP_MIRROR_ACL_REDIRECT_NEXTHOP_INDEX;
bf_rt_action_id_t smi_id::A_INGRESS_IP_MIRROR_ACL_MIRROR;
bf_rt_field_id_t smi_id::P_INGRESS_IP_MIRROR_ACL_MIRROR_METER_INDEX;
bf_rt_field_id_t smi_id::P_INGRESS_IP_MIRROR_ACL_MIRROR_SESSION_ID;
bf_rt_action_id_t smi_id::A_INGRESS_IP_MIRROR_ACL_MIRROR_OUT;
bf_rt_field_id_t smi_id::P_INGRESS_IP_MIRROR_ACL_MIRROR_OUT_METER_INDEX;
bf_rt_field_id_t smi_id::P_INGRESS_IP_MIRROR_ACL_MIRROR_OUT_SESSION_ID;
bf_rt_action_id_t smi_id::A_INGRESS_IP_MIRROR_ACL_SET_TC;
bf_rt_field_id_t smi_id::P_INGRESS_IP_MIRROR_ACL_SET_TC_TC;
bf_rt_action_id_t smi_id::A_INGRESS_IP_MIRROR_ACL_SET_COLOR;
bf_rt_field_id_t smi_id::P_INGRESS_IP_MIRROR_ACL_SET_COLOR_COLOR;
bf_rt_action_id_t smi_id::A_INGRESS_IP_MIRROR_ACL_TRAP;
bf_rt_field_id_t smi_id::P_INGRESS_IP_MIRROR_ACL_TRAP_TRAP_ID;
bf_rt_field_id_t smi_id::P_INGRESS_IP_MIRROR_ACL_TRAP_METER_INDEX;
bf_rt_action_id_t smi_id::A_INGRESS_IP_MIRROR_ACL_COPY;
bf_rt_field_id_t smi_id::P_INGRESS_IP_MIRROR_ACL_COPY_TRAP_ID;
bf_rt_field_id_t smi_id::P_INGRESS_IP_MIRROR_ACL_COPY_METER_INDEX;
bf_rt_action_id_t smi_id::A_INGRESS_IP_MIRROR_ACL_SET_DTEL_REPORT_TYPE;
bf_rt_field_id_t smi_id::P_INGRESS_IP_MIRROR_ACL_DTEL_REPORT_TYPE;
bf_rt_field_id_t smi_id::D_INGRESS_IP_MIRROR_ACL_COUNTER_SPEC_BYTES;
bf_rt_field_id_t smi_id::D_INGRESS_IP_MIRROR_ACL_COUNTER_SPEC_PKTS;
bf_rt_action_id_t
    smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL_23_16;
bf_rt_field_id_t
    smi_id::F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_INGRESS_PORT_LAG_INDEX;
bf_rt_field_id_t smi_id::D_INGRESS_IP_MIRROR_ACL_METER_SPEC_CIR_KBPS;
bf_rt_field_id_t smi_id::D_INGRESS_IP_MIRROR_ACL_METER_SPEC_PIR_KBPS;
bf_rt_field_id_t smi_id::D_INGRESS_IP_MIRROR_ACL_METER_SPEC_CBS_KBITS;
bf_rt_field_id_t smi_id::D_INGRESS_IP_MIRROR_ACL_METER_SPEC_PBS_KBITS;

bf_rt_table_id_t smi_id::T_INGRESS_IP_MIRROR_ACL_METER_ACTION;
bf_rt_field_id_t smi_id::F_INGRESS_IP_MIRROR_ACL_METER_ACTION_COLOR;
bf_rt_field_id_t smi_id::F_INGRESS_IP_MIRROR_ACL_METER_ACTION_INDEX;
bf_rt_action_id_t smi_id::A_INGRESS_IP_MIRROR_ACL_METER_ACTION_COUNT;
bf_rt_action_id_t smi_id::A_INGRESS_IP_MIRROR_ACL_METER_ACTION_DROP_AND_COUNT;
bf_rt_field_id_t
    smi_id::D_INGRESS_IP_MIRROR_ACL_METER_ACTION_COUNTER_SPEC_BYTES;

bf_rt_table_id_t smi_id::T_INGRESS_IP_QOS_ACL;
bf_rt_field_id_t smi_id::F_INGRESS_IP_QOS_ACL_LOCAL_MD_ETYPE_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_IP_QOS_ACL_LOCAL_MD_QOS_MAC_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_MAC_TYPE;
bf_rt_field_id_t smi_id::F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_IP_SRC_ADDR;
bf_rt_field_id_t smi_id::F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_IP_DST_ADDR;
bf_rt_field_id_t smi_id::F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_IP_PROTO;
bf_rt_field_id_t smi_id::F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_IP_TOS;
bf_rt_field_id_t smi_id::F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_L4_SRC_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_L4_DST_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_TCP_FLAGS;
bf_rt_field_id_t smi_id::F_INGRESS_IP_QOS_ACL_LOCAL_MD_BD;
bf_rt_field_id_t smi_id::F_INGRESS_IP_QOS_ACL_LOCAL_MD_L4_SRC_PORT_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_IP_QOS_ACL_LOCAL_MD_L4_DST_PORT_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_IP_QOS_ACL_LOCAL_MD_INGRESS_PORT_LAG_INDEX;
bf_rt_field_id_t smi_id::F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_PCP;
bf_rt_field_id_t smi_id::F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_DEI;
bf_rt_field_id_t smi_id::F_INGRESS_IP_QOS_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_IP_QOS_ACL_HDR_VLAN_TAG0_VALID;
bf_rt_field_id_t smi_id::F_INGRESS_IP_QOS_ACL_MATCH_PRIORITY;
bf_rt_field_id_t smi_id::F_INGRESS_IP_QOS_ACL_LOCAL_MD_FLAGS_RMAC_HIT;
bf_rt_action_id_t smi_id::A_INGRESS_IP_QOS_ACL_SET_TC;
bf_rt_field_id_t smi_id::P_INGRESS_IP_QOS_ACL_SET_TC_TC;
bf_rt_field_id_t smi_id::P_INGRESS_IP_QOS_ACL_SET_TC_METER_INDEX;
bf_rt_action_id_t smi_id::A_INGRESS_IP_QOS_ACL_SET_COLOR;
bf_rt_field_id_t smi_id::P_INGRESS_IP_QOS_ACL_SET_COLOR_COLOR;
bf_rt_field_id_t smi_id::P_INGRESS_IP_QOS_ACL_SET_COLOR_METER_INDEX;
bf_rt_action_id_t smi_id::A_INGRESS_IP_QOS_ACL_SET_METER;
bf_rt_field_id_t smi_id::P_INGRESS_IP_QOS_ACL_SET_METER_INDEX;
bf_rt_action_id_t smi_id::A_INGRESS_IP_QOS_ACL_SET_QOS_PARAMS;
bf_rt_field_id_t smi_id::P_INGRESS_IP_QOS_ACL_SET_QOS_PARAMS_TC;
bf_rt_field_id_t smi_id::P_INGRESS_IP_QOS_ACL_SET_QOS_PARAMS_COLOR;
bf_rt_field_id_t smi_id::P_INGRESS_IP_QOS_ACL_SET_QOS_PARAMS_METER_INDEX;
bf_rt_action_id_t smi_id::A_INGRESS_IP_QOS_ACL_NO_ACTION;
bf_rt_field_id_t smi_id::P_INGRESS_IP_QOS_ACL_NO_ACTION_METER_INDEX;
bf_rt_field_id_t smi_id::D_INGRESS_IP_QOS_ACL_COUNTER_SPEC_BYTES;
bf_rt_field_id_t smi_id::D_INGRESS_IP_QOS_ACL_COUNTER_SPEC_PKTS;
bf_rt_field_id_t smi_id::D_INGRESS_IP_QOS_ACL_METER_SPEC_CIR_KBPS;
bf_rt_field_id_t smi_id::D_INGRESS_IP_QOS_ACL_METER_SPEC_PIR_KBPS;
bf_rt_field_id_t smi_id::D_INGRESS_IP_QOS_ACL_METER_SPEC_CBS_KBITS;
bf_rt_field_id_t smi_id::D_INGRESS_IP_QOS_ACL_METER_SPEC_PBS_KBITS;

bf_rt_table_id_t smi_id::T_INGRESS_IP_QOS_ACL_METER_ACTION;
bf_rt_field_id_t smi_id::F_INGRESS_IP_QOS_ACL_METER_ACTION_COLOR;
bf_rt_field_id_t smi_id::F_INGRESS_IP_QOS_ACL_METER_ACTION_INDEX;
bf_rt_action_id_t smi_id::A_INGRESS_IP_QOS_ACL_METER_ACTION_COUNT;
bf_rt_action_id_t smi_id::A_INGRESS_IP_QOS_ACL_METER_ACTION_DROP_AND_COUNT;
bf_rt_field_id_t smi_id::D_INGRESS_IP_QOS_ACL_METER_ACTION_COUNTER_SPEC_BYTES;

bf_rt_table_id_t smi_id::T_INGRESS_IP_DTEL_ACL;
bf_rt_field_id_t smi_id::F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_MAC_TYPE;
bf_rt_field_id_t smi_id::F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_SRC_ADDR;
bf_rt_field_id_t smi_id::F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD3;
bf_rt_field_id_t smi_id::F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD2;
bf_rt_field_id_t smi_id::F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_DST_ADDR;
bf_rt_field_id_t smi_id::F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD3;
bf_rt_field_id_t smi_id::F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD2;
bf_rt_field_id_t smi_id::F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_PROTO;
bf_rt_field_id_t smi_id::F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_TOS;
bf_rt_field_id_t smi_id::F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_L4_SRC_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_L4_DST_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_TTL;
bf_rt_field_id_t smi_id::F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_FRAG;
bf_rt_field_id_t smi_id::F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_TCP_FLAGS;
bf_rt_field_id_t smi_id::F_INGRESS_IP_DTEL_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_IP_DTEL_ACL_LOCAL_MD_BD_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_IP_DTEL_ACL_LOCAL_MD_L4_SRC_PORT_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_IP_DTEL_ACL_LOCAL_MD_L4_DST_PORT_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_IP_DTEL_ACL_MATCH_PRIORITY;
bf_rt_action_id_t smi_id::A_INGRESS_IP_DTEL_ACL_NO_ACTION;
bf_rt_action_id_t smi_id::A_INGRESS_IP_DTEL_ACL_DROP;
bf_rt_action_id_t smi_id::A_INGRESS_IP_DTEL_ACL_PERMIT;
bf_rt_action_id_t smi_id::A_INGRESS_IP_DTEL_ACL_REDIRECT_NEXTHOP;
bf_rt_field_id_t smi_id::P_INGRESS_IP_DTEL_ACL_REDIRECT_NEXTHOP_INDEX;
bf_rt_action_id_t smi_id::A_INGRESS_IP_DTEL_ACL_MIRROR;
bf_rt_field_id_t smi_id::P_INGRESS_IP_DTEL_ACL_MIRROR_METER_INDEX;
bf_rt_field_id_t smi_id::P_INGRESS_IP_DTEL_ACL_MIRROR_SESSION_ID;
bf_rt_action_id_t smi_id::A_INGRESS_IP_DTEL_ACL_SET_TC;
bf_rt_field_id_t smi_id::P_INGRESS_IP_DTEL_ACL_SET_TC_TC;
bf_rt_action_id_t smi_id::A_INGRESS_IP_DTEL_ACL_SET_COLOR;
bf_rt_field_id_t smi_id::P_INGRESS_IP_DTEL_ACL_SET_COLOR_COLOR;
bf_rt_action_id_t smi_id::A_INGRESS_IP_DTEL_ACL_SET_DTEL_REPORT_TYPE;
bf_rt_field_id_t smi_id::P_INGRESS_IP_DTEL_ACL_DTEL_REPORT_TYPE;
bf_rt_action_id_t smi_id::A_INGRESS_IP_DTEL_ACL_IFA_CLONE_SAMPLE;
bf_rt_field_id_t smi_id::P_INGRESS_IP_DTEL_ACL_IFA_CLONE_SESSION_ID;
bf_rt_action_id_t smi_id::A_INGRESS_IP_DTEL_ACL_IFA_CLONE_AND_SET_REPORT_TYPE;
bf_rt_field_id_t smi_id::P_INGRESS_IP_DTEL_ACL_IFA_CLONE_SESSION_ID_WITH_TYPE;
bf_rt_field_id_t smi_id::P_INGRESS_IP_DTEL_ACL_DTEL_REPORT_TYPE_WITH_CLONE;
bf_rt_field_id_t smi_id::D_INGRESS_IP_DTEL_ACL_COUNTER_SPEC_BYTES;
bf_rt_field_id_t smi_id::D_INGRESS_IP_DTEL_ACL_COUNTER_SPEC_PKTS;

bf_rt_table_id_t smi_id::T_INGRESS_TOS_MIRROR_ACL;
bf_rt_field_id_t smi_id::F_INGRESS_TOS_MIRROR_ACL_LOCAL_MD_LKP_IP_TOS;
bf_rt_field_id_t
    smi_id::F_INGRESS_TOS_MIRROR_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL;
bf_rt_field_id_t smi_id::F_INGRESS_TOS_MIRROR_ACL_MATCH_PRIORITY;
bf_rt_action_id_t smi_id::A_INGRESS_TOS_MIRROR_ACL_MIRROR;
bf_rt_field_id_t smi_id::P_INGRESS_TOS_MIRROR_ACL_MIRROR_METER_INDEX;
bf_rt_field_id_t smi_id::P_INGRESS_TOS_MIRROR_ACL_MIRROR_SESSION_ID;
bf_rt_field_id_t smi_id::D_INGRESS_TOS_MIRROR_ACL_COUNTER_SPEC_BYTES;
bf_rt_field_id_t smi_id::D_INGRESS_TOS_MIRROR_ACL_COUNTER_SPEC_PKTS;
bf_rt_action_id_t smi_id::A_INGRESS_TOS_MIRROR_ACL_MIRROR_OUT;
bf_rt_field_id_t smi_id::P_INGRESS_TOS_MIRROR_ACL_MIRROR_OUT_METER_INDEX;
bf_rt_field_id_t smi_id::P_INGRESS_TOS_MIRROR_ACL_MIRROR_OUT_SESSION_ID;

bf_rt_table_id_t smi_id::T_INGRESS_L4_SRC_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_L4_SRC_PORT_LKP_L4_SRC_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_L4_SRC_PORT_PRIORITY;
bf_rt_action_id_t smi_id::A_INGRESS_SET_SRC_PORT_LABEL;
bf_rt_field_id_t smi_id::P_INGRESS_SET_SRC_PORT_LABEL_LABEL;

bf_rt_table_id_t smi_id::T_INGRESS_L4_DST_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_L4_DST_PORT_LKP_L4_DST_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_L4_DST_PORT_PRIORITY;
bf_rt_action_id_t smi_id::A_INGRESS_SET_DST_PORT_LABEL;
bf_rt_field_id_t smi_id::P_INGRESS_SET_DST_PORT_LABEL_LABEL;

bf_rt_table_id_t smi_id::T_INGRESS_QOS_ACL_L4_SRC_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_QOS_ACL_L4_SRC_PORT_LKP_L4_SRC_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_QOS_ACL_L4_SRC_PORT_PRIORITY;
bf_rt_action_id_t smi_id::A_INGRESS_QOS_ACL_SET_SRC_PORT_LABEL;
bf_rt_field_id_t smi_id::P_INGRESS_QOS_ACL_SET_SRC_PORT_LABEL_LABEL;

bf_rt_table_id_t smi_id::T_INGRESS_QOS_ACL_L4_DST_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_QOS_ACL_L4_DST_PORT_LKP_L4_DST_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_QOS_ACL_L4_DST_PORT_PRIORITY;
bf_rt_action_id_t smi_id::A_INGRESS_QOS_ACL_SET_DST_PORT_LABEL;
bf_rt_field_id_t smi_id::P_INGRESS_QOS_ACL_SET_DST_PORT_LABEL_LABEL;

bf_rt_table_id_t smi_id::T_INGRESS_IP_ACL_L4_SRC_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_IP_ACL_L4_SRC_PORT_LKP_L4_SRC_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_IP_ACL_L4_SRC_PORT_PRIORITY;
bf_rt_action_id_t smi_id::A_INGRESS_IP_ACL_SET_SRC_PORT_LABEL;
bf_rt_field_id_t smi_id::P_INGRESS_IP_ACL_SET_SRC_PORT_LABEL_LABEL;

bf_rt_table_id_t smi_id::T_INGRESS_IP_ACL_L4_DST_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_IP_ACL_L4_DST_PORT_LKP_L4_DST_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_IP_ACL_L4_DST_PORT_PRIORITY;
bf_rt_action_id_t smi_id::A_INGRESS_IP_ACL_SET_DST_PORT_LABEL;
bf_rt_field_id_t smi_id::P_INGRESS_IP_ACL_SET_DST_PORT_LABEL_LABEL;

bf_rt_table_id_t smi_id::T_INGRESS_DROP_STATS;
bf_rt_field_id_t smi_id::F_INGRESS_DROP_STATS_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_DROP_STATS_DROP_REASON;
bf_rt_action_id_t smi_id::A_INGRESS_DROP_STATS_COUNT;
bf_rt_field_id_t smi_id::D_INGRESS_DROP_STATS_PKTS;

bf_rt_table_id_t smi_id::T_INGRESS_SYSTEM_ACL;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_BD;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_BD_LABEL;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_INGRESS_PORT_LAG_INDEX;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LKP_PKT_TYPE;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LKP_MAC_TYPE;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LKP_MAC_DST_ADDR;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LKP_IP_DST_ADDR;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LKP_IP_TYPE;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LKP_IP_TTL;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LKP_IP_PROTO;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LKP_L4_SRC_PORT;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LKP_L4_DST_PORT;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LKP_ARP_OPCODE;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_ACL_DENY;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_RACL_DENY;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_ACL_COPY_CANCEL;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_RMAC_HIT;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_DMAC_MISS;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_MYIP;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_GLEAN;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_ROUTED;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_ARP_SUPPRESS;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_FIB_LPM_MISS;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_FIB_DROP;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_SAMPLE_PACKET;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_STORM_CONTROL_COLOR;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_METER_PACKET_ACTION;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_METER_DROP;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_LINK_LOCAL;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_FLAGS_PFC_WD_DROP;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_CHECKS_SAME_BD;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_CHECKS_SAME_IF;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_CHECKS_MRPF;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_STP_STATE;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV4_MD_UNICAST_ENABLE;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV4_MD_MULTICAST_ENABLE;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV4_MD_MULTICAST_SNOOPING;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV6_MD_UNICAST_ENABLE;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV6_MD_MULTICAST_ENABLE;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_IPV6_MD_MULTICAST_SNOOPING;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_MPLS_ENABLE;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_DROP_REASON;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_L2_DROP_REASON;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_PRIORITY;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_MPLS_TRAP;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_HOSTIF_TRAP_ID;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_HDR_MPLS_0_TTL;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_HDR_MPLS_0_VALID;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_MPLS_ROUTER_ALERT;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_SRV6_TRAP;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_BFD_TO_CPU;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_VRF_TTL_VIOLATION;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_VRF_TTL_VIOLATION_VALID;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_VRF_IP_OPTIONS_VIOLATION;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_VRF_UNKNOWN_L3_MULTICAST_TRAP;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_HDR_IP_OPTIONS_VALID;
bf_rt_action_id_t smi_id::A_SYSTEM_ACL_PERMIT;
bf_rt_action_id_t smi_id::A_SYSTEM_ACL_DROP;
bf_rt_field_id_t smi_id::P_SYSTEM_ACL_DROP_DROP_REASON;
bf_rt_field_id_t smi_id::P_SYSTEM_ACL_DROP_DISABLE_LEARNING;
bf_rt_action_id_t smi_id::A_SYSTEM_ACL_DENY;
bf_rt_field_id_t smi_id::P_SYSTEM_ACL_DENY_DROP_REASON;
bf_rt_field_id_t smi_id::P_SYSTEM_ACL_DENY_DISABLE_LEARNING;
bf_rt_action_id_t smi_id::A_SYSTEM_ACL_COPY_TO_CPU_CANCEL;
bf_rt_action_id_t smi_id::A_SYSTEM_ACL_COPY_TO_CPU;
bf_rt_field_id_t smi_id::P_SYSTEM_ACL_COPY_TO_CPU_REASON_CODE;
bf_rt_field_id_t smi_id::P_SYSTEM_ACL_COPY_TO_CPU_QID;
bf_rt_field_id_t smi_id::P_SYSTEM_ACL_COPY_TO_CPU_METER_ID;
bf_rt_field_id_t smi_id::P_SYSTEM_ACL_COPY_TO_CPU_DISABLE_LEARNING;
bf_rt_field_id_t smi_id::P_SYSTEM_ACL_COPY_TO_CPU_OVERWRITE_QID;
bf_rt_action_id_t smi_id::A_SYSTEM_ACL_REDIRECT_TO_CPU;
bf_rt_field_id_t smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_REASON_CODE;
bf_rt_field_id_t smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_QID;
bf_rt_field_id_t smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_METER_ID;
bf_rt_field_id_t smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_DISABLE_LEARNING;
bf_rt_field_id_t smi_id::P_SYSTEM_ACL_REDIRECT_TO_CPU_OVERWRITE_QID;
bf_rt_action_id_t smi_id::A_SYSTEM_ACL_COPY_SFLOW_TO_CPU;
bf_rt_field_id_t smi_id::P_SYSTEM_ACL_COPY_SFLOW_TO_CPU_REASON_CODE;
bf_rt_field_id_t smi_id::P_SYSTEM_ACL_COPY_SFLOW_TO_CPU_QID;
bf_rt_field_id_t smi_id::P_SYSTEM_ACL_COPY_SFLOW_TO_CPU_METER_ID;
bf_rt_field_id_t smi_id::P_SYSTEM_ACL_COPY_SFLOW_TO_CPU_DISABLE_LEARNING;
bf_rt_field_id_t smi_id::P_SYSTEM_ACL_COPY_SFLOW_TO_CPU_OVERWRITE_QID;
bf_rt_action_id_t smi_id::A_SYSTEM_ACL_REDIRECT_SFLOW_TO_CPU;
bf_rt_field_id_t smi_id::P_SYSTEM_ACL_REDIRECT_SFLOW_TO_CPU_REASON_CODE;
bf_rt_field_id_t smi_id::P_SYSTEM_ACL_REDIRECT_SFLOW_TO_CPU_QID;
bf_rt_field_id_t smi_id::P_SYSTEM_ACL_REDIRECT_SFLOW_TO_CPU_METER_ID;
bf_rt_field_id_t smi_id::P_SYSTEM_ACL_REDIRECT_SFLOW_TO_CPU_DISABLE_LEARNING;
bf_rt_field_id_t smi_id::P_SYSTEM_ACL_REDIRECT_SFLOW_TO_CPU_OVERWRITE_QID;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_NAT_HIT;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_NAT_SAME_ZONE;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_TUNNEL_TERMINATE;
bf_rt_field_id_t smi_id::F_SYSTEM_ACL_LOCAL_MD_MULTICAST_HIT;
bf_rt_action_id_t smi_id::A_SYSTEM_ACL_REDIRECT_BFD_TO_CPU;
bf_rt_field_id_t smi_id::P_SYSTEM_ACL_REDIRECT_BFD_TO_CPU_REASON_CODE;
bf_rt_field_id_t smi_id::P_SYSTEM_ACL_REDIRECT_BFD_TO_CPU_QID;
bf_rt_field_id_t smi_id::P_SYSTEM_ACL_REDIRECT_BFD_TO_CPU_METER_ID;
bf_rt_field_id_t smi_id::P_SYSTEM_ACL_REDIRECT_BFD_TO_CPU_DISABLE_LEARNING;

bf_rt_table_id_t smi_id::T_COPP;
bf_rt_field_id_t smi_id::F_COPP_PACKET_COLOR;
bf_rt_field_id_t smi_id::F_COPP_COPP_METER_ID;
bf_rt_field_id_t smi_id::F_COPP_PRIORITY;
bf_rt_action_id_t smi_id::A_COPP_DROP;
bf_rt_action_id_t smi_id::A_COPP_PERMIT;
bf_rt_field_id_t smi_id::D_COPP_STATS_COUNTER_SPEC_PKTS;

bf_rt_table_id_t smi_id::T_COPP_METER;
bf_rt_field_id_t smi_id::F_COPP_METER_METER_INDEX;
bf_rt_field_id_t smi_id::D_COPP_METER_METER_SPEC_CIR_PPS;
bf_rt_field_id_t smi_id::D_COPP_METER_METER_SPEC_PIR_PPS;
bf_rt_field_id_t smi_id::D_COPP_METER_METER_SPEC_CBS_PKTS;
bf_rt_field_id_t smi_id::D_COPP_METER_METER_SPEC_PBS_PKTS;

bf_rt_table_id_t smi_id::T_EGRESS_COPP;
bf_rt_field_id_t smi_id::F_EGRESS_COPP_PACKET_COLOR;
bf_rt_field_id_t smi_id::F_EGRESS_COPP_COPP_METER_ID;
bf_rt_field_id_t smi_id::F_EGRESS_COPP_PRIORITY;
bf_rt_action_id_t smi_id::A_EGRESS_COPP_DROP;
bf_rt_action_id_t smi_id::A_EGRESS_COPP_PERMIT;
bf_rt_field_id_t smi_id::D_EGRESS_COPP_STATS_COUNTER_SPEC_PKTS;

bf_rt_table_id_t smi_id::T_EGRESS_COPP_METER;
bf_rt_field_id_t smi_id::F_EGRESS_COPP_METER_METER_INDEX;
bf_rt_field_id_t smi_id::D_EGRESS_COPP_METER_METER_SPEC_CIR_PPS;
bf_rt_field_id_t smi_id::D_EGRESS_COPP_METER_METER_SPEC_PIR_PPS;
bf_rt_field_id_t smi_id::D_EGRESS_COPP_METER_METER_SPEC_CBS_PKTS;
bf_rt_field_id_t smi_id::D_EGRESS_COPP_METER_METER_SPEC_PBS_PKTS;

bf_rt_table_id_t smi_id::T_EGRESS_MAC_ACL;
bf_rt_field_id_t smi_id::F_EGRESS_MAC_ACL_LOCAL_MD_EGRESS_PORT_LAG_LABEL;
bf_rt_field_id_t smi_id::F_EGRESS_MAC_ACL_LOCAL_MD_BD_LABEL;
bf_rt_field_id_t smi_id::F_EGRESS_MAC_ACL_LOCAL_MD_BD;
bf_rt_field_id_t smi_id::F_EGRESS_MAC_ACL_SRC_ADDR;
bf_rt_field_id_t smi_id::F_EGRESS_MAC_ACL_DST_ADDR;
bf_rt_field_id_t smi_id::F_EGRESS_MAC_ACL_ETHER_TYPE;
bf_rt_field_id_t smi_id::F_EGRESS_MAC_ACL_LOCAL_MD_USER_METADATA;
bf_rt_field_id_t smi_id::F_EGRESS_MAC_ACL_MATCH_PRIORITY;
bf_rt_field_id_t smi_id::F_EGRESS_MAC_ACL_LOCAL_MD_EGRESS_PORT_LAG_INDEX;
bf_rt_field_id_t smi_id::F_EGRESS_MAC_ACL_HDR_VLAN_TAG0_PCP;
bf_rt_field_id_t smi_id::F_EGRESS_MAC_ACL_HDR_VLAN_TAG0_DEI;
bf_rt_field_id_t smi_id::F_EGRESS_MAC_ACL_HDR_VLAN_TAG0_VALID;
bf_rt_field_id_t smi_id::F_EGRESS_MAC_ACL_LOCAL_MD_FLAGS_ROUTED;
bf_rt_action_id_t smi_id::A_EGRESS_MAC_ACL_NO_ACTION;
bf_rt_action_id_t smi_id::A_EGRESS_MAC_ACL_DROP;
bf_rt_action_id_t smi_id::A_EGRESS_MAC_ACL_PERMIT;
bf_rt_field_id_t smi_id::P_EGRESS_MAC_ACL_PERMIT_METER_INDEX;
bf_rt_action_id_t smi_id::A_EGRESS_MAC_ACL_MIRROR;
bf_rt_field_id_t smi_id::P_EGRESS_MAC_ACL_MIRROR_METER_INDEX;
bf_rt_field_id_t smi_id::P_EGRESS_MAC_ACL_MIRROR_SESSION_ID;
bf_rt_field_id_t smi_id::D_EGRESS_MAC_ACL_COUNTER_SPEC_BYTES;
bf_rt_field_id_t smi_id::D_EGRESS_MAC_ACL_COUNTER_SPEC_PKTS;

bf_rt_table_id_t smi_id::T_EGRESS_L4_SRC_PORT;
bf_rt_field_id_t smi_id::F_EGRESS_L4_SRC_PORT_LKP_L4_SRC_PORT;
bf_rt_field_id_t smi_id::F_EGRESS_L4_SRC_PORT_PRIORITY;
bf_rt_action_id_t smi_id::A_EGRESS_SET_SRC_PORT_LABEL;
bf_rt_field_id_t smi_id::P_EGRESS_SET_SRC_PORT_LABEL_LABEL;

bf_rt_table_id_t smi_id::T_EGRESS_L4_DST_PORT;
bf_rt_field_id_t smi_id::F_EGRESS_L4_DST_PORT_LKP_L4_DST_PORT;
bf_rt_field_id_t smi_id::F_EGRESS_L4_DST_PORT_PRIORITY;
bf_rt_action_id_t smi_id::A_EGRESS_SET_DST_PORT_LABEL;
bf_rt_field_id_t smi_id::P_EGRESS_SET_DST_PORT_LABEL_LABEL;

bf_rt_table_id_t smi_id::T_EGRESS_IPV4_ACL;
bf_rt_field_id_t smi_id::F_EGRESS_IPV4_ACL_LOCAL_MD_EGRESS_PORT_LAG_LABEL;
bf_rt_field_id_t smi_id::F_EGRESS_IPV4_ACL_LOCAL_MD_BD_LABEL;
bf_rt_field_id_t smi_id::F_EGRESS_IPV4_ACL_LOCAL_MD_BD;
bf_rt_field_id_t smi_id::F_EGRESS_IPV4_ACL_HDR_IPV4_SRC_ADDR;
bf_rt_field_id_t smi_id::F_EGRESS_IPV4_ACL_HDR_IPV4_DST_ADDR;
bf_rt_field_id_t smi_id::F_EGRESS_IPV4_ACL_HDR_IPV4_PROTOCOL;
bf_rt_field_id_t smi_id::F_EGRESS_IPV4_ACL_HDR_IPV4_DIFFSERV;
bf_rt_field_id_t smi_id::F_EGRESS_IPV4_ACL_LOCAL_MD_LKP_TCP_FLAGS;
bf_rt_field_id_t smi_id::F_EGRESS_IPV4_ACL_LOCAL_MD_LKP_L4_SRC_PORT;
bf_rt_field_id_t smi_id::F_EGRESS_IPV4_ACL_LOCAL_MD_LKP_L4_DST_PORT;
bf_rt_field_id_t smi_id::F_EGRESS_IPV4_ACL_LOCAL_MD_L4_SRC_PORT_LABEL;
bf_rt_field_id_t smi_id::F_EGRESS_IPV4_ACL_LOCAL_MD_L4_DST_PORT_LABEL;
bf_rt_field_id_t smi_id::F_EGRESS_IPV4_ACL_LOCAL_MD_USER_METADATA;
bf_rt_field_id_t smi_id::F_EGRESS_IPV4_ACL_LOCAL_MD_OUT_PORTS_GROUP_LABEL;
bf_rt_field_id_t smi_id::F_EGRESS_IPV4_ACL_ETHER_TYPE;
bf_rt_field_id_t smi_id::F_EGRESS_IPV4_ACL_MATCH_PRIORITY;
bf_rt_field_id_t smi_id::F_EGRESS_IPV4_ACL_LOCAL_MD_EGRESS_PORT_LAG_INDEX;
bf_rt_field_id_t smi_id::F_EGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_SRC_ADDR;
bf_rt_field_id_t smi_id::F_EGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_DST_ADDR;
bf_rt_field_id_t smi_id::F_EGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_PROTO;
bf_rt_field_id_t smi_id::F_EGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_TOS;
bf_rt_field_id_t smi_id::F_EGRESS_IPV4_ACL_LOCAL_MD_FLAGS_ROUTED;
bf_rt_action_id_t smi_id::A_EGRESS_IPV4_ACL_NO_ACTION;
bf_rt_action_id_t smi_id::A_EGRESS_IPV4_ACL_DROP;
bf_rt_action_id_t smi_id::A_EGRESS_IPV4_ACL_PERMIT;
bf_rt_field_id_t smi_id::P_EGRESS_IPV4_ACL_PERMIT_METER_INDEX;
bf_rt_action_id_t smi_id::A_EGRESS_IPV4_ACL_MIRROR;
bf_rt_field_id_t smi_id::P_EGRESS_IPV4_ACL_MIRROR_METER_INDEX;
bf_rt_field_id_t smi_id::P_EGRESS_IPV4_ACL_MIRROR_SESSION_ID;
bf_rt_action_id_t smi_id::A_EGRESS_IPV4_ACL_MIRROR_IN;
bf_rt_field_id_t smi_id::P_EGRESS_IPV4_ACL_MIRROR_IN_METER_INDEX;
bf_rt_field_id_t smi_id::P_EGRESS_IPV4_ACL_MIRROR_IN_SESSION_ID;
bf_rt_field_id_t smi_id::D_EGRESS_IPV4_ACL_COUNTER_SPEC_BYTES;
bf_rt_field_id_t smi_id::D_EGRESS_IPV4_ACL_COUNTER_SPEC_PKTS;

bf_rt_table_id_t smi_id::T_EGRESS_IPV6_ACL;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_ACL_LOCAL_MD_EGRESS_PORT_LAG_LABEL;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_ACL_LOCAL_MD_BD_LABEL;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_ACL_LOCAL_MD_BD;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_ACL_HDR_IPV6_SRC_ADDR;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_ACL_HDR_IPV6_SRC_ADDR_WORD3;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_ACL_HDR_IPV6_SRC_ADDR_WORD2;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_ACL_HDR_IPV6_DST_ADDR;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_ACL_HDR_IPV6_DST_ADDR_WORD3;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_ACL_HDR_IPV6_DST_ADDR_WORD2;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_ACL_HDR_IPV6_NEXT_HDR;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_ACL_HDR_IPV6_TRAFFIC_CLASS;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_ACL_LOCAL_MD_LKP_TCP_FLAGS;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_ACL_LOCAL_MD_LKP_L4_SRC_PORT;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_ACL_LOCAL_MD_LKP_L4_DST_PORT;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_ACL_LOCAL_MD_L4_SRC_PORT_LABEL;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_ACL_LOCAL_MD_L4_DST_PORT_LABEL;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_ACL_LOCAL_MD_USER_METADATA;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_ACL_LOCAL_MD_OUT_PORTS_GROUP_LABEL;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_ACL_ETHER_TYPE;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_ACL_MATCH_PRIORITY;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_ACL_LOCAL_MD_EGRESS_PORT_LAG_INDEX;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_SRC_ADDR;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_ACL_HDR_IP_SRC_ADDR_WORD3;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD2;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_ACL_HDR_IPV6_SRC_ADDR_WORD10;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_DST_ADDR;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_ACL_HDR_IP_DST_ADDR_WORD3;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD2;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_ACL_HDR_IPV6_DST_ADDR_WORD10;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_PROTO;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_TOS;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_ACL_LOCAL_MD_FLAGS_ROUTED;
bf_rt_action_id_t smi_id::A_EGRESS_IPV6_ACL_NO_ACTION;
bf_rt_action_id_t smi_id::A_EGRESS_IPV6_ACL_DROP;
bf_rt_action_id_t smi_id::A_EGRESS_IPV6_ACL_PERMIT;
bf_rt_field_id_t smi_id::P_EGRESS_IPV6_ACL_PERMIT_METER_INDEX;
bf_rt_action_id_t smi_id::A_EGRESS_IPV6_ACL_MIRROR;
bf_rt_field_id_t smi_id::P_EGRESS_IPV6_ACL_MIRROR_METER_INDEX;
bf_rt_field_id_t smi_id::P_EGRESS_IPV6_ACL_MIRROR_SESSION_ID;
bf_rt_action_id_t smi_id::A_EGRESS_IPV6_ACL_MIRROR_IN;
bf_rt_field_id_t smi_id::P_EGRESS_IPV6_ACL_MIRROR_IN_METER_INDEX;
bf_rt_field_id_t smi_id::P_EGRESS_IPV6_ACL_MIRROR_IN_SESSION_ID;
bf_rt_field_id_t smi_id::D_EGRESS_IPV6_ACL_COUNTER_SPEC_BYTES;
bf_rt_field_id_t smi_id::D_EGRESS_IPV6_ACL_COUNTER_SPEC_PKTS;

bf_rt_table_id_t smi_id::T_EGRESS_TOS_MIRROR_ACL;
bf_rt_field_id_t smi_id::F_EGRESS_TOS_MIRROR_ACL_LOCAL_MD_EGRESS_PORT_LAG_LABEL;
bf_rt_field_id_t smi_id::F_EGRESS_TOS_MIRROR_ACL_MATCH_PRIORITY;
bf_rt_field_id_t smi_id::F_EGRESS_TOS_MIRROR_ACL_HDR_IPV6_VALID;
bf_rt_field_id_t smi_id::F_EGRESS_TOS_MIRROR_ACL_HDR_IPV6_TRAFFIC_CLASS;
bf_rt_field_id_t smi_id::F_EGRESS_TOS_MIRROR_ACL_HDR_IPV4_VALID;
bf_rt_field_id_t smi_id::F_EGRESS_TOS_MIRROR_ACL_HDR_IPV4_DIFFSERV;
bf_rt_action_id_t smi_id::A_EGRESS_TOS_MIRROR_ACL_MIRROR;
bf_rt_field_id_t smi_id::P_EGRESS_TOS_MIRROR_ACL_MIRROR_METER_INDEX;
bf_rt_field_id_t smi_id::P_EGRESS_TOS_MIRROR_ACL_MIRROR_SESSION_ID;
bf_rt_field_id_t smi_id::D_EGRESS_TOS_MIRROR_ACL_COUNTER_SPEC_BYTES;
bf_rt_field_id_t smi_id::D_EGRESS_TOS_MIRROR_ACL_COUNTER_SPEC_PKTS;
bf_rt_action_id_t smi_id::A_EGRESS_TOS_MIRROR_ACL_MIRROR_IN;
bf_rt_field_id_t smi_id::P_EGRESS_TOS_MIRROR_ACL_MIRROR_IN_METER_INDEX;
bf_rt_field_id_t smi_id::P_EGRESS_TOS_MIRROR_ACL_MIRROR_IN_SESSION_ID;

bf_rt_table_id_t smi_id::T_EGRESS_DROP_STATS;
bf_rt_field_id_t smi_id::F_EGRESS_DROP_STATS_PORT;
bf_rt_field_id_t smi_id::F_EGRESS_DROP_STATS_DROP_REASON;
bf_rt_action_id_t smi_id::A_EGRESS_DROP_STATS_COUNT;
bf_rt_field_id_t smi_id::D_EGRESS_DROP_STATS_PKTS;

bf_rt_table_id_t smi_id::T_EGRESS_SYSTEM_ACL;
bf_rt_field_id_t smi_id::F_EGRESS_SYSTEM_ACL_EG_INTR_MD_EGRESS_PORT;
bf_rt_field_id_t smi_id::F_EGRESS_SYSTEM_ACL_LOCAL_MD_FLAGS_ACL_DENY;
bf_rt_field_id_t smi_id::F_EGRESS_SYSTEM_ACL_LOCAL_MD_FLAGS_WRED_DROP;
bf_rt_field_id_t smi_id::F_EGRESS_SYSTEM_ACL_LOCAL_MD_FLAGS_PFC_WD_DROP;
bf_rt_field_id_t smi_id::F_EGRESS_SYSTEM_ACL_LOCAL_MD_CHECKS_MTU;
bf_rt_field_id_t smi_id::F_EGRESS_SYSTEM_ACL_LOCAL_MD_FLAGS_METER_PACKET_ACTION;
bf_rt_field_id_t smi_id::F_EGRESS_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_METER_DROP;
bf_rt_field_id_t smi_id::F_EGRESS_SYSTEM_ACL_LOCAL_MD_QOS_ACL_METER_COLOR;
bf_rt_field_id_t smi_id::F_EGRESS_SYSTEM_ACL_LOCAL_MD_MIRROR_ACL_METER_COLOR;
bf_rt_field_id_t
    smi_id::F_EGRESS_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_ISOLATION_PACKET_DROP;
bf_rt_field_id_t
    smi_id::F_EGRESS_SYSTEM_ACL_LOCAL_MD_FLAGS_BPORT_ISOLATION_PACKET_DROP;
bf_rt_field_id_t smi_id::F_EGRESS_SYSTEM_ACL_PRIORITY;
bf_rt_field_id_t smi_id::F_EGRESS_SYSTEM_ACL_LOCAL_MD_CHECKS_STP;
bf_rt_action_id_t smi_id::A_EGRESS_SYSTEM_ACL_DROP;
bf_rt_field_id_t smi_id::P_EGRESS_SYSTEM_ACL_DROP_REASON_CODE;
bf_rt_action_id_t smi_id::A_EGRESS_SYSTEM_ACL_MIRROR_METER_DROP;
bf_rt_field_id_t smi_id::P_EGRESS_SYSTEM_ACL_MIRROR_METER_DROP_REASON_CODE;
bf_rt_action_id_t smi_id::A_EGRESS_SYSTEM_ACL_COPY_TO_CPU;
bf_rt_field_id_t smi_id::P_EGRESS_SYSTEM_ACL_COPY_TO_CPU_REASON_CODE;
bf_rt_field_id_t smi_id::P_EGRESS_SYSTEM_ACL_COPY_TO_CPU_METER_ID;
bf_rt_action_id_t smi_id::A_EGRESS_SYSTEM_ACL_REDIRECT_TO_CPU;
bf_rt_field_id_t smi_id::P_EGRESS_SYSTEM_ACL_REDIRECT_TO_CPU_REASON_CODE;
bf_rt_field_id_t smi_id::P_EGRESS_SYSTEM_ACL_REDIRECT_TO_CPU_METER_ID;
bf_rt_field_id_t smi_id::D_EGRESS_SYSTEM_ACL_DROP_STATS_PKTS;

bf_rt_table_id_t smi_id::T_EGRESS_IPV4_MIRROR_ACL;
bf_rt_field_id_t
    smi_id::F_EGRESS_IPV4_MIRROR_ACL_LOCAL_MD_EGRESS_PORT_LAG_LABEL;
bf_rt_field_id_t smi_id::F_EGRESS_IPV4_MIRROR_ACL_LOCAL_MD_BD_LABEL;
bf_rt_field_id_t smi_id::F_EGRESS_IPV4_MIRROR_ACL_LOCAL_MD_BD;
bf_rt_field_id_t smi_id::F_EGRESS_IPV4_MIRROR_ACL_HDR_IPV4_SRC_ADDR;
bf_rt_field_id_t smi_id::F_EGRESS_IPV4_MIRROR_ACL_HDR_IPV4_DST_ADDR;
bf_rt_field_id_t smi_id::F_EGRESS_IPV4_MIRROR_ACL_HDR_IPV4_PROTOCOL;
bf_rt_field_id_t smi_id::F_EGRESS_IPV4_MIRROR_ACL_HDR_IPV4_DIFFSERV;
bf_rt_field_id_t smi_id::F_EGRESS_IPV4_MIRROR_ACL_ETHER_TYPE;
bf_rt_field_id_t smi_id::F_EGRESS_IPV4_MIRROR_ACL_LOCAL_MD_LKP_TCP_FLAGS;
bf_rt_field_id_t smi_id::F_EGRESS_IPV4_MIRROR_ACL_LOCAL_MD_LKP_L4_SRC_PORT;
bf_rt_field_id_t smi_id::F_EGRESS_IPV4_MIRROR_ACL_LOCAL_MD_LKP_L4_DST_PORT;
bf_rt_field_id_t smi_id::F_EGRESS_IPV4_MIRROR_ACL_LOCAL_MD_L4_SRC_PORT_LABEL;
bf_rt_field_id_t smi_id::F_EGRESS_IPV4_MIRROR_ACL_LOCAL_MD_L4_DST_PORT_LABEL;
bf_rt_field_id_t
    smi_id::F_EGRESS_IPV4_MIRROR_ACL_LOCAL_MD_OUT_PORTS_GROUP_LABEL;
bf_rt_field_id_t smi_id::F_EGRESS_IPV4_MIRROR_ACL_MATCH_PRIORITY;
bf_rt_action_id_t smi_id::A_EGRESS_IPV4_MIRROR_ACL_NO_ACTION;
bf_rt_action_id_t smi_id::A_EGRESS_IPV4_MIRROR_ACL_DROP;
bf_rt_action_id_t smi_id::A_EGRESS_IPV4_MIRROR_ACL_PERMIT;
bf_rt_field_id_t smi_id::P_EGRESS_IPV4_MIRROR_ACL_PERMIT_METER_INDEX;
bf_rt_action_id_t smi_id::A_EGRESS_IPV4_MIRROR_ACL_MIRROR;
bf_rt_field_id_t smi_id::P_EGRESS_IPV4_MIRROR_ACL_MIRROR_METER_INDEX;
bf_rt_field_id_t smi_id::P_EGRESS_IPV4_MIRROR_ACL_MIRROR_SESSION_ID;
bf_rt_action_id_t smi_id::A_EGRESS_IPV4_MIRROR_ACL_MIRROR_IN;
bf_rt_field_id_t smi_id::P_EGRESS_IPV4_MIRROR_ACL_MIRROR_IN_METER_INDEX;
bf_rt_field_id_t smi_id::P_EGRESS_IPV4_MIRROR_ACL_MIRROR_IN_SESSION_ID;
bf_rt_field_id_t smi_id::D_EGRESS_IPV4_MIRROR_ACL_COUNTER_SPEC_BYTES;
bf_rt_field_id_t smi_id::D_EGRESS_IPV4_MIRROR_ACL_COUNTER_SPEC_PKTS;

bf_rt_table_id_t smi_id::T_EGRESS_IPV6_MIRROR_ACL;
bf_rt_field_id_t
    smi_id::F_EGRESS_IPV6_MIRROR_ACL_LOCAL_MD_EGRESS_PORT_LAG_LABEL;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_MIRROR_ACL_LOCAL_MD_BD_LABEL;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_MIRROR_ACL_LOCAL_MD_BD;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_MIRROR_ACL_HDR_IPV6_SRC_ADDR;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_MIRROR_ACL_HDR_IPV6_DST_ADDR;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_MIRROR_ACL_HDR_IPV6_NEXT_HDR;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_MIRROR_ACL_HDR_IPV6_TRAFFIC_CLASS;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_MIRROR_ACL_ETHER_TYPE;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_MIRROR_ACL_LOCAL_MD_LKP_TCP_FLAGS;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_MIRROR_ACL_LOCAL_MD_LKP_L4_SRC_PORT;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_MIRROR_ACL_LOCAL_MD_LKP_L4_DST_PORT;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_MIRROR_ACL_LOCAL_MD_L4_SRC_PORT_LABEL;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_MIRROR_ACL_LOCAL_MD_L4_DST_PORT_LABEL;
bf_rt_field_id_t
    smi_id::F_EGRESS_IPV6_MIRROR_ACL_LOCAL_MD_OUT_PORTS_GROUP_LABEL;
bf_rt_field_id_t smi_id::F_EGRESS_IPV6_MIRROR_ACL_MATCH_PRIORITY;
bf_rt_action_id_t smi_id::A_EGRESS_IPV6_MIRROR_ACL_NO_ACTION;
bf_rt_action_id_t smi_id::A_EGRESS_IPV6_MIRROR_ACL_DROP;
bf_rt_action_id_t smi_id::A_EGRESS_IPV6_MIRROR_ACL_PERMIT;
bf_rt_field_id_t smi_id::P_EGRESS_IPV6_MIRROR_ACL_PERMIT_METER_INDEX;
bf_rt_action_id_t smi_id::A_EGRESS_IPV6_MIRROR_ACL_MIRROR;
bf_rt_field_id_t smi_id::P_EGRESS_IPV6_MIRROR_ACL_MIRROR_METER_INDEX;
bf_rt_field_id_t smi_id::P_EGRESS_IPV6_MIRROR_ACL_MIRROR_SESSION_ID;
bf_rt_action_id_t smi_id::A_EGRESS_IPV6_MIRROR_ACL_MIRROR_IN;
bf_rt_field_id_t smi_id::P_EGRESS_IPV6_MIRROR_ACL_MIRROR_IN_METER_INDEX;
bf_rt_field_id_t smi_id::P_EGRESS_IPV6_MIRROR_ACL_MIRROR_IN_SESSION_ID;
bf_rt_field_id_t smi_id::D_EGRESS_IPV6_MIRROR_ACL_COUNTER_SPEC_BYTES;
bf_rt_field_id_t smi_id::D_EGRESS_IPV6_MIRROR_ACL_COUNTER_SPEC_PKTS;

//  start acl2.p4 egress
bf_rt_table_id_t smi_id::T_EGRESS_IP_MIRROR_ACL;
bf_rt_field_id_t smi_id::F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_BD;
bf_rt_field_id_t smi_id::F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_MIRROR_MAC_LABEL;
bf_rt_field_id_t smi_id::F_EGRESS_IP_MIRROR_ACL_HDR_IP_SRC_ADDR_WORD3;
bf_rt_field_id_t smi_id::F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD2;
bf_rt_field_id_t smi_id::F_EGRESS_IP_MIRROR_ACL_HDR_IPV6_SRC_ADDR_WORD10;
bf_rt_field_id_t smi_id::F_EGRESS_IP_MIRROR_ACL_HDR_IP_DST_ADDR_WORD3;
bf_rt_field_id_t smi_id::F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD2;
bf_rt_field_id_t smi_id::F_EGRESS_IP_MIRROR_ACL_HDR_IPV6_DST_ADDR_WORD10;
bf_rt_field_id_t smi_id::F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_PROTOCOL;
bf_rt_field_id_t smi_id::F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_TOS;
bf_rt_field_id_t smi_id::F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_TCP_FLAGS;
bf_rt_field_id_t smi_id::F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_L4_SRC_PORT;
bf_rt_field_id_t smi_id::F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_L4_DST_PORT;
bf_rt_field_id_t smi_id::F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_EGRESS_PORT_LAG_INDEX;
bf_rt_field_id_t smi_id::F_EGRESS_IP_MIRROR_ACL_MATCH_PRIORITY;
bf_rt_field_id_t smi_id::F_EGRESS_IP_MIRROR_ACL_HDR_VLAN_TAG0_PCP;
bf_rt_field_id_t smi_id::F_EGRESS_IP_MIRROR_ACL_HDR_VLAN_TAG0_DEI;
bf_rt_field_id_t smi_id::F_EGRESS_IP_MIRROR_ACL_HDR_VLAN_TAG0_VALID;
bf_rt_field_id_t smi_id::F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_FLAGS_ROUTED;
bf_rt_action_id_t smi_id::A_EGRESS_IP_MIRROR_ACL_NO_ACTION;
bf_rt_field_id_t smi_id::P_EGRESS_IP_MIRROR_ACL_NO_ACTION_METER_INDEX;
bf_rt_action_id_t smi_id::A_EGRESS_IP_MIRROR_ACL_MIRROR;
bf_rt_field_id_t smi_id::P_EGRESS_IP_MIRROR_ACL_MIRROR_METER_INDEX;
bf_rt_field_id_t smi_id::P_EGRESS_IP_MIRROR_ACL_MIRROR_SESSION_ID;
bf_rt_field_id_t smi_id::D_EGRESS_IP_MIRROR_ACL_COUNTER_SPEC_BYTES;
bf_rt_field_id_t smi_id::D_EGRESS_IP_MIRROR_ACL_COUNTER_SPEC_PKTS;
bf_rt_field_id_t smi_id::D_EGRESS_IP_MIRROR_ACL_METER_SPEC_CIR_KBPS;
bf_rt_field_id_t smi_id::D_EGRESS_IP_MIRROR_ACL_METER_SPEC_PIR_KBPS;
bf_rt_field_id_t smi_id::D_EGRESS_IP_MIRROR_ACL_METER_SPEC_CBS_KBITS;
bf_rt_field_id_t smi_id::D_EGRESS_IP_MIRROR_ACL_METER_SPEC_PBS_KBITS;

bf_rt_table_id_t smi_id::T_EGRESS_IP_MIRROR_ACL_METER_ACTION;
bf_rt_field_id_t smi_id::F_EGRESS_IP_MIRROR_ACL_METER_ACTION_COLOR;
bf_rt_field_id_t smi_id::F_EGRESS_IP_MIRROR_ACL_METER_ACTION_INDEX;
bf_rt_action_id_t smi_id::A_EGRESS_IP_MIRROR_ACL_METER_ACTION_COUNT;
bf_rt_field_id_t smi_id::D_EGRESS_IP_MIRROR_ACL_METER_ACTION_COUNTER_SPEC_BYTES;

bf_rt_table_id_t smi_id::T_EGRESS_IP_QOS_ACL;
bf_rt_field_id_t smi_id::F_EGRESS_IP_QOS_ACL_LOCAL_MD_BD;
bf_rt_field_id_t smi_id::F_EGRESS_IP_QOS_ACL_LOCAL_MD_QOS_MAC_LABEL;
bf_rt_field_id_t smi_id::F_EGRESS_IP_QOS_ACL_HDR_IP_SRC_ADDR_WORD3;
bf_rt_field_id_t smi_id::F_EGRESS_IP_QOS_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD2;
bf_rt_field_id_t smi_id::F_EGRESS_IP_QOS_ACL_HDR_IPV6_SRC_ADDR_WORD10;
bf_rt_field_id_t smi_id::F_EGRESS_IP_QOS_ACL_HDR_IP_DST_ADDR_WORD3;
bf_rt_field_id_t smi_id::F_EGRESS_IP_QOS_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD2;
bf_rt_field_id_t smi_id::F_EGRESS_IP_QOS_ACL_HDR_IPV6_DST_ADDR_WORD10;
bf_rt_field_id_t smi_id::F_EGRESS_IP_QOS_ACL_LOCAL_MD_LKP_IP_PROTOCOL;
bf_rt_field_id_t smi_id::F_EGRESS_IP_QOS_ACL_LOCAL_MD_LKP_IP_TOS;
bf_rt_field_id_t smi_id::F_EGRESS_IP_QOS_ACL_LOCAL_MD_LKP_TCP_FLAGS;
bf_rt_field_id_t smi_id::F_EGRESS_IP_QOS_ACL_LOCAL_MD_LKP_L4_SRC_PORT;
bf_rt_field_id_t smi_id::F_EGRESS_IP_QOS_ACL_LOCAL_MD_LKP_L4_DST_PORT;
bf_rt_field_id_t smi_id::F_EGRESS_IP_QOS_ACL_LOCAL_MD_EGRESS_PORT_LAG_INDEX;
bf_rt_field_id_t smi_id::F_EGRESS_IP_QOS_ACL_MATCH_PRIORITY;
bf_rt_field_id_t smi_id::F_EGRESS_IP_QOS_ACL_HDR_VLAN_TAG0_PCP;
bf_rt_field_id_t smi_id::F_EGRESS_IP_QOS_ACL_HDR_VLAN_TAG0_DEI;
bf_rt_field_id_t smi_id::F_EGRESS_IP_QOS_ACL_HDR_VLAN_TAG0_VALID;
bf_rt_field_id_t smi_id::F_EGRESS_IP_QOS_ACL_LOCAL_MD_FLAGS_ROUTED;
bf_rt_action_id_t smi_id::A_EGRESS_IP_QOS_ACL_NO_ACTION;
bf_rt_field_id_t smi_id::P_EGRESS_IP_QOS_ACL_NO_ACTION_METER_INDEX;
bf_rt_action_id_t smi_id::A_EGRESS_IP_QOS_ACL_SET_PCP;
bf_rt_field_id_t smi_id::P_EGRESS_IP_QOS_ACL_SET_PCP_PCP;
bf_rt_field_id_t smi_id::P_EGRESS_IP_QOS_ACL_SET_PCP_METER_INDEX;
bf_rt_action_id_t smi_id::A_EGRESS_IP_QOS_ACL_SET_IPV4_TOS;
bf_rt_field_id_t smi_id::P_EGRESS_IP_QOS_ACL_SET_IPV4_TOS_TOS;
bf_rt_field_id_t smi_id::P_EGRESS_IP_QOS_ACL_SET_IPV4_TOS_METER_INDEX;
bf_rt_action_id_t smi_id::A_EGRESS_IP_QOS_ACL_SET_IPV6_TOS;
bf_rt_field_id_t smi_id::P_EGRESS_IP_QOS_ACL_SET_IPV6_TOS_TOS;
bf_rt_field_id_t smi_id::P_EGRESS_IP_QOS_ACL_SET_IPV6_TOS_METER_INDEX;
bf_rt_action_id_t smi_id::A_EGRESS_IP_QOS_ACL_SET_PCP_IPV4_TOS;
bf_rt_field_id_t smi_id::P_EGRESS_IP_QOS_ACL_SET_PCP_IPV4_TOS_TOS;
bf_rt_field_id_t smi_id::P_EGRESS_IP_QOS_ACL_SET_PCP_IPV4_TOS_PCP;
bf_rt_field_id_t smi_id::P_EGRESS_IP_QOS_ACL_SET_PCP_IPV4_TOS_METER_INDEX;
bf_rt_action_id_t smi_id::A_EGRESS_IP_QOS_ACL_SET_PCP_IPV6_TOS;
bf_rt_field_id_t smi_id::P_EGRESS_IP_QOS_ACL_SET_PCP_IPV6_TOS_TOS;
bf_rt_field_id_t smi_id::P_EGRESS_IP_QOS_ACL_SET_PCP_IPV6_TOS_PCP;
bf_rt_field_id_t smi_id::P_EGRESS_IP_QOS_ACL_SET_PCP_IPV6_TOS_METER_INDEX;
bf_rt_action_id_t smi_id::A_EGRESS_IP_QOS_ACL_SET_METER;
bf_rt_field_id_t smi_id::P_EGRESS_IP_QOS_ACL_SET_METER_INDEX;
bf_rt_action_id_t smi_id::A_EGRESS_IP_QOS_ACL_SET_QOS_PARAMS;
bf_rt_field_id_t smi_id::P_EGRESS_IP_QOS_ACL_SET_QOS_PARAMS_PCP;
bf_rt_field_id_t smi_id::P_EGRESS_IP_QOS_ACL_SET_QOS_PARAMS_TOS;
bf_rt_field_id_t smi_id::P_EGRESS_IP_QOS_ACL_SET_QOS_PARAMS_METER_INDEX;
bf_rt_field_id_t smi_id::D_EGRESS_IP_QOS_ACL_COUNTER_SPEC_BYTES;
bf_rt_field_id_t smi_id::D_EGRESS_IP_QOS_ACL_COUNTER_SPEC_PKTS;
bf_rt_field_id_t smi_id::D_EGRESS_IP_QOS_ACL_METER_SPEC_CIR_KBPS;
bf_rt_field_id_t smi_id::D_EGRESS_IP_QOS_ACL_METER_SPEC_PIR_KBPS;
bf_rt_field_id_t smi_id::D_EGRESS_IP_QOS_ACL_METER_SPEC_CBS_KBITS;
bf_rt_field_id_t smi_id::D_EGRESS_IP_QOS_ACL_METER_SPEC_PBS_KBITS;

bf_rt_table_id_t smi_id::T_EGRESS_IP_QOS_ACL_METER_ACTION;
bf_rt_field_id_t smi_id::F_EGRESS_IP_QOS_ACL_METER_ACTION_COLOR;
bf_rt_field_id_t smi_id::F_EGRESS_IP_QOS_ACL_METER_ACTION_INDEX;
bf_rt_action_id_t smi_id::A_EGRESS_IP_QOS_ACL_METER_ACTION_COUNT;
bf_rt_field_id_t smi_id::D_EGRESS_IP_QOS_ACL_METER_ACTION_COUNTER_SPEC_BYTES;

bf_rt_field_id_t smi_id::T_EGRESS_ACL_QOS_MACADDR;
bf_rt_field_id_t smi_id::F_EGRESS_ACL_QOS_MACADDR_PORT_LAG_INDEX;
bf_rt_field_id_t smi_id::F_EGRESS_ACL_QOS_MACADDR_SMAC_ADDR;
bf_rt_field_id_t smi_id::F_EGRESS_ACL_QOS_MACADDR_DMAC_ADDR;
bf_rt_action_id_t smi_id::A_EGRESS_ACL_QOS_MACADDR_SET_MACADDR_LABEL;
bf_rt_field_id_t smi_id::D_EGRESS_ACL_QOS_MACADDR_SET_MACADDR_LABEL;
bf_rt_field_id_t smi_id::F_EGRESS_ACL_QOS_MACADDR_MATCH_PRIORITY;

bf_rt_field_id_t smi_id::T_EGRESS_ACL_MIRROR_MACADDR;
bf_rt_field_id_t smi_id::F_EGRESS_ACL_MIRROR_MACADDR_PORT_LAG_INDEX;
bf_rt_field_id_t smi_id::F_EGRESS_ACL_MIRROR_MACADDR_SMAC_ADDR;
bf_rt_field_id_t smi_id::F_EGRESS_ACL_MIRROR_MACADDR_DMAC_ADDR;
bf_rt_action_id_t smi_id::A_EGRESS_ACL_MIRROR_MACADDR_SET_MACADDR_LABEL;
bf_rt_field_id_t smi_id::D_EGRESS_ACL_MIRROR_MACADDR_SET_MACADDR_LABEL;
bf_rt_field_id_t smi_id::F_EGRESS_ACL_MIRROR_MACADDR_MATCH_PRIORITY;
//  acl2.p4 egress complete

/* multicast.p4 */
bf_rt_table_id_t smi_id::T_IPV4_MULTICAST_BRIDGE_S_G;
bf_rt_field_id_t smi_id::F_IPV4_MULTICAST_BRIDGE_S_G_BD;
bf_rt_field_id_t smi_id::F_IPV4_MULTICAST_BRIDGE_S_G_SRC_ADDR;
bf_rt_field_id_t smi_id::F_IPV4_MULTICAST_BRIDGE_S_G_GRP_ADDR;
bf_rt_action_id_t smi_id::A_IPV4_MULTICAST_BRIDGE_S_G_HIT;
bf_rt_field_id_t smi_id::P_IPV4_MULTICAST_BRIDGE_S_G_HIT_MGID;

bf_rt_table_id_t smi_id::T_IPV4_MULTICAST_BRIDGE_X_G;
bf_rt_field_id_t smi_id::F_IPV4_MULTICAST_BRIDGE_X_G_BD;
bf_rt_field_id_t smi_id::F_IPV4_MULTICAST_BRIDGE_X_G_GRP_ADDR;
bf_rt_action_id_t smi_id::A_IPV4_MULTICAST_BRIDGE_X_G_HIT;
bf_rt_field_id_t smi_id::P_IPV4_MULTICAST_BRIDGE_X_G_HIT_MGID;

bf_rt_table_id_t smi_id::T_IPV4_MULTICAST_ROUTE_S_G;
bf_rt_field_id_t smi_id::F_IPV4_MULTICAST_ROUTE_S_G_VRF;
bf_rt_field_id_t smi_id::F_IPV4_MULTICAST_ROUTE_S_G_SRC_ADDR;
bf_rt_field_id_t smi_id::F_IPV4_MULTICAST_ROUTE_S_G_GRP_ADDR;
bf_rt_action_id_t smi_id::A_IPV4_MULTICAST_ROUTE_S_G_HIT;
bf_rt_field_id_t smi_id::P_IPV4_MULTICAST_ROUTE_S_G_HIT_MGID;
bf_rt_field_id_t smi_id::P_IPV4_MULTICAST_ROUTE_S_G_HIT_RPF_GROUP;
bf_rt_field_id_t smi_id::P_IPV4_MULTICAST_ROUTE_S_G_HIT_PKTS;

bf_rt_table_id_t smi_id::T_IPV4_MULTICAST_ROUTE_X_G;
bf_rt_field_id_t smi_id::F_IPV4_MULTICAST_ROUTE_X_G_VRF;
bf_rt_field_id_t smi_id::F_IPV4_MULTICAST_ROUTE_X_G_GRP_ADDR;
bf_rt_action_id_t smi_id::A_IPV4_MULTICAST_ROUTE_X_G_HIT_BIDIR;
bf_rt_field_id_t smi_id::P_IPV4_MULTICAST_ROUTE_X_G_HIT_BIDIR_MGID;
bf_rt_field_id_t smi_id::P_IPV4_MULTICAST_ROUTE_X_G_HIT_BIDIR_RPF_GROUP;
bf_rt_action_id_t smi_id::A_IPV4_MULTICAST_ROUTE_X_G_HIT_SM;
bf_rt_field_id_t smi_id::P_IPV4_MULTICAST_ROUTE_X_G_HIT_SM_MGID;
bf_rt_field_id_t smi_id::P_IPV4_MULTICAST_ROUTE_X_G_HIT_SM_RPF_GROUP;
bf_rt_field_id_t smi_id::P_IPV4_MULTICAST_ROUTE_X_G_HIT_SM_PKTS;

bf_rt_table_id_t smi_id::T_IPV6_MULTICAST_BRIDGE_S_G;
bf_rt_field_id_t smi_id::F_IPV6_MULTICAST_BRIDGE_S_G_BD;
bf_rt_field_id_t smi_id::F_IPV6_MULTICAST_BRIDGE_S_G_GRP_ADDR;
bf_rt_field_id_t smi_id::F_IPV6_MULTICAST_BRIDGE_S_G_SRC_ADDR;
bf_rt_action_id_t smi_id::A_IPV6_MULTICAST_BRIDGE_S_G_HIT;
bf_rt_field_id_t smi_id::P_IPV6_MULTICAST_BRIDGE_S_G_HIT_MGID;

bf_rt_table_id_t smi_id::T_IPV6_MULTICAST_BRIDGE_X_G;
bf_rt_field_id_t smi_id::F_IPV6_MULTICAST_BRIDGE_X_G_BD;
bf_rt_field_id_t smi_id::F_IPV6_MULTICAST_BRIDGE_X_G_GRP_ADDR;
bf_rt_action_id_t smi_id::A_IPV6_MULTICAST_BRIDGE_X_G_HIT;
bf_rt_field_id_t smi_id::P_IPV6_MULTICAST_BRIDGE_X_G_HIT_MGID;

bf_rt_table_id_t smi_id::T_IPV6_MULTICAST_ROUTE_S_G;
bf_rt_field_id_t smi_id::F_IPV6_MULTICAST_ROUTE_S_G_VRF;
bf_rt_field_id_t smi_id::F_IPV6_MULTICAST_ROUTE_S_G_GRP_ADDR;
bf_rt_field_id_t smi_id::F_IPV6_MULTICAST_ROUTE_S_G_SRC_ADDR;
bf_rt_action_id_t smi_id::A_IPV6_MULTICAST_ROUTE_S_G_HIT;
bf_rt_field_id_t smi_id::P_IPV6_MULTICAST_ROUTE_S_G_HIT_MGID;
bf_rt_field_id_t smi_id::P_IPV6_MULTICAST_ROUTE_S_G_HIT_RPF_GROUP;
bf_rt_field_id_t smi_id::P_IPV6_MULTICAST_ROUTE_S_G_HIT_PKTS;

bf_rt_table_id_t smi_id::T_IPV6_MULTICAST_ROUTE_X_G;
bf_rt_field_id_t smi_id::F_IPV6_MULTICAST_ROUTE_X_G_VRF;
bf_rt_field_id_t smi_id::F_IPV6_MULTICAST_ROUTE_X_G_GRP_ADDR;
bf_rt_action_id_t smi_id::A_IPV6_MULTICAST_ROUTE_X_G_HIT_BIDIR;
bf_rt_field_id_t smi_id::P_IPV6_MULTICAST_ROUTE_X_G_HIT_BIDIR_MGID;
bf_rt_field_id_t smi_id::P_IPV6_MULTICAST_ROUTE_X_G_HIT_BIDIR_RPF_GROUP;
bf_rt_field_id_t smi_id::P_IPV6_MULTICAST_ROUTE_X_G_HIT_BIDIR_PKTS;
bf_rt_action_id_t smi_id::A_IPV6_MULTICAST_ROUTE_X_G_HIT_SM;
bf_rt_field_id_t smi_id::P_IPV6_MULTICAST_ROUTE_X_G_HIT_SM_MGID;
bf_rt_field_id_t smi_id::P_IPV6_MULTICAST_ROUTE_X_G_HIT_SM_RPF_GROUP;
bf_rt_field_id_t smi_id::P_IPV6_MULTICAST_ROUTE_X_G_HIT_SM_PKTS;

bf_rt_table_id_t smi_id::T_MCAST_FWD_RESULT;
bf_rt_field_id_t smi_id::F_MCAST_FWD_RESULT_MULTICAST_HIT;
bf_rt_field_id_t smi_id::F_MCAST_FWD_RESULT_LKP_IP_TYPE;
bf_rt_field_id_t smi_id::F_MCAST_FWD_RESULT_IPV4_MCAST_SNOOPING;
bf_rt_field_id_t smi_id::F_MCAST_FWD_RESULT_IPV6_MCAST_SNOOPING;
bf_rt_field_id_t smi_id::F_MCAST_FWD_RESULT_LOCAL_MD_MULTICAST_MODE;
bf_rt_field_id_t smi_id::F_MCAST_FWD_RESULT_RPF_CHECK;
bf_rt_field_id_t smi_id::F_MCAST_FWD_RESULT_PRIORITY;
bf_rt_action_id_t smi_id::A_MCAST_FWD_RESULT_SET_MULTICAST_BRIDGE;
bf_rt_field_id_t smi_id::P_MCAST_FWD_RESULT_SET_MULTICAST_BRIDGE_MRPF;
bf_rt_action_id_t smi_id::A_MCAST_FWD_RESULT_SET_MULTICAST_ROUTE;
bf_rt_action_id_t smi_id::A_MCAST_FWD_RESULT_SET_MULTICAST_FLOOD;
bf_rt_field_id_t smi_id::P_MCAST_FWD_RESULT_SET_MULTICAST_FLOOD_MRPF;
bf_rt_field_id_t smi_id::P_MCAST_FWD_RESULT_SET_MULTICAST_FLOOD_FLOOD;

bf_rt_table_id_t smi_id::T_BD_FLOOD;
bf_rt_field_id_t smi_id::F_BD_FLOOD_BD;
bf_rt_field_id_t smi_id::F_BD_FLOOD_PKT_TYPE;
bf_rt_field_id_t smi_id::F_BD_FLOOD_FLOOD_TO_MROUTERS;
bf_rt_action_id_t smi_id::A_FLOOD;
bf_rt_field_id_t smi_id::P_FLOOD_MGID;

bf_rt_table_id_t smi_id::T_RID;
bf_rt_field_id_t smi_id::F_RID_REPLICATION_ID;
bf_rt_action_id_t smi_id::A_RID_HIT;
bf_rt_field_id_t smi_id::P_RID_HIT_BD;
bf_rt_action_id_t smi_id::A_RID_TUNNEL_RID_HIT;
bf_rt_field_id_t smi_id::P_RID_TUNNEL_RID_HIT_NEXTHOP;
bf_rt_field_id_t smi_id::P_RID_TUNNEL_RID_HIT_TUNNEL_NEXTHOP;
bf_rt_action_id_t smi_id::A_RID_TUNNEL_MC_RID_HIT;
bf_rt_field_id_t smi_id::P_RID_TUNNEL_MC_RID_HIT_BD;
bf_rt_field_id_t smi_id::P_RID_TUNNEL_MC_RID_HIT_NEXTHOP;
bf_rt_field_id_t smi_id::P_RID_TUNNEL_MC_RID_HIT_TUNNEL_NEXTHOP;

/* rewrite.p4 */
bf_rt_table_id_t smi_id::T_MIRROR_REWRITE;
bf_rt_field_id_t smi_id::F_MIRROR_REWRITE_MIRROR_MD_SESSION_ID;
bf_rt_action_id_t smi_id::A_REWRITE_DUMMY;
bf_rt_field_id_t smi_id::P_REWRITE_DUMMY_QUEUE_ID;
bf_rt_action_id_t smi_id::A_REWRITE_ERSPAN_TYPE2;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE2_QUEUE_ID;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE2_SMAC;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE2_DMAC;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE2_SIP;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE2_DIP;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE2_TOS;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE2_TTL;
bf_rt_action_id_t smi_id::A_REWRITE_ERSPAN_TYPE2_VLAN;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE2_VLAN_ETHER_TYPE;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE2_VLAN_PCP;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE2_VLAN_VID;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE2_VLAN_QUEUE_ID;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE2_VLAN_SMAC;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE2_VLAN_DMAC;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE2_VLAN_SIP;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE2_VLAN_DIP;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE2_VLAN_TOS;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE2_VLAN_TTL;
bf_rt_action_id_t smi_id::A_REWRITE_ERSPAN_TYPE3;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE3_QUEUE_ID;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE3_SMAC;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE3_DMAC;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE3_SIP;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE3_DIP;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE3_TOS;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE3_TTL;
bf_rt_action_id_t smi_id::A_REWRITE_ERSPAN_TYPE3_VLAN;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE3_VLAN_ETHER_TYPE;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE3_VLAN_PCP;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE3_VLAN_VID;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE3_VLAN_QUEUE_ID;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE3_VLAN_SMAC;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE3_VLAN_DMAC;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE3_VLAN_SIP;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE3_VLAN_DIP;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE3_VLAN_TOS;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE3_VLAN_TTL;
bf_rt_action_id_t smi_id::A_REWRITE_ERSPAN_TYPE3_PLAT;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE3_PLAT_QUEUE_ID;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE3_PLAT_SMAC;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE3_PLAT_DMAC;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE3_PLAT_SIP;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE3_PLAT_DIP;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE3_PLAT_TOS;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE3_PLAT_TTL;
bf_rt_action_id_t smi_id::A_REWRITE_ERSPAN_TYPE3_PLAT_VLAN;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_ETHER_TYPE;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_PCP;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_VID;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_QUEUE_ID;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_SMAC;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_DMAC;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_SIP;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_DIP;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_TOS;
bf_rt_field_id_t smi_id::P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_TTL;
bf_rt_action_id_t smi_id::A_REWRITE_RSPAN;
bf_rt_field_id_t smi_id::P_REWRITE_RSPAN_QUEUE_ID;
bf_rt_field_id_t smi_id::P_REWRITE_RSPAN_VID;
bf_rt_action_id_t smi_id::A_REWRITE_DTEL_REPORT;
bf_rt_field_id_t smi_id::P_REWRITE_DTEL_REPORT_SMAC;
bf_rt_field_id_t smi_id::P_REWRITE_DTEL_REPORT_DMAC;
bf_rt_field_id_t smi_id::P_REWRITE_DTEL_REPORT_SIP;
bf_rt_field_id_t smi_id::P_REWRITE_DTEL_REPORT_DIP;
bf_rt_field_id_t smi_id::P_REWRITE_DTEL_REPORT_UDP_DST_PORT;
bf_rt_field_id_t smi_id::P_REWRITE_DTEL_REPORT_UDP_SRC_PORT;
bf_rt_field_id_t smi_id::P_REWRITE_DTEL_REPORT_TTL;
bf_rt_field_id_t smi_id::P_REWRITE_DTEL_REPORT_TOS;
bf_rt_field_id_t smi_id::P_REWRITE_DTEL_REPORT_SESSION_ID;
bf_rt_field_id_t smi_id::P_REWRITE_DTEL_REPORT_MAX_PKT_LEN;
bf_rt_action_id_t smi_id::A_REWRITE_DTEL_REPORT_ENTROPY;
bf_rt_field_id_t smi_id::P_REWRITE_DTEL_REPORT_ENTROPY_SMAC;
bf_rt_field_id_t smi_id::P_REWRITE_DTEL_REPORT_ENTROPY_DMAC;
bf_rt_field_id_t smi_id::P_REWRITE_DTEL_REPORT_ENTROPY_SIP;
bf_rt_field_id_t smi_id::P_REWRITE_DTEL_REPORT_ENTROPY_DIP;
bf_rt_field_id_t smi_id::P_REWRITE_DTEL_REPORT_ENTROPY_UDP_DST_PORT;
bf_rt_field_id_t smi_id::P_REWRITE_DTEL_REPORT_ENTROPY_TTL;
bf_rt_field_id_t smi_id::P_REWRITE_DTEL_REPORT_ENTROPY_TOS;
bf_rt_field_id_t smi_id::P_REWRITE_DTEL_REPORT_ENTROPY_SESSION_ID;
bf_rt_field_id_t smi_id::P_REWRITE_DTEL_REPORT_ENTROPY_MAX_PKT_LEN;
bf_rt_action_id_t smi_id::A_REWRITE_IP_UDP_LENGTHS;
bf_rt_action_id_t smi_id::A_REWRITE_DTEL_IFA_CLONE;

bf_rt_table_id_t smi_id::T_TUNNEL_NEXTHOP;
bf_rt_field_id_t smi_id::F_TUNNEL_NEXTHOP_LOCAL_MD_TUNNEL_NEXTHOP;
bf_rt_action_id_t smi_id::A_TUNNEL_NEXTHOP_L2_TUNNEL_ENCAP;
bf_rt_field_id_t smi_id::P_TUNNEL_NEXTHOP_L2_TUNNEL_ENCAP_TYPE;
bf_rt_field_id_t smi_id::P_TUNNEL_NEXTHOP_L2_TUNNEL_ENCAP_DIP_INDEX;
bf_rt_field_id_t smi_id::P_TUNNEL_NEXTHOP_L2_TUNNEL_ENCAP_TUNNEL_INDEX;
bf_rt_field_id_t smi_id::P_TUNNEL_NEXTHOP_L2_TUNNEL_ENCAP_TUNNEL_MAPPER_INDEX;
bf_rt_action_id_t smi_id::A_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP;
bf_rt_field_id_t smi_id::P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_DMAC;
bf_rt_field_id_t smi_id::P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_TYPE;
bf_rt_field_id_t smi_id::P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_DIP_INDEX;
bf_rt_field_id_t smi_id::P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_TUNNEL_INDEX;
bf_rt_action_id_t smi_id::A_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_WITH_VNI;
bf_rt_field_id_t smi_id::P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_WITH_VNI_DMAC;
bf_rt_field_id_t smi_id::P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_WITH_VNI_TYPE;
bf_rt_field_id_t smi_id::P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_WITH_VNI_VNI;
bf_rt_field_id_t smi_id::P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_WITH_VNI_DIP_INDEX;
bf_rt_field_id_t smi_id::P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_WITH_VNI_TUNNEL_INDEX;
bf_rt_action_id_t smi_id::A_TUNNEL_NEXTHOP_SRV6_ENCAP;
bf_rt_field_id_t smi_id::P_SRV6_ENCAP_SEG_LEN;
bf_rt_field_id_t smi_id::P_SRV6_ENCAP_TUNNEL_INDEX;
bf_rt_action_id_t smi_id::A_TUNNEL_NEXTHOP_SRV6_INSERT;
bf_rt_field_id_t smi_id::P_SRV6_INSERT_SEG_LEN;

/* tunnel.p4 */
bf_rt_table_id_t smi_id::T_IPV4_SRC_VTEP;
bf_rt_field_id_t smi_id::F_IPV4_SRC_VTEP_SRC_ADDR;
bf_rt_field_id_t smi_id::F_IPV4_SRC_VTEP_LOCAL_MD_VRF;
bf_rt_field_id_t smi_id::F_IPV4_SRC_VTEP_LOCAL_MD_TUNNEL_TYPE;
bf_rt_action_id_t smi_id::A_IPV4_SRC_VTEP_MISS;
bf_rt_action_id_t smi_id::A_IPV4_SRC_VTEP_HIT;
bf_rt_field_id_t smi_id::P_IPV4_SRC_VTEP_HIT_IFINDEX;

bf_rt_table_id_t smi_id::T_DECAP_ECN;
bf_rt_table_id_t smi_id::F_MODE_ECN;

bf_rt_table_id_t smi_id::T_IPV4_DST_VTEP;
bf_rt_field_id_t smi_id::F_IPV4_DST_VTEP_DST_ADDR;
bf_rt_field_id_t smi_id::F_IPV4_DST_VTEP_SRC_ADDR;
bf_rt_field_id_t smi_id::F_IPV4_DST_VTEP_LOCAL_MD_VRF;
bf_rt_field_id_t smi_id::F_IPV4_DST_VTEP_LOCAL_MD_TUNNEL_TYPE;
bf_rt_field_id_t smi_id::P_IPV4_DST_VTEP_SET_PROPERTIES_ECN_MODE;
bf_rt_action_id_t smi_id::A_IPV4_DST_VTEP_HIT;
bf_rt_field_id_t smi_id::P_IPV4_DST_VTEP_HIT_TTL_MODE;
bf_rt_field_id_t smi_id::P_IPV4_DST_VTEP_HIT_QOS_MODE;
bf_rt_action_id_t smi_id::A_IPV4_DST_VTEP_SET_INNER_BD_PROPERTIES;
bf_rt_field_id_t smi_id::P_IPV4_DST_VTEP_SET_PROPERTIES_BD;
bf_rt_field_id_t smi_id::P_IPV4_DST_VTEP_SET_PROPERTIES_VRF;
bf_rt_field_id_t smi_id::P_IPV4_DST_VTEP_SET_PROPERTIES_VRF_TTL_VIOLATION;
bf_rt_field_id_t smi_id::P_IPV4_DST_VTEP_SET_PROPERTIES_VRF_TTL_VIOLATION_VALID;
bf_rt_field_id_t
    smi_id::P_IPV4_DST_VTEP_SET_PROPERTIES_VRF_IP_OPTIONS_VIOLATION;
bf_rt_field_id_t smi_id::P_IPV4_DST_VTEP_SET_PROPERTIES_BD_LABEL;
// bf_rt_field_id_t smi_id::P_IPV4_DST_VTEP_SET_PROPERTIES_RID;
bf_rt_field_id_t smi_id::P_IPV4_DST_VTEP_SET_PROPERTIES_LEARNING_MODE;
bf_rt_field_id_t smi_id::P_IPV4_DST_VTEP_SET_PROPERTIES_IPV4_UNICAST_ENABLE;
bf_rt_field_id_t smi_id::P_IPV4_DST_VTEP_SET_PROPERTIES_IPV6_UNICAST_ENABLE;
bf_rt_field_id_t smi_id::P_IPV4_DST_VTEP_SET_PROPERTIES_TTL_MODE;
bf_rt_field_id_t smi_id::P_IPV4_DST_VTEP_SET_PROPERTIES_QOS_MODE;

bf_rt_table_id_t smi_id::T_IPV6_SRC_VTEP;
bf_rt_field_id_t smi_id::F_IPV6_SRC_VTEP_SRC_ADDR;
bf_rt_field_id_t smi_id::F_IPV6_SRC_VTEP_LOCAL_MD_VRF;
bf_rt_field_id_t smi_id::F_IPV6_SRC_VTEP_LOCAL_MD_TUNNEL_TYPE;
bf_rt_action_id_t smi_id::A_IPV6_SRC_VTEP_MISS;
bf_rt_action_id_t smi_id::A_IPV6_SRC_VTEP_HIT;
bf_rt_field_id_t smi_id::P_IPV6_SRC_VTEP_HIT_IFINDEX;

bf_rt_table_id_t smi_id::T_IPV6_DST_VTEP;
bf_rt_field_id_t smi_id::F_IPV6_DST_VTEP_SRC_ADDR;
bf_rt_field_id_t smi_id::F_IPV6_DST_VTEP_DST_ADDR;
bf_rt_field_id_t smi_id::F_IPV6_DST_VTEP_LOCAL_MD_VRF;
bf_rt_field_id_t smi_id::F_IPV6_DST_VTEP_LOCAL_MD_TUNNEL_TYPE;
bf_rt_field_id_t smi_id::P_IPV6_DST_VTEP_SET_PROPERTIES_ECN_MODE;
bf_rt_action_id_t smi_id::A_IPV6_DST_VTEP_HIT;
bf_rt_field_id_t smi_id::P_IPV6_DST_VTEP_HIT_TTL_MODE;
bf_rt_field_id_t smi_id::P_IPV6_DST_VTEP_HIT_QOS_MODE;
bf_rt_action_id_t smi_id::A_IPV6_DST_VTEP_SET_INNER_BD_PROPERTIES;
bf_rt_field_id_t smi_id::P_IPV6_DST_VTEP_SET_PROPERTIES_BD;
bf_rt_field_id_t smi_id::P_IPV6_DST_VTEP_SET_PROPERTIES_VRF;
bf_rt_field_id_t smi_id::P_IPV6_DST_VTEP_SET_PROPERTIES_BD_LABEL;
// bf_rt_field_id_t smi_id::P_IPV6_DST_VTEP_SET_PROPERTIES_RID;
bf_rt_field_id_t smi_id::P_IPV6_DST_VTEP_SET_PROPERTIES_LEARNING_MODE;
bf_rt_field_id_t smi_id::P_IPV6_DST_VTEP_SET_PROPERTIES_IPV4_UNICAST_ENABLE;
bf_rt_field_id_t smi_id::P_IPV6_DST_VTEP_SET_PROPERTIES_IPV6_UNICAST_ENABLE;
bf_rt_field_id_t smi_id::P_IPV6_DST_VTEP_SET_PROPERTIES_TTL_MODE;
bf_rt_field_id_t smi_id::P_IPV6_DST_VTEP_SET_PROPERTIES_QOS_MODE;

bf_rt_table_id_t smi_id::T_TUNNEL_RMAC;
bf_rt_field_id_t smi_id::F_TUNNEL_RMAC_LOCAL_MD_TUNNEL_VNI;
bf_rt_field_id_t smi_id::F_TUNNEL_RMAC_HDR_INNER_ETHERNET_DST_ADDR;
bf_rt_action_id_t smi_id::A_TUNNEL_RMAC_MISS;
bf_rt_action_id_t smi_id::A_TUNNEL_RMAC_HIT;

bf_rt_table_id_t smi_id::T_TUNNEL_VXLAN_DEVICE_RMAC;
bf_rt_field_id_t smi_id::F_TUNNEL_VXLAN_DEVICE_RMAC_HDR_INNER_ETHERNET_DST_ADDR;

bf_rt_table_id_t smi_id::T_VNI_TO_BD_MAPPING;
bf_rt_field_id_t smi_id::F_VNI_TO_BD_MAPPING_LOCAL_MD_TUNNEL_VNI;
bf_rt_action_id_t smi_id::A_VNI_TO_BD_MAPPING_SET_INNER_BD_PROPERTIES;
bf_rt_field_id_t smi_id::P_VNI_SET_PROPERTIES_BD;
bf_rt_field_id_t smi_id::P_VNI_SET_PROPERTIES_VRF;
bf_rt_field_id_t smi_id::P_VNI_SET_PROPERTIES_VRF_TTL_VIOLATION;
bf_rt_field_id_t smi_id::P_VNI_SET_PROPERTIES_VRF_TTL_VIOLATION_VALID;
bf_rt_field_id_t smi_id::P_VNI_SET_PROPERTIES_VRF_IP_OPTIONS_VIOLATION;

bf_rt_field_id_t smi_id::P_VNI_SET_PROPERTIES_BD_LABEL;
// bf_rt_field_id_t smi_id::P_VNI_SET_PROPERTIES_RID;
bf_rt_field_id_t smi_id::P_VNI_SET_PROPERTIES_LEARNING_MODE;
bf_rt_field_id_t smi_id::P_VNI_SET_PROPERTIES_IPV4_UNICAST_ENABLE;
bf_rt_field_id_t smi_id::P_VNI_SET_PROPERTIES_IPV6_UNICAST_ENABLE;

bf_rt_table_id_t smi_id::T_TUNNEL_SRC_ADDR_REWRITE;
bf_rt_field_id_t smi_id::F_TUNNEL_SRC_ADDR_REWRITE_TUNNEL_INDEX;
bf_rt_action_id_t smi_id::A_TUNNEL_SRC_ADDR_REWRITE_IPV4_SIP_REWRITE;
bf_rt_field_id_t smi_id::P_TUNNEL_SRC_ADDR_REWRITE_IPV4_SIP_REWRITE_SRC_ADDR;
bf_rt_field_id_t smi_id::P_TUNNEL_SRC_ADDR_REWRITE_IPV4_SIP_REWRITE_TTL_VAL;
bf_rt_field_id_t smi_id::P_TUNNEL_SRC_ADDR_REWRITE_IPV4_SIP_REWRITE_DSCP_VAL;
bf_rt_action_id_t smi_id::A_TUNNEL_SRC_ADDR_REWRITE_IPV6_SIP_REWRITE;
bf_rt_field_id_t smi_id::P_TUNNEL_SRC_ADDR_REWRITE_IPV6_SIP_REWRITE_SRC_ADDR;
bf_rt_field_id_t smi_id::P_TUNNEL_SRC_ADDR_REWRITE_IPV6_SIP_REWRITE_TTL_VAL;
bf_rt_field_id_t smi_id::P_TUNNEL_SRC_ADDR_REWRITE_IPV6_SIP_REWRITE_DSCP_VAL;

bf_rt_table_id_t smi_id::T_TUNNEL_DST_ADDR_REWRITE;
bf_rt_field_id_t smi_id::F_TUNNEL_DST_ADDR_REWRITE_LOCAL_MD_TUNNEL_DIP_INDEX;
bf_rt_action_id_t smi_id::A_TUNNEL_DST_ADDR_REWRITE_IPV4_DIP_REWRITE;
bf_rt_field_id_t smi_id::P_TUNNEL_DST_ADDR_REWRITE_IPV4_DIP_REWRITE_DST_ADDR;
bf_rt_action_id_t smi_id::A_TUNNEL_DST_ADDR_REWRITE_IPV6_DIP_REWRITE;
bf_rt_field_id_t smi_id::P_TUNNEL_DST_ADDR_REWRITE_IPV6_DIP_REWRITE_DST_ADDR;

bf_rt_table_id_t smi_id::T_TUNNEL_REWRITE_ENCAP_TTL;
bf_rt_field_id_t smi_id::F_TUNNEL_REWRITE_ENCAP_TTL_LOCAL_MD_TUNNEL_INDEX;
bf_rt_field_id_t smi_id::F_TUNNEL_REWRITE_ENCAP_TTL_IPV4_VALID;
bf_rt_field_id_t smi_id::F_TUNNEL_REWRITE_ENCAP_TTL_IPV6_VALID;
bf_rt_field_id_t smi_id::F_TUNNEL_REWRITE_ENCAP_TTL_INNER_IPV4_VALID;
bf_rt_field_id_t smi_id::F_TUNNEL_REWRITE_ENCAP_TTL_INNER_IPV6_VALID;
bf_rt_action_id_t smi_id::A_TUNNEL_REWRITE_ENCAP_TTL_V4_IN_V4_PIPE;
bf_rt_field_id_t smi_id::P_TUNNEL_REWRITE_ENCAP_TTL_V4_IN_V4_PIPE_TTL_VAL;
bf_rt_action_id_t smi_id::A_TUNNEL_REWRITE_ENCAP_TTL_V4_IN_V6_PIPE;
bf_rt_field_id_t smi_id::P_TUNNEL_REWRITE_ENCAP_TTL_V4_IN_V6_PIPE_TTL_VAL;
bf_rt_action_id_t smi_id::A_TUNNEL_REWRITE_ENCAP_TTL_V6_IN_V4_PIPE;
bf_rt_field_id_t smi_id::P_TUNNEL_REWRITE_ENCAP_TTL_V6_IN_V4_PIPE_TTL_VAL;
bf_rt_action_id_t smi_id::A_TUNNEL_REWRITE_ENCAP_TTL_V6_IN_V6_PIPE;
bf_rt_field_id_t smi_id::P_TUNNEL_REWRITE_ENCAP_TTL_V6_IN_V6_PIPE_TTL_VAL;
bf_rt_action_id_t smi_id::A_TUNNEL_REWRITE_ENCAP_TTL_V4_IN_V6_UNIFORM;
bf_rt_action_id_t smi_id::A_TUNNEL_REWRITE_ENCAP_TTL_V6_IN_V4_UNIFORM;

bf_rt_table_id_t smi_id::T_TUNNEL_REWRITE_ENCAP_DSCP;
bf_rt_field_id_t smi_id::F_TUNNEL_REWRITE_ENCAP_DSCP_LOCAL_MD_TUNNEL_INDEX;
bf_rt_field_id_t smi_id::F_TUNNEL_REWRITE_ENCAP_DSCP_IPV4_VALID;
bf_rt_field_id_t smi_id::F_TUNNEL_REWRITE_ENCAP_DSCP_IPV6_VALID;
bf_rt_field_id_t smi_id::F_TUNNEL_REWRITE_ENCAP_DSCP_INNER_IPV4_VALID;
bf_rt_field_id_t smi_id::F_TUNNEL_REWRITE_ENCAP_DSCP_INNER_IPV6_VALID;
bf_rt_action_id_t smi_id::A_TUNNEL_REWRITE_ENCAP_DSCP_V4_IN_V4_ECN;
bf_rt_action_id_t smi_id::A_TUNNEL_REWRITE_ENCAP_DSCP_V4_IN_V6_ECN;
bf_rt_action_id_t smi_id::A_TUNNEL_REWRITE_ENCAP_DSCP_V6_IN_V4_ECN;
bf_rt_action_id_t smi_id::A_TUNNEL_REWRITE_ENCAP_DSCP_V6_IN_V6_ECN;
bf_rt_action_id_t smi_id::A_TUNNEL_REWRITE_ENCAP_DSCP_V4_IN_V4_UNIFORM;
bf_rt_action_id_t smi_id::A_TUNNEL_REWRITE_ENCAP_DSCP_V4_IN_V6_UNIFORM;
bf_rt_action_id_t smi_id::A_TUNNEL_REWRITE_ENCAP_DSCP_V6_IN_V4_UNIFORM;
bf_rt_action_id_t smi_id::A_TUNNEL_REWRITE_ENCAP_DSCP_V6_IN_V6_UNIFORM;
bf_rt_action_id_t smi_id::A_TUNNEL_REWRITE_ENCAP_DSCP_V4_PIPE;
bf_rt_action_id_t smi_id::A_TUNNEL_REWRITE_ENCAP_DSCP_V6_PIPE;
bf_rt_field_id_t smi_id::P_TUNNEL_REWRITE_ENCAP_DSCP_V4_PIPE_DSCP_VAL;
bf_rt_field_id_t smi_id::P_TUNNEL_REWRITE_ENCAP_DSCP_V6_PIPE_DSCP_VAL;

bf_rt_table_id_t smi_id::T_EGRESS_VRF_MAPPING;
bf_rt_field_id_t smi_id::F_EGRESS_VRF_MAPPING_LOCAL_MD_VRF;
bf_rt_action_id_t smi_id::A_EGRESS_VRF_MAPPING_SET_VRF_PROPERTIES;
bf_rt_field_id_t smi_id::P_EGRESS_VRF_MAPPING_SET_VRF_PROPERTIES_SMAC;

bf_rt_table_id_t smi_id::T_BD_TO_VNI_MAPPING;
bf_rt_field_id_t smi_id::F_BD_TO_VNI_MAPPING_LOCAL_MD_BD;
bf_rt_field_id_t smi_id::F_BD_TO_VNI_MAPPING_LOCAL_MD_TUNNEL_MAPPER_INDEX;
bf_rt_action_id_t smi_id::A_BD_TO_VNI_MAPPING_SET_VNI;
bf_rt_field_id_t smi_id::P_BD_SET_PROPERTIES_VNI;

bf_rt_table_id_t smi_id::T_VRF_TO_VNI_MAPPING;
bf_rt_field_id_t smi_id::F_VRF_TO_VNI_MAPPING_LOCAL_MD_VRF;
bf_rt_action_id_t smi_id::A_VRF_TO_VNI_MAPPING_SET_VNI;
bf_rt_field_id_t smi_id::P_VRF_SET_PROPERTIES_VNI;

bf_rt_table_id_t smi_id::T_TUNNEL_TABLE;
bf_rt_field_id_t smi_id::F_TUNNEL_TABLE_LOCAL_MD_TUNNEL_TYPE;
bf_rt_action_id_t smi_id::A_TUNNEL_TABLE_ENCAP_IPV4_VXLAN;
bf_rt_field_id_t smi_id::F_TUNNEL_TABLE_ENCAP_IPV4_VXLAN_PORT;
bf_rt_action_id_t smi_id::A_TUNNEL_TABLE_ENCAP_IPV6_VXLAN;
bf_rt_field_id_t smi_id::F_TUNNEL_TABLE_ENCAP_IPV6_VXLAN_PORT;
bf_rt_action_id_t smi_id::A_TUNNEL_TABLE_ENCAP_IPV4_IP;
bf_rt_action_id_t smi_id::A_TUNNEL_TABLE_ENCAP_IPV6_IP;
bf_rt_action_id_t smi_id::A_TUNNEL_TABLE_ENCAP_IPV4_GRE;
bf_rt_action_id_t smi_id::A_TUNNEL_TABLE_ENCAP_IPV6_GRE;

/* qos.p4 */
bf_rt_table_id_t smi_id::T_DSCP_TC_MAP;
bf_rt_field_id_t smi_id::F_DSCP_TC_MAP_QOS_INGRESS_PORT;
bf_rt_field_id_t smi_id::F_DSCP_TC_MAP_LKP_IP_DSCP;
bf_rt_action_id_t smi_id::A_DSCP_TC_MAP_SET_INGRESS_TC;
bf_rt_field_id_t smi_id::F_DSCP_TC_MAP_SET_INGRESS_TC_TC;
bf_rt_action_id_t smi_id::A_DSCP_TC_MAP_SET_INGRESS_COLOR;
bf_rt_field_id_t smi_id::F_DSCP_TC_MAP_SET_INGRESS_COLOR_COLOR;
bf_rt_action_id_t smi_id::A_DSCP_TC_MAP_SET_INGRESS_TC_AND_COLOR;
bf_rt_field_id_t smi_id::F_DSCP_TC_MAP_SET_INGRESS_TC_AND_COLOR_TC;
bf_rt_field_id_t smi_id::F_DSCP_TC_MAP_SET_INGRESS_TC_AND_COLOR_COLOR;
// bf_rt_action_id_t smi_id::A_DSCP_TC_MAP_SET_INGRESS_TC_COLOR_AND_METER;
// bf_rt_field_id_t smi_id::F_DSCP_TC_MAP_SET_INGRESS_TC_COLOR_AND_METER_TC;
// bf_rt_field_id_t smi_id::F_DSCP_TC_MAP_SET_INGRESS_TC_COLOR_AND_METER_COLOR;
// bf_rt_field_id_t smi_id::F_DSCP_TC_MAP_SET_INGRESS_TC_COLOR_AND_METER_METER;

bf_rt_table_id_t smi_id::T_PCP_TC_MAP;
bf_rt_field_id_t smi_id::F_PCP_TC_MAP_QOS_INGRESS_PORT;
bf_rt_field_id_t smi_id::F_PCP_TC_MAP_LKP_PCP;
bf_rt_action_id_t smi_id::A_PCP_TC_MAP_SET_INGRESS_TC;
bf_rt_field_id_t smi_id::F_PCP_TC_MAP_SET_INGRESS_TC_TC;
bf_rt_action_id_t smi_id::A_PCP_TC_MAP_SET_INGRESS_COLOR;
bf_rt_field_id_t smi_id::F_PCP_TC_MAP_SET_INGRESS_COLOR_COLOR;
bf_rt_action_id_t smi_id::A_PCP_TC_MAP_SET_INGRESS_TC_AND_COLOR;
bf_rt_field_id_t smi_id::F_PCP_TC_MAP_SET_INGRESS_TC_AND_COLOR_TC;
bf_rt_field_id_t smi_id::F_PCP_TC_MAP_SET_INGRESS_TC_AND_COLOR_COLOR;
// bf_rt_action_id_t smi_id::A_PCP_TC_MAP_SET_INGRESS_TC_COLOR_AND_METER;
// bf_rt_field_id_t smi_id::F_PCP_TC_MAP_SET_INGRESS_TC_COLOR_AND_METER_TC;
// bf_rt_field_id_t smi_id::F_PCP_TC_MAP_SET_INGRESS_TC_COLOR_AND_METER_COLOR;
// bf_rt_field_id_t smi_id::F_PCP_TC_MAP_SET_INGRESS_TC_COLOR_AND_METER_METER;
//
bf_rt_table_id_t smi_id::T_TRAFFIC_CLASS;
bf_rt_field_id_t smi_id::F_TRAFFIC_CLASS_LOCAL_MD_INGRESS_PORT;
bf_rt_field_id_t smi_id::F_TRAFFIC_CLASS_QOS_MD_COLOR;
bf_rt_field_id_t smi_id::F_TRAFFIC_CLASS_QOS_MD_TC;
bf_rt_action_id_t smi_id::A_TRAFFIC_CLASS_SET_ICOS;
bf_rt_field_id_t smi_id::F_TRAFFIC_CLASS_SET_ICOS_ICOS;
bf_rt_action_id_t smi_id::A_TRAFFIC_CLASS_SET_QID;
bf_rt_field_id_t smi_id::F_TRAFFIC_CLASS_SET_QID_QID;
bf_rt_action_id_t smi_id::A_TRAFFIC_CLASS_SET_ICOS_AND_QUEUE;
bf_rt_field_id_t smi_id::F_TRAFFIC_CLASS_SET_ICOS_AND_QUEUE_ICOS;
bf_rt_field_id_t smi_id::F_TRAFFIC_CLASS_SET_ICOS_AND_QUEUE_QID;

bf_rt_table_id_t smi_id::T_PPG;
bf_rt_field_id_t smi_id::F_PPG_LOCAL_MD_INGRESS_PORT;
bf_rt_field_id_t smi_id::F_PPG_IG_INTR_MD_FOR_TM_INGRESS_COS;
bf_rt_action_id_t smi_id::A_PPG_COUNT;
bf_rt_field_id_t smi_id::P_PPG_STATS_BYTES;
bf_rt_field_id_t smi_id::P_PPG_STATS_PKTS;

bf_rt_table_id_t smi_id::T_QUEUE;
bf_rt_field_id_t smi_id::F_QUEUE_EG_INTR_MD_PORT;
bf_rt_field_id_t smi_id::F_QUEUE_LOCAL_MD_QOS_QID;
bf_rt_action_id_t smi_id::A_QUEUE_COUNT;
bf_rt_field_id_t smi_id::P_QUEUE_STATS_BYTES;
bf_rt_field_id_t smi_id::P_QUEUE_STATS_PKTS;

bf_rt_table_id_t smi_id::T_L3_QOS_MAP;
bf_rt_field_id_t smi_id::F_L3_QOS_MAP_PORT;
bf_rt_field_id_t smi_id::F_L3_QOS_MAP_MD_TC;
bf_rt_field_id_t smi_id::F_L3_QOS_MAP_MD_COLOR;
bf_rt_field_id_t smi_id::F_L3_QOS_MAP_HDR_IPV4_VALID;
bf_rt_field_id_t smi_id::F_L3_QOS_MAP_HDR_IPV6_VALID;
bf_rt_action_id_t smi_id::A_L3_QOS_MAP_SET_IPV4_DSCP;
bf_rt_field_id_t smi_id::F_L3_QOS_MAP_SET_IPV4_DSCP_DSCP;
bf_rt_action_id_t smi_id::A_L3_QOS_MAP_SET_IPV6_DSCP;
bf_rt_field_id_t smi_id::F_L3_QOS_MAP_SET_IPV6_DSCP_DSCP;

bf_rt_table_id_t smi_id::T_L2_QOS_MAP;
bf_rt_field_id_t smi_id::F_L2_QOS_MAP_PORT;
bf_rt_field_id_t smi_id::F_L2_QOS_MAP_MD_TC;
bf_rt_field_id_t smi_id::F_L2_QOS_MAP_MD_COLOR;
bf_rt_action_id_t smi_id::A_L2_QOS_MAP_SET_VLAN_PCP;
bf_rt_field_id_t smi_id::F_L2_QOS_MAP_SET_VLAN_PCP_PCP;

bf_rt_table_id_t smi_id::T_INGRESS_PFC_WD_ACL;
bf_rt_field_id_t smi_id::F_INGRESS_PFC_WD_ACL_QID;
bf_rt_field_id_t smi_id::F_INGRESS_PFC_WD_ACL_PORT;
bf_rt_field_id_t smi_id::A_INGRESS_PFC_WD_ACL_DENY;
bf_rt_field_id_t smi_id::P_INGRESS_PFC_WD_ACL_STATS_COUNTER_SPEC_PKTS;
bf_rt_field_id_t smi_id::P_INGRESS_PFC_WD_ACL_STATS_COUNTER_SPEC_BYTES;

bf_rt_table_id_t smi_id::T_EGRESS_PFC_WD_ACL;
bf_rt_field_id_t smi_id::F_EGRESS_PFC_WD_ACL_QID;
bf_rt_field_id_t smi_id::F_EGRESS_PFC_WD_ACL_PORT;
bf_rt_field_id_t smi_id::A_EGRESS_PFC_WD_ACL_DENY;
bf_rt_field_id_t smi_id::P_EGRESS_PFC_WD_ACL_STATS_COUNTER_SPEC_PKTS;
bf_rt_field_id_t smi_id::P_EGRESS_PFC_WD_ACL_STATS_COUNTER_SPEC_BYTES;

bf_rt_table_id_t smi_id::T_ETRAP_IPV4_ACL;
bf_rt_field_id_t smi_id::F_ETRAP_IPV4_ACL_LOCAL_MD_LKP_IP_SRC_ADDR;
bf_rt_field_id_t smi_id::F_ETRAP_IPV4_ACL_LOCAL_MD_LKP_IP_DST_ADDR;
bf_rt_action_id_t smi_id::A_ETRAP_IPV4_ACL_SET_METER_AND_TC;
bf_rt_field_id_t smi_id::P_ETRAP_IPV4_ACL_SET_METER_AND_TC_INDEX;
bf_rt_field_id_t smi_id::P_ETRAP_IPV4_ACL_SET_METER_AND_TC_TC;

bf_rt_table_id_t smi_id::T_ETRAP_IPV6_ACL;
bf_rt_field_id_t smi_id::F_ETRAP_IPV6_ACL_LOCAL_MD_LKP_IP_SRC_ADDR;
bf_rt_field_id_t smi_id::F_ETRAP_IPV6_ACL_LOCAL_MD_LKP_IP_DST_ADDR;
bf_rt_action_id_t smi_id::A_ETRAP_IPV6_ACL_SET_METER_AND_TC;
bf_rt_field_id_t smi_id::P_ETRAP_IPV6_ACL_SET_METER_AND_TC_INDEX;
bf_rt_field_id_t smi_id::P_ETRAP_IPV6_ACL_SET_METER_AND_TC_TC;

bf_rt_table_id_t smi_id::T_ETRAP_METER;
bf_rt_field_id_t smi_id::F_ETRAP_METER_METER_INDEX;
bf_rt_field_id_t smi_id::D_ETRAP_METER_METER_SPEC_CIR_KBPS;
bf_rt_field_id_t smi_id::D_ETRAP_METER_METER_SPEC_PIR_KBPS;
bf_rt_field_id_t smi_id::D_ETRAP_METER_METER_SPEC_CBS_KBITS;
bf_rt_field_id_t smi_id::D_ETRAP_METER_METER_SPEC_PBS_KBITS;

bf_rt_table_id_t smi_id::T_ETRAP_METER_INDEX;
bf_rt_table_id_t smi_id::F_ETRAP_METER_INDEX_LOCAL_MD_QOS_ETRAP_INDEX;
bf_rt_action_id_t smi_id::A_ETRAP_METER_INDEX_ACTION;
bf_rt_table_id_t smi_id::P_ETRAP_METER_INDEX_ACTION_INDEX;

bf_rt_table_id_t smi_id::T_ETRAP_METER_STATE;
bf_rt_field_id_t smi_id::F_ETRAP_METER_STATE_LOCAL_MD_QOS_ETRAP_INDEX;
bf_rt_action_id_t smi_id::A_ETRAP_METER_STATE_ACTION;

bf_rt_table_id_t smi_id::T_ETRAP_STATE;
bf_rt_field_id_t smi_id::F_ETRAP_STATE_LOCAL_MD_QOS_ETRAP_COLOR;
bf_rt_action_id_t smi_id::A_ETRAP_STATE_ETRAP_RED_STATE;
bf_rt_action_id_t smi_id::A_ETRAP_STATE_ETRAP_GREEN_STATE;

bf_rt_table_id_t smi_id::T_ETRAP_STATE_REG;
bf_rt_field_id_t smi_id::F_ETRAP_STATE_REG_REGISTER_INDEX;
bf_rt_field_id_t smi_id::D_ETRAP_STATE_REG_RESULT_VALUE;

/* meter.p4 */
bf_rt_table_id_t smi_id::T_STORM_CONTROL_STATS;
bf_rt_field_id_t smi_id::F_STORM_CONTROL_STATS_COLOR;
bf_rt_field_id_t smi_id::F_STORM_CONTROL_STATS_PKT_TYPE;
bf_rt_field_id_t smi_id::F_STORM_CONTROL_STATS_LOCAL_MD_INGRESS_PORT;
bf_rt_field_id_t smi_id::F_STORM_CONTROL_STATS_DMAC_MISS;
bf_rt_field_id_t smi_id::F_STORM_CONTROL_STATS_MULTICAST_HIT;
bf_rt_action_id_t smi_id::A_STORM_CONTROL_STATS_COUNT;
bf_rt_action_id_t smi_id::A_STORM_CONTROL_STATS_DROP_AND_COUNT;
bf_rt_field_id_t smi_id::D_STORM_CONTROL_STATS_COUNTER_SPEC_PKTS;
bf_rt_field_id_t smi_id::D_STORM_CONTROL_STATS_COUNTER_SPEC_BYTES;

bf_rt_table_id_t smi_id::T_STORM_CONTROL;
bf_rt_field_id_t smi_id::F_STORM_CONTROL_LOCAL_MD_INGRESS_PORT;
bf_rt_field_id_t smi_id::F_STORM_CONTROL_PKT_TYPE;
bf_rt_field_id_t smi_id::F_STORM_CONTROL_DMAC_MISS;
bf_rt_field_id_t smi_id::F_STORM_CONTROL_MULTICAST_HIT;
bf_rt_action_id_t smi_id::A_STORM_CONTROL_SET_METER;
bf_rt_field_id_t smi_id::P_STORM_CONTROL_SET_METER_INDEX;

bf_rt_table_id_t smi_id::T_STORM_CONTROL_METER;
bf_rt_field_id_t smi_id::F_STORM_CONTROL_METER_METER_INDEX;
bf_rt_field_id_t smi_id::D_STORM_CONTROL_METER_METER_SPEC_CIR_KBPS;
bf_rt_field_id_t smi_id::D_STORM_CONTROL_METER_METER_SPEC_PIR_KBPS;
bf_rt_field_id_t smi_id::D_STORM_CONTROL_METER_METER_SPEC_CBS_KBITS;
bf_rt_field_id_t smi_id::D_STORM_CONTROL_METER_METER_SPEC_PBS_KBITS;

bf_rt_table_id_t smi_id::T_INGRESS_MIRROR_METER_ACTION;
bf_rt_field_id_t smi_id::F_INGRESS_MIRROR_METER_ACTION_COLOR;
bf_rt_field_id_t
    smi_id::F_INGRESS_MIRROR_METER_ACTION_LOCAL_MD_MIRROR_METER_INDEX;
bf_rt_action_id_t smi_id::A_INGRESS_MIRROR_METER_MIRROR_AND_COUNT;
bf_rt_action_id_t smi_id::A_INGRESS_MIRROR_METER_NO_MIRROR_AND_COUNT;
bf_rt_field_id_t smi_id::D_INGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_PKTS;
bf_rt_field_id_t smi_id::D_INGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_BYTES;

bf_rt_table_id_t smi_id::T_INGRESS_MIRROR_METER;
bf_rt_field_id_t smi_id::F_INGRESS_MIRROR_METER_METER_INDEX;
bf_rt_field_id_t smi_id::D_INGRESS_MIRROR_METER_METER_SPEC_CIR_PPS;
bf_rt_field_id_t smi_id::D_INGRESS_MIRROR_METER_METER_SPEC_PIR_PPS;
bf_rt_field_id_t smi_id::D_INGRESS_MIRROR_METER_METER_SPEC_CBS_PKTS;
bf_rt_field_id_t smi_id::D_INGRESS_MIRROR_METER_METER_SPEC_PBS_PKTS;

bf_rt_table_id_t smi_id::T_INGRESS_MIRROR_METER_INDEX;
bf_rt_field_id_t
    smi_id::F_INGRESS_MIRROR_METER_INDEX_LOCAL_MD_MIRROR_METER_INDEX;
bf_rt_action_id_t smi_id::A_SET_INGRESS_MIRROR_METER;
bf_rt_field_id_t smi_id::D_SET_INGRESS_MIRROR_METER_INDEX;

bf_rt_table_id_t smi_id::T_EGRESS_MIRROR_METER_ACTION;
bf_rt_field_id_t smi_id::F_EGRESS_MIRROR_METER_ACTION_COLOR;
bf_rt_field_id_t
    smi_id::F_EGRESS_MIRROR_METER_ACTION_LOCAL_MD_MIRROR_METER_INDEX;
bf_rt_action_id_t smi_id::A_EGRESS_MIRROR_METER_MIRROR_AND_COUNT;
bf_rt_action_id_t smi_id::A_EGRESS_MIRROR_METER_NO_MIRROR_AND_COUNT;
bf_rt_field_id_t smi_id::D_EGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_PKTS;
bf_rt_field_id_t smi_id::D_EGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_BYTES;

bf_rt_table_id_t smi_id::T_EGRESS_MIRROR_METER;
bf_rt_field_id_t smi_id::F_EGRESS_MIRROR_METER_METER_INDEX;
bf_rt_field_id_t smi_id::D_EGRESS_MIRROR_METER_METER_SPEC_CIR_PPS;
bf_rt_field_id_t smi_id::D_EGRESS_MIRROR_METER_METER_SPEC_PIR_PPS;
bf_rt_field_id_t smi_id::D_EGRESS_MIRROR_METER_METER_SPEC_CBS_PKTS;
bf_rt_field_id_t smi_id::D_EGRESS_MIRROR_METER_METER_SPEC_PBS_PKTS;

bf_rt_table_id_t smi_id::T_EGRESS_MIRROR_METER_INDEX;
bf_rt_field_id_t
    smi_id::F_EGRESS_MIRROR_METER_INDEX_LOCAL_MD_MIRROR_METER_INDEX;
bf_rt_action_id_t smi_id::A_SET_EGRESS_MIRROR_METER;
bf_rt_field_id_t smi_id::D_SET_EGRESS_MIRROR_METER_INDEX;

bf_rt_table_id_t smi_id::T_INGRESS_PORT_METER_INDEX;
bf_rt_field_id_t
    smi_id::F_INGRESS_PORT_METER_INDEX_LOCAL_MD_QOS_PORT_METER_INDEX;
bf_rt_action_id_t smi_id::A_SET_INGRESS_PORT_METER;
bf_rt_field_id_t smi_id::D_SET_INGRESS_PORT_METER_INDEX;

bf_rt_table_id_t smi_id::T_INGRESS_PORT_METER_ACTION;
bf_rt_field_id_t smi_id::F_INGRESS_PORT_METER_ACTION_COLOR;
bf_rt_field_id_t
    smi_id::F_INGRESS_PORT_METER_ACTION_LOCAL_MD_QOS_PORT_METER_INDEX;
bf_rt_action_id_t smi_id::A_INGRESS_PORT_METER_COUNT;
bf_rt_action_id_t smi_id::A_INGRESS_PORT_METER_DROP_AND_COUNT;
bf_rt_field_id_t smi_id::D_INGRESS_PORT_METER_ACTION_COUNTER_SPEC_PKTS;
bf_rt_field_id_t smi_id::D_INGRESS_PORT_METER_ACTION_COUNTER_SPEC_BYTES;

bf_rt_table_id_t smi_id::T_INGRESS_PORT_METER;
bf_rt_field_id_t smi_id::F_INGRESS_PORT_METER_METER_INDEX;
bf_rt_field_id_t smi_id::D_INGRESS_PORT_METER_METER_SPEC_CIR_KBPS;
bf_rt_field_id_t smi_id::D_INGRESS_PORT_METER_METER_SPEC_PIR_KBPS;
bf_rt_field_id_t smi_id::D_INGRESS_PORT_METER_METER_SPEC_CBS_KBITS;
bf_rt_field_id_t smi_id::D_INGRESS_PORT_METER_METER_SPEC_PBS_KBITS;

bf_rt_table_id_t smi_id::T_EGRESS_PORT_METER_INDEX;
bf_rt_field_id_t
    smi_id::F_EGRESS_PORT_METER_INDEX_LOCAL_MD_QOS_PORT_METER_INDEX;
bf_rt_action_id_t smi_id::A_SET_EGRESS_PORT_METER;
bf_rt_field_id_t smi_id::D_SET_EGRESS_PORT_METER_INDEX;

bf_rt_table_id_t smi_id::T_EGRESS_PORT_METER_ACTION;
bf_rt_field_id_t smi_id::F_EGRESS_PORT_METER_ACTION_COLOR;
bf_rt_field_id_t
    smi_id::F_EGRESS_PORT_METER_ACTION_LOCAL_MD_QOS_PORT_METER_INDEX;
bf_rt_action_id_t smi_id::A_EGRESS_PORT_METER_COUNT;
bf_rt_action_id_t smi_id::A_EGRESS_PORT_METER_DROP_AND_COUNT;
bf_rt_field_id_t smi_id::D_EGRESS_PORT_METER_ACTION_COUNTER_SPEC_PKTS;
bf_rt_field_id_t smi_id::D_EGRESS_PORT_METER_ACTION_COUNTER_SPEC_BYTES;

bf_rt_table_id_t smi_id::T_EGRESS_PORT_METER;
bf_rt_field_id_t smi_id::F_EGRESS_PORT_METER_METER_INDEX;
bf_rt_field_id_t smi_id::D_EGRESS_PORT_METER_METER_SPEC_CIR_KBPS;
bf_rt_field_id_t smi_id::D_EGRESS_PORT_METER_METER_SPEC_PIR_KBPS;
bf_rt_field_id_t smi_id::D_EGRESS_PORT_METER_METER_SPEC_CBS_KBITS;
bf_rt_field_id_t smi_id::D_EGRESS_PORT_METER_METER_SPEC_PBS_KBITS;

bf_rt_table_id_t smi_id::T_INGRESS_ACL_METER_INDEX;
bf_rt_field_id_t smi_id::F_INGRESS_ACL_METER_INDEX_LOCAL_MD_QOS_ACL_METER_INDEX;
bf_rt_action_id_t smi_id::A_SET_INGRESS_ACL_METER;
bf_rt_field_id_t smi_id::D_SET_INGRESS_ACL_METER_INDEX;

bf_rt_table_id_t smi_id::T_INGRESS_ACL_METER_ACTION;
bf_rt_field_id_t smi_id::F_INGRESS_ACL_METER_ACTION_COLOR;
bf_rt_field_id_t
    smi_id::F_INGRESS_ACL_METER_ACTION_LOCAL_MD_QOS_ACL_METER_INDEX;
bf_rt_action_id_t smi_id::A_INGRESS_ACL_METER_COUNT;
bf_rt_field_id_t smi_id::D_INGRESS_ACL_METER_COUNT_PACKET_ACTION;
bf_rt_field_id_t smi_id::D_INGRESS_ACL_METER_ACTION_COUNTER_SPEC_PKTS;
bf_rt_field_id_t smi_id::D_INGRESS_ACL_METER_ACTION_COUNTER_SPEC_BYTES;

bf_rt_table_id_t smi_id::T_INGRESS_ACL_METER;
bf_rt_field_id_t smi_id::F_INGRESS_ACL_METER_METER_INDEX;
bf_rt_field_id_t smi_id::D_INGRESS_ACL_METER_METER_SPEC_CIR_KBPS;
bf_rt_field_id_t smi_id::D_INGRESS_ACL_METER_METER_SPEC_PIR_KBPS;
bf_rt_field_id_t smi_id::D_INGRESS_ACL_METER_METER_SPEC_CBS_KBITS;
bf_rt_field_id_t smi_id::D_INGRESS_ACL_METER_METER_SPEC_PBS_KBITS;

bf_rt_table_id_t smi_id::T_EGRESS_ACL_METER_INDEX;
bf_rt_field_id_t smi_id::F_EGRESS_ACL_METER_INDEX_LOCAL_MD_QOS_ACL_METER_INDEX;
bf_rt_action_id_t smi_id::A_SET_EGRESS_ACL_METER;
bf_rt_field_id_t smi_id::D_SET_EGRESS_ACL_METER_INDEX;

bf_rt_table_id_t smi_id::T_EGRESS_ACL_METER_ACTION;
bf_rt_field_id_t smi_id::F_EGRESS_ACL_METER_ACTION_COLOR;
bf_rt_field_id_t smi_id::F_EGRESS_ACL_METER_ACTION_LOCAL_MD_QOS_ACL_METER_INDEX;
bf_rt_action_id_t smi_id::A_EGRESS_ACL_METER_COUNT;
bf_rt_field_id_t smi_id::D_EGRESS_ACL_METER_COUNT_PACKET_ACTION;
bf_rt_field_id_t smi_id::D_EGRESS_ACL_METER_ACTION_COUNTER_SPEC_PKTS;
bf_rt_field_id_t smi_id::D_EGRESS_ACL_METER_ACTION_COUNTER_SPEC_BYTES;

bf_rt_table_id_t smi_id::T_EGRESS_ACL_METER;
bf_rt_field_id_t smi_id::F_EGRESS_ACL_METER_METER_INDEX;
bf_rt_field_id_t smi_id::D_EGRESS_ACL_METER_METER_SPEC_CIR_KBPS;
bf_rt_field_id_t smi_id::D_EGRESS_ACL_METER_METER_SPEC_PIR_KBPS;
bf_rt_field_id_t smi_id::D_EGRESS_ACL_METER_METER_SPEC_CBS_KBITS;
bf_rt_field_id_t smi_id::D_EGRESS_ACL_METER_METER_SPEC_PBS_KBITS;

/* wred.p4 */
bf_rt_table_id_t smi_id::T_ECN;
bf_rt_field_id_t smi_id::F_ECN_LOCAL_MD_INGRESS_PORT_LAG_LABEL;
bf_rt_field_id_t smi_id::F_ECN_LKP_IP_TOS;
bf_rt_field_id_t smi_id::F_ECN_LKP_TCP_FLAGS;
bf_rt_action_id_t smi_id::A_ECN_SET_INGRESS_COLOR;
bf_rt_field_id_t smi_id::D_ECN_SET_INGRESS_COLOR_COLOR;

bf_rt_table_id_t smi_id::T_WRED_SESSION;
bf_rt_field_id_t smi_id::F_WRED_SESSION_INDEX;
bf_rt_field_id_t smi_id::D_WRED_SESSION_TIME_CONSTANT;
bf_rt_field_id_t smi_id::D_WRED_SESSION_MIN_THRES;
bf_rt_field_id_t smi_id::D_WRED_SESSION_MAX_THRES;
bf_rt_field_id_t smi_id::D_WRED_SESSION_MAX_PROB;

bf_rt_table_id_t smi_id::T_V4_WRED_ACTION;
bf_rt_field_id_t smi_id::F_V4_WRED_ACTION_INDEX;
bf_rt_field_id_t smi_id::F_V4_WRED_ACTION_HDR_IPV4_ECN;
bf_rt_action_id_t smi_id::A_WRED_ACTION_DROP;
bf_rt_action_id_t smi_id::A_WRED_ACTION_SET_IPV4_ECN;

bf_rt_table_id_t smi_id::T_V6_WRED_ACTION;
bf_rt_field_id_t smi_id::F_V6_WRED_ACTION_INDEX;
bf_rt_field_id_t smi_id::F_V6_WRED_ACTION_HDR_IPV6_ECN;
bf_rt_action_id_t smi_id::A_WRED_ACTION_SET_IPV6_ECN;

bf_rt_table_id_t smi_id::T_WRED_INDEX;
bf_rt_field_id_t smi_id::F_WRED_INDEX_EG_INTR_MD_EGRESS_PORT;
bf_rt_field_id_t smi_id::F_WRED_INDEX_QOS_MD_QID;
bf_rt_field_id_t smi_id::F_WRED_INDEX_QOS_MD_COLOR;
bf_rt_action_id_t smi_id::A_WRED_INDEX_SET_WRED_INDEX;
bf_rt_field_id_t smi_id::D_SET_WRED_INDEX_WRED_INDEX;

bf_rt_table_id_t smi_id::T_EGRESS_WRED_STATS;
bf_rt_field_id_t smi_id::F_EGRESS_WRED_STATS_PORT;
bf_rt_field_id_t smi_id::F_EGRESS_WRED_STATS_QID;
bf_rt_field_id_t smi_id::F_EGRESS_WRED_STATS_COLOR;
bf_rt_field_id_t smi_id::F_EGRESS_WRED_STATS_WRED_DROP;
bf_rt_action_id_t smi_id::A_EGRESS_WRED_STATS_COUNT;
bf_rt_field_id_t smi_id::D_EGRESS_WRED_STATS_COUNTER_SPEC_BYTES;
bf_rt_field_id_t smi_id::D_EGRESS_WRED_STATS_COUNTER_SPEC_PKTS;

/* dtel.p4 */
bf_rt_table_id_t smi_id::T_QUEUE_REPORT_QUOTAS;
bf_rt_field_id_t smi_id::F_QUOTAS_REGISTER_INDEX;
bf_rt_field_id_t smi_id::D_QUOTAS_COUNTER;
bf_rt_field_id_t smi_id::D_QUOTAS_LATENCY;

bf_rt_table_id_t smi_id::T_QUEUE_REPORT_THRESHOLDS;
bf_rt_field_id_t smi_id::F_THRESHOLDS_REGISTER_INDEX;
bf_rt_field_id_t smi_id::D_THRESHOLDS_QDEPTH;
bf_rt_field_id_t smi_id::D_THRESHOLDS_LATENCY;

bf_rt_table_id_t smi_id::T_QUEUE_REPORT_ALERT;
bf_rt_field_id_t smi_id::F_QUEUE_REPORT_ALERT_QID;
bf_rt_field_id_t smi_id::F_QUEUE_REPORT_ALERT_PORT;
bf_rt_action_id_t smi_id::A_SET_QALERT;
bf_rt_field_id_t smi_id::D_SET_QALERT_INDEX;
bf_rt_field_id_t smi_id::D_SET_QALERT_QUOTA;
bf_rt_field_id_t smi_id::D_SET_QALERT_QUANTIZATION_MASK;
bf_rt_action_id_t smi_id::A_SET_QMASK;
bf_rt_field_id_t smi_id::D_SET_QMASK_QUANTIZATION_MASK;

bf_rt_table_id_t smi_id::T_QUEUE_REPORT_CHECK_QUOTA;
bf_rt_field_id_t smi_id::F_CHECK_QUOTA_PKT_SRC;
bf_rt_field_id_t smi_id::F_CHECK_QUOTA_QALERT;
bf_rt_field_id_t smi_id::F_CHECK_QUOTA_QID;
bf_rt_field_id_t smi_id::F_CHECK_QUOTA_PORT;
bf_rt_action_id_t smi_id::A_RESET_QUOTA;
bf_rt_field_id_t smi_id::D_RESET_QUOTA_INDEX;
bf_rt_action_id_t smi_id::A_UPDATE_QUOTA;
bf_rt_field_id_t smi_id::D_UPDATE_QUOTA_INDEX;
bf_rt_action_id_t smi_id::A_CHECK_LATENCY_UPDATE_QUOTA;
bf_rt_field_id_t smi_id::D_CHECK_LATENCY_UPDATE_QUOTA_INDEX;

bf_rt_table_id_t smi_id::T_MOD_CONFIG;
bf_rt_field_id_t smi_id::F_MOD_CONFIG_DROP_REASON;
bf_rt_field_id_t smi_id::F_MOD_CONFIG_DTEL_MD_REPORT_TYPE;
bf_rt_field_id_t smi_id::F_MOD_CONFIG_PRIORITY;
bf_rt_action_id_t smi_id::A_MOD_CONFIG_MIRROR;
bf_rt_action_id_t smi_id::A_MOD_CONFIG_MIRROR_AND_SET_D_BIT;

bf_rt_table_id_t smi_id::T_DOD_CONFIG;
bf_rt_field_id_t smi_id::F_DOD_CONFIG_LOCAL_MD_DTEL_REPORT_TYPE;
bf_rt_field_id_t smi_id::F_DOD_CONFIG_EGRESS_PORT;
bf_rt_field_id_t smi_id::F_DOD_CONFIG_QID;
bf_rt_field_id_t smi_id::F_DOD_CONFIG_LOCAL_MD_MULTICAST_ID;
bf_rt_field_id_t smi_id::F_DOD_CONFIG_LOCAL_MD_CPU_REASON;
bf_rt_field_id_t smi_id::F_DOD_CONFIG_PRIORITY;
bf_rt_action_id_t smi_id::A_DOD_CONFIG_ENABLE_DOD;
bf_rt_action_id_t smi_id::A_DOD_CONFIG_DISABLE_DOD;

bf_rt_table_id_t smi_id::T_DTEL_MIRROR_SESSION;
bf_rt_field_id_t smi_id::F_DTEL_MIRROR_SESSION_HDR_ETHERNET_VALID;
bf_rt_field_id_t smi_id::D_DTEL_MIRROR_SESSION_ACTION_MEMBER_ID;
bf_rt_field_id_t smi_id::D_DTEL_MIRROR_SESSION_SELECTOR_GROUP_ID;

bf_rt_table_id_t smi_id::SG_SESSION_SELECTOR_GROUP;
bf_rt_field_id_t smi_id::F_SESSION_SELECTOR_GROUP_ID;
bf_rt_field_id_t smi_id::P_SESSION_SELECTOR_GROUP_MAX_GROUP_SIZE;
bf_rt_field_id_t smi_id::P_SESSION_SELECTOR_GROUP_ACTION_MEMBER_ID_ARRAY;
bf_rt_field_id_t smi_id::P_SESSION_SELECTOR_GROUP_ACTION_MEMBER_STATUS_ARRAY;

bf_rt_table_id_t smi_id::AP_SESSION_SELECTOR;
bf_rt_field_id_t smi_id::F_SESSION_SELECTOR_ACTION_MEMBER_ID;
bf_rt_action_id_t smi_id::A_SET_MIRROR_SESSION;
bf_rt_field_id_t smi_id::P_SET_MIRROR_SESSION_SESSION_ID;

bf_rt_table_id_t smi_id::T_DTEL_CONFIG;
bf_rt_field_id_t smi_id::F_DTEL_CONFIG_PKT_SRC;
bf_rt_field_id_t smi_id::F_DTEL_CONFIG_REPORT_TYPE;
bf_rt_field_id_t smi_id::F_DTEL_CONFIG_DROP_REPORT_FLAG;
bf_rt_field_id_t smi_id::F_DTEL_CONFIG_FLOW_REPORT_FLAG;
bf_rt_field_id_t smi_id::F_DTEL_CONFIG_QUEUE_REPORT_FLAG;
bf_rt_field_id_t smi_id::F_DTEL_CONFIG_DROP_REASON;
bf_rt_field_id_t smi_id::F_DTEL_CONFIG_MIRROR_TYPE;
bf_rt_field_id_t smi_id::F_DTEL_CONFIG_DROP_HDR_VALID;
bf_rt_field_id_t smi_id::F_DTEL_CONFIG_TCP_FLAGS;
bf_rt_field_id_t smi_id::F_DTEL_CONFIG_IPV4_HDR_VALID;
bf_rt_field_id_t smi_id::F_DTEL_CONFIG_IPV6_HDR_VALID;
bf_rt_field_id_t smi_id::F_DTEL_CONFIG_IPV4_DIFFSERV;
bf_rt_field_id_t smi_id::F_DTEL_CONFIG_IPV6_TRAFFIC_CLASS;
bf_rt_field_id_t smi_id::F_DTEL_CONFIG_IFA_CLONED;
bf_rt_field_id_t smi_id::F_DTEL_CONFIG_PRIORITY;
bf_rt_action_id_t smi_id::A_DTEL_CONFIG_MIRROR_SWITCH_LOCAL;
bf_rt_action_id_t smi_id::A_DTEL_CONFIG_MIRROR_SWITCH_LOCAL_SET_Q;
bf_rt_action_id_t smi_id::A_DTEL_CONFIG_MIRROR_SWITCH_LOCAL_SET_F_AND_DROP;
bf_rt_action_id_t smi_id::A_DTEL_CONFIG_MIRROR_SWITCH_LOCAL_SET_Q_F_AND_DROP;
bf_rt_action_id_t smi_id::A_DTEL_CONFIG_MIRROR_DROP;
bf_rt_action_id_t smi_id::A_DTEL_CONFIG_MIRROR_DROP_SET_Q;
bf_rt_action_id_t smi_id::A_DTEL_CONFIG_MIRROR_CLONE;
bf_rt_action_id_t smi_id::A_DTEL_CONFIG_DROP;
bf_rt_action_id_t smi_id::A_DTEL_CONFIG_UPDATE;
bf_rt_field_id_t smi_id::D_DTEL_CONFIG_UPDATE_SWITCH_ID;
bf_rt_field_id_t smi_id::D_DTEL_CONFIG_UPDATE_HW_ID;
bf_rt_field_id_t smi_id::D_DTEL_CONFIG_UPDATE_NEXT_PROTO;
bf_rt_field_id_t smi_id::D_DTEL_CONFIG_UPDATE_MD_LENGTH;
bf_rt_field_id_t smi_id::D_DTEL_CONFIG_UPDATE_REP_MD_BITS;
bf_rt_field_id_t smi_id::D_DTEL_CONFIG_UPDATE_REPORT_TYPE;
bf_rt_action_id_t smi_id::A_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE;
bf_rt_field_id_t smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_SWITCH_ID;
bf_rt_field_id_t smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_HW_ID;
bf_rt_field_id_t smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_NEXT_PROTO;
bf_rt_field_id_t smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_MD_LENGTH;
bf_rt_field_id_t smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_REP_MD_BITS;
bf_rt_field_id_t smi_id::D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_REPORT_TYPE;
bf_rt_action_id_t smi_id::A_DTEL_CONFIG_UPDATE_SET_ETRAP;
bf_rt_field_id_t smi_id::D_DTEL_CONFIG_UPDATE_SET_ETRAP_SWITCH_ID;
bf_rt_field_id_t smi_id::D_DTEL_CONFIG_UPDATE_SET_ETRAP_HW_ID;
bf_rt_field_id_t smi_id::D_DTEL_CONFIG_UPDATE_SET_ETRAP_NEXT_PROTO;
bf_rt_field_id_t smi_id::D_DTEL_CONFIG_UPDATE_SET_ETRAP_MD_LENGTH;
bf_rt_field_id_t smi_id::D_DTEL_CONFIG_UPDATE_SET_ETRAP_REP_MD_BITS;
bf_rt_field_id_t smi_id::D_DTEL_CONFIG_UPDATE_SET_ETRAP_REPORT_TYPE;
bf_rt_field_id_t smi_id::D_DTEL_CONFIG_UPDATE_SET_ETRAP_STATUS;
bf_rt_action_id_t smi_id::A_DTEL_CONFIG_SET_IPV4_DSCP_ALL;
bf_rt_field_id_t smi_id::D_DTEL_CONFIG_IPV4_DSCP_ALL;
bf_rt_action_id_t smi_id::A_DTEL_CONFIG_SET_IPV6_DSCP_ALL;
bf_rt_field_id_t smi_id::D_DTEL_CONFIG_IPV6_DSCP_ALL;
bf_rt_action_id_t smi_id::A_DTEL_CONFIG_SET_IPV4_DSCP_2;
bf_rt_field_id_t smi_id::D_DTEL_CONFIG_IPV4_DSCP_BIT_2;
bf_rt_action_id_t smi_id::A_DTEL_CONFIG_SET_IPV6_DSCP_2;
bf_rt_field_id_t smi_id::D_DTEL_CONFIG_IPV6_DSCP_BIT_2;
bf_rt_action_id_t smi_id::A_DTEL_CONFIG_SET_IPV4_DSCP_3;
bf_rt_field_id_t smi_id::D_DTEL_CONFIG_IPV4_DSCP_BIT_3;
bf_rt_action_id_t smi_id::A_DTEL_CONFIG_SET_IPV6_DSCP_3;
bf_rt_field_id_t smi_id::D_DTEL_CONFIG_IPV6_DSCP_BIT_3;
bf_rt_action_id_t smi_id::A_DTEL_CONFIG_SET_IPV4_DSCP_4;
bf_rt_field_id_t smi_id::D_DTEL_CONFIG_IPV4_DSCP_BIT_4;
bf_rt_action_id_t smi_id::A_DTEL_CONFIG_SET_IPV6_DSCP_4;
bf_rt_field_id_t smi_id::D_DTEL_CONFIG_IPV6_DSCP_BIT_4;
bf_rt_action_id_t smi_id::A_DTEL_CONFIG_SET_IPV4_DSCP_5;
bf_rt_field_id_t smi_id::D_DTEL_CONFIG_IPV4_DSCP_BIT_5;
bf_rt_action_id_t smi_id::A_DTEL_CONFIG_SET_IPV6_DSCP_5;
bf_rt_field_id_t smi_id::D_DTEL_CONFIG_IPV6_DSCP_BIT_5;
bf_rt_action_id_t smi_id::A_DTEL_CONFIG_SET_IPV4_DSCP_6;
bf_rt_field_id_t smi_id::D_DTEL_CONFIG_IPV4_DSCP_BIT_6;
bf_rt_action_id_t smi_id::A_DTEL_CONFIG_SET_IPV6_DSCP_6;
bf_rt_field_id_t smi_id::D_DTEL_CONFIG_IPV6_DSCP_BIT_6;
bf_rt_action_id_t smi_id::A_DTEL_CONFIG_SET_IPV4_DSCP_7;
bf_rt_field_id_t smi_id::D_DTEL_CONFIG_IPV4_DSCP_BIT_7;
bf_rt_action_id_t smi_id::A_DTEL_CONFIG_SET_IPV6_DSCP_7;
bf_rt_field_id_t smi_id::D_DTEL_CONFIG_IPV6_DSCP_BIT_7;

bf_rt_table_id_t smi_id::T_INGRESS_PORT_CONVERSION;
bf_rt_field_id_t smi_id::F_INGRESS_PORT_CONVERSION_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_PORT_CONVERSION_REPORT_VALID;
bf_rt_action_id_t smi_id::A_CONVERT_INGRESS_PORT;
bf_rt_field_id_t smi_id::D_CONVERT_INGRESS_PORT_PORT;

bf_rt_table_id_t smi_id::T_EGRESS_PORT_CONVERSION;
bf_rt_field_id_t smi_id::F_EGRESS_PORT_CONVERSION_PORT;
bf_rt_field_id_t smi_id::F_EGRESS_PORT_CONVERSION_REPORT_VALID;
bf_rt_action_id_t smi_id::A_CONVERT_EGRESS_PORT;
bf_rt_field_id_t smi_id::D_CONVERT_EGRESS_PORT_PORT;

bf_rt_table_id_t smi_id::T_EGRESS_DROP_REPORT_BLOOM_FILTER_1;
bf_rt_field_id_t smi_id::F_EGRESS_DROP_REPORT_BLOOM_FILTER_1_REGISTER_INDEX;
bf_rt_field_id_t smi_id::D_EGRESS_DROP_REPORT_BLOOM_FILTER_1_RESULT_VALUE;

bf_rt_table_id_t smi_id::T_EGRESS_DROP_REPORT_BLOOM_FILTER_2;
bf_rt_field_id_t smi_id::F_EGRESS_DROP_REPORT_BLOOM_FILTER_2_REGISTER_INDEX;
bf_rt_field_id_t smi_id::D_EGRESS_DROP_REPORT_BLOOM_FILTER_2_RESULT_VALUE;

bf_rt_table_id_t smi_id::T_EGRESS_FLOW_REPORT_BLOOM_FILTER_1;
bf_rt_field_id_t smi_id::F_EGRESS_FLOW_REPORT_BLOOM_FILTER_1_REGISTER_INDEX;
bf_rt_field_id_t smi_id::D_EGRESS_FLOW_REPORT_BLOOM_FILTER_1_RESULT_VALUE;

bf_rt_table_id_t smi_id::T_EGRESS_FLOW_REPORT_BLOOM_FILTER_2;
bf_rt_field_id_t smi_id::F_EGRESS_FLOW_REPORT_BLOOM_FILTER_2_REGISTER_INDEX;
bf_rt_field_id_t smi_id::D_EGRESS_FLOW_REPORT_BLOOM_FILTER_2_RESULT_VALUE;

bf_rt_table_id_t smi_id::T_INT_EDGE_PORT_LOOKUP;
bf_rt_field_id_t smi_id::F_INT_EDGE_PORT_LOOKUP_PORT;
bf_rt_action_id_t smi_id::A_INT_EDGE_SET_IFA_EDGE;
bf_rt_action_id_t smi_id::A_INT_EDGE_SET_CLONE_MIRROR_SESSION_ID;
bf_rt_field_id_t smi_id::D_INT_EDGE_CLONE_MIRROR_SESSION_ID;

bf_rt_table_id_t smi_id::T_INGRESS_IP_DTEL_ACL_SAMPLE_SESSION;
bf_rt_field_id_t smi_id::F_INGRESS_IP_DTEL_ACL_SAMPLE_SESSION_REGISTER_INDEX;
bf_rt_field_id_t smi_id::D_INGRESS_IP_DTEL_ACL_SAMPLE_SESSION_REG_CURRENT;
bf_rt_field_id_t smi_id::D_INGRESS_IP_DTEL_ACL_SAMPLE_SESSION_REG_RATE;

bf_rt_table_id_t smi_id::T_INGRESS_SFLOW_SESSION;
bf_rt_field_id_t smi_id::F_INGRESS_SFLOW_SESSION_REGISTER_INDEX;
bf_rt_field_id_t smi_id::D_INGRESS_SFLOW_SESSION_REG_CURRENT;
bf_rt_field_id_t smi_id::D_INGRESS_SFLOW_SESSION_REG_RATE;

// NAT
bf_rt_table_id_t smi_id::T_INGRESS_NAT_DNAPT_INDEX;
bf_rt_field_id_t smi_id::F_INGRESS_NAT_DNAPT_INDEX_PROTOCOL;
bf_rt_field_id_t smi_id::F_INGRESS_NAT_DNAPT_INDEX_DIP;
bf_rt_field_id_t smi_id::F_INGRESS_NAT_DNAPT_INDEX_DPORT;
bf_rt_table_id_t smi_id::A_INGRESS_NAT_DNAPT_SET_INDEX;
bf_rt_table_id_t smi_id::P_INGRESS_NAT_DNAPT_SET_INDEX;
bf_rt_field_id_t smi_id::D_INGRESS_NAT_DNAPT_INDEX_TTL;

bf_rt_table_id_t smi_id::T_INGRESS_NAT_DEST_NAPT;
bf_rt_field_id_t smi_id::F_INGRESS_NAT_DEST_NAPT_PROTOCOL;
bf_rt_field_id_t smi_id::F_INGRESS_NAT_DEST_NAPT_DIP;
bf_rt_field_id_t smi_id::F_INGRESS_NAT_DEST_NAPT_DPORT;
bf_rt_table_id_t smi_id::F_INGRESS_NAT_DNAPT_INDEX;
bf_rt_action_id_t smi_id::A_INGRESS_NAT_DEST_NAPT_REWRITE;
bf_rt_action_id_t smi_id::A_INGRESS_NAT_DEST_NAPT_MISS;
bf_rt_field_id_t smi_id::P_DEST_NAPT_REWRITE_DIP;
bf_rt_field_id_t smi_id::P_DEST_NAPT_REWRITE_DPORT;
bf_rt_field_id_t smi_id::D_INGRESS_NAT_DEST_NAPT_COUNTER_SPEC_PKTS;
bf_rt_field_id_t smi_id::D_INGRESS_NAT_DEST_NAPT_COUNTER_SPEC_BYTES;
bf_rt_field_id_t smi_id::D_INGRESS_NAT_DEST_NAPT_TTL;

bf_rt_table_id_t smi_id::T_INGRESS_NAT_DEST_NAT;
bf_rt_field_id_t smi_id::F_INGRESS_NAT_DEST_NAT_DIP;
bf_rt_action_id_t smi_id::A_INGRESS_NAT_DEST_NAT_REWRITE;
bf_rt_action_id_t smi_id::A_INGRESS_NAT_DEST_NAT_MISS;
bf_rt_field_id_t smi_id::P_DEST_NAT_REWRITE_DIP;
bf_rt_field_id_t smi_id::D_INGRESS_NAT_DEST_NAT_COUNTER_SPEC_PKTS;
bf_rt_field_id_t smi_id::D_INGRESS_NAT_DEST_NAT_COUNTER_SPEC_BYTES;
bf_rt_field_id_t smi_id::D_INGRESS_NAT_DEST_NAT_TTL;

bf_rt_table_id_t smi_id::T_INGRESS_NAT_DEST_NAT_POOL;
bf_rt_field_id_t smi_id::F_INGRESS_NAT_DEST_NAT_POOL_DIP;
bf_rt_action_id_t smi_id::A_INGRESS_NAT_DEST_NAT_POOL_HIT;
bf_rt_action_id_t smi_id::A_INGRESS_NAT_DEST_NAT_POOL_MISS;
bf_rt_field_id_t smi_id::D_INGRESS_NAT_DEST_NAT_POOL_COUNTER_SPEC_PKTS;
bf_rt_field_id_t smi_id::D_INGRESS_NAT_DEST_NAT_POOL_COUNTER_SPEC_BYTES;

bf_rt_table_id_t smi_id::T_INGRESS_NAT_FLOW_NAPT;
bf_rt_field_id_t smi_id::F_INGRESS_NAT_FLOW_NAPT_PROTOCOL;
bf_rt_field_id_t smi_id::F_INGRESS_NAT_FLOW_NAPT_DIP;
bf_rt_field_id_t smi_id::F_INGRESS_NAT_FLOW_NAPT_DPORT;
bf_rt_field_id_t smi_id::F_INGRESS_NAT_FLOW_NAPT_SIP;
bf_rt_field_id_t smi_id::F_INGRESS_NAT_FLOW_NAPT_SPORT;
bf_rt_action_id_t smi_id::A_INGRESS_NAT_FLOW_NAPT_REWRITE;
bf_rt_action_id_t smi_id::A_INGRESS_NAT_FLOW_NAPT_MISS;
bf_rt_field_id_t smi_id::P_FLOW_NAPT_REWRITE_DIP;
bf_rt_field_id_t smi_id::P_FLOW_NAPT_REWRITE_SIP;
bf_rt_field_id_t smi_id::P_FLOW_NAPT_REWRITE_DPORT;
bf_rt_field_id_t smi_id::P_FLOW_NAPT_REWRITE_SPORT;
bf_rt_field_id_t smi_id::D_INGRESS_NAT_FLOW_NAPT_COUNTER_SPEC_PKTS;
bf_rt_field_id_t smi_id::D_INGRESS_NAT_FLOW_NAPT_COUNTER_SPEC_BYTES;

bf_rt_table_id_t smi_id::T_INGRESS_NAT_FLOW_NAT;
bf_rt_field_id_t smi_id::F_INGRESS_NAT_FLOW_NAT_DIP;
bf_rt_field_id_t smi_id::F_INGRESS_NAT_FLOW_NAT_SIP;
bf_rt_action_id_t smi_id::A_INGRESS_NAT_FLOW_NAT_REWRITE;
bf_rt_action_id_t smi_id::A_INGRESS_NAT_FLOW_NAT_MISS;
bf_rt_field_id_t smi_id::P_FLOW_NAT_REWRITE_DIP;
bf_rt_field_id_t smi_id::P_FLOW_NAT_REWRITE_SIP;
bf_rt_field_id_t smi_id::D_INGRESS_NAT_FLOW_NAT_COUNTER_SPEC_PKTS;
bf_rt_field_id_t smi_id::D_INGRESS_NAT_FLOW_NAT_COUNTER_SPEC_BYTES;

bf_rt_table_id_t smi_id::T_INGRESS_NAT_REWRITE;
bf_rt_field_id_t smi_id::F_INGRESS_NAT_REWRITE_NAT_HIT;
bf_rt_field_id_t smi_id::F_INGRESS_NAT_REWRITE_IP_PROTO;
bf_rt_field_id_t smi_id::F_INGRESS_NAT_REWRITE_NAT_SAME_ZONE_CHECK;
bf_rt_action_id_t smi_id::A_INGRESS_NAT_REWRITE_TCP_FLOW;
bf_rt_action_id_t smi_id::A_INGRESS_NAT_REWRITE_UDP_FLOW;
bf_rt_action_id_t smi_id::A_INGRESS_NAT_REWRITE_IPSA_IPDA;
bf_rt_action_id_t smi_id::A_INGRESS_NAT_REWRITE_TCP_DPORT_IPDA;
bf_rt_action_id_t smi_id::A_INGRESS_NAT_REWRITE_UDP_DPORT_IPDA;
bf_rt_action_id_t smi_id::A_INGRESS_NAT_REWRITE_IPDA;
bf_rt_action_id_t smi_id::A_INGRESS_NAT_REWRITE_TCP_SPORT_IPSA;
bf_rt_action_id_t smi_id::A_INGRESS_NAT_REWRITE_UDP_SPORT_IPSA;
bf_rt_action_id_t smi_id::A_INGRESS_NAT_REWRITE_IPSA;

bf_rt_table_id_t smi_id::T_INGRESS_NAT_SNAT;
bf_rt_field_id_t smi_id::F_INGRESS_NAT_SNAT_IPSA;
bf_rt_action_id_t smi_id::A_INGRESS_NAT_SNAT_REWRITE;
bf_rt_action_id_t smi_id::A_INGRESS_NAT_SNAT_MISS;
bf_rt_field_id_t smi_id::P_SOURCE_NAT_REWRITE_SIP;
bf_rt_field_id_t smi_id::D_INGRESS_NAT_SNAT_COUNTER_SPEC_PKTS;
bf_rt_field_id_t smi_id::D_INGRESS_NAT_SNAT_COUNTER_SPEC_BYTES;
bf_rt_field_id_t smi_id::D_INGRESS_NAT_SNAT_TTL;

bf_rt_table_id_t smi_id::T_INGRESS_NAT_SNAPT_INDEX;
bf_rt_field_id_t smi_id::F_INGRESS_NAT_SNAPT_INDEX_IPSA;
bf_rt_field_id_t smi_id::F_INGRESS_NAT_SNAPT_INDEX_IP_PROTO;
bf_rt_field_id_t smi_id::F_INGRESS_NAT_SNAPT_INDEX_IP_L4_SRC_PORT;
bf_rt_table_id_t smi_id::A_INGRESS_NAT_SNAPT_SET_INDEX;
bf_rt_table_id_t smi_id::P_INGRESS_NAT_SNAPT_SET_INDEX;
bf_rt_field_id_t smi_id::D_INGRESS_NAT_SNAPT_INDEX_TTL;

bf_rt_table_id_t smi_id::T_INGRESS_NAT_SNAPT;
bf_rt_field_id_t smi_id::F_INGRESS_NAT_SNAPT_IPSA;
bf_rt_field_id_t smi_id::F_INGRESS_NAT_SNAPT_IP_PROTO;
bf_rt_field_id_t smi_id::F_INGRESS_NAT_SNAPT_IP_L4_SRC_PORT;
bf_rt_table_id_t smi_id::F_INGRESS_NAT_SNAPT_INDEX;
bf_rt_action_id_t smi_id::A_INGRESS_NAT_SNAPT_REWRITE;
bf_rt_action_id_t smi_id::A_INGRESS_NAT_SNAPT_MISS;
bf_rt_field_id_t smi_id::P_SOURCE_NAPT_REWRITE_SIP;
bf_rt_field_id_t smi_id::P_SOURCE_NAPT_REWRITE_SPORT;
bf_rt_field_id_t smi_id::D_INGRESS_NAT_SNAPT_COUNTER_SPEC_PKTS;
bf_rt_field_id_t smi_id::D_INGRESS_NAT_SNAPT_COUNTER_SPEC_BYTES;
bf_rt_field_id_t smi_id::D_INGRESS_NAT_SNAPT_TTL;

// SRv6
bf_rt_table_id_t smi_id::T_MY_SID;
bf_rt_field_id_t smi_id::F_MY_SID_IPV6_DST_ADDR;
bf_rt_field_id_t smi_id::F_MY_SID_VRF;
bf_rt_field_id_t smi_id::F_MY_SID_SRH_HDR_VALID;
bf_rt_field_id_t smi_id::F_MY_SID_SRH_SEG_LEFT;
bf_rt_field_id_t smi_id::F_MY_SID_MATCH_PRIORITY;
bf_rt_action_id_t smi_id::A_ENDPOINT_ACTION_END;
bf_rt_action_id_t smi_id::A_ENDPOINT_ACTION_END_WITH_PSP;
bf_rt_action_id_t smi_id::A_ENDPOINT_ACTION_END_WITH_USD;
bf_rt_action_id_t smi_id::A_ENDPOINT_ACTION_END_UN;
bf_rt_action_id_t smi_id::A_ENDPOINT_ACTION_END_X;
bf_rt_field_id_t smi_id::D_ENDPOINT_ACTION_END_X_NEXTHOP;
bf_rt_action_id_t smi_id::A_ENDPOINT_ACTION_END_X_WITH_PSP;
bf_rt_field_id_t smi_id::D_ENDPOINT_ACTION_END_X_WITH_PSP_NEXTHOP;
bf_rt_action_id_t smi_id::A_ENDPOINT_ACTION_END_X_WITH_USD;
bf_rt_field_id_t smi_id::D_ENDPOINT_ACTION_END_X_WITH_USD_NEXTHOP;
bf_rt_action_id_t smi_id::A_ENDPOINT_ACTION_END_UA;
bf_rt_field_id_t smi_id::D_ENDPOINT_ACTION_END_UA_NEXTHOP;
bf_rt_action_id_t smi_id::A_ENDPOINT_ACTION_END_T;
bf_rt_field_id_t smi_id::D_ENDPOINT_ACTION_END_T_VRF;
bf_rt_action_id_t smi_id::A_ENDPOINT_ACTION_END_DT4;
bf_rt_field_id_t smi_id::D_ENDPOINT_ACTION_END_DT4_VRF;
bf_rt_action_id_t smi_id::A_ENDPOINT_ACTION_END_DT6;
bf_rt_field_id_t smi_id::D_ENDPOINT_ACTION_END_DT6_VRF;
bf_rt_action_id_t smi_id::A_ENDPOINT_ACTION_END_DT46;
bf_rt_field_id_t smi_id::D_ENDPOINT_ACTION_END_DT46_VRF;
bf_rt_action_id_t smi_id::A_ENDPOINT_ACTION_END_DX4;
bf_rt_field_id_t smi_id::D_ENDPOINT_ACTION_END_DX4_NEXTHOP;
bf_rt_action_id_t smi_id::A_ENDPOINT_ACTION_END_DX6;
bf_rt_field_id_t smi_id::D_ENDPOINT_ACTION_END_DX6_NEXTHOP;
bf_rt_action_id_t smi_id::A_ENDPOINT_ACTION_END_B6_ENCAPS_RED;
bf_rt_field_id_t smi_id::D_ENDPOINT_ACTION_END_B6_ENCAPS_RED_NEXTHOP;
bf_rt_action_id_t smi_id::A_ENDPOINT_ACTION_END_B6_INSERT_RED;
bf_rt_field_id_t smi_id::D_ENDPOINT_ACTION_END_B6_INSERT_RED_NEXTHOP;
bf_rt_action_id_t smi_id::A_ENDPOINT_ACTION_TRAP;
bf_rt_action_id_t smi_id::A_ENDPOINT_ACTION_DROP;
bf_rt_field_id_t smi_id::D_MY_SID_COUNTER_SPEC_PKTS;
bf_rt_field_id_t smi_id::D_MY_SID_COUNTER_SPEC_BYTES;

bf_rt_table_id_t smi_id::T_SID_REWRITE;
bf_rt_field_id_t smi_id::F_SID_REWRITE_LOCAL_MD_TUNNEL_NEXTHOP;
bf_rt_action_id_t smi_id::A_SRV6_ENCAPS_SID_REWRITE_0;
bf_rt_field_id_t smi_id::D_SRV6_ENCAPS_SID_REWRITE_0_S0;
bf_rt_action_id_t smi_id::A_SRV6_ENCAPS_SID_REWRITE_1;
bf_rt_field_id_t smi_id::D_SRV6_ENCAPS_SID_REWRITE_1_S0;
bf_rt_field_id_t smi_id::D_SRV6_ENCAPS_SID_REWRITE_1_S1;
bf_rt_action_id_t smi_id::A_SRV6_ENCAPS_SID_REWRITE_2;
bf_rt_field_id_t smi_id::D_SRV6_ENCAPS_SID_REWRITE_2_S0;
bf_rt_field_id_t smi_id::D_SRV6_ENCAPS_SID_REWRITE_2_S1;
bf_rt_field_id_t smi_id::D_SRV6_ENCAPS_SID_REWRITE_2_S2;
bf_rt_action_id_t smi_id::A_SRV6_INSERT_SID_REWRITE_0;
bf_rt_field_id_t smi_id::D_SRV6_INSERT_SID_REWRITE_0_S0;
bf_rt_action_id_t smi_id::A_SRV6_INSERT_SID_REWRITE_1;
bf_rt_field_id_t smi_id::D_SRV6_INSERT_SID_REWRITE_1_S0;
bf_rt_field_id_t smi_id::D_SRV6_INSERT_SID_REWRITE_1_S1;
bf_rt_field_id_t smi_id::D_SRV6_SID_REWRITE_COUNTER_SPEC_PKTS;
bf_rt_field_id_t smi_id::D_SRV6_SID_REWRITE_COUNTER_SPEC_BYTES;

bf_rt_table_id_t smi_id::T_MPLS_LABEL;
bf_rt_field_id_t smi_id::F_MPLS_LABEL_LOCAL_MD_TUNNEL_NEXTHOP;
bf_rt_action_id_t smi_id::A_MPLS_PUSH_1_LABEL;
bf_rt_field_id_t smi_id::P_MPLS_PUSH_1_LABEL0;
bf_rt_action_id_t smi_id::A_MPLS_PUSH_2_LABEL;
bf_rt_field_id_t smi_id::P_MPLS_PUSH_2_LABEL0;
bf_rt_field_id_t smi_id::P_MPLS_PUSH_2_LABEL1;
bf_rt_action_id_t smi_id::A_MPLS_PUSH_3_LABEL;
bf_rt_field_id_t smi_id::P_MPLS_PUSH_3_LABEL0;
bf_rt_field_id_t smi_id::P_MPLS_PUSH_3_LABEL1;
bf_rt_field_id_t smi_id::P_MPLS_PUSH_3_LABEL2;
bf_rt_action_id_t smi_id::A_MPLS_SWAP_LABEL;
bf_rt_field_id_t smi_id::P_MPLS_SWAP_LABEL0;

bf_rt_table_id_t smi_id::T_MPLS_ENCAP;
bf_rt_field_id_t smi_id::F_MPLS_PUSH_COUNT;
bf_rt_action_id_t smi_id::A_MPLS_ENCAP_NOACTION;
bf_rt_action_id_t smi_id::A_MPLS_ENCAP_1;
bf_rt_action_id_t smi_id::A_MPLS_ENCAP_2;
bf_rt_action_id_t smi_id::A_MPLS_ENCAP_3;

bf_rt_table_id_t smi_id::T_MPLS_TTL_REWRITE;
bf_rt_field_id_t smi_id::F_MPLS_TTL_REWRITE_PUSH_COUNT;
bf_rt_field_id_t smi_id::F_MPLS_TTL_REWRITE_SWAP;
bf_rt_action_id_t smi_id::A_MPLS_REWRITE_TTL_1_PIPE;
bf_rt_action_id_t smi_id::A_MPLS_REWRITE_TTL_2_PIPE;
bf_rt_action_id_t smi_id::A_MPLS_REWRITE_TTL_3_PIPE;
bf_rt_action_id_t smi_id::A_MPLS_REWRITE_TTL_DECREMENT;

bf_rt_table_id_t smi_id::T_MPLS_EXP_REWRITE;
bf_rt_field_id_t smi_id::F_MPLS_EXP_REWRITE_PUSH_COUNT;
bf_rt_action_id_t smi_id::A_MPLS_EXP_REWRITE_LABEL1;
bf_rt_action_id_t smi_id::A_MPLS_EXP_REWRITE_LABEL2;
bf_rt_action_id_t smi_id::A_MPLS_EXP_REWRITE_LABEL3;

bf_rt_action_id_t smi_id::A_TUNNEL_NEXTHOP_MPLS_PUSH;
bf_rt_field_id_t smi_id::P_MPLS_PUSH_LABEL_COUNT;
bf_rt_field_id_t smi_id::P_MPLS_PUSH_ENCAP_TTL_MODE;
bf_rt_field_id_t smi_id::P_MPLS_PUSH_ENCAP_TTL;
bf_rt_field_id_t smi_id::P_MPLS_PUSH_ENCAP_QOS_MODE;
bf_rt_field_id_t smi_id::P_MPLS_PUSH_ENCAP_EXP;
bf_rt_field_id_t smi_id::P_MPLS_PUSH_SWAP;

bf_rt_table_id_t smi_id::T_MPLS_FIB;
bf_rt_field_id_t smi_id::F_MPLS_FIB_LOOKUP_LABEL;
bf_rt_action_id_t smi_id::A_MPLS_FIB_MPLS_TERM;
bf_rt_action_id_t smi_id::A_MPLS_FIB_MPLS_SWAP;
bf_rt_action_id_t smi_id::A_MPLS_FIB_MPLS_PHP;
bf_rt_action_id_t smi_id::A_MPLS_FIB_MPLS_DROP;
bf_rt_action_id_t smi_id::A_MPLS_FIB_MPLS_TRAP;
bf_rt_field_id_t smi_id::P_MPLS_FIB_MPLS_PHP_NEXTHOP;
bf_rt_field_id_t smi_id::P_MPLS_FIB_MPLS_PHP_POP_COUNT;
bf_rt_field_id_t smi_id::P_MPLS_FIB_MPLS_PHP_TTL_MODE;
bf_rt_field_id_t smi_id::P_MPLS_FIB_MPLS_PHP_QOS_MODE;
bf_rt_field_id_t smi_id::P_MPLS_FIB_MPLS_SWAP_NEXTHOP;
bf_rt_field_id_t smi_id::P_MPLS_FIB_MPLS_SWAP_POP_COUNT;
bf_rt_field_id_t smi_id::P_MPLS_FIB_MPLS_TERM_BD;
bf_rt_field_id_t smi_id::P_MPLS_FIB_MPLS_TERM_VRF;
bf_rt_field_id_t smi_id::P_MPLS_FIB_MPLS_TERM_VRF_TTL_VIOLATION;
bf_rt_field_id_t smi_id::P_MPLS_FIB_MPLS_TERM_VRF_TTL_VIOLATION_VALID;
bf_rt_field_id_t smi_id::P_MPLS_FIB_MPLS_TERM_VRF_IP_OPTIONS_VIOLATION;
bf_rt_field_id_t smi_id::P_MPLS_FIB_MPLS_TERM_BD_LABEL;
// bf_rt_field_id_t smi_id::P_MPLS_FIB_MPLS_TERM_RID;
bf_rt_field_id_t smi_id::P_MPLS_FIB_MPLS_TERM_LEARN_MODE;
bf_rt_field_id_t smi_id::P_MPLS_FIB_MPLS_TERM_V4_UNICAST_EN;
bf_rt_field_id_t smi_id::P_MPLS_FIB_MPLS_TERM_V6_UNICAST_EN;
bf_rt_field_id_t smi_id::P_MPLS_FIB_MPLS_TERM_POP_COUNT;
bf_rt_field_id_t smi_id::P_MPLS_FIB_MPLS_TERM_TTL_MODE;
bf_rt_field_id_t smi_id::P_MPLS_FIB_MPLS_TERM_QOS_MODE;
bf_rt_field_id_t smi_id::D_MPLS_FIB_MPLS_COUNTER_SPEC_PKTS;
bf_rt_field_id_t smi_id::D_MPLS_FIB_MPLS_COUNTER_SPEC_BYTES;

bf_rt_table_id_t smi_id::T_MPLS_POP;
bf_rt_action_id_t smi_id::A_MPLS_POP_POP1;
bf_rt_action_id_t smi_id::A_MPLS_POP_POP2;
bf_rt_action_id_t smi_id::A_MPLS_POP_POP3;
bf_rt_field_id_t smi_id::F_MPLS_POP_POP_COUNT;

bf_rt_table_id_t smi_id::T_SFC_FILTER_EPOCH_REG;
bf_rt_field_id_t smi_id::P_SFC_FILTER_EPOCH_REG_DURATION;

bf_rt_table_id_t smi_id::T_INGRESS_PORT_IP_STATS;
bf_rt_field_id_t smi_id::F_INGRESS_PORT_IP_STATS_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_PORT_IP_STATS_HDR_IPV4_VALID;
bf_rt_field_id_t smi_id::F_INGRESS_PORT_IP_STATS_HDR_IPV6_VALID;
bf_rt_field_id_t smi_id::F_INGRESS_PORT_IP_STATS_DROP;
bf_rt_field_id_t smi_id::F_INGRESS_PORT_IP_STATS_COPY_TO_CPU;
bf_rt_field_id_t smi_id::F_INGRESS_PORT_IP_STATS_HDR_ETHERNET_DST_ADDR;
bf_rt_field_id_t smi_id::F_INGRESS_PORT_IP_STATS_PRIORITY;
bf_rt_field_id_t smi_id::D_INGRESS_PORT_IP_STATS_COUNTER_SPEC_PKTS;
bf_rt_field_id_t smi_id::D_INGRESS_PORT_IP_STATS_COUNTER_SPEC_BYTES;
bf_rt_action_id_t smi_id::A_INGRESS_PORT_IP_STATS_COUNT;

bf_rt_table_id_t smi_id::T_EGRESS_PORT_IP_STATS;
bf_rt_field_id_t smi_id::F_EGRESS_PORT_IP_STATS_PORT;
bf_rt_field_id_t smi_id::F_EGRESS_PORT_IP_STATS_HDR_IPV4_VALID;
bf_rt_field_id_t smi_id::F_EGRESS_PORT_IP_STATS_HDR_IPV6_VALID;
bf_rt_field_id_t smi_id::F_EGRESS_PORT_IP_STATS_DROP;
bf_rt_field_id_t smi_id::F_EGRESS_PORT_IP_STATS_COPY_TO_CPU;
bf_rt_field_id_t smi_id::F_EGRESS_PORT_IP_STATS_HDR_ETHERNET_DST_ADDR;
bf_rt_field_id_t smi_id::F_EGRESS_PORT_IP_STATS_PRIORITY;
bf_rt_field_id_t smi_id::D_EGRESS_PORT_IP_STATS_COUNTER_SPEC_PKTS;
bf_rt_field_id_t smi_id::D_EGRESS_PORT_IP_STATS_COUNTER_SPEC_BYTES;
bf_rt_action_id_t smi_id::A_EGRESS_PORT_IP_STATS_COUNT;

bf_rt_table_id_t smi_id::T_ROTATE_HASH;
bf_rt_action_id_t smi_id::A_ROTATE_BY_0;
bf_rt_action_id_t smi_id::A_ROTATE_BY_1;
bf_rt_action_id_t smi_id::A_ROTATE_BY_2;
bf_rt_action_id_t smi_id::A_ROTATE_BY_3;
bf_rt_action_id_t smi_id::A_ROTATE_BY_4;
bf_rt_action_id_t smi_id::A_ROTATE_BY_5;
bf_rt_action_id_t smi_id::A_ROTATE_BY_6;
bf_rt_action_id_t smi_id::A_ROTATE_BY_7;
bf_rt_action_id_t smi_id::A_ROTATE_BY_8;
bf_rt_action_id_t smi_id::A_ROTATE_BY_9;
bf_rt_action_id_t smi_id::A_ROTATE_BY_10;
bf_rt_action_id_t smi_id::A_ROTATE_BY_11;
bf_rt_action_id_t smi_id::A_ROTATE_BY_12;
bf_rt_action_id_t smi_id::A_ROTATE_BY_13;
bf_rt_action_id_t smi_id::A_ROTATE_BY_14;
bf_rt_action_id_t smi_id::A_ROTATE_BY_15;

bf_rt_table_id_t smi_id::T_INGRESS_FP_FOLD;
bf_rt_field_id_t smi_id::F_INGRESS_FP_IG_INTR_MD_INGRESS_PORT;
bf_rt_action_id_t smi_id::A_INGRESS_FP_FOLD_SET_EGRESS_PORT;
bf_rt_field_id_t smi_id::P_INGRESS_FP_FOLD_SET_EGRESS_PORT_DEV_PORT;

bf_rt_table_id_t smi_id::T_BFD_TX_SESSION;
bf_rt_field_id_t smi_id::F_BFD_TX_SESSION_ID;
bf_rt_action_id_t smi_id::A_BFD_TX_SESSION_V4;
bf_rt_field_id_t smi_id::P_BFD_TX_SESSION_V4_TX_MULT;
bf_rt_field_id_t smi_id::P_BFD_TX_SESSION_V4_SESSION_ID;
bf_rt_field_id_t smi_id::P_BFD_TX_SESSION_V4_VRF;
bf_rt_field_id_t smi_id::P_BFD_TX_SESSION_V4_SIP;
bf_rt_field_id_t smi_id::P_BFD_TX_SESSION_V4_DIP;
bf_rt_field_id_t smi_id::P_BFD_TX_SESSION_V4_SPORT;
bf_rt_field_id_t smi_id::P_BFD_TX_SESSION_V4_DPORT;
bf_rt_action_id_t smi_id::A_BFD_TX_SESSION_V6;
bf_rt_field_id_t smi_id::P_BFD_TX_SESSION_V6_TX_MULT;
bf_rt_field_id_t smi_id::P_BFD_TX_SESSION_V6_SESSION_ID;
bf_rt_field_id_t smi_id::P_BFD_TX_SESSION_V6_VRF;
bf_rt_field_id_t smi_id::P_BFD_TX_SESSION_V6_SIP;
bf_rt_field_id_t smi_id::P_BFD_TX_SESSION_V6_DIP;
bf_rt_field_id_t smi_id::P_BFD_TX_SESSION_V6_SPORT;
bf_rt_field_id_t smi_id::P_BFD_TX_SESSION_V6_DPORT;
bf_rt_action_id_t smi_id::A_BFD_TX_SESSION_BFD_TX_DROP_PKT;
bf_rt_table_id_t smi_id::T_BFD_RX_SESSION;
bf_rt_field_id_t smi_id::F_BFD_RX_SESSION_HDR_BFD_MY_DISCRIMINATOR;
bf_rt_field_id_t smi_id::F_BFD_RX_SESSION_HDR_BFD_YOUR_DISCRIMINATOR;
bf_rt_field_id_t smi_id::F_BFD_RX_SESSION_HDR_BFD_VERSION;
bf_rt_field_id_t smi_id::F_BFD_RX_SESSION_HDR_BFD_FLAGS;
bf_rt_field_id_t smi_id::F_BFD_RX_SESSION_HDR_BFD_DESIRED_MIN_TX_INTERVAL;
bf_rt_field_id_t smi_id::F_BFD_RX_SESSION_HDR_BFD_REQ_MIN_RX_INTERVAL;
bf_rt_action_id_t smi_id::A_BFD_RX_SESSION_INFO;
bf_rt_field_id_t smi_id::P_BFD_RX_SESSION_INFO_RX_MULT;
bf_rt_field_id_t smi_id::P_BFD_RX_SESSION_INFO_SESSION_ID;
bf_rt_field_id_t smi_id::P_BFD_RX_SESSION_INFO_PKTGEN_PIPE;
bf_rt_field_id_t smi_id::P_BFD_RX_SESSION_INFO_RECIRC_PORT;
bf_rt_action_id_t smi_id::A_BFD_RX_SESSION_MISS;
bf_rt_table_id_t smi_id::T_BFD_RX_TIMER;
bf_rt_field_id_t smi_id::F_BFD_RX_TIMER_LOCAL_MD_BFD_PKT_TX;
bf_rt_field_id_t smi_id::F_BFD_RX_TIMER_LOCAL_MD_BFD_SESSION_ID;
bf_rt_table_id_t smi_id::T_BFD_RX_TIMER_REG;
bf_rt_field_id_t smi_id::F_BFD_RX_TIMER_REG_INDEX;
bf_rt_field_id_t smi_id::D_BFD_RX_TIMER_REG_DATA;
bf_rt_action_id_t smi_id::A_BFD_RX_TIMER_RESET;
bf_rt_field_id_t smi_id::P_BFD_RX_TIMER_RESET_SESSION_ID;
bf_rt_action_id_t smi_id::A_BFD_RX_TIMER_CHECK;
bf_rt_field_id_t smi_id::P_BFD_RX_TIMER_CHECK_SESSION_ID;
bf_rt_table_id_t smi_id::T_BFD_PKT_ACTION;
bf_rt_field_id_t smi_id::F_BFD_PKT_ACTION_LOCAL_MD_BFD_PKT_TX;
bf_rt_field_id_t smi_id::F_BFD_PKT_ACTION_LOCAL_MD_BFD_PKT_ACTION;
bf_rt_field_id_t smi_id::F_BFD_PKT_ACTION_LOCAL_MD_BFD_PKTGEN_PIPE;
bf_rt_field_id_t smi_id::F_BFD_PKT_ACTION_PRIORITY;
bf_rt_action_id_t smi_id::A_BFD_PKT_ACTION_BFD_PKT_TO_CPU;
bf_rt_action_id_t smi_id::A_BFD_PKT_ACTION_BFD_RECIRC_TO_PKTGEN_PIPE;
bf_rt_action_id_t smi_id::A_BFD_PKT_ACTION_NOACTION;
bf_rt_action_id_t smi_id::A_BFD_PKT_ACTION_BFD_TX_PKT;
bf_rt_action_id_t smi_id::A_BFD_PKT_ACTION_BFD_DROP_PKT;
bf_rt_table_id_t smi_id::T_BFD_TX_TIMER;
bf_rt_field_id_t smi_id::F_BFD_TX_TIMER_LOCAL_MD_BFD_SESSION_ID;
bf_rt_action_id_t smi_id::A_BFD_TX_TIMER_CHECK;
bf_rt_field_id_t smi_id::P_BFD_TX_TIMER_CHECK_SESSION_ID;
bf_rt_field_id_t smi_id::P_BFD_TX_TIMER_CHECK_DETECT_MULTI;
bf_rt_field_id_t smi_id::P_BFD_TX_TIMER_CHECK_MY_DISCRIMINATOR;
bf_rt_field_id_t smi_id::P_BFD_TX_TIMER_CHECK_YOUR_DISCRIMINATOR;
bf_rt_field_id_t smi_id::P_BFD_TX_TIMER_CHECK_DESIRED_MIN_TX_INTERVAL;
bf_rt_field_id_t smi_id::P_BFD_TX_TIMER_CHECK_REQ_MIN_RX_INTERVAL;
bf_rt_action_id_t smi_id::A_BFD_TX_TIMER_BFD_DROP_PKT;

bf_rt_table_id_t smi_id::T_INGRESS_PKTGEN_PORT;
bf_rt_field_id_t smi_id::F_INGRESS_PKTGEN_PORT_ETHER_TYPE;
bf_rt_field_id_t smi_id::F_INGRESS_PKTGEN_PORT_PORT;

}  // namespace bf_rt
}  // namespace smi
