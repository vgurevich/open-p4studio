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


#include <tofino/pdfixed/pd_plcmt.h>

p4_pd_status_t p4_pd_register_plcmnt_mem_fns(p4_pd_plcmnt_alloc alloc_fn,
                                             p4_pd_plcmnt_free free_fn) {
  return pipe_register_plcmnt_mem_fns(alloc_fn, free_fn);
}
p4_pd_status_t p4_pd_plcmt_data_size(const void *data, size_t *size) {
  return bf_drv_plcmt_data_size(data, size);
}
p4_pd_status_t p4_pd_plcmt_copy(const void *src, void *dst) {
  return bf_drv_plcmt_copy(src, dst);
}
p4_pd_status_t p4_pd_plcmt_duplicate(const void *src, void **copy) {
  return bf_drv_plcmt_duplicate(src, copy);
}

p4_pd_status_t p4_pd_plcmt_pack_data_size(const void *unpacked_data,
                                          size_t *size) {
  return bf_drv_plcmt_pack_data_size(unpacked_data, size);
}
p4_pd_status_t p4_pd_plcmt_unpack_data_size(const void *packed_data,
                                            size_t *size) {
  return bf_drv_plcmt_unpack_data_size(packed_data, size);
}
p4_pd_status_t p4_pd_plcmt_copy_pack(const void *unpacked_src,
                                     void *packed_dst) {
  return bf_drv_plcmt_copy_pack(unpacked_src, packed_dst);
}
p4_pd_status_t p4_pd_plcmt_copy_unpack(const void *packed_src,
                                       void *unpacked_dst) {
  return bf_drv_plcmt_copy_unpack(packed_src, unpacked_dst);
}

p4_pd_status_t p4_pd_plcmt_free(void *data) { return bf_drv_plcmt_free(data); }

struct p4_pd_plcmt_info {
  int a;
  void *b, *c;
};
struct p4_pd_plcmt_info *p4_pd_create_plcmt_info() {
  return (struct p4_pd_plcmt_info *)pipe_create_plcmt_info();
}
void p4_pd_destroy_plcmt_info(struct p4_pd_plcmt_info *info) {
  pipe_destroy_plcmt_info((struct pipe_plcmt_info *)info);
}

