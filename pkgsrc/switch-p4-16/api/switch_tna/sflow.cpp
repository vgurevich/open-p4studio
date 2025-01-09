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


#include <memory>
#include <set>

#include "s3/switch_packet.h"
#include "switch_tna/utils.h"

namespace smi {
using namespace smi::bf_rt;  // NOLINT(build/namespaces)
using ::smi::logging::switch_log;

switch_status_t add_sflow_entry(bf_rt_target_t dev_target,
                                uint32_t session_id,
                                uint32_t rate) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  _Table table(dev_target, get_bf_rt_info(), smi_id::T_INGRESS_SFLOW_SESSION);
  _MatchKey register_key(smi_id::T_INGRESS_SFLOW_SESSION);
  _ActionEntry register_action(smi_id::T_INGRESS_SFLOW_SESSION);
  register_action.init_indirect_data();
  status |= register_key.set_exact(
      smi_id::F_INGRESS_SFLOW_SESSION_REGISTER_INDEX, session_id);
  status |= register_action.set_arg(smi_id::D_INGRESS_SFLOW_SESSION_REG_CURRENT,
                                    rate);
  status |=
      register_action.set_arg(smi_id::D_INGRESS_SFLOW_SESSION_REG_RATE, rate);
  status = table.entry_modify(register_key, register_action);
  if (status != SWITCH_STATUS_SUCCESS) {
    switch_log(
        SWITCH_API_LEVEL_ERROR,
        SWITCH_OBJECT_TYPE_NONE,
        "{}:{}: failed ing sflow session status {} session_id {} rate {}",
        __func__,
        __LINE__,
        status,
        session_id,
        rate);
  }

  // update packet driver id to rate setting
  switch_pkt_sflow_id_to_rate_set(session_id, rate, dev_target.pipe_id);

  return status;
}

// SFLOW session space is carved as first 64 for port, and rest as shared
switch_status_t update_sflow_session_info(bool shared,
                                          uint16_t dev_port,
                                          uint16_t session_id,
                                          uint32_t rate) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (shared) {
    // if session is shared, program on all pipes
    for (bf_dev_pipe_t tpipe :
         _Table(smi_id::T_INGRESS_SFLOW_SESSION).get_active_pipes()) {
      bf_rt_target_t dev_target = {.dev_id = 0, .pipe_id = tpipe};
      status = add_sflow_entry(dev_target, session_id, rate);
    }
  } else {
    // if exclusive, the first 64 entries are reserved per port
    bf_rt_target_t dev_target = compute_dev_target_for_table(
        dev_port, smi_id::T_INGRESS_SFLOW_SESSION, true);
    status = add_sflow_entry(dev_target, session_id, rate);
  }

  return status;
}

class ingress_sflow_session : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_INGRESS_SFLOW_SESSION;
  static const switch_attr_id_t status_attr_id =
      SWITCH_INGRESS_SFLOW_SESSION_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INGRESS_SFLOW_SESSION_ATTR_PARENT_HANDLE;

 public:
  ingress_sflow_session(const switch_object_id_t parent,
                        switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    switch_object_id_t ingress_sflow_handle = {0};
    switch_enum_t _mode = {}, port_type = {};
    uint16_t dev_port = 0;
    uint32_t rate = 0;
    bool shared = false;

    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_TYPE, port_type);
    if (port_type.enumdata != SWITCH_PORT_ATTR_TYPE_NORMAL) return;

    uint8_t sflow_id = compute_sflow_id(parent);

    status |= switch_store::v_get(
        parent, SWITCH_PORT_ATTR_INGRESS_SFLOW_HANDLE, ingress_sflow_handle);
    status |= switch_store::v_get(parent, SWITCH_PORT_ATTR_DEV_PORT, dev_port);

    if (ingress_sflow_handle != 0) {
      status |= switch_store::v_get(
          ingress_sflow_handle, SWITCH_SFLOW_SESSION_ATTR_MODE, _mode);
      status |= switch_store::v_get(
          ingress_sflow_handle, SWITCH_SFLOW_SESSION_ATTR_SAMPLE_RATE, rate);

      if (_mode.enumdata == SWITCH_SFLOW_SESSION_ATTR_MODE_SHARED) {
        shared = true;
      }
    }

    // No need to update sflow table as it is not executed for invalid sflow_id
    // table
    if (sflow_id != SWITCH_SFLOW_INVALID_ID) {
      status |= update_sflow_session_info(shared, dev_port, sflow_id, rate);
    }
  }
};

class sflow_session_helper : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_SFLOW_SESSION_HELPER;
  static const switch_attr_id_t status_attr_id =
      SWITCH_SFLOW_SESSION_HELPER_ATTR_STATUS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_SFLOW_SESSION_HELPER_ATTR_PARENT_HANDLE;

 public:
  sflow_session_helper(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    std::set<switch_object_id_t> port_handles;
    status = switch_store::referencing_set_get(
        parent, SWITCH_OBJECT_TYPE_PORT, port_handles);
    for (const auto port_handle : port_handles) {
      ingress_sflow_session i_sflow(port_handle, status);
      i_sflow.create_update();
    }
    std::set<switch_object_id_t> acl_entry_handles;
    status |= switch_store::referencing_set_get(
        parent, SWITCH_OBJECT_TYPE_ACL_ENTRY, acl_entry_handles);
    for (const auto &acl_entry_handle : acl_entry_handles) {
      std::unique_ptr<object> acl_sample_obj(factory::get_instance().create(
          SWITCH_OBJECT_TYPE_ACL_SAMPLE_SESSION, acl_entry_handle, status));
      if (acl_sample_obj != nullptr) {
        status = acl_sample_obj->create_update();
      }
    }
  }
};

switch_status_t sflow_init() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  REGISTER_OBJECT(ingress_sflow_session,
                  SWITCH_OBJECT_TYPE_INGRESS_SFLOW_SESSION);
  REGISTER_OBJECT(sflow_session_helper,
                  SWITCH_OBJECT_TYPE_SFLOW_SESSION_HELPER);

  return status;
}

switch_status_t sflow_clean() { return SWITCH_STATUS_SUCCESS; }

}  // namespace smi
