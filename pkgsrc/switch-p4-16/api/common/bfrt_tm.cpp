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


#include "common/bfrt_tm.h"

#include <vector>
#include <set>
#include <memory>
#include <utility>
#include <bitset>
#include <string>
#include <map>

#include "common/utils.h"
#include "common/qos_pdfixed.h"

namespace smi {
using namespace smi::bf_rt;  // NOLINT(build/namespaces)
using ::smi::logging::switch_log;

/*
 * Map pd_pool_id to string.
 *
 * @param pd_pool_id      The pool identifier.
 * @param pool_id         Pool id string.
 * @return                Status of the API call.
 *  SWITCH_STATUS_SUCCESS on success
 */
switch_status_t pool_id_mapping(p4_pd_pool_id_t pd_pool_id,
                                std::string &pool_id) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  static std::map<p4_pd_pool_id_t, std::string> pool_id_map{
      {PD_INGRESS_POOL_0, "IG_APP_POOL_0"},
      {PD_INGRESS_POOL_1, "IG_APP_POOL_1"},
      {PD_INGRESS_POOL_2, "IG_APP_POOL_2"},
      {PD_INGRESS_POOL_3, "IG_APP_POOL_3"},
      {PD_EGRESS_POOL_0, "EG_APP_POOL_0"},
      {PD_EGRESS_POOL_1, "EG_APP_POOL_1"},
      {PD_EGRESS_POOL_2, "EG_APP_POOL_2"},
      {PD_EGRESS_POOL_3, "EG_APP_POOL_3"}};

  if (pool_id_map.find(pd_pool_id) != pool_id_map.end()) {
    pool_id = pool_id_map[pd_pool_id];
  } else {
    return SWITCH_STATUS_FAILURE;
  }
  return status;
}

/*
 * Get TM pool size cells paramter.
 *
 * @param pd_pool_id      The pool identifier.
 * @param[out] max_cell   Configured pool size.
 * @return                Status of the API call.
 *  SWITCH_STATUS_SUCCESS on success
 */
switch_status_t bfrt_tm_pool_cfg_size_cells_get(
    const p4_pd_pool_id_t pd_pool_id, uint32_t &max_cell) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::string pool_id = "";

  status = pool_id_mapping(pd_pool_id, pool_id);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_BUFFER_POOL,
               "{}:{}: Failed to map pool id to the string {}",
               __func__,
               __LINE__,
               status);
    return status;
  }

  _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_TM_POOL_CFG);
  _MatchKey match_key(smi_id::T_TM_POOL_CFG);
  _ActionEntry entry(smi_id::T_TM_POOL_CFG);

  entry.init_indirect_data();
  status = match_key.set_exact(smi_id::F_TM_POOL_CFG_POOL, pool_id);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_BUFFER_POOL,
               "{}:{}: Failed to set exact to match key pool id {} for the "
               "tm.pool.cfg table status {}",
               __func__,
               __LINE__,
               pool_id,
               status);
    return status;
  }
  status = table.entry_get(match_key, entry);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_BUFFER_POOL,
               "{}:{}: Failed to get TM pool config entry for pool id {} "
               "status {}",
               __func__,
               __LINE__,
               pool_id,
               status);
    return status;
  }
  status = entry.get_arg(smi_id::D_TM_POOL_CFG_SIZE_CELLS, &max_cell);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_BUFFER_POOL,
               "{}:{}: Failed to get max cell field for the pool id key {} "
               "for the tm.pool.cfg table status {}",
               __func__,
               __LINE__,
               pool_id,
               status);
    return status;
  }
  return status;
}

/*
 * Get TM pool watermark cells and usage cells paramters.
 *
 * @param pd_pool_id      The pool identifier.
 * @param usage_cells     Pool usage count in terms of number of cells.
 * @param watermark_cells Watermark value for pool in terms of number of cells.
 * @return                Status of the API call.
 *  SWITCH_STATUS_SUCCESS on success
 */
