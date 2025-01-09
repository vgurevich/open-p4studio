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


#ifndef LLD_IND_REG_IF_TOF_H_INCLUDED
#define LLD_IND_REG_IF_TOF_H_INCLUDED

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

lld_err_t lld_ind_write_tof(bf_dev_id_t dev_id,
                            uint64_t ind_addr,
                            uint64_t data_hi,
                            uint64_t data_lo);

lld_err_t lld_ind_read_tof(bf_dev_id_t dev_id,
                           uint64_t ind_addr,
                           uint64_t *data_hi,
                           uint64_t *data_lo);

#ifdef __cplusplus
}
#endif /* C++ */

#endif  // LLD_IND_REG_IF_TOF_H_INCLUDED
