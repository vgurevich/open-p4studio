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


#include <target-sys/bf_sal/bf_sys_intf.h>
#include <pipe_mgr/pipe_mgr_intf.h>
#include <tofino/pdfixed/pd_ms.h>

#include <target-utils/third-party/judy-1.0.5/src/Judy.h>
#include <string.h>

#define NUM_DEVICES 256

typedef struct mbr_info_s {
  p4_pd_act_hdl_t act_hdl;
  Pvoid_t grp_list;

  pd_res_spec_t resources[PIPE_NUM_TBL_RESOURCES];
  int resource_count;
} mbr_info_t;

typedef struct grp_info_s {
  p4_pd_act_hdl_t act_hdl;
  int mbr_count;

  pd_res_spec_t resources[PIPE_NUM_TBL_RESOURCES];
  int resource_count;
} grp_info_t;

struct p4_pd_ms_table_state_s {
  Pvoid_t devices_mbr_to_info[NUM_DEVICES];
  Pvoid_t devices_grp_to_info[NUM_DEVICES];
};

typedef struct p4_pd_ms_txn_node_s {
  struct p4_pd_ms_table_state_s *state;
  struct p4_pd_ms_table_state_s *backup;
} p4_pd_ms_txn_node_t;

static Pvoid_t txn_state;

void p4_pd_ms_init(void) { txn_state = NULL; }

p4_pd_ms_table_state_t *p4_pd_ms_init_state(void) {
  p4_pd_ms_table_state_t *state = bf_sys_malloc(sizeof(p4_pd_ms_table_state_t));
  memset(state->devices_mbr_to_info, 0, NUM_DEVICES * sizeof(Pvoid_t));
  memset(state->devices_grp_to_info, 0, NUM_DEVICES * sizeof(Pvoid_t));
  return state;
}

void p4_pd_ms_destroy_state_for_dev(p4_pd_ms_table_state_t *state,
                                    uint32_t dev_id) {
  Word_t Index;
  PWord_t PValue;
  mbr_info_t *mbr_info;
  grp_info_t *grp_info;
  Word_t Rc_word;
  Index = 0;

  if (!state) {
    return;
  }

  JLF(PValue, state->devices_mbr_to_info[dev_id], Index);
  while (PValue != NULL) {
    mbr_info = (mbr_info_t *)*PValue;
    if (mbr_info) {
      J1FA(Rc_word, mbr_info->grp_list);
      (void)Rc_word;
      bf_sys_free(mbr_info);
    }
    JLN(PValue, state->devices_mbr_to_info[dev_id], Index);
  }
  JLFA(Rc_word, state->devices_mbr_to_info[dev_id]);
  (void)Rc_word;
  state->devices_mbr_to_info[dev_id] = NULL;

  Index = 0;
  JLF(PValue, state->devices_grp_to_info[dev_id], Index);
  while (PValue != NULL) {
    grp_info = (grp_info_t *)*PValue;
    if (grp_info) {
      bf_sys_free(grp_info);
    }
    JLN(PValue, state->devices_grp_to_info[dev_id], Index);
  }
  JLFA(Rc_word, state->devices_grp_to_info[dev_id]);
  (void)Rc_word;
  state->devices_grp_to_info[dev_id] = NULL;
}

void p4_pd_ms_destroy_state(p4_pd_ms_table_state_t *state) {
  bf_dev_id_t dev_id;

  if (!state) {
    return;
  }

  for (dev_id = 0; dev_id < NUM_DEVICES; dev_id++) {
    p4_pd_ms_destroy_state_for_dev(state, dev_id);
  }

  bf_sys_free(state);
}

void p4_pd_ms_destroy_txn_state_for_dev(uint32_t dev_id) {
  p4_pd_ms_txn_node_t *node;
  PWord_t PValue, PValueTxn;
  Word_t sess_hdl = 0;
  JLF(PValueTxn, txn_state, sess_hdl);
  while (PValueTxn != NULL) {
    Word_t Index = 0;
    JLF(PValue, (Pvoid_t)*PValueTxn, Index);
    while (PValue != NULL) {
      node = (p4_pd_ms_txn_node_t *)*PValue;
      p4_pd_ms_destroy_state_for_dev(node->backup, dev_id);
      JLN(PValue, (Pvoid_t)*PValueTxn, Index);
    }
    JLN(PValueTxn, txn_state, sess_hdl);
  }
}

static void p4_pd_ms_destroy_backup(p4_pd_ms_table_state_t *backup) {
  p4_pd_ms_destroy_state(backup);
}

