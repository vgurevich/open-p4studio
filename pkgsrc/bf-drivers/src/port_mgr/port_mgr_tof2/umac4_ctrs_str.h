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


#ifndef umac_ctrs_str_h
#define umac_ctrs_str_h

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

#include "autogen-required-headers.h"
#include "umac4_ctrs.h"

char *umac4_rmon_ctr_to_name_str(umac4_rmon_ctr_e ctr);
char *umac4_rmon_ctr_to_desc_str(umac4_rmon_ctr_e ctr);
char **umac4_rmon_ctr_name_strs(void);
char **umac4_rmon_ctr_desc_strs(void);
char *umac4_pcs_ctr_to_name_str(umac4_pcs_ctr_e ctr);
char *umac4_pcs_ctr_to_desc_str(umac4_pcs_ctr_e ctr);
char **umac4_pcs_ctr_name_strs(void);
char **umac4_pcs_ctr_desc_strs(void);
char *umac4_rs_fec_ctr_to_name_str(umac4_pcs_ctr_e ctr);
char *umac4_rs_fec_ctr_to_desc_str(umac4_pcs_ctr_e ctr);
char **umac4_rs_fec_ctr_name_strs(void);
char **umac4_rs_fec_ctr_desc_strs(void);
char *umac4_pcs_vl_ctr_to_name_str(umac4_pcs_ctr_e ctr);
char *umac4_pcs_vl_ctr_to_desc_str(umac4_pcs_ctr_e ctr);
char **umac4_pcs_vl_ctr_name_strs(void);
char **umac4_pcs_vl_ctr_desc_strs(void);
char *umac4_rs_fec_ln_ctr_to_name_str(umac4_pcs_ctr_e ctr);
char *umac4_rs_fec_ln_ctr_to_desc_str(umac4_pcs_ctr_e ctr);
char **umac4_rs_fec_ln_ctr_name_strs(void);
char **umac4_rs_fec_ln_ctr_desc_strs(void);
char *umac4_fc_fec_ln_ctr_to_name_str(umac4_pcs_ctr_e ctr);
char *umac4_fc_fec_ln_ctr_to_desc_str(umac4_pcs_ctr_e ctr);
char **umac4_fc_fec_ln_ctr_name_strs(void);
char **umac4_fc_fec_ln_ctr_desc_strs(void);

#ifdef __cplusplus
}
#endif /* C++ */

#endif
