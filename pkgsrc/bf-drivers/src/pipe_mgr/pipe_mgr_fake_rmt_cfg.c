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


/*******************************************************************************
 *
 *
 *
 *****************************************************************************/
/* Standard header includes */
#include <math.h>
#include <unistd.h>
#include <dlfcn.h>

/* Module header files */
#include <pipe_mgr/pipe_mgr_err.h>
#include <pipe_mgr/pipe_mgr_config.h>
#include <pipe_mgr/pipe_mgr_porting.h>
#include <pipe_mgr/pipe_mgr_intf.h>

/* Local header files */
#include "pipe_mgr_int.h"
#include "pipe_mgr_table_packing.h"

size_t p4_fake_lrn_cfg_type_sz(uint8_t lrn_cfg_type) {
  (void)lrn_cfg_type;
  return 20;
}

uint8_t p4_fake_fld_lst_hdl_to_lq_cfg_type(
    pipe_fld_lst_hdl_t flow_lrn_fld_lst_hdl) {
  (void)flow_lrn_fld_lst_hdl;
  return (flow_lrn_fld_lst_hdl & 0x7);
}

pipe_status_t p4_fake_lrn_decode(uint8_t pipe,
                                 uint8_t learn_cfg_type,
                                 uint8_t lq_data[48],
                                 void *lrn_digest_entry,
                                 uint32_t index) {
  (void)pipe;
  size_t num_data_bytes = p4_fake_lrn_cfg_type_sz(learn_cfg_type);

  PIPE_MGR_MEMCPY((uint8_t *)lrn_digest_entry + index * num_data_bytes,
                  lq_data,
                  num_data_bytes);

  return PIPE_SUCCESS;
}

void pipe_mgr_setup_fake_profile_func_ptrs(profile_id_t profile_id,
                                           rmt_dev_info_t *dev_info) {
  (void)profile_id;
  dev_info->fake_rmt_cfg = true;
  return;
}