void p4_pd_ms_cleanup(void) {
  p4_pd_ms_txn_node_t *node;
  PWord_t PValue, PValueTxn;
  Word_t sess_hdl = 0;
  Word_t Rc_word;
  JLF(PValueTxn, txn_state, sess_hdl);
  while (PValueTxn != NULL) {
    Word_t Index = 0;
    JLF(PValue, (Pvoid_t)*PValueTxn, Index);
    while (PValue != NULL) {
      node = (p4_pd_ms_txn_node_t *)*PValue;
      p4_pd_ms_destroy_backup(node->backup);
      bf_sys_free(node);
      JLN(PValue, (Pvoid_t)*PValueTxn, Index);
    }
    Pvoid_t *jnodes = (Pvoid_t *)PValueTxn;
    JLFA(Rc_word, *jnodes);
    (void)Rc_word;
    JLN(PValueTxn, txn_state, sess_hdl);
  }
  JLFA(Rc_word, txn_state);
  (void)Rc_word;
}

static void p4_pd_ms_copy_grp_list(mbr_info_t *src, mbr_info_t *dst) {
  Word_t Index = 0;
  int Rc_int;
  J1F(Rc_int, src->grp_list, Index);
  while (Rc_int) {
    J1S(Rc_int, dst->grp_list, Index);
    J1N(Rc_int, src->grp_list, Index);
  }
}

static void p4_pd_ms_backup_mbr(p4_pd_ms_table_state_t *state,
                                p4_pd_ms_table_state_t *backup,
                                bf_dev_id_t dev_id,
                                p4_pd_mbr_hdl_t mbr_hdl) {
  PWord_t PValue;
  JLG(PValue, backup->devices_mbr_to_info[dev_id], mbr_hdl);
  if (PValue) {
    return;
  }

  mbr_info_t *mbr_info = NULL;
  JLG(PValue, state->devices_mbr_to_info[dev_id], mbr_hdl);
  if (PValue) {
    mbr_info = bf_sys_malloc(sizeof(mbr_info_t));
    memcpy(mbr_info, (mbr_info_t *)*PValue, sizeof(mbr_info_t));
    mbr_info->grp_list = NULL;
    p4_pd_ms_copy_grp_list((mbr_info_t *)*PValue, mbr_info);
  }
  PWord_t PValueBackup;
  JLI(PValueBackup, backup->devices_mbr_to_info[dev_id], mbr_hdl);
  *PValueBackup = (Word_t)mbr_info;
}

static void p4_pd_ms_backup_grp(p4_pd_ms_table_state_t *state,
                                p4_pd_ms_table_state_t *backup,
                                bf_dev_id_t dev_id,
                                p4_pd_grp_hdl_t grp_hdl) {
  PWord_t PValue;
  JLG(PValue, backup->devices_grp_to_info[dev_id], grp_hdl);
  if (PValue) {
    return;
  }

  grp_info_t *grp_info = NULL;
  JLG(PValue, state->devices_grp_to_info[dev_id], grp_hdl);
  if (PValue) {
    grp_info = bf_sys_malloc(sizeof(grp_info_t));
    memcpy(grp_info, (grp_info_t *)*PValue, sizeof(grp_info_t));
  }
  PWord_t PValueBackup;
  JLI(PValueBackup, backup->devices_grp_to_info[dev_id], grp_hdl);
  *PValueBackup = (Word_t)grp_info;
}

static void p4_pd_ms_backup(p4_pd_sess_hdl_t sess_hdl,
                            p4_pd_ms_table_state_t *state,
                            bf_dev_id_t dev_id,
                            p4_pd_mbr_hdl_t mbr_hdl,
                            p4_pd_grp_hdl_t grp_hdl) {
  PWord_t PValueTxn, PValueTxnNode;
  Pvoid_t TxnMap;
  Word_t Index = (Word_t)state;
  p4_pd_ms_txn_node_t *txn_node = NULL;
  JLG(PValueTxn, txn_state, sess_hdl);
  if (PValueTxn) {
    TxnMap = (Pvoid_t)*PValueTxn;
    JLG(PValueTxnNode, TxnMap, Index);
    if (PValueTxnNode) {
      txn_node = (p4_pd_ms_txn_node_t *)*PValueTxnNode;
    }
    if (!txn_node) {
      txn_node = bf_sys_malloc(sizeof(p4_pd_ms_txn_node_t));
      txn_node->state = state;
      txn_node->backup = p4_pd_ms_init_state();
      JLI(PValueTxnNode, TxnMap, Index);
      if (!*PValueTxn) {
        *PValueTxn = (Word_t)TxnMap;
      }
      *PValueTxnNode = (Word_t)txn_node;
    }
    if (mbr_hdl) {
      p4_pd_ms_backup_mbr(txn_node->state, txn_node->backup, dev_id, mbr_hdl);
    }
    if (grp_hdl) {
      p4_pd_ms_backup_grp(txn_node->state, txn_node->backup, dev_id, grp_hdl);
    }
  }
}

