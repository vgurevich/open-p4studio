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

reg_decoder_fld_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_port_ppg_mapping_fld_list[] = {
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
reg_decoder_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_port_ppg_mapping = { 6, tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_port_ppg_mapping_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_min_cnt_fld_list[] = {
    { "cnt", 18, 0, 0, 0,
      "19 bit - PPG cell count" },
};
reg_decoder_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_min_cnt = { 1, tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_min_cnt_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_shr_cnt_fld_list[] = {
    { "cnt", 18, 0, 0, 0,
      "19 bit - PPG cell count" },
};
reg_decoder_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_shr_cnt = { 1, tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_shr_cnt_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_hdr_cnt_fld_list[] = {
    { "cnt", 18, 0, 0, 0,
      "19 bit - PPG cell count" },
};
reg_decoder_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_hdr_cnt = { 1, tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_hdr_cnt_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_min_th_fld_list[] = {
    { "cnt", 18, 0, 0, 0,
      "19 bit - PPG cell threshold" },
};
reg_decoder_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_min_th = { 1, tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_min_th_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_shr_th_fld_list[] = {
    { "cnt", 18, 0, 0, 0,
      "19 bit - PPG cell threshold" },
};
reg_decoder_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_shr_th = { 1, tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_shr_th_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_hdr_th_fld_list[] = {
    { "cnt", 18, 0, 0, 0,
      "19 bit - PPG cell threshold" },
};
reg_decoder_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_hdr_th = { 1, tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_hdr_th_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_pfc_fld_list[] = {
    { "pfc", 0, 0, 0, 0,
      "1 bit - Pause state" },
};
reg_decoder_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_pfc = { 1, tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_pfc_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_icos_fld_list[] = {
    { "icos", 7, 0, 0, 0,
      "8 bit - Mapping table used to map PPG to iCoS" },
};
reg_decoder_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_icos = { 1, tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_icos_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_drop_st_fld_list[] = {
    { "drop_st", 0, 0, 0, 0,
      "1 bit - PPG Drop state" },
};
reg_decoder_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_drop_st = { 1, tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_drop_st_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_drop_st_fld_list[] = {
    { "drop_st", 0, 0, 0, 0,
      "1 bit - PG Drop state" },
};
reg_decoder_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_drop_st = { 1, tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_drop_st_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_off_idx_fld_list[] = {
    { "off_idx", 4, 0, 0, 0,
      "5 bit - PPG offset Index" },
    { "dyn", 5, 5, 0, 0,
      "1 bit - Dynamic buffer state" },
};
reg_decoder_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_off_idx = { 2, tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_off_idx_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_off_idx_fld_list[] = {
    { "off_idx", 4, 0, 0, 0,
      "5 bit - PPG offset Index" },
    { "dyn", 5, 5, 0, 0,
      "1 bit - Dynamic buffer state" },
};
reg_decoder_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_off_idx = { 2, tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_off_idx_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_min_cnt_fld_list[] = {
    { "cnt", 18, 0, 0, 0,
      "19 bit - Default PG cell count" },
};
reg_decoder_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_min_cnt = { 1, tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_min_cnt_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_shr_cnt_fld_list[] = {
    { "cnt", 18, 0, 0, 0,
      "19 bit - Default PG cell count" },
};
reg_decoder_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_shr_cnt = { 1, tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_shr_cnt_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_min_th_fld_list[] = {
    { "cnt", 18, 0, 0, 0,
      "19 bit - Default PG configuration" },
};
reg_decoder_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_min_th = { 1, tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_min_th_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_shr_th_fld_list[] = {
    { "cnt", 18, 0, 0, 0,
      "19 bit - Default PG configuration" },
};
reg_decoder_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_shr_th = { 1, tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_shr_th_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_port_shr_th_fld_list[] = {
    { "cnt", 18, 0, 0, 0,
      "19 bit - Port threshold" },
};
reg_decoder_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_port_shr_th = { 1, tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_port_shr_th_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_port_hdr_th_fld_list[] = {
    { "cnt", 18, 0, 0, 0,
      "19 bit - Port threshold" },
};
reg_decoder_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_port_hdr_th = { 1, tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_port_hdr_th_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_port_wm_fld_list[] = {
    { "cnt", 18, 0, 0, 0,
      "19 bit - Port Count" },
};
reg_decoder_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_port_wm = { 1, tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_port_wm_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_port_min_cnt_fld_list[] = {
    { "cnt", 18, 0, 0, 0,
      "19 bit - Port Minimum Count" },
};
reg_decoder_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_port_min_cnt = { 1, tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_port_min_cnt_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_port_hdr_cnt_fld_list[] = {
    { "cnt", 18, 0, 0, 0,
      "19 bit - Port hdr Count" },
};
reg_decoder_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_port_hdr_cnt = { 1, tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_port_hdr_cnt_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_port_shr_cnt_fld_list[] = {
    { "cnt", 18, 0, 0, 0,
      "19 bit - Port shr Count" },
};
reg_decoder_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_port_shr_cnt = { 1, tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_port_shr_cnt_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_port_st_fld_list[] = {
    { "shr_lmt", 0, 0, 0, 0,
      "1 bit - Port shr limit status" },
    { "hdr_lmt", 1, 1, 0, 0,
      "1 bit - Port hdr limit status" },
};
reg_decoder_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_port_st = { 2, tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_port_st_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_wm_cnt_fld_list[] = {
    { "cnt", 18, 0, 0, 0,
      "19 bit - PPG max usage count" },
};
reg_decoder_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_wm_cnt = { 1, tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_wm_cnt_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_drop_count_ppg_fld_list[] = {
    { "cnt", 39, 0, 0, 0,
      "40 bit - Per PG drop count" },
};
reg_decoder_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_drop_count_ppg = { 1, tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_drop_count_ppg_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_drop_count_port_fld_list[] = {
    { "cnt", 39, 0, 0, 0,
      "40 bit - Per Port drop count" },
};
reg_decoder_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_drop_count_port = { 1, tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_drop_count_port_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_pfc_state_fld_list[] = {
    { "port_ppg_state", 7, 0, 0, 0,
      "8 bit - Per ingress pipe port pfc/pause state" },
    { "rm_pfc_state", 15, 8, 0, 0,
      "8 bit - PFC state" },
    { "mac_pfc_out", 23, 16, 0, 0,
      "8 bit - MAC Pfc out state" },
    { "mac_pause_out", 24, 24, 0, 0,
      "1 bit - MAC Pause Out State" },
};
reg_decoder_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_pfc_state = { 4, tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_pfc_state_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_qid_map_fld_list[] = {
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
reg_decoder_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_qid_map = { 16, tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_qid_map_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_qacq_state_fld_list[] = {
    { "nomin", 0, 0, 0, 0,
      "1 bit - Nominal state" },
    { "red_off", 1, 1, 0, 0,
      "1 bit - Red Off" },
    { "yel_off", 2, 2, 0, 0,
      "1 bit - Yellow Off" },
    { "gre_off", 3, 3, 0, 0,
      "1 bit - Green Off" },
};
reg_decoder_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_qacq_state = { 4, tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_qacq_state_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_qacq_ap_config_fld_list[] = {
    { "sp_id", 1, 0, 0, 0,
      "2 bit - Service Pool Id" },
    { "yel_red_drop_en", 2, 2, 0, 0,
      "1 bit - Yellow and Red Queue Tail drop" },
    { "gre_drop_en", 3, 3, 0, 0,
      "1 bit - Green Queue Tail Drop" },
};
reg_decoder_t tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_qacq_ap_config = { 3, tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_qacq_ap_config_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_caa_mem_top_rspec__caa_block_grp0_fld_list[] = {
    { "ptr0", 9, 0, 0, 0,
      "10 bit - CDM block free cell pointer" },
    { "ptr1", 19, 10, 0, 0,
      "10 bit - CDM block free cell pointer" },
};
reg_decoder_t tof2_tm_caa_mem_top_rspec__caa_block_grp0 = { 2, tof2_tm_caa_mem_top_rspec__caa_block_grp0_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_caa_mem_top_rspec__caa_block_grp1_fld_list[] = {
    { "ptr0", 9, 0, 0, 0,
      "10 bit - CDM block free cell pointer" },
    { "ptr1", 19, 10, 0, 0,
      "10 bit - CDM block free cell pointer" },
};
reg_decoder_t tof2_tm_caa_mem_top_rspec__caa_block_grp1 = { 2, tof2_tm_caa_mem_top_rspec__caa_block_grp1_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_caa_mem_top_rspec__caa_block_grp2_fld_list[] = {
    { "ptr0", 9, 0, 0, 0,
      "10 bit - CDM block free cell pointer" },
    { "ptr1", 19, 10, 0, 0,
      "10 bit - CDM block free cell pointer" },
};
reg_decoder_t tof2_tm_caa_mem_top_rspec__caa_block_grp2 = { 2, tof2_tm_caa_mem_top_rspec__caa_block_grp2_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_caa_mem_top_rspec__caa_block_grp3_fld_list[] = {
    { "ptr0", 9, 0, 0, 0,
      "10 bit - CDM block free cell pointer" },
    { "ptr1", 19, 10, 0, 0,
      "10 bit - CDM block free cell pointer" },
};
reg_decoder_t tof2_tm_caa_mem_top_rspec__caa_block_grp3 = { 2, tof2_tm_caa_mem_top_rspec__caa_block_grp3_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_caa_mem_top_rspec__caa_block_grp4_fld_list[] = {
    { "ptr0", 9, 0, 0, 0,
      "10 bit - CDM block free cell pointer" },
    { "ptr1", 19, 10, 0, 0,
      "10 bit - CDM block free cell pointer" },
};
reg_decoder_t tof2_tm_caa_mem_top_rspec__caa_block_grp4 = { 2, tof2_tm_caa_mem_top_rspec__caa_block_grp4_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_caa_mem_top_rspec__caa_block_grp5_fld_list[] = {
    { "ptr0", 9, 0, 0, 0,
      "10 bit - CDM block free cell pointer" },
    { "ptr1", 19, 10, 0, 0,
      "10 bit - CDM block free cell pointer" },
};
reg_decoder_t tof2_tm_caa_mem_top_rspec__caa_block_grp5 = { 2, tof2_tm_caa_mem_top_rspec__caa_block_grp5_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_queue_min_thrd_config_fld_list[] = {
    { "min_lmt", 18, 0, 0, 0,
      "19 bit - Queue min threshold configuration" },
};
reg_decoder_t tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_queue_min_thrd_config = { 1, tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_queue_min_thrd_config_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_queue_shr_thrd_config_fld_list[] = {
    { "shr_lmt", 18, 0, 0, 0,
      " 19 bit - Queue Share threshold configuration " },
    { "offset_idx", 23, 19, 0, 0,
      " 5 bit - Index to offset profile for Queue Resume limit " },
    { "dyn_en", 24, 24, 0, 0,
      " 1 bit - Queue Share limit dynamic mode " },
    { "fast_recover_mode", 25, 25, 0, 0,
      " 1 bit - Fast recovery mode state " },
};
reg_decoder_t tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_queue_shr_thrd_config = { 4, tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_queue_shr_thrd_config_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_queue_ap_config_fld_list[] = {
    { "ap_id", 1, 0, 0, 0,
      "2 bit - Application Service Pool Id" },
    { "q_color_drop_en", 2, 2, 0, 0,
      "1 bit - Queue tail color drop on Yellow/Red packets" },
    { "q_drop_en", 3, 3, 0, 0,
      "1 bit - Queue tail drop on all packets" },
};
reg_decoder_t tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_queue_ap_config = { 3, tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_queue_ap_config_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_queue_color_limit_fld_list[] = {
    { "red_lmt_perc", 3, 0, 0, 0,
      "4 bit - Queue Red limit percentage share" },
    { "red_offset_idx", 7, 4, 0, 0,
      "4 bit - queue Red offset Index" },
    { "yel_lmt_perc", 11, 8, 0, 0,
      "4 bit - Queue Yellow limit percentage share" },
    { "yel_offset_idx", 15, 12, 0, 0,
      "4 bit - queue Yellow offset Index" },
};
reg_decoder_t tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_queue_color_limit = { 4, tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_queue_color_limit_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_queue_cell_count_fld_list[] = {
    { "queue_cell_count", 18, 0, 0, 0,
      "19 bit - Total queue cell usage count" },
};
reg_decoder_t tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_queue_cell_count = { 1, tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_queue_cell_count_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_queue_wm_cell_count_fld_list[] = {
    { "cell_count", 18, 0, 0, 0,
      "19 bit - Queue water mark cell usage count" },
};
reg_decoder_t tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_queue_wm_cell_count = { 1, tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_queue_wm_cell_count_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_port_wm_cell_count_fld_list[] = {
    { "cell_count", 18, 0, 0, 0,
      "19 bit - Port water mark cell usage count" },
};
reg_decoder_t tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_port_wm_cell_count = { 1, tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_port_wm_cell_count_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_port_config_fld_list[] = {
    { "port_thrd", 18, 0, 0, 0,
      "19 bit - Max shared pool buffer usage limit" },
    { "offset_idx", 23, 19, 0, 0,
      "5 bit - Index to offset profile for port threshold" },
};
reg_decoder_t tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_port_config = { 2, tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_port_config_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_port_cell_count_fld_list[] = {
    { "port_cell_count", 18, 0, 0, 0,
      "19 bit - port cell count" },
};
reg_decoder_t tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_port_cell_count = { 1, tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_port_cell_count_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_drop_count_port_fld_list[] = {
    { "count", 39, 0, 0, 0,
      "40 bit - Port packet drop count" },
};
reg_decoder_t tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_drop_count_port = { 1, tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_drop_count_port_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_drop_count_queue_fld_list[] = {
    { "count", 39, 0, 0, 0,
      "40 bit - Queue packet drop count" },
};
reg_decoder_t tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_drop_count_queue = { 1, tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_drop_count_queue_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_qid_mapping_fld_list[] = {
    { "qid_mid0", 4, 0, 0, 0,
      "5 bit - QID Mapping Index for MC Queue" },
    { "qid_mid1", 9, 5, 0, 0,
      "5 bit - QID Mapping Index for MC Queue" },
    { "qid_mid2", 14, 10, 0, 0,
      "5 bit - QID Mapping Index for MC Queue" },
    { "qid_mid3", 19, 15, 0, 0,
      "5 bit - QID Mapping Index for MC Queue" },
    { "qid_mid4", 24, 20, 0, 0,
      "5 bit - QID Mapping Index for MC Queue" },
    { "qid_mid5", 29, 25, 0, 0,
      "5 bit - QID Mapping Index for MC Queue" },
    { "qid_mid6", 34, 30, 0, 0,
      "5 bit - QID Mapping Index for MC Queue" },
    { "qid_mid7", 39, 35, 0, 0,
      "5 bit - QID Mapping Index for MC Queue" },
};
reg_decoder_t tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_qid_mapping = { 8, tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_qid_mapping_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_sch_pipe_mem_rspec__port_max_lb_static_mem_fld_list[] = {
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
reg_decoder_t tof2_tm_sch_pipe_mem_rspec__port_max_lb_static_mem = { 5, tof2_tm_sch_pipe_mem_rspec__port_max_lb_static_mem_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_sch_pipe_mem_rspec__port_max_lb_dynamic_mem_fld_list[] = {
    { "level", 24, 0, 0, 0,
      "25 bit - Bucket Level" },
};
reg_decoder_t tof2_tm_sch_pipe_mem_rspec__port_max_lb_dynamic_mem = { 1, tof2_tm_sch_pipe_mem_rspec__port_max_lb_dynamic_mem_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_sch_pipe_mem_rspec__l1_min_lb_static_mem_fld_list[] = {
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
reg_decoder_t tof2_tm_sch_pipe_mem_rspec__l1_min_lb_static_mem = { 5, tof2_tm_sch_pipe_mem_rspec__l1_min_lb_static_mem_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_sch_pipe_mem_rspec__l1_min_lb_dynamic_mem_fld_list[] = {
    { "level", 24, 0, 0, 0,
      "25 bit - Bucket Level" },
};
reg_decoder_t tof2_tm_sch_pipe_mem_rspec__l1_min_lb_dynamic_mem = { 1, tof2_tm_sch_pipe_mem_rspec__l1_min_lb_dynamic_mem_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_sch_pipe_mem_rspec__l1_max_lb_static_mem_fld_list[] = {
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
reg_decoder_t tof2_tm_sch_pipe_mem_rspec__l1_max_lb_static_mem = { 5, tof2_tm_sch_pipe_mem_rspec__l1_max_lb_static_mem_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_sch_pipe_mem_rspec__l1_max_lb_dynamic_mem_fld_list[] = {
    { "level", 24, 0, 0, 0,
      "25 bit - Bucket Level" },
};
reg_decoder_t tof2_tm_sch_pipe_mem_rspec__l1_max_lb_dynamic_mem = { 1, tof2_tm_sch_pipe_mem_rspec__l1_max_lb_dynamic_mem_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_sch_pipe_mem_rspec__l1_exc_static_mem_fld_list[] = {
    { "pps", 0, 0, 0, 0,
      "1 bit - Used to indicate rate is pps if set to '1' else it is bps" },
    { "wt", 10, 1, 0, 0,
      "10 bit - A 10b weight used for DWRR among the queues at same Max priority level. The value==0 is used to disable the DWRR especially when Max Rate Leakybucket is used" },
};
reg_decoder_t tof2_tm_sch_pipe_mem_rspec__l1_exc_static_mem = { 2, tof2_tm_sch_pipe_mem_rspec__l1_exc_static_mem_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_sch_pipe_mem_rspec__l1_exc_dynamic_mem_fld_list[] = {
    { "level", 20, 0, 0, 0,
      "21 bit - Signed current bucket level in bytes/packets (1b sign + 20b level)" },
};
reg_decoder_t tof2_tm_sch_pipe_mem_rspec__l1_exc_dynamic_mem = { 1, tof2_tm_sch_pipe_mem_rspec__l1_exc_dynamic_mem_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_sch_pipe_mem_rspec__q_min_lb_static_mem_fld_list[] = {
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
reg_decoder_t tof2_tm_sch_pipe_mem_rspec__q_min_lb_static_mem = { 5, tof2_tm_sch_pipe_mem_rspec__q_min_lb_static_mem_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_sch_pipe_mem_rspec__q_min_lb_dynamic_mem_fld_list[] = {
    { "level", 24, 0, 0, 0,
      "25 bit - Signed current bucket level in bytes/packets (1b sign + 24b level)" },
};
reg_decoder_t tof2_tm_sch_pipe_mem_rspec__q_min_lb_dynamic_mem = { 1, tof2_tm_sch_pipe_mem_rspec__q_min_lb_dynamic_mem_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_sch_pipe_mem_rspec__q_max_lb_static_mem_fld_list[] = {
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
reg_decoder_t tof2_tm_sch_pipe_mem_rspec__q_max_lb_static_mem = { 5, tof2_tm_sch_pipe_mem_rspec__q_max_lb_static_mem_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_sch_pipe_mem_rspec__q_max_lb_dynamic_mem_fld_list[] = {
    { "level", 24, 0, 0, 0,
      "25 bit - Signed current bucket level in bytes/packets (1b sign + 24b level)" },
};
reg_decoder_t tof2_tm_sch_pipe_mem_rspec__q_max_lb_dynamic_mem = { 1, tof2_tm_sch_pipe_mem_rspec__q_max_lb_dynamic_mem_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_sch_pipe_mem_rspec__q_exc_static_mem_fld_list[] = {
    { "pps", 0, 0, 0, 0,
      "1 bit - Used to indicate rate is pps if set to '1' else it is bps" },
    { "wt", 10, 1, 0, 0,
      "10 bit - A 10b weight used for DWRR among the queues at same Max priority level. The value==0 is used to disable the DWRR especially when Max Rate Leakybucket is used" },
};
reg_decoder_t tof2_tm_sch_pipe_mem_rspec__q_exc_static_mem = { 2, tof2_tm_sch_pipe_mem_rspec__q_exc_static_mem_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_sch_pipe_mem_rspec__q_exc_dynamic_mem_fld_list[] = {
    { "level", 20, 0, 0, 0,
      "21 bit - Signed current bucket level in bytes/packets (1b sign + 20b level)" },
};
reg_decoder_t tof2_tm_sch_pipe_mem_rspec__q_exc_dynamic_mem = { 1, tof2_tm_sch_pipe_mem_rspec__q_exc_dynamic_mem_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_sch_pipe_mem_rspec__q_pfc_status_mem_fld_list[] = {
    { "pfc_pause", 0, 0, 0, 0,
      "1 bit - Pause state" },
};
reg_decoder_t tof2_tm_sch_pipe_mem_rspec__q_pfc_status_mem = { 1, tof2_tm_sch_pipe_mem_rspec__q_pfc_status_mem_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_sch_pipe_mem_rspec__q_adv_fc_status_mem_fld_list[] = {
    { "adv_fc", 0, 0, 0, 0,
      "1 bit - Flow Control Status" },
};
reg_decoder_t tof2_tm_sch_pipe_mem_rspec__q_adv_fc_status_mem = { 1, tof2_tm_sch_pipe_mem_rspec__q_adv_fc_status_mem_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_sch_pipe_mem_rspec__p_occ_mem_fld_list[] = {
    { "node_occ_cntr", 7, 0, 0, 0,
      "8 bit - Occupancy Counter" },
    { "node_occ", 8, 8, 0, 0,
      "1 bit - Node occupancy indicator" },
};
reg_decoder_t tof2_tm_sch_pipe_mem_rspec__p_occ_mem = { 2, tof2_tm_sch_pipe_mem_rspec__p_occ_mem_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_sch_pipe_mem_rspec__l1_occ_mem_fld_list[] = {
    { "node_occ_cntr", 7, 0, 0, 0,
      "8 bit - Occupancy Counter" },
    { "node_occ", 8, 8, 0, 0,
      "1 bit - Node occupancy indicator" },
};
reg_decoder_t tof2_tm_sch_pipe_mem_rspec__l1_occ_mem = { 2, tof2_tm_sch_pipe_mem_rspec__l1_occ_mem_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_sch_pipe_mem_rspec__q_occ_mem_fld_list[] = {
    { "node_occ_cntr", 3, 0, 0, 0,
      "4 bit - Occupancy Counter" },
    { "node_occ", 4, 4, 0, 0,
      "1 bit - Node occupancy indicator" },
};
reg_decoder_t tof2_tm_sch_pipe_mem_rspec__q_occ_mem = { 2, tof2_tm_sch_pipe_mem_rspec__q_occ_mem_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_clc_pipe_mem_rspec__csr_memory_clc_clm_fld_list[] = {
    { "err", 0, 0, 0, 0,
      "1 bit - Error Indication" },
    { "eop", 1, 1, 0, 0,
      "1 bit - EOP" },
    { "bcnt", 9, 2, 0, 0,
      "8 bit - Counter" },
    { "ptr", 28, 10, 0, 0,
      "19 bit - Pointer" },
};
reg_decoder_t tof2_tm_clc_pipe_mem_rspec__csr_memory_clc_clm = { 4, tof2_tm_clc_pipe_mem_rspec__csr_memory_clc_clm_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_pex_pipe_mem_rspec__csr_memory_pex_clm_fld_list[] = {
    { "err", 0, 0, 0, 0,
      "1 bit - Error Indication" },
    { "eop", 1, 1, 0, 0,
      "1 bit - EOP" },
    { "bcnt", 9, 2, 0, 0,
      "8 bit - Counter" },
    { "ptr", 28, 10, 0, 0,
      "19 bit - Pointer" },
};
reg_decoder_t tof2_tm_pex_pipe_mem_rspec__csr_memory_pex_clm = { 4, tof2_tm_pex_pipe_mem_rspec__csr_memory_pex_clm_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_qlc_pipe_mem_rspec__csr_memory_qlc_qlm_fld_list[] = {
    { "ptr", 18, 0, 0, 0,
      "19 bit - Memory address" },
};
reg_decoder_t tof2_tm_qlc_pipe_mem_rspec__csr_memory_qlc_qlm = { 1, tof2_tm_qlc_pipe_mem_rspec__csr_memory_qlc_qlm_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_qlc_pipe_mem_rspec__csr_memory_qlc_ht_fld_list[] = {
    { "vld", 0, 0, 0, 0,
      "1 bit - Valid or Invalid" },
    { "t", 11, 1, 0, 0,
      "11 bit - Threshold" },
    { "h", 22, 12, 0, 0,
      "11 bit - Heuristics" },
};
reg_decoder_t tof2_tm_qlc_pipe_mem_rspec__csr_memory_qlc_ht = { 3, tof2_tm_qlc_pipe_mem_rspec__csr_memory_qlc_ht_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_qlc_pipe_mem_rspec__csr_memory_qlc_vq_fld_list[] = {
    { "base", 10, 0, 0, 0,
      "11 bit  - Base address" },
    { "cap", 13, 11, 0, 0,
      "3 bit - Capacity" },
    { "h", 16, 14, 0, 0,
      "3 bit - heuristics" },
    { "t", 19, 17, 0, 0,
      "3 bit - Threshold" },
};
reg_decoder_t tof2_tm_qlc_pipe_mem_rspec__csr_memory_qlc_vq = { 4, tof2_tm_qlc_pipe_mem_rspec__csr_memory_qlc_vq_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_prc_pipe_mem_rspec__csr_memory_prc_prm_fld_list[] = {
    { "cnt0", 17, 0, 0, 0,
      "18 bit - counter" },
    { "cnt1", 35, 18, 0, 0,
      "18 bit - counter" },
    { "cnt2", 53, 36, 0, 0,
      "18 bit - counter" },
    { "cnt3", 71, 54, 0, 0,
      "18 bit - counter" },
};
reg_decoder_t tof2_tm_prc_pipe_mem_rspec__csr_memory_prc_prm = { 4, tof2_tm_prc_pipe_mem_rspec__csr_memory_prc_prm_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_prc_pipe_mem_rspec__csr_memory_prc_map_fld_list[] = {
    { "map", 63, 0, 0, 0,
      "64 bit - Address" },
};
reg_decoder_t tof2_tm_prc_pipe_mem_rspec__csr_memory_prc_map = { 1, tof2_tm_prc_pipe_mem_rspec__csr_memory_prc_map_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_prc_pipe_mem_rspec__csr_memory_prc_cache0_fld_list[] = {
    { "cache0", 17, 0, 0, 0,
      "18 bit - Cache" },
    { "cache1", 35, 18, 0, 0,
      "18 bit - Cache" },
    { "cache2", 53, 36, 0, 0,
      "18 bit - Cache" },
    { "cache3", 71, 54, 0, 0,
      "18 bit - Cache" },
};
reg_decoder_t tof2_tm_prc_pipe_mem_rspec__csr_memory_prc_cache0 = { 4, tof2_tm_prc_pipe_mem_rspec__csr_memory_prc_cache0_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_prc_pipe_mem_rspec__csr_memory_prc_cache1_fld_list[] = {
    { "cache0", 17, 0, 0, 0,
      "18 bit - Cache" },
    { "cache1", 35, 18, 0, 0,
      "18 bit - Cache" },
    { "cache2", 53, 36, 0, 0,
      "18 bit - Cache" },
    { "cache3", 71, 54, 0, 0,
      "18 bit - Cache" },
};
reg_decoder_t tof2_tm_prc_pipe_mem_rspec__csr_memory_prc_cache1 = { 4, tof2_tm_prc_pipe_mem_rspec__csr_memory_prc_cache1_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_prc_pipe_mem_rspec__csr_memory_prc_tag_fld_list[] = {
    { "tag", 9, 0, 0, 0,
      "10 bit - Tag" },
};
reg_decoder_t tof2_tm_prc_pipe_mem_rspec__csr_memory_prc_tag = { 1, tof2_tm_prc_pipe_mem_rspec__csr_memory_prc_tag_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_pre_pipe_mem_rspec__mit_mem_word_fld_list[] = {
    { "mem_word", 79, 0, 0, 0,
      "80 bit - MGID memory word" },
};
reg_decoder_t tof2_tm_pre_pipe_mem_rspec__mit_mem_word = { 1, tof2_tm_pre_pipe_mem_rspec__mit_mem_word_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_pre_common_mem_rspec__rdm_mem_word_fld_list[] = {
    { "mem_word", 79, 0, 0, 0,
      "80 bit - RDM memory word" },
};
reg_decoder_t tof2_tm_pre_common_mem_rspec__rdm_mem_word = { 1, tof2_tm_pre_common_mem_rspec__rdm_mem_word_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_pre_common_mem_rspec__pbt0_mem_word_fld_list[] = {
    { "mem_word", 8, 0, 0, 0,
      "9 bit - Memory word" },
};
reg_decoder_t tof2_tm_pre_common_mem_rspec__pbt0_mem_word = { 1, tof2_tm_pre_common_mem_rspec__pbt0_mem_word_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_pre_common_mem_rspec__pbt1_mem_word_fld_list[] = {
    { "mem_word", 8, 0, 0, 0,
      "9 bit - Memory word" },
};
reg_decoder_t tof2_tm_pre_common_mem_rspec__pbt1_mem_word = { 1, tof2_tm_pre_common_mem_rspec__pbt1_mem_word_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_pre_common_mem_rspec__lit0_np_mem_word_fld_list[] = {
    { "mem_word", 25, 0, 0, 0,
      "26 bit - Memory word" },
};
reg_decoder_t tof2_tm_pre_common_mem_rspec__lit0_np_mem_word = { 1, tof2_tm_pre_common_mem_rspec__lit0_np_mem_word_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_pre_common_mem_rspec__lit1_np_mem_word_fld_list[] = {
    { "mem_word", 25, 0, 0, 0,
      "26 bit - Memory word" },
};
reg_decoder_t tof2_tm_pre_common_mem_rspec__lit1_np_mem_word = { 1, tof2_tm_pre_common_mem_rspec__lit1_np_mem_word_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_pre_common_mem_rspec__lit0_bm_mem_word0_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof2_tm_pre_common_mem_rspec__lit0_bm_mem_word0 = { 1, tof2_tm_pre_common_mem_rspec__lit0_bm_mem_word0_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_pre_common_mem_rspec__lit0_bm_mem_word1_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof2_tm_pre_common_mem_rspec__lit0_bm_mem_word1 = { 1, tof2_tm_pre_common_mem_rspec__lit0_bm_mem_word1_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_pre_common_mem_rspec__lit0_bm_mem_word2_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof2_tm_pre_common_mem_rspec__lit0_bm_mem_word2 = { 1, tof2_tm_pre_common_mem_rspec__lit0_bm_mem_word2_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_pre_common_mem_rspec__lit0_bm_mem_word3_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof2_tm_pre_common_mem_rspec__lit0_bm_mem_word3 = { 1, tof2_tm_pre_common_mem_rspec__lit0_bm_mem_word3_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_pre_common_mem_rspec__lit1_bm_mem_word0_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof2_tm_pre_common_mem_rspec__lit1_bm_mem_word0 = { 1, tof2_tm_pre_common_mem_rspec__lit1_bm_mem_word0_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_pre_common_mem_rspec__lit1_bm_mem_word1_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof2_tm_pre_common_mem_rspec__lit1_bm_mem_word1 = { 1, tof2_tm_pre_common_mem_rspec__lit1_bm_mem_word1_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_pre_common_mem_rspec__lit1_bm_mem_word2_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof2_tm_pre_common_mem_rspec__lit1_bm_mem_word2 = { 1, tof2_tm_pre_common_mem_rspec__lit1_bm_mem_word2_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_pre_common_mem_rspec__lit1_bm_mem_word3_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof2_tm_pre_common_mem_rspec__lit1_bm_mem_word3 = { 1, tof2_tm_pre_common_mem_rspec__lit1_bm_mem_word3_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_pre_common_mem_rspec__pmt0_mem_word0_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof2_tm_pre_common_mem_rspec__pmt0_mem_word0 = { 1, tof2_tm_pre_common_mem_rspec__pmt0_mem_word0_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_pre_common_mem_rspec__pmt0_mem_word1_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof2_tm_pre_common_mem_rspec__pmt0_mem_word1 = { 1, tof2_tm_pre_common_mem_rspec__pmt0_mem_word1_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_pre_common_mem_rspec__pmt0_mem_word2_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof2_tm_pre_common_mem_rspec__pmt0_mem_word2 = { 1, tof2_tm_pre_common_mem_rspec__pmt0_mem_word2_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_pre_common_mem_rspec__pmt0_mem_word3_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof2_tm_pre_common_mem_rspec__pmt0_mem_word3 = { 1, tof2_tm_pre_common_mem_rspec__pmt0_mem_word3_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_pre_common_mem_rspec__pmt1_mem_word0_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof2_tm_pre_common_mem_rspec__pmt1_mem_word0 = { 1, tof2_tm_pre_common_mem_rspec__pmt1_mem_word0_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_pre_common_mem_rspec__pmt1_mem_word1_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof2_tm_pre_common_mem_rspec__pmt1_mem_word1 = { 1, tof2_tm_pre_common_mem_rspec__pmt1_mem_word1_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_pre_common_mem_rspec__pmt1_mem_word2_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof2_tm_pre_common_mem_rspec__pmt1_mem_word2 = { 1, tof2_tm_pre_common_mem_rspec__pmt1_mem_word2_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_pre_common_mem_rspec__pmt1_mem_word3_fld_list[] = {
    { "mem_word", 71, 0, 0, 0,
      "72 bit - Memory word" },
};
reg_decoder_t tof2_tm_pre_common_mem_rspec__pmt1_mem_word3 = { 1, tof2_tm_pre_common_mem_rspec__pmt1_mem_word3_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_psc_mem_top_rspec__psc_block_grp0_fld_list[] = {
    { "ptr0", 9, 0, 0, 0,
      "10 bit - Cell Pointer" },
    { "ptr1", 19, 10, 0, 0,
      "10 bit - Cell Pointer" },
};
reg_decoder_t tof2_tm_psc_mem_top_rspec__psc_block_grp0 = { 2, tof2_tm_psc_mem_top_rspec__psc_block_grp0_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_psc_mem_top_rspec__psc_block_grp1_fld_list[] = {
    { "ptr0", 9, 0, 0, 0,
      "10 bit - Cell Pointer" },
    { "ptr1", 19, 10, 0, 0,
      "10 bit - Cell Pointer" },
};
reg_decoder_t tof2_tm_psc_mem_top_rspec__psc_block_grp1 = { 2, tof2_tm_psc_mem_top_rspec__psc_block_grp1_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_psc_mem_top_rspec__psc_block_grp2_fld_list[] = {
    { "ptr0", 9, 0, 0, 0,
      "10 bit - Cell Pointer" },
    { "ptr1", 19, 10, 0, 0,
      "10 bit - Cell Pointer" },
};
reg_decoder_t tof2_tm_psc_mem_top_rspec__psc_block_grp2 = { 2, tof2_tm_psc_mem_top_rspec__psc_block_grp2_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_psc_mem_top_rspec__psc_block_grp3_fld_list[] = {
    { "ptr0", 9, 0, 0, 0,
      "10 bit - Cell Pointer" },
    { "ptr1", 19, 10, 0, 0,
      "10 bit - Cell Pointer" },
};
reg_decoder_t tof2_tm_psc_mem_top_rspec__psc_block_grp3 = { 2, tof2_tm_psc_mem_top_rspec__psc_block_grp3_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_psc_mem_top_rspec__psc_block_grp4_fld_list[] = {
    { "ptr0", 9, 0, 0, 0,
      "10 bit - Cell Pointer" },
    { "ptr1", 19, 10, 0, 0,
      "10 bit - Cell Pointer" },
};
reg_decoder_t tof2_tm_psc_mem_top_rspec__psc_block_grp4 = { 2, tof2_tm_psc_mem_top_rspec__psc_block_grp4_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_tm_psc_mem_top_rspec__psc_block_grp5_fld_list[] = {
    { "ptr0", 9, 0, 0, 0,
      "10 bit - Cell Pointer" },
    { "ptr1", 19, 10, 0, 0,
      "10 bit - Cell Pointer" },
};
reg_decoder_t tof2_tm_psc_mem_top_rspec__psc_block_grp5 = { 2, tof2_tm_psc_mem_top_rspec__psc_block_grp5_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_mau_addrmap__dummy_register_fld_list[] = {
    { "dummy_field0", 63, 0, 0, 0,
      "" },
    { "dummy_field1", 127, 64, 0, 0,
      "" },
};
reg_decoder_t tof2_mau_addrmap__dummy_register = { 2, tof2_mau_addrmap__dummy_register_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_prsr_mem_main_rspec__po_action_row_fld_list[] = {
    { "phv_src[0]", 5, 0, 0, 0,
      "Packet data offset. [code]MSB == 1[/code] sources from the global version number and timestamp." },
    { "phv_src[1]", 11, 6, 0, 0,
      "Packet data offset. [code]MSB == 1[/code] sources from the global version number and timestamp." },
    { "phv_src[2]", 17, 12, 0, 0,
      "Packet data offset. [code]MSB == 1[/code] sources from the global version number and timestamp." },
    { "phv_src[3]", 23, 18, 0, 0,
      "Packet data offset. [code]MSB == 1[/code] sources from the global version number and timestamp." },
    { "phv_src[4]", 29, 24, 0, 0,
      "Packet data offset. [code]MSB == 1[/code] sources from the global version number and timestamp." },
    { "phv_src[5]", 35, 30, 0, 0,
      "Packet data offset. [code]MSB == 1[/code] sources from the global version number and timestamp." },
    { "phv_src[6]", 41, 36, 0, 0,
      "Packet data offset. [code]MSB == 1[/code] sources from the global version number and timestamp." },
    { "phv_src[7]", 47, 42, 0, 0,
      "Packet data offset. [code]MSB == 1[/code] sources from the global version number and timestamp." },
    { "phv_src[8]", 53, 48, 0, 0,
      "Packet data offset. [code]MSB == 1[/code] sources from the global version number and timestamp." },
    { "phv_src[9]", 59, 54, 0, 0,
      "Packet data offset. [code]MSB == 1[/code] sources from the global version number and timestamp." },
    { "phv_src[10]", 65, 60, 0, 0,
      "Packet data offset. [code]MSB == 1[/code] sources from the global version number and timestamp." },
    { "phv_src[11]", 71, 66, 0, 0,
      "Packet data offset. [code]MSB == 1[/code] sources from the global version number and timestamp." },
    { "phv_src[12]", 77, 72, 0, 0,
      "Packet data offset. [code]MSB == 1[/code] sources from the global version number and timestamp." },
    { "phv_src[13]", 83, 78, 0, 0,
      "Packet data offset. [code]MSB == 1[/code] sources from the global version number and timestamp." },
    { "phv_src[14]", 89, 84, 0, 0,
      "Packet data offset. [code]MSB == 1[/code] sources from the global version number and timestamp." },
    { "phv_src[15]", 95, 90, 0, 0,
      "Packet data offset. [code]MSB == 1[/code] sources from the global version number and timestamp." },
    { "phv_src[16]", 101, 96, 0, 0,
      "Packet data offset. [code]MSB == 1[/code] sources from the global version number and timestamp." },
    { "phv_src[17]", 107, 102, 0, 0,
      "Packet data offset. [code]MSB == 1[/code] sources from the global version number and timestamp." },
    { "phv_src[18]", 113, 108, 0, 0,
      "Packet data offset. [code]MSB == 1[/code] sources from the global version number and timestamp." },
    { "phv_src[19]", 119, 114, 0, 0,
      "Packet data offset. [code]MSB == 1[/code] sources from the global version number and timestamp." },
    { "phv_dst[0]", 127, 120, 0, 0,
      "Destination PHV container" },
    { "phv_dst[1]", 135, 128, 0, 0,
      "Destination PHV container" },
    { "phv_dst[2]", 143, 136, 0, 0,
      "Destination PHV container" },
    { "phv_dst[3]", 151, 144, 0, 0,
      "Destination PHV container" },
    { "phv_dst[4]", 159, 152, 0, 0,
      "Destination PHV container" },
    { "phv_dst[5]", 167, 160, 0, 0,
      "Destination PHV container" },
    { "phv_dst[6]", 175, 168, 0, 0,
      "Destination PHV container" },
    { "phv_dst[7]", 183, 176, 0, 0,
      "Destination PHV container" },
    { "phv_dst[8]", 191, 184, 0, 0,
      "Destination PHV container" },
    { "phv_dst[9]", 199, 192, 0, 0,
      "Destination PHV container" },
    { "phv_dst[10]", 207, 200, 0, 0,
      "Destination PHV container" },
    { "phv_dst[11]", 215, 208, 0, 0,
      "Destination PHV container" },
    { "phv_dst[12]", 223, 216, 0, 0,
      "Destination PHV container" },
    { "phv_dst[13]", 231, 224, 0, 0,
      "Destination PHV container" },
    { "phv_dst[14]", 239, 232, 0, 0,
      "Destination PHV container" },
    { "phv_dst[15]", 247, 240, 0, 0,
      "Destination PHV container" },
    { "phv_dst[16]", 255, 248, 0, 0,
      "Destination PHV container" },
    { "phv_dst[17]", 263, 256, 0, 0,
      "Destination PHV container" },
    { "phv_dst[18]", 271, 264, 0, 0,
      "Destination PHV container" },
    { "phv_dst[19]", 279, 272, 0, 0,
      "Destination PHV container" },
    { "phv_offset_add_dst[0]", 280, 280, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "phv_offset_add_dst[1]", 281, 281, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "phv_offset_add_dst[2]", 282, 282, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "phv_offset_add_dst[3]", 283, 283, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "phv_offset_add_dst[4]", 284, 284, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "phv_offset_add_dst[5]", 285, 285, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "phv_offset_add_dst[6]", 286, 286, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "phv_offset_add_dst[7]", 287, 287, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "phv_offset_add_dst[8]", 288, 288, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "phv_offset_add_dst[9]", 289, 289, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "phv_offset_add_dst[10]", 290, 290, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "phv_offset_add_dst[11]", 291, 291, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "phv_offset_add_dst[12]", 292, 292, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "phv_offset_add_dst[13]", 293, 293, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "phv_offset_add_dst[14]", 294, 294, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "phv_offset_add_dst[15]", 295, 295, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "phv_offset_add_dst[16]", 296, 296, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "phv_offset_add_dst[17]", 297, 297, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "phv_offset_add_dst[18]", 298, 298, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "phv_offset_add_dst[19]", 299, 299, 0, 0,
      "Add the current offset to the destination. Alters which [italic]PHV container[/italic] to use as the destination." },
    { "extract_type[0]", 301, 300, 0, 0,
      "Extraction type:[list][*]0: no extraction[*]1: 8b extraction into PHV container LSB[*]2: 8b extraction into PHV container MSB[*]3: 16b extraction into PHV container[/list]" },
    { "extract_type[1]", 303, 302, 0, 0,
      "Extraction type:[list][*]0: no extraction[*]1: 8b extraction into PHV container LSB[*]2: 8b extraction into PHV container MSB[*]3: 16b extraction into PHV container[/list]" },
    { "extract_type[2]", 305, 304, 0, 0,
      "Extraction type:[list][*]0: no extraction[*]1: 8b extraction into PHV container LSB[*]2: 8b extraction into PHV container MSB[*]3: 16b extraction into PHV container[/list]" },
    { "extract_type[3]", 307, 306, 0, 0,
      "Extraction type:[list][*]0: no extraction[*]1: 8b extraction into PHV container LSB[*]2: 8b extraction into PHV container MSB[*]3: 16b extraction into PHV container[/list]" },
    { "extract_type[4]", 309, 308, 0, 0,
      "Extraction type:[list][*]0: no extraction[*]1: 8b extraction into PHV container LSB[*]2: 8b extraction into PHV container MSB[*]3: 16b extraction into PHV container[/list]" },
    { "extract_type[5]", 311, 310, 0, 0,
      "Extraction type:[list][*]0: no extraction[*]1: 8b extraction into PHV container LSB[*]2: 8b extraction into PHV container MSB[*]3: 16b extraction into PHV container[/list]" },
    { "extract_type[6]", 313, 312, 0, 0,
      "Extraction type:[list][*]0: no extraction[*]1: 8b extraction into PHV container LSB[*]2: 8b extraction into PHV container MSB[*]3: 16b extraction into PHV container[/list]" },
    { "extract_type[7]", 315, 314, 0, 0,
      "Extraction type:[list][*]0: no extraction[*]1: 8b extraction into PHV container LSB[*]2: 8b extraction into PHV container MSB[*]3: 16b extraction into PHV container[/list]" },
    { "extract_type[8]", 317, 316, 0, 0,
      "Extraction type:[list][*]0: no extraction[*]1: 8b extraction into PHV container LSB[*]2: 8b extraction into PHV container MSB[*]3: 16b extraction into PHV container[/list]" },
    { "extract_type[9]", 319, 318, 0, 0,
      "Extraction type:[list][*]0: no extraction[*]1: 8b extraction into PHV container LSB[*]2: 8b extraction into PHV container MSB[*]3: 16b extraction into PHV container[/list]" },
    { "extract_type[10]", 321, 320, 0, 0,
      "Extraction type:[list][*]0: no extraction[*]1: 8b extraction into PHV container LSB[*]2: 8b extraction into PHV container MSB[*]3: 16b extraction into PHV container[/list]" },
    { "extract_type[11]", 323, 322, 0, 0,
      "Extraction type:[list][*]0: no extraction[*]1: 8b extraction into PHV container LSB[*]2: 8b extraction into PHV container MSB[*]3: 16b extraction into PHV container[/list]" },
    { "extract_type[12]", 325, 324, 0, 0,
      "Extraction type:[list][*]0: no extraction[*]1: 8b extraction into PHV container LSB[*]2: 8b extraction into PHV container MSB[*]3: 16b extraction into PHV container[/list]" },
    { "extract_type[13]", 327, 326, 0, 0,
      "Extraction type:[list][*]0: no extraction[*]1: 8b extraction into PHV container LSB[*]2: 8b extraction into PHV container MSB[*]3: 16b extraction into PHV container[/list]" },
    { "extract_type[14]", 329, 328, 0, 0,
      "Extraction type:[list][*]0: no extraction[*]1: 8b extraction into PHV container LSB[*]2: 8b extraction into PHV container MSB[*]3: 16b extraction into PHV container[/list]" },
    { "extract_type[15]", 331, 330, 0, 0,
      "Extraction type:[list][*]0: no extraction[*]1: 8b extraction into PHV container LSB[*]2: 8b extraction into PHV container MSB[*]3: 16b extraction into PHV container[/list]" },
    { "extract_type[16]", 333, 332, 0, 0,
      "Extraction type:[list][*]0: no extraction[*]1: 8b extraction into PHV container LSB[*]2: 8b extraction into PHV container MSB[*]3: 16b extraction into PHV container[/list]" },
    { "extract_type[17]", 335, 334, 0, 0,
      "Extraction type:[list][*]0: no extraction[*]1: 8b extraction into PHV container LSB[*]2: 8b extraction into PHV container MSB[*]3: 16b extraction into PHV container[/list]" },
    { "extract_type[18]", 337, 336, 0, 0,
      "Extraction type:[list][*]0: no extraction[*]1: 8b extraction into PHV container LSB[*]2: 8b extraction into PHV container MSB[*]3: 16b extraction into PHV container[/list]" },
    { "extract_type[19]", 339, 338, 0, 0,
      "Extraction type:[list][*]0: no extraction[*]1: 8b extraction into PHV container LSB[*]2: 8b extraction into PHV container MSB[*]3: 16b extraction into PHV container[/list]" },
    { "csum_addr[0]", 344, 340, 0, 0,
      "" },
    { "csum_addr[1]", 349, 345, 0, 0,
      "" },
    { "csum_addr[2]", 354, 350, 0, 0,
      "" },
    { "csum_addr[3]", 359, 355, 0, 0,
      "" },
    { "csum_addr[4]", 364, 360, 0, 0,
      "" },
    { "csum_en[0]", 365, 365, 0, 0,
      "" },
    { "csum_en[1]", 366, 366, 0, 0,
      "" },
    { "csum_en[2]", 367, 367, 0, 0,
      "" },
    { "csum_en[3]", 368, 368, 0, 0,
      "" },
    { "csum_en[4]", 369, 369, 0, 0,
      "" },
    { "dst_offset_inc", 374, 370, 0, 0,
      "Increment the offset by this amount or, if jbay_mem.pipes.parde.i_prsr_mem.po_action_row.dst_offset_rst is asserted, set the offset to this amount.The offset is updated [italic]after[/italic] the current offset is added to the destination addresses in this action entry." },
    { "dst_offset_rst", 375, 375, 0, 0,
      "Reset the destination offset before adding the value contained in jbay_mem.pipes.parde.i_prsr_mem.po_action_row.dst_offset_rst.The offset is updated [italic]after[/italic] the current offset is added to the destination addresses in this action entry." },
    { "pri_upd_type", 376, 376, 0, 0,
      "Priority update type: [list][*]0: immediate value supplied as source (see jbay_mem.pipes.parde.i_prsr_mem.po_action_row.pri_upd_val_mask and jbay_mem.pipes.parde.i_prsr_mem.po_action_row.pri_upd_en_shr)[*]1: use packet data as source (see jbay_mem.pipes.parde.i_prsr_mem.po_action_row.pri_upd_src, jbay_mem.pipes.parde.i_prsr_mem.po_action_row.pri_upd_en_shr and jbay_mem.pipes.parde.i_prsr_mem.po_action_row.pri_upd_val_mask)[/list]" },
    { "pri_upd_src", 381, 377, 0, 0,
      "Packet data offset. Can not access the global version number or timestamp." },
    { "pri_upd_en_shr", 385, 382, 0, 0,
      "Priority update enable or right-shift amount. Determined by the update type (jbay_mem.pipes.parde.i_prsr_mem.po_action_row.pri_upd_type):[list][*]0: Update priority if [code]jbay_mem.pipes.parde.i_prsr_mem.po_action_row.pri_upd_en_shr[0] == 1[/code][*]1: Right-shift the 16b value at jbay_mem.pipes.parde.i_prsr_mem.po_action_row.pri_upd_src. Priority is the three LSBs of the shifted 16b value after masking[/list]" },
    { "pri_upd_val_mask", 388, 386, 0, 0,
      "Priority value or mask, determined by jbay_mem.pipes.parde.i_prsr_mem.po_action_row.pri_upd_type:[list][*]0: immediate value to set as the priority[*]1: mask to apply to the extracted and shifted value[/list]" },
    { "ver_upd_type", 389, 389, 0, 0,
      "Version update type: [list][*]0: immediate value supplied as source (see jbay_mem.pipes.parde.i_prsr_mem.po_action_row.ver_upd_val_mask and jbay_mem.pipes.parde.i_prsr_mem.po_action_row.ver_upd_en_shr)[*]1: use packet data as source (see jbay_mem.pipes.parde.i_prsr_mem.po_action_row.ver_upd_src, jbay_mem.pipes.parde.i_prsr_mem.po_action_row.ver_upd_en_shr and jbay_mem.pipes.parde.i_prsr_mem.po_action_row.ver_upd_val_mask)[/list]" },
    { "ver_upd_src", 394, 390, 0, 0,
      "Packet data offset. Can not access the global version number or timestamp." },
    { "ver_upd_en_shr", 398, 395, 0, 0,
      "Version update enable or right-shift amount. Determined by the update type:[list][*]0: Version update (determined by LSB)[*]1: Right-shift the 16b value at jbay_mem.pipes.parde.i_prsr_mem.po_action_row.ver_upd_src. Version is the two LSBs of the shifted 16b value after masking[/list]" },
    { "ver_upd_val_mask", 400, 399, 0, 0,
      "Version value or mask, determined by jbay_mem.pipes.parde.i_prsr_mem.po_action_row.ver_upd_type:[list][*]0: immediate value to set as the priority[*]1: mask to apply to the extracted and shifted value[/list]" },
    { "val_const[0]", 416, 401, 0, 0,
      "16b value constant" },
    { "val_const[1]", 432, 417, 0, 0,
      "16b value constant" },
    { "val_const_rot[0]", 433, 433, 0, 0,
      "Right-rotate the value constant by offset before PHV extraction." },
    { "val_const_rot[1]", 434, 434, 0, 0,
      "Right-rotate the value constant by offset before PHV extraction." },
    { "val_const_32b_bond", 435, 435, 0, 0,
      "Treat the two 16b value constants as a single 32b value when performing a constant rotate." },
    { "clot_type[0]", 436, 436, 0, 0,
      "Length source: [list][*]0: length supplied as immediate value[*]1: generate CLOT, extracting length from packet data[/list]" },
    { "clot_type[1]", 437, 437, 0, 0,
      "Length source: [list][*]0: length supplied as immediate value[*]1: generate CLOT, extracting length from packet data[/list]" },
    { "clot_len_src[0]", 443, 438, 0, 0,
      "CLOT length or length source address, as determined by jbay_mem.pipes.parde.i_prsr_mem.po_action_row.clot_type: [list][*]0: length - 1 (e.g., a true length of 4 is be encoded as 3); supports true lengths of 1-64B[*]1: 5 LSBs specify the address of the 16b value to extract[/list]" },
    { "clot_len_src[1]", 449, 444, 0, 0,
      "CLOT length or length source address, as determined by jbay_mem.pipes.parde.i_prsr_mem.po_action_row.clot_type: [list][*]0: length - 1 (e.g., a true length of 4 is be encoded as 3); supports true lengths of 1-64B[*]1: 5 LSBs specify the address of the 16b value to extract[/list]" },
    { "clot_en_len_shr[0]", 453, 450, 0, 0,
      "CLOT enable or length right-shift amount. Determined by the type (jbay_mem.pipes.parde.i_prsr_mem.po_action_row.clot_type):[list][*]0: CLOT output enable (bit 0)[*]1: Right-shift the 16b value at jbay_mem.pipes.parde.i_prsr_mem.po_action_row.clot_len_src. Length is the 6 LSBs of the shifted 16b value after masking[/list]" },
    { "clot_en_len_shr[1]", 457, 454, 0, 0,
      "CLOT enable or length right-shift amount. Determined by the type (jbay_mem.pipes.parde.i_prsr_mem.po_action_row.clot_type):[list][*]0: CLOT output enable (bit 0)[*]1: Right-shift the 16b value at jbay_mem.pipes.parde.i_prsr_mem.po_action_row.clot_len_src. Length is the 6 LSBs of the shifted 16b value after masking[/list]" },
    { "clot_len_mask[0]", 463, 458, 0, 0,
      "Mask to apply to extracted length (after right-shift)." },
    { "clot_len_mask[1]", 469, 464, 0, 0,
      "Mask to apply to extracted length (after right-shift)." },
    { "clot_len_add", 475, 470, 0, 0,
      "Amount to [italic]add[/italic] to the length of the first CLOT. Only the first CLOT supports the add; the second does not.[p]The addition operation is unsigned and overflow is ignored. Subtraction is peformed by treating this field as a two's complement value. For example, subtracting 1 is achieved by setting this field to [code]6'b111111[/code] (the 6b two's complement value for -1). This results in 63 being added, but because overflow is ignored, the final result is the same as subtracting 1." },
    { "clot_offset[0]", 480, 476, 0, 0,
      "CLOT offset within the current packet data." },
    { "clot_offset[1]", 485, 481, 0, 0,
      "CLOT offset within the current packet data." },
    { "clot_tag[0]", 491, 486, 0, 0,
      "Tag value for the CLOT" },
    { "clot_tag[1]", 497, 492, 0, 0,
      "Tag value for the CLOT" },
    { "clot_tag_offset_add[0]", 498, 498, 0, 0,
      "Add the current offset to the CLOT tag." },
    { "clot_tag_offset_add[1]", 499, 499, 0, 0,
      "Add the current offset to the CLOT tag." },
    { "clot_has_csum[0]", 500, 500, 0, 0,
      "The CLOT has a checksum (wait for output from the checksum engines)." },
    { "clot_has_csum[1]", 501, 501, 0, 0,
      "The CLOT has a checksum (wait for output from the checksum engines)." },
    { "hdr_len_inc_stop", 502, 502, 0, 0,
      "Stop incrementing the header length count. (Adds jbay_mem.pipes.parde.i_prsr_mem.po_action_row.hdr_len_inc_final_amt to the length before stopping.)" },
    { "hdr_len_inc_final_amt", 508, 503, 0, 0,
      "Amount to add to the header length count in the cycle when jbay_mem.pipes.parde.i_prsr_mem.po_action_row.hdr_len_inc_stop is asserted." },
    { "disable_partial_hdr_err", 509, 509, 0, 0,
      "Disable partial header error reporting during [italic]this[/italic] cycle." },
};
reg_decoder_t tof2_prsr_mem_main_rspec__po_action_row = { 125, tof2_prsr_mem_main_rspec__po_action_row_fld_list, 512 /* bits */, 0 };

reg_decoder_fld_t tof2_prsr_mem_main_rspec__ml_tcam_row_fld_list[] = {
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
reg_decoder_t tof2_prsr_mem_main_rspec__ml_tcam_row = { 18, tof2_prsr_mem_main_rspec__ml_tcam_row_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_prsr_mem_main_rspec__ml_ea_row_fld_list[] = {
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
      "This is the last header to be processed. Advance to the next packet.Note: jbay_mem.pipes.parde.i_prsr_mem.ml_ea_row.shift_amt must specify the number of header bytes in the current word to identify the payload location." },
    { "ctr_op", 62, 61, 0, 0,
      "Counter operation to perform:[list][*]0: Add jbay_mem.pipes.parde.i_prsr_mem.ml_ea_row.ctr_amt_idx to the current value.[*]1: Load the counter from the stack (pop) and add jbay_mem.pipes.parde.i_prsr_mem.ml_ea_row.ctr_amt_idx[*]2: Load immediate value from the jbay_mem.pipes.parde.i_prsr_mem.ml_ea_row.ctr_amt_idx field.[*]3: Load counter from CounterRAM[jbay_mem.pipes.parde.i_prsr_mem.ml_ea_row.ctr_amt_idx][/list]" },
    { "ctr_amt_idx", 70, 63, 0, 0,
      "Counter increment or load value:[list][*]jbay_mem.pipes.parde.i_prsr_mem.ml_ea_row.ctr_op= 0 or 1: Immediate value to increment the counter by.[*]jbay_mem.pipes.parde.i_prsr_mem.ml_ea_row.ctr_op= 2: Immediate value to load into the counter.[*]jbay_mem.pipes.parde.i_prsr_mem.ml_ea_row.ctr_op= 3: Counter initialization RAM entry to load.[/list]" },
    { "ctr_stack_push", 71, 71, 0, 0,
      "Push the current counter value onto the stack. Valid only when jbay_mem.pipes.parde.i_prsr_mem.ml_ea_row.ctr_op is 2 or 3." },
    { "ctr_stack_upd_w_top", 72, 72, 0, 0,
      "Update the pushed value when the top-of-stack value is updated. Valid only when a value is being pushed onto the stack." },
};
reg_decoder_t tof2_prsr_mem_main_rspec__ml_ea_row = { 21, tof2_prsr_mem_main_rspec__ml_ea_row_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_prsr_mem_main_rspec__ml_ctr_init_ram_fld_list[] = {
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
};
reg_decoder_t tof2_prsr_mem_main_rspec__ml_ctr_init_ram = { 5, tof2_prsr_mem_main_rspec__ml_ctr_init_ram_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_prsr_mem_main_rspec__po_csum_ctrl_0_row_fld_list[] = {
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
      "[list][*]VERIFY: indicates the bit within the destination PHV to record the validity of the checksum.Note: [code]target_bit_position = bit_position % container_size[/code].[*]RESIDUAL: indicates where the header ends in the current packet word. Any headers after the current header are automatically subtracted from the residual.[*]CLOT: Unused.[/list]Valid only when jbay_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_0_row.hdr_end is asserted." },
    { "dst", 78, 71, 0, 0,
      "Destination PHV field or tag.Valid only when jbay_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_0_row.hdr_end is asserted." },
    { "hdr_end", 79, 79, 0, 0,
      "The header containing the checksum ends in this packet word.[list][*]0: Additional words to come[*]1: Final word of header. For VERIFY checksums, verify that the checksum is correct by comparing to zero. For RESIDUAL checksums, automatically subtract any headers that follow.[/list]" },
    { "type", 81, 80, 0, 0,
      "Checksum calculation type:[list][*]0: Verification -- Verify that a checksum is valid. A final sum of zero indicates a valid checksum.[*]1: Residual -- Calculate the residual of the checksum, i.e., the contribution of [italic]non-header[/italic] bytes of the packet.[*]2: CLOT -- Calculate the checksum for CLOT data. Only valid for checksum units 2-4; checksum output ignored when used for units 0 and 1.[/list]Sampled when jbay_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_0_row.start == 1." },
    { "start", 82, 82, 0, 0,
      "Start a new checksum calculation and clear any previously calculated value" },
    { "zeros_as_ones", 83, 83, 0, 0,
      "The current data word contains a checksum; an all-zeros checksum is encoded as all-ones. This is used by certain protocols, such as UDP, which use all zeros to indicate the checksum is not used. Indicates that an all-ones value at jbay_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_0_row.zeros_as_ones_pos should be converted to all-zeros." },
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
reg_decoder_t tof2_prsr_mem_main_rspec__po_csum_ctrl_0_row = { 75, tof2_prsr_mem_main_rspec__po_csum_ctrl_0_row_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_prsr_mem_main_rspec__po_csum_ctrl_1_row_fld_list[] = {
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
      "[list][*]VERIFY: indicates the bit within the destination PHV to record the validity of the checksum.Note: [code]target_bit_position = bit_position % container_size[/code].[*]RESIDUAL: indicates where the header ends in the current packet word. Any headers after the current header are automatically subtracted from the residual.[*]CLOT: Unused.[/list]Valid only when jbay_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_1_row.hdr_end is asserted." },
    { "dst", 78, 71, 0, 0,
      "Destination PHV field or tag.Valid only when jbay_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_1_row.hdr_end is asserted." },
    { "hdr_end", 79, 79, 0, 0,
      "The header containing the checksum ends in this packet word.[list][*]0: Additional words to come[*]1: Final word of header. For VERIFY checksums, verify that the checksum is correct by comparing to zero. For RESIDUAL checksums, automatically subtract any headers that follow.[/list]" },
    { "type", 81, 80, 0, 0,
      "Checksum calculation type:[list][*]0: Verification -- Verify that a checksum is valid. A final sum of zero indicates a valid checksum.[*]1: Residual -- Calculate the residual of the checksum, i.e., the contribution of [italic]non-header[/italic] bytes of the packet.[*]2: CLOT -- Calculate the checksum for CLOT data. Only valid for checksum units 2-4; checksum output ignored when used for units 0 and 1.[/list]Sampled when jbay_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_1_row.start == 1." },
    { "start", 82, 82, 0, 0,
      "Start a new checksum calculation and clear any previously calculated value" },
    { "zeros_as_ones", 83, 83, 0, 0,
      "The current data word contains a checksum; an all-zeros checksum is encoded as all-ones. This is used by certain protocols, such as UDP, which use all zeros to indicate the checksum is not used. Indicates that an all-ones value at jbay_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_1_row.zeros_as_ones_pos should be converted to all-zeros." },
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
reg_decoder_t tof2_prsr_mem_main_rspec__po_csum_ctrl_1_row = { 75, tof2_prsr_mem_main_rspec__po_csum_ctrl_1_row_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_prsr_mem_main_rspec__po_csum_ctrl_2_row_fld_list[] = {
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
      "[list][*]VERIFY: indicates the bit within the destination PHV to record the validity of the checksum.Note: [code]target_bit_position = bit_position % container_size[/code].[*]RESIDUAL: indicates where the header ends in the current packet word. Any headers after the current header are automatically subtracted from the residual.[*]CLOT: Unused.[/list]Valid only when jbay_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_2_row.hdr_end is asserted." },
    { "dst", 78, 71, 0, 0,
      "Destination PHV field or tag.Valid only when jbay_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_2_row.hdr_end is asserted." },
    { "hdr_end", 79, 79, 0, 0,
      "The header containing the checksum ends in this packet word.[list][*]0: Additional words to come[*]1: Final word of header. For VERIFY checksums, verify that the checksum is correct by comparing to zero. For RESIDUAL checksums, automatically subtract any headers that follow.[/list]" },
    { "type", 81, 80, 0, 0,
      "Checksum calculation type:[list][*]0: Verification -- Verify that a checksum is valid. A final sum of zero indicates a valid checksum.[*]1: Residual -- Calculate the residual of the checksum, i.e., the contribution of [italic]non-header[/italic] bytes of the packet.[*]2: CLOT -- Calculate the checksum for CLOT data. Only valid for checksum units 2-4; checksum output ignored when used for units 0 and 1.[/list]Sampled when jbay_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_2_row.start == 1." },
    { "start", 82, 82, 0, 0,
      "Start a new checksum calculation and clear any previously calculated value" },
    { "zeros_as_ones", 83, 83, 0, 0,
      "The current data word contains a checksum; an all-zeros checksum is encoded as all-ones. This is used by certain protocols, such as UDP, which use all zeros to indicate the checksum is not used. Indicates that an all-ones value at jbay_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_2_row.zeros_as_ones_pos should be converted to all-zeros." },
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
reg_decoder_t tof2_prsr_mem_main_rspec__po_csum_ctrl_2_row = { 75, tof2_prsr_mem_main_rspec__po_csum_ctrl_2_row_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_prsr_mem_main_rspec__po_csum_ctrl_3_row_fld_list[] = {
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
      "[list][*]VERIFY: indicates the bit within the destination PHV to record the validity of the checksum.Note: [code]target_bit_position = bit_position % container_size[/code].[*]RESIDUAL: indicates where the header ends in the current packet word. Any headers after the current header are automatically subtracted from the residual.[*]CLOT: Unused.[/list]Valid only when jbay_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_3_row.hdr_end is asserted." },
    { "dst", 78, 71, 0, 0,
      "Destination PHV field or tag.Valid only when jbay_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_3_row.hdr_end is asserted." },
    { "hdr_end", 79, 79, 0, 0,
      "The header containing the checksum ends in this packet word.[list][*]0: Additional words to come[*]1: Final word of header. For VERIFY checksums, verify that the checksum is correct by comparing to zero. For RESIDUAL checksums, automatically subtract any headers that follow.[/list]" },
    { "type", 81, 80, 0, 0,
      "Checksum calculation type:[list][*]0: Verification -- Verify that a checksum is valid. A final sum of zero indicates a valid checksum.[*]1: Residual -- Calculate the residual of the checksum, i.e., the contribution of [italic]non-header[/italic] bytes of the packet.[*]2: CLOT -- Calculate the checksum for CLOT data. Only valid for checksum units 2-4; checksum output ignored when used for units 0 and 1.[/list]Sampled when jbay_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_3_row.start == 1." },
    { "start", 82, 82, 0, 0,
      "Start a new checksum calculation and clear any previously calculated value" },
    { "zeros_as_ones", 83, 83, 0, 0,
      "The current data word contains a checksum; an all-zeros checksum is encoded as all-ones. This is used by certain protocols, such as UDP, which use all zeros to indicate the checksum is not used. Indicates that an all-ones value at jbay_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_3_row.zeros_as_ones_pos should be converted to all-zeros." },
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
reg_decoder_t tof2_prsr_mem_main_rspec__po_csum_ctrl_3_row = { 75, tof2_prsr_mem_main_rspec__po_csum_ctrl_3_row_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_prsr_mem_main_rspec__po_csum_ctrl_4_row_fld_list[] = {
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
      "[list][*]VERIFY: indicates the bit within the destination PHV to record the validity of the checksum.Note: [code]target_bit_position = bit_position % container_size[/code].[*]RESIDUAL: indicates where the header ends in the current packet word. Any headers after the current header are automatically subtracted from the residual.[*]CLOT: Unused.[/list]Valid only when jbay_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_4_row.hdr_end is asserted." },
    { "dst", 78, 71, 0, 0,
      "Destination PHV field or tag.Valid only when jbay_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_4_row.hdr_end is asserted." },
    { "hdr_end", 79, 79, 0, 0,
      "The header containing the checksum ends in this packet word.[list][*]0: Additional words to come[*]1: Final word of header. For VERIFY checksums, verify that the checksum is correct by comparing to zero. For RESIDUAL checksums, automatically subtract any headers that follow.[/list]" },
    { "type", 81, 80, 0, 0,
      "Checksum calculation type:[list][*]0: Verification -- Verify that a checksum is valid. A final sum of zero indicates a valid checksum.[*]1: Residual -- Calculate the residual of the checksum, i.e., the contribution of [italic]non-header[/italic] bytes of the packet.[*]2: CLOT -- Calculate the checksum for CLOT data. Only valid for checksum units 2-4; checksum output ignored when used for units 0 and 1.[/list]Sampled when jbay_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_4_row.start == 1." },
    { "start", 82, 82, 0, 0,
      "Start a new checksum calculation and clear any previously calculated value" },
    { "zeros_as_ones", 83, 83, 0, 0,
      "The current data word contains a checksum; an all-zeros checksum is encoded as all-ones. This is used by certain protocols, such as UDP, which use all zeros to indicate the checksum is not used. Indicates that an all-ones value at jbay_mem.pipes.parde.i_prsr_mem.po_csum_ctrl_4_row.zeros_as_ones_pos should be converted to all-zeros." },
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
reg_decoder_t tof2_prsr_mem_main_rspec__po_csum_ctrl_4_row = { 75, tof2_prsr_mem_main_rspec__po_csum_ctrl_4_row_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_pgr_mem_rspec__buffer_mem_word_fld_list[] = {
    { "mem_word", 127, 0, 0, 0,
      "" },
};
reg_decoder_t tof2_pgr_mem_rspec__buffer_mem_word = { 1, tof2_pgr_mem_rspec__buffer_mem_word_fld_list, 128 /* bits */, 0 };

reg_decoder_fld_t tof2_pgr_ph0_rspec__phase0_mem_word_fld_list[] = {
    { "mem_word", 127, 0, 0, 0,
      "" },
};
reg_decoder_t tof2_pgr_ph0_rspec__phase0_mem_word = { 1, tof2_pgr_ph0_rspec__phase0_mem_word_fld_list, 128 /* bits */, 0 };

cmd_arg_item_t tof2_tm_wac_pipe_mem_rspec_mem_list[] = {
{ "csr_memory_wac_port_ppg_mapping", NULL, 0x0, &tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_port_ppg_mapping, 0x1240 },
{ "csr_memory_wac_ppg_min_cnt", NULL, 0x180, &tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_min_cnt, 0x800 },
{ "csr_memory_wac_ppg_shr_cnt", NULL, 0x200, &tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_shr_cnt, 0x800 },
{ "csr_memory_wac_ppg_hdr_cnt", NULL, 0x280, &tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_hdr_cnt, 0x800 },
{ "csr_memory_wac_ppg_min_th", NULL, 0x300, &tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_min_th, 0x800 },
{ "csr_memory_wac_ppg_shr_th", NULL, 0x380, &tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_shr_th, 0x800 },
{ "csr_memory_wac_ppg_hdr_th", NULL, 0x400, &tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_hdr_th, 0x800 },
{ "csr_memory_wac_ppg_pfc", NULL, 0x480, &tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_pfc, 0x800 },
{ "csr_memory_wac_ppg_icos", NULL, 0x500, &tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_icos, 0x800 },
{ "csr_memory_wac_ppg_drop_st", NULL, 0x580, &tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_drop_st, 0x800 },
{ "csr_memory_wac_pg_drop_st", NULL, 0x600, &tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_drop_st, 0x490 },
{ "csr_memory_wac_ppg_off_idx", NULL, 0x680, &tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_ppg_off_idx, 0x800 },
{ "csr_memory_wac_pg_off_idx", NULL, 0x700, &tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_off_idx, 0x490 },
{ "csr_memory_wac_pg_min_cnt", NULL, 0x780, &tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_min_cnt, 0x490 },
{ "csr_memory_wac_pg_shr_cnt", NULL, 0x800, &tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_shr_cnt, 0x490 },
{ "csr_memory_wac_pg_min_th", NULL, 0x880, &tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_min_th, 0x490 },
{ "csr_memory_wac_pg_shr_th", NULL, 0x900, &tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_shr_th, 0x490 },
{ "csr_memory_wac_port_shr_th", NULL, 0x980, &tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_port_shr_th, 0x490 },
{ "csr_memory_wac_port_hdr_th", NULL, 0xa00, &tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_port_hdr_th, 0x490 },
{ "csr_memory_wac_port_wm", NULL, 0xa80, &tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_port_wm, 0x490 },
{ "csr_memory_wac_port_min_cnt", NULL, 0xb00, &tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_port_min_cnt, 0x490 },
{ "csr_memory_wac_port_hdr_cnt", NULL, 0xb80, &tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_port_hdr_cnt, 0x490 },
{ "csr_memory_wac_port_shr_cnt", NULL, 0xc00, &tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_port_shr_cnt, 0x490 },
{ "csr_memory_wac_port_st", NULL, 0xc80, &tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_port_st, 0x490 },
{ "csr_memory_wac_pg_wm_cnt", NULL, 0xd00, &tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_pg_wm_cnt, 0xc90 },
{ "csr_memory_wac_drop_count_ppg", NULL, 0xe00, &tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_drop_count_ppg, 0xc90 },
{ "csr_memory_wac_drop_count_port", NULL, 0xf00, &tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_drop_count_port, 0x490 },
{ "csr_memory_wac_pfc_state", NULL, 0xf80, &tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_pfc_state, 0x480 },
{ "csr_memory_wac_qid_map", NULL, 0x1000, &tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_qid_map, 0x2400 },
{ "csr_memory_wac_qacq_state", NULL, 0x1800, &tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_qacq_state, 0x4800 },
{ "csr_memory_wac_qacq_ap_config", NULL, 0x2000, &tof2_tm_wac_pipe_mem_rspec__csr_memory_wac_qacq_ap_config, 0x4800 },
};
cmd_arg_t tof2_tm_wac_pipe_mem_rspec_mem = { 31, tof2_tm_wac_pipe_mem_rspec_mem_list };

cmd_arg_item_t tof2_tm_wac_mem_top_rspec_mem_list[] = {
{ "wac_pipe_mem[0]", &tof2_tm_wac_pipe_mem_rspec_mem, 0x0, NULL },
{ "wac_pipe_mem[1]", &tof2_tm_wac_pipe_mem_rspec_mem, 0x10000000, NULL },
{ "wac_pipe_mem[2]", &tof2_tm_wac_pipe_mem_rspec_mem, 0x20000000, NULL },
{ "wac_pipe_mem[3]", &tof2_tm_wac_pipe_mem_rspec_mem, 0x30000000, NULL },
};
cmd_arg_t tof2_tm_wac_mem_top_rspec_mem = { 4, tof2_tm_wac_mem_top_rspec_mem_list };

cmd_arg_item_t tof2_tm_caa_mem_top_rspec_mem_list[] = {
{ "caa_block_grp0", NULL, 0x0, &tof2_tm_caa_mem_top_rspec__caa_block_grp0, 0x80000 },
{ "caa_block_grp1", NULL, 0x8000, &tof2_tm_caa_mem_top_rspec__caa_block_grp1, 0x80000 },
{ "caa_block_grp2", NULL, 0x10000, &tof2_tm_caa_mem_top_rspec__caa_block_grp2, 0x80000 },
{ "caa_block_grp3", NULL, 0x18000, &tof2_tm_caa_mem_top_rspec__caa_block_grp3, 0x80000 },
{ "caa_block_grp4", NULL, 0x20000, &tof2_tm_caa_mem_top_rspec__caa_block_grp4, 0x80000 },
{ "caa_block_grp5", NULL, 0x28000, &tof2_tm_caa_mem_top_rspec__caa_block_grp5, 0x80000 },
};
cmd_arg_t tof2_tm_caa_mem_top_rspec_mem = { 6, tof2_tm_caa_mem_top_rspec_mem_list };

cmd_arg_item_t tof2_tm_qac_pipe_mem_rspec_mem_list[] = {
{ "csr_memory_qac_queue_min_thrd_config", NULL, 0x0, &tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_queue_min_thrd_config, 0x4800 },
{ "csr_memory_qac_queue_shr_thrd_config", NULL, 0x800, &tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_queue_shr_thrd_config, 0x4800 },
{ "csr_memory_qac_queue_ap_config", NULL, 0x1000, &tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_queue_ap_config, 0x4800 },
{ "csr_memory_qac_queue_color_limit", NULL, 0x1800, &tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_queue_color_limit, 0x4800 },
{ "csr_memory_qac_queue_cell_count", NULL, 0x2000, &tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_queue_cell_count, 0x4800 },
{ "csr_memory_qac_queue_wm_cell_count", NULL, 0x2800, &tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_queue_wm_cell_count, 0x4800 },
{ "csr_memory_qac_port_wm_cell_count", NULL, 0x2c80, &tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_port_wm_cell_count, 0x480 },
{ "csr_memory_qac_port_config", NULL, 0x2d00, &tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_port_config, 0x480 },
{ "csr_memory_qac_port_cell_count", NULL, 0x2d80, &tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_port_cell_count, 0x480 },
{ "csr_memory_qac_drop_count_port", NULL, 0x2e00, &tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_drop_count_port, 0xd80 },
{ "csr_memory_qac_drop_count_queue", NULL, 0x3000, &tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_drop_count_queue, 0x4800 },
{ "csr_memory_qac_qid_mapping", NULL, 0x3600, &tof2_tm_qac_pipe_mem_rspec__csr_memory_qac_qid_mapping, 0x1200 },
};
cmd_arg_t tof2_tm_qac_pipe_mem_rspec_mem = { 12, tof2_tm_qac_pipe_mem_rspec_mem_list };

cmd_arg_item_t tof2_tm_qac_mem_top_rspec_mem_list[] = {
{ "qac_pipe_mem[0]", &tof2_tm_qac_pipe_mem_rspec_mem, 0x0, NULL },
{ "qac_pipe_mem[1]", &tof2_tm_qac_pipe_mem_rspec_mem, 0x10000000, NULL },
{ "qac_pipe_mem[2]", &tof2_tm_qac_pipe_mem_rspec_mem, 0x20000000, NULL },
{ "qac_pipe_mem[3]", &tof2_tm_qac_pipe_mem_rspec_mem, 0x30000000, NULL },
};
cmd_arg_t tof2_tm_qac_mem_top_rspec_mem = { 4, tof2_tm_qac_mem_top_rspec_mem_list };

cmd_arg_item_t tof2_tm_sch_pipe_mem_rspec_mem_list[] = {
{ "port_max_lb_static_mem", NULL, 0x0, &tof2_tm_sch_pipe_mem_rspec__port_max_lb_static_mem, 0x480 },
{ "port_max_lb_dynamic_mem", NULL, 0x80, &tof2_tm_sch_pipe_mem_rspec__port_max_lb_dynamic_mem, 0x480 },
{ "l1_min_lb_static_mem", NULL, 0x200, &tof2_tm_sch_pipe_mem_rspec__l1_min_lb_static_mem, 0x1200 },
{ "l1_min_lb_dynamic_mem", NULL, 0x400, &tof2_tm_sch_pipe_mem_rspec__l1_min_lb_dynamic_mem, 0x1200 },
{ "l1_max_lb_static_mem", NULL, 0x600, &tof2_tm_sch_pipe_mem_rspec__l1_max_lb_static_mem, 0x1200 },
{ "l1_max_lb_dynamic_mem", NULL, 0x800, &tof2_tm_sch_pipe_mem_rspec__l1_max_lb_dynamic_mem, 0x1200 },
{ "l1_exc_static_mem", NULL, 0xa00, &tof2_tm_sch_pipe_mem_rspec__l1_exc_static_mem, 0x1200 },
{ "l1_exc_dynamic_mem", NULL, 0xc00, &tof2_tm_sch_pipe_mem_rspec__l1_exc_dynamic_mem, 0x1200 },
{ "q_min_lb_static_mem", NULL, 0x1000, &tof2_tm_sch_pipe_mem_rspec__q_min_lb_static_mem, 0x4800 },
{ "q_min_lb_dynamic_mem", NULL, 0x1800, &tof2_tm_sch_pipe_mem_rspec__q_min_lb_dynamic_mem, 0x4800 },
{ "q_max_lb_static_mem", NULL, 0x2000, &tof2_tm_sch_pipe_mem_rspec__q_max_lb_static_mem, 0x4800 },
{ "q_max_lb_dynamic_mem", NULL, 0x2800, &tof2_tm_sch_pipe_mem_rspec__q_max_lb_dynamic_mem, 0x4800 },
{ "q_exc_static_mem", NULL, 0x3000, &tof2_tm_sch_pipe_mem_rspec__q_exc_static_mem, 0x4800 },
{ "q_exc_dynamic_mem", NULL, 0x3800, &tof2_tm_sch_pipe_mem_rspec__q_exc_dynamic_mem, 0x4800 },
{ "q_pfc_status_mem", NULL, 0x4000, &tof2_tm_sch_pipe_mem_rspec__q_pfc_status_mem, 0x4800 },
{ "q_adv_fc_status_mem", NULL, 0x4800, &tof2_tm_sch_pipe_mem_rspec__q_adv_fc_status_mem, 0x4800 },
{ "p_occ_mem", NULL, 0x4c80, &tof2_tm_sch_pipe_mem_rspec__p_occ_mem, 0x480 },
{ "l1_occ_mem", NULL, 0x4e00, &tof2_tm_sch_pipe_mem_rspec__l1_occ_mem, 0x1200 },
{ "q_occ_mem", NULL, 0x5000, &tof2_tm_sch_pipe_mem_rspec__q_occ_mem, 0x4800 },
};
cmd_arg_t tof2_tm_sch_pipe_mem_rspec_mem = { 19, tof2_tm_sch_pipe_mem_rspec_mem_list };

cmd_arg_item_t tof2_tm_sch_mem_top_rspec_mem_list[] = {
{ "sch_pipe_mem[0]", &tof2_tm_sch_pipe_mem_rspec_mem, 0x0, NULL },
{ "sch_pipe_mem[1]", &tof2_tm_sch_pipe_mem_rspec_mem, 0x10000000, NULL },
};
cmd_arg_t tof2_tm_sch_mem_top_rspec_mem = { 2, tof2_tm_sch_mem_top_rspec_mem_list };

cmd_arg_item_t tof2_tm_clc_pipe_mem_rspec_mem_list[] = {
{ "csr_memory_clc_clm", NULL, 0x0, &tof2_tm_clc_pipe_mem_rspec__csr_memory_clc_clm, 0x600000 },
};
cmd_arg_t tof2_tm_clc_pipe_mem_rspec_mem = { 1, tof2_tm_clc_pipe_mem_rspec_mem_list };

cmd_arg_item_t tof2_tm_clc_mem_top_rspec_mem_list[] = {
{ "clc[0]", &tof2_tm_clc_pipe_mem_rspec_mem, 0x0, NULL },
{ "clc[1]", &tof2_tm_clc_pipe_mem_rspec_mem, 0x10000000, NULL },
{ "clc[2]", &tof2_tm_clc_pipe_mem_rspec_mem, 0x20000000, NULL },
{ "clc[3]", &tof2_tm_clc_pipe_mem_rspec_mem, 0x30000000, NULL },
};
cmd_arg_t tof2_tm_clc_mem_top_rspec_mem = { 4, tof2_tm_clc_mem_top_rspec_mem_list };

cmd_arg_item_t tof2_tm_pex_pipe_mem_rspec_mem_list[] = {
{ "csr_memory_pex_clm", NULL, 0x0, &tof2_tm_pex_pipe_mem_rspec__csr_memory_pex_clm, 0x600000 },
};
cmd_arg_t tof2_tm_pex_pipe_mem_rspec_mem = { 1, tof2_tm_pex_pipe_mem_rspec_mem_list };

cmd_arg_item_t tof2_tm_pex_mem_top_rspec_mem_list[] = {
{ "pex[0]", &tof2_tm_pex_pipe_mem_rspec_mem, 0x0, NULL },
{ "pex[1]", &tof2_tm_pex_pipe_mem_rspec_mem, 0x10000000, NULL },
{ "pex[2]", &tof2_tm_pex_pipe_mem_rspec_mem, 0x20000000, NULL },
{ "pex[3]", &tof2_tm_pex_pipe_mem_rspec_mem, 0x30000000, NULL },
};
cmd_arg_t tof2_tm_pex_mem_top_rspec_mem = { 4, tof2_tm_pex_mem_top_rspec_mem_list };

cmd_arg_item_t tof2_tm_qlc_pipe_mem_rspec_mem_list[] = {
{ "csr_memory_qlc_qlm", NULL, 0x0, &tof2_tm_qlc_pipe_mem_rspec__csr_memory_qlc_qlm, 0x600000 },
{ "csr_memory_qlc_ht", NULL, 0x80000, &tof2_tm_qlc_pipe_mem_rspec__csr_memory_qlc_ht, 0x4800 },
{ "csr_memory_qlc_vq", NULL, 0x80800, &tof2_tm_qlc_pipe_mem_rspec__csr_memory_qlc_vq, 0x4800 },
};
cmd_arg_t tof2_tm_qlc_pipe_mem_rspec_mem = { 3, tof2_tm_qlc_pipe_mem_rspec_mem_list };

cmd_arg_item_t tof2_tm_qlc_mem_top_rspec_mem_list[] = {
{ "qlc_mem[0]", &tof2_tm_qlc_pipe_mem_rspec_mem, 0x0, NULL },
{ "qlc_mem[1]", &tof2_tm_qlc_pipe_mem_rspec_mem, 0x10000000, NULL },
{ "qlc_mem[2]", &tof2_tm_qlc_pipe_mem_rspec_mem, 0x20000000, NULL },
{ "qlc_mem[3]", &tof2_tm_qlc_pipe_mem_rspec_mem, 0x30000000, NULL },
};
cmd_arg_t tof2_tm_qlc_mem_top_rspec_mem = { 4, tof2_tm_qlc_mem_top_rspec_mem_list };

cmd_arg_item_t tof2_tm_prc_pipe_mem_rspec_mem_list[] = {
{ "csr_memory_prc_prm", NULL, 0x0, &tof2_tm_prc_pipe_mem_rspec__csr_memory_prc_prm, 0x180000 },
{ "csr_memory_prc_map", NULL, 0x20000, &tof2_tm_prc_pipe_mem_rspec__csr_memory_prc_map, 0x18000 },
{ "csr_memory_prc_cache0", NULL, 0x22000, &tof2_tm_prc_pipe_mem_rspec__csr_memory_prc_cache0, 0x2000 },
{ "csr_memory_prc_cache1", NULL, 0x22200, &tof2_tm_prc_pipe_mem_rspec__csr_memory_prc_cache1, 0x2000 },
{ "csr_memory_prc_tag", NULL, 0x22400, &tof2_tm_prc_pipe_mem_rspec__csr_memory_prc_tag, 0x2000 },
};
cmd_arg_t tof2_tm_prc_pipe_mem_rspec_mem = { 5, tof2_tm_prc_pipe_mem_rspec_mem_list };

cmd_arg_item_t tof2_tm_prc_mem_top_rspec_mem_list[] = {
{ "prc_mem[0]", &tof2_tm_prc_pipe_mem_rspec_mem, 0x0, NULL },
{ "prc_mem[1]", &tof2_tm_prc_pipe_mem_rspec_mem, 0x10000000, NULL },
{ "prc_mem[2]", &tof2_tm_prc_pipe_mem_rspec_mem, 0x20000000, NULL },
{ "prc_mem[3]", &tof2_tm_prc_pipe_mem_rspec_mem, 0x30000000, NULL },
};
cmd_arg_t tof2_tm_prc_mem_top_rspec_mem = { 4, tof2_tm_prc_mem_top_rspec_mem_list };

cmd_arg_item_t tof2_tm_pre_pipe_mem_rspec_mem_list[] = {
{ "mit_mem_word", NULL, 0x0, &tof2_tm_pre_pipe_mem_rspec__mit_mem_word, 0x40000 },
};
cmd_arg_t tof2_tm_pre_pipe_mem_rspec_mem = { 1, tof2_tm_pre_pipe_mem_rspec_mem_list };

cmd_arg_item_t tof2_tm_pre_common_mem_rspec_mem_list[] = {
{ "rdm_mem_word", NULL, 0x400000, &tof2_tm_pre_common_mem_rspec__rdm_mem_word, 0x800000 },
{ "pbt0_mem_word", NULL, 0x40000000, &tof2_tm_pre_common_mem_rspec__pbt0_mem_word, 0x1200 },
{ "pbt1_mem_word", NULL, 0x40100000, &tof2_tm_pre_common_mem_rspec__pbt1_mem_word, 0x1200 },
{ "lit0_np_mem_word", NULL, 0x40200000, &tof2_tm_pre_common_mem_rspec__lit0_np_mem_word, 0x1000 },
{ "lit1_np_mem_word", NULL, 0x40300000, &tof2_tm_pre_common_mem_rspec__lit1_np_mem_word, 0x1000 },
{ "lit0_bm_mem_word0", NULL, 0x80000000, &tof2_tm_pre_common_mem_rspec__lit0_bm_mem_word0, 0x1000 },
{ "lit0_bm_mem_word1", NULL, 0x80040000, &tof2_tm_pre_common_mem_rspec__lit0_bm_mem_word1, 0x1000 },
{ "lit0_bm_mem_word2", NULL, 0x80080000, &tof2_tm_pre_common_mem_rspec__lit0_bm_mem_word2, 0x1000 },
{ "lit0_bm_mem_word3", NULL, 0x800c0000, &tof2_tm_pre_common_mem_rspec__lit0_bm_mem_word3, 0x1000 },
{ "lit1_bm_mem_word0", NULL, 0x80100000, &tof2_tm_pre_common_mem_rspec__lit1_bm_mem_word0, 0x1000 },
{ "lit1_bm_mem_word1", NULL, 0x80140000, &tof2_tm_pre_common_mem_rspec__lit1_bm_mem_word1, 0x1000 },
{ "lit1_bm_mem_word2", NULL, 0x80180000, &tof2_tm_pre_common_mem_rspec__lit1_bm_mem_word2, 0x1000 },
{ "lit1_bm_mem_word3", NULL, 0x801c0000, &tof2_tm_pre_common_mem_rspec__lit1_bm_mem_word3, 0x1000 },
{ "pmt0_mem_word0", NULL, 0x80200000, &tof2_tm_pre_common_mem_rspec__pmt0_mem_word0, 0x1200 },
{ "pmt0_mem_word1", NULL, 0x80240000, &tof2_tm_pre_common_mem_rspec__pmt0_mem_word1, 0x1200 },
{ "pmt0_mem_word2", NULL, 0x80280000, &tof2_tm_pre_common_mem_rspec__pmt0_mem_word2, 0x1200 },
{ "pmt0_mem_word3", NULL, 0x802c0000, &tof2_tm_pre_common_mem_rspec__pmt0_mem_word3, 0x1200 },
{ "pmt1_mem_word0", NULL, 0x80300000, &tof2_tm_pre_common_mem_rspec__pmt1_mem_word0, 0x1200 },
{ "pmt1_mem_word1", NULL, 0x80340000, &tof2_tm_pre_common_mem_rspec__pmt1_mem_word1, 0x1200 },
{ "pmt1_mem_word2", NULL, 0x80380000, &tof2_tm_pre_common_mem_rspec__pmt1_mem_word2, 0x1200 },
{ "pmt1_mem_word3", NULL, 0x803c0000, &tof2_tm_pre_common_mem_rspec__pmt1_mem_word3, 0x1200 },
};
cmd_arg_t tof2_tm_pre_common_mem_rspec_mem = { 21, tof2_tm_pre_common_mem_rspec_mem_list };

cmd_arg_item_t tof2_tm_pre_mem_top_rspec_mem_list[] = {
{ "pre_pipe_mem[0]", &tof2_tm_pre_pipe_mem_rspec_mem, 0x0, NULL },
{ "pre_pipe_mem[1]", &tof2_tm_pre_pipe_mem_rspec_mem, 0x100000, NULL },
{ "pre_pipe_mem[2]", &tof2_tm_pre_pipe_mem_rspec_mem, 0x200000, NULL },
{ "pre_pipe_mem[3]", &tof2_tm_pre_pipe_mem_rspec_mem, 0x300000, NULL },
{ "pre_common_mem", &tof2_tm_pre_common_mem_rspec_mem, 0x100000000, NULL },
};
cmd_arg_t tof2_tm_pre_mem_top_rspec_mem = { 5, tof2_tm_pre_mem_top_rspec_mem_list };

cmd_arg_item_t tof2_tm_psc_mem_top_rspec_mem_list[] = {
{ "psc_block_grp0", NULL, 0x0, &tof2_tm_psc_mem_top_rspec__psc_block_grp0, 0x80000 },
{ "psc_block_grp1", NULL, 0x8000, &tof2_tm_psc_mem_top_rspec__psc_block_grp1, 0x80000 },
{ "psc_block_grp2", NULL, 0x10000, &tof2_tm_psc_mem_top_rspec__psc_block_grp2, 0x80000 },
{ "psc_block_grp3", NULL, 0x18000, &tof2_tm_psc_mem_top_rspec__psc_block_grp3, 0x80000 },
{ "psc_block_grp4", NULL, 0x20000, &tof2_tm_psc_mem_top_rspec__psc_block_grp4, 0x80000 },
{ "psc_block_grp5", NULL, 0x28000, &tof2_tm_psc_mem_top_rspec__psc_block_grp5, 0x80000 },
};
cmd_arg_t tof2_tm_psc_mem_top_rspec_mem = { 6, tof2_tm_psc_mem_top_rspec_mem_list };

cmd_arg_item_t tof2_tm_top_mem_rspec_mem_list[] = {
{ "tm_wac", &tof2_tm_wac_mem_top_rspec_mem, 0x4200000000, NULL },
{ "tm_caa", &tof2_tm_caa_mem_top_rspec_mem, 0x4600000000, NULL },
{ "tm_qac", &tof2_tm_qac_mem_top_rspec_mem, 0x4a00000000, NULL },
{ "tm_scha", &tof2_tm_sch_mem_top_rspec_mem, 0x4e00000000, NULL },
{ "tm_schb", &tof2_tm_sch_mem_top_rspec_mem, 0x5200000000, NULL },
{ "tm_clc", &tof2_tm_clc_mem_top_rspec_mem, 0x5600000000, NULL },
{ "tm_pex", &tof2_tm_pex_mem_top_rspec_mem, 0x5a00000000, NULL },
{ "tm_qlc", &tof2_tm_qlc_mem_top_rspec_mem, 0x5e00000000, NULL },
{ "tm_prc", &tof2_tm_prc_mem_top_rspec_mem, 0x6200000000, NULL },
{ "tm_pre", &tof2_tm_pre_mem_top_rspec_mem, 0x6600000000, NULL },
{ "tm_psc", &tof2_tm_psc_mem_top_rspec_mem, 0x6a00000000, NULL },
};
cmd_arg_t tof2_tm_top_mem_rspec_mem = { 11, tof2_tm_top_mem_rspec_mem_list };

cmd_arg_item_t tof2_prsr_mem_main_rspec_mem_list[] = {
{ "po_action_row", NULL, 0x0, &tof2_prsr_mem_main_rspec__po_action_row, 0 },
{ "ml_tcam_row", NULL, 0x400, &tof2_prsr_mem_main_rspec__ml_tcam_row, 0 },
{ "ml_ea_row", NULL, 0x500, &tof2_prsr_mem_main_rspec__ml_ea_row, 0 },
{ "ml_ctr_init_ram", NULL, 0x600, &tof2_prsr_mem_main_rspec__ml_ctr_init_ram, 0 },
{ "po_csum_ctrl_0_row", NULL, 0x620, &tof2_prsr_mem_main_rspec__po_csum_ctrl_0_row, 0 },
{ "po_csum_ctrl_1_row", NULL, 0x640, &tof2_prsr_mem_main_rspec__po_csum_ctrl_1_row, 0 },
{ "po_csum_ctrl_2_row", NULL, 0x660, &tof2_prsr_mem_main_rspec__po_csum_ctrl_2_row, 0 },
{ "po_csum_ctrl_3_row", NULL, 0x680, &tof2_prsr_mem_main_rspec__po_csum_ctrl_3_row, 0 },
{ "po_csum_ctrl_4_row", NULL, 0x6a0, &tof2_prsr_mem_main_rspec__po_csum_ctrl_4_row, 0 },
};
cmd_arg_t tof2_prsr_mem_main_rspec_mem = { 9, tof2_prsr_mem_main_rspec_mem_list };

cmd_arg_item_t tof2_pgr_mem_rspec_mem_list[] = {
{ "buffer_mem_word", NULL, 0x0, &tof2_pgr_mem_rspec__buffer_mem_word, 0 },
};
cmd_arg_t tof2_pgr_mem_rspec_mem = { 1, tof2_pgr_mem_rspec_mem_list };

cmd_arg_item_t tof2_pgr_ph0_rspec_mem_list[] = {
{ "phase0_mem_word", NULL, 0x0, &tof2_pgr_ph0_rspec__phase0_mem_word, 0 },
};
cmd_arg_t tof2_pgr_ph0_rspec_mem = { 1, tof2_pgr_ph0_rspec_mem_list };

cmd_arg_item_t tof2_parde_mem_mem_list[] = {
{ "i_prsr_mem[0]", &tof2_prsr_mem_main_rspec_mem, 0x0, NULL },
{ "i_prsr_mem[1]", &tof2_prsr_mem_main_rspec_mem, 0x800, NULL },
{ "i_prsr_mem[2]", &tof2_prsr_mem_main_rspec_mem, 0x1000, NULL },
{ "i_prsr_mem[3]", &tof2_prsr_mem_main_rspec_mem, 0x1800, NULL },
{ "i_prsr_mem[4]", &tof2_prsr_mem_main_rspec_mem, 0x2000, NULL },
{ "i_prsr_mem[5]", &tof2_prsr_mem_main_rspec_mem, 0x2800, NULL },
{ "i_prsr_mem[6]", &tof2_prsr_mem_main_rspec_mem, 0x3000, NULL },
{ "i_prsr_mem[7]", &tof2_prsr_mem_main_rspec_mem, 0x3800, NULL },
{ "i_prsr_mem[8]", &tof2_prsr_mem_main_rspec_mem, 0x4000, NULL },
{ "i_prsr_mem[9]", &tof2_prsr_mem_main_rspec_mem, 0x4800, NULL },
{ "i_prsr_mem[10]", &tof2_prsr_mem_main_rspec_mem, 0x5000, NULL },
{ "i_prsr_mem[11]", &tof2_prsr_mem_main_rspec_mem, 0x5800, NULL },
{ "i_prsr_mem[12]", &tof2_prsr_mem_main_rspec_mem, 0x6000, NULL },
{ "i_prsr_mem[13]", &tof2_prsr_mem_main_rspec_mem, 0x6800, NULL },
{ "i_prsr_mem[14]", &tof2_prsr_mem_main_rspec_mem, 0x7000, NULL },
{ "i_prsr_mem[15]", &tof2_prsr_mem_main_rspec_mem, 0x7800, NULL },
{ "i_prsr_mem[16]", &tof2_prsr_mem_main_rspec_mem, 0x8000, NULL },
{ "i_prsr_mem[17]", &tof2_prsr_mem_main_rspec_mem, 0x8800, NULL },
{ "i_prsr_mem[18]", &tof2_prsr_mem_main_rspec_mem, 0x9000, NULL },
{ "i_prsr_mem[19]", &tof2_prsr_mem_main_rspec_mem, 0x9800, NULL },
{ "i_prsr_mem[20]", &tof2_prsr_mem_main_rspec_mem, 0xa000, NULL },
{ "i_prsr_mem[21]", &tof2_prsr_mem_main_rspec_mem, 0xa800, NULL },
{ "i_prsr_mem[22]", &tof2_prsr_mem_main_rspec_mem, 0xb000, NULL },
{ "i_prsr_mem[23]", &tof2_prsr_mem_main_rspec_mem, 0xb800, NULL },
{ "i_prsr_mem[24]", &tof2_prsr_mem_main_rspec_mem, 0xc000, NULL },
{ "i_prsr_mem[25]", &tof2_prsr_mem_main_rspec_mem, 0xc800, NULL },
{ "i_prsr_mem[26]", &tof2_prsr_mem_main_rspec_mem, 0xd000, NULL },
{ "i_prsr_mem[27]", &tof2_prsr_mem_main_rspec_mem, 0xd800, NULL },
{ "i_prsr_mem[28]", &tof2_prsr_mem_main_rspec_mem, 0xe000, NULL },
{ "i_prsr_mem[29]", &tof2_prsr_mem_main_rspec_mem, 0xe800, NULL },
{ "i_prsr_mem[30]", &tof2_prsr_mem_main_rspec_mem, 0xf000, NULL },
{ "i_prsr_mem[31]", &tof2_prsr_mem_main_rspec_mem, 0xf800, NULL },
{ "i_prsr_mem[32]", &tof2_prsr_mem_main_rspec_mem, 0x10000, NULL },
{ "i_prsr_mem[33]", &tof2_prsr_mem_main_rspec_mem, 0x10800, NULL },
{ "i_prsr_mem[34]", &tof2_prsr_mem_main_rspec_mem, 0x11000, NULL },
{ "i_prsr_mem[35]", &tof2_prsr_mem_main_rspec_mem, 0x11800, NULL },
{ "e_prsr_mem[0]", &tof2_prsr_mem_main_rspec_mem, 0x20000, NULL },
{ "e_prsr_mem[1]", &tof2_prsr_mem_main_rspec_mem, 0x20800, NULL },
{ "e_prsr_mem[2]", &tof2_prsr_mem_main_rspec_mem, 0x21000, NULL },
{ "e_prsr_mem[3]", &tof2_prsr_mem_main_rspec_mem, 0x21800, NULL },
{ "e_prsr_mem[4]", &tof2_prsr_mem_main_rspec_mem, 0x22000, NULL },
{ "e_prsr_mem[5]", &tof2_prsr_mem_main_rspec_mem, 0x22800, NULL },
{ "e_prsr_mem[6]", &tof2_prsr_mem_main_rspec_mem, 0x23000, NULL },
{ "e_prsr_mem[7]", &tof2_prsr_mem_main_rspec_mem, 0x23800, NULL },
{ "e_prsr_mem[8]", &tof2_prsr_mem_main_rspec_mem, 0x24000, NULL },
{ "e_prsr_mem[9]", &tof2_prsr_mem_main_rspec_mem, 0x24800, NULL },
{ "e_prsr_mem[10]", &tof2_prsr_mem_main_rspec_mem, 0x25000, NULL },
{ "e_prsr_mem[11]", &tof2_prsr_mem_main_rspec_mem, 0x25800, NULL },
{ "e_prsr_mem[12]", &tof2_prsr_mem_main_rspec_mem, 0x26000, NULL },
{ "e_prsr_mem[13]", &tof2_prsr_mem_main_rspec_mem, 0x26800, NULL },
{ "e_prsr_mem[14]", &tof2_prsr_mem_main_rspec_mem, 0x27000, NULL },
{ "e_prsr_mem[15]", &tof2_prsr_mem_main_rspec_mem, 0x27800, NULL },
{ "e_prsr_mem[16]", &tof2_prsr_mem_main_rspec_mem, 0x28000, NULL },

{ "e_prsr_mem[17]", &tof2_prsr_mem_main_rspec_mem, 0x28800, NULL },
{ "e_prsr_mem[18]", &tof2_prsr_mem_main_rspec_mem, 0x29000, NULL },
{ "e_prsr_mem[19]", &tof2_prsr_mem_main_rspec_mem, 0x29800, NULL },
{ "e_prsr_mem[20]", &tof2_prsr_mem_main_rspec_mem, 0x2a000, NULL },
{ "e_prsr_mem[21]", &tof2_prsr_mem_main_rspec_mem, 0x2a800, NULL },
{ "e_prsr_mem[22]", &tof2_prsr_mem_main_rspec_mem, 0x2b000, NULL },
{ "e_prsr_mem[23]", &tof2_prsr_mem_main_rspec_mem, 0x2b800, NULL },
{ "e_prsr_mem[24]", &tof2_prsr_mem_main_rspec_mem, 0x2c000, NULL },
{ "e_prsr_mem[25]", &tof2_prsr_mem_main_rspec_mem, 0x2c800, NULL },
{ "e_prsr_mem[26]", &tof2_prsr_mem_main_rspec_mem, 0x2d000, NULL },
{ "e_prsr_mem[27]", &tof2_prsr_mem_main_rspec_mem, 0x2d800, NULL },
{ "e_prsr_mem[28]", &tof2_prsr_mem_main_rspec_mem, 0x2e000, NULL },
{ "e_prsr_mem[29]", &tof2_prsr_mem_main_rspec_mem, 0x2e800, NULL },
{ "e_prsr_mem[30]", &tof2_prsr_mem_main_rspec_mem, 0x2f000, NULL },
{ "e_prsr_mem[31]", &tof2_prsr_mem_main_rspec_mem, 0x2f800, NULL },
{ "e_prsr_mem[32]", &tof2_prsr_mem_main_rspec_mem, 0x30000, NULL },
{ "e_prsr_mem[33]", &tof2_prsr_mem_main_rspec_mem, 0x30800, NULL },
{ "e_prsr_mem[34]", &tof2_prsr_mem_main_rspec_mem, 0x31000, NULL },
{ "e_prsr_mem[35]", &tof2_prsr_mem_main_rspec_mem, 0x31800, NULL },
{ "pgr_mem_rspec", &tof2_pgr_mem_rspec_mem, 0x34000, NULL },
{ "pgr_ph0_rspec", &tof2_pgr_ph0_rspec_mem, 0x36000, NULL },
};
cmd_arg_t tof2_parde_mem_mem = { 74, tof2_parde_mem_mem_list };

cmd_arg_item_t tof2_pipe_addrmap_mem_list[] = {
{ "parde", &tof2_parde_mem_mem, 0x6080000000, NULL },
};
cmd_arg_t tof2_pipe_addrmap_mem = { 1, tof2_pipe_addrmap_mem_list };

cmd_arg_item_t tof2_jbay_mem_mem_list[] = {
{ "tm", &tof2_tm_top_mem_rspec_mem, 0x0, NULL },
{ "pipes[0]", &tof2_pipe_addrmap_mem, 0x20000000000, NULL },
{ "pipes[1]", &tof2_pipe_addrmap_mem, 0x28000000000, NULL },
{ "pipes[2]", &tof2_pipe_addrmap_mem, 0x30000000000, NULL },
{ "pipes[3]", &tof2_pipe_addrmap_mem, 0x38000000000, NULL },
};
cmd_arg_t tof2_mem = { 5, tof2_jbay_mem_mem_list };
