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


#ifndef lld_fault_h_included
#define lld_fault_h_included

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file lld_fault.h
 * \brief Fault-handling types
 *
 */

/**
 * @}
 */

extern void lld_fault_possible_unimplemented_reg(bf_dev_id_t dev_id,
                                                 uint32_t reg);
extern void lld_fault_possible_uncorrectable_ecc_u64(bf_dev_id_t dev_id,
                                                     uint64_t mem64b);
extern void lld_fault_uncorrectable_ecc(bf_dev_id_t dev_id, uint64_t mem64b);
extern void lld_fault_correctable_ecc(bf_dev_id_t dev_id, uint64_t mem64b);
extern void lld_fault_dma_error(bf_dev_id_t dev_id, uint64_t *completion_desc);
extern void lld_fault_sw_error(bf_dev_id_t dev_id, bf_sw_fault_e hint);
extern void lld_debug_bus_init(bf_dev_id_t dev_id);

#ifdef __cplusplus
}
#endif /* C++ */

#endif  // lld_fault_h_included
