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


#ifndef umac_ctrs_h
#define umac_ctrs_h

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

#define UMAC4_CTR_BASE_RMON 0xC000
#define UMAC4_CTR_BASE_PCS 0xC300
#define UMAC4_CTR_BASE_RS_FEC 0xC358
#define UMAC4_CTR_BASE_PCS_VL 0xC370
#define UMAC4_CTR_BASE_RS_FEC_LN 0xE370
#define UMAC4_CTR_BASE_FC_FEC_LN 0xD3B0

/********************************************************************
 * RMON counters (per ch)
 ********************************************************************/
#define UMAC4_RMON_CTR(x) x,

typedef enum {
#include "umac4_rmon.h"
} umac4_rmon_ctr_e;
#undef UMAC4_RMON_CTR

#define UMAC4_RMON_CTR(x) uint64_t x;

typedef struct umac4_rmon_ctr_t {
#include "umac4_rmon.h"
} umac4_rmon_ctr_t;
#undef UMAC4_RMON_CTR

/********************************************************************
 * PCS counters (per ch)
 ********************************************************************/
#define UMAC4_PCS_CTR(x) x,

typedef enum {
#include "umac4_pcs_ctrs.h"
} umac4_pcs_ctr_e;
#undef UMAC4_PCS_CTR

#define UMAC4_PCS_CTR(x) uint64_t x;

typedef struct umac4_pcs_ctr_t {
#include "umac4_pcs_ctrs.h"
} umac4_pcs_ctr_t;
#undef UMAC4_PCS_CTR

/********************************************************************
 * RS FEC counters (per ch)
 ********************************************************************/
#define UMAC4_RS_FEC_CTR(x) x,

typedef enum {
#include "umac4_rs_fec_ctrs.h"
} umac4_rs_fec_ctr_e;
#undef UMAC4_RS_FEC_CTR

#define UMAC4_RS_FEC_CTR(x) uint64_t x;

typedef struct umac4_rs_fec_ctr_t {
#include "umac4_rs_fec_ctrs.h"
} umac4_rs_fec_ctr_t;
#undef UMAC4_RS_FEC_CTR

/********************************************************************
 * PCS virtual lane counters (per vl)
 ********************************************************************/
#define UMAC4_PCS_VL_CTR(x) x,

typedef enum {
#include "umac4_pcs_vl_ctrs.h"
} umac4_pcs_vl_ctr_e;
#undef UMAC4_PCS_VL_CTR

#define UMAC4_PCS_VL_CTR(x) uint64_t x;

typedef struct umac4_pcs_vl_ctr_t {
#include "umac4_pcs_vl_ctrs.h"
} umac4_pcs_vl_ctr_t;
#undef UMAC4_PCS_VL_CTR

/********************************************************************
 * RS FEC lane counters (per FEC ln)
 ********************************************************************/
#define UMAC4_RS_FEC_LN_CTR(x) x,

typedef enum {
#include "umac4_rs_fec_ln_ctrs.h"
} umac4_rs_fec_ln_ctr_e;
#undef UMAC4_RS_FEC_LN_CTR

#define UMAC4_RS_FEC_LN_CTR(x) uint64_t x;

typedef struct umac4_rs_fec_ln_ctr_t {
#include "umac4_rs_fec_ln_ctrs.h"
} umac4_rs_fec_ln_ctr_t;
#undef UMAC4_RS_FEC_LN_CTR

/********************************************************************
 * FC FEC lane counters (per FEC ln)
 ********************************************************************/
#define UMAC4_FC_FEC_LN_CTR(x) x,

typedef enum {
#include "umac4_fc_fec_ln_ctrs.h"
} umac4_fc_fec_ln_ctr_e;
#undef UMAC4_FC_FEC_LN_CTR

#define UMAC4_FC_FEC_LN_CTR(x) uint64_t x;

typedef struct umac4_fc_fec_ln_ctr_t {
#include "umac4_fc_fec_ln_ctrs.h"
} umac4_fc_fec_ln_ctr_t;
#undef UMAC4_FC_FEC_LN_CTR

