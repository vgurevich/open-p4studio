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
 * @file pipe_mgr_p4parser_ctx.h
 * @date
 *
 *  DataStructures populated after processing p4parser details using Json file
 */

#include <stdint.h>

#ifndef __PIPE_MGR_P4PARSER_CTX__
#define __PIPE_MGR_P4PARSER_CTX__

#include <target-utils/third-party/cJSON/cJSON.h>
#include <target-utils/id/id.h>
#include <target-utils/map/map.h>

#include <pipe_mgr/pipe_mgr_err.h>

// --------------- P4 Parser-Value-Set Overview -----------------
/*
 * P4 construct parser_value_set provides ability to provide at run time
 * parser branch condition value. Compile time constant value provided in p4
 * program will continue to remain; however by providing ability to modify/add
 * new value using which packet parser can transition between parser states
 * helps to recognize new kind of packets without modifying P4 program and
 * recompiling.
 *
 *  Compiler will publish pvs related information into context json.
 *  PVS details publised in json include,
 *    - Tcam entry# , Tcam container# into which PVS value should
 *      be programmed by runtime API
 *    - bit mapping of PVS value to tcam-container bits
 *    - Parser State ID that against which the PVS value is used
 *  Driver on startup will parse context json and prepare data structures.
 *  A list of pvs data structure nodes are maintained that apply on ingress
 *  parser and another list of pvs nodes are maintained for egress parsers.
 *
 *  Run time API that intends to provide constant value to pvs name, will
 *  pass via API pvs name, value and mask. pvs data structure node is
 *  is searched in both ingress parser list of nodes as well as in
 *  egress parser list of pvs nodes. A node should be found in one or
 *  both lists. PVS node contains necessary information fetched from JSON.
 *  Using that information and after applying mask on the value passed in
 *  run time API, the value is programmed into parser TCAMs.
 *
 */

#define PIPE_PVS_ENT_HDL_INVALID_HDL 0xdeadbeef

typedef struct pipemgr_p4parser_pvs_tcam_s {
  uint8_t allocated;  // 1: in use; 0 is free to use
  uint16_t tcam_row;  // Physical tcam row entry number
  uint32_t value;     // Value this container matches on.
                      // for now using uint32 as it is not expected to
                      // have parser container wider than 32bits.
  uint32_t mask;
  uint32_t encoded_val;
  uint32_t encoded_msk;
} pipemgr_p4parser_pvs_tcam_t;

struct pipe_mgr_pvs_encoding {
  /* First apply src_data_mask to the input.
   * Then right shift by masked_src_right_shift.
   * Then left shift by masked_src_left_shift.
   * Then OR the result into the PVS entry. */
  uint32_t src_data_mask;
  uint8_t masked_src_right_shift;
  uint8_t masked_src_left_shift;
};

typedef struct pipemgr_p4parser_pvs_hw_parser_instance_s {
  pipemgr_p4parser_pvs_tcam_t *tcam_rows;
} pipemgr_p4parser_pvs_hw_parser_instance_t;

typedef struct pipemgr_p4parser_pvs_pipe_parser_instance_s {
  /* Array is allocated per logical parser in a pipe. */
  pipemgr_p4parser_pvs_hw_parser_instance_t *parser_instance;
} pipemgr_p4parser_pvs_pipe_parser_instance_t;

#define PIPE_PVS_MAX_SHARE_PVS_STATE 16
typedef struct pipemgr_p4parser_pvs_t {
  uint8_t pvs_size;     // Number of entries in pvs
  uint8_t gress;        // Ingress = 0, egress = 1
  uint8_t encoding_cnt; /* Length of encoding array. */
  uint8_t parser_state_id[PIPE_PVS_MAX_SHARE_PVS_STATE];  // Parser state
                                                          // corresponding to
                                                          // pvs
  uint8_t parser_state_numb;  // Number of parser states
  pipe_mgr_pvs_parser_scope_en
      parser_scope;  // specify what parser instances are targetted
                     // when modifying pvs
  pipe_mgr_pvs_pipe_scope_en pipe_scope;  // specify what pipes are targetted
                                          // when modifying pvs
  scope_pipes_t user_define_pipe_scope
      [PIPE_MGR_MAX_USER_DEFINED_SCOPES];  // only used when pipe_scope is
                                           // PIPE_MGR_PVS_SCOPE_USER_DEFINED
  pipemgr_p4parser_pvs_pipe_parser_instance_t *
      pipe_instance[PIPE_PVS_MAX_SHARE_PVS_STATE];  // First (fixed dimension)
                                                    // is per parser state and
                                                    // would only be non-zero
                                                    // for PVSs which exist in
                                                    // multiple parse states per
                                                    // gress. Second dimension
                                                    // is per logical pipe.
  struct pipe_mgr_pvs_encoding *encoding;
} pipemgr_p4parser_pvs_t;

