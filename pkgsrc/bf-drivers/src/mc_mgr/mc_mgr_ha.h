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


#ifndef __MC_MGR_HA_H__
#define __MC_MGR_HA_H__

void mc_mgr_free_ha_hw_state(struct mc_mgr_dev_hw_state *st);
bf_status_t mc_mgr_hitless_ha_hw_read(bf_dev_id_t dev_id);
bf_status_t mc_mgr_read_hw_state(int sid,
                                 bf_dev_id_t dev,
                                 struct mc_mgr_dev_hw_state *state);
bf_status_t mc_mgr_compute_delta_changes(bf_dev_id_t dev_id,
                                         bool disable_input_pkts);
bf_status_t mc_mgr_push_delta_changes(bf_dev_id_t dev_id);
#endif
