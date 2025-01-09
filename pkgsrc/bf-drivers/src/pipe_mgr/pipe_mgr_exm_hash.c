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


/*!
 * @file pipe_mgr_exm_hash.c
 * @date
 *
 * Contains implementation of the "hashing block" for runtime software. This
 * implementation pretty much tries to replicate the hash block in the device.
 */

/* Standard header includes */
#include <math.h>

/* Module header includes */
#include <pipe_mgr/pipe_mgr_intf.h>

/* Local header includes */
#include "pipe_mgr_exm_hash.h"
#include "pipe_mgr_exm_tbl_mgr_int.h"
#include "pipe_mgr_int.h"
#include "pipe_mgr_table_packing.h"

/* Some globals to make life easy */

/* Number of bits of hash required to address all the locations in one
 * Match-action RAM unit.
 */
uint32_t _tofino_num_hash_bits = 0;

/* A mask that needs to be used to extract the number of hash-bits from a
 * given number.
 */
uint32_t _tofino_hash_mask = 0;

pipe_status_t pipe_mgr_hash_init(void) {
  _tofino_num_hash_bits = 10;

  _tofino_hash_mask = pow(2, _tofino_num_hash_bits) - 1;

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_exm_hash_compute(bf_dev_id_t dev_id,
                                        profile_id_t profile_id,
                                        pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                        pipe_tbl_match_spec_t *pipe_match_spec,
                                        dev_stage_t stage_id,
                                        pipe_exm_hash_t *hash,
                                        uint32_t *num_hashes) {
  pipe_status_t status = PIPE_SUCCESS;
  (void)profile_id;
  /* For now initialize the number_hashes to 1 */
  uint8_t number_hashes = 1;

  // There are three functions(alogorithm) available for hash computation
  // 1. bf_hash_mat_entry_hash_compute
  // 2. bf_hash_mat_entry_hash2_compute
  // 3. bf_hash_mat_entry_radix_hash_compute
  // The third algorithm performs better than the other 2 but uses more memory.
  // Algorithms 1 and 2 use same amount of memory but 2nd algorithm performs
  // much better than 1
  // Currently we are using Algorithm3.

  status = bf_hash_mat_entry_radix_hash_compute(
      dev_id, profile_id, stage_id, mat_tbl_hdl, pipe_match_spec, false, hash);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Error in computing the hash for exact match table"
        " with handle 0x%x",
        __func__,
        mat_tbl_hdl);
    return status;
  }

  *num_hashes = number_hashes;
  return status;
}

pipe_status_t pipe_mgr_exm_proxy_hash_compute(bf_dev_id_t device_id,
                                              profile_id_t profile_id,
                                              pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                              pipe_tbl_match_spec_t *match_spec,
                                              dev_stage_t stage_id,
                                              uint64_t *proxy_hash) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_sess_hdl_t sess_hdl = 0;
  (void)sess_hdl;
  (void)profile_id;
  pipe_exm_hash_t hash = {0};

  // There are three functions(alogorithm) available for hash computation
  // 1. bf_hash_mat_entry_hash_compute
  // 2. bf_hash_mat_entry_hash2_compute
  // 3. bf_hash_mat_entry_radix_hash_compute
  // The third algorithm performs better than the other 2 but uses more memory.
  // Algorithms 1 and 2 use same amount of memory but 2nd algorithm performs
  // much better than 1
  // Currently we are using Algorithm3.
  status = bf_hash_mat_entry_radix_hash_compute(
      device_id, profile_id, stage_id, mat_tbl_hdl, match_spec, true, &hash);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Error in computing the proxy hash for table"
        " with handle 0x%x",
        __func__,
        mat_tbl_hdl);
  }

  *proxy_hash = hash.hash_value;
  return status;
}

uint32_t pipe_mgr_exm_extract_per_hashway_hash(pipe_exm_hash_t *hash,
                                               void *hdata,
                                               uint32_t *subword_loc) {
  uint32_t ram_select = 0;
  uint32_t ram_line = 0;
  uint64_t hash_value = hash->hash_value;
  pipe_mgr_exm_hash_way_data_t *hashway_data =
      (pipe_mgr_exm_hash_way_data_t *)hdata;
  if (hashway_data->num_subword_bits > 0) {
    if (!subword_loc) {
      LOG_ERROR("%s : Error Failed null check!", __func__);
      return ~0;
    }
    *subword_loc =
        hash->hash_value & ((1 << hashway_data->num_subword_bits) - 1);
    hash_value = hash->hash_value >> hashway_data->num_subword_bits;
  }
  ram_line = ((hash_value) >> (hashway_data->ram_line_start_offset)) &
             (uint32_t)((1 << hashway_data->num_ram_line_bits) - 1);

  ram_select = ((hash_value) >> (hashway_data->ram_select_start_offset)) &
               (uint32_t)((1 << hashway_data->num_ram_select_bits) - 1);
  return ((ram_select << hashway_data->num_ram_line_bits) | ram_line);
}