typedef struct pipemgr_p4parser_key_htbl_node_s {
  pipe_pvs_hdl_t pvs_ent_hdl;  // hdl associated with this PVS entry
  uint8_t gress;          // gress of the parser this entry is programmed into
  bf_dev_pipe_t pipe_id;  // pipes in which this entry was programmed into
  uint32_t parser_id;     // hw parser where this entry was programmed into
  uint32_t parser_value;  // value passed in by the user
  uint32_t parser_value_mask;  // value mask passed in by the user
  // encoded value and mask can be different across gresses
  uint32_t encoded_val[2];   // encoded parser value; 0->ingress 1->egress
  uint32_t encoded_mask[2];  // encoded parser value mask; 0->ingress 1->egress
  uint16_t tcam_entry_idx;   // physical tcam row entry number.
} pipemgr_p4parser_key_htbl_node_t;

/* Structure for entry handle management */
typedef struct pipemgr_p4parser_ent_hdl_mgmt_s {
  /* Entry handle allocator */
  bf_id_allocator *ent_hdl_allocator;
  /* A backup entry handle allocator for txn purposes */
  bf_id_allocator *backup_ent_hdl_allocator;
} pipemgr_p4parser_ent_hdl_mgmt_t;

typedef struct pipemgr_p4parser_global_gress_node_t {
  char *pvs_name;                              // Name of the PVS defined in P4
  uint32_t pvs_handle;                         // PVS handle
  profile_id_t profile_id;                     // Profile
  pipe_prsr_instance_hdl_t prsr_instance_hdl;  // prsr instance hdl
  uint8_t gress_scope;
  pipemgr_p4parser_pvs_t *ingress_pvs;  // Ingress Parser Value Set variable
  pipemgr_p4parser_pvs_t *egress_pvs;   // Egress Parser Value Set variable
  pipemgr_p4parser_ent_hdl_mgmt_t *ent_hdl_mgr;  // Entry handle allocator
  bf_map_t htbl;  // Maps entry handle to pipemgr_p4parser_key_htbl_node_t
} pipemgr_p4parser_global_gress_node_t;

typedef struct pipemgr_p4parser_profile_ctx_ {
  bf_map_t gbl_hash_tbl;
} pipemgr_p4parser_profile_ctx_t;

typedef struct pipemgr_p4parser_ctx_ {
  uint32_t num_profiles;
  pipemgr_p4parser_profile_ctx_t *profiles;
} pipemgr_p4parser_ctx_t;

typedef enum pipemgr_p4parser_prsr_tcam_state {
  PIPEMGR_P4PARSER_PRSR_TCAM_FREE = 0,
  PIPEMGR_P4PARSER_PRSR_TCAM_ALLOCATED
} pipemgr_p4parser_prsr_tcam_state_en;

pipe_status_t pipe_mgr_pvs_parser_tcam_add(pipe_sess_hdl_t sess_hdl,
                                           bf_dev_id_t devid,
                                           pipe_pvs_hdl_t pvs_handle,
                                           uint8_t gress,
                                           bf_dev_pipe_t pipeid,
                                           uint8_t parser_id,
                                           uint32_t parser_value,
                                           uint32_t parser_value_mask,
                                           pipe_pvs_hdl_t *pvs_entry_handle);

pipe_status_t pipe_mgr_pvs_parser_tcam_modify(pipe_sess_hdl_t sess_hdl,
                                              bf_dev_id_t devid,
                                              pipe_pvs_hdl_t pvs_handle,
                                              pipe_pvs_hdl_t pvs_entry_handle,
                                              uint32_t parser_value,
                                              uint32_t parser_value_mask);

