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

 ******************************************************************************/
#ifndef _TDI_LEARN_IMPL_HPP
#define _TDI_LEARN_IMPL_HPP

#ifdef __cplusplus
extern "C" {
#endif
#include <pipe_mgr/pipe_mgr_intf.h>

#ifdef __cplusplus
}
#endif

#include <string>
#include <vector>
#include <mutex>
#include <map>
#include <memory>
#include <functional>

#include <tdi/tdi_learn.hpp>
#include <tdi/tdi_learn.h>
#include "../tdi_common/tdi_table_impl.hpp"

namespace tdi {

class TdiLearnObj;

class TdiLearnField {
 public:
  TdiLearnField(tdi_id_t id, size_t s, std::string n, size_t off);
  ~TdiLearnField() = default;

  tdi_id_t getFieldId() const { return field_id; };
  const std::string &getName() const { return name; };
  const size_t &getSize() const { return size; };
  const size_t &getOffset() const { return offset; };
  const bool &getIsPtr() const { return is_ptr; };

 private:
  tdi_id_t field_id;
  size_t size;
  std::string name;
  bool is_ptr;
  size_t offset;
};

// We inherit from TdiTableDataObj because we want to
// reuse the setValue and getValue "not-implemented"
// versions
class TdiLearnDataObj : public TdiTableDataObj {
 public:
  TdiLearnDataObj(const TdiLearnObj *learn_obj, void *entries);

  tdi_status_t getValue(const tdi_id_t &field_id, uint64_t *val) const;

  tdi_status_t getValue(const tdi_id_t &field_id,
                        const size_t &size,
                        uint8_t *val) const;
  tdi_status_t getParent(const TdiTable **table_ret) const;
  tdi_status_t getParent(const TdiLearn **learn_ret) const;

 private:
  // Backpointer to the learn obj this data obj belongs to
  const TdiLearnObj *learn;
  void *buffer;
  size_t size;
};

class TdiLearnObj : public TdiLearn {
 public:
  TdiLearnObj(const std::string &program_name,
              tdi_id_t l_id,
              pipe_fld_lst_hdl_t f_hdl,
              std::string n,
              std::map<tdi_id_t, std::unique_ptr<TdiLearnField>> l_field_map);
  ~TdiLearnObj() = default;

  tdi_status_t tdiLearnCallbackRegister(
      const std::shared_ptr<TdiSession> session,
      const tdi_target_t &dev_tgt,
      const tdiCbFunction &callback_fn,
      const void *cookie) const;
  tdi_status_t tdiLearnCallbackRegisterHelper(
      const std::shared_ptr<TdiSession> session,
      const tdi_target_t &dev_tgt,
      const tdiCbFunction &callback_cpp,
      const tdi_cb_function callback_c,
      const void *cookie) const;

  tdi_status_t tdiLearnCallbackDeregister(
      const std::shared_ptr<TdiSession> session,
      const tdi_target_t &dev_tgt) const;

  tdi_status_t tdiLearnNotifyAck(const std::shared_ptr<TdiSession> session,
                                 tdi_learn_msg_hdl *const learn_msg_hdl) const;

  tdi_status_t learnIdGet(tdi_id_t *id) const;

  tdi_status_t learnNameGet(std::string *name) const;

  tdi_status_t learnFieldIdListGet(std::vector<tdi_id_t> *id) const;

  tdi_status_t learnFieldIdGet(const std::string &name,
                               tdi_id_t *field_id) const;

  tdi_status_t learnFieldSizeGet(const tdi_id_t &field_id, size_t *size) const;

  tdi_status_t learnFieldIsPtrGet(const tdi_id_t &field_id, bool *is_ptr) const;

  tdi_status_t learnFieldNameGet(const tdi_id_t &field_id,
                                 std::string *name) const;

  const std::string &getName() const { return learn_name; };

  // hidden functions
  const TdiLearnField *getLearnField(const tdi_id_t &field_id) const;
  size_t getLearnMsgSize() const { return learn_msg_size; };

  uint32_t learnFieldListSize() const { return lrn_fields.size(); };

 private:
  const pipe_fld_lst_hdl_t &getLearnHandle() { return flow_lrn_fld_lst_hdl; };

  // Name of the program to which this learn obj belongs
  std::string prog_name;
  tdi_id_t learn_id;
  pipe_fld_lst_hdl_t flow_lrn_fld_lst_hdl;
  std::string learn_name;
  std::map<tdi_id_t, std::unique_ptr<TdiLearnField>> lrn_fields;
  size_t learn_msg_size;

  friend pipe_status_t tdiLearnPipeMgrInternalCb(
      pipe_sess_hdl_t sess_hdl,
      pipe_flow_lrn_msg_t *pipe_flow_lrn_msg,
      void *callback_fn_cookie);
};

}  // namespace tdi

#endif
