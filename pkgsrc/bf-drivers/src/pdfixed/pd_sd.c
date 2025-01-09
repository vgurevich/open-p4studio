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


#include <tofino/pdfixed/pd_sd.h>
#include <port_mgr/bf_port_if.h>
#include <port_mgr/bf_serdes_if.h>
#include <lld/lld_err.h>
#include <lld/lld_sku.h>

p4_pd_status_t p4_pd_sd_mgmt_clksel_set(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        int lane,
                                        int clk_sel) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_mgmt_clksel_set(
      dev_id, dev_port, lane, (bf_sds_mgmt_clk_clksel_t)clk_sel);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)clk_sel;
#endif
}

p4_pd_status_t p4_pd_sd_mgmt_clksel_get(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        int lane,
                                        int *clk_sel) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_mgmt_clksel_get(
      dev_id, dev_port, lane, (bf_sds_mgmt_clk_clksel_t *)clk_sel);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)clk_sel;
#endif
}

p4_pd_status_t p4_pd_sd_mgmt_access_method_set(bf_dev_id_t dev_id, int method) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_mgmt_access_method_set(dev_id,
                                          (bf_serdes_access_method_e)method);
#else
  return 0;
  (void)dev_id;
  (void)method;
#endif
}

p4_pd_status_t p4_pd_sd_mgmt_access_method_get(bf_dev_id_t dev_id,
                                               int *method) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_mgmt_access_method_get(dev_id,
                                          (bf_serdes_access_method_e *)method);
#else
  return 0;
  (void)dev_id;
  (void)method;
#endif
}

p4_pd_status_t p4_pd_sd_mgmt_bcast_set(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       int lane,
                                       bool tx_dir,
                                       bool en) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_mgmt_bcast_set(dev_id, dev_port, lane, tx_dir, en);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)tx_dir;
  (void)en;
#endif
}

p4_pd_status_t p4_pd_sd_mgmt_bcast_get(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       int lane,
                                       bool tx_dir,
                                       bool *en) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_mgmt_bcast_get(dev_id, dev_port, lane, tx_dir, en);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)tx_dir;
  (void)en;
#endif
}

p4_pd_status_t p4_pd_sd_mgmt_reg_set(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port,
                                     int lane,
                                     bool tx_dir,
                                     int reg,
                                     int data) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_mgmt_reg_set(
      dev_id, dev_port, lane, tx_dir, reg, (uint32_t)data);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)tx_dir;
  (void)reg;
  (void)data;
#endif
}

p4_pd_status_t p4_pd_sd_mgmt_reg_get(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port,
                                     int lane,
                                     bool tx_dir,
                                     int reg,
                                     int *data) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_mgmt_reg_get(
      dev_id, dev_port, lane, tx_dir, reg, (uint32_t *)data);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)tx_dir;
  (void)reg;
  (void)data;
#endif
}

p4_pd_status_t p4_pd_sd_mgmt_uc_int(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port,
                                    int lane,
                                    bool tx_dir,
                                    int interrupt,
                                    int int_data,
                                    int *rtn_data) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_mgmt_uc_int(dev_id,
                               dev_port,
                               lane,
                               tx_dir,
                               interrupt,
                               (uint32_t)int_data,
                               (uint32_t *)rtn_data);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)tx_dir;
  (void)interrupt;
  (void)int_data;
  (void)rtn_data;
#endif
}

p4_pd_status_t p4_pd_sd_port_lane_map_set(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          int tx[4],
                                          int rx[4]) {
#ifdef INCLUDE_SERDES_PKG
  bf_mac_block_lane_map_t lane_map;
  uint32_t mac_blk, channel;
  lld_err_t err;

  lane_map.tx_lane[0] = tx[0];
  lane_map.tx_lane[1] = tx[1];
  lane_map.tx_lane[2] = tx[2];
  lane_map.tx_lane[3] = tx[3];
  lane_map.rx_lane[0] = rx[0];
  lane_map.rx_lane[1] = rx[1];
  lane_map.rx_lane[2] = rx[2];
  lane_map.rx_lane[3] = rx[3];
  lane_map.dev_port = dev_port;

  err = lld_sku_map_dev_port_id_to_mac_ch(dev_id, dev_port, &mac_blk, &channel);
  if (err != LLD_OK) {
    return BF_INVALID_ARG;
  }

  return bf_port_lane_map_set(dev_id, mac_blk, &lane_map);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)tx;
  (void)rx;