switch_status_t bfrt_tm_counter_pool_usage_get(p4_pd_pool_id_t pd_pool_id,
                                               uint32_t &usage_cells,
                                               uint32_t &watermark_cells) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::string pool_id = "";

  status = pool_id_mapping(pd_pool_id, pool_id);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_BUFFER_POOL,
               "{}:{}: Failed to map pool id to the string {}",
               __func__,
               __LINE__,
               status);
    return status;
  }

  _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_TM_COUNTER_POOL);
  _MatchKey match_key(smi_id::T_TM_COUNTER_POOL);
  _ActionEntry entry(smi_id::T_TM_COUNTER_POOL);

  entry.init_indirect_data();
  status = match_key.set_exact(smi_id::F_TM_COUNTER_POOL, pool_id);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_BUFFER_POOL,
               "{}:{}: Failed to set exact to match key pool id {} "
               "for the tm.counter.pool table status {}",
               __func__,
               __LINE__,
               pool_id,
               status);
    return status;
  }
  status = table.entry_get(match_key, entry);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_BUFFER_POOL,
               "{}:{}: Failed to get TM counter pool entry for the pool id {} "
               "status {}",
               __func__,
               __LINE__,
               pool_id,
               status);
    return status;
  }
  status = entry.get_arg(smi_id::D_TM_COUNTER_POOL_USAGE_CELLS, &usage_cells);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_BUFFER_POOL,
               "{}:{}: Failed to get tm counter pool usage cells field for the "
               "pool id key {} for the tm.counter.pool table status {}",
               __func__,
               __LINE__,
               pool_id,
               status);
    return status;
  }
  status = entry.get_arg(smi_id::D_TM_COUNTER_POOL_WATERMARK_CELLS,
                         &watermark_cells);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_BUFFER_POOL,
               "{}:{}: Failed to get tm counter pool watermark cells field "
               " for the pool id key {} for the tm.counter.pool table "
               "status {}",
               __func__,
               __LINE__,
               pool_id,
               status);
    return status;
  }
  return status;
}

/*
 * Clear TM pool watermark cells paramter.
 *
 * @param pd_pool_id      The pool identifier.
 * @return                Status of the API call.
 *  SWITCH_STATUS_SUCCESS on success
 */
switch_status_t bfrt_tm_counter_pool_watermark_cells_clear(
    p4_pd_pool_id_t pd_pool_id) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::string pool_id = "";

  status = pool_id_mapping(pd_pool_id, pool_id);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_BUFFER_POOL,
               "{}:{}: Failed to map pool id to the string {}",
               __func__,
               __LINE__,
               status);
    return status;
  }

  _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_TM_COUNTER_POOL);
  _MatchKey match_key(smi_id::T_TM_COUNTER_POOL);
  _ActionEntry entry(smi_id::T_TM_COUNTER_POOL);

  entry.init_indirect_data();
  status = match_key.set_exact(smi_id::F_TM_COUNTER_POOL, pool_id);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_BUFFER_POOL,
               "{}:{}: Failed to set exact to match pool id key {} for the "
               "tm.counter.pool table status {}",
               __func__,
               __LINE__,
               pool_id,
               status);
    return status;
  }
  uint32_t clear_arg = 0;
  status = entry.set_arg(smi_id::D_TM_COUNTER_POOL_WATERMARK_CELLS, clear_arg);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_BUFFER_POOL,
               "{}:{}: Failed to clear watermark cells field for the pool id "
               "key {} for the tm.counter.pool table status {}",
               __func__,
               __LINE__,
               pool_id,
               status);
    return status;
  }
  status = table.entry_modify(match_key, entry);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_BUFFER_POOL,
               "{}:{}: Failed to modify TM counter pool entry {} "
               "status {}",
               __func__,
               __LINE__,
               status);
    return status;
  }
  return status;
}

/*
 * Set TM pool limit cells paramter.
 *
 * @param pd_pool_id      The pool identifier.
 * @param icos            Class of Service value.
 * @param pfc_limit_cells Configured PFC limit in terms of number
 *                        of cells for an application pool.
 * @return                Status of the API call.
 *  SWITCH_STATUS_SUCCESS on success
 */