static int same_resource(pd_res_spec_t *res1, pd_res_spec_t *res2) {
  return (res1->tbl_hdl == res2->tbl_hdl) && (res1->tbl_idx == res2->tbl_idx);
}

p4_pd_status_t p4_pd_ms_add_mbr_to_grp(p4_pd_sess_hdl_t sess_hdl,
                                       p4_pd_ms_table_state_t *state,
                                       bf_dev_id_t dev_id,
                                       p4_pd_mbr_hdl_t mbr_hdl,
                                       p4_pd_grp_hdl_t grp_hdl) {
  p4_pd_ms_backup(sess_hdl, state, dev_id, mbr_hdl, grp_hdl);

  PWord_t PValue;
  JLG(PValue, state->devices_mbr_to_info[dev_id], mbr_hdl);
  if (!PValue) {
    return BF_OBJECT_NOT_FOUND;
  }
  int Rc_int;
  mbr_info_t *mbr_info = (mbr_info_t *)*PValue;
  J1S(Rc_int, mbr_info->grp_list, grp_hdl);

  JLG(PValue, state->devices_grp_to_info[dev_id], grp_hdl);
  if (!PValue) {
    return BF_OBJECT_NOT_FOUND;
  }
  grp_info_t *grp_info = (grp_info_t *)*PValue;

  if (!Rc_int) return BF_OBJECT_NOT_FOUND;

  if (grp_info->mbr_count++ == 0) {
    grp_info->resource_count = mbr_info->resource_count;
    memcpy(
        grp_info->resources, mbr_info->resources, sizeof(mbr_info->resources));
  } else {
    bf_sys_assert(grp_info->resource_count == mbr_info->resource_count);
    int i;
    for (i = 0; i < mbr_info->resource_count; i++) {
      bf_sys_assert(
          same_resource(&grp_info->resources[i], &mbr_info->resources[i]));
    }
  }

  return BF_SUCCESS;
}

p4_pd_status_t p4_pd_ms_del_mbr_from_grp(p4_pd_sess_hdl_t sess_hdl,
                                         p4_pd_ms_table_state_t *state,
                                         bf_dev_id_t dev_id,
                                         p4_pd_mbr_hdl_t mbr_hdl,
                                         p4_pd_grp_hdl_t grp_hdl) {
  p4_pd_ms_backup(sess_hdl, state, dev_id, mbr_hdl, grp_hdl);

  PWord_t PValue;
  JLG(PValue, state->devices_mbr_to_info[dev_id], mbr_hdl);
  if (!PValue) {
    return BF_OBJECT_NOT_FOUND;
  }
  int Rc_int;
  mbr_info_t *mbr_info = (mbr_info_t *)*PValue;
  J1U(Rc_int, mbr_info->grp_list, grp_hdl);

  if (!Rc_int) return BF_OBJECT_NOT_FOUND;

  JLG(PValue, state->devices_grp_to_info[dev_id], grp_hdl);
  if (!PValue) {
    return BF_OBJECT_NOT_FOUND;
  }
  grp_info_t *grp_info = (grp_info_t *)*PValue;
  grp_info->mbr_count--;
  if (!grp_info->mbr_count) {
    // Reset act hdl and resources if last member is removed from group
    memset(grp_info, 0, sizeof(grp_info_t));
  }

  return BF_SUCCESS;
}

p4_pd_status_t p4_pd_ms_mbr_apply_to_grps(p4_pd_ms_table_state_t *state,
                                          bf_dev_id_t dev_id,
                                          p4_pd_mbr_hdl_t mbr_hdl,
                                          PDMSGrpFn grp_fn,
                                          void *aux) {
  PWord_t PValue;
  JLG(PValue, state->devices_mbr_to_info[dev_id], mbr_hdl);
  if (!PValue) {
    return BF_OBJECT_NOT_FOUND;
  }
  Word_t Index = 0;
  int Rc_int;
  mbr_info_t *mbr_info = (mbr_info_t *)*PValue;
  J1F(Rc_int, mbr_info->grp_list, Index);
  while (Rc_int) {
    grp_fn(dev_id, mbr_hdl, Index, aux);
    J1N(Rc_int, mbr_info->grp_list, Index);
  }
  return BF_SUCCESS;
}

