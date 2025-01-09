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


#ifndef __MC_MGR_BH_H__
#define __MC_MGR_BH_H__

#include <mc_mgr/mc_mgr_types.h>
#include "mc_mgr_int.h"

void mc_mgr_one_at_a_time_begin();
bf_status_t mc_mgr_one_at_a_time_begin_try();
void mc_mgr_one_at_a_time_end();

mc_l1_node_t *mc_mgr_lookup_l1_node(bf_dev_id_t dev,
                                    bf_mc_node_hdl_t h,
                                    const char *where,
                                    const int line);
mc_ecmp_grp_t *mc_mgr_lookup_ecmp(bf_dev_id_t dev,
                                  bf_mc_ecmp_hdl_t h,
                                  const char *where,
                                  const int line);

int mc_mgr_validate_session(bf_mc_session_hdl_t hdl,
                            const char *where,
                            const int line);
bool mc_mgr_validate_dev(bf_dev_id_t dev, const char *where, const int line);

bool mc_mgr_get_mgid_map_bit(bf_dev_id_t dev, int grp);
void mc_mgr_set_mgid_map_bit(bf_dev_id_t dev, int grp, bool val);

int mc_mgr_calculate_pipe_mask(mc_l1_node_t *node);
int mc_mgr_current_pipe_mask(mc_l1_node_t *node);
bf_status_t mc_mgr_update_tvt(
    int sid, int dev, int mgid, bool batch, const char *where, const int line);
bf_status_t mc_mgr_update_pvt(int sid,
                              bf_dev_id_t dev,
                              int mgid,
                              bool batch,
                              const char *where,
                              const int line);
void mc_mgr_get_l2_chain_sz(int pipe,
                            mc_l1_node_t *node,
                            int *port_cnt,
                            int *lag_cnt);
void mc_mgr_get_port_node_size(bf_dev_id_t dev,
                               bf_dev_pipe_t pipe,
                               mc_l1_node_t *node,
                               int *cnt,
                               uint8_t *mask);
int mc_mgr_compute_longest_l2(bf_dev_id_t dev, int pipe, int mgid);
bf_status_t mc_mgr_set_l1_tail(
    int sid, bf_dev_id_t dev, int pipe, int mgid, int len);
bool mc_mgr_update_len(int sid, bf_dev_id_t dev, int pipe, int mgid, int len);
bf_status_t mc_mgr_allocate_l2_chain(int sid,
                                     bf_dev_id_t dev,
                                     int pipe,
                                     mc_l1_node_t *node,
                                     uint32_t *l2_length,
                                     mc_mgr_rdm_addr_list_t **port_addrs,
                                     mc_mgr_rdm_addr_list_t **lag_addrs);
int mc_mgr_write_l2_port_node(int sid,
                              mc_l1_node_t *node,
                              uint32_t new_rdm_addr,
                              uint8_t mask);
bf_status_t mc_mgr_l1_write(int sid, mc_l1_node_t *node);
bf_status_t mc_mgr_ecmp_l1_write(int sid, mc_l1_node_t *node);
bf_status_t mc_mgr_l1_remove(int sid, mc_l1_node_t *node);

void mc_mgr_collect_l1s_l2_chain_addrs(bf_dev_id_t dev,
                                       uint32_t l1_rdm_addr,
                                       mc_mgr_rdm_addr_list_t **addrs);
void mc_mgr_delete_l1(mc_l1_hw_node_t *n);
void mc_mgr_delete_l1s_l2_chain(int sid, bf_dev_id_t dev, uint32_t l1_rdm_addr);
void mc_mgr_delete_l1_node_from_pipe(int sid,
                                     mc_l1_node_t *node,
                                     int pipe,
                                     mc_mgr_rdm_addr_list_t **to_free);
void mc_mgr_write_l1_rid_node(int sid,
                              int pipe,
                              mc_l1_node_t *node,
                              uint32_t l1_rdm_addr,
                              uint32_t l2_rdm_addr);
