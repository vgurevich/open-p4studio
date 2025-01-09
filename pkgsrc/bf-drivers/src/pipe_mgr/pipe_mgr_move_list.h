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


#ifndef _PIPE_MGR_MOVE_LIST_H
#define _PIPE_MGR_MOVE_LIST_H

#include <pipe_mgr/pipe_mgr_intf.h>
#include "pipe_mgr_int.h"

/*
 *
 * Types and APIs for MAT entry driver specific data.
 *
 */

enum pipe_mgr_tag_tags {
  PIPE_MGR_ENTRY_DATA_MAT = 0x10,
  PIPE_MGR_ENTRY_DATA_ADT = 0x27,
  PIPE_MGR_ENTRY_DATA_SEL = 0x12,
  PIPE_MGR_ENTRY_PACKED_DATA_MAT = 0x11,
  PIPE_MGR_ENTRY_PACKED_DATA_ADT = 0x28,
};

/* This is a dummy structure representing some of the fields stored for MAT
 * entry data.  Don't use "sizeof" on this type or try to declare arrays.
 * Always use the functions below to pack and unpack data out of this. */
struct pipe_mgr_mat_data {
  pipe_mgr_data_tag_t tag;
  uint16_t flags;
  pipe_act_fn_hdl_t act_fn_hdl;
  pipe_tbl_match_spec_t match_spec;
  pipe_action_spec_t action_spec;
  // uint64_t proxy_hash; Optional field (flag 4)
  // uint32_t ttl; Optional field (flag 1)
  // uint32_t selector_length; Optional field (flag 2)
  // uint32_t stful_seq_nu; Optional field (flag 8)
  // uint8_t match_spec_data_bytes[]; Optional field
  // uint8_t match_spec_mask_bytes[]; Optional field
  // uint8_t action_data_bytes[];
};
static inline bool mat_data_has_stful_seq_no(
    const struct pipe_mgr_mat_data *data) {
  PIPE_MGR_DBGCHK(data);
  return data ? data->flags & 8 : false;
}
static inline bool mat_data_has_proxy_hash(
    const struct pipe_mgr_mat_data *data) {
  PIPE_MGR_DBGCHK(data);
  return data ? data->flags & 4 : false;
}
static inline bool mat_data_has_ttl(const struct pipe_mgr_mat_data *data) {
  PIPE_MGR_DBGCHK(data);
  return data ? data->flags & 1 : false;
}
static inline bool mat_data_has_sel(const struct pipe_mgr_mat_data *data) {
  PIPE_MGR_DBGCHK(data);
  return data ? data->flags & 2 : false;
}
static inline size_t mat_ent_data_size_and_offsets(
    uint32_t ms_size,
    uint32_t num_action_data_bytes,
    bool has_proxy_hash,
    bool has_ttl,
    bool has_sel_len,
    bool has_stful_seq_nu,
    uint32_t *ph_off,
    uint32_t *ttl_off,
    uint32_t *sel_off,
    uint32_t *stful_seq_off,
    uint32_t *msd_off,
    uint32_t *msm_off,
    uint32_t *ad_off) {
  size_t alloc_sz;
  uint32_t offset = sizeof(struct pipe_mgr_mat_data);
  uint32_t proxy_hash_offset = 0, ttl_offset = 0, sel_offset = 0,
           stful_seq_nu_offset = 0, ms_data_offset = 0, ms_mask_offset = 0,
           as_data_offset = 0;
  if (has_proxy_hash) {
    proxy_hash_offset = (offset + 7) & ~7u;
    offset = proxy_hash_offset + 8;
  }
  if (has_ttl) {
    ttl_offset = (offset + 3) & ~3u;
    offset = ttl_offset + 4;
  }
  if (has_sel_len) {
    sel_offset = (offset + 3) & ~3u;
    offset = sel_offset + 4;
  }
  if (has_stful_seq_nu) {
    stful_seq_nu_offset = (offset + 3) & ~3u;
    offset = stful_seq_nu_offset + 4;
  }
  if (ms_size) {
    ms_data_offset = (offset + 7) & ~7u;
    offset = ms_data_offset + ms_size;
    ms_mask_offset = (offset + 7) & ~7u;
    offset = ms_mask_offset + ms_size;
  }
  if (num_action_data_bytes) {
    as_data_offset = (offset + 7) & ~7u;
    offset = as_data_offset + num_action_data_bytes;
  }
  alloc_sz = (offset + 7) & ~7u;

  *ph_off = proxy_hash_offset;
  *ttl_off = ttl_offset;
  *sel_off = sel_offset;
  *stful_seq_off = stful_seq_nu_offset;
  *msd_off = ms_data_offset;
  *msm_off = ms_mask_offset;
  *ad_off = as_data_offset;
  return alloc_sz;
}
/* Allocate a new MAT entry data object and fill in the data to store.  If
 * some fields are not needed (e.g. ttl) then pass a zero.  Note that the
 * action_spec is NOT optional. */