#endif
}

p4_pd_status_t p4_pd_sd_port_lane_map_get(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          int tx[4],
                                          int rx[4]) {
#ifdef INCLUDE_SERDES_PKG
  bf_mac_block_lane_map_t lane_map;
  p4_pd_status_t sts;

  sts = bf_port_lane_map_get(dev_id, dev_port, &lane_map);
  tx[0] = lane_map.tx_lane[0];
  tx[1] = lane_map.tx_lane[1];
  tx[2] = lane_map.tx_lane[2];
  tx[3] = lane_map.tx_lane[3];
  rx[0] = lane_map.rx_lane[0];
  rx[1] = lane_map.rx_lane[1];
  rx[2] = lane_map.rx_lane[2];
  rx[3] = lane_map.rx_lane[3];

  return sts;
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)tx;
  (void)rx;
#endif
}

p4_pd_status_t p4_pd_sd_dev_rx_eq_cal_rr_set(bf_dev_id_t dev_id,
                                             int fine_tune_lane_cnt) {
#ifdef INCLUDE_SERDES_PKG
  return bf_dev_rx_eq_cal_rr_set(dev_id, fine_tune_lane_cnt);
#else
  return 0;
  (void)dev_id;
  (void)fine_tune_lane_cnt;
#endif
}

p4_pd_status_t p4_pd_sd_dev_rx_eq_cal_rr_get(bf_dev_id_t dev_id,
                                             int *fine_tune_lane_cnt) {
#ifdef INCLUDE_SERDES_PKG
  return bf_dev_rx_eq_cal_rr_get(dev_id, fine_tune_lane_cnt);
#else
  return 0;
  (void)dev_id;
  (void)fine_tune_lane_cnt;
#endif
}

p4_pd_status_t p4_pd_sd_tx_pll_clksel_set(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          int lane,
                                          int clk_source) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_tx_pll_clksel_set(
      dev_id, dev_port, lane, (bf_sds_tx_pll_clksel_t)clk_source);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)clk_source;
#endif
}

p4_pd_status_t p4_pd_sd_tx_pll_clksel_get(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          int lane,
                                          int *clk_source) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_tx_pll_clksel_get(
      dev_id, dev_port, lane, (bf_sds_tx_pll_clksel_t *)clk_source);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)clk_source;
#endif
}

p4_pd_status_t p4_pd_sd_lane_init_run(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      int lane,
                                      int line_rate,
                                      bool init_rx,
                                      bool init_tx,
                                      bool tx_drv_en,
                                      bool phase_cal) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_lane_init_run(dev_id,
                                 dev_port,
                                 lane,
                                 line_rate,
                                 init_rx,
                                 init_tx,
                                 tx_drv_en,
                                 phase_cal);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)line_rate;
  (void)init_rx;
  (void)init_tx;
  (void)tx_drv_en;
  (void)phase_cal;
#endif
}

p4_pd_status_t p4_pd_sd_tx_pll_lock_get(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        int lane,
                                        bool *locked) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_tx_pll_lock_get(dev_id, dev_port, lane, locked);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)locked;
#endif
}

p4_pd_status_t p4_pd_sd_rx_cdr_lock_get(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        int lane,
                                        bool *locked) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_rx_cdr_lock_get(dev_id, dev_port, lane, locked);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)locked;
#endif
}

p4_pd_status_t p4_pd_sd_tx_pll_status_get(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          int lane,
                                          bool *locked,
                                          int *div,
                                          int *freq) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_tx_pll_status_get(dev_id, dev_port, lane, locked, div, freq);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)locked;
  (void)div;
  (void)freq;
#endif
}

p4_pd_status_t p4_pd_sd_rx_cdr_status_get(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          int lane,
                                          bool *locked,
                                          int *div,
                                          int *freq) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_rx_cdr_status_get(dev_id, dev_port, lane, locked, div, freq);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)locked;
  (void)div;
  (void)freq;