bf_status_t mc_mgr_ecmp_ptr_write(int sid, mc_l1_node_t *node);
bf_status_t mc_mgr_ecmp_ptr_remove(int sid, mc_l1_node_t *node);

void mc_mgr_write_actv_vector(int sid,
                              bf_dev_id_t dev,
                              mc_ecmp_grp_t *g,
                              uint32_t new_vec,
                              uint32_t *new_base);
void mc_mgr_write_stby_vector(int sid,
                              bf_dev_id_t dev,
                              mc_ecmp_grp_t *g,
                              uint32_t new_vec,
                              uint32_t *new_base);
void mc_mgr_ecmp_update_tail_reevaluate(int sid,
                                        bf_dev_id_t dev,
                                        mc_ecmp_grp_t *g);
bf_status_t mc_mgr_get_l1_ecmps(
    int sid,
    bf_dev_id_t dev,
    int size,
    mc_mgr_rdm_addr_list_t *l1_ecmp_addrs_arr[MC_MGR_NUM_PIPES]);
bf_status_t mc_mgr_ecmp_first_mbr_add(int sid,
                                      bf_dev_id_t dev,
                                      mc_ecmp_grp_t *g,
                                      mc_l1_node_t *node);
bf_status_t mc_mgr_ecmp_mbr_add_to_hole(int sid,
                                        bf_dev_id_t dev,
                                        mc_ecmp_grp_t *g,
                                        mc_l1_node_t *node);
bf_status_t mc_mgr_ecmp_mbr_add_to_end(int sid,
                                       bf_dev_id_t dev,
                                       mc_ecmp_grp_t *g,
                                       mc_l1_node_t *node);
bf_status_t mc_mgr_write_one_ecmp_mbr(int sid,
                                      bf_dev_id_t dev,
                                      uint32_t *addr,
                                      uint32_t offset,
                                      mc_l1_node_t *mbr,
                                      mc_ecmp_grp_t *grp,
                                      mc_mgr_rdm_addr_list_t **port_addrs,
                                      mc_mgr_rdm_addr_list_t **lag_addrs);
bf_status_t mc_mgr_ecmp_mbr_rem_one_mbr(int sid,
                                        bf_dev_id_t dev,
                                        mc_ecmp_grp_t *g,
                                        mc_l1_node_t *node);
bf_status_t mc_mgr_ecmp_add_block_n_one_mbr(
    int sid,
    bf_dev_id_t dev,
    uint32_t offset,
    int add_mask,
    mc_l1_node_t *node,
    mc_ecmp_grp_t *grp,
    mc_mgr_rdm_addr_list_t **l1_end_addrs,
    mc_mgr_rdm_addr_list_t **port_addrs,
    mc_mgr_rdm_addr_list_t **lag_addrs);
bf_status_t mc_mgr_ecmp_upd_one_mbr(int sid,
                                    bf_dev_id_t dev,
                                    uint32_t offset,
                                    int upd_mask,
                                    mc_l1_node_t *node,
                                    mc_ecmp_grp_t *grp,
                                    mc_mgr_rdm_addr_list_t **port_addrs,
                                    mc_mgr_rdm_addr_list_t **lag_addrs);
bf_status_t mc_mgr_write_one_ecmp_mbr_l1_ecmps(
    int sid,
    bf_dev_id_t dev,
    int upd_mask,
    mc_l1_node_t *l1_ecmp_node,
    mc_ecmp_grp_t *grp,
    mc_mgr_rdm_addr_list_t **l1_ecmp_addrs);

bf_status_t mc_mgr_get_ecmp_blocks(
    int sid,
    bf_dev_id_t dev,
    int size,
    int add_mask,
    mc_mgr_rdm_addr_list_t *l1_end_addrs_arr[MC_MGR_NUM_PIPES]);