p4_pd_status_t p4_pd_ms_new_mbr(p4_pd_sess_hdl_t sess_hdl,
                                p4_pd_ms_table_state_t *state,
                                bf_dev_id_t dev_id,
                                p4_pd_mbr_hdl_t mbr_hdl) {
  p4_pd_ms_backup(sess_hdl, state, dev_id, mbr_hdl, 0);

  mbr_info_t *mbr_info = bf_sys_malloc(sizeof(mbr_info_t));
  memset(mbr_info, 0, sizeof(mbr_info_t));
  PWord_t PValue;
  JLI(PValue, state->devices_mbr_to_info[dev_id], mbr_hdl);
  bf_sys_assert(!(*PValue));
  *PValue = (Word_t)mbr_info;
  return BF_SUCCESS;
}

static p4_pd_status_t p4_pd_ms_del_mbr_int(p4_pd_ms_table_state_t *state,
                                           bf_dev_id_t dev_id,
                                           p4_pd_mbr_hdl_t mbr_hdl) {
  PWord_t PValue;
  JLG(PValue, state->devices_mbr_to_info[dev_id], mbr_hdl);
  Word_t num_freed;
  if (!PValue) {
    return BF_OBJECT_NOT_FOUND;
  }
  mbr_info_t *mbr_info = (mbr_info_t *)*PValue;
  J1FA(num_freed, mbr_info->grp_list);
  (void)num_freed;
  int Rc_int;
  bf_sys_free(mbr_info);
  JLD(Rc_int, state->devices_mbr_to_info[dev_id], mbr_hdl);
  bf_sys_assert(Rc_int);
  return BF_SUCCESS;
}

p4_pd_status_t p4_pd_ms_del_mbr(p4_pd_sess_hdl_t sess_hdl,
                                p4_pd_ms_table_state_t *state,
                                bf_dev_id_t dev_id,
                                p4_pd_mbr_hdl_t mbr_hdl) {
  p4_pd_ms_backup(sess_hdl, state, dev_id, mbr_hdl, 0);

  return p4_pd_ms_del_mbr_int(state, dev_id, mbr_hdl);
}

p4_pd_status_t p4_pd_ms_get_mbr_act(p4_pd_ms_table_state_t *state,
                                    bf_dev_id_t dev_id,
                                    p4_pd_mbr_hdl_t mbr_hdl,
                                    p4_pd_act_hdl_t *act_hdl) {
  PWord_t PValue;
  JLG(PValue, state->devices_mbr_to_info[dev_id], mbr_hdl);
  if (!PValue) {
    return BF_OBJECT_NOT_FOUND;
  }
  mbr_info_t *mbr_info = (mbr_info_t *)*PValue;
  if (act_hdl) {
    *act_hdl = mbr_info->act_hdl;
  }
  return BF_SUCCESS;
}

p4_pd_status_t p4_pd_ms_set_mbr_act(p4_pd_ms_table_state_t *state,
                                    bf_dev_id_t dev_id,
                                    p4_pd_mbr_hdl_t mbr_hdl,
                                    p4_pd_act_hdl_t act_hdl) {
  PWord_t PValue;
  JLG(PValue, state->devices_mbr_to_info[dev_id], mbr_hdl);
  if (!PValue) {
    return BF_OBJECT_NOT_FOUND;
  }
  mbr_info_t *mbr_info = (mbr_info_t *)*PValue;
  mbr_info->act_hdl = act_hdl;
  return BF_SUCCESS;
}

p4_pd_status_t p4_pd_ms_get_grp_act(p4_pd_ms_table_state_t *state,
                                    bf_dev_id_t dev_id,
                                    p4_pd_grp_hdl_t grp_hdl,
                                    p4_pd_act_hdl_t *act_hdl) {
  PWord_t PValue;
  JLG(PValue, state->devices_grp_to_info[dev_id], grp_hdl);
  if (!PValue) {
    return BF_OBJECT_NOT_FOUND;
  }
  grp_info_t *grp_info = (grp_info_t *)*PValue;
  if (act_hdl) {
    *act_hdl = grp_info->act_hdl;
  }
  return BF_SUCCESS;
}

p4_pd_status_t p4_pd_ms_set_grp_act(p4_pd_ms_table_state_t *state,
                                    bf_dev_id_t dev_id,
                                    p4_pd_grp_hdl_t grp_hdl,
                                    p4_pd_act_hdl_t act_hdl) {
  PWord_t PValue;
  JLG(PValue, state->devices_grp_to_info[dev_id], grp_hdl);
  if (!PValue) {
    return BF_OBJECT_NOT_FOUND;
  }
  grp_info_t *grp_info = (grp_info_t *)*PValue;
  grp_info->act_hdl = act_hdl;
  return BF_SUCCESS;
}