static inline struct pipe_mgr_mat_data *make_mat_ent_data(
    pipe_tbl_match_spec_t *match_spec,
    pipe_action_spec_t *action_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    uint32_t ttl,
    uint32_t selector_len,
    uint32_t stful_seq_nu,
    uint64_t proxy_hash) {
  uint32_t ms_size = match_spec ? match_spec->num_match_bytes : 0;
  uint32_t num_action_data_bytes = 0;
  if (action_spec) {
    if (IS_ACTION_SPEC_ACT_DATA(action_spec)) {
      num_action_data_bytes = action_spec->act_data.num_action_data_bytes;
    }
  }
  /* Compute total length of the data as well as the offsets of the optional
   * fields. */
  uint32_t proxy_hash_offset, ttl_offset, sel_offset, stful_seq_nu_offset,
      ms_data_offset, ms_mask_offset, as_data_offset;
  size_t alloc_sz = mat_ent_data_size_and_offsets(ms_size,
                                                  num_action_data_bytes,
                                                  !!proxy_hash,
                                                  !!ttl,
                                                  !!selector_len,
                                                  !!stful_seq_nu,
                                                  &proxy_hash_offset,
                                                  &ttl_offset,
                                                  &sel_offset,
                                                  &stful_seq_nu_offset,
                                                  &ms_data_offset,
                                                  &ms_mask_offset,
                                                  &as_data_offset);

  /* Allocate the memory and copy the fields into it. */
  void *data = get_pipe_mgr_ctx()->alloc_fn(alloc_sz);
  if (!data) return NULL;
  struct pipe_mgr_mat_data *mat_data = data;
  mat_data->flags = 0;
  mat_data->act_fn_hdl = act_fn_hdl;
  if (ttl) {
    *((uint32_t *)data + ttl_offset / 4) = ttl;
    mat_data->flags |= 1;
  }
  if (selector_len) {
    *((uint32_t *)data + sel_offset / 4) = selector_len;
    mat_data->flags |= 2;
  }
  if (proxy_hash) {
    *((uint64_t *)data + proxy_hash_offset / 8) = proxy_hash;
    mat_data->flags |= 4;
  }
  if (stful_seq_nu) {
    *((uint32_t *)data + stful_seq_nu_offset / 4) = stful_seq_nu;
    mat_data->flags |= 8;
  }
  if (match_spec) {
    mat_data->match_spec = *match_spec;
    if (match_spec->num_match_bytes) {
      mat_data->match_spec.match_value_bits = (uint8_t *)data + ms_data_offset;
      mat_data->match_spec.match_mask_bits = (uint8_t *)data + ms_mask_offset;
      PIPE_MGR_MEMCPY(mat_data->match_spec.match_value_bits,
                      match_spec->match_value_bits,
                      match_spec->num_match_bytes);
      PIPE_MGR_MEMCPY(mat_data->match_spec.match_mask_bits,
                      match_spec->match_mask_bits,
                      match_spec->num_match_bytes);
    } else {
      mat_data->match_spec.match_value_bits = NULL;
      mat_data->match_spec.match_mask_bits = NULL;
    }
  } else {
    pipe_tbl_match_spec_t x = {0};
    mat_data->match_spec = x;
  }

  if (action_spec) {
    mat_data->action_spec.pipe_action_datatype_bmap =
        action_spec->pipe_action_datatype_bmap;
    mat_data->action_spec.adt_ent_hdl = action_spec->adt_ent_hdl;
    mat_data->action_spec.sel_grp_hdl = action_spec->sel_grp_hdl;
    if (num_action_data_bytes) {
      mat_data->action_spec.act_data.num_valid_action_data_bits =
          action_spec->act_data.num_valid_action_data_bits;
      mat_data->action_spec.act_data.num_action_data_bytes =
          action_spec->act_data.num_action_data_bytes;
      mat_data->action_spec.act_data.action_data_bits =
          (uint8_t *)data + as_data_offset;
      if (mat_data->action_spec.act_data.action_data_bits &&
          action_spec->act_data.action_data_bits) {
        memcpy(mat_data->action_spec.act_data.action_data_bits,
               action_spec->act_data.action_data_bits,
               num_action_data_bytes);
      }
    } else {
      mat_data->action_spec.act_data.action_data_bits = NULL;
      mat_data->action_spec.act_data.num_valid_action_data_bits = 0;
      mat_data->action_spec.act_data.num_action_data_bytes = 0;
    }
    mat_data->action_spec.resource_count = action_spec->resource_count;
    if (action_spec->resource_count) {
      memcpy(mat_data->action_spec.resources,
             action_spec->resources,
             action_spec->resource_count * sizeof(pipe_res_spec_t));
    }
    if (action_spec->resource_count != PIPE_NUM_TBL_RESOURCES) {
      memset(&mat_data->action_spec
                  .resources[mat_data->action_spec.resource_count],
             0,
             (PIPE_NUM_TBL_RESOURCES - mat_data->action_spec.resource_count) *
                 sizeof(pipe_res_spec_t));
    }
  } else {
    pipe_action_spec_t x = {0};
    mat_data->action_spec = x;
  }
  mat_data->tag = PIPE_MGR_ENTRY_DATA_MAT;
  return mat_data;
}
/* Release memory allocated for MAT entry data. */
static inline void free_mat_ent_data(struct pipe_mgr_mat_data *data) {
  if (data) {
    get_pipe_mgr_ctx()->free_fn(data);
  }
}
/* Modify the data for export (clear any pointers in it). */
static inline void mat_ent_data_prep_export(struct pipe_mgr_mat_data *data) {
  PIPE_MGR_DBGCHK(data);
  if (!data) return;
  data->match_spec.match_value_bits = NULL;
  data->match_spec.match_mask_bits = NULL;
  data->action_spec.act_data.action_data_bits = NULL;
}
/* Modify the data for import (reset any pointers in it). */
static inline void mat_ent_data_prep_import(struct pipe_mgr_mat_data *data) {
  PIPE_MGR_DBGCHK(data);
  if (!data) return;
  uint32_t ms_data_off, ms_mask_off, ad_off, x = 0;
  mat_ent_data_size_and_offsets(
      data->match_spec.num_match_bytes,
      data->action_spec.act_data.num_action_data_bytes,
      mat_data_has_proxy_hash(data),
      mat_data_has_ttl(data),
      mat_data_has_sel(data),
      mat_data_has_stful_seq_no(data),
      &x,
      &x,
      &x,
      &x,
      &ms_data_off,
      &ms_mask_off,
      &ad_off);
  if (data->match_spec.num_match_bytes) {
    data->match_spec.match_value_bits = (uint8_t *)data + ms_data_off;
    data->match_spec.match_mask_bits = (uint8_t *)data + ms_mask_off;
  } else {
    data->match_spec.match_value_bits = NULL;
    data->match_spec.match_mask_bits = NULL;
  }
  if (data->action_spec.act_data.num_action_data_bytes) {
    data->action_spec.act_data.action_data_bits = (uint8_t *)data + ad_off;
  } else {
    data->action_spec.act_data.action_data_bits = NULL;
  }
}
/* Get the size of an allocated entry data. */
static inline size_t mat_ent_data_size(const struct pipe_mgr_mat_data *data) {
  PIPE_MGR_DBGCHK(data);
  if (!data) return 0;
  uint32_t x = 0;
  size_t size = mat_ent_data_size_and_offsets(
      data->match_spec.num_match_bytes,
      data->action_spec.act_data.num_action_data_bytes,
      mat_data_has_proxy_hash(data),
      mat_data_has_ttl(data),
      mat_data_has_sel(data),
      mat_data_has_stful_seq_no(data),
      &x,
      &x,
      &x,
      &x,
      &x,
      &x,
      &x);
  return size;
}
static inline void mat_ent_data_copy(const struct pipe_mgr_mat_data *src,
                                     struct pipe_mgr_mat_data *dst) {
  PIPE_MGR_DBGCHK(src);
  PIPE_MGR_DBGCHK(dst);
  if (!src || !dst) return;
  uint32_t x = 0;
  uint32_t ms_data_off, ms_mask_off, ad_off;
  size_t sz = mat_ent_data_size_and_offsets(
      src->match_spec.num_match_bytes,
      src->action_spec.act_data.num_action_data_bytes,
      mat_data_has_proxy_hash(src),
      mat_data_has_ttl(src),
      mat_data_has_sel(src),
      mat_data_has_stful_seq_no(src),
      &x,
      &x,
      &x,
      &x,
      &ms_data_off,
      &ms_mask_off,
      &ad_off);
  PIPE_MGR_MEMCPY(dst, src, sz);
  if (dst->match_spec.num_match_bytes) {
    dst->match_spec.match_value_bits = (uint8_t *)dst + ms_data_off;
    dst->match_spec.match_mask_bits = (uint8_t *)dst + ms_mask_off;
  }
  if (dst->action_spec.act_data.num_action_data_bytes) {
    dst->action_spec.act_data.action_data_bits = (uint8_t *)dst + ad_off;
  }
}
static inline size_t mat_ent_data_size_packed(
    const struct pipe_mgr_mat_data *data) {
  if (!data) return 0;
  /* The packed data has a few fixed fields at the beginning, count them first.
   */
  size_t pack_sz = sizeof data->tag + sizeof data->flags +
                   sizeof data->match_spec.num_match_bytes +
                   sizeof data->match_spec.num_valid_match_bits +
                   sizeof data->action_spec.resource_count +
                   sizeof data->action_spec.pipe_action_datatype_bmap;
  if (IS_ACTION_SPEC_ACT_DATA(&data->action_spec)) {
    pack_sz += sizeof data->action_spec.act_data.num_action_data_bytes;
    pack_sz += sizeof data->action_spec.act_data.num_valid_action_data_bits;
  }
  pack_sz += sizeof data->act_fn_hdl + sizeof data->match_spec.partition_index +
             sizeof data->match_spec.priority +
             sizeof data->action_spec.adt_ent_hdl +
             sizeof data->action_spec.sel_grp_hdl;

  /* Account for the optional fields. */
  if (mat_data_has_proxy_hash(data)) {
    pack_sz += sizeof(uint64_t);
  }
  if (mat_data_has_ttl(data)) {
    pack_sz += sizeof(uint32_t);
  }
  if (mat_data_has_sel(data)) {
    pack_sz += sizeof(uint32_t);
  }
  if (mat_data_has_stful_seq_no(data)) {
    pack_sz += sizeof(uint32_t);
  }

  /* Account for the variable length specs (match, action data, resources). */
  pack_sz += 2 * data->match_spec.num_match_bytes;
  if (IS_ACTION_SPEC_ACT_DATA(&data->action_spec)) {
    pack_sz += data->action_spec.act_data.num_action_data_bytes;
  }
  pack_sz +=
      data->action_spec.resource_count * sizeof data->action_spec.resources[0];

  return pack_sz;
}
static inline size_t mat_ent_data_size_unpacked(const void *packed_data) {
  if (!packed_data) return 0;

  /* First validate the tag in the packed data. */
  uint8_t *src = (uint8_t *)packed_data;
  struct pipe_mgr_mat_data unpacked_tmp;
  PIPE_MGR_MEMCPY(&unpacked_tmp.tag, src, sizeof unpacked_tmp.tag);
  src += sizeof unpacked_tmp.tag;
  if (unpacked_tmp.tag != PIPE_MGR_ENTRY_PACKED_DATA_MAT) return 0;

  /* Extract the match spec size, action data size, and flags from the packed
   * data.  In order to extract the action data size the action spec type must
   * first be extracted. */
  PIPE_MGR_MEMCPY(&unpacked_tmp.flags, src, sizeof unpacked_tmp.flags);
  src += sizeof unpacked_tmp.flags;
  PIPE_MGR_MEMCPY(&unpacked_tmp.match_spec.num_match_bytes,
                  src,
                  sizeof unpacked_tmp.match_spec.num_match_bytes);
  src += sizeof unpacked_tmp.match_spec.num_match_bytes;
  PIPE_MGR_MEMCPY(&unpacked_tmp.match_spec.num_valid_match_bits,
                  src,
                  sizeof unpacked_tmp.match_spec.num_valid_match_bits);
  src += sizeof unpacked_tmp.match_spec.num_valid_match_bits;
  PIPE_MGR_MEMCPY(&unpacked_tmp.action_spec.resource_count,
                  src,
                  sizeof unpacked_tmp.action_spec.resource_count);
  src += sizeof unpacked_tmp.action_spec.resource_count;
  PIPE_MGR_MEMCPY(&unpacked_tmp.action_spec.pipe_action_datatype_bmap,
                  src,
                  sizeof unpacked_tmp.action_spec.pipe_action_datatype_bmap);
  src += sizeof unpacked_tmp.action_spec.pipe_action_datatype_bmap;
  if (IS_ACTION_SPEC_ACT_DATA(&unpacked_tmp.action_spec)) {
    PIPE_MGR_MEMCPY(
        &unpacked_tmp.action_spec.act_data.num_action_data_bytes,
        src,
        sizeof unpacked_tmp.action_spec.act_data.num_action_data_bytes);
    src += sizeof unpacked_tmp.action_spec.act_data.num_action_data_bytes;
    PIPE_MGR_MEMCPY(
        &unpacked_tmp.action_spec.act_data.num_valid_action_data_bits,
        src,
        sizeof unpacked_tmp.action_spec.act_data.num_valid_action_data_bits);
    src += sizeof unpacked_tmp.action_spec.act_data.num_valid_action_data_bits;
  } else {
    unpacked_tmp.action_spec.act_data.num_action_data_bytes = 0;
    unpacked_tmp.action_spec.act_data.num_valid_action_data_bits = 0;
  }

  /* Now that all the required information has been extracted, compute the size
   * of the unpacked data. */
  uint32_t unused;
  size_t unpacked_sz = mat_ent_data_size_and_offsets(
      unpacked_tmp.match_spec.num_match_bytes,
      unpacked_tmp.action_spec.act_data.num_action_data_bytes,
      mat_data_has_proxy_hash(&unpacked_tmp),
      mat_data_has_ttl(&unpacked_tmp),
      mat_data_has_sel(&unpacked_tmp),
      mat_data_has_stful_seq_no(&unpacked_tmp),
      &unused,
      &unused,
      &unused,
      &unused,
      &unused,
      &unused,
      &unused);
  return unpacked_sz;
}
static inline bf_status_t mat_ent_data_pack(
    const struct pipe_mgr_mat_data *unpacked_src, void *packed_dst) {
  if (!unpacked_src || !packed_dst) return BF_INVALID_ARG;
  /* Pack the entry data into dst in the following order:
   *  tag
   *  flags
   *  match_spec.num_match_bytes
   *  match_spec.num_valid_match_bits
   *  action_spec.resource_count
   *  action_spec.pipe_action_datatype_bmap
   *  action_spec.act_data.num_action_data_bytes (if act-data type)
   *  action_spec.act_data.num_valid_action_data_bits (if act-data type)
   *
   *  act_fn_hdl
   *  match_spec.partition_index
   *  match_spec.priority
   *  action_spec.adt_ent_hdl
   *  action_spec.sel_grp_hdl
   *
   *  mat_data optional fields (proxy hash, ttl, etc.)
   *  match_spec.match_value_bits (contents, not pointer)
   *  match_spec.match_mask_bits (contents, not pointer)
   *  action_spec.act_data.action_data_bits
   *  action_spec.resources (only valid resources)
   */
  const uint8_t *src = (const uint8_t *)unpacked_src;
  uint8_t *dst = (uint8_t *)packed_dst;
  uint32_t ms_size = unpacked_src->match_spec.num_match_bytes;
  bool has_act_data = IS_ACTION_SPEC_ACT_DATA(&unpacked_src->action_spec);
  uint16_t as_size =
      has_act_data ? unpacked_src->action_spec.act_data.num_action_data_bytes
                   : 0;
  bool has_proxy_hash = mat_data_has_proxy_hash(unpacked_src);
  bool has_ttl = mat_data_has_ttl(unpacked_src);
  bool has_sel = mat_data_has_sel(unpacked_src);
  bool has_stful = mat_data_has_stful_seq_no(unpacked_src);
  uint32_t ph_off = 0;
  uint32_t ttl_off = 0;
  uint32_t sel_off = 0;
  uint32_t stful_seq_off = 0;
  uint32_t msd_off = 0;
  uint32_t msm_off = 0;
  uint32_t ad_off = 0;
  mat_ent_data_size_and_offsets(ms_size,
                                as_size,
                                has_proxy_hash,
                                has_ttl,
                                has_sel,
                                has_stful,
                                &ph_off,
                                &ttl_off,
                                &sel_off,
                                &stful_seq_off,
                                &msd_off,
                                &msm_off,
                                &ad_off);
  /* Update the tag in the packed copy */
  pipe_mgr_data_tag_t packed_tag = PIPE_MGR_ENTRY_PACKED_DATA_MAT;
  PIPE_MGR_MEMCPY(dst, &packed_tag, sizeof packed_tag);
  dst = dst + sizeof packed_tag;
  /* Copy the flags */
  PIPE_MGR_MEMCPY(dst, &unpacked_src->flags, sizeof unpacked_src->flags);
  dst = dst + sizeof unpacked_src->flags;
  /* Copy match spec key/mask size */
  PIPE_MGR_MEMCPY(dst,
                  &unpacked_src->match_spec.num_match_bytes,
                  sizeof unpacked_src->match_spec.num_match_bytes);
  dst += sizeof unpacked_src->match_spec.num_match_bytes;
  PIPE_MGR_MEMCPY(dst,
                  &unpacked_src->match_spec.num_valid_match_bits,
                  sizeof unpacked_src->match_spec.num_valid_match_bits);
  dst += sizeof unpacked_src->match_spec.num_valid_match_bits;
  /* Copy the number of resources */
  PIPE_MGR_MEMCPY(dst,
                  &unpacked_src->action_spec.resource_count,
                  sizeof unpacked_src->action_spec.resource_count);
  dst += sizeof unpacked_src->action_spec.resource_count;
  /* Copy the action data type */
  PIPE_MGR_MEMCPY(dst,
                  &unpacked_src->action_spec.pipe_action_datatype_bmap,
                  sizeof unpacked_src->action_spec.pipe_action_datatype_bmap);
  dst += sizeof unpacked_src->action_spec.pipe_action_datatype_bmap;
  /* Copy the size of the action data if this uses action data. */
  if (has_act_data) {
    PIPE_MGR_MEMCPY(
        dst,
        &unpacked_src->action_spec.act_data.num_action_data_bytes,
        sizeof unpacked_src->action_spec.act_data.num_action_data_bytes);
    dst += sizeof unpacked_src->action_spec.act_data.num_action_data_bytes;
    PIPE_MGR_MEMCPY(
        dst,
        &unpacked_src->action_spec.act_data.num_valid_action_data_bits,
        sizeof unpacked_src->action_spec.act_data.num_valid_action_data_bits);
    dst += sizeof unpacked_src->action_spec.act_data.num_valid_action_data_bits;
  }

  /* Now copy the fixed fields. */
  PIPE_MGR_MEMCPY(
      dst, &unpacked_src->act_fn_hdl, sizeof unpacked_src->act_fn_hdl);
  dst = dst + sizeof unpacked_src->act_fn_hdl;
  PIPE_MGR_MEMCPY(dst,
                  &unpacked_src->match_spec.partition_index,
                  sizeof unpacked_src->match_spec.partition_index);
  dst += sizeof unpacked_src->match_spec.partition_index;
  PIPE_MGR_MEMCPY(dst,
                  &unpacked_src->match_spec.priority,
                  sizeof unpacked_src->match_spec.priority);
  dst += sizeof unpacked_src->match_spec.priority;
  PIPE_MGR_MEMCPY(dst,
                  &unpacked_src->action_spec.adt_ent_hdl,
                  sizeof unpacked_src->action_spec.adt_ent_hdl);
  dst += sizeof unpacked_src->action_spec.adt_ent_hdl;
  PIPE_MGR_MEMCPY(dst,
                  &unpacked_src->action_spec.sel_grp_hdl,
                  sizeof unpacked_src->action_spec.sel_grp_hdl);
  dst += sizeof unpacked_src->action_spec.sel_grp_hdl;

  /* Copy the optional fields based on the flags */
  if (has_proxy_hash) {
    PIPE_MGR_MEMCPY(dst, src + ph_off, sizeof(uint64_t));
    dst += sizeof(uint64_t);
  }
  if (has_ttl) {
    PIPE_MGR_MEMCPY(dst, src + ttl_off, sizeof(uint32_t));
    dst += sizeof(uint32_t);
  }
  if (has_sel) {
    PIPE_MGR_MEMCPY(dst, src + sel_off, sizeof(uint32_t));
    dst += sizeof(uint32_t);
  }
  if (has_stful) {
    PIPE_MGR_MEMCPY(dst, src + stful_seq_off, sizeof(uint32_t));
    dst += sizeof(uint32_t);
  }

  /* Copy the match spec key/mask if present. */
  if (unpacked_src->match_spec.num_match_bytes) {
    int cp_sz = unpacked_src->match_spec.num_match_bytes;
    PIPE_MGR_MEMCPY(dst, src + msd_off, cp_sz);
    dst += cp_sz;
    PIPE_MGR_MEMCPY(dst, src + msm_off, cp_sz);
    dst += cp_sz;
  }

  /* Copy the action data if present. */
  if (has_act_data && as_size) {
    PIPE_MGR_MEMCPY(dst, src + ad_off, as_size);
    dst += as_size;
  }

  /* Lastly, copy the resources if they are valid. */
  if (unpacked_src->action_spec.resource_count) {
    PIPE_MGR_MEMCPY(dst,
                    &unpacked_src->action_spec.resources[0],
                    unpacked_src->action_spec.resource_count *
                        sizeof unpacked_src->action_spec.resources[0]);
    dst += unpacked_src->action_spec.resource_count *
           sizeof unpacked_src->action_spec.resources[0];
  }
  return BF_SUCCESS;
}
static inline bf_status_t mat_ent_data_unpack(
    const void *packed_src, struct pipe_mgr_mat_data *unpacked_dst) {
  if (!packed_src || !unpacked_dst) return BF_INVALID_ARG;
  if (PIPE_MGR_ENTRY_PACKED_DATA_MAT != *(pipe_mgr_data_tag_t *)packed_src) {
    return BF_INVALID_ARG;
  }
  const uint8_t *src = (const uint8_t *)packed_src;

  /* Set tag */
  unpacked_dst->tag = PIPE_MGR_ENTRY_DATA_MAT;
  src += sizeof unpacked_dst->tag;
  /* Extract flags */
  PIPE_MGR_MEMCPY(&unpacked_dst->flags, src, sizeof unpacked_dst->flags);
  src += sizeof unpacked_dst->flags;
  /* Extract match spec size */
  PIPE_MGR_MEMCPY(&unpacked_dst->match_spec.num_match_bytes,
                  src,
                  sizeof unpacked_dst->match_spec.num_match_bytes);
  src += sizeof unpacked_dst->match_spec.num_match_bytes;
  PIPE_MGR_MEMCPY(&unpacked_dst->match_spec.num_valid_match_bits,
                  src,
                  sizeof unpacked_dst->match_spec.num_valid_match_bits);
  src += sizeof unpacked_dst->match_spec.num_valid_match_bits;
  int ms_size = unpacked_dst->match_spec.num_match_bytes;
  /* Extract resource count */
  PIPE_MGR_MEMCPY(&unpacked_dst->action_spec.resource_count,
                  src,
                  sizeof unpacked_dst->action_spec.resource_count);
  src += sizeof unpacked_dst->action_spec.resource_count;
  /* Extract action spec type */
  PIPE_MGR_MEMCPY(&unpacked_dst->action_spec.pipe_action_datatype_bmap,
                  src,
                  sizeof unpacked_dst->action_spec.pipe_action_datatype_bmap);
  src += sizeof unpacked_dst->action_spec.pipe_action_datatype_bmap;
  bool has_act_data = IS_ACTION_SPEC_ACT_DATA(&unpacked_dst->action_spec);
  /* Extract action data size */
  uint16_t ad_sz = 0;
  if (has_act_data) {
    PIPE_MGR_MEMCPY(
        &unpacked_dst->action_spec.act_data.num_action_data_bytes,
        src,
        sizeof unpacked_dst->action_spec.act_data.num_action_data_bytes);
    src += sizeof unpacked_dst->action_spec.act_data.num_action_data_bytes;
    PIPE_MGR_MEMCPY(
        &unpacked_dst->action_spec.act_data.num_valid_action_data_bits,
        src,
        sizeof unpacked_dst->action_spec.act_data.num_valid_action_data_bits);
    src += sizeof unpacked_dst->action_spec.act_data.num_valid_action_data_bits;
    ad_sz = unpacked_dst->action_spec.act_data.num_action_data_bytes;
  }
  /* Extract action function handle */
  PIPE_MGR_MEMCPY(
      &unpacked_dst->act_fn_hdl, src, sizeof unpacked_dst->act_fn_hdl);
  src += sizeof unpacked_dst->act_fn_hdl;
  /* Extract partition index */
  PIPE_MGR_MEMCPY(&unpacked_dst->match_spec.partition_index,
                  src,
                  sizeof unpacked_dst->match_spec.partition_index);
  src += sizeof unpacked_dst->match_spec.partition_index;
  /* Extract priority */
  PIPE_MGR_MEMCPY(&unpacked_dst->match_spec.priority,
                  src,
                  sizeof unpacked_dst->match_spec.priority);
  src += sizeof unpacked_dst->match_spec.priority;
  /* Unpack the action spec's ADT entry handle */
  PIPE_MGR_MEMCPY(&unpacked_dst->action_spec.adt_ent_hdl,
                  src,
                  sizeof unpacked_dst->action_spec.adt_ent_hdl);
  src += sizeof unpacked_dst->action_spec.adt_ent_hdl;
  /* Unpack the action spec's SEL group handle */
  PIPE_MGR_MEMCPY(&unpacked_dst->action_spec.sel_grp_hdl,
                  src,
                  sizeof unpacked_dst->action_spec.sel_grp_hdl);
  src += sizeof unpacked_dst->action_spec.sel_grp_hdl;

  /* Get the offsets for all the optional/variable fields in the destination so
   * we can fill them in. */
  bool has_proxy_hash = mat_data_has_proxy_hash(unpacked_dst);
  bool has_ttl = mat_data_has_ttl(unpacked_dst);
  bool has_sel = mat_data_has_sel(unpacked_dst);
  bool has_stful = mat_data_has_stful_seq_no(unpacked_dst);
  uint32_t ph_off = 0;
  uint32_t ttl_off = 0;
  uint32_t sel_off = 0;
  uint32_t stful_seq_off = 0;
  uint32_t msd_off = 0;
  uint32_t msm_off = 0;
  uint32_t ad_off = 0;
  mat_ent_data_size_and_offsets(ms_size,
                                ad_sz,
                                has_proxy_hash,
                                has_ttl,
                                has_sel,
                                has_stful,
                                &ph_off,
                                &ttl_off,
                                &sel_off,
                                &stful_seq_off,
                                &msd_off,
                                &msm_off,
                                &ad_off);
  /* Extract the optional fields: proxy hash, ttl, selector length,
   * stful-seq-number */
  if (has_proxy_hash) {
    PIPE_MGR_MEMCPY((uint8_t *)unpacked_dst + ph_off, src, sizeof(uint64_t));
    src += sizeof(uint64_t);
  }
  if (has_ttl) {
    PIPE_MGR_MEMCPY((uint8_t *)unpacked_dst + ttl_off, src, sizeof(uint32_t));
    src += sizeof(uint32_t);
  }
  if (has_sel) {
    PIPE_MGR_MEMCPY((uint8_t *)unpacked_dst + sel_off, src, sizeof(uint32_t));
    src += sizeof(uint32_t);
  }
  if (has_stful) {
    PIPE_MGR_MEMCPY(
        (uint8_t *)unpacked_dst + stful_seq_off, src, sizeof(uint32_t));
    src += sizeof(uint32_t);
  }

  /* Unpack the match spec's key and mask */
  PIPE_MGR_MEMCPY((uint8_t *)unpacked_dst + msd_off, src, ms_size);
  src += ms_size;
  PIPE_MGR_MEMCPY((uint8_t *)unpacked_dst + msm_off, src, ms_size);
  src += ms_size;

  /* Unpack the action data if the entry has any. */
  if (has_act_data && ad_sz) {
    PIPE_MGR_MEMCPY((uint8_t *)unpacked_dst + ad_off, src, ad_sz);
    src += ad_sz;
  }
  /* Unpack the resources. */
  if (unpacked_dst->action_spec.resource_count) {
    PIPE_MGR_MEMCPY(unpacked_dst->action_spec.resources,
                    src,
                    unpacked_dst->action_spec.resource_count *
                        sizeof unpacked_dst->action_spec.resources[0]);
  }
  return BF_SUCCESS;
}
/*
 * A few unpack functions for various fields.
 */
