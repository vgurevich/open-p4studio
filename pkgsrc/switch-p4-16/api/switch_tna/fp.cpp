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


#include "switch_tna/utils.h"
extern "C" {
#include "tofino/bf_pal/pltfm_intf.h"
}

#define FP_FOLD_PIPE(pipe) pipe ^ 0x1
#define MAP_DEV_PORT_TO_PIPE(dev_port, pipe) \
  ((dev_port & ~(3U << 7)) | pipe << 7)

// Currently for 32Q hardware, driver implicitly adds internal ports in 100G
// mode, hence only one of the 4 dev port quadruplet is available. The below
// mask is used to ensure that traffic from each of the 4 quadruplet ports in
// the external pipe is folded on to the 100G port added by the driver. On the
// model, the behavior is slightly different, where in the driver does not add
// internal ports and these ports are explicitly added by the BMAI layer
// corresponding to each external port added by the user application. Due to
// this behavior such a mask is not required for model operation. Going forward
// this behavior shall be fixed and the driver/model behavior will be the same.
// When that happens this code hack can be removed
#define PORT_MASK 0x1FC

namespace smi {
using namespace smi::bf_rt;  // NOLINT(build/namespaces)
using ::smi::logging::switch_log;

class fp_fold : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_FP_FOLD;
  static const switch_attr_id_t status_attr_id = SWITCH_FP_FOLD_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_FP_FOLD_ATTR_PARENT_HANDLE;

 public:
  fp_fold(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_INGRESS_FP_FOLD,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    uint16_t dev_port{};
    switch_enum_t port_type{SWITCH_PORT_ATTR_TYPE_NORMAL};
    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_DEV_PORT, dev_port);
    bf_dev_pipe_t port_pipe = DEV_PORT_TO_PIPE(dev_port);
    bf_dev_pipe_t fold_pipe = FP_FOLD_PIPE(port_pipe);
    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_TYPE, port_type);
    match_key.set_exact(smi_id::F_INGRESS_FP_IG_INTR_MD_INGRESS_PORT, dev_port);
    switch_object_id_t device_handle{};
    status |=
        switch_store::v_get(parent, SWITCH_PORT_ATTR_DEVICE, device_handle);
    if (port_type.enumdata == SWITCH_PORT_ATTR_TYPE_NORMAL) {
      uint16_t device = 0;
      status |=
          switch_store::v_get(device_handle, SWITCH_DEVICE_ATTR_DEV_ID, device);
      uint16_t fold_dev_port = MAP_DEV_PORT_TO_PIPE(dev_port, fold_pipe);
      bool sw_model = false;
      bf_status_t bf_status = BF_SUCCESS;
      bf_status = bf_pal_pltfm_type_get(device, &sw_model);
      if (bf_status != BF_SUCCESS) {
        switch_log(SWITCH_API_LEVEL_ERROR,
                   SWITCH_OBJECT_TYPE_PORT,
                   "{}.{}: Failed to query platform type for device {} "
                   "status {}",
                   __func__,
                   __LINE__,
                   device,
                   bf_err_str(bf_status));
        status |= pal_status_xlate(bf_status);
        return;
      }
      bool bmai_managed_internal_ports =
          SWITCH_CONTEXT.is_internal_pipe_ports_bmai_managed();
      if (!bmai_managed_internal_ports) {
        fold_dev_port &= PORT_MASK;
      }
      action_entry.init_action_data(smi_id::A_INGRESS_FP_FOLD_SET_EGRESS_PORT);
      status |= action_entry.set_arg(
          smi_id::P_INGRESS_FP_FOLD_SET_EGRESS_PORT_DEV_PORT, fold_dev_port);
    } else if (port_type.enumdata == SWITCH_PORT_ATTR_TYPE_CPU) {
      uint16_t recirc_dev_port{};
      status |=
          get_recirc_port_in_pipe(device_handle, fold_pipe, recirc_dev_port);
      action_entry.init_action_data(smi_id::A_INGRESS_FP_FOLD_SET_EGRESS_PORT);
      status |= action_entry.set_arg(
          smi_id::P_INGRESS_FP_FOLD_SET_EGRESS_PORT_DEV_PORT, recirc_dev_port);
    } else {
      clear_attrs();
      return;
    }
  }
};

