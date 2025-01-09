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


#ifndef _TDI_PRE_STATE_HPP
#define _TDI_PRE_STATE_HPP

#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "tdi_mc_mgr_intf.hpp"

namespace tdi {

class TdiPREStateObj {
 public:
  enum class PREStateIdType {
    MULTICAST_MGID = 0,     // MGID
    MULTICAST_NODE_ID = 1,  // Node id
    MULTICAST_ECMP_ID = 2,  // ECMP id
  };

  tdi_status_t statePREIdAdd(const PREStateIdType &id_type,
                             const tdi_id_t &id,
                             const tdi_id_t &hdl);

  tdi_status_t statePREIdDel(const PREStateIdType &id_type, const tdi_id_t &id);

  bool statePREIdExists(const PREStateIdType &id_type,
                        const tdi_id_t &id) const;

  bool statePREHdlExists(const PREStateIdType &id_type,
                         const tdi_id_t &hdl) const;

  tdi_status_t statePREIdGet(const PREStateIdType &id_type,
                             const tdi_id_t &hdl,
                             tdi_id_t *id) const;

  tdi_status_t statePREHdlGet(const PREStateIdType &id_type,
                              const tdi_id_t &id,
                              tdi_id_t *hdl) const;

  tdi_status_t statePREIdGetFirst(const PREStateIdType &id_type,
                                  tdi_id_t *first_id,
                                  tdi_id_t *first_hdl) const;

  tdi_status_t statePREIdGetNext(const PREStateIdType &id_type,
                                 const tdi_id_t &id,
                                 tdi_id_t *next_id,
                                 tdi_id_t *next_hdl) const;

 private:
  mutable std::mutex state_lock;

  // Need to maintain mappings between IDs and handles
  // for multicast objects (mgid, node, ecmp) and vice versa. This
  // has to be maintained globally and not per table basis as this
  // mapping needs to be accessed across tables (e.g. mgid table
  // needs both node id <---> node handle mapping and
  // ecmp <---> ecmp handle mapping).
  // All IDs to handles mapping are ordered maps to support entry
  // get first. Others are unordered maps to have better performance
  // with really large scale PRE objects

  // MGID to MGID  handle map
  std::map<bf_mc_grp_id_t, bf_mc_mgrp_hdl_t> mgid_to_hdl_map;
  // Node id to node handle map
  std::map<tdi_id_t, bf_mc_node_hdl_t> node_id_to_hdl_map;
  // ECMP id to ECMP handle map
  std::map<tdi_id_t, bf_mc_ecmp_hdl_t> ecmp_id_to_hdl_map;

  // MGID handle to MGID map
  std::unordered_map<bf_mc_mgrp_hdl_t, bf_mc_grp_id_t> mgid_hdl_to_mgid_map;
  // Node handle to node id map
  std::unordered_map<bf_mc_node_hdl_t, tdi_id_t> node_hdl_to_id_map;
  // ECMP handle to ECMP id map
  std::unordered_map<bf_mc_ecmp_hdl_t, tdi_id_t> ecmp_hdl_to_id_map;
};

}  // namespace tdi

#endif  // _TDI_PRE_STATE_HPP