static inline pipe_tbl_match_spec_t *unpack_mat_ent_data_ms(
    struct pipe_mgr_mat_data *data) {
  PIPE_MGR_DBGCHK(data);
  return data ? &data->match_spec : NULL;
}
static inline pipe_action_spec_t *unpack_mat_ent_data_as(
    struct pipe_mgr_mat_data *data) {
  PIPE_MGR_DBGCHK(data);
  return data ? &data->action_spec : NULL;
}
static inline pipe_act_fn_hdl_t unpack_mat_ent_data_afun_hdl(
    struct pipe_mgr_mat_data *data) {
  PIPE_MGR_DBGCHK(data);
  return data ? data->act_fn_hdl : 0;
}
static inline uint64_t unpack_mat_ent_data_phash(
    struct pipe_mgr_mat_data *data) {
  PIPE_MGR_DBGCHK(data);
  void *x = data;
  if (x && mat_data_has_proxy_hash(data)) {
    return *((uint64_t *)x + sizeof(struct pipe_mgr_mat_data) / 8);
  } else {
    return 0;
  }
}
static inline uint32_t unpack_mat_ent_data_ttl(struct pipe_mgr_mat_data *data) {
  if (!data) {
    PIPE_MGR_DBGCHK(data);
    return 0;
  }
  void *x = data;
  if (mat_data_has_ttl(data)) {
    return *((uint32_t *)x + (sizeof(struct pipe_mgr_mat_data) +
                              (mat_data_has_proxy_hash(data) ? 8 : 0)) /
                                 4);
  } else {
    return 0;
  }
}

