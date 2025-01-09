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


#ifndef _MC_MGR_H
#define _MC_MGR_H

#include <mc_mgr/mc_mgr_types.h>
#include "mc_mgr_int.h"
#include <dvm/bf_drv_intf.h>

mc_rmutex_t *mc_mgr_ctx_lock();
bool mc_mgr_ready();
bool mc_mgr_dev_present(bf_dev_id_t dev);

bf_status_t mc_mgr_create_session(bf_mc_session_hdl_t *shdl);

bf_status_t mc_mgr_init();
mc_mgr_port_info_t *mc_mgr_get_port_info(bf_dev_id_t dev, uint16_t port_id);

bf_status_t mc_mgr_mgrp_create(bf_dev_id_t dev, bf_mc_grp_id_t mgid);
bf_status_t mc_mgr_mgrp_destroy(int sid, bf_dev_id_t dev, bf_mc_grp_id_t mgid);
bf_status_t mc_mgr_mgrp_get_first_node_mbr(int sid,
                                           bf_dev_id_t dev,
                                           bf_mc_grp_id_t mgid,
                                           bf_mc_node_hdl_t *node_hdl,
                                           bool *node_l1_xid_valid,
                                           bf_mc_l1_xid_t *node_l1_xid);
bf_status_t mc_mgr_mgrp_get_node_mbr_count(int sid,
                                           bf_dev_id_t dev,
                                           bf_mc_grp_id_t mgid,
                                           uint32_t *count);
bf_status_t mc_mgr_mgrp_get_next_i_node_mbr(int sid,
                                            bf_dev_id_t dev,
                                            bf_mc_grp_id_t mgid,
                                            bf_mc_node_hdl_t node_hdl,
                                            uint32_t i,
                                            bf_mc_node_hdl_t *next_node_hdls,
                                            bool *next_node_l1_xids_valid,
                                            bf_mc_l1_xid_t *next_node_l1_xids);
bf_status_t mc_mgr_mgrp_get_first_ecmp_mbr(int sid,
                                           bf_dev_id_t dev,
                                           bf_mc_grp_id_t mgid,
                                           bf_mc_ecmp_hdl_t *ecmp_hdl,
                                           bool *ecmp_l1_xid_valid,
                                           bf_mc_l1_xid_t *ecmp_l1_xid);
bf_status_t mc_mgr_mgrp_get_ecmp_mbr_count(int sid,
                                           bf_dev_id_t dev,
                                           bf_mc_grp_id_t mgid,
                                           uint32_t *count);
bf_status_t mc_mgr_mgrp_get_next_i_ecmp_mbr(int sid,
                                            bf_dev_id_t dev,
                                            bf_mc_grp_id_t mgid,
                                            bf_mc_ecmp_hdl_t ecmp_hdl,
                                            uint32_t i,
                                            bf_mc_ecmp_hdl_t *next_ecmp_hdls,
                                            bool *next_ecmp_l1_xids_valid,
                                            bf_mc_l1_xid_t *next_ecmp_l1_xids);
bf_status_t mc_mgr_ecmp_alloc(int sid, bf_dev_id_t dev, bf_mc_ecmp_hdl_t h);
bf_status_t mc_mgr_ecmp_free(int sid, bf_dev_id_t dev, mc_ecmp_grp_t *g);
bf_status_t mc_mgr_ecmp_mbr_add(int sid,
                                bf_dev_id_t dev,
                                mc_ecmp_grp_t *g,
                                mc_l1_node_t *n);
bf_status_t mc_mgr_ecmp_mbr_rem(int sid,
                                bf_dev_id_t dev,
                                mc_ecmp_grp_t *g,
                                mc_l1_node_t *n);
bf_status_t mc_mgr_ecmp_mbr_mod(int sid,
                                bf_dev_id_t dev,
                                mc_ecmp_grp_t *g,
                                mc_l1_node_t **ns,
                                uint32_t size);
bf_status_t mc_mgr_ecmp_associate(int sid,
                                  bf_dev_id_t dev,
                                  bf_mc_grp_id_t mgid,
                                  mc_ecmp_grp_t *ecmp,
                                  bf_mc_l1_xid_t xid,
                                  bool use_xid);
bf_status_t mc_mgr_ecmp_dissociate(int sid,
                                   bf_dev_id_t dev,
                                   bf_mc_grp_id_t mgid,
                                   mc_ecmp_grp_t *ecmp);