uint64_t umac4_ctrs_rmon_ctr_in_ram_get(uint64_t *ctr_array,
                                        umac4_rmon_ctr_e ctr);
uint64_t umac4_ctrs_rmon_ctr_get(bf_dev_id_t dev_id,
                                 uint32_t umac,
                                 uint32_t ch,
                                 umac4_rmon_ctr_e ctr);
void umac4_ctrs_rmon_get(bf_dev_id_t dev_id,
                         uint32_t umac,
                         uint32_t ch,
                         umac4_rmon_ctr_t *ctrs);
void umac4_ctrs_rmon_dump(bf_dev_id_t dev_id, uint32_t umac, uint32_t ch);
uint64_t umac4_ctrs_pcs_ctr_get(bf_dev_id_t dev_id,
                                uint32_t umac,
                                uint32_t ch,
                                umac4_pcs_ctr_e ctr);
void umac4_ctrs_pcs_get(bf_dev_id_t dev_id,
                        uint32_t umac,
                        uint32_t ch,
                        umac4_pcs_ctr_t *ctrs);
void umac4_ctrs_pcs_dump(bf_dev_id_t dev_id, uint32_t umac, uint32_t ch);
uint64_t umac4_ctrs_rs_fec_ctr_get(bf_dev_id_t dev_id,
                                   uint32_t umac,
                                   uint32_t ch,
                                   umac4_rs_fec_ctr_e ctr);
void umac4_ctrs_rs_fec_get(bf_dev_id_t dev_id,
                           uint32_t umac,
                           uint32_t ch,
                           umac4_rs_fec_ctr_t *ctrs);
void umac4_ctrs_rs_fec_dump(bf_dev_id_t dev_id, uint32_t umac, uint32_t ch);
uint64_t umac4_ctrs_pcs_vl_ctr_get(bf_dev_id_t dev_id,
                                   uint32_t umac,
                                   umac4_pcs_vl_ctr_e ctr);
void umac4_ctrs_pcs_vl_get(bf_dev_id_t dev_id,
                           uint32_t umac,
                           umac4_pcs_vl_ctr_t *ctrs);
void umac4_ctrs_pcs_vl_dump(bf_dev_id_t dev_id, uint32_t umac);
uint64_t umac4_ctrs_rs_fec_ln_ctr_get(bf_dev_id_t dev_id,
                                      uint32_t umac,
                                      umac4_rs_fec_ln_ctr_e ctr);
void umac4_ctrs_rs_fec_ln_get(bf_dev_id_t dev_id,
                              uint32_t umac,
                              umac4_rs_fec_ln_ctr_t *ctrs);
void umac4_ctrs_rs_fec_ln_dump(bf_dev_id_t dev_id, uint32_t umac);
uint64_t umac4_ctrs_fc_fec_ln_ctr_get(bf_dev_id_t dev_id,
                                      uint32_t umac,
                                      umac4_fc_fec_ln_ctr_e ctr);
void umac4_ctrs_fc_fec_ln_get(bf_dev_id_t dev_id,
                              uint32_t umac,
                              umac4_fc_fec_ln_ctr_t *ctrs);
void umac4_ctrs_rs_fec_ln_range_get(bf_dev_id_t dev_id,
                                    uint32_t umac,
                                    uint32_t start_ctr,
                                    uint32_t n_ctr,
                                    umac4_rs_fec_ln_ctr_t *ctrs);
void umac4_ctrs_fc_fec_ln_dump(bf_dev_id_t dev_id, uint32_t umac);

uint32_t umac4_ctrs_rs_fec_ctr_address_get(bf_dev_id_t dev_id,
                                           uint32_t umac,
                                           uint32_t ch,
                                           umac4_rs_fec_ln_ctr_e ctr);
uint32_t umac4_ctrs_rs_fec_ln_ctr_address_get(bf_dev_id_t dev_id,
                                              uint32_t umac,
                                              umac4_rs_fec_ln_ctr_e ctr);

#ifdef __cplusplus
}
#endif /* C++ */

#endif
