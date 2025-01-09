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


#ifdef __cplusplus
extern "C" {
#endif
#include <bf_rt/bf_rt_common.h>
#ifdef __cplusplus
}

#endif
#include <bf_rt/bf_rt_session.hpp>
#include <bf_rt/bf_rt_table_operations.h>
#include <bf_rt/bf_rt_table_operations.hpp>
#include "bf_rt_init_impl.hpp"
#include "bf_rt_table_operations_impl.hpp"
#include "bf_rt_table_operations_state.hpp"
#include "bf_rt_pipe_mgr_intf.hpp"

namespace bfrt {
namespace {

// Maximum time in seconds to wait for a table operation callback execution.
// It should be large enough to allow requests on tables with maximum size
// possible for the HW platform.
// Different clients, like gRPC, can have other timeouts for their purposes,
// and this one is to be applied at BFRT level to make sure there is no state
// object left awaiting callback forever due to some HW or pipe manager issue.
// Default value estimation (3 min.) might need adjustment in the future.
// It is assumed that the callback delayed for so long will never come,
// otherwise it will awake the reused state earlier and out of its sequence.
// The subsequent actual callback will either find the state released, or it
// will become another early response out of sequence, and so on.
// An already released state and a synchronous call state should have their
// callback pointers clear, so no unexpected call to the client callback
// happens from them, but the log warning.
static const int operation_default_grace_sec = 3 * 60;

// Maximum default number of active operations (state objects) allowed to be
// requested simultaneously per-table, per-operation.
// This number should be adjusted for each table and operation type, depending
// on the table size and how much time is needed to execute the operation.
// Large number of concurrent requests slows down response, consumes memory
// and DMA buffers.
static const std::size_t operation_default_max_table_op_states = 256;

// Maximum default number of threads to serve for client's callback execution
// on per-operation, per-table basis.
// This number should be adjusted for each table and operation type, depending
// on how much time it takes to execute the client callback, and how many
// simultaneous operations are expected to run in parallel for the table.
// If this number is set to zero, then no threads will be spawn by BFRT and
// the client callback will be executed by the low-level driver thread.
// This might slow down the driver's performance.
static const std::size_t operation_default_max_callback_threads = 1;

// This templated callback function is to be called from the low-level driver
// code to execute the table operation state as an internal proxy callback
// object.
template <typename T, typename U>
void bfRtOperationsInternalCallback(bf_dev_id_t device_id, void *cb_cookie) {
  BfRtStateTableOperations<T, U>::stateTableOperationsCallback(device_id,
                                                               cb_cookie);
}

}  // anonymous namespace

TableOperationsType BfRtTableOperationsImpl::getType(
    const std::string &op_name, const BfRtTable::TableType &object_type) {
  TableOperationsType ret_val = TableOperationsType::INVALID;
  switch (object_type) {
    case BfRtTable::TableType::REGISTER:
      if (op_name == "Sync") {
        ret_val = TableOperationsType::REGISTER_SYNC;
      }
      break;
    case BfRtTable::TableType::COUNTER:
      if (op_name == "Sync") {
        ret_val = TableOperationsType::COUNTER_SYNC;
      }
      break;
    case BfRtTable::TableType::MATCH_DIRECT:
    case BfRtTable::TableType::MATCH_INDIRECT:
    case BfRtTable::TableType::MATCH_INDIRECT_SELECTOR:
      if (op_name == "SyncRegisters") {
        ret_val = TableOperationsType::REGISTER_SYNC;
      } else if (op_name == "SyncCounters") {
        ret_val = TableOperationsType::COUNTER_SYNC;
      } else if (op_name == "UpdateHitState") {
        ret_val = TableOperationsType::HIT_STATUS_UPDATE;
      }
      break;
    default:
      break;
  }
  return ret_val;
}

// Works on Match Action tables and register tables only
bf_status_t BfRtTableOperationsImpl::registerSyncSet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const BfRtRegisterSyncCb &callback,
    const void *cookie) {
  return registerSyncInternal(session, dev_tgt, callback, nullptr, cookie);
}

