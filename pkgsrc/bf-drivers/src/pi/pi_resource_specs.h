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


#ifndef _PI_RESOURCE_SPECS_H_
#define _PI_RESOURCE_SPECS_H_

#include <PI/p4info.h>
#include <PI/pi.h>

#include <pipe_mgr/pipe_mgr_intf.h>

void convert_from_pipe_meter_spec(const pi_p4info_t *p4info,
                                  pi_p4_id_t meter_id,
                                  const pipe_meter_spec_t *pipe_meter_spec,
                                  pi_meter_spec_t *meter_spec);

void convert_to_pipe_meter_spec(const pi_p4info_t *p4info,
                                pi_p4_id_t meter_id,
                                const pi_meter_spec_t *meter_spec,
                                pipe_meter_spec_t *pipe_meter_spec);

void convert_to_counter_data(const pi_p4info_t *p4info,
                             pi_p4_id_t counter_id,
                             const pipe_stat_data_t *stat_data,
                             pi_counter_data_t *counter_data);

#endif  // _PI_RESOURCE_SPECS_H_
