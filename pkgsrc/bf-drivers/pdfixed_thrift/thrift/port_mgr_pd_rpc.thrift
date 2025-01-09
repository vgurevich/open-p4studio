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

include "res.thrift"

namespace py port_mgr_pd_rpc
namespace cpp port_mgr_pd_rpc
namespace c_glib port_mgr_pd_rpc

exception InvalidPortMgrOperation {
  1:i32 code
}

service port_mgr {
  # Port Mgr APIs
  void port_mgr_mtu_set(1: i32 dev_id, 2: i32 port_id, 3: i32 tx_mtu, 4: i32 rx_mtu) throws (1:InvalidPortMgrOperation ouch);
}
