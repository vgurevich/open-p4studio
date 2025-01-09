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


#include <math.h>
#include "mc_mgr.h"
#include <bf_types/bf_types.h>
#include <dvm/bf_dma_types.h>
#include <lld/bf_dma_if.h>
#include <lld/lld_tof_addr_conversion.h>
#include <lld/tofino_defs.h>
#include <tofino_regs/pipe_top_level.h>
#include "mc_mgr_handle.h"
#include "mc_mgr_int.h"
#include "mc_mgr_bh.h"

#define MC_MGR_RDM_NODE_TYPE_S 0
#define MC_MGR_RDM_NODE_TYPE_W 4

#define MC_MGR_RDM_NODE_L1RID_TYPE 0xD
#define MC_MGR_RDM_NODE_L1RID_L1_NEXT_S 4
#define MC_MGR_RDM_NODE_L1RID_L1_NEXT_W 20
#define MC_MGR_RDM_NODE_L1RID_L2_NEXT_S 24
#define MC_MGR_RDM_NODE_L1RID_L2_NEXT_W 20
#define MC_MGR_RDM_NODE_L1RID_RID_S 48
#define MC_MGR_RDM_NODE_L1RID_RID_W 16

#define MC_MGR_RDM_NODE_L1RIDXID_TYPE 0xE
#define MC_MGR_RDM_NODE_L1RIDXID_L1_NEXT_S 4
#define MC_MGR_RDM_NODE_L1RIDXID_L1_NEXT_W 20
#define MC_MGR_RDM_NODE_L1RIDXID_L2_NEXT_S 24
#define MC_MGR_RDM_NODE_L1RIDXID_L2_NEXT_W 20
#define MC_MGR_RDM_NODE_L1RIDXID_RID_S 48
#define MC_MGR_RDM_NODE_L1RIDXID_RID_W 16
#define MC_MGR_RDM_NODE_L1RIDXID_XID_S 64
#define MC_MGR_RDM_NODE_L1RIDXID_XID_W 16

#define MC_MGR_RDM_NODE_L1RIDEND_TYPE 0xF
#define MC_MGR_RDM_NODE_L1RIDEND_L2_NEXT_S 4
#define MC_MGR_RDM_NODE_L1RIDEND_L2_NEXT_W 20
#define MC_MGR_RDM_NODE_L1RIDEND_RID_S 24
#define MC_MGR_RDM_NODE_L1RIDEND_RID_W 16

#define MC_MGR_RDM_NODE_VEC_TYPE 4
#define MC_MGR_RDM_NODE_VEC_BASE_S 4
#define MC_MGR_RDM_NODE_VEC_BASE_W 20
#define MC_MGR_RDM_NODE_VEC_LEN_S 24
#define MC_MGR_RDM_NODE_VEC_LEN_W 5
#define MC_MGR_RDM_NODE_VEC_VEC_S 32
#define MC_MGR_RDM_NODE_VEC_VEC_W 32
#define MC_MGR_RDM_NODE_VEC_ID_S 64
#define MC_MGR_RDM_NODE_VEC_ID_W 16

#define MC_MGR_RDM_NODE_PTR_TYPE 5
#define MC_MGR_RDM_NODE_PTR_NEXT_S 4
#define MC_MGR_RDM_NODE_PTR_NEXT_W 20
#define MC_MGR_RDM_NODE_PTR_P0_S 24
#define MC_MGR_RDM_NODE_PTR_P0_W 20
#define MC_MGR_RDM_NODE_PTR_P1_S 44
#define MC_MGR_RDM_NODE_PTR_P1_W 20