static inline void set_mat_ent_data_ttl(struct pipe_mgr_mat_data *data,
                                        uint32_t ttl) {
  if (!data) {
    PIPE_MGR_DBGCHK(data);
    return;
  }
  void *x = data;
  if (mat_data_has_ttl(data)) {
    *((uint32_t *)x + (sizeof(struct pipe_mgr_mat_data) +
                       (mat_data_has_proxy_hash(data) ? 8 : 0)) /
                          4) = ttl;
  }
}

static inline void set_mat_ent_data_priority(struct pipe_mgr_mat_data *data,
                                             uint32_t priority) {
  PIPE_MGR_DBGCHK(data);
  if (!data) return;
  data->match_spec.priority = priority;
}

static inline uint32_t unpack_mat_ent_data_sel(struct pipe_mgr_mat_data *data) {
  if (!data) {
    PIPE_MGR_DBGCHK(data);
    return 0;
  }
  void *x = data;
  if (mat_data_has_sel(data)) {
    return *((uint32_t *)x + (sizeof(struct pipe_mgr_mat_data) +
                              (mat_data_has_ttl(data) ? 4 : 0) +
                              (mat_data_has_proxy_hash(data) ? 8 : 0)) /
                                 4);
  } else {
    return 0;
  }
}
static inline uint32_t unpack_mat_ent_data_stful(
    struct pipe_mgr_mat_data *data) {
  if (!data) {
    PIPE_MGR_DBGCHK(data);
    return 0;
  }
  void *x = data;
  if (mat_data_has_stful_seq_no(data)) {
    return *((uint32_t *)x + (sizeof(struct pipe_mgr_mat_data) +
                              (mat_data_has_ttl(data) ? 4 : 0) +
                              (mat_data_has_sel(data) ? 4 : 0) +
                              (mat_data_has_proxy_hash(data) ? 8 : 0)) /
                                 4);
  } else {
    return 0;
  }
}