p4_pd_status_t p4_pd_ms_del_grp(p4_pd_sess_hdl_t sess_hdl,
                                p4_pd_ms_table_state_t *state,
                                bf_dev_id_t dev_id,
                                p4_pd_grp_hdl_t grp_hdl,
                                uint32_t num_mbrs,
                                p4_pd_mbr_hdl_t *mbr_hdls) {
  p4_pd_ms_backup(sess_hdl, state, dev_id, 0, grp_hdl);

  PWord_t PValue;
  int Rc_int;
  mbr_info_t *mbr_info = NULL;
  p4_pd_mbr_hdl_t mbr_hdl = 0;
  (void)Rc_int;  // to make compiler happy

  // Validate input
  if (num_mbrs && mbr_hdls == NULL) {
    return BF_INVALID_ARG;
  }

  // If the group has members, update members' group list
  // in pd ms state before deleting the group
  if (num_mbrs) {
    for (uint32_t i = 0; i < num_mbrs; i++) {
      mbr_hdl = mbr_hdls[i];

      JLG(PValue, state->devices_mbr_to_info[dev_id], mbr_hdl);
      if (!PValue) {
        // Error, but ok to continue with next member
        continue;
      }
      mbr_info = (mbr_info_t *)*PValue;
      p4_pd_ms_backup(sess_hdl, state, dev_id, mbr_hdl, 0);
      J1U(Rc_int, mbr_info->grp_list, grp_hdl);
    }
  }

  JLG(PValue, state->devices_grp_to_info[dev_id], grp_hdl);
  if (!PValue) {
    return BF_OBJECT_NOT_FOUND;
  }
  grp_info_t *grp_info = (grp_info_t *)*PValue;
  bf_sys_free(grp_info);
  JLD(Rc_int, state->devices_grp_to_info[dev_id], grp_hdl);
  return BF_SUCCESS;
}

p4_pd_status_t p4_pd_ms_new_grp(p4_pd_sess_hdl_t sess_hdl,
                                p4_pd_ms_table_state_t *state,
                                bf_dev_id_t dev_id,
                                p4_pd_grp_hdl_t grp_hdl) {
  p4_pd_ms_backup(sess_hdl, state, dev_id, 0, grp_hdl);

  grp_info_t *grp_info = bf_sys_malloc(sizeof(grp_info_t));
  memset(grp_info, 0, sizeof(grp_info_t));
  PWord_t PValue;
  JLI(PValue, state->devices_grp_to_info[dev_id], grp_hdl);
  bf_sys_assert(!(*PValue));
  *PValue = (Word_t)grp_info;
  return BF_SUCCESS;
}

p4_pd_status_t p4_pd_ms_grp_get_mbrs(p4_pd_ms_table_state_t *state,
                                     bf_dev_id_t dev_id,
                                     p4_pd_grp_hdl_t grp_hdl,
                                     p4_pd_mbr_hdl_t *mbr_hdls,
                                     int *num_mbrs) {
  Word_t Index = 0;
  PWord_t PValue;
  int Rc_int;
  mbr_info_t *mbr_info;
  *num_mbrs = 0;
  JLF(PValue, state->devices_mbr_to_info[dev_id], Index);
  while (PValue != NULL) {
    mbr_info = (mbr_info_t *)*PValue;
    J1T(Rc_int, mbr_info->grp_list, grp_hdl);
    if (Rc_int) {  // in group
      *(mbr_hdls++) = (p4_pd_mbr_hdl_t)Index;
      (*num_mbrs)++;
    }
    JLN(PValue, state->devices_mbr_to_info[dev_id], Index);
  }
  return BF_SUCCESS;
}

p4_pd_status_t p4_pd_ms_mbr_add_res(p4_pd_ms_table_state_t *state,
                                    bf_dev_id_t dev_id,
                                    p4_pd_mbr_hdl_t mbr_hdl,
                                    pd_res_spec_t *res_spec) {
  PWord_t PValue;
  JLG(PValue, state->devices_mbr_to_info[dev_id], mbr_hdl);
  if (!PValue) {
    return BF_OBJECT_NOT_FOUND;
  }
  mbr_info_t *mbr_info = (mbr_info_t *)*PValue;

  int i;
  for (i = 0; i < mbr_info->resource_count; i++) {
    if (same_resource(res_spec, &mbr_info->resources[i])) {
      return BF_ALREADY_EXISTS;
    }
  }

  bf_sys_assert(mbr_info->resource_count < PIPE_NUM_TBL_RESOURCES);
  mbr_info->resources[mbr_info->resource_count++] = *res_spec;
  return BF_SUCCESS;
}