bf_status_t mc_mgr_l1_associate(int sid,
                                mc_l1_node_t *node,
                                bf_mc_grp_id_t mgid,
                                bf_dev_id_t dev,
                                bf_mc_l1_xid_t xid,
                                bool use_xid);
bf_status_t mc_mgr_l1_dissociate(int sid,
                                 mc_l1_node_t *node,
                                 bf_mc_grp_id_t mgid);
bf_status_t mc_mgr_lag_update(int sid,
                              bf_dev_id_t dev,
                              int lag_id,
                              bf_mc_port_map_t port_map);
bf_status_t mc_mgr_lag_update_rmt_cnt(
    int sid, bf_dev_id_t dev, int lag_id, int l_cnt, int r_cnt);
bf_status_t mc_mgr_get_port_fwd_state(int sid,
                                      bf_dev_id_t dev,
                                      int port_bit_index,
                                      bool *is_active);
bf_status_t mc_mgr_set_port_fwd_state(int sid,
                                      bf_dev_id_t dev,
                                      int port_bit_index,
                                      bool inactive);
bf_status_t mc_mgr_set_port_ff_mode(int sid, bf_dev_id_t dev, bool en);
bf_status_t mc_mgr_clr_port_ff_state(bf_dev_id_t dev, int port_bit_index);
bf_status_t mc_mgr_get_port_ff_state(bf_dev_id_t dev,
                                     int port_bit_index,
                                     bool *is_active);
bf_status_t mc_mgr_set_backup_port_mode(int sid, bf_dev_id_t dev, bool en);
bf_status_t mc_mgr_set_backup_port(int sid,
                                   bf_dev_id_t dev,
                                   int protected_bit_idx,
                                   int backup_bit_idx);
bf_status_t mc_mgr_get_backup_port(int sid,
                                   bf_dev_id_t dev,
                                   int protected_bit_idx,
                                   int *backup_port_bit_idx);
bf_status_t mc_mgr_get_c2c(int sid,
                           bf_dev_id_t dev,
                           bf_dev_port_t *port,
                           bool *enable);
bf_status_t mc_mgr_set_c2c(int sid,
                           bf_dev_id_t dev,
                           bool enable,
                           bf_dev_port_t local_port);
bf_status_t mc_mgr_set_l1_time_slice(int sid, bf_dev_id_t dev, int count);
bf_status_t mc_mgr_set_max_nodes(int sid, bf_dev_id_t dev, int l1, int l2);

bf_status_t mc_mgr_get_lag_hw(bf_dev_id_t dev,
                              int ver,
                              int id,
                              bf_bitset_t *lag);
bf_status_t mc_mgr_get_lag_np_hw(
    bf_dev_id_t dev, int ver, int id, int *l_cnt, int *r_cnt);
bf_status_t mc_mgr_set_lag_np(
    bf_dev_id_t dev, int ver, int id, int l_cnt, int r_cnt);
bf_status_t mc_mgr_update_yid_tbl(int sid,
                                  bf_dev_id_t dev,
                                  int yid,
                                  bf_mc_port_map_t port_map);

bf_status_t mc_get_port_prune_table_size(bf_dev_id_t dev_id, uint32_t *count);

bf_status_t mc_mgr_set_pmt_seg(
    bf_dev_id_t dev, int ver, int id, int seg, bf_bitset_t *pmt);
bf_status_t mc_mgr_set_global_rid(bf_dev_id_t dev, uint16_t rid);
bf_status_t mc_mgr_get_global_rid(bf_dev_id_t dev, uint16_t *rid);
bf_status_t mc_mgr_get_blk_id_grp_hw(bf_dev_id_t dev, int idx, uint32_t *id);
uint32_t mc_mgr_get_bit_mask(int bit_cnt);

bool mc_mgr_in_batch(int sid);
void mc_mgr_begin_batch(int sid);
bf_status_t mc_mgr_flush_batch(int sid);
bf_status_t mc_mgr_end_batch(int sid);
bool mc_mgr_is_device_locked(bf_dev_id_t dev_id);
bool mc_mgr_versioning_on(int sid, bf_dev_id_t dev_id);
bool mc_mgr_in_batch(int sid);
void mc_mgr_enable_all_dr(bf_dev_id_t dev_id);
void mc_mgr_disable_all_dr(bf_dev_id_t dev_id);
bool mc_mgr_l1_node_contains_port(mc_l1_node_t *node, bf_dev_port_t port_id);

bf_status_t mc_mgr_read_hw_state(int sid,
                                 bf_dev_id_t dev,
                                 struct mc_mgr_dev_hw_state *state);
#endif
