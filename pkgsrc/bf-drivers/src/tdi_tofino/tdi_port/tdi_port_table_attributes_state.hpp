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


#ifndef _TDI_PORT_TBL_ATTRIBUTES_STATE_HPP
#define _TDI_PORT_TBL_ATTRIBUTES_STATE_HPP

#include <mutex>
#include <unordered_map>

#ifdef __cplusplus
extern "C" {
#endif
#include "pipe_mgr/pipe_mgr_intf.h"
#include <tdi/common/tdi_defs.h>
#ifdef __cplusplus
}
#endif

#include <tdi/common/tdi_attributes.hpp>
#include <tdi/common/tdi_utils.hpp>

// tofino include
#include <tdi_tofino/tdi_tofino_attributes.hpp>
#include <tdi_tofino/c_frontend/tdi_tofino_attributes.h>

namespace tdi {
namespace tna {
namespace tofino {

int tdiPortStatusChgInternalCb(bf_dev_id_t dev_id,
                               int dev_port,
                               bool port_up,
                               void *cookie);

class StateTableAttributesPort {
 public:
  StateTableAttributesPort(tdi_id_t id) : table_id_(id){};

  void stateTableAttributesPortSet(bool enabled,
                                   PortStatusNotifCb callback_fn,
                                   tdi_port_status_chg_cb callback_c,
                                   const tdi::Table *table,
                                   void *cookie);

  void stateTableAttributesPortReset();
  std::tuple<bool,
             PortStatusNotifCb,
             tdi_port_status_chg_cb,
             const tdi::Table *,
             void *>
  stateTableAttributesPortGet();

 private:
  std::mutex state_lock;
  tdi_id_t table_id_;
  bool enabled_;
  PortStatusNotifCb callback_;
  tdi_port_status_chg_cb callback_c_;
  tdi::Table *table_;
  void *cookie_;
};

}  // namespace tofino
}  // namespace tna
}  // namespace tdi
#endif  // _TDI_PORT_TBL_ATTRIBUTES_STATE_HPP
