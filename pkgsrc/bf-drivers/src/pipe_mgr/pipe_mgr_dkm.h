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
 * @file pipe_mgr_dkm.h
 * @date
 *
 *  DataStructures to apply exact match field key mask at run time.
 */

#ifndef __PIPE_MGR_DYNAMIC_EXM_KEY_H__
#define __PIPE_MGR_DYNAMIC_EXM_KEY_H__

#include <stdint.h>
#include "bf_types/bf_types.h"
#include "pipe_mgr/pipe_mgr_intf.h"
#include "pipe_mgr_int.h"

/*
 *        --- In brief: Dynamic Exact Match Field Key Mask ---
 *
 * A normal Exact Match Table with match fields (f1, f2, f3..) matches on using
 * all bits of f1, f2, f3 by default. When necessary to exlude one or more bits
 * of f1, f2, f3,... from matching, match-mask per field can be specified
 * in p4. Specifying match-mask in P4 will not allow run time modification of
 * match mask. Applications that desire modification of match-mask at run time
 * need new capability. Details of dynamic match-key-mask that can be provided
 * at run time is described and implemented in this file.
 *
 * - By associating pragma dynamic_table_key_masks with exact match table, all
 *   match key fields of the table can have their own match-mask-bits that
 *   allows to exclude all or subset of bits from each match-key.
 *
 * - Every match entry of match table will be applied with same match-key-mask.
 *   It is not possible to apply mask-key-mask on per match-entry basis.
 *
 * - When table is not populated (or empty) it is possible to setup
 *   match-key-mask value for all match fields of the table.
 *
 * - By default all bits of every match-key field are included for matching.
 *
 * - High match mask bit (or 1) implies match field bit is included for
 *   exact match hashing.
 *
 * - Regular mat entry add operation on a P4 table with pragma
 *   dynamic_table_key_masks should apply field mask transparently and
 *   then compute hash using only match field bits that included as per
 *   match key mask.
 */

/*
 * Since match-mask applies to all entries of the dynamic key mask table
 * (its not possible to vary match-key-mask across entries in the table. All
 * entries should have same match-key-mask) an API to build match-key-mask
 * is generated for each dynamic key mask table in p4. Also an API to apply
 * key mask is generated for each dynamic key mask table.
 *
 * - A new PD structure to build match-key-mask-spec.
 *
 * - A new PD API to apply match-key-mask-spec on all match fields of a
 *   p4 table is generated.
 *
 */

/*
 * A general work flow to use match key mask is
 *  - build match key mask spec using PD generated structure.
 *  - Using PD API, apply_to_p4_table_key_mask(match_keymask_spec)
 *  - To add an entry to EXM table, repeat following steps.
 *    - build-match-spec
 *    - build-action-spec
 *    - add_to_p4_table_with_action(matchspec, actionspec)
 */

typedef struct pipe_mgr_dkm_gfm_bit_t {
  uint8_t col;     /* GFM output hash bit, 0..51 */
  uint8_t grp;     /* GFM input group, 0..7 */
  uint8_t grp_bit; /* Bit in input group, 0..127 */
} pipe_mgr_dkm_gfm_bit_t;

typedef struct pipe_mgr_dkm_gfm_cfg_t {
  int num_bits;
  pipe_mgr_dkm_gfm_bit_t *bits;
} pipe_mgr_dkm_gfm_cfg_t;

typedef struct pipemgr_dkm_ms_cfg_ {
  unsigned int num_stages;
  unsigned int num_ms_bits;
  pipe_mgr_dkm_gfm_cfg_t **hash; /* Per-stage, per MS bit */
} pipemgr_dkm_ms_cfg_t;

typedef struct pipemgr_dkm_lut_ {
  // Table handle identifying exm table with ability to configure key mask
  // at run time.
  pipe_mat_tbl_hdl_t exm_tbl_hdl;
  // HW Configuration for the table
  pipemgr_dkm_ms_cfg_t *dkm_cfg;
} pipemgr_dkm_lut_t;

/* A struct representing state to (re)configure a register in the GFM. */
typedef struct pipe_mgr_chip_gfm_ {
  /* The updated 32-bit value to write into the config register. */
  uint32_t byte_pair;
  /* A flag indicating if the register needs to be updated. */
  bool updated;
  /* The original 32-bit value the config register hold. */
  uint32_t backup;
} pipe_mgr_chip_gfm_t;

typedef struct pipe_mgr_chip_match_mask_ {
  bool updated[4];
  uint32_t match_mask[4];
} pipe_mgr_chip_match_mask_t;

bool pipe_mgr_is_dkm_table(bf_dev_id_t devid, pipe_mat_tbl_hdl_t mat_tbl_hdl);

pipe_status_t pipe_mgr_dkm_set(pipe_sess_hdl_t sess_hdl,
                               bf_dev_id_t devid,
                               pipe_mat_tbl_hdl_t mat_tbl_hdl,
                               pipe_tbl_match_spec_t *match_spec);

pipe_status_t pipe_mgr_dkm_get(pipe_sess_hdl_t sess_hdl,
                               bf_dev_id_t devid,
                               pipe_mat_tbl_hdl_t mat_tbl_hdl,
                               pipe_tbl_match_spec_t *match_spec);

/* Reset Match Mask back to power on default value which
 * includes all match key mask bits.
 */
pipe_status_t pipe_mgr_dkm_reset(pipe_sess_hdl_t sess_hdl,
                                 bf_dev_id_t devid,
                                 pipe_mat_tbl_hdl_t mat_tbl_hdl);

pipe_status_t pipe_mgr_dkm_rebuild_keymask(pipe_sess_hdl_t sess_hdl,
                                           bf_dev_id_t devid,
                                           pipe_mat_tbl_hdl_t mat_tbl_hdl);

#endif
