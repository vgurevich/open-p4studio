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
/*
Register Version: "20220210_170028_d900ae87c_ppu_tfc"
*/
static inline uint64_t tof2_mem_tm_tm_wac_wac_pipe_mem(uint32_t wac_pipe_mem_word) {
  return 0x42000000000 + (0x100000000 * wac_pipe_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_port_ppg_mapping(uint32_t wac_pipe_mem, uint32_t csr_memory_wac_port_ppg_mapping_word) {
  return 0x42000000000 + (0x100000000 * wac_pipe_mem) + (0x10 * csr_memory_wac_port_ppg_mapping_word);
}

static inline uint64_t tof2_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_ppg_min_cnt(uint32_t wac_pipe_mem, uint32_t csr_memory_wac_ppg_min_cnt_word) {
  return 0x42000001800 + (0x100000000 * wac_pipe_mem) + (0x10 * csr_memory_wac_ppg_min_cnt_word);
}

static inline uint64_t tof2_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_ppg_shr_cnt(uint32_t wac_pipe_mem, uint32_t csr_memory_wac_ppg_shr_cnt_word) {
  return 0x42000002000 + (0x100000000 * wac_pipe_mem) + (0x10 * csr_memory_wac_ppg_shr_cnt_word);
}

static inline uint64_t tof2_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_ppg_hdr_cnt(uint32_t wac_pipe_mem, uint32_t csr_memory_wac_ppg_hdr_cnt_word) {
  return 0x42000002800 + (0x100000000 * wac_pipe_mem) + (0x10 * csr_memory_wac_ppg_hdr_cnt_word);
}

static inline uint64_t tof2_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_ppg_min_th(uint32_t wac_pipe_mem, uint32_t csr_memory_wac_ppg_min_th_word) {
  return 0x42000003000 + (0x100000000 * wac_pipe_mem) + (0x10 * csr_memory_wac_ppg_min_th_word);
}

static inline uint64_t tof2_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_ppg_shr_th(uint32_t wac_pipe_mem, uint32_t csr_memory_wac_ppg_shr_th_word) {
  return 0x42000003800 + (0x100000000 * wac_pipe_mem) + (0x10 * csr_memory_wac_ppg_shr_th_word);
}

static inline uint64_t tof2_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_ppg_hdr_th(uint32_t wac_pipe_mem, uint32_t csr_memory_wac_ppg_hdr_th_word) {
  return 0x42000004000 + (0x100000000 * wac_pipe_mem) + (0x10 * csr_memory_wac_ppg_hdr_th_word);
}

static inline uint64_t tof2_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_ppg_pfc(uint32_t wac_pipe_mem, uint32_t csr_memory_wac_ppg_pfc_word) {
  return 0x42000004800 + (0x100000000 * wac_pipe_mem) + (0x10 * csr_memory_wac_ppg_pfc_word);
}

static inline uint64_t tof2_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_ppg_icos(uint32_t wac_pipe_mem, uint32_t csr_memory_wac_ppg_icos_word) {
  return 0x42000005000 + (0x100000000 * wac_pipe_mem) + (0x10 * csr_memory_wac_ppg_icos_word);
}

static inline uint64_t tof2_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_ppg_drop_st(uint32_t wac_pipe_mem, uint32_t csr_memory_wac_ppg_drop_st_word) {
  return 0x42000005800 + (0x100000000 * wac_pipe_mem) + (0x10 * csr_memory_wac_ppg_drop_st_word);
}

static inline uint64_t tof2_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_pg_drop_st(uint32_t wac_pipe_mem, uint32_t csr_memory_wac_pg_drop_st_word) {
  return 0x42000006000 + (0x100000000 * wac_pipe_mem) + (0x10 * csr_memory_wac_pg_drop_st_word);
}

static inline uint64_t tof2_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_ppg_off_idx(uint32_t wac_pipe_mem, uint32_t csr_memory_wac_ppg_off_idx_word) {
  return 0x42000006800 + (0x100000000 * wac_pipe_mem) + (0x10 * csr_memory_wac_ppg_off_idx_word);
}

static inline uint64_t tof2_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_pg_off_idx(uint32_t wac_pipe_mem, uint32_t csr_memory_wac_pg_off_idx_word) {
  return 0x42000007000 + (0x100000000 * wac_pipe_mem) + (0x10 * csr_memory_wac_pg_off_idx_word);
}

static inline uint64_t tof2_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_pg_min_cnt(uint32_t wac_pipe_mem, uint32_t csr_memory_wac_pg_min_cnt_word) {
  return 0x42000007800 + (0x100000000 * wac_pipe_mem) + (0x10 * csr_memory_wac_pg_min_cnt_word);
}

static inline uint64_t tof2_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_pg_shr_cnt(uint32_t wac_pipe_mem, uint32_t csr_memory_wac_pg_shr_cnt_word) {
  return 0x42000008000 + (0x100000000 * wac_pipe_mem) + (0x10 * csr_memory_wac_pg_shr_cnt_word);
}

static inline uint64_t tof2_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_pg_min_th(uint32_t wac_pipe_mem, uint32_t csr_memory_wac_pg_min_th_word) {
  return 0x42000008800 + (0x100000000 * wac_pipe_mem) + (0x10 * csr_memory_wac_pg_min_th_word);
}

static inline uint64_t tof2_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_pg_shr_th(uint32_t wac_pipe_mem, uint32_t csr_memory_wac_pg_shr_th_word) {
  return 0x42000009000 + (0x100000000 * wac_pipe_mem) + (0x10 * csr_memory_wac_pg_shr_th_word);
}

static inline uint64_t tof2_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_port_shr_th(uint32_t wac_pipe_mem, uint32_t csr_memory_wac_port_shr_th_word) {
  return 0x42000009800 + (0x100000000 * wac_pipe_mem) + (0x10 * csr_memory_wac_port_shr_th_word);
}

static inline uint64_t tof2_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_port_hdr_th(uint32_t wac_pipe_mem, uint32_t csr_memory_wac_port_hdr_th_word) {
  return 0x4200000a000 + (0x100000000 * wac_pipe_mem) + (0x10 * csr_memory_wac_port_hdr_th_word);
}

static inline uint64_t tof2_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_port_wm(uint32_t wac_pipe_mem, uint32_t csr_memory_wac_port_wm_word) {
  return 0x4200000a800 + (0x100000000 * wac_pipe_mem) + (0x10 * csr_memory_wac_port_wm_word);
}

static inline uint64_t tof2_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_port_min_cnt(uint32_t wac_pipe_mem, uint32_t csr_memory_wac_port_min_cnt_word) {
  return 0x4200000b000 + (0x100000000 * wac_pipe_mem) + (0x10 * csr_memory_wac_port_min_cnt_word);
}

static inline uint64_t tof2_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_port_hdr_cnt(uint32_t wac_pipe_mem, uint32_t csr_memory_wac_port_hdr_cnt_word) {
  return 0x4200000b800 + (0x100000000 * wac_pipe_mem) + (0x10 * csr_memory_wac_port_hdr_cnt_word);
}

static inline uint64_t tof2_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_port_shr_cnt(uint32_t wac_pipe_mem, uint32_t csr_memory_wac_port_shr_cnt_word) {
  return 0x4200000c000 + (0x100000000 * wac_pipe_mem) + (0x10 * csr_memory_wac_port_shr_cnt_word);
}

static inline uint64_t tof2_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_port_st(uint32_t wac_pipe_mem, uint32_t csr_memory_wac_port_st_word) {
  return 0x4200000c800 + (0x100000000 * wac_pipe_mem) + (0x10 * csr_memory_wac_port_st_word);
}

static inline uint64_t tof2_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_pg_wm_cnt(uint32_t wac_pipe_mem, uint32_t csr_memory_wac_pg_wm_cnt_word) {
  return 0x4200000d000 + (0x100000000 * wac_pipe_mem) + (0x10 * csr_memory_wac_pg_wm_cnt_word);
}

static inline uint64_t tof2_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_drop_count_ppg(uint32_t wac_pipe_mem, uint32_t csr_memory_wac_drop_count_ppg_word) {
  return 0x4200000e000 + (0x100000000 * wac_pipe_mem) + (0x10 * csr_memory_wac_drop_count_ppg_word);
}

static inline uint64_t tof2_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_drop_count_port(uint32_t wac_pipe_mem, uint32_t csr_memory_wac_drop_count_port_word) {
  return 0x4200000f000 + (0x100000000 * wac_pipe_mem) + (0x10 * csr_memory_wac_drop_count_port_word);
}

static inline uint64_t tof2_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_pfc_state(uint32_t wac_pipe_mem, uint32_t csr_memory_wac_pfc_state_word) {
  return 0x4200000f800 + (0x100000000 * wac_pipe_mem) + (0x10 * csr_memory_wac_pfc_state_word);
}

static inline uint64_t tof2_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_qid_map(uint32_t wac_pipe_mem, uint32_t csr_memory_wac_qid_map_word) {
  return 0x42000010000 + (0x100000000 * wac_pipe_mem) + (0x10 * csr_memory_wac_qid_map_word);
}

static inline uint64_t tof2_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_qacq_state(uint32_t wac_pipe_mem, uint32_t csr_memory_wac_qacq_state_word) {
  return 0x42000018000 + (0x100000000 * wac_pipe_mem) + (0x10 * csr_memory_wac_qacq_state_word);
}

static inline uint64_t tof2_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_qacq_ap_config(uint32_t wac_pipe_mem, uint32_t csr_memory_wac_qacq_ap_config_word) {
  return 0x42000020000 + (0x100000000 * wac_pipe_mem) + (0x10 * csr_memory_wac_qacq_ap_config_word);
}

static inline uint64_t tof2_mem_tm_tm_caa_caa_block_grp0(uint32_t caa_block_grp0_word) {
  return 0x46000000000 + (0x10 * caa_block_grp0_word);
}

static inline uint64_t tof2_mem_tm_tm_caa_caa_block_grp1(uint32_t caa_block_grp1_word) {
  return 0x46000080000 + (0x10 * caa_block_grp1_word);
}

static inline uint64_t tof2_mem_tm_tm_caa_caa_block_grp2(uint32_t caa_block_grp2_word) {
  return 0x46000100000 + (0x10 * caa_block_grp2_word);
}

static inline uint64_t tof2_mem_tm_tm_caa_caa_block_grp3(uint32_t caa_block_grp3_word) {
  return 0x46000180000 + (0x10 * caa_block_grp3_word);
}

static inline uint64_t tof2_mem_tm_tm_caa_caa_block_grp4(uint32_t caa_block_grp4_word) {
  return 0x46000200000 + (0x10 * caa_block_grp4_word);
}

static inline uint64_t tof2_mem_tm_tm_caa_caa_block_grp5(uint32_t caa_block_grp5_word) {
  return 0x46000280000 + (0x10 * caa_block_grp5_word);
}

static inline uint64_t tof2_mem_tm_tm_qac_qac_pipe_mem(uint32_t qac_pipe_mem_word) {
  return 0x4a000000000 + (0x100000000 * qac_pipe_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_queue_min_thrd_config(uint32_t qac_pipe_mem, uint32_t csr_memory_qac_queue_min_thrd_config_word) {
  return 0x4a000000000 + (0x100000000 * qac_pipe_mem) + (0x10 * csr_memory_qac_queue_min_thrd_config_word);
}

static inline uint64_t tof2_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_queue_shr_thrd_config(uint32_t qac_pipe_mem, uint32_t csr_memory_qac_queue_shr_thrd_config_word) {
  return 0x4a000008000 + (0x100000000 * qac_pipe_mem) + (0x10 * csr_memory_qac_queue_shr_thrd_config_word);
}

static inline uint64_t tof2_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_queue_ap_config(uint32_t qac_pipe_mem, uint32_t csr_memory_qac_queue_ap_config_word) {
  return 0x4a000010000 + (0x100000000 * qac_pipe_mem) + (0x10 * csr_memory_qac_queue_ap_config_word);
}

static inline uint64_t tof2_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_queue_color_limit(uint32_t qac_pipe_mem, uint32_t csr_memory_qac_queue_color_limit_word) {
  return 0x4a000018000 + (0x100000000 * qac_pipe_mem) + (0x10 * csr_memory_qac_queue_color_limit_word);
}

static inline uint64_t tof2_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_queue_cell_count(uint32_t qac_pipe_mem, uint32_t csr_memory_qac_queue_cell_count_word) {
  return 0x4a000020000 + (0x100000000 * qac_pipe_mem) + (0x10 * csr_memory_qac_queue_cell_count_word);
}

static inline uint64_t tof2_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_queue_wm_cell_count(uint32_t qac_pipe_mem, uint32_t csr_memory_qac_queue_wm_cell_count_word) {
  return 0x4a000028000 + (0x100000000 * qac_pipe_mem) + (0x10 * csr_memory_qac_queue_wm_cell_count_word);
}

static inline uint64_t tof2_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_port_wm_cell_count(uint32_t qac_pipe_mem, uint32_t csr_memory_qac_port_wm_cell_count_word) {
  return 0x4a00002c800 + (0x100000000 * qac_pipe_mem) + (0x10 * csr_memory_qac_port_wm_cell_count_word);
}

static inline uint64_t tof2_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_port_config(uint32_t qac_pipe_mem, uint32_t csr_memory_qac_port_config_word) {
  return 0x4a00002d000 + (0x100000000 * qac_pipe_mem) + (0x10 * csr_memory_qac_port_config_word);
}

static inline uint64_t tof2_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_port_cell_count(uint32_t qac_pipe_mem, uint32_t csr_memory_qac_port_cell_count_word) {
  return 0x4a00002d800 + (0x100000000 * qac_pipe_mem) + (0x10 * csr_memory_qac_port_cell_count_word);
}

static inline uint64_t tof2_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_drop_count_port(uint32_t qac_pipe_mem, uint32_t csr_memory_qac_drop_count_port_word) {
  return 0x4a00002e000 + (0x100000000 * qac_pipe_mem) + (0x10 * csr_memory_qac_drop_count_port_word);
}

static inline uint64_t tof2_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_drop_count_queue(uint32_t qac_pipe_mem, uint32_t csr_memory_qac_drop_count_queue_word) {
  return 0x4a000030000 + (0x100000000 * qac_pipe_mem) + (0x10 * csr_memory_qac_drop_count_queue_word);
}

static inline uint64_t tof2_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_qid_mapping(uint32_t qac_pipe_mem, uint32_t csr_memory_qac_qid_mapping_word) {
  return 0x4a000036000 + (0x100000000 * qac_pipe_mem) + (0x10 * csr_memory_qac_qid_mapping_word);
}

static inline uint64_t tof2_mem_tm_tm_scha_sch_pipe_mem_port_max_lb_static_mem(uint32_t sch_pipe_mem, uint32_t port_max_lb_static_mem_word) {
  return 0x4e000000000 + (0x100000000 * sch_pipe_mem) + (0x10 * port_max_lb_static_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_scha_sch_pipe_mem(uint32_t sch_pipe_mem_word) {
  return 0x4e000000000 + (0x100000000 * sch_pipe_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_scha_sch_pipe_mem_port_max_lb_dynamic_mem(uint32_t sch_pipe_mem, uint32_t port_max_lb_dynamic_mem_word) {
  return 0x4e000000800 + (0x100000000 * sch_pipe_mem) + (0x10 * port_max_lb_dynamic_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_scha_sch_pipe_mem_l1_min_lb_static_mem(uint32_t sch_pipe_mem, uint32_t l1_min_lb_static_mem_word) {
  return 0x4e000002000 + (0x100000000 * sch_pipe_mem) + (0x10 * l1_min_lb_static_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_scha_sch_pipe_mem_l1_min_lb_dynamic_mem(uint32_t sch_pipe_mem, uint32_t l1_min_lb_dynamic_mem_word) {
  return 0x4e000004000 + (0x100000000 * sch_pipe_mem) + (0x10 * l1_min_lb_dynamic_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_scha_sch_pipe_mem_l1_max_lb_static_mem(uint32_t sch_pipe_mem, uint32_t l1_max_lb_static_mem_word) {
  return 0x4e000006000 + (0x100000000 * sch_pipe_mem) + (0x10 * l1_max_lb_static_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_scha_sch_pipe_mem_l1_max_lb_dynamic_mem(uint32_t sch_pipe_mem, uint32_t l1_max_lb_dynamic_mem_word) {
  return 0x4e000008000 + (0x100000000 * sch_pipe_mem) + (0x10 * l1_max_lb_dynamic_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_scha_sch_pipe_mem_l1_exc_static_mem(uint32_t sch_pipe_mem, uint32_t l1_exc_static_mem_word) {
  return 0x4e00000a000 + (0x100000000 * sch_pipe_mem) + (0x10 * l1_exc_static_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_scha_sch_pipe_mem_l1_exc_dynamic_mem(uint32_t sch_pipe_mem, uint32_t l1_exc_dynamic_mem_word) {
  return 0x4e00000c000 + (0x100000000 * sch_pipe_mem) + (0x10 * l1_exc_dynamic_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_scha_sch_pipe_mem_q_min_lb_static_mem(uint32_t sch_pipe_mem, uint32_t q_min_lb_static_mem_word) {
  return 0x4e000010000 + (0x100000000 * sch_pipe_mem) + (0x10 * q_min_lb_static_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_scha_sch_pipe_mem_q_min_lb_dynamic_mem(uint32_t sch_pipe_mem, uint32_t q_min_lb_dynamic_mem_word) {
  return 0x4e000018000 + (0x100000000 * sch_pipe_mem) + (0x10 * q_min_lb_dynamic_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_scha_sch_pipe_mem_q_max_lb_static_mem(uint32_t sch_pipe_mem, uint32_t q_max_lb_static_mem_word) {
  return 0x4e000020000 + (0x100000000 * sch_pipe_mem) + (0x10 * q_max_lb_static_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_scha_sch_pipe_mem_q_max_lb_dynamic_mem(uint32_t sch_pipe_mem, uint32_t q_max_lb_dynamic_mem_word) {
  return 0x4e000028000 + (0x100000000 * sch_pipe_mem) + (0x10 * q_max_lb_dynamic_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_scha_sch_pipe_mem_q_exc_static_mem(uint32_t sch_pipe_mem, uint32_t q_exc_static_mem_word) {
  return 0x4e000030000 + (0x100000000 * sch_pipe_mem) + (0x10 * q_exc_static_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_scha_sch_pipe_mem_q_exc_dynamic_mem(uint32_t sch_pipe_mem, uint32_t q_exc_dynamic_mem_word) {
  return 0x4e000038000 + (0x100000000 * sch_pipe_mem) + (0x10 * q_exc_dynamic_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_scha_sch_pipe_mem_q_pfc_status_mem(uint32_t sch_pipe_mem, uint32_t q_pfc_status_mem_word) {
  return 0x4e000040000 + (0x100000000 * sch_pipe_mem) + (0x10 * q_pfc_status_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_scha_sch_pipe_mem_q_adv_fc_status_mem(uint32_t sch_pipe_mem, uint32_t q_adv_fc_status_mem_word) {
  return 0x4e000048000 + (0x100000000 * sch_pipe_mem) + (0x10 * q_adv_fc_status_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_scha_sch_pipe_mem_p_occ_mem(uint32_t sch_pipe_mem, uint32_t p_occ_mem_word) {
  return 0x4e00004c800 + (0x100000000 * sch_pipe_mem) + (0x10 * p_occ_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_scha_sch_pipe_mem_l1_occ_mem(uint32_t sch_pipe_mem, uint32_t l1_occ_mem_word) {
  return 0x4e00004e000 + (0x100000000 * sch_pipe_mem) + (0x10 * l1_occ_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_scha_sch_pipe_mem_q_occ_mem(uint32_t sch_pipe_mem, uint32_t q_occ_mem_word) {
  return 0x4e000050000 + (0x100000000 * sch_pipe_mem) + (0x10 * q_occ_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_schb_sch_pipe_mem_port_max_lb_static_mem(uint32_t sch_pipe_mem, uint32_t port_max_lb_static_mem_word) {
  return 0x52000000000 + (0x100000000 * sch_pipe_mem) + (0x10 * port_max_lb_static_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_schb_sch_pipe_mem(uint32_t sch_pipe_mem_word) {
  return 0x52000000000 + (0x100000000 * sch_pipe_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_schb_sch_pipe_mem_port_max_lb_dynamic_mem(uint32_t sch_pipe_mem, uint32_t port_max_lb_dynamic_mem_word) {
  return 0x52000000800 + (0x100000000 * sch_pipe_mem) + (0x10 * port_max_lb_dynamic_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_schb_sch_pipe_mem_l1_min_lb_static_mem(uint32_t sch_pipe_mem, uint32_t l1_min_lb_static_mem_word) {
  return 0x52000002000 + (0x100000000 * sch_pipe_mem) + (0x10 * l1_min_lb_static_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_schb_sch_pipe_mem_l1_min_lb_dynamic_mem(uint32_t sch_pipe_mem, uint32_t l1_min_lb_dynamic_mem_word) {
  return 0x52000004000 + (0x100000000 * sch_pipe_mem) + (0x10 * l1_min_lb_dynamic_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_schb_sch_pipe_mem_l1_max_lb_static_mem(uint32_t sch_pipe_mem, uint32_t l1_max_lb_static_mem_word) {
  return 0x52000006000 + (0x100000000 * sch_pipe_mem) + (0x10 * l1_max_lb_static_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_schb_sch_pipe_mem_l1_max_lb_dynamic_mem(uint32_t sch_pipe_mem, uint32_t l1_max_lb_dynamic_mem_word) {
  return 0x52000008000 + (0x100000000 * sch_pipe_mem) + (0x10 * l1_max_lb_dynamic_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_schb_sch_pipe_mem_l1_exc_static_mem(uint32_t sch_pipe_mem, uint32_t l1_exc_static_mem_word) {
  return 0x5200000a000 + (0x100000000 * sch_pipe_mem) + (0x10 * l1_exc_static_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_schb_sch_pipe_mem_l1_exc_dynamic_mem(uint32_t sch_pipe_mem, uint32_t l1_exc_dynamic_mem_word) {
  return 0x5200000c000 + (0x100000000 * sch_pipe_mem) + (0x10 * l1_exc_dynamic_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_schb_sch_pipe_mem_q_min_lb_static_mem(uint32_t sch_pipe_mem, uint32_t q_min_lb_static_mem_word) {
  return 0x52000010000 + (0x100000000 * sch_pipe_mem) + (0x10 * q_min_lb_static_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_schb_sch_pipe_mem_q_min_lb_dynamic_mem(uint32_t sch_pipe_mem, uint32_t q_min_lb_dynamic_mem_word) {
  return 0x52000018000 + (0x100000000 * sch_pipe_mem) + (0x10 * q_min_lb_dynamic_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_schb_sch_pipe_mem_q_max_lb_static_mem(uint32_t sch_pipe_mem, uint32_t q_max_lb_static_mem_word) {
  return 0x52000020000 + (0x100000000 * sch_pipe_mem) + (0x10 * q_max_lb_static_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_schb_sch_pipe_mem_q_max_lb_dynamic_mem(uint32_t sch_pipe_mem, uint32_t q_max_lb_dynamic_mem_word) {
  return 0x52000028000 + (0x100000000 * sch_pipe_mem) + (0x10 * q_max_lb_dynamic_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_schb_sch_pipe_mem_q_exc_static_mem(uint32_t sch_pipe_mem, uint32_t q_exc_static_mem_word) {
  return 0x52000030000 + (0x100000000 * sch_pipe_mem) + (0x10 * q_exc_static_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_schb_sch_pipe_mem_q_exc_dynamic_mem(uint32_t sch_pipe_mem, uint32_t q_exc_dynamic_mem_word) {
  return 0x52000038000 + (0x100000000 * sch_pipe_mem) + (0x10 * q_exc_dynamic_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_schb_sch_pipe_mem_q_pfc_status_mem(uint32_t sch_pipe_mem, uint32_t q_pfc_status_mem_word) {
  return 0x52000040000 + (0x100000000 * sch_pipe_mem) + (0x10 * q_pfc_status_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_schb_sch_pipe_mem_q_adv_fc_status_mem(uint32_t sch_pipe_mem, uint32_t q_adv_fc_status_mem_word) {
  return 0x52000048000 + (0x100000000 * sch_pipe_mem) + (0x10 * q_adv_fc_status_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_schb_sch_pipe_mem_p_occ_mem(uint32_t sch_pipe_mem, uint32_t p_occ_mem_word) {
  return 0x5200004c800 + (0x100000000 * sch_pipe_mem) + (0x10 * p_occ_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_schb_sch_pipe_mem_l1_occ_mem(uint32_t sch_pipe_mem, uint32_t l1_occ_mem_word) {
  return 0x5200004e000 + (0x100000000 * sch_pipe_mem) + (0x10 * l1_occ_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_schb_sch_pipe_mem_q_occ_mem(uint32_t sch_pipe_mem, uint32_t q_occ_mem_word) {
  return 0x52000050000 + (0x100000000 * sch_pipe_mem) + (0x10 * q_occ_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_clc_clc_csr_memory_clc_clm(uint32_t clc, uint32_t csr_memory_clc_clm_word) {
  return 0x56000000000 + (0x100000000 * clc) + (0x10 * csr_memory_clc_clm_word);
}

static inline uint64_t tof2_mem_tm_tm_clc_clc(uint32_t clc_word) {
  return 0x56000000000 + (0x100000000 * clc_word);
}

static inline uint64_t tof2_mem_tm_tm_pex_pex_csr_memory_pex_clm(uint32_t pex, uint32_t csr_memory_pex_clm_word) {
  return 0x5a000000000 + (0x100000000 * pex) + (0x10 * csr_memory_pex_clm_word);
}

static inline uint64_t tof2_mem_tm_tm_pex_pex(uint32_t pex_word) {
  return 0x5a000000000 + (0x100000000 * pex_word);
}

static inline uint64_t tof2_mem_tm_tm_qlc_qlc_mem(uint32_t qlc_mem_word) {
  return 0x5e000000000 + (0x100000000 * qlc_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_qlc_qlc_mem_csr_memory_qlc_qlm(uint32_t qlc_mem, uint32_t csr_memory_qlc_qlm_word) {
  return 0x5e000000000 + (0x100000000 * qlc_mem) + (0x10 * csr_memory_qlc_qlm_word);
}

static inline uint64_t tof2_mem_tm_tm_qlc_qlc_mem_csr_memory_qlc_ht(uint32_t qlc_mem, uint32_t csr_memory_qlc_ht_word) {
  return 0x5e000800000 + (0x100000000 * qlc_mem) + (0x10 * csr_memory_qlc_ht_word);
}

static inline uint64_t tof2_mem_tm_tm_qlc_qlc_mem_csr_memory_qlc_vq(uint32_t qlc_mem, uint32_t csr_memory_qlc_vq_word) {
  return 0x5e000808000 + (0x100000000 * qlc_mem) + (0x10 * csr_memory_qlc_vq_word);
}

static inline uint64_t tof2_mem_tm_tm_prc_prc_mem(uint32_t prc_mem_word) {
  return 0x62000000000 + (0x100000000 * prc_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_prc_prc_mem_csr_memory_prc_prm(uint32_t prc_mem, uint32_t csr_memory_prc_prm_word) {
  return 0x62000000000 + (0x100000000 * prc_mem) + (0x10 * csr_memory_prc_prm_word);
}

static inline uint64_t tof2_mem_tm_tm_prc_prc_mem_csr_memory_prc_map(uint32_t prc_mem, uint32_t csr_memory_prc_map_word) {
  return 0x62000200000 + (0x100000000 * prc_mem) + (0x10 * csr_memory_prc_map_word);
}

static inline uint64_t tof2_mem_tm_tm_prc_prc_mem_csr_memory_prc_cache0(uint32_t prc_mem, uint32_t csr_memory_prc_cache0_word) {
  return 0x62000220000 + (0x100000000 * prc_mem) + (0x10 * csr_memory_prc_cache0_word);
}

static inline uint64_t tof2_mem_tm_tm_prc_prc_mem_csr_memory_prc_cache1(uint32_t prc_mem, uint32_t csr_memory_prc_cache1_word) {
  return 0x62000222000 + (0x100000000 * prc_mem) + (0x10 * csr_memory_prc_cache1_word);
}

static inline uint64_t tof2_mem_tm_tm_prc_prc_mem_csr_memory_prc_tag(uint32_t prc_mem, uint32_t csr_memory_prc_tag_word) {
  return 0x62000224000 + (0x100000000 * prc_mem) + (0x10 * csr_memory_prc_tag_word);
}

static inline uint64_t tof2_mem_tm_tm_pre_pre_pipe_mem(uint32_t pre_pipe_mem_word) {
  return 0x66000000000 + (0x1000000 * pre_pipe_mem_word);
}

static inline uint64_t tof2_mem_tm_tm_pre_pre_pipe_mem_mit_mem_word(uint32_t pre_pipe_mem, uint32_t mit_mem_word_word) {
  return 0x66000000000 + (0x1000000 * pre_pipe_mem) + (0x10 * mit_mem_word_word);
}

static inline uint64_t tof2_mem_tm_tm_pre_pre_common_mem_rdm_mem_word(uint32_t rdm_mem_word_word) {
  return 0x67004000000 + (0x10 * rdm_mem_word_word);
}

static inline uint64_t tof2_mem_tm_tm_pre_pre_common_mem_pbt0_mem_word(uint32_t pbt0_mem_word_word) {
  return 0x67400000000 + (0x10 * pbt0_mem_word_word);
}

static inline uint64_t tof2_mem_tm_tm_pre_pre_common_mem_pbt1_mem_word(uint32_t pbt1_mem_word_word) {
  return 0x67401000000 + (0x10 * pbt1_mem_word_word);
}

static inline uint64_t tof2_mem_tm_tm_pre_pre_common_mem_lit0_np_mem_word(uint32_t lit0_np_mem_word_word) {
  return 0x67402000000 + (0x10 * lit0_np_mem_word_word);
}

static inline uint64_t tof2_mem_tm_tm_pre_pre_common_mem_lit1_np_mem_word(uint32_t lit1_np_mem_word_word) {
  return 0x67403000000 + (0x10 * lit1_np_mem_word_word);
}

static inline uint64_t tof2_mem_tm_tm_pre_pre_common_mem_lit0_bm_mem_word0(uint32_t lit0_bm_mem_word0_word) {
  return 0x67800000000 + (0x10 * lit0_bm_mem_word0_word);
}

static inline uint64_t tof2_mem_tm_tm_pre_pre_common_mem_lit0_bm_mem_word1(uint32_t lit0_bm_mem_word1_word) {
  return 0x67800400000 + (0x10 * lit0_bm_mem_word1_word);
}

static inline uint64_t tof2_mem_tm_tm_pre_pre_common_mem_lit0_bm_mem_word2(uint32_t lit0_bm_mem_word2_word) {
  return 0x67800800000 + (0x10 * lit0_bm_mem_word2_word);
}

static inline uint64_t tof2_mem_tm_tm_pre_pre_common_mem_lit0_bm_mem_word3(uint32_t lit0_bm_mem_word3_word) {
  return 0x67800c00000 + (0x10 * lit0_bm_mem_word3_word);
}

static inline uint64_t tof2_mem_tm_tm_pre_pre_common_mem_lit1_bm_mem_word0(uint32_t lit1_bm_mem_word0_word) {
  return 0x67801000000 + (0x10 * lit1_bm_mem_word0_word);
}

static inline uint64_t tof2_mem_tm_tm_pre_pre_common_mem_lit1_bm_mem_word1(uint32_t lit1_bm_mem_word1_word) {
  return 0x67801400000 + (0x10 * lit1_bm_mem_word1_word);
}

static inline uint64_t tof2_mem_tm_tm_pre_pre_common_mem_lit1_bm_mem_word2(uint32_t lit1_bm_mem_word2_word) {
  return 0x67801800000 + (0x10 * lit1_bm_mem_word2_word);
}

static inline uint64_t tof2_mem_tm_tm_pre_pre_common_mem_lit1_bm_mem_word3(uint32_t lit1_bm_mem_word3_word) {
  return 0x67801c00000 + (0x10 * lit1_bm_mem_word3_word);
}

static inline uint64_t tof2_mem_tm_tm_pre_pre_common_mem_pmt0_mem_word0(uint32_t pmt0_mem_word0_word) {
  return 0x67802000000 + (0x10 * pmt0_mem_word0_word);
}

static inline uint64_t tof2_mem_tm_tm_pre_pre_common_mem_pmt0_mem_word1(uint32_t pmt0_mem_word1_word) {
  return 0x67802400000 + (0x10 * pmt0_mem_word1_word);
}

static inline uint64_t tof2_mem_tm_tm_pre_pre_common_mem_pmt0_mem_word2(uint32_t pmt0_mem_word2_word) {
  return 0x67802800000 + (0x10 * pmt0_mem_word2_word);
}

static inline uint64_t tof2_mem_tm_tm_pre_pre_common_mem_pmt0_mem_word3(uint32_t pmt0_mem_word3_word) {
  return 0x67802c00000 + (0x10 * pmt0_mem_word3_word);
}

static inline uint64_t tof2_mem_tm_tm_pre_pre_common_mem_pmt1_mem_word0(uint32_t pmt1_mem_word0_word) {
  return 0x67803000000 + (0x10 * pmt1_mem_word0_word);
}

static inline uint64_t tof2_mem_tm_tm_pre_pre_common_mem_pmt1_mem_word1(uint32_t pmt1_mem_word1_word) {
  return 0x67803400000 + (0x10 * pmt1_mem_word1_word);
}

static inline uint64_t tof2_mem_tm_tm_pre_pre_common_mem_pmt1_mem_word2(uint32_t pmt1_mem_word2_word) {
  return 0x67803800000 + (0x10 * pmt1_mem_word2_word);
}

static inline uint64_t tof2_mem_tm_tm_pre_pre_common_mem_pmt1_mem_word3(uint32_t pmt1_mem_word3_word) {
  return 0x67803c00000 + (0x10 * pmt1_mem_word3_word);
}

static inline uint64_t tof2_mem_tm_tm_psc_psc_block_grp0(uint32_t psc_block_grp0_word) {
  return 0x6a000000000 + (0x10 * psc_block_grp0_word);
}

static inline uint64_t tof2_mem_tm_tm_psc_psc_block_grp1(uint32_t psc_block_grp1_word) {
  return 0x6a000080000 + (0x10 * psc_block_grp1_word);
}

static inline uint64_t tof2_mem_tm_tm_psc_psc_block_grp2(uint32_t psc_block_grp2_word) {
  return 0x6a000100000 + (0x10 * psc_block_grp2_word);
}

static inline uint64_t tof2_mem_tm_tm_psc_psc_block_grp3(uint32_t psc_block_grp3_word) {
  return 0x6a000180000 + (0x10 * psc_block_grp3_word);
}

static inline uint64_t tof2_mem_tm_tm_psc_psc_block_grp4(uint32_t psc_block_grp4_word) {
  return 0x6a000200000 + (0x10 * psc_block_grp4_word);
}

static inline uint64_t tof2_mem_tm_tm_psc_psc_block_grp5(uint32_t psc_block_grp5_word) {
  return 0x6a000280000 + (0x10 * psc_block_grp5_word);
}

static inline uint64_t tof2_mem_pipes_mau(uint32_t pipes, uint32_t mau_word) {
  return 0x200000000000 + (0x80000000000 * pipes) + (0x4000000000 * mau_word);
}

static inline uint64_t tof2_mem_pipes(uint32_t pipes_word) {
  return 0x200000000000 + (0x80000000000 * pipes_word);
}

static inline uint64_t tof2_mem_pipes_parde_i_prsr_mem(uint32_t pipes, uint32_t i_prsr_mem_word) {
  return 0x260800000000 + (0x80000000000 * pipes) + (0x8000 * i_prsr_mem_word);
}

static inline uint64_t tof2_mem_pipes_parde_i_prsr_mem_po_action_row(uint32_t pipes, uint32_t i_prsr_mem, uint32_t po_action_row_word) {
  return 0x260800000000 + (0x80000000000 * pipes) + (0x8000 * i_prsr_mem) + (0x40 * po_action_row_word);
}

static inline uint64_t tof2_mem_pipes_parde_i_prsr_mem_ml_tcam_row(uint32_t pipes, uint32_t i_prsr_mem, uint32_t ml_tcam_row_word) {
  return 0x260800004000 + (0x80000000000 * pipes) + (0x8000 * i_prsr_mem) + (0x10 * ml_tcam_row_word);
}

static inline uint64_t tof2_mem_pipes_parde_i_prsr_mem_ml_ea_row(uint32_t pipes, uint32_t i_prsr_mem, uint32_t ml_ea_row_word) {
  return 0x260800005000 + (0x80000000000 * pipes) + (0x8000 * i_prsr_mem) + (0x10 * ml_ea_row_word);
}

static inline uint64_t tof2_mem_pipes_parde_i_prsr_mem_ml_ctr_init_ram(uint32_t pipes, uint32_t i_prsr_mem, uint32_t ml_ctr_init_ram_word) {
  return 0x260800006000 + (0x80000000000 * pipes) + (0x8000 * i_prsr_mem) + (0x10 * ml_ctr_init_ram_word);
}

static inline uint64_t tof2_mem_pipes_parde_i_prsr_mem_po_csum_ctrl_0_row(uint32_t pipes, uint32_t i_prsr_mem, uint32_t po_csum_ctrl_0_row_word) {
  return 0x260800006200 + (0x80000000000 * pipes) + (0x8000 * i_prsr_mem) + (0x10 * po_csum_ctrl_0_row_word);
}

static inline uint64_t tof2_mem_pipes_parde_i_prsr_mem_po_csum_ctrl_1_row(uint32_t pipes, uint32_t i_prsr_mem, uint32_t po_csum_ctrl_1_row_word) {
  return 0x260800006400 + (0x80000000000 * pipes) + (0x8000 * i_prsr_mem) + (0x10 * po_csum_ctrl_1_row_word);
}

static inline uint64_t tof2_mem_pipes_parde_i_prsr_mem_po_csum_ctrl_2_row(uint32_t pipes, uint32_t i_prsr_mem, uint32_t po_csum_ctrl_2_row_word) {
  return 0x260800006600 + (0x80000000000 * pipes) + (0x8000 * i_prsr_mem) + (0x10 * po_csum_ctrl_2_row_word);
}

static inline uint64_t tof2_mem_pipes_parde_i_prsr_mem_po_csum_ctrl_3_row(uint32_t pipes, uint32_t i_prsr_mem, uint32_t po_csum_ctrl_3_row_word) {
  return 0x260800006800 + (0x80000000000 * pipes) + (0x8000 * i_prsr_mem) + (0x10 * po_csum_ctrl_3_row_word);
}

static inline uint64_t tof2_mem_pipes_parde_i_prsr_mem_po_csum_ctrl_4_row(uint32_t pipes, uint32_t i_prsr_mem, uint32_t po_csum_ctrl_4_row_word) {
  return 0x260800006a00 + (0x80000000000 * pipes) + (0x8000 * i_prsr_mem) + (0x10 * po_csum_ctrl_4_row_word);
}

static inline uint64_t tof2_mem_pipes_parde_e_prsr_mem(uint32_t pipes, uint32_t e_prsr_mem_word) {
  return 0x260800200000 + (0x80000000000 * pipes) + (0x8000 * e_prsr_mem_word);
}

static inline uint64_t tof2_mem_pipes_parde_e_prsr_mem_po_action_row(uint32_t pipes, uint32_t e_prsr_mem, uint32_t po_action_row_word) {
  return 0x260800200000 + (0x80000000000 * pipes) + (0x8000 * e_prsr_mem) + (0x40 * po_action_row_word);
}

static inline uint64_t tof2_mem_pipes_parde_e_prsr_mem_ml_tcam_row(uint32_t pipes, uint32_t e_prsr_mem, uint32_t ml_tcam_row_word) {
  return 0x260800204000 + (0x80000000000 * pipes) + (0x8000 * e_prsr_mem) + (0x10 * ml_tcam_row_word);
}

static inline uint64_t tof2_mem_pipes_parde_e_prsr_mem_ml_ea_row(uint32_t pipes, uint32_t e_prsr_mem, uint32_t ml_ea_row_word) {
  return 0x260800205000 + (0x80000000000 * pipes) + (0x8000 * e_prsr_mem) + (0x10 * ml_ea_row_word);
}

static inline uint64_t tof2_mem_pipes_parde_e_prsr_mem_ml_ctr_init_ram(uint32_t pipes, uint32_t e_prsr_mem, uint32_t ml_ctr_init_ram_word) {
  return 0x260800206000 + (0x80000000000 * pipes) + (0x8000 * e_prsr_mem) + (0x10 * ml_ctr_init_ram_word);
}

static inline uint64_t tof2_mem_pipes_parde_e_prsr_mem_po_csum_ctrl_0_row(uint32_t pipes, uint32_t e_prsr_mem, uint32_t po_csum_ctrl_0_row_word) {
  return 0x260800206200 + (0x80000000000 * pipes) + (0x8000 * e_prsr_mem) + (0x10 * po_csum_ctrl_0_row_word);
}

static inline uint64_t tof2_mem_pipes_parde_e_prsr_mem_po_csum_ctrl_1_row(uint32_t pipes, uint32_t e_prsr_mem, uint32_t po_csum_ctrl_1_row_word) {
  return 0x260800206400 + (0x80000000000 * pipes) + (0x8000 * e_prsr_mem) + (0x10 * po_csum_ctrl_1_row_word);
}

static inline uint64_t tof2_mem_pipes_parde_e_prsr_mem_po_csum_ctrl_2_row(uint32_t pipes, uint32_t e_prsr_mem, uint32_t po_csum_ctrl_2_row_word) {
  return 0x260800206600 + (0x80000000000 * pipes) + (0x8000 * e_prsr_mem) + (0x10 * po_csum_ctrl_2_row_word);
}

static inline uint64_t tof2_mem_pipes_parde_e_prsr_mem_po_csum_ctrl_3_row(uint32_t pipes, uint32_t e_prsr_mem, uint32_t po_csum_ctrl_3_row_word) {
  return 0x260800206800 + (0x80000000000 * pipes) + (0x8000 * e_prsr_mem) + (0x10 * po_csum_ctrl_3_row_word);
}

static inline uint64_t tof2_mem_pipes_parde_e_prsr_mem_po_csum_ctrl_4_row(uint32_t pipes, uint32_t e_prsr_mem, uint32_t po_csum_ctrl_4_row_word) {
  return 0x260800206a00 + (0x80000000000 * pipes) + (0x8000 * e_prsr_mem) + (0x10 * po_csum_ctrl_4_row_word);
}

static inline uint64_t tof2_mem_pipes_parde_pgr_mem_rspec(uint32_t pipes, uint32_t pgr_mem_rspec_word) {
  return 0x260800340000 + (0x80000000000 * pipes) + (0x10 * pgr_mem_rspec_word);
}

static inline uint64_t tof2_mem_pipes_parde_pgr_ph0_rspec(uint32_t pipes, uint32_t pgr_ph0_rspec_word) {
  return 0x260800360000 + (0x80000000000 * pipes) + (0x10 * pgr_ph0_rspec_word);
}

/* clang-format on */
