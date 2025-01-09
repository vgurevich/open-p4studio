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


#ifndef _TDI_TBL_ATTRIBUTES_STATE_HPP
#define _TDI_TBL_ATTRIBUTES_STATE_HPP

#ifdef __cplusplus
extern "C" {
#endif
#include "pipe_mgr/pipe_mgr_intf.h"
#include <tdi/common/tdi_defs.h>
#ifdef __cplusplus
}
#endif

#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <tuple>

#include <tdi/common/tdi_table.hpp>
#include <tdi/common/tdi_attributes.hpp>
#include <tdi/common/tdi_utils.hpp>

#include <tdi/arch/tna/tna_defs.h>

#include <tdi_tofino/tdi_tofino_defs.h>
#include <tdi_tofino/tdi_tofino_attributes.hpp>
#include <tdi_tofino/c_frontend/tdi_tofino_attributes.h>

#include <tdi_p4/tdi_p4_table_impl.hpp>

#include <tdi_common/tdi_context_info.hpp>

namespace tdi {
namespace tna {
namespace tofino {

void tdiIdleTmoExpiryInternalCb(bf_dev_id_t dev_id,
                                pipe_mat_ent_hdl_t entry_hdl,
                                pipe_idle_time_hit_state_e hs,
                                pipe_tbl_match_spec_t *match_spec_allocated,
                                void *cookie);

pipe_status_t selUpdatePipeMgrInternalCb(pipe_sess_hdl_t sess_hdl,
                                         dev_target_t dev_tgt,
                                         void *cookie,
                                         pipe_sel_grp_hdl_t sel_grp_hdl,
                                         pipe_adt_ent_hdl_t adt_ent_hdl,
                                         int logical_table_index,
                                         bool is_add);

class StateTableAttributesAging {
 public:
  StateTableAttributesAging()
      : enabled_(false),
        callback_c_((tdi_idle_tmo_expiry_cb) nullptr),
        table_(nullptr),
        cookie_(nullptr){};
  StateTableAttributesAging(bool enable,
                            IdleTmoExpiryCb callback_cpp,
                            tdi_idle_tmo_expiry_cb callback_c,
                            const tdi::Table *table,
                            void *cookie)
      : enabled_(enable),
        callback_cpp_(callback_cpp),
        callback_c_(callback_c),
        table_(table),
        cookie_(cookie){};
  void stateTableAttributesAgingSet(bool enabled,
                                    IdleTmoExpiryCb callback_cpp,
                                    tdi_idle_tmo_expiry_cb callback_c,
                                    const tdi::Table *table,
                                    void *cookie);
  void stateTableAttributesAgingReset();
  std::tuple<bool,
             IdleTmoExpiryCb,
             tdi_idle_tmo_expiry_cb,
             const tdi::Table *,
             void *>
  stateTableAttributesAgingGet();

 private:
  bool enabled_;
  IdleTmoExpiryCb callback_cpp_;
  tdi_idle_tmo_expiry_cb callback_c_;
  const tdi::Table *table_;
  void *cookie_;
};

class StateSelUpdateCb {
 public:
  StateSelUpdateCb()
      : enable_(false),
        sel_table_(nullptr),
        c_callback_fn_((tdi_selector_table_update_cb) nullptr),
        cookie_(nullptr){};
  StateSelUpdateCb(bool enable,
                   const tdi::Table *sel_table,
                   std::weak_ptr<Session> session_obj,
                   const SelUpdateCb &cpp_callback_fn,
                   const tdi_selector_table_update_cb &c_callback_fn,
                   const void *cookie)
      : enable_(enable),
        sel_table_(sel_table),
        session_obj_(session_obj),
        cpp_callback_fn_(cpp_callback_fn),
        c_callback_fn_(c_callback_fn),
        cookie_(cookie){};

  void reset();

  std::tuple<bool,
             const tdi::Table *,
             const std::weak_ptr<Session>,
             SelUpdateCb,
             tdi_selector_table_update_cb,
             const void *>
  stateGet();

 private:
  bool enable_;
  const tdi::Table *sel_table_;
  std::weak_ptr<Session> session_obj_;
  SelUpdateCb cpp_callback_fn_;
  tdi_selector_table_update_cb c_callback_fn_;
  const void *cookie_;
};

class StateTableAttributes {
 public:
  StateTableAttributes(tdi_id_t id)
      : table_id_{id},
        entry_scope(TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_ALL_PIPELINES){};
  StateTableAttributesAging &getAgingAttributeObj() {
    std::lock_guard<std::mutex> lock(state_lock);
    return aging_attributes;
  }

  StateSelUpdateCb &getSelUpdateCbObj() {
    std::lock_guard<std::mutex> lock(state_lock);
    return sel_update_cb;
  }

  void setAgingAttributeObj(const StateTableAttributesAging &aging) {
    std::lock_guard<std::mutex> lock(state_lock);
    aging_attributes = aging;
  }

  void setSelUpdateCbObj(const StateSelUpdateCb &sel_update_cb_) {
    std::lock_guard<std::mutex> lock(state_lock);
    sel_update_cb = sel_update_cb_;
  }

  void getSelUpdateCbObj(StateSelUpdateCb *sel_update_cb_) {
    std::lock_guard<std::mutex> lock(state_lock);
    *sel_update_cb_ = sel_update_cb;
  }

  void resetSelUpdateCbObj() {
    std::lock_guard<std::mutex> lock(state_lock);
    sel_update_cb.reset();
  }

  void agingAttributesReset() {
    std::lock_guard<std::mutex> lock(state_lock);
    aging_attributes.stateTableAttributesAgingReset();
  }

  void setEntryScope(const tdi_tofino_attributes_entry_scope_e &scope) {
    std::lock_guard<std::mutex> lock(state_lock);
    entry_scope = scope;
  }

  tdi_tofino_attributes_entry_scope_e getEntryScope() {
    std::lock_guard<std::mutex> lock(state_lock);
    return entry_scope;
  }

 private:
  tdi_id_t table_id_;
  std::mutex state_lock;
  StateTableAttributesAging aging_attributes;
  StateSelUpdateCb sel_update_cb;
  tdi_tofino_attributes_entry_scope_e entry_scope;
};
}  // namespace tofino
}  // namespace tna
}  // namespace tdi
#endif  // _TDI_TBL_ATTRIBUTES_STATE_HPP