pipe_status_t pipe_mgr_pvs_parser_tcam_delete(pipe_sess_hdl_t sess_hdl,
                                              bf_dev_id_t devid,
                                              pipe_pvs_hdl_t pvs_handle,
                                              pipe_pvs_hdl_t pvs_entry_handle);

pipe_status_t pipe_mgr_pvs_parser_entry_handle_get(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t devid,
    pipe_pvs_hdl_t pvs_handle,
    uint8_t gress,
    bf_dev_pipe_t pipeid,
    uint8_t parser_id,
    uint32_t pvs_key,
    uint32_t pvs_mask,
    pipe_pvs_hdl_t *pvs_entry_handle);

pipe_status_t pipemgr_pvs_db_init(bf_dev_id_t devid, uint32_t num_profiles);
pipe_status_t pipe_mgr_pvs_init(pipe_sess_hdl_t sess_hdl, bf_dev_id_t devid);

void pipe_mgr_free_pvs(bf_dev_id_t devid);

void pipe_mgr_free_pvs_helper(bf_dev_id_t devid, profile_id_t prof_id);

pipe_status_t pipe_mgr_pvs_get_global_gress_node(
    bf_dev_id_t devid,
    uint32_t pvs_handle,
    pipemgr_p4parser_global_gress_node_t **pvs_node);

pipe_status_t pipe_mgr_pvs_parser_property_set(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_pvs_hdl_t pvs_handle,
    pipe_mgr_pvs_prop_type_t property,
    pipe_mgr_pvs_prop_value_t value,
    pipe_mgr_pvs_prop_args_t args);

pipe_status_t pipe_mgr_pvs_parser_property_get(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_pvs_hdl_t pvs_handle,
    pipe_mgr_pvs_prop_type_t property,
    pipe_mgr_pvs_prop_value_t *value,
    pipe_mgr_pvs_prop_args_t args);

pipe_status_t pipe_mgr_pvs_parser_tcam_get(pipe_sess_hdl_t sess_hdl,
                                           bf_dev_id_t devid,
                                           pipe_pvs_hdl_t pvs_handle,
                                           pipe_pvs_hdl_t pvs_entry_handle,
                                           uint32_t *pvs_key,
                                           uint32_t *pvs_mask,
                                           uint8_t *entry_gress,
                                           bf_dev_pipe_t *entry_pipe,
                                           uint8_t *entry_parser_id);

pipe_status_t pipe_mgr_pvs_parser_tcam_hw_get(pipe_sess_hdl_t sess_hdl,
                                              bf_dev_id_t devid,
                                              uint8_t gress,
                                              bf_dev_pipe_t pipeid,
                                              uint8_t parser_id,
                                              pipe_pvs_hdl_t pvs_handle,
                                              pipe_pvs_hdl_t pvs_entry_handle,
                                              uint32_t *pvs_key,
                                              uint32_t *pvs_mask);

pipe_status_t pipe_mgr_pvs_get_next_handles(bf_dev_id_t dev_id,
                                            pipe_pvs_hdl_t pvs_handle,
                                            bf_dev_direction_t gress,
                                            bf_dev_pipe_t pipe_id,
                                            uint8_t parser_id,
                                            int entry_handle,
                                            int handle_count,
                                            pipe_pvs_hdl_t *entry_handles);

pipe_status_t pipe_mgr_pvs_get_count(bf_dev_id_t dev_id,
                                     pipe_pvs_hdl_t pvs_handle,
                                     bf_dev_direction_t gress,
                                     bf_dev_pipe_t pipe_id,
                                     uint8_t parser_id,
                                     bool read_from_hw,
                                     uint32_t *count);

pipe_status_t pipe_mgr_pvs_parser_tcam_clear(pipe_sess_hdl_t sess_hdl,
                                             bf_dev_id_t dev_id,
                                             pipe_pvs_hdl_t pvs_handle,
                                             bf_dev_direction_t gress,
                                             bf_dev_pipe_t pipe_id,
                                             uint8_t parser_id);

pipe_status_t pipe_mgr_pvs_parser_exclusive_access_start(bf_dev_id_t devid);
pipe_status_t pipe_mgr_pvs_parser_exclusive_access_end(bf_dev_id_t devid);
#endif