p4_pd_status_t p4_pd_set_one_plcmt_op_mat_add(
    struct p4_pd_plcmt_info *info,
    p4_pd_entry_hdl_t ent_hdl,
    bf_dev_pipe_t pipe,
    p4_pd_idx_t ent_idx,
    p4_pd_idx_t indirect_selection_idx,
    p4_pd_idx_t indirect_action_idx,
    void *ent_data) {
  return pipe_set_one_plcmt_op_mat_add((struct pipe_plcmt_info *)info,
                                       ent_hdl,
                                       pipe,
                                       ent_idx,
                                       indirect_selection_idx,
                                       indirect_action_idx,
                                       ent_data);
}
p4_pd_status_t p4_pd_set_one_plcmt_op_mat_mov(
    struct p4_pd_plcmt_info *info,
    p4_pd_entry_hdl_t ent_hdl,
    p4_pd_idx_t ent_idx,
    p4_pd_idx_t indirect_selection_index,
    p4_pd_idx_t indirect_action_index,
    void *ent_data) {
  return pipe_set_one_plcmt_op_mat_mov((struct pipe_plcmt_info *)info,
                                       ent_hdl,
                                       ent_idx,
                                       indirect_selection_index,
                                       indirect_action_index,
                                       ent_data);
}
p4_pd_status_t p4_pd_set_one_plcmt_op_mat_mod(
    struct p4_pd_plcmt_info *info,
    p4_pd_entry_hdl_t ent_hdl,
    p4_pd_idx_t indirect_selection_index,
    p4_pd_idx_t indirect_action_index,
    void *ent_data) {
  return pipe_set_one_plcmt_op_mat_mod((struct pipe_plcmt_info *)info,
                                       ent_hdl,
                                       indirect_selection_index,
                                       indirect_action_index,
                                       ent_data);
}
p4_pd_status_t p4_pd_set_one_plcmt_op_mat_del(struct p4_pd_plcmt_info *info,
                                              p4_pd_entry_hdl_t ent_hdl) {
  return pipe_set_one_plcmt_op_mat_del((struct pipe_plcmt_info *)info, ent_hdl);
}
p4_pd_status_t p4_pd_set_one_plcmt_op_mat_set_dflt(
    struct p4_pd_plcmt_info *info,
    p4_pd_entry_hdl_t ent_hdl,
    bf_dev_pipe_t pipe,
    p4_pd_idx_t indirect_selection_idx,
    p4_pd_idx_t indirect_action_idx,
    void *ent_data) {
  return pipe_set_one_plcmt_op_mat_set_dflt((struct pipe_plcmt_info *)info,
                                            ent_hdl,
                                            pipe,
                                            indirect_selection_idx,
                                            indirect_action_idx,
                                            ent_data);
}
p4_pd_status_t p4_pd_set_one_plcmt_op_mat_clr_dflt(
    struct p4_pd_plcmt_info *info,
    p4_pd_entry_hdl_t ent_hdl,
    bf_dev_pipe_t pipe) {
  return pipe_set_one_plcmt_op_mat_clr_dflt(
      (struct pipe_plcmt_info *)info, ent_hdl, pipe);
}
p4_pd_status_t p4_pd_set_one_plcmt_op_mat_add_multi(
    struct p4_pd_plcmt_info *info,
    p4_pd_entry_hdl_t ent_hdl,
    bf_dev_pipe_t pipe,
    p4_pd_idx_t indirect_selection_idx,
    p4_pd_idx_t indirect_action_idx,
    int array_length,
    struct p4_pd_multi_index *location_array,
    void *ent_data) {
  return pipe_set_one_plcmt_op_mat_add_multi(
      (struct pipe_plcmt_info *)info,
      ent_hdl,
      pipe,
      indirect_selection_idx,
      indirect_action_idx,
      array_length,
      (struct pipe_multi_index *)location_array,
      ent_data);
}
p4_pd_status_t p4_pd_set_one_plcmt_op_mat_mov_multi(
    struct p4_pd_plcmt_info *info,
    p4_pd_entry_hdl_t ent_hdl,
    p4_pd_idx_t indirect_selection_idx,
    p4_pd_idx_t indirect_action_idx,
    int array_length,
    struct p4_pd_multi_index *location_array,
    void *ent_data) {
  return pipe_set_one_plcmt_op_mat_mov_multi(
      (struct pipe_plcmt_info *)info,
      ent_hdl,
      indirect_selection_idx,
      indirect_action_idx,
      array_length,
      (struct pipe_multi_index *)location_array,
      ent_data);
}

p4_pd_status_t p4_pd_set_one_plcmt_op_adt_add(struct p4_pd_plcmt_info *info,
                                              p4_pd_entry_hdl_t ent_hdl,
                                              bf_dev_pipe_t pipe,
                                              void *ent_data) {
  return pipe_set_one_plcmt_op_adt_add(
      (struct pipe_plcmt_info *)info, ent_hdl, pipe, ent_data);
}
p4_pd_status_t p4_pd_set_one_plcmt_op_adt_mod(struct p4_pd_plcmt_info *info,
                                              p4_pd_entry_hdl_t ent_hdl,
                                              void *ent_data) {
  return pipe_set_one_plcmt_op_adt_mod(
      (struct pipe_plcmt_info *)info, ent_hdl, ent_data);
}
p4_pd_status_t p4_pd_set_one_plcmt_op_adt_del(struct p4_pd_plcmt_info *info,
                                              p4_pd_entry_hdl_t ent_hdl) {
  return pipe_set_one_plcmt_op_adt_del((struct pipe_plcmt_info *)info, ent_hdl);
}