#define MC_MGR_RDM_NODE_PTRXID_TYPE 6
#define MC_MGR_RDM_NODE_PTRXID_NEXT_S 4
#define MC_MGR_RDM_NODE_PTRXID_NEXT_W 20
#define MC_MGR_RDM_NODE_PTRXID_P0_S 24
#define MC_MGR_RDM_NODE_PTRXID_P0_W 20
#define MC_MGR_RDM_NODE_PTRXID_P1_S 44
#define MC_MGR_RDM_NODE_PTRXID_P1_W 20
#define MC_MGR_RDM_NODE_PTRXID_XID_S 64
#define MC_MGR_RDM_NODE_PTRXID_XID_W 16

#define MC_MGR_RDM_NODE_L2PORT18_TYPE 8
#define MC_MGR_RDM_NODE_L2PORT18_PIPE_S 4
#define MC_MGR_RDM_NODE_L2PORT18_PIPE_W 2
#define MC_MGR_RDM_NODE_L2PORT18_LAST_S 7
#define MC_MGR_RDM_NODE_L2PORT18_LAST_W 1
#define MC_MGR_RDM_NODE_L2PORT18_PVEC_LO_S 8
#define MC_MGR_RDM_NODE_L2PORT18_PVEC_LO_W 16
#define MC_MGR_RDM_NODE_L2PORT18_PVEC_HI_S (8 + 16)
#define MC_MGR_RDM_NODE_L2PORT18_PVEC_HI_W 2

#define MC_MGR_RDM_NODE_L2PORT72_TYPE 9
#define MC_MGR_RDM_NODE_L2PORT72_PIPE_S 4
#define MC_MGR_RDM_NODE_L2PORT72_PIPE_W 2
#define MC_MGR_RDM_NODE_L2PORT72_LAST_S 7
#define MC_MGR_RDM_NODE_L2PORT72_LAST_W 1
#define MC_MGR_RDM_NODE_L2PORT72_PVEC_LO_S 8
#define MC_MGR_RDM_NODE_L2PORT72_PVEC_LO_W 64
#define MC_MGR_RDM_NODE_L2PORT72_PVEC_HI_S (8 + 64)
#define MC_MGR_RDM_NODE_L2PORT72_PVEC_HI_W (72 - 64)

#define MC_MGR_RDM_NODE_L2LAG_TYPE 12
#define MC_MGR_RDM_NODE_L2LAG_NEXT_S 4
#define MC_MGR_RDM_NODE_L2LAG_NEXT_W 20
#define MC_MGR_RDM_NODE_L2LAG_INDX_S 24
#define MC_MGR_RDM_NODE_L2LAG_INDX_W 8