void mc_mgr_move_mbr_block(int sid,
                           bf_dev_id_t dev,
                           mc_ecmp_grp_t *g,
                           mc_mgr_rdm_addr_list_t *l1_dst[MC_MGR_NUM_PIPES]);
void mc_mgr_free_mbr_block(int sid, bf_dev_id_t dev, mc_ecmp_grp_t *g);

uint32_t mc_mgr_get_mit(bf_dev_id_t dev, int pipe, int mgid);

uint8_t mc_mgr_get_pvt(bf_dev_id_t dev, int mgid);
mc_mgr_pvt_entry_t mc_mgr_get_pvt_row(bf_dev_id_t dev, int row);

uint8_t mc_mgr_get_tvt(int dev, int mgid);
uint8_t mc_mgr_get_tvt_hw(bf_dev_id_t dev, int pipe, int mgid);

bool mc_mgr_get_tbl_ver(bf_dev_id_t dev);

bf_status_t mc_mgr_c2c_pipe_msk_get_hw(bf_dev_id_t dev,
                                       int pipe,
                                       uint32_t *mask);

bf_status_t mc_mgr_program_pvt(int sid,
                               bf_dev_id_t dev,
                               uint16_t mgid,
                               uint8_t mask,
                               uint32_t tbl_msk,
                               bool batch,
                               const char *where,
                               const int line);
bf_status_t mc_mgr_program_tvt(int sid,
                               uint8_t dev,
                               uint16_t mgid,
                               uint8_t mask,
                               bool batch,
                               const char *where,
                               const int line);
void mc_mgr_program_pvt_shadow(bf_dev_id_t dev, uint16_t mgid, uint8_t mask);

bf_status_t mc_mgr_tvt_write_row_from_shadow(bf_dev_id_t dev,
                                             bf_dev_pipe_t pipe,
                                             uint16_t row,
                                             bool batch);

bf_status_t mc_mgr_pvt_write_row_from_shadow(bf_dev_id_t dev,
                                             int sid,
                                             int pipe,
                                             uint16_t row,
                                             uint32_t tbl_msk,
                                             bool batch);
bf_status_t mc_mgr_program_tbl_ver(int sid, bf_dev_id_t dev, bool ver);
bf_status_t mc_mgr_wait_for_ver_drain(bf_dev_id_t dev, int ver);
uint16_t mc_mgr_current_ecmp_pipe_mask(mc_ecmp_grp_t *grp);
bf_status_t mc_mgr_lag_to_node_map_rmv(bf_dev_id_t dev,
                                       int lag_id,
                                       mc_l1_node_t *node);
bf_status_t mc_mgr_lag_to_node_map_add(bf_dev_id_t dev,
                                       int lag_id,
                                       mc_l1_node_t *node);
int mc_mgr_calculate_lag2pipe_mask(bf_dev_id_t dev, int lag_id);
bf_status_t mc_mgr_update_lag_to_l1_nodes(int sid, bf_dev_id_t dev, int lag_id);
bool mc_mgr_is_prot_port_node_lags(bf_dev_id_t dev,
                                   mc_l1_node_t *node,
                                   bf_dev_port_t port);
bf_status_t mc_mgr_set_backup_ecmps(int sid,
                                    bf_dev_id_t dev,
                                    int *old_lag_mask,
                                    int *new_lag_mask,
                                    int protected_bit_idx,
                                    int backup_bit_idx,
                                    int old_backup,
                                    bf_bitset_t *prot_port_lags);
int mc_mgr_calculate_backup_l1_add_mask(mc_l1_node_t *node,
                                        int old_backup_idx,
                                        int new_backup_idx);
int mc_mgr_calculate_backup_del_mask(mc_l1_node_t *node,
                                     int old_backup_idx,
                                     int new_backup_idx);

int mc_mgr_calculate_backup_mod_pm(mc_l1_node_t *node,
                                   int old_backup_idx,
                                   int new_backup_idx);
#endif