// Works on Match Action tables and counter tables only
bf_status_t BfRtTableOperationsImpl::counterSyncSet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const BfRtCounterSyncCb &callback,
    const void *cookie) {
  return counterSyncInternal(session, dev_tgt, callback, nullptr, cookie);
}

bf_status_t BfRtTableOperationsImpl::hitStateUpdateSet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const BfRtHitStateUpdateCb &callback,
    const void *cookie) {
  return hitStateUpdateInternal(session, dev_tgt, callback, nullptr, cookie);
}

// Works on Match Action tables and register tables only
bf_status_t BfRtTableOperationsImpl::registerSyncSetCFrontend(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const bf_rt_register_sync_cb &callback,
    const void *cookie) {
  return registerSyncInternal(session, dev_tgt, nullptr, callback, cookie);
}

// Works on Match Action tables and counter tables only
bf_status_t BfRtTableOperationsImpl::counterSyncSetCFrontend(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const bf_rt_counter_sync_cb &callback,
    const void *cookie) {
  return counterSyncInternal(session, dev_tgt, nullptr, callback, cookie);
}

bf_status_t BfRtTableOperationsImpl::hitStateUpdateSetCFrontend(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const bf_rt_hit_state_update_cb &callback,
    const void *cookie) {
  return hitStateUpdateInternal(session, dev_tgt, nullptr, callback, cookie);
}

bf_status_t BfRtTableOperationsImpl::registerSyncInternal(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const BfRtRegisterSyncCb &callback,
    const bf_rt_register_sync_cb &callback_c,
    const void *cookie) {
  session_ = &session;
  dev_tgt_ = dev_tgt;
  register_cpp_ = callback;
  register_c_ = callback_c;
  cookie_ = cookie;
  return BF_SUCCESS;
}

bf_status_t BfRtTableOperationsImpl::counterSyncInternal(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const BfRtCounterSyncCb &callback,
    const bf_rt_counter_sync_cb &callback_c,
    const void *cookie) {
  session_ = &session;
  dev_tgt_ = dev_tgt;
  counter_cpp_ = callback;
  counter_c_ = callback_c;
  cookie_ = cookie;
  return BF_SUCCESS;
}

bf_status_t BfRtTableOperationsImpl::hitStateUpdateInternal(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const BfRtHitStateUpdateCb &callback,
    const bf_rt_hit_state_update_cb &callback_c,
    const void *cookie) {
  session_ = &session;
  dev_tgt_ = dev_tgt;
  hit_state_cpp_ = callback;
  hit_state_c_ = callback_c;
  cookie_ = cookie;
  return BF_SUCCESS;
}