class cpu_pvs : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_CPU_PVS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_CPU_PVS_ATTR_PARENT_HANDLE;
  uint16_t cpu_dev_port = {0};
  switch_object_id_t device_handle{};
  static constexpr bf_rt_target_t dev_tgt = {.dev_id = 0,
                                             .pipe_id = BF_DEV_PIPE_ALL,
                                             .direction = BF_DEV_DIR_ALL,
                                             .prsr_id = 0xFF};

 public:
  cpu_pvs(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    uint64_t port_id{};
    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_PORT_ID, port_id);
    // Nothing do if the pipeline is not folded or if the port is non cpu
    // port
    if (port_id != SWITCH_CPU_PORT_ETH_DEFAULT &&
        port_id != SWITCH_CPU_PORT_PCIE_DEFAULT) {
      attrs.clear();
      return;
    }

    status |=
        switch_store::v_get(parent, SWITCH_PORT_ATTR_DEVICE, device_handle);
    status |=
        switch_store::v_get(parent, SWITCH_PORT_ATTR_DEV_PORT, cpu_dev_port);
  }
  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (!device_handle.data) return status;
    status = auto_object::create_update();
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_NONE,
                 "{}.{}: auto_obj.create_update failure status {}",
                 __func__,
                 __LINE__,
                 status);
      return status;
    }

    // Set Egress1 parser value set.
    bool bf_rt_status = false;
    uint16_t port_mask = 0x1FF;
    uint16_t ether_type_mask = 0xFFFF;
    uint16_t recirc_port{};
    auto egress1_dev_tgt = compute_dev_target_for_table(
        cpu_dev_port, smi_id::T_INTERNAL_PIPE_CPU_PORT, true);

    _Table egress1_table(
        dev_tgt, get_bf_rt_info(), smi_id::T_INTERNAL_PIPE_CPU_PORT);
    _MatchKey egress1_match_key(smi_id::T_INTERNAL_PIPE_CPU_PORT);
    _ActionEntry egress1_action_entry(smi_id::T_INTERNAL_PIPE_CPU_PORT);

    status |= egress1_match_key.set_ternary(
        smi_id::F_INTERNAL_PIPE_CPU_PORT_ETHER_TYPE,
        ether_type_bfn,
        ether_type_mask);
    status |= get_recirc_port_in_pipe(
        device_handle, egress1_dev_tgt.pipe_id, recirc_port);
    status |= egress1_match_key.set_ternary(
        smi_id::F_INTERNAL_PIPE_CPU_PORT_PORT, recirc_port, port_mask);
    status |= egress1_action_entry.init_indirect_data();
    status |= egress1_table.entry_add(
        egress1_match_key, egress1_action_entry, bf_rt_status);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(
          SWITCH_API_LEVEL_ERROR,
          SWITCH_OBJECT_TYPE_PORT,
          "{}.{}: failure to set Switch Egress1 pvs for cpu port handle {} "
          "status {}",
          __func__,
          __LINE__,
          cpu_dev_port,
          status);
      return status;
    }

    // Set Ingress1 parser value set.
    auto ingress1_dev_tgt = compute_dev_target_for_table(
        cpu_dev_port, smi_id::T_INGRESS_PIPE_CPU_PORT, true);

    _Table ingress1_table(
        dev_tgt, get_bf_rt_info(), smi_id::T_INGRESS_PIPE_CPU_PORT);
    _MatchKey ingress1_match_key(smi_id::T_INGRESS_PIPE_CPU_PORT);
    _ActionEntry ingress1_action_entry(smi_id::T_INGRESS_PIPE_CPU_PORT);

    status |= ingress1_match_key.set_ternary(
        smi_id::F_INGRESS_PIPE_CPU_PORT_ETHER_TYPE,
        ether_type_bfn,
        ether_type_mask);
    status |= get_recirc_port_in_pipe(
        device_handle, ingress1_dev_tgt.pipe_id, recirc_port);
    status |= ingress1_match_key.set_ternary(
        smi_id::F_INGRESS_PIPE_CPU_PORT_PORT, recirc_port, port_mask);
    status |= ingress1_action_entry.init_indirect_data();
    status |= ingress1_table.entry_add(
        ingress1_match_key, ingress1_action_entry, bf_rt_status);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(
          SWITCH_API_LEVEL_ERROR,
          SWITCH_OBJECT_TYPE_PORT,
          "{}.{}: failure to set Switch Ingress 1 pvs for cpu port handle {} "
          "status {}",
          __func__,
          __LINE__,
          cpu_dev_port,
          status);
      return status;
    }
    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    if (!device_handle.data) return status;

    // Set Egress1 parser value set.
    uint16_t port_mask = 0x1FF;
    uint16_t ether_type_mask = 0xFFFF;
    uint16_t recirc_port{};
    auto egress1_dev_tgt = compute_dev_target_for_table(
        cpu_dev_port, smi_id::T_INTERNAL_PIPE_CPU_PORT, true);

    _Table egress1_table(
        dev_tgt, get_bf_rt_info(), smi_id::T_INTERNAL_PIPE_CPU_PORT);
    _MatchKey egress1_match_key(smi_id::T_INTERNAL_PIPE_CPU_PORT);

    status |= egress1_match_key.set_ternary(
        smi_id::F_INTERNAL_PIPE_CPU_PORT_ETHER_TYPE,
        ether_type_bfn,
        ether_type_mask);
    status |= get_recirc_port_in_pipe(
        device_handle, egress1_dev_tgt.pipe_id, recirc_port);
    status |= egress1_match_key.set_ternary(
        smi_id::F_INTERNAL_PIPE_CPU_PORT_PORT, recirc_port, port_mask);
    status |= egress1_table.entry_delete(egress1_match_key);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(
          SWITCH_API_LEVEL_ERROR,
          SWITCH_OBJECT_TYPE_PORT,
          "{}.{}: failure to delete Switch Egress1 pvs for cpu port handle {} "
          "status {}",
          __func__,
          __LINE__,
          cpu_dev_port,
          status);
      return status;
    }

    // Set Ingress1 parser value set.
    auto ingress1_dev_tgt = compute_dev_target_for_table(
        cpu_dev_port, smi_id::T_INGRESS_PIPE_CPU_PORT, true);

    _Table ingress1_table(
        dev_tgt, get_bf_rt_info(), smi_id::T_INGRESS_PIPE_CPU_PORT);
    _MatchKey ingress1_match_key(smi_id::T_INGRESS_PIPE_CPU_PORT);
    _ActionEntry ingress1_action_entry(smi_id::T_INGRESS_PIPE_CPU_PORT);

    status |= ingress1_match_key.set_ternary(
        smi_id::F_INGRESS_PIPE_CPU_PORT_ETHER_TYPE,
        ether_type_bfn,
        ether_type_mask);
    status |= get_recirc_port_in_pipe(
        device_handle, ingress1_dev_tgt.pipe_id, recirc_port);
    status |= ingress1_match_key.set_ternary(
        smi_id::F_INGRESS_PIPE_CPU_PORT_PORT, recirc_port, port_mask);
    status |= ingress1_table.entry_delete(ingress1_match_key);
    if (status != SWITCH_STATUS_SUCCESS) {
      switch_log(SWITCH_API_LEVEL_ERROR,
                 SWITCH_OBJECT_TYPE_PORT,
                 "{}.{}: failure to delete Switch Ingress 1 pvs for cpu port "
                 "handle {} "
                 "status {}",
                 __func__,
                 __LINE__,
                 cpu_dev_port,
                 status);
      return status;
    }
    return status;
  }
};
constexpr bf_rt_target_t cpu_pvs::dev_tgt;

switch_status_t fp_init() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  if (feature::is_feature_set(SWITCH_FEATURE_FOLDED_SWITCH_PIPELINE)) {
    REGISTER_OBJECT(fp_fold, SWITCH_OBJECT_TYPE_FP_FOLD);
    REGISTER_OBJECT(cpu_pvs, SWITCH_OBJECT_TYPE_CPU_PVS);
  }
  return status;
}
switch_status_t fp_clean() { return SWITCH_STATUS_SUCCESS; }
}  // namespace smi
