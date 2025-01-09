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


#ifndef lld_int_cb_h
#define lld_int_cb_h

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

typedef void (*lld_mac_int_dump_this_cb)(bf_dev_id_t dev_id,
                                         int mac_block,
                                         int ch,
                                         uint32_t int_reg,
                                         int bit,
                                         uint32_t total,
                                         uint32_t shown);

typedef void (*lld_mac_int_dump_cb)(lld_mac_int_dump_this_cb fn,
                                    bf_dev_id_t dev_id);
typedef void (*lld_mac_int_poll_cb)(bf_dev_id_t dev_id, int mac_block, int ch);
typedef void (*lld_mac_int_bh_wakeup_cb)(bf_dev_id_t dev_id);

#endif  // lld_int_cb_h
