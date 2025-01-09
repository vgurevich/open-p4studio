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


bf_status_t mc_mgr_get_bkup_port_reg(bf_dev_id_t dev,
                                     int ver,
                                     int port_bit_idx,
                                     int *bkup_bit_idx);
bf_status_t mc_mgr_set_bkup_port_wrl(
    int sid, bf_dev_id_t dev, int ver, int port_bit_idx, int bkup_bit_idx);

bf_status_t mc_mgr_get_lit_np_reg(
    bf_dev_id_t dev, int ver, int id, int *l_cnt, int *r_cnt);
bf_status_t mc_mgr_set_lit_np_wrl(int sid, bf_dev_id_t dev, int ver, int id);

bf_status_t mc_mgr_get_lit_seg_reg(
    bf_dev_id_t dev, int ver, int id, int seg, bf_bitset_t *val);
bf_status_t mc_mgr_set_lit_wrl(int sid, bf_dev_id_t dev, int ver, int lag_id);

bf_status_t mc_mgr_get_pmt_seg_reg(
    bf_dev_id_t dev, int ver, int yid, int seg, bf_bitset_t *val);
bf_status_t mc_mgr_set_pmt_wrl(int sid, bf_dev_id_t dev, int ver, int yid);

bf_status_t mc_mgr_get_mit_row_reg(bf_dev_id_t dev,
                                   int pipe,
                                   int row,
                                   uint32_t *mit0,
                                   uint32_t *mit1,
                                   uint32_t *mit2,
                                   uint32_t *mit3);
bf_status_t mc_mgr_set_mit_wrl(int sid, bf_dev_id_t dev, int pipe, int mgid);

bf_status_t mc_mgr_get_rdm_reg(
    int sid, bf_dev_id_t dev, int line, uint64_t *hi, uint64_t *lo);
bf_status_t mc_mgr_set_rdm_wrl(
    int sid, bf_dev_id_t dev, int line, uint64_t hi, uint64_t lo);