/*
 *
 * Types and APIs for MAT move lists.
 *
 */
typedef struct pipe_mgr_move_list_t {
  /* Pointers to support a single linked list of these. */
  struct pipe_mgr_move_list_t *next;
  /* Driver specific data:
   * Match spec, action spec, resource configuration */
  struct pipe_mgr_mat_data *data;
  /* Entry handle being affected by this operation. */
  pipe_ent_hdl_t entry_hdl;
  /* Action entry handle pointed to by this match entry. Applicable only
   * for match tables indirectly addressing an action table.
   */
  pipe_adt_ent_hdl_t adt_ent_hdl;
  /* A flag to indicate whether the action entry handle is valid */
  /* Logical index used for indirect action. */
  bool adt_ent_hdl_valid;
  /* Logical index of the selection group. */
  pipe_idx_t logical_sel_idx;
  /* Logical action index of the indirectly addressed action entry.
   * Applicable only for indirectly referenced action tables. Set to
   * PIPE_MGR_ACT_LOGICAL_IDX_INVALID if invalid.
   */
  pipe_idx_t logical_action_idx;
  /* Number of selector words in hardware. Applicable only if the the match
   * entry points to a selector group. ZERO otherwise.
   */
  uint32_t selector_len;
  /* What type of operation we're doing: add, delete, move, modify, etc. */
  uint8_t op; /* enum pipe_mat_update_type */
  bf_dev_pipe_t pipe;
  uint16_t padding;
  /* Union of location information.  In the simple case (single) just a single
   * logical index.  In the complex case (multi, for wide TCAM ranges) a list
   * of base logical indexes and a count of how many consecutive indexes at
   * each base index. */
  union {
    struct {
      /* Logical index involved in the operation.
       * Destination index for moves and adds.
       * Not used for deletes and modifications. */
      pipe_idx_t logical_idx;
    } single;
    struct {
      unsigned int array_sz;
      struct pipe_multi_index *locations;
    } multi;
  } u;
  /* In case of a modification changing the mat_data this is the old data
   * that needs to be freed. */
  struct pipe_mgr_mat_data *old_data;
} pipe_mgr_move_list_t;

static inline pipe_mgr_move_list_t *alloc_move_list(
    pipe_mgr_move_list_t *ml,
    enum pipe_mat_update_type op,
    bf_dev_pipe_t pipe) {
  pipe_mgr_move_list_t *x = PIPE_MGR_MALLOC(sizeof(pipe_mgr_move_list_t));
  if (x) {
    x->next = NULL;
    x->data = NULL;
    x->entry_hdl = 0;
    x->adt_ent_hdl = 0;
    x->adt_ent_hdl_valid = false;
    x->logical_sel_idx = 0;
    x->logical_action_idx = 0;
    x->selector_len = 0;
    x->op = op;
    x->pipe = pipe;
    x->padding = 0;
    x->u.single.logical_idx = 0;
    x->u.multi.array_sz = 0;
    x->u.multi.locations = NULL;
    x->old_data = NULL;
    if (ml) ml->next = x;
  }
  return x;
}

static inline bf_dev_pipe_t get_move_list_pipe(pipe_mgr_move_list_t *m) {
  PIPE_MGR_DBGCHK(m);
  return !m ? BF_DEV_PIPE_ALL : m->pipe;
}