p4_pd_status_t p4_pd_ms_mbr_get_res(p4_pd_ms_table_state_t *state,
                                    bf_dev_id_t dev_id,
                                    p4_pd_mbr_hdl_t mbr_hdl,
                                    int *count,
                                    pd_res_spec_t **res_specs) {
  PWord_t PValue;
  if (state->devices_mbr_to_info[dev_id]) {
    JLG(PValue, state->devices_mbr_to_info[dev_id], mbr_hdl);
    if (!PValue) {
      return BF_OBJECT_NOT_FOUND;
    }
    mbr_info_t *mbr_info = (mbr_info_t *)*PValue;
    *count = mbr_info->resource_count;
    *res_specs = mbr_info->resources;
  } else {
    *count = 0;
  }
  return BF_SUCCESS;
}

p4_pd_status_t p4_pd_ms_grp_update_res(bf_dev_id_t dev_id,
                                       p4_pd_mbr_hdl_t mbr_hdl,
                                       p4_pd_grp_hdl_t grp_hdl,
                                       void *aux) {
  p4_pd_ms_table_state_t *state = (p4_pd_ms_table_state_t *)aux;
  PWord_t PValue;
  JLG(PValue, state->devices_mbr_to_info[dev_id], mbr_hdl);
  if (!PValue) {
    return BF_OBJECT_NOT_FOUND;
  }
  mbr_info_t *mbr_info = (mbr_info_t *)*PValue;

  JLG(PValue, state->devices_grp_to_info[dev_id], grp_hdl);
  if (!PValue) {
    return BF_OBJECT_NOT_FOUND;
  }
  grp_info_t *grp_info = (grp_info_t *)*PValue;

  grp_info->resource_count = mbr_info->resource_count;
  memcpy(grp_info->resources, mbr_info->resources, sizeof(mbr_info->resources));
  return BF_SUCCESS;
}

p4_pd_status_t p4_pd_ms_grp_get_res(p4_pd_ms_table_state_t *state,
                                    bf_dev_id_t dev_id,
                                    p4_pd_grp_hdl_t grp_hdl,
                                    int *count,
                                    pd_res_spec_t **res_specs) {
  PWord_t PValue;
  JLG(PValue, state->devices_grp_to_info[dev_id], grp_hdl);
  if (!PValue) {
    return BF_OBJECT_NOT_FOUND;
  }
  grp_info_t *grp_info = (grp_info_t *)*PValue;
  if (grp_info->mbr_count == 0) return BF_OBJECT_NOT_FOUND;
  *count = grp_info->resource_count;
  *res_specs = grp_info->resources;
  return BF_SUCCESS;
}

void p4_pd_ms_restore_backup(p4_pd_ms_table_state_t *state,
                             p4_pd_ms_table_state_t *backup) {
  PWord_t PValue;
  PWord_t PValueBackup;
  Word_t mbr_hdl, grp_hdl;
  bf_dev_id_t dev_id;
  int Rc_int;

  for (dev_id = 0; dev_id < NUM_DEVICES; dev_id++) {
    mbr_hdl = 0;
    JLF(PValueBackup, backup->devices_mbr_to_info[dev_id], mbr_hdl);
    while (PValueBackup != NULL) {
      JLG(PValue, state->devices_mbr_to_info[dev_id], mbr_hdl);
      if (PValue) {
        p4_pd_ms_del_mbr_int(state, dev_id, mbr_hdl);
      }
      if (*PValueBackup) {
        JLI(PValue, state->devices_mbr_to_info[dev_id], mbr_hdl);
        *PValue = *PValueBackup;
        *PValueBackup = 0;
      }
      JLN(PValueBackup, backup->devices_mbr_to_info[dev_id], mbr_hdl);
    }

    grp_hdl = 0;
    JLF(PValueBackup, backup->devices_grp_to_info[dev_id], grp_hdl);
    while (PValueBackup != NULL) {
      JLG(PValue, state->devices_grp_to_info[dev_id], grp_hdl);
      if (PValue) {
        bf_sys_free((grp_info_t *)*PValue);
        JLD(Rc_int, state->devices_grp_to_info[dev_id], grp_hdl);
        bf_sys_dbgchk(Rc_int != JERR);
      }
      if (*PValueBackup) {
        JLI(PValue, state->devices_grp_to_info[dev_id], grp_hdl);
        *PValue = *PValueBackup;
        *PValueBackup = 0;
      }
      JLN(PValueBackup, backup->devices_grp_to_info[dev_id], grp_hdl);
    }
  }

  p4_pd_ms_destroy_state(backup);
}

