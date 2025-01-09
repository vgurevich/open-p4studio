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


#ifndef BF_MAP_H_INCLUDED
#define BF_MAP_H_INCLUDED

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

bf_status_t bf_map_logical_umac3_to_physical(bf_dev_id_t dev_id,
                                             uint32_t logical_umac,
                                             uint32_t *physical_umac);
bf_status_t bf_map_logical_umac4_to_physical(bf_dev_id_t dev_id,
                                             uint32_t logical_umac,
                                             uint32_t *physical_umac);

#ifdef __cplusplus
}
#endif /* C++ */

#endif  // BF_PORT_IF_H_INCLUDED