static inline void free_one_move_list_node_and_data_(pipe_mgr_move_list_t *ml,
                                                     bool failure_case,
                                                     bool cleanup,
                                                     bool free_old_mod_data) {
  PIPE_MGR_DBGCHK(ml);
  if (!ml) return;
  switch (ml->op) {
    case PIPE_MAT_UPDATE_ADD:
      if (failure_case) free_mat_ent_data(ml->data);
      break;
    case PIPE_MAT_UPDATE_ADD_MULTI:
      if (failure_case) free_mat_ent_data(ml->data);
      if (ml->u.multi.array_sz && ml->u.multi.locations)
        PIPE_MGR_FREE(ml->u.multi.locations);
      break;
    case PIPE_MAT_UPDATE_SET_DFLT:
      if (failure_case) free_mat_ent_data(ml->data);
      break;
    case PIPE_MAT_UPDATE_CLR_DFLT:
      if (!failure_case && cleanup) free_mat_ent_data(ml->data);
      break;
    case PIPE_MAT_UPDATE_DEL:
      if (!failure_case && cleanup) free_mat_ent_data(ml->data);
      break;
    case PIPE_MAT_UPDATE_MOD:
      if (failure_case) free_mat_ent_data(ml->data);
      if (!failure_case && cleanup && free_old_mod_data) {
        free_mat_ent_data(ml->old_data);
      }
      break;
    case PIPE_MAT_UPDATE_MOV:
      break;
    case PIPE_MAT_UPDATE_MOV_MULTI:
      if (ml->u.multi.array_sz && ml->u.multi.locations)
        PIPE_MGR_FREE(ml->u.multi.locations);
      break;
    case PIPE_MAT_UPDATE_MOV_MOD:
      if (ml->old_data) {
        /* Atomic modify move */
        if (failure_case) free_mat_ent_data(ml->data);
        if (!failure_case && cleanup && free_old_mod_data) {
          free_mat_ent_data(ml->old_data);
        }
      }
      break;
    case PIPE_MAT_UPDATE_MOV_MULTI_MOD:
      if (ml->old_data) {
        /* Atomic modify move */
        if (failure_case) free_mat_ent_data(ml->data);
        if (!failure_case && cleanup && free_old_mod_data) {
          free_mat_ent_data(ml->old_data);
        }
      }
      if (ml->u.multi.array_sz && ml->u.multi.locations)
        PIPE_MGR_FREE(ml->u.multi.locations);
      break;
  }
  PIPE_MGR_FREE(ml);
  return;
}
static inline void free_move_list_and_data_(pipe_mgr_move_list_t **move_list,
                                            bool failure_case,
                                            bool cleanup,
                                            bool free_old_mod_data) {
  PIPE_MGR_DBGCHK(move_list);
  if (!move_list) return;
  pipe_mgr_move_list_t *ml = *move_list;
  while (ml) {
    pipe_mgr_move_list_t *x = ml;
    ml = ml->next;
    free_one_move_list_node_and_data_(
        x, failure_case, cleanup, free_old_mod_data);
  }
  *move_list = NULL;
}
static inline void free_move_list_and_data(pipe_mgr_move_list_t **move_list,
                                           bool free_old_mod_data) {
  bool clean_up = get_pipe_mgr_ctx()->free_fn == bf_sys_free;
  free_move_list_and_data_(move_list, true, clean_up, free_old_mod_data);
}
static inline void free_move_list(pipe_mgr_move_list_t **move_list,
                                  bool free_old_mod_data) {
  bool clean_up = get_pipe_mgr_ctx()->free_fn == bf_sys_free;
  free_move_list_and_data_(move_list, false, clean_up, free_old_mod_data);
}

static inline void free_one_move_list_node_and_data(
    pipe_mgr_move_list_t **move_list, bool free_old_mod_data) {
  bool clean_up = get_pipe_mgr_ctx()->free_fn == bf_sys_free;
  free_one_move_list_node_and_data_(
      *move_list, true, clean_up, free_old_mod_data);
  *move_list = NULL;
}

/*
 *
 * Types and APIs for ADT entry driver specific data.
 *
 */

/* Packed format:
 *   tag, num_resources, const, act_fn_hdl, action_data.num_action_data_bytes,
 *   action_data.num_valid_action_data_bits, action data, resource specs */