#endif
}

p4_pd_status_t p4_pd_sd_lane_loopback_set(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          int lane,
                                          int loopback_mode) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_lane_loopback_set(
      dev_id, dev_port, lane, (bf_sds_loopback_t)loopback_mode);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)loopback_mode;
#endif
}

p4_pd_status_t p4_pd_sd_lane_loopback_get(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          int lane,
                                          int *loopback_mode) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_lane_loopback_get(
      dev_id, dev_port, lane, (bf_sds_loopback_t *)loopback_mode);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)loopback_mode;
#endif
}

p4_pd_status_t p4_pd_sd_tx_en_set(bf_dev_id_t dev_id,
                                  bf_dev_port_t dev_port,
                                  int lane,
                                  bool en) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_tx_en_set(dev_id, dev_port, lane, en);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)en;
#endif
}

p4_pd_status_t p4_pd_sd_tx_en_get(bf_dev_id_t dev_id,
                                  bf_dev_port_t dev_port,
                                  int lane,
                                  bool *en) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_tx_en_get(dev_id, dev_port, lane, en);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)en;
#endif
}

p4_pd_status_t p4_pd_sd_tx_drv_en_set(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      int lane,
                                      bool en) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_tx_drv_en_set(dev_id, dev_port, lane, en);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)en;
#endif
}

p4_pd_status_t p4_pd_sd_tx_drv_en_get(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      int lane,
                                      bool *en) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_tx_drv_en_get(dev_id, dev_port, lane, en);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)en;
#endif
}

p4_pd_status_t p4_pd_sd_tx_drv_inv_set(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       int lane,
                                       bool inv) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_tx_drv_inv_set(dev_id, dev_port, lane, inv);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)inv;
#endif
}

p4_pd_status_t p4_pd_sd_tx_drv_inv_get(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       int lane,
                                       bool *inv) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_tx_drv_inv_get(dev_id, dev_port, lane, inv);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)inv;
#endif
}

p4_pd_status_t p4_pd_sd_tx_drv_attn_is_valid(int attn_main,
                                             int attn_post,
                                             int attn_pre) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_tx_drv_attn_is_valid(attn_main, attn_post, attn_pre);
#else
  return 0;
  (void)attn_main;
  (void)attn_post;
  (void)attn_pre;
#endif
}

p4_pd_status_t p4_pd_sd_tx_drv_attn_set(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        int lane,
                                        int attn_main,
                                        int attn_post,
                                        int attn_pre) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_tx_drv_attn_set(
      dev_id, dev_port, lane, attn_main, attn_post, attn_pre);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)attn_main;
  (void)attn_post;
  (void)attn_pre;
#endif
}

p4_pd_status_t p4_pd_sd_tx_drv_attn_get(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        int lane,
                                        int *attn_main,
                                        int *attn_post,
                                        int *attn_pre) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_tx_drv_attn_get(
      dev_id, dev_port, lane, attn_main, attn_post, attn_pre);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)attn_main;
  (void)attn_post;
  (void)attn_pre;
#endif
}

p4_pd_status_t p4_pd_sd_tx_drv_amp_set(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       int lane,
                                       int amp_main,
                                       int amp_post,
                                       int amp_pre) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_tx_drv_amp_set(
      dev_id, dev_port, lane, amp_main, amp_post, amp_pre);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)amp_main;
  (void)amp_post;
  (void)amp_pre;
#endif
}

p4_pd_status_t p4_pd_sd_tx_drv_amp_get(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       int lane,
                                       int *amp_main,
                                       int *amp_post,
                                       int *amp_pre) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_tx_drv_amp_get(
      dev_id, dev_port, lane, amp_main, amp_post, amp_pre);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)amp_main;
  (void)amp_post;
  (void)amp_pre;
#endif
}