switch_status_t bfrt_tm_pool_app_pfc_limit_cells_set(
    p4_pd_pool_id_t pd_pool_id,
    const uint8_t icos,
    const uint32_t pfc_limit_cells) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::string pool_id = "";
  status = pool_id_mapping(pd_pool_id, pool_id);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_BUFFER_POOL,
               "{}:{}: Failed to map pool id to the string {}",
               __func__,
               __LINE__,
               status);
    return status;
  }

  _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_TM_POOL_APP_PFC);
  _MatchKey match_key(smi_id::T_TM_POOL_APP_PFC);
  _ActionEntry entry(smi_id::T_TM_POOL_APP_PFC);

  entry.init_indirect_data();
  status = match_key.set_exact(smi_id::F_TM_POOL_APP_PFC_POOL, pool_id);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_BUFFER_POOL,
               "{}:{}: Failed to set exact to match pool id key {} for the "
               "tm.pool.app.pfc table status {}",
               __func__,
               __LINE__,
               pool_id,
               status);
    return status;
  }
  status = match_key.set_exact(smi_id::F_TM_POOL_APP_PFC_COS, icos);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_BUFFER_POOL,
               "{}:{}: Failed to set exact to match icos key for the "
               "tm.pool.app.pfc table status {}",
               __func__,
               __LINE__,
               status);
    return status;
  }
  status =
      entry.set_arg(smi_id::D_TM_POOL_APP_PFC_LIMIT_CELLS, pfc_limit_cells);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_BUFFER_POOL,
               "{}:{}: Failed to set limit cells field for the pool id {} and "
               "icos keys for the tm.pool.app.pfc table status {}",
               __func__,
               __LINE__,
               pool_id,
               status);
    return status;
  }
  status = table.entry_modify(match_key, entry);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_BUFFER_POOL,
               "{}:{}: Failed to modify TM pool app PFC entry status for the "
               "status {}",
               __func__,
               __LINE__,
               status);
    return status;
  }
  return status;
}

/*
 * Set TM pool size cells paramter.
 *
 * @param pd_pool_id      The pool identifier.
 * @param[out] size_cells Configured number of cells for a particular pool.
 * @return                Status of the API call.
 *  SWITCH_STATUS_SUCCESS on success
 */
switch_status_t bfrt_tm_pool_cfg_size_cells_set(
    const p4_pd_pool_id_t pd_pool_id, const uint32_t size_cells) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::string pool_id = "";

  status = pool_id_mapping(pd_pool_id, pool_id);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_BUFFER_POOL,
               "{}:{}: Failed to map pool id to the string {}",
               __func__,
               __LINE__,
               status);
    return status;
  }

  _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_TM_POOL_CFG);
  _MatchKey match_key(smi_id::T_TM_POOL_CFG);
  _ActionEntry entry(smi_id::T_TM_POOL_CFG);

  entry.init_indirect_data();
  status = match_key.set_exact(smi_id::F_TM_POOL_CFG_POOL, pool_id);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_BUFFER_POOL,
               "{}:{}: Failed to set exact to match pool id key {} for the "
               "tm.pool.cfg table status {}",
               __func__,
               __LINE__,
               pool_id,
               status);
    return status;
  }
  status = entry.set_arg(smi_id::D_TM_POOL_CFG_SIZE_CELLS, size_cells);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_BUFFER_POOL,
               "{}:{}: Failed to set pool size cells field for the pool id "
               "key {} for the tm.pool.cfg table status {}",
               __func__,
               __LINE__,
               pool_id,
               status);
    return status;
  }
  status = table.entry_modify(match_key, entry);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_BUFFER_POOL,
               "{}:{}: Failed to modify TM pool config entry "
               "status {}",
               __func__,
               __LINE__,
               status);
    return status;
  }
  return status;
}

/*
 * Set TM Port Scheduler shaping rate parameters.
 *
 * @param dev_port        The device port ID
 * @param rate_unit       Unit of shaping rate: PPS - in Packets/s;
 *                        BPS - Kbit/s for rate.
 * @param max_rate        Port Shaping rate in units (Packets/s or Kbit/s)
 * @param max_burst_size  Port Shaping burst size
 * @return                Status of the API call.
 *  SWITCH_STATUS_SUCCESS on success
 */
