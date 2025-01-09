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


bf_status_t mc_mgr_get_tbl_ver_reg(bf_dev_id_t dev,
                                   bf_dev_pipe_t pipe,
                                   int *ver);

bf_status_t mc_mgr_get_pvt_reg(bf_dev_id_t dev,
                               int phy_pipe,
                               int table,
                               int mgid_grp,
                               mc_mgr_pvt_entry_t *val);

bf_status_t mc_mgr_get_ver_cnt_reg(bf_dev_id_t dev,
                                   bf_dev_pipe_t pipe,
                                   int ver,
                                   uint32_t *count);

bf_status_t mc_mgr_get_global_rid_reg(bf_dev_id_t dev, uint16_t *rid);
bf_status_t mc_mgr_set_global_rid_reg(bf_dev_id_t dev, uint16_t rid);

bf_status_t mc_mgr_get_rdm_blk_id_grp_reg(bf_dev_id_t dev,
                                          int index,
                                          uint32_t *id);
bf_status_t mc_mgr_set_rdm_blk_id_grp_wrl(int sid, bf_dev_id_t dev, int grp);

bf_status_t mc_mgr_get_port_mask_reg(
    bf_dev_id_t dev, int ver, int port, uint32_t *blk, bool *val);
bf_status_t mc_mgr_set_port_mask_reg(bf_dev_id_t dev, int ver, int port);
bf_status_t mc_mgr_set_port_mask_wrl(int sid,
                                     bf_dev_id_t dev,
                                     int ver,
                                     int port);

/* pre_common.common_ctrl */
bf_status_t mc_mgr_get_comm_ctrl_reg(bf_dev_id_t dev,
                                     uint8_t *pipe_sel,
                                     bool *ff_en,
                                     bool *bkup_en);
bf_status_t mc_mgr_set_comm_ctrl_wrl(int sid, bf_dev_id_t dev);

bf_status_t mc_mgr_get_port_down_reg(bf_dev_id_t dev, int port, bool *down);
bf_status_t mc_mgr_clr_port_down_reg(bf_dev_id_t dev, int port);

/* pre.ctrl */
bf_status_t mc_mgr_set_pre_ctrl_wrl(int sid, bf_dev_id_t dev);
bf_status_t mc_mgr_get_pre_ctrl_reg(bf_dev_id_t dev,
                                    bf_dev_pipe_t pipe,
                                    bool *c2c_en,
                                    int *c2c_port,
                                    uint8_t *l1_slice);

bf_status_t mc_mgr_get_max_l1_reg(bf_dev_id_t dev,
                                  bf_dev_pipe_t pipe,
                                  int *count);
bf_status_t mc_mgr_set_max_l1_wrl(int sid, bf_dev_id_t dev);
bf_status_t mc_mgr_get_max_l2_reg(bf_dev_id_t dev,
                                  bf_dev_pipe_t pipe,
                                  int *count);
bf_status_t mc_mgr_set_max_l2_wrl(int sid, bf_dev_id_t dev);
bf_status_t mc_mgr_c2c_pipe_msk_set_reg_wrl(int sid,
                                            bf_dev_id_t dev,
                                            uint32_t mask);
