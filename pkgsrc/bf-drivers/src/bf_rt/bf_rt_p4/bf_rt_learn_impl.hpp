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


#ifndef _BF_RT_LEARN_IMPL_HPP
#define _BF_RT_LEARN_IMPL_HPP

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

#include <bf_rt/bf_rt_learn.hpp>
#include <bf_rt/bf_rt_learn.h>
#include "../bf_rt_common/bf_rt_table_impl.hpp"

namespace bfrt {

class BfRtLearnObj;

class BfRtLearnField {
 public:
  BfRtLearnField(bf_rt_id_t id, size_t s, std::string n, size_t off);
  ~BfRtLearnField() = default;

  bf_rt_id_t getFieldId() const { return field_id; };
  const std::string &getName() const { return name; };
  const size_t &getSize() const { return size; };
  const size_t &getOffset() const { return offset; };
  const bool &getIsPtr() const { return is_ptr; };

 private:
  bf_rt_id_t field_id;
  size_t size;
  std::string name;
  bool is_ptr;
  size_t offset;
};

// We inherit from BfRtTableDataObj because we want to
// reuse the setValue and getValue "not-implemented"
// versions
class BfRtLearnDataObj : public BfRtTableDataObj {
 public:
  BfRtLearnDataObj(const BfRtLearnObj *learn_obj, void *entries);

  bf_status_t getValue(const bf_rt_id_t &field_id, uint64_t *val) const;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *val) const;
  bf_status_t getParent(const BfRtTable **table_ret) const;
  bf_status_t getParent(const BfRtLearn **learn_ret) const;

 private:
  // Backpointer to the learn obj this data obj belongs to
  const BfRtLearnObj *learn;
  void *buffer;
  size_t size;
};

class BfRtLearnObj : public BfRtLearn {
 public:
  BfRtLearnObj(
      const std::string &program_name,
      bf_rt_id_t l_id,
      pipe_fld_lst_hdl_t f_hdl,
      std::string n,
      std::map<bf_rt_id_t, std::unique_ptr<BfRtLearnField>> l_field_map);
  ~BfRtLearnObj() = default;

  bf_status_t bfRtLearnCallbackRegister(
      const std::shared_ptr<BfRtSession> session,
      const bf_rt_target_t &dev_tgt,
      const bfRtCbFunction &callback_fn,
      const void *cookie) const;
  bf_status_t bfRtLearnCallbackRegisterHelper(
      const std::shared_ptr<BfRtSession> session,
      const bf_rt_target_t &dev_tgt,
      const bfRtCbFunction &callback_cpp,
      const bf_rt_cb_function callback_c,
      const void *cookie) const;

  bf_status_t bfRtLearnCallbackDeregister(
      const std::shared_ptr<BfRtSession> session,
      const bf_rt_target_t &dev_tgt) const;

  bf_status_t bfRtLearnNotifyAck(
      const std::shared_ptr<BfRtSession> session,
      bf_rt_learn_msg_hdl *const learn_msg_hdl) const;

  bf_status_t learnIdGet(bf_rt_id_t *id) const;

  bf_status_t learnNameGet(std::string *name) const;

  bf_status_t learnFieldIdListGet(std::vector<bf_rt_id_t> *id) const;

  bf_status_t learnFieldIdGet(const std::string &name,
                              bf_rt_id_t *field_id) const;

  bf_status_t learnFieldSizeGet(const bf_rt_id_t &field_id, size_t *size) const;

  bf_status_t learnFieldIsPtrGet(const bf_rt_id_t &field_id,
                                 bool *is_ptr) const;

  bf_status_t learnFieldNameGet(const bf_rt_id_t &field_id,
                                std::string *name) const;

  const std::string &getName() const { return learn_name; };

  // hidden functions
  const BfRtLearnField *getLearnField(const bf_rt_id_t &field_id) const;
  size_t getLearnMsgSize() const { return learn_msg_size; };

  uint32_t learnFieldListSize() const { return lrn_fields.size(); };

 private:
  const pipe_fld_lst_hdl_t &getLearnHandle() { return flow_lrn_fld_lst_hdl; };

  // Name of the program to which this learn obj belongs
  std::string prog_name;
  bf_rt_id_t learn_id;
  pipe_fld_lst_hdl_t flow_lrn_fld_lst_hdl;
  std::string learn_name;
  std::map<bf_rt_id_t, std::unique_ptr<BfRtLearnField>> lrn_fields;
  size_t learn_msg_size;

  friend pipe_status_t bfRtLearnPipeMgrInternalCb(
      pipe_sess_hdl_t sess_hdl,
      pipe_flow_lrn_msg_t *pipe_flow_lrn_msg,
      void *callback_fn_cookie);
};

}  // namespace bfrt

#endif