p4_pd_status_t p4_pd_sd_tx_drv_status_get(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          int lane,
                                          bool *tx_en,
                                          bool *tx_drv_en,
                                          bool *tx_inv,
                                          int *amp_main,
                                          int *amp_post,
                                          int *amp_pre) {
#ifdef INCLUDE_SERDES_PKG
  bf_sds_tx_drv_status_t ds;
  p4_pd_status_t status;
  status = bf_serdes_tx_drv_status_get(dev_id, dev_port, lane, &ds);
  if (status == 0) {
    *tx_en = ds.tx_en;
    *tx_drv_en = ds.tx_drv_en;
    *tx_inv = ds.tx_inv;
    *amp_main = ds.amp_main;
    *amp_post = ds.amp_post;
    *amp_pre = ds.amp_pre;
  }
  return status;
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)tx_en;
  (void)tx_drv_en;
  (void)tx_inv;
  (void)amp_main;
  (void)amp_post;
  (void)amp_pre;
#endif
}

p4_pd_status_t p4_pd_sd_rx_en_set(bf_dev_id_t dev_id,
                                  bf_dev_port_t dev_port,
                                  int lane,
                                  bool en) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_rx_en_set(dev_id, dev_port, lane, en);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)en;
#endif
}

p4_pd_status_t p4_pd_sd_rx_en_get(bf_dev_id_t dev_id,
                                  bf_dev_port_t dev_port,
                                  int lane,
                                  bool *en) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_rx_en_get(dev_id, dev_port, lane, en);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)en;
#endif
}

p4_pd_status_t p4_pd_sd_rx_afe_inv_set(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       int lane,
                                       bool inv) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_rx_afe_inv_set(dev_id, dev_port, lane, inv);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)inv;
#endif
}

p4_pd_status_t p4_pd_sd_rx_afe_inv_get(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       int lane,
                                       bool *inv) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_rx_afe_inv_get(dev_id, dev_port, lane, inv);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)inv;
#endif
}

p4_pd_status_t p4_pd_sd_rx_afe_term_set(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        int lane,
                                        int rx_term) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_rx_afe_term_set(
      dev_id, dev_port, lane, (bf_sds_rx_term_t)rx_term);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)rx_term;
#endif
}

p4_pd_status_t p4_pd_sd_rx_afe_term_get(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        int lane,
                                        int *rx_term) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_rx_afe_term_get(
      dev_id, dev_port, lane, (bf_sds_rx_term_t *)rx_term);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)rx_term;
#endif
}

p4_pd_status_t p4_pd_sd_rx_afe_los_thres_set(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             int lane,
                                             bool rx_los_en,
                                             int rx_los_thres) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_rx_afe_los_thres_set(
      dev_id, dev_port, lane, rx_los_en, rx_los_thres);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)rx_los_en;
  (void)rx_los_thres;
#endif
}

p4_pd_status_t p4_pd_sd_rx_afe_los_thres_get(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             int lane,
                                             bool *rx_los_en,
                                             int *rx_los_thres) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_rx_afe_los_thres_get(
      dev_id, dev_port, lane, rx_los_en, rx_los_thres);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)rx_los_en;
  (void)rx_los_thres;
#endif
}

p4_pd_status_t p4_pd_sd_rx_afe_los_get(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       int lane,
                                       bool *rx_los) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_rx_afe_los_get(dev_id, dev_port, lane, rx_los);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)rx_los;
#endif
}

p4_pd_status_t p4_pd_sd_rx_eq_cal_busy_get(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           int lane,
                                           int chk_cnt,
                                           int chk_wait,
                                           bool *uc_busy) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_rx_eq_cal_busy_get(
      dev_id, dev_port, lane, chk_cnt, chk_wait, uc_busy);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)chk_cnt;
  (void)chk_wait;
  (void)uc_busy;
#endif
}

p4_pd_status_t p4_pd_sd_rx_eq_ctle_set(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       int lane,
                                       int ctle_dc,
                                       int ctle_lf,
                                       int ctle_hf,
                                       int ctle_bw) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_rx_eq_ctle_set(
      dev_id, dev_port, lane, ctle_dc, ctle_lf, ctle_hf, ctle_bw);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)ctle_dc;
  (void)ctle_lf;
  (void)ctle_hf;
  (void)ctle_bw;
#endif
}

