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


#ifndef API_SWITCH_TNA_MINIMAL_MATCH_ACTION_H_
#define API_SWITCH_TNA_MINIMAL_MATCH_ACTION_H_

#include "s3/bf_rt_backend.h"

namespace smi {
namespace bf_rt {
/**
 * p4_object_match_action
 *
 * This class is a wrapper to add a single entry to a MAU table.
 */
class minimal_match_action : public object {
 public:
  minimal_match_action(const switch_attr_id_t status_attr_id,
                       const switch_object_type_t auto_ot,
                       const switch_attr_id_t parent_attr_id,
                       const switch_object_id_t parent);
  ~minimal_match_action() {}
  switch_status_t create_update(_Table &table,
                                _MatchKey &match_key,
                                _ActionEntry &action_entry);
  switch_status_t del(_Table &table, _MatchKey &match_key);
  switch_object_id_t get_auto_oid() { return auto_obj.get_auto_oid(); }
  void device_tgt_set(bf_rt_target_t dev_tgt) { table_dev_tgt = dev_tgt; }
  void clear_attrs() { auto_obj.attrs.clear(); }

 protected:
  auto_object auto_obj;

 private:
  switch_status_t pi_create_update(bool &bf_rt_status,
                                   _Table &table,
                                   _MatchKey &match_key,
                                   _ActionEntry &action_entry);
  switch_status_t pi_del(bool &bf_rt_status,
                         _Table &table,
                         _MatchKey &match_key);
  switch_status_t add_to_cache(_Table &table,
                               _MatchKey &match_key,
                               _ActionEntry &action_entry);

  bf_rt_target_t table_dev_tgt;
  const switch_attr_id_t _status_attr_id = 0;
};
}  // namespace bf_rt
}  // namespace smi

#endif  // API_SWITCH_TNA_MINIMAL_MATCH_ACTION_H_