static inline pipe_mgr_adt_ent_data_t *make_adt_ent_data(
    pipe_action_data_spec_t *action_data,
    pipe_act_fn_hdl_t act_fn_hdl,
    int num_resources,
    int is_const,
    adt_data_resources_t *resources) {
  /* Compute required allocation size to hold all fields. */
  size_t alloc_sz = 0;
  alloc_sz += sizeof(pipe_mgr_adt_ent_data_t);
  alloc_sz += action_data ? action_data->num_action_data_bytes : 0;
  uint32_t as_data_offset = sizeof(pipe_mgr_adt_ent_data_t);
  void *data = get_pipe_mgr_ctx()->alloc_fn(alloc_sz);
  if (!data) return NULL;
  pipe_mgr_adt_ent_data_t *adt_data = data;
  adt_data->act_fn_hdl = act_fn_hdl;
  adt_data->action_data.num_valid_action_data_bits =
      action_data ? action_data->num_valid_action_data_bits : 0;
  adt_data->action_data.num_action_data_bytes =
      action_data ? action_data->num_action_data_bytes : 0;
  if (adt_data->action_data.num_action_data_bytes) {
    adt_data->action_data.action_data_bits = (uint8_t *)data + as_data_offset;
    PIPE_MGR_MEMCPY(adt_data->action_data.action_data_bits,
                    action_data->action_data_bits,
                    action_data->num_action_data_bytes);
  } else {
    adt_data->action_data.action_data_bits = NULL;
  }
  adt_data->num_resources = num_resources;
  if (adt_data->num_resources) {
    PIPE_MGR_MEMCPY(adt_data->adt_data_resources,
                    resources,
                    num_resources * sizeof(adt_data_resources_t));
  }
  adt_data->tag = PIPE_MGR_ENTRY_DATA_ADT;
  adt_data->is_const = is_const;
  return adt_data;
}
/* Release memory allocated for ADT entry data. */
static inline void free_adt_ent_data(pipe_mgr_adt_ent_data_t *data) {
  if (data) get_pipe_mgr_ctx()->free_fn(data);
}
/* Modify the data for export (clear any pointers in it). */
static inline void adt_ent_data_prep_export(pipe_mgr_adt_ent_data_t *data) {
  PIPE_MGR_DBGCHK(data);
  if (data) data->action_data.action_data_bits = NULL;
}
/* Modify the data for import (reset any pointers in it). */
static inline void adt_ent_data_prep_import(pipe_mgr_adt_ent_data_t *data) {
  PIPE_MGR_DBGCHK(data);
  if (!data) return;
  if (data->action_data.num_action_data_bytes) {
    data->action_data.action_data_bits =
        (uint8_t *)data + sizeof(pipe_mgr_adt_ent_data_t);
  } else {
    data->action_data.action_data_bits = NULL;
  }
}
/* Get the size of an allocated ADT entry data. */
static inline size_t adt_ent_data_size(const pipe_mgr_adt_ent_data_t *data) {
  PIPE_MGR_DBGCHK(data);
  if (!data) return 0;
  size_t size = sizeof(pipe_mgr_adt_ent_data_t);
  size += data->action_data.num_action_data_bytes;
  return size;
}
static inline void adt_ent_data_copy(const pipe_mgr_adt_ent_data_t *src,
                                     pipe_mgr_adt_ent_data_t *dst) {
  PIPE_MGR_DBGCHK(src);
  PIPE_MGR_DBGCHK(dst);
  if (!src || !dst) return;
  PIPE_MGR_MEMCPY(dst, src, adt_ent_data_size(src));
  if (src->action_data.num_action_data_bytes) {
    dst->action_data.action_data_bits =
        (uint8_t *)dst + sizeof(pipe_mgr_adt_ent_data_t);
  }
}
static inline size_t adt_ent_data_size_packed(
    const pipe_mgr_adt_ent_data_t *data) {
  if (!data) return 0;
  size_t pack_sz = sizeof data->tag + sizeof data->num_resources +
                   sizeof data->is_const + sizeof data->act_fn_hdl +
                   sizeof data->action_data.num_action_data_bytes +
                   sizeof data->action_data.num_valid_action_data_bits +
                   data->action_data.num_action_data_bytes +
                   data->num_resources * sizeof data->adt_data_resources[0];
  return pack_sz;
}
static inline size_t adt_ent_data_size_unpacked(const void *packed_data) {
  if (!packed_data) return 0;

  /* First validate the tag in the packed data. */
  uint8_t *src = (uint8_t *)packed_data;
  pipe_mgr_adt_ent_data_t unpacked_tmp;
  PIPE_MGR_MEMCPY(&unpacked_tmp.tag, src, sizeof unpacked_tmp.tag);
  src += sizeof unpacked_tmp.tag;
  if (unpacked_tmp.tag != PIPE_MGR_ENTRY_PACKED_DATA_ADT) return 0;

  /* Skip over the num_resources, const flag, and action function handle in the
   * packed data. */
  src += sizeof unpacked_tmp.num_resources;
  src += sizeof unpacked_tmp.is_const;
  src += sizeof unpacked_tmp.act_fn_hdl;

  /* Extract the size of the action data from the packed entry. */
  PIPE_MGR_MEMCPY(&unpacked_tmp.action_data.num_action_data_bytes,
                  src,
                  sizeof unpacked_tmp.action_data.num_action_data_bytes);

  /* Compute the unpacked size. */
  size_t unpacked_sz = sizeof unpacked_tmp;
  unpacked_sz += unpacked_tmp.action_data.num_action_data_bytes;
  return unpacked_sz;
}
static inline bf_status_t adt_ent_data_pack(
    const pipe_mgr_adt_ent_data_t *unpacked_src, void *packed_dst) {
  if (!unpacked_src || !packed_dst) return BF_INVALID_ARG;
  uint8_t *dst = (uint8_t *)packed_dst;

  pipe_mgr_data_tag_t tag = PIPE_MGR_ENTRY_PACKED_DATA_ADT;
  PIPE_MGR_MEMCPY(dst, &tag, sizeof tag);
  dst += sizeof tag;
  PIPE_MGR_MEMCPY(
      dst, &unpacked_src->num_resources, sizeof unpacked_src->num_resources);
  dst += sizeof unpacked_src->num_resources;
  PIPE_MGR_MEMCPY(dst, &unpacked_src->is_const, sizeof unpacked_src->is_const);
  dst += sizeof unpacked_src->is_const;
  PIPE_MGR_MEMCPY(
      dst, &unpacked_src->act_fn_hdl, sizeof unpacked_src->act_fn_hdl);
  dst += sizeof unpacked_src->act_fn_hdl;
  PIPE_MGR_MEMCPY(dst,
                  &unpacked_src->action_data.num_action_data_bytes,
                  sizeof unpacked_src->action_data.num_action_data_bytes);
  dst += sizeof unpacked_src->action_data.num_action_data_bytes;
  PIPE_MGR_MEMCPY(dst,
                  &unpacked_src->action_data.num_valid_action_data_bits,
                  sizeof unpacked_src->action_data.num_valid_action_data_bits);
  dst += sizeof unpacked_src->action_data.num_valid_action_data_bits;
  if (unpacked_src->action_data.num_valid_action_data_bits) {
    int num_bytes = unpacked_src->action_data.num_action_data_bytes;
    uint8_t *act_data_src =
        (uint8_t *)unpacked_src + sizeof(pipe_mgr_adt_ent_data_t);
    PIPE_MGR_MEMCPY(dst, act_data_src, num_bytes);
    dst += num_bytes;
  }
  if (unpacked_src->num_resources) {
    int res_sz = unpacked_src->num_resources *
                 sizeof unpacked_src->adt_data_resources[0];
    PIPE_MGR_MEMCPY(dst, &unpacked_src->adt_data_resources, res_sz);
    dst += res_sz;
  }
  return BF_SUCCESS;
}
static inline bf_status_t adt_ent_data_unpack(
    const void *packed_src, pipe_mgr_adt_ent_data_t *unpacked_dst) {
  const uint8_t *src = (const uint8_t *)packed_src;
  if (!packed_src || !unpacked_dst) return BF_INVALID_ARG;

  /* Unpack the tag, verify it has the expected value in the packed data. */
  pipe_mgr_data_tag_t tag = *(pipe_mgr_data_tag_t *)src;
  if (tag != PIPE_MGR_ENTRY_PACKED_DATA_ADT) return BF_INVALID_ARG;
  unpacked_dst->tag = PIPE_MGR_ENTRY_DATA_ADT;
  src += sizeof unpacked_dst->tag;

  PIPE_MGR_MEMCPY(
      &unpacked_dst->num_resources, src, sizeof unpacked_dst->num_resources);
  src += sizeof unpacked_dst->num_resources;
  PIPE_MGR_MEMCPY(&unpacked_dst->is_const, src, sizeof unpacked_dst->is_const);
  src += sizeof unpacked_dst->is_const;
  PIPE_MGR_MEMCPY(
      &unpacked_dst->act_fn_hdl, src, sizeof unpacked_dst->act_fn_hdl);
  src += sizeof unpacked_dst->act_fn_hdl;
  PIPE_MGR_MEMCPY(&unpacked_dst->action_data.num_action_data_bytes,
                  src,
                  sizeof unpacked_dst->action_data.num_action_data_bytes);
  src += sizeof unpacked_dst->action_data.num_action_data_bytes;
  PIPE_MGR_MEMCPY(&unpacked_dst->action_data.num_valid_action_data_bits,
                  src,
                  sizeof unpacked_dst->action_data.num_valid_action_data_bits);
  src += sizeof unpacked_dst->action_data.num_valid_action_data_bits;
  size_t ad_sz = unpacked_dst->action_data.num_action_data_bytes;
  if (ad_sz) {
    uint8_t *dst = (uint8_t *)unpacked_dst + sizeof(pipe_mgr_adt_ent_data_t);
    PIPE_MGR_MEMCPY(dst, src, ad_sz);
    src += ad_sz;
    unpacked_dst->action_data.action_data_bits = dst;
  } else {
    unpacked_dst->action_data.action_data_bits = NULL;
  }
  if (unpacked_dst->num_resources) {
    PIPE_MGR_MEMCPY(unpacked_dst->adt_data_resources,
                    src,
                    unpacked_dst->num_resources *
                        sizeof unpacked_dst->adt_data_resources[0]);
  }
  return BF_SUCCESS;
}

/*
 * A few unpack functions for various fields.
 */
static inline pipe_action_data_spec_t *unpack_adt_ent_data_ad(
    pipe_mgr_adt_ent_data_t *data) {
  PIPE_MGR_DBGCHK(data);
  return data ? &data->action_data : NULL;
}
static inline pipe_act_fn_hdl_t unpack_adt_ent_data_afun_hdl(
    pipe_mgr_adt_ent_data_t *data) {
  PIPE_MGR_DBGCHK(data);
  return data ? data->act_fn_hdl : 0;
}
static inline int unpack_adt_ent_data_num_resources(
    pipe_mgr_adt_ent_data_t *data) {
  PIPE_MGR_DBGCHK(data);
  return data ? data->num_resources : 0;
}
static inline int unpack_adt_ent_data_const(pipe_mgr_adt_ent_data_t *data) {
  PIPE_MGR_DBGCHK(data);
  return data ? data->is_const : 0;
}
static inline adt_data_resources_t *unpack_adt_ent_data_resources(
    pipe_mgr_adt_ent_data_t *data) {
  PIPE_MGR_DBGCHK(data);
  return data ? data->adt_data_resources : NULL;
}
/*
 *
 * Types and APIs for ADT move lists.
 *
 */
typedef struct pipe_mgr_adt_move_list_t {
  struct pipe_mgr_adt_move_list_t *next;
  pipe_mgr_adt_ent_data_t *data;
  pipe_ent_hdl_t entry_hdl;
  bf_dev_pipe_t pipe_id;
  enum pipe_adt_update_type op;
  /* In case of a modification changing the adt_data this is the old data
   * that needs to be freed. */
  pipe_mgr_adt_ent_data_t *old_data;
  /* Applies to HA hitless mode when passing the state from LLP to HLP. */
  bool sharable;
} pipe_mgr_adt_move_list_t;

static inline pipe_mgr_adt_move_list_t *alloc_adt_move_list(
    pipe_mgr_adt_move_list_t *ml, enum pipe_adt_update_type op) {
  pipe_mgr_adt_move_list_t *x =
      PIPE_MGR_MALLOC(sizeof(pipe_mgr_adt_move_list_t));
  if (x) {
    x->next = NULL;
    x->data = NULL;
    x->entry_hdl = 0;
    x->pipe_id = ~0;
    x->op = op;
    x->old_data = NULL;
    x->sharable = false;
    if (ml) ml->next = x;
  }
  return x;
}