p4_pd_status_t p4_pd_sd_rx_eq_ctle_get(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       int lane,
                                       int *ctle_dc,
                                       int *ctle_lf,
                                       int *ctle_hf,
                                       int *ctle_bw) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_rx_eq_ctle_get(
      dev_id, dev_port, lane, ctle_dc, ctle_lf, ctle_hf, ctle_bw);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)ctle_dc;
  (void)ctle_lf;
  (void)ctle_hf;
  (void)ctle_bw;
#endif
}

p4_pd_status_t p4_pd_sd_rx_eq_dfe_adv_set(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          int lane,
                                          int tap_num,
                                          int tap_val) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_rx_eq_dfe_adv_set(dev_id, dev_port, lane, tap_num, tap_val);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)tap_num;
  (void)tap_val;
#endif
}

p4_pd_status_t p4_pd_sd_rx_eq_dfe_adv_get(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          int lane,
                                          int tap_num,
                                          int *tap_val) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_rx_eq_dfe_adv_get(dev_id, dev_port, lane, tap_num, tap_val);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)tap_num;
  (void)tap_val;
#endif
}

p4_pd_status_t p4_pd_sd_rx_eq_dfe_set(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      int lane,
                                      int dfe_gain,
                                      int tap1,
                                      int tap2,
                                      int tap3,
                                      int tap4) {
#ifdef INCLUDE_SERDES_PKG
  int taps[4];

  taps[0] = tap1;
  taps[1] = tap2;
  taps[2] = tap3;
  taps[3] = tap4;
  return bf_serdes_rx_eq_dfe_set(dev_id, dev_port, lane, dfe_gain, taps);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)dfe_gain;
  (void)tap1;
  (void)tap2;
  (void)tap3;
  (void)tap4;
#endif
}

p4_pd_status_t p4_pd_sd_rx_eq_dfe_get(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      int lane,
                                      int *dfe_gain,
                                      int *tap1,
                                      int *tap2,
                                      int *tap3,
                                      int *tap4) {
#ifdef INCLUDE_SERDES_PKG
  int taps[4];
  p4_pd_status_t status;

  status = bf_serdes_rx_eq_dfe_get(dev_id, dev_port, lane, dfe_gain, taps);
  if (status == 0) {
    *tap1 = taps[0];
    *tap2 = taps[1];
    *tap3 = taps[2];
    *tap4 = taps[3];
  }
  return status;
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)dfe_gain;
  (void)tap1;
  (void)tap2;
  (void)tap3;
  (void)tap4;
#endif
}

p4_pd_status_t p4_pd_sd_rx_eq_cal_param_set(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            int lane,
                                            int ctle_dc_hint,
                                            int dfe_gain_range,
                                            int pcal_loop_cnt) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_rx_eq_cal_param_set(
      dev_id, dev_port, lane, ctle_dc_hint, dfe_gain_range, pcal_loop_cnt);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)ctle_dc_hint;
  (void)dfe_gain_range;
  (void)pcal_loop_cnt;
#endif
}

p4_pd_status_t p4_pd_sd_rx_eq_cal_param_get(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            int lane,
                                            int *ctle_dc_hint,
                                            int *dfe_gain_range,
                                            int *pcal_loop_cnt) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_rx_eq_cal_param_get(
      dev_id, dev_port, lane, ctle_dc_hint, dfe_gain_range, pcal_loop_cnt);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)ctle_dc_hint;
  (void)dfe_gain_range;
  (void)pcal_loop_cnt;
#endif
}

p4_pd_status_t p4_pd_sd_rx_eq_cal_adv_run(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          int lane,
                                          int cal_cmd,
                                          int ctle_cal_cfg,
                                          int dfe_fixed) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_rx_eq_cal_adv_run(
      dev_id, dev_port, lane, cal_cmd, ctle_cal_cfg, dfe_fixed);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)cal_cmd;
  (void)ctle_cal_cfg;
  (void)dfe_fixed;
#endif
}

p4_pd_status_t p4_pd_sd_rx_eq_ical_run(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       int lane) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_rx_eq_ical_run(dev_id, dev_port, lane);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
#endif
}

