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

#include <algorithm>

#include <tdi_common/tdi_utils.hpp>

#include "tdi_pre_state.hpp"

namespace tdi {

// PRE state APIs

tdi_status_t TdiPREStateObj::statePREIdAdd(const PREStateIdType &id_type,
                                           const tdi_id_t &id,
                                           const tdi_id_t &hdl) {
  if (this->statePREIdExists(id_type, id)) {
    LOG_ERROR("%s:%d ID %d already exists for PRE state id type %d",
              __func__,
              __LINE__,
              id,
              static_cast<int>(id_type));
    return TDI_ALREADY_EXISTS;
  }

  std::lock_guard<std::mutex> lock(state_lock);

  switch (id_type) {
    case PREStateIdType::MULTICAST_MGID: {
      bf_mc_mgrp_hdl_t mgrp_hdl = static_cast<bf_mc_mgrp_hdl_t>(hdl);
      bf_mc_grp_id_t mgid = static_cast<bf_mc_grp_id_t>(id);
      mgid_to_hdl_map[mgid] = mgrp_hdl;
      mgid_hdl_to_mgid_map[mgrp_hdl] = mgid;
    } break;

    case PREStateIdType::MULTICAST_NODE_ID: {
      bf_mc_node_hdl_t node_hdl = static_cast<bf_mc_node_hdl_t>(hdl);
      node_id_to_hdl_map[id] = node_hdl;
      node_hdl_to_id_map[node_hdl] = id;
    } break;

    case PREStateIdType::MULTICAST_ECMP_ID: {
      bf_mc_ecmp_hdl_t ecmp_hdl = static_cast<bf_mc_ecmp_hdl_t>(hdl);
      ecmp_id_to_hdl_map[id] = ecmp_hdl;
      ecmp_hdl_to_id_map[ecmp_hdl] = id;
    } break;

    default:
      LOG_ERROR("%s:%d Invalid PRE state ID type %d",
                __func__,
                __LINE__,
                static_cast<int>(id_type));
      return TDI_INVALID_ARG;
  }

  return TDI_SUCCESS;
}

tdi_status_t TdiPREStateObj::statePREIdDel(const PREStateIdType &id_type,
                                           const tdi_id_t &id) {
  std::lock_guard<std::mutex> lock(state_lock);

  switch (id_type) {
    case PREStateIdType::MULTICAST_MGID: {
      // Check if the MGID exists
      bf_mc_grp_id_t mgid = static_cast<bf_mc_grp_id_t>(id);
      auto it_mgid_to_hdl = mgid_to_hdl_map.find(mgid);
      if (it_mgid_to_hdl == mgid_to_hdl_map.end()) {
        return TDI_OBJECT_NOT_FOUND;
      }

      // Check if the MGID handle exists
      bf_mc_mgrp_hdl_t mgrp_hdl = it_mgid_to_hdl->second;
      auto it_hdl_to_mgid = mgid_hdl_to_mgid_map.find(mgrp_hdl);
      TDI_ASSERT(it_hdl_to_mgid != mgid_hdl_to_mgid_map.end());

      // Delete the entry from both mappings
      // (mgid<-->hdl and mgrp_hdl <-->id)
      mgid_to_hdl_map.erase(mgid);
      mgid_hdl_to_mgid_map.erase(mgrp_hdl);
    } break;

    case PREStateIdType::MULTICAST_NODE_ID: {
      // Check if the Node ID exists
      auto it_node_id_to_hdl = node_id_to_hdl_map.find(id);
      if (it_node_id_to_hdl == node_id_to_hdl_map.end()) {
        return TDI_OBJECT_NOT_FOUND;
      }

      // Check if the Node handle exists
      bf_mc_node_hdl_t node_hdl = it_node_id_to_hdl->second;
      auto it_node_hdl_to_id = node_hdl_to_id_map.find(node_hdl);
      TDI_ASSERT(it_node_hdl_to_id != node_hdl_to_id_map.end());

      // Delete the entry from both mappings
      // (node_id<-->hdl and node_hdl <-->id)
      node_id_to_hdl_map.erase(id);
      node_hdl_to_id_map.erase(node_hdl);
    } break;

    case PREStateIdType::MULTICAST_ECMP_ID: {
      // Check if the ECMP ID exists
      auto it_ecmp_id_to_hdl = ecmp_id_to_hdl_map.find(id);
      if (it_ecmp_id_to_hdl == ecmp_id_to_hdl_map.end()) {
        return TDI_OBJECT_NOT_FOUND;
      }

      // Check if the ECMP handle exists
      bf_mc_ecmp_hdl_t ecmp_hdl = it_ecmp_id_to_hdl->second;
      auto it_ecmp_hdl_to_id = ecmp_hdl_to_id_map.find(ecmp_hdl);
      TDI_ASSERT(it_ecmp_hdl_to_id != ecmp_hdl_to_id_map.end());

      // Delete the entry from both mappings
      // (ecmp_id<-->hdl and ecmp_hdl <-->id)
      ecmp_id_to_hdl_map.erase(id);
      ecmp_hdl_to_id_map.erase(ecmp_hdl);
    } break;

    default:
      LOG_ERROR("%s:%d Invalid PRE state ID type %d",
                __func__,
                __LINE__,
                static_cast<int>(id_type));
      return TDI_INVALID_ARG;
  }

  return TDI_SUCCESS;
}

bool TdiPREStateObj::statePREIdExists(const PREStateIdType &id_type,
                                      const tdi_id_t &id) const {
  std::lock_guard<std::mutex> lock(state_lock);

  switch (id_type) {
    case PREStateIdType::MULTICAST_MGID: {
      bf_mc_grp_id_t mgid = static_cast<bf_mc_grp_id_t>(id);
      auto it_mgid_hdl = mgid_to_hdl_map.find(mgid);
      if (it_mgid_hdl != mgid_to_hdl_map.end()) {
        return true;
      }
    } break;

    case PREStateIdType::MULTICAST_NODE_ID: {
      auto it_node_it_to_hdl = node_id_to_hdl_map.find(id);
      if (it_node_it_to_hdl != node_id_to_hdl_map.end()) {
        return true;
      }
    } break;

    case PREStateIdType::MULTICAST_ECMP_ID: {
      auto it_ecmp_id_to_hdl = ecmp_id_to_hdl_map.find(id);
      if (it_ecmp_id_to_hdl != ecmp_id_to_hdl_map.end()) {
        return true;
      }
    } break;

    default:
      LOG_ERROR("%s:%d Invalid PRE state ID type %d",
                __func__,
                __LINE__,
                static_cast<int>(id_type));
      break;
  }

  return false;
}

bool TdiPREStateObj::statePREHdlExists(const PREStateIdType &id_type,
                                       const tdi_id_t &hdl) const {
  std::lock_guard<std::mutex> lock(state_lock);

  switch (id_type) {
    case PREStateIdType::MULTICAST_MGID: {
      bf_mc_mgrp_hdl_t mgrp_hdl = static_cast<bf_mc_mgrp_hdl_t>(hdl);
      auto it_mgid_hdl_to_id = mgid_hdl_to_mgid_map.find(mgrp_hdl);
      if (it_mgid_hdl_to_id != mgid_hdl_to_mgid_map.end()) {
        return true;
      }
    } break;

    case PREStateIdType::MULTICAST_NODE_ID: {
      bf_mc_node_hdl_t node_hdl = static_cast<bf_mc_node_hdl_t>(hdl);
      auto node_hdl_to_id = node_hdl_to_id_map.find(node_hdl);
      if (node_hdl_to_id != node_hdl_to_id_map.end()) {
        return true;
      }
    } break;

    case PREStateIdType::MULTICAST_ECMP_ID: {
      bf_mc_ecmp_hdl_t ecmp_hdl = static_cast<bf_mc_ecmp_hdl_t>(hdl);
      auto ecmp_hdl_to_id = ecmp_hdl_to_id_map.find(ecmp_hdl);
      if (ecmp_hdl_to_id != ecmp_hdl_to_id_map.end()) {
        return true;
      }
    } break;

    default:
      LOG_ERROR("%s:%d Invalid PRE state ID type %d",
                __func__,
                __LINE__,
                static_cast<int>(id_type));
      break;
  }

  return false;
}

tdi_status_t TdiPREStateObj::statePREIdGet(const PREStateIdType &id_type,
                                           const tdi_id_t &hdl,
                                           tdi_id_t *id) const {
  std::lock_guard<std::mutex> lock(state_lock);

  switch (id_type) {
    case PREStateIdType::MULTICAST_MGID: {
      bf_mc_mgrp_hdl_t mgrp_hdl = static_cast<bf_mc_mgrp_hdl_t>(hdl);
      auto it_hdl_to_mgid = mgid_hdl_to_mgid_map.find(mgrp_hdl);
      if (it_hdl_to_mgid == mgid_hdl_to_mgid_map.end()) {
        return TDI_OBJECT_NOT_FOUND;
      }
      *id = it_hdl_to_mgid->second;
    } break;

    case PREStateIdType::MULTICAST_NODE_ID: {
      bf_mc_node_hdl_t node_hdl = static_cast<bf_mc_node_hdl_t>(hdl);
      auto it_node_hdl_to_id = node_hdl_to_id_map.find(node_hdl);
      if (it_node_hdl_to_id == node_hdl_to_id_map.end()) {
        return TDI_OBJECT_NOT_FOUND;
      }
      *id = it_node_hdl_to_id->second;
    } break;

    case PREStateIdType::MULTICAST_ECMP_ID: {
      bf_mc_ecmp_hdl_t ecmp_hdl = static_cast<bf_mc_ecmp_hdl_t>(hdl);
      auto it_ecmp_hdl_to_id = ecmp_hdl_to_id_map.find(ecmp_hdl);
      if (it_ecmp_hdl_to_id == ecmp_hdl_to_id_map.end()) {
        return TDI_OBJECT_NOT_FOUND;
      }
      *id = it_ecmp_hdl_to_id->second;
    } break;

    default:
      LOG_ERROR("%s:%d Invalid PRE state ID type %d",
                __func__,
                __LINE__,
                static_cast<int>(id_type));
      return TDI_INVALID_ARG;
  }

  return TDI_SUCCESS;
}

tdi_status_t TdiPREStateObj::statePREHdlGet(const PREStateIdType &id_type,
                                            const tdi_id_t &id,
                                            tdi_id_t *hdl) const {
  std::lock_guard<std::mutex> lock(state_lock);

  switch (id_type) {
    case PREStateIdType::MULTICAST_MGID: {
      bf_mc_grp_id_t mgid = static_cast<bf_mc_grp_id_t>(id);
      auto it_mgid_to_hdl = mgid_to_hdl_map.find(mgid);
      if (it_mgid_to_hdl == mgid_to_hdl_map.end()) {
        return TDI_OBJECT_NOT_FOUND;
      }
      *hdl = it_mgid_to_hdl->second;
    } break;

    case PREStateIdType::MULTICAST_NODE_ID: {
      auto it_node_id_to_hdl = node_id_to_hdl_map.find(id);
      if (it_node_id_to_hdl == node_id_to_hdl_map.end()) {
        return TDI_OBJECT_NOT_FOUND;
      }
      *hdl = it_node_id_to_hdl->second;
    } break;

    case PREStateIdType::MULTICAST_ECMP_ID: {
      auto it_ecmp_id_to_hdl = ecmp_id_to_hdl_map.find(id);
      if (it_ecmp_id_to_hdl == ecmp_id_to_hdl_map.end()) {
        return TDI_OBJECT_NOT_FOUND;
      }
      *hdl = it_ecmp_id_to_hdl->second;
    } break;

    default:
      LOG_ERROR("%s:%d Invalid PRE state ID type %d",
                __func__,
                __LINE__,
                static_cast<int>(id_type));
      return TDI_INVALID_ARG;
  }

  return TDI_SUCCESS;
}

tdi_status_t TdiPREStateObj::statePREIdGetFirst(const PREStateIdType &id_type,
                                                tdi_id_t *first_id,
                                                tdi_id_t *first_hdl) const {
  std::lock_guard<std::mutex> lock(state_lock);

  switch (id_type) {
    case PREStateIdType::MULTICAST_MGID: {
      auto it_mgid_to_hdl = mgid_to_hdl_map.begin();
      if (it_mgid_to_hdl == mgid_to_hdl_map.end()) {
        return TDI_OBJECT_NOT_FOUND;
      }
      *first_id = it_mgid_to_hdl->first;
      *first_hdl = it_mgid_to_hdl->second;
    } break;

    case PREStateIdType::MULTICAST_NODE_ID: {
      auto it_node_id_to_hdl = node_id_to_hdl_map.begin();
      if (it_node_id_to_hdl == node_id_to_hdl_map.end()) {
        return TDI_OBJECT_NOT_FOUND;
      }
      *first_id = it_node_id_to_hdl->first;
      *first_hdl = it_node_id_to_hdl->second;
    } break;

    case PREStateIdType::MULTICAST_ECMP_ID: {
      auto it_ecmp_id_to_hdl = ecmp_id_to_hdl_map.begin();
      if (it_ecmp_id_to_hdl == ecmp_id_to_hdl_map.end()) {
        return TDI_OBJECT_NOT_FOUND;
      }
      *first_id = it_ecmp_id_to_hdl->first;
      *first_hdl = it_ecmp_id_to_hdl->second;
    } break;

    default:
      LOG_ERROR("%s:%d Invalid PRE state ID type %d",
                __func__,
                __LINE__,
                static_cast<int>(id_type));
      return TDI_INVALID_ARG;
  }

  return TDI_SUCCESS;
}

tdi_status_t TdiPREStateObj::statePREIdGetNext(const PREStateIdType &id_type,
                                               const tdi_id_t &id,
                                               tdi_id_t *next_id,
                                               tdi_id_t *next_hdl) const {
  std::lock_guard<std::mutex> lock(state_lock);

  switch (id_type) {
    case PREStateIdType::MULTICAST_MGID: {
      auto it_mgid_to_hdl = mgid_to_hdl_map.upper_bound(id);
      if (it_mgid_to_hdl == mgid_to_hdl_map.end()) {
        return TDI_OBJECT_NOT_FOUND;
      }
      *next_id = it_mgid_to_hdl->first;
      *next_hdl = it_mgid_to_hdl->second;
    } break;

    case PREStateIdType::MULTICAST_NODE_ID: {
      auto it_node_id_to_hdl = node_id_to_hdl_map.upper_bound(id);
      if (it_node_id_to_hdl == node_id_to_hdl_map.end()) {
        return TDI_OBJECT_NOT_FOUND;
      }
      *next_id = it_node_id_to_hdl->first;
      *next_hdl = it_node_id_to_hdl->second;
    } break;

    case PREStateIdType::MULTICAST_ECMP_ID: {
      auto it_ecmp_id_to_hdl = ecmp_id_to_hdl_map.upper_bound(id);
      if (it_ecmp_id_to_hdl == ecmp_id_to_hdl_map.end()) {
        return TDI_OBJECT_NOT_FOUND;
      }
      *next_id = it_ecmp_id_to_hdl->first;
      *next_hdl = it_ecmp_id_to_hdl->second;
    } break;

    default:
      LOG_ERROR("%s:%d Invalid PRE state ID type %d",
                __func__,
                __LINE__,
                static_cast<int>(id_type));
      return TDI_INVALID_ARG;
  }

  return TDI_SUCCESS;
}

}  // namespace tdi
