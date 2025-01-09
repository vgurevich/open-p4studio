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
#include <pipe_mgr/pipe_mgr_intf.h>

#ifdef __cplusplus
}
#endif

#include <memory>
#include <string>
#include <map>
#include <vector>
#include <cstring>

#include <tdi/tdi_session.hpp>
#include <tdi/tdi_table.hpp>
#include <tdi_common/tdi_init_impl.hpp>
#include <tdi_common/tdi_utils.hpp>
#include "tdi_learn_impl.hpp"

namespace tdi {

TdiLearnField::TdiLearnField(tdi_id_t id, size_t s, std::string n, size_t off)
    : field_id(id), size(s), name(n), is_ptr(false), offset(off) {
  if (s > 64) {
    is_ptr = true;
  }
}

TdiLearnDataObj::TdiLearnDataObj(const TdiLearnObj *learn_obj, void *entries)
    : TdiTableDataObj(nullptr), learn(learn_obj), buffer(entries), size() {
  if (entries) {
    size = learn->getLearnMsgSize();
  }
}

tdi_status_t TdiLearnDataObj::getValue(const tdi_id_t &field_id,
                                       uint64_t *val) const {
  auto learn_field = learn->getLearnField(field_id);
  // check if the field_id exists or not
  if (!learn_field) {
    return TDI_OBJECT_NOT_FOUND;
  }
  // check if the size of the field is < 64 Bits, else return not-supported
  if (learn_field->getSize() > 64) {
    LOG_ERROR("%s:%d API not supported for this field", __func__, __LINE__);
    return TDI_NOT_SUPPORTED;
  }
  // size to copy will be byte padded
  size_t learn_field_size = (learn_field->getSize() + 7) / 8;
  uint8_t *read_ptr = static_cast<uint8_t *>(buffer);
  read_ptr += learn_field->getOffset();
  TdiEndiannessHandler::toHostOrder(learn_field_size, read_ptr, val);
  return TDI_SUCCESS;
}

tdi_status_t TdiLearnDataObj::getValue(const tdi_id_t &field_id,
                                       const size_t &size_out,
                                       uint8_t *val) const {
  auto learn_field = learn->getLearnField(field_id);
  // check if the field_id exists or not
  if (!learn_field) {
    return TDI_OBJECT_NOT_FOUND;
  }
  // check if the passed in size == field_size, else throw an error
  auto field_size = learn_field->getSize();
  field_size = (field_size + 7) / 8;
  if (size_out != field_size) {
    LOG_ERROR("%s:%d Field size = %zu, passed in size = %zu",
              __func__,
              __LINE__,
              field_size,
              size_out);
    return TDI_INVALID_ARG;
  }
  uint8_t *read_ptr = static_cast<uint8_t *>(buffer);
  read_ptr += learn_field->getOffset();
  std::memcpy(val, read_ptr, size_out);
  return TDI_SUCCESS;
}

tdi_status_t TdiLearnDataObj::getParent(
    const TdiTable ** /* table_ret*/) const {
  LOG_ERROR(
      "%s:%d Cannot get table object from a Learn Data", __func__, __LINE__);
  return TDI_NOT_SUPPORTED;
}

tdi_status_t TdiLearnDataObj::getParent(const TdiLearn **learn_ret) const {
  *learn_ret = learn;
  return TDI_SUCCESS;
}
// convert pipe_flow_lrn_msg to tdi field-list and call
// cb registered with tdi
pipe_status_t tdiLearnPipeMgrInternalCb(pipe_sess_hdl_t sess_hdl,
                                        pipe_flow_lrn_msg_t *pipe_flow_lrn_msg,
                                        void *callback_fn_cookie) {
  // get information from the learn_state we had sent in the registration
  auto learn_state = static_cast<TdiStateLearn *>(callback_fn_cookie);
  auto t = learn_state->stateLearnGet();
  auto learn_obj = std::get<0>(t);
  auto session_obj = std::get<1>(t).lock();
  auto callback_cpp = std::get<2>(t);
  auto callback_c = std::get<3>(t);
  auto cookie = std::get<4>(t);

  if (!callback_cpp && !callback_c) {
    LOG_ERROR("%s:%d No Callback fundtion found", __func__, __LINE__);
    return PIPE_OBJ_NOT_FOUND;
  }
  // session sanity checks
  // check if the session found in state is == to the session_hdl passed by
  // pipe_mgr. If not, then it was changed midway. We cannot go ahead anymore
  if (session_obj == nullptr) {
    LOG_ERROR("%s:%d Session was destroyed", __func__, __LINE__);
    return PIPE_SESSION_NOT_FOUND;
  }
  if (sess_hdl != session_obj->sessHandleGet()) {
    LOG_ERROR("%s:%d Looks like session handle was changed from %d to %d",
              __func__,
              __LINE__,
              sess_hdl,
              session_obj->sessHandleGet());
    return PIPE_SESSION_NOT_FOUND;
  }

  // Now create TdiLearnDataObj objects for every entry and send to the
  // callback
  std::vector<std::unique_ptr<TdiLearnData>> lrn_data_list;
  size_t current_offset = 0;
  for (uint32_t i = 0; i < pipe_flow_lrn_msg->num_entries; i++) {
    uint8_t *entry_ptr =
        static_cast<uint8_t *>(pipe_flow_lrn_msg->entries) + current_offset;
    current_offset += learn_obj->getLearnMsgSize();
    std::unique_ptr<TdiLearnData> data(
        new TdiLearnDataObj(learn_obj, entry_ptr));
    lrn_data_list.push_back(std::move(data));
  }

  const tdi_target_t tdi_tgt = {
      pipe_flow_lrn_msg->dev_tgt.device_id,
      pipe_flow_lrn_msg->dev_tgt.dev_pipe_id,
      TDI_DEV_DIR_INGRESS /* direction : don't care */,
      0xff /* parser_id : don't care */};
  if (callback_cpp) {
    return static_cast<pipe_status_t>(
        callback_cpp(tdi_tgt,
                     session_obj,
                     std::move(lrn_data_list),
                     reinterpret_cast<tdi_learn_msg_hdl *>(pipe_flow_lrn_msg),
                     cookie));
  } else {
    // Make a vector of pointers to tdi_learn_data_hdl
    // which we will make from our vector of unique_ptrs
    std::vector<tdi_learn_data_hdl *> lrn_data_c_list;
    for (auto &item : lrn_data_list) {
      lrn_data_c_list.push_back(
          reinterpret_cast<tdi_learn_data_hdl *>(item.release()));
    }
    return static_cast<pipe_status_t>(
        callback_c(&tdi_tgt,
                   reinterpret_cast<const tdi_session_hdl *>(session_obj.get()),
                   &lrn_data_c_list[0],
                   lrn_data_c_list.size(),
                   reinterpret_cast<tdi_learn_msg_hdl *>(pipe_flow_lrn_msg),
                   cookie));
  }
}

TdiLearnObj::TdiLearnObj(
    const std::string &program_name,
    tdi_id_t l_id,
    pipe_fld_lst_hdl_t f_hdl,
    std::string n,
    std::map<tdi_id_t, std::unique_ptr<TdiLearnField>> l_field_map)
    : prog_name(program_name),
      learn_id(l_id),
      flow_lrn_fld_lst_hdl(f_hdl),
      learn_name(n),
      lrn_fields(std::move(l_field_map)),
      learn_msg_size(0) {
  // Calculate the size of an individual learn message from a
  // pipe_flow_lrn_msg_t.
  // Each field's bitwidth is rounded up to the next byte.  If a field is three
  // bytes it is rounded up to be a four byte field since there is no uint24_t.
  // Fields over four bytes are assumed to be byte arrays and not uint64_ts.
  for (const auto &lrn_field : lrn_fields) {
    auto size = lrn_field.second->getSize();
    size = (size + 7) / 8;
    if (size == 3) size = 4;
    learn_msg_size += size;
  }
}

tdi_status_t TdiLearnObj::tdiLearnCallbackRegister(
    const std::shared_ptr<TdiSession> session,
    const tdi_target_t &dev_tgt,
    const tdiCbFunction &callback_fn,
    const void *cookie) const {
  return tdiLearnCallbackRegisterHelper(
      session, dev_tgt, callback_fn, nullptr, cookie);
}

tdi_status_t TdiLearnObj::tdiLearnCallbackRegisterHelper(
    const std::shared_ptr<TdiSession> session,
    const tdi_target_t &dev_tgt,
    const tdiCbFunction &callback_cpp,
    const tdi_cb_function callback_c,
    const void *cookie) const {
  // Get learn state and update it
  auto device_state =
      TdiDevMgrImpl::tdiDeviceStateGet(dev_tgt.dev_id, prog_name);
  if (device_state == nullptr) {
    // TODO Assert ?
    return TDI_OBJECT_NOT_FOUND;
  }
  // Update the state
  auto learn_state = device_state->learnState.getObjState(learn_id);
  learn_state->stateLearnSet(this, session, callback_cpp, callback_c, cookie);

  // call pipe_mgr to register
  return pipe_mgr_lrn_digest_notification_register(session->sessHandleGet(),
                                                   dev_tgt.dev_id,
                                                   flow_lrn_fld_lst_hdl,
                                                   tdiLearnPipeMgrInternalCb,
                                                   learn_state.get());
}

tdi_status_t TdiLearnObj::tdiLearnCallbackDeregister(
    const std::shared_ptr<TdiSession> session,
    const tdi_target_t &dev_tgt) const {
  // Get learn state and update it
  auto device_state =
      TdiDevMgrImpl::tdiDeviceStateGet(dev_tgt.dev_id, prog_name);
  if (device_state == nullptr) {
    // TODO Assert ?
    return TDI_OBJECT_NOT_FOUND;
  }
  auto learn_state = device_state->learnState.getObjState(learn_id);
  learn_state->stateLearnReset();

  // deregister the tdi cb from pipe_mgr
  return pipe_mgr_lrn_digest_notification_deregister(
      session->sessHandleGet(), dev_tgt.dev_id, flow_lrn_fld_lst_hdl);
}

tdi_status_t TdiLearnObj::learnIdGet(tdi_id_t *id) const {
  *id = learn_id;
  return TDI_SUCCESS;
}

tdi_status_t TdiLearnObj::learnNameGet(std::string *name) const {
  *name = learn_name;
  return TDI_SUCCESS;
}

tdi_status_t TdiLearnObj::tdiLearnNotifyAck(
    const std::shared_ptr<TdiSession> session,
    tdi_learn_msg_hdl *learn_msg_hdl) const {
  return pipe_mgr_flow_lrn_notify_ack(
      session->sessHandleGet(),
      flow_lrn_fld_lst_hdl,
      reinterpret_cast<pipe_flow_lrn_msg_t *>(learn_msg_hdl));
}

tdi_status_t TdiLearnObj::learnFieldIdListGet(std::vector<tdi_id_t> *id) const {
  if (id == nullptr) {
    LOG_ERROR("%s:%d Please allocate memory for out param", __func__, __LINE__);
    return TDI_INVALID_ARG;
  }
  for (auto const &item : lrn_fields) {
    id->push_back(item.second->getFieldId());
  }
  return TDI_SUCCESS;
}

tdi_status_t TdiLearnObj::learnFieldIdGet(const std::string &name,
                                          tdi_id_t *field_id) const {
  if (field_id == nullptr) {
    LOG_ERROR("%s:%d Please pass mem allocated out param", __func__, __LINE__);
    return TDI_INVALID_ARG;
  }
  auto found = std::find_if(
      lrn_fields.begin(),
      lrn_fields.end(),
      [&name](const std::pair<const tdi_id_t, std::unique_ptr<TdiLearnField>>
                  &map_item) { return (name == map_item.second->getName()); });
  if (found != lrn_fields.end()) {
    *field_id = (*found).second->getFieldId();
    return TDI_SUCCESS;
  }
  LOG_ERROR("%s:%d Field \"%s\" not found", __func__, __LINE__, name.c_str());
  return TDI_OBJECT_NOT_FOUND;
}

tdi_status_t TdiLearnObj::learnFieldSizeGet(const tdi_id_t &field_id,
                                            size_t *size) const {
  auto field = getLearnField(field_id);
  if (field == nullptr) {
    return TDI_OBJECT_NOT_FOUND;
  }
  *size = field->getSize();
  return TDI_SUCCESS;
}

tdi_status_t TdiLearnObj::learnFieldIsPtrGet(const tdi_id_t &field_id,
                                             bool *is_ptr) const {
  auto field = getLearnField(field_id);
  if (field == nullptr) {
    return TDI_OBJECT_NOT_FOUND;
  }
  *is_ptr = field->getIsPtr();
  return TDI_SUCCESS;
}

tdi_status_t TdiLearnObj::learnFieldNameGet(const tdi_id_t &field_id,
                                            std::string *name) const {
  auto field = getLearnField(field_id);
  if (field == nullptr) {
    return TDI_OBJECT_NOT_FOUND;
  }
  *name = field->getName();
  return TDI_SUCCESS;
}

const TdiLearnField *TdiLearnObj::getLearnField(
    const tdi_id_t &field_id) const {
  if (lrn_fields.find(field_id) == lrn_fields.end()) {
    LOG_ERROR("%s:%d Field-Id %d not found", __func__, __LINE__, field_id);
    return nullptr;
  }
  return lrn_fields.at(field_id).get();
}

}  // namespace tdi