p4_pd_status_t p4_pd_sd_rx_eq_cal_eye_get(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          int lane,
                                          int *cal_eye) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_rx_eq_cal_eye_get(dev_id, dev_port, lane, cal_eye);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)cal_eye;
#endif
}

p4_pd_status_t p4_pd_sd_rx_eq_ical_eye_get(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           int lane,
                                           int cal_good_thres,
                                           int *cal_done,
                                           int *cal_good,
                                           int *cal_eye) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_rx_eq_ical_eye_get(dev_id,
                                      dev_port,
                                      lane,
                                      cal_good_thres,
                                      (bool *)cal_done,
                                      (bool *)cal_good,
                                      cal_eye);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)cal_good_thres;
  (void)cal_done;
  (void)cal_good;
  (void)cal_eye;
#endif
}

p4_pd_status_t p4_pd_sd_rx_eq_pcal_run(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       int lane,
                                       int cal_count) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_rx_eq_pcal_run(dev_id, dev_port, lane, cal_count);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)cal_count;
#endif
}

p4_pd_status_t p4_pd_sd_rx_eq_status_get(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         int lane,
                                         rx_eq_status_t *st) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_rx_eq_status_get(
      dev_id, dev_port, lane, (bf_sds_rx_eq_status_t *)st);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)st;
#endif
}

p4_pd_status_t p4_pd_sd_rx_eye_offset_set(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          int lane,
                                          int offset_en,
                                          int pos_x,
                                          int pos_y) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_rx_eye_offset_set(
      dev_id, dev_port, lane, offset_en, pos_x, pos_y);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)offset_en;
  (void)pos_x;
  (void)pos_y;
#endif
}

p4_pd_status_t p4_pd_sd_rx_eye_get(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   int lane,
                                   int meas_mode,
                                   int meas_ber,
                                   int *meas_eye) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_rx_eye_get(
      dev_id, dev_port, lane, meas_mode, meas_ber, meas_eye);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)meas_mode;
  (void)meas_ber;
  (void)meas_eye;
#endif
}

p4_pd_status_t p4_pd_sd_rx_eye_3d_get(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      int lane,
                                      int meas_ber,
                                      char *meas_eye,
                                      int max_eye_data) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_rx_eye_3d_get(
      dev_id, dev_port, lane, meas_ber, meas_eye, max_eye_data);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)meas_ber;
  (void)meas_eye;
  (void)max_eye_data;
#endif
}

p4_pd_status_t p4_pd_sd_tx_err_inj_set(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       int lane,
                                       int num_bits) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_tx_err_inj_set(dev_id, dev_port, lane, num_bits);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)num_bits;
#endif
}

p4_pd_status_t p4_pd_sd_rx_err_inj_set(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       int lane,
                                       int num_bits) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_rx_err_inj_set(dev_id, dev_port, lane, num_bits);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)num_bits;
#endif
}

p4_pd_status_t p4_pd_sd_tx_patsel_set(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      int lane,
                                      int tx_patsel) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_tx_patsel_set(
      dev_id, dev_port, lane, (bf_sds_pat_patsel_t)tx_patsel);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)tx_patsel;
#endif
}

p4_pd_status_t p4_pd_sd_tx_patsel_get(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      int lane,
                                      int *tx_patsel) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_tx_patsel_get(
      dev_id, dev_port, lane, (bf_sds_pat_patsel_t *)tx_patsel);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)tx_patsel;
#endif
}

p4_pd_status_t p4_pd_sd_rx_patsel_set(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      int lane,
                                      int rx_patsel) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_rx_patsel_set(
      dev_id, dev_port, lane, (bf_sds_pat_patsel_t)rx_patsel);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)rx_patsel;
#endif
}

p4_pd_status_t p4_pd_sd_rx_patsel_get(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      int lane,
                                      int *rx_patsel) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_rx_patsel_get(
      dev_id, dev_port, lane, (bf_sds_pat_patsel_t *)rx_patsel);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)rx_patsel;
#endif
}

p4_pd_status_t p4_pd_sd_rx_err_cnt_get(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       int lane,
                                       uint32_t *err_cnt) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_rx_err_cnt_get(dev_id, dev_port, lane, err_cnt);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)err_cnt;