bf_status_t tof2_mc_mgr_rdm_decode_line(mc_mgr_rdm_line_t *line) {
  bf_bitset_t x;
  bf_bs_init(&x, 80, line->data);

  line->type[0] = mc_mgr_rdm_node_type_invalid;
  line->type[1] = mc_mgr_rdm_node_type_invalid;

  bool upper_valid = false;
  int type = bf_bs_get_word(&x, MC_MGR_RDM_NODE_TYPE_S, MC_MGR_RDM_NODE_TYPE_W);

  switch (type) {
    case 0:
      upper_valid = true;
      line->type[0] = mc_mgr_rdm_node_type_invalid;
      break;
    case MC_MGR_RDM_NODE_L1RID_TYPE:
      line->type[0] = mc_mgr_rdm_node_type_rid;
      line->u.rid.next_l1 = bf_bs_get_word(
          &x, MC_MGR_RDM_NODE_L1RID_L1_NEXT_S, MC_MGR_RDM_NODE_L1RID_L1_NEXT_W);
      line->u.rid.next_l2 = bf_bs_get_word(
          &x, MC_MGR_RDM_NODE_L1RID_L2_NEXT_S, MC_MGR_RDM_NODE_L1RID_L2_NEXT_W);
      line->u.rid.rid = bf_bs_get_word(
          &x, MC_MGR_RDM_NODE_L1RID_RID_S, MC_MGR_RDM_NODE_L1RID_RID_W);
      break;
    case MC_MGR_RDM_NODE_L1RIDXID_TYPE:
      line->type[0] = mc_mgr_rdm_node_type_xid;
      line->u.xid.next_l1 = bf_bs_get_word(&x,
                                           MC_MGR_RDM_NODE_L1RIDXID_L1_NEXT_S,
                                           MC_MGR_RDM_NODE_L1RIDXID_L1_NEXT_W);
      line->u.xid.next_l2 = bf_bs_get_word(&x,
                                           MC_MGR_RDM_NODE_L1RIDXID_L2_NEXT_S,
                                           MC_MGR_RDM_NODE_L1RIDXID_L2_NEXT_W);
      line->u.xid.rid = bf_bs_get_word(
          &x, MC_MGR_RDM_NODE_L1RIDXID_RID_S, MC_MGR_RDM_NODE_L1RIDXID_RID_W);
      line->u.xid.xid = bf_bs_get_word(
          &x, MC_MGR_RDM_NODE_L1RIDXID_XID_S, MC_MGR_RDM_NODE_L1RIDXID_XID_W);
      break;
    case MC_MGR_RDM_NODE_L1RIDEND_TYPE:
      line->type[0] = mc_mgr_rdm_node_type_end;
      upper_valid = true;
      line->u.end[0].next_l2 =
          bf_bs_get_word(&x,
                         MC_MGR_RDM_NODE_L1RIDEND_L2_NEXT_S,
                         MC_MGR_RDM_NODE_L1RIDEND_L2_NEXT_W);
      line->u.end[0].rid = bf_bs_get_word(
          &x, MC_MGR_RDM_NODE_L1RIDEND_RID_S, MC_MGR_RDM_NODE_L1RIDEND_RID_W);
      break;
    case MC_MGR_RDM_NODE_PTR_TYPE:
      line->type[0] = mc_mgr_rdm_node_type_ecmp;
      line->u.ecmp.next_l1 = bf_bs_get_word(
          &x, MC_MGR_RDM_NODE_PTR_NEXT_S, MC_MGR_RDM_NODE_PTR_NEXT_W);
      line->u.ecmp.vector0 = bf_bs_get_word(
          &x, MC_MGR_RDM_NODE_PTR_P0_S, MC_MGR_RDM_NODE_PTR_P0_W);
      line->u.ecmp.vector1 = bf_bs_get_word(
          &x, MC_MGR_RDM_NODE_PTR_P1_S, MC_MGR_RDM_NODE_PTR_P1_W);
      break;
    case MC_MGR_RDM_NODE_PTRXID_TYPE:
      line->type[0] = mc_mgr_rdm_node_type_ecmp_xid;
      line->u.ecmp_xid.next_l1 = bf_bs_get_word(
          &x, MC_MGR_RDM_NODE_PTRXID_NEXT_S, MC_MGR_RDM_NODE_PTRXID_NEXT_W);
      line->u.ecmp_xid.vector0 = bf_bs_get_word(
          &x, MC_MGR_RDM_NODE_PTRXID_P0_S, MC_MGR_RDM_NODE_PTRXID_P0_W);
      line->u.ecmp_xid.vector1 = bf_bs_get_word(
          &x, MC_MGR_RDM_NODE_PTRXID_P1_S, MC_MGR_RDM_NODE_PTRXID_P1_W);
      line->u.ecmp_xid.xid = bf_bs_get_word(
          &x, MC_MGR_RDM_NODE_PTRXID_XID_S, MC_MGR_RDM_NODE_PTRXID_XID_W);
      break;
    case MC_MGR_RDM_NODE_VEC_TYPE:
      line->type[0] = mc_mgr_rdm_node_type_vector;
      line->u.vector.base_l1 = bf_bs_get_word(
          &x, MC_MGR_RDM_NODE_VEC_BASE_S, MC_MGR_RDM_NODE_VEC_BASE_W);
      line->u.vector.length = bf_bs_get_word(
          &x, MC_MGR_RDM_NODE_VEC_LEN_S, MC_MGR_RDM_NODE_VEC_LEN_W);
      line->u.vector.vector = bf_bs_get_word(
          &x, MC_MGR_RDM_NODE_VEC_VEC_S, MC_MGR_RDM_NODE_VEC_VEC_W);
      line->u.vector.id = bf_bs_get_word(
          &x, MC_MGR_RDM_NODE_VEC_ID_S, MC_MGR_RDM_NODE_VEC_ID_W);
      break;
    case MC_MGR_RDM_NODE_L2PORT18_TYPE:
      line->type[0] = mc_mgr_rdm_node_type_port18;
      upper_valid = true;
      line->u.port18[0].pipe = bf_bs_get_word(
          &x, MC_MGR_RDM_NODE_L2PORT18_PIPE_S, MC_MGR_RDM_NODE_L2PORT18_PIPE_W);
      line->u.port18[0].last = bf_bs_get_word(
          &x, MC_MGR_RDM_NODE_L2PORT18_LAST_S, MC_MGR_RDM_NODE_L2PORT18_LAST_W);
      line->u.port18[0].ports =
          bf_bs_get_word(&x,
                         MC_MGR_RDM_NODE_L2PORT18_PVEC_LO_S,
                         MC_MGR_RDM_NODE_L2PORT18_PVEC_LO_W);
      line->u.port18[0].spv =
          bf_bs_get_word(&x,
                         MC_MGR_RDM_NODE_L2PORT18_PVEC_HI_S,
                         MC_MGR_RDM_NODE_L2PORT18_PVEC_HI_W);
      break;
    case MC_MGR_RDM_NODE_L2PORT72_TYPE:
      line->type[0] = mc_mgr_rdm_node_type_port72;
      line->u.port72.pipe = bf_bs_get_word(
          &x, MC_MGR_RDM_NODE_L2PORT72_PIPE_S, MC_MGR_RDM_NODE_L2PORT72_PIPE_W);
      line->u.port72.last = bf_bs_get_word(
          &x, MC_MGR_RDM_NODE_L2PORT72_LAST_S, MC_MGR_RDM_NODE_L2PORT72_LAST_W);
      line->u.port72.ports = bf_bs_get_word(&x,
                                            MC_MGR_RDM_NODE_L2PORT72_PVEC_LO_S,
                                            MC_MGR_RDM_NODE_L2PORT72_PVEC_LO_W);
      line->u.port72.spv = bf_bs_get_word(&x,
                                          MC_MGR_RDM_NODE_L2PORT72_PVEC_HI_S,
                                          MC_MGR_RDM_NODE_L2PORT72_PVEC_HI_W);
      break;
    case MC_MGR_RDM_NODE_L2LAG_TYPE:
      line->type[0] = mc_mgr_rdm_node_type_lag;
      upper_valid = true;
      line->u.lag[0].next_l2 = bf_bs_get_word(
          &x, MC_MGR_RDM_NODE_L2LAG_NEXT_S, MC_MGR_RDM_NODE_L2LAG_NEXT_W);
      line->u.lag[0].lag_id = bf_bs_get_word(
          &x, MC_MGR_RDM_NODE_L2LAG_INDX_S, MC_MGR_RDM_NODE_L2LAG_INDX_W);
      break;
    default:
      LOG_ERROR("Unknown node type (low half)");
      break;
  }
  if (upper_valid) {
    type =
        bf_bs_get_word(&x, MC_MGR_RDM_NODE_TYPE_S + 40, MC_MGR_RDM_NODE_TYPE_W);
    switch (type) {
      case 0:
        line->type[1] = mc_mgr_rdm_node_type_invalid;
        break;
      case MC_MGR_RDM_NODE_L1RIDEND_TYPE:
        line->type[1] = mc_mgr_rdm_node_type_end;
        line->u.end[1].next_l2 =
            bf_bs_get_word(&x,
                           MC_MGR_RDM_NODE_L1RIDEND_L2_NEXT_S + 40,
                           MC_MGR_RDM_NODE_L1RIDEND_L2_NEXT_W);
        line->u.end[1].rid = bf_bs_get_word(&x,
                                            MC_MGR_RDM_NODE_L1RIDEND_RID_S + 40,
                                            MC_MGR_RDM_NODE_L1RIDEND_RID_W);
        break;
      case MC_MGR_RDM_NODE_L2PORT18_TYPE:
        line->type[1] = mc_mgr_rdm_node_type_port18;
        line->u.port18[1].pipe =
            bf_bs_get_word(&x,
                           MC_MGR_RDM_NODE_L2PORT18_PIPE_S + 40,
                           MC_MGR_RDM_NODE_L2PORT18_PIPE_W);
        line->u.port18[1].last =
            bf_bs_get_word(&x,
                           MC_MGR_RDM_NODE_L2PORT18_LAST_S + 40,
                           MC_MGR_RDM_NODE_L2PORT18_LAST_W);
        line->u.port18[1].ports =
            bf_bs_get_word(&x,
                           MC_MGR_RDM_NODE_L2PORT18_PVEC_LO_S + 40,
                           MC_MGR_RDM_NODE_L2PORT18_PVEC_LO_W);
        line->u.port18[1].spv =
            bf_bs_get_word(&x,
                           MC_MGR_RDM_NODE_L2PORT18_PVEC_HI_S + 40,
                           MC_MGR_RDM_NODE_L2PORT18_PVEC_HI_W);
        break;
      case MC_MGR_RDM_NODE_L2LAG_TYPE:
        line->type[1] = mc_mgr_rdm_node_type_lag;
        line->u.lag[1].next_l2 =
            bf_bs_get_word(&x,
                           MC_MGR_RDM_NODE_L2LAG_NEXT_S + 40,
                           MC_MGR_RDM_NODE_L2LAG_NEXT_W);
        line->u.lag[1].lag_id =
            bf_bs_get_word(&x,
                           MC_MGR_RDM_NODE_L2LAG_INDX_S + 40,
                           MC_MGR_RDM_NODE_L2LAG_INDX_W);
        break;
      default:
        LOG_ERROR("Unknown node type (upper half)");
        break;
    }
  }
  return BF_SUCCESS;
}