switch_status_t bfrt_tm_port_sched_shaping_rate_set(
    const uint32_t dev_port,
    std::string rate_unit,
    const uint32_t max_rate,
    const uint32_t max_burst_size) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  _Table table(
      get_dev_tgt(), get_bf_rt_info(), smi_id::T_TM_PORT_SCHED_SHAPING);
  _MatchKey match_key(smi_id::T_TM_PORT_SCHED_SHAPING);
  _ActionEntry entry(smi_id::T_TM_PORT_SCHED_SHAPING);

  entry.init_indirect_data();
  status =
      match_key.set_exact(smi_id::F_TM_PORT_SCHED_SHAPING_DEV_PORT, dev_port);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "{}:{}: Failed to set exact to match dev_port key for the "
               "tm.port.sched_shaping table status {}",
               __func__,
               __LINE__,
               status);
    return status;
  }
  status |= entry.set_arg(smi_id::D_TM_PORT_SCHED_SHAPING_UNIT, rate_unit);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "{}:{}: Failed to set rate unit field for the dev port key "
               "for the tm.port.sched_shaping table status {}",
               __func__,
               __LINE__,
               status);
    return status;
  }
  status |= entry.set_arg(smi_id::D_TM_PORT_SCHED_SHAPING_MAX_RATE, max_rate);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "{}:{}: Failed to set max rate field for the dev port key "
               "for the tm.port.sched_shaping table status {}",
               __func__,
               __LINE__,
               status);
    return status;
  }
  status |= entry.set_arg(smi_id::D_TM_PORT_SCHED_SHAPING_MAX_BURST_SIZE,
                          max_burst_size);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "{}:{}: Failed to set max burst size field for the dev port key"
               " for the tm.port.sched_shaping table status {}",
               __func__,
               __LINE__,
               status);
    return status;
  }
  status |= table.entry_modify(match_key, entry);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "{}:{}: Failed to modify TM port sched shaping entry "
               "status {}",
               __func__,
               __LINE__,
               status);
    return status;
  }
  return status;
}

/*
 * Set TM flow control mode to use on the port.
 *
 * @param dev_port        The device port ID
 * @param mode_tx         Flow control mode to use on the port:
 *                        PFC (IEE 802.1Qbb), port PAUSE (IEEE 802.3 ) or NONE
 * @return                Status of the API call.
 *  SWITCH_STATUS_SUCCESS on success
 */
switch_status_t bfrt_tm_port_flowcontrol_mode_tx_set(const uint32_t dev_port,
                                                     std::string mode_tx) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_TM_PORT_FLOWCONTROL);
  _MatchKey match_key(smi_id::T_TM_PORT_FLOWCONTROL);
  _ActionEntry entry(smi_id::T_TM_PORT_FLOWCONTROL);

  entry.init_indirect_data();
  status =
      match_key.set_exact(smi_id::F_TM_PORT_FLOWCONTROL_DEV_PORT, dev_port);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_PORT,
               "{}:{}: Failed to set exact to match dev port key for the "
               "tm.port.flowcontrol table status {}",
               __func__,
               __LINE__,
               status);
    return status;
  }
  status |= entry.set_arg(smi_id::D_TM_PORT_FLOWCONTROL_MODE_TX, mode_tx);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "{}:{}: Failed to set mode tx field for the dev port key "
               "for the tm.port.flowcontrol table status {}",
               __func__,
               __LINE__,
               status);
    return status;
  }
  status |= table.entry_modify(match_key, entry);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "{}:{}: Failed to modify TM port flowcontrol entry "
               "status {}",
               __func__,
               __LINE__,
               status);
    return status;
  }
  return status;
}

/*
 * Set TM port should react on flow control frame.
 *
 * @param dev_port        The device port ID
 * @param mode_rx         How the port should react on flow control frame:
 *                        PFC - to honor PPF; PORT - to pause the port; NONE
 * @return                Status of the API call.
 *  SWITCH_STATUS_SUCCESS on success
 */
