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


#include <string>
#include <vector>
#include <set>

#include "switch_tna/utils.h"

namespace smi {
using namespace smi::bf_rt;  // NOLINT(build/namespaces)
using ::smi::logging::switch_log;

class fold_pipeline : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_FOLD_PIPELINE;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_FOLD_PIPELINE_ATTR_PARENT_HANDLE;
  std::set<bf_dev_pipe_t> active_pipes;

 public:
  fold_pipeline(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    const std::string fold_table("fold");
    const std::string fold_action("fold_2_pipe");
    if (feature::is_feature_set(SWITCH_FEATURE_ASYMMETRIC_FOLDED_PIPELINE)) {
      active_pipes = get_active_pipes();
    } else {
      attrs.clear();
      return;
    }
    if (active_pipes.size() == 4) {
      // Nothing to do for 4 pipeline system. Taken care by default action.
      attrs.clear();
      return;
    } else if (active_pipes.size() == 2) {
      /* Fold Table exists only on non switch pipeline for asymmetric folded
       * pipeline case. For the fold table we do
       * not have the fqn for the table. So we iterate through list of tables in
       * non switch pipeline and look for this
       * table. If the table exists and it has the fold_2_pipe action we update
       * the default entry to this action */
      auto bf_rt_info = get_bf_rt_info();
      std::vector<const BfRtTable *> tables;
      status = bf_rt_info->bfrtInfoGetTables(&tables);
      if (status != SWITCH_STATUS_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_DEVICE,
                   "{}.{}: Failed to update default  action entry for table {}"
                   "to {}. Failed to fetch tables list from BfRt status {}",
                   __func__,
                   __LINE__,
                   fold_table,
                   fold_action,
                   status);
      }
      for (auto &&table : tables) {
        std::string table_name;
        status = table->tableNameGet(&table_name);
        if (status != SWITCH_STATUS_SUCCESS) continue;
        if (endsWith(table_name, fold_table)) {
          bf_rt_id_t table_id{0};
          table->tableIdGet(&table_id);
          _Table _table(get_dev_tgt(), get_bf_rt_info(), table_id);
          std::vector<bf_rt_action_id_t> action_ids;
          status = table->actionIdListGet(&action_ids);
          if (status != SWITCH_STATUS_SUCCESS) continue;
          bf_rt_action_id_t action_id{0};
          for (auto &&action : action_ids) {
            std::string action_name;
            status = table->actionNameGet(action, &action_name);
            if (status != SWITCH_STATUS_SUCCESS) continue;
            if (endsWith(action_name, fold_action)) {
              action_id = action;
              break;
            }
          }
          if (!action_id) continue;
          _ActionEntry action_entry(table_id);
          action_entry.init_action_data(action_id);
          status |= _table.default_entry_set(action_entry, false);
        }
      }
    }
  }
};