void tof2_mc_mgr_rdm_encode_line(mc_mgr_rdm_line_t *line) {
  bf_bitset_t x;
  bf_bs_init(&x, 80, line->data);
  line->data[0] = line->data[1] = 0;
  bool upper_valid = false;

  switch (line->type[0]) {
    case mc_mgr_rdm_node_type_invalid:
      upper_valid = true;
      break;
    case mc_mgr_rdm_node_type_rid:
      bf_bs_set_word(&x,
                     MC_MGR_RDM_NODE_TYPE_S,
                     MC_MGR_RDM_NODE_TYPE_W,
                     MC_MGR_RDM_NODE_L1RID_TYPE);
      bf_bs_set_word(&x,
                     MC_MGR_RDM_NODE_L1RID_L1_NEXT_S,
                     MC_MGR_RDM_NODE_L1RID_L1_NEXT_W,
                     line->u.rid.next_l1);
      bf_bs_set_word(&x,
                     MC_MGR_RDM_NODE_L1RID_L2_NEXT_S,
                     MC_MGR_RDM_NODE_L1RID_L2_NEXT_W,
                     line->u.rid.next_l2);
      bf_bs_set_word(&x,
                     MC_MGR_RDM_NODE_L1RID_RID_S,
                     MC_MGR_RDM_NODE_L1RID_RID_W,
                     line->u.rid.rid);
      break;
    case mc_mgr_rdm_node_type_xid:
      bf_bs_set_word(&x,
                     MC_MGR_RDM_NODE_TYPE_S,
                     MC_MGR_RDM_NODE_TYPE_W,
                     MC_MGR_RDM_NODE_L1RIDXID_TYPE);
      bf_bs_set_word(&x,
                     MC_MGR_RDM_NODE_L1RIDXID_L1_NEXT_S,
                     MC_MGR_RDM_NODE_L1RIDXID_L1_NEXT_W,
                     line->u.xid.next_l1);
      bf_bs_set_word(&x,
                     MC_MGR_RDM_NODE_L1RIDXID_L2_NEXT_S,
                     MC_MGR_RDM_NODE_L1RIDXID_L2_NEXT_W,
                     line->u.xid.next_l2);
      bf_bs_set_word(&x,
                     MC_MGR_RDM_NODE_L1RIDXID_RID_S,
                     MC_MGR_RDM_NODE_L1RIDXID_RID_W,
                     line->u.xid.rid);
      bf_bs_set_word(&x,
                     MC_MGR_RDM_NODE_L1RIDXID_XID_S,
                     MC_MGR_RDM_NODE_L1RIDXID_XID_W,
                     line->u.xid.xid);
      break;
    case mc_mgr_rdm_node_type_end:
      upper_valid = true;
      bf_bs_set_word(&x,
                     MC_MGR_RDM_NODE_TYPE_S,
                     MC_MGR_RDM_NODE_TYPE_W,
                     MC_MGR_RDM_NODE_L1RIDEND_TYPE);
      bf_bs_set_word(&x,
                     MC_MGR_RDM_NODE_L1RIDEND_L2_NEXT_S,
                     MC_MGR_RDM_NODE_L1RIDEND_L2_NEXT_W,
                     line->u.end[0].next_l2);
      bf_bs_set_word(&x,
                     MC_MGR_RDM_NODE_L1RIDEND_RID_S,
                     MC_MGR_RDM_NODE_L1RIDEND_RID_W,
                     line->u.end[0].rid);
      break;
    case mc_mgr_rdm_node_type_ecmp:
      bf_bs_set_word(&x,
                     MC_MGR_RDM_NODE_TYPE_S,
                     MC_MGR_RDM_NODE_TYPE_W,
                     MC_MGR_RDM_NODE_PTR_TYPE);
      bf_bs_set_word(&x,
                     MC_MGR_RDM_NODE_PTR_NEXT_S,
                     MC_MGR_RDM_NODE_PTR_NEXT_W,
                     line->u.ecmp.next_l1);
      bf_bs_set_word(&x,
                     MC_MGR_RDM_NODE_PTR_P0_S,
                     MC_MGR_RDM_NODE_PTR_P0_W,
                     line->u.ecmp.vector0);
      bf_bs_set_word(&x,
                     MC_MGR_RDM_NODE_PTR_P1_S,
                     MC_MGR_RDM_NODE_PTR_P1_W,
                     line->u.ecmp.vector1);
      break;
    case mc_mgr_rdm_node_type_ecmp_xid:
      bf_bs_set_word(&x,
                     MC_MGR_RDM_NODE_TYPE_S,
                     MC_MGR_RDM_NODE_TYPE_W,
                     MC_MGR_RDM_NODE_PTRXID_TYPE);
      bf_bs_set_word(&x,
                     MC_MGR_RDM_NODE_PTRXID_NEXT_S,
                     MC_MGR_RDM_NODE_PTRXID_NEXT_W,
                     line->u.ecmp_xid.next_l1);
      bf_bs_set_word(&x,
                     MC_MGR_RDM_NODE_PTRXID_P0_S,
                     MC_MGR_RDM_NODE_PTRXID_P0_W,
                     line->u.ecmp_xid.vector0);
      bf_bs_set_word(&x,
                     MC_MGR_RDM_NODE_PTRXID_P1_S,
                     MC_MGR_RDM_NODE_PTRXID_P1_W,
                     line->u.ecmp_xid.vector1);
      bf_bs_set_word(&x,
                     MC_MGR_RDM_NODE_PTRXID_XID_S,
                     MC_MGR_RDM_NODE_PTRXID_XID_W,
                     line->u.ecmp_xid.xid);
      break;
    case mc_mgr_rdm_node_type_vector:
      bf_bs_set_word(&x,
                     MC_MGR_RDM_NODE_TYPE_S,
                     MC_MGR_RDM_NODE_TYPE_W,
                     MC_MGR_RDM_NODE_VEC_TYPE);
      bf_bs_set_word(&x,
                     MC_MGR_RDM_NODE_VEC_BASE_S,
                     MC_MGR_RDM_NODE_VEC_BASE_W,
                     line->u.vector.base_l1);
      bf_bs_set_word(&x,
                     MC_MGR_RDM_NODE_VEC_LEN_S,
                     MC_MGR_RDM_NODE_VEC_LEN_W,
                     line->u.vector.length);
      bf_bs_set_word(&x,
                     MC_MGR_RDM_NODE_VEC_VEC_S,
                     MC_MGR_RDM_NODE_VEC_VEC_W,
                     line->u.vector.vector);
      bf_bs_set_word(&x,
                     MC_MGR_RDM_NODE_VEC_ID_S,
                     MC_MGR_RDM_NODE_VEC_ID_W,
                     line->u.vector.id);
      break;
    case mc_mgr_rdm_node_type_port18:
      upper_valid = true;
      bf_bs_set_word(&x,
                     MC_MGR_RDM_NODE_TYPE_S,
                     MC_MGR_RDM_NODE_TYPE_W,
                     MC_MGR_RDM_NODE_L2PORT18_TYPE);
      bf_bs_set_word(&x,
                     MC_MGR_RDM_NODE_L2PORT18_PIPE_S,
                     MC_MGR_RDM_NODE_L2PORT18_PIPE_W,
                     line->u.port18[0].pipe);
      bf_bs_set_word(&x,
                     MC_MGR_RDM_NODE_L2PORT18_LAST_S,
                     MC_MGR_RDM_NODE_L2PORT18_LAST_W,
                     line->u.port18[0].last);
      bf_bs_set_word(&x,
                     MC_MGR_RDM_NODE_L2PORT18_PVEC_LO_S,
                     MC_MGR_RDM_NODE_L2PORT18_PVEC_LO_W,
                     line->u.port18[0].ports);
      bf_bs_set_word(&x,
                     MC_MGR_RDM_NODE_L2PORT18_PVEC_HI_S,
                     MC_MGR_RDM_NODE_L2PORT18_PVEC_HI_W,
                     line->u.port18[0].spv);
      break;
    case mc_mgr_rdm_node_type_port72:
      bf_bs_set_word(&x,
                     MC_MGR_RDM_NODE_TYPE_S,
                     MC_MGR_RDM_NODE_TYPE_W,
                     MC_MGR_RDM_NODE_L2PORT72_TYPE);
      bf_bs_set_word(&x,
                     MC_MGR_RDM_NODE_L2PORT72_PIPE_S,
                     MC_MGR_RDM_NODE_L2PORT72_PIPE_W,
                     line->u.port72.pipe);
      bf_bs_set_word(&x,
                     MC_MGR_RDM_NODE_L2PORT72_LAST_S,
                     MC_MGR_RDM_NODE_L2PORT72_LAST_W,
                     line->u.port72.last);
      bf_bs_set_word(&x,
                     MC_MGR_RDM_NODE_L2PORT72_PVEC_LO_S,
                     MC_MGR_RDM_NODE_L2PORT72_PVEC_LO_W,
                     line->u.port72.ports);
      bf_bs_set_word(&x,
                     MC_MGR_RDM_NODE_L2PORT72_PVEC_HI_S,
                     MC_MGR_RDM_NODE_L2PORT72_PVEC_HI_W,
                     line->u.port72.spv);
      break;
    case mc_mgr_rdm_node_type_lag:
      upper_valid = true;
      bf_bs_set_word(&x,
                     MC_MGR_RDM_NODE_TYPE_S,
                     MC_MGR_RDM_NODE_TYPE_W,
                     MC_MGR_RDM_NODE_L2LAG_TYPE);
      bf_bs_set_word(&x,
                     MC_MGR_RDM_NODE_L2LAG_NEXT_S,
                     MC_MGR_RDM_NODE_L2LAG_NEXT_W,
                     line->u.lag[0].next_l2);
      bf_bs_set_word(&x,
                     MC_MGR_RDM_NODE_L2LAG_INDX_S,
                     MC_MGR_RDM_NODE_L2LAG_INDX_W,
                     line->u.lag[0].lag_id);
      break;
  }

  if (upper_valid) {
    switch (line->type[1]) {
      case mc_mgr_rdm_node_type_invalid:
        break;
      case mc_mgr_rdm_node_type_end:
        bf_bs_set_word(&x,
                       MC_MGR_RDM_NODE_TYPE_S + 40,
                       MC_MGR_RDM_NODE_TYPE_W,
                       MC_MGR_RDM_NODE_L1RIDEND_TYPE);
        bf_bs_set_word(&x,
                       MC_MGR_RDM_NODE_L1RIDEND_L2_NEXT_S + 40,
                       MC_MGR_RDM_NODE_L1RIDEND_L2_NEXT_W,
                       line->u.end[1].next_l2);
        bf_bs_set_word(&x,
                       MC_MGR_RDM_NODE_L1RIDEND_RID_S + 40,
                       MC_MGR_RDM_NODE_L1RIDEND_RID_W,
                       line->u.end[1].rid);
        break;
      case mc_mgr_rdm_node_type_port18:
        bf_bs_set_word(&x,
                       MC_MGR_RDM_NODE_TYPE_S + 40,
                       MC_MGR_RDM_NODE_TYPE_W,
                       MC_MGR_RDM_NODE_L2PORT18_TYPE);
        bf_bs_set_word(&x,
                       MC_MGR_RDM_NODE_L2PORT18_PIPE_S + 40,
                       MC_MGR_RDM_NODE_L2PORT18_PIPE_W,
                       line->u.port18[1].pipe);
        bf_bs_set_word(&x,
                       MC_MGR_RDM_NODE_L2PORT18_LAST_S + 40,
                       MC_MGR_RDM_NODE_L2PORT18_LAST_W,
                       line->u.port18[1].last);
        bf_bs_set_word(&x,
                       MC_MGR_RDM_NODE_L2PORT18_PVEC_LO_S + 40,
                       MC_MGR_RDM_NODE_L2PORT18_PVEC_LO_W,
                       line->u.port18[1].ports);
        bf_bs_set_word(&x,
                       MC_MGR_RDM_NODE_L2PORT18_PVEC_HI_S + 40,
                       MC_MGR_RDM_NODE_L2PORT18_PVEC_HI_W,
                       line->u.port18[1].spv);
        break;
      case mc_mgr_rdm_node_type_lag:
        bf_bs_set_word(&x,
                       MC_MGR_RDM_NODE_TYPE_S + 40,
                       MC_MGR_RDM_NODE_TYPE_W,
                       MC_MGR_RDM_NODE_L2LAG_TYPE);
        bf_bs_set_word(&x,
                       MC_MGR_RDM_NODE_L2LAG_NEXT_S + 40,
                       MC_MGR_RDM_NODE_L2LAG_NEXT_W,
                       line->u.lag[1].next_l2);
        bf_bs_set_word(&x,
                       MC_MGR_RDM_NODE_L2LAG_INDX_S + 40,
                       MC_MGR_RDM_NODE_L2LAG_INDX_W,
                       line->u.lag[1].lag_id);
        break;
      default:
        MC_MGR_DBGCHK(0);
        break;
    }
  } else {
    MC_MGR_DBGCHK(mc_mgr_rdm_node_type_invalid == line->type[1]);
  }
}