switch_status_t bfrt_tm_port_flowcontrol_mode_rx_set(const uint32_t dev_port,
                                                     std::string mode_rx) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_TM_PORT_FLOWCONTROL);
  _MatchKey match_key(smi_id::T_TM_PORT_FLOWCONTROL);
  _ActionEntry entry(smi_id::T_TM_PORT_FLOWCONTROL);

  entry.init_indirect_data();
  status =
      match_key.set_exact(smi_id::F_TM_PORT_FLOWCONTROL_DEV_PORT, dev_port);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "{}:{}: Failed to set exact to match dev port key for the "
               "tm.port.flowcontrol table status {}",
               __func__,
               __LINE__,
               status);
    return status;
  }
  status |= entry.set_arg(smi_id::D_TM_PORT_FLOWCONTROL_MODE_RX, mode_rx);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "{}:{}: Failed to set mode rx field for the dev port key "
               "for the tm.port.flowcontrol table status {}",
               __func__,
               __LINE__,
               status);
    return status;
  }
  status |= table.entry_modify(match_key, entry);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "{}:{}: Failed to modify TM port flowcontrol entry "
               "status {}",
               __func__,
               __LINE__,
               status);
    return status;
  }
  return status;
}

/*
 * Set TM port iCoS to CoS map translation when needs to send PFC frame to the
 * connected counterparty in the PFC mode.
 *
 * @param dev_port        The device port ID
 * @param cos_to_icos     iCoS to CoS map
 * @return                Status of the API call.
 *  SWITCH_STATUS_SUCCESS on success
 */
switch_status_t bfrt_tm_port_flowcontrol_cos_to_icos_set(
    const uint32_t dev_port, uint8_t cos_to_icos[SWITCH_BUFFER_PFC_ICOS_MAX]) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::vector<bf_rt_id_t> cos_to_icos_map;
  for (int i = 0; i < SWITCH_BUFFER_PFC_ICOS_MAX; i++) {
    cos_to_icos_map.push_back((bf_rt_id_t)cos_to_icos[i]);
  }
  _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_TM_PORT_FLOWCONTROL);
  _MatchKey match_key(smi_id::T_TM_PORT_FLOWCONTROL);
  _ActionEntry entry(smi_id::T_TM_PORT_FLOWCONTROL);

  entry.init_indirect_data();
  status =
      match_key.set_exact(smi_id::F_TM_PORT_FLOWCONTROL_DEV_PORT, dev_port);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_BUFFER_POOL,
               "{}:{}: Failed to set exact to match dev port key for the "
               "tm.port.flowcontrol table status {}",
               __func__,
               __LINE__,
               status);
    return status;
  }
  status |=
      entry.set_arg(smi_id::D_TM_PORT_FLOWCONTROL_COS_TO_ICOS, cos_to_icos_map);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "{}:{}: Failed to set cos to icos map field for the dev port "
               "key for the tm.port.flowcontrol table status {}",
               __func__,
               __LINE__,
               status);
    return status;
  }
  status |= table.entry_modify(match_key, entry);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "{}:{}: Failed to modify TM port floewcontrol entry for the "
               "status {}",
               __func__,
               __LINE__,
               status);
    return status;
  }
  return status;
}

/*
 * Enable token bucket that assures max shaping rate for the Port with port's
 * scheduler shaping parameters
 *
 * @param dev_port        The device port ID
 * @param max_rate_enable true/false to enable or disable max shaping rate
 * @return                Status of the API call.
 *  SWITCH_STATUS_SUCCESS on success
 */
