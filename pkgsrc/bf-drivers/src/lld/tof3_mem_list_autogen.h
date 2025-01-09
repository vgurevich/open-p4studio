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


/* clang-format off */

reg_decoder_fld_t tof3_tm_wac_pvt_table__mgid1_tbl_fld_list[] = {
    { "mgid_map_0", 8, 0, 0, 0,
      "9 bit - MGID pipe vector table map" },
    { "mgid_map_1", 17, 9, 0, 0,
      "9 bit - MGID pipe vector table map" },
    { "mgid_map_2", 26, 18, 0, 0,
      "9 bit - MGID pipe vector table map" },
    { "mgid_map_3", 35, 27, 0, 0,
      "9 bit - MGID pipe vector table map" },
    { "mgid_map_4", 44, 36, 0, 0,
      "9 bit - MGID pipe vector table map" },
    { "mgid_map_5", 53, 45, 0, 0,
      "9 bit - MGID pipe vector table map" },
    { "mgid_map_6", 62, 54, 0, 0,
      "9 bit - MGID pipe vector table map" },
    { "mgid_map_7", 71, 63, 0, 0,
      "9 bit - MGID pipe vector table map" },
};
reg_decoder_t tof3_tm_wac_pvt_table__mgid1_tbl = { 8, tof3_tm_wac_pvt_table__mgid1_tbl_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_wac_pvt_table__mgid2_tbl_fld_list[] = {
    { "mgid_map_0", 8, 0, 0, 0,
      "9 bit - MGID pipe vector table map" },
    { "mgid_map_1", 17, 9, 0, 0,
      "9 bit - MGID pipe vector table map" },
    { "mgid_map_2", 26, 18, 0, 0,
      "9 bit - MGID pipe vector table map" },
    { "mgid_map_3", 35, 27, 0, 0,
      "9 bit - MGID pipe vector table map" },
    { "mgid_map_4", 44, 36, 0, 0,
      "9 bit - MGID pipe vector table map" },
    { "mgid_map_5", 53, 45, 0, 0,
      "9 bit - MGID pipe vector table map" },
    { "mgid_map_6", 62, 54, 0, 0,
      "9 bit - MGID pipe vector table map" },
    { "mgid_map_7", 71, 63, 0, 0,
      "9 bit - MGID pipe vector table map" },
};
reg_decoder_t tof3_tm_wac_pvt_table__mgid2_tbl = { 8, tof3_tm_wac_pvt_table__mgid2_tbl_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_port_ppg_mapping_fld_list[] = {
    { "ppg0", 7, 0, 0, 0,
      "8 bit - Priority Group Index 0" },
    { "apid0", 9, 8, 0, 0,
      "2 bit - Application Service Pool ID  0" },
    { "enb0", 10, 10, 0, 0,
      "1 bit - Is PPG Index Valid" },
    { "ppg1", 18, 11, 0, 0,
      "8 bit - Priority Group Index 1" },
    { "apid1", 20, 19, 0, 0,
      "2 bit - Application Service Pool ID  1" },
    { "enb1", 21, 21, 0, 0,
      "1 bit - Is PPG Index Valid" },
};
reg_decoder_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_port_ppg_mapping = { 6, tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_port_ppg_mapping_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_min_cnt_fld_list[] = {
    { "cnt", 18, 0, 0, 0,
      "19 bit - PPG cell count" },
};
reg_decoder_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_min_cnt = { 1, tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_min_cnt_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_shr_cnt_fld_list[] = {
    { "cnt", 18, 0, 0, 0,
      "19 bit - PPG cell count" },
};
reg_decoder_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_shr_cnt = { 1, tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_shr_cnt_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_hdr_cnt_fld_list[] = {
    { "cnt", 18, 0, 0, 0,
      "19 bit - PPG cell count" },
};
reg_decoder_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_hdr_cnt = { 1, tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_hdr_cnt_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_min_th_fld_list[] = {
    { "cnt", 18, 0, 0, 0,
      "19 bit - PPG cell threshold" },
};
reg_decoder_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_min_th = { 1, tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_min_th_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_shr_th_fld_list[] = {
    { "cnt", 18, 0, 0, 0,
      "19 bit - PPG cell threshold" },
};
reg_decoder_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_shr_th = { 1, tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_shr_th_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_hdr_th_fld_list[] = {
    { "cnt", 18, 0, 0, 0,
      "19 bit - PPG cell threshold" },
};
reg_decoder_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_hdr_th = { 1, tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_hdr_th_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_pfc_fld_list[] = {
    { "pfc", 0, 0, 0, 0,
      "1 bit - Pause state" },
};
reg_decoder_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_pfc = { 1, tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_pfc_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_icos_fld_list[] = {
    { "icos", 7, 0, 0, 0,
      "8 bit - Mapping table used to map PPG to iCoS" },
};
reg_decoder_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_icos = { 1, tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_icos_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_drop_st_fld_list[] = {
    { "drop_st", 0, 0, 0, 0,
      "1 bit - PPG Drop state" },
};
reg_decoder_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_drop_st = { 1, tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_drop_st_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_drop_st_fld_list[] = {
    { "drop_st", 0, 0, 0, 0,
      "1 bit - PG Drop state" },
};
reg_decoder_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_drop_st = { 1, tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_drop_st_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_off_idx_fld_list[] = {
    { "off_idx", 4, 0, 0, 0,
      "5 bit - PPG offset Index" },
    { "dyn", 5, 5, 0, 0,
      "1 bit - Dynamic buffer state" },
};
reg_decoder_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_off_idx = { 2, tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_off_idx_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_off_idx_fld_list[] = {
    { "off_idx", 4, 0, 0, 0,
      "5 bit - PPG offset Index" },
    { "dyn", 5, 5, 0, 0,
      "1 bit - Dynamic buffer state" },
};
reg_decoder_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_off_idx = { 2, tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_off_idx_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_min_cnt_fld_list[] = {
    { "cnt", 18, 0, 0, 0,
      "19 bit - Default PG cell count" },
};
reg_decoder_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_min_cnt = { 1, tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_min_cnt_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_shr_cnt_fld_list[] = {
    { "cnt", 18, 0, 0, 0,
      "19 bit - Default PG cell count" },
};
reg_decoder_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_shr_cnt = { 1, tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_shr_cnt_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_min_th_fld_list[] = {
    { "cnt", 18, 0, 0, 0,
      "19 bit - Default PG configuration" },
};
reg_decoder_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_min_th = { 1, tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_min_th_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_shr_th_fld_list[] = {
    { "cnt", 18, 0, 0, 0,
      "19 bit - Default PG configuration" },
};
reg_decoder_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_shr_th = { 1, tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_shr_th_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_port_shr_th_fld_list[] = {
    { "cnt", 18, 0, 0, 0,
      "19 bit - Port threshold" },
};
reg_decoder_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_port_shr_th = { 1, tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_port_shr_th_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_port_hdr_th_fld_list[] = {
    { "cnt", 18, 0, 0, 0,
      "19 bit - Port threshold" },
};
reg_decoder_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_port_hdr_th = { 1, tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_port_hdr_th_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_port_wm_fld_list[] = {
    { "cnt", 18, 0, 0, 0,
      "19 bit - Port Count" },
};
reg_decoder_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_port_wm = { 1, tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_port_wm_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_port_min_cnt_fld_list[] = {
    { "cnt", 18, 0, 0, 0,
      "19 bit - Port Minimum Count" },
};
reg_decoder_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_port_min_cnt = { 1, tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_port_min_cnt_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_port_hdr_cnt_fld_list[] = {
    { "cnt", 18, 0, 0, 0,
      "19 bit - Port hdr Count" },
};
reg_decoder_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_port_hdr_cnt = { 1, tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_port_hdr_cnt_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_port_shr_cnt_fld_list[] = {
    { "cnt", 18, 0, 0, 0,
      "19 bit - Port shr Count" },
};
reg_decoder_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_port_shr_cnt = { 1, tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_port_shr_cnt_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_port_st_fld_list[] = {
    { "shr_lmt", 0, 0, 0, 0,
      "1 bit - Port shr limit status" },
    { "hdr_lmt", 1, 1, 0, 0,
      "1 bit - Port hdr limit status" },
};
reg_decoder_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_port_st = { 2, tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_port_st_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_wm_cnt_fld_list[] = {
    { "cnt", 18, 0, 0, 0,
      "19 bit - PPG max usage count" },
};
reg_decoder_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_wm_cnt = { 1, tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_wm_cnt_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_drop_count_ppg_fld_list[] = {
    { "cnt", 39, 0, 0, 0,
      "40 bit - Per PG drop count" },
};
reg_decoder_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_drop_count_ppg = { 1, tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_drop_count_ppg_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_drop_count_port_fld_list[] = {
    { "cnt", 39, 0, 0, 0,
      "40 bit - Per Port drop count" },
};
reg_decoder_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_drop_count_port = { 1, tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_drop_count_port_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_pfc_state_fld_list[] = {
    { "port_ppg_state", 7, 0, 0, 0,
      "8 bit - Per ingress pipe port pfc/pause state" },
    { "rm_pfc_state", 15, 8, 0, 0,
      "8 bit - PFC state" },
    { "mac_pfc_out", 23, 16, 0, 0,
      "8 bit - MAC Pfc out state" },
    { "mac_pause_out", 24, 24, 0, 0,
      "1 bit - MAC Pause Out State" },
};
reg_decoder_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_pfc_state = { 4, tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_pfc_state_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_qid_map_fld_list[] = {
    { "qid_mid0", 4, 0, 0, 0,
      "5 bit - QID Mapping Index" },
    { "qid_mid1", 9, 5, 0, 0,
      "5 bit - QID Mapping Index" },
    { "qid_mid2", 14, 10, 0, 0,
      "5 bit - QID Mapping Index" },
    { "qid_mid3", 19, 15, 0, 0,
      "5 bit - QID Mapping Index" },
    { "qid_mid4", 24, 20, 0, 0,
      "5 bit - QID Mapping Index" },
    { "qid_mid5", 29, 25, 0, 0,
      "5 bit - QID Mapping Index" },
    { "qid_mid6", 34, 30, 0, 0,
      "5 bit - QID Mapping Index" },
    { "qid_mid7", 39, 35, 0, 0,
      "5 bit - QID Mapping Index" },
    { "qid_mid8", 44, 40, 0, 0,
      "5 bit - QID Mapping Index" },
    { "qid_mid9", 49, 45, 0, 0,
      "5 bit - QID Mapping Index" },
    { "qid_mid10", 54, 50, 0, 0,
      "5 bit - QID Mapping Index" },
    { "qid_mid11", 59, 55, 0, 0,
      "5 bit - QID Mapping Index" },
    { "qid_mid12", 64, 40, 0, 0,
      "5 bit - QID Mapping Index" },
    { "qid_mid13", 69, 65, 0, 0,
      "5 bit - QID Mapping Index" },
    { "qid_mid14", 74, 70, 0, 0,
      "5 bit - QID Mapping Index" },
    { "qid_mid15", 79, 75, 0, 0,
      "5 bit - QID Mapping Index" },
};
reg_decoder_t tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_qid_map = { 16, tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_qid_map_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_wac_common_mem_rspec__wac_common_mem_lcl_qstate_fld_list[] = {
    { "nomin", 0, 0, 0, 0,
      "1 bit - Nominal state" },
    { "red_off", 1, 1, 0, 0,
      "1 bit - Red Off" },
    { "yel_off", 2, 2, 0, 0,
      "1 bit - Yellow Off" },
    { "gre_off", 3, 3, 0, 0,
      "1 bit - Green Off" },
};
reg_decoder_t tof3_tm_wac_common_mem_rspec__wac_common_mem_lcl_qstate = { 4, tof3_tm_wac_common_mem_rspec__wac_common_mem_lcl_qstate_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_wac_common_mem_rspec__wac_common_mem_rmt_qstate_fld_list[] = {
    { "nomin", 0, 0, 0, 0,
      "1 bit - Nominal state" },
    { "red_off", 1, 1, 0, 0,
      "1 bit - Red Off" },
    { "yel_off", 2, 2, 0, 0,
      "1 bit - Yellow Off" },
    { "gre_off", 3, 3, 0, 0,
      "1 bit - Green Off" },
};
reg_decoder_t tof3_tm_wac_common_mem_rspec__wac_common_mem_rmt_qstate = { 4, tof3_tm_wac_common_mem_rspec__wac_common_mem_rmt_qstate_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_wac_common_mem_rspec__wac_common_mem_qacq_ap_config_fld_list[] = {
    { "sp_id", 1, 0, 0, 0,
      "2 bit - Service Pool Id" },
    { "yel_red_drop_en", 2, 2, 0, 0,
      "1 bit - Yellow and Red Queue Tail drop" },
    { "gre_drop_en", 3, 3, 0, 0,
      "1 bit - Green Queue Tail Drop" },
};
reg_decoder_t tof3_tm_wac_common_mem_rspec__wac_common_mem_qacq_ap_config = { 3, tof3_tm_wac_common_mem_rspec__wac_common_mem_qacq_ap_config_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_caa_mem_top_rspec__caa_block_grp0_fld_list[] = {
    { "ptr0", 9, 0, 0, 0,
      "10 bit - CDM block free cell pointer" },
    { "ptr1", 19, 10, 0, 0,
      "10 bit - CDM block free cell pointer" },
};
reg_decoder_t tof3_tm_caa_mem_top_rspec__caa_block_grp0 = { 2, tof3_tm_caa_mem_top_rspec__caa_block_grp0_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_caa_mem_top_rspec__caa_block_grp1_fld_list[] = {
    { "ptr0", 9, 0, 0, 0,
      "10 bit - CDM block free cell pointer" },
    { "ptr1", 19, 10, 0, 0,
      "10 bit - CDM block free cell pointer" },
};
reg_decoder_t tof3_tm_caa_mem_top_rspec__caa_block_grp1 = { 2, tof3_tm_caa_mem_top_rspec__caa_block_grp1_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_caa_mem_top_rspec__caa_block_grp2_fld_list[] = {
    { "ptr0", 9, 0, 0, 0,
      "10 bit - CDM block free cell pointer" },
    { "ptr1", 19, 10, 0, 0,
      "10 bit - CDM block free cell pointer" },
};
reg_decoder_t tof3_tm_caa_mem_top_rspec__caa_block_grp2 = { 2, tof3_tm_caa_mem_top_rspec__caa_block_grp2_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_caa_mem_top_rspec__caa_block_grp3_fld_list[] = {
    { "ptr0", 9, 0, 0, 0,
      "10 bit - CDM block free cell pointer" },
    { "ptr1", 19, 10, 0, 0,
      "10 bit - CDM block free cell pointer" },
};
reg_decoder_t tof3_tm_caa_mem_top_rspec__caa_block_grp3 = { 2, tof3_tm_caa_mem_top_rspec__caa_block_grp3_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_caa_mem_top_rspec__caa_block_grp4_fld_list[] = {
    { "ptr0", 9, 0, 0, 0,
      "10 bit - CDM block free cell pointer" },
    { "ptr1", 19, 10, 0, 0,
      "10 bit - CDM block free cell pointer" },
};
reg_decoder_t tof3_tm_caa_mem_top_rspec__caa_block_grp4 = { 2, tof3_tm_caa_mem_top_rspec__caa_block_grp4_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_caa_mem_top_rspec__caa_block_grp5_fld_list[] = {
    { "ptr0", 9, 0, 0, 0,
      "10 bit - CDM block free cell pointer" },
    { "ptr1", 19, 10, 0, 0,
      "10 bit - CDM block free cell pointer" },
};
reg_decoder_t tof3_tm_caa_mem_top_rspec__caa_block_grp5 = { 2, tof3_tm_caa_mem_top_rspec__caa_block_grp5_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_qac_pipe_mem_rspec__csr_memory_qac_queue_config_fld_list[] = {
    { "shr_cfg", 25, 0, 0, 0,
      "26 bit - Queue buffer threshold.[18:0] - Scheduler Limit[23:19] - Offset Index[24:24] - Dynamic Enable[25:25] - Fast Recovery Mode" },
    { "color_cfg", 41, 26, 0, 0,
      "16 bit - Queue color threshold limit[29:26] - Red limit Percentage[33:30] - Red Offset Index[37:34] - Yellow Limit Precentage[41:38] - Yeelow Offset Index" },
    { "ap_config", 45, 42, 0, 0,
      "4 bit - Queue Application Service Pool[43:42] - Application Service Pool ID[44:44] - Queue Color Drop Enable[45:45] - Queue Drop Enable" },
    { "min_lmt", 64, 46, 0, 0,
      "19 bit - Egress Queue Min buffer Threshold" },
    { "min_lmt_pending", 83, 65, 0, 0,
      "19 bit - Egress Queue Pending Min Buffer Threshold" },
    { "min_lmt_pending_val", 84, 84, 0, 0,
      "1 bit - Egress Queue Pending Min Buffer Threshold Valid " },
};
reg_decoder_t tof3_tm_qac_pipe_mem_rspec__csr_memory_qac_queue_config = { 6, tof3_tm_qac_pipe_mem_rspec__csr_memory_qac_queue_config_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_qac_pipe_mem_rspec__csr_memory_qac_queue_cell_count_fld_list[] = {
    { "queue_cell_count", 18, 0, 0, 0,
      "19 bit - Queue total cell usage count" },
};
reg_decoder_t tof3_tm_qac_pipe_mem_rspec__csr_memory_qac_queue_cell_count = { 1, tof3_tm_qac_pipe_mem_rspec__csr_memory_qac_queue_cell_count_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_qac_pipe_mem_rspec__csr_memory_qac_queue_wm_cell_count_fld_list[] = {
    { "cell_count", 18, 0, 0, 0,
      "19 bit - Queue Water Mark total cell usage count" },
};
reg_decoder_t tof3_tm_qac_pipe_mem_rspec__csr_memory_qac_queue_wm_cell_count = { 1, tof3_tm_qac_pipe_mem_rspec__csr_memory_qac_queue_wm_cell_count_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_qac_pipe_mem_rspec__csr_memory_qac_port_wm_cell_count_fld_list[] = {
    { "cell_count", 18, 0, 0, 0,
      "19 bit - Port Water Mark total cell usage count" },
};
reg_decoder_t tof3_tm_qac_pipe_mem_rspec__csr_memory_qac_port_wm_cell_count = { 1, tof3_tm_qac_pipe_mem_rspec__csr_memory_qac_port_wm_cell_count_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_qac_pipe_mem_rspec__csr_memory_qac_port_config_fld_list[] = {
    { "port_thrd", 18, 0, 0, 0,
      "19 bit - Max buffer usage limit" },
    { "offset_idx", 23, 19, 0, 0,
      "5 bit - Index to offset_profile" },
    { "dod_mappedqid", 30, 24, 0, 0,
      "7 bit - Mapped Qid" },
};
reg_decoder_t tof3_tm_qac_pipe_mem_rspec__csr_memory_qac_port_config = { 3, tof3_tm_qac_pipe_mem_rspec__csr_memory_qac_port_config_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_qac_pipe_mem_rspec__csr_memory_qac_port_cell_count_fld_list[] = {
    { "port_cell_count", 18, 0, 0, 0,
      "19 bit - Port Cell Count" },
};
reg_decoder_t tof3_tm_qac_pipe_mem_rspec__csr_memory_qac_port_cell_count = { 1, tof3_tm_qac_pipe_mem_rspec__csr_memory_qac_port_cell_count_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_qac_pipe_mem_rspec__csr_memory_qac_drop_count_port_fld_list[] = {
    { "count", 39, 0, 0, 0,
      "40 bit - Egress Buffer Por Port  Packet Drop Count" },
};
reg_decoder_t tof3_tm_qac_pipe_mem_rspec__csr_memory_qac_drop_count_port = { 1, tof3_tm_qac_pipe_mem_rspec__csr_memory_qac_drop_count_port_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_qac_pipe_mem_rspec__csr_memory_qac_drop_count_queue_fld_list[] = {
    { "count", 39, 0, 0, 0,
      "40 bit - Egress Buffer Por Queue Packet Drop Count" },
};
reg_decoder_t tof3_tm_qac_pipe_mem_rspec__csr_memory_qac_drop_count_queue = { 1, tof3_tm_qac_pipe_mem_rspec__csr_memory_qac_drop_count_queue_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_qac_pipe_mem_rspec__csr_memory_qac_qid_mapping_fld_list[] = {
    { "qid_mid0", 3, 0, 0, 0,
      "4 bit - QID Mapping Index for MC Queue" },
    { "qid_mid1", 7, 4, 0, 0,
      "4 bit - QID Mapping Index for MC Queue" },
    { "qid_mid2", 11, 8, 0, 0,
      "4 bit - QID Mapping Index for MC Queue" },
    { "qid_mid3", 15, 12, 0, 0,
      "4 bit - QID Mapping Index for MC Queue" },
    { "qid_mid4", 19, 16, 0, 0,
      "4 bit - QID Mapping Index for MC Queue" },
    { "qid_mid5", 23, 20, 0, 0,
      "4 bit - QID Mapping Index for MC Queue" },
    { "qid_mid6", 27, 24, 0, 0,
      "4 bit - QID Mapping Index for MC Queue" },
    { "qid_mid7", 31, 28, 0, 0,
      "4 bit - QID Mapping Index for MC Queue" },
};
reg_decoder_t tof3_tm_qac_pipe_mem_rspec__csr_memory_qac_qid_mapping = { 8, tof3_tm_qac_pipe_mem_rspec__csr_memory_qac_qid_mapping_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_sch_pipe_mem_rspec__port_max_lb_static_mem_fld_list[] = {
    { "pps", 0, 0, 0, 0,
      "1 bit - 1 for pps & 0 for bps" },
    { "bs_exp", 4, 1, 0, 0,
      "4 bit - Bucket size - Expo" },
    { "bs_mant", 12, 5, 0, 0,
      "8 bit - Bucket size - Mantissa" },
    { "rate_exp", 16, 13, 0, 0,
      "4 bit - Rate - Expo" },
    { "rate_mant", 28, 17, 0, 0,
      "12 bit - Rate - Mantissa" },
};
reg_decoder_t tof3_tm_sch_pipe_mem_rspec__port_max_lb_static_mem = { 5, tof3_tm_sch_pipe_mem_rspec__port_max_lb_static_mem_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_sch_pipe_mem_rspec__port_max_lb_dynamic_mem_fld_list[] = {
    { "level", 24, 0, 0, 0,
      "25 bit - Bucket Level" },
};
reg_decoder_t tof3_tm_sch_pipe_mem_rspec__port_max_lb_dynamic_mem = { 1, tof3_tm_sch_pipe_mem_rspec__port_max_lb_dynamic_mem_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_sch_pipe_mem_rspec__l1_min_lb_static_mem_fld_list[] = {
    { "pps", 0, 0, 0, 0,
      "1 bit - 1 for pps & 0 for bps" },
    { "bs_exp", 4, 1, 0, 0,
      "4 bit - Bucket size - Expo" },
    { "bs_mant", 12, 5, 0, 0,
      "8 bit - Bucket size - Mantissa" },
    { "rate_exp", 16, 13, 0, 0,
      "4 bit - Rate - Expo" },
    { "rate_mant", 28, 17, 0, 0,
      "12 bit - Rate - Mantissa" },
};
reg_decoder_t tof3_tm_sch_pipe_mem_rspec__l1_min_lb_static_mem = { 5, tof3_tm_sch_pipe_mem_rspec__l1_min_lb_static_mem_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_sch_pipe_mem_rspec__l1_min_lb_dynamic_mem_fld_list[] = {
    { "level", 24, 0, 0, 0,
      "25 bit - Bucket Level" },
};
reg_decoder_t tof3_tm_sch_pipe_mem_rspec__l1_min_lb_dynamic_mem = { 1, tof3_tm_sch_pipe_mem_rspec__l1_min_lb_dynamic_mem_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_sch_pipe_mem_rspec__l1_max_lb_static_mem_fld_list[] = {
    { "pps", 0, 0, 0, 0,
      "1 bit - 1 for pps & 0 for bps" },
    { "bs_exp", 4, 1, 0, 0,
      "4 bit - Bucket size - Expo" },
    { "bs_mant", 12, 5, 0, 0,
      "8 bit - Bucket size - Mantissa" },
    { "rate_exp", 16, 13, 0, 0,
      "4 bit - Rate - Expo" },
    { "rate_mant", 28, 17, 0, 0,
      "12 bit - Rate - Mantissa" },
};
reg_decoder_t tof3_tm_sch_pipe_mem_rspec__l1_max_lb_static_mem = { 5, tof3_tm_sch_pipe_mem_rspec__l1_max_lb_static_mem_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_sch_pipe_mem_rspec__l1_max_lb_dynamic_mem_fld_list[] = {
    { "level", 24, 0, 0, 0,
      "25 bit - Bucket Level" },
};
reg_decoder_t tof3_tm_sch_pipe_mem_rspec__l1_max_lb_dynamic_mem = { 1, tof3_tm_sch_pipe_mem_rspec__l1_max_lb_dynamic_mem_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_sch_pipe_mem_rspec__l1_exc_static_mem_fld_list[] = {
    { "pps", 0, 0, 0, 0,
      "1 bit - Used to indicate rate is pps if set to '1' else it is bps" },
    { "wt", 10, 1, 0, 0,
      "10 bit - A 10b weight used for DWRR among the queues at same Max priority level. The value==0 is used to disable the DWRR especially when Max Rate Leakybucket is used" },
};
reg_decoder_t tof3_tm_sch_pipe_mem_rspec__l1_exc_static_mem = { 2, tof3_tm_sch_pipe_mem_rspec__l1_exc_static_mem_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_sch_pipe_mem_rspec__l1_exc_dynamic_mem_fld_list[] = {
    { "level", 20, 0, 0, 0,
      "21 bit - Signed current bucket level in bytes/packets (1b sign + 20b level)" },
};
reg_decoder_t tof3_tm_sch_pipe_mem_rspec__l1_exc_dynamic_mem = { 1, tof3_tm_sch_pipe_mem_rspec__l1_exc_dynamic_mem_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_sch_pipe_mem_rspec__q_min_lb_static_mem_fld_list[] = {
    { "pps", 0, 0, 0, 0,
      "1 bit - 1 for pps & 0 for bps" },
    { "bs_exp", 4, 1, 0, 0,
      "4 bit - Bucket size - Expo" },
    { "bs_mant", 12, 5, 0, 0,
      "8 bit - Bucket size - Mantissa" },
    { "rate_exp", 16, 13, 0, 0,
      "4 bit - Rate - Expo" },
    { "rate_mant", 28, 17, 0, 0,
      "12 bit - Rate - Mantissa" },
};
reg_decoder_t tof3_tm_sch_pipe_mem_rspec__q_min_lb_static_mem = { 5, tof3_tm_sch_pipe_mem_rspec__q_min_lb_static_mem_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_sch_pipe_mem_rspec__q_min_lb_dynamic_mem_fld_list[] = {
    { "level", 24, 0, 0, 0,
      "25 bit - Signed current bucket level in bytes/packets (1b sign + 24b level)" },
};
reg_decoder_t tof3_tm_sch_pipe_mem_rspec__q_min_lb_dynamic_mem = { 1, tof3_tm_sch_pipe_mem_rspec__q_min_lb_dynamic_mem_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_sch_pipe_mem_rspec__q_max_lb_static_mem_fld_list[] = {
    { "pps", 0, 0, 0, 0,
      "1 bit - 1 for pps & 0 for bps" },
    { "bs_exp", 4, 1, 0, 0,
      "4 bit - Bucket size - Expo" },
    { "bs_mant", 12, 5, 0, 0,
      "8 bit - Bucket size - Mantissa" },
    { "rate_exp", 16, 13, 0, 0,
      "4 bit - Rate - Expo" },
    { "rate_mant", 28, 17, 0, 0,
      "12 bit - Rate - Mantissa" },
};
reg_decoder_t tof3_tm_sch_pipe_mem_rspec__q_max_lb_static_mem = { 5, tof3_tm_sch_pipe_mem_rspec__q_max_lb_static_mem_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_sch_pipe_mem_rspec__q_max_lb_dynamic_mem_fld_list[] = {
    { "level", 24, 0, 0, 0,
      "25 bit - Signed current bucket level in bytes/packets (1b sign + 24b level)" },
};
reg_decoder_t tof3_tm_sch_pipe_mem_rspec__q_max_lb_dynamic_mem = { 1, tof3_tm_sch_pipe_mem_rspec__q_max_lb_dynamic_mem_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_sch_pipe_mem_rspec__q_exc_static_mem_fld_list[] = {
    { "pps", 0, 0, 0, 0,
      "1 bit - Used to indicate rate is pps if set to '1' else it is bps" },
    { "wt", 10, 1, 0, 0,
      "10 bit - A 10b weight used for DWRR among the queues at same Max priority level. The value==0 is used to disable the DWRR especially when Max Rate Leakybucket is used" },
};
reg_decoder_t tof3_tm_sch_pipe_mem_rspec__q_exc_static_mem = { 2, tof3_tm_sch_pipe_mem_rspec__q_exc_static_mem_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_sch_pipe_mem_rspec__q_exc_dynamic_mem_fld_list[] = {
    { "level", 20, 0, 0, 0,
      "21 bit - Signed current bucket level in bytes/packets (1b sign + 20b level)" },
};
reg_decoder_t tof3_tm_sch_pipe_mem_rspec__q_exc_dynamic_mem = { 1, tof3_tm_sch_pipe_mem_rspec__q_exc_dynamic_mem_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_sch_pipe_mem_rspec__q_pfc_status_mem_fld_list[] = {
    { "pfc_pause", 0, 0, 0, 0,
      "1 bit - Pause state" },
};
reg_decoder_t tof3_tm_sch_pipe_mem_rspec__q_pfc_status_mem = { 1, tof3_tm_sch_pipe_mem_rspec__q_pfc_status_mem_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_sch_pipe_mem_rspec__q_adv_fc_status_mem_fld_list[] = {
    { "adv_fc", 0, 0, 0, 0,
      "1 bit - Flow Control Status" },
};
reg_decoder_t tof3_tm_sch_pipe_mem_rspec__q_adv_fc_status_mem = { 1, tof3_tm_sch_pipe_mem_rspec__q_adv_fc_status_mem_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_sch_pipe_mem_rspec__p_occ_mem_fld_list[] = {
    { "node_occ_cntr", 7, 0, 0, 0,
      "8 bit - Occupancy Counter" },
    { "node_occ", 8, 8, 0, 0,
      "1 bit - Node occupancy indicator" },
};
reg_decoder_t tof3_tm_sch_pipe_mem_rspec__p_occ_mem = { 2, tof3_tm_sch_pipe_mem_rspec__p_occ_mem_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_sch_pipe_mem_rspec__l1_occ_mem_fld_list[] = {
    { "node_occ_cntr", 7, 0, 0, 0,
      "8 bit - Occupancy Counter" },
    { "node_occ", 8, 8, 0, 0,
      "1 bit - Node occupancy indicator" },
};
reg_decoder_t tof3_tm_sch_pipe_mem_rspec__l1_occ_mem = { 2, tof3_tm_sch_pipe_mem_rspec__l1_occ_mem_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_sch_pipe_mem_rspec__q_occ_mem_fld_list[] = {
    { "node_occ_cntr", 3, 0, 0, 0,
      "4 bit - Occupancy Counter" },
    { "node_occ", 4, 4, 0, 0,
      "1 bit - Node occupancy indicator" },
};
reg_decoder_t tof3_tm_sch_pipe_mem_rspec__q_occ_mem = { 2, tof3_tm_sch_pipe_mem_rspec__q_occ_mem_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_clc_pipe_mem_rspec__csr_memory_clc_clm_fld_list[] = {
    { "err", 0, 0, 0, 0,
      "1 bit - Error Indication" },
    { "eop", 1, 1, 0, 0,
      "1 bit - EOP" },
    { "bcnt", 9, 2, 0, 0,
      "8 bit - Counter" },
    { "ptr", 28, 10, 0, 0,
      "19 bit - Pointer" },
};
reg_decoder_t tof3_tm_clc_pipe_mem_rspec__csr_memory_clc_clm = { 4, tof3_tm_clc_pipe_mem_rspec__csr_memory_clc_clm_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_pex_pipe_mem_rspec__csr_memory_pex_clm_fld_list[] = {
    { "err", 0, 0, 0, 0,
      "1 bit - Error Indication" },
    { "eop", 1, 1, 0, 0,
      "1 bit - EOP" },
    { "bcnt", 9, 2, 0, 0,
      "8 bit - Counter" },
    { "ptr", 28, 10, 0, 0,
      "19 bit - Pointer" },
};
reg_decoder_t tof3_tm_pex_pipe_mem_rspec__csr_memory_pex_clm = { 4, tof3_tm_pex_pipe_mem_rspec__csr_memory_pex_clm_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_qlc_pipe_mem_rspec__csr_memory_qlc_qlm_fld_list[] = {
    { "ptr", 18, 0, 0, 0,
      "19 bit - Memory address" },
};
reg_decoder_t tof3_tm_qlc_pipe_mem_rspec__csr_memory_qlc_qlm = { 1, tof3_tm_qlc_pipe_mem_rspec__csr_memory_qlc_qlm_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_qlc_pipe_mem_rspec__csr_memory_qlc_ht_fld_list[] = {
    { "vld", 0, 0, 0, 0,
      "1 bit - Valid or Invalid" },
    { "t", 19, 1, 0, 0,
      "19 bit - Threshold" },
    { "h", 38, 20, 0, 0,
      "19 bit - Heuristics" },
};
reg_decoder_t tof3_tm_qlc_pipe_mem_rspec__csr_memory_qlc_ht = { 3, tof3_tm_qlc_pipe_mem_rspec__csr_memory_qlc_ht_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_qlc_pipe_mem_rspec__csr_memory_qlc_vq_fld_list[] = {
    { "base", 7, 0, 0, 0,
      "8 bit  - Base address" },
    { "cap", 9, 8, 0, 0,
      "2 bit - Capacity" },
    { "h", 13, 10, 0, 0,
      "4 bit - heuristics" },
    { "t", 17, 14, 0, 0,
      "4 bit - Threshold" },
};
reg_decoder_t tof3_tm_qlc_pipe_mem_rspec__csr_memory_qlc_vq = { 4, tof3_tm_qlc_pipe_mem_rspec__csr_memory_qlc_vq_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_prc_pipe_mem_rspec__csr_memory_prc_prm_fld_list[] = {
    { "cnt0", 17, 0, 0, 0,
      "18 bit - counter" },
    { "cnt1", 35, 18, 0, 0,
      "18 bit - counter" },
    { "cnt2", 53, 36, 0, 0,
      "18 bit - counter" },
    { "cnt3", 71, 54, 0, 0,
      "18 bit - counter" },
};
reg_decoder_t tof3_tm_prc_pipe_mem_rspec__csr_memory_prc_prm = { 4, tof3_tm_prc_pipe_mem_rspec__csr_memory_prc_prm_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_prc_pipe_mem_rspec__csr_memory_prc_map_fld_list[] = {
    { "map", 63, 0, 0, 0,
      "64 bit - Address" },
};
reg_decoder_t tof3_tm_prc_pipe_mem_rspec__csr_memory_prc_map = { 1, tof3_tm_prc_pipe_mem_rspec__csr_memory_prc_map_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_prc_pipe_mem_rspec__csr_memory_prc_cache0_fld_list[] = {
    { "cache0", 17, 0, 0, 0,
      "18 bit - Cache" },
    { "cache1", 35, 18, 0, 0,
      "18 bit - Cache" },
    { "cache2", 53, 36, 0, 0,
      "18 bit - Cache" },
    { "cache3", 71, 54, 0, 0,
      "18 bit - Cache" },
};
reg_decoder_t tof3_tm_prc_pipe_mem_rspec__csr_memory_prc_cache0 = { 4, tof3_tm_prc_pipe_mem_rspec__csr_memory_prc_cache0_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_prc_pipe_mem_rspec__csr_memory_prc_cache1_fld_list[] = {
    { "cache0", 17, 0, 0, 0,
      "18 bit - Cache" },
    { "cache1", 35, 18, 0, 0,
      "18 bit - Cache" },
    { "cache2", 53, 36, 0, 0,
      "18 bit - Cache" },
    { "cache3", 71, 54, 0, 0,
      "18 bit - Cache" },
};
reg_decoder_t tof3_tm_prc_pipe_mem_rspec__csr_memory_prc_cache1 = { 4, tof3_tm_prc_pipe_mem_rspec__csr_memory_prc_cache1_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_prc_pipe_mem_rspec__csr_memory_prc_tag_fld_list[] = {
    { "tag", 9, 0, 0, 0,
      "10 bit - Tag" },
};
reg_decoder_t tof3_tm_prc_pipe_mem_rspec__csr_memory_prc_tag = { 1, tof3_tm_prc_pipe_mem_rspec__csr_memory_prc_tag_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_pre_pipe_mem_rspec__mit_mem_word_fld_list[] = {
    { "mem_word", 79, 0, 0, 0,
      "80 bit - MGID memory word" },
};
reg_decoder_t tof3_tm_pre_pipe_mem_rspec__mit_mem_word = { 1, tof3_tm_pre_pipe_mem_rspec__mit_mem_word_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_pre_common_mem_rspec__rdm_mem_word_fld_list[] = {
    { "mem_word", 79, 0, 0, 0,
      "80 bit - RDM memory word" },
};
reg_decoder_t tof3_tm_pre_common_mem_rspec__rdm_mem_word = { 1, tof3_tm_pre_common_mem_rspec__rdm_mem_word_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_pre_common_mem_rspec__pbt0_mem_word_fld_list[] = {
    { "mem_word", 8, 0, 0, 0,
      "9 bit - Memory word" },
};
reg_decoder_t tof3_tm_pre_common_mem_rspec__pbt0_mem_word = { 1, tof3_tm_pre_common_mem_rspec__pbt0_mem_word_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_pre_common_mem_rspec__pbt1_mem_word_fld_list[] = {
    { "mem_word", 8, 0, 0, 0,
      "9 bit - Memory word" },
};
reg_decoder_t tof3_tm_pre_common_mem_rspec__pbt1_mem_word = { 1, tof3_tm_pre_common_mem_rspec__pbt1_mem_word_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_pre_common_mem_rspec__lit0_np_mem_word_fld_list[] = {
    { "mem_word", 25, 0, 0, 0,
      "26 bit - Memory word" },
};
reg_decoder_t tof3_tm_pre_common_mem_rspec__lit0_np_mem_word = { 1, tof3_tm_pre_common_mem_rspec__lit0_np_mem_word_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_pre_common_mem_rspec__lit1_np_mem_word_fld_list[] = {
    { "mem_word", 25, 0, 0, 0,
      "26 bit - Memory word" },
};
reg_decoder_t tof3_tm_pre_common_mem_rspec__lit1_np_mem_word = { 1, tof3_tm_pre_common_mem_rspec__lit1_np_mem_word_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_pre_common_mem_rspec__lit0_bm_mem_word0_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof3_tm_pre_common_mem_rspec__lit0_bm_mem_word0 = { 1, tof3_tm_pre_common_mem_rspec__lit0_bm_mem_word0_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_pre_common_mem_rspec__lit0_bm_mem_word1_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof3_tm_pre_common_mem_rspec__lit0_bm_mem_word1 = { 1, tof3_tm_pre_common_mem_rspec__lit0_bm_mem_word1_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_pre_common_mem_rspec__lit0_bm_mem_word2_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof3_tm_pre_common_mem_rspec__lit0_bm_mem_word2 = { 1, tof3_tm_pre_common_mem_rspec__lit0_bm_mem_word2_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_pre_common_mem_rspec__lit0_bm_mem_word3_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof3_tm_pre_common_mem_rspec__lit0_bm_mem_word3 = { 1, tof3_tm_pre_common_mem_rspec__lit0_bm_mem_word3_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_pre_common_mem_rspec__lit0_bm_mem_word4_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof3_tm_pre_common_mem_rspec__lit0_bm_mem_word4 = { 1, tof3_tm_pre_common_mem_rspec__lit0_bm_mem_word4_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_pre_common_mem_rspec__lit0_bm_mem_word5_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof3_tm_pre_common_mem_rspec__lit0_bm_mem_word5 = { 1, tof3_tm_pre_common_mem_rspec__lit0_bm_mem_word5_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_pre_common_mem_rspec__lit0_bm_mem_word6_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof3_tm_pre_common_mem_rspec__lit0_bm_mem_word6 = { 1, tof3_tm_pre_common_mem_rspec__lit0_bm_mem_word6_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_pre_common_mem_rspec__lit0_bm_mem_word7_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof3_tm_pre_common_mem_rspec__lit0_bm_mem_word7 = { 1, tof3_tm_pre_common_mem_rspec__lit0_bm_mem_word7_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_pre_common_mem_rspec__lit1_bm_mem_word0_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof3_tm_pre_common_mem_rspec__lit1_bm_mem_word0 = { 1, tof3_tm_pre_common_mem_rspec__lit1_bm_mem_word0_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_pre_common_mem_rspec__lit1_bm_mem_word1_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof3_tm_pre_common_mem_rspec__lit1_bm_mem_word1 = { 1, tof3_tm_pre_common_mem_rspec__lit1_bm_mem_word1_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_pre_common_mem_rspec__lit1_bm_mem_word2_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof3_tm_pre_common_mem_rspec__lit1_bm_mem_word2 = { 1, tof3_tm_pre_common_mem_rspec__lit1_bm_mem_word2_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_pre_common_mem_rspec__lit1_bm_mem_word3_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof3_tm_pre_common_mem_rspec__lit1_bm_mem_word3 = { 1, tof3_tm_pre_common_mem_rspec__lit1_bm_mem_word3_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_pre_common_mem_rspec__lit1_bm_mem_word4_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof3_tm_pre_common_mem_rspec__lit1_bm_mem_word4 = { 1, tof3_tm_pre_common_mem_rspec__lit1_bm_mem_word4_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_pre_common_mem_rspec__lit1_bm_mem_word5_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof3_tm_pre_common_mem_rspec__lit1_bm_mem_word5 = { 1, tof3_tm_pre_common_mem_rspec__lit1_bm_mem_word5_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_pre_common_mem_rspec__lit1_bm_mem_word6_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof3_tm_pre_common_mem_rspec__lit1_bm_mem_word6 = { 1, tof3_tm_pre_common_mem_rspec__lit1_bm_mem_word6_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_pre_common_mem_rspec__lit1_bm_mem_word7_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof3_tm_pre_common_mem_rspec__lit1_bm_mem_word7 = { 1, tof3_tm_pre_common_mem_rspec__lit1_bm_mem_word7_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_pre_common_mem_rspec__pmt0_mem_word0_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof3_tm_pre_common_mem_rspec__pmt0_mem_word0 = { 1, tof3_tm_pre_common_mem_rspec__pmt0_mem_word0_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_pre_common_mem_rspec__pmt0_mem_word1_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof3_tm_pre_common_mem_rspec__pmt0_mem_word1 = { 1, tof3_tm_pre_common_mem_rspec__pmt0_mem_word1_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_pre_common_mem_rspec__pmt0_mem_word2_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof3_tm_pre_common_mem_rspec__pmt0_mem_word2 = { 1, tof3_tm_pre_common_mem_rspec__pmt0_mem_word2_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_pre_common_mem_rspec__pmt0_mem_word3_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof3_tm_pre_common_mem_rspec__pmt0_mem_word3 = { 1, tof3_tm_pre_common_mem_rspec__pmt0_mem_word3_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_pre_common_mem_rspec__pmt0_mem_word4_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof3_tm_pre_common_mem_rspec__pmt0_mem_word4 = { 1, tof3_tm_pre_common_mem_rspec__pmt0_mem_word4_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_pre_common_mem_rspec__pmt0_mem_word5_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof3_tm_pre_common_mem_rspec__pmt0_mem_word5 = { 1, tof3_tm_pre_common_mem_rspec__pmt0_mem_word5_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_pre_common_mem_rspec__pmt0_mem_word6_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof3_tm_pre_common_mem_rspec__pmt0_mem_word6 = { 1, tof3_tm_pre_common_mem_rspec__pmt0_mem_word6_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_pre_common_mem_rspec__pmt0_mem_word7_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof3_tm_pre_common_mem_rspec__pmt0_mem_word7 = { 1, tof3_tm_pre_common_mem_rspec__pmt0_mem_word7_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_pre_common_mem_rspec__pmt1_mem_word0_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof3_tm_pre_common_mem_rspec__pmt1_mem_word0 = { 1, tof3_tm_pre_common_mem_rspec__pmt1_mem_word0_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_pre_common_mem_rspec__pmt1_mem_word1_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof3_tm_pre_common_mem_rspec__pmt1_mem_word1 = { 1, tof3_tm_pre_common_mem_rspec__pmt1_mem_word1_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_pre_common_mem_rspec__pmt1_mem_word2_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof3_tm_pre_common_mem_rspec__pmt1_mem_word2 = { 1, tof3_tm_pre_common_mem_rspec__pmt1_mem_word2_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_pre_common_mem_rspec__pmt1_mem_word3_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof3_tm_pre_common_mem_rspec__pmt1_mem_word3 = { 1, tof3_tm_pre_common_mem_rspec__pmt1_mem_word3_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_pre_common_mem_rspec__pmt1_mem_word4_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof3_tm_pre_common_mem_rspec__pmt1_mem_word4 = { 1, tof3_tm_pre_common_mem_rspec__pmt1_mem_word4_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_pre_common_mem_rspec__pmt1_mem_word5_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof3_tm_pre_common_mem_rspec__pmt1_mem_word5 = { 1, tof3_tm_pre_common_mem_rspec__pmt1_mem_word5_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_pre_common_mem_rspec__pmt1_mem_word6_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof3_tm_pre_common_mem_rspec__pmt1_mem_word6 = { 1, tof3_tm_pre_common_mem_rspec__pmt1_mem_word6_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_pre_common_mem_rspec__pmt1_mem_word7_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof3_tm_pre_common_mem_rspec__pmt1_mem_word7 = { 1, tof3_tm_pre_common_mem_rspec__pmt1_mem_word7_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_psc_mem_top_rspec__psc_block_grp0_fld_list[] = {
    { "ptr0", 9, 0, 0, 0,
      "10 bit - Cell Pointer" },
    { "ptr1", 19, 10, 0, 0,
      "10 bit - Cell Pointer" },
};
reg_decoder_t tof3_tm_psc_mem_top_rspec__psc_block_grp0 = { 2, tof3_tm_psc_mem_top_rspec__psc_block_grp0_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_psc_mem_top_rspec__psc_block_grp1_fld_list[] = {
    { "ptr0", 9, 0, 0, 0,
      "10 bit - Cell Pointer" },
    { "ptr1", 19, 10, 0, 0,
      "10 bit - Cell Pointer" },
};
reg_decoder_t tof3_tm_psc_mem_top_rspec__psc_block_grp1 = { 2, tof3_tm_psc_mem_top_rspec__psc_block_grp1_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_psc_mem_top_rspec__psc_block_grp2_fld_list[] = {
    { "ptr0", 9, 0, 0, 0,
      "10 bit - Cell Pointer" },
    { "ptr1", 19, 10, 0, 0,
      "10 bit - Cell Pointer" },
};
reg_decoder_t tof3_tm_psc_mem_top_rspec__psc_block_grp2 = { 2, tof3_tm_psc_mem_top_rspec__psc_block_grp2_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_psc_mem_top_rspec__psc_block_grp3_fld_list[] = {
    { "ptr0", 9, 0, 0, 0,
      "10 bit - Cell Pointer" },
    { "ptr1", 19, 10, 0, 0,
      "10 bit - Cell Pointer" },
};
reg_decoder_t tof3_tm_psc_mem_top_rspec__psc_block_grp3 = { 2, tof3_tm_psc_mem_top_rspec__psc_block_grp3_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_psc_mem_top_rspec__psc_block_grp4_fld_list[] = {
    { "ptr0", 9, 0, 0, 0,
      "10 bit - Cell Pointer" },
    { "ptr1", 19, 10, 0, 0,
      "10 bit - Cell Pointer" },
};
reg_decoder_t tof3_tm_psc_mem_top_rspec__psc_block_grp4 = { 2, tof3_tm_psc_mem_top_rspec__psc_block_grp4_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_tm_psc_mem_top_rspec__psc_block_grp5_fld_list[] = {
    { "ptr0", 9, 0, 0, 0,
      "10 bit - Cell Pointer" },
    { "ptr1", 19, 10, 0, 0,
      "10 bit - Cell Pointer" },
};
reg_decoder_t tof3_tm_psc_mem_top_rspec__psc_block_grp5 = { 2, tof3_tm_psc_mem_top_rspec__psc_block_grp5_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_mau_addrmap__dummy_register_fld_list[] = {
    { "dummy_field0", 63, 0, 0, 0,
      "" },
    { "dummy_field1", 127, 64, 0, 0,
      "" },
};
reg_decoder_t tof3_mau_addrmap__dummy_register = { 2, tof3_mau_addrmap__dummy_register_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_ipb_mem_rspec__meta_phase0_16byte_fld_list[] = {
    { "phase0_16byte", 127, 0, 0, 0,
      "128 bit - Meta Data" },
};
reg_decoder_t tof3_ipb_mem_rspec__meta_phase0_16byte = { 1, tof3_ipb_mem_rspec__meta_phase0_16byte_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_ipb_mem_rspec__meta_phase0_8byte_ver_fld_list[] = {
    { "ver", 1, 0, 0, 0,
      "2 bit - Version" },
    { "phase0_8byte", 65, 2, 0, 0,
      "64 bit - Meta Data" },
};
reg_decoder_t tof3_ipb_mem_rspec__meta_phase0_8byte_ver = { 2, tof3_ipb_mem_rspec__meta_phase0_8byte_ver_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_prsr_mem_main_rspec__po_action_row_fld_list[] = {
    { "hdr_len_inc_stop", 0, 0, 0, 0,
      "Stop incrementing the header length count. (Performs any increment specified in the current cycle.)" },
    { "hdr_len_inc", 1, 1, 0, 0,
      "Increment the header length counter by the sum of all shifts since the last increment, including the shift in this cycle." },
    { "val_const_0", 17, 2, 0, 0,
      "16b value constant" },
    { "phv_ext_cnt_16", 22, 18, 0, 0,
      "Number of 16b extractions." },
    { "phv_ext_cnt_8_hi", 27, 23, 0, 0,
      "Number of 8b high extractions." },
    { "phv_ext_cnt_8_lo", 32, 28, 0, 0,
      "Number of 8b low extractions." },
    { "phv_src_0", 38, 33, 0, 0,
      "Packet data offset. [list][*][code]MSB == 0[/code]: source from packet data at this byte offset.[*][code]MSB == 1[/code]: source from the global version/timestamp.[/list]" },
    { "phv_src_1", 44, 39, 0, 0,
      "Packet data offset. [list][*][code]MSB == 0[/code]: source from packet data at this byte offset.[*][code]MSB == 1[/code]: source from the global version/timestamp.[/list]" },
    { "phv_src_2", 50, 45, 0, 0,
      "Packet data offset. [list][*][code]MSB == 0[/code]: source from packet data at this byte offset.[*][code]MSB == 1[/code]: source from the global version/timestamp.[/list]" },
    { "phv_src_3", 56, 51, 0, 0,
      "Packet data offset. [list][*][code]MSB == 0[/code]: source from packet data at this byte offset.[*][code]MSB == 1[/code]: source from the global version/timestamp.[/list]" },
    { "phv_src_4", 62, 57, 0, 0,
      "Packet data offset. [list][*][code]MSB == 0[/code]: source from packet data at this byte offset.[*][code]MSB == 1[/code]: source from the global version/timestamp.[/list]" },
    { "phv_src_5", 68, 63, 0, 0,
      "Packet data offset. [list][*][code]MSB == 0[/code]: source from packet data at this byte offset.[*][code]MSB == 1[/code]: source from the global version/timestamp.[/list]" },
    { "phv_dst_0", 76, 69, 0, 0,
      "Destination PHV container" },
    { "phv_dst_1", 84, 77, 0, 0,
      "Destination PHV container" },
    { "phv_dst_2", 92, 85, 0, 0,
      "Destination PHV container" },
    { "phv_dst_3", 100, 93, 0, 0,
      "Destination PHV container" },
    { "phv_dst_4", 108, 101, 0, 0,
      "Destination PHV container" },
    { "phv_dst_5", 116, 109, 0, 0,
      "Destination PHV container" },
    { "pri_upd_type", 117, 117, 0, 0,
      "Priority update type: [list][*]0: immediate value supplied as source (see cb_mem.pipes.parde.i_prsr_mem.po_action_row.pri_upd_val_mask and cb_mem.pipes.parde.i_prsr_mem.po_action_row.pri_upd_en_shr)[*]1: use packet data as source (see cb_mem.pipes.parde.i_prsr_mem.po_action_row.pri_upd_src, cb_mem.pipes.parde.i_prsr_mem.po_action_row.pri_upd_en_shr and cb_mem.pipes.parde.i_prsr_mem.po_action_row.pri_upd_val_mask)[/list]" },
    { "pri_upd_en_shr", 121, 118, 0, 0,
      "Priority update enable or right-shift amount. Determined by the update type (cb_mem.pipes.parde.i_prsr_mem.po_action_row.pri_upd_type):[list][*]0: Update priority if [code]cb_mem.pipes.parde.i_prsr_mem.po_action_row.pri_upd_en_shr[0] == 1[/code][*]1: Right-shift the 16b value at cb_mem.pipes.parde.i_prsr_mem.po_action_row.pri_upd_src. Priority is the three LSBs of the shifted 16b value after masking[/list]" },
    { "pri_upd_val_mask", 124, 122, 0, 0,
      "Priority value or mask, determined by cb_mem.pipes.parde.i_prsr_mem.po_action_row.pri_upd_type:[list][*]0: immediate value to set as the priority[*]1: mask to apply to the extracted and shifted value[/list]" },
    { "pri_upd_src", 129, 125, 0, 0,
      "Packet data offset. No access to the global version number or timestamp." },
    { "phv_src_6", 135, 130, 0, 0,
      "Packet data offset. [list][*][code]MSB == 0[/code]: source from packet data at this byte offset.[*][code]MSB == 1[/code]: source from the global version/timestamp.[/list]" },
    { "phv_src_7", 141, 136, 0, 0,
      "Packet data offset. [list][*][code]MSB == 0[/code]: source from packet data at this byte offset.[*][code]MSB == 1[/code]: source from the global version/timestamp.[/list]" },
    { "phv_src_8", 147, 142, 0, 0,
      "Packet data offset. [list][*][code]MSB == 0[/code]: source from packet data at this byte offset.[*][code]MSB == 1[/code]: source from the global version/timestamp.[/list]" },
    { "phv_src_9", 153, 148, 0, 0,
      "Packet data offset. [list][*][code]MSB == 0[/code]: source from packet data at this byte offset.[*][code]MSB == 1[/code]: source from the global version/timestamp.[/list]" },
    { "phv_dst_6", 161, 154, 0, 0,
      "Destination PHV container" },
    { "phv_dst_7", 169, 162, 0, 0,
      "Destination PHV container" },
    { "phv_dst_8", 177, 170, 0, 0,
      "Destination PHV container" },
    { "phv_dst_9", 185, 178, 0, 0,
      "Destination PHV container" },
    { "val_const_1", 201, 186, 0, 0,
      "16b value constant" },
    { "clot_type_0", 202, 202, 0, 0,
      "Length source: [list][*]0: length supplied as immediate value[*]1: generate CLOT, extracting length from packet data[/list]" },
    { "clot_len_src_0", 208, 203, 0, 0,
      "CLOT length or length source address, as determined by cb_mem.pipes.parde.i_prsr_mem.po_action_row.clot_type_0: [list][*]0: length - 1 (e.g., a true length of 4 is be encoded as 3); supports true lengths of 1-64B[*]1: 5 LSBs specify the address of the 16b value to extract[/list]" },
    { "clot_en_len_shr_0", 212, 209, 0, 0,
      "CLOT enable or length right-shift amount. Determined by the type (cb_mem.pipes.parde.i_prsr_mem.po_action_row.clot_type_0):[list][*]0: CLOT output enable (bit 0)[*]1: Right-shift the 16b value at cb_mem.pipes.parde.i_prsr_mem.po_action_row.clot_len_src_0. Length is the 6 LSBs of the shifted 16b value after masking[/list]" },
    { "clot_len_mask_0", 218, 213, 0, 0,
      "Mask to apply to extracted length (after right-shift)." },
    { "clot_offset_0", 223, 219, 0, 0,
      "CLOT offset within the current packet data." },
    { "clot_tag_0", 229, 224, 0, 0,
      "Tag value for the CLOT" },
    { "clot_has_csum_0", 230, 230, 0, 0,
      "The CLOT has a checksum (wait for output from the checksum engines)." },
    { "clot_len_add_0", 236, 231, 0, 0,
      "Amount to [italic]add[/italic] to the length of the first CLOT. Only the first CLOT supports the add; the second does not.[p]The addition operation is unsigned and overflow is ignored. Subtraction is peformed by treating this field as a two's complement value. For example, subtracting 1 is achieved by setting this field to [code]6'b111111[/code] (the 6b two's complement value for -1). This results in 63 being added, but because overflow is ignored, the final result is the same as subtracting 1." },
    { "csum_addr_0", 241, 237, 0, 0,
      "" },
    { "csum_addr_1", 246, 242, 0, 0,
      "" },
    { "csum_addr_2", 251, 247, 0, 0,
      "" },
    { "csum_en_0", 252, 252, 0, 0,
      "" },
    { "csum_en_1", 253, 253, 0, 0,
      "" },
    { "csum_en_2", 254, 254, 0, 0,
      "" },
    { "disable_partial_hdr_err", 255, 255, 0, 0,
      "Disable partial header error reporting during [italic]this[/italic] cycle." },
    { "partial_hdr_err_proc", 256, 256, 0, 0,
      "When a partial header error occurs, and cb_mem.pipes.parde.i_prsr_mem.po_action_row.disable_partial_hdr_err is set, perform as much processing (extractions, CLOT generation, etc) as possible. Without this, no extractions or related processing is performed." },
    { "phv_src_10", 262, 257, 0, 0,
      "Packet data offset. [list][*][code]MSB == 0[/code]: source from packet data at this byte offset.[*][code]MSB == 1[/code]: source from the global version/timestamp.[/list]" },
    { "phv_src_11", 268, 263, 0, 0,
      "Packet data offset. [list][*][code]MSB == 0[/code]: source from packet data at this byte offset.[*][code]MSB == 1[/code]: source from the global version/timestamp.[/list]" },
    { "phv_src_12", 274, 269, 0, 0,
      "Packet data offset. [list][*][code]MSB == 0[/code]: source from packet data at this byte offset.[*][code]MSB == 1[/code]: source from the global version/timestamp.[/list]" },
    { "phv_src_13", 280, 275, 0, 0,
      "Packet data offset. [list][*][code]MSB == 0[/code]: source from packet data at this byte offset.[*][code]MSB == 1[/code]: source from the global version/timestamp.[/list]" },
    { "phv_src_14", 286, 281, 0, 0,
      "Packet data offset. [list][*][code]MSB == 0[/code]: source from packet data at this byte offset.[*][code]MSB == 1[/code]: source from the global version/timestamp.[/list]" },
    { "phv_src_15", 292, 287, 0, 0,
      "Packet data offset. [list][*][code]MSB == 0[/code]: source from packet data at this byte offset.[*][code]MSB == 1[/code]: source from the global version/timestamp.[/list]" },
    { "phv_dst_10", 300, 293, 0, 0,
      "Destination PHV container" },
    { "phv_dst_11", 308, 301, 0, 0,
      "Destination PHV container" },
    { "phv_dst_12", 316, 309, 0, 0,
      "Destination PHV container" },
    { "phv_dst_13", 324, 317, 0, 0,
      "Destination PHV container" },
    { "phv_dst_14", 332, 325, 0, 0,
      "Destination PHV container" },
    { "phv_dst_15", 340, 333, 0, 0,
      "Destination PHV container" },
    { "csum_addr_3", 345, 341, 0, 0,
      "" },
    { "csum_addr_4", 350, 346, 0, 0,
      "" },
    { "csum_en_3", 351, 351, 0, 0,
      "" },
    { "csum_en_4", 352, 352, 0, 0,
      "" },
    { "dst_offset_inc", 357, 353, 0, 0,
      "Increment the offset by this amount or, if cb_mem.pipes.parde.i_prsr_mem.po_action_row.dst_offset_rst is asserted, set the offset to this amount.The offset is updated [italic]after[/italic] the current offset is added to the destination addresses in this action entry." },
    { "dst_offset_rst", 358, 358, 0, 0,
      "Reset the destination offset before adding the value contained in cb_mem.pipes.parde.i_prsr_mem.po_action_row.dst_offset_rst.The offset is updated [italic]after[/italic] the current offset is added to the destination addresses in this action entry." },
    { "phv_offset_add_dst_0", 359, 359, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "phv_offset_add_dst_1", 360, 360, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "phv_offset_add_dst_2", 361, 361, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "phv_offset_add_dst_3", 362, 362, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "phv_offset_add_dst_4", 363, 363, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "phv_offset_add_dst_5", 364, 364, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "phv_offset_add_dst_6", 365, 365, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "phv_offset_add_dst_7", 366, 366, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "phv_offset_add_dst_8", 367, 367, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "phv_offset_add_dst_9", 368, 368, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "phv_offset_add_dst_10", 369, 369, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "phv_offset_add_dst_11", 370, 370, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "phv_offset_add_dst_12", 371, 371, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "phv_offset_add_dst_13", 372, 372, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "phv_offset_add_dst_14", 373, 373, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "phv_offset_add_dst_15", 374, 374, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "clot_tag_offset_add_0", 375, 375, 0, 0,
      "Add the current offset to the CLOT tag." },
    { "val_const_rot[0]", 376, 376, 0, 0,
      "Right-rotate the value constant by offset before PHV extraction." },
    { "val_const_rot[1]", 377, 377, 0, 0,
      "Right-rotate the value constant by offset before PHV extraction." },
    { "val_const_32b_bond", 378, 378, 0, 0,
      "Treat the two 16b value constants as a single 32b value when performing a constant rotate." },
    { "phv_src_16", 384, 379, 0, 0,
      "Packet data offset. [list][*][code]MSB == 0[/code]: source from packet data at this byte offset.[*][code]MSB == 1[/code]: source from the global version/timestamp.[/list]" },
    { "phv_src_17", 390, 385, 0, 0,
      "Packet data offset. [list][*][code]MSB == 0[/code]: source from packet data at this byte offset.[*][code]MSB == 1[/code]: source from the global version/timestamp.[/list]" },
    { "phv_src_18", 396, 391, 0, 0,
      "Packet data offset. [list][*][code]MSB == 0[/code]: source from packet data at this byte offset.[*][code]MSB == 1[/code]: source from the global version/timestamp.[/list]" },
    { "phv_src_19", 402, 397, 0, 0,
      "Packet data offset. [list][*][code]MSB == 0[/code]: source from packet data at this byte offset.[*][code]MSB == 1[/code]: source from the global version/timestamp.[/list]" },
    { "phv_dst_16", 410, 403, 0, 0,
      "Destination PHV container" },
    { "phv_dst_17", 418, 411, 0, 0,
      "Destination PHV container" },
    { "phv_dst_18", 426, 419, 0, 0,
      "Destination PHV container" },
    { "phv_dst_19", 434, 427, 0, 0,
      "Destination PHV container" },
    { "clot_type_1", 435, 435, 0, 0,
      "Length source: [list][*]0: length supplied as immediate value[*]1: generate CLOT, extracting length from packet data[/list]" },
    { "clot_len_src_1", 441, 436, 0, 0,
      "CLOT length or length source address, as determined by cb_mem.pipes.parde.i_prsr_mem.po_action_row.clot_type_0: [list][*]0: length - 1 (e.g., a true length of 4 is be encoded as 3); supports true lengths of 1-64B[*]1: 5 LSBs specify the address of the 16b value to extract[/list]" },
    { "clot_en_len_shr_1", 445, 442, 0, 0,
      "CLOT enable or length right-shift amount. Determined by the type (cb_mem.pipes.parde.i_prsr_mem.po_action_row.clot_type_0):[list][*]0: CLOT output enable (bit 0)[*]1: Right-shift the 16b value at cb_mem.pipes.parde.i_prsr_mem.po_action_row.clot_len_src_0. Length is the 6 LSBs of the shifted 16b value after masking[/list]" },
    { "clot_len_mask_1", 451, 446, 0, 0,
      "Mask to apply to extracted length (after right-shift)." },
    { "clot_offset_1", 456, 452, 0, 0,
      "CLOT offset within the current packet data." },
    { "clot_tag_1", 462, 457, 0, 0,
      "Tag value for the CLOT" },
    { "clot_has_csum_1", 463, 463, 0, 0,
      "The CLOT has a checksum (wait for output from the checksum engines)." },
    { "phv_offset_add_dst_16", 464, 464, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "phv_offset_add_dst_17", 465, 465, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "phv_offset_add_dst_18", 466, 466, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "phv_offset_add_dst_19", 467, 467, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "clot_tag_offset_add_1", 468, 468, 0, 0,
      "Add the current offset to the CLOT tag." },
    { "ver_upd_type", 469, 469, 0, 0,
      "Version update type: [list][*]0: immediate value supplied as source (see cb_mem.pipes.parde.i_prsr_mem.po_action_row.ver_upd_val_mask and cb_mem.pipes.parde.i_prsr_mem.po_action_row.ver_upd_en_shr)[*]1: use packet data as source (see cb_mem.pipes.parde.i_prsr_mem.po_action_row.ver_upd_src, cb_mem.pipes.parde.i_prsr_mem.po_action_row.ver_upd_en_shr and cb_mem.pipes.parde.i_prsr_mem.po_action_row.ver_upd_val_mask)[/list]" },
    { "ver_upd_en_shr", 473, 470, 0, 0,
      "Version update enable or right-shift amount. Determined by the update type:[list][*]0: Version update (determined by LSB)[*]1: Right-shift the 16b value at cb_mem.pipes.parde.i_prsr_mem.po_action_row.ver_upd_src. Version is the two LSBs of the shifted 16b value after masking[/list]" },
    { "ver_upd_val_mask", 475, 474, 0, 0,
      "Version value or mask, determined by cb_mem.pipes.parde.i_prsr_mem.po_action_row.ver_upd_type:[list][*]0: immediate value to set as the priority[*]1: mask to apply to the extracted and shifted value[/list]" },
    { "ver_upd_src", 480, 476, 0, 0,
      "Packet data offset. No access to the global version number or timestamp." },
};
reg_decoder_t tof3_prsr_mem_main_rspec__po_action_row = { 109, tof3_prsr_mem_main_rspec__po_action_row_fld_list, 512 /* bits */, 0 };

reg_decoder_fld_t tof3_prsr_mem_main_rspec__ml_tcam_row_fld_list[] = {
    { "w0_lookup_8[0]", 7, 0, 0, 0,
      "" },
    { "w0_lookup_8[1]", 15, 8, 0, 0,
      "" },
    { "w0_lookup_8[2]", 23, 16, 0, 0,
      "" },
    { "w0_lookup_8[3]", 31, 24, 0, 0,
      "" },
    { "w0_curr_state", 39, 32, 0, 0,
      "" },
    { "w0_ctr_zero", 40, 40, 0, 0,
      "[code]counter == 0[/code]" },
    { "w0_ctr_neg", 41, 41, 0, 0,
      "[code]counter < 0[/code]" },
    { "w0_ver_0", 42, 42, 0, 0,
      "[code]version[0][/code]" },
    { "w0_ver_1", 43, 43, 0, 0,
      "[code]version[1][/code]" },
    { "w1_lookup_8[0]", 71, 64, 0, 0,
      "" },
    { "w1_lookup_8[1]", 79, 72, 0, 0,
      "" },
    { "w1_lookup_8[2]", 87, 80, 0, 0,
      "" },
    { "w1_lookup_8[3]", 95, 88, 0, 0,
      "" },
    { "w1_curr_state", 103, 96, 0, 0,
      "" },
    { "w1_ctr_zero", 104, 104, 0, 0,
      "[code]counter == 0[/code]" },
    { "w1_ctr_neg", 105, 105, 0, 0,
      "[code]counter < 0[/code]" },
    { "w1_ver_0", 106, 106, 0, 0,
      "[code]version[0][/code]" },
    { "w1_ver_1", 107, 107, 0, 0,
      "[code]version[1][/code]" },
};
reg_decoder_t tof3_prsr_mem_main_rspec__ml_tcam_row = { 18, tof3_prsr_mem_main_rspec__ml_tcam_row_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_prsr_mem_main_rspec__ml_ea_row_fld_list[] = {
    { "nxt_state", 7, 0, 0, 0,
      "" },
    { "nxt_state_mask", 15, 8, 0, 0,
      "Mask to apply to current state/next state:[list][*]0 = keep existing value[*]1 = use new value[/list][code]state = (nxt_state & nxt_state_mask) | (curr_state & ~nxt_state_mask)[/code]" },
    { "lookup_offset_8[0]", 21, 16, 0, 0,
      "Offset from byte 0 the beginning of the 32B packet data input. If [code]MSB == 1[/code] then source from the additional static bytes." },
    { "lookup_offset_8[1]", 27, 22, 0, 0,
      "Offset from byte 0 the beginning of the 32B packet data input. If [code]MSB == 1[/code] then source from the additional static bytes." },
    { "lookup_offset_8[2]", 33, 28, 0, 0,
      "Offset from byte 0 the beginning of the 32B packet data input. If [code]MSB == 1[/code] then source from the additional static bytes." },
    { "lookup_offset_8[3]", 39, 34, 0, 0,
      "Offset from byte 0 the beginning of the 32B packet data input. If [code]MSB == 1[/code] then source from the additional static bytes." },
    { "ld_lookup_8[0]", 40, 40, 0, 0,
      "" },
    { "ld_lookup_8[1]", 41, 41, 0, 0,
      "" },
    { "ld_lookup_8[2]", 42, 42, 0, 0,
      "" },
    { "ld_lookup_8[3]", 43, 43, 0, 0,
      "" },
    { "sv_lookup_8[0]", 44, 44, 0, 0,
      "" },
    { "sv_lookup_8[1]", 45, 45, 0, 0,
      "" },
    { "sv_lookup_8[2]", 46, 46, 0, 0,
      "" },
    { "sv_lookup_8[3]", 47, 47, 0, 0,
      "" },
    { "shift_amt", 53, 48, 0, 0,
      "Shift the packet data input by this amount" },
    { "buf_req", 59, 54, 0, 0,
      "Buffer occuapancy requirement for this entry to match. A buffer occupancy less than this will generate an error if the end of packet is seen, or will stall when the end of packet is not seen." },
    { "done", 60, 60, 0, 0,
      "This is the last header to be processed. Advance to the next packet.Note: cb_mem.pipes.parde.i_prsr_mem.ml_ea_row.shift_amt must specify the number of header bytes in the current word to identify the payload location." },
    { "ctr_op", 64, 61, 0, 0,
      "Counter operation to perform:[list][*]0: Load with the immediate value from the cb_mem.pipes.parde.i_prsr_mem.ml_ea_row.ctr_amt_idx field.[*]1: Pop the stack and load with the immediate value from the cb_mem.pipes.parde.i_prsr_mem.ml_ea_row.ctr_amt_idx field.[*]2: Push the stack and load with the immediate value from the cb_mem.pipes.parde.i_prsr_mem.ml_ea_row.ctr_amt_idx field.[*]3: Add cb_mem.pipes.parde.i_prsr_mem.ml_ea_row.ctr_amt_idx to the current top-of-stack value.[*]4: Load via CounterRAM[cb_mem.pipes.parde.i_prsr_mem.ml_ea_row.ctr_amt_idx].[*]5: Pop the stack and load counter via CounterRAM[cb_mem.pipes.parde.i_prsr_mem.ml_ea_row.ctr_amt_idx].[*]6: Push the stack and load counter via CounterRAM[cb_mem.pipes.parde.i_prsr_mem.ml_ea_row.ctr_amt_idx].[*]7: Pop the stack and add cb_mem.pipes.parde.i_prsr_mem.ml_ea_row.ctr_amt_idx.[*]8: Load from packet data.[*]9: Pop the stack and load from packet data.[*]10: Push the stack and load from packet data.[*]12: Load from packet data and shift.[*]13: Pop the stack and load from packet data and shift.[*]14: Push the stack and load from packet data and shift.[/list]For the three load from packet operations (8, 9, and 10), the 5 MSBs of cb_mem.pipes.parde.i_prsr_mem.ml_ea_row.ctr_amt_idx specify the byte offset to load. This value is rotated by the value in the 3 LSBs of cb_mem.pipes.parde.i_prsr_mem.ml_ea_row.ctr_amt_idx, and is masked via cb_mem.pipes.parde.i_prsr_mem.ml_ea_row.lookup_offset_8[code][3][/code]: the 3 MSBs specify an amount from the left to mask, and the 3 LSBs specify an amount from the right to mask.[p]For the three load from packet and shift operations (12, 13, and 14), the 5 MSBs of cb_mem.pipes.parde.i_prsr_mem.ml_ea_row.ctr_amt_idx specify the byte offset to load. This value is rotated by the value in the 3 LSBs of cb_mem.pipes.parde.i_prsr_mem.ml_ea_row.ctr_amt_idx, and is masked via cb_mem.pipes.parde.i_prsr_mem.ml_ea_row.shift_amt: the 3 MSBs specify an amount from the left to mask, and the 3 LSBs specify an amount from the right to mask." },
    { "ctr_amt_idx", 72, 65, 0, 0,
      "Counter increment or load value:[list][*]cb_mem.pipes.parde.i_prsr_mem.ml_ea_row.ctr_op = 0, 1 or 2: Immediate value to load into the counter.[*]cb_mem.pipes.parde.i_prsr_mem.ml_ea_row.ctr_op = 3 or 7: Immediate value to increment the counter by.[*]cb_mem.pipes.parde.i_prsr_mem.ml_ea_row.ctr_op = 4, 5, or 6: Counter initialization RAM entry to load (specified in 4 LSBs; 4 MSBs ignored).[*]cb_mem.pipes.parde.i_prsr_mem.ml_ea_row.ctr_op = 8, 9, 10, 12, 13, or 14: 5 MSBs specify byte offset within the packet data; 3 LSBs specify a rotate amount..[/list]" },
    { "ctr_stack_upd_w_top", 73, 73, 0, 0,
      "Update the pushed value when the top-of-stack value is updated. Valid only when a value is being pushed onto the stack." },
    { "action_ram_en", 75, 74, 0, 0,
      "Action RAM enable:[list][*]2'b00: activate bank 0.[*]2'b01: activate banks 0-1.[*]2'b10: activate banks 0-2.[*]2'b11: activate banks 0-3.[/list]" },
};
reg_decoder_t tof3_prsr_mem_main_rspec__ml_ea_row = { 21, tof3_prsr_mem_main_rspec__ml_ea_row_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_prsr_mem_main_rspec__ml_ctr_init_ram_fld_list[] = {
    { "add", 7, 0, 0, 0,
      "Value to add to the rotated & masked value" },
    { "mask_8", 15, 8, 0, 0,
      "Mask the rotate value: [code]rot_val & mask[/code]" },
    { "rotate", 18, 16, 0, 0,
      "Amount to right rotate the source value" },
    { "max", 26, 19, 0, 0,
      "Maximum permitted value in source" },
    { "src", 28, 27, 0, 0,
      "TCAM lookup input to use as the counter source" },
    { "add_to_stack", 29, 29, 0, 0,
      "Add the cb_mem.pipes.parde.i_prsr_mem.ml_ctr_init_ram.add value to the elements in the stack (for entries with [code]update_with_top[/code] set appropriately)." },
};
reg_decoder_t tof3_prsr_mem_main_rspec__ml_ctr_init_ram = { 6, tof3_prsr_mem_main_rspec__ml_ctr_init_ram_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_prsr_mem_main_rspec__po_csum_ctrl_0_row_fld_list[] = {
    { "add", 15, 0, 0, 0,
      "16b constant to add to the checksum calculation" },
    { "swap[0]", 16, 16, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[1]", 17, 17, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[2]", 18, 18, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[3]", 19, 19, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[4]", 20, 20, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[5]", 21, 21, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[6]", 22, 22, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[7]", 23, 23, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[8]", 24, 24, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[9]", 25, 25, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[10]", 26, 26, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[11]", 27, 27, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[12]", 28, 28, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[13]", 29, 29, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[14]", 30, 30, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[15]", 31, 31, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[16]", 32, 32, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "shr", 33, 33, 0, 0,
      "Rotate the 32B packet word right by one prior to processing.Note: treat 32B input word as a left-aligned 34B data word." },
    { "mask[0]", 34, 34, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[1]", 35, 35, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[2]", 36, 36, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[3]", 37, 37, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[4]", 38, 38, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[5]", 39, 39, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[6]", 40, 40, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[7]", 41, 41, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[8]", 42, 42, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[9]", 43, 43, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[10]", 44, 44, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[11]", 45, 45, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[12]", 46, 46, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[13]", 47, 47, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[14]", 48, 48, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[15]", 49, 49, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[16]", 50, 50, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[17]", 51, 51, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[18]", 52, 52, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[19]", 53, 53, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[20]", 54, 54, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[21]", 55, 55, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[22]", 56, 56, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[23]", 57, 57, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[24]", 58, 58, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[25]", 59, 59, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[26]", 60, 60, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[27]", 61, 61, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[28]", 62, 62, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[29]", 63, 63, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[30]", 64, 64, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[31]", 65, 65, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "dst_bit_hdr_end_pos", 70, 66, 0, 0,
      "[list][*]VERIFY: indicates the bit within the destination PHV to record the validity of the checksum.Note: [code]target_bit_position = bit_position % container_size[/code].[*]RESIDUAL: indicates where the header ends in the current packet word. Any headers after the current header are automatically subtracted from the residual.[*]CLOT: Unused.[/list]Valid only when cb_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_0_row.hdr_end is asserted." },
    { "dst", 78, 71, 0, 0,
      "Destination PHV field or tag.Valid only when cb_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_0_row.hdr_end is asserted." },
    { "hdr_end", 79, 79, 0, 0,
      "The header containing the checksum ends in this packet word.[list][*]0: Additional words to come[*]1: Final word of header. For VERIFY checksums, verify that the checksum is correct by comparing to zero. For RESIDUAL checksums, automatically subtract any headers that follow.[/list]" },
    { "type", 81, 80, 0, 0,
      "Checksum calculation type:[list][*]0: Verification -- Verify that a checksum is valid. A final sum of zero indicates a valid checksum.[*]1: Residual -- Calculate the residual of the checksum, i.e., the contribution of [italic]non-header[/italic] bytes of the packet.[*]2: CLOT -- Calculate the checksum for CLOT data. Only valid for checksum units 2-4; checksum output ignored when used for units 0 and 1.[/list]Sampled when cb_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_0_row.start == 1." },
    { "start", 82, 82, 0, 0,
      "Start a new checksum calculation and clear any previously calculated value" },
    { "zeros_as_ones", 83, 83, 0, 0,
      "The current data word contains a checksum; an all-zeros checksum is encoded as all-ones. This is used by certain protocols, such as UDP, which use all zeros to indicate the checksum is not used. Indicates that an all-ones value at cb_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_0_row.zeros_as_ones_pos should be converted to all-zeros." },
    { "zeros_as_ones_pos", 88, 84, 0, 0,
      "The position of the checksum within the data word. Only necessary when zeros-as-ones is in use." },
    { "mul_2[0]", 89, 89, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[1]", 90, 90, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[2]", 91, 91, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[3]", 92, 92, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[4]", 93, 93, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[5]", 94, 94, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[6]", 95, 95, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[7]", 96, 96, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[8]", 97, 97, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[9]", 98, 98, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[10]", 99, 99, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[11]", 100, 100, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[12]", 101, 101, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[13]", 102, 102, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[14]", 103, 103, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[15]", 104, 104, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[16]", 105, 105, 0, 0,
      "Multiply each 16b word by two" },
};
reg_decoder_t tof3_prsr_mem_main_rspec__po_csum_ctrl_0_row = { 75, tof3_prsr_mem_main_rspec__po_csum_ctrl_0_row_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_prsr_mem_main_rspec__po_csum_ctrl_1_row_fld_list[] = {
    { "add", 15, 0, 0, 0,
      "16b constant to add to the checksum calculation" },
    { "swap[0]", 16, 16, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[1]", 17, 17, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[2]", 18, 18, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[3]", 19, 19, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[4]", 20, 20, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[5]", 21, 21, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[6]", 22, 22, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[7]", 23, 23, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[8]", 24, 24, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[9]", 25, 25, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[10]", 26, 26, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[11]", 27, 27, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[12]", 28, 28, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[13]", 29, 29, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[14]", 30, 30, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[15]", 31, 31, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[16]", 32, 32, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "shr", 33, 33, 0, 0,
      "Rotate the 32B packet word right by one prior to processing.Note: treat 32B input word as a left-aligned 34B data word." },
    { "mask[0]", 34, 34, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[1]", 35, 35, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[2]", 36, 36, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[3]", 37, 37, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[4]", 38, 38, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[5]", 39, 39, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[6]", 40, 40, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[7]", 41, 41, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[8]", 42, 42, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[9]", 43, 43, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[10]", 44, 44, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[11]", 45, 45, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[12]", 46, 46, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[13]", 47, 47, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[14]", 48, 48, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[15]", 49, 49, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[16]", 50, 50, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[17]", 51, 51, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[18]", 52, 52, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[19]", 53, 53, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[20]", 54, 54, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[21]", 55, 55, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[22]", 56, 56, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[23]", 57, 57, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[24]", 58, 58, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[25]", 59, 59, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[26]", 60, 60, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[27]", 61, 61, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[28]", 62, 62, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[29]", 63, 63, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[30]", 64, 64, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[31]", 65, 65, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "dst_bit_hdr_end_pos", 70, 66, 0, 0,
      "[list][*]VERIFY: indicates the bit within the destination PHV to record the validity of the checksum.Note: [code]target_bit_position = bit_position % container_size[/code].[*]RESIDUAL: indicates where the header ends in the current packet word. Any headers after the current header are automatically subtracted from the residual.[*]CLOT: Unused.[/list]Valid only when cb_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_1_row.hdr_end is asserted." },
    { "dst", 78, 71, 0, 0,
      "Destination PHV field or tag.Valid only when cb_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_1_row.hdr_end is asserted." },
    { "hdr_end", 79, 79, 0, 0,
      "The header containing the checksum ends in this packet word.[list][*]0: Additional words to come[*]1: Final word of header. For VERIFY checksums, verify that the checksum is correct by comparing to zero. For RESIDUAL checksums, automatically subtract any headers that follow.[/list]" },
    { "type", 81, 80, 0, 0,
      "Checksum calculation type:[list][*]0: Verification -- Verify that a checksum is valid. A final sum of zero indicates a valid checksum.[*]1: Residual -- Calculate the residual of the checksum, i.e., the contribution of [italic]non-header[/italic] bytes of the packet.[*]2: CLOT -- Calculate the checksum for CLOT data. Only valid for checksum units 2-4; checksum output ignored when used for units 0 and 1.[/list]Sampled when cb_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_1_row.start == 1." },
    { "start", 82, 82, 0, 0,
      "Start a new checksum calculation and clear any previously calculated value" },
    { "zeros_as_ones", 83, 83, 0, 0,
      "The current data word contains a checksum; an all-zeros checksum is encoded as all-ones. This is used by certain protocols, such as UDP, which use all zeros to indicate the checksum is not used. Indicates that an all-ones value at cb_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_1_row.zeros_as_ones_pos should be converted to all-zeros." },
    { "zeros_as_ones_pos", 88, 84, 0, 0,
      "The position of the checksum within the data word. Only necessary when zeros-as-ones is in use." },
    { "mul_2[0]", 89, 89, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[1]", 90, 90, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[2]", 91, 91, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[3]", 92, 92, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[4]", 93, 93, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[5]", 94, 94, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[6]", 95, 95, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[7]", 96, 96, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[8]", 97, 97, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[9]", 98, 98, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[10]", 99, 99, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[11]", 100, 100, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[12]", 101, 101, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[13]", 102, 102, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[14]", 103, 103, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[15]", 104, 104, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[16]", 105, 105, 0, 0,
      "Multiply each 16b word by two" },
};
reg_decoder_t tof3_prsr_mem_main_rspec__po_csum_ctrl_1_row = { 75, tof3_prsr_mem_main_rspec__po_csum_ctrl_1_row_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_prsr_mem_main_rspec__po_csum_ctrl_2_row_fld_list[] = {
    { "add", 15, 0, 0, 0,
      "16b constant to add to the checksum calculation" },
    { "swap[0]", 16, 16, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[1]", 17, 17, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[2]", 18, 18, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[3]", 19, 19, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[4]", 20, 20, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[5]", 21, 21, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[6]", 22, 22, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[7]", 23, 23, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[8]", 24, 24, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[9]", 25, 25, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[10]", 26, 26, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[11]", 27, 27, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[12]", 28, 28, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[13]", 29, 29, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[14]", 30, 30, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[15]", 31, 31, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[16]", 32, 32, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "shr", 33, 33, 0, 0,
      "Rotate the 32B packet word right by one prior to processing.Note: treat 32B input word as a left-aligned 34B data word." },
    { "mask[0]", 34, 34, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[1]", 35, 35, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[2]", 36, 36, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[3]", 37, 37, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[4]", 38, 38, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[5]", 39, 39, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[6]", 40, 40, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[7]", 41, 41, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[8]", 42, 42, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[9]", 43, 43, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[10]", 44, 44, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[11]", 45, 45, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[12]", 46, 46, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[13]", 47, 47, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[14]", 48, 48, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[15]", 49, 49, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[16]", 50, 50, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[17]", 51, 51, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[18]", 52, 52, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[19]", 53, 53, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[20]", 54, 54, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[21]", 55, 55, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[22]", 56, 56, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[23]", 57, 57, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[24]", 58, 58, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[25]", 59, 59, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[26]", 60, 60, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[27]", 61, 61, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[28]", 62, 62, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[29]", 63, 63, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[30]", 64, 64, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[31]", 65, 65, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "dst_bit_hdr_end_pos", 70, 66, 0, 0,
      "[list][*]VERIFY: indicates the bit within the destination PHV to record the validity of the checksum.Note: [code]target_bit_position = bit_position % container_size[/code].[*]RESIDUAL: indicates where the header ends in the current packet word. Any headers after the current header are automatically subtracted from the residual.[*]CLOT: Unused.[/list]Valid only when cb_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_2_row.hdr_end is asserted." },
    { "dst", 78, 71, 0, 0,
      "Destination PHV field or tag.Valid only when cb_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_2_row.hdr_end is asserted." },
    { "hdr_end", 79, 79, 0, 0,
      "The header containing the checksum ends in this packet word.[list][*]0: Additional words to come[*]1: Final word of header. For VERIFY checksums, verify that the checksum is correct by comparing to zero. For RESIDUAL checksums, automatically subtract any headers that follow.[/list]" },
    { "type", 81, 80, 0, 0,
      "Checksum calculation type:[list][*]0: Verification -- Verify that a checksum is valid. A final sum of zero indicates a valid checksum.[*]1: Residual -- Calculate the residual of the checksum, i.e., the contribution of [italic]non-header[/italic] bytes of the packet.[*]2: CLOT -- Calculate the checksum for CLOT data. Only valid for checksum units 2-4; checksum output ignored when used for units 0 and 1.[/list]Sampled when cb_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_2_row.start == 1." },
    { "start", 82, 82, 0, 0,
      "Start a new checksum calculation and clear any previously calculated value" },
    { "zeros_as_ones", 83, 83, 0, 0,
      "The current data word contains a checksum; an all-zeros checksum is encoded as all-ones. This is used by certain protocols, such as UDP, which use all zeros to indicate the checksum is not used. Indicates that an all-ones value at cb_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_2_row.zeros_as_ones_pos should be converted to all-zeros." },
    { "zeros_as_ones_pos", 88, 84, 0, 0,
      "The position of the checksum within the data word. Only necessary when zeros-as-ones is in use." },
    { "mul_2[0]", 89, 89, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[1]", 90, 90, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[2]", 91, 91, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[3]", 92, 92, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[4]", 93, 93, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[5]", 94, 94, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[6]", 95, 95, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[7]", 96, 96, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[8]", 97, 97, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[9]", 98, 98, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[10]", 99, 99, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[11]", 100, 100, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[12]", 101, 101, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[13]", 102, 102, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[14]", 103, 103, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[15]", 104, 104, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[16]", 105, 105, 0, 0,
      "Multiply each 16b word by two" },
};
reg_decoder_t tof3_prsr_mem_main_rspec__po_csum_ctrl_2_row = { 75, tof3_prsr_mem_main_rspec__po_csum_ctrl_2_row_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_prsr_mem_main_rspec__po_csum_ctrl_3_row_fld_list[] = {
    { "add", 15, 0, 0, 0,
      "16b constant to add to the checksum calculation" },
    { "swap[0]", 16, 16, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[1]", 17, 17, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[2]", 18, 18, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[3]", 19, 19, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[4]", 20, 20, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[5]", 21, 21, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[6]", 22, 22, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[7]", 23, 23, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[8]", 24, 24, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[9]", 25, 25, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[10]", 26, 26, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[11]", 27, 27, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[12]", 28, 28, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[13]", 29, 29, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[14]", 30, 30, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[15]", 31, 31, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[16]", 32, 32, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "shr", 33, 33, 0, 0,
      "Rotate the 32B packet word right by one prior to processing.Note: treat 32B input word as a left-aligned 34B data word." },
    { "mask[0]", 34, 34, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[1]", 35, 35, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[2]", 36, 36, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[3]", 37, 37, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[4]", 38, 38, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[5]", 39, 39, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[6]", 40, 40, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[7]", 41, 41, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[8]", 42, 42, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[9]", 43, 43, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[10]", 44, 44, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[11]", 45, 45, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[12]", 46, 46, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[13]", 47, 47, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[14]", 48, 48, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[15]", 49, 49, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[16]", 50, 50, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[17]", 51, 51, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[18]", 52, 52, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[19]", 53, 53, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[20]", 54, 54, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[21]", 55, 55, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[22]", 56, 56, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[23]", 57, 57, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[24]", 58, 58, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[25]", 59, 59, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[26]", 60, 60, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[27]", 61, 61, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[28]", 62, 62, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[29]", 63, 63, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[30]", 64, 64, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[31]", 65, 65, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "dst_bit_hdr_end_pos", 70, 66, 0, 0,
      "[list][*]VERIFY: indicates the bit within the destination PHV to record the validity of the checksum.Note: [code]target_bit_position = bit_position % container_size[/code].[*]RESIDUAL: indicates where the header ends in the current packet word. Any headers after the current header are automatically subtracted from the residual.[*]CLOT: Unused.[/list]Valid only when cb_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_3_row.hdr_end is asserted." },
    { "dst", 78, 71, 0, 0,
      "Destination PHV field or tag.Valid only when cb_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_3_row.hdr_end is asserted." },
    { "hdr_end", 79, 79, 0, 0,
      "The header containing the checksum ends in this packet word.[list][*]0: Additional words to come[*]1: Final word of header. For VERIFY checksums, verify that the checksum is correct by comparing to zero. For RESIDUAL checksums, automatically subtract any headers that follow.[/list]" },
    { "type", 81, 80, 0, 0,
      "Checksum calculation type:[list][*]0: Verification -- Verify that a checksum is valid. A final sum of zero indicates a valid checksum.[*]1: Residual -- Calculate the residual of the checksum, i.e., the contribution of [italic]non-header[/italic] bytes of the packet.[*]2: CLOT -- Calculate the checksum for CLOT data. Only valid for checksum units 2-4; checksum output ignored when used for units 0 and 1.[/list]Sampled when cb_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_3_row.start == 1." },
    { "start", 82, 82, 0, 0,
      "Start a new checksum calculation and clear any previously calculated value" },
    { "zeros_as_ones", 83, 83, 0, 0,
      "The current data word contains a checksum; an all-zeros checksum is encoded as all-ones. This is used by certain protocols, such as UDP, which use all zeros to indicate the checksum is not used. Indicates that an all-ones value at cb_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_3_row.zeros_as_ones_pos should be converted to all-zeros." },
    { "zeros_as_ones_pos", 88, 84, 0, 0,
      "The position of the checksum within the data word. Only necessary when zeros-as-ones is in use." },
    { "mul_2[0]", 89, 89, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[1]", 90, 90, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[2]", 91, 91, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[3]", 92, 92, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[4]", 93, 93, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[5]", 94, 94, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[6]", 95, 95, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[7]", 96, 96, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[8]", 97, 97, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[9]", 98, 98, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[10]", 99, 99, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[11]", 100, 100, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[12]", 101, 101, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[13]", 102, 102, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[14]", 103, 103, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[15]", 104, 104, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[16]", 105, 105, 0, 0,
      "Multiply each 16b word by two" },
};
reg_decoder_t tof3_prsr_mem_main_rspec__po_csum_ctrl_3_row = { 75, tof3_prsr_mem_main_rspec__po_csum_ctrl_3_row_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_prsr_mem_main_rspec__po_csum_ctrl_4_row_fld_list[] = {
    { "add", 15, 0, 0, 0,
      "16b constant to add to the checksum calculation" },
    { "swap[0]", 16, 16, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[1]", 17, 17, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[2]", 18, 18, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[3]", 19, 19, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[4]", 20, 20, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[5]", 21, 21, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[6]", 22, 22, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[7]", 23, 23, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[8]", 24, 24, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[9]", 25, 25, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[10]", 26, 26, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[11]", 27, 27, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[12]", 28, 28, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[13]", 29, 29, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[14]", 30, 30, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[15]", 31, 31, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "swap[16]", 32, 32, 0, 0,
      "Swap the corresponding two bytes within each 16b field" },
    { "shr", 33, 33, 0, 0,
      "Rotate the 32B packet word right by one prior to processing.Note: treat 32B input word as a left-aligned 34B data word." },
    { "mask[0]", 34, 34, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[1]", 35, 35, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[2]", 36, 36, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[3]", 37, 37, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[4]", 38, 38, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[5]", 39, 39, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[6]", 40, 40, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[7]", 41, 41, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[8]", 42, 42, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[9]", 43, 43, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[10]", 44, 44, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[11]", 45, 45, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[12]", 46, 46, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[13]", 47, 47, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[14]", 48, 48, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[15]", 49, 49, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[16]", 50, 50, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[17]", 51, 51, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[18]", 52, 52, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[19]", 53, 53, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[20]", 54, 54, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[21]", 55, 55, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[22]", 56, 56, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[23]", 57, 57, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[24]", 58, 58, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[25]", 59, 59, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[26]", 60, 60, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[27]", 61, 61, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[28]", 62, 62, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[29]", 63, 63, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[30]", 64, 64, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "mask[31]", 65, 65, 0, 0,
      "Include the corresponding byte in the checksum calculation" },
    { "dst_bit_hdr_end_pos", 70, 66, 0, 0,
      "[list][*]VERIFY: indicates the bit within the destination PHV to record the validity of the checksum.Note: [code]target_bit_position = bit_position % container_size[/code].[*]RESIDUAL: indicates where the header ends in the current packet word. Any headers after the current header are automatically subtracted from the residual.[*]CLOT: Unused.[/list]Valid only when cb_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_4_row.hdr_end is asserted." },
    { "dst", 78, 71, 0, 0,
      "Destination PHV field or tag.Valid only when cb_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_4_row.hdr_end is asserted." },
    { "hdr_end", 79, 79, 0, 0,
      "The header containing the checksum ends in this packet word.[list][*]0: Additional words to come[*]1: Final word of header. For VERIFY checksums, verify that the checksum is correct by comparing to zero. For RESIDUAL checksums, automatically subtract any headers that follow.[/list]" },
    { "type", 81, 80, 0, 0,
      "Checksum calculation type:[list][*]0: Verification -- Verify that a checksum is valid. A final sum of zero indicates a valid checksum.[*]1: Residual -- Calculate the residual of the checksum, i.e., the contribution of [italic]non-header[/italic] bytes of the packet.[*]2: CLOT -- Calculate the checksum for CLOT data. Only valid for checksum units 2-4; checksum output ignored when used for units 0 and 1.[/list]Sampled when cb_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_4_row.start == 1." },
    { "start", 82, 82, 0, 0,
      "Start a new checksum calculation and clear any previously calculated value" },
    { "zeros_as_ones", 83, 83, 0, 0,
      "The current data word contains a checksum; an all-zeros checksum is encoded as all-ones. This is used by certain protocols, such as UDP, which use all zeros to indicate the checksum is not used. Indicates that an all-ones value at cb_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_4_row.zeros_as_ones_pos should be converted to all-zeros." },
    { "zeros_as_ones_pos", 88, 84, 0, 0,
      "The position of the checksum within the data word. Only necessary when zeros-as-ones is in use." },
    { "mul_2[0]", 89, 89, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[1]", 90, 90, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[2]", 91, 91, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[3]", 92, 92, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[4]", 93, 93, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[5]", 94, 94, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[6]", 95, 95, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[7]", 96, 96, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[8]", 97, 97, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[9]", 98, 98, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[10]", 99, 99, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[11]", 100, 100, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[12]", 101, 101, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[13]", 102, 102, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[14]", 103, 103, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[15]", 104, 104, 0, 0,
      "Multiply each 16b word by two" },
    { "mul_2[16]", 105, 105, 0, 0,
      "Multiply each 16b word by two" },
};
reg_decoder_t tof3_prsr_mem_main_rspec__po_csum_ctrl_4_row = { 75, tof3_prsr_mem_main_rspec__po_csum_ctrl_4_row_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof3_pgr_mem_rspec__buffer_mem_word_fld_list[] = {
    { "mem_word", 127, 0, 0, 0,
      "" },
};
reg_decoder_t tof3_pgr_mem_rspec__buffer_mem_word = { 1, tof3_pgr_mem_rspec__buffer_mem_word_fld_list, 128 /* bits */, 0 };

cmd_arg_item_t tof3_tm_wac_pvt_table_mem_list[] = {
{ "mgid1_tbl", NULL, 0x0, &tof3_tm_wac_pvt_table__mgid1_tbl, 0x20000 },
{ "mgid2_tbl", NULL, 0x2000, &tof3_tm_wac_pvt_table__mgid2_tbl, 0x20000 },
};
cmd_arg_t tof3_tm_wac_pvt_table_mem = { 2, tof3_tm_wac_pvt_table_mem_list };

cmd_arg_item_t tof3_tm_wac_pipe_mem_rspec_mem_list[] = {
{ "csr_memory_wac_port_ppg_mapping", NULL, 0x0, &tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_port_ppg_mapping, 0xa50 },
{ "csr_memory_wac_ppg_min_cnt", NULL, 0x100, &tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_min_cnt, 0x800 },
{ "csr_memory_wac_ppg_shr_cnt", NULL, 0x180, &tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_shr_cnt, 0x800 },
{ "csr_memory_wac_ppg_hdr_cnt", NULL, 0x200, &tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_hdr_cnt, 0x800 },
{ "csr_memory_wac_ppg_min_th", NULL, 0x280, &tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_min_th, 0x800 },
{ "csr_memory_wac_ppg_shr_th", NULL, 0x300, &tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_shr_th, 0x800 },
{ "csr_memory_wac_ppg_hdr_th", NULL, 0x380, &tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_hdr_th, 0x800 },
{ "csr_memory_wac_ppg_pfc", NULL, 0x400, &tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_pfc, 0x800 },
{ "csr_memory_wac_ppg_icos", NULL, 0x480, &tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_icos, 0x800 },
{ "csr_memory_wac_ppg_drop_st", NULL, 0x500, &tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_drop_st, 0x800 },
{ "csr_memory_wac_pg_drop_st", NULL, 0x580, &tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_drop_st, 0x250 },
{ "csr_memory_wac_ppg_off_idx", NULL, 0x600, &tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_off_idx, 0x800 },
{ "csr_memory_wac_pg_off_idx", NULL, 0x680, &tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_off_idx, 0x250 },
{ "csr_memory_wac_pg_min_cnt", NULL, 0x6c0, &tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_min_cnt, 0x250 },
{ "csr_memory_wac_pg_shr_cnt", NULL, 0x700, &tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_shr_cnt, 0x250 },
{ "csr_memory_wac_pg_min_th", NULL, 0x740, &tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_min_th, 0x250 },
{ "csr_memory_wac_pg_shr_th", NULL, 0x780, &tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_shr_th, 0x250 },
{ "csr_memory_wac_port_shr_th", NULL, 0x7c0, &tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_port_shr_th, 0x250 },
{ "csr_memory_wac_port_hdr_th", NULL, 0x800, &tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_port_hdr_th, 0x250 },
{ "csr_memory_wac_port_wm", NULL, 0x840, &tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_port_wm, 0x250 },
{ "csr_memory_wac_port_min_cnt", NULL, 0x880, &tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_port_min_cnt, 0x250 },
{ "csr_memory_wac_port_hdr_cnt", NULL, 0x8c0, &tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_port_hdr_cnt, 0x250 },
{ "csr_memory_wac_port_shr_cnt", NULL, 0x900, &tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_port_shr_cnt, 0x250 },
{ "csr_memory_wac_port_st", NULL, 0x940, &tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_port_st, 0x250 },
{ "csr_memory_wac_pg_wm_cnt", NULL, 0xa00, &tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_wm_cnt, 0xa50 },
{ "csr_memory_wac_drop_count_ppg", NULL, 0xb00, &tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_drop_count_ppg, 0xa50 },
{ "csr_memory_wac_drop_count_port", NULL, 0xbc0, &tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_drop_count_port, 0x250 },
{ "csr_memory_wac_pfc_state", NULL, 0xc00, &tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_pfc_state, 0x240 },
{ "csr_memory_wac_qid_map", NULL, 0xe00, &tof3_tm_wac_pipe_mem_rspec__csr_memory_wac_qid_map, 0x1200 },
{ "csr_memory_wac_pvt_table", &tof3_tm_wac_pvt_table_mem, 0x4000, NULL },
};
cmd_arg_t tof3_tm_wac_pipe_mem_rspec_mem = { 30, tof3_tm_wac_pipe_mem_rspec_mem_list };

cmd_arg_item_t tof3_tm_wac_common_mem_rspec_mem_list[] = {
{ "wac_common_mem_lcl_qstate", NULL, 0x0, &tof3_tm_wac_common_mem_rspec__wac_common_mem_lcl_qstate, 0x2400 },
{ "wac_common_mem_rmt_qstate", NULL, 0x400, &tof3_tm_wac_common_mem_rspec__wac_common_mem_rmt_qstate, 0x2400 },
{ "wac_common_mem_qacq_ap_config", NULL, 0x800, &tof3_tm_wac_common_mem_rspec__wac_common_mem_qacq_ap_config, 0x2400 },
};
cmd_arg_t tof3_tm_wac_common_mem_rspec_mem = { 3, tof3_tm_wac_common_mem_rspec_mem_list };

cmd_arg_item_t tof3_tm_wac_mem_top_rspec_mem_list[] = {
{ "wac_pipe_mem[0]", &tof3_tm_wac_pipe_mem_rspec_mem, 0x0, NULL },
{ "wac_pipe_mem[1]", &tof3_tm_wac_pipe_mem_rspec_mem, 0x10000000, NULL },
{ "wac_pipe_mem[2]", &tof3_tm_wac_pipe_mem_rspec_mem, 0x20000000, NULL },
{ "wac_pipe_mem[3]", &tof3_tm_wac_pipe_mem_rspec_mem, 0x30000000, NULL },
{ "wac_common_mem[0]", &tof3_tm_wac_common_mem_rspec_mem, 0x80000000, NULL },
{ "wac_common_mem[1]", &tof3_tm_wac_common_mem_rspec_mem, 0x90000000, NULL },
{ "wac_common_mem[2]", &tof3_tm_wac_common_mem_rspec_mem, 0xa0000000, NULL },
{ "wac_common_mem[3]", &tof3_tm_wac_common_mem_rspec_mem, 0xb0000000, NULL },
{ "wac_common_mem[4]", &tof3_tm_wac_common_mem_rspec_mem, 0xc0000000, NULL },
{ "wac_common_mem[5]", &tof3_tm_wac_common_mem_rspec_mem, 0xd0000000, NULL },
{ "wac_common_mem[6]", &tof3_tm_wac_common_mem_rspec_mem, 0xe0000000, NULL },
{ "wac_common_mem[7]", &tof3_tm_wac_common_mem_rspec_mem, 0xf0000000, NULL },
};
cmd_arg_t tof3_tm_wac_mem_top_rspec_mem = { 12, tof3_tm_wac_mem_top_rspec_mem_list };

cmd_arg_item_t tof3_tm_caa_mem_top_rspec_mem_list[] = {
{ "caa_block_grp0", NULL, 0x0, &tof3_tm_caa_mem_top_rspec__caa_block_grp0, 0x80000 },
{ "caa_block_grp1", NULL, 0x8000, &tof3_tm_caa_mem_top_rspec__caa_block_grp1, 0x80000 },
{ "caa_block_grp2", NULL, 0x10000, &tof3_tm_caa_mem_top_rspec__caa_block_grp2, 0x80000 },
{ "caa_block_grp3", NULL, 0x18000, &tof3_tm_caa_mem_top_rspec__caa_block_grp3, 0x80000 },
{ "caa_block_grp4", NULL, 0x20000, &tof3_tm_caa_mem_top_rspec__caa_block_grp4, 0x80000 },
{ "caa_block_grp5", NULL, 0x28000, &tof3_tm_caa_mem_top_rspec__caa_block_grp5, 0x80000 },
};
cmd_arg_t tof3_tm_caa_mem_top_rspec_mem = { 6, tof3_tm_caa_mem_top_rspec_mem_list };

cmd_arg_item_t tof3_tm_qac_pipe_mem_rspec_mem_list[] = {
{ "csr_memory_qac_queue_config", NULL, 0x0, &tof3_tm_qac_pipe_mem_rspec__csr_memory_qac_queue_config, 0x2400 },
{ "csr_memory_qac_queue_cell_count", NULL, 0x400, &tof3_tm_qac_pipe_mem_rspec__csr_memory_qac_queue_cell_count, 0x2400 },
{ "csr_memory_qac_queue_wm_cell_count", NULL, 0x800, &tof3_tm_qac_pipe_mem_rspec__csr_memory_qac_queue_wm_cell_count, 0x2400 },
{ "csr_memory_qac_port_wm_cell_count", NULL, 0xa40, &tof3_tm_qac_pipe_mem_rspec__csr_memory_qac_port_wm_cell_count, 0x240 },
{ "csr_memory_qac_port_config", NULL, 0xa80, &tof3_tm_qac_pipe_mem_rspec__csr_memory_qac_port_config, 0x240 },
{ "csr_memory_qac_port_cell_count", NULL, 0xac0, &tof3_tm_qac_pipe_mem_rspec__csr_memory_qac_port_cell_count, 0x240 },
{ "csr_memory_qac_drop_count_port", NULL, 0xb00, &tof3_tm_qac_pipe_mem_rspec__csr_memory_qac_drop_count_port, 0x6c0 },
{ "csr_memory_qac_drop_count_queue", NULL, 0xc00, &tof3_tm_qac_pipe_mem_rspec__csr_memory_qac_drop_count_queue, 0x2400 },
{ "csr_memory_qac_qid_mapping", NULL, 0xe80, &tof3_tm_qac_pipe_mem_rspec__csr_memory_qac_qid_mapping, 0x480 },
};
cmd_arg_t tof3_tm_qac_pipe_mem_rspec_mem = { 9, tof3_tm_qac_pipe_mem_rspec_mem_list };

cmd_arg_item_t tof3_tm_qac_mem_top_rspec_mem_list[] = {
{ "qac_pipe_mem[0]", &tof3_tm_qac_pipe_mem_rspec_mem, 0x0, NULL },
{ "qac_pipe_mem[1]", &tof3_tm_qac_pipe_mem_rspec_mem, 0x10000000, NULL },
{ "qac_pipe_mem[2]", &tof3_tm_qac_pipe_mem_rspec_mem, 0x20000000, NULL },
{ "qac_pipe_mem[3]", &tof3_tm_qac_pipe_mem_rspec_mem, 0x30000000, NULL },
{ "qac_pipe_mem[4]", &tof3_tm_qac_pipe_mem_rspec_mem, 0x40000000, NULL },
{ "qac_pipe_mem[5]", &tof3_tm_qac_pipe_mem_rspec_mem, 0x50000000, NULL },
{ "qac_pipe_mem[6]", &tof3_tm_qac_pipe_mem_rspec_mem, 0x60000000, NULL },
{ "qac_pipe_mem[7]", &tof3_tm_qac_pipe_mem_rspec_mem, 0x70000000, NULL },
};
cmd_arg_t tof3_tm_qac_mem_top_rspec_mem = { 8, tof3_tm_qac_mem_top_rspec_mem_list };

cmd_arg_item_t tof3_tm_sch_pipe_mem_rspec_mem_list[] = {
{ "port_max_lb_static_mem", NULL, 0x0, &tof3_tm_sch_pipe_mem_rspec__port_max_lb_static_mem, 0x240 },
{ "port_max_lb_dynamic_mem", NULL, 0x40, &tof3_tm_sch_pipe_mem_rspec__port_max_lb_dynamic_mem, 0x240 },
{ "l1_min_lb_static_mem", NULL, 0x100, &tof3_tm_sch_pipe_mem_rspec__l1_min_lb_static_mem, 0x900 },
{ "l1_min_lb_dynamic_mem", NULL, 0x200, &tof3_tm_sch_pipe_mem_rspec__l1_min_lb_dynamic_mem, 0x900 },
{ "l1_max_lb_static_mem", NULL, 0x300, &tof3_tm_sch_pipe_mem_rspec__l1_max_lb_static_mem, 0x900 },
{ "l1_max_lb_dynamic_mem", NULL, 0x400, &tof3_tm_sch_pipe_mem_rspec__l1_max_lb_dynamic_mem, 0x900 },
{ "l1_exc_static_mem", NULL, 0x500, &tof3_tm_sch_pipe_mem_rspec__l1_exc_static_mem, 0x900 },
{ "l1_exc_dynamic_mem", NULL, 0x600, &tof3_tm_sch_pipe_mem_rspec__l1_exc_dynamic_mem, 0x900 },
{ "q_min_lb_static_mem", NULL, 0x800, &tof3_tm_sch_pipe_mem_rspec__q_min_lb_static_mem, 0x2400 },
{ "q_min_lb_dynamic_mem", NULL, 0xc00, &tof3_tm_sch_pipe_mem_rspec__q_min_lb_dynamic_mem, 0x2400 },
{ "q_max_lb_static_mem", NULL, 0x1000, &tof3_tm_sch_pipe_mem_rspec__q_max_lb_static_mem, 0x2400 },
{ "q_max_lb_dynamic_mem", NULL, 0x1400, &tof3_tm_sch_pipe_mem_rspec__q_max_lb_dynamic_mem, 0x2400 },
{ "q_exc_static_mem", NULL, 0x1800, &tof3_tm_sch_pipe_mem_rspec__q_exc_static_mem, 0x2400 },
{ "q_exc_dynamic_mem", NULL, 0x1c00, &tof3_tm_sch_pipe_mem_rspec__q_exc_dynamic_mem, 0x2400 },
{ "q_pfc_status_mem", NULL, 0x2000, &tof3_tm_sch_pipe_mem_rspec__q_pfc_status_mem, 0x2400 },
{ "q_adv_fc_status_mem", NULL, 0x2400, &tof3_tm_sch_pipe_mem_rspec__q_adv_fc_status_mem, 0x2400 },
{ "p_occ_mem", NULL, 0x2640, &tof3_tm_sch_pipe_mem_rspec__p_occ_mem, 0x240 },
{ "l1_occ_mem", NULL, 0x2700, &tof3_tm_sch_pipe_mem_rspec__l1_occ_mem, 0x900 },
{ "q_occ_mem", NULL, 0x2800, &tof3_tm_sch_pipe_mem_rspec__q_occ_mem, 0x2400 },
};
cmd_arg_t tof3_tm_sch_pipe_mem_rspec_mem = { 19, tof3_tm_sch_pipe_mem_rspec_mem_list };

cmd_arg_item_t tof3_tm_sch_mem_top_rspec_mem_list[] = {
{ "sch[0]", &tof3_tm_sch_pipe_mem_rspec_mem, 0x0, NULL },
{ "sch[1]", &tof3_tm_sch_pipe_mem_rspec_mem, 0x10000000, NULL },
{ "sch[2]", &tof3_tm_sch_pipe_mem_rspec_mem, 0x20000000, NULL },
{ "sch[3]", &tof3_tm_sch_pipe_mem_rspec_mem, 0x30000000, NULL },
};
cmd_arg_t tof3_tm_sch_mem_top_rspec_mem = { 4, tof3_tm_sch_mem_top_rspec_mem_list };

cmd_arg_item_t tof3_tm_clc_pipe_mem_rspec_mem_list[] = {
{ "csr_memory_clc_clm", NULL, 0x0, &tof3_tm_clc_pipe_mem_rspec__csr_memory_clc_clm, 0x600000 },
};
cmd_arg_t tof3_tm_clc_pipe_mem_rspec_mem = { 1, tof3_tm_clc_pipe_mem_rspec_mem_list };

cmd_arg_item_t tof3_tm_clc_mem_top_rspec_mem_list[] = {
{ "clc[0]", &tof3_tm_clc_pipe_mem_rspec_mem, 0x0, NULL },
{ "clc[1]", &tof3_tm_clc_pipe_mem_rspec_mem, 0x10000000, NULL },
{ "clc[2]", &tof3_tm_clc_pipe_mem_rspec_mem, 0x20000000, NULL },
{ "clc[3]", &tof3_tm_clc_pipe_mem_rspec_mem, 0x30000000, NULL },
};
cmd_arg_t tof3_tm_clc_mem_top_rspec_mem = { 4, tof3_tm_clc_mem_top_rspec_mem_list };

cmd_arg_item_t tof3_tm_pex_pipe_mem_rspec_mem_list[] = {
{ "csr_memory_pex_clm", NULL, 0x0, &tof3_tm_pex_pipe_mem_rspec__csr_memory_pex_clm, 0x600000 },
};
cmd_arg_t tof3_tm_pex_pipe_mem_rspec_mem = { 1, tof3_tm_pex_pipe_mem_rspec_mem_list };

cmd_arg_item_t tof3_tm_pex_mem_top_rspec_mem_list[] = {
{ "pex[0]", &tof3_tm_pex_pipe_mem_rspec_mem, 0x0, NULL },
{ "pex[1]", &tof3_tm_pex_pipe_mem_rspec_mem, 0x10000000, NULL },
{ "pex[2]", &tof3_tm_pex_pipe_mem_rspec_mem, 0x20000000, NULL },
{ "pex[3]", &tof3_tm_pex_pipe_mem_rspec_mem, 0x30000000, NULL },
{ "pex[4]", &tof3_tm_pex_pipe_mem_rspec_mem, 0x40000000, NULL },
{ "pex[5]", &tof3_tm_pex_pipe_mem_rspec_mem, 0x50000000, NULL },
{ "pex[6]", &tof3_tm_pex_pipe_mem_rspec_mem, 0x60000000, NULL },
{ "pex[7]", &tof3_tm_pex_pipe_mem_rspec_mem, 0x70000000, NULL },
};
cmd_arg_t tof3_tm_pex_mem_top_rspec_mem = { 8, tof3_tm_pex_mem_top_rspec_mem_list };

cmd_arg_item_t tof3_tm_qlc_pipe_mem_rspec_mem_list[] = {
{ "csr_memory_qlc_qlm", NULL, 0x0, &tof3_tm_qlc_pipe_mem_rspec__csr_memory_qlc_qlm, 0x600000 },
{ "csr_memory_qlc_ht", NULL, 0x80000, &tof3_tm_qlc_pipe_mem_rspec__csr_memory_qlc_ht, 0x4800 },
{ "csr_memory_qlc_vq", NULL, 0x80800, &tof3_tm_qlc_pipe_mem_rspec__csr_memory_qlc_vq, 0x2400 },
};
cmd_arg_t tof3_tm_qlc_pipe_mem_rspec_mem = { 3, tof3_tm_qlc_pipe_mem_rspec_mem_list };

cmd_arg_item_t tof3_tm_qlc_mem_top_rspec_mem_list[] = {
{ "qlc_mem[0]", &tof3_tm_qlc_pipe_mem_rspec_mem, 0x0, NULL },
{ "qlc_mem[1]", &tof3_tm_qlc_pipe_mem_rspec_mem, 0x10000000, NULL },
{ "qlc_mem[2]", &tof3_tm_qlc_pipe_mem_rspec_mem, 0x20000000, NULL },
{ "qlc_mem[3]", &tof3_tm_qlc_pipe_mem_rspec_mem, 0x30000000, NULL },
{ "qlc_mem[4]", &tof3_tm_qlc_pipe_mem_rspec_mem, 0x40000000, NULL },
{ "qlc_mem[5]", &tof3_tm_qlc_pipe_mem_rspec_mem, 0x50000000, NULL },
{ "qlc_mem[6]", &tof3_tm_qlc_pipe_mem_rspec_mem, 0x60000000, NULL },
{ "qlc_mem[7]", &tof3_tm_qlc_pipe_mem_rspec_mem, 0x70000000, NULL },
};
cmd_arg_t tof3_tm_qlc_mem_top_rspec_mem = { 8, tof3_tm_qlc_mem_top_rspec_mem_list };

cmd_arg_item_t tof3_tm_prc_pipe_mem_rspec_mem_list[] = {
{ "csr_memory_prc_prm", NULL, 0x0, &tof3_tm_prc_pipe_mem_rspec__csr_memory_prc_prm, 0x180000 },
{ "csr_memory_prc_map", NULL, 0x20000, &tof3_tm_prc_pipe_mem_rspec__csr_memory_prc_map, 0x18000 },
{ "csr_memory_prc_cache0", NULL, 0x22000, &tof3_tm_prc_pipe_mem_rspec__csr_memory_prc_cache0, 0x2000 },
{ "csr_memory_prc_cache1", NULL, 0x22200, &tof3_tm_prc_pipe_mem_rspec__csr_memory_prc_cache1, 0x2000 },
{ "csr_memory_prc_tag", NULL, 0x22400, &tof3_tm_prc_pipe_mem_rspec__csr_memory_prc_tag, 0x2000 },
};
cmd_arg_t tof3_tm_prc_pipe_mem_rspec_mem = { 5, tof3_tm_prc_pipe_mem_rspec_mem_list };

cmd_arg_item_t tof3_tm_prc_mem_top_rspec_mem_list[] = {
{ "prc_mem[0]", &tof3_tm_prc_pipe_mem_rspec_mem, 0x0, NULL },
{ "prc_mem[1]", &tof3_tm_prc_pipe_mem_rspec_mem, 0x10000000, NULL },
{ "prc_mem[2]", &tof3_tm_prc_pipe_mem_rspec_mem, 0x20000000, NULL },
{ "prc_mem[3]", &tof3_tm_prc_pipe_mem_rspec_mem, 0x30000000, NULL },
{ "prc_mem[4]", &tof3_tm_prc_pipe_mem_rspec_mem, 0x40000000, NULL },
{ "prc_mem[5]", &tof3_tm_prc_pipe_mem_rspec_mem, 0x50000000, NULL },
{ "prc_mem[6]", &tof3_tm_prc_pipe_mem_rspec_mem, 0x60000000, NULL },
{ "prc_mem[7]", &tof3_tm_prc_pipe_mem_rspec_mem, 0x70000000, NULL },
};
cmd_arg_t tof3_tm_prc_mem_top_rspec_mem = { 8, tof3_tm_prc_mem_top_rspec_mem_list };

cmd_arg_item_t tof3_tm_pre_pipe_mem_rspec_mem_list[] = {
{ "mit_mem_word", NULL, 0x0, &tof3_tm_pre_pipe_mem_rspec__mit_mem_word, 0x40000 },
};
cmd_arg_t tof3_tm_pre_pipe_mem_rspec_mem = { 1, tof3_tm_pre_pipe_mem_rspec_mem_list };

cmd_arg_item_t tof3_tm_pre_common_mem_rspec_mem_list[] = {
{ "rdm_mem_word", NULL, 0x800000, &tof3_tm_pre_common_mem_rspec__rdm_mem_word, 0x800000 },
{ "pbt0_mem_word", NULL, 0x40000000, &tof3_tm_pre_common_mem_rspec__pbt0_mem_word, 0x1200 },
{ "pbt1_mem_word", NULL, 0x40100000, &tof3_tm_pre_common_mem_rspec__pbt1_mem_word, 0x1200 },
{ "lit0_np_mem_word", NULL, 0x40200000, &tof3_tm_pre_common_mem_rspec__lit0_np_mem_word, 0x1000 },
{ "lit1_np_mem_word", NULL, 0x40300000, &tof3_tm_pre_common_mem_rspec__lit1_np_mem_word, 0x1000 },
{ "lit0_bm_mem_word0", NULL, 0x80000000, &tof3_tm_pre_common_mem_rspec__lit0_bm_mem_word0, 0x1000 },
{ "lit0_bm_mem_word1", NULL, 0x80020000, &tof3_tm_pre_common_mem_rspec__lit0_bm_mem_word1, 0x1000 },
{ "lit0_bm_mem_word2", NULL, 0x80040000, &tof3_tm_pre_common_mem_rspec__lit0_bm_mem_word2, 0x1000 },
{ "lit0_bm_mem_word3", NULL, 0x80060000, &tof3_tm_pre_common_mem_rspec__lit0_bm_mem_word3, 0x1000 },
{ "lit0_bm_mem_word4", NULL, 0x80080000, &tof3_tm_pre_common_mem_rspec__lit0_bm_mem_word4, 0x1000 },
{ "lit0_bm_mem_word5", NULL, 0x800a0000, &tof3_tm_pre_common_mem_rspec__lit0_bm_mem_word5, 0x1000 },
{ "lit0_bm_mem_word6", NULL, 0x800c0000, &tof3_tm_pre_common_mem_rspec__lit0_bm_mem_word6, 0x1000 },
{ "lit0_bm_mem_word7", NULL, 0x800e0000, &tof3_tm_pre_common_mem_rspec__lit0_bm_mem_word7, 0x1000 },
{ "lit1_bm_mem_word0", NULL, 0x80100000, &tof3_tm_pre_common_mem_rspec__lit1_bm_mem_word0, 0x1000 },
{ "lit1_bm_mem_word1", NULL, 0x80120000, &tof3_tm_pre_common_mem_rspec__lit1_bm_mem_word1, 0x1000 },
{ "lit1_bm_mem_word2", NULL, 0x80140000, &tof3_tm_pre_common_mem_rspec__lit1_bm_mem_word2, 0x1000 },
{ "lit1_bm_mem_word3", NULL, 0x80160000, &tof3_tm_pre_common_mem_rspec__lit1_bm_mem_word3, 0x1000 },
{ "lit1_bm_mem_word4", NULL, 0x80180000, &tof3_tm_pre_common_mem_rspec__lit1_bm_mem_word4, 0x1000 },
{ "lit1_bm_mem_word5", NULL, 0x801a0000, &tof3_tm_pre_common_mem_rspec__lit1_bm_mem_word5, 0x1000 },
{ "lit1_bm_mem_word6", NULL, 0x801c0000, &tof3_tm_pre_common_mem_rspec__lit1_bm_mem_word6, 0x1000 },
{ "lit1_bm_mem_word7", NULL, 0x801e0000, &tof3_tm_pre_common_mem_rspec__lit1_bm_mem_word7, 0x1000 },
{ "pmt0_mem_word0", NULL, 0x80200000, &tof3_tm_pre_common_mem_rspec__pmt0_mem_word0, 0x1200 },
{ "pmt0_mem_word1", NULL, 0x80220000, &tof3_tm_pre_common_mem_rspec__pmt0_mem_word1, 0x1200 },
{ "pmt0_mem_word2", NULL, 0x80240000, &tof3_tm_pre_common_mem_rspec__pmt0_mem_word2, 0x1200 },
{ "pmt0_mem_word3", NULL, 0x80260000, &tof3_tm_pre_common_mem_rspec__pmt0_mem_word3, 0x1200 },
{ "pmt0_mem_word4", NULL, 0x80280000, &tof3_tm_pre_common_mem_rspec__pmt0_mem_word4, 0x1200 },
{ "pmt0_mem_word5", NULL, 0x802a0000, &tof3_tm_pre_common_mem_rspec__pmt0_mem_word5, 0x1200 },
{ "pmt0_mem_word6", NULL, 0x802c0000, &tof3_tm_pre_common_mem_rspec__pmt0_mem_word6, 0x1200 },
{ "pmt0_mem_word7", NULL, 0x802e0000, &tof3_tm_pre_common_mem_rspec__pmt0_mem_word7, 0x1200 },
{ "pmt1_mem_word0", NULL, 0x80300000, &tof3_tm_pre_common_mem_rspec__pmt1_mem_word0, 0x1200 },
{ "pmt1_mem_word1", NULL, 0x80320000, &tof3_tm_pre_common_mem_rspec__pmt1_mem_word1, 0x1200 },
{ "pmt1_mem_word2", NULL, 0x80340000, &tof3_tm_pre_common_mem_rspec__pmt1_mem_word2, 0x1200 },
{ "pmt1_mem_word3", NULL, 0x80360000, &tof3_tm_pre_common_mem_rspec__pmt1_mem_word3, 0x1200 },
{ "pmt1_mem_word4", NULL, 0x80380000, &tof3_tm_pre_common_mem_rspec__pmt1_mem_word4, 0x1200 },
{ "pmt1_mem_word5", NULL, 0x803a0000, &tof3_tm_pre_common_mem_rspec__pmt1_mem_word5, 0x1200 },
{ "pmt1_mem_word6", NULL, 0x803c0000, &tof3_tm_pre_common_mem_rspec__pmt1_mem_word6, 0x1200 },
{ "pmt1_mem_word7", NULL, 0x803e0000, &tof3_tm_pre_common_mem_rspec__pmt1_mem_word7, 0x1200 },
};
cmd_arg_t tof3_tm_pre_common_mem_rspec_mem = { 37, tof3_tm_pre_common_mem_rspec_mem_list };

cmd_arg_item_t tof3_tm_pre_mem_top_rspec_mem_list[] = {
{ "pre_pipe_mem[0]", &tof3_tm_pre_pipe_mem_rspec_mem, 0x0, NULL },
{ "pre_pipe_mem[1]", &tof3_tm_pre_pipe_mem_rspec_mem, 0x100000, NULL },
{ "pre_pipe_mem[2]", &tof3_tm_pre_pipe_mem_rspec_mem, 0x200000, NULL },
{ "pre_pipe_mem[3]", &tof3_tm_pre_pipe_mem_rspec_mem, 0x300000, NULL },
{ "pre_pipe_mem[4]", &tof3_tm_pre_pipe_mem_rspec_mem, 0x400000, NULL },
{ "pre_pipe_mem[5]", &tof3_tm_pre_pipe_mem_rspec_mem, 0x500000, NULL },
{ "pre_pipe_mem[6]", &tof3_tm_pre_pipe_mem_rspec_mem, 0x600000, NULL },
{ "pre_pipe_mem[7]", &tof3_tm_pre_pipe_mem_rspec_mem, 0x700000, NULL },
{ "pre_common_mem", &tof3_tm_pre_common_mem_rspec_mem, 0x100000000, NULL },
};
cmd_arg_t tof3_tm_pre_mem_top_rspec_mem = { 9, tof3_tm_pre_mem_top_rspec_mem_list };

cmd_arg_item_t tof3_tm_psc_mem_top_rspec_mem_list[] = {
{ "psc_block_grp0", NULL, 0x0, &tof3_tm_psc_mem_top_rspec__psc_block_grp0, 0x80000 },
{ "psc_block_grp1", NULL, 0x8000, &tof3_tm_psc_mem_top_rspec__psc_block_grp1, 0x80000 },
{ "psc_block_grp2", NULL, 0x10000, &tof3_tm_psc_mem_top_rspec__psc_block_grp2, 0x80000 },
{ "psc_block_grp3", NULL, 0x18000, &tof3_tm_psc_mem_top_rspec__psc_block_grp3, 0x80000 },
{ "psc_block_grp4", NULL, 0x20000, &tof3_tm_psc_mem_top_rspec__psc_block_grp4, 0x80000 },
{ "psc_block_grp5", NULL, 0x28000, &tof3_tm_psc_mem_top_rspec__psc_block_grp5, 0x80000 },
};
cmd_arg_t tof3_tm_psc_mem_top_rspec_mem = { 6, tof3_tm_psc_mem_top_rspec_mem_list };

cmd_arg_item_t tof3_tm_top_mem_rspec_mem_list[] = {
{ "tm_wac", &tof3_tm_wac_mem_top_rspec_mem, 0x4200000000, NULL },
{ "tm_caa", &tof3_tm_caa_mem_top_rspec_mem, 0x4600000000, NULL },
{ "tm_qac", &tof3_tm_qac_mem_top_rspec_mem, 0x4a00000000, NULL },
{ "tm_sch", &tof3_tm_sch_mem_top_rspec_mem, 0x4e00000000, NULL },
{ "tm_clc", &tof3_tm_clc_mem_top_rspec_mem, 0x5600000000, NULL },
{ "tm_pex", &tof3_tm_pex_mem_top_rspec_mem, 0x5a00000000, NULL },
{ "tm_qlc", &tof3_tm_qlc_mem_top_rspec_mem, 0x5e00000000, NULL },
{ "tm_prc", &tof3_tm_prc_mem_top_rspec_mem, 0x6200000000, NULL },
{ "tm_pre", &tof3_tm_pre_mem_top_rspec_mem, 0x6600000000, NULL },
{ "tm_psc", &tof3_tm_psc_mem_top_rspec_mem, 0x6a00000000, NULL },
};
cmd_arg_t tof3_tm_top_mem_rspec_mem = { 10, tof3_tm_top_mem_rspec_mem_list };

cmd_arg_item_t tof3_ipb_mem_rspec_mem_list[] = {
{ "meta_phase0_16byte", NULL, 0x0, &tof3_ipb_mem_rspec__meta_phase0_16byte, 0x500 },
{ "meta_phase0_8byte_ver", NULL, 0x80, &tof3_ipb_mem_rspec__meta_phase0_8byte_ver, 0x500 },
};
cmd_arg_t tof3_ipb_mem_rspec_mem = { 2, tof3_ipb_mem_rspec_mem_list };

cmd_arg_item_t tof3_prsr_mem_main_rspec_mem_list[] = {
{ "po_action_row", NULL, 0x0, &tof3_prsr_mem_main_rspec__po_action_row, 0 },
{ "ml_tcam_row", NULL, 0x400, &tof3_prsr_mem_main_rspec__ml_tcam_row, 0 },
{ "ml_ea_row", NULL, 0x500, &tof3_prsr_mem_main_rspec__ml_ea_row, 0 },
{ "ml_ctr_init_ram", NULL, 0x600, &tof3_prsr_mem_main_rspec__ml_ctr_init_ram, 0 },
{ "po_csum_ctrl_0_row", NULL, 0x620, &tof3_prsr_mem_main_rspec__po_csum_ctrl_0_row, 0 },
{ "po_csum_ctrl_1_row", NULL, 0x640, &tof3_prsr_mem_main_rspec__po_csum_ctrl_1_row, 0 },
{ "po_csum_ctrl_2_row", NULL, 0x660, &tof3_prsr_mem_main_rspec__po_csum_ctrl_2_row, 0 },
{ "po_csum_ctrl_3_row", NULL, 0x680, &tof3_prsr_mem_main_rspec__po_csum_ctrl_3_row, 0 },
{ "po_csum_ctrl_4_row", NULL, 0x6a0, &tof3_prsr_mem_main_rspec__po_csum_ctrl_4_row, 0 },
{ "ipb_mem", &tof3_ipb_mem_rspec_mem, 0x700, NULL },
};
cmd_arg_t tof3_prsr_mem_main_rspec_mem = { 10, tof3_prsr_mem_main_rspec_mem_list };

cmd_arg_item_t tof3_pgr_mem_rspec_mem_list[] = {
{ "buffer_mem_word", NULL, 0x0, &tof3_pgr_mem_rspec__buffer_mem_word, 0 },
};
cmd_arg_t tof3_pgr_mem_rspec_mem = { 1, tof3_pgr_mem_rspec_mem_list };

cmd_arg_item_t tof3_parde_mem_mem_list[] = {
{ "i_prsr_mem[0]", &tof3_prsr_mem_main_rspec_mem, 0x0, NULL },
{ "i_prsr_mem[1]", &tof3_prsr_mem_main_rspec_mem, 0x800, NULL },
{ "i_prsr_mem[2]", &tof3_prsr_mem_main_rspec_mem, 0x1000, NULL },
{ "i_prsr_mem[3]", &tof3_prsr_mem_main_rspec_mem, 0x1800, NULL },
{ "i_prsr_mem[4]", &tof3_prsr_mem_main_rspec_mem, 0x2000, NULL },
{ "i_prsr_mem[5]", &tof3_prsr_mem_main_rspec_mem, 0x2800, NULL },
{ "i_prsr_mem[6]", &tof3_prsr_mem_main_rspec_mem, 0x3000, NULL },
{ "i_prsr_mem[7]", &tof3_prsr_mem_main_rspec_mem, 0x3800, NULL },
{ "i_prsr_mem[8]", &tof3_prsr_mem_main_rspec_mem, 0x4000, NULL },
{ "i_prsr_mem[9]", &tof3_prsr_mem_main_rspec_mem, 0x4800, NULL },
{ "i_prsr_mem[10]", &tof3_prsr_mem_main_rspec_mem, 0x5000, NULL },
{ "i_prsr_mem[11]", &tof3_prsr_mem_main_rspec_mem, 0x5800, NULL },
{ "i_prsr_mem[12]", &tof3_prsr_mem_main_rspec_mem, 0x6000, NULL },
{ "i_prsr_mem[13]", &tof3_prsr_mem_main_rspec_mem, 0x6800, NULL },
{ "i_prsr_mem[14]", &tof3_prsr_mem_main_rspec_mem, 0x7000, NULL },
{ "i_prsr_mem[15]", &tof3_prsr_mem_main_rspec_mem, 0x7800, NULL },
{ "i_prsr_mem[16]", &tof3_prsr_mem_main_rspec_mem, 0x8000, NULL },
{ "i_prsr_mem[17]", &tof3_prsr_mem_main_rspec_mem, 0x8800, NULL },
{ "i_prsr_mem[18]", &tof3_prsr_mem_main_rspec_mem, 0x9000, NULL },
{ "i_prsr_mem[19]", &tof3_prsr_mem_main_rspec_mem, 0x9800, NULL },
{ "i_prsr_mem[20]", &tof3_prsr_mem_main_rspec_mem, 0xa000, NULL },
{ "i_prsr_mem[21]", &tof3_prsr_mem_main_rspec_mem, 0xa800, NULL },
{ "i_prsr_mem[22]", &tof3_prsr_mem_main_rspec_mem, 0xb000, NULL },
{ "i_prsr_mem[23]", &tof3_prsr_mem_main_rspec_mem, 0xb800, NULL },
{ "i_prsr_mem[24]", &tof3_prsr_mem_main_rspec_mem, 0xc000, NULL },
{ "i_prsr_mem[25]", &tof3_prsr_mem_main_rspec_mem, 0xc800, NULL },
{ "i_prsr_mem[26]", &tof3_prsr_mem_main_rspec_mem, 0xd000, NULL },
{ "i_prsr_mem[27]", &tof3_prsr_mem_main_rspec_mem, 0xd800, NULL },
{ "i_prsr_mem[28]", &tof3_prsr_mem_main_rspec_mem, 0xe000, NULL },
{ "i_prsr_mem[29]", &tof3_prsr_mem_main_rspec_mem, 0xe800, NULL },
{ "i_prsr_mem[30]", &tof3_prsr_mem_main_rspec_mem, 0xf000, NULL },
{ "i_prsr_mem[31]", &tof3_prsr_mem_main_rspec_mem, 0xf800, NULL },
{ "i_prsr_mem[32]", &tof3_prsr_mem_main_rspec_mem, 0x10000, NULL },
{ "i_prsr_mem[33]", &tof3_prsr_mem_main_rspec_mem, 0x10800, NULL },
{ "i_prsr_mem[34]", &tof3_prsr_mem_main_rspec_mem, 0x11000, NULL },
{ "i_prsr_mem[35]", &tof3_prsr_mem_main_rspec_mem, 0x11800, NULL },
{ "e_prsr_mem[0]", &tof3_prsr_mem_main_rspec_mem, 0x20000, NULL },
{ "e_prsr_mem[1]", &tof3_prsr_mem_main_rspec_mem, 0x20800, NULL },
{ "e_prsr_mem[2]", &tof3_prsr_mem_main_rspec_mem, 0x21000, NULL },
{ "e_prsr_mem[3]", &tof3_prsr_mem_main_rspec_mem, 0x21800, NULL },
{ "e_prsr_mem[4]", &tof3_prsr_mem_main_rspec_mem, 0x22000, NULL },
{ "e_prsr_mem[5]", &tof3_prsr_mem_main_rspec_mem, 0x22800, NULL },
{ "e_prsr_mem[6]", &tof3_prsr_mem_main_rspec_mem, 0x23000, NULL },
{ "e_prsr_mem[7]", &tof3_prsr_mem_main_rspec_mem, 0x23800, NULL },
{ "e_prsr_mem[8]", &tof3_prsr_mem_main_rspec_mem, 0x24000, NULL },
{ "e_prsr_mem[9]", &tof3_prsr_mem_main_rspec_mem, 0x24800, NULL },
{ "e_prsr_mem[10]", &tof3_prsr_mem_main_rspec_mem, 0x25000, NULL },
{ "e_prsr_mem[11]", &tof3_prsr_mem_main_rspec_mem, 0x25800, NULL },
{ "e_prsr_mem[12]", &tof3_prsr_mem_main_rspec_mem, 0x26000, NULL },
{ "e_prsr_mem[13]", &tof3_prsr_mem_main_rspec_mem, 0x26800, NULL },
{ "e_prsr_mem[14]", &tof3_prsr_mem_main_rspec_mem, 0x27000, NULL },
{ "e_prsr_mem[15]", &tof3_prsr_mem_main_rspec_mem, 0x27800, NULL },
{ "e_prsr_mem[16]", &tof3_prsr_mem_main_rspec_mem, 0x28000, NULL },
{ "e_prsr_mem[17]", &tof3_prsr_mem_main_rspec_mem, 0x28800, NULL },
{ "e_prsr_mem[18]", &tof3_prsr_mem_main_rspec_mem, 0x29000, NULL },
{ "e_prsr_mem[19]", &tof3_prsr_mem_main_rspec_mem, 0x29800, NULL },
{ "e_prsr_mem[20]", &tof3_prsr_mem_main_rspec_mem, 0x2a000, NULL },
{ "e_prsr_mem[21]", &tof3_prsr_mem_main_rspec_mem, 0x2a800, NULL },
{ "e_prsr_mem[22]", &tof3_prsr_mem_main_rspec_mem, 0x2b000, NULL },
{ "e_prsr_mem[23]", &tof3_prsr_mem_main_rspec_mem, 0x2b800, NULL },
{ "e_prsr_mem[24]", &tof3_prsr_mem_main_rspec_mem, 0x2c000, NULL },
{ "e_prsr_mem[25]", &tof3_prsr_mem_main_rspec_mem, 0x2c800, NULL },
{ "e_prsr_mem[26]", &tof3_prsr_mem_main_rspec_mem, 0x2d000, NULL },
{ "e_prsr_mem[27]", &tof3_prsr_mem_main_rspec_mem, 0x2d800, NULL },
{ "e_prsr_mem[28]", &tof3_prsr_mem_main_rspec_mem, 0x2e000, NULL },
{ "e_prsr_mem[29]", &tof3_prsr_mem_main_rspec_mem, 0x2e800, NULL },
{ "e_prsr_mem[30]", &tof3_prsr_mem_main_rspec_mem, 0x2f000, NULL },
{ "e_prsr_mem[31]", &tof3_prsr_mem_main_rspec_mem, 0x2f800, NULL },
{ "e_prsr_mem[32]", &tof3_prsr_mem_main_rspec_mem, 0x30000, NULL },
{ "e_prsr_mem[33]", &tof3_prsr_mem_main_rspec_mem, 0x30800, NULL },
{ "e_prsr_mem[34]", &tof3_prsr_mem_main_rspec_mem, 0x31000, NULL },
{ "e_prsr_mem[35]", &tof3_prsr_mem_main_rspec_mem, 0x31800, NULL },
{ "pgr_mem_rspec", &tof3_pgr_mem_rspec_mem, 0x34000, NULL },
};
cmd_arg_t tof3_parde_mem_mem = { 73, tof3_parde_mem_mem_list };

cmd_arg_item_t tof3_pipe_addrmap_mem_list[] = {
{ "parde", &tof3_parde_mem_mem, 0x6080000000, NULL },
};
cmd_arg_t tof3_pipe_addrmap_mem = { 1, tof3_pipe_addrmap_mem_list };

cmd_arg_item_t tof3_cb_mem_mem_list[] = {
{ "tm", &tof3_tm_top_mem_rspec_mem, 0x0, NULL },
{ "pipes[0]", &tof3_pipe_addrmap_mem, 0x20000000000, NULL },
{ "pipes[1]", &tof3_pipe_addrmap_mem, 0x28000000000, NULL },
{ "pipes[2]", &tof3_pipe_addrmap_mem, 0x30000000000, NULL },
{ "pipes[3]", &tof3_pipe_addrmap_mem, 0x38000000000, NULL },
};
cmd_arg_t tof3_cb_mem_mem = { 5, tof3_cb_mem_mem_list };