bf_status_t BfRtTableOperationsImpl::registerSyncExecute() const {
  // If table is MAT then call direct sync else if register
  // table then indirect sync
  BfRtTable::TableType table_type;
  table_->tableTypeGet(&table_type);
  bool direct = false;
  if (table_type == BfRtTable::TableType::MATCH_DIRECT ||
      table_type == BfRtTable::TableType::MATCH_INDIRECT ||
      table_type == BfRtTable::TableType::MATCH_INDIRECT_SELECTOR) {
    direct = true;
  } else if (table_type == BfRtTable::TableType::REGISTER) {
    direct = false;
  } else {
    LOG_ERROR("%s:%d %s Invalid function for this table",
              __func__,
              __LINE__,
              table_->table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }
  // get the state and update it
  auto prog_name = table_->programNameGet();
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt_.dev_id, prog_name);
  if (device_state == nullptr) {
    LOG_ERROR("%s:%d device state not found for device %d",
              __func__,
              __LINE__,
              dev_tgt_.dev_id);
    return BF_OBJECT_NOT_FOUND;
  }
  auto register_state_pool =
      device_state->operationsRegisterState.getObjState(table_->table_id_get());

  // If the previous operation holds the table state longer, then consider
  // its state as obsolete and override it by this new operation state.
  // TODO: make this timeout a class property with get/set methods.
  int register_grace_sec = operation_default_grace_sec;

  // Maximum number of simultaneous register sync requests per table.
  // TODO: make it a class property with get/set methods.
  std::size_t register_max_states = operation_default_max_table_op_states;

  // Maximum number of callback servicing threads.
  // TODO: make it a class property with get/set methods.
  std::size_t register_max_threads = operation_default_max_callback_threads;

  std::size_t state_obj_idx = 0;
  bool got_state =
      register_state_pool->stateTableOperationsRetain(register_cpp_,
                                                      register_c_,
                                                      cookie_,
                                                      dev_tgt_,
                                                      register_grace_sec,
                                                      register_max_states,
                                                      register_max_threads,
                                                      state_obj_idx);
  if (!got_state) {
    LOG_TRACE(
        "%s:%d %s Too many table operations are in progress, "
        "try to repeat later.",
        __func__,
        __LINE__,
        table_->table_name_get().c_str());
    return BF_EAGAIN;
  }

  // If no cb has been registered, then dont register BFRT cb with
  // pipe_mgr
  // We still want to hold an operation state object of a synchronous call
  // at BFRT to count it as active together with other concurrent requests.
  bool register_cb = true;
  if (!register_cpp_ && !register_c_) {
    register_cb = false;
  }

  // call pipe_mgr function to register internal counter-callback
  auto *pipeMgr = PipeMgrIntf::getInstance(*this->session_);
  dev_target_t dev_tgt_pipe_mgr = {dev_tgt_.dev_id, dev_tgt_.pipe_id};
  pipe_status_t pm_status = PIPE_UNEXPECTED;

  if (direct) {
    pm_status = pipeMgr->pipeStfulDirectDatabaseSync(
        session_->sessHandleGet(),
        dev_tgt_pipe_mgr,
        table_->tablePipeHandleGet(),
        register_cb == true
            ? bfRtOperationsInternalCallback<BfRtRegisterSyncCb,
                                             bf_rt_register_sync_cb>
            : nullptr,
        register_state_pool->getItem(state_obj_idx));
  } else {
    pm_status = pipeMgr->pipeStfulDatabaseSync(
        session_->sessHandleGet(),
        dev_tgt_pipe_mgr,
        table_->tablePipeHandleGet(),
        register_cb == true
            ? bfRtOperationsInternalCallback<BfRtRegisterSyncCb,
                                             bf_rt_register_sync_cb>
            : nullptr,
        register_state_pool->getItem(state_obj_idx));
  }

  if (PIPE_SUCCESS != pm_status || !(register_cb)) {
    // The operation state is not needed for failed
    // or for synchronous call.
    // The caller should take care of the cookie memory.
    LOG_DBG("%s:%d %s Table state %zu not needed%s. Pipe Manager rc=%d",
            __func__,
            __LINE__,
            table_->table_name_get().c_str(),
            state_obj_idx,
            (register_cb) ? "" : " without callback",
            pm_status);
    register_state_pool->stateTableOperationsReset(state_obj_idx);
  }

  if (PIPE_SUCCESS != pm_status) {
    LOG_ERROR("%s:%d %s Pipe Manager error=%d",
              __func__,
              __LINE__,
              table_->table_name_get().c_str(),
              pm_status);
    return BF_INTERNAL_ERROR;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTableOperationsImpl::counterSyncExecute() const {
  // If table is MAT then call direct sync else if counter
  // table then idirect sync
  BfRtTable::TableType table_type;
  table_->tableTypeGet(&table_type);
  bool direct = false;
  if (table_type == BfRtTable::TableType::MATCH_DIRECT ||
      table_type == BfRtTable::TableType::MATCH_INDIRECT ||
      table_type == BfRtTable::TableType::MATCH_INDIRECT_SELECTOR) {
    direct = true;
  } else if (table_type == BfRtTable::TableType::COUNTER) {
    direct = false;
  } else {
    LOG_ERROR("%s:%d %s Invalid function for this table",
              __func__,
              __LINE__,
              table_->table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }
  // get the state and update it
  auto prog_name = table_->programNameGet();
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt_.dev_id, prog_name);
  if (device_state == nullptr) {
    LOG_ERROR("%s:%d device state not found for device %d",
              __func__,
              __LINE__,
              dev_tgt_.dev_id);
    return BF_OBJECT_NOT_FOUND;
  }
  auto counter_state_pool =
      device_state->operationsCounterState.getObjState(table_->table_id_get());

  // If the previous operation holds the table state longer, then consider
  // its state as obsolete and override it by this new operation state.
  // TODO: make this timeout a class property with get/set methods.
  int counter_grace_sec = operation_default_grace_sec;

  // Maximum number of simultaneous counter sync requests per table.
  // TODO: make it a class property with get/set methods.
  std::size_t counter_max_states = operation_default_max_table_op_states;

  // Maximum number of callback servicing threads.
  // TODO: make it a class property with get/set methods.
  std::size_t counter_max_threads = operation_default_max_callback_threads;

  std::size_t state_obj_idx = 0;
  bool got_state =
      counter_state_pool->stateTableOperationsRetain(counter_cpp_,
                                                     counter_c_,
                                                     cookie_,
                                                     dev_tgt_,
                                                     counter_grace_sec,
                                                     counter_max_states,
                                                     counter_max_threads,
                                                     state_obj_idx);
  if (!got_state) {
    LOG_TRACE(
        "%s:%d %s Too many table operations are in progress, "
        "try to repeat later.",
        __func__,
        __LINE__,
        table_->table_name_get().c_str());
    return BF_EAGAIN;
  }

  // If no cb has been registered, then dont register BFRT cb with
  // pipe_mgr
  // We still want to hold an operation state object of a synchronous call
  // at BFRT to count it as active together with other concurrent requests.
  bool counter_cb = true;
  if (!counter_cpp_ && !counter_c_) {
    counter_cb = false;
  }

  // call pipe_mgr function to counter internal counter-callback
  auto *pipeMgr = PipeMgrIntf::getInstance(*this->session_);
  dev_target_t dev_tgt_pipe_mgr = {dev_tgt_.dev_id, dev_tgt_.pipe_id};
  pipe_status_t pm_status = PIPE_UNEXPECTED;

  if (direct) {
    pm_status = pipeMgr->pipeMgrDirectStatDatabaseSync(
        session_->sessHandleGet(),
        dev_tgt_pipe_mgr,
        table_->tablePipeHandleGet(),
        counter_cb == true
            ? bfRtOperationsInternalCallback<BfRtCounterSyncCb,
                                             bf_rt_counter_sync_cb>
            : nullptr,
        counter_state_pool->getItem(state_obj_idx));
  } else {
    pm_status = pipeMgr->pipeMgrStatDatabaseSync(
        session_->sessHandleGet(),
        dev_tgt_pipe_mgr,
        table_->tablePipeHandleGet(),
        counter_cb == true
            ? bfRtOperationsInternalCallback<BfRtCounterSyncCb,
                                             bf_rt_counter_sync_cb>
            : nullptr,
        counter_state_pool->getItem(state_obj_idx));
  }

  if (PIPE_SUCCESS != pm_status || !(counter_cb)) {
    // The operation state is not needed for failed
    // or for synchronous call.
    // The caller should take care of the cookie memory.
    LOG_DBG("%s:%d %s Table state %zu not needed%s. Pipe Manager rc=%d",
            __func__,
            __LINE__,
            table_->table_name_get().c_str(),
            state_obj_idx,
            (counter_cb) ? "" : " without callback",
            pm_status);
    counter_state_pool->stateTableOperationsReset(state_obj_idx);
  }

  if (PIPE_SUCCESS != pm_status) {
    LOG_ERROR("%s:%d %s Pipe Manager error=%d",
              __func__,
              __LINE__,
              table_->table_name_get().c_str(),
              pm_status);
    return BF_INTERNAL_ERROR;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTableOperationsImpl::hitStateUpdateExecute() const {
  // If table is MAT then call else error
  BfRtTable::TableType table_type;
  table_->tableTypeGet(&table_type);
  if (!(table_type == BfRtTable::TableType::MATCH_DIRECT ||
        table_type == BfRtTable::TableType::MATCH_INDIRECT ||
        table_type == BfRtTable::TableType::MATCH_INDIRECT_SELECTOR)) {
    LOG_ERROR("%s:%d %s Invalid function for this table",
              __func__,
              __LINE__,
              table_->table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }
  // get the state and update it
  auto prog_name = table_->programNameGet();
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt_.dev_id, prog_name);
  if (device_state == nullptr) {
    LOG_ERROR("%s:%d device state not found for device %d",
              __func__,
              __LINE__,
              dev_tgt_.dev_id);
    return BF_OBJECT_NOT_FOUND;
  }
  auto hit_state_pool =
      device_state->operationsHitStateState.getObjState(table_->table_id_get());

  // If the previous operation holds the table state longer, then consider
  // its state as obsolete and override it by this new operation state.
  // TODO: make this timeout a class property with get/set methods.
  int hit_grace_sec = operation_default_grace_sec;

  // Maximum number of simultaneous hit state sync requests per table.
  // TODO: make it a class property with get/set methods.
  std::size_t hit_state_max_states = operation_default_max_table_op_states;

  // Maximum number of callback servicing threads.
  // TODO: make it a class property with get/set methods.
  std::size_t hit_state_max_threads = operation_default_max_callback_threads;

  std::size_t state_obj_idx = 0;
  bool got_state =
      hit_state_pool->stateTableOperationsRetain(hit_state_cpp_,
                                                 hit_state_c_,
                                                 cookie_,
                                                 dev_tgt_,
                                                 hit_grace_sec,
                                                 hit_state_max_states,
                                                 hit_state_max_threads,
                                                 state_obj_idx);
  if (!got_state) {
    LOG_TRACE(
        "%s:%d %s Too many table operations are in progress, "
        "try to repeat later.",
        __func__,
        __LINE__,
        table_->table_name_get().c_str());
    return BF_EAGAIN;
  }

  // call pipe_mgr function to hitState internal hitState-callback
  auto *pipeMgr = PipeMgrIntf::getInstance(*this->session_);
  dev_target_t dev_tgt_pipe_mgr = {dev_tgt_.dev_id, dev_tgt_.pipe_id};

  pipe_status_t pm_status = pipeMgr->pipeMgrIdleTimeUpdateHitState(
      session_->sessHandleGet(),
      dev_tgt_pipe_mgr.device_id,
      table_->tablePipeHandleGet(),
      bfRtOperationsInternalCallback<BfRtHitStateUpdateCb,
                                     bf_rt_hit_state_update_cb>,
      hit_state_pool->getItem(state_obj_idx));

  if (PIPE_SUCCESS != pm_status || !(hit_state_cpp_ || hit_state_c_)) {
    // The operation state is not needed on error, or without callbacks.
    // The Caller should take care of the cookie memory.
    LOG_DBG("%s:%d %s Table state %zu not needed. Pipe Manager rc=%d",
            __func__,
            __LINE__,
            table_->table_name_get().c_str(),
            state_obj_idx,
            pm_status);
    hit_state_pool->stateTableOperationsReset(state_obj_idx);
  }

  if (PIPE_SUCCESS != pm_status) {
    LOG_ERROR("%s:%d %s Pipe Manager error=%d",
              __func__,
              __LINE__,
              table_->table_name_get().c_str(),
              pm_status);
    return BF_INTERNAL_ERROR;
  }

  return BF_SUCCESS;
}

}  // namespace bfrt