class fold_cpu : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_FOLD_CPU;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_FOLD_CPU_ATTR_PARENT_HANDLE;
  std::set<bf_dev_pipe_t> active_pipes;

 public:
  fold_cpu(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    uint64_t port_id{};
    bf_status_t bf_status = BF_SUCCESS;
    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_PORT_ID, port_id);
    // Nothing do if the pipeline is not folded or if the port is non cpu
    // port
    if (!feature::is_feature_set(SWITCH_FEATURE_ASYMMETRIC_FOLDED_PIPELINE) ||
        (port_id != SWITCH_CPU_PORT_ETH_DEFAULT &&
         port_id != SWITCH_CPU_PORT_PCIE_DEFAULT)) {
      attrs.clear();
      return;
    }
    auto &switch_egress_pipes = SWITCH_CONTEXT.get_switch_egress_pipe_list();
    auto &switch_ingress_pipes = SWITCH_CONTEXT.get_switch_ingress_pipe_list();
    switch_object_id_t cpu_port_handle{parent}, device_handle{};
    uint16_t dev_port{};
    status |=
        switch_store::v_get(parent, SWITCH_PORT_ATTR_DEVICE, device_handle);
    status |= switch_store::v_get(
        cpu_port_handle, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_PORT,
                 "{}.{}: Failed to configure cpu port folding: {}",
                 __func__,
                 __LINE__,
                 status);
      return;
    }
    auto cpu_pipe = DEV_PORT_TO_PIPE(dev_port);
    bool in_switch_ing_pipe{false}, in_switch_eg_pipe{false};
    if (std::find(switch_ingress_pipes.begin(),
                  switch_ingress_pipes.end(),
                  cpu_pipe) != switch_ingress_pipes.end()) {
      in_switch_ing_pipe = true;
    }
    if (std::find(switch_egress_pipes.begin(),
                  switch_egress_pipes.end(),
                  cpu_pipe) != switch_egress_pipes.end()) {
      in_switch_eg_pipe = true;
    }

    const std::string fold_table("fold");
    const std::string set_egress_port("set_egress_port");
    const std::string match_field("ig_intr_md.ingress_port");
    const std::string action_field("dev_port");

    auto bf_rt_info = get_bf_rt_info();
    std::vector<const BfRtTable *> tables;
    bf_status = bf_rt_info->bfrtInfoGetTables(&tables);
    if (bf_status != BF_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_DEVICE,
                 "{}.{}: Failed to add cpu port translation entry in table {}"
                 ".Failed to fetch tables list from BfRt. status {}",
                 __func__,
                 __LINE__,
                 fold_table,
                 bf_rt_status_xlate(bf_status));
    }

    for (auto &&table : tables) {
      std::string table_name;
      bf_status = BF_SUCCESS;
      bf_status = table->tableNameGet(&table_name);
      if (bf_status != BF_SUCCESS) continue;
      if (endsWith(table_name, fold_table)) {
        // Get Fold Table
        bf_rt_id_t table_id{};
        table->tableIdGet(&table_id);
        _Table _table(get_dev_tgt(), get_bf_rt_info(), table_id);
        if (_table.get_active_pipes() !=
            SWITCH_CONTEXT.get_switch_egress_pipe_list())
          continue;

        // Get Fold Table Match:local_md.ingress_port
        std::vector<bf_rt_action_id_t> key_field_ids;
        bf_status = table->keyFieldIdListGet(&key_field_ids);
        if (bf_status != BF_SUCCESS) continue;
        bf_rt_action_id_t key_field_id{0};
        for (auto &&key_field : key_field_ids) {
          std::string key_field_name;
          bf_status = table->keyFieldNameGet(key_field, &key_field_name);
          if (bf_status != BF_SUCCESS) continue;
          if (endsWith(key_field_name, match_field)) {
            key_field_id = key_field;
            break;
          }
        }
        if (!key_field_id) continue;

        // Get Fold Table Action:set_egress_port
        std::vector<bf_rt_action_id_t> action_ids;
        bf_status = table->actionIdListGet(&action_ids);
        if (bf_status != BF_SUCCESS) continue;
        bf_rt_action_id_t action_id{0};
        for (auto &&action : action_ids) {
          std::string action_name;
          bf_status = table->actionNameGet(action, &action_name);
          if (bf_status != BF_SUCCESS) continue;
          if (endsWith(action_name, set_egress_port)) {
            action_id = action;
            break;
          }
        }
        if (!action_id) continue;

        // Find action field id
        bf_rt_field_id_t field_id{0};
        bf_status = table->dataFieldIdGet(action_field, action_id, &field_id);
        if (bf_status != BF_SUCCESS) continue;

        if (!in_switch_eg_pipe) {
          _MatchKey match_key(table_id);
          _ActionEntry action_entry(table_id);
          auto recirc_pipe = EGRESS_DEV_PORT_TO_PIPE(dev_port);
          uint16_t recirc_port{};
          status |=
              get_recirc_port_in_pipe(device_handle, recirc_pipe, recirc_port);
          status |= match_key.set_exact(key_field_id,
                                        translate_egress_port(recirc_port));
          action_entry.init_action_data(action_id);
          status |= action_entry.set_arg(field_id, dev_port);
          bool bf_rt_status = false;
          status |=
              _table.entry_add(match_key, action_entry, bf_rt_status, false);
          if (status != SWITCH_STATUS_SUCCESS) {
            switch_log(SWITCH_API_LEVEL_ERROR,
                       SWITCH_OBJECT_TYPE_PORT,
                       "{}.{}: Failed to  setup pipeline forwarding from "
                       "Recirc Port (Switch Engress) {} -> CPU Port {} status "
                       "{}",
                       __func__,
                       __LINE__,
                       recirc_port,
                       dev_port,
                       status);
          }
        }
        if (!in_switch_ing_pipe) {
          _MatchKey match_key(table_id);
          _ActionEntry action_entry(table_id);
          auto recirc_pipe = INGRESS_DEV_PORT_TO_PIPE(dev_port);
          uint16_t recirc_port{};
          status |=
              get_recirc_port_in_pipe(device_handle, recirc_pipe, recirc_port);
          status |= match_key.set_exact(key_field_id, dev_port);
          action_entry.init_action_data(action_id);
          status |= action_entry.set_arg(field_id, recirc_port);
          bool bf_rt_status = false;
          status |=
              _table.entry_add(match_key, action_entry, bf_rt_status, false);
          if (status != SWITCH_STATUS_SUCCESS) {
            switch_log(SWITCH_API_LEVEL_ERROR,
                       SWITCH_OBJECT_TYPE_PORT,
                       "{}.{}: Failed to setup pipeline forwarding from CPU "
                       "Port {} -> Recirc Port (Switch Ingress) {} status {}",
                       __func__,
                       __LINE__,
                       recirc_port,
                       dev_port,
                       status);
          }
        }
      }
    }
  }
};

switch_status_t afp_init() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (feature::is_feature_set(SWITCH_FEATURE_ASYMMETRIC_FOLDED_PIPELINE)) {
    bf_status_t bf_status = BF_SUCCESS;
    const std::string fold_table("fold");

    auto bf_rt_info = get_bf_rt_info();
    std::vector<const BfRtTable *> tables;
    bf_status = bf_rt_info->bfrtInfoGetTables(&tables);
    status = bf_rt_status_xlate(bf_status);
    if (bf_status != BF_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_DEVICE,
                 "{}.{}: Failed to get bfrt table list, status {}",
                 __func__,
                 __LINE__,
                 status);
      return status;
    }

    for (auto &&table : tables) {
      std::string table_name;
      bf_status = BF_SUCCESS;
      bf_status = table->tableNameGet(&table_name);
      if (bf_status != BF_SUCCESS) continue;
      if (endsWith(table_name, fold_table)) {
        // Get Fold Table
        bf_rt_id_t table_id{};
        table->tableIdGet(&table_id);
        SWITCH_CONTEXT.append_fold_table_ids(table_id);

        for (auto p : _Table(table_id).get_active_pipes()) {
          SWITCH_CONTEXT.update_switch_non_ingress_pipe_list(p);
        }
      }
    }

    REGISTER_OBJECT(fold_pipeline, SWITCH_OBJECT_TYPE_FOLD_PIPELINE);
    REGISTER_OBJECT(fold_cpu, SWITCH_OBJECT_TYPE_FOLD_CPU);
  }
  return status;
}

switch_status_t afp_clean() { return SWITCH_STATUS_SUCCESS; }

}  // namespace smi