#endif
}

p4_pd_status_t p4_pd_sd_tx_fixed_pat_set(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         int lane,
                                         int tx_fixed_pat_0,
                                         int tx_fixed_pat_1,
                                         int tx_fixed_pat_2,
                                         int tx_fixed_pat_3) {
#ifdef INCLUDE_SERDES_PKG
  int fixed_pat[4];

  fixed_pat[0] = tx_fixed_pat_0;
  fixed_pat[1] = tx_fixed_pat_1;
  fixed_pat[2] = tx_fixed_pat_2;
  fixed_pat[3] = tx_fixed_pat_3;
  return bf_serdes_tx_fixed_pat_set(dev_id, dev_port, lane, fixed_pat);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)tx_fixed_pat_0;
  (void)tx_fixed_pat_1;
  (void)tx_fixed_pat_2;
  (void)tx_fixed_pat_3;
#endif
}

p4_pd_status_t p4_pd_sd_tx_fixed_pat_get(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         int lane,
                                         int *tx_fixed_pat_0,
                                         int *tx_fixed_pat_1,
                                         int *tx_fixed_pat_2,
                                         int *tx_fixed_pat_3) {
#ifdef INCLUDE_SERDES_PKG
  p4_pd_status_t status;
  int fixed_pat[4] = {0};

  status = bf_serdes_tx_fixed_pat_get(dev_id, dev_port, lane, fixed_pat);
  *tx_fixed_pat_0 = fixed_pat[0];
  *tx_fixed_pat_1 = fixed_pat[1];
  *tx_fixed_pat_2 = fixed_pat[2];
  *tx_fixed_pat_3 = fixed_pat[3];
  return status;
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)tx_fixed_pat_0;
  (void)tx_fixed_pat_1;
  (void)tx_fixed_pat_2;
  (void)tx_fixed_pat_3;
#endif
}

p4_pd_status_t p4_pd_sd_rx_data_cap_get(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        int lane,
                                        int *rx_cap_pat_0,
                                        int *rx_cap_pat_1,
                                        int *rx_cap_pat_2,
                                        int *rx_cap_pat_3) {
#ifdef INCLUDE_SERDES_PKG
  int fixed_pat[4];
  p4_pd_status_t status;

  status = bf_serdes_rx_data_cap_get(dev_id, dev_port, lane, fixed_pat);
  *rx_cap_pat_0 = fixed_pat[0];
  *rx_cap_pat_1 = fixed_pat[1];
  *rx_cap_pat_2 = fixed_pat[2];
  *rx_cap_pat_3 = fixed_pat[3];
  return status;
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)rx_cap_pat_0;
  (void)rx_cap_pat_1;
  (void)rx_cap_pat_2;
  (void)rx_cap_pat_3;
#endif
}

p4_pd_status_t p4_pd_sd_get_tx_eq(bf_dev_id_t dev_id,
                                  bf_dev_port_t dev_port,
                                  int lane,
                                  int *pre,
                                  int *atten,
                                  int *post) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_get_tx_eq(dev_id, dev_port, lane, pre, atten, post);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)pre;
  (void)atten;
  (void)post;
#endif
}

p4_pd_status_t p4_pd_sd_set_tx_eq(bf_dev_id_t dev_id,
                                  bf_dev_port_t dev_port,
                                  int lane,
                                  int pre,
                                  int atten,
                                  int post) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_set_tx_eq(dev_id, dev_port, lane, pre, atten, post);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)pre;
  (void)atten;
  (void)post;
#endif
}

p4_pd_status_t p4_pd_sd_get_pll_state(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      int lane,
                                      int expected_divider) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_get_pll_state(dev_id, dev_port, lane, expected_divider);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)expected_divider;
#endif
}

p4_pd_status_t p4_pd_sd_get_tx_output_en(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         int lane,
                                         bool *en) {
#ifdef INCLUDE_SERDES_PKG
  return bf_serdes_get_tx_output_en(dev_id, dev_port, lane, en);
#else
  return 0;
  (void)dev_id;
  (void)dev_port;
  (void)lane;
  (void)en;
#endif
}