static inline void free_adt_move_list_and_data_(
    pipe_mgr_adt_move_list_t **move_list, bool failure_case, bool clean_up) {
  PIPE_MGR_DBGCHK(move_list);
  if (!move_list) return;
  pipe_mgr_adt_move_list_t *ml = *move_list;
  while (ml) {
    switch (ml->op) {
      case PIPE_ADT_UPDATE_ADD:
        if (failure_case) free_adt_ent_data(ml->data);
        break;
      case PIPE_ADT_UPDATE_DEL:
        if (!failure_case && clean_up) free_adt_ent_data(ml->data);
        break;
      case PIPE_ADT_UPDATE_MOD:
        if (failure_case) free_adt_ent_data(ml->data);
        if (!failure_case && clean_up) free_adt_ent_data(ml->old_data);
        break;
    }
    pipe_mgr_adt_move_list_t *x = ml;
    ml = ml->next;
    PIPE_MGR_FREE(x);
  }
  *move_list = NULL;
}
static inline void free_adt_move_list_and_data(
    pipe_mgr_adt_move_list_t **move_list) {
  bool clean_up = get_pipe_mgr_ctx()->free_fn == bf_sys_free;
  free_adt_move_list_and_data_(move_list, true, clean_up);
}
static inline void free_adt_move_list(pipe_mgr_adt_move_list_t **move_list) {
  bool clean_up = get_pipe_mgr_ctx()->free_fn == bf_sys_free;
  free_adt_move_list_and_data_(move_list, false, clean_up);
}

/*
 *
 * Types and APIs for SEL move lists.
 *
 */
struct pipe_mgr_sel_data {
  pipe_mgr_data_tag_t tag;
  uint16_t pad;
  int array_len;
  pipe_idx_t adt_index_array[1];
};
static inline struct pipe_mgr_sel_data *make_sel_ent_data(
    int array_len, pipe_idx_t *adt_index_array) {
  PIPE_MGR_DBGCHK(adt_index_array);
  size_t alloc_sz = 0;
  alloc_sz += sizeof(struct pipe_mgr_sel_data);
  alloc_sz += (array_len - 1) * sizeof(pipe_idx_t);
  void *data = get_pipe_mgr_ctx()->alloc_fn(alloc_sz);
  struct pipe_mgr_sel_data *sel_data = data;
  if (data) {
    for (int i = 0; i < array_len; ++i) {
      sel_data->adt_index_array[i] = adt_index_array ? adt_index_array[i] : 0;
    }
    sel_data->array_len = array_len;
    sel_data->tag = PIPE_MGR_ENTRY_DATA_SEL;
    sel_data->pad = 0;
  }
  return sel_data;
}

typedef struct pipe_mgr_sel_move_list_t {
  struct pipe_mgr_sel_move_list_t *next;
  struct pipe_mgr_sel_data *data;
  pipe_sel_grp_hdl_t sel_grp_hdl;
  pipe_adt_ent_hdl_t adt_mbr_hdl;
  pipe_idx_t logical_sel_index;
  pipe_idx_t logical_sel_subindex;
  bf_dev_pipe_t pipe;
  uint32_t sel_grp_size;  // Number of RAM words
  uint32_t max_mbrs;      // Number of members the group supports
  enum pipe_sel_update_type op;
  int locations_length;
  struct pipe_multi_index *locations;
  bool disabled_mbr;
  /* Member requires only handle update. No HW update.
   * Applies only to Member Add and Member Del.
   */
  bool replace_mbr_hdl;
} pipe_mgr_sel_move_list_t;

/* Release memory allocated for SEL entry data. */
static inline void free_sel_ent_data(struct pipe_mgr_sel_move_list_t *ml) {
  if (!ml->data) return;
  get_pipe_mgr_ctx()->free_fn(ml->data);
  ml->data = NULL;
}
/* Get the size of an allocated SEL entry data. */
static inline size_t sel_ent_data_size(const struct pipe_mgr_sel_data *data) {
  PIPE_MGR_DBGCHK(data);
  if (!data) return 0;
  size_t size = sizeof(struct pipe_mgr_sel_data);
  if (data->array_len > 0) {
    size += sizeof(pipe_idx_t) * (data->array_len - 1);
  }
  return size;
}
static inline void sel_ent_data_copy(const struct pipe_mgr_sel_data *src,
                                     struct pipe_mgr_sel_data *dst) {
  PIPE_MGR_DBGCHK(src);
  PIPE_MGR_DBGCHK(dst);
  if (!src || !dst) return;
  PIPE_MGR_MEMCPY(dst, src, sel_ent_data_size(src));
}
static inline size_t sel_ent_data_size_packed(
    const struct pipe_mgr_sel_data *data) {
  /* Nothing to pack, return the standard size. */
  return sel_ent_data_size(data);
}
static inline size_t sel_ent_data_size_unpacked(const void *data) {
  /* Nothing to unpack, return the standard size. */
  return sel_ent_data_size((struct pipe_mgr_sel_data *)data);
}
static inline bf_status_t sel_ent_data_pack(
    const struct pipe_mgr_sel_data *unpacked_src, void *packed_dst) {
  if (!unpacked_src || !packed_dst) return BF_INVALID_ARG;
  size_t x = sel_ent_data_size(unpacked_src);
  PIPE_MGR_MEMCPY(packed_dst, unpacked_src, x);
  return BF_SUCCESS;
}
static inline bf_status_t sel_ent_data_unpack(
    const void *packed_src, struct pipe_mgr_sel_data *unpacked_dst) {
  if (!packed_src || !unpacked_dst) return BF_INVALID_ARG;
  size_t x = sel_ent_data_size((const struct pipe_mgr_sel_data *)packed_src);
  PIPE_MGR_MEMCPY(unpacked_dst, packed_src, x);
  return BF_SUCCESS;
}

/*
 * A few unpack functions for various fields.
 */
static inline pipe_idx_t unpack_sel_ent_data_adt_index(
    struct pipe_mgr_sel_data *data, int index) {
  PIPE_MGR_DBGCHK(data);
  return data ? data->adt_index_array[index] : 0;
}

static inline pipe_mgr_sel_move_list_t *alloc_sel_move_list(
    pipe_mgr_sel_move_list_t *ml,
    enum pipe_sel_update_type op,
    bf_dev_pipe_t pipe) {
  pipe_mgr_sel_move_list_t *x =
      PIPE_MGR_MALLOC(sizeof(pipe_mgr_sel_move_list_t));
  if (x) {
    x->next = NULL;
    x->data = NULL;
    x->sel_grp_hdl = 0;
    x->adt_mbr_hdl = 0;
    x->logical_sel_index = 0;
    x->logical_sel_subindex = 0;
    x->sel_grp_size = 0;
    x->max_mbrs = 0;
    x->op = op;
    x->locations_length = 0;
    x->locations = NULL;
    x->pipe = pipe;
    x->disabled_mbr = false;
    x->replace_mbr_hdl = false;
    if (ml) ml->next = x;
  }
  return x;
}

static inline void free_sel_move_list_and_data_(
    pipe_mgr_sel_move_list_t **move_list, bool free_data, bool clean_up) {
  PIPE_MGR_DBGCHK(move_list);
  if (!move_list) return;
  pipe_mgr_sel_move_list_t *ml = *move_list;
  while (ml) {
    switch (ml->op) {
      case PIPE_SEL_UPDATE_GROUP_CREATE:
        if (ml->locations && ml->locations_length) PIPE_MGR_FREE(ml->locations);
        break;
      case PIPE_SEL_UPDATE_GROUP_DESTROY:
        break;
      case PIPE_SEL_UPDATE_ADD:
        if (free_data && clean_up) free_sel_ent_data(ml);
        break;
      case PIPE_SEL_UPDATE_DEL:
        if (free_data && clean_up) free_sel_ent_data(ml);
        break;
      case PIPE_SEL_UPDATE_ACTIVATE:
        break;
      case PIPE_SEL_UPDATE_DEACTIVATE:
        break;
      case PIPE_SEL_UPDATE_SET_FALLBACK:
        if (free_data && clean_up) free_sel_ent_data(ml);
        break;
      case PIPE_SEL_UPDATE_CLR_FALLBACK:
        break;
    }
    pipe_mgr_sel_move_list_t *x = ml;
    ml = ml->next;
    PIPE_MGR_FREE(x);
  }
  *move_list = NULL;
}
static inline void free_sel_move_list_and_data(
    pipe_mgr_sel_move_list_t **move_list) {
  bool clean_up = get_pipe_mgr_ctx()->free_fn == bf_sys_free;
  free_sel_move_list_and_data_(move_list, true, clean_up);
}
static inline void free_sel_move_list(pipe_mgr_sel_move_list_t **move_list) {
  bool clean_up = get_pipe_mgr_ctx()->free_fn == bf_sys_free;
  free_sel_move_list_and_data_(move_list, false, clean_up);
}

#endif  //_PIPE_MGR_MOVE_LIST_H