p4_pd_status_t p4_pd_set_one_plcmt_op_sel_grp_create(
    struct p4_pd_plcmt_info *info,
    p4_pd_grp_hdl_t grp_hdl,
    bf_dev_pipe_t pipe,
    uint32_t num_indexes,
    uint32_t max_members,
    p4_pd_idx_t base_logical_index,
    int array_length,
    struct p4_pd_multi_index *location_array) {
  return pipe_set_one_plcmt_op_sel_grp_create(
      (struct pipe_plcmt_info *)info,
      grp_hdl,
      pipe,
      num_indexes,
      max_members,
      base_logical_index,
      array_length,
      (struct pipe_multi_index *)location_array);
}
p4_pd_status_t p4_pd_set_one_plcmt_op_sel_grp_destroy(
    struct p4_pd_plcmt_info *info,
    p4_pd_grp_hdl_t grp_hdl,
    bf_dev_pipe_t pipe) {
  return pipe_set_one_plcmt_op_sel_grp_destroy(
      (struct pipe_plcmt_info *)info, grp_hdl, pipe);
}
p4_pd_status_t p4_pd_set_one_plcmt_op_sel_add(struct p4_pd_plcmt_info *info,
                                              p4_pd_grp_hdl_t grp_hdl,
                                              p4_pd_entry_hdl_t ent_hdl,
                                              bf_dev_pipe_t pipe,
                                              p4_pd_idx_t ent_idx,
                                              p4_pd_idx_t ent_subidx,
                                              void *ent_data) {
  return pipe_set_one_plcmt_op_sel_add((struct pipe_plcmt_info *)info,
                                       grp_hdl,
                                       ent_hdl,
                                       pipe,
                                       ent_idx,
                                       ent_subidx,
                                       ent_data);
}
p4_pd_status_t p4_pd_set_one_plcmt_op_sel_del(struct p4_pd_plcmt_info *info,
                                              p4_pd_grp_hdl_t grp_hdl,
                                              p4_pd_entry_hdl_t ent_hdl,
                                              bf_dev_pipe_t pipe,
                                              p4_pd_idx_t ent_idx,
                                              p4_pd_idx_t ent_subidx) {
  return pipe_set_one_plcmt_op_sel_del((struct pipe_plcmt_info *)info,
                                       grp_hdl,
                                       ent_hdl,
                                       pipe,
                                       ent_idx,
                                       ent_subidx);
}
p4_pd_status_t p4_pd_set_one_plcmt_op_sel_activate(
    struct p4_pd_plcmt_info *info,
    p4_pd_grp_hdl_t grp_hdl,
    p4_pd_entry_hdl_t ent_hdl,
    bf_dev_pipe_t pipe,
    p4_pd_idx_t ent_idx,
    p4_pd_idx_t ent_subidx) {
  return pipe_set_one_plcmt_op_sel_activate((struct pipe_plcmt_info *)info,
                                            grp_hdl,
                                            ent_hdl,
                                            pipe,
                                            ent_idx,
                                            ent_subidx);
}
p4_pd_status_t p4_pd_set_one_plcmt_op_sel_deactivate(
    struct p4_pd_plcmt_info *info,
    p4_pd_grp_hdl_t grp_hdl,
    p4_pd_entry_hdl_t ent_hdl,
    bf_dev_pipe_t pipe,
    p4_pd_idx_t ent_idx,
    p4_pd_idx_t ent_subidx) {
  return pipe_set_one_plcmt_op_sel_deactivate((struct pipe_plcmt_info *)info,
                                              grp_hdl,
                                              ent_hdl,
                                              pipe,
                                              ent_idx,
                                              ent_subidx);
}
p4_pd_status_t p4_pd_set_one_plcmt_op_sel_set_fallback(
    struct p4_pd_plcmt_info *info,
    p4_pd_entry_hdl_t ent_hdl,
    bf_dev_pipe_t pipe,
    void *ent_data) {
  return pipe_set_one_plcmt_op_sel_set_fallback(
      (struct pipe_plcmt_info *)info, ent_hdl, pipe, ent_data);
}
p4_pd_status_t p4_pd_set_one_plcmt_op_sel_clr_fallback(
    struct p4_pd_plcmt_info *info, bf_dev_pipe_t pipe) {
  return pipe_set_one_plcmt_op_sel_clr_fallback((struct pipe_plcmt_info *)info,
                                                pipe);
}

p4_pd_status_t p4_pd_process_plcmt_info(p4_pd_sess_hdl_t sess_hdl,
                                        bf_dev_id_t dev_id,
                                        p4_pd_tbl_hdl_t tbl_hdl,
                                        struct p4_pd_plcmt_info *info,
                                        uint32_t api_flags,
                                        uint32_t *processed) {
  return pipe_process_plcmt_info(sess_hdl,
                                 dev_id,
                                 tbl_hdl,
                                 (struct pipe_plcmt_info *)info,
                                 api_flags,
                                 processed);
}

p4_pd_status_t p4_pd_plcmt_set_adt_ent_hdl(void *data,
                                           p4_pd_entry_hdl_t entry_hdl) {
  return pipe_set_adt_ent_hdl_in_mat_data(data, entry_hdl);
}

p4_pd_status_t p4_pd_plcmt_set_sel_grp_hdl(void *data,
                                           p4_pd_grp_hdl_t grp_hdl) {
  return pipe_set_sel_grp_hdl_in_mat_data(data, grp_hdl);
}

p4_pd_status_t p4_pd_plcmt_set_ttl(void *data, uint32_t ttl) {
  return pipe_set_ttl_in_mat_data(data, ttl);
}