switch_status_t bfrt_tm_port_sched_cfg_max_rate_enable_set(
    const uint32_t dev_port, const bool max_rate_enable) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  _Table table(get_dev_tgt(), get_bf_rt_info(), smi_id::T_TM_PORT_SCHED_CFG);
  _MatchKey match_key(smi_id::T_TM_PORT_SCHED_CFG);
  _ActionEntry entry(smi_id::T_TM_PORT_SCHED_CFG);

  entry.init_indirect_data();
  status = match_key.set_exact(smi_id::F_TM_PORT_SCHED_CFG_DEV_PORT, dev_port);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_BUFFER_POOL,
               "{}:{}: Failed to set exact to match dev port key for the "
               "tm.port.sched_cfg table status {}",
               __func__,
               __LINE__,
               status);
    return status;
  }
  status |= entry.set_arg(smi_id::D_TM_PORT_SCHED_CFG_MAX_RATE_ENABLE,
                          max_rate_enable);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "{}:{}: Failed to set max rate enable field for the dev port "
               "key for the tm.port.sched_cfg table status {}",
               __func__,
               __LINE__,
               status);
    return status;
  }
  status |= table.entry_modify(match_key, entry);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "{}:{}: Failed to modify TM port sched config entry for the "
               "status {}",
               __func__,
               __LINE__,
               status);
    return status;
  }
  return status;
}

/*
 * Get cell size in bytes.
 *
 * @param device_id         The device ID
 * @param cell_size_bytes   Cell size in bytes
 * @return                  Status of the API call.
 *  SWITCH_STATUS_SUCCESS on success
 */
switch_status_t bfrt_tm_cfg_cell_size_bytes_get(const uint16_t device_id,
                                                uint32_t &cell_size_bytes) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  const bf_rt_target_t dev_tgt = {.dev_id = device_id,
                                  .pipe_id = BF_DEV_PIPE_ALL};

  _Table table_tm(dev_tgt, get_bf_rt_info(), smi_id::T_TM_CFG);
  _ActionEntry entry(smi_id::T_TM_CFG);
  entry.init_indirect_data();

  status = table_tm.default_entry_get(entry);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "{}.{}: Failed to entry_get from tm.cfg table entry:{}",
               __func__,
               __LINE__,
               switch_error_to_string(status));
    return status;
  }

  status = entry.get_arg(smi_id::D_TM_CFG_CELL_SIZE, &cell_size_bytes);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "{}.{}: Failed to get_arg cell_size_bytes for the tm cfg table,"
               " status:{}",
               __func__,
               __LINE__,
               switch_error_to_string(status));
  }
  return status;
}

/*
 * Get total buffer size.
 *
 * @param device_id           The device ID
 * @param total_buffer_size   Total buffer size in bytes
 * @return                    Status of the API call.
 *  SWITCH_STATUS_SUCCESS on success
 */
switch_status_t bfrt_tm_cfg_total_buffer_size_get(const uint16_t device_id,
                                                  uint64_t &total_buffer_size) {
  uint32_t cell_size_bytes = 0, total_cells = 0;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  const bf_rt_target_t dev_tgt = {.dev_id = device_id,
                                  .pipe_id = BF_DEV_PIPE_ALL};

  _Table table_tm(dev_tgt, get_bf_rt_info(), smi_id::T_TM_CFG);
  _ActionEntry entry(smi_id::T_TM_CFG);
  entry.init_indirect_data();

  status = table_tm.default_entry_get(entry);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "{}.{}: Failed to get entry from tm.cfg table entry:{}",
               __func__,
               __LINE__,
               switch_error_to_string(status));
    return status;
  }

  status = entry.get_arg(smi_id::D_TM_CFG_CELL_SIZE, &cell_size_bytes);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_NONE,
               "{}.{}: Failed to retrieve cell_size_bytes for the tm.cfg "
               "table, status:{}",
               __func__,
               __LINE__,
               switch_error_to_string(status));
    return status;
  }

  status = entry.get_arg(smi_id::D_TM_CFG_TOTAL_CELLS, &total_cells);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OBJECT_TYPE_DEVICE,
               "{}.{}: Failed to retrieve total_cells for the tm.cfg "
               "table, status:{}",
               __func__,
               __LINE__,
               switch_error_to_string(status));
    return status;
  }

  total_buffer_size = (cell_size_bytes * total_cells);

  return status;
}

}  // namespace smi