p4_pd_status_t p4_pd_ms_begin_txn(p4_pd_sess_hdl_t sess_hdl) {
  PWord_t PValue;
  JLI(PValue, txn_state, sess_hdl);
  if (PValue == PJERR) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

p4_pd_status_t p4_pd_ms_commit_txn(p4_pd_sess_hdl_t sess_hdl) {
  p4_pd_ms_txn_node_t *node;
  PWord_t PValue, PValueTxn;
  Word_t Index = 0;
  int Rc_int;

  JLG(PValueTxn, txn_state, sess_hdl);
  if (!PValueTxn) {
    return BF_OBJECT_NOT_FOUND;
  }

  JLF(PValue, (Pvoid_t)*PValueTxn, Index);
  while (PValue != NULL) {
    node = (p4_pd_ms_txn_node_t *)*PValue;
    p4_pd_ms_destroy_backup(node->backup);
    bf_sys_free(node);
    JLN(PValue, (Pvoid_t)*PValueTxn, Index);
  }
  JLD(Rc_int, txn_state, sess_hdl);
  if (Rc_int == JERR) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

p4_pd_status_t p4_pd_ms_abort_txn(p4_pd_sess_hdl_t sess_hdl) {
  p4_pd_ms_txn_node_t *node;
  PWord_t PValue, PValueTxn;
  Word_t Index = 0;
  int Rc_int;

  JLG(PValueTxn, txn_state, sess_hdl);
  if (!PValueTxn) {
    return BF_OBJECT_NOT_FOUND;
  }

  JLF(PValue, (Pvoid_t)*PValueTxn, Index);
  while (PValue != NULL) {
    node = (p4_pd_ms_txn_node_t *)*PValue;
    p4_pd_ms_restore_backup(node->state, node->backup);
    bf_sys_free(node);
    JLN(PValue, (Pvoid_t)*PValueTxn, Index);
  }
  JLD(Rc_int, txn_state, sess_hdl);
  if (Rc_int == JERR) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

void p4_pd_ms_log_state(bf_dev_id_t dev_id,
                        p4_pd_ms_table_state_t *state,
                        cJSON *prof) {
  PWord_t PValue;
  mbr_info_t *mbr_info;
  grp_info_t *grp_info;
  Word_t mbr_hdl, grp_hdl;
  Word_t Index = 0;
  int Rc_int, res_idx;
  cJSON *mbrs, *mbr, *grps, *grp;
  cJSON *mbr_grps, *mbr_grp, *resrs, *resr;

  cJSON_AddItemToObject(prof, "mbrs", mbrs = cJSON_CreateArray());
  mbr_hdl = 0;
  JLF(PValue, state->devices_mbr_to_info[dev_id], mbr_hdl);
  while (PValue != NULL) {
    mbr_info = (mbr_info_t *)*PValue;
    cJSON_AddItemToArray(mbrs, mbr = cJSON_CreateObject());
    cJSON_AddNumberToObject(mbr, "mbr_hdl", mbr_hdl);
    cJSON_AddNumberToObject(mbr, "act_hdl", mbr_info->act_hdl);

    cJSON_AddItemToObject(mbr, "grp_list", mbr_grps = cJSON_CreateArray());
    Index = 0;
    J1F(Rc_int, mbr_info->grp_list, Index);
    while (Rc_int) {
      cJSON_AddItemToArray(mbr_grps, mbr_grp = cJSON_CreateObject());
      cJSON_AddNumberToObject(mbr_grp, "grp_hdl", Index);
      J1N(Rc_int, mbr_info->grp_list, Index);
    }

    cJSON_AddItemToObject(mbr, "resources", resrs = cJSON_CreateArray());
    for (res_idx = 0; res_idx < mbr_info->resource_count; res_idx++) {
      cJSON_AddItemToArray(resrs, resr = cJSON_CreateObject());
      cJSON_AddNumberToObject(resr, "res_idx", res_idx);
      cJSON_AddNumberToObject(
          resr, "tbl_hdl", mbr_info->resources[res_idx].tbl_hdl);
      cJSON_AddNumberToObject(
          resr, "tbl_idx", mbr_info->resources[res_idx].tbl_idx);
    }

    JLN(PValue, state->devices_mbr_to_info[dev_id], mbr_hdl);
  }

  cJSON_AddItemToObject(prof, "grps", grps = cJSON_CreateArray());
  grp_hdl = 0;
  JLF(PValue, state->devices_grp_to_info[dev_id], grp_hdl);
  while (PValue != NULL) {
    grp_info = (grp_info_t *)*PValue;
    cJSON_AddItemToArray(grps, grp = cJSON_CreateObject());
    cJSON_AddNumberToObject(grp, "grp_hdl", grp_hdl);
    cJSON_AddNumberToObject(grp, "act_hdl", grp_info->act_hdl);
    cJSON_AddNumberToObject(grp, "mbr_count", grp_info->mbr_count);

    cJSON_AddItemToObject(grp, "resources", resrs = cJSON_CreateArray());
    for (res_idx = 0; res_idx < grp_info->resource_count; res_idx++) {
      cJSON_AddItemToArray(resrs, resr = cJSON_CreateObject());
      cJSON_AddNumberToObject(
          resr, "tbl_hdl", grp_info->resources[res_idx].tbl_hdl);
      cJSON_AddNumberToObject(
          resr, "tbl_idx", grp_info->resources[res_idx].tbl_idx);
    }

    JLN(PValue, state->devices_grp_to_info[dev_id], grp_hdl);
  }
}

void p4_pd_ms_restore_state(bf_dev_id_t dev_id,
                            p4_pd_ms_table_state_t *state,
                            cJSON *prof) {
  cJSON *mbrs, *mbr, *mbr_grps, *mbr_grp;
  cJSON *grps, *grp;
  cJSON *resrs, *resr;
  mbr_info_t *mbr_info;
  grp_info_t *grp_info;
  Word_t mbr_hdl, grp_hdl;
  uint32_t res_idx;
  PWord_t PValue;
  int Rc_int;

  mbrs = cJSON_GetObjectItem(prof, "mbrs");
  for (mbr = mbrs->child; mbr; mbr = mbr->next) {
    mbr_hdl = cJSON_GetObjectItem(mbr, "mbr_hdl")->valuedouble;
    JLG(PValue, state->devices_mbr_to_info[dev_id], mbr_hdl);
    if (PValue) {
      mbr_info = (mbr_info_t *)*PValue;
    } else {
      mbr_info = bf_sys_malloc(sizeof(mbr_info_t));
      memset(mbr_info, 0, sizeof(mbr_info_t));
      JLI(PValue, state->devices_mbr_to_info[dev_id], mbr_hdl);
      bf_sys_assert(!(*PValue));
      *PValue = (Word_t)mbr_info;
    }
    mbr_info->act_hdl = cJSON_GetObjectItem(mbr, "act_hdl")->valuedouble;

    mbr_grps = cJSON_GetObjectItem(mbr, "grp_list");
    for (mbr_grp = mbr_grps->child; mbr_grp; mbr_grp = mbr_grp->next) {
      grp_hdl = cJSON_GetObjectItem(mbr_grp, "grp_hdl")->valuedouble;
      J1S(Rc_int, mbr_info->grp_list, grp_hdl);
      (void)Rc_int;
    }

    resrs = cJSON_GetObjectItem(mbr, "resources");
    for (resr = resrs->child, res_idx = 0; resr; resr = resr->next, res_idx++) {
      mbr_info->resources[res_idx].tbl_hdl =
          cJSON_GetObjectItem(resr, "tbl_hdl")->valuedouble;
      mbr_info->resources[res_idx].tbl_idx =
          cJSON_GetObjectItem(resr, "tbl_idx")->valuedouble;
    }
    mbr_info->resource_count = res_idx;
  }

  grps = cJSON_GetObjectItem(prof, "grps");
  for (grp = grps->child; grp; grp = grp->next) {
    grp_hdl = cJSON_GetObjectItem(grp, "grp_hdl")->valuedouble;
    JLG(PValue, state->devices_grp_to_info[dev_id], grp_hdl);
    if (PValue) {
      grp_info = (grp_info_t *)*PValue;
    } else {
      grp_info = bf_sys_malloc(sizeof(grp_info_t));
      memset(grp_info, 0, sizeof(grp_info_t));
      JLI(PValue, state->devices_grp_to_info[dev_id], grp_hdl);
      bf_sys_assert(!(*PValue));
      *PValue = (Word_t)grp_info;
    }
    grp_info->act_hdl = cJSON_GetObjectItem(grp, "act_hdl")->valuedouble;
    grp_info->mbr_count = cJSON_GetObjectItem(grp, "mbr_count")->valuedouble;

    resrs = cJSON_GetObjectItem(grp, "resources");
    for (resr = resrs->child, res_idx = 0; resr; resr = resr->next, res_idx++) {
      grp_info->resources[res_idx].tbl_hdl =
          cJSON_GetObjectItem(resr, "tbl_hdl")->valuedouble;
      grp_info->resources[res_idx].tbl_idx =
          cJSON_GetObjectItem(resr, "tbl_idx")->valuedouble;
    }
    grp_info->resource_count = res_idx;
  }
}
