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

#ifndef BF_TOF3_SERDES_UTILS_INCLUDED
#define BF_TOF3_SERDES_UTILS_INCLUDED

#include <stdint.h>
#include <bf_types/bf_types.h>

bf_status_t bf_tof3_serdes_lane_install(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        uint32_t logical_ln, uint32_t tx_ln,
                                        uint32_t rx_ln, uint32_t is_4ln);

bf_status_t bf_tof3_serdes_csr_def_get(uint32_t dev_id, uint32_t dev_port,
                                       uint32_t ln, uint32_t csr_addr,
                                       char **name, uint32_t *num_flds,
                                       uint32_t *fld_num_base, char **comment);

bf_status_t bf_tof3_serdes_vreg_def_get(uint32_t dev_id, uint32_t dev_port,
                                        uint32_t ln, uint32_t csr_addr,
                                        char **name, uint32_t *num_flds,
                                        uint32_t *fld_num_base, char **comment);

bf_status_t bf_tof3_serdes_csr_def_get_next(uint32_t dev_id, uint32_t dev_port,
                                            uint32_t ln, uint32_t csr_addr,
                                            uint32_t *next_csr_addr,
                                            char **name, uint32_t *num_flds,
                                            uint32_t *fld_num_base,
                                            char **comment);

bf_status_t bf_tof3_serdes_csr_fld_def_get(
    uint32_t dev_id, uint32_t dev_port, uint32_t ln, uint32_t csr_addr,
    uint32_t fld_num, char **name, uint32_t *lo_bit, uint32_t *width,
    uint32_t *mask, uint32_t *access, uint32_t *reset_value, char **comment);
int bf_tof3_serdes_read_status(uint32_t dev_id, uint32_t dev_port, uint32_t ln,
                               uint32_t br);
int bf_tof3_serdes_read_status2(uint32_t dev_id, uint32_t dev_port, uint32_t ln,
                                uint32_t br);
char *bf_tof3_serdes_prbs_mode_str(uint32_t mode);
char *bf_tof3_serdes_term_mode_str(uint32_t mode);
bf_status_t bf_tof3_sweep(bf_dev_id_t dev_id, bf_dev_port_t tx_dev_port,
                          bf_dev_port_t rx_dev_port, uint32_t ln,
                          // tx settings
                          uint32_t cm3, uint32_t cm2, uint32_t cm1, uint32_t c0,
                          uint32_t c1,
                          // rx settings
                          uint32_t ctle_adapt_en, uint32_t ctle_adapt_boost,
                          uint32_t vga_cap,
                          // return vals
                          uint32_t *eq_ack, uint32_t *cdr_lock,
                          uint32_t *bist_lock, double *ber);
bf_status_t bf_tof3_run_lt(bf_dev_id_t dev_id, bf_dev_port_t dev_port,
                           uint32_t ln, uint32_t *lt_status);
bf_status_t bf_tof3_run_lt2(bf_dev_id_t dev_id, bf_dev_port_t dev_port,
                            uint32_t ln, uint32_t width, uint32_t clause);

char *bf_tof3_serdes_tech_ability_to_str(uint32_t tech_ability);
char *bf_tof3_serdes_loopback_mode_to_str(uint32_t loopback_mode);
uint32_t bf_tof3_serdes_str_to_loopback_mode(char *str);

#endif // BF_TOF3_SERDES_UTILS_INCLUDED
